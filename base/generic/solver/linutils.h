/*
 *  lin_utils: Ascend Linear Algebra Utilities
 *  by Kirk Andre' Abbott
 *  Created: 12 March 1995
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: linutils.h,v $
 *  Date last modified: $Date: 1997/07/18 12:14:34 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1995 Kirk Andre' Abbott
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
 *  lin_utils: Ascend Linear Algebra Utilities.
 *  <pre>
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "mtx.h"
 *                #include "linsol.h"
 *                #include "linsolqr.h"
 *  </pre>
 *  @todo Document functions in linutils.h.
 */

#ifndef linutils_already_included
#define linutils_already_included

/** @addtogroup linear Linear
 * @{
 */

ASC_DLLSPEC(double ) linutils_A_1_norm(mtx_matrix_t mtx,
                                mtx_region_t *reg);

ASC_DLLSPEC(double ) linutils_A_infinity_norm(mtx_matrix_t mtx,
                                       mtx_region_t *reg);

ASC_DLLSPEC(double ) linutils_A_Frobenius_norm(mtx_matrix_t mtx,
                                        mtx_region_t *reg);

ASC_DLLSPEC(double ) linutils_A_condqr_kaa(linsolqr_system_t lin_sys,
                                    mtx_matrix_t mtx,
                                    mtx_region_t *reg);

ASC_DLLSPEC(double ) linutils_A_cond_kaa(linsol_system_t lin_sys,
                                  mtx_matrix_t mtx,
                                  mtx_region_t *reg);

/**	@} */

#endif /* linutils_already_included */
