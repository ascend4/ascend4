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

/*
 *  When #including .h, make sure these files are #included first:
 *         #include "compiler.h"
 */



#ifndef __VLIST_H_SEEN__
#define __VLIST_H_SEEN__
/* requires
# #include"types.h"
*/

/*
 *  Don't ever access these things yourself.  Use the functions and/or
 *  macros defined in this file.
 */
struct VariableList {
  struct VariableList *next;
  struct Name *nptr;
};

extern struct VariableList *CreateVariableNode(struct Name *);
/*
 *  struct VariableList *CreateVariableNode(n)
 *  struct Name *n;
 *  Create a node with name pointer n.  The next attribute is initialized to
 *  NULL.
 */

extern void LinkVariableNodes(struct VariableList *,struct VariableList *);
/*
 *  void LinkVariableListNodes(cur,next)
 *  struct VariableList *cur,*next;
 *  Set the next attribute of cur to next.  Next can be NULL, but cur cannot.
 */

#ifdef NDEBUG
#define NextVariableNode(n) ((n)->next)
#else
#define NextVariableNode(n) NextVariableNodeF(n)
#endif
extern CONST struct VariableList
*NextVariableNodeF(CONST struct VariableList *);
/*
 *  macro NextVariableNode(n)
 *  extern CONST struct VariableList *NextVariableNodeF(n)
 *  struct VariableList *n;
 *  Return the name list node linked to n.
 */

#ifdef NDEBUG
#define NamePointer(n) ((n)->nptr)
#else
#define NamePointer(n) NamePointerF(n)
#endif
extern CONST struct Name *NamePointerF(CONST struct VariableList *);
/*
 *  struct Name *NamePointer(n)
 *  struct VariableList *n;
 *  Return the name pointer stored in node n.
 */

extern struct VariableList *CopyVariableList(struct VariableList *);
/*
 *  struct VariableList *CopyVariableList(n)
 *  struct VariableList *n;
 *  Make and return a copy of the whole name list.
 */

extern void DestroyVariableList(struct VariableList *);
/*
 *  void DestroyVariableList(n)
 *  struct VariableList *n;
 *  Deallocate the memory associated with the list of names.
 */

extern void DestroyVariableListNode(struct VariableList *);
/*
 *  void DestroyVariableListNode(n)
 *  struct VariableList *n;
 *  Deallocate this name node, and don't change the next one.
 */

extern struct VariableList *JoinVariableLists(struct VariableList *,
           struct VariableList *);
/*
 *  struct VariableList *JoinVariableLists(n1,n2)
 *  struct VariableList *n1,*n2;
 *  Appends n2 to the end of n1.  This will return n1, unless n1 is NULL
 *  in which case it will return n2.
 */

extern struct VariableList *ReverseVariableList(struct VariableList *);
/*
 *  struct VariableList *ReverseVariableList(n)
 *  struct VariableList *n;
 *  Returns the reverse of n.
 */

extern unsigned long VariableListLength(CONST struct VariableList *);
/*
 *  unsigned long VariableListLength(n);
 *  CONST struct VariableList *n;
 *  Returns the length of the variable list. Does not attempt to expand arrays
 *  of variables or anything like that.
 */

extern int CompareVariableLists(CONST struct VariableList *,
                                CONST struct VariableList *);
/*
 *  int CompareVariableLists(vl1,vl2);
 *  Returns -1,0,1 as vl1 is < == > vl2 in content.
 *  The NULL list is > all lists.
 */
#endif /* __VLIST_H_SEEN__ */
