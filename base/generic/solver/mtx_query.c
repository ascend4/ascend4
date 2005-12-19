/*
 *  mtx: Ascend Sparse Matrix Package
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: mtx_query.c,v $
 *  Date last modified: $Date: 1997/07/28 20:53:07 $
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
#include "general/mathmacros.h"

real64 mtx_next_in_row( mtx_matrix_t mtx, mtx_coord_t *coord, mtx_range_t *rng)
{
   static struct element_t *elt = NULL;
   struct element_t Rewind;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
   if( coord->col == mtx_FIRST ) {
      if (not_in_range(0,mtx->order-1,coord->row)) {
        /* coord->col = mtx_LAST; already true */
        return D_ZERO;
      }
      Rewind.next.col = mtx->hdr.row[mtx->perm.row.cur_to_org[coord->row]];
      elt = mtx_next_col(&Rewind,rng,mtx->perm.col.org_to_cur);
   } else elt = mtx_next_col(elt,rng,mtx->perm.col.org_to_cur);
   coord->col = (ISNULL(elt)) ? mtx_LAST : mtx->perm.col.org_to_cur[elt->col];
   return( ISNULL(elt) ? D_ZERO : elt->value );
}

real64 mtx_next_in_col( mtx_matrix_t mtx, mtx_coord_t *coord, mtx_range_t *rng)
{
   static struct element_t *elt = NULL;
   struct element_t Rewind;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return 0;
#endif
   if( coord->row == mtx_FIRST ) {
      if (not_in_range(0,mtx->order-1,coord->col)) {
        /* coord->row = mtx_LAST; already true */
        return D_ZERO;
      }
      Rewind.next.row = mtx->hdr.col[mtx->perm.col.cur_to_org[coord->col]];
      elt = mtx_next_row(&Rewind,rng,mtx->perm.row.org_to_cur);
   } else elt = mtx_next_row(elt,rng,mtx->perm.row.org_to_cur);
   coord->row = (ISNULL(elt)) ? mtx_LAST : mtx->perm.row.org_to_cur[elt->row];
   return( ISNULL(elt) ? D_ZERO : elt->value );
}

real64 mtx_row_max( mtx_matrix_t mtx, mtx_coord_t *coord, mtx_range_t *rng, real64 *val)
{
  struct element_t *elt,*maxelt;
  real64 maxabs;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return (double)-1.0;
#endif
  maxabs = D_ZERO;
  maxelt = NULL;
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[coord->row]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      if( fabs(elt->value) < maxabs ) {
        continue; /* skip uninteresting elements */
      }
      if (ISNULL(maxelt)) { /* first element found */
        maxelt = elt;
        maxabs = fabs(elt->value);
      } else { /* tie breakers: elt >= maxelt */
        if( fabs(elt->value) > maxabs /* bigger */ ||
          tocur[elt->col] < tocur[maxelt->col] /* nearer */ ) {
          maxelt = elt;
          maxabs = fabs(elt->value);
        }
      }
    }
  } else {
    if( rng->high >= rng->low ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        if( in_range(rng,tocur[elt->col]) ) {
          if( fabs(elt->value) < maxabs ) {
            continue; /* skip uninteresting elements */
          }
          if (ISNULL(maxelt)) { /* first element found */
            maxelt = elt;
            maxabs = fabs(elt->value);
          } else { /* tie breakers: elt >= maxelt */
            if( fabs(elt->value) > maxabs /* bigger */ ||
              tocur[elt->col] < tocur[maxelt->col] /* nearer */ ) {
              maxelt = elt;
              maxabs = fabs(elt->value);
            }
          }
        }
      }
    }
  }

  coord->col = (ISNULL(maxelt)) ? mtx_NONE : tocur[maxelt->col];
  if (val) {
    if (maxelt) *val=maxelt->value; else *val=D_ZERO;
  }
  return(maxabs);
}

real64 mtx_col_max( mtx_matrix_t mtx, mtx_coord_t *coord, mtx_range_t *rng, real64 *val)
{
  struct element_t *elt,*maxelt;
  real64 maxabs;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return (double)-1.0;
#endif
  maxabs = D_ZERO;
  maxelt = NULL;
  tocur = mtx->perm.row.org_to_cur;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[coord->col]];
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      if( fabs(elt->value) < maxabs ) {
        continue; /* skip uninteresting elements */
      }
      if (ISNULL(maxelt)) { /* first element found */
        maxelt = elt;
        maxabs = fabs(elt->value);
      } else { /* tie breakers: elt >= maxelt */
        if( fabs(elt->value) > maxabs /* bigger */ ||
          tocur[elt->row] < tocur[maxelt->row] /* nearer */ ) {
          maxelt = elt;
          maxabs = fabs(elt->value);
        }
      }
    }
  } else {
    if( rng->high >= rng->low ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        if( in_range(rng,tocur[elt->row]) ) {
          if( fabs(elt->value) < maxabs ) {
            continue; /* skip uninteresting elements */
          }
          if (ISNULL(maxelt)) { /* first element found */
            maxelt = elt;
            maxabs = fabs(elt->value);
          } else { /* tie breakers: elt >= maxelt */
            if( fabs(elt->value) > maxabs /* bigger */ ||
              tocur[elt->row] < tocur[maxelt->row] /* nearer */ ) {
              maxelt = elt;
              maxabs = fabs(elt->value);
            }
          }
        }
      }
    }
  }

  coord->row = (ISNULL(maxelt)) ? mtx_NONE : tocur[maxelt->row];
  if (val) {
    if (maxelt) *val=maxelt->value; else *val=D_ZERO;
  }
  return(maxabs);
}

real64 mtx_row_min( mtx_matrix_t mtx, mtx_coord_t *coord,
		   mtx_range_t *rng, real64 *val, real64 minval)
{
  struct element_t *elt,*minelt;
  real64 minabs;
  real64 big_num = 1e50;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return (double)-1.0;
#endif
  minabs = big_num;
  minelt = NULL;
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[coord->row]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      if( fabs(elt->value) > minabs ) {
        continue; /* skip uninteresting elements */
      }
      if( (fabs(elt->value) < minabs /* smaller */ ||
	 tocur[elt->col] < tocur[minelt->col] /* nearer */ )
	 && (fabs(elt->value) > minval)) {
	minelt = elt;
	minabs = fabs(elt->value);
      }
    }
  } else {
    if( rng->high >= rng->low ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        if( in_range(rng,tocur[elt->col]) ) {
          if( fabs(elt->value) > minabs ) {
            continue; /* skip uninteresting elements */
          }
	  if( (fabs(elt->value) < minabs /* smaller */ ||
	     tocur[elt->col] < tocur[minelt->col] /* nearer */ )
	     && (fabs(elt->value) > minval)) {
	    minelt = elt;
	    minabs = fabs(elt->value);
	  }
        }
      }
    }
  }
  if(minabs == big_num){
    coord->col = mtx_NONE;
    if (val) {
      *val=0;
    }
    minabs = 1;
  } else {
    coord->col = (ISNULL(minelt)) ? mtx_NONE : tocur[minelt->col];
    if (val) {
      *val=minelt->value;
    }
  }
  return(minabs);
}

real64 mtx_col_min( mtx_matrix_t mtx, mtx_coord_t *coord,
		   mtx_range_t *rng, real64 *val, real64 minval)
{
  struct element_t *elt,*minelt;
  real64 minabs;
  real64 big_num = 1e50;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return (double)-1.0;
#endif
  minabs = big_num;
  minelt = NULL;
  tocur = mtx->perm.row.org_to_cur;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[coord->col]];
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      if( fabs(elt->value) > minabs ) {
        continue; /* skip uninteresting elements */
      }
      if( (fabs(elt->value) < minabs /* smaller */ ||
	 tocur[elt->row] < tocur[minelt->row] /* nearer */ )
	 && (fabs(elt->value) > minval)) {
	minelt = elt;
	minabs = fabs(elt->value);
      }
    }
  } else {
    if( rng->high >= rng->low ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        if( in_range(rng,tocur[elt->row]) ) {
          if( fabs(elt->value) > minabs ) {
            continue; /* skip uninteresting elements */
          }
	  if( (fabs(elt->value) < minabs /* smaller */ ||
	       tocur[elt->row] < tocur[minelt->row] /* nearer */ )
	     && (fabs(elt->value) > minval)) {
	    minelt = elt;
	    minabs = fabs(elt->value);
          }
        }
      }
    }
  }
  if(minabs == big_num){
    coord->col = mtx_NONE;
    if (val) {
      *val=0;
    }
    minabs = 1;
  } else {
    coord->row = (ISNULL(minelt)) ? mtx_NONE : tocur[minelt->row];
    if (val) {
      *val=minelt->value;
    }
  }
  return(minabs);
}

real64 mtx_get_pivot_col( mtx_matrix_t mtx, mtx_coord_t *coord,
                                mtx_range_t *rng, real64 *val,
                                real64 ptol, real64 eps)
{
  struct element_t *elt;
  real64 rmax, thold;
  real64 *rvec;
  int32 *tocur;
  int32 *ivec;
  char *mark;
  int32 space,j, k,len,pivot;


#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return (double)-1.0;
#endif
  if (rng == mtx_ALL_COLS) {
    space = mtx->order;
  } else {
    space = rng->high - rng->low +1;
  }
  if (space <1 || coord->row < 0 || coord->row >= mtx->order) {
    /* empty or inverted range is bogus */
    coord->col = mtx_NONE;
    return D_ZERO;
  }
  mark = mtx_null_mark(space);
  if (ISNULL(mark)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) mtx_get_pivot_col. Insufficient memory.\n");
    coord->col = mtx_NONE;
    return D_ZERO;
  }
  rvec = mtx_null_sum(space);
  if (ISNULL(rvec)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) mtx_get_pivot_col. Insufficient memory.\n");
    mtx_null_mark_release();
    coord->col = mtx_NONE;
    return D_ZERO;
  }
  ivec = mtx_null_index(space);
  if (ISNULL(ivec)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) mtx_get_pivot_col. Insufficient memory.\n");
    mtx_null_mark_release();
    mtx_null_sum_release();
    coord->col = mtx_NONE;
    return D_ZERO;
  }
  /* we're happy now, init search */

  rmax = D_ZERO;
  thold = eps = fabs(eps);
  len = 0;
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[coord->row]];

  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      if( (rvec[len] = fabs(elt->value) ) > thold ) {
        mark[len] = (elt->value < D_ZERO);
        ivec[len] = tocur[elt->col];
        if (rvec[len] > rmax) {
          rmax = rvec[len];
          thold = MAX(rvec[len]*ptol,eps);
        }
        len++;
      }
    }
    if (!len) {
      /* found nothing > eps */
      coord->col = mtx_NONE;
      rvec[len] = D_ZERO; /* rezero the only buff we've touched. */
      mtx_null_mark_release();
      mtx_null_sum_release();
      mtx_null_index_release();
      return D_ZERO;
    }
    k = mtx->order;
  } else {
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      if( in_range(rng,tocur[elt->col]) ) {
        if( (rvec[len] = fabs(elt->value) ) > thold ) {
          mark[len] = (elt->value < D_ZERO);
          ivec[len] = tocur[elt->col];
          if (rvec[len] > rmax) {
            rmax = rvec[len];
            thold = MAX(rvec[len]*ptol,eps);
          }
          len++;
        }
      }
    }
    if (!len) {
      /* found nothing > eps */
      coord->col = mtx_NONE;
      rvec[len] = D_ZERO; /* rezero the only buff we've touched. */
      mtx_null_mark_release();
      mtx_null_sum_release();
      mtx_null_index_release();
      return D_ZERO;
    }
    k = rng->high+1;
  }

  /* thold is now ptol * maxabsinrow, and k 1 past the right edge of rng. */
  /* k becomes the nz.col we're after, pivot is the rvec/ivec location of k */
  for (j = 0, pivot = k; j < len; j++) {
    if (rvec[j] >= thold && ivec[j] < k ) {
      k = ivec[j];
      pivot = j;
    }
  }
  coord->col = k;            /* set col return */
  rmax = rvec[pivot];        /* set return number */
  if (val) {                 /* set val return */
    if (mark[pivot]) {
      *val = -rvec[pivot];
    } else {
      *val = rvec[pivot];
    }
  }
  /* clean up buffers */
  mtx_zero_real64(rvec,len);
  mtx_zero_int32(ivec,len);
  mtx_zero_char(mark,len);
  mtx_null_mark_release();
  mtx_null_sum_release();
  mtx_null_index_release();
  /* quit */
  return(rmax);
}

real64 mtx_get_pivot_row( mtx_matrix_t mtx, mtx_coord_t *coord,
                                mtx_range_t *rng, real64 *val,
                                real64 ptol, real64 eps)
{
  struct element_t *elt;
  real64 rmax, thold;
  real64 *rvec;
  int32 *tocur;
  int32 *ivec;
  char *mark;
  int32 space,j, k,len,pivot;


#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return (double)-1.0;
#endif
  if (rng == mtx_ALL_ROWS) {
    space = mtx->order;
  } else {
    space = rng->high - rng->low +1;
  }
  if (space <1 || coord->col < 0 || coord->col >= mtx->order) {
    /* empty or inverted range is bogus */
    coord->row = mtx_NONE;
    return D_ZERO;
  }
  mark = mtx_null_mark(space);
  if (ISNULL(mark)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) mtx_get_pivot_row. Insufficient memory.\n");
    coord->row = mtx_NONE;
    return D_ZERO;
  }
  rvec = mtx_null_sum(space);
  if (ISNULL(rvec)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) mtx_get_pivot_row. Insufficient memory.\n");
    mtx_null_mark_release();
    coord->row = mtx_NONE;
    return D_ZERO;
  }
  ivec = mtx_null_index(space);
  if (ISNULL(ivec)) {
    FPRINTF(g_mtxerr,"Error: (mtx.c) mtx_get_pivot_row. Insufficient memory.\n");
    mtx_null_mark_release();
    mtx_null_sum_release();
    coord->row = mtx_NONE;
    return D_ZERO;
  }
  /* we're happy now, init search */

  rmax = D_ZERO;
  thold = eps = fabs(eps);
  len = 0;
  tocur = mtx->perm.row.org_to_cur;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[coord->col]];

  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      if( (rvec[len] = fabs(elt->value) ) > thold ) {
        mark[len] = (elt->value < D_ZERO);
        ivec[len] = tocur[elt->row];
        if (rvec[len] > rmax) {
          rmax = rvec[len];
          thold = MAX(rvec[len]*ptol,eps);
        }
        len++;
      }
    }
    if (!len) {
      /* found nothing > eps */
      coord->row = mtx_NONE;
      rvec[len] = D_ZERO; /* rezero the only buff we've touched. */
      mtx_null_mark_release();
      mtx_null_sum_release();
      mtx_null_index_release();
      return D_ZERO;
    }
    k = mtx->order;
  } else {
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      if( in_range(rng,tocur[elt->row]) ) {
        if( (rvec[len] = fabs(elt->value) ) > thold ) {
          mark[len] = (elt->value < D_ZERO);
          ivec[len] = tocur[elt->row];
          if (rvec[len] > rmax) {
            rmax = rvec[len];
            thold = MAX(rvec[len]*ptol,eps);
          }
          len++;
        }
      }
    }
    if (!len) {
      /* found nothing > eps */
      coord->row = mtx_NONE;
      rvec[len] = D_ZERO; /* rezero the only buff we've touched. */
      mtx_null_mark_release();
      mtx_null_sum_release();
      mtx_null_index_release();
      return D_ZERO;
    }
    k = rng->high+1;
  }

  /* thold is now ptol * maxabsincol, and k 1 past the bottom edge of rng. */
  /* k becomes the nz.col we're after, pivot is the rvec/ivec location of k */
  for (j = 0, pivot = k; j < len; j++) {
    if (rvec[j] >= thold && ivec[j] < k ) {
      k = ivec[j];
      pivot = j;
    }
  }
  coord->row = k;            /* set row return */
  rmax = rvec[pivot];        /* set return number */
  if (val) {                 /* set val return */
    if (mark[pivot]) {
      *val = -rvec[pivot];
    } else {
      *val = rvec[pivot];
    }
  }
  /* clean up buffers */
  mtx_zero_real64(rvec,len);
  mtx_zero_int32(ivec,len);
  mtx_zero_char(mark,len);
  mtx_null_mark_release();
  mtx_null_sum_release();
  mtx_null_index_release();
  /* quit */
  return(rmax);
}

int32 mtx_nonzeros_in_row( mtx_matrix_t mtx, int32 row, mtx_range_t *rng)
{
   struct element_t *elt;
   int32 *tocur;
   int32 count;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return mtx_NONE;
#endif
   count = ZERO;
   tocur = mtx->perm.col.org_to_cur;
   elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
   if( rng == mtx_ALL_COLS )
      for( ; NOTNULL(elt); elt = elt->next.col ) ++count;
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.col )
         if( in_range(rng,tocur[elt->col]) ) ++count;

   return(count);
}

int32 mtx_nonzeros_in_col( mtx_matrix_t mtx, int32 col, mtx_range_t *rng)
{
   struct element_t *elt;
   int32 *tocur;
   int32 count;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return mtx_NONE;
#endif
   count = ZERO;
   tocur = mtx->perm.row.org_to_cur;
   elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
   if( rng == mtx_ALL_ROWS )
      for( ; NOTNULL(elt); elt = elt->next.row ) ++count;
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.row )
         if( in_range(rng,tocur[elt->row]) ) ++count;

   return(count);
}

int32 mtx_nonzeros_in_region(mtx_matrix_t mtx,mtx_region_t *reg)
{
  struct element_t *elt;
  int32 count;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return mtx_NONE;
#endif
  count = ZERO;
  if( reg == mtx_ENTIRE_MATRIX ) {
    int32 org_row;
    for( org_row = ZERO ; org_row < mtx->order ; org_row++)
      for (elt = mtx->hdr.row[org_row] ; NOTNULL(elt) ; elt = elt->next.col)
        count++;
  } else {
    if( reg->row.high >= reg->row.low  && reg->col.high >= reg->col.low) {
      int32 cur_row,rlim;
      int32 *toorg, *tocur;
      mtx_range_t *rng;
      tocur = mtx->perm.col.org_to_cur;
      toorg = mtx->perm.row.cur_to_org;
      rlim=reg->row.high;
      rng=&(reg->col);
      for (cur_row=reg->row.low; cur_row<=rlim; cur_row++) {
        elt = mtx->hdr.row[toorg[cur_row]];
        for( ; NOTNULL(elt); elt = elt->next.col )
          if( in_range(rng,tocur[elt->col]) ) ++count;
      }
    }
  }
  return(count);
}


int32 mtx_numbers_in_row( mtx_matrix_t mtx, int32 row, mtx_range_t *rng)
{
  struct element_t *elt;
  int32 *tocur;
  int32 count;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return mtx_NONE;
#endif
  count = ZERO;
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      if (elt->value != D_ZERO) ++count;
    }
  } else {
    if( rng->high >= rng->low ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        if( in_range(rng,tocur[elt->col]) && elt->value!= D_ZERO ) ++count;
      }
    }
  }
  return(count);
}

int32 mtx_numbers_in_col( mtx_matrix_t mtx, int32 col, mtx_range_t *rng)
{
  struct element_t *elt;
  int32 *tocur;
  int32 count;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return mtx_NONE;
#endif
  count = ZERO;
  tocur = mtx->perm.row.org_to_cur;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      if (elt->value != D_ZERO)
        ++count;
    }
  } else {
    if( rng->high >= rng->low ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        if( in_range(rng,tocur[elt->row]) && elt->value!= D_ZERO )
          ++count;
      }
    }
  }
  return(count);
}

int32 mtx_numbers_in_region(mtx_matrix_t mtx,mtx_region_t *reg)
{
  struct element_t *elt;
  int32 count;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return mtx_NONE;
#endif
  count = ZERO;
  if( reg == mtx_ENTIRE_MATRIX ) {
    int32 org_row;
    for( org_row = ZERO ; org_row < mtx->order ; org_row++)
      for (elt = mtx->hdr.row[org_row] ; NOTNULL(elt) ; elt = elt->next.col)
        if (elt->value!=D_ZERO) count++;
  } else {
    if( reg->row.high >= reg->row.low  && reg->col.high >= reg->col.low) {
      int32 cur_row,rlim;
      int32 *toorg, *tocur;
      mtx_range_t *rng;
      tocur = mtx->perm.col.org_to_cur;
      toorg = mtx->perm.row.cur_to_org;
      rlim=reg->row.high;
      rng=&(reg->col);
      for (cur_row=reg->row.low; cur_row<=rlim; cur_row++) {
        elt = mtx->hdr.row[toorg[cur_row]];
        for( ; NOTNULL(elt); elt = elt->next.col )
          if( in_range(rng,tocur[elt->col]) && elt->value!=D_ZERO ) ++count;
      }
    }
  }
  return(count);
}

static real64 msqr(register real64 d) {
  return d*d;
}

void mtx_org_row_vec(mtx_matrix_t mtx, int32 row,
                     real64 *vec, mtx_range_t *rng)
{
  struct element_t *elt;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col )
      vec[elt->col]=elt->value;
    return;
  }
  if( rng->high >= rng->low ) {
    int32 *tocur;
    register int32 org_col, cur_col;
    tocur = mtx->perm.col.org_to_cur;
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      cur_col=tocur[(org_col = elt->col)];
      if( in_range(rng,cur_col) )
        vec[org_col]=elt->value;
    }
  }
}

void mtx_org_col_vec(mtx_matrix_t mtx, int32 col,
                     real64 *vec,mtx_range_t *rng)
{
  struct element_t *elt;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row )
      vec[elt->row]=elt->value;
    return;
  }
  if( rng->high >= rng->low ) {
    int32 *tocur;
    register int32 org_row, cur_row;
    tocur = mtx->perm.row.org_to_cur;
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      cur_row=tocur[(org_row = elt->row)];
      if( in_range(rng,cur_row) )
        vec[org_row]=elt->value;
    }
  }
}

void mtx_cur_row_vec(mtx_matrix_t mtx, int32 row,
                     real64 *vec, mtx_range_t *rng)
{
  struct element_t *elt;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col )
      vec[tocur[elt->col]]=elt->value;
    return;
  }
  if( rng->high >= rng->low ) {
    register int32 cur_col;
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      cur_col=tocur[elt->col];
      if( in_range(rng,cur_col) )
        vec[cur_col]=elt->value;
    }
  }
}

void mtx_cur_col_vec(mtx_matrix_t mtx, int32 col,
                     real64 *vec,mtx_range_t *rng)
{
  struct element_t *elt;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  tocur = mtx->perm.row.org_to_cur;
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row )
      vec[tocur[elt->row]]=elt->value;
    return;
  }
  if( rng->high >= rng->low ) {
    register int32 cur_row;
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      cur_row=tocur[elt->row];
      if( in_range(rng,cur_row) )
        vec[cur_row]=elt->value;
    }
  }
}

mtx_sparse_t *mtx_org_row_sparse(mtx_matrix_t mtx, int32 row,
                                 mtx_sparse_t * const vec, mtx_range_t *rng,
                                 int zeroes)
{
  struct element_t *elt;
  int32 k,cap;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(vec)) return NULL;
#endif
  if (row < 0 || row >= mtx->order)  {
    FPRINTF(g_mtxerr,"ERROR: (mtx_org_row_sparse) Row out of range.\n");
    return NULL;
  }
  cap = vec->cap;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if( rng == mtx_ALL_COLS ) {
    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.col ) {
        if (elt->value != D_ZERO) {
          vec->data[k] = elt->value;
          vec->idata[k] = elt->col;
          k++;
        }
      }
    } else {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.col ) {
        vec->data[k] = elt->value;
        vec->idata[k] = elt->col;
        k++;
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  }
  if( rng->high >= rng->low ) {
    int32 *tocur;
    register int32 cur_col;

    tocur = mtx->perm.col.org_to_cur;
    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.col ) {
        if (elt->value != D_ZERO) {
          cur_col = tocur[elt->col];
          if( in_range(rng,cur_col) ) {
            vec->idata[k] = elt->col;
            vec->data[k] = elt->value;
            k++;
          }
        }
      }
    } else {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.col ) {
        cur_col = tocur[elt->col];
        if( in_range(rng,cur_col) ) {
          vec->idata[k] = elt->col;
          vec->data[k] = elt->value;
          k++;
        }
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  } else { /* nothing can exist in backward range. */
    vec->len = 0;
    return vec;
  }
}

mtx_sparse_t *mtx_org_col_sparse(mtx_matrix_t mtx, int32 col,
                                 mtx_sparse_t * const vec, mtx_range_t *rng,
                                 int zeroes)
{
  struct element_t *elt;
  int32 k,cap;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(vec)) return NULL;
#endif
  if (col < 0 || col >= mtx->order)  {
    FPRINTF(g_mtxerr,"ERROR: (mtx_org_col_sparse) Col out of range.\n");
    return NULL;
  }
  cap = vec->cap;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  if( rng == mtx_ALL_ROWS ) {
    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.row ) {
        if (elt->value != D_ZERO) {
          vec->data[k] = elt->value;
          vec->idata[k] = elt->row;
          k++;
        }
      }
    } else {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.row ) {
        vec->data[k] = elt->value;
        vec->idata[k] = elt->row;
        k++;
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  }
  if( rng->high >= rng->low ) {
    int32 *tocur;
    register int32 cur_row;

    tocur = mtx->perm.row.org_to_cur;
    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.row ) {
        if (elt->value != D_ZERO) {
          cur_row = tocur[elt->row];
          if( in_range(rng,cur_row) ) {
            vec->idata[k] = elt->row;
            vec->data[k] = elt->value;
            k++;
          }
        }
      }
    } else {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.row ) {
        cur_row = tocur[elt->row];
        if( in_range(rng,cur_row) ) {
          vec->idata[k] = elt->row;
          vec->data[k] = elt->value;
          k++;
        }
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  } else { /* nothing can exist in backward range. */
    vec->len = 0;
    return vec;
  }
}

mtx_sparse_t *mtx_cur_row_sparse(mtx_matrix_t mtx, int32 row,
                                 mtx_sparse_t * const vec, mtx_range_t *rng,
                                 int zeroes)
{
  int32 *tocur;
  const struct element_t *elt;
  int32 cap,k;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(vec)) return NULL;
#endif
  if (row < 0 || row >= mtx->order)  {
    FPRINTF(g_mtxerr,"ERROR: (mtx_cur_row_sparse) Row out of range.\n");
    return NULL;
  }
  tocur = mtx->perm.col.org_to_cur;
  cap = vec->cap;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if( rng == mtx_ALL_COLS ) {
    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt) && k < cap ; elt = elt->next.col ) {
        if (elt->value != D_ZERO) {
          vec->data[k] = elt->value;
          vec->idata[k] = tocur[elt->col];
          k++;
        }
      }
    } else {
      for(k=0; NOTNULL(elt) && k < cap ; elt = elt->next.col ) {
        vec->data[k] = elt->value;
        vec->idata[k] = tocur[elt->col];
        k++;
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  }
  /* range is not ALL */
  if( rng->high >= rng->low ) {
    register int32 cur_col;

    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.col ) {
        if (elt->value != D_ZERO) {
          cur_col = tocur[elt->col];
          if( in_range(rng,cur_col) ) {
            vec->idata[k] = cur_col;
            vec->data[k] = elt->value;
            k++;
          }
        }
      }
    } else {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.col ) {
        cur_col = tocur[elt->col];
        if( in_range(rng,cur_col) ) {
          vec->idata[k] = cur_col;
          vec->data[k] = elt->value;
          k++;
        }
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  } else { /* nothing can exist in backward range. set len if needed. */
    vec->len = 0;
    return vec;
  }
}

mtx_sparse_t *mtx_cur_col_sparse(mtx_matrix_t mtx, int32 col,
                                 mtx_sparse_t * const vec,mtx_range_t *rng,
                                 int zeroes)
{
  int32 *tocur;
  const struct element_t *elt;
  int32 cap,k;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx) || !mtx_check_sparse(vec)) return NULL;
#endif
  if (col < 0 || col >= mtx->order)  {
    FPRINTF(g_mtxerr,"ERROR: (mtx_cur_col_sparse) Col out of range.\n");
    return NULL;
  }
  tocur = mtx->perm.row.org_to_cur;
  cap = vec->cap;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  if( rng == mtx_ALL_ROWS ) {
    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt); elt = elt->next.row ) {
        if (elt->value != D_ZERO) {
          vec->data[k] = elt->value;
          vec->idata[k] = tocur[elt->row];
          k++;
        }
      }
    } else {
      for(k=0; NOTNULL(elt); elt = elt->next.row ) {
        vec->data[k] = elt->value;
        vec->idata[k] = tocur[elt->row];
        k++;
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  }
  if( rng->high >= rng->low ) {
    register int32 cur_row;

    if (zeroes == mtx_IGNORE_ZEROES) {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.row ) {
        if (elt->value != D_ZERO) {
          cur_row = tocur[elt->row];
          if( in_range(rng,cur_row) ) {
            vec->idata[k] = cur_row;
            vec->data[k] = elt->value;
            k++;
          }
        }
      }
    } else {
      for(k=0; NOTNULL(elt) && k < cap; elt = elt->next.row ) {
        cur_row = tocur[elt->row];
        if( in_range(rng,cur_row) ) {
          vec->idata[k] = cur_row;
          vec->data[k] = elt->value;
          k++;
        }
      }
    }
    vec->len = k;
    if (ISNULL(elt)) {
      return vec;
    }
    return NULL; /* ran out of vector */
  } else { /* nothing can exist in backward range. set len if needed. */
    vec->len = 0;
    return vec;
  }
}

void mtx_zr_org_vec_using_row(mtx_matrix_t mtx,int32 row,
                              real64 *vec,mtx_range_t *rng)
{
  struct element_t *elt;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col )
      vec[elt->col]=D_ZERO;
    return;
  }
  if( rng->high >= rng->low ) {
    register int32 org_col, cur_col;
    int32 *tocur;
    tocur = mtx->perm.col.org_to_cur;
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      cur_col=tocur[(org_col = elt->col)];
      if( in_range(rng,cur_col) )
        vec[org_col]=D_ZERO;
    }
  }
}

void mtx_zr_org_vec_using_col(mtx_matrix_t mtx, int32 col,
                              real64 *vec,mtx_range_t *rng)
{
  struct element_t *elt;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row )
      vec[elt->row]=D_ZERO;
    return;
  }
  if( rng->high >= rng->low ) {
    register int32 org_row, cur_row;
    int32 *tocur;
    tocur = mtx->perm.row.org_to_cur;
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      cur_row=tocur[(org_row = elt->row)];
      if( in_range(rng,cur_row) )
        vec[org_row]=D_ZERO;
    }
  }
}

void mtx_zr_cur_vec_using_row(mtx_matrix_t mtx,int32 row,
                              real64 *vec,mtx_range_t *rng)
{
  struct element_t *elt;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if( rng == mtx_ALL_COLS ) {
    for( ; NOTNULL(elt); elt = elt->next.col )
      vec[tocur[elt->col]]=D_ZERO;
    return;
  }
  if( rng->high >= rng->low ) {
    register int32 cur_col;
    for( ; NOTNULL(elt); elt = elt->next.col ) {
      cur_col=tocur[elt->col];
      if( in_range(rng,cur_col) )
        vec[cur_col]=D_ZERO;
    }
  }
}

void mtx_zr_cur_vec_using_col(mtx_matrix_t mtx, int32 col,
                              real64 *vec,mtx_range_t *rng)
{
  struct element_t *elt;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  tocur = mtx->perm.row.org_to_cur;
  if( rng == mtx_ALL_ROWS ) {
    for( ; NOTNULL(elt); elt = elt->next.row )
      vec[tocur[elt->row]]=D_ZERO;
    return;
  }
  if( rng->high >= rng->low ) {
    register int32 cur_row;
    for( ; NOTNULL(elt); elt = elt->next.row ) {
      cur_row=tocur[elt->row];
      if( in_range(rng,cur_row) )
        vec[cur_row]=D_ZERO;
    }
  }
}

real64 mtx_sum_sqrs_in_row( mtx_matrix_t mtx, int32 row, const mtx_range_t *rng)
{
   struct element_t *elt;
   int32 *tocur;
   register real64 sum;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
   sum = D_ZERO;
   tocur = mtx->perm.col.org_to_cur;
   elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
   if( rng == mtx_ALL_COLS )
      for( ; NOTNULL(elt); elt = elt->next.col ) sum += msqr(elt->value);
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.col )
         if( in_range(rng,tocur[elt->col]) ) sum += msqr(elt->value);

   return(sum);
}

real64 mtx_sum_sqrs_in_col( mtx_matrix_t mtx, int32 col, const mtx_range_t *rng)
{
   struct element_t *elt;
   int32 *tocur;
   register real64 sum;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
   sum = D_ZERO;
   tocur = mtx->perm.row.org_to_cur;
   elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
   if( rng == mtx_ALL_ROWS )
      for( ; NOTNULL(elt); elt = elt->next.row ) sum += msqr(elt->value);
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.row )
         if( in_range(rng,tocur[elt->row]) ) sum += msqr(elt->value);

   return(sum);
}

real64 mtx_sum_abs_in_row( mtx_matrix_t mtx, int32 row, const mtx_range_t *rng)
{
   struct element_t *elt;
   int32 *tocur;
   register real64 sum;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
   sum = D_ZERO;
   tocur = mtx->perm.col.org_to_cur;
   elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
   if( rng == mtx_ALL_COLS )
      for( ; NOTNULL(elt); elt = elt->next.col ) sum+=fabs(elt->value);
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.col )
         if( in_range(rng,tocur[elt->col]) ) sum+=fabs(elt->value);

   return(sum);
}

real64 mtx_sum_abs_in_col( mtx_matrix_t mtx, int32 col, const mtx_range_t *rng)
{
   struct element_t *elt;
   int32 *tocur;
   register real64 sum;

#if MTX_DEBUG
   if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
   sum = D_ZERO;
   tocur = mtx->perm.row.org_to_cur;
   elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
   if( rng == mtx_ALL_ROWS )
      for( ; NOTNULL(elt); elt = elt->next.row ) sum+=fabs(elt->value);
   else if( rng->high >= rng->low )
      for( ; NOTNULL(elt); elt = elt->next.row )
         if( in_range(rng,tocur[elt->row]) ) sum+=fabs(elt->value);

   return(sum);
}

real64 mtx_row_dot_full_org_vec(mtx_matrix_t mtx,
                                      int32 row,
                                      real64 *orgvec,
                                      mtx_range_t *rng,
                                      boolean transpose)
{
  struct element_t *elt;
  int32 *tocur;
  register real64 sum;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
  sum = D_ZERO;
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];

  if (transpose) {
    int32 *toorg;
    toorg = mtx->perm.row.cur_to_org;
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        sum+=elt->value * orgvec[toorg[tocur[elt->col]]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.col ) {
          cur=tocur[elt->col];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[toorg[cur]];
          }
        }
      }
    }
  } else {
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        sum+=elt->value * orgvec[elt->col];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur,org;
        for( ; NOTNULL(elt); elt = elt->next.col ) {
          cur=tocur[org=elt->col];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[org];
          }
        }
      }
    }
  }
  return(sum);
}

real64 mtx_col_dot_full_org_vec(mtx_matrix_t mtx,
                                      int32 col,
                                      real64 *orgvec,
                                      mtx_range_t *rng,
                                      boolean transpose)
{
  struct element_t *elt;
  int32 *tocur;
  register real64 sum;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
  sum = D_ZERO;
  tocur = mtx->perm.row.org_to_cur;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];

  if (transpose) {
    int32 *toorg;
    toorg = mtx->perm.col.cur_to_org;
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        sum+=elt->value * orgvec[toorg[tocur[elt->row]]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.row ) {
          cur=tocur[elt->row];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[toorg[cur]];
          }
        }
      }
    }
  } else {
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        sum+=elt->value * orgvec[elt->row];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 org,cur;
        for( ; NOTNULL(elt); elt = elt->next.row ) {
          cur=tocur[org=elt->row];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[org];
          }
        }
      }
    }
  }
  return(sum);
}

real64 mtx_row_dot_full_cur_vec(mtx_matrix_t mtx,
                                      int32 row,
                                      real64 *curvec,
                                      mtx_range_t *rng,
                                      boolean transpose)
{
  struct element_t *elt;
  int32 *tocur;
  register real64 sum;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
  sum = D_ZERO;
  if (!transpose) {
    tocur = mtx->perm.col.org_to_cur;
    elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        sum+=elt->value * curvec[tocur[elt->col]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.col ) {
          cur=tocur[elt->col];
          if( in_range(rng,cur) ) {
            sum+=elt->value * curvec[cur];
          }
        }
      }
    }
  } else {
    FPRINTF(g_mtxerr,"(mtx) mtx_row_dot_full_cur_vec:  Transpose version not\n");
    FPRINTF(g_mtxerr,"(mtx)                            done. Returning 0.0.\n");
  }
  return(sum);
}


real64 mtx_col_dot_full_cur_vec(mtx_matrix_t mtx,
                                      int32 col,
                                      real64 *curvec,
                                      mtx_range_t *rng,
                                      boolean transpose)
{
  struct element_t *elt;
  int32 *tocur;
  register real64 sum;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
  sum = D_ZERO;
  if (!transpose) {
    tocur = mtx->perm.row.org_to_cur;
    elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        sum+=elt->value * curvec[tocur[elt->row]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.row ) {
          cur=tocur[elt->row];
          if( in_range(rng,cur) ) {
            sum+=elt->value * curvec[cur];
          }
        }
      }
    }
  } else {
    FPRINTF(g_mtxerr,"(mtx) mtx_col_dot_full_cur_vec:  Transpose version not\n");
    FPRINTF(g_mtxerr,"(mtx)                            done. Returning 0.0.\n");
  }
  return sum;
}

real64 mtx_row_dot_full_org_custom_vec(mtx_matrix_t mtx,
				       mtx_matrix_t mtx2,
                                      int32 row,
                                      real64 *orgvec,
                                      mtx_range_t *rng,
                                      boolean transpose)
{
  struct element_t *elt;
  int32 *tocur;
  register real64 sum;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
  sum = D_ZERO;
  tocur = mtx2->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];

  if (transpose) {
    int32 *toorg;
    toorg = mtx2->perm.row.cur_to_org;
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        sum+=elt->value * orgvec[toorg[tocur[elt->col]]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.col ) {
          cur=tocur[elt->col];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[toorg[cur]];
          }
        }
      }
    }
  } else {
    int32 *toorg;
    toorg = mtx2->perm.col.cur_to_org;
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        sum+=elt->value * orgvec[toorg[tocur[elt->col]]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.col ) {
          cur=tocur[elt->col];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[toorg[cur]];
          }
        }
      }
    }
  }
  return(sum);
}

real64 mtx_col_dot_full_org_custom_vec(mtx_matrix_t mtx,
				       mtx_matrix_t mtx2,
                                      int32 col,
                                      real64 *orgvec,
                                      mtx_range_t *rng,
                                      boolean transpose)
{
  struct element_t *elt;
  int32 *tocur;
  register real64 sum;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return D_ZERO;
#endif
  sum = D_ZERO;
  tocur = mtx2->perm.row.org_to_cur;
  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];

  if (transpose) {
    int32 *toorg;
    toorg = mtx2->perm.col.cur_to_org;
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        sum+=elt->value * orgvec[toorg[tocur[elt->row]]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.row ) {
          cur=tocur[elt->row];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[toorg[cur]];
          }
        }
      }
    }
  } else {
    int32 *toorg;
    toorg = mtx2->perm.row.cur_to_org;
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        sum+=elt->value * orgvec[toorg[tocur[elt->row]]];
      }
    } else {
      if( rng->high >= rng->low ) {
        register int32 cur;
        for( ; NOTNULL(elt); elt = elt->next.row ) {
          cur=tocur[elt->row];
          if( in_range(rng,cur) ) {
            sum+=elt->value * orgvec[toorg[cur]];
          }
        }
      }
    }
  }
  return(sum);
}


void mtx_org_vec_add_row(mtx_matrix_t mtx, real64 *vec, int32 row,
                         real64 factor, mtx_range_t *rng, boolean tr)
{
  struct element_t *elt;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if (tr) {
    int32 *row2org,*org2col;

    row2org = mtx->perm.row.cur_to_org;
    org2col = mtx->perm.col.org_to_cur;
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        vec[row2org[org2col[elt->col]]] += factor * elt->value;
      }
      return;
    }
    if( rng->high >= rng->low ) {
      register int32 cur_col,lo,hi;

      lo=rng->low; hi=rng->high;
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        cur_col = org2col[elt->col];
        if( fast_in_range(lo,hi,cur_col) ) {
          vec[row2org[cur_col]] += factor * elt->value;
        }
      }
    }
  } else {
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        vec[elt->col] += factor * elt->value;
      }
      return;
    }
    if( rng->high >= rng->low ) {
      int32 *tocur;
      register int32 cur_col, org_col,lo,hi;
      lo=rng->low; hi=rng->high;
      tocur = mtx->perm.col.org_to_cur;
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        cur_col=tocur[(org_col=elt->col)];
        if( fast_in_range(lo,hi,cur_col) ) {
          vec[org_col] += factor * elt->value;
        }
      }
    }
  }
}

void mtx_org_vec_add_col(mtx_matrix_t mtx, real64 *vec, int32 col,
                         real64 factor, mtx_range_t *rng, boolean tr)
{
  struct element_t *elt;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif

  elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
  if (tr) {
    int32 *col2org,*org2row;

    org2row = mtx->perm.row.org_to_cur;
    col2org = mtx->perm.col.cur_to_org;
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        vec[col2org[org2row[elt->row]]] += factor * elt->value;
      }
      return;
    }
    if( rng->high >= rng->low ) {
      register int32 cur_row,lo,hi;
      lo=rng->low; hi=rng->high;
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        cur_row = org2row[elt->row];
        if( fast_in_range(lo,hi,cur_row) ) {
          vec[col2org[cur_row]] += factor * elt->value;
        }
      }
    }
  } else {
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        vec[elt->row] += factor * elt->value;
      }
      return;
    }
    if( rng->high >= rng->low ) {
      register int32 cur_row, org_row,lo,hi;
      int32 *tocur;

      lo=rng->low; hi=rng->high;
      tocur = mtx->perm.row.org_to_cur;
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        cur_row=tocur[(org_row=elt->row)];
        if( fast_in_range(lo,hi,cur_row) ) {
          vec[org_row] += factor * elt->value;
        }
      }
    }
  }
}

void mtx_cur_vec_add_row(mtx_matrix_t mtx, real64 *vec, int32 row,
                         real64 factor, mtx_range_t *rng, boolean tr)
{
  struct element_t *elt;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  tocur = mtx->perm.col.org_to_cur;
  elt = mtx->hdr.row[mtx->perm.row.cur_to_org[row]];
  if (!tr) {
    if( rng == mtx_ALL_COLS ) {
      for( ; NOTNULL(elt); elt = elt->next.col )
        vec[tocur[elt->col]] += factor * elt->value;
      return;
    }
    if( rng->high >= rng->low ) {
      register int32 cur_col,lo,hi;
      lo= rng->low; hi=rng->high;
      for( ; NOTNULL(elt); elt = elt->next.col ) {
        cur_col=tocur[elt->col];
        if( fast_in_range(lo,hi,cur_col) )
          vec[cur_col] += factor * elt->value;
      }
    }
  } else {
    FPRINTF(g_mtxerr,"(mtx) mtx_cur_vec_add_row:   transpose==TRUE not done\n");
    /* don't know what it would mean in terms of permutations. */
  }
}

void mtx_cur_vec_add_col(mtx_matrix_t mtx, real64 *vec, int32 col,
                         real64 factor, mtx_range_t *rng, boolean tr)
{
  struct element_t *elt;
  int32 *tocur;

#if MTX_DEBUG
  if(!mtx_check_matrix(mtx)) return;
#endif
  if (!tr) {
    tocur = mtx->perm.row.org_to_cur;
    elt = mtx->hdr.col[mtx->perm.col.cur_to_org[col]];
    if( rng == mtx_ALL_ROWS ) {
      for( ; NOTNULL(elt); elt = elt->next.row )
        vec[tocur[elt->row]] += factor * elt->value;
      return;
    }
    if( rng->high >= rng->low ) {
      register int32 cur_row,lo,hi;
      lo= rng->low; hi=rng->high;
      for( ; NOTNULL(elt); elt = elt->next.row ) {
        cur_row=tocur[elt->row];
        if( fast_in_range(lo,hi,cur_row) )
          vec[cur_row] += factor * elt->value;
      }
    }
  } else {
    FPRINTF(g_mtxerr,"(mtx) mtx_cur_vec_add_col:   transpose==TRUE not done\n");
    /* don't know what it would mean in terms of permutations. */
  }
}

#undef __MTX_C_SEEN__
