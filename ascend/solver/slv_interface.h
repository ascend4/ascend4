/*	ASCEND modelling environment
	Copyright (c) 1990 Thomas Guthrie Epperly
	Copyright (c) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (c) 2011 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Interface to Karl Westerberg's Solver.

	@DEPRECATED This header file relates to a disused command-line based interface
	for the ASCEND system. It might still be possible to do something useful
	with this though, so it has been kept for now -- JP, Feb 2011.

	@todo Clean junk out of solver/slv_interface.h.
*//*
 *  Interface to Karl Westerberg's Solver
 *  Tom Epperly
 *  Created: June, 1990
 *  Copyright (C) 1990 Thomas Guthrie Epperly
 *  Patched 1/94 for ASCEND3C -baa
 *  Only Solve is implemented in slv_interface.c
 *  Last in CVS: $Revision: 1.4 $ $Date: 1997/07/18 12:17:10 $ $Author: mthomas $
 */

#ifndef ASC_SLV_INTERFACE_H
#define ASC_SLV_INTERFACE_H

#include <ascend/general/platform.h>
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

/* @} */

#endif  /* ASC_SLV_INTERFACE_H */

