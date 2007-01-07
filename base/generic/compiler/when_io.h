/*
 *  When Output Routines
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: when_io.h,v $
 *  Date last modified: $Date: 1997/07/29 15:52:58 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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

/** @file
 *  When Output Routines.
 *  <pre>
 *  When #including when_io.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *  </pre>
 */

#ifndef __WHENINST_IO_H_SEEN__
#define __WHENINST_IO_H_SEEN__

/*===========================*/

extern void WriteWhen(FILE *f, CONST struct Instance *wheninst,
                      CONST struct Instance *ref);
/**<
 *  <!--  void WriteWhen(f,wheninst,ref);                              -->
 *  <!--  FILE *f;                                                     -->
 *  <!--  CONST struct Instance *wheninst;                             -->
 *  <!--  CONST struct Instance *ref;                                  -->
 *  Write a when instance to the file f
 */

ASC_DLLSPEC char *WriteWhenString(CONST struct Instance *wheninst,
                             CONST struct Instance *ref);
/**<
 *  <!--  char WriteWhenString(wheninst,ref);                          -->
 *  <!--  char *result;                                                -->
 *  <!--  CONST struct Instance *wheninst;                             -->
 *  <!--  CONST struct Instance *ref;                                  -->
 *  Write a when instance to a char
 */

#endif  /* __WHENINST_IO_H_SEEN__ */

