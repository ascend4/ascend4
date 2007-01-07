/*
 *  Temporary variable list output routines
 *  by Tom Epperly
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: vlistio.h,v $
 *  Date last modified: $Date: 1997/07/18 12:36:41 $
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
 *  Temporary variable list output routines.
 *  <pre>
 *  When #including vlistio.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "vlist.h"
 *  </pre>
 */

#ifndef ASC_VLISTIO_H
#define ASC_VLISTIO_H

/**	addtogroup compiler Compiler
	@{
*/

extern void WriteVariableList(FILE *f, CONST struct VariableList *n);
/**<
 *  <!--  void WriteVariableList(f,n)                                  -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  struct VariableList *n;                                      -->
 *  No leading or trailing white space is added
 */

extern void WriteVariableListNode(FILE *f, CONST struct VariableList *n);
/**<
 *  <!--  void WriteVariableListNode(f,n);                             -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  struct VariableList *n;                                      -->
 *  Write just this one variable list node, and not any of the ones
 *  following it.
 */

/* @} */

#endif /* ASC_VLISTIO_H */

