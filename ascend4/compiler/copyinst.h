/*
 *  Ascend Instance Tree Type Definitions
 *  by Tom Epperly
 *  8/16/89
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: copyinst.h,v $
 *  Date last modified: $Date: 1997/09/08 18:07:35 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#ifndef __COPYINST_H_SEEN__
#define __COPYINST_H_SEEN__
/*
 *  When #including copyinst.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "child.h"
 *         #include "type_desc.h"
 */



enum Copy_enum {
  c_none, c_reference, c_tomodify
};

extern void CheckChildCopies(unsigned long int, struct Instance **);
/*
 * CheckChildCopies(num,clist);
 * Fixes up the num subatomic children sets copied into clist.
 */

extern void RedoChildPointers(unsigned long int, struct Instance *,
                              struct Instance **, CONST struct Instance *,
                              struct Instance * CONST *);
/*
 * RedoChildPointers(num,newparent,newchildptrs, oldparent,oldchildptrs);
 * Fix up the num subatomic child pointers of newparent.
 */

extern struct Instance *ShortCutMakeUniversalInstance(struct TypeDescription*);
/*
 *  struct Instance *ShortCutMakeUniversalInstance(type);
 *  struct TypeDescription *type;
 *
 *  Checks if the given type is universal and if a universal instance
 *  exists. If one does, it will be returned, otherwise this returns NULL.
 *  If the a non-NULL instance is returned, it is the responsibility of
 *  the caller to ensure that the instance *knows* about the caller.
 *  See AddParent for example.
 *  Historical note: This function used to be a part of almost all the
 *  Create*Instance routines, but has been separated, so that those functions
 *  will now *always* create memory.
 */

extern void CollectNodes(struct Instance *,struct gl_list_t *);
/*
 *  CollectNodes(i,l);
 *  Appends i to l, and sets the tmpnum of i to be the length of l
 *  after i was appended. If i does not have a tmpnum (ATOM children)
 *  this function just returns.
 */

extern struct Instance *ShortCutProtoInstance(struct TypeDescription *);
/*
 *  struct Instance *ShortCutProtoInstance(type);
 *  struct TypeDescription *type;
 *
 *  Checks if a prototype exists for the given type definition.
 *  If one does, a copy of a prototype is returned.
 *  If the a non NULL instance is returned, it is the responsibility of
 *  the caller to ensure that the instance *knows* about the caller.
 *  See AddParent for example.
 *  Historical note: This function has been created such that it may be
 *  called separately from CreateInstance. If it were copied for example
 *  then there is no need to redo default statements etc. And so a caller
 *  can treat a copied instance specially.
 */

extern struct Instance *CopyInstance(CONST struct Instance *);
/*
 *  struct Instance *CopyInstance(i)
 *  const struct Instance *i;
 *
 *  1995
 *  This will make a copy of instance i.  i may not be a fundamental
 *  atomic instance.  At the current time, there are the following additional
 *  restrictions on instance i.
 *
 *  1) "i" may not contain instances of universal types
 *  2) all instances in the instance tree "i" must have parent's
 *     only in the instance tree "i".
 *  3) instances in the instance tree "i" may not be ARE_ALIKE'd
 *     to instances outside of instance tree "i".
 *  4) The tree must not have any pending statements, as pendings
 *     and bitlists are basically ignored.
 *
 *  1997 (as left by abbott): 
 *  1) is no longer valid,
 *  2&3) only partially correct,
 *  4) is more important than ever. --baa
 *
 *  This routine is especially good for copying atomic instances, since it is
 *  faster to copy an atomic instance rather than instantiating it.
 */

#endif
/* __COPYINST_H_SEEN__ */
