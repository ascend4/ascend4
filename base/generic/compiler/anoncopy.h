/*
 *  anoncopy.h
 *  by Benjamin Allan
 *  September 08, 1997
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: anoncopy.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:35 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

/** @file
 *  Anonymous type manipulation.
 *  <pre>
 *  When #including anoncopy.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "list.h"
 *         #include "instance_enum.h"
 *  </pre>
 */

#ifndef ASC_ANONCOPY_H
#define ASC_ANONCOPY_H

extern struct gl_list_t *Pass2CollectAnonProtoVars(struct Instance *i);
/**<
 * Returns a gl_list of index paths through i to reach the vars
 * occurring in relations (or relation arrays) of i.
 * i must be a MODEL.
 * Each var will only occur once in the path list returned.
 * An index path can be followed through any instance isomorphic to i
 * and will end at a variable semantically equivalent to the one
 * in i that generated the path.
 * At the expense of a visit tree call (which we need anyway)
 * this returns the list unsorted and never searched.<br><br>
 *
 * The list returned should be destroyed with
 * Pass2DestroyAnonProtoVars().
 */

extern void Pass2DestroyAnonProtoVars(struct gl_list_t *indexpathlist);
/**<
 * Deallocate the indexpathlist collected by Pass2CollectAnonProtoVars().
 */

extern void Pass2CopyAnonProto(struct Instance *proto,
                               struct BitList *protoblist,
                               struct gl_list_t *protovarindices,
                               struct Instance *i);
/**<
 * Copies all the local relations (including those in arrays)
 * of the MODEL instance proto to the instance i using only
 * local information. No global information is needed, but
 * we need to arrange that the tmpnums all start and end 0
 * so we can avoid extra 0ing of them.
 */

#endif /* ASC_ANONCOPY_H */

