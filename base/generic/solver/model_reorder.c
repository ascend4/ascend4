/*	ASCEND modelling environment
	Copyright(C) 1996 Benjamin Andrew Allan
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
*//** @file
	Model-based Reordering Routines

	These functions are part of a new design for feeding
	solvers from the ASCEND compiler.

	We're starting with a basic RBBD implementation.

	Assumptions:
	  - If the MODELs are hierarchical, then they have been indexed in a
	    tree bottom-up fashion.
	  - Input is a square block that needs tearing and not a rectangle.
*//*
	by Benjamin Andrew Allan, 6/22/96
	Last in CVS: $Revision: 1.11 $ $Date: 1997/07/18 12:14:43 $ $Author: mthomas $
*/

#include "model_reorder.h"

#include <utilities/ascConfig.h>

#include <utilities/ascMalloc.h>
#include <general/list.h>

#include "slv_client.h"

#define MRDEBUG 0
/**< if !=0, generate spew while working */
#define CUTOFFDEFAULT 1000
/**< tuning parameter in need of investigation */

mr_reorder_t *mr_reorder_create(slv_system_t slv, mtx_matrix_t mtx, int32 size)
{
  mr_reorder_t *res;
  if(slv==NULL  || mtx == NULL || size < 1) {
    return NULL;
  }
  res = ASC_NEW_CLEAR(mr_reorder_t);
  if (res == NULL) return res;
  res->slv = slv;
  res->mtx = mtx;
  res->cutoff = CUTOFFDEFAULT;
  res->activelen = size;
  res->local = ASC_NEW_ARRAY_CLEAR(int32,2*size);
  res->tmpmodlist = res->local+size; /* 2nd half of allocation is tmpmodlist */
  res->active = ASC_NEW_ARRAY_CLEAR(char,size);
  if (res->local == NULL || res->active == NULL) {
    if(res->local !=NULL) ascfree(res->local);
    if(res->active !=NULL) ascfree(res->active);
    ascfree(res);
    res = NULL;
  }
  return res;
}

void mr_reorder_destroy(mr_reorder_t *sys)
{
  if (sys==NULL) return;
  sys->slv=NULL;
  sys->mtx=NULL;
  ascfree(sys->local); /*tmpmodlist freed too, hereby */
  ascfree(sys->active);
  ascfree(sys);
}

/* comparing distinct integers with qsort this yields an increasing
   integer list  (or else integer overflow) */
static int intcompare(int *i, int *j)
{
  return(*i - *j);
}

int mr_bisect_partition(mr_reorder_t *sys, mtx_region_t *reg,
                        int top, MRBlockReorderF rfunc)
{
#if MRDEBUG
  FILE *fptr;
  static mtx_region_t graph_reg;
  static int fnum;
  static char fname[80];
#endif
  mtx_block_t mb;
  mtx_block_t *mbptr;
  mtx_region_t *block;
  struct rel_relation **rp;
  mtx_coord_t coord;
  int partition, newpart,size,threshold, status = 0;
  int32 k,row,mod=0,rmax,stop,nrows;

  /* check all our pointers */

  if (sys==NULL || sys->slv == NULL || sys->mtx == NULL ||
      sys->local == NULL || sys->active == NULL ||
      sys->tmpmodlist == NULL || sys->activelen == 0 ||
      sys->cutoff < 10 || rfunc == NULL
     ) {
#if MRDEBUG
    FPRINTF(stderr,"mr_bisect_partition miscalled.\n");
#endif
    return 1;
  }

  rp = slv_get_solvers_rel_list(sys->slv);

  /* CREATE block list as needed */
  if (top==1) {
    mbptr = &mb;
    mb.nblocks = 1;
    mb.block = reg;
#if MRDEBUG
    graph_reg = *reg;
    fnum=0;
#endif
  } else {
    mbptr = mtx_block_partition(sys->mtx,reg);
    if (mbptr == NULL || (mbptr->block == NULL && mbptr->nblocks) ) {
      if (mbptr != NULL) ascfree(mbptr);
      return 1;
    }
    sys->nblts++;
  }

#if MRDEBUG
  FPRINTF(stderr,"mtx_bisect_partition2 (%d,%d - %d,%d).\n",
    reg->row.low,reg->col.low,reg->row.high,reg->col.high);
  sprintf(fname,"/tmp/bisect1.plot.%d",fnum);
  fptr = fopen(fname,"w+");
  mtx_write_region_plot(fptr,sys->mtx,&graph_reg);
  fclose(fptr);
  fnum++;
#endif

  /* tear (or skip) the k remaining partitions */
  for (k=0; k < mbptr->nblocks; k++) {
    block = &(mbptr->block[k]);
    size = block->row.high - block->row.low +1;

    /* skip 1x1, 2x2 blocks which must be full */
    if (block->row.high - block->row.low < 3) {
      continue; /* next k */
    }

    /* blocks below cutoff the user reorders instead */
    if (size < sys->cutoff) {
#if MRDEBUG
      FPRINTF(stderr,"Calling user reorder on rows %d - %d.\n",
        block->row.low,block->row.high);
#endif
      rfunc(sys->slv,sys->mtx,block);
      continue; /* next k */
    }

    /* ok, here's where we do the work */
    threshold = size/2;		/* we want to divide in approximately half */
    rmax = block->row.high;

    /* tag in block models and count in block relations */
    for (row = block->row.low; row <= rmax; row++) {
      mod = rel_model(rp[mtx_row_to_org(sys->mtx,row)]);
      if (!(sys->active[mod])) {
        sys->active[mod] = 1;
        sys->tmpmodlist[sys->tmpmodused++] = mod;
      }
      sys->local[mod]++;
    }

    /* if block is all one MODEL, tell user to eat it */
    if (sys->tmpmodused < 2) {
#if MRDEBUG
      FPRINTF(stderr,"Found oversized MODEL including relation\n");
      rel_write_name(sys->slv,rp[mtx_row_to_org(sys->mtx,rmax)],stderr);
      FPRINTF(stderr,"\nCalling user reorder on rows %d - %d.\n",
        block->row.low,block->row.high);
#endif
      /* rezero active and local arrays, just 1! */
      sys->local[0] = 0;
      sys->active[0] = 0;
      rfunc(sys->slv,sys->mtx,block);
      continue; /* next k */
    }

    /* sort the MODEL indices in tmpmodlist. using this fact later
     * enables us to punt the rel_partition flag in favor of just
     * checking MODEL index of a row against the last MODEL in the
     * first partition when hunting in incidence for complicating cols.
     * We might want to reverse things if the tree was numbered top
     * down rather than bottom up.
     * It would be nice if we could prove this sort is not needed,
     * but I haven't had the time yet to try.
     * A slightly different, probably bigger, data structure
     * might also allow the sort to be avoided.
     * Probably we should create a doubly linked list version
     * of tmpmodlist and insert sorted as it is created,
     * then map the final thing down to the vector needed.
     * Who knows? It's a small list in any case.
     */
    qsort(sys->tmpmodlist,sys->tmpmodused,sizeof(int),
          (int (*)(const void *, const void *))intcompare);

    /* now try to split tmpmodlist approximately in relation count half */
    stop = 0;
    nrows = 0;
    /* local[mod] should be the size of the last MODEL in the first half
     * on loop exit.
     */
    while(stop < sys->tmpmodused && nrows < threshold) {
      /* stop at the first MODEL division after the halfway point */
      mod = sys->tmpmodlist[stop++];
      nrows += sys->local[mod];
    }
    /* check if the division BEFORE the halfway point is closer to center
     * but not at beginning.
     * This should help with large, centered blocks that OTHERWISE
     * would put us way past half and result in overtearing.
     *  |------d1---------|t|---------------------d2------------|
     * d1: threshold-(nrows-local[mod])    d2: nrows-threshold
     * the abs should not be needed, but to be safe.
     */
    if ( stop > 1 &&
         abs(nrows - threshold) > abs(threshold - (nrows - sys->local[mod])) ) {
      nrows -= sys->local[mod];
      stop--;
      mod = sys->tmpmodlist[stop-1];
    }

    /* now all relations with MODEL number > mod are in the
       second partition with MODEL number <= mod are in the first. */

    /* now we're going to identify and permute out the tears,
       diddling with the block. rmax preserves block->row.high */
    coord.col = block->row.low;
    while (coord.col <=  block->row.high) {
      coord.row = mtx_FIRST;
      partition = -1; /* neither partition seen */
      while ( mtx_next_in_col(sys->mtx,&coord,&(block->row)),
              coord.row != mtx_LAST) {

        /* check if this column crossed partitions */
        newpart = (rel_model(rp[mtx_row_to_org(sys->mtx,coord.row)]) > mod);
        if (partition >= 0 && partition != newpart) {
          /* this is a tear. */
          sys->ntears++;
          /* symmetrically permute tear to block outer edge */
          mtx_swap_cols(sys->mtx,coord.col,block->row.high);
          mtx_swap_rows(sys->mtx,coord.col,block->row.high);
          rel_set_torn(rp[mtx_row_to_org(sys->mtx,block->row.high)],1);
          /* reduce the block row range, break next_in loop.
           * with any luck, this stops us from double tearing, though
           * in a completely arbitrary way. Another covert tie-breaking.
           */
          block->row.high--;
          break; /* next coord.col. */
        }
        /* it doesn't cross a partition yet, keep looking */
        partition = newpart; /* slightly redundant. cheaper than checking */
      }
      coord.col++;
    }
    block->col.high = block->row.high;

    /* rezero active and local arrays so as not to confuse recursion */
    while (sys->tmpmodused > 0) {
      mod = sys->tmpmodlist[--(sys->tmpmodused)];
      sys->local[mod] = 0;
      sys->active[mod] = 0;
    }
    /* there are no flags on rels to rezero */

    /* attack block left after tearing. */
    status += mr_bisect_partition(sys,block,0,rfunc);
  }

  /* if block list is locally created, destroy it */
  if (!top) {
    mtx_destroy_blocklist(mbptr);
  }
  return status;
}


int mr_bisect_partition2(mr_reorder_t *sys, mtx_region_t *reg,
                        int top, MRBlockReorderF rfunc)
{
#if MRDEBUG
  FILE *fptr;
  static mtx_region_t graph_reg;
  static int fnum;
  static char fname[80];
#endif
  mtx_block_t mb;
  mtx_block_t *mbptr;
  mtx_region_t *block;
  struct rel_relation **rp;
  mtx_coord_t coord;
  int partition, newpart,size,threshold, status = 0;
  int32 k,row,mod=0,rmax,stop,nrows,nexttear;

  /* check all our pointers */

  if (sys==NULL || sys->slv == NULL || sys->mtx == NULL ||
      sys->local == NULL || sys->active == NULL ||
      sys->tmpmodlist == NULL || sys->activelen == 0 ||
      sys->cutoff < 10 || rfunc == NULL
     ) {
#if MRDEBUG
    FPRINTF(stderr,"mr_bisect_partition2 miscalled.\n");
#endif
    return 1;
  }

  rp = slv_get_solvers_rel_list(sys->slv);

  /* CREATE block list as needed */
  if (top==1) {
    mbptr = &mb;
    mb.nblocks = 1;
    mb.block = reg;
#if MRDEBUG
    graph_reg = *reg;
    fnum=0;
#endif
  } else {
#if MRDEBUG
    FPRINTF(stderr,"mtx_bisect_partition2 (%d,%d - %d,%d).\n",
      reg->row.low,reg->col.low,reg->row.high,reg->col.high);
#endif
    mbptr = mtx_block_partition(sys->mtx,reg);
    if (mbptr == NULL || (mbptr->block == NULL && mbptr->nblocks) ) {
      if (mbptr != NULL) ascfree(mbptr);
      return 1;
    }

#if MRDEBUG
    FPRINTF(stderr,"BLT List:\n");
    for (k=0;k < mbptr->nblocks; k++) {
      FPRINTF(stderr,"B: %d,%d - %d,%d\n",
        mbptr->block[k].row.low,mbptr->block[k].col.low,
        mbptr->block[k].row.high,mbptr->block[k].col.high);
    }
#endif
    sys->nblts++;
  }

#if MRDEBUG
  FPRINTF(stderr,"mtx_bisect_partition2 (%d,%d - %d,%d).\n",
    reg->row.low,reg->col.low,reg->row.high,reg->col.high);
  sprintf(fname,"/tmp/bisect2.plot.%d",fnum);
  fptr = fopen(fname,"w+");
  mtx_write_region_plot(fptr,sys->mtx,&graph_reg);
  fclose(fptr);
  fnum++;
#endif

  /* tear (or skip) the k remaining partitions */
  for (k=0; k < mbptr->nblocks; k++) {
    block = &(mbptr->block[k]);
    size = block->row.high - block->row.low +1;

    /* skip 1x1, 2x2 blocks which must be full */
    if (block->row.high - block->row.low < 3) {
      continue; /* next k */
    }

    /* blocks below cutoff the user reorders instead */
    if (size < sys->cutoff) {
#if MRDEBUG
      FPRINTF(stderr,"Calling user reorder on rows %d - %d.\n",
        block->row.low,block->row.high);
#endif
      rfunc(sys->slv,sys->mtx,block);
      continue; /* next k */
    }

    /* ok, here's where we do the work */
    threshold = size/2;		/* we want to divide in approximately half */
    rmax = block->row.high;

#if MRDEBUG
      FPRINTF(stderr,"Splitting block with threshold %d.\n", threshold);
#endif

    /* tag in block models and count in block relations */
    for (row = block->row.low; row <= rmax; row++) {
      mod = rel_model(rp[mtx_row_to_org(sys->mtx,row)]);
      if (!(sys->active[mod])) {
        sys->active[mod] = 1;
        sys->tmpmodlist[sys->tmpmodused++] = mod;
      }
      sys->local[mod]++;
    }

    /* if block is all one MODEL, tell user to eat it */
    if (sys->tmpmodused < 2) {
#if MRDEBUG
      FPRINTF(stderr,"Found oversized MODEL including relation\n");
      rel_write_name(sys->slv,rp[mtx_row_to_org(sys->mtx,rmax)],stderr);
      FPRINTF(stderr,"\nCalling user reorder on rows %d - %d.\n",
        block->row.low,block->row.high);
#endif
      /* rezero active and local arrays, just 1! */
      sys->local[0] = 0;
      sys->active[0] = 0;
      rfunc(sys->slv,sys->mtx,block);
      continue; /* next k */
    }

#if MRDEBUG
    FPRINTF(stderr,"Number of models in block %d\n",sys->tmpmodused);
    FPRINTF(stderr,"tearing block %d-%d\n",block->row.low,block->row.high);
#endif
    /* sort the MODEL indices in tmpmodlist. using this fact later
     * enables us to punt the rel_partition flag in favor of just
     * checking MODEL index of a row against the last MODEL in the
     * first partition.
     * We might want to reverse things if the tree was numbered top
     * down rather than bottom up.
     */
    qsort(sys->tmpmodlist,sys->tmpmodused,sizeof(int),
          (int (*)(const void *, const void *))intcompare);

    /* now try to split tmpmodlist approximately in relation count half */
    stop = 0;
    nrows = 0;
    while(stop < sys->tmpmodused && nrows < threshold) {
      mod = sys->tmpmodlist[stop++];
      nrows += sys->local[mod];
    }
    /* Now all relations with MODEL number > mod are in the
     * second partition with MODEL number <= mod are in the first.
     */
#if MRDEBUG
    FPRINTF(stderr,"Number of rows in first half %d\n",nrows);
    FPRINTF(stderr,"Number of models in first half %d\n",stop);
#endif

    /* now we're going to identify and permute out the tears,
     * diddling with the block. rmax preserves block->row.high
     */
    coord.col = block->row.low;
    nexttear = block->row.high;
    while (coord.col <=  nexttear) {
      coord.row = mtx_FIRST;
      partition = -1; /* neither partition seen */
      while ( mtx_next_in_col(sys->mtx,&coord,&(block->row)),
              coord.row != mtx_LAST) {

        /* check if this column crossed partitions */
        newpart = (rel_model(rp[mtx_row_to_org(sys->mtx,coord.row)]) > mod);
#if MRDEBUG
        if (newpart <0) {
          FPRINTF(stderr,"unexpect newpart < 0 (%d)\n",coord.row);
        }
#endif
        if (partition >= 0 && partition != newpart) {
          /* this is a tear. */
          sys->ntears++;
          /* symmetrically permute tear to block outer edge */
          mtx_swap_cols(sys->mtx,coord.col,nexttear);
          mtx_swap_rows(sys->mtx,coord.col,nexttear);
          rel_set_torn(rp[mtx_row_to_org(sys->mtx,nexttear)],1);
          /* reduce the block row range, break next_in loop.
           * with any luck, this stops us from double tearing, though
           * in a completely arbitrary way. Another covert tie-breaking.
           */
#if MRDEBUG
          FPRINTF(stderr,"tear row %d\n",nexttear);
#endif
          nexttear--;
          break; /* next coord.col. */
        }
        /* it doesn't cross a partition yet, keep looking */
        partition = newpart; /* slightly redundant. cheaper than checking */
      }
      coord.col++;
    }
#if MRDEBUG
    FPRINTF(stderr,"TORE %d rows\n", block->col.high-nexttear);
#endif
    block->col.high = block->row.high = nexttear;

    /* rezero active and local arrays so as not to confuse recursion */
    while (sys->tmpmodused > 0) {
      mod = sys->tmpmodlist[--(sys->tmpmodused)];
      sys->local[mod] = 0;
      sys->active[mod] = 0;
    }
    /* there are no flags on rels to rezero */

    /* attack block left after tearing. */
    status += mr_bisect_partition2(sys,block,0,rfunc);
  }

  /* if block list is locally created, destroy it */
  if (!top) {
    mtx_destroy_blocklist(mbptr);
  }
  return status;
}
