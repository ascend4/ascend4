/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
	Connection of the IPOPT optimisation solver into ASCEND.

	THIS IS STILL VERY MUCH UNDER DEVELOPMENT AND INCOMPLETE. I'VE ACTUALLY
	ONLY JUST STARTED WRITING IT by starting with asc_tron.c and modifying.

	The IPOPT solver is documented at http://projects.coin-or.org/Ipopt/
*//*
	ASCEND wrapper for IPOPT originally by John Pye, Jun 2007.
*/

#include <utilities/config.h>

#ifndef ASC_WITH_IPOPT
# error "ASC_WITH_IPOPT must be defined in order to build this." 
#endif

#include <solver/solver.h>

#include <system/calc.h>
#include <system/relman.h>
#include <system/slv_stdcalls.h>
#include <system/block.h>

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascDynaLoad.h>
#include <utilities/mem.h>
#include <utilities/ascEnvVar.h>
#include <general/tm_time.h>
#include <general/env.h>

#include <ipopt/IpStdCInterface.h>

ASC_DLLSPEC SolverRegisterFn ipopt_register;

/*------------------------------------------------------------------------------
  DATA STRUCTURES AND FORWARD DEFS
*/

/**
	Documentation of solver options for IPOPT is at
	http://www.coin-or.org/Ipopt/documentation/node1.html
*/
enum{
	IPOPT_PARAM_TOL
	,IPOPT_PARAM_MAX_ITER
	,IPOPT_PARAM_SAFEEVAL
	,IPOPT_PARAM_MU_STRATEGY
	,IPOPT_PARAMS
};

#define SYS(s) ((IpoptSystem *)(s))

struct IpoptSystemStruct{

	/*
		Problem definition
	*/
	slv_system_t                slv;     /* slv_system_t back-link */
	struct rel_relation         *obj;    /* Objective function: NULL = none */
	struct rel_relation         *old_obj;/* Objective function: NULL = none */
	struct var_variable         **vlist; /* Variable list (NULL terminated) */
	struct rel_relation         **rlist; /* Relation list (NULL terminated) */

	var_filter_t vfilt;
	rel_filter_t rfilt;

	/*
		Solver information
	*/
	int32                  presolved;    /* ? Has the system been presolved */
	int32                  resolve;      /* ? Has the system been resolved */
	slv_parameters_t       p;            /* Parameters */
	slv_status_t           s;            /* Status (as of iteration end) */

	int32                  cap;          /* Order of matrix/vectors */
	int32                  rank;         /* Symbolic rank of problem */
	int32                  vused;        /* Free and incident variables */
	int32                  vtot;         /* length of varlist */
	int32                  rused;        /* Included relations */
	int32                  rtot;         /* length of rellist */
	double                 clock;        /* CPU time */

	int32 calc_ok;
	double obj_val;

	void *parm_array[IPOPT_PARAMS];
	struct slv_parameter pa[IPOPT_PARAMS];

	/*
		IPOPT DATA
	*/
	Index n;                          /* number of variables */
	Index m;                          /* number of constraints */

	int nnzJ; /* number of non zeros in the jacobian of the constraints */
	int nnzH; /* number of non-zeros in the hessian of the objective */

	Number* x_L;                  /* lower bounds on x */
	Number* x_U;                  /* upper bounds on x */
	Number* g_L;                  /* lower bounds on g */
	Number* g_U;                  /* upper bounds on g */

	IpoptProblem nlp;             /* IpoptProblem */

	enum ApplicationReturnStatus status; /* Solve return code */
	Number* x;                    /* starting point and solution vector */
	Number* mult_x_L;             /* lower bound multipliers at the solution */
	Number* mult_x_U;             /* upper bound multipliers at the solution */
	Index i;                      /* generic counter */
};

typedef struct IpoptSystemStruct IpoptSystem;

static int ipopt_get_default_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters);

static void ipopt_iteration_begins(IpoptSystem *sys);
static void ipopt_iteration_ends(IpoptSystem *sys);

/*------------------------------------------------------------------------------
  SYSTEM SETUP/DESTROY, STATUS AND SOLVER ELIGIBILITY
*/

static SlvClientToken ipopt_create(slv_system_t server, int32*statusindex){
	IpoptSystem *sys;

	sys = ASC_NEW_CLEAR(IpoptSystem);
	if(sys==NULL){
		*statusindex = 1;
		return sys;
	}

	sys->p.parms = sys->pa;
	sys->p.dynamic_parms = 0;
	ipopt_get_default_parameters(server,(SlvClientToken)sys,&(sys->p));
	sys->p.whose = (*statusindex);

	sys->presolved = 0;
	sys->resolve = 0;

	sys->n = -1;
	sys->m = -1;

	sys->s.ok = TRUE;
	sys->s.calc_ok = TRUE;
	sys->s.costsize = 0;
	sys->s.cost = NULL; /*redundant, but sanity preserving */
	sys->s.block.number_of = 1;
	sys->s.block.current_block = 0;
	sys->s.block.current_reordered_block = 0;
	sys->s.block.current_size = 0;
	sys->s.block.previous_total_size = 0;
	sys->s.block.iteration = 0;
	sys->s.block.funcs = 0;
	sys->s.block.jacs = 0;
	sys->s.block.cpu_elapsed = 0;
	sys->s.block.functime = 0;
	sys->s.block.jactime = 0;
	sys->s.block.residual = 0;

	sys->rfilt.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
	sys->rfilt.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
	sys->vfilt.matchbits =  (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
	sys->vfilt.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

	sys->vlist = slv_get_solvers_var_list(server);
	sys->rlist = slv_get_solvers_rel_list(server);

	sys->rtot = slv_get_num_solvers_rels(server);
	sys->vtot = slv_get_num_solvers_vars(server);

	sys->obj = slv_get_obj_relation(server);
	if(sys->vlist == NULL) {
		ASC_FREE(sys);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"IPOPT called with no variables.");
		*statusindex = -2;
		return NULL;
	}
	if(sys->rlist == NULL && sys->obj == NULL) {
		ASC_FREE(sys);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"IPOPT called with no relations or objective.");
		*statusindex = -1;
		return NULL;
	}
	
	/* do nothing with the objective list, pars, bounds, extrels, etc etc */

	slv_check_var_initialization(server);
	*statusindex = 0;
	return((SlvClientToken)sys);
}

static int32 ipopt_destroy(slv_system_t server, SlvClientToken asys){
	UNUSED_PARAMETER(server);
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"ipopt_destroy not implemented");
	return 1;
}	


static int ipopt_get_status(slv_system_t server, SlvClientToken asys
		,slv_status_t *status
){
	IpoptSystem *sys;
	(void)server;  /* stop gcc whine about unused parameter */

	sys = SYS(asys);
	//if (check_system(sys)) return 1;
	mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
	return 0;
}

static int32 ipopt_eligible_solver(slv_system_t server){
	UNUSED_PARAMETER(server);
	
	/// TODO check that there is a MAXIMIZE or MINIMIZE statement
	/// TODO check that there are no discrete-valued variables
	/// TODO check that there are no WHENs or CONDITIONALs
	/// TODO check anything else?

	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"ipopt_eligible_solver not implemented");
	return 1;
}

/*------------------------------------------------------------------------------
  SOLVER PARAMETERS
*/

static
int32 ipopt_get_default_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
	IpoptSystem *sys = NULL;
	struct slv_parameter *new_parms = NULL;
	int32 make_macros = 0;

	if(server != NULL && asys != NULL) {
		sys = SYS(asys);
		make_macros = 1;
	}

	if(parameters->parms == NULL) {
		new_parms = ASC_NEW_ARRAY_OR_NULL(struct slv_parameter,IPOPT_PARAMS);
		if(new_parms == NULL) {
		    return -1;
		}
		parameters->parms = new_parms;
		parameters->dynamic_parms = 1;
	}

	parameters->num_parms = 0;

	slv_param_int(parameters,IPOPT_PARAM_MAX_ITER
		,(SlvParameterInitInt){{"max_iter"
			,"Maximum number of iterations",2
			,"The algorithm terminates with an error message if the number of iterations exceeded this number."
		}, 3000, 0, 100000000}
	);

	slv_param_real(parameters,IPOPT_PARAM_TOL
		,(SlvParameterInitReal){{"tol"
			,"Desired convergence tolerance (relative)",2
			,"Determines the convergence tolerance for the algorithm. The algorithm"
			" terminates successfully, if the (scaled) NLP error becomes smaller"
			" than this value, and if the (absolute) criteria according to "
			" 'dual_inf_tol', 'primal_inf_tol', and 'cmpl_inf_tol' are met. (This"
			" is epsilon_tol in Eqn. (6) in implementation paper). See also "
			" 'acceptable_tol' as a second termination criterion. Note, some other"
			" algorithmic features also use this quantity to determine thresholds"
			" etc."
		}, 1e-8, 0, 1e20}
	);

	slv_param_char(parameters,IPOPT_PARAM_MU_STRATEGY
		,(SlvParameterInitChar){{"mu_strategy"
			,"Update strategy for barrier parameter",6
			,"Determines which barrier parameter update strategy is to be used."
			" 'monotone' is the monotone (Fiacco-McCormick) strategy;"
			" 'adaptive' is the adaptive update strategy."
		}, "monotone"}, (char *[]){
			"monotone","adaptive",NULL
		}
	);

	slv_param_bool(parameters,IPOPT_PARAM_SAFEEVAL
		,(SlvParameterInitBool){{"safeeval"
			,"Use safe evaluation?",1
			,"Use 'safe' function evaluation routines (TRUE) or allow ASCEND to "
			"throw SIGFPE errors which will then halt integration (FALSE)."
		}, FALSE}
	);

	asc_assert(parameters->num_parms==IPOPT_PARAMS);

	return 1;
}

static void ipopt_get_parameters(slv_system_t server, SlvClientToken asys
		, slv_parameters_t *parameters
){
	IpoptSystem *sys;
	UNUSED_PARAMETER(server);

	sys = SYS(asys);
	//if(check_system(sys)) return;
	mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}


static void ipopt_set_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
	IpoptSystem *sys;
	UNUSED_PARAMETER(server);

	sys = SYS(asys);
	//if (check_system(sys)) return;
	mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

/*------------------------------------------------------------------------------
  EVALUATION AND RESULT HOOK FUNCTIONS
*/

/**
	update the model with new 'x' vector.
	@return 0 on success.
*/
int ipopt_update_model(IpoptSystem *sys, const double *x){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return 1; /* error */
}

/** Function to evaluate the objective function f(x).
	@return 1 on success, 0 on failure

    @param n (in), the number of variables in the problem (dimension of 'x').
    @param x (in), the values for the primal variables, 'x' , at which 'f(x)' is to be evaluated.
    @param new_x (in), false if any evaluation method was previously called with the same values in 'x', true otherwise.
    @param obj_value (out) the value of the objective function ('f(x)').
*/
Bool ipopt_eval_f(Index n, Number *x, Bool new_x,  Number *obj_value, void *user_data){
	IpoptSystem *sys;
	sys = SYS(user_data);
	int res;

	asc_assert(n==sys->n);
	asc_assert(sys->obj!=NULL);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}

	sys->calc_ok = TRUE;

	*obj_value = relman_eval(sys->obj,&(sys->calc_ok),SLV_PARAM_BOOL(&(sys->p),IPOPT_PARAM_SAFEEVAL));

	return sys->calc_ok;
}

/**
	@return 1 on success
*/
Bool ipopt_eval_grad_f(Index n, Number* x, Bool new_x, Number* grad_f, void *user_data){
	IpoptSystem *sys;
	sys = SYS(user_data);
	int j, res, len;
	int count;
	double *derivatives;
	int *variables;
	static var_filter_t vfilter = {
		VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR
		,VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED
	};

	asc_assert(n==sys->n);
	asc_assert(sys->obj);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}


	/* evaluate grad_f(x) somehow */
	for(j=0; j<n; ++j){
		grad_f[j] = 0;
	}
	
    len = rel_n_incidences(sys->obj);
    variables = ASC_NEW_ARRAY(int,len);
    derivatives = ASC_NEW_ARRAY(double,len);

    relman_diff2(
        sys->obj,&vfilter,derivatives,variables
	    , &count,SLV_PARAM_BOOL(&(sys->p),IPOPT_PARAM_SAFEEVAL)
    );

	for(j=0; j<len; ++j){
		grad_f[variables[j]] = derivatives[j];
	}

	ASC_FREE(variables);
	ASC_FREE(derivatives);

	return 1; /* success, presumably */
}

Bool ipopt_eval_g(Index n, Number* x, Bool new_x, Index m, Number *g, void *user_data){
	IpoptSystem *sys;
	sys = SYS(user_data);
	int i, res;

	asc_assert(n==sys->n);
	asc_assert(m==sys->m);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}

	for(i=0; i<m; ++i){
		g[i] = 0;
	}

	return 0; /* fail: not yet implemented */
}

Bool ipopt_eval_jac_g(Index n, Number* x, Bool new_x, Index m
		, Index nele_jac, Index* iRow, Index *jCol, Number* values
		, void *user_data
){
	IpoptSystem *sys;
	sys = SYS(user_data);
	int i,res;

	asc_assert(n==sys->n);
	asc_assert(nele_jac==sys->nnzJ);
	asc_assert(m==sys->m);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}

	/* loop through the constraints */
	for(i=0; i<m; ++i){
		/* get derivatives for constraint i */
		/* insert the derivatives into the matrix in row i, columns j */
	}

	return 0; /* fail: not yet implemented */
}

Bool ipopt_eval_h(Index n, Number* x, Bool new_x
		, Number obj_factor, Index m, Number* lambda
		, Bool new_lambda, Index nele_hess, Index* iRow
		, Index* jCol, Number* values
		, void *user_data
){
	if(iRow != NULL){
		asc_assert(jCol !=NULL);
		asc_assert(x==NULL); asc_assert(lambda==NULL); asc_assert(values==NULL);

		/* identify the sparsity structure of the Hessian (note: only the lower-
		left part is required by IPOPT , because the Hessian is symmetric) */
		CONSOLE_DEBUG("Analysing of Hessian matrix sparsity structure not implemented");

		/*
		for(i=0; i<nvars; ++i){
			for(j=i; j<nvars; ++j){
				if(r2(sys->obj, sys->vlist[i], sys->vlist[j]) != 0){
					iRow[nele_hess] = i;
					jCol[nele_hess] = j;
					nele_hess++
				}
			}
		}
		*/								
				
	}else{
		asc_assert(jCol==NULL);
		asc_assert(x!=NULL); asc_assert(lambda!=NULL); asc_assert(values!=NULL);

		/* evaluate the Hessian matrix */
		CONSOLE_DEBUG("Evaluation of Hessian matrix not implemented");
	}
	
	return 0; /* fail: not yet implemented */
}

/*------------------------------------------------------------------------------
  SOLVE ROUTINES
*/

static int ipopt_presolve(slv_system_t server, SlvClientToken asys){
	IpoptSystem *sys;
	int max, i;

	CONSOLE_DEBUG("PRESOLVE");

	sys = SYS(asys);
	ipopt_iteration_begins(sys);
	//check_system(sys);

	asc_assert(sys->vlist && sys->rlist);

	/** @TODO work out if matrix creation is not again needed */

	/** @TODO slv_sort_rels_and_vars(server,&(sys->m),&(sys->n)); */

	/* set all relations as being 'unsatisfied' to start with... */
	for(i=0; i < sys->rtot; ++i){
		rel_set_satisfied(sys->rlist[i],FALSE);
	}

	sys->obj = slv_get_obj_relation(server); /*may have changed objective*/
	if(!sys->obj){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No objective function was specified");
		return -3;
	}

	CONSOLE_DEBUG("got objective rel %p",sys->obj);

	/* calculate nnz for hessian matrix @TODO FIXME */

	sys->nnzH = relman_hessian_count(sys->rlist, sys->rtot, &(sys->vfilt), &(sys->rfilt), &max);

	/* need to provide sparsity structure for hessian matrix? */

#if 0
	/** @SEE http://www.coin-or.org/Ipopt/documentation/node37.html */
	ipopt_eval_h(number_of_variables, NULL/*x at which to evaluate*/, TRUE /* new x */
			, 1.0/*obj_factor*/, number_of_constraints, lambda/* values of the constraint multipliers */
			, TRUE /* new lambda */, 0 /* number of nonzero elements in the Hessian */, Index* iRow
			, Index* jCol, Number* values
			, void *user_data
	);
#endif


	max = relman_obj_direction(sys->obj);
	if(max==-1){
		CONSOLE_DEBUG("this is a MINIMIZE problem");
	}else{
		CONSOLE_DEBUG("this is a MAXIMIZE problem");
	}

	CONSOLE_DEBUG("got %d relations and %d vars in system", sys->rtot, sys->vtot);
	/* calculate number of non-zeros in the Jacobian matrix for the constraint equations */

	sys->nnzJ = relman_jacobian_count(sys->rlist, sys->rtot, &(sys->vfilt), &(sys->rfilt), &max);

	CONSOLE_DEBUG("got %d non-zeros in constraint Jacobian", sys->nnzJ);
	
	/* need to provide sparsity structure for jacobian? */



#if 0
	if(sys->presolved > 0) { /* system has been presolved before */
		if(!conopt_dof_changed(sys) /*no changes in fixed or included flags*/
		        && sys->p.partition == sys->J.old_partition
		        && sys->obj == sys->old_obj
		){
		    matrix_creation_needed = 0;
		    CONOPT_CONSOLE_DEBUG("YOU JUST AVOIDED MATRIX DESTRUCTION/CREATION");
		}
	}
#endif

#if 0
	// check all this...

		sys->presolved = 1; /* full presolve recognized here */
		sys->resolve = 0;   /* initialize resolve flag */

		sys->J.old_partition = sys->p.partition;
		sys->old_obj = sys->obj;

		slv_sort_rels_and_vars(server,&(sys->con.m),&(sys->con.n));
		CONOPT_CONSOLE_DEBUG("FOUND %d CONSTRAINTS AND %d VARS",sys->con.m,sys->con.n);
		if (sys->obj != NULL) {
		    CONOPT_CONSOLE_DEBUG("ADDING OBJECT AS A ROW");
		    sys->con.m++; /* treat objective as a row */
		}

		cntvect = ASC_NEW_ARRAY(int,COIDEF_Size());
		COIDEF_Ini(cntvect);
		sys->con.cntvect = cntvect;
		CONOPT_CONSOLE_DEBUG("NUMBER OF CONSTRAINTS = %d",sys->con.m);
		COIDEF_NumVar(cntvect, &(sys->con.n));
		COIDEF_NumCon(cntvect, &(sys->con.m));
		sys->con.nz = num_jacobian_nonzeros(sys, &(sys->con.maxrow));
		COIDEF_NumNZ(cntvect, &(sys->con.nz));
		COIDEF_NumNlNz(cntvect, &(sys->con.nz));

		sys->con.base = 0;
		COIDEF_Base(cntvect,&(sys->con.base));
		COIDEF_ErrLim(cntvect, &(DOMLIM));
		COIDEF_ItLim(cntvect, &(ITER_LIMIT));

		if(sys->obj!=NULL){
			sys->con.optdir = relman_obj_direction(sys->obj);
			sys->con.objcon = sys->con.m - 1; /* objective will be last row */
			CONOPT_CONSOLE_DEBUG("SETTING OBJECTIVE CONSTRAINT TO BE %d",sys->con.objcon);
		}else{
			sys->con.optdir = 0;
			sys->con.objcon = 0;
		}
		COIDEF_OptDir(cntvect, &(sys->con.optdir));
		COIDEF_ObjCon(cntvect, &(sys->con.objcon));

		temp = 0;
		COIDEF_StdOut(cntvect, &temp);

		int debugfv = 1;
		COIDEF_DebugFV(cntvect, &debugfv);

		destroy_vectors(sys);
		destroy_matrices(sys);
		create_matrices(server,sys);
		create_vectors(sys);

		sys->s.block.current_reordered_block = -2;
	}

	//...

	if( matrix_creation_needed ) {
		destroy_array(sys->s.cost);
		sys->s.cost = create_zero_array(sys->s.costsize,struct slv_block_cost);
		for( ind = 0; ind < sys->s.costsize; ++ind ) {
		    sys->s.cost[ind].reorder_method = -1;
		}
	} else {
		reset_cost(sys->s.cost,sys->s.costsize);
	}

#endif

	/* Reset status */
	sys->s.iteration = 0;
	sys->s.cpu_elapsed = 0.0;
	sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
	sys->s.block.previous_total_size = 0;
	sys->s.costsize = 1+sys->s.block.number_of;


	/* set to go to first unconverged block */
	sys->s.block.current_block = -1;
	sys->s.block.current_size = 0;
	sys->s.calc_ok = TRUE;
	sys->s.block.iteration = 0;
	sys->obj_val =  MAXDOUBLE/2000.0;

	ipopt_iteration_ends(sys);

	CONSOLE_DEBUG("Reset status");

	/* sys->s.cost[sys->s.block.number_of].time=sys->s.cpu_elapsed; */

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"presolve completed");
	return 0;
}


static int ipopt_solve(slv_system_t server, SlvClientToken asys){
	IpoptSystem *sys;
	UNUSED_PARAMETER(server);
	enum ApplicationReturnStatus status;
	int ret, i, j;
	struct var_variable *var;

	sys = SYS(asys);

	double *x, *x_L, *x_U, *g_L, *g_U, *mult_x_L, *mult_x_U;

	CONSOLE_DEBUG("SOLVING...");

	/* set the number of variables and allocate space for the bounds */
	x_L = ASC_NEW_ARRAY(Number,sys->n);
	x_U = ASC_NEW_ARRAY(Number,sys->n);

	/* @TODO set the values for the variable bounds */
	int jj = 0;
	for(j = 0; j < sys->vtot; j++){
		var = sys->vlist[j];
		if(var_apply_filter(var,&(sys->vfilt))){
			x_L[jj] = var_lower_bound(var);
			x_U[jj] = var_upper_bound(var);
			jj++;
		}
	}

	/** @TODO set bounds on the constraints? */
	/* need to identify equations that share the same non-constant parts? */
	/* then find the constant parts and make then g_L or g_U accordingly */
	/* what to do about other bounds? */

	/* create the IpoptProblem */
	sys->nlp = CreateIpoptProblem(sys->n, x_L, x_U, sys->m, g_L, g_U, sys->nnzJ, sys->nnzH, 0/*index style=C*/, 
		&ipopt_eval_f, &ipopt_eval_g, &ipopt_eval_grad_f, 
		&ipopt_eval_jac_g, &ipopt_eval_h
	);
  
	/* We can free the memory now - the values for the bounds have been
	copied internally in CreateIpoptProblem */
	ASC_FREE(x_L);
	ASC_FREE(x_U);
	ASC_FREE(g_L);
	ASC_FREE(g_U);

	/* set some options */
	AddIpoptNumOption(sys->nlp, "tol", SLV_PARAM_BOOL(&(sys->p),IPOPT_PARAM_TOL));
	AddIpoptStrOption(sys->nlp, "mu_strategy", SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_MU_STRATEGY));

	/* initial values */
	x = ASC_NEW_ARRAY(Number, sys->n);

	/** @TODO get values of 'x' from the model */

	/* allocate space to store the bound multipliers at the solution */
	mult_x_L = ASC_NEW_ARRAY(Number, sys->n);
	mult_x_U = ASC_NEW_ARRAY(Number, sys->n);

	/* solve the problem */
	status = IpoptSolve(sys->nlp, x, NULL, &sys->obj_val, NULL, mult_x_L, mult_x_U, NULL);

	/** @TODO update the sys->s.xxxxx flags based on value of 'status' */

	if (status == Solve_Succeeded) {
		CONSOLE_DEBUG("Solution of the primal variables, x");
		for (i=0; i<sys->n; i++) {
			CONSOLE_DEBUG("   x[%d] = %e\n", i, x[i]); 
		}

		CONSOLE_DEBUG("Solution of the bound multipliers, z_L and z_U");
		for (i=0; i<sys->n; i++) {
			CONSOLE_DEBUG("   z_L[%d] = %e", i, mult_x_L[i]); 
		}
		for (i=0; i<sys->n; i++) {
			CONSOLE_DEBUG("    z_U[%d] = %e", i, mult_x_U[i]); 
		}

		CONSOLE_DEBUG("Objective value");
		CONSOLE_DEBUG("    f(x*) = %e", sys->obj_val); 

		ret = 0; /* success */
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed solve, unknown status");
		ret = 1; /* failure */
	}
 
	/* free allocated memory */
	FreeIpoptProblem(sys->nlp);
	ASC_FREE(x);
	ASC_FREE(mult_x_L);
	ASC_FREE(mult_x_U);

	return ret;
}

/**
	Prepare sys for entering an iteration, increasing the iteration counts
	and starting the clock.
*/
static void ipopt_iteration_begins(IpoptSystem *sys){
	sys->clock = tm_cpu_time();
	++(sys->s.block.iteration);
	++(sys->s.iteration);
}


/*
	Prepare sys for exiting an iteration, stopping the clock and recording
	the cpu time.
*/
static void ipopt_iteration_ends(IpoptSystem *sys){
	double cpu_elapsed;   /* elapsed this iteration */

	cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
	sys->s.block.cpu_elapsed += cpu_elapsed;
	sys->s.cpu_elapsed += cpu_elapsed;
}



static int ipopt_iterate(slv_system_t server, SlvClientToken asys){
	UNUSED_PARAMETER(server);
	CONSOLE_DEBUG("ITERATING...");
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return 1;
}

static int ipopt_resolve(slv_system_t server, SlvClientToken asys){
	UNUSED_PARAMETER(server);

	/* if implementing this, use the 'warm start' thing in IPOPT */

	/* provide initial values of the 'multipliers' */

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return 1;
}

static const SlvFunctionsT ipopt_internals = {
	67
	,"IPOPT"
	,ipopt_create
  	,ipopt_destroy
	,ipopt_eligible_solver
	,ipopt_get_default_parameters
	,ipopt_get_parameters
	,ipopt_set_parameters
	,ipopt_get_status
	,ipopt_solve
	,ipopt_presolve
	,ipopt_iterate
	,ipopt_resolve
	,NULL
	,NULL
	,NULL
};

int ipopt_register(void){
	return solver_register(&ipopt_internals);
}
