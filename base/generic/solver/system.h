/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

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
	System routines for the SLV solver.

	This is the ascend version of the system module.
	This provides a means of extracting a solver system
	from an ASCEND instance tree.
	The result is supposed to appear totally instance
	independent, though it is not really, so that
	only the var and rel modules need to know about
	the ascend compiler.
	With any luck the expr, exprman, and relman files leave.

	04/94 - original version by Joe Zaher
	06/96 - total reimplementation by Ben Allan

	Requires:
	#include "utilities/ascConfig.h"
	#include "slv_types.h"
*//*
	by Karl Michael Westerberg and Joseph Zaher
	Created: 2/6/90
	Version: $Revision: 1.8 $
	Version control file: $RCSfile: system.h,v $
	Date last modified: $Date: 1997/07/18 12:17:26 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_SYSTEM_H
#define ASC_SYSTEM_H

#include <utilities/ascConfig.h>

extern ASC_DLLSPEC(slv_system_t) system_build(SlvBackendToken inst);
/**<
 ***  In ascend, backendtoken is a struct Instance *.
 ***  Visits the instance tree and collects all variables,
 ***  relations, and objective functions into pointer lists.
 **/

extern void system_destroy(slv_system_t sys);
/**<
 ***  <!--  system_destroy(sys)                                        -->
 ***  <!--  slv_system_t sys;                                          -->
 ***
 ***  Destroys the latest model formulation.
 **/

extern void system_free_reused_mem(void);
/**<
 ***  <!--  system_free_reused_mem()                                   -->
 ***
 ***  Deallocates any memory that solvers may be squirrelling away for
 ***  internal reuse. Calling this while any slv_system_t exists
 ***  is likely to be fatal: handle with care.
 **/

#endif  /* ASC_SYSTEM_H */

