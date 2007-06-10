/*	ASCEND Solver Interface
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1995 Benjamin Andrew Allan, Kirk Andre' Abbott
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*//** @file
	Basic matrix operations
*//*
	Originally 'mtx' by Karl Michael Westerberg, 5/3/90
	New version 'mtx2' by Benjamin Andrew Allan
	Last in CVS: $Revision: 1.13 $ $Date: 2000/01/25 02:27:10 $ $Author: ballan $
*/

#ifndef ASC_MTX_BASIC_H
#define ASC_MTX_BASIC_H

#include "mtx.h"

/**	@addtogroup linear Linear
	@{
*/

/***********************************************************************\
  mtx types creation/destruction/info routines
\***********************************************************************/
extern boolean check_matrix(mtx_matrix_t mtx, char *file, int line);
/**<
 *  Implementation function for mtx_check_matrix().
 *  Do not call this function directly - use mtx_check_matrix() instead.
 *  @todo check_matrix() should return TRUE/FALSE, not 1/0.
 */
#define mtx_check_matrix(mtx) check_matrix((mtx),__FILE__,__LINE__)
/**<
 **  Checks the integrity flag of the matrix.
 **  Returns 1 if mtx is ok (non-NULL, integrity flag ok).
 **  Otherwise returns 0.  If mtx is not ok, a message is
 **  printed to g_mtxerr.   If mtx is a slave, the integrity
 **  on the master mtx is also checked.
 **  @param  mtx  mtx_matrix_t, the matrix to check.
 **  @return 1 if given matrix is valid, 0 otherwise.
 **/

extern boolean check_sparse(const mtx_sparse_t * const sp, 
                            char *file, int line);
/**<
 *  Implementation function for mtx_check_sparse().
 *  Do not call this function directly - use mtx_check_sparse() instead.
 *  @todo check_sparse() should return TRUE/FALSE, not 1/0.
 */
#define mtx_check_sparse(sp) check_sparse((sp),__FILE__,__LINE__)
/**<
 **  Checks the consistency of a sparse matrix as best it can.
 **  Returns 1 if sp is ok, FALSE if something wierd is found.
 **  If sp is not ok, a message is  printed to g_mtxerr.
 **  @param  sp  const mtx_sparse_t * const, the sparse matrix to check.
 **  @return 1 if given matrix is valid, 0 otherwise.
 **/

ASC_DLLSPEC mtx_coord_t *mtx_coord(mtx_coord_t *coordp, int32 row, int32 col);
/**<
	Places the values of row and col into coordp and returns
	the coordp pointer again.
	
	Typical usage:                                     <pre>
	  {
	    mtx_coord_t coord;
	    value = mtx_value(matrix,mtx_coord(&coord,row,col));
	  }                                                </pre>
*/
extern mtx_range_t *mtx_range(mtx_range_t *rangep, int32 low, int32 high);
/**<
	Places the values of low and high into rangep and returns
	the rangep pointer again.
*/
ASC_DLLSPEC mtx_region_t *mtx_region(mtx_region_t *regionp,
                                int32 rowlow, int32 rowhigh,
                                int32 collow, int32 colhigh);
/**<
	Places the ranges specified by (rowlow,rowhigh) and
	(collow,colhigh) into regionp and returns the
	regionp pointer again.
*/

ASC_DLLSPEC void mtx_zero_int32(int32 *data, int len);
/**< Zeros an int32 vector of specified length. Ignores NULL input vectors. */
ASC_DLLSPEC void mtx_zero_real64(real64 *data, int len);
/**< Zeros a real64 vector of specified length. Ignores NULL input vectors. */
extern void mtx_zero_ptr(void **data, int len);
/**< Zeros a vector of pointers of specified length. Ignores NULL input vectors. */
#define mtx_zero_char(ar,len) if ((ar)!=NULL) memset((ar),0,(len))
/**<
	Zeros a char vector of specified length. Ignores NULL input vectors.
*/

ASC_DLLSPEC mtx_matrix_t mtx_create(void);
/**<
	Creates a 0-order matrix and returns a handle to it.
	Matrix created is a master with no slaves.
*/

extern mtx_matrix_t mtx_create_slave(mtx_matrix_t master);
/**<
 ***  Create and return a matrix which shares all structural
 ***  information EXCEPT incidence pattern/values with the
 ***  master matrix given. A master may have as many slaves
 ***  as desired. Slaves cannot have slaves. Slaves cannot
 ***  become masters. Copies of slave matrices are totally
 ***  independent of both slave and master.<br><br>
 ***
 ***  All structural manipulation/query function calls will
 ***  be passed up to the master. When the master is resized,
 ***  all its slaves are resized.<br><br>
 ***
 ***  This function will return NULL if called on a matrix which
 ***  is a slave or otherwise bad.<br><br>
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

extern void mtx_debug_redirect_freeze(void);
/**<
 ***  Stops future mtx_create()/mtx_create_slave() from
 ***  redirecting errors back to stderr.
 **/

ASC_DLLSPEC void mtx_destroy(mtx_matrix_t matrix);
/**<
 ***  Destroys the matrix, freeing the memory it occupied.
 ***  Does nothing if matrix fails mtx_check_matrix.
 ***  Destroys all slaves associated with the matrix if
 ***  it is a master. If mtx is a slave, its master must
 ***  also pass check_matrix before slave is destroyed.
 **/

extern mtx_sparse_t *mtx_create_sparse(int32 capacity);
/**<
 ***  Creates a sparse vector with capacity given and returns it.
 ***  The length of the sparse data is initialized to 0.
 ***  If insufficient memory is available, returns NULL.
 **/

ASC_DLLSPEC void mtx_destroy_sparse(mtx_sparse_t *sp);
/**<
 ***  Given a pointer to the sparse structure, deallocates everything to
 ***  do with the structure, including the structure itself. The pointer
 ***  sp is invalidated. Handles NULL gracefully.
 **/

extern void mtx_destroy_blocklist(mtx_block_t *bl);
/**<
 ***  Given a pointer to the block structure, deallocates everything to
 ***  do with the structure, including the structure itself. The pointer
 ***  bl is invalidated. Handles NULL gracefully.
 ***  If nblocks is 0 and the region array is not, region pointer will be
 ***  abandoned.
 **/

extern mtx_matrix_t mtx_duplicate_region(mtx_matrix_t matrix,
                                         mtx_region_t *region,
                                         real64 drop);
/**<
 ***  Creates a slave of the matrix given (or of the master of the matrix
 ***  given). This operator provides a low overhead way of
 ***  saving a matrix region for later computation. The slave matrix
 ***  returned is kept permuted to the same ordering as the master from
 ***  which it was created.<br><br>
 ***
 ***  The incidence is copied from the matrix given, even if the matrix
 ***  given is the slave of another matrix. During the copy, all Aij such
 ***  that abs(Aij) < drop  in the matrix given are ignored.<br><br>
 ***
 ***  If you want a slave but do not want to copy any incidence, you should
 ***  just use mtx_create_slave(matrix); instead.<br><br>
 ***
 -$-  Does nothing if matrix fails mtx_check_matrix; returns NULL.
 **/

extern mtx_matrix_t mtx_copy_options(mtx_matrix_t matrix,
                                     boolean blocks, boolean incidence,
                                     mtx_region_t *region, real64 drop);
/**<
 ***  General matrix copy function.
 ***  All operations can take either a master or a slave matrix
 ***  and all return a NEW master. If you want a slave copy, see
 ***  mtx_duplicate_region.<br><br>
 ***
 ***  This function should generally be considered an implementation
 ***  function not to be called directly.  Use one of the macros described
 ***  below to perform the type of copy desired.
 ***
 ***  <pre>
 ***  mtx_copy_complete(mtx)
 ***    Copies everything to do with a mtx. Copying the block information
 ***    is usually redundant because the caller should use the block info
 ***    from the original matrix, but in the odd event that such is not
 ***    the case, we provide the copy_complete operator.
 ***    Note that if you are copying a matrix created by mtx_copy
 ***    or mtx_copy_wo_incidence then the copy returned will not have
 ***    block data.
 ***
 ***  mtx_copy(mtx)
 ***    Copies the matrix except for the block structure and returns
 ***    a handle to the new copy. Most commonly used.
 ***
 ***  mtx_copy_region(matrix,region)
 ***    Copies the matrix excluding the block structure and any
 ***    incidence outside the region given.
 ***
 ***  mtx_copy_region_drop(matrix,region,drop)
 ***    As mtx_copy_region(), except that incidence of magnitude < abs(drop) is
 ***    not copied. Note that in C 0.0 !< 0.0. Use a really small number if
 ***    you want 0s suppressed in the copy.
 ***
 ***  mtx_copy_wo_incidence(mtx)
 ***    Copies the matrix except for the nonzero and block structure
 ***    and returns a handle to the new copy. If you find yourself
 ***    doing mtx_copy followed by mtx_clear, use this instead.<br><br>
 ***
 -$-  Does nothing if matrix fails mtx_check_matrix; returns NULL.
 ***  </pre>
 **/
#define mtx_copy(m) mtx_copy_options((m),FALSE,TRUE,mtx_ENTIRE_MATRIX,0.0)
/**< See mtx_copy_options(). */
#define mtx_copy_region(m,r) mtx_copy_options((m),FALSE,TRUE,(r),0.0)
/**< See mtx_copy_options(). */
#define mtx_copy_region_drop(m,r,d) mtx_copy_options((m),FALSE,TRUE,(r),(d))
/**< See mtx_copy_options(). */
#define mtx_copy_wo_incidence(m) mtx_copy_options((m),FALSE,FALSE,NULL,0.0)
/**< See mtx_copy_options(). */
#define mtx_copy_complete(m) mtx_copy_options((m),TRUE,TRUE, mtx_ENTIRE_MATRIX,0.0)
/**< See mtx_copy_options(). */

ASC_DLLSPEC int32 mtx_order(mtx_matrix_t matrix);
/**<
 ***  Returns the order of the matrix, be it master or slave.
 -$-  Order of a corrupt matrix is -1;
 **/

ASC_DLLSPEC int32 mtx_capacity(mtx_matrix_t matrix);
/**< 
 ***  Returns the capacity of the matrix, be it master or slave.
 ***  Original row/column numbers must lie in the range 0 <= ndx < cap.
 ***  cap may be larger than the matrix order if the order of the matrix
 ***  was ever at time higher than it is now.
 -$-  capacity of a corrupt matrix is -1;
 **/

ASC_DLLSPEC void mtx_set_order(mtx_matrix_t matrix, int32 order);
/**< 
 ***  Changes the order of the matrix to the new order, either truncating
 ***  the matrix, or extending it with blank rows and columns if necessary.
 ***  Does not change the incidence pattern if the matrix is being expanded.
 ***  Calls on slaves are passed up to the master. Calls on a master reset
 ***  the order for all of its slaves.
 ***
 ***  @bug This should return a memory allocation status boolean,
 ***       but doesn't.  Does nothing to a bad matrix.
 **/

extern void mtx_clear_coord(mtx_matrix_t matrix, int32 row, int32 col);
/**<
 ***  Erase an element in a matrix.  The specified element must
 ***  exist to be erased.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.
 ***  Will work as usual on masters/slaves.
 ***  @see mtx_clear() for more about matrix clearing functions.
 **/

extern void mtx_clear_row(mtx_matrix_t matrix, int32 row, mtx_range_t *rng);
/**<
 ***  Erase a range of elements in a single row of a matrix.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.
 ***  Will work as usual on masters/slaves.
 ***  @see mtx_clear() for more about matrix clearing functions.
 **/

extern void mtx_clear_col(mtx_matrix_t matrix,int32 col, mtx_range_t *rng);
/**<
 ***  Erase a range of elements in a single column of a matrix.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.
 ***  Will work as usual on masters/slaves.
 ***  @see mtx_clear() for more about matrix clearing functions.
 **/

extern void mtx_clear_rowlist(mtx_matrix_t matrix,
                              mtx_sparse_t *sp, mtx_range_t *rng);
/**<
 ***  NOT IMPLEMENTED.
 ***  Erase a range of elements in the list of rows given in
 ***  the idata of the sparse.  The data of the sparse need not
 ***  be valid as it will not be referenced.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.
 ***  Will work as usual on masters/slaves.
 ***  @see mtx_clear() for more about matrix clearing functions.
 **/

extern void mtx_clear_collist(mtx_matrix_t matrix,
                              mtx_sparse_t *sp, mtx_range_t *rng);
/**<
 ***  NOT IMPLEMENTED.
 ***  Erase a range of elements in the list of columns given in
 ***  the idata of the sparse.  The data of the sparse need not
 ***  be valid as it will not be referenced.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.
 ***  Will work as usual on masters/slaves.
 ***  @see mtx_clear() for more about matrix clearing functions.
 **/

ASC_DLLSPEC void mtx_clear_region(mtx_matrix_t matrix, mtx_region_t *region);
/**<
 ***  Erases all elements in the given region.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.<br><br>
 ***  mtx_clear_region(slave,region) can be used without affecting other
 ***      slaves or the master.
 ***  mtx_clear_region(master,region) affects only the master UNLESS
 ***      region is mtx_ENTIRE_MATRIX. mtx_ENTIRE_MATRIX clears the master
 ***      and ALL incidence in ALL its slaves.
 ***  @see mtx_clear() for more about matrix clearing functions.
 **/

extern void mtx_reset_perm(mtx_matrix_t matrix);
/**<
 ***  Restores the original row/column ordering.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.<br><br>
 ***  Note that mtx_reset_perm() invalidates any data saved with
 ***  the mtx_*_block_perm functions.<br><br>
 ***  mtx_reset_perm(master or slave) passes the reset up to the master.
 ***  @see mtx_clear() for more about matrix clearing functions.
 **/

ASC_DLLSPEC void mtx_clear(mtx_matrix_t matrix);
/**<
 ***  Clears everything in the matrix.
 -$-  Does nothing to a bad matrix if MTX_DEBUG is defined.<br><br>
 ***
 ***  It is generally most efficient to clear as many incidences in possible
 ***  in a single call. When a set of adjacent rows/columns is to be cleared,
 ***  use mtx_clear_region. If a rather disjoint set is to be cleared (such
 ***  as the nonlinear rows of a mixed linear/nonlinear jacobian) use
 ***  mtx_clear_rowlist/collist.<br><br>
 ***
 ***  Note that mtx_clear() invalidates any data saved with the
 ***  mtx_*_block_perm functions.<br><br>
 ***  mtx_clear(master or slave) passes up to the master.
 **/


ASC_DLLSPEC real64 mtx_value(mtx_matrix_t matrix, mtx_coord_t *coord);
/**<
 ***
 ***  Use of mtx_value() should be avoided if at all possible
 ***  inside loops. See mtx_next_in_* for doing mtx/vector operations.
 ***  Returns the value of the given element in the matrix.
 ***  Doesn't matter whether a matrix is slave or master.
 -$-  Returns 0.0/does nothing from/to a bad matrix.
 **/

ASC_DLLSPEC void mtx_set_value(mtx_matrix_t matrix,
                          mtx_coord_t *coord, real64 value);
/**<
 ***  Sets the value of the given element in the matrix.
 ***  Because the only sane usage of mtx_set_value() is for
 ***  things like setting something in the diagonal,
 ***  mtx_set_value() remembers the last place mtx_value() returned
 ***  and checks it before starting a search. Any element destruction
 ***  causes mtx_set_value() to forget, however.
 ***  This remembrance is matrix specific and is not affected by
 ***  element destruction in other unrelated matrices.
 ***  Like mtx_value(), use of mtx_set_value() should be avoided if at
 ***  all possible inside loops.  See mtx_next_in_* for doing mtx/vector
 ***  operations.<br><br>
 ***
 ***  mtx_set_value(mtx,coord,0.0) will create no incidence if none
 ***  is presently there, nor does it delete an incidence.
 ***
 ***  Doesn't matter whether a matrix is slave or master.
 ***
 -$-  Returns 0.0/does nothing from/to a bad matrix.
 *** @see mtx_fill_value()
 **/

ASC_DLLSPEC void mtx_fill_value(mtx_matrix_t matrix,
                           mtx_coord_t *coord, real64 value);
/**<
 ***  Sets the value of the given element in the matrix.
 ***  Use this function in place of mtx_set_value() in those instances 
 ***  where the caller knows there currently exists no element at the 
 ***  coordinate whose value is to be set.  mtx_fill_value() can also be 
 ***  used if a  mtx_assemble() call will happen before ANY other numeric 
 ***  or structural calls are made.
 ***
 ***  Doesn't matter whether a matrix is slave or master.
 ***
 -$-  Returns 0.0/does nothing from/to a bad matrix.
 ***  @see mtx_fill_org_value()
 **/

extern void mtx_fill_org_value(mtx_matrix_t matrix,
                               const mtx_coord_t *coord,
                               real64 value);
/**<
 ***  Sets the value of the given element in the matrix.
 ***  This function is just like mtx_fill_value() except the location
 ***  given is understood as the (orgrow,orgcol) location to put the fill.
 ***
 ***  Doesn't matter whether a matrix is slave or master.
 ***
 -$-  Returns 0.0/does nothing from/to a bad matrix.
 **/

/** grandfathering old linsol. @todo Remove when linsolqr replaces linsol. */
#define mtx_add_value(a,b,c) mtx_fill_value((a),(b),(c))

extern int32 mtx_assemble(mtx_matrix_t matrix);
/**<
 ***  Takes a matrix, assumed to have redundant and otherwise insane incidences
 ***  created by mtx_fill_value and sums all like entries, eliminating
 ***  the duplicates and the zeroes. Returns -# of duplicate elements removed.
 ***  returns 1 if fails for some reason.
 ***  @todo Could stand to have the error messages it emits improved.
 ***  @todo Could stand to take a rowrange or a rowlist,
 ***        a colrange or a collist,droptol.
 ***  Zeroes are not counted as duplicates.
 ***  algorithm cost: O(3*nnz)
 **/

/* ********************************************************************* *\
  mtx element routines
  None of these routines care about master/slave status.
\* ********************************************************************* */

extern void mtx_del_zr_in_row(mtx_matrix_t matrix, int32 row);
/**<
 ***  "Non-zeros" in the given row which are actually
 ***  zero are effectively removed (i.e. the status of "non-zero" is revoked).
 -$-  Does nothing to a bad matrix.
 **/
extern void mtx_del_zr_in_col(mtx_matrix_t matrix, int32 col);
/**<
 ***  "Non-zeros" in the given col which are actually
 ***  zero are effectively removed (i.e. the status of "non-zero" is revoked).
 -$-  Does nothing to a bad matrix.
 **/
extern void mtx_del_zr_in_rowrange(mtx_matrix_t matrix, mtx_range_t *rng);
/**<
 ***  "Non-zeros" in the given range of rows which are actually
 ***  zero are effectively removed (i.e. the status of "non-zero" is revoked).
 -$-  Does nothing to a bad matrix.
 **/
extern void mtx_del_zr_in_colrange(mtx_matrix_t matrix, mtx_range_t *rng);
/**<
 ***  "Non-zeros" in the given range of cols which are actually
 ***  zero are effectively removed (i.e. the status of "non-zero" is revoked).
 -$-  Does nothing to a bad matrix.
 **/

extern void mtx_steal_org_row_vec(mtx_matrix_t mtx, int32 row,
                                  real64 *vec, mtx_range_t *colrng);
/**<
 ***  The user is expected to supply the vec; we cannot check it.
 ***  Copies the mtx nonzeros currently within colrng INTO array vec which is
 ***  indexed by org column number. Does not affect other
 ***  entries of vec in or outside the range. In particular, vec
 ***  is NOT zeroed within the range unless there is a matrix element
 ***  with value zero at that location.
 ***  All incidence within colrng of the row given is removed from the matrix.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/
extern void mtx_steal_org_col_vec(mtx_matrix_t mtx, int32 col,
                                  real64 *vec, mtx_range_t *rowrng);
/**<
 ***  The user is expected to supply the vec; we cannot check it.
 ***  Copies the mtx nonzeros currently within rowrng INTO array vec which is
 ***  indexed by org row number. Does not affect other
 ***  entries of vec in or outside the range. In particular, vec
 ***  is NOT zeroed within the range unless there is a matrix element
 ***  with value zero at that location.
 ***  All incidence within rowrng of the col given is removed from the matrix.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/
extern void mtx_steal_cur_row_vec(mtx_matrix_t mtx, int32 row,
                                  real64 *vec, mtx_range_t *colrng);
/**<
 ***  The user is expected to supply the vec; we cannot check it.
 ***  Copies the mtx nonzeros currently within colrng INTO array vec which is
 ***  indexed by cur column number. Does not affect other
 ***  entries of vec in or outside the range. In particular, vec
 ***  is NOT zeroed within the range unless there is a matrix element
 ***  with value zero at that location.
 ***  All incidence within colrng of the row given is removed from the matrix.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/
extern void mtx_steal_cur_col_vec(mtx_matrix_t mtx, int32 col,
                                  real64 *vec, mtx_range_t *rowrng);
/**<
 ***  The user is expected to supply the vec; we cannot check it.
 ***  Copies the mtx nonzeros currently within rowrng INTO array vec which is
 ***  indexed by cur row number. Does not affect other
 ***  entries of vec in or outside the range. In particular, vec
 ***  is NOT zeroed within the range unless there is a matrix element
 ***  with value zero at that location.
 ***  All incidence within rowrng of the col given is removed from the matrix.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
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

extern boolean mtx_steal_org_row_sparse(mtx_matrix_t mtx, 
                                        int32 row,
                                        mtx_sparse_t *sparse,
                                        mtx_range_t *colrng);
/**<
 ***  Copies the mtx nonzeros currently within colrng to the sparse,
 ***  indexing by org column number. Nonzeros with value 0.0 WILL NOT
 ***  be included in the sparse. sparse->len will be set accordingly.
 ***  All incidence within colrng will be deleted from the mtx.<br><br>
 ***
 ***  The user must supply the sparse; if it is too small, err will be
 ***  TRUE and data will not be collected and incidence not cleared.
 ***  mtx_CREATE_SPARSE is not a valid argument to this function.
 ***  What is too small? For all flavors the sparse must
 ***  have at least the capacity indicated by the col/rowrng.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/
extern boolean mtx_steal_org_col_sparse(mtx_matrix_t mtx,
                                        int32 col,
                                        mtx_sparse_t *sparse,
                                        mtx_range_t *rowrng);
/**<
 ***  Copies the mtx nonzeros currently within rowrng to the sparse,
 ***  indexing by org rowumn number. Nonzeros with value 0.0 WILL NOT
 ***  be included in the sparse. sparse->len will be set accordingly.
 ***  All incidence within rowrng will be deleted from the mtx.<br><br>
 ***
 ***  The user must supply the sparse; if it is too small, err will be
 ***  TRUE and data will not be collected and incidence not cleared.
 ***  mtx_CREATE_SPARSE is not a valid argument to this function.
 ***  What is too small? For all flavors the sparse must
 ***  have at least the capacity indicated by the row/colrng.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/
extern boolean mtx_steal_cur_row_sparse(mtx_matrix_t mtx,
                                        int32 row,
                                        mtx_sparse_t *sparse,
                                        mtx_range_t *colrng);
/**<
 ***  Copies the mtx nonzeros currently within colrng to the sparse,
 ***  indexing by cur column number. Nonzeros with value 0.0 WILL NOT
 ***  be included in the sparse. sparse->len will be set accordingly.
 ***  All incidence within colrng will be deleted from the mtx.<br><br>
 ***
 ***  The user must supply the sparse; if it is too small, err will be
 ***  TRUE and data will not be collected and incidence not cleared.
 ***  mtx_CREATE_SPARSE is not a valid argument to this function.
 ***  What is too small? For all flavors the sparse must
 ***  have at least the capacity indicated by the col/rowrng.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/
extern boolean mtx_steal_cur_col_sparse(mtx_matrix_t mtx,
                                        int32 col,
                                        mtx_sparse_t *sparse,
                                        mtx_range_t *rowrng);
/**<
 ***  Copies the mtx nonzeros currently within rowrng to the sparse,
 ***  indexing by cur rowumn number. Nonzeros with value 0.0 WILL NOT
 ***  be included in the sparse. sparse->len will be set accordingly.
 ***  All incidence within rowrng will be deleted from the mtx.<br><br>
 ***
 ***  The user must supply the sparse; if it is too small, err will be
 ***  TRUE and data will not be collected and incidence not cleared.
 ***  mtx_CREATE_SPARSE is not a valid argument to this function.
 ***  What is too small? For all flavors the sparse must
 ***  have at least the capacity indicated by the row/colrng.<br><br>
 ***
 ***  Notes: It is rather faster to call this with mtx_ALL_COLS/ROWS when
 ***  the row/col of interest is known to have incidence exclusively in
 ***  the range of interest.<br><br>
 ***
 -$-  Fetches nothing from a bad matrix.
 **/

/*
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

extern void mtx_fill_org_row_vec(mtx_matrix_t mtx, int32 row,
                                 real64 *vec, mtx_range_t *colrng);
/**<
 ***  Assumes that the colrng of row in the matrix is empty and
 ***  copies the nonzeros currently within colrng FROM array vec which is
 ***  indexed by org column number INTO the matrix.<br><br>
 ***
 ***  Notes: It is faster to call this with a narrow range than with
 ***  mtx_ALL_COLS/ROWS. It is very marginally faster to call
 ***  mtx_fill_cur_row_vec() instead of this function.<br><br>
 ! !  If you use this when the range is NOT empty, you will sooner or
 ! !  later certainly lose numerical integrity and you may lose memory
 ! !  integrity. The sparse matrix programmer cannot afford to be naive.<br><br>
 ***
 -$-  Puts nothing to a bad matrix.
 **/
extern void mtx_fill_org_col_vec(mtx_matrix_t mtx, int32 col,
                                 real64 *vec, mtx_range_t *rowrng);
/**<
 ***  Assumes that the rowrng of col in the matrix is empty and
 ***  copies the nonzeros currently within rowrng FROM array vec which is
 ***  indexed by org row number INTO the matrix.
 ***
 ***  Notes: It is faster to call this with a narrow range than with
 ***  mtx_ALL_COLS/ROWS. It is very marginally faster to call
 ***  mtx_fill_cur_col_vec() than this function.<br><br>
 ! !  If you use this when the range is NOT empty, you will sooner or
 ! !  later certainly lose numerical integrity and you may lose memory
 ! !  integrity. The sparse matrix programmer cannot afford to be naive.<br><br>
 ***
 -$-  Puts nothing to a bad matrix.
 **/
extern void mtx_fill_cur_row_vec(mtx_matrix_t mtx, int32 row,
                                 real64 *vec, mtx_range_t *colrng);
/**<
 ***  Assumes that the colrng of row in the matrix is empty and
 ***  copies the nonzeros currently within colrng FROM array vec which is
 ***  indexed by cur column number INTO the matrix.<br><br>
 ***
 ***  Notes: It is faster to call this with a narrow range than with
 ***  mtx_ALL_COLS/ROWS. It is very marginally faster to call
 ***  this function instead of mtx_fill_org_row_vec().<br><br>
 ! !  If you use this when the range is NOT empty, you will sooner or
 ! !  later certainly lose numerical integrity and you may lose memory
 ! !  integrity. The sparse matrix programmer cannot afford to be naive.<br><br>
 ***
 -$-  Puts nothing to a bad matrix.
 **/
extern void mtx_fill_cur_col_vec(mtx_matrix_t mtx, int32 col,
                                 real64 *vec, mtx_range_t *rowrng);
/**<
 ***  Assumes that the rowrng of col in the matrix is empty and
 ***  copies the nonzeros currently within rowrng FROM array vec which is
 ***  indexed by cur row number INTO the matrix.
 ***
 ***  Notes: It is faster to call this with a narrow range than with
 ***  mtx_ALL_COLS/ROWS. It is very marginally faster to call
 ***  this functions instead of mtx_fill_org_col_vec().<br><br>
 ! !  If you use this when the range is NOT empty, you will sooner or
 ! !  later certainly lose numerical integrity and you may lose memory
 ! !  integrity. The sparse matrix programmer cannot afford to be naive.<br><br>
 ***
 -$-  Puts nothing to a bad matrix.
 **/
extern void mtx_dropfill_cur_row_vec(mtx_matrix_t mtx, int32 row,
                                 real64 *vec, mtx_range_t *colrng,
                                 real64 tolerance);
/**<
 ***  Assumes that the colrng of row in the matrix is empty and
 ***  copies the values such that abs(value) >tolerance currently
 ***  within colrng FROM array vec which is
 ***  indexed by cur row number INTO the matrix.<br><br>
 ***
 ***  Notes: It is faster to call this with a narrow range than with
 ***  mtx_ALL_COLS/ROWS.
 ! !  If you use this when the range is NOT empty, you will sooner or
 ! !  later certainly lose numerical integrity and you may lose memory
 ! !  integrity. The sparse matrix programmer cannot afford to be naive.<br><br>
 ***
 -$-  Puts nothing to a bad matrix.
 **/
extern void mtx_dropfill_cur_col_vec(mtx_matrix_t mtx, int32 col,
                                 real64 *vec, mtx_range_t *rowrng,
                                 real64 tolerance);
/**<
 ***  Assumes that the rowrng of col in the matrix is empty and
 ***  copies the values such that abs(value) >tolerance currently
 ***  within rowrng FROM array vec which is
 ***  indexed by cur column number INTO the matrix.<br><br>
 ***
 ***  Notes: It is faster to call this with a narrow range than with
 ***  mtx_ALL_COLS/ROWS.
 ! !  If you use this when the range is NOT empty, you will sooner or
 ! !  later certainly lose numerical integrity and you may lose memory
 ! !  integrity. The sparse matrix programmer cannot afford to be naive.<br><br>
 ***
 -$-  Puts nothing to a bad matrix.
 **/

extern void mtx_fill_org_row_sparse(mtx_matrix_t mtx, int32 row,
                                    const mtx_sparse_t *sparse);
/**<
 ***  Fills the current row given with the data in the sparse given.
 ***  The indices in sp->idata are taken as org col indices.
 ***  It is assumed that the row is empty, at least where data is
 ***  being added.<br><br>
 ***
 ***  This function ignores 0.0 in the data and does not create numbers
 ***  there in the mtx.
 **/
extern void mtx_fill_org_col_sparse(mtx_matrix_t mtx, int32 col,
                                    const mtx_sparse_t *sparse);
/**<
 ***  Fills the current col given with the data in the sparse given.
 ***  The indices in sp->idata are taken as org row indices.
 ***  It is assumed that the col is empty, at least where data is
 ***  being added.<br><br>
 ***
 ***  This function ignores 0.0 in the data and does not create numbers
 ***  there in the mtx.
 **/
extern void mtx_fill_cur_row_sparse(mtx_matrix_t mtx, int32 row,
                                    const mtx_sparse_t *sparse);
/**<
 ***  Fills the current row given with the data in the sparse given.
 ***  The indices in sp->idata are taken as cur col indices.
 ***  It is assumed that the row is empty, at least where data is
 ***  being added.<br><br>
 ***
 ***  This function ignores 0.0 in the data and does not create numbers
 ***  there in the mtx.
 **/
extern void mtx_fill_cur_col_sparse(mtx_matrix_t mtx, int32 col,
                                    const mtx_sparse_t *sparse);
/**<
 ***  Fills the current col given with the data in the sparse given.
 ***  The indices in sp->idata are taken as cur row indices.
 ***  It is assumed that the col is empty, at least where data is
 ***  being added.<br><br>
 ***
 ***  This function ignores 0.0 in the data and does not create numbers
 ***  there in the mtx.
 **/


ASC_DLLSPEC void mtx_mult_row(mtx_matrix_t mtx, int32 row,
                         real64 factor, mtx_range_t *colrng);
/**< 
 ***  Multiplies the given row by a given factor.  Only those
 ***  elements with column index within the given range are multiplied.
 ***  This function tests for factor=0.0 and blows away the row if true.
 -$-  Does nothing on a bad matrix.
 **/
ASC_DLLSPEC void mtx_mult_col(mtx_matrix_t mtx, int32 col,
                         real64 factor, mtx_range_t *rowrng);
/**<
 ***  Multiplies the given column by a given factor.  Only those
 ***  elements with row index within the given range are multiplied.
 ***  This function tests for factor=0.0 and blows away the row if true.
 -$-  Does nothing on a bad matrix.
 **/
extern void mtx_mult_row_zero(mtx_matrix_t mtx, int32 row,
                              mtx_range_t *colrng);
/**< 
 ***  Zeros the given row.  Only those
 ***  elements with column index within the given range are zeroed.
 ***  This function zeros without disturbing structure.
 -$-  Does nothing on a bad matrix.
 **/
extern void mtx_mult_col_zero(mtx_matrix_t mtx, int32 col,
                              mtx_range_t *rowrng);
/**< 
 ***  Zeros the given column.  Only those
 ***  elements with row index within the given range are zeroed.
 ***  This function zeros without disturbing structure.
 -$-  Does nothing on a bad matrix.
 **/

extern void mtx_add_row(mtx_matrix_t mtx, int32 srow, int32 trow,
                        real64 factor, mtx_range_t *colrng);
/**<
 ***  Adds a given multiple of row srow to row trow.
 ***  Only those elements with column index within the given range are
 ***  so affected.<br><br>
 -$-  Does nothing on a bad matrix.
 **/
extern void mtx_add_col(mtx_matrix_t mtx, int32 scol, int32 tcol,
                        real64 factor, mtx_range_t *rowrng);
/**<
 ***  Adds a given multiple of column scol to column tcol.
 ***  Only those elements with row index within the given range are
 ***  so affected.<br><br>
 -$-  Does nothing on a bad matrix.
 **/

extern void mtx_add_row_series(int32 srow, real64 factor,
                               mtx_range_t *colrng);
/**<
 ***  Adds the given factor to row srow.
 ***  Only those elements of s with column index within the
 ***  given range are added to factor.
 ***  When possible, range mtx_ALL_COLS/ROWS is faster for sparse rows.
 ! !  Calling this without a prior call to mtx_add_row_series_init() 
 ! !  is an error.<br><br>
 ***
 ! !  Warning: You MUST release any grabbed row/col created on a matrix
 ! !  before destroying that matrix. Failure to do so is fatal.<br><br><br><br>
 ***
 -$-  Does nothing on a bad matrix.
 **/
extern void mtx_add_col_series(int32 scol, real64 factor,
                               mtx_range_t *rowrng);
/**<
 ***  Adds the given factor to column scol.
 ***  Only those elements of s with row index within the
 ***  given range are added to factor.
 ***  When possible, range mtx_ALL_COLS/ROWS is faster for sparse rows.
 ! !  Calling this without a prior call to mtx_add_col_series_init()
 ! !  is an error.<br><br>
 ***
 ! !  Warning: You MUST release any grabbed row/col created on a matrix
 ! !  before destroying that matrix. Failure to do so is fatal.<br><br><br><br>
 ***
 -$-  Does nothing on a bad matrix.
 **/
extern void mtx_add_row_series_init(mtx_matrix_t mtx, int32 trow,
                                    boolean userow);
/**<
 ***  Grab or release elements depending on the value of trow.<br><br>
 ***
 ***  If t>=0, grab a row trow of the matrix to have several rows
 ***  from the same matrix added to it.  userow is ignored for t>=0.
 ***  Only one row at a time may be grabbed for all existing
 ***  matrices. (One row and one col may be grabbed contemporaneously.)
 ***  You must release the row (see below) before you can grab another.<br><br>
 ***
 ***  If t is mtx_NONE, release the last row which was grabbed for multiple adds.
 ***  If userow is TRUE, the current contents of row trow
 ***  (from the previous call) will be used to release the row.
 ***  If userow is FALSE, a pessimistic release method will be
 ***  used instead of the row previously specified.<br><br>
 ! !  If ANY destructive operations have been done on the row, call
 ! !  this with userow==FALSE.
 ! !  The mtx given to a release call must match that given in the grabbing
 ! !  or the release is ignored.
 ***  For very dense rows, it may be faster to call with userow
 ***  == FALSE since the release with TRUE requires a row traversal.<br><br>
 ***
 ! !  Warning: You MUST release any grabbed row created on a matrix
 ! !  before destroying that matrix. Failure to do so is fatal.<br><br>
 ***
 -$-  Does nothing on a bad matrix.
 **/
extern void mtx_add_col_series_init(mtx_matrix_t mtx, int32 tcol,
                                    boolean usecol);
/**<
 ***  Grab or release elements depending on the value of tcol.<br><br>
 ***
 ***  If t>=0, grab a column tcol of the matrix to have several columns
 ***  from the same matrix added to it.  usecol is ignored for t>=0.
 ***  Only one column at a time may be grabbed for all existing
 ***  matrices. (One row and one col may be grabbed contemporaneously.)
 ***  You must release the column (see below) before you can grab another.<br><br>
 ***
 ***  If t is mtx_NONE, release the last column which was grabbed for multiple adds.
 ***  If usecol is TRUE, the current contents of column tcol
 ***  (from the previous call) will be used to release the column.
 ***  If usecol is FALSE, a pessimistic release method will be
 ***  used instead of the column previously specified.<br><br>
 ! !  If ANY destructive operations have been done on the column, call
 ! !  this with usecol==FALSE.
 ! !  The mtx given to a release call must match that given in the grabbing
 ! !  or the release is ignored.
 ***  For very dense columns, it may be faster to call with usecol
 ***  == FALSE since the release with TRUE requires a column traversal.<br><br>
 ***
 ! !  Warning: You MUST release any grabbed column created on a matrix
 ! !  before destroying that matrix. Failure to do so is fatal.<br><br>
 ***
 -$-  Does nothing on a bad matrix.
 **/
/* OLD GROUP COMMENT */
/*
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

/*
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

extern void mtx_old_add_row_sparse(mtx_matrix_t mtx, int32 row,
                                   real64 *drow, real64 factor,
                                   mtx_range_t *colrng, int32 *collist);
/**<
 ***  Adds factor*(drow)[j] to row.
 ***  Drow is assumed to be the size of the matrix and in current
 ***  row order, not original order.<br><br>
 ***
 ***  If colrng is mtx_ALL_COLS, then the following is TRUE:
 ***    If collist is NOT NULL, it will be used instead of colrng
 ***    to determine which columns j of drow are added to row.
 ***    Collist must be an array terminated by -1. The sanity of
 ***    current indices in collist is the users responsibility.
 ***    In particular, in list mode the value of drow is added blindly
 ***    rather than being checked for 0.0 and the range of indices is not
 ***    checked against the matrix size.
 ***    If collist IS NULL the range will be mtx_ALL_COLS.
 ***  If a range is specified, collist is ignored and only those
 ***  elements with column index j within the given range are affected.<br><br>
 ***
 ***  List mode is faster if you are adding the same dense vector
 ***  (differing only by factor) across many (>2) rows/columns.
 ***  Range mode is more convenient. The caller should switch between the
 ***  two as appropriate.<br><br>
 -$-  Does nothing on a bad matrix.
 ***
 ***  @todo The interface of this operator needs some work!
 ***        In particular, it should be redone with consideration given
 ***        to the mtx_sparse_t recently added.
 **/
extern void mtx_old_add_col_sparse(mtx_matrix_t mtx, int32 col,
                                   real64 *dcol, real64 factor,
                                   mtx_range_t *rowrng, int32 *rowlist);
/**<
 ***  Adds factor*(dcol)[j] to col.
 ***  Dcol is assumed to be the size of the matrix and in current
 ***  column order, not original order.<br><br>
 ***
 ***  If rowrng is mtx_ALL_ROWS, then the following is TRUE:
 ***    If rowlist is NOT NULL, it will be used instead of rowrng
 ***    to determine which rows j of dcol are added to col.
 ***    Rowlist must be an array terminated by -1. The sanity of
 ***    current indices in rowlist is the users responsibility.
 ***    In particular, in list mode the value of dcol is added blindly
 ***    rather than being checked for 0.0 and the range of indices is not
 ***    checked against the matrix size.
 ***    If rowlist IS NULL the range will be mtx_ALL_ROWS.
 ***  If a range is specified, rowlist is ignored and only those
 ***  elements with row index j within the given range are affected.<br><br>
 ***
 ***  List mode is faster if you are adding the same dense vector
 ***  (differing only by factor) across many (>2) rows/columns.
 ***  Range mode is more convenient. The caller should switch between the
 ***  two as appropriate.<br><br>
 -$-  Does nothing on a bad matrix.
 ***
 ***  @todo The interface of this operator needs some work!
 ***        In particular, it should be redone with consideration given
 ***        to the mtx_sparse_t recently added.
 **/
/* OLD GROUP COMMENT */
/*
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

extern void mtx_add_row_sparse(mtx_matrix_t mtx, int32 row, 
                               real64 factor, mtx_sparse_t *sparse);
/**< @todo mtx_add_row_sparse() NOT IMPLEMENTED. */
extern void mtx_add_col_sparse(mtx_matrix_t mtx, int32 col,
                               real64 factor, mtx_sparse_t *sparse);
/**< @todo mtx_add_col_sparse() NOT IMPLEMENTED. */

extern size_t mtx_size(mtx_matrix_t mtx);
/**<
 ***  Returns the amount of memory in use by a matrix and all its
 ***  bits and pieces.
 ***  Slaves report only their incremental cost, that is they do
 ***  not report the cost of the structures they share with a master.
 ***  Masters do report the cost of their slaves.
 **/
extern size_t mtx_chattel_size(mtx_matrix_t mtx);
/**<
 ***  Returns the memory used by all slaves of the matrix given
 ***  that is not shared with the master. Returns 0 from a slave.
 **/

extern void mtx_free_reused_mem(void);
/**<
 ***  Deallocates any memory that mtx may be squirrelling away for
 ***  internal reuse. Calling this while any slv_system_t exists
 ***  is likely to be fatal: handle with care.
 **/

/* ********************************************************************* *\
  mtx io routines
\* ********************************************************************* */
extern void mtx_write_sparse(FILE *file, mtx_sparse_t *sparse);
/**<
 ***  Outputs sizes and data of sparse vector to the file given.
 **/

extern void mtx_write_region_human_f(FILE *file,
                                     mtx_matrix_t mtx,
                                     mtx_region_t *region,
                                     int colwise,
                                     int orgwise);
/**<
	Internal function to output a matrix region to file.
	Implementation function for the intended user-callable functions
	  - mtx_write_region_human()
	  - mtx_write_region_human_rows()
	  - mtx_write_region_human_cols()
	  - mtx_write_region_human_orgrows()
	  - mtx_write_region_human_orgcols()

	Outputs permutation and values of the nonzero elements in the
	given region of the mtx to the file given.
	If colwise != 0, output will be column grouped,
	otherwise it will be row grouped.
	If orgwise != 0, only org indices will be printed.
	Doesn't care about master/slave status.
*/

#define mtx_write_region_human(f,m,r) mtx_write_region_human_f((f),(m),(r),0,0)
/**< 
	Grandfather support for the old usages.
	@see mtx_write_region_human_f().
*/
#define mtx_write_region_human_rows(f,m,r) \
mtx_write_region_human_f((f),(m),(r),0,0)
/**< 
	Writes row oriented human readable output of a mtx region.
	@see mtx_write_region_human_f().
*/
#define mtx_write_region_human_cols(f,m,r) \
mtx_write_region_human_f((f),(m),(r),1,0)
/**< 
	Writes column oriented human readable output of a mtx region. 
	@see mtx_write_region_human_f().
*/
#define mtx_write_region_human_orgrows(f,m,r) \
mtx_write_region_human_f((f),(m),(r),0,1)
/**< 
	Writes row oriented human readable output of a mtx region. 
	@see mtx_write_region_human_f(). 
*/
#define mtx_write_region_human_orgcols(f,m,r) \
mtx_write_region_human_f((f),(m),(r),1,1)
/**< 
	Writes column oriented human readable output of a mtx region.  
	@see mtx_write_region_human_f(). 
*/

ASC_DLLSPEC void mtx_write_region(FILE *file, 
                             mtx_matrix_t mtx, 
                             mtx_region_t *region);
/**<
	Outputs permutation and values of the nonzero elements in the
	given region of the mtx to the file given along with the matrix
	size.
	The coordinates of the nonzeros written will be in original
	(unpermuted) indexing. This file is for mtx_read_region, but is
	in ASCII for portability.
	Doesn't care about master/slave status.<br><br>
	Not intended for human consumptions, but just so you know
	permutations are written in order r2org org2r c2org org2c.
	parity is not written.<br><br>
	If the region given is mtx_ENTIRE_MATRIX, and there is a
	block structure present in the matrix, it will be written as well
	and the symbolic rank will go out with the block structure.
*/

extern mtx_matrix_t mtx_read_region(FILE *file, 
                                    mtx_matrix_t mtx, 
                                    int transposed);
/**<
 ***  Reads a matrix region.
 ***  From the file pointer, does the following:
 ***   - Gets the matrix order of the data in the file.
 ***   - If mtx is NULL, creates it.
 ***   - Expands the matrix given or created to the data order if it is too small.
 ***   - Permutes the matrix to the permutation found in the file.
 ***   - Reads the region limits in the file and clears that region in the mtx.
 ***   - Reads coefficients from the file.
 ***
 ***  transposed governs if the matrix data should be treated as
 ***  row col 	or 	col row
 ! !  Warnings:   Do not add incidence data to the
 ! !  file except in those org_rows and org_cols which are within the
 ! !  region specified (note that the region is given in CUR coordinates.)
 ! !  Adding incidence outside the region may lead to loss of internal
 ! !  consistency unless the entire mtx given is empty when this is called.
 ! !  If you send a matrix larger than the order of data in the file,
 ! !  you should do a reset to it first to undo the previous permutation.<br><br>
 ***
 ***  Return value is the pointer to the mtx given or created.
 ***  If error in reading matrix file, mtx returned may be incomplete or NULL
 ***  if the mtx given was NULL.
 ***  If the file read has block information in it and further the user sent
 ***  in NULL to this function, block info will be read and added to the
 ***  returned matrix.<br><br>
 ***
 ***  Doesn't care about master/slave status.
 **/

ASC_DLLSPEC void mtx_write_region_matlab(FILE *file, 
                                    mtx_matrix_t mtx, 
                                    mtx_region_t *region);
/**<
	Outputs values of the nonzero elements in the
	given region of the mtx to the file in matlab/harwell sparse format.
	The "a=[\n" and trailing "];\n" are not supplied, since the caller
	knows better what they should look like.
	Row/column coordinates printed are the cur coordinates.
*/

ASC_DLLSPEC int mtx_write_region_mmio(FILE *file
	,mtx_matrix_t mtx, mtx_region_t *region);
/**<
	Outputs values of the nonzero elements in the
	given region of the mtx to the file in Matrix Market sparse format
	Row/column coordinates printed are the 'cur' coordinates.

	@param region Set to mtx_ENTIRE_MATRIX for the entire matrix to be output
	@param file Could be stderr, stdout, or a file pointer that you have opened
	@return 0 on success
	@see ASC_WITH_MMIO
*/

ASC_DLLSPEC void mtx_write_region_plot(FILE *file, 
		mtx_matrix_t mtx, mtx_region_t *region);
/**<
	Outputs the coordinates of elements to file with format suitable
	for xgraph consumption from the given region of the mtx.
	A suitable xgraph invocation would be "xgraph -nl -m filename".
	Doesn't care about master/slave status.
*/

ASC_DLLSPEC void mtx_write_region_csr(FILE *file,
		mtx_matrix_t mtx, mtx_region_t *region, int offset);
/**<
	This function writes out the matrix in compressed row format.
	There may be a _ccs version (column based) one day.
	Doesn't care about master/slave status.
*/

ASC_DLLSPEC void mtx_write_region_smms(FILE *file,
		mtx_matrix_t mtx, mtx_region_t *region,	int offset);
/**<
	The _smms version writes out the matrix in a form digestible by
	Alvarado's Sparse Matrix Manipulation System.
	Doesn't care about master/slave status.

	Writes the given region of the matrix to the named file.
	Will write the entire matrix if the region is mtx_ENTIRE_MATRIX.
	offset controls whether fortran (1) or c style indexing is done.

	@NOTE I contacted Alvarado about his code and he's trying to dig it up.
	At present it's not available anywhere.

	@see http://citeseer.ist.psu.edu/366279.html
*/

extern mtx_matrix_t mtx_read_smms(FILE *file,
		mtx_matrix_t mtx, int transpose);
/**<
	Reads a matrix in smms format. If a NULL matrix
	is sent in, it will create and return it. If a non NULL matrix
	is given, the order will be increased if necessary. The contents
	of the old mtx, will be blown away. The transpose flag dictates
	whether the transpose should be read in.
	Doesn't care about master/slave status.
*/

extern void mtx_exception_recover(void);
/**<
	Cleans up after a floating point exception has
	occurred during matrix manipulations.  You don't need to know 
	what this does, except that you should call it any time this may
	have occurred.  Several functions use
	data structures that can cause insanity in the event of exception.<br><br>
	
	Just for the curious, it resets several internal data structures
	needed including ones used in the operators:
	  - mtx_add_row/col()
  - mtx_add_row/col_series()
  - mtx_add_row/col_series_init()
  - mtx_assemble()
  - mtx_add_outer_product()
*/

ASC_DLLSPEC void mtx__debug_output(FILE *file, mtx_matrix_t mtx);
/**<
	Debug-outputs all internal information about a matrix to file.
	In the process, integrity checks are performed.
	If file is NULL, output goes to default (which is stderr.)
	Doesn't care about master/slave status.
*/

/** @} */

#endif /**< ASC_MTX_BASIC_H */
