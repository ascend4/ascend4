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
	Connection of the TRON solver into ASCEND.

	THIS IS STILL VERY MUCH UNDER DEVELOPMENT AND INCOMPLETE. I'VE ACTUALLY
	ONLY JUST STARTED WRITING IT by starting with asc_conopt.c and modifying.
*//*
	originally by John Pye, Jun 2007.
*/

#include <math.h>
#include <ctype.h>

#include <solver/solver.h>
#include <system/slv_stdcalls.h>
#include <system/relman.h>

#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascDynaLoad.h>
#include <utilities/mem.h>
#include <utilities/ascEnvVar.h>
#include <general/tm_time.h>
#include <general/env.h>

/*
	'tron.h' declares what we expect to find in the DLL/SO that we will dlopen
	when this solver is loaded.
*/

#include "tron.h"

ASC_DLLSPEC SolverRegisterFn tron_register;

/*
  Output in user defined TRON subroutines
*/
#define TRON_DEBUG

#ifdef TRON_DEBUG
# define TRON_CONSOLE_DEBUG(...) CONSOLE_DEBUG(__VA_ARGS__)
#else
# define TRON_CONSOLE_DEBUG(...) (void)0
#endif

/*
  makes lots of extra spew
*/
#define DEBUG FALSE

#define TRONSYS(s)        ((TronSystem *)(s))

/* for integrity checks */
#define TRON_CREATED ((int32)813029392)
#define TRON_DESTROYED ((int32)103289182)

/*------------------------------------------------------------------------------
  DATA STRUCTURES
*/

enum{
	TRON_PARAM_ITERMAX
	,TRON_PARAM_MAXFEV
	,TRON_PARAM_FATOL
	,TRON_PARAM_FRTOL
	,TRON_PARAM_FMIN
	,TRON_PARAM_CGTOL
	,TRON_PARAM_GTOL
	,TRON_PARAM_TIMELIMIT
	,TRON_PARAM_SAFECALC
	,TRON_PARAMS
};

/**
	TRON's matrix storage format, for a square n-by-n matrix
*/
typedef struct{
	double *ltp; /**< lower triangular part, compressed column storage, dimension nnz + n*p. */
	double *diag; /**< diagonal elements, n elements */
	int *col_ptr; /** pointers to the columns, dimension n+1. The nonzeros in column j of L are in the
	       col_ptr(j), ... , col_ptr(j+1) - 1 positions of l. */
	int *row_ind; /** row indices for the strict lower
	       triangular part of L (ltp) in compressed column storage. */
} TronMatrix;

#define TRON_MATRIX_ARG(SYS,MAT) (SYS)->MAT.ltp,(SYS)->MAT.diag,(SYS)->MAT.col_ptr,(SYS)->MAT.row_ind

typedef struct{

	char task[60];
	slv_parameters_t       params;        /* Parameters */
	slv_status_t           status;        /* Status (as of iteration end) */
	slv_system_t           slv;           /* slv_system_t back-link */
	struct slv_parameter pa[TRON_PARAMS]; /* dunno... */

	/* Problem definition */
	struct rel_relation *obj;    /* Objective function: NULL = none */
	struct rel_relation *old_obj;/* Objective function: NULL = none */
	struct var_variable **vlist; /* Variable list (NULL terminated) */
	struct rel_relation **rlist; /* Relation list (NULL terminated) */
  	long vtot;                   /* length of varlist */
  	long rtot;                   /* length of rellist */
	int m, n;                    /* size of system matrix? */
	double objective;            /* value of objective function */
	int vused;                   /* Free and incident variables */
	int rused;                   /* Included relations */

	/* Solver information */
	int       created;    /* Whether the system been created */
	int     presolved;    /* Whether the system been presolved */
	int optimised;

	double *x;
	double *xupper; /* upper bounds on each variable */
	double *xlower; /* lower bounds on each variable */
	double *f;
	double *g;
	TronMatrix A, B, L;

	double *xc, *s, *dsave, *wa;
	int *indfree, *isave, *iwa;

	double                 clock;        /* CPU time */

} TronSystem;

/* this stuff was in the above as well */

#if 0
  struct update_data     update;       /* Jacobian frequency counters */
  int32                  cap;          /* Order of matrix/vectors */
  int32                  rank;         /* Symbolic rank of problem */
#endif

#if 0
  void *parm_array[TRON_PARAMS];
#endif

#if 0
  /*
    TRON DATA
  */
  struct tron_data con;
#endif

#if 0
  /*
    Calculated data (scaled)
  */
  struct jacobian_data   J;            /* linearized system */

  struct vec_vector     nominals;     /* Variable nominals */
  struct vec_vector     weights;      /* Relation weights */
  struct vec_vector     relnoms;      /* Relation nominals */
  struct vec_vector     variables;    /* Variable values */
  struct vec_vector     residuals;    /* Relation residuals */

  real64                 objective;    /* Objective function evaluation */
#endif

/*------------------*/

#if 0
/*
  Auxiliary structures
*/

struct update_data {
  int                    jacobian;   /* Countdown on jacobian updating */
  int                    weights;    /* Countdown on weights updating */
  int                    nominals;   /* Countdown on nominals updating */
  int                    relnoms;    /* Countdown on relnom updating */
  int                    iterative;  /* Countdown on iterative scale update */
};


/*
  varpivots, relpivots used only in optimizing, if we rewrite calc_pivots
  without them.
*/
struct jacobian_data {
  linsolqr_system_t      sys;          /* Linear system */
  mtx_matrix_t           mtx;          /* Transpose gradient of residuals */
  real64                 *rhs;         /* RHS from linear system */
  unsigned               *varpivots;   /* Pivoted variables */
  unsigned               *relpivots;   /* Pivoted relations */
  unsigned               *subregions;  /* Set of subregion indeces */
  dof_t                  *dofdata;     /* dof data pointer from server */
  mtx_region_t           reg;          /* Current block region */
  int32                  rank;         /* Numerical rank of the jacobian */
  enum factor_method     fm;           /* Linear factorization method */
  boolean                accurate;     /* ? Recalculate matrix */
  boolean                singular;     /* ? Can matrix be inverted */
  boolean                old_partition;/* old value of partition flag */
};
#endif

/* forward decls */

static void tron_initialize(TronSystem * sys);
static int check_system(TronSystem *sys);
static void iteration_begins(TronSystem *sys);
static void iteration_ends( TronSystem *sys);
static int32 tron_dof_changed(TronSystem *sys);
static void update_status( TronSystem *sys);
static boolean calc_objective( TronSystem *sys);
static boolean calc_residuals( TronSystem *sys);

/*------------------------------------------------------------------------------
  SOLVER PARAMETERS
*/

/*
these are the parameters that AMPL says it offers for the TRON solver:

itermax     the limit on the number of conjugate gradient iterations.
maxfev      the limit on the number of function evaluations
fatol       the absolute error desired in the function
frtol       the relative error desired in the function
fmin        a lower bound for the function
cgtol       convergence criteria for the conjugate gradient method
gtol        relative 2-norm of projected grdients; convergence criteria for the trust region method
*/

static
int32 tron_get_default_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
  TronSystem *sys = NULL;
  struct slv_parameter *new_parms = NULL;
  int32 make_macros = 0;

  if(server != NULL && asys != NULL) {
    sys = TRONSYS(asys);
    make_macros = 1;
  }

  if(parameters->parms == NULL) {
    new_parms = ASC_NEW_ARRAY_OR_NULL(struct slv_parameter,TRON_PARAMS);
    if(new_parms == NULL) {
      return -1;
    }
    parameters->parms = new_parms;
    parameters->dynamic_parms = 1;
  }

  parameters->num_parms = 0;
  asc_assert(TRON_PARAMS==7);

  slv_param_int(parameters,TRON_PARAM_ITERMAX
  	,(SlvParameterInitInt){{"itermax"
  		,"Max. CG iterations",1
  		,"Maximum number of conjugate gradient iterations"
  	}, 30, 1, 20000}
  );

  slv_param_int(parameters,TRON_PARAM_MAXFEV
  	,(SlvParameterInitInt){{"maxfev"
  		,"Max. function evaluations",1
  		,"Maximum number of function evaluations"
  	}, 30, 1, 20000}
  );

  slv_param_real(parameters,TRON_PARAM_FATOL
  	,(SlvParameterInitReal){{"fatol"
  		,"Error tolerance (absolute)",1
  		,"The absolute error desired in the function"
  	}, 1e-8, 1e-13, 100000.5}
  );

  slv_param_real(parameters,TRON_PARAM_FRTOL
  	,(SlvParameterInitReal){{"frtol"
  		,"Error tolerance (relative)",1
  		,"The relative error desired in the function"
  	}, 1e-8, 1e-13, 100000.5}
  );

  slv_param_real(parameters,TRON_PARAM_FMIN
  	,(SlvParameterInitReal){{"fmin"
  		,"Function lower bound",1
  		,"A lower bound for the function"
  	}, -1e8, 1e8, -1e8}
  );

  slv_param_real(parameters,TRON_PARAM_CGTOL
  	,(SlvParameterInitReal){{"cgtol"
  		,"Tolerance for CG convergence criteria",1
  		,"Convergence criteria for the conjugate gradient method"
  	}, 1e-8, 1e-13, 100000.5}
  );

  slv_param_real(parameters,TRON_PARAM_GTOL
  	,(SlvParameterInitReal){{"gtol"
  		,"Trust region tolerance",1
  		,"relative 2-norm of projected gradients; convergence criteria for the trust region method"
  	}, 1e-8, 1e-13, 100000.5}
  );

  slv_param_int(parameters,TRON_PARAM_TIMELIMIT
  	,(SlvParameterInitInt){{"timelimit"
  		,"time limit",1
  		,"time limit (seconds)"
  	}, 30, 1, 20000}
  );

  slv_param_bool(parameters,TRON_PARAM_SAFECALC
	,(SlvParameterInitBool){{"safecalc"
		,"safe calc",1
		,"safe floating-point operations"
	}, 0}
  );

  asc_assert(parameters->num_parms==TRON_PARAMS);

  return 1;
}

static void tron_get_parameters(slv_system_t server, SlvClientToken asys
		, slv_parameters_t *parameters
){
  TronSystem *sys;
  UNUSED_PARAMETER(server);

  sys = TRONSYS(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->params),parameters,sizeof(slv_parameters_t));
}

static void tron_set_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
  TronSystem *sys;
  UNUSED_PARAMETER(server);

  sys = TRONSYS(asys);
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->params),sizeof(slv_parameters_t));
}

/*-----------------------------------------------------------------------------
  SET-UP AND DESTROY SOLVER DATA
*/

static SlvClientToken tron_create(slv_system_t server, int32*statusindex){
  TronSystem *sys;

  sys = ASC_NEW_CLEAR(TronSystem);
  if(sys==NULL){
    *statusindex = 1;
    return sys;
  }

  sys->slv = server;
  sys->params.parms = sys->pa;
  sys->params.dynamic_parms = 0;
  tron_get_default_parameters(server,(SlvClientToken)sys,&(sys->params));

  sys->created = TRON_CREATED;
  sys->presolved = 0;

  sys->params.whose = (*statusindex);

  sys->status.ok = TRUE;
  sys->status.calc_ok = TRUE;
  sys->status.costsize = 0;
  sys->status.cost = NULL; /*redundant, but sanity-preserving */
  sys->vlist = slv_get_solvers_var_list(server);
  sys->rlist = slv_get_solvers_rel_list(server);
  sys->rtot = slv_get_num_solvers_rels(server);
  sys->vtot = slv_get_num_solvers_vars(server);

  sys->obj = slv_get_obj_relation(server);

  if (sys->vlist == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"TRON called with no variables.");
    *statusindex = -2;
    return NULL; /* prolly leak here */
  }
  if (sys->rlist == NULL && sys->obj == NULL) {
    ascfree(sys);
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"TRON called with no relations or objective.");
    *statusindex = -1;
    return NULL; /* prolly leak here */
  }

  /*
	apparently we don't give a damn about the objective list or the pars or
	bounds or extrels or any of the other crap.
  */

  slv_check_var_initialization(server);

  sys->xc = ASC_NEW_ARRAY(double,sys->n);
  sys->s = ASC_NEW_ARRAY(double,sys->n);
  sys->indfree = ASC_NEW_ARRAY(int,sys->n);
  sys->isave = ASC_NEW_ARRAY(int,sys->n*3);
  sys->dsave = ASC_NEW_ARRAY(double,sys->n*3);
  sys->wa = ASC_NEW_ARRAY(double,sys->n*7);
  sys->iwa = ASC_NEW_ARRAY(int,sys->n*3);

  *statusindex = 0; /* <-- what is that? */

  return((SlvClientToken)sys);
}


/**
	@TODO document this
*/
static int32 tron_destroy(slv_system_t server, SlvClientToken asys){
  TronSystem *sys;
  UNUSED_PARAMETER(server);

  sys = TRONSYS(asys);
  if (check_system(sys)) return 1;
  /* destroy_vectors(sys); */
  /* destroy_matrices(sys); */
  slv_destroy_parms(&(sys->params));
  sys->created = TRON_DESTROYED;
  if(sys->status.cost){
	ASC_FREE(sys->status.cost);
  }

  /* clear all the work arrays used by TRON */
#define F(AA) if(sys->AA)ASC_FREE(sys->AA)
  F(xc); F(s); F(indfree); F(isave); F(dsave); F(wa); F(iwa);
#undef F

  ascfree( (POINTER)asys );
  return 0;
}

#if 0
static void create_matrices(slv_system_t server, TronSystem *sys){
  sys->J.mtx = mtx_create();
  mtx_set_order(sys->J.mtx,sys->cap);
  structural_analysis(server,sys);
}


static void create_vectors(TronSystem *sys){
  sys->nominals.vec = create_array(sys->cap,real64);
  sys->nominals.rng = &(sys->J.reg.col);
  sys->weights.vec = create_array(sys->cap,real64);
  sys->weights.rng = &(sys->J.reg.row);
  sys->relnoms.vec = create_array(sys->cap,real64);
  sys->relnoms.rng = &(sys->J.reg.row);
  sys->variables.vec = create_array(sys->cap,real64);
  sys->variables.rng = &(sys->J.reg.col);
  sys->residuals.vec = create_array(sys->cap,real64);
  sys->residuals.rng = &(sys->J.reg.row);
}
#endif

#if 0
static void destroy_matrices( TronSystem *sys){
   if( sys->J.mtx ) {
     mtx_destroy(sys->J.mtx);
   }
}

static void destroy_vectors( TronSystem *sys){
   destroy_array(sys->nominals.vec);
   destroy_array(sys->weights.vec);
   destroy_array(sys->relnoms.vec);
   destroy_array(sys->variables.vec);
   destroy_array(sys->residuals.vec);
}
#endif

/*------------------------------------------------------------------------------
  HIGH-LEVEL SOLVE ROUTINES
*/

static int tron_presolve(slv_system_t server, SlvClientToken asys){
  struct rel_relation **rp;
  int32 ind;
  int32 matrix_creation_needed = 1;
  TronSystem *sys;

#if 0
  int32 cap;
  struct var_variable **vp;
  int *cntvect, temp;
#endif

  TRON_CONSOLE_DEBUG("PRESOLVE");

  sys = TRONSYS(asys);
  iteration_begins(sys);
  check_system(sys);
  if( sys->vlist == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Variable list was not set.");
    return -1;
  }
  if( sys->rlist == NULL && sys->obj == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Relation list and objective were not set.");
    return -2;
  }

  sys->obj = slv_get_obj_relation(server); /*may have changed objective*/

  if(!sys->obj){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"No objective function was specified");
	return -3;
  }

  if(sys->presolved > 0) { /* system has been presolved before */
    if(!tron_dof_changed(sys) /*no changes in fixed or included flags*/
      /*   && sys->params.partition == sys->J.old_partition
         && sys->obj == sys->old_obj */
    ){
      matrix_creation_needed = 0;
      TRON_CONSOLE_DEBUG("YOU JUST AVOIDED MATRIX DESTRUCTION/CREATION");
    }
  }

  rp=sys->rlist;
  for( ind = 0; ind < sys->rtot; ++ind ) {
    rel_set_satisfied(rp[ind],FALSE);
  }
  if( matrix_creation_needed ) {

#if 0
    cap = slv_get_num_solvers_rels(sys->slv);
    sys->cap = slv_get_num_solvers_vars(sys->slv);
    sys->cap = MAX(sys->cap,cap);
    vp=sys->vlist;
    for( ind = 0; ind < sys->vtot; ++ind ) {
      var_set_in_block(vp[ind],FALSE);
    }
    rp=sys->rlist;
    for( ind = 0; ind < sys->rtot; ++ind ) {
      rel_set_in_block(rp[ind],FALSE);
      rel_set_satisfied(rp[ind],FALSE);
    }
#endif

    sys->presolved = 1; /* full presolve recognized here */

#if 0
    sys->J.old_partition = sys->params.partition;
#endif
    sys->old_obj = sys->obj;

    slv_sort_rels_and_vars(server,&(sys->m),&(sys->n));
    TRON_CONSOLE_DEBUG("FOUND %d CONSTRAINTS AND %d VARS",sys->m,sys->n);
    if (sys->obj != NULL) {
      TRON_CONSOLE_DEBUG("ADDING OBJECT AS A ROW");
      sys->m++; /* treat objective as a row */
    }

    /* destroy_vectors(sys); */
    /* destroy_matrices(sys); */
    /* create_matrices(server,sys); */
    /* create_vectors(sys); */

    sys->status.block.current_reordered_block = -2;
  }

  /* Reset status */
  sys->optimised = 0;
  sys->status.iteration = 0;
  sys->status.cpu_elapsed = 0.0;
  sys->status.converged = sys->status.diverged = sys->status.inconsistent = FALSE;
  sys->status.block.previous_total_size = 0;
  sys->status.costsize = 1+sys->status.block.number_of;

  if( matrix_creation_needed ) {
#if 0
    destroy_array(sys->status.cost);
    sys->status.cost = create_zero_array(sys->status.costsize,struct slv_block_cost);
    for( ind = 0; ind < sys->status.costsize; ++ind ) {
      sys->status.cost[ind].reorder_method = -1;
    }
  } else {
    reset_cost(sys->status.cost,sys->status.costsize);
#endif
  }

  /* set to go to first unconverged block */
  sys->status.block.current_block = -1;
  sys->status.block.current_size = 0;
  sys->status.calc_ok = TRUE;
  sys->status.block.iteration = 0;
  sys->objective =  MAXDOUBLE/2000.0;

  update_status(sys);
  iteration_ends(sys);
  sys->status.cost[sys->status.block.number_of].time=sys->status.cpu_elapsed;

  return 0;
}

const char TRON_START[] = "START";
const char TRON_GH[] = "GH";
const char TRON_CONV[] = "CONV";

/**
	@TODO document this
*/
static int tron_iterate(slv_system_t server, SlvClientToken asys){
	TronSystem *sys;
	sys = TRONSYS(asys);

	if(server == NULL || sys==NULL) return -1;
	if(check_system(sys)) return -2;
	if(!sys->status.ready_to_solve){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not ready to solve.");
		return 1;
	}

	if(sys->status.block.current_block==-1) {
		tron_initialize(sys);
		sys->status.converged = sys->optimised;
		update_status(sys);
		/* do something with scaling here? calc_relnoms(sys)? */
	}

	iteration_begins(sys);

	/* may have changed objective (how does that happen, BTW?) */
	sys->obj = slv_get_obj_relation(server);

	if(sys->task[0]=='F' || strncmp(sys->task,TRON_START,5)==0){

		// Evaluate the function at x and store in f.

	}
	if(strncmp(sys->task,TRON_GH,2)==0 || strncmp(sys->task,TRON_START,5)==0){

		// Evaluate the gradient at x and store in g.

		// Evaluate the Hessian at x and store in compressed
		// column storage in (a,adiag,acol_ptr,arow_ind)
	}

	double frtol = SLV_PARAM_REAL(&(sys->params),TRON_PARAM_FRTOL);
	double fatol = SLV_PARAM_REAL(&(sys->params),TRON_PARAM_FATOL);
	double cgtol = SLV_PARAM_REAL(&(sys->params),TRON_PARAM_CGTOL);
	int itermax = SLV_PARAM_INT(&(sys->params),TRON_PARAM_ITERMAX);
	double fmin = SLV_PARAM_REAL(&(sys->params),TRON_PARAM_FMIN);
	double delta = SLV_PARAM_REAL(&(sys->params),TRON_PARAM_GTOL);
	/** @TODO fmin should be taken from the model declaration somehow, not a solar parameter. */

	DTRON(&(sys->n),sys->x,sys->xlower,sys->xupper,sys->f,sys->g
		,TRON_MATRIX_ARG(sys,A)
		,&frtol,&fatol,&fmin,&cgtol,&itermax,&delta,sys->task
		,TRON_MATRIX_ARG(sys,B)
		,TRON_MATRIX_ARG(sys,L)
		,sys->xc,sys->s,sys->indfree
		,sys->isave,sys->dsave,sys->wa,sys->iwa
	);

	if(strncmp(sys->task,TRON_CONV,4)==0){
		sys->status.converged = 1;
		TRON_CONSOLE_DEBUG("System has converged");
	}

	TRON_CONSOLE_DEBUG("DTRON status is '%s'",sys->task);

	calc_objective(sys);
#if 0
	calc_objectives(sys);
	sys->residuals.accurate = FALSE;
#endif
	calc_residuals(sys);
#if 0
	update_cost(sys);
#endif
	iteration_ends(sys);
	update_status(sys);

	return 0;
}

/**
	@TODO document this
*/
static int tron_solve(slv_system_t server, SlvClientToken asys){
	TronSystem *sys;
	int err = 0;
	sys = TRONSYS(asys);
	if(server == NULL || sys==NULL) return -1;
	if(check_system(sys)) return -2;
	while(sys->status.ready_to_solve)err = err | tron_iterate(server,sys);
	return err;
}

/*------------------------------------------------------------------------------
  INTEGRITY CHECKS
*/

/**
	Checks sys for NULL and for integrity.
*/
static int check_system(TronSystem *sys){

  if( sys == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL system handle");
    return 1;
  }

  switch( sys->created ) {
  case TRON_CREATED:
    return 0;
  case TRON_DESTROYED:
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System was recently destroyed.");
    return 1;
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System reused or never allocated.");
    return 1;
  }
}

/*------------------------------------------------------------------------------
  CALCULATION ROUTINES

	ok = calc_objective(sys)
	ok = calc_residuals(sys)
	ok = calc_J(sys)
	calc_nominals(sys)
	calc_weights(sys)
	scale_J(sys)
	scale_variables(sys)
	scale_residuals(sys)
*/

#if 0
/**
	Count jacobian elements and set max to the number of elements
	in the densest row
*/
static int32 num_jacobian_nonzeros(TronSystem *sys, int32 *max){
  int32 row, len, licn,c,count,row_max;
  struct rel_relation *rel;
  rel_filter_t rf;
  var_filter_t vf;
  const struct var_variable **list;

  rf.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rf.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  vf.matchbits =  (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
  vf.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

  licn = 0;
  *max = 0;
  row_max = sys->m;
  if (sys->obj != NULL) {
    row_max--;
  }
  /* replace at leisure with
   * relman_jacobian_count(sys->rlist,row_max,&vfilter,&rfilter,max);
   */
  for( row = 0; row < row_max; row++ ) {
    rel = sys->rlist[row];
    if (rel_apply_filter(rel,&rf)) { /* shouldn't be needed */
      len = rel_n_incidences(rel);
      list = rel_incidence_list(rel);
      count = 0;
      for (c=0; c < len; c++) {
	if( var_apply_filter(list[c],&vf) ) {
	  licn++;
	  count++;
	}
      }
      *max = MAX(*max,count);
    }
  }
  if (sys->obj != NULL) {
    rel = sys->obj;
    len = rel_n_incidences(rel);
    list = rel_incidence_list(rel);
    count = 0;
    for (c=0; c < len; c++) {
      if( var_apply_filter(list[c],&vf) ) {
	licn++;
	count++;
      }
    }
    *max = MAX(*max,count);
  }
  return licn;
}
#endif

/**
	Evaluate the objective function.
*/
static boolean calc_objective(TronSystem *sys){
	boolean calc_ok = TRUE;
	asc_assert(sys->obj!=NULL);
	if(sys->obj){
		sys->objective = relman_eval(sys->obj,&calc_ok,SLV_PARAM_BOOL(&(sys->params),TRON_PARAM_SAFECALC));
	}else{
		sys->objective = 0.0;
	}
	return calc_ok;
}

#if 0
/**
	Evaluate all objectives.
*/
static boolean calc_objectives( TronSystem *sys){
  int32 len,i;
  static rel_filter_t rfilter;
  struct rel_relation **rlist=NULL;
  rfilter.matchbits = (REL_INCLUDED);
  rfilter.matchvalue =(REL_INCLUDED);
  rlist = slv_get_solvers_obj_list(sys->slv);
  len = slv_get_num_solvers_objs(sys->slv);
  calc_ok = TRUE;
  for (i = 0; i < len; i++) {
    if (rel_apply_filter(rlist[i],&rfilter)) {
	  asc_assert(rlist[i]!=NULL);
      relman_eval(rlist[i],&calc_ok,SAFE_CALC);
if DEBUG
      if (calc_ok == FALSE) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"error in calc_objectives");
        calc_ok = TRUE;
      }
endif /* DEBUG */
    }
  }
  return calc_ok;
}
#endif

#if 0
/**
	Calculate all of the residuals in the current block and compute
	the residual norm for block status.

	@return true iff calculations preceded without error.
*/
static boolean calc_residuals( TronSystem *sys){
  int32 row;
  struct rel_relation *rel;
  double time0;

  if( sys->residuals.accurate ) return TRUE;

  calc_ok = TRUE;
  row = sys->residuals.rng->low;
  time0=tm_cpu_time();
  for( ; row <= sys->residuals.rng->high; row++ ) {
    rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
if DEBUG
    if (!rel) {
      int32r;
      r=mtx_row_to_org(sys->J.mtx,row);
      ERROR_REPORTER_HERE(ASC_PROG_ERR
		,"NULL relation found at row %d rel %d in calc_residuals!",(int)row,r
      );
    }
endif /* DEBUG */
    asc_assert(rel!=NULL);
    sys->residuals.vec[row] = relman_eval(rel,&calc_ok,SAFE_CALC);

    relman_calc_satisfied(rel,sys->params.tolerance.feasible);
  }
  sys->status.block.functime += (tm_cpu_time() -time0);
  sys->status.block.funcs++;
  square_norm( &(sys->residuals) );
  sys->status.block.residual = calc_sqrt_D0(sys->residuals.norm2);
  if(!calc_ok){
    TRON_CONSOLE_DEBUG("ERROR IN EVALUATION");
  }
  return(calc_ok);
}
#endif 

#if 0
/**
	Calculate the current block of the jacobian.
	It is initially unscaled.
*/
static boolean calc_J( TronSystem *sys){
  int32 row;
  var_filter_t vfilter;
  double time0;
  real64 resid;

  calc_ok = TRUE;
  vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);
  time0=tm_cpu_time();
  mtx_clear_region(sys->J.mtx,&(sys->J.reg));
  for( row = sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ ) {
    struct rel_relation *rel;
    rel = sys->rlist[row];
    relman_diffs(rel,&vfilter,sys->J.mtx,&resid,SAFE_CALC);
  }
  sys->status.block.jactime += (tm_cpu_time() - time0);
  sys->status.block.jacs++;

  if( --(sys->update.nominals) <= 0 ) sys->nominals.accurate = FALSE;
  if( --(sys->update.weights) <= 0 ) sys->weights.accurate = FALSE;

  return(calc_ok);
}
#endif

#if 0
/**
	Retrieve the nominal values of all of the block variables,
	and ensure that they are all strictly positive.
*/
static void calc_nominals( TronSystem *sys){
  int32 col;
  if( sys->nominals.accurate ) return;
  col = sys->nominals.rng->low;
  if(strcmp(SCALEOPT,"NONE") == 0 ||
     strcmp(SCALEOPT,"ITERATIVE") == 0){
    for( ; col <= sys->nominals.rng->high; col++ ) {
      sys->nominals.vec[col] = 1;
    }
  } else {
    for( ; col <= sys->nominals.rng->high; col++ ) {
      struct var_variable *var;
      real64 n;
      var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
      n = var_nominal(var);
      if( n <= 0.0 ) {
	if( n == 0.0 ) {
	  n = TOO_SMALL;

	  ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
	  FPRINTF(ASCERR,"Variable ");
	  print_var_name(ASCERR,sys,var);
	  FPRINTF(ASCERR," has nominal value of zero. Resetting to %g.\n",n);
      error_reporter_end_flush();

	  var_set_nominal(var,n);
	} else {
	  n =  -n;

	  ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
	  FPRINTF(ASCERR,"Variable ");
	  print_var_name(ASCERR,sys,var);
	  FPRINTF(ASCERR," has negative nominal value. Resetting to %g.\n",n);
      error_reporter_end_flush();

	  var_set_nominal(var,n);
	}
      }
if DEBUG
	  ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
      FPRINTF(ASCERR,"Column %d is");
      print_var_name(ASCERR,sys,var);
      FPRINTF(ASCERR,"\nScaling of column %d is %g\n",col,n);
      error_reporter_end_flush();
endif /* DEBUG */
      sys->nominals.vec[col] = n;
    }
  }
  square_norm( &(sys->nominals) );
  sys->update.nominals = UPDATE_NOMINALS;
  sys->nominals.accurate = TRUE;
}
#endif

#if 0
/**
	Calculate the weights of all of the block relations
	to scale the rows of the Jacobian.
*/
static void calc_weights( TronSystem *sys)
{
  mtx_coord_t nz;
  real64 sum;

  if( sys->weights.accurate )
    return;

  nz.row = sys->weights.rng->low;
  if(strcmp(SCALEOPT,"NONE") == 0 ||
     strcmp(SCALEOPT,"ITERATIVE") == 0) {
    for( ; nz.row <= sys->weights.rng->high; (nz.row)++ ) {
      sys->weights.vec[nz.row] = 1;
    }
  } else if (strcmp(SCALEOPT,"ROW_2NORM") == 0 ||
	     strcmp(SCALEOPT,"2NORM+ITERATIVE") == 0) {
    for( ; nz.row <= sys->weights.rng->high; (nz.row)++ ) {
      sum=mtx_sum_sqrs_in_row(sys->J.mtx,nz.row,&(sys->J.reg.col));
      sys->weights.vec[nz.row] = (sum>0.0) ? 1.0/calc_sqrt_D0(sum) : 1.0;
    }
  } else if (strcmp(SCALEOPT,"RELNOM") == 0 ||
	     strcmp(SCALEOPT,"RELNOM+ITERATIVE") == 0) {
    for( ; nz.row <= sys->weights.rng->high; (nz.row)++ ) {
      sys->weights.vec[nz.row] =
	1.0/rel_nominal(sys->rlist[mtx_row_to_org(sys->J.mtx,nz.row)]);
    }
  }
  square_norm( &(sys->weights) );
  sys->update.weights = UPDATE_WEIGHTS;
  sys->residuals.accurate = FALSE;
  sys->weights.accurate = TRUE;
}
#endif


/**
	Reset all flags to setup a new solve.
	Should set sys->status.block.current_block = -1
	before calling.

	@TODO This is currently a HACK! Not sure if should call when done.
*/
void tron_initialize( TronSystem *sys){

  sys->status.block.current_block++;
  /*
   * Next line was added to create the aray cost, whis is used by
   * the interface to display residuals and number of iterations
   */
  sys->status.costsize = 1+sys->status.block.number_of;

  if( sys->status.block.current_block < sys->status.block.number_of ) {
    boolean ok;

    sys->status.block.iteration = 0;
    sys->status.block.cpu_elapsed = 0.0;
    sys->status.block.functime = 0.0;
    sys->status.block.jactime = 0.0;
    sys->status.block.funcs = 0;
    sys->status.block.jacs = 0;

    sys->status.calc_ok = TRUE;

    if( !(ok = calc_objective(sys)) ) {
         ERROR_REPORTER_HERE(ASC_PROG_ERR,"Objective calculation errors detected.");
    }

#if 0
    sys->status.calc_ok = sys->status.calc_ok && ok;

    if(!(sys->params.ignore_bounds) ) {
      slv_ensure_bounds(
          sys->slv, sys->J.reg.col.low,
          sys->J.reg.col.high,MIF(sys)
      );
    }

    sys->residuals.accurate = FALSE;
    if( !(ok = calc_residuals(sys)) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Residual calculation errors detected.");
    }

    sys->status.calc_ok = sys->status.calc_ok && ok;

    /* Must be updated as soon as required */
    sys->J.accurate = FALSE;
    sys->update.weights = 0;
    sys->update.nominals = 0;
    sys->update.relnoms = 0;
    sys->update.iterative = 0;
    sys->variables.accurate = FALSE;
#endif
  }
}


/*------------------------------------------------------------------------------
  ITERATION BEGIN/END ROUTINES
*/

/**
	Prepare sys for entering an iteration, increasing the iteration counts
	and starting the clock.
*/
static void iteration_begins(TronSystem *sys){
   sys->clock = tm_cpu_time();
   ++(sys->status.block.iteration);
   ++(sys->status.iteration);
}


/*
	Prepare sys for exiting an iteration, stopping the clock and recording
	the cpu time.
*/
static void iteration_ends( TronSystem *sys){
   double cpu_elapsed;   /* elapsed this iteration */

   cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
   sys->status.block.cpu_elapsed += cpu_elapsed;
   sys->status.cpu_elapsed += cpu_elapsed;
}


/**
	Update the solver status.
 */
static void update_status( TronSystem *sys){
   boolean unsuccessful;

#if 0
   if( !sys->status.converged ) {
      sys->status.time_limit_exceeded =
         (sys->status.block.cpu_elapsed >= TIME_LIMIT);
      sys->status.iteration_limit_exceeded =
         (sys->status.block.iteration >= ITER_LIMIT);
   }
#endif

   unsuccessful = sys->status.diverged || sys->status.inconsistent ||
      sys->status.iteration_limit_exceeded || sys->status.time_limit_exceeded;

   sys->status.ready_to_solve = !unsuccessful && !sys->status.converged;
   sys->status.ok = !unsuccessful && sys->status.calc_ok && !sys->status.struct_singular;
}

/*-----------------------------------------------------------------------------
  EXTERNAL ROUTINES (see slv_client.h)
*/

static int32 tron_eligible_solver(slv_system_t server){
	struct rel_relation **rp;
	for( rp=slv_get_solvers_rel_list(server); *rp != NULL ; ++rp ) {
		if( rel_less(*rp) || rel_greater(*rp) ){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"less-than and greater-than relations are not permitted with TRON");
			return(FALSE);
		}
	}
	return(TRUE);
}


static int tron_get_status(slv_system_t server, SlvClientToken asys
		,slv_status_t *status
){
	TronSystem *sys;
	UNUSED_PARAMETER(server);

	sys = TRONSYS(asys);
	if (check_system(sys)) return 1;
	mem_copy_cast(&(sys->status),status,sizeof(slv_status_t));
	return 0;
}

#if 0
/**
	Perform structural analysis on the system, setting the flags in
	status.

	The problem must be set up, the relation/variable list
	must be non-NULL. The jacobian (linear) system must be created
	and have the correct order (stored in sys->cap).  Everything else
	will be determined here.

	On entry there isn't yet a correspondence between var_sindex and
	jacobian column. Here we establish that.

	@NOTE this function has been striped of its guts for TRON and may go away
*/
static void structural_analysis(slv_system_t server, TronSystem *sys){

  var_filter_t vfilter;
  rel_filter_t rfilter;

  /*
   * The server has marked incidence flags already.
   */
  /* count included equalities */
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  sys->rused = slv_count_solvers_rels(server,&rfilter);

  /* count free and incident vars */
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  sys->vused = slv_count_solvers_vars(server,&vfilter);

  /* Symbolic analysis */
  sys->rtot = slv_get_num_solvers_rels(server);
  sys->vtot = slv_get_num_solvers_vars(server);

  /*
   * The next few lines are used to calculate the rank of the nonlinear
   * system. We need it to evaluate if the system is structurally
   * singular or not. Calculating this number will keep TRON from
   * displaying a "structurally singular" error message
   */
  if (sys->rtot) {
    slv_block_partition(server);
  }
  sys->J.dofdata = slv_get_dofdata(server);
  sys->rank = sys->J.dofdata->structural_rank;
  /*
   * Unify the partitions since we feed TRON with a single block.
   */
  slv_block_unify(server);


  sys->J.reg.row.low = sys->J.reg.col.low = 0;
  sys->J.reg.row.high = sys->m - 1;
  if (sys->obj != NULL) sys->J.reg.row.high--;
  sys->J.reg.col.high = sys->n - 1;

  if(slv_check_bounds(sys->slv,sys->vused,-1,"fixed ")){
    sys->status.inconsistent = 1;
  }

  /* Initialize Status */
  sys->status.over_defined = (sys->rused > sys->vused);
  sys->status.under_defined = (sys->rused < sys->vused);
  sys->status.struct_singular = (sys->rank < sys->rused);
  sys->status.block.number_of = (slv_get_solvers_blocks(sys->slv))->nblocks;

}
#endif

/**
	Check if any fixed or included flags have
	changed since the last presolve.
*/
static int32 tron_dof_changed(TronSystem *sys){
  int32 ind, result = 0;
  /* Currently we have two copies of the fixed and included flags
     which must be kept in sync.  The var_fixed and rel_included
     functions perform the syncronization and hence must be called
     over the whole var list and rel list respectively.  When we move
     to using only one set of flags (bit flags) this function can
     be changed to return 1 at the first indication of a change
     in the dof. */

  /* search for vars that were fixed and are now free */
  for( ind = sys->vused; ind < sys->vtot; ++ind ) {
    if( !var_fixed(sys->vlist[ind]) && var_active(sys->vlist[ind]) ) {
      ++result;
    }
  }
  /* search for rels that were unincluded and are now included */
  for( ind = sys->rused; ind < sys->rtot; ++ind ) {
    if( rel_included(sys->rlist[ind]) && rel_active(sys->rlist[ind])) {
      ++result;
    }
  }
  /* search for vars that were free and are now fixed */
  for( ind = sys->vused -1; ind >= 0; --ind ) {
    if( var_fixed(sys->vlist[ind]) ||  !var_active(sys->vlist[ind])) {
      ++result;
    }
  }
  /* search for rels that were included and are now unincluded */
  for( ind = sys->rused -1; ind >= 0; --ind ) {
    if( !rel_included(sys->rlist[ind]) || !rel_active(sys->rlist[ind]) ) {
      ++result;
    }
  }
  return result;
}

#if 0
static void reset_cost(struct slv_block_cost *cost,int32 costsize){
  int32 ind;
  for( ind = 0; ind < costsize; ++ind ) {
    cost[ind].size = 0;
    cost[ind].iterations = 0;
    cost[ind].funcs = 0;
    cost[ind].jacs = 0;
    cost[ind].functime = 0;
    cost[ind].jactime = 0;
    cost[ind].time = 0;
    cost[ind].resid = 0;
  }
}
#endif

#if 0
/**
	Update the values of the array cost, which is used by the interface
	to display residual and number of iterations. For use after running CONOPT
*/
static void update_cost(TronSystem *sys)
{
  int32 ci;
  if (sys->status.cost == NULL) {
    sys->status.cost = create_zero_array(sys->status.costsize,struct slv_block_cost);
  } else {
    reset_cost(sys->status.cost,sys->status.costsize);
  }
  ci=sys->status.block.current_block;
  sys->status.cost[ci].size	= sys->status.block.current_size;
  sys->status.cost[ci].iterations = sys->status.block.iteration;
  sys->status.cost[ci].resid	= sys->status.block.residual;
}
#endif

/*------------------------------------------------------------------------------
  LOADING TRON FROM SHARED LIBRARY, IF AVAILABLE
*/

typedef struct{
    dtron_fn_t *dtron_ptr;
} DTronFns;

DTronFns tron_fptrs;

int tron_dlopen(){
	static int loaded=0;
	char *libpath;
	int status;
	char fnsymbol[400], *c;
	const char *libname=ASC_TRON_LIB;
	const char *envvar;

	if(loaded) {
		return 0; /* already loaded */
	}

	CONSOLE_DEBUG("LOADING TRON...");

	envvar  = ASC_TRON_ENVVAR;

	/* need to import this variable into the ascend 'environment' */
	if(-1!=env_import(ASC_TRON_ENVVAR,getenv,Asc_PutEnv)){
		CONSOLE_DEBUG("Searching in path '%s' (from env var '%s')",getenv(envvar),envvar);
	}
	libpath = SearchArchiveLibraryPath(libname, ASC_TRON_DLPATH, envvar);

	if(libpath==NULL){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERR
			, "Library '%s' could not be located (check value of env var '%s' and/or default path '%s')"
			, libname, envvar, ASC_TRON_DLPATH
		);
		return 1;
	}

	status = Asc_DynamicLoad(libpath, NULL);
	if (status != 0) {
		ASC_FREE(libpath);
		return 1; /* failed to load */
	}


# if defined(FNAME_UCASE_NODECOR) || defined(FNAME_UCASE_DECOR) || defined(FNAME_UCASE_PREDECOR)
#  define FNCASE(C) C=toupper(C)
# elif defined(FNAME_LCASE_NODECOR) || defined(FNAME_LCASE_DECOR)
#  define FNCASE(C) C=tolower(C)
# else
#  error "Need platform-specific information (asc_tron.c)"
# endif

# if defined(FNAME_UCASE_DECOR) || defined(FNAME_LCASE_DECOR)
#  define FNDECOR(S,L) strcat(S,"_")
# elif defined(FNAME_UCASE_PREDECOR) /* on windows, precede with _ and append @L (integer value of L) */
#  define FNDECOR(S,L) strcat(S,L);for(c=S+strlen(S)+1;c>S;--c){*c=*(c-1);} *S='_';
# else
#  define FNDECOR(S,L) (void)0
# endif

# define FN_PTR_GET(T,A,V,L) \
	sprintf(fnsymbol,"%s",#T); \
	for(c=fnsymbol;*c!='\0';++c){ \
		FNCASE(*c); \
	} \
	FNDECOR(fnsymbol,L); \
	tron_fptrs.T##_ptr = (T##_fn_t *)Asc_DynamicFunction(libpath,fnsymbol); \
	if(tron_fptrs.T##_ptr==NULL)status+=1;

	FN_PTR_GET(dtron,TRON_DTRON_ARGS,TRON_DTRON_VALS,"");

# undef FN_PTR_GET
# undef FNDECOR
# undef FNCASE

	ASC_FREE(libpath);

	if(status!=0){
		return 1; /* failed to resolve all symbols */
	}

    loaded = 1; /* save result for next time, as we will never unload it */
	return 0;
}

/*------------------------------------------------------------------------------
  REGISTERING 'TRON' WITH ASCEND
*/

static const SlvFunctionsT tron_internals = {
	10
	,"TRON"
	,tron_create
  	,tron_destroy
	,tron_eligible_solver
	,tron_get_default_parameters
	,tron_get_parameters
	,tron_set_parameters
	,tron_get_status
	,tron_solve
	,tron_presolve
	,tron_iterate
	,NULL
	,NULL
	,NULL
	,NULL
};

int tron_register(void){
#ifndef ASC_LINKED_TRON
	if(tron_dlopen()){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to load TRON");
		return 1;
	}
#endif
	return solver_register(&tron_internals);
}
