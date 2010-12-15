/*	ASCEND modelling environment
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
*//**
	@file
	Access to the IDA integrator for ASCEND. IDA is a DAE solver that comes
	as part of the GPL-licensed SUNDIALS solver package from LLNL.

	IDA provides the non-linear parts, as well as a number of pluggable linear
	solvers: dense, banded and krylov types.

	We also implement here an EXPERIMENTAL direct sparse linear solver for IDA
	using the ASCEND linsolqr routines.

	@see http://www.llnl.gov/casc/sundials/
*//*
	by John Pye, May 2006
*/

#define _GNU_SOURCE

#include <signal.h>
#include <setjmp.h>
#include <fenv.h>
#include <math.h>

/* SUNDIALS includes */
#ifdef ASC_WITH_IDA

#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==2
# include <sundials/sundials_config.h>
# include <sundials/sundials_nvector.h>
# include <ida/ida_spgmr.h>
# include <ida.h>
# include <nvector_serial.h>
#else
# include <sundials/sundials_config.h>
# include <nvector/nvector_serial.h>
# include <ida/ida.h>
#endif

# include <sundials/sundials_dense.h>
# include <ida/ida_spgmr.h>
# include <ida/ida_spbcgs.h>
# include <ida/ida_sptfqmr.h>
# include <ida/ida_dense.h>

# ifndef IDA_SUCCESS
#  error "Failed to include SUNDIALS IDA header file"
# endif
#else
# error "If you're building this file, you should have ASC_WITH_IDA"
#endif

#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/panic.h>
#include <ascend/compiler/instance_enum.h>

#include <ascend/system/slv_client.h>
#include <ascend/system/relman.h>
#include <ascend/system/block.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/jacobian.h>
#include <ascend/system/bndman.h>

#include <ascend/utilities/config.h>
#include <ascend/integrator/integrator.h>

#include "idalinear.h"
#include "idaanalyse.h"
#include "ida_impl.h"

/*
	for cases where we don't have SUNDIALS_VERSION_MINOR defined, guess version 2.2
*/
#ifndef SUNDIALS_VERSION_MINOR
# ifdef __GNUC__
#  warning "GUESSING SUNDIALS VERSION 2.2"
# endif
# define SUNDIALS_VERSION_MINOR 2
#endif
#ifndef SUNDIALS_VERSION_MAJOR
# define SUNDIALS_VERSION_MAJOR 2
#endif

/* SUNDIALS 2.4.0 introduces new DlsMat in place of DenseMat */
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==4
# define IDA_MTX_T DlsMat
# define IDADENSE_SUCCESS IDADLS_SUCCESS
# define IDADENSE_MEM_NULL IDADLS_MEM_NULL
# define IDADENSE_ILL_INPUT IDADLS_ILL_INPUT
# define IDADENSE_MEM_FAIL IDADLS_MEM_FAIL
#else
# define IDA_MTX_T DenseMat
#endif

/* #define FEX_DEBUG */
#define JEX_DEBUG
/* #define DJEX_DEBUG */
#define SOLVE_DEBUG
#define STATS_DEBUG
#define PREC_DEBUG
/* #define ROOT_DEBUG */

/* #define DIFFINDEX_DEBUG */
/* #define ANALYSE_DEBUG */
/* #define DESTROY_DEBUG */
/* #define MATRIX_DEBUG */

static IntegratorCreateFn integrator_ida_create;
static IntegratorParamsDefaultFn integrator_ida_params_default;
static IntegratorSolveFn integrator_ida_solve;
static IntegratorFreeFn integrator_ida_free;
IntegratorDebugFn integrator_ida_debug;
static IntegratorWriteMatrixFn integrator_ida_write_matrix;

/**
	Everthing that the outside world needs to know about IDA
*/
static const IntegratorInternals integrator_ida_internals = {
	integrator_ida_create
	,integrator_ida_params_default
	,integrator_ida_analyse
	,integrator_ida_solve
	,integrator_ida_write_matrix
	,integrator_ida_debug
	,integrator_ida_free
	,INTEG_IDA
	,"IDA"
};

extern ASC_EXPORT int ida_register(void){
	CONSOLE_DEBUG("Registering IDA...");
	return integrator_register(&integrator_ida_internals);
}

/*-------------------------------------------------------------
  FORWARD DECLS
*/

/* forward dec needed for IntegratorIdaPrecFreeFn */
struct IntegratorIdaDataStruct;

/* functions for allocating storage for and freeing preconditioner data */
typedef void IntegratorIdaPrecCreateFn(IntegratorSystem *sys);
typedef void IntegratorIdaPrecFreeFn(struct IntegratorIdaDataStruct *enginedata);


/**
	Struct containing any stuff that IDA needs that doesn't fit into the
	common IntegratorSystem struct.
*/
typedef struct IntegratorIdaDataStruct{

	struct rel_relation **rellist;   /**< NULL terminated list of ACTIVE rels */
	int nrels; /* number of ACTIVE rels */

	struct bnd_boundary **bndlist;	 /**< NULL-terminated list of boundaries, for use in the root-finding  code */
	int nbnds; /* number of boundaries */

	int safeeval;                    /**< whether to pass the 'safe' flag to relman_eval */
	var_filter_t vfilter;
	rel_filter_t rfilter;            /**< Used to filter relations from solver's rellist (@TODO needs work) */
	void *precdata;                  /**< For use by the preconditioner */
	IntegratorIdaPrecFreeFn *pfree;	 /**< Store instructions here on how to free precdata */

} IntegratorIdaData;


typedef struct IntegratorIdaPrecDJStruct{
	N_Vector PIii; /**< diagonal elements of the inversed Jacobi preconditioner */
} IntegratorIdaPrecDataJacobi;

typedef struct IntegratorIdaPrecDJFStruct{
	linsolqr_system_t L;
} IntegratorIdaPrecDataJacobian;

/**
	Hold all the function pointers associated with a particular preconditioner
	We don't need to store the 'pfree' function here as it is allocated to the enginedata struct
	by the pcreate function (ensures that corresponding 'free' and 'create' are always used)

	@note IDA uses a different convention for function pointer types, so no '*'.
*/
typedef struct IntegratorIdaPrecStruct{
	IntegratorIdaPrecCreateFn *pcreate;
	IDASpilsPrecSetupFn psetup;
	IDASpilsPrecSolveFn psolve;
} IntegratorIdaPrec;

/* residual function forward declaration */
static int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data);

static int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
);

/* error handler forward declaration */
static void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
);

/* dense jacobian evaluation for IDADense dense direct linear solver */
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
static int integrator_ida_djex(int Neq, realtype tt, realtype c_j
		, N_Vector yy, N_Vector yp, N_Vector rr
		, IDA_MTX_T Jac, void *jac_data
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
);
#else
static int integrator_ida_djex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, IDA_MTX_T Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
);
#endif

/* sparse jacobian evaluation for ASCEND's sparse direct solver */
static IntegratorSparseJacFn integrator_ida_sjex;

/* boundary-detection function */
static int integrator_ida_rootfn(realtype tt, N_Vector yy, N_Vector yp, realtype *gout, void *g_data);

typedef struct IntegratorIdaStatsStruct{
	long nsteps;
	long nrevals;
	long nlinsetups;
	long netfails;
	int qlast, qcur;
	realtype hinused, hlast, hcur;
	realtype tcur;
} IntegratorIdaStats;

typedef void (IntegratorVarVisitorFn)(IntegratorSystem *sys, struct var_variable *var, const int *varindx);

/*static IntegratorVarVisitorFn integrator_dae_classify_var;
static void integrator_visit_system_vars(IntegratorSystem *sys,IntegratorVarVisitorFn *visitor);
static void integrator_dae_show_var(IntegratorSystem *sys, struct var_variable *var, const int *varindx); */

static int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s);
static void integrator_ida_write_stats(IntegratorIdaStats *stats);
static void integrator_ida_write_incidence(IntegratorSystem *sys);

/*------
  Full jacobian preconditioner -- experimental
*/

static int integrator_ida_psetup_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *prec_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
);

static int integrator_ida_psolve_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *prec_data,
		 N_Vector tmp
);

static void integrator_ida_pcreate_jacobian(IntegratorSystem *sys);

static void integrator_ida_pfree_jacobian(IntegratorIdaData *enginedata);

static const IntegratorIdaPrec prec_jacobian = {
	integrator_ida_pcreate_jacobian
	, integrator_ida_psetup_jacobian
	, integrator_ida_psolve_jacobian
};

/*------
  Jacobi preconditioner -- experimental
*/

static int integrator_ida_psetup_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *prec_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
);

static int integrator_ida_psolve_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *prec_data,
		 N_Vector tmp
);

static void integrator_ida_pcreate_jacobi(IntegratorSystem *sys);

static void integrator_ida_pfree_jacobi(IntegratorIdaData *enginedata);

static const IntegratorIdaPrec prec_jacobi = {
	integrator_ida_pcreate_jacobi
	, integrator_ida_psetup_jacobi
	, integrator_ida_psolve_jacobi
};

/*-------------------------------------------------------------
  SETUP/TEARDOWN ROUTINES
*/
static void integrator_ida_create(IntegratorSystem *sys){
	CONSOLE_DEBUG("ALLOCATING IDA ENGINE DATA");
	IntegratorIdaData *enginedata;
	enginedata = ASC_NEW(IntegratorIdaData);
	CONSOLE_DEBUG("enginedata = %p",enginedata);
	enginedata->rellist = NULL;
	enginedata->safeeval = 0;
	enginedata->vfilter.matchbits =  VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | VAR_FIXED;
	enginedata->vfilter.matchvalue = VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | 0;
	enginedata->pfree = NULL;

	enginedata->rfilter.matchbits =  REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;
	enginedata->rfilter.matchvalue = REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;

	sys->enginedata = (void *)enginedata;

	integrator_ida_params_default(sys);
}

static void integrator_ida_free(void *enginedata){
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("DESTROYING IDA engine data at %p",enginedata);
#endif
	IntegratorIdaData *d = (IntegratorIdaData *)enginedata;
	asc_assert(d);
	if(d->pfree){
		CONSOLE_DEBUG("DESTROYING preconditioner data using fn at %p",d->pfree);
		/* free the preconditioner data, whatever it happens to be */
		(d->pfree)(enginedata);
	}

	ASC_FREE(d->rellist);

#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("Now destroying the enginedata");
#endif
	ASC_FREE(d);
#ifdef DESTROY_DEBUG
	CONSOLE_DEBUG("enginedata freed");
#endif
}

static IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *sys){
	IntegratorIdaData *d;
	assert(sys!=NULL);
	assert(sys->enginedata!=NULL);
	assert(sys->engine==INTEG_IDA);
	d = ((IntegratorIdaData *)(sys->enginedata));
	return d;
}

/*-------------------------------------------------------------
  PARAMETERS FOR IDA
*/

enum ida_parameters{
	IDA_PARAM_LINSOLVER
	,IDA_PARAM_MAXL
	,IDA_PARAM_MAXORD
	,IDA_PARAM_AUTODIFF
	,IDA_PARAM_CALCIC
	,IDA_PARAM_SAFEEVAL
	,IDA_PARAM_RTOL
	,IDA_PARAM_ATOL
	,IDA_PARAM_ATOLVECT
	,IDA_PARAM_GSMODIFIED
	,IDA_PARAM_MAXNCF
	,IDA_PARAM_PREC
		,IDA_PARAMS_SIZE
};

/**
	Here the full set of parameters is defined, along with upper/lower bounds,
	etc. The values are stuck into the sys->params structure.

	To add a new parameter, first give it a name IDA_PARAM_* in thge above enum ida_parameters
	list. Then add a slv_param_*(...) statement below to define the type, description and range
	for the new parameter.

	@return 0 on success
*/
static int integrator_ida_params_default(IntegratorSystem *sys){
	asc_assert(sys!=NULL);
	asc_assert(sys->engine==INTEG_IDA);
	slv_parameters_t *p;
	p = &(sys->params);

	slv_destroy_parms(p);

	if(p->parms==NULL){
		CONSOLE_DEBUG("params NULL");
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, IDA_PARAMS_SIZE);
		if(p->parms==NULL)return -1;
		p->dynamic_parms = 1;
	}else{
		CONSOLE_DEBUG("params not NULL");
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_bool(p,IDA_PARAM_AUTODIFF
		,(SlvParameterInitBool){{"autodiff"
			,"Use auto-diff?",1
			,"Use automatic differentiation of expressions (1) or use numerical derivatives (0)"
		}, TRUE}
	);

	slv_param_char(p,IDA_PARAM_CALCIC
		,(SlvParameterInitChar){{"calcic"
			,"Initial conditions calcuation",1
			,"Use specified values of ydot to solve for inital y (Y),"
			" or use the the values of the differential variables (yd) to solve"
			" for the pure algebraic variables (ya) along with the derivatives"
			" of the differential variables (yddot) (YA_YDP), or else don't solve"
			" the intial conditions at all (NONE). See IDA manual p 41 (IDASetId)"
		}, "YA_YDP"}, (char *[]){"Y", "YA_YDP", "NONE",NULL}
	);

	slv_param_bool(p,IDA_PARAM_SAFEEVAL
		,(SlvParameterInitBool){{"safeeval"
			,"Use safe evaluation?",1
			,"Use 'safe' function evaluation routines (TRUE) or allow ASCEND to "
			"throw SIGFPE errors which will then halt integration."
		}, FALSE}
	);


	slv_param_bool(p,IDA_PARAM_ATOLVECT
		,(SlvParameterInitBool){{"atolvect"
			,"Use 'ode_atol' values as specified?",1
			,"If TRUE, values of 'ode_atol' are taken from your model and used "
			" in the integration. If FALSE, a scalar absolute tolerance value"
			" is shared by all variables. See IDA manual, section 5.5.1"
		}, TRUE }
	);

	slv_param_real(p,IDA_PARAM_ATOL
		,(SlvParameterInitReal){{"atol"
			,"Scalar absolute error tolerance",1
			,"Value of the scalar absolute error tolerance. See also 'atolvect'."
			" See IDA manual, sections 5.5.1 and 5.5.2 'Advice on choice and use of tolerances'"
		}, 1e-5, 0, 1e10 }
	);

	slv_param_real(p,IDA_PARAM_RTOL
		,(SlvParameterInitReal){{"rtol"
			,"Scalar relative error tolerance",1
			,"Value of the scalar relative error tolerance. (Note that for IDA,"
			" it's not possible to set per-variable relative tolerances as it is"
			" with LSODE)."
			" See IDA manual, section 5.5.2 'Advice on choice and use of tolerances'"
		}, 1e-4, 0, 1 }
	);

	slv_param_char(p,IDA_PARAM_LINSOLVER
		,(SlvParameterInitChar){{"linsolver"
			,"Linear solver",1
			,"See IDA manual, section 5.5.3. Choose 'ASCEND' to use the linsolqr"
			" direct linear solver bundled with ASCEND, 'DENSE' to use the dense"
			" solver bundled with IDA, or one of the Krylov solvers SPGMR, SPBCG"
			" or SPTFQMR (which still need preconditioners to be implemented"
			" before they can be very useful."
		}, "DENSE"}, (char *[]){"ASCEND","DENSE","BAND","SPGMR","SPBCG","SPTFQMR",NULL}
	);

	slv_param_int(p,IDA_PARAM_MAXL
		,(SlvParameterInitInt){{"maxl"
			,"Maximum Krylov dimension",0
			,"The maximum dimension of Krylov space used by the linear solver"
			" (for SPGMR, SPBCG, SPTFQMR) with IDA. See IDA manual section 5.5."
			" The default of 0 results in IDA using its internal default, which"
			" is currently a value of 5."
		}, 0, 0, 20 }
	);

	slv_param_int(p,IDA_PARAM_MAXORD
		,(SlvParameterInitInt){{"maxord"
			,"Maximum order of linear multistep method",0
			,"The maximum order of the linear multistep method with IDA. See"
			" IDA manual p 38."
		}, 5, 1, 5 }
	);

	slv_param_bool(p,IDA_PARAM_GSMODIFIED
		,(SlvParameterInitBool){{"gsmodified"
			,"Gram-Schmidt Orthogonalisation Scheme", 2
			,"TRUE = GS_MODIFIED, FALSE = GS_CLASSICAL. See IDA manual section"
			" 5.5.6.6. Only applies when linsolve=SPGMR is selected."
		}, TRUE}
	);

	slv_param_int(p,IDA_PARAM_MAXNCF
		,(SlvParameterInitInt){{"maxncf"
			,"Max nonlinear solver convergence failures per step", 2
			,"Maximum number of allowable nonlinear solver convergence failures"
			" on one step. See IDA manual section 5.5.6.1."
		}, 10,0,1000 }
	);

	slv_param_char(p,IDA_PARAM_PREC
		,(SlvParameterInitChar){{"prec"
			,"Preconditioner",1
			,"See IDA manual, section section 5.6.8."
		},"NONE"}, (char *[]){"NONE","DIAG",NULL}
	);

	asc_assert(p->num_parms == IDA_PARAMS_SIZE);

	CONSOLE_DEBUG("Created %d params", p->num_parms);

	return 0;
}

/*-------------------------------------------------------------
  MAIN IDA SOLVER ROUTINE, see IDA manual, sec 5.4, p. 27 ff.
*/

/*static double div1(double a, double b){
	return a/b;
}*/

typedef int IdaFlagFn(void *,int *);
typedef char *IdaFlagNameFn(int);

/* return 0 on success */
static int integrator_ida_solve(
		IntegratorSystem *sys
		, unsigned long start_index
		, unsigned long finish_index
){
	void *ida_mem;
	int flag, flag1, t_index;
	realtype t0, reltol, abstol, t, tret, tout1;
	N_Vector y0, yp0, abstolvect, ypret, yret, id;
	IntegratorIdaData *enginedata;
	char *linsolver;
	int maxl;
	IdaFlagFn *flagfn;
	IdaFlagNameFn *flagnamefn;
	const char *flagfntype;
	char *pname = NULL;
	struct rel_relation **rels;
	int *rootsfound;
	char havecrossed;

#ifdef SOLVE_DEBUG
	char *varname, *relname;
#endif

	int i,j,n_activerels,n_solverrels;
	const IntegratorIdaPrec *prec = NULL;
	int icopt; /* initial conditions strategy */

	CONSOLE_DEBUG("STARTING IDA...");

	enginedata = integrator_ida_enginedata(sys);

	enginedata->safeeval = SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_SAFEEVAL);
	CONSOLE_DEBUG("safeeval = %d",enginedata->safeeval);

	/* store reference to list of relations (in enginedata) */
	n_solverrels = slv_get_num_solvers_rels(sys->system);

	n_activerels = slv_count_solvers_rels(sys->system, &integrator_ida_rel);

	enginedata->bndlist = slv_get_solvers_bnd_list(sys->system);
	enginedata->nbnds = slv_get_num_solvers_bnds(sys->system);

	enginedata->rellist = ASC_NEW_ARRAY(struct rel_relation *, n_activerels);

	rels = slv_get_solvers_rel_list(sys->system);

	j=0;
	for(i=0; i < n_solverrels; ++i){
		if(rel_apply_filter(rels[i], &integrator_ida_rel)){
#ifdef SOLVE_DEBUG
			relname = rel_make_name(sys->system, rels[i]);
			CONSOLE_DEBUG("rel '%s': 0x%x", relname, rel_flags(rels[i]));
			ASC_FREE(relname);
#endif
			enginedata->rellist[j++]=rels[i];
		}
	}
	asc_assert(j==n_activerels);

	CONSOLE_DEBUG("rels matchbits:  0x%x",integrator_ida_rel.matchbits);
	CONSOLE_DEBUG("rels matchvalue: 0x%x",integrator_ida_rel.matchvalue);

	CONSOLE_DEBUG("Number of relations: %d",n_solverrels);
	CONSOLE_DEBUG("Number of active relations: %d",n_activerels);
	CONSOLE_DEBUG("Number of dependent vars: %d",sys->n_y);
	CONSOLE_DEBUG("Number of boundaries: %d",enginedata->nbnds);

	enginedata->nrels = n_activerels;

	if(enginedata->nrels != sys->n_y){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Integration problem is not square (%d active rels, %d vars)"
			,n_activerels, sys->n_y
		);
		return 1; /* failure */
	}

#ifdef SOLVE_DEBUG
	integrator_ida_debug(sys,stderr);
#endif

	/* retrieve initial values from the system */

	/** @TODO fix this, the starting time != first sample */
	t0 = integrator_get_t(sys);
	CONSOLE_DEBUG("RETRIEVED t0 = %f",t0);

	CONSOLE_DEBUG("RETRIEVING y0");

	y0 = N_VNew_Serial(sys->n_y);
	integrator_get_y(sys,NV_DATA_S(y0));

#ifdef SOLVE_DEBUG
	CONSOLE_DEBUG("RETRIEVING yp0");
#endif

	yp0 = N_VNew_Serial(sys->n_y);
	integrator_get_ydot(sys,NV_DATA_S(yp0));

#ifdef SOLVE_DEBUG
	N_VPrint_Serial(yp0);
	CONSOLE_DEBUG("yp0 is at %p",&yp0);
#endif

	/* create IDA object */
	ida_mem = IDACreate();

	/* relative error tolerance */
	reltol = SLV_PARAM_REAL(&(sys->params),IDA_PARAM_RTOL);
	CONSOLE_DEBUG("rtol = %8.2e",reltol);


	/* allocate internal memory */
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
	flag = IDAInit(ida_mem, &integrator_ida_fex, t0, y0 ,yp0);
#else
	if(SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_ATOLVECT)){
		/* vector of absolute tolerances */
		CONSOLE_DEBUG("USING VECTOR OF ATOL VALUES");
		abstolvect = N_VNew_Serial(sys->n_y);
		integrator_get_atol(sys,NV_DATA_S(abstolvect));

		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, reltol, abstolvect);

		N_VDestroy_Serial(abstolvect);
	}else{
		/* scalar absolute tolerance (one value for all) */
		abstol = SLV_PARAM_REAL(&(sys->params),IDA_PARAM_ATOL);
		CONSOLE_DEBUG("USING SCALAR ATOL VALUE = %8.2e",abstol);
		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SS, reltol, &abstol);
	}
#endif

	if(flag==IDA_MEM_NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
		return 2;
	}else if(flag==IDA_MEM_FAIL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDAMalloc)");
		return 3;
	}else if(flag==IDA_ILL_INPUT){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid input to IDAMalloc");
		return 4;
	}/* else success */


#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
	CONSOLE_DEBUG("Assigning tolerances...");
	/* assign tolerances */
	if(SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_ATOLVECT)){
		CONSOLE_DEBUG("using vector of atol values");
		abstolvect = N_VNew_Serial(sys->n_y);
		integrator_get_atol(sys,NV_DATA_S(abstolvect));
		IDASVtolerances(ida_mem, reltol, abstolvect);
		N_VDestroy_Serial(abstolvect);
	}else{
		/* scalar tolerances */
		abstol = SLV_PARAM_REAL(&(sys->params),IDA_PARAM_ATOL);
		CONSOLE_DEBUG("using scalar atol value = %8.2e",abstol);
		IDASStolerances(ida_mem, reltol, abstol);
	}
#endif

	/* set optional inputs... */
	IDASetErrHandlerFn(ida_mem, &integrator_ida_error, (void *)sys);
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
	IDASetUserData(ida_mem, (void *)sys);
#else
	IDASetRdata(ida_mem, (void *)sys);
#endif
	IDASetMaxStep(ida_mem, integrator_get_maxstep(sys));
	IDASetInitStep(ida_mem, integrator_get_stepzero(sys));
	IDASetMaxNumSteps(ida_mem, integrator_get_maxsubsteps(sys));
	if(integrator_get_minstep(sys)>0){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"IDA does not support minstep (ignored)\n");
	}

	CONSOLE_DEBUG("MAXNCF = %d",SLV_PARAM_INT(&sys->params,IDA_PARAM_MAXNCF));
    IDASetMaxConvFails(ida_mem,SLV_PARAM_INT(&sys->params,IDA_PARAM_MAXNCF));

	CONSOLE_DEBUG("MAXORD = %d",SLV_PARAM_INT(&sys->params,IDA_PARAM_MAXORD));
    IDASetMaxOrd(ida_mem,SLV_PARAM_INT(&sys->params,IDA_PARAM_MAXORD));

	/* there's no capability for setting *minimum* step size in IDA */


	/* attach linear solver module, using the default value of maxl */
	linsolver = SLV_PARAM_CHAR(&(sys->params),IDA_PARAM_LINSOLVER);
	CONSOLE_DEBUG("ASSIGNING LINEAR SOLVER '%s'",linsolver);
	if(strcmp(linsolver,"ASCEND")==0){
		CONSOLE_DEBUG("ASCEND DIRECT SOLVER, size = %d",sys->n_y);
		IDAASCEND(ida_mem,sys->n_y);
		IDAASCENDSetJacFn(ida_mem, &integrator_ida_sjex, (void *)sys);

		flagfntype = "IDAASCEND";
		flagfn = &IDAASCENDGetLastFlag;
		flagnamefn = &IDAASCENDGetReturnFlagName;

	}else if(strcmp(linsolver,"DENSE")==0){
		CONSOLE_DEBUG("DENSE DIRECT SOLVER, size = %d",sys->n_y);
		flag = IDADense(ida_mem, sys->n_y);
		switch(flag){
			case IDADENSE_SUCCESS: break;
			case IDADENSE_MEM_NULL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL"); return 5;
			case IDADENSE_ILL_INPUT: ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDADENSE is not compatible with current nvector module"); return 5;
			case IDADENSE_MEM_FAIL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"Memory allocation failed for IDADENSE"); return 5;
			default: ERROR_REPORTER_HERE(ASC_PROG_ERR,"bad return"); return 5;
		}

		if(SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
			flag = IDADlsSetDenseJacFn(ida_mem, &integrator_ida_djex);
#else
			flag = IDADenseSetJacFn(ida_mem, &integrator_ida_djex, (void *)sys);
#endif
			switch(flag){
				case IDADENSE_SUCCESS: break;
				default: ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed IDADenseSetJacFn"); return 6;
			}
		}else{
			CONSOLE_DEBUG("USING NUMERICAL DIFF");
		}

		flagfntype = "IDADENSE";
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
		flagfn = &IDADlsGetLastFlag;
		flagnamefn = &IDADlsGetReturnFlagName;
#else
		flagfn = &IDADenseGetLastFlag;
		flagnamefn = &IDADenseGetReturnFlagName;
#endif
	}else{
		/* remaining methods are all SPILS */
		CONSOLE_DEBUG("IDA SPILS");

		maxl = SLV_PARAM_INT(&(sys->params),IDA_PARAM_MAXL);
		CONSOLE_DEBUG("maxl = %d",maxl);

		/* what preconditioner? */
		pname = SLV_PARAM_CHAR(&(sys->params),IDA_PARAM_PREC);
		if(strcmp(pname,"NONE")==0){
			prec = NULL;
		}else if(strcmp(pname,"JACOBI")==0){
			prec = &prec_jacobi;
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid preconditioner choice '%s'",pname);
			return 7;
		}

		/* which SPILS linear solver? */
		if(strcmp(linsolver,"SPGMR")==0){
			CONSOLE_DEBUG("IDA SPGMR");
			flag = IDASpgmr(ida_mem, maxl); /* 0 means use the default max Krylov dimension of 5 */
		}else if(strcmp(linsolver,"SPBCG")==0){
			CONSOLE_DEBUG("IDA SPBCG");
			flag = IDASpbcg(ida_mem, maxl);
		}else if(strcmp(linsolver,"SPTFQMR")==0){
			CONSOLE_DEBUG("IDA SPTFQMR");
			flag = IDASptfqmr(ida_mem,maxl);
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unknown IDA linear solver choice '%s'",linsolver);
			return 8;
		}

		if(prec){
			/* assign the preconditioner to the linear solver */
			(prec->pcreate)(sys);
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
			IDASpilsSetPreconditioner(ida_mem,prec->psetup,prec->psolve);
#else
			IDASpilsSetPreconditioner(ida_mem,prec->psetup,prec->psolve,(void *)sys);
#endif
			CONSOLE_DEBUG("PRECONDITIONER = %s",pname);
		}else{
			CONSOLE_DEBUG("No preconditioner");
		}

		flagfntype = "IDASPILS";
		flagfn = &IDASpilsGetLastFlag;
		flagnamefn = &IDASpilsGetReturnFlagName;

		if(flag==IDASPILS_MEM_NULL){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
			return 9;
		}else if(flag==IDASPILS_MEM_FAIL){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDASpgmr)");
			return 9;
		}/* else success */

		/* assign the J*v function */
		if(SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
		    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex);
#else
		    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex, (void *)sys);
#endif
			if(flag==IDASPILS_MEM_NULL){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
				return 10;
			}else if(flag==IDASPILS_LMEM_NULL){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDASPILS linear solver has not been initialized");
				return 10;
			}/* else success */
		}else{
			CONSOLE_DEBUG("USING NUMERICAL DIFF");
		}

		if(strcmp(linsolver,"SPGMR")==0){
			/* select Gram-Schmidt orthogonalisation */
			if(SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_GSMODIFIED)){
				CONSOLE_DEBUG("USING MODIFIED GS");
				flag = IDASpilsSetGSType(ida_mem,MODIFIED_GS);
				if(flag!=IDASPILS_SUCCESS){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 11;
				}
			}else{
				CONSOLE_DEBUG("USING CLASSICAL GS");
				flag = IDASpilsSetGSType(ida_mem,CLASSICAL_GS);
				if(flag!=IDASPILS_SUCCESS){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 11;
				}
			}
		}
	}

	/* set linear solver optional inputs...
		...nothing here at the moment...
	*/

	/* calculate initial conditions */
	icopt = 0;
	if(strcmp(SLV_PARAM_CHAR(&sys->params,IDA_PARAM_CALCIC),"Y")==0){
		CONSOLE_DEBUG("Solving initial conditions using values of yddot");
		icopt = IDA_Y_INIT;
		asc_assert(icopt!=0);
	}else if(strcmp(SLV_PARAM_CHAR(&sys->params,IDA_PARAM_CALCIC),"YA_YDP")==0){
		CONSOLE_DEBUG("Solving initial conditions using values of yd");
		icopt = IDA_YA_YDP_INIT;
		asc_assert(icopt!=0);
		id = N_VNew_Serial(sys->n_y);
		for(i=0; i < sys->n_y; ++i){
			if(sys->ydot[i] == NULL){
				NV_Ith_S(id,i) = 0.0;
#ifdef SOLVE_DEBUG
				varname = var_make_name(sys->system,sys->y[i]);
				CONSOLE_DEBUG("y[%d] = '%s' is pure algebraic",i,varname);
				ASC_FREE(varname);
#endif
			}else{
#ifdef SOLVE_DEBUG
				CONSOLE_DEBUG("y[%d] is differential",i);
#endif
				NV_Ith_S(id,i) = 1.0;
			}
		}
		IDASetId(ida_mem, id);
		N_VDestroy_Serial(id);
	}else if(strcmp(SLV_PARAM_CHAR(&sys->params,IDA_PARAM_CALCIC),"NONE")==0){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Not solving initial conditions: check current residuals");
	}else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid 'iccalc' value: check solver parameters.");
	}

	if(icopt){
		sys->currentstep=0;
	 	t_index=start_index + 1;
		tout1 = samplelist_get(sys->samples, t_index);

		CONSOLE_DEBUG("SOLVING INITIAL CONDITIONS IDACalcIC (tout1 = %f)", tout1);

#ifdef ASC_SIGNAL_TRAPS
		/* catch SIGFPE if desired to */
		if(enginedata->safeeval){
			CONSOLE_DEBUG("SETTING TO IGNORE SIGFPE...");
			Asc_SignalHandlerPush(SIGFPE,SIG_DFL);
		}else{
# ifdef FEX_DEBUG
			CONSOLE_DEBUG("SETTING TO CATCH SIGFPE...");
# endif
			Asc_SignalHandlerPushDefault(SIGFPE);
		}
		if (setjmp(g_fpe_env)==0) {
#endif

# if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=3
		flag = IDACalcIC(ida_mem, icopt, tout1);/* new API from v2.3  */
# else
		flag = IDACalcIC(ida_mem, t0, y0, yp0, icopt, tout1);
# endif
		/* check flags and output status */
		switch(flag){
			case IDA_SUCCESS:
				CONSOLE_DEBUG("Initial conditions solved OK");
				break;

			case IDA_LSETUP_FAIL:
			case IDA_LINIT_FAIL:
			case IDA_LSOLVE_FAIL:
			case IDA_NO_RECOVERY:
				flag1 = -999;
				flag = (flagfn)(ida_mem,&flag1);
				if(flag){
					ERROR_REPORTER_HERE(ASC_PROG_ERR
						,"Unable to retrieve error code from %s (err %d)"
						,flagfntype,flag
					);
					return 12;
				}
				ERROR_REPORTER_HERE(ASC_PROG_ERR
					,"%s returned flag '%s' (value = %d)"
					,flagfntype,(flagnamefn)(flag1),flag1
				);
				return 12;

			default:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve initial condition (IDACalcIC)");
				return 12;
		}
#ifdef ASC_SIGNAL_TRAPS
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error while solving initial conditions");
			return 13;
		}

		if(enginedata->safeeval){
			Asc_SignalHandlerPop(SIGFPE,SIG_DFL);
		}else{
			CONSOLE_DEBUG("pop...");
			Asc_SignalHandlerPopDefault(SIGFPE);
			CONSOLE_DEBUG("...pop");
		}
#endif
	}/* icopt */

	/* optionally, specify ROO-FINDING problem */
	if(enginedata->nbnds){
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
		IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn);
#else
		IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn, (void *)sys);
#endif
	}

	/* -- set up the IntegratorReporter */
	integrator_output_init(sys);

	/* -- store the initial values of all the stuff */
	integrator_output_write(sys);
	integrator_output_write_obs(sys);

	/* specify where the returned values should be stored */
	yret = y0;
	ypret = yp0;

	/* advance solution in time, return values as yret and derivatives as ypret */
	sys->currentstep=1;
	for(t_index=start_index+1;t_index <= finish_index;++t_index, ++sys->currentstep){
		t = samplelist_get(sys->samples, t_index);
		t0 = integrator_get_t(sys);
		asc_assert(t > t0);

#ifdef SOLVE_DEBUG
		CONSOLE_DEBUG("Integrating from t0 = %f to t = %f", t0, t);
#endif

#ifdef ASC_SIGNAL_TRAPS
		Asc_SignalHandlerPushDefault(SIGINT);
		if(setjmp(g_int_env)==0) {
#endif
		flag = IDASolve(ida_mem, t, &tret, yret, ypret, IDA_NORMAL);
#ifdef ASC_SIGNAL_TRAPS
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Caught interrupt");
			flag = -555;
		}
		Asc_SignalHandlerPopDefault(SIGINT);
#endif

		/* check for roots found */
		if(enginedata->nbnds){
			rootsfound = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);
			havecrossed = 0;
			if(IDA_SUCCESS == IDAGetRootInfo(ida_mem, rootsfound)){
				for(i=0; i < enginedata->nbnds; ++i){
					if(rootsfound[i]){
						havecrossed = 1;
#ifdef SOLVE_DEBUG
						relname = bnd_make_name(sys->system,enginedata->bndlist[i]);
						ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Boundary '%s' crossed",relname);
						ASC_FREE(relname);
#else
						ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Boundary %d crossed!", i);
#endif
					}
				}

				/* so, now we need to restart the integration. we will assume that
				everything changes: number of variables, etc, etc, etc. */

				if(havecrossed){
					CONSOLE_DEBUG("Boundaries were crossed; need to reinitialise solver...");
					/** try resetting the boundary states now? */
					//IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn, (void *)sys);

#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
					IDAReInit(ida_mem, tret, yret, ypret);
#elif SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR<3
					/* TODO find out what version needs the following line... not sure. */
					//IDAReInit(ida_mem, &integrator_ida_fex, tret, yret, ypret);

					// FIXME this stuff has not been tested yet, and is very incomplete.

					if(SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_ATOLVECT)){
						/* vector of absolute tolerances */
						CONSOLE_DEBUG("USING VECTOR OF ATOL VALUES");
						abstolvect = N_VNew_Serial(sys->n_y);
						integrator_get_atol(sys,NV_DATA_S(abstolvect));
						flag = IDAReInit(ida_mem, &integrator_ida_fex, tret, yret, ypret, IDA_SV, reltol, abstolvect);
						N_VDestroy_Serial(abstolvect);
					}else{
						/* scalar absolute tolerance (one value for all) */
						abstol = SLV_PARAM_REAL(&(sys->params),IDA_PARAM_ATOL);
						CONSOLE_DEBUG("USING SCALAR ATOL VALUE = %8.2e",abstol);
						flag = IDAReInit(ida_mem, &integrator_ida_fex, tret, yret, ypret, IDA_SS, reltol, &abstol);
					}
#else
					/* allocate internal memory */
					if(SLV_PARAM_BOOL(&(sys->params),IDA_PARAM_ATOLVECT)){
						/* vector of absolute tolerances */
						abstolvect = N_VNew_Serial(sys->n_y);
						integrator_get_atol(sys,NV_DATA_S(abstolvect));
						if(IDA_SUCCESS != IDAReInit(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, reltol, abstolvect)){
							ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to reinitialise IDA");
						}
						N_VDestroy_Serial(abstolvect);
					}else{
						/* scalar absolute tolerance (one value for all) */
						abstol = SLV_PARAM_REAL(&(sys->params),IDA_PARAM_ATOL);
						if(IDA_SUCCESS != IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SS, reltol, &abstol)){
							ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to reinitialise IDA");
						}
					}
#endif
				}
			}else{
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to fetch boundary-crossing info");
			}
			ASC_FREE(rootsfound);
		}


		/* pass the values of everything back to the compiler */
		integrator_set_t(sys, (double)tret);
		integrator_set_y(sys, NV_DATA_S(yret));
		integrator_set_ydot(sys, NV_DATA_S(ypret));

		if(flag<0){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve t = %f (IDASolve), error %d", t, flag);
			break;
		}

		/* -- do something so that sys knows the values of tret, yret and ypret */

		/* -- store the current values of all the stuff */
		integrator_output_write(sys);
		integrator_output_write_obs(sys);

	}/* loop through next sample timestep */

	/* -- close the IntegratorReporter */
	integrator_output_close(sys);

	/* get optional outputs */
#ifdef STATS_DEBUG
	IntegratorIdaStats stats;
	if(IDA_SUCCESS == integrator_ida_stats(ida_mem, &stats)){
		integrator_ida_write_stats(&stats);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to fetch stats!?!?");
	}
#endif

	/* free solution memory */
	N_VDestroy_Serial(yret);
	N_VDestroy_Serial(ypret);

	/* free solver memory */
	IDAFree(ida_mem);

	if(flag < -500){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Interrupted while attempting t = %f", t);
		return -flag;
	}

	if(flag < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Solving aborted while attempting t = %f", t);
		return 14;
	}

	/* all done, success */
	return 0;
}

/*--------------------------------------------------
  RESIDUALS AND JACOBIAN AND IDAROOTFN
*/

#if 0
typedef void (SignalHandlerFn)(int);
SignalHandlerFn integrator_ida_sig;
SignalHandlerFn *integrator_ida_sig_old;
jmp_buf integrator_ida_jmp_buf;
fenv_t integrator_ida_fenv_old;


void integrator_ida_write_feinfo(){
	int f;
	f = fegetexcept();
	CONSOLE_DEBUG("Locating nature of exception...");
	if(f & FE_DIVBYZERO)ERROR_REPORTER_HERE(ASC_PROG_ERR,"DIV BY ZERO");
	if(f & FE_INEXACT)ERROR_REPORTER_HERE(ASC_PROG_ERR,"INEXACT");
	if(f & FE_INVALID)ERROR_REPORTER_HERE(ASC_PROG_ERR,"INVALID");
	if(f & FE_OVERFLOW)ERROR_REPORTER_HERE(ASC_PROG_ERR,"OVERFLOW");
	if(f & FE_UNDERFLOW)ERROR_REPORTER_HERE(ASC_PROG_ERR,"UNDERFLOW");
	if(f==0)ERROR_REPORTER_HERE(ASC_PROG_ERR,"FLAGS ARE CLEAR?!?");
}

void integrator_ida_sig(int sig){
	/* the wrong signal: rethrow to the default handler */
	if(sig!=SIGFPE){
		signal(SIGFPE,SIG_DFL);
		raise(sig);
	}
	integrator_ida_write_feinfo();
	CONSOLE_DEBUG("Caught SIGFPE=%d (in signal handler). Jumping to...",sig);
	longjmp(integrator_ida_jmp_buf,sig);
}
#endif

/**
	Function to evaluate system residuals, in the form required for IDA.

	Given tt, yy and yp, we need to evaluate and return rr.

	@param tt current value of indep variable (time)
	@param yy current values of dependent variable vector
	@param yp current values of derivatives of dependent variables
	@param rr the output residual vector (is we're returning data to)
	@param res_data pointer to our stuff (sys in this case).

	@return 0 on success, positive on recoverable error, and
		negative on unrecoverable error.
*/
static int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data){
	IntegratorSystem *sys;
	IntegratorIdaData *enginedata;
	int i, calc_ok, is_error;
	struct rel_relation** relptr;
	double resid;
	char *relname;
#ifdef FEX_DEBUG
	char *varname;
	char diffname[30];
#endif

	sys = (IntegratorSystem *)res_data;
	enginedata = integrator_ida_enginedata(sys);

#ifdef FEX_DEBUG
	/* fprintf(stderr,"\n\n"); */
	CONSOLE_DEBUG("EVALUTE RESIDUALS...");
#endif

	if(NV_LENGTH_S(rr)!=enginedata->nrels){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid residuals nrels!=length(rr)");
		return -1; /* unrecoverable */
	}

	/* pass the values of everything back to the compiler */
	integrator_set_t(sys, (double)tt);
	integrator_set_y(sys, NV_DATA_S(yy));
	integrator_set_ydot(sys, NV_DATA_S(yp));

	/* perform bounds checking on all variables */
	if(slv_check_bounds(sys->system, 0, -1, NULL)){
		/* ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Variable(s) out of bounds"); */
		return 1;
	}

	/* evaluate each residual in the rellist */
	is_error = 0;
	relptr = enginedata->rellist;


#ifdef ASC_SIGNAL_TRAPS
	if(enginedata->safeeval){
		Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
	}else{
# ifdef FEX_DEBUG
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SETTING TO CATCH SIGFPE...");
# endif
		Asc_SignalHandlerPushDefault(SIGFPE);
	}

	if (SETJMP(g_fpe_env)==0) {
#endif


	for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
	){
		resid = relman_eval(*relptr, &calc_ok, enginedata->safeeval);

		NV_Ith_S(rr,i) = resid;
		if(!calc_ok){
			relname = rel_make_name(sys->system, *relptr);
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Calculation error in rel '%s'",relname);
			ASC_FREE(relname);
			/* presumable some output already made? */
			is_error = 1;
		}/*else{
			CONSOLE_DEBUG("Calc OK");
		}*/
	}

	if(!is_error){
		for(i=0;i< enginedata->nrels; ++i){
			if(isnan(NV_Ith_S(rr,i))){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"NAN detected in residual %d",i);
				is_error=1;
			}
		}
#ifdef FEX_DEBUG
		if(!is_error){
			CONSOLE_DEBUG("No NAN detected");
		}
#endif
	}

#ifdef ASC_SIGNAL_TRAPS
	}else{
		relname = rel_make_name(sys->system, *relptr);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error (SIGFPE) in rel '%s'",relname);
		ASC_FREE(relname);
		is_error = 1;
	}

	if(enginedata->safeeval){
		Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
	}else{
		Asc_SignalHandlerPopDefault(SIGFPE);
	}
#endif


#ifdef FEX_DEBUG
	/* output residuals to console */
	CONSOLE_DEBUG("RESIDUAL OUTPUT");
	fprintf(stderr,"index\t%25s\t%25s\t%s\n","y","ydot","resid");
	for(i=0; i<sys->n_y; ++i){
		varname = var_make_name(sys->system,sys->y[i]);
		fprintf(stderr,"%d\t%15s=%10f\t",i,varname,NV_Ith_S(yy,i));
		if(sys->ydot[i]){
			varname = var_make_name(sys->system,sys->ydot[i]);
			fprintf(stderr,"%15s=%10f\t",varname,NV_Ith_S(yp,i));
		}else{
			snprintf(diffname,99,"diff(%s)",varname);
			fprintf(stderr,"%15s=%10f\t",diffname,NV_Ith_S(yp,i));
		}
		ASC_FREE(varname);
		relname = rel_make_name(sys->system,enginedata->rellist[i]);
		fprintf(stderr,"'%s'=%f (%p)\n",relname,NV_Ith_S(rr,i),enginedata->rellist[i]);
	}
#endif

	if(is_error){
		return 1;
	}

#ifdef FEX_DEBUG
	CONSOLE_DEBUG("RESIDUAL OK");
#endif
	return 0;
}

/**
	Dense Jacobian evaluation. Only suitable for small problems!
	Has been seen working for problems up to around 2000 vars, FWIW.
*/
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
static int integrator_ida_djex(int Neq, realtype tt, realtype c_j
		, N_Vector yy, N_Vector yp, N_Vector rr
		, IDA_MTX_T Jac, void *jac_data
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
#else
static int integrator_ida_djex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, IDA_MTX_T Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
#endif
	IntegratorSystem *sys;
	IntegratorIdaData *enginedata;
	char *relname;
#ifdef DJEX_DEBUG
	struct var_variable **varlist;
	char *varname;
#endif
	struct rel_relation **relptr;
	int i;
	double *derivatives;
	struct var_variable **variables;
	int count, j;
	int status, is_error = 0;

	sys = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(sys);

	/* allocate space for returns from relman_diff3 */
	/** @TODO instead, we should use 'tmp1' and 'tmp2' here... */
	variables = ASC_NEW_ARRAY(struct var_variable*, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/* pass the values of everything back to the compiler */
	integrator_set_t(sys, (double)tt);
	integrator_set_y(sys, NV_DATA_S(yy));
	integrator_set_ydot(sys, NV_DATA_S(yp));

	/* perform bounds checking on all variables */
	if(slv_check_bounds(sys->system, 0, -1, NULL)){
		/* ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Variable(s) out of bounds"); */
		return 1;
	}

#ifdef DJEX_DEBUG
	varlist = slv_get_solvers_var_list(sys->system);

	/* print vars */
	for(i=0; i < sys->n_y; ++i){
		varname = var_make_name(sys->system, sys->y[i]);
		CONSOLE_DEBUG("%s = %f",varname,NV_Ith_S(yy,i));
		asc_assert(NV_Ith_S(yy,i) == var_value(sys->y[i]));
		ASC_FREE(varname);
	}

	/* print derivatives */
	for(i=0; i < sys->n_y; ++i){
		if(sys->ydot[i]){
			varname = var_make_name(sys->system, sys->ydot[i]);
			CONSOLE_DEBUG("%s = %f =%g",varname,NV_Ith_S(yp,i),var_value(sys->ydot[i]));
			ASC_FREE(varname);
		}else{
			varname = var_make_name(sys->system, sys->y[i]);
			CONSOLE_DEBUG("diff(%s) = %g",varname,NV_Ith_S(yp,i));
			ASC_FREE(varname);
		}
	}

	/* print step size */
	CONSOLE_DEBUG("<c_j> = %g",c_j);
#endif

	/* build up the dense jacobian matrix... */
	status = 0;
	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);

		if(status){
			relname = rel_make_name(sys->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			is_error = 1;
			break;
		}

		/* output what's going on here ... */
#ifdef DJEX_DEBUG
		relname = rel_make_name(sys->system, *relptr);
		fprintf(stderr,"%d: '%s': ",i,relname);
		for(j=0;j<count;++j){
			varname = var_make_name(sys->system, variables[j]);
			if(var_deriv(variables[j])){
				fprintf(stderr,"  '%s'=",varname);
				fprintf(stderr,"ydot[%d]",integrator_ida_diffindex(sys,variables[j]));
			}else{
				fprintf(stderr,"  '%s'=y[%d]",varname,var_sindex(variables[j]));
			}
			ASC_FREE(varname);
		}
		/* relname is freed further down */
		fprintf(stderr,"\n");
#endif

		/* insert values into the Jacobian row in appropriate spots (can assume Jac starts with zeros -- IDA manual) */
		for(j=0; j < count; ++j){
#ifdef DJEX_DEBUG
			varname = var_make_name(sys->system,variables[j]);
			fprintf(stderr,"d(%s)/d(%s) = %g",relname,varname,derivatives[j]);
			ASC_FREE(varname);
#endif
			if(!var_deriv(variables[j])){
#ifdef DJEX_DEBUG
				fprintf(stderr," --> J[%d,%d] += %g\n", i,j,derivatives[j]);
				asc_assert(var_sindex(variables[j]) >= 0);
				ASC_ASSERT_LT(var_sindex(variables[j]) , Neq);
#endif
				DENSE_ELEM(Jac,i,var_sindex(variables[j])) += derivatives[j];
			}else{
				DENSE_ELEM(Jac,i,integrator_ida_diffindex(sys,variables[j])) += derivatives[j] * c_j;
#ifdef DJEX_DEBUG
				fprintf(stderr," --> * c_j --> J[%d,%d] += %g\n", i,j,derivatives[j] * c_j);
#endif
			}
		}
	}

#ifdef DJEX_DEBUG
	ASC_FREE(relname);
	CONSOLE_DEBUG("PRINTING JAC");
	fprintf(stderr,"\t");
	for(j=0; j < sys->n_y; ++j){
		if(j)fprintf(stderr,"\t");
		varname = var_make_name(sys->system,sys->y[j]);
		fprintf(stderr,"%11s",varname);
		ASC_FREE(varname);
	}
	fprintf(stderr,"\n");
	for(i=0; i < enginedata->nrels; ++i){
		relname = rel_make_name(sys->system, enginedata->rellist[i]);
		fprintf(stderr,"%s\t",relname);
		ASC_FREE(relname);

		for(j=0; j < sys->n_y; ++j){
			if(j)fprintf(stderr,"\t");
			fprintf(stderr,"%11.2e",DENSE_ELEM(Jac,i,j));
		}
		fprintf(stderr,"\n");
	}
#endif

	/* test for NANs */
	if(!is_error){
		for(i=0;i< enginedata->nrels; ++i){
			for(j=0;j<sys->n_y;++j){
				if(isnan(DENSE_ELEM(Jac,i,j))){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"NAN detected in jacobian J[%d,%d]",i,j);
					is_error=1;
				}
			}
		}
#ifdef DJEX_DEBUG
		if(!is_error){
			CONSOLE_DEBUG("No NAN detected");
		}
#endif
	}

/*	if(integrator_ida_check_diffindex(sys)){
		is_error = 1;
	}*/

	if(is_error){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"There were derivative evaluation errors in the dense jacobian");
		return 1;
	}

#ifdef DJEX_DEBUG
	CONSOLE_DEBUG("DJEX RETURNING 0");
	/* ASC_PANIC("Quitting"); */
#endif
	return 0;
}

/**
	Function to evaluate the product J*v, in the form required for IDA (see IDASpilsSetJacTimesVecFn)

	Given tt, yy, yp, rr and v, we need to evaluate and return Jv.

	@param tt current value of the independent variable (time, t)
	@param yy current value of the dependent variable vector, y(t).
	@param yp current value of y'(t).
	@param rr current value of the residual vector F(t, y, y').
	@param v  the vector by which the Jacobian must be multiplied to the right.
	@param Jv the output vector computed
	@param c_j the scalar in the system Jacobian, proportional to the inverse of the step size ($ \alpha$ in Eq. (3.5) ).
	@param jac_data pointer to our stuff (sys in this case, passed into IDA via IDASp*SetJacTimesVecFn.)
	@param tmp1 @see tmp2
	@param tmp2 (as well as tmp1) pointers to memory allocated for variables of type N_Vector for use here as temporary storage or work space.
	@return 0 on success
*/
static int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
){
	IntegratorSystem *sys;
	IntegratorIdaData *enginedata;
	int i, j, is_error=0;
	struct rel_relation** relptr = 0;
	char *relname;
	int status;
	double Jv_i;

	struct var_variable **variables;
	double *derivatives;
	int count;
	struct var_variable **varlist;
#ifdef JEX_DEBUG

	CONSOLE_DEBUG("EVALUATING JACOBIAN...");
#endif

	sys = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(sys);
	varlist = slv_get_solvers_var_list(sys->system);

	/* pass the values of everything back to the compiler */
	integrator_set_t(sys, (double)tt);
	integrator_set_y(sys, NV_DATA_S(yy));
	integrator_set_ydot(sys, NV_DATA_S(yp));
	/* no real use for residuals (rr) here, I don't think? */

	/* allocate space for returns from relman_diff2: we *should* be able to use 'tmp1' and 'tmp2' here... */

	i = NV_LENGTH_S(yy) * 2;
#ifdef JEX_DEBUG
	CONSOLE_DEBUG("Allocating 'variables' with length %d",i);
#endif
	variables = ASC_NEW_ARRAY(struct var_variable*, i);
	derivatives = ASC_NEW_ARRAY(double, i);

	/* evaluate the derivatives... */
	/* J = dG_dy = dF_dy + alpha * dF_dyp */

#ifdef ASC_SIGNAL_TRAPS
	Asc_SignalHandlerPushDefault(SIGFPE);
	if (SETJMP(g_fpe_env)==0) {
#endif
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			/* get derivatives for this particular relation */
			status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
#ifdef JEX_DEBUG
			CONSOLE_DEBUG("Got derivatives against %d matching variables, status = %d", count,status);
#endif

			if(status){
				relname = rel_make_name(sys->system, *relptr);
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Calculation error in rel '%s'",relname);
				ASC_FREE(relname);
				is_error = 1;
				break;
			}

			/*
				Now we have the derivatives wrt each alg/diff variable in the
				present equation. variables[] points into the varlist. need
				a mapping from the varlist to the y and ydot lists.
			*/

			Jv_i = 0;
			for(j=0; j < count; ++j){
				/* CONSOLE_DEBUG("j = %d, variables[j] = %d, n_y = %ld", j, variables[j], sys->n_y);
				varname = var_make_name(sys->system, enginedata->varlist[variables[j]]);
				if(varname){
					CONSOLE_DEBUG("Variable %d '%s' derivative = %f", variables[j],varname,derivatives[j]);
					ASC_FREE(varname);
				}else{
					CONSOLE_DEBUG("Variable %d (UNKNOWN!): derivative = %f",variables[j],derivatives[j]);
				}
				*/

				/* we don't calculate derivatives wrt indep var */
				asc_assert(variables[j]>=0);
				if(variables[j] == sys->x) continue;
#ifdef JEX_DEBUG
				CONSOLE_DEBUG("j = %d: variables[j] = %d",j,var_sindex(variables[j]));
#endif
				if(var_deriv(variables[j])){
#define DIFFINDEX integrator_ida_diffindex(sys,variables[j])
#ifdef JEX_DEBUG
					fprintf(stderr,"Jv[%d] += %f (dF[%d]/dydot[%d] = %f, v[%d] = %f)\n", i
						, derivatives[j] * NV_Ith_S(v,DIFFINDEX)
						, i, DIFFINDEX, derivatives[j]
						, DIFFINDEX, NV_Ith_S(v,DIFFINDEX)
					);
#endif
					asc_assert(sys->ydot[DIFFINDEX]==variables[j]);
					Jv_i += derivatives[j] * NV_Ith_S(v,DIFFINDEX) * c_j;
#undef DIFFINDEX
				}else{
#define VARINDEX var_sindex(variables[j])
#ifdef JEX_DEBUG
					asc_assert(sys->y[VARINDEX]==variables[j]);
					fprintf(stderr,"Jv[%d] += %f (dF[%d]/dy[%d] = %f, v[%d] = %f)\n"
						, i
						, derivatives[j] * NV_Ith_S(v,VARINDEX)
						, i, VARINDEX, derivatives[j]
						, VARINDEX, NV_Ith_S(v,VARINDEX)
					);
#endif
					Jv_i += derivatives[j] * NV_Ith_S(v,VARINDEX);
#undef VARINDEX
				}
			}

			NV_Ith_S(Jv,i) = Jv_i;
#ifdef JEX_DEBUG
			CONSOLE_DEBUG("rel = %p",*relptr);
			relname = rel_make_name(sys->system, *relptr);
			CONSOLE_DEBUG("'%s': Jv[%d] = %f", relname, i, NV_Ith_S(Jv,i));
			ASC_FREE(relname);
			return 1;
#endif
		}
#ifdef ASC_SIGNAL_TRAPS
	}else{
		relname = rel_make_name(sys->system, *relptr);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error (SIGFPE) in rel '%s'",relname);
		ASC_FREE(relname);
		is_error = 1;
	}
	Asc_SignalHandlerPopDefault(SIGFPE);
#endif

	if(is_error){
		CONSOLE_DEBUG("SOME ERRORS FOUND IN EVALUATION");
		return 1;
	}
	return 0;
}

/* sparse jacobian evaluation for IDAASCEND sparse direct linear solver */
static int integrator_ida_sjex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, mtx_matrix_t Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return -1;
}

/* root finding function */

int integrator_ida_rootfn(realtype tt, N_Vector yy, N_Vector yp, realtype *gout, void *g_data){
	IntegratorSystem *sys;
	IntegratorIdaData *enginedata;
	int i;
#ifdef ROOT_DEBUG
	char *relname;
#endif

	asc_assert(g_data!=NULL);
	sys = (IntegratorSystem *)g_data;
	enginedata = integrator_ida_enginedata(sys);

	/* pass the values of everything back to the compiler */
	integrator_set_t(sys, (double)tt);
	integrator_set_y(sys, NV_DATA_S(yy));
	integrator_set_ydot(sys, NV_DATA_S(yp));

	asc_assert(gout!=NULL);

#ifdef ROOT_DEBUG
	CONSOLE_DEBUG("t = %f",tt);
#endif

	/* evaluate the residuals for each of the boundaries */
	for(i=0; i < enginedata->nbnds; ++i){
		switch(bnd_kind(enginedata->bndlist[i])){
			case e_bnd_rel: /* real-valued boundary relation */
				gout[i] = bndman_real_eval(enginedata->bndlist[i]);
#ifdef ROOT_DEBUG
				relname = bnd_make_name(sys->system,enginedata->bndlist[i]);
				CONSOLE_DEBUG("gout[%d] = %f (boundary '%s')", i, gout[i], relname);
				ASC_FREE(relname);
#endif
				break;
			case e_bnd_logrel:
				if(bndman_log_eval(enginedata->bndlist[i])){
					CONSOLE_DEBUG("bnd[%d] = TRUE",i);
#ifdef ROOT_DEBUG
					relname = bnd_make_name(sys->system,enginedata->bndlist[i]);
					CONSOLE_DEBUG("gout[%d] = %f (boundary '%s')", i, gout[i], relname);
					ASC_FREE(relname);
#endif
					gout[i] = +1.0;
				}else{
					CONSOLE_DEBUG("bnd[%d] = FALSE",i);
					gout[i] = -1.0;
				}
				break;
			case e_bnd_undefined:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid boundary type e_bnd_undefined");
				return 1;
		}
	}

	return 0; /* no way to detect errors in bndman_*_eval at this stage */
}


/*----------------------------------------------
  FULL JACOBIAN PRECONDITIONER -- EXPERIMENTAL.
*/

static void integrator_ida_pcreate_jacobian(IntegratorSystem *sys){
	IntegratorIdaData *enginedata =sys->enginedata;
	IntegratorIdaPrecDataJacobian *precdata;
	precdata = ASC_NEW(IntegratorIdaPrecDataJacobian);
	mtx_matrix_t P;
	asc_assert(sys->n_y);
	precdata->L = linsolqr_create_default();

	/* allocate matrix to be used by linsolqr */
	P = mtx_create();
	mtx_set_order(P, sys->n_y);
	linsolqr_set_matrix(precdata->L, P);

	enginedata->pfree = &integrator_ida_pfree_jacobian;
	enginedata->precdata = precdata;
	CONSOLE_DEBUG("Allocated memory for Full Jacobian preconditioner");
}

static void integrator_ida_pfree_jacobian(IntegratorIdaData *enginedata){
	mtx_matrix_t P;
	IntegratorIdaPrecDataJacobian *precdata;

	if(enginedata->precdata){
		precdata = (IntegratorIdaPrecDataJacobian *)enginedata->precdata;
		P = linsolqr_get_matrix(precdata->L);
		mtx_destroy(P);
		linsolqr_destroy(precdata->L);
		ASC_FREE(precdata);
		enginedata->precdata = NULL;

		CONSOLE_DEBUG("Freed memory for Full Jacobian preconditioner");
	}
	enginedata->pfree = NULL;
}

/**
	EXPERIMENTAL. Full Jacobian preconditioner for use with IDA Krylov solvers

	'setup' function.
*/
static int integrator_ida_psetup_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *p_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
){
	int i, j, res;
	IntegratorSystem *sys;
	IntegratorIdaData *enginedata;
	IntegratorIdaPrecDataJacobian *precdata;
	linsolqr_system_t L;
	mtx_matrix_t P;
	struct rel_relation **relptr;

	sys = (IntegratorSystem *)p_data;
	enginedata = sys->enginedata;
	precdata = (IntegratorIdaPrecDataJacobian *)(enginedata->precdata);
	double *derivatives;
	struct var_variable **variables;
	int count, status;
	char *relname;
	mtx_coord_t C;

	L = precdata->L;
	P = linsolqr_get_matrix(L);
	mtx_clear(P);

	CONSOLE_DEBUG("Setting up Jacobian preconditioner");

	variables = ASC_NEW_ARRAY(struct var_variable*, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/**
		@TODO FIXME here we are using the very inefficient and contorted approach
		of calculating the whole jacobian, then extracting just the diagonal elements.
	*/

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			relname = rel_make_name(sys->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating preconditioner derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}
		/* CONSOLE_DEBUG("Got %d derivatives from relation %d",count,i); */
		/* find the diagonal elements */
		for(j=0; j<count; ++j){
			if(var_deriv(variables[j])){
				mtx_fill_value(P, mtx_coord(&C, i, var_sindex(variables[j])), c_j * derivatives[j]);
			}else{
				mtx_fill_value(P, mtx_coord(&C, i, var_sindex(variables[j])), derivatives[j]);
			}
		}
	}

	mtx_assemble(P);

	if(status){
		CONSOLE_DEBUG("Error found when evaluating derivatives");
		res = 1; goto finish; /* recoverable */
	}

	integrator_ida_write_incidence(sys);

	res = 0;
finish:
	ASC_FREE(variables);
	ASC_FREE(derivatives);
	return res;
};

/**
	EXPERIMENTAL. Full Jacobian preconditioner for use with IDA Krylov solvers

	'solve' function.
*/
static int integrator_ida_psolve_jacobian(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *p_data,
		 N_Vector tmp
){
	IntegratorSystem *sys;
	IntegratorIdaData *data;
	IntegratorIdaPrecDataJacobian *precdata;
	sys = (IntegratorSystem *)p_data;
	data = sys->enginedata;
	precdata = (IntegratorIdaPrecDataJacobian *)(data->precdata);
	linsolqr_system_t L = precdata->L;

	linsolqr_add_rhs(L,NV_DATA_S(rvec),FALSE);

	mtx_region_t R;
	R.row.low = R.col.low = 0;
	R.row.high = R.col.high = mtx_order(linsolqr_get_matrix(L)) - 1;
    linsolqr_set_region(L,R);

    linsolqr_prep(L,linsolqr_fmethod_to_fclass(linsolqr_fmethod(L)));
    linsolqr_reorder(L, &R, linsolqr_rmethod(L));

	/// @TODO more here

	linsolqr_remove_rhs(L,NV_DATA_S(rvec));

	CONSOLE_DEBUG("Solving Jacobian preconditioner (c_j = %f)",c_j);
	return 0;
};


/*----------------------------------------------
  JACOBI PRECONDITIONER -- EXPERIMENTAL.
*/

static void integrator_ida_pcreate_jacobi(IntegratorSystem *sys){
	IntegratorIdaData *enginedata =sys->enginedata;
	IntegratorIdaPrecDataJacobi *precdata;
	precdata = ASC_NEW(IntegratorIdaPrecDataJacobi);

	asc_assert(sys->n_y);
	precdata->PIii = N_VNew_Serial(sys->n_y);

	enginedata->pfree = &integrator_ida_pfree_jacobi;
	enginedata->precdata = precdata;
	CONSOLE_DEBUG("Allocated memory for Jacobi preconditioner");
}

static void integrator_ida_pfree_jacobi(IntegratorIdaData *enginedata){
	if(enginedata->precdata){
		IntegratorIdaPrecDataJacobi *precdata = (IntegratorIdaPrecDataJacobi *)enginedata->precdata;
		N_VDestroy_Serial(precdata->PIii);

		ASC_FREE(precdata);
		enginedata->precdata = NULL;
		CONSOLE_DEBUG("Freed memory for Jacobi preconditioner");
	}
	enginedata->pfree = NULL;
}

/**
	EXPERIMENTAL. Jacobi preconditioner for use with IDA Krylov solvers

	'setup' function.
*/
static int integrator_ida_psetup_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *p_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
){
	int i, j, res;
	IntegratorSystem *sys;
	IntegratorIdaData *enginedata;
	IntegratorIdaPrecDataJacobi *precdata;
	struct rel_relation **relptr;

	sys = (IntegratorSystem *)p_data;
	enginedata = sys->enginedata;
	precdata = (IntegratorIdaPrecDataJacobi *)(enginedata->precdata);
	double *derivatives;
	struct var_variable **variables;
	int count, status;
	char *relname;

	CONSOLE_DEBUG("Setting up Jacobi preconditioner");

	variables = ASC_NEW_ARRAY(struct var_variable*, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/**
		@TODO FIXME here we are using the very inefficient and contorted approach
		of calculating the whole jacobian, then extracting just the diagonal elements.
	*/

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){

		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			relname = rel_make_name(sys->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating preconditioner derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}
		/* CONSOLE_DEBUG("Got %d derivatives from relation %d",count,i); */
		/* find the diagonal elements */
		for(j=0; j<count; ++j){
			if(var_sindex(variables[j])==i){
				if(var_deriv(variables[j])){
					NV_Ith_S(precdata->PIii, i) = 1./(c_j * derivatives[j]);
				}else{
					NV_Ith_S(precdata->PIii, i) = 1./derivatives[j];
				}

			}
		}
#ifdef PREC_DEBUG
		CONSOLE_DEBUG("PI[%d] = %f",i,NV_Ith_S(precdata->PIii,i));
#endif
	}

	if(status){
		CONSOLE_DEBUG("Error found when evaluating derivatives");
		res = 1; goto finish; /* recoverable */
	}

	integrator_ida_write_incidence(sys);

	res = 0;
finish:
	ASC_FREE(variables);
	ASC_FREE(derivatives);
	return res;
};

/**
	EXPERIMENTAL. Jacobi preconditioner for use with IDA Krylov solvers

	'solve' function.
*/
static int integrator_ida_psolve_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *p_data,
		 N_Vector tmp
){
	IntegratorSystem *sys;
	IntegratorIdaData *data;
	IntegratorIdaPrecDataJacobi *precdata;
	sys = (IntegratorSystem *)p_data;
	data = sys->enginedata;
	precdata = (IntegratorIdaPrecDataJacobi *)(data->precdata);

	CONSOLE_DEBUG("Solving Jacobi preconditioner (c_j = %f)",c_j);
	N_VProd(precdata->PIii, rvec, zvec);
	return 0;
};

/*----------------------------------------------
  STATS
*/

/**
	A simple wrapper to the IDAGetIntegratorStats function. Returns all the
	status in a struct instead of separately.

	@return IDA_SUCCESS on success.
*/
static int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s){

#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==2

	int res;

	/*
		There is an error in the documentation for this function in Sundials 2.2.
		According the the header file, the hinused stat is not provided.
	*/
	res = IDAGetIntegratorStats(ida_mem, &s->nsteps, &s->nrevals, &s->nlinsetups,
		&s->netfails, &s->qlast, &s->qcur, &s->hlast, &s->hcur,
		&s->tcur
	);

	/* get the missing statistic */
	IDAGetActualInitStep(ida_mem, &s->hinused);

	return res;
#else

	return IDAGetIntegratorStats(ida_mem, &s->nsteps, &s->nrevals, &s->nlinsetups
		,&s->netfails, &s->qlast, &s->qcur, &s->hinused
		,&s->hlast, &s->hcur, &s->tcur
	);

#endif
}

/**
	This routine just outputs the stats to the CONSOLE_DEBUG routine.

	@TODO provide a GUI way of stats reporting from IDA.
*/
static void integrator_ida_write_stats(IntegratorIdaStats *stats){
# define SL(N) CONSOLE_DEBUG("%s = %ld",#N,stats->N)
# define SI(N) CONSOLE_DEBUG("%s = %d",#N,stats->N)
# define SR(N) CONSOLE_DEBUG("%s = %f",#N,stats->N)
		SL(nsteps); SL(nrevals); SL(nlinsetups); SL(netfails);
		SI(qlast); SI(qcur);
		SR(hinused); SR(hlast); SR(hcur); SR(tcur);
# undef SL
# undef SI
# undef SR
}

/*------------------------------------------------------------------------------
  OUTPUT OF INTERNALS: JACOBIAN / INCIDENCE MATRIX / DEBUG INFO
*/

/**
	Here we construct the local transfer matrix. It's a bit of a naive
	approach; probably far more efficient ways can be worked out. But it will
	hopefully be a useful way to examine stability of some spatial
	discretisation schemes for PDAE systems.

	http://ascendserver.cheme.cmu.edu/wiki/index.php/IDA#Stability
*/
static int integrator_ida_transfer_matrix(const IntegratorSystem *sys, struct SystemJacobianStruct *J){
	int i=0, res;
	enum submat{II_GA=0, II_GD, II_FA, II_FD, II_FDP, II_NUM};

	const var_filter_t *matvf[II_NUM] = {
		&system_vfilter_algeb
		,&system_vfilter_diff
		,&system_vfilter_algeb
		,&system_vfilter_diff
		,&system_vfilter_deriv
	};

	const rel_filter_t *matrf[II_NUM] = {
		&system_rfilter_algeb
		,&system_rfilter_algeb
		,&system_rfilter_diff
		,&system_rfilter_diff
		,&system_rfilter_diff
	};

	struct SystemJacobianStruct D[II_NUM];

	for(i=0;i<II_NUM;++i){
		res = system_jacobian(sys->system, matrf[i], matvf[i], 1/*safe*/ ,&(D[i]));
	}

	/* compute inverses for matrices that need it */
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return 1;
}


/**
	Our task here is to write the matrices that IDA *should* be seeing. We
	are actually making calls to relman_diff in order to do that, so we're
	really going back to the variables in the actualy system and computing
	row by row what the values are. This should mean just a single call to
	each blackbox present in the system (if blackbox caching is working
	correctly).
*/
static int integrator_ida_write_matrix(const IntegratorSystem *sys, FILE *f, const char *type){
	/* IntegratorIdaData *enginedata; */
	struct SystemJacobianStruct J = {NULL,NULL,NULL,0,0};
	int status=1;
	mtx_region_t R;

	if(type==NULL)type = "dx'/dx";

	if(0==strcmp(type,"dg/dz")){
		CONSOLE_DEBUG("Calculating dg/dz...");
		status = system_jacobian(sys->system
			, &system_rfilter_algeb, &system_vfilter_algeb
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dg/dx")){
		CONSOLE_DEBUG("Calculating dg/dx...");
		status = system_jacobian(sys->system
			, &system_rfilter_algeb, &system_vfilter_diff
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"df/dx'")){
		CONSOLE_DEBUG("Calculating df/dx'...");
		status = system_jacobian(sys->system
			, &system_rfilter_diff, &system_vfilter_deriv
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"df/dz")){
		CONSOLE_DEBUG("Calculating df/dz...");
		status = system_jacobian(sys->system
			, &system_rfilter_diff, &system_vfilter_algeb
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"df/dx")){
		CONSOLE_DEBUG("Calculating df/dx...");
		status = system_jacobian(sys->system
			, &system_rfilter_diff, &system_vfilter_diff
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dF/dy")){
		CONSOLE_DEBUG("Calculating dF/dy...");
		status = system_jacobian(sys->system
			, &system_rfilter_all, &system_vfilter_nonderiv
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dF/dy'")){
		CONSOLE_DEBUG("Calculating dF/dy'...");
		status = system_jacobian(sys->system
			, &system_rfilter_all, &system_vfilter_deriv
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dx'/dx")){
		/* system state transfer matrix dyd'/dyd */
		status = integrator_ida_transfer_matrix(sys, &J);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid matrix type '%s'",type);
		return 1;
	}

	if(status){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error calculating matrix");
	}else{
		/* send the region explicitly, so that we handle non-square correctly */
		R.row.low = 0; R.col.low = 0;
		R.row.high = J.n_rels - 1; R.col.high = J.n_vars - 1;
		/* note that we're not fussy about empty matrices here... */
		mtx_write_region_mmio(f,J.M,&R);
	}

	if(J.vars)ASC_FREE(J.vars);
	if(J.rels)ASC_FREE(J.rels);
	if(J.M)mtx_destroy(J.M);

	return status;
}

/**
	This routine outputs matrix structure in a crude text format, for the sake
	of debugging.
*/
static void integrator_ida_write_incidence(IntegratorSystem *sys){
	int i, j;
	struct rel_relation **relptr;
	IntegratorIdaData *enginedata = sys->enginedata;
	double *derivatives;
	struct var_variable **variables;
	int count, status;
	char *relname;

	if(enginedata->nrels > 100){
		CONSOLE_DEBUG("Ignoring call (matrix size too big = %d)",enginedata->nrels);
		return;
	}

	variables = ASC_NEW_ARRAY(struct var_variable *, sys->n_y * 2);
	derivatives = ASC_NEW_ARRAY(double, sys->n_y * 2);

	CONSOLE_DEBUG("Outputting incidence information to console...");

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		relname = rel_make_name(sys->system, *relptr);

		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}

		fprintf(stderr,"%3d:%-15s:",i,relname);
		ASC_FREE(relname);

		for(j=0; j<count; ++j){
			if(var_deriv(variables[j])){
				fprintf(stderr," %p:ydot[%d]",variables[j],integrator_ida_diffindex(sys,variables[j]));
			}else{
				fprintf(stderr," %p:y[%d]",variables[j],var_sindex(variables[j]));
			}
		}
		fprintf(stderr,"\n");
	}
	ASC_FREE(variables);
	ASC_FREE(derivatives);
}

/* @return 0 on success */
int integrator_ida_debug(const IntegratorSystem *sys, FILE *fp){
	char *varname, *relname;
	struct var_variable **vlist, *var;
	struct rel_relation **rlist, *rel;
	long vlen, rlen;
	long i;
	long di;

	fprintf(fp,"THERE ARE %d VARIABLES IN THE INTEGRATION SYSTEM\n\n",sys->n_y);

	/* if(integrator_sort_obs_vars(sys))return 10; */

	if(sys->y && sys->ydot){
		fprintf(fp,"CONTENTS OF THE 'Y' AND 'YDOT' LISTS\n\n");
		fprintf(fp,"index\t%-15s\tydot\n","y");
		fprintf(fp,"-----\t%-15s\t-----\n","-----");
		for(i=0;i<sys->n_y;++i){
			varname = var_make_name(sys->system, sys->y[i]);
			fprintf(fp,"%ld\t%-15s\t",i,varname);
			if(sys->ydot[i]){
				ASC_FREE(varname);
				varname = var_make_name(sys->system, sys->ydot[i]);
				fprintf(fp,"%s\n",varname);
				ASC_FREE(varname);
			}else{
				fprintf(fp,".\n");
				ASC_FREE(varname);
			}
		}
	}else{
		fprintf(fp,"'Y' and 'YDOT' LISTS ARE NOT SET!\n");
	}

	fprintf(fp,"\n\nCONTENTS OF THE VAR_FLAGS AND VAR_SINDEX\n\n");
	fprintf(fp,"sindex\t%-15s\ty    \tydot \n","name");
	fprintf(fp,"------\t%-15s\t-----\t-----\n","----");


	/* visit all the slv_system_t master var lists to collect vars */
	/* find the vars mostly in this one */
	vlist = slv_get_solvers_var_list(sys->system);
	vlen = slv_get_num_solvers_vars(sys->system);
	for(i=0;i<vlen;i++){
		var = vlist[i];

		varname = var_make_name(sys->system, var);
		fprintf(fp,"%ld\t%-15s\t",i,varname);

		if(var_fixed(var)){
			// it's fixed, so not really a DAE var
			fprintf(fp,"(fixed)\n");
		}else if(!var_active(var)){
			// inactive
			fprintf(fp,"(inactive)\n");
		}else if(!var_incident(var)){
			// not incident
			fprintf(fp,"(not incident)\n");
		}else{
			if(var_deriv(var)){
				if(sys->y_id){
					di = integrator_ida_diffindex1(sys,var);
					if(di>=0){
						ASC_FREE(varname);
						varname = var_make_name(sys->system,vlist[di]);
						fprintf(fp,".\tdiff(%ld='%s')\n",di,varname);
					}else{
						fprintf(fp,".\tdiff(???,err=%ld)\n",di);
					}
				}else{
					fprintf(fp,".\tderiv... of??\n");
				}
			}else{
				fprintf(fp,"%d\t.\n",var_sindex(var));
			}
		}
		ASC_FREE(varname);
	}

	/* let's write out the relations too */
	rlist = slv_get_solvers_rel_list(sys->system);
	rlen = slv_get_num_solvers_rels(sys->system);

	fprintf(fp,"\nALL RELATIONS IN THE SOLVER'S LIST (%ld)\n\n",rlen);
	fprintf(fp,"index\tname\n");
	fprintf(fp,"-----\t----\n");
	for(i=0; i<rlen; ++i){
		rel = rlist[i];
		relname = rel_make_name(sys->system,rel);
		fprintf(fp,"%ld\t%s\n",i,relname);
		ASC_FREE(relname);
	}

	/* write out the derivative chains */
	fprintf(fp,"\nDERIVATIVE CHAINS\n");
	if(integrator_ida_analyse_debug(sys,stderr)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error getting diffvars debug info");
		return 340;
	}
	fprintf(fp,"\n");

	/* and lets write block debug output */
	system_block_debug(sys->system, fp);

	return 0; /* success */
}

/*----------------------------------------------
  ERROR REPORTING
*/
/**
	Error message reporter function to be passed to IDA. All error messages
	will trigger a call to this function, so we should find everything
	appearing on the console (in the case of Tcl/Tk) or in the errors/warnings
	panel (in the case of PyGTK).
*/
static void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
){
	IntegratorSystem *sys;
	error_severity_t sev;

	/* cast back the IntegratorSystem, just in case we need it */
	sys = (IntegratorSystem *)eh_data;

	/* severity depends on the sign of the error_code value */
	if(error_code <= 0){
		sev = ASC_PROG_ERR;
	}else{
		sev = ASC_PROG_WARNING;
	}

	/* use our all-purpose error reporting to get stuff back to the GUI */
	error_reporter(sev,module,0,function,"%s (error %d)",msg,error_code);
}
