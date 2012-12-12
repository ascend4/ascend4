/*
 *  Ascend Instance Name Tree Visit Implementation
 *  by Benjamin Andrew Allan
 *  9/19/97
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: visitlink.h,v $
 *  Date last modified: $Date: 1997/12/20 17:51:58 $
 *  Last modified by: $Author: ballan $
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Ascend Instance Name Tree Visit Implementation.
 *  <pre>
 *  When #including visitname.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef ASC_VISITNAME_H
#define ASC_VISITNAME_H

/**	@addtogroup compiler_inst Compiler Instance Hierarchy
	@{
*/

typedef void (*VisitNameProc)(struct Instance *,struct gl_list_t *);
/**<
 *  Typedef for a function that takes an instance;
 *  Used with VisitNameTree() (if/when implemented).
 */

typedef void (*VisitNameTwoProc)(struct Instance *,
                                 struct gl_list_t *,
                                 VOIDPTR);
/**<
 *  <!--  func(i,path,userdata);                                       -->
 *  Typedef for a function that takes an instance,
 *  a path to the instance, and a user pointer,
 *  Used with VisitNameTreeTwo().
 *  The name path is pairs of childnum/instance* in gl_list_t form
 *  indicating how the current node was reached.
 *  The path is subject to change, so if your function needs it
 *  to persist, copy it to your own data.<br><br>
 *
 *  For example, the length of the path is 0 at the root instance and
 *  at any instance below the root:
 *  gl_length(path) == 2*(number links followed),
 *  (unsigned long)gl_fetch(path,1) number of child of root followed
 *  gl_fetch(path,2) == child of root.
 *
 *  The last element of the path will be a parent OF the instance i,
 *  and the len-1th element of the path will be the child index of i
 *  in last element.
 *  Because ASCEND creates instance trees that are acyclic, no instance
 *  will ever appear in the path twice. Child numbers may repeat, obviously.
 */

/*
 *  Tree procedures
 */
#ifdef NDEBUG
#define VisitNameTreeTwo(a,b,c,d,e,f) \
  SilentVisitNameTreeTwo((a), (b), (c), (d), (e), (f))
/**<  @see SilentVisitNameTreeTwo(). */
#else
#define VisitNameTreeTwo(a,b,c,d,e,f) \
  SlowVisitNameTreeTwo((a), (b), (c), (d), (e), (f))
/**<  @see SlowVisitNameTreeTwo(). */
#endif
extern void SilentVisitNameTreeTwo(struct Instance *inst,
                                   VisitNameTwoProc proc,
                                   int depth,
                                   int leaf,
                                   int anon_flags,
                                   VOIDPTR userdata);

/**<
 *  <!--  void SilentVisitNameTreeTwo(inst,proc,depth,leaf,anon_flags,userdata); -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitNameTwoProc proc;                                       -->
 *  <!--  int depth,leaf,anon_flags;                                   -->
 *  <!--  VOIDPTR userdata;                                            -->
 *
 *  Visit every node of an instance tree and call proc for each node.
 *  proc will be called on inst even if it matches anon_flags, or is an
 *  atom or child of an atom regardless of leaf.<br><br>
 *
 *  SilentVisitInstanceTree() performs the same actions as
 *  SlowVisitNameTree().  It is faster, however, since it tells you
 *  nothing when it encounters NULL.<br><br>
 *
 *  This function is an implementation function for VisitNameTreeTwo()
 *  when NDEBUG is defined.
 *
 *  @param inst       The Instance to visit.  Should not be a SIM_INST.
 *                    Simulation instances appear always visited.
 *  @param proc       proc to call.  Will not be called with NULL instances.
 *  @param depth      Controls the order of the call.  If depth is true, the
 *                    visitation will be bottom up (i.e. children before
 *                    parents); otherwise, the visitation will be top down
 *                    (i.e. parents before children).
 *  @param leaf       Determines if the children on atoms will be visited.
 *                    If leaf is true, the children of atoms will be visited;
 *                    otherwise, it won't.
 *  @param anon_flags Controls whether or not interesting instances are visited.
 *                    If (anon_flags & GetAnonFlags(i)) != 0 then i is not visited.
 *  @param userdata   Pointer to data to pass as the last argument to the
 *                    user-supplied proc.
*/

extern void SlowVisitNameTreeTwo(struct Instance *inst,
                                 VisitNameTwoProc proc,
                                 int depth,
                                 int leaf,
                                 int anon_flags,
                                 VOIDPTR userdata);
/**<
 *  <!--  void SlowVisitNameTreeTwo(inst,proc,depth,leaf,anon_flags,userdata); -->
 *  <!--  struct Instance *inst;                                       -->
 *  <!--  VisitNameTwoProc proc;                                       -->
 *  <!--  int depth,leaf,anon_flags;                                   -->
 *  <!--  VOIDPTR userdata;                                            -->
 *
 *  Visit every node of an instance tree and call proc for each node.
 *  proc will be called on inst even if it matches anon_flags, or is an
 *  atom or child of an atom regardless of leaf.<br><br>
 *
 *  SlowVisitInstanceTree() performs the same actions as
 *  SilentVisitNameTree().  It is slower, however, since it tells you
 *  can tell you where it finds NULL children. Information costs time.<br><br>
 *
 *  This function is an implementation function for VisitInstanceTree()
 *  when NDEBUG is not defined (i.e. debug mode).
 *
 *  @param inst       The Instance to visit.  Should not be a SIM_INST.
 *                    Simulation instances appear always visited.
 *  @param proc       proc to call.  Will not be called with NULL instances.
 *  @param depth      Controls the order of the call.  If depth is true, the
 *                    visitation will be bottom up (i.e. children before
 *                    parents); otherwise, the visitation will be top down
 *                    (i.e. parents before children).
 *  @param leaf       Determines if the children on atoms will be visited.
 *                    If leaf is true, the children of atoms will be visited;
 *                    otherwise, it won't.
 *  @param anon_flags Controls whether or not interesting instances are visited.
 *                    If (anon_flags & GetAnonFlags(i)) != 0 then i is not visited.
 *  @param userdata   Pointer to data to pass as the last argument to the
 *                    user-supplied proc.
*/
/* OLD GROUP COMMENT */
/*
 *  macro VisitNameTreeTwo(inst,proc,depth,leaf,anon_flags,userdata);
 *  void SilentVisitNameTreeTwo(inst,proc,depth,leaf,anon_flags,userdata);
 *  void SlowVisitNameTreeTwo(inst,proc,depth,leaf,anon_flags,userdata);
 *  struct Instance *inst;
 *  VisitNameTwoProc proc;
 *  int depth,leaf,anon_flags;
 *  VOIDPTR userdata;
 *
 *  inst should not be a SIM_INST. Simulation instances appear always visited.
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
 *  the visitation will be top down (i.e, parents before children).
 *
 *  anon_flags controls whether or not interesting instances are visited.
 *  If (anon_flags & GetAnonFlags(i)) != 0 then i is not visited.
 *
 *  In all cases, proc will be called on the given inst
 *  (even if it matches anon_flags or is an atom or child
 *  of an atom regardless of leaf).
 *
 *  SlowVisitNameTree is as above and it
 *  can tell you where it finds NULL children. Information costs time.
 *
 *  SilentVisitNameTree is the fastest as it
 *  tells you nothing when it encounters NULL.
 *
 *  userdata is passed as the last argument to the user supplied proc.
 */

/* @} */

#endif /* ASC_VISITNAME_H */

