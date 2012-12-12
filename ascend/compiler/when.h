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
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  WHEN List Routines.
 */

#ifndef ASC_WHEN_H
#define ASC_WHEN_H

#include "stattypes.h"

/**	@addtogroup compiler_stmt Compiler Statements
	@{
*/

extern struct WhenList *CreateWhen(struct Set *set, struct StatementList *sl);
/**< 
 *  Create a when node.
 *  If set = NULL, this indicates an OTHERWISE case.
 */

extern struct WhenList *ReverseWhenCases(struct WhenList *w);
/**< 
 *  Reverse this list.
 */

extern struct WhenList *LinkWhenCases(struct WhenList *w1, struct WhenList *w2);
/**< 
 *  Link two case lists and return the joined list.  This works best when
 *  w1 is a one element list.
 */


#ifdef NDEBUG
#define NextWhenCase(w) ((w)->next)
#else
#define NextWhenCase(w) NextWhenCaseF(w)
#endif
/**<
 *  Return the next case in the list.
 *  @param w struct WhenList*, the when list to query.
 *  @return The next case as a struct WhenList*.
 *  @see NextWhenCaseF()
 */
extern struct WhenList *NextWhenCaseF(struct WhenList *w);
/**<
 *  Implementation function for NextWhenCase() (debug mode).
 *  Do not call this function directly - use NextWhenCase() instead.
 */

#ifdef NDEBUG
#define WhenSetList(w) ((w)->values)
#else
#define WhenSetList(w) WhenSetListF(w)
#endif
/**<
 *  This will return the set list part of a WhenList structure. When
 *  the set is NULL, this indicates an OTHERWISE case.
 *  @param w struct WhenList*, the when list to query.
 *  @return The set list part as a struct Set*.
 *  @see WhenSetListF()
 */
extern struct Set *WhenSetListF(struct WhenList *w);
/**<
 *  Implementation function for WhenSetList() (debug mode).
 *  Do not call this function directly - use WhenSetList() instead.
 */

#ifdef NDEBUG
#define WhenStatementList(w) ((w)->slist)
#else
#define WhenStatementList(w) WhenStatementListF(w)
#endif
/**<
 *  Return the statement list.
 *  @param w struct WhenList*, the when list to query.
 *  @return The statement list part as a struct StatementList*.
 *  @see WhenStatementListF()
 */
extern struct StatementList *WhenStatementListF(struct WhenList *w);
/**<
 *  Implementation function for WhenStatementList() (debug mode).
 *  Do not call this function directly - use WhenStatementList() instead.
 */

extern void DestroyWhenList(struct WhenList *w);
/**< 
 *  Destroy a whole list.
 */

extern void DestroyWhenNode(struct WhenList *w);
/**< 
 *  Destroy just this node.
 */

extern struct WhenList *CopyWhenNode(struct WhenList *w);
/**< 
 *  Copy a case.  The next attribute is initialized to NULL.
 */

extern struct WhenList *CopyWhenList(struct WhenList *w);
/**< 
 *  Copy the whole list contents. not a reference count change.
 */

/* @} */

#endif  /* ASC_WHEN_H */

