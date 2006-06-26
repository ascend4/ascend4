/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1995 Kirk Andre Abbott

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
	Special stuff for handling external relations in the instance hierarchy.

	I think that this means that we are keeping a list of all those variables
	in our systems that may be accessed by external methods and functions, so
	that we can use a simplified API to allow get/set. -- JP

	@TODO add documentation! what this is all about?

	@TODO why do we need another $@%! global variable for all this whatsit?

	Requires
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
*//*
	by Tom Epperly
	8/16/89
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/18 12:29:35 $ $Author: mthomas $
*/

#ifndef ASC_EXTINST_H
#define ASC_EXTINST_H

extern struct Instance **g_ExtVariablesTable;
/**<
	A global variable which is non NULL if external relations have been
	processed and have hence added variables to the table. After use it
	should be appropriately reset.
*/

extern struct Instance **AddVarToTable(struct Instance *inst, int *added);
/**<
	Given an instance will store it in the ExtVariablesTable and will return
	the 'handle' to the instance. If variable existed already, it will not
	be added. If there was a failure then the variable will not be added.
	This is reflected in the variable "added".
*/

extern void FixExternalVars(struct Instance *old, struct Instance *new);
/**< 
	Replaces old with new in the table.
	This will be called only for MODEL_INSTS.

	Used by MergeModels, RefineModels, apparently -- JP.
*/

extern void SetSimulationExtVars(struct Instance *i, struct Instance **extvars);
/**< 
	Will set the given extvar table to the instance. Not for the casual user !!
	Could not avoid exporting this one. I would rather have not.
	However instantiate needs to be able to set this table when finished.
*/

#endif  /* ASC_EXTINST_H */

