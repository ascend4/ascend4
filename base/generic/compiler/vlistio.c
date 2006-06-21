/*
 *  Temporary variable list output routines
 *  by Tom Epperly
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: vlistio.c,v $
 *  Date last modified: $Date: 1997/12/02 12:00:21 $
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

#include <utilities/ascConfig.h>
#include <general/dstring.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "symtab.h"
#include "vlist.h"
#include "vlistio.h"
#include "nameio.h"

#ifndef lint
static CONST char VariableListIOID[] = "$Id: vlistio.c,v 1.8 1997/12/02 12:00:21 ballan Exp $";
#endif

void WriteVariableList(FILE *f, CONST struct VariableList *n)
{
  while(n!=NULL) {
    WriteVariableListNode(f,n);
    n = NextVariableNode(n);
    if (n!=NULL) {
      PUTC(',',f);
      PUTC(' ',f);
    }
  }
}


void WriteVariableListNode(FILE *f, CONST struct VariableList *n)
{
  if (n==NULL) return;
  WriteName(f,NamePointer(n));
}
