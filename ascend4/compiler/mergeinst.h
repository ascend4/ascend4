/*
 *  Ascend Instance Tree Type Definitions
 *  by Tom Epperly
 *  8/16/89
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: mergeinst.h,v $
 *  Date last modified: $Date: 1997/09/08 18:08:06 $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#ifndef __MERGEINST_H_SEEN__
#define __MERGEINST_H_SEEN__


/*
 *  When #including mergeinst.h, make sure these files are #included first:
 *     #include "instance_enum.h"
 */


extern struct Instance *MergeInstances(struct Instance *,struct Instance *);
/*
 *  struct Instance *MergeInstances(i1,i2)
 *  struct Instance *i1,*i2;
 *  This is the ARE_THE_SAME operator.
 *  The return value is the merged instance or NULL if can't be done.
 *  Instances are merged from the bottom up, checking one step ahead
 *  for type compatibility.
 *  This operator is MOSTLY BROKEN due to the addition of
 *  parameterized types. In particular, MoreRefined needs to be
 *  augmented to check args that are now part of the extended
 *  type definition.
 *  This may add to or change the pending instance list.
 *  If the return is NULL, the state of i1 and i2 in structural
 *  terms is highly questionable. They may be partially merged.
 *
 * Bugs: merging two vars in a shared relation will mess up all the
 * other copies of the relation unless all of their corresponding
 * vars are also merged. Fixed for token relations, but not others.
 */

extern void PostMergeCheck(struct Instance *);
/*
 *  void PostMergeCheck(i)
 *  struct Instance *i;
 *  This should be performed after a MergeInstances call or a sequence of
 *  MergeInstances calls.  This goes through the merged instance tree and
 *  make sure the ARE_ALIKE cliques are being enforced.
 */


extern void MergeCliques(struct Instance *,struct Instance *);
/*
 *  void MergeCliques(i1,i2)
 *  struct Instance *i1,*i2;
 *  Merge the cliques represented by i1 and i2.  This does not enforce
 *  any type consistency between the clicks; it just links them together.
 *  It doesn't matter if i1 and i2 are already in the same clique.
 */

#endif
/* __MERGEINST_H_SEEN__ */
