/*
 *  Ascend Instance Tree Killing
 *  by Tom Epperly
 *  8/16/89
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: destroyinst.h,v $
 *  Date last modified: $Date: 1997/07/18 12:28:54 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Ben Allan
 *  based on instance.c
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
 *  Ascend Instance Tree Killing.
 *  <pre>
 *  When #including destroyinst.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *  </pre>
 */

#ifndef ASC_DESTROYINST_H
#define ASC_DESTROYINST_H

/**	addtogroup compiler Compiler
	@{
*/

ASC_DLLSPEC void DestroyInstance(struct Instance *inst, struct Instance *parent);
/**<
 *  If parent is NULL, this will destroy all references to inst and deallocate
 *  the memory associated with inst.  It will delete inst's reference to its
 *  children.  If parent is not NULL, this will destroy parent's reference
 *  to inst.  If this is the only reference to inst, it will deallocate the
 *  memory associated with inst.<br><br>
 *
 *  DestroyInstance will modify the parent's of inst to remove the reference.
 */
/*
 *  void DestroyInstance(inst,parent)
 *  struct Instance *inst,*parent;
 */

/* @} */

#endif  /* ASC_DESTROYINST_H */

