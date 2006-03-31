/*
 *  Temporary Name output routine
 *  by Tom Epperly
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: nameio.c,v $
 *  Date last modified: $Date: 1998/04/16 00:43:27 $
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
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "func.h"
#include "types.h"
#include "name.h"
#include "nameio.h"
#include "sets.h"
#include "setio.h"

#ifndef lint
static CONST char NameIOId[] = "$Id: nameio.c,v 1.10 1998/04/16 00:43:27 ballan Exp $";
#endif


void WriteNameNode(FILE *f, CONST struct Name *n)
{
  if (n==NULL) return;
  if (NameId(n)) {
    FPRINTF(f,"%s",SCP(NameIdPtr(n)));
  } else {
    PUTC('[',f);
    WriteSet(f,NameSetPtr(n));
    PUTC(']',f);
  }
}

void WriteName(FILE *f, CONST struct Name *n)
{
  while (n!=NULL) {
    WriteNameNode(f,n);
    n = NextName(n);
    if ((n!=NULL)&&(NameId(n))) PUTC('.',f);
  }
}

/*
 * These functions here perform just like their counterparts
 * above, execept that they write to a preinitialized dynamic
 * string. Correspondingly they use the '2Str'
 */
void WriteNameNode2Str(Asc_DString *dstring, CONST struct Name *n)
{
  if (n==NULL) return;
  if (NameId(n)) {
    Asc_DStringAppend(dstring,SCP(NameIdPtr(n)),-1);
  } else {
    Asc_DStringAppend(dstring,"[",1);
    WriteSet2Str(dstring,NameSetPtr(n));
    Asc_DStringAppend(dstring,"]",1);
  }
}

char *WriteNameString(CONST struct Name *n)
{
  char *result;
  Asc_DString ds, *dsPtr;
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  WriteName2Str(dsPtr,n);
  result = Asc_DStringResult(dsPtr);
  Asc_DStringFree(dsPtr);
  return result;
}

void WriteName2Str(Asc_DString *dstring, CONST struct Name *n)
{
  while (n!=NULL) {
    WriteNameNode2Str(dstring,n);
    n = NextName(n);
    if ((n!=NULL)&&(NameId(n)))
      Asc_DStringAppend(dstring,".",-1);
  }
}


