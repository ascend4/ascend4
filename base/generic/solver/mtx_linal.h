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
	requires:   #include "utilities/ascConfig.h"
	requires:   #include "mtx.h"
*//* 
	mtx by Karl Michael Westerberg, 5/3/90
	mtx2 by Benjamin Andrew Allan
	Last in CVS: $Revision: 1.4 $ $Date: 1997/07/18 12:15:02 $ $Author: mthomas $
*/

#ifndef __MTX_LINAL_H_SEEN__
#define __MTX_LINAL_H_SEEN__

/**	@addtogroup linear 
	Linear solver routines
	@{ */

extern void mtx_householder_transform_region(mtx_matrix_t mtx,
                                             const real64 coef,
                                             const mtx_sparse_t *orgsp,
                                             const mtx_region_t *reg,
                                             real64 droptol,
                                             boolean transpose);
/**<
 ***  Does a sparse Householder transformation to the matrix A
 ***  denoted by region, resulting in Anew.<br><br>
 ***  Mathematically: Anew = (I - coef * u dot Transpose[u]) dot A.<br><br>
 ***  Slightly faster in the case of coef = 1.0.
 -$-  Returns immediately in the case of coef = 0.0.
 ***  Note following unusual assumptions, however:
 ***   -# The user has already cleared out the leading column in the
 ***      region and we will not be doing anything to it. This is usually
 ***      the case since that column is where u was derived from.
 ***   -# The sparse, sp, supplied by the user contains the nonzero
 ***      elements of u and is indexed by org row number.
 ***      Note that a soft zero in u will not hurt us: we skip it.
 ***   -# There are no elements in the regions to the left and the right
 ***      of the region.
 ***
 ***  mtx_ENTIRE_MATRIX is not a valid region for this function.
 ***
 ***  @bug (11/95) ignores the transpose and droptol arguments.
 ***  @bug (11/95) does not remove soft zeroes created when 2 numbers add to zero.
 ***
 ***  This decl has the '-$-' thing going on.
 **/

/** @} */

#endif /* __MTX_LINAL_H_SEEN__ */
