/**< 
 *  ASCEND Printf stdout/stderr Substitutes Dispatcher
 *  by Benjamin Allan
 *  Created: 4.March.2005
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: ascPrint.h,v $
 *  Date last modified: $Date: 1997/10/29 13:08:50 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND utilities.
 *
 *  Copyright 2005, Benjamin Andrew Allan
 *
 *  The ASCEND utilities is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND utilities is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#ifndef _ASCPRINT_H
#define _ASCPRINT_H

/**< 
 *  When including this file, you must include:
 *      #include <stdarg.h>
 *      #include "compiler/compiler.h"
 *      #include "utilities/ascprint.h"
 */


/**
 * Output functions interceptor vtable. This should be constructed
 * and the functions fully operational before it is
 * pushed on the stack of output tables.
 */
struct Asc_PrintVTable {
	CONST char *name;
	int (*print)(FILE *fp, CONST char *format, va_list args);
	int (*fflush)(FILE *);
	struct Asc_PrintVTable *next;
};


extern int Asc_PrintPushVTable(struct Asc_PrintVTable *vtable);
/**< 
 *  int Asc_PrintPushVTable(v)
 *
 *  Add a vtable to the ascend output interceptor list.
 *  This function should be called after the user interface io channels
 *  has been initialized.  Any calls to Asc_Printf() made prior to calling
 *  Asc_PrintAddVTable() will use ordinary printf().
 *  More than one vtable can be pushed, in which case output will go
 *  to all the outputs setup in the reverse or in which tables were pushed.
 *  The vtable will be rejected if it or any field in it is null,
 *  other than next which must be null.
 *  If the vtable's name conflicts, return 1 otherwise return 0.
 *
 *  He who pushes a vtable should eventually remove it and destroy it.
 */

extern struct Asc_PrintVTable * Asc_PrintRemoveVTable(CONST char *name);
/**< 
 * oldVt = Asc_PrintRemoveVTable(name);
 *
 * Take a vtable out of the printing list and return it.
 * If not found in list, return is NULL.
 */

extern int Asc_Printf(CONST char *, ...);
/**< 
 *  int Asc_Printf(format, variable_number_args)
 *      CONST char *format;            // sprintf-style format string
 *      va_list variable_number_args;  // arguments for the `format' string
 *
 * Loops over all defined print vtables.
 *  Using the sprintf-style format string `format', print the
 *  `variable_number_args' to some approximation of stdout.  Return the
 *  number of bytes printed from the final vtable that is called.
 *
 *  This is needed under Windows to redirect stdout to the TkConsole
 *  instead of into the bit bucket.
 */

extern int Asc_FPrintf(FILE *, CONST char *, ...);
/**< 
 *  int Asc_FPrintf(fileptr, format, variable_number_args)
 *      FILE *fileptr;                 // file handle to send output to
 *      CONST char *format;            // sprintf-style format string
 *      va_list variable_number_args;  // arguments for the `format' string
 *
 * Loops over all defined print vtables.
 *  Using the sprintf-style format string `format', print the
 *  `variable_number_args' to the file handle `fileptr'.  Return the
 *  number of bytes printed from the final vtable that is called.
 *
 *  This is needed under Windows to redirect stdout and stderr to the
 *  TkConsole instead of into the bit bucket.
 */


extern int Asc_FFlush(FILE *);
/**< 
 *  int Asc_FFlush(fileptr)
 *      FILE *fileptr;
 *
 * Loops over all defined print vtables.
 *  Flush output to the file pointed to by the file pointer `fileptr';
 *  return 0 for success and EOF for failure.
 *
 *  This is needed for consistency with Asc_FPrintf() and Asc_Printf().
 */


extern int Asc_FPutc(int, FILE*);
/**< 
 *  int Asc_FPutc( c, fileptr );
 *      int c;
 *      FILE *fileptr;
 *
 * Loops over all defined print vtables.
 *  Print the character `c' to the output file pointed to by the
 *  file pointer `fileptr'; return 1 for success and EOF for failure.
 *
 *  This is needed under Windows to redirect stdout and stderr to the
 *  TkConsole instead of into the bit bucket.
 */


extern int Asc_Putchar(int);
/**< 
 *  int Asc_Putchar( c );
 *      int c;
 *
 * Loops over all defined print vtables.
 *  Print the character `c' to `stdout'; return 1 for success and
 *  EOF for failure.
 *
 *  This is needed under Windows to redirect stdout and stderr to the
 *  TkConsole instead of into the bit bucket.
 */
#endif /**< _ASCPRINT_H */
