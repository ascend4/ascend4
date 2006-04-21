/*
 *  watchptio.c: An API to ascend methods
 *  by Benjamin Allan
 *  March 17, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: watchptio.c,v $
 *  Date last modified: $Date: 1998/06/16 16:38:52 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

#include <utilities/ascConfig.h>
#include <general/list.h>
#include "compiler.h"
#include "stattypes.h"
#include "slist.h"
#include "statio.h"
#include "proc.h"
#include "watchptio.h"

/* move this function to watchpt.h, add WriteProcedureString */
void WriteProcedure(FILE *f, struct InitProcedure *p)
{
  FPRINTF(f,"METHOD %s;\n",SCP(ProcName(p)));
  WriteStatementList(f,ProcStatementList(p),4);
  FPRINTF(f,"END %s;\n",SCP(ProcName(p)));
}

