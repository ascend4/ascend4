/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
 *  Variable name list

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler.h"
	#include "expr_types.h"
*//* 
	by Tom Epperly
	August 8, 1989
	Last in CVS: $Revision: 1.7 $ $Date: 1997/07/18 12:36:38 $ $Author: mthomas $
*/

#ifndef ASC_VLIST_H
#define ASC_VLIST_H

/**	@addtogroup compiler Compiler
	@{
*/

/**<
 *  Don't ever access these things yourself.  Use the functions and/or
 *  macros defined in this file.
 */
struct VariableList {
  struct VariableList *next;
  struct Name *nptr;
};

extern struct VariableList *CreateVariableNode(struct Name *name);
/**<
 *  Create a node with Name pointer name.  The next attribute is initialized to
 *  NULL.
 */

extern void LinkVariableNodes(struct VariableList *cur, struct VariableList *next);
/**< 
 *  Set the next attribute of cur to next.  Next can be NULL, but cur cannot.
 */

#ifdef NDEBUG
# define NextVariableNode(n) ((n)->next)
#else
extern CONST struct VariableList *NextVariableNodeF(CONST struct VariableList *vl);
/**<
	Implementation function for NextVariableNode() (debug mode).
	Do not call this function directly - use NextVariableNode() instead.
*/
# define NextVariableNode(n) NextVariableNodeF(n)
#endif
/**<
	Return the name list node linked to vl.
	@param n CONST struct VariableList*, the list to query.
	@return The node as a CONST struct VariableList*.
	@see NextVariableNodeF()
*/

#ifdef NDEBUG
# define NamePointer(n) ((n)->nptr)
#else
extern CONST struct Name *NamePointerF(CONST struct VariableList *vl);
/**<
 *  Implementation function for NamePointer() (debug mode).
 *  Do not call this function directly - use NamePointer() instead.
 */
# define NamePointer(n) NamePointerF(n)
#endif
/**<
 *  Return the name pointer stored in node vl.
 *  @param n CONST struct VariableList*, the list to query.
 *  @return The name as a CONST struct Name*.
 *  @see NamePointerF()
 */

extern struct VariableList *CopyVariableList(struct VariableList *vl);
/**< 
 *  Make and return a copy of the whole name list.
 */

extern void DestroyVariableList(struct VariableList *vl);
/**< 
 *  Deallocate the memory associated with the list of names.
 */

extern void DestroyVariableListNode(struct VariableList *vl);
/**< 
 *  Deallocate this name node, and don't change the next one.
 */

extern struct VariableList
*JoinVariableLists(struct VariableList *vl1, struct VariableList *vl2);
/**< 
 *  Appends vl2 to the end of vl1.  This will return vl1, unless vl1 is NULL
 *  in which case it will return vl2.
 */

extern struct VariableList *ReverseVariableList(struct VariableList *vl);
/**< 
 *  Returns the reverse of vl.
 */

extern unsigned long VariableListLength(CONST struct VariableList *vl);
/**< 
 *  Returns the length of the variable list. Does not attempt to expand arrays
 *  of variables or anything like that.
 */

extern int CompareVariableLists(CONST struct VariableList *vl1,
                                CONST struct VariableList *vl2);
/**<
 *  Returns -1,0,1 as vl1 is < == > vl2 in content.
 *  The NULL list is > all lists.
 */

/* @} */

#endif /* ASC_VLIST_H */
