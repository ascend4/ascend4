/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: system.h,v $
 *  Date last modified: $Date: 1997/07/18 12:17:26 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

/*************************************************************************
 ***  Contents:     System module (ascend)
 ***
 ***  Authors:      Joseph Zaher
 ***
 ***  Dates:        04/94 - original version by Joe Zaher
 ***                06/96 - original total reimplementation by Ben Allan
 ***
 ***  Description:  This is the ascend version of the system module.
 ***                This provides a means of extracting a solver system
 ***                from an ASCEND instance tree.
 ***                The result is supposed to appear totally instance
 ***                independent, though it is not really, so that
 ***                only the var and rel modules need to know about
 ***                the ascend compiler.
 ***                With any luck the expr, exprman, and relman files leave.
 *************************************************************************/
#ifndef system__already_included
#define system__already_included
/* requires #include "slv_types.h" */

extern slv_system_t system_build(SlvBackendToken);
/**
 ***  sys = system_build(inst)
 ***  slv_system_t sys;
 ***  SlvBackendToken inst;
 ***
 ***  In ascend, backendtoken is a struct Instance *.
 ***  Visits the instance tree and collects all variables,
 ***  relations, and objective functions into pointer lists.
 **/

extern void system_destroy(slv_system_t);
/**
 ***  system_destroy(sys)
 ***  slv_system_t sys;
 ***
 ***  Destroys the latest model formulation.
 **/

extern void system_free_reused_mem(void);
/**
 ***  system_free_reused_mem()
 ***
 ***  Deallocates any memory that solvers may be squirrelling away for
 ***  internal reuse. Calling this while any slv_system_t exists
 ***  is likely to be fatal: handle with care.
 **/

#endif
