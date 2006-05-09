/*
 *  ScriptProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: ScriptProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
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
 *  Script procedures.
 *  <pre>
 *  Requires:     #include "tcl.h"
 *                #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASCTK_SCRIPTPROC_H
#define ASCTK_SCRIPTPROC_H

extern int Asc_ScriptInterrupt;
/**<
 * The C variable corresponding to the script being interrupted
 * by an interactive user or call with a trapped exception.
 */

extern int Asc_ScriptConfigureInterrupt(int bool, Tcl_Interp *interp);
/**<
 * <!--  err = Asc_ScriptConfigureInterrupt(bool,interp);              -->
 * <!--  int err,bool;                                                 -->
 * Configures the script variable ascScripVect(menubreak) to
 * shadow the C int Asc_ScriptInterrupt.
 * Tcl makes sure changes to ascScripVect(menubreak)
 * are reflected in the C variable.
 * See the man pages of Tcl_LinkVar, Tcl_UnlinkVar, Tcl_UpdateLinkedVar
 * for details.
 *
 * @param interp The interpretter the ascend application is running in.
 * @param bool   Is 1 at startup and 0 at shutdown.
 * @return Returns 0 if the call is ok, 1 if the call is not ok.
 */

extern int Asc_ScriptEvalCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[]);
/**<
 *  <!--  int Asc_ScriptEvalCmd;                                       -->
 *  Attempts to evaluate in the global sphere rather than locally.<br><br>
 *
 *  Registered as : \"script_eval\" string";
 */

extern int Asc_ScriptRefineCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, CONST84 char *argv[]);
/**<
 *  Refines the qlfdid given to type if possible. if qlfdid exists and
 *  is none null, return will be as Asc_BrowInstanceRefineCmd().<br><br>
 *
 *  Registered as: srefine <type> search <qlfdid>
 */

extern int Asc_ScriptMergeCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/**<
 *  Merges the qlfdid if possible.<br><br>
 *
 *  Registered as: smerge <qlfdid> <qlfdid>
 */

extern int Asc_FastRaiseCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/**<
 *  Calls XRaiseWindow rather than the big tk spew which is so slow.<br><br>
 *
 *  Registered as:  asc_raise windowname
 */

STDHLF_H(Asc_TimeCmd);

extern int Asc_TimeCmd(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[]);
/**<
 *  Timing function.
 *  string is a valid tcl script to be evaluated globally as script_eval.
 *  n is the number of times to evaluate the script for an average time.
 *  n defaults to 1 since many ASCEND scripts are not repeatable.<br><br>
 *
 *  Registered as:  asc_clock <string> [n]
 */

/** Long help text part 1 */
#define Asc_TimeCmdHL1 \
"\
 *  Returns a list of numbers: \n\
 *  {real_time_avg time_avg clock_avg clock_max clock_min CPS} \n\
 *  real_time_avg- average real seconds consumed per evaluation of string. \n\
 *  time_avg- average cpu seconds consumed per evaluation of string. \n\
"
/** Long help text part 2 */
#define Asc_TimeCmdHL2 \
"\
 *  clock_avg- average clock units consumed per evaluation of string. \n\
 *  clock_max- most clock units consumed by a single call \n\
 *  clock_min- least clock units consumed by a single call \n\
 *  CPS- CLOCKS_PER_SEC value by which times are computed from clocks. \n\
"

extern int Asc_StringCompact(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[]);
/**<
 *  Takes a string and eats away any extra blanks or tabs that are
 *  not protected by matched {}. replaces unprotected tabs with a
 *  single space. trims leading/trailing blanks/tabs. Doesn't
 *  treat \n as a special character.<br><br>
 *
 *  Registered as:  stringcompact <string>
 */

#endif  /* ASCTK_SCRIPTPROC_H */

