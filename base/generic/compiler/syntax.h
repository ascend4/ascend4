/* 
 *  Syntax Routines
 *  by Tom Epperly
 *  Created: 3/22/1990
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: syntax.h,v $
 *  Date last modified: $Date: 1998/04/12 18:31:16 $
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

/** @file
 *  Syntax Routines.
 *  These routines support the yacc parsing file ascParse.y.  This module
 *  provides functions to check syntax.
 *  <pre>
 *  When #including syntax.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "exprs.h"
 *         #include "stattypes.h"
 *  </pre>
 */

#ifndef __SYNTAX_H_SEEN__
#define __SYNTAX_H_SEEN__

extern unsigned NumberOfRelOps(struct Expr *ex);
/**< 
 *  <!--  unsigned NumberOfRelOps(ex)                                  -->
 *  <!--  struct Expr *ex;                                             -->
 *  Return the number of relation operators in the given Expr.
 */

extern int IsRelation(struct Expr *ex);
/**< 
 *  <!--  unsigned IsRelation(ex)                                      -->
 *  <!--  struct Expr *ex;                                             -->
 *  Return 1 in case of a real relation. 0 if logical relation.
 */

extern int GetParseRelnsFlag(void);
/**<
 *  <!--  int GetParseRelnsFlag(void);                                 -->
 *  Returns the value of the parse relations flag. If this flag is set to
 *  TRUE, which it is by default, normal ASCEND relations will be parsed;
 *  otherwise they will not be. This does not affect external relations.
 */

extern void SetParseRelnsFlag(int flag);
/**< 
 *  <!--  void SetParseRelnsFlag(flag);                                -->
 *  This function sets the parse relations flag. Any nonzero integer will
 *  turn the flag on. Later it may be given more meaning.
 */

#endif  /* __SYNTAX_H_SEEN__ */

