/* 
 *  mtx: Ascend Sparse Matrix Package
 *  by Benjamin Andrew Allan
 *  Derived from mtx by Karl Michael Westerberg
 *  Created: 5/3/90
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: mtx_reorder.h,v $
 *  Date last modified: $Date: 1997/07/18 12:15:17 $
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

#ifndef __MTX_REORDER_H_SEEN__
#define __MTX_REORDER_H_SEEN__

enum mtx_reorder_method {
  mtx_UNKNOWN,  /**< junk method */
  mtx_SPK1,     /**< Stadtherr's SPK1 reordering */
  mtx_TSPK1,    /**< transpose of Stadtherr's SPK1 reordering */
  mtx_NATURAL   /**< kinda pointless, don't you think? */
};

extern int mtx_reorder(mtx_matrix_t mtx,
                       mtx_region_t *region, 
                       enum mtx_reorder_method rmeth);
/**<
 ***  <!--  mtx_reorder(sys,region,rmeth)                              -->
 ***  <!--  mtx_matrix_t mtx;                                          -->
 ***  <!--  mtx_region_t *region;                                      -->
 ***  <!--  enum mtx_reorder_method;                                   -->
 ***
 ***  The specified region of the coefficient matrix is reordered.
 ***  The specified region
 ***  is assumed to contain only nonempty rows and columns and have a full
 ***  diagonal.
 ***  If the matrix in is from a nonlinear system, the
 ***  pattern in the matrix should probably be the structural one (as
 ***  opposed to the numerically derived incidence which may be less.)<br><br>
 ***
 ***  If you use the numerically derived incidence, you will need to reorder
 ***  before every factorization. This is generally not cost effective.
 ***  If region given is mtx_ENTIRE_MATRIX, a search will be done to find
 ***  an appropriate bounding region in the coefficient mtx. This is
 ***  not a particularly cheap search.<br><br>
 ***
 ***  The reorder call should be done (once) at
 ***  the beginning of any series of linear solutions being performed on
 ***  on structurally identical matrices.<br><br>
 ***
 ***  We HATE mtx_ENTIRE_MATRIX as the input region.<br><br>
 ***
 ***  Brief notes on the reorderings available.
 ***   - SPK1:
 ***   - TSPK1: Both these operate only on a square region which is of.
 ***            full rank (in particular it should have a 0 free diagonal).
 ***            If you give us a bogus region, we will Try to make something
 ***            decent of it, but good results are improbable.
 ***   - Natural: Blesses the system and does nothing.
 ***              Again, the rows/cols not in the diagonal are dependent.
 ***
 ***  On reordering in general: 'Optimal' reordering is an NP complete
 ***  task. Real reordering methods are heuristic and tend to break down
 ***  when the matrix being reordered is in some sense 'large' so that
 ***  the tie-breaking and other heuristics fail.<br><br>
 ***
 ***  Return 0 if ok, 1 if bad input detected, 2 if unable to do.
 **/

#endif /* __MTX_REORDER_H_SEEN__ */

