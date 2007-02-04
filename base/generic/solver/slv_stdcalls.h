/*	ASCEND modelling environment
	Copyright (C) 1998, 2006 Carnegie Mellon University
	Copyright (C) 1996 Benjamin Andrew Allan

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
*//*
	@file
	'Standard clients' module. This appears to include stuff relating
	matrix generation, bounds checking, registration of solver engines.
*//*
	by Benjamin Andrew Allan
	5/19/96
	Last in CVS: $Revision: 1.8 $ $Date: 1998/06/16 16:53:07 $ $Author: mthomas $
*/

#ifndef ASC_SLV_STDCLIENTS_H
#define ASC_SLV_STDCLIENTS_H

#include <utilities/ascConfig.h>
#include <linear/mtx.h>

#include "slv_client.h"

/**	@addtogroup system System
	@{
*/

/*------------------------------------------------------------------------------
  MATRIX GENERATION AND REORDERING
*/

extern int slv_make_incidence_mtx(slv_system_t sys, 
		mtx_matrix_t mtx, var_filter_t *vf, rel_filter_t *rf);
/**<
	Populates a matrix according to the sys solvers_vars, solvers_rels
	lists and the filters given. The filter should have at least
	SVAR = 1 bit on. mtx given must be created (not null) and with
	order >= max( slv_get_num_solvers_rels(sys),slv_get_num_solvers_vars(sys));
	Vars and rels on solvers lists should be sindexed 0 -> len-1.

	@return 0 on success
*/

/* -- moved block partitioning stuff to block.h -- JP */

extern void slv_sort_rels_and_vars(slv_system_t sys,
		int32 *rel_count, int32 *var_count);
/**<
 *  Reindexes systems rel and var lists such that rel list is in order:
 *  rel_list = |included and active rels|unincluded|inactive|
 *  and the var list is in the order:
 *  var_list = |incident free solver vars|unincluded|inactive|<br><br>
 *
 *  Upon return rel_count will be the number of included and active rels
 *  and var_count will be the number of incident free solver vars.
 *  If either rel_count or var_count is -1 then a memory allocation
 *  failed and this function bailed out without doing any work.
 */

/*------------------------------------------------------------------------------
  BOUNDS CHECKING
*/

extern int slv_ensure_bounds(slv_system_t sys, int32 lo, int32 hi, FILE *fp);
/**<
	Takes a system and a range of vars (lo,hi) and makes sure all the
	variables are within bounds and bounds are reasonable. makes
	repairs in all cases where they are not. 

	If fp is not NULL, prints notices to the file given.

	@return number of repairs made or -1 if something weird found.
*/

extern int slv_check_bounds(const slv_system_t sys, int32 lo, int32 hi,
	const char *label
);
/**<
	Takes a system and a range of vars (lo,hi) from the solver's var list
	and makes sure all the variables are within bounds and bounds are 
	make sense (lower < upper). Reports errors in all cases where they are not.
	Does not change anything.

	You can express lo,hi as negative numbers, which means you count back from
	the last var (-1), second last (-2), etc. So lo=0,hi=-1 means the full range.

	@return 0 if bounds are ok and variable lies in range, non-zero otherwise
*/

/*------------------------------------------------------------------------------
  SOLVER REGISTRATION
*/

ASC_DLLSPEC int SlvRegisterStandardClients(void);
/**<
	Attempts to register solvers slv0 through (as of 6/96) slv7.

	The solvers registered here are those linked at build time of the
	ascend binary. See slv_client.h for registering dynamically loaded
	solvers.

	@return number of solvers registered successfully
*/

/*------------------------------------------------------------------------------
  OUTPUT ASSIGNMENT AND PARTITIONING IN LOGICAL RELATIONS
*/

extern int slv_make_log_incidence_mtx(slv_system_t sys, 
                                          mtx_matrix_t mtx,
                                          dis_filter_t *dvf, 
                                          logrel_filter_t *lrf);
/**<
	Populates a matrix according to the sys solvers_dvars, solvers_logrels
	lists and the filters given. mtx given must be created (not null) and with
	order >= max( slv_get_num_solvers_logrels(sys),
	              slv_get_num_solvers_dvars(sys));
	Dvars and logrels on solvers lists should be sindexed 0 -> len-1.

	@return 0 on success
*/

extern int slv_log_block_partition(slv_system_t sys);
/**<
	Takes a system and reorders its solvers_dvars and solvers_logrels
	list so that they fall into a block lower triangular form and
	the system's block list is set to match.
	Only included and active logrelations and free, incident boolean_var
	are so ordered.
	The blocks derived are put in the slv_system_t block list.<br><br>

	@return 0 on success, 2 on out-of-memory, 1 on any other failure	
*/

/* @} */

#endif  /** ASC_SLV_STDCLIENTS_H */
