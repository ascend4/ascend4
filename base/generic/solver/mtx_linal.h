/* 
 *  mtx2: Ascend Sparse Matrix Package
 *  by Benjamin Andrew Allan
 *  Derived from mtx by Karl Michael Westerberg
 *  Created: 5/3/90
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: mtx_linal.h,v $
 *  Date last modified: $Date: 1997/07/18 12:15:02 $
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

/** @file
 *  mtx2: Ascend Sparse Matrix Package.
 *  <pre>
 *  requires:   #include "utilities/ascConfig.h"
 *  requires:   #include "mtx.h"
 *  </pre>
 */

#ifndef __MTX_LINAL_H_SEEN__
#define __MTX_LINAL_H_SEEN__

extern void mtx_householder_transform_region(mtx_matrix_t mtx,
                                             const real64 coef,
                                             const mtx_sparse_t *orgsp,
                                             const mtx_region_t *reg,
                                             real64 droptol,
                                             boolean transpose);
/**<
 -$-  <!--  mtx_householder_transform_region(mtx,coef,orgsp,reg,droptol,transpose); -->
 ***  <!--  mtx_matrix_t mtx;                                          -->
 ***  <!--  real64 coef,droptol;                                       -->
 ***  <!--  mtx_sparse_t *sp;                                          -->
 ***  <!--  mtx_region_t *reg;                                         -->
 ***  <!--  real64 droptol;                                            -->
 ***  <!--  boolean transpose;                                         -->
 ***
 ***  Does a sparse Householder transformation to the matrix A
 ***  denoted by region, resulting in Anew.<br><br>
 ***  Mathematically: Anew = (I - coef * u dot Transpose[u]) dot A.<br><br>
 ***  Slightly faster in the case of coef = 1.0.
 ***  Returns immediately in the case of coef = 0.0.
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
 **/

#endif /* __MTX_LINAL_H_SEEN__ */
