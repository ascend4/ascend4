/*	ASCEND modelling environment
	Copyright 1997, Carnegie Mellon University
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Sensititvity analysis code.

	Requires:     
	#include "tcl.h"
	#include "utilities/ascConfig.h"
	#include "solver/slv_client.h"

	@todo Do we really need 2 files called sensitivity.[ch]?  Other one in base/packages.
*//*
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: Sensitivity.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
 *  Last modified by: $Author: ballan $
*/



#ifndef ASCTK_SENSITIVITY_H
#define ASCTK_SENSITIVITY_H

extern int Asc_MtxNormsCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/**<
 *  Calculates some matrix norms and a condition number.
 *  This code is placed here until we can find a proper home for it.
 *  It should perhaps be in DebugProc[12].[ch].  In the current incarnation
 *  is to be in conjuction with some sensiticity ananlysis code.
 *
 *  @todo Find proper home for Asc_MtxNormsCmd().
 */

#endif  /* _ASCTK_SENSITIVITY_H */

