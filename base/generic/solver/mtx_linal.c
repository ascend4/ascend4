/*
 *  mtx: Ascend Sparse Matrix Package
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: mtx_linal.c,v $
 *  Date last modified: $Date: 1997/07/28 20:53:03 $
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

#include <math.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/mem.h"
#include "solver/mtx.h"
/* grab our private parts */
#define __MTX_C_SEEN__
#include "solver/mtx_use_only.h"

/*
 * at present these assume the caller knows how long the vectors
 * value and index must be ahead of time, or that they are of size
 * mtx_order, which can get expensive. This may be a good functionality,
 * but it needs some examples of why it should be supported before
 * these are exported properly.
 */
/*********************************************************************\
  Start of New functions added by kaa.		KAA_DEBUG
\*********************************************************************/
/*
 * Export these if they work.
 */
/* they don't, or more to the point, the duplicate the sparse_t functions
 * and are a total waste of bytes. */
#ifdef DELETE_THIS_DEAD_FUNCTION
static int32 mtx_cur_col_to_csr(mtx_matrix_t mtx,
			       int32 col,
			       real64 *value,
			       int32 *index)
{
  struct element_t *elt;
  int32 *tocur;
  int i = 0;

  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  tocur = mtx->perm.row.org_to_cur;

  for( ; NOTNULL(elt); elt = elt->next.row ) {
    index[i] = tocur[elt->row];
    value[i] = elt->value;
    i++;
  }
  return i;
}
#endif  /* DELETE_THIS_DEAD_FUNCTION */

#ifdef DELETE_THIS_DEAD_FUNCTION
static
int32 mtx_cur_col_to_csr_slow(mtx_matrix_t mtx,
				    int32 col,
				    mtx_range_t *rng,
				    real64 *value,
				    int32 *index)
{
  struct element_t *elt;
  int32 *tocur;
  int32 cur;
  int i = 0;

  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  tocur = mtx->perm.row.org_to_cur;

  for( ; NOTNULL(elt); elt = elt->next.row ) {
    cur = tocur[elt->row];
    if (in_range(rng,cur)) {
      index[i] = cur;
      value[i] = elt->value;
      i++;
    }
  }
  return i;
}
#endif  /* DELETE_THIS_DEAD_FUNCTION */

#ifdef DELETE_THIS_DEAD_FUNCTION
static
int32 mtx_cur_row_to_csr(mtx_matrix_t mtx,
			       int32 row,
			       real64 *value,
			       int32 *index)
{
  struct element_t *elt;
  int32 *tocur;
  int i = 0;

  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];

  for( ; NOTNULL(elt); elt = elt->next.col) {
    index[i] = tocur[elt->col];
    value[i] = elt->value;
    i++;
  }
  return i;
}
#endif  /* DELETE_THIS_DEAD_FUNCTION */

#ifdef DELETE_THIS_DEAD_FUNCTION
static
int32 mtx_cur_row_to_csr_slow(mtx_matrix_t mtx,
				    int32 row,
				    mtx_range_t *rng,
				    real64 *value,
				    int32 *index)
{
  struct element_t *elt;
  int32 *tocur;
  int32 cur;
  int i = 0;

  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];

  for( ; NOTNULL(elt); elt = elt->next.col) {
    cur = tocur[elt->col];
    if (in_range(rng,cur)) {
      index[i] = cur;
      value[i] = elt->value;
      i++;
    }
  }
  return i;
}
#endif  /* DELETE_THIS_DEAD_FUNCTION */

/*********************************************************************\
  End of New functions added by kaa.		KAA_DEBUG
\*********************************************************************/


/*
 * mtx_householder_transform_region(mtx,coef,sp,reg,droptol,transpose);
 * add the dot product  coef * u dot Transpose[u] dot A to A.
 * real use: Anew = (I + k u.Transpose[u]) dot A
 * aka A += u dot Transpose[u] dot A.
 * bugs. assumes no incidence outside region in the rows of u.
 * or its a feature: buys a LOT of speed.
 */
#undef DUMELT
#define DUMELT ((struct element_t *)8)
void mtx_householder_transform_region(mtx_matrix_t mtx,
                                      const real64 coef,
                                      const mtx_sparse_t *sp,
                                      const mtx_region_t *reg,
                                      real64 droptol,
                                      boolean transpose)
{
  int32 *ctoorg;
  struct element_t *elt;
  real64 u,mult;
  int32 ku, kr, numcols, kcol;
  register int32 org;

  /* the following are reuseable buffers we must zero before releasing */
  char *mv;               /* mark buffer, should we need it, droptol */
  struct element_t **ev;  /* elt buffer for row addition */
  real64 *hrow;     /* u^T . A */
  int32 *irow;      /* list of nonzeros in hrow */

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  if (coef==D_ZERO) return; /* adding 0 rather silly */
  if (reg==mtx_ENTIRE_MATRIX ||
      reg->col.low > reg->col.high || reg->row.low > reg->row.high) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) Bogus Householder region given.\n");
    return;
  }
  if (sp==NULL) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) Bogus Householder u given.\n");
    return;
  }
  if (sp->len == 0) return; /* I - k00t = I */
  if (ISNULL(sp->idata) || ISNULL(sp->data)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) Bogus Householder u data given.\n");
    return;
  }
  if (droptol != D_ZERO || transpose) {
    FPRINTF(g_mtxerr,
      "Error: (mtx.c) Householder droptol and transpose not done.\n");
    return;
  }
  mv = mtx_null_mark(mtx->order);
  if (ISNULL(mv)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) Householder. Insufficient memory.\n");
    return;
  }
  ev = mtx_null_vector(mtx->order);
  if (ISNULL(ev)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) Householder. Insufficient memory.\n");
    mtx_null_mark_release();
    return;
  }
  hrow = mtx_null_sum(mtx->order);
  if (ISNULL(hrow)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) Householder. Insufficient memory.\n");
    mtx_null_mark_release();
    mtx_null_vector_release();
    return;
  }
  irow = mtx_null_index(reg->col.high - reg->col.low +1);
  if (ISNULL(irow)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) Householder. Insufficient memory.\n");
    mtx_null_mark_release();
    mtx_null_vector_release();
    mtx_null_sum_release();
    return;
  }
  /* are we happy yet? ! */

  ctoorg = mtx->perm.col.cur_to_org;

  /* accumulate the dot products for the stuff in the region.no range check! */
  /* in I- tau u u^T . A this is hrow=u^T.A. idata[ku] is an org row index. */
  for (ku=0; ku < sp->len; ku++) {
    u = sp->data[ku];
    if (u != D_ZERO) {
      elt = mtx->hdr.row[sp->idata[ku]];
      while (NOTNULL(elt)) {
        hrow[elt->col] += u*elt->value; /* 15% */
        elt = elt->next.col;            /* 4% */
      }
    }
  }
  /* accumulate the indices needed to drive zeroing */
  kr = 0;
  for (ku = reg->col.low; ku <= reg->col.high; ku++) {
    org = ctoorg[ku];
    if (hrow[org]!=D_ZERO) {
      irow[kr] = org; /* collect index */
      ev[org] = DUMELT; /* init elt array with bogus ptr */
      kr++;
    }
  }
  /* irow 0..kr-1 now has the org col indexes of nonzero elements in hrow,ev */
  numcols = kr;
  /* now add hh transform to rows of A in u, cases with and without coef = 1 */
  if (coef == D_ONE) {
    for (ku=0; ku < sp->len; ku++) {
      if (sp->data[ku] != D_ZERO) {
        mult = -sp->data[ku];
        org = sp->idata[ku];
        /* expand row of interest into locations of interest */
        elt = mtx->hdr.row[org];
        while (NOTNULL(elt)) {
          if (NOTNULL(ev[elt->col])) ev[elt->col] = elt;   /* 11% */
          elt = elt->next.col;                             /* 4% */
        }
        /* run through irow doing the math (finally) */
        for (kr=0; kr < numcols; kr++) {
          kcol = irow[kr];                             /* 2 */
          if (ev[kcol] > DUMELT) {                     /* 12% */
            ev[kcol]->value += mult*hrow[kcol];        /* 14% */
            ev[kcol] = DUMELT;                         /* 9% */
            /* reinit ev col */
          } else {
            mtx_create_element_value(mtx, org, kcol, mult*hrow[kcol]); /*6*/
            /* let ev[irow[kr]] col stay DUMELT */
          }
        }
      }
    }
  } else {
    for (ku=0; ku < sp->len; ku++) {
      mult = -sp->data[ku]*coef;
      if (mult != D_ZERO) {
        org = sp->idata[ku];
        /* expand row of interest into locations of interest */
        elt = mtx->hdr.row[org];
        while (NOTNULL(elt)) {
          if (NOTNULL(ev[elt->col])) ev[elt->col] = elt;
          elt = elt->next.col;
        }
        /* run through irow doing the math (finally) */
        for (kr=0; kr < numcols; kr++) {
          kcol = irow[kr];
          if (ev[kcol] > DUMELT) {
            ev[kcol]->value += mult*hrow[kcol];
            ev[kcol] = DUMELT;
            /* reinit ev col */
          } else {
            mtx_create_element_value(mtx, org, kcol, mult*hrow[kcol]);
            /* let ev[irow[kr]] col stay DUMELT */
          }
        }
      }
    }
  }
  /* end case coef != 1 */
  for (kr=0; kr < numcols; kr++) {
    ev[irow[kr]] = NULL;
    hrow[irow[kr]] = D_ZERO;
    irow[kr] = 0;
  }
  /* pack up and go home */
  mtx_null_mark_release();
  mtx_null_vector_release();
  mtx_null_sum_release();
  mtx_null_index_release();
  return;
}
#undef DUMELT


/*
 *********************************************************************
 * mtx_eliminate_row.
 *
 * Start of some new routines, so that we can do so micro
 * management of the elimination routines.
 *********************************************************************
 */
struct mtx_linklist {
  int32 index;
  struct mtx_linklist *prev;
};

/*
 * This function assumes that head is not NULL.
 * we dont check !!
 */
#ifdef THIS_IS_AN_UNUSED_FUNCTION
static struct mtx_linklist *insert_link(struct mtx_linklist *head,
					int k,
					mem_store_t eltbuffer)
{
  struct mtx_linklist *ptr1, *ptr2;
  struct mtx_linklist *target;

  target = (struct mtx_linklist *)mem_get_element(eltbuffer);
  target->index = k;

  ptr2 = ptr1 = head;
  ptr1 = head->prev;
  while (ptr1) {
    if (ptr1->index < k) {
      target->prev = ptr1;
      ptr2->prev = target;
      return target;
    }
    ptr2 = ptr1;
    ptr1 = ptr1->prev;
  }
  /*
   * If we are here then we reached the end of
   * the chain. Just add target and quit.
   */
  target->prev = ptr1;
  ptr2->prev = target;
  return target;
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

/*
 * head may be NULL for this function. We simply add
 * the new index *unsorted* and return the end of top
 * of the chain.
 */
#ifdef THIS_IS_AN_UNUSED_FUNCTION
static struct mtx_linklist *add_link(struct mtx_linklist *head,
				     int k,
				     mem_store_t eltbuffer)
{
  struct mtx_linklist *target;

  target = (struct mtx_linklist *)mem_get_element(eltbuffer);
  target->index = k;

  target->prev = head;
  return target;
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

/*
 * NOTE: This function needs a droptol as a parameter
 */

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
void mtx_eliminate_row2(mtx_matrix_t mtx,
			mtx_matrix_t upper_mtx,
			mtx_range_t *rng,
			int32 row,
			real64 *vec,
			real64 *pivots,
			int32 *inserted,
			mem_store_t eltbuffer)
{
  struct element_t *elt;
  struct mtx_linklist *diag, *right=NULL, *ptr;
  int32 *tocur;
  real64 multiplier;
  int k, j;
  mtx_coord_t nz;

  (void)rng;  /*  stop gcc whine about unused parameter  */

  /*
   * Move all non-zeros from current row to full array.
   * The full array would have been initialized before,
   * we must put it back in the clean state when we leave.
   * All operations are done over mtx_ALL_COLS.
   */
  /*
   * we use the following code fragment instead of :
   * 	mtx_cur_row_vec(mtx,row,vec,mtx_ALL_COLS);
   * so that we can put the elements in the correct place.
   */

  diag = (struct mtx_linklist *)mem_get_element(eltbuffer);
  diag->index = row;
  diag->prev = NULL;
  inserted[row] = 1;

  /*
   * the insertion for this phase.
   */
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  ptr = diag;
  for ( ;NOTNULL(elt); elt = elt->next.col) {
    if (elt->value == 0.0) {
      continue;			/* hard zeros */
    }
    k = tocur[elt->col];
    vec[k] = elt->value;
    if (k < row) {		/* the less than is critical */
      (void)insert_link(diag,k,eltbuffer);
      inserted[k] = 1;
    }
    else if (k > row) {
      right = add_link(right,k,eltbuffer);
      inserted[k] = 1;
    }
    else
      continue;
  }

  mtx_clear_row(mtx,row,mtx_ALL_COLS);

  /* we shuold be trapping for these 0 multipliers before. !!! */

  for (ptr = diag->prev; NOTNULL(ptr); ) {
    k = ptr->index;
    multiplier = vec[k]/pivots[k];
    if (multiplier==D_ZERO) {
      ptr = ptr->prev;
      /* FPRINTF(stderr,"0 multiplier found at %d\n",k); */
      continue;
    }
#ifdef NOP_DEBUG
    mtx_number_ops++;
#endif /* NOP_DEBUG */
    elt = mtx->hdr.row[mtx->perm.row.cur_to_org[k]];
    for ( ;NOTNULL(elt); elt = elt->next.col) {
      j = tocur[elt->col];
      if (!inserted[j]) {
	if (j < k)
	  (void)insert_link(ptr,j,eltbuffer);
	else
	  right = add_link(right,j,eltbuffer);
	inserted[j] = 1;
      }
      vec[j] = vec[j] - multiplier * elt->value;
#ifdef NOP_DEBUG
    mtx_number_ops++;
#endif /* NOP_DEBUG */
    }
    vec[k] = multiplier;	/* backpatch multiplier */
    ptr = ptr->prev;
  }

  /*
   * Map the data back to the appropriate matrices.
   */

  /*
   * Fill up the upper_matrix with the multipliers.
   */
  nz.row = row;
  for (ptr = diag->prev; NOTNULL(ptr); ptr = ptr->prev) {
    nz.col = ptr->index;
    if (vec[nz.col] != D_ZERO) {		/* dont fill hard 0's */
      mtx_fill_value(upper_mtx,&nz,vec[nz.col]);
    }
    vec[nz.col] = D_ZERO;
    inserted[nz.col] = 0;
  }

  /*
   * Fill the diagonal back to the regular matrix.
   */
  nz.col = row;
  if (vec[nz.col] != D_ZERO) {			/* dont fill hard 0's */
    mtx_fill_value(mtx,&nz,vec[nz.col]);
  }
  vec[row] = D_ZERO;
  inserted[row] = 0;

  /*
   * Fill the lower matrix with the stuff to the right of
   * diagonal.
   */
  for (ptr = right; NOTNULL(ptr); ptr = ptr->prev) {
    nz.col = ptr->index;
    if (fabs(vec[nz.col]) > 1.0e-16) {	/* THIS NEEDS A DROP TOL DEFINE */
      mtx_fill_value(mtx,&nz,vec[nz.col]);
    }
    vec[nz.col] = D_ZERO;
    inserted[nz.col] = 0;
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

#undef __MTX_C_SEEN__
