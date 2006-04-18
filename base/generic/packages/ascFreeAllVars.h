/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
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
	Variable clearing routines.

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler/instance_enum.h"
	#include "compiler/compiler.h"
	#include "general/list.h"
	#include "compiler/extfunc.h"
*//*
	by Ben Allan
	February 24, 1998
	Part of ASCEND
	Version: $Revision: 1.4 $
	Version control file: $RCSfile: ascFreeAllVars.h,v $
	Date last modified: $Date: 1998/06/16 16:42:10 $
	Last modified by: $Author: mthomas $
*/

#ifndef ASC_ASCFREEALLVARS_H
#define ASC_ASCFREEALLVARS_H

#include <utilities/ascConfig.h>

ASC_DLLSPEC(int) Asc_ClearVarsInTree(struct Instance *i);
/**< Asc_ClearVarsInTree(i).
 * A service routine which assumes a solver_var modeling world
 * and clears (set var.fixed := FALSE) all var and refinements
 * of var in the DAG rooted at i.
 */

ASC_DLLSPEC(int) Asc_FreeAllVars( struct Instance *rootinstance,
                                    struct gl_list_t *arglist);
/**<
 *  err = Asc_FreeAllVars(NULL,rootinstance,arglist,0);
 *  All arguments except rootinstance are ignored.
 *  rootinstance is used as the argument to a call to Asc_ClearVarsInTree.
 *  This wrapper exists only for old EXTERNAL a la abbott compatibility
 *  and should be trashed ASAP.
 */

#endif /* ASC_ASCFREEALLVARS_H */

