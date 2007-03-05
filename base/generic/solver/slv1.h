/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: slv1.h,v $
 *  Date last modified: $Date: 1997/07/18 12:15:57 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

/** @file
 *  MINOS solver registration module.
 *  <pre>
 *  Contents:     MINOS module
 *  </pre>
 *  @todo MINOS (solver/slv1.c) is out-of-date and will not compile.
 *        Should be either fixed or archived.
 */

#ifndef ASC_SLV1_H
#define ASC_SLV1_H

#include <system/slv_client.h>

/**	@addtogroup solver Solver
	@{
*/

typedef struct slv1_system_structure *slv1_system_t;

int slv1_register(SlvFunctionsT *sft);
/**<
 *  Registration function for the MINOS solver.
 *  This is the function that tells the system about the MINOS solver.
 *  Our index is not necessarily going to be 1. That everything here is
 *  named slv1* is just a historical event.
 *
 *  @param sft SlvFunctionsT to receive the solver registration info.
 *  @return Returns non-zero on error (e.g. f == NULL), zero if all is ok.
 */

/* @} */

#endif  /* slv1__already_included */

