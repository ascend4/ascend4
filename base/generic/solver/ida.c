/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
	@see http://www.llnl.gov/casc/sundials/
*//*
	by John Pye, May 2006
*/

/*
	Be careful with the following. This file requires both the 'ida.h' from
	SUNDIALS as well as the 'ida.h' from ASCEND. Make sure that we're getting
	both of these; if you get problems check your build tool for the paths being
	passed to the C preprocessor.
*/

/* standard includes */
#include <signal.h>

/* ASCEND includes */
#include "ida.h"
#include <utilities/error.h>
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascPanic.h>
#include <compiler/instance_enum.h>
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "linsol.h"
#include "linsolqr.h"
#include "slv_common.h"
#include "slv_client.h"
#include "relman.h"

/* SUNDIALS includes */
#ifdef ASC_WITH_IDA
# include <sundials/sundials_config.h>
# include <sundials/sundials_dense.h>
# include <ida/ida.h>
# include <nvector/nvector_serial.h>
# include <ida/ida_spgmr.h>
# include <ida/ida_spbcgs.h>
# include <ida/ida_sptfqmr.h>
# include <ida/ida_dense.h>
# ifndef IDA_SUCCESS
#  error "Failed to include SUNDIALS IDA header file"
# endif
#endif

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

/* check that we've got what we expect now */
#ifndef ASC_IDA_H
# error "Failed to include ASCEND IDA header file"
#endif

/* #define FEX_DEBUG */
/* #define JEX_DEBUG */
/* #define SOLVE_DEBUG */

const IntegratorInternals integrator_ida_internals = {
	integrator_ida_create
	,integrator_ida_params_default
	,integrator_analyse_dae /* note, this routine is back in integrator.c */
	,integrator_ida_solve
	,integrator_ida_free
	,INTEG_IDA
	,"IDA"
};

/**
	Struct containing any stuff that IDA needs that doesn't fit into the
	common IntegratorSystem struct.
*/
typedef struct{
	struct rel_relation **rellist;   /**< NULL terminated list of rels */
	struct var_variable **varlist;   /**< NULL terminated list of vars. ONLY USED FOR DEBUGGING -- get rid of it! */
	struct bnd_boundary **bndlist;	 /**< NULL-terminated list of boundaries, for use in the root-finding  code */
	int nrels;
	int safeeval;                    /**< whether to pass the 'safe' flag to relman_eval */
	N_Vector pp;                     /**< Preconditioner values */
} IntegratorIdaData;

/*-------------------------------------------------------------
  FORWARD DECLS
*/
/* residual function forward declaration */
int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data);

int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
);

/* error handler forward declaration */
void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
);

int integrator_ida_djex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, DenseMat Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
);

int integrator_ida_psetup(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *prec_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
);

int integrator_ida_psolve(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *prec_data,
		 N_Vector tmp
);

static const IDASpilsPrecSetupFn psetup1 = &integrator_ida_psetup;
static const IDASpilsPrecSolveFn psolve1 = &integrator_ida_psolve;

/*-------------------------------------------------------------
  SETUP/TEARDOWN ROUTINES
*/
void integrator_ida_create(IntegratorSystem *blsys){
	CONSOLE_DEBUG("ALLOCATING IDA ENGINE DATA");
	IntegratorIdaData *enginedata;
	enginedata = ASC_NEW(IntegratorIdaData);
	enginedata->rellist = NULL;
	enginedata->varlist = NULL;
	enginedata->safeeval = 0;
	enginedata->pp = NULL;
	blsys->enginedata = (void *)enginedata;
	integrator_ida_params_default(blsys);
}

void integrator_ida_free(void *enginedata){
	CONSOLE_DEBUG("DELETING IDA ENGINE DATA");
	IntegratorIdaData *d = (IntegratorIdaData *)enginedata;
	if(d->pp)N_VDestroy_Serial(d->pp);
	/* note, we don't own the rellist, so don't need to free it */
	ASC_FREE(d);
}

IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *blsys){
	IntegratorIdaData *d;
	assert(blsys!=NULL);
	assert(blsys->enginedata!=NULL);
	assert(blsys->engine==INTEG_IDA);
	d = ((IntegratorIdaData *)(blsys->enginedata));
	return d;
}

/*-------------------------------------------------------------
  PARAMETERS FOR IDA
*/

enum ida_parameters{
	IDA_PARAM_LINSOLVER
	,IDA_PARAM_MAXL
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
	etc. The values are stuck into the blsys->params structure.

	To add a new parameter, first give it a name IDA_PARAM_* in thge above enum ida_parameters
	list. Then add a slv_param_*(...) statement below to define the type, description and range
	for the new parameter.

	@return 0 on success
*/
int integrator_ida_params_default(IntegratorSystem *blsys){
	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_IDA);
	slv_parameters_t *p;
	p = &(blsys->params);

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

	slv_param_bool(p,IDA_PARAM_CALCIC
			,(SlvParameterInitBool){{"calcic"
			,"Calculate initial conditions?",1
			,"Use IDA to calculate initial conditions (1) or else assume that the model will already be solved for this case (0)"
		}, FALSE}
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
			" See IDA manual, section 5.5.1"
		}, 1e-5, DBL_MIN, DBL_MAX }
	);

	slv_param_real(p,IDA_PARAM_RTOL
			,(SlvParameterInitReal){{"rtol"
			,"Scalar relative error tolerance",1
			,"Value of the scalar relative error tolerance. (Note that for IDA,"
			" it's not possible to set per-variable relative tolerances as it is"
			" with LSODE)."
			" See IDA manual, section 5.5.1"
		}, 1e-4, 0, DBL_MAX }
	);

	slv_param_char(p,IDA_PARAM_LINSOLVER
			,(SlvParameterInitChar){{"linsolver"
			,"Linear solver",1
			,"See IDA manual, section 5.5.3."
		}, "SPGMR"}, (char *[]){"DENSE","BAND","SPGMR","SPBCG","SPTFQMR",NULL}
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

/* return 1 on success */
int integrator_ida_solve(
		IntegratorSystem *blsys
		, unsigned long start_index
		, unsigned long finish_index
){
	void *ida_mem;
	int size, flag, flag1, t_index;
	realtype t0, reltol, abstol, t, tret, tout1;
	N_Vector y0, yp0, abstolvect, ypret, yret;
	IntegratorIdaData *enginedata;
	char *linsolver;
	int maxl;
	IdaFlagFn *flagfn;
	IdaFlagNameFn *flagnamefn;
	const char *flagfntype;
	char *pname;
	IDASpilsPrecSetupFn psetup;
	IDASpilsPrecSolveFn psolve;

	CONSOLE_DEBUG("STARTING IDA...");

	enginedata = integrator_ida_enginedata(blsys);

	enginedata->safeeval = SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_SAFEEVAL);
	CONSOLE_DEBUG("safeeval = %d",enginedata->safeeval);

	/* store reference to list of relations (in enginedata) */
	enginedata->nrels = slv_get_num_solvers_rels(blsys->system);
	enginedata->rellist = slv_get_solvers_rel_list(blsys->system);
	enginedata->varlist = slv_get_solvers_var_list(blsys->system);
	enginedata->bndlist = slv_get_solvers_bnd_list(blsys->system);

	CONSOLE_DEBUG("Number of relations: %d",enginedata->nrels);
	CONSOLE_DEBUG("Number of dependent vars: %ld",blsys->n_y);
	size = blsys->n_y;

	if(enginedata->nrels!=size){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration problem is not square (%d rels, %d vars)", enginedata->nrels, size);
		return 0; /* failure */
	}

	/* retrieve initial values from the system */

	/** @TODO fix this, the starting time != first sample */
	t0 = integrator_get_t(blsys);
	CONSOLE_DEBUG("RETRIEVED t0 = %f",t0);

	CONSOLE_DEBUG("RETRIEVING y0");

	y0 = N_VNew_Serial(size);
	integrator_get_y(blsys,NV_DATA_S(y0));

#ifdef SOLVE_DEBUG
	CONSOLE_DEBUG("RETRIEVING yp0");
#endif

	yp0 = N_VNew_Serial(size);
	integrator_get_ydot(blsys,NV_DATA_S(yp0));

#ifdef SOLVE_DEBUG
	N_VPrint_Serial(yp0);
	CONSOLE_DEBUG("yp0 is at %p",&yp0);
#endif

	/* create IDA object */
	ida_mem = IDACreate();

	/* relative error tolerance */
	reltol = SLV_PARAM_REAL(&(blsys->params),IDA_PARAM_RTOL);
	CONSOLE_DEBUG("rtol = %8.2e",reltol);

	/* allocate internal memory */
	if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_ATOLVECT)){
		/* vector of absolute tolerances */
		CONSOLE_DEBUG("USING VECTOR OF ATOL VALUES");
		abstolvect = N_VNew_Serial(size);
		integrator_get_atol(blsys,NV_DATA_S(abstolvect));

		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SV, reltol, abstolvect);

		N_VDestroy_Serial(abstolvect);
	}else{
		/* scalar absolute tolerance (one value for all) */
		CONSOLE_DEBUG("USING SCALAR ATOL VALUE = %8.2e",abstol);
		abstol = SLV_PARAM_REAL(&(blsys->params),IDA_PARAM_ATOL);
		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SS, reltol, &abstol);
	}

	if(flag==IDA_MEM_NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
		return 0;
	}else if(flag==IDA_MEM_FAIL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDAMalloc)");
		return 0;
	}else if(flag==IDA_ILL_INPUT){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid input to IDAMalloc");
		return 0;
	}/* else success */

	/* set optional inputs... */
	IDASetErrHandlerFn(ida_mem, &integrator_ida_error, (void *)blsys);
	IDASetRdata(ida_mem, (void *)blsys);
	IDASetMaxStep(ida_mem, integrator_get_maxstep(blsys));
	IDASetInitStep(ida_mem, integrator_get_stepzero(blsys));
	IDASetMaxNumSteps(ida_mem, integrator_get_maxsubsteps(blsys));
	if(integrator_get_minstep(blsys)>0){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"IDA does not support minstep (ignored)\n");
	}

	CONSOLE_DEBUG("MAXNCF = %d",SLV_PARAM_INT(&blsys->params,IDA_PARAM_MAXNCF));
    IDASetMaxConvFails(ida_mem,SLV_PARAM_INT(&blsys->params,IDA_PARAM_MAXNCF));

	/* there's no capability for setting *minimum* step size in IDA */


	/* attach linear solver module, using the default value of maxl */
	linsolver = SLV_PARAM_CHAR(&(blsys->params),IDA_PARAM_LINSOLVER);
	CONSOLE_DEBUG("ASSIGNING LINEAR SOLVER '%s'",linsolver);
	if(strcmp(linsolver,"DENSE")==0){
		CONSOLE_DEBUG("DENSE DIRECT SOLVER, size = %d",size);
		flag = IDADense(ida_mem, size);
		switch(flag){
			case IDADENSE_SUCCESS: break;
			case IDADENSE_MEM_NULL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL"); return 0;
			case IDADENSE_ILL_INPUT: ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDADENSE is not compatible with current nvector module"); return 0;
			case IDADENSE_MEM_FAIL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"Memory allocation failed for IDADENSE"); return 0;
			default: ERROR_REPORTER_HERE(ASC_PROG_ERR,"bad return"); return 0;
		}

		if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
			flag = IDADenseSetJacFn(ida_mem, &integrator_ida_djex, (void *)blsys);
			switch(flag){
				case IDADENSE_SUCCESS: break;
				default: ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed IDADenseSetJacFn"); return 0;
			}
		}else{
			CONSOLE_DEBUG("USING NUMERICAL DIFF");
		}

		flagfntype = "IDADENSE";
		flagfn = &IDADenseGetLastFlag;
		flagnamefn = &IDADenseGetReturnFlagName;
	}else{
		/* remaining methods are all SPILS */
		CONSOLE_DEBUG("IDA SPILS");

		maxl = SLV_PARAM_INT(&(blsys->params),IDA_PARAM_MAXL);
		CONSOLE_DEBUG("maxl = %d",maxl);

		pname = SLV_PARAM_CHAR(&(blsys->params),IDA_PARAM_PREC);
		if(strcmp(pname,"NONE")==0){
			psetup=NULL;
			psolve=NULL;
		}else if(strcmp(pname,"DIAG")==0){
			psetup=&integrator_ida_psetup;
			psolve=&integrator_ida_psolve;
		}

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
			return 0;
		}

		enginedata->pp = N_VNew_Serial(blsys->n_y);
		IDASpilsSetPreconditioner(ida_mem,psetup,psolve,(void *)blsys);
		CONSOLE_DEBUG("PRECONDITIONER = %s",pname);

		flagfntype = "IDASPILS";
		flagfn = &IDASpilsGetLastFlag;
		flagnamefn = &IDASpilsGetReturnFlagName;

		if(flag==IDASPILS_MEM_NULL){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
			return 0;
		}else if(flag==IDASPILS_MEM_FAIL){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDASpgmr)");
			return 0;
		}/* else success */

		/* assign the J*v function */
		if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
		    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex, (void *)blsys);
			if(flag==IDASPILS_MEM_NULL){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
				return 0;
			}else if(flag==IDASPILS_LMEM_NULL){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDASPILS linear solver has not been initialized");
				return 0;
			}/* else success */
		}else{
			CONSOLE_DEBUG("USING NUMERICAL DIFF");
		}

		if(strcmp(linsolver,"SPGMR")==0){
			/* select Gram-Schmidt orthogonalisation */
			if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_GSMODIFIED)){
				CONSOLE_DEBUG("USING MODIFIED GS");
				flag = IDASpilsSetGSType(ida_mem,MODIFIED_GS);
				if(flag!=IDASPILS_SUCCESS){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 0;
				}
			}else{
				CONSOLE_DEBUG("USING CLASSICAL GS");
				flag = IDASpilsSetGSType(ida_mem,CLASSICAL_GS);
				if(flag!=IDASPILS_SUCCESS){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 0;
				}
			}
		}
	}

	/* set linear solver optional inputs...
		...nothing here at the moment...
	*/

	/* calculate initial conditions */
	if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_CALCIC)){
		CONSOLE_DEBUG("Solving initial conditions (given derivatives)");

		blsys->currentstep=0;
	 	t_index=start_index;
		tout1 = samplelist_get(blsys->samples, t_index);

		CONSOLE_DEBUG("SOLVING INITIAL CONDITIONS IDACalcIC (tout1 = %f)", tout1);

		/* correct initial values, given derivatives */
	# if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==3
		/* note the new API from version 2.3 and onwards */
		flag = IDACalcIC(ida_mem, IDA_Y_INIT, tout1);
	# else
		flag = IDACalcIC(ida_mem, t0, y0, yp0, IDA_Y_INIT, tout1);
	# endif

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
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to retrieve error code from %s (err %d)",flagfntype,flag);
					return 0;
				}
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s returned flag '%s' (value = %d)",flagfntype,(flagnamefn)(flag1),flag1);
				return 0;

			default:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve initial condition (IDACalcIC)");
				return 0;
		}
	}else{
		CONSOLE_DEBUG("Not solving initial conditions (see IDA parameter 'calcic')");
	}

	/* optionally, specify ROO-FINDING PROBLEM */

	/* -- set up the IntegratorReporter */
	integrator_output_init(blsys);

	/* -- store the initial values of all the stuff */
	integrator_output_write(blsys);
	integrator_output_write_obs(blsys);

	/* specify where the returned values should be stored */
	yret = y0;
	ypret = yp0;

	/* advance solution in time, return values as yret and derivatives as ypret */
	blsys->currentstep=1;
	for(t_index=start_index+1;t_index <= finish_index;++t_index, ++blsys->currentstep){
		t = samplelist_get(blsys->samples, t_index);
		t0 = integrator_get_t(blsys);
		asc_assert(t > t0);

#ifdef SOLVE_DEBUG
		CONSOLE_DEBUG("Integratoring from t0 = %f to t = %f", t0, t);
#endif

		flag = IDASolve(ida_mem, t, &tret, yret, ypret, IDA_NORMAL);

		/* pass the values of everything back to the compiler */
		integrator_set_t(blsys, (double)tret);
		integrator_set_y(blsys, NV_DATA_S(yret));
		integrator_set_ydot(blsys, NV_DATA_S(ypret));

		if(flag<0){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve t = %f (IDASolve), error %d", t, flag);
			break;
		}

		/* -- do something so that blsys knows the values of tret, yret and ypret */

		/* -- store the current values of all the stuff */
		integrator_output_write(blsys);
		integrator_output_write_obs(blsys);
	}

	/* -- close the IntegratorReporter */
	integrator_output_close(blsys);

	if(flag < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Solving aborted while attempting t = %f", t);
		return 0;
	}

	/* get optional outputs */

	/* free solution memory */
	N_VDestroy_Serial(yret);
	N_VDestroy_Serial(ypret);

	/* free solver memory */
	IDAFree(ida_mem);

	/* all done */
	return 1;
}

/*--------------------------------------------------
  RESIDUALS AND JACOBIAN
*/
/**
	Function to evaluate system residuals, in the form required for IDA.

	Given tt, yy and yp, we need to evaluate and return rr.

	@param tt current value of indep variable (time)
	@param yy current values of dependent variable vector
	@param yp current values of derivatives of dependent variables
	@param rr the output residual vector (is we're returning data to)
	@param res_data pointer to our stuff (blsys in this case).

	@return 0 on success, positive on recoverable error, and
		negative on unrecoverable error.
*/
int integrator_ida_fex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr, void *res_data){
	IntegratorSystem *blsys;
	IntegratorIdaData *enginedata;
	int i, calc_ok, is_error;
	struct rel_relation** relptr;
	double resid;
	char *relname;
#ifdef FEX_DEBUG
	char *varname;
	char diffname[30];
#endif

	blsys = (IntegratorSystem *)res_data;
	enginedata = integrator_ida_enginedata(blsys);

#ifdef FEX_DEBUG
	/* fprintf(stderr,"\n\n"); */
	CONSOLE_DEBUG("EVALUTE RESIDUALS...");
#endif

	/* pass the values of everything back to the compiler */
	integrator_set_t(blsys, (double)tt);
	integrator_set_y(blsys, NV_DATA_S(yy));
	integrator_set_ydot(blsys, NV_DATA_S(yp));

	if(NV_LENGTH_S(rr)!=enginedata->nrels){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid residuals nrels!=length(rr)");
		return -1; /* unrecoverable */
	}

	/**
		@TODO does this function (fex) do bounds checking already?
	*/

	/* evaluate each residual in the rellist */
	is_error = 0;
	relptr = enginedata->rellist;

	if(enginedata->safeeval){
		Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
	}else{
#ifdef FEX_DEBUG
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SETTING TO CATCH SIGFPE...");
#endif
		Asc_SignalHandlerPushDefault(SIGFPE);
	}

	if (setjmp(g_fpe_env)==0) {
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			resid = relman_eval(*relptr, &calc_ok, enginedata->safeeval);

			NV_Ith_S(rr,i) = resid;
			if(!calc_ok){
				relname = rel_make_name(blsys->system, *relptr);
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Calculation error in rel '%s'",relname);
				ASC_FREE(relname);
				/* presumable some output already made? */
				is_error = 1;
			}/*else{
				CONSOLE_DEBUG("Calc OK");
			}*/
		}
	}else{
		relname = rel_make_name(blsys->system, *relptr);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error (SIGFPE) in rel '%s'",relname);
		ASC_FREE(relname);
		is_error = 1;
	}

	if(enginedata->safeeval){
		Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
	}else{
		Asc_SignalHandlerPopDefault(SIGFPE);
	}

#ifdef FEX_DEBUG
	/* output residuals to console */
	CONSOLE_DEBUG("RESIDUAL OUTPUT");
	fprintf(stderr,"index\t%25s\t%25s\t%s\n","y","ydot","resid");
	for(i=0; i<blsys->n_y; ++i){
		varname = var_make_name(blsys->system,blsys->y[i]);
		fprintf(stderr,"%d\t%15s=%10f\t",i,varname,NV_Ith_S(yy,i));
		if(blsys->ydot[i]){
			varname = var_make_name(blsys->system,blsys->ydot[i]);
			fprintf(stderr,"%15s=%10f\t",varname,NV_Ith_S(yp,i));
		}else{
			snprintf(diffname,99,"diff(%s)",varname);
			fprintf(stderr,"%15s=%10f\t",diffname,NV_Ith_S(yp,i));
		}
		ASC_FREE(varname);
		relname = rel_make_name(blsys->system,enginedata->rellist[i]);
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
*/
int integrator_ida_djex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, DenseMat Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
	IntegratorSystem *blsys;
	IntegratorIdaData *enginedata;
	char *relname;
#ifdef JEX_DEBUG
	char *varname;
#endif
	int status;
	struct rel_relation **relptr;
	int i;
	var_filter_t filter = {VAR_SVAR | VAR_FIXED, VAR_SVAR};
	double *derivatives;
	int *variables;
	int count, j;
	long var_yindex;

	blsys = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(blsys);

	/* allocate space for returns from relman_diff2: we *should* be able to use 'tmp1' and 'tmp2' here... */
	variables = ASC_NEW_ARRAY(int, NV_LENGTH_S(yy) * 2);
	derivatives = ASC_NEW_ARRAY(double, NV_LENGTH_S(yy) * 2);

	/* pass the values of everything back to the compiler */
	integrator_set_t(blsys, (double)tt);
	integrator_set_y(blsys, NV_DATA_S(yy));
	integrator_set_ydot(blsys, NV_DATA_S(yp));

#ifdef JEX_DEBUG
	/* print vars */
	for(i=0; i < blsys->n_y; ++i){
		varname = var_make_name(blsys->system, blsys->y[i]);
		CONSOLE_DEBUG("%s = %f = %f",varname,NV_Ith_S(yy,i),var_value(blsys->y[i]));
		ASC_FREE(varname);
	}

	/* print derivatives */
	for(i=0; i < blsys->n_y; ++i){
		if(blsys->ydot[i]){
			varname = var_make_name(blsys->system, blsys->ydot[i]);
			CONSOLE_DEBUG("%s = %f =%f",varname,NV_Ith_S(yp,i),var_value(blsys->ydot[i]));
			ASC_FREE(varname);
		}else{
			varname = var_make_name(blsys->system, blsys->y[i]);
			CONSOLE_DEBUG("diff(%s) = %f",varname,NV_Ith_S(yp,i));
			ASC_FREE(varname);
		}
	}

	/* print step size */
	CONSOLE_DEBUG("<c_j> = %f",c_j);
#endif

	/* build up the dense jacobian matrix... */
	status = 0;
	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		/* get derivatives for this particular relation */
		status = relman_diff2(*relptr, &filter, derivatives, variables, &count, enginedata->safeeval);

		if(status){
			relname = rel_make_name(blsys->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}

		/* output what's going on here ... */
#ifdef JEX_DEBUG
		relname = rel_make_name(blsys->system, *relptr);
		CONSOLE_DEBUG("RELATION %d '%s'",i,relname);
		fprintf(stderr,"%d: '%s': ",i,relname);
		ASC_FREE(relname);
		for(j=0;j<count;++j){
			varname = var_make_name(blsys->system, enginedata->varlist[variables[j]]);
			var_yindex = blsys->y_id[variables[j]];
			if(var_yindex >=0){
				fprintf(stderr,"  var[%d]='%s'=y[%ld]",variables[j],varname,var_yindex);
			}else{
				fprintf(stderr,"  var[%d]='%s'=ydot[%ld]",variables[j],varname,-var_yindex-1);
			}
			ASC_FREE(varname);
		}
		fprintf(stderr,"\n");
#endif
		/* insert values into the Jacobian row in appropriate spots (can assume Jac starts with zeros -- IDA manual) */
		for(j=0; j < count; ++j){
			var_yindex = blsys->y_id[variables[j]];
#ifndef __WIN32__
			/* the SUNDIALS headers seem not to store 'N' on Windows */
			ASC_ASSERT_RANGE(var_yindex, -Jac->N, Jac->N);
#endif
			if(var_yindex >= 0){
				asc_assert(blsys->y[var_yindex]==enginedata->varlist[variables[j]]);
				DENSE_ELEM(Jac,i,var_yindex) += derivatives[j];
			}else{
				asc_assert(blsys->ydot[-var_yindex-1]==enginedata->varlist[variables[j]]);
				DENSE_ELEM(Jac,i,-var_yindex-1) += derivatives[j] * c_j;
			}
		}
	}

#ifdef JEX_DEBUG
	CONSOLE_DEBUG("PRINTING JAC");
	fprintf(stderr,"\t");
	for(j=0; j < blsys->n_y; ++j){
		if(j)fprintf(stderr,"\t");
		varname = var_make_name(blsys->system,blsys->y[j]);
		fprintf(stderr,"%11s",varname);
		ASC_FREE(varname);
	}
	fprintf(stderr,"\n");
	for(i=0; i < enginedata->nrels; ++i){
		relname = rel_make_name(blsys->system, enginedata->rellist[i]);
		fprintf(stderr,"%s\t",relname);
		ASC_FREE(relname);

		for(j=0; j < blsys->n_y; ++j){
			if(j)fprintf(stderr,"\t");
			fprintf(stderr,"%11.2e",DENSE_ELEM(Jac,i,j));
		}
		fprintf(stderr,"\n");
	}
#endif

	if(status){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"There were derivative evaluation errors in the dense jacobian");
		return 1;
	}

#ifdef FEX_DEBUG
	CONSOLE_DEBUG("DJEX RETURNING 0");
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
	@param jac_data pointer to our stuff (blsys in this case, passed into IDA via IDASp*SetJacTimesVecFn.)
	@param tmp1 @see tmp2
	@param tmp2 (as well as tmp1) pointers to memory allocated for variables of type N_Vector for use here as temporary storage or work space.
	@return 0 on success
*/
int integrator_ida_jvex(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr
		, N_Vector v, N_Vector Jv, realtype c_j
		, void *jac_data, N_Vector tmp1, N_Vector tmp2
){
	IntegratorSystem *blsys;
	IntegratorIdaData *enginedata;
	int i, j, is_error=0;
	struct rel_relation** relptr;
	char *relname;
	int status;
	double Jv_i;
	long var_yindex;

	int *variables;
	double *derivatives;
	var_filter_t filter;
	int count;
#ifdef JEX_DEBUG
	CONSOLE_DEBUG("EVALUATING JACOBIAN...");
#endif

	blsys = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(blsys);

	/* pass the values of everything back to the compiler */
	integrator_set_t(blsys, (double)tt);
	integrator_set_y(blsys, NV_DATA_S(yy));
	integrator_set_ydot(blsys, NV_DATA_S(yp));
	/* no real use for residuals (rr) here, I don't think? */

	/* allocate space for returns from relman_diff2: we *should* be able to use 'tmp1' and 'tmp2' here... */

	i = NV_LENGTH_S(yy) * 2;
#ifdef JEX_DEBUG
	CONSOLE_DEBUG("Allocating 'variables' with length %d",i);
#endif
	variables = ASC_NEW_ARRAY(int, i);
	derivatives = ASC_NEW_ARRAY(double, i);

	/* evaluate the derivatives... */
	/* J = dG_dy = dF_dy + alpha * dF_dyp */

	filter.matchbits = VAR_SVAR | VAR_FIXED;
	filter.matchvalue = VAR_SVAR;

	Asc_SignalHandlerPushDefault(SIGFPE);
	if (setjmp(g_fpe_env)==0) {
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			/* get derivatives for this particular relation */
			status = relman_diff2(*relptr, &filter, derivatives, variables, &count, enginedata->safeeval);
#ifdef JEX_DEBUG
			CONSOLE_DEBUG("Got derivatives against %d matching variables, status = %d", count,status);
#endif

			if(status){
				relname = rel_make_name(blsys->system, *relptr);
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
				/* CONSOLE_DEBUG("j = %d, variables[j] = %d, n_y = %ld", j, variables[j], blsys->n_y);
				varname = var_make_name(blsys->system, enginedata->varlist[variables[j]]);
				if(varname){
					CONSOLE_DEBUG("Variable %d '%s' derivative = %f", variables[j],varname,derivatives[j]);
					ASC_FREE(varname);
				}else{
					CONSOLE_DEBUG("Variable %d (UNKNOWN!): derivative = %f",variables[j],derivatives[j]);
				}
				*/

				/* we don't calculate derivatives wrt indep var */
				asc_assert(variables[j]>=0);
				if(enginedata->varlist[variables[j]] == blsys->x) continue;

				var_yindex = blsys->y_id[variables[j]];
#ifdef JEX_DEBUG
				CONSOLE_DEBUG("j = %d: variables[j] = %d, y_id = %ld",j,variables[j],var_yindex);
#endif

				ASC_ASSERT_RANGE(-var_yindex-1, -NV_LENGTH_S(v),NV_LENGTH_S(v));

				if(var_yindex >= 0){
#ifdef JEX_DEBUG
					asc_assert(blsys->y[var_yindex]==enginedata->varlist[variables[j]]);
					fprintf(stderr,"Jv[%d] += %f (dF[%d]/dy[%ld] = %f, v[%ld] = %f)\n", i
						, derivatives[j] * NV_Ith_S(v,var_yindex)
						, i, var_yindex, derivatives[j]
						, var_yindex, NV_Ith_S(v,var_yindex)
					);
#endif
					Jv_i += derivatives[j] * NV_Ith_S(v,var_yindex);
				}else{
#ifdef JEX_DEBUG
					fprintf(stderr,"Jv[%d] += %f (dF[%d]/dydot[%ld] = %f, v[%ld] = %f)\n", i
						, derivatives[j] * NV_Ith_S(v,-var_yindex-1)
						, i, -var_yindex-1, derivatives[j]
						, -var_yindex-1, NV_Ith_S(v,-var_yindex-1)
					);
#endif
					asc_assert(blsys->ydot[-var_yindex-1]==enginedata->varlist[variables[j]]);
					Jv_i += derivatives[j] * NV_Ith_S(v,-var_yindex-1) * c_j;
				}
			}

			NV_Ith_S(Jv,i) = Jv_i;
#ifdef JEX_DEBUG
			CONSOLE_DEBUG("rel = %p",*relptr);
			relname = rel_make_name(blsys->system, *relptr);
			CONSOLE_DEBUG("'%s': Jv[%d] = %f", relname, i, NV_Ith_S(Jv,i));
			//ASC_FREE(relname);
			return 1;
#endif
		}
	}else{
		relname = rel_make_name(blsys->system, *relptr);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error (SIGFPE) in rel '%s'",relname);
		ASC_FREE(relname);
		is_error = 1;
	}
	Asc_SignalHandlerPopDefault(SIGFPE);

	if(is_error){
		CONSOLE_DEBUG("SOME ERRORS FOUND IN EVALUATION");
		return 1;
	}
	return 0;
}

/*----------------------------------------------
  PRECONDITIONER
*/


/**
	EXPERIMENTAL. A diagonal preconditioner for use with IDA Krylov solvers

	'setup' function.
*/
int integrator_ida_psetup(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *p_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
){
	int i;
	IntegratorSystem *blsys;
	IntegratorIdaData *data;
	blsys = (IntegratorSystem *)p_data;
	data = blsys->enginedata;

	double y = 1; /* derivative of y[i] in relation i */
	double ydot = 1; /* derivative of ydot[i] in relation i */

	N_VConst(1, data->pp);
	for(i=0; i<blsys->n_y; ++i){
		/* @TODO calculate y, ydot here */
		NV_Ith_S(data->pp, i) = 1 / (y + c_j * ydot);
	}
	return 0;
};

/**
	EXPERIMENTAL. A diagonal preconditioner for use with IDA Krylov solvers

	'solve' function.
*/
int integrator_ida_psolve(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *p_data,
		 N_Vector tmp
){
	IntegratorSystem *blsys;
	IntegratorIdaData *data;
	blsys = (IntegratorSystem *)p_data;
	data = blsys->enginedata;

	N_VProd(data->pp, rvec, zvec);
	return 0;
};

/*----------------------------------------------
  ERROR REPORTING
*/
/**
	Error message reporter function to be passed to IDA. All error messages
	will trigger a call to this function, so we should find everything
	appearing on the console (in the case of Tcl/Tk) or in the errors/warnings
	panel (in the case of PyGTK).
*/
void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
){
	IntegratorSystem *blsys;
	error_severity_t sev;

	/* cast back the IntegratorSystem, just in case we need it */
	blsys = (IntegratorSystem *)eh_data;

	/* severity depends on the sign of the error_code value */
	if(error_code <= 0){
		sev = ASC_PROG_ERR;
	}else{
		sev = ASC_PROG_WARNING;
	}

	/* use our all-purpose error reporting to get stuff back to the GUI */
	error_reporter(sev,module,0,function,"%s (error %d)",msg,error_code);
}
