/* 
 *  Variable name list
 *  by Tom Epperly
 *  August 8, 1989
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: vlist.h,v $
 *  Date last modified: $Date: 1997/07/18 12:36:38 $
 *  Last modified by: $Author: mthomas $
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
 *  Variable name list.
 *  <pre>
 *  When #including vlist.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "expr_types.h"
 *  </pre>
 */

#ifndef __VLIST_H_SEEN__
#define __VLIST_H_SEEN__

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
 *  <!--  struct VariableList *CreateVariableNode(name)                -->
 *  <!--  struct Name *name;                                           -->
 *  Create a node with Name pointer name.  The next attribute is initialized to
 *  NULL.
 */

extern void LinkVariableNodes(struct VariableList *cur, struct VariableList *next);
/**< 
 *  <!--  void LinkVariableListNodes(cur,next)                         -->
 *  <!--  struct VariableList *cur,*next;                              -->
 *  Set the next attribute of cur to next.  Next can be NULL, but cur cannot.
 */

#ifdef NDEBUG
#define NextVariableNode(n) ((n)->next)
#else
#define NextVariableNode(n) NextVariableNodeF(n)
#endif
/**<
 *  Return the name list node linked to vl.
 *  @param n CONST struct VariableList*, the list to query.
 *  @return The node as a CONST struct VariableList*.
 *  @see NextVariableNodeF()
 */
extern CONST struct VariableList
*NextVariableNodeF(CONST struct VariableList *vl);
/**<
 *  <!--  macro NextVariableNode(nvl)                                  -->
 *  <!--  extern CONST struct VariableList *NextVariableNodeF(vl)      -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  Return the name list node linked to vl.                      -->
 *  Implementation function for NextVariableNode() (debug mode).
 *  Do not call this function directly - use NextVariableNode() instead.
 */

#ifdef NDEBUG
#define NamePointer(n) ((n)->nptr)
#else
#define NamePointer(n) NamePointerF(n)
#endif
/**<
 *  Return the name pointer stored in node vl.
 *  @param n CONST struct VariableList*, the list to query.
 *  @return The name as a CONST struct Name*.
 *  @see NamePointerF()
 */
extern CONST struct Name *NamePointerF(CONST struct VariableList *vl);
/**<
 *  <!--  struct Name *NamePointer(vl)                                 -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  Return the name pointer stored in node vl.                   -->
 *  Implementation function for NamePointer() (debug mode).
 *  Do not call this function directly - use NamePointer() instead.
 */

extern struct VariableList *CopyVariableList(struct VariableList *vl);
/**< 
 *  <!--  struct VariableList *CopyVariableList(vl)                    -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Make and return a copy of the whole name list.
 */

extern void DestroyVariableList(struct VariableList *vl);
/**< 
 *  <!--  void DestroyVariableList(vl)                                 -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Deallocate the memory associated with the list of names.
 */

extern void DestroyVariableListNode(struct VariableList *vl);
/**< 
 *  <!--  void DestroyVariableListNode(vl)                             -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Deallocate this name node, and don't change the next one.
 */

extern struct VariableList
*JoinVariableLists(struct VariableList *vl1, struct VariableList *vl2);
/**< 
 *  <!--  struct VariableList *JoinVariableLists(vl1,vl2)              -->
 *  <!--  struct VariableList *vl1,*vl2;                               -->
 *  Appends vl2 to the end of vl1.  This will return vl1, unless vl1 is NULL
 *  in which case it will return vl2.
 */

extern struct VariableList *ReverseVariableList(struct VariableList *vl);
/**< 
 *  <!--  struct VariableList *ReverseVariableList(vl)                 -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Returns the reverse of vl.
 */

extern unsigned long VariableListLength(CONST struct VariableList *vl);
/**< 
 *  <!--  unsigned long VariableListLength(vl);                        -->
 *  <!--  CONST struct VariableList *vl;                               -->
 *  Returns the length of the variable list. Does not attempt to expand arrays
 *  of variables or anything like that.
 */

extern int CompareVariableLists(CONST struct VariableList *vl1,
                                CONST struct VariableList *vl2);
/**<
 *  <!--  int CompareVariableLists(vl1,vl2);                           -->
 *  Returns -1,0,1 as vl1 is < == > vl2 in content.
 *  The NULL list is > all lists.
 */

#endif /* __VLIST_H_SEEN__ */

