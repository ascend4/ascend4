/*
 *  Ascend Instance Tree Type Visit Implementation
 *  by Tom Epperly, Ben Allan, Vicente Rico-Ramirez
 *  8/16/89
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: visitinst.h,v $
 *  Date last modified: $Date: 1997/12/20 17:51:56 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Ben Allan, Vicente' Rico-Ramirez
 *  Based on
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Ascend Instance Tree Type Visit Implementation.
 *  <pre>
 *  When #including visitinst.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef __VISITINST_H_SEEN__
#define __VISITINST_H_SEEN__

typedef void (*VisitProc)(struct Instance *);
/**< 
 *  Typedef for a function that takes an instance;
 *  Used with VisitInstanceTree().
 */

typedef void (*VisitTwoProc)(struct Instance *,VOIDPTR);
/**<
 *  Typedef for a function that takes an instance and a user pointer;
 *  Used with VisitInstanceTreeTwo().
 */

typedef void (*IndexedVisitProc)(struct Instance *, unsigned long *,
                                 int, VOIDPTR);
/**<
 *  Used with IndexedVisitInstanceTree().
 *  fcn(i,llist,activelen,userdata);<br>
 *  The user supplied 'fcn' must accept the list of child indices from the
 *  root to the instance given and llist[0..activelen-1] contain
 *  valid data, though llist may have a higher capacity.
 *  If the user function needs to know the higher capacity or the
 *  root instance of the visit, that is it's job to keep track of in
 *  userdata.
 */

/** Mapinfo visit info. */
enum visitmap_enum {
  vimap_ERROR = -1, /**< mapinfo is not valid */
  vimap_DOWN,       /**< mapinfo represents visiting a child of inst */
  vimap_UP          /**< mapinfo represents returning from completed inst */
};

/**
 * Mapping node for instances seen maps.
 * The information recorded is redundant in some sense, but we
 * don't know the minimum desired by a given client.
 */
struct visitmapinfo {
  struct Instance *parent;  /**< inst came from to context. maybe NULL */
  struct Instance *context; /**< inst being visited. never NULL */
  enum visitmap_enum dir;   /**< see above */
  int last;                 /**< map index of last time left,if UP.else -1 */
  unsigned long child;      /**< child going to from inst */
};

/*
 *  Tree procedures
 */

extern void ResetVisitCounts(struct Instance *inst);
/**< 
 *  <!--  void ResetVisitCounts(i);                                    -->
 *
 *  Resets the visit count of all instances in the tree rooted at i to 0.
 *  i must NOT be a fundamental instance (i.e. atom child) or a SIM_INST.
 *  Set global_visit_num = 0 after calling this on all instances.
 *  This will prevent integer overflow of visit counters in long
 *  duration ASCEND sessions.<br><br>
 *
 *  Note that because of the way prototypes and universal instances work,
 *  this function must be called on EVERY existing instance or the compiler
 *  and most ASCEND clients will malfunction SEVERELY.
 *  Instances in prototype libraries.
 *  Instances in all simulations.
 *  Who knows where else they lurk?
 *  Given these restrictions, it is obvious that it is an INTERFACE
 *  job to keep an eye on global_visit_num and call reset for everything
 *  in the environment and then set global_visit_num to 0.
 *
 *  @bug At present, no known interface calls this function yet.
 */

ASC_DLLSPEC(void ) SilentVisitInstanceTree(struct Instance *inst,
                                    VisitProc proc, int depth, int leaf);
/**<
 *  <!--  void SilentVisitInstanceTree(inst,proc,depth,leaf)           -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitProc proc;                                              -->
 *  <!--  int depth,leaf;                                              -->
 *
 *  Visit every node of an instance tree and call proc for each node.
 *  proc will be called on inst regardless of leaf, even it is an
 *  atom or child of an atom.<br><br>
 *
 *  SilentVisitInstanceTree() performs the same actions as
 *  FastVisitInstanceTree() and SlowVisitInstanceTree().  It is the
 *  fastest, however, since it tells you nothing when it encounters NULL.<br><br>
 *
 *  This function is an implementation function for VisitInstanceTree()
 *  when NDEBUG is defined.
 *
 *  @param inst   The Instance to visit.  Should not be a SIM_INST.
 *                Simulation instances appear always visited.
 *  @param proc   proc to call.  Will not be called with NULL instances.
 *  @param depth  Controls the order of the call.  If depth is true, the
 *                visitation will be bottom up (i.e. children before
 *                parents); otherwise, the visitation will be top down
 *                (i.e. parents before children).
 *  @param leaf   Determines if the children on atoms will be visited.
 *                If leaf is true, the children of atoms will be visited;
 *                otherwise, it won't.
*/

extern void FastVisitInstanceTree(struct Instance *inst,
                                  VisitProc proc, int depth, int leaf);
/**<
 *  <!--  void FastVisitInstanceTree(inst,proc,depth,leaf)             -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitProc proc;                                              -->
 *  <!--  int depth,leaf;                                              -->
 *
 *  Visit every node of an instance tree and call proc for each node.
 *  proc will be called on inst regardless of leaf, even it is an
 *  atom or child of an atom.<br><br>
 *
 *  FastVisitInstanceTree() performs the same actions as
 *  SilentVisitInstanceTree() and SlowVisitInstanceTree().  It is
 *  intermediate in speed since it tells you when, but not where, it
 *  finds NULL children.
 *
 *  @param inst   The Instance to visit.  Should not be a SIM_INST.
 *                Simulation instances appear always visited.
 *  @param proc   proc to call.  Will not be called with NULL instances.
 *  @param depth  Controls the order of the call.  If depth is true, the
 *                visitation will be bottom up (i.e. children before
 *                parents); otherwise, the visitation will be top down
 *                (i.e. parents before children).
 *  @param leaf   Determines if the children on atoms will be visited.
 *                If leaf is true, the children of atoms will be visited;
 *                otherwise, it won't.
 */

ASC_DLLSPEC(void ) SlowVisitInstanceTree(struct Instance *inst, 
                                  VisitProc proc, int depth, int leaf);
/**<
 *  <!--  void SlowVisitInstanceTree(inst,proc,depth,leaf)             -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitProc proc;                                              -->
 *  <!--  int depth,leaf;                                              -->
 *
 *  Visit every node of an instance tree and call proc for each node.
 *  proc will be called on inst regardless of leaf, even it is an
 *  atom or child of an atom.<br><br>
 *
 *  SlowVisitInstanceTree() performs the same actions as
 *  SilentVisitInstanceTree() and FaseVisitInstanceTree().  It is
 *  the slowest in speed since it can tell you where it finds NULL
 *  children. Information costs time.
 *
 *  This function is an implementation function for VisitInstanceTree()
 *  when NDEBUG is not defined (i.e. in debug mode).
 *
 *  @param inst   The Instance to visit.  Should not be a SIM_INST.
 *                Simulation instances appear always visited.
 *  @param proc   proc to call.  Will not be called with NULL instances.
 *  @param depth  Controls the order of the call.  If depth is true, the
 *                visitation will be bottom up (i.e. children before
 *                parents); otherwise, the visitation will be top down
 *                (i.e. parents before children).
 *  @param leaf   Determines if the children on atoms will be visited.
 *                If leaf is true, the children of atoms will be visited;
 *                otherwise, it won't.
 */

#ifndef NDEBUG
#define VisitInstanceTree(a,b,c,d) SlowVisitInstanceTree(a,b,c,d)
/**<  @see SlowVisitInstanceTree(). */
#else
#define VisitInstanceTree(a,b,c,d) SilentVisitInstanceTree(a,b,c,d)
/**<  @see SilentVisitInstanceTree(). */
#endif
/* OLD GROUP COMMENT */
/*
 *  macro VisitInstanceTree(inst,proc,depth,leaf)
 *
 *  void FastVisitInstanceTree(inst,proc,depth,leaf)
 *  void SlowVisitInstanceTree(inst,proc,depth,leaf)
 *  void SilentVisitInstanceTree(inst,proc,depth,leaf)
 *  struct Instance *inst;
 *  VisitProc proc;
 *  int depth,leaf;
 *
 *  i should not be a SIM_INST. Simulation instances appear always visited.
 *  This procedure will visit every node of an instance tree and call
 *  proc for each node, subject to the following conditions:
 *
 *  proc will not be called with NULL instances.
 *
 *  The integer leaf determines if this procedure will
 *  visit the children on atoms.  If leaf is true, if will visit the children
 *  of atoms; otherwise, it won't.
 *
 *  depth controls the order of the call.  If depth is true, the
 *  visitation will be bottom up (i.e., children before parents); otherwise,
 *  the visitation will be top down(i.e, parents before children).
 *
 *  In all cases, proc will be called on the given inst
 *  (even it is an atom or child of an atom regardless of leaf).
 *
 *  FastVisitInstanceTree is just as above except that it
 *  tells you when, but not where, it finds NULL children.
 *
 *  SlowVisitInstanceTree is just as above except that it
 *  can tell you where it finds NULL children. Information costs time.
 *
 *  SilentVisitInstanceTree is the fastest as it
 *  tells you nothing when it encounters NULL.
 */

extern void IndexedVisitInstanceTree(struct Instance *inst,
                                     IndexedVisitProc proc,
                                     int depth,
                                     int leaf,
                                     unsigned long **llist,
                                     unsigned int * llen,
                                     VOIDPTR userdata);
/**<
 *  <!--  void IndexedVisitInstanceTree(inst,proc,depth,leaf,llist,llen,userdata) -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  IndexedVisitProc proc;                                       -->
 *  <!--  int depth,leaf;                                              -->
 *  <!--  unsigned long **llist;                                       -->
 *  <!--  unsigned int *llen;                                          -->
 *  <!--  VOIDPTR userdata; (Treated as in VisitInstanceTreeTwo.)      -->
 *
 *  As SilentVisitInstancetree(), except that at each visited node,
 *  proc is called with llist and the current length of the data
 *  in the llist. The data in the llist is the child list index of
 *  each child visited, and llen then is obviously the relative depth.
 *  So, for example, llen = 0 when proc is called on the instance
 *  given, and llen = 1 with llist[0] = 2 when visiting the second
 *  child of the instance given.<br><br>
 *
 *  llist is a handle to an array of unsigned long. If the depth
 *  visited exceeds *llen, then llist will be reallocated and both
 *  *llist and *llen will be updated so that the caller is made
 *  aware of the change.
 */

extern void SilentVisitInstanceTreeTwo(struct Instance *inst,
                                       VisitTwoProc proc,
                                       int depth,
                                       int leaf,
                                       VOIDPTR userdata);
/**<
 *  <!--  void SilentVisitInstanceTreeTwo(inst,proc,depth,leaf,userdata-->)
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitTwoProc proc;                                           -->
 *  <!--  int depth, leaf;                                             -->
 *  <!--  VOIDPTR userdata;                                            -->
 *  This function has exactly the same functionality as SilentVisitInstanceTree(),
 *  *except* that it allows the user to pass in an optional argument.
 *  This may be a pointer to anything and must be cast appropriately.
 *  The use of this function arises, e.g., when visiting the instance tree
 *  and based on certain queries needing to append to a list. This second
 *  argument may now be a pointer to that list. Otherwise a global list
 *  structure would have to be created. This just avoids the clutter of
 *  global variables.
 */

extern void VisitInstanceTreeTwo(struct Instance *inst,
                                 VisitTwoProc proc,
                                 int depth,
                                 int leaf,
                                 VOIDPTR userdata);
/**<
 *  <!--  void VisitInstanceTreeTwo(inst,proc,depth,leaf,userdata)     -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitTwoProc proc;                                           -->
 *  <!--  int depth,leaf;                                              -->
 *  <!--  VOIDPTR userdata;                                            -->
 *  This function has exactly the same functionality as FastVisitInstanceTree(),
 *  *except* that it allows the user to pass in an optional argument.
 *  This may be a pointer to anything and must be cast appropriately.
 *  The use of this function arises, e.g., when visiting the instance tree
 *  and based on certain queries needing to append to a list. This second
 *  argument may now be a pointer to that list. Otherwise a global list
 *  structure would have to be created. This just avoids the clutter of
 *  global variables.<br><br>
 *
 *  Note: the Slow mode is not supported.
 */

extern void SilentVisitInstanceFringeTwo(struct Instance *inst,
                                         VisitTwoProc proc1,
                                         VisitTwoProc proc2,
                                         int depth,
                                         int leaf,
                                         VOIDPTR userdata);
/**<
 *  <!--  void SilentVisitInstanceFringeTwo(inst,proc1,proc2,depth,leaf,userdata) -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitTwoProc proc1,proc2;                                    -->
 *  <!--  int depth, leaf;                                              -->
 *  <!--  VOIDPTR userdata;                                            -->
 *
 *  This function has exactly the same functionality as
 *  SilentVisitInstanceTreeTwo(), *except* when an instance is seen again,
 *  it is revisited with proc2,  but there the revisit stops --
 *  the children of revisited instances are not revisited.<br><br>
 *
 *  This is a damned weird function. With it, UNIVERSALs are seen
 *  more than once, in the case of Dummy lots of times. The user
 *  is responsible for doing the right thing with redundant visits.
 */

extern void VisitInstanceFringeTwo(struct Instance *inst,
                                   VisitTwoProc proc1,
                                   VisitTwoProc proc2,
                                   int depth,
                                   int leaf,
                                   VOIDPTR userdata);
/**<
 *  <!--  void VisitInstanceFringeTwo(inst,proc1,proc2,depth,leaf,userdata) -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitTwoProc proc1,proc2;                                    -->
 *  <!--  int depth, leaf;                                             -->
 *  <!--  VOIDPTR userdata;                                            -->
 *
 *  This function has exactly the same functionality as
 *  VisitInstanceTreeTwo(), *except* when an instance is seen again,
 *  it is revisited with proc2, but there the revisit stops --
 *  the children of revisited instances are not revisited.<br><br>
 *
 *  This is a damned weird function. With it, UNIVERSALs are seen
 *  more than once, in the case of Dummy lots of times. The user
 *  is responsible for doing the right thing with redundant visits.
 */

extern void SilentVisitInstanceRootsTwo(struct Instance *inst,
                                        VisitTwoProc proc,
                                        int depth,
                                        VOIDPTR userdata);
/**<
 * <!--  void SilentVisitInstanceRootsTwo(i,proc,depth,userdata);      -->
 *
 * Like SilentVisitInstanceTreeTwo(), except goes up the tree instead
 * of down, so leaf is not needed. The meaning of depth is sort of
 * inverted also. Eg. when visiting the instances in the name a.b.c,
 * depth = 0 --> the VisitTwoProc will be applied to c, then b, then a.
 */

extern struct visitmapinfo *MakeVisitMap(struct Instance *inst, unsigned long *maplen);
/**<
 * <!--  map = MakeVisitMap(i,&maplen);                                -->
 * <!--  struct Instance *i; (input)                                   -->
 * <!--  struct visitmapinfo *map;                                     -->
 * <!--  unsigned long *maplen; (output)                               -->
 *
 * Returns a map of the instances which will be encountered (and how)
 * in the course of a VisitInstance(). User should free map when done with it.
 * maplist is as described by the comments for visitmapinfo above.
 * The map returned is actually size maplen+1, and the last element may be
 * used to store checksums or other info to detect array overrun. The last
 * element is initialized to 0/NULL/ERROR at creation.<br><br>
 *
 * The visitation indicated by this mapping (both bottom-up and top-down) is:
 * - v<i>
 * - v<i>.b
 * - ^<i>.b.f
 * - ^<i>.b
 * - v<i>
 * - ^<i>.c
 * - ^<i>
 *
 * To avoid reallocation for what is potentially a darn big piece of
 * memory, we visit the tree to get size then again to get data.
 */

#endif  /* __VISITINST_H_SEEN__ */

