/* 
 *  SLV: Ascend Nonlinear Solver
 *  by Kirk Andre' Abbott
 *  Created: 10/06/95
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: rootfind.h,v $
 *  Date last modified: $Date: 1997/07/18 12:33:57 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
 *  COPYING.
 */

/** @file
	Root finding routine for the SLV solver.
	<pre>
	Requires:     #include "utilities/ascConfig.h"
	
	              #ifndef STAND_ALONE
	                  #include "extfunc.h"  (for ExtValFunc())
	              #else
	                  #include "codegen_support.h"
	              #endif
	</pre>
	@todo extfunc.h needed only for the definition of ExtEvalFunc -
	      This definition should probably be moved to types.h. or compiler.h

	@todo codegen_support.h not in base/generic/solver - need to update include?
*/

#ifndef ASC_ROOTFIND_H
#define ASC_ROOTFIND_H

/**	addtogroup compiler Compiler
	@{
*/

extern double zbrent(ExtEvalFunc *func,
                     double *lowbound,
                     double *upbound,
                     int *mode,
                     int *m,
                     int *n,
                     double *x,
                     double *u,
                     double *f,
                     double *g,
                     double *tolerance,
                     int *status);
/**< Find a root in a given interval using Brent's method.

	Using Brents method, find the root of a function known to lie
	between x1 and x2. The root, returned as zbrent, will be refined
	until its accuracy is tol. The result of status must be monitored
	to see if we were successful. A nonzero code means that the result
	returned is erroneous. n is the index into the x vector of the variable
	that is to be solved for.

	@param func      The evaluation function.
	@param lowbound  Lower bound.
	@param upbound   Upper bound.
	@param mode      Passed to the eval func.
	@param m         The relation index.
	@param n         The variable index.
	@param x         The x vector -- needed by eval func.
	@param u         The u vector -- needed by eval func.
	@param f         Vector of residuals.
	@param g         Vector of gradients.
	@param tolerance Accuracy of solution.
	@param status    Success or failure.
	@return          The location of the found root (see also status code)
*/

/* @} */

#endif  /* ASC_ROOTFIND_H */

