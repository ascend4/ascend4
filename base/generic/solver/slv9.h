/*
 *  Conditional Modeling Solver
 *  by Vicente Rico-Ramirez
 *  Created: 04/97
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: slv9.h,v $
 *  Date last modified: $Date: 1997/07/29 15:48:07 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the SLV solver.
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
 *  Conditional Modeling Solver (CMSlv) registration module.
 *  <pre>
 *  Contents:     CMSlv module (Conditional Modeling Solver)
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "slv_client.h"
 *  </pre>
 */

#ifndef ASC_SLV9_H
#define ASC_SLV9_H

typedef struct slv9_system_structure *slv9_system_t;

int slv9_register(SlvFunctionsT *f);
/**<
 *  Registration function for the ASCEND CMSlv solver.
 *  This is the function that tells the system about the CMSlv solver.
 *  Our index is not necessarily going to be 9. That everything here is
 *  named slv9* is just a historical result and a convenient way of
 *  shutting up the linker.
 *
 *  @param f SlvFunctionsT to receive the solver registration info.
 *  @return Returns non-zero on error (e.g. f == NULL), zero if all is ok.
 */

#endif  /* ASC_SLV9_H */

