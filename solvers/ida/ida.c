/*	ASCEND modelling environment
	Copyright (C) 2006-2011 Carnegie Mellon University

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
#include "idaprec.h"
#include "idacalc.h"
#include "idaio.h"

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

typedef void (IntegratorVarVisitorFn)(IntegratorSystem *integ, struct var_variable *var, const int *varindx);

/*static IntegratorVarVisitorFn integrator_dae_classify_var;
static void integrator_visit_system_vars(IntegratorSystem *integ,IntegratorVarVisitorFn *visitor);
static void integrator_dae_show_var(IntegratorSystem *integ, struct var_variable *var, const int *varindx); */

static int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s);

/*-------------------------------------------------------------
  SETUP/TEARDOWN ROUTINES
*/
static void integrator_ida_create(IntegratorSystem *integ){
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

	integ->enginedata = (void *)enginedata;

	integrator_ida_params_default(integ);
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

IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *integ){
	IntegratorIdaData *d;
	assert(integ!=NULL);
	assert(integ->enginedata!=NULL);
	assert(integ->engine==INTEG_IDA);
	d = ((IntegratorIdaData *)(integ->enginedata));
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
	etc. The values are stuck into the integ->params structure.

	To add a new parameter, first give it a name IDA_PARAM_* in thge above enum ida_parameters
	list. Then add a slv_param_*(...) statement below to define the type, description and range
	for the new parameter.

	@return 0 on success
*/
static int integrator_ida_params_default(IntegratorSystem *integ){
	asc_assert(integ!=NULL);
	asc_assert(integ->engine==INTEG_IDA);
	slv_parameters_t *p;
	p = &(integ->params);

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

typedef int IdaFlagFn(void *,int *);
typedef char *IdaFlagNameFn(int);

/* return 0 on success */
static int integrator_ida_solve(
		IntegratorSystem *integ
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

	enginedata = integrator_ida_enginedata(integ);

	enginedata->safeeval = SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_SAFEEVAL);
	CONSOLE_DEBUG("safeeval = %d",enginedata->safeeval);

	/* store reference to list of relations (in enginedata) */
	n_solverrels = slv_get_num_solvers_rels(integ->system);

	n_activerels = slv_count_solvers_rels(integ->system, &integrator_ida_rel);

	enginedata->bndlist = slv_get_solvers_bnd_list(integ->system);
	enginedata->nbnds = slv_get_num_solvers_bnds(integ->system);

	enginedata->rellist = ASC_NEW_ARRAY(struct rel_relation *, n_activerels);

	rels = slv_get_solvers_rel_list(integ->system);

	j=0;
	for(i=0; i < n_solverrels; ++i){
		if(rel_apply_filter(rels[i], &integrator_ida_rel)){
#ifdef SOLVE_DEBUG
			relname = rel_make_name(integ->system, rels[i]);
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
	CONSOLE_DEBUG("Number of dependent vars: %d",integ->n_y);
	CONSOLE_DEBUG("Number of boundaries: %d",enginedata->nbnds);

	enginedata->nrels = n_activerels;

	if(enginedata->nrels != integ->n_y){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Integration problem is not square (%d active rels, %d vars)"
			,n_activerels, integ->n_y
		);
		return 1; /* failure */
	}

#ifdef SOLVE_DEBUG
	integrator_ida_debug(integ,stderr);
#endif

	/* retrieve initial values from the system */

	/** @TODO fix this, the starting time != first sample */
	t0 = integrator_get_t(integ);
	CONSOLE_DEBUG("RETRIEVED t0 = %f",t0);

	CONSOLE_DEBUG("RETRIEVING y0");

	y0 = N_VNew_Serial(integ->n_y);
	integrator_get_y(integ,NV_DATA_S(y0));

#ifdef SOLVE_DEBUG
	CONSOLE_DEBUG("RETRIEVING yp0");
#endif

	yp0 = N_VNew_Serial(integ->n_y);
	integrator_get_ydot(integ,NV_DATA_S(yp0));

#ifdef SOLVE_DEBUG
	N_VPrint_Serial(yp0);
	CONSOLE_DEBUG("yp0 is at %p",&yp0);
#endif

	/* create IDA object */
	ida_mem = IDACreate();

	/* relative error tolerance */
	reltol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_RTOL);
	CONSOLE_DEBUG("rtol = %8.2e",reltol);


	/* allocate internal memory */
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
	flag = IDAInit(ida_mem, &integrator_ida_fex, t0, y0 ,yp0);
#else
	if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_ATOLVECT)){
		/* vector of absolute tolerances */
		CONSOLE_DEBUG("USING VECTOR OF ATOL VALUES");
		abstolvect = N_VNew_Serial(integ->n_y);
		integrator_get_atol(integ,NV_DATA_S(abstolvect));

		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, reltol, abstolvect);

		N_VDestroy_Serial(abstolvect);
	}else{
		/* scalar absolute tolerance (one value for all) */
		abstol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_ATOL);
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
	if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_ATOLVECT)){
		CONSOLE_DEBUG("using vector of atol values");
		abstolvect = N_VNew_Serial(integ->n_y);
		integrator_get_atol(integ,NV_DATA_S(abstolvect));
		IDASVtolerances(ida_mem, reltol, abstolvect);
		N_VDestroy_Serial(abstolvect);
	}else{
		/* scalar tolerances */
		abstol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_ATOL);
		CONSOLE_DEBUG("using scalar atol value = %8.2e",abstol);
		IDASStolerances(ida_mem, reltol, abstol);
	}
#endif

	/* set optional inputs... */
	IDASetErrHandlerFn(ida_mem, &integrator_ida_error, (void *)integ);
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
	IDASetUserData(ida_mem, (void *)integ);
#else
	IDASetRdata(ida_mem, (void *)integ);
#endif
	IDASetMaxStep(ida_mem, integrator_get_maxstep(integ));
	IDASetInitStep(ida_mem, integrator_get_stepzero(integ));
	IDASetMaxNumSteps(ida_mem, integrator_get_maxsubsteps(integ));
	if(integrator_get_minstep(integ)>0){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"IDA does not support minstep (ignored)\n");
	}

	CONSOLE_DEBUG("MAXNCF = %d",SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXNCF));
    IDASetMaxConvFails(ida_mem,SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXNCF));

	CONSOLE_DEBUG("MAXORD = %d",SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXORD));
    IDASetMaxOrd(ida_mem,SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXORD));

	/* there's no capability for setting *minimum* step size in IDA */


	/* attach linear solver module, using the default value of maxl */
	linsolver = SLV_PARAM_CHAR(&(integ->params),IDA_PARAM_LINSOLVER);
	CONSOLE_DEBUG("ASSIGNING LINEAR SOLVER '%s'",linsolver);
	if(strcmp(linsolver,"ASCEND")==0){
		CONSOLE_DEBUG("ASCEND DIRECT SOLVER, size = %d",integ->n_y);
		IDAASCEND(ida_mem,integ->n_y);
		IDAASCENDSetJacFn(ida_mem, &integrator_ida_sjex, (void *)integ);

		flagfntype = "IDAASCEND";
		flagfn = &IDAASCENDGetLastFlag;
		flagnamefn = &IDAASCENDGetReturnFlagName;

	}else if(strcmp(linsolver,"DENSE")==0){
		CONSOLE_DEBUG("DENSE DIRECT SOLVER, size = %d",integ->n_y);
		flag = IDADense(ida_mem, integ->n_y);
		switch(flag){
			case IDADENSE_SUCCESS: break;
			case IDADENSE_MEM_NULL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL"); return 5;
			case IDADENSE_ILL_INPUT: ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDADENSE is not compatible with current nvector module"); return 5;
			case IDADENSE_MEM_FAIL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"Memory allocation failed for IDADENSE"); return 5;
			default: ERROR_REPORTER_HERE(ASC_PROG_ERR,"bad return"); return 5;
		}

		if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
			flag = IDADlsSetDenseJacFn(ida_mem, &integrator_ida_djex);
#else
			flag = IDADenseSetJacFn(ida_mem, &integrator_ida_djex, (void *)integ);
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

		maxl = SLV_PARAM_INT(&(integ->params),IDA_PARAM_MAXL);
		CONSOLE_DEBUG("maxl = %d",maxl);

		/* what preconditioner? */
		pname = SLV_PARAM_CHAR(&(integ->params),IDA_PARAM_PREC);
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
			(prec->pcreate)(integ);
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
			IDASpilsSetPreconditioner(ida_mem,prec->psetup,prec->psolve);
#else
			IDASpilsSetPreconditioner(ida_mem,prec->psetup,prec->psolve,(void *)integ);
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
		if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
		    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex);
#else
		    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex, (void *)integ);
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
			if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_GSMODIFIED)){
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
	if(strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC),"Y")==0){
		CONSOLE_DEBUG("Solving initial conditions using values of yddot");
		icopt = IDA_Y_INIT;
		asc_assert(icopt!=0);
	}else if(strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC),"YA_YDP")==0){
		CONSOLE_DEBUG("Solving initial conditions using values of yd");
		icopt = IDA_YA_YDP_INIT;
		asc_assert(icopt!=0);
		id = N_VNew_Serial(integ->n_y);
		for(i=0; i < integ->n_y; ++i){
			if(integ->ydot[i] == NULL){
				NV_Ith_S(id,i) = 0.0;
#ifdef SOLVE_DEBUG
				varname = var_make_name(integ->system,integ->y[i]);
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
	}else if(strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC),"NONE")==0){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Not solving initial conditions: check current residuals");
	}else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid 'iccalc' value: check solver parameters.");
	}

	if(icopt){
		integ->currentstep=0;
	 	t_index=start_index + 1;
		tout1 = samplelist_get(integ->samples, t_index);

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
		IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn, (void *)integ);
#endif
	}

	/* -- set up the IntegratorReporter */
	integrator_output_init(integ);

	/* -- store the initial values of all the stuff */
	integrator_output_write(integ);
	integrator_output_write_obs(integ);

	/* specify where the returned values should be stored */
	yret = y0;
	ypret = yp0;

	/* advance solution in time, return values as yret and derivatives as ypret */
	integ->currentstep=1;
	for(t_index=start_index+1;t_index <= finish_index;++t_index, ++integ->currentstep){
		t = samplelist_get(integ->samples, t_index);
		t0 = integrator_get_t(integ);
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

		/* this seems to work, so we can use it to avoid needing 'havecrossed'? */
		if(flag == IDA_ROOT_RETURN){
			CONSOLE_DEBUG("IDA reports root found!");
		}

		/* so we will check for roots found explicitly */
		if(enginedata->nbnds){
			rootsfound = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);
			havecrossed = 0;
			if(IDA_SUCCESS == IDAGetRootInfo(ida_mem, rootsfound)){
				for(i=0; i < enginedata->nbnds; ++i){
					if(rootsfound[i]){
						havecrossed = 1;
#ifdef SOLVE_DEBUG
						relname = bnd_make_name(integ->system,enginedata->bndlist[i]);
						ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Boundary '%s' crossed%s",relname,rootsfound[i]>0?" (increasing)":" (decreasing)");
						ASC_FREE(relname);
#else
						ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Boundary %d crossed!%s", i, ,rootsfound[i]>0?" (increasing)":" (decreasing)");
#endif
					}
				}

				/* so, now we need to restart the integration. we will assume that
				everything changes: number of variables, etc, etc, etc. */

				if(havecrossed){
					CONSOLE_DEBUG("Boundaries were crossed; need to reinitialise solver...");
					/** try resetting the boundary states now? */
					//IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn, (void *)integ);

#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
					IDAReInit(ida_mem, tret, yret, ypret);
#elif SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR<3
					/* TODO find out what version needs the following line... not sure. */
					//IDAReInit(ida_mem, &integrator_ida_fex, tret, yret, ypret);

					// FIXME this stuff has not been tested yet, and is very incomplete.

					if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_ATOLVECT)){
						/* vector of absolute tolerances */
						CONSOLE_DEBUG("USING VECTOR OF ATOL VALUES");
						abstolvect = N_VNew_Serial(integ->n_y);
						integrator_get_atol(integ,NV_DATA_S(abstolvect));
						flag = IDAReInit(ida_mem, &integrator_ida_fex, tret, yret, ypret, IDA_SV, reltol, abstolvect);
						N_VDestroy_Serial(abstolvect);
					}else{
						/* scalar absolute tolerance (one value for all) */
						abstol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_ATOL);
						CONSOLE_DEBUG("USING SCALAR ATOL VALUE = %8.2e",abstol);
						flag = IDAReInit(ida_mem, &integrator_ida_fex, tret, yret, ypret, IDA_SS, reltol, &abstol);
					}
#else
					/* allocate internal memory */
					if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_ATOLVECT)){
						/* vector of absolute tolerances */
						abstolvect = N_VNew_Serial(integ->n_y);
						integrator_get_atol(integ,NV_DATA_S(abstolvect));
						if(IDA_SUCCESS != IDAReInit(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, reltol, abstolvect)){
							ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to reinitialise IDA");
						}
						N_VDestroy_Serial(abstolvect);
					}else{
						/* scalar absolute tolerance (one value for all) */
						abstol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_ATOL);
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
		integrator_set_t(integ, (double)tret);
		integrator_set_y(integ, NV_DATA_S(yret));
		integrator_set_ydot(integ, NV_DATA_S(ypret));

		if(flag<0){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve t = %f (IDASolve), error %d", t, flag);
			break;
		}

		/* -- do something so that integ knows the values of tret, yret and ypret */

		/* -- store the current values of all the stuff */
		integrator_output_write(integ);
		integrator_output_write_obs(integ);

	}/* loop through next sample timestep */

	/* -- close the IntegratorReporter */
	integrator_output_close(integ);

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

