/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 2006 Carnegie Mellon University

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
	Routines to Process Set Instance Values.

	This group of routines is used to process the value fields of set
	instances.

	Requires:
	#include <stdio.h>
	#include "utilities/ascConfig.h"
	#include "compiler.h"
	#include "list.h"
*//*
	by Tom Epperly
	11/16/89
	Version: $Revision: 1.7 $
	Version control file: $RCSfile: setinstval.h,v $
	Date last modified: $Date: 1998/02/05 16:37:52 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_SETINSTVAL_H
#define ASC_SETINSTVAL_H

/**	addtogroup compiler Compiler
	@{
*/

#include <utilities/ascConfig.h>

enum set_kind {
  integer_set,  /**< set of integer values */
  string_set,   /**< set of string values */
  empty_set     /**< set with nothing in it */
};

/** Do not confuse this set with the struct Set in types.h */
struct set_t {
  enum set_kind kind;
  struct gl_list_t *list;
};

extern void InitSetManager(void);
/**<
 *  This function initializes the pool.
 *  We pool the set stubs for reuse, since there are so many used in
 *  most ASCEND models of any complexity.
 *  The pool grows as needed. It does not shrink until destroyed.
 *  It is a fatal error to call this a second time unless
 *  DestroySetManager has been called first.
 *  In the pooling operation we only keep the set stubs, not the
 *  gl_lists associated.<br><br>
 *
 *  This exits if it cannot init the pool because of insufficient
 *  memory.
 */

extern void DestroySetManager(void);
/**<
 *  Destroys the set pool. Bombs if no pool exists.
 */

extern void ReportSetManager(FILE*f);
/**<
 *  Reports on the set pool to f.
 */

extern struct set_t *CreateEmptySet(void);
/**<
 *  Creates an empty set.
 */

extern void InsertInteger(struct set_t *set, long i);
/**<
 *  Insert i into set.  You can insert i multiple times without causing
 *  problems.  Set must be non-NULL.  This will coerce an empty set type
 *  into an integer set type; after that no symbols can be inserted into
 *  the set.
 */

extern void InsertIntegerRange(struct set_t *set, long lower, long upper);
/**<
 *  Insert the elements i such that i >= lower and i <= upper.  Note if
 *  lower > upper, no elements are inserted.  This will coerce an empty
 *  set type into an integer set type.
 */

extern void InsertString(struct set_t *set, symchar *str);
/**<
 *  Insert str into set.  This will coerce an empty set type into
 *  a string set type; after which no integers can be inserted.
 */

extern struct set_t *SetUnion(CONST struct set_t *s1, CONST struct set_t *s2);
/**<
 *  Create a set which is the union of s1 and s2.  s1 and s2 are uneffected.
 *  Union with empty set is always okay.
 */

extern struct set_t *SetIntersection(CONST struct set_t *s1,
                                     CONST struct set_t *s2);
/**<
 *  Create a set which is the intersection of s1 and s2.  s1 and s2 are
 *  uneffected.  Intersection with empty set is okay.
 */

extern struct set_t *SetDifference(CONST struct set_t *s1, CONST struct set_t *s2);
/**<
 *  Create a set which is define as follows:
 *  return set := { i | (i IN s1) and (NOT (i IN s2))}
 */

extern struct set_t *CopySet(CONST struct set_t *set);
/**<
 *  Copy a set.
 */

extern int IntMember(long i, CONST struct set_t *set);
/**<
 *  Return a TRUE value if i is a member of set; otherwise, return a false
 *  value.
 */

extern int StrMember(symchar *str, CONST struct set_t *set);
/**<
 *  Return a TRUE value if str is a member of set; otherwise, return a false
 *  value.
 */

extern void DestroySet(struct set_t *set);
/**<  "Free up" set pointer. Actually returns it to the pool of set stubs. */

extern int NullSet(CONST struct set_t *set);
/**<  Testing if the set is empty.  TRUE if set is empty. */

ASC_DLLSPEC unsigned long Cardinality(CONST struct set_t *set);
/**<  Return the number of members. */

ASC_DLLSPEC symchar*FetchStrMember(CONST struct set_t *set, unsigned long n);
/**<  Returns the nth (internal sort order ) member symbol. */

ASC_DLLSPEC long FetchIntMember(CONST struct set_t *set, unsigned long n);
/**<  Returns the nth (internal sort order ) member number. */

extern void SetIterate(struct set_t *set, void (*func)());
/**<  Calls func for every element in set. */

ASC_DLLSPEC enum set_kind SetKind(CONST struct set_t *set);
/**<  Returns the type of the set. */

extern int SetsEqual(CONST struct set_t *s1, CONST struct set_t *s2);
/**<
 *  Returns a true value if the two sets are equal.  Set equality is
 *  defined as:
 *  (i IN s1) <==> (i IN s2)
 *  This always returns FALSE if the sets are of different type.
 */

extern int Subset(CONST struct set_t *s1, CONST struct set_t *s2);
/**<
 *  Returns a true value if s1 is contained in s2.  It always returns FALSE
 *  if the sets are of different type(even if they are empty sets).
 *
 *  - TRUE  if (i IN s1) ==> (i IN s2)
 *  - FALSE otherwise
 */

extern int CmpSetInstVal(CONST struct set_t *s1, CONST struct set_t *s2);
/**<
 *  Returns -1, 0, 1 from comparing s1,s2.
 */

/*
 *  Some ordered set processing. The elements of the below sets are
 *  not necessarily unique and not necessarily ordered. In this way they
 *  behave more like lists. For TRUE sets use the InsertInteger and
 *  InsertString functions above.
 */

extern void AppendIntegerElement(struct set_t *set, long int i );
/**<
 *  This function will append an integer to a set. In so doing it will NOT
 *  attempt to sort the elements of the set or to make the elements of the
 *  set unique. In this way the set is treated as a list.
 */

extern void AppendStringElement(struct set_t *set, symchar *str);
/**<
 *  This function will append an string to a set. In so doing it will NOT
 *  attempt to sort the elements of the set or to make the elements of the
 *  set unique. In this way the set is treated as a list.
 */

/* @} */

#endif /* ASC_SETINSTVAL_H */

