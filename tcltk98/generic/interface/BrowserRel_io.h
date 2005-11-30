/*
 *  BrowserRel_io.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: BrowserRel_io.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:05 $
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
 *  Relation IO Routines
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *      #include "interface/BrowserRel_io.h"
 *  </pre>
 */

#ifndef ASCTK_BROWSERREL_IO_H
#define ASCTK_BROWSERREL_IO_H

extern int Asc_BrowWriteRelListCmd(ClientData cdata,Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Write the relation in infix to the interpreter. This gracefully
 *  handles all the types of relations now supported in ASCEND.
 *  If conditional relations exist,It separates the relation list
 *  in two lists. The list of  normal
 *  relations and the list of conditional relations.
 *  Conditional relations do not have to be solved, they
 *  represent boundaries in conditional modeling.<br><br>
 *
 *  Registered as: bgetrels
 */

extern int Asc_BrowWriteCondRelListCmd(ClientData cdata,Tcl_Interp *interp,
                                       int argc, CONST84 char *argv[]);
/**<
 *  Write the list of conditional relations in infix to the interpreter.
 *  This gracefully handles all the types of relations now supported in
 *  ASCEND.<br><br>
 *
 *  Registered as: bgetcondrels
 */

extern int Asc_BrowWriteRelListPostfixCmd(ClientData cdata,Tcl_Interp *interp,
                                          int argc, CONST84 char *argv[]);
/**<
 *  Writes to the interpreter rather than to a file.
 *  As I can see Kirk did not finish this implementation for all the types
 *  of relations.<br><br>
 *
 *  Registered as: bgetrelspf.
 */

extern int Asc_BrowWriteRelsForAtomCmd(ClientData cdata,Tcl_Interp *interp,
                                       int argc, CONST84 char *argv[]);
/**<
 *  Will return a proper Tcl list of all relations associated with
 *  the given atom. Works on the current or the search instance.
 *  Will return TCL_ERROR if the instance is NULL, or the atom is not
 *  a REAL_ATOM_INST. This will be later extended to deal with boolean
 *  atoms. If some of the relations are conditionals, this information
 *  is also provided.<br><br>
 *
 *  Registered as: __brow_relsforatom ?cur?search.
 */

#endif  /* ASCTK_BROWSERREL_IO_H */

