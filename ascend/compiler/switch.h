/* 
 *  SWITCH List Routines
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: switch.h,v $
 *  Date last modified: $Date: 1997/07/18 12:35:18 $
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  SWITCH List Routines.
 *  <pre>
 *  When #including switch.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "sets.h"
 *         #include "exprs.h"
 *         #include "bit.h"
 *         #include "stattypes.h"
 *  </pre>
 */

#ifndef ASC_SWITCH_H
#define ASC_SWITCH_H

#include "expr_types.h"

/**	@addtogroup compiler_stmt Compiler Statements
	@{
*/

extern struct SwitchList *CreateSwitch(struct Set *set, struct StatementList *sl);
/**<
 *  Create a Switch node.  This will examine the set data structure which
 *  must contain constant boolean expressions, symbols values or integer
 *  values. If set = NULL, this indicates an OTHERWISE case.
 */

extern struct SwitchList *ReverseSwitchCases(struct SwitchList *sw);
/**<
 *  Reverse this list.
 */

extern struct SwitchList *LinkSwitchCases(struct SwitchList *sw1,
                                          struct SwitchList *sw2);
/**<
 *  Link two case lists and return the joined list.  This works best when
 *  sw1 is a one element list.
 */

#ifdef NDEBUG
#define NextSwitchCase(sw) ((sw)->next)
#else
#define NextSwitchCase(sw) NextSwitchCaseF(sw)
#endif
/**<
 *  Return the next case.
 *  @param sw struct SwitchList*, the list to query.
 *  @return The next case as a struct SwitchList*.
 *  @see NextSwitchCaseF()
 */
extern struct SwitchList *NextSwitchCaseF(struct SwitchList *cs);
/**<
 *  Implementation function for NextSwitchCase().  Do not call this
 *  function directly - use NextSwitchCase() instead.
 */

#ifdef NDEBUG
#define SwitchSetList(sw) ((sw)->values)
#else
#define SwitchSetList(sw) SwitchSetListF(sw)
#endif
/**<
 *  This will return the set list part of a SwitchList structure. When
 *  the set is NULL, this indicates an OTHERWISE case.
 *  @param sw struct SwitchList*, the list to query.
 *  @return The list as a struct Set*.
 *  @see SwitchSetListF()
 */
extern struct Set *SwitchSetListF(struct SwitchList *sw);
/**<
 *  Implementation function for SwitchSetList().  Do not call this
 *  function directly - use SwitchSetList() instead.
 */

#ifdef NDEBUG
#define SwitchStatementList(sw) ((sw)->slist)
#else
#define SwitchStatementList(sw) SwitchStatementListF(sw)
#endif
/**<
 *  Return the statement list.
 *  @param sw struct SwitchList*, the list to query.
 *  @return The list as a struct StatementList*.
 *  @see SwitchStatementListF()
 */
extern struct StatementList *SwitchStatementListF(struct SwitchList *sw);
/**<
 *  Implementation function for SwitchStatementList().  Do not call this
 *  function directly - use SwitchStatementList() instead.
 */

extern void DestroySwitchList(struct SwitchList *sw);
/**< 
 *  Destroy a whole list.
 */

extern void DestroySwitchNode(struct SwitchList *sw);
/**< 
 *  Destroy just this node.
 */

extern struct SwitchList *CopySwitchNode(struct SwitchList *sw);
/**< 
 *  Copy a case.  The next attribute is initialized to NULL.
 */

extern struct SwitchList *CopySwitchList(struct SwitchList *sw);
/**< 
 *  Copy the whole list content. not a reference count change.
 */

/* @} */

#endif  /* ASC_SWITCH_H */

