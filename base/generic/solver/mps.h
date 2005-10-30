/*
 *  write_MPS: create the actual MPS file
 *  by Craig Schmidt
 *  Created: 2/19/95
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: mps.h,v $
 *  Date last modified: $Date: 1997/07/18 12:14:48 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Craig Schmidt
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

/** @file
 *  write_MPS: create the actual MPS file.
 *  <pre>
 *  Contents:     MPS module
 *
 *  Authors:      Craig Schmidt
 *
 *  Dates:        02/95 - Original version
 *
 *  Description:  This module will create an MPS file representation
 *                of the current system.  It is passed a mps_data_t
 *                data structure, the solver subparameters, and the
 *                name of the file.
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "slv6.h"
 *  </pre>
 */

#ifndef MPS__already_included
#define MPS__already_included

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
#endif  /* MPS__already_included */

