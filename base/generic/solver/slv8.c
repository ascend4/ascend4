/*
 *  Incorporation of the nonlinear solver CONOPT to ASCEND
 *  by Ken Tyner
 *  Created: 6/97
 *  Version: $Revision: 1.31 $
 *  Version control file: $RCSfile: slv8.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:50 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/set.h>
#include <general/tm_time.h>
#include <utilities/mem.h>
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
#include "conopt.h"
#include "slv8.h"
#include "slv_stdcalls.h"
#include "conoptdll.h"

#define slv8_register_conopt_function register_conopt_function
#define slv8_coicsm coicsm
#define slv8_coimem coimem

#if !defined(STATIC_CONOPT) && !defined(DYNAMIC_CONOPT)

int slv8_register(SlvFunctionsT *f)
{
  (void)f;  /* stop gcc whine about unused parameter */

  FPRINTF(stderr,"CONOPT not compiled in this ASCEND IV.\n");
  return 1;
}

#else /* either STATIC_CONOPT or DYNAMIC_CONOPT is defined */

/* #ifdef DYNAMIC_CONOPT */
/* do dynamic loading stuff.   yeah, right */

/* #else *//* following is used if STATIC_CONOPT is defined */

/*
 * Output in user defined CONOPT subroutines
 */
#define CONDBG 0
#define NONBASIC_DEBUG FALSE

/*
 * makes lots of extra spew
 */
#define DEBUG FALSE

#define SLV8(s) ((slv8_system_t)(s))
#define MI8F(s) slv_get_output_file( SLV8(s)->p.output.more_important )
#define SERVER (sys->slv)
#define slv8_PA_SIZE 33
#define SAFE_CALC_PTR (sys->parm_array[0])
#define SAFE_CALC     ((*(int *)SAFE_CALC_PTR))
#define SCALEOPT_PTR (sys->parm_array[1])
#define SCALEOPT     ((*(char **)SCALEOPT_PTR))
#define TOO_SMALL_PTR (sys->parm_array[2])
#define TOO_SMALL     ((*(real64 *)TOO_SMALL_PTR))
#define UPDATE_NOMINALS_PTR (sys->parm_array[3])
#define UPDATE_NOMINALS     ((*(int *)UPDATE_NOMINALS_PTR))
#define UPDATE_RELNOMS_PTR (sys->parm_array[4])
#define UPDATE_RELNOMS     ((*(int *)UPDATE_RELNOMS_PTR))
#define UPDATE_WEIGHTS_PTR (sys->parm_array[5])
#define UPDATE_WEIGHTS     ((*(int *)UPDATE_WEIGHTS_PTR))
#define DUMPCNORM_PTR (sys->parm_array[6])
#define DUMPCNORM     ((*(int *)DUMPCNORM_PTR))
#define CNLOW_PTR (sys->parm_array[7])
#define CNLOW     ((*(real64 *)CNLOW_PTR))
#define CNHIGH_PTR (sys->parm_array[8])
#define CNHIGH     ((*(real64 *)CNHIGH_PTR))
#define UPDATE_JACOBIAN_PTR   (sys->parm_array[9])
#define UPDATE_JACOBIAN       ((*(int *)UPDATE_JACOBIAN_PTR))
#define ITSCALELIM_PTR (sys->parm_array[10])
#define ITSCALELIM     ((*(int *)ITSCALELIM_PTR))
#define ITSCALETOL_PTR (sys->parm_array[11])
#define ITSCALETOL     ((*(real64 *)ITSCALETOL_PTR))
#define LIFDS_PTR (sys->parm_array[12])
#define LIFDS     ((*(int32 *)LIFDS_PTR))
#define REORDER_OPTION_PTR (sys->parm_array[13])
#define REORDER_OPTION     ((*(char **)REORDER_OPTION_PTR))
#define CUTOFF_PTR (sys->parm_array[14])
#define CUTOFF     ((*(int32 *)CUTOFF_PTR))
#define RELNOMSCALE_PTR (sys->parm_array[15])
#define RELNOMSCALE     ((*(int *)RELNOMSCALE_PTR))
#define ITER_LIMIT_PTR (sys->parm_array[16])
#define ITER_LIMIT     ((*(int32 *)ITER_LIMIT_PTR))
#define TIME_LIMIT_PTR (sys->parm_array[17])
#define TIME_LIMIT     ((*(int32 *)TIME_LIMIT_PTR))
#define DOMLIM_PTR (sys->parm_array[18])
#define DOMLIM     ((*(int32 *)DOMLIM_PTR))
#define RTMAXJ_PTR (sys->parm_array[19])
#define RTMAXJ     ((*(real64 *)RTMAXJ_PTR))

/*
 * Auxiliar structures
 */

struct update_data {
  int                    jacobian;   /* Countdown on jacobian updating */
  int                    weights;    /* Countdown on weights updating */
  int                    nominals;   /* Countdown on nominals updating */
  int                    relnoms;    /* Countdown on relnom updating */
  int                    iterative;  /* Countdown on iterative scale update */
};


/*
 * varpivots, relpivots used only in optimizing, if we rewrite calc_pivots
 * without them.
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
  enum factor_method     fm;		/* Linear factorization method */
  boolean                accurate;     /* ? Recalculate matrix */
  boolean                singular;     /* ? Can matrix be inverted */
  boolean                old_partition;     /* old value of partition flag */
};

struct slv8_system_structure {

  /*
   *  Problem definition
   */
  slv_system_t   	      slv;     /* slv_system_t back-link */
  struct rel_relation         *obj;    /* Objective function: NULL = none */
  struct rel_relation         *old_obj;/* Objective function: NULL = none */
  struct var_variable         **vlist; /* Variable list (NULL terminated) */
  struct rel_relation         **rlist; /* Relation list (NULL terminated) */

  /*
   *  Solver information
   */
  int                    integrity;    /* ? Has the system been created */
  int32                  presolved;    /* ? Has the system been presolved */
  int32                  resolve;      /* ? Has the system been resolved */
  slv_parameters_t       p;            /* Parameters */
  slv_status_t           s;            /* Status (as of iteration end) */
  struct update_data     update;       /* Jacobian frequency counters */
  int32                  cap;          /* Order of matrix/vectors */
  int32                  rank;         /* Symbolic rank of problem */
  int32                  vused;        /* Free and incident variables */
  int32                  vtot;         /* length of varlist */
  int32                  rused;        /* Included relations */
  int32                  rtot;         /* length of rellist */
  double                 clock;        /* CPU time */

  void *parm_array[slv8_PA_SIZE];
  struct slv_parameter pa[slv8_PA_SIZE];

  /*
   *  CONOPT DATA
   */
  struct conopt_data con;

  /*
   *  Calculated data (scaled)
   */
  struct jacobian_data   J;            /* linearized system */

  struct vector_data     nominals;     /* Variable nominals */
  struct vector_data     weights;      /* Relation weights */
  struct vector_data     relnoms;      /* Relation nominals */
  struct vector_data     variables;    /* Variable values */
  struct vector_data     residuals;    /* Relation residuals */

  real64           objective;    /* Objective function evaluation */
};


/*
 *  Integrity checks
 *  ----------------
 *  check_system(sys)
 */

#define OK        ((int32)813029392)
#define DESTROYED ((int32)103289182)

/*
 *  Checks sys for NULL and for integrity.
 */
static int check_system(slv8_system_t sys)
{
  if( sys == NULL ) {
    FPRINTF(stderr,"ERROR:  (slv8) check_system\n");
    FPRINTF(stderr,"        NULL system handle.\n");
    return 1;
  }

  switch( sys->integrity ) {
  case OK:
    return 0;
  case DESTROYED:
    FPRINTF(stderr,"ERROR:  (slv8) check_system\n");
    FPRINTF(stderr,"        System was recently destroyed.\n");
    return 1;
  default:
    FPRINTF(stderr,"ERROR:  (slv8) check_system\n");
    FPRINTF(stderr,"        System reused or never allocated.\n");
    return 1;
  }
}

/*
 *  General input/output routines
 *  -----------------------------
 *  print_var_name(out,sys,var)
 *  print_rel_name(out,sys,rel)
 */

#define print_var_name(a,b,c) slv_print_var_name((a),(b)->slv,(c))
#define print_rel_name(a,b,c) slv_print_rel_name((a),(b)->slv,(c))

/*
 *  Debug output routines
 *  ---------------------
 *  debug_delimiter(fp)
 *  debug_out_vector(fp,vec)
 *  debug_out_var_values(fp,sys)
 *  debug_out_rel_residuals(fp,sys)
 *  debug_out_jacobian(fp,sys)
 *  debug_out_hessian(fp,sys)
 */

/*
 *  Outputs a hyphenated line.
 */
static void debug_delimiter( FILE *fp)
{
  int32 i;
  for( i=0; i<60; i++ ) PUTC('-',fp);
  PUTC('\n',fp);
}

#if DEBUG

/*
 *  Outputs a vector.
 */
static void debug_out_vector( FILE *fp, slv8_system_t sys,
                              struct vector_data *vec)
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

/*
 *  Outputs all variable values in current block.
 */
static void debug_out_var_values( FILE *fp, slv8_system_t sys)
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

/*
 *  Outputs all relation residuals in current block.
 */
static void debug_out_rel_residuals( FILE *fp, slv8_system_t sys)
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


/*
 *  Outputs permutation and values of the nonzero elements in the
 *  the jacobian matrix.
 */
static void debug_out_jacobian( FILE *fp, slv8_system_t sys)
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

/*
 *  Outputs permutation and values of the nonzero elements in the
 *  reduced hessian matrix.
 */
static void debug_out_hessian( FILE *fp, slv8_system_t sys)
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

#endif /* DEBUG */


/*
 *  Array/vector operations
 *  ----------------------------
 *  destroy_array(p)
 *  create_array(len,type)
 *
 *  zero_vector(vec)
 *  copy_vector(vec1,vec2)
 *  prod = inner_product(vec1,vec2)
 *  norm2 = square_norm(vec)
 *  matrix_product(mtx,vec,prod,scale,transpose)
 */

#define destroy_array(p)  \
   if( (p) != NULL ) ascfree((p))
#define create_array(len,type)  \
   ((len) > 0 ? (type *)ascmalloc((len)*sizeof(type)) : NULL)
#define create_zero_array(len,type)  \
   ((len) > 0 ? (type *)asccalloc((len),sizeof(type)) : NULL)

#define zero_vector(v) slv_zero_vector(v)
#define copy_vector(v,t) slv_copy_vector((v),(t))
#define inner_product(v,u) slv_inner_product((v),(u))
#define square_norm(v)  slv_square_norm(v)
#define matrix_product(m,v,p,s,t) slv_matrix_product((m),(v),(p),(s),(t))


/*
 *  Calculation routines
 *  --------------------
 *  ok = calc_objective(sys)
 *  ok = calc_residuals(sys)
 *  ok = calc_J(sys)
 *  calc_nominals(sys)
 *  calc_weights(sys)
 *  scale_J(sys)
 *  scale_variables(sys)
 *  scale_residuals(sys)
 */

/*
 * counts jacobian elements and sets max to the number of elements
 * in the densest row
 */
static int32 num_jacobian_nonzeros(slv8_system_t sys, int32 *max)
{
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
  row_max = sys->con.m;
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


/*
 *  Evaluate the objective function.
 */
static boolean calc_objective( slv8_system_t sys)
{
  calc_ok = TRUE;
  sys->objective =
    (sys->obj ? relman_eval(sys->obj,&calc_ok,SAFE_CALC) : 0.0);
  return calc_ok;
}


/*
 *  Evaluate all objectives.
 */
static boolean calc_objectives( slv8_system_t sys)
{
  int32 len,i;
  static rel_filter_t rfilter;
  struct rel_relation **rlist=NULL;
  rfilter.matchbits = (REL_INCLUDED);
  rfilter.matchvalue =(REL_INCLUDED);
  rlist = slv_get_solvers_obj_list(SERVER);
  len = slv_get_num_solvers_objs(SERVER);
  calc_ok = TRUE;
  for (i = 0; i < len; i++) {
    if (rel_apply_filter(rlist[i],&rfilter)) {
      relman_eval(rlist[i],&calc_ok,SAFE_CALC);
#if DEBUG
      if (calc_ok == FALSE) {
	FPRINTF(stderr,"error in calc_objectives\n");
	calc_ok = TRUE;
      }
#endif /* DEBUG */
    }
  }
  return calc_ok;
}

/*
 *  Calculates all of the residuals in the current block and computes
 *  the residual norm for block status.  Returns true iff calculations
 *  preceded without error.
 */
static boolean calc_residuals( slv8_system_t sys)
{
  int32 row;
  struct rel_relation *rel;
  double time0;

  if( sys->residuals.accurate ) return TRUE;

  calc_ok = TRUE;
  row = sys->residuals.rng->low;
  time0=tm_cpu_time();
  for( ; row <= sys->residuals.rng->high; row++ ) {
    rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
#if DEBUG
    if (!rel) {
      int32r;
      r=mtx_row_to_org(sys->J.mtx,row);
      FPRINTF(MIF(sys),"NULL relation found !!\n");
      FPRINTF(MIF(sys),"at row %d rel %d in calc_residuals\n",(int)row,r);
      FFLUSH(MIF(sys));
    }
#endif /* DEBUG */
    sys->residuals.vec[row] = relman_eval(rel,&calc_ok,SAFE_CALC);

    relman_calc_satisfied(rel,sys->p.tolerance.feasible);
  }
  sys->s.block.functime += (tm_cpu_time() -time0);
  sys->s.block.funcs++;
  square_norm( &(sys->residuals) );
  sys->s.block.residual = calc_sqrt_D0(sys->residuals.norm2);
  return(calc_ok);
}


/*
 *  Calculates the current block of the jacobian.
 *  It is initially unscaled.
 */
static boolean calc_J( slv8_system_t sys)
{
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
  sys->s.block.jactime += (tm_cpu_time() - time0);
  sys->s.block.jacs++;

  if( --(sys->update.nominals) <= 0 ) sys->nominals.accurate = FALSE;
  if( --(sys->update.weights) <= 0 ) sys->weights.accurate = FALSE;

  return(calc_ok);
}

/*
 *  Retrieves the nominal values of all of the block variables,
 *  insuring that they are all strictly positive.
 */
static void calc_nominals( slv8_system_t sys)
{
  int32 col;
  FILE *fp = MIF(sys);
  if( sys->nominals.accurate ) return;
  fp = MIF(sys);
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
	  FPRINTF(fp,"ERROR:  (slv8) calc_nominals\n");
	  FPRINTF(fp,"        Variable ");
	  print_var_name(fp,sys,var);
	  FPRINTF(fp,"        \nhas nominal value of zero.\n");
	  FPRINTF(fp,"        Resetting to %g.\n",n);
	  var_set_nominal(var,n);
	} else {
	  n =  -n;
	  FPRINTF(fp,"ERROR:  (slv8) calc_nominals\n");
	  FPRINTF(fp,"        Variable ");
	  print_var_name(fp,sys,var);
	  FPRINTF(fp,"        \nhas negative nominal value.\n");
	  FPRINTF(fp,"        Resetting to %g.\n",n);
	  var_set_nominal(var,n);
	}
      }
#if DEBUG
      FPRINTF(fp,"Column %d is");
      print_var_name(fp,sys,var);
      FPRINTF(fp,"\nScaling of column %d is %g\n",col,n);
#endif /* DEBUG */
      sys->nominals.vec[col] = n;
    }
  }
  square_norm( &(sys->nominals) );
  sys->update.nominals = UPDATE_NOMINALS;
  sys->nominals.accurate = TRUE;
}


/*
 *  Calculates the weights of all of the block relations
 *  to scale the rows of the Jacobian.
 */
static void calc_weights( slv8_system_t sys)
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


/*
 *  Scales the jacobian.
 */
static void scale_J( slv8_system_t sys)
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
}

static void jacobian_scaled(slv8_system_t sys)
{
  int32 col;
  if (DUMPCNORM) {
    for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ ) {
      real64 cnorm;
      cnorm =
        calc_sqrt_D0(mtx_sum_sqrs_in_col(sys->J.mtx,col,&(sys->J.reg.row)));
      if (cnorm >CNHIGH || cnorm <CNLOW) {
        FPRINTF(MIF(sys),"[col %d org %d] %g\n", col,
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
#endif /* DEBUG */
}



static void scale_variables( slv8_system_t sys)
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
#endif /* DEBUG */
}


/*
 *  Scales the previously calculated residuals.
 */
static void scale_residuals( slv8_system_t sys)
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
#endif  /* DEBUG */
}


/*
 * Calculates relnoms for all relations in sys
 * using variable nominals.
 */
static void calc_relnoms(slv8_system_t sys)
{
  int32 row, col;
  struct var_variable *var;
  struct rel_relation *rel;
  real64 *var_list;
  var_list = create_array(sys->cap,real64);
  col = 0;
  var = sys->vlist[col];
  /* store current variable values and
     set variable value to nominal value */
  while(var != NULL){
    var_list[col] = var_value(var);
    var_set_value(var, var_nominal(var));
    col++;
    var = sys->vlist[col];
  }
  row = 0;
  rel = sys->rlist[row];
  /* calculate relation nominal */
  while(rel != NULL){
    relman_scale(rel);
    row++;
    rel = sys->rlist[row];
  }
  col = 0;
  var = sys->vlist[col];
  /* restore variable values */
  while(var != NULL){
    var_set_value(var, var_list[col]);
    col++;
    var = sys->vlist[col];
  }
  destroy_array(var_list);
}

/*
 *  Returns the maximum ratio of magnitudes of any two nonzero
 *  elements in the same column of mtx.  Only considers elements
 *  in region reg.
 */
static real64 col_max_ratio(mtx_matrix_t *mtx,
			    mtx_region_t *reg)
{
  real64 ratio;
  real64 max_ratio;
  real64 num, denom, dummy;
  mtx_coord_t coord;
  max_ratio = 0;
  for(coord.col = reg->col.low;
	coord.col <= reg->col.high; coord.col++) {
    ratio = 0;
    num = mtx_col_max(*mtx,&(coord),&(reg->row),&(dummy));
    denom = mtx_col_min(*mtx,&(coord),&(reg->row),&(dummy),1e-7);
    if(denom >0){
      ratio = num/denom;
    }
    if(ratio > 10000000){
/*      FPRINTF(stderr,"HELPME\n");*/
    }
    if(ratio > max_ratio){
      max_ratio = ratio;
    }
  }
  if(max_ratio == 0){
    max_ratio = 1;
  }
  return max_ratio;
}


/*
 *  Returns the maximum ratio of magnitudes of any two nonzero
 *  elements in the same row of mtx.  Only considers elements
 *  in region reg.
 */
static real64 row_max_ratio(mtx_matrix_t *mtx,
			    mtx_region_t *reg)
{
  real64 ratio;
  real64 max_ratio;
  real64 num, denom, dummy;
  mtx_coord_t coord;
  max_ratio = 0;
  for(coord.row = reg->row.low;
	coord.row <= reg->row.high; coord.row++) {
    ratio = 0;
    num = mtx_row_max(*mtx,&(coord),&(reg->col),&(dummy));
    denom = mtx_row_min(*mtx,&(coord),&(reg->col),&(dummy),1e-7);
    if(denom >0){
      ratio = num/denom;
    }
    if(ratio > 10000000){
/*      FPRINTF(stderr,"HELPME\n");*/
    }
    if(ratio > max_ratio){
      max_ratio = ratio;
    }
  }
  if(max_ratio == 0){
    max_ratio = 1;
  }
  return max_ratio;
}

/*
 *  Calculates scaling factor suggested by Fourer.
 *  For option = 0, returns scaling factor for
 *  row number loc.
 *  For option = 1, returns scaling factor for
 *  col number loc.
 */
static real64 calc_fourer_scale(mtx_matrix_t mtx,
			      mtx_region_t reg,
			      int32 loc,
			      int32 option)
{
  mtx_coord_t coord;
  real64 min, max, dummy;
  real64 scale;
  if(option == 0){
    if((loc < reg.row.low) || (loc > reg.row.high)){
      return 1;
    }
    coord.row = loc;
    min = mtx_row_min(mtx,&(coord),&(reg.col),&(dummy),1e-7);
    max = mtx_row_max(mtx,&(coord),&(reg.col),&(dummy));
    scale = min*max;
    if(scale > 0){
      scale = sqrt(scale);
    } else {
      scale = 1;
    }
    return scale;
  } else {
    if(loc < reg.col.low || loc > reg.col.high){
      return 1;
    }
    coord.col = loc;
    min = mtx_col_min(mtx,&(coord),&(reg.row),&(dummy),1e-7);
    max = mtx_col_max(mtx,&(coord),&(reg.row),&(dummy));
    scale = min*max;
    if(scale > 0){
      scale = sqrt(scale);
    } else {
      scale = 1;
    }
    return scale;
  }
}

/*
 *  This funcion is an implementation of the scaling
 *  routine by Fourer on p304 of Mathematical Programing
 *  vol 23, (1982).
 *  This function will scale the Jacobian and store the scaling
 *  factors in sys->nominals and sys->weights.
 *  If the Jacobian has been previously scaled
 *  by another method (during this iteration) then these vectors
 *  should contain the scale factors used in that scaling.
 */
static void scale_J_iterative(slv8_system_t sys)
{
  real64 rho_col_old, rho_col_new;
  real64 rho_row_old, rho_row_new;
  int32 k;
  int32 done;
  int32 row, col;
  real64 *colvec = sys->nominals.vec;
  real64 *rowvec = sys->weights.vec;
  real64 rowscale, colscale;
  rho_col_old = col_max_ratio(&(sys->J.mtx),&(sys->J.reg));
  rho_row_old = row_max_ratio(&(sys->J.mtx),&(sys->J.reg));
  k = 0;
  done = 0;
  while(done == 0){
    k++;
    for(row = sys->J.reg.row.low;
	row <= sys->J.reg.row.high; row++){
      rowscale = 1/calc_fourer_scale(sys->J.mtx,sys->J.reg,row,0);
      mtx_mult_row(sys->J.mtx,row,rowscale,&(sys->J.reg.col));
      rowvec[row] *= rowscale;
    }
    for(col = sys->J.reg.col.low;
	col <= sys->J.reg.col.high; col++){
      colscale = 1/calc_fourer_scale(sys->J.mtx,sys->J.reg,col,1);
      mtx_mult_col(sys->J.mtx,col,colscale,&(sys->J.reg.row));
      colvec[col] *= colscale;
    }
    rho_col_new = col_max_ratio(&(sys->J.mtx),&(sys->J.reg));
    rho_row_new = row_max_ratio(&(sys->J.mtx),&(sys->J.reg));
    if((rho_col_new >= ITSCALETOL*rho_col_old &&
	rho_row_new >= ITSCALETOL*rho_row_old)
       || k >= ITSCALELIM){
      done = 1;
/*      FPRINTF(MIF(sys),"%d ITERATIVE SCALING ITERATIONS\n",k);*/
    } else {
      rho_row_old = rho_row_new;
      rho_col_old = rho_col_new;
    }
  }
  square_norm( &(sys->nominals) );
  sys->update.nominals = UPDATE_NOMINALS;
  sys->nominals.accurate = TRUE;

  square_norm( &(sys->weights) );
  sys->update.weights = UPDATE_WEIGHTS;
  sys->residuals.accurate = FALSE;
  sys->weights.accurate = TRUE;
}


/*
 * Scale system dependent on interface parameters
 */
static void scale_system( slv8_system_t sys )
{
  if(strcmp(SCALEOPT,"NONE") == 0){
    if(sys->J.accurate == FALSE){
      calc_nominals(sys);
      calc_weights(sys);
      jacobian_scaled(sys);
    }
    scale_variables(sys);
    scale_residuals(sys);
    return;
  }
  if(strcmp(SCALEOPT,"ROW_2NORM") == 0 ||
     strcmp(SCALEOPT,"RELNOM") == 0){
    if(sys->J.accurate == FALSE){
      scale_J(sys);
      jacobian_scaled(sys);
    }
    scale_variables(sys);
    scale_residuals(sys);
    return;
  }
  if(strcmp(SCALEOPT,"2NORM+ITERATIVE") == 0 ||
     strcmp(SCALEOPT,"RELNOM+ITERATIVE") == 0){
    if(sys->J.accurate == FALSE){
      --sys->update.iterative;
      if(sys->update.iterative <= 0) {
	scale_J(sys);
	scale_J_iterative(sys);
	sys->update.iterative =
	  UPDATE_WEIGHTS < UPDATE_NOMINALS ? UPDATE_WEIGHTS : UPDATE_NOMINALS;
      } else {
	sys->weights.accurate = TRUE;
	sys->nominals.accurate = TRUE;
	scale_J(sys); /* will use current scaling vectors */
      }
      jacobian_scaled(sys);
    }
    scale_variables(sys);
    scale_residuals(sys);
    return;
  }
  if(strcmp(SCALEOPT,"ITERATIVE") == 0){
    if(sys->J.accurate == FALSE){
      --sys->update.iterative;
      if(sys->update.iterative <= 0) {
	calc_nominals(sys);
	calc_weights(sys);
	scale_J_iterative(sys);
	sys->update.iterative =
	  UPDATE_WEIGHTS < UPDATE_NOMINALS ? UPDATE_WEIGHTS : UPDATE_NOMINALS;
      } else {
	sys->weights.accurate = TRUE;
	sys->nominals.accurate = TRUE;
	scale_J(sys); /* will use current scaling vectors */
      }
      jacobian_scaled(sys);
    }
    scale_variables(sys);
    scale_residuals(sys);
  }
  return;
}


/*
 *  Resets all flags to setup a new solve.
 *  Should set sys->s.block.current_block = -1
 *  before calling.
 *  This is currently a HACK!
 *  not sure if should call when done.
 */
static void conopt_initialize( slv8_system_t sys)
{

  sys->s.block.current_block++;
  /*
   * Next line was added to create the aray cost, whis is used by
   * the interface to display residuals and number of iterations
   */
  sys->s.costsize = 1+sys->s.block.number_of;

  if( sys->s.block.current_block < sys->s.block.number_of ) {
    boolean ok;

    sys->s.block.iteration = 0;
    sys->s.block.cpu_elapsed = 0.0;
    sys->s.block.functime = 0.0;
    sys->s.block.jactime = 0.0;
    sys->s.block.funcs = 0;
    sys->s.block.jacs = 0;

    sys->s.calc_ok = TRUE;

    if(sys->p.output.less_important && (LIFDS ||
      sys->s.block.current_size > 1)) {
      debug_delimiter(LIF(sys));
      debug_delimiter(LIF(sys));
    }
    if(sys->p.output.less_important && LIFDS) {
      FPRINTF(LIF(sys),"\n%-40s ---> %d in [%d..%d]\n",
              "Current block number", sys->s.block.current_block,
              0, sys->s.block.number_of-1);
      FPRINTF(LIF(sys),"%-40s ---> %d\n", "Current block size",
        sys->s.block.current_size);
    }
    if( !(ok = calc_objective(sys)) ) {
         FPRINTF(MIF(sys),"Objective calculation errors detected.\n");
    }
    if(sys->p.output.less_important && sys->obj) {
      FPRINTF(LIF(sys),"%-40s ---> %g\n", "Objective", sys->objective);
    }
    sys->s.calc_ok = sys->s.calc_ok && ok;

    if (!(sys->p.ignore_bounds) ) {
      slv_insure_bounds(SERVER, sys->J.reg.col.low,
                        sys->J.reg.col.high,MIF(sys));
    }

    sys->residuals.accurate = FALSE;
    if( !(ok = calc_residuals(sys)) ) {
      FPRINTF(MIF(sys),"Residual calculation errors detected.\n");
    }
    if( sys->p.output.less_important &&
        (sys->s.block.current_size >1 ||
         LIFDS) ) {
      FPRINTF(LIF(sys),"%-40s ---> %g\n", "Residual norm (unscaled)",
        sys->s.block.residual);
    }
    sys->s.calc_ok = sys->s.calc_ok && ok;

    /* Must be updated as soon as required */
    sys->J.accurate = FALSE;
    sys->update.weights = 0;
    sys->update.nominals = 0;
    sys->update.relnoms = 0;
    sys->update.iterative = 0;
    sys->variables.accurate = FALSE;
  }
}


/*
 *  Iteration begin/end routines
 *  ----------------------------
 *  iteration_begins(sys)
 *  iteration_ends(sys)
 */

/*
 *  Prepares sys for entering an iteration, increasing the iteration counts
 *  and starting the clock.
 */
static void iteration_begins( slv8_system_t sys)
{
   sys->clock = tm_cpu_time();
   ++(sys->s.block.iteration);
   ++(sys->s.iteration);
   if(sys->p.output.less_important && LIFDS) {
     FPRINTF(LIF(sys),"\n%-40s ---> %d\n",
             "Iteration", sys->s.block.iteration);
     FPRINTF(LIF(sys),"%-40s ---> %d\n",
             "Total iteration", sys->s.iteration);
   }
}


/*
 *  Prepares sys for exiting an iteration, stopping the clock and recording
 *  the cpu time.
 */
static void iteration_ends( slv8_system_t sys)
{
   double cpu_elapsed;   /* elapsed this iteration */

   cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
   sys->s.block.cpu_elapsed += cpu_elapsed;
   sys->s.cpu_elapsed += cpu_elapsed;
   if(sys->p.output.less_important && LIFDS) {
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Elapsed time", sys->s.block.cpu_elapsed);
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Total elapsed time", sys->s.cpu_elapsed);
   }
}


/*
 *  Updates the solver status.
 */
static void update_status( slv8_system_t sys)
{
   boolean unsuccessful;

   if( !sys->s.converged ) {
      sys->s.time_limit_exceeded =
         (sys->s.block.cpu_elapsed >= TIME_LIMIT);
      sys->s.iteration_limit_exceeded =
         (sys->s.block.iteration >= ITER_LIMIT);
   }

   unsuccessful = sys->s.diverged || sys->s.inconsistent ||
      sys->s.iteration_limit_exceeded || sys->s.time_limit_exceeded;

   sys->s.ready_to_solve = !unsuccessful && !sys->s.converged;
   sys->s.ok = !unsuccessful && sys->s.calc_ok && !sys->s.struct_singular;
}


static
int32 slv8_get_default_parameters(slv_system_t server, SlvClientToken asys,
                                  slv_parameters_t *parameters)
{
  slv8_system_t sys;
  union parm_arg lo,hi,val;
  struct slv_parameter *new_parms = NULL;
  static char *reorder_names[] = {
    "SPK1","TEAR_DROP","OVER_TEAR"
  };
  static char *scaling_names[] = {
    "NONE","ROW_2NORM","RELNOM" /*,"2NORM+ITERATIVE",
    "RELNOM+ITERATIVE","ITERATIVE" */
  };
  int32 make_macros = 0;
  if (server != NULL && asys != NULL) {
    sys = SLV8(asys);
    make_macros = 1;
  }

#if DEBUG  /* keep purify from whining on UMR */
  lo.argr = hi.argr = val.argr = 0.0;
#endif  /* DEBUG */

  if (parameters->parms == NULL) {
    /* an external client wants our parameter list.
     * an instance of slv8_system_structure has this pointer
     * already set in slv8_create
     */
    new_parms = (struct slv_parameter *)
      ascmalloc((slv8_PA_SIZE)*sizeof(struct slv_parameter));
    if (new_parms == NULL) {
      return -1;
    }
    parameters->parms = new_parms;
    parameters->dynamic_parms = 1;
  }
  parameters->num_parms = 0;

  /* begin defining parameters */

  slv_define_parm(parameters, real_parm,
	       "infinity","RTMAXV","Internal value of infinity",
	       U_p_real(val,10e20),U_p_real(lo,10),U_p_real(hi,MAX_REAL),2);

  slv_define_parm(parameters, real_parm,
	       "maxjac","RTMAXJ","Maximum Jacobian Element",
	       U_p_real(val,2e8),U_p_real(lo,10),U_p_real(hi,MAX_REAL),2);
  SLV_RPARM_MACRO(RTMAXJ_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "hessian_ub","RTMXJ2","upper bound on 2nd derivatives",
	       U_p_real(val,1e4),U_p_real(lo,0),U_p_real(hi,MAX_REAL),2);

  slv_define_parm(parameters, real_parm,
	       "maxfeastol", "RTNWMA",
	       "max residual considered feasible (may be scaled)",
	       U_p_real(val, 1e-3),U_p_real(lo, 1e-13),U_p_real(hi,10e10),2);

  slv_define_parm(parameters, real_parm,
	       "minfeastol", "RTNWMI",
	       "residuals below this always considered feasible",
	       U_p_real(val, 4e-10),U_p_real(lo, 1e-20),U_p_real(hi,10e10),2);

  slv_define_parm(parameters, real_parm,
	       "oneDsearch","RTONED","accuracy of one dimensional search",
	       U_p_real(val,0.2),U_p_real(lo,0.1),U_p_real(hi,0.7),2);

  slv_define_parm(parameters, real_parm,
	       "stepmult","RVSTLM","steplength multiplier",
	       U_p_real(val,4),U_p_real(lo,0),U_p_real(hi,MAX_REAL),2);

  slv_define_parm(parameters, real_parm,
	       "objtol","RTOBJR","relative objective tolerance",
	       U_p_real(val,3e-13),U_p_real(lo,0),U_p_real(hi,1),2);

  slv_define_parm(parameters, real_parm,
	       "pivottol","RTPIVA","absolute pivot tolerance",
	       U_p_real(val,1e-7),U_p_real(lo,1e-15),U_p_real(hi,1),2);

  slv_define_parm(parameters, real_parm,
	       "pivtolrel","RTPIVR","relative pivot tolerance",
	       U_p_real(val,0.05),U_p_real(lo,0),U_p_real(hi,1),2);

  slv_define_parm(parameters, real_parm,
	       "opttol","RTREDG","optimality tolerance",
	       U_p_real(val,2e-5),U_p_real(lo,0),U_p_real(hi,MAX_REAL),2);

  slv_define_parm(parameters, int_parm,
	       "log_freq","LFILOG","Log Frequency",
	       U_p_int(val,10),U_p_int(lo,1),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "iterationlimit", "LFITER", "maximum number of iterations",
	       U_p_int(val, 1000),U_p_int(lo, 1),U_p_int(hi,MAX_INT),1);
  SLV_IPARM_MACRO(ITER_LIMIT_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "slowproglim","LFNICR","limit for slow progress",
	       U_p_int(val,5),U_p_int(lo,1),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "maxhessdim","LFNICR","maximum Hessian dimension",
	       U_p_int(val,500),U_p_int(lo,1),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "supbasiclim","LFMXNS","limit on new superbasics",
	       U_p_int(val,5),U_p_int(lo,0),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "errlim","max # func errs",
	       "limit on function evaluation errors",
	       U_p_int(val,500),U_p_int(lo,0),U_p_int(hi,MAX_INT),1);
  SLV_IPARM_MACRO(DOMLIM_PTR,parameters);

  slv_define_parm(parameters, char_parm,
	       "scaleopt", "scaling option", "scaling option",
	       U_p_string(val,scaling_names[1]),
	       U_p_strings(lo,scaling_names),
	       U_p_int(hi,sizeof(scaling_names)/sizeof(char *)),3);
  SLV_CPARM_MACRO(SCALEOPT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "lifds", "show singletons details", "show singletons details",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 3);
  SLV_BPARM_MACRO(LIFDS_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "safe_calc", "safe calculations", "safe calculations",
	       U_p_bool(val, 1),U_p_bool(lo,0),U_p_bool(hi,1), 3);
  SLV_BPARM_MACRO(SAFE_CALC_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "toosmall", "default for zero nominal",
               "default for zero nominal",
	       U_p_real(val, 1e-8),U_p_real(lo, 1e-12),U_p_real(hi,1.0), 3);
  SLV_RPARM_MACRO(TOO_SMALL_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "upwts", "Row scaling update frequency",
               "Row scaling update frequency",
	       U_p_int(val, 1),U_p_int(lo,0),U_p_int(hi,20000), 3);
  SLV_IPARM_MACRO(UPDATE_WEIGHTS_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "upnom", "Column scaling update frequency",
               "Column scaling update frequency",
	       U_p_int(val, 1000),U_p_int(lo,0),U_p_int(hi,20000), 3);
  SLV_IPARM_MACRO(UPDATE_NOMINALS_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "cncols", "Check poorly scaled columns",
               "Check poorly scaled columns",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 3);
  SLV_BPARM_MACRO(DUMPCNORM_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "cnlow", "smallest allowable column norm",
               "smallest allowable column norm",
	       U_p_real(val, 0.01),U_p_real(lo, 0),U_p_real(hi,10e10), 3);
  SLV_RPARM_MACRO(CNLOW_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "cnhigh", "largest allowable column norm",
               "largest allowable column norm",
	       U_p_real(val, 100.0),U_p_real(lo,0),U_p_real(hi,10e10), 3);
  SLV_RPARM_MACRO(CNHIGH_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "upjac", "Jacobian update frequency",
               "Jacobian update frequency",
	       U_p_int(val, 1),U_p_int(lo,0),U_p_int(hi,20000), 3);
  SLV_IPARM_MACRO(UPDATE_JACOBIAN_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "itscalelim", "Iteration lim for iterative scale",
               "Iteration lim for iterative scale",
	       U_p_int(val, 10),U_p_int(lo,0),U_p_int(hi,20000), 3);
  SLV_IPARM_MACRO(ITSCALELIM_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "itscaletol", "Iterative scaling tolerance",
               "Iterative scaling tolerance",
	       U_p_real(val, 0.99999),U_p_real(lo,0),U_p_real(hi,1.0), 3);
  SLV_RPARM_MACRO(ITSCALETOL_PTR,parameters);

  slv_define_parm(parameters, char_parm,
	       "reorder", "reorder method", "reorder method",
	       U_p_string(val,reorder_names[0]),
	       U_p_strings(lo,reorder_names),
	       U_p_int(hi,sizeof(reorder_names)/sizeof(char *)),3);
  SLV_CPARM_MACRO(REORDER_OPTION_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "cutoff", "block size cutoff (MODEL-based)",
               "block size cutoff (MODEL-based)",
	       U_p_int(val, 200),U_p_int(lo,0),U_p_int(hi,20000), 3);
  SLV_IPARM_MACRO(CUTOFF_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "relnomscale", "calc rel nominals", "calc rel nominals",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 3);
  SLV_BPARM_MACRO(RELNOMSCALE_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "timelimit", "time limit (CPU sec/block)",
               "time limit (CPU sec/block)",
	       U_p_int(val,1500),U_p_int(lo, 1),U_p_int(hi,20000),1);
  SLV_IPARM_MACRO(TIME_LIMIT_PTR,parameters);

  return 1;
}


/*
 *  External routines
 *  -----------------
 *  See slv_client.h
 */

static SlvClientToken slv8_create(slv_system_t server, int32*statusindex)
{
  slv8_system_t sys;

  sys = (slv8_system_t)asccalloc(1, sizeof(struct slv8_system_structure) );
  if (sys==NULL) {
    *statusindex = 1;
    return sys;
  }
  SERVER = server;
  sys->p.parms = sys->pa;
  sys->p.dynamic_parms = 0;
  slv8_get_default_parameters(server,(SlvClientToken)sys,&(sys->p));
  sys->integrity = OK;
  sys->presolved = 0;
  sys->resolve = 0;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = stdout;

  sys->p.whose = (*statusindex);

  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.costsize = 0;
  sys->s.cost = NULL; /*redundant, but sanity preserving */
  sys->vlist = slv_get_solvers_var_list(server);
  sys->rlist = slv_get_solvers_rel_list(server);
  sys->obj = slv_get_obj_relation(server);
  if (sys->vlist == NULL) {
    ascfree(sys);
    FPRINTF(MIF(sys),"CONOPT called with no variables.\n");
    *statusindex = -2;
    return NULL; /* prolly leak here */
  }
  if (sys->rlist == NULL && sys->obj == NULL) {
    ascfree(sys);
    FPRINTF(MIF(sys),"CONOPT called with no relations or objective.\n");
    *statusindex = -1;
    return NULL; /* prolly leak here */
  }
  /* we don't give a damn about the objective list or the pars or
   * bounds or extrels or any of the other crap.
   */
  slv_check_var_initialization(server);
  *statusindex = 0;
  return((SlvClientToken)sys);
}

static void destroy_matrices( slv8_system_t sys)
{
   if( sys->J.mtx ) {
     mtx_destroy(sys->J.mtx);
   }
}

static void destroy_vectors( slv8_system_t sys)
{
   destroy_array(sys->nominals.vec);
   destroy_array(sys->weights.vec);
   destroy_array(sys->relnoms.vec);
   destroy_array(sys->variables.vec);
   destroy_array(sys->residuals.vec);
}


static int32 slv8_eligible_solver(slv_system_t server)
{
  struct rel_relation **rp;
  for( rp=slv_get_solvers_rel_list(server); *rp != NULL ; ++rp ) {
    if( rel_less(*rp) || rel_greater(*rp) ) return(FALSE);
  }
  return(TRUE);
}


static void slv8_get_parameters(slv_system_t server, SlvClientToken asys,
                                slv_parameters_t *parameters)
{
  slv8_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = SLV8(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}


static void slv8_set_parameters(slv_system_t server, SlvClientToken asys,
                                slv_parameters_t *parameters)
{
  slv8_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = SLV8(asys);
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}


static void slv8_get_status(slv_system_t server, SlvClientToken asys,
                            slv_status_t *status)
{
  slv8_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = SLV8(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
}


static linsolqr_system_t slv8_get_linsolqr_sys(slv_system_t server,
                                               SlvClientToken asys)
{
  slv8_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = SLV8(asys);
  if (check_system(sys)) return NULL;
  return(sys->J.sys);
}


static linsol_system_t slv8_get_linsol_sys(slv_system_t server,
                                           SlvClientToken asys)
{
  (void)server;  /* stop gcc whine about unused parameter */
  (void)asys;    /* stop gcc whine about unused parameter */
  return( NULL );
}


/*
 *  Performs structural analysis on the system, setting the flags in
 *  status.  The problem must be set up, the relation/variable list
 *  must be non-NULL.  The
 *  jacobian (linear) system must be created and have the correct order
 *  (stored in sys->cap).  Everything else will be determined here.
 *  On entry there isn't yet a correspondence between var_sindex and
 *  jacobian column. Here we establish that.
 */
static void structural_analysis(slv_system_t server, slv8_system_t sys)
{
/* this function has been striped of its guts for conopt
   and may go away */

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
   * singular or not. Calculating this number will keep CONOPT from
   * displaying a "structurally singular" error message
   */
  if (sys->rtot) {
    slv_block_partition(server);
  }
  sys->J.dofdata = slv_get_dofdata(server);
  sys->rank = sys->J.dofdata->structural_rank;
  /*
   * Unify the partitions since we feed CONOPT with a single block.
   */
  slv_block_unify(server);


  sys->J.reg.row.low = sys->J.reg.col.low = 0;
  sys->J.reg.row.high = sys->con.m -1;
  if (sys->obj != NULL) sys->J.reg.row.high--;
  sys->J.reg.col.high = sys->con.n -1;
  slv_check_bounds(SERVER,sys->vused,sys->vtot-1,MIF(sys),"fixed");

  /* Initialize Status */
  sys->s.over_defined = (sys->rused > sys->vused);
  sys->s.under_defined = (sys->rused < sys->vused);
  sys->s.struct_singular = (sys->rank < sys->rused);
  sys->s.block.number_of = (slv_get_solvers_blocks(SERVER))->nblocks;

}


static void create_matrices(slv_system_t server, slv8_system_t sys)
{
  sys->J.mtx = mtx_create();
  mtx_set_order(sys->J.mtx,sys->cap);
  structural_analysis(server,sys);
}


static void create_vectors(sys)
slv8_system_t sys;
{
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


static void slv8_dump_internals(slv_system_t server,
				SlvClientToken sys,int32 level)
{
  (void)server;  /* stop gcc whine about unused parameter */

  check_system(sys);
  if (level > 0) {
    FPRINTF(MI8F(sys),"ERROR:  (slv8) slv8_dump_internals\n");
    FPRINTF(MI8F(sys),"        slv8 does not dump its internals.\n");
  }
}


/*
 * Here we will check if any fixed or included flags have
 * changed since the last presolve.
 */
static int32 slv8_dof_changed(slv8_system_t sys)
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


/*
 * After running CONOPT, we need to update the values of the array
 * cost, which is used by the interface to display residual and number
 * of iterations
 */
static void update_cost(slv8_system_t sys)
{
  int32 ci;
  if (sys->s.cost == NULL) {
    sys->s.cost = create_zero_array(sys->s.costsize,struct slv_block_cost);
  } else {
    reset_cost(sys->s.cost,sys->s.costsize);
  }
  ci=sys->s.block.current_block;
  sys->s.cost[ci].size	= sys->s.block.current_size;
  sys->s.cost[ci].iterations = sys->s.block.iteration;
  sys->s.cost[ci].resid	= sys->s.block.residual;
}


static void slv8_presolve(slv_system_t server, SlvClientToken asys)
{
  struct var_variable **vp;
  struct rel_relation **rp;
  int32 cap, ind;
  int32 matrix_creation_needed = 1;
  slv8_system_t sys;

  sys = SLV8(asys);
  iteration_begins(sys);
  check_system(sys);
  if( sys->vlist == NULL ) {
    FPRINTF(MIF(sys),"ERROR:  (slv8) slv8_presolve\n");
    FPRINTF(MIF(sys),"        Variable list was never set.\n");
    return;
  }
  if( sys->rlist == NULL && sys->obj == NULL ) {
    FPRINTF(MIF(sys),"ERROR:  (slv8) slv8_presolve\n");
    FPRINTF(MIF(sys),"        Relation list and objective never set.\n");
    return;
  }

  sys->obj = slv_get_obj_relation(server); /*may have changed objective*/

  if(sys->presolved > 0) { /* system has been presolved before */
    if(!slv8_dof_changed(sys) /*no changes in fixed or included flags*/
       && sys->p.partition == sys->J.old_partition
       && sys->obj == sys->old_obj) {
#if DEBUG
      FPRINTF(MIF(sys),"YOU JUST AVOIDED MATRIX DESTRUCTION/CREATION\n");
#endif  /* DEBUG */
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
    sys->resolve = 0;   /* initialize resolve flag */
    sys->J.old_partition = sys->p.partition;
    sys->old_obj = sys->obj;

    slv_sort_rels_and_vars(server,&(sys->con.m),&(sys->con.n));
    if (sys->obj != NULL) {
      sys->con.m++; /* treat objective as a row */
    }

    sys->con.ipsz[0] = sys->con.n;
    sys->con.ipsz[1] = sys->con.m;
    sys->con.nz = num_jacobian_nonzeros(sys, &(sys->con.maxrow));
    sys->con.ipsz[2] = sys->con.nz;
    sys->con.nintgr = NINTGR;
    /*
     * Memory estimation by calling the CONOPT subroutine coimem
     * The use of conopt_estimate_memory is a hack to avoid
     * unresolved external during the linking of the CONOPT library.
     * See conopt.h
     */
    conopt_estimate_memory(&(sys->con.nintgr),&(sys->con.ipsz[0]),
                &(sys->con.minmem),&(sys->con.estmem));

    if (sys->con.work != NULL) {
      sys->con.work = (real64 *)ascrealloc(sys->con.work,
					   sys->con.estmem*sizeof(real64));
    } else {
      /* calloc here doesn't help the crash */
      sys->con.work = (real64 *)ascmalloc(sys->con.estmem*sizeof(real64));
    }
    sys->con.lwork = sys->con.estmem;
    destroy_vectors(sys);
    destroy_matrices(sys);
    create_matrices(server,sys);
    create_vectors(sys);

    sys->s.block.current_reordered_block = -2;
  }

  /* Reset status */
  sys->con.optimized = 0;
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


static void slv8_resolve(slv_system_t server, SlvClientToken asys)
{
  struct var_variable **vp;
  struct rel_relation **rp;
  slv8_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = SLV8(asys);

  check_system(sys);
  for( vp = sys->vlist ; *vp != NULL ; ++vp ) {
    var_set_in_block(*vp,FALSE);
  }
  for( rp = sys->rlist ; *rp != NULL ; ++rp ) {
    rel_set_in_block(*rp,FALSE);
    rel_set_satisfied(*rp,FALSE);
  }

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
  sys->objective =  MAXDOUBLE/2000.0;

  update_status(sys);
}


/*
 * CONOPT ROUTINES
 */

/*
 * WORK TO DO:
 * Fix interface so that solvers define status messages. We
 * should not be stuck with one standard set that all solvers
 * must deal with.
 * Reimplement old code to detect linear coefficients and use
 * in conopt hookup.
 * Implement better communication routines.
 * Give more contol to interface (ex. turn off error counting,
 * switch from we alocate to conopt allocates, etc.).
 * Record marginal values and make available to user.
 * Set up interface such that any variable can be selected and
 * either maximized or minimized.
 * Get rid of our Jacobian calculation routine and stuff conopt's
 * workspace directly (in unsorted order). This will require
 * rewriting the scaling functions. (This code really should
 * not be in the solver)
 * Fix up restart code...we don't keep track of which values
 * change so must update entire Jacobian and residual vector
 * but may save some time by not having to redo column pointers etc.
 * Implement a way to bailout...check for ^C and tell conopt to
 * return as soon as possible.
 * Currently will not take problem like MIN x^2...it will complain
 * about empty rows.  Must formulate as y=x^2; MIN y; until we
 * fix the way we handle objectives.
 */


/*
 * COIRMS Based on the information provided in Coispz, CONOPT will
 * allocate the number of vectors into which the user can define
 * the details of the model. The details of the model are defined
 * here.
 *
 * COIRMS(lower, curr, upper, vsta, type,rhs, fv, esta, colsta,
 * rowno, value, nlflag, n, m, n1, nz, usrmem)
 *
 * lower - lower bounds on the variables
 * curr  - intial values of the variables
 * upper - upper bounds on the variables
 * vsta  - initial status of the variable(o nonbasic, 1 basic)
 * type  - types of equations (0 equality,1 greater,2 less)
 * rhs   - values of the right hand sides
 * fv    - sum of the nonlinear terms in the initial point
 * esta  - initial status of the slack in the constraint (nonbasic,basic)
 * colsta- start of column pointers
 * rowno - row or equation numbers of the nonzeros
 * value - values of the jacobian elements
 * nlflag- nonlinearity flags(0 nonzero constant,1 varying)
 * n     - number of variables
 * m     - number of constraints
 * n1    - n+1
 * nz    - number of jacobian elements
 * usrmem- user memory defined by conopt
 */
static void slv8_coirms(real64 *lower, real64 *curr, real64 *upper,
                        int32 *vsta, int32 *type, real64 *rhs,
                        real64 *fv, int32 *esta, int32 *colsta,
                        int32 *rowno, real64 *value, int32 *nlflag,
                        int32 *n, int32 *m, int32 *n1, int32 *nz,
                        real64 *usrmem)
{
  int32 col,row,count,count_old,len,c,r,offset, obj_count;
  real64 nominal, up, low;
  struct var_variable *var;
  struct rel_relation *rel;
  const struct rel_relation **rlist=NULL;
  static rel_filter_t rfilter;
  static var_filter_t vfilter;
  real64 *derivatives;
  int32 *variables;
  mtx_coord_t coord;
  slv8_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)vsta;  (void)rhs;   (void)esta;  (void)n;

  sys = (slv8_system_t)usrmem;
  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
  rfilter.matchvalue =(REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);

  vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

  calc_J(sys);
  calc_residuals(sys);
  scale_system(sys);

  for (offset = col = sys->J.reg.col.low;
       col <= sys->J.reg.col.high; col++) {
    var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
    nominal = sys->nominals.vec[col];
    low = var_lower_bound(var)/nominal;
    up = var_upper_bound(var)/nominal;
/* KHACK: get rid of hard coded numbers */
    lower[col-offset] = low > -1e20 ? low : -1e20;
    upper[col-offset] = up < 1e20 ? up : 1e20;
    curr[col-offset] = sys->variables.vec[col]; /* already scaled */
    vsta[col-offset] = !var_nonbasic(var);
  }
  for (offset = row = sys->J.reg.row.low;
       row <= sys->J.reg.row.high; row++) {
    rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
    nominal = sys->weights.vec[row];
    fv[row-offset] = sys->residuals.vec[row]; /* already scaled */
  }
  for (row = 0; row < *m; row++) {
    type[row] = 0;
  }
  if (sys->obj != NULL) {
    type[*m-1] = 3;
  }

  if (sys->obj != NULL) {
    len = rel_n_incidences(sys->obj);
    variables = (int32 *)ascmalloc(len*sizeof(int32));
    derivatives = (real64 *)ascmalloc(len*sizeof(real64));
    relman_diff2(sys->obj,&vfilter,derivatives,variables,
		 &(obj_count),SAFE_CALC);
  }

  count = count_old = 0;

  colsta[0] = 1;

  for (offset = col = sys->J.reg.col.low;
       col <= sys->J.reg.col.high; col++) {
    coord.col = col;
    var = sys->vlist[col];
#if CONDBG
    if (!var_apply_filter(var,&vfilter) ) {
      FPRINTF(stderr,"var doesn't pass filter\n");
    }
#endif /* CONDBG */
    len = var_n_incidences(var);
    rlist = var_incidence_list(var);
    count_old = count;
    for (c=0; c < len; c++) {
      /* assuming obj on list... check this */
      if (rel_apply_filter(rlist[c],&rfilter)) {
	coord.row = rel_sindex(rlist[c]);
	rowno[count] = C2F(rel_sindex(rlist[c]) - offset);
	value[count] = mtx_value(sys->J.mtx,&coord);
	nlflag[count] = 1;               /* fix this later */
	if(rlist[c] == sys->obj) {
#if CONDBG
	  FPRINTF(stderr,"found objective in unexpected location\n");
#endif /* CONDBG */
	}
      if (fabs(value[count]) > RTMAXJ) {
#if CONDBG
	FPRINTF(stderr,"Large Jacobian value being set to RTMAXJ\n");
#endif /* CONDBG */
	if (value[count] > 0) {
	  value[count] = RTMAXJ-1;
	} else {
	  value[count] = -RTMAXJ+1;
	}
      }
	count++;
      }
      if(rlist[c] == sys->obj) {
	for (r = 0; r < obj_count; r++) {
	  if ( variables[r] == var_sindex(var) ) {
	    rowno[count] = *m;
	    value[count] = derivatives[r];
	    nlflag[count] = 1;               /* fix this later */
	    if (fabs(value[count]) > RTMAXJ) {
	      if (value[count] > 0) {
		value[count] = RTMAXJ-1;
	      } else {
		value[count] = -RTMAXJ+1;
	      }
	    }
	    count++;
	  }
	}
      }
    }
    if (count_old != count) {
      colsta[col - offset] = C2F(count_old);
    }
  }
  colsta[F2C(*n1)] = *nz + 1;
  if (sys->obj != NULL) {
    ascfree(variables);
    ascfree(derivatives);
  }
}

#if 0 /* not compatible with our version */
/*
 * COIFBL Defines the nonlinearities of the model by returning
 * numerical values. It works on a block of rows during each call.
 * COIFBL( x, g, otn, nto, from, to, jac, stcl, rnum, cnum, nl, strw,
 *         llen, indx, mode, errcnt, n, m, n1, m1, nz, usrmem)
 *
 * x     - punt of evaluation provided by conopt
 * g     - vector of function values
 * otn   - old to new permutation vector
 * nto   - new to old permutation vector
 * from  - range in permutation
 * to    - range in permutation
 * jac   - vector of jacobian values.
 *         The following are vectors defining the jacobian structure
 * stcl  - start of column pointers
 * rnum  - row numbers
 * cnum  - column numbers
 * nl    - nonlinearity flags
 * strw  - start row pointers
 * llen  - count of linear jacobian elements
 * indx  - pointers from the row-wise representation
 * mode  - indicator of mode of evaluation
 * errcnt- number of function evaluation errors
 * n     - umber of variables
 * m     - number of constraints
 * n1    - n+1
 * m1    - m+1
 * nz    - number of jacobian elements
 * usrmem- user memory defined by conopt
 */
static void slv8_coifbl(real64 *x, real64 *g, int32 *otn, int32 *nto,
	                int32 *from, int32 *to, real64 *jac, int32 *stcl,
	                int32 *rnum, int32 *cnum, int32 *nl, int32 *strw,
	                int32 *llen, int32 *indx, int32 *mode, int32 *errcnt,
	                int32 *n, int32 *m, int32 *n1, int32 *m1, int32 *nz,
	                real64 *usrmem)
{
  int32 offset, col, row, j, jj, len, c;
  real64 nominal, value;
  struct var_variable *var;
  struct rel_relation *rel;
  int32 *jac_row;
  int32 *variables;
  real64 *derivatives;
  static var_filter_t vfilter;
  slv8_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)nto;  (void)stcl;   (void)rnum;  (void)nl;
  (void)errcnt;  (void)n1;   (void)m1;  (void)nz;

  sys = (slv8_system_t)usrmem;

  for (offset = col = sys->J.reg.col.low;
       col <= sys->J.reg.col.high; col++) {
    var = sys->vlist[col];
    nominal = sys->nominals.vec[col];
    value = x[col-offset] * nominal;
    var_set_value(var, value);
  }
  /* NOTE: could be more efficient when mode = 3 */
  if (*mode == 1 || *mode == 3) {
    for (offset = row = sys->J.reg.row.low;
	 row <= sys->J.reg.row.high; row++) {
      if (F2C(*to) <= otn[row-offset] && otn[row-offset] <= F2C(*from)) {
	rel = sys->rlist[row];
	g[row-offset] = relman_eval(rel,&calc_ok,SAFE_CALC)
	  * sys->weights.vec[row];
      }
    }
    if (F2C(*to) <= otn[F2C(*m)] && otn[F2C(*m)] <= F2C(*from)) {
      if(calc_objective(sys)){
	g[F2C(*m)] = sys->objective;
      } else {
	FPRINTF(MIF(sys),"slv8_coifbl: ERROR IN OBJECTIVE CALCULATION\n");
      }
    }
  }
  jac_row = (int32 *)ascmalloc((*n)*sizeof(int32));
  if (*mode == 2 || *mode == 3) {
    len = sys->con.maxrow;
    variables = (int32 *)ascmalloc(len*sizeof(int32));
    derivatives = (real64 *)ascmalloc(len*sizeof(real64));
    vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
    vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);
    for (offset = row = sys->J.reg.row.low;
	 row <= sys->J.reg.row.high; row++) {
      if (F2C(*to) <= otn[row-offset] && otn[row-offset] <= F2C(*from)) {
	rel = sys->rlist[row];
	relman_diff2(rel,&vfilter,derivatives,variables,
		 &(len),SAFE_CALC);
	for (c = 0; c < len; c++) {
	  jac_row[variables[c]] = derivatives[c];
	}
	for (j = strw[row-offset] + llen[row-offset];
	     j < strw[row-offset + 1]; j++) {
	  jj = indx[F2C(j)];
	  jac[F2C(jj)] = jac_row[F2C(cnum[F2C(jj)])]
	    * sys->weights.vec[row]
	      *  sys->nominals.vec[F2C(cnum[F2C(jj)])];
	  if(fabs(jac[F2C(jj)]) > RTMAXJ) {
            if (jac[F2C(jj)] < 0) {
              jac[F2C(jj)] = -RTMAXJ+1;
	    } else {
              jac[F2C(jj)] = RTMAXJ-1;
	    }
#if CONDBG
	    FPRINTF(stderr,"large jac element\n");
#endif  /* CONDBG */
	  }
	}
      }
    }
  }
}
#endif /* 0 */

/*
 * COIFDE Defines the nonlinearities of the model by returning
 * numerical values. It works on one row or equation at a time
 * COIFDE(x, g, jac, rowno, jcnm, mode, errcnt, newpt, n, nj, usrmem)
 *
 * x      - punt of evaluation provided by conopt
 * g      - function value
 * jac    - jacobian values
 * rowno  - number of the row for which nonlinearities will be eval
 * jcnm   - list of column number fon the NL nonzeros
 * mode   - indicator of mode of evaluation
 * errcnt - sum of number of func evaluation errors thus far
 * newpt  - new point indicator
 * nj     - number of nonlinear nonzero jacobian elements
 * n      - number of variables
 * usrmem - user memory
 */
static void slv8_coifde(real64 *x, real64 *g, real64 *jac, int32 *rowno,
	                int32 *jcnm, int32 *mode, int32 *errcnt,
	                int32 *newpt, int32 *n, int32 *nj, real64 *usrmem)
{
  int32 offset, col, row, len, c;
  real64 nominal, value;
  struct var_variable *var;
  struct rel_relation *rel;
  int32 *variables;
  real64 *derivatives;
  static var_filter_t vfilter;
  slv8_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)jcnm;  (void)n;   (void)nj;

  sys = (slv8_system_t)usrmem;
  if (*newpt == 1) {
    for (offset = col = sys->J.reg.col.low;
	 col <= sys->J.reg.col.high; col++) {
      var = sys->vlist[col];
      nominal = sys->nominals.vec[col];
      value = x[col-offset] * nominal;
      var_set_value(var, value);
    }
  }
  /* NOTE: could be more efficient when mode = 3
   * (with future versions of CONOPT)
   */
  if (*mode == 1 || *mode == 3) {
    offset =  sys->J.reg.row.low;
    row = F2C(*rowno + offset);
    if ((*rowno == sys->con.m) && (sys->obj != NULL)){
      if(calc_objective(sys)){
	*g = sys->objective;
      } else {
	FPRINTF(MIF(sys),"slv8_coifde: ERROR IN OBJECTIVE CALCULATION\n");
      }
    } else {
      rel = sys->rlist[row];
      *g = relman_eval(rel,&calc_ok,SAFE_CALC)
	* sys->weights.vec[row];
      if (!calc_ok) {
	(*errcnt)++;
      }
    }
  }
  if (*mode == 2 || *mode == 3) {
    len = sys->con.maxrow;
    variables = (int32 *)ascmalloc(len*sizeof(int32));
    derivatives = (real64 *)ascmalloc(len*sizeof(real64));
    vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
    vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

    offset =  sys->J.reg.row.low;
    row = F2C(*rowno + offset);
    if ((*rowno == sys->con.m) && (sys->obj != NULL)){
      rel = sys->obj;
      calc_ok = relman_diff2(rel,&vfilter,derivatives,variables,
		   &(len),SAFE_CALC);
      for (c = 0; c < len; c++) {
	jac[variables[c]] = derivatives[c]
	  *  sys->nominals.vec[variables[c]];
      }
      if (!calc_ok) {
	(*errcnt)++;
      }
    } else {
      rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
      calc_ok = relman_diff2(rel,&vfilter,derivatives,variables,
		   &(len),SAFE_CALC);
      for (c = 0; c < len; c++) {
	jac[variables[c]] = derivatives[c]
	  * sys->weights.vec[row] *  sys->nominals.vec[variables[c]];
      }
      if (!calc_ok) {
	(*errcnt)++;
      }
    }
    for (c = 0; c < len; c++) {
      if(fabs(jac[variables[c]]) > RTMAXJ) {
#if CONDBG
	FPRINTF(stderr,"large jac element\n");
#endif /* CONDBG  */
        if (jac[variables[c]] < 0) {
          jac[variables[c]] = -RTMAXJ+1;
	} else {
          jac[variables[c]] = RTMAXJ-1;
	}
      }
    }
    ascfree(variables);
    ascfree(derivatives);
  }
}


/*
 * COISTA Pass the solution from CONOPT to the modeler. It returns
 * completion status
 * COISTA(modsta, solsts, iter, objval, usrmem)
 *
 * modsta - model status
 * solsta - solver status
 * iter   - number of iterations
 * objval - objective value
 * usrmem - user memory
 */
static void slv8_coista(int32 *modsta, int32 *solsta, int32 *iter,
	                real64 *objval, real64 *usrmem)
{
  slv8_system_t sys;
  sys = (slv8_system_t)usrmem;
  sys->con.modsta = *modsta;
  sys->con.solsta = *solsta;

#if NONBASIC_DEBUG
  FPRINTF(ASCERR," Model Status = %d\n",*modsta);
  FPRINTF(ASCERR," Solver Status = %d\n",*solsta);
#endif /* NONBASIC_DEBUG */

  sys->con.iter = *iter;
  sys->con.obj = sys->objective = *objval;
}


/*
 * COIRS passes the solution from CONOPT to the modeler. It returns
 * solution values
 * COIRS(val, xmar, xbas, xsta, yval, ymar, ybas, ysta, n, m, usrmem)
 *
 * xval   - the solution values of the variables
 * xmar   - corresponding marginal values
 * xbas   - basis indicator for column (at bound, basic, nonbasic)
 * xsta   - status of column (normal, nonoptimal, infeasible,unbounded)
 * yval   - values of the left hand side in all the rows
 * ymar   - corresponding marginal values
 * ybas   - basis indicator for row
 * ysta   - status of row
 * n      - number of variables
 * m      - number of constraints
 * usrmem - user memory
 */
static void slv8_coirs(real64 *xval, real64 *xmar, int32 *xbas, int32 *xsta,
	               real64 *yval, real64 *ymar, int32 *ybas, int32 * ysta,
	               int32 *n, int32 *m, real64 *usrmem)
{
  int32 offset, col, c;
  real64 nominal, value;
  struct var_variable *var;
  slv8_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)xmar;  (void)xsta;  (void)yval;
  (void)ymar;  (void)ysta;  (void)ybas;    (void)m;


  sys = (slv8_system_t)usrmem;

  offset = sys->J.reg.col.low;
  for (c = 0; c < *n; c++) {
    col = c + offset;
    var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
    nominal = sys->nominals.vec[col];
    /*
     * value of the variable
     */
    value = xval[c] * nominal;
    var_set_value(var, value);
    /*
     * status (basic, nonbasic) of the variable
     */
    if (xbas[c] != 2) {
      var_set_nonbasic(var,TRUE);
#if NONBASIC_DEBUG
      FPRINTF(ASCERR," c = %d\n",c);
      FPRINTF(ASCERR," status = %d\n",xbas[c]);
      FPRINTF(ASCERR," nonbasic \n");
#endif /* NONBASIC_DEBUG */
    } else {
      var_set_nonbasic(var,FALSE);
    }
  }

  /* should pull out additional info here */
}


/*
 * COIUSZ communicates and update of an existing model to CONOPT
 * COIUSZ(nintg, ipsz, nreal, rpsz, usrmem)
 *
 * nintg - number of positions in ipsz
 * ipsz  - array describing problem size and options
 * nreal - number of positions in rpsz
 * rpsz  - array of reals describing problem size and options
 * usrmem- user memory
 */
static void slv8_coiusz(int32 *nintg, int32 *ipsz, int32 *nreal,
	                real64 *rpsz, real64 *usrmem)
{
  /*
   * "zero changes" subroutines. the default for all the values is a
   * no change situation
   */

  /*
   * stop gcc whining about unused parameter
   */
  (void)nintg;  (void)ipsz;   (void)nreal;  (void)rpsz;
  (void)usrmem;

#if 0
  slv8_system_t sys;

  /*
   * To Ken: This was in the subroutine before. But all the values
   * are the same as in coipsz. So, no redefintion is necessary since
   * the defaults contain the same information
   */

  /*
   * stop gcc whining about unused parameter
   */
  (void)nintg;  (void)nreal;

  sys = (slv8_system_t)usrmem;

  ipsz[F2C(1)] = sys->con.n;
  ipsz[F2C(2)] = sys->con.m;
  ipsz[F2C(3)] = sys->con.nz;
  ipsz[F2C(4)] = 0;             /* FIX THESE AT A LATER DATE!!!!  */
  ipsz[F2C(5)] = sys->con.nz;   /* ASSUMING ALL NONLINEAR FOR NOW */
  if (sys->obj != NULL) {
    ipsz[F2C(6)] = relman_obj_direction(sys->obj);
    ipsz[F2C(7)] = sys->con.m;    /* objective will be last row     */
  } else {
    ipsz[F2C(7)] = 0;
  }
  ipsz[F2C(8)] = ITER_LIMIT;
  ipsz[F2C(12)] = 1;             /* NON DEFAULT VALUE */
  ipsz[F2C(13)] = 1;             /* NON DEFAULT VALUE */

  rpsz[F2C(7)] = TIME_LIMIT;

#endif

 return;
}


/*
 * COIOPT communicates non-default option values to CONOPT
 * COIOPT(name, rval, ival, lval, usrmem)
 * name   - the name of a CONOPT CR-cell defined by the modeler
 * rval   - the value to be assigned to name if the cells contains a real
 * ival   - the value to be assigned to name if the cells contains an int
 * lval   - the value to be assigned to name if the cells contains a log value
 * usrmem - user memory
 */
static void slv8_coiopt(char *name, real64 *rval, int32 *ival,
		        int32 *logical, real64 *usrmem)
{
  slv8_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)logical;

  sys = (slv8_system_t)usrmem;
  name = memset(name,' ',8);
  while (sys->con.opt_count < slv8_PA_SIZE) {
    if (strlen(sys->p.parms[sys->con.opt_count].interface_label) == 6){
      if (strncmp(sys->p.parms[sys->con.opt_count].interface_label,
                  "R",1) == 0) {
	name = 
          strncpy(name, sys->p.parms[sys->con.opt_count].interface_label,6);
	*rval = sys->p.parms[sys->con.opt_count].info.r.value;
	sys->con.opt_count++;
	return;
      } else if (strncmp(sys->p.parms[sys->con.opt_count].interface_label,
                         "L",1) == 0) {
	name = 
          strncpy(name,sys->p.parms[sys->con.opt_count].interface_label,6);
	*ival = sys->p.parms[sys->con.opt_count].info.i.value;
	sys->con.opt_count++;
	return;
      }
    }
    sys->con.opt_count++;
  }

  /* sending blank to quit iterative calling */
  name = memset(name,' ',8);
}


/*
 * COIPSZ communicates the model size and structure to CONOPT
 * COIPSZ(nintg, ipsz, nreal, rpsz, usrmem)
 *
 * ningt  - number of positions in ipsz
 * ipsz   - array describing problem size and options
 * nreal  - number of positions in rpsz
 * rpsz   - array of reals describing problem size and options
 * usrmem - user memory
 */
static void slv8_coipsz(int32 *nintg, int32 *ipsz, int32 *nreal,
	                real64 *rpsz, real64 *usrmem)
{
  slv8_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)nintg;  (void)nreal;

  sys = (slv8_system_t)usrmem;

  ipsz[F2C(1)] = sys->con.n;
  ipsz[F2C(2)] = sys->con.m;
  ipsz[F2C(3)] = sys->con.nz;
  ipsz[F2C(4)] = 0;             /* FIX THESE AT A LATER DATE!!!!  */
  ipsz[F2C(5)] = sys->con.nz;   /* ASSUMING ALL NONLINEAR FOR NOW */
  if (sys->obj != NULL) {
    ipsz[F2C(6)] = relman_obj_direction(sys->obj);
    ipsz[F2C(7)] = sys->con.m;    /* objective will be last row     */
  } else {
    ipsz[F2C(7)] = 0;
  }
  ipsz[F2C(8)] = ITER_LIMIT;
  ipsz[F2C(9)] = DOMLIM;
  ipsz[F2C(10)] = 1;             /* OUTPUT TO SUBROUTINE */
  ipsz[F2C(11)] = 0;             /* NO OUTPUT TO SCREEN */
  ipsz[F2C(12)] = 1;             /* NON DEFAULT VALUE */
  ipsz[F2C(13)] = 1;             /* NON DEFAULT VALUE */
  ipsz[F2C(14)] = 1;		 /* NON DEFAULT VALUE */
  ipsz[F2C(15)] = 1;             /* NON DEFAULT VALUE */
  ipsz[F2C(16)] = 1;             /* NON DEFAULT VALUE */
  ipsz[F2C(17)] = 0;
  ipsz[F2C(18)] = 0;
  ipsz[F2C(19)] = 0;
  ipsz[F2C(20)] = 0;
  ipsz[F2C(21)] = 0;
  ipsz[F2C(22)] = 1;             /* NON DEFAULT VALUE */
  /*skipping remainder of ipsz which are fortran io parameters */

  rpsz[F2C(1)] = 1e20;
  rpsz[F2C(2)] = -1e20;
  rpsz[F2C(3)] = 1.2e20;
/*rpsz[F2C(4)] = NA*/
/*rpsz[F2C(5)] = eps*/
  rpsz[F2C(6)] = 0;
  rpsz[F2C(7)] = TIME_LIMIT;
  rpsz[F2C(8)] = 1;

}

/* conopt communication subroutines */
void slv8_coiec COIEC_ARGS {
  char *name=NULL;
  struct var_variable **vp;
  slv8_system_t sys;
  sys = (slv8_system_t)usrmem;
  vp=slv_get_solvers_var_list(SERVER);
  /* assumes cur = org */
  vp = vp + (*colno + sys->J.reg.col.low);
  name= var_make_name(SERVER,*vp);
  FPRINTF(stderr,"ERROR in variable:\n     %s\n",name);
  FPRINTF(stdout,"     %.*s\n",*msglen,&msg[0]);
  if (name) {
    ascfree(name);
  }
}
void slv8_coier COIER_ARGS {
  char *name=NULL;
  struct rel_relation **rp;
  slv8_system_t sys;
  sys = (slv8_system_t)usrmem;
  rp=slv_get_solvers_rel_list(SERVER);
  rp = rp + (*rowno + sys->J.reg.row.low);;
  name= rel_make_name(SERVER,*rp);
  FPRINTF(stderr,"ERROR in relation:\n     %s\n",name);
  FPRINTF(stdout,"     %.*s\n",*msglen,&msg[0]);
  if (name) {
    ascfree(name);
  }
}
void slv8_coienz COIENZ_ARGS {
  char *relname=NULL;
  char *varname=NULL;
  struct rel_relation **rp;
  struct var_variable **vp;
  
  slv8_system_t sys;
  sys = (slv8_system_t)usrmem;
  
  rp=slv_get_solvers_rel_list(SERVER);
  vp=slv_get_solvers_var_list(SERVER);
  /* assumes cur = org */
  rp = rp + (*rowno + sys->J.reg.row.low);;
  vp = vp + (*colno + sys->J.reg.col.low);
  relname= rel_make_name(SERVER,*rp);
  varname= var_make_name(SERVER,*vp);
  FPRINTF(stderr,"ERROR in jacobian element:\n");
  FPRINTF(stderr,"     relation: %s\n     variable: %s\n",
    relname, varname);
  FPRINTF(stdout,"%.*s\n",*msglen,&msg[0]);
  if (relname) {
    ascfree(relname);
  }
  if (varname) {
    ascfree(varname);
  }
}

void slv8_coimsg COIMSG_ARGS {
  int32 stop, i, len;
  char *line[15];
  /* should put option to make stop = *smsg or *nmsg 
   * and option to route output
   */
  stop = *nmsg;
  for (i = 0; i < stop; i++) {
    len = llen[i];
    line[i] = &msgv[i*80];
    FPRINTF(stdout,"%.*s\n",len,line[i]);
  }
}

void slv8_coiprg COIPRG_ARGS {
  slv8_system_t sys;
  sys = (slv8_system_t)usrmem;
  if (sys->con.progress_count == 0) {
    FPRINTF(stderr,
      "  iter   phase  numinf  numnop   nsuper  ");
    FPRINTF(stderr,
      "                    suminf                     objval          rgmax\n");
  }
  FPRINTF(stdout,"%6i  ",intrep[0]);
  FPRINTF(stdout,"  %6i  ",intrep[1]);
  FPRINTF(stdout,"    %6i  ",intrep[2]);
  FPRINTF(stdout,"     %6i  ",intrep[3]);
  FPRINTF(stdout,"     %6i    ",intrep[4]);
  FPRINTF(stdout,"  %16e  ",rl[0]);
  FPRINTF(stdout,"  %16e  ",rl[1]);
  FPRINTF(stdout,"  %7.2e  ",rl[2]);
  FPRINTF(stdout,"\n");
  sys->con.progress_count++;
  if (sys->con.progress_count == 10) { /* 10 should be iface parm */
    sys->con.progress_count = 0;
  }
  
}

void slv8_coiorc COIORC_ARGS {
  if (*resid != 0.0) {
    char *relname=NULL;
    char *varname=NULL;
    struct rel_relation **rp;
    struct var_variable **vp;
    int32 row, col;

    slv8_system_t sys;
    sys = (slv8_system_t)usrmem;
  
    rp=slv_get_solvers_rel_list(SERVER);
    vp=slv_get_solvers_var_list(SERVER);
    /* assumes cur = org */
    row = F2C(*rowno + sys->J.reg.row.low);
    rp = rp + row;
    col = F2C(*colno + sys->J.reg.col.low);
    vp = vp + col;
    relname= rel_make_name(SERVER,*rp);
    varname= var_make_name(SERVER,*vp);
  
    FPRINTF(stderr,"ERROR: Infeasible specification discovered at:\n");
    FPRINTF(stderr,"     relation: %s\n          residual: %g\n",
      relname, (*resid)/sys->weights.vec[row]);
    FPRINTF(stderr,"     variable: %s\n          value: %g\n",
      varname, (*value)*sys->nominals.vec[col]);
    
    if (relname) {
      ascfree(relname);
    }
    if (varname) {
      ascfree(varname);
    }
  }
}
void slv8_coiscr COISCR_ARGS {
  FPRINTF(stdout,"%.*s\n",*len,&msg[0]);
}



/*
 * slv_conopt iterate calls conopt_start, which calls coicsm
 * to starts CONOPT. The use of conopt_start is a H A C K    to avoid
 * unresolved external during the linking of the CONOPT library.
 * See conopt.h
 */
static void slv_conopt_iterate(slv8_system_t sys)
{
  real64 **usrmem;
  conopt_pointers conopt_ptrs;

  conopt_ptrs = (conopt_pointers)asccalloc
                 (1, sizeof(struct conopt_function_pointers ) );
  conopt_ptrs->coirms_ptr = slv8_coirms;
  conopt_ptrs->coifbl_ptr = NULL;
  conopt_ptrs->coifde_ptr = slv8_coifde;
  conopt_ptrs->coirs_ptr = slv8_coirs;
  conopt_ptrs->coista_ptr = slv8_coista;
  conopt_ptrs->coiusz_ptr = NULL;
  conopt_ptrs->coiopt_ptr = slv8_coiopt;
  conopt_ptrs->coipsz_ptr = slv8_coipsz;

  conopt_ptrs->coimsg_ptr = slv8_coimsg;
  conopt_ptrs->coiscr_ptr = slv8_coiscr;
  conopt_ptrs->coiec_ptr = slv8_coiec;
  conopt_ptrs->coier_ptr = slv8_coier;
  conopt_ptrs->coienz_ptr = slv8_coienz;
  conopt_ptrs->coiprg_ptr = slv8_coiprg;
  conopt_ptrs->coiorc_ptr = slv8_coiorc;

  usrmem = (real64 **)(ascmalloc(2*sizeof(real64 *)));

/*
 * We pass the pointers to sys and conopt_ptrs instead of a usrmem array.
 * Cast the appropriate element of usrmem back to slv9_system_t and
 * conopt_pointers to access the information required
 */
  usrmem[0] = (real64 *)conopt_ptrs;
  usrmem[1] = (real64 *)sys;

  sys->con.opt_count = 0; /* reset count on slv8_coiopt calls */
  sys->con.progress_count = 0; /* reset count on coiprg calls */

  sys->con.kept = 1;
  conopt_start(&(sys->con.kept), usrmem, &(sys->con.lwork),
	 sys->con.work, &(sys->con.maxusd), &(sys->con.curusd));
  /*
   * We need to check conopt's status codes before claim
   * optimization complete. For now just see status file
   */
  sys->con.optimized = 1;

  ascfree(conopt_ptrs);
  ascfree(usrmem);
}


/*
 * Function created to provide the interface with the correct values
 * for number of iterations, residuals, solved variables, etc
 */
static void update_block_information(slv8_system_t sys)
{
  int32 row,col;

  row = sys->J.reg.row.high - sys->J.reg.row.low + 1;
  col = sys->J.reg.col.high - sys->J.reg.col.low + 1;
  sys->s.block.current_size = MAX(row,col);

  sys->s.block.iteration = sys->con.iter;
  sys->s.iteration = sys->con.iter;

  if ( (sys->con.modsta == 1 || sys->con.modsta == 2)
       && sys->con.solsta == 1 ) {
    sys->s.converged = TRUE;
    sys->s.block.previous_total_size += sys->s.block.current_size;
  } else {
    if (sys->con.solsta == 2 ) {
      sys->s.converged = FALSE;
      sys->s.inconsistent = FALSE;
    } else {
      sys->s.converged = FALSE;
      sys->s.inconsistent = TRUE;
    }
  }
}


static void slv8_iterate(slv_system_t server, SlvClientToken asys)
{
  slv8_system_t sys;
  FILE              *mif;
  FILE              *lif;
  sys = SLV8(asys);
  mif = MIF(sys);
  lif = LIF(sys);
  if (server == NULL || sys==NULL) return;
  if (check_system(SLV8(sys))) return;
  if( !sys->s.ready_to_solve ) {
    FPRINTF(mif,"ERROR:  (slv8) slv8_iterate\n");
    FPRINTF(mif,"        Not ready to solve.\n");
    return;
  }
  
  if (sys->s.block.current_block==-1) {
    conopt_initialize(sys);
    sys->s.converged = sys->con.optimized;
    update_status(sys);
    if( RELNOMSCALE == 1 || (strcmp(SCALEOPT,"RELNOM") == 0) ||
       (strcmp(SCALEOPT,"RELNOM+ITERATIVE") == 0) ){
      calc_relnoms(sys);
    }
  }
  if (sys->p.output.less_important && (sys->s.block.current_size >1 ||
      LIFDS)) {
    debug_delimiter(lif);
  }
  iteration_begins(sys);
  if (1 || sys->J.reg.row.high != sys->J.reg.col.high) {
    /*may have changed objective*/
    sys->obj = slv_get_obj_relation(server);
    slv_conopt_iterate(sys);
    update_block_information(sys); /* update values of block information */
    calc_objective(sys);
    calc_objectives(sys);
    sys->residuals.accurate = FALSE;
    calc_residuals(sys);
    update_cost(sys);
    iteration_ends(sys);
    update_status(sys);
    return;
  }
}


static void slv8_solve(slv_system_t server, SlvClientToken asys)
{
  slv8_system_t sys;
  sys = SLV8(asys);
  if (server == NULL || sys==NULL) return;
  if (check_system(sys)) return;
  while( sys->s.ready_to_solve ) slv8_iterate(server,sys);
}

static mtx_matrix_t slv8_get_jacobian(slv_system_t server, SlvClientToken sys)
{
  if (server == NULL || sys==NULL) return NULL;
  if (check_system(SLV8(sys))) return NULL;
  return SLV8(sys)->J.mtx;
}

static int32 slv8_destroy(slv_system_t server, SlvClientToken asys)
{
  slv8_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)server;

  sys = SLV8(asys);
  if (check_system(sys)) return 1;
  destroy_vectors(sys);
  destroy_matrices(sys);
  slv_destroy_parms(&(sys->p));
  sys->integrity = DESTROYED;
  if (sys->s.cost) ascfree(sys->s.cost);
  if (sys->con.work != NULL) {
    ascfree(sys->con.work);
    sys->con.work = NULL;
  }
  ascfree( (POINTER)asys );
  return 0;
}

int32 slv8_register(SlvFunctionsT *sft)
{
  if (sft==NULL)  {
    FPRINTF(stderr,"slv8_register called with NULL pointer\n");
    return 1;
  }
#ifdef DYNAMIC_CONOPT
  if (conopt_load() == 1) {
    FPRINTF(stderr,"Registration failure: CONOPT dll unavailable\n");
	return 1;
  }
#endif /* DYNAMIC_CONOPT */

  sft->name = "CONOPT";
  sft->ccreate = slv8_create;
  sft->cdestroy = slv8_destroy;
  sft->celigible = slv8_eligible_solver;
  sft->getdefparam = slv8_get_default_parameters;
  sft->getparam = slv8_get_parameters;
  sft->setparam = slv8_set_parameters;
  sft->getstatus = slv8_get_status;
  sft->solve = slv8_solve;
  sft->presolve = slv8_presolve;
  sft->iterate = slv8_iterate;
  sft->resolve = slv8_resolve;
  sft->getlinsol = slv8_get_linsol_sys;
  sft->getlinsys = slv8_get_linsolqr_sys;
  sft->getsysmtx = slv8_get_jacobian;
  sft->dumpinternals = slv8_dump_internals;
  return 0;
}

/* #endif */ /* #else clause of DYNAMIC_CONOPT */
#endif /* #else clause of !STATIC_CONOPT && !DYNAMIC_CONOPT */
