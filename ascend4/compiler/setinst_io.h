/*
 *  Set Instance Output Routine
 *  by Tom Epperly
 *  Created: 2/15/90
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: setinst_io.h,v $
 *  Date last modified: $Date: 1997/07/18 12:34:41 $
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

/*
 *  When #including setinst_io.h, make sure these files are #included first:
 *         #include "compiler.h"
 *         #include "setinstval.h"
 */


#ifndef __SETINST_IO_H_SEEN__
#define __SETINST_IO_H_SEEN__
/* requires
# #include<stdio.h>
# #include"compiler.h"
# #include"setinstval.h"
*/

extern void WriteInstSet(FILE *,CONST struct set_t *);
/*
 *  void WriteInstSet(f,s)
 *  FILE *f;
 *  const struct set_t *s;
 */
#endif /* __SETINST_IO_H_SEEN__ */
