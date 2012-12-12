/*
 *  Logical Relation Output Routines
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: logrel_io.h,v $
 *  Date last modified: $Date: 1997/07/29 15:52:45 $
 *  Last modified by: $Author: rv2a $
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Logical Relation Output Routines.
 *  <pre>
 *  When #including logrel)io.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *         #include "exprs.h"
 *         #include "logrelation.h"
 *  </pre>
 */

#ifndef ASC_LOGRELATION_IO_H
#define ASC_LOGRELATION_IO_H

#include "logrelation.h"

/**	@addtogroup compiler_logrel Compiler Logical Relations
	@{
*/

extern int LogExprNeedParentheses(enum Expr_enum parent,
                                  enum Expr_enum child,
                                  int rhs);
/**<
 *  Given a unary or binary expression, will determine whether, the
 *  child expression needs parentheses. "rhs" tells if we are looking
 *  at the left or right side of a binary token.
 */

extern void WriteLogRel(FILE *f,
                        CONST struct Instance *lrelinst,
                        CONST struct Instance *ref);
/**<
 *  Write the logical relation in infix to the file indicated.
 */

ASC_DLLSPEC char *WriteLogRelToString(CONST struct Instance *lrelinst,
                                 CONST struct Instance *ref);
/**<
 *  Write the logical relation in infix to a char.
 */

extern void WriteLogRelPostfix(FILE *f,
                               CONST struct Instance *lrelinst,
                               CONST struct Instance *ref);
/**<
 *  Write the logical relation in postfix to the file indicated.
 */

ASC_DLLSPEC char *WriteLogRelPostfixToString(CONST struct Instance *lrelinst,
                                        CONST struct Instance *ref);
/**<
 *  Write the logical relation in postfix to a char
 */

extern void WriteLogRelInfix(FILE *f,
                             CONST struct Instance *lrelinst,
                             CONST struct Instance *ref);
/**<
 *  Write the logical relation in infix to the file indicated.
 */

extern void WriteLogRelationsInTree(FILE *f, struct Instance *lrelinst);
/**<
 *  Search for logical relations in an instance and write them to the
 *  file given in various formats.
 */

extern void SaveLogRelBoolVars(FILE *f, CONST struct logrelation *lr);
/**<
 *  Given a logical relation will save its variable list in the ASCEND
 *  condensed format.
 */


extern void SaveLogRel(FILE *f, CONST struct Instance *lrelinst);
/**<
 *  Given a logical relation will save it in the ASCEND condensed
 *  format.
 */

/* @} */

#endif /* ASC_LOGRELATION_IO_H */

