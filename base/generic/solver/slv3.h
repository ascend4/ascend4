/*
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: slv3.h,v $
 *  Date last modified: $Date: 1997/07/18 12:16:16 $
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
 *  QRSlv solver registration module.
 *  <pre>
 *  Contents:     QRSlv module
 *
 *  Authors:      Ben Allan
 *
 *  Dates:        02/95 - original version
 *                Test bed version of Slv for linear solver changes
 *                and eventually Slv kernal<-->ASCEND changes.
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "slv_client.h"
 *  </pre>
 */

#ifndef ASC_SLV3_H
#define ASC_SLV3_H

typedef struct slv3_system_structure *slv3_system_t;

int slv3_register(SlvFunctionsT *f);
/**<
 *  Registration function for the ASCEND QRSlv nonlinear solver.
 *  This is the function that tells the system about the QRSlv solver.
 *  Our index is not necessarily going to be 3.  That everything here is
 *  named slv3* is just a historical result and a convenient way of
 *  shutting up the linker.
 *
 *  @param f SlvFunctionsT to receive the solver registration info.
 *  @return Returns non-zero on error (e.g. f == NULL), zero if all is ok.
 */

#endif  /* ASC_SLV3_H */

