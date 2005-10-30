/*
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: slv0.h,v $
 *  Date last modified: $Date: 1997/07/18 12:15:51 $
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
 *  Slv solver registration module.
 *  <pre>
 *  Contents:     Slv module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                04/91 - fine tuned modified marquadt computation,
 *                        provided minor iterations for step generation
 *                        within each major iteration of jacobian
 *                        updates
 *                06/93 - eliminated pointer sublists being generated
 *                        at the beginning of each block
 *                04/94 - extended scope to equality constrained
 *                        optimization.
 *
 *  Description:  This file is created by make_slv_header, so don't
 *                modify it yourself.  All functions defined in this
 *                header have identical protocols to the corresponding
 *                functions in slv.h except that slv_system_t ==>
 *                slv0_system_t and slv0_eligible_solver() only takes one
 *                parameter: the system.  Note also that the select
 *                solver functions don't exist.
 *
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "slv_client.h"
 *  </pre>
 *  @todo Slv (solver/slv0.c) is out-of-date and will not compile.
 *        Should be either fixed or archived.
 */

#ifndef slv0__already_included
#define slv0__already_included

typedef struct slv0_system_structure *slv0_system_t;

int slv0_register(SlvFunctionsT *sft);
/**<
 *  Registration function for the Slv nonlinear solver.
 *  This is the function that tells the system about the QRSlv solver.
 *  Our index is not necessarily going to be 0. That everything here is
 *  named slv0* is just a historical event.
 *
 *  @param sft SlvFunctionsT to receive the solver registration info.
 *  @return Returns non-zero on error (e.g. f == NULL), zero if all is ok.
 */

#endif  /* slv0__already_included */

