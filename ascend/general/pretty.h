/*
 *  pretty.h
 *  I/O Formatting Functions
 *  by Ben Allan
 *  Created: 01/98
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: pretty.h,v $
 *  Date last modified: $Date: 2001/01/28 03:39:39 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Language Interpreter
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The ASCEND Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The ASCEND Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  I/O Formatting Functions.
 *  <pre>
 *  Requires:
 *        #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef __pretty_h_seen__
#define __pretty_h_seen__

/**
 *  Writes a string to a file, splitting it at whitespace characters to
 *  try to limit each line to the specified width.  The string is split 
 *  at the blank characters, tabs, or returns.  Long words are not broken, 
 *  which can result in lines longer than requested.  The printed string 
 *  will always have a trailing '\n',  possibly 2 if the original string 
 *  ends in a '\n'.  This function is not very bright, but works ok on 
 *  nonpathological input.<br><br>
 *
 *  Each line written is indented by indent spaces.  Note that width does 
 *  not include the  indent, so print_long_string(f,s,70,2) -> 72 char 
 *  lines.  If fp or string is NULL, nothing is printed.  If width < 4 
 *  or indent < 0, no formatting is attempted and the string is simply 
 *  printed as-is.<br><br>
 *
 *  Note that \n and \t count as a character when determining line length,
 *  so you won't get perfect formatting. If you want perfect formatting,
 *  fix it. How big is a tab? In particular, \n will cause a missing
 *  indent.  See print_long_string_EOL() for somewhat more controlled 
 *  formatting.<br><br>
 *
 *  Passing this function a constant string may result in a crash
 *  on platforms/compilers in which assigning characters through a
 *  pointer is disallowed for const strings.
 *
 *  @param fp     The file stream to receive the output.  If NULL,
 *                nothing is printed.
 *  @param string The string to format into lines and print to fp.
 *                If NULL, nothing is printed.
 *  @param width  The maximum requested line length.  If less than 4,
 *                no formatting is performed.
 *  @param indent The number of spaces by which to indent each line.
 *                If less than 0, no formatting is performed.
 *  @return The total number of characters printed.
 */
ASC_DLLSPEC int print_long_string(FILE *fp, char *string, int width, int indent);

/**
 *  Writes a string to a file, splitting it at special delimiter
 *  characters.  This function works like print_long_string(), except
 *  the string is broken at occurences of /{star}EOL{star}/ (commented EOL).
 *
 *  @param fp     The file stream to receive the output.  If NULL,
 *                nothing is printed.
 *  @param string The string to format into lines and print to fp.
 *                If NULL, nothing is printed.
 *  @param indent The number of spaces by which to indent each line.
 *                If less than 0, no formatting is performed.
 *  @return The total number of characters printed.
 */
ASC_DLLSPEC int print_long_string_EOL(FILE *fp, char *string, int indent);

#endif /* __pretty_h_seen__ */

