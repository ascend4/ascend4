/*
 *  lin_utils: Ascend Linear Algebra Utilities
 *  by Kirk Andre' Abbott
 *  Created: 12 March 1995
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: linutils.c,v $
 *  Date last modified: $Date: 2000/01/25 02:27:01 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1995 Kirk Andre' Abbott
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

#include "linutils.h"

#include <stdio.h>
#include <math.h>

#include <utilities/ascConfig.h>
#include <general/mathmacros.h>
#include <utilities/ascMalloc.h>
#include <general/tm_time.h>

#include "mtx.h"
#include "linsolqr.h"


double linutils_A_1_norm(mtx_matrix_t mtx, mtx_region_t *reg){
  mtx_region_t mtxreg;
  double value, maxvalue = 0;
  int col;

  if (mtx==NULL)
    return 1.0e20;

  if (reg==NULL) {
    mtxreg.row.low = mtxreg.col.low = 0;
    mtxreg.row.high = mtxreg.col.high = mtx_order(mtx);
  }
  else{
    mtxreg = *reg;
  }
  for (col = mtxreg.col.low; col < mtxreg.col.high; col++) {
    value = mtx_sum_sqrs_in_col(mtx, col, &mtxreg.row);
    maxvalue = MAX(maxvalue,value);
  }
  return maxvalue;
}


double linutils_A_infinity_norm(mtx_matrix_t mtx, mtx_region_t *reg)

{
  mtx_region_t mtxreg;
  double value, maxvalue = 0;
  int row;

  if (mtx==NULL)
    return 1.0e20;

  if (reg==NULL) {
    mtxreg.row.low = mtxreg.col.low = 0;
    mtxreg.row.high = mtxreg.col.high = mtx_order(mtx);
  }
  else{
    mtxreg = *reg;
  }
  for (row = mtxreg.row.low; row < mtxreg.row.high; row++) {
    value = mtx_sum_sqrs_in_row(mtx, row, &mtxreg.col);
    maxvalue = MAX(maxvalue,value);
  }
  return maxvalue;
}


double linutils_A_Frobenius_norm(mtx_matrix_t mtx, mtx_region_t *reg){
  mtx_region_t mtxreg;
  double value;
  /* double maxvalue = 0; */
  int row;

  if (mtx==NULL)
    return 1.0e20;

  if (reg==NULL) {
    mtxreg.row.low = mtxreg.col.low = 0;
    mtxreg.row.high = mtxreg.col.high = mtx_order(mtx);
  }
  else{
    mtxreg = *reg;
  }

  value = 0.0;
  for (row = mtxreg.row.low; row < mtxreg.row.high; row++) {
    value += mtx_sum_sqrs_in_row(mtx, row, &mtxreg.col);
  }
  return sqrt(value);
}

/*
 * This function has this funny name, because I am using
 * a very inelegant method to do this computation, but I
 * am using the linsolqr system primitives. Ben Allan has
 * much more efficient ways of doing these computations,
 * in fact, he gets them for free at the end of his QR
 * analyses. It assumes that the linear system provided
 * has *already* been factored. We then compute the inverse
 * explicitly, compute the Frobenius norms for both the
 * inverse and the original matrix and return:
 *               -1
 * || A || . || A ||
 *       F         F
 * Warning !! this function is neither fast nor memory
 * efficient. !!
 */
double linutils_A_condqr_kaa(linsolqr_system_t lin_sys,
			     mtx_matrix_t mtx,
			     mtx_region_t *reg)
{
  mtx_matrix_t factors;
  mtx_region_t mtxreg;
  double *solution = NULL, *rhs = NULL;
  double a_norm, a_inverse_norm = 0.0;
  int rank,capacity;
  int j,k;
  double comptime;

  if (mtx==NULL)
    return 1.0e20;

  if (reg==NULL) {
    mtxreg.row.low = mtxreg.col.low = 0;
    mtxreg.row.high = mtxreg.col.high = mtx_symbolic_rank(mtx);
  }
  else{
    mtxreg = *reg;
  }

  /*
   * Let us do the expensive part first, which is to calculate,
   * A inverse column by column. While are doing this we will
   * compute and accumulated the 2 norm for each column.
   */
  factors = linsolqr_get_matrix(lin_sys);
  capacity = mtx_capacity(factors);
  solution = ASC_NEW_ARRAY_CLEAR(double,capacity);
  rhs = ASC_NEW_ARRAY_CLEAR(double,capacity);
  linsolqr_add_rhs(lin_sys,rhs,FALSE);

  rank = mtx_symbolic_rank(factors);
  comptime = tm_cpu_time();
  for (j=0;j<rank;j++) {
    rhs[j] = 1.0;
    linsolqr_rhs_was_changed(lin_sys,rhs);
    linsolqr_solve(lin_sys,rhs);
    linsolqr_copy_solution(lin_sys,rhs,solution);

    for (k=0;k<capacity;k++) {/* dense vector 2 norm */
      a_inverse_norm += solution[k]*solution[k];
      solution[k] = 0.0;
    }
    rhs[j] = 0.0;
  }
  comptime = tm_cpu_time() - comptime;
  FPRINTF(stderr,"Time to compute explicit inverse by linsolqr = %g\n",comptime);
  a_inverse_norm = sqrt(a_inverse_norm);
  linsolqr_remove_rhs(lin_sys,rhs);

  /*
   * We should now have a_inverse_norm
   * All we need to do now is to calculate a_norm and
   * return the result: a_norm * a_inverse_norm.
   */

  a_norm = linutils_A_Frobenius_norm(mtx, &mtxreg);
  FPRINTF(stderr,"A norm = %g\tA_inverse norm = %g; condition # = %g\n",
	  a_norm,a_inverse_norm,a_norm*a_inverse_norm);

  if (solution) ascfree(solution);
  if (rhs) ascfree(rhs);

  return (a_norm*a_inverse_norm);
}

