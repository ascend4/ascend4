/**< 
 *  Routines to Process Set Instance Values
 *  by Tom Epperly
 *  11/16/89
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: setinstval.h,v $
 *  Date last modified: $Date: 1998/02/05 16:37:52 $
 *  Last modified by: $Author: ballan $
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
 *
 *
 *  This group of routines is used to process the value fields of set
 *  instances.
 */


/**< 
 *  When #including setinstval.h, make sure these files are #included first:
 *         #include "compiler.h"
 */


#ifndef __SETINSTVAL_H_SEEN__
#define __SETINSTVAL_H_SEEN__
/**< requires
# #include <stdio.h>
# #include"compiler.h"
# #include"list.h"
*/
enum set_kind {
  integer_set,			/**< set of integer values */
  string_set,			/**< set of string values */
  empty_set			/**< set with nothing in it */
};

/**< Do not confuse this set with the struct Set in types.h */
struct set_t {
  enum set_kind kind;
  struct gl_list_t *list;
};

extern void InitSetManager(void);
/**< 
 *  We pool the set stubs for reuse, since there are so many used in
 *  most ASCEND models of any complexity.
 *  This function initializes the pool.
 *  The pool grows as needed. It does not shrink until destroyed.
 *  It is a fatal error to call this a second time unless
 *  DestroySetManager has been called first.
 *  In the pooling operation we only keep the set stubs, not the
 *  gl_lists associated.
 *
 *  This exits if it cannot init the pool because of insufficient
 *  memory.
 */

extern void DestroySetManager(void);
/**< 
 *  Destroys the set pool. Bombs if no pool exists.
 */

extern void ReportSetManager(FILE*);
/**< 
 *  ReportSetManager(f);
 *  FILE *f;
 *  Reports on the set pool to f.
 */

extern struct set_t *CreateEmptySet(void);
/**< 
 *  struct set_t *CreateEmptySet()
 *  Creates an empty set.
 */

extern void InsertInteger(struct set_t *,long);
/**< 
 *  void InsertInteger(set,i)
 *  struct set_t *set;
 *  long i;
 *  Insert i into set.  You can insert i multiple times without causing
 *  problems.  Set must be non-NULL.  This will coerce an empty set type
 *  into an integer set type; after that no symbols can be inserted into
 *  the set.
 */

extern void InsertIntegerRange(struct set_t *,long,long);
/**< 
 *  struct InsertIntegerRange(set,lower,upper)
 *  struct set_t *set;
 *  long lower, upper;
 *  Inser the elements i such that i >= lower and i <= upper.  Note if
 *  lower > upper, no elements are inserted.  This will coerce an empty
 *  set type into an integer set type.
 */

extern void InsertString(struct set_t *,symchar *);
/**< 
 *  struct InsertString(set,str)
 *  struct set_t *set;
 *  symchar *str;
 *  Insert str into set.  This will coerce an empty set type into
 *  a string set type; after which no integers can be inserted.
 */

extern struct set_t *SetUnion(CONST struct set_t *,CONST struct set_t *);
/**< 
 *  struct set_t *SetUnion(s1,s2)
 *  const struct set_t *s1,*s2;
 *  Create a set which is the union of s1 and s2.  s1 and s2 are uneffected.
 *  Union with empty set is always okay.
 */

extern struct set_t *SetIntersection(CONST struct set_t *,
         CONST struct set_t *);
/**< 
 *  struct set_t *SetIntersection(s1,s2);
 *  const struct set_t *s1,*s2;
 *  Create a set which is the intersection of s1 and s2.  s1 and s2 are
 *  uneffected.  Intersection with empty set is okay.
 */

extern struct set_t *SetDifference(CONST struct set_t *,CONST struct set_t *);
/**< 
 *  struct set_t *SetDifference(s1,s2)
 *  const struct set_t *s1,*s2;
 *  Create a set which is define as follows:
 *  return set := { i | (i IN s1) and (NOT (i IN s2))}
 */

extern struct set_t *CopySet(CONST struct set_t *);
/**< 
 *  struct set_t *CopySet(set)
 *  const struct set_t *set;
 */

extern int IntMember(long,CONST struct set_t *);
/**< 
 *  int IntMember(i,set)
 *  long i;
 *  const struct set_t *set;
 *  Return a TRUE value if i is a member of set; otherwise, return a false
 *  value.
 */

extern int StrMember(symchar *,CONST struct set_t *);
/**< 
 *  int StrMember(str,set)
 *  symchar *str;
 *  const struct set_t *set;
 *  Return a TRUE value if str is a member of set; otherwise, return a false
 *  value.
 */

extern void DestroySet(struct set_t *);
/**< 
 *  "Free up" set pointer. Actually returns it to the pool of set stubs.
 */

extern int NullSet(CONST struct set_t *);
/**< 
 *  Testing if the set is empty.  TRUE if set is empty.
 */

extern unsigned long Cardinality(CONST struct set_t *);
/**< 
 *  Return the number of members.
 */

extern symchar *FetchStrMember(CONST struct set_t *,unsigned long);
/**< 
 * returns the nth (internal sort order ) member symbol.
 */

extern long FetchIntMember(CONST struct set_t *,unsigned long);

extern void SetIterate(struct set_t *,void (*)());
/**< 
 *
 */

extern enum set_kind SetKind(CONST struct set_t *);
/**< 
 *  Returns the type of the set.
 */

extern int SetsEqual(CONST struct set_t *,CONST struct set_t *);
/**< 
 *  int SetsEqual(s1,s2)
 *  const struct set_t *s1,*s2;
 *  Returns a true value if the two sets are equal.  Set equality is
 *  defined as:
 *  (i IN s1) <==> (i IN s2)
 *  This always returns FALSE if the sets are of different type.
 */

extern int Subset(CONST struct set_t *,CONST struct set_t *);
/**< 
 *  int Subset(s1,s2);
 *  struct set_t *s1,*s2;
 *  Returns a true value if s1 is contained in s2.  It always returns FALSE
 *  if the sets are of different type(even if they are empty sets).
 *
 *  TRUE  if (i IN s1) ==> (i IN s2)
 *  FALSE otherwise
 */

extern int CmpSetInstVal(CONST struct set_t *, CONST struct set_t *);
/**< 
 *  int CmpSetInstVal(s1,s2)
 *  struct set_t *s1,*s2;
 *  Returns -1, 0, 1 from comparing s1,s2.
 */


/**< 
 *  Some ordered set processing. The elements of the below sets are
 *  not necessarily unique and not necessarily ordered. In this way they
 *  behave more like lists. For TRUE sets use the InsertInteger and
 *  InsertString functions above.
 */

extern void AppendIntegerElement(struct set_t *, long int);
/**< 
 *  void AppendIntegerElement(set,i);
 *  struct set_t *set;
 *  long int i;
 *  This function will append an integer to a set. In so doing it will NOT
 *  attempt to sort the elements of the set or to make the elements of the
 *  set unique. In this way the set is treated as a list.
 */

extern void AppendStringElement(struct set_t *, symchar *);
/**< 
 *  struct set_t *set;
 *  symchar *str;
 *  This function will append an string to a set. In so doing it will NOT
 *  attempt to sort the elements of the set or to make the elements of the
 *  set unique. In this way the set is treated as a list.
 */

#endif /**< __SETINSTVAL_H_SEEN__ */
