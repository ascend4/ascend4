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

#ifndef __pretty_h_seen__
#define __pretty_h_seen__

/*
 * count = print_long_string(fp,string,width,indent);
 * Writes a string to a file, splitting it at the blank characters
 * tabs, or returns to attempt keeping line length < width given.
 * each line written is indented by indent characters.
 * this function is not very bright, but works on nonpathological
 * input ok. long words are not broken.
 * width does not include the indent, so pls(f,s,70,2) -> 72 char lines.
 * If width < 4 or indent < 0, gives up and simply prints the string.
 *
 * Note that \n and \t count as a character when determining line length,
 * so you won't get perfect formatting. If you want perfect formatting,
 * fix it. How big is a tab? In particular, \n will cause a missing
 * indent.
 */
extern int print_long_string(FILE *, char *, int, int);

/* Like print_long_string, except instead of breaking at width, it
 * breaks at occurences of /{star}EOL{star}/ (commented EOL).
 * print_long_string(fp,string,indent);
 */
extern void print_long_string_EOL(FILE *, char *, int);

#endif /* __pretty_h_seen__ */
