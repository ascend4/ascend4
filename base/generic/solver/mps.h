/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1995 Craig Schmidt
	Copyright (C) 2007 Carnegie Mellon University

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
	write_MPS: create the actual MPS file

	This module will create an MPS file representation
	of the current system.  It is passed a mps_data_t
	data structure, the solver subparameters, and the
	name of the file.
*//*
	by Craig Schmidt, 2/19/95
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/18 12:14:48 $ $Author: mthomas $
*/

#ifndef ASC_MPS_H
#define ASC_MPS_H

#include <utilities/ascConfig.h>
#include <system/var.h>

/**	@addtogroup solver Solver
	@{
*/

#ifdef STATIC_MPS

/** 
 *  Writes out a map file.
 *  The VXXXXXXX variable names are mapped to the actual ASCEND names.
 *
 *  @param name   File name to receive the output.
 *  @param vlist  Variable list (NULL terminated).
 *  @return Returns zero on success, non-zero if an error occurred.
 */
extern boolean write_name_map(const char *name,
                              struct var_variable **vlist);

/*
 *  Writes out the MPS file.
 *
 *  @param name   File name to receive the output.
 *  @param mps    The main chunk of data for the problem.
 *  @param iarray Integer subparameters, array of size slv6_IA_SIZE.
 *  @param rarray Real subparameters, array of size slv6_RA_SIZE.
 *  @return Returns zero on success, non-zero if an error occurred.
 */
extern boolean write_MPS(const char *name,
                         mps_data_t mps,
                         int iarray[],
                         double rarray[]);   

#endif  /* STATIC_MPS */
/* @} */

#endif  /* ASC_MPS_H */

