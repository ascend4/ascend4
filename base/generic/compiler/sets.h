/*
 *  Set routine external definitions
 *  by Tom Epperly
 *  July 31, 1989
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: sets.h,v $
 *  Date last modified: $Date: 1998/01/06 12:05:34 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/** @file
 *  Set routine external definitions.
 *  <pre>
 *  When #including sets.h, make sure these files are #included first:
 *         #include "dimen.h"
 *  </pre>
 */

#ifndef ASC_SETS_H
#define ASC_SETS_H

#include <utilities/ascConfig.h>
#include <general/list.h>

#include "fractions.h"
#include "compiler.h"
#include "expr_types.h"

/**	@addtogroup compiler Compiler
	@{
*/

extern struct Set *CreateSingleSet(struct Expr *ex);
/**< 
 *  Create a set node of type single with ex as its expression.
 */

extern struct Set *CreateRangeSet(struct Expr *lower, struct Expr *upper);
/**< 
 *  Create a set node of type range with lower as its lower bound and
 *  upper as its upper bound.
 */

extern void LinkSets(struct Set *cur, struct Set *next);
/**< 
 *  Sets the next field of cur to next.
 */

#ifdef NDEBUG
#define NextSet(s) ((s)->next)
#else
#define NextSet(s) NextSetF(s)
#endif
/**<
 *  Return the next set. (if in an expression of Set)
 *  @param s CONST struct Set*, the set to query.
 *  @return The next set as a struct Set*.
 *  @see NextSetF()
 */
extern struct Set *NextSetF(CONST struct Set *s);
/**<
 *  Implementation function for NextSet().  Do not call this
 *  function directly - use NextSet() instead.
 */

#ifdef NDEBUG
#define SetType(s) ((s)->range)
#else
#define SetType(s) SetTypeF(s)
#endif
/**<
 *  Return the type of the set. Single expression or range (..).
 *  @param s CONST struct Set*, the set to query.
 *  @return The type as an int.
 *  @see SetTypeF()
 */
extern int SetTypeF(CONST struct Set *s);
/**<
 *  Implementation function for SetType().  Do not call this
 *  function directly - use SetType() instead.
 */

#ifdef NDEBUG
#define GetSingleExpr(s) ((s)->val.e)
#else
#define GetSingleExpr(s) GetSingleExprF(s)
#endif
/**<
 *  Returns the expression for the single value.
 *  Assumes that s is not a range.
 *  @param s CONST struct Set*, the set to query.
 *  @return The expression as an CONST struct Expr*.
 *  @see GetSingleExprF()
 */
extern CONST struct Expr *GetSingleExprF(CONST struct Set *s);
/**<
 *  Implementation function for GetSingleExpr().  Do not call this
 *  function directly - use GetSingleExpr() instead.
 */

#ifdef NDEBUG
#define GetLowerExpr(s) ((s)->val.r.lower)
#else
#define GetLowerExpr(s) GetLowerExprF(s)
#endif
/**<
 *  Returns the lower value expression.  Assumes that s is a range.
 *  @param s CONST struct Set*, the set to query.
 *  @return The expression as an CONST struct Expr*.
 *  @see GetLowerExprF()
 */
extern CONST struct Expr *GetLowerExprF(CONST struct Set *s);
/**<
 *  Implementation function for GetLowerExpr().  Do not call this
 *  function directly - use GetLowerExpr() instead.
 */

#ifdef NDEBUG
#define GetUpperExpr(s) ((s)->val.r.upper)
#else
#define GetUpperExpr(s) GetUpperExprF(s)
#endif
/**<
 *  Returns the upper value expression.  Assumes that s is a range.  
 *  @param s CONST struct Set*, the set to query.
 *  @return The expression as an CONST struct Expr*.
 *  @see GetUpperExprF()
 */
extern CONST struct Expr *GetUpperExprF(CONST struct Set *s);
/**<
 *  Implementation function for GetUpperExpr().  Do not call this
 *  function directly - use GetUpperExpr() instead.
 */

extern struct Set *CopySetNode(CONST struct Set *s);
/**< 
 *  Return a copy of s, but not anything pointed at by s->next.
 *  copy->next = NULL.
 *  Handles NULL input gracefully, returning NULL.
 */

extern struct Set *CopySetList(CONST struct Set *s);
/**< 
 *  Return a copy of s.
 *  Handles NULL input gracefully.
 */

extern void DestroySetNode(struct Set *s);
/**<
 *  Destroys Set node given and the expression contents of the sets.
 *  Pays not the slightest attention to the ref_count B.S.
 */

extern void DestroySetList(struct Set *s);
/**< 
 *  Destroys Set chain given and the expression contents of the sets.
 *  Handles NULL input gracefully.
 *  Pays not the slightest attention to the ref_count B.S.
 */

extern void DestroySetHead(struct Set *s);
/**< 
 *  Destroys Set node given but not the expression contents of the node.
 *  Normally should not be used.
 */

extern struct Set *CopySetByReference(struct Set *s);
/**< 
 *  Increase ref_count.
 */

extern void DestroySetListByReference(struct Set *s);
/**< 
 *  Decrements the reference count and destroys the elements 
 *  in the set if the count is zero.
 */

extern void DestroySetNodeByReference(struct Set *s);
/**< 
 *  Decrease ref_count.
 */

extern struct Set *JoinSetLists(struct Set *s1, struct Set *s2);
/**< 
 *  Append list s2 to the end of s1.  Returns the pointer to the joined list.
 *  If s1 is NULL, it simply returns s2.
 */

extern struct Set *ReverseSetList(struct Set *s);
/**< 
 *  Reverse the order of the set list.
 */

extern int SetStructuresEqual(CONST struct Set *s1, CONST struct Set *s2);
/**< 
 *  Return TRUE if and only if, s1 and s2 are structurally equivalent.
 */

extern int CompareSetStructures(CONST struct Set *s1, CONST struct Set *s2);
/**< 
 *  Returns -1,0,1 as s1 < = > s2.
 *  compares on length and on content value.
 *  a range > an expression
 *  a NULL Set * > all Sets.
 */

extern unsigned long SetLength(CONST struct Set *s);
/**< 
 *  Returns the number of elements in the set given.
 */

extern struct gl_list_t *SetNameList(CONST struct Set *s);
/**< 
 *  Returns a list containing all the names found in the Set given.
 *  The names in the list belong to the set, so you can destroy
 *  just the list when you are done with it. The list may be
 *  empty. It should never be NULL.
 */

extern char *CreateStrFromSet(CONST struct Set *set);
/**< 
 *  Returns a copy of the string representation of the given set.
 */

extern void sets_init_pool(void);
/**<
 *  Starts memory recycle. Do not call twice before stopping recycle.
 */

extern void sets_destroy_pool(void);
/**< 
 *  Stops memory recycle. Do not call while ANY Expr are outstanding.
 */

extern void sets_report_pool(void);
/**< 
 *  Write the pool report to ASCERR for the sets pool.
 */

/* @} */

#endif  /* ASC_SETS_H */

