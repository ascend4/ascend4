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
	'Standard clients' module. This appears to include stuff relating to block
	partitioning, bounds checking, registration of solver engines.

	Block partitioning gets done by a selected solver engine, at its
	discretion. See functions 'structural_analysis' in slv3 and others.

	Requires:
	#include <stdio.h>
	#include "utilities/ascConfig.h"
	#include "system.h"
	#include "slv_client.h"
	#include "mtx.h"
	#include "slvDOF.h"

	@NOTE 'BLT' = block lower triangular, 'BUT' = block upper triangular.
*//*
	by Benjamin Andrew Allan
	5/19/96
	Last in CVS: $Revision: 1.8 $ $Date: 1998/06/16 16:53:07 $ $Author: mthomas $
*/

#ifndef ASC_SLV_STDCLIENTS_H
#define ASC_SLV_STDCLIENTS_H

/**	@addtogroup solver Solver
	@{
*/


extern int slv_std_make_incidence_mtx(slv_system_t sys, 
		mtx_matrix_t mtx, var_filter_t *vf, rel_filter_t *rf);
/**<
	Populates a matrix according to the sys solvers_vars, solvers_rels
	lists and the filters given. The filter should have at least
	SVAR = 1 bit on. mtx given must be created (not null) and with
	order >= max( slv_get_num_solvers_rels(sys),slv_get_num_solvers_vars(sys));
	Vars and rels on solvers lists should be sindexed 0 -> len-1.

	@return 0 on success
*/

/*------------------------------------------------------------------------------
  BLOCK PARTITIONING
*/

#define slv_block_partition(s) slv_block_partition_real((s),0)
/**< do block lower triangular permutation to system solver's lists */

#define slv_block_partition_upper(s) slv_block_partition_real((s),1)
/**< do block upper triangular permutation to system solver's lists */

extern int slv_block_partition_real(slv_system_t sys, int uppertriangular);
/**<
	Takes a system and reorders its solvers_vars and solvers_rels
	list so that they fall into a block lower(upper) triangular form and
	the system's block list is set to match.
	Only included equality relations and free, incident solver_var
	are so ordered.
	
	The blocks derived are put in the slv_system_t block list.

	Produces whine about DOF analysis on file stderr.

	@NOTE doesn't grok inequalities	

	ASSUMPTION: We assume that the variable and relation bit flags
	are in sync with the flags on the atoms.  The need for this
	assumption and the associated bookkeeping should go away when
	we move to using only the bit flags.  Currently var_fixed and
	rel_included are in charge of the syncronization.

	@return 0 on success, 2 on out-of-memory, 1 on any other failure
*/

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
     
int slv_block_unify(slv_system_t sys);
/**<
 *  Takes a system that has been through slv_block_partition and combines
 *  the blocks that comprise rows 0 -> (structural_rank -1) and
 *  columns 0 to ncols-1 in the system's dof_t into one block.
 *  This may be useful for optimizers which want a whole rectangular
 *  block.
 */

extern int slv_set_up_block(slv_system_t sys, int32 block);
/**<
 *  This function should be called on blocks which have previously
 *  been reordered.  Variable and relation maintanence is performed
 *  so that the block will be ready for an interation.
 *  ***rest of this is cut from slv_spk1_reorder***<br><br>
 *
 *  System should have first been processed by slv_block_partition,
 *  so that all the current rows and columns within the block
 *  correspond to org rows and columns within the block.
 *  solvers_vars and solvers_rels lists are
 *  reordered and reindexed within that region to match.<br><br>
 *
 *  The block should contain only included equality relations and
 *  free, incident solver_vars.
 *  If we made a matrix out of the block, it should have a full<br><br>
 *  diagonal.
 *
	@return 0 on success, 2 on out-of-memory, 1 on any other failure.
 *
 *  Preconditions of use:
 *    No vars outside the block in the solvers_var list should have
 *    VAR_INBLOCK set to TRUE.<br><br>
 *
 *  Sideeffects:
 *  We will diddle with all the vars in the solvers_vars and solver_rels
 *  list so that all rel/vars inside the block respond TRUE to in_block.
 *  We will do this even if we decide to block is too small to be worth
 *  reordering or is the wrong shape.<br><br>
 *
 *  @bug This should optionally take a user supplied matrix so it doesn't
 *       allocate a big matrix header+perms every time. This needs some
 *       thought.
 *
 *  @todo Revisit design of slv_set_up_block() - take user matrix?
 */

extern int slv_spk1_reorder_block(slv_system_t sys,
		int32 block, int32 transpose);
/**<
 *  System should have first been processed by slv_block_partition,
 *  so that all the current rows and columns within the block
 *  correspond to org rows and columns within the block.
 *  Within the diagonal block given (which must be square), an SPK1
 *  reordering is done. solvers_vars and solvers_rels lists are
 *  reordered and reindexed within that region to match.
 *  The block should contain only included equality relations and
 *  free, incident solver_vars.
 *  transpose determines if SPK1 is transposed.
 *  If we made a matrix out of the block, it should have a full
 *  diagonal.
 *
	@return 0 on success, 2 on out-of-memory, 1 on any other failure.
 *
 *  Preconditions of use:
 *    No vars outside the block in the solvers_var list should have
 *    VAR_INBLOCK set to TRUE.
 *
 *  Sideeffects:
 *  We will diddle with all the vars in the solvers_vars and solver_rels
 *  list so that all rel/vars inside the block respond TRUE to in_block.
 *  We will do this even if we decide to block is too small to be worth
 *  reordering or is the wrong shape.
 *
 *  @bug This should optionally take a user supplied matrix so it doesn't
 *       allocate a big matrix header+perms every time. This needs some
 *       thought.
 *
 *  @todo Revisit design of slv_set_up_block() - take user matrix?
 */

extern int slv_tear_drop_reorder_block(slv_system_t sys,
                                       int32 blockindex,
                                       int32 cutoff,
                                       int two,
                                       enum mtx_reorder_method blockmethod);
/**<
 *  System should have first been processed by slv_block_partition,
 *  so that all the current rows and columns within the block
 *  correspond to org rows and columns within the block.
 *  If you made a matrix out of the block, it should have a full
 *  diagonal.<br><br>
 *
 *  Given a system and the index of a block in that system,
 *  applies a variant of the reordering hinted at in [Abbott,1996].
 *  The implementation here is by Ben Allan, so any discrepancies
 *  (positive) in performance with respect to the Abbott
 *  thesis data are Ben's fault.<br><br>
 *
 *  If two != 0, applies an algorithm slightly more similar to that
 *  of Abbott, getting borders roughly twice as large.
 *  Preconditions of use: same as spk1_reorder_block.
 *  Sideeffects and bugs: same as spk1_reorder_block.
 *  Additional bugs: none known, but they're there.

	@return ???
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
	Takes a system and a range of vars (lo,hi) and makes sure all the
	variables are within bounds and bounds are reasonable. 
	Reports errors in all cases where they are not.
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

extern int slv_std_make_log_incidence_mtx(slv_system_t sys, 
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
