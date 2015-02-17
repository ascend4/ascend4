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
	Registration of Tcl/Tk bitmaps.

	Requires
	#include <tcl.h>
	#include <utilities/ascConfig.h>
	#include "AscBitmaps.h"
*//*
 *  AscBitmaps.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: AscBitmaps.h,v $
 *  Date last modified: $Date: 1998/04/25 18:14:15 $
 *  Last modified by: $Author: ballan $
*/

#ifndef ASCTK_ASCBITMAPS_H
#define ASCTK_ASCBITMAPS_H

extern int Asc_RegisterBitmaps(Tcl_Interp *interp);
/**<
 *  Register all the bitmaps so we don't have to lug them around in files.
 *  For starters we define square type bitmaps in 1x1 -> 14x14 size
 *  named as follows:
 *  - asc_sq_N => a solid square NxN
 *  - asc_sq_hN => a hollow square NxN
 *  - asc_sq_xN => a hollow, diagonally crossed square NxN
 *  - asc_sq_cN => a diagonal cross in the space of an NxN square
 *  - *(unimplemented)
 *  - * asc_lrt_N => solid lower right triangle in the space of an NxN square
 *  - * asc_lrt_hN => solid lower right triangle in hollow of an NxN square
 *  - * asc_llt_N => solid lower left triangle in the space of an NxN square
 *  - * asc_llt_hN => solid lower left triangle in hollow of an NxN square
 *  - * asc_urt_N => solid upper right triangle in the space of an NxN square
 *  - * asc_urt_hN => solid upper right triangle in hollow of an NxN square
 *  - * asc_ult_N => solid upper left triangle in the space of an NxN square
 *  - * asc_ult_hN => solid upper left triangle in hollow of an NxN square
 *  Each type is registered in all sizes, though in the small N
 *  some types are indistiguishable.
 *  Next toolAttributes (the grill) is registered.
 *  a solver stopsign (stop)is registered
 *  flying and not flying shoe is registered  wfeet feet
 *  grablock is registered (lock stolen from lire).
 */

#endif  /* ASCTK_ASCBITMAPS_H */

