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
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */
/*  known bugs
 *  still uses pl_ functions and assumes the old slv protocol.
 */
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/set.h>
#include <utilities/mem.h>
#include <general/tm_time.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/module.h>
#include <compiler/compiler.h>
#include <compiler/library.h>
#include <compiler/instance_io.h>
#include "mtx.h"
#include "linsol.h"
#include "linsolqr.h"
#include "slv_types.h"
#include "var.h"
#include "rel.h"
#include "discrete.h"
#include "conditional.h"
#include "logrel.h"
#include "bnd.h"
#include "calc.h"
#include "relman.h"
#include "slv_common.h"
#include "slv_client.h"
#include "slv6.h"
#include "mps.h"
/*
#  include "interface/old_utils.h"
*/


#if !defined(STATIC_MPS) && !defined(DYNAMIC_MPS)
int slv6_register(SlvFunctionsT *f)
{
  (void)f;  /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"makeMPS not compiled in this ASCEND IV.\n");
  return 1;
}
#else /* either STATIC_MPS or DYNAMIC_MPS is defined */
#ifdef DYNAMIC_MPS
/* do dynamic loading stuff.   yeah, right */
#else /* following is used if STATIC_MPS is defined */

#ifndef KILL
#define KILL TRUE
#endif
#define DEBUG FALSE

struct slv6_system_structure {

   /**
    ***  Problem definition
    **/
   slv_system_t           slv;          /* slv_system_t back-link */
   struct rel_relation    *obj;          /* Objective function: NULL = none */
   struct var_variable    **vlist;       /* Variable list (NULL terminated) */
   struct var_variable    **vlist_user;  /* User vlist (NULL = determine) */
   bnd_boundary_t         *blist;       /* Boundary list (NULL terminated) */
   bnd_boundary_t         *blist_user;  /* User blist (NULL = none) */
   struct rel_relation    **rlist;       /* Relation list (NULL terminated) */
   struct rel_relation    **rlist_user;  /* User rlist (NULL = none) */
   struct ExtRelCache     **erlist;     /* External relations cache list */
   struct ExtRelCache     **erlist_user;/* User erlist (NULL = none) */

   /**
    ***  Solver information
    **/
   int                    integrity;    /* ? Has the system been created */
   slv_parameters_t       p;            /* Parameters */
   slv_status_t           s;            /* Status flags */
   double                 clock;        /* CPU time */
   int iarray[slv6_IA_SIZE];            /* Integer subparameters */
   double rarray[slv6_RA_SIZE];         /* Real subparameters */
   char *carray[slv6_CA_SIZE];          /* Charptr subparameter */

   /**
    ***  Calculated Data
    ***
    **/
   mps_data_t  mps;          /* the main chunk of data for the problem */

};

/* _________________________________________________________________________ */

/**
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
      FPRINTF(stderr,"ERROR:  (slv6) check_system\n");
      FPRINTF(stderr,"        NULL system handle.\n");
      return 1;
   }

   switch( sys->integrity ) {
   case OK:
      return 0;
   case DESTROYED:
      FPRINTF(stderr,"ERROR:  (slv6) check_system\n");
      FPRINTF(stderr,"        System was recently destroyed.\n");
      return 1;
   default:
      FPRINTF(stderr,"ERROR:  (slv6) check_system\n");
      FPRINTF(stderr,"        System reused or never allocated.\n");
      return 1;
   }
}

/* _________________________________________________________________________ */

/**
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
   ((len) > 0 ? (type *)ascmalloc((len)*sizeof(type)) : NULL)
#define create_zero_array(len,type)  \
   ((len) > 0 ? (type *)asccalloc((len),sizeof(type)) : NULL)
#define zero_array(arr,nelts,type)    \
   mem_zero_byte_cast((arr),0,(nelts)*sizeof(type))
/* Zeros an array of nelts objects, each having given type. */


static void nuke_pointers(mps_data_t mps) { /* free all allocated memory in mps data structure */

   if (mps.Ac_mtx != NULL) {       /* delete old matrix if the exist */
       mtx_destroy(mps.Ac_mtx);
       mps.Ac_mtx = NULL;
   }

   if (mps.lbrow != NULL) {        /* delete old vector if it exists */
       destroy_array(mps.lbrow);
       mps.lbrow = NULL;
   }

   if (mps.ubrow != NULL) {        /* delete old vector if it exists */
       destroy_array(mps.ubrow);
       mps.ubrow = NULL;
   }

   if (mps.bcol != NULL) {         /* delete old vector if the exist */
       destroy_array(mps.bcol);
       mps.bcol = NULL;
   }

   if (mps.typerow != NULL) {      /* delete old vector if it exists */
       destroy_array(mps.typerow);
       mps.typerow = NULL;
   }

   if (mps.relopcol != NULL) {     /* delete old vector if it exists */
       destroy_array(mps.relopcol);
       mps.relopcol = NULL;
   }
}

/* _________________________________________________________________________ */

/**
 ***  General input/output routines
 ***  -----------------------------
 ***     fp = MIF(sys)
 ***     fp = LIF(sys)
 **/

static FILE *get_output_file(FILE *fp)
/**
 ***  Returns fp if fp!=NULL, or a file pointer
 ***  open to nul device if fp == NULL.
 **/
{
   static FILE *nuldev = NULL;
   static char fname[] = "/dev/null";

   if( fp==NULL ) {
      if(nuldev==NULL)
	 if( (nuldev=fopen(fname,"w")) == NULL ) {
	    FPRINTF(stderr,"ERROR:  (slv6) get_output_file\n");
	    FPRINTF(stderr,"        Unable to open %s.\n",fname);
	 }
      fp=nuldev;
   }
   return(fp);
}

/*   #define MIF(sys) get_output_file( (sys)->p.output.more_important )
 *   #define LIF(sys) get_output_file( (sys)->p.output.less_important )
 */

/* _________________________________________________________________________ */

/**
 ***  Routines for common filters
 ***  -----------------
 ***  free_inc_var_filter -  true for non-fixed incident variables
 ***  inc_rel_filter      -  true for incident relations
 **/

extern boolean free_inc_var_filter(struct var_variable *var)
/**
 ***  I've been calling this particular var filter a lot ,
 ***  so I decided to make it a subroutine.  Returns true if
 ***  var is not fixed and incident in something.
 **/
{
      var_filter_t vfilter;
      vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
      vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);

      /*     vfilter.fixed = var_false;*/            /* calc for all non-fixed vars */
      /* vfilter.incident = var_true;  */        /* incident vars only */
      /* vfilter.in_block = var_ignore; */

      return var_apply_filter(var,&vfilter);
}

static boolean inc_rel_filter(struct rel_relation *rel)
/**
 ***  Returns true if rel is an incident relation.
 **/
{
   rel_filter_t rfilter;  /* filter for included rels */
   rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
   rfilter.matchvalue = (REL_INCLUDED| REL_ACTIVE );
   /*   rfilter.included = rel_true;
   rfilter.equality = rel_ignore;
   rfilter.in_block = rel_ignore;
   rfilter.in_subregion = rel_ignore; */

   return rel_apply_filter(rel,&rfilter);
}


/* _________________________________________________________________________ */

/**
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

static boolean calc_c(mtx_matrix_t mtx,     /* matrix to store derivs */
                      int32 org_row,  /* original number of row to store them */
                      struct rel_relation  *obj)           /* expression to diffs */
/**
 ***  Calculate gradient of the objective function. (or any expression, for that matter)
 ***  On the linear system we should have, this is the c vector of
 ***  our problem max/min {cx: Ax<=b}.
 ***  On nonlinear problems is the linearization of problem at current point
 **/
{
      var_filter_t vfilter;
	  int32 row;

      if ((mtx == NULL) || (obj == NULL)) {         /* got a bad pointer */
          FPRINTF(stderr,"ERROR:  (slv6) calc_c\n");
          FPRINTF(stderr,"        Routine was passed a NULL pointer!\n");
          return FALSE;
      }

      vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
      vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
      /*      vfilter.fixed = var_false;
      vfilter.incident = var_true;
      vfilter.in_block = var_ignore;    */

      row = mtx_org_to_row(mtx,org_row);       /* convert from original numbering to current */

#ifndef KILL
      exprman_diffs(obj, &vfilter, row, mtx);  /* find the deriviative, sets calc_ok as result */
#else
     FPRINTF(stderr,"  function calc_c is slv6 is broken.\n");
     FPRINTF(stderr," It calls exprman_diffs with wrong number of args.\n");
     exit;
#endif
      return calc_ok;                          /* did things work out ?, calc_ok in calc.h */
}


static real64 *calc_bounds(struct var_variable **vlist, /* variable list to get bounds */
                                 int32 vinc,      /* number of incident variables */
                                 boolean upper)         /* do upper, else lower */

/**
 **  Stores the upper or lower bounds of all non-fixed, incident vars
 **  in a new array, which is returned by the routine
 **/
{
      real64 *tmp,*tmp_array_origin;  /* temporary storage for our bounds data */

      if (vlist == NULL) {         /* got a bad pointer */
          FPRINTF(stderr,"ERROR:  (slv6) calc_bounds\n");
          FPRINTF(stderr,"        Routine was passed a NULL variable list pointer!\n");
          return FALSE;
      }

      tmp_array_origin = create_array(vinc,real64);
      if (tmp_array_origin == NULL) {
          FPRINTF(stderr,"ERROR:  (slv6) calc_bounds\n");
          FPRINTF(stderr,"        Memory allocation failed!\n");
          return FALSE;
      }

      tmp = tmp_array_origin;
      for( ; *vlist != NULL ; ++vlist )
        if( free_inc_var_filter(*vlist) )
           if (upper) { *tmp=var_upper_bound(*vlist); tmp++; }
           else   { *tmp=var_lower_bound(*vlist); tmp++; }

      return tmp_array_origin;
}


static char *calc_reloplist(struct rel_relation **rlist,
                            int32    rused)   /* entry for each relation */
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
{
   char *reloplist;
   char *tmp;                  /* pointer for storage */

   reloplist = create_array(rused,char);  /* see macro, over all incident relations */
   if (reloplist == NULL) {         /* memory allocation failed */
          FPRINTF(stderr,"ERROR:  (slv6) calc_reloplist\n");
          FPRINTF(stderr,"        Memory allocation failed!\n");
          return NULL;
   }

   tmp = reloplist;
   for ( ;*rlist != NULL; rlist++)
   {
       if (inc_rel_filter(*rlist))   /* is an incident var */
           switch (rel_type(*rlist)) {
               case e_less:
               case e_lesseq:
                              *tmp = rel_TOK_less;
                              break;

               case e_equal:
                              *tmp = rel_TOK_equal;
                              break;
               case e_greater:
               case e_greatereq:
                              *tmp = rel_TOK_greater;
                              break;
               default:
                              FPRINTF(stderr,"ERROR:  (slv6) calc_reloplist\n");
                              FPRINTF(stderr,"        Unknown relation type (not greater, less, or equal)\n");
                              return NULL;
           }
       else
           *tmp = rel_TOK_nonincident;

       tmp++;  /* increment to new location in array */

   }

   return reloplist;
}


static char *calc_svtlist( struct var_variable **vlist,    /* input, not modified */
                           int32 vused,        /* number of vars (incident or nonincident, free or fixed) */
                           int *solver_var_used,     /* number of each type of var are cached  */
                           int *solver_relaxed_used,
                           int *solver_int_used,
                           int *solver_binary_used,
                           int *solver_semi_used,
                           int *solver_other_used,
                           int *solver_fixed)
/**
 ***  This function constructs the solver var list from the variable list.
 ***
 ***  WARNING:  This routine assumes that a struct var_variable *is an Instance.
 ***  In the future this is going to change, and this routine will break.
 ***
 **/
{
   struct TypeDescription *type;              /* type of the current var */
   struct TypeDescription *solver_var_type;   /* type of the standard types */
   struct TypeDescription *solver_int_type;
   struct TypeDescription *solver_binary_type;
   struct TypeDescription *solver_semi_type;

   char *svtlist;  /* pointer for storage */
   char *tmp;      /* temp pointer to set values */

   /* get the types for variable definitions */

   if( (solver_var_type = FindType(SOLVER_VAR_STR)) == NULL ) {
       FPRINTF(stderr,"ERROR:  (slv6.c) get_solver_var_type\n");
       FPRINTF(stderr,"        Type solver_var not defined.\n");
       FPRINTF(stderr,"        MPS will not work.\n");
       return NULL;
   }
   if( (solver_int_type = FindType(SOLVER_INT_STR)) == NULL ) {
       FPRINTF(stderr,"ERROR:  (slv6.c) get_solver_var_type\n");
       FPRINTF(stderr,"        Type solver_int not defined.\n");
       FPRINTF(stderr,"        MPS will not work.\n");
       return NULL;
   }
   if( (solver_binary_type = FindType(SOLVER_BINARY_STR)) == NULL ) {
       FPRINTF(stderr,"ERROR:  (slv6.c) get_solver_var_type\n");
       FPRINTF(stderr,"        Type solver_binary not defined.\n");
       FPRINTF(stderr,"        MPS will not work.\n");
       return NULL;
   }
   if( (solver_semi_type = FindType(SOLVER_SEMI_STR)) == NULL ) {
       FPRINTF(stderr,"ERROR:  (slv6.c) get_solver_var_type\n");
       FPRINTF(stderr,"        Type solver_semi not defined.\n");
       FPRINTF(stderr,"        MPS will not work.\n");
       return NULL;
   }

   /* allocate memory and initialize stuff */
   svtlist = create_array(vused,char);  /* see macro */
   if (svtlist == NULL) {         /* memory allocation failed */
          FPRINTF(stderr,"ERROR:  (slv6) calc_svtlist\n");
          FPRINTF(stderr,"        Memory allocation failed for solver var type list!\n");
          return NULL;
   }

   *solver_var_used = 0;
   *solver_relaxed_used = 0;
   *solver_int_used = 0;
   *solver_binary_used = 0;
   *solver_semi_used = 0;
   *solver_other_used = 0;
   *solver_fixed = 0;
   tmp = svtlist;

   /* loop over all vars */

   for( ; *vlist != NULL ; ++vlist )  {
      if( free_inc_var_filter(*vlist) )
      {
          type = InstanceTypeDesc( (struct Instance *) *vlist);

          if (type == MoreRefined(type,solver_binary_type) )
          {
              if (var_relaxed(*vlist))
              {
                   *tmp = SOLVER_RELAXED;
                   *solver_relaxed_used++;
              }
              else
              {
                   *tmp = SOLVER_BINARY;
                   *solver_binary_used++;
              }
          }
          else
          {
              if (type == MoreRefined(type,solver_int_type) )
              {
                   if (var_relaxed(*vlist))
                   {
                         *tmp = SOLVER_RELAXED;
                         *solver_relaxed_used++;
                   }
                   else
                   {
                         *tmp = SOLVER_INT;
                         *solver_int_used++;
                   }
              }
              else
              {
                   if (type == MoreRefined(type,solver_semi_type) )
                   {
                         if (var_relaxed(*vlist))
                         {
                              *tmp = SOLVER_RELAXED;
                              *solver_relaxed_used++;
                         }
                         else
                         {
                              *tmp = SOLVER_SEMI;
                              *solver_semi_used++;
                         }
                   }
                   else
                   {
                         if (type == MoreRefined(type,solver_var_type) )
                         {  /* either solver var or some refinement */
                              *tmp = SOLVER_VAR;
                              *solver_var_used++;
                         }
                         else
                         {
                             FPRINTF(stderr,"ERROR:  (slv6) determine_svtlist\n");
                             FPRINTF(stderr,"        Unknown solver_var type encountered.\n");
                              /* should never get to here */
                         }
                   }          /* if semi */
              }          /* if int */
          }         /* if binary */
      }        /* if free inc var */
      else
      {
         *tmp = SOLVER_FIXED;
         *solver_fixed++;
      }

      tmp++;

    }  /* for */

    return svtlist;

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
int32     cap,          in: capacity of matrix
int32     rused,        in: total number of relations used
int32     vused,        in: total number of variables used
struct rel_relation  **rlist,       in: Relation list (NULL terminated)
struct rel_relation  *obj,          in: objective function
int32     crow,         in: row to store objective row
slv_status_t    *s,          out: s.block.jactime, and s.calc_ok
int32     *rank,       out
real64    **rhs_orig   out: rhs array origin
**/
/**
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
 ***
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
 ***
 **/
{
   mtx_matrix_t mtx;      /* main data structure */
   var_filter_t vfilter;  /* checks for free incident vars in relman_diffs */
   double time0;          /* time of Jacobian calculation, among other things */
   struct rel_relation **rp;    /* relation pointer */
   real64 *rhs;     /* temporary pointer for our RHS data */

   if(obj == NULL) {         /* a little preflight checking */
      FPRINTF(stderr,"ERROR:  (slv6) calc_matrix\n");
      FPRINTF(stderr,"        System must have an objective!\n");
      return NULL;
   }

   time0=tm_cpu_time();           /* start timing */
   s->calc_ok = TRUE;            /* no errors yet */

   mtx = mtx_create();            /* create 0 order matrix */
   mtx_set_order(mtx,cap);        /* adjust size of square matrix to size of cap
                                    (cap set in presolve.) The relman_diffs
                                     routine returns values in the matrix */
                                  /* these routines don't return success/fail */

   vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
   vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);
   /*   vfilter.fixed = var_false;
   vfilter.incident = var_true;
   vfilter.in_block = var_ignore; */

   /* want to save column of residuals as they come along from relman_diffs */
   *rhs_orig = create_array(rused,real64);
   if(*rhs_orig == NULL) {         /* memory allocation failed */
      FPRINTF(stderr,"ERROR:  (slv6) calc_matrix\n");
      FPRINTF(stderr,"        Memory allocation for right hand side failed!\n");
      return NULL;
   }
   rhs = *rhs_orig;


  /* note: the rhs array is the residual at the current point, not what we want!
     see further comments at the start of this routine */

   for( rp = rlist ; *rp != NULL ; ++rp ) {
      /* fill out A matrix only for used elements */
      if( inc_rel_filter(*rp) ) {
         *rhs = relman_diffs(*rp,&vfilter,mtx);
         /*calculate each row of A matrix here! */
         rhs++;
         if( ! calc_ok ) {   /* error check each call, calc_ok is in calc.h */
            s->calc_ok = FALSE;  /* error in diffs ! */
            FPRINTF(stderr,"ERROR:  (slv6) calc_matrix\n");
            FPRINTF(stderr,"        Error in calculating A matrix.\n");
            destroy_array(rhs_orig);  /* clean up house, then die */
            mtx_destroy(mtx);                 /* zap all alocated memory */
            return NULL;
         }
      }
   }
   /* Calculate the rank of the matrix, before we add extra rows/cols */
   mtx_output_assign(mtx, crow, vused);
   if(! mtx_output_assigned(mtx)) {  /* output assignment failed */
      FPRINTF(stderr,"ERROR:  (slv6) calc_matrix\n");
      FPRINTF(stderr,"        Output assignment to calculate rank of problem failed.\n");
      mtx_destroy(mtx);                 /* zap all alocated memory */
      destroy_array(rhs_orig);  /* clean up house, then die */
      return NULL;
   }
   *rank = mtx_symbolic_rank(mtx);

   if( *rank < 0 ) {
      FPRINTF(stderr,"ERROR:  (slv6) calc_matrix\n");
      FPRINTF(stderr,"        Symbolic rank calculation failed, matrix may be bad.\n");
      return mtx;
   }

   /* calculate the c vector and save it to the matrix */
   if( ! calc_c(mtx, mtx_org_to_row(mtx,crow), obj) ) {
      s->calc_ok = FALSE;  /* error in diffs ! */
      FPRINTF(stderr,"ERROR:  (slv6) calc_matrix\n");
      FPRINTF(stderr,"        Error in calculating objective coefficients.\n");
      mtx_destroy(mtx);    /* commit suicide */
      destroy_array(rhs_orig);  /* clean up house, then die */
      return NULL;
   }

   s->block.jactime = tm_cpu_time() - time0;  /* set overall jacobian time */

   return mtx;
}


static void real_rhs(mtx_matrix_t    Ac_mtx,      /* Matrix representation of problem */
                     char            relopcol[],  /* is it incident? */
                     struct var_variable  **vlist,      /* Variable list (NULL terminated) */
                     int32     rused,       /* in: total number of relations used */
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
   real64 a;        /* value of mtx element */
   mtx_coord_t  nz;       /* coordinate of row/column in A matrix */
   mtx_range_t  range;    /* storage for range of A matrix, run down a column */
   int32  currow;   /* counter for current row */
   int          orgrow;   /* original row number */
   int          orgcol;   /* orignal col number */
   double       rowval;   /* the sum of a[i]*x[i] in the row */

   if(rhs == NULL) {         /* a little preflight checking */
      FPRINTF(stderr,"ERROR:  (slv6) real_rhs\n");
      FPRINTF(stderr,"        The routine was passed a NULL rhs pointer!\n");
      return;
   }

   for(currow = 0; currow < rused; currow++)      {      /* loop over all rows, is _current_ column number */
      orgrow = mtx_row_to_org(Ac_mtx, currow);
      if (relopcol[orgrow] != rel_TOK_nonincident)  {   /* if it is incident row */

 	   nz.col = mtx_FIRST;    /* first nonzero col */
	   nz.row = currow;       /* current row */
           rowval = 0.0;          /* accumulate value here */

           a = mtx_next_in_row(Ac_mtx,&nz,mtx_range(&range,0,rused));

           do  {  orgcol  = mtx_col_to_org(Ac_mtx, nz.col);
                  rowval += a*var_value(*(vlist+orgcol));
                  a = mtx_next_in_row(Ac_mtx,&nz,mtx_range(&range,0,rused));

               } while (nz.col != mtx_LAST);

          rhs[orgrow] = rowval - rhs[orgrow];  /* set real value of right hand side */

      }
  }

}

/* _________________________________________________________________________ */

/**
 ***  Routines used by presolve
 ***  --------------------
 ***  insure_bounds - fix inconsistent bounds
 ***  update_vlist - add vars to vlist
 ***  determine_vlist - build new vlist
 **/


static void insure_bounds(FILE *mif,slv6_system_t sys, struct var_variable *var)
/**
 ***  Insures that the variable value is within its bounds.
 **/
{
   real64 val,low,high;

   low = var_lower_bound(var);
   high = var_upper_bound(var);
   val = var_value(var);
   if( low > high ) {
      FPRINTF(mif,"Bounds for variable ");
      slv_print_var_name(mif,sys->slv,var);
      FPRINTF(mif," are inconsistent [%g,%g].\n",low,high);
      FPRINTF(mif,"Bounds will be swapped.\n");
      var_set_upper_bound(var, low);
      var_set_lower_bound(var, high);
      low = var_lower_bound(var);
      high = var_upper_bound(var);
   }

   if( low > val ) {
      FPRINTF(mif,"Variable ");
      slv_print_var_name(mif,sys->slv,var);
      FPRINTF(mif," was initialized below its lower bound.\n");
      FPRINTF(mif,"It will be moved to its lower bound.\n");
      var_set_value(var, low);
   } else if( val > high ) {
      FPRINTF(mif,"Variable ");
      slv_print_var_name(mif,sys->slv,var);
      FPRINTF(mif," was initialized above its upper bound.\n");
      FPRINTF(mif,"It will be moved to its upper bound.\n");
      var_set_value(var, high);
   }
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

static void determine_vlist(slv6_system_t sys)
/**
 ***  This function constructs the variable list from the relation list.  It
 ***  is assumed that sys->vlist_user == NULL so that this is necessary.
 **/
{
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


void slv6_set_var_list(sys,vlist)
slv6_system_t sys;
struct var_variable **vlist;
{
   static struct var_variable *empty_list[] = {NULL};
   check_system(sys);
   if( sys->vlist_user == NULL )
      if( sys->vlist != NULL && pl_length(sys->vlist) > 0 )
	 ascfree( (POINTER)(sys->vlist) );
   sys->vlist_user = vlist;
   sys->vlist = (vlist==NULL ? empty_list : vlist);
   sys->s.ready_to_solve = FALSE;
}

struct var_variable **slv6_get_var_list(sys)
slv6_system_t sys;
{
   check_system(sys);
   return( sys->vlist_user );
}

void slv6_set_bnd_list(sys,blist)
slv6_system_t sys;
bnd_boundary_t *blist;
{
   static bnd_boundary_t empty_list[] = {NULL};
   check_system(sys);
   sys->blist_user = blist;
   sys->blist = (blist==NULL ? empty_list : blist);
   sys->s.ready_to_solve = FALSE;
}

bnd_boundary_t *slv6_get_bnd_list(sys)
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

int slv6_count_bnds(sys,bfilter)
slv6_system_t sys;
bnd_filter_t *bfilter;
{
   bnd_boundary_t *bp;
   int32 count = 0;
   check_system(sys);
   for( bp=sys->blist; *bp != NULL; bp++ )
      if( bnd_apply_filter(*bp,bfilter) ) ++count;
   return( count );
}

int slv6_count_rels(sys,rfilter)
slv6_system_t sys;
rel_filter_t *rfilter;
{
   struct rel_relation **rp;
   int32 count = 0;
   check_system(sys);
   for( rp=sys->rlist; *rp != NULL; rp++ )
      if( rel_apply_filter(*rp,rfilter) ) ++count;
   return( count );
}

void slv6_set_obj_relation(slv6_system_t sys,struct rel_relation *obj)
{
   check_system(sys);
   sys->obj = obj;
   sys->s.ready_to_solve = FALSE;
}

struct rel_relation *slv6_get_obj_relation(slv6_system_t sys)
{
   check_system(sys);
   return(sys->obj);
}

void slv6_get_parameters(sys,parameters)
slv6_system_t sys;
slv_parameters_t *parameters;
{
   check_system(sys);
   mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

void slv6_set_parameters(sys,parameters)
slv6_system_t sys;
slv_parameters_t *parameters;
{
   check_system(sys);
   if (parameters->whose==slv6_solver_number)
   mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

void slv6_get_status(sys,status)
slv6_system_t sys;
slv_status_t *status;
{
   check_system(sys);
   mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}

void slv6_dump_internals(sys,level)
slv6_system_t sys;
int level;
{
   check_system(sys);
   if (level > 0) {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_dump_internals\n");
      FPRINTF(stderr,"        slv6 does not dump its internals.\n");
   }
}


/* _________________________________________________________________________ */

/**
 ***  External routines with minor modifications
 ***  -----------------
 ***  slv6_get_linsol_sys(sys)    just returns NULL & error msg
 ***  slv6_change_basis           just return FALSE & error msg
 **/


linsol_system_t slv6_get_linsol_sys(sys)   /* just returns NULL & error msg */
slv6_system_t sys;
{

/* In the MPS file maker, there is no linsol_sys to return.
   So I just write out an error message, and return a NULL pointer  */

   FPRINTF(stderr,"ERROR:  (slv6) slv6_change_basis\n");
   FPRINTF(stderr,"        This solver does not support changing the basis.\n");

   return NULL;
}


boolean slv6_change_basis(slv6_system_t sys,int32 var, mtx_range_t *rng){

/* In the MPS file maker, changing the basis doesn't make any sense.
   Nor, for that matter, is there a basis in the first place.
   So I just write out an error message, and return FALSE  */

   FPRINTF(stderr,"ERROR:  (slv6) slv6_change_basis\n");
   FPRINTF(stderr,"        This solver does not support changing the basis.\n");

   return FALSE;
}


/* _________________________________________________________________________ */

/**
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


slv6_system_t slv6_create()   /* added mps initialization */
{
  slv6_system_t sys;

/***  This routine allocates memory and initializes all data structures
 ***  It should be a good source of comments on the system parameters and
 ***  status flags used in slv6
 **/

/***  Allocate main system memory ***/

  sys = (slv6_system_t)ascmalloc( sizeof(struct slv6_system_structure) );
  mem_zero_byte_cast(sys,0,sizeof(struct slv6_system_structure));
  sys->integrity = OK;


/***  Initialize system parameters ***/

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


int slv6_destroy(sys)
slv6_system_t sys;
{
   int i;

   if (check_system(sys)) return 1;
   slv6_set_var_list(sys,(struct var_variable **)NULL);
   slv6_set_obj_function(sys,NULL);
   slv6_set_bnd_list(sys,NULL);
   slv6_set_rel_list(sys,NULL);
   slv6_set_extrel_list(sys,NULL);
   sys->integrity = DESTROYED;
   if (sys->s.cost) ascfree(sys->s.cost);  /* deallocate cost array */

   /* deallocate strings here */
   for (i=0; i< slv6_CA_SIZE; i++) {
       if (sys->p.sp.cap[i] != NULL) ascfree(sys->p.sp.cap[i]);  /* deallocate old, if any */
   }

   nuke_pointers(sys->mps);   /* free memory, and set all pointers to NULL */
   ascfree( (POINTER)sys );


   return 0;
}


boolean slv6_eligible_solver(sys)

/***  The system must have a relation list and objective before
 ***  slv6_eligible_solver will return true
 **/

slv6_system_t sys;
{
   struct rel_relation **rp;
   var_filter_t vfilter;

   check_system(sys);
   if( sys->rlist == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_eligible_solver\n");
      FPRINTF(stderr,"        Relation list was never set.\n");
      return (FALSE);
   }
   if( sys->obj == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_eligible_solver\n");
      FPRINTF(stderr,"        No objective in problem.\n");
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
   if (sys->iarray[SP6_NONLIN] == 0){
      for( rp=sys->rlist ; *rp != NULL ; ++rp )   /* check relations */
          if(!relman_is_linear(*rp,&vfilter)) {
            FPRINTF(MIF(sys), "ERROR:  With the current settings, the MPS generator can only\n");
            FPRINTF(MIF(sys), "        handle linear models. Nonlinearity in constraint:\n");
            slv_print_rel_name(MIF(sys),sys->slv, *rp);
            return(FALSE);   /* don't do nonlinearities */
          }
#ifndef KILL
      if (!exprman_is_linear(sys->obj,&vfilter)){
          FPRINTF(MIF(sys), "ERROR:  With the current settings, the MPS generator can only\n");
          FPRINTF(MIF(sys), "        handle linear models. Nonlinearity in objective.\n");
          return(FALSE);   /* don't do nonlinearities */
      }
#else
     FPRINTF(stderr,"  function slv6_elegible_solver is slv6 is broken.\n");
     FPRINTF(stderr," It calls exprman_is_linear with wrong number of args.\n");
     exit;
#endif
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

void slv6_presolve(sys)
slv6_system_t sys;
{
   struct var_variable **vp;
   struct rel_relation **rp;
   bnd_boundary_t *bp;
   int32 cap;

   bnd_filter_t bfilter;

   /* Check if necessary pointers are non-NULL */
   check_system(sys);
   if( sys->vlist == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Variable list was never set.\n");
      return;
   }
   if( sys->blist == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Boundary list was never set.\n");
      return;
   }
   if( sys->rlist == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Relation list was never set.\n");
      return;
   }

   /* time presolve */
   sys->clock = tm_cpu_time();  /* record start time */

/*  set up vlist, if necessary, and set all vars, rels, and boundary's
    to being nonincident, set up index scheme */

#ifndef KILL
   if( sys->vlist_user == NULL ) determine_vlist(sys);
#else
   if( sys->vlist_user == NULL ){
     FPRINTF(stderr,"  function determine_vlist is slv6 is broken.\n");
     exit;
   }
#endif
   sys->mps.cap = 0;
   for( vp=sys->vlist,cap=0 ; *vp != NULL ; ++vp ) {
      var_set_sindex(*vp,cap++);
      var_set_in_block(*vp,FALSE);
   }
   sys->mps.cap = cap;
   for( rp=sys->rlist,cap=0 ; *rp != NULL ; ++rp ) {
      rel_set_index(*rp,cap++);
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

      /* compute info for variables */
   sys->mps.vused = 0;     /* number starting at 0 */
   sys->mps.vinc = 0;
   for( vp = sys->vlist ; *vp != NULL ; vp++ ) {
      if( free_inc_var_filter(*vp) )
          sys->mps.vinc++;
      sys->mps.vused++;    /* count up incident, non-fixed vars */
   }

   /* calculate values for other index_mps_t vars */
   sys->mps.crow     = sys->mps.rused;    /* note rused = N means rows 0 to N-1, exist,
                                             the next one will be numbered rused */
   /* calculate rank later */

   /* Call slv6_elgibile_solver to see if the solver has a chance */
   /* If not bail now ... requires the incidence values of prev section be set */
   if(! slv6_eligible_solver(sys)) return;

   /*  Make sure that at least one incident variable and at least one incident
       relation exist, else bail */
   if ((sys->mps.rinc == 0) || (sys->mps.vinc == 0))  {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Your model must have at least one incident variable and equation.\n");
      FPRINTF(stderr,"        Incident variables: %d\n", sys->mps.vinc);
      FPRINTF(stderr,"        Incident equations: %d\n", sys->mps.rinc);
      return;
   }

   /* free memory, and set all pointers to NULL */
   nuke_pointers(sys->mps);

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
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Call to calc_matrix failed.\n");
      nuke_pointers(sys->mps);
      return;
   }

   /* get upper bound row */
   sys->mps.ubrow = calc_bounds(sys->vlist, sys->mps.vinc, TRUE);
   if (sys->mps.ubrow == NULL)  {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Error in calculating variable upper bounds.\n");
      nuke_pointers(sys->mps);
      return;
   }

   /* get lower bound row */
   sys->mps.lbrow = calc_bounds(sys->vlist, sys->mps.vinc, FALSE);
   if (sys->mps.lbrow == NULL)  {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Error in calculating variable lower bounds.\n");
      nuke_pointers(sys->mps);
      return;
   }

   /* Call calc_svtlist to allocate array of variable types */
   sys->mps.typerow = calc_svtlist(sys->vlist,
                                   sys->mps.vinc,
                                   &sys->mps.solver_var_used,      /* output */
                                   &sys->mps.solver_relaxed_used,  /* output */
                                   &sys->mps.solver_int_used,      /* output */
                                   &sys->mps.solver_binary_used,   /* output */
                                   &sys->mps.solver_semi_used,     /* output */
                                   &sys->mps.solver_other_used,    /* output */
                                   &sys->mps.solver_fixed);        /* output */
   if(sys->mps.typerow == NULL) {         /* allocation failed */
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Error in calculating the variable type list!\n");
      nuke_pointers(sys->mps);
      return;
   }

   /* Call calc_reloplist here, to calculate the relational operators >=, <=, = */
    sys->mps.relopcol = calc_reloplist(sys->rlist, sys->mps.rinc);
    if(sys->mps.relopcol == NULL) {         /* allocation failed */
      FPRINTF(stderr,"ERROR:  (slv6) slv6_presolve\n");
      FPRINTF(stderr,"        Error in calculating the relational operators!\n");
      nuke_pointers(sys->mps);
      return;
   }

   /* adjust the rhs vector so it actually contains the rhs */
   real_rhs(sys->mps.Ac_mtx,      /* Matrix representation of problem */
            sys->mps.relopcol,    /* is it incident? */
            sys->vlist,           /* in: Variable list (NULL terminated) */
            sys->mps.rused,       /* in: total number of relations used */
            sys->mps.bcol);       /* out: rhs array origin */


   /* Call insure_bounds over all vars to make bounds self-consistent */
   for( vp=sys->vlist; *vp != NULL ; ++vp )
     insure_bounds(MIF(sys),sys, *vp);

   /* Reset status flags */
   sys->s.over_defined = (sys->mps.rinc > sys->mps.vinc);
   sys->s.under_defined = (sys->mps.rinc < sys->mps.vinc);
   sys->s.struct_singular = (sys->mps.rank < sys->mps.rinc);
   sys->s.ok = sys->s.calc_ok && !sys->s.struct_singular;
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

void slv6_solve(sys)
slv6_system_t sys;
{

   /* make sure none of the mps pointers are NULL */
   if ((sys->mps.Ac_mtx == NULL) ||
       (sys->mps.lbrow == NULL) ||
       (sys->mps.ubrow == NULL) ||
       (sys->mps.bcol == NULL) ||
       (sys->mps.typerow == NULL) ||
       (sys->mps.relopcol == NULL)) {
      FPRINTF(MIF(sys),"ERROR:  Matrix representation of problem is not available.\n");
      FPRINTF(MIF(sys),"        Perhaps the presolve routine was not called before slv6_solve.\n");
      return;
   }

   /* Check system to see if it can be solved  */
   check_system(sys);
   if( !sys->s.ready_to_solve ) {
      FPRINTF(stderr,"ERROR:  (slv6) slv6_solve\n");
      FPRINTF(stderr,"        Not ready to solve.\n");
      return;
   }

   sys->clock = tm_cpu_time();   /* record start time for solve */


 /*  FPRINTF(MIF(sys),"_________________________________________\n");
   mtx_write_region_human(MIF(sys), sys->mps.Ac_mtx, mtx_ENTIRE_MATRIX);
   FPRINTF(MIF(sys),"_________________________________________\n");
 */

   /* Call write_mps to create the mps file */
   write_MPS(sys->carray[SP6_FILENAME],     /* filename for output */
             sys->mps,                      /* main chunk of data */
             sys->iarray,                   /* Integer subparameters */
             sys->rarray);                  /* Real subparameters */

   /* replace .mps with .map at end of filename */
   *(sys->carray[SP6_FILENAME]+strlen(sys->carray[SP6_FILENAME])-2) = 'a';
   *(sys->carray[SP6_FILENAME]+strlen(sys->carray[SP6_FILENAME])-1) = 'p';

   /* writes out a file mapping the CXXXXXXX variable names with the actual ASCEND names */
   write_name_map(sys->carray[SP6_FILENAME],   /* user-specified filename */
                  sys->vlist);



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


void slv6_iterate(sys)
slv6_system_t sys;
{
  /*  Writing an MPS file is a one shot deal.  Thus, an interation
      is equivalent to solving the problem.  So we just call
      slv6_solve   */

   check_system(sys);
   slv6_solve(sys);

}


void slv6_resolve(sys)
slv6_system_t sys;
{

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
   slv6_solve(sys);
}


int slv6_register(SlvFunctionsT *sft)
{
  if (sft==NULL)  {
    FPRINTF(stderr,"slv6_register called with NULL pointer\n");
    return 1;
  }

  sft->name = "makeMPS";
  sft->ccreate = slv6_create;
  sft->cdestroy = slv6_destroy;
  sft->celigible = slv6_eligible_solver;
  sft->getparam = slv6_get_parameters;
  sft->setparam = slv6_set_parameters;
  sft->getstatus = slv6_get_status;
  sft->solve = slv6_solve;
  sft->presolve = slv6_presolve;
  sft->iterate = slv6_iterate;
  sft->resolve = slv6_resolve;
  sft->getlinsol = slv6_get_linsol_sys;
  sft->getlinsys = slv6_get_linsolqr_sys;
  sft->getsysmtx = slv6_get_jacobian;
  sft->dumpinternals = slv6_dump_internals;
  return 0;
}
#endif /* #else clause of DYNAMIC_MPS */
#endif /* #else clause of !STATIC_MPS && !DYNAMIC_MPS */
