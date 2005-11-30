/*
 *  Qlfdid.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: Qlfdid.h,v $
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

#ifndef ASCTK_QLFDID_H
#define ASCTK_QLFDID_H

#include "compiler/qlfdid.h"

extern int Asc_BrowQlfdidSearchCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, CONST84 char *argv[]);
/**<
 *  <!--  Asc_BrowQlfdidSearchCmd                                      -->
 *  Will take a fully qualified ascend name and will search for the instance
 *  with that name. Will leave g_search_inst looking at the named instance
 *  if successful and will return a parsed string. Otherwise should return
 *  a TCL_ERROR with the message "Orphaned part", where part is the portion
 *  that could not be found.<br><br>
 *
 *  Registered as : \"qlfdid name\"
 */

#endif  /* ASCTK_QLFDID_H */

