/**< 
 *  Temporary variable list output routines
 *  by Tom Epperly
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: vlistio.h,v $
 *  Date last modified: $Date: 1997/07/18 12:36:41 $
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

#ifndef __VLISTIO_H_SEEN__
#define __VLISTIO_H_SEEN__
/**< requires
# #include<stdio.h>
# #include"vlist.h"
*/

extern void WriteVariableList(FILE *,CONST struct VariableList *);
/**< 
 *  void WriteVariableList(f,n)
 *  FILE *f;
 *  struct VariableList *n;
 *  No leading or trailing white space is added
 */

extern void WriteVariableListNode(FILE *, CONST struct VariableList *);
/**< 
 *  void WriteVariableListNode(f,n);
 *  FILE *f;
 *  struct VariableList *n;
 *  Write just this one variable list node, and not any of the ones
 *  following it.
 */
#endif /**< __VLISTIO_H_SEEN__ */
