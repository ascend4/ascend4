/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
	Copyright (C) 2006, 2007 Carnegie Mellon University

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
	Variable clearing routines, for implementation of METHOD ClearAll.
*//*
	by Ben Allan, February 24, 1998
	Last in CVS: $Revision: 1.4 $ $Date: 1998/06/16 16:42:10 $ $Author: mthomas $
*/

#ifndef ASC_ASCFREEALLVARS_H
#define ASC_ASCFREEALLVARS_H

#include <compiler/extfunc.h>
#include <general/list.h>

ASC_DLLSPEC int Asc_ClearVarsInTree(struct Instance *i);
/**<
	A service routine which assumes a solver_var modeling world
	and clears (set var.fixed := FALSE) all var and refinements
	of var in the DAG rooted at i.
*/

ExtMethodRun Asc_FreeAllVars;
/**<
	err = Asc_FreeAllVars(NULL,rootinstance,arglist,0);
	All arguments except rootinstance are ignored.
	rootinstance is used as the argument to a call to Asc_ClearVarsInTree.
*/

#endif /* ASC_ASCFREEALLVARS_H */

