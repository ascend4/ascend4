/*	ASCEND modelling environment
	Copyright (C) 2007-2008 Carnegie Mellon University

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

	THIS IS STILL VERY MUCH UNDER DEVELOPMENT AND INCOMPLETE.

	The IPOPT solver is documented at http://projects.coin-or.org/Ipopt/
*//*
	ASCEND wrapper for IPOPT originally by John Pye, Jun 2007 onwards.
*/

#include <ascend/utilities/config.h>

#ifndef ASC_WITH_IPOPT
# error "ASC_WITH_IPOPT must be defined in order to build this." 
#endif

#include <math.h>

#include <ascend/solver/solver.h>

#include <ascend/system/calc.h>
#include <ascend/system/relman.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/block.h>

#include <ascend/utilities/ascConfig.h>
#include <ascend/utilities/ascPanic.h>
#include <ascend/utilities/ascMalloc.h>
#include <ascend/utilities/ascDynaLoad.h>
#include <ascend/utilities/mem.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/general/tm_time.h>
#include <ascend/general/env.h>

#include <ascend/general/ltmatrix.h>

#include <coin/IpStdCInterface.h>

ASC_DLLSPEC SolverRegisterFn ipopt_register;

/*------------------------------------------------------------------------------
  DATA STRUCTURES AND FORWARD DEFS
*/

/**
	Documentation of solver options for IPOPT is at
	http://www.coin-or.org/Ipopt/documentation/node1.html
*/
enum{
	/** ASCEND OPTIONS */
	ASCEND_PARAM_SAFEEVAL
	/** OUTPUT OPTIONS */
	,IPOPT_PARAM_PRINT_LEVEL	
	,IPOPT_PARAM_PRINT_USER_OPTIONS
	/** TERMINATION OPTIONS */
	,IPOPT_PARAM_TOL
	,IPOPT_PARAM_MAX_ITER
	,IPOPT_PARAM_MAX_CPU_TIME
	,IPOPT_PARAM_DIVERGING_ITERATES_TOL
	,IPOPT_PARAM_CONSTR_VIOL_TOL	
	,IPOPT_PARAM_DUAL_INFEASIBILITY_TOL
	,IPOPT_PARAM_ACCEPTABLE_TOL
	,IPOPT_PARAM_ACCEPTABLE_ITER
	/** LINEAR SOLVER OPTIONS */
	,IPOPT_PARAM_LINEAR_SOLVER
	/** BARRIER PARAMETER OPTIONS */
	,IPOPT_PARAM_MU_STRATEGY
	/** DERIVATIVE TEST OPTIONS */
	,IPOPT_PARAM_DERIVATIVE_TEST
	/** QUASI-NEWTON OPTIONS */
	,IPOPT_PARAM_HESS_APPROX
	/** OPTIONS COUNT */	
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
	Index m;                          /* number of constraints (excl the 'objective relation')*/

	Index nnzJ; /* number of non zeros in the jacobian of the constraints */
	Index nnzH; /* number of non-zeros in the hessian of the objective */

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

static SlvClientToken ipopt_create(slv_system_t server, int32 *statusindex){
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

	sys->rfilt.matchbits = (REL_INCLUDED |  REL_ACTIVE);
	sys->rfilt.matchvalue = (REL_INCLUDED | REL_ACTIVE);
	sys->vfilt.matchbits =  (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
	sys->vfilt.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

	sys->vlist = slv_get_solvers_var_list(server);
	sys->rlist = slv_get_solvers_rel_list(server);

	sys->rtot = slv_get_num_solvers_rels(server);
	sys->vtot = slv_get_num_solvers_vars(server);

	sys->obj = slv_get_obj_relation(server);

	sys->slv = server;

	/*char *tmp = rel_make_name(sys->slv,sys->obj);
	//CONSOLE_DEBUG("Objective relation is '%s'",tmp);
	ASC_FREE(tmp);*/

	//CONSOLE_DEBUG("There are %d constraint relations.", sys->rtot);

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

/**
	Update the solver status. FIXME can't we get rid of this silly function
	somehot?
 */
static void update_status(IpoptSystem *sys){
   boolean unsuccessful;

   sys->s.time_limit_exceeded = FALSE; /* can't do this one with IPOPT */
   sys->s.iteration_limit_exceeded = FALSE; /* IPOPT handles this one internally */

   unsuccessful = sys->s.diverged || sys->s.inconsistent ||
      sys->s.iteration_limit_exceeded || sys->s.time_limit_exceeded;

   sys->s.ready_to_solve = !unsuccessful && !sys->s.converged;
   sys->s.ok = !unsuccessful && sys->s.calc_ok && !sys->s.struct_singular;
}

static int32 ipopt_eligible_solver(slv_system_t server){
	struct rel_relation **rp;
	struct var_variable **vp;
	rel_filter_t rfilt;
	var_filter_t vfilt;

	rfilt.matchbits = (REL_CONDITIONAL |  REL_INWHEN);
	rfilt.matchvalue = (REL_CONDITIONAL | REL_INWHEN);

	vfilt.matchbits = (VAR_BINARY);
	vfilt.matchvalue = (VAR_BINARY);

	/// @todo check that there is a MAXIMIZE or MINIMIZE statement
	if (slv_get_obj_relation(server) == NULL)
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"No objective function found");

	/// @todo check that there are no WHENs or CONDITIONALs
	for( rp=slv_get_solvers_rel_list(server); *rp != NULL ; ++rp ) {
		if(rel_apply_filter(*rp,&rfilt)){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"WHEN and CONDITIONAL Statements are not supported.");
			return(FALSE);
		}
	}
	
	/// @todo check that there are no discrete-valued variables
	for( vp=slv_get_solvers_var_list(server); *vp != NULL ; ++vp ) {
		if(var_apply_filter(*vp,&vfilt)){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Discrete Variables not supported.");
			return(FALSE);
		}
	}
	
	/// @todo check anything else?

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

	/** ASCEND Options */

	slv_param_bool(parameters,ASCEND_PARAM_SAFEEVAL
		,(SlvParameterInitBool){{"safeeval"
			,"Use safe evaluation?",1
			,"Use 'safe' function evaluation routines (TRUE) or allow ASCEND to "
			"throw SIGFPE errors which will then halt integration (FALSE)."
		}, FALSE}
	);
	
	
	/** Output Options */

	slv_param_int(parameters,IPOPT_PARAM_PRINT_LEVEL
		,(SlvParameterInitInt){{"print_level"
			,"Output verbosity level",2
			,"Sets the default verbosity level for console output."
			 " The larger this value the more detailed is the output."
			 " Default value is 5 and range is 0 to 12."
		}, 5, 0, 12}
	);


	slv_param_char(parameters,IPOPT_PARAM_PRINT_USER_OPTIONS
		,(SlvParameterInitChar){{"print_user_options"
			,"Print all options set by the user.",2
			,"If selected, the algorithm will print the list of all options"
			 " set by the user including their values and whether they have"
			 " been used. In some cases this information might be incorrect,"
			 " due to the internal program flow. The default value for this "
			 " string option is 'no'. "
		}, "yes"}, (char *[]){
			"no","yes",NULL
		}
	); 



	/** Termination Options */

	slv_param_int(parameters,IPOPT_PARAM_MAX_ITER
		,(SlvParameterInitInt){{"max_iter"
			,"Maximum number of iterations",3
			,"The algorithm terminates with an error message if the number of iterations exceeded this number."
		}, 3000, 0, 100000000}
	);

	slv_param_real(parameters,IPOPT_PARAM_TOL
		,(SlvParameterInitReal){{"tol"
			,"Desired convergence tolerance (relative)",3
			,"Determines the convergence tolerance for the algorithm. The algorithm"
			" terminates successfully, if the (scaled) NLP error becomes smaller"
			" than this value, and if the (absolute) criteria according to "
			" 'dual_inf_tol', 'primal_inf_tol', and 'cmpl_inf_tol' are met. (This"
			" is epsilon_tol in Eqn. (6) in implementation paper). See also "
			" 'acceptable_tol' as a second termination criterion. Note, some other"
			" algorithmic features also use this quantity to determine thresholds"
			" etc."
		}, 1.e-8, 0, 1.e20}
	);

	slv_param_real(parameters,IPOPT_PARAM_MAX_CPU_TIME
		,(SlvParameterInitReal){{"max_cpu_time"
			,"Maximum CPU time allowed per problem (seconds)",3  
			,"The algorithm terminates with an error message if the CPU time exceeds this value."
		}, 1.e6, 0, 1.e7}
	);

	slv_param_real(parameters,IPOPT_PARAM_DIVERGING_ITERATES_TOL
		,(SlvParameterInitReal){{"diverging_iterates_tol"
			,"Threshold for maximal value of primal iterates",3
			,"If any component of the primal iterates exceeded this value" 
			 " (in absolute terms), the optimization is aborted with the "
			 "exit message that the iterates seem to be diverging"
		}, 1.e20, 0, 1.e50}
	);


	slv_param_real(parameters,IPOPT_PARAM_DUAL_INFEASIBILITY_TOL
		,(SlvParameterInitReal){{"dual_inf_tol"
			,"Desired threshold for the dual infeasibility.",3
			,"Absolute tolerance on the dual infeasibility. Successful " 
			 "termination requires that the max-norm of the (unscaled) "
			 "dual infeasibility is less than this threshold. The valid "
			 "range for this real option is 0 < dual_inf_tol < +inf and" 
			 " its default value is 1."
		}, 1, 0, 1.e50}
	);


	slv_param_real(parameters,IPOPT_PARAM_CONSTR_VIOL_TOL
		,(SlvParameterInitReal){{"constr_viol_tol"
			,"Desired threshold for the constraint violation.",3
			,"Absolute tolerance on the constraint violation. Successful" 
			 "termination requires that the max-norm of the (unscaled) "
			 " constraint violation is less than this threshold. The valid"
			 " range for this real option is 0 < constr_viol_tol < +inf and" 
			 " its default value is  0.0001"
		}, 1e-4, 0, 1.e50}
	);

	slv_param_real(parameters,IPOPT_PARAM_ACCEPTABLE_TOL
		,(SlvParameterInitReal){{"acceptable_tol"
			,"Acceptable convergence tolerance (relative).",3
			,"Determines which (scaled) overall optimality error is"
			" considered to be 'acceptable.' There are two levels of"
			" termination criteria. If the usual 'desired' tolerances"
			" (see tol, dual_inf_tol etc) are satisfied at an iteration,"
			" the algorithm immediately terminates with a success message."
			" On the other hand, if the algorithm encounters 'acceptable_iter'"
			" many iterations in a row that are considered 'acceptable', it will"
			" terminate before the desired convergence tolerance is met. This is"
			" useful in cases where the algorithm might not be able to achieve the"
			"'desired' level of accuracy. The valid range for this real option is "
			"0 < acceptable_tol < +inf and its default value is  1e-06"
		}, 1e-6, 0, 1.e50}
	);

	slv_param_int(parameters,IPOPT_PARAM_ACCEPTABLE_ITER
		,(SlvParameterInitInt){{"acceptable_iter"
			,"Num. of 'acceptable' iters before triggering stop.",3
			,"If the algorithm encounters this many successive 'acceptable' "
			 "iterates (see 'acceptable_tol'), it terminates, assuming that "
			 "the problem has been solved to best possible accuracy given round-off."
			 " If it is set to zero, this heuristic is disabled. The valid range for"
			 " this integer option is  0 < acceptable_iter  <  +inf and its default "
			 "value is  15."
		}, 15, 0, 100000000}
	);


	/** Linear Solver Options*/

	/* see http://www.coin-or.org/Ipopt/documentation/node139.html */
	slv_param_char(parameters,IPOPT_PARAM_LINEAR_SOLVER
		,(SlvParameterInitChar){{"linear_solver"
			,"Linear solver used for step computations.",4
			,"Determines which linear algebra package is to be used for the"
			" solution of the augmented linear system (for obtaining the search"
			" directions). Note, the code must have been compiled with the"
			" linear solver you want to choose. Depending on your Ipopt"
			" installation, not all options are available. The default value"
			" for this string option is 'ma27'."
			" Available options *may* include: ma27, ma57, pardiso, wsmp,"
			" mumps, custom."
		}, "mumps"}, (char *[]){
			"ma27","ma57","pardiso","wsmp","mumps","custom",NULL
		}
	); 


	/** Barrier Parameter Options*/

	slv_param_char(parameters,IPOPT_PARAM_MU_STRATEGY
		,(SlvParameterInitChar){{"mu_strategy"
			,"Update strategy for barrier parameter",5
			,"Determines which barrier parameter update strategy is to be used."
			" 'monotone' is the monotone (Fiacco-McCormick) strategy;"
			" 'adaptive' is the adaptive update strategy."
		}, "monotone"}, (char *[]){
			"monotone","adaptive",NULL
		}
	);

	/** Derivative Test Options */

	slv_param_char(parameters,IPOPT_PARAM_DERIVATIVE_TEST
		,(SlvParameterInitChar){{"derivative_test"
			,"Use Derivative Checker?",6
			,"A finite-difference derivative checker is provided by IPOPT, which"
			" will check Jacobian and gradient functions ('first-order') or"
			" all first-order derivatives as well as the Hessian matrix"
			" ('second-order'). The default is to perform no checks ('none')."
		}, "none"}, (char *[]){
			"none","first-order","second-order",NULL
		}
	);

	/** Quasi-Newton Options*/

	slv_param_char(parameters,IPOPT_PARAM_HESS_APPROX
		,(SlvParameterInitChar){{"hessian_approximation"
			,"Hessian calculation method",7
			,"Use either an exact Hessian matrix based on symbolic derivatives"
			" computed from the equations in the model ('exact'), or else use"
			" a limited-memory quasi-Newton approximation ('limited-memory')."
			" The default is 'limited-memory'."
		}, "exact"}, (char *[]){
			"exact","limited-memory",NULL
		}
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
	unsigned j;

	//CONSOLE_DEBUG("Updating Model ...");

	asc_assert(sys);
	asc_assert(sys->vlist);

	/* FIXME do we need to update any other stuff? */
	for(j = 0; j < sys->n; ++j){
		//CONSOLE_DEBUG("value of var[%d] = %g", j, x[j]);
	    	asc_assert(!isnan(x[j]));
		var_set_value(sys->vlist[j], x[j]);
	}

	return 0;
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

	//CONSOLE_DEBUG("ipopt_eval_f");

	asc_assert(n==sys->n);
	asc_assert(sys->obj!=NULL);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}

	sys->calc_ok = TRUE;


/*	char *relname;
	relname = rel_make_name(sys->slv,sys->obj);
	//CONSOLE_DEBUG("%s", relname);
	ascfree(relname);*/
	*obj_value = relman_eval(sys->obj,&(sys->calc_ok),SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL));
	asc_assert(!isnan(*obj_value));
	//CONSOLE_DEBUG("sys->obj_value = %g",*obj_value);
	//CONSOLE_DEBUG("done ipopt_eval_f");
	return sys->calc_ok;
}

/**
	@return 1 on success
*/
Bool ipopt_eval_grad_f(Index n, Number* x, Bool new_x, Number* grad_f, void *user_data){
	IpoptSystem *sys;
	int j, res, len;
	int count;
	double *derivatives;
	int *variables;

	sys = SYS(user_data);

	static var_filter_t vfilter = {
		VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED
		,VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR
	};

	//CONSOLE_DEBUG("ipopt_eval_grad_f");

	asc_assert(n==sys->n);
	asc_assert(sys->obj);
	asc_assert(sys->slv);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}


	/* evaluate grad_f(x) somehow */
	for(j=0; j<n; ++j){
		grad_f[j] = 0;
	}
	
	len = rel_n_incidences(sys->obj);
	variables = ASC_NEW_ARRAY_CLEAR(int,len);
	derivatives = ASC_NEW_ARRAY_CLEAR(double,len);
	/** @todo Check if memory allocation was successful and flag error if otherwise */
	//CONSOLE_DEBUG("Length of incidences: %d",len);
	//CONSOLE_DEBUG("allocated variables,derivatives");
	
	/*relman_diff2(
		sys->obj,&vfilter,derivatives,variables
		, &count,SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL)
	);*/

	relman_diff2_rev(
				 sys->obj,&vfilter,derivatives,variables
			, &count,SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL)
	);
	
	
	for(j=0; j<len; ++j){
		//asc_assert(!isnan(derivatives[j]));
		grad_f[variables[j]] = derivatives[j];
		char *tmp = var_make_name(sys->slv, sys->vlist[variables[j]]);
		//CONSOLE_DEBUG("var %d ('%s'): varindex = %d, x = %g, df/dx = %f", j, tmp, variables[j], var_value(sys->vlist[variables[j]]), derivatives[j]);
		ASC_FREE(tmp);
	}

	if(variables)ASC_FREE(variables);
	if(derivatives)ASC_FREE(derivatives);

	//CONSOLE_DEBUG("done ipopt_eval_grad_f");
	return 1; /* success, presumably */
}

Bool ipopt_eval_g(Index n, Number* x, Bool new_x, Index m, Number *g, void *user_data){
	IpoptSystem *sys;
	sys = SYS(user_data);
	int i, res;
	struct rel_relation *rel;
	int calc_ok = 1;

	//CONSOLE_DEBUG("ipopt_eval_g (n=%d, m=%d)",sys->n, sys->m);

	asc_assert(n==sys->n);
	asc_assert(m==sys->m);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}

	for(i=0;i<m;++i){
		//CONSOLE_DEBUG("rel %d: %s.",i,(sys->rlist[i] == sys->obj ? "OBJECTIVE" : "constraint")); //minor fix was rlist[0] -- MNM
	}

	/** @todo constraint rels are all relations except the objective rel. do we need to sort the objective to the end? */
	for(i=0; i<m; ++i){
		rel = sys->rlist[i];
		asc_assert(rel!=NULL);
		//if(rel == sys->obj) continue;	/* I think this completes the function for the time being */
		g[i] = relman_eval(rel, &calc_ok,SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL));
		asc_assert(!isnan(g[i]));
		//CONSOLE_DEBUG("g[%d] = %f",i,g[i]);
	}

	return calc_ok; /* fail: not yet implemented */
}

Bool ipopt_eval_jac_g(Index n, Number* x, Bool new_x, Index m
		, Index nele_jac, Index* iRow, Index *jCol, Number* values
		, void *user_data
){
	IpoptSystem *sys;
	sys = SYS(user_data);
	int i,res,j,k,len,count;
	struct var_variable **incidence_list;
	int *variables;
	double *derivatives; 

	static var_filter_t vfilter = {
		VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED,
		VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR
	};	


	//CONSOLE_DEBUG("ipopt_eval_jac_g... nnzJ = %d",sys->nnzJ);
	//CONSOLE_DEBUG("ipopt_eval_jac_g... n = %d",sys->n);
	//CONSOLE_DEBUG("ipopt_eval_jac_g... m = %d",sys->m);
	
	/*if(!values){
		//CONSOLE_DEBUG("sparsity structure requested, but not implemented");
		return 0; // failure 
	}*/

	asc_assert(sys!=NULL);
	asc_assert(n==sys->n);
	asc_assert(nele_jac==sys->nnzJ);
	asc_assert(m==sys->m);

	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}

	if(values == NULL){
		k=0;
		for(i=0; i<m;++i){
			incidence_list = (struct var_variable**) rel_incidence_list(sys->rlist[i]); 
			if(incidence_list!=NULL){
				len=rel_n_incidences(sys->rlist[i]);
				for(j=0;j<len;j++){
						iRow[k]=i; // should i use sindex of row here or is this ok?
						jCol[k++]=incidence_list[j]->sindex;
						//CONSOLE_DEBUG("Location of Non Zero: {%d,%d}; k = %d",i,incidence_list[j]->sindex,k);
				}		
			}
			else{
				ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Unused Relation???");
				return FALSE; //I'm not sure about the action to take.
			}
		}
		//CONSOLE_DEBUG("Finished Locating Non-Zero elements in Sparse Matrix");
	}	
	else{
		/** @todo Allocating and Deallocating memory for each row??? you must be out of your mind :O */
		k=0;
		variables = ASC_NEW_ARRAY(int,n);
		derivatives = ASC_NEW_ARRAY(double,n);
		for(i=0; i<m;++i){
			incidence_list = (struct var_variable**) rel_incidence_list(sys->rlist[i]);
			if(incidence_list!=NULL){
			      len = rel_n_incidences(sys->rlist[i]);
			      /*relman_diff2(sys->rlist[i],&vfilter,derivatives,variables
			      ,&count,SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL)
			      );*/
				  relman_diff2_rev(sys->rlist[i],&vfilter,derivatives,variables
						  ,&count,SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL)
						);
			      for(j=0;j<len;j++){
					asc_assert(!isnan(derivatives[j]));
				    values[k++] = derivatives[j];
			      }
			}
			else{
				ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Unused Relation???");
				return FALSE; //I'm not sure about the action to take.
			}
		}
		if(variables)ASC_FREE(variables);
		if(derivatives)ASC_FREE(derivatives);	
		//CONSOLE_DEBUG("Filled in values of Jacobian");
	}
	//CONSOLE_DEBUG("done ipopt_eval_jac_g");
	return TRUE;
}

Bool ipopt_eval_h(Index n, Number* x, Bool new_x
		, Number obj_factor, Index m, Number* lambda
		, Bool new_lambda, Index nele_hess, Index* iRow
		, Index* jCol, Number* values
		, void *user_data
){
	IpoptSystem *sys;
	sys = SYS(user_data);

	int res,count;

	struct var_variable **incidence_list;

	hessian_mtx *hess_matrix; 

	unsigned long i;	

	Index row;
	Index col;
	Index idx;


	static var_filter_t vfilter = {
		VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED,
 		VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR
	};	

	//CONSOLE_DEBUG("IN FUNCTION ipopt_eval_h");
	//CONSOLE_DEBUG("ipopt_eval_h... nnzH = %d",sys->nnzH);
	//CONSOLE_DEBUG("ipopt_eval_h... n = %d",sys->n);

	asc_assert(sys!=NULL);
	asc_assert(n==sys->n);
	asc_assert(nele_hess==sys->nnzH);
	
	if(new_x){
		res = ipopt_update_model(sys,x);
		if(res)return 0; /* fail model update */
	}

	if(values == NULL){
		asc_assert(iRow !=NULL && jCol != NULL);
		

		/* identify the sparsity structure of the Hessian (note: only the lower-
		left part is required by IPOPT , because the Hessian is symmetric) */
		//CONSOLE_DEBUG("Analysing of Hessian matrix sparsity structure not implemented");
		//CONSOLE_DEBUG("Dense Hessian Calculations being performed");
	
		idx = 0;

		for (row = 0; row < n; row++) {
			for (col = 0; col <= row; col++) {
				iRow[idx] = row; 
				jCol[idx] = col;
				idx++;
			}
		}
		asc_assert(idx == nele_hess);

	}
	else{
		asc_assert(jCol==NULL && iRow==NULL);
		asc_assert(lambda!=NULL); 
		
		/** Array of LT matrix */
		hess_matrix = Hessian_Mtx_create(Lower,n);
		
		//CONSOLE_DEBUG("Order of Hessian MATRIX [%d x %d]",n,n);
		
		/** Correction for objective function **/
		//CONSOLE_DEBUG("Correction for Objective Relation underway");
		relman_hess(sys->obj,&vfilter,hess_matrix,&count,n,SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL));
		
		idx = 0;

		for (row = 0; row < n; row++) {
			for (col = 0; col <= row; col++) {
				values[idx] = Hessian_Mtx_get_element(hess_matrix,row,col) * (obj_factor);
				idx++;
			}
		}
		asc_assert(idx == nele_hess);
		

		/** Correction for m-relations **/

	
		for(i=0; i<m; i++){
			/** @TODO Initialize the Hess Matrix Elements to zero */
			Hessian_Mtx_clear(hess_matrix);

			incidence_list = (struct var_variable**) rel_incidence_list(sys->rlist[i]);
			if(incidence_list!=NULL){
				//CONSOLE_DEBUG("Correction for Constraint Relation [%lu] underway",i);
				relman_hess(sys->rlist[i],&vfilter,hess_matrix,&count,n,SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL));
			
				idx=0;

				for (row = 0; row < n; row++) {
					for (col = 0; col <= row; col++) {
						values[idx] +=  Hessian_Mtx_get_element(hess_matrix,row,col) * (lambda[i]);
						idx++;
					}
				}
				asc_assert(idx == nele_hess);	

			}
			else{
				ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Unused Relation???");
				Hessian_Mtx_destroy(hess_matrix);
				return FALSE; //I'm not sure about the action to take.
			}
		}
		
		//CONSOLE_DEBUG("Hessian Matrix evaluation successful");

		Hessian_Mtx_destroy(hess_matrix);
		
		/* evaluate the Hessian matrix */
		//CONSOLE_DEBUG("Evaluation of Hessian matrix Completed");
	}
	
	return TRUE; /* fail: not yet implemented */
}

/*------------------------------------------------------------------------------
  SOLVE ROUTINES
*/

static int ipopt_presolve(slv_system_t server, SlvClientToken asys){
	IpoptSystem *sys;
	int max, i;
	struct var_variable *var;

	//CONSOLE_DEBUG("PRESOLVE");

	sys = SYS(asys);
	ipopt_iteration_begins(sys);
	//check_system(sys);

	asc_assert(sys->vlist && sys->rlist);

	/** @todo work out if matrix creation is not again needed */

	/** @todo slv_sort_rels_and_vars(server,&(sys->m),&(sys->n)); */


	/* count the number of optimisation variables */
	sys->n = 0;
	for(i = 0; i < sys->vtot; i++){
		var = sys->vlist[i];
		if(var_apply_filter(var,&(sys->vfilt))){
			sys->n++;
		}
	}

	/* set all relations as being 'unsatisfied' to start with... */
	for(i=0; i < sys->rtot; ++i){
		rel_set_satisfied(sys->rlist[i],FALSE);
	}

	sys->obj = slv_get_obj_relation(server); /*may have changed objective*/

	
	if(!sys->obj){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No objective function was specified");
		return -3;
	}
	//CONSOLE_DEBUG("got objective rel %p",sys->obj);
	/* @todo check if old_obj == obj ? */

	/* TODO are there cases where these should be different: answer: NO. they are always the same -- JP */
	sys->m = sys->rtot;
	//CONSOLE_DEBUG("Numbers of constraints = %d",sys->m);

	/** @todo we need to move the objective relation to the end of the list */

	/*for(i=0;i<sys->rtot-1;++i){
		//CONSOLE_DEBUG("%d",i);
		if(sys->rlist[i] == sys->obj)
			//CONSOLE_DEBUG("<-------------------------------This Check Works------------------------>");

	}*/

	//CONSOLE_DEBUG("got objective rel %p",sys->obj);

	/* calculate nnz for hessian matrix @todo FIXME */

	if(strcmp(SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_HESS_APPROX),"exact")==0){
		/** @todo fix rtot to be 'm' instead */
		sys->nnzH = ((sys->n)*((sys->n)+1))/2; //dense Hessian count
	}else{
		//CONSOLE_DEBUG("Skipping relman_hessian_count as hessian method is not exact.");
		//sys->nnzH = sys->n * sys->m;
	}

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
		//CONSOLE_DEBUG("this is a MINIMIZE problem");
	}else{
		//CONSOLE_DEBUG("this is a MAXIMIZE problem");
	}

	//CONSOLE_DEBUG("got %d constraints and %d vars in system", sys->m, sys->n);
	/* calculate number of non-zeros in the Jacobian matrix for the constraint equations */

	/* @todo make sure objective rel moved to end */
	sys->nnzJ = relman_jacobian_count(sys->rlist, sys->m, &(sys->vfilt), &(sys->rfilt), &max);
	/*sys->nnzJ=0;
	for(i=0;i<sys->m;++i){
		sys->nnzJ += rel_n_incidences(sys->rlist[i]);
	}*/

	//CONSOLE_DEBUG("got %d non-zeros in constraint Jacobian", sys->nnzJ);
	
	/* need to provide sparsity structure for jacobian? */



#if 0
	if(sys->presolved > 0) { /* system has been presolved before */
		if(!conopt_dof_changed(sys) /*no changes in fixed or included flags*/
		        && sys->p.partition == sys->J.old_partition
		        && sys->obj == sys->old_obj
		){
		    matrix_creation_needed = 0;
		    CONOPT_//CONSOLE_DEBUG("YOU JUST AVOIDED MATRIX DESTRUCTION/CREATION");
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
		CONOPT_//CONSOLE_DEBUG("FOUND %d CONSTRAINTS AND %d VARS",sys->con.m,sys->con.n);
		if (sys->obj != NULL) {
		    CONOPT_//CONSOLE_DEBUG("ADDING OBJECT AS A ROW");
		    sys->con.m++; /* treat objective as a row */
		}

		cntvect = ASC_NEW_ARRAY(int,COIDEF_Size());
		COIDEF_Ini(cntvect);
		sys->con.cntvect = cntvect;
		CONOPT_//CONSOLE_DEBUG("NUMBER OF CONSTRAINTS = %d",sys->con.m);
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
			CONOPT_//CONSOLE_DEBUG("SETTING OBJECTIVE CONSTRAINT TO BE %d",sys->con.objcon);
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
	//CONSOLE_DEBUG("sys->obj_val=%g",sys->obj_val);
	update_status(sys);

	ipopt_iteration_ends(sys);

	//CONSOLE_DEBUG("Reset status");

	/* sys->s.cost[sys->s.block.number_of].time=sys->s.cpu_elapsed; */

	//ERROR_REPORTER_HERE(ASC_USER_SUCCESS,"presolve completed");
	return 0;
}


static int ipopt_solve(slv_system_t server, SlvClientToken asys){
	IpoptSystem *sys;
	UNUSED_PARAMETER(server);
	enum ApplicationReturnStatus status;
	int ret, i, j;
	struct var_variable *var;
	enum rel_enum type_of_rel;
	sys = SYS(asys);

	double *x, *x_L, *x_U, *g_L, *g_U, *mult_x_L, *mult_x_U;

	//CONSOLE_DEBUG("SOLVING: sys->n = %d, sys->m = %d...",sys->n,sys->m);
	asc_assert(sys->n!=-1);

	/* set the number of variables and allocate space for the bounds */
	x_L = ASC_NEW_ARRAY(Number,sys->n);
	x_U = ASC_NEW_ARRAY(Number,sys->n);

	//CONSOLE_DEBUG("SETTING BOUNDS...");

	/* @todo set the values for the variable bounds */
	int jj = 0;
	for(j = 0; j < sys->vtot; j++){
		//CONSOLE_DEBUG("j = %d, vtot = %d, vlist = %p",j,sys->vtot,sys->vlist);
		var = sys->vlist[j];
		if(var_apply_filter(var,&(sys->vfilt))){
			//CONSOLE_DEBUG("setting x_L[%d] = %e",jj,var_lower_bound(var));
			assert(jj<sys->n);
			x_L[jj] = var_lower_bound(var);
			//CONSOLE_DEBUG("setting x_U[%d] = %e",jj,var_upper_bound(var));
			x_U[jj] = var_upper_bound(var);
			jj++;
		}
	}

	//CONSOLE_DEBUG("jj = %d, sys->n = %d", jj, sys->n);
	assert(jj==sys->n);

	/** @todo set bounds on the constraints? */
	/* is it possible to identify f(x)<a; f(x) >b and fold them into one? */
	/* then find the constant parts and make then g_L or g_U accordingly */
	/* what to do about other bounds? */
	/* set the number of variables and allocate space for the bounds */
	g_L = ASC_NEW_ARRAY(Number,sys->m);
	g_U = ASC_NEW_ARRAY(Number,sys->m);
	//CONSOLE_DEBUG("Allocated arrays for bounds of relations");
	if(g_L!=NULL && g_U!=NULL) 	
		for(j = 0; j < sys->m; j++){
			type_of_rel = rel_relop(sys->rlist[j]);
			if (type_of_rel == e_rel_less || type_of_rel == e_rel_lesseq){
				g_L[j] = -2.0e19;  //refer to IPOPT Manual "The C Interface"
				g_U[j] = 0;
			}
			else if (type_of_rel == e_rel_greatereq || type_of_rel == e_rel_greater){
				g_L[j] = 0;
				g_U[j] = 2.0e19;  //refer to IPOPT Manual "the C Interface"
			}	
			else{
				g_L[j] = 0;
				g_U[j] = 0;
			}
			//CONSOLE_DEBUG("set g_L[%d] = %e",j,g_L[j]);
			//CONSOLE_DEBUG("set g_U[%d] = %e",j,g_U[j]);
		}
	else
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to allocate arrays for bounds of relations");


	//CONSOLE_DEBUG("CREATING PROBLEM...");

	/* create the IpoptProblem */
	//CONSOLE_DEBUG("n = %d, m = %d, nnzJ = %d, nnzH = %d",sys->n, sys->m, sys->nnzJ, sys->nnzH);
	sys->nlp = CreateIpoptProblem(sys->n, x_L, x_U, sys->m, g_L, g_U, sys->nnzJ, sys->nnzH, 0/*index style=C*/, 
		&ipopt_eval_f, &ipopt_eval_g, &ipopt_eval_grad_f, 
		&ipopt_eval_jac_g, &ipopt_eval_h
	);

	//CONSOLE_DEBUG("FREEING INTERNAL STUFF");
  
	/* We can free the memory now - the values for the bounds have been
	copied internally in CreateIpoptProblem */
#if 0
	/* freeing this stuff seems to cause a crash...?!?!? */
	ASC_FREE(x_L);
	ASC_FREE(x_U);
	ASC_FREE(g_L);
	ASC_FREE(g_U);
#endif

	//CONSOLE_DEBUG("SETTING OPTIONS...");
	/* set some options */
	/** OUTPUT OPTIONS */
	AddIpoptIntOption(sys->nlp, "print_level", SLV_PARAM_INT(&(sys->p),IPOPT_PARAM_PRINT_LEVEL));
	AddIpoptStrOption(sys->nlp, "print_user_options", SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_PRINT_USER_OPTIONS));
	/** TERMINATION OPTIONS */
	AddIpoptIntOption(sys->nlp, "max_iter", SLV_PARAM_INT(&(sys->p),IPOPT_PARAM_MAX_ITER));
	AddIpoptNumOption(sys->nlp, "tol", SLV_PARAM_REAL(&(sys->p),IPOPT_PARAM_TOL));
	AddIpoptNumOption(sys->nlp, "max_cpu_time", SLV_PARAM_REAL(&(sys->p),IPOPT_PARAM_MAX_CPU_TIME));
	AddIpoptNumOption(sys->nlp, "diverging_iterates_tol", SLV_PARAM_REAL(&(sys->p),IPOPT_PARAM_DIVERGING_ITERATES_TOL));
	AddIpoptNumOption(sys->nlp, "dual_inf_tol", SLV_PARAM_REAL(&(sys->p),IPOPT_PARAM_DUAL_INFEASIBILITY_TOL));
	AddIpoptNumOption(sys->nlp, "constr_viol_tol", SLV_PARAM_REAL(&(sys->p),IPOPT_PARAM_CONSTR_VIOL_TOL));
	AddIpoptNumOption(sys->nlp, "acceptable_tol", SLV_PARAM_REAL(&(sys->p),IPOPT_PARAM_ACCEPTABLE_TOL));
	AddIpoptIntOption(sys->nlp, "acceptable_iter", SLV_PARAM_INT(&(sys->p),IPOPT_PARAM_ACCEPTABLE_ITER));
	/** BARRIER PARAMETER OTPIONS*/
	AddIpoptStrOption(sys->nlp, "mu_strategy", SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_MU_STRATEGY));
	/** DERIVATIVE TEST OPTIONS */
	AddIpoptStrOption(sys->nlp, "derivative_test", SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_DERIVATIVE_TEST));
	/** QUASI-NEWTON OPTIONS */
	AddIpoptStrOption(sys->nlp, "hessian_approximation", SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_HESS_APPROX));
	/** LINEAR SOLVER OPTIONS */
	AddIpoptStrOption(sys->nlp, "linear_solver", SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_LINEAR_SOLVER));
	

	//CONSOLE_DEBUG("Hessian method: %s",SLV_PARAM_CHAR(&(sys->p),IPOPT_PARAM_HESS_APPROX));

	//CONSOLE_DEBUG("number of vars n = %d, number of rels m = %d",sys->n, sys->m);

	/* initial values */
	x = ASC_NEW_ARRAY(Number, sys->n);
	/*setting initial values here.*/
	//CONSOLE_DEBUG("Setting starting values for free variables.");
	for(i=0;i<sys->n;++i){
		//CONSOLE_DEBUG("set x[%d] = %g",i,var_value(sys->vlist[i])); // need to set the default values
		x[i]=var_value(sys->vlist[i]);
	}
	/** @todo get values of 'x' from the model */

	/* allocate space to store the bound multipliers at the solution */
	mult_x_L = ASC_NEW_ARRAY(Number, sys->n);
	mult_x_U = ASC_NEW_ARRAY(Number, sys->n);

	//CONSOLE_DEBUG("Calling IpoptSolve...");

	//CONSOLE_DEBUG("sys->objval = %g", sys->obj_val);

	/* solve the problem */
	status = IpoptSolve(sys->nlp, x, NULL, &sys->obj_val, NULL, mult_x_L, mult_x_U, (void*)sys);

	//CONSOLE_DEBUG("Done IpoptSolve...");

	/** @todo update the sys->s.xxxxx flags based on value of 'status' */

	if (status == Solve_Succeeded) {
		sys->s.converged = TRUE;
		
		sys->s.block.current_block = -1; //is this 1??
		sys->s.cost = ASC_NEW_ARRAY(struct slv_block_cost,1);
		sys->s.cost->size=sys->s.block.current_size=sys->n;
		sys->s.cost->iterations=sys->s.block.iteration;
		sys->s.cost->funcs=sys->s.block.funcs;
		sys->s.cost->jacs=sys->s.block.jacs;
		sys->s.cost->time=sys->s.block.cpu_elapsed;
		sys->s.cost->functime=sys->s.block.functime;
		sys->s.cost->jactime=sys->s.block.jactime;
		

		//CONSOLE_DEBUG("Solution of the primal variables, x");
		for (i=0; i<sys->n; i++) {
			//CONSOLE_DEBUG("   x[%d] = %e\n", i, x[i]); 
		}		

		//CONSOLE_DEBUG("Solution of the bound multipliers, z_L and z_U");
		for (i=0; i<sys->n; i++) {
			//CONSOLE_DEBUG("   z_L[%d] = %e", i, mult_x_L[i]); 
		}
		for (i=0; i<sys->n; i++) {
			//CONSOLE_DEBUG("    z_U[%d] = %e", i, mult_x_U[i]); 
		}

		//CONSOLE_DEBUG("Objective value");
		//CONSOLE_DEBUG("    f(x*) = %e", sys->obj_val); 
		
		
		
		ret = 0; /* success */
		ipopt_iteration_ends(sys);
		update_status(sys);
	}else{
		//Treat all other cases here
		switch(status){
		case Solved_To_Acceptable_Level:
			/** @todo What should be done here? */
			ERROR_REPORTER_HERE(ASC_USER_NOTE,"Solved to acceptable level");	
			break;
		case Infeasible_Problem_Detected:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Infeasible Problem Detected");
			break;
		case Diverging_Iterates:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Diverging iterations found.");
			break;
		case User_Requested_Stop:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"User Requested Stop.");
			break;
		case Maximum_Iterations_Exceeded:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Maximum Iterations Exceeded.");
			break;
		case Restoration_Failed:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Restoration Failed.");
			break;
		case Error_In_Step_Computation:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Error in Step Computation.");
			break;
		case Maximum_CpuTime_Exceeded:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Maximum CPU Time exceeded.");
			break;
		case Not_Enough_Degrees_Of_Freedom:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Not enough degrees of freedom.");
			break;
		case Invalid_Problem_Definition:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Invalid problem definition.");
			break;
		case Invalid_Option:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Invalid Option.");
			break;
		case Invalid_Number_Detected:
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Invalid Number Detected.");
			break;
		case Unrecoverable_Exception:
			ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Unrecoverable_Exception.");
			break;
		case Insufficient_Memory:
			ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Insufficient Memory.");
			break;
		case Internal_Error:
			ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Internal Error.");
			break;
		}
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Failed solve, unknown status");
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
	//CONSOLE_DEBUG("ipopt_iterate about to call ipopt_solve...");
	return ipopt_solve(server,asys);
}

static int ipopt_resolve(slv_system_t server, SlvClientToken asys){
	IpoptSystem *sys;
  	sys = SYS(asys);	

	/** @todo if implementing this, use the 'warm start' thing in IPOPT */

	/** @todo provide initial values of the 'multipliers' */
	
	sys->resolve = 1; /* resolved recognized here */
	
	/* Reset status */
	sys->s.iteration = 0;
	sys->s.cpu_elapsed = 0.0;
	sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
	sys->s.block.previous_total_size = 0;
	
	/* go to first unconverged block */
	sys->s.block.current_block = -1;
	sys->s.block.current_size = 0;
	sys->s.calc_ok = TRUE;
	sys->s.block.iteration = 0;
	sys->obj_val =  MAXDOUBLE/2000.0;
	
	update_status(sys);
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
