/*
 *  Syntax Routines
 *  by Tom Epperly
 *  Created: 3/22/1990
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: syntax.c,v $
 *  Date last modified: $Date: 1998/04/12 18:31:14 $
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
 *
 */
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/exprs.h"
#include "compiler/syntax.h"
#ifndef NULL
#define NULL 0
#endif

#ifndef lint
static CONST char SyntaxModuleRCSid[] = "$Id: syntax.c,v 1.11 1998/04/12 18:31:14 ballan Exp $";
#endif

unsigned NumberOfRelOps(struct Expr *ex)
{
  register unsigned count=0;
  while(ex!=NULL){
    switch(ExprType(ex)){
    case e_equal:
    case e_notequal:
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
    case e_boolean_eq:
    case e_boolean_neq:
      count++;
      break;
    default:
      break;
    }
    ex = NextExpr(ex);
  }
  return count;
}


/* broken. booleans? */
int IsRelation(struct Expr *ex)
{
  while(ex!=NULL){
    switch(ExprType(ex)){
    case e_equal:
    case e_notequal:
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
    case e_maximize:
    case e_minimize:
      return 1;
    default:
      break;
    }
    ex = NextExpr(ex);
  }
  return 0;
}



/*
 * This function controls  turning on/off the parse relations
 * flag. By default it is on. This variable is defined in
 * ascParse.y.
 */
int GetParseRelnsFlag(void)
{
  return g_parse_relns;
}

void SetParseRelnsFlag(int flag)
{
  g_parse_relns = flag;
}

