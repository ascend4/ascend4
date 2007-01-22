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

#include "ida.h"

#include <signal.h>

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
#else
# error "Where is IDA?"
#endif

#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <utilities/ascSignal.h>
#include <utilities/ascPanic.h>
#include <compiler/instance_enum.h>

#include <solver/slv_client.h>
#include <solver/relman.h>
#ifdef ASC_IDA_NEW_ANALYSE
# include <solver/diffvars.h>
# include <solver/slv_stdcalls.h>
#endif

#include "idalinear.h"

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
#define JEX_DEBUG
#define SOLVE_DEBUG
#define STATS_DEBUG
#define PREC_DEBUG

/**
	Everthing that the outside world needs to know about IDA
*/
const IntegratorInternals integrator_ida_internals = {
	integrator_ida_create
	,integrator_ida_params_default
#ifdef ASC_IDA_NEW_ANALYSE
	,integrator_ida_analyse
#else
	,integrator_analyse_dae /* note, this routine is back in integrator.c */
#endif
	,integrator_ida_solve
	,NULL /* writematrixfn */
	,integrator_ida_debug
	,integrator_ida_free
	,INTEG_IDA
	,"IDA"
};

/*-------------------------------------------------------------
  FORWARD DECLS
*/

/* forward dec needed for IntegratorIdaPrecFreeFn */
struct IntegratorIdaDataStruct;

/* functions for allocating storage for and freeing preconditioner data */
typedef void IntegratorIdaPrecCreateFn(IntegratorSystem *blsys);
typedef void IntegratorIdaPrecFreeFn(struct IntegratorIdaDataStruct *enginedata);

/**
	Struct containing any stuff that IDA needs that doesn't fit into the
	common IntegratorSystem struct.
*/
typedef struct IntegratorIdaDataStruct{
	struct rel_relation **rellist;   /**< NULL terminated list of rels */
	struct bnd_boundary **bndlist;	 /**< NULL-terminated list of boundaries, for use in the root-finding  code */
	int nrels;
	int safeeval;                    /**< whether to pass the 'safe' flag to relman_eval */
	var_filter_t vfilter;
	rel_filter_t rfilter;            /**< Used to filter relations from rellist (@TODO needs work) */
	void *precdata;                  /**< For use by the preconditioner */
	IntegratorIdaPrecFreeFn *pfree;	 /**< Store instructions here on how to free precdata */
} IntegratorIdaData;

typedef struct IntegratorIdaPrecDJStruct{
	N_Vector PIii; /**< diagonal elements of the inversed Jacobi preconditioner */
} IntegratorIdaPrecDataJacobi;

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

/* dense jacobian evaluation for IDADense dense direct linear solver */
int integrator_ida_djex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, DenseMat Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
);

/* sparse jacobian evaluation for ASCEND's sparse direct solver */
IntegratorSparseJacFn integrator_ida_sjex;

typedef struct IntegratorIdaStatsStruct{
	long nsteps;
	long nrevals;
	long nlinsetups;
	long netfails;
	int qlast, qcur;
	realtype hinused, hlast, hcur;
	realtype tcur;
} IntegratorIdaStats;

int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s);
void integrator_ida_write_stats(IntegratorIdaStats *stats);
void integrator_ida_write_incidence(IntegratorSystem *blsys);
/*------
  Jacobi preconditioner -- experimental 
*/

int integrator_ida_psetup_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *prec_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
);

int integrator_ida_psolve_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *prec_data,
		 N_Vector tmp
);

void integrator_ida_pcreate_jacobi(IntegratorSystem *blsys);

void integrator_ida_pfree_jacobi(IntegratorIdaData *enginedata);

static const IntegratorIdaPrec prec_jacobi = {
	integrator_ida_pcreate_jacobi
	, integrator_ida_psetup_jacobi
	, integrator_ida_psolve_jacobi
};

/*-------------------------------------------------------------
  SETUP/TEARDOWN ROUTINES
*/
void integrator_ida_create(IntegratorSystem *blsys){
	CONSOLE_DEBUG("ALLOCATING IDA ENGINE DATA");
	IntegratorIdaData *enginedata;
	enginedata = ASC_NEW(IntegratorIdaData);
	enginedata->rellist = NULL;
	enginedata->safeeval = 0;
	enginedata->vfilter.matchbits =  VAR_SVAR | VAR_ACTIVE | VAR_FIXED;
	enginedata->vfilter.matchvalue = VAR_SVAR | VAR_ACTIVE;
	enginedata->pfree = NULL;

	enginedata->rfilter.matchbits =  REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;
	enginedata->rfilter.matchvalue = REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;

	blsys->enginedata = (void *)enginedata;

	integrator_ida_params_default(blsys);
}

void integrator_ida_free(void *enginedata){
	CONSOLE_DEBUG("DELETING IDA ENGINE DATA");
	IntegratorIdaData *d = (IntegratorIdaData *)enginedata;
	if(d->pfree){
		/* free the preconditioner data, whatever it happens to be */
		(d->pfree)(enginedata);
	}
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
		}, 1e-5, 1e-15, 1.0e15 }
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
		}, "SPGMR"}, (char *[]){"ASCEND","DENSE","BAND","SPGMR","SPBCG","SPTFQMR",NULL}
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

/*------------------------------------------------------------------------------
  ANALYSIS ROUTINE (new implementation)
*/

typedef void (IntegratorVarVisitorFn)(IntegratorSystem *sys, struct var_variable *var, const int *varindx);
void integrator_visit_system_vars(IntegratorSystem *sys,IntegratorVarVisitorFn *visitor);
IntegratorVarVisitorFn integrator_dae_classify_var;
void integrator_dae_show_var(IntegratorSystem *sys, struct var_variable *var, const int *varindx);

#ifdef ASC_IDA_NEW_ANALYSE
/** 
	Perform additional problem analysis to prepare problem for integration with 
	IDA.

	We assume that the analyse_generate_diffvars routine has been called, so 
	that we just need to call slv_get_diffvars to access the derivative
	chains.

	We can also assume that the independent variable has been found.

	See mailing list, ~Jan 2007.

	Note, the stuff for identifying the static and output sub-problems should
	be part of integrator.c, not this file. We will assume this is handled 

	@return 0 on success 
	@see integrator_analyse
*/
int integrator_ida_analyse(struct IntegratorSystemStruct *sys){
	struct var_variable **solversvars, **vlist;
	unsigned long nsolversvars;
	struct rel_relation **solversrels;
	unsigned long nsolversrels;
	const SolverDiffVarCollection *diffvars;
	SolverDiffVarSequence seq;
	long i, j, n_y, n_ydot, n_dyn;

	struct var_variable *v;
	char *varname;

	CONSOLE_DEBUG("NEW integrator_ida_analyse------------------>");
	
	asc_assert(sys->engine==INTEG_IDA);

	/* get our list of derivative variables */
	solversvars = slv_get_solvers_var_list(sys->system);
	nsolversvars = slv_get_num_solvers_vars(sys->system);
    CONSOLE_DEBUG("Solver has %lu vars",nsolversvars);

	/* partition into static, dynamic and output problems */
	CONSOLE_DEBUG("Block partitioning system...");
	if(slv_block_partition(sys->system)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to block-partition system");
		return 1;
	}

	solversrels = slv_get_solvers_rel_list(sys->system);
	nsolversrels = slv_get_num_solvers_rels(sys->system);
	CONSOLE_DEBUG("System has %lu rels",nsolversrels);

	diffvars = slv_get_diffvars(sys->system);
	asc_assert(diffvars!=NULL);

	CONSOLE_DEBUG("Got %ld chains",diffvars->nseqs);
	
	if(diffvars->maxorder > 2){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"System higher-order derivatives. You must manually reduce the"
			" system to a first-order system of DAEs. (maxorder=%d)"
			,diffvars->maxorder
		);
		return 1;
	};

	n_dyn = diffvars->nalg + 2 * diffvars->ndiff;

	/* set up the dynamic problem */
	CONSOLE_DEBUG("Setting up the dynamic problem");

	asc_assert(sys->y == NULL);

	sys->y = ASC_NEW_ARRAY(struct var_variable *,diffvars->nalg + diffvars->ndiff);
	sys->ydot = ASC_NEW_ARRAY(struct var_variable *,diffvars->nalg + diffvars->ndiff);
	sys->y_id = ASC_NEW_ARRAY(long,n_dyn);
	n_y = 0;

	/* initialise y_id to n_dyn (i.e. off limits) */
	for(i=0;i<n_dyn;++i){
		sys->y_id[i] = n_dyn;
	}                 
	
	/* add the variables from the derivative chains */
	for(i=0; i<diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		if(seq.n == 1){
			v = seq.vars[0];
			varname = var_make_name(sys->system,v);
			if(var_fixed(v)){
				CONSOLE_DEBUG("'%s' is fixed",varname);
				ASC_FREE(varname);
				continue;
			}
			CONSOLE_DEBUG("'%s' is algebraic",varname);
		}else{
			v = seq.vars[0];
			varname = var_make_name(sys->system,v);
			asc_assert(var_active(v));
			if(var_fixed(v)){
				CONSOLE_DEBUG("Differential var '%s' is fixed",varname);
				ASC_FREE(varname);
				for(j=1; j<seq.n; ++j){
					v = seq.vars[j];
					varname = var_make_name(sys->system,v);
					var_set_active(v,FALSE);
					var_set_value(v,0);
					CONSOLE_DEBUG("Derivative '%s' set inactive",varname);
					ASC_FREE(varname);
				}
				continue;
			}else if(var_fixed(seq.vars[1])){
				CONSOLE_DEBUG("Derivative of var '%s' is fixed; converting to algebraic",varname);
			}else{
				/* seq.n > 1, var is not fixed, deriv is not fixed */
				asc_assert(var_active(seq.vars[1]));
				asc_assert(seq.n == 2);
				sys->y[n_y] = v;
				sys->y_id[var_sindex(v)] = n_y;
				CONSOLE_DEBUG("'%s' is differential",varname);
				ASC_FREE(varname);
				v = seq.vars[1];
				sys->ydot[n_y] = v;
				sys->y_id[var_sindex(v)] = -n_y-1;
				varname = var_make_name(sys->system,v);
				CONSOLE_DEBUG("'%s' is derivative",varname);
				ASC_FREE(varname);
				n_y++;
				continue;
			}
		}
		/* fall through: v is algebraic */
		ASC_FREE(varname);
		sys->y[n_y] = v;
		sys->ydot[n_y] = NULL;
		sys->y_id[var_sindex(v)]=n_y;
		n_y++;
		continue;
	}
	sys->n_y = n_y;
	n_ydot = n_y;

	if(sys->n_y != nsolversrels){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Problem is not square: n_y = %d, n_rels = %d (n_alg = %d, n_diff = %d)"
			,sys->n_y, nsolversrels, diffvars->nalg, diffvars->ndiff
		);
		return 2;
	}

	/* check the indep var is same as was located elsewhere */
	if(diffvars->nindep>1){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Multiple variables specified as independent (ode_type=-1)");
		return 3;
	}else if(diffvars->nindep<1){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Independent var not set (ode_type=-1)");
		return 4;
	}else if(diffvars->indep[0]!=sys->x){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Indep var doesn't match");
		return 5;
	}

	/* get the observations */
	sys->n_obs = diffvars->nobs;
	sys->obs = ASC_NEW_ARRAY(struct var_variable *,sys->n_obs);
	for(i=0;i<sys->n_obs;++i){
		sys->obs[i] = diffvars->obs[i];
		varname = var_make_name(sys->system,sys->obs[i]);
		CONSOLE_DEBUG("'%s' is observation",varname);
		ASC_FREE(varname);
	}

	/*   - 'y' list as [ya|yd] */
	/*   - sparsity pattern for dF/dy and dF/dy' */
	/*   - sparsity pattern for union of above */
	/*   - block decomposition based on above */
    /*   - block decomposition results in reordering of y and y' */
	/*   - boundaries (optional) */
	/* ERROR_REPORTER_HERE(ASC_PROG_ERR,"Implementation incomplete");
	return -1; */
}
#endif

/*-------------------------------------------------------------
  MAIN IDA SOLVER ROUTINE, see IDA manual, sec 5.4, p. 27 ff.
*/

/*static double div1(double a, double b){
	return a/b;
}*/

typedef int IdaFlagFn(void *,int *);
typedef char *IdaFlagNameFn(int);

/* return 0 on success */
int integrator_ida_solve(
		IntegratorSystem *blsys
		, unsigned long start_index
		, unsigned long finish_index
){
	void *ida_mem;
	int size, flag, flag1, t_index;
	realtype t0, reltol, abstol, t, tret, tout1;
	N_Vector y0, yp0, abstolvect, ypret, yret, id;
	IntegratorIdaData *enginedata;
	char *linsolver;
	int maxl;
	IdaFlagFn *flagfn;
	IdaFlagNameFn *flagnamefn;
	const char *flagfntype;
	char *pname = NULL;
	char *varname;
	int i;
	const IntegratorIdaPrec *prec = NULL;
	int icopt; /* initial conditions strategy */

	CONSOLE_DEBUG("STARTING IDA...");

	enginedata = integrator_ida_enginedata(blsys);

	enginedata->safeeval = SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_SAFEEVAL);
	CONSOLE_DEBUG("safeeval = %d",enginedata->safeeval);

	/* store reference to list of relations (in enginedata) */
	enginedata->nrels = slv_get_num_solvers_rels(blsys->system);
	enginedata->rellist = slv_get_solvers_rel_list(blsys->system);
	enginedata->bndlist = slv_get_solvers_bnd_list(blsys->system);

	CONSOLE_DEBUG("Number of relations: %d",enginedata->nrels);
	CONSOLE_DEBUG("Number of dependent vars: %ld",blsys->n_y);
	size = blsys->n_y;

	if(enginedata->nrels!=size){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Integration problem is not square (%d rels, %d vars)", enginedata->nrels, size);
		return 1; /* failure */
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
		abstol = SLV_PARAM_REAL(&(blsys->params),IDA_PARAM_ATOL);
		CONSOLE_DEBUG("USING SCALAR ATOL VALUE = %8.2e",abstol);
		flag = IDAMalloc(ida_mem, &integrator_ida_fex, t0, y0, yp0, IDA_SS, reltol, &abstol);
	}

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
	if(strcmp(linsolver,"ASCEND")==0){
		CONSOLE_DEBUG("ASCEND DIRECT SOLVER, size = %d",size);
		IDAASCEND(ida_mem,size);
		IDAASCENDSetJacFn(ida_mem, &integrator_ida_sjex, (void *)blsys);

		flagfntype = "IDAASCEND";
		flagfn = &IDAASCENDGetLastFlag;
		flagnamefn = &IDAASCENDGetReturnFlagName;

	}else if(strcmp(linsolver,"DENSE")==0){
		CONSOLE_DEBUG("DENSE DIRECT SOLVER, size = %d",size);
		flag = IDADense(ida_mem, size);
		switch(flag){
			case IDADENSE_SUCCESS: break;
			case IDADENSE_MEM_NULL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL"); return 5;
			case IDADENSE_ILL_INPUT: ERROR_REPORTER_HERE(ASC_PROG_ERR,"IDADENSE is not compatible with current nvector module"); return 5;
			case IDADENSE_MEM_FAIL: ERROR_REPORTER_HERE(ASC_PROG_ERR,"Memory allocation failed for IDADENSE"); return 5;
			default: ERROR_REPORTER_HERE(ASC_PROG_ERR,"bad return"); return 5;
		}

		if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
			flag = IDADenseSetJacFn(ida_mem, &integrator_ida_djex, (void *)blsys);
			switch(flag){
				case IDADENSE_SUCCESS: break;
				default: ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed IDADenseSetJacFn"); return 6;
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

		/* what preconditioner? */
		pname = SLV_PARAM_CHAR(&(blsys->params),IDA_PARAM_PREC);
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
			(prec->pcreate)(blsys);
			IDASpilsSetPreconditioner(ida_mem,prec->psetup,prec->psolve,(void *)blsys);
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
		if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_AUTODIFF)){
			CONSOLE_DEBUG("USING AUTODIFF");
		    flag = IDASpilsSetJacTimesVecFn(ida_mem, &integrator_ida_jvex, (void *)blsys);
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
			if(SLV_PARAM_BOOL(&(blsys->params),IDA_PARAM_GSMODIFIED)){
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
	if(strcmp(SLV_PARAM_CHAR(&blsys->params,IDA_PARAM_CALCIC),"Y")==0){
		CONSOLE_DEBUG("Solving initial conditions using values of yddot");
		icopt = IDA_Y_INIT;
		asc_assert(icopt!=0);
	}else if(strcmp(SLV_PARAM_CHAR(&blsys->params,IDA_PARAM_CALCIC),"YA_YDP")==0){
		CONSOLE_DEBUG("Solving initial conditions using values of yd");
		icopt = IDA_YA_YDP_INIT;
		asc_assert(icopt!=0);
		id = N_VNew_Serial(blsys->n_y);
		for(i=0; i < blsys->n_y; ++i){
			if(blsys->ydot[i] == NULL){
				NV_Ith_S(id,i) = 0.0;
				varname = var_make_name(blsys->system,blsys->y[i]);
				CONSOLE_DEBUG("y[%d] = '%s' is pure algebraic",i,varname);
				ASC_FREE(varname);
			}else{
				CONSOLE_DEBUG("y[%d] is differential",i);
				NV_Ith_S(id,i) = 1.0;
			}
		}
		IDASetId(ida_mem, id);
		N_VDestroy_Serial(id);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Not solving initial conditions: check current residuals");
	}

	if(icopt){
		blsys->currentstep=0;
	 	t_index=start_index + 1;
		tout1 = samplelist_get(blsys->samples, t_index);

		CONSOLE_DEBUG("SOLVING INITIAL CONDITIONS IDACalcIC (tout1 = %f)", tout1);

#ifdef ASC_SIGNAL_TRAPS
		/* catch SIGFPE if desired to */
		if(enginedata->safeeval){
			CONSOLE_DEBUG("SETTING TO IGNORE SIGFPE...");
			Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
		}else{
#ifdef FEX_DEBUG
			CONSOLE_DEBUG("SETTING TO CATCH SIGFPE...");
#endif
			Asc_SignalHandlerPushDefault(SIGFPE);
		}
		if (setjmp(g_fpe_env)==0) {
#endif

			//CONSOLE_DEBUG("Raising signal...");
			//CONSOLE_DEBUG("1/0 = %f", div1(1.0,0.0));
			//CONSOLE_DEBUG("Still here...");
		
			/* correct initial values, given derivatives */
# if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR==3
			/* note the new API from version 2.3 and onwards */
			flag = IDACalcIC(ida_mem, icopt, tout1);
# else
			flag = IDACalcIC(ida_mem, t0, y0, yp0, icopt, tout1);
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
						return 12;
					}
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s returned flag '%s' (value = %d)",flagfntype,(flagnamefn)(flag1),flag1);
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
			Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
		}else{
			CONSOLE_DEBUG("pop...");
			Asc_SignalHandlerPopDefault(SIGFPE);
			CONSOLE_DEBUG("...pop");
		}
#endif

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
		CONSOLE_DEBUG("Integrating from t0 = %f to t = %f", t0, t);
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

	/* get optional outputs */
#ifdef STATS_DEBUG
	IntegratorIdaStats stats;
	if(IDA_SUCCESS == integrator_ida_stats(ida_mem, &stats)){
		integrator_ida_write_stats(&stats);
	}
#endif

	/* free solution memory */
	N_VDestroy_Serial(yret);
	N_VDestroy_Serial(ypret);

	/* free solver memory */
	IDAFree(ida_mem);

	if(flag < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Solving aborted while attempting t = %f", t);
		return 14;
	}

	/* all done, success */
	return 0;
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
				relname = rel_make_name(blsys->system, *relptr);
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Calculation error in rel '%s'",relname);
				ASC_FREE(relname);
				/* presumable some output already made? */
				is_error = 1;
			}/*else{
				CONSOLE_DEBUG("Calc OK");
			}*/
		}

#ifdef ASC_SIGNAL_TRAPS
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
#endif

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
#ifdef DJEX_DEBUG
	struct var_variable **varlist;
	char *varname;
#endif
	int status;
	struct rel_relation **relptr;
	int i;
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

#ifdef DJEX_DEBUG
	varlist = slv_get_solvers_var_list(blsys->system);

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
		status = relman_diff2(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);

		if(status){
			relname = rel_make_name(blsys->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}

		/* output what's going on here ... */
#ifdef DJEX_DEBUG
		relname = rel_make_name(blsys->system, *relptr);
		CONSOLE_DEBUG("RELATION %d '%s'",i,relname);
		fprintf(stderr,"%d: '%s': ",i,relname);
		ASC_FREE(relname);
		for(j=0;j<count;++j){
			varname = var_make_name(blsys->system, varlist[variables[j]]);
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
		asc_assert(blsys->y_id);
		for(j=0; j < count; ++j){
			var_yindex = blsys->y_id[variables[j]];
			/* the SUNDIALS headers seem not to store 'N' on Windows */
			ASC_ASSERT_RANGE(var_yindex, -blsys->n_y, blsys->n_y);

			if(var_yindex >= 0){
#ifdef DJEX_DEBUG
				asc_assert(blsys->y[var_yindex]==varlist[variables[j]]);
#endif
				DENSE_ELEM(Jac,i,var_yindex) += derivatives[j];
			}else{
#ifdef DJEX_DEBUG
				asc_assert(blsys->ydot[-var_yindex-1]==varlist[variables[j]]);
#endif
				DENSE_ELEM(Jac,i,-var_yindex-1) += derivatives[j] * c_j;
			}
		}
	}

#ifdef DJEX_DEBUG
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

#ifdef DJEX_DEBUG
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
	int count;
	struct var_variable **varlist;
#ifdef JEX_DEBUG

	CONSOLE_DEBUG("EVALUATING JACOBIAN...");
#endif

	blsys = (IntegratorSystem *)jac_data;
	enginedata = integrator_ida_enginedata(blsys);
	varlist = slv_get_solvers_var_list(blsys->system);

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

#ifdef ASC_SIGNAL_TRAPS
	Asc_SignalHandlerPushDefault(SIGFPE);
	if (SETJMP(g_fpe_env)==0) {
#endif
		for(i=0, relptr = enginedata->rellist;
				i< enginedata->nrels && relptr != NULL;
				++i, ++relptr
		){
			/* get derivatives for this particular relation */
			status = relman_diff2(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
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
			asc_assert(blsys->y_id);
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
				if(varlist[variables[j]] == blsys->x) continue;

				var_yindex = blsys->y_id[variables[j]];
#ifdef JEX_DEBUG
				CONSOLE_DEBUG("j = %d: variables[j] = %d, y_id = %ld",j,variables[j],var_yindex);
#endif

				ASC_ASSERT_RANGE(-var_yindex-1, -NV_LENGTH_S(v),NV_LENGTH_S(v));
				
				if(var_yindex >= 0){
#ifdef JEX_DEBUG
					asc_assert(blsys->y[var_yindex]==varlist[variables[j]]);
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
					asc_assert(blsys->ydot[-var_yindex-1]==varlist[variables[j]]);
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
#ifdef ASC_SIGNAL_TRAPS
	}else{
		relname = rel_make_name(blsys->system, *relptr);
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
int integrator_ida_sjex(long int Neq, realtype tt
		, N_Vector yy, N_Vector yp, N_Vector rr
		, realtype c_j, void *jac_data, mtx_matrix_t Jac
		, N_Vector tmp1, N_Vector tmp2, N_Vector tmp3
){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return -1;
}

/*----------------------------------------------
  JACOBI PRECONDITIONER -- EXPERIMENTAL.
*/

void integrator_ida_pcreate_jacobi(IntegratorSystem *blsys){
	IntegratorIdaData * enginedata =blsys->enginedata;
	IntegratorIdaPrecDataJacobi *precdata;
	precdata = ASC_NEW(IntegratorIdaPrecDataJacobi);

	asc_assert(blsys->n_y);
	precdata->PIii = N_VNew_Serial(blsys->n_y);

	enginedata->pfree = &integrator_ida_pfree_jacobi;
	enginedata->precdata = precdata;
	CONSOLE_DEBUG("Allocated memory for Jacobi preconditioner");
}

void integrator_ida_pfree_jacobi(IntegratorIdaData *enginedata){
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
int integrator_ida_psetup_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 realtype c_j, void *p_data,
		 N_Vector tmp1, N_Vector tmp2,
		 N_Vector tmp3
){
	int i, j;
	IntegratorSystem *blsys;
	IntegratorIdaData *enginedata;
	IntegratorIdaPrecDataJacobi *precdata;
	struct rel_relation **relptr;

	blsys = (IntegratorSystem *)p_data;
	enginedata = blsys->enginedata;
	precdata = (IntegratorIdaPrecDataJacobi *)(enginedata->precdata);
	double *derivatives;
	int *variables;
	int count, status;
	char *relname;
	int var_yindex;

	CONSOLE_DEBUG("Setting up Jacobi preconditioner");

	variables = ASC_NEW_ARRAY(int, NV_LENGTH_S(yy) * 2);
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
		status = relman_diff2(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			relname = rel_make_name(blsys->system, *relptr);
			CONSOLE_DEBUG("ERROR calculating preconditioner derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}
		/* CONSOLE_DEBUG("Got %d derivatives from relation %d",count,i); */
		/* find the diagonal elements */
		asc_assert(blsys->y_id);
		for(j=0; j<count; ++j){
			if(variables[j]==i){
				var_yindex = blsys->y_id[variables[j]];
				if(var_yindex >= 0){
					NV_Ith_S(precdata->PIii, i) = 1./derivatives[j];
				}else{
					NV_Ith_S(precdata->PIii, i) = 1./(c_j * derivatives[j]);
				}
			}
		}
#ifdef PREC_DEBUG
		CONSOLE_DEBUG("PI[%d] = %f",i,NV_Ith_S(precdata->PIii,i));
#endif
	}

	if(status){
		CONSOLE_DEBUG("Error found when evaluating derivatives");
		return 1; /* recoverable */
	}

	integrator_ida_write_incidence(blsys);

	ASC_FREE(variables);
	ASC_FREE(derivatives);

	return 0;
};

/**
	EXPERIMENTAL. Jacobi preconditioner for use with IDA Krylov solvers

	'solve' function.
*/
int integrator_ida_psolve_jacobi(realtype tt,
		 N_Vector yy, N_Vector yp, N_Vector rr,
		 N_Vector rvec, N_Vector zvec,
		 realtype c_j, realtype delta, void *p_data,
		 N_Vector tmp
){
	IntegratorSystem *blsys;
	IntegratorIdaData *data;
	IntegratorIdaPrecDataJacobi *precdata;
	blsys = (IntegratorSystem *)p_data;
	data = blsys->enginedata;
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

	@return IDA_SUCCESS on sucess.
*/
int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s){
	return IDAGetIntegratorStats(ida_mem, &s->nsteps, &s->nrevals, &s->nlinsetups
		,&s->netfails, &s->qlast, &s->qcur, &s->hinused
		,&s->hlast, &s->hcur, &s->tcur
	);
}

/**
	This routine just outputs the stats to the CONSOLE_DEBUG routine.

	@TODO provide a GUI way of stats reporting from IDA.
*/
void integrator_ida_write_stats(IntegratorIdaStats *stats){
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

enum integrator_ida_write_jac_enum{
	II_WRITE_Y
	, II_WRITE_YDOT
};

/**
	@TODO COMPLETE THIS...
*/
void integrator_ida_write_jacobian(IntegratorSystem *blsys, realtype c_j, FILE *f, enum integrator_ida_write_jac_enum type){
	IntegratorIdaData *enginedata;
	MM_typecode matcode;                        
	int nnz, rhomax;
	double *derivatives;
	int *variables;
	struct rel_relation **relptr;
	int i, j, status, count, var_yindex;
	char *relname;

	var_filter_t vfiltery  = {
		VAR_SVAR | VAR_FIXED | VAR_DERIV
	    , VAR_SVAR 
	};
	var_filter_t vfilteryd = {
		VAR_SVAR | VAR_FIXED | VAR_DERIV
		, VAR_SVAR | VAR_DERIV
	};

	enginedata = (IntegratorIdaData *)blsys->enginedata;

	/* number of non-zeros for all the non-FIXED solver_vars,
		in all the active included equality relations.
	*/
	nnz = relman_jacobian_count(enginedata->rellist, enginedata->nrels
		, &enginedata->vfilter, &enginedata->rfilter
		, &rhomax
	);

	/* we must have found the same number of relations */
	asc_assert(rhomax == enginedata->nrels);

	/* output the mmio file header, now that we know our size*/
	/* note that we are asserting that our problem is square */
	mm_initialize_typecode(&matcode);
	mm_set_matrix(&matcode);
	mm_set_coordinate(&matcode);
	mm_set_real(&matcode);
    mm_write_banner(f, matcode); 
    mm_write_mtx_crd_size(f, enginedata->nrels, enginedata->nrels, nnz);	

	variables = ASC_NEW_ARRAY(int, blsys->n_y * 2);
	derivatives = ASC_NEW_ARRAY(double, blsys->n_y * 2);

	CONSOLE_DEBUG("Writing sparse Jacobian to file...");

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		relname = rel_make_name(blsys->system, *relptr);

		/* get derivatives of y */
		status = relman_diff2(*relptr, &vfiltery, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}

		/* get derivatives of y */
		status = relman_diff2(*relptr, &vfilteryd, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}

		asc_assert(blsys->y_id);
		for(j=0; j<count; ++j){
			var_yindex = blsys->y_id[variables[j]];
			if(var_yindex >= 0 && type == II_WRITE_Y){
		        fprintf(f, "%d %d %10.3g\n", i, var_yindex, derivatives[j]);
			}else if(var_yindex < 0 && type == II_WRITE_YDOT){
				fprintf(f, "%d %d %10.3g\n", i, -var_yindex-1, derivatives[j]);
			}
		}
	}
	ASC_FREE(variables);
	ASC_FREE(derivatives);
}

/**
	This routine outputs matrix structure in a crude text format, for the sake
	of debugging.
*/
void integrator_ida_write_incidence(IntegratorSystem *blsys){
	int i, j;
	struct rel_relation **relptr;
	IntegratorIdaData *enginedata = blsys->enginedata;
	double *derivatives;
	int *variables;
	int count, status;
	char *relname;
	int var_yindex;

	if(enginedata->nrels > 100){
		CONSOLE_DEBUG("Ignoring call (matrix size too big = %d)",enginedata->nrels);
		return;
	}

	variables = ASC_NEW_ARRAY(int, blsys->n_y * 2);
	derivatives = ASC_NEW_ARRAY(double, blsys->n_y * 2);

	CONSOLE_DEBUG("Outputting incidence information to console...");

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		relname = rel_make_name(blsys->system, *relptr);

		/* get derivatives for this particular relation */
		status = relman_diff2(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}

		fprintf(stderr,"%3d:%-15s:",i,relname);		
		ASC_FREE(relname);

		asc_assert(blsys->y_id);
		for(j=0; j<count; ++j){
			var_yindex = blsys->y_id[variables[j]];
			if(var_yindex >= 0){
				fprintf(stderr," %d:y[%d]",variables[j],var_yindex);
			}else{
				fprintf(stderr," %d:ydot[%d]",variables[j],-var_yindex-1);
			}
		}
		fprintf(stderr,"\n");
	}
	ASC_FREE(variables);
	ASC_FREE(derivatives);
}

/* @return 0 on success */
int integrator_ida_debug(const IntegratorSystem *sys, FILE *fp){
	char *varname;
	struct var_variable **vlist, *var;
	int vlen;
	long i, y_id;

	fprintf(fp,"THERE ARE %ld VARIABLES IN THE INTEGRATION SYSTEM\n\n",sys->n_y);

	/* if(integrator_sort_obs_vars(sys))return 10; */

	fprintf(fp,"RESULTS OF ANALYSIS\n\n");
	fprintf(fp,"index\ty\tydot\n");
	fprintf(fp,"-----\t-----\t-----\n");
	for(i=0;i<sys->n_y;++i){
		varname = var_make_name(sys->system, sys->y[i]);
		fprintf(fp,"%ld\t%s\t",i,varname);
		if(sys->ydot[i]){
			ASC_FREE(varname);
			varname = var_make_name(sys->system, sys->ydot[i]);
			fprintf(fp,"%s\n",varname);
			ASC_FREE(varname);
		}else{
			fprintf(fp,".\n",varname);
			ASC_FREE(varname);
		}
	}

	fprintf(fp,"\n\nCORRESPONDENCE OF SOLVER VARS TO INTEGRATOR VARS\n\n");
	fprintf(fp,"index\t%-15s\ty_id\ty\tydot\n","name");
	fprintf(fp,"-----\t%-15s\t-----\t-----\t-----\n","----");


	/* visit all the slv_system_t master var lists to collect vars */
	/* find the vars mostly in this one */
	vlist = slv_get_solvers_var_list(sys->system);
	vlen = slv_get_num_solvers_vars(sys->system);
	for (i=0;i<vlen;i++) {
		var = vlist[i];

		varname = var_make_name(sys->system, var);
		fprintf(fp,"%d\t%-15s\t",i,varname);

		if(var_fixed(var)){
			// it's fixed, so not really a DAE var
			fprintf(fp,"(fixed)\n");
		}else if(!var_active(var)){
			// inactive
			fprintf(fp,"(inactive)\n");
		}else{
			// a DAE var, otherwise
			y_id = sys->y_id[i];

			fprintf(fp,"%ld", y_id);
			if(y_id >= 0){
				fprintf(fp,"\ty[%ld]\t.\n",y_id);
			}else{
				fprintf(fp,"\t.\tydot[%ld]\n",-y_id-1);
			}

			ASC_ASSERT_LT(i,1e7L);
			ASC_ASSERT_LT(y_id, 9999999L);
			ASC_ASSERT_LT(-9999999L, y_id);
		}
		ASC_FREE(varname);


	}


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
