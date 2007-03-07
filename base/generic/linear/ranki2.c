#include "ranki2.h"

#include <math.h>
#include <general/tm_time.h>

#include "linsolqr_impl.h"
#include "ranki_impl.h"

/*------------------------------------------------------------------------------
 * Start of the 2 bodied factorization schemes.
 *
 * ranki2_entry will be used to access these routines.
 * there is now code called:
 * rankikw2_factor
 * rankijz2_factor
 * eliminate_row2
 * ranki2_entry
 * forward_substitute2
 * backward_substitute2
 * ranki2_solve
 *
 * We have fixed the calc_col_dependent and calc_row_dependent
 * routines.
 */

extern int32 mtx_number_ops;

/* this function does no permutation of any sort */
static
void eliminate_row2(mtx_matrix_t mtx,
		mtx_matrix_t upper_mtx,
		mtx_range_t *rng,
		int32 row,       /* row to eliminate */
		real64 *tmp,     /* temporary array */
		real64 *pivots,  /* prior pivots array */
		real64 dtol      /* drop tolerance */
){
  mtx_coord_t nz;
  int j,low,high;
  double tmpval;

  /*
   * Move all non-zeros from current row to full array.
   * The full array would have been initialized before,
   * we must put it back in the clean state when we leave.
   * All operations are done over mtx_ALL_COLS.
   *
   * Note: because of an addrow over mtx_ALL_COLS, entries
   * of tmp outside rng may have nonzeros put in them if
   * sys->factors has nonzeros in the outlying columns.
   * If enough of these pile up out there during elimination
   * of a block and the compiler treats double overflow as
   * a signalable error, we will have a floating point
   * exception.
   * Currently sys->factors is a copy of a region from
   * the sys->coef matrix, so we should not have anything
   * out there to pile up. If we make a variant which
   * does not copy the coef matrix but uses it directly,
   * this code needs to be revisited.
   */
  mtx_steal_cur_row_vec(mtx,row,tmp,mtx_ALL_COLS);

  /*
   * Eliminates nzs from pivot, one by one, filling tmp with multipliers
   * We now operate on the entire row, since we are not writing the
   * multipliers back to the matrix.
   */

  low = rng->low;
  high = rng->high;
  for (j = row-1 ; j >= low ; --(j) ) {
    if (tmp[j] == D_ZERO)
      continue;                 /* Nothing to do for this row */
    tmpval = tmp[j]/pivots[j];  /* Compute multiplier */

    /*
     * tmpval is now the multiplier. We use it to eliminate the row,
     * but backpatch it to tmp, *after* we do the elimination, as
     * mtx_cur_vec_add_row over all columns will stomp on tmp[j]
     */
    mtx_cur_vec_add_row(mtx,tmp,j,-tmpval,mtx_ALL_COLS,FALSE);
    tmp[j] = tmpval;            /* patch the diagonal */
  }

  /*
   * Fill up the upper triangular matrix.
   * refill the range [low .. row-1]. Remember that we have
   * to zero all nnz's in tmp.
   */
  nz.row = row;
  for (j=low;j<row;j++) {
    if (tmp[j] != D_ZERO) {
      nz.col = j;
      mtx_fill_value(upper_mtx,&nz,tmp[j]);
#if RBADEBUG
      FPRINTF(gscr,"fillingU: or %d oc %d (cc %d) %24.18g\n",
        mtx_row_to_org(mtx,nz.row), mtx_col_to_org(mtx,nz.col), nz.col, tmp[j]);
#endif
      tmp[j] = D_ZERO;
    }
  }
  /*
   * refill the range [row .. high]. We *wont* keep
   * the diagonal as this is sorted out by the pivot
   * array, but here we must keep it, sigh.
   * At this stage we don't know that the diagonal is the
   * pivot, as that is determined after elimination done.
   * Outer loop must delete the ultimate pivot element.
   * Odds are good, however, that it is the current diagonal,
   * so put in the diagonal last.
   */
  low = row;
  for (j=high;j>=low;j--) {
    if (tmp[j] != D_ZERO) {
      if (fabs(tmp[j]) > dtol) {
        nz.col = j;
        mtx_fill_value(mtx,&nz,tmp[j]);
#if RBADEBUG
        FPRINTF(gscr,"fillingL: or %d oc %d %24.18g\n",
          mtx_row_to_org(mtx,nz.row), mtx_col_to_org(mtx,nz.col), tmp[j]);
#endif
      }
      tmp[j] = D_ZERO; /* zero element regardless */
    }
  }
  tmp[row] = D_ZERO;
}

/*
 * This function is the same as rankikw_factor except
 * that it uses 2 matrices; one to store L and one for U.
 * As such it uses eliminate_row2 rather than eliminate_row.
 * Note there has been a change in the way ptol is used,
 * now selecting nearest passing, rather than taking max.
 * We assume there is NO incidence outside the region to be factored
 * in sys->factors.
 */
static void rankikw2_factor(linsolqr_system_t sys)
{
  mtx_coord_t nz;
  int32 last_row, defect;
  mtx_range_t pivot_candidates;
  real64 *tmp;
  real64 pivot, *pivots;
  mtx_matrix_t mtx, upper_mtx;


  /**
   ** tmp and pivots are cur indexed, tmp by cols, pivots by rows.
   ** rather than allocating tmp every time, which can get expensive,
   ** we allocate it once *elsewhere* and zero it all here.
   ** Because we don't know the state of it on entry (due to exceptions,
   ** etc, the rezeroing is unavoidable.
   ** Eliminate_row2 is responsible for returning the working region
   ** zeroed.
   **/

  tmp = sys->ludata->tmp;
  mtx_zero_real64(tmp,sys->capacity);
  pivots = sys->ludata->pivlist;
  mtx = sys->factors;
  upper_mtx = sys->inverse;
  defect = 0;
#if RBADEBUG
  gscr = fopen("/usr1/ballan/tmp/lu/rkw","w+");
#endif

  sys->smallest_pivot = MAXDOUBLE;
  last_row = pivot_candidates.high = sys->rng.high;
  for( nz.row = sys->rng.low ; nz.row <= last_row ; ) {

#if (RBADEBUG && 0)
    /* remove the && 0 and you better have a lot for free
     * disk space
     */
    mtx_write_region_human_orgrows(gscr,mtx,mtx_ENTIRE_MATRIX);
#endif

    pivot_candidates.low = nz.col = nz.row;
    pivots[nz.row]=pivot = mtx_value(mtx,&nz);
    pivot = fabs(pivot);
    if( pivot > sys->pivot_zero &&
       pivot >= sys->ptol * mtx_row_max(mtx,&nz,&pivot_candidates,NULL) &&
       !col_is_a_spike(sys,nz.row) ) {
      /* Good pivot and not a spike: continue with next row */
      if( pivot < sys->smallest_pivot )  {
        sys->smallest_pivot = pivot;
      }
#if RBADEBUG
      FPRINTF(gscr,"Cheap pivot col %d (org %d)\n",
        nz.row, mtx_col_to_org(mtx,nz.row));
      if (nz.row ==179) {
        CONSOLE_DEBUG("ben, stop here");
      }
#endif
      ++(nz.row);
      continue;
    }
    /* pivots for rows nz.row back to sys->rng->low are stored in pivots */
    /**
    	Row is a spike row or will
    	be when a necessary column
    	exchange occurs.
     **/
    eliminate_row2(mtx,upper_mtx,&(sys->rng),nz.row,tmp,pivots,sys->dtol);
    /* pivot will be leftmost of those that pass ptol and eps, or 0.0. */
    if (defect) {
      /* unfortunately, can't use mtx_ALL_COLS in search now because we must
         ignore columns that were dragged out with previous singular rows. */
      pivot = mtx_get_pivot_col(mtx,&nz,&pivot_candidates,&(pivots[nz.row]),
                                sys->ptol,sys->pivot_zero);
    } else {
      /* no singular cols, and eliminate moved all left of diag crap out, so */
      pivot = mtx_get_pivot_col(mtx,&nz,mtx_ALL_COLS,&(pivots[nz.row]),
                                sys->ptol,sys->pivot_zero);
    }
#if RBADEBUG
      FPRINTF(gscr,"Pivot column found %d (org %d)\n",
        nz.col, mtx_col_to_org(mtx,nz.col));
#endif
    if( pivot < sys->pivot_zero ) { /* pivot is an epsilon */
      /*
       * Dependent row, drag to the end. The upper_mtx is a slave
       * of mtx, and will be dragged automatically.
       */
      mtx_drag(mtx, nz.row, last_row);
      number_drag(pivots, nz.row, last_row);
      --last_row;
      defect = 1;
    } else {
      /* Independent row: nz contains selected pivot */
      mtx_swap_cols(mtx, nz.row, nz.col);   /* this Fixes U as well */
      /* Move pivot to diagonal */
      mtx_drag(mtx, nz.row, sys->rng.low ); /* this Fix U as well */
      number_drag(pivots, nz.row, sys->rng.low);
      if( pivot < sys->smallest_pivot )
        sys->smallest_pivot = pivot;
      ++(nz.row);
    }
  }
#if RBADEBUG
/* get rid of this  asap*/
  {
    FILE *fp;
    int32 cc;

    fp = fopen("/usr1/ballan/tmp/lu/kw2p","w+");
    FPRINTF(fp,"kw2 final pivot sequence:\n");
    for (cc= sys->rng.low; cc <= last_row; cc++) {
      FPRINTF(fp,"orgrow,orgcol = %d,%d,%24.18g\n",
        mtx_row_to_org(mtx,cc),
        mtx_col_to_org(mtx,cc), pivots[cc]);
    }
    fclose(fp);
  }

#endif

  zero_diagonal_elements(sys->factors,sys->rng.low,sys->rng.high);
  sys->rank = last_row - sys->rng.low + 1;

#if RBADEBUG
  {
   FILE *fp;
   fp = fopen("/usr1/ballan/tmp/lu/kw2m","w+");
   mtx_write_region_human_rows(fp,mtx,mtx_ENTIRE_MATRIX);
   fclose(fp);
   fp = fopen("/usr1/ballan/tmp/lu/kw2um","w+");
   mtx_write_region_human_rows(fp,upper_mtx,mtx_ENTIRE_MATRIX);
   fclose(fp);
  }
#endif
#if RBADEBUG
  fclose(gscr);
#endif
}

/*
 * This function is the same as rankijz_factor except
 * that it uses 2 matrices; one to store L and one for U.
 * As such it uses eliminate_row2 rather than eliminate_row.
 *
 * We are now using the slave matrix feature of the code, and
 * thus we all drag/swaps automatically affect the slave (upper_mtx)
 * as well.
 */
static void rankijz2_factor(linsolqr_system_t sys)
{
  real64 biggest;
  mtx_coord_t nz, best;
  mtx_region_t candidates;
  real64 *tmp;
  real64 pivot, *pivots;
  mtx_matrix_t mtx, upper_mtx;
  /**
   ** tmp and pivots are cur indexed, tmp by cols, pivots by rows.
   ** rather than allocating tmp every time, which can get expensive,
   ** we allocate it once elsewhere and zero it all here.
   ** Because we don't know the state of it on entry (due to exceptions,
   ** etc), the rezeroing is unavoidable.
   ** Eliminate_row2 is responsible for returning the working region
   ** zeroed.
   **/
  tmp = sys->ludata->tmp;
  mtx_zero_real64(tmp,sys->capacity);
  pivots=sys->ludata->pivlist;
  mtx = sys->factors;
  upper_mtx = sys->inverse;

  sys->smallest_pivot = MAXDOUBLE;
  candidates.row.high = sys->rng.high;
  candidates.col.high = sys->rng.high;
  for( nz.row = sys->rng.low ; nz.row <= candidates.row.high ; ) {
    nz.col = nz.row;
    pivots[nz.row] = pivot = mtx_value(mtx,&nz);
    pivot = fabs(pivot);
    candidates.row.low = nz.row;
    candidates.col.low = nz.row;
    if( !col_is_a_spike(sys,nz.row) ) {
      best.col = nz.row;
      mtx_col_max(mtx,&best,&(candidates.row),&biggest);
      /* make sure we aren't missing someone really nice in the column */
      if( fabs(biggest) >= sys->pivot_zero ) {
        if( pivot < sys->pivot_zero || pivot < sys->ptol*fabs(biggest) ) {
          mtx_swap_rows(mtx,nz.row,best.row); /* Fixes U as well */
          pivots[nz.row] = biggest;
          pivot = fabs(biggest);
        }
        if( pivot < sys->smallest_pivot ) sys->smallest_pivot = pivot;
        ++(nz.row);
        continue;
      }
    }
    /* pivots for rows nz.row back to sys->rng->low are stored in pivots */
    /**
    	Row is a spike row or will
    	be when a necessary column
    	exchange occurs.
     **/
    eliminate_row2(mtx,upper_mtx,&(sys->rng),nz.row,tmp,pivots,sys->dtol);
    /* pivot will be largest of those available. get size and value */
    pivot=mtx_row_max(mtx,&nz,&(candidates.col),&(pivots[nz.row]));
    /* Move pivot to diagonal */
    if( pivot < sys->pivot_zero ) { /* pivot is an epsilon */
      /* Dependent row, drag nz to lower right */
      mtx_drag(mtx,nz.row,candidates.row.high);
      number_drag(pivots,nz.row,candidates.row.high);
      --(candidates.row.high);
    } else {
      /* Independent row, drag nz to upper left */
      mtx_swap_cols(mtx,nz.row,nz.col); /* this Fixes U as well */
      mtx_drag(mtx ,nz.row ,sys->rng.low);

      number_drag(pivots,nz.row,sys->rng.low);
      if( pivot < sys->smallest_pivot )
        sys->smallest_pivot = pivot;
      ++(nz.row);
    }
  }
  zero_diagonal_elements(sys->factors,sys->rng.low,sys->rng.high);
  sys->rank = candidates.row.high - sys->rng.low + 1;
}

/*
 * See the comments attached to forward_substitute.
 * This code otherwise is the same. It uses a 2 bodied
 * matrix as well as making use of mtx_ALL_COLS and
 * mtx_ALL_ROWS whereever possible.
 * We make the following additional assumptions here:
	 (if they do not hold, do NOT use this function)
 * - sys->inverse (U) has no incidence that is not multipliers
	 (and the diagonal of 1s is NOT in sys->inverse.)
	 As of 10/95, this is how ranki2 U is constructed.
 * - sys->factors (L) has no incidence on the upper triangle,
	 including the diagonal, or outside the factored region.
	  relaxation: incidence anywhere allowed if value = 0.0
	              since 0 doesn't contribute to a dot product
	              and the only thing we do with triangles is dot them.
 * - There may be singular rows and columns in the factorization,
	 but any additions coming from these rows/columns during
	 mtx_ALL_*O*S operations will not contribute to sums because the
	 user zeroed the arr entries corresponding to these before
	 calling this function.
 */
void forward_substitute2(linsolqr_system_t sys,
		real64 *arr,
		boolean transpose
){
  mtx_coord_t nz;
  real64 sum, *pivlist;
  mtx_matrix_t mtx;
  int32 dotlim;
  boolean nonzero_found=FALSE;

  pivlist=sys->ludata->pivlist;
  dotlim = sys->rng.low+sys->rank;
  if (transpose) { /* arr is indexed by original column number */
    mtx=sys->inverse;
    for( nz.col=sys->rng.low; nz.col < dotlim; ++(nz.col) ) {
      register int32 org_col;

      org_col = mtx_col_to_org(mtx,nz.col);
      if (arr[org_col]!=D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
        sum=mtx_col_dot_full_org_vec(mtx,nz.col,arr,mtx_ALL_ROWS,TRUE);
        /* arr[org_col] = (arr[org_col] - sum)  / D_ONE */;
        arr[org_col] -= sum;
      }
    }
  } else { /* arr is indexed by original row number */
    mtx=sys->factors;
    for( nz.row=sys->rng.low; nz.row < dotlim; ++(nz.row) ) {
      register int32 org_row;

      org_row = mtx_row_to_org(mtx,nz.row);
      if (arr[org_row]!=D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
        sum = mtx_row_dot_full_org_vec(mtx,nz.row,arr,mtx_ALL_COLS,TRUE);
        /*
           nz.col = nz.row;
           arr[org_row] = (arr[org_row] - sum) / mtx_value(mtx,&nz);
         */
        arr[org_row] = (arr[org_row] - sum) / pivlist[nz.row];
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
void backward_substitute2(linsolqr_system_t sys,
		real64 *arr,
		boolean transpose
){
  mtx_coord_t nz;
  real64 sum, *pivlist;
  mtx_matrix_t mtx;
  int32 dotlim;
  boolean nonzero_found=FALSE; /* once TRUE, substitution must be done
                                  over remaining rows/cols */

  dotlim=sys->rng.low;
  pivlist=sys->ludata->pivlist;
  if (transpose) { /* arr is indexed by original column number */
    mtx = sys->factors;
    for( nz.col = sys->rng.low+sys->rank-1; nz.col >= dotlim ; --(nz.col) ) {
      register int32 org_col;

      org_col = mtx_col_to_org(mtx,nz.col);
      if (arr[org_col] != D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
        sum = mtx_col_dot_full_org_vec(mtx,nz.col,arr,mtx_ALL_ROWS,TRUE);
        arr[org_col] = (arr[org_col] - sum) / pivlist[nz.col];
      }
    }
  } else {			/* arr is indexed by original row number */
    /* we are working from the bottom up */
    mtx = sys->inverse;
    for( nz.row = sys->rng.low+sys->rank-1; nz.row >= dotlim ; --(nz.row) ) {
      register int32 org_row;

      org_row = mtx_row_to_org(mtx,nz.row);
      if (arr[org_row]!=D_ZERO) nonzero_found=TRUE;
      if (nonzero_found) {
        sum= mtx_row_dot_full_org_vec(mtx,nz.row,arr,mtx_ALL_COLS,TRUE);
        arr[org_row] -= sum;
      }
    }
  }
}

void calc_dependent_rows_ranki2(linsolqr_system_t sys){
  mtx_coord_t nz;
  real64 value;
  mtx_range_t colrange;
  mtx_range_t rowrange;
  real64 *lc;
  mtx_matrix_t mtx;

  sys->rowdeps = TRUE;
  if( ( (sys->reg.row.low == sys->rng.low) &&
        ( sys->reg.row.high == sys->rng.low+sys->rank-1 )
      ) || sys->rank==0 )
    return;

  lc = sys->ludata->tmp;
  colrange.low = sys->rng.low;
  colrange.high = colrange.low + sys->rank - 1;
  rowrange.low = sys->rng.high;
  rowrange.high = sys->rng.low+sys->rank;
  mtx=sys->factors;

  nz.row = sys->reg.row.low;
  for( ; nz.row <= sys->reg.row.high; nz.row++ ) {
    if( nz.row == sys->rng.low ) {
      nz.row = rowrange.high-1;
      continue;
    }
    mtx_zero_real64(lc,(sys->capacity));
    /* must zero the whole thing to use the backward_substitute2 right */
    mtx_org_row_vec(mtx,nz.row,lc,&colrange);
    if( nz.row < rowrange.high || nz.row > rowrange.low )
      backward_substitute2(sys,lc,TRUE);
    forward_substitute2(sys,lc,TRUE);
    mtx_clear_row(mtx,nz.row,&colrange);
    for( nz.col=colrange.low; nz.col <= colrange.high; nz.col++ ) {
      value = lc[mtx_col_to_org(mtx,nz.col)];
      if( value != D_ZERO ) mtx_fill_value(mtx,&nz,value);
    }
  }
}

void calc_dependent_cols_ranki2(linsolqr_system_t sys){
  mtx_coord_t nz;
  real64 value;
  mtx_range_t rowrange;
  mtx_range_t colrange;
  real64 *lc;
  mtx_matrix_t mtx;

  sys->coldeps = TRUE;
  if( ( (sys->reg.col.low == sys->rng.low) &&
        ( sys->reg.col.high == sys->rng.low+sys->rank-1 )
      ) || sys->rank==0 )
    return;

  lc = sys->ludata->tmp;
  rowrange.low = sys->rng.low;
  rowrange.high = rowrange.low + sys->rank - 1;
  colrange.high = sys->rng.low+sys->rank;
  colrange.low = sys->rng.high;
  mtx=sys->factors;

  nz.col = sys->reg.col.low;
  for( ; nz.col <= sys->reg.col.high; nz.col++ ) {
    if( nz.col == sys->rng.low ) {
      nz.col = colrange.high-1;
      continue;
    }
    mtx_zero_real64(lc,sys->capacity);
    mtx_org_col_vec(mtx,nz.col,lc,&rowrange);
    if( nz.col < colrange.high || nz.col > colrange.low )
      backward_substitute2(sys,lc,FALSE);
    forward_substitute2(sys,lc,FALSE);
    mtx_clear_col(mtx,nz.col,&rowrange);
    for( nz.row=rowrange.low; nz.row <= rowrange.high; nz.row++ ) {
        value = lc[mtx_row_to_org(mtx,nz.row)];
      if( value != D_ZERO ) mtx_fill_value(mtx,&nz,value);
    }
  }
}

int ranki2_solve(linsolqr_system_t sys, struct rhs_list *rl){
  /* zero any unsolved for vars first so they don't contaminate
     mtx_ALL_*O*S dot products.
  */
  zero_unpivoted_vars(sys,rl->varvalue,rl->transpose);
  backward_substitute2(sys,rl->varvalue,rl->transpose);
  forward_substitute2(sys,rl->varvalue,rl->transpose);
  return 0;
}

/* include rankiba2 implementation. Note that this is VERY VERY bad style
and needs to be fixed immediately by splitting up the linsolqr file.
This is our fastest (far and away) ranki implementation -- drag free.
*/
#if 0
#include "rankiba2.c"
#endif

#ifdef BUILD_KIRK_CODE
/*
 * this forward declaration is necessary to keept the
 * system happy until we can organize this file
 */

static
int kirk1_factor(linsolqr_system_t sys,
	mtx_region_t *A11,
	int kirk_method
);

#endif /* BUILD_KIRK_CODE */

/*
 * This is the entry point for all the ranki2_* schemes
 * which make use of a 2 bodied matrix.
 * it does not respond to ranki_kw and ranki_jz as if they
 * were calls for ranki_kw2 and ranki_jz2. with the new
 * factorization classes , there's really no call to be
 * sneaking around the header functions.
 * ranki_ka goes here as well. check sys->fmethod to see whos
 * expected to be in use.
 */
int ranki2_entry(linsolqr_system_t sys, mtx_region_t *region){
  struct rhs_list *rl;
  double comptime;

  CHECK_SYSTEM(sys);
  if (sys->factored) return 0;
  switch(sys->fmethod) {
  case ranki_ba2:
  case ranki_kw2:
  case ranki_jz2:    /* add new methods here */
#ifdef BUILD_KIRK_CODE
  case ranki_ka:
#endif /* BUILD_KIRK_CODE */
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
  ensure_capacity(sys);
  ensure_lu_capacity(sys);

  comptime = tm_cpu_time();
  switch(sys->fmethod) {
  case ranki_ba2:
    rankiba2_factor(sys);
    break;
  case ranki_kw2:
    rankikw2_factor(sys);
    break;
  case ranki_jz2:
    rankijz2_factor(sys);
    break;

#ifdef BUILD_KIRK_CODE
  case ranki_ka:
    kirk1_factor(sys,region,2);
    break;
#endif /* BUILD_KIRK_CODE */

  default:
    return 1;
  }
  sys->factored = TRUE;

#define KAA_DEBUG 1
#if KAA_DEBUG
  if (g_linsolqr_timing) {
    int anz;
    int fnz;
    anz = mtx_nonzeros_in_region(sys->coef,region);
    fnz = mtx_nonzeros_in_region(sys->factors,region) +
      mtx_nonzeros_in_region(sys->inverse,0);
    comptime = tm_cpu_time() - comptime;
    CONSOLE_DEBUG("A-NNZ: %d Factor time: %f Fill %g",
      anz,comptime,( anz>0 ? (double)fnz/(double)anz : 0));
  }
#endif /* KAA_DEBUG */
#undef KAA_DEBUG
  return 0;
}

/*
  End of RANKI implementation functions.
*/
