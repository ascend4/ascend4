/*
 *  Read line module
 *  by Karl Westerberg
 *  Created: 6/90
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: readln.h,v $
 *  Date last modified: $Date: 1997/07/18 12:04:27 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 *  COPYING.  COPYING is in ../compiler.
 */

/** @file
 *  Read line module.
 *  <pre>
 *  Contents:     Read line module
 *
 *  Authors:      Karl Westerberg
 *
 *  Dates:        06/90 - original version
 *
 *  Description:  When possible, use [a]readln instead of [a]freadln,
 *                since [a]readln will provide a better interface to the
 *                terminal.
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "ascMalloc.h"
 *  </pre>
 */

#ifndef _READLN_H
#define _READLN_H

extern int readln(char *str, int max);
/**<
 *  Reads a line from standard input to a target string having a
 *  specified capacity.  If the line read is too long, the remainder of 
 *  the line is bypassed.  The number of characters read is returned
 *  (excluding newline, which is removed from str), or -1 if EOF was
 *  encountered.  If str is NULL, -1 is returned.
 *
 *  @param str  The char buffer to receive the input.
 *  @param max  Capacity of str, or the max chars to read.
 *  @return Returns the number of chars read (not including newline),
 *          or -1 on error.
 */

extern int freadln(char *str, int max, FILE *input);
/**<
 *  Reads a line from a file stream to a target string having a
 *  specified capacity.  If the line read is too long, the remainder of
 *  the line is bypassed.  The number of characters read is returned
 *  (excluding newline, which is removed from str), or -1 if EOF was
 *  encountered.  If str or input is NULL, -1 is returned.
 *
 *  @param str    The char buffer to receive the input.
 *  @param max    Capacity of str, or the max chars to read.
 *  @param input  File stream to read from.
 *  @return Returns the number of chars read (not including newline),
 *          or -1 on error.
 */

extern char *areadln(void);
/**<
 *  Reads a line from standard input and returns a newly-allocated
 *  string containing all that was read.  NULL is returned if EOF 
 *  is encountered or memory cannot be allocated.  The returned
 *  string should be destroyed with ascfree() when no longer in use.
 *
 *  @return  A new string containing the input chars, of NULL on error.
 */

extern char *afreadln(FILE *fp);
/**<
 *  Reads a line from a file stream and returns a newly-allocated
 *  string containing all that was read.  NULL is returned if EOF
 *  is encountered or memory cannot be allocated.  The returned
 *  string should be destroyed ith ascfree() when no longer in use.
 *
 *  @param fp The file stream to read from.
 *  @return  A new string containing the input chars, of NULL on error.
 */

extern long readlong(long number_default);
/**<
 *  Reads in a line from standard input and extracts a long integer 
 *  from that line.  If none is found, then the default is returned.
 *
 *  @param number_default The default value to return if a long
 *                        cannot be read.
 *  @return The input long integer or number_default if none is found.
 */

extern double readdouble(double value_default);
/**<
 *  Reads in a line from standard input and extracts a double from
 *  that line.  If none is found, then the default is returned.
 *
 *  @param value_default The default value to return if a double
 *                       cannot be read.
 *  @return The input double or value_default if none is found.
 */

#endif  /* _READLN_H */

