/**< 
 *  SELECT List Routines
 *  by Vicente Rico-Ramirez
 *  1/97
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: select.h,v $
 *  Date last modified: $Date: 1997/07/29 15:52:55 $
 *  Last modified by: $Author: rv2a $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#ifndef __SELECT_H_SEEN__
#define __SELECT_H_SEEN__

/**< requires
# #include"compiler.h"
# #include"sets.h"
# #include"exprs.h"
# #include"bit.h"
# #include"stattypes.h"
*/

extern struct SelectList *CreateSelect(struct Set *,struct StatementList *);
/**< 
 *  struct SelectList *CreateSelect(set,sl)
 *  struct Set *set;
 *  struct StatementList *sl;
 *  Create a Select node.  This will examine the set data structure which
 *  must contain only constant expressions (TRUE or FALSE), symbol values or
 *  integer values.
 *  If set = NULL, this indicates an OTHERWISE case.
 */

extern struct SelectList *ReverseSelectCases(struct SelectList *);
/**< 
 *  struct SelectList *ReverseSelectCases(sel)
 *  struct SelectList *sel;
 *  Reverse this list.
 */

extern struct SelectList *
LinkSelectCases(struct SelectList *,struct SelectList *);
/**< 
 *  struct SelectList *LinkSelectCases(sel1,sel2)
 *  struct SelectList *sel1,*sel2;
 *  Link two case lists and return the joined list.  This works best when
 *  sel1 is a one element list.
 */


#ifdef NDEBUG
#define NextSelectCase(sel) ((sel)->next)
#else
#define NextSelectCase(sel) NextSelectCaseF(sel)
#endif
extern struct SelectList *NextSelectCaseF(struct SelectList *);
/**< 
 *  macro NextSelectCase(case)
 *  struct SelectList *NextSelectCaseF(case)
 *  struct SelectList *case;
 *  Return the next case.
 */

#ifdef NDEBUG
#define SelectSetList(sel) ((sel)->values)
#else
#define SelectSetList(sel) SelectSetListF(sel)
#endif
extern struct Set *SelectSetListF(struct SelectList *);
/**< 
 *  macro SelectSetList(sel)
 *  struct Set *SelectSetListF(sel)
 *  const struct SelectList *sel;
 *  This will return the set list part of a SelectList structure. When
 *  the set is NULL, this indicates an OTHERWISE case.
 */

#ifdef NDEBUG
#define SelectStatementList(sel) ((sel)->slist)
#else
#define SelectStatementList(sel) SelectStatementListF(sel)
#endif
extern struct StatementList *SelectStatementListF(struct SelectList *);
/**< 
 *  macro SelectStatementList(sel)
 *  const struct StatementList *SelectStatementListF(sel)
 *  const struct SelectList *sel;
 *  Return the statement list.
 */

extern void DestroySelectList(struct SelectList *);
/**< 
 *  void DestroySelectList(sel)
 *  struct SelectList *sel;
 *  Destroy a whole list.
 */

extern void DestroySelectNode(struct SelectList *);
/**< 
 *  void DestroySelectNode(sel)
 *  struct SelectList *sel;
 *  Destroy just this node.
 */

extern struct SelectList *CopySelectNode(struct SelectList *);
/**< 
 *  struct SelectList *CopySelectNode(sel)
 *  struct SelectList *sel;
 *  Copy a case.  The next attribute is initialized to NULL.
 */

extern struct SelectList *CopySelectList(struct SelectList *);
/**< 
 *  struct SelectList *CopySelectList(sel)
 *  struct SelectList *sel;
 *  Copy the whole list content. not a reference count change.
 */
#endif /**< __SELECT_H_SEEN__ */



