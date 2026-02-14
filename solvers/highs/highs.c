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
#include <string.h>

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
   struct slv_parameter pa[SP6_PARAMS];

   slv_status_t           s;            /* Status flags */
   double                 clock;        /* CPU time */

   /**
    ***  Calculated Data
    ***
    **/
   mps_data_t  mps;          /* the main chunk of data for the problem */

};


static int highs_get_default_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
	struct slv_parameter *new_parms = NULL;

	(void)server;
	(void)asys;
	highs_link_capi_stubs();

	if(parameters->parms == NULL) {
		new_parms = ASC_NEW_ARRAY_OR_NULL(struct slv_parameter,SP6_PARAMS);
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

	/** Integer and Bool Options */

	slv_param_bool(parameters,SP6_NONLIN
		,(SlvParameterInitBool){{"nonlin"
			,"Linearise non-linear equations?",1
			,"Perform linearisation of non-linear models at the current point (TRUE)"
			" or else require a linear model (FALSE)."
		}, FALSE}
	);


	slv_param_bool(parameters,SP6_RELAXED
		,(SlvParameterInitBool){{"relaxed"
			,"Solve LP relaxation?",1
			,"Solve regular problem (FALSE) or LP relaxation of problem (TRUE)."
		}, FALSE}
	);

	slv_param_bool(parameters,SP6_NONNEG
		,(SlvParameterInitBool){{"nonneg"
			,"Require non-negative?",1
			,"Solver handles free vars (FALSE) or solver requires that all vars have LB=0, UB=infinity, no FR or MI"
		}, FALSE}
	);

	slv_param_int(parameters,SP6_OBJ
		,(SlvParameterInitInt){{"obj"
			,"Objective function type",2
			,"0->solver assumes minimization, do nothing special; 1->solver assumes maximization, swap obj coeff for min problems; 2->solver support SCICONIC style MINIMIZE; 3->solver supports QOMILP style MAX/MIN in names section"
		}, 0, 0, 3}
	);

	slv_param_int(parameters,SP6_BINARY
		,(SlvParameterInitInt){{"binary"
			,"Binary variable support",2
			,"0->solver supports binary variables using INTORG; 1->solver supports binary variables with BV option in BOUNDS; 2->no support"
		}, 2, 0, 2}
	);

	slv_param_int(parameters,SP6_INTEGER
		,(SlvParameterInitInt){{"integer"
			,"Integer variable method",2
			,"0->solver defines integer vars using INTORG; 1->solver defines integer vars using UI in BOUNDS; 2->no support for integer vars"
		}, 2, 0, 2}
	);


	slv_param_bool(parameters,SP6_SEMI
		,(SlvParameterInitBool){{"semi"
			,"Semi-continuous support?",2
			,"0->no support; 1->solver supports SCICONIC style semi-continuous vars"
		}, FALSE}
	);

	slv_param_bool(parameters,SP6_SOS1
		,(SlvParameterInitBool){{"sos1"
			,"SOS1 support?",2
			,"0->no support; 1->solver supports SOS1, i.e. sum(Xi) = 1"
		}, FALSE}
	);

	slv_param_bool(parameters,SP6_SOS2
		,(SlvParameterInitBool){{"sos2"
			,"SOS2 support? (ignored)",2
			,"This parameter currently ignored; no support for type 2 yet. 0->no support; 1->solver supports SOS2, i.e. sum(xi) <=2, with 2 nonzeros being adjacent"
		}, FALSE}
	);

	slv_param_bool(parameters,SP6_SOS3
		,(SlvParameterInitBool){{"sos3"
			,"SOS3 support?",2
			,"0->no support; 1->solver supports SOS3, i.e. sum(xi) <= 1"
		}, FALSE}
	);

	slv_param_bool(parameters,SP6_BO
		,(SlvParameterInitBool){{"bo"
			,"BO current bound support?",3
			,"0->no support; 1->solver supports QOMILP-style BO cutoff bound in names section. Note: value of bound is set in 'bndval'."
		}, FALSE}
	);

	slv_param_bool(parameters,SP6_EPS
		,(SlvParameterInitBool){{"eps"
			,"EPS termination criterion support?",4
			,"0->no support; 1->solver supports QOMILP-style EPS termination criterion. Note: value of bound is set in 'epsval'."
		}, FALSE}
	);


	slv_param_real(parameters,SP6_BOVAL
		,(SlvParameterInitReal){{"boval"
			,"BO cutoff bound value",3
			,"Value of QOMILP style BO cutoff bound in names section. Ignored if 'bo' is FALSE."
		}, 0, -1e99, 1.e99}
	);

	slv_param_real(parameters,SP6_EPSVAL
		,(SlvParameterInitReal){{"epsval"
			,"EPS termination criterion value",4
			,"Value of QOMILP-style EPS termination criterion. Note: Ignored if 'eps' is FALSE."
		}, 0, -1e99, 1.e99}
	);

	slv_param_real(parameters,SP6_PINF
		,(SlvParameterInitReal){{"pinf"
			,"Positive 'infinity' value",5
			,"Any upper bound greater than 'pinf' will be set to +infinity"
		}, 1e30, 0, 1.e99}
	);

	slv_param_real(parameters,SP6_MINF
		,(SlvParameterInitReal){{"minf"
			,"Minus 'infinity' value",5
			,"Any lower bound greater than 'minf' will be set to -infinity"
		}, -1e30, -1e99, 0}
	);

	slv_param_char(parameters,SP6_FILENAME
		,(SlvParameterInitChar){{"filename"
			,"Output filename",1
			,"Name of the output file to be created."
		}, "outfile.txt"}, (char *[]){
			"outfile.txt","outfile1.txt","outfile2.txt","outfile3.txt",NULL
		} /* FIXME how to specify that the user can type this in as free text? */
	);

	asc_assert(parameters->num_parms==SP6_PARAMS);

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

static boolean inc_rel_filter(struct rel_relation *rel)
/**
 ***  Returns true if rel is an incident relation.
 **/
{
	return lp_inc_rel_filter(rel);
}


/* _________________________________________________________________________ */

/*
 ***  Routines to calculate the mps problem representation
 ***  --------------------
 ***  var_relaxed - is the variable relaxed or not?
 ***  calc_c - calculate c vector, coefficients of objective
        (called by calc_matrix)
 ***  calc_bounds - convert var bounds to an array of numbers
 ***  calc_reloplist - convert relation operators <=, >=, = to array of numbers
 ***  calc_svtlist - create array of numbers containing var types
 ***  calc_matrix - compute entire matrix representaion of problem
 **/


/**
 ***  Calculate gradient of the objective function. (or any expression, for that matter)
 ***  On the linear system we should have, this is the c vector of
 ***  our problem max/min {cx: Ax<=b}.
 ***  On nonlinear problems is the linearization of problem at current point
 **/
static boolean calc_c(mtx_matrix_t mtx,     /* matrix to store derivs */
                      int32 org_row,  /* original number of row to store them */
                      struct rel_relation  *obj)           /* expression to diffs */
{
	return lp_calc_c(mtx,org_row,obj);
}


/**
 **  Stores the upper or lower bounds of all non-fixed, incident vars
 **  in a new array, which is returned by the routine
 **/
static real64 *calc_bounds(struct var_variable **vlist, /* variable list to get bounds */
                                 int32 vused,      /* number of variables in solver list */
                                 boolean upper)         /* do upper, else lower */

{
	return lp_calc_bounds(vlist,vused,upper);
}


/**
 ***  This function constructs the a list of relational operators: <=, >=, =
 ***  from the relations list.  The values for each relational operator
 ***  corresponds to rel_TOK_less, rel_TOK_equal, and rel_TOK_greater.
 ***  (Defined in rel.h)
 ***  Or rel_TOK_nonincident for relations that aren't incident (see slv6.h)
 ***
 ***  Note: the rel_less, rel_equal, and rel_greater routines in rel.h don't
 ***  behave as you'd expect, and should _not_ be used.  Use rel_type instead.
 **/
static char *calc_reloplist(struct rel_relation **rlist,
                            int32    rused)   /* entry for each relation */
{
	return lp_calc_reloplist(rlist,rused);
}

/**
	This function constructs the solver var list from the variable list.

	WARNING:  This routine assumes that a struct var_variable *is an Instance.
	In the future this is going to change, and this routine will break.

	UPDATE: have made some steps to fixing above wrong assumption -- Feb 2011 JP.
 **/
static char *calc_svtlist( struct var_variable **vlist,    /* input, not modified */
                           int32 vused,        /* number of vars (incident or nonincident, free or fixed) */
                           int *solver_var_used,     /* number of each type of var are cached  */
                           int *solver_relaxed_used,
                           int *solver_int_used,
                           int *solver_binary_used,
                           int *solver_semi_used,
                           int *solver_other_used,
                           int *solver_fixed){
	return lp_calc_svtlist(
		vlist
		,vused
		,solver_var_used
		,solver_relaxed_used
		,solver_int_used
		,solver_binary_used
		,solver_semi_used
		,solver_other_used
		,solver_fixed
	);
}

static mtx_matrix_t calc_matrix(int32     cap,
                                int32     rused,
                                int32     vused,
                                struct rel_relation  **rlist,
                                struct rel_relation *obj,
                                int32     crow,
                                slv_status_t    *s,
                                int32     *rank,
                                real64    **rhs_orig)
/**
	@param     cap,          in: capacity of matrix
	@param    rused,        in: total number of relations used
	@param    vused,        in: total number of variables used
	@param rlist,       in: Relation list (NULL terminated)
	@param obj,          in: objective function
	@param crow,         in: row to store objective row
	@param s,          out: s.block.jactime, and s.calc_ok
	@param rank,       out
	@oaram rhs_orig   out: rhs array origin

 ***  Creates and calculates a matrix representation of the A matrix, c row,
 ***  and the RHS or b column (which is stored in rhs_orig).
 ***  On nonlinear problems is the linearization of problem at current point.
 ***
 *** Note: the residual stored in the rhs array is not the real right hand side.
 ***  The residual returned by the diffs call is just the value of
 **         (lhs expr) - (rhs expr)
 ***  we want the residual excluding the current variables.
 ***  At the moment there isn't a
 ***  clean way to do this.  It will be calculated in the real_rhs routine.
 ***  This routine
 ***  will take for each relation:
 ***         sum(i, (Jacobian value var[i])*(Variable value var[i])) - rhs[i]
 ***  which is the real rhs.
 ***  However, this will _not_ be valid for the nonlinear case.
 ***  Other than this, the mps file will be the linearization of a nonlinear
 ***  system at the
 ***  current point.  If you are adding an MINLP feature,
 ***  you'll need to come up with a better way.
 ***
<pre>
 ***       MPS matrix strucutre
 ***                                    v
 ***       min/max cx:                  u
 ***       Ax (<= = >=) b               s
 ***                                    e
 ***                       1            d
 ***
 ***                       |            |
 ***                       |            |
 ***                      \ /          \ /
 ***
 ***                      +-            -+
 ***       1          ->  |              |
 ***                      |              |
 ***                      |      A       |
 ***                      |              |
 ***       rused      ->  |              |
 ***                      +-            -+
 ***
 ***       crow       ->  [      c       ]
 ***
 ***
 ***       rused                 row of last incident relation
 ***       crow = rused + 1,     row of cost vector
 ***       vused                 column of last incident variable
 ***
 ***       cap = max(vused+1,rused), size of sparse square matrix
 ***       cap = N ---> row/column 0 to N-1 exist
</pre>
*/
{
	(void)calc_c;
	return lp_calc_matrix(cap,rused,vused,rlist,obj,crow,s,rank,rhs_orig);
}


static void real_rhs(mtx_matrix_t    Ac_mtx,      /* Matrix representation of problem */
                     char            relopcol[],  /* is it incident? */
                     struct var_variable  **vlist,      /* Variable list (NULL terminated) */
                     int32     rused,       /* in: total number of relations used */
                     int32     vused,       /* in: total number of variables used */
                     real64    rhs[])       /* out: rhs array origin */
/**
 ***  Takes the residuals stored in rhs, and converts them into the actual right
 ***  hand sides we want.
 ***
 ***  Note: the residual stored in the rhs array is not the real right hand side.
 ***  The residual returned by the diffs call is just the value of
 **         (lhs expr) - (rhs expr)
 ***  we want the residual excluding the current variables.  At the moment there isn't a
 ***  clean way to do this.  It will be calculated in the real_rhs routine.  This routine
 ***  will take for each relation:
 ***         sum(i, (Jacobian value var[i])*(Variable value var[i])) - rhs[i]
 ***  which is the real rhs.  However, this will _not_ be valid for the nonlinear case.
 ***  Other than this, the mps file will be the linearization of a nonlinear system at the
 ***  current point.  If you are adding an MINLP feature, you'll need to come up with a better way.
 ***
 **/
{
	lp_real_rhs(Ac_mtx,relopcol,vlist,rused,vused,rhs);
}

/* _________________________________________________________________________ */

/**
 ***  Routines used by presolve
 ***  --------------------
 ***  ensure_bounds - fix inconsistent bounds
 ***  update_vlist - add vars to vlist
 ***  determine_vlist - build new vlist
 **/


static void ensure_bounds(FILE *mif,highs_system_t sys, struct var_variable *var)
/**
 ***  Ensures that the variable value is within its bounds.
 **/
{
	lp_ensure_bounds(mif,sys->slv,var);
}

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
	sys->mps.typerow = NULL;
	sys->mps.relopcol = NULL;


	/***  Initialize status flags ***/

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

	sys->s.block.number_of            = 1;      /* always 1, just have 1 block */
	sys->s.block.current_block        = 0;      /* always 1, start in first and only block */
	sys->s.block.current_size         = 0;      /* set to sys->mps.vused in highs_presolve */
	sys->s.block.previous_total_size  = 0;      /* always 0, never used */

	/* same : */
	sys->s.block.iteration            = 0;      /* set to 0 after highs_presolve; set to 1 after highs_solve */
	sys->s.iteration                  = 0;      /* set to 0 after highs_presolve; set to 1 after highs_solve */

	/* same : */
	sys->s.block.cpu_elapsed          = 0.0;    /* set to time taken by highs_presolve and highs_solve */
	sys->s.cpu_elapsed                = 0.0;    /* set to time taken by highs_presolve and highs_solve */

	sys->s.block.functime             = 0.0;    /* always 0.0 since no function evaluation, never used */
	sys->s.block.residual             = 0.0;    /* always 0.0 since not iterating, never used */
	sys->s.block.jactime              = 0.0;    /* calculated in highs_presolve, time for jacobian eval */

	sys->s.costsize                   = sys->s.block.number_of;  /* just one cost block, which will be set in  */

	sys->s.cost=create_zero_array(sys->s.costsize,struct slv_block_cost);  /* allocate memory */


	/* Note: the cost vars are equivalent to other sys->s.* vars

	sys->s.cost->size        = sys->s.block.current_size
	sys->s.cost->iterations  = sys->s.block.iteration
	sys->s.cost->jacs        = sys->s.block.iteration
	sys->s.cost->funcs       = always 0 since no function evals needed
	sys->s.cost->time        = sys->s.block.cpu_elapsed
	sys->s.cost->resid       = 0.0  whatever this is ?
	sys->s.cost->functime    = 0.0  since no function evals needed
	sys->s.cost->jactime     = sys->s.block.jactime

	*/

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
	if (sys->s.cost) ascfree(sys->s.cost);  /* deallocate cost array */

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

   /*  Check that the system is linear if iarray[SP6_NONLIN] == 0 */
   if (SLV_PARAM_BOOL(&(sys->p),SP6_NONLIN) == 0){
      for( rp=sys->rlist ; *rp != NULL ; ++rp )   /* check relations */
          if(!relman_is_linear(*rp,&vfilter)) {
            char *relname = rel_make_name(sys->slv,*rp);
            ERROR_REPORTER_HERE(ASC_PROG_ERR
               ,"With current settings, HiGHS requires linear models; nonlinearity in constraint '%s'."
               ,(relname ? relname : "<unknown>")
            );
            ASC_FREE(relname);
            return(FALSE);   /* don't do nonlinearities */
          }
      if(!relman_is_linear(sys->obj,&vfilter)){
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
	highs_system_t sys;
	sys = SYS(server);

   struct var_variable **vp;
   struct rel_relation **rp;

   /* Check if necessary pointers are non-NULL */
   check_system(sys);
   if( sys->vlist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"variable list was never set.");
      return;
   }
   if( sys->blist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"boundary list was never set.");
      return;
   }
   if( sys->rlist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"relation list was never set.");
      return;
   }

   /* time presolve */
   sys->clock = tm_cpu_time();  /* record start time */

#if 0
/*  set up vlist, if necessary, and set all vars, rels, and boundary's
    to being nonincident, set up index scheme */

#ifndef KILL
   if( sys->vlist_user == NULL ) determine_vlist(sys);
#else
   if( sys->vlist_user == NULL ){
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"automatic variable-list setup is broken.");
     exit(1);
   }
#endif
   sys->mps.cap = 0;
   for( vp=sys->vlist,cap=0 ; *vp != NULL ; ++vp ) {
      var_set_sindex(*vp,cap++);
      var_set_in_block(*vp,FALSE);
   }
   sys->mps.cap = cap;
   for( rp=sys->rlist,cap=0 ; *rp != NULL ; ++rp ) {
      rel_set_sindex(*rp,cap++);
      rel_set_in_block(*rp,FALSE);
      rel_set_satisfied(*rp,FALSE);
   }
   sys->mps.cap = MAX(sys->mps.cap,cap+1);   /* allow an extra relation for crow,
                                                cap = N --> row/col 0 to N-1 exist */
   for( bp = sys->blist ; *bp != NULL ; ++bp ) {
      bnd_set_in_block(*bp,FALSE);
      bnd_set_active(*bp,FALSE);
   }

    /**
    ***  Now mark all variables appearing in the objective function,
    ***  the boundaries and the relations as incident.
    **/

   /* Mark all variables appearing in the objective function as incident */
   if( sys->obj ) exprman_decide_incidence_obj(sys->obj);

      /* Set incidence of included vars in included bounds, calc bused, set all bounds inactive */
   sys->mps.bused = 0;
   bfilter.included = bnd_true;
   bfilter.in_block = bnd_ignore;
   for( bp = sys->blist ; *bp != NULL ; bp++ ) {
      if( bnd_apply_filter(*bp,&bfilter) ) {
	  bndman_decide_incidence(*bp);  /* mark incident variables */
          sys->mps.bused++;
      }
   }

   /* Count the incident relations in rused */
   sys->mps.rused = 0;
   sys->mps.rinc = 0;
   for( rp = sys->rlist ; *rp != NULL ; rp++ ) {
      rel_set_satisfied(*rp,FALSE);
      if( inc_rel_filter(*rp) ) {
     	 relman_decide_incidence(*rp);   /* mark incident variables in included rels */
         sys->mps.rinc++;
      }
      sys->mps.rused++;
   }

#endif
	  /* Legacy incidence setup above is disabled; compute the core counts here. */
	   sys->mps.rused = 0;
	   sys->mps.rinc = 0;
	   for( rp = sys->rlist ; *rp != NULL ; ++rp ) {
	      if( inc_rel_filter(*rp) ) {
	         sys->mps.rinc++;
	      }
	      sys->mps.rused++;
	   }

	      /* compute info for variables */
	   sys->mps.vused = 0;     /* number starting at 0 */
	   sys->mps.vinc = 0;
	   for( vp = sys->vlist ; *vp != NULL ; vp++ ) {
	      if( highs_free_inc_var_filter(*vp) )
	          sys->mps.vinc++;
	      sys->mps.vused++;    /* count up incident, non-fixed vars */
	   }
	   if(sys->mps.vinc == 0){
	      /* Fallback for systems where incident flags are not pre-marked. */
	      var_filter_t active_free;
	      active_free.matchbits = (VAR_FIXED | VAR_ACTIVE);
	      active_free.matchvalue = VAR_ACTIVE;
	      for( vp = sys->vlist ; *vp != NULL ; ++vp ) {
	         if( var_apply_filter(*vp,&active_free) ){
	            sys->mps.vinc++;
	         }
	      }
	   }

	   /* calculate values for other index_mps_t vars */
	   sys->mps.cap = sys->mps.vused;
	   if(sys->mps.cap < sys->mps.rused + 1){
	      sys->mps.cap = sys->mps.rused + 1;
	   }
	   sys->mps.crow     = sys->mps.rused;    /* note rused = N means rows 0 to N-1, exist,
	                                             the next one will be numbered rused */
	   /* calculate rank later */

	   /* Call highs_elgibile_solver to see if the solver has a chance */
	   /* If not bail now ... requires the incidence values of prev section be set */
	   if(! highs_eligible_solver(sys)) {
	      ERROR_REPORTER_HERE(ASC_PROG_ERR,"model is not eligible with current options.");
	      return;
	   }

   /*  Make sure that at least one incident variable and at least one incident
       relation exist, else bail */
   if ((sys->mps.rinc == 0) || (sys->mps.vinc == 0))  {
      ERROR_REPORTER_HERE(ASC_PROG_ERR
         ,"model must have at least one incident variable and equation (incident variables=%d, incident equations=%d)."
         ,sys->mps.vinc,sys->mps.rinc
      );
      return;
   }

   /* free memory, and set all pointers to NULL */
   nuke_pointers(&(sys->mps));

   /* setup matrix representaion of problem */
   sys->mps.Ac_mtx = calc_matrix(sys->mps.cap,
                                 sys->mps.rused,
                                 sys->mps.vused,
                                 sys->rlist,
                                 sys->obj,
                                 sys->mps.crow,
                                 &sys->s,        /* how long for the jacobian calcs and any errors */
                                 &sys->mps.rank,
                                 &sys->mps.bcol);
   if( sys->mps.Ac_mtx == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed to build matrix representation.");
      nuke_pointers(&(sys->mps));
      return;
   }

   /* get upper bound row */
   sys->mps.ubrow = calc_bounds(sys->vlist, sys->mps.vused, TRUE);
   if (sys->mps.ubrow == NULL)  {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"error calculating variable upper bounds.");
      nuke_pointers(&(sys->mps));
      return;
   }

   /* get lower bound row */
   sys->mps.lbrow = calc_bounds(sys->vlist, sys->mps.vused, FALSE);
   if (sys->mps.lbrow == NULL)  {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"error calculating variable lower bounds.");
      nuke_pointers(&(sys->mps));
      return;
   }

	/* Call calc_svtlist to allocate array of variable types */
	sys->mps.typerow = calc_svtlist(sys->vlist,
	                                sys->mps.vused,
	                                &sys->mps.solver_var_used,      /* output */
                                   &sys->mps.solver_relaxed_used,  /* output */
                                   &sys->mps.solver_int_used,      /* output */
                                   &sys->mps.solver_binary_used,   /* output */
                                   &sys->mps.solver_semi_used,     /* output */
                                   &sys->mps.solver_other_used,    /* output */
                                   &sys->mps.solver_fixed);        /* output */
   if(sys->mps.typerow == NULL) {         /* allocation failed */
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"error calculating variable type list.");
      nuke_pointers(&(sys->mps));
      return;
   }

   /* Call calc_reloplist here, to calculate the relational operators >=, <=, = */
	 sys->mps.relopcol = calc_reloplist(sys->rlist, sys->mps.rused);
    if(sys->mps.relopcol == NULL) {         /* allocation failed */
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"error calculating relational operators.");
      nuke_pointers(&(sys->mps));
      return;
   }

   /* adjust the rhs vector so it actually contains the rhs */
   real_rhs(sys->mps.Ac_mtx,      /* Matrix representation of problem */
            sys->mps.relopcol,    /* is it incident? */
            sys->vlist,           /* in: Variable list (NULL terminated) */
            sys->mps.rused,       /* in: total number of relations used */
            sys->mps.vused,       /* in: total number of variables used */
            sys->mps.bcol);       /* out: rhs array origin */


   /* Call ensure_bounds over all vars to make bounds self-consistent */
   for( vp=sys->vlist; *vp != NULL ; ++vp )
     ensure_bounds(NULL,sys, *vp);

   /* Reset status flags */
   sys->s.over_defined = (sys->mps.rinc > sys->mps.vinc);
   sys->s.under_defined = (sys->mps.rinc < sys->mps.vinc);
   sys->s.struct_singular = (sys->mps.rank < sys->mps.rinc);
   /* HiGHS can solve LP/MIP models even with rank-deficient row sets. */
   sys->s.ok = sys->s.calc_ok;
   sys->s.ready_to_solve = sys->s.ok;

   sys->s.converged = FALSE;      /* changes to true after highs_solve */
   sys->s.block.current_size = sys->mps.vused;
   sys->s.cost->size = sys->s.block.current_size;

   sys->s.cpu_elapsed       = (double)(tm_cpu_time() - sys->clock);  /* record times */
   sys->s.block.cpu_elapsed = sys->s.cpu_elapsed;
   sys->s.cost->time        = sys->s.cpu_elapsed;
   sys->s.cost->jactime     = sys->s.block.jactime;  /* from calc_matrix */

   sys->s.block.iteration   = 0;  /* reset iteration "count", changes to 1 after highs_solve */
   sys->s.iteration         = 0;
   sys->s.cost->iterations  = 0;
   sys->s.cost->jacs        = 0;

}

struct highs_problem_data{
	HighsInt num_col;
	HighsInt num_row;
	HighsInt num_nz;
	HighsInt *a_start;
	HighsInt *a_index;
	double *a_value;
	double *col_cost;
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
	int32 rused, vused, orgrow, orgcol, rowcount, nnzmax;
	int32 currow, curcol;
	int32 nnz;
	int *row_map = NULL;
	int relaxed;
	double pinf, minf, hinf;
	mtx_coord_t nz;
	mtx_range_t range;
	real64 a;
	int direction;

	rused = sys->mps.rused;
	vused = sys->mps.vused;
	relaxed = SLV_PARAM_BOOL(&(sys->p),SP6_RELAXED);
	pinf = SLV_PARAM_REAL(&(sys->p),SP6_PINF);
	minf = SLV_PARAM_REAL(&(sys->p),SP6_MINF);
	hinf = 1e30;
	*is_mip = 0;

	memset(p,0,sizeof(*p));

	row_map = ASC_NEW_ARRAY_OR_NULL(int,rused);
	if(row_map == NULL)return 0;
	for(orgrow = 0; orgrow < rused; ++orgrow){
		row_map[orgrow] = -1;
	}

	rowcount = 0;
	for(orgrow = 0; orgrow < rused; ++orgrow){
		if(sys->mps.relopcol[orgrow] != rel_TOK_nonincident){
			row_map[orgrow] = rowcount++;
		}
	}

	nnzmax = 0;
	for(orgcol = 0; orgcol < vused; ++orgcol){
		curcol = mtx_org_to_col(sys->mps.Ac_mtx,orgcol);
		if(curcol < 0)continue;
		nz.col = curcol;
		nz.row = mtx_FIRST;
		a = mtx_next_in_col(sys->mps.Ac_mtx,&nz,mtx_range(&range,0,rused-1));
		while(nz.row != mtx_LAST){
			(void)a;
			++nnzmax;
			a = mtx_next_in_col(sys->mps.Ac_mtx,&nz,mtx_range(&range,0,rused-1));
		}
	}

	p->num_col = (HighsInt)vused;
	p->num_row = (HighsInt)rowcount;
	p->num_nz = 0;

	p->a_start = ASC_NEW_ARRAY_OR_NULL(HighsInt,vused+1);
	p->a_index = ASC_NEW_ARRAY_OR_NULL(HighsInt,MAX(nnzmax,1));
	p->a_value = ASC_NEW_ARRAY_OR_NULL(double,MAX(nnzmax,1));
	p->col_cost = ASC_NEW_ARRAY_OR_NULL(double,vused);
	p->col_lower = ASC_NEW_ARRAY_OR_NULL(double,vused);
	p->col_upper = ASC_NEW_ARRAY_OR_NULL(double,vused);
	p->integrality = ASC_NEW_ARRAY_OR_NULL(HighsInt,vused);
	p->row_lower = ASC_NEW_ARRAY_OR_NULL(double,MAX(rowcount,1));
	p->row_upper = ASC_NEW_ARRAY_OR_NULL(double,MAX(rowcount,1));
	p->col_value = ASC_NEW_ARRAY_OR_NULL(double,vused);
	p->col_dual = ASC_NEW_ARRAY_OR_NULL(double,vused);
	p->row_value = ASC_NEW_ARRAY_OR_NULL(double,MAX(rowcount,1));
	p->row_dual = ASC_NEW_ARRAY_OR_NULL(double,MAX(rowcount,1));
	if(
		p->a_start == NULL || p->a_index == NULL || p->a_value == NULL
		|| p->col_cost == NULL || p->col_lower == NULL || p->col_upper == NULL
		|| p->integrality == NULL || p->row_lower == NULL || p->row_upper == NULL
		|| p->col_value == NULL || p->col_dual == NULL
		|| p->row_value == NULL || p->row_dual == NULL
	){
		ascfree(row_map);
		return 0;
	}

	for(orgcol = 0; orgcol < vused; ++orgcol){
		p->col_cost[orgcol] = 0.0;
		p->integrality[orgcol] = kHighsVarTypeContinuous;
		p->col_lower[orgcol] = (sys->mps.lbrow[orgcol] <= minf) ? -hinf : sys->mps.lbrow[orgcol];
		p->col_upper[orgcol] = (sys->mps.ubrow[orgcol] >= pinf) ? hinf : sys->mps.ubrow[orgcol];
		if(!relaxed){
			switch(sys->mps.typerow[orgcol]){
				case MPS_INT:
				case MPS_BINARY:
					p->integrality[orgcol] = kHighsVarTypeInteger;
					*is_mip = 1;
					break;
				case MPS_SEMI:
					p->integrality[orgcol] = kHighsVarTypeSemiContinuous;
					*is_mip = 1;
					break;
				default:
					break;
			}
		}
	}

	for(orgrow = 0; orgrow < rused; ++orgrow){
		int ridx = row_map[orgrow];
		if(ridx < 0)continue;
		switch(sys->mps.relopcol[orgrow]){
			case rel_TOK_less:
				p->row_lower[ridx] = -hinf;
				p->row_upper[ridx] = sys->mps.bcol[orgrow];
				break;
			case rel_TOK_greater:
				p->row_lower[ridx] = sys->mps.bcol[orgrow];
				p->row_upper[ridx] = hinf;
				break;
			case rel_TOK_equal:
				p->row_lower[ridx] = sys->mps.bcol[orgrow];
				p->row_upper[ridx] = sys->mps.bcol[orgrow];
				break;
			default:
				p->row_lower[ridx] = -hinf;
				p->row_upper[ridx] = hinf;
				break;
		}
	}

	currow = mtx_org_to_row(sys->mps.Ac_mtx,sys->mps.crow);
	if(currow >= 0){
		nz.row = currow;
		nz.col = mtx_FIRST;
		a = mtx_next_in_row(sys->mps.Ac_mtx,&nz,mtx_range(&range,0,vused-1));
		while(nz.col != mtx_LAST){
			orgcol = mtx_col_to_org(sys->mps.Ac_mtx,nz.col);
			if(orgcol >= 0 && orgcol < vused){
				p->col_cost[orgcol] = a;
			}
			a = mtx_next_in_row(sys->mps.Ac_mtx,&nz,mtx_range(&range,0,vused-1));
		}
	}

	nnz = 0;
	for(orgcol = 0; orgcol < vused; ++orgcol){
		curcol = mtx_org_to_col(sys->mps.Ac_mtx,orgcol);
		p->a_start[orgcol] = (HighsInt)nnz;
		if(curcol < 0)continue;
		nz.col = curcol;
		nz.row = mtx_FIRST;
		a = mtx_next_in_col(sys->mps.Ac_mtx,&nz,mtx_range(&range,0,rused-1));
		while(nz.row != mtx_LAST){
			orgrow = mtx_row_to_org(sys->mps.Ac_mtx,nz.row);
			if(orgrow >= 0 && orgrow < rused && row_map[orgrow] >= 0){
				p->a_index[nnz] = (HighsInt)row_map[orgrow];
				p->a_value[nnz] = a;
				++nnz;
			}
			a = mtx_next_in_col(sys->mps.Ac_mtx,&nz,mtx_range(&range,0,rused-1));
		}
	}
	p->a_start[vused] = (HighsInt)nnz;
	p->num_nz = (HighsInt)nnz;

	direction = relman_obj_direction(sys->obj);
	if(direction == 1){
		/* nothing to do: HiGHS model sense will be set to maximize */
	}

	ascfree(row_map);
	return 1;
}

void highs_solve(slv_system_t server){
	highs_system_t sys;
	void *highs = NULL;
	struct highs_problem_data p;
	HighsInt status;
	HighsInt model_status;
	HighsInt sense;
	int is_mip;
	struct var_variable **vp;
	struct rel_relation **rp;
	int safeeval;
	int calc_ok;
	int all_calc_ok;

	sys = SYS(server);

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

	sys->clock = tm_cpu_time();
	is_mip = 0;
	if(!highs_build_problem(sys,&p,&is_mip)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed constructing sparse LP/MIP arrays.");
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		sys->s.ready_to_solve = FALSE;
		return;
	}

	highs = Highs_create();
	if(highs == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed to create HiGHS instance.");
		highs_problem_data_free(&p);
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		sys->s.ready_to_solve = FALSE;
		return;
	}

	(void)Highs_setBoolOptionValue(highs,"output_flag",0);
	sense = (relman_obj_direction(sys->obj) == 1) ? kHighsObjSenseMaximize : kHighsObjSenseMinimize;
	if(is_mip){
		status = Highs_passMip(
			highs,p.num_col,p.num_row,p.num_nz
			,kHighsMatrixFormatColwise,sense,0.0
			,p.col_cost,p.col_lower,p.col_upper,p.row_lower,p.row_upper
			,p.a_start,p.a_index,p.a_value,p.integrality
		);
	}else{
		status = Highs_passLp(
			highs,p.num_col,p.num_row,p.num_nz
			,kHighsMatrixFormatColwise,sense,0.0
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

	status = Highs_run(highs);
	model_status = Highs_getModelStatus(highs);
	if(status == kHighsStatusError){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"HiGHS run failed.");
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		goto done;
	}

	if(
		Highs_getSolution(highs,p.col_value,p.col_dual,p.row_value,p.row_dual)
		== kHighsStatusError
	){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"unable to fetch HiGHS solution.");
		sys->s.converged = FALSE;
		sys->s.diverged = TRUE;
		goto done;
	}

	sys->s.converged = (model_status == kHighsModelStatusOptimal);
	sys->s.diverged = !sys->s.converged;
	sys->s.inconsistent = (
		model_status == kHighsModelStatusInfeasible
		|| model_status == kHighsModelStatusUnboundedOrInfeasible
	);

	if(sys->s.converged){
		for(vp = sys->vlist; *vp != NULL; ++vp){
			int32 orgcol = var_sindex(*vp);
			if(orgcol >= 0 && orgcol < sys->mps.vused){
				var_set_value(*vp,p.col_value[orgcol]);
			}
		}

		safeeval = SLV_PARAM_BOOL(&(sys->p),ASCEND_PARAM_SAFEEVAL);
		all_calc_ok = 1;
		calc_ok = 1;

		if(sys->obj != NULL){
			(void)relman_eval(sys->obj,&calc_ok,safeeval);
			if(!calc_ok)all_calc_ok = 0;
		}
		for(rp = sys->rlist; *rp != NULL; ++rp){
			if(inc_rel_filter(*rp)){
				(void)relman_eval(*rp,&calc_ok,safeeval);
				if(!calc_ok)all_calc_ok = 0;
			}
		}
		sys->s.calc_ok = all_calc_ok;

		if(!all_calc_ok){
			ERROR_REPORTER_HERE(ASC_PROG_WARNING
				,"converged but residual/objective refresh failed."
			);
		}
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_WARNING
			,"model status is %ld (not optimal)."
			,(long)model_status
		);
	}

done:
	sys->s.cpu_elapsed += (double)(tm_cpu_time() - sys->clock);
	sys->s.block.cpu_elapsed = sys->s.cpu_elapsed;
	sys->s.cost->time = sys->s.cpu_elapsed;
	sys->s.ok = sys->s.calc_ok && sys->s.converged;
	sys->s.ready_to_solve = FALSE;
	sys->s.block.iteration = 1;
	sys->s.iteration = 1;
	sys->s.cost->iterations = 1;
	sys->s.cost->jacs = 1;

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
	highs_system_t sys;
	sys = SYS(server);

  /* This routine is meant to be called when the following parts of
     the system change:
       - any parameter except "partition".
       - variable values.
       - variable nominal values.
       - variable bounds.
     However, if var values or bounds change, we need a new MPS file,
     so there is no way to use the previous solution.
     Just call highs_solve, and do it the normal way.
  */

   check_system(sys);
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
	return solver_register(&highs_client_internals);
}
