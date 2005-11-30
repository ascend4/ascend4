/* 
 *  SlvDOF: Ascend Degrees of Freedom module
 *  by Benjamin A Allan and Vicente Rico-Ramirez
 *  Created: 7/11/94
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: slvDOF.h,v $
 *  Date last modified: $Date: 1998/03/30 22:07:03 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the ASCEND solver interface.
 *
 *  Copyright (C) 1994 Benjamin Andrew Allan
 *  Copyright (C) 1998 Vicente Rico-Ramirez
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
 *  COPYING.
 *  COPYING is found in ../compiler.
 */

/** @file
 *  Ascend Degrees of Freedom module
 *  <pre>
 *  Contents:     DOF module (Solver Degrees of Freedom)
 *
 *  Authors:      Ben Allan
 *
 *  Dates:        07/94 - original version
 *                06/96 - reimplementation
 *                03/98 - Added the consistency analysis for conditional
 *                        models
 *
 *  Description:  This file contains routines for managing degrees
 *                of freedom through the user interface. It uses
 *                its own incidence matrix separate from the mtx of
 *                any given solver.
 *
 *  Requires:     #include <stdio.h>
 *                #include "utilities/ascConfig.h"
 *                #include "slv_client.h"
 *  </pre>
 */

#ifndef slvdof_already_included
#define slvdof_already_included

/*
 * The following functions are for use in running DOF analysis and dialog
 * through the interface. The tcl callbacks for them will be coded in
 * DebugProc.c probably.
 * Partitioning is assumed to be in effect.
 * In general, the lists returned will be longer than number of degrees of
 * freedom which can be changed. DOF dialogs should be done 1 var/rel at a
 * time.
 */

extern int slvDOF_eligible(slv_system_t server, int32 **vil);
/**
 * @return 1 if able to determine lists, 0 otherwise.
 * @param vil pointer to an int32 array which will fill in and return
 * If return is 1, user should ascfree(*vil) when done with it. Return 0
 * -> vil will be null.
 * The index list is terminated with a -1 but may be of any length.
 * The indices are var_sindex of vars on solvers_var_list.
 * vil is incident vars eligible to be fixed.
 * This routine does not sort out which vars are in a particular part of
 * the MODEL represented by the system -- that is a UI job.
 * Vars that do not appear in equations will not be listed as eligible
 * since they cannot help the DOF state of the system.
 */

extern int slvDOF_structsing(slv_system_t server,
                             int32 relindex, 
                             int32 **vil, 
                             int32 **ril, 
                             int32 **fil);
/**< 
 *  <!--  if(slvDOF_structsing(server,relindex,&vil,&ril,&fil)) {.... } -->
 *  <!--  int32 *vil, *ril, fil.                                        -->
 *  Returns 1 if able to determine lists, 0 otherwise.
 *  relindex should be the sindex of an unassigned, included equation
 *  from the solvers_rel_list.
 *  If relindex is mtx_FIRST, the list returned will be the intersection of
 *  all singularity lists for any unassignable included equations in
 *  the system.
 *  If return is 1, user should free r/v/fil when done with
 *  them. Return 0 -> lists will be null.
 *  The index lists are terminated with a -1.
 *  The indices are *_sindex of vars/rels on solvers_*_list.
 *  @param vil  int32 **, the vars involved in a structural singularity.
 *  @param ril  int32 **, the rels involved in a structural singularity.
 *  @param fil  int32 **, the vars which reduce the structural singularity if freed.
 */

extern int32 slvDOF_status(slv_system_t server, int32 *status, int32 *dof);
/**<
 *  Return the status of the current problem.
 *
 *  - status = 1  ==> underspecified
 *  - status = 2  ==> square
 *  - status = 3  ==> structurally singular
 *  - status = 4  ==> overspecifed
 *  - status = 5  ==> Error !! ( insufficient memory, NULL argument, failed to presolve)
 *
 *  If the system is underspecified, we will also get the number of the
 *  degrees of freedom for the problem.
 */

extern int32 get_globally_consistent_eligible(slv_system_t server,
                                              int32 **eliset);
/**<
 *  <!--  usage: get_globally_consistent_eligible(server,&(eliset))     -->
 *  <!--  int32 *eliset                                                 -->
 *
 *  Returns 1 if able to determine list, 0 otherwise.
 *  That is you send us the address of a pointer to an int32 array
 *  and we will fill in the pointer.
 *  If return is 1, user should free eliset when done with it. Return 0
 *  -> eliset will be null.
 *  The index list is terminated with a -1 but may be of any length.
 *  The indices are var_mindex of vars on master_var_list.
 *  elist is incident vars eligible to be fixed.
 */

extern int32 consistency_analysis(slv_system_t server, int32 **fixed);
/**<
 *  <!--  usage: consistency_analysis(server,&(fixed))                  -->
 *  <!--  int32 *fixed                                                  -->
 *
 *  Returns 1 if system is structurally consistent, 0 otherwise.
 *  That is you send us the address of a pointer to an int32 array
 *  and we will fill in the pointer.
 *  The function
 *  performs an automatized combinatorial consitency analysis to find
 *  a partition which causes all the alernatives in the system to be
 *  consitent. If the system is consistent, the array 'fixed' will contain
 *  the solver var indices of a set of  variables which, if fixed, will
 *  result in a consistent partition for all the alternatives in the
 *  system.<br><br>
 *  If return is 1, user should free 'fixed' when done with it. Return 0
 *  -> fixed will be null.
 *  The index list is terminated with a -1 but may be of any length.
 *  The indices are var_mindex of vars on master_var_list.
 */

#endif  /* slvdof_already_included */

