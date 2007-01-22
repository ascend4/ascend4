/*
 *  SLV type definition for ASCEND solvers
 *  SLV: Ascend Nonlinear Solver
 *  by Benjamin Andrew Allan
 *  Created: 6/1/96
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: slv_types.h,v $
 *  Date last modified: $Date: 1997/07/18 12:17:21 $
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
	Type definitions as they should be seen by solver engines.

	We're trying to hide the implementation detail from the solver engines,
	to force them to use the API defined in slv_common.h, slv_client.h, etc.

	Requires: (none)
*//*
	by Ben Allan
	06/96 - original version
*/

#ifndef ASC_SLV_TYPES_H
#define ASC_SLV_TYPES_H

/**	@addtogroup system System
	@{
*/

typedef void *SlvBackendToken;
/**<
	Backends that provide the residuals, gradients, and so forth
	may be object-oriented and associate some sort of pointer
	with each of the variables or relations they are serving up.

	<b>In the case of ASCEND, a SlvBackendToken is simply a pointer to an
	Instance structure.</b>

	Since we want the Slv interface to appear totally backend
	independent (in particular because it is convenient), we
	define our interface in terms of SlvBackendTokens.
	Any backend can be connected by an appropriate set of
	routines conforming to the headers of slv_*, rel.h, var.h
	and system.h.
	
	We haven't yet been able to think through the possibilities of
	having multiple backends operating _simultaneously_, mainly
	because the ASCEND backend is quite capable of wrapping all
	the other backends we can think of.
*/

typedef struct slv_system_structure *slv_system_t;
/**<
	This is the handle which should be used in all of the functions
	in slv and system to reference a mathematical problem.
*/

/* @} */

#endif  /* ASC_SLV_TYPES_H */

