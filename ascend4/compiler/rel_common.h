/*
 *  Common Relation Construction Routines
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: rel_common.h,v $
 *  Date last modified: $Date: 1997/07/18 12:33:09 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

#ifndef __REL_COMMON_H_SEEN__
#define __REL_COMMON_H_SEEN__


extern int CmpP(CONST char *, CONST char *);
/*
 *  int CmpP(c1,c2)
 *  CONST char *c1;
 *  CONST char *c2;
 *  Compare two character pointers.
 */

extern void Swap(unsigned long int *, unsigned long int *);
/*
 *  void Swap(p1,p2)
 *  unsigned long int *p1
 *  unsigned long int *p2
 *  unsigned long temp
 *  Exchange the value of pointers p1 and p2
 */

extern CONST struct Expr *FindLastExpr(register CONST struct Expr *);
/*
 *  CONST struct Expr *FindLastExpr(ex)
 *  register CONST struct Expr *ex
 *  Return the next pointer in a link of expressions
 */


#endif /*__ REL_COMMON_H_SEEN__  */
