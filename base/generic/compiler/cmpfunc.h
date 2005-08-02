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
 */

/** @file
 *  ASCEND comparison functions of various sorts.
 *
 *  The problem with instance_types.h is that it requires
 *  pulling in practically the whole compiler headers, so
 *  most clients that only want this for the simpler cmp
 *  functions can use it without that because of a nasty
 *  ifdef down below.
 *
 *  <pre>
 *  When #including cmpfunc.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *  and possibly, or if CmpIntIndex is undefined,
 *         #include "instance_types.h"
 *  </pre>
*/

#ifndef __CMPFUNC_H_SEEN__
#define __CMPFUNC_H_SEEN__

/* reminder strcmp("abc","fcd") --> -1 */
/*
 *  When used with a sort function, these yield a list ordered in
 *  increasing value.
 *  if comparing symbols, use CmpSymchar for maximum speed or
 *  strcmp(SCP(sc1),SCP(sc2));
 */

extern int CmpSymchar(symchar *s1, symchar *s2);
/**<
 *  Compare two strings from the symbol table.
 *  Regardless of the details of the symchar definition, returns
 *  the result of alphabetic comparison between two items from the
 *  symbol table.
 *  NULL does not appear in the symbol table,
 *  so it better not be given here.
 *  Not necessarily implemented as strcmp.
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   s1 <  s2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   s1 == s2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   s1 >  s2  </td></tr></table>
 */

extern int CmpPtrs(void *p1, void *p2);
/**<
 *  <!--  CmpPtrs(p1,p2);                                              -->
 *  <!--  returns:        if:                                          -->
 *  <!--  -1              p1<p2                                        -->
 *  <!--   0              p1==p2                                       -->
 *  <!--   1              p1>p2                                        -->
 *  General pointer comparison function.
 *  The comparison is for the pointers themselves, not what they point at.
 *  Either pointer may be NULL.
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   p1 <  p2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   p1 == p2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   p1 >  p2  </td></tr></table>
 *  @see CmpPtrsReverse()
 */

extern int CmpRealPtrs(void *p1, void *p2);
/**<
 *  <!--  CmpRealPtrs(p1,p2);                                          -->
 *  <!--  First asserts p1, p2 != NULL, then:                          -->
 *  <!--  returns:        if:                                          -->
 *  <!--  -1              p1<p2                                        -->
 *  <!--   0              p1==p2                                       -->
 *  <!--   1              p1>p2                                        -->
 *  Real pointer comparison function.
 *  The comparison is for the pointers themselves, not what they point at.
 *  Neither pointer may be NULL (checked by assertion).
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   p1 <  p2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   p1 == p2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   p1 >  p2  </td></tr></table>
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
/**<
 *  Macro for redirection of CmpParents based on debug mode.
 *  @see CmpRealPtrs(), CmpParentsF()
 */
extern int CmpParentsF(CONST struct Instance *i1, CONST struct Instance *i2);
/**<
 *  <!--  Comparison functions for parent instance pointers.           -->
 *  Parent instance pointer comparison function.
 *  The comparison is for the pointers themselves, not what they point at.
 *  Neither pointer may be NULL (checked by assertion).
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   p1 <  p2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   p1 == p2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   p1 >  p2  </td></tr></table>
 */

#ifndef NDEBUG
#define CmpRelations CmpRealPtrs
#else
#define CmpRelations CmpRelationsF
#endif
/**<
 *  Macro for redirection of CmpRelations based on debug mode.
 *  @see CmpRealPtrs(), CmpRelationsF()
 */
extern int CmpRelationsF(CONST struct Instance *i1, CONST struct Instance *i2);
/**<
 *  <!--  Comparison functions for relation instance pointers.         -->
 *  Relation instance pointer comparison function.
 *  The comparison is for the pointers themselves, not what they point at.
 *  Neither pointer may be NULL (checked by assertion).
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   p1 <  p2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   p1 == p2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   p1 >  p2  </td></tr></table>
 */

#ifndef NDEBUG
#define CmpLogRelations CmpRealPtrs
#else
#define CmpLogRelations CmpLogRelationsF
#endif
/**<
 *  Macro for redirection of CmpLogRelations based on debug mode.
 *  @see CmpRealPtrs(), CmpLogRelationsF()
 */
extern int CmpLogRelationsF(CONST struct Instance *i1, CONST struct Instance *i2);
/**<
 *  <!--  Comparison functions for logrelation instance pointers.      -->
 *  Log relation instance pointer comparison function.
 *  The comparison is for the pointers themselves, not what they point at.
 *  Neither pointer may be NULL (checked by assertion).
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   p1 <  p2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   p1 == p2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   p1 >  p2  </td></tr></table>
 */

#ifndef NDEBUG
#define CmpWhens CmpRealPtrs
#else
#define CmpWhens CmpWhensF
#endif
/**<
 *  Macro for redirection of CmpWhens based on debug mode.
 *  @see CmpRealPtrs(), CmpWhensF()
 */
extern int CmpWhensF(CONST struct Instance *i1, CONST struct Instance *i2);
/**<
 *  <!--  Comparison functions for when instance pointers.             -->
 *  When instance pointer comparison function.
 *  The comparison is for the pointers themselves, not what they point at.
 *  Neither pointer may be NULL (checked by assertion).
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   p1 <  p2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   p1 == p2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   p1 >  p2  </td></tr></table>
 */

#ifdef __INSTANCE_TYPES_H_SEEN__

extern int CmpIntIndex(CONST struct ArrayChild *a, CONST struct ArrayChild *b);
/**<
 *  Compare integer indexes of ArrayChildren a and b.
 *  Both a and b must be non-NULL and are then ASSUMED to be 
 *  indexed by integer.
 *
 *  This function requires instance_types.h be seen first or it is
 *  invisible.
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   a index <  b index  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   a index == b index  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   a index >  b index  </td></tr></table>
 */
extern int CmpStrIndex(CONST struct ArrayChild *a, CONST struct ArrayChild *b);
/**<
 *  <!--  CmpStrIndex(a,b),CmpIntIndex(a,b)                            -->
 *  <!--  Comparison functions for ArrayChildren.                      -->
 *  <!--  Both a and b must be nonNULL and are then ASSUMED to be an ar-->raychild
 *  <!--  of the type appropriate for the function name.               -->
 *  <!--                                                               -->
 *  <!--  This function requires instance_types.h be seen first or it is -->
 *  <!--  invisible.                                                   -->
 *  Compare string indexes of ArrayChildren a and b.
 *  Both a and b must be non-NULL and are then ASSUMED to be
 *  indexed by string.
 *
 *  This function requires instance_types.h be seen first or it is
 *  invisible.
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   a index <  b index  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   a index == b index  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   a index >  b index  </td></tr></table>
 */

#endif  /* INSTANCE_TYPES_H_SEEN__ */

/*
 *  When used with a sort function, these yield a list ordered in
 *  decreasing value.
 */

extern int CmpPtrsReverse(void *p1, void *p2);
/**<
 *  <!--  CmpPtrsReverse(p1,p2);                                       -->
 *  <!--  returns:        if:                                          -->
 *  <!--  -1              p1>p2                                        -->
 *  <!--   0              p1==p2                                       -->
 *  <!--   1              p1<p2                                        -->
 *  General pointer comparison function with reverse ordering.
 *  The comparison is for the pointers themselves, not what they point at.
 *  Either pointer may be NULL.
 *  @return <table><tr align="center"><td> Result </td><td>  Condition   </td></tr>
 *                 <tr align="center"><td>   -1   </td><td>   p1 >  p2  </td></tr>
 *                 <tr align="center"><td>    0   </td><td>   p1 == p2  </td></tr>
 *                 <tr align="center"><td>    1   </td><td>   p1 <  p2  </td></tr></table>
 *  @see CmpPtrs()
 */

#endif  /* __CMPFUNC_H_SEEN__ */

