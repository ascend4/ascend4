/*
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.33 $
 *  Version control file: $RCSfile: slv0.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:16 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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

#include "slv0.h"

#include <stdarg.h>

#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <utilities/set.h>
#include <general/tm_time.h>
#include <utilities/mem.h>

#include "calc.h"
#include "relman.h"
#include "slv_common.h"

#if !defined(STATIC_SLV) && !defined(DYNAMIC_SLV)
/* do nothing */
int slv0_register(SlvFunctionsT *f)
{
  (void)f;  /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"Slv not compiled in this ASCEND IV.\n");
  return 1;
}
#else /* either STATIC_SLV or DYNAMIC_SLV is defined */
#ifdef DYNAMIC_SLV
/* do dynamic loading stuff.   yeah, right */
#else /* following is used if STATIC_SLV is defined */


#define KILL 0
/* code that needs to be deleted compiles only with kill = 1 */
#define DEBUG TRUE

#define slv0_solver_name "Slv"   /* Solver's name. don't mess with the caps!*/
#define slv0_solver_number 0   /* Solver's number */

#define SLV0(s) ((slv0_system_t)(s))

#define slv0_IA_SIZE 2
#define slv0_RA_SIZE 0
#define slv0_CA_SIZE 0
#define slv0_VA_SIZE 0

/* subscripts for ia */
#define SP0_LIFDS 0
#define SP0_SAVLIN 1

/*********************************************************************\
 Subparameters implemented:   (value/meaning)
 sp.ia[SP0_LIFDS]  0=>do not show full detail info for singletons
                   1=>do (this value ignored if detailed solve info not on.
 sp.ia[SP0_SAVLIN] 0=>do not append linearizations arising in the newton
                      scheme to the file SlvLinsol.dat.
                   1=>do.
\*********************************************************************/
struct update_data {
   int                    jacobian;     /* Countdown on jacobian updating */
   int                    weights;      /* Countdown on weights updating */
   int                    nominals;     /* Countdown on nominals updating */
};

struct jacobian_data {
   linsol_system_t        sys;          /* Linear system */
   mtx_matrix_t           mtx;          /* Transpose gradient of residuals */
   real64           *rhs;         /* RHS from linear system */
   unsigned               *varpivots;   /* Pivoted variables */
   unsigned               *relpivots;   /* Pivoted relations */
   unsigned               *subregions;  /* Set of subregion indeces */
   mtx_region_t           reg;          /* Current block region */
   int32            rank;         /* Numerical rank of the jacobian */
   boolean                accurate;     /* ? Recalculate matrix */
   boolean                singular;     /* ? Can matrix be inverted */
};

struct hessian_data {
   struct vec_vector     Bs;           /* Product of B and s */
   struct vec_vector     y;            /* Difference in stationaries */
   real64           ys;           /* inner product of y and s */
   real64           sBs;          /* inner product of s and Bs */
   struct hessian_data    *next;        /* previous iteration data */
};

struct reduced_data {
   real64           **mtx;        /* Dense matrix */
   real64           *ZBs;         /* Reduced Bs */
   real64           *Zy;          /* Reduced y */
   int32            order;        /* Degrees of freedom */
   boolean                accurate;     /* Ready to re-compute ? */
};

struct slv0_system_structure {

   /**
    ***  Problem definition
    **/
   slv_system_t           slv;          /* slv_system_t back-link */
   struct rel_relation    *obj;          /* Objective function: NULL = none */
   struct var_variable    **vlist;       /* Variable list (NULL terminated) */
   struct var_variable    **vlist_user;  /* User vlist (NULL=determine) */
   struct rel_relation    **rlist;       /* Relation list (NULL terminated) */
   struct rel_relation    **rlist_user;  /* User rlist (NULL = none) */
   struct ExtRelCache     **erlist;     /* External relations cache list */
   struct ExtRelCache     **erlist_user; /* User erlist (NULL = none) */

   /**
    ***  Solver information
    **/
   int                    integrity;    /* ? Has the system been created */
   slv_parameters_t       p;            /* Parameters */
   slv_status_t           s;            /* Status (as of iteration end) */
   struct update_data     update;       /* Jacobian frequency counters */
   int32            cap;          /* Order of matrix/vectors */
   int32            rank;         /* Symbolic rank of problem */
   int32            vused;        /* Free and incident variables */
   int32            vtot;         /* length of varlist */
   int32            bused;        /* Included boundaries */
   int32            rused;        /* Included relations */
   int32            rtot;         /* length of varlist */
   double                 clock;        /* CPU time */
   int iarray[slv0_IA_SIZE];

   /**
    ***  Calculated data (scaled)
    **/
   struct jacobian_data   J;            /* linearized system */
   struct hessian_data    *B;           /* Curvature information */
   struct reduced_data    ZBZ;          /* Reduced hessian */

   struct vec_vector     nominals;     /* Variable nominals */
   struct vec_vector     weights;      /* Relation weights */
   struct vec_vector     variables;    /* Variable values */
   struct vec_vector     residuals;    /* Relation residuals */
   struct vec_vector     gradient;     /* Objective gradient */
   struct vec_vector     multipliers;  /* Relation multipliers */
   struct vec_vector     stationary;   /* Lagrange gradient */
   struct vec_vector     gamma;        /* Feasibility steepest descent */
   struct vec_vector     Jgamma;       /* Product of J and gamma */
   struct vec_vector     newton;       /* Dependent variables */
   struct vec_vector     Bnewton;      /* Product of B and newton */
   struct vec_vector     nullspace;    /* Independent variables */
   struct vec_vector     varstep1;     /* 1st order in variables */
   struct vec_vector     Bvarstep1;    /* Product of B and varstep1 */
   struct vec_vector     varstep2;     /* 2nd order in variables */
   struct vec_vector     Bvarstep2;    /* Product of B and varstep2 */
   struct vec_vector     mulstep1;     /* 1st order in multipliers */
   struct vec_vector     mulstep2;     /* 2nd order in multipliers */
   struct vec_vector     varstep;      /* Step in variables */
   struct vec_vector     mulstep;      /* Step in multipliers */

   real64           objective;    /* Objective function evaluation */
   real64           phi;          /* Unconstrained minimizer */
   real64           maxstep;      /* Maximum step size allowed */
   real64           progress;     /* Steepest directional derivative */
};


/**
 ***  Integrity checks
 ***  ----------------
 ***     check_system(sys)
 **/

#define OK        ((int)813028392)
#define DESTROYED ((int)103289182)
static int check_system(sys)
slv0_system_t sys;
/**
 ***  Checks sys for NULL and for integrity.
 **/
{
   if( sys == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv0) check_system\n");
      FPRINTF(stderr,"        NULL system handle.\n");
      return 1;
   }

   switch( sys->integrity ) {
   case OK:
      return 0;
   case DESTROYED:
      FPRINTF(stderr,"ERROR:  (slv0) check_system\n");
      FPRINTF(stderr,"        System was recently destroyed.\n");
      return 1;
   default:
      FPRINTF(stderr,"ERROR:  (slv0) check_system\n");
      FPRINTF(stderr,"        System reused or never allocated.\n");
      return 1;
   }
}


/**
 ***  General input/output routines
 ***  -----------------------------
 ***     print_var_name(out,sys,var)
 ***     print_bnd_name(out,sys,bnd)
 ***     print_rel_name(out,sys,rel)
 **/

#define print_var_name(a,b,c) slv_print_var_name((a),(b)->slv,(c))
#define print_rel_name(a,b,c) slv_print_rel_name((a),(b)->slv,(c))
#define print_bnd_name(a,b,c) slv_print_bnd_name((a),(c))

/**
 ***  Debug output routines
 ***  ---------------------
 ***     debug_delimiter(fp)
 ***     debug_out_vector(fp,vec)
 ***     debug_out_var_values(fp,sys)
 ***     debug_out_rel_residuals(fp,sys)
 ***     debug_out_jacobian(fp,sys)
 ***     debug_out_hessian(fp,sys)
 ***     debug_write_array(fp,real64 *,length)
 **/

static void debug_delimiter(fp)
FILE *fp;
/**
 ***  Outputs a hyphenated line.
 **/
{
   int i;
   for( i=0; i<60; i++ ) PUTC('-',fp);
   PUTC('\n',fp);
}


static void debug_out_vector(fp,sys,vec)
FILE *fp;
slv0_system_t sys;
struct vec_vector *vec;
/**
 ***  Outputs a vector.
 **/
{
   int32 ndx;
   FPRINTF(fp,"Norm = %g, Accurate = %s, Vector range = %d to %d\n",
	   calc_sqrt_D0(vec->norm2), vec->accurate?"TRUE":"FALSE",
	   vec->rng->low,vec->rng->high);
   FPRINTF(fp,"Vector --> ");
   for( ndx=vec->rng->low ; ndx<=vec->rng->high ; ++ndx )
      FPRINTF(fp, "%g ", vec->vec[ndx]);
   PUTC('\n',fp);
}

static void debug_out_var_values(fp,sys)
FILE *fp;
slv0_system_t sys;
/**
 ***  Outputs all variable values in current block.
 **/
{
   int32 col;
   int32 orig_col;

   FPRINTF(fp,"Var values --> ");
   for( col = sys->J.reg.col.low; col <= sys->J.reg.col.high ; col++ ) {
      struct var_variable *var;
      orig_col = mtx_col_to_org(sys->J.mtx,col);
      var = sys->vlist[orig_col];
      /* print_var_name(fp,sys,var); **** KAA_DEBUG */
      FPRINTF(fp,"x[%d] = %12.8f\n", orig_col, var_value(var));
   }
   PUTC('\n',fp);
}

static void debug_out_rel_residuals(fp,sys)
FILE *fp;
slv0_system_t sys;
/**
 ***  Outputs all relation residuals in current block.
 **/
{
   int32 row;

   FPRINTF(fp,"Rel residuals --> ");
   for( row = sys->J.reg.row.low; row <= sys->J.reg.row.high ; row++ ) {
      struct rel_relation *rel;
      rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
      print_rel_name(fp,sys,rel);
      FPRINTF(fp,"=%g ",rel_residual(rel));
   }
   PUTC('\n',fp);
}

static void debug_out_jacobian(fp,sys)
FILE *fp;
slv0_system_t sys;
/**
 ***  Outputs permutation and values of the nonzero elements in the
 ***  the jacobian matrix.
 **/
{
   mtx_coord_t nz;
   real64 value;

   nz.row = sys->J.reg.row.low;
   for( ; nz.row <= sys->J.reg.row.high; ++(nz.row) ) {
      FPRINTF(fp,"   Row %d (rel %d)\n",
	      nz.row,
	      mtx_row_to_org(sys->J.mtx,nz.row));
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(sys->J.mtx,&nz,&(sys->J.reg.col)),
	    nz.col != mtx_LAST )
	 FPRINTF(fp,"      Col %d (var %d) has value %g\n",
		 nz.col, mtx_col_to_org(sys->J.mtx,nz.col), value);
   }
}

static void debug_out_hessian(fp,sys)
FILE *fp;
slv0_system_t sys;
/**
 ***  Outputs permutation and values of the nonzero elements in the
 ***  reduced hessian matrix.
 **/
{
   mtx_coord_t nz;

   for( nz.row = 0; nz.row < sys->ZBZ.order; nz.row++ ) {
      nz.col = nz.row + sys->J.reg.col.high + 1 - sys->ZBZ.order;
      FPRINTF(fp,"   ZBZ[%d (var %d)] = ",
	      nz.row, mtx_col_to_org(sys->J.mtx,nz.col));
      for( nz.col = 0; nz.col < sys->ZBZ.order; nz.col++ )
	 FPRINTF(fp,"%10g ",sys->ZBZ.mtx[nz.row][nz.col]);
      PUTC('\n',fp);
   }
}



static void debug_write_array(FILE *fp,real64 *vec, int32 length)
{
  int32 i;
  for (i=0; i< length;i++)
    FPRINTF(fp,"%.20g\n",vec[i]);
}


static char savlinfilename[]="SlvLinsol.dat.              \0";
static char savlinfilebase[]="SlvLinsol.dat.\0";
static int savlinnum=0;
/** The number to postfix to savlinfilebase. increases with file accesses. **/

/**
 ***  Array/vector operations
 ***  ----------------------------
 ***     destroy_array(p)
 ***     create_array(len,type)
 ***     zero_array(arr,len,type)
 ***
 ***     zero_vector(vec)
 ***     copy_vector(vec1,vec2)
 ***     prod = inner_product(vec1,vec2)
 ***     norm2 = square_norm(vec)
 ***     matrix_product(mtx,vec,prod,scale,transpose)
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

#define zero_vector(v) vec_zero(v)
#define copy_vector(v,t) vec_copy((v),(t))
#define inner_product(v,u) vec_inner_product((v),(u))
#define square_norm(v)  vec_square_norm(v)
#define matrix_product(m,v,p,s,t) vec_matrix_product((m),(v),(p),(s),(t))

/**
 ***  Calculation routines
 ***  --------------------
 ***     ok = calc_objective(sys)
 ***     ok = calc_residuals(sys)
 ***     ok = calc_J(sys)
 ***     calc_nominals(sys)
 ***     calc_weights(sys)
 ***     scale_J(sys)
 ***     scale_variables(sys)
 ***     scale_residuals(sys)
 ***     ok = calc_gradient(sys)
 ***     calc_B(sys)
 ***     calc_ZBZ(sys)
 ***     calc_pivots(sys)
 ***     calc_rhs(sys)
 ***     calc_multipliers(sys)
 ***     calc_stationary(sys)
 ***     calc_newton(sys)
 ***     calc_Bnewton(sys)
 ***     calc_nullspace(sys)
 ***     calc_gamma(sys)
 ***     calc_Jgamma(sys)
 ***     calc_varstep1(sys)
 ***     calc_Bvarstep1(sys)
 ***     calc_varstep2(sys)
 ***     calc_Bvarstep2(sys)
 ***     calc_mulstep1(sys)
 ***     calc_mulstep2(sys)
 ***     calc_varstep(sys)
 ***     calc_mulstep(sys)
 ***     calc_phi(sys)
 **/

#define UPDATE_JACOBIAN     1            /* Update jacobian this often */
#define UPDATE_WEIGHTS      1            /* Update weights this often */
#define UPDATE_NOMINALS     1000         /* Update nominals this often */
#define TOO_SMALL           1e-8
#define TOWARD_BOUNDS       0.95


#define OPTIMIZING(sys)     ((sys)->ZBZ.order > 0)

static boolean calc_objective(sys)
slv0_system_t sys;
/**
 ***  Evaluate the objective function.
 **/
{
   calc_ok = TRUE;
   Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
   sys->objective = (sys->obj ? relman_eval(sys->obj,&calc_ok) : 0.0);
   Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
   return calc_ok;
}

static boolean calc_inequalities(sys)
slv0_system_t sys;
/**
 ***  Calculates all of the residuals of included inequalities.
 ***  Returns true iff (calculations preceded without error and
 ***  all inequalities are satisfied.)
 **/
{
  struct rel_relation **rp;
  boolean satisfied=TRUE;
  static rel_filter_t rfilter;
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);

  calc_ok = TRUE;
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
  for (rp=sys->rlist;*rp != NULL; rp++) {
    if (rel_apply_filter(*rp,&rfilter)) {
      relman_eval(*rp,&calc_ok);
      satisfied=
        satisfied && relman_calc_satisfied(*rp,sys->p.tolerance.feasible);
    }
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  return (calc_ok && satisfied);
}

static boolean calc_residuals(sys)
slv0_system_t sys;
/**
 ***  Calculates all of the residuals in the current block and computes
 ***  the residual norm for block status.  Returns true iff calculations
 ***  preceded without error.
 **/
{
   int32 row;
   struct rel_relation *rel;
   double time0;

   if( sys->residuals.accurate )
      return TRUE;

   calc_ok = TRUE;
   row = sys->residuals.rng->low;
   time0=tm_cpu_time();
   Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
   for( ; row <= sys->residuals.rng->high; row++ ) {
      rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
#if DEBUG
      if (!rel) {
        int r;
        r=mtx_row_to_org(sys->J.mtx,row);
        FPRINTF(stderr,"NULL relation found !!\n");
        FPRINTF(stderr,"at row %d rel %d in calc_residuals\n",(int)row,r);
        FFLUSH(stderr);
      }
#endif
      sys->residuals.vec[row] = relman_eval(rel,&calc_ok);
      /*
	 FPRINTF(stderr,"RESIDUAL[%d] = %12.8g\n",
	 mtx_row_to_org(sys->J.mtx,row), sys->residuals.vec[row]);
	 FFLUSH(stderr);
	 */

      relman_calc_satisfied(rel,sys->p.tolerance.feasible);
   }
   Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
   sys->s.block.functime += (tm_cpu_time() -time0);
   sys->s.block.funcs++;
   square_norm( &(sys->residuals) );
   sys->s.block.residual = calc_sqrt_D0(sys->residuals.norm2);
   return(calc_ok);
}


static boolean calc_J(sys)
slv0_system_t sys;
/**
 ***  Calculates the current block of the jacobian.
 ***  It is initially unscaled.
 **/
{
   int32 row;
   var_filter_t vfilter;
   double time0;

   if( sys->J.accurate )
      return TRUE;

   calc_ok = TRUE;
   vfilter.matchbits = (VAR_INBLOCK | VAR_ACTIVE);
   vfilter.matchvalue = (VAR_INBLOCK | VAR_ACTIVE);
   /*   vfilter.fixed = var_ignore;
   vfilter.incident = var_ignore;
   vfilter.in_block = var_true; */
   time0=tm_cpu_time();
   mtx_clear_region(sys->J.mtx,&(sys->J.reg));
   for( row = sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ ) {
      struct rel_relation *rel;
      rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
      relman_diffs(rel,&vfilter,sys->J.mtx);
   }
   sys->s.block.jactime += (tm_cpu_time() - time0);
   sys->s.block.jacs++;

   if( --(sys->update.nominals) <= 0 )
      sys->nominals.accurate = FALSE;
   if( --(sys->update.weights) <= 0 )
      sys->weights.accurate = FALSE;

   linsol_matrix_was_changed(sys->J.sys);
   return(calc_ok);
}


static void calc_nominals(sys)
slv0_system_t sys;
/**
 ***  Retrieves the nominal values of all of the block variables,
 ***  insuring that they are all strictly positive.
 **/
{
   int32 col;

   if( sys->nominals.accurate )
      return;

   col = sys->nominals.rng->low;
   for( ; col <= sys->nominals.rng->high; col++ ) {
      struct var_variable *var;
      real64 n;
      var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
      n = var_nominal(var);
      if( n <= 0.0 ) {
	 FILE *fp = MIF(sys);
	 if( n == 0.0 ) {
	    n = TOO_SMALL;
	    FPRINTF(fp,"ERROR:  (slv0) calc_nominals\n");
	    FPRINTF(fp,"        Variable ");
	    print_var_name(fp,sys,var);
	    FPRINTF(fp,"        \nhas nominal value of zero.\n");
	    FPRINTF(fp,"        Resetting to %g.\n",n);
	    var_set_nominal(var,n);
	 } else {
	    n =  -n;
	    FPRINTF(fp,"ERROR:  (slv0) calc_nominals\n");
	    FPRINTF(fp,"        Variable ");
	    print_var_name(fp,sys,var);
	    FPRINTF(fp,"        \nhas negative nominal value.\n");
	    FPRINTF(fp,"        Resetting to %g.\n",n);
	    var_set_nominal(var,n);
	 }
      }
      sys->nominals.vec[col] = n;
   }
   square_norm( &(sys->nominals) );
   sys->update.nominals = UPDATE_NOMINALS;
   sys->nominals.accurate = TRUE;
}


static void calc_weights(sys)
slv0_system_t sys;
/**
 ***  Calculates the weights of all of the block relations
 ***  to scale the rows of the Jacobian.
 **/
{
   mtx_coord_t nz;

   if( sys->weights.accurate )
      return;

   nz.row = sys->weights.rng->low;
   for( ; nz.row <= sys->weights.rng->high; (nz.row)++ ) {
      real64 sum;
      sum=mtx_sum_sqrs_in_row(sys->J.mtx,nz.row,&(sys->J.reg.col));
      sys->weights.vec[nz.row] = (sum>0.0) ? 1.0/calc_sqrt_D0(sum) : 1.0;
   }
   square_norm( &(sys->weights) );
   sys->update.weights = UPDATE_WEIGHTS;
   sys->residuals.accurate = FALSE;
   sys->weights.accurate = TRUE;
}


static void scale_J(sys)
slv0_system_t sys;
/**
 ***  Scales the jacobian.
 **/
{
   int32 row;
   int32 col;

   if( sys->J.accurate )
      return;

   calc_nominals(sys);

   for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ )
      mtx_mult_col(sys->J.mtx,col,sys->nominals.vec[col],&(sys->J.reg.row));

   calc_weights(sys);
   for( row=sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ )
      mtx_mult_row(sys->J.mtx,row,sys->weights.vec[row],&(sys->J.reg.col));

   sys->update.jacobian = UPDATE_JACOBIAN;
   sys->J.accurate = TRUE;
   sys->J.singular = FALSE;  /* yet to be determined */
#if DEBUG
   FPRINTF(LIF(sys),"\nJacobian: \n");
   debug_out_jacobian(LIF(sys),sys);
#endif
}


static void scale_variables(sys)
slv0_system_t sys;
{
   int32 col;

   if( sys->variables.accurate )
      return;

   col = sys->variables.rng->low;
   for( ; col <= sys->variables.rng->high; col++ ) {
      struct var_variable *var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
      sys->variables.vec[col] = var_value(var)/sys->nominals.vec[col];
   }
   square_norm( &(sys->variables) );
   sys->variables.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Variables:  ");
   debug_out_vector(LIF(sys),sys,&(sys->variables));
#endif
}


static void scale_residuals(sys)
slv0_system_t sys;
/**
 ***  Scales the previously calculated residuals.
 **/
{
   int32 row;

   if( sys->residuals.accurate )
      return;

   row = sys->residuals.rng->low;
   for( ; row <= sys->residuals.rng->high; row++ ) {
      struct rel_relation *rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
      sys->residuals.vec[row] = rel_residual(rel)*sys->weights.vec[row];
   }
   square_norm( &(sys->residuals) );
   sys->residuals.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Residuals:  ");
   debug_out_vector(LIF(sys),sys,&(sys->residuals));
#endif
}

static boolean calc_gradient(sys)
slv0_system_t sys;
/**
 ***  Calculate scaled gradient of the objective function.
 **/
{
   if( sys->gradient.accurate )
      return TRUE;

   calc_ok = TRUE;
   if ( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->gradient));
      sys->gradient.norm2 = 0.0;
   } else {
      struct var_variable **vp,**tmp;
      var_filter_t vfilter;
      vfilter.matchbits = (VAR_INBLOCK | VAR_ACTIVE);
      vfilter.matchvalue = (VAR_INBLOCK | VAR_ACTIVE);
      /* vfilter.fixed = var_ignore;
      vfilter.incident = var_ignore;
      vfilter.in_block = var_true; */
      zero_vector(&(sys->gradient));
#if 0 /* this needs to be reimplemented  */
      for( vp=tmp=expr_incidence_list(sys->obj,NULL) ; *vp != NULL ; ++vp ) {
	 int32 col;
	 col = mtx_org_to_col(sys->J.mtx,var_sindex(*vp));
	 if( var_apply_filter(*vp,&vfilter) )
	    sys->gradient.vec[col] = sys->nominals.vec[col]*
	       exprman_diff(sys->obj,*vp);
      }
      ascfree(tmp);
      square_norm( &(sys->gradient) );
#else
      Asc_Panic(2, "calc_gradient",
                "need to reimplemnt objective eval slv0\n");
#endif
   }
   sys->gradient.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Gradient:  ");
   debug_out_vector(LIF(sys),sys,&(sys->gradient));
#endif
   return calc_ok;
}


static void create_update(sys)
slv0_system_t sys;
/**
 ***  Create a new hessian_data structure for storing
 ***  latest update information.
 **/
{
   struct hessian_data *update;

   if( !OPTIMIZING(sys) )
      return;

   update = (struct hessian_data *)ascmalloc(sizeof(struct hessian_data));
   update->y.vec = create_array(sys->cap,real64);
   update->y.rng = &(sys->J.reg.col);
   update->y.accurate = FALSE;
   update->Bs.vec = create_array(sys->cap,real64);
   update->Bs.rng = &(sys->J.reg.col);
   update->Bs.accurate = FALSE;
   update->next = sys->B;
   sys->B = update;
}


#define POSITIVE_DEFINITE  1.0e-2
static void calc_B(sys)
slv0_system_t sys;
/**
 ***  Computes a rank 2 BFGS update to the hessian matrix
 ***  B which accumulates curvature.
 **/
{
   if( sys->s.block.iteration > 1 )
      create_update(sys);
   else
      if( sys->B ) {
	 struct hessian_data *update;
	 for( update=sys->B; update != NULL; ) {
	    struct hessian_data *handle;
	    handle = update;
	    update = update->next;
	    destroy_array(handle->y.vec);
	    destroy_array(handle->Bs.vec);
	    ascfree(handle);
	 }
	 sys->B = NULL;
      }

   if( sys->B ) {
      real64 theta;
      /**
       ***  The y vector
       **/
      if( !sys->B->y.accurate ) {
	 int32 col;
	 matrix_product(sys->J.mtx, &(sys->multipliers),
			&(sys->B->y), 1.0, TRUE);
	 col = sys->B->y.rng->low;
	 for( ; col <= sys->B->y.rng->high; col++ )
	    sys->B->y.vec[col] += sys->gradient.vec[col] -
	       sys->stationary.vec[col];
	 square_norm( &(sys->B->y) );
	 sys->B->y.accurate = TRUE;
      }

      /**
       ***  The Bs vector
       **/
      if( !sys->B->Bs.accurate ) {
	 struct hessian_data *update;
	 copy_vector(&(sys->varstep),&(sys->B->Bs));
	 for( update=sys->B->next; update != NULL; update = update->next ) {
	    int32 col;
	    real64 ys = inner_product( &(update->y),&(sys->varstep) );
	    real64 sBs = inner_product( &(update->Bs),&(sys->varstep) );
	    col = sys->B->Bs.rng->low;
	    for( ; col<=sys->B->Bs.rng->high; col++) {
	       sys->B->Bs.vec[col] += update->ys > 0.0 ?
		  (update->y.vec[col])*ys/update->ys : 0.0;
	       sys->B->Bs.vec[col] -= update->sBs > 0.0 ?
		  (update->Bs.vec[col])*sBs/update->sBs : 0.0;
	    }
	 }
	 square_norm( &(sys->B->Bs) );
	 sys->B->Bs.accurate = TRUE;
      }

      sys->B->ys = inner_product( &(sys->B->y),&(sys->varstep) );
      sys->B->sBs = inner_product( &(sys->B->Bs),&(sys->varstep) );

      if( sys->B->ys == 0.0 && sys->B->sBs == 0.0 ) theta = 0.0;
      else theta = sys->B->ys < POSITIVE_DEFINITE*sys->B->sBs ?
	 (1.0-POSITIVE_DEFINITE)*sys->B->sBs/(sys->B->sBs - sys->B->ys):1.0;
#if DEBUG
      FPRINTF(LIF(sys),"ys, sBs, PD, theta = %g, %g, %g, %g\n",
	      sys->B->ys,
	      sys->B->sBs,
	      POSITIVE_DEFINITE,
	      theta);
#endif
      if( theta < 1.0 ) {
	 int32 col;
	 col = sys->B->y.rng->low;
	 for( ; col <= sys->B->y.rng->high; col++ )
	    sys->B->y.vec[col] = theta*sys->B->y.vec[col] +
	       (1.0-theta)*sys->B->Bs.vec[col];
	 square_norm( &(sys->B->y) );
	 sys->B->ys = theta*sys->B->ys + (1.0-theta)*sys->B->sBs;
      }
   }
}
#undef POSITIVE_DEFINITE


static void calc_pivots(sys)
slv0_system_t sys;
/**
 ***  Obtain the equations and variables which
 ***  are able to be pivoted.
 **/
{
   linsol_system_t lsys = sys->J.sys;
   FILE *fp = LIF(sys);
#undef KAA_DEBUG
#ifdef KAA_DEBUG
   double time1 = tm_cpu_time();
   linsol_invert(lsys,&(sys->J.reg));
   time1 = tm_cpu_time() - time1;
   FPRINTF(stdout,"Time to invert block = %g seconds\n",time1);
#else
   linsol_invert(lsys,&(sys->J.reg));
#endif

   set_null(sys->J.relpivots,sys->cap);
   set_null(sys->J.varpivots,sys->cap);
   linsol_get_pivot_sets(lsys,sys->J.relpivots,sys->J.varpivots);
   sys->J.rank = linsol_rank(lsys);
   sys->J.singular = FALSE;
   if( sys->J.rank < sys->J.reg.row.high-sys->J.reg.row.low+1 ) {
      int32 row;
      sys->J.singular = TRUE;
      for( row=sys->J.reg.row.low; row <= sys->J.reg.row.high ; row++ ) {
	 int32 org_row;
	 org_row = mtx_row_to_org(sys->J.mtx,row);
	 if( !set_is_member(sys->J.relpivots,org_row) ) {
	    struct rel_relation *rel;
	    rel = sys->rlist[org_row];
	    FPRINTF(fp,"%-40s ---> ","Relation not pivoted");
	    print_rel_name(fp,sys,rel);
	    PUTC('\n',fp);

	    /**
	     ***  assign zeros to the corresponding weights
	     ***  so that subsequent calls to "scale_residuals"
	     ***  will only measure the pivoted equations.
	     **/
	    sys->weights.vec[row] = 0.0;
	    sys->residuals.vec[row] = 0.0;
	    sys->residuals.accurate = FALSE;
	    mtx_mult_row(sys->J.mtx,row,0.0,&(sys->J.reg.col));
	 }
      }
      if( !sys->residuals.accurate ) {
	 square_norm( &(sys->residuals) );
	 sys->residuals.accurate = TRUE;
	 sys->update.weights = 0;  /* re-compute weights next iteration. */
      }
   }
   if( sys->J.rank < sys->J.reg.col.high-sys->J.reg.col.low+1 ) {
      int32 col;
      for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high ; col++ ) {
	 int32 org_col;
	 org_col = mtx_col_to_org(sys->J.mtx,col);
	 if( !set_is_member(sys->J.varpivots,org_col) ) {
	    struct var_variable *var;
	    var = sys->vlist[org_col];
	    FPRINTF(fp,"%-40s ---> ","Variable not pivoted");
	    print_var_name(fp,sys,var);
	    PUTC('\n',fp);
	    /**
	     ***  If we're not optimizing (everything should be
	     ***  pivotable) or this was one of the dependent variables,
	     ***  consider this variable as if it were fixed.
	     **/
	    if( col <= sys->J.reg.col.high - sys->ZBZ.order )
	       mtx_mult_col(sys->J.mtx,col,0.0,&(sys->J.reg.row));
	 }
      }
   }
   FPRINTF(fp,"%-40s ---> %d (%s)\n","Jacobian rank", sys->J.rank,
	   sys->J.singular ? "deficient":"full");
   if (sys->p.output.less_important) {
     FPRINTF(LIF(sys),"%-40s ---> %g\n","Smallest pivot",
             linsol_smallest_pivot(sys->J.sys));
   }
}


static void calc_ZBZ(sys)
slv0_system_t sys;
/**
 ***  Updates the reduced hessian matrix.
 **/
{
   mtx_coord_t nz;

   if( sys->ZBZ.accurate ) return;

   for( nz.row = 0; nz.row < sys->ZBZ.order; nz.row++ ) {
      for( nz.col = 0; nz.col <= nz.row; nz.col++ ) {
	 int32 col, depr, depc;
	 col = nz.row+sys->J.reg.col.high+1-sys->ZBZ.order;
	 depr = mtx_col_to_org(sys->J.mtx,col);
	 col = nz.col+sys->J.reg.col.high+1-sys->ZBZ.order;
	 depc = mtx_col_to_org(sys->J.mtx,col);
	 sys->ZBZ.mtx[nz.row][nz.col] = (nz.row==nz.col ? 1.0 : 0.0);
	 col = sys->J.reg.col.low;
	 for( ; col <= sys->J.reg.col.high - sys->ZBZ.order; col++ ) {
	    int32 ind = mtx_col_to_org(sys->J.mtx,col);
	    if( set_is_member(sys->J.varpivots,ind) )
	       sys->ZBZ.mtx[nz.row][nz.col] +=
		  (-linsol_org_col_dependency(sys->J.sys,depr,ind))*
		     (-linsol_org_col_dependency(sys->J.sys,depc,ind));
	 }
	 if( nz.row != nz.col )
	    sys->ZBZ.mtx[nz.col][nz.row] =
	       sys->ZBZ.mtx[nz.row][nz.col];
      }
   }
   if( OPTIMIZING(sys) ) {
      struct hessian_data *update;
      for( update=sys->B; update != NULL; update = update->next ) {
	 mtx_coord_t nz;
	 for( nz.row=0; nz.row < sys->ZBZ.order; nz.row++ ) {
	    int32 col, dep;
	    col = nz.row + sys->J.reg.col.high + 1 - sys->ZBZ.order;
	    dep = mtx_col_to_org(sys->J.mtx,col);
	    sys->ZBZ.Zy[nz.row] = update->y.vec[col];
	    sys->ZBZ.ZBs[nz.row] = update->Bs.vec[col];
	    col = sys->J.reg.col.low;
	    for( ; col <= sys->J.reg.col.high - sys->ZBZ.order; col++ ) {
	       int32 ind = mtx_col_to_org(sys->J.mtx,col);
	       if( set_is_member(sys->J.varpivots,ind) ) {
		  sys->ZBZ.Zy[nz.row] += update->y.vec[col]*
		     (-linsol_org_col_dependency(sys->J.sys,dep,ind));
		  sys->ZBZ.ZBs[nz.row] += update->Bs.vec[col]*
		     (-linsol_org_col_dependency(sys->J.sys,dep,ind));
	       }
	    }
	    for( nz.col=0; nz.col <= nz.row; nz.col++ ) {
	       sys->ZBZ.mtx[nz.row][nz.col] += update->ys > 0.0 ?
		  sys->ZBZ.Zy[nz.row]*sys->ZBZ.Zy[nz.col]/update->ys : 0.0;
	       sys->ZBZ.mtx[nz.row][nz.col] -= update->sBs > 0.0 ?
		  sys->ZBZ.ZBs[nz.row]*sys->ZBZ.ZBs[nz.col]/update->sBs : 0.0;
	       if( nz.row != nz.col )
		  sys->ZBZ.mtx[nz.col][nz.row] =
		     sys->ZBZ.mtx[nz.row][nz.col];
	    }
	 }
      }
   }
   sys->ZBZ.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"\nReduced Hessian:  \n");
   debug_out_hessian(LIF(sys),sys);
#endif
}


static void calc_rhs(sys,vec,scalar,transpose)
slv0_system_t sys;
struct vec_vector *vec;
real64 scalar;
boolean transpose;
/**
 ***  Calculates just the jacobian RHS.  This function should be used to
 ***  supplement calculation of the jacobian.  The vector vec must
 ***  already be calculated and scaled so as to simply be added to the
 ***  rhs.  Caller is responsible for initially zeroing the rhs vector.
 **/
{
   if( transpose ) {     /* vec is indexed by col */
      int32 col;
      for( col=vec->rng->low; col<=vec->rng->high; col++ )
	 sys->J.rhs[mtx_col_to_org(sys->J.mtx,col)] +=
	    scalar*vec->vec[col];

   } else {              /* vec is indexed by row */
      int32 row;
      for( row=vec->rng->low; row<=vec->rng->high; row++ )
	 sys->J.rhs[mtx_row_to_org(sys->J.mtx,row)] +=
	    scalar*vec->vec[row];
   }
   linsol_rhs_was_changed(sys->J.sys,sys->J.rhs);
}


static void calc_multipliers(sys)
slv0_system_t sys;
/**
 ***  Computes the lagrange multipliers for the equality constraints.
 **/
{
   if( sys->multipliers.accurate )
      return;

   if ( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->multipliers));
      sys->multipliers.norm2 = 0.0;
   } else {
      linsol_system_t lsys = sys->J.sys;
      int32 row;
      sys->J.rhs = linsol_get_rhs(lsys,0);
      mem_zero_byte_cast(sys->J.rhs,0.0,sys->cap*sizeof(real64));
      calc_rhs(sys, &(sys->gradient), -1.0, TRUE );
      linsol_solve(lsys,sys->J.rhs);
      row = sys->multipliers.rng->low;
      for( ; row <= sys->multipliers.rng->high; row++ ) {
	 struct rel_relation *rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
	 sys->multipliers.vec[row] = linsol_var_value
	    (lsys,sys->J.rhs,mtx_row_to_org(sys->J.mtx,row));
	 rel_set_multiplier(rel,sys->multipliers.vec[row]*
			    sys->weights.vec[row]);

      }
      if (sys->iarray[SP0_SAVLIN]) {
        FILE *ldat;
        int32 ov;
        sprintf(savlinfilename,"%s%d",savlinfilebase,savlinnum++);
        ldat=fopen(savlinfilename,"w");
        FPRINTF(ldat,
                "================= multipliersrhs (orgcoled) itn %d =====\n",
                sys->s.iteration);
        debug_write_array(ldat,sys->J.rhs,sys->cap);
        FPRINTF(ldat,
                "================= multipliers (orgrowed) ============\n");
        for(ov=0 ; ov < sys->cap; ov++ )
          FPRINTF(ldat,"%.20g\n",linsol_var_value(lsys,sys->J.rhs,ov));
        fclose(ldat);
      }
      square_norm( &(sys->multipliers) );
   }
   sys->multipliers.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Multipliers:  ");
   debug_out_vector(LIF(sys),sys,&(sys->multipliers));
#endif
}


static void calc_stationary(sys)
slv0_system_t sys;
/**
 ***  Computes the gradient of the lagrangian which
 ***  should be zero at the optimum solution.
 **/
{
   if( sys->stationary.accurate )
      return;

   if ( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->stationary));
      sys->stationary.norm2 = 0.0;
   } else {
      int32 col;
      matrix_product(sys->J.mtx, &(sys->multipliers),
		     &(sys->stationary), 1.0, TRUE);
      col = sys->stationary.rng->low;
      for( ; col <= sys->stationary.rng->high; col++ )
	 sys->stationary.vec[col] += sys->gradient.vec[col];
      square_norm( &(sys->stationary) );
   }
   sys->stationary.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Stationary:  ");
   debug_out_vector(LIF(sys),sys,&(sys->stationary));
#endif
}


static void calc_gamma(sys)
slv0_system_t sys;
/**
 ***  Calculate the gamma vector.
 **/
{
   if( sys->gamma.accurate )
      return;

   matrix_product(sys->J.mtx, &(sys->residuals),
		  &(sys->gamma), -1.0, TRUE);
   square_norm( &(sys->gamma) );
   sys->gamma.accurate = TRUE;

#if DEBUG
   FPRINTF(LIF(sys),"Gamma:  ");
   debug_out_vector(LIF(sys),sys,&(sys->gamma));
#endif
}

static void calc_Jgamma(sys)
slv0_system_t sys;
/**
 ***  Calculate the Jgamma vector.
 **/
{
   if( sys->Jgamma.accurate )
      return;

   matrix_product(sys->J.mtx, &(sys->gamma),
		  &(sys->Jgamma), 1.0, FALSE);
   square_norm( &(sys->Jgamma) );
   sys->Jgamma.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Jgamma:  ");
   debug_out_vector(LIF(sys),sys,&(sys->Jgamma));
#endif
}


static void calc_newton(sys)
slv0_system_t sys;
/**
 ***  Computes a step to solve the linearized equations.
 **/
{
   linsol_system_t lsys = sys->J.sys;
   int32 col;

   if( sys->newton.accurate )
      return;

   sys->J.rhs = linsol_get_rhs(lsys,1);
   mem_zero_byte_cast(sys->J.rhs,0.0,sys->cap*sizeof(real64));
   calc_rhs(sys, &(sys->residuals), -1.0, FALSE);
   linsol_solve(lsys,sys->J.rhs);
   col = sys->newton.rng->low;
   for( ; col <= sys->newton.rng->high; col++ )
      sys->newton.vec[col] = linsol_var_value
	 (lsys,sys->J.rhs,mtx_col_to_org(sys->J.mtx,col));
   if (sys->iarray[SP0_SAVLIN]) {
     FILE *ldat;
     int32 ov;
     sprintf(savlinfilename,"%s%d",savlinfilebase,savlinnum++);
     ldat=fopen(savlinfilename,"w");
     FPRINTF(ldat,"================= resids (orgrowed) itn %d =====\n",
             sys->s.iteration);
     debug_write_array(ldat,sys->J.rhs,sys->cap);
     FPRINTF(ldat,"================= vars (orgcoled) ============\n");
     for(ov=0 ; ov < sys->cap; ov++ )
       FPRINTF(ldat,"%.20g\n",linsol_var_value(lsys,sys->J.rhs,ov));
     fclose(ldat);
   }
   square_norm( &(sys->newton) );
   sys->newton.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Newton:  ");
   debug_out_vector(LIF(sys),sys,&(sys->newton));
#endif
}


static void calc_Bnewton(sys)
slv0_system_t sys;
/**
 ***  Computes an update to the product B and newton.
 **/
{
   if( sys->Bnewton.accurate )
      return;

   if ( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->Bnewton));
      sys->Bnewton.norm2 = 0.0;
   } else {
      struct hessian_data *update;
      copy_vector(&(sys->newton),&(sys->Bnewton));
      for( update=sys->B; update != NULL; update = update->next ) {
	 int32 col;
	 real64 yn = inner_product( &(update->y),&(sys->newton) );
	 real64 sBn = inner_product( &(update->Bs),&(sys->newton) );
	 col = sys->Bnewton.rng->low;
	 for( ; col <= sys->Bnewton.rng->high; col++ ) {
	    sys->Bnewton.vec[col] += update->ys > 0.0 ?
	       (update->y.vec[col])*yn/update->ys : 0.0;
	    sys->Bnewton.vec[col] -= update->sBs > 0.0 ?
	       (update->Bs.vec[col])*sBn/update->sBs : 0.0;
	 }
      }
      square_norm( &(sys->Bnewton) );
   }
   sys->Bnewton.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Bnewton:  ");
   debug_out_vector(LIF(sys),sys,&(sys->Bnewton));
#endif
}


static void calc_nullspace(sys)
slv0_system_t sys;
/**
 ***  Calculate the nullspace move.
 **/
{
   if( sys->nullspace.accurate )
      return;

   if( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->nullspace));
      sys->nullspace.norm2 = 0.0;
   } else {
      mtx_coord_t nz;
      zero_vector(&(sys->nullspace));
      for( nz.row=0; nz.row < sys->ZBZ.order; nz.row++ ) {
	 int32 col, dep, ndx;
	 col = nz.row+sys->J.reg.col.high+1-sys->ZBZ.order;
	 dep = mtx_col_to_org(sys->J.mtx,col);
	 sys->nullspace.vec[col] = -sys->stationary.vec[col] -
	    sys->Bnewton.vec[col];
	 ndx = sys->J.reg.col.low;
	 for( ; ndx <= sys->J.reg.col.high - sys->ZBZ.order; ndx++ ) {
	    int32 ind = mtx_col_to_org(sys->J.mtx,ndx);
	    if( set_is_member(sys->J.varpivots,ind) )
	       sys->nullspace.vec[col] -=
		  (sys->stationary.vec[ndx] + sys->Bnewton.vec[ndx])*
		     (-linsol_org_col_dependency(sys->J.sys,dep,ind));
	 }
      }
      /**
       ***  Must invert ZBZ first.  It's symmetric so
       ***  can find Cholesky factors.  Essentially, find
       ***  the "square root" of the matrix such that
       ***
       ***         T
       ***  L U = U U = ZBZ, where U is an upper triangular
       ***  matrix.
       **/
      for( nz.row = 0; nz.row < sys->ZBZ.order; nz.row++ ) {
	 for( nz.col = nz.row; nz.col < sys->ZBZ.order; nz.col++ ) {
	    int32 col;
	    for( col = 0; col < nz.row; col++ )
	       sys->ZBZ.mtx[nz.row][nz.col] -=
		  sys->ZBZ.mtx[nz.row][col]*
		     sys->ZBZ.mtx[col][nz.col];
	    if( nz.row == nz.col )
	       sys->ZBZ.mtx[nz.row][nz.col] =
		  calc_sqrt_D0(sys->ZBZ.mtx[nz.row][nz.col]);
	    else {
	       sys->ZBZ.mtx[nz.row][nz.col] /=
		  sys->ZBZ.mtx[nz.row][nz.row];
	       sys->ZBZ.mtx[nz.col][nz.row] =
		  sys->ZBZ.mtx[nz.row][nz.col];
	    }
	 }
      }
#if DEBUG
   FPRINTF(LIF(sys),"\nInverse Reduced Hessian:  \n");
   debug_out_hessian(LIF(sys),sys);
#endif
      /**
       ***  forward substitute
       **/
      for( nz.row = 0; nz.row < sys->ZBZ.order; nz.row++ ) {
	 int32 offset = sys->J.reg.col.high+1-sys->ZBZ.order;
	 for( nz.col = 0; nz.col < nz.row; nz.col++ ) {
	    sys->nullspace.vec[nz.row+offset] -=
	       sys->nullspace.vec[nz.col+offset]*
		  sys->ZBZ.mtx[nz.row][nz.col];
	 }
	 sys->nullspace.vec[nz.row+offset] /=
	    sys->ZBZ.mtx[nz.row][nz.row];
      }

      /**
       ***  backward substitute
       **/
      for( nz.row = sys->ZBZ.order-1; nz.row >= 0; nz.row-- ) {
	 int32 offset = sys->J.reg.col.high+1-sys->ZBZ.order;
	 for( nz.col = nz.row+1; nz.col < sys->ZBZ.order; nz.col++ ) {
	    sys->nullspace.vec[nz.row+offset] -=
	       sys->nullspace.vec[nz.col+offset]*
		  sys->ZBZ.mtx[nz.row][nz.col];
	 }
	 sys->nullspace.vec[nz.row+offset] /=
	    sys->ZBZ.mtx[nz.row][nz.row];
      }
      square_norm( &(sys->nullspace) );
   }
   sys->nullspace.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Nullspace:  ");
   debug_out_vector(LIF(sys),sys,&(sys->nullspace));
#endif
}

static void calc_varstep1(sys)
slv0_system_t sys;
/**
 ***  Calculate the 1st order descent direction for phi
 ***  in the variables.
 **/
{
   if( sys->varstep1.accurate )
      return;

   if( !OPTIMIZING(sys) ) {
      copy_vector(&(sys->gamma),&(sys->varstep1));
      sys->varstep1.norm2 = sys->gamma.norm2;
   } else {
      int32 col;
      col = sys->varstep1.rng->low;
      for( ; col <= sys->varstep1.rng->high; col++ )
	 sys->varstep1.vec[col] = sys->p.rho*sys->gamma.vec[col] -
	    sys->stationary.vec[col];
      square_norm( &(sys->varstep1) );
   }
   sys->varstep1.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Varstep1:  ");
   debug_out_vector(LIF(sys),sys,&(sys->varstep1));
#endif
}


static void calc_Bvarstep1(sys)
slv0_system_t sys;
/**
 ***  Computes an update to the product B and varstep1.
 **/
{
   if( sys->Bvarstep1.accurate )
      return;

   if ( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->Bvarstep1));
      sys->Bvarstep1.norm2 = 0.0;
   } else {
      struct hessian_data *update;
      copy_vector(&(sys->varstep1),&(sys->Bvarstep1));
      for( update=sys->B; update != NULL; update = update->next ) {
	 int32 col;
	 real64 yv = inner_product( &(update->y),&(sys->varstep1) );
	 real64 sBv = inner_product( &(update->Bs),&(sys->varstep1) );
	 col = sys->Bvarstep1.rng->low;
	 for( ; col <= sys->Bvarstep1.rng->high; col++ ) {
	    sys->Bvarstep1.vec[col] += update->ys > 0.0 ?
	       (update->y.vec[col])*yv/update->ys : 0.0;
	    sys->Bvarstep1.vec[col] -= update->sBs > 0.0 ?
	       (update->Bs.vec[col])*sBv/update->sBs : 0.0;
	 }
      }
      square_norm( &(sys->Bvarstep1) );
   }
   sys->Bvarstep1.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Bvarstep1:  ");
   debug_out_vector(LIF(sys),sys,&(sys->Bvarstep1));
#endif
}


static void calc_varstep2(sys)
slv0_system_t sys;
/**
 ***  Calculate the 2nd order descent direction for phi
 ***  in the variables.
 **/
{
   if( sys->varstep2.accurate )
      return;

   if( !OPTIMIZING(sys) ) {
      copy_vector(&(sys->newton),&(sys->varstep2));
      sys->varstep2.norm2 = sys->newton.norm2;
   } else {
      int32 col;
      col = sys->varstep2.rng->low;
      for( ; col <= sys->varstep2.rng->high - sys->ZBZ.order ; ++col ) {
	 int32 dep;
	 int32 ind = mtx_col_to_org(sys->J.mtx,col);
	 sys->varstep2.vec[col] = sys->newton.vec[col];
	 if( set_is_member(sys->J.varpivots,ind) ) {
	    dep = sys->varstep2.rng->high + 1 - sys->ZBZ.order;
	    for( ; dep <= sys->varstep2.rng->high; dep++ )
	       sys->varstep2.vec[col] += sys->nullspace.vec[dep]*
		  (-linsol_org_col_dependency(sys->J.sys,dep,ind));
	 }
      }
      col = sys->varstep2.rng->high + 1 - sys->ZBZ.order;
      for( ; col <= sys->varstep2.rng->high; ++col )
	 sys->varstep2.vec[col] = sys->nullspace.vec[col] +
	    sys->newton.vec[col];
      square_norm( &(sys->varstep2) );
   }
   sys->varstep2.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Varstep2:  ");
   debug_out_vector(LIF(sys),sys,&(sys->varstep2));
#endif
}


static void calc_Bvarstep2(sys)
slv0_system_t sys;
/**
 ***  Computes an update to the product B and varstep2.
 **/
{
   if( sys->Bvarstep2.accurate )
      return;

   if ( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->Bvarstep2));
      sys->Bvarstep2.norm2 = 0.0;
   } else {
      struct hessian_data *update;
      copy_vector(&(sys->varstep2),&(sys->Bvarstep2));
      for( update=sys->B; update != NULL; update = update->next ) {
	 int32 col;
	 real64 yv = inner_product( &(update->y),&(sys->varstep2) );
	 real64 sBv = inner_product( &(update->Bs),&(sys->varstep2) );
	 col = sys->Bvarstep2.rng->low;
	 for( ; col <= sys->Bvarstep2.rng->high; col++ ) {
	    sys->Bvarstep2.vec[col] += update->ys > 0.0 ?
	       (update->y.vec[col])*yv/update->ys : 0.0;
	    sys->Bvarstep2.vec[col] -= update->sBs > 0.0 ?
	       (update->Bs.vec[col])*sBv/update->sBs : 0.0;
	 }
      }
      square_norm( &(sys->Bvarstep2) );
   }
   sys->Bvarstep2.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Bvarstep2:  ");
   debug_out_vector(LIF(sys),sys,&(sys->Bvarstep2));
#endif
}


static void calc_mulstep1(sys)
slv0_system_t sys;
/**
 ***  Calculate the negative gradient direction of phi in the
 ***  multipliers.
 **/
{
   if( sys->mulstep1.accurate )
      return;

   if( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->mulstep1));
      sys->mulstep1.norm2 = 0.0;
   } else {
      int32 row;
      row = sys->mulstep1.rng->low;
      for( ; row <= sys->mulstep1.rng->high; row++ )
	 sys->mulstep1.vec[row] = -sys->residuals.vec[row];
      square_norm( &(sys->mulstep1) );
   }
   sys->mulstep1.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Mulstep1:  ");
   debug_out_vector(LIF(sys),sys,&(sys->mulstep1));
#endif
}


static void calc_mulstep2(sys)
slv0_system_t sys;
/**
 ***  Calculate the mulstep2 direction of phi in the
 ***  multipliers.
 **/
{
   if( sys->mulstep2.accurate )
      return;

   if( !OPTIMIZING(sys) ) {
      zero_vector(&(sys->mulstep2));
      sys->mulstep2.norm2 = 0.0;
   } else {
      linsol_system_t lsys = sys->J.sys;
      int32 row;
      sys->J.rhs = linsol_get_rhs(lsys,2);
      mem_zero_byte_cast(sys->J.rhs,0.0,sys->cap*sizeof(real64));
      calc_rhs(sys, &(sys->Bvarstep2), -1.0, TRUE);
      calc_rhs(sys, &(sys->stationary), -1.0, TRUE);
      linsol_solve(lsys,sys->J.rhs);
      row = sys->mulstep2.rng->low;
      for( ; row <= sys->mulstep2.rng->high; row++ )
	 sys->mulstep2.vec[row] = linsol_var_value
	    (lsys,sys->J.rhs,mtx_row_to_org(sys->J.mtx,row));
      if (sys->iarray[SP0_SAVLIN]) {
        FILE *ldat;
        int32 ov;
        sprintf(savlinfilename,"%s%d",savlinfilebase,savlinnum++);
        ldat=fopen(savlinfilename,"w");
        FPRINTF(ldat,
                "================= mulstep2rhs (orgcoled) itn %d =======\n",
                sys->s.iteration);
        debug_write_array(ldat,sys->J.rhs,sys->cap);
        FPRINTF(ldat,
                "================= mulstep2vars (orgrowed) ============\n");
        for(ov=0 ; ov < sys->cap; ov++ )
          FPRINTF(ldat,"%.20g\n",linsol_var_value(lsys,sys->J.rhs,ov));
        fclose(ldat);
      }
      square_norm( &(sys->mulstep2) );
   }
   sys->mulstep2.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Mulstep2:  ");
   debug_out_vector(LIF(sys),sys,&(sys->mulstep2));
#endif
}


static void calc_phi(sys)
slv0_system_t sys;
/**
 ***  Computes the global minimizing function Phi.
 **/
{
   if( !OPTIMIZING(sys) )
      sys->phi = 0.5*sys->residuals.norm2;
   else {
      sys->phi = sys->objective;
      sys->phi += inner_product( &(sys->multipliers),&(sys->residuals) );
      sys->phi += 0.5*sys->p.rho*sys->residuals.norm2;
   }
}


/**
 ***  OK.  Here's where we compute the actual step to be taken.  It will
 ***  be some linear combination of the 1st order and 2nd order steps.
 **/

typedef real64 sym_2x2_t[3];        /* Stores symmetric 2x2 matrices */

struct parms_t {
   real64 low,high,guess;           /* Used to search for parameter */
};

struct calc_step_vars {
   sym_2x2_t coef1, coef2;
   real64 rhs[2];   /* RHS for 2x2 system */
   struct parms_t parms;
   real64 alpha1, alpha2;
   real64 error;    /* Error between step norm and sys->maxstep */
};

static void calc_2x2_system(sys,vars)
slv0_system_t sys;
struct calc_step_vars *vars;
/**
 ***  Calculates 2x2 system (coef1,coef2,rhs).
 **/
{
   vars->coef1[0] = (2.0*sys->phi/sys->newton.norm2)*
      calc_sqrt_D0(sys->newton.norm2)/calc_sqrt_D0(sys->gamma.norm2);
   vars->coef1[1] = 1.0;
   vars->coef1[2] = (sys->Jgamma.norm2/sys->gamma.norm2)*
      calc_sqrt_D0(sys->newton.norm2)/calc_sqrt_D0(sys->gamma.norm2);

   vars->coef2[0] = 1.0;
   vars->coef2[1] = 2.0*sys->phi/
      calc_sqrt_D0(sys->newton.norm2)/calc_sqrt_D0(sys->gamma.norm2);
   vars->coef2[2] = 1.0;

   vars->rhs[0] = 2.0*sys->phi/
      sys->maxstep/calc_sqrt_D0(sys->gamma.norm2);
   vars->rhs[1] = calc_sqrt_D0(sys->newton.norm2)/sys->maxstep;
}

static void coefs_from_parm(sys,vars)
slv0_system_t sys;
struct calc_step_vars *vars;
/**
 ***  Determines alpha1 and alpha2 from the parameter (guess).
 **/
{
#define DETZERO (1e-8)

   sym_2x2_t coef;     /* Actual coefficient matrix */
   real64 det;   /* Determinant of coefficient matrix */
   int i;

   for( i=0 ; i<3 ; ++i ) coef[i] =
      vars->coef1[i] + vars->parms.guess * vars->coef2[i];
   det = coef[0]*coef[2] - coef[1]*coef[1];
   if( det < 0.0 )
      FPRINTF(MIF(sys),"%-40s ---> %g\n",
	      "    Unexpected negative determinant!",det);
   if( det <= DETZERO ) {
      /**
       ***  varstep2 and varstep1 are essentially parallel:
       ***  adjust length of either
       **/
      vars->alpha2 = 0.0;
      vars->alpha1 = 1.0;
   } else {
      vars->alpha2  = (vars->rhs[0]*coef[2] - vars->rhs[1]*coef[1])/det;
      vars->alpha1 = (vars->rhs[1]*coef[0] - vars->rhs[0]*coef[1])/det;
   }
#undef DETZERO
}

static real64 step_norm2(sys,vars)
slv0_system_t sys;
struct calc_step_vars *vars;
/**
 ***  Computes step vector length based on 1st order and 2nd order
 ***  vectors and their coefficients.
 **/
{
   return sys->maxstep*sys->maxstep*
      (vars->alpha2 * vars->alpha2 +
       vars->alpha2 * vars->alpha1 * sys->phi/
       calc_sqrt_D0(sys->varstep2.norm2 + sys->mulstep2.norm2)/
       calc_sqrt_D0(sys->varstep1.norm2 + sys->mulstep1.norm2) +
       vars->alpha1 * vars->alpha1);
}

static void adjust_parms(sys,vars)
slv0_system_t sys;
struct calc_step_vars *vars;
/**
 ***  Re-guesses the parameters based on
 ***  step size vs. target value.
 **/
{
   vars->error = (calc_sqrt_D0(step_norm2(sys,vars))/sys->maxstep) - 1.0;
   if( vars->error > 0.0 ) {
      /* Increase parameter (to decrease step length) */
      vars->parms.low = vars->parms.guess;
      vars->parms.guess = (vars->parms.high>3.0*vars->parms.guess)
         ? 2.0*vars->parms.guess
	    : 0.5*(vars->parms.low + vars->parms.high);
   } else {
      /* Decrease parameter (to increase step norm) */
      vars->parms.high = vars->parms.guess;
      vars->parms.guess = 0.5*(vars->parms.low + vars->parms.high);
   }
}

static void compute_step(sys,vars)
slv0_system_t sys;
struct calc_step_vars *vars;
/**
 ***  Computes the step based on the coefficients in vars.
 **/
{
   int32 row,col;
   real64 tot1_norm2, tot2_norm2;

   tot1_norm2 = sys->varstep1.norm2 + sys->mulstep1.norm2;
   tot2_norm2 = sys->varstep2.norm2 + sys->mulstep2.norm2;
   if( !sys->varstep.accurate ) {
      for( col=sys->varstep.rng->low ; col<=sys->varstep.rng->high ; ++col )
	 if( (vars->alpha2 == 1.0) && (vars->alpha1 == 0.0) ) {
	    sys->varstep.vec[col] = sys->maxstep *
	       sys->varstep2.vec[col]/calc_sqrt_D0(tot2_norm2);
	 } else if( (vars->alpha2 == 0.0) && (vars->alpha1 == 1.0) ) {
	    sys->varstep.vec[col] = sys->maxstep *
	       sys->varstep1.vec[col]/calc_sqrt_D0(tot1_norm2);
	 } else if( (vars->alpha2 != 0.0) && (vars->alpha1 != 0.0) ) {
	    sys->varstep.vec[col] = sys->maxstep*
	       (
		vars->alpha2*sys->varstep2.vec[col]/calc_sqrt_D0(tot2_norm2) +
		vars->alpha1*sys->varstep1.vec[col]/calc_sqrt_D0(tot1_norm2)
		);
	 }
      sys->varstep.accurate = TRUE;
   }
   if( !sys->mulstep.accurate ) {
      for( row=sys->mulstep.rng->low ; row<=sys->mulstep.rng->high ; ++row )
	 if( (vars->alpha2 == 1.0) && (vars->alpha1 == 0.0) ) {
	    sys->mulstep.vec[row] = sys->maxstep *
	       sys->mulstep2.vec[row]/calc_sqrt_D0(tot2_norm2);
	 } else if( (vars->alpha2 == 0.0) && (vars->alpha1 == 1.0) ) {
	    sys->mulstep.vec[row] = sys->maxstep *
	       sys->mulstep1.vec[row]/calc_sqrt_D0(tot1_norm2);
	 } else if( (vars->alpha2 != 0.0) && (vars->alpha1 != 0.0) ) {
	    sys->mulstep.vec[row] = sys->maxstep*
	       (
		vars->alpha2*sys->mulstep2.vec[row]/calc_sqrt_D0(tot2_norm2) +
		vars->alpha1*sys->mulstep1.vec[row]/calc_sqrt_D0(tot1_norm2)
		);
	 }
      sys->mulstep.accurate = TRUE;
   }
#if DEBUG
   FPRINTF(LIF(sys),"Varstep:  ");
   debug_out_vector(LIF(sys),sys,&(sys->varstep));
   FPRINTF(LIF(sys),"Mulstep:  ");
   debug_out_vector(LIF(sys),sys,&(sys->mulstep));
#endif
}


static void calc_step(sys, minor)
slv0_system_t sys;
int minor;
/**
 ***  Calculates step vector, based on sys->maxstep, and the varstep2/
 ***  varstep1 and mulstep2/mulstep1 vectors.  Nothing is assumed to be
 ***  calculated, except the weights and the jacobian (scaled).  Also,
 ***  the step is not checked for legitimacy.
 ***  NOTE: the step is scaled.
 **/
{
#define STEPSIZEERR_MAX   1e-4
   /* Step size must be determined this precisely */
#define PARMRNG_MIN       1e-12
   /* OR parameter range must be this narrow to exit */

   struct calc_step_vars vars;
   real64 tot1_norm2, tot2_norm2;

   if( sys->varstep.accurate && sys->mulstep.accurate )
      return;
   if (sys->p.output.less_important) {
     FPRINTF(LIF(sys),"\n%-40s ---> %d\n", "    Step trial",minor);
   }

   tot1_norm2 = sys->varstep1.norm2 + sys->mulstep1.norm2;
   tot2_norm2 = sys->varstep2.norm2 + sys->mulstep2.norm2;
   if( (tot1_norm2 == 0.0) && (tot2_norm2 == 0.0) ) {
      /* Take no step at all */
      vars.alpha1 = 0.0;
      vars.alpha2 = 0.0;
      sys->maxstep = 0.0;
      sys->varstep.norm2 = 0.0;
      sys->mulstep.norm2 = 0.0;

   } else if( (tot2_norm2 > 0.0) && OPTIMIZING(sys) ) {
      /* Stay in varstep2 direction */
      vars.alpha1 = 0.0;
      vars.alpha2 = 1.0;
      sys->maxstep = MIN(sys->maxstep,calc_sqrt_D0(tot2_norm2));
      sys->varstep.norm2 = calc_sqr_D0(sys->maxstep)*
	 sys->varstep2.norm2/tot2_norm2;
      sys->mulstep.norm2 = calc_sqr_D0(sys->maxstep)*
	 sys->mulstep2.norm2/tot2_norm2;

   } else if( (tot2_norm2>0.0)&&(calc_sqrt_D0(tot2_norm2)<=sys->maxstep) ) {
      /* Attempt step in varstep2 direction */
      vars.alpha1 = 0.0;
      vars.alpha2 = 1.0;
      sys->maxstep = calc_sqrt_D0(tot2_norm2);
      sys->varstep.norm2 = calc_sqr_D0(sys->maxstep)*
	 sys->varstep2.norm2/tot2_norm2;
      sys->mulstep.norm2 = calc_sqr_D0(sys->maxstep)*
	 sys->mulstep2.norm2/tot2_norm2;

   } else if( (tot2_norm2==0.0 || sys->s.block.current_size==1) &&
	     (tot1_norm2 > 0.0) ) {
      /* Attempt step in varstep1 direction */
      vars.alpha1 = 1.0;
      vars.alpha2 = 0.0;
      if ( (sys->gamma.norm2/sys->Jgamma.norm2)*
	  calc_sqrt_D0(sys->gamma.norm2) <= sys->maxstep )
	 sys->maxstep = (sys->gamma.norm2/sys->Jgamma.norm2)*
	    calc_sqrt_D0(sys->gamma.norm2);
      sys->varstep.norm2 = calc_sqr_D0(sys->maxstep)*
	 sys->varstep1.norm2/tot1_norm2;
      sys->mulstep.norm2 = calc_sqr_D0(sys->maxstep)*
	 sys->mulstep1.norm2/tot1_norm2;

   } else {
      /* Attempt step in varstep1-varstep2 direction */
      vars.parms.low = 0.0;
      vars.parms.high = MAXDOUBLE;
      vars.parms.guess = 1.0;
      calc_2x2_system(sys,&vars);
      do {
	 coefs_from_parm(sys, &vars);
	 adjust_parms(sys, &vars);
      } while( fabs(vars.error) > STEPSIZEERR_MAX &&
	      vars.parms.high - vars.parms.low > PARMRNG_MIN );
      if (sys->p.output.less_important) {
        FPRINTF(LIF(sys),"%-40s ---> %g\n",
                "    parameter high", vars.parms.high);
        FPRINTF(LIF(sys),"%-40s ---> %g\n",
                "    parameter low", vars.parms.low);
        FPRINTF(LIF(sys),"%-40s ---> %g\n",
                "    Error in step length", vars.error);
      }
      sys->varstep.norm2 = step_norm2(sys, &vars);
      sys->mulstep.norm2 = 0.0;
   }

   if (sys->p.output.less_important) {
     FPRINTF(LIF(sys),"%-40s ---> %g\n", "    Alpha1 coefficient (normalized)",
	     vars.alpha1);
     FPRINTF(LIF(sys),"%-40s ---> %g\n", "    Alpha2 coefficient (normalized)",
	     vars.alpha2);
   }
   compute_step(sys,&vars);
   return;

#undef STEPSIZEERR_MAX
#undef PARMRNG_MIN
}



/**
 ***  Variable values maintenance
 ***  ---------------------------
 ***     restore_variables(sys)
 ***     coef = required_coef_to_stay_inbounds(sys)
 ***     apply_step(sys,coef)
 ***     step_accepted(sys)
 ***     change_maxstep(sys,maxstep)
 **/

static void restore_variables(sys)
slv0_system_t sys;
/**
 ***  Restores the values of the variables before applying
 ***  a step.
 **/
{
   int32 col;

   for( col = sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ ) {
      struct var_variable *var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
      var_set_value(var,sys->variables.vec[col]*sys->nominals.vec[col]);
   }
}


static real64 required_coef_to_stay_inbounds(sys)
slv0_system_t sys;
/**
 ***  Calculates the maximum fraction of the step which can be
 ***  taken without going out of bounds.  If the entire step can be
 ***  taken, 1.0 is returned.  Otherwise a value less than 1 is
 ***  returned.  It is assumed that the current variable values
 ***  are within their bounds.  The step must be calculated.
 **/
{
   real64 mincoef;
   int32 col;

   if( sys->p.ignore_bounds )
      return(1.0);

   mincoef = 1.0;
   for( col=sys->varstep.rng->low; col <= sys->varstep.rng->high; col++ ) {
      struct var_variable *var;
      real64 coef,dx,val,bnd;
      var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
      coef = 1.0;
      dx = sys->varstep.vec[col] * sys->nominals.vec[col];
      bnd = var_upper_bound(var);
      if( (val=var_value(var)) + dx > bnd )
	 coef = MIN((bnd-val)/dx, 1.0);
      bnd = var_lower_bound(var);
      if( val + dx < bnd )
	 coef = MIN((bnd-val)/dx, 1.0);
      if( coef < mincoef )
	 mincoef = coef;
   }
   return(mincoef);
}


#define TRUNCATE     FALSE     /* override projection */
static void apply_step(sys)
slv0_system_t sys;
/**
 ***  Adds sys->varstep to the variable values in block: projecting
 ***  near bounds.
 **/
{
   FILE *lif = LIF(sys);
   int nproj = 0;
   real64 bounds_coef = 1.0;
   int32 col;

   if (TRUNCATE && (!sys->p.ignore_bounds))
      bounds_coef = required_coef_to_stay_inbounds(sys);

   for( col=sys->varstep.rng->low; col <= sys->varstep.rng->high; col++ ) {
      struct var_variable *var;
      real64 dx,val,bnd;
      var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
      dx = sys->nominals.vec[col]*sys->varstep.vec[col];
      val = var_value(var);
      if (bounds_coef < 1.0) {
	 dx = dx*TOWARD_BOUNDS*bounds_coef;
	 sys->varstep.vec[col] = dx/sys->nominals.vec[col];
      } else {
	 if( !sys->p.ignore_bounds ) {
	    if( val + dx > (bnd=var_upper_bound(var)) ) {
	       dx = TOWARD_BOUNDS*(bnd-val);
	       sys->varstep.vec[col] = dx/sys->nominals.vec[col];
               if (sys->p.output.less_important) {
                 FPRINTF(lif,"%-40s ---> ",
                         "    Variable projected to upper bound");
                 print_var_name(lif,sys,var); PUTC('\n',lif);
               }
	       ++nproj;
	    } else if( val + dx < (bnd=var_lower_bound(var)) ) {
	       dx = TOWARD_BOUNDS*(bnd-val);
	       sys->varstep.vec[col] = dx/sys->nominals.vec[col];
               if (sys->p.output.less_important) {
	         FPRINTF(lif,"%-40s ---> ",
                         "    Variable projected to lower bound");
                 print_var_name(lif,sys,var); PUTC('\n',lif);
               }
	       ++nproj;
	    }
	 }
      }
      var_set_value(var,val+dx);
   }

   if( !sys->p.ignore_bounds ) {
      if (nproj > 0) {
	 square_norm(&(sys->varstep));
	 sys->progress = calc_sqrt_D0
	    (calc_sqrt_D0((sys->varstep.norm2 + sys->mulstep.norm2)*
			  (sys->varstep1.norm2 + sys->mulstep1.norm2)));
         if (sys->p.output.less_important) {
           FPRINTF(lif,"%-40s ---> %g\n", "    Projected step length (scaled)",
           calc_sqrt_D0(sys->varstep.norm2 + sys->mulstep.norm2));
           FPRINTF(lif,"%-40s ---> %g\n",
                   "    Projected progress", sys->progress);
         }
      }
      if (bounds_coef < 1.0) {
	 square_norm(&(sys->varstep));
         if (sys->p.output.less_important) {
            FPRINTF(lif,"%-40s ---> %g\n",
                    "    Truncated step length (scaled)",
                    calc_sqrt_D0(sys->varstep.norm2 + sys->mulstep.norm2));
         }
	 sys->progress = calc_sqrt_D0
	    (calc_sqrt_D0((sys->varstep.norm2 + sys->mulstep.norm2)*
			  (sys->varstep1.norm2 + sys->mulstep1.norm2)));
	    (calc_sqrt_D0(sys->varstep.norm2*sys->varstep1.norm2));
         if (sys->p.output.less_important) {
           FPRINTF(lif,"%-40s ---> %g\n",
                   "    Truncated progress", sys->progress);
         }
      }
   }

   /* Allow weighted residuals to be recalculated at new point */
   sys->residuals.accurate = FALSE;

   return;
}
#undef TRUNCATE

static void step_accepted(sys)
slv0_system_t sys;
/**
 ***  This function should be called when the step is accepted.
 **/
{
   /* Maintain update status on jacobian and weights */
   if (--(sys->update.jacobian) <= 0)
      sys->J.accurate = FALSE;

   sys->ZBZ.accurate = FALSE;
   sys->variables.accurate = FALSE;
   sys->gradient.accurate = FALSE;
   sys->multipliers.accurate = FALSE;
   sys->stationary.accurate = FALSE;
   sys->newton.accurate = FALSE;
   sys->Bnewton.accurate = FALSE;
   sys->nullspace.accurate = FALSE;
   sys->gamma.accurate = FALSE;
   sys->Jgamma.accurate = FALSE;
   sys->varstep1.accurate = FALSE;
   sys->Bvarstep1.accurate = FALSE;
   sys->varstep2.accurate = FALSE;
   sys->Bvarstep2.accurate = FALSE;
   sys->mulstep1.accurate = FALSE;
   sys->mulstep2.accurate = FALSE;
   sys->varstep.accurate = FALSE;
   sys->mulstep.accurate = FALSE;

   if( !OPTIMIZING(sys) ) {
      sys->ZBZ.accurate = TRUE;
      sys->gradient.accurate = TRUE;
      sys->multipliers.accurate = TRUE;
      sys->stationary.accurate = TRUE;
      sys->Bnewton.accurate = TRUE;
      sys->nullspace.accurate = TRUE;
      sys->Bvarstep1.accurate = TRUE;
      sys->Bvarstep2.accurate = TRUE;
   }
}

static void change_maxstep(sys,maxstep)
slv0_system_t sys;
real64 maxstep;
/**
 ***  This function changes sys->maxstep to the given number and should be
 ***  called whenever sys->maxstep is to be changed.
 **/
{
   sys->maxstep = maxstep;
   sys->varstep.accurate = FALSE;
   if( OPTIMIZING(sys) ) sys->mulstep.accurate = FALSE;
}



/**
 ***  Block routines
 ***  --------------
 ***     feas = block_feasible(sys)
 ***     insure_bounds(sys,var)
 ***     move_to_next_block(sys)
 ***     find_next_unconverged_block(sys)
 **/

static boolean block_feasible(sys)
slv0_system_t sys;
/**
 ***  Returns TRUE if the current block is feasible, FALSE otherwise.
 ***  It is assumed that the residuals have been computed.
 **/
{
   int32 row;

   if( !sys->s.calc_ok )
      return(FALSE);

   for( row = sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ ) {
      struct rel_relation *rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
      if( !rel_satisfied(rel) ) return FALSE;
   }
   return TRUE;
}

static void insure_bounds(sys,var)
slv0_system_t sys;
struct var_variable *var;
/**
 ***  Insures that the variable value is within its bounds.
 **/
{
   real64 val,low,high;
   FILE *mif = MIF(sys);

   if( sys->p.ignore_bounds )
      return;

   low = var_lower_bound(var);
   high = var_upper_bound(var);
   val = var_value(var);
   if( low > high ) {
      FPRINTF(mif,"Bounds for variable ");
      print_var_name(mif,sys,var);
      FPRINTF(mif," are inconsistent [%g,%g].\n",low,high);
      FPRINTF(mif,"Bounds will be swapped.\n");
      var_set_upper_bound(var, low);
      var_set_lower_bound(var, high);
      low = var_lower_bound(var);
      high = var_upper_bound(var);
   }

   if( low > val ) {
      FPRINTF(mif,"Variable ");
      print_var_name(mif,sys,var);
      FPRINTF(mif," was initialized below its lower bound.\n");
      FPRINTF(mif,"It will be moved to its lower bound.\n");
      var_set_value(var, low);
   } else if( val > high ) {
      FPRINTF(mif,"Variable ");
      print_var_name(mif,sys,var);
      FPRINTF(mif," was initialized above its upper bound.\n");
      FPRINTF(mif,"It will be moved to its upper bound.\n");
      var_set_value(var, high);
   }
}

static void move_to_next_block(sys)
slv0_system_t sys;
/**
 ***  Moves on to the next block, updating all of the solver information.
 ***  To move to the first block, set sys->s.block.current_block to -1 before
 ***  calling.  If already at the last block, then sys->s.block.current_block
 ***  will equal the number of blocks and the system will be declared
 ***  converged.  Otherwise, the residuals for the new block will be computed
 ***  and sys->s.calc_ok set according.
 **/
{
#ifdef KAA_DEBUG
  int32 klowcol, khighcol;
  int32 klowrow, khighrow;
  struct rel_relation *krel;
  struct var_variable *kvar;
  double ktime =0;
  int k, ktmp;
#endif

  /*   sys->clock = tm_cpu_time();   */

   if( sys->s.block.current_block >= 0 ) {

      int32 row;
      int32 col;
      int32 ci;
      struct var_variable *var;
      struct rel_relation *rel;

      /* Record cost accounting info here. */
      ci=sys->s.block.current_block;
      sys->s.cost[ci].size=sys->s.block.current_size;
      sys->s.cost[ci].iterations=sys->s.block.iteration;
      sys->s.cost[ci].funcs=sys->s.block.funcs;
      sys->s.cost[ci].jacs=sys->s.block.jacs;
      sys->s.cost[ci].functime=sys->s.block.functime;
      sys->s.cost[ci].jactime=sys->s.block.jactime;
      sys->s.cost[ci].time=sys->s.block.cpu_elapsed;
      sys->s.cost[ci].resid=sys->s.block.residual;

#undef KAA_DEBUG
#ifdef KAA_DEBUG
      if (sys->s.block.current_size>1) {
	FPRINTF(stderr,"Block %d took %d iterations\n",
		sys->s.block.current_block,sys->s.block.iteration);
      }
#endif /* KAA_DEBUG */

      /* De-initialize previous block */
      if (sys->p.output.less_important && (sys->s.block.current_size >1 ||
          sys->iarray[SP0_LIFDS])) {
        FPRINTF(LIF(sys),"Block %d converged.\n",
	        sys->s.block.current_block);
      }
      for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ ) {
	 var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
	 var_set_in_block(var,FALSE);
      }
      for( row=sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ ) {
	 rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
	 rel_set_in_block(rel,FALSE);
      }
      sys->s.block.previous_total_size += sys->s.block.current_size;
   }

   sys->s.block.current_block += 1;
   if( sys->s.block.current_block < sys->s.block.number_of ) {
      int32 row;
      int32 col;
      boolean ok;

      /* Initialize next block */
      if( OPTIMIZING(sys) )
	 mtx_region(&(sys->J.reg), 0, sys->rank-1, 0, sys->vused-1 );
      else
	 mtx_block(sys->J.mtx,sys->s.block.current_block,&(sys->J.reg));

      row = sys->J.reg.row.high - sys->J.reg.row.low + 1;
      col = sys->J.reg.col.high - sys->J.reg.col.low + 1;
      sys->s.block.current_size = MAX(row,col);

      sys->s.block.iteration = 0;
      sys->s.block.funcs = 0;
      sys->s.block.jacs = 0;
      sys->s.block.cpu_elapsed = 0.0;
      sys->s.block.functime = 0.0;
      sys->s.block.jactime = 0.0;

/*************************************************************************/
      for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ ) {
	 struct var_variable *var;
	 var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
	 var_set_in_block(var,TRUE);
	 insure_bounds(sys,var);
      }
      for( row=sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ ) {
	 struct rel_relation *rel;
	 rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
	 rel_set_in_block(rel,TRUE);
      }
/* REORDER */
      if( sys->s.block.current_size > 1L )
	linsol_reorder(sys->J.sys,&(sys->J.reg));

      if(sys->p.output.less_important && (sys->iarray[SP0_LIFDS] ||
         sys->s.block.current_size > 1)) {
        debug_delimiter(LIF(sys));
        debug_delimiter(LIF(sys));
      }
      if(sys->p.output.less_important && sys->iarray[SP0_LIFDS]) {
        FPRINTF(LIF(sys),"\n%-40s ---> %d in [%d..%d]\n",
                "Current block number", sys->s.block.current_block,
                0, sys->s.block.number_of-1);
        FPRINTF(LIF(sys),"%-40s ---> %d\n", "Current block size",
          sys->s.block.current_size);
      }
      sys->s.calc_ok = TRUE;

      if( !(ok = calc_objective(sys)) )
	 FPRINTF(MIF(sys),"Objective calculation errors detected.\n");
      if(sys->p.output.less_important && sys->obj) {
        FPRINTF(LIF(sys),"%-40s ---> %g\n", "Objective", sys->objective);
      }
      sys->s.calc_ok = sys->s.calc_ok && ok;

      sys->residuals.accurate = FALSE;
      if( !(ok = calc_residuals(sys)) )
	 FPRINTF(MIF(sys),"Residual calculation errors detected.\n");
      if(sys->p.output.less_important && (sys->s.block.current_size >1 ||
         sys->iarray[SP0_LIFDS])) {
        FPRINTF(LIF(sys),"%-40s ---> %g\n", "Residual norm (unscaled)",
                sys->s.block.residual);
      }
      sys->s.calc_ok = sys->s.calc_ok && ok;

      /* Must be updated as soon as required */
      sys->J.accurate = FALSE;
      sys->update.weights = 0;
      sys->update.nominals = 0;
      sys->ZBZ.accurate = FALSE;
      sys->variables.accurate = FALSE;
      sys->gradient.accurate = FALSE;
      sys->multipliers.accurate = FALSE;
      sys->stationary.accurate = FALSE;
      sys->newton.accurate = FALSE;
      sys->Bnewton.accurate = FALSE;
      sys->nullspace.accurate = FALSE;
      sys->gamma.accurate = FALSE;
      sys->Jgamma.accurate = FALSE;
      sys->varstep1.accurate = FALSE;
      sys->Bvarstep1.accurate = FALSE;
      sys->varstep2.accurate = FALSE;
      sys->Bvarstep2.accurate = FALSE;
      sys->mulstep1.accurate = FALSE;
      sys->mulstep2.accurate = FALSE;
      sys->varstep.accurate = FALSE;
      sys->mulstep.accurate = FALSE;

      if( !OPTIMIZING(sys) ) {
	 sys->ZBZ.accurate = TRUE;
	 sys->gradient.accurate = TRUE;
	 sys->multipliers.accurate = TRUE;
	 sys->stationary.accurate = TRUE;
	 sys->Bnewton.accurate = TRUE;
	 sys->nullspace.accurate = TRUE;
	 sys->Bvarstep1.accurate = TRUE;
	 sys->Bvarstep2.accurate = TRUE;
      }

   } else {
      boolean ok;
      /**
       ***  Before we claim convergence, we must check if we left behind
       ***  some unassigned relations.  If and only if they happen to be
       ***  satisfied at the current point, convergence has been obtained.
       ***
       ***  Also insures that all included relations have valid residuals.
       ***  Included inequalities will have correct residuals.
       ***  Unsatisfied included inequalities cause inconsistency.
       ***
       ***  This of course ignores that fact an objective function might
       ***  be present.  Then, feasibility isn't enough, is it now.
       **/
      if( sys->s.struct_singular ) {
         /* black box w/singletons provoking bug here, maybe */
	 sys->s.block.current_size = sys->rused - sys->rank;
         if(sys->p.output.less_important) {
           debug_delimiter(LIF(sys));
           FPRINTF(LIF(sys),"%-40s ---> %d\n", "Unassigned Relations",
                   sys->s.block.current_size);
         }
	 sys->J.reg.row.low = sys->J.reg.col.low = sys->rank;
	 sys->J.reg.row.high = sys->J.reg.col.high = sys->rused - 1;
	 sys->residuals.accurate = FALSE;
	 if( !(ok=calc_residuals(sys)) )
	    FPRINTF(MIF(sys),"Residual calculation errors detected.\n");
         if(sys->p.output.less_important) {
           FPRINTF(LIF(sys),"%-40s ---> %g\n", "Residual norm (unscaled)",
                   sys->s.block.residual);
         }
	 if( block_feasible(sys) ) {
           if(sys->p.output.less_important) {
	     FPRINTF(LIF(sys),"\nUnassigned relations ok. You lucked out.\n");
           }
	   sys->s.converged = TRUE;
	 } else {
           if(sys->p.output.less_important) {
             FPRINTF(LIF(sys),"\nProblem inconsistent:  %s.\n",
                     "Unassigned relations not satisfied");
           }
           sys->s.inconsistent = TRUE;
	 }
         if(sys->p.output.less_important) {
           debug_delimiter(LIF(sys));
         }
      } else {
	 sys->s.converged = TRUE;
      }
      /* nearly done checking. Must verify included inequalities if
         we think equalities are ok. */
      if (sys->s.converged) {
         sys->s.inconsistent=(!calc_inequalities(sys));
      }
   }
}

static void find_next_unconverged_block(sys)
slv0_system_t sys;
/**
 ***  Moves to next unconverged block, assuming that the current block has
 ***  converged (or is -1, to start).
 **/
{
   do {
     move_to_next_block(sys);
   } while( !sys->s.converged && block_feasible(sys) && !OPTIMIZING(sys) );
}


/**
 ***  Iteration begin/end routines
 ***  ----------------------------
 ***     iteration_begins(sys)
 ***     iteration_ends(sys)
 **/

static void iteration_begins(sys)
slv0_system_t sys;
/**
 ***  Prepares sys for entering an iteration, increasing the iteration counts
 ***  and starting the clock.
 **/
{
   sys->clock = tm_cpu_time();
   ++(sys->s.block.iteration);
   ++(sys->s.iteration);
   if(sys->p.output.less_important&& (sys->s.block.current_size >1 ||
      sys->iarray[SP0_LIFDS])) {
     FPRINTF(LIF(sys),"\n%-40s ---> %d\n",
             "Iteration", sys->s.block.iteration);
     FPRINTF(LIF(sys),"%-40s ---> %d\n",
             "Total iteration", sys->s.iteration);
   }
}

static void iteration_ends(sys)
slv0_system_t sys;
/**
 ***  Prepares sys for exiting an iteration, stopping the clock and recording
 ***  the cpu time.
 **/
{
   double cpu_elapsed;   /* elapsed this iteration */

   cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
   sys->s.block.cpu_elapsed += cpu_elapsed;
   sys->s.cpu_elapsed += cpu_elapsed;
   if(sys->p.output.less_important && (sys->s.block.current_size >1 ||
         sys->iarray[SP0_LIFDS])) {
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Elapsed time", sys->s.block.cpu_elapsed);
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Total elapsed time", sys->s.cpu_elapsed);
   }
}

static void update_status(sys)
slv0_system_t sys;
/**
 ***  Updates the solver status.
 **/
{
   boolean unsuccessful;

   if( !sys->s.converged ) {
      sys->s.time_limit_exceeded =
	 abs(sys->s.block.cpu_elapsed >= sys->p.time_limit);
      sys->s.iteration_limit_exceeded =
	 abs(sys->s.block.iteration >= sys->p.iteration_limit);
   }

   unsuccessful = sys->s.diverged || sys->s.inconsistent ||
      sys->s.iteration_limit_exceeded || sys->s.time_limit_exceeded;

   sys->s.ready_to_solve =abs( !unsuccessful && !sys->s.converged);
   sys->s.ok = abs(!unsuccessful &&
                       sys->s.calc_ok && !sys->s.struct_singular);
}

/**
 ***  External routines
 ***  -----------------
 ***     See slv.h
 **/

static SlvClientToken slv0_create(slv_system_t server, int *statusindex)
{
  slv0_system_t sys;
  FILE *fplif, *fpmif;

  fpmif = stderr;	/* will set up a proper file handle eventually */
  fplif = fpmif;

  sys = (slv0_system_t)ascmalloc( sizeof(struct slv0_system_structure) );
  if (sys==NULL) {
      *statusindex = 1;
      return sys;
  }
  mem_zero_byte_cast(sys,0,sizeof(struct slv0_system_structure));
  sys->integrity = OK;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = NULL;
  sys->p.tolerance.pivot = 0.1;       /* scaled */
  sys->p.tolerance.singular = 1e-12;  /* scaled */
  sys->p.tolerance.feasible = 1e-8;  /* unscaled */
  sys->p.tolerance.stationary = 1e-8;   /* scaled */
  sys->p.tolerance.termination = 1e-12;  /* scaled */
  sys->p.time_limit = 1500.0;
  sys->p.iteration_limit = 100;
  sys->p.partition = TRUE;
  sys->p.ignore_bounds = FALSE;
  sys->p.whose = slv0_solver_number;
  sys->p.rho = 1.0;
  sys->p.sp.iap=&(sys->iarray[0]); /* all defaults in iarray are 0 */
  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.costsize = 0;
  sys->s.cost = NULL; /*redundant, but sanity preserving */

  return(sys);
}

static void destroy_matrices(sys)
slv0_system_t sys;
{
   if( sys->J.sys ) {
      int count = linsol_number_of_rhs(sys->J.sys)-1;
      for( ; count >= 0; count-- )
	 destroy_array(linsol_get_rhs(sys->J.sys,count));
      mtx_destroy(linsol_get_matrix(sys->J.sys));
      linsol_destroy(sys->J.sys);
      if( sys->J.relpivots ) set_destroy( sys->J.relpivots );
      if( sys->J.varpivots ) set_destroy( sys->J.varpivots );
      sys->J.sys = NULL;
   }
   if( sys->B ) {
      struct hessian_data *update;
      for( update=sys->B; update != NULL; ) {
	 struct hessian_data *handle;
	 handle = update;
	 update = update->next;
	 destroy_array(handle->y.vec);
	 destroy_array(handle->Bs.vec);
	 ascfree(handle);
      }
      sys->B = NULL;
   }
   if( sys->ZBZ.order > 0 ) {
      int i;
      for( i = 0; i < sys->ZBZ.order; i++ )
	 destroy_array(sys->ZBZ.mtx[i]);
      destroy_array(sys->ZBZ.mtx);
      destroy_array(sys->ZBZ.ZBs);
      destroy_array(sys->ZBZ.Zy);
      sys->ZBZ.order = 0;
   }
}

static void destroy_vectors(sys)
slv0_system_t sys;
{
   destroy_array(sys->nominals.vec);
   destroy_array(sys->weights.vec);
   destroy_array(sys->variables.vec);
   destroy_array(sys->residuals.vec);
   destroy_array(sys->gradient.vec);
   destroy_array(sys->multipliers.vec);
   destroy_array(sys->stationary.vec);
   destroy_array(sys->newton.vec);
   destroy_array(sys->Bnewton.vec);
   destroy_array(sys->nullspace.vec);
   destroy_array(sys->gamma.vec);
   destroy_array(sys->Jgamma.vec);
   destroy_array(sys->varstep1.vec);
   destroy_array(sys->Bvarstep1.vec);
   destroy_array(sys->varstep2.vec);
   destroy_array(sys->Bvarstep2.vec);
   destroy_array(sys->mulstep1.vec);
   destroy_array(sys->mulstep2.vec);
   destroy_array(sys->varstep.vec);
   destroy_array(sys->mulstep.vec);
}

static void slv0_closefiles(sys)
slv0_system_t sys;
{
  return;
}

static void slv0_set_var_list(slv0_system_t sys,struct var_variable **vlist)
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

static struct var_variable **slv0_get_var_list(sys)
slv0_system_t sys;
{
   check_system(sys);
   return( sys->vlist_user );
}

static void slv0_set_rel_list(slv0_system_t sys, struct rel_relation **rlist)
{
   static struct rel_relation *empty_list[] = {NULL};
   check_system(sys);
   sys->rlist_user = rlist;
   sys->rlist = (rlist==NULL ? empty_list : rlist);
   sys->s.ready_to_solve = FALSE;
}

static struct rel_relation **slv0_get_rel_list(sys)
slv0_system_t sys;
{
   check_system(sys);
   return( sys->rlist_user );
}

static int slv0_count_vars(sys,vfilter)
slv0_system_t sys;
var_filter_t *vfilter;
{
   struct var_variable **vp;
   int32 count = 0;
   check_system(sys);
   for( vp=sys->vlist; *vp != NULL; vp++ )
      if( var_apply_filter(*vp,vfilter) ) ++count;
   return( count );
}

static int slv0_count_rels(slv0_system_t sys,rel_filter_t *rfilter)
{
   struct rel_relation **rp;
   int32 count = 0;
   check_system(sys);
   for( rp=sys->rlist; *rp != NULL; rp++ )
      if( rel_apply_filter(*rp,rfilter) ) ++count;
   return( count );
}

static int slv0_eligible_solver(slv_system_t server)
{
   struct rel_relation **rp;
  for( rp=slv_get_solvers_rel_list(server); *rp != NULL ; ++rp ) {
    if( rel_less(*rp) || rel_greater(*rp) ) return(FALSE);
  }
  return(TRUE);
}

static void slv0_get_parameters(slv_system_t server, SlvClientToken asys,
                         slv_parameters_t *parameters)
{
  slv0_system_t sys;
  sys = SLV0(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

static void slv0_set_parameters(slv_system_t server, SlvClientToken asys,
                         slv_parameters_t *parameters)
{
  slv0_system_t sys;
  sys = SLV0(asys);
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

static void slv0_get_status(slv_system_t server, SlvClientToken asys,
                            slv_status_t *status)
{
  slv0_system_t sys;
  sys = SLV0(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}

static linsol_system_t slv0_get_linsol_sys(slv_system_t server,
                                           SlvClientToken asys)
{
  slv0_system_t sys;
  sys = SLV0(asys);
  if (check_system(sys)) return NULL;
  return(sys->J.sys);
}

static void sort_unassigned_rows(slv0_system_t sys) {
/**
 *** Rearranges unassigned rows of the matrix as follows
 *** (note that unassigned rows may be from unassigned relations
 *** included or unincluded or empty rows that are unused):
 *** rows included && !assigned | rows of unincluded eqns | empty rows.
 *** Empty in this context means row unassociated with eqn, not whether
 *** there is incidence in that row.
 *** (mtx_output_assign no longer puts empties up into the 0 .. rtot-1
 *** region.)
 *** Joe correctly contends that this is a solver job and not an output
 *** assign postprocessing job mtx could do.
 *** (mtx_output assign does in fact leave unassigned rows _with_incidence
 *** up against the assigned region, but rows with no incidence
 *** are mixed up with unincluded rows.
 **/
  int32 i,j,k,rank,rinc,rtot,cap;
  mtx_matrix_t mtx;
  mtx=sys->J.mtx;
  rank=sys->rank;
  rinc=sys->rused;
  rtot=sys->rtot;
  cap=sys->cap;
  k=cap-1;
  j=rtot-1;
  /* now move all included but unassigned rows up against assigned rows */
  for (i=rank; i<rinc; i++) {
    if (!rel_included(sys->rlist[mtx_row_to_org(mtx,i)]) ||
	!rel_active(sys->rlist[mtx_row_to_org(mtx,i)]) ) {
    /* if unincluded found in range we will check for convergence ...*/
      while (!rel_included(sys->rlist[mtx_row_to_org(mtx,j)]) ||
	     !rel_active(sys->rlist[mtx_row_to_org(mtx,j)]) ) j--;
      /* find last included row */
      if (j > i) mtx_swap_rows(mtx,i,j--);
      else FPRINTF(stderr,"Bug in slv0.c:sort_unassigned_rows\n");
      /* if this message is seen we would have swapped included into unincls
         an accounting error has taken place elsewhere in presolve*/
    }
  } /* all included but unassigned rows now against bottom of assigned region*/
}

static void structural_analysis(slv_system_t server, slv0_system_t sys)
/**
 ***  Performs structural analysis on the system, setting the flags in
 ***  status.  The problem must be set up, the relation/variable list
 ***  must be non-NULL, and their sizes must be in sys->s.v/r.total.  The
 ***  jacobian (linear) system must be created and have the correct order
 ***  (stored in sys->cap).  Everything else will be determined here.
 **/
{
  struct var_variable **vp;
  var_filter_t vfilter;
  struct rel_relation **rp;
  rel_filter_t rfilter;

  /**
   *** The server has marked incidenct flags already.
   **/
  /* count included equalities */
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  sys->rused = slv_count_solvers_rels(server,&rfilter);

  /* count free and incident vars */
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  sys->vused = slv_count_solvers_vars(server,&vfilter);

  /* Generate incidence matrix */
  mtx_clear(sys->J.mtx);
  for( rp = sys->rlist ; *rp != NULL ; ++rp )
    if( rel_apply_filter(*rp,&rfilter) ) {
      relman_get_incidence(*rp,&vfilter,sys->J.mtx);
  }

  /* Symbolic analysis */
  sys->rtot = slv_get_num_solvers_rels(server);
  sys->vtot = slv_get_num_solvers_vars(server);
  mtx_output_assign(sys->J.mtx,sys->rtot,sys->vtot);
  /* symbolic rank is set */
  sys->rank = mtx_symbolic_rank(sys->J.mtx);
  sys->ZBZ.order = sys->obj ? (sys->vused - sys->rank) : 0;
  /* eventually we should let another client do this */
  if( sys->p.partition && !OPTIMIZING(sys) ) {
     mtx_partition(sys->J.mtx);
  }
  sort_unassigned_rows(sys);

  /* Initialize Status */
  sys->s.over_defined = (sys->rused > sys->vused);
  sys->s.under_defined = (sys->rused < sys->vused);
  sys->s.struct_singular = (sys->rank < sys->rused);
  sys->s.block.number_of = mtx_number_of_blocks(sys->J.mtx);
}

static void create_matrices(slv_system_t server, slv0_system_t sys)
{
   sys->J.sys = linsol_create();
   sys->J.mtx = mtx_create();
   mtx_set_order(sys->J.mtx,sys->cap);
   linsol_set_matrix(sys->J.sys,sys->J.mtx);
   linsol_set_pivot_zero(sys->J.sys, sys->p.tolerance.singular);
   linsol_set_pivot_tolerance(sys->J.sys, sys->p.tolerance.pivot);
   /* rhs 0 for sys->multipliers */
   sys->J.rhs = create_array(sys->cap,real64);
   linsol_add_rhs(sys->J.sys,sys->J.rhs,TRUE);
   /* rhs 1 for sys->newton */
   sys->J.rhs = create_array(sys->cap,real64);
   linsol_add_rhs(sys->J.sys,sys->J.rhs,FALSE);
   /* rhs 2 for sys->mulstep2 */
   sys->J.rhs = create_array(sys->cap,real64);
   linsol_add_rhs(sys->J.sys,sys->J.rhs,TRUE);
   sys->J.relpivots = set_create(sys->cap);
   sys->J.varpivots = set_create(sys->cap);

   structural_analysis(server,sys);
   sys->ZBZ.mtx = create_array(sys->ZBZ.order,real64 *);
   if( sys->ZBZ.mtx ) {
      int i;
      for( i=0; i<sys->ZBZ.order; i++ )
	 sys->ZBZ.mtx[i] = create_array(sys->ZBZ.order,real64);
   }
   sys->ZBZ.ZBs = create_array(sys->ZBZ.order,real64);
   sys->ZBZ.Zy = create_array(sys->ZBZ.order,real64);
}

static void create_vectors(sys)
slv0_system_t sys;
{

   sys->nominals.vec = create_array(sys->cap,real64);
   sys->nominals.rng = &(sys->J.reg.col);
   sys->weights.vec = create_array(sys->cap,real64);
   sys->weights.rng = &(sys->J.reg.row);
   sys->variables.vec = create_array(sys->cap,real64);
   sys->variables.rng = &(sys->J.reg.col);
   sys->residuals.vec = create_array(sys->cap,real64);
   sys->residuals.rng = &(sys->J.reg.row);
   if( OPTIMIZING(sys) ) {
      sys->gradient.vec = create_array(sys->cap,real64);
      sys->gradient.rng = &(sys->J.reg.col);
      sys->multipliers.vec = create_array(sys->cap,real64);
      sys->multipliers.rng = &(sys->J.reg.row);
      sys->stationary.vec = create_array(sys->cap,real64);
      sys->stationary.rng = &(sys->J.reg.col);
   } else {
      sys->gradient.vec = NULL;
      sys->multipliers.vec = NULL;
      sys->stationary.vec = NULL;
   }
   sys->newton.vec = create_array(sys->cap,real64);
   sys->newton.rng = &(sys->J.reg.col);
   if( OPTIMIZING(sys) ) {
      sys->Bnewton.vec = create_array(sys->cap,real64);
      sys->Bnewton.rng = &(sys->J.reg.col);
      sys->nullspace.vec = create_array(sys->cap,real64);
      sys->nullspace.rng = &(sys->J.reg.col);
   } else {
      sys->Bnewton.vec = NULL;
      sys->nullspace.vec = NULL;
   }
   sys->gamma.vec = create_array(sys->cap,real64);
   sys->gamma.rng = &(sys->J.reg.col);
   sys->Jgamma.vec = create_array(sys->cap,real64);
   sys->Jgamma.rng = &(sys->J.reg.row);

   sys->varstep1.vec = create_array(sys->cap,real64);
   sys->varstep1.rng = &(sys->J.reg.col);
   if( OPTIMIZING(sys) ) {
      sys->Bvarstep1.vec = create_array(sys->cap,real64);
      sys->Bvarstep1.rng = &(sys->J.reg.col);
   } else {
      sys->Bvarstep1.vec = NULL;
   }
   sys->varstep2.vec = create_array(sys->cap,real64);
   sys->varstep2.rng = &(sys->J.reg.col);
   if( OPTIMIZING(sys) ) {
      sys->Bvarstep2.vec = create_array(sys->cap,real64);
      sys->Bvarstep2.rng = &(sys->J.reg.col);
   } else {
      sys->Bvarstep2.vec = NULL;
   }
   sys->mulstep1.vec = create_array(sys->cap,real64);
   sys->mulstep1.rng = &(sys->J.reg.row);
   sys->mulstep2.vec = create_array(sys->cap,real64);
   sys->mulstep2.rng = &(sys->J.reg.row);
   sys->varstep.vec = create_array(sys->cap,real64);
   sys->varstep.rng = &(sys->J.reg.col);
   sys->mulstep.vec = create_array(sys->cap,real64);
   sys->mulstep.rng = &(sys->J.reg.row);
}

void slv0_dump_internals(slv_system_t server, SlvClientToken sys,int level)
{
   check_system(sys);
   if (level > 0) {
      FPRINTF(stderr,"ERROR:  (slv0) slv0_dump_internals\n");
      FPRINTF(stderr,"        slv0 does not dump its internals.\n");
   }
}

void slv0_presolve(slv_system_t server, SlvClientToken asys)
{
  struct var_variable **vp;
  struct rel_relation **rp;
  int32 cap;
  slv0_system_t sys;

  sys = SLV0(asys);
  iteration_begins(sys);
  check_system(sys);
  if( sys->vlist == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv0) slv0_presolve\n");
      FPRINTF(stderr,"        Variable list was never set.\n");
      return;
  }
  if( sys->rlist == NULL ) {
      FPRINTF(stderr,"ERROR:  (slv0) slv0_presolve\n");
      FPRINTF(stderr,"        Relation list was never set.\n");
      return;
  }

  if( sys->vlist_user == NULL ) {
    Asc_Panic(2, "slv0_presolve",
              "the logic in slve presolve needs to be modified\n");
  }
  sys->cap = 0;
  for( vp=sys->vlist,cap=0 ; *vp != NULL ; ++vp ) {
      var_set_in_block(*vp,FALSE);
  }
  sys->cap = cap;
  for( rp=sys->rlist,cap=0 ; *rp != NULL ; ++rp ) {
      rel_set_in_block(*rp,FALSE);
      rel_set_satisfied(*rp,FALSE);
  }
  sys->cap = MAX(sys->cap,cap);

  destroy_matrices(sys);
  destroy_vectors(sys);
  create_matrices(server,sys);
  create_vectors(sys);

  /* Reset status */
  sys->s.iteration = 0;
  sys->s.cpu_elapsed = 0.0;
  sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
  sys->s.block.previous_total_size = 0;
  sys->s.costsize=1+sys->s.block.number_of;
  destroy_array(sys->s.cost);
  sys->s.cost=create_zero_array(sys->s.costsize,struct slv_block_cost);

  /* set to go to first unconverged block */
  sys->s.block.current_block = -1;
  sys->s.block.current_size = 0;
  sys->s.calc_ok = TRUE;
  sys->s.block.iteration = 0;
  sys->objective =  MAXDOUBLE/2000.0;
  /* trying to do it in slv_iterate
     find_next_unconverged_block(sys);
     */

  update_status(sys);
  iteration_ends(sys);
  sys->s.cost[sys->s.block.number_of].time=sys->s.cpu_elapsed;
  /* mtx_clear_region(sys->J.mtx,mtx_ENTIRE_MATRIX); doesn't seem to help*/
}

static boolean slv0_change_basis(slv0_system_t sys,int32 var, mtx_range_t *rng){
   boolean didit;
   didit=mtx_make_col_independent(sys->J.mtx,
                                  mtx_org_to_col(sys->J.mtx,var),rng);
   if (didit && sys->p.partition && !OPTIMIZING(sys)) {
     struct slv_block_cost oldpresolve;
     int32 oldblocks;
     oldblocks=sys->s.block.number_of;
     oldpresolve=sys->s.cost[sys->s.costsize-1];
     mtx_partition(sys->J.mtx);
     sys->s.block.number_of=mtx_number_of_blocks(sys->J.mtx);
     if (oldblocks!=sys->s.block.number_of) {
       ascfree(sys->s.cost);
       sys->s.costsize=sys->s.block.number_of+1;
       sys->s.cost=create_zero_array(sys->s.costsize,struct slv_block_cost);
       sys->s.cost[sys->s.costsize-1]=oldpresolve;
     }
   }
   return didit;
}

static void slv0_resolve(slv_system_t server, SlvClientToken asys)
{
   struct var_variable **vp;
   struct rel_relation **rp;
   slv0_system_t sys;
   sys = SLV0(asys);

   check_system(sys);
   for( vp = sys->vlist ; *vp != NULL ; ++vp ) {
      var_set_in_block(*vp,FALSE);
   }
   for( rp = sys->rlist ; *rp != NULL ; ++rp ) {
      rel_set_in_block(*rp,FALSE);
      rel_set_satisfied(*rp,FALSE);
   }

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
   sys->objective =  MAXDOUBLE/2000.0;

/* trying to do it in slv_iterate
   find_next_unconverged_block(sys);
*/
   update_status(sys);
}


static int directly_solve(sys,rel,var)
slv0_system_t sys;
struct rel_relation *rel;
struct var_variable *var;
/**
 ***  Attempt to directly solve the given relation (equality constraint) for
 ***  the given variable, leaving the others fixed.  Returns an integer
 ***  signifying the status as one of the following three:
 ***
 ***     0  ==>  Unable to determine anything.
 ***     1  ==>  Solution found, variable value set to this solution.
 ***    -1  ==>  No solution found.
 ***
 ***  The variable bounds will be upheld, unless they are to be ignored.
 **/
{
   boolean able;
   int nsolns;
   real64 *slist;

   slist = relman_directly_solve_new(rel,var,&able,&nsolns,
                                     sys->p.tolerance.feasible);
   if( !able ) return(0);
   while( --nsolns >= 0 ) {
     if( sys->p.ignore_bounds ) {
       var_set_value(var,slist[nsolns]);
       break;
     }
     if( var_lower_bound(var) > slist[nsolns] ) {
       real64 save = var_value(var);
       var_set_value(var,var_lower_bound(var));
       Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
       relman_eval(rel,&calc_ok);
       Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
       if( relman_calc_satisfied(rel,sys->p.tolerance.feasible) )
	 break;
       var_set_value(var,save);
     } else if( var_upper_bound(var) < slist[nsolns] ) {
       real64 save = var_value(var);
       var_set_value(var,var_upper_bound(var));
       Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
       relman_eval(rel,&calc_ok);
       Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
       if( relman_calc_satisfied(rel,sys->p.tolerance.feasible) )
	 break;
       var_set_value(var,save);
     } else {
       var_set_value(var,slist[nsolns]);
       break;
     }
   }
   /* destroy_array(slist); do not do this */
   return( nsolns >= 0 ? 1 : -1 );
}

#define MIN_COEF            0.05   /* "Largest" drop in maxstep */
#define MAX_COEF            0.95   /* "Smallest" drop in maxstep */
#define REDUCE              FALSE
#define EXACT_LINE_SEARCH   FALSE
static void slv0_iterate(slv_system_t server, SlvClientToken asys)
{
   slv0_system_t sys;
   FILE              *mif = MIF(sys);
   FILE              *lif = LIF(sys);
   real64      bounds_coef=1.0;
   real64      previous = 0.0;
   real64      oldphi, norm2;
   boolean           first=TRUE, bounds_ok=FALSE,
                     new_ok=FALSE, descent_ok=FALSE;
   int               minor = 0,ds_status=0;
   double            time0;
  sys = SLV0(asys);
  mif = MIF(sys);
  lif = LIF(sys);
  if (server == NULL || sys==NULL) return;
  if (check_system(SLV0(sys))) return;
   if( !sys->s.ready_to_solve ) {
      FPRINTF(stderr,"ERROR:  (slv0) slv0_iterate\n");
      FPRINTF(stderr,"        Not ready to solve.\n");
      return;
   }
/* trying it this way */
   if (sys->s.block.current_block==-1) {
      find_next_unconverged_block(sys);
      update_status(sys);
      return;
   }
   if (sys->p.output.less_important && (sys->s.block.current_size >1 ||
       sys->iarray[SP0_LIFDS])) {
     debug_delimiter(lif);
   }
   iteration_begins(sys);

   /**
    ***  Attempt direct solve if appropriate
    **/
   if( !OPTIMIZING(sys) && sys->s.block.iteration == 1 &&
      sys->s.block.current_size == 1 ) {
      struct var_variable *var;
      struct rel_relation *rel;
      var = sys->vlist[mtx_col_to_org(sys->J.mtx,sys->J.reg.col.low)];
      rel = sys->rlist[mtx_row_to_org(sys->J.mtx,sys->J.reg.row.low)];
      if (sys->p.output.less_important && sys->iarray[SP0_LIFDS]) {
        FPRINTF(lif,"%-40s ---> (%d)", "Singleton relation",
                mtx_row_to_org(sys->J.mtx,sys->J.reg.row.low));
        print_rel_name(lif,sys,rel); PUTC('\n',lif);
        FPRINTF(lif,"%-40s ---> (%d)", "Singleton variable",
                mtx_col_to_org(sys->J.mtx,sys->J.reg.col.low));
        print_var_name(lif,sys,var); PUTC('\n',lif);
      }

      /* Attempt direct solve */
      time0=tm_cpu_time();
      ds_status=directly_solve(sys,rel,var);
      sys->s.block.functime += (tm_cpu_time()-time0);

      switch( ds_status ) {
       case 0:
         if (sys->p.output.less_important) {
           FPRINTF(lif,"Unable to directly solve.\n");
         }
	 break;
       case 1: {
         if (sys->p.output.less_important && sys->iarray[SP0_LIFDS]) {
	   FPRINTF(lif,"Directly solved.\n");
         }
	 /* added kaa for debugging -- remove */
	 sys->s.calc_ok = calc_residuals(sys);
	 iteration_ends(sys);
	 find_next_unconverged_block(sys);
	 update_status(sys);
	 return;
       }
#undef KAA_DEBUG
       case -1:
	 sys->s.inconsistent = TRUE;
         FPRINTF(mif,"No solution exists within the bounds given for:\n");
         print_var_name(mif,sys,var); PUTC('\n',mif);
         FPRINTF(mif,"when inverting relation:\n");
         print_rel_name(mif,sys,rel); PUTC('\n',mif);
	 iteration_ends(sys);
	 update_status(sys);
	 return;
      }
   }

   if( !calc_J(sys) )
	  ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Jacobian calculation errors detected.");
      Asc_FPrintf(MIF(sys),"Jacobian calculation errors detected.\n");
   scale_J(sys);
   scale_variables(sys);
   scale_residuals(sys);
   if( !calc_gradient(sys) )
	  ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Gradient calculation errors detected.");
      Asc_FPrintf(MIF(sys),"Gradient calculation errors detected.\n");

   calc_pivots(sys);
   if (sys->iarray[SP0_SAVLIN]) {
     FILE *ldat;
     sprintf(savlinfilename,"%s%d",savlinfilebase,savlinnum++);
     ldat=fopen(savlinfilename,"w");
     FPRINTF(ldat,"================= block %d, major itn %d ============\n",
       sys->s.block.current_block, sys->s.iteration);
     mtx_write_region(ldat,sys->J.mtx,&(sys->J.reg));
     fclose(ldat);
   }
   calc_B(sys);
   calc_ZBZ(sys);
   calc_multipliers(sys);
   calc_stationary(sys);

   if( OPTIMIZING(sys) && block_feasible(sys) &&
      calc_sqrt_D0(sys->stationary.norm2) <= sys->p.tolerance.stationary ) {
      iteration_ends(sys);
      find_next_unconverged_block(sys);
      update_status(sys);
      return;
   }

   calc_phi(sys);
   if (sys->p.output.less_important) {
     FPRINTF(lif,"%-40s ---> %g\n","Phi", sys->phi);
   }

   calc_gamma(sys);
   calc_Jgamma(sys);

   if( !OPTIMIZING(sys) &&
      sys->gamma.norm2 <= sys->p.tolerance.termination*sys->phi ) {
      FPRINTF(mif,"\nProblem diverged:  Gamma norm too small.\n");
      sys->s.diverged = TRUE;
      iteration_ends(sys);
      update_status(sys);
      return;
   }

   calc_newton(sys);
   if (sys->residuals.norm2 > 1.0e-32) {
     norm2 = inner_product(&(sys->newton),&(sys->gamma))/sys->residuals.norm2;
     if( fabs(norm2 - 1.0) > 1e-4 ) {
        FPRINTF(MIF(sys),"WARNING:(slv0) slv0_iterate\n");
        FPRINTF(MIF(sys),"        Jacobian inverse inaccurate.\n");
        FPRINTF(MIF(sys),"        Smallest pivot = %g, JJ' norm2 = %g\n",
                linsol_smallest_pivot(sys->J.sys), norm2);
     }
   }
   /* if we're at the solution, who cares. in fact, why are we here? */

   calc_Bnewton(sys);
   calc_nullspace(sys);

   calc_varstep1(sys);
   calc_Bvarstep1(sys);
   calc_varstep2(sys);
   calc_Bvarstep2(sys);
   calc_mulstep1(sys);
   calc_mulstep2(sys);

   first = TRUE;
   oldphi = sys->phi;
   while (first || !bounds_ok || !new_ok || !descent_ok) {

      minor++;

      /* FIXME: fix this part as in slv3.c */
    int32 maxMinorIterations;
    maxMinorIterations = 30;
    if(minor >= maxMinorIterations){
      FPRINTF(stderr,"\nslv0: Too many minor iterations. Check variables on bounds.\n");
      sys->s.inconsistent = TRUE;
      iteration_ends(sys);
      update_status(sys);
      return;
    }

/* end of code by AWW */


      if (first) {
	 change_maxstep(sys, MAXDOUBLE);
	 first = FALSE;

      } else if (!bounds_ok) {
	 real64 maxstep_coef;
	 maxstep_coef = 0.5*(1.0 + TOWARD_BOUNDS*bounds_coef);
	 if (maxstep_coef < MIN_COEF) maxstep_coef = MIN_COEF;
	 if (maxstep_coef > MAX_COEF) maxstep_coef = MAX_COEF;
         if (sys->p.output.less_important) {
           FPRINTF(lif,"%-40s ---> %g\n",
                   "    Step reduction (bounds not ok)", maxstep_coef);
         }
	 restore_variables(sys);
	 change_maxstep(sys, maxstep_coef*sys->maxstep);

      } else if (!new_ok) {
	 real64 maxstep_coef;
	 maxstep_coef = 0.50;
         if (sys->p.output.less_important) {
           FPRINTF(lif,"%-40s ---> %g\n",
                   "    Step reduction (calculations not ok)", maxstep_coef);
         }
	 restore_variables(sys);
	 change_maxstep(sys, maxstep_coef*sys->maxstep);

      } else if (!descent_ok) {
	 real64 maxstep_coef;
	 previous = MIN(sys->phi, oldphi);
	 if( OPTIMIZING(sys) )
	    maxstep_coef = 0.5;
	 else {
	    real64 denom;
	    denom = sys->phi - oldphi +
	       sys->maxstep*calc_sqrt_D0(sys->gamma.norm2);
	    maxstep_coef = denom > 0.0 ? 0.5*
	       sys->maxstep*calc_sqrt_D0(sys->gamma.norm2)/denom : MAX_COEF;
	 }
	 if (maxstep_coef < MIN_COEF) maxstep_coef = MIN_COEF;
	 if (maxstep_coef > MAX_COEF) maxstep_coef = MAX_COEF;
         if (sys->p.output.less_important) {
           FPRINTF(lif,"%-40s ---> %g\n",
                   "    Step reduction (descent not ok)",maxstep_coef);
         }
	 sys->phi = oldphi;
	 restore_variables(sys);
	 change_maxstep(sys, maxstep_coef*sys->maxstep);
      }

      calc_step(sys, minor);
      sys->progress = calc_sqrt_D0
	 (calc_sqrt_D0((sys->varstep.norm2+sys->mulstep.norm2)*
		       (sys->varstep1.norm2+sys->mulstep1.norm2)));
      sys->maxstep = calc_sqrt_D0(sys->varstep.norm2 + sys->mulstep.norm2);
      if (sys->p.output.less_important) {
        FPRINTF(lif,"%-40s ---> %g\n",
                "    Suggested step length (scaled)", sys->maxstep);
        FPRINTF(lif,"%-40s ---> %g\n",
                "    Suggested progress", sys->progress);
      }
      if (sys->progress <= sys->p.tolerance.termination) {
	 FPRINTF(mif,"\nProblem diverged:  Suggested progress too small.\n");
	 sys->s.diverged = TRUE;
	 iteration_ends(sys);
	 update_status(sys);
	 return;
      }

      /**
       ***  Check bounds.
       **/
      bounds_ok = ((!REDUCE) || (sys->p.ignore_bounds) ||
		   ((bounds_coef=required_coef_to_stay_inbounds(sys)) == 1.0));
      if( !bounds_ok ) {
	 previous = oldphi;
	 continue;
      }

      /**
       ***  Apply step.
       **/
      apply_step(sys);
      if (sys->progress <= sys->p.tolerance.termination) {
	 FPRINTF(mif,"\nProblem diverged:  Applied progress too small.\n");
	 restore_variables(sys);
	 sys->s.diverged = TRUE;
	 iteration_ends(sys);
	 update_status(sys);
	 return;
      }

      /**
       ***  Check calculations at new point.
       **/
      new_ok = (calc_objective(sys) && calc_residuals(sys));
      if (!sys->s.calc_ok) {
         if (sys->p.output.less_important) {
           FPRINTF(lif,"    Step accepted.\n");
         }
	 if (new_ok)
	    FPRINTF(mif,"\nCalculation errors resolved.\n");
	 step_accepted(sys);
	 sys->s.calc_ok = new_ok;
	 iteration_ends(sys);
	 update_status(sys);
	 return;
      }
      if( !new_ok ) {
	 previous = oldphi;
	 continue;
      }

      /**
       ***  Check for descent.
       **/
      scale_residuals(sys);
      calc_phi(sys);
      sys->phi += inner_product( &(sys->mulstep),&(sys->residuals) );
      if (sys->p.output.less_important) {
        FPRINTF(lif,"%-40s ---> %g\n", "    Anticipated phi",sys->phi);
      }
      descent_ok = (sys->phi < oldphi);
      if (EXACT_LINE_SEARCH)
	 descent_ok = (descent_ok && (sys->phi >= previous));

   }

   step_accepted(sys);
   if (sys->p.output.less_important) {
     FPRINTF(lif,"    Step accepted.\n");
     if (sys->obj) {
       FPRINTF(lif,"\n%-40s ---> %g\n", "Objective", sys->objective);
     }
     FPRINTF(lif,"%-40s ---> %g\n", "Residual norm (unscaled)",
             sys->s.block.residual);
   }

   /**
    ***  Check for equation solving convergence within the block
    **/
   iteration_ends(sys);
   if( !OPTIMIZING(sys) && block_feasible(sys) )
      find_next_unconverged_block(sys);
   update_status(sys);
}
#undef EXACT_LINE_SEARCH
#undef REDUCE
#undef MAX_COEF
#undef MIN_COEF
#undef TOO_SMALL
#undef TOWARD_BOUNDS
#undef UPDATE_JACOBIAN
#undef UPDATE_WEIGHTS
#undef UPDATE_NOMINALS


static void slv0_solve(slv_system_t server, SlvClientToken asys)
{
  slv0_system_t sys;
  sys = SLV0(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(sys)) return;
  while( sys->s.ready_to_solve )
      slv0_iterate(server,sys);
}

static mtx_matrix_t slv0_get_jacobian(slv_system_t server, SlvClientToken sys)
{
  if (server == NULL || sys==NULL) return NULL;
  if (check_system(SLV0(sys))) return NULL;
  return SLV0(sys)->J.mtx;
}

static int slv0_destroy(slv_system_t server, SlvClientToken asys)
{
  slv0_system_t sys;
  sys = SLV0(asys);
  if (check_system(sys)) return 1;
  slv0_closefiles(sys);
  slv0_set_var_list(sys,NULL);
  slv0_set_rel_list(sys,NULL);
  destroy_matrices(sys);
  destroy_vectors(sys);
  sys->integrity = DESTROYED;
  if (sys->s.cost) ascfree(sys->s.cost);
  ascfree( (POINTER)sys );
  return 0;
}

int slv0_register(SlvFunctionsT *sft)
{
  if (sft==NULL)  {
    FPRINTF(stderr,"slv0_register called with NULL pointer\n");
    return 1;
  }

  sft->name = "Slv";
  sft->ccreate = slv0_create;
  sft->cdestroy = slv0_destroy;
  sft->celigible = slv0_eligible_solver;
  sft->getparam = slv0_get_parameters;
  sft->setparam = slv0_set_parameters;
  sft->getstatus = slv0_get_status;
  sft->solve = slv0_solve;
  sft->presolve = slv0_presolve;
  sft->iterate = slv0_iterate;
  sft->resolve = slv0_resolve;
  sft->getlinsol = slv0_get_linsol_sys;
  sft->getlinsys = NULL;
  sft->getsysmtx = slv0_get_jacobian;
  sft->dumpinternals = slv0_dump_internals;
  return 0;
}
#endif /* #else clause of DYNAMIC_SLV */
#endif /* #else clause of !STATIC_SLV && !DYNAMIC_SLV */
