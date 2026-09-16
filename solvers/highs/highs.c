/*
 *  HiGHS linear solver interface for ASCEND
 *  by John Pye
 *  Created: 14 Feb 2026
 *
 *  This file is part of ASCEND
 *
 *  Copyright (C) 2026 John Pye
 *
 *  ASCEND is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  ASCEND is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "highs.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/set.h>
#include <ascend/general/mem.h>
#include <ascend/general/tm_time.h>
#include <ascend/general/list.h>
#include <ascend/general/dstring.h>
#include <ascend/utilities/error.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/instquery.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdarg.h>

#include <ascend/linear/mtx.h>

#include <ascend/system/calc.h>
#include <ascend/system/relman.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/bnd.h>
#include <ascend/system/var.h>
#include <ascend/system/rel.h>
#include <ascend/system/lp_utils.h>
#include <interfaces/highs_c_api.h>

#ifndef KILL
#define KILL TRUE
#endif
#define DEBUG FALSE
#define HIGHS_PROGRESS_REPORT_INTERVAL 1.0
//#define HIGHS_DEBUG
#ifdef HIGHS_DEBUG
# define MSG(...) CONSOLE_DEBUG(__VA_ARGS__)
#else
# define MSG(...) ((void)0)
#endif

#define SYS(s) ((highs_system_t)(s))

ASC_DLLSPEC SolverRegisterFn highs_register;

/* HiGHS 1.13.1 header declares this static symbol without defining it. */
static const char* Highs_compilationDate(void){ return ""; }
static void highs_link_capi_stubs(void){ (void)Highs_compilationDate(); }

struct highs_system_structure {

   /**
    ***  Problem definition
    **/
   slv_system_t           slv;          /* slv_system_t back-link */
   struct rel_relation    *obj;          /* Objective function: NULL = none */
   struct var_variable    **vlist;       /* Variable list (NULL terminated) */
   struct var_variable    **vlist_user;  /* User vlist (NULL = determine) */
   struct bnd_boundary    *blist;       /* Boundary list (NULL terminated) */
   struct bnd_boundary    *blist_user;  /* User blist (NULL = none) */
   struct rel_relation    **rlist;       /* Relation list (NULL terminated) */
   struct rel_relation    **rlist_user;  /* User rlist (NULL = none) */
   struct ExtRelCache     **erlist;     /* External relations cache list */
   struct ExtRelCache     **erlist_user;/* User erlist (NULL = none) */

   /**
    ***  Solver information
    **/
   int                    integrity;    /* ? Has the system been created */

   slv_parameters_t       p;            /* Parameters */
   struct slv_parameter pa[HIGHS_PARAMS];

   slv_status_t           s;            /* Status flags */
   double                 clock;        /* CPU time */
   double                 next_progress_report_time;
   int                    progress_report_count;

   /**
    ***  Calculated Data
    ***
    **/
   mps_data_t  mps;          /* the main chunk of data for the problem */
   int prepared_relaxed;    /* global domain policy used to assemble mps */

};

static void highs_spoof_block_status(highs_system_t sys){
   struct slv__block_status_structure *block = &sys->s.block;
   int32 size = (sys->mps.vused > 0) ? (int32)sys->mps.vused : 0;

   block->number_of = 1;
   block->current_block = 0;
   block->current_reordered_block = 0;
   block->current_size = size;
   block->previous_total_size = 0;
   block->previous_total_size_vars = 0;
   block->iteration = sys->s.iteration;
   block->funcs = 0;
   block->jacs = 0;
   block->cpu_elapsed = sys->s.cpu_elapsed;
   block->functime = 0.0;
   block->jactime = 0.0;
   block->residual = 0.0;
}

static int highs_get_default_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
	struct slv_parameter *new_parms = NULL;

	(void)server;
	(void)asys;
	highs_link_capi_stubs();

	if(parameters->parms == NULL) {
		new_parms = ASC_NEW_ARRAY_OR_NULL(struct slv_parameter,HIGHS_PARAMS);
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

	/** Model handling options */

	slv_param_bool(parameters,HIGHS_PARAM_NONLIN
		,(SlvParameterInitBool){{"nonlin"
			,"Linearise non-linear equations?",1
			,"Perform linearisation of non-linear models at the current point (TRUE)"
			" or else require a linear model (FALSE)."
		}, FALSE}
	);


	slv_param_bool(parameters,HIGHS_PARAM_RELAXED
		,(SlvParameterInitBool){{"relaxed"
			,"Solve LP relaxation?",1
			,"Solve regular problem (FALSE) or LP relaxation of problem (TRUE)."
		}, FALSE}
	);

	slv_param_bool(parameters,HIGHS_PARAM_PROGRESS_CALLBACKS
		,(SlvParameterInitBool){{"progress_callbacks"
			,"Enable progress callbacks?",2
			,"Enable HiGHS callback-based progress reporting (text output and GUI polling support)."
		}, TRUE}
	);
	slv_param_bool(parameters,HIGHS_PARAM_PROGRESS_LOG
		,(SlvParameterInitBool){{"progress_log"
			,"Log progress to console?",2
			,"Emit progress lines via console/error reporter."
		}, FALSE}
	);

	/** HiGHS runtime options */

	slv_param_real(parameters,HIGHS_PARAM_TIME_LIMIT
		,(SlvParameterInitReal){{"time_limit"
			,"Time limit (s)",2
			,"HiGHS time limit in seconds."
		}, 1e20, 0.0, 1.e30}
	);

	slv_param_int(parameters,HIGHS_PARAM_THREADS
		,(SlvParameterInitInt){{"threads"
			,"Threads",2
			,"HiGHS thread count (0 = automatic)."
		}, 0, 0, 1024}
	);

	slv_param_char(parameters,HIGHS_PARAM_PRESOLVE
		,(SlvParameterInitChar){{"presolve"
			,"Presolve mode",2
			,"HiGHS presolve option."
		}, "choose"}, (char *[]){
			"choose","on","off",NULL
		}
	);

	slv_param_char(parameters,HIGHS_PARAM_SOLVER
		,(SlvParameterInitChar){{"solver"
			,"Algorithm",2
			,"HiGHS solver strategy."
		}, "choose"}, (char *[]){
			"choose","simplex","ipm","ipx",NULL
		}
	);

	slv_param_char(parameters,HIGHS_PARAM_PARALLEL
		,(SlvParameterInitChar){{"parallel"
			,"Parallel mode",2
			,"HiGHS parallel option."
		}, "choose"}, (char *[]){
			"choose","on","off",NULL
		}
	);

	slv_param_real(parameters,HIGHS_PARAM_MIP_REL_GAP
		,(SlvParameterInitReal){{"mip_rel_gap"
			,"MIP relative gap",3
			,"HiGHS relative optimality gap target for MIP."
		}, 1e-4, 0.0, 1.e30}
	);

	slv_param_real(parameters,HIGHS_PARAM_MIP_ABS_GAP
		,(SlvParameterInitReal){{"mip_abs_gap"
			,"MIP absolute gap",3
			,"HiGHS absolute optimality gap target for MIP."
		}, 1e-6, 0.0, 1.e30}
	);

	slv_param_int(parameters,HIGHS_PARAM_RANDOM_SEED
		,(SlvParameterInitInt){{"random_seed"
			,"Random seed",3
			,"HiGHS random seed."
		}, 0, 0, 2147483647}
	);

	slv_param_real(parameters,HIGHS_PARAM_PINF
		,(SlvParameterInitReal){{"pinf"
			,"Positive 'infinity' threshold",4
			,"Any upper bound greater than 'pinf' is treated as +infinity in the exported matrix."
		}, 1e30, 0, 1.e99}
	);

	slv_param_real(parameters,HIGHS_PARAM_MINF
		,(SlvParameterInitReal){{"minf"
			,"Negative 'infinity' threshold",4
			,"Any lower bound less than 'minf' is treated as -infinity in the exported matrix."
		}, -1e30, -1e99, 0}
	);
	slv_param_bool(parameters,HIGHS_PARAM_VARNOM_SCALE
		,(SlvParameterInitBool){{"varnom_scale"
			,"Scale by variable nominals?",4
			,"Scale LP/MIP columns and bounds using continuous-variable nominal values."
		}, TRUE}
	);
	slv_param_bool(parameters,HIGHS_PARAM_RELNOM_SCALE
		,(SlvParameterInitBool){{"relnom_scale"
			,"Scale by relation nominals?",4
			,"Scale LP/MIP constraint rows and RHS using relation nominal values."
		}, TRUE}
	);

	asc_assert(parameters->num_parms==HIGHS_PARAMS);

	return 1;
}


/* _________________________________________________________________________ */

/*
 ***  Integrity checks
 ***  ----------------
 ***     check_system(sys)
 **/

#define OK        ((int)813025392)
#define DESTROYED ((int)103289182)
static int check_system(highs_system_t sys)
/**
 ***  Checks sys for NULL and for integrity.
 **/
{
   if( sys == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL system handle.");
      return 1;
   }

   switch( sys->integrity ) {
   case OK:
      return 0;
   case DESTROYED:
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"system was recently destroyed.");
      return 1;
   default:
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"system reused or never allocated.");
      return 1;
   }
}

/* _________________________________________________________________________ */

/*
 ***  Array/vector operations
 ***  ----------------------------
 ***     destroy_array(p)
 ***     create_array(len,type)
 ***     zero_array(arr,len,type)
 ***     nuke_pointers(mps_data_t mps)  - free allocated memory in mps
 **/

#define destroy_array(p)  \
   if( (p) != NULL ) ascfree((p))
#define create_array(len,type)  \
   ((len) > 0 ? ASC_NEW_ARRAY(type,len) : NULL)
#define create_zero_array(len,type)  \
   ((len) > 0 ? ASC_NEW_ARRAY_CLEAR(type,len) : NULL)
#define zero_array(arr,nelts,type)    \
   mem_zero_byte_cast((arr),0,(nelts)*sizeof(type))
/* Zeros an array of nelts objects, each having given type. */


static void nuke_pointers(mps_data_t *mps) {
	lp_nuke_pointers(mps);
}

/* _________________________________________________________________________ */

/*
 ***  General input/output routines
 ***  -----------------------------
 ***     fp = MIF(sys)
 ***     fp = LIF(sys)
 **/

/*
 ***  Routines for common filters
 ***  -----------------
 ***  highs_free_inc_var_filter -  true for non-fixed incident variables
 ***  inc_rel_filter      -  true for incident relations
 **/

/**
	I've been calling this particular var filter a lot ,
	so I decided to make it a subroutine.  Returns true if
	var is not fixed and incident in something.
*/
extern boolean highs_free_inc_var_filter(struct var_variable *var){
	return lp_free_inc_var_filter(var);
}

/* _________________________________________________________________________ */

/* LP assembly, sparse export and writeback are shared in libascend. */
#ifndef KILL

static struct var_variable **update_vlist(struct var_variable * *vlist, expr_t expr)
/**
 ***  Updates vlist, adding variables incident on given expression.  The
 ***  old list is destroyed and the new list is returned.
 **/
{
   struct var_variable **newlist,**elist;
   long nelts;

   elist = expr_incidence_list(expr,NULL);
   if( vlist == NULL ) return(elist);
   if( (nelts=pl_length(elist)) == 0 ) {
      ascfree( (POINTER)elist );
      return(vlist);
   }

   nelts += pl_length(vlist) + 1;
   newlist = (struct var_variable **)ascmalloc( sizeof(struct var_variable *) * (int)nelts );
   pl_merge_0(newlist,vlist,elist);
   ascfree( (POINTER)vlist );
   ascfree( (POINTER)elist );
   return(newlist);
}

#endif

#ifndef KILL

/**
	This function constructs the variable list from the relation list.  It
	is assumed that sys->vlist_user == NULL so that this is necessary.

	@todo FIXME this function seems to be building its own vlist, which we
	shouldn't have to do using the 'new' Solver API.
*/
static void determine_vlist(highs_system_t sys){
   bnd_boundary_t *bp;
   struct rel_relation **rp;

   if( pl_length(sys->vlist) > 0 )
      ascfree( (POINTER)sys->vlist );
   sys->vlist = NULL;
   for( bp=sys->blist ; *bp != NULL ; ++bp ) {
      sys->vlist = update_vlist(sys->vlist,bnd_lhs(*bp));
      sys->vlist = update_vlist(sys->vlist,bnd_rhs(*bp));
   }
   for( rp=sys->rlist ; *rp != NULL ; ++rp ) {
      sys->vlist = update_vlist(sys->vlist,rel_lhs(*rp));
      sys->vlist = update_vlist(sys->vlist,rel_rhs(*rp));
   }
   if( sys->obj )
      sys->vlist = update_vlist(sys->vlist,sys->obj);

   if( sys->vlist == NULL )
      highs_set_var_list(sys,NULL);
}

#endif

/* _________________________________________________________________________ */

/**
 ***  External routines used from slv0 without modificiation
 ***
 ***  highs_set_var_list(sys,vlist)
 ***  highs_get_var_list(sys)
 ***  highs_set_bnd_list(sys,blist)
 ***  highs_get_bnd_list(sys)
 ***  highs_set_rel_list(sys,rlist)
 ***  highs_get_rel_list(sys)
 ***  highs_set_extrel_list(sys,erlist)
 ***  highs_get_extrel_list(sys)
 ***  highs_count_vars(sys,vfilter)
 ***  highs_count_bnds(sys,bfilter)
 ***  highs_count_rels(sys,rfilter)
 ***  highs_set_obj_function(sys,obj)
 ***  highs_get_obj_function(sys)
 ***  highs_get_parameters(sys,parameters)
 ***  highs_set_parameters(sys,parameters)
 ***  highs_get_status(sys,status)
 ***  highs_dump_internals(sys,level)
 **/


#if 0
void highs_set_var_list(highs_system_t sys, struct var_variable **vlist){
   static struct var_variable *empty_list[] = {NULL};
   check_system(sys);
   if( sys->vlist_user == NULL )
      if( sys->vlist != NULL && pl_length(sys->vlist) > 0 )
	 ascfree( (POINTER)(sys->vlist) );
   sys->vlist_user = vlist;
   sys->vlist = (vlist==NULL ? empty_list : vlist);
   sys->s.ready_to_solve = FALSE;
}

struct var_variable **highs_get_var_list(highs_system_t sys){
   check_system(sys);
   return( sys->vlist_user );
}

void highs_set_bnd_list(highs_system_t sys, struct bnd_boundary *blist){
   static struct bnd_boundary empty_list[] = {};
   check_system(sys);
   sys->blist_user = blist;
   sys->blist = (blist==NULL ? empty_list : blist);
   sys->s.ready_to_solve = FALSE;
}

struct bnd_boundary *highs_get_bnd_list(sys)
highs_system_t sys;
{
   check_system(sys);
   return( sys->blist_user );
}

void highs_set_rel_list(sys,rlist)
highs_system_t sys;
struct rel_relation **rlist;
{
   static struct rel_relation *empty_list[] = {NULL};
   check_system(sys);
   sys->rlist_user = rlist;
   sys->rlist = (rlist==NULL ? empty_list : rlist);
   sys->s.ready_to_solve = FALSE;
}

struct rel_relation **highs_get_rel_list(sys)
highs_system_t sys;
{
   check_system(sys);
   return( sys->rlist_user );
}

void highs_set_extrel_list(sys,erlist)
highs_system_t sys;
struct ExtRelCache **erlist;
{
   static struct ExtRelCache *empty_list[] = {NULL};
   check_system(sys);
   sys->erlist_user = erlist;
   sys->erlist = (erlist==NULL ? empty_list : erlist);
   sys->s.ready_to_solve = FALSE;
}

struct ExtRelCache **highs_get_extrel_list(sys)
highs_system_t sys;
{
   check_system(sys);
   return( sys->erlist_user );
}

int highs_count_vars(sys,vfilter)
highs_system_t sys;
var_filter_t *vfilter;
{
   struct var_variable **vp;
   int32 count = 0;
   check_system(sys);
   for( vp=sys->vlist; *vp != NULL; vp++ )
      if( var_apply_filter(*vp,vfilter) ) ++count;
   return( count );
}

int highs_count_bnds(highs_system_t sys,bnd_filter_t *bfilter){
	struct bnd_boundary *bp;
	int32 count = 0;
	check_system(sys);
	for( bp=sys->blist; *bp != NULL; bp++ ){
		if( bnd_apply_filter(*bp,bfilter) ) ++count;
	}
	return( count );
}

int highs_count_rels(highs_system_t sys,rel_filter_t *rfilter){
   struct rel_relation **rp;
   int32 count = 0;
   check_system(sys);
   for( rp=sys->rlist; *rp != NULL; rp++ )
      if( rel_apply_filter(*rp,rfilter) ) ++count;
   return( count );
}

void highs_set_obj_relation(highs_system_t sys,struct rel_relation *obj){
   check_system(sys);
   sys->obj = obj;
   sys->s.ready_to_solve = FALSE;
}

struct rel_relation *highs_get_obj_relation(highs_system_t sys){
   check_system(sys);
   return(sys->obj);
}

void highs_dump_internals(highs_system_t sys, int level){
   check_system(sys);
   if (level > 0) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"dumping internals is not implemented.");
   }
}
#endif

void highs_get_parameters(slv_system_t server,slv_parameters_t *parameters){
	highs_system_t sys;
	sys = SYS(server);
	check_system(sys);
	mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

void highs_set_parameters(slv_system_t server, slv_parameters_t *parameters){
	highs_system_t sys;
	sys = SYS(server);
	check_system(sys);
	if (parameters->whose==highs_solver_number)
	mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

void highs_get_status(slv_system_t server, slv_status_t *status){
	highs_system_t sys;
	sys = SYS(server);
	check_system(sys);
	mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}

/* _________________________________________________________________________ */

/**
 ***  External routines with minor modifications
 ***  -----------------
 ***  highs_change_basis           just return FALSE & error msg
 **/

boolean highs_change_basis(highs_system_t sys,int32 var, mtx_range_t *rng){
/* In the MPS file maker, changing the basis doesn't make any sense.
   Nor, for that matter, is there a basis in the first place.
   So I just write out an error message, and return FALSE  */

   ERROR_REPORTER_HERE(ASC_PROG_ERR,"changing basis is not supported.");

   return FALSE;
}


/* _________________________________________________________________________ */

/*
 ***  External routines unique to slv6 (Based on routines from slv0)
 ***  -----------------
 ***  highs_create()               added solver specific initialization
 ***  highs_destroy(sys)           added solver specific dealocation
 ***  highs_eligible_solver(sys)   see if solver can do the current problem
 ***  highs_presolve(sys)          set up system and create matrix/vectors
 ***  highs_solve(sys)             call MPS routines
 ***  highs_iterate(sys)           just calls highs_solve
 ***  highs_resolve(sys)           just calls highs_solve
 **/


/**
	This routine allocates memory and initializes all data structures
	It should be a good source of comments on the system parameters and
	status flags used in slv6
*/
static SlvClientToken highs_create(slv_system_t server, int32 *statusindex){   /* added mps initialization */
	highs_system_t sys;

	sys = ASC_NEW_CLEAR(struct highs_system_structure);
	if(sys==NULL){
		*statusindex = 1;
		return sys;
	}

	/* Bind this solver instance to the current system lists. */
	sys->slv = server;
	sys->vlist_user = slv_get_solvers_var_list(server);
	sys->vlist = sys->vlist_user;
	sys->blist_user = (struct bnd_boundary *)slv_get_solvers_bnd_list(server);
	sys->blist = sys->blist_user;
	sys->rlist_user = slv_get_solvers_rel_list(server);
	sys->rlist = sys->rlist_user;
	sys->obj = slv_get_obj_relation(server);

	/***  Initialize system parameters ***/

	sys->p.parms = sys->pa;
	sys->p.dynamic_parms = 0;
	highs_get_default_parameters(server,(SlvClientToken)sys,&(sys->p));
	sys->p.whose = (*statusindex);

	sys->integrity = OK;

#if 0
	sys->p.output.more_important = stdout;  /* used in MIF macro */
	sys->p.output.less_important = NULL;    /*   used in LIF macro (which is not used) */

	sys->p.tolerance.pivot = 0.1;           /* these tolerances are never used */
	sys->p.tolerance.singular = 1e-12;
	sys->p.tolerance.feasible = 1e-8;
	sys->p.tolerance.stationary = 1e-8;
	sys->p.tolerance.termination = 1e-12;
	sys->p.time_limit = 1500.0;             /* never used */
	sys->p.iteration_limit = 100;           /* never used */
	sys->p.partition = FALSE;               /* never used, but don't want partitioning */
	sys->p.ignore_bounds = FALSE;           /* never used, but must satisfy bounds */
	sys->p.whose = highs_solver_number;      /* read in highs_set_parameters */
	sys->p.rho = 1.0;
	sys->p.sp.iap=&(sys->iarray[0]);        /* all defaults in iarray are 0 */
	sys->p.sp.rap=&(sys->rarray[0]);        /* all defaults in rarray are 0 */
	sys->p.sp.cap=&(sys->carray[0]);        /* all defaults in carray are NULL */
	sys->p.sp.vap=NULL;                     /* not currently used */
#endif

	/***  Initialize mps data structure ***/

	sys->mps.Ac_mtx = NULL;    /* set all pointers to NULL - will all be set in presolve */
	sys->mps.lbrow = NULL;     /* all other data in mps structure is 0 */
	sys->mps.ubrow = NULL;
	sys->mps.bcol = NULL;
	sys->mps.col_scale = NULL;
	sys->mps.row_scale = NULL;
	sys->mps.typerow = NULL;
	sys->mps.relopcol = NULL;


	/***  Initialize status flags ***/

	sys->s.kind                      = SLV_STATUS_LP;
	sys->s.over_defined               = FALSE;  /* set to (sys->mps.rinc > sys->mps.vinc) in highs_presolve */
	sys->s.under_defined              = FALSE;  /* set to (sys->mps.rinc < sys->mps.vinc) in highs_presolve */
	sys->s.struct_singular            = FALSE;  /* set to (sys->mps.rank < sys->mps.rinc) in highs_presolve */
	sys->s.calc_ok                    = TRUE;   /* set in calc_matrix (FALSE if error occurs with diffs calc) */
	sys->s.ok                         = TRUE;   /* set to (sys->s.calc_ok && !sys->s.struct_singular) in highs_presolve */
	sys->s.ready_to_solve             = FALSE;  /* set to (sys->.ok) after highs_presolve,
		                                       set FALSE after:  highs_set_var_list, highs_set_bnd_list,
		                                           highs_set_rel_list, highs_set_extrel_list, highs_set_obj_function
		                                       tested in highs_solve */
	sys->s.converged                  = FALSE;  /* set FALSE after highs_presolve; set TRUE after highs_solve */
	sys->s.diverged                   = FALSE;  /* always FALSE, never used */
	sys->s.inconsistent               = FALSE;  /* always FALSE, never used */
	sys->s.iteration_limit_exceeded   = FALSE;  /* always FALSE, never used */
	sys->s.time_limit_exceeded        = FALSE;  /* always FALSE, never used */

	sys->s.iteration                  = 0;      /* reset iteration count */
	sys->s.cpu_elapsed                = 0.0;    /* reset elapsed time */

	memset(&sys->s.u,0,sizeof(sys->s.u));
	sys->next_progress_report_time = 0.0;
	sys->progress_report_count = 0;

	/* Successful create should clear warning/status code. */
	*statusindex = 0;
	return(sys);
}

static int highs_destroy(slv_system_t server, SlvClientToken asys){
	highs_system_t sys;
	sys = SYS(server);
	//int i;
	if(server == NULL || sys==NULL)return 1;

	if(check_system(sys))return 1;
#if 0
	highs_set_var_list(sys,(struct var_variable **)NULL);
	//highs_set_obj_function(sys,NULL);
	highs_set_bnd_list(sys,NULL);
	highs_set_rel_list(sys,NULL);
	highs_set_extrel_list(sys,NULL);
#endif
	sys->integrity = DESTROYED;
	/* no per-status allocations */

	slv_destroy_parms(&(sys->p));

	nuke_pointers(&(sys->mps));   /* free memory, and set all pointers to NULL */
	ascfree( (POINTER)sys );


	return 0;
}


/**
	The system must have a relation list and objective before
	highs_eligible_solver will return true
 */
boolean highs_eligible_solver(highs_system_t server){
	highs_system_t sys;
	sys = SYS(server);

   struct rel_relation **rp;
   var_filter_t vfilter;

   check_system(sys);
   if( sys->rlist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"relation list was never set.");
      return (FALSE);
   }
   if( sys->obj == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"no objective in problem.");
      return (FALSE);
   }

   /* To Do:  External Relations are currently being ingored.  Is that proper?
              What if they're nonlinear   */

   vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
   vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
   /*
   vfilter.fixed = var_false;
   vfilter.incident = var_true;
   vfilter.in_block = var_ignore;   */

   /*  Check that the system is linear unless nonlinear linearization is enabled. */
   if (SLV_PARAM_BOOL(&(sys->p),HIGHS_PARAM_NONLIN) == 0){
      for( rp=sys->rlist ; *rp != NULL ; ++rp )   /* check relations */
          if(lp_inc_rel_filter(*rp) && !lp_relation_is_affine(*rp,&vfilter)) {
            char *relname = rel_make_name(sys->slv,*rp);
            ERROR_REPORTER_HERE(ASC_PROG_ERR
               ,"With current settings, HiGHS requires linear models; nonlinearity in constraint '%s'."
               ,(relname ? relname : "<unknown>")
            );
            ASC_FREE(relname);
            return(FALSE);   /* don't do nonlinearities */
          }
      if(!lp_relation_is_affine(sys->obj,&vfilter)){
          char *relname = rel_make_name(sys->slv,sys->obj);
          ERROR_REPORTER_HERE(ASC_PROG_ERR
             ,"With current settings, HiGHS requires linear models; nonlinearity in objective '%s'."
             ,(relname ? relname : "<unknown>")
          );
          ASC_FREE(relname);
          return(FALSE);   /* don't do nonlinearities */
      }
   }

   /*  Note: initially I had this routine check to see if solver could handle
       binary, integer, semicontinuous, etc.  Now I just convert types automatically.
       Binary vars become integer vars if a solver can do int but not binary.
       Semicontinuous vars become regular solver vars, with warnings about the
       conversion.  If it can't handle integer vars, they are treated as regular
       solver vars, with warnings.

       These conversions take place in the MPS.c file.*/

   return TRUE;
}

void highs_presolve(slv_system_t server){
   highs_system_t sys = SYS(server);
   sys->obj = slv_get_obj_relation(sys->slv);
   sys->vlist = slv_get_solvers_var_list(sys->slv);
   sys->rlist = slv_get_solvers_rel_list(sys->slv);
   memset(&sys->s,0,sizeof(sys->s));
   sys->s.kind = SLV_STATUS_LP;
   if(!highs_eligible_solver(sys) || lp_prepare_relaxed(sys->slv,&sys->mps,&sys->s,
      SLV_PARAM_BOOL(&sys->p,HIGHS_PARAM_VARNOM_SCALE),
      SLV_PARAM_BOOL(&sys->p,HIGHS_PARAM_RELNOM_SCALE),
      SLV_PARAM_BOOL(&sys->p,HIGHS_PARAM_RELAXED))){
      sys->s.calc_ok = FALSE;
      return;
   }
   sys->s.kind = lp_problem_is_mip(&sys->mps) ? SLV_STATUS_MIP : SLV_STATUS_LP;
   sys->prepared_relaxed = SLV_PARAM_BOOL(&sys->p,HIGHS_PARAM_RELAXED);
   sys->s.ok = sys->s.calc_ok = sys->s.ready_to_solve = TRUE;
   highs_spoof_block_status(sys);
}

struct highs_problem_data{
	HighsInt num_col;
	HighsInt num_row;
	HighsInt num_nz;
	HighsInt *a_start;
	HighsInt *a_index;
	double *a_value;
	double *col_cost;
	double objective_offset;
	double *col_lower;
	double *col_upper;
	double *row_lower;
	double *row_upper;
	HighsInt *integrality;
	double *col_value;
	double *col_dual;
	double *row_value;
	double *row_dual;
};

static void highs_problem_data_free(struct highs_problem_data *p){
	if(p == NULL)return;
	if(p->a_start)ascfree(p->a_start);
	if(p->a_index)ascfree(p->a_index);
	if(p->a_value)ascfree(p->a_value);
	if(p->col_cost)ascfree(p->col_cost);
	if(p->col_lower)ascfree(p->col_lower);
	if(p->col_upper)ascfree(p->col_upper);
	if(p->row_lower)ascfree(p->row_lower);
	if(p->row_upper)ascfree(p->row_upper);
	if(p->integrality)ascfree(p->integrality);
	if(p->col_value)ascfree(p->col_value);
	if(p->col_dual)ascfree(p->col_dual);
	if(p->row_value)ascfree(p->row_value);
	if(p->row_dual)ascfree(p->row_dual);
	memset(p,0,sizeof(*p));
}

static int highs_build_problem(highs_system_t sys, struct highs_problem_data *p, int *is_mip){
   lp_sparse_t shared = {0};
   int32 i;
   memset(p,0,sizeof(*p));
   *is_mip = 0;
   if(lp_sparse_build(&shared,&sys->mps,sys->vlist,sys->obj,
      SLV_PARAM_REAL(&sys->p,HIGHS_PARAM_MINF),
      SLV_PARAM_REAL(&sys->p,HIGHS_PARAM_PINF)))return 0;
   p->num_col=shared.num_col; p->num_row=shared.num_row; p->num_nz=shared.num_nz;
   p->objective_offset=shared.objective_offset;
   p->col_cost=shared.cost; shared.cost=NULL;
   p->col_lower=shared.lower; shared.lower=NULL;
   p->col_upper=shared.upper; shared.upper=NULL;
   p->row_lower=shared.row_lower; shared.row_lower=NULL;
   p->row_upper=shared.row_upper; shared.row_upper=NULL;
   p->a_value=shared.value; shared.value=NULL;
   /* HiGHS may use 64-bit indices; do not alias libascend's int32 arrays. */
   p->a_start=ASC_NEW_ARRAY(HighsInt,p->num_col+1);
   p->a_index=ASC_NEW_ARRAY(HighsInt,MAX(p->num_nz,1));
   p->integrality=ASC_NEW_ARRAY(HighsInt,p->num_col);
   p->col_value=ASC_NEW_ARRAY(double,p->num_col);
   p->col_dual=ASC_NEW_ARRAY(double,p->num_col);
   p->row_value=ASC_NEW_ARRAY(double,MAX(p->num_row,1));
   p->row_dual=ASC_NEW_ARRAY(double,MAX(p->num_row,1));
   if(!p->a_start || !p->a_index || !p->integrality || !p->col_value ||
      !p->col_dual || !p->row_value || !p->row_dual){
      lp_sparse_destroy(&shared); highs_problem_data_free(p); return 0;
   }
   for(i=0;i<=p->num_col;++i)p->a_start[i]=shared.start[i];
   for(i=0;i<p->num_nz;++i)p->a_index[i]=shared.index[i];
   for(i=0;i<p->num_col;++i){
      p->integrality[i]=kHighsVarTypeContinuous;
      if(shared.type[i]==MPS_INT || shared.type[i]==MPS_BINARY)
         p->integrality[i]=kHighsVarTypeInteger;
      else if(shared.type[i]==MPS_SEMI)p->integrality[i]=kHighsVarTypeSemiContinuous;
      if(p->integrality[i]!=kHighsVarTypeContinuous)*is_mip=1;
   }
   lp_sparse_destroy(&shared);
   return 1;
}

static int highs_apply_options(highs_system_t sys, void *highs){
	const char *presolve = SLV_PARAM_CHAR(&(sys->p),HIGHS_PARAM_PRESOLVE);
	const char *solver = SLV_PARAM_CHAR(&(sys->p),HIGHS_PARAM_SOLVER);
	const char *parallel = SLV_PARAM_CHAR(&(sys->p),HIGHS_PARAM_PARALLEL);
	HighsInt status;

#define APPLY_AND_VERIFY_BOOL_OPTION(NAME, VALUE) do{ \
	HighsInt expected = (VALUE); \
	HighsInt actual = 0; \
	status = Highs_setBoolOptionValue(highs,(NAME),expected); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed setting HiGHS option '%s'.",(NAME)); \
		return 0; \
	} \
	status = Highs_getBoolOptionValue(highs,(NAME),&actual); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed reading back HiGHS option '%s'.",(NAME)); \
		return 0; \
	} \
	if(actual != expected){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"HiGHS option '%s' read-back mismatch (expected %ld, got %ld).",(NAME),(long)expected,(long)actual); \
		return 0; \
	} \
}while(0)
#define APPLY_AND_VERIFY_INT_OPTION(NAME, VALUE) do{ \
	HighsInt expected = (VALUE); \
	HighsInt actual = 0; \
	status = Highs_setIntOptionValue(highs,(NAME),expected); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed setting HiGHS option '%s'=%ld.",(NAME),(long)expected); \
		return 0; \
	} \
	status = Highs_getIntOptionValue(highs,(NAME),&actual); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed reading back HiGHS option '%s'.",(NAME)); \
		return 0; \
	} \
	if(actual != expected){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"HiGHS option '%s' read-back mismatch (expected %ld, got %ld).",(NAME),(long)expected,(long)actual); \
		return 0; \
	} \
}while(0)
#define APPLY_AND_VERIFY_DOUBLE_OPTION(NAME, VALUE) do{ \
	double expected = (VALUE); \
	double actual = 0.0; \
	double tol; \
	status = Highs_setDoubleOptionValue(highs,(NAME),expected); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed setting HiGHS option '%s'=%g.",(NAME),expected); \
		return 0; \
	} \
	status = Highs_getDoubleOptionValue(highs,(NAME),&actual); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed reading back HiGHS option '%s'.",(NAME)); \
		return 0; \
	} \
	tol = 1e-12 * (1.0 + fabs(expected)); \
	if(fabs(actual - expected) > tol){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"HiGHS option '%s' read-back mismatch (expected %.17g, got %.17g).",(NAME),expected,actual); \
		return 0; \
	} \
}while(0)
#define APPLY_AND_VERIFY_STRING_OPTION(NAME, VALUE) do{ \
	const char *expected = (VALUE); \
	char actual[512]; \
	status = Highs_setStringOptionValue(highs,(NAME),expected); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed setting HiGHS option '%s'='%s'.",(NAME),expected); \
		return 0; \
	} \
	status = Highs_getStringOptionValue(highs,(NAME),actual); \
	if(status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed reading back HiGHS option '%s'.",(NAME)); \
		return 0; \
	} \
	if(strcmp(actual,expected) != 0){ \
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"HiGHS option '%s' read-back mismatch (expected '%s', got '%s').",(NAME),expected,actual); \
		return 0; \
	} \
}while(0)

	APPLY_AND_VERIFY_BOOL_OPTION("output_flag",0);
	APPLY_AND_VERIFY_DOUBLE_OPTION("time_limit",SLV_PARAM_REAL(&(sys->p),HIGHS_PARAM_TIME_LIMIT));
	APPLY_AND_VERIFY_INT_OPTION("threads",(HighsInt)SLV_PARAM_INT(&(sys->p),HIGHS_PARAM_THREADS));
	APPLY_AND_VERIFY_STRING_OPTION("presolve",(presolve != NULL ? presolve : "choose"));
	APPLY_AND_VERIFY_STRING_OPTION("solver",(solver != NULL ? solver : "choose"));
	APPLY_AND_VERIFY_STRING_OPTION("parallel",(parallel != NULL ? parallel : "choose"));
	APPLY_AND_VERIFY_DOUBLE_OPTION("mip_rel_gap",SLV_PARAM_REAL(&(sys->p),HIGHS_PARAM_MIP_REL_GAP));
	APPLY_AND_VERIFY_DOUBLE_OPTION("mip_abs_gap",SLV_PARAM_REAL(&(sys->p),HIGHS_PARAM_MIP_ABS_GAP));
	APPLY_AND_VERIFY_INT_OPTION("random_seed",(HighsInt)SLV_PARAM_INT(&(sys->p),HIGHS_PARAM_RANDOM_SEED));

#undef APPLY_AND_VERIFY_BOOL_OPTION
#undef APPLY_AND_VERIFY_INT_OPTION
#undef APPLY_AND_VERIFY_DOUBLE_OPTION
#undef APPLY_AND_VERIFY_STRING_OPTION

	return 1;
}

static int highs_callback_total_iteration_count(const HighsCallbackDataOut *data_out){
	long long total = 0;
	if(data_out == NULL)return 0;
	if(data_out->simplex_iteration_count > 0){
		total += (long long)data_out->simplex_iteration_count;
	}
	if(data_out->ipm_iteration_count > 0){
		total += (long long)data_out->ipm_iteration_count;
	}
	if(data_out->pdlp_iteration_count > 0){
		total += (long long)data_out->pdlp_iteration_count;
	}
	if(total > INT_MAX)return INT_MAX;
	if(total < 0)return 0;
	return (int)total;
}

static void highs_progress_append(char *details, size_t cap, int *n, const char *fmt, ...){
	va_list ap;
	int wrote;
	size_t rem;
	if(*n < 0 || (size_t)(*n) >= cap)return;
	rem = cap - (size_t)(*n);
	va_start(ap,fmt);
	wrote = vsnprintf(details + (*n),rem,fmt,ap);
	va_end(ap);
	if(wrote < 0){
		*n = (int)cap;
		return;
	}
	if((size_t)wrote >= rem){
		*n = (int)cap;
		return;
	}
	*n += wrote;
}

static void highs_report_progress(
	highs_system_t sys, int callback_type,
	const HighsCallbackDataOut *data_out, int iteration_count, double running_time
){
	char details[512];
	int n = 0;
	const char *tag = "callback";
	int have_mip_data = 0;
	if(sys == NULL || data_out == NULL)return;
	switch(callback_type){
		case kHighsCallbackSimplexInterrupt: tag = "simplex"; break;
		case kHighsCallbackIpmInterrupt: tag = "ipm"; break;
		case kHighsCallbackMipInterrupt: tag = "mip"; have_mip_data = 1; break;
		case kHighsCallbackLogging: tag = "log"; break;
		case kHighsCallbackMipLogging: tag = "mip_log"; have_mip_data = 1; break;
		case kHighsCallbackMipSolution: tag = "mip_solution"; have_mip_data = 1; break;
		case kHighsCallbackMipImprovingSolution: tag = "mip_incumbent"; have_mip_data = 1; break;
		default: break;
	}

	highs_progress_append(details,sizeof(details),&n
		,"tag=%s, t=%.3gs, iter=%d"
		,tag,running_time,iteration_count
	);
	if(isfinite(data_out->objective_function_value)){
		highs_progress_append(details,sizeof(details),&n
			,", obj=%.17g",data_out->objective_function_value
		);
	}
	if(have_mip_data && data_out->mip_node_count >= 0){
		highs_progress_append(details,sizeof(details),&n
			,", mip_nodes=%lld",(long long)data_out->mip_node_count
		);
	}
	if(have_mip_data && data_out->mip_total_lp_iterations >= 0){
		highs_progress_append(details,sizeof(details),&n
			,", mip_lp_iter=%lld",(long long)data_out->mip_total_lp_iterations
		);
	}
	if(have_mip_data && isfinite(data_out->mip_primal_bound)){
		highs_progress_append(details,sizeof(details),&n
			,", mip_primal=%.17g",data_out->mip_primal_bound
		);
	}
	if(have_mip_data && isfinite(data_out->mip_dual_bound)){
		highs_progress_append(details,sizeof(details),&n
			,", mip_dual=%.17g",data_out->mip_dual_bound
		);
	}
	if(have_mip_data && isfinite(data_out->mip_gap)){
		highs_progress_append(details,sizeof(details),&n
			,", mip_gap=%.17g",data_out->mip_gap
		);
	}else if(
		have_mip_data
		&& isfinite(data_out->mip_primal_bound)
		&& isfinite(data_out->mip_dual_bound)
	){
		double abs_gap = fabs(data_out->mip_primal_bound - data_out->mip_dual_bound);
		highs_progress_append(details,sizeof(details),&n
			,", mip_abs_gap=%.17g",abs_gap
		);
	}

	if(SLV_PARAM_BOOL(&(sys->p),HIGHS_PARAM_PROGRESS_LOG)){
		MSG("progress: %s",details);
		#ifdef HIGHS_DEBUG
		ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"(HiGHS progress) %s",details);
		#endif
	}
	(void)slv_report_progress("HiGHS",details);
	sys->progress_report_count++;
}

static void highs_solver_callback(
	int callback_type, const char *message,
	const HighsCallbackDataOut *data_out,
	HighsCallbackDataIn *data_in, void *user_data
){
	highs_system_t sys = (highs_system_t)user_data;
	int iteration_count;
	double running_time;
	int force_progress;
	(void)message;
	if(sys == NULL)return;
	if(message != NULL && message[0] != '\0'){
		if(strcmp(message,"MIP check limits") != 0
			&& SLV_PARAM_BOOL(&(sys->p),HIGHS_PARAM_PROGRESS_LOG)
		){
			MSG("callback[%d]: %s",callback_type,message);
		}
	}

	iteration_count = highs_callback_total_iteration_count(data_out);
	if(iteration_count > 0){
		sys->s.iteration = iteration_count;
	}

	if(data_out != NULL && sys->s.kind == SLV_STATUS_MIP){
		slv_status_mip_t *mip = slv_status_mip_rw(&sys->s);
		if(mip != NULL){
			if(data_out->mip_node_count >= 0){
				mip->have_node_count = 1;
				mip->node_count = (long long)data_out->mip_node_count;
			}
			if(data_out->mip_total_lp_iterations >= 0){
				mip->have_total_lp_iterations = 1;
				if(data_out->mip_total_lp_iterations > INT_MAX){
					mip->total_lp_iterations = INT_MAX;
				}else{
					mip->total_lp_iterations = (int32)data_out->mip_total_lp_iterations;
				}
			}
			if(isfinite(data_out->mip_primal_bound)){
				mip->have_primal_bound = 1;
				mip->primal_bound = data_out->mip_primal_bound;
			}
			if(isfinite(data_out->mip_dual_bound)){
				mip->have_dual_bound = 1;
				mip->dual_bound = data_out->mip_dual_bound;
			}
			if(isfinite(data_out->mip_gap)){
				mip->have_gap = 1;
				mip->gap = data_out->mip_gap;
			}
			if(mip->have_primal_bound && mip->have_dual_bound){
				mip->have_abs_gap = 1;
				mip->abs_gap = fabs(mip->primal_bound - mip->dual_bound);
			}
		}
	}

	force_progress = (
		callback_type == kHighsCallbackMipSolution
		|| callback_type == kHighsCallbackMipImprovingSolution
		|| callback_type == kHighsCallbackMipLogging
	);

	if(
		data_out != NULL
		&& SLV_PARAM_BOOL(&(sys->p),HIGHS_PARAM_PROGRESS_CALLBACKS)
		&& (
			force_progress
			|| sys->progress_report_count == 0
			|| (
				(isfinite(data_out->running_time) && data_out->running_time >= 0.0 ? data_out->running_time : 0.0)
				>= sys->next_progress_report_time
			)
		)
	){
		running_time = (isfinite(data_out->running_time) && data_out->running_time >= 0.0 ? data_out->running_time : 0.0);
		sys->s.cpu_elapsed = running_time;
		highs_report_progress(sys,callback_type,data_out,iteration_count,running_time);
		sys->next_progress_report_time = running_time + HIGHS_PROGRESS_REPORT_INTERVAL;
	}

	if(data_in != NULL && slv_get_solver_interrupt()){
		data_in->user_interrupt = 1;
		sys->s.panic = TRUE;
	}
}

static void highs_enable_callbacks(highs_system_t sys, void *highs, int is_mip){
	HighsInt status;
	int progress_callbacks_enabled;
	progress_callbacks_enabled = SLV_PARAM_BOOL(&(sys->p),HIGHS_PARAM_PROGRESS_CALLBACKS);
	status = Highs_setCallback(highs,&highs_solver_callback,(void *)sys);
	if(status == kHighsStatusError){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING
			,"unable to install HiGHS callback; progress/interrupt callbacks disabled."
		);
		return;
	}
	MSG("installed HiGHS callback handler (progress callbacks %s).",progress_callbacks_enabled ? "enabled" : "disabled");

#define HIGHS_START_CALLBACK(TYPE) do{ \
	HighsInt cb_status = Highs_startCallback(highs,(TYPE)); \
	if(cb_status == kHighsStatusError){ \
		ERROR_REPORTER_HERE(ASC_PROG_WARNING \
			,"unable to start HiGHS callback type %ld." \
			,(long)(TYPE) \
		); \
	}else{ \
		MSG("started HiGHS callback type %ld.",(long)(TYPE)); \
	} \
}while(0)

	HIGHS_START_CALLBACK(kHighsCallbackSimplexInterrupt);
	HIGHS_START_CALLBACK(kHighsCallbackIpmInterrupt);
	if(progress_callbacks_enabled){
		HIGHS_START_CALLBACK(kHighsCallbackLogging);
	}
	if(is_mip){
		HIGHS_START_CALLBACK(kHighsCallbackMipInterrupt);
		if(progress_callbacks_enabled){
			HIGHS_START_CALLBACK(kHighsCallbackMipLogging);
			HIGHS_START_CALLBACK(kHighsCallbackMipSolution);
			HIGHS_START_CALLBACK(kHighsCallbackMipImprovingSolution);
		}
	}

#undef HIGHS_START_CALLBACK
}

struct highs_info_snapshot{
	int have_objective_function_value;
	double objective_function_value;
	int have_simplex_iteration_count;
	HighsInt simplex_iteration_count;
	int have_ipm_iteration_count;
	HighsInt ipm_iteration_count;
	int have_pdlp_iteration_count;
	HighsInt pdlp_iteration_count;
	int have_primal_solution_status;
	HighsInt primal_solution_status;
	int have_dual_solution_status;
	HighsInt dual_solution_status;
	int have_basis_validity;
	HighsInt basis_validity;
	int have_max_primal_infeasibility;
	double max_primal_infeasibility;
	int have_max_dual_infeasibility;
	double max_dual_infeasibility;
	int have_num_primal_infeasibilities;
	HighsInt num_primal_infeasibilities;
	int have_num_dual_infeasibilities;
	HighsInt num_dual_infeasibilities;
	int have_mip_primal_bound;
	double mip_primal_bound;
	int have_mip_gap;
	double mip_gap;
	int have_mip_dual_bound;
	double mip_dual_bound;
	int have_mip_node_count;
	int64_t mip_node_count;
	int have_mip_total_lp_iterations;
	int64_t mip_total_lp_iterations;
};

static int highs_get_info_int_value(const void *highs, const char *name, HighsInt *value){
	HighsInt status = Highs_getIntInfoValue(highs,name,value);
	return status == kHighsStatusOk || status == kHighsStatusWarning;
}

static int highs_get_info_double_value(const void *highs, const char *name, double *value){
	HighsInt status = Highs_getDoubleInfoValue(highs,name,value);
	return status == kHighsStatusOk || status == kHighsStatusWarning;
}

static int highs_get_info_int64_value(const void *highs, const char *name, int64_t *value){
	HighsInt status = Highs_getInt64InfoValue(highs,name,value);
	return status == kHighsStatusOk || status == kHighsStatusWarning;
}

static const char *highs_model_status_name(HighsInt model_status){
	switch(model_status){
		case kHighsModelStatusNotset: return "Notset";
		case kHighsModelStatusLoadError: return "LoadError";
		case kHighsModelStatusModelError: return "ModelError";
		case kHighsModelStatusPresolveError: return "PresolveError";
		case kHighsModelStatusSolveError: return "SolveError";
		case kHighsModelStatusPostsolveError: return "PostsolveError";
		case kHighsModelStatusModelEmpty: return "ModelEmpty";
		case kHighsModelStatusOptimal: return "Optimal";
		case kHighsModelStatusInfeasible: return "Infeasible";
		case kHighsModelStatusUnboundedOrInfeasible: return "UnboundedOrInfeasible";
		case kHighsModelStatusUnbounded: return "Unbounded";
		case kHighsModelStatusObjectiveBound: return "ObjectiveBound";
		case kHighsModelStatusObjectiveTarget: return "ObjectiveTarget";
		case kHighsModelStatusTimeLimit: return "TimeLimit";
		case kHighsModelStatusIterationLimit: return "IterationLimit";
		case kHighsModelStatusUnknown: return "Unknown";
		case kHighsModelStatusSolutionLimit: return "SolutionLimit";
		case kHighsModelStatusInterrupt: return "Interrupt";
		default: return "UnknownStatusCode";
	}
}

static const char *highs_solution_status_name(HighsInt solution_status){
	switch(solution_status){
		case kHighsSolutionStatusNone: return "None";
		case kHighsSolutionStatusInfeasible: return "Infeasible";
		case kHighsSolutionStatusFeasible: return "Feasible";
		default: return "Unknown";
	}
}

static const char *highs_basis_validity_name(HighsInt basis_validity){
	switch(basis_validity){
		case kHighsBasisValidityInvalid: return "Invalid";
		case kHighsBasisValidityValid: return "Valid";
		default: return "Unknown";
	}
}

static slv_solution_status_t highs_solution_status_to_slv(HighsInt solution_status){
	switch(solution_status){
		case kHighsSolutionStatusNone: return SLV_SOLUTION_STATUS_NONE;
		case kHighsSolutionStatusInfeasible: return SLV_SOLUTION_STATUS_INFEASIBLE;
		case kHighsSolutionStatusFeasible: return SLV_SOLUTION_STATUS_FEASIBLE;
		default: return SLV_SOLUTION_STATUS_UNKNOWN;
	}
}

static slv_basis_status_t highs_basis_validity_to_slv(HighsInt basis_validity){
	switch(basis_validity){
		case kHighsBasisValidityInvalid: return SLV_BASIS_STATUS_INVALID;
		case kHighsBasisValidityValid: return SLV_BASIS_STATUS_VALID;
		default: return SLV_BASIS_STATUS_UNKNOWN;
	}
}

static int32 highs_clamp_int64_to_int32(long long value){
	if(value > INT_MAX)return INT_MAX;
	if(value < INT_MIN)return INT_MIN;
	return (int32)value;
}

static void highs_fill_lp_status(
	slv_status_lp_t *lp, const struct highs_info_snapshot *info, HighsInt model_status
){
	if(lp == NULL || info == NULL)return;
	memset(lp,0,sizeof(*lp));
	lp->have_model_status = 1;
	lp->model_status = (int32)model_status;

	if(info->have_objective_function_value && isfinite(info->objective_function_value)){
		lp->have_objective = 1;
		lp->objective_value = info->objective_function_value;
	}
	if(info->have_primal_solution_status){
		lp->have_primal_status = 1;
		lp->primal_status = highs_solution_status_to_slv(info->primal_solution_status);
	}
	if(info->have_dual_solution_status){
		lp->have_dual_status = 1;
		lp->dual_status = highs_solution_status_to_slv(info->dual_solution_status);
	}
	if(info->have_basis_validity){
		lp->have_basis_status = 1;
		lp->basis_status = highs_basis_validity_to_slv(info->basis_validity);
	}
	if(info->have_max_primal_infeasibility && isfinite(info->max_primal_infeasibility)){
		lp->have_max_primal_infeas = 1;
		lp->max_primal_infeasibility = info->max_primal_infeasibility;
	}
	if(info->have_max_dual_infeasibility && isfinite(info->max_dual_infeasibility)){
		lp->have_max_dual_infeas = 1;
		lp->max_dual_infeasibility = info->max_dual_infeasibility;
	}
	if(info->have_num_primal_infeasibilities && info->num_primal_infeasibilities >= 0){
		lp->have_num_primal_infeas = 1;
		lp->num_primal_infeasibilities = highs_clamp_int64_to_int32((long long)info->num_primal_infeasibilities);
	}
	if(info->have_num_dual_infeasibilities && info->num_dual_infeasibilities >= 0){
		lp->have_num_dual_infeas = 1;
		lp->num_dual_infeasibilities = highs_clamp_int64_to_int32((long long)info->num_dual_infeasibilities);
	}
	if(info->have_simplex_iteration_count && info->simplex_iteration_count >= 0){
		lp->have_simplex_iterations = 1;
		lp->simplex_iterations = highs_clamp_int64_to_int32((long long)info->simplex_iteration_count);
	}
	if(info->have_ipm_iteration_count && info->ipm_iteration_count >= 0){
		lp->have_ipm_iterations = 1;
		lp->ipm_iterations = highs_clamp_int64_to_int32((long long)info->ipm_iteration_count);
	}
	if(info->have_pdlp_iteration_count && info->pdlp_iteration_count >= 0){
		lp->have_pdlp_iterations = 1;
		lp->pdlp_iterations = highs_clamp_int64_to_int32((long long)info->pdlp_iteration_count);
	}
}

static void highs_fill_mip_status(
	slv_status_mip_t *mip, const struct highs_info_snapshot *info, HighsInt model_status
){
	if(mip == NULL || info == NULL)return;
	memset(mip,0,sizeof(*mip));
	highs_fill_lp_status(&mip->lp, info, model_status);

	if(info->have_mip_primal_bound && isfinite(info->mip_primal_bound)){
		mip->have_primal_bound = 1;
		mip->primal_bound = info->mip_primal_bound;
	}
	if(info->have_mip_dual_bound && isfinite(info->mip_dual_bound)){
		mip->have_dual_bound = 1;
		mip->dual_bound = info->mip_dual_bound;
	}
	if(info->have_mip_gap && isfinite(info->mip_gap)){
		mip->have_gap = 1;
		mip->gap = info->mip_gap;
	}
	if(info->have_mip_node_count && info->mip_node_count >= 0){
		mip->have_node_count = 1;
		mip->node_count = (long long)info->mip_node_count;
	}
	if(info->have_mip_total_lp_iterations && info->mip_total_lp_iterations >= 0){
		mip->have_total_lp_iterations = 1;
		mip->total_lp_iterations = highs_clamp_int64_to_int32((long long)info->mip_total_lp_iterations);
	}
	if(mip->have_primal_bound && mip->have_dual_bound){
		mip->have_abs_gap = 1;
		mip->abs_gap = fabs(mip->primal_bound - mip->dual_bound);
	}
}

static void highs_collect_info_snapshot(const void *highs, struct highs_info_snapshot *info){
	memset(info,0,sizeof(*info));
	info->have_objective_function_value = highs_get_info_double_value(highs,"objective_function_value",&info->objective_function_value);
	info->have_simplex_iteration_count = highs_get_info_int_value(highs,"simplex_iteration_count",&info->simplex_iteration_count);
	info->have_ipm_iteration_count = highs_get_info_int_value(highs,"ipm_iteration_count",&info->ipm_iteration_count);
	info->have_pdlp_iteration_count = highs_get_info_int_value(highs,"pdlp_iteration_count",&info->pdlp_iteration_count);
	info->have_primal_solution_status = highs_get_info_int_value(highs,"primal_solution_status",&info->primal_solution_status);
	info->have_dual_solution_status = highs_get_info_int_value(highs,"dual_solution_status",&info->dual_solution_status);
	info->have_basis_validity = highs_get_info_int_value(highs,"basis_validity",&info->basis_validity);
	info->have_max_primal_infeasibility = highs_get_info_double_value(highs,"max_primal_infeasibility",&info->max_primal_infeasibility);
	info->have_max_dual_infeasibility = highs_get_info_double_value(highs,"max_dual_infeasibility",&info->max_dual_infeasibility);
	info->have_num_primal_infeasibilities = highs_get_info_int_value(highs,"num_primal_infeasibilities",&info->num_primal_infeasibilities);
	info->have_num_dual_infeasibilities = highs_get_info_int_value(highs,"num_dual_infeasibilities",&info->num_dual_infeasibilities);
	info->have_mip_primal_bound = highs_get_info_double_value(highs,"mip_primal_bound",&info->mip_primal_bound);
	info->have_mip_gap = highs_get_info_double_value(highs,"mip_gap",&info->mip_gap);
	info->have_mip_dual_bound = highs_get_info_double_value(highs,"mip_dual_bound",&info->mip_dual_bound);
	info->have_mip_node_count = highs_get_info_int64_value(highs,"mip_node_count",&info->mip_node_count);
	info->have_mip_total_lp_iterations = highs_get_info_int64_value(highs,"mip_total_lp_iterations",&info->mip_total_lp_iterations);
}

static int highs_has_feasible_primal_solution(const struct highs_info_snapshot *info){
	return info->have_primal_solution_status && info->primal_solution_status == kHighsSolutionStatusFeasible;
}

static void highs_update_status_flags_from_model_status(highs_system_t sys, HighsInt model_status){
	sys->s.converged = (model_status == kHighsModelStatusOptimal);
	sys->s.diverged = !sys->s.converged;
	sys->s.inconsistent = (
		model_status == kHighsModelStatusInfeasible
		|| model_status == kHighsModelStatusUnboundedOrInfeasible
	);
	sys->s.time_limit_exceeded = (model_status == kHighsModelStatusTimeLimit);
	sys->s.iteration_limit_exceeded = (model_status == kHighsModelStatusIterationLimit);
	sys->s.panic = (model_status == kHighsModelStatusInterrupt);
}

static int highs_total_iteration_count(const struct highs_info_snapshot *info){
	long long total = 0;
	int have_any = 0;
	if(info->have_simplex_iteration_count && info->simplex_iteration_count >= 0){
		total += (long long)info->simplex_iteration_count;
		have_any = 1;
	}
	if(info->have_ipm_iteration_count && info->ipm_iteration_count >= 0){
		total += (long long)info->ipm_iteration_count;
		have_any = 1;
	}
	if(info->have_pdlp_iteration_count && info->pdlp_iteration_count >= 0){
		total += (long long)info->pdlp_iteration_count;
		have_any = 1;
	}
	if(!have_any)return 0;
	if(total > INT_MAX)return INT_MAX;
	if(total < 0)return 0;
	return (int)total;
}

static void highs_diag_append(char *details, size_t cap, int *n, const char *fmt, ...){
	va_list ap;
	int wrote;
	size_t rem;
	if(*n < 0 || (size_t)(*n) >= cap)return;
	rem = cap - (size_t)(*n);
	va_start(ap,fmt);
	wrote = vsnprintf(details + (*n),rem,fmt,ap);
	va_end(ap);
	if(wrote < 0){
		*n = (int)cap;
		return;
	}
	if((size_t)wrote >= rem){
		*n = (int)cap;
		return;
	}
	*n += wrote;
}

static void highs_report_nonoptimal_status(
	HighsInt model_status, const struct highs_info_snapshot *info, int have_primal_solution
){
	const char *status_name = highs_model_status_name(model_status);
	error_severity_t sev = ASC_PROG_WARNING;
	char details[1024];
	int n = 0;
	if(
		model_status == kHighsModelStatusLoadError
		|| model_status == kHighsModelStatusModelError
		|| model_status == kHighsModelStatusPresolveError
		|| model_status == kHighsModelStatusSolveError
		|| model_status == kHighsModelStatusPostsolveError
	){
		sev = ASC_PROG_ERROR;
	}

	ERROR_REPORTER_HERE(sev
		,"HiGHS terminated with status %s (%ld)."
		,status_name,(long)model_status
	);

	if(info->have_primal_solution_status){
		highs_diag_append(details,sizeof(details),&n
			,"primal=%s"
			,highs_solution_status_name(info->primal_solution_status)
		);
	}
	if(info->have_dual_solution_status && n < (int)sizeof(details)){
		highs_diag_append(details,sizeof(details),&n
			,"%sdual=%s"
			,(n > 0 ? ", " : "")
			,highs_solution_status_name(info->dual_solution_status)
		);
	}
	if(info->have_basis_validity && n < (int)sizeof(details)){
		highs_diag_append(details,sizeof(details),&n
			,"%sbasis=%s"
			,(n > 0 ? ", " : "")
			,highs_basis_validity_name(info->basis_validity)
		);
	}
	if(
		info->have_objective_function_value
		&& isfinite(info->objective_function_value)
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%sobj=%.17g"
			,(n > 0 ? ", " : "")
			,info->objective_function_value
		);
	}
	if(
		info->have_simplex_iteration_count
		&& info->simplex_iteration_count >= 0
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%ssimplex_iter=%ld"
			,(n > 0 ? ", " : "")
			,(long)info->simplex_iteration_count
		);
	}
	if(
		info->have_ipm_iteration_count
		&& info->ipm_iteration_count >= 0
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%sipm_iter=%ld"
			,(n > 0 ? ", " : "")
			,(long)info->ipm_iteration_count
		);
	}
	if(
		info->have_pdlp_iteration_count
		&& info->pdlp_iteration_count >= 0
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%spdlp_iter=%ld"
			,(n > 0 ? ", " : "")
			,(long)info->pdlp_iteration_count
		);
	}
	if(info->have_mip_node_count && info->mip_node_count >= 0 && n < (int)sizeof(details)){
		highs_diag_append(details,sizeof(details),&n
			,"%smip_nodes=%lld"
			,(n > 0 ? ", " : "")
			,(long long)info->mip_node_count
		);
	}
	if(info->have_mip_gap && isfinite(info->mip_gap) && n < (int)sizeof(details)){
		highs_diag_append(details,sizeof(details),&n
			,"%smip_gap=%.17g"
			,(n > 0 ? ", " : "")
			,info->mip_gap
		);
	}
	if(
		info->have_max_primal_infeasibility
		&& isfinite(info->max_primal_infeasibility)
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%smax_primal_inf=%.17g"
			,(n > 0 ? ", " : "")
			,info->max_primal_infeasibility
		);
	}
	if(
		info->have_num_primal_infeasibilities
		&& info->num_primal_infeasibilities >= 0
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%snum_primal_inf=%ld"
			,(n > 0 ? ", " : "")
			,(long)info->num_primal_infeasibilities
		);
	}
	if(
		info->have_max_dual_infeasibility
		&& isfinite(info->max_dual_infeasibility)
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%smax_dual_inf=%.17g"
			,(n > 0 ? ", " : "")
			,info->max_dual_infeasibility
		);
	}
	if(
		info->have_num_dual_infeasibilities
		&& info->num_dual_infeasibilities >= 0
		&& n < (int)sizeof(details)
	){
		highs_diag_append(details,sizeof(details),&n
			,"%snum_dual_inf=%ld"
			,(n > 0 ? ", " : "")
			,(long)info->num_dual_infeasibilities
		);
	}
	if(n > 0){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"HiGHS diagnostics: %s.",details);
	}
	if(!have_primal_solution){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"HiGHS did not return a feasible primal solution.");
	}
}

void highs_solve(slv_system_t server){
	highs_system_t sys;
	void *highs = NULL;
	struct highs_problem_data p;
	HighsInt status;
	HighsInt model_status;
	HighsInt sense;
	int is_mip;
	int safeeval;
	int all_calc_ok;
	int have_primal_solution = 0;
	int iteration_count;
	struct highs_info_snapshot info;

	sys = SYS(server);
	memset(&info,0,sizeof(info));
	model_status = kHighsModelStatusNotset;
	MSG("starting HiGHS solve.");
	/* Domain relaxation now happens before scaling in the shared exporter.
	 * Preserve support for changing this option after ASCEND presolve.
	 */
	if(sys->mps.Ac_mtx && sys->prepared_relaxed!=SLV_PARAM_BOOL(&sys->p,HIGHS_PARAM_RELAXED)){
		highs_presolve(server);
	}

	/* make sure none of the LP data pointers are NULL */
	if ((sys->mps.Ac_mtx == NULL) ||
		(sys->mps.lbrow == NULL) ||
		(sys->mps.ubrow == NULL) ||
		(sys->mps.bcol == NULL) ||
		(sys->mps.typerow == NULL) ||
		(sys->mps.relopcol == NULL)
	){
		ERROR_REPORTER_HERE(ASC_PROG_ERR
			,"matrix representation is not available; presolve may not have been called."
		);
		return;
	}

	check_system(sys);
	if(!sys->s.ready_to_solve){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"not ready to solve.");
		return;
	}
	sys->s.converged = FALSE;
	sys->s.diverged = FALSE;
	sys->s.inconsistent = FALSE;
	sys->s.time_limit_exceeded = FALSE;
	sys->s.iteration_limit_exceeded = FALSE;
	sys->s.panic = FALSE;
	sys->s.kind = SLV_STATUS_LP;
	memset(&sys->s.u,0,sizeof(sys->s.u));
	sys->next_progress_report_time = 0.0;
	sys->progress_report_count = 0;

	sys->clock = tm_cpu_time();
	is_mip = 0;
	if(!highs_build_problem(sys,&p,&is_mip)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed constructing sparse LP/MIP arrays.");
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		sys->s.ready_to_solve = FALSE;
		return;
	}
	sys->s.kind = is_mip ? SLV_STATUS_MIP : SLV_STATUS_LP;
	memset(&sys->s.u,0,sizeof(sys->s.u));
	sys->s.iteration = 0;
	MSG(
		"constructed %s model: cols=%ld rows=%ld nz=%ld."
		,(is_mip ? "MIP" : "LP")
		,(long)p.num_col,(long)p.num_row,(long)p.num_nz
	);

	highs = Highs_create();
	if(highs == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed to create HiGHS instance.");
		highs_problem_data_free(&p);
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		sys->s.ready_to_solve = FALSE;
		return;
	}

	if(!highs_apply_options(sys,highs)){
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		goto done;
	}
	sense = (relman_obj_direction(sys->obj) == 1) ? kHighsObjSenseMaximize : kHighsObjSenseMinimize;
	if(is_mip){
		status = Highs_passMip(
			highs,p.num_col,p.num_row,p.num_nz
			,kHighsMatrixFormatColwise,sense,p.objective_offset
			,p.col_cost,p.col_lower,p.col_upper,p.row_lower,p.row_upper
			,p.a_start,p.a_index,p.a_value,p.integrality
		);
	}else{
		status = Highs_passLp(
			highs,p.num_col,p.num_row,p.num_nz
			,kHighsMatrixFormatColwise,sense,p.objective_offset
			,p.col_cost,p.col_lower,p.col_upper,p.row_lower,p.row_upper
			,p.a_start,p.a_index,p.a_value
		);
	}

	if(status == kHighsStatusError){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed passing model to HiGHS.");
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		goto done;
	}
	MSG("model loaded into HiGHS.");
	highs_enable_callbacks(sys,highs,is_mip);

	status = Highs_run(highs);
	model_status = Highs_getModelStatus(highs);
	MSG(
		"HiGHS run finished: status=%ld, model_status=%s (%ld)."
		,(long)status,highs_model_status_name(model_status),(long)model_status
	);
	if(status == kHighsStatusError){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"HiGHS run failed.");
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		goto done;
	}

	highs_collect_info_snapshot(highs,&info);
	highs_update_status_flags_from_model_status(sys,model_status);
	if(sys->s.kind == SLV_STATUS_MIP){
		highs_fill_mip_status(slv_status_mip_rw(&sys->s),&info,model_status);
	}else{
		highs_fill_lp_status(slv_status_lp_rw(&sys->s),&info,model_status);
	}
	have_primal_solution = highs_has_feasible_primal_solution(&info);
	MSG(
		"status flags: converged=%d inconsistent=%d diverged=%d time_limit=%d iter_limit=%d primal_feasible=%d."
		,sys->s.converged,sys->s.inconsistent,sys->s.diverged
		,sys->s.time_limit_exceeded,sys->s.iteration_limit_exceeded
		,have_primal_solution
	);
	if(sys->s.converged || have_primal_solution){
		if(
			Highs_getSolution(highs,p.col_value,p.col_dual,p.row_value,p.row_dual)
			== kHighsStatusError
		){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"unable to fetch HiGHS solution.");
			if(sys->s.converged){
				sys->s.converged = FALSE;
				sys->s.diverged = TRUE;
			}
			have_primal_solution = 0;
		}
	}

	if(sys->s.converged || have_primal_solution){
		safeeval = SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL);
		all_calc_ok = lp_write_solution(sys->slv,&sys->mps,p.col_value,safeeval) == 0;
		sys->s.calc_ok = all_calc_ok;

		if(!all_calc_ok){
			ERROR_REPORTER_HERE(ASC_PROG_WARNING,
				"%s but residual/objective refresh failed."
				,(sys->s.converged ? "converged" : "non-optimal solve returned feasible solution")
			);
		}
	}else if(!sys->s.converged){
		/* no writeback performed */
	}

	if(!sys->s.converged){
		highs_report_nonoptimal_status(model_status,&info,have_primal_solution);
	}

done:
	if(sys->s.panic){
		slv_set_solver_interrupt(0);
	}
	sys->s.cpu_elapsed += (double)(tm_cpu_time() - sys->clock);
	sys->s.ok = sys->s.calc_ok && sys->s.converged;
	sys->s.ready_to_solve = FALSE;
	iteration_count = highs_total_iteration_count(&info);
	if(iteration_count <= 0 && sys->s.iteration > 0){
		iteration_count = sys->s.iteration;
	}
	sys->s.iteration = iteration_count;
	highs_spoof_block_status(sys);
	MSG(
		"final status: ok=%d converged=%d calc_ok=%d ready_to_solve=%d iter=%d."
		,sys->s.ok,sys->s.converged,sys->s.calc_ok,sys->s.ready_to_solve,sys->s.iteration
	);

	if(highs)Highs_destroy(highs);
	highs_problem_data_free(&p);
}


void highs_iterate(slv_system_t server){
	highs_system_t sys;
	sys = SYS(server);
  /*  Writing an MPS file is a one shot deal.  Thus, an interation
      is equivalent to solving the problem.  So we just call
      highs_solve   */

   check_system(sys);
   highs_solve(server);
}


void highs_resolve(slv_system_t server){
   highs_presolve(server);
   highs_solve(server);
}

/* Adapters from modern solver API (server + token) to legacy highs callbacks. */
static int highs_client_destroy(slv_system_t server, SlvClientToken asys){
	(void)server;
	return highs_destroy((slv_system_t)asys, asys);
}

static int highs_client_eligible_solver(slv_system_t server){
	SlvClientToken asys = slv_get_client_token(server);
	if(asys == NULL){
		return 0;
	}
	return highs_eligible_solver((highs_system_t)asys) ? 1 : 0;
}

static void highs_client_get_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	(void)server;
	highs_get_parameters((slv_system_t)asys, parameters);
}

static void highs_client_set_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	(void)server;
	highs_set_parameters((slv_system_t)asys, parameters);
}

static int highs_client_get_status(slv_system_t server, SlvClientToken asys, slv_status_t *status){
	(void)server;
	highs_get_status((slv_system_t)asys, status);
	return 0;
}

static int highs_client_solve(slv_system_t server, SlvClientToken asys){
	(void)server;
	highs_solve((slv_system_t)asys);
	return 0;
}

static int highs_client_presolve(slv_system_t server, SlvClientToken asys){
	(void)server;
	highs_presolve((slv_system_t)asys);
	return 0;
}

static int highs_client_iterate(slv_system_t server, SlvClientToken asys){
	(void)server;
	highs_iterate((slv_system_t)asys);
	return 0;
}

static int highs_client_resolve(slv_system_t server, SlvClientToken asys){
	(void)server;
	highs_resolve((slv_system_t)asys);
	return 0;
}

static int highs_client_get_version(char *buf, size_t buflen){
	const char *version;
	if(buf == NULL || buflen == 0){
		return 1;
	}
	version = Highs_version();
	if(version != NULL && version[0] != '\0'){
		snprintf(buf,buflen,"HiGHS %s",version);
	}else{
		snprintf(buf,buflen,"HiGHS %d.%d.%d"
			,(int)Highs_versionMajor()
			,(int)Highs_versionMinor()
			,(int)Highs_versionPatch()
		);
	}
	return 0;
}


static const SlvFunctionsT highs_client_internals = {
	highs_solver_number
	,"HiGHS"
	,highs_create
  	,highs_client_destroy
	,highs_client_eligible_solver
	,highs_get_default_parameters
	,highs_client_get_parameters
	,highs_client_set_parameters
	,highs_client_get_status
	,highs_client_solve
	,highs_client_presolve
	,highs_client_iterate
	,highs_client_resolve
	,NULL
	,NULL
	,NULL
};


int highs_register(void){
	if(solver_register(&highs_client_internals)){
		return 1;
	}
	solver_register_version("HiGHS",highs_client_get_version);
	return 0;
}
