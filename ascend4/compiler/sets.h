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

/*
 *  When #including sets.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 */


#ifndef __SETS_H_SEEN__
#define __SETS_H_SEEN__
/* requires
# #include"compiler.h"
# #include"types.h"
# #include"list.h"
*/

extern struct Set *CreateSingleSet(struct Expr *);
/*
 *  struct Set *CreateSingleSet(ex)
 *  struct Expr *ex;
 *  Create a set node of type single with ex as its expression.
 */

extern struct Set *CreateRangeSet(struct Expr *,struct Expr *);
/*
 *  struct Set *CreateRangeSet(lower,upper)
 *  struct Expr *lower,*upper;
 *  Create a set node of type range with lower as its lower bound and
 *  upper as its upper bound.
 */

extern void LinkSets(struct Set *,struct Set *);
/*
 *  void LinkSets(cur,next)
 *  struct Set *cur, *next;
 *  Sets the next field of cur to next.
 */

#ifdef NDEBUG
#define NextSet(s) ((s)->next)
#else
#define NextSet(s) NextSetF(s)
#endif
extern struct Set *NextSetF(CONST struct Set *);
/*
 *  macro NextSet(s)
 *  struct Set *NextSetF(s)
 *  const struct Set *s;
 *  Return the next set. (if in an expression of Set)
 */

#ifdef NDEBUG
#define SetType(s) ((s)->range)
#else
#define SetType(s) SetTypeF(s)
#endif
extern int SetTypeF(CONST struct Set *);
/*
 *  macro SetType(s)
 *  const int SetTypeF(s)
 *  struct Set *s;
 *  return the type of the set. (single expression or range (..))
 */

#ifdef NDEBUG
#define GetSingleExpr(s) ((s)->val.e)
#else
#define GetSingleExpr(s) GetSingleExprF(s)
#endif
extern CONST struct Expr *GetSingleExprF(CONST struct Set *);
/*
 *  macro GetSingleExpr(s)
 *  const struct Expr *GetSingleExprF(s)
 *  const struct Set *s;
 *  Assumes that s is not a range.  Returns the expression for the single
 *  value.
 */

#ifdef NDEBUG
#define GetLowerExpr(s) ((s)->val.r.lower)
#else
#define GetLowerExpr(s) GetLowerExprF(s)
#endif
extern CONST struct Expr *GetLowerExprF(CONST struct Set *);
/*
 *  macro GetLowerExpr(s)
 *  const struct Expr *GetLowerExprF(s)
 *  const struct Set *s;
 *  Assumes that s is a range.  Returns the lower value expression.
 */

#ifdef NDEBUG
#define GetUpperExpr(s) ((s)->val.r.upper)
#else
#define GetUpperExpr(s) GetUpperExprF(s)
#endif
extern CONST struct Expr *GetUpperExprF(CONST struct Set *);
/*
 *  macro GetUpperExpr(s)
 *  const struct Expr *GetUpperExprF(s)
 *  const struct Set *s;
 *  Assumes that s is a range.  Returns the upper value expression.
 */

extern struct Set *CopySetNode(CONST struct Set *);
/*
 *  struct Set *CopySetNode(s)
 *  const struct Set *s;
 *  Return a copy of s, but not anything pointed at by s->next.
 *  copy->next = NULL.
 *  Handles NULL input gracefully, returning NULL.
 */

extern struct Set *CopySetList(CONST struct Set *);
/*
 *  struct Set *CopySetList(s)
 *  const struct Set *s;
 *  Return a copy of s.
 *  Handles NULL input gracefully.
 */

extern void DestroySetNode(struct Set *);
/*
 *  void DestroySetNode(s)
 *  Destroys Set node given and the expression contents of the sets.
 *  struct Set *s;
 *  Pays not the slightest attention to the ref_count B.S.
 */

extern void DestroySetList(struct Set *);
/*
 *  void DestroySetList(s)
 *  struct Set *s;
 *  Destroys Set chain given and the expression contents of the sets.
 *  Handles NULL input gracefully.
 *  Pays not the slightest attention to the ref_count B.S.
 */

extern void DestroySetHead(struct Set *);
/*
 *  void DestroySetHead(s)
 *  Destroys Set node given but not the expression contents of the node.
 *  struct Set *s;
 *  Normally should not be used.
 */

extern struct Set *CopySetByReference(struct Set *);
/*
 *  struct Set *CopySetByReference(s)
 *  const struct Set *s;
 *  increase ref_count.
 */

extern void DestroySetListByReference(struct Set *);
/*
 *  void DestroySetListByReference(s)
 *  struct Set *s;
 */

extern void DestroySetNodeByReference(struct Set *);
/*
 *  void DestroySetNodeByReference(s)
 *  struct Set *s;
 *  decrease ref_count;
 */

extern struct Set *JoinSetLists(struct Set *,struct Set *);
/*
 *  void JoinSetLists(s1,s2)
 *  struct Set *s1, *s2;
 *  Append list s2 to the end of s1.  Returns the pointer to the joined list.
 *  If s1 is NULL, it simply returns s2.
 */

extern struct Set *ReverseSetList(struct Set *);
/*
 *  struct Set *ReverseSetList(s)
 *  struct Set *s;
 *  Reverse the order of the set list.
 */

extern int SetStructuresEqual(CONST struct Set *,CONST struct Set *);
/*
 *  int SetStructuresEqual(s1,s2)
 *  const struct Set *s1,*s2;
 *  Return TRUE if and only if, s1 and s2 are structurally equivalent.
 */

extern int CompareSetStructures(CONST struct Set *,CONST struct Set *);
/*
 *  int CompareSetStructures(s1,s2)
 *  const struct Set *s1,*s2;
 *  Returns -1,0,1 as s1 < = > s2.
 *  compares on length and on content value.
 *  a range > an expression
 *  a NULL Set * > all Sets.
 */

extern unsigned long SetLength(CONST struct Set *);
/*
 *  Returns the number of elements in the set given.
 */

extern struct gl_list_t *SetNameList(CONST struct Set *);
/*
 *  Returns a list containing all the names found in the Set given.
 *  The names in the list belong to the set, so you can destroy
 *  just the list when you are done with it. The list may be
 *  empty. It should never be NULL.
 */

extern char *CreateStrFromSet(CONST struct Set *);
/*
 *  char *CreateStrFromSet(set);
 *  const struct Set *set;
 *  Returns a copy of the string representation of the given set.
 */


extern void sets_init_pool(void);
/*
 * starts memory recycle. do not call twice before stopping recycle.
 */

extern void sets_destroy_pool(void);
/*
 * stops memory recycle. do not call while ANY Expr are outstanding.
 */

extern void sets_report_pool(void);
/*
 * write the pool report to ASCERR for the sets pool.
 */

#endif /* __SETS_H_SEEN__ */
