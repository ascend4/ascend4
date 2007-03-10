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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//* @file
	Boundary Manipulator Module.

	This module will provide supplemental operations for
	boundaries such as evaluation.
*//*
	Created: 04/97
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/18 12:13:58 $ $Author: mthomas $
*/

#ifndef ASC_BNDMAN_H
#define ASC_BNDMAN_H

#include <utilities/ascConfig.h>
#include "bnd.h"

/**	@addtogroup solver Solver
	@{
*/

extern double bndman_real_eval(struct bnd_boundary *bnd);
/**<  Returns the (real) residual of the boundary. */

extern int32  bndman_log_eval(struct bnd_boundary *bnd);
/**<
 *  Returns the (boolean) residual of the boundary.
 */

extern int32 bndman_calc_satisfied(struct bnd_boundary *bnd);
/**<
 *  Returns whether the boundary is currently satisfied based on its
 *  calculated residual.
 */

extern int32 bndman_calc_at_zero(struct bnd_boundary *bnd);
/**<
 *  Returns whether the current point lies at the zero of a boundary
 *  based on the calculation of the residual of the boundary
 *  Used only for real(not boolean) boundaries.
 */

/* @} */

#endif /* ASC_BNDMAN_H */
