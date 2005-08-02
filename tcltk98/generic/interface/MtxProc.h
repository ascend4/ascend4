/*
 *  MtxProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: MtxProc.h,v $
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
 *  Incidence matrix routines.
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *      #include "interface/MtxProc.h"
 *  </pre>
 */

#ifndef MtxProc_module_loaded
#define MtxProc_module_loaded

extern int Asc_MtxGUIPlotIncidence(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  Plot the current incidence matrix based on the solver system following
 *  the parameters given.<br><br>
 *
 *  Registered as:
 *      mtx_gui_plot_incidence <sf xoff yoff cname bmfg bmbg ra ca va ea>
 *  <pre>
 *  The inputs are:
 *  sf (1-14): (this range should perhaps be queried about first, but
 *               we are foregoing that nicety at the moment.)
 *  xoff, yoff: canvas plot offsets from 0,0
 *  cname: an existing(!) tk canvas name
 *  bmfg: bitmap foreground color
 *  bmbg: bitmap background color
 *  ra: name of canvas rowindex to relation number conversion array
 *  ca: name of canvas colindex to variable number conversion array
 *     The following note is not yet implemented.
 *       Note that in the case of slack variables, the "variable number"
 *       will be preceded by a "-" and the number is the number of the
 *       relation for which that var is the slack.
 *     Until it is implemented, all slacks have "variable number" -1.
 *  va: name of variable number to canvas colindex conversion array
 *       Inverts what ca does.
 *  ea: name of relation number to canvas rowindex conversion array
 *       Inverts what ra does.
 *
 *  The outputs are:
 *  0 if ok, an error message otherwise.
 *  side effects: the canvas gets plotted
 *  in array ra : the element "num" and a bunch of elements with numeric
 *                 names are set. num is the number of elements set.
 *                 $ra($i) is the relation index corresponding to the
 *                 i'th row displayed on the canvas.
 *  in array ca : the element "num" and a bunch of elements with numeric
 *                 names are set. num is the number of elements set.
 *                 $ca($i) is the variable index corresponding to the
 *                 i'th column displayed on the canvas. It is something
 *                 odd (see above) if the variable is not on the varlist.
 *  in array ea : the element "num" and a bunch of elements with numeric
 *                 names are set. num is the number of elements set.
 *                 $ea(i) is the canvas row index corresponding to
 *                 relation number i displayed on the canvas.
 *  in array va : the element "num" and a bunch of elements with numeric
 *                 names are set. num is the number of elements set.
 *                 $va(i) is the canvas colindex corresponding to
 *                 variable number i displayed on the canvas.
 *              For the moment, slacks do not appear in va.
 *  scrollregion list {xul yul xlr ylr}
 *
 *  On entry, the canvas must exist. The arrays will be created if they
 *  do not exist. ra, ca, va, ea should be the names of global tcl arrays.
 *  The caller is responsible for cleaning off the canvas and clearing
 *  the arrays before calling, if those actions are desired. We don't
 *  mind stomping all over them if they are already filled.
 *
 *  Bugs: This is all one big bug because tcl is too slow.
 *  What the canvas looks like:
 *  The following formats should be input somehow rather than hardwired.
 *  asc_sq_: used for variable free and incident on included relations.
 *  asc_sq_h: used for variable free and incident on unincluded relations.
 *  asc_sq_x: used for variable fixed and incident on included relations.
 *  asc_sq_c: used for variable fixed and incident on unincluded relations.
 *  asc_lrt_: used for slack variable incident on included relation.
 *  asc_lrt_h: used for slack variable incident on unincluded relation.
 *
 *  The incidence matrix will contain only active vars and active rels
 *  </pre>
 */

extern int Asc_MtxHelpList(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/**<
 *  mtxhelp command for tcl.
 *  no arg -> return tcl list.
 *  "s" -> list names only, "l" -> short explanations also, to stderr.<br><br>
 *
 *  Registered as:  mtxhelp [s,l]
 */

#endif  /* MtxProc_module_loaded */

