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
	Block partitioning stuff

	Block partitioning gets done by a selected solver engine, at its
	discretion. See functions 'structural_analysis' in slv3 and others.

	@NOTE 'BLT' = block lower triangular, 'BUT' = block upper triangular.
*//*
	by Benjamin Andrew Allan
	5/19/96
	Last in CVS: $Revision: 1.8 $ $Date: 1998/06/16 16:53:07 $ $Author: mthomas $
*/

#include "slv_types.h"
#include <stdio.h>
#include <linear/mtx_reorder.h>

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
	list so that they fall into a BUT/BLT (block upper/lower triangular) form 
	and	the system's block list is set to match.
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

	@param upppertriangular if 1, partition into BUT form. If 0, partitition into BLT for.
	@return 0 on success, 2 on out-of-memory, 1 on any other failure
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
	This function should be called on blocks which have previously
	been reordered.  Variable and relation maintanence is performed
	so that the block will be ready for an interation.

	The further comments under slv_spk1_reorder_block also apply.
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

extern int block_debug(slv_system_t sys, FILE *fp);
/**<
	Create debug output detailing the current block structure of the system.
*/
	
