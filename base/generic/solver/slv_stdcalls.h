/**< 
 *  Standard Clients module of ASCEND environment
 *  by Benjamin Andrew Allan
 *  5/19/96
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: slv_stdcalls.h,v $
 *  Date last modified: $Date: 1998/06/16 16:53:07 $
 *  Last modified by: $Author: mthomas $
 *
 *  Copyright(C) 1996 Benjamin Andrew Allan
 *  Copyright(C) 1998 Carnegie Mellon University
 *
 *  This file is part of the ASCEND IV math programming system.
 *  It registers our primary clients (solvers) and provides a home
 *  for several unregistered utility clients.
 *
 *  The Ascend Math Programming System is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Math Programming System is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

#ifndef _slv_stdclients_h_seen_
#define _slv_stdclients_h_seen_
/**< requires #include <stdio.h> */
/**< requires #include "system.h" */
/**< requires #include "slv_client.h" */
/**< requires #include "mtx.h" */
/**< requires #include "slvDOF.h" */

extern int slv_std_make_incidence_mtx(slv_system_t, mtx_matrix_t, 
                                      var_filter_t *,rel_filter_t *);
/*********************************************************************\
err = slv_std_make_incidence_mtx(sys,mtx,vf,rf);

Populates a matrix according to the sys solvers_vars, solvers_rels
lists and the filters given. The filter should have at least 
SVAR = 1 bit on. mtx given must be created (not null) and with
order >= max( slv_get_num_solvers_rels(sys),slv_get_num_solvers_vars(sys));
Vars and rels on solvers lists should be sindexed 0 -> len-1.
Returns 0 if went ok.
\*********************************************************************/

#define slv_block_partition(s) slv_block_partition_real((s),0)
/**< do BLT permutation to system solvers lists */

#define slv_block_partition_upper(s) slv_block_partition_real((s),1)
/**< do BUT permutation to system solvers lists */

extern int slv_block_partition_real(slv_system_t,int);
/*********************************************************************\
  status = slv_block_partition(sys);
  Takes a system and reorders its solvers_vars and solvers_rels
  list so that they fall into a block lower(upper) triangular form and
  the system's block list is set to match.
  Only included equality relations and free, incident solver_var
  are so ordered.
  The blocks derived are put in the slv_system_t block list.
  If this succeeds, return value is 0.
  If fail due to insufficient memory, return is 2.
  If fail for any other reason, return is 1.
  Produces whine about DOF analysis on file stderr.

  ASSUMPTION: We assume that the variable and relation bit flags
  are in sync with the flags on the atoms.  The need for this
  assumption and the associated bookkeeping should go away when
  we move to using only the bit flags.  Currently var_fixed and
  rel_included are in charge of the syncronization.
\*********************************************************************/

extern void slv_sort_rels_and_vars(slv_system_t,int32 *,int32 *);
/*********************************************************************\
  slv_sort_rels_and_vars(sys,rel_count,var_count);
  Reindexes systems rel and var lists such that rel list is in order:
  rel_list = |included and active rels|unincluded|inactive|
  and the var list is in the order:
  var_list = |incident free solver vars|unincluded|inactive|
  Upon return rel_count will be the number of included and active rels
  and var_count will be the number of incident free solver vars.
  If either rel_count or var_count is -1 then a memory allocation
  failed and this function bailed out without doing any work.
\*********************************************************************/
     
int slv_block_unify(slv_system_t);
/*********************************************************************\
  Takes a system that has been through slv_block_partition and combines
  the blocks that comprise rows 0 -> (structural_rank -1) and
  columns 0 to ncols-1 in the system's dof_t into one block.
  This may be useful for optimizers which want a whole rectangular
  block.
\*********************************************************************/

extern int slv_set_up_block(slv_system_t,int32);
/*********************************************************************\
  status = slv_set_up_block(sys,block);
  This function should be called on blocks which have previously
  been reordered.  Variable and relation maintanence is performed
  so that the block will be ready for an interation.
  ***rest of this is cut from slv_spk1_reorder***
  System should have first been processed by slv_block_partition,
  so that all the current rows and columns within the block
  correspond to org rows and columns within the block.
  solvers_vars and solvers_rels lists are
  reordered and reindexed within that region to match.
  The block should contain only included equality relations and
  free, incident solver_vars.
  If we made a matrix out of the block, it should have a full
  diagonal.
  If this succeeds, return value is 0.
  If fail due to insufficient memory, return is 2.
  If fail for any other reason, return is 1.

  Preconditions of use:
    No vars outside the block in the solvers_var list should have
    VAR_INBLOCK set to TRUE.
  Sideeffects: 
  We will diddle with all the vars in the solvers_vars and solver_rels
  list so that all rel/vars inside the block respond TRUE to in_block.
  We will do this even if we decide to block is too small to be worth
  reordering or is the wrong shape.
  Bugs:
  This should optionally take a user supplied matrix so it doesn't
  allocate a big matrix header+perms every time. This needs some
  thought.
\*********************************************************************/

     
extern int slv_spk1_reorder_block(slv_system_t, int32, int32);
/*********************************************************************\
  status = slv_spk1_reorder_block(sys,block,transpose);
  System should have first been processed by slv_block_partition,
  so that all the current rows and columns within the block
  correspond to org rows and columns within the block.
  Within the diagonal block given (which must be square), an SPK1
  reordering is done. solvers_vars and solvers_rels lists are
  reordered and reindexed within that region to match.
  The block should contain only included equality relations and
  free, incident solver_vars.
  transpose determines if SPK1 is transposed.
  If we made a matrix out of the block, it should have a full
  diagonal.
  If this succeeds, return value is 0.
  If fail due to insufficient memory, return is 2.
  If fail for any other reason, return is 1.

  Preconditions of use:
    No vars outside the block in the solvers_var list should have
    VAR_INBLOCK set to TRUE.
  Sideeffects: 
  We will diddle with all the vars in the solvers_vars and solver_rels
  list so that all rel/vars inside the block respond TRUE to in_block.
  We will do this even if we decide to block is too small to be worth
  reordering or is the wrong shape.
  Bugs:
  This should optionally take a user supplied matrix so it doesn't
  allocate a big matrix header+perms every time. This needs some
  thought.
\*********************************************************************/

extern int slv_tear_drop_reorder_block(slv_system_t,int32,int32,
                                       int,enum mtx_reorder_method);
/*********************************************************************\
  slv_tear_drop_reorder_block(sys,blockindex,cutoff,two,blockmethod);
  System should have first been processed by slv_block_partition,
  so that all the current rows and columns within the block
  correspond to org rows and columns within the block.
  If you made a matrix out of the block, it should have a full
  diagonal.
  Given a system and the index of a block in that system,
  applies a variant of the reordering hinted at in [Abbott,1996].
  The implementation here is by Ben Allan, so any discrepancies
  (positive) in performance with respect to the Abbott
  thesis data are Ben's fault.
  If two != 0, applies an algorithm slightly more similar to that
  of Abbott, getting borders roughly twice as large.
  Preconditions of use: same as spk1_reorder_block.
  Sideeffects and bugs: same as spk1_reorder_block.
  Additional bugs: none known, but they're there.
\*********************************************************************/

extern int slv_insure_bounds(slv_system_t,int32,int32,FILE *);
/*********************************************************************\
  nchange = slv_insure_bounds(sys,lo,hi,fp);
  Takes a system and a range of vars (lo,hi) and makes sure all the
  variables are within bounds and bounds are reasonable. makes 
  repairs in all cases where they are not. Returns the number
  of repairs made or -1 if something weird found.
  If fp is not NULL, prints notices to the file given.
\*********************************************************************/

extern void slv_check_bounds(const slv_system_t,int32,int32, 
                             FILE *,const char *);
/*********************************************************************\
  slv_check_bounds(sys,lo,hi,fp,label);
  Takes a system and a range of vars (lo,hi) and makes sure all the
  variables are within bounds and bounds are reasonable. Whines
  to fp using the label in all cases where they are not.
  Does not change anything.
  If fp is NULL, does nothing.
\*********************************************************************/

extern int SlvRegisterStandardClients(void);
/*********************************************************************\
  Attempts to register solvers slv0 through (as of 6/96) slv7.
  Returns the number of these which register successfully.
  The solvers registered here are those linked at build time of the
  ascend binary. See slv_client.h for registering dynamically loaded
  solvers.
\*********************************************************************/


/*********************************************************************\
          Output Assignment and partitiong in Logical Relations    
\*********************************************************************/

extern int slv_std_make_log_incidence_mtx(slv_system_t, mtx_matrix_t, 
                                          dis_filter_t *,logrel_filter_t *);
/*********************************************************************\
err = slv_std_make_log_incidence_mtx(sys,mtx,dvf,lrf);

Populates a matrix according to the sys solvers_dvars, solvers_logrels
lists and the filters given. mtx given must be created (not null) and with
order >= max( slv_get_num_solvers_logrels(sys),
              slv_get_num_solvers_dvars(sys));
Dvars and logrels on solvers lists should be sindexed 0 -> len-1.
Returns 0 if went ok.
\*********************************************************************/

extern int slv_log_block_partition(slv_system_t);
/*********************************************************************\
  status = slv_log_block_partition(sys);
  Takes a system and reorders its solvers_dvars and solvers_logrels
  list so that they fall into a block lower triangular form and
  the system's block list is set to match.
  Only included and active logrelations and free, incident boolean_var
  are so ordered.
  The blocks derived are put in the slv_system_t block list.
  If this succeeds, return value is 0.
  If fail due to insufficient memory, return is 2.
  If fail for any other reason, return is 1.
\*********************************************************************/

#endif /**<  _slv_stdclients_h_seen_ */
