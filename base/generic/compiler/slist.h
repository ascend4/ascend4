/*
 *  Statement list routines
 *  by Tom Epperly
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: slist.h,v $
 *  Date last modified: $Date: 1997/07/18 12:35:00 $
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
 *  Statement list routines.
 *  <pre>
 *  When #including slist.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "list.h"
 *         #include "stattypes.h"
 *  </pre>
 */

#ifndef __SLIST_H_SEEN__
#define __SLIST_H_SEEN__

extern struct StatementList *CreateStatementList(struct gl_list_t *l);
/**<
 *  <!--  struct StatementList *CreateStatementList(l)                 -->
 *  <!--  struct gl_list_t *l;                                         -->
 *  Create a statement list structure.
 */

extern struct StatementList *EmptyStatementList(void);
/**< 
 *  <!--  struct StatementList *EmptyStatementList()                   -->
 *  Create an empty statement list structure.
 */

#ifdef NDEBUG
#define GetList(sl) ((sl)->l)
#else
#define GetList(sl) GetListF(sl,__FILE__,__LINE__)
#endif
/**<
 *  Return the list structure.
 *  @param sl CONST struct StatementList*, list to query.
 *  @return The list as a struct gl_list_t*.
 *  @see GetListF()
 */
extern struct gl_list_t *GetListF(CONST struct StatementList *sl,
                                  CONST char *file,
                                  int line);
/**<
 *  <!--  macro GetList(sl)                                            -->
 *  <!--  struct gl_list_t *GetListF(sl,file,line)                     -->
 *  <!--  struct StatementList *sl;                                    -->
 *  <!--  Return the list structure                                    -->
 *  Implementation function for GetList().  Do not call this
 *  function directly - use GetList() instead.
 */

extern unsigned long StatementInList(CONST struct StatementList *sl,
                                     CONST struct Statement *s);
/**<
 *  <!--  c = StatementInList(sl,s);                                   -->
 *  <!--  struct StatementList *sl;                                    -->
 *  <!--  struct Statement *s;                                         -->
 *  Returns the position of s in sl, if s is in sl. otherwise returns 0.
 *  Handles NULL input gracefully. Does not look inside compound statements.
 */

extern struct Statement *GetStatement(CONST struct StatementList *sl,
                                      unsigned long int j);
/**<
 *  <!--  s = GetStatement(sl,j);                                      -->
 *  <!--  struct StatementList *sl;                                    -->
 *  <!--  unsigned long j;                                             -->
 *  <!--  CONST struct Statement *s;                                   -->
 *  Returns the jth statement from sl, or NULL if it doesn't exist.
 *  Tolerates all forms of insane input, and much code relies on this fact.
 *  Correct values of j are 1..StatementListLength(sl).
 */

extern struct StatementList *CopyStatementList(struct StatementList *sl);
/**< 
 *  <!--  struct StatementList *CopyStatementList(sl)                  -->
 *  <!--  struct StatementList *sl;                                    -->
 *  By reference on the slist. Statements in the list are not
 *  changed in any way.
 */

extern struct StatementList *CopyListToModify(struct StatementList *sl);
/**<
 *  <!--  struct StatementList *CopyListToModify(sl)                   -->
 *  <!--  struct StatementList *sl;                                    -->
 *  Creates new slist and new copies in memory of all that it contains.
 *  Avoid this operator if possible.
 */

extern int CompareStatementLists(CONST struct StatementList *sl1,
                                 CONST struct StatementList *sl2,
                                 unsigned long int *diff);
/**<
 *  <!--  cmp = CompareStatementLists(sl1,sl2,diff);                   -->
 *  <!--  CONST struct StatementList *sl1, *sl2;                       -->
 *  <!--  unsigned long int *diff;                                     -->
 *  <!--  int cmp;                                                     -->
 *
 *  Returns -1 0 1 as sl1 < == > sl2.
 *  If cmp != 0, diff is the position of the first unequal statement.
 *  If the lists are of different length, but the longer is contentwise
 *  identical up to the length of the shorter, then diff will be
 *  the length of the shorter list+1.
 *  Returns the position of the first statement which is not
 *  equivalent between sl1 and sl2.
 *  diff must not be NULL.
 *  Returns 0 if there is no difference.
 */

extern int CompareISLists(CONST struct StatementList *sl1,
                          CONST struct StatementList *sl2,
                          unsigned long int *diff);
/**<
 *  <!--  cmp = CompareISLists(sl1,sl2,diff);                          -->
 *  <!--  CONST struct StatementList *sl1, *sl2;                       -->
 *  <!--  unsigned long int diff;                                      -->
 *  <!--  int cmp;                                                     -->
 *
 *  Compare slists containing only StateIS, some StateARE
 *  (WILL_BE, IS_A, IS_REFINED_TO, WILL_BE_THE_SAME)
 *  statements. (Particularly no compound statements are allowed,
 *  except FOR loops containing only the above types.<br><br>
 *
 *  Returns -1 0 1 as sl1 < == > sl2.
 *  If cmp != 0, diff is the position of the first type incompatible statement.
 *  If the lists are of different length, but the longer is contentwise
 *  identical up to the length of the shorter, then diff will be
 *  the length of the shorter list+1.
 *  Returns the position of the first statement which is not
 *  compatible between sl1 and sl2.
 *  Returns 0 if there is no incompatibility.<br><br>
 *
 *  Special usage notes:
 *  In this function, order matters: sl2 statements should be 'more refined'
 *  than sl1 statements.
 *  In particular, incompatibility exists unless the type given in
 *  sl2 == or MoreRefined() than the type given in sl1.
 */

extern void AppendStatement(struct StatementList *sl, struct Statement *s);
/**< 
 *  <!--  AppendStatement(sl,s);                                       -->
 *  <!--  struct StatementList *sl;                                    -->
 *  <!--  struct Statement *s;                                         -->
 *
 *  Add s to sl, copying s by reference.
 *  sl may be empty, but not NULL. s must not be NULL.
 */

extern struct StatementList
*AppendStatementLists(CONST struct StatementList *sl1,
                      struct StatementList *sl2);
/**< 
 *  <!--  struct StatementList *AppendStatementLists(sl1,sl2)          -->
 *  <!--  slr = AppendStatementLists(sl1,sl2);                         -->
 *  <!--  CONST struct StatementList *sl1;                             -->
 *  <!--  struct StatementList *sl2, *slr;                             -->
 *
 *  Make a _new_ list, slr, that contains all the statements
 *  (copied by reference) from sl1 followed by
 *  all the statements from sl2 (copied by reference).
 *  Then destroy list sl2. (not its statements).
 *  sl1 and sl2 may be empty, but not NULL.<br><br>
 *
 *  sl1 is not destroyed, on the assumption that it belongs
 *  to someone else who isn't done with it yet.<br><br>
 *
 *  This is the function to use to copy an existing list by
 *  refering to its statements rather than by just upping the
 *  reference count on the slist.
 *  e.g. to copy:
 *  newlist = AppendStatementList(oldlist,EmptyStatementList());
 */

extern void DestroyStatementList(struct StatementList *sl);
/**<
 *  <!--  void DestroyStatementList(sl)                                -->
 *  <!--  struct StatementList *sl;                                    -->
 *  Destroy a statement list.  Tolerates null input.
 */

#endif  /* __SLIST_H_SEEN__ */

