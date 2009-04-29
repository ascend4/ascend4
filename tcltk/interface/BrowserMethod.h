/*
 *  BrowserMethod.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: BrowserMethod.h,v $
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
 *  Browser Method Routines
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *      #include "interface/BrowserMethod.h"
 *  </pre>
 */

#ifndef ASCTK_BROWSERMETHOD_H
#define ASCTK_BROWSERMETHOD_H

STDHLF_H(Asc_BrowInitializeCmd);
extern int Asc_BrowInitializeCmd(ClientData cdata,
                                 Tcl_Interp *interp,
                                 int argc,
                                 CONST84 char **argv);
/** Registered as */
#define Asc_BrowInitializeCmdHN "brow_runmethod"
/**  Usage */
#define Asc_BrowInitializeCmdHU \
  Asc_BrowInitializeCmdHN "-method name -qlfdid instance_name options"
/**  Short help text */
#define Asc_BrowInitializeCmdHS \
  "Runs the method named in the instance named"
/**  Long help text */
#define Asc_BrowInitializeCmdHL "\
 * Runs a method with control of where output goes, what kind of error \n\
 * handling, and so forth according to the options:\n\
 *   -stopOnErr 1		causes error to be reported immediately.\n\
 *   -stopOnErr 0		ignores errors (message, but continue).\n\
 *   -backtrace 0		on stopping, offer only local message.\n\
 *   -backtrace 1		on stopping, print method stack unwind.\n\
 *   -output file		send output to file. If not set, to ASCERR.\n\
"

extern int Asc_BrowRunAssignmentCmd(ClientData cdata, Tcl_Interp *interp,
                                    int argc, CONST84 char *argv[]);
/**<
 *  <!--  Asc_BrowRunAssignmentCmd                                     -->
 *  usage: brow_assign [-search] value [units]  {browser inst assignment}
 *  Checks for Boolean, Integers and Reals.
 *  A null inst returns a TCL_ERROR.
 *  Bad data or setting an already set integer gets a TCL_ERROR w/message.
 *  If units are present and type is Real, will attempt to use units in
 *  assignment. If units dimensionally incompatible, returns an error.
 *  If units are missing or "*" will assign the value directly without
 *  conversion and without changing the dimensionality of the real.
 *  If value is "UNDEFINED" returns without doing anything, regardless
 *  of units.
 */

extern int Asc_BrowRunAssignQlfdidCmd2(ClientData cdata, Tcl_Interp *interp,
                                       int argc, CONST84 char *argv[]);
/**<
 *  <!--  Asc_BrowRunAssignQlfdidCmd2                                  -->
 *  Usage : qassgn2 qlfdid value [units] {qualified id assignment}
 *  See the notes for Asc_BrowRunAssignment. The same applies only this
 *  function requires a qulaified id. An errors will return a TCL_ERROR;
 */

extern int Asc_BrowRunAssignQlfdidCmd3(ClientData cdata, Tcl_Interp *interp,
                                       int argc, CONST84 char *argv[]);
/**<
 *  <!--  Asc_BrowRunAssignQlfdidCmd3                                  -->
 *  Usage : qassgn3 qlfdid value [units] [-relative] {qualified id assignment}
 *  Yet another variant to try to get some more speed. This version
 *  uses Asc_QlfdidSearch3, which is the mininal implementation of
 *  searching for a name.
 *  If -relative arguement provided then the assignment is expected to
 *  be relative to the results of the last call to qlfdid (tcl call).
 */

extern int Asc_BrowWriteProcedure(ClientData cdata, Tcl_Interp *interp,
                                  int argc, CONST84 char *argv[]);
/**<
 *  <!--  bgetproc procname pathname [search]                          -->
 *  Write the statements of a named procedure
 *  (assumed to be in g_curinst unless search appears)
 *  to a full file pathname given.
 */

extern int Asc_BrowSetAtomAttribute(Tcl_Interp *interp, 
                                    struct Instance *atominstance,
                                    symchar *childname, 
                                    enum inst_t childtype, 
                                    void *dataptr);
/**<
 * <!--  status = Asc_BrowSetAtomAttribute(interp,atominstance,        -->
 * <!--                                    childname,childtype,dataptr)-->;
 * Sets the value of an attribute of the ATOM/REL instance given.
 * Childname must be from the compiler symbol table via AddSymbol or
 * AddSymbolL. Childtype determines what dataptr contains.
 * Childtype must be REAL_INST, INTEGER_INST, BOOLEAN_INST, SYMBOL_INST.
 * SET_INST is not supported at this time. dataptr must be
 * an appropriate value object for each of the INST types above:
 * double *, long *, int *, symchar **, respectively.
 *
 * Notes:
 *  - A symbol value MUST come from the symbol table.
 *  - You cannot change the dimens of a real child this way
 *    and the double * given is assumed in SI.
 *
 * Return a value and message other than TCL_OK if these conditions
 * are not met. Except that if the childname or symbol value given
 * are not in the symbol table, then does not return.
 */

#endif  /* ASCTK_BROWSERMETHOD_H */

