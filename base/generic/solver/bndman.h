/**< 
 *  Boundary Manipulator Module
 *  Created: 04/97
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: bndman.h,v $
 *  Date last modified: $Date: 1997/07/18 12:13:58 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */


/**< 
 *  Contents:     Boundary manipulator module
 *
 *  Dates:        04/97 - original version
 * 
 *  Description:  This module will provide supplemental operations for
 *                boundaries such as evaluation.
 */

#ifndef bndman__already_included
#define bndman__already_included

extern real64 bndman_real_eval(struct bnd_boundary *);
extern int32  bndman_log_eval(struct bnd_boundary *);
/**< 
 *  value = bndman_real_eval(bnd)
 *  logvalue = bndman_log_eval(bnd)
 *  real64 value;
 *  int32 logvalue;
 *  struct bnd_boundary *bnd;
 *
 *  Returns the (real/boolean) residual of the boundary.
 */

extern int32 bndman_calc_satisfied(struct bnd_boundary *);
/**< 
 *  value = bndman_calc_satisfied(bnd);
 *  int32 value;
 *  struct bnd_boundary *bnd;
 *
 *  Returns whether the boundary is currently satisfied based on its
 *  calculated residual.
 */

extern int32 bndman_calc_at_zero(struct bnd_boundary *);
/**< 
 *  value = bndman_calc_at_zero(bnd);
 *  int32 value;
 *  struct bnd_boundary *bnd;
 *
 *  Returns whether the current point lies at the zero of a boundary
 *  based on the calculation of the residual of the boundary
 *  Used only for real(not boolean) boundaries.
 */

#endif  /**< bndman__already_included */
