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
	Instance Garbage Dump

	This is a resting place for un-needed instances.  They may be stored in
	the dump in case they are needed later on.  Only pure instance should
	be stored in the dump, and the dumping procedure is responsible for
	making sure that the instance it "pure", and unconnected to anything
	else.  Typically, these restrictions are only met by atoms.

	Requires:
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
	#include "compiler.h"
*//*
	by Tom Epperly
	10/24/89
	Version: $Revision: 1.7 $
	Version control file: $RCSfile: dump.h,v $
	Date last modified: $Date: 1998/02/05 16:35:51 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_DUMP_H
#define ASC_DUMP_H

#include <utilities/ascConfig.h>

#define MESSYTHRESH 10

extern void InitDump(void);
/**<
 *  Must be called to initialize the dump.
 */

extern ASC_DLLSPEC(void) EmptyTrash(void);
/**<
 *  Delete all the instances in the dump.  The dump can still be used
 *  after this call.
 */

extern void TendTrash(void);
/**<
 *  This is a less drastic version of EmptyTrash(), which only deletes
 *  instance with too many copies in the dump.  Too many is more than
 *  MESSYTHRES copies of a given type.
 */

extern void TrashType(symchar *str);
/**<
 *  Delete any copies of type 'str' from the dump.  This should be called
 *  if for instance the definition of 'str' is being changed.  In such
 *  cases, fetching copies of 'str' from the dump would return the old
 *  version.<br><br>
 *
 *  str should have no leading blanks and should be a valid identifier.
 */

extern void AddInstance(struct Instance *i);
/**<
 *  This adds instance i to the trash dump.
 */

extern struct Instance *FindInstance(symchar *str);
/**<
 *  This will return an instance if it one is available; otherwise, it
 *  returns NULL.
 */

#endif /* ASC_DUMP_H */

