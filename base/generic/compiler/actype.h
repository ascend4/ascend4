/*
 *  Ascend Character Types
 *  Shortcut character recognition
 *  by Tom Epperly
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: actype.h,v $
 *  Date last modified: $Date: 1997/07/18 12:27:46 $
 *  Last modified by: $Author: mthomas $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/*
 *  When #including actype.h, make sure these files are #included first:
 *         #include "compiler.h"
 */

#ifndef __ACTYPE_H_SEEN__
#define __ACTYPE_H_SEEN__
/* requires
 *# #include"compiler.h"
 */

extern CONST unsigned char ascend_char_t[];

#define UNIT_CHAR 1
#define ID_CHAR 2
#define DIGIT_CHAR 4
#define SPACE_CHAR 8
#define ALPHA_CHAR 16

#define isunit(c) ((ascend_char_t[(unsigned char)c])&UNIT_CHAR)
/*
 *  macro isunit(c)
 *  char c;
 *  Produce a true value if c is a character than can appear inside unit
 *  delimeters.
 */

#define isidchar(c) ((ascend_char_t[(unsigned char)c])&ID_CHAR)
/*
 *  macro isidchar(c)
 *  char c;
 *  Produce a true value if c is a character that can appear in an
 *  identifier.
 */
#endif /* __ACTYPE_H_SEEN__ */
