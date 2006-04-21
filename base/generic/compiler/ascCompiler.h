/*	ASCEND modelling environment
	Copyright (C) 1997 Benjamin Andrew Allan
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
*//**
	@file
	Basic Initializations for Ascend

	This module initializes the fundamental data structures used by the rest of
	Ascend and pulls in system headers. Largely this means memory management.

	Requires:
	#include "utilities/ascConfig.h"
*//*
	by Ben Allan
	Version: $Revision: 1.2 $
	Version control file: $RCSfile: ascCompiler.h,v $
	Date last modified: $Date: 1997/07/18 12:27:56 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_ASCCOMPILER_H
#define ASC_ASCCOMPILER_H

#include <utilities/ascConfig.h>

ASC_DLLSPEC(int) Asc_CompilerInit(int simplify_relations);
/**<
 *  Initialize any resources used by the ASCEND compiler.
 *
 *  If this function returns nonzero, ASCEND cannot run and a ton
 *  of memory might be leaked.
 *
 *  The value of simplify_relations sets the initial value of a flag
 *  which tells the compiler to simplify compiled equations or not.
 *  It has no effect on the success or failure of the call.
 *
 *  @BUG At present it needs to more aggressively check the return codes
 *       from the functions this calls. Currently returns 0 regardless.
 */

ASC_DLLSPEC(void ) Asc_CompilerDestroy(void);
/**<
 *  Clean up any resources used by the compiler.
 *  This function should not be called while there are any clients
 *  with pointers to any compiler structures, including gl_lists.
 */

#endif /* ASC_ASCCOMPILER_H */
