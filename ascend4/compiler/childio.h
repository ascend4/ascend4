/*
 *  Model Child list output routines
 *  by Ben Allan
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: childio.h,v $
 *  Date last modified: $Date: 1998/06/11 17:36:23 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
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
 *
 *  This is a package of routines to process child list io.
 */

/*
 *  When #including child.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 */


#ifndef __CHILDIO_H_SEEN__
#define __CHILDIO_H_SEEN__
/* requires
 *include"compiler.h"
 *include"list.h"
 *include"child.h"
 */

/*
 *  WriteChildList(fp,cl)
 *  Write what is known at parse time about the children in the child list
 *  given.  What is known may be surprising. It may be only mildly
 *  accurate.
 */
extern void WriteChildList(FILE *,ChildListPtr);

/*
 * s = WriteChildDetails(cl,n);
 * Return a string containing buckets o'stuff about the nth child in list.
 * The string will make use of braces as necessary to delimit
 * items. What each item is will be subject to change according
 * to the meta-data given by WriteChildMetaDetails.
 * Items are booleans, integers or strings as explained below.
 *
 * The string returned is the caller's responsibility.
 */
extern char *WriteChildDetails(ChildListPtr,unsigned long);

/*
 * metas = WriteChildMetaDetails();
 * Returns a string with fields brace delimited. Each field
 * describes the corresponding field of a WriteChildDetails
 * return string. The ordering and size may be expected to shift as
 * ASCEND evolves. The hope is that the contents of metas individual
 * fields will shift much more slowly than the ordering, number of
 * fields and so forth. Using metas, one can expect to write code
 * which survives changes in plain s.
 * 
 * The returned string is NOT yours to free. (safe to keep, though).
 * The format is one or more elements in braces like:
 * "{data} {data} {data}"
 * where data is a triplet separated by - "name-ctype-{explanation}"
 * Name is a single string with no blanks,
 * ctype is boolean, integer, or string.
 * explanation is for human consumption and explains the field.
 */
extern CONST char *WriteChildMetaDetails(void);

/*
 * WriteChildMissing(fp,scope,childname);
 * FILE *fp;
 * char *scope;
 * symchar *childname;
 * Issues a child missing error to file if
 * the same childname/scope has not been missing since the last call
 * with any NULL argument.
 */
extern void WriteChildMissing(FILE *, char *, symchar *);
#endif /* __CHILDIO_H_SEEN__ */
