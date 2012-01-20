#ifndef ASC_ASCPRINT_H
#define ASC_ASCPRINT_H
/*	ASCEND modelling environment
 	Copyright 2005, Benjamin Andrew Allan

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file Printf stdout/stderr Substitutes Dispatcher
 	ASCEND Printf stdout/stderr substitutes dispatcher.
 	These routines redirect output to stdout and stdin to other
 	functions defined by the user.  This is not required when you
 	desire to send output to an available console, which is normally
 	the case on Linux/unix.  However, in cases where a console is
 	not available (e.g. Win32 gui) or you prefer to redirect output
 	for other reasons, these routines make it possible.
*//*
 	by Benjamin Allan
 	Created: 4.March.2005
	Last in CVS: $Revision: 1.6 $ $Date: 1997/10/29 13:08:50 $ $Author: mthomas $
*/

#include <stdio.h>
#include <stdarg.h>

#include <ascend/general/platform.h>
#include <ascend/general/color.h>

/**	@addtogroup utilities_print Utilities Output Stream Handling
	@{
*/

/**
 * Output functions interceptor vtable. This should be constructed
 * and the functions fully operational before it is
 * pushed on the stack of output tables.
 *
 * This should be constructed and the functions fully operational
 * before it is pushed on the stack of output tables.
 */
struct Asc_PrintVTable {
  CONST char *name;                                         /**< Vtable name. */
  int (*print)(FILE *fp, CONST char *format, va_list args); /**< Print function. */
  int (*fflush)(FILE *);                                    /**< Flush function. */
  struct Asc_PrintVTable *next;                             /**< Next vtable in linked list. */
};


ASC_DLLSPEC int Asc_PrintPushVTable(struct Asc_PrintVTable *vtable);
/**<
 *  Adds a vtable to the ASCEND output interceptor list.
 *  Each registered vtable defines print and flush functions that are
 *  called by the internal ASCEND print functions to do the actual output.
 *  This function should be called after the user interface io channels
 *  have been initialized.  Any calls to Asc_Printf() made prior to calling
 *  Asc_PrintAddVTable() will use ordinary printf().
 *
 *  More than one vtable can be pushed, in which case output will go to all
 *  the outputs setup in the reverse order in which tables were pushed.
 *  The vtable will be rejected if it or any field in it is null,
 *  other than next which must be null.  It is advisable to use distinct
 *  names for all registered vtables so they may be uniquely identified in
 *  calls to Asc_PrintRemoveVTable().  This in not, however, enforced.
 *
 *  He who pushes a vtable should eventually remove it and destroy it.
 *
 *  @param vtable The new vtable to add to the output interceptor list.
 *  @return Returns 1 if an error occurs, 0 otherwise.
 */

ASC_DLLSPEC struct Asc_PrintVTable *Asc_PrintRemoveVTable(CONST char *name);
/**<
 *  Removes a vtable from the printing list and returns it.
 *  The first vtable found with the specified name is the one removed.
 *  If name is NULL or not found in the list, NULL is returned.
 *
 *  @param name The name of the vtable to remove from the printing list.
 *  @return Returns a pointer to the removed vtable if found, NULL otherwise.
 */

ASC_DLLSPEC int Asc_PrintHasVTable(CONST char *name);
/**<
 *  Queries whether a vtable is registered.
 *
 *  @param name Pointer to a string containing the name of the vtable to find.
 *  @return Returns TRUE if a vtable with the specified name is registered,
 *          FALSE otherwise.
 */

ASC_DLLSPEC int Asc_Printf(CONST char *format, ...);
/**<
 *  Prints the specified variables to stdout using all registered print
 *  vtables.  Using the sprintf-style format string `format', this
 *  function prints the `variable_number_args' to stdout using the print
 *  functions defined in the registered vtables.  The same output is
 *  printed using all registered vtables.  If no vtables are registered,
 *  nothing will be output.  Returns the number of bytes printed from
 *  the final vtable that is called.<br><br>
 *
 *  This is needed under Windows to redirect stdout to the TkConsole
 *  instead of into the bit bucket.
 *
 *  @param format The sprintf-style format string to use to format the output.
 *  @param ...    Variable number of arguments to print using the specified format.
 *  @return Returns the number of bytes printed from the final vtable called.
 */

ASC_DLLSPEC int Asc_FPrintf(FILE *fileptr, CONST char *format, ...);
/**<
 *  Prints the specified variables to fileptr using all registered print
 *  vtables.  Using the sprintf-style format string `format', this
 *  function prints the `variable_number_args' to the file handle
 *  'fileptr' using the print functions defined in the registered vtables.
 *  The same output is  printed using all registered vtables.  If no vtables
 *  are registered, nothing will be output.  Returns the number of bytes
 *  printed from the final vtable that is called.<br><br>
 *
 *  This is needed under Windows to redirect stdout to the TkConsole
 *  instead of into the bit bucket.
 *
 *  @param fileptr The file handle to receive the output.
 *  @param format  The sprintf-style format string to use to format the output.
 *  @param ...     Variable number of arguments to print using the specified format.
 *  @return Returns the number of bytes printed from the final vtable called.
 */

extern int Asc_VFPrintf(FILE *fileptr, CONST char *format, va_list args);
/**<
	Var-arg output function, required for calls passed through by error.h

	Follows calling convention of vfprintf from <stdarg.h>, see K&R2, p 245.
*/

ASC_DLLSPEC int Asc_FFlush(FILE *fileptr);
/**<
 *  Flushes output to fileptr.
 *  This function loops over all registered print vtables, flushing
 *  output to the file pointed to by `fileptr'.  If no vtables are
 *  registered, no streams will be flushed.  Returns 0 for success
 *  and EOF for failure.<br><br>
 *
 *  This is needed for consistency with Asc_FPrintf() and Asc_Printf().
 *
 *  @param fileptr The file handle to flush output to.
 *  @return Returns 0 for success, EOF otherwise.
 */

ASC_DLLSPEC int Asc_FPutc(int c, FILE *fileptr);
/**<
 *  Prints c to the file stream pointed to by fileptr.
 *  If fileptr is 'stdout' or 'stderr', this function loops over
 *  all registered print vtables.  For other file streams, the standard
 *  fputc() function is used.  Returns non-zero for success and EOF for
 *  failure.<br><br>
 *
 *  This is needed under Windows to redirect stdout and stderr to the
 *  TkConsole instead of into the bit bucket.
 *
 *  @param c       The character to print.
 *  @param fileptr The file handle to print c to.
 *  @return Returns non-zero for success, EOF otherwise.
 */

ASC_DLLSPEC int Asc_Putchar(int c);
/**<
 *  Prints c to stdout.
 *  This function loops over all registered print vtables.
 *  Returns 1 for success and EOF for failure.<br><br>
 *
 *  This is needed under Windows to redirect stdout and stderr to the
 *  TkConsole instead of into the bit bucket.
 *
 *  @param c       The character to print.
 *  @return Returns 1 for success, EOF otherwise.
 */

/* @} */

#endif /* _ASCPRINT_H */

