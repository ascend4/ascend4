/*	ASCEND modelling environment
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
*//* @file
	Vector math structure and methods

	If we get brave, we will consider replacing the cores of these routines with 
	BLAS calls. We aren't overeager to go mixed language call nuts just yet,
	however.

	Comment: the NVector implementation provided with SUNDIALS might be an
	easier-to-integrate solution for this. It's also MPI-friendly. -- John Pye

	A dense vector class of some utility and the functions for it.
	The vector consists of an array of real64 (vec) and a mtx_range_t
	(rng) which refers to subsets of the range of indexes of vec.
	When calling the various vector functions, the range indexes in
	rng are used to calculate offsets in the vec array.  Therefore,
	it is important that your rng->(low,high) refer to valid indexes
	of vec[].  In particular
	  - neither rng->low nor rng->high may be negative
	  - low <= high
	  - high < length of vec
	This means that whatever your maximum high is, you should allocate
	(high+1) values in vec.

	@todo solver/slv_common:vec_vector & operations should be
	      moved to a module in general or utilities.
*/

#ifndef ASC_MTX_VECTOR_H
#define ASC_MTX_VECTOR_H

#include <utilities/ascConfig.h>
#include "mtx.h"

/** @addtogroup linear Linear
 * @{
 */

struct vec_vector {
   real64       norm2;      /**< 2-norm of vector squared. */
   mtx_range_t  *rng;       /**< Pointer to range of vector (low..high). */
   real64       *vec;       /**< Data array (NULL => uninitialized). */
   boolean      accurate;   /**< Is vector currently accurate?  User-manipulated. */
};

ASC_DLLSPEC struct vec_vector *vec_create(int32 low, int32 high);
/**<
 *  Returns a new vec_vector initialized to the specified range.
 *  This function creates, initializes, and returns a new vec_vector
 *  structure.  The vector is initialized using init_vector() and
 *  a pointer to the new struct is returned.  If the specified range
 *  is improper (see vec_init()) then a valid vector cannot be
 *  created and NULL is returned.<br><br>
 *
 *  Destruction of the returned vec_vector is the responsibility of
 *  the caller.  vec_destroy() may be used for this purpose.
 *
 *  @param low  The lower bound of the vector's range.
 *  @param high The upper bound of the vector's range.
 *  @return A new initialized vec_vector, or NULL if one could
 *          not be created.
 */

ASC_DLLSPEC int vec_init(struct vec_vector *vec, int32 low, int32 high);
/**<
 *  Initializes a vec_vector structure.
 *  The new range (low..high) is considered proper if both low and
 *  high are zero or positive, and (low <= high).  If the new range is
 *  not proper (or if vec itself is NULL), then no modifications are
 *  made to vec.<br><br>
 *
 *  If the range is proper then vec->rng is allocated if NULL and then
 *  set using low and high.  Then vec->vec is allocated (if NULL) or
 *  reallocated to size (high+1).  The data in vec->vec is not
 *  initialized or changed.  The member vec->accurate is set to FALSE.
 *
 *  @param vec  Pointer to the vec_vector to initialize.
 *  @param low  The lower bound of the vector's range.
 *  @param high The upper bound of the vector's range.
 *  @return Returns 0 if the vector is initialized successfully,
 *          1 if an improper range was specified, 2 if vec is NULL,
 *          and 3 if memory cannot be allocated.
 */

ASC_DLLSPEC void vec_destroy(struct vec_vector *vec);
/**<
 *  Destroys a vector and its assocated data.
 *  Deallocates any memory held in vec->rng and vec->vec,
 *  and then deallocates the vector itself.  NULL is tolerated
 *  for vec, vec->rng, or vec->vec.
 *
 *  @param vec Pointer to the vec_vector to destroy.
 */

ASC_DLLSPEC void vec_zero(struct vec_vector *vec);
/**<
 *  Zeroes a vector.
 *  The vector entries between vec->rng.low and  vec->rng.high will
 *  be set to 0.0.
 *  The following are not allowed and are checked by assertion:
 *    - NULL vec
 *    - NULL vec->rng
 *    - NULL vec->vec
 *    - vec->rng->low < 0
 *    - vec->rng->low > vec->rng->high
 *
 *  @param vec The vector to zero.
 */

ASC_DLLSPEC void vec_copy(struct vec_vector *srcvec,
                            struct vec_vector *destvec);
/**<
 *  Copies the data from srcvec to destvec.
 *  The data in the range [srcvec->rng.low .. srcvec->rng.high]
 *  is copied to destvec starting at position destvec->rng.low.
 *  destvec must have at least as many elements in vec as srcvec.
 *  The following are not allowed and are checked by assertion:
 *    - NULL srcvec
 *    - NULL srcvec->rng
 *    - NULL srcvec->vec
 *    - srcvec->rng->low < 0
 *    - srcvec->rng->low > srcvec->rng->high
 *    - NULL destvec
 *    - NULL destvec->rng
 *    - NULL destvec->vec
 *    - destvec->rng->low < 0
 *
 *  @param srcvec  The vector to copy.
 *  @param destvec The vector to receive the copied data.
 */

ASC_DLLSPEC real64 vec_inner_product(struct vec_vector *vec1,
                                struct vec_vector *vec2);
/**<
 *  Calculates the dot product of 2 vectors.
 *  Dot [vec1->rng.low .. vec1->rng.high] with vec2 starting at
 *  position vec2->rng.low.
 *  The following are not allowed and are checked by assertion:
 *    - NULL vec1
 *    - NULL vec1->rng
 *    - NULL vec1->vec
 *    - vec1->rng->low < 0
 *    - vec1->rng->low > vec1->rng->high
 *    - NULL vec2
 *    - NULL vec2->rng
 *    - NULL vec2->vec
 *    - vec2->rng->low < 0
 *
 *  @param vec1 The 1st vector for the dot product.
 *  @param vec2 The 2nd vector for the dot product.
 *  @todo solver/slv_common:vec_inner_product() could stand to be optimized.
 */

ASC_DLLSPEC real64 vec_square_norm(struct vec_vector *vec);
/**<
 *  Calculates the dot product of a vector with itself.
 *  Dot [vec->rng.low .. vec->rng.high] with itself and store the
 *  result in vec->norm2.
 *  The following are not allowed and are checked by assertion:
 *    - NULL vec
 *    - NULL vec->rng
 *    - NULL vec->vec
 *    - vec->rng->low < 0
 *    - vec->rng->low > vec->rng->high
 *
 *  @param vec The vector for the dot product.
 *  @todo solver/slv_common:vec_square_norm() could stand to be optimized.
 */

ASC_DLLSPEC void vec_matrix_product(mtx_matrix_t mtx,
                               struct vec_vector *vec,
                               struct vec_vector *prod,
                               real64 scale,
                               boolean transpose);
/**<
 *  Calculates the product of a vector, matrix, and scale factor.
 *  Stores prod := (scale)*(mtx)*(vec) if transpose = FALSE,
 *  or prod := (scale)*(mtx-transpose)(vec) if transpose = TRUE.
 *  vec and prod must be completely different.
 *  If (!transpose) vec->vec is assumed indexed by current col and
 *                 prod->vec is indexed by current row of mtx.
 *  If (transpose) vec->vec is assumed indexed by current row and
 *                 prod->vec is indexed by current col of mtx.
 *  The following are not allowed and are checked by assertion:
 *    - NULL mtx
 *    - NULL vec
 *    - NULL vec->rng
 *    - NULL vec->vec
 *    - vec->rng->low < 0
 *    - vec->rng->low > vec->rng->high
 *    - NULL prod
 *    - NULL prod->rng
 *    - NULL prod->vec
 *    - prod->rng->low < 0
 *    - prod->rng->low > prod->rng->high
 *
 *  @param mtx       The matrix for the product.
 *  @param vec       The vector for the product.
 *  @param prod      The vector to receive the matrix product.
 *  @param scale     The scale factor by which to multiply the matrix product.
 *  @param transpose Flag for whether to use mtx or its transpose.
 *
 *  @todo solver/slv_common:slv_mtx_product needs attention -
 *        does it go into mtx?
 */

ASC_DLLSPEC void vec_write(FILE *fp, struct vec_vector *vec);
/**<
 *  Write vector information to a file stream.
 *  Prints general information about the vector followed by the
 *  values in the range of the vector to file fp.
 *
 *  @param fp  The file stream to receive the report.
 *  @param vec The vector on which to report.
 */

/*------------------------------------------------------------------------------
  BLAS-LIKE FUNCTIONS
*/

ASC_DLLSPEC real64 vec_dot(int32 len, const real64 *a1, const real64 *a2);
/**<
 *  Calculates the dot product of 2 arrays of real64.
 *  This is an optimized routine (loop unrolled).  It takes
 *  advantage of identical vectors.  The 2 arrays must have
 *  at least len elements.
 *  The following are not allowed and are checked by assertion:
 *    - NULL a1
 *    - NULL a2
 *    - len < 0
 *
 *  The same algorithm is used inside vec_inner_product(), so there
 *  is no need to use this function directly if you are using the
 *  vec_vector type.
 *
 *  @param len The length of the 2 arrays.
 *  @param a1  The 1st array for the dot product.
 *  @param a2  The 2nd array for the dot product.
 */

/**
	@}
*/

#endif
