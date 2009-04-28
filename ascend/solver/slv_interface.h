/*
 *  Interface to Karl Westerberg's Solver
 *  Tom Epperly
 *  Created: June, 1990
 *  Copyright (C) 1990 Thomas Guthrie Epperly
 *  Patched 1/94 for ASCEND3C -baa
 *  Only Solve is implemented in slv_interface.c
 *                            
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: slv_interface.h,v $
 *  Date last modified: $Date: 1997/07/18 12:17:10 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  Interface to the Westerberg SLV Solver.

 *  @todo Clean junk out of solver/slv_interface.h.
 */

#ifndef ASC_SLV_INTERFACE_H
#define ASC_SLV_INTERFACE_H

#include <utilities/ascConfig.h>
/* #include <compiler/instance_enum.h> */

/**	@addtogroup solver Solver
	@{
*/

/* ASC_DLLSPEC void Solve(struct Instance *inst); */
/**<
 *  This is the link that the command line interface should call. 

	@TODO can we delete this?
 */

#define JACFUNC void (*)(int,int,double)
/**<
 *  Type definition for storing Jacobian elements.  The function definition
 *  is as follows:
 *
 *  void JacStore(row,col,value)
 *  int row,col;
 *  double value;
 * 
 *  @todo If still needed, should it be a typedef?
 */

#define SLOPEFUNC void (*)(int,int,double,double)
/**<
 *  Type definition for storing slope matrix elements.  The function
 *  definition is as follows:
 *
 *  void SlopeStore(row,col,low,high)
 *  int row,col;
 *  double low,high;
 *
 *  @todo If still needed, should it be a typedef?
 */

#endif  /* ASC_SLV_INTERFACE_H */

