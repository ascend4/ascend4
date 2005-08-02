/*
 *  Incorporation of the nonlinear solver CONOPT to ASCEND
 *  by Ken Tyner 
 *  Created: 6/97
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: slv8.h,v $
 *  Date last modified: $Date: 1997/08/12 16:43:46 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 *  CONOPT solver registration module.
 *  <pre>
 *  Contents:     CONOPT module
 *
 *  Authors:      Ken Tyner and Vicente Rico-Ramirez
 *
 *  Dates:        06/97 - original version
 *                07/97 - Creating a structure of function pointers so that
 *                        multiple calls to CONOPT can be made
 *                08/97 - Improving CONOPT message report to ASCEND
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "slv_client.h"
 *  </pre>
 */

#ifndef slv8__already_included
#define slv8__already_included

typedef struct slv8_system_structure *slv8_system_t;

int slv8_register(SlvFunctionsT *);
/**< 
 *  This is the function that tells the system about the CONOPT solver.
 *  Our index is not necessarily going to be 8. That everything here is
 *  named slv8* is just a historical result and a convenient way of
 *  shutting up the linker.
 */

#endif  /* ifndef slv8__already_included */

