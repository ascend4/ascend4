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
 */
 
/*  ChangeLog
 *
 *      10/13/2005  Added PanicCallbackFunc and Asc_PanicSetCallback() to
 *                  support callback functionality & ability to cancel exit
 *                  for use in unit test.  Added asc_assert() for handling
 *                  of assertions through Asc_Panic() & potential decoupling 
 *                  of assertions from NDEBUG (J.D. St.Clair)
 */

/** @file
 *  Ascend Panic - fatal error handling.
 *  <pre>
 *  To include this header file, you must include the following:
 *      #include <stdarg.h>
 *      #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef _ascpanic_h_seen
#define _ascpanic_h_seen

#ifdef ASC_NO_ASSERTIONS

#define asc_assert(x) ((void)0)
/**<  If assertions turned off, asc_assert() does nothing. */

#else /* assertions enabled */

#define asc_assert(cond) \
  ((cond) ? (void)0 : Asc_Panic(ASCERR_ASSERTION_FAILED, NULL, \
                                "Assertion failed in %s:%d:  '%s'", \
                                __FILE__, __LINE__, #cond))
/**< Our assert macro.  Uses Asc_Panic() to report & handle assertion failure. */

#endif	/* ASC_NO_ASSERTIONS */

extern void Asc_Panic(CONST int status, CONST char *function,
                      CONST char *format, ...);
/**<
 *  Prints a fatal error message, calls any registered callback
 *  function, and (usually) exits the program.
 *  This is the fatal error handler for the ASCEND system.  It prints
 *  the printf-style variable arguments using the specified format to
 *  ASCERR file handle.  The message is formatted with a header
 *  (e.g. 'ASCEND PANIC!!) and the name of the function (if non-NULL),
 *  followed by the variables & format passed as arguments.  ASCERR
 *  should have been initialized to a valid file stream or else the
 *  message will not be printed (checked by assertion).<br><br>
 *
 *  If a valid file name has been previously set using
 *  Asc_PanicSetOutfile(), the message is printed to this file also.
 *  Under Windows, a MessageBox will also be displayed with the
 *  message.<br><br>
 *
 *  If a callback has been set using Asc_PanicSetCallback(), the
 *  registered function will be called with the specified status.
 *  If the callback returns non-NULL, then exit() is called to end
 *  the program.  This is the default behavior.  If the callback
 *  is able to resolve the problem, then it should return zero and
 *  Asc_Panic() will just return.  This will be useful mostly for
 *  testing purposes, and should be used with caution.
 *
 *  @param status   Status code passed by the calling function.
 *  @param function Pointer to the name of the calling function.
 *  @param format   printf-style format string for VAR_ARGS to follow.
 */

extern void Asc_PanicSetOutfile(CONST char *filename);
/**<
 *  Sets a file name for reporting of Asc_Panic() messages.
 *  Calling this function with a non-NULL "filename" will cause
 *  Asc_Panic() to write panic messages to "filename" in addition to the
 *  ASCERR file handle.  Passing in a "filename" of NULL causes panic
 *  messages not to be written to disk---this undoes the effect of
 *  previous calls to Asc_PanicSetOutfile().
 *
 *  @param filename Pointer to the name of the file to print messages to.
 */

typedef int (*PanicCallbackFunc)(int);
/**<
 *  Signature of the callback function called by Asc_Panic().
 *  The function takes a single argument, the status code passed
 *  to Asc_Panic() by the original caller.  The function should
 *  return non-zero if Asc_Panic() should exit() the program, the
 *  default behavior.  If the function is able to resolve the problem
 *  somehow, returning 0 will instruct Asc_Panic() to just return.<br><br>
 *
 *  This functionality is provided primarily for internal testing
 *  purposes.  It should be used with extreme caution in release
 *  code.  Asc_Panic() is called from all over ASCEND for many
 *  error conditions, and current calls assume no return.
 */

extern PanicCallbackFunc Asc_PanicSetCallback(PanicCallbackFunc func);
/**<
 *  Registers a callback function to be called by Asc_Panic().
 *  This allows the user to specify a cleanup function to be
 *  called during a fatal error.  See PanicCallbackFunc for
 *  more information on the callback function.
 *
 *  @param func Pointer to the new function to call during an Asc_Panic().
 *  @return A pointer to the previously-registered callback, or NULL
 *          if none was registered.
 */

extern void Asc_PanicDisplayMessageBox(int TRUE_or_FALSE);
/**<
 *  Controls whether a MessageBox is displayed by Asc_Panic() on Windows.
 *  If TRUE_or_FALSE is non-zero then the MessageBox is displayed (the
 *  default).  Pass FALSE to disable display of the MessageBox.
 *  On other platforms, has no effect.
 */

#endif  /* _ascpanic_h_seen */

