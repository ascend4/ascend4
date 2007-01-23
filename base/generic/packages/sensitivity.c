/*	ASCEND modelling environment
	Copyright (C) 1990-2006 Carnegie-Mellon University

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
*//** @file
	Sensititvity analysis code

	@see documentation in sensitivity.h.
*//*
	by Kirk Abbott
*/

#include "sensitivity.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utilities/error.h>
#include <general/mathmacros.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>

#include <compiler/packages.h>


#include <compiler/atomvalue.h>
#include <compiler/instquery.h>

#include <solver/system.h>

#define DEBUG 1

#if 0
static real64 *zero_vector(real64 *vec, int size)
{
  int c;
  for (c=0;c<size;c++) {
    vec[c] = 0.0;
  }
  return vec;
}
#endif

/*----------------------------------------------------
  UTILITY ROUTINES
*/

/**
	Fetch an element from a branch of an arglist...?
*/
struct Instance *FetchElement(struct gl_list_t *arglist,
				     unsigned long whichbranch,
				     unsigned long whichelement){
  struct gl_list_t *branch;
  struct Instance *element;

  if (!arglist) return NULL;
  branch = (struct gl_list_t *)gl_fetch(arglist,whichbranch);
  element = (struct Instance *)gl_fetch(branch,whichelement);
  return element;
}

/**
	Looks like it returns the number of relations in a solver's system

	@param sys The system in question
	@return int Number of relations in the system
	@see slv_count_solvers_rels
*/
int NumberRels(slv_system_t sys){
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
  }
  else return nrels;
}

int NumberIncludedRels(slv_system_t sys){
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

/**
	Looks like it returns the number of free variables in a solver's system

	@param sys The system in question
	@return the number of free variables

	@see slv_count_solvers_vars
*/
int NumberFreeVars(slv_system_t sys){
  static int nvars = -1;
  var_filter_t vfilter;
  int tmp;

  if (!sys) {
    /* no system: return error */
    nvars = -1;
    return -1;
  }

  if (nvars >=0){
    /* return stored value */
    return nvars;
  }

  /* get used (free and incident) variable count */
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT  | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  tmp = slv_count_solvers_vars(sys,&vfilter);
  nvars = tmp;
  return tmp;
}

/*
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
 */


/*
 * At this point we should have an empty jacobian. We now
 * need to call relman_diff over the *entire* matrix.
 * fixed and free.
 *
 *  Calculates the entire jacobian.
 *  It is initially unscaled.
 */
int Compute_J(slv_system_t sys)
{
  int32 row;
  var_filter_t vfilter;
  linsolqr_system_t lqr_sys;
  mtx_matrix_t mtx;
  struct rel_relation **rlist;
  int nrows;
  real64 resid;

  /*
   * Get the linear system from the solve system.
   * Get the matrix from the linear system.
   * Get the relations list from the solve system.
   */
  lqr_sys = slv_get_linsolqr_sys(sys);
  mtx = linsolqr_get_matrix(lqr_sys);
  rlist = slv_get_solvers_rel_list(sys);
  nrows = slv_get_num_solvers_rels(sys);

  calc_ok = TRUE;
  vfilter.matchbits =  (VAR_SVAR | VAR_ACTIVE) ;
  vfilter.matchvalue =  vfilter.matchbits ;

  /*
   * Clear the entire matrix and then compute
   * the gradients at the current point.
   */
  mtx_clear_region(mtx,mtx_ENTIRE_MATRIX);
  for(row=0; row<nrows; row++) {
    struct rel_relation *rel;
    rel = rlist[mtx_row_to_org(mtx,row)];
	asc_assert(rel!=NULL);
    (void)relman_diffs(rel,&vfilter,mtx,&resid,1);
  }
  linsolqr_matrix_was_changed(lqr_sys);

  return(!calc_ok);
}

/**
	@NOTE there is another (probably better?) version of this in models/sensitivity
	that uses sparse matrices. This one got pulled out of some files in tcltk dir
	and is used by the LSODE integrator.

 * Note a rhs would have been previously added
 * to keep the system happy.
 * Now handles both linsol and linsolqr as needed.
 * Assumes region to be factored is in upper left corner.
 * Region is reordered using the last reorder method used in
 * the case of linsolqr, so all linsolqr methods are available
 * to this routine.
*/
int LUFactorJacobian(slv_system_t sys){
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
  CONSOLE_DEBUG("Time taken for LUFactorJacobian = %g\n",time1);
#endif
  return 0;
}


/**
	At this point the rhs would have already been added.

	Extended to handle either factorization code:
	linsol or linsolqr.

	This routine is part of the 'temporary solution' for derivatives in lsode.c
*/
int Compute_dy_dx_smart(slv_system_t sys,
                               real64 *rhs,
                               DenseMatrix dy_dx,
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
  solution = ASC_NEW_ARRAY_CLEAR(real64,capacity);

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
        DENSEMATRIX_ELEM(dy_dx,i,j) = -1.0*solution[row]; /* the -1 comes from the lin alg */
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
        DENSEMATRIX_ELEM(dy_dx,i,j) = -1.0*solution[row];
      }
      mtx_zr_org_vec_using_col(mtx,current_col,rhs,mtx_ALL_ROWS);
    }
  }
  if (solution) {
    ascfree((char *)solution);
  }

#if DOTIME
  time1 = tm_cpu_time() - time1;
  CONSOLE_DEBUG("Time for Compute_dy_dx_smart = %g\n",time1);
#endif
  return 0;
}

#undef DEBUG
