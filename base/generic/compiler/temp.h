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

/*
 *  When #including temp.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "value_type.h"
 */



#ifndef __TEMP_H_SEEN__
#define __TEMP_H_SEEN__
/* requires
# #include"compiler.h"
# #include"value_type.h"
*/

extern void AddTemp(symchar *);
/*
 *  void AddTemp(name)
 *  const char *name;
 *  Make a temporary variable called name.  Assume that one doesn't
 *  already exist.
 */

extern void SetTemp(symchar *,struct value_t);
/*
 *  void SetTemp(name,value)
 *  const char *name;
 *  struct value_t value;
 */

extern void RemoveTemp(symchar *);
/*
 *  void RemoveTemp(name)
 *  const char *name;
 *  Remove a temporary variable called name.
 */

extern int TempExists(symchar *);
/*
 *  int TempExists(name)
 *  const char *name;
 *  Return true is a temporary variable of the given name exists.
 */

extern struct value_t TempValue(symchar *);
/*
 *  struct value_t TempValue(name)
 *  const char *name;
 *  Return the value of a temporary variable.
 */

extern void DestroyTemporaryList();
/*
 *  void DestroyTemporaryList()
 *  Free the memory for the temporary variable list.
 */
#endif /* __TEMP_H_SEEN__ */
