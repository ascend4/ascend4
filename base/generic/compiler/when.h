/*
 *  WHEN List Routines
 *  by Vicente Rico-Ramirez
 *  7/96
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: when.h,v $
 *  Date last modified: $Date: 1997/07/18 12:36:46 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Vicente Rico-Ramirez
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
#ifndef __WHEN_H_SEEN__
#define __WHEN_H_SEEN__
/* requires
# #include"compiler.h"
# #include"sets.h"
# #include"exprs.h"
# #include"bit.h"
# #include"stattypes.h"
*/

extern struct WhenList *CreateWhen(struct Set *,struct StatementList *);
/*
 *  struct WhenList *CreateWhen(set,sl)
 *  struct Set *set;
 *  struct StatementList *sl;
 *  Create a when node.
 *  If set = NULL, this indicates an OTHERWISE case.
 */

extern struct WhenList *ReverseWhenCases(struct WhenList *);
/*
 *  struct WhenList *ReverseWhenCases(w)
 *  struct WhenList *w;
 *  Reverse this list.
 */

extern struct WhenList *LinkWhenCases(struct WhenList *,struct WhenList *);
/*
 *  struct WhenList *LinkWhenCases(w1,w2)
 *  struct WhenList *w1,*w2;
 *  Link two case lists and return the joined list.  This works best when
 *  w1 is a one element list.
 */


#ifdef NDEBUG
#define NextWhenCase(w) ((w)->next)
#else
#define NextWhenCase(w) NextWhenCaseF(w)
#endif
extern struct WhenList *NextWhenCaseF(struct WhenList *);
/*
 *  macro NextWhenCase(case)
 *  struct WhenList *NextWhenCaseF(case)
 *  struct WhenList *case;
 *  Return the next case.
 */

#ifdef NDEBUG
#define WhenSetList(w) ((w)->values)
#else
#define WhenSetList(w) WhenSetListF(w)
#endif
extern struct Set *WhenSetListF(struct WhenList *);
/*
 *  macro WhenSetList(w)
 *  struct Set *WhenSetListF(w)
 *  const struct WhenList *w;
 *  This will return the set list part of a WhenList structure. When
 *  the set is NULL, this indicates an OTHERWISE case.
 */

#ifdef NDEBUG
#define WhenStatementList(w) ((w)->slist)
#else
#define WhenStatementList(w) WhenStatementListF(w)
#endif
extern struct StatementList *WhenStatementListF(struct WhenList *);
/*
 *  macro WhenStatementList(w)
 *  const struct StatementList *WhenStatementListF(w)
 *  const struct WhenList *w;
 *  Return the statement list.
 */

extern void DestroyWhenList(struct WhenList *);
/*
 *  void DestroyWhenList(w)
 *  struct WhenList *w;
 *  Destroy a whole list.
 */

extern void DestroyWhenNode(struct WhenList *);
/*
 *  void DestroyWhenNode(w)
 *  struct WhenList *w;
 *  Destroy just this node.
 */

extern struct WhenList *CopyWhenNode(struct WhenList *);
/*
 *  struct WhenList *CopyWhenNode(w)
 *  struct WhenList *w;
 *  Copy a case.  The next attribute is initialized to NULL.
 */

extern struct WhenList *CopyWhenList(struct WhenList *);
/*
 *  struct WhenList *CopyWhenList(w)
 *  struct WhenList *w;
 *  Copy the whole list contents. not a reference count change.
 */
#endif /* __WHEN_H_SEEN__ */



