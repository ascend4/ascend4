/*
 *  BrowLogRel_io.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: BrowLogRel_io.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:04 $
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
 *  Browser Logical Relation Output Routines
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *      #include "interface/BrowLogRel_io.h"
 *  </pre>
 */

#ifndef logrel_io_module
#define logrel_io_module

extern int Asc_BrowWriteLogRelListCmd(ClientData cdata, Tcl_Interp *interp,
                                      int argc, CONST84 char *argv[]);
/**<
 *  Registered as: bgetlogrels  ?cur?search? save
 *  Write a list of logical relations in INFIX representation
 */

extern int Asc_BrowWriteCondLogRelListCmd(ClientData cdata, Tcl_Interp *interp,
                                          int argc, CONST84 char *argv[]);
/**<
 *  Registered as: bgetcondlogrels
 *  Write the list of conditional logical relations in infix to the
 *  interpreter.
 */

extern int Asc_BrowWriteLogRelListPostfixCmd(ClientData cdata,
                                             Tcl_Interp *interp,
                                             int argc,
                                             CONST84 char *argv[]);
/**<
 *  Registered as: bgetlogrelspf ?cur?search? save
 *  Writes a list of logical relations in POSTFIX to the interpreter
 *  rather than to a file.
 */

extern int Asc_BrowWriteLogRelsForAtomCmd(ClientData cdata, Tcl_Interp *interp,
                                          int argc, CONST84 char *argv[]);
/**<
 *  Registered as: __brow_logrelsforatom ?cur?search.
 *
 *  Will return a proper Tcl list of all logical relations associated
 *  with the given atom. Works on the current or the search instance.
 *  Will return TCL_ERROR if the instance is NULL, or the atom is not
 *  a BOOLEAN_ATOM_INST. If some of the logical relations are conditional,
 *  this information is also provided.
 */

#endif /* logrel_io_module */

