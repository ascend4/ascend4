/*
 *  SolverGlobals.h
 *  by Ben Allan
 *  Created: 7/97
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: SolverGlobals.h,v $
 *  Date last modified: $Date: 1997/07/18 12:23:49 $
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

#ifndef SolverGlobals_module_loaded
#define SolverGlobals_module_loaded

/*
 * The following global variables are used to maintain
 * a single global solver system.
 * This is bad.
 * The storage of these variables is in SolverProc.c.
 */
extern int g_solvinst_ndx, g_solvinst_limit;
extern slv_system_t g_browsys_cur, g_solvsys_cur;
extern struct Instance *g_solvinst_root;      /* solver root instance */
extern struct Instance *g_solvinst_cur;       /* solver current instance */

#endif /* SolverGlobals_module_loaded */
