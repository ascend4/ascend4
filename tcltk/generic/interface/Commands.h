/*
 *  Commands.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: Commands.h,v $
 *  Date last modified: $Date: 1997/07/18 12:22:40 $
 *  Last modified by: $Author: mthomas $
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
 *  Command Management Routines
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *      #include "interface/HelpProc.h"
 *      #include "interface/Commands.h"
 *  </pre>
 */

#ifndef ASCTK_COMMANDS_H
#define ASCTK_COMMANDS_H

extern void Asc_CreateCommands(Tcl_Interp *interp);
/**<
 *  <!--  void Asc_CreateCommands(interp);                             -->
 *  Registers all C written code so that the
 *  Tcl interpreter can see them. All Tcl commands should be registered
 *  here.
 */

extern void Asc_AddCommand(Tcl_Interp *interp, 
                           char *cmdName, 
                           Tcl_CmdProc *proc,
                           ClientData cdata, 
                           Tcl_CmdDeleteProc *deleteProc,
                           CONST char *group, 
                           CONST char *usage,
                           CONST char *shorth,
                           HLFunc longh);
/**<
 * <!--  Asc_AddCommand(interp,cmdName,proc,cdata,deleteProc,          -->
 * <!--                 group,usage,shorth,longh)                      -->
 * Adds a command to tcl and the ASCEND commandline help data structures.
 * This function should be used for commands created by dynamically
 * linked packages that want to use the ascend help facility.
 */

#define ASCADDCOM(interp,cmdName,proc,group,usage,shorth,longh) \
  Asc_AddCommand((interp), (cmdName), (proc), \
                 (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL, \
                 (group),(usage),(shorth),(longh))
/**<
 * The easier macro form of Asc_AddCommand used for those commands which
 * do not have tk-widget-like behavior or clientdata.
 */

#endif  /* ASCTK_COMMANDS_H */

