/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1998 Carnegie Mellon University

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
	Anonymous type manipulation

	Requires
	#include "utilities/ascConfig.h"
	#include "list.h"
	#include "instance_enum.h"
*//*
	by Benjamin Allan
	September 08, 1997
	Last in CVS $Revision: 1.2 $ $Date: 1998/06/16 16:38:35 $ $Author: mthomas $
*/

#ifndef ASC_ANONCOPY_H
#define ASC_ANONCOPY_H

extern struct gl_list_t *Pass2CollectAnonProtoVars(struct Instance *i);
/**<
	Returns a gl_list of index paths through i to reach the vars
	occurring in relations (or relation arrays) of i.
	i must be a MODEL.

	Each var will only occur once in the path list returned.
	An index path can be followed through any instance isomorphic to i
	and will end at a variable semantically equivalent to the one
	in i that generated the path.

	At the expense of a visit tree call (which we need anyway)
	this returns the list unsorted and never searched.<br><br>
	
	The list returned should be destroyed with
	Pass2DestroyAnonProtoVars().
*/

extern void Pass2DestroyAnonProtoVars(struct gl_list_t *indexpathlist);
/**<
	Deallocate the indexpathlist collected by Pass2CollectAnonProtoVars().
*/

extern void Pass2CopyAnonProto(struct Instance *proto,
                               struct BitList *protoblist,
                               struct gl_list_t *protovarindices,
                               struct Instance *i);
/**<
	Copies all the local relations (including those in arrays)
	of the MODEL instance proto to the instance i using only
	local information. No global information is needed, but
	we need to arrange that the tmpnums all start and end 0
	so we can avoid extra 0ing of them.
*/

#endif /* ASC_ANONCOPY_H */

