/*
 *  listio.c
 *  List I/O Routines
 *  by Ben Allan
 *  Created: 12/97
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: listio.c,v $
 *  Date last modified: $Date: 1998/06/16 15:47:41 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND Language Interpreter
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The ASCEND Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The ASCEND Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.  COPYING is found in ../compiler.
 */

#include <stdio.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "general/list.h"
#include "general/listio.h"

void gl_write_list(FILE *fp, struct gl_list_t *l)
{
  unsigned long c,len;
  FILE *myfp;

  asc_assert(NULL != l);

  if (fp==NULL) {
    myfp = stderr;
  } else {
    myfp = fp;
  }
  len = gl_length(l);
  for (c= 1; c <= len ; c++) {
    fprintf(myfp,"%lu: 0x%p (%lu)\n",c,gl_fetch(l,c),
             (unsigned long)gl_fetch(l,c));
  }
}
