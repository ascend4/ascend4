/* :ex: set ts=2 */
/*	ASCEND modelling environment
	Copyright 1997, Carnegie Mellon University
	Copyright (C) 2006-2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	LSODE Integrator

	Here we are solving the system of first order ordinary differential
	equations, ydot = F(y,t), with y(t_0)=y_0 given.

	In some places 'x' is named instead of 't'. We're trying to migrate to using
	't' to label the independent variable.

	There is some excellent documentation about the LSODE integrator available
	in the document "Description and Use of LSODE, the Livermore Solver for
	Ordinary Differential Equations, by K Radhakrishnan and A C Hindmarsh.
	http://www.llnl.gov/CASC/nsde/pubs/u113855.pdf

	(old implementation notes:)

	As fortran io is unreliably portable (vc5+digital fortran)
	we have converted xerrwv to xascwv provided here.

	The lsode interface variable t is actually an array of
	2 doubles rather than just 1. The first is the the one
	used by lsode. The second is used by LSODE_FEX to tell
	what the last time it was called was. This is so the
	C driver can tell if it needs to resolve d to compute
	observation variables. If x[0]==x[1] we save ourselves
	a solve.

	@NOTE The above doesn't work since lsode doesn't use the same t internally
	that we hand it. @ENDNOTE
*//*
	original version by Kirk Abbott and Ben Allan, Jan 1994.
	Last in CVS $Revision: 1.29 $ $Date: 2000/01/25 02:26:31 $ $Author: ballan $
*/

#include <time.h>
#ifndef CLOCKS_PER_SEC
# error "Where is CLOCKS_PER_SEC?"
#endif

#include <utilities/config.h>

#ifdef ASC_SIGNAL_TRAPS
# include <signal.h>
# include <general/except.h>
#endif

#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <compiler/instance_enum.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>

#include "slv_types.h"
#include "mtx.h"
#include "rel.h"
#include "var.h"
#include "discrete.h"
#include "conditional.h"
#include "bnd.h"
#include "logrel.h"
#include "slv_common.h"
#include "linsol.h"
#include "linsolqr.h"
#include "slv_client.h"
#include "slv_stdcalls.h"
#include <packages/sensitivity.h>
#include "densemtx.h"

#include "integrator.h"
#include "lsode.h"

/* #define TIMING_DEBUG */

const IntegratorInternals integrator_lsode_internals = {
	integrator_lsode_create
	,integrator_lsode_params_default
	,integrator_analyse_ode /* note, this routine is back in integrator.c */
	,integrator_lsode_solve
	,integrator_lsode_write_matrix
	,integrator_lsode_free
	,INTEG_LSODE
	,"LSODE"
};

/*
#include "Sensitivity.h"
*//* see the packages dir */

/*
 *  NOUNDERBARS --> FORTRAN compiler naming convention for subroutine
 *  is wierd. WIN32/CRAY is treated as special case
 */
#ifdef APOLLO
#define NOUNDERBARS TRUE
#endif
#ifdef _HPUX_SOURCE
#define NOUNDERBARS TRUE
#endif
/* AIX xlf will not suffix an underbar on a symbol
 * unless xlf is given the ``-qextname'' option
 */
#ifdef _AIX
#define NOUNDERBARS TRUE
#endif

#ifdef NOUNDERBARS
#define LSODE lsode
#define LSODE_JEX jex
#define LSODE_FEX fex
#define GETCOMMON get_lsode_common
#define XASCWV xascwv
#else
/* sun, __alpha, __sgi, ... */
#define LSODE lsode_
#define LSODE_JEX jex_
#define LSODE_FEX fex_
#define GETCOMMON get_lsode_common_
#define XASCWV xascwv_
#endif

#if defined(CRAY) || (defined(__WIN32__) && !defined(__MINGW32_VERSION))
#undef LSODE
#undef LSODE_JEX
#undef LSODE_FEX
#undef GETCOMMON
#undef XASCWV
#define XASCWV XASCWV
#define LSODE LSODE
#define LSODE_JEX JEX
#define LSODE_FEX FEX
#define GETCOMMON GET_LSODE_COMMON
#endif

#define ASC_CLOCK_CHECK_PERIOD 1 /* number of FEX or JEX cycled between GUI updates */
#define ASC_CLOCK_MAX_GUI_WAIT (0.5*CLOCKS_PER_SEC) /* max number of clock ticks between GUI updates */
/* definitions of lsode supported children of atoms, etc */
/********************************************************************/
/* solver_var children expected for state variables */
static symchar *g_symbols[2];
#define STATERTOL g_symbols[0]
#define STATEATOL g_symbols[1]
static
void InitTolNames(void)
{
  STATERTOL = AddSymbol("ode_rtol");
  STATEATOL = AddSymbol("ode_atol");
}

/*--------------------------
  Data space for use by LSODE
*/

/**
	Because LSODE doesn't seem to make an allowance for 'client data' we
	have to store this as a 'local global' and fish it out when we're in the
	callbacks.
*/
static IntegratorSystem *l_lsode_blsys = 0;

typedef enum{
	lsode_none=0		/* true on first call */
	, lsode_function	/**< a function evaluation */
	, lsode_derivative	/**< a gradient evaluation */
} IntegratorLsodeLastCallType;
/**
	Enumeration that tells ASCEND what was the most recent thing LSODE did.
*/

typedef enum{
	lsode_ok=0			/**< success */
	, lsode_nok			/**< bad return from func or grad */
} IntegratorLsodeStatusCode;
/**
	Enumeration to tell ASCEND if anything failed in a FEX or JEX call.
*/

typedef struct IntegratorLsodeDataStruct{
	long n_eqns;                     /**< dimension of state vector */
	int *input_indices;              /**< vector of state vars indexes */
	int *output_indices;             /**< vector of derivative var indexes */
	struct var_variable **y_vars;    /**< NULL-terminated list of states vars */
	struct var_variable **ydot_vars; /**< NULL-terminated list of derivative vars*/
	struct rel_relation **rlist;     /**< NULL-terminated list of relevant rels
	                                    to be differentiated */
	DenseMatrix dydot_dy;               /**< change in derivatives wrt states */

	IntegratorLsodeLastCallType lastcall;  /* type of last call; func or grad */
	IntegratorLsodeStatusCode   status;    /* solve status */
	char stop;                             /* stop requested? */
	int partitioned;                       /* partioned func evals or not */

	clock_t lastwrite;                     /* time of last call to the reporter 'write' function */
} IntegratorLsodeData;

/**<
	Global data structure for LSODE.

	@NOTE LSODE is not reentrant! @ENDNOTE
*/

/** Macro to declare a local var and fetch the 'enginedata' stuff into it from l_lsode_blsys. */
#define LSODEDATA_GET(N) \
	IntegratorLsodeData *N; \
	asc_assert(l_lsode_blsys!=NULL); \
	N = (IntegratorLsodeData *)l_lsode_blsys->enginedata; \
	asc_assert(N!=NULL)

/** Macro to set the globa l_lsode_blsys to the currently blsys ptr. */
#define LSODEDATA_SET(N) \
	asc_assert(l_lsode_blsys==NULL); \
	asc_assert(N!=NULL); \
	l_lsode_blsys = N

#define LSODEDATA_RELEASE() \
	asc_assert(l_lsode_blsys!=NULL); \
	l_lsode_blsys = NULL;

/*----------------------------
  Function types that LSODE wants to use
*/

/**
	Type of function used to evaluate derivative system.
*/
typedef void LsodeEvalFn(int *, double *, double *, double *);

/**
	Type of function used to evaluate jacobian system.
*/
typedef void LsodeJacobianFn(int *, double *, double *, int *, int *, double *, int *);

/*----------------------------
  forward declarations
*/

int integrator_lsode_setup_diffs(IntegratorSystem *blsys);

/**
	void LSODE(&fex, &neq, y, &x, &xend, &itol, reltol, abtol, &itask,
		&istate, &iopt ,rwork, &lrw, iwork, &liw, &jex, &mf);

	This is a prototype for the *fortran* LSODE function.

	No 'extern' here, so we want linker to complain if no static linkage.
 */
void LSODE(LsodeEvalFn*,int *neq ,double *y ,double *x
	  ,double *xend
	  ,int *itol ,double *reltol ,double *abtol
	  ,int *itask ,int *istate ,int *iopt
	  ,double *rwork ,int *lrw
	  ,int *iwork ,int *liw
	  ,LsodeJacobianFn *jex ,int *mf
);

/*------------------------------------------------------
  Memory allocation/free
*/

void integrator_lsode_create(IntegratorSystem *blsys){
	IntegratorLsodeData *d;
	d = ASC_NEW_CLEAR(IntegratorLsodeData);
	d->n_eqns=0;
	d->input_indices=NULL;
	d->output_indices=NULL;
	d->y_vars=NULL;
	d->ydot_vars=NULL;
	d->rlist=NULL;
	d->dydot_dy=DENSEMATRIX_EMPTY;
	blsys->enginedata=(void*)d;
	integrator_lsode_params_default(blsys);

}

/**
	Cleanup the data struct that belongs to LSODE
*/
void integrator_lsode_free(void *enginedata){
	IntegratorLsodeData d;
	d = *((IntegratorLsodeData *)enginedata);

	if(d.input_indices)ASC_FREE(d.input_indices);
	d.input_indices = NULL;

	if(d.output_indices)ASC_FREE(d.output_indices);
	d.output_indices = NULL;

	if(d.y_vars)ASC_FREE(d.y_vars);
	d.y_vars = NULL;

	if(d.ydot_vars)ASC_FREE(d.ydot_vars);
	d.ydot_vars = NULL;

	if(d.rlist)ASC_FREE(d.rlist);
	d.rlist =  NULL;

	densematrix_destroy(d.dydot_dy);

	d.n_eqns = 0L;
}

/*------------------------------------------------------------------------------
	PARAMETERS
*/

enum ida_parameters{
	LSODE_PARAM_METH
	,LSODE_PARAM_MITER
	,LSODE_PARAM_MAXORD
	,LSODE_PARAM_TIMING
	,LSODE_PARAM_RTOLVECT
	,LSODE_PARAM_RTOL
	,LSODE_PARAM_ATOLVECT
	,LSODE_PARAM_ATOL
	,LSODE_PARAMS_SIZE
};

/**
	Here the full set of parameters is defined, along with upper/lower bounds,
	etc. The values are stuck into the blsys->params structure.

	@return 0 on success
*/
int integrator_lsode_params_default(IntegratorSystem *blsys){

	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_LSODE);
	slv_parameters_t *p;
	p = &(blsys->params);

	slv_destroy_parms(p);

	if(p->parms==NULL){
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, LSODE_PARAMS_SIZE);
		if(p->parms==NULL)return -1;
		p->dynamic_parms = 1;
	}else{
		asc_assert(p->num_parms == LSODE_PARAMS_SIZE);
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_char(p,LSODE_PARAM_METH
			,(SlvParameterInitChar){{"meth"
			,"Integration method",1
			,"AM=Adams-Moulton method (for non-stiff problems), BDF=Backwards"
			" Difference Formular (for stiff problems). See 'Description and"
			" Use of LSODE', section 3.1."
		}, "BDF"}, (char *[]){"AM","BDF",NULL}
	);

	slv_param_int(p,LSODE_PARAM_MITER
			,(SlvParameterInitInt){{"miter"
			,"Corrector iteration technique", 1
			,"0=Functional iteration, 1=Modified Newton iteration with user-"
			"supplied analytical Jacobian, 2=Modified Newton iteration with"
			" internally-generated numerical Jacobian, 3=Modified Jacobi-Newton"
			" iteration with internally generated numerical Jacobian. See "
			" 'Description and Use of LSODE', section 3.1. Note that not all"
			" methods described there are available via ASCEND."
		}, 1, 0, 3}
	);

	slv_param_int(p,LSODE_PARAM_MAXORD
			,(SlvParameterInitInt){{"maxord"
			,"Maximum method order", 1
			,"See 'Description and Use of LSODE', p 92 and p 8. Limits <=12 for BDF"
			" and <=5 for AM. Higher values are reduced automatically. See notes on"
			" page 92 regarding oscillatory systems."
		}, 12, 1, 12}
	);

	slv_param_bool(p,LSODE_PARAM_TIMING
			,(SlvParameterInitBool){{"timing"
			,"Output timing statistics?",1
			,"If TRUE, additional timing statistics will be output to the"
			" console during integration."
		}, TRUE}
	);

	slv_param_bool(p,LSODE_PARAM_ATOLVECT
			,(SlvParameterInitBool){{"atolvect"
			,"Use 'ode_atol' values as specified for each var?",1
			,"If TRUE, values of 'ode_atol' are taken from your model and used"
			" in the integration. If FALSE, a scalar absolute tolerance (atol)"
			" is shared by all variables."
		}, TRUE }
	);

	slv_param_real(p,LSODE_PARAM_ATOL
			,(SlvParameterInitReal){{"atol"
			,"Scalar absolute error tolerance",1
			,"Default value of the scalar absolute error tolerance (for cases"
			" where not specified in oda_atol var property. See 'lsode.f' for"
			" details"
		}, 1e-6, 1e-15, 1e10 }
	);

	slv_param_bool(p,LSODE_PARAM_RTOLVECT
			,(SlvParameterInitBool){{"rtolvect"
			,"Use 'ode_rtol' values as specified for each var?",1
			,"If TRUE, values of 'ode_atol' are taken from your model and used "
			" in the integration. If FALSE, a scalar absolute tolerance (rtol)"
			" is shared by all variables."
		}, TRUE }
	);

	slv_param_real(p,LSODE_PARAM_RTOL
			,(SlvParameterInitReal){{"rtol"
			,"Scalar relative error tolerance",1
			,"Default value of the scalar relative error tolerance (for cases"
			" where not specified in oda_rtol var property. See 'lsode.f' for"
			" details"
		}, 1e-6, 1e-15, 1 }
	);

	asc_assert(p->num_parms == LSODE_PARAMS_SIZE);
	return 0;
}

/*------------------------------------------------------------------------------
  PROBLEM ANALYSIS
*/

/**
	@TODO needs work. Assumes struct Instance* and struct var_variable*
	are synonymous, which demonstrates the need for a method to take
	an instance and ask the solvers for its global or local index
	if var and inst are decoupled.
*/
int integrator_lsode_setup_diffs(IntegratorSystem *blsys) {
	/* long n_eqns; */
	unsigned long nch,i;

	struct var_variable **vp;
	int *ip;

	IntegratorLsodeData *enginedata;
	asc_assert(blsys!=NULL);
	enginedata = (IntegratorLsodeData *)blsys->enginedata;

	assert(enginedata->n_eqns==blsys->n_y);

	/*
		Put the
		Let us now process what we consider *inputs* to the problem as
		far as ASCEND is concerned; i.e. the state vars or the y_vars's
		if you prefer.
	*/
	nch = enginedata->n_eqns;

	vp = enginedata->y_vars;
	ip = enginedata->input_indices;
	for (i=0;i<nch;i++) {
		*vp = (struct var_variable *)blsys->y[i];
		*ip = var_sindex(*vp);
		vp++;
		ip++;
	}
	*vp = NULL;	/* terminate */

	/*
		Let us now go for the outputs, ie the derivative terms.
	*/
	vp = enginedata->ydot_vars;
	ip = enginedata->output_indices;
	for (i=0;i<nch;i++) {
		*vp = (struct var_variable *)blsys->ydot[i];
		*ip = var_sindex(*vp);
		vp++;		/* dont assume that a var is synonymous with */
		ip++;		/* an Instance; that might/will change soon */
	}
	*vp = NULL;		/* terminate */

	return 0;
}

/**
	allocates, fills, and returns the atol vector based on LSODE

	State variables missing child ode_rtol will be defaulted to ATOL
*/
static double *lsode_get_atol( IntegratorSystem *blsys) {

  struct Instance *tol;
  double *atoli;
  int i,len;
  double atol;

  len = blsys->n_y;
  atoli = ASC_NEW_ARRAY(double, blsys->n_y); /* changed, this was n_y+1 before, dunnowi -- JP */
  if (atoli == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
    return atoli;
  }

  if(!SLV_PARAM_BOOL(&(blsys->params),LSODE_PARAM_ATOLVECT)){
	atol = SLV_PARAM_REAL(&(blsys->params),LSODE_PARAM_ATOL);
	CONSOLE_DEBUG("Using ATOL = %f for all vars", atol);
    for(i=0; i<len; ++i){
      atoli[i] = atol;
	}
  }else{
    InitTolNames();
    for (i=0; i<len; i++) {

      tol = ChildByChar(var_instance(blsys->y[i]),STATEATOL);
      if (tol == NULL || !AtomAssigned(tol) ) {
        atoli[i] = SLV_PARAM_REAL(&(blsys->params),LSODE_PARAM_ATOL);
        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Assuming atol = %3g"
      	  "for ode_atol child undefined for state variable %ld."
        	,atoli[i], blsys->y_id[i]
        );
      } else {
        atoli[i] = RealAtomValue(tol);
        CONSOLE_DEBUG("Using atol %3g for state variable %ld.",atoli[i], blsys->y_id[i]);
      }
    }
  }
  return atoli;
}

/**
	Allocates, fills, and returns the rtol vector based on LSODE

	State variables missing child ode_rtol will be defaulted to RTOL
*/
static double *lsode_get_rtol( IntegratorSystem *blsys) {

  struct Instance *tol;
  double rtol, *rtoli;
  int i,len;

  len = blsys->n_y;
  rtoli = ASC_NEW_ARRAY(double, blsys->n_y+1);
  if (rtoli == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
    return rtoli;
  }
  if(!SLV_PARAM_BOOL(&(blsys->params),LSODE_PARAM_RTOLVECT)){
	rtol = SLV_PARAM_REAL(&(blsys->params),LSODE_PARAM_RTOL);
	CONSOLE_DEBUG("Using RTOL = %f for all vars", rtol);
    for(i=0; i<len; ++i){
      rtoli[i] = rtol;
	}
  }else{
    InitTolNames();
    for (i=0; i<len; i++) {
      tol = ChildByChar(var_instance(blsys->y[i]),STATERTOL);
      if (tol == NULL || !AtomAssigned(tol) ) {
        rtoli[i] = SLV_PARAM_REAL(&(blsys->params),LSODE_PARAM_RTOL);

        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Assuming rtol = %3g"
        	"for ode_rtol child undefined for state variable %ld."
        	,rtoli[i], blsys->y_id[i]
        );

      } else {
        rtoli[i] = RealAtomValue(tol);
      }
    }
  }
  rtoli[len] = SLV_PARAM_REAL(&(blsys->params),LSODE_PARAM_RTOL);
  return rtoli;
}

/*
	Write out a a status message based on the istate parameter.
*/
static void lsode_write_istate( int istate) {
	switch (istate) {
		case -1:
			FPRINTF(ASCERR,"Excess steps taken on this call"
				" (perhaps wrong MF)."); break;
		case -2:
			FPRINTF(ASCERR,"Excess accuracy requested"
				" (tolerances too small)."); break;
		case -3:
			FPRINTF(ASCERR,"Illegal input detected"
				" (see console)."); break;
		case -4:
			FPRINTF(ASCERR,"Repeated error test failures"
				" (check all inputs)."); break;
		case -5:
			FPRINTF(ASCERR,"Repeated convergence failures"
				" (perhaps bad Jacobian supplied, or wrong choice of MF or tolerances)."); break;
		case -6:
			FPRINTF(ASCERR,"Error weight became zero during problem"
				" (solution component i vanished, and atol or atol(i) = 0)."); break;
		case -7:
			FPRINTF(ASCERR,"Interrupted? User cancelled operation?"); break;
		case -8:
			FPRINTF(ASCERR,"Error in nonlinear solver"); break;
		default:
			FPRINTF(ASCERR,"Unknown 'istate' error code %d from LSODE.",istate);
			break;
	}
}

/**
	Free memory allocated for the LSODE, but first check.
*/
static void lsode_free_mem(double *y, double *reltol, double *abtol, double *rwork,
                    int *iwork, double *obs, double *dydx)
{
  if (y != NULL) {
    ascfree((double *)y);
  }
  if (reltol != NULL) {
    ascfree((double *)reltol);
  }
  if (abtol != NULL) {
    ascfree((double *)abtol);
  }
  if (rwork != NULL) {
    ascfree((double *)rwork);
  }
  if (iwork != NULL) {
    ascfree((int *)iwork);
  }
  if (obs != NULL) {
    ascfree((double *)obs);
  }
  if (dydx != NULL) {
    ascfree((double *)dydx);
  }
}

/**
	"Temporary" derivative evaluation routine (pre 1995!).

	Ben says: "The proper permanent fix for lsode is to dump it in favor of
	cvode or dassl." (so: see ida.c)

	@return 0 on success

	@NOTE It is assumed the system has been solved at the current point. @ENDNOTE
*/
int integrator_lsode_derivatives(IntegratorSystem *blsys
		, int ninputs
		, int noutputs
){
  static int n_calls = 0;
  linsolqr_system_t linsys;	/* stuff for the linear system & matrix */
  mtx_matrix_t mtx;
  int32 capacity;
  real64 *scratch_vector = NULL;
  int result=0;
  IntegratorLsodeData *enginedata;

  asc_assert(blsys!=NULL);
  enginedata = (IntegratorLsodeData *)blsys->enginedata;
  asc_assert(enginedata!=NULL);
  asc_assert(DENSEMATRIX_DATA(enginedata->dydot_dy)!=NULL);
  asc_assert(enginedata->input_indices!=NULL);

  int *inputs_ndx_list = enginedata->input_indices;
  int *outputs_ndx_list = enginedata->output_indices;
  asc_assert(ninputs == blsys->n_y);

  (void)NumberFreeVars(NULL);		/* used to re-init the system */
  (void)NumberIncludedRels(NULL);	/* used to re-init the system */
  if (!blsys->system) {
    FPRINTF(stderr,"The solve system does not exist !\n");
    return 1;
  }

  result = Compute_J(blsys->system);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in calc Jacobian\n");
    return 1;
  }

  linsys = slv_get_linsolqr_sys(blsys->system);	/* get the linear system */
  if (linsys==NULL) {
    FPRINTF(stderr,"Early termination due to missing linsolqr system.\n");
    return 1;
  }
  mtx = slv_get_sys_mtx(blsys->system);	/* get the matrix */
  if (mtx==NULL) {
    FPRINTF(stderr,"Early termination due to missing mtx in linsolqr.\n");
    return 1;
  }
  capacity = mtx_capacity(mtx);
  scratch_vector = ASC_NEW_ARRAY_CLEAR(real64,capacity);
  linsolqr_add_rhs(linsys,scratch_vector,FALSE);

  result = LUFactorJacobian(blsys->system);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in LUFactorJacobian\n");
    goto error;
  }
  result = Compute_dy_dx_smart(blsys->system, scratch_vector, enginedata->dydot_dy,
                               inputs_ndx_list, ninputs,
                               outputs_ndx_list, noutputs);

  linsolqr_remove_rhs(linsys,scratch_vector);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in Compute_dy_dx\n");
    goto error;
  }

error:
  n_calls++;
  if(scratch_vector){
    ascfree((char *)scratch_vector);
  }
  return result;
}

/**
	The current way that we are getting the derivatives (if the problem
	was solved partitioned) messes up the slv_system so that we *have*
	to do a *presolve* rather than a simply a *resolve* before doing
	function calls.  This code below attempts to handle these cases.
*/
static void LSODE_FEX( int *n_eq ,double *t ,double *y ,double *ydot){
	static short clockcheck = 0;
  slv_status_t status;
  LSODEDATA_GET(lsodedata);

  /*  slv_parameters_t parameters; pity lsode doesn't allow error returns */
  /* int i; */
  unsigned long res;

#ifdef TIMING_DEBUG
  clock_t time1,time2;
#endif

  /* CONSOLE_DEBUG("Calling for a function evaluation"); */

#ifdef TIMING_DEBUG
  CONSOLE_DEBUG("Calling for a function evaluation");
  time1 = clock();
#endif

  /*
 	t[1]=t[0]; can't do this. lsode calls us with a different t than the t we sent in.
  */
  integrator_set_t(l_lsode_blsys, t[0]);
  integrator_set_y(l_lsode_blsys, y);

#ifdef TIMING_DEBUG
  time2 = clock();
#endif

  switch(lsodedata->lastcall) {
  case lsode_none:		/* first call */
	CONSOLE_DEBUG("FIRST CALL...");

  case lsode_derivative:
    if (lsodedata->partitioned) {
			/* CONSOLE_DEBUG("PRE-SOLVE"); */
      slv_presolve(l_lsode_blsys->system);
    } else {
			/** @TODO this doesn't ever seem to be called */
			CONSOLE_DEBUG("RE-SOLVE");
      slv_resolve(l_lsode_blsys->system);
    }
    break;
  default:
  case lsode_function:
    slv_resolve(l_lsode_blsys->system);
    break;
  }

  if((res =slv_solve(l_lsode_blsys->system))){
		CONSOLE_DEBUG("solver returns error %ld",res);
	}

  slv_get_status(l_lsode_blsys->system, &status);
  if(slv_check_bounds(l_lsode_blsys->system,0,-1,"")){
    lsodedata->status = lsode_nok;
  }

  /* pass the solver status to the integrator */
  res = integrator_checkstatus(status);

	/* Do we need to do clock check? */
	if((++clockcheck % ASC_CLOCK_CHECK_PERIOD)==0){
		if((clock() - lsodedata->lastwrite) > ASC_CLOCK_MAX_GUI_WAIT){
			integrator_output_write(l_lsode_blsys);
			lsodedata->lastwrite = clock(); /* don't count the update time, or we might never get anything done */
		}
	}

#ifdef TIMING_DEBUG
  time2 = clock() - time2;
#endif

  if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve for derivatives (%d)",res);
#if 0
  	ERROR_REPORTER_START_HERE(ASC_PROG_ERR);
    FPRINTF(ASCERR,"Unable to compute the vector of derivatives with the following values for the state variables:\n");
    for (i = 0; i< *n_eq; i++) {
      FPRINTF(ASCERR,"y[%4d] = %f\n",i, y[i]);
    }
    error_reporter_end_flush();
#endif
		lsodedata->stop = 1;
    lsodedata->status = lsode_nok;
#ifdef ASC_SIGNAL_TRAPS
		raise(SIGINT);
#endif
  }else{
    lsodedata->status = lsode_ok;
		/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"lsodedata->status = %d",lsodedata->status); */
  }
  integrator_get_ydot(l_lsode_blsys, ydot);

  lsodedata->lastcall = lsode_function;
#ifdef TIMING_DEBUG
  time1 = clock() - time1;
  CONSOLE_DEBUG("Function evalulation has been completed in %ld ticks. True function call time = %ld ticks",time1,time2);
#endif
}

/**
	Evaluate the jacobian
*/
static void LSODE_JEX(int *neq ,double *t, double *y
		, int *ml ,int *mu ,double *pd, int *nrpd
){
	static short clockcheck = 0;
  int nok = 0;
  int i,j;

  LSODEDATA_GET(lsodedata);

  UNUSED_PARAMETER(t);
  UNUSED_PARAMETER(y);
  UNUSED_PARAMETER(ml);
  UNUSED_PARAMETER(mu);

  /* CONSOLE_DEBUG("Calling for a gradient evaluation"); */
#ifdef TIMING_DEBUG
  clock_t time1;

  CONSOLE_DEBUG("Calling for a gradient evaluation");
  time1 = clock();
#endif
  /*
   * Make the real call.
   */

  nok = integrator_lsode_derivatives(l_lsode_blsys
		, *neq
		, *nrpd
  );

  if(nok){
  	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in computing the derivatives for the system. Failing...");
    lsodedata->status = lsode_nok;
    lsodedata->lastcall = lsode_derivative;
		lsodedata->stop = 1;
    return;
  }else{
    lsodedata->status = lsode_ok;
    lsodedata->lastcall = lsode_derivative;
  }

	/* Do we need to do clock check? */
	if((++clockcheck % ASC_CLOCK_CHECK_PERIOD)==0){
		/* do we need to update the GUI? */
#ifdef TIMING_DEBUG
		CONSOLE_DEBUG("CLOCK = %ld", clock());
#endif
		if((clock() - lsodedata->lastwrite) > ASC_CLOCK_MAX_GUI_WAIT){
			integrator_output_write(l_lsode_blsys);
			lsodedata->lastwrite = clock(); /* don't count the update time, or we might never get anything done */
		}
	}

  /*
	Map data from C based matrix to Fortan matrix.
	We will send in a column major ordering vector for pd.
  */
  asc_assert(*neq == DENSEMATRIX_NCOLS(lsodedata->dydot_dy));
  asc_assert(*nrpd == DENSEMATRIX_NROWS(lsodedata->dydot_dy));
  for (j=0;j<*neq;j++) { /* loop through columnns */
    for (i=0;i<*nrpd;i++){ /* loop through rows */
			/* CONSOLE_DEBUG("JAC[r=%d,c=%d]=%f",i,j,lsodedata.dydot_dy[i][j]); */
      *pd++ = DENSEMATRIX_ELEM(lsodedata->dydot_dy,i,j);
    }
  }

#ifdef TIMING_DEBUG
  time1 = clock() - time1;
  CONSOLE_DEBUG("Time to do gradient evaluation %ld ticks",time1);
#endif

  return;
}

/**
	The public function: here we do the actual integration, I guess.

	Return 0 on success
*/
int integrator_lsode_solve(IntegratorSystem *blsys
		, unsigned long start_index, unsigned long finish_index
){
	slv_status_t status;
	slv_parameters_t params;
	IntegratorLsodeData *d;

	double x[2];
	double xend,xprev;
	unsigned long nsamples, neq;
	long nobs;
	int  itol, itask, mf, lrw, liw;
	unsigned long index;
	int istate, iopt;
	double * rwork;
	int * iwork;
	double *y, *abtol, *reltol, *obs, *dydx;
	int my_neq;
	int reporterstatus;
	const char *method; /* Table 3.1 in D&UoLSODE */
	int miter; /* Table 3.2 in D&UoLSODE */
	int maxord; /* page 92 in D&UoLSODE */

	d = (IntegratorLsodeData *)(blsys->enginedata);

	/* the numer of equations must be equal to blsys->n_y, the number of states */
	d->n_eqns = blsys->n_y;
	assert(d->n_eqns>0);

	d->input_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);
	d->output_indices = ASC_NEW_ARRAY_CLEAR(int, d->n_eqns);
	d->dydot_dy = densematrix_create(d->n_eqns,d->n_eqns);

	d->y_vars = ASC_NEW_ARRAY(struct var_variable *,d->n_eqns+1);
	d->ydot_vars = ASC_NEW_ARRAY(struct var_variable *, d->n_eqns+1);

	integrator_lsode_setup_diffs(blsys);

	/* LSODE should be OK to deal with any linsol/linsolqr-based solver. But for
	the moment we restrict to just QRSlv. */
	if(strcmp(slv_solver_name(slv_get_selected_solver(blsys->system)),"QRSlv") != 0) {
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"QRSlv must be selected before integration.");
		return 1;
	}

	CONSOLE_DEBUG("Solver selected is '%s'",slv_solver_name(slv_get_selected_solver(blsys->system)));

	slv_get_status(blsys->system, &status);

  if (status.struct_singular) {
  	ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration will not be performed. The system is structurally singular.");
    d->status = lsode_nok;
    return 2;
  }

#if defined(STATIC_LSOD) || defined (DYNAMIC_LSOD)

  /* here we assume integrators.c is in charge of dynamic loading */

  slv_get_parameters(blsys->system,&params);
  d->partitioned = 1;
  d->stop = 0; /* clear 'stop' flag */

	/* read METH and MITER parameters, create MF value */
	method = SLV_PARAM_CHAR(&(blsys->params),LSODE_PARAM_METH);
	miter = SLV_PARAM_INT(&(blsys->params),LSODE_PARAM_MITER);
	maxord = SLV_PARAM_INT(&(blsys->params),LSODE_PARAM_MAXORD);
	if(miter < 0 || miter > 3){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unacceptable value '%d' of parameter 'miter'",miter);
		return 5;
	}
	if(strcmp(method,"BDF")==0){
		CONSOLE_DEBUG("method = BDF");
		mf = 20 + miter;
		if(maxord > 5){
			maxord = 5;
			CONSOLE_DEBUG("MAXORD reduced to 5 for BDF");
		}
	}else if(strcmp(method,"AM")==0){
		CONSOLE_DEBUG("method = AM");
		if(maxord > 12){
			maxord = 12;
			CONSOLE_DEBUG("MAXORD reduced to 12 for AM");
		}
		mf = 10 + miter;
  }else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unacceptable value '%d' of parameter 'meth'",method);
		return 5;
	}

	CONSOLE_DEBUG("MF = %d",mf);

  nsamples = integrator_getnsamples(blsys);
  if (nsamples <2) {
  	ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration will not be performed. The system has no end sample time defined.");
    d->status = lsode_nok;
    return 3;
  }
  neq = blsys->n_y;
  nobs = blsys->n_obs;

  /* samplelist_debug(blsys->samples); */

  /* x[0] = integrator_get_t(blsys); */
  x[0] = integrator_getsample(blsys, 0);
  x[1] = x[0]-1; /* make sure we don't start with wierd x[1] */

	/* RWORK memory requirements: see D&UoLSODE p 82 */
	switch(mf){
		case 10: case 20:
			lrw = 20 + neq * (maxord + 1) + 3 * neq;
			break;
		case 11: case 12: case 21: case 22:
			lrw = 22 + neq * (maxord + 1) + 3 * neq + neq*neq;
			break;
		case 13: case 23:
			lrw = 22 + neq * (maxord + 1) + 4 * neq;
			break;
		default:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unknown size requirements for this value of 'mf'");
			return 4;
	}

  rwork = ASC_NEW_ARRAY_CLEAR(double, lrw+1);
  liw = 20 + neq;
  iwork = ASC_NEW_ARRAY_CLEAR(int, liw+1);
  y = integrator_get_y(blsys, NULL);
  reltol = lsode_get_rtol(blsys);
  abtol = lsode_get_atol(blsys);
  obs = integrator_get_observations(blsys, NULL);
  dydx = ASC_NEW_ARRAY_CLEAR(double, neq+1);
  if(!y || !obs || !abtol || !reltol || !rwork || !iwork || !dydx) {
    lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory for lsode.");
    d->status = lsode_nok;
    return 4;
  }

  /*
  	Prepare args and call lsode.
  */
  itol = 4;
  itask = 1;
  istate = 1;
  iopt = 1;
  rwork[4] = integrator_get_stepzero(blsys);
  rwork[5] = integrator_get_maxstep(blsys);
  rwork[6] = integrator_get_minstep(blsys);
  iwork[5] = integrator_get_maxsubsteps(blsys);
	iwork[4] = maxord;
	CONSOLE_DEBUG("MAXORD = %d",maxord);

  if(x[0] > integrator_getsample(blsys, 2)){
    ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid initialisation time: exceeds second timestep value");
  	return 5;
  }

  /* put the values from derivative system into the record */
  integrator_setsample(blsys, start_index, x[0]);

  integrator_output_init(blsys);

  /* -- store the initial values of all the stuff */
  integrator_output_write(blsys);
  integrator_output_write_obs(blsys);

  my_neq = (int)neq;

  /*
	First time entering lsode, x is input. After that,
	lsode uses x as output (y output is y(x)). To drive
	the loop ahead in time, all we need to do is keep upping
	xend.
  */

  blsys->currentstep = 0;
  for(index = start_index; index < finish_index; index++, 	blsys->currentstep++) {
    xend = integrator_getsample(blsys, index+1);
    xprev = x[0];
		asc_assert(xend > xprev);
    /* CONSOLE_DEBUG("LSODE call #%lu: x = [%f,%f]", index,xprev,xend); */

# ifdef ASC_SIGNAL_TRAPS

		Asc_SignalHandlerPushDefault(SIGFPE);
		Asc_SignalHandlerPushDefault(SIGINT);

    if(SETJMP(g_fpe_env)==0) {
# endif /* ASC_SIGNAL_TRAPS */

	  /* CONSOLE_DEBUG("Calling LSODE with end-time = %f",xend); */
      /*
	  switch(mf){
		case 10:
			CONSOLE_DEBUG("Non-stiff (Adams) method; no Jacobian will be used"); break;
		case 21:
			CONSOLE_DEBUG("Stiff (BDF) method, user-supplied full Jacobian"); break;
		case 22:
			CONSOLE_DEBUG("Stiff (BDF) method, internally generated full Jacobian"); break;
		case 24:
			CONSOLE_DEBUG("Stiff (BDF) method, user-supplied banded jacobian"); break;
		case 25:
			CONSOLE_DEBUG("Stiff (BDF) method, internally generated banded jacobian"); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid method id %d for LSODE",mf);
			return 0; * failure *
      }
	  */

			d->lastwrite = clock();

      /* provides some rudimentary locking to prevent reentrance*/
      LSODEDATA_SET(blsys);

      LSODE(&(LSODE_FEX), &my_neq, y, x, &xend,
            &itol, reltol, abtol, &itask, &istate,
            &iopt ,rwork, &lrw, iwork, &liw, &(LSODE_JEX), &mf);

      /* clear the global var */
      LSODEDATA_RELEASE();

# ifdef ASC_SIGNAL_TRAPS
    }else{
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to float error in LSODE call.");
      lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
      d->status = lsode_ok;		/* clean up before we go */
      d->lastcall = lsode_none;
      return 6;
    }
# endif

    /* CONSOLE_DEBUG("AFTER %lu LSODE CALL\n", index); */
    /* this check is better done in fex,jex, but lsode takes no status */
/*    if (Solv_C_CheckHalt()) {
      if (istate >= 0) {
        istate=-7;
      }
    }
*/
	if(d->stop){
		istate=-8;
	}

    if (istate < 0 ) {
      /* some kind of error occurred... */
      ERROR_REPORTER_START_HERE(ASC_PROG_ERR);
      lsode_write_istate(istate);
      FPRINTF(ASCERR, "\nFurthest point reached was t = %g.\n",x[0]);
      error_reporter_end_flush();

      lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
      integrator_output_close(blsys);
      return 7;
    }

    if (d->status==lsode_nok) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to an error in derivative computations.");
      lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
      d->status = lsode_ok;		/* clean up before we go */
      d->lastcall = lsode_none;
      integrator_output_close(blsys);
      return 8;
    }

    integrator_setsample(blsys, index+1, x[0]);
    /* record when lsode actually came back */
    integrator_set_t(blsys, x[0]);
    integrator_set_y(blsys, y);
    /* put x,y in d in case lsode got x,y by interpolation, as it does  */

	reporterstatus = integrator_output_write(blsys);

	if(reporterstatus==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration cancelled");
		lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
		d->status = lsode_ok;
		d->lastcall = lsode_none;
		integrator_output_close(blsys);
		return 9;
	}

	if (nobs > 0) {
# ifdef ASC_SIGNAL_TRAPS
      if (SETJMP(g_fpe_env)==0) {
# endif /* ASC_SIGNAL_TRAPS */

        /* solve for obs since d isn't necessarily already
           computed there though lsode's x and y may be.
           Note that since lsode usually steps beyond xend
           x1 usually wouldn't be x0 precisely if the x1/x0
           scheme worked, which it doesn't anyway. */

        LSODEDATA_SET(blsys);
        LSODE_FEX(&my_neq, x, y, dydx);
        LSODEDATA_RELEASE();

        /* calculate observations, if any, at returned x and y. */
        obs = integrator_get_observations(blsys, obs);

        integrator_output_write_obs(blsys);

# ifdef ASC_SIGNAL_TRAPS
      } else {
      	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration terminated due to float error in LSODE FEX call.");
        lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);
        d->status = lsode_ok;               /* clean up before we go */
        d->lastcall = lsode_none;
        integrator_output_close(blsys);
        return 10;
      }
# endif /* ASC_SIGNAL_TRAPS */
    }
    /* CONSOLE_DEBUG("Integration completed from %3g to %3g.",xprev,x[0]); */
  }

  CONSOLE_DEBUG("...");
  CONSOLE_DEBUG("Number of steps taken: %1d.", iwork[10]);
  CONSOLE_DEBUG("Number of function evaluations: %1d.", iwork[11]);
  CONSOLE_DEBUG("Number of Jacobian evaluations: %1d.", iwork[12]);
  CONSOLE_DEBUG("...");


  lsode_free_mem(y,reltol,abtol,rwork,iwork,obs,dydx);

  /*
   * return the system to its original state.
   */

  d->status = lsode_ok;
  d->lastcall = lsode_none;

  integrator_output_close(blsys);

  CONSOLE_DEBUG("--- LSODE done ---");
  return 0; /* success */

#else /* STATIC_LSOD || DYNAMIC_LSOD */

  ERROR_REPORTER_HERE(ASC_PROG_ERR,"Integration will not be performed. LSODE binary not available.");
  d->status = lsode_nok;
  return 11;

#endif
}

/**
	Function XASCWV is an error reporting function replacing the XERRWV
	routine in lsode.f. The call signature is the same with the original Fortran
	function.

	@see the comments for 'xerrwv' from lsode.f, with which XASCWV is compatible...

	@param msg    = the message (hollerith literal or integer array).
	@param nmes   = the length of msg (number of characters).
	@param nerr   = the error number (not used).
	@param level  = the error level..
	        0 or 1 means recoverable (control returns to caller).
	        2 means fatal (run is aborted--see note below).
	@param ni     = number of integers (0, 1, or 2) to be printed with message.
	@param i1,i2  = integers to be printed, depending on ni.
	@param nr     = number of reals (0, 1, or 2) to be printed with message.
	@param r1,r2  = reals to be printed, depending on nr.
*/
void XASCWV( char *msg, /* pointer to start of message */
             int *nmes, /* the length of msg (number of characters) */
             int *nerr, /* the error number (not used). */
             int *level,
             int *ni,
             int *i1,
             int *i2,
             int *nr,
             double *r1,
             double *r2
){
	static double r1last;

	asc_assert(*level!=2); // LSODE doesn't give level 2 in our version.

	switch(*nerr){
		case 17:
			if(*ni==2){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"rwork length needed, lenrw = %d > %d = lrw",*i1, *i2);
				return;
			} break;
		case 52:
			if(*nr==2){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Illegal t = %f, not in range (t - hu,t) = (%f,%f)", r1last, *r1, *r2);
				return;
			}else if(*nr==1){
				r1last = *r1;
				return;
			} break;
		case 201:
			if(*nr==0 && *ni==0)return;
			if(*nr==1 && *ni==1){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"At current t=%f, mxstep=%d steps taken on this call before reaching tout.",*r1,*i1);
				return;
			} break;
		case 204:
			if(*nr==0 && *ni==0)return;
			if(*nr==2){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error test failed repeatedly or with abs(h)=hmin.\nt=%f and step size h=%f",*r1,*r2);
				return;
			} break;
		case 205:
			if(*nr==0 && *ni==0)return;
			if(*nr==2){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Corrector convergence test failed repeatedly or with abs(h)=hmin.\nt=%f and step size h=%f",*r1,*r2);
				return;
			} break;
		case 27:
			if(*nr==1 && *ni==1){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Trouble with INTDY: itask = %d, tout = %f", *i1, *r1);
				return;
			} break;
	}

	ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);

	/* note that %.*s means that a string length (integer) and string pointer are being required */
	FPRINTF(stderr,"LSODE error: (%d) %.*s",*nerr,*nmes,msg);
	if(*ni == 1) {
		FPRINTF(stderr,"\nwhere i1 = %d",*i1);
	}else	if(*ni == 2) {
		FPRINTF(stderr,"\nwhere i1 = %d, i2 = %d",*i1,*i2);
	}
	if(*nr == 1) {
		FPRINTF(stderr,"\nwhere r1 = %.13g", *r1);
	}else if(*nr == 2) {
		FPRINTF(stderr,"\nwhere r1 = %.13g, r2 = %.13g", *r1,*r2);
	}
	error_reporter_end_flush();
}

int integrator_lsode_write_matrix(const IntegratorSystem *blsys, FILE *fp){
	IntegratorLsodeData *enginedata;
	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_LSODE);
	asc_assert(blsys->enginedata);
	enginedata = (IntegratorLsodeData *)blsys->enginedata;

	if(!DENSEMATRIX_DATA(enginedata->dydot_dy)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"dydot_dy contains no data");
	}

#ifdef ASC_WITH_MMIO
	densematrix_write_mmio(enginedata->dydot_dy,fp);
	CONSOLE_DEBUG("Returning after matrix output");
	return 0;
#else
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"MMIO routines not available");
	return 1;
#endif
}





