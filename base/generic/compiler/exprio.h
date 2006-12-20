/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Expression Input/Output
	
	Requires:
	#include "utilities/ascConfig.h"
	#include "fractions.h"
	#include "compiler.h"
	#include "dimen.h"
	#include "expr_types.h"
	#include "symtab.h"
*//*
	by Tom Epperly
	Last in CVS: $Revision: 1.6 $ $Date: 1998/02/05 16:35:57 $ $Author: ballan $
*/

#ifndef ASC_EXPRIO_H
#define ASC_EXPRIO_H

extern CONST char *ExprEnumName(CONST enum Expr_enum t);
/**< 
	Returns a pointer to a string containing the name of the Expr term
	given. Do not free this string under any circumstances.
	This string is not in the symbol table.
*/

extern void WriteExprNode(FILE *f, CONST struct Expr *e);
/**<
	Write a single expression node with no leading or trailing white space.
 */

extern void WriteExpr(FILE *f, CONST struct Expr *e);
/**<
	Write the expression with no leading or trailing white space.
	@NOTE The output is in POSTFIX format
*/

extern void WriteExprNode2Str(Asc_DString *dstring, CONST struct Expr *e);
/**<	
	Write a single expression node to a string with no leading
	or trailing white space.
*/

extern void WriteExpr2Str(Asc_DString *dstring, CONST struct Expr *e);
/**<
	Write the expression to a string with no leading or trailing white space.
	@param dstring string into output is returned
	@NOTE The return is in POSTFIX format.
*/

#endif /* ASC_EXPRIO_H */

