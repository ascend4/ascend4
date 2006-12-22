#ifdef __MTX_C_SEEN__
/*
 *  mtx2: Ascend Sparse Matrix Package
 *  by Benjamin Andrew Allan
 *  Derived from mtx by Karl Michael Westerberg
 *  Created: 5/3/90
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: mtx_use_only.h,v $
 *  Date last modified: $Date: 2000/01/25 02:27:13 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1996 Benjamin Andrew Allan
 *  based (loosely) on mtx
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Kirk Andre Abbott, Benjamin Andrew Allan
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
 *  mtx2: Ascend Sparse Matrix Package (Private).
 *
 *  This file defines the private parts of an mtx and is only for mtx*.c
 *  consumption. Any temptation to include this header in a linear or
 *  nonlinear solver package is a symptom of extremely bad programming
 *  and lack of proper task analysis. This header should be regarded as
 *  highly unstable. We make ABSOLUTELY NO commitment to maintain
 *  consistency between any two versions of this file.<br><br>
 *
 *  Note to third parties:
 *  mtx is PRODUCTION code in very long use at Carnegie Mellon University.
 *  *  As such, we maintain a very tight hold of the internals of our data
 *  structure so that we can easily prove the code when apparent bugs arise.
 *  99.44% of "bugs" experienced in using mtx are the result of not
 *  reading the public headers carefully.<br><br>
 *
 *  The material in this file was never a part of any header until the
 *  old mtx.c file got so big that we had to split it up to make it
 *  manageable.<br><br>
 *
 *  Note to future developers of the mtx module. If you change ANYTHING
 *  in this file it is YOUR job to:
 *  a) clear that change with all the other developers using this header
 *  b) fix ALL the other mtx*.c files that depend on it.
 *  If you are not willing to do that much work, why the hell are you
 *  dabbling in sparse matrix math? go work on GUIs.
 *  <pre>
 *  requires:   #include <stdio.h>
 *  requires:   #include "utilities/ascConfig.h"
 *  requires:   #include "mem.h"
 *  requires:   #include "mtx.h"
 *  <pre>
 */

#ifndef __MTX_INTERNAL_USE_ONLY_H__
#define __MTX_INTERNAL_USE_ONLY_H__

#ifndef FALSE
/** These should have come in from ascConfig.h.
 *  @todo Remove redefines of FALSE & TRUE to enforce pre-inclusion of ascCenfig.h? */
#define FALSE 0
#define TRUE 1
#endif

#define MTX_DEBUG FALSE
/**< MTX_DEBUG is a no holds barred sanity checking flag for use when
 * nothing else is giving a clue why something is going wrong. It
 * slows down the code to a crawl. Do not under any conditions change
 * its value or undefine it except at this location. If you need some
 * other sort of debugging flag for debugging a particular function,
 * use some personal debugging flag.
 */

#define EVEN FALSE
#define ODD TRUE
#define SWAPS_PRESERVE_ORDER TRUE
/**< 
 ***  Do row and column swaps preserve the ordering of non-zeros in rows
 ***  and columns?  Setting this to TRUE means swapping only entails the
 ***  movement of integer row or column numbers and NOT the exchange of
 ***  entire row or columns.
 **/
#define WIDTHMAGIC 2048
/**< 
 *** WIDTHMAGIC is the byte size to aim for in allocating groups of elements.
 **/
#define LENMAGIC 10
/**<
 *** LENMAGIC initial # of groups of elements, hence the smallest
 *** possible number of elements a matrix will ever have is LENM*WIDTHM/eltsize.
 **/

extern FILE *g_mtxerr;
/**<
 *** Global file pointer to which errors are reported. Should never be
 *** NULL. Also useful when running ascend in gdb and you can't find
 *** any other file pointer to use.
 **/

/** just a struct to make resulting code more readable. */
struct next_element_t {
  struct element_t *row;
  struct element_t *col;
};

/**
 ***  This is the basic jacobian element of an mtx.
 ***  It's size is 24 bytes on 4 byte pointer machines and
 ***  32 bytes on 8 byte pointer machines.
 ***  The elements form a bidirectional singly linked list.
 ***  The row and col indices in an element refer back to
 ***  the header positions of the two lists that element is in.
 ***  That is, each element knows its orgrow and orgcol.
 **/  
struct element_t {
  real64 value;
  int32 row;
  int32 col;
  struct next_element_t next;
};

/**
 ***  Each matrix is really just a pair of arrays of pointers to
 ***  elements. The index of a row or column in THESE arrays is
 ***  what is referred to as an org index. A value of NULL in
 ***  either array means that that row (or col) is empty.<br><br>
 ***
 ***  When we insert elements in the matrix, we simply shove the
 ***  element in at the head of the its row/column lists.
 ***  When we delete an element in the matrix, we search in one
 ***  direction and unlink the element, marking it "dead". Then a
 ***  general pass in the other direction unlinks all the "dead"
 ***  elements.<br><br>
 ***
 ***  Special note: The -1th element of nz_header arrays is NOT allocated.
 **/
struct nz_headers_t {
  struct element_t **row;
  struct element_t **col;
};

/**<
 ***  We maintain, rather than rederiving, the information required to
 ***  answer all possible permutation questions.
 ***  This is a policy decision based on the fact that mtx is research
 ***  code that needs maximal flexibility at reasonable speed.<br><br>
 ***
 ***  The -1th element of org_to_cur and cur_to_org are defined because
 ***  -1 is used all over mtx as an error return. It's easier to debug
 ***  things without the memory access errors that would happen if
 ***  -1 were not allocated or were part of memory in some other object.<br><br>
 ***
 ***  Special note: The -1th element of nz_header arrays is NOT allocated.<br><br>
 ***
 ***  Do not access the parity field of a slave matrix, refer to its master. 
 ***  Conduct all permuting operations on the master.
 **/
struct permutation_t {
  int32 *org_to_cur;      /**< org_to_cur[-1] = -1 */
  int32 *cur_to_org;      /**< cur_to_org[-1] = -1 */
  boolean parity;
};

struct permutations_t {
  struct permutation_t row;
  struct permutation_t col;
  int32 transpose;
};

/**<
 *** There is a list of blocks associated with a matrix.
 *** This is an artifact of POOR solver API design between
 *** Peter Piela and Karl Westerberg. The blockwise decomposition
 *** information properly belongs to a linear or nonlinear solver
 *** and not to the mtx.
 ***
 ***  @todo We intend to fix this soon.
 **/
struct structural_data_t {
  int32 symbolic_rank;    /**< Symbolic rank (< 0 if invalid) */
  int32 nblocks;          /**< # blocks in matrix */
  mtx_region_t *block;    /**< Pointer to array of blocks */
};

/**<
 ***  capacity may be > order.
 ***  A matrix of capacity 0 doesn't have a mem_store_t yet and elements
 ***  cannot be queried about without a core dump.
 **/
struct mtx_header {
  int integrity;                  /**< Integrity integer */
  int32 order;                    /**< Order of the matrix */
  int32 capacity;                 /**< Capacity of all the arrays */
  int32 nslaves;                  /**< number of slave matrices */
  struct nz_headers_t hdr;        /**< Non-zero headers of the matrix */
  struct element_t *last_value;   /**< value/set_value memory */
  mem_store_t ms;                 /**< element cache memory */
  struct permutations_t perm;     /**< Permutation vectors */
  struct structural_data_t *data; /**< Pointer to structural information */
  mtx_matrix_t master;            /**< the master of this mtx, if slave */
  mtx_matrix_t *slaves;           /**< array of slave matrices */
};

/**<
 *** If you want to save a permutation for restoration, you
 *** have to make a copy of that data, eh? Here's the place you
 *** put it. Note that the block list should be disappearing from
 *** from the structural data soon.
 **/
struct mtx_block_perm_structure {
  int integrity;
  int32 order;                    /**< Order of the matrix */
  int32 capacity;                 /**< Capacity of all the arrays */
  mtx_matrix_t mtx;               /**< matrix of origin */
  struct permutations_t perm;     /**< Permutation vectors */
  struct structural_data_t *data; /**< Pointers to structural information */
};

#define OK        ((int)201539237)
/**< Matrix integrity (ok) value. */
#define DESTROYED ((int)531503871)
/**< matrix integrity (destroyed) value. */

#define ZERO    ((int32)0)
#define D_ZERO  ((real64)0.0)
#define D_ONE   ((real64)1.0)
/**< useful constants if your C compiler is not too bright about ANSI */

#define ISSLAVE(m) ((m)->master!=NULL)
/**<  Returns 1 if m is a slave matrix, 0 if not. */

#define ordered3(a,b,c) ((a) <= (b) && (b) <= (c))
#define in_range(rng,ndx) ordered3((rng)->low,ndx,(rng)->high)
#define legal(mtx,ndx) ordered3(ZERO,ndx,(mtx)->order-1)
/**< 
 ***  Boolean operators to compare a row or column
 ***  index with some specified range or the maximum
 ***  range of the matrix in which it is used.
 **/

#define fast_in_range(l,h,i) ( ordered3(l,i,h) )
#define not_in_range(l,h,i) ( (i)<(l) || (i)>(h) ) 
/**< 
 ***  Boolean operators to compare 3 integers.
 ***  l <= h must be TRUE or these will lie. In many cases,
 ***  this condition can (or should) be met before in_range
 ***  is called. Sometimes these are not faster since the lo,hi vals cost.
 ***  In particular, queries like next_col do not profit while calls
 ***  which must traverse an entire row/col do.
 ***  Gains in cycle count on dec alphas+cc are about 10% per function,
 ***  but the gains in time are more like 1%, so alpha pixie is lying a little.
 ***  For compilers which are not as clever as Decs, (gcc, sun acc) the
 ***  gains should be much more visible. (some do not realize rng->low
 ***  is invariant even with -O.)
 ***  Note that these are 'loose' comparisons if !(l<=h)
 **/

#define zero(ptr,nelts,type)  \
   mem_zero_byte_cast((ptr),0,(nelts)*sizeof(type))
/**< 
 ***  Zeros a vector of specified length and type.
 ***  It is inefficient to use, however, if you know the type
 ***  is one of the basic types (int,double,ptr,char)
 **/


/* ************************************************************************ *\
  Private check routines
\* ************************************************************************ */
extern int super_check_matrix(mtx_matrix_t mtx);
/**< 
 *** After somevery extensive checking, returns an error count.
 *** More or less assume MTX_DEBUG is TRUE, and that is the only
 *** condition under which this should be called.
 **/

/* ************************************************************************ *\
  Element CREATE/find routines. Please try to confine use of these to
  mtx_basic.c as much as possible.
  Use of find should be avoided at all costs, and in particular
  absolutely noone outside mtx should put their fingers on elements.

  These functions are not exported to generic users because they are
  on the critical path and we cannot afford the sanity checking required.
  They should only be called in contexts where the arguments are
  guaranteed valid.
\* ************************************************************************ */

struct element_t *mtx_find_element(mtx_matrix_t mtx, 
                                   int32 org_row, 
                                   int32 org_col);
/**< 
 ***  Searches for a given element of the matrix and returns a pointer to it
 ***  if it exists, or NULL if it doesn't exist. 
 ***  It is *ASSUMED* that org_row
 ***  and org_col are legal indices. May crash if they are not.
 **/

struct element_t *mtx_create_element(mtx_matrix_t mtx, 
                                     int32 org_row, 
                                     int32 org_col);
/**<
 ***  Creates the given element and returns a pointer to it.  The value is
 ***  initially zero.
 ***  It is *ASSUMED* that org_row
 ***  and org_col are legal indices. May crash if they are not.
 ***  If mtx_DEBUG is TRUE, then we will whine if the element already
 ***  exists, but go ahead and create it anyway.
 **/

struct element_t *mtx_create_element_value(mtx_matrix_t mtx, 
                                           int32 org_row,
                                           int32 org_col,
                                           real64 val);
/**<
 ***  Creates the given element and returns a pointer to it.  The value is
 ***  initialzed to val.
 ***  It is *ASSUMED* that org_row
 ***  and org_col are legal indices. May crash if they are not.
 ***  If mtx_DEBUG is TRUE, then we will whine if the element already
 ***  exists, but go ahead and create it anyway.
 **/

/* ************************************************************************ *\
  Element list traversals. No linear algebra programmer with an ounce of
  intelligence would ever need to use these in critical path functions.
\* ************************************************************************ */
extern struct element_t *mtx_next_col(register struct element_t *elt,
                                      mtx_range_t *rng, 
                                      int32 *tocur);
/**< 
 ***  Returns the next element after elt that is in the range
 ***  rng according to the permutation vector tocur given. May return NULL.
 **/

extern struct element_t *mtx_next_row(register struct element_t *elt,
                                      mtx_range_t *rng, 
                                      int32 *tocur);
/**< 
 ***  Returns the next element after elt that is in the range
 ***  rng according to the permutation vector tocur given. May return NULL.
 **/

/* ************************************************************************ *\
  Permutation memory management.
\* ************************************************************************ */
extern int32 *mtx_alloc_perm(int32 cap);
/**<
 ***  Allocates a permutation vector.  The user need
 ***  not concern himself with the -1st element, which does exist.
 **/

extern void mtx_copy_perm(int32 *tarperm, int32 *srcperm, int32 cap);
/**<
 ***  Copies srcperm to tarperm given the capacity of srcperm.
 ***  If tarperm was obtained from alloc_perm(), the -1 has already been copied.
 **/

extern void mtx_free_perm(int32 *perm);
/**<
 ***  Frees resources used by a permutation vector.
 **/

/* ************************************************************************ *\
  It is advantageous in an interactive system to introduce reusable 
  memory and monitor its integrity rather than to repeatedly allocate
  and zero it. The following code accomplishes this for mtx.
  A null_vector is an array of objects (size s, length n) with value 0.
  This sort of memory management is needed because there is always the chance
  that a floating point exception could cause premature return of an mtx
  client. This way we have a safe place to store pointers to the memory
  even if the user's algorithm loses them.
\* ************************************************************************ */

struct reusable_data_vector {
  void *arr;          /**< pointer to array of objects size entrysize */
  int capacity;       /**< number of object slots in array */
  size_t entry_size;  /**< size of slots */
  int last_line;      /**< line most recently associated with this structure,
                         should be 0 if the array is not in use. */
};

extern struct reusable_data_vector
  g_mtx_null_index_data,      /**< bunch of int32 */
  g_mtx_null_sum_data,        /**< bunch of mtx_value_t */
  g_mtx_null_mark_data,       /**< bunch of char */
  g_mtx_null_vector_data,     /**< bunch of element pointers */
  g_mtx_null_col_vector_data, /**< bunch of element pointers */
  g_mtx_null_row_vector_data; /**< bunch of element pointers */

/* OLD GROUP COMMENTS */
/*
 ***  vec = mtx_null_vector(nptrs);
 ***  vec = mtx_null_col_vector(nptrs);
 ***  vec = mtx_null_row_vector(nptrs);
 ***  marks = mtx_null_mark(nchar);
 ***  sums = mtx_null_sum(nnums);
 ***  indexes = mtx_null_index(ninds);
 ***
 ***  struct element_t **vec;
 ***  char *marks;
 ***  real64 *sums;
 ***  int32 *indexes;
 ***  int32 nptrs, nchar, nnums, ninds;
 ***
 ***  Returns an array of chars, elt pointers, indexes or numbers all NULL/0.
 ***  We need these a lot, but seldom simultaneously, and we know generally
 ***  how to rezero them when done with them.
 ***  These functions should not be
 ***  called again until the vector is re-NULLED and out of use.
 ***  If we detect a double call, we will whine loudly, renull
 ***  the array ourselves, and give it to you again.
 ***  To avoid whining, call the corresponding release functions
 ***  each time you are done with one of these vectors.
 ***
 ***  In the event of insufficient memory (alloc failed) we will
 ***  return NULL. If we return NULL, you needn't call the release function.
 ***
 ***  mtx_null_vector_release();
 ***  mtx_null_col_vector_release();
 ***  mtx_null_row_vector_release();
 ***  mtx_null_mark_release();
 ***  mtx_null_sum_release();
 ***  mtx_null_index_release();
 *** 
 ***  These are a memory reuse promoter.
 ***  Calling with cap==0 frees any memory in use.
 ***  Clientlists -- PLEASE KEEP THIS UP TO DATE --
 ***   mtx_null_vector:
 ***      expand_row,expand_col, mtx_assemble
 ***      mtx_householder_transform
 ***   mtx_null_row_vector:
 ***      expand_row_series
 ***   mtx_null_col_vector:
 ***      expand_col_series
 ***   mtx_null_mark:
 ***      mtx_householder_transform
 ***   mtx_null_sum:
 ***      mtx_householder_transform
 ***   mtx_null_index:
 ***      mtx_householder_transform
 **/
#define mtx_null_vector(cap) \
 ((struct element_t **)mtx_null_vector_f(cap,__LINE__,__FILE__, \
                                         &g_mtx_null_vector_data,"null_vector"))
/**<
 *  Returns an array of elt pointers all NULL/0.
 *  This function should not be called again until the vector is 
 *  re-NULLED and out of use.  If we detect a double call, we will
 *  whine loudly, renull the array ourselves, and give it to you again.
 *  To avoid whining, call mtx_null_vector_release()
 *  each time you are done with the returned vector.<br><br>
 *
 *  In the event of insufficient memory (alloc failed) we will
 *  return NULL. If we return NULL, you needn't call the release function.
 *  @param cap int32, the capacity of the matrix (0 to free memory).
 *  @return No return value.
 *  @see mtx_null_vector_f()
 */
#define mtx_null_row_vector(cap) \
  ((struct element_t **)mtx_null_vector_f(cap,__LINE__,__FILE__, \
                                          &g_mtx_null_row_vector_data,\
                                          "null_row_vector"))
/**<
 *  Returns an array of elt pointers all NULL/0.
 *  See mtx_null_vector() for more details.
 *  @param cap int32, the capacity of the matrix (0 to free memory).
 *  @return No return value.
 *  @see mtx_null_vector_f()
 */
#define mtx_null_col_vector(cap) \
  ((struct element_t **)mtx_null_vector_f(cap,__LINE__,__FILE__, \
                                          &g_mtx_null_col_vector_data,\
                                          "null_col_vector"))
/**<
 *  Returns an array of elt pointers all NULL/0.
 *  See mtx_null_vector() for more details.
 *  @param cap int32, the capacity of the matrix (0 to free memory).
 *  @return No return value.
 *  @see mtx_null_vector_f()
 */
#define mtx_null_mark(cap) \
  ((char *)mtx_null_vector_f(cap,__LINE__,__FILE__, \
                             &g_mtx_null_mark_data,"null_mark"))
/**<
 *  Returns an array of chars all NULL/0.
 *  This function should not be called again until the vector is
 *  re-NULLED and out of use.  If we detect a double call, we will
 *  whine loudly, renull the array ourselves, and give it to you again.
 *  To avoid whining, call mtx_null_mark_release()
 *  each time you are done with the returned vector.<br><br>
 *
 *  In the event of insufficient memory (alloc failed) we will
 *  return NULL. If we return NULL, you needn't call the release function.
 *  @param cap int32, the capacity of the array (0 to free memory).
 *  @return No return value.
 *  @see mtx_null_vector_f()
 */
#define mtx_null_sum(cap) \
  ((real64 *)mtx_null_vector_f(cap,__LINE__,__FILE__, \
                                     &g_mtx_null_sum_data,"null_sum"))
/**<
 *  Returns an array of real64 numbers all NULL/0.
 *  This function should not be called again until the vector is
 *  re-NULLED and out of use.  If we detect a double call, we will
 *  whine loudly, renull the array ourselves, and give it to you again.
 *  To avoid whining, call mtx_null_sum_release()
 *  each time you are done with the returned array.<br><br>
 *
 *  In the event of insufficient memory (alloc failed) we will
 *  return NULL. If we return NULL, you needn't call the release function.
 *  @param cap int32, the capacity of the array (0 to free memory).
 *  @return No return value.
 *  @see mtx_null_vector_f()
 */
#define mtx_null_index(cap) \
  ((int32 *)mtx_null_vector_f(cap,__LINE__,__FILE__, \
                                    &g_mtx_null_index_data,"null_index"))

/**<
 *  Returns an array of int32 indexes all NULL/0.
 *  This function should not be called again until the vector is
 *  re-NULLED and out of use.  If we detect a double call, we will
 *  whine loudly, renull the array ourselves, and give it to you again.
 *  To avoid whining, call mtx_null_index_release()
 *  each time you are done with the returned array.<br><br>
 *
 *  In the event of insufficient memory (alloc failed) we will
 *  return NULL. If we return NULL, you needn't call the release function.
 *  @param cap int32, the capacity of the array (0 to free memory).
 *  @return No return value.
 *  @see mtx_null_vector_f()
 */
#define mtx_null_vector_release() \
  mtx_null_vector_release_f(__LINE__,__FILE__, \
                            &g_mtx_null_vector_data,"null_vector")
/**<
 *  Marks a vector as not in use, or whines if it wasn't.
 *  @param None.
 *  @return No return value.
 *  @see mtx_null_vector_release_f()
 */
#define mtx_null_col_vector_release() \
  mtx_null_vector_release_f(__LINE__,__FILE__, \
                            &g_mtx_null_col_vector_data,"null_col_vector")
/**<
 *  Marks a vector as not in use, or whines if it wasn't.
 *  @param None.
 *  @return No return value.
 *  @see mtx_null_vector_release_f()
 */
#define mtx_null_row_vector_release() \
  mtx_null_vector_release_f(__LINE__,__FILE__, \
                            &g_mtx_null_row_vector_data,"null_row_vector")
/**<
 *  Marks a vector as not in use, or whines if it wasn't.
 *  @param None.
 *  @return No return value.
 *  @see mtx_null_vector_release_f()
 */
#define mtx_null_mark_release() \
  mtx_null_vector_release_f(__LINE__,__FILE__, \
                            &g_mtx_null_mark_data,"null_mark")
/**<
 *  Marks a char array as not in use, or whines if it wasn't.
 *  @param None.
 *  @return No return value.
 *  @see mtx_null_vector_release_f()
 */
#define mtx_null_sum_release() \
  mtx_null_vector_release_f(__LINE__,__FILE__, \
                            &g_mtx_null_sum_data,"null_sum")
/**<
 *  Marks a number array as not in use, or whines if it wasn't.
 *  @param None.
 *  @return No return value.
 *  @see mtx_null_vector_release_f()
 */
#define mtx_null_index_release() \
  mtx_null_vector_release_f(__LINE__,__FILE__, \
                            &g_mtx_null_index_data,"null_index")
/**<
 *  Marks an index array as not in use, or whines if it wasn't.
 *  @param None.
 *  @return No return value.
 *  @see mtx_null_vector_release_f()
 */

extern void *mtx_null_vector_f(int32 cap, int line, CONST char *file,
                               struct reusable_data_vector *ptr, char *fn);
/**<
 ***  Implementation function for macros generating vectors of NULL
 ***  elements.  This includes:
 ***    - mtx_null_vector()
 ***    - mtx_null_col_vector()
 ***    - mtx_null_row_vector()
 ***    - mtx_null_mark()
 ***    - mtx_null_sum()
 ***    - mtx_null_index()
 ***
 ***  Do not call this function directly - use the appropriate macro
 ***  instead.
 ***  Returns a pointer to cap*ptr->entry_size bytes, which must be cast.
 ***  The memory pointed at is believed to be zero, and will be if the
 ***  user is properly rezeroing the vector before it is released.
 ***  If insufficient memory is available, this whines and returns NULL.
 ***  Calling this with cap==0 causes the reused memory to be deallocated and
 ***  returns NULL.
 **/

extern void mtx_null_vector_release_f(int line, 
                                      CONST char *file,
                                      struct reusable_data_vector *ptr, 
                                      char *fn);
/**<
 ***  Implementation function for macros releasing reusable vectors.
 ***  This includes:
 ***    - mtx_null_vector_release()
 ***    - mtx_null_col_vector_release()
 ***    - mtx_null_row_vector_release()
 ***    - mtx_null_mark_release()
 ***    - mtx_null_sum_release()
 ***    - mtx_null_index_release()
 ***
 ***  Do not call this function directly - use the appropriate macro
 ***  instead.
 ***  Marks a vector as not in use, or whines if it wasn't.
 ***  Does no other checking. Uses line, file and fn in error reporting.
 **/

extern void mtx_reset_null_vectors(void);
/**<
 *** This resets the reusable arrays of zeroes to zero in the event
 *** that they may have been corrupted.
 **/

/*
 * INTERNAL element vector operations of some utility.
 */

extern struct element_t **mtx_expand_row(mtx_matrix_t mtx, int32 orgrow);
/**<
 ***  Expands the given row into an array of pointers, indexed on original
 ***  col number.  The array is obtained from mtx_null_vector().
 ***  Be sure to call mtx_null_vector_release() when done with the vector and
 ***  you have rezeroed it.
 ***  You cannot call this twice without releasing first or call 
 ***  mtx_expand_col().
 **/

extern struct element_t **mtx_expand_col(mtx_matrix_t mtx, int32 orgcol);
/**<
 ***  Expands the given col into an array of pointers, indexed on original
 ***  row number.  The array is obtained from mtx_null_vector().
 ***  Be sure to call mtx_null_vector_release() when done with the vector and
 ***  you have rezeroed it.
 ***  You cannot call this twice without releasing first or call 
 ***  mtx_expand_row().
 **/

extern void mtx_renull_using_row(mtx_matrix_t mtx, 
                                 int32 orgrow,
                                 struct element_t **arr);
/**<
 ***  Makes arr NULLed again, assuming that the only non-NULL elements
 ***  must correspond to original col numbers that exist in the given
 ***  orgrow.
 **/

extern void mtx_renull_using_col(mtx_matrix_t mtx, 
                                 int32 orgcol,
                                 struct element_t **arr);
/**<
 ***  Makes arr NULLed again, assuming that the only non-NULL elements
 ***  must correspond to original row numbers that exist in the given
 ***  orgcol.
 **/

extern void mtx_renull_all(mtx_matrix_t mtx, struct element_t **arr);
/**<
 ***  Makes arr NULLed again, assuming it is size mtx->order.
 **/

#endif /* __MTX_INTERNAL_USE_ONLY_H__ */
#endif /* none of your business if you aren't mtx_*.c */

