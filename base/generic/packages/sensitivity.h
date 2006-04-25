/*
	Sensitivity add-on for ASCEND
	by Tom Epperly, Kirk Abbot

	Copyright (C) 1990-2006 Carnegie-Mellon University

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
	This file is part of the SLV solver.
*/

/** @file
	@note This is a static package in ASCEND. It can not be built as a dynamic package
	because it doesn't contain the self-registration stuff.

	This file attempts to implement the extraction of dy_dx from
	a system of equations. If one considers a black-box where x are
	the input variables, u are input parameters, y are the output
	variables, f(x,y,u) is the system of equations that will be solved
	for given x and u, then one can extract the sensitivity information
	of y wrt x.

	One crude but simple way of doing this is to finite-difference the
	given black box, i.e, perturb x, n\subx times by delta x, resolve
	the system of equations at the new value of x, and compute
	dy/dx = (f(x\sub1) - f(x\sub2))/(x\sub1 - x\sub2).
	This is known to be very expensive.

	The solution that will be adopted here is to formulate the Jacobian J of
	the system, (or the system is already solved, to grab the jacobian at
	the solution. Develop the sensitivity matrix df/dx by exact numnerical
	differentiation; this will be a n x n\subx matrix. Compute the LU factors
	for J, and to solve column by column to : LU dz/dx = df/dx. Here
	z, represents all the internal variables to the system and the strictly
	output variables y. In other words J = df/dz.

	Given the solution of the above problem we then simply extract the rows
	of dz/dx, that pertain to the y variables that we are interested in;
	this will give us dy/dx.

	@todo Do we really need 2 files called [Ss]ensitivity.[ch]?  Other one is in tcltk98.

	@todo Make this self-registering so that it can be compiled as a dlopenable library.
*/

/** @file
Requires:
#include <utilities/ascConfig.h>
#include <compiler/instance_enum.h>
#include <compiler/compiler.h>
#include <general/list.h>
#include <compiler/extfunc.h>
*/

#ifndef ASC_SENSITIVITY_H
#define ASC_SENSITIVITY_H

/* ignores: interp, i, whichvar */
extern int do_solve_eval( struct Instance *i, struct gl_list_t *arglist);

/* ignores: interp, i, whichvar */
extern int do_finite_diff_eval( struct Instance *i, struct gl_list_t *arglist);

extern char sensitivity_help[];

/* ignores: interp, i,  */
extern int do_sensitivity_eval_all( struct Instance *i, struct gl_list_t *arglist);

/* ignores: interp, i,  */
extern int do_sensitivity_eval( struct Instance *i, struct gl_list_t *arglist);

ASC_DLLSPEC(int) sensitivity_register(void);

#endif  /* ASC_SENSITIVITY_H */

