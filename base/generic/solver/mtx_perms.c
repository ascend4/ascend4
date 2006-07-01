/*  mtx: Ascend Sparse Matrix Package
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: mtx_perms.c,v $
 *  Date last modified: $Date: 1998/05/06 17:28:51 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Kirk Andre Abbott, Benjamin Andrew Allan
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include "mtx.h"
/* grab our private parts */
#define __MTX_C_SEEN__
#include "mtx_use_only.h"


static mtx_block_perm_t alloc_block_perm()
/**
 ***  Allocates a block_perm header and returns a pointer to it.
 **/
{
   mtx_block_perm_t bp;
   bp = (mtx_block_perm_t)asccalloc(1,sizeof(struct mtx_block_perm_structure));
   bp->integrity = OK;
   bp->data =
     (struct structural_data_t *)ascmalloc(sizeof(struct structural_data_t));
   return(bp);
}
#define free_block_perm(bp) ascfree(bp)

/**
 This is a strange little function which, at the expense of much array logic,
 sets an org_to_cur permutation array based on the data in a cur_to_org.
 Method:
    for i = start..len-1
      o2c[c2o[i]] = i;
 If the majority of an org_to_cur is being copied, it is probably much
 better to copy the whole permutation from the relevant original org_to_cur
 rather than piecewise as is done here.
 WARNING: ABSOLUTELY NOTHING is being done to insure sanity in the function.
 If you can't draw the picture of what this function does by hand and
 compute an example result, don't use it. That said, this saves lots of
 otherwise very annoying time spent in re-reordering.
**/
static void fix_org2cur_from_cur2org(int32 *c2o, int32 start,
                                int32 *o2c, int32 len) {
  int32 i;
  if (ISNULL(c2o) || ISNULL(o2c) || start <0 || len < 1) {
    FPRINTF(g_mtxerr,"ERROR (mtx-internal) fix_org2cur_from_cur2org");
    FPRINTF(g_mtxerr,"                     got bogus args");
    return;
  }
  for (i=start; i < len ; i++) {
    o2c[c2o[i]] = i;
  }
  return;
}

extern mtx_block_perm_t mtx_create_block_perm(mtx_matrix_t mtx)
{
  mtx_block_perm_t bp = NULL;
  int32 bnum;

  if (ISSLAVE(mtx)) {
    return (mtx_create_block_perm(mtx->master));
  }
  if (!mtx_check_matrix(mtx)) return NULL;
  if (mtx->capacity < 2) {
    FPRINTF(g_mtxerr,"ERROR: mtx_create_block_perm:   mtx given is too\n");
    FPRINTF(g_mtxerr,"                                small to permute.\n");
    FPRINTF(g_mtxerr,"                                Returning NULL.\n");
    return NULL;
  }
  if (mtx->data->symbolic_rank < 0) {
    FPRINTF(g_mtxerr,"ERROR: mtx_create_block_perm:   mtx given is not\n");
    FPRINTF(g_mtxerr,"                                output assigned.\n");
    FPRINTF(g_mtxerr,"                                Returning NULL.\n");
    return NULL;
  }

  /* create same size matrix information */
  bp = alloc_block_perm();
  if (ISNULL(bp)) {
    FPRINTF(g_mtxerr,"ERROR: mtx_create_block_perm:   insufficient memory\n");
    FPRINTF(g_mtxerr,"                                allocating bperm (1).\n");
    FPRINTF(g_mtxerr,"                                Returning NULL.\n");
    return NULL;
  }
  bp->order = mtx->order;
  bp->capacity = mtx->capacity;
  bp->mtx = mtx;

  /* copy permutation information */
  bp->perm.row.org_to_cur = mtx_alloc_perm(bp->capacity);
  bp->perm.row.cur_to_org = mtx_alloc_perm(bp->capacity);
  bp->perm.col.org_to_cur = mtx_alloc_perm(bp->capacity);
  bp->perm.col.cur_to_org = mtx_alloc_perm(bp->capacity);
  if (ISNULL(bp->perm.row.org_to_cur) ||
      ISNULL(bp->perm.row.cur_to_org) ||
      ISNULL(bp->perm.col.org_to_cur) ||
      ISNULL(bp->perm.col.cur_to_org) ) {
    FPRINTF(g_mtxerr,"ERROR: mtx_create_block_perm:   insufficient memory\n");
    FPRINTF(g_mtxerr,"                                allocating bperm (2).\n");
    FPRINTF(g_mtxerr,"                                Returning NULL.\n");
    mtx_free_perm(bp->perm.row.org_to_cur);
    mtx_free_perm(bp->perm.row.cur_to_org);
    mtx_free_perm(bp->perm.col.org_to_cur);
    mtx_free_perm(bp->perm.col.cur_to_org);
    free_block_perm(bp);
    return NULL;
  }

  mtx_copy_perm(bp->perm.row.org_to_cur,
                mtx->perm.row.org_to_cur,bp->capacity);
  mtx_copy_perm(bp->perm.row.cur_to_org,
                mtx->perm.row.cur_to_org,bp->capacity);
  bp->perm.row.parity = mtx_row_parity(mtx);

  mtx_copy_perm(bp->perm.col.org_to_cur,
                mtx->perm.col.org_to_cur,bp->capacity);
  mtx_copy_perm(bp->perm.col.cur_to_org,
                mtx->perm.col.cur_to_org,bp->capacity);
  bp->perm.col.parity = mtx_col_parity(mtx);

  bp->data = (struct structural_data_t *)
    ascmalloc(sizeof(struct structural_data_t));
  bp->data->symbolic_rank = mtx->data->symbolic_rank;
  bp->data->nblocks = mtx->data->nblocks;
  bp->data->block = mtx->data->nblocks > 0 ? (mtx_region_t *)
    ascmalloc( mtx->data->nblocks*sizeof(mtx_region_t) ) : NULL;
  for( bnum=0; bnum < mtx->data->nblocks; bnum++ ) {
    bp->data->block[bnum] = mtx->data->block[bnum];
  }
  return bp;
}

int mtx_update_block_perm(mtx_matrix_t mtx, int32 bnum,
                          mtx_block_perm_t bp)
{
  if (ISSLAVE(mtx)) {
    return (mtx_update_block_perm(mtx->master,bnum,bp));
  }
  return 3;
}

int mtx_restore_block_perm(mtx_matrix_t mtx, int32 bnum,
                           mtx_block_perm_t bp)
{
  int32 len;
  if (mtx_check_matrix(mtx) != 1) {
    FPRINTF(g_mtxerr,"ERROR: (mtx) restore_block_perm:  Called with bad\n");
    FPRINTF(g_mtxerr,"                                  mtx.\n");
    return 1;
  }
  if (ISSLAVE(mtx)) {
    return(mtx_restore_block_perm(mtx,bnum,bp));
  }
  if (ISNULL(bp) || bp->integrity != OK) {
    FPRINTF(g_mtxerr,"ERROR: (mtx) restore_block_perm:  Called with bad\n");
    FPRINTF(g_mtxerr,"                                  bperm.\n");
    return 2;
  }
  if (bp->mtx != mtx) {
    FPRINTF(g_mtxerr,"ERROR: (mtx) restore_block_perm:  Mismatched mtx\n");
    FPRINTF(g_mtxerr,"                                  and bperm given.\n");
    return 3;
  }
  if (bp->order != mtx->order) {
    FPRINTF(g_mtxerr,"ERROR: (mtx) restore_block_perm:  Mismatched order mtx\n");
    FPRINTF(g_mtxerr,"                                  and bperm given.\n");
    return 3;
  }
  if (bnum != mtx_ALL_BLOCKS && bp->data->nblocks != 0) {
    /* check block sanity */
    if (mtx->data->symbolic_rank < 0) {
      FPRINTF(g_mtxerr,"ERROR: mtx_restore_block_perm:   mtx given is not\n");
      FPRINTF(g_mtxerr,"                                 output assigned.\n");
      FPRINTF(g_mtxerr,"                                 Returning NULL.\n");
      return 3;
    }
    if ( bp->data->nblocks != mtx->data->nblocks ||
         bp->data->block[bnum].row.low != mtx->data->block[bnum].row.low ||
         bp->data->block[bnum].col.low != mtx->data->block[bnum].col.low ||
         bp->data->block[bnum].row.high != mtx->data->block[bnum].row.high ||
         bp->data->block[bnum].col.high != mtx->data->block[bnum].col.high ) {
      FPRINTF(g_mtxerr,
        "ERROR: (mtx) restore_block_perm:  Mismatched block mtx\n");
      FPRINTF(g_mtxerr,
        "                                  and bperm given.\n");
      return 3;
    }
    /* now copy block bnumth info accordingly, resetting parity */
    /* remember that copy_perm is for some weird reason cp(targ,src,len) */

    len =  bp->data->block[bnum].row.high - bp->data->block[bnum].row.low +1;

    /* block given is in cur coordinates, so we can copy relevant stretches
       of the cur_to_org permutations sequentially. pointer arithmetic. */
    mtx_copy_perm((mtx->perm.row.cur_to_org) + bp->data->block[bnum].row.low,
                  (bp->perm.row.cur_to_org) + bp->data->block[bnum].row.low,
                  len);
    fix_org2cur_from_cur2org(mtx->perm.row.cur_to_org,
                             bp->data->block[bnum].row.low,
                             mtx->perm.row.org_to_cur, len);
    mtx->perm.row.parity = bp->perm.row.parity;

    len =  bp->data->block[bnum].col.high - bp->data->block[bnum].col.low +1;
    mtx_copy_perm((mtx->perm.col.cur_to_org) + bp->data->block[bnum].col.low,
                  (bp->perm.col.cur_to_org) + bp->data->block[bnum].col.low,
                  len);
    fix_org2cur_from_cur2org(mtx->perm.col.cur_to_org,
                             bp->data->block[bnum].col.low,
                             mtx->perm.col.org_to_cur, len);
    mtx->perm.col.parity = bp->perm.col.parity;

  } else {
    /* copy all perm info */
    mtx_copy_perm(mtx->perm.row.cur_to_org,
                  bp->perm.row.cur_to_org,bp->capacity);
    mtx_copy_perm(mtx->perm.row.org_to_cur,
                  bp->perm.row.org_to_cur,bp->capacity);
    mtx->perm.row.parity = bp->perm.row.parity;

    mtx_copy_perm(mtx->perm.col.org_to_cur,
                  bp->perm.col.org_to_cur,bp->capacity);
    mtx_copy_perm(mtx->perm.col.cur_to_org,
                  bp->perm.col.cur_to_org,bp->capacity);
    mtx->perm.col.parity = bp->perm.col.parity;

    /* copy all oa/block info if any. */
    mtx->data->symbolic_rank = bp->data->symbolic_rank;
    if ( mtx->data->nblocks != bp->data->nblocks) {
      if (NOTNULL(mtx->data->block)) {
        ascfree(mtx->data->block);
      }
      mtx->data->nblocks = bp->data->nblocks;
      mtx->data->block = bp->data->nblocks > 0 ? (mtx_region_t *)
        ascmalloc( mtx->data->nblocks*sizeof(mtx_region_t) ) : NULL;
    }
    for( bnum=0; bnum < bp->data->nblocks; bnum++ ) {
      mtx->data->block[bnum] = bp->data->block[bnum];
    }
  }
#if MTX_DEBUG
  if (super_check_matrix(mtx)) {
    FPRINTF(g_mtxerr,"ERROR: mtx_restore_block_perm:   mtx restored now\n");
    FPRINTF(g_mtxerr,"                                 fails sanity.\n");
    return 3;
  }
#endif
  return 0;
}

extern int mtx_destroy_block_perm(mtx_block_perm_t bp)
{
  if (bp->integrity != OK) {
    if (bp->integrity == DESTROYED) {
      FPRINTF(g_mtxerr,
	"ERROR: mtx_destroy_block_perm:  Called with recently\n");
      FPRINTF(g_mtxerr,"                                destroyed bperm.\n");
    } else {
      FPRINTF(g_mtxerr,
        "ERROR: mtx_destroy_block_perm: Called with apparently\n");
      FPRINTF(g_mtxerr,"                               corrupted bperm.\n");
    }
    return 1;
  }

  mtx_free_perm(bp->perm.row.org_to_cur);
  mtx_free_perm(bp->perm.row.cur_to_org);
  mtx_free_perm(bp->perm.col.org_to_cur);
  mtx_free_perm(bp->perm.col.cur_to_org);

  if( NOTNULL(bp->data->block) )
    ascfree(bp->data->block);
  ascfree(bp->data);

  bp->integrity=DESTROYED;
  free_block_perm(bp);
  return 0;
}


extern size_t mtx_block_perm_size(mtx_block_perm_t bp)
{
  size_t size=0;
  if (ISNULL(bp)) return size;
  size += sizeof(struct mtx_block_perm_structure);
  /* data */
  size += sizeof(struct structural_data_t);
  /* block data */
  if (bp->data->nblocks >0)
    size += sizeof(mtx_region_t)*(size_t)(bp->data->nblocks-1);
  /* permutations */
  size += (size_t)4*(sizeof(int32)*(size_t)bp->capacity);
  return size;
}

/* DO NOT CALL this on the perm of a matrix that is a slave */
static void swap( struct permutation_t *perm, int32 cur1,int32 cur2)
{
   register int32 org1,org2;

   if( cur1 == cur2 ) return;
   org1 = perm->cur_to_org[cur1];
   org2 = perm->cur_to_org[cur2];
   perm->cur_to_org[cur1] = org2;
   perm->cur_to_org[cur2] = org1;
   perm->org_to_cur[org1] = cur2;
   perm->org_to_cur[org2] = cur1;
   perm->parity = !(perm->parity);
}

void mtx_swap_rows( mtx_matrix_t mtx, int32 cur1,int32 cur2)
{
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return;
#endif
  }
  swap(&(mtx->perm.row),cur1,cur2);
}

void mtx_swap_cols( mtx_matrix_t mtx, int32 cur1,int32 cur2)
{
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return;
#endif
  }
  swap(&(mtx->perm.col),cur1,cur2);
}

void mtx_drag( mtx_matrix_t mtx, int32 begin, int32 end)
{
  register int32 *rcto, *ccto, *rotc, *cotc;
  int32 holdrow,holdcol;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return;
#endif
  }
  if( begin == end ) return;
  if( abs(begin-end) % 2 ) {
    mtx->perm.row.parity = !(mtx->perm.row.parity);
    mtx->perm.col.parity = !(mtx->perm.col.parity);
  }
  rcto = mtx->perm.row.cur_to_org;
  ccto = mtx->perm.col.cur_to_org;
  rotc = mtx->perm.row.org_to_cur;
  cotc = mtx->perm.col.org_to_cur;
  holdrow = rcto[begin];
  holdcol = ccto[begin];
  while( begin < end ) {
    rotc[rcto[begin]=rcto[begin+1]] = begin;
    cotc[ccto[begin]=ccto[begin+1]] = begin;
    begin++;
  }
  while( begin > end ) {
    rotc[rcto[begin]=rcto[begin-1]] = begin;
    cotc[ccto[begin]=ccto[begin-1]] = begin;
    begin--;
  }
  rotc[rcto[end]=holdrow] = end;
  cotc[ccto[end]=holdcol] = end;
}

void mtx_reverse_diagonal( mtx_matrix_t mtx, int32 begin, int32 end)
{
  register int32 center,j;
  register struct permutation_t *rperm, *cperm;
#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return;
#endif
  }
  if (begin > end) return;
  if (end >= mtx->order) return;
  center=(end-begin+1)/2;
  rperm = &(mtx->perm.row);
  cperm = &(mtx->perm.col);
  for (j=0; j<center; j++) {
    swap(cperm,begin,end);
    swap(rperm,begin,end);
    begin++;
    end--;
  }
}

int32 mtx_row_to_org( mtx_matrix_t mtx, int32 row)
{
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return -1;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return -1;
#endif
  }
  return( mtx->perm.row.cur_to_org[row] );
}

int32 mtx_col_to_org(mtx_matrix_t mtx, int32 col)
{
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return -1;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return -1;
#endif
  }
  return( mtx->perm.col.cur_to_org[col] );
}

int32 mtx_org_to_row( mtx_matrix_t mtx, int32 org)
{
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return -1;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return -1;
#endif
  }
  return( mtx->perm.row.org_to_cur[org] );
}

int32 mtx_org_to_col( mtx_matrix_t mtx, int32 org){
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return -1;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if(!mtx_check_matrix(mtx)) return -1;
#endif
  }
  return( mtx->perm.col.org_to_cur[org] );
}

boolean mtx_row_parity(mtx_matrix_t mtx)
{
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return -1;
  if (ISSLAVE(mtx)) {
    if(!mtx_check_matrix(mtx->master)) return -1;
  }
#endif
  if (ISSLAVE(mtx)) {
    return( mtx->master->perm.row.parity );
  } else {
    return( mtx->perm.row.parity );
  }
}

boolean mtx_col_parity( mtx_matrix_t mtx)
{
#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return -1;
  if (ISSLAVE(mtx)) {
    if(!mtx_check_matrix(mtx->master)) return -1;
  }
#endif
  if (ISSLAVE(mtx)) {
    return( mtx->master->perm.col.parity );
  } else {
    return( mtx->perm.col.parity );
  }
}

/***********************************************************************\
 structural analysis stuff
\***********************************************************************/
struct assign_row_vars {
   int32 rv_indicator;		/* rows visited indicator */
   int32 *row_visited;
   mtx_range_t unassigned_cols;
   mtx_range_t assigned_cols;
};

/* MUST NOT BE CALLED ON slave matrix */
static int32 assign_row( mtx_matrix_t mtx, int32 row,
			 struct assign_row_vars *vars)
/**
 ***  Attempts to assign one of the columns in the range
 ***  vars->unassigned_columns to the given row (exchange that column with
 ***  column "row" in order to place a non-zero on the diagonal).  Returns
 ***  the column that is assigned, or < 0 if not possible.
 ***
 ***  Assignment is done row-wise by starting with the top row, finding the
 ***  nearest column in the row to the right of the diagonal to assign to it,
 ***  and advancing to the next row.  If no column exists to the right of the
 ***  diagonal, then it precedes along a steward's path to find amongst the
 ***  previously assigned columns one which will be more suitably assigned to
 ***  it, thereby forcing a row which was visited earlier to be re-assigned to
 ***  some new column.  Termination occurs when there are no more unvisited
 ***  rows.  If there still remains non-empty yet unassigned rows, they will
 ***  be positioned just after the last assigned row.  Same goes for any
 ***  remaining non-empty unassigned columns.  The condensed grouping of
 ***  assigned rows and columns will be of dimension equal to the symbolic
 ***  rank of the matrix and are used to form an initial block.
 **/
{
   struct element_t Rewind, *elt;
   int32 *tocur;

   tocur = mtx->perm.col.org_to_cur;
   Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
   if( NULL != (elt=mtx_next_col(&Rewind,&(vars->unassigned_cols),tocur)) ) {
      /* Cheap assignment */
      register int32 col;
      col = tocur[elt->col];
      swap(&(mtx->perm.col),col,row);
      return(col);
   }

   vars->row_visited[row] = vars->rv_indicator;
   for( elt = &Rewind ; NULL != (elt=mtx_next_col(elt,&(vars->assigned_cols),tocur)) ;) {
      int32 col;
      col = tocur[elt->col];
      if( vars->row_visited[col] != vars->rv_indicator ) {
         register int32 assigned_col;
         if( (assigned_col = assign_row(mtx,col,vars)) >= ZERO ) {
            swap(&(mtx->perm.col),assigned_col,row);
            return( assigned_col );
         }
      }
   }
   return( -1 );
}

/*
 * This function is very similar to mtx_output_assign. It however
 * takes a region in which to do this output_assignement.
 * It returns the number of rows that could be assigned.
 * Remember that rv stands for row_visited, for newcomers to this file.
 */
int mtx_output_assign_region(mtx_matrix_t mtx,
			     mtx_region_t *region,
			     int *orphaned_rows)
{
  struct assign_row_vars vars;
  struct element_t Rewind;
  int32 nrows, *tocur;
  mtx_range_t single_col;
  mtx_coord_t nz;
  int notassigned = 0;

#if MTX_DEBUG
   if( !mtx_check_matrix(mtx) ) return 0;
#endif

  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if( !mtx_check_matrix(mtx) ) return 0;
#endif
  }

  vars.row_visited = mtx->order > ZERO ? ASC_NEW_ARRAY_CLEAR(int32,mtx->order) : NULL;
  vars.rv_indicator = 1;

  if (region!=mtx_ENTIRE_MATRIX) {
    vars.unassigned_cols.high = region->col.high;
    vars.assigned_cols.low = region->col.low;
    tocur = mtx->perm.col.org_to_cur;
    nrows = region->row.high - region->row.low + 1;
  }
  else{
    vars.unassigned_cols.high = mtx->order - 1;
    vars.assigned_cols.low = ZERO;
    tocur = mtx->perm.col.org_to_cur;
    nrows =  mtx->order;
  }


  for( nz.row = region->row.low ; nz.row < nrows ; ) {
    single_col.low = single_col.high = nz.row;
    vars.unassigned_cols.low = nz.row;
    vars.assigned_cols.high = nz.row - 1;

    /*
     * Find next nnz in row and attempt to assign it using the
     * recursive function assign_row. If successful, proceed
     * to the next row.  Otherwise, move the row down to end of the
     * matrix. -- we are structurally singular.
     */
    Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
    if (mtx_next_col(&Rewind, &single_col, tocur) ||
	assign_row(mtx, nz.row, &vars) >= ZERO) {
      ++nz.row;					/* Go to next row */
    }
    else {
      swap(&(mtx->perm.row), nz.row, --nrows);	/* Move row to the end */
      notassigned++;
    }
    ++vars.rv_indicator;	/* Effectively clear vars.row_visited */
  }

  if (NOTNULL(vars.row_visited)) ascfree(vars.row_visited);
  *orphaned_rows = notassigned;	/* return count of unassigned rows */
  return nrows;
}

void mtx_output_assign( mtx_matrix_t mtx, int32 hirow, int32 hicol)
{
  struct assign_row_vars vars;
  struct element_t Rewind;
  int32 nrows, ndxhigh, *tocur;
  mtx_range_t single_col;
  mtx_coord_t nz;

#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) return; /*ben*/
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if( !mtx_check_matrix(mtx) ) return;
#endif
  }
  vars.row_visited = mtx->order > ZERO ? ASC_NEW_ARRAY_CLEAR(int32,mtx->order) : NULL;
  vars.rv_indicator = 1;
  vars.unassigned_cols.high = mtx->order-1;
  vars.assigned_cols.low = ZERO;
  tocur = mtx->perm.col.org_to_cur;

  nrows = mtx->order;
  for( nz.row = ZERO ; nz.row < nrows ; ) {
    single_col.low = single_col.high = nz.row;
    vars.unassigned_cols.low = nz.row;
    vars.assigned_cols.high = nz.row-1;
    Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
    if( mtx_next_col(&Rewind,&single_col,tocur) ||
        assign_row(mtx,nz.row,&vars) >= ZERO ) {
      ++nz.row; /* Go to next row */
    } else {
      swap(&(mtx->perm.row),nz.row,--nrows);   /* Move row to the end */
    }
    ++vars.rv_indicator; /* Effectively clear vars.row_visited */
  }

  mtx->data->symbolic_rank = nrows;
  if( NOTNULL(mtx->data->block) )
    ascfree(mtx->data->block);

   /**
    ***  Place all assigned rows and columns into one
    ***  initial block.
    **/
  if( mtx->data->symbolic_rank > 0 ) {
    mtx->data->nblocks = 1;
    mtx->data->block = (mtx_region_t *)ascmalloc( sizeof(mtx_region_t) );
    mtx->data->block[0].row.low = mtx->data->block[0].col.low = 0;
    mtx->data->block[0].row.high = mtx->data->block[0].col.high = nrows - 1;
  } else {
    mtx->data->nblocks = 0;
    mtx->data->block = NULL;
  }

   /**
    ***  Condense all unassigned rows on the outside of the block.
    **/
  ndxhigh = mtx->order-1;
  for (nz.row = nrows; nz.row <= ndxhigh; ) {
    if( NOTNULL(mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]]) ) ++nz.row;
    else swap(&(mtx->perm.row),nz.row,ndxhigh--);
  }
   /**
    ***  Condense any incidenceless but wanted orgrows on
    ***  the outside of the assigned and unassigned block if sensible.
    **/
  if (nz.row < hirow) {
    ndxhigh = mtx->order-1;
    for (; nz.row <= ndxhigh; ) {
      if( mtx->perm.row.cur_to_org[nz.row] < hirow ) ++nz.row;
      else swap(&(mtx->perm.row),nz.row,ndxhigh--);
    }
  }
   /**
    ***  Condense all unassigned columns on
    ***  the outside of the block.
    **/
  ndxhigh = mtx->order-1;
  for (nz.col = nrows; nz.col <= ndxhigh; ) {
    if( NOTNULL(mtx->hdr.col[mtx->perm.col.cur_to_org[nz.col]]) ) ++nz.col;
    else swap(&(mtx->perm.col),nz.col,ndxhigh--);
  }
   /**
    ***  Condense any incidenceless but wanted orgcols on
    ***  the outside of the assigned and unassigned block if sensible.
    **/
  if (nz.col < hicol) {
    ndxhigh = mtx->order-1;
    for (; nz.col <= ndxhigh; ) {
      if( mtx->perm.col.cur_to_org[nz.col] < hicol ) ++nz.col;
      else swap(&(mtx->perm.col),nz.col,ndxhigh--);
    }
  }

  if( NOTNULL(vars.row_visited) )
    ascfree( vars.row_visited );
}

boolean mtx_output_assigned( mtx_matrix_t mtx)
{
  if(!mtx_check_matrix(mtx)) return 0;
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if( !mtx_check_matrix(mtx) ) return 0;
#endif
  }
  return (mtx->data->symbolic_rank > -1);
}

int32 mtx_symbolic_rank( mtx_matrix_t mtx)
{
#if MTX_DEBUG
   if( !mtx_check_matrix(mtx) ) return -2;
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if( !mtx_check_matrix(mtx) ) return -2;
#endif
  }

  if( !mtx_output_assigned(mtx) ) {   /* matrix will be checked */
#if MTX_DEBUG
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_symbolic_rank\n");
    FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
#endif
  }
  return (mtx->data->symbolic_rank);
}

void mtx_set_symbolic_rank( mtx_matrix_t mtx, int32 rank)
{
  mtx->data->symbolic_rank = rank;
}

boolean mtx_make_col_independent( mtx_matrix_t mtx, int32 col, mtx_range_t *rng)
{
  struct assign_row_vars vars;
  mtx_coord_t nz;
  int32 ncol;

  if (mtx!=NULL && ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if( !mtx_check_matrix(mtx) ) return -2;
#endif
  }
  if( !mtx_output_assigned(mtx) ) {   /* matrix will be checked */
#if MTX_DEBUG
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_make_col_independent\n");
    FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
#endif
    if(!mtx_check_matrix(mtx)) return -2;
  }
  if( col >= mtx->data->symbolic_rank ) return TRUE; /* col already ind */
  if( rng->high < rng->low ) return FALSE; /* nobody to choose from */
  if( rng->low < mtx->data->symbolic_rank ) return FALSE; /* bad choices */

  vars.row_visited = mtx->order > ZERO ? ASC_NEW_ARRAY_CLEAR(int32,mtx->order) : NULL;
  vars.rv_indicator = 1;
  vars.unassigned_cols.low = rng->low;
  vars.unassigned_cols.high = rng->high;
  vars.assigned_cols.low = ZERO;
  vars.assigned_cols.high = mtx->data->symbolic_rank-2;

  /**
   ***  Move (col,col) to the bottom right corner
   **/
  nz.row = nz.col = col;
  swap(&(mtx->perm.row),nz.row,mtx->data->symbolic_rank-1);
  swap(&(mtx->perm.col),nz.col,mtx->data->symbolic_rank-1);

  ncol= assign_row(mtx,mtx->data->symbolic_rank-1,&vars);
  swap(&(mtx->perm.row),nz.row,mtx->data->symbolic_rank-1);
  swap(&(mtx->perm.col),nz.col,mtx->data->symbolic_rank-1);
  if(ncol < ZERO ) {
    /* put it back the way it was */
    return FALSE;
  }

  if( NOTNULL(mtx->data->block) && mtx->data->nblocks > 1 ) {
    ascfree(mtx->data->block);
    mtx->data->nblocks = 1;
    mtx->data->block = (mtx_region_t *)ascmalloc( sizeof(mtx_region_t) );
    mtx->data->block[0].row.low = ZERO;
    mtx->data->block[0].col.low = ZERO;
    mtx->data->block[0].row.high = mtx->data->symbolic_rank-1;
    mtx->data->block[0].col.high = mtx->data->symbolic_rank-1;
  }
  if( NOTNULL(vars.row_visited) )
    ascfree( vars.row_visited );
  return TRUE;
}

struct analyze_row_vars {
   int32 done;
   int32 vstack;
   int32 *lowlink;
   int32 *blocksize;
   int32 nblocks;
};

static
int32 analyze_row(mtx_matrix_t mtx, int32 row, struct analyze_row_vars *vars,
                  mtx_range_t *rng)
/*
 *  Places the row on the stack (row must be initially unvisited) and
 *  then determines lowlink[row].  If, in the end, lowlink[row] = row,
 *  then this row and all rows "above it on the stack" (i.e.
 *  vstack <= ndx <= row) belong to the same block, which is then moved
 *  from the stack to done (nblocks and blocksize updated).  The new row
 *  number is returned (this may be different from the given row number
 *  since rows tend to move around).
 */
{
   mtx_range_t cols;
   struct element_t Rewind, *elt;
   int32 *tocur;

   /* Push row onto the stack */
   --(vars->vstack);
   swap(&(mtx->perm.row),row,vars->vstack);
   swap(&(mtx->perm.col),row,vars->vstack);
   row = vars->vstack;
   vars->lowlink[row] = row;   /* Starting value of lowlink */

   /*
    *  Scan the row for non-zeros.  If unvisited indices are found, analyze
    *  them first (recursive call to this function) and in any case, if they
    *  are still on the stack, update lowlink[row] according to the new
    *  information.  When the entire row is scanned, lowlink[row] should be
    *  completely computed.
    */

   /*
    * THESE WERE:			KAA_MODIFICATION
    * cols.low = 0; and
    * cols.high = mtx->data->symbolic_rank-1;
    */
   cols.low = rng->low;
   cols.high = rng->high;
   tocur = mtx->perm.col.org_to_cur;
   Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
   for( elt = &Rewind ; NULL != (elt=mtx_next_col(elt,&cols,tocur)) ; ) {
      int32 col = tocur[elt->col];
      if( vars->done <= col && col < vars->vstack ) {
         /* Column unvisited, compute lowlink[col] */
#if SWAPS_PRESERVE_ORDER
	 col = analyze_row(mtx,col,vars,rng);
#else
	 if( col != analyze_row(mtx,col,vars,rng) ) {
	    elt = &Rewind;
            continue;
         }
#endif
      }

      if( col >= vars->vstack ) {
         /* Still on stack: update lowlink[row] */
         if( vars->lowlink[col] > vars->lowlink[row] )
            vars->lowlink[row] = vars->lowlink[col];
      } else {
         /**
          ***  col is done (already in a block) and therefore
          ***  won't contribute anything to lowlink[row]
          **/
      }
   }

   if( vars->lowlink[row] == row ) {
      /* Rows currently in vstack..row form the next block */
      ++(vars->nblocks);
      vars->blocksize[vars->done] = row - vars->vstack + 1; /* Block size */
      /* Move rows/columns from the stack to done */
      for( ; vars->vstack <= row ; ++(vars->vstack),++(vars->done) ) {
         swap(&(mtx->perm.row),vars->vstack,vars->done);
         swap(&(mtx->perm.col),vars->vstack,vars->done);
      }
      return( vars->done - 1 ); /* That's where the row ends up */
   }
   return(row);
}

static
int32 analyze_col(mtx_matrix_t mtx, int32 col, struct analyze_row_vars *vars,
                  mtx_range_t *rng)
/* (swap row for col here)
 *  Places the row on the stack (row must be initially unvisited) and
 *  then determines lowlink[row].  If, in the end, lowlink[row] = row,
 *  then this row and all rows "above it on the stack" (i.e.
 *  vstack <= ndx <= row) belong to the same block, which is then moved
 *  from the stack to done (nblocks and blocksize updated).  The new row
 *  number is returned (this may be different from the given row number
 *  since rows tend to move around).
 */
{
   mtx_range_t rows;
   struct element_t Rewind, *elt;
   int32 *tocur;

   /* Push row onto the stack */
   --(vars->vstack);
   swap(&(mtx->perm.col),col,vars->vstack);
   swap(&(mtx->perm.row),col,vars->vstack);
   col = vars->vstack;
   vars->lowlink[col] = col;   /* Starting value of lowlink */

   /*
    *  Scan the row for non-zeros.  If unvisited indices are found, analyze
    *  them first (recursive call to this function) and in any case, if they
    *  are still on the stack, update lowlink[row] according to the new
    *  information.  When the entire row is scanned, lowlink[row] should be
    *  completely computed.
    */

   /* for full version:
    * cols.low = 0; and
    * cols.high = mtx->data->symbolic_rank-1;
    */
   rows.low = rng->low;
   rows.high = rng->high;
   tocur = mtx->perm.row.org_to_cur;
   Rewind.next.row = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
   for( elt = &Rewind ; NULL != (elt=mtx_next_row(elt,&rows,tocur)) ; ) {
      int32 row = tocur[elt->row];
      if( vars->done <= row && row < vars->vstack ) {
         /* Column unvisited, compute lowlink[col] */
#if SWAPS_PRESERVE_ORDER
	 row = analyze_col(mtx,row,vars,rng);
#else
	 if( row != analyze_col(mtx,row,vars,rng) ) {
	    elt = &Rewind;
            continue;
         }
#endif
      }

      if( row >= vars->vstack ) {
         /* Still on stack: update lowlink[row] */
         if( vars->lowlink[row] > vars->lowlink[col] ) {
            vars->lowlink[col] = vars->lowlink[row];
         }
      } else {
         /*
          *  col is done (already in a block) and therefore
          *  won't contribute anything to lowlink[row]
          */
      }
   }

   if( vars->lowlink[col] == col ) {
      /* Rows currently in vstack..row form the next block */
      ++(vars->nblocks);
      vars->blocksize[vars->done] = col - vars->vstack + 1; /* Block size */
      /* Move rows/columns from the stack to done */
      for( ; vars->vstack <= col ; ++(vars->vstack),++(vars->done) ) {
         swap(&(mtx->perm.col),vars->vstack,vars->done);
         swap(&(mtx->perm.row),vars->vstack,vars->done);
      }
      return( vars->done - 1 ); /* That's where the row ends up */
   }
   return(col);
}


/* a function to check the diagonal for holes. if symbolic_rank is
 * set, and rng->high < rank, returns immediately.
 * Worst Cost: O(nnz)  where nnz = incidence count sum over rows in rng.
 * Does not pass calls up to master.
 * Returns number of holes detected in diagonal in rng given.
 * If noisy != 0, writes the hole locations to g_mtxerr.
 * On detectably bad input, returns -1.
 */
int32 mtx_full_diagonal(mtx_matrix_t mtx, mtx_range_t *rng, int noisy)
{
  mtx_coord_t nz;
  mtx_range_t diag;
  int32 holecount=0;

  if (mtx==NULL || rng == NULL) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_full_diagonal sent NULL.\n");
    return -1;
  }
  if (mtx_output_assigned(mtx) && rng->high < mtx->data->symbolic_rank) {
    return 0;
  }
  if (!mtx_check_matrix(mtx)) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_full_diagonal sent bad mtx.\n");
    return -1;
  }
  if (rng->low <0 || rng->high >= mtx->order) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_full_diagonal sent bad rng.\n");
    return -1;
  }
  if (noisy) {
    for (nz.row=rng->low; nz.row <= rng->high; nz.row++) {
      diag.high = diag.low = nz.row;
      nz.col = mtx_FIRST;
      mtx_next_in_row(mtx,&nz,&diag); /* find diagonal, if there */
      if (nz.col==mtx_LAST) {
        holecount++;
        FPRINTF(g_mtxerr,"mtx: mtx_full_diagonal found hole at %d.\n",nz.row);
      }
    }
  } else {
    for (nz.row=rng->low; nz.row <= rng->high; nz.row++) {
      diag.high = diag.low = nz.row;
      nz.col = mtx_FIRST;
      mtx_next_in_row(mtx,&nz,&diag); /* find diagonal, if there */
      if (nz.col==mtx_LAST) holecount++;
    }
  }
  return holecount;
}

/* so long as the mesh is sane, traversing it along one axis transposes
 * it in both directions. This must be called just before swapping headers
 * or the semantics are not right.
 */
static
void transpose_mesh(mtx_matrix_t mtx)
{
  struct element_t *elttmp; /* swap var for nextelt pointers in elements */
  struct element_t *next, *elt;
  int32 ndxtmp;			/* swap var for orgrow/orgcol indices */
  int32 nrows, ndx;

  if (mtx == NULL) {
    return;
  }
  nrows = mtx->order;
  for( ndx = ZERO ; ndx < nrows ; ndx++) {
    next = mtx->hdr.row[ndx];
    /* traverse the row, turning it into a column behind us as we go. */
    while (next != NULL) {
      elt = next; /* the use of elt is redundant, but much more readable. */
      /* swap org row/col indices */
      ndxtmp = elt->col;
      elt->col = elt->row;
      elt->row = ndxtmp;
      /* swap next elt pointers */
      elttmp = elt->next.col;
      elt->next.col = elt->next.row;
      elt->next.row = elttmp;
      next = elttmp;
    }
  }
}

int32 mtx_transpose(mtx_matrix_t mtx)
{
  struct element_t **hdrtmp;	/* swap var for headers */
  int32 *permtmp;		/* swap var for permutation vectors. */
  int32 ndxtmp,bnum;			/* swap var for indices */
  mtx_matrix_t master;
  int32 slave;


#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_transpose\n");
    FPRINTF(g_mtxerr,"        Matrix given is in error.\n");
    return mtx_NONE; /*ben*/
  }
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if( !mtx_check_matrix(mtx) ) {
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_transpose\n");
      FPRINTF(g_mtxerr,"        Matrix given is in error.\n");
      return mtx_NONE;
    }
#endif
  }
  master = mtx;
  if (master->perm.transpose == mtx_NONE) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_transpose\n");
    FPRINTF(g_mtxerr,"        Matrix given is 0 order.\n");
    return mtx_NONE;
  }
  master->perm.transpose = !(master->perm.transpose);
  /* swap perms on master */
  permtmp = mtx->perm.col.org_to_cur;  /* do o2v */
  mtx->perm.col.org_to_cur = mtx->perm.row.org_to_cur;
  mtx->perm.row.org_to_cur = permtmp;
  permtmp = mtx->perm.col.cur_to_org;  /* do v2o */
  mtx->perm.col.cur_to_org = mtx->perm.row.cur_to_org;
  mtx->perm.row.cur_to_org = permtmp;

  /* transpose mesh on master */
  transpose_mesh(mtx);

  /* swap headers on master */
  hdrtmp = mtx->hdr.col;
  mtx->hdr.col = mtx->hdr.row;
  mtx->hdr.row = hdrtmp;

  /* transpose blocks on master */
  if (mtx->data->block != NULL) {
    for (bnum = 0; bnum < mtx->data->nblocks; bnum++) {
      /* upper left */
      ndxtmp = mtx->data->block[bnum].row.low;
      mtx->data->block[bnum].row.low = mtx->data->block[bnum].col.low;
      mtx->data->block[bnum].col.low = ndxtmp;
      /* lower right */
      ndxtmp = mtx->data->block[bnum].row.high;
      mtx->data->block[bnum].row.high = mtx->data->block[bnum].col.high;
      mtx->data->block[bnum].col.high = ndxtmp;
    }
  }

  /* swap hdrs, update perm references, and do mesh on slaves */
  for (slave = 0; slave < master->nslaves; slave++) {
    mtx = master->slaves[slave];
    mtx->perm.row.org_to_cur = master->perm.row.org_to_cur;
    mtx->perm.row.cur_to_org = master->perm.row.cur_to_org;
    mtx->perm.col.org_to_cur = master->perm.col.org_to_cur;
    mtx->perm.col.cur_to_org = master->perm.col.cur_to_org;
    transpose_mesh(mtx);
    hdrtmp = mtx->hdr.col;
    mtx->hdr.col = mtx->hdr.row;
    mtx->hdr.row = hdrtmp;
  }

  return 0;
}

int32 mtx_isa_transpose(mtx_matrix_t mtx)
{
#if MTX_DEBUG
  if( !mtx_check_matrix(mtx) ) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_isa_transpose\n");
    FPRINTF(g_mtxerr,"        Matrix given is in error.\n");
    return mtx_NONE; /*ben*/
  }
#endif
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
#if MTX_DEBUG
    if( !mtx_check_matrix(mtx) ) {
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_isa_transpose\n");
      FPRINTF(g_mtxerr,"        Matrix given's master is in error.\n");
      return mtx_NONE;
    }
#endif
  }
  return mtx->perm.transpose;
}

/*
 * This function uses the same algorithm as its sister function
 * mtx_partition (at the moment). It is more general in that it
 * explicitly takes a region, and will return a block_list
 * structure rather than manipulating the one associated with
 * the matrix. It now works (we hope) on an arbitrary square
 * region.
 */
mtx_block_t *mtx_block_partition( mtx_matrix_t mtx, mtx_region_t *reg)
{
  struct analyze_row_vars vars;
  int32 blocknum, start, size;
  mtx_block_t *blocklist;

  if (mtx!=NULL && ISSLAVE(mtx)) {
    mtx = mtx->master;
  }

  if (!mtx_output_assigned(mtx)) {
    int mc = mtx_check_matrix(mtx);
    if (!mc) return NULL;
    if (mtx_full_diagonal(mtx,&(reg->row),1)) {
      FPRINTF(g_mtxerr,"WARNING:  (mtx) mtx_block_partition\n");
      FPRINTF(g_mtxerr,"          Assignment bad. partitioning may be bad..\n");
    }
  }

  vars.vstack = reg->row.high + 1;	/* was symbolic_rank */
  if (vars.vstack > 0) {
    vars.lowlink = vars.blocksize =
      ASC_NEW_ARRAY(int32,vars.vstack);
  } else {
    vars.lowlink = vars.blocksize = NULL;
  }
  vars.done = 0;
  vars.nblocks = 0;

  /*
   * The core of the partition routine. Analyze row
   * now takes a range, which represents a square region.
   */
  while( vars.done < vars.vstack ) {
    analyze_row(mtx,vars.done,&vars,&(reg->row));
  }

  /*
   * Prepare the block list structure and copy the data
   * from into it from vars.
   */
  if (vars.nblocks > 0) {
    blocklist = (mtx_block_t *)ascmalloc(sizeof(mtx_block_t));
    blocklist->nblocks = vars.nblocks;
    blocklist->block = (mtx_region_t *)
      ascmalloc(vars.nblocks * sizeof(mtx_region_t));
  } else {
    return NULL;
  }

  for(start = blocknum = 0; blocknum < vars.nblocks; blocknum++) {
    size = vars.blocksize[start];
    blocklist->block[blocknum].row.low =
      blocklist->block[blocknum].col.low = start;
    blocklist->block[blocknum].row.high =
      blocklist->block[blocknum].col.high = start + size - 1;
    start += size;
  }
  if(NOTNULL(vars.lowlink))
    ascfree((POINTER)vars.lowlink);

  return blocklist;
}


void mtx_partition( mtx_matrix_t mtx)
/**
 ***  Matrix must be previously output assigned.  Then the assigned
 ***  region (symbolic basis) portion of the matrix is partitioned
 ***  leaving unassigned rows and columns on the outside.
 ***
 ***  vstack -----------------------+
 ***  done -------+                 |
 ***              V                 V
 ***     done     |    unvisited    |   stack (visited)
 ***
 ***  0 <= "done" < done <= "unvisited" < vstack <= "stack" < order
 ***
 ***  For all ndx on the stack, lowlink[ndx] "points to" the deepest part of
 ***  the stack (highest index) which can be reached from row ndx by the
 ***  associated directed graph (with edges r-->c, where (r,c) is non-zero).
 ***
 ***  For blocks which are already completed, blocksize[start] = block size,
 ***  where start is the starting row/column index of the block.  In every
 ***  case, 0 <= start < done, so lowlink and blocksize can be the same
 ***  array.
 **/
{
  struct analyze_row_vars vars;
  int32 blocknum, start, size;
  mtx_range_t rng;

  if (mtx!=NULL && ISSLAVE(mtx)) {
    mtx = mtx->master;
  }
  if( !mtx_output_assigned(mtx) ) {
    int mc=mtx_check_matrix(mtx);
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_partition\n");
    FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
    if(!mc) return;
  }

  vars.vstack = mtx->data->symbolic_rank;
  vars.lowlink = vars.blocksize = vars.vstack > 0 ?
    ASC_NEW_ARRAY(int32,vars.vstack) : NULL;
  vars.done = 0;
  vars.nblocks = 0;

  rng.low = 0;				/* KAA_MODIFICATION */
  rng.high = mtx->data->symbolic_rank-1;
  while( vars.done < vars.vstack )
    analyze_row(mtx,vars.done,&vars,&rng);

  if( NOTNULL(mtx->data->block) )
    ascfree(mtx->data->block);
  mtx->data->nblocks = vars.nblocks;
  mtx->data->block = vars.nblocks > 0 ?
    ASC_NEW_ARRAY(mtx_region_t,vars.nblocks) : NULL;
  for( start=blocknum=0 ; blocknum < vars.nblocks ; blocknum++ ) {
    size = vars.blocksize[start];
    mtx->data->block[blocknum].row.low =
       mtx->data->block[blocknum].col.low = start;
    mtx->data->block[blocknum].row.high =
       mtx->data->block[blocknum].col.high = start + size - 1;
    start += size;
  }
  if( NOTNULL(vars.lowlink) )
    ascfree( (POINTER)vars.lowlink );
}

void mtx_ut_partition( mtx_matrix_t mtx)
/*
 * This is the upper triangular version of the blt mtx_partition function.
 * Swap 'row' and 'column' or r and c in all related comments.
 *  Matrix must be previously output assigned.  Then the assigned
 *  region (symbolic basis) portion of the matrix is partitioned
 *  leaving unassigned rows and columns on the outside.
 *
 *  vstack -----------------------+
 *  done -------+                 |
 *              V                 V
 *     done     |    unvisited    |   stack (visited)
 *
 *  0 <= "done" < done <= "unvisited" < vstack <= "stack" < order
 *
 *  For all ndx on the stack, lowlink[ndx] "points to" the deepest part of
 *  the stack (highest index) which can be reached from row ndx by the
 *  associated directed graph (with edges r-->c, where (r,c) is non-zero).
 *
 *  For blocks which are already completed, blocksize[start] = block size,
 *  where start is the starting row/column index of the block.  In every
 *  case, 0 <= start < done, so lowlink and blocksize can be the same
 *  array.
 */
{
  struct analyze_row_vars vars;
  int32 blocknum, start, size;
  mtx_range_t rng;

  if (mtx!=NULL && ISSLAVE(mtx)) {
    mtx = mtx->master;
  }
  if( !mtx_output_assigned(mtx) ) {
    int mc=mtx_check_matrix(mtx);
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_ut_partition\n");
    FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
    if(!mc) return;
  }

  vars.vstack = mtx->data->symbolic_rank;
  vars.lowlink = vars.blocksize = vars.vstack > 0 ?
    ASC_NEW_ARRAY(int32,vars.vstack) : NULL;
  vars.done = 0;
  vars.nblocks = 0;

  rng.low = 0;				
  rng.high = mtx->data->symbolic_rank-1;
  while( vars.done < vars.vstack ) {
    analyze_col(mtx,vars.done,&vars,&rng);
  }

  if( NOTNULL(mtx->data->block) ) {
    ascfree(mtx->data->block);
  }
  mtx->data->nblocks = vars.nblocks;
  mtx->data->block = vars.nblocks > 0 ?
    ASC_NEW_ARRAY(mtx_region_t,vars.nblocks) : NULL;
  for( start=blocknum=0 ; blocknum < vars.nblocks ; blocknum++ ) {
    size = vars.blocksize[start];
    mtx->data->block[blocknum].row.low =
       mtx->data->block[blocknum].col.low = start;
    mtx->data->block[blocknum].row.high =
       mtx->data->block[blocknum].col.high = start + size - 1;
    start += size;
  }
  if( NOTNULL(vars.lowlink) ) {
    ascfree( (POINTER)vars.lowlink );
  }
}

#ifdef DELETE_THIS_DEAD_FUNCTION
/* a comparator meeting the qsort spec exactly for any pair i1,i2 */
static int mtx_cmpint(int *i1, int *i2)
{
  if (*i1 < *i2) return -1;
  return (*i1 > *i2);
}
#endif /* DELETE_THIS_DEAD_FUNCTION */

/* a faster comparator which assumes i1 =/= i2 ever */
static int mtx_cmpintfast(int *i1, int *i2)
{
  if (*i1 < *i2) return -1;
  return 1;
}

/* this could be faster, but as it's a 1 time deal, who cares?
   we're at the mercy of the system qsort. */
/* ah, don't you hate prototypes? now it looks like this
function matters... */
void mtx_org_permute(mtx_matrix_t mtx, mtx_region_t *reg)
{
  if (ISNULL(mtx)) {
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_org_permute\n");
    FPRINTF(g_mtxerr,"        NULL matrix given.\n");
    return;
  }
  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
  }
  if (ISNULL(reg)) {
    mtx_reset_perm(mtx);
  } else {
    int32 i,j,len;
    int32 *sort,*top;
    len=reg->col.high-reg->col.low+1;
    if (len>1) {
      /* create sort space */
      top=sort=ASC_NEW_ARRAY(int,len);
      if (ISNULL(sort)) {
        FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_org_permute\n");
        FPRINTF(g_mtxerr,"        Insufficient memory. Not permuted.\n");
        return;
      }
      /* copy current org ordering into array */
      for (i = reg->col.low; i <= reg->col.high; i++) {
        *top = mtx->perm.col.cur_to_org[i];
        top++;
      }
      /* sort org columns */
      qsort((void *)sort,(size_t)len,sizeof(int),
            (int (*)(const void *,const  void *))mtx_cmpintfast);
      /* permute columns */
      top = sort;
      for (i= reg->col.low; i < reg->col.high; i++ ) {
        if (mtx->perm.col.cur_to_org[i] != *top) {
          j = mtx->perm.col.org_to_cur[*top];
          swap(&(mtx->perm.col),i,j);
        }
        top++;
      }
      ascfree(sort);
    }
    /* ditto for rows */
    len=reg->row.high-reg->row.low+1;
    if (len>1) {
      top=sort=ASC_NEW_ARRAY(int,len);
      if (ISNULL(sort)) {
        FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_org_permute\n");
        FPRINTF(g_mtxerr,"        Insufficient memory. Not permuted.\n");
        return;
      }
      for (i = reg->row.low; i <= reg->row.high; i++) {
        *top = mtx->perm.row.cur_to_org[i];
        top++;
      }
      qsort((void *)sort,(size_t)len,sizeof(int),
            (int (*)(const void *,const  void *))mtx_cmpintfast);
      top = sort;
      for (i= reg->row.low; i < reg->row.high; i++ ) {
        if (mtx->perm.row.cur_to_org[i] != *top) {
          j = mtx->perm.row.org_to_cur[*top];
          swap(&(mtx->perm.row),i,j);
        }
        top++;
      }
      ascfree(sort);
    }
  }
}

/***********************************************************************\
 end structural analysis stuff
\***********************************************************************/
boolean mtx_check_blocks( mtx_matrix_t mtx)
{
  mtx_range_t invalid;
  int32 blocknum;

  if (ISSLAVE(mtx)) {
    mtx = mtx->master;
  }
  if( !mtx_output_assigned(mtx) ) {   /* matrix will be checked */
    FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_check_blocks\n");
    FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
    return( FALSE );
  }

  invalid.high = mtx->data->symbolic_rank-1;
  /* Last block need not be checked */
  for( blocknum = 0 ; blocknum < mtx->data->nblocks-1 ; ++blocknum ) {
    mtx_coord_t nz;
    invalid.low = mtx->data->block[blocknum].row.high + 1;
    for( nz.row=mtx->data->block[blocknum].row.low ;
         nz.row<=mtx->data->block[blocknum].row.high ;
         ++nz.row ) {
      struct element_t Rewind;
      Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[nz.row]];
      if( mtx_next_col(&Rewind,&invalid,mtx->perm.col.org_to_cur) ) {
        mtx->data->nblocks = -1;
        if( NOTNULL(mtx->data->block) )
          ascfree(mtx->data->block);
        return(FALSE);
      }
    }
  }
  return(TRUE);
}

int32 mtx_number_of_blocks( mtx_matrix_t mtx)
{
   if( !mtx_output_assigned(mtx) ) {   /* matrix will be checked */
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_number_of_blocks\n");
      FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
   }

#if MTX_DEBUG
   if( !mtx_check_blocks(mtx) ) {
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_number_of_blocks\n");
      FPRINTF(g_mtxerr,"        Invalid partition.\n");
      return mtx_NONE;
   }
#endif

   return (mtx->data->nblocks);
}

int32 mtx_block( mtx_matrix_t mtx, int32 block_number, mtx_region_t *block)
{
   mtx_region_t *reg;
   int error=0;

   if( !mtx_output_assigned(mtx) ) {   /* matrix will be checked */
#if MTX_DEBUG
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_block\n");
      FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
#endif
      error=1;
   }

#if MTX_DEBUG
   if( !mtx_check_blocks(mtx) ) {
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_block\n");
      FPRINTF(g_mtxerr,"        Invalid partition.\n");
   }
#endif

   if( (block_number > mtx->data->nblocks-1) || (block_number<0) ) {
#if MTX_DEBUG
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_block\n");
      FPRINTF(g_mtxerr,"        Block number doesn't exist.\n");
#endif
      error=1;
   }
   if (error) {
     block->row.low=0;
     block->col.low=0;
     block->row.high=mtx->order-1;
     block->col.high=mtx->order-1;
     return mtx_NONE;
   }

   reg = &(mtx->data->block[block_number]);
   mem_copy_cast(reg,block,sizeof(mtx_region_t));
   return 0;
}

int32 mtx_block_containing_row(mtx_matrix_t mtx, int32 row, mtx_region_t *block)
{
   int32 blow,bhigh;
   mtx_region_t *reg;

   if( !mtx_output_assigned(mtx) ) {   /* matrix will be checked */
#if MTX_DEBUG
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_block_containing_row\n");
      FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
#endif
      return 0;
   }

#if MTX_DEBUG
   if( !mtx_check_blocks(mtx) ) {
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_block_containing_row\n");
      FPRINTF(g_mtxerr,"        Invalid partition.\n");
   }
#endif

   blow = 0;
   bhigh = mtx->data->nblocks-1;
   while( blow <= bhigh ) {
      int32 block_number = (blow+bhigh)/2;
      if( row > mtx->data->block[block_number].row.high ) {
         blow = block_number+1;
      } else if( row < mtx->data->block[block_number].row.low ) {
         bhigh = block_number-1;
      } else {
         reg = &(mtx->data->block[block_number]);
         mem_copy_cast(reg,block,sizeof(mtx_region_t));
         return(block_number);
      }
   }
   return(mtx_NONE);
}

int32 mtx_block_containing_col(mtx_matrix_t mtx, int32 col, mtx_region_t *block)
{
   int32 blow,bhigh;
   mtx_region_t *reg;

   if( !mtx_output_assigned(mtx) ) {   /* matrix will be checked */
#if MTX_DEBUG
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_block_containing_col\n");
      FPRINTF(g_mtxerr,"        Matrix not output assigned.\n");
#endif
      return 0;
   }

#if MTX_DEBUG
   if( !mtx_check_blocks(mtx) ) {
      FPRINTF(g_mtxerr,"ERROR:  (mtx) mtx_block_containing_col\n");
      FPRINTF(g_mtxerr,"        Invalid partition.\n");
   }
#endif

   blow = 0;
   bhigh = mtx->data->nblocks-1;
   while( blow <= bhigh ) {
      int32 block_number = (blow+bhigh)/2;
      if( col > mtx->data->block[block_number].col.high ) {
         blow = block_number+1;
      } else if( col < mtx->data->block[block_number].col.low ) {
         bhigh = block_number-1;
      } else {
         reg = &(mtx->data->block[block_number]);
         mem_copy_cast(reg,block,sizeof(mtx_region_t));
         return(block_number);
      }
   }
   return(mtx_NONE);
}

#undef __MTX_C_SEEN__
