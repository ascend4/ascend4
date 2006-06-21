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

/** @file
 *  Common Relation Construction Routines.
 *  <pre>
 *  When #including rel_common.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "expr_types.h"
 *  </pre>
 */

#ifndef ASC_REL_COMMON_H
#define ASC_REL_COMMON_H

extern int CmpP(CONST char *c1, CONST char *c2);
/**< 
 *  Compare two character pointers.
 */

extern void Swap(unsigned long int *p1, unsigned long int *p2);
/**< 
 *  Exchange the value of pointers p1 and p2
 */

extern CONST struct Expr *FindLastExpr(register CONST struct Expr *ex);
/**< 
 *  Return the next pointer in a link of expressions
 */

#endif  /* ASC_REL_COMMON_H */

