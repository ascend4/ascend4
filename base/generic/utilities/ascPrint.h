/**< 
 *  ASCEND Printf Substitutes
 *  by Mark Thomas
 *  Created: 27.May.1997
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: ascPrint.h,v $
 *  Date last modified: $Date: 1997/10/29 13:08:50 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND utilities.
 *
 *  Copyright 1997, Carnegie Mellon University
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


extern int Asc_PrintInit(void);
/**< 
 *  int Asc_PrintInit()
 *
 *  Initialize ASCEND's pointers to the Tcl channels used for output.
 *  This function should be called after the Tcl channels have been
 *  initialized.  Any calls to Asc_Printf() made prior to calling
 *  Asc_PrintInit() will use ordinary printf().
 */

extern int Asc_Printf(CONST char *, ...);
/**< 
 *  int Asc_Printf(format, variable_number_args)
 *      CONST char *format;            // sprintf-style format string
 *      va_list variable_number_args;  // arguments for the `format' string
 *
 *  Using the sprintf-style format string `format', print the
 *  `variable_number_args' to some approximation of stdout.  Return the
 *  number of bytes printed.
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
 *  Using the sprintf-style format string `format', print the
 *  `variable_number_args' to the file handle `fileptr'.  Return the
 *  number of bytes printed.
 *
 *  This is needed under Windows to redirect stdout and stderr to the
 *  TkConsole instead of into the bit bucket.
 */


extern int Asc_FFlush(FILE *);
/**< 
 *  int Asc_FFlush(fileptr)
 *      FILE *fileptr;
 *
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
 *  Print the character `c' to `stdout'; return 1 for success and
 *  EOF for failure.
 *
 *  This is needed under Windows to redirect stdout and stderr to the
 *  TkConsole instead of into the bit bucket.
 */
#endif /**< _ASCPRINT_H */
