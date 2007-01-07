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
 *         #include "compiler/fractions.h"
 *         #include "compiler/compiler.h"
 *         #include "compiler/dimen.h"
 *         #include "compiler/types.h"
 *         #include "compiler/exprs.h"
 *         #include "compiler/stattypes.h"
 *  </pre>
 */

#ifndef ASC_SYNTAX_H
#define ASC_SYNTAX_H

/**	addtogroup compiler Compiler
	@{
*/

extern unsigned NumberOfRelOps(struct Expr *ex);
/**< 
 *  Counts the number of relation operators in the given Expr.
 *
 *  @param ex The expresion to evaluate.
 *  @return The number of relation operators in ex.
 */

extern int IsRelation(struct Expr *ex);
/**<
 *  Tests whether an expression is a real or logical relation.
 *
 *  @param ex The expresion to test.
 *  @return Returns 1 if ex is a real relation, 0 if a
 *          logical relation.
 *  @todo compiler/syntax:IsRelation() notated as broken in source file.
 *        Fix or remove comment.
 */

extern int GetParseRelnsFlag(void);
/**<
 *  Retrieves the value of the parse relations flag. If this flag is 
 *  non-zero (the default), normal ASCEND relations will be parsed.  
 *  If it is 0, they will not be.  This flag does not affect 
 *  external relations.
 *
 *  @return The current value of the parse relations flag.
 */

ASC_DLLSPEC void SetParseRelnsFlag(int flag);
/**<
 *  Sets the parse relations flag. Any nonzero integer will
 *  cause relations to be parsed (see GetParseRelnsFlag()).
 *  Later it may be given more meaning.
 *
 *  @param flag The new value for the parse relations flag.
 */

/* @} */

#endif  /* ASC_SYNTAX_H */

