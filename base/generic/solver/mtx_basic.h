/**< 
 *  mtx2: Ascend Sparse Matrix Package
 *  by Benjamin Andrew Allan
 *  Derived from mtx by Karl Michael Westerberg
 *  Created: 5/3/90
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: mtx_basic.h,v $
 *  Date last modified: $Date: 2000/01/25 02:27:10 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Benjamin Andrew Allan, Kirk Andre' Abbott
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
#ifndef __MTX_BASIC_H_SEEN__
#define __MTX_BASIC_H_SEEN__
/**< requires #include "mtx.h" */


/***********************************************************************\
  mtx types creation/destruction/info routines
\***********************************************************************/
extern boolean      check_matrix(mtx_matrix_t, char *, int);
#define mtx_check_matrix(m) check_matrix((m),__FILE__,__LINE__)
/**< (mtx_matrix_t, char *, int); */
/**< 
 ** Use mtx_check_matrix in all cases.
 ** If mtx given is ok, returns 1, otherwise returns 0.
 ** spits up to stderr if not ok.
 ** If mtx is a slave, also checks integrity on the master mtx.
 mm macro extern boolean mtx_check_matrix(m)
 **/

extern boolean      check_sparse(const mtx_sparse_t * const, char *, int);
#define mtx_check_sparse(sp) check_sparse((sp),__FILE__,__LINE__)
/**< (mtx_sparse_t *, char *, int); */
/**< 
 ** Use mtx_check_sparse in all cases.
 ** If sparse given is ok, returns 1, otherwise returns 0.
 ** spits up to stderr if not ok.
 mm macro extern boolean mtx_check_sparse(sp)
 **/

extern mtx_coord_t  *mtx_coord(mtx_coord_t *, int32, int32);
extern mtx_range_t  *mtx_range(mtx_range_t *, int32, int32);
extern mtx_region_t *mtx_region(mtx_region_t *, int32, int32,
                                int32, int32);
/**< 
 ***  coordp = mtx_coord(coordp,row,col)
 ***  rangep = mtx_range(rangep,low,high)
 ***  regionp = mtx_region(regionp,rowlow,rowhigh,collow,colhigh)
 ***  mtx_coord_t *coordp;
 ***  mtx_range_t *rangep;
 ***  mtx_region_t *regionp;
 ***  int32 row,col,low,high,rowlow,rowhigh,collow,colhigh;
 ***
 ***  Places the values of arguments 2,3,... into the structure pointed to
 ***  by argument 1 and returns the pointer to it again.
 *** 
 ***  Typical usage:
 ***  {
 ***     mtx_coord_t coord;
 ***     value = mtx_value(matrix,mtx_coord(&coord,row,col));
 ***  }
 **/

extern void mtx_zero_int32(int32 *, int);
extern void mtx_zero_real64(real64 *, int);
extern void mtx_zero_ptr(void **, int);
#define mtx_zero_char(ar,len) if ((ar)!=NULL) memset((ar),0,(len))
/**< 
 ***  mtx_zero_XXX(ptr,length);
 ***  Zeros a vector of specified length and type.
 ***  Ignore NULL input vectors.
 **/

extern mtx_matrix_t mtx_create();
/**< 
 ***  matrix = mtx_create()
 ***  mtx_matrix_t matrix;
 ***
 ***  Creates a 0-order matrix and returns a handle to it.
 ***  Matrix created is a master with no slaves.
 **/

extern mtx_matrix_t mtx_create_slave(mtx_matrix_t);
/**< 
 ***  matrix = mtx_create_slave(master)
 ***  mtx_matrix_t matrix, master;
 ***
 ***  Create and return a matrix which shares all structural
 ***  information EXCEPT incidence pattern/values with the
 ***  master matrix given. A master may have as many slaves
 ***  as desired. Slaves cannot have slaves. Slaves cannot
 ***  become masters. Copies of slave matrices are totally
 ***  independent of both slave and master.
 ***
 ***  All structural manipulation/query function calls will
 ***  be passed up to the master. When the master is resized, 
 ***  all its slaves are resized.
 ***
 ***  This function will return NULL if called on a matrix which
 ***  is a slave or otherwise bad.
 ***  
 ***  Slave matrices exist to:
 ***  - Let the user keep a whole stack of matrices in permutation
 ***    synchrony for the price of structural operations on only
 ***    the master matrix. Principally, this reduces mtx_drag cost.
 ***  - Reduce the memory overhead of a matrix duplicate
 ***    when the duplicate is to be kept in structural synchrony.
 ***    The additional memory required to maintain a slave matrix
 ***    is the cost of the incidence stored in the slave plus
 ***    2*mtx_capacity(master)*sizeof(double) + sizeof(void *).
 ***  - Demonstrate that C can knock the cookies out of FORTRAN
 ***    in speed while delivering twice the semantic content
 ***    in the output of a matrix factorization routine.
 **/

extern void mtx_debug_redirect_freeze();
/**< 
 *** stops future mtx_create/mtx_create_slave from
 *** redirecting errors back to stderr.
 **/

extern void         mtx_destroy(mtx_matrix_t);
/**< 
 ***  mtx_destroy(matrix);
 ***  mtx_matrix_t matrix;
 ***
 ***  Destroys the matrix, freeing the memory it occupied.
 ***  Does nothing if matrix fails mtx_check_matrix.
 ***  Destroys all slaves associated with the matrix if
 ***  it is a master. If mtx is a slave, its master must
 ***  also pass check_matrix before slave is destroyed.
 **/

extern mtx_sparse_t *mtx_create_sparse(int32);
/**< 
 ***  sp = mtx_create_sparse(capacity);
 ***  mtx_sparse_t *sp;
 ***  int32 capacity;
 ***  
 ***  Creates a sparse vector with capacity given and returns it.
 ***  The length of the sparse data is initialized to 0.
 ***  If insufficient memory is available, returns NULL.
 ***  
 **/

extern void         mtx_destroy_sparse(mtx_sparse_t *);
/**< 
 ***  mtx_destroy_sparse(sp);
 ***  mtx_sparse_t *sp;
 ***  
 ***  Given a pointer to the sparse structure, deallocates everything to
 ***  do with the structure, including the structure itself. The pointer
 ***  sp is invalidated. Handles NULL gracefully.
 **/

extern void         mtx_destroy_blocklist(mtx_block_t *);
/**< 
 ***  mtx_destroy_blocklist);
 ***  mtx_block_t *bl;
 ***  
 ***  Given a pointer to the block structure, deallocates everything to
 ***  do with the structure, including the structure itself. The pointer
 ***  bl is invalidated. Handles NULL gracefully.
 ***  If nblocks is 0 and the region array is not, region pointer will be
 ***  abandoned.
 **/

extern mtx_matrix_t mtx_duplicate_region(mtx_matrix_t, mtx_region_t *, real64);
/**< 
 -$-  slave = mtx_duplicate_region(matrix,region,drop);
 ***
 ***  mtx_matrix_t slave, matrix;
 ***  mtx_region_t *region;
 ***  real64 drop;
 ***
 ***  Creates a slave of the matrix given (or of the master of the matrix
 ***  given). This operator provides a low overhead way of
 ***  saving a matrix region for later computation. The slave matrix
 ***  returned is kept permuted to the same ordering as the master from
 ***  which it was created. 
 ***
 ***  The incidence is copied from the matrix given, even if the matrix
 ***  given is the slave of another matrix. During the copy, all Aij such
 ***  that abs(Aij) < drop  in the matrix given are ignored.
 ***
 ***  If you want a slave but do not want to copy any incidence, you should
 ***  just use mtx_create_slave(matrix); instead.
 ***
 -$-  Does nothing if matrix fails mtx_check_matrix; returns NULL.
 **/

extern mtx_matrix_t mtx_copy_options(mtx_matrix_t, boolean,
                                     boolean, mtx_region_t *, real64);
/**< 
 -$-  copy = mtx_copy_options(matrix,blocks,incidence,region,drop);
 ***
 ***  All operations can take either a master or a slave matrix
 ***  and all return a NEW master. If you want a slave copy, see
 ***  mtx_duplicate_region.
 ***
 ***  MACROS:
 -$-  copy = mtx_copy(matrix)
 -$-  copy = mtx_copy_region(matrix,region)
 -$-  copy = mtx_copy_region_drop(matrix,region,drop)
 -$-  copy = mtx_copy_wo_incidence(matrix)
 -$-  copy = mtx_copy_complete(matrix)
 ***  mtx_matrix_t copy,matrix;
 ***  boolean blocks, incidence;
 ***  mtx_region_t *region;
 ***  real64 drop;
 ***
 ***  copy = mtx_copy_complete(mtx):
 ***  Copies everything to do with a mtx. Copying the block information
 ***  is usually redundant because the caller should use the block info
 ***  from the original matrix, but in the odd event that such is not
 ***  the case, we provide the copy_complete operator.
 ***  Note that if you are copying a matrix created by mtx_copy
 ***  or mtx_copy_wo_incidence then the copy returned will not have
 ***  block data.
 ***
 ***  mtx_copy(mtx):
 ***  Copies the matrix except for the block structure and returns
 ***  a handle to the new copy. Most commonly used.
 ***
 ***  mtx_copy_region(matrix,region):
 ***  Copies the matrix excluding the block structure and any
 ***  incidence outside the region given.
 ***
 ***  mtx_copy_region_drop(matrix,region,drop):
 ***  As mtx_copy_region, except that incidence of magnitude < abs(drop) is
 ***  not copied. Note that in C 0.0 !< 0.0. Use a really small number if
 ***  you want 0s suppressed in the copy.
 ***
 ***  mtx_copy_wo_incidence(mtx):
 ***  Copies the matrix except for the nonzero and block structure
 ***  and returns a handle to the new copy. If you find yourself
 ***  doing mtx_copy followed by mtx_clear, use this instead.
 ***
 ***
 -$-  Does nothing if matrix fails mtx_check_matrix; returns NULL.
 ***
 mmm  macro extern  mtx_matrix_t  mtx_copy(m) 
 mmm  macro extern  mtx_matrix_t  mtx_copy_region(m,r)
 mmm  macro extern  mtx_matrix_t  mtx_copy_region_drop(m,r,d)
 mmm  macro extern  mtx_matrix_t  mtx_copy_wo_incidence(m)
 mmm  macro extern  mtx_matrix_t  mtx_copy_complete(m)
 **/
#define mtx_copy(m) mtx_copy_options((m),FALSE,TRUE,mtx_ENTIRE_MATRIX,0.0)
#define mtx_copy_region(m,r) mtx_copy_options((m),FALSE,TRUE,(r),0.0)
#define mtx_copy_region_drop(m,r,d) mtx_copy_options((m),FALSE,TRUE,(r),(d))
#define mtx_copy_wo_incidence(m) mtx_copy_options((m),FALSE,FALSE,NULL,0.0)
#define mtx_copy_complete(m) mtx_copy_options((m),TRUE,TRUE, \
mtx_ENTIRE_MATRIX,0.0)

extern int32  mtx_order(mtx_matrix_t);
/**< 
 ***  order = mtx_order(matrix)
 ***  int32 order;
 ***  mtx_matrix_t matrix;
 ***  
 ***  Returns the order of the matrix, be it master or slave.
 -$-  Order of a corrupt matrix is -1;
 **/

extern int32  mtx_capacity(mtx_matrix_t);
/**< 
 ***  cap = mtx_capacity(matrix)
 ***  int32 cap;
 ***  mtx_matrix_t matrix;
 ***
 ***  Returns the capacity of the matrix, be it master or slave.
 ***  Original row/column numbers must lie in the range 0 <= ndx < cap.
 ***  cap may be larger than the matrix order if the order of the matrix
 ***  was ever at time higher than it is now.
 -$-  capacity of a corrupt matrix is -1;
 **/

extern void         mtx_set_order(mtx_matrix_t, int32);
/**< 
 ***  mtx_set_order(matrix,order)
 ***  mtx_matrix_t matrix;
 ***  int32 order;
 ***  
 ***  Changes the order of the matrix to the new order, either truncating
 ***  the matrix, or extending it with blank rows and columns if necessary.
 ***  Does not change the incidence pattern if the matrix is being expanded.
 ***  Calls on slaves are passed up to the master. Calls on a master reset
 ***  the order for all of its slaves.
 ***
 ***  Bugs:
 ***  This should return a memory allocation status boolean, but doesn't.
 -$-  Does nothing to a bad matrix.
 **/

extern void         mtx_clear_coord(mtx_matrix_t, int32,int32);
extern void         mtx_clear_row(mtx_matrix_t,int32,mtx_range_t *);
extern void         mtx_clear_col(mtx_matrix_t,int32,mtx_range_t *);
extern void         mtx_clear_rowlist(mtx_matrix_t,
                                      mtx_sparse_t *,mtx_range_t *);
extern void         mtx_clear_collist(mtx_matrix_t,
                                      mtx_sparse_t *,mtx_range_t *);
extern void         mtx_clear_region(mtx_matrix_t, mtx_region_t *);
extern void         mtx_reset_perm(mtx_matrix_t);
extern void         mtx_clear(mtx_matrix_t);
/**< 
 -$-  mtx_clear_coord(matrix,row,col)
 -$-  mtx_clear_row(matrix,row,rng)
 -$-  mtx_clear_col(matrix,col,rng)
 -$-  mtx_clear_rowlist(matrix,sp,rng)  !* not yet implemented *!
 -$-  mtx_clear_collist(matrix,sp,rng)  !* not yet implemented *!
 -$-  mtx_clear_region(matrix,region)
 -$-  mtx_reset_perm(matrix)
 -$-  mtx_clear(matrix)
 ***  mtx_matrix_t matrix;
 ***  mtx_sparse_t *sp;
 ***  mtx_region_t *region;
 ***  mtx_range_t *rowrng;
 ***  
 ***  mtx_clear_coord will make sure a specific element doesn't exist.
 ***  mtx_clear_row and mtx_clear_col will erase a range of elements
 ***    in a single row or column.
 ***  mtx_clear_rowlist and mtx_clear_collist will erase a range of elements
 ***    in the list of rows or columns given in the idata of the sparse.
 ***    The data of the sparse need not be valid as it will not be referenced.
 ***  mtx_clear_region erases all elements in the given region.
 ***  mtx_reset_perm restores the original row/column ordering.
 ***  mtx_clear clears everything.
 -$-  Does nothing to a bad matrix.
 ***  
 ***  It is generally most efficient to clear as many incidences in possible
 ***  in a single call. When a set of adjacent rows/columns is to be cleared,
 ***  use mtx_clear_region. If a rather disjoint set is to be cleared (such
 ***  as the nonlinear rows of a mixed linear/nonlinear jacobian) use
 ***  mtx_clear_rowlist/collist.
 ***  
 ***  Note that mtx_clear and mtx_reset_perm invalidate any data saved
 ***  with the mtx_*_block_perm functions.
 ***  Notes on masters/slaves:
 ***    mtx_clear_coord, mtx_clear_row, mtx_clear_col, mtx_clear_rowlist,
 ***      mtx_clear_collist work as usual. 
 ***    mtx_clear_region(slave,region) can be used without affecting other
 ***      slaves or the master.
 ***    mtx_clear_region(master,region) affects only the master UNLESS
 ***      region is mtx_ENTIRE_MATRIX. mtx_ENTIRE_MATRIX clears the master
 ***      and ALL incidence in ALL its slaves.
 ***    mtx_reset_perm(master or slave) passes the reset up to the master.
 ***    mtx_clear(master or slave) passes up to the master.
 ***     
 **/

extern real64 mtx_value(mtx_matrix_t, mtx_coord_t *);
extern void         mtx_set_value(mtx_matrix_t, mtx_coord_t *, real64);
extern void         mtx_fill_value(mtx_matrix_t, mtx_coord_t *, real64);
extern void         mtx_fill_org_value(mtx_matrix_t, const mtx_coord_t *,
                                       real64);
/**< 
 -$-  value = mtx_value(matrix,coord)
 -$-  mtx_set_value(matrix,coord,value)
 -$-  mtx_fill_value(matrix,coord,value)
 -$-  mtx_fill_org_value(matrix,orgcoord,value)
 ***  real64 value;
 ***  mtx_matrix_t matrix;
 ***  mtx_coord_t *coord, *orgcoord;
 ***  
 ***  Use of mtx_value, mtx_set_value should be avoided if at all possible
 ***  inside loops. See mtx_next_in_* for doing mtx/vector operations.
 ***  Returns/sets the value of the given element in the matrix.
 ***  Because the only sane usage of mtx_value, mtx_set_value is for
 ***  things like getting and setting something in the diagonal,
 ***  mtx_set_value remembers the last place mtx_value returned
 ***  and checks it before starting a search. Any element destruction
 ***  causes mtx_set_value to forget, however.
 ***  This remembrance is matrix specific and is not affected by
 ***  element destruction in other unrelated matrices.
 ***  
 ***  mtx_fill_value is
 ***  to be used in place of mtx_set_value in those instances where the
 ***  caller knows there currently exists no element at the coordinate
 ***  whose value is to be set. mtx_fill_value can also be used if a
 ***  mtx_assemble call will happen before ANY other numeric or structural
 ***  calls are made.
 ***  
 ***  mtx_fill_org_value is
 ***  just like mtx_fill_value except the location given is understood as
 ***  the (orgrow,orgcol) location to put the fill.
 ***  
 ***  mtx_set_value(mtx,coord,0.0) will create no incidence if none
 ***  is presently there, nor does it delete an incidence.
 ***  
 ***  Doesn't matter whether a matrix is slave or master.
 ***
 -$-  Returns 0.0/does nothing from/to a bad matrix.
 **/
/**< grandfathering old linsol. remove when linsolqr replaces linsol */
#define mtx_add_value(a,b,c) mtx_fill_value((a),(b),(c))

extern int32  mtx_assemble(mtx_matrix_t);
/**< 
 ***  dinc = mtx_assemble(mtx_matrix_t);
 ***  mtx_matrix_t matrix;
 ***  int32 dinc; 
 ***
 ***  Takes a matrix, assumed to have redundant and otherwise insane incidences
 ***  created by mtx_fill_value and sums all like entries, eliminating
 ***  the duplicates and the zeroes. Returns -# of duplicate elements removed.
 ***  returns 1 if fails for some reason. 
 ***  Could stand to have the error messages it emits improved.
 ***  Could stand to take a rowrange or a rowlist,
 ***  a colrange or a collist,droptol. Zeroes are not counted as duplicates.
 ***  algorithm cost: O(3*nnz)
 **/

/***********************************************************************\
  mtx element routines
  None of these routines care about master/slave status.
\***********************************************************************/
extern void         mtx_del_zr_in_row(mtx_matrix_t, int32);
extern void         mtx_del_zr_in_col(mtx_matrix_t, int32);
extern void         mtx_del_zr_in_rowrange(mtx_matrix_t, mtx_range_t *);
extern void         mtx_del_zr_in_colrange(mtx_matrix_t, mtx_range_t *);
/**< 
 -$-  mtx_del_zr_in_row(matrix,row)
 -$-  mtx_del_zr_in_col(matrix,col)
 -$-  mtx_del_zr_in_rowrange(matrix,rng)
 -$-  mtx_del_zr_in_colrange(matrix,rng)
 ***  mtx_matrix_t matrix;
 ***  int32 row,col;
 ***  mtx_range_t *rng;
 ***
 ***  "Non-zeros" in the given row/col/rng of rows/cols which are actually
 ***  zero are effectively removed (i.e. the status of "non-zero" is revoked).
 -$-  Does nothing to a bad matrix.
 **/

extern void         mtx_steal_org_row_vec(mtx_matrix_t, int32,
                                          real64 *, mtx_range_t *);
extern void         mtx_steal_org_col_vec(mtx_matrix_t, int32,
                                          real64 *, mtx_range_t *);
extern void         mtx_steal_cur_row_vec(mtx_matrix_t, int32,
                                          real64 *, mtx_range_t *);
extern void         mtx_steal_cur_col_vec(mtx_matrix_t, int32,
                                          real64 *, mtx_range_t *);
/**< 
 -$-  mtx_steal_org_row_vec(mtx,row,vec,colrng)
 -$-  mtx_steal_org_col_vec(mtx,col,vec,rowrng)
 -$-  mtx_steal_cur_row_vec(mtx,row,vec,colrng)
 -$-  mtx_steal_cur_col_vec(mtx,col,vec,rowrng)
 ***  mtx_matrix_t mtx;
 ***  int32 row,col;
 ***  real64 *vec;
 ***  mtx_range_t *colrng,*rowrng;
 ***  
 ***  mtx_steal_org/cur_row_vec:
 ***  The user is expected to supply the vec; we cannot check it.
 ***  Copies the mtx nonzeros currently within colrng INTO array vec which is
 ***  indexed by org/cur column number. Does not affect other
 ***  entries of vec in or outside the range. In particular, vec
 ***  is NOT zeroed within the range unless there is a matrix element
 ***  with value zero at that location.
 ***  All incidence within colrng of the row given is removed from the matrix.
 ***  
 ***  mtx_steal_org/cur_col_vec:
 ***  Switch row <--> col in above.
 ***  
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
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

extern boolean      mtx_steal_org_row_sparse(mtx_matrix_t, int32,
                                             mtx_sparse_t *, mtx_range_t *);
extern boolean      mtx_steal_org_col_sparse(mtx_matrix_t, int32,
                                             mtx_sparse_t *, mtx_range_t *);
extern boolean      mtx_steal_cur_row_sparse(mtx_matrix_t, int32,
                                             mtx_sparse_t *, mtx_range_t *);
extern boolean      mtx_steal_cur_col_sparse(mtx_matrix_t, int32,
                                             mtx_sparse_t *, mtx_range_t *);
/**< 
 -$-  err = mtx_steal_org_row_sparse(mtx,row,sparse,colrng)
 -$-  err = mtx_steal_org_col_sparse(mtx,col,sparse,rowrng)
 -$-  err = mtx_steal_cur_row_sparse(mtx,row,sparse,colrng)
 -$-  err = mtx_steal_cur_col_sparse(mtx,col,sparse,rowrng)
 ***  mtx_matrix_t mtx;
 ***  int32 row,col;
 ***  mtx_sparse_t *sparse;
 ***  mtx_range_t *colrng,*rowrng;
 ***  boolean err;
 ***  
 ***  The user must supply the sparse; if it is too small, err will be 
 ***  TRUE and data will not be collected and incidence not cleared.
 ***  mtx_CREATE_SPARSE is not a valid argument to this function.
 ***  What is too small? For all flavors the sparse must
 ***  have at least the capacity indicated by the col/rowrng.
 ***  
 ***  mtx_steal_org/cur_row_sparse:
 ***  Copies the mtx nonzeros currently within colrng to the sparse,
 ***  indexing by org/cur column number. Nonzeros with value 0.0 WILL NOT
 ***  be included in the sparse. sparse->len will be set accordingly.
 ***  All incidence within colrng will be deleted from the mtx.
 ***  
 ***  mtx_steal_org/cur_col_sparse:
 ***  Switch row <--> col in above.
 ***  
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in 
 ***  the range of interest.
 ***
 -$-  Fetches nothing from a bad matrix.
 **/

/**< 
extern wish  ! ! !  NOT YET IMPLEMENTED. NO USE FOR THEM SO FAR.
extern void  ! ! !  mtx_set_org_row_vec(mtx_matrix_t, int32,
                                        real64 *, mtx_range_t *,boolean);
extern void  ! ! !  mtx_set_org_col_vec(mtx_matrix_t, int32,
                                        real64 *, mtx_range_t *,boolean);
extern void  ! ! !  mtx_set_cur_row_vec(mtx_matrix_t, int32,
                                        real64 *, mtx_range_t *,boolean);
extern void  ! ! !  mtx_set_cur_col_vec(mtx_matrix_t, int32,
                                        real64 *, mtx_range_t *,boolean);
 -$-  mtx_set_org_row_vec(mtx,row,vec,colrng,destructive)
 -$-  mtx_set_org_col_vec(mtx,col,vec,rowrng,destructive)
 -$-  mtx_set_cur_row_vec(mtx,row,vec,colrng,destructive)
 -$-  mtx_set_cur_col_vec(mtx,col,vec,rowrng,destructive)
 ***  mtx_matrix_t mtx;
 ***  int32 row,col;
 ***  real64 *vec;
 ***  mtx_range_t *colrng,*rowrng;
 ***  boolean destructive;
 ***  
 ***  mtx_set_org/cur_row_vec:
 ***  Copies the nonzeros currently within colrng FROM array vec which is
 ***  indexed by org/cur column number INTO the matrix.
 ***  If destructive is TRUE, existing nonzeros which are given value 0.0
 ***
 ***  mtx_set_org/cur_col_vec:
 ***  Switch row <--> col in above.
 ***  
 ***  Notes: It is faster to call this with a narrow range than with
 ***  mtx_ALL_COLS/ROWS. For ranges with very low sparsity, it may
 ***  be faster to call mtx_clear_row/col followed by mtx_fill_*_*_vec,
 ***  depending on the time cost of allocating and deallocating small
 ***  pieces of memory on the machine in question.
 ***  
 ! !  Warning: If destructive is TRUE, care should be taken when using
 ! !  these operators within a mtx_next_in_* loop that the current element
 ! !  of the loop is not zeroed by the vec causing it to be deallocated.
 ***
 -$-  Puts nothing to a bad matrix.
 **/

extern void         mtx_fill_org_row_vec(mtx_matrix_t, int32,
                                         real64 *, mtx_range_t *);
extern void         mtx_fill_org_col_vec(mtx_matrix_t, int32,
                                         real64 *, mtx_range_t *);
extern void         mtx_fill_cur_row_vec(mtx_matrix_t, int32,
                                         real64 *, mtx_range_t *);
extern void         mtx_fill_cur_col_vec(mtx_matrix_t, int32,
                                         real64 *, mtx_range_t *);
extern void         mtx_dropfill_cur_row_vec(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             real64);
extern void         mtx_dropfill_cur_col_vec(mtx_matrix_t, int32,
                                             real64 *, mtx_range_t *,
                                             real64);
/**< 
 -$-  mtx_fill_org_row_vec(mtx,row,vec,colrng)
 -$-  mtx_fill_org_col_vec(mtx,col,vec,rowrng)
 -$-  mtx_fill_cur_row_vec(mtx,row,vec,colrng)
 -$-  mtx_fill_cur_col_vec(mtx,col,vec,rowrng)
 -$-  mtx_dropfill_cur_row_vec(mtx,row,vec,colrng,tolerance)
 -$-  mtx_dropfill_cur_col_vec(mtx,col,vec,rowrng,tolerance)
 ***  mtx_matrix_t mtx;
 ***  int32 row, col;
 ***  real64 tolerance, *vec;
 ***  mtx_range_t *colrng, *rowrng;
 ***  
 ***  mtx_fill_org/cur_row_vec:
 ***  Assumes that the colrng of row in the matrix is empty and
 ***  copies the nonzeros currently within colrng FROM array vec which is
 ***  indexed by org/cur column number INTO the matrix.
 ***
 ***  mtx_fill_org/cur_col_vec:
 ***  Switch row <--> col in above.
 ***  
 ***  mtx_dropfill_cur_row/col_vec:
 ***  Assumes that the colrng of row in the matrix is empty and
 ***  copies the values such that abs(value) >tolerance currently
 ***  within colrng FROM array vec which is
 ***  indexed by cur column/row number INTO the matrix.
 ***
 ***  Notes: It is faster to call these with a narrow range than with
 ***  mtx_ALL_COLS/ROWS. It is very marginally faster to call the cur
 ***  rather than the org flavor of these functions.
 ! !  If you use this when the range is NOT empty, you will sooner or
 ! !  later certainly lose numerical integrity and you may lose memory 
 ! !  integrity. The sparse matrix programmer cannot afford to be naive.
 ***
 -$-  Puts nothing to a bad matrix.
 **/

extern void         mtx_fill_org_row_sparse(mtx_matrix_t, int32,
                                            const mtx_sparse_t *);
extern void         mtx_fill_org_col_sparse(mtx_matrix_t, int32,
                                            const mtx_sparse_t *);
extern void         mtx_fill_cur_row_sparse(mtx_matrix_t, int32,
                                            const mtx_sparse_t *);
extern void         mtx_fill_cur_col_sparse(mtx_matrix_t, int32,
                                            const mtx_sparse_t *);
/**< 
 -$-  mtx_fill_org_row_sparse(mtx,row,sp)
 -$-  mtx_fill_org_col_sparse(mtx,col,sp)
 -$-  mtx_fill_cur_row_sparse(mtx,row,sp)
 -$-  mtx_fill_cur_col_sparse(mtx,col,sp)
 ***  mtx_matrix_t mtx;
 ***  int32 row, col;
 ***  mtx_sparse_t *sp;
 ***
 ***  mtx_fill_org_row_sparse:
 ***    Fills the current row given with the data in the sparse given.
 ***    The indices in sp->idata are taken as org col indices.
 ***    It is assumed that the row is empty, at least where data is
 ***    being added.
 ***  mtx_fill_org_col_sparse:
 ***    Swap row/col in the previous description.
 ***
 ***  mtx_fill_cur_row_sparse:
 ***    Fills the current row given with the data in the sparse given.
 ***    The indices in sp->idata are taken as cur col indices.
 ***    It is assumed that the row is empty, at least where data is
 ***    being added.
 ***  mtx_fill_cur_col_sparse:
 ***    Swap row/col in the previous description.
 ***
 ***  All these functions ignore 0.0 in the data and do not create numbers
 ***  there in the mtx.
 **/


extern void         mtx_mult_row(mtx_matrix_t,int32,
                                 real64,mtx_range_t *);
extern void         mtx_mult_col(mtx_matrix_t,int32,
                                 real64,mtx_range_t *);
extern void         mtx_mult_row_zero(mtx_matrix_t,int32,mtx_range_t *);
extern void         mtx_mult_col_zero(mtx_matrix_t,int32,mtx_range_t *);
/**< 
 -$-  mtx_mult_row(matrix,row,factor,colrng)
 -$-  mtx_mult_col(matrix,col,factor,rowrng)
 -$-  mtx_mult_row_zero(matrix,row,colrng)
 -$-  mtx_mult_col_zero(matrix,col,rowrng)
 ***  mtx_matrix_t matrix;
 ***  int32 row,col;
 ***  real64 factor;
 ***  mtx_range_t *colrng,*rowrng;
 ***
 ***  Multiplies the given row/column by a given factor.  Only those
 ***  elements with column/row index within the given range are multiplied.
 ***  mtx_mult_row/col tests for factor=0.0 and blows away the row if true.
 ***  mtx_mult_row/col_zero zeros without disturbing structure.
 -$-  Does nothing on a bad matrix.
 **/

extern void         mtx_add_row(mtx_matrix_t,int32,int32,
                                real64,mtx_range_t *);
extern void         mtx_add_col(mtx_matrix_t,int32,int32,
                                real64,mtx_range_t *);
/**< 
 -$-  mtx_add_row(matrix,srow,trow,factor,colrng)
 -$-  mtx_add_col(matrix,scol,tcol,factor,rowrng)
 ***  mtx_matrix_t matrix;
 ***  int32 srow,trow,scol,tcol;
 ***  real64 factor;
 ***  mtx_range_t *colrng,*rowrng;
 ***  
 ***  Adds a given multiple of row srow/column scol to row trow/column tcol.
 ***  Only those elements with column/row index within the given range are
 ***  so affected.
 -$-  Does nothing on a bad matrix.
 **/

extern void         mtx_add_row_series(int32, real64,
                                       mtx_range_t *);
extern void         mtx_add_col_series(int32, real64,
                                       mtx_range_t *);
extern void         mtx_add_row_series_init(mtx_matrix_t, int32,
                                            boolean);
extern void         mtx_add_col_series_init(mtx_matrix_t, int32,
                                            boolean);
/**< 
 -$-  mtx_add_row_series(srow,factor,colrng)
 -$-  mtx_add_col_series(scol,factor,rowrng)
 -$-  mtx_add_row_series_init(matrix,trow,userow)
 -$-  mtx_add_col_series_init(matrix,tcol,usecol)
 ***  mtx_matrix_t matrix;
 ***  int32 srow,trow,scol,tcol;
 ***  real64 factor;
 ***  mtx_range_t *colrng,*rowrng; 
 ***  boolean userow,usecol;
 ***  
 ***  mtx_add_row/col_series_init(mtx, t>=0, bool):
 ***    Grab a row/column t of the matrix to have several rows/columns
 ***    from the same matrix added to it. bool is ignored for t>=0.
 ***    Only one row/col at a time may be grabbed for all existing 
 ***    matrices. (One row and one col may be grabbed contemporaneously.)
 ***    You must release the row/col (see below) before you can grab another.
 ***  
 ***  mtx_add_row/col_series_init(mtx, mtx_NONE, userow/col):
 ***    Release the last row/column which was grabbed for multiple adds.
 ***    If userow/col is TRUE, the current contents of row/col t
 ***    (from the previous call) will be used to release the row/col.
 ***    If userow/col is FALSE, a pessimistic release method will be
 ***    used instead of the row/col previously specified.
 ! !    If ANY destructive operations have been done on the row/col, call
 ! !    this with userow/col==FALSE.
 ! !    The mtx given to a release call must match that given in the grabbing
 ! !    or the release is ignored.
 ***    For very dense rows/cols, it may be faster to call with userow/col
 ***    == FALSE since the release with TRUE requires a row/col traversal.
 ***  
 ***  mtx_add_row/col_series(s,factor,rowrng)
 ***    Adds the given multiple of row/column s to trow/tcol.
 ***    Only those elements of s with column/row index within the
 ***    given range are added to trow/tcol.
 ***    When possible, range mtx_ALL_COLS/ROWS is faster for sparse rows.
 ! !    Calling these without a prior call to series_init is an error.
 ***  
 ! !  Warning: You MUST release any grabbed row/col created on a matrix
 ! !  before destroying that matrix. Failure to do so is fatal.
 ***  
 -$-  Does nothing on a bad matrix.
 **/

/**< 
extern wish  ! ! !  NOT YET IMPLEMENTED. NO USE FOR THEM SO FAR. 
extern void  ! ! !  mtx_add_row_org_vec(mtx_matrix_t,int32,
                                        real64 *,real64,
                                        mtx_range_t *, boolean);
extern void  ! ! !  mtx_add_col_org_vec(mtx_matrix_t,int32,
                                        real64 *,real64,
                                        mtx_range_t *, boolean);
extern void  ! ! !  mtx_add_row_cur_vec(mtx_matrix_t,int32,
                                        real64 *,real64,
                                        mtx_range_t *, boolean);
extern void  ! ! !  mtx_add_col_cur_vec(mtx_matrix_t,int32,
                                        real64 *,real64,
                                        mtx_range_t *, boolean);
 -$-  mtx_add_row_org_vec(matrix,trow,svec,factor,colrng,transpose)
 -$-  mtx_add_col_org_vec(matrix,tcol,svec,factor,rowrng,transpose)
 -$-  mtx_add_row_cur_vec(matrix,trow,svec,factor,colrng,transpose)
 -$-  mtx_add_col_cur_vec(matrix,tcol,svec,factor,rowrng,transpose)
 ***  mtx_matrix_t matrix;
 ***  int32 trow,tcol;
 ***  real64 *svec;
 ***  real64 factor;
 ***  mtx_range_t *colrng,*rowrng;
 ***  boolean transpose;
 ***  
 ***  mtx_add_row_org/cur_vec:
 ***  In matrix trow[col] += factor*svec[col] for elements of svec in colrng.
 ***  svec is org/cur indexed, by row if !transpose and by column if transpose.
 ***  
 ***  mtx_add_col_org/cur_vec:
 ***  Reverse row <-->col in above
 ***  
 ***  This is an absurdly expensive thing to do. Think very carefully about
 ***  whether you are ignoring existing sparsity info before asking that this
 ***  mtx primitive be coded.
 -$-  Does nothing on a bad matrix.
 **/


extern void mtx_old_add_row_sparse(mtx_matrix_t, int32, real64 *,
                                  real64, mtx_range_t *, int32 *);
extern void mtx_old_add_col_sparse(mtx_matrix_t, int32, real64 *,
                                  real64, mtx_range_t *, int32 *);
/**< 
 ***  Note: the interface of this operator needs some work!
 ***  In particular, it should be redone with consideration given
 ***  to the mtx_sparse_t recently added.
 ***  
 -$-  mtx_old_add_row_sparse(matrix,row,drow,factor,colrng,collist)
 -$-  mtx_old_add_col_sparse(matrix,col,dcol,factor,rowrng,rowlist)
 ***  mtx_matrix_t matrix;
 ***  int32 row,col,*collist,*rowlist;
 ***  real64 *drow, *dcol;
 ***  real64 factor;
 ***  mtx_range_t *colrng,*rowrng;
 ***  
 ***  Dense in these function names refers to the row/col data type,
 ***  a vector as opposed to an mtx row/col, not the information density.
 ***  Adds factor*(drow/dcol)[j] to row/col.
 ***  Drow/dcol are assumed to be the size of the matrix and in current
 ***  row/col order, not original order.
 ***  
 ***  If colrng/rowrng is mtx_ALL_COLS/ROWS, then the following is TRUE:
 ***    If collist/rowlist is NOT NULL, it will be used instead of the rng
 ***    to determine which columns/rows j of drow/dcol are added to row/col.
 ***    Collist/rowlist must be an array terminated by -1. The sanity of
 ***    current indices in collist/rowlist is the users responsibility.
 ***    In particular, in list mode the value of drow/dcol is added blindly
 ***    rather than being checked for 0.0 and the range of indices is not
 ***    checked against the matrix size.
 ***    If collist/rowlist IS NULL the range will be mtx_ALL_COLS/ROWS.
 ***  If a range is specified, collist/rowlist is ignored and only those
 ***  elements with column/row index j within the given range are affected.
 ***  
 ***  List mode is faster if you are adding the same dense vector
 ***  (differing only by factor) across many (>2) rows/columns. 
 ***  Range mode is more convenient. The caller should switch between the 
 ***  two as appropriate.
 -$-  Does nothing on a bad matrix.
 **/

extern void mtx_add_row_sparse(mtx_matrix_t, int32, real64, mtx_sparse_t *);
extern void mtx_add_col_sparse(mtx_matrix_t, int32, real64, mtx_sparse_t *);
/**< 
 -$-  mtx_add_row_sparse(matrix,row,factor,sparse)
 -$-  mtx_add_col_sparse(matrix,col,factor,sparse)
 ***  mtx_matrix_t matrix;
 ***  int32 row,col;
 ***  real64 factor;
 ***  mtx_sparse_t *sparse;  expected to be in org coords.
 ***  NOT IMPLEMENTED.
 **/

extern size_t       mtx_size(mtx_matrix_t);
extern size_t       mtx_chattel_size(mtx_matrix_t);
/**< 
 *** size=mtx_size(mtx)
 *** size=mtx_chattel_size(mtx)
 *** size_t size;
 *** mtx_matrix_t mtx;
 ***
 ***  mtx_size(mtx);
 ***  Returns the amount of memory in use by a matrix and all its
 ***  bits and pieces.
 ***  Slaves report only their incremental cost, that is they do
 ***  not report the cost of the structures they share with a master.
 ***  Masters do report the cost of their slaves.
 ***
 ***  mtx_chattel_size(master);
 ***  Returns the memory used by all slaves of the matrix given
 ***  that is not shared with the master. Returns 0 from a slave.
 **/

extern void         mtx_free_reused_mem();
/**< 
 ***  mtx_free_reused_mem()
 ***
 ***  Deallocates any memory that mtx may be squirrelling away for
 ***  internal reuse. Calling this while any slv_system_t exists
 ***  is likely to be fatal: handle with care.
 **/

/***********************************************************************\
  mtx io routines
\***********************************************************************/
extern void         mtx_write_sparse(FILE *,mtx_sparse_t *);
/**< 
 ***  mtx_write_sparse(file,sparse);
 ***  Outputs sizes and data of sparse vector to the file given.
 **/

extern void         mtx_write_region_human_f(FILE *,mtx_matrix_t,
                                             mtx_region_t *,int,int);
/**< 
 ***  MACROS:
 ***  mtx_write_region_human(file,mtx,region);
 ***    Grandfather support for the old usages.
 ***  mtx_write_region_human_rows(file,mtx,region);
 ***    Writes row oriented human readable output of a mtx region.
 ***  mtx_write_region_human_cols(file,mtx,region);
 ***    Writes column oriented human readable output of a mtx region.
 ***
 ***  Internal:
 ***  mtx_write_region_human_f(file,mtx,region,colwise,orgwise);
 ***  Outputs permutation and values of the nonzero elements in the
 ***  given region of the mtx to the file given.
 ***  If colwise != 0, output will be column grouped,
 ***  otherwise it will be row grouped.
 ***  If orgwise != 0, only org indices will be printed.
 ***  Doesn't care about master/slave status.
 ***
 mmm  macro extern void mtx_write_region_human(file,mtx,region) 
 mmm  macro extern void mtx_write_region_human_rows(file,mtx,region) 
 mmm  macro extern void mtx_write_region_human_cols(file,mtx,region) 
 mmm  macro extern void mtx_write_region_human_orgrows(file,mtx,region) 
 mmm  macro extern void mtx_write_region_human_orgcols(file,mtx,region) 
 **/
#define mtx_write_region_human(f,m,r) mtx_write_region_human_f((f),(m),(r),0,0)
#define mtx_write_region_human_rows(f,m,r) \
mtx_write_region_human_f((f),(m),(r),0,0)
#define mtx_write_region_human_cols(f,m,r) \
mtx_write_region_human_f((f),(m),(r),1,0)
#define mtx_write_region_human_orgrows(f,m,r) \
mtx_write_region_human_f((f),(m),(r),0,1)
#define mtx_write_region_human_orgcols(f,m,r) \
mtx_write_region_human_f((f),(m),(r),1,1)

extern void         mtx_write_region(FILE *,mtx_matrix_t,mtx_region_t *);
/**< 
 ***  mtx_write_region(file,mtx,region)
 ***  Outputs permutation and values of the nonzero elements in the
 ***  given region of the mtx to the file given along with the matrix
 ***  size.
 ***  The coordinates of the nonzeros written will be in original
 ***  (unpermuted) indexing. This file is for mtx_read_region, but is
 ***  in ASCII for portability.
 ***  Doesn't care about master/slave status.
 ***  Not intended for human consumptions, but just so you know
 ***  permutations are written in order r2org org2r c2org org2c.
 ***  parity is not written.
 ***  If the region given is mtx_ENTIRE_MATRIX, and there is a
 ***  block structure present in the matrix, it will be written as well
 ***  and the symbolic rank will go out with the block structure.
 **/

extern mtx_matrix_t mtx_read_region(FILE *,mtx_matrix_t,int);
/**< 
 ***  mtx_read_region(file,mtx,transposed)
 ***  From the file pointer, does the following:
 ***  Gets the matrix order of the data in the file.
 ***  If mtx is NULL, creates it. 
 ***  Expands the matrix given or created to the data order if it is too small.
 ***  Permutes the matrix to the permutation found in the file.
 ***  Reads the region limits in the file and clears that region in the mtx.
 ***  Reads coefficients from the file.
 ***
 ***  transposed governs if the matrix data should be treated as
 ***  row col 	or 	col row
 ! !  Warnings:   Do not add incidence data to the
 ! !  file except in those org_rows and org_cols which are within the
 ! !  region specified (note that the region is given in CUR coordinates.)
 ! !  Adding incidence outside the region may lead to loss of internal
 ! !  consistency unless the entire mtx given is empty when this is called.
 ! !  If you send a matrix larger than the order of data in the file,
 ! !  you should do a reset to it first to undo the previous permutation.
 ***  
 ***  Return value is the pointer to the mtx given or created.
 ***  If error in reading matrix file, mtx returned may be incomplete or NULL
 ***  if the mtx given was NULL.
 ***  If the file read has block information in it and further the user sent
 ***  in NULL to this function, block info will be read and added to the
 ***  returned matrix.
 ***
 ***  Doesn't care about master/slave status.
 **/

extern void         mtx_write_region_matlab(FILE *,mtx_matrix_t,mtx_region_t *);
/**< 
 ***  mtx_write_region_matlab(file,mtx,region)
 ***  Outputs values of the nonzero elements in the
 ***  given region of the mtx to the file in matlab/harwell sparse format.
 ***  The "a=[\n" and trailing "];\n" are not supplied, since the caller
 ***  knows better what they should look like.
 ***  Row/column coordinates printed are the cur coordinates.
 **/

extern void         mtx_write_region_plot(FILE *,mtx_matrix_t,mtx_region_t *);
/**< 
 ***  mtx_write_region_plot(file,mtx,region)
 ***  Outputs the coordinates of elements to file with format suitable
 ***  for xgraph consumption from the given region of the mtx.
 ***  A suitable xgraph invocation would be "xgraph -nl -m filename".
 ***  Doesn't care about master/slave status.
 **/

extern void         mtx_write_region_csr(FILE *,mtx_matrix_t,
					 mtx_region_t *, int);
extern void         mtx_write_region_smms(FILE *,mtx_matrix_t,
					  mtx_region_t *, int);
extern mtx_matrix_t mtx_read_smms(FILE *,mtx_matrix_t,int);

/**< 
 ***  mtx_write_region_csr(file,mtx,region,offset)
 ***  mtx_write_region_smms(file,mtx,region,offset)
 ***  mtx = mtx_read_smms(file,mtx,transpose);
 ***
 ***  Writes the given region of the matrix to the named file.
 ***  Will write the entire matrix if the region is mtx_ENTIRE_MATRIX.
 ***  The _csr version writes out the matrix in compressed row format.
 ***  The _smms version writes out the matrix in a form digestible by
 ***  Alvarado's Sparse Matrix Manipulation System.
 ***  There may be a _ccs version (column based) one day.
 ***  offset controls whether fortran (1) or c style indexing is done.
 ***
 ***  mtx_read_smms reads a matrix in smms format. If a NULL matrix
 ***  is sent in, it will create and return it. If a non NULL matrix
 ***  is given, the order will be increased if necessary. The contents
 ***  of the old mtx, will be blown away. The transpose flag dictates
 ***  whether the transpose should be read in. 
 ***  Doesn't care about master/slave status.
 **/

extern void         mtx_exception_recover();
/**< 
 ***  mtx_exception_recover();
 ***  You don't need to know what this does, except that you should call
 ***  it any time it is probable that a floating point exception has
 ***  occurred during matrix manipulations.  Several functions use
 ***  data structures that can cause insanity in the event of exception.
 ***
 ***  Just for the curious, it resets several internal data structures
 ***  needed including ones used in the operators:
 ***    mtx_add_row/col
 ***    mtx_add_row/col_series
 ***    mtx_add_row/col_series_init
 ***    mtx_assemble
 ***    mtx_add_outer_product
 **/
 
extern void         mtx__debug_output(FILE *,mtx_matrix_t);
/**< 
 ***  mtx_debug_output(fp,matrix)
 ***  FILE *fp;
 ***  mtx_matrix_t matrix;
 ***  
 ***  Debug outputs all internal information about a matrix to file.
 ***  In the process, integrity checks are performed.
 ***  If file is NULL, output goes to default (which is stderr.)
 ***  Doesn't care about master/slave status.
 **/

#endif /**< __MTX_BASIC_H_SEEN__ */
