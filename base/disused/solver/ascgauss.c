/*
 *  ascgauss: sparse gauss elimination
 *  by Ken Tyner and Ben Allan
 *  Created: 4/14/97
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: ascgauss.c,v $
 *  Date last modified: $Date: 2000/01/25 02:26:48 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
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

/*
 * This file contains gauss and modified gauss implementations.
 * The first gauss is a partial column pivoted gauss L\U.
 * Initially (4/97) it is not the fastest possible code, but is
 * to be easily understood.
 * It needs a header.
 */
#define GAUSSALONE 0
#if GAUSSALONE
#include <math.h>
#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include <general/tm_time.h>
#include "mtx.h"
#include "linsolqr.h"
#define D_ZERO (real64)0.0
#define ZERO (int32)0
#define D_ONE (real64)1.0
#endif

#define GAUSS_DEBUG TRUE

/*
 * See the comments attached to forward_substitute.
 * This code otherwise is the same. It uses a 2 bodied
 * matrix as well as making use of mtx_ALL_COLS and
 * mtx_ALL_ROWS whereever possible.
 * We make the following additional assumptions here:
 *   (if they do not hold, do NOT use this function)
 * - sys->inverse (L) has no incidence that is not multipliers
 *   (and the diagonal of 1s is NOT in sys->inverse.)
 * - sys->factors (U) has no incidence on the upper triangle,
 *   including the diagonal, or outside the factored region.
 *    relaxation: incidence anywhere allowed if value = 0.0
 *                since 0 doesn't contribute to a dot product
 *                and the only thing we do with triangles is dot them.
 * - There may be singular rows and columns in the factorization,
 *   but any additions coming from these rows/columns during
 *   mtx_ALL_*O*S operations will not contribute to sums because the
 *   user zeroed the arr entries corresponding to these before
 *   calling this function.
 */
static void forward_substitute_gauss2(linsolqr_system_t sys,
				     real64 *arr,
				     boolean transpose)
{
  mtx_coord_t nz;
  real64 sum, *pivlist;
  mtx_matrix_t mtx;
  int32 dotlim;
  boolean nonzero_found=FALSE;

  pivlist=sys->ludata->pivlist;
  dotlim = sys->rng.low+sys->rank;
  if (transpose) {		/* arr is indexed by original column number */
    mtx=sys->factors;
    for( nz.col=sys->rng.low; nz.col < dotlim; ++(nz.col) ) {
      register int32 org_col;

      org_col = mtx_col_to_org(mtx,nz.col);
      if (arr[org_col]!=D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
	sum=mtx_col_dot_full_org_vec(mtx,nz.col,arr,mtx_ALL_ROWS,TRUE);
	arr[org_col] = (arr[org_col] - sum) / pivlist[nz.col];
      }
    }
  } else {			/* arr is indexed by original row number */
    mtx=sys->inverse;
    for( nz.row=sys->rng.low; nz.row < dotlim; ++(nz.row) ) {
      register int32 org_row;

      org_row = mtx_row_to_org(mtx,nz.row);
      if (arr[org_row]!=D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
	sum = mtx_row_dot_full_org_vec(mtx,nz.row,arr,mtx_ALL_COLS,TRUE);
	arr[org_row] -= sum;
      }
    }
  }
}

/*
 * See the comments attached to backward_substitute and
 * forward_substitute2.
 * When solving for the transpose, then we are actually
 * running of the lower triangle, hence we use sys->factors.
 * Otherwise we use sys->inverse which stores U.
 */
static void backward_substitute_gauss2(linsolqr_system_t sys,
				      real64 *arr,
				      boolean transpose)
{
  mtx_coord_t nz;
  real64 sum, *pivlist;
  mtx_matrix_t mtx;
  int32 dotlim;
  boolean nonzero_found=FALSE;	/* once TRUE, substitution must be done
				   over remaining rows/cols */

  dotlim=sys->rng.low;
  pivlist=sys->ludata->pivlist;
  if (transpose) {		/* arr is indexed by original column number */
    mtx = sys->inverse;
    for( nz.col = sys->rng.low+sys->rank-1; nz.col >= dotlim ; --(nz.col) ) {
      register int32 org_col;

      org_col = mtx_col_to_org(mtx,nz.col);
      if (arr[org_col] != D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
	sum = mtx_col_dot_full_org_vec(mtx,nz.col,arr,mtx_ALL_ROWS,TRUE);
	arr[org_col] -= sum;
      }
    }
  } else {			/* arr is indexed by original row number */
    /* we are working from the bottom up */
    mtx = sys->factors;
    for( nz.row = sys->rng.low+sys->rank-1; nz.row >= dotlim ; --(nz.row) ) {
      register int32 org_row;

      org_row = mtx_row_to_org(mtx,nz.row);
      if (arr[org_row]!=D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
	sum= mtx_row_dot_full_org_vec(mtx,nz.row,arr,mtx_ALL_COLS,TRUE);
	arr[org_row] = (arr[org_row] - sum) / pivlist[nz.row];
      }
    }
  }
}

static int gauss2_solve(linsolqr_system_t sys, struct rhs_list *rl)
{
  /* zero any unsolved for vars first so they don't contaminate
     mtx_ALL_*O*S dot products.
  */
  zero_unpivoted_vars(sys,rl->varvalue,rl->transpose);
  forward_substitute_gauss2(sys,rl->varvalue,rl->transpose);
  backward_substitute_gauss2(sys,rl->varvalue,rl->transpose);
  return 0;
}



/* this function eliminates a column */
static
int eliminate_column2(
mtx_matrix_t mtx, /* the matrix being factored (upper triangularized) */
mtx_matrix_t lmtx,	/* the multipliers of the factorization, L */
int32 column,	/* the column being eliminated (the pivot is (column,column))*/
int32 lastrow,	/* rows from column+1 to lastrow will be eliminated in column*/
real64 pivot,	/* the pivot value */
mtx_sparse_t *pivotrow	/* the pivot row, less the pivot entry, orgindexed */
)
{
  mtx_coord_t nz;
  mtx_range_t elimrng;
  real64 multiplier;

  /* for rows in column+1 .. lastrow */
  elimrng.low = column+1;
  elimrng.high = lastrow;

  nz.row = mtx_FIRST;
  while ((void)mtx_next_in_col(mtx,&nz,&elimrng),
         nz.row !=mtx_LAST) {
    multiplier = mtx_value(mtx,&nz)/pivot;
    mtx_add_row_sparse(mtx,nz.row,-multiplier,pivotrow);
    mtx_fill_value(lmtx,&nz,multiplier);
  }
  mtx_clear_col(mtx,column,&elimrng);
  return 0;
}

/* This factors a square region given in sys->rng.
 * We might like to have a wider pivot region later, so does not assume
 * that the region to the right of the factorization is empty.
 * Assumes the region to the left of the factorization IS empty.
 */
int gaussba2_factor_square(linsolqr_system_t sys)
{
  mtx_coord_t nz;
  int32 lastrow,lastcolumn,column;
  mtx_range_t pivot_candidates;
  real64 *tmp;
  real64 pivot, *pivots;
  int32 length, defect = 0;
  mtx_matrix_t mtx, lmtx;
  mtx_sparse_t *pivotrow;
  int err = 0;

  length = sys->rng.high - sys->rng.low + 1; /* ? */
  sys->smallest_pivot = MAXDOUBLE;
  lastrow = lastcolumn = pivot_candidates.high = sys->rng.high;
  pivot_candidates.low = sys->rng.low;

  tmp = sys->ludata->tmp;
  pivots = sys->ludata->pivlist;
  mtx = sys->factors;
  lmtx = sys->inverse;
  pivotrow = sys->ludata->tmp_sparse;

/* need pivotrow allocation */

  for (column = sys->rng.low; column <= lastcolumn; ) {
    /* pick pivot. */
    nz.row = column;
    pivot = mtx_get_pivot_col(mtx,&nz,&pivot_candidates,&(pivots[nz.row]),
                              sys->ptol,sys->pivot_zero);

    if (  pivot < sys->pivot_zero ) { /* pivot is an epsilon */
      /* If no pivot, drag to bottom and lastcolumn-- */
      mtx_drag(mtx, nz.row, lastcolumn);
      number_drag(pivots, nz.row, lastcolumn);
      lastcolumn--;
      pivot_candidates.high--; /* is this correct? */
      defect++;
    } else {
      /* collect pivot row. pivot removed to reduce numerics cost*/
      mtx_clear_coord(mtx,nz.row,nz.col);
      pivotrow = mtx_org_row_sparse(mtx,nz.row,pivotrow,
				    mtx_ALL_COLS,mtx_IGNORE_ZEROES); /* check this */
      /* eliminate column */
#if (GAUSS_DEBUG)
  {
    FILE *fp;
    char fname[80];

    sprintf(fname,"/tmp/lu/uga.%d",column");
    fp = fopen(fname,"w+");
    mtx_write_region_human_rows(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);

    sprintf(fname,"/tmp/lu/lga.%d",column");
    fp = fopen(fname,"w+");
    mtx_write_region_human_rows(fp,lmtx,mtx_ENTIRE_MATRIX);
    fclose(fp);

    sprintf(fname,"/tmp/lu/aga.plot%d",column);
    fp=fopen(fname,"w+");
    mtx_write_region_plot(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif
      err = eliminate_column2(mtx,lmtx,column,lastrow,pivot,pivotrow);
      if (err != 0) {
        break;
      }
      pivot_candidates.low++;
      column++;
    }
  }
  sys->rank = pivot_candidates.high - sys->rng.low + 1;
  return err; /* err = 0 is good */
}


int gauss2_entry(linsolqr_system_t sys, mtx_region_t *region)
{
  struct rhs_list *rl;
  double time;

  CHECK_SYSTEM(sys);
  if (sys->factored) return 0;
  switch(sys->fmethod) {
  case gauss_ba2:    /* add new methods here */
    break;
  default:
    return 1;
  }

  if (ISNULL(sys->ludata)) return 1;
  if (NOTNULL(sys->inverse)) mtx_destroy(sys->inverse);
  sys->inverse = NULL;
  if (NOTNULL(sys->factors)) mtx_destroy(sys->factors);
  if (region == mtx_ENTIRE_MATRIX) determine_pivot_range(sys);
  else square_region(sys,region);

  sys->factors = mtx_copy_region(sys->coef,region);
  sys->inverse = mtx_create_slave(sys->factors);
  sys->rank = -1;
  sys->smallest_pivot = MAXDOUBLE;
  for (rl = sys->rl ; NOTNULL(rl) ; rl = rl->next)
    rl->solved = FALSE;
  insure_capacity(sys);
  insure_lu_capacity(sys);

  time = tm_cpu_time();
  switch(sys->fmethod) {
  case gauss_ba2:
    gaussba2_factor_square(sys);
    break;
  default:
    return 1;
  }
  sys->factored = TRUE;

#define KAA_DEBUG
#ifdef KAA_DEBUG
  time = tm_cpu_time() - time;
  FPRINTF(stderr,"Time for Inversion = %f\n",time);
  FPRINTF(stderr,"Non-zeros in A = %d\n",
    mtx_nonzeros_in_region(sys->coef,region));
  FPRINTF(stderr,"Non-zeros in U+L = %d\n",
    mtx_nonzeros_in_region(sys->factors,region) +
    mtx_nonzeros_in_region(sys->inverse,0));
#endif /* KAA_DEBUG */
#undef KAA_DEBUG
  return 0;
}









/* The following crap is from kirk. we should be ditching it and writing a
 * real gauss suitable to ken's and other needs.
 */
#define GAUSS_ELIMINATE_ROWS_IMPLEMENTED 0
#if GAUSS_ELIMINATE_ROWS_IMPLEMENTED
static void gauss_eliminate_rows(mtx_matrix_t mtx,
				 mtx_matrix_t lower_mtx,
				 int32 col,
				 real64 *value,
				 int32 *index,
				 real64 pivot)
{
  mtx_coord_t nz;
  real64 multiplier;
  int32 i, ii;
  int colcount, row;

/*
  colcount = mtx_cur_col_to_csr(mtx,col,value,index);
*/
  mtx_clear_col(mtx,col,mtx_ALL_ROWS);

  row = col;
  nz.col = col;
  for (ii=0; ii<colcount; ii++) {
    i = index[ii];
    if (i <= row) {
      /* nothing to do - we would be back patching U here. */
      continue;
    }
    else{
      multiplier = value[ii]/pivot;
      mtx_add_row(mtx,row,i,-multiplier,mtx_ALL_COLS);
      nz.row = i;
      mtx_fill_value(lower_mtx,&nz,multiplier);
    }
  }
}


/*
 * At the moment this code assumes that we are given a
 * square region. It also assumes that the submatrix A11 will
 * be provided; in which case pivot selection should be restricted
 * to A11. Later on we will modify the code to accept a rectangular
 * region, we will then make use of the 'last_pivotable column',
 * and sys->reg. At the moment only sys->rng and A11 are used.
 */
static int kirk_gauss_factor(linsolqr_system_t sys,mtx_region_t *A11)
{
  /*
   * THIS HAS BUG -- row can not be updated.
   */
  mtx_matrix_t mtx, lower_mtx;
  mtx_coord_t nz;
  int32 last_col;
  mtx_range_t pivot_candidates;
  real64 pivot, abs_pivot, *pivots, max;
  int32 row,col;
  int32 *index = NULL;		/* used by csr */
  real64 *value = NULL;		/* used by csr */
  int length;

  mtx = sys->factors;
  lower_mtx = sys->inverse;
  pivots = sys->ludata->pivlist;

  last_col = sys->rng.high;
  pivot_candidates.high = A11->col.high;	/* initialize */

  length = sys->rng.high - sys->rng.low + 1;
  value = (real64 *)ascmalloc(length*sizeof(real64));
  index = (int32 *)ascmalloc(length*sizeof(int32));

  /*
   * First find the maximum element in the col *regardless*. This
   * will leave nz at the correct position in case we need to a
   * column swap. We have to adjust the pivot range as we go along.
   */
  for (col = sys->rng.low; col < last_col; ) {
    nz.row = nz.col = col;
    pivot_candidates.low = col;
    if (nz.col > A11->col.high) {
      pivot_candidates.high = sys->rng.high;
    }
    pivot = mtx_value(mtx,&nz);

    max = mtx_col_max(mtx, &nz, &pivot_candidates, &pivots[col]);
    if (max <= sys->pivot_zero) {
      FPRINTF(stderr,"Matrix appears numerically singular at %d %g %g\n",
	      row,pivot,max);
      /* patch for the time being */
      pivot = pivots[col] = 1.0;
      col++;
      continue;
    }

    abs_pivot = fabs(pivot);
    if ((abs_pivot >= sys->ptol * max)) {
      pivots[col] = pivot;
    }
    else {
      mtx_swap_rows(mtx, row, nz.row);
      pivot = pivots[col] = max;
    }

    gauss_eliminate_rows(mtx, lower_mtx, col, value, index, pivot);
    col++;
  }

  if (value) ascfree((char *)value);
  if (index) ascfree((char *)index);

  sys->rank = last_col - sys->rng.low + 1;
  return sys->rank;
}
#endif /* GAUSS_ELIMINATE_ROWS_IMPLEMENTED */
