/*	ASCEND modelling environment
	Copyright (C) 2006 Benjamin Andrew Allan
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
 *  Ascend Instance Tree Name Search Routines.
*//*
	by Ben Allan
	Created: 10/3/2006
*/

#ifndef ASC_FINDPATH_H
#define ASC_FINDPATH_H

#include <ascend/general/list.h>
#include "instance_enum.h"
#include "find.h"

/**	@addtogroup compiler_inst Compiler Instance Hierarchy
	@{
*/

extern struct gl_list_t *FindInstancesPaths(CONST struct Instance *i,
                                       CONST struct Name *n,
                                       enum find_errors *err);
/**<
 *  Return the list of Names expanded from n that have instances.
 *  Don't forget to destroy the names in the list before destroying list.
 *  If this returns NULL,
 *  it indicates that it couldn't find something in the name.
 *  Check err to discover why.
 */

/* @} */

#endif /* ASC_FINDPATH_H */

