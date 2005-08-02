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

extern int readln(char *s, int max);
/**<
 *  Reads a line from standard input, given a target string
 *  and capacity.  If line read in is too long, the remainder of the line
 *  is bypassed.  Number returned is the amount read in, excluding newline
 *  (which is removed from s), or -1 if EOF was encountered.
 */
extern int freadln(char *s, int max, FILE *input);
/**<
 *  <!--  length = readln(s,max)                                       -->
 *  <!--  length = freadln(s,max,input)                                -->
 *  <!--  int length;                                                  -->
 *  <!--  char *s;                                                     -->
 *  <!--  int max;                                                     -->
 *  <!--  FILE *input;                                                 -->
 *
 *  Reads a line from specified input, given a target string
 *  and capacity.  If line read in is too long, the remainder of the line
 *  is bypassed.  Number returned is the amount read in, excluding newline
 *  (which is removed from s), or -1 if EOF was encountered.
 */

extern char *areadln(void);
/**<
 *  Reads a line from standard input and returns a
 *  sufficiently large allocated string containing all that was read.
 *  NULL is returned if EOF is encountered or ascmalloc() fails to create
 *  string.  String should be destroyed when no longer in use.
 */
extern char *afreadln(FILE *fp);
/**<
 *  <!--  str = areadln()                                              -->
 *  <!--  str = afreadln(fp)                                           -->
 *
 *  Reads a line from specified input and returns a
 *  sufficiently large allocated string containing all that was read.
 *  NULL is returned if EOF is encountered or ascmalloc() fails to create
 *  string.  String should be destroyed when no longer in use.
 */

extern long readlong(long number_default);
/**<
 *  Reads in a line from standard input and extracts a long integer 
 *  from that line.  If none is found, then the default is returned.
 */
extern double readdouble(double value_default);
/**<
 *  <!--  number = readlong(number_default)                            -->
 *  <!--  value = readdouble(value_default)                            -->
 *  <!--  long number, number_default;                                 -->
 *  <!--  double value, value_default;                                 -->
 *
 *  Reads in a line from standard input and extracts a double from 
 *  that line.  If none is found, then the default is returned.
 */

#endif  /* _READLN_H */

