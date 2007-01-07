/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1996 Ben Allan
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
*//**
	@file
	Ascend Instance Tree Link Implementation.

	General comments on parents:

	  - Parents are numbered from 1 up, so zero is never a valid
	    parent number

	  - Parent lists are kept sorted by address.

	  - The ordering of the parents list is not predictable in
	    any usefull way.

	  - The number of parents for a given instance can be modified
	    by ARE_THE_SAME's.

	  - The number of parents is NOT constant for a given type.

	  - Parent lists will contain no duplications.
	    (when AddParent is used correctly, that is.)

	  - DummyInstances don't track parents -- they play a reference
	    count game.

	General comments on children:

	  - Children are number from 1 up, so zero is never a valid child
	    number.

	  - Children numbers will remain constant for a given type except
	    for arrays which can have varying numbers of children.

	  - Children numbers may or may not change will an instance is
	    refined.

	  - Children are always sorted in increasing order either
	    alphabetically or numerically depending on the type of
	    naming.

	  - Children don't know what their name is.  Only a parent
	    knows the names of the children below it.

	  - DummyInstances have no children.

	based on instance.c

	Requires:
	#include "utilities/ascConfig.h"
	#include "instance_enum.h"
	#include "compiler.h"
*//*
	by Tom Epperly 8/16/89, Ben Allan
	Version: $Revision: 1.11 $
	Version control file: $RCSfile: parentchild.h,v $
	Date last modified: $Date: 1998/02/05 16:37:24 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_PARENTCHILD_H
#define ASC_PARENTCHILD_H

#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>
#include <compiler/instance_name.h>

/* Parent routines */

ASC_DLLSPEC unsigned long NumberParents(CONST struct Instance *i);
/**<
 *  Return the number of parents that instance i has.
 *  DummyInstances have no parents, apparently.
 */

ASC_DLLSPEC struct Instance *InstanceParent(CONST struct Instance *i, unsigned long n);
/**<
 *  Return a pointer to parent number n.  Parents are numbered from
 *  1 to NumberParents(i).  0(zero) is not a valid parent number.
 *  DummyInstances have no parents, apparently.
 */

extern unsigned long SearchForParent(CONST struct Instance *i,
                                     CONST struct Instance *p);
/**<
 *  Look in instance "i"'s parent list for parent "p".  If "p" is not
 *  found, it will return 0; otherwise, it will return the parent number of
 *  "p" in instance "i"'s parent list.
 *  DummyInstance never has parents, it thinks.
 */

extern void DeleteParent(struct Instance *i, unsigned long pos);
/**<
 *  Remove parent in pos from i's parent list.
 *  DummyInstance just reduces its reference count.
 */

ASC_DLLSPEC struct InstanceName ParentsName(CONST struct Instance *p,
                                       CONST struct Instance *c);
/**<
 *  This will returns parent "p"'s name for "c".  This assumes that
 *  "c" is actually a child of "p", and it will return a bad value if
 *  "c" is not a child of "p".  Note this is actually a combination of
 *  ChildIndex and ChildName defined below.  It is defined separately
 *  because it will be call alot.  This is not a fast routine taking time
 *  proportional to the number of children p has.
 *  DummyInstance may have several names and which you get is arbitrary
 *  because you aren't supposed to care about dummy instances.
 */

extern void AddParent(struct Instance *i, struct Instance *p);
/**<
 *  This will add parent "p" to instance "i"'s parent list.  This only
 *  creates the link from "i" to "p"; it doesn't create the link from
 *  "p" to "i" which must be done with the routines below.
 *  This doesn't <-*****
 *  check if p is already a parent of "i"; you should know that it isn't
 *  before calling this.
 *  Exception:
 *  Every reference to DummyInstance should call this, and since
 *  SearchForParent will always say that p is not a parent of dummy,
 *  things work out ok.
 */

/* Children routines */

ASC_DLLSPEC unsigned long NumberChildren(CONST struct Instance *i);
/**<
 *  Return the number of children that instance i has.
 */


ASC_DLLSPEC struct Instance*InstanceChild(CONST struct Instance *i,
                                      unsigned long n);
/**<
 *  Return a pointer to children number n.  Do not confuse the child number
 *  n with an integer index.  The two numbers may have no correlation.
 *  Child number range from 1 to NumberChildren(i).  0(zero) is not a valid
 *  child number.
 *  May exit on improper i.
 */

ASC_DLLSPEC struct InstanceName ChildName(CONST struct Instance *i, unsigned long n);
/**<
 *  Returns the name of the n'th child of i.  Assumes that i has an
 *  n'th child.
 */

ASC_DLLSPEC CONST struct Statement *ChildDeclaration(CONST struct Instance *i,
                                                unsigned long n);
/**<
 *  Returns the declaration statement (IS_A,ALIASE,ARRAY) of the n'th child
 *  of i, if i has an n'th child. May return NULL under very odd circumstances.
 *  Does not return redeclarations (refinement) statements.
 *  i must not be NULL! The nth child may be NULL, but we don't care.
 *  (Note that as of 2/97 this function cannot return NULL because
 *  the ascend language is basetype safe semantically.)
 */

extern CONST struct TypeDescription *ChildRefines(CONST struct Instance *i,
                                                  unsigned long n);
/**<
 *  Returns the type of the n'th child of i as determined at parse time.
 *  The nth child of the instance i doesn't need to exist yet. (may be null)
 *  The nth child of the instance will be of at least this type if the
 *  child is not an array.
 *  If the child is an array, the type returned here
 *  will be the type of the array elements as determined at parse time.
 *  This function may return NULL -- this merely means that at parse
 *  time we couldn't quite untwist the naming to determine a type.<br><br>
 *
 *  Warning: Unselected parts will yield instances of type DUMMYINST
 *  instead of whatever type this returns.
 */

ASC_DLLSPEC unsigned long ChildSearch(CONST struct Instance *i,
                                 CONST struct InstanceName *name);
/**<
 *  This procedure will search instance i for a child that matches "name".
 *  It it is unsuccessful, it will return 0; otherwise, it will return the
 *  index of the child that matches.  It is assumed that all children have
 *  unique names and that there is no need to worry about more than one child
 *  matching "name".  This can be called on childless instance without
 *  error. The strings in the InstanceName must come from the symbol table.
 */

ASC_DLLSPEC struct Instance*ChildByChar(CONST struct Instance *i,
                                    symchar *str);
/**<
 *  This returns to the pointer to a child, c, of parent,p, named by str.
 *  str must be a simple name. If child not found, returns NULL.
 *  str must be from the symbol table. If AscFindSymbol(str)==NULL,
 *  then this function should not be called because NO instance
 *  can have a child with a name which is not in the symbol table.
 */

/*
 *  extern unsigned long ChildSearchRegExp(); NOT IMPLEMENTED.
 *  unsigned long ChildSearchRegExp(i,name,start)
 *  struct Instance *i;
 *  char *name;
 *  unsigned long start;
 *
 *  This is an unplanned extension to the initial implementation to add regular
 *  expression searches.  The function will search from child number "start"
 *  to the first match.  If no match is found, it will return 0; otherwise
 *  it will return the first match of the regular expression.  Examples of
 *  regular expressions are:
 *  f*	which means all strings starting with the letter f
 *  f?	which means all two character long strings starting with
 *  f
 */

ASC_DLLSPEC unsigned long ChildIndex(CONST struct Instance *i,
                                CONST struct Instance *child);
/**<
 *  This procedure searches through the child list of instance i for child.
 *  If it does not find a match, it returns 0; otherwise, it will return
 *  the child number of "child".
 */

extern void StoreChildPtr(struct Instance *i,
                          unsigned long n,
                          struct Instance *child);
/**<
 *  Store the child pointer "child" in position n of i.  This only creates
 *  the link from i to child, and not the back link which is created by
 *  add parent.  Instance "i" cannot be a fundamental atom; atoms are
 *  created in a special way.
 */

#endif  /* ASC_PARENTCHILD_H */
