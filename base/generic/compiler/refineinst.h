/*
 *  Ascend Instance Tree Type Definitions
 *  by Tom Epperly
 *  8/16/89
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: refineinst.h,v $
 *  Date last modified: $Date: 1997/07/18 12:33:05 $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Instance Refinement Routine and Clique Management
 *  <pre>
 *  When #including refineinst.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "child.h"
 *  </pre>
 */

#ifndef ASC_REFINEINST_H
#define ASC_REFINEINST_H

/**	@addtogroup compiler Compiler
	@{
*/

extern struct Instance *RefineInstance(struct Instance *i,
                                       struct TypeDescription *type,
                                       struct Instance *arginst);
/**<
 *  <!--  struct Instance *RefineInstance(i,type,arginst)              -->
 *  <!--  struct Instance *, *arginsti;                                -->
 *  <!--  struct TypeDescription *type;                                -->
 *  This returns the refined instance.  In most cases, the return value equals
 *  i, but sometimes it doesn't.  In general the interface should not
 *  call this routine.  Check RefineClique below.  This may change
 *  the current pending list, adding to it or changing it.
 *  Proper arginst must be supplied if the type is in any way parameterized.<br><br>
 *
 *  It is assumed that type is conformable with i's current type.
 *  The reference count for type is increased by this procedure.  This
 *  doesn't check for cliques.
 */

ASC_DLLSPEC struct Instance *RefineClique(struct Instance *i,
                                     struct TypeDescription *type,
                                     struct Instance *arginst);
/**<
 *  <!--  void RefineClique(i,type,arginst)                            -->
 *  <!--  struct Instance *i, *arginst;                                -->
 *  <!--  struct TypeDescription *type;                                -->
 *  This returns the refined instance.  In most cases, the return value equals
 *  i, but sometimes it doesn't.
 *  Proper arginst must be supplied if the type is in any way parameterized.
 *  If something bizarre happens, this will return NULL and the instances
 *  in the clique of i may be traumatized -- i should still exist, however
 *  as NULL is just an indicator of extreme angst.<br><br>
 *
 *  It is assumed that type is conformable with i's current type.
 *  The reference count for type is increased by this procedure.  This
 *  will refine the whole clique.  In general, the interface should always
 *  use this instead of RefineInstance.  This may change the current pending
 *  list, adding to it or changing it.
 */

/* @} */

#endif  /* ASC_REFINEINST_H */

