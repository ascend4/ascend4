/*
 *  Lsode.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: Lsode.h,v $
 *  Date last modified: $Date: 1997/07/18 12:23:18 $
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

#ifndef lsode__already_included
#define lsode__already_included

/* requires #include "slv_client.h" */
/* requires #include "Integrators.h" */

extern void Asc_BLsodeIntegrate(slv_system_t, unsigned long ,unsigned long,
                             struct Integ_system_t *);
/*
 *  void Asc_BLsodeIntegrate(sys,start_index, finish_index,blsys)
 *  Takes the start and finish index as defined by the user and carries
 *  out the integration using repeated calls to the function lsode.
 *  Assumes sys corresponds to g_solvinst_cur.
 *  works off instances of type blsode taken from blsys.
 */

/*
 *  The macro DOTIME is defined inside Lsode.c.
 *  If its value is TRUE, we spew all sorts of time junk.
 *  If FALSE we are quiet. Default for this macro is FALSE.
 */

#endif
