/*
 *  AscBitmaps.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: AscBitmaps.h,v $
 *  Date last modified: $Date: 1998/04/25 18:14:15 $
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
 *  Registration of Tcl/Tk bitmaps.
 *  <pre>
 *  To include this header, you must include the following:
 *      #include <tcl.h>
 *      #include <utilities/ascConfig.h>
 *      #include "AscBitmaps.h"
 *  </pre>
 */

#ifndef AscBitmap_module_loaded
#define AscBitmap_module_loaded

/**
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
extern int Asc_RegisterBitmaps(Tcl_Interp *interp);

#endif  /* AscBitmap_module_loaded */

