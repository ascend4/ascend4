/*
 *  MPS: Ascend MPS file generator
 *  by Craig Schmidt
 *  Created: 2/11/95
 *  Version: $Revision: 1.29 $
 *  Version control file: $RCSfile: slv6.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:38 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Craig Schmidt
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*  known bugs
 *  still uses pl_ functions and assumes the old slv protocol.
 */

#include "slv6.h"
#include "mps.h"

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

#include <ascend/linear/mtx.h>

#include <ascend/system/calc.h>
#include <ascend/system/relman.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/bnd.h>
#include <ascend/system/var.h>
#include <ascend/system/rel.h>
#include <ascend/system/lp_utils.h>

#ifndef KILL
#define KILL TRUE
#endif
#define DEBUG FALSE

#define SYS(s) ((slv6_system_t)(s))

ASC_DLLSPEC SolverRegisterFn makemps_register;

struct slv6_system_structure {

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


static int slv6_get_default_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
	struct slv_parameter *new_parms = NULL;
	(void)server;
	(void)asys;

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
static int check_system(slv6_system_t sys)
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
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System was recently destroyed.");
      return 1;
   default:
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System reused or never allocated.");
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
 ***  Routines for common filters
 ***  -----------------
 ***  free_inc_var_filter -  true for non-fixed incident variables
 ***  inc_rel_filter      -  true for incident relations
 **/

/**
	I've been calling this particular var filter a lot ,
	so I decided to make it a subroutine.  Returns true if
	var is not fixed and incident in something.
*/
extern boolean free_inc_var_filter(struct var_variable *var){
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


static void ensure_bounds(FILE *mif,slv6_system_t sys, struct var_variable *var)
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
static void determine_vlist(slv6_system_t sys){
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
      slv6_set_var_list(sys,NULL);
}

#endif

/* _________________________________________________________________________ */

/**
 ***  External routines used from slv0 without modificiation
 ***
 ***  slv6_set_var_list(sys,vlist)
 ***  slv6_get_var_list(sys)
 ***  slv6_set_bnd_list(sys,blist)
 ***  slv6_get_bnd_list(sys)
 ***  slv6_set_rel_list(sys,rlist)
 ***  slv6_get_rel_list(sys)
 ***  slv6_set_extrel_list(sys,erlist)
 ***  slv6_get_extrel_list(sys)
 ***  slv6_count_vars(sys,vfilter)
 ***  slv6_count_bnds(sys,bfilter)
 ***  slv6_count_rels(sys,rfilter)
 ***  slv6_set_obj_function(sys,obj)
 ***  slv6_get_obj_function(sys)
 ***  slv6_get_parameters(sys,parameters)
 ***  slv6_set_parameters(sys,parameters)
 ***  slv6_get_status(sys,status)
 ***  slv6_dump_internals(sys,level)
 **/


#if 0
void slv6_set_var_list(slv6_system_t sys, struct var_variable **vlist){
   static struct var_variable *empty_list[] = {NULL};
   check_system(sys);
   if( sys->vlist_user == NULL )
      if( sys->vlist != NULL && pl_length(sys->vlist) > 0 )
	 ascfree( (POINTER)(sys->vlist) );
   sys->vlist_user = vlist;
   sys->vlist = (vlist==NULL ? empty_list : vlist);
   sys->s.ready_to_solve = FALSE;
}

struct var_variable **slv6_get_var_list(slv6_system_t sys){
   check_system(sys);
   return( sys->vlist_user );
}

void slv6_set_bnd_list(slv6_system_t sys, struct bnd_boundary *blist){
   static struct bnd_boundary empty_list[] = {};
   check_system(sys);
   sys->blist_user = blist;
   sys->blist = (blist==NULL ? empty_list : blist);
   sys->s.ready_to_solve = FALSE;
}

struct bnd_boundary *slv6_get_bnd_list(sys)
slv6_system_t sys;
{
   check_system(sys);
   return( sys->blist_user );
}

void slv6_set_rel_list(sys,rlist)
slv6_system_t sys;
struct rel_relation **rlist;
{
   static struct rel_relation *empty_list[] = {NULL};
   check_system(sys);
   sys->rlist_user = rlist;
   sys->rlist = (rlist==NULL ? empty_list : rlist);
   sys->s.ready_to_solve = FALSE;
}

struct rel_relation **slv6_get_rel_list(sys)
slv6_system_t sys;
{
   check_system(sys);
   return( sys->rlist_user );
}

void slv6_set_extrel_list(sys,erlist)
slv6_system_t sys;
struct ExtRelCache **erlist;
{
   static struct ExtRelCache *empty_list[] = {NULL};
   check_system(sys);
   sys->erlist_user = erlist;
   sys->erlist = (erlist==NULL ? empty_list : erlist);
   sys->s.ready_to_solve = FALSE;
}

struct ExtRelCache **slv6_get_extrel_list(sys)
slv6_system_t sys;
{
   check_system(sys);
   return( sys->erlist_user );
}

int slv6_count_vars(sys,vfilter)
slv6_system_t sys;
var_filter_t *vfilter;
{
   struct var_variable **vp;
   int32 count = 0;
   check_system(sys);
   for( vp=sys->vlist; *vp != NULL; vp++ )
      if( var_apply_filter(*vp,vfilter) ) ++count;
   return( count );
}

int slv6_count_bnds(slv6_system_t sys,bnd_filter_t *bfilter){
	struct bnd_boundary *bp;
	int32 count = 0;
	check_system(sys);
	for( bp=sys->blist; *bp != NULL; bp++ ){
		if( bnd_apply_filter(*bp,bfilter) ) ++count;
	}
	return( count );
}

int slv6_count_rels(slv6_system_t sys,rel_filter_t *rfilter){
   struct rel_relation **rp;
   int32 count = 0;
   check_system(sys);
   for( rp=sys->rlist; *rp != NULL; rp++ )
      if( rel_apply_filter(*rp,rfilter) ) ++count;
   return( count );
}

void slv6_set_obj_relation(slv6_system_t sys,struct rel_relation *obj){
   check_system(sys);
   sys->obj = obj;
   sys->s.ready_to_solve = FALSE;
}

struct rel_relation *slv6_get_obj_relation(slv6_system_t sys){
   check_system(sys);
   return(sys->obj);
}

void slv6_dump_internals(slv6_system_t sys, int level){
   check_system(sys);
   if (level > 0) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Dumping internals is not implemented.");
   }
}
#endif

void slv6_get_parameters(slv_system_t server,slv_parameters_t *parameters){
	slv6_system_t sys;
	sys = SYS(server);
	check_system(sys);
	mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

void slv6_set_parameters(slv_system_t server, slv_parameters_t *parameters){
	slv6_system_t sys;
	sys = SYS(server);
	check_system(sys);
	if (parameters->whose==slv6_solver_number)
	mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

void slv6_get_status(slv_system_t server, slv_status_t *status){
	slv6_system_t sys;
	sys = SYS(server);
	check_system(sys);
	mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}

/* _________________________________________________________________________ */

/**
 ***  External routines with minor modifications
 ***  -----------------
 ***  slv6_change_basis           just return FALSE & error msg
 **/

boolean slv6_change_basis(slv6_system_t sys,int32 var, mtx_range_t *rng){
/* In the MPS file maker, changing the basis doesn't make any sense.
   Nor, for that matter, is there a basis in the first place.
   So I just write out an error message, and return FALSE  */

   ERROR_REPORTER_HERE(ASC_PROG_ERR,"Changing basis is not supported.");

   return FALSE;
}


/* _________________________________________________________________________ */

/*
 ***  External routines unique to slv6 (Based on routines from slv0)
 ***  -----------------
 ***  slv6_create()               added solver specific initialization
 ***  slv6_destroy(sys)           added solver specific dealocation
 ***  slv6_eligible_solver(sys)   see if solver can do the current problem
 ***  slv6_presolve(sys)          set up system and create matrix/vectors
 ***  slv6_solve(sys)             call MPS routines
 ***  slv6_iterate(sys)           just calls slv6_solve
 ***  slv6_resolve(sys)           just calls slv6_solve
 **/


/**
	This routine allocates memory and initializes all data structures
	It should be a good source of comments on the system parameters and
	status flags used in slv6
*/
static SlvClientToken slv6_create(slv_system_t server, int32 *statusindex){   /* added mps initialization */
	slv6_system_t sys;

	sys = ASC_NEW_CLEAR(struct slv6_system_structure);
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
	slv6_get_default_parameters(server,(SlvClientToken)sys,&(sys->p));
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
	sys->p.whose = slv6_solver_number;      /* read in slv6_set_parameters */
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

	sys->s.over_defined               = FALSE;  /* set to (sys->mps.rinc > sys->mps.vinc) in slv6_presolve */
	sys->s.under_defined              = FALSE;  /* set to (sys->mps.rinc < sys->mps.vinc) in slv6_presolve */
	sys->s.struct_singular            = FALSE;  /* set to (sys->mps.rank < sys->mps.rinc) in slv6_presolve */
	sys->s.calc_ok                    = TRUE;   /* set in calc_matrix (FALSE if error occurs with diffs calc) */
	sys->s.ok                         = TRUE;   /* set to (sys->s.calc_ok && !sys->s.struct_singular) in slv6_presolve */
	sys->s.ready_to_solve             = FALSE;  /* set to (sys->.ok) after slv6_presolve,
		                                       set FALSE after:  slv6_set_var_list, slv6_set_bnd_list,
		                                           slv6_set_rel_list, slv6_set_extrel_list, slv6_set_obj_function
		                                       tested in slv6_solve */
	sys->s.converged                  = FALSE;  /* set FALSE after slv6_presolve; set TRUE after slv6_solve */
	sys->s.diverged                   = FALSE;  /* always FALSE, never used */
	sys->s.inconsistent               = FALSE;  /* always FALSE, never used */
	sys->s.iteration_limit_exceeded   = FALSE;  /* always FALSE, never used */
	sys->s.time_limit_exceeded        = FALSE;  /* always FALSE, never used */

	sys->s.block.number_of            = 1;      /* always 1, just have 1 block */
	sys->s.block.current_block        = 0;      /* always 1, start in first and only block */
	sys->s.block.current_size         = 0;      /* set to sys->mps.vused in slv6_presolve */
	sys->s.block.previous_total_size  = 0;      /* always 0, never used */

	/* same : */
	sys->s.block.iteration            = 0;      /* set to 0 after slv6_presolve; set to 1 after slv6_solve */
	sys->s.iteration                  = 0;      /* set to 0 after slv6_presolve; set to 1 after slv6_solve */

	/* same : */
	sys->s.block.cpu_elapsed          = 0.0;    /* set to time taken by slv6_presolve and slv6_solve */
	sys->s.cpu_elapsed                = 0.0;    /* set to time taken by slv6_presolve and slv6_solve */

	sys->s.block.functime             = 0.0;    /* always 0.0 since no function evaluation, never used */
	sys->s.block.residual             = 0.0;    /* always 0.0 since not iterating, never used */
	sys->s.block.jactime              = 0.0;    /* calculated in slv6_presolve, time for jacobian eval */

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

static int slv6_destroy(slv_system_t server, SlvClientToken asys){
	slv6_system_t sys;
	sys = SYS(server);
	//int i;
	if(server == NULL || sys==NULL)return 1;

	if(check_system(sys))return 1;
#if 0
	slv6_set_var_list(sys,(struct var_variable **)NULL);
	//slv6_set_obj_function(sys,NULL);
	slv6_set_bnd_list(sys,NULL);
	slv6_set_rel_list(sys,NULL);
	slv6_set_extrel_list(sys,NULL);
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
	slv6_eligible_solver will return true
 */
boolean slv6_eligible_solver(slv6_system_t server){
	slv6_system_t sys;
	sys = SYS(server);

   struct rel_relation **rp;
   var_filter_t vfilter;

   check_system(sys);
   if( sys->rlist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Relation list was never set.");
      return (FALSE);
   }
   if( sys->obj == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"No objective in problem.");
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
               ,"With current settings, MakeMPS requires linear models; nonlinearity in constraint '%s'."
               ,(relname ? relname : "<unknown>")
            );
            ASC_FREE(relname);
            return(FALSE);   /* don't do nonlinearities */
          }
      if(!relman_is_linear(sys->obj,&vfilter)){
          char *relname = rel_make_name(sys->slv,sys->obj);
          ERROR_REPORTER_HERE(ASC_PROG_ERR
             ,"With current settings, MakeMPS requires linear models; nonlinearity in objective '%s'."
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

void slv6_presolve(slv_system_t server){
	slv6_system_t sys;
	sys = SYS(server);

   struct var_variable **vp;
   struct rel_relation **rp;

   /* Check if necessary pointers are non-NULL */
   check_system(sys);
   if( sys->vlist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Variable list was never set.");
      return;
   }
   if( sys->blist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Boundary list was never set.");
      return;
   }
   if( sys->rlist == NULL ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Relation list was never set.");
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
	      if( free_inc_var_filter(*vp) )
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

	   /* Call slv6_elgibile_solver to see if the solver has a chance */
	   /* If not bail now ... requires the incidence values of prev section be set */
	   if(! slv6_eligible_solver(sys)) {
	      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Model is not eligible for MakeMPS export with current options.");
	      return;
	   }

   /*  Make sure that at least one incident variable and at least one incident
       relation exist, else bail */
   if ((sys->mps.rinc == 0) || (sys->mps.vinc == 0))  {
      ERROR_REPORTER_HERE(ASC_PROG_ERR
         ,"Your model must have at least one incident variable and equation (incident variables: %d, incident equations: %d)."
         ,sys->mps.vinc, sys->mps.rinc
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
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in calculating variable upper bounds.");
      nuke_pointers(&(sys->mps));
      return;
   }

   /* get lower bound row */
   sys->mps.lbrow = calc_bounds(sys->vlist, sys->mps.vused, FALSE);
   if (sys->mps.lbrow == NULL)  {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in calculating variable lower bounds.");
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
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in calculating the variable type list!");
      nuke_pointers(&(sys->mps));
      return;
   }

   /* Call calc_reloplist here, to calculate the relational operators >=, <=, = */
	 sys->mps.relopcol = calc_reloplist(sys->rlist, sys->mps.rused);
    if(sys->mps.relopcol == NULL) {         /* allocation failed */
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in calculating the relational operators!");
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
   /* MakeMPS exports are valid even when the row set is rank-deficient. */
   sys->s.ok = sys->s.calc_ok;
   sys->s.ready_to_solve = sys->s.ok;

   sys->s.converged = FALSE;      /* changes to true after slv6_solve */
   sys->s.block.current_size = sys->mps.vused;
   sys->s.cost->size = sys->s.block.current_size;

   sys->s.cpu_elapsed       = (double)(tm_cpu_time() - sys->clock);  /* record times */
   sys->s.block.cpu_elapsed = sys->s.cpu_elapsed;
   sys->s.cost->time        = sys->s.cpu_elapsed;
   sys->s.cost->jactime     = sys->s.block.jactime;  /* from calc_matrix */

   sys->s.block.iteration   = 0;  /* reset iteration "count", changes to 1 after slv6_solve */
   sys->s.iteration         = 0;
   sys->s.cost->iterations  = 0;
   sys->s.cost->jacs        = 0;

}

void slv6_solve(slv_system_t server){
	slv6_system_t sys;
	sys = SYS(server);

   /* make sure none of the mps pointers are NULL */
   if ((sys->mps.Ac_mtx == NULL) ||
       (sys->mps.lbrow == NULL) ||
       (sys->mps.ubrow == NULL) ||
       (sys->mps.bcol == NULL) ||
       (sys->mps.typerow == NULL) ||
       (sys->mps.relopcol == NULL)) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR
         ,"Matrix representation of problem is not available; presolve may not have been called."
      );
      return;
   }

   /* Check system to see if it can be solved  */
   check_system(sys);
   if( !sys->s.ready_to_solve ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not ready to solve.");
      return;
   }

   sys->clock = tm_cpu_time();   /* record start time for solve */


 /*  FPRINTF(MIF(sys),"_________________________________________\n");
   mtx_write_region_human(MIF(sys), sys->mps.Ac_mtx, mtx_ENTIRE_MATRIX);
   FPRINTF(MIF(sys),"_________________________________________\n");
 */

#define FN SLV_PARAM_CHAR(&(sys->p),SP6_FILENAME)

   /* Call write_mps to create the mps file */
   write_MPS(FN,     /* filename for output */
             sys->mps,                      /* main chunk of data */
             &(sys->p));

   /* replace .mps with .map at end of filename */
   *(FN+strlen(FN)-2) = 'a';
   *(FN+strlen(FN)-1) = 'p';

   /* writes out a file mapping the CXXXXXXX variable names with the actual ASCEND names */
   write_name_map(FN,   /* user-specified filename */
                  sys->vlist);
#undef FN



   sys->s.cpu_elapsed += (double)(tm_cpu_time() - sys->clock);
   /* compute total elapsed time */
   sys->s.block.cpu_elapsed = sys->s.cpu_elapsed;
   sys->s.cost->time        = sys->s.cpu_elapsed;

   sys->s.converged = TRUE;
   sys->s.ready_to_solve = FALSE;   /* !sys->s.converged  */

   sys->s.block.iteration   = 1;  /* change iteration "count", goes to 0 after slv6_presolve */
   sys->s.iteration         = 1;
   sys->s.cost->iterations  = 1;
   sys->s.cost->jacs        = 1;
   sys->s.ready_to_solve = FALSE;

}


void slv6_iterate(slv_system_t server){
	slv6_system_t sys;
	sys = SYS(server);
  /*  Writing an MPS file is a one shot deal.  Thus, an interation
      is equivalent to solving the problem.  So we just call
      slv6_solve   */

   check_system(sys);
   slv6_solve(server);
}


void slv6_resolve(slv_system_t server){
	slv6_system_t sys;
	sys = SYS(server);

  /* This routine is meant to be called when the following parts of
     the system change:
       - any parameter except "partition".
       - variable values.
       - variable nominal values.
       - variable bounds.
     However, if var values or bounds change, we need a new MPS file,
     so there is no way to use the previous solution.
     Just call slv6_solve, and do it the normal way.
  */

   check_system(sys);
	slv6_solve(server);
}

/* Adapters from modern solver API (server + token) to legacy slv6 callbacks. */
static int makemps_destroy(slv_system_t server, SlvClientToken asys){
	(void)server;
	return slv6_destroy((slv_system_t)asys, asys);
}

static int makemps_eligible_solver(slv_system_t server){
	SlvClientToken asys = slv_get_client_token(server);
	if(asys == NULL){
		return 0;
	}
	return slv6_eligible_solver((slv6_system_t)asys) ? 1 : 0;
}

static void makemps_get_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	(void)server;
	slv6_get_parameters((slv_system_t)asys, parameters);
}

static void makemps_set_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	(void)server;
	slv6_set_parameters((slv_system_t)asys, parameters);
}

static int makemps_get_status(slv_system_t server, SlvClientToken asys, slv_status_t *status){
	(void)server;
	slv6_get_status((slv_system_t)asys, status);
	return 0;
}

static int makemps_solve(slv_system_t server, SlvClientToken asys){
	(void)server;
	slv6_solve((slv_system_t)asys);
	return 0;
}

static int makemps_presolve(slv_system_t server, SlvClientToken asys){
	(void)server;
	slv6_presolve((slv_system_t)asys);
	return 0;
}

static int makemps_iterate(slv_system_t server, SlvClientToken asys){
	(void)server;
	slv6_iterate((slv_system_t)asys);
	return 0;
}

static int makemps_resolve(slv_system_t server, SlvClientToken asys){
	(void)server;
	slv6_resolve((slv_system_t)asys);
	return 0;
}


static const SlvFunctionsT makemps_internals = {
	6
	,"MakeMPS"
	,slv6_create
  	,makemps_destroy
	,makemps_eligible_solver
	,slv6_get_default_parameters
	,makemps_get_parameters
	,makemps_set_parameters
	,makemps_get_status
	,makemps_solve
	,makemps_presolve
	,makemps_iterate
	,makemps_resolve
	,NULL
	,NULL
	,NULL
};


int makemps_register(void){
	return solver_register(&makemps_internals);
}
