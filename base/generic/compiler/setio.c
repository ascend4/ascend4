/*
 *  Set output routines
 *  by Tom Epperly
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: setio.c,v $
 *  Date last modified: $Date: 1997/12/02 12:00:23 $
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

#include <utilities/ascConfig.h>
#include <general/dstring.h>
#include "compiler.h"
#include "functype.h"
#include "fractions.h"
#include "dimen.h"
#include "types.h"
#include "sets.h"
#include "setio.h"
#include "exprio.h"

#ifndef lint
static CONST char SetIOID[] = "$Id: setio.c,v 1.9 1997/12/02 12:00:23 ballan Exp $";
#endif


void WriteSetNode(FILE *f, CONST struct Set *s)
{
  if (SetType(s)) {
    WriteExpr(f,GetLowerExpr(s));
    PUTC('.',f);
    PUTC('.',f);
    WriteExpr(f,GetUpperExpr(s));
  } else {
    WriteExpr(f,GetSingleExpr(s));
  }
}

void WriteSet(FILE *f, CONST struct Set *s)
{
  while(s!=NULL) {
    WriteSetNode(f,s);
    s = NextSet(s);
    if (s!=NULL) {
      PUTC(',',f);
      PUTC(' ',f);
    }
  }
}

/*
 * These functions are similar to those above, but write to
 * a dynamic string, rather than to a FILE *.
 */
void WriteSetNode2Str(Asc_DString *dstring, CONST struct Set *s)
{
  if (SetType(s)) {
    WriteExpr2Str(dstring,GetLowerExpr(s));
    Asc_DStringAppend(dstring,"..",-1);
    WriteExpr2Str(dstring,GetUpperExpr(s));
  }
  else
    WriteExpr2Str(dstring,GetSingleExpr(s));
}

void WriteSet2Str(Asc_DString *dstring, CONST struct Set *s)
{
  while(s!=NULL) {
    WriteSetNode2Str(dstring,s);
    s = NextSet(s);
    if (s!=NULL)
      Asc_DStringAppend(dstring,",",-1);
  }
}


