/*
 *  ASCEND comparison functions of various sorts.
 *  by Ben Allen
 *  Created 9/16/96
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: cmpfunc.h,v $
 *  Date last modified: $Date: 1997/07/18 12:28:32 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 */

#ifndef __CMPFUNC_H_SEEN__
#define __CMPFUNC_H_SEEN__

/* requires
# #include "compiler.h"
# #include "instance_enum.h"
and possibly, or CmpIntIndex is undefined,
# #include "instance_types.h"
The problem with instance_types.h is that it requires
pulling in practically the whole compiler headers, so
most clients that only want this for the simpler cmp
functions can use it without that because of a nasty
ifdef down below.
*/

/* reminder strcmp("abc","fcd") --> -1 */
/*
 *  When used with a sort function, these yield a list ordered in
 *  increasing value.
 *  if comparing symbols, use CmpSymchar for maximum speed or
 *  strcmp(SCP(sc1),SCP(sc2));
 */

extern int CmpSymchar(symchar *,symchar *);
/*
 *  Regardless of the details of the symchar definition, returns
 *  the result of alphabetic comparison between two items from the
 *  symbol table.
 *  NULL does not appear in the symbol table,
 *  so it better not be given here.
 *  Not necessarily implemented as strcmp.
 */

extern int CmpPtrs(void *, void *);
/*
 *  CmpPtrs(p1,p2);
 *  returns:	if:
 *  -1		p1<p2
 *   0		p1==p2
 *   1		p1>p2
 */

extern int CmpRealPtrs(void *, void *);
/*
 *  CmpRealPtrs(p1,p2);
 *  First asserts p1, p2 != NULL, then:
 *  returns:	if:
 *  -1		p1<p2
 *   0		p1==p2
 *   1		p1>p2
 */

/*
 *  Comparison functions for instance pointers.
 *  All rather similar, but we may decide to change the comparison
 *  methods later, so each has its own function.
 */

#ifndef NDEBUG
#define CmpParents CmpRealPtrs
#else
#define CmpParents CmpParentsF
#endif
extern int CmpParentsF(CONST struct Instance *,CONST struct Instance *);
/*
 *  Comparison functions for parent instance pointers.
 */

#ifndef NDEBUG
#define CmpRelations CmpRealPtrs
#else
#define CmpRelations CmpRelationsF
#endif
extern int CmpRelationsF(CONST struct Instance *,CONST struct Instance *);
/*
 *  Comparison functions for relation instance pointers.
 */

#ifndef NDEBUG
#define CmpLogRelations CmpRealPtrs
#else
#define CmpLogRelations CmpLogRelationsF
#endif
extern int CmpLogRelationsF(CONST struct Instance *,CONST struct Instance *);
/*
 *  Comparison functions for logrelation instance pointers.
 */

#ifndef NDEBUG
#define CmpWhens CmpRealPtrs
#else
#define CmpWhens CmpWhensF
#endif
extern int CmpWhensF(CONST struct Instance *,CONST struct Instance *);
/*
 *  Comparison functions for when instance pointers.
 */


#ifdef __INSTANCE_TYPES_H_SEEN__

extern int CmpIntIndex(CONST struct ArrayChild *, CONST struct ArrayChild *);
extern int CmpStrIndex(CONST struct ArrayChild *, CONST struct ArrayChild *);
/*
 *  CmpStrIndex(a,b),CmpIntIndex(a,b)
 *  Comparison functions for ArrayChildren.
 *  Both a and b must be nonNULL and are then ASSUMED to be an arraychild
 *  of the type appropriate for the function name.
 * 
 *  This function requires instance_types.h be seen first or it is
 *  invisible.
 */

#endif
/* INSTANCE_TYPES_H_SEEN__ */

/*
 *  When used with a sort function, these yield a list ordered in
 *  decreasing value.
 */

extern int CmpPtrsReverse(void *, void *);
/*
 *  CmpPtrsReverse(p1,p2);
 *  returns:	if:
 *  -1		p1>p2
 *   0		p1==p2
 *   1		p1<p2
 */

#endif
/* __CMPFUNC_H_SEEN__ */
