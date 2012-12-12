/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @defgroup system_bndman System Boundary Manipulation

	This module will provide supplemental operations for
	boundaries such as evaluation.
*//*
	Created: 04/97
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/18 12:13:58 $ $Author: mthomas $
*/

#ifndef ASC_BNDMAN_H
#define ASC_BNDMAN_H

#include <ascend/general/platform.h>
#include "bnd.h"

/**	@addtogroup system_bndman
	@{
*/

ASC_DLLSPEC double bndman_real_eval(struct bnd_boundary *bnd);
/**<  Returns the (real) residual of the boundary. */

ASC_DLLSPEC int32  bndman_log_eval(struct bnd_boundary *bnd);
/**<
 *  Returns the (boolean) residual of the boundary.
 */

ASC_DLLSPEC int32 bndman_calc_satisfied(struct bnd_boundary *bnd);
/**<
 *  Returns whether the boundary is currently satisfied based on its
 *  calculated residual.
 */

ASC_DLLSPEC int32 bndman_calc_at_zero(struct bnd_boundary *bnd);
/**<
 *  Returns whether the current point lies at the zero of a boundary
 *  based on the calculation of the residual of the boundary
 *  Used only for real(not boolean) boundaries.
 */

/* @} */

#endif /* ASC_BNDMAN_H */
