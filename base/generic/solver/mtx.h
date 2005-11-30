/*
 *  mtx: Ascend Sparse Matrix Package
 *  by Benjamin Andrew Allan
 *  Derived from mtx by Karl Michael Westerberg
 *  Created: 5/3/90
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: mtx.h,v $
 *  Date last modified: $Date: 1998/07/05 20:46:27 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Benjamin Andrew Allan, Kirk Andre' Abbott
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

/** @file
 *  mtx: Ascend Sparse Matrix Package.
 *  <pre>
 *  Contents:     Matrix modules module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *                Ben Allan
 *
 *  Dates:    KW  06/90 - original version
 *            JZ  01/94 - added output assignment and partitioning
 *                        operations allowing them to be performed
 *                        individually.
 *            JZ  08/94 - reduced the internal practice of repeatedly
 *                        finding the existence of an element by re-
 *                        writing mtx_next_in_row and mtx_next_in_col
 *                        to use a static element pointer which can
 *                        advance and be rewound.  Dramatic cpu time
 *                        savings were obtained by having the values
 *                        of subsequent elements returned by these
 *                        functions to eliminate the need for calling
 *                        mtx_value on the obtained coordinate.  A new
 *                        function mtx_add_value was added to replace
 *                        mtx_set_value in applications where it is
 *                        known by the user that no nonzero currently
 *                        exists at the coordinate whose value is being
 *                        set.
 *            BA  08/94 - Protoized header file to reduce coding errors.
 *            BA  11/94 - Added mtx_write_region as it is done elsewhere
 *                        and seems a common lowlevel operation.
 *                        Made error output file specifiable in __debug.
 *                        Made functions explicitly void if needed.
 *                        For the following, see headers for explanation.
 *                        Added mtx_copy_complete and mtx_copy_wo_incidence.
 *                        Added mtx_drag from JZ.
 *                        Added mtx_write_region_plot from JZ for xgraphs.
 *                        Added mtx_free_reused_mem.
 *                        Added mtx_add_row_sparse and mtx_add_col_sparse.
 *                        Added mtx_nonzeros_in_region.
 *                        Added mtx_numbers_in_row.
 *                        Added mtx_numbers_in_col.
 *                        Added mtx_numbers_in_region.
 *                        Annotated next_in operator descriptions.
 *                        Renamed mtx_add_value to mtx_fill_value in
 *                        anticipation of a set of fill commands and to
 *                        keep the mtx_add namespace implying math only.
 *            BA  12/94 - Added a set of dense vector (double array)
 *                        handling routines:
 *                        mtx_org_row_vec \
 *                        mtx_org_col_vec | stuff an array which is cur or
 *                        mtx_cur_row_vec | org indexed from a row/col.
 *                        mtx_cur_col_vec /
 *                        mtx_set_org_row_vec \ UNIMPLEMENTED
 *                        mtx_set_org_col_vec | set a mtx row/col from
 *                        mtx_set_cur_row_vec | cur or org indexed array.
 *                        mtx_set_cur_col_vec /
 *                        mtx_fill_org_row_vec \
 *                        mtx_fill_org_col_vec | set empty mtx row/col from
 *                        mtx_fill_cur_row_vec | cur or org indexed array.
 *                        mtx_fill_cur_col_vec /
 *                        mtx_zr_org_vec_using_row \
 *                        mtx_zr_org_vec_using_col | zero org/cur vec entries
 *                        mtx_zr_cur_vec_using_row | matching row/col incid.
 *                        mtx_zr_cur_vec_using_col /
 *                        mtx_org_vec_add_row \
 *                        mtx_org_vec_add_col | vec += mtx row/col is a
 *                        mtx_cur_vec_add_row | cur/org indexed array.
 *                        mtx_cur_vec_add_col /
 *                        mtx_add_row_org_vec \ UNIMPLEMENTED
 *                        mtx_add_col_org_vec | mtx row/col += vec. vec a
 *                        mtx_add_row_cur_vec | cur/org indexed array.
 *                        mtx_add_col_cur_vec /
 *                        Added mtx_add_row/col_series for use in
 *                        common elimination schemes.
 *                        Added dot operators for dot products
 *                        mtx_row_dot_full_org_vec \
 *                        mtx_col_dot_full_org_vec | row/col dot vec indexed
 *                        mtx_row_dot_full_cur_vec | by org or cur row or col
 *                        mtx_col_dot_full_cur_vec /
 *
 *                        Declared a sparse vector type that can be mapped
 *                        directly to compressed row or column storage
 *                        called mtx_sparse_t.
 *
 *                        Added mtx_write_region_human.
 *                        Added mtx_read_region which is inverse of
 *                        mtx_write_region.
 *                        Revised mtx_row_max, mtx_col_max to return
 *                        signed value of element with max abs(value).
 *                        Consolidated mtx_copy* functions into one with
 *                        a set of macros for the various flavors.
 *
 *               6/95 ba  Added mtx_org_permute for 'natural' orderings.
 *                        Added mtx_reverse_diagonal which does that.
 *
 *               8/95 ba  Added slave matrices and attendant functions.
 *                        mtx_create_slave       adds a slave.
 *                        mtx_chattel_size       counts slave memory.
 *
 *                        Added map back functions with a drop tolerance.
 *                        mtx_dropfill_cur_row_vec \ cur indexed vector
 *                        mtx_dropfill_cur_col_vec / filled in mtx if big.
 *                        Added a coordinate list struct for passing lists
 *                        of elements around.
 *
 *              10/95 ba  Added sparse vector fetch routines and destroy.
 *                        mtx_org_row_sparse \
 *                        mtx_org_col_sparse | stuff a sparse which is cur or
 *                        mtx_cur_row_sparse | org indexed from a row/col.
 *                        mtx_cur_col_sparse /
 *                        mtx_destroy_sparse >  deallocate a sparse.
 *              11/95 ba  Added spec, but not code, for mtx_clear_rowlist
 *                        in anticipation of nonlinear solvers recomputing
 *                        the gradient of only the nonlinear rows.
 *                        Added also:
 *                        mtx_assemble, which does just that in much
 *                                      the finite element sense.
 *                        mtx_fill_org_value which takes org coord.
 *                        mtx_exception_recover  which resets internals
 *                          after floating point err has left things in
 *                          an illdefined state. Also in this process
 *                          unified several pieces of code that manage
 *                          internal buffers.
 *
 *              12/95 ba  mtx_steal_org_row_sparse \ clear incidence and
 *                        mtx_steal_org_col_sparse | stuff a sparse which is
 *                        mtx_steal_cur_row_sparse | cur or org indexed from
 *                        mtx_steal_cur_col_sparse / a row/col.
 *                        mtx_steal_org_row_vec \ clear incidence and
 *                        mtx_steal_org_col_vec | stuff an array which is
 *                        mtx_steal_cur_row_vec | cur or org indexed from
 *                        mtx_steal_cur_col_vec / a row/col.
 *                        mtx_get_pivot_col \  LU partial pivot selection
 *                        mtx_get_pivot_row /  with a sparsity tolerance.
 *               1/96 ba  mtx_fill_org_row_sparse \
 *                        mtx_fill_org_col_sparse | set empty row/col from
 *                        mtx_fill_cur_row_sparse | cur or org indexed sparse.
 *                        mtx_fill_cur_col_sparse /
 *                        mtx_write_region_human     \ Revised write human
 *                        mtx_write_region_human_rows| so we can have row
 *                        mtx_write_region_human_cols| or col oriented print.
 *                        mtx_write_region_human_f   / Supports old usage.
 *
 *               5/96 ba  Split mtx.[ch] into several files to make life
 *                        much easier for maintaining. All the mtx_*.c
 *                        files are intended to stand together as a
 *                        sparse matrix package. The split is for header
 *                        digestibility and for ease of maintenance.
 *                        The decomposition is as follows:
 *
 *                        mtx.h.
 *
 *                        This file, which includes the rest.
 *                        This describes general mtx concepts.
 *                        There is no mtx.c.
 *
 *                        mtx_basic.c mtx_basic.h Most memory management.
 *                        mtx_perms.c mtx_perms.h Permuation management.
 *                        mtx_query.c mtx_query.h Most queries, readonly math.
 *                        mtx_linal.c mtx_linal.h Unjustifiable pieces.
 *                        mtx_internal_use_only.[ch] Nobody's business.
 *
 *               4/97 ba  Added mtx_transpose, mtx_isa_transpose to perms.
 *                        Added mtx_duplicate_region to make a slave copy.
 *
 *               7/98 ba  Added mtx_write_region_matlab for harwell format.
 *
 *
 *  Description:  This module allows the user to create and manipulate
 *                matrices.  The following is list of what constitutes
 *                a "matrix":
 *
 *                   - A square nxn (where n is the order of the matrix)
 *                     rectangular region of numbers, indexed by
 *                     (row,col) pairs where 0<=row<n and 0<=col<n.
 *
 *                   - Row and column permutations which keep track of
 *                     where a given row/column "came from" originally.
 *
 *                The coefficient matrix (i.e. the nxn region of numbers)
 *                can be divided into two classes, non-zeros and zeros.
 *                Roughly speaking, a given element is a non-zero so long
 *                as its value has the POTENTIAL of not equaling zero.
 *                Thus, if the value of an element (r,c) is not equal to
 *                zero, then (r,c) must be classified as a non-zero:
 *                however the converse need not hold, unless either
 *                mtx_del_zr_in_row or mtx_del_zr_in_col has been
 *                recently called with r or c respectively.
 *
 *                The mtx_matrix_t includes a data structure for block
 *                information storage, a block being a matrix subregion.
 *                This feature supports partitioning solvers.
 *
 *                There are only 3 fundamental operations _on_ matrices
 *                  vector scaling  (mtx_mult_*)
 *                  vector addition (mtx_add_*)
 *                  permutation     (mtx_swap_*)
 *                Matrix elements are maintained as
 *                relatively unordered linked lists. Mtx_next_in_* is
 *                most generally useful operator for doing sparse math
 *                with this matrix package. There are several operations
 *                that take matrices/vectors and return vectors/scalars.
 *                Sparse matrix-matrix computations are best coded with
 *                vector primitives and knowledge of the specific matrix,
 *                so they are not provided here.
 *
 *                It's amazing for a package that only addresses the
 *                fundamentals, this still has over 100 calls and looks
 *                like an extremely well documented kitchen sink.
 *
 *                The user may grep on extern to get a semiformatted list
 *                of the operator names in this file. Please follow the
 *                formatting if you add functions to this module.
 *                i.e.  grep extern mtx2*h  cooks up a reduced header.
 **
 ** header conventions: (DON'T SKIP READING THIS SECTION!!)
 **   -$- in place of *** implies as described in compilation flags below.
 **   ! ! in place of *** implies a warning which should be read before
 **   ! ! using the function under discussion in certain ways.
 **
 ** compilation flags:
 **   MTX_DEBUG  - if TRUE, check_matrix will be invoked all over the place
 **                and -$- returns will be in effect.
 **                check_sparse is also used where appropriate if TRUE.
 **              - if FALSE, routines marked with -$- will assume that you
 **                are sending in a good mtx_matrix_t. Check accordingly.
 **                They will also assume any mtx_sparse_t * is good.
 **   MTX_DEBUG is wired in file mtx.c, but shouldn't be.
 **   MTX_DEBUG TRUE slows this code down incredibly and should not be
 **   used when compiling production code.
 **
 ** vocabulary for this header file.
 **   int32   a 32 bit signed integer (int usually, long on some archs.)
 **   real64  a 64 bit real (double on most systems.)
 **                 By using these typedefs, all the code remains intact
 **                 across different systems/compilers: only the typedefs
 **                 have to change if there is a difference in data size.
 **
 **   mtx, matrix:  a peculiar sparse matrix structure which should not
 **                 be manipulated except by means of functions in this
 **                 header. Attempts to go around by another path are
 **                 99.44% likely to fail, and 95% likely to fail
 **                 catastrophically.
 **
 **   cur_row,
 **   cur_col, or
 **   row,col:      an int32 denoting the ith(jth) row(column) as the
 **                 matrix is currently permuted (range 0..order-1).
 **   org_row, org_col:
 **                 an int32 denoting the ith(jth) row(col) as the matrix
 **                 is ordered in the unpermuted state (range 0..order-1).
 **
 **   vec:          an array of real64, generally of size
 **                 mtx->order or larger. Indexing scheme is indeterminate.
 **   org_vec:      a vec indexed by the org_row or org_col index.
 **   cur_vec:      a vec indexed by the (current) row or col index.
 **
 **   s,txxx:       source or target xxx, where xxx may be a  vec, row, col
 **                 org_row, or org_col.
 **
 **   order:        the working size of a matrix.
 **                 order N ==> cols/rows 0->N-1 exist.
 **   capacity:     the length, essentially, of the mtx edges.
 **                 This number is >= order, normally ==.
 **
 **   coord:        a pair of current indices (row,col).
 **   range:        a pair of current indices (low,high).
 **                 operations which take a range will do nothing if
 **                 low > high.
 **   region:       a pair of ranges (rowrange,colrange)
 **   Coords, ranges and regions are passed by pointer to mtx functions.
 **
 **   sparse:       a compressed vector, with indeterminate indexing scheme.
 **                 a sparse has a capacity (maximum amount of data)
 **                 a length (the current amount of data)
 **                 an array of real64 (the data) and
 **                 an array of int32 (where the data correspond to.)
 **                 The array of int32 may be cur or org and this
 **                 is determined by the context in which the sparse is used.
 **
 **                 Sparses are generally passed by pointer.
 *
 * Requires:      #include <stdio.h>
 *                #include <string.h>
 *                #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASC_MTX_H
#define ASC_MTX_H

/***********************************************************************\
  public mtx data structures
\***********************************************************************/
typedef struct mtx_header *mtx_matrix_t;
/**< Handle to the matrix */

/* Used to index rows and columns of matrices */

/**
 ***  Refers to (row,col) element of a matrix
 **/
typedef struct mtx_coord_struct {
   int32 row,col;
} mtx_coord_t;

/**
 *** provide a list type for giving functions a list of coordinates.
 **/
struct mtx_coord_list {
  mtx_coord_t coord;
  struct mtx_coord_list *next;
};

typedef struct mtx_range_struct {
  int32 low,high;
} mtx_range_t;
/**<
 ***  Range refered to is low..high, inclusive.  If low>high, range is empty
 **/

typedef struct mtx_region_struct {
  mtx_range_t row,col;
} mtx_region_t;
/**< 
 ***  Rectangular region of a matrix
 **/

typedef struct mtx_block_struct {
  int32 nblocks;
  mtx_region_t *block;		/**< array of regions denoting blocks */
} mtx_block_t;
/**< 
 *** Block structure of a matrix.
 **/

typedef struct mtx_sparse_vector_struct {
  real64 *data;
  int32 *idata;
  int32 cap;
  int32 len;
} mtx_sparse_t;
/**< 
 *** A structure for holding a compressed row or column vector.
 *** The semantics imposed for this structure are very limited
 *** so that one may be used (or reused) in many contexts.
 *** An mtx_sparse_t doesn't know where its data came from, or by
 *** what scheme the data is indexed.<br><br>
 ***
 *** The value of data[i] goes with mtx location somehow indexed idata[i].
 ***
 *** The values of idata[i] do not have to be in any particular
 *** order, and the vector knows nothing about whether the indices
 *** stored in idata are row indices, column indices, org_row indices
 *** or org_column indices. This information is determined by the context
 *** of the mtx_sparse_t.<br><br>
 ***
 *** cap is the size of data and idata.
 *** len is the length of data and idata (starting from location 0)
 *** which contains valid data.<br><br>
 ***
 *** A value of -1 for len indicates that pointers data and idata are invalid,
 *** as does a value of 0 for cap.<br><br>
 ***
 *** NOTE: The mtx_sparse_t is not a pointer to a struct, it IS the struct.
 **/

typedef struct mtx_block_perm_structure *mtx_block_perm_t;
/**< 
 ***  The mtx_block_perm_structure contains a copy of the information
 ***  needed to apply a previously derived and saved ordering to the
 ***  matrix in question. Precisely what this information is is nobody's
 ***  business. We reserve the right to change at a moment's notice what
 ***  the mtx_block_perm_structure actually is.<br><br>
 ***
 ***  The utility of the mtx_block_perm_structure is as follows:
 ***  Say in a sequence of nonlinear solves, one wishes to solve
 ***  many matrices of the same nonzero pattern, but different
 ***  nonzero values. One wants to reorder a specified block
 ***  of the matrix one time and reuse that initial ordering at each
 ***  factorization. Now suppose one also wants to use a linear
 ***  solution method that messes with the ordering during factorization.
 ***  One can do the following:
 ***  Create the mtx in question, output assign and partition it.<br><br>
 ***  <pre>
 ***  Call bp=mtx_create_block_perm(mtx).
 ***   This returns a copy of the information we need to store.
 ***  Working with individual blocks is then as follows:
 *** 1- Reorder the block in question in the mtx in any way you like.
 *** 2- Call mtx_update_block_perm(mtx,block_number,bp)
 ***      This updates the saved information for rows and columns
 ***      *** within *** the block specified by copying from the mtx.
 ***      All sorts of inexpensive sanity checks will be performed to
 ***      ensure that this is a wise thing to do.
 *** 3- Factor, or otherwise do anything you want to the permutation
 ***    of the original mtx, so long as you do not mess with more than
 ***    one block. Changing the order (size) of the mtx is not permitted.
 *** 4- Call mtx_restore_block_perm(mtx,block_number,bp)
 ***     This takes information stored in bp and resets the permutation
 ***     of the mtx within block_number to match it.
 ***     All sorts of inexpensive sanity checks will be performed to
 ***     ensure that this is a wise thing to do.
 *** 5- Repeat steps 3 and 4 ad nauseum, or loop back through 1 and 2
 ***    if moving from block to block.
 *** 6- When done with the matrix, or at least the process in question,
 ***    don't forget to do an mtx_destroy_block_perm(bp).
 ***  </pre>
 ***  Other variations on this scheme are possible. See the headers for
 ***  details of the individual functions.
 **/

#define mtx_ENTIRE_MATRIX NULL
/**< 
 ***  Refers to the entire matrix in any function where a region is called for.
 **/

#define mtx_ALL_ROWS NULL
#define mtx_ALL_COLS NULL
/**< 
 ***  Refers to all rows/columns in any function where a row/column range
 ***  is called for.
 **/

#define mtx_CREATE_SPARSE NULL
/**< 
 ***  In calls that take an mtx_sparse_t as an argument AND return a
 ***  pointer to a sparse AND do not note otherwise in the header
 ***  mtx_CREATE_SPARSE can be passed the argument and the call will
 ***  allocate the sparse_t data structure it returns. In such cases,
 ***  it is the users responsibility to see that the sparse is
 ***  eventually destroyed.
 **/

#define mtx_IGNORE_ZEROES (1)
#define mtx_SOFT_ZEROES (0)
/**< 
 *** Those functions which fill a sparse_t might optionally return
 *** soft zeroes given mtx_SOFT_ZEROES.
 **/

#define mtx_FIRST      ((int32)(-1))
#define mtx_LAST       ((int32)(-1))
#define mtx_NONE       ((int32)(-1))
#define mtx_ALL_BLOCKS ((int32)(-1))

#include "mtx_basic.h"
 /**
 ***  All operations involving accounting/memory
 ***  management and CHANGES to matrix data structure.
 ***  This includes data file i/o and floating point
 ***  exception recovery handling.
 **/

#include "mtx_perms.h"
#include "mtx_reorder.h"
/***                        
 ***  Functions for permuting matrices, reordering, etc.
 ***  These probably ought to be combined. mtx_reorder is currently
 ***  thinly disguised linsol code that needs to be redone in the mtx idiom.
 **/

#include "mtx_query.h"
/***
 ***  READ_ONLY functions that don't make more sense
 ***  in mtx2_perms or mtx2_basic. Most vector data
 ***  and basic math operations fall in this file.
 **/

#include "mtx_linal.h"
/***                        
 ***  Functions which probably shouldn't exist at all,
 ***  but are simply too damned expensive to do without
 ***  knowing all the internals of the mtx. These tend
 ***  to be ultracritical path functions in linsol.
 ***  These functions generally have very, very
 ***  complex semantics and preconditions and assumptions.
 ***  You really DON'T want to add functions to this
 ***  file or use those that are in it.
 **/

#endif /* ASC_MTX_H */
