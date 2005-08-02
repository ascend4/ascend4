/*
 *  Name external definitions
 *  by Tom Epperly
 *  July 31, 1989
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: name.h,v $
 *  Date last modified: $Date: 1998/06/16 16:36:28 $
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
 *  Name external definitions.
 *  <pre>
 *  When #including name.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *  </pre>
 *  @todo These things need to be pooled to save space and time. baa 3/96
 */

#ifndef __NAME_H_SEEN__
#define __NAME_H_SEEN__

#define CreateIdName(s) CreateIdNameF((s),NAMEBIT_IDTY)
/**< 
 *  Create name with Id flag.
 *  @see CreateIdNameF()
 */
#define CreateSystemIdName(s) CreateIdNameF((s),NAMEBIT_IDTY|NAMEBIT_AUTO)
/**<
 *  Create system name with Id+Auto flags.
 *  @see CreateIdNameF()
 */
extern struct Name *CreateIdNameF(symchar *s, int bits);
/**<
 *  <!--  macro CreateIdName(s)                                        -->
 *  <!--  macro CreateSystemIdName(s)                                  -->
 *  <!--  Create name with Id or system name Id+Auto flags.            -->
 *  <!--  struct Name *CreateIdNameF(s,bits)                           -->
 *  <!--  symchar *s;                                                  -->
 *  Create a name node with the identifier s
 *  and flag bits associated with it.  Implementation
 *  function for CreateIdName() and CreateSystemIdName().
 */

extern struct Name *CreateSetName(struct Set *s);
/**< 
 *  <!--  struct Name *CreateSetName(s)                                -->
 *  <!--  struct Set *s;                                               -->
 *  Create a name node of type set with the set s associated with it.
 */

extern void LinkNames(struct Name *cur, struct Name *next);
/**< 
 *  <!--  void LinkName(cur,next)                                      -->
 *  <!--  struct Name *cur;                                            -->
 *  <!--  const struct Name *next;                                     -->
 *  Link "next" to cur so that NextName(cur) = next
 */

#ifdef NDEBUG
#define NextName(n) ((n)->next)
#else
#define NextName(n) NextNameF(n)
#endif
/**<
 *  Return the next attribute of n.
 *  @param n CONST struct Name*, Name to query.
 *  @return The next attribute as a struct Name*.
 *  @see NextNameF()
 */
extern struct Name *NextNameF(CONST struct Name *n);
/**<
 *  <!--  macro NextName(n)                                            -->
 *  <!--  struct Name *NextNameF(n)                                    -->
 *  <!--  const struct Name *n;                                        -->
 *  <!--  Return the next attribute of n.                              -->
 *  Implementation function for NextName().  Do not call this
 *  function directly - use NextName() instead.
 */

#ifdef NDEBUG
#define NameId(n) ((n)->bits & NAMEBIT_IDTY)
#else
#define NameId(n) NameIdF(n)
#endif
/**<
 *  Test whether a Name is an identifier.
 *  We should have analogous functions for CHAT and ATTR, but since no
 *  clients yet use them, they aren't implemented.
 *  @param n CONST struct Name*, Name to query.
 *  @return An int:  NAMEBIT_IDTY if n is an identifier type Name or 0 otherwise.
 *  @see NameIdF()
 */
extern int NameIdF(CONST struct Name *n);
/**< 
 *  <!--  macro NameId(n)                                              -->
 *  <!--  int NameIdF(n)                                               -->
 *  <!--  const struct Name *n;                                        -->
 *  <!--  Return NAMEBIT_IDTY if n is an identifier type Name or 0 otherwise. -->
 *  <!--  We should have analogous functions for CHAT and ATTR, but since no  -->
 *  <!--  clients yet use them, they aren't implemented.                      -->
 *  Implementation function for NameId().  Do not call this
 *  function directly - use NameId() instead.
 */

#ifdef NDEBUG
#define NameAuto(n) ((n)->bits & (NAMEBIT_AUTO|NAMEBIT_IDTY))
#else
#define NameAuto(n) NameAutoF(n)
#endif
/**<
 *  Test whether a Name is a system generated identifier.
 *  @param n CONST struct Name*, Name to query.
 *  @return An int:  NAMEBIT_AUTO if n is an system generated identifier
 *          type Name, or 0 otherwise.
 *  @see NameAutoF()
 */
extern int NameAutoF(CONST struct Name *n);
/**<
 *  <!--  macro NameAuto(n)                                            -->
 *  <!--  int NameAutoF(n)                                             -->
 *  <!--  const struct Name *n;                                        -->
 *  <!--  Return NAMEBIT_AUTO if n is an system generated identifier   -->
 *  <!--  type Name, or 0 otherwise.                                   -->
 *  Implementation function for NameAuto().  Do not call this
 *  function directly - use NameAuto() instead.
 */

#ifdef NDEBUG
#define NameIdPtr(n) ((n)->val.id)
#else
#define NameIdPtr(n) NameIdPtrF(n)
#endif
/**<
 *  Returns the id pointer for identifier type name node n.
 *  @param n CONST struct Name*, Name to query.
 *  @return The id pointer as a symchar*.
 *  @see NameIdPtrF()
 */
extern symchar *NameIdPtrF(CONST struct Name *n);
/**< 
 *  <!--  macro NameIdPtr(n)                                           -->
 *  <!--  extern const char *NameIdPtrF(n)                             -->
 *  <!--  const struct Name *n;                                        -->
 *  <!--  Assumes that n is a identifier type name node.               -->
 *  <!--  Returns the id pointer.                                      -->
 *  Implementation function for NameIdPtr().  Do not call this
 *  function directly - use NameIdPtr() instead.
 */

extern symchar *SimpleNameIdPtr(CONST struct Name *n);
/**< 
 *  <!--  const char *SimpleNameIdPtr(n)                               -->
 *  <!--  const struct Name *n;                                        -->
 *  Return NULL if n is not an NameId or if it has a next field.  Otherwise,
 *  it returns the char pointer.
 */

extern unsigned int NameLength(CONST struct Name *n);
/**< 
 *  <!--  unsigned int NameLength(n);                                  -->
 *  <!--  const struct Name *n;                                        -->
 *  Returns the number of links in a chain of names. May be used in a
 *  similar manner to SimpleNameIdPtr, to determine if a name is a simple
 *  name e.g. "my_var" (len = 1 ) rather than "my_var.your_var[1..3]"
 *  (len = 3);
 */

#ifdef NDEBUG
#define NameSetPtr(n) ((n)->val.s)
#else
#define NameSetPtr(n) NameSetPtrF(n)
#endif
/**<
 *  Returns the set pointer for set type name node n.
 *  @param n CONST struct Name*, Name to query.
 *  @return The set pointer as a CONST struct Set*.
 *  @see NameSetPtrF()
 */
extern CONST struct Set *NameSetPtrF(CONST struct Name *n);
/**< 
 *  <!--  const struct Set *NameSetPtrF(n)                             -->
 *  <!--  const struct Name *n;                                        -->
 *  <!--  Assumes that n is a set type name node.                      -->
 *  <!--  Returns the set pointer.                                     -->
 *  Implementation function for NameSetPtr().  Do not call this
 *  function directly - use NameSetPtr() instead.
 */

extern struct Name *CopyName(CONST struct Name *n);
/**< 
 *  <!--  struct Name *CopyName(n)                                     -->
 *  <!--  const struct Name *n;                                        -->
 *  Make and return a copy of the whole name.
 */

extern void DestroyName(struct Name *n);
/**< 
 *  <!--  void DestroyName(n)                                          -->
 *  <!--  struct Name *n;                                              -->
 *  Deallocate the whole name linked list
 *  Handles NULL input gracefully.
 */

extern void DestroyNamePtr(struct Name *n);
/**< 
 *  <!--  void DestroyNamePtr(n)                                       -->
 *  <!--  struct Name *n;                                              -->
 *  Deallocate this name node, and don't change the next node.
 *  Handles NULL input gracefully.
 */

extern struct Name *JoinNames(struct Name *n1, struct Name *n2);
/**< 
 *  <!--  struct Name *JoinNames(n1,n2)                                -->
 *  <!--  struct Name *n1, *n2;                                        -->
 *  Appends n2 to the end of n1.  This will return n1, unless n1 is NULL in
 *  which case it will return n2.
 */

extern struct Name *ReverseName(struct Name *n);
/**< 
 *  <!--  struct Name *ReverseName(n)                                  -->
 *  <!--  struct Name *n;                                              -->
 *  Returns the reverse of n.
 */

extern CONST struct Name *NextIdName(CONST struct Name *n);
/**< 
 *  <!--  struct Name *NextIdName(n)                                   -->
 *  <!--  struct Name *n;                                              -->
 *  Returns the first NameId element in the name after the current element
 *  which is expected to be a NameId. If there is none, returns NULL.
 */

extern int NameCompound(CONST struct Name *n);
/**< 
 *  <!--  int NameCompound(n)                                          -->
 *  <!--  const struct Name *n;                                        -->
 *  Test whether name is compound (i.e. crosses a MODEL/ATOM boundary).
 *  If so this returns 1, OTHERWISE this returns 0.  So array names 
 *  will return 0.
 *  The following return 0:
 *  - a
 *  - a[i]
 *  - [i][j]  -- though this isn't a proper name, generally.
 *
 *  The following return 1:
 *  - a.b
 *  - a[i].b
 *  - [i].b
 *
 *  So basically, if the name is printed with a . this will return 1.
 */

extern int NamesEqual(CONST struct Name *n1, CONST struct Name *n2);
/**< 
 *  <!--  int NamesEqual(n1,n2)                                        -->
 *  <!--  const struct Name *n1,*n2;                                   -->
 *  Return TRUE if and only if n1 and n2 are structurally equivalent.
 */

extern int CompareNames(CONST struct Name *n1, CONST struct Name *n2);
/**< 
 *  <!--  int CompareNames(n1,n2)                                      -->
 *  <!--  const struct Name *n1,*n2;                                   -->
 *  Returns -1 0 1 as n1 < = > n2.
 *  Will need fixing when we have supported attributes.
 */

extern void name_init_pool(void);
/**< 
 * Starts memory recycle. Do not call twice before stopping recycle.
 */

extern void name_destroy_pool(void);
/**< 
 * Stops memory recycle. Do not call while ANY names are outstanding.
 */

extern void name_report_pool(void);
/**< 
 * Write the pool report to ASCERR for the name pool.
 */

#endif  /* __NAME_H_SEEN__ */

