/*
 *  Set module
 *  by Karl Westerberg
 *  Created: 6/90
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: set.h,v $
 *  Date last modified: $Date: 1997/07/18 12:04:30 $
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
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is in ../compiler.
 */

/*
 *  Contents:     Set module
 *
 *  Authors:      Karl Westerberg
 *
 *  Dates:        06/90 - original version
 *
 *  Description:  This module is responsible for primitive manipulation
 *                of sets of integers (using a binary bit map technique).
 *                In particular, the set variables have domain
 *                {k in N | k < n}, where n is a fixed constant, which
 *                we will call the (bit-)size of the set.  The sets are
 *                actually arrays of unsigned of appropriate length,
 *                and pointers to such arrays can actually be used in
 *                place of pointers returned by set_create() (although
 *                one must be careful not to destroy such sets with
 *                set_destroy()).
 */

/** @file
 *  Integer set module.
 *  This module supports primitive manipulation of sets of positive
 *  integers.  It provides functions for creating, adding to, removing
 *  from, querying, and destroying such sets.  Note that much of the
 *  module's functionality is not used by ASCEND and is deprecated.
 *  <pre>
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "utilities/ascMalloc.h"
 *  </pre>
 */

#ifndef	_set_h_seen
#define _set_h_seen

#define	set_size(n) \
   (((n)+((int)WORDSIZE-1))/(int)WORDSIZE)
/**<
 *  Returns the number of unsigned ints required to store a set of n bits.
 *  WORDSIZE is computed in ascConfig.h.  Clients should not need to use
 *  this function directly.  It is needed for other macro definitions in
 *  this file.
 *
 *  @param n int, the number of bits (or ints) to store.
 *  @return The number of unsigned ints needed as an int.
 */

#define	set_create(n) ( set_size(n)>0 ? ASC_NEW_ARRAY(unsigned,set_size(n)) : NULL )
/**<
 *  Creates a new set of size n.
 *  The size indicates the maximum integer that the set can hold.
 *  The initial contents are garbage.  The caller is responsible for
 *  deallocating the returned set using set_destroy().
 *
 *  @param n int, the size for the new set.
 *  @return A pointer to the newly-created set (as an unsigned *), or NULL
 *          if (n <= 0) or memory could not be allocated.
 */

#define	set_destroy(set) \
   (ascfree((POINTER)set))
/**<
 *  Destroys a set.
 *  The set should have been created using set_create().
 *
 *  @param set unsigned *, the set to destroy.
 *  @return No return value.
 */

#define	set_ndx(k) \
   ((k)/(int)WORDSIZE)
/**<
 *  Index into array of unsigned where element k's status is found.
 *  Clients should not need to use this function directly.  It is
 *  needed for other macro definitions in this file.
 *
 *  @param k int, the element whose index is needed.
 *  @return The index of k as an int.
 */

/*
 *  previous definition: trying to get rid of mask.h
 *
 *#define	set_mask(k) \
 *   mask_I_E((k)%WORDSIZE)
 */
#define	set_mask(k) \
   (((unsigned)1) << ((k)%WORDSIZE))
/**<
 *  Returns an integer with the bit corresponding to element k turned on.
 *  Clients should not need to use this function directly.  It is
 *  needed for other macro definitions in this file.
 *
 *  @param k int, the element for which to turn on a bit.
 *  @return The bitmask for k as an unsigned int.
 */

#define	set_is_member(set,k) \
   ((set[set_ndx(k)] & set_mask(k)) != 0)
/**<
 *  Tests whether element k belongs to a given set.
 *  It is assumed that 0 <= k < n, where n is the size of the set.
 *
 *  @param set  unsigned *, the set to check for k.
 *  @param k    int, the element to evaluate.
 *  @return TRUE if k belongs to the given set, FALSE otherwise.
 */

#define	set_chk_is_member(set,k,n) \
   ((k)>=0 && (k)<(n) && set_is_member(set,k))
/**<
 *  Tests whether element k belongs to a given set with validation.
 *  This is the same as set_is_member(), except that validation is
 *  first done to ensure k is within the range of set.
 *
 *  @param set  unsigned *, the set to check for k (non-NULL).
 *  @param k    int, the element to validate and evaluate.
 *  @param n    int, the size of the set.
 *  @return TRUE if k belongs to the given set, FALSE otherwise.
 */

ASC_DLLSPEC unsigned int *set_null(unsigned int *set, int n);
/**<
 *  Clears a set so that it has no elements.
 *  The size of the set must be indicated.
 *  set may not be NULL (checked by assertion).
 *
 *  @param set The set to clear (non-NULL).
 *  @param n   The size of the set.
 *  @return Returns a pointer to the set.
 */

ASC_DLLSPEC void set_change_member(unsigned int *set, int k, boolean value);
/**<
 *  Adds or removes an element from a set.
 *  If value==TRUE, then k is added to the set.  Otherwise k 
 *  is taken out of the set.  It is assumed that 0 <= k < n.
 *  set may not be NULL (checked by assertion).
 *
 *  @param set  The set to modify (non-NULL).
 *  @param k    The element to add or remove.
 *  @param value Flag for action:  TRUE -> k is added, FALSE -> k is removed.
 */

#ifdef THIS_IS_DEAD_CODE

extern unsigned int *set_copy(unsigned int *set, unsigned int *target,
                              int n, int n2);
/**<
 *  <!--  set_copy(set,target,n,n2)                                    -->
 *  <!--  unsigned *set, *target;                                      -->
 *  <!--  int n,n2;                                                    -->
 *
 *  Copies one set to another.  The bit size of the source and
 *  target must be given.  If the size of the target is less
 *  than the source, then the set is truncated.  If the size of
 *  the target is more than the source, then the set is extended
 *  with 0's.  It is assumed that the target has already been
 *  created. Returns the pointer to target.
 *  @deprecated This function is not in use or supported.
 */

extern void set_change_member_rng(unsigned int *set,
                                  int k1, int k2, boolean value);
/**<
 *  <!--  set_change_member_rng(set,k1,k2,value)                       -->
 *  <!--  unsigned *set;                                               -->
 *  <!--  int k1, k2;                                                  -->
 *  <!--  boolean value;                                               -->
 *
 *  Changes the membership status for all elements
 *  in k1..k2 (see set_change_member).  It is assumed
 *  that 0 <= k1,k2 < n.
 *  @deprecated This function is not in use or supported.
 */

extern int set_find_next(unsigned int *set, int k, int n);
/**<
 *  <!--  next = set_find_next(set,k,n)                                -->
 *  <!--  int next;                                                    -->
 *  <!--  unsigned *set;                                               -->
 *  <!--  int k, n;                                                    -->
 *
 *  Returns the first member of set greater than k.
 *  If k=-1 upon entering, the minimum of the set is
 *  returned.  If return>=n upon exiting, then there is no
 *  member in the set greater than k.
 *  @deprecated This function is not in use or supported.
 */

extern int set_count(unsigned int *set, int n);
/**<
 *  <!--  count = set_count(set,n)                                     -->
 *  <!--  int count;                                                   -->
 *  <!--  unsigned *set;                                               -->
 *  <!--  int n;                                                       -->
 *
 *  Returns the cardinality of the set.
 *  @deprecated This function is not in use or supported.
 */

extern unsigned *set_complement(unsigned int *set, int n);
/**<
 *  <!--  set_complement(set,n)                                        -->
 *  <!--  unsigned *set;                                               -->
 *  <!--  int n;                                                       -->
 *
 *  Removes all elements which are currently in the
 *  set and adds all elements which were not.
 *  Returns pointer to set.
 *  @deprecated This function is not in use or supported.
 */

extern unsigned *set_intersect(unsigned int *set1,
                               unsigned int *set2,
                               int n, int n2);
/**<
 *  <!--  set_intersect(set,set2,n,n2)                                 -->
 *  <!--  unsigned *set, *set2;                                        -->
 *  <!--  int n,n2;                                                    -->
 *
 *  Replaces set with the intersection of set and set2.
 *  The bit-sizes of each set must be given (if they are
 *  different, then set2 is effectively extended or truncated
 *  for purposes of computing intersection, although the
 *  size change does not actually occur).
 *  Returns the pointer to set, which has been modified.
 *  @deprecated This function is not in use or supported.
 */

extern unsigned *set_union(unsigned int *set,
                           unsigned int *set2,
                           int n, int n2);
/**<
 *  <!--  set_union(set,set2,n,n2)                                     -->
 *  <!--  unsigned *set, *set2;                                        -->
 *  <!--  int n,n2;                                                    -->
 *
 *  Replaces set with the union of set and set2.
 *  Size mismatch handled as for intersection.
 *  Returns the pointer to set, which has been modified.
 *  @deprecated This function is not in use or supported.
 */

#endif /* THIS_IS_DEAD_CODE */

#endif  /* _set_h_seen  */

