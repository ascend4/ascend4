/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

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
	ASCEND linear solver
	
	A linear system consists of a coefficient matrix (A)
    and possibly several right-hand-sides (rhs).  The
    solution vector x sought can be that of either
	<pre>
	                                       T
	                 A x = rhs      or    A x = rhs    </pre>

	depending on a specification inherent with rhs which
	dictates whether or not A should be transposed.  If a
	rhs specifies transpose, then the vector itself is
	expected to be indexed by the original column numbers
	of the coefficient matrix and the solution vector shall
	be indexed by original row numbers.  Otherwise, rhs
	is expected to be indexed by original row numbers while
	the solution can be referenced using original column
	numbers.  The coefficient matrix and each rhs will be
	preserved throughout solving, except that the
	coefficient matrix may be permuted during reordering.
*//*
	by Karl Michael Westerberg, created 2/6/90
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/18 12:14:20 $ $Author: mthomas $
	Authors:      Karl Westerberg
	              Joseph Zaher
	Dates:
		06/90 - original version
		04/91 - removed output assignment and partitioning
			    (belong in structural analysis)
		08/92 - added transpose ability
		01/94 - broke out linsol_invert() and linsol_solve()
*/

#ifndef ASC_LINSOL_H
#define ASC_LINSOL_H

#include <utilities/ascConfig.h>
#include "mtx.h"

/**	@addtogroup linear Linear
	@{
*/

typedef struct linsol_header *linsol_system_t;
/**<  linsol_system_t is the linear system handle. */

extern linsol_system_t linsol_create(void);
/**<
 *  Creates a linear system and returns a pointer to it.  Initially the
 *  system has no coefficient matrix and no rhs.
 */

extern void linsol_destroy(linsol_system_t sys);
/**<
 *  Destroys the linear system.  The coefficient matrix and each rhs are
 *  not destroyed by this call.
 */

extern void linsol_set_matrix(linsol_system_t sys, mtx_matrix_t mtx);
/**<
 *  Sets the coefficient matrix to mtx.
 */

ASC_DLLSPEC mtx_matrix_t linsol_get_matrix(linsol_system_t sys);
/**<
 *  Returns the coefficient matrix.
 */

ASC_DLLSPEC mtx_matrix_t linsol_get_inverse(linsol_system_t sys);
/**<
 *  Returns the inverse matrix. May be NULL.
 */

extern void linsol_add_rhs(linsol_system_t sys,
                           real64 *rhs,
                           boolean transpose);
/**<
 *  Adds the given rhs to the collection of rhs's already part of the
 *  system.  rhs should point to an array of numbers indexed by original
 *  column number if the linear system is to be solved using the transpose
 *  of the matrix or by original row number if the matrix is not to be
 *  transposed.  This is determined using the boolean transpose.  The
 *  rhs should be refered to in the future by its pointer.
 */

extern void linsol_remove_rhs(linsol_system_t sys, real64 *rhs);
/**<
 *  Removes the given rhs from the system.  The rhs is not destroyed, just
 *  removed from the system.
 */

extern int linsol_number_of_rhs(linsol_system_t sys);
/**<
 *  Returns the number of rhs's currently part of the system.
 */

ASC_DLLSPEC real64 *linsol_get_rhs(linsol_system_t sys, int n);
/**< 
 *  Returns the n-th rhs, where rhs's are indexed in the order they were
 *  added using linsol_add_rhs() from 0 to (# rhs's)-1.  NULL is returned
 *  if the index is out of range.
 */

ASC_DLLSPEC void linsol_matrix_was_changed(linsol_system_t sys);
/**< 
 *  Informs the solver that a numerical value of a non-zero was changed.
 *  This must be called whenever any numerical changes to the matrix are
 *  made.
 */

ASC_DLLSPEC void linsol_rhs_was_changed(linsol_system_t sys, real64 *rhs);
/**< 
 *  Informs the solver that the given rhs has been modified.  This must be
 *  called whenever the rhs is modified.
 */

extern void linsol_set_pivot_zero(linsol_system_t sys, real64 pivot_zero);
/**<
 *  Sets the pivot zero for the system.  Pivots less than or equal to
 *  this value are regarded as zero.  linsol_set_pivot_zero() will
 *  automatically call linsol_matrix_was_changed().
 */
extern real64 linsol_pivot_zero(linsol_system_t sys);
/**<
	Gets the pivot zero for the system.  Pivots less than or equal to
	this value are regarded as zero.  

	Calls linsol_matrix_was_changed().
*/

extern void linsol_set_pivot_tolerance(linsol_system_t sys, real64 ptol);
/**<
 *  Sets the pivot tolerance for the system.  Pivots less than this
 *  fraction of the maximum pivot value in the same row are disregarded.
 *  linsol_set_pivot_tolerance() will automatically call
 *  linsol_matrix_was_changed().
 */
extern real64 linsol_pivot_tolerance(linsol_system_t sys);
/**<
	Gets the pivot tolerance for the system.  Pivots less than this
	fraction of the maximum pivot value in the same row are disregarded.
	
	Calls linsol_matrix_was_changed().
*/

ASC_DLLSPEC void linsol_reorder(linsol_system_t sys, mtx_region_t *region);
/**<
 *  The specified region of the coefficient matrix is reordered.  This
 *  should be called before inverting the matrix.  The specified region
 *  is assumed to contain only nonempty rows and columns.
 */

ASC_DLLSPEC void linsol_invert(linsol_system_t sys, mtx_region_t *region);
/**<
 *  Decompose the specified region of a copy of the coefficient matrix
 *  into upper and lower triangular factors (if necessary) which can be
 *  inverted easily when applied to any rhs.  Matrix must be inverted in
 *  order to perform any of the operations below.  The numerical rank and
 *  the smallest pivot encountered during pivoting are computed in the
 *  process.
 */

extern int32 linsol_rank(linsol_system_t sys);
/**<
 *  Returns the rank of the system.  The system must be previously
 *  inverted.
 */

extern real64 linsol_smallest_pivot(linsol_system_t sys);
/**<
 *  Returns the smallest pivot accepted in solving the system.  The
 *  system must be previously inverted.  If no pivoting was performed,
 *  MAXDOUBLE is returned.
 */

extern int linsol_get_pivot_sets(linsol_system_t sys,
                                 unsigned *org_rowpivots,
                                 unsigned *org_colpivots);
/**<
 *  Returns the set of original row numbers / original column numbers which
 *  have been pivoted.  org_rowpivots and org_colpivots are assumed to be
 *  sets created by (or at least for) the set module with sufficient size
 *  before calling this function.  They must also be previously NULLed.
 *  The system must be previously inverted.
 *  The sets input should be the result of set_create(neqn),set_create(nvar).
 *  There is no association of rows with columns here.<br><br>
 *
 *  Status is 0 if not inverted, 1 if inverted. if 0, sets will not be
 *  changed.
 */

extern int32 linsol_org_row_to_org_col(linsol_system_t sys,
                                       int32 org_row);
/**<
 *  Returns the org_col associated with the given org_row/org_col.
 *  Pivoted original columns and pivoted original rows can be associated
 *  with one another via the pivot sequence.    If the given
 *  org_row is not pivoted, a meaningless value is returned.  The
 *  system must be previously inverted. If not inverted, a value will be
 *  returned, but linsol may reorder making the value wrong.
 */
extern int32 linsol_org_col_to_org_row(linsol_system_t sys,
                                       int32 org_col);
/**<
 *  Returns the org_row associated with the given org_col.
 *  Pivoted original columns and pivoted original rows can be associated
 *  with one another via the pivot sequence.    If the given
 *  org_col is not pivoted, a meaningless value is returned.  The
 *  system must be previously inverted. If not inverted, a value is returned,these functions
 *  but linsol may reorder making the value wrong.
 */

extern real64 linsol_org_row_dependency(linsol_system_t sys,
                                        int32 dep, int32 ind);
/**< See linsol_org_col_dependency(). */
extern real64 linsol_org_col_dependency(linsol_system_t sys,
                                        int32 dep, int32 ind);
/**<
 *  Any original row / column of the coefficient matrix which when submitted
 *  to the linear solver is not pivoted, is called dependent and can be
 *  written as a linear combination of the independent (pivoted) original
 *  rows / columns.  These functions return the previously computed
 *  coefficients of the linear combination.  The system must be previously
 *  inverted and the independent row / column must be a member of the
 *  set of row / column pivots obtained by linsol_get_pivot_sets.
 */

ASC_DLLSPEC void linsol_solve(linsol_system_t sys, real64 *rhs);
/**<
 *  Solves the system of linear equations (if necessary) utilizing the
 *  specified rhs along with the previously inverted matrix.  The rhs
 *  is automatically checked if the matrix factors need to be transposed
 *  or not.
 */

extern real64 linsol_var_value(linsol_system_t sys, 
                               real64 *rhs, int32 var);
/**<
 *  Returns the value of the variable in the solution vector associated
 *  with the given rhs and either the matrix or its transpose.  The rhs
 *  must be solved for first.  If rhs specifies transpose, then var is
 *  expected to be an original row number, otherwise it should be an
 *  original column number.
 */

ASC_DLLSPEC boolean linsol_copy_solution(linsol_system_t sys,
                                    real64 *rhs, real64 *vector);
/**<
 *  Will copy the solution vector into the vector provided.
 *  The user is responsible for allocating and deallocating said
 *  vector. The provided vector should be at least
 *  mtx_cacpacity(mtx), where mtx is the matrix that linsol_set_matrix,
 *  was called with. The returned vector will be indexed in the same
 *  way that the rhs is indexed.<br><br>
 *
 *  Returns TRUE in the event of any of any errors, FALSE otherwise.
 */

extern real64 linsol_eqn_residual(linsol_system_t sys,
                                  real64 *rhs, int32 eqn);
/**<
 *  Returns the equation residual using the solution vector associated
 *  with the given rhs and either the matrix or its transpose.
 *  <pre>
 *                                           T
 *     residual = A x - b   or   residual = A x - b
 *  </pre>
 *  The rhs must be solved for first.  If rhs specifies transpose, then
 *  eqn is expected to be an original column number, otherwise it should
 *  be an original row number.
 */

/** @} */
 
#endif  /* ASC_LINSOL_H */

