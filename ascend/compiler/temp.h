/* 
 *  Temporary Variable Module
 *  by Tom Epperly
 *  Created: 1/17/90
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: temp.h,v $
 *  Date last modified: $Date: 1998/02/05 16:38:10 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/** @file
 *  Temporary Variable Module.
 *  <pre>
 *  When #including temp.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "expr_types.h"
 *         #include "value_type.h"
 *  </pre>
 */

#ifndef ASC_TEMP_H
#define ASC_TEMP_H

/**	@addtogroup compiler Compiler
	@{
*/

extern void AddTemp(symchar *name);
/**<
 *  <!--  void AddTemp(name)                                           -->
 *  <!--  const char *name;                                            -->
 *  Make a temporary variable called name.  Assume that one doesn't
 *  already exist.
 */

extern void SetTemp(symchar *name, struct value_t value);
/**< 
 *  <!--  void SetTemp(name,value)                                     -->
 *  <!--  const char *name;                                            -->
 *  <!--  struct value_t value;                                        -->
 *  Set the value of a temporary variable.
 */

extern void RemoveTemp(symchar *name);
/**< 
 *  <!--  void RemoveTemp(name)                                        -->
 *  <!--  const char *name;                                            -->
 *  Remove a temporary variable called name.
 */

extern int TempExists(symchar *name);
/**< 
 *  <!--  int TempExists(name)                                         -->
 *  <!--  const char *name;                                            -->
 *  Return true is a temporary variable of the given name exists.
 */

extern struct value_t TempValue(symchar *name);
/**< 
 *  <!--  struct value_t TempValue(name)                               -->
 *  <!--  const char *name;                                            -->
 *  Return the value of a temporary variable.
 */

extern void DestroyTemporaryList(void);
/**< 
 *  <!--  void DestroyTemporaryList()                                  -->
 *  Free the memory for the temporary variable list.
 */

/* @} */

#endif  /* ASC_TEMP_H */

