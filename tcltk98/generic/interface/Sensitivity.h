/*
 *  Sensitivity.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: Sensitivity.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:08 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
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
 *  Sensititvity analysis code.
 *  <pre>
 *  Requires:     #include "tcl.h"
 *                #include "utilities/ascConfig.h"
 *                #include "solver/slv_client.h"
 *  </pre>
 *  @todo Do we really need 2 files called sensitivity.[ch]?  Other one in base/packages.
 */

#ifndef _sensitivity_h_seen_
#define _sensitivity_h_seen_

extern int Asc_BLsodeDerivatives(slv_system_t sys,
                                 double **dy_dx,
                                 int *inputs_ndx_list,
                                 int ninputs,
                                 int *outputs_ndx_list,
                                 int noutputs);
/**<
 *  The entry point for the Lsode Integrator.
 *  This needs further explanation, but roughly:
 *  dy_dx IS_A 2d array of sensitivity partial derivatives.
 *  ninputs  the number of fixed state variables in the DAE.
 *  noutputs the number of derivatives
 *  ndx_list the var indices of the variables in question.
 *
 *  @todo Asc_BLsodeDerivatives() does not pass in a list of 
 *        var_variables instead of indices, as it should.
 */

extern int Asc_MtxNormsCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[]);
/**<
 *  Calculates some matrix norms and a condition number.
 *  This code is placed here until we can find a proper home for it.
 *  It should perhaps be in DebugProc[12].[ch].  In the current incarnation
 *  is to be in conjuction with some sensiticity ananlysis code.
 *
 *  @todo Find proper home for Asc_MtxNormsCmd().
 */

#endif  /* _sensitivity_h_seen_ */

