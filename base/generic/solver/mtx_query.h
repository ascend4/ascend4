/*	ASCEND modelling environment
	Copyright (C) 1996 Benjamin Andrew Allan
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
	mtx2: Ascend Sparse Matrix Package.
	Requires:
	#include "utilities/ascConfig.h"
	#include "mtx.h"
*//*
	mtx by Karl Michael Westerberg, created 5/3/90
	mtx2 by Benjamin Andrew Allan
	Last in CVS: $Revision: 1.9 $ $Date: 1997/07/18 12:15:12 $ $Author: mthomas $
*/

#ifndef __MTX_QUERY_H_SEEN__
#define __MTX_QUERY_H_SEEN__

/**	@addtogroup linear Linear
	@{
*/

ASC_DLLSPEC real64 mtx_next_in_row(mtx_matrix_t matrix, 
                              mtx_coord_t *coord,
                              mtx_range_t *colrng);
/**< See mtx_next_in_col(), switching row & column references. */
extern real64 mtx_next_in_col(mtx_matrix_t matrix,
                              mtx_coord_t *coord,
                              mtx_range_t *rowrng);
/**<
 ***  Searches for the next non-zero in the given row, whose column index
 ***  lies in the given range, and returns its value.  Coord->col is set to
 ***  the column in which it was found.  To find the first non-zero in the
 ***  row, set coord->col = mtx_FIRST.  Coord->col will equal mtx_LAST and
 ***  0.0 will be returned if there is no next non-zero. The row (and mtx!)
 ***  searched is always the one specified when mtx_next_in_row was most
 ***  recently called with a coord such that coord->col == mtx_FIRST.
 ***  If colrng.low > colrng.high, return is mtx_LAST, which gives correct
 ***  behaviour for mtx/vector processing loops.
 ***  If nz.row is not in the matrix (<0 || >= mtx order) on an initial
 ***  call (with nz.col == mtx_FIRST) returns 0.0 and nz.col == mtx_LAST.
 ***
 ***  Example of canonical usage:                                       <pre>
 ***     nz.col = col;                   (* the col to be traversed *)
 ***     nz.row = mtx_FIRST;             (* initializer, which must be used *)
 ***     while( value = mtx_next_in_col(mtx,&nz,rowrng)),
 ***            nz.row != mtx_LAST ) {   (* your thing here *) }        </pre>
 ***  Note that the logic test of the while is ONLY nz.row != mtx_LAST.
 ***  C lets you do whatever you choose before the comma.<br><br>
 ***
 -$-  Returns FALSE from bad a matrix.<br><br>
 ***
 ***  NOTE: The notion of "next non-zero" is arbitrary, but will not change
 ***  until the next destructive operation on the matrix is performed.
 ***  Neither the row/col number nor the rel/var index will
 ***  behave monotonically while traversing a col/row. This means there
 ***  is nothing you can do with the range to shrink the search space
 ***  based on the last row/col returned. Basic matrix operations, when
 ***  properly thought about in a general sparse context, do not care
 ***  about the order in which the elements of vectors are processed.
 ***  <pre>
 ***  WARNINGS:
 !1!  - You may NOT nest mtx_next_in_col within mtx_next_in_col
 ! !    or mtx_next_in_row within mtx_next_in_row. We have yet to find a
 ! !    process suggestive of doing so that was not better handled with
 ! !    a for loop. This is a global constraint. Violation results in garbage.
 !2!  - You may NOT safely perform operation mtx_del_zr_in_col while traversing
 ! !    a column unless the value just returned from the previous next_in_col is
 ! !    nonzero or the coord.row last returned is mtx_LAST. Also, you may NOT
 ! !    safely perform operation mtx_del_zr_in_row on coord.row unless the
 ! !    value just returned from the previous next_in_col is nonzero.
 ! !    The proscription here is for mtx_next_in_ and mtx_del_zr_in_ operating
 ! !    on the same matrix. mtx_del_zr_in_ is safe to call if the mtx_next_in_
 ! !    driving it is happening on a different matrix.
 !3!  Transpose warning 2 for next_in_row.
 ! !  Violation of 2 or 3 results, sooner or later, in a memory fault.
 ***  You CAN nest mtx_next_in_col within mtx_next_in_row or vice versa,
 ***  however.
 ***  </pre>
 **/

extern real64 mtx_row_max(mtx_matrix_t matrix,
                          mtx_coord_t *coord,
                          mtx_range_t *colrng,
                          real64 *signval);
/**< See mtx_col_max(), switching row & column references. */
extern real64 mtx_col_max(mtx_matrix_t matrix, 
                          mtx_coord_t *coord,
                          mtx_range_t *rowrng, 
                          real64 *signval);
/**<
 ***  Searches for the element in the given row, with column index in the
 ***  given column range, which has the largest absolute value.  The user
 ***  should set coord->row to the desired row to search, and this function
 ***  will set coord->col to the column index where the maximum was found
 ***  (or mtx_NONE if no non-zero was found in that range).  The absolute
 ***  value of that element is also returned. If the pointer signval is not
 ***  NULL, the real64 pointed to will be stuffed with the signed
 ***  value of the maximum sized element.
 ***  In the event of ties, the element with the lowest current column
 ***  index wins.
 ***
 -$-  Returns -1.0 from a bad matrix.
 **/
extern real64 mtx_row_min(mtx_matrix_t matrix,
                          mtx_coord_t *coord,
                          mtx_range_t *colrng,
                          real64 *signval,
                          real64 minval);
/**< See mtx_col_min(), switching row & column references. */
extern real64 mtx_col_min(mtx_matrix_t matrix,
                          mtx_coord_t *coord,
                          mtx_range_t *rowrng,
                          real64 *signval,
                          real64 minval);
/**<
 ***  Searches for the element in the given row, with column index in the
 ***  given column range, which has the smallest absolute value.  The user
 ***  should set coord->row to the desired row to search, and this function
 ***  will set coord->col to the column index where the minimum was found
 ***  (or mtx_NONE if no non-zero was found in that range).  The absolute
 ***  value of that element is also returned. If the pointer signval is not
 ***  NULL, the real64 pointed to will be stuffed with the signed
 ***  value of the minimum sized element.
 ***  In the event of ties, the element with the lowest current column
 ***  index wins.<br><br>
 ***  This function only looks at the nonzero elements, and will only
 ***  find numbers between minval  and 1e50.  If no number is found signval
 ***  will be zero and
 ***  value will be one.<br><br>
 ***
 -$-  Returns -1.0 from a bad matrix.
 **/

extern real64 mtx_get_pivot_col(mtx_matrix_t matrix, 
                                mtx_coord_t *coord,
                                mtx_range_t *colrng, 
                                real64 *signval,
                                real64 tol, 
                                real64 eps);
/**< See mtx_get_pivot_row(), switching row & column references. */
extern real64 mtx_get_pivot_row(mtx_matrix_t matrix,
                                mtx_coord_t *coord,
                                mtx_range_t *rowrng,
                                real64 *signval,
                                real64 tol,
                                real64 eps);
/**<
 ***  This implements efficiently the standard sparse modification
 ***  of LU partial pivot selection.
 ***
 ***  Searches for the leftmost element in the colrng of the given row,
 ***  which passes the sparse partial pivoting criteria:
 ***    -# aij >= eps,
 ***    -# aij >= tol * max_abs_element_value_in_colrng.
 ***  The absolute value of the passing element is returned, or 0.0 if
 ***  there are no entries that pass criterion 1.  The user
 ***  should set coord->row to the desired row to search. This function
 ***  will set coord->col to the column index where the result was found
 ***  (or mtx_NONE if nothing good was found in that range).  The absolute
 ***  value of that element is also returned. If the pointer signval is not
 ***  NULL, the real64 pointed to will be stuffed with the signed
 ***  value of the selected element.
 ***  This function is faster when colrng == mtx_ALL_COLS can be used.<br><br>
 ***
 -$-  Returns -1.0 from a bad matrix.
 **/

extern int32 mtx_nonzeros_in_row(mtx_matrix_t matrix,
                                 int32 row,
                                 mtx_range_t *colrng);
/**<
 ***  Counts the number of incidences in the given row whose column index
 ***  lies in the given column range.
 -$-  Returns -1 from a bad matrix.
 **/
extern int32 mtx_nonzeros_in_col(mtx_matrix_t matrix,
                                 int32 col,
                                 mtx_range_t *rowrng);
/**<
 ***  Counts the number of incidences in the given column whose row index
 ***  lies in the given row range.
 -$-  Returns -1 from a bad matrix.
 **/
extern int32 mtx_nonzeros_in_region(mtx_matrix_t matrix,
                                    mtx_region_t *reg);
/**<
 ***  Counts the non-zero values in the given region.
 -$-  Returns -1 from a bad matrix.
 **/
extern int32 mtx_numbers_in_row(mtx_matrix_t matrix,
                                int32 row,
                                mtx_range_t *colrng);
/**<
 ***  Counts the non-zero values in the given row whose column index
 ***  lies in the given column range.
 -$-  Returns -1 from a bad matrix.
 **/
extern int32 mtx_numbers_in_col(mtx_matrix_t matrix,
                                int32 col,
                                mtx_range_t *rowrng);
/**<
 ***  Counts the non-zero values in the given column whose row index
 ***  lies in the given row range.
 -$-  Returns -1 from a bad matrix.
 **/
extern int32 mtx_numbers_in_region(mtx_matrix_t matrix,
                                   mtx_region_t *reg);
/**<
 ***  Counts the number of incidences in the given region.
 -$-  Returns -1 from a bad matrix.
 **/

/* ********************************************************************* *\
  mtx vector operation routines
  None of these routines care about master/slave status.
\* ********************************************************************* */

   /* *************************************************************** *\
    Dense vector operations, rather analogous to the mtx_value suite.
    These are tools for data motion. No dense-dense arithmetic operators
    are provided as these are best left to the user to design or steal
    from elsewhere, e.g. blas. The (de)allocation of dense vectors is
    the user's job,as is insuring that the dense vectors used are
    large enough to accomodate operations in the range of the given
    mtx_range_t. mtx->order is a safe size to use if you can't think
    of something else.
   \* *************************************************************** */

extern void mtx_org_row_vec(mtx_matrix_t mtx, int32 row,
                            real64 *vec, mtx_range_t *colrng);
/**< See mtx_cur_col_vec(), switching row & column references. */
ASC_DLLSPEC void mtx_org_col_vec(mtx_matrix_t mtx, int32 col,
                            real64 *vec, mtx_range_t *rowrng);
/**< See mtx_cur_col_vec(). */
extern void mtx_cur_row_vec(mtx_matrix_t mtx, int32 row,
                            real64 *vec, mtx_range_t *colrng);
/**< See mtx_cur_col_vec(), switching row & column references. */
extern void mtx_cur_col_vec(mtx_matrix_t mtx, int32 col,
                            real64 *vec, mtx_range_t *rowrng);
/**<
 ***  The user is expected to supply the vec; we cannot check it.
 ***  Copies the mtx nonzeros currently within rowrng INTO array vec which is
 ***  indexed by org/cur row number. Does not affect other
 ***  entries of vec in or outside the range. In particular, vec
 ***  is NOT zeroed within the range unless there is a matrix element
 ***  with value zero at that location.<br><br>
 ***
 ***  Notes: It is faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/

   /* *************************************************************** *\
    Sparse vector operations, rather analogous to the mtx_value suite.
    These are tools for data motion. No arithmetic operators
    are provided as yet. The deallocation of sparse vectors is
    the user's job. See the notes at the top of this header for the
    data semantics of a sparse vector.
    The user may construct a sparse vector.
    Calls which return a sparse vector may create or use and return
    a user supplied sparse vector. These calls are at liberty to
    reallocate the data memory if that supplied is insufficient to
    hold the data. The len and cap values of the vector will be reset
    as appropriate.

    Functions do not create a sparse unless it says in their header
    that mtx_CREATE_SPARSE is a valid argument.
   \* *************************************************************** */

extern mtx_sparse_t *mtx_org_row_sparse(mtx_matrix_t mtx,
                                        int32 row,
                                        mtx_sparse_t * const sparse,
                                        mtx_range_t *colrng,
                                        int zeroes);
/**< See mtx_cur_col_sparse(), switching row & column references. */
extern mtx_sparse_t *mtx_org_col_sparse(mtx_matrix_t mtx,
                                        int32 col,
                                        mtx_sparse_t * const sparse,
                                        mtx_range_t *rowrng,
                                        int zeroes);
/**< See mtx_cur_col_sparse(). */
extern mtx_sparse_t *mtx_cur_row_sparse(mtx_matrix_t mtx,
                                        int32 row,
                                        mtx_sparse_t * const sparse,
                                        mtx_range_t *colrng,
                                        int zeroes);
/**< See mtx_cur_col_sparse(), switching row & column references. */
extern mtx_sparse_t *mtx_cur_col_sparse(mtx_matrix_t mtx,
                                        int32 col,
                                        mtx_sparse_t * const sparse,
                                        mtx_range_t *rowrng,
                                        int zeroes);
/**<
 ***  Copies the mtx nonzeros currently within rowrng to the sparse,
 ***  indexing by org/cur row number. Nonzeros with value 0.0 WILL
 ***  be included in the sparse iff zeros is mtx_SOFT_ZEROES.
 ***  sparse->len will be set accordingly.
 ***
 ***  The user must supply the sparse. It will not be enlarged.
 ***  mtx_CREATE_SPARSE is not a valid argument.
 ***  If capacity of the sparse given is insufficient, we will
 ***  copy as much data as will fit into sparse and return NULL.
 ***  User beware!<br><br>
 ***
 ***  Notes: It is faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/

extern void mtx_zr_org_vec_using_row(mtx_matrix_t mtx, int32 row,
                                     real64 *vec, mtx_range_t *colrng);
/**< See mtx_zr_cur_vec_using_col(), switching row & column references. */
ASC_DLLSPEC void mtx_zr_org_vec_using_col(mtx_matrix_t mtx, int32 col,
                                     real64 *vec, mtx_range_t *rowrng);
/**< See mtx_zr_cur_vec_using_col(). */
extern void mtx_zr_cur_vec_using_row(mtx_matrix_t mtx, int32 row,
                                     real64 *vec, mtx_range_t *colrng);
/**< See mtx_zr_cur_vec_using_col(), switching row & column references. */
extern void mtx_zr_cur_vec_using_col(mtx_matrix_t mtx, int32 col,
                                     real64 *vec, mtx_range_t *rowrng);
/**< 
 ***  Sets the values of vec (indexed by org/cur col) corresponding to
 ***  incidences in rowrng to 0.0.<br><br>
 ***
 ***  Notes: It is faster to call this with mtx_ALL_COLS/ROWS when
 ***  practical, and the org flavor is faster than the cur flavor.<br><br>
 ***
 -$-  Does nothing given a bad matrix.
 **/

extern real64 mtx_sum_sqrs_in_row(mtx_matrix_t mtx, int32 row,
                                  const mtx_range_t *colrng);
/**<
 ***  Compute sum of squares of non-zeros in the given row whose column index
 ***  lies in the given column range.
 ***
 -$-  Returns 0.0 from a bad matrix.
 **/
extern real64 mtx_sum_sqrs_in_col(mtx_matrix_t mtx, int32 col,
                                  const mtx_range_t *rowrng);
/**<
 ***  Compute sum of squares of non-zeros in the given column whose row index
 ***  lies in the given row range.
 ***
 -$-  Returns 0.0 from a bad matrix.
 **/
extern real64 mtx_sum_abs_in_row(mtx_matrix_t mtx, int32 row,
                                 const mtx_range_t *colrng);
/**<
 ***  Compute sum of absolute values of non-zeros in the
 ***  given row whose column index lies in the given column range.
 ***
 -$-  Returns 0.0 from a bad matrix.
 **/
extern real64 mtx_sum_abs_in_col(mtx_matrix_t mtx, int32 col,
                                 const mtx_range_t *rowrng);
/**<
 ***  Compute sum of absolute values of non-zeros in the
 ***  given column whose row index lies in the given row range.
 ***
 -$-  Returns 0.0 from a bad matrix.
 **/

extern real64 mtx_col_dot_full_org_vec(mtx_matrix_t mtx,
                                       int32 col,
                                       real64 *orgvec,
                                       mtx_range_t *rowrng,
                                       boolean transpose);
/**< See mtx_row_dot_full_org_vec(), switching row & column references. */
extern real64 mtx_row_dot_full_org_vec(mtx_matrix_t mtx,
                                       int32 row,
                                       real64 *orgvec, 
                                       mtx_range_t *colrng,
                                       boolean transpose);
/**<
 ***  <pre>
 ***    For transpose==FALSE:
 ***    Compute the dot product of the row given with the org_col indexed
 ***    orgvec over the colrng given (colrng being the cur indexed
 ***    limits as usual.)
 ***    i.e. SUM( mtx(row,col_cur) * orgvec[mtx_col_to_org(mtx,col_cur)] )
 ***         for all inrange(col_cur).
 ***
 ***    For transpose==TRUE:
 ***    Compute the dot product of the row given with the org_row indexed
 ***    orgvec over the colrng given (colrng being the cur indexed
 ***    limits as usual.)
 ***    i.e. SUM( mtx(row,col_cur) * orgvec[mtx_row_to_org(mtx,col_cur)] )
 ***         for all inrange(col_cur).
 ***  </pre>
 -$-  Returns 0.0 from a bad matrix.
 **/

extern real64 mtx_col_dot_full_cur_vec(mtx_matrix_t mtx, 
                                       int32 col,
                                       real64 *curcolvec, 
                                       mtx_range_t *rowrng,
                                       boolean transpose);
/**< See mtx_row_dot_full_cur_vec(), switching row & column references. */
extern real64 mtx_row_dot_full_cur_vec(mtx_matrix_t mtx,
                                       int32 row,
                                       real64 *currowvec,
                                       mtx_range_t *colrng,
                                       boolean transpose);
/**<
 ***  Compute the dot product of the row given with the cur col indexed
 ***  currowvec over the colrng given (colrng being the cur indexed
 ***  limits as usual.)<br><br>
 ***
 -$-  Returns 0.0 from a bad matrix.<br><br>
 ***
 ! !  Transpose is currently not implemented. A warning will be issued.
 ! !  When someone finds a use and can explain what the transpose versions
 ! !  of these functions do in terms of permutations, it will be coded.<br><br>
 ***
 ***  Note: This pair of operators is slightly less expensive than
 ***  the mtx_*_dot_full_org_vec is.
 **/

extern real64 mtx_col_dot_full_org_custom_vec(mtx_matrix_t matrix1,
                                              mtx_matrix_t matrix2, 
                                              int32 row,
                                              real64 *orgvec, 
                                              mtx_range_t *colrng,
                                              boolean transpose);
/**< See mtx_row_dot_full_org_custom_vec(), switching row & column references. */
extern real64 mtx_row_dot_full_org_custom_vec(mtx_matrix_t matrix1,
                                              mtx_matrix_t matrix2,
                                              int32 col,
                                              real64 *orgvec, 
                                              mtx_range_t *rowrng,
                                              boolean transpose);
/**< 
 ***  <pre>
 ***    For transpose==FALSE:
 ***    Compute the dot product of the row given with the org_col indexed
 ***    orgvec (wrt matrix2) over the colrng given (colrng being the cur
 ***    indexed limits of matrix1.)
 ***    i.e. SUM( mtx(row,col_cur) *
 ***              orgvec[mtx_col_to_org(mtx2,col_cur)] )
 ***         for all inrange(col_cur).
 ***
 ***    For transpose==TRUE:
 ***    Compute the dot product of the row given with the org_row indexed
 ***    orgvec (wrt matrix2) over the colrng given (colrng being the cur
 ***    indexed limits of matrix1.)
 ***    i.e. SUM( mtx(row,col_cur) *
 ***              orgvec[mtx_row_to_org(mtx2,col_cur)] )
 ***         for all inrange(col_cur).
 ***  </pre>
 -$-  Returns 0.0 from a bad matrix.
 **/

extern void mtx_org_vec_add_col(mtx_matrix_t mtx, 
                                real64 *tvec,
                                int32 scol, 
                                real64 factor,
                                mtx_range_t *rowrng,
                                boolean transpose);
/**< See mtx_cur_vec_add_row(), switching row & column references. */
extern void mtx_org_vec_add_row(mtx_matrix_t mtx,
                                real64 *tvec,
                                int32 srow, 
                                real64 factor,
                                mtx_range_t *colrng,
                                boolean transpose);
/**< See mtx_cur_vec_add_row(). */
extern void mtx_cur_vec_add_col(mtx_matrix_t mtx,
                                real64 *tvec,
                                int32 scol, 
                                real64 factor,
                                mtx_range_t *rowrng,
                                boolean transpose);
/**< See mtx_cur_vec_add_row(), switching row & column references. */
extern void mtx_cur_vec_add_row(mtx_matrix_t mtx,
                                real64 *tvec,
                                int32 srow, 
                                real64 factor,
                                mtx_range_t *colrng,
                                boolean transpose);
/**< 
 ***  Adds multiple factor of srow to tvec for those columns in colrng.
 ***  tvec is org/cur col indexed if transpose==FALSE.
 ***    i.e. this is just adding rows.
 ***  tvec is org/cur row indexed if transpose==TRUE. 
 ***    orgvec[mtx_row_to_org(col)]+=factor*element(srow,col)
 ! !    curvec[???]+=factor*element(srow,col)
 ***
 ***  Since switching row and column is hard for the transpose, here it is:
 ***    orgvec[mtx_col_to_org(row)]+=factor*element(row,scol)
 ! !    curvec[???]+=factor*element(row,scol)
 ***   <br><br>
 ***  Notes: It is faster to use this with mtx_ALL_COLS/ROWS where
 ***  possible.
 ***  Use transpose==TRUE here if you would use transpose==TRUE
 ***  for dotting the row/col with the same vector.<br><br>
 ! !  Warning:
 ! !  Like mtx_row/col_dot_full_cur_vec,
 ! !  the transpose==TRUE flavors of mtx_cur_vec_add_row/col
 ! !  are NOT implemented. Nobody has found a use for them and nobody
 ! !  has yet cooked up what they mean in permutation terms.<br><br>
 ***
 -$-  Does nothing to a bad matrix.
 **/

/** @} */

#endif /* __MTX_QUERY_H_SEEN__ */
