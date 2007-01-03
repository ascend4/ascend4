/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1996 Benjamin Andrew Allan

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
	Sparse matrix permutation functions

	Requires:
	#include "utilities/ascConfig.h"
	#include "mtx.h"
*//*
	by Benjamin Andrew Allan
	derived from mtx by Karl Michael Westerberg
	Created: 5/3/90
	Last in CVS: $Revision: 1.10 $ $Date: 1998/05/06 17:28:54 $ $Author: ballan $
*/
 
#ifndef ASC_MTX_PERMS_H
#define ASC_MTX_PERMS_H

/** @addtogroup linear
 * @{
 */

/* the following block_perm functions are not yet implemented: 
 *   this is the software spec. 5/3/95 baa. */

extern mtx_block_perm_t mtx_create_block_perm(mtx_matrix_t mtx);
/**< 
 ***  Returns a token with the permutation/block information of the 
 ***  mtx given. The mtx given must be previously output assigned and, 
 ***  if it is to be partitioned, should already be partitioned.
 ***  The permutation returned can be used subsequently in various
 ***  ways, but all operations must be on the mtx the data came from.
 ***  If the matrix is reoutput assigned or repartitioned, the data
 ***  in bp becomes (potentially) invalid and should be completely
 ***  updated with mtx_update_block_perm.
 ***  Returns NULL if a bad mtx is detected.<br><br>
 ***
 ***  Passes calls on slave matrices up to the master matrix.<br><br>
 ***
 -$-  Checks extra carefully for a bad matrix, and returns NULL if so.
 **/

extern int mtx_update_block_perm(mtx_matrix_t mtx,
                                 int32 bnum, 
                                 mtx_block_perm_t bperm);
/**<
 ***  Given an mtx, a block number, and an existing bperm, this
 ***  routine updates the bperm permutation information about the
 ***  block bnum in the mtx. 
 ***  The bperm updated must come from the mtx the bperm was created
 ***  for. The mtx must be output assigned and, if the mtx was
 ***  partitioned, the partition data must be consistent between the
 ***  bperm and the mtx. The mtx cannot have been resized.<br><br>
 ***
 ***  Exceptions: If bnum is mtx_ALL_BLOCKS, we check only for the
 ***  mtx identity and output assignment. All previous permutation and
 ***  block information is ignored and replace by current info.
 ***  Calling with mtx_ALL_BLOCKS is substantially more expensive
 ***  than calling with a specific block number unless the block is
 ***  nearly the size of the problem.<br><br>
 ***
 ***  Passes calls on slave matrices up to the master matrix.<br><br>
 ***
 ***  Returns 1 from a bad matrix, 2 from a bad bperm, 3 from a mismatch.
 ***  Returns 0 from a successful call.<br><br>
 ***
 -$-  Does excessive result checking, and then returns 3 if the
 -$-  excessive checks fail.
 **/

extern int mtx_restore_block_perm(mtx_matrix_t mtx, 
                                  int32 bnum,
                                  mtx_block_perm_t bperm);
/**<
 ***  Given an mtx, a block number, and an existing bperm, this
 ***  routine updates the mtx permutation information for the
 ***  block bnum using the bperm. 
 ***  The mtx updated must go with the bperm.
 ***  The mtx must be output assigned and, if the mtx was
 ***  partitioned, the partition data must be consistent between the
 ***  bperm and the mtx. The mtx cannot have been resized.
 ***  The parity of the mtx will be set to that stored with the bperm;
 ***  if you try to do sneaky things, parity may get out of whack.<br><br>
 ***
 ***  Exceptions: If bnum is mtx_ALL_BLOCKS, we check only for the
 ***  mtx identity and order. All previous permutation and
 ***  block information is ignored and replaced by bperm info.
 ***  The mtx's output_assigned and partition characteristics will
 ***  be restored to their values at the time the bperm was last
 ***  created or updated.<br><br>
 ***  Calling with mtx_ALL_BLOCKS is substantially more expensive
 ***  than calling with a specific block number if all you really
 ***  want updated is a specific block.<br><br>
 ***
 ***  Passes calls on slave matrices up to the master matrix.<br><br>
 ***
 ***  Returns 1 from a bad matrix, 2 from a bad bperm, 3 from a mismatch.
 ***  Returns 0 from a successful call.<br><br>
 ***
 -$-  Does excessive checking, and then returns usual values if the
 -$-  corresponding excessive checks fail.
 **/

extern int mtx_destroy_block_perm(mtx_block_perm_t bperm);
/**< 
 ***  Deallocates all memory associated with the bperm.
 ***  Has nothing to do with the matrix of bperm's origin.
 ***  Returns 1 if anything untoward happens during the
 ***  destruction. Returns 0 otherwise.
 **/

extern size_t mtx_block_perm_size(mtx_block_perm_t bperm);
/**< 
 ***  One for the bean counters. Returns current memory used by
 ***  the mtx_block_perm_t. Bytes as usual.
 **/

/* end block_perm functions */

/*------------------------------------------------------------------------------
  MTX PERMUTATION AND PERMUTATION INFO ROUTINES
*/

extern void mtx_swap_rows(mtx_matrix_t mtx, int32 row1, int32 row2);
/**<
 ***  Swaps two rows of the matrix.  The association between the
 ***  "original row number" and the row contents is not
 ***  changed, so that some record is kept as to where a given row
 ***  originally came from (see mtx_org_to_row(), etc.).
 ***
 ***  Passes calls on slave matrices up to the master matrix.
 -$-  Does nothing to a bad matrix.
 **/
ASC_DLLSPEC(void ) mtx_swap_cols(mtx_matrix_t mtx, int32 col1, int32 col2);
/**<
 ***  Swaps two columns of the matrix.  The association between the
 ***  "original column number" and the column contents is not
 ***  changed, so that some record is kept as to where a given column
 ***  originally came from (see mtx_org_to_row(), etc.).
 ***
 ***  Passes calls on slave matrices up to the master matrix.
 -$-  Does nothing to a bad matrix.
 **/
extern void mtx_drag(mtx_matrix_t mtx, int32 d1, int32 d2);
/**<
 ***  Drag shifts diagonal element d1 to position d2, or vice versa,
 ***  rotating all the intermediate elements as if d1 were swapped
 ***  (row and col) with all the intermediate di.<br><br>
 ***  Drag exists because it is twice the speed of the following
 ***  implementation outside of mtx:           <pre>
 ***     while( n1 < n2 ) {
 ***        mtx_swap_rows(mtx,n1,n1+1);
 ***        mtx_swap_cols(mtx,n1,n1+1);
 ***        ++n1;
 ***     }
 ***      while( n1 > n2 ) {
 ***        mtx_swap_rows(mtx,n1,n1-1);
 ***        mtx_swap_cols(mtx,n1,n1-1);
 ***        --n1;
 ***     }                                     </pre>
 ***  If it turns out that a cycle_col or cycle_row (independent of diagonal)
 ***  is wanted, implementation is now trivial.
 ***
 ***  Passes calls on slave matrices up to the master matrix.
 -$-  Does nothing to a bad matrix.
 **/
extern void mtx_reverse_diagonal(mtx_matrix_t mtx, int32 d1, int32 d2);
/**<
 ***  Does a series of symmetric permutations that reverses
 ***  the ordering of the diagonal between (and including) d1 & d2.
 ***  If d2 < d1, does nothing.
 ***
 ***  Passes calls on slave matrices up to the master matrix.
 -$-  Does nothing to a bad matrix.
 **/


ASC_DLLSPEC(int32  ) mtx_row_to_org(mtx_matrix_t mtx, int32 row);
/**<
	Converts original row number <--> row number.
	Passes calls on slave matrices up to the master matrix.
	
	@return original row number, or -1 from a bad matrix.
*/

ASC_DLLSPEC(int32  ) mtx_col_to_org(mtx_matrix_t mtx, int32 col);
/**<
	Converts  original column number <--> column number.
	Passes calls on slave matrices up to the master matrix.

	@return original column numner, or -1 from a bad matrix.
*/

ASC_DLLSPEC(int32  ) mtx_org_to_row(mtx_matrix_t mtx, int32 orgrow);
/**<
	Converts original row number <--> row number.
	Passes calls on slave matrices up to the master matrix.

	@return row number, or -1 from a bad matrix.
*/

ASC_DLLSPEC(int32  ) mtx_org_to_col(mtx_matrix_t mtx, int32 orgcol);
/**<
	Converts  original column number <--> column number.
	Passes calls on slave matrices up to the master matrix.

	@return column number, or -1 in case of a bad matrix.
*/


extern boolean mtx_row_parity(mtx_matrix_t mtx);
/**<
 ***  Returns the parity (even=FALSE,odd=TRUE) of the permutation which
 ***  carries original row to current row numbers.
 ***  Passes calls on slave matrices up to the master matrix.
 -$-  Returns -1 from a bad matrix.
 **/
extern boolean mtx_col_parity(mtx_matrix_t mtx);
/**<
 ***  Returns the parity (even=FALSE,odd=TRUE) of the permutation which
 ***  carries original column to current column numbers.
 ***  Passes calls on slave matrices up to the master matrix.
 -$-  Returns -1 from a bad matrix.
 **/

/*------------------------------------------------------------------------------
  MTX STRUCTURAL MANIPULATION AND INFO ROUTINES
*/

extern int mtx_output_assign_region(mtx_matrix_t mtx, 
                                    mtx_region_t *region,
                                    int *orphaned_rows);
/**<
 ***  This function is very similar to its sister function mtx_output_assign.
 ***  It reorders the matrix to put as many non-zeros on the diagonal as
 ***  possible. It mtx_ENTIRE_MATRIX is sent in as the region, the output
 ***  assignment will affect the entire matrix. Otherwise the output
 ***  assignment will be restricted to the designated region.
 ***  This function returns the symbolic rank of the matrix. If a row can
 ***  not be assigned it is moved to the end of lower edge of the matrix.
 ***  The count of the unassigned aka orphaned rows is returned also.
 ***  Unlike mtx_output_assign nothing else is done to the matrix.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 ***
 -$-  Does nothing to a bad matrix.
 **/

extern void	mtx_output_assign(mtx_matrix_t mtx, int32 hirow, int32 hicol);
/**<
 ***  Reorders the matrix to put as many non-zeros on the diagonal as
 ***  possible.  This function does not assume the validity of a previous
 ***  assignment.  mtx_symbolic_rank() may be subsequently called to produce
 ***  the symbolic rank.  All assigned rows and columns go into forming
 ***  a single initial block which is of dimension equal to the symbolic
 ***  rank of the matrix.<br><br>
 ***
 ***  All unassigned rows and columns with incidence are then placed
 ***  against the borders of the nonsingular region while all empty rows
 ***  and columns are moved toward the outer borders of the matrix.
 ***  If hirow and hicol are not improperly small, we will guarantee that all
 ***  original rows r, 0 <= r < hirow, and original columns c,
 ***  0 <= c < hicol, are placed next to the initial block. That is,
 ***  if that block is contained within the block ((0,0),(hirow,hicol)).
 ***  This is not a particularly interesting property mathematically, but
 ***  it makes things loads easier when writing interfaces that involve
 ***  mtx objects; the first hirow rows and hicol cols of the jacobian will
 ***  correspond to some relation or variable even if they haven't incidence.
 -$-  Does nothing to a bad matrix.<br><br>
 ***  NOTE:
 ***  When used on an incidence matrix, the fixed vars and unincluded
 ***  relations which don't generate elements may end up anywhere between
 ***  the initial block and the ends of the matrix if hirow and hicol are
 ***  not specified correctly.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 ***
 ***  Note that mtx_output_assign invalidates any data saved
 ***  with the mtx_*_block_perm functions.
 **/

ASC_DLLSPEC(boolean ) mtx_output_assigned(mtx_matrix_t mtx);
/**< 
 ***  Determines if the matrix has been previously output assigned.
 ***  Calls on slaves are passed up to the master matrix.
 ***
 -$-  Returns FALSE on a bad matrix.
 **/

ASC_DLLSPEC(int32  ) mtx_symbolic_rank(mtx_matrix_t mtx);
/**< 
 ***  Returns the symbolic rank determined by a previous call to
 ***  mtx_output_assign().
 ***  Calls on slaves are passed up to the master matrix.
 ***
 -$-  Returns -2 on bad matrix or -1 on unassigned one.
 **/
     
extern void mtx_set_symbolic_rank(mtx_matrix_t mtx, int32 rank);
/**< 
 ***  Sets symbolic rank of mtx.  This is used in a hack
 ***  and is not intended for general use.
 **/
     
extern boolean mtx_make_col_independent(mtx_matrix_t mtx,
                                        int32 col,
                                        mtx_range_t *rng);
/**< 
 ***  Removes col from the basis in place of one of the columns in rng.
 ***  I.e. redoes the mtx_output_assign so as to not use col, if
 ***  possible, using one of the cols in rng instead.
 ***  Clears any prior partitioning data.
 ***  Note that mtx_make_col_independent invalidates any data saved
 ***  with the mtx_*_block_perm functions.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/

extern void mtx_org_permute(mtx_matrix_t mtx, mtx_region_t * region);
/**< 
 ***  Given a region, repermutes rows and columns within it to the ordering
 ***  where mtx_row_to_org(mtx,i) < mtx_row_to_org(mtx,i+1) for all i and
 ***  where mtx_col_to_org(mtx,j) < mtx_row_to_org(mtx,j+1) for all j
 ***  within the region. At present this is implemented as a qsort and
 ***  may be a little pricey.<br><br>
 ***
 ***  mtx_org_permute(mtx,mtx_ENTIRE_MATRIX); is equivalent to
 ***  mtx_reset_perm(mtx);
 **/

extern int32 mtx_full_diagonal(mtx_matrix_t mtx, mtx_range_t *rng, int noisy);
/**< 
 ***  This function checks the diagonal for holes. If symbolic_rank is
 ***  set, and rng->high < rank, returns immediately.
 ***  Returns number of holes detected in diagonal in rng given.
 ***  If noisy != 0, writes the hole locations to g_mtxerr.
 ***  On detectably bad input, returns -1.
 ***  Worst Cost: O(nnz)  where nnz = incidence count sum over rows in rng.
 ***  Usual Cost: O(n) where n = a small factor *(rng->high - rng->low +1)
 ***  Does not pass calls up to master.
 **/

extern int32 mtx_transpose(mtx_matrix_t mtx);
/**< 
 *** Transposes everything about the matrix. The user is
 *** responsible for keeping track of the change in the semantics
 *** this implies if the matrix is being used in a nonlinear context.<br><br>
 ***
 *** Transposing a matrix also transposes all associated slave/master
 *** matrices.
 *** To check if a matrix has been transposed, use mtx_isa_transpose.<br><br>
 *** Cost O(Nnz).
 *** 0 order matrices cannot be transposed.
 -$- Returns mtx_NONE on a bad or 0 order matrix.
 **/

extern int32 mtx_isa_transpose(mtx_matrix_t mtx);
/**< 
 *** Returns 1 if the matrix is transposed from another and 0 if not.
 *** Calling mtx_transpose twice yields a mtx which responds with 0.
 -$- Returns mtx_NONE on a bad or 0 order matrix.
 **/

/*------------------------------------------------------------------------------
  START OF SOME NEW BLOCK MATRIX ROUTINES.

  At the moment the block structure is intimately tied up with the matrix.
*/

extern mtx_block_t *mtx_block_partition(mtx_matrix_t mtx, mtx_region_t *reg);
/**< 
 ***  Partitions the single block of a previously output assigned matrix
 ***  into smaller blocks if possible. This function unlike its sister
 ***  mtx_partition, takes in, and returns information explicitly, rather
 ***  than assuming it to be a property of the matrix. The result will
 ***  be NULL, if there are no blocks. At the moment works only on
 ***  a square matrix. The range reg->row is assumed to represent the
 ***  extreme points of a square region. The caller owns the result and 
 ***  must be deallocate it.
 ***  If the matrix given (or its master) is not assigned, this will
 ***  verify (at some expense) that the diagonal is full and whine
 ***  if not. It will attempt to partition anyway assuming a diagonal.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/


/*------------------------------------------------------------------------------
  THE ORIGINAL BLOCK MANIPULATION ROUTINES.
*/

extern void mtx_partition(mtx_matrix_t mtx);
/**< 
 ***  Takes an output assigned matrix and does a permutation to 
 ***  block-lower-triangular form of the square region from
 ***  0,0 to symbolic_rank-1, symbolic_rank-1.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/

extern void mtx_ut_partition(mtx_matrix_t mtx);
/**< 
 ***  Takes an output assigned matrix and does a permutation to 
 ***  block-UPPER-triangular form of the square region from
 ***  0,0 to symbolic_rank-1, symbolic_rank-1.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/

extern boolean mtx_check_blocks(mtx_matrix_t mtx);
/**< 
 ***  Checks whether or not the block data are consistent with the
 ***  non-zero structure of the matrix.  If not, the block data
 ***  are destroyed. Matrix must be previously output assigned,
 ***  if not (or bad matrix) nothing is done.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/

extern int32 mtx_number_of_blocks(mtx_matrix_t mtx);
/**< 
 ***  Returns the number of blocks in the matrix.  Matrix must be
 ***  previously output assigned.
 ***
 ***  Calls on slaves are passed up to the master matrix.
 ***
 -$-  Returns mtx_NONE if matrix is bad which is liable to lead to core
 -$-  dumping if the user is not checking for a bad return.
 **/

extern int32 mtx_block(mtx_matrix_t mtx, 
                       int32 block_number, 
                       mtx_region_t *block);
/**<
 ***  Sets block to the "block_number"-th block (indexed 0 to nblocks-1) in
 ***  the matrix.  Matrix must be previously output assigned.
 ***  Returns mtx_NONE if not previously output assigned 
 ***  or if requested block does not exist, or if no blocks exist,
 ***  otherwise returns 0. 
 ***  If mtx_NONE is returned, block will contain the region
 ***  ((0,order-1),(0,order-1)) where order is the mtx_order.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/

ASC_DLLSPEC(int32 ) mtx_block_containing_row(mtx_matrix_t mtx, 
                                      int32 row,
                                      mtx_region_t *block);
/**<
 ***  Sets block to the block which contains the given row and
 ***  returns the block number.  If the given row doesn't belong
 ***  to any block, mtx_NONE is returned. Matrix must be previously output
 ***  assigned or a 0 is returned.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/
extern int32 mtx_block_containing_col(mtx_matrix_t mtx,
                                      int32 col,
                                      mtx_region_t *block);
/**<
 ***  Sets block to the block which contains the given column and
 ***  returns the block number.  If the given column doesn't belong
 ***  to any block, mtx_NONE is returned. Matrix must be previously output
 ***  assigned or a 0 is returned.<br><br>
 ***
 ***  Calls on slaves are passed up to the master matrix.
 **/

/** @} */

#endif /* ASC_MTX_PERMS_H */
