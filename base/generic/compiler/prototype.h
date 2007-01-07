/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
	This module stores a copy of atom instances.

	Given the amount of time we spend looking for prototypes, we need
	to be running distinct prototype libraries for models, atoms, and
	possibly constants.

	Require:
	#include "utilities/ascConfig.h"
	#include"instance_enum.h"
	#include "compiler.h"
*//*
	by Tom Epperly
	Version: $Revision: 1.7 $
	Version control file: $RCSfile: prototype.h,v $
	Date last modified: $Date: 1998/02/05 16:37:30 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_PROTOTYPE_H
#define ASC_PROTOTYPE_H

#include <utilities/ascConfig.h>

extern void InitializePrototype(void);
/**<  Must be called before any other prototype procedure. */

ASC_DLLSPEC struct Instance *LookupPrototype(symchar *t);
/**<
 *  Check if an instance of type "t" is in the prototype library.  If
 *  no instance of that type exists, NULL is returned.
 *  t is from symbol table.
 */

extern void DeletePrototype(symchar *t);
/**<
 *  Delete the type t from the prototype library.  This should be done
 *  when the definition of type "t" is changed or when the definition of
 *  an ancestor of type "t" is change.
 */

ASC_DLLSPEC void AddPrototype(struct Instance *i);
/**<
 *  This will add instance i to the prototype library.  If another definition
 *  of type "i" exists, it is deleted and replace with then new one.
 */

ASC_DLLSPEC void DestroyPrototype(void);
/**<
 *  This deletes all the instances in the prototype library.  This should
 *  be done before the program exits.
 */

#endif /* ASC_PROTOTYPE_H */

