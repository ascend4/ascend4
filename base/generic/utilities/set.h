/**< 
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

/**< 
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
#ifndef	_SET_H
#define _SET_H

/**< requires
# #include "base.h"
# #include "mask.h"
# #include "ascmalloc.h"
*/

#define	set_size(n) \
   (((n)+(WORDSIZE-1))/WORDSIZE)
/**< 
 *  Returns the number of unsigned ints required
 *  to store a set of n bits.
 *  WORDSIZE is computed in base.h
 */

#define	set_create(n) \
   (set_size(n)>0?((unsigned *)ascmalloc(sizeof(unsigned)*set_size(n))):NULL)
/**< 
 *  Returns a pointer to a newly created set
 *  of bit-size n.  The initial contents are
 *  garbage.
 */

#define	set_destroy(set) \
   (ascfree((POINTER)set))
/**< 
 *  Destroys the set.
 */

#define	set_ndx(k) \
   ((k)/WORDSIZE)
/**< 
 *  Index into array of unsigned
 *  where element k's status is found
 */

/**< 
 *  previous definition: trying to get rid of mask.h
 *
 *#define	set_mask(k) \
 *   mask_I_E((k)%WORDSIZE)
 */
#define	set_mask(k) \
   (((unsigned)1) << ((k)%WORDSIZE))
/**< 
 *  Returns an integer with the bit
 *  corresponding to element k turned on
 */

#define	set_is_member(set,k) \
   ((set[set_ndx(k)] & set_mask(k)) != 0)
/**< 
 *  Returns TRUE if k belongs to the given set, FALSE
 *  otherwise.  It is assumed that 0 <= k < n, where n is the
 *  bit-size of the set.
 */

extern unsigned int *set_null(unsigned int *, int);
/**< 
 *  set_null(set,n)
 *  unsigned *set;
 *  int n;
 *
 *  The set is cleared (so that it now has no elements).
 *  The bit-size must be passed in. Returns pointer to set.
 */

extern void set_change_member(unsigned int *  , int  , boolean );
/**<     
 *  set_change_member(set,k,value)
 *  unsigned *set;
 *  int k;
 *  boolen value;
 *
 *  If value==TRUE, then k is added to the set.
 *  Otherwise k is taken out of the set.  It is
 *  assumed that 0 <= k < n.
 */

#ifdef THIS_IS_DEAD_CODE
#define	set_chk_is_member(set,k,n) \
   ((k)>=0 && (k)<(n) && set_is_member(set,k))
/**< 
 *  Make sure k is within the limits of the set indeces
 *  before looking to see if it is a member.
 */

extern unsigned int *set_copy(unsigned int *, unsigned int *, int, int);
/**< 
 *  set_copy(set,target,n,n2)
 *  unsigned *set, *target;
 *  int n,n2;
 *  
 *  Copies one set to another.  The bit size of the source and
 *  target must be given.  If the size of the target is less
 *  than the source, then the set is truncated.  If the size of
 *  the target is more than the source, then the set is extended
 *  with 0's.  It is assumed that the target has already been
 *  created. Returns the pointer to target.
 */

extern void set_change_member_rng(unsigned int *  , int  , int  , boolean );
/**< 
 *  set_change_member_rng(set,k1,k2,value)
 *  unsigned *set;
 *  int k1, k2;
 *  boolean value;
 *
 *  Changes the membership status for all elements
 *  in k1..k2 (see set_change_member).  It is assumed
 *  that 0 <= k1,k2 < n.
 */

extern int set_find_next(unsigned int *  , int  , int );
/**< 
 *  next = set_find_next(set,k,n)
 *  int next;
 *  unsigned *set;
 *  int k, n;
 *
 *  Returns the first member of set greater than k.
 *  If k=-1 upon entering, the minimum of the set is
 *  returned.  If return>=n upon exiting, then there is no
 *  member in the set greater than k.
 */

extern int set_count(unsigned int *  , int );
/**< 
 *  count = set_count(set,n)
 *  int count;
 *  unsigned *set;
 *  int n;
 *
 *  Returns the cardinality of the set.
 */
 
extern unsigned *set_complement(unsigned int *  , int );
/**< 
 *  set_complement(set,n)
 *  unsigned *set;
 *  int n;
 *
 *  Removes all elements which are currently in the
 *  set and adds all elements which were not.
 *  Returns pointer to set.
 */

extern unsigned *set_intersect(unsigned int *, unsigned int *, int, int);
/**< 
 *  set_intersect(set,set2,n,n2)
 *  unsigned *set, *set2;
 *  int n,n2;
 *
 *  Replaces set with the intersection of set and set2.
 *  The bit-sizes of each set must be given (if they are
 *  different, then set2 is effectively extended or truncated
 *  for purposes of computing intersection, although the
 *  size change does not actually occur).
 *  Returns the pointer to set, which has been modified.
 */

extern unsigned *set_union(unsigned int *  , unsigned int *  , int  , int );
/**< 
 *  set_union(set,set2,n,n2)
 *  unsigned *set, *set2;
 *  int n,n2;
 *
 *  Replaces set with the union of set and set2.
 *  Size mismatch handled as for intersection.
 *  Returns the pointer to set, which has been modified.
 */
#endif /**< THIS_IS_DEAD_CODE */

#endif  /**< _SET_H  */
