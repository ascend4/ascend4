/*
 *  Temporary name output routine
 *  by Tom Epperly
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: nameio.h,v $
 *  Date last modified: $Date: 1998/04/16 00:43:29 $
 *  Last modified by: $Author: ballan $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#ifndef __NAMEIO_H_SEEN__
#define __NAMEIO_H_SEEN__
/* requires
# #include<stdio.h>
# #include"compiler.h"
# #include"types.h"
# #include"symtab.h"
*/

extern void WriteName(FILE *,CONST struct Name *);
/*
 *  void WriteName(f,n)
 *  FILE *f;
 *  const struct Name *n;
 *  No leading or trailing spaces are added, and no trailing newline is
 *  added.
 */

extern void WriteNameNode(FILE *,CONST struct Name *);
/*
 *  void WriteNameNode(f,n)
 *  FILE *f;
 *  const struct Name *n;
 *  Write just this one name node, and not any of the ones following it.
 */

extern char *WriteNameString(CONST struct Name *);
/*
 * return a string containing the name. user is responsible
 * for freeing string.
 */

extern void WriteName2Str(Asc_DString *,CONST struct Name *);
/*
 *  void WriteName2Str(dstring,n)
 *  Asc_DString *dstring;
 *  const struct Name *n;
 *  No leading or trailing spaces are added, and no trailing newline is
 *  added.
 */

extern void WriteNameNode2Str(Asc_DString *,CONST struct Name *);
/*
 *  void WriteNameNode2Str(dstring,n)
 *  Asc_DString *dstring;
 *  const struct Name *n;
 *  Write just this one name node, and not any of the ones following it.
 */
#endif /* __NAMEIO_H_SEEN__ */
