/*
 *  Set Instance Output Routine
 *  by Tom Epperly
 *  Created: 2/15/90
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: setinst_io.c,v $
 *  Date last modified: $Date: 1997/10/28 19:20:43 $
 *  Last modified by: $Author: mthomas $
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
#include<stdio.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "compiler/setinstval.h"
#include "compiler/setinst_io.h"

#ifndef lint
static CONST char SetInstIORCSid[]="$Id: setinst_io.c,v 1.7 1997/10/28 19:20:43 mthomas Exp $";
#endif

void WriteInstSet(FILE *f, CONST struct set_t *s)
{
  unsigned long c,len;
  switch(SetKind(s)){
  case empty_set: FPRINTF(f,"[]"); break;
  case integer_set:
    PUTC('[',f);
    len = Cardinality(s);
    for(c=1;c<=len;c++)
      FPRINTF(f,(c<len) ? "%ld,": "%ld",FetchIntMember(s,c));
    FPRINTF(f,"]");
    break;
  case string_set:
    PUTC('[',f);
    len = Cardinality(s);
    for(c=1;c<=len;c++) {
      FPRINTF(f, (c<len) ? "'%s'," : "'%s'" , SCP( FetchStrMember(s,c) ) );
    }
    FPRINTF(f,"]");
    break;
  }
}
