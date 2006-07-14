/*
 *  anonmerge.c       
 *  Minimalist merge detection for anonymous type detection.
 *  by Benjamin Andrew Allan
 *  Created September 21, 1997
 *  Copyright 1997 Carnegie Mellon University.
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: anonmerg.h,v $
 *  Date last modified: $Date: 1997/12/20 17:50:59 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
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
 *  COPYING.
 */

/** @file
 *  Minimalist merge detection for anonymous type detection.
 *  <pre>
 *  When #including anonmerg.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *
 *  The idea is to find and mark all the merges that need to be
 *  marked to disambiguate the anontype classification scheme
 *  which can be mislead into believing identical anontypes when
 *  one instance has some children merged but the other doesn't.
 *  Put another way, we want to record the minimum set of merges
 *  that can account for the identity of all variables/constants
 *  in any relation,logical relation, or when statement.
 *
 *  Assumptions:
 *
 *     - We will perform this step at a point in the compilation
 *       sequence where the instance structures are set (any time after
 *       phase 1). Instances cannot move while detecting merges.
 *     - The tmpnums, anon_flags, and interface pointers are not in use.
 *       (To be safe we will PushInterfacePointers. Tmpnums left 0, and
 *       anon_flags are left 0 unless the instance is UNIVERSAL-like.)
 *     - Universal instances need not be investigated for merge
 *       properties. That is, when enumerating a namespace and we
 *       encounter a universal instance, we will skip it.
 *       We must discover anonymous universals (formal types of which there
 *       is only 1 instance) and do.
 *
 *  Desirables:
 *
 *     - We want to mark instances with a minimal set of merges that accounts
 *       for the deviations in their name space, i.e. we don't want to collect
 *       the merges for all a.b.x, c.b.x if a.b, c.b are merged.
 *     - We want to mark the instance which is the smallest scope containing
 *       both merges. That is, a.b.c, a.b.d are merged, we want to mark a.b,
 *       not a.
 *
 *  Problems:
 *
 *     Because there is no simple indexing of parents of an instance
 *     (i.e. I can't travel from a parent p to a child c and easily
 *     know which of the back (parent) links of c I should follow
 *     to return to the parent) we must store paths for navigation
 *     in terms of child number/instance context pairs. Following
 *     back a parent trail requires a search for the child being
 *     left which is unacceptable.
 *
 *  These functions maintain all their own data and do not use any
 *  global information, EXCEPT that they use instance InterfacePtrs
 *  tmpnums and anon_flags.
 * </pre>
 *
 * (the following is moved from anonmerg.c, needs editing and deduping against
 * above comments)
 *
 *  The idea is to find and mark all the merges that need to be
 *  marked to disambiguate the anontype classification scheme
 *  which can be mislead into believing identical anontypes when
 *  one instance has some children merged but the other doesn't.
 *  Put another way, we want to record the minimum set of merges
 *  that can account for the identity of all variables/constants
 *  in any relation,logical relation, or when statement.
 *
 *  Assumptions:
 *  - We will perform this step at a point in the compilation
 *  sequence where the instance structures are set (any time after
 *  phase 1). Instances cannot move while detecting merges.
 *  - The tmpnums and interface pointers are not in use. (To be
 *  safe we will PushInterfacePointers. Tmpnums left 0.)
 *  - Universal instances need not be investigated for merge
 *  properties. That is, when enumerating a namespace and we
 *  encounter a universal instance, we will skip it and all
 *  its children. We should probably introduce a child-of-universal
 *  bit in the ChildList info and disallow merges/aliases of parts
 *  of universals with the outside in order to prevent graphs of
 *  universal instances from having connections anywhere except at
 *  the top. There are unofficial universals, and we will detect them.
 *
 *
 *  Desirables:
 *  - We want to mark instances with a minimal set of merges that accounts
 *  for the deviations in their name space, i.e. we don't want to collect
 *  the merges for all a.b.x, c.b.x if a.b, c.b are merged.
 *  - We want to mark the instance which is the smallest scope containing
 *  both merges. That is, a.b.c, a.b.d are merged, we want to mark a.b,
 *  not a.
 *
 *  Problems:
 *  Because there is no simple indexing of parents of an instance
 *  (i.e. I can't travel from a parent p to a child c and easily
 *  know which of the back (parent) links of c I should follow
 *  to return to the parent) we must store paths for navigation
 *  in terms of child number/instance context pairs. Following
 *  back a parent trail requires a search for the child being
 *  left which is unacceptable. This is the same as an io NameNode,
 *  unless we decide to play a reference count game to avoid memory
 *  consumption.
 *
 *  Much of this process is readily explained in terms of a global
 *  list of childnumber/context pairs which maps the entire namespace.
 *  Fortunately we don't need to build this list, we can build parts
 *  of it only, but the overall algorithm becomes obscured.
 *  Because we are mapping the LINK space, rather than the node space,
 *  we have to write our own bizarre set of VisitName functions.
 *  We will have to visit each link between any two objects once.
 *
 *  These functions maintain all their own data and do not use any
 *  global information, EXCEPT that they use instance InterfacePtrs.
 *  For this reason they are not thread-safe. They also build data
 *  structures with references to instances, so no instance moving
 *  actions should be performed while there are structures from this
 *  module in use.
 */

#ifndef __ANONMERG_H_SEEN__
#define __ANONMERG_H_SEEN__

/**
 * If want to collect/report some statistics, set to 1.
 * what statistics are written and where depends on flags
 * defined in anonmerg.c. this is for debugging purposes
 * only.
 */
#define AMSTAT 0

/**
 * <!--  VOIDPTR vp = Asc_AnonMergeMarkIPs(root);                      -->
 * <!--  struct Instance *root                                         -->
 * This function finds merged instances, anon or otherwise, and
 * records the merges at the scope most appropriate.
 * On return, an InterfacePointers in the tree of root
 * point to some data we understand iff anonflags contains
 * data we understand, so don't mess with anonflags while we
 * have marked instances.
 * When done using these IPs, the caller should call
 * AnonMergeDestroyIPs(vp);
 * This function uses the push/pop protocol for ips, so ip data
 * of other clients may be lost if Unmark is not called properly.
 * Generally, both functions should be called from the same scope.<br><br>
 *
 * ! ! Assumes that tmpnums are all 0 on entry, and leaves any
 * it has touched 0 on exit.<br><br>
 *
 * Does not record recursive merges, i.e. if a,b ARE_THE_SAME,
 * don't record a.i,b.i ARE_THE_SAME. This is simple to detect
 * as a.i,b.i have the same childnumber/instanceparent pair.<br><br>
 *
 * Does not record 'merges' that are explicitly accounted for
 * by an ALIASES statement in a type definition.
 */
extern VOIDPTR Asc_AnonMergeMarkIPs(struct Instance *root);

/**
 * <!--  int cmp = Asc_AnonMergeCmpInstances(i1,i2);                   -->
 * <!--  CONST struct Instance *i1, *i2;                               -->
 * Returns the comparison of the merge information stored in two
 * instances.  These instances must of the same formal type and have
 * children of the same anonymous types. It doesn't make
 * sense to call the function on ATOM-like instances, since they
 * can have no deeper merged structures.
 * Objects which are not supposed to have a list of merges will
 * always return 2.
 * If the comparison is valid, it will return 0,1,-1.
 * UNIVERSAL instances will always return 0. (Think about it.)
 * Comparing i1 to i1 is fatal.
 */
extern int Asc_AnonMergeCmpInstances(CONST struct Instance *i1,
                                     CONST struct Instance *i2);

/**
 * <!--  AnonMergeUnmarkIPs(vp)                                        -->
 * Frees data structure returned by AnonMergeMarkIPs.
 * Neglect to call this and you will cause trouble.
 * You really haven't the slightest need to know what the
 * contents of vp are.
 */
extern void Asc_AnonMergeUnmarkIPs(VOIDPTR vp);

/**
 * <!--  Asc_AnonMergeWriteList(fp,i);                                 -->
 * Writes the AnonMerge path lists for instance i.
 * The function is for debugging only. it will not
 * work except after anonmergmarkip is called and before
 * the closing unmarkip is called.
 * if i is bogus, this may crash.
 */
extern void Asc_AnonMergeWriteList(FILE *fp, struct Instance *i);

#endif  /* __ANONMERG_H_SEEN__ */

