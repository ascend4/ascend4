/*
 *  Common Relation construction routines
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: rel_common.c,v $
 *  Date last modified: $Date: 1997/07/18 12:33:07 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997  Carnegie Mellon University
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

#include<math.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "exprs.h"
#include "rel_common.h"


int CmpP(CONST char *c1, CONST char *c2)
{
  if (c1 > c2) return 1;
  if (c1 < c2) return -1;
  return 0;
}


void Swap(unsigned long int *p1, unsigned long int *p2)
{
  unsigned long temp;
  temp = *p1;
  *p1 = *p2;
  *p2 = temp;
}


CONST struct Expr *FindLastExpr(register CONST struct Expr *ex)
{
  assert(ex!=NULL);
  while(NextExpr(ex)!=NULL) ex = NextExpr(ex);
  return ex;
}
