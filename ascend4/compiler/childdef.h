/*
 *  Ascend Atom Child Definition Module
 *  by Benjamin A Allan
 *  Created: 11/20/96
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: childdef.h,v $
 *  Date last modified: $Date: 1998/02/05 16:35:37 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/*
 *  When #including typedef.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *         #include "module.h"
 *         #include "list.h"
 *         #include "slist.h"
 *         #include "dimen.h"
 *         #include "child.h"
 *         #include "type_desc.h"
 */
#ifndef __CHILDDEF_H_SEEN__
#define __CHILDDEF_H_SEEN__

/*
 * There are 5 allowable ATOM children types, at present.
 * The functions BaseType and GetTypeInfoFromISA rely on a table
 * and the ordering within it. This is a hack to restrict the types
 * allowed as ATOM children.
 */
#define NUM_FUNDTYPES 5

extern int BaseType(symchar *);
/*
 * int BaseType(name)
 * symchar * name;
 * Returns the number of the fundamental type, or -1 if not fundamental.
 * The code here is *VERY* dependent upon the number and position
 * of the FundamentalTypeList (internal to childdef.c)!
 * Note that the fundamentalTypeList should be refilled with
 * pointers from the symbol table and then comparisons done
 * by ptr rather than strcmp. But that comes after we get a
 * real symbol table.
 */

extern struct ChildDesc *MakeChildDesc(symchar *,
                                       struct StatementList *,
                                       ChildListPtr);
/*
 * struct ChildDesc *childd = MakeChildDesc(name,sl,clist);
 * symchar *name;
 * struct StatementList *sl;
 * ChildListPtr clist;
 * Returns the child description array based on legal statements in sl.
 */

extern unsigned long CalcByteSize(enum type_kind,
                                  ChildListPtr,
             struct ChildDesc *);
/*
 * len =  CalcByteSize(t,clist,childd);
 * enum type_kind t;
 * ChildListPtr clist;
 * struct ChildDesc *childd;
 * unsigned long len;
 * Calculates the byte size of an atomic instance given its
 * child information
 */

#endif
/* __CHILDDEF_H_SEEN__ */
