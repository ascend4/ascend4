/*
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.44 $
 *  Version control file: $RCSfile: slv7.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:43 $
 *  Last modified by: $Author: ballan $
 *
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1996 Kenneth Harrison Tyner
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

#include <math.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <utilities/set.h>
#include <general/tm_time.h>
#include <utilities/mem.h>
#include <compiler/compiler.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/functype.h>
#include <compiler/func.h>
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
#include "slv7.h"
#include "slv_stdcalls.h"

#if !defined(STATIC_NGSLV) && !defined(DYNAMIC_NGSLV)
int slv7_register(SlvFunctionsT *f)
{
  (void)f;  /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"NGSlv not compiled in this ASCEND IV.\n");
  return 1;
}
#else /* either STATIC_NGSLV or DYNAMIC_NGSLV is defined */
#ifdef DYNAMIC_NGSLV
/* do dynamic loading stuff.   yeah, right */
#else /* following is used if STATIC_NGSLV is defined */

#define NGSLV 1  /* if = 1, include NGSLV functions and data structures */

#define KDEBUG TRUE
#define KILL 0
/* code that needs to be deleted compiles only with kill = 1 */
#define DEBUG FALSE
#define TERMSCALE FALSE
/* if TERMSCALE TRUE try to use rel_nominals */

#define D_ZERO (double)0.0
#define SLV7(s) ((slv7_system_t)(s))
#define SERVER (sys->slv)

#define slv7_IA_SIZE 13
#define slv7_RA_SIZE 11
#define slv7_CA_SIZE 0
#define slv7_VA_SIZE 0
/* subscripts for iarray */
#define SP7_LIFDS  0
#define SP7_SAVLIN 1
#define SP7_RELNOM 2
#define SP7_CUTOFF 3
#define SP7_UPJAC  4
#define SP7_UPWTS  5
#define SP7_UPNOM  6
#define SP7_REDUCE 7
#define SP7_EXACT  8
#define SP7_CNCOLS 9
#define SP7_BTRUNC 10
#define SP7_ORDERM 11
#define SP7_SAFECALC 12
static char *slv7_ianames[slv7_IA_SIZE] =
{"lifds","savlin","relnom","cutoff","upjac","upwts",
 "upnom","reduce","exact","cncols","btrunc","reorder"
 "safe_calc"};
static char *slv7_iaexpln[slv7_IA_SIZE] = {
"If lifds != 0 and showlessimportant is TRUE, show direct solve details",
"If savlin != 0, write out matrix data file at each iteration to SlvLinsol.dat",
"If relnom != 0, use relation nominals to scale the jacobian, else row 2 norm",
"Cutoff is the block size cutoff for MODEL-based reordering of partitions",
"Update jacobian every this many major iterations",
"Update row scalings every this many major iterations",
"Update column scalings every this many major iterations",
"Require misunderstood reduction somewhere in the stepping algorithm",
"Require residual >= some other number in the stepping algorithm",
"Check jacobian for poorly scaled columns and whine if found",
"Truncate whole step vector rather than componentwise at variable bound",
"Reorder option. 0 = MODEL based, 1 = MODEL based2, 2 = simple spk1"
"Use safe calculation routines"
};
#define UPDATE_JACOBIAN   (sys->iarray[SP7_UPJAC])
#define UPDATE_WEIGHTS    (sys->iarray[SP7_UPWTS])
#define UPDATE_NOMINALS   (sys->iarray[SP7_UPNOM])
#define REDUCE            (sys->iarray[SP7_REDUCE])
#define EXACT_LINE_SEARCH (sys->iarray[SP7_EXACT])
#define DUMPCNORM 	  (sys->iarray[SP7_CNCOLS])
#define TRUNCATE    	  (sys->iarray[SP7_BTRUNC])
#define SAFE_CALC         (sys->iarray[SP7_SAFECALC])

/* subscripts for rarray */
#define SP7_2SMALL	0
#define SP7_CNLOW	1
#define SP7_CNHIGH	2
#define SP7_TBNDS	3
#define SP7_POSDEF	4
#define SP7_DET0	5
#define SP7_SSERRM	6
#define SP7_PRMAX	7
#define SP7_MINCO	8
#define SP7_MAXCO	9
#define SP7_GMULT	10
static char *slv7_ranames[slv7_RA_SIZE] =
{"toosmall","cnlow","cnhigh","tobnds","posdef","detzero",
 "steperrmax","prngmin","mincoef","maxcoef","gradmult"};
static char *slv7_raexpln[slv7_RA_SIZE] = {
"Var nominal to use if user specifies 0.0",
"Smallest column norm we won't complain about if checking",
"Largest column norm we won't complain about if checking",
"If bound is in the way, we go this fraction toward it",
"Hessian fudge number when optimizing",
"Minimum 2x2 determinant of newton/gradient we consider non-parallel",
"Step size must be determined this precisely, or prngmin happy",
"Parameter range must be this narrow to exit inner loop if step size unhappy",
"'Largest' drop in maxstep allowed",
"'Smallest' drop in maxstep allowed",
"Multiplier for gradient step in unpivoted region"};

#define TOO_SMALL		(sys->rarray[SP7_2SMALL])
#define CNLOW			(sys->rarray[SP7_CNLOW])
#define CNHIGH			(sys->rarray[SP7_CNHIGH])
#define TOWARD_BOUNDS		(sys->rarray[SP7_TBNDS])
#define POSITIVE_DEFINITE	(sys->rarray[SP7_POSDEF])
#define DETZERO			(sys->rarray[SP7_DET0])
#define STEPSIZEERR_MAX		(sys->rarray[SP7_SSERRM])
#define PARMRNG_MIN		(sys->rarray[SP7_PRMAX])
#define MIN_COEF		(sys->rarray[SP7_MINCO])
#define MAX_COEF		(sys->rarray[SP7_MAXCO])
#define GRAD_MULT		(sys->rarray[SP7_GMULT])

/*********************************************************************\
 Subparameters implemented:   (value/meaning)
 sp.ia[SP7_LIFDS]  0=>do not show full detail info for singletons
                   1=>do (this value ignored if detailed solve info not on.
 sp.ia[SP7_SAVLIN] 0=>do not append linearizations arising in the newton
                      scheme to the file SlvLinsol.dat.
                   1=>do.
 sp.ia[SP7_RELNOM] 0=>use row 2 norm in calc_weights.
                   1=>use rel_nominal in calc_weights.
 sp.ia[SP7_CUTOFF] MODEL tearing/reordering cutoff number.

 sp.rarray[*] 	Generally cryptic parameters left by Joe. Someone
		should play with and document them. See the defaults.

 if TERMSCALE FALSE, SP7_RELNOM is ignored.
\*********************************************************************/
struct update_data {
  int                    jacobian;     /* Countdown on jacobian updating */
  int                    weights;      /* Countdown on weights updating */
  int                    nominals;     /* Countdown on nominals updating */
};

/* varpivots, relpivots used only in optimizing, if we rewrite calc_pivots
without them. */
struct jacobian_data {
  linsolqr_system_t      sys;          /* Linear system */
  mtx_matrix_t           mtx;          /* Transpose gradient of residuals */
  real64           *rhs;         /* RHS from linear system */
  unsigned               *varpivots;   /* Pivoted variables */
  unsigned               *relpivots;   /* Pivoted relations */
  unsigned               *subregions;  /* Set of subregion indeces */
  dof_t                  *dofdata;     /* dof data pointer from server */
  mtx_region_t           reg;          /* Current block region */
  int32            rank;         /* Numerical rank of the jacobian */
  enum factor_method     fm;		/* Linear factorization method */
  boolean                accurate;     /* ? Recalculate matrix */
  boolean                singular;     /* ? Can matrix be inverted */
  boolean                old_partition;     /* old value of partition flag */
#if NGSLV
  int32                  rank_defect;  /* Numerical rank defect of the jacobian*/
  mtx_sparse_t           *singcols;
  mtx_sparse_t           *pivrows;
  mtx_sparse_t           *singrows;
  mtx_sparse_t           *pivcols;
  mtx_region_t           un_p_col_reg;    /* Unpivoted col region of current block */
  mtx_region_t           un_p_row_reg;    /* Unpivoted row region of current block */
  mtx_region_t           A12_reg;         /* A12 region */
  mtx_region_t           A21_reg;         /* A21 region */
  mtx_region_t           A22_reg;         /* A22 region */
#endif /*NGSLV*/
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

/**
 *** line search data for ngslv
 **/
struct linesearch_data {
  real64           obj;           /* objective function value */
  real64           obj2;           /* 2nd (newt) objective function value */
  real64           grad;          /* objective function gradient */
  real64           newton_mult;   /* scaling factor for newton step (0-1) */
  real64           full_grad_mult;/* calculated gradient multiplier */
  real64           grad_mult;     /* additional scaling for grad step (0-1) */
  real64           p_error;
  real64           un_p_error;
  int32            rank;
  int32            rank_defect;
  real64           newton_norm;
  real64           grad_norm;
};

struct slv7_system_structure {

  /**
   ***  Problem definition
   **/
  slv_system_t   	        slv;   /* slv_system_t back-link */
  struct rel_relation         *obj;    /* Objective function: NULL = none */
  struct var_variable         **vlist; /* Variable list (NULL terminated) */
  struct rel_relation         **rlist; /* Relation list (NULL terminated) */

  /**
   ***  Solver information
   **/
  int                    integrity;    /* ? Has the system been created */
  int32                  presolved;    /* ? Has the system been presolved */
  slv_parameters_t       p;            /* Parameters */
  slv_status_t           s;            /* Status (as of iteration end) */
  struct update_data     update;       /* Jacobian frequency counters */
  int32            cap;          /* Order of matrix/vectors */
  int32            rank;         /* Symbolic rank of problem */
  int32            vused;        /* Free and incident variables */
  int32            vtot;         /* length of varlist */
  int32            rused;        /* Included relations */
  int32            rtot;         /* length of varlist */
  double                 clock;        /* CPU time */
  double rarray[slv7_RA_SIZE];
  int iarray[slv7_IA_SIZE];

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
#if NGSLV
  struct vec_vector     grad_newton;  /* newton grad vec KHACK */
  struct vec_vector     grad_newton2;  /* 2nd newton grad vec KHACK */
  struct vec_vector     un_p_grad;    /* Gradient for unpivoted variables */
  struct vec_vector     tmp;          /* tmp vector for backsolve on U */
  struct vec_vector     tmp2;          /* tmp vector for grad mult calc */
  struct vec_vector     tmp_ls;        /* tmp vector for line search */
  struct linesearch_data  line_search;   /* data for ngslv line search */
#endif /*NGSLV*/
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

#define OK        ((int)813029392)
#define DESTROYED ((int)103289182)
static int check_system(slv7_system_t sys)
/**
 ***  Checks sys for NULL and for integrity.
 **/
{
  if( sys == NULL ) {
    FPRINTF(stderr,"ERROR:  (slv7) check_system\n");
    FPRINTF(stderr,"        NULL system handle.\n");
    return 1;
  }

  switch( sys->integrity ) {
  case OK:
    return 0;
  case DESTROYED:
    FPRINTF(stderr,"ERROR:  (slv7) check_system\n");
    FPRINTF(stderr,"        System was recently destroyed.\n");
    return 1;
  default:
    FPRINTF(stderr,"ERROR:  (slv7) check_system\n");
    FPRINTF(stderr,"        System reused or never allocated.\n");
    return 1;
  }
}

/**
 ***  General input/output routines
 ***  -----------------------------
 ***     print_var_name(out,sys,var)
 ***     print_rel_name(out,sys,rel)
 **/

#define print_var_name(a,b,c) slv_print_var_name((a),(b)->slv,(c))
#define print_rel_name(a,b,c) slv_print_rel_name((a),(b)->slv,(c))

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

static void debug_delimiter( FILE *fp)
/**
 ***  Outputs a hyphenated line.
 **/
{
  int i;
  for( i=0; i<60; i++ ) PUTC('-',fp);
  PUTC('\n',fp);
}

#if DEBUG
static void debug_out_vector( FILE *fp, slv7_system_t sys,
                              struct vec_vector *vec)
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

static void debug_out_var_values( FILE *fp, slv7_system_t sys)
/**
 ***  Outputs all variable values in current block.
 **/
{
  int32 col;
  struct var_variable *var;

  FPRINTF(fp,"Var values --> \n");
  for( col = sys->J.reg.col.low; col <= sys->J.reg.col.high ; col++ ) {
    var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
    print_var_name(fp,sys,var);
    FPRINTF(fp, "\nI	Lb	Value	Ub	Scale	Col	INom\n");
    FPRINTF(fp,"%d\t%.4g\t%.4g\t%.4g\t%.4g\t%d\t%.4g\n",
      var_sindex(var),var_lower_bound(var),var_value(var),
      var_upper_bound(var),var_nominal(var),
      col,sys->nominals.vec[col]);
  }
}

static void debug_out_rel_residuals( FILE *fp, slv7_system_t sys)
/**
 ***  Outputs all relation residuals in current block.
 **/
{
  int32 row;

  FPRINTF(fp,"Rel residuals --> \n");
  for( row = sys->J.reg.row.low; row <= sys->J.reg.row.high ; row++ ) {
    struct rel_relation *rel;
    rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
    FPRINTF(fp,"  %g : ",rel_residual(rel));
    print_rel_name(fp,sys,rel);
    PUTC('\n',fp);
  }
  PUTC('\n',fp);
}

static void debug_out_jacobian( FILE *fp, slv7_system_t sys)
/**
 ***  Outputs permutation and values of the nonzero elements in the
 ***  the jacobian matrix.
 **/
{
  mtx_coord_t nz;
  real64 value;

  nz.row = sys->J.reg.row.low;
  for( ; nz.row <= sys->J.reg.row.high; ++(nz.row) ) {
    FPRINTF(fp,"   Row %d (rel %d)\n", nz.row,
      mtx_row_to_org(sys->J.mtx,nz.row));
    nz.col = mtx_FIRST;
    while( value = mtx_next_in_row(sys->J.mtx,&nz,&(sys->J.reg.col)),
           nz.col != mtx_LAST ) {
      FPRINTF(fp,"      Col %d (var %d) has value %g\n", nz.col,
        mtx_col_to_org(sys->J.mtx,nz.col), value);
    }
  }
}

static void debug_out_hessian( FILE *fp, slv7_system_t sys)
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
    for( nz.col = 0; nz.col < sys->ZBZ.order; nz.col++ ) {
      FPRINTF(fp,"%10g ",sys->ZBZ.mtx[nz.row][nz.col]);
    }
      PUTC('\n',fp);
  }
}

#endif

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
   ((len) > 0 ? (type *)ascmalloc((len)*sizeof(type)) : NULL)
#define create_zero_array(len,type)  \
   ((len) > 0 ? (type *)asccalloc((len),sizeof(type)) : NULL)
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
 ***     ok = calc_boundaries(sys)
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


#define OPTIMIZING(sys)     ((sys)->ZBZ.order > 0)

static boolean calc_objective( slv7_system_t sys)
/**
 ***  Evaluate the objective function.
 **/
{
  calc_ok = TRUE;
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
  sys->objective = (sys->obj ? relman_eval(sys->obj,&calc_ok,SAFE_CALC) : 0.0);
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  return calc_ok;
}


static boolean calc_inequalities( slv7_system_t sys)
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
      relman_eval(*rp,&calc_ok,SAFE_CALC);
      satisfied= satisfied &&
#if TERMSCALE
        relman_calc_satisfied_scaled(*rp,sys->p.tolerance.feasible);
#else
        relman_calc_satisfied(*rp,sys->p.tolerance.feasible);
#endif
    }
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  return (calc_ok && satisfied);
}

static boolean calc_residuals( slv7_system_t sys)
/**
 ***  Calculates all of the residuals in the current block and computes
 ***  the residual norm for block status.  Returns true iff calculations
 ***  preceded without error.
 **/
{
  int32 row;
  struct rel_relation *rel;
  double time0;

  if( sys->residuals.accurate ) return TRUE;

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
    sys->residuals.vec[row] = relman_eval(rel,&calc_ok,SAFE_CALC);
#if TERMSCALE
    relman_calc_satisfied_scaled(rel,sys->p.tolerance.feasible);
#else
    relman_calc_satisfied(rel,sys->p.tolerance.feasible);
#endif
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  sys->s.block.functime += (tm_cpu_time() -time0);
  sys->s.block.funcs++;
  square_norm( &(sys->residuals) );
  sys->s.block.residual = calc_sqrt_D0(sys->residuals.norm2);
  return(calc_ok);
}


static boolean calc_J( slv7_system_t sys)
/**
 ***  Calculates the current block of the jacobian.
 ***  It is initially unscaled.
 **/
{
  int32 row;
  var_filter_t vfilter;
  double time0;
  real64 resid;

  if( sys->J.accurate )
    return TRUE;

  calc_ok = TRUE;
  vfilter.matchbits = (VAR_INBLOCK | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INBLOCK | VAR_ACTIVE);
  time0=tm_cpu_time();
  mtx_clear_region(sys->J.mtx,&(sys->J.reg));
  for( row = sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ ) {
    struct rel_relation *rel;
    rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
    relman_diffs(rel,&vfilter,sys->J.mtx,&resid,SAFE_CALC);
  }
  sys->s.block.jactime += (tm_cpu_time() - time0);
  sys->s.block.jacs++;

  if( --(sys->update.nominals) <= 0 ) sys->nominals.accurate = FALSE;
  if( --(sys->update.weights) <= 0 ) sys->weights.accurate = FALSE;

  linsolqr_matrix_was_changed(sys->J.sys);
  return(calc_ok);
}


static void calc_nominals( slv7_system_t sys)
/**
 ***  Retrieves the nominal values of all of the block variables,
 ***  insuring that they are all strictly positive.
 **/
{
  int32 col;
  FILE *fp = MIF(sys);
  if( sys->nominals.accurate ) return;
  fp = MIF(sys);
  col = sys->nominals.rng->low;
  for( ; col <= sys->nominals.rng->high; col++ ) {
    struct var_variable *var;
    real64 n;
    var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
    n = var_nominal(var);
    if( n <= 0.0 ) {
      if( n == 0.0 ) {
        n = TOO_SMALL;
        FPRINTF(fp,"ERROR:  (slv7) calc_nominals\n");
        FPRINTF(fp,"        Variable ");
        print_var_name(fp,sys,var);
        FPRINTF(fp,"        \nhas nominal value of zero.\n");
        FPRINTF(fp,"        Resetting to %g.\n",n);
        var_set_nominal(var,n);
      } else {
        n =  -n;
        FPRINTF(fp,"ERROR:  (slv7) calc_nominals\n");
        FPRINTF(fp,"        Variable ");
        print_var_name(fp,sys,var);
        FPRINTF(fp,"        \nhas negative nominal value.\n");
        FPRINTF(fp,"        Resetting to %g.\n",n);
        var_set_nominal(var,n);
      }
    }
#if DEBUG
    FPRINTF(fp,"Column %d is",col);
    print_var_name(fp,sys,var);
    FPRINTF(fp,"\nScaling of column %d is %g\n",col,n);
#endif
    sys->nominals.vec[col] = n;
  }
  square_norm( &(sys->nominals) );
  sys->update.nominals = UPDATE_NOMINALS;
  sys->nominals.accurate = TRUE;
}

#if TERMSCALE
/* rel_nominal scaling of rows supported as an alternative if
   TERMSCALE true. scales by 2 norm if SP7_RELNOM==0 */
static void calc_weights( slv7_system_t sys)
/**
 ***  Calculates the weights of all of the block relations
 ***  to scale the rows of the Jacobian.
 ***  Switch between interface and 2norm weight by SP7_RELNOM.
 **/
{
  mtx_coord_t nz;

  if( sys->weights.accurate )
    return;

  nz.row = sys->weights.rng->low;
  if (!(sys->iarray[SP7_RELNOM])) {
    for( ; nz.row <= sys->weights.rng->high; (nz.row)++ ) {
      real64 sum;
      sum=mtx_sum_sqrs_in_row(sys->J.mtx,nz.row,&(sys->J.reg.col));
      sys->weights.vec[nz.row] = (sum>0.0) ? 1.0/calc_sqrt_D0(sum) : 1.0;
    }
  } else {
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
#else
/* 2 norm scaling of rows */

static void calc_weights( slv7_system_t sys)
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
/* end of termscale alternative calcweights */
#endif

static void scale_J( slv7_system_t sys)
/**
 ***  Scales the jacobian.
 **/
{
  int32 row;
  int32 col;

  if( sys->J.accurate ) return;

  calc_nominals(sys);
  for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ )
    mtx_mult_col(sys->J.mtx,col,sys->nominals.vec[col],&(sys->J.reg.row));

  calc_weights(sys);
  for( row=sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ )
     mtx_mult_row(sys->J.mtx,row,sys->weights.vec[row],&(sys->J.reg.col));

  if (DUMPCNORM) {
    for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ ) {
      real64 cnorm;
      cnorm =
        calc_sqrt_D0(mtx_sum_sqrs_in_col(sys->J.mtx,col,&(sys->J.reg.row)));
      if (cnorm >CNHIGH || cnorm <CNLOW) {
        FPRINTF(stderr,"[col %d org %d] %g\n", col,
          mtx_col_to_org(sys->J.mtx,col), cnorm);
      }
    }
  }

  sys->update.jacobian = UPDATE_JACOBIAN;
  sys->J.accurate = TRUE;
  sys->J.singular = FALSE;  /* yet to be determined */
#if DEBUG
  FPRINTF(LIF(sys),"\nJacobian: \n");
  debug_out_jacobian(LIF(sys),sys);
#endif
}


static void scale_variables( slv7_system_t sys)
{
  int32 col;

  if( sys->variables.accurate ) return;

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


static void scale_residuals( slv7_system_t sys)
/**
 ***  Scales the previously calculated residuals.
 **/
{
  int32 row;

  if( sys->residuals.accurate ) return;

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

static boolean calc_gradient(slv7_system_t sys)
/**
 ***  Calculate scaled gradient of the objective function.
 **/
{
/*
 *
 * This entire function needs to be reimplemented with relman_diffs.
 *
 */
  if( sys->gradient.accurate ) return TRUE;

  calc_ok = TRUE;
  if ( !OPTIMIZING(sys) ) {
    zero_vector(&(sys->gradient));
    sys->gradient.norm2 = 0.0;
  } else {
    real64 pd;
    const struct var_variable **vp;
    var_filter_t vfilter;
    vfilter.matchbits = (VAR_INBLOCK | VAR_SVAR | VAR_ACTIVE);
    vfilter.matchvalue = (VAR_INBLOCK | VAR_SVAR | VAR_ACTIVE);
    zero_vector(&(sys->gradient));
    ASC_PANIC("abort");
    /* the next line will core dump anyway since vp not null-terminated*/
    for( vp = rel_incidence_list(sys->obj) ; *vp != NULL ; ++vp ) {
      int32 col;
      col = mtx_org_to_col(sys->J.mtx,var_sindex(*vp));
      if( var_apply_filter(*vp,&vfilter) ) {
        relman_diff(sys->obj,*vp,&pd,SAFE_CALC);
        sys->gradient.vec[col] = sys->nominals.vec[col]*pd;
      }
    }
    square_norm( &(sys->gradient) );
  }
  sys->gradient.accurate = TRUE;
#if DEBUG
  FPRINTF(LIF(sys),"Gradient:  ");
  debug_out_vector(LIF(sys),sys,&(sys->gradient));
#endif
  return calc_ok;
}

static void create_update( slv7_system_t sys)
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


static void calc_B( slv7_system_t sys)
/**
 ***  Computes a rank 2 BFGS update to the hessian matrix
 ***  B which accumulates curvature.
 **/
{
  if( sys->s.block.iteration > 1 ) {
    create_update(sys);
  } else {
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
      for( ; col <= sys->B->y.rng->high; col++ ) {
          sys->B->y.vec[col] += sys->gradient.vec[col] -
             sys->stationary.vec[col];
      }
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

    if( sys->B->ys == 0.0 && sys->B->sBs == 0.0 ) {
      theta = 0.0;
    } else {
      theta = sys->B->ys < POSITIVE_DEFINITE*sys->B->sBs ?
       (1.0-POSITIVE_DEFINITE)*sys->B->sBs/(sys->B->sBs - sys->B->ys):1.0;
    }
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


static int calc_pivots(slv7_system_t sys)
/**
 ***  Obtain the equations and variables which
 ***  are able to be pivoted.
 ***  return value is the row rank deficiency, which we hope is 0.
 **/
{
  int row_rank_defect=0;
  linsolqr_system_t lsys = sys->J.sys;

  linsolqr_factor(lsys,sys->J.fm);        	/* factor */

  if (OPTIMIZING(sys)) {
    /* need things for nullspace move. don't care about
     * dependency coefficiency in any circumstances at present.
     */
    linsolqr_calc_col_dependencies(lsys);
    set_null(sys->J.relpivots,sys->cap);
    set_null(sys->J.varpivots,sys->cap);
    linsolqr_get_pivot_sets(lsys,sys->J.relpivots,sys->J.varpivots);
  }

  sys->J.rank = linsolqr_rank(lsys);
  sys->J.singular = FALSE;
  row_rank_defect = sys->J.reg.row.high -
    sys->J.reg.row.low+1 - sys->J.rank;
  if( row_rank_defect > 0 ) {
    sys->J.singular = TRUE;
  }
#if KDEBUG
  sys->J.pivrows = linsolqr_pivoted_rows(lsys);
  sys->J.singrows = linsolqr_unpivoted_rows(lsys);
#endif /* KDEBUG */
  return row_rank_defect;
}

static void zero_un_p_weights(slv7_system_t sys)
/* This function used to be part of calc_pivots but
   it seemed to be a severe side effect and has been
   moved */
{
  FILE *fp = LIF(sys);

  if( sys->J.rank_defect > 0 ) {
    linsolqr_system_t lsys = sys->J.sys;
    int32 row,krow;
    mtx_sparse_t *uprows=NULL;
    sys->J.singular = TRUE;
    uprows = linsolqr_unpivoted_rows(lsys);
    if (uprows !=NULL) {
      for( krow=0; krow < uprows->len ; krow++ ) {
        int32 org_row;
        struct rel_relation *rel;

        org_row = uprows->idata[krow];
        row = mtx_org_to_row(sys->J.mtx,org_row);
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
      mtx_destroy_sparse(uprows);
    }
    if( !sys->residuals.accurate ) {
      square_norm( &(sys->residuals) );
      sys->residuals.accurate = TRUE;
      sys->update.weights = 0;  /* re-compute weights next iteration. */
    }
  }
  if( sys->J.rank < sys->J.reg.col.high-sys->J.reg.col.low+1 ) {
    int32 col,kcol;
    mtx_sparse_t *upcols=NULL;
    if (NOTNULL(upcols)) {
      for( kcol=0; upcols != NULL && kcol < upcols->len ; kcol++ ) {
        int32 org_col;
        struct var_variable *var;

        org_col = upcols->idata[kcol];
        col = mtx_org_to_col(sys->J.mtx,org_col);
        var = sys->vlist[org_col];
        FPRINTF(fp,"%-40s ---> ","Variable not pivoted");
        print_var_name(fp,sys,var);
        PUTC('\n',fp);
        /**
         ***  If we're not optimizing (everything should be
         ***  pivotable) or this was one of the dependent variables,
         ***  consider this variable as if it were fixed.
         **/
        if( col <= sys->J.reg.col.high - sys->ZBZ.order ) {
          mtx_mult_col(sys->J.mtx,col,0.0,&(sys->J.reg.row));
        }
      }
      mtx_destroy_sparse(upcols);
    }
  }
  if (sys->p.output.less_important) {
    FPRINTF(LIF(sys),"%-40s ---> %d (%s)\n","Jacobian rank", sys->J.rank,
            sys->J.singular ? "deficient":"full");
    FPRINTF(LIF(sys),"%-40s ---> %g\n","Smallest pivot",
            linsolqr_smallest_pivot(sys->J.sys));
  }
}

static void calc_ZBZ(slv7_system_t sys)
/**
 ***  Updates the reduced hessian matrix.
 ***  if !OPTIMIZING just sets zbz.accurate true and returns.
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
                  (-linsolqr_org_col_dependency(sys->J.sys,depr,ind))*
                     (-linsolqr_org_col_dependency(sys->J.sys,depc,ind));
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
                     (-linsolqr_org_col_dependency(sys->J.sys,dep,ind));
                  sys->ZBZ.ZBs[nz.row] += update->Bs.vec[col]*
                     (-linsolqr_org_col_dependency(sys->J.sys,dep,ind));
               }
            }
            for( nz.col=0; nz.col <= nz.row; nz.col++ ) {
               sys->ZBZ.mtx[nz.row][nz.col] += update->ys > 0.0 ?
                  sys->ZBZ.Zy[nz.row]*sys->ZBZ.Zy[nz.col]/update->ys : 0.0;
               sys->ZBZ.mtx[nz.row][nz.col] -= update->sBs > 0.0 ?
                  sys->ZBZ.ZBs[nz.row]*sys->ZBZ.ZBs[nz.col]/update->sBs : 0.0;
               if( nz.row != nz.col ) {
                  sys->ZBZ.mtx[nz.col][nz.row] =
                     sys->ZBZ.mtx[nz.row][nz.col];
               }
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


static void calc_rhs(slv7_system_t sys, struct vec_vector *vec,
                     real64 scalar, boolean transpose)
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
   linsolqr_rhs_was_changed(sys->J.sys,sys->J.rhs);
}


static void calc_multipliers(slv7_system_t sys)
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
      linsolqr_system_t lsys = sys->J.sys;
      int32 row;
      sys->J.rhs = linsolqr_get_rhs(lsys,0);
      mem_zero_byte_cast(sys->J.rhs,0.0,sys->cap*sizeof(real64));
      calc_rhs(sys, &(sys->gradient), -1.0, TRUE );
      linsolqr_solve(lsys,sys->J.rhs);
      row = sys->multipliers.rng->low;
      for( ; row <= sys->multipliers.rng->high; row++ ) {
         struct rel_relation *rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
         sys->multipliers.vec[row] = linsolqr_var_value
            (lsys,sys->J.rhs,mtx_row_to_org(sys->J.mtx,row));
         rel_set_multiplier(rel,sys->multipliers.vec[row]*
                            sys->weights.vec[row]);

      }
      if (sys->iarray[SP7_SAVLIN]) {
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
          FPRINTF(ldat,"%.20g\n",linsolqr_var_value(lsys,sys->J.rhs,ov));
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


static void calc_stationary( slv7_system_t sys)
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


static void calc_gamma( slv7_system_t sys)
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

static void calc_Jgamma( slv7_system_t sys)
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


static void calc_newton( slv7_system_t sys)
/**
 ***  Computes a step to solve the linearized equations.
 **/
{
   linsolqr_system_t lsys = sys->J.sys;
   int32 col;

   if( sys->newton.accurate )
      return;

   sys->J.rhs = linsolqr_get_rhs(lsys,1);
   mem_zero_byte_cast(sys->J.rhs,0.0,sys->cap*sizeof(real64));
   calc_rhs(sys, &(sys->residuals), -1.0, FALSE);
   linsolqr_solve(lsys,sys->J.rhs);
   col = sys->newton.rng->low;
   for( ; col <= sys->newton.rng->high; col++ )
      sys->newton.vec[col] = linsolqr_var_value
         (lsys,sys->J.rhs,mtx_col_to_org(sys->J.mtx,col));
   if (sys->iarray[SP7_SAVLIN]) {
     FILE *ldat;
     int32 ov;
     sprintf(savlinfilename,"%s%d",savlinfilebase,savlinnum++);
     ldat=fopen(savlinfilename,"w");
     FPRINTF(ldat,"================= resids (orgrowed) itn %d =====\n",
             sys->s.iteration);
     debug_write_array(ldat,sys->J.rhs,sys->cap);
     FPRINTF(ldat,"================= vars (orgcoled) ============\n");
     for(ov=0 ; ov < sys->cap; ov++ )
       FPRINTF(ldat,"%.20g\n",linsolqr_var_value(lsys,sys->J.rhs,ov));
     fclose(ldat);
   }
   square_norm( &(sys->newton) );
   sys->newton.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Newton:  ");
   debug_out_vector(LIF(sys),sys,&(sys->newton));
#endif
}

static void ngslv_calc_newton( slv7_system_t sys,mtx_matrix_t *factors)
/**
 ***  Computes a step to solve the linearized equations.
 ***  Stores in sys->grad_newton vector in cur col order
 ***  wrt factors.
 **/
{
   linsolqr_system_t lsys = sys->J.sys;
   int32 col;

   if( sys->newton.accurate )
      return;

   sys->J.rhs = linsolqr_get_rhs(lsys,1);
   mem_zero_byte_cast(sys->J.rhs,0.0,sys->cap*sizeof(real64));
   calc_rhs(sys, &(sys->residuals), -1.0, FALSE);
   linsolqr_solve(lsys,sys->J.rhs);
   col = sys->newton.rng->low;
   for( ; col <= sys->newton.rng->high; col++ )
      sys->grad_newton.vec[col] = linsolqr_var_value
         (lsys,sys->J.rhs,mtx_col_to_org(*factors,col));
   if (sys->iarray[SP7_SAVLIN]) {
     FILE *ldat;
     int32 ov;
     sprintf(savlinfilename,"%s%d",savlinfilebase,savlinnum++);
     ldat=fopen(savlinfilename,"w");
     FPRINTF(ldat,"================= resids (orgrowed) itn %d =====\n",
             sys->s.iteration);
     debug_write_array(ldat,sys->J.rhs,sys->cap);
     FPRINTF(ldat,"================= vars (orgcoled) ============\n");
     for(ov=0 ; ov < sys->cap; ov++ )
       FPRINTF(ldat,"%.20g\n",linsolqr_var_value(lsys,sys->J.rhs,ov));
     fclose(ldat);
   }
   square_norm( &(sys->newton) );
   sys->newton.accurate = TRUE;
#if DEBUG
   FPRINTF(LIF(sys),"Newton:  ");
   debug_out_vector(LIF(sys),sys,&(sys->newton));
#endif
}

static void calc_Bnewton( slv7_system_t sys)
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


static void calc_nullspace( slv7_system_t sys)
/**
 ***  Calculate the nullspace move if OPTIMIZING.
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
                     (-linsolqr_org_col_dependency(sys->J.sys,dep,ind));
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

static void calc_varstep1( slv7_system_t sys)
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


static void calc_Bvarstep1( slv7_system_t sys)
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


static void calc_varstep2( slv7_system_t sys)
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
                  (-linsolqr_org_col_dependency(sys->J.sys,dep,ind));
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


static void calc_Bvarstep2( slv7_system_t sys)
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


static void calc_mulstep1( slv7_system_t sys)
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


static void calc_mulstep2( slv7_system_t sys)
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
      linsolqr_system_t lsys = sys->J.sys;
      int32 row;
      sys->J.rhs = linsolqr_get_rhs(lsys,2);
      mem_zero_byte_cast(sys->J.rhs,0.0,sys->cap*sizeof(real64));
      calc_rhs(sys, &(sys->Bvarstep2), -1.0, TRUE);
      calc_rhs(sys, &(sys->stationary), -1.0, TRUE);
      linsolqr_solve(lsys,sys->J.rhs);
      row = sys->mulstep2.rng->low;
      for( ; row <= sys->mulstep2.rng->high; row++ )
         sys->mulstep2.vec[row] = linsolqr_var_value
            (lsys,sys->J.rhs,mtx_row_to_org(sys->J.mtx,row));
      if (sys->iarray[SP7_SAVLIN]) {
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
          FPRINTF(ldat,"%.20g\n",linsolqr_var_value(lsys,sys->J.rhs,ov));
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


static void calc_phi( slv7_system_t sys)
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

#if NGSLV
/**
 *** Current implementation problems:
 *** 1) The regions (un_p_row, A12, etc) which are currently part of
 ***    sys->J are a bit deceptive in that they really should be associated
 ***    with factors.
 **/

/**
 ***  NGSlv Linesearch Functions
 **/

static void set_up_line_search(slv7_system_t sys,mtx_matrix_t *factors,
			       linsolqr_system_t *lsys)
{
  mtx_coord_t loc;
  int32 ind;
  zero_vector(&(sys->tmp_ls));
  for (loc.col = sys->J.un_p_col_reg.col.low;
       loc.col <= sys->J.un_p_col_reg.col.high; loc.col++ ) {
    sys->tmp_ls.vec[mtx_col_to_org(*factors,loc.col)] =
      sys->grad_newton.vec[loc.col] *
      sys->line_search.full_grad_mult;
  }
  for (loc.col = sys->J.reg.col.low;
       loc.col < sys->J.un_p_col_reg.col.low; loc.col++ ) {
    ind = linsolqr_org_col_to_org_row(*lsys,mtx_col_to_org(*factors,loc.col));
    sys->tmp_ls.vec[mtx_col_to_org(*factors,loc.col)] =
      - sys->line_search.full_grad_mult *
	sys->line_search.newton_mult *
	  sys->tmp.vec[ind];
  }
}
/*
static void set_up_line_search(slv7_system_t sys,mtx_matrix_t *factors,
			       linsolqr_system_t *lsys)
{
  mtx_coord_t loc;
  int32 ind;
  zero_vector(&(sys->tmp_ls));
  for (loc.col = sys->J.un_p_col_reg.col.low;
       loc.col <= sys->J.un_p_col_reg.col.high; loc.col++ ) {
    sys->tmp_ls.vec[loc.col] = sys->grad_newton.vec[loc.col] *
      sys->line_search.full_grad_mult;
  }
  for (loc.col = sys->J.reg.col.low;
       loc.col < sys->J.un_p_col_reg.col.low; loc.col++ ) {
    ind = linsolqr_org_col_to_org_row(*lsys,mtx_col_to_org(*factors,loc.col));
    sys->tmp_ls.vec[loc.col] = - sys->line_search.full_grad_mult *
      sys->tmp.vec[ind];
  }
}*/

static void ls_calc_gradient(slv7_system_t sys,
			       mtx_matrix_t *factors,
			       mtx_matrix_t *outer_factors,
			       linsolqr_system_t *lsys)

/**
 ***  Calculates gradient of the linesearch objective function (ls_obj).
 ***  ls_obj = sqrt(sum(h^2)) where h = gradient_eqn_residual and the sum
 ***  is over the gradient equations
 ***  d(ls_obj)/d(alpha) = sum(d(h)/d(alpha))
 ***  d(h)/d(alpha) = J(d(x)/d(alpha))
 ***                   | -inv(L)*A12'*alpha |  <-- Newton eqns
 ***  d(x)/d(alpha) =  |       alpha        |  <-- Gradient eqns
 ***
 **/
{
  int32 row,col,ind,status;
  var_filter_t vfilter;
  real64 resid,ls_obj = 0.0;
  mtx_coord_t loc;
  real64 sum_dh_dalpha;
  struct rel_relation *rel;

  /*   zero_vector(&(sys->tmp_ls));*/
  /* calculate the gradient rows of the jacobian and the corresponding
     residuals at the trial point */
  calc_ok = TRUE;
  vfilter.matchbits = (VAR_INBLOCK | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INBLOCK | VAR_ACTIVE);

  mtx_clear_region(sys->J.mtx,&(sys->J.reg));
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
  for(row = sys->J.reg.row.low;
      row < sys->J.un_p_row_reg.row.low; row++) {
    rel = sys->rlist[mtx_row_to_org(*factors,row)];
    resid = relman_eval(rel,&status,SAFE_CALC);
    ls_obj += sqr(resid);
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
  sys->line_search.obj2 = sqrt(ls_obj);
  ls_obj = 0;

  for(row = sys->J.un_p_row_reg.row.low;
      row <= sys->J.un_p_row_reg.row.high; row++) {
    rel = sys->rlist[mtx_row_to_org(*factors,row)];
    relman_diffs(rel,&vfilter,sys->J.mtx,&resid,SAFE_CALC);
    ls_obj += sqr(resid);
  }
  sys->line_search.obj = sqrt(ls_obj);
  FPRINTF(stderr,"TOTAL ERR = %16.8g\n",sys->line_search.obj +
	  sys->line_search.obj2);
  FPRINTF(stderr,"LS_OBJ = %16.8g, SCALED = %16.8g\n",
	  sys->line_search.obj,
	  sys->line_search.obj/(sys->J.un_p_row_reg.row.high -
				sys->J.un_p_row_reg.row.low + 1));
  FPRINTF(stderr,"LS_OBJ2 = %16.8g, SCALED = %16.8g\n",
	  sys->line_search.obj2,
	  sys->line_search.obj2/(sys->J.un_p_row_reg.row.low -
				 sys->J.reg.row.low));
  for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ ) {
    mtx_mult_col(sys->J.mtx,col,sys->nominals.vec[col],&(sys->J.reg.row));
  }
  for( row=sys->J.reg.row.low; row <= sys->J.reg.row.high; row++ ) {
    mtx_mult_row(sys->J.mtx,row,sys->weights.vec[row],&(sys->J.reg.col));
  }

  /* CORRECT CODE, WRONG IDEA */
  /* Here we calculate -inv(L)*A12'*alpha.  This vector is stored
     in sys->tmp_ls in org row order (wrt factors).*/
  /*A12' is stored in outer_factors*/
  /*   for (loc.row = sys->J.A12_reg.row.low;
       loc.row <= sys->J.A12_reg.row.high; loc.row++ ) {
       ind = mtx_row_to_org(*factors,loc.row);
       sys->tmp_ls.vec[ind] = -sys->line_search.grad_mult*
       mtx_sum_num_in_row(*outer_factors,loc.row,&(sys->J.A12_reg.col));
       }
       forward_substitute2(*lsys,sys->tmp_ls.vec,FALSE);
       for (loc.row = sys->J.A22_reg.row.low;
       loc.row <= sys->J.A22_reg.row.high; loc.row++ ) {
       ind = mtx_row_to_org(*factors,loc.row);
       sys->tmp_ls.vec[ind] = sys->line_search.grad_mult;
       }
       */

  for (loc.row = sys->J.A22_reg.row.low;
       loc.row <= sys->J.A22_reg.row.high; loc.row++ ) {
    ind = mtx_org_to_row(sys->J.mtx,mtx_row_to_org(*factors,loc.row));
    /*     sum_dh_dalpha += mtx_row_dot_full_org_custom_vec(sys->J.mtx,*factors,
	   ind,sys->tmp_ls.vec,
	   mtx_ALL_COLS,
	   TRUE);*/
    sum_dh_dalpha += mtx_row_dot_full_org_vec(sys->J.mtx,
					      ind,sys->tmp_ls.vec,
					      mtx_ALL_COLS,
					      FALSE);
  }

  if (sys->line_search.obj != 0) {
    sys->line_search.grad = sum_dh_dalpha/sys->line_search.obj;
  } else {
    sys->line_search.grad = sum_dh_dalpha;
  }
  FPRINTF(stderr,"LS_GRAD = %16.8g\n",sys->line_search.grad);
}
/**
 ***  NGSlv Functions
 **/

static void set_un_p_reg(slv7_system_t sys)
/**
 *** Sets unpivoted regions
 **/
{
  sys->J.un_p_col_reg.col.low = sys->J.reg.col.high - sys->J.rank_defect + 1;
  sys->J.un_p_col_reg.col.high = sys->J.reg.col.high;
  sys->J.un_p_col_reg.row.low = sys->J.reg.row.low;
  sys->J.un_p_col_reg.row.high = sys->J.reg.row.high;

  sys->J.un_p_row_reg.col.low = sys->J.reg.col.low;
  sys->J.un_p_row_reg.col.high = sys->J.reg.col.high;
  sys->J.un_p_row_reg.row.low = sys->J.reg.row.high - sys->J.rank_defect + 1;
  sys->J.un_p_row_reg.row.high = sys->J.reg.row.high;

  sys->J.A21_reg.col.low = sys->J.reg.col.low;
  sys->J.A21_reg.col.high = sys->J.reg.col.high - sys->J.rank_defect;
  sys->J.A21_reg.row.low = sys->J.reg.row.high - sys->J.rank_defect + 1;
  sys->J.A21_reg.row.high = sys->J.reg.row.high;

  sys->J.A12_reg.col.low = sys->J.reg.col.high - sys->J.rank_defect + 1;
  sys->J.A12_reg.col.high = sys->J.reg.col.high;
  sys->J.A12_reg.row.low = sys->J.reg.row.low;
  sys->J.A12_reg.row.high = sys->J.reg.row.high - sys->J.rank_defect;

  sys->J.A22_reg.col.low = sys->J.reg.col.high - sys->J.rank_defect + 1;
  sys->J.A22_reg.col.high = sys->J.reg.col.high;
  sys->J.A22_reg.row.low = sys->J.reg.row.high - sys->J.rank_defect + 1;
  sys->J.A22_reg.row.high = sys->J.reg.row.high;
}

mtx_sparse_t *create_tmp(slv7_system_t sys)
/**
 *** Function to allocate tmp matrix
 **/
{
  mtx_sparse_t *tmp = NULL;
  tmp = (mtx_sparse_t *)ascmalloc(sizeof(mtx_sparse_t));
  if (ISNULL(tmp)) {
    FPRINTF(stderr,"ERROR:  (slv7) create_tmp\n");
    FPRINTF(stderr,"        Insufficient memory.\n");
    return tmp;
  }
  tmp->cap = sys->J.reg.col.high - sys->J.reg.col.low + 1;
  tmp->len = tmp->cap;
  tmp->data = (real64 *)ascmalloc(sizeof(real64)*tmp->cap);
  tmp->idata = (int32 *)ascmalloc(sizeof(int32)*tmp->cap);
  if (ISNULL(tmp->data) || ISNULL(tmp->idata)) {
    FPRINTF(stderr,"ERROR:  (slv7) create_tmp\n");
    FPRINTF(stderr,"        Insufficient memory.\n");
    mtx_destroy_sparse(tmp);
    tmp = NULL;
  }
  return tmp;
}

static void fill_un_p_row_reg(slv7_system_t sys,mtx_matrix_t *factors,
			      mtx_matrix_t *coef,mtx_matrix_t *outer_factors,
			      mtx_sparse_t **tmp)
/***
 *** Get rows from 'A' matrix and stuff into outer_factors
 *** This fills the un pivoted rows with the elements
 *** in the correct order. Note that no range checking is
 *** needed since we are taking rows from entire region of
 *** 'A' and inserting into the corresponding region of
 *** factors.
 **/
{
  /* CHECK IF SCALING IS NEEDED */
  mtx_coord_t loc;
  int32 k,ind;
  for( loc.row = sys->J.un_p_row_reg.row.low;
    loc.row <= sys->J.un_p_row_reg.row.high; loc.row++ ){
    ind = mtx_row_to_org(*factors,loc.row);
    ind = mtx_org_to_row(*coef,ind);
    *tmp = mtx_org_row_sparse(*coef,ind,*tmp,&(sys->J.un_p_row_reg.col),
                              mtx_IGNORE_ZEROES);
    for (k = 0; k < (*tmp)->len; k++) {
      loc.col = mtx_org_to_col(*factors,(*tmp)->idata[k]);
      mtx_fill_value(*outer_factors,&(loc),(*tmp)->data[k]);
    }
  }
}

static void fill_un_p_col_reg(slv7_system_t sys,mtx_matrix_t *factors,
			      mtx_matrix_t *coef,mtx_matrix_t *outer_factors,
			      mtx_sparse_t **tmp,linsolqr_system_t *lsys)
/***
 *** This fills the L12 setion of outer_factors with
 *** inv(U11)*A12.  Note that range checking is
 *** needed. We are taking cols from entire region of
 *** 'A', removing unwanted elements, premultiplying
 *** by inv(U11) and inserting into the corresponding
 *** region of factors.
 *** We also calculate A22' at this time.
 **/
{
  /*  this is a temporary hack and should be fixed
   *  we need a better way to access these functions
   */
  extern void backward_substitute2(linsolqr_system_t sys,
				   real64 *arr,
				   boolean transpose);
  extern void forward_substitute2(linsolqr_system_t sys,
				  real64 *arr,
				  boolean transpose);

  /* CHECK IF SCALING IS NEEDED (i.e. is coef scaled?)*/
  mtx_coord_t loc;
  int32 k,ind;
  double tmp_double;
  zero_vector(&(sys->tmp));
  for( loc.col = sys->J.un_p_col_reg.col.low;
      loc.col <= sys->J.un_p_col_reg.col.high; loc.col++ ) {
    ind = mtx_col_to_org(*factors,loc.col);
    ind = mtx_org_to_col(*coef,ind);
    *tmp = mtx_org_col_sparse(*coef,ind,*tmp,&(sys->J.reg.row),
                               mtx_IGNORE_ZEROES);

    /* This loop stores a col of A12 into sys->tmp.vec          */
    /* in org row order. There should be no other non-zeros     */
    /* than those introduced here.                              */
    for (k = 0; k < (*tmp)->len; k++) {
      if (mtx_org_to_row(*coef,(*tmp)->idata[k]) < sys->J.un_p_row_reg.row.low) {
	sys->tmp.vec[mtx_row_to_org(*factors,mtx_org_to_row(*coef,(*tmp)->idata[k]))] =
	  (*tmp)->data[k];
      }
    }

    backward_substitute2(*lsys,sys->tmp.vec,FALSE);
    for (k = sys->tmp.rng->low; k <= sys->tmp.rng->high; k++) {
      if (sys->tmp.vec[k] != D_ZERO) {
	loc.row = mtx_org_to_row(*factors,k);
	mtx_fill_value(*outer_factors,&loc,sys->tmp.vec[k]);
	/* Do not zero sys->tmp.vec here as it will be used */
	/* in calculating A22'                              */
      }
    }

    /* now calculating A22' = A22 - A21*inv(L11)*inv(U11)*A12 */
    forward_substitute2(*lsys,sys->tmp.vec,FALSE);
    for (loc.row = sys->J.A22_reg.row.low;
	 loc.row <= sys->J.A22_reg.row.high; loc.row++ ) {
      tmp_double = mtx_value(*outer_factors,&loc);
      mtx_clear_coord(*outer_factors,loc.row,loc.col);
      mtx_fill_value(*outer_factors,&(loc),
		    (tmp_double -
		     mtx_row_dot_full_org_vec(*outer_factors,loc.row,
					     sys->tmp.vec,
					     &(sys->J.un_p_row_reg.col),
					     TRUE)));
    }
    /* note that the above code is in very bad style.
     * we should probably use mtx_fill_value and then
     * an mtx_assemble after all rows/cols are done
     */
    zero_vector(&(sys->tmp));
  }
}

static void ngslv_calc_varstep(slv7_system_t sys,mtx_matrix_t *factors)
{
  int32 col,ind;
  col = sys->newton.rng->low;
  for( ; col <= sys->newton.rng->high; col++ ) {
    ind = mtx_org_to_col(sys->J.mtx,mtx_col_to_org(*factors,col));
    sys->varstep.vec[ind] = sys->grad_newton2.vec[col];
  }
}

static void calc_un_p_rhs(slv7_system_t sys, mtx_matrix_t *factors,
			  mtx_matrix_t *outer_factors,
			  real64 *varvalue)
/**
 *** This function stores B2 in the lower section of sys->tmp.vec
 *** then proceeds to calculate B2' = B2 - A21*inv(L11)*inv(U11)*B1
 *** where inv(L11)*inv(U11)*B1 is the newton direction in the
 *** pivoted vars.
 **/
{
  mtx_coord_t loc;
  /* Store B2 in sys->tmp.vec in org row order wrt sys->J.mtx */
  for( loc.row = sys->J.un_p_row_reg.row.low;
      loc.row <= sys->J.un_p_row_reg.row.high; loc.row++ ) {
    sys->tmp.vec[mtx_row_to_org(sys->J.mtx,loc.row)] =
      -(sys->residuals.vec[loc.row]);
  }
  for( loc.row = sys->J.un_p_row_reg.row.low;
      loc.row <= sys->J.un_p_row_reg.row.high; loc.row++ ) {
    sys->tmp.vec[mtx_row_to_org(*factors,loc.row)] -=
      mtx_row_dot_full_cur_vec(*outer_factors,loc.row,
			       sys->grad_newton.vec,
			       &(sys->J.A21_reg.col),FALSE);
  }
}

static void calc_un_p_grad(slv7_system_t sys,mtx_matrix_t *factors,
			   mtx_matrix_t *outer_factors,
			   real64 *varvalue)
/**
 *** Calculates Xg, the gradient direction in the unpivoted variables.
 *** Xg = trans(A22')*B2' stored in sys->grad_newton.vec in cur col order (wrt J.mtx).
 **/
{
  mtx_coord_t loc;
  for (loc.col = sys->J.un_p_col_reg.col.low;
       loc.col <= sys->J.un_p_col_reg.col.high; loc.col++ ) {
    sys->grad_newton.vec[loc.col] =
      mtx_col_dot_full_org_vec(*outer_factors,loc.col,sys->tmp.vec,
			       &(sys->J.un_p_row_reg.row),FALSE);
  }
}

static void calc_newton_adjustment(slv7_system_t sys,linsolqr_system_t *lsys,
				   mtx_matrix_t *factors,
				   mtx_matrix_t *outer_factors,
				   real64 *varvalue)
/**
 *** This function calculates the unscaled adjustment to the newton step
 *** inv(L11)*A12'*Xg stored in tmp.vec in org row order (wrt factors).
 *** note that A12' = inv(U11)*A12 is stored in outer_factors.
 **/
{
  extern void forward_substitute2(linsolqr_system_t sys,
				  real64 *arr,
				  boolean transpose);
  mtx_coord_t loc;
  int32 ind;
  for (loc.row = sys->J.A12_reg.row.low;
       loc.row <= sys->J.A12_reg.row.high; loc.row++ ) {
    ind = mtx_row_to_org(*factors,loc.row);
    sys->tmp.vec[ind] =
      mtx_row_dot_full_cur_vec(*outer_factors,loc.row,sys->grad_newton.vec,
			       mtx_ALL_COLS,FALSE); /*all_cols should be ok here???*/
  }
  forward_substitute2(*lsys,sys->tmp.vec,FALSE);
}

static void set_grad_multiplier(slv7_system_t sys,
				  mtx_matrix_t *outer_factors)
{
  int32 ind;
  double denominator, numerator,alpha;
  denominator = numerator = 0;
  for (ind = sys->J.un_p_col_reg.col.low;
       ind <= sys->J.un_p_col_reg.col.high; ind++ ) {
    sys->tmp2.vec[ind] =
      mtx_row_dot_full_cur_vec(*outer_factors,ind,sys->grad_newton.vec,
			       &(sys->J.un_p_col_reg.col),FALSE);
    numerator += sqr(sys->grad_newton.vec[ind]);
    denominator += sqr(sys->tmp2.vec[ind]);
  }
  if (denominator == 0) {
    FPRINTF(stderr,"Infinite denominator in NGSlv\n");
    sys->line_search.full_grad_mult = 0.0;
  }
  alpha = numerator/denominator;
  if (alpha < 0){
    FPRINTF(stderr,"Warning: Negative Alpha in NGSlv\n");
    sys->line_search.full_grad_mult = -alpha;
  } else {
    sys->line_search.full_grad_mult = alpha;
  }
}

static void calc_ng_step(slv7_system_t sys,linsolqr_system_t *lsys,
			 mtx_matrix_t *factors,
			 real64 *varvalue)
/**
 *** Calculates the actual step given the gradient multiplier.
 *** Stores the step in sys->grad_newton2.vec in cur col order.
 **/
{
  mtx_coord_t loc;
  int32 ind;
#if KDEBUG
  real64 gradnorm2 = 0;
  real64 newtnorm2 = 0;
#endif /*KDEBUG*/
  zero_vector(&(sys->grad_newton2));
  /* we will do our own dense arithmetic here */
  for (loc.col = sys->J.un_p_col_reg.col.low;
       loc.col <= sys->J.un_p_col_reg.col.high; loc.col++ ) {
    sys->grad_newton2.vec[loc.col] = sys->grad_newton.vec[loc.col] *
      sys->line_search.full_grad_mult *
      sys->line_search.grad_mult;
#if KDEBUG
    gradnorm2 += sqr(sys->grad_newton2.vec[loc.col]);
#endif /*KDEBUG*/
  }
  for (loc.col = sys->J.reg.col.low;
       loc.col < sys->J.un_p_col_reg.col.low; loc.col++ ) {
    /* THIS IS A MESS */
    ind = linsolqr_org_col_to_org_row(*lsys,mtx_col_to_org(*factors,loc.col));
    sys->grad_newton2.vec[loc.col] = sys->line_search.newton_mult *
      (sys->grad_newton.vec[loc.col] -
      sys->line_search.full_grad_mult * sys->tmp.vec[ind] *
	sys->line_search.grad_mult);
#if KDEBUG
    newtnorm2 += sqr(sys->grad_newton2.vec[loc.col]);
#endif /*KDEBUG*/
  }
#if KDEBUG
  gradnorm2 = sqrt(gradnorm2);
  newtnorm2 = sqrt(newtnorm2);
  sys->line_search.newton_norm = newtnorm2;
  sys->line_search.grad_norm = gradnorm2;
  FPRINTF(stderr,"NEWTON NORM = %16.8g, SCALED = %16.8g\n",
	  newtnorm2,
	  newtnorm2/(sys->J.un_p_row_reg.row.low -
		     sys->J.reg.row.low));
  FPRINTF(stderr,"GRADIENT NORM = %16.8g, SCALED = %16.8g\n",
	  gradnorm2,
	  gradnorm2/(sys->J.un_p_row_reg.row.high -
		     sys->J.un_p_row_reg.row.low + 1));
#endif /*KDEBUG*/
}
static void print_ls_data(slv7_system_t sys, int32 task)
{
  FILE *output;
  output = fopen ("ngslv_data","a");
  if (task == 0) {
    FPRINTF(output,"%4.3g %d %d %8.4g %8.4g %8.4g %8.4g %8.4g %8.4g %8.4g\n",
	    sys->p.tolerance.singular,
	    sys->line_search.rank_defect,
	    sys->line_search.rank,
	    sys->line_search.newton_norm, /* 2norm newton step */
	    sys->line_search.grad_norm, /* 2norm grad step */
	    sys->line_search.full_grad_mult, /* calculated grad mult */
	    sys->line_search.grad_mult, /* grad multiplier 0->1 */
	    sys->line_search.newton_mult, /* newton multiplier 0->1 */
	    sys->line_search.obj2, /* newton error */
	    sys->line_search.obj /* gradient error */
	    );
  } else if (task == 1) {
    FPRINTF(output," ****STEP ACCEPTED**** \n");
    FPRINTF(output," grad mult = %8.4g\n",sys->line_search.grad_mult);
    FPRINTF(output," ********************* \n");
  } else {
    FPRINTF(output,"\n");
  }
  fclose (output);
}

static real64 ngslv_calc_newton_mult(slv7_system_t sys)
{
  real64 mult = 1, var_norm, ratio;
  int32 col;
  ratio = sys->line_search.newton_norm/sys->line_search.rank;
  if( ratio > 0.05 ) {
    mult = 0.05 / ratio;
  }
  return mult;

  for( col = sys->J.reg.col.low;
      col < sys->J.un_p_col_reg.col.low; col++) {
    struct var_variable *var;
    var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
    if (sys->nominals.vec[col] != 0) {
      var_norm = sqr(var_value(var) / sys->nominals.vec[col]);
    }
  }
  var_norm = sqrt(var_norm);
  ratio = sys->line_search.newton_norm/var_norm;
  if (ratio > 1) {
    mult = 1/ratio;
  }
  return mult;
}

static void apply_step( slv7_system_t );
static void restore_variables( slv7_system_t );

static void ngslv_line_search(slv7_system_t sys,
			      linsolqr_system_t *lsys,
			      mtx_matrix_t *factors,
			      mtx_matrix_t *outer_factors,
			      real64 *varvalue)
{
  int32 ind;
  real64 best_obj = 1e300, best_alpha = -1;
  sys->line_search.newton_mult = 1;
  sys->line_search.grad_mult = 1;

  sys->line_search.rank = sys->J.un_p_row_reg.row.low -
				 sys->J.reg.row.low;
  sys->line_search.rank_defect = sys->J.un_p_row_reg.row.high -
				sys->J.un_p_row_reg.row.low + 1;
/*  calc_ng_step(sys,lsys,factors,varvalue);
  ngslv_calc_varstep(sys,factors);
  apply_step(sys);
  ls_calc_gradient(sys,factors,outer_factors,lsys);
  sys->line_search.newton_mult = ngslv_calc_newton_mult(sys);
  restore_variables(sys);
*/
  for (ind = 1; ind <= 10; ++ind) {
    calc_ng_step(sys,lsys,factors,varvalue);
    ngslv_calc_varstep(sys,factors);
    apply_step(sys);
    ls_calc_gradient(sys,factors,outer_factors,lsys);
    if(sys->line_search.obj + sys->line_search.obj2 < best_obj) {
      best_obj = sys->line_search.obj;
      best_alpha = sys->line_search.grad_mult;
    }
    print_ls_data(sys,0);
    restore_variables(sys);
    sys->line_search.grad_mult -= 0.1;
    sys->line_search.newton_mult -= 0.1;
  }
  sys->line_search.grad_mult = best_alpha;
  calc_ng_step(sys,lsys,factors,varvalue);
  ngslv_calc_varstep(sys,factors);
  print_ls_data(sys,1);
}

static void calc_newton_grad_step(slv7_system_t sys)
/**
 *** Calculate Newton/Gradient step
 **/
{
  mtx_sparse_t *tmp = NULL;
  linsolqr_system_t lsys = sys->J.sys;
  mtx_matrix_t factors, coef, outer_factors;
  real64 *varvalue;
  /* THIS IS GOING AWAY
  int32 status_ok; */

  factors = linsolqr_get_factors(lsys);
  outer_factors = mtx_create_slave(factors);
  coef = linsolqr_get_matrix(lsys);
  varvalue = linsolqr_get_varvalue(lsys,0);

  set_un_p_reg(sys);
  tmp = create_tmp(sys);
  if (tmp == NULL) {
    return;
  }
  mtx_clear_region(factors,&(sys->J.un_p_row_reg));
  mtx_clear_region(factors,&(sys->J.A12_reg));

  fill_un_p_row_reg(sys,&factors,&coef,&outer_factors,&tmp);
  fill_un_p_col_reg(sys,&factors,&coef,&outer_factors,&tmp,&lsys);

/* THIS IS GOING AWAY
  status_ok = linsolqr_setup_ngslv(lsys,sys->J.rhs,
				   &(sys->J.un_p_row_reg.row),sys->tmp.vec);
  if (status_ok != 1) {
    return;
  }
*/
  zero_vector(&(sys->grad_newton));
  ngslv_calc_newton(sys,&factors);
  calc_un_p_rhs(sys,&factors,&outer_factors,varvalue);
  calc_un_p_grad(sys,&factors,&outer_factors,varvalue);
  set_grad_multiplier(sys,&outer_factors);
  zero_vector(&(sys->tmp));
  calc_newton_adjustment(sys,&lsys,&factors,&outer_factors,varvalue);

  /* Here we will 'manualy' scale the step. This should eventually be a linesearch */
  sys->line_search.full_grad_mult =
    sys->rarray[SP7_GMULT] * sys->line_search.full_grad_mult;
/*
  FPRINTF(stderr,"ALPHA = %16.8g\n",sys->line_search.full_grad_mult);
  sys->line_search.newton_mult = 1;
  calc_ng_step(sys,&lsys,&factors,varvalue);
  ngslv_calc_varstep(sys,&factors);
*/
  set_up_line_search(sys,&factors,&lsys);
  ngslv_line_search(sys,&lsys,&factors,&outer_factors,varvalue);
}
#endif /*NGSLV*/

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

static void calc_2x2_system(slv7_system_t sys, struct calc_step_vars *vars)
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

static void coefs_from_parm( slv7_system_t sys, struct calc_step_vars *vars)
/**
 ***  Determines alpha1 and alpha2 from the parameter (guess).
 **/
{

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
}

static real64 step_norm2( slv7_system_t sys, struct calc_step_vars *vars)
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

static void adjust_parms( slv7_system_t sys, struct calc_step_vars *vars)
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

static void compute_step( slv7_system_t sys, struct calc_step_vars *vars)
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


static void calc_step( slv7_system_t sys, int minor)
/**
 ***  Calculates step vector, based on sys->maxstep, and the varstep2/
 ***  varstep1 and mulstep2/mulstep1 vectors.  Nothing is assumed to be
 ***  calculated, except the weights and the jacobian (scaled).  Also,
 ***  the step is not checked for legitimacy.
 ***  NOTE: the step is scaled.
 **/
{

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

static void restore_variables( slv7_system_t sys)
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


static real64 required_coef_to_stay_inbounds( slv7_system_t sys)
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


static void apply_step( slv7_system_t sys)
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

static void ngslv_apply_step( slv7_system_t sys)
/**
 ***  Adds sys->varstep to the variable values in block: projecting
 ***  near bounds.
 **/
{
   FILE *lif = LIF(sys);
   int nproj = 0;
   real64 bounds_coef = 1.0, norm = 0;
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
/*         sys->varstep.vec[col] = dx/sys->nominals.vec[col];*/
	 norm += sqr(dx/sys->nominals.vec[col]);
      } else {
         if( !sys->p.ignore_bounds ) {
            if( val + dx > (bnd=var_upper_bound(var)) ) {
               dx = TOWARD_BOUNDS*(bnd-val);
/*               sys->varstep.vec[col] = dx/sys->nominals.vec[col];*/
	       norm += sqr(dx/sys->nominals.vec[col]);
               if (sys->p.output.less_important) {
                 FPRINTF(lif,"%-40s ---> ",
                         "    Variable projected to upper bound");
                 print_var_name(lif,sys,var); PUTC('\n',lif);
               }
               ++nproj;
            } else if( val + dx < (bnd=var_lower_bound(var)) ) {
               dx = TOWARD_BOUNDS*(bnd-val);
/*               sys->varstep.vec[col] = dx/sys->nominals.vec[col];*/
	       norm += sqr(dx/sys->nominals.vec[col]);
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
   sys->varstep.norm2 = norm;
   if( !sys->p.ignore_bounds ) {
      if (nproj > 0) {
/*         square_norm(&(sys->varstep));*/
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
/*         square_norm(&(sys->varstep));*/
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

static void step_accepted( slv7_system_t sys)
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

static void change_maxstep( slv7_system_t sys, real64 maxstep)
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
 ***     move_to_next_block(sys)
 ***     find_next_unconverged_block(sys)
 **/

static boolean block_feasible( slv7_system_t sys)
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

static void move_to_next_block( slv7_system_t sys)
/**
 ***  Moves on to the next block, updating all of the solver information.
 ***  To move to the first block, set sys->s.block.current_block to -1 before
 ***  calling.  If already at the last block, then sys->s.block.current_block
 ***  will equal the number of blocks and the system will be declared
 ***  converged.  Otherwise, the residuals for the new block will be computed
 ***  and sys->s.calc_ok set according.
 **/
{
  struct var_variable *var;
  struct rel_relation *rel;
  int32 row;
  int32 col;
  int32 ci;

  if( sys->s.block.current_block >= 0 ) {


    /* Record cost accounting info here. */
    ci=sys->s.block.current_block;
    sys->s.cost[ci].size	=	sys->s.block.current_size;
    sys->s.cost[ci].iterations	=	sys->s.block.iteration;
    sys->s.cost[ci].funcs	=	sys->s.block.funcs;
    sys->s.cost[ci].jacs	=	sys->s.block.jacs;
    sys->s.cost[ci].functime	=	sys->s.block.functime;
    sys->s.cost[ci].jactime	=	sys->s.block.jactime;
    sys->s.cost[ci].time	=	sys->s.block.cpu_elapsed;
    sys->s.cost[ci].resid	=	sys->s.block.residual;

    /* De-initialize previous block */
    if (sys->p.output.less_important && (sys->s.block.current_size >1 ||
        sys->iarray[SP7_LIFDS])) {
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

  sys->s.block.current_block++;
  if( sys->s.block.current_block < sys->s.block.number_of ) {
    int32 row;
    int32 col;
    boolean ok;

    /* Initialize next block */
    if( OPTIMIZING(sys) ) {
      mtx_region(&(sys->J.reg), 0, sys->rank-1, 0, sys->vused-1 );
    } else {
      sys->J.reg =
        (slv_get_solvers_blocks(SERVER))->block[sys->s.block.current_block];
    }

    row = sys->J.reg.row.high - sys->J.reg.row.low + 1;
    col = sys->J.reg.col.high - sys->J.reg.col.low + 1;
    sys->s.block.current_size = MAX(row,col);

    sys->s.block.iteration = 0;
    sys->s.block.cpu_elapsed = 0.0;
    sys->s.block.functime = 0.0;
    sys->s.block.jactime = 0.0;
    sys->s.block.funcs = 0;
    sys->s.block.jacs = 0;

    if(sys->p.output.less_important && (sys->iarray[SP7_LIFDS] ||
      sys->s.block.current_size > 1)) {
      debug_delimiter(LIF(sys));
      debug_delimiter(LIF(sys));
    }
    if(sys->p.output.less_important && sys->iarray[SP7_LIFDS]) {
      FPRINTF(LIF(sys),"\n%-40s ---> %d in [%d..%d]\n",
              "Current block number", sys->s.block.current_block,
              0, sys->s.block.number_of-1);
      FPRINTF(LIF(sys),"%-40s ---> %d\n", "Current block size",
        sys->s.block.current_size);
    }
    sys->s.calc_ok = TRUE;

    if( !(ok = calc_objective(sys)) ) {
         FPRINTF(MIF(sys),"Objective calculation errors detected.\n");
    }
    if(sys->p.output.less_important && sys->obj) {
      FPRINTF(LIF(sys),"%-40s ---> %g\n", "Objective", sys->objective);
    }
    sys->s.calc_ok = sys->s.calc_ok && ok;

    if (!(sys->p.ignore_bounds) ) {
      slv_ensure_bounds(SERVER, sys->J.reg.col.low,
                        sys->J.reg.col.high,MIF(sys));
    }

    sys->residuals.accurate = FALSE;
    if( !(ok = calc_residuals(sys)) ) {
      FPRINTF(MIF(sys),"Residual calculation errors detected.\n");
    }
    if( sys->p.output.less_important &&
        (sys->s.block.current_size >1 ||
         sys->iarray[SP7_LIFDS]) ) {
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
      if( !(ok=calc_residuals(sys)) ) {
         FPRINTF(MIF(sys),"Residual calculation errors detected.\n");
      }
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

static void reorder_new_block(slv7_system_t sys)
/**
 ***  Calls the appropriate reorder function on a block
 **/
{
  int32 method;
  if( sys->s.block.current_block < sys->s.block.number_of ) {
    switch(sys->iarray[SP7_ORDERM]) {
    case 1:
      method = 1;
    case 0:
    default:
      method = 0;
    }

    if( sys->s.block.current_block <= sys->s.block.current_reordered_block &&
       sys->s.cost[sys->s.block.current_block].reorder_method == method &&
       sys->s.block.current_block >= 0 ) {
#if DEBUG
      FPRINTF(stderr,"YOU JUST AVOIDED A REORDERING\n");
#endif
      slv_set_up_block(SERVER,sys->s.block.current_block);
      /* tell linsol to bless it and get on with things */
      linsolqr_reorder(sys->J.sys,&(sys->J.reg),natural);
      return; /*must have been reordered since last system build*/
    }

    /* Let the slv client function take care of reordering things
     * and setting in block flags.
     */
    switch(sys->iarray[SP7_ORDERM]) {
    case 1:
      sys->s.cost[sys->s.block.current_block].reorder_method = 1;
      slv_spk1_reorder_block(SERVER,sys->s.block.current_block);
      break;
    case 0:
    default:
      sys->s.cost[sys->s.block.current_block].reorder_method = 0;
      slv_tear_drop_reorder_block(SERVER,sys->s.block.current_block,
                                  sys->iarray[SP7_CUTOFF],
				  sys->iarray[SP7_ORDERM],mtx_SPK1);
      break;
    }
    /* tell linsol to bless it and get on with things */
    linsolqr_reorder(sys->J.sys,&(sys->J.reg),natural);
    if (sys->s.block.current_block > sys->s.block.current_reordered_block) {
      sys->s.block.current_reordered_block = sys->s.block.current_block;
    }
  }
}

static void find_next_unconverged_block( slv7_system_t sys)
/**
 ***  Moves to next unconverged block, assuming that the current block has
 ***  converged (or is -1, to start).
 **/
{
   do {
     move_to_next_block(sys);
#if DEBUG
     debug_out_var_values(stderr,sys);
     debug_out_rel_residuals(stderr,sys);
#endif
   } while( !sys->s.converged && block_feasible(sys) && !OPTIMIZING(sys) );
   reorder_new_block(sys);
}


/**
 ***  Iteration begin/end routines
 ***  ----------------------------
 ***     iteration_begins(sys)
 ***     iteration_ends(sys)
 **/

static void iteration_begins( slv7_system_t sys)
/**
 ***  Prepares sys for entering an iteration, increasing the iteration counts
 ***  and starting the clock.
 **/
{
   sys->clock = tm_cpu_time();
   ++(sys->s.block.iteration);
   ++(sys->s.iteration);
   if(sys->p.output.less_important&& (sys->s.block.current_size >1 ||
      sys->iarray[SP7_LIFDS])) {
     FPRINTF(LIF(sys),"\n%-40s ---> %d\n",
             "Iteration", sys->s.block.iteration);
     FPRINTF(LIF(sys),"%-40s ---> %d\n",
             "Total iteration", sys->s.iteration);
   }
}

static void iteration_ends( slv7_system_t sys)
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
         sys->iarray[SP7_LIFDS])) {
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Elapsed time", sys->s.block.cpu_elapsed);
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Total elapsed time", sys->s.cpu_elapsed);
   }
}

static void update_status( slv7_system_t sys)
/**
 ***  Updates the solver status.
 **/
{
   boolean unsuccessful;

   if( !sys->s.converged ) {
      sys->s.time_limit_exceeded =
         (sys->s.block.cpu_elapsed >= sys->p.time_limit);
      sys->s.iteration_limit_exceeded =
         (sys->s.block.iteration >= sys->p.iteration_limit);
   }

   unsuccessful = sys->s.diverged || sys->s.inconsistent ||
      sys->s.iteration_limit_exceeded || sys->s.time_limit_exceeded;

   sys->s.ready_to_solve = !unsuccessful && !sys->s.converged;
   sys->s.ok = !unsuccessful && sys->s.calc_ok && !sys->s.struct_singular;
}

/**
 ***  External routines
 ***  -----------------
 ***     See slv_client.h
 **/

static SlvClientToken slv7_create(slv_system_t server, int *statusindex)
{
  slv7_system_t sys;

  sys = (slv7_system_t)ascmalloc( sizeof(struct slv7_system_structure) );
  if (sys==NULL) {
    *statusindex = 1;
    return sys;
  }
  mem_zero_byte_cast(sys,0,sizeof(struct slv7_system_structure));
  SERVER = server;
  sys->integrity = OK;
  sys->presolved = 0;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = stdout;
  sys->p.tolerance.pivot = 0.1;       /* scaled */
  sys->p.tolerance.singular = 1e-12;  /* scaled */
  sys->p.tolerance.feasible = 1e-8;  /* unscaled */
  sys->p.tolerance.rootfind = 1e-12;  /* unscaled */
  sys->p.tolerance.stationary = 1e-8;   /* scaled */
  sys->p.tolerance.termination = 1e-12;  /* scaled */
  sys->p.time_limit = 1500.0;
  sys->p.iteration_limit = 100;
  sys->p.partition = TRUE;
  sys->J.old_partition = TRUE;
  sys->p.ignore_bounds = FALSE;
  sys->p.whose = (*statusindex);
  sys->p.rho = 1.0;
  sys->p.sp.ilen = slv7_IA_SIZE; /* all defaults in iarray are 0 */
  sys->p.sp.rlen = slv7_RA_SIZE; /* all defaults in rarray are 0 */
  sys->p.sp.iap=&(sys->iarray[0]); /* all defaults in r/iarray are 0 except */
  sys->p.sp.rap=&(sys->rarray[0]); /* all defaults in r/iarray are 0 except */
  sys->iarray[SP7_LIFDS] = 0;
  sys->iarray[SP7_CNCOLS] = 1;
  sys->iarray[SP7_CUTOFF] = 500;
  sys->iarray[SP7_UPJAC] = 1;
  sys->iarray[SP7_UPWTS] = 1;
  sys->iarray[SP7_UPNOM] = 1000;
  sys->rarray[SP7_2SMALL] = 1e-8;
  sys->rarray[SP7_CNLOW] = 0.01;
  sys->rarray[SP7_CNHIGH] = 100.0;
  sys->rarray[SP7_TBNDS] = 0.95;
  sys->rarray[SP7_POSDEF] = 0.01;
  sys->rarray[SP7_DET0] = 1e-8;
  sys->rarray[SP7_SSERRM] = 1e-4;
  sys->rarray[SP7_PRMAX] = 1e-12;
  sys->rarray[SP7_MINCO] = 0.05;
  sys->rarray[SP7_MAXCO] = 0.95;
  sys->rarray[SP7_GMULT] = 1e-4;

  sys->p.sp.ianames=slv7_ianames; /* iarray symbolic names are here */
  sys->p.sp.ranames=slv7_ranames; /* rarray symbolic names are here */
  sys->p.sp.iaexpln=slv7_iaexpln; /* iarray explanations are here */
  sys->p.sp.raexpln=slv7_raexpln; /* rarray explanations are here */
  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.costsize = 0;
  sys->s.cost = NULL; /*redundant, but sanity preserving */
  sys->vlist = slv_get_solvers_var_list(server);
  sys->rlist = slv_get_solvers_rel_list(server);
  sys->obj = slv_get_obj_relation(server);
  if (sys->vlist == NULL) {
    ascfree(sys);
    FPRINTF(stderr,"NGSlv called with no variables.\n");
    *statusindex = -2;
    return NULL;
  }
  if (sys->rlist == NULL && sys->obj == NULL) {
    ascfree(sys);
    FPRINTF(stderr,"NGSlv called with no relations or objective.\n");
    *statusindex = -1;
    return NULL;
  }
  /* we don't give a damn about the objective list or the pars or
   * bounds or extrels or any of the other crap.
   */
  slv_check_var_initialization(server);
  *statusindex = 0;
  return((SlvClientToken)sys);
}

static void destroy_matrices( slv7_system_t sys)
{
   if( sys->J.sys ) {
      int count = linsolqr_number_of_rhs(sys->J.sys)-1;
      for( ; count >= 0; count-- )
         destroy_array(linsolqr_get_rhs(sys->J.sys,count));
      mtx_destroy(linsolqr_get_matrix(sys->J.sys));
      linsolqr_set_matrix(sys->J.sys,NULL);
      linsolqr_destroy(sys->J.sys);
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

static void destroy_vectors( slv7_system_t sys)
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
   destroy_array(sys->un_p_grad.vec);
   destroy_array(sys->tmp.vec);
   destroy_array(sys->tmp2.vec);
   destroy_array(sys->tmp_ls.vec);
}

static int slv7_eligible_solver(slv_system_t server)
{
  struct rel_relation **rp;
  for( rp=slv_get_solvers_rel_list(server); *rp != NULL ; ++rp ) {
    if( rel_less(*rp) || rel_greater(*rp) ) return(FALSE);
  }
  return(TRUE);
}

static void slv7_get_parameters(slv_system_t server, SlvClientToken asys,
                         slv_parameters_t *parameters)
{
  slv7_system_t sys;
  sys = SLV7(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

static void slv7_set_parameters(slv_system_t server, SlvClientToken asys,
                         slv_parameters_t *parameters)
{
  slv7_system_t sys;
  sys = SLV7(asys);
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

static void slv7_get_status(slv_system_t server, SlvClientToken asys,
                            slv_status_t *status)
{
  slv7_system_t sys;
  sys = SLV7(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}

static linsolqr_system_t slv7_get_linsolqr_sys(slv_system_t server,
                                               SlvClientToken asys)
{
  slv7_system_t sys;
  sys = SLV7(asys);
  if (check_system(sys)) return NULL;
  return(sys->J.sys);
}

static linsol_system_t slv7_get_linsol_sys(slv_system_t server,
                                           SlvClientToken asys)
{
  return( NULL );
}

static void structural_analysis(slv_system_t server, slv7_system_t sys)
/**
 ***  Performs structural analysis on the system, setting the flags in
 ***  status.  The problem must be set up, the relation/variable list
 ***  must be non-NULL.  The
 ***  jacobian (linear) system must be created and have the correct order
 ***  (stored in sys->cap).  Everything else will be determined here.
 ***  On entry there isn't yet a correspondence between var_sindex and
 ***  jacobian column. Here we establish that.
 **/
{
  var_filter_t vfilter;
  rel_filter_t rfilter;

  /**
   *** The server has marked incidence flags already.
   **/
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
  if (sys->rtot) {
    slv_block_partition(server);
  }
  sys->J.dofdata = slv_get_dofdata(server);
  sys->rank = sys->J.dofdata->structural_rank;
  sys->ZBZ.order = sys->obj ? (sys->vused - sys->rank) : 0;
  if( !(sys->p.partition) || OPTIMIZING(sys) ) {
    slv_block_unify(server);
  }

  if(slv_check_bounds(SERVER,sys->vused,-1,"fixed ")){
    sys->s.inconsistent = 1;
  }

  /* Initialize Status */
  sys->s.over_defined = (sys->rused > sys->vused);
  sys->s.under_defined = (sys->rused < sys->vused);
  sys->s.struct_singular = (sys->rank < sys->rused);
  sys->s.block.number_of = (slv_get_solvers_blocks(SERVER))->nblocks;
}

/*
  configures linsolqr system, among other things.
  sets type to be ranki_kw or ranki_jz as determined by
  parameters.
*/
static void create_matrices(slv_system_t server, slv7_system_t sys)
{
  sys->J.sys = linsolqr_create();
  sys->J.mtx = mtx_create();
  switch (sys->p.factor_option) {
  case 0:
    sys->J.fm = ranki_kw;
    break;
  case 1:
    sys->J.fm = ranki_jz;
    break;
  case 2:
    sys->J.fm = ranki_kw2;
    break;
  case 3:
    sys->J.fm = ranki_jz2;
    break;
  default:
    sys->J.fm = ranki_kw2;
    break;
  }
  mtx_set_order(sys->J.mtx,sys->cap);
  linsolqr_set_matrix(sys->J.sys,sys->J.mtx);
  linsolqr_prep(sys->J.sys,ranki);
  linsolqr_set_pivot_zero(sys->J.sys, sys->p.tolerance.singular);
  linsolqr_set_drop_tolerance(sys->J.sys, sys->p.tolerance.drop);
  linsolqr_set_pivot_tolerance(sys->J.sys, sys->p.tolerance.pivot);
  /* this next one is fishy, but we don't use qr so not panicking */
  linsolqr_set_condition_tolerance(sys->J.sys, sys->p.tolerance.pivot);
  /* rhs 0 for sys->multipliers */
  sys->J.rhs = create_array(sys->cap,real64);
  linsolqr_add_rhs(sys->J.sys,sys->J.rhs,TRUE);
  /* rhs 1 for sys->newton */
  sys->J.rhs = create_array(sys->cap,real64);
  linsolqr_add_rhs(sys->J.sys,sys->J.rhs,FALSE);
  /* rhs 2 for sys->mulstep2 */
  sys->J.rhs = create_array(sys->cap,real64);
  linsolqr_add_rhs(sys->J.sys,sys->J.rhs,TRUE);
  sys->J.relpivots = set_create(sys->cap);
  sys->J.varpivots = set_create(sys->cap);

  structural_analysis(server,sys);
  sys->ZBZ.mtx = create_array(sys->ZBZ.order,real64 *);
  if( sys->ZBZ.mtx ) {
    int i;
    for( i=0; i<sys->ZBZ.order; i++ ) {
      sys->ZBZ.mtx[i] = create_array(sys->ZBZ.order,real64);
    }
  }
  sys->ZBZ.ZBs = create_array(sys->ZBZ.order,real64);
  sys->ZBZ.Zy = create_array(sys->ZBZ.order,real64);
}

static void create_vectors(sys)
slv7_system_t sys;
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
#if NGSLV
  sys->grad_newton.vec = create_array(sys->cap,real64);
  sys->grad_newton.rng = &(sys->J.reg.col);
  sys->grad_newton2.vec = create_array(sys->cap,real64);
  sys->grad_newton2.rng = &(sys->J.reg.col);
  sys->un_p_grad.vec = create_array(sys->cap,real64);
  sys->un_p_grad.rng = &(sys->J.un_p_col_reg.col);
  sys->tmp.vec = create_array(sys->cap,real64);
  sys->tmp.rng = &(sys->J.reg.row);
  sys->tmp2.vec = create_array(sys->cap,real64);
  sys->tmp2.rng = &(sys->J.reg.row);
  sys->tmp_ls.vec = create_array(sys->cap,real64);
  sys->tmp_ls.rng = &(sys->J.reg.row);
#endif /*NGSLV*/
}

void slv7_dump_internals(slv_system_t server, SlvClientToken sys,int level)
{
  check_system(sys);
  if (level > 0) {
    FPRINTF(stderr,"ERROR:  (slv7) slv7_dump_internals\n");
    FPRINTF(stderr,"        slv7 does not dump its internals.\n");
  }
}

int32 slv7_dof_changed(slv7_system_t sys)
/**
 *** Here we will check if any fixed or included flags have
 *** changed since the last presolve.
 **/
{
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
    if( rel_included(sys->rlist[ind]) && rel_active(sys->rlist[ind]) ) {
      ++result;
    }
  }
  /* search for vars that were free and are now fixed */
  for( ind = sys->vused -1; ind >= 0; --ind ) {
    if( var_fixed(sys->vlist[ind]) || !var_active(sys->vlist[ind])) {
      ++result;
    }
  }
  /* search for rels that were included and are now unincluded */
  for( ind = sys->rused -1; ind >= 0; --ind ) {
    if( !rel_included(sys->rlist[ind]) || !rel_active(sys->rlist[ind])) {
      ++result;
    }
  }
  return result;
}

static void reset_cost(struct slv_block_cost *cost,int32 costsize)
{
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

static void slv7_update_linsolqr(slv7_system_t sys)
{
  switch (sys->p.factor_option) {
  case 0:
    sys->J.fm = ranki_kw;
    break;
  case 1:
    sys->J.fm = ranki_jz;
    break;
  case 2:
    sys->J.fm = ranki_kw2;
    break;
  case 3:
    sys->J.fm = ranki_jz2;
    break;
  default:
    sys->J.fm = ranki_kw2;
    break;
  }
  linsolqr_set_pivot_zero(sys->J.sys, sys->p.tolerance.singular);
  linsolqr_set_drop_tolerance(sys->J.sys, sys->p.tolerance.drop);
  linsolqr_set_pivot_tolerance(sys->J.sys, sys->p.tolerance.pivot);
  /* this next one is fishy, but we don't use qr so not panicking */
  linsolqr_set_condition_tolerance(sys->J.sys, sys->p.tolerance.pivot);
}

void slv7_presolve(slv_system_t server, SlvClientToken asys)
{
  struct var_variable **vp;
  struct rel_relation **rp;
  int32 cap,ind;
  int32 matrix_creation_needed = 1;
  slv7_system_t sys;

  sys = SLV7(asys);
  iteration_begins(sys);
  check_system(sys);
  if( sys->vlist == NULL ) {
    FPRINTF(stderr,"ERROR:  (slv7) slv7_presolve\n");
    FPRINTF(stderr,"        Variable list was never set.\n");
    return;
  }
  if( sys->rlist == NULL && sys->obj == NULL ) {
    FPRINTF(stderr,"ERROR:  (slv7) slv7_presolve\n");
    FPRINTF(stderr,"        Relation list and objective never set.\n");
    return;
  }

  if(sys->presolved > 0) { /* system has been presolve before */
    if(!slv7_dof_changed(sys) /*no changes in fixed or included flags*/
       && sys->p.partition == sys->J.old_partition) {
#if DEBUG
      FPRINTF(stderr,"YOU JUST AVOIDED MATRIX DESTRUCTION/CREATION\n");
#endif
      matrix_creation_needed = 0;
    }
  }

  rp=sys->rlist;
  for( ind = 0; ind < sys->rtot; ++ind ) {
    rel_set_satisfied(rp[ind],FALSE);
  }
  if( matrix_creation_needed ) {
    cap = slv_get_num_solvers_rels(SERVER);
    sys->cap = slv_get_num_solvers_vars(SERVER);
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

    sys->presolved = 1; /* full presolve recognized here */
    sys->J.old_partition = sys->p.partition;
    destroy_matrices(sys);
    destroy_vectors(sys);
    create_matrices(server,sys);
    create_vectors(sys);
    sys->s.block.current_reordered_block = -2;
  } else {
    slv7_update_linsolqr(sys);
  }

  /* Reset status */
  sys->s.iteration = 0;
  sys->s.cpu_elapsed = 0.0;
  sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
  sys->s.block.previous_total_size = 0;
  sys->s.costsize = 1+sys->s.block.number_of;
  if( matrix_creation_needed ) {
    destroy_array(sys->s.cost);
    sys->s.cost = create_zero_array(sys->s.costsize,struct slv_block_cost);
    for( ind = 0; ind < sys->s.costsize; ++ind ) {
      sys->s.cost[ind].reorder_method = -1;
    }
  } else {
    reset_cost(sys->s.cost,sys->s.costsize);
  }

  /* set to go to first unconverged block */
  sys->s.block.current_block = -1;
  sys->s.block.current_size = 0;
  sys->s.calc_ok = TRUE;
  sys->s.block.iteration = 0;
  sys->objective =  MAXDOUBLE/2000.0;

  update_status(sys);
  iteration_ends(sys);
  sys->s.cost[sys->s.block.number_of].time=sys->s.cpu_elapsed;
}

static boolean slv7_change_basis(slv7_system_t sys,int32 var,
                                 mtx_range_t *rng){
#if REIMPLEMENT
  boolean didit;
  didit = mtx_make_col_independent(sys->J.mtx,
    mtx_org_to_col(sys->J.mtx,var),rng);
  if (didit && sys->p.partition && !OPTIMIZING(sys) ) {
    struct slv_block_cost oldpresolve;
    int32 oldblocks;
    oldblocks = sys->s.block.number_of;
    oldpresolve = sys->s.cost[sys->s.costsize-1];
    mtx_partition(sys->J.mtx); /* this call needs to be replaced */
    sys->s.block.number_of =
      (slv_get_solvers_blocks(SERVER))->nblocks;
    if (oldblocks != sys->s.block.number_of) {
      ascfree(sys->s.cost);
      sys->s.costsize = sys->s.block.number_of+1;
      sys->s.cost = create_zero_array(sys->s.costsize,struct slv_block_cost);
      sys->s.cost[sys->s.costsize-1] = oldpresolve;
    }
  }
  return didit;
#else
  return 0;
#endif
}

static void slv7_resolve(slv_system_t server, SlvClientToken asys)
{
  struct var_variable **vp;
  struct rel_relation **rp;
  slv7_system_t sys;
  sys = SLV7(asys);

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

  update_status(sys);
}

static void slv7_iterate(slv_system_t server, SlvClientToken asys)
{
  slv7_system_t sys;
  FILE              *mif = MIF(sys);
  FILE              *lif = LIF(sys);
  real64      bounds_coef=1.0;
  real64      previous = 0.0;
  real64      oldphi, norm2;
  boolean           first=TRUE, bounds_ok=FALSE,
                    new_ok=FALSE, descent_ok=FALSE;
  int               minor = 0,ds_status=0;
  double            time0;
#if KDEBUG
  real64 p_norm2,un_p_norm2;
  int32 row, ind;
#endif /*KDEBUG*/
  sys = SLV7(asys);
  mif = MIF(sys);
  lif = LIF(sys);
  if (server == NULL || sys==NULL) return;
  if (check_system(SLV7(sys))) return;
  if( !sys->s.ready_to_solve ) {
    FPRINTF(stderr,"ERROR:  (slv7) slv7_iterate\n");
    FPRINTF(stderr,"        Not ready to solve.\n");
    return;
  }

  if (sys->s.block.current_block==-1) {
    find_next_unconverged_block(sys);
    update_status(sys);
    return;
  }
  if (sys->p.output.less_important && (sys->s.block.current_size >1 ||
      sys->iarray[SP7_LIFDS])) {
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
    if (sys->p.output.less_important && sys->iarray[SP7_LIFDS]) {
      FPRINTF(lif,"%-40s ---> (%d)", "Singleton relation",
              mtx_row_to_org(sys->J.mtx,sys->J.reg.row.low));
      print_rel_name(lif,sys,rel); PUTC('\n',lif);
      FPRINTF(lif,"%-40s ---> (%d)", "Singleton variable",
              mtx_col_to_org(sys->J.mtx,sys->J.reg.col.low));
      print_var_name(lif,sys,var); PUTC('\n',lif);
    }

    /* Attempt direct solve */
    time0=tm_cpu_time();
    ds_status=slv_direct_solve(SERVER,rel,var,mif,sys->p.tolerance.feasible,
                               sys->p.ignore_bounds,TERMSCALE);
    sys->s.block.functime += (tm_cpu_time()-time0);

    switch( ds_status ) {
    case 0:
      if (sys->p.output.less_important) {
        FPRINTF(lif,"Unable to directly solve.\n");
      }
      break;
    case 1:
      if (sys->p.output.less_important && sys->iarray[SP7_LIFDS]) {
        FPRINTF(lif,"Directly solved.\n");
      }
      sys->s.calc_ok = calc_residuals(sys);
      iteration_ends(sys);
      find_next_unconverged_block(sys);
      update_status(sys);
      return;
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
  } /* if fails with a 0, go on to newton a 1x1 */
  if( !calc_J(sys) ) {
    FPRINTF(MIF(sys),"NGSLV: Jacobian calculation errors detected.\n");
  }
  scale_J(sys);
  scale_variables(sys);
  scale_residuals(sys);
  if( !calc_gradient(sys) )
    FPRINTF(MIF(sys),"NGSLV: Gradient calculation errors detected.\n");

  sys->J.rank_defect=0;
  sys->J.rank_defect = calc_pivots(sys);
#if NGSLV
  if (  sys->J.rank_defect == 0 ) {
    zero_un_p_weights(sys);
  }
#else
  zero_un_p_weights(sys);
#endif
  if (sys->iarray[SP7_SAVLIN]) {
    FILE *ldat;
    sprintf(savlinfilename,"%s%d",savlinfilebase,savlinnum++);
    ldat=fopen(savlinfilename,"w");
    FPRINTF(ldat,"================= block %d, major itn %d ============\n",
      sys->s.block.current_block, sys->s.iteration);
    mtx_write_region(ldat,sys->J.mtx,&(sys->J.reg));
    fclose(ldat);
  }
  calc_B(sys);
  calc_ZBZ(sys); /* only call */
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
#if NGSLV
#if KDEBUG
   FPRINTF(stderr,"\n RANK DEFECT = %d\n",sys->J.rank_defect);
#endif /*KDEBUG*/
  if( sys->J.rank_defect > 0 ){
      calc_newton_grad_step(sys);
  } else {
      calc_newton(sys);
  }
#else /*NGSLV*/
  calc_newton(sys);
#endif /*NGSLV*/

  if (sys->residuals.norm2 > 1.0e-32) {
    norm2 = inner_product(&(sys->newton),&(sys->gamma))/sys->residuals.norm2;
    if( fabs(norm2 - 1.0) > 1e-4 ) {
      FPRINTF(MIF(sys),"WARNING:(slv7) slv7_iterate\n");
      FPRINTF(MIF(sys),"        Jacobian inverse inaccurate.\n");
      FPRINTF(MIF(sys),"        Smallest pivot = %g, JJ' norm2 = %g\n",
              linsolqr_smallest_pivot(sys->J.sys), norm2);
    }
  }
  /* if we're at the solution, who cares. in fact, why are we here? */

  calc_Bnewton(sys);
  calc_nullspace(sys); /* only call */

  calc_varstep1(sys);
  calc_Bvarstep1(sys);
  calc_varstep2(sys); /* only call */
  calc_Bvarstep2(sys);
  calc_mulstep1(sys);
  calc_mulstep2(sys);

  first = TRUE;
  oldphi = sys->phi;
#if NGSLV
  if (sys->J.rank_defect > 0) {
/* commenting out control structure while building a new one
   change_maxstep(sys, MAXDOUBLE);
    first = FALSE;
    if (!bounds_ok) {
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

      if (maxstep_coef < MIN_COEF) maxstep_coef = MIN_COEF;
      if (maxstep_coef > MAX_COEF) maxstep_coef = MAX_COEF;
      if (sys->p.output.less_important) {
	FPRINTF(lif,"%-40s ---> %g\n",
                "    Step reduction (descent not ok)",maxstep_coef);
      }
      sys->phi = oldphi;
      restore_variables(sys);
      change_maxstep(sys, maxstep_coef*sys->maxstep);

      calc_step(sys, minor);
      sys->progress = calc_sqrt_D0(
				   calc_sqrt_D0((sys->varstep.norm2+sys->mulstep.norm2)*
						(sys->varstep1.norm2+sys->mulstep1.norm2)));
      sys->maxstep = calc_sqrt_D0(sys->varstep.norm2 + sys->mulstep.norm2);
      if (sys->p.output.less_important) {
	FPRINTF(lif,"%-40s ---> %g\n",
		"    Suggested step length (scaled)", sys->maxstep);
	FPRINTF(lif,"%-40s ---> %g\n",
		"    Suggested progress", sys->progress);
      }


    }
*/ /*contol*/
    /**
     ***  Check bounds.
     **/
    bounds_ok = ((!REDUCE) || (sys->p.ignore_bounds) ||
                ((bounds_coef=required_coef_to_stay_inbounds(sys)) == 1.0));
/*    if( !bounds_ok ) {
      previous = oldphi;
      continue;
    }*/

    /**
     ***  Apply step.
     **/
    ngslv_apply_step(sys);
/*    if (sys->progress <= sys->p.tolerance.termination) {
      FPRINTF(mif,"\nProblem diverged:  Applied progress too small.\n");
      restore_variables(sys);
      sys->s.diverged = TRUE;
      iteration_ends(sys);
      update_status(sys);
      return;
    }*/

    /**
     ***  Check calculations at new point.
     **/
    new_ok = (calc_objective(sys) && calc_residuals(sys));
#if KDEBUG
    p_norm2 = D_ZERO;
    for(row = 0; row < sys->J.rank; row++){
      ind = sys->J.pivrows->idata[row];
      ind = mtx_org_to_row(sys->J.mtx,ind);
      p_norm2 = p_norm2 + sqr(sys->residuals.vec[ind]);
    }
    un_p_norm2 = D_ZERO;
    for(row = 0; row < sys->J.rank_defect; row++){
      ind = sys->J.singrows->idata[row];
      ind = mtx_org_to_row(sys->J.mtx,ind);
      un_p_norm2 = un_p_norm2 + sqr(sys->residuals.vec[ind]);
    }
    un_p_norm2 = sqrt(un_p_norm2);
    p_norm2 = sqrt(p_norm2);
/*    FPRINTF(stderr,"\nP_NORM2 = %16.8g\n\n",p_norm2);
    FPRINTF(stderr,"\nUN_P_NORM2 = %16.8g\n\n",un_p_norm2);
    FPRINTF(stderr,"%16.8g ",p_norm2);
    FPRINTF(stderr,"%16.8g ",un_p_norm2);   */

/*    out = fopen("OUT","a");
         FPRINTF(out,"\nGRAD MULT = %16.8g\n",sys->rarray[SP7_GMULT]);
	 FPRINTF(out,"RANK = %d, RANK DEFECT = %d\n",sys->J.rank,sys->J.rank_defect);*/
/*	 FPRINTF(stderr,"P_ERROR = %16.8g, UN_P_ERROR = %16.8g\n",p_norm2,un_p_norm2);*/
	 FPRINTF(stderr,"P_NORM = %16.8g, UN_P_NORM = %16.8g\n",p_norm2,un_p_norm2);
/*    FPRINTF(out,"%16.8g ",sys->rarray[SP7_GMULT]);
    FPRINTF(out,"%d %d ",sys->J.rank,sys->J.rank_defect);
    FPRINTF(out,"%16.8g %16.8g ",p_norm2,un_p_norm2);
    FPRINTF(out,"%16.8g\n",sqrt(sqr(p_norm2) + sqr(un_p_norm2)));
    fclose(out);*/
#endif /*KDEBUG*/
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
/*    if( !new_ok ) {
      previous = oldphi;
      continue;
    }*/

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
    if (EXACT_LINE_SEARCH) {
      descent_ok = (descent_ok && (sys->phi >= previous));
    }

  } else {
# endif /*NGSLV*/

  while (first || !bounds_ok || !new_ok || !descent_ok) {

    minor++;

    if (first) {
      change_maxstep(sys, MAXDOUBLE);
      first = FALSE;
    } else {
      if (!bounds_ok) {
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
      } else {
        if (!new_ok) {
          real64 maxstep_coef;
          maxstep_coef = 0.50;
          if (sys->p.output.less_important) {
            FPRINTF(lif,"%-40s ---> %g\n",
              "    Step reduction (calculations not ok)", maxstep_coef);
          }
          restore_variables(sys);
          change_maxstep(sys, maxstep_coef*sys->maxstep);
        } else {
          if (!descent_ok) {
            real64 maxstep_coef;
            previous = MIN(sys->phi, oldphi);
            if( OPTIMIZING(sys) ) {
              maxstep_coef = 0.5;
            } else {
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
        }
      }
    }

    calc_step(sys, minor);
    sys->progress = calc_sqrt_D0(
      calc_sqrt_D0((sys->varstep.norm2+sys->mulstep.norm2)*
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
  } /* end of big while loop */
#if NGSLV
  }
#endif /*NGSLV*/

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
#if DEBUG
      FPRINTF(stderr,"******end of iteration*************\n");
      debug_out_var_values(LIF(sys), sys);
      debug_out_rel_residuals(LIF(sys), sys);
      FPRINTF(stderr,"***********************************\n");
#endif
/* NGSLV KHACK FOR TESTING ONLY */
  if (sys->rarray[SP7_GMULT] < 0 && sys->s.block.current_size > 100) {
    FPRINTF(stderr,"\nNGSLV KHACK FOR TESTING ONLY\n\n");
    restore_variables(sys);
  }


  iteration_ends(sys);
  if( !OPTIMIZING(sys) && block_feasible(sys) )  {
    if (sys->J.rank_defect) {
      FPRINTF(mif,"Block %d singular one step before convergence.\n",
        sys->s.block.current_block);
      FPRINTF(mif,
        "You may wish to check for numeric dependency at solution.\n");
    }
    find_next_unconverged_block(sys);
  }
  update_status(sys);
}


static void slv7_solve(slv_system_t server, SlvClientToken asys)
{
  slv7_system_t sys;
  sys = SLV7(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(sys)) return;
  while( sys->s.ready_to_solve ) slv7_iterate(server,sys);
}

static mtx_matrix_t slv7_get_jacobian(slv_system_t server, SlvClientToken sys)
{
  if (server == NULL || sys==NULL) return NULL;
  if (check_system(SLV7(sys))) return NULL;
  return SLV7(sys)->J.mtx;
}

static int slv7_destroy(slv_system_t server, SlvClientToken asys)
{
  slv7_system_t sys;
  sys = SLV7(asys);
  if (check_system(sys)) return 1;
  destroy_matrices(sys);
  destroy_vectors(sys);
  sys->integrity = DESTROYED;
  if (sys->s.cost) ascfree(sys->s.cost);
  ascfree( (POINTER)asys );
  return 0;
}

int slv7_register(SlvFunctionsT *sft)
{
  if (sft==NULL)  {
    FPRINTF(stderr,"slv7_register called with NULL pointer\n");
    return 1;
  }

  sft->name = "NGSlv";
  sft->ccreate = slv7_create;
  sft->cdestroy = slv7_destroy;
  sft->celigible = slv7_eligible_solver;
  sft->getparam = slv7_get_parameters;
  sft->setparam = slv7_set_parameters;
  sft->getstatus = slv7_get_status;
  sft->solve = slv7_solve;
  sft->presolve = slv7_presolve;
  sft->iterate = slv7_iterate;
  sft->resolve = slv7_resolve;
  sft->getlinsol = slv7_get_linsol_sys;
  sft->getlinsys = slv7_get_linsolqr_sys;
  sft->getsysmtx = slv7_get_jacobian;
  sft->dumpinternals = slv7_dump_internals;
  return 0;
}
#endif /* #else clause of DYNAMIC_NGSLV */
#endif /* #else clause of !STATIC_NGSLV && !DYNAMIC_NGSLV */
