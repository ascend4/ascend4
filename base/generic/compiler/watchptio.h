/*
 *  watchptio.h: An API to ascend methods
 *  by Benjamin Allan                          
 *  March 17, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: watchptio.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:53 $
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

/** @file
 *  An API to
 *  ascend methods via an interactive or external interface
 *  without knowing about ascend compiler internals.
 *  <pre>
 *  When #including watchptio.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "proc.h"
 *  </pre>
 */

#ifndef __WATCHPTIO_H_SEEN__
#define __WATCHPTIO_H_SEEN__

/**
 *  <!--  void WriteProcedure(f,p)                                     -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  struct InitProcedure *p;                                     -->
 *  Write the procedure to the file.
 */
extern void WriteProcedure(FILE *f, struct InitProcedure *p);

#endif  /* __WATCHPTIO_H_SEEN__ */

