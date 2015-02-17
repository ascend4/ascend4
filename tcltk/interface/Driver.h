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
*//**
	@file
	Main Tcl/Tk Driver.
*//*
	Driver.h
	by Kirk Abbott and Ben Allan
	Created: 1/94
	Version: $Revision: 1.3 $
	Version control file: $RCSfile: Driver.h,v $
	Date last modified: $Date: 2003/08/23 18:43:06 $
	Last modified by: $Author: ballan $
*/

#ifndef ASCTK_DRIVER_H
#define ASCTK_DRIVER_H

extern Tcl_Interp *g_interp;
/**<
 *  Interpreter for this application.  We need tp make it global
 *  so that our signal/floating-porint traps can access it.
 */

ASC_EXPORT int AscDriver(int argc, CONST char **argv);
/**<
	Entry point from main.c
*/

void
Asc_Prompt(Tcl_Interp *interp, int partial);
/**<
	Only needed here for use in Ctrl-C handler in SolverProc.c
*/

extern int Asc_LoadWin(ClientData, Tcl_Interp *, int, CONST84 char**);
/**<
 *  Tcl callback to determine if Tcl/Tk should build Tk windows.
 *  Returns 1 if it should, 0 if we only want a command line.<br><br>
 *
 *  Currently (06/97), this call is pointless since we do not
 *  support command-line-only operation.
 */


#ifdef DEBUG_MALLOC
extern int Asc_DebugMallocCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/**<
 *  Tcl callback for Tom Epperly's debugging malloc.
 */
#endif  /* DEBUG_MALLOC */

#endif  /* ASCTK_DRIVER_H  */

