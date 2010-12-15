/*  ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 2009 Carnegie Mellon University

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
	Type Description Output.
*//*
	by Tom Epperly, 1/15/89
	Last in CVS $Revision: 1.9 $ $Date: 1998/03/26 20:40:28 $ $Author: ballan $
*/

#ifndef ASC_TYPE_DESCIO_H
#define ASC_TYPE_DESCIO_H

#include <stdio.h>

#include <ascend/general/platform.h>

#include "type_desc.h"

/**	@addtogroup compiler_type Compiler Type Description
	@{
*/

ASC_DLLSPEC void WriteDefinition(FILE *f, struct TypeDescription *desc);
/**<
	Write the type description structure to the given file in text.
	May include compiler derived information in comments.
*/

ASC_DLLSPEC void WriteDiffDefinition(FILE *f, struct TypeDescription *desc);
/**< 
	Write the type description structure to the given file in text but
	only those statements that are in the declarative section which are
	different from the refinement ancestor of the type. The procedures
	are not dealt with, as that is messy. If no ancestor, defaults to
	writing all declarative statements.

	Note that the parameters, WHEREs, reductions and absorbed
	statements of desc are *not* written.
*/

extern symchar *GetBaseTypeName(enum type_kind t);
/**< 
	Returns the symbol for the kind of type given.
	InitBaseTypeNames must have been called first.
	ascCompiler takes care of that.
*/

extern void InitBaseTypeNames(void);
/**< 
	Set up the basetypes symbol table.
*/

/* @} */

#endif /* ASC_TYPE_DESCIO_H */
