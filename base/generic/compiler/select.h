/*
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

/** @file
 *  SELECT List Routines.
 *  <pre>
 *  When #including select.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "sets.h"
 *         #include "exprs.h"
 *         #include "bit.h"
 *         #include "stattypes.h"
 *  </pre>
 */

#ifndef ASC_SELECT_H
#define ASC_SELECT_H

/**	addtogroup compiler Compiler
	@{
*/

extern struct SelectList *CreateSelect(struct Set *set, struct StatementList *sl);
/**< 
 *  <!--  struct SelectList *CreateSelect(set,sl)                      -->
 *  <!--  struct Set *set;                                             -->
 *  <!--  struct StatementList *sl;                                    -->
 *  Create a Select node.  This will examine the set data structure which
 *  must contain only constant expressions (TRUE or FALSE), symbol values or
 *  integer values.
 *  If set = NULL, this indicates an OTHERWISE case.
 */

extern struct SelectList *ReverseSelectCases(struct SelectList *sel);
/**< 
 *  <!--  struct SelectList *ReverseSelectCases(sel)                   -->
 *  <!--  struct SelectList *sel;                                      -->
 *  Reverse this list.
 */

extern struct SelectList
*LinkSelectCases(struct SelectList *sel1, struct SelectList *sel2);
/**< 
 *  <!--  struct SelectList *LinkSelectCases(sel1,sel2)                -->
 *  <!--  struct SelectList *sel1,*sel2;                               -->
 *  Link two case lists and return the joined list.  This works best when
 *  sel1 is a one element list.
 */


#ifdef NDEBUG
#define NextSelectCase(sel) ((sel)->next)
#else
#define NextSelectCase(sel) NextSelectCaseF(sel)
#endif
/**<
 *  Return the next case.
 *  @param sel struct SelectList*, the SelectList to query.
 *  @return Returns the next case as a struct SelectList*.
 *  @see NextSelectCaseF()
 */
extern struct SelectList *NextSelectCaseF(struct SelectList *cs);
/**<
 *  <!--  macro NextSelectCase(cs)                                     -->
 *  <!--  struct SelectList *NextSelectCaseF(cs)                       -->
 *  <!--  struct SelectList *cs;                                       -->
 *  <!--  Return the next case.                                        -->
 *  Implementation function for NextSelectCase().  Do not call this
 *  function directly - use NextSelectCase() instead.
 */

#ifdef NDEBUG
#define SelectSetList(sel) ((sel)->values)
#else
#define SelectSetList(sel) SelectSetListF(sel)
#endif
/**<
 *  This will return the set list part of a SelectList structure. When
 *  the set is NULL, this indicates an OTHERWISE case.
 *  @param sel struct SelectList*, the SelectList to query..
 *  @return Returns the set list as a struct Set*.
 *  @see SelectSetListF()
 */
extern struct Set *SelectSetListF(struct SelectList *sel);
/**<
 *  <!--  macro SelectSetList(sel)                                     -->
 *  <!--  struct Set *SelectSetListF(sel)                              -->
 *  <!--  const struct SelectList *sel;                                -->
 *  <!--  This will return the set list part of a SelectList structure.--> 
 *  <!--  When the set is NULL, this indicates an OTHERWISE case.      -->
 *  Implementation function for SelectSetList().  Do not call this
 *  function directly - use SelectSetList() instead.
 */

#ifdef NDEBUG
#define SelectStatementList(sel) ((sel)->slist)
#else
#define SelectStatementList(sel) SelectStatementListF(sel)
#endif
/**<
 *  Return the statement list.
 *  @param sel struct SelectList*, the SelectList to query..
 *  @return Returns the statement list as a struct StatementList*.
 *  @see SelectStatementListF()
 */
extern struct StatementList *SelectStatementListF(struct SelectList *sel);
/**<
 *  <!--  macro SelectStatementList(sel)                               -->
 *  <!--  const struct StatementList *SelectStatementListF(sel)        -->
 *  <!--  const struct SelectList *sel;                                -->
 *  <!--  Return the statement list.                                   -->
 *  Implementation function for SelectStatementList().  Do not call this
 *  function directly - use SelectStatementList() instead.
 */

extern void DestroySelectList(struct SelectList *sel);
/**< 
 *  <!--  void DestroySelectList(sel)                                  -->
 *  <!--  struct SelectList *sel;                                      -->
 *  Destroy a whole list.
 */

extern void DestroySelectNode(struct SelectList *sel);
/**< 
 *  <!--  void DestroySelectNode(sel)                                  -->
 *  <!--  struct SelectList *sel;                                      -->
 *  Destroy just this node.
 */

extern struct SelectList *CopySelectNode(struct SelectList *sel);
/**< 
 *  <!--  struct SelectList *CopySelectNode(sel)                       -->
 *  <!--  struct SelectList *sel;                                      -->
 *  Copy a case.  The next attribute is initialized to NULL.
 */

extern struct SelectList *CopySelectList(struct SelectList *sel);
/**< 
 *  <!--  struct SelectList *CopySelectList(sel)                       -->
 *  <!--  struct SelectList *sel;                                      -->
 *  Copy the whole list content. not a reference count change.
 */

/* @} */

#endif  /* ASC_SELECT_H */

