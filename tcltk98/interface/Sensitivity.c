/*
 *  Sensitivity.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.31 $
 *  Version control file: $RCSfile: Sensitivity.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */


/*
 *  Sensititvity analysis code.
 */

#include <math.h>
#include "tcl.h"
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/tm_time.h"
#include "general/list.h"
#include "solver/slv_types.h"
#include "solver/mtx.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/logrel.h"
#include "solver/bnd.h"
#include "solver/slv_common.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/linutils.h"
#include "solver/calc.h"
#include "solver/relman.h"
#include "solver/slv_client.h"
#include "solver/system.h"
#include "interface/HelpProc.h"
#include "interface/Sensitivity.h"
#include "interface/HelpProc.h"
#include "interface/SolverGlobals.h"

#ifndef lint
static CONST char SensitivityID[] = "$Id: Sensitivity.c,v 1.31 2003/08/23 18:43:08 ballan Exp $";
#endif


#define SAFE_FIX_ME 0

#define DOTIME FALSE
#define DEBUG 1
/*
 * This file attempts to implement the extraction of dy_dx from
 * a system of equations. If one considers a black-box where x are
 * the input variables, u are inuut parameters, y are the output
 * variables, f(x,y,u) is the system of equations that will be solved
 * for given x and u, then one can extract the sensitivity information
 * of y wrt x.
 * One crude but simple way of doing this is to finite-difference the
 * given black box, i.e, perturb x, n\subx times by delta x, resolve
 * the system of equations at the new value of x, and compute
 * dy/dx = (f(x\sub1) - f(x\sub2))/(x\sub1 - x\sub2).
 * This is known to be very expensive.
 *
 * The solution that will be adopted here is to formulate the Jacobian J of
 * the system, (or the system is already solved, to grab the jacobian at
 * the solution. Develop the sensitivity matrix df/dx by exact numnerical
 * differentiation; this will be a n x n\subx matrix. Compute the LU factors
 * for J, and to solve column by column to : LU dz/dx = df/dx. Here
 * z, represents all the internal variables to the system and the strictly
 * output variables y. In other words J = df/dz.
 *
 * Given the solution of the above problem we then simply extract the rows
 * of dz/dx, that pertain to the y variables that we are interested in;
 * this will give us dy/dx.
 */

#ifdef THIS_MAY_BE_UNUSED_CODE
static real64 *zero_vector(real64 *vec, int size)
{
  int c;
  for (c=0;c<size;c++) {
    vec[c] = 0.0;
  }
  return vec;
}
#endif

static int NumberIncludedRels(slv_system_t sys)
{
  static int nrels = -1;
  rel_filter_t rfilter;
  int tmp;

  if (!sys) { /* a NULL system may be used to reinitialise the count */
    nrels = -1;
    return -1;
  }
  if (nrels < 0) {
    /*get used (included) relation count -- equalities only !*/
    rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
    rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE);
    tmp = slv_count_solvers_rels(sys,&rfilter);
    nrels = tmp;
    return tmp;
  } else {
    return nrels;
  }
}

static int NumberFreeVars(slv_system_t sys)
{
  static int nvars = -1;
  var_filter_t vfilter;
  int tmp;

  if (!sys) {
    nvars = -1;
    return -1;
  }
  if (nvars < 0) {
    /*get used (free and incident) variable count */

    vfilter.matchbits = (VAR_FIXED |VAR_INCIDENT| VAR_SVAR | VAR_ACTIVE);
    vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
    tmp = slv_count_solvers_vars(sys,&vfilter);
    nvars = tmp;
    return tmp;
  } else {
    return nvars;
  }
}


/*
 *********************************************************************
 * The following bit of code does the computation of the matrix
 * dz/dx. It accepts a slv_system_t and a list of 'input' variables.
 * The matrix df/dx now exists and sits to the right of the main
 * square region of the Jacobian. We will do the following in turn
 * for each variable in this list:
 *
 * 1) 	Get the variable index, and use it to extract the required column
 *    	from the main gradient matrix, this will be stored in a temporary
 *    	vector.
 * 2) 	We will then clear the column in the original matrix, as we want to
 *	store the caluclated results back in place.
 * 3)	Add the vector extracted in 1) as rhs to the main matrix.
 * 4)	Call linsol solve on this rhs to yield a solution which represents
 * 	a column of dx/dx.
 * 6)	Store this vector back in the location cleared out in 2).
 * 7)	Goto 1.
 *
 * At the end of this procedure we would have calculated all the columns of
 * dz/dx, and left them back in the main matrix.
 *********************************************************************
 */


/*
 * At this point we should have an empty jacobian. We now
 * need to call relman_diff over the *entire* matrix.
 * Calculates the entire jacobian. It is initially unscaled.
 *
 * Note: this assumes the sys given is using one of the ascend solvers
 * with either linsol or linsolqr. Both are now allowed. baa 7/95
 */
static int Compute_J(slv_system_t sys)
{
  int32 row;
  var_filter_t vfilter;
  linsol_system_t lin_sys = NULL;
  linsolqr_system_t lqr_sys = NULL;
  mtx_matrix_t mtx;
  struct rel_relation **rlist;
  int nrows;
  int qr=0;
#if DOTIME
  double time1;
#endif

#if DOTIME
  time1 = tm_cpu_time();
#endif
  /*
   * Get the linear system from the solve system.
   * Get the matrix from the linear system.
   * Get the relations list from the solve system.
   */
  lin_sys = slv_get_linsol_sys(sys);
  if (lin_sys==NULL) {
    qr=1;
    lqr_sys=slv_get_linsolqr_sys(sys);
  }
  mtx = slv_get_sys_mtx(sys);
  if (mtx==NULL || (lin_sys==NULL && lqr_sys==NULL)) {
    FPRINTF(stderr,"Compute_dy_dx: error, found NULL.\n");
    return 1;
  }
  rlist = slv_get_solvers_rel_list(sys);
  nrows = NumberIncludedRels(sys);

  calc_ok = TRUE;
  vfilter.matchbits = VAR_SVAR;
  vfilter.matchvalue = VAR_SVAR;
  /*
   * Clear the entire matrix and then compute
   * the gradients at the current point.
   */
  mtx_clear_region(mtx,mtx_ENTIRE_MATRIX);
  for(row=0; row<nrows; row++) {
    struct rel_relation *rel;
    /* added  */
    double resid;

    rel = rlist[mtx_row_to_org(mtx,row)];
    (void)relman_diffs(rel,&vfilter,mtx,&resid,SAFE_FIX_ME);

    /* added  */
    rel_set_residual(rel,resid);

  }
  if (qr) {
    linsolqr_matrix_was_changed(lqr_sys);
  } else {
    linsol_matrix_was_changed(lin_sys);
  }
#if DOTIME
  time1 = tm_cpu_time() - time1;
  FPRINTF(stderr,"Time taken for Compute_J = %g\n",time1);
#endif
  return(!calc_ok);
}


/*
 * Note a rhs would have been previously added
 * to keep the system happy.
 * Now handles both linsol and linsolqr as needed.
 * Assumes region to be factored is in upper left corner.
 * Region is reordered using the last reorder method used in
 * the case of linsolqr, so all linsolqr methods are available
 * to this routine.
 */
static int LUFactorJacobian(slv_system_t sys)
{
  linsol_system_t lin_sys=NULL;
  linsolqr_system_t lqr_sys=NULL;
  mtx_region_t region;
  int size,nvars,nrels;
#if DOTIME
  double time1;
#endif

#if DOTIME
  time1 = tm_cpu_time();
#endif

  nvars = NumberFreeVars(sys);
  nrels = NumberIncludedRels(sys);
  size = MAX(nvars,nrels);		/* get size of matrix */
  mtx_region(&region,0,size-1,0,size-1);	/* set the region */
  lin_sys = slv_get_linsol_sys(sys);	/* get the linear system */
  if (lin_sys!=NULL) {
    linsol_matrix_was_changed(lin_sys);
    linsol_reorder(lin_sys,&region);	/* This was entire_MATRIX */
    linsol_invert(lin_sys,&region); 	/* invert the given matrix over
                                         * the given region */
  } else {
    enum factor_method fm;
    int oldtiming;
    lqr_sys = slv_get_linsolqr_sys(sys);
    /* WE are ASSUMING that the system has been qr_preped before now! */
    linsolqr_matrix_was_changed(lqr_sys);
    linsolqr_reorder(lqr_sys,&region,natural); /* should retain original */
    fm = linsolqr_fmethod(lqr_sys);
    if (fm == unknown_f) {
      fm = ranki_kw2; /* make sure somebody set it */
    }
    oldtiming = g_linsolqr_timing;
    g_linsolqr_timing = 0;
    linsolqr_factor(lqr_sys,fm);
    g_linsolqr_timing = oldtiming;
  }

#if DOTIME
  time1 = tm_cpu_time() - time1;
  FPRINTF(stderr,"Time taken for LUFactorJacobian = %g\n",time1);
#endif
  return 0;
}


/*
 * At this point the rhs would have already been added.
 *
 * Extended to handle either factorization code:
 * linsol or linsolqr.
 */
static int Compute_dy_dx_smart(slv_system_t sys,
                               real64 *rhs,
                               real64 **dy_dx,
                               int *inputs, int ninputs,
                               int *outputs, int noutputs)
{
  linsol_system_t lin_sys=NULL;
  linsolqr_system_t lqr_sys=NULL;
  mtx_matrix_t mtx;
  int col,current_col;
  int row;
  int capacity;
  real64 *solution = NULL;
  int i,j;
#if DOTIME
  double time1;
#endif

#if DOTIME
  time1 = tm_cpu_time();
#endif

  lin_sys = slv_get_linsol_sys(sys); 	/* get the linear system */
  lqr_sys = slv_get_linsolqr_sys(sys); 	/* get the linear system */
  mtx = slv_get_sys_mtx(sys);	 	/* get the matrix */

  capacity = mtx_capacity(mtx);
  solution = (real64 *)asccalloc(capacity,sizeof(real64));

  /*
   * The array inputs is a list of original indexes, of the variables
   * that we are trying to obtain the sensitivity to. We have to
   * get the *current* column from the matrix based on those indices.
   * Hence we use mtx_org_to_col. This little manipulation is not
   * necessary for the computed solution as the solve routine returns
   * the results in the *original* order rather than the *current* order.
   */
  if (lin_sys!=NULL) {
    for (j=0;j<ninputs;j++) {
      col = inputs[j];
      current_col = mtx_org_to_col(mtx,col);
      mtx_org_col_vec(mtx,current_col,rhs,mtx_ALL_ROWS); /* get the column */

      linsol_rhs_was_changed(lin_sys,rhs);
      linsol_solve(lin_sys,rhs);			/* solve */
      linsol_copy_solution(lin_sys,rhs,solution);	/* get the solution */

      for (i=0;i<noutputs;i++) {	/* copy the solution to dy_dx */
        row = outputs[i];
        dy_dx[i][j] = -1.0*solution[row]; /* the -1 comes from the lin alg */
      }
      /*
       * zero the vector using the incidence pattern of our column.
       * This is faster than the naiive approach of zeroing
       * everything especially if the vector is large.
       */
      mtx_zr_org_vec_using_col(mtx,current_col,rhs,mtx_ALL_ROWS);
    }
  } else {
    for (j=0;j<ninputs;j++) {
      col = inputs[j];
      current_col = mtx_org_to_col(mtx,col);
      mtx_org_col_vec(mtx,current_col,rhs,mtx_ALL_ROWS);

      linsolqr_rhs_was_changed(lqr_sys,rhs);
      linsolqr_solve(lqr_sys,rhs);
      linsolqr_copy_solution(lqr_sys,rhs,solution);

      for (i=0;i<noutputs;i++) {
        row = outputs[i];
        dy_dx[i][j] = -1.0*solution[row];
      }
      mtx_zr_org_vec_using_col(mtx,current_col,rhs,mtx_ALL_ROWS);
    }
  }
  if (solution) {
    ascfree((char *)solution);
  }

#if DOTIME
  time1 = tm_cpu_time() - time1;
  FPRINTF(stderr,"Time for Compute_dy_dx_smart = %g\n",time1);
#endif
  return 0;
}


/*
 *********************************************************************
 * This code is provided for the benefit of a temporary
 * fix for the derivative problem in Lsode.
 * The proper permanent fix for lsode is to dump it in favor of
 * cvode or dassl.
 * Extended 7/95 baa to deal with linsolqr and blsode.
 * It is assumed the system has been solved at the current point.
 *********************************************************************
 */
int Asc_BLsodeDerivatives(slv_system_t sys, double **dy_dx,
                       int *inputs_ndx_list, int ninputs,
                       int *outputs_ndx_list, int noutputs)
{
  static int n_calls = 0;
  linsolqr_system_t lqr_sys;	/* stuff for the linear system & matrix */
  mtx_matrix_t mtx;
  int32 capacity;
  real64 *scratch_vector = NULL;
  int result=0;

  (void)NumberFreeVars(NULL);		/* used to re-init the system */
  (void)NumberIncludedRels(NULL);	/* used to re-init the system */
  if (!sys) {
    FPRINTF(stderr,"The solve system does not exist !\n");
    return 1;
  }

  result = Compute_J(sys);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in calc Jacobian\n");
    return 1;
  }

  lqr_sys = slv_get_linsolqr_sys(sys);	/* get the linear system */
  if (lqr_sys==NULL) {
    FPRINTF(stderr,"Early termination due to missing linsolqr system.\n");
    return 1;
  }
  mtx = slv_get_sys_mtx(sys);	/* get the matrix */
  if (mtx==NULL) {
    FPRINTF(stderr,"Early termination due to missing mtx in linsolqr.\n");
    return 1;
  }
  capacity = mtx_capacity(mtx);
  scratch_vector = (real64 *)asccalloc(capacity,sizeof(real64));
  linsolqr_add_rhs(lqr_sys,scratch_vector,FALSE);

  result = LUFactorJacobian(sys);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in LUFactorJacobian\n");
    goto error;
  }
  result = Compute_dy_dx_smart(sys, scratch_vector, dy_dx,
                               inputs_ndx_list, ninputs,
                               outputs_ndx_list, noutputs);

  linsolqr_remove_rhs(lqr_sys,scratch_vector);
  if (result) {
    FPRINTF(stderr,"Early termination due to failure in Compute_dy_dx\n");
    goto error;
  }

 error:
  n_calls++;
  if (scratch_vector) {
    ascfree((char *)scratch_vector);
  }
  return result;
}


int Asc_MtxNormsCmd(ClientData cdata, Tcl_Interp *interp,
                int argc, CONST84 char *argv[])
{
  slv_system_t sys;
  linsol_system_t lin_sys = NULL;
  linsolqr_system_t linqr_sys = NULL;
  mtx_matrix_t mtx;
  mtx_region_t reg;
  double norm;
  int solver;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if (argc!=1) {
    Tcl_SetResult(interp, "wrong # args: Usage __mtx_norms", TCL_STATIC);
    return TCL_ERROR;
  }
  sys = g_solvsys_cur;
  if (sys==NULL) {
    Tcl_SetResult(interp, "__mtx_norms called with slv_system", TCL_STATIC);
    return TCL_ERROR;
  }
  solver = slv_get_selected_solver(sys);
  switch(solver) {
  default:
  case 0:
    lin_sys = slv_get_linsol_sys(sys);
    mtx = linsol_get_matrix(lin_sys);
    reg.col.low = reg.row.low = 0;
    reg.col.high = reg.row.high = mtx_symbolic_rank(mtx);
    norm = linutils_A_1_norm(mtx,&reg);
    FPRINTF(stderr,"A_1_norm = %g\n",norm);
    norm = linutils_A_infinity_norm(mtx,&reg);
    FPRINTF(stderr,"A_infinity_norm = %g\n",norm);
    norm = linutils_A_Frobenius_norm(mtx,&reg);
    FPRINTF(stderr,"A_Frobenius_norm = %g\n",norm);
    norm = linutils_A_cond_kaa(lin_sys,mtx,NULL);
    FPRINTF(stderr,"A_condition # = %g\n",norm);
    break;
  case 3:
  case 5:
    linqr_sys = slv_get_linsolqr_sys(sys);
    mtx = linsolqr_get_matrix(linqr_sys);
    reg.col.low = reg.row.low = 0;
    reg.col.high = reg.row.high = mtx_symbolic_rank(mtx);
    norm = linutils_A_1_norm(mtx,&reg);
    FPRINTF(stderr,"A_1_norm = %g\n",norm);
    norm = linutils_A_infinity_norm(mtx,&reg);
    FPRINTF(stderr,"A_infinity_norm = %g\n",norm);
    norm = linutils_A_Frobenius_norm(mtx,&reg);
    FPRINTF(stderr,"A_Frobenius_norm = %g\n",norm);
    norm = linutils_A_condqr_kaa(linqr_sys,mtx,NULL);
    FPRINTF(stderr,"A_condition # = %g\n",norm);
    break;
  }
  return TCL_OK;
}
