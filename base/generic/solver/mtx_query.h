/*
 *  mtx2: Ascend Sparse Matrix Package
 *  by Benjamin Andrew Allan
 *  Derived from mtx by Karl Michael Westerberg
 *  Created: 5/3/90
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: mtx_query.h,v $
 *  Date last modified: $Date: 1997/07/18 12:15:12 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
#ifndef __MTX_QUERY_H_SEEN__
#define __MTX_QUERY_H_SEEN__
/* requires #include "mtx.h" */


extern real64 mtx_next_in_row(mtx_matrix_t, mtx_coord_t *,mtx_range_t *);
extern real64 mtx_next_in_col(mtx_matrix_t, mtx_coord_t *,mtx_range_t *);
/**
 -$-  value = mtx_next_in_row(matrix,coord,colrng)
 -$-  value = mtx_next_in_col(matrix,coord,rowrng)
 ***  real64 value;
 ***  mtx_matrix_t matrix;
 ***  mtx_coord_t *coord;
 ***  mtx_range_t *colrng,*rowrng;
 ***
 ***  mtx_next_in_row():
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
 ***  mtx_next_in_col():
 ***  Switch row <--> column in the discussion above.
 ***
 ***  Example of canonical usage:
 ***     nz.col = col;                   (* the col to be traversed *)
 ***     nz.row = mtx_FIRST;             (* initializer, which must be used *)
 ***     while( value = mtx_next_in_col(mtx,&nz,rowrng)),
 ***            nz.row != mtx_LAST ) {   (* your thing here *) }
 ***  Note that the logic test of the while is ONLY nz.row != mtx_LAST.
 ***  C lets you do whatever you choose before the comma.
 ***
 -$-  Returns FALSE from bad a matrix.
 ***
 ***  NOTE: The notion of "next non-zero" is arbitrary, but will not change
 ***  until the next destructive operation on the matrix is performed.
 ***  Neither the row/col number nor the rel/var index will
 ***  behave monotonically while traversing a col/row. This means there
 ***  is nothing you can do with the range to shrink the search space
 ***  based on the last row/col returned. Basic matrix operations, when
 ***  properly thought about in a general sparse context, do not care
 ***  about the order in which the elements of vectors are processed.
 ***
 ***  WARNINGS:
 !1!  - You may NOT nest mtx_next_in_col within mtx_next_in_col
 ! !  or mtx_next_in_row within mtx_next_in_row. We have yet to find a
 ! !  process suggestive of doing so that was not better handled with
 ! !  a for loop. This is a global constraint. Violation results in garbage.
 !2!  - You may NOT safely perform operation mtx_del_zr_in_col while traversing
 ! !  a column unless the value just returned from the previous next_in_col is
 ! !  nonzero or the coord.row last returned is mtx_LAST. Also, you may NOT
 ! !  safely perform operation mtx_del_zr_in_row on coord.row unless the
 ! !  value just returned from the previous next_in_col is nonzero.
 ! !  The proscription here is for mtx_next_in_ and mtx_del_zr_in_ operating
 ! !  on the same matrix. mtx_del_zr_in_ is safe to call if the mtx_next_in_
 ! !  driving it is happening on a different matrix.
 !3!  Transpose warning 2 for next_in_row.
 ! !  Violation of 2 or 3 results, sooner or later, in a memory fault.
 ***  You CAN nest mtx_next_in_col within mtx_next_in_row or vice versa,
 ***  however.
 **/

extern real64 mtx_row_max(mtx_matrix_t, mtx_coord_t *,
                                mtx_range_t *, real64 *);
extern real64 mtx_col_max(mtx_matrix_t, mtx_coord_t *,
                                mtx_range_t *, real64 *);
/**
 -$-  value = mtx_row_max(matrix,coord,colrng,signval)
 -$-  value = mtx_col_max(matrix,coord,rowrng,signval)
 -$-  value = mtx_row_min(matrix,coord,colrng,signval)
 -$-  value = mtx_col_min(matrix,coord,rowrng,signval)
 ***  real64 value, *signval;
 ***  mtx_matrix_t matrix;
 ***  mtx_coord_t *coord;
 ***  mtx_range_t *colrng,*rowrng;
 ***  
 ***  mtx_row_max():
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
 ***  mtx_col_max():
 ***  Replace row <--> column above.
 ***
 -$-  Returns -1.0 from a bad matrix.
 **/
extern real64 mtx_row_min(mtx_matrix_t, mtx_coord_t *,
                                mtx_range_t *, real64 *,real64);
extern real64 mtx_col_min(mtx_matrix_t, mtx_coord_t *,
                                mtx_range_t *, real64 *,real64);
/**
 -$-  value = mtx_row_min(matrix,coord,colrng,signval,minval)
 -$-  value = mtx_col_min(matrix,coord,rowrng,signval,minval)
 ***  real64 value, *signval, minval;
 ***  mtx_matrix_t matrix;
 ***  mtx_coord_t *coord;
 ***  mtx_range_t *colrng,*rowrng;
 ***
 ***  These functions are basicaly defined as their max counterparts
 ***  above with the appropriate swapping of max<-->min.
 ***  This function only looks at the nonzero elements
 ***  The min functions will only find numbers between minval
 ***  and 1e50.  If no number is found signval will be zero and
 ***  value will be one.
 **/
extern real64 mtx_get_pivot_col(mtx_matrix_t, mtx_coord_t *,
                                      mtx_range_t *, real64 *,
                                      real64, real64);
extern real64 mtx_get_pivot_row(mtx_matrix_t, mtx_coord_t *,
                                      mtx_range_t *, real64 *,
                                      real64, real64);
/**
 -$-  value = mtx_get_pivot_col(matrix,coord,colrng,signval,tol,eps)
 -$-  value = mtx_get_pivot_row(matrix,coord,rowrng,signval,tol,eps)
 ***  real64 value, *signval;
 ***  mtx_matrix_t matrix;
 ***  mtx_coord_t *coord;
 ***  mtx_range_t *colrng,*rowrng;
 ***  
 ***  This implements efficiently the standard sparse modification 
 ***  of LU partial pivot selection.
 ***  
 ***  mtx_get_pivot_col();
 ***  Searches for the leftmost element in the colrng of the given row, 
 ***  which passes the sparse partial pivoting criteria:
 ***    1) aij >= eps,
 ***    2) aij >= tol * max_abs_element_value_in_colrng.
 ***  The absolute value of the passing element is returned, or 0.0 if
 ***  there are no entries that pass criterion 1.  The user 
 ***  should set coord->row to the desired row to search. This function
 ***  will set coord->col to the column index where the result was found
 ***  (or mtx_NONE if nothing good was found in that range).  The absolute
 ***  value of that element is also returned. If the pointer signval is not
 ***  NULL, the real64 pointed to will be stuffed with the signed
 ***  value of the selected element.
 ***  This function is faster when colrng == mtx_ALL_COLS can be used.
 ***
 ***  mtx_get_pivot_row():
 ***  Replace row <--> column above.
 -$-  Returns -1.0 from a bad matrix.
 **/

extern int32  mtx_nonzeros_in_row(mtx_matrix_t,int32,mtx_range_t *);
extern int32  mtx_nonzeros_in_col(mtx_matrix_t,int32,mtx_range_t *);
extern int32  mtx_nonzeros_in_region(mtx_matrix_t, mtx_region_t *);
extern int32  mtx_numbers_in_row(mtx_matrix_t,int32,mtx_range_t *);
extern int32  mtx_numbers_in_col(mtx_matrix_t,int32,mtx_range_t *);
extern int32  mtx_numbers_in_region(mtx_matrix_t, mtx_region_t *);
/**
 -$-  count = mtx_nonzeros_in_row(matrix,row,colrng)
 -$-  count = mtx_nonzeros_in_col(matrix,col,rowrng)
 -$-  count = mtx_nonzeros_in_region(matrix,reg)
 -$-  count = mtx_numbers_in_row(matrix,row,colrng)
 -$-  count = mtx_numbers_in_col(matrix,col,rowrng)
 -$-  count = mtx_numbers_in_region(matrix,reg)
 ***  int32 count;
 ***  mtx_matrix_t matrix;
 ***  int32 row,col;
 ***  mtx_range_t *colrng,*rowrng;
 ***  mtx_region_t *reg;
 ***
 ***  mtx_nonzeros_in_row():
 ***  Counts the number of incidences in the given row whose column index
 ***  lies in the given column range. 
 ***  
 ***  mtx_nonzeros_in_col():
 ***  Replace row <--> column above.
 ***
 ***  mtx_nonzeros_in_region():
 ***  Counts the number of incidences in the given region.
 ***
 ***  mtx_numbers_in_row():
 ***  Counts the non-zero values in the given row whose column index
 ***  lies in the given column range. 
 ***  
 ***  mtx_numbers_in_col():
 ***  Replace row <--> column above.
 ***  
 ***  mtx_nonzeros_in_region():
 ***  Counts the non-zero values in the given region.
 ***  
 -$-  Returns -1 from a bad matrix.
 **/

/***********************************************************************\
  mtx vector operation routines
  None of these routines care about master/slave status.
\***********************************************************************/

   /*****************************************************************\
    Dense vector operations, rather analogous to the mtx_value suite.
    These are tools for data motion. No dense-dense arithmetic operators
    are provided as these are best left to the user to design or steal
    from elsewhere, e.g. blas. The (de)allocation of dense vectors is
    the user's job,as is insuring that the dense vectors used are 
    large enough to accomodate operations in the range of the given 
    mtx_range_t. mtx->order is a safe size to use if you can't think
    of something else. 
   \*****************************************************************/

extern void         mtx_org_row_vec(mtx_matrix_t, int32,
                                    real64 *, mtx_range_t *);
extern void         mtx_org_col_vec(mtx_matrix_t, int32,
                                    real64 *, mtx_range_t *);
extern void         mtx_cur_row_vec(mtx_matrix_t, int32,
                                    real64 *, mtx_range_t *);
extern void         mtx_cur_col_vec(mtx_matrix_t, int32,
                                    real64 *, mtx_range_t *);
/**
 -$-  mtx_org_row_vec(mtx,row,vec,colrng)
 -$-  mtx_org_col_vec(mtx,col,vec,rowrng)
 -$-  mtx_cur_row_vec(mtx,row,vec,colrng)
 -$-  mtx_cur_col_vec(mtx,col,vec,rowrng)
 ***  mtx_matrix_t mtx;
 ***  int32 row,col;
 ***  real64 *vec;
 ***  mtx_range_t *colrng,*rowrng;
 ***  
 ***  mtx_org/cur_row_vec:
 ***  The user is expected to supply the vec; we cannot check it.
 ***  Copies the mtx nonzeros currently within colrng INTO array vec which is
 ***  indexed by org/cur column number. Does not affect other
 ***  entries of vec in or outside the range. In particular, vec
 ***  is NOT zeroed within the range unless there is a matrix element
 ***  with value zero at that location.
 ***  
 ***  mtx_org/cur_col_vec:
 ***  Switch row <--> col in above.
 ***  
 ***  Notes: It is faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in 
 ***  the range of interest.
 ***
 -$-  Fetches nothing from a bad matrix.
 **/

   /*****************************************************************\
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
   \*****************************************************************/

extern mtx_sparse_t *mtx_org_row_sparse(mtx_matrix_t, int32,
                                        mtx_sparse_t * const, mtx_range_t *,
                                        int);
extern mtx_sparse_t *mtx_org_col_sparse(mtx_matrix_t, int32,
                                        mtx_sparse_t * const, mtx_range_t *,
                                        int);
extern mtx_sparse_t *mtx_cur_row_sparse(mtx_matrix_t, int32,
                                        mtx_sparse_t * const, mtx_range_t *,
                                        int);
extern mtx_sparse_t *mtx_cur_col_sparse(mtx_matrix_t, int32,
                                        mtx_sparse_t * const, mtx_range_t *,
                                        int);
/**
 -$-  sparse = mtx_org_row_sparse(mtx,row,sparse,colrng,zeroes)
 -$-  sparse = mtx_org_col_sparse(mtx,col,sparse,rowrng,zeroes)
 -$-  sparse = mtx_cur_row_sparse(mtx,row,sparse,colrng,zeroes)
 -$-  sparse = mtx_cur_col_sparse(mtx,col,sparse,rowrng,zeroes)
 ***  mtx_matrix_t mtx;
 ***  int32 row,col;
 ***  mtx_sparse_t *sparse;
 ***  mtx_range_t *colrng,*rowrng;
 ***  int zeroes;
 ***  
 ***  The user must supply the sparse. It will not be enlarged.
 ***  mtx_CREATE_SPARSE is not a valid argument.
 ***  If capacity of the sparse given is insufficient, we will
 ***  copy as much data as will fit into sparse and return NULL.
 ***  User beware!
 ***
 ***  mtx_org/cur_row_sparse:
 ***  Copies the mtx nonzeros currently within colrng to the sparse,
 ***  indexing by org/cur column number. Nonzeros with value 0.0 WILL
 ***  be included in the sparse iff zeros is mtx_SOFT_ZEROES.
 ***  sparse->len will be set accordingly.
 ***  
 ***  mtx_org/cur_col_sparse:
 ***  Switch row <--> col in above.
 ***  
 ***  Notes: It is faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in 
 ***  the range of interest.
 ***
 -$-  Fetches nothing from a bad matrix.
 **/


extern void         mtx_zr_org_vec_using_row(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *);
extern void         mtx_zr_org_vec_using_col(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *);
extern void         mtx_zr_cur_vec_using_row(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *);
extern void         mtx_zr_cur_vec_using_col(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *);
/**
 -$-  mtx_zr_org_vec_using_row(mtx,row,vec,colrng)
 -$-  mtx_zr_org_vec_using_col(mtx,col,vec,rowrng)
 -$-  mtx_zr_cur_vec_using_row(mtx,row,vec,colrng)
 -$-  mtx_zr_cur_vec_using_col(mtx,col,vec,rowrng)
 ***  mtx_matrix_t mtx;
 ***  int32 row,col;
 ***  real64 *vec;
 ***  mtx_range_t *colrng,*rowrng;
 ***  
 ***  mtx_zr_org/cur_vec_using_row:
 ***  Sets the values of vec (indexed by org/cur row) corresponding to
 ***  incidences in colrng to 0.0.
 ***
 ***  mtx_zr_org/cur_vec_using_col:
 ***  Switch row <--> col in above.
 ***  
 ***  Notes: It is faster to call this with mtx_ALL_COLS/ROWS when
 ***  practical, and the org flavor is faster than the cur flavor.
 ***
 -$-  Does nothing given a bad matrix.
 **/

extern real64 mtx_sum_sqrs_in_row(mtx_matrix_t, int32,
                                        const mtx_range_t *);
extern real64 mtx_sum_sqrs_in_col(mtx_matrix_t, int32,
                                        const mtx_range_t *);
extern real64 mtx_sum_abs_in_row(mtx_matrix_t, int32,
                                        const mtx_range_t *);
extern real64 mtx_sum_abs_in_col(mtx_matrix_t, int32,
                                        const mtx_range_t *);
/**
 -$-  sum = mtx_sum_sqrs_in_row(matrix,row,colrng)
 -$-  sum = mtx_sum_sqrs_in_col(matrix,col,rowrng)
 -$-  sum = mtx_sum_abs_in_row(matrix,row,colrng)
 -$-  sum = mtx_sum_abs_in_col(matrix,col,rowrng)
 ***  real64 sum;
 ***  mtx_matrix_t matrix;
 ***  int32 row,col;
 ***  mtx_range_t *colrng,*rowrng;
 ***
 ***  mtx_sum_sqrs_in_row():
 ***  Compute sum of squares of non-zeros in the given row whose column index
 ***  lies in the given column range.
 ***  mtx_sum_abs_in_row():
 ***  Compute sum of absolute values of non-zeros in the
 ***  given row whose column index lies in the given column range.
 ***
 ***  mtx_sum_sqrs_in_col():
 ***  Replace row <--> column above.
 ***  mtx_sum_abs_in_col():
 ***  Replace row <--> column above.
 -$-  Returns 0.0 from a bad matrix.
 **/

extern real64 mtx_row_dot_full_org_vec(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             boolean);

extern real64 mtx_col_dot_full_org_vec(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             boolean);
/**
 -$-  sum = mtx_row_dot_full_org_vec(matrix,row,orgvec,colrng,transpose)
 -$-  sum = mtx_col_dot_full_org_vec(matrix,col,orgvec,rowrng,transpose)
 ***  real64 sum;
 ***  mtx_matrix_t matrix;
 ***  int32 row,col;
 ***  real64 *orgvec;
 ***  mtx_range_t *colrng,*rowrng;
 ***  boolean transpose;
 ***
 ***  mtx_row_dot_full_org_vec:
 ***
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
 ***
 ***  mtx_col_dot_full_org_vec:
 ***  Replace row <--> col above.
 -$-  Returns 0.0 from a bad matrix.
 **/

extern real64 mtx_row_dot_full_cur_vec(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             boolean);
extern real64 mtx_col_dot_full_cur_vec(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             boolean);
/**
 -$-  sum = mtx_row_dot_full_cur_vec(matrix,row,currowvec,colrng,transpose)
 -$-  sum = mtx_col_dot_full_cur_vec(matrix,col,curcolvec,rowrng,transpose)
 ***  real64 sum;
 ***  mtx_matrix_t matrix;
 ***  int32 row,col;
 ***  real64 *currowvec, *curcolvec;
 ***  mtx_range_t *colrng,*rowrng;
 ***  boolean transpose;
 ***
 ! !  Transpose is currently not implemented. A warning will be issued.
 ! !  When someone finds a use and can explain what the transpose versions
 ! !  of these functions do in terms of permutations, it will be coded.
 ***
 ***  mtx_row_dot_full_cur_vec:
 ***  Compute the dot product of the row given with the cur col indexed
 ***  currowvec over the colrng given (colrng being the cur indexed
 ***  limits as usual.)
 ***
 ***  mtx_col_dot_full_cur_vec:
 ***  Replace row <--> column above.
 -$-  Returns 0.0 from a bad matrix.
 ***
 ***  Note: This pair of operators is slightly less expensive than
 ***  the mtx_*_dot_full_org_vec is.
 **/

extern real64 mtx_row_dot_full_org_custom_vec(mtx_matrix_t,
					      mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             boolean);

extern real64 mtx_col_dot_full_org_custom_vec(mtx_matrix_t,
					      mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             boolean);
/**
 -$-  sum =
 mtx_row_dot_full_org_custom_vec(matrix1,matrix2,row,orgvec,colrng,transpose)
 -$-  sum =
 mtx_col_dot_full_org_custom_vec(matrix1,matrix2,col,orgvec,rowrng,transpose)
 ***  real64 sum;
 ***  mtx_matrix_t matrix1;
 ***  mtx_matrix_t matrix2;
 ***  int32 row,col;
 ***  real64 *orgvec;
 ***  mtx_range_t *colrng,*rowrng;
 ***  boolean transpose;
 ***
 ***  mtx_row_dot_full_org_vec:
 ***
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
 ***
 ***  mtx_col_dot_full_org_custom_vec:
 ***  Replace row <--> col above.
 -$-  Returns 0.0 from a bad matrix.
 **/

extern void         mtx_org_vec_add_row(mtx_matrix_t, real64 *,
                                        int32, real64,
                                        mtx_range_t *,boolean);
extern void         mtx_org_vec_add_col(mtx_matrix_t, real64 *,
                                        int32, real64,
                                        mtx_range_t *,boolean);
extern void         mtx_cur_vec_add_row(mtx_matrix_t, real64 *,
                                        int32, real64,
                                        mtx_range_t *,boolean);
extern void         mtx_cur_vec_add_col(mtx_matrix_t, real64 *,
                                        int32, real64,
                                        mtx_range_t *,boolean);
/**
 -$-  mtx_org_vec_add_row(matrix,tvec,srow,factor,colrng,transpose)
 -$-  mtx_org_vec_add_col(matrix,tvec,scol,factor,rowrng,transpose)
 -$-  mtx_cur_vec_add_row(matrix,tvec,srow,factor,colrng,transpose)
 -$-  mtx_cur_vec_add_col(matrix,tvec,scol,factor,rowrng,transpose)
 ***  mtx_matrix_t matrix;
 ***  int32 srow,scol; 
 ***  real64 *tvec;
 ***  real64 factor;
 ***  mtx_range_t *colrng,*rowrng;
 ***  boolean transpose;
 ***  
 ***  mtx_org/cur_vec_add_row:
 ***  Adds multiple factor of srow to tvec for those columns in colrng.
 ***  tvec is org/cur col indexed if transpose==FALSE.
 ***    i.e. this is just adding rows.
 ***  tvec is org/cur row indexed if transpose==TRUE. 
 ***    orgvec[mtx_row_to_org(col)]+=factor*element(srow,col)
 ! !    curvec[???]+=factor*element(srow,col)
 ***  
 ***  mtx_org/cur_vec_add_col:
 ***  Reverse row <-->col in above.
 ***  Since this reversal is hard for the transpose, here it is:
 ***    orgvec[mtx_col_to_org(row)]+=factor*element(row,scol)
 ! !    curvec[???]+=factor*element(row,scol)
 ***   
 ***  Notes: It is faster to use this with mtx_ALL_COLS/ROWS where
 ***  possible.
 ***  Use transpose==TRUE here if you would use transpose==TRUE
 ***  for dotting the row/col with the same vector.
 ! !  Warning:
 ! !  Like mtx_row/col_dot_full_cur_vec,
 ! !  the transpose==TRUE flavors of mtx_cur_vec_add_row/col
 ! !  are NOT implemented. Nobody has found a use for them and nobody
 ! !  has yet cooked up what they mean in permutation terms.
 ***  
 -$-  Does nothing to a bad matrix.
 **/

#endif /* __MTX_QUERY_H_SEEN__ */
