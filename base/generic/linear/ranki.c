#include "ranki.h"
#include "linsolqr_impl.h"
#include "ranki_impl.h"

#include <general/tm_time.h>
#include <math.h>

static void eliminate_row(mtx_matrix_t mtx,
                          mtx_range_t *rng,
                          int32 row,      /* row to eliminate */
                          real64 *tmp,    /* temporary array */
                          real64 *pivots) /* prior pivots array */
/**
	Eliminates the given row to the left of the diagonal element, assuming
	valid pivots for all of the diagonal elements above it (the elements
	above those diagonal elements, if any exist, are assumed to be U
	elements and therefore ignored).  The temporary array is used by this
	function to do its job.  tmp[k], where rng->low <= k <= rng->high must
	be defined (allocated) but need not be initialized.
	pivots[k] where rng->low <= k <=rng->high must be allocated and
	pivots[rng->low] to pivots[row-1] must contain the previous pivots.
 **/
{
   mtx_coord_t nz;
   mtx_range_t left_to_eliminate,high_cols,tmp_cols;
   boolean do_series=FALSE;

   high_cols.low = row;
   high_cols.high = rng->high;
   left_to_eliminate.low = rng->low;
   left_to_eliminate.high = row - 1;

   /* Move non-zeros left of pivot from matrix to full array */
   mtx_zero_real64(tmp+rng->low,(row-rng->low));
   nz.row = row;
   mtx_cur_row_vec(mtx,row,tmp,&left_to_eliminate);
   mtx_clear_row(mtx,row,&left_to_eliminate);

   /* eliminates nzs from pivot, one by one, filling tmp with multipliers */
   do_series= ( (row-1) >= (rng->low) );
   if (do_series) {
     mtx_add_row_series_init(mtx,row,FALSE);
     /* do nothing between here and the END call to _init which removes
        elements from row */
   }
   for( nz.row = row-1 ; nz.row >= rng->low ; --(nz.row) ) {
      if( tmp[nz.row] == D_ZERO )
         continue;   /* Nothing to do for this row */

      nz.col = nz.row;
      tmp[nz.row] /= pivots[nz.row];
      /* tmp[nz.row] now equals multiplier */

      /* Perform "mtx_add_row" for full array part of the row */
      left_to_eliminate.high = nz.row - 1;
      mtx_cur_vec_add_row(mtx,tmp,nz.row,-tmp[nz.row],
                          &left_to_eliminate,FALSE);

      /* Perform "mtx_add_row" on remaining part of row */
      mtx_add_row_series(nz.row,-tmp[nz.row],&high_cols);
   }
   if (do_series) {
      mtx_add_row_series_init(mtx,mtx_NONE,TRUE);
   }

   mtx_fill_cur_row_vec(mtx,row,tmp,mtx_range(&tmp_cols,rng->low,row-1));

}


/* not in use it seems */
#if BUILD_DEAD_CODE /* yes, baa */
static int32 top_of_spike(linsolqr_system_t sys,int32 col)
/**
	Determines the top row (row of lowest index) in a possible spike
	above the diagonal element in the given column.  If there is no spike,
	then (row = ) col is returned.
 **/
{
   mtx_range_t above_diagonal;
   mtx_coord_t nz;
   real64 value;
   int32 top_row;

   above_diagonal.low = sys->rng.low;
   above_diagonal.high = col-1;
   top_row = nz.col = col;
   nz.row = mtx_FIRST;
   while( value = mtx_next_in_col(sys->factors,&nz,&above_diagonal),
         nz.row != mtx_LAST )
      if( nz.row < top_row )
         top_row = nz.row;
   return( top_row );
}
#endif /* BUILD_DEAD_CODE */

/**
	Determines if the col is a spike, characterized by having any
	nonzeros above the diagonal. By construction there shouldn't
	be any numeric 0 nonzeros above the diagonal, so we don't prune
	them out here.
 **/
boolean col_is_a_spike(linsolqr_system_t sys,int32 col){

  mtx_range_t above_diagonal;
  mtx_coord_t nz;

  above_diagonal.low = sys->rng.low;
  above_diagonal.high = col-1;
  nz.col = col;
  nz.row = mtx_FIRST;
  /* this loop is cheaper than it looks */
  while( (void)mtx_next_in_col(sys->factors,&nz,&above_diagonal),
         nz.row != mtx_LAST ) {
    if( nz.row < col ) return TRUE;
  }
  return( FALSE );
}

/* see implementation notes before modifying this function! */
void number_drag(real64 *vec, int32 rfrom, int32 rto){
  int32 i;
  real64 tmp;
  if (rto <rfrom) {
    tmp=vec[rfrom];
    for (i=rfrom; i> rto; i--) {
      vec[i]=vec[i-1]; /* rotate right */
    }
    vec[rto]=tmp;
    return;
  }
  if (rto > rfrom) {
    tmp=vec[rfrom];
    for (i=rfrom; i< rto; i++) {
      vec[i]=vec[i+1]; /* rotate left */
    }
    vec[rto]=tmp;
  }
}

static void rankikw_factor(linsolqr_system_t sys)
/**
	(the following description also applies to rankijz_factor which is
	different only in pivot selection strategy.)
	This is the heart of the linear equation solver.  This function
	factorizes the matrix into a lower (L) and upper (U) triangular
	matrix.  sys->smallest_pivot and sys->rank are calculated.  The
	RANKI method is utilized.  At the end of elimination, the matrix A
	is factored into A = U L, where U and L are stored as follows:

	    <----- r ----> <- n-r ->
	   +--------------+---------+
	   |              |         |
	   |         U    |         |
	   |              |         |
	   |   L          |         | r
	   |              |         |
	   +--------------+---------+
	   |              |         |
	   |              |   0     | n-r
	   |              |         |
	   +--------------+---------+

	The rows and columns have been permuted so that all of the pivoted
	original rows and columns are found in the first r rows and columns
	of the region.  The last n-r rows and columns are unpivoted.  U has
	1's on its diagonal, whereas L's diagonal is stored in the matrix.

	Everything above was written as though the entire matrix is
	involved.  In reality, only the relevant square region is involved.
*/
{
   mtx_coord_t nz;
   int32 last_row;
   mtx_range_t pivot_candidates;
   real64 *tmp;
   real64 pivot, *pivots;
   int32 length;
   mtx_matrix_t mtx;

   length = sys->rng.high - sys->rng.low + 1;
   tmp = sys->ludata->tmp;
   /* eliminate row takes care of zeroing the relevant region and won't
      change values outside of it. */
   pivots=sys->ludata->pivlist;
   mtx=sys->factors;

   sys->smallest_pivot = MAXDOUBLE;
   last_row = pivot_candidates.high = sys->rng.high;
   for( nz.row = sys->rng.low ; nz.row <= last_row ; ) {

      pivot_candidates.low = nz.col = nz.row;
      pivots[nz.row]=pivot = mtx_value(mtx,&nz);
      pivot = fabs(pivot);
      if( pivot > sys->pivot_zero &&
         pivot >= sys->ptol * mtx_row_max(mtx,&nz,&pivot_candidates,NULL) &&
         !col_is_a_spike(sys,nz.row) ) {
         /* Good pivot and not a spike: continue with next row */
         if( pivot < sys->smallest_pivot )
            sys->smallest_pivot = pivot;
         ++(nz.row);
         continue;
      }
      /* pivots for rows nz.row back to sys->rng->low are stored in pivots */
      /**
      	Row is a spike row or will
      	be when a necessary column
      	exchange occurs.
       **/
      eliminate_row(mtx,&(sys->rng),nz.row,tmp,pivots);
      /* pivot will be largest of those available. get size and value */
      pivot=mtx_row_max(mtx,&nz,&pivot_candidates,&(pivots[nz.row]));
      if( pivot <= sys->pivot_zero ) { /* pivot is an epsilon */
         /* Dependent row, drag to the end */
         mtx_drag(mtx,nz.row,last_row);
         number_drag(pivots,nz.row,last_row);
         --last_row;
#undef KAA_DEBUG
#ifdef KAA_DEBUG
         ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Warning: Row %d is dependent with pivot %20.8g",nz.row,pivot);
#endif /* KAA_DEBUG */
      } else {
         /* Independent row: nz contains best pivot */
         /* Move pivot to diagonal */
         mtx_swap_cols(mtx,nz.row,nz.col);
         mtx_drag( mtx , nz.row , sys->rng.low );
         number_drag(pivots,nz.row,sys->rng.low);
         if( pivot < sys->smallest_pivot )
            sys->smallest_pivot = pivot;
         ++(nz.row);
      }
   }

   sys->rank = last_row - sys->rng.low + 1;
}


static void rankijz_factor(linsolqr_system_t sys)
/**
	This is an alternate pivoting strategy introduced by Joe Zaher.
	it looks down and across for good pivots rather than just across.
	
 **/
{
   real64 biggest;
   mtx_coord_t nz, best;
   mtx_region_t candidates;
   real64 *tmp;
   real64 pivot, *pivots;
   int32 length;
   mtx_matrix_t mtx;

   length = sys->rng.high - sys->rng.low + 1;
   tmp = sys->ludata->tmp;
   /* eliminate row takes care of zeroing the relevant region and won't
      change values outside of it. */
   pivots=sys->ludata->pivlist;
   mtx=sys->factors;

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
         if( fabs(biggest) >= sys->pivot_zero ) {
            if( pivot < sys->pivot_zero || pivot < sys->ptol*fabs(biggest) ) {
               mtx_swap_rows(mtx,nz.row,best.row);
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
      eliminate_row(mtx,&(sys->rng),nz.row,tmp,pivots);
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
         mtx_swap_cols(mtx,nz.row,nz.col);
         mtx_drag( mtx , nz.row , sys->rng.low );
         number_drag(pivots,nz.row,sys->rng.low);
         if( pivot < sys->smallest_pivot )
            sys->smallest_pivot = pivot;
         ++(nz.row);
      }
   }

   sys->rank = candidates.row.high - sys->rng.low + 1;
}

/**
	The region to factor is first isolated by truncating the region
	provided to the largest square region confined to the matrix diagonal.
	It is presumed it will contain no empty rows or columns and that it has
	been previously reordered using linsolqr_reorder(sys,region,spk1)
	or a sane variant of spk1.
	This is the entry point for all ranki based strategies, regardless of
	pivot selection subtleties.
*/
int ranki_entry(linsolqr_system_t sys,mtx_region_t *region){
   struct rhs_list *rl;
   double comptime;

   CHECK_SYSTEM(sys);
   if( sys->factored )
      return 0;
   switch(sys->fmethod) {
   case ranki_kw:
   case ranki_jz:
   case ranki_ka: /* add new methods here */
     break;
   default:
     return 1;
   }

   if(ISNULL(sys->ludata))
      return 1;

   if( NOTNULL(sys->inverse) )
      mtx_destroy(sys->inverse);
   sys->inverse=NULL;
   if( NOTNULL(sys->factors) )
      mtx_destroy(sys->factors);
   if( region == mtx_ENTIRE_MATRIX ) determine_pivot_range(sys);
   else square_region(sys,region);

   sys->factors = mtx_copy_region(sys->coef,region);
   sys->rank = -1;
   sys->smallest_pivot = MAXDOUBLE;
   for( rl = sys->rl ; NOTNULL(rl) ; rl = rl->next )
      rl->solved = FALSE;
   ensure_capacity(sys);
   ensure_lu_capacity(sys);

   comptime = tm_cpu_time();
   switch(sys->fmethod) {
   case ranki_ka:
   case ranki_kw:
     rankikw_factor(sys);
     break;
   case ranki_jz:
   default:
     rankijz_factor(sys);
   }
 /* no longer done automatically as noone usually cares.
   calc_dependent_rows_ranki1(sys);
   calc_dependent_cols_ranki1(sys);
 */
   sys->factored = TRUE;

#undef KAA_DEBUG
#if KAA_DEBUG
   comptime = tm_cpu_time() - comptime;
   ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Time for Inversion = %f",comptime);
   ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-zeros in Inverse = %d",mtx_nonzeros_in_region(sys->factors,region));
#endif
   return 0;
}

/**
	Forward substitute.  It is assumed that the L (or U) part of
	sys->factors is computed.  This function converts c to x in place.  The
	values are stored in arr indexed by original row number (or original
	column number).

	   transpose = FALSE:                  transpose = TRUE:
	                                        T
	   L x = c                             U x = c

	The following formula holds:
	   0<=k<r ==> x(k) = [c(k) - L(k,(0..k-1)) dot x(0..k-1)] / L(k,k)
	or
	   0<=k<r ==> x(k) = [c(k) - U((0..k-1),k) dot x(0..k-1)] / U(k,k)

	U(k,k) is 1 by construction. L(k,k) is stored in sys->ludata->pivlist[k]
	and in matrix.
 **/
void forward_substitute(linsolqr_system_t sys,
                               real64 *arr,
                               boolean transpose){
   mtx_range_t dot_rng;
   mtx_coord_t nz;
   real64 sum, *pivlist;
   mtx_matrix_t mtx;
   int32 dotlim;
   boolean nonzero_found=FALSE;

   mtx=sys->factors;
   pivlist=sys->ludata->pivlist;
   dot_rng.low = sys->rng.low;
   dotlim=dot_rng.low+sys->rank;
   if (transpose) {     /* arr is indexed by original column number */
     for( nz.col=dot_rng.low; nz.col < dotlim; ++(nz.col) ) {
       register int32 org_col;

       dot_rng.high = nz.col - 1;
       org_col = mtx_col_to_org(mtx,nz.col);
       if (arr[org_col]!=D_ZERO) nonzero_found=TRUE;
       if (nonzero_found) {
         sum=mtx_col_dot_full_org_vec(mtx,nz.col,arr,&dot_rng,TRUE);
         /* arr[org_col] = (arr[org_col] - sum)  / D_ONE */;
         arr[org_col] -=sum;
       }
     }

   } else {             /* arr is indexed by original row number */
     for( nz.row=dot_rng.low; nz.row < dotlim; ++(nz.row) ) {
       register int32 org_row;

       dot_rng.high = nz.row - 1;
       org_row = mtx_row_to_org(mtx,nz.row);
       if (arr[org_row]!=D_ZERO) nonzero_found=TRUE;
       if (nonzero_found) {
         sum = mtx_row_dot_full_org_vec(mtx,nz.row,arr,&dot_rng,TRUE);
/*
         nz.col = nz.row;
         arr[org_row] = (arr[org_row] - sum) / mtx_value(mtx,&nz);
*/
         arr[org_row] = (arr[org_row] - sum) / pivlist[nz.row];
       }
     }
   }
}

/**
	Backward substitute.  It is assumed that the U (or L) part of
	sys->factors is computed.  This function converts rhs to c in place.  The
	values are stored in arr indexed by original row number (or original
	column number).

	  transpose = FALSE:                  transpose = TRUE:
	                                       T
	  U c = rhs                           L c = rhs

	The following formula holds:
	transpose=FALSE: (the usual for J.dx=-f where rhs is -f)
	   0<=k<r ==> c(k) = [rhs(k) - U(k,(k+1..r-1)) dot c(k+1..r-1)] / U(k,k)
	   working up from the bottom.
	or
	transpose=TRUE:
	   0<=k<r ==> c(k) = [rhs(k) - L((k+1..r-1),k) dot c(k+1..r-1)] / L(k,k)
	   working right to left.

	U(k,k) is 1 by construction. L(k,k) is stored in sys->ludata->pivlist[k]
	and in matrix.
 **/
void backward_substitute(linsolqr_system_t sys,
                                real64 *arr,
                                boolean transpose){
   mtx_range_t dot_rng;
   mtx_coord_t nz;
   real64 sum, *pivlist;
   mtx_matrix_t mtx;
   int32 dotlim;
   boolean nonzero_found=FALSE; /* once TRUE, substitution must be done
                                   over remaining rows/cols */

   dot_rng.high = sys->rng.low + sys->rank - 1;
   dotlim=sys->rng.low;
   mtx=sys->factors;
   pivlist=sys->ludata->pivlist;
   if (transpose) {     /* arr is indexed by original column number */
     for( nz.col = dot_rng.high ; nz.col >= dotlim ; --(nz.col) ) {
       register int32 org_col;

       dot_rng.low = nz.col + 1;

       org_col = mtx_col_to_org(mtx,nz.col);
       if (arr[org_col]!=D_ZERO) nonzero_found=TRUE;
       if (nonzero_found) {
         sum= mtx_col_dot_full_org_vec(mtx,nz.col,arr,&dot_rng,TRUE);
/*
  reminders:
         nz.row = nz.col;
         arr[org_col] = (arr[org_col] - sum) / mtx_value(mtx,&nz);
*/
         arr[org_col] = (arr[org_col] - sum) / pivlist[nz.col];
       }
     }
   } else {             /* arr is indexed by original row number */
     /* we are working from the bottom up */
     for( nz.row = dot_rng.high ; nz.row >= dotlim ; --(nz.row) ) {
       register int32 org_row;

       dot_rng.low = nz.row + 1;
       org_row = mtx_row_to_org(mtx,nz.row);

       if (arr[org_row]!=D_ZERO) nonzero_found=TRUE;
       if (nonzero_found) {
         sum= mtx_row_dot_full_org_vec(mtx,nz.row,arr,&dot_rng,TRUE);
/*
  reminders:
         nz.row = nz.col;
         arr[org_row] = (arr[org_row] - sum) / D_ONE;
*/
         arr[org_row] -= sum;
       }
     }
   }
}


int ranki_solve(linsolqr_system_t sys, struct rhs_list *rl){
   backward_substitute(sys,rl->varvalue,rl->transpose);
   forward_substitute(sys,rl->varvalue,rl->transpose);
   zero_unpivoted_vars(sys,rl->varvalue,rl->transpose);
   return 0;
}
