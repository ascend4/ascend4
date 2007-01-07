/*
 *  Temporary set output routines
 *  by Tom Epperly
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: setio.h,v $
 *  Date last modified: $Date: 1997/07/18 12:34:48 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/** @file
 *  Temporary set output routines.
 *  <pre>
 *  When #including set_io.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "expr_types.h"
 *         #include "symtab.h"
 *  </pre>
 */

#ifndef ASC_SETIO_H
#define ASC_SETIO_H

/**	addtogroup compiler Compiler
	@{
*/

extern void WriteSetNode(FILE *f, CONST struct Set *s);
/**< 
 *  <!--  void WriteSetNode(f,s)                                       -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct Set *s;                                         -->
 *  Write this set node without any leading or trailing white space.
 */

extern void WriteSet(FILE *f, CONST struct Set *s);
/**< 
 *  <!--  void WriteSet(f,s)                                           -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  const struct Set *s;                                         -->
 *  Output the set with no leading or trailing white space.
 */

extern void WriteSetNode2Str(Asc_DString *dstring, CONST struct Set *s);
/**<
 *  <!--  void WriteSetNode2Str(dstring,s)                             -->
 *  <!--  Asc_DString *dstring;                                        -->
 *  <!--  const struct Set *s;                                         -->
 *  Write this set node without any leading or trailing white space.
 */

extern void WriteSet2Str(Asc_DString *dstring, CONST struct Set *s);
/**< 
 *  <!--  void WriteSet2Str(dsring,s)                                  -->
 *  <!--  Asc_DString *dstring;                                        -->
 *  <!--  const struct Set *s;                                         -->
 *  Output the set with no leading or trailing white space.
 */

/* @} */

#endif  /* ASC_SETIO_H */

