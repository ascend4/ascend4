/*
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: slv7.h,v $
 *  Date last modified: $Date: 1997/07/18 12:16:33 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
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
 *  Nonlinear Solver (NGSlv) registration module.
 *  <pre>
 *  Contents:     NGSlv module
 *
 *  Authors:      Ben Allan, Kenneth Tyner
 *
 *  Dates:        02/96 - copy of QRSlv: Modifications Pending
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "slv_client.h"  
 *  </pre>
 *  @todo Update Dates note when modifactions complete.
 */

#ifndef slv7__already_included
#define slv7__already_included

typedef struct slv7_system_structure *slv7_system_t;

int slv7_register(SlvFunctionsT *);
/**<
 *  This is the function that tells the system about the NGSlv solver.
 *  Our index is not necessarily going to be 0. That everything here is
 *  named slv7* is just a historical event.
 */

#endif  /* slv7__already_included */

