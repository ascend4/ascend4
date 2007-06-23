/*	ASCEND modelling environment
	Copyright (C) 1997, 2006, 2007 Carnegie Mellon University

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
	Connection of the CONOPT solver into ASCEND.
*//*
	originally by Ken Tyner and Vicente Rico-Ramirez, Jun-Aug 1997.
	updated for CONOPT 3 by John Pye, July 2006.
*/

#include <math.h>

#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/set.h>
#include <general/tm_time.h>
#include <general/mathmacros.h>
#include <utilities/mem.h>
#include <general/list.h>

#include <linear/mtx_vector.h>

#include <system/calc.h>
#include <system/relman.h>
#include <system/slv_stdcalls.h>
#include <system/block.h>
#include <solver/solver.h>

#include <solver/conopt.h>

typedef struct conopt_system_structure *conopt_system_t;

#ifdef ASC_WITH_CONOPT
# define HAVE_CONOPT 1
#else
# define HAVE_CONOPT 0
#endif

ASC_DLLSPEC SolverRegisterFn conopt_register;

#define conopt_register_conopt_function register_conopt_function
#define conopt_coicsm coicsm
#define conopt_coimem coimem

#ifndef ASC_WITH_CONOPT
int conopt_register(SlvFunctionsT *f){
  (void)f;  /* stop gcc whine about unused parameter */

  ERROR_REPORTER_HERE(ASC_PROG_ERR,"CONOPT has not been compiled into this copy of ASCEND.");
  return 1;
}
#else

/*
  Output in user defined CONOPT subroutines
*/
#define CONDBG 0
#define NONBASIC_DEBUG FALSE

#if CONDBG
# define CONOPT_CONSOLE_DEBUG(...) CONSOLE_DEBUG(__VA_ARGS__)
#else
# define CONOPT_CONSOLE_DEBUG(...) (void)0
#endif

/*
  makes lots of extra spew
*/
#define DEBUG FALSE

#define CONOPT(s)        ((conopt_system_t)(s))
#define MI8F(s)        slv_get_output_file( CONOPT(s)->p.output.more_important )
#define SERVER         (sys->slv)
#define conopt_PA_SIZE 56
#define SAFE_CALC_PTR  (sys->parm_array[0])
#define SAFE_CALC      ((*(int *)SAFE_CALC_PTR))
#define SCALEOPT_PTR   (sys->parm_array[1])
#define SCALEOPT       ((*(char **)SCALEOPT_PTR))
#define TOO_SMALL_PTR  (sys->parm_array[2])
#define TOO_SMALL      ((*(real64 *)TOO_SMALL_PTR))
#define UPDATE_NOMINALS_PTR (sys->parm_array[3])
#define UPDATE_NOMINALS     ((*(int *)UPDATE_NOMINALS_PTR))
#define UPDATE_RELNOMS_PTR (sys->parm_array[4])
#define UPDATE_RELNOMS ((*(int *)UPDATE_RELNOMS_PTR))
#define UPDATE_WEIGHTS_PTR (sys->parm_array[5])
#define UPDATE_WEIGHTS ((*(int *)UPDATE_WEIGHTS_PTR))
#define DUMPCNORM_PTR  (sys->parm_array[6])
#define DUMPCNORM      ((*(int *)DUMPCNORM_PTR))
#define CNLOW_PTR      (sys->parm_array[7])
#define CNLOW          ((*(real64 *)CNLOW_PTR))
#define CNHIGH_PTR     (sys->parm_array[8])
#define CNHIGH         ((*(real64 *)CNHIGH_PTR))
#define UPDATE_JACOBIAN_PTR   (sys->parm_array[9])
#define UPDATE_JACOBIAN       ((*(int *)UPDATE_JACOBIAN_PTR))
#define ITSCALELIM_PTR (sys->parm_array[10])
#define ITSCALELIM     ((*(int *)ITSCALELIM_PTR))
#define ITSCALETOL_PTR (sys->parm_array[11])
#define ITSCALETOL     ((*(real64 *)ITSCALETOL_PTR))
#define LIFDS_PTR      (sys->parm_array[12])
#define LIFDS          ((*(int32 *)LIFDS_PTR))
#define REORDER_OPTION_PTR (sys->parm_array[13])
#define REORDER_OPTION ((*(char **)REORDER_OPTION_PTR))
#define CUTOFF_PTR     (sys->parm_array[14])
#define CUTOFF         ((*(int32 *)CUTOFF_PTR))
#define RELNOMSCALE_PTR (sys->parm_array[15])
#define RELNOMSCALE    ((*(int *)RELNOMSCALE_PTR))
#define ITER_LIMIT_PTR (sys->parm_array[16])
#define ITER_LIMIT     ((*(int32 *)ITER_LIMIT_PTR))
#define TIME_LIMIT_PTR (sys->parm_array[17])
#define TIME_LIMIT     ((*(int32 *)TIME_LIMIT_PTR))
#define DOMLIM_PTR     (sys->parm_array[18])
#define DOMLIM         ((*(int32 *)DOMLIM_PTR))
#define RTMAXJ_PTR     (sys->parm_array[19])
#define RTMAXJ         ((*(real64 *)RTMAXJ_PTR))

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

struct conopt_system_structure {

  /*
    Problem definition
  */
  slv_system_t                slv;     /* slv_system_t back-link */
  struct rel_relation         *obj;    /* Objective function: NULL = none */
  struct rel_relation         *old_obj;/* Objective function: NULL = none */
  struct var_variable         **vlist; /* Variable list (NULL terminated) */
  struct rel_relation         **rlist; /* Relation list (NULL terminated) */

  /*
    Solver information
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

  void *parm_array[conopt_PA_SIZE];
  struct slv_parameter pa[conopt_PA_SIZE];

  /*
    CONOPT DATA
  */
  struct conopt_data con;

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
};


/*------------------------------------------------------------------------------
  INTEGRITY CHECKS
*/

#define OK        ((int32)813029392)
#define DESTROYED ((int32)103289182)

/**
	Checks sys for NULL and for integrity.
*/
static int check_system(conopt_system_t sys){

  if( sys == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL system handle");
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

/*------------------------------------------------------------------------------
  GENERAL INPUT/OUTPUT ROUTINES
*/

#define print_var_name(a,b,c) slv_print_var_name((a),(b)->slv,(c))
#define print_rel_name(a,b,c) slv_print_rel_name((a),(b)->slv,(c))

/*------------------------------------------------------------------------------
  DEBUG OUTPUT ROUTINES
*/

/**
	Output a hyphenated line.
*/
static void debug_delimiter(){
  CONSOLE_DEBUG("------------------------------------------------");
}

#if DEBUG

/**
	Output a vector.
*/
static void debug_out_vector(conopt_system_t sys
		,struct vec_vector *vec
){
  int32 ndx;
  CONSOLE_DEBUG("Norm = %g, Accurate = %s, Vector range = %d to %d\n",
    calc_sqrt_D0(vec->norm2), vec->accurate?"TRUE":"FALSE",
    vec->rng->low,vec->rng->high
  );
  CONSOLE_DEBUG("Vector --> ");
  for( ndx=vec->rng->low ; ndx<=vec->rng->high ; ++ndx )
    CONSOLE_DEBUG("%g ", vec->vec[ndx]);
}

/**
	Output all variable values in current block.
*/
static void debug_out_var_values(conopt_system_t sys){
  int32 col;
  struct var_variable *var;

  CONSOLE_DEBUG("Var values -->");
  for( col = sys->J.reg.col.low; col <= sys->J.reg.col.high ; col++ ) {
    var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
    print_var_name(ASCERR,sys,var); /** @TODO fix this */
    CONSOLE_DEBUG("I	Lb	Value	Ub	Scale	Col	INom");
    CONSOLE_DEBUG("%d\t%.4g\t%.4g\t%.4g\t%.4g\t%d\t%.4g",
      var_sindex(var),var_lower_bound(var),var_value(var),
      var_upper_bound(var),var_nominal(var),
      col,sys->nominals.vec[col]
    );
  }
}

/**
	Output all relation residuals in current block.
*/
static void debug_out_rel_residuals(conopt_system_t sys){
  int32 row;

  CONSOLE_DEBUG("Rel residuals -->");
  for( row = sys->J.reg.row.low; row <= sys->J.reg.row.high ; row++ ) {
    struct rel_relation *rel;
    rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
    CONSOLE_DEBUG("  %g : ",rel_residual(rel));
    print_rel_name(ASCERR,sys,rel); /** @TODO fix this */
  }
}


/**
	Output permutation and values of the nonzero elements in the
	the jacobian matrix.
*/
static void debug_out_jacobian(conopt_system_t sys){
  mtx_coord_t nz;
  real64 value;

  nz.row = sys->J.reg.row.low;
  for( ; nz.row <= sys->J.reg.row.high; ++(nz.row) ){
    CONSOLE_DEBUG("Row %d (rel %d)\n"
	  , nz.row, mtx_row_to_org(sys->J.mtx,nz.row)
    );
    nz.col = mtx_FIRST;

    while(
	  value = mtx_next_in_row(sys->J.mtx,&nz,&(sys->J.reg.col))
      , nz.col != mtx_LAST
	){
      CONSOLE_DEBUG("Col %d (var %d) has value %g\n", nz.col,
        mtx_col_to_org(sys->J.mtx,nz.col), value);
    }
  }
}

/**
	Output permutation and values of the nonzero elements in the
	reduced hessian matrix.
*/
static void debug_out_hessian( FILE *fp, conopt_system_t sys){
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

/*------------------------------------------------------------------------------
  ARRAY AND VECTOR OPERATIONS

	destroy_array(p)
	create_array(len,type)

	zero_vector(vec)
	copy_vector(vec1,vec2)
	prod = inner_product(vec1,vec2)
	norm2 = square_norm(vec)
	matrix_product(mtx,vec,prod,scale,transpose)
*/

#define destroy_array(p)  \
   if( (p) != NULL ) ascfree((p))
#define create_array(len,type)  \
   ((len) > 0 ? (type *)ascmalloc((len)*sizeof(type)) : NULL)
#define create_zero_array(len,type)  \
   ((len) > 0 ? (type *)asccalloc((len),sizeof(type)) : NULL)

#define zero_vector(v) vec_zero(v)
#define copy_vector(v,t) vec_copy((v),(t))
#define inner_product(v,u) vec_inner_product((v),(u))
#define square_norm(v)  vec_square_norm(v)
#define matrix_product(m,v,p,s,t) vec_matrix_product((m),(v),(p),(s),(t))


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

/**
	Count jacobian elements and set max to the number of elements
	in the densest row
*/
static int32 num_jacobian_nonzeros(conopt_system_t sys, int32 *max){
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


/**
	Evaluate the objective function.
*/
static boolean calc_objective( conopt_system_t sys){
  calc_ok = TRUE;
  asc_assert(sys->obj!=NULL);
  sys->objective = (sys->obj ? relman_eval(sys->obj,&calc_ok,SAFE_CALC) : 0.0);
  return calc_ok;
}

/**
	Evaluate all objectives.
*/
static boolean calc_objectives( conopt_system_t sys){
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
	  asc_assert(rlist[i]!=NULL);
      relman_eval(rlist[i],&calc_ok,SAFE_CALC);
#if DEBUG
      if (calc_ok == FALSE) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"error in calc_objectives");
        calc_ok = TRUE;
      }
#endif /* DEBUG */
    }
  }
  return calc_ok;
}

/**
	Calculate all of the residuals in the current block and compute
	the residual norm for block status.

	@return true iff calculations preceded without error.
*/
static boolean calc_residuals( conopt_system_t sys){
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
      ERROR_REPORTER_HERE(ASC_PROG_ERR
		,"NULL relation found at row %d rel %d in calc_residuals!",(int)row,r
      );
    }
#endif /* DEBUG */
    asc_assert(rel!=NULL);
    sys->residuals.vec[row] = relman_eval(rel,&calc_ok,SAFE_CALC);

    relman_calc_satisfied(rel,sys->p.tolerance.feasible);
  }
  sys->s.block.functime += (tm_cpu_time() -time0);
  sys->s.block.funcs++;
  square_norm( &(sys->residuals) );
  sys->s.block.residual = calc_sqrt_D0(sys->residuals.norm2);
  if(!calc_ok){
    CONOPT_CONSOLE_DEBUG("ERROR IN EVALUATION");
  }
  return(calc_ok);
}


/**
	Calculate the current block of the jacobian.
	It is initially unscaled.
*/
static boolean calc_J( conopt_system_t sys){
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

/**
	Retrieve the nominal values of all of the block variables,
	and ensure that they are all strictly positive.
*/
static void calc_nominals( conopt_system_t sys){
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

	  ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
	  FPRINTF(ASCERR,"Variable ");
	  print_var_name(fp,sys,var);
	  FPRINTF(ASCERR," has nominal value of zero.\n");
	  FPRINTF(ASCERR,"Resetting to %g.\n",n);
      error_reporter_end_flush();

	  var_set_nominal(var,n);
	} else {
	  n =  -n;

	  ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
	  FPRINTF(fp,"Variable ");
	  print_var_name(fp,sys,var);
	  FPRINTF(fp," has negative nominal value.\n");
	  FPRINTF(fp,"Resetting to %g.\n",n);
      error_reporter_end_flush();

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


/**
	Calculate the weights of all of the block relations
	to scale the rows of the Jacobian.
*/
static void calc_weights( conopt_system_t sys)
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


/**
	Scale the jacobian.
*/
static void scale_J( conopt_system_t sys){
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

/**
	@TODO document this
*/
static void jacobian_scaled(conopt_system_t sys){
  int32 col;
  if (DUMPCNORM) {
    for( col=sys->J.reg.col.low; col <= sys->J.reg.col.high; col++ ) {
      real64 cnorm;
      cnorm = calc_sqrt_D0(mtx_sum_sqrs_in_col(sys->J.mtx,col,&(sys->J.reg.row)));
      if (cnorm >CNHIGH || cnorm <CNLOW) {
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"[col %d org %d] %g\n", col,
          mtx_col_to_org(sys->J.mtx,col), cnorm
        );
      }
    }
  }

  sys->update.jacobian = UPDATE_JACOBIAN;
  sys->J.accurate = TRUE;
  sys->J.singular = FALSE;  /* yet to be determined */
#if DEBUG
  CONSOLE_DEBUG("Jacobian:");
  debug_out_jacobian(sys);
#endif /* DEBUG */
}

/**
	@TODO document this
*/
static void scale_variables( conopt_system_t sys)
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
  CONSOLE_DEBUG("Variables:");
  debug_out_vector(sys,&(sys->variables));
#endif /* DEBUG */
}


/*
 *  Scales the previously calculated residuals.
 */
static void scale_residuals( conopt_system_t sys)
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
  CONSOLE_DEBUG("Residuals:");
  debug_out_vector(sys,&(sys->residuals));
#endif  /* DEBUG */
}


/**
	Calculate relnoms for all relations in sys
	using variable nominals.
*/
static void calc_relnoms(conopt_system_t sys){
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
	Return the maximum ratio of magnitudes of any two nonzero
	elements in the same column of mtx.  Only consider elements
	in region reg.
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


/**
	Return the maximum ratio of magnitudes of any two nonzero
	elements in the same row of mtx.  Only consider elements
	in region reg.
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

/**
	Calculate scaling factor suggested by Fourer.

	For option==0, returns scaling factor for
	row number loc.

	For option==1, returns scaling factor for
	col number loc.
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

/**
	An implementation of the scaling routine by Fourer on
	p304 of Mathematical Programing vol 23, (1982).

	Scale the Jacobian and store the scaling
	factors in sys->nominals and sys->weights.
	If the Jacobian has been previously scaled
	by another method (during this iteration) then these vectors
	should contain the scale factors used in that scaling.
*/
static void scale_J_iterative(conopt_system_t sys){
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


/**
	Scale system dependent on interface parameters
*/
static void scale_system( conopt_system_t sys ){
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


/**
	Reset all flags to setup a new solve.
	Should set sys->s.block.current_block = -1
	before calling.

	@TODO This is currently a HACK! Not sure if should call when done.
*/
static void conopt_initialize( conopt_system_t sys){

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
      debug_delimiter();
    }
    if(sys->p.output.less_important && LIFDS) {
      CONSOLE_DEBUG("%-40s ---> %d in [%d..%d]"
        , "Current block number", sys->s.block.current_block
        , 0, sys->s.block.number_of-1
      );
      CONSOLE_DEBUG("%-40s ---> %d", "Current block size"
        , sys->s.block.current_size
      );
    }
    if( !(ok = calc_objective(sys)) ) {
         ERROR_REPORTER_HERE(ASC_PROG_ERR,"Objective calculation errors detected.");
    }
    if(sys->p.output.less_important && sys->obj) {
      CONSOLE_DEBUG("%-40s ---> %g", "Objective", sys->objective);
    }
    sys->s.calc_ok = sys->s.calc_ok && ok;

    if(!(sys->p.ignore_bounds) ) {
      slv_ensure_bounds(
          SERVER, sys->J.reg.col.low,
          sys->J.reg.col.high,MIF(sys)
      );
    }

    sys->residuals.accurate = FALSE;
    if( !(ok = calc_residuals(sys)) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Residual calculation errors detected.");
    }
    if(sys->p.output.less_important &&
        (sys->s.block.current_size >1 ||
        LIFDS)
    ){
      CONSOLE_DEBUG("%-40s ---> %g", "Residual norm (unscaled)",sys->s.block.residual);
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


/*------------------------------------------------------------------------------
  ITERATION BEGIN/END ROUTINES
*/

/**
	Prepare sys for entering an iteration, increasing the iteration counts
	and starting the clock.
*/
static void iteration_begins( conopt_system_t sys){
   sys->clock = tm_cpu_time();
   ++(sys->s.block.iteration);
   ++(sys->s.iteration);
   if(sys->p.output.less_important && LIFDS) {
     CONSOLE_DEBUG("%-40s ---> %d","Iteration", sys->s.block.iteration);
     CONSOLE_DEBUG("%-40s ---> %d","Total iteration", sys->s.iteration);
   }
}


/*
	Prepare sys for exiting an iteration, stopping the clock and recording
	the cpu time.
*/
static void iteration_ends( conopt_system_t sys){
   double cpu_elapsed;   /* elapsed this iteration */

   cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
   sys->s.block.cpu_elapsed += cpu_elapsed;
   sys->s.cpu_elapsed += cpu_elapsed;
   if(sys->p.output.less_important && LIFDS) {
     CONSOLE_DEBUG("%-40s ---> %g","Elapsed time", sys->s.block.cpu_elapsed);
     CONSOLE_DEBUG("%-40s ---> %g","Total elapsed time", sys->s.cpu_elapsed);
   }
}


/**
	Update the solver status.
 */
static void update_status( conopt_system_t sys){
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
int32 conopt_get_default_parameters(slv_system_t server, SlvClientToken asys
		, slv_parameters_t *parameters
){
  conopt_system_t sys;
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
    sys = CONOPT(asys);
    make_macros = 1;
  }

#if DEBUG  /* keep purify from whining on UMR */
  lo.argr = hi.argr = val.argr = 0.0;
#endif  /* DEBUG */

  if (parameters->parms == NULL) {
    /* an external client wants our parameter list.
     * an instance of conopt_system_structure has this pointer
     * already set in conopt_create
     */
    new_parms = ASC_NEW_ARRAY(struct slv_parameter, conopt_PA_SIZE);
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
	       "hessian_ub","RTMXJ2","Upper bound on 2nd derivatives",
	       U_p_real(val,1e4),U_p_real(lo,0),U_p_real(hi,MAX_REAL),2);

  slv_define_parm(parameters, real_parm,
	       "maxfeastol", "RTNWMA",
	       "Max. residual considered feasible (may be scaled)",
	       U_p_real(val, 1e-3),U_p_real(lo, 1e-13),U_p_real(hi,10e10),2);

  slv_define_parm(parameters, real_parm,
	       "minfeastol", "RTNWMI",
	       "Min. residual considered feasible",
	       U_p_real(val, 4e-10),U_p_real(lo, 1e-20),U_p_real(hi,10e10),2);

  slv_define_parm(parameters, real_parm,
	       "oneDsearch","RTONED","Accuracy of one dimensional search",
	       U_p_real(val,0.2),U_p_real(lo,0.1),U_p_real(hi,0.7),2);

  slv_define_parm(parameters, real_parm,
	       "stepmult","RVSTLM","Step-length multiplier",
	       U_p_real(val,4),U_p_real(lo,0),U_p_real(hi,MAX_REAL),2);

  slv_define_parm(parameters, real_parm,
	       "objtol","RTOBJR","Relative objective tolerance",
	       U_p_real(val,3e-13),U_p_real(lo,0),U_p_real(hi,1),2);

  slv_define_parm(parameters, real_parm,
	       "pivottol","RTPIVA","Absolute pivot tolerance",
	       U_p_real(val,1e-7),U_p_real(lo,1e-15),U_p_real(hi,1),2);

  slv_define_parm(parameters, real_parm,
	       "pivtolrel","RTPIVR","Relative pivot tolerance",
	       U_p_real(val,0.05),U_p_real(lo,0),U_p_real(hi,1),2);

  slv_define_parm(parameters, real_parm,
	       "opttol","RTREDG","Optimality tolerance",
	       U_p_real(val,2e-5),U_p_real(lo,0),U_p_real(hi,MAX_REAL),2);

  /* integer valued parameters */

  slv_define_parm(parameters, int_parm,
	       "log_freq","LFILOG","How often (in iterations) to write logging info",
	       U_p_int(val,10),U_p_int(lo,1),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "log_freq","LFILOS","How often to write logging info during SLP and SQP",
	       U_p_int(val,10),U_p_int(lo,1),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "iterationlimit", "LFITER", "Maximum number of iterations",
	       U_p_int(val, 1000),U_p_int(lo, 1),U_p_int(hi,MAX_INT),1);
  SLV_IPARM_MACRO(ITER_LIMIT_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "slowproglim","LFNICR","Limit for slow progress",
	       U_p_int(val,12),U_p_int(lo,2),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "maxhessdim","LFNSUP","Maximum Hessian dimension",
	       U_p_int(val,500),U_p_int(lo,5),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "supbasiclim","LFMXNS","Limit on new super-basics",
	       U_p_int(val,5),U_p_int(lo,0),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "lfscal","LFSCAL","Minimum frequency for updating row/col scales (see LSSCAL)",
	       U_p_int(val,20),U_p_int(lo,1),U_p_int(hi,MAX_INT),1);

   slv_define_parm(parameters, int_parm,
	       "lfstal","LFSTAL","Upper bound on the number of stalled iterations",
	       U_p_int(val,100),U_p_int(lo,2),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "lkdebg","LKDEBG","How often (in iterations) to write debugging info for derivatives",
	       U_p_int(val,0),U_p_int(lo,-1),U_p_int(hi,MAX_INT),1);
  
  slv_define_parm(parameters, int_parm,
	       "lkdeb2","LKDEB2","How often (in iterations) to write debugging info for second derivs",
	       U_p_int(val,0),U_p_int(lo,-1),U_p_int(hi,MAX_INT),1);

  slv_define_parm(parameters, int_parm,
	       "lmdebg","LMDEBG","Func/derivative debugging: 0=default, 1=additional continuity tests",
	       U_p_int(val,0),U_p_int(lo,0),U_p_int(hi,1),1);

  slv_define_parm(parameters, int_parm,
	       "lmmxsf","LMMXSF","Method used to calc max step during Phase 0: 0=default, 1=new",
	       U_p_int(val,0),U_p_int(lo,0),U_p_int(hi,1),1);

  slv_define_parm(parameters, int_parm,
	       "lmmxst","LMMXST","'As for LMMXSF but for when tolerances are tightened'",
	       U_p_int(val,0),U_p_int(lo,0),U_p_int(hi,1),1);

  slv_define_parm(parameters, int_parm,
	       "errlim","max # func errs",
	       "Limit on number of function evaluation errors",
	       U_p_int(val,500),U_p_int(lo,0),U_p_int(hi,MAX_INT),1);
  SLV_IPARM_MACRO(DOMLIM_PTR,parameters);

  slv_define_parm(parameters, char_parm,
	       "scaleopt", "scaling option", "Scaling option",
	       U_p_string(val,scaling_names[1]),
	       U_p_strings(lo,scaling_names),
	       U_p_int(hi,sizeof(scaling_names)/sizeof(char *)),3);
  SLV_CPARM_MACRO(SCALEOPT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "lifds", "show singletons details", "Show singletons details",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 3);
  SLV_BPARM_MACRO(LIFDS_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "safe_calc", "safe calculations", "Safe calculations",
	       U_p_bool(val, 1),U_p_bool(lo,0),U_p_bool(hi,1), 3);
  SLV_BPARM_MACRO(SAFE_CALC_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "toosmall", "default for zero nominal",
               "Default for zero nominal",
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
               "Largest allowable column norm",
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
	       "reorder", "reorder method", "Re-order method",
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
	       "relnomscale", "calc rel nominals", "Whether to calculate relation nominals?",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 3);
  SLV_BPARM_MACRO(RELNOMSCALE_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "timelimit", "time limit (CPU sec/block)",
               "Time limit (CPU sec/block)",
	       U_p_int(val,1500),U_p_int(lo, 1),U_p_int(hi,20000),1);
  SLV_IPARM_MACRO(TIME_LIMIT_PTR,parameters);



  // CONOPT boolean options

  slv_define_parm(parameters, bool_parm,
      "ls2ptj", "LS2PTJ", "Allow computation of 2nd derivatives by peturbation",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lsanrm", "LSANRM", "Use 'steepest edge' procedure",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lscrsh", "LSCRSH", "Use Crash procedures to create initial basis",
	  U_p_bool(val, 1),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lseslp", "LSESLP", "Enable SLP mode",
	  U_p_bool(val, 1),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lsismp", "LSISMP", "Ignore small pivots",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lslack", "LSLACK", "Use the set of all slacks as the initial basis",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lspret", "LSPRET", "Identify and solve pre-triangular equations",
	  U_p_bool(val, 1),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lspost", "LSPOST", "Identify post-triangular equations (that can combine with the Objective)",
	  U_p_bool(val, 1),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lssqrs", "LSSQRS", "Modeller declares that this is a square system (c.f. COIDEF_Square)",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lsscal", "LSSCAL", "Use dynamic scaling algorithm (NB, manual scaling is preferred)",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lstcrs", "LSTCRS", "Try to crash triangular bases using Gould & Reid technique",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  slv_define_parm(parameters, bool_parm,
      "lstria", "LSTRIA", "Modeller declares that the equations form a triangular or recursive system",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 4);

  // Quick mode

  slv_define_parm(parameters, bool_parm,
      "lsnop2", "LSNOP2", "No \"Phase 2\"",
	  U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 5);

  slv_define_parm(parameters, int_parm,
      "lfmxp4","LFMXP4","Maximum number of iterations in Phase 4",
	       U_p_int(val,1000000000),U_p_int(lo,1),U_p_int(hi,MAX_INT),5);

  slv_define_parm(parameters, real_parm,
      "rvobjl", "RVOBJL","Limit on objective in Quick Mode",
	       U_p_real(val, 0),U_p_real(lo,0),U_p_real(hi,10e10), 5);
	
  asc_assert(parameters->num_parms==conopt_PA_SIZE);

  return 1;
}


/*-----------------------------------------------------------------------------
  EXTERNAL ROUTINES (see slv_client.h)
*/

static SlvClientToken conopt_create(slv_system_t server, int32*statusindex){
  conopt_system_t sys;

  sys = ASC_NEW_CLEAR(struct conopt_system_structure);
  if (sys==NULL) {
    *statusindex = 1;
    return sys;
  }
  SERVER = server;
  sys->p.parms = sys->pa;
  sys->p.dynamic_parms = 0;
  conopt_get_default_parameters(server,(SlvClientToken)sys,&(sys->p));
  sys->integrity = OK;
  sys->presolved = 0;
  sys->resolve = 0;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = stdout;

  sys->p.whose = (*statusindex);

  sys->con.work=NULL;

  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.costsize = 0;
  sys->s.cost = NULL; /*redundant, but sanity preserving */
  sys->vlist = slv_get_solvers_var_list(server);
  sys->rlist = slv_get_solvers_rel_list(server);
  sys->obj = slv_get_obj_relation(server);
  if (sys->vlist == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"CONOPT called with no variables.");
    *statusindex = -2;
    return NULL; /* prolly leak here */
  }
  if (sys->rlist == NULL && sys->obj == NULL) {
    ascfree(sys);
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"CONOPT called with no relations or objective.");
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

static void destroy_matrices( conopt_system_t sys){
   if( sys->J.mtx ) {
     mtx_destroy(sys->J.mtx);
   }
}

static void destroy_vectors( conopt_system_t sys){
   destroy_array(sys->nominals.vec);
   destroy_array(sys->weights.vec);
   destroy_array(sys->relnoms.vec);
   destroy_array(sys->variables.vec);
   destroy_array(sys->residuals.vec);
}


static int32 conopt_eligible_solver(slv_system_t server){
	struct rel_relation **rp;
	for( rp=slv_get_solvers_rel_list(server); *rp != NULL ; ++rp ) {
		if( rel_less(*rp) || rel_greater(*rp) ){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"less-than and greater-than relations are not permitted with CONOPT");
			return(FALSE);
		}
	}
	return(TRUE);
}


static void conopt_get_parameters(slv_system_t server, SlvClientToken asys
		, slv_parameters_t *parameters
){
  conopt_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = CONOPT(asys);
  if (check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}


static void conopt_set_parameters(slv_system_t server, SlvClientToken asys
		,slv_parameters_t *parameters
){
  conopt_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = CONOPT(asys);
  if (check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}


static int conopt_get_status(slv_system_t server, SlvClientToken asys
		,slv_status_t *status
){
	conopt_system_t sys;
	(void)server;  /* stop gcc whine about unused parameter */

	sys = CONOPT(asys);
	if (check_system(sys)) return 1;
	mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
	return 0;
}


static linsolqr_system_t conopt_get_linsolqr_sys(slv_system_t server
		,SlvClientToken asys
){
  conopt_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = CONOPT(asys);
  if (check_system(sys)) return NULL;
  return(sys->J.sys);
}

/**
	Perform structural analysis on the system, setting the flags in
	status.

	The problem must be set up, the relation/variable list
	must be non-NULL. The jacobian (linear) system must be created
	and have the correct order (stored in sys->cap).  Everything else
	will be determined here.

	On entry there isn't yet a correspondence between var_sindex and
	jacobian column. Here we establish that.

	@NOTE this function has been striped of its guts for CONOPT and may go away
*/
static void structural_analysis(slv_system_t server, conopt_system_t sys){

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
  sys->J.reg.row.high = sys->con.m - 1;
  if (sys->obj != NULL) sys->J.reg.row.high--;
  sys->J.reg.col.high = sys->con.n - 1;

  if(slv_check_bounds(SERVER,sys->vused,-1,"fixed ")){
    sys->s.inconsistent = 1;
  }

  /* Initialize Status */
  sys->s.over_defined = (sys->rused > sys->vused);
  sys->s.under_defined = (sys->rused < sys->vused);
  sys->s.struct_singular = (sys->rank < sys->rused);
  sys->s.block.number_of = (slv_get_solvers_blocks(SERVER))->nblocks;

}


static void create_matrices(slv_system_t server, conopt_system_t sys){
  sys->J.mtx = mtx_create();
  mtx_set_order(sys->J.mtx,sys->cap);
  structural_analysis(server,sys);
}


static void create_vectors(conopt_system_t sys){
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


static void conopt_dump_internals(slv_system_t server
		, SlvClientToken sys, int32 level
){
  (void)server;  /* stop gcc whine about unused parameter */

  check_system(sys);
  if (level > 0) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Can't dump internals with CONOPT");
  }
}


/**
	Check if any fixed or included flags have
	changed since the last presolve.
*/
static int32 conopt_dof_changed(conopt_system_t sys)
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


/**
	Update the values of the array cost, which is used by the interface
	to display residual and number of iterations. For use after running CONOPT
*/
static void update_cost(conopt_system_t sys)
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

/*------------------------------------------------------------------------------
	CONOPT ROUTINES
*/

/**
	@TODO
	  - Fix interface so that solvers define status messages. We
	    should not be stuck with one standard set that all solvers
	    must deal with.

	  - Reimplement old code to detect linear coefficients and use
	    in conopt hookup.

	  - Implement better communication routines.

	  - Give more contol to interface (ex. turn off error counting,
	    switch from we alocate to conopt allocates, etc.).

	  - Record marginal values and make available to user.

	  - Set up interface such that any variable can be selected and
	    either maximized or minimized.

	  - Get rid of our Jacobian calculation routine and stuff conopt's
	    workspace directly (in unsorted order). This will require
	    rewriting the scaling functions. (This code really should
	    not be in the solver)

	  - Fix up restart code...we don't keep track of which values
	    change so must update entire Jacobian and residual vector
	    but may save some time by not having to redo column pointers etc.

	  - Implement a way to bailout...check for ^C and tell conopt to
	    return as soon as possible.

	  - Currently will not take problem like MIN x^2...it will complain
	    about empty rows.  Must formulate as y=x^2; MIN y; until we
	    fix the way we handle objectives.
*/


/**
	This is the 'ReadMatrix' callback. The provides the mechanism for ASCEND
	to tell CONOPT about the lower and upper bounds on variables, the initial
	values, the initial basic/non-basis status of variables, the types of
	equation constraints and the values of the RHSes, and information about the
	structure of the equations.

	See the CONOPT reference manual for full details.

	@param lower   lower bounds on the variables
	@param curr    intial values of the variables
	@param upper   upper bounds on the variables
	@param vsta    initial status of the variable(o nonbasic, 1 basic)
	@param type    types of equations (0 equality,1 greater,2 less)
	@param rhs     values of the right hand sides
	@param esta    initial status of the slack in the constraint (nonbasic,basic)
	@param colsta  start of column pointers
	@param rowno   row or equation numbers of the nonzeros
	@param value   values of the jacobian elements
	@param nlflag  nonlinearity flags(0 nonzero constant,1 varying)
	@param n       number of variables
	@param m       number of constraints
	@param nz      number of jacobian elements
	@param usrmem  user memory defined by conopt
*/
static int COI_CALL conopt_readmatrix(
		double *lower, double *curr, double *upper
		, int *vsta,  int *type, double *rhs
		, int *esta,  int *colsta, int *rowno
		, double *value, int *nlflag, int *n, int *m, int *nz
		, double *usrmem
){
  int32 col,row,count,count_old,len,c,r,offset, obj_count;
  real64 nominal, up, low;
  struct var_variable *var;
  const struct rel_relation **rlist=NULL;
  static rel_filter_t rfilter;
  static var_filter_t vfilter;
  real64 *derivatives;
  int32 *variables;
  mtx_coord_t coord;
  conopt_system_t sys;

  /*
    stop gcc whining about unused parameter
  */
  (void)vsta;  (void)rhs;   (void)esta;  (void)n;

  sys = (conopt_system_t)usrmem;
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

    lower[col-offset] = low > -CONOPT_BOUNDLIMIT ? low : -CONOPT_BOUNDLIMIT;
    upper[col-offset] = up < CONOPT_BOUNDLIMIT ? up : CONOPT_BOUNDLIMIT;
	CONSOLE_DEBUG("BOUNDS for var %d: [%g,%g]",col-offset,lower[col-offset],upper[col-offset]);
    curr[col-offset] = sys->variables.vec[col]; /* already scaled */
    vsta[col-offset] = !var_nonbasic(var);
  }
  /*
  for (offset = row = sys->J.reg.row.low;
       row <= sys->J.reg.row.high; row++) {

    rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
    nominal = sys->weights.vec[row];
    * fv[row-offset] = sys->residuals.vec[row];* * already scaled *
  }
  */

  /* set relation types: all equalities except for last one */
  for (row = 0; row < *m; row++) {
    type[row] = 0;
  }
  if (sys->obj != NULL) {
    type[*m - 1] = 3; /* objective function */
  }

  /* get derivatives of objective function? */
  if (sys->obj != NULL) {
    len = rel_n_incidences(sys->obj);
    variables = ASC_NEW_ARRAY(int32,len);
    derivatives = ASC_NEW_ARRAY(real64,len);

    relman_diff2(
        sys->obj,&vfilter,derivatives,variables
	    , &(obj_count),SAFE_CALC
    );
	/* what about checking error code? -- JP */
  }

  count = count_old = 0;

  colsta[0] = 0;

  for(offset = col = sys->J.reg.col.low
      ; col <= sys->J.reg.col.high
      ; col++
  ){
    coord.col = col;
    var = sys->vlist[col];
#if CONDBG
    if (!var_apply_filter(var,&vfilter) ) {
      CONSOLE_DEBUG("var doesn't pass filter");
    }
#endif /* CONDBG */
    len = var_n_incidences(var);
    rlist = var_incidence_list(var);
    count_old = count;
    for (c=0; c < len; c++) {
      /* assuming obj on list... check this */
      if (rel_apply_filter(rlist[c],&rfilter)) {
		coord.row = rel_sindex(rlist[c]);
		rowno[count] = (rel_sindex(rlist[c]) - offset);
		value[count] = mtx_value(sys->J.mtx,&coord);
		nlflag[count] = 1;               /* fix this later */
		if(rlist[c] == sys->obj) {
#if CONDBG
		  CONSOLE_DEBUG("found objective in unexpected location");
#endif /* CONDBG */
		}
        if (fabs(value[count]) > RTMAXJ) {
#if CONDBG
		  CONSOLE_DEBUG("Large Jacobian value being set to RTMAXJ");
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
		    rowno[count] = *m - 1;
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
	  /* CONSOLE_DEBUG("COLSTA[%d] = %d",col-offset,count_old); */
      colsta[col - offset] = count_old;
    }
  }
  /* CONSOLE_DEBUG("COLSTA[%d] = %d",*n,*nz + 1); */
  colsta[*n] = *nz;
  if (sys->obj != NULL) {
    ascfree(variables);
    ascfree(derivatives);
  }

  return 0;
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
static void conopt_coifbl(real64 *x, real64 *g, int32 *otn, int32 *nto,
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
  conopt_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)nto;  (void)stcl;   (void)rnum;  (void)nl;
  (void)errcnt;  (void)n1;   (void)m1;  (void)nz;

  sys = (conopt_system_t)usrmem;

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
	FPRINTF(MIF(sys),"conopt_coifbl: ERROR IN OBJECTIVE CALCULATION\n");
      }
    }
  }
  jac_row = (int32 *)ascmalloc((*n)*sizeof(int32));
  if (*mode == 2 || *mode == 3) {
    len = sys->con.maxrow;
    variables = ASC_NEW_ARRAY(int32,len);
    derivatives = ASC_NEW_ARRAY(real64,len);
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
	COIFDE Defines the nonlinearities of the model by returning
	numerical values. It works on one row or equation at a time

	@param x      punt of evaluation provided by conopt
	@param g      function value
	@param jac    jacobian values
	@param rowno  number of the row for which nonlinearities will be eval
	@param jcnm   list of column number fon the NL nonzeros
	@param mode   indicator of mode of evaluation, 1=G, 2=JAC, 3=G & JAC
	@param ignerr ???
	@param errcnt sum of number of func evaluation errors thus far
	@param newpt  new point indicator
	@param nj     number of nonlinear nonzero jacobian elements
	@param n      number of variables
	@param usrmem user memory
*/
static int COI_CALL conopt_fdeval(
		double *x, double *g, double *jac
		, int *rowno, int *jcnm, int *mode, int *ignerr
		, int *errcnt, int *newpt, int *n, int *nj
		, double *usrmem
){
  int32 offset, col, row, len, c;
  real64 nominal, value;
  struct var_variable *var;
  struct rel_relation *rel;
  int32 *variables;
  real64 *derivatives;
  static var_filter_t vfilter;
  conopt_system_t sys;
  int status;

  /* stop gcc whining about unused parameter */
  (void)jcnm;  (void)n;   (void)nj;

  CONOPT_CONSOLE_DEBUG("EVALUATION STARTING (row=%d, n=%d, nj=%d)",*rowno,*n,*nj);

  sys = (conopt_system_t)usrmem;
  if (*newpt == 1) {
	/* CONSOLE_DEBUG("NEW POINT"); */
	/* a new point */
    for (offset = col = sys->J.reg.col.low;
	 col <= sys->J.reg.col.high; col++) {
      var = sys->vlist[col];
      nominal = sys->nominals.vec[col];
      value = x[col-offset] * nominal;
      var_set_value(var, value);
    }
  }
  /**
	@TODO could be more efficient when mode = 3
	(with future versions of CONOPT)
  */
  if (*mode == 1 || *mode == 3) {
	CONOPT_CONSOLE_DEBUG("FUNCTION VALUES");
    offset =  sys->J.reg.row.low;
    row = *rowno + offset;
	CONOPT_CONSOLE_DEBUG("ROWNO = %d, OFFSET = %d: ROW = ROW = %d",*rowno, offset, row);
    if ((*rowno == sys->con.m - 1) && (sys->obj != NULL)){
      if(calc_objective(sys)){
		*g = sys->objective;
      }else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in calculation of objective.");
      }
    }else{
	  rel = sys->rlist[row];
	  asc_assert(rel!=NULL);
	  *g = relman_eval(rel,&calc_ok,SAFE_CALC)
	  * sys->weights.vec[row];
	  if (!calc_ok) {
        CONOPT_CONSOLE_DEBUG("EVALUATION ERROR IN RELMAN_EVAL");
		(*errcnt)++;
	  }
    }
  }
  if (*mode == 2 || *mode == 3) {
	CONOPT_CONSOLE_DEBUG("JACOBIAN VALUES");
    len = sys->con.maxrow;
    variables = ASC_NEW_ARRAY(int32,len);
    derivatives = ASC_NEW_ARRAY(real64,len);
    vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED);
    vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

    offset =  sys->J.reg.row.low;
    row = *rowno + offset;
    if ((*rowno == sys->con.m - 1) && (sys->obj != NULL)){
      rel = sys->obj;
	  asc_assert(rel!=NULL);
      status = relman_diff2(rel,&vfilter,derivatives,variables,
		   &(len),SAFE_CALC);
      for (c = 0; c < len; c++) {
		jac[variables[c]] = derivatives[c] *  sys->nominals.vec[variables[c]];
		CONOPT_CONSOLE_DEBUG("Jacobian for row %d, var %d = %f",*rowno,variables[c],jac[variables[c]]);
      }
      if(status){
		CONOPT_CONSOLE_DEBUG("ERROR IN JACOBIAN EVALUATION (OBJECTIVE) (%d)",status);
		(*errcnt)++;
      }
    }else{
      CONOPT_CONSOLE_DEBUG("NOT LAST ROW");
      rel = sys->rlist[mtx_row_to_org(sys->J.mtx,row)];
	  asc_assert(rel!=NULL);
      status = relman_diff2(rel,&vfilter,derivatives,variables,
		   &(len),SAFE_CALC);
      for (c = 0; c < len; c++) {
		jac[variables[c]] = derivatives[c]
		  * sys->weights.vec[row] *  sys->nominals.vec[variables[c]];
		CONOPT_CONSOLE_DEBUG("Jacobian for row %d, var %d = %f",mtx_row_to_org(sys->J.mtx,row),variables[c],jac[variables[c]]);
      }
      if(status){
		CONOPT_CONSOLE_DEBUG("ERROR IN JACOBIAN EVALUATION (%d)",status);		
		(*errcnt)++;
      }
    }
    for (c = 0; c < len; c++) {
      if(fabs(jac[variables[c]]) > RTMAXJ) {
		CONOPT_CONSOLE_DEBUG("large jac element");
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
  return 0;
}


/**
	COISTA Pass the solution from CONOPT to the modeler. It returns
	completion status

	@param modsta model status
	@param solsta solver status
	@param iter   number of iterations
	@param objval objective value
	@param usrmem user memory
*/
static int COI_CALL conopt_status(
		int32 *modsta, int32 *solsta, int32 *iter
		, real64 *objval, real64 *usrmem
){
  conopt_system_t sys;
  sys = (conopt_system_t)usrmem;

  /* for later access from elsewhere */
  sys->con.modsta = *modsta;
  sys->con.solsta = *solsta;
  sys->con.iter = *iter;
  sys->con.obj = sys->objective = *objval;

  asc_conopt_status(modsta,solsta,iter,objval,usrmem);

  return 0;
}


/**
	COIRS passes the solution from CONOPT to the modeler. It returns
	solution values

	@param xval   - the solution values of the variables
	@param xmar   - corresponding marginal values
	@param xbas   - basis indicator for column (at bound, basic, nonbasic)
	@param xsta   - status of column (normal, nonoptimal, infeasible,unbounded)
	@param yval   - values of the left hand side in all the rows
	@param ymar   - corresponding marginal values
	@param ybas   - basis indicator for row
	@param ysta   - status of row
	@param n      - number of variables
	@param m      - number of constraints
	@param usrmem - user memory
*/
static int COI_CALL conopt_solution(
		double *xval, double *xmar, int *xbas, int *xsta,
		double *yval, double *ymar, int *ybas, int * ysta,
		int *n, int *m, double *usrmem
){
  int32 offset, col, c;
  real64 nominal, value;
  struct var_variable *var;
  conopt_system_t sys;

  struct var_variable **vp;
  char *varname;
  const char *varstat;

  /*
   * stop gcc whining about unused parameter
   */
  (void)xmar;  (void)xsta;  (void)yval;
  (void)ymar;  (void)ysta;  (void)ybas;    (void)m;

  sys = (conopt_system_t)usrmem;
  offset = sys->J.reg.col.low;

  /* the values returned... */
  vp=slv_get_solvers_var_list(SERVER);
  for(c = 0; c < *n; ++c){
	col = c + offset;
    var = sys->vlist[mtx_col_to_org(sys->J.mtx,col)];
	nominal = sys->nominals.vec[col];
    value = xval[c]*nominal;
    varname = var_make_name(SERVER,var);
    /* pass the value back to ASCEND */
	var_set_value(var,value);
    /* pass the variable status (basic, nonbasic) back to ASCEND */
	switch(xbas[c]){
		case 0: varstat = "at lower bound"; break;
		case 1: varstat = "at upper bound"; break;
		case 2: varstat = "basic"; var_set_nonbasic(var,FALSE); break;
		case 3: varstat = "super-basic"; break;
    }
    if(xbas[c] != 2){
	  var_set_nonbasic(var,TRUE);
	}

	CONOPT_CONSOLE_DEBUG("%d: %s = %f (%s)",c,varname,value,varstat);
	ASC_FREE(varname);
  }

  /* should pull out additional info here */
  return 0;
}

#if 0 /* think that this is removed from the API now */
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
static void conopt_coiusz(int32 *nintg, int32 *ipsz, int32 *nreal,
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

$if 0
  conopt_system_t sys;

  /*
   * To Ken: This was in the subroutine before. But all the values
   * are the same as in coipsz. So, no redefintion is necessary since
   * the defaults contain the same information
   */

  /*
   * stop gcc whining about unused parameter
   */
  (void)nintg;  (void)nreal;

  sys = (conopt_system_t)usrmem;

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

$endif

 return;
}
#endif


/**
	COIOPT communicates non-default option values to CONOPT

	@param name   - the name of a CONOPT CR-cell defined by the modeler
	@param rval   - the value to be assigned to name if the cells contains a real
	@param ival   - the value to be assigned to name if the cells contains an int
	@param lval   - the value to be assigned to name if the cells contains a log value
	@param usrmem - user memory
*/
static int COI_CALL conopt_option(
		int *NCALL, double *rval, int *ival, int *logical
	    , double *usrmem, char *name, int lenname
){
  conopt_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)logical;

  sys = (conopt_system_t)usrmem;
  name = memset(name,' ',8);
  while (sys->con.opt_count < conopt_PA_SIZE) {
    if (strlen(sys->p.parms[sys->con.opt_count].interface_label) == 6){
      if(0==strncmp(sys->p.parms[sys->con.opt_count].interface_label,"R",1)){
		/* real-valued (R*) parameter */
		name = strncpy(name, sys->p.parms[sys->con.opt_count].interface_label,6);
		*rval = sys->p.parms[sys->con.opt_count].info.r.value;
        CONOPT_CONSOLE_DEBUG("Set real option '%s' to %f"
			,sys->p.parms[sys->con.opt_count].interface_label,*rval
		);
		sys->con.opt_count++;
		return 0;
      }else if(0==strncmp(sys->p.parms[sys->con.opt_count].interface_label,"LS",2)){
		/* boolean-balued (LS*) parameter */
        name = strncpy(name,sys->p.parms[sys->con.opt_count].interface_label,6);
		*logical = sys->p.parms[sys->con.opt_count].info.b.value;
        CONOPT_CONSOLE_DEBUG("Set bool option '%s' to %s"
			,sys->p.parms[sys->con.opt_count].interface_label,((*logical)?"TRUE":"FALSE")
		);
		sys->con.opt_count++;
		return 0;
      }else if(0==strncmp(sys->p.parms[sys->con.opt_count].interface_label,"L",1)){
		/* integer-valued (L*) parameter */
		name = strncpy(name,sys->p.parms[sys->con.opt_count].interface_label,6);
		*ival = sys->p.parms[sys->con.opt_count].info.i.value;
        CONOPT_CONSOLE_DEBUG("Set int option '%s' to %d"
			,sys->p.parms[sys->con.opt_count].interface_label,*ival
		);
		sys->con.opt_count++;
		return 0;
      }
    }
    sys->con.opt_count++;
  }

  /* sending blank to quit iterative calling */
  name = memset(name,' ',8);

  return 0;
}

int COI_CALL conopt_errmsg( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN
		, double* USRMEM, char* MSG, int LENMSG
){
	conopt_system_t sys;
	char *relname=NULL, *varname=NULL;
	struct var_variable **vp;
	struct rel_relation **rp;

	sys = (conopt_system_t)USRMEM;


	if(*COLNO!=-1){
		vp=slv_get_solvers_var_list(SERVER);
		vp = vp + (*COLNO + sys->J.reg.col.low);
		asc_assert(*vp!=NULL);
		varname= var_make_name(SERVER,*vp);
	}
	if(*ROWNO!=-1){
		rp=slv_get_solvers_rel_list(SERVER);
		rp = rp + (*ROWNO + sys->J.reg.row.low);
		if(*rp!=NULL){
			relname = rel_make_name(SERVER,*rp);
		}
	}

	ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
	if(*ROWNO == -1){
	    FPRINTF(ASCERR,"Variable '%s' : ",varname);
		ASC_FREE(varname);
	}else if(*COLNO == -1 ){
	    FPRINTF(ASCERR,"Relation '%s' : ",relname);
		if(relname)ASC_FREE(relname);
	}else{
	    FPRINTF(ASCERR,"Variable '%s' appearing in relation '%s' : ",varname,relname);
		ASC_FREE(varname);
		if(relname)ASC_FREE(relname);
	}
	FPRINTF(ASCERR,"%*s", *MSGLEN, MSG);
	error_reporter_end_flush();
	return 0;
}

/**
	Note that the COI_Solve declaration is a clever wrapping of CONOPT to allow
	dlopening of the CONOPT DLL/SO, rather than dynamic linking, since CONOPT
	will not always be available.
*/
static void slv_conopt_iterate(conopt_system_t sys)
{
  int retcode;
  /*
	We pass the pointer to sys as 'usrmem'.
	Cast back to slv9_system_t to access the information required
  */
  COIDEF_UsrMem(sys->con.cntvect, (double *)sys);

  sys->con.opt_count = 0; /* reset count on conopt_coiopt calls */
  sys->con.progress_count = 0; /* reset count on coiprg calls */

  sys->con.kept = 1;

  retcode = COI_Solve(sys->con.cntvect);

  CONOPT_CONSOLE_DEBUG("CONOPT COI_Solve return code %d",retcode);

  /* conopt_start(&(sys->con.kept), usrmem, &(sys->con.lwork),
	 sys->con.work, &(sys->con.maxusd), &(sys->con.curusd)); */

  if(sys->con.solsta == 1 && sys->con.modsta == 1){
	  sys->con.optimized = 1;
  }else{
	sys->con.optimized = 0;
  }
}

/**
	Function created to provide the interface with the correct values
	for number of iterations, residuals, solved variables, etc
*/
static void update_block_information(conopt_system_t sys)
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


static int conopt_presolve(slv_system_t server, SlvClientToken asys){
  struct var_variable **vp;
  struct rel_relation **rp;
  int32 cap, ind;
  int32 matrix_creation_needed = 1;
  conopt_system_t sys;
  int *cntvect, temp;

  CONOPT_CONSOLE_DEBUG("PRESOLVE");

  sys = CONOPT(asys);
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
    if(!conopt_dof_changed(sys) /*no changes in fixed or included flags*/
         && sys->p.partition == sys->J.old_partition
         && sys->obj == sys->old_obj
    ){
      matrix_creation_needed = 0;
      CONOPT_CONSOLE_DEBUG("YOU JUST AVOIDED MATRIX DESTRUCTION/CREATION");
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

	COIDEF_ReadMatrix(cntvect, &conopt_readmatrix);
	COIDEF_FDEval(cntvect, &conopt_fdeval);
	COIDEF_Option(cntvect, &conopt_option);
	COIDEF_Solution(cntvect, &conopt_solution);
	COIDEF_Status(cntvect, &conopt_status);
	COIDEF_Message(cntvect, &asc_conopt_message);
	COIDEF_ErrMsg(cntvect, &conopt_errmsg);
	COIDEF_Progress(cntvect, &asc_conopt_progress);

	int debugfv = 1;
	COIDEF_DebugFV(cntvect, &debugfv);

#if 0 /* these are the parameters we need to pass to CONOPT */
  ipsz[F2C(4)] = 0;             /* FIX THESE AT A LATER DATE!!!!  */
  if (sys->obj != NULL) {
    ipsz[F2C(6)] = relman_obj_direction(sys->obj);
    ipsz[F2C(7)] = sys->con.m;    /* objective will be last row     */
  } else {
    ipsz[F2C(7)] = 0;
  }
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
#endif

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

  return 0;
}

/**
	@TODO check this: not sure if 'resolve' is really working or not -- JP
*/
static int conopt_resolve(slv_system_t server, SlvClientToken asys){
  struct var_variable **vp;
  struct rel_relation **rp;
  conopt_system_t sys;
  (void)server;  /* stop gcc whine about unused parameter */

  sys = CONOPT(asys);

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
  return 0;
}

/**
	@TODO document this
*/
static int conopt_iterate(slv_system_t server, SlvClientToken asys){
  conopt_system_t sys;
  FILE              *mif;
  FILE              *lif;
  sys = CONOPT(asys);
  mif = MIF(sys);
  lif = LIF(sys);
  if (server == NULL || sys==NULL) return -1;
  if (check_system(CONOPT(sys))) return -2;
  if( !sys->s.ready_to_solve ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not ready to solve.");
    return 1;
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
    debug_delimiter();
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
  }

  return 0;
}

/**
	@TODO document this
*/
static int conopt_solve(slv_system_t server, SlvClientToken asys){
  conopt_system_t sys;
  int err = 0;
  sys = CONOPT(asys);
  if (server == NULL || sys==NULL) return -1;
  if (check_system(sys)) return -2;
  while( sys->s.ready_to_solve )err = err | conopt_iterate(server,sys);
  return err;
}

/**
	@TODO document this
*/
static mtx_matrix_t conopt_get_jacobian(slv_system_t server, SlvClientToken sys){
  if (server == NULL || sys==NULL) return NULL;
  if (check_system(CONOPT(sys))) return NULL;
  return CONOPT(sys)->J.mtx;
}

/**
	@TODO document this
*/
static int32 conopt_destroy(slv_system_t server, SlvClientToken asys){
  conopt_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)server;

  sys = CONOPT(asys);
  if (check_system(sys)) return 1;
  destroy_vectors(sys);
  destroy_matrices(sys);
  slv_destroy_parms(&(sys->p));
  sys->integrity = DESTROYED;
  if(sys->s.cost){
	ASC_FREE(sys->s.cost);
  }

  if(sys->con.work != NULL){
    ASC_FREE(sys->con.work);
    sys->con.work = NULL;
  }
  ascfree( (POINTER)asys );
  return 0;
}

static const SlvFunctionsT conopt_internals = {
	88
	,"CONOPT"
	,conopt_create
  	,conopt_destroy
	,conopt_eligible_solver
	,conopt_get_default_parameters
	,conopt_get_parameters
	,conopt_set_parameters
	,conopt_get_status
	,conopt_solve
	,conopt_presolve
	,conopt_iterate
	,conopt_resolve
	,conopt_get_linsolqr_sys
	,conopt_get_jacobian
	,conopt_dump_internals
};

int conopt_register(void){
#ifndef ASC_LINKED_CONOPT
	if(asc_conopt_load()){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to load CONOPT");
		return 1;
	}
#endif
	return solver_register(&conopt_internals);
}

#endif /* ASC_WITH_CONOPT */
