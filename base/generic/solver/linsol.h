/**< 
 *  linsol: Ascend Linear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: linsol.h,v $
 *  Date last modified: $Date: 1997/07/18 12:14:20 $
 *  Last modified by: $Author: mthomas $
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

/*************************************************************************
 ***  Contents:     Linear equation solver module
 ***
 ***  Authors:      Karl Westerberg
 ***                Joseph Zaher
 ***
 ***  Dates:        06/90 - original version
 ***                04/91 - removed output assignment and partitioning
 ***                        (belong in structural analysis)
 ***                08/92 - added transpose ability
 ***                01/94 - broke out linsol_invert() and linsol_solve()
 ***
 ***  Description:  A linear system consists of a coefficient matrix (A)
 ***                and possibly several right-hand-sides (rhs).  The
 ***                solution vector x sought can be that of either
 ***
 ***                                         T
 ***                   A x = rhs      or    A x = rhs
 ***
 ***                depending on a specification inherent with rhs which
 ***                dictates whether or not A should be transposed.  If a
 ***                rhs specifies transpose, then the vector itself is
 ***                expected to be indexed by the original column numbers
 ***                of the coefficient matrix and the solution vector shall
 ***                be indexed by original row numbers.  Otherwise, rhs
 ***                is expected to be indexed by original row numbers while
 ***                the solution can be referenced using original column
 ***                numbers.  The coefficient matrix and each rhs will be
 ***                preserved throughout solving, except that the
 ***                coefficient matrix may be permuted during reordering.
 *************************************************************************/
#ifndef linsol__already_included
#define linsol__already_included


typedef struct linsol_header *linsol_system_t;
/**< 
 ***  linsol_system_t is the linear system handle.
 **/

extern linsol_system_t linsol_create();
/**< 
 ***  sys = linsol_create()
 ***  linsol_system_t sys;
 ***
 ***  Creates a linear system and returns a pointer to it.  Initially the
 ***  system has no coefficient matrix and no rhs.
 **/

extern void linsol_destroy();
/**< 
 ***  linsol_destroy(sys)
 ***  linsol_system_t sys;
 ***
 ***  Destroys the linear system.  The coefficient matrix and each rhs are
 ***  not destroyed by this call.
 **/

extern void linsol_set_matrix();
/**< 
 ***  linsol_set_matrix(sys,mtx)
 ***  linsol_system_t sys;
 ***  mtx_matrix_t mtx;
 ***
 ***  Sets the coefficient matrix to mtx.
 **/

extern mtx_matrix_t linsol_get_matrix();
/**< 
 ***  mtx = linsol_get_matrix(sys)
 ***  mtx_matrix_t mtx;
 ***  linsol_system_t sys;
 ***
 ***  Returns the coefficient matrix.
 **/

extern mtx_matrix_t linsol_get_inverse();
/**< 
 ***  mtx = linsol_get_inverse(sys)
 ***  mtx_matrix_t mtx;
 ***  linsol_system_t sys;
 ***
 ***  Returns the inverse matrix. May be NULL.
 **/

extern void linsol_add_rhs();
/**< 
 ***  linsol_add_rhs(sys,rhs,transpose)
 ***  linsol_system_t sys;
 ***  real64 *rhs;
 ***  boolean transpose;
 ***
 ***  Adds the given rhs to the collection of rhs's already part of the
 ***  system.  rhs should point to an array of numbers indexed by original
 ***  column number if the linear system is to be solved using the transpose
 ***  of the matrix or by original row number if the matrix is not to be
 ***  transposed.  This is determined using the boolean transpose.  The
 ***  rhs should be refered to in the future by its pointer.
 **/

extern void linsol_remove_rhs();
/**< 
 ***  linsol_remove_rhs(sys,rhs)
 ***  linsol_system_t sys;
 ***  real64 *rhs;
 ***
 ***  Removes the given rhs from the system.  The rhs is not destroyed, just
 ***  removed from the system.
 **/

extern int linsol_number_of_rhs();
/**< 
 ***  nrhs = linsol_number_of_rhs(sys)
 ***  int nrhs;
 ***  linsol_system_t sys;
 ***
 ***  Returns the number of rhs's currently part of the system.
 **/

extern real64 *linsol_get_rhs();
/**< 
 ***  rhs = linsol_get_rhs(sys,n)
 ***  real64 *rhs;
 ***  linsol_system_t sys;
 ***  int n;
 ***
 ***  Returns the n-th rhs, where rhs's are indexed in the order they were
 ***  added using linsol_add_rhs() from 0 to (# rhs's)-1.  NULL is returned
 ***  if the index is out of range.
 **/

extern void linsol_matrix_was_changed();
/**< 
 ***  linsol_matrix_was_changed(sys)
 ***  linsol_system_t sys;
 ***
 ***  Informs the solver that a numerical value of a non-zero was changed.
 ***  This must be called whenever any numerical changes to the matrix are
 ***  made.  
 **/

extern void linsol_rhs_was_changed();
/**< 
 ***  linsol_rhs_was_changed(sys,rhs)
 ***  linsol_system_t sys;
 ***  real64 *rhs;  
 ***
 ***  Informs the solver that the given rhs has been modified.  This must be
 ***  called whenever the rhs is modified.
 **/

extern void linsol_set_pivot_zero();
extern real64 linsol_pivot_zero();
/**< 
 ***  linsol_set_pivot_zero(sys,pivot_zero)
 ***  pivot_zero = linsol_pivot_zero(sys)
 ***  linsol_system_t sys;
 ***  real64 pivot_zero;
 ***
 ***  Sets/gets the pivot zero for the system.  Pivots less than or equal to
 ***  this value are regarded as zero.  linsol_set_pivot_zero() will
 ***  automatically call linsol_matrix_was_changed().
 **/

extern void linsol_set_pivot_tolerance();
extern real64 linsol_pivot_tolerance();
/**< 
 ***  linsol_set_pivot_tolerance(sys,ptol)
 ***  ptol = linsol_pivot_tolerance(sys)
 ***  linsol_system_t sys;
 ***  real64 ptol;
 ***
 ***  Sets/gets the pivot tolerance for the system.  Pivots less than this
 ***  fraction of the maximum pivot value in the same row are disregarded.
 ***  linsol_set_pivot_tolerance() will automatically call
 ***  linsol_matrix_was_changed().
 **/

extern void linsol_reorder();
/**< 
 ***  linsol_reorder(sys,region)
 ***  linsol_system_t sys;
 ***  mtx_region_t *region;
 ***
 ***  The specified region of the coefficient matrix is reordered.  This
 ***  should be called before inverting the matrix.  The specified region
 ***  is assumed to contain only nonempty rows and columns.
 **/

extern void linsol_invert();
/**< 
 ***  linsol_invert(sys,region)
 ***  linsol_system_t sys;
 ***  mtx_region_t *region;
 ***
 ***  Decompose the specified region of a copy of the coefficient matrix
 ***  into upper and lower triangular factors (if necessary) which can be
 ***  inverted easily when applied to any rhs.  Matrix must be inverted in
 ***  order to perform any of the operations below.  The numerical rank and
 ***  the smallest pivot encountered during pivoting are computed in the
 ***  process.
 **/

extern int32 linsol_rank();
/**< 
 ***  rank = linsol_rank(sys)
 ***  int32 rank;
 ***  linsol_system_t sys;
 *** 
 ***  Returns the rank of the system.  The system must be previously
 ***  inverted.
 **/

extern real64 linsol_smallest_pivot();
/**< 
 ***  smallest_pivot = linsol_smallest_pivot(sys)
 ***  real64 smallest_pivot;
 ***  linsol_system_t sys;
 ***
 ***  Returns the smallest pivot accepted in solving the system.  The
 ***  system must be previously inverted.  If no pivoting was performed,
 ***  MAXDOUBLE is returned.
 **/

extern int linsol_get_pivot_sets();
/**< 
 ***  status=linsol_get_pivot_sets(sys,org_rowpivots,org_colpivots)
 ***  linsol_system_t sys;
 ***  unsigned *org_rowpivots,*org_colpivots;  (see the "set" module)
 ***
 ***  Returns the set of original row numbers / original column numbers which
 ***  have been pivoted.  org_rowpivots and org_colpivots are assumed to be
 ***  sets created by (or at least for) the set module with sufficient size
 ***  before calling this function.  They must also be previously NULLed.
 ***  The system must be previously inverted.
 ***  The sets input should be the result of set_create(neqn),set_create(nvar).
 ***  There is no association of rows with columns here.
 ***
 ***  Status is 0 if not inverted, 1 if inverted. if 0, sets will not be
 ***  changed.
 **/

extern int32 linsol_org_row_to_org_col();
extern int32 linsol_org_col_to_org_row();
/**< 
 ***  org_col = linsol_org_row_to_org_col(sys,org_row)
 ***  org_row = linsol_org_col_to_org_row(sys,org_col)
 ***  linsol_system_t sys;
 ***  int32 org_col,org_row;
 ***
 ***  Pivoted original columns and pivoted original rows can be associated
 ***  with one another via the pivot sequence.  These functions returned the
 ***  org_col/org_row associated with the given org_row/org_col.  If the given
 ***  org_row/org_col is not pivoted, a meaningless value is returned.  The
 ***  system must be previously inverted. If not inverted, these functions
 ***  will return a value, but linsol may reorder making the value wrong.
 **/

extern real64 linsol_org_row_dependency();
extern real64 linsol_org_col_dependency();
/**< 
 ***  coef = linsol_org_row_dependency(sys,dep,ind)
 ***  coef = linsol_org_col_dependency(sys,dep,ind)
 ***  real64 coef;
 ***  linsol_system_t sys;
 ***  int32 dep,ind;
 ***
 ***  Any original row / column of the coefficient matrix which when submitted
 ***  to the linear solver is not pivoted, is called dependent and can be
 ***  written as a linear combination of the independent (pivoted) original
 ***  rows / columns.  These functions return the previously computed
 ***  coefficients of the linear combination.  The system must be previously
 ***  inverted and the independent row / column must be a member of the
 ***  set of row / column pivots obtained by linsol_get_pivot_sets.
 **/

extern void linsol_solve();
/**< 
 ***  linsol_solve(sys,rhs)
 ***  linsol_system_t sys;
 ***  real64 *rhs; 
 ***
 ***  Solves the system of linear equations (if necessary) utilizing the
 ***  specified rhs along with the previously inverted matrix.  The rhs
 ***  is automatically checked if the matrix factors need to be transposed
 ***  or not.
 **/

extern real64 linsol_var_value();
/**< 
 ***  value = linsol_var_value(sys,rhs,var)
 ***  real64 value;
 ***  linsol_system_t sys;
 ***  real64 *rhs;
 ***  int32 var;
 *** 
 ***  Returns the value of the variable in the solution vector associated
 ***  with the given rhs and either the matrix or its transpose.  The rhs
 ***  must be solved for first.  If rhs specifies transpose, then var is
 ***  expected to be an original row number, otherwise it should be an
 ***  original column number.
 **/

extern boolean linsol_copy_solution();
/**< 
 ***  result = linsol_copy_solution(sys,rhs,vector)
 ***  linsol_system_t sys;
 ***  real64 *rhs;
 ***  real64 *vector;
 *** 
 ***  Will copy the solution vector into the vector provided.
 ***  The user is responsible for allocating and deallocating said
 ***  vector. The provided vector should be at least
 ***  mtx_cacpacity(mtx), where mtx is the matrix that linsol_set_matrix,
 ***  was called with. The returned vector will be indexed in the same
 ***  way that the rhs is indexed.
 ***
 ***  Returns TRUE in the event of any of any errors, FALSE otherwise.
 **/

extern real64 linsol_eqn_residual();
/**< 
 ***  residual = linsol_eqn_residual(sys,rhs,eqn)
 ***  real64 residual;
 ***  linsol_system_t sys;
 ***  real64 *rhs;
 ***  int32 eqn;
 ***
 ***  Returns the equation residual using the solution vector associated
 ***  with the given rhs and either the matrix or its transpose.
 ***
 ***                                           T
 ***     residual = A x - b   or   residual = A x - b
 ***
 ***  The rhs must be solved for first.  If rhs specifies transpose, then
 ***  eqn is expected to be an original column number, otherwise it should
 ***  be an original row number.
 **/

#endif
