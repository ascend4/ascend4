/*
 *  SlvProc.h
 *  by Ken Tyner and Ben Allan
 *  Created: 6/97
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: SlvProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
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
 *
 *  This probe implementation constitutes a total rework of the probe
 *  first created by Abbott, 1994.
 */

#ifndef SlvProc_module_loaded
#define SlvProc_module_loaded

/*
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "interface/HelpProc.h"
 *      #include "interface/SlvProc.h" (which is sort of obvious)
 */

#define SLVMONITORPREFIX "slvmon"

extern int Asc_VarAnalyzeCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[]);
extern int Asc_RelAnalyzeCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[]);
/*
 *  int Asc_VarAnalyzeCmd;
 *  Registered as \"__var_analyze low high
 *                  scaling?lower?upper?other
 *  rel?abs tolerance <othervalue>.
 *  int Asc_RelAnalyzeCmd;
 *  Registered as \"__rel_analyze low high
 *                  residual?other
 *  rel?abs tolerance <othervalue>.
 *  Comments :
 *
 *  This function will analyze the elements in the current slv_systemto see
 *  whether they satisfy the proximity queries. The variable type supported is
 *  the "solver_var". It can be used to analyse variables or relations for
 *  proximity to a "othervalue". rel?abs will determine whether
 *  the queries are done relatively or using absolute values. tolerance must
 *  be set appropriately based on rel?abs. The queries ?scaling?lower?upper
 *  pertain to the nominal,lower_bound, and upper_bound of a solver_var with
 *  respect to the value of the solver_var. "residual" pertains to the residual
 *  of a relation as compared to 0.0. If for any of the queries,
 *  the values are too close to zero, then "absolute" queries are made.
 *  Internal units are used for all queries.
 *
 *  Examples:
 *  1) __var_analyze 20 46 lower rel 0.2;
 *     will analyse (indexes 20 through 46 inclusive) all solver_vars
 *     that have values which are within 20% of their lower_bounds.
 *  2) __var_analyze 20 46 lower abs 0.2;
 *     will analyse (indexes 20 through 46 inclusive) all solver_vars
 *     that have values which are within 0.2 of their lower_bounds.
 *  3) __var_analyze 20 46 other rel 0.2 15.0;
 *     will analyse (indexes 20 through 46 inclusive) all solver_vars
 *     that have values which are within 20% of 15.0.
 *
 *  Will be return a formatted string with of the form:
 *  {index b_close} {index b_far} where index is the original probe list index.
 *  See the source for more details.
 */

STDHLF_H(Asc_SolveMonitorCmd);

extern int Asc_SolveMonitorCmd(ClientData, Tcl_Interp *, int, CONST84 char **);
/* 
 * status = Asc_SolveMonitorCmd((cdata,interp,argc,argv);
 * Registered as: */
#define Asc_SolveMonitorCmdHN "slv_monitor" 
/* which should eventually be __something, when we clean up the slv_ mess */
/* Usage: */
#define Asc_SolveMonitorCmdHU \
  "slv_monitor takes no arguments yet"
#define Asc_SolveMonitorCmdHS \
 "Returns the name of a monitor command to watch var/rel changes"
#define Asc_SolveMonitorCmdHL1 "Eventually this should take an argument "
#define Asc_SolveMonitorCmdHL2 "that is the id of a tcl solver token.\n"
#define Asc_SolveMonitorCmdHL3 "\
 * This creates a monitor and returns its symbolic handle.\n\
 * Multiple monitors can exist and are manipulated by their\n\
 * symbolic handles.\n\
 * A monitor may be used on a series of unrelated slv_system_t.\n\
 * Currently, this function gets its slv_system_t from g_solvsys_cur,\n\
 * but it should be changed to take a slvsys interface id when\n\
 * the solver interface is changed to work by name.\n\
"
/* 
 * because they need to be defined in a visible place, the info strings
 * for the monitors created follow:
 */
#define SolveMonitorHS \
 "Returns the list of changes in residuals, variables, or steps in variables"
#define SolveMonitorHU \
 "slvmonN destroy OR slvmonN slvsysname change <var,rel> OR\n\
slvmonN slvsysname geometry w h x y rmin rmax vmax OR \n\
slvmonN slvsysname plotdata <value,speed,residual>\n\
"
#define SolveMonitorHL1 \
"\
 * s change var         return scaled values that changed\n\
 * s change rel         return scaled residuals that changed\n\
 * Returns a tcl list whose elements are the changed values\n\
 * with master list indices {index value}\n\
 * values are scaled by nominals, or 1 if nominal is 0.\n\
 * Scaled values > vmax (or rmax) will be returned as having the\n\
 * value of the max, rather than their TRUE value.\n\
"
#define SolveMonitorHL2 \
"\n\
 * s geometry w h x y rmin rmax vmax\n\
 * sets conversion parameters for plotdata (w h x y)\n\
 * and limits for data values. This is called with a window\n\
 * size or range change that will require new drawing.\n\
 * x,y is the upper left corner of a coordinate space like a tk canvas.\n\
 * w,h is the width and height of the space in distinctly plottable points\n\
 * The 0 line of the area is y + h/2 for values. The values will be\n\
 * spread evenly over the width in order of increasing solver master index.\n\
"
#define SolveMonitorHL3 \
"\n\
 * s plotdata value     return plot info for scaled values that changed\n\
 * s plotdata speed     return plot info for scaled rates of value change\n\
 * s plotdata residual  return plot info for scaled residuals that changed\n\
 *\n\
"
#define SolveMonitorHL4 \
"\
 * Each option returns a list of {x y index} for changed values of the\n\
 * variables or relations. The x,y are coordinates at which a point\n\
 * should be plotted based on a transformation derived from whxy info\n\
 * last obtained by the geometry command of the monitor.\n\
"
#define SolveMonitorHL5 \
"\
 * The transformation may specify the same coordinate for more than\n\
 * one relation or variable.\n\
 * If this function raises a floating point exception, it will\n\
 * not return data, but an error.\n\
"
#define SolveMonitorHL6 \
"\
*	value:	The variable values v are scaled by nominal.\n\
*		They are then reduced to vmax if abs(v) > vmax.\n\
*		They are then scaled to the plotting region. The\n\
*		returned value is {plotx ploty mastervarindex} for\n\
*		all variables that changed since the last call or geometry.\n\
"
#define SolveMonitorHL7 \
"\
*	speed:	The variable values v are scaled by nominal.\n\
*		They are then reduced to vmax if abs(v) > vmax.\n\
* 		The delta in the scaled value since the last call\n\
*		is calculated. If the delta has changed, it is scaled\n\
*		to the plotting region and returned as\n\
*		{plotx ploty mastervarindex} for\n\
*		all variables that had a step size change.\n\
"
#define SolveMonitorHL8 \
"\
*  residual:	The relation residuals, as scaled by nominals and reduced\n\
*		to rmax if necessary, which have changed since the last call\n\
*		are transformed to a dual log scale in the plot region.\n\
*		Residuals < rmin considered as rmin. Residuals < 0 plotted\n\
*		in the lower half of the plot region. Distance from the\n\
*		vertical center of the canvas indicates the residual magnitude"
/* end o spew */

#endif /* SlvProc_module_loaded*/
