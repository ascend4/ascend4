/*
 *  Ascend Panic
 *  by Mark Thomas
 *  Created: 1997.05.15
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: ascPanic.h,v $
 *  Date last modified: $Date: 1997/07/18 11:43:23 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997  Carnegie Mellon University
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
 *
 */

#ifndef _ASCPANIC_H
#define _ASCPANIC_H

/*
 *  To include this header file, you must include the following:
 *      #include <stdarg.h>
 *      #include "compiler/compiler.h"
 *      #include "compiler/ascpanic.h"
 */


/*
 * Asc_Panic( status, function, format, args )
 *      int status;
 *      CONST char *function
 *      CONST char *format
 *      VAR_ARGS args
 *
 *  This function prints the arguments "args" using the format string
 *  "format" to the ASCERR file handle.  The first line of the panic
 *  message will print ``ASCEND PANIC!! in function'' if the argument
 *  "function" is not NULL.  If an panic output file location has been
 *  specified with the Asc_PanicSetOutfile() function, the panic message
 *  is also stored there.  Under Windows, we also pop up a MessageBox
 *  containing the message.  Finally, we exit the program with the status
 *  "status".
 *
 *  Side Effects: Exits the program.
 */
extern void Asc_Panic(CONST int status, CONST char *function,
                      CONST char *format, ...);


/*
 *  Asc_PanicSetOutfile(filename)
 *      CONST char *filename;
 *
 *  Calling this function with a non-NULL "filename" will cause
 *  Asc_Panic() to write panic messages to "filename" in addition to the
 *  ASCERR file handle.  Passing in a "filename" of NULL causes panic
 *  messages not to be written to disk---this undoes the effect of
 *  previous calls to Asc_PanicSetOutfile()
 */
extern void Asc_PanicSetOutfile(CONST char *filename);

#endif  /* _ASCPANIC_H */
