/*
 *  Driver.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: Driver.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:06 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  Main Tcl/Tk Driver.
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASCTK_DRIVER_H
#define ASCTK_DRIVER_H

extern int g_compiler_timing;
/**<
 *  <!--  g_compiler_timing                                            -->
 *
 *  TRUE if compiler timing is to be printed.
 *  default is false, set to TRUE by passing -t on the command line
 */

extern Tcl_Interp *g_interp;
/**<
 *  <!--  g_interp                                                     -->
 *
 *  Interpreter for this application.  We need tp make it global
 *  so that our signal/floating-porint traps can access it.
 */

extern int Asc_LoadWin(ClientData, Tcl_Interp *, int, CONST84 char**);
/**<
 *  <!--  int Asc_LoadWin(cdata, interp, argc, argv)                   -->
 *  <!--      ClientData cdata;                                        -->
 *  <!--      Tcl_Interp *interp;                                      -->
 *  <!--      int argc;                                                -->
 *  <!--      char **argv;                                             -->
 *
 *  Tcl callback to determine if Tcl/Tk should build Tk windows.
 *  Returns 1 if it should, 0 if we only want a command line.<br><br>
 *
 *  Currently (06/97), this call is pointless since we do not
 *  support command-line-only operation.
 */


#ifdef DEBUG_MALLOC
extern int Asc_DebugMallocCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/**<
 *  <!--  int Asc_DebugMallocCmd(cdata, interp, argc, argv)            -->
 *  <!--      ClientData cdata;                                        -->
 *  <!--      Tcl_Interp *interp;                                      -->
 *  <!--      int argc;                                                -->
 *  <!--      char **argv;                                             -->
 *
 *  Tcl callback for Tom Epperly's debugging malloc.
 */
#endif  /* DEBUG_MALLOC */

#endif  /* ASCTK_DRIVER_H  */

