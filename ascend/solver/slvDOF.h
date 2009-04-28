/*	ASCEND modelling environment
	Copyright (C) 1994 Benjamin Andrew Allan
	Copyright (C) 1998 Vicente Rico-Ramirez
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	ASCEND Degrees of Freedom module

	This file contains routines for managing degrees
	of freedom through the user interface. It uses
	its own incidence matrix separate from the mtx of
	any given solver.
*//*
	by Benjamin A Allan and Vicente Rico-Ramirez
	Created: 7/11/94
	Last in CVS: $Revision: 1.10 $ $Date: 1998/03/30 22:07:03 $ $Author: rv2a $

	Dates:        07/94 - original version
	              06/96 - reimplementation
	              03/98 - Added the consistency analysis for conditional
	                      models
*/

#ifndef ASC_SLVDOF_H
#define ASC_SLVDOF_H

#include <system/slv_client.h>

/**	@addtogroup solver Solver
	@{
*/

#include <utilities/ascConfig.h>

/*
	The following functions are for use in running DOF analysis and dialog
	through the interface. The tcl callbacks are coded in DebugProc.c.

	Partitioning is assumed to be in effect.

	In general, the lists returned will be longer than number of degrees of
	freedom which can be changed. DOF dialogs should be done 1 var/rel at a
	time.
*/

ASC_DLLSPEC int slvDOF_eligible(slv_system_t server, int32 **vil);
/**<
	Calculate a list of incident variables elegible to be fixed

	This routine does not sort out which vars are in a particular part of
	the MODEL represented by the system -- that is a UI job.

	Vars that do not appear in equations will not be listed as eligible
	since they cannot help the DOF state of the system.

	If return is 1, user owns the returned list vil and must free it. If
	return is zero, vil will be NULL.

	@param vil pointer to an int32 array which will fill in and return. The
	indices are var_sindex values for vars from the solvers_var_list. The index
	list is terminated with a -1 but may be of any length.

	@return 1 if able to determine lists, 0 otherwise.
*/

ASC_DLLSPEC int slvDOF_structsing(slv_system_t server
		,int32 relindex
		,int32 **vil
		,int32 **ril
		,int32 **fil
);
/**<
	Analyse slv system for structural singularity and return. If relinst is set to mtx_FIRST, routine returns
	the intersection of all singularity lists for any unassignable included equations in the system.

	@param relindex sindex of an unassigned,included equation from solvers_rel_list, or else value mtx_FIRST.
	@param vil  list of vars involved in a structural singularity, (-1) terminated.
	@param ril  list of rels involved in a structural singularity, (-1) terminated.
	@param fil  list of vars which reduce the structural singularity if freed, (-1) terminated.

	@return 0 on success, 1 otherwise (in which case vil,ril,fil will not have been allocated)

    -- old comments --
 *  Returns 1 if able to determine lists, 0 otherwise.
 *  relindex should be the sindex of an unassigned, included equation
 *  from the solvers_rel_list.
 *  If relindex is mtx_FIRST, the list returned will be the intersection of
 *  all singularity lists for any unassignable included equations in
 *  the system.
 *  The indices are *_sindex of vars/rels on solvers_*_list.
 */

ASC_DLLSPEC int32 slvDOF_status(slv_system_t server, int32 *status, int32 *dof);
/**<
	Return the structural status of the current system into 'status':
	   - 1 : underspecified
	   - 2 : square
	   - 3 : structurally singular
	   - 4 : overspecifed
	   - 5 : error (with console output)

	If the system is underspecified, we will also get the number of the
	degrees of freedom for the problem.

	@return 1 on success, 0 on error (error output to console)

	@param status pointer to variable where the singularity status is returned.
	@param dof degrees of freedom for the current conditional model 
	configuration (in case where status is underspecified)
*/

ASC_DLLSPEC int32 get_globally_consistent_eligible(slv_system_t server,
                                              int32 **eliset);
/**<
 *  Returns 1 if able to determine list, 0 otherwise.
 *  That is you send us the address of a pointer to an int32 array
 *  and we will fill in the pointer.
 *  If return is 1, user should free eliset when done with it. Return 0
 *  -> eliset will be null.
 *  The index list is terminated with a -1 but may be of any length.
 *  The indices are var_mindex of vars on master_var_list.
 *  elist is incident vars eligible to be fixed.
 */

ASC_DLLSPEC int32 consistency_analysis(slv_system_t server, int32 **fixed);
/**<
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

/* @} */

#endif  /* ASC_SLVDOF_H */
