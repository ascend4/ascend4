/*
 *  Model-based Reordering Routines
 *  by Benjamin Andrew Allan
 *  6/22/96
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: model_reorder.h,v $
 *  Date last modified: $Date: 1997/07/18 12:14:44 $
 *  Last modified by: $Author: mthomas $
 *  Copyright(C) 1996 Benjamin Andrew Allan
 *
 *  This file is part of the ASCEND IV math programming system.
 *
 *  The ASCEND IV math programming system is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND IV math programming system is distributed in hope that it will be
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
 *  Model-based Reordering Routines.
 *
 *  These functions are part of a new design for feeding
 *  solvers from the ASCEND compiler.
 *
 *  File to play MODEL-relation based reordering games.
 *  We're starting with a basic RBBD implementation.<br><br>
 *
 *  Assumptions:
 *  - If the MODELs are hierarchical, then they have been indexed in a
 *    tree bottom-up fashion.
 *  - Input is a square block that needs tearing and not a rectangle.
 *  <pre>
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "slv_types.h"
 *                #include "mtx.h"
 *  </pre>
 */

#ifndef _model_reorder_h_seen_
#define _model_reorder_h_seen_

#define MRDEBUG 0
/**< if !=0, generate spew while working */
#define CUTOFFDEFAULT 1000
/**< tuning parameter in need of investigation */

typedef struct mr_bisection_structure {
  /* data spaces */
  slv_system_t slv;
  mtx_matrix_t mtx;
  int32 *local;       /**< MODEL indexed array of counts of relations */
  char *active;       /**< in block flags for models */
  int32 *tmpmodlist;  /**< a unique list of indices of models found so far */
  int32 tmpmodused;   /**< currently used length of tmpmodlist */
  int32 activelen;    /**< length of active,local and tmpmodlist arrays */

  /* control parameters */
  int32 cutoff;
  int32 maxtears;
  double maxtearfraction;

  /* miscellaneous statistics */
  int32 ntears;
  int32 nblts;
} mr_reorder_t;
/**<
 *  The mr_reorder_t is a workspace structure containing most of the items
 *  needed for the recursive function mr_bisect_partition and any similar
 *  functions. mr_reorder_create and destroy are provided to set up
 *  a mr_reorder_t.
 *
 *    - The mtx herein should correspond to the solvers_rel/var lists of the
 *      slv_system_t here.
 *    - The pointers local, tmpmodlist, and active should contain arrays of
 *      size activelen and these arrays should be (re)initialized to 0
 *      for each call of mr_bisect_partition.
 *    - Activelen should be the number of models in the slv_system_t. In
 *      particular array[rel_model(rel)] should exist for all rel in the
 *      solvers_rel_list.
 *    - Cutoff is the smallest block the user wants us to try to tear.
 *    - Maxtears is the largest number of tears user wants to allow in any
 *      single block. IT IS IGNORED.
 *    - Maxtearfraction is the largest percentage of any single block the
 *      user wants us to try to tear. IT IS IGNORED.
 *    - Ntears is the number of tear columns we find.
 *    - Nblts is the number of BLT permutations we do.
 */

extern mr_reorder_t *mr_reorder_create(slv_system_t slvsys, 
                                       mtx_matrix_t mtx, 
                                       int32 nmodels);
/**<
 *  <!--  mrsys = mr_reorder_create(slvsys,mtx,nmodels);               -->
 *  Returns a mr_reorder_t all set up with cutoff set to CUTOFFDEFAULT
 *  for a problem that has nmodels models in it.
 *  The arrays are initialized to 0.
 *  rel_model(rel) of any rel in slvsys should return a number 0..nmodels-1.
 *  Returns NULL if insufficient memory.
 */

extern void mr_reorder_destroy(mr_reorder_t *mrsys);
/**<
 *  <!--  mr_reorder_destroy(mrsys);                                   -->
 *  Deallocate a mr_reorder_t *.
 */

typedef int (MRBlockReorderF)(slv_system_t,mtx_matrix_t,mtx_region_t *);
/**<
 *  This is a user supplied function that will be applied called
 *  with sys->slv,sys->mtx, and regions that fall below sys->cutoff
 *  in size.  The user may do any sort of logic they care to,
 *  including ignoring reordering blocks below a certain size. It
 *  must be supplied, even if it does nothing.<br><br>
 *
 *  This function could be dumb and just apply SPK1-like things, or
 *  be very smart and reorder based on the number and type of models
 *  found in the region. This function should be the subject of some
 *  experimentation since mr_bisect_partition just identifies tears
 *  and subregions.<br><br>
 *
 *  This function is also the one called on blocks bigger than the
 *  cutoff size that however contain equations from exactly 1 MODEL.<br><br>
 *
 *  We don't (yet, anyway) check the return value.
 */

extern int mr_bisect_partition(mr_reorder_t *sys,
                               mtx_region_t *reg, 
                               int top,
                               MRBlockReorderF rfunc);
/**<
 *  <!--  stat = mr_bisect_partition(sys, reg, top, rfunc);            -->
 *  <!--  mr_reorder_t *sys;                                           -->
 *  <!--  mtx_region_t *reg;                                           -->
 *  <!--  int top;                                                     -->
 *  <!--  MRBlockReorderF rfunc;                                       -->
 *
 *  This function is recursive and produces a sys->mtx reordered to be
 *  recursive block bordered diagonal (RBBD).
 *
 *  If top is 1, the region given in block should be (within itself)
 *  a strongly connected square block. If top is 0, we will do a BLT
 *  permutation on the region first assuming the region is square
 *  and has a full diagonal. The normal mode of externally calling
 *  this function should be with top==1 and we then recurse with
 *  top==0.<br><br>
 *
 *  One the outermost call, the relations in reg should have all
 *  their REL_TORN flag bits set to 0.  On return from the outermost
 *  call, the columns identified as tears will have the REL_TORN
 *  flag bits set to 1. It turns out we don't use the REL_PARTITION
 *  flag.<br><br>
 *
 *  The mr_reorder_t * given must be filled in completely before
 *  calling this function. Anything amiss will cause us to return
 *  immediately without doing anything.  Returns 0 if ok, > 0 if
 *  anything else wrong. Basically, the only thing that can go
 *  wrong is, if during one of our blt permutations, we run out
 *  of memory. Nonzero return value will be the number of times
 *  we encountered malloc failure.<br><br>
 *
 *  The values in reg may be messed with, so if the data in
 *  reg is needed after this function call, keep it elsewhere.
 */

extern int mr_bisect_partition2(mr_reorder_t *sys,
                                mtx_region_t *reg,
                                int top,
                                MRBlockReorderF rfunc);
/**<
 *  <!--  stat = mr_bisect_partition2(sys, reg, top, rfunc);           -->
 *  <!--  mr_reorder_t *sys;                                           -->
 *  <!--  mtx_region_t *reg;                                           -->
 *  <!--  int top;                                                     -->
 *  <!--  MRBlockReorderF rfunc;                                       -->
 *
 *  This is the function most similar to the algorithm described/tested
 *  in the Abbott thesis.
 *
 *  This function is recursive and produces a sys->mtx reordered to be
 *  recursive block bordered diagonal (RBBD) with borders roughly twice
 *  the size of those in mr_bisect_partition because of a simple idiocy
 *  in the way it locates tears. The relative performance of the two
 *  versions is not clear.<br><br>
 *
 *  If top is 1, the region given in block should be (within itself)
 *  a strongly connected square block. If top is 0, we will do a BLT
 *  permutation on the region first assuming the region is square
 *  and has a full diagonal. The normal mode of externally calling
 *  this function should be with top==1 and we then recurse with
 *  top==0.<br><br>
 *
 *  One the outermost call, the relations in reg should have all
 *  their REL_TORN flag bits set to 0.  On return from the outermost
 *  call, the columns identified as tears will have the REL_TORN
 *  flag bits set to 1. It turns out we don't use the REL_PARTITION
 *  flag.<br><br>
 *
 *  The mr_reorder_t * given must be filled in completely before
 *  calling this function. Anything amiss will cause us to return
 *  immediately without doing anything.  Returns 0 if ok, > 0 if
 *  anything else wrong. Basically, the only thing that can go
 *  wrong is, if during one of our blt permutations, we run out
 *  of memory. Nonzero return value will be the number of times
 *  we encountered malloc failure.<br><br>
 *
 *  The values in reg may be messed with, so if the data in
 *  reg is needed after this function call, keep it elsewhere.
 */

#endif /* _model_reorder_h_seen_ */

