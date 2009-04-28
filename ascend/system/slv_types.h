/*	ASCEND modelling environment
	Copyright (C) 1996 Benjamin Andrew Allan
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
*//** @file
	Type definitions as they should be seen by solver engines.

	We're trying to hide the implementation detail from the solver engines,
	to force them to use the API defined in slv_common.h, slv_client.h, etc.
*//*
	by Benjamin Andrew Allan, created 6/1/96.
	Last in CVS: $Revision: 1.4 $ $Date: 1997/07/18 12:17:21 $ $Author: mthomas $
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

typedef struct system_structure *slv_system_t;
/**<
	This is the handle which should be used in all of the functions
	in slv and system to reference a mathematical problem. The details of this
	structure are in system_impl.h, which you should only include if you 
	are writing code in the 'system' dir.
*/

/* @} */

#endif  /* ASC_SLV_TYPES_H */

