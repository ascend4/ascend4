/*
 *  Dimensions Output Routine
 *  by Tom Epperly
 *  Created: 2/14/90
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: dimen_io.h,v $
 *  Date last modified: $Date: 1998/04/11 01:31:10 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Dimensions Output Routine
 *  <pre>
 *  When #including dimen_io.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *  </pre>
 */

#ifndef ASC_DIMEN_IO_H
#define ASC_DIMEN_IO_H

/**	addtogroup compiler Compiler
	@{
*/

/**
 * Write the human readable and parser edible string form of the
 * dimen given.
 */
ASC_DLLSPEC char *WriteDimensionString(CONST dim_type *p);

/** 
 * Write the canonical, indigestible form of the dimen with
 * full numeric details. ugh.
 */
extern void WriteDimensions(FILE *f, CONST dim_type *dimp);

/* @} */

#endif /* ASC_DIMEN_IO_H */

