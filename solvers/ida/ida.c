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
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include "ida.h"
#include "idalinear.h"
#include "idaanalyse.h"
#include "idatypes.h"
#include "idaprec.h"
#include "idacalc.h"
#include "idaio.h"
#include "idaboundary.h"

#include <signal.h>
#include <setjmp.h>
#include <fenv.h>
#include <math.h>

#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/panic.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/packages.h>

#include <ascend/system/slv_client.h>
#include <ascend/system/bndman.h>
#include <ascend/system/relman.h>
#include <ascend/system/block.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/jacobian.h>
#include <ascend/system/bndman.h>

#include <ascend/utilities/config.h>
#include <ascend/integrator/integrator.h>

#ifndef IDA_DEBUG
# define IDA_DEBUG 0
#endif
#if !IDA_DEBUG
# undef CONSOLE_DEBUG
# define CONSOLE_DEBUG(...) ((void)0)
#endif

#if IDA_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...)
#endif

/* #define FEX_DEBUG */
/* #define SOLVE_DEBUG */
/* #define STATS_DEBUG */
/* #define DESTROY_DEBUG */

/*-------------------------------------------------------------
 SOLVER REGISTRATION
 */

/*
	The following functions are declared static because we don't want them
	ever to be called directly from outside code. Instead, they are call via
	function pointers passed through the ida_register function.
*/
static IntegratorCreateFn integrator_ida_create;
static IntegratorParamsDefaultFn integrator_ida_params_default;
static IntegratorInitialiseFn integrator_ida_initialise;
static IntegratorSolveFn integrator_ida_solve;
static IntegratorFreeFn integrator_ida_free;

/**
 This data structure contains pointers to the various functions that are
 exposed to libascend in order for ASCEND to drive this solver.
 */
static const IntegratorInternals integrator_ida_internals = {
		integrator_ida_create, integrator_ida_params_default,
		integrator_ida_analyse, integrator_ida_initialise, integrator_ida_solve,
		integrator_ida_write_matrix, integrator_ida_debug, integrator_ida_free,
		INTEG_IDA, "IDA" };

/**
 This function is accessed by libascend when loading this solver. The
 function will register the integrator such that it can then be applied
 to solving problems.
 */
extern ASC_EXPORT int ida_register(void) {
	MSG("Registering IDA...");
	return integrator_register(&integrator_ida_internals);
}

/*-------------------------------------------------------------
 FORWARD DECLS
 */

typedef void ( IntegratorVarVisitorFn)(IntegratorSystem *integ,
		struct var_variable *var, const int *varindx);

/*static IntegratorVarVisitorFn integrator_dae_classify_var;
 static void integrator_visit_system_vars(IntegratorSystem *integ,IntegratorVarVisitorFn *visitor);
 static void integrator_dae_show_var(IntegratorSystem *integ, struct var_variable *var, const int *varindx); */

static int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s);

/*-------------------------------------------------------------
 SETUP/TEARDOWN ROUTINES
 */

/**
 Allocate memory inside the IntegratorIdaData structure at
 the time when IDA is assigned to a particular system as its integrator.
 */
static void integrator_ida_create(IntegratorSystem *integ) {
	MSG("ALLOCATING IDA ENGINE DATA");
	IntegratorIdaData *enginedata;
	enginedata = ASC_NEW(IntegratorIdaData);
	MSG("enginedata = %p",enginedata);
	enginedata->rellist = NULL;
	enginedata->safeeval = 0;
	enginedata->warned_minstep_ignored = 0;
	enginedata->vfilter.matchbits = VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE
			| VAR_FIXED;
	enginedata->vfilter.matchvalue = VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | 0;
	enginedata->pfree = NULL;

	enginedata->rfilter.matchbits = REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;
	enginedata->rfilter.matchvalue = REL_EQUALITY | REL_INCLUDED | REL_ACTIVE;

	enginedata->flagfn = NULL;
	enginedata->flagfntype = NULL;
	enginedata->flagnamefn = NULL;
#if SUNDIALS_VERSION_MAJOR >= 5
	enginedata->linear_solver = NULL;
	enginedata->dense_matrix = NULL;
#endif
#if SUNDIALS_VERSION_MAJOR >= 6
	enginedata->sunctx = NULL;
# if SUNDIALS_VERSION_MAJOR >= 7
	if(SUNContext_Create(SUN_COMM_NULL, &enginedata->sunctx)){
# else
	if(SUNContext_Create(NULL, &enginedata->sunctx)){
# endif
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to create SUNDIALS context for IDA");
	}
#endif

	integ->enginedata = (void *) enginedata;

	integrator_ida_params_default(integ);
}

/**
 Free any memory specifically allocated for use by our ASCEND wrapper for
 the IDA integrator, other than that which the IDA code itself may be
 managing.
 */
static void integrator_ida_free(void *enginedata) {
#ifdef DESTROY_DEBUG
	MSG("DESTROYING IDA engine data at %p",enginedata);
#endif
	IntegratorIdaData *d = (IntegratorIdaData *) enginedata;
	asc_assert(d);
	if (d->pfree) {
		MSG("DESTROYING preconditioner data using fn at %p",d->pfree);
		/* free the preconditioner data, whatever it happens to be */
		(d->pfree)(enginedata);
	}

	ASC_FREE(d->rellist);

#if SUNDIALS_VERSION_MAJOR >= 5
	if(d->linear_solver != NULL){
		SUNLinSolFree(d->linear_solver);
		d->linear_solver = NULL;
	}
	if(d->dense_matrix != NULL){
		SUNMatDestroy(d->dense_matrix);
		d->dense_matrix = NULL;
	}
#endif
#if SUNDIALS_VERSION_MAJOR >= 6
	if(d->sunctx != NULL){
		SUNContext_Free(&d->sunctx);
	}
#endif

#ifdef DESTROY_DEBUG
	MSG("Now destroying the enginedata");
#endif
	ASC_FREE(d);
#ifdef DESTROY_DEBUG
	MSG("enginedata freed");
#endif
}

static int integrator_ida_initialise(IntegratorSystem *integ){
	int qrslv_index;

	if(!integrator_has_initial_relations(integ)){
		return 0;
	}
	if(package_load("qrslv", NULL) != 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to load QRSlv for IDA initialization solve");
		return 1;
	}
	qrslv_index = slv_lookup_client("QRSlv");
	if(qrslv_index < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"QRSlv is unavailable for IDA initialization solve");
		return 2;
	}
	return integrator_initialise_with_solver(integ, qrslv_index);
}

IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *integ) {
	IntegratorIdaData *d;
	assert(integ!=NULL);
	assert(integ->enginedata!=NULL);
	assert(integ->engine==INTEG_IDA);
	d = ((IntegratorIdaData *) (integ->enginedata));
	return d;
}

/*-------------------------------------------------------------
 PARAMETERS FOR IDA
 */

enum ida_parameters {
	IDA_PARAM_LINSOLVER,
	IDA_PARAM_MAXL,
	IDA_PARAM_MAXORD,
	IDA_PARAM_AUTODIFF,
	IDA_PARAM_CALCIC,
	IDA_PARAM_SAFEEVAL,
	IDA_PARAM_RTOL,
	IDA_PARAM_ATOL,
	IDA_PARAM_ATOLVECT,
	IDA_PARAM_GSMODIFIED,
	IDA_PARAM_MAXNCF,
	IDA_PARAM_PREC,
	IDA_PARAMS_SIZE
};

/**
 Here the full set of parameters is defined, along with upper/lower bounds,
 etc. The values are stuck into the integ->params structure.

 To add a new parameter, first give it a name IDA_PARAM_* in thge above enum ida_parameters
 list. Then add a slv_param_*(...) statement below to define the type, description and range
 for the new parameter.

 @return 0 on success
 */
static int integrator_ida_params_default(IntegratorSystem *integ) {
	asc_assert(integ!=NULL);
	asc_assert(integ->engine==INTEG_IDA);
	slv_parameters_t *p;
	p = &(integ->params);

	slv_destroy_parms(p);

	if (p->parms == NULL) {
		MSG("params NULL");
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, IDA_PARAMS_SIZE);
		if (p->parms == NULL)
			return -1;
		p->dynamic_parms = 1;
	} else {
		MSG("params not NULL");
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_bool(p,IDA_PARAM_AUTODIFF,
		(SlvParameterInitBool) { {"autodiff"
						,"Use auto-diff?",1
						,"Use automatic differentiation of expressions (1) or use numerical derivatives (0)"
			}, TRUE}
	);

	slv_param_char(p,IDA_PARAM_CALCIC
		,(SlvParameterInitChar) { {"calcic"
				,"Initial conditions calcuation",1
				,"Use specified values of ydot to solve for inital y (Y),"
				" or use the the values of the differential variables (yd) to solve"
				" for the pure algebraic variables (ya) along with the derivatives"
				" of the differential variables (yddot) (YA_YDP), or else don't solve"
				" the intial conditions at all (NONE). See IDA manual p 41 (IDASetId)"
		}, "YA_YDP"}, (char *[]) {"Y", "YA_YDP", "NONE",NULL}
	);

	slv_param_bool(p,IDA_PARAM_SAFEEVAL
		,(SlvParameterInitBool) { {"safeeval"
				,"Use safe evaluation?",1
				,"Use 'safe' function evaluation routines (TRUE) or allow ASCEND to "
				"throw SIGFPE errors which will then halt integration."
			}, FALSE}
	);

	slv_param_bool(p,IDA_PARAM_ATOLVECT
		,(SlvParameterInitBool) { {"atolvect"
				,"Use 'ode_atol' values as specified?",1
				,"If TRUE, values of 'ode_atol' are taken from your model and used "
				" in the integration. If FALSE, a scalar absolute tolerance value"
				" is shared by all variables. See IDA manual, section 5.5.1"
			}, TRUE}
	);

	slv_param_real(p,IDA_PARAM_ATOL
		,(SlvParameterInitReal) { {"atol"
				,"Scalar absolute error tolerance",1
				,"Value of the scalar absolute error tolerance. See also 'atolvect'."
				" See IDA manual, sections 5.5.1 and 5.5.2 'Advice on choice and use of tolerances'"
			}, 1e-5, 0, 1e10}
	);

	slv_param_real(p,IDA_PARAM_RTOL
		,(SlvParameterInitReal) { {"rtol"
				,"Scalar relative error tolerance",1
				,"Value of the scalar relative error tolerance. (Note that for IDA,"
				" it's not possible to set per-variable relative tolerances as it is"
				" with LSODE)."
				" See IDA manual, section 5.5.2 'Advice on choice and use of tolerances'"
			}, 1e-4, 0, 1}
	);

	slv_param_char(p,IDA_PARAM_LINSOLVER
		,(SlvParameterInitChar) { {"linsolver"
				,"Linear solver",1
				,"See IDA manual, section 5.5.3. Choose 'ASCEND' to use the linsolqr"
				" direct linear solver bundled with ASCEND, 'DENSE' to use the dense"
				" solver bundled with IDA, or one of the Krylov solvers SPGMR, SPBCG"
				" or SPTFQMR (which still need preconditioners to be implemented"
				" before they can be very useful."
			}, "DENSE"}, (char *[]) {"ASCEND","DENSE","BAND","SPGMR","SPBCG","SPTFQMR",NULL}
	);

	slv_param_int(p,IDA_PARAM_MAXL
		,(SlvParameterInitInt) { {"maxl"
				,"Maximum Krylov dimension",0
				,"The maximum dimension of Krylov space used by the linear solver"
				" (for SPGMR, SPBCG, SPTFQMR) with IDA. See IDA manual section 5.5."
				" The default of 0 results in IDA using its internal default, which"
				" is currently a value of 5."
			}, 0, 0, 20}
	);

	slv_param_int(p,IDA_PARAM_MAXORD
		,(SlvParameterInitInt) { {"maxord"
				,"Maximum order of linear multistep method",0
				,"The maximum order of the linear multistep method with IDA. See"
				" IDA manual p 38."
			}, 5, 1, 5}
	);

	slv_param_bool(p,IDA_PARAM_GSMODIFIED
		,(SlvParameterInitBool) { {"gsmodified"
				,"Gram-Schmidt Orthogonalisation Scheme", 2
				,"TRUE = GS_MODIFIED, FALSE = GS_CLASSICAL. See IDA manual section"
				" 5.5.6.6. Only applies when linsolve=SPGMR is selected."
			}, TRUE}
	);

	slv_param_int(p,IDA_PARAM_MAXNCF
		,(SlvParameterInitInt) { {"maxncf"
				,"Max nonlinear solver convergence failures per step", 2
				,"Maximum number of allowable nonlinear solver convergence failures"
				" on one step. See IDA manual section 5.5.6.1."
			}, 10,0,1000}
	);

	slv_param_char(p,IDA_PARAM_PREC
		,(SlvParameterInitChar) { {"prec"
				,"Preconditioner",1
				,"See IDA manual, section section 5.6.8."
			},"NONE"}, (char *[]) {"NONE","JACOBI","DIAG",NULL}
	);

	asc_assert(p->num_parms == IDA_PARAMS_SIZE);

	MSG("Created %d params", p->num_parms);

	return 0;
}

/*******************************************
 * SOLVE SETUP FUNCTIONS
 *******************************************/

int ida_load_rellist(IntegratorSystem *integ) {
	IntegratorIdaData *enginedata;
	struct rel_relation **rels;
	char *relname;
	int i, j, n_solverrels, n_active_rels;

	enginedata = integrator_ida_enginedata(integ);

	n_solverrels = slv_get_num_solvers_rels(integ->system);
	n_active_rels = slv_count_solvers_rels(integ->system, &integrator_ida_rel);
	rels = slv_get_solvers_rel_list(integ->system);

	if (enginedata->rellist != NULL) {
		ASC_FREE(enginedata->rellist);
		enginedata->rellist = NULL;
	}

	enginedata->rellist
			= ASC_NEW_ARRAY(struct rel_relation *, n_active_rels);

#ifdef SOLVE_DEBUG
	MSG("rels matchbits:  0x%x",integrator_ida_rel.matchbits);
	MSG("rels matchvalue: 0x%x",integrator_ida_rel.matchvalue);

	MSG("Number of relations: %d",n_solverrels);
	MSG("Number of active relations: %d",n_active_rels);
	MSG("Number of dependent vars: %d",integ->n_y);
	MSG("Number of boundaries: %d",enginedata->nbnds);
#endif


	j = 0;
	for (i = 0; i < n_solverrels; ++i) {
		if (rel_apply_filter(rels[i], &integrator_ida_rel)) {
#ifdef SOLVE_DEBUG
			relname = rel_make_name(integ->system, rels[i]);
			MSG("rel '%s': 0x%x", relname, rel_flags(rels[i]));
			ASC_FREE(relname);
#endif
			enginedata->rellist[j++] = rels[i];
		}
	}

	asc_assert(j == n_active_rels);
	enginedata->nrels = n_active_rels;

	if (enginedata->nrels != integ->n_y) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR
				,"Integration problem is not square (%d active rels, %d vars)"
				,n_active_rels, integ->n_y
		);
		return 1; /* failure */
	}

	return 0;
}

/*
 * Retrieve initial values from the system
 */
int ida_retrieve_IVs(IntegratorSystem *integ, realtype t0, N_Vector y0,
		N_Vector yp0) {

#ifdef SOLVE_DEBUG
	char *varname;
	char diffname[100];
	int i;
	MSG("RETRIEVING INITIAL VALUES:");
	MSG("t0 = %f",t0);
#endif

	integrator_get_y(integ, NV_DATA_S(y0));
	integrator_get_ydot(integ, NV_DATA_S(yp0));

#ifdef SOLVE_DEBUG
	fprintf(stderr, "index\t%25s\t%25s\n", "y", "ydot");
	for (i = 0; i < integ->n_y; ++i) {
		varname = var_make_name(integ->system, integ->y[i]);
		fprintf(stderr, "%d\t%15s=%10f\t", i, varname, NV_Ith_S(y0,i));

		if (integ->ydot[i]) {
			ASC_FREE(varname);
			varname = var_make_name(integ->system, integ->ydot[i]);
			fprintf(stderr, "%15s=%10f\t\n", varname, NV_Ith_S(yp0,i));
		} else {
			snprintf(diffname,sizeof(diffname),"diff(%s)",varname);
			fprintf(stderr,"%15s=%10f\t\n",diffname,NV_Ith_S(yp0,i));
		}
		ASC_FREE(varname);
	}
#endif

	return 0;
}

/*
 * Assign internal memory for the appropriate sundials version and error
 * tolerance parameter settings.
 */
int ida_malloc(IntegratorSystem *integ, void *ida_mem, realtype t0,
		N_Vector y0, N_Vector yp0) {
	int flag;
	N_Vector abstolvect;
	realtype reltol, abstol;

	/* relative error tolerance */
	reltol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_RTOL);
	MSG("rtol = %8.2e",reltol);


#if SUNDIALS_VERSION_MAJOR >= 5
	flag = IDAInit(ida_mem, &integrator_ida_fex, t0, y0 ,yp0);
#endif

	if (flag == IDA_MEM_NULL) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
		return 2;
	} else if (flag == IDA_MEM_FAIL) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate memory (IDAInit)");
		return 3;
	} else if (flag == IDA_ILL_INPUT) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid input to IDAInit");
		return 4;
	}

#if SUNDIALS_VERSION_MAJOR >= 5
	MSG("Assigning tolerances...");
	/* assign tolerances */
	if(SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_ATOLVECT)) {
		MSG("using vector of atol values");
#if SUNDIALS_VERSION_MAJOR >= 6
		{
			IntegratorIdaData *enginedata = integrator_ida_enginedata(integ);
			abstolvect = N_VNew_Serial(integ->n_y, enginedata->sunctx);
		}
#else
		abstolvect = N_VNew_Serial(integ->n_y);
#endif
		integrator_get_atol(integ,NV_DATA_S(abstolvect));
		IDASVtolerances(ida_mem, reltol, abstolvect);
		N_VDestroy_Serial(abstolvect);
	} else {
		/* scalar tolerances */
		abstol = SLV_PARAM_REAL(&(integ->params),IDA_PARAM_ATOL);
		MSG("using scalar atol value = %8.2e",abstol);
		IDASStolerances(ida_mem, reltol, abstol);
	}
#endif

	/* success */
	return 0;
}

/*
 * Set parameter inputs for step size, the linear solver module and preconditioner
 */
int ida_set_optional_inputs(IntegratorSystem *integ, void *ida_mem, N_Vector y0) {
	int flag;
	char *linsolver;
	char *pname = NULL;
	int maxl;
	const IntegratorIdaPrec *prec = NULL;

	IntegratorIdaData *enginedata = integ->enginedata;

#if SUNDIALS_VERSION_MAJOR >= 7
	/* IDASetErrHandlerFn was removed in SUNDIALS 7; use default context-based handling. */
#elif SUNDIALS_VERSION_MAJOR >= 5
	IDASetErrHandlerFn(ida_mem, &integrator_ida_error, (void *) integ);
#endif
#if SUNDIALS_VERSION_MAJOR >= 5
	IDASetUserData(ida_mem, (void *)integ);
#endif
	IDASetMaxStep(ida_mem, integrator_get_maxstep(integ));
	IDASetInitStep(ida_mem, integrator_get_stepzero(integ));
	IDASetMaxNumSteps(ida_mem, integrator_get_maxsubsteps(integ));
	if (integrator_get_minstep(integ) > 0 && !enginedata->warned_minstep_ignored) {
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"IDA does not support minstep (ignored)\n");
		enginedata->warned_minstep_ignored = 1;
	}

	MSG("MAXNCF = %d",SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXNCF));
	IDASetMaxConvFails(ida_mem, SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXNCF));

	MSG("MAXORD = %d",SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXORD));
	IDASetMaxOrd(ida_mem, SLV_PARAM_INT(&integ->params,IDA_PARAM_MAXORD));

	/* there's no capability for setting *minimum* step size in IDA */

	if (enginedata->pfree) {
		(enginedata->pfree)(enginedata);
	}
#if SUNDIALS_VERSION_MAJOR >= 5
	if (enginedata->linear_solver != NULL) {
		SUNLinSolFree(enginedata->linear_solver);
		enginedata->linear_solver = NULL;
	}
	if (enginedata->dense_matrix != NULL) {
		SUNMatDestroy(enginedata->dense_matrix);
		enginedata->dense_matrix = NULL;
	}
#endif

	/* attach linear solver module, using the default value of maxl */
	linsolver = SLV_PARAM_CHAR(&(integ->params),IDA_PARAM_LINSOLVER);
	MSG("ASSIGNING LINEAR SOLVER '%s'",linsolver);
	if (strcmp(linsolver, "ASCEND") == 0) {
		ERROR_REPORTER_HERE(ASC_PROG_WARNING
			,"The experimental ASCEND direct linear solver is unavailable with SUNDIALS %d; using DENSE instead"
			,SUNDIALS_VERSION_MAJOR
		);
		linsolver = "DENSE";
	}

	if (strcmp(linsolver, "DENSE") == 0) {
		MSG("DENSE DIRECT SOLVER, size = %d",integ->n_y);
#if SUNDIALS_VERSION_MAJOR >= 6
		enginedata->dense_matrix = SUNDenseMatrix(integ->n_y, integ->n_y, enginedata->sunctx);
		enginedata->linear_solver = SUNLinSol_Dense(y0, enginedata->dense_matrix, enginedata->sunctx);
#else
		enginedata->dense_matrix = SUNDenseMatrix(integ->n_y, integ->n_y);
		enginedata->linear_solver = SUNLinSol_Dense(y0, enginedata->dense_matrix);
#endif
		if (enginedata->dense_matrix == NULL) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to allocate SUNMatrix for IDA");
			return 5;
		}
		if (enginedata->linear_solver == NULL) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to allocate dense SUNLinearSolver for IDA");
			return 5;
		}
		flag = IDASetLinearSolver(ida_mem, enginedata->linear_solver, enginedata->dense_matrix);
		if (flag != IDALS_SUCCESS) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to attach dense linear solver to IDA");
			return 5;
		}

		if (SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_AUTODIFF)) {
			MSG("USING AUTODIFF");
			flag = IDASetJacFn(ida_mem, &integrator_ida_djex);
			if (flag != IDALS_SUCCESS) {
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed IDASetJacFn");
				return 6;
			}
		} else {
			MSG("USING NUMERICAL DIFF");
		}

		enginedata->flagfntype = "IDADENSE";
		enginedata->flagfn = &IDAGetLastLinFlag;
		enginedata->flagnamefn = &IDAGetLinReturnFlagName;
	} else {
		/* remaining methods are all SPILS */
		MSG("IDA SPILS");

		maxl = SLV_PARAM_INT(&(integ->params),IDA_PARAM_MAXL);
		MSG("maxl = %d",maxl);

		/* what preconditioner? */
		pname = SLV_PARAM_CHAR(&(integ->params),IDA_PARAM_PREC);
		if (strcmp(pname, "NONE") == 0) {
			prec = NULL;
		} else if (strcmp(pname, "JACOBI") == 0 || strcmp(pname, "DIAG") == 0) {
			prec = &prec_jacobi;
		} else {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid preconditioner choice '%s'",pname);
			return 7;
		}

		/* which SPILS linear solver? */
		if (strcmp(linsolver, "SPGMR") == 0) {
			MSG("IDA SPGMR");
#if SUNDIALS_VERSION_MAJOR >= 6
			enginedata->linear_solver = SUNLinSol_SPGMR(y0, prec ? SUN_PREC_LEFT : SUN_PREC_NONE, maxl, enginedata->sunctx);
#else
			enginedata->linear_solver = SUNLinSol_SPGMR(y0, prec ? SUN_PREC_LEFT : SUN_PREC_NONE, maxl);
#endif
			flag = enginedata->linear_solver == NULL ? IDALS_MEM_FAIL : IDASetLinearSolver(ida_mem, enginedata->linear_solver, NULL);
		} else if (strcmp(linsolver, "SPBCG") == 0) {
			MSG("IDA SPBCG");
#if SUNDIALS_VERSION_MAJOR >= 6
			enginedata->linear_solver = SUNLinSol_SPBCGS(y0, prec ? SUN_PREC_LEFT : SUN_PREC_NONE, maxl, enginedata->sunctx);
#else
			enginedata->linear_solver = SUNLinSol_SPBCGS(y0, prec ? SUN_PREC_LEFT : SUN_PREC_NONE, maxl);
#endif
			flag = enginedata->linear_solver == NULL ? IDALS_MEM_FAIL : IDASetLinearSolver(ida_mem, enginedata->linear_solver, NULL);
		} else if (strcmp(linsolver, "SPTFQMR") == 0) {
			MSG("IDA SPTFQMR");
#if SUNDIALS_VERSION_MAJOR >= 6
			enginedata->linear_solver = SUNLinSol_SPTFQMR(y0, prec ? SUN_PREC_LEFT : SUN_PREC_NONE, maxl, enginedata->sunctx);
#else
			enginedata->linear_solver = SUNLinSol_SPTFQMR(y0, prec ? SUN_PREC_LEFT : SUN_PREC_NONE, maxl);
#endif
			flag = enginedata->linear_solver == NULL ? IDALS_MEM_FAIL : IDASetLinearSolver(ida_mem, enginedata->linear_solver, NULL);
		} else {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unknown IDA linear solver choice '%s'",linsolver);
			return 8;
		}

		if (prec) {
			/* assign the preconditioner to the linear solver */
			(prec->pcreate)(integ);
			IDASetPreconditioner(ida_mem,prec->psetup,prec->psolve);
			MSG("PRECONDITIONER = %s",pname);
		} else {
			MSG("No preconditioner");
		}

		enginedata->flagfntype = "IDASPILS";
		enginedata->flagfn = &IDAGetLastLinFlag;
		enginedata->flagnamefn = &IDAGetLinReturnFlagName;
		if (flag == IDALS_MEM_NULL) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
			return 9;
		} else if (flag == IDALS_MEM_FAIL) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to allocate IDA linear solver");
			return 9;
		}


		/* assign the J*v function */
		if (SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_AUTODIFF)) {
			MSG("USING AUTODIFF");
			flag = IDASetJacTimes(ida_mem, NULL, &integrator_ida_jvex);
			if (flag < 0) {
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"ida_mem is NULL");
				return 10;
			}
		} else {
			MSG("USING NUMERICAL DIFF");
		}

		if (strcmp(linsolver, "SPGMR") == 0) {
			/* select Gram-Schmidt orthogonalisation */
			if (SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_GSMODIFIED)) {
				MSG("USING MODIFIED GS");
				flag = SUNLinSol_SPGMRSetGSType(enginedata->linear_solver, SUN_MODIFIED_GS);
				if (flag != SUNLS_SUCCESS) {
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 11;
				}
			} else {
				MSG("USING CLASSICAL GS");
				flag = SUNLinSol_SPGMRSetGSType(enginedata->linear_solver, SUN_CLASSICAL_GS);
				if (flag != SUNLS_SUCCESS) {
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to set GS_MODIFIED");
					return 11;
				}
			}
		}
	}

	/* set linear solver optional inputs...
	 ...nothing here at the moment...
	 */

	return 0;
} /* ida_set_optional_inputs */

/**
 * Calculate initial conditions using IDACalcIC
 */

int ida_setup_IC(IntegratorSystem *integ, void *ida_mem,
		realtype tout1, realtype t0, N_Vector y0, N_Vector yp0) {
	int i, flag = 0;
	long int flag1;
	int icopt; /* initial conditions strategy */
	N_Vector id;

	IntegratorIdaData *enginedata = integ->enginedata;

#ifdef SOLVE_DEBUG
	char *varname;
#endif

	icopt = 0;
	if (strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC), "Y") == 0) {
		MSG("Solving initial conditions using values of yddot");
		icopt = IDA_Y_INIT;
		asc_assert(icopt!=0);
	} else if (strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC), "YA_YDP")
			== 0) {
		MSG("Solving initial conditions using values of yd");
		icopt = IDA_YA_YDP_INIT;
		asc_assert(icopt!=0);
#if SUNDIALS_VERSION_MAJOR >= 6
		id = N_VNew_Serial(integ->n_y, enginedata->sunctx);
#else
		id = N_VNew_Serial(integ->n_y);
#endif
		for (i = 0; i < integ->n_y; ++i) {
			if (integ->ydot[i] == NULL) {
				NV_Ith_S(id, i) = 0.0;
#ifdef SOLVE_DEBUG
				varname = var_make_name(integ->system, integ->y[i]);
				MSG("y[%d] = '%s' is pure algebraic",i,varname);
				ASC_FREE(varname);
#endif
			} else {
#ifdef SOLVE_DEBUG
				MSG("y[%d] is differential",i);
#endif
				NV_Ith_S(id, i) = 1.0;
			}
		}
		IDASetId(ida_mem, id);
		N_VDestroy_Serial(id);
	} else if (strcmp(SLV_PARAM_CHAR(&integ->params,IDA_PARAM_CALCIC), "NONE")
			== 0) {
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Not solving initial conditions: check current residuals");
	} else {
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid 'iccalc' value: check solver parameters.");
	}

	if (icopt) {

		MSG("SOLVING INITIAL CONDITIONS IDACalcIC (tout1 = %f)", tout1);

#ifdef ASC_SIGNAL_TRAPS
		/* catch SIGFPE if desired to */
		if (enginedata->safeeval) {
			MSG("SETTING TO IGNORE SIGFPE...");
			Asc_SignalHandlerPush(SIGFPE, SIG_DFL);
		} else {
# ifdef FEX_DEBUG
			MSG("SETTING TO CATCH SIGFPE...");
# endif
			Asc_SignalHandlerPushDefault(SIGFPE);
		}
		if (setjmp(g_fpe_env) == 0) {
#endif

# if SUNDIALS_VERSION_MAJOR >= 5
			flag = IDACalcIC(ida_mem, icopt, tout1);/* new API from v2.3  */
# endif
			/* check flags and output status */
			switch (flag) {
			case IDA_SUCCESS:
				MSG("Initial conditions solved OK");
				break;

			case IDA_LSETUP_FAIL:
			case IDA_LINIT_FAIL:
			case IDA_LSOLVE_FAIL:
			case IDA_NO_RECOVERY:
				flag1 = -999;
				flag = (enginedata->flagfn)(ida_mem, &flag1);
				if (flag) {
					ERROR_REPORTER_HERE(ASC_PROG_ERR
							,"Unable to retrieve error code from %s (err %d)"
							,enginedata->flagfntype,flag
					);
					return 12;
				}
				ERROR_REPORTER_HERE(ASC_PROG_ERR
						,"%s returned flag '%s' (value = %d)"
						,enginedata->flagfntype,(enginedata->flagnamefn)(flag1),flag1
				);
				return 12;

			default:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve initial condition (IDACalcIC)");
				return 12;
			}
# if SUNDIALS_VERSION_MAJOR >= 5
			flag = IDAGetConsistentIC(ida_mem, y0, yp0);
			if(flag != IDA_SUCCESS){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to retrieve consistent initial conditions");
				return 14;
			}
			integrator_set_y(integ, NV_DATA_S(y0));
			integrator_set_ydot(integ, NV_DATA_S(yp0));
			flag = IDAReInit(ida_mem, t0, y0, yp0);
			if(flag != IDA_SUCCESS){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to reinitialise IDA with consistent initial conditions");
				return 15;
			}
# endif
	#ifdef ASC_SIGNAL_TRAPS
		} else {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Floating point error while solving initial conditions");
			return 13;
		}

		if (enginedata->safeeval) {
			Asc_SignalHandlerPop(SIGFPE, SIG_DFL);
		} else {
			MSG("pop...");
			Asc_SignalHandlerPopDefault(SIGFPE);
			MSG("...pop");
		}
#endif
	}/* icopt */

	return 0;
} /* ida_setup_IC */


int ida_root_init(IntegratorSystem *integ, void *ida_mem) {
	IntegratorIdaData *enginedata = integ->enginedata;

	if (enginedata->nbnds) {
#if SUNDIALS_VERSION_MAJOR >= 5
		IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn);
#endif
	}

	return 0;

}

/**
 * Allocate memory and prepare for integration.
 *
 * @param t	the first point at which a solution is desired. Required by IdaCalcIC
 *
 * @return 0 on success
 */
int ida_prepare_integrator(IntegratorSystem *integ, void *ida_mem,
		realtype tout1) {
	realtype t0;
	N_Vector y0, yp0;

	y0 	= ida_bnd_new_zero_NV(integ, integ->n_y);
	yp0 = ida_bnd_new_zero_NV(integ, integ->n_y);

	int i;
	double val;
	MSG("Values of the derivatives present in the model");
	for(i=0; i < integ->n_y; i++) {
		if(integ->ydot[i]){
			val = var_value(integ->ydot[i]);
			MSG("ydot[%d]= %g", i, val);
		}
	}

	t0 = integrator_get_t(integ);
	ida_retrieve_IVs(integ, t0, y0, yp0);

#ifdef IDA_BND_DEBUG
	MSG("Retrieved IVs before IDACalcIC: y0");
	N_VPrint_Serial(y0);
	MSG("Retrieved IVs before IDACalcIC: yp0");
	N_VPrint_Serial(yp0);
#endif

	/* allocate internal memory  */
	ida_malloc(integ, ida_mem, t0, y0, yp0);

	/* set optional inputs... */
	ida_set_optional_inputs(integ, ida_mem, y0);

	/* calculate initial conditions */
	ida_setup_IC(integ, ida_mem, tout1, t0, y0, yp0);

	/* specify ROOT-FINDING problem (if necessary) */
	ida_root_init(integ, ida_mem);

	/* Clean up */
	N_VDestroy_Serial(y0);
	N_VDestroy_Serial(yp0);
	return 0;
}

/**
 * Reinitialise IDA after boundary-crossing. We assume that IDAInit has been called.
 * Works with Sundials 2.4.0.
 *
 * @param t	the first point at which a solution is desired. Required by IdaCalcIC
 *
 * @return 0 on success
 */
int ida_reinit_integrator(IntegratorSystem *integ, void *ida_mem,
		realtype tout1) {
	realtype t0;
	N_Vector y0, yp0;

	y0 	= ida_bnd_new_zero_NV(integ, integ->n_y);
	yp0 = ida_bnd_new_zero_NV(integ, integ->n_y);

	t0 = integrator_get_t(integ);
	ida_retrieve_IVs(integ, t0, y0, yp0);

	int flag;

	flag = IDAReInit(ida_mem, t0, y0, yp0);
	if (flag!=IDA_SUCCESS) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR, "Reinitialisation failed.");
	}

	ida_set_optional_inputs(integ, ida_mem, y0);

	/* calculate initial conditions */
	ida_setup_IC(integ, ida_mem, tout1, t0, y0, yp0);

	ida_root_init(integ, ida_mem);

	/* Clean up */
	N_VDestroy_Serial(y0);
	N_VDestroy_Serial(yp0);
	return 0;
}

	/*-------------------------------------------------------------
	 MAIN IDA SOLVER ROUTINE, see IDA manual, sec 5.4, p. 27 ff.
	 */

/**
 Main IDA solver routine. We can assume that the integrator_ida_analyse
 function will already have been run.

 The presence of 'start_index' and 'finish_index' is a hangover from the
 Tcl/Tk GUI. We would like to get rid of them eventually.

 @return 0 on success */
static int integrator_ida_solve(IntegratorSystem *integ,
		unsigned long start_index, unsigned long finish_index) {
	void *ida_mem;
	int t_index;
	realtype t0, tout, tret, tol = 0.0001;
	N_Vector ypret, yret;
	IntegratorIdaData *enginedata;
	int i, flag = 0;

	int *rootsfound;			/** < IDA rootfinder reports root index in here */
	int *rootdir;				/** < Used to tell IDA to ignore doulve crossings */
	int *crossed_to_state;		/** < Boundary truth states immediately after the crossing */
	int *bnd_cond_states;		/** < Record of boundary states so that IDA can tell LRSlv
									   how to evaluate a boundary crossing */

	int	need_to_reconfigure;	/** < Flag to indicate system rebuild after crossing */
	int need_to_reinteg = 0;	/** < Flag for when crossings happen on or very close to timesteps */
	int	skipping_output;		/** < Flag to skip output to reporter */

#ifdef SOLVE_DEBUG
	char *relname;
#endif

	MSG("STARTING IDA...");
	/* Setup boundary list */
	enginedata = integrator_ida_enginedata(integ);
	enginedata->bndlist = slv_get_solvers_bnd_list(integ->system);
	enginedata->nbnds = slv_get_num_solvers_bnds(integ->system);
	enginedata->safeeval = SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_SAFEEVAL);
	MSG("safeeval = %d",enginedata->safeeval);



#ifdef SOLVE_DEBUG
	integrator_ida_debug(integ, stderr);
#endif


	/* Initialise boundary condition states if appropriate. Reconfigure if necessary */
	if (enginedata->nbnds) {
		bnd_cond_states = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);

		for (i = 0; i < enginedata->nbnds; i++) {
			bnd_cond_states[i] = bndman_calc_satisfied(enginedata->bndlist[i]);
		}
		ida_setup_lrslv(integ);

	}

	/* store reference to list of relations (in enginedata) */
		ida_load_rellist(integ);

	/* create IDA object */
#if SUNDIALS_VERSION_MAJOR >= 6
	ida_mem = IDACreate(enginedata->sunctx);
#else
	ida_mem = IDACreate();
#endif

	/* Setup parameter inputs and initial conditions for IDA. */
	tout = samplelist_get(integ->samples, start_index + 1);
	ida_prepare_integrator(integ, ida_mem, tout);



	/* -- set up the IntegratorReporter */
	integrator_output_init(integ);

	/* -- store the initial values of all the stuff */
	integrator_output_write(integ);
	integrator_output_write_obs(integ);

	/* specify where the returned values should be stored */
	yret 	= ida_bnd_new_zero_NV(integ, integ->n_y);
	ypret 	= ida_bnd_new_zero_NV(integ, integ->n_y);

	/* advance solution in time, return values as yret and derivatives as ypret */
	integ->currentstep = 1;
	for (t_index = start_index + 1; t_index <= finish_index; ++t_index, ++integ->currentstep) {
		tout = samplelist_get(integ->samples, t_index);
		t0 = integrator_get_t(integ);

		asc_assert(tout > t0);

#ifdef SOLVE_DEBUG
		MSG("Integrating from t0 = %f to t = %f", t0, tout);
#endif


		/**
		 * do { solve routine } while(need_to_reintegrate)
		 * If IDA detects a boundary sufficiently far from the desired output
		 * time tout, then the system is reconfigured and IDASolve is recalled
		 * up to the same tindex.
		 *
		 * @TODO "Sufficiently far" is currently tol = 0.0001, i.e complete arbitrary
		 * Should reflect IDACalcIC's requirements?
		 */

		do {
			if(need_to_reinteg) {
				MSG("Resuming integration from %f to %f", integrator_get_t(integ), tout);
				integrator_output_write(integ);
			}

			/* Control flags for boundary crossings */
			need_to_reinteg = 0;
			skipping_output = 0;

#ifdef ASC_SIGNAL_TRAPS
			Asc_SignalHandlerPushDefault(SIGINT);
			if (setjmp(g_int_env) == 0) {
#endif

				flag = IDASolve(ida_mem, tout, &tret, yret, ypret, IDA_NORMAL);
#ifdef ASC_SIGNAL_TRAPS
			} else {
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Caught interrupt");
				flag = -555;
			}
			Asc_SignalHandlerPopDefault(SIGINT);
#endif

			if (enginedata->nbnds) {


				if (flag == IDA_ROOT_RETURN) {
					MSG("IDA reports root found!");

					/* Store the root index */
					rootsfound = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);
					rootdir = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);
					crossed_to_state = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);

					if (IDA_SUCCESS != IDAGetRootInfo(ida_mem, rootsfound)) {
						ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to fetch boundary-crossing info");
						return 14;
					}
#ifdef SOLVE_DEBUG
					for (i = 0; i < enginedata->nbnds; i++) {

						if (rootsfound[i]) {
							relname = bnd_make_name(integ->system, enginedata->bndlist[i]);
							ERROR_REPORTER_HERE(ASC_PROG_WARNING,
									"Boundary '%s' crossed%s",relname,
									rootsfound[i]>0?" (increasing)":" (decreasing)");
							ASC_FREE(relname);
						}
					}
#endif
					need_to_reconfigure = ida_cross_boundary(integ, rootsfound,
							bnd_cond_states);

					if (need_to_reconfigure) {
						for(i = 0; i < enginedata->nbnds; ++i){
							crossed_to_state[i] = bnd_cond_states[i];
						}
						MSG("Boundaries were crossed; "
								"need to reinitialise solver...");
						/* so, now we need to restart the integration. we will assume that
						 everything changes: number of variables, etc, etc, etc. */

						/* First output data exactly on the boundary */
						 integrator_output_write(integ);
						 integrator_output_write_obs(integ);

						if (ida_bnd_event_iterate(integ, ida_mem, tout) != 0) {
							return 1;
						}

						/* Need to destroy and rebuild system */
						//IDAFree(ida_mem);
						//ida_mem = IDACreate();

						/* Are we sufficiently far from tout to continue
						 * integrating on this timestep? */
						if (fabs(tret - tout) > tol) {
							need_to_reinteg = 1;
						} else {
							/* Advance timestep, skip writing output once*/
							tout = samplelist_get(integ->samples, t_index + 1);
							skipping_output = 1;
						}

						/* n_y may have changed */
						N_VDestroy_Serial(yret);
						N_VDestroy_Serial(ypret);

						yret = ida_bnd_new_zero_NV(integ, integ->n_y);
						ypret = ida_bnd_new_zero_NV(integ, integ->n_y);

#if SUNDIALS_VERSION_MAJOR >= 5
						/* If the post-event state is still on a boundary, suppress
						 * the just-seen crossing direction to avoid an immediate
						 * double hit. Only do this when the post-event state is
						 * still on the new side of the crossed boundary; if REINIT
						 * has moved the system to the opposite side we must allow
						 * the next same-direction crossing. */
						for(i = 0; i < enginedata->nbnds; i++) {
							bnd_cond_states[i] = bndman_calc_satisfied(enginedata->bndlist[i]);
						}
						for(i = 0; i < enginedata->nbnds; i++) {
							if(rootsfound[i] != 0
								&& bnd_cond_states[i] == crossed_to_state[i]){
								rootdir[i] = rootsfound[i];
							}else{
								rootdir[i] = 0;
							}
						}

						IDASetRootDirection(ida_mem, rootdir);
#endif

					} /* need to reconfigure */
					ASC_FREE(rootsfound);
					ASC_FREE(rootdir);
					ASC_FREE(crossed_to_state);
				} /* IDA_ROOT_RETURN */
			} /* nbnds */

		} while (need_to_reinteg); /* end of solve time step */

		if (!skipping_output) {
			/* pass the values of everything back to the compiler */
			integrator_set_t(integ, (double) tret);
			integrator_set_y(integ, NV_DATA_S(yret));
			integrator_set_ydot(integ, NV_DATA_S(ypret));

			/* -- store the current values of all the stuff */
			integrator_output_write(integ);
			integrator_output_write_obs(integ);
		}

		if (flag < 0) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to solve t = %f (IDASolve), error %d", tout, flag);
			break;
		}


	}/* loop through next sample timestep */

	/* -- close the IntegratorReporter */
	integrator_output_close(integ);

	/* get optional outputs */
#ifdef STATS_DEBUG
	IntegratorIdaStats stats;
	if (IDA_SUCCESS == integrator_ida_stats(ida_mem, &stats)) {
		integrator_ida_write_stats(&stats);
	} else {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to fetch stats!?!?");
	}
#endif


	/* free solution memory */
	N_VDestroy_Serial(yret);
	N_VDestroy_Serial(ypret);

	/* free bnd states if appropriate */
	if (enginedata->nbnds) {
		ASC_FREE(bnd_cond_states);
	}

	/* free solver memory */
	IDAFree(&ida_mem);

	if (flag < -500) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Interrupted while attempting t = %f", tout);
		return -flag;
	}

	if (flag < 0) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Solving aborted while attempting t = %f", tout);
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
static int integrator_ida_stats(void *ida_mem, IntegratorIdaStats *s) {

	return IDAGetIntegratorStats(ida_mem, &s->nsteps, &s->nrevals, &s->nlinsetups
			,&s->netfails, &s->qlast, &s->qcur, &s->hinused
			,&s->hlast, &s->hcur, &s->tcur
	);
}

/* vim: set ts=4: */
