/*
 *  Expression Input/Output
 *  by Tom Epperly
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: exprio.h,v $
 *  Date last modified: $Date: 1998/02/05 16:35:57 $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#ifndef __EXPRIO_H_SEEN__
#define __EXPRIO_H_SEEN__


/*
 *  When #including exprio.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "symtab.h"
 */


extern CONST char *ExprEnumName(CONST enum Expr_enum);
/*
 *  CONST char *ExprEnumName(t);
 *  CONST enum Expr_enum t;
 *  Returns a pointer to a string containing the name of the Expr term
 *  given. Do not free this string under any circumstances.
 *  This string is not in the symbol table.
 */

extern void WriteExprNode(FILE *,CONST struct Expr *);
/*
 *  void WriteExprNode(f,e)
 *  FILE *f;
 *  const struct Expr *e;
 */

extern void WriteExpr(FILE *,CONST struct Expr *);
/*
 *  void WriteExpr(f,e)
 *  FILE *f;
 *  const struct Expr *e;
 *  Write the expression with no leading or trailing white space.
 */

extern void WriteExprNode2Str(Asc_DString *,CONST struct Expr *);
/*
 *  void WriteExprNode2Str(dstring,e)
 *  Asc_DString *dstring;
 *  const struct Expr *e;
 */

extern void WriteExpr2Str(Asc_DString *,CONST struct Expr *);
/*
 *  void WriteExpr2Str(dstring,e)
 *  Asc_DString *dstring;
 *  const struct Expr *e;
 *  Write the expression with no leading or trailing white space.
 */
#endif /* __EXPRIO_H_SEEN__ */





