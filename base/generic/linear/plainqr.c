/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
	Author: Benjamin A Allan
	Last in CVS: $Revision: 1.9 $ $Date: 1997/07/18 12:15:22 $ $Author: mthomas $
*/

#include "plainqr.h"

#include "linsolqr_impl.h"
#include "ranki_impl.h"

#include <general/mathmacros.h>
#include <utilities/ascConfig.h>
#include <utilities/error.h>

#include <math.h>

/***************************************************************************\
  CPQR implementation.
  an insert to linsolqr.c of a column pivoted (with ptol) householder qr
  method.
\***************************************************************************/
#define CPQR_DEBUG FALSE

#ifndef SQR
#define SQR(x) (x)*(x)
#endif
/**
 *** Finds the largest rectangle within the region given which has the
 *** UPPERLEFT corner on the diagonal. Also sets sys->rng to be the
 *** appropriate range of columns to factorize.
 *** Returns 1 if for any reason it cannot do so.
 **/
static int ul_rectangle_region(linsolqr_system_t sys, mtx_region_t *r)
{
  int32 cmax;
  if (ISNULL(sys) || ISNULL(r) || ISNULL(sys->coef)) return 1;
  cmax = mtx_order(sys->coef)-1;
  if ( r->row.low < 0 || r->row.high < 0 ||
       r->col.low < 0 || r->col.high < 0 ||
       r->row.low > cmax || r->row.high > cmax ||
       r->col.low > cmax || r->col.high > cmax ||
       r->col.low > r->col.high || r->row.low > r->row.high
     ) {
    CONSOLE_DEBUG("Region rows=[%d,%d], cols=[%d,%d]",r->row.low,r->row.high,r->col.low,r->col.high);
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad matrix region specified (cmax = %d)",cmax);
    return 1;
  }
  if (r->row.low > r->col.low) {
    r->col.low = r->row.low;
  } else {
    r->row.low = r->col.low;
  }
  if ( r->col.low > r->col.high || r->row.low > r->row.high ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Region given off diagonal.\n");
    return 1;
  }
  sys->rng.low = r->col.low;
  sys->rng.high = MIN(r->col.high,r->row.high);
#if CPQR_DEBUG
  CONSOLE_DEBUG("factor region: rows (%d,%d) cols (%d,%d)\n",
                  r->row.low,r->row.high, r->col.low,r->col.high);
  CONSOLE_DEBUG("factor range: cols %d- %d\n",sys->rng.low,sys->rng.high);
#endif
  return 0;
}


static real64 cpqr_compute_alpha(real64 * const alpha,
                                       const mtx_matrix_t mtx,
                                       const mtx_region_t * const reg)
/**
 *** Computes alphas for columns in a region indicated by reg.
 *** alpha are Frobenius norms : sqrt( sum(sqr(aij)|i,j) )
 *** as opposed
 *** to matrix 2-norm: (the largest eigenvalue of At dot A, see stewart p 180)
 *** or the matrix 1 norm: max( sum(abs(aij) | i) |j)
 *** or the matrix inf norm: max( sum(abs(aij) | j) |i)
 *** Returns the Frobenius norm of the region calculated.
 *** Stores the alphas in curcol order in the vector alpha given.
 *** If it turns out dragging this cur ordered alpha ends up taking
 *** much more time than the pivot search, the alpha storage format
 *** should change to org ordered and the search do the depermutation.
 **/
{
  int32 col;
  real64 anorm = D_ZERO, tsqr;
  for (col = reg->col.low; col <= reg->col.high; col++) {
    tsqr = mtx_sum_sqrs_in_col(mtx,col,&(reg->row));
    alpha[col] = sqrt(tsqr);
#if CPQR_DEBUG
    CONSOLE_DEBUG("alpha[%d](org %d) = %g\n",
      col,mtx_col_to_org(mtx,col),alpha[col]);
#endif
    anorm += tsqr;
  }
#if CPQR_DEBUG
  CONSOLE_DEBUG("region frobenius norm: %g\n",sqrt(anorm));
#endif
  return sqrt(anorm);
}


static int cpqr_reduce_region(linsolqr_system_t sys,
                               mtx_region_t *r,
                               int32 ccol)
/*
 * Reduce, or recompute as needed, the column norms of the region r
 * given less its leading row and column.
 * Assumes the leftmost column of the region is empty in the topmost row!
 * When done, also increases r->row.low and r->col.low by 1.
 * RESUM is an adjustable parameter to control when to recompute the
 * column norm from scratch.
 * This function does not update the overall A norm.
 * returns 1 if ccol was changed, 0 if not.
 */
#ifdef RESUM
#undef RESUM
#endif
#define RESUM (double)1.0e+8
{
  real64 sanew, as; /* sanew is pronounced S. A. New */
  real64 *data, *alpha;
  int32 *idata, lim, i;
  int ret;

  idata = sys->qrdata->sp.idata;
  data = sys->qrdata->sp.data;
  alpha = sys->qrdata->alpha;
  ret = 0;

  /* get top row incidence */
  mtx_cur_row_sparse(sys->factors,r->row.low, &(sys->qrdata->sp),
                     mtx_ALL_COLS, mtx_SOFT_ZEROES);
  /* bump down or recompute the changed rows. an orthogonal transformation
     is alleged to not change the norms, so it is sufficient reduce the
     norms with a little checking for floating point inanity. */
  for (i=0, lim = sys->qrdata->sp.len;
       i < lim;
       i++) {
    int32 col;

    col = idata[i];
    if (col == ccol) ret = 1;
    as = SQR(alpha[col]);
    sanew = as - SQR(data[i]);
    if (sanew * RESUM <= as) {
      alpha[col] = sqrt(sanew);
    } else {
      alpha[col] = sqrt(mtx_sum_sqrs_in_col(sys->factors,col,&(r->row)));
    }
  }
  r->row.low++;
  r->col.low++;
  return ret;
}
#undef RESUM

static real64 cpqr_permute(linsolqr_system_t sys,
                                 int32 newcol,
                                 mtx_range_t *active)
/**
 ***  Permute the region bounded by active and search so that
 ***  the newcol given is now on the left edge. Rows may also
 ***  be permuted if deemed wise.
 ***  In particular if things have not gone in some measure dense:
 ***
 ***    For a square or tall region,
 ***    the diagonal location (newcol,newcol) will be dragged up to
 ***    (active.low, active.low). That this should be a col swap
 ***    instead is debatable and should be experimentally tested later.
 ***
 ***    For a wide region, the newcol is dragged up if it is currently
 ***    within the range active, otherwise it is swapped to active.high
 ***    and dragged up to active.low (n log n ouch) in hopes that
 ***    the column punted from range is one most full of junk we don't
 ***    want to use in the future. This strategy is, again, debatable.
 ***
 ***  System vectors (alpha mainly) will be permuted to stay consistent.
 ***  We should probably also return the sparse corresponding to this
 ***  column at the end of permutation.
 **/
{
  real64 tmp;
#if CPQR_DEBUG
  CONSOLE_DEBUG("cpqr_permuted called to move %d back to %d\n",
    newcol,active->low);
#endif
  if (newcol <= active->low) return D_ZERO;
  if (newcol > active->high) {
    /* swap cols */
    mtx_swap_cols(sys->factors,newcol,active->high); /* also does hhvects */
    /* likewise alphas */
    tmp = sys->qrdata->alpha[newcol];
    sys->qrdata->alpha[newcol] = sys->qrdata->alpha[active->high];
    sys->qrdata->alpha[active->high] = tmp;
    newcol = active->high; /* prepare for drag */
  }
  mtx_drag(sys->factors,newcol,active->low);
  number_drag(sys->qrdata->alpha,newcol,active->low);
  return D_ZERO;
}

static boolean cpqr_get_householder(linsolqr_system_t sys,
                                    int32 curcol,
                                    mtx_range_t *rng,
                                    mtx_sparse_t *sp)
/**
 *** A Householder matrix is H = I - hhcol dot Transpose(hhcol)
 *** where hhcol is a Householder vector constructed so H dot x = -s * e1.
 *** I.e. we want a vector hhcol so H zeros all below the diagonal.
 *** We suppose the matrix column (x) being transformed is well-scaled:
 *** i.e. no gymnastics to avoid over or underflow are needed.
 *** The transform H is unitary. At present note we are constructing
 *** hhcol so that no multiplier is necessary in applying hhcol later.
 ***
 *** 4 Cases:
 ***
 *1* If there is nothing in the column (or the column is negligible)
 *1* we return FALSE. tau and alpha [curcol] are set to 0.
 ***
 *2* If there is only one nonzero and it is at the top, we are already
 *2* in the desired form, so sp->len gets set to 0 as does tau[curcol].
 *2* For consistency with R in this case we also flop the sign on alpha[curcol].
 *2* We return TRUE.
 ***
 *** Calculate the Householder vector hhcol as follows
 *** (compare to Intro to Matrix Computations, Stewart, 1971, p233):
 ***
 *** s = x^T dot x = norm2(x) a.k.a. alpha[i] with appropriate sign.
 *** e1 = unit vector: [1,0,...,0].
 *** hhcol = (Sqrt(s^2 + x1*s))^(-1) (x + s dot e1)
 ***  - the order of operations has been slightly improved.
 ***  - the topmost of element, x1, of x need not be nonzero.
 ***  - hhcol returned in sp.
 ***  - the transform vector (sp) is not put back into any matrix.
 ***  - tau has been eliminated (=1.0) from Stewart's algorithm.
 ***
 ***  In more particular:
 ***
 *3*  If x1 != 0:
 *3*  0) alpha = 2norm of curcol, precomputed.
 *3*  1) x1 = A(c,c)
 *3*  2) s = alpha = copysign(alpha,x1) (avoid cancellation in x1+s)
 *3*  3) xdothhcoll = s*(x1+s)          (Stewart's pi, local alias t)
 *3*  4) x1 = (x1+s)
 *3*  5) tau[curcol] = 1           unless xdothhcol==0 in which case tau=0
 *3*                               and we get out of here mighty confused.
 *3*                               Note, tau can now be stored as a boolean.
 ***
 *4*  If x1 == 0:
 *4*  0) alpha = 2norm of curcol, precomputed.
 *4*  1) x1=0.
 *4*  2) s = alpha
 *4*     R(c,c) is now -s, (alias -alpha[c].)
 *4*  3) xdothhcoll = s*s               (Stewart's pi, alias t)
 *4*  4) new incidence x1 = s           (appended at end of sparse)
 *4*  5) tau[curcol] = 1           unless xdothhcol==0 in which case tau=0
 *4*                               and we get out of here.
 ***
 3*4  Then, for x1 == 0 or x1 != 0:
 3*4    x = x/sqrt(xdothhcol)
 ***
 *** The result is a vector stuffed in sp yielding orthogonal H.
 *** Any time you want to do something with the normalized vector, don't
 *** forget to check tau[c] of the coefficient for the operation.
 *** The sparse returned has org row indexed data.
 ***
 *** Be sure to watch the signs if updating alpha later: not just the norm.
 *** It is best if mtx(curcol,curcol) is the largest element in its column,
 *** but hey, we assumed well scaled, so who cares?
 ***
 ! ! Column curcol will have all elements of x removed from it in all cases.
 ***
 *** On an empty or zero curcol this will return FALSE, but this should
 *** never have been called with such a column. If FALSE, nothing was
 *** done to A (except perhaps a 0.0 valued incidence was removed)
 *** and alpha and tau[curcol] are 0.
 **/
{
  real64 s,t;
  real64 *alpha /* , x1 */;
  mtx_matrix_t mtx;
  int32 orgrow,acc_loc,i;

  alpha = sys->qrdata->alpha;
  if ( alpha[curcol] == D_ZERO) {
    /* somebody dumb called us and we set them straight. Case 1 */
    sys->qrdata->tau[curcol] = D_ZERO;   /* signal weirdness */
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with alpha[col] = 0.\n");
    return FALSE;
  }

  mtx = sys->factors;

  /* find the org row index of A(c,c) */
  orgrow = mtx_row_to_org(mtx,curcol);

  /* fetch and empty the column we build u from */
  mtx_steal_org_col_sparse(mtx,curcol,sp,rng);
#if CPQR_DEBUG
  CONSOLE_DEBUG("cpqr_get_householder found column %d:\n",curcol);
  mtx_write_sparse(stderr,sp);
#endif
  /* check sp sanity  backup Case 1 */
  if ( sp->len < 1 ) {
    alpha[curcol] = D_ZERO; /* somebody lied and we set them straight */
    sys->qrdata->tau[curcol] = D_ZERO;   /* signal weirdness */
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with empty column, norm = 0.\n");
    return FALSE;
  }
  /* check for Identity = H , Case 2 */
  if (sp->len == 1 && mtx_row_to_org(mtx,curcol) == sp->idata[0]) {
    /**
     ** singletons cheap: Anew = Aold, no fill.
     ** tau(col)= 0.0  (no transform signal later)
     ** alpha(col) = -Anew(col,col) [alpha(i)= -Rii, remember?]
     **/
    sys->qrdata->tau[curcol] = D_ZERO;/* should be defaulted, but in case */
    sys->qrdata->alpha[curcol] = (-sp->data[0]);
    sp->len = 0; /* tell outer loop to screw off */
    return TRUE;
  }
  /* search to see where if we have A(c,c) incident. if no, acc_loc=len */
  for (acc_loc=0; acc_loc < sp->len && orgrow != sp->idata[acc_loc]; acc_loc++);
  /* whether it is best to search this forward or backward is questionable.
     in any case, acc_loc should be the location of acc, or the new location
     for a created acc when done. At beginning, with relatively little fill
     it shouldn't matter. as fill grows, it may be better to search backward
     except in columns where the diagonal is prior fill. */

  if (acc_loc < sp->len) {       /* proceeding if A(c,c) found */
    /* x1 = sp->data[acc_loc]; */
    /* alpha = -Rcc, change sign if needed and add to x1 */
    s = alpha[curcol] = copysign(alpha[curcol],sp->data[acc_loc]);
    sp->data[acc_loc] += s;
    t = 1/sqrt(s*(sp->data[acc_loc]));
    /*
       If, like some gcc compilers, your compiler is too amazingly braindead
       to have copysign in the math library as IEEE recommends, link in the
       math library on your system that DOES follow the recommendation.
       On HPs this is /lib/pa1.1/libm.a or libM.a.
    */
  } else {                       /* A(c,c) is not there yet, append */
    /* x1 = 0; */
    /* s = alpha[curcol]; */
    sp->len++;
    sp->idata[acc_loc] = orgrow;
    sp->data[acc_loc] = alpha[curcol];
    t = 1/alpha[curcol];
  }

  /*
   If t is 0, then the current column will be effectively
   empty somehow and we will have a zero pivot.  In this case, we
   can not use this column so GET_HOUSEHOLDER is FALSE
   and column is undisturbed, except for having been cleaned.
   An untrapped underflow must have occurred, because we had a nonzero above.
  */
  if (t != D_ZERO) {
    register double apiv;
    /* scale the hhcol by stewarts 1/pi sqrt, essentially */
    for (i = 0; i < sp->len; i++) {
      sp->data[i] *= t;
    }
    /* record tau. archaic. */
    sys->qrdata->tau[curcol] = D_ONE;
    /* record min pivot size */
    apiv = fabs(alpha[curcol]);
#if CPQR_DEBUG
    CONSOLE_DEBUG("pivotsize = %g\n",apiv);
    mtx_write_sparse(stderr,sp);
#endif
    if (apiv < sys->smallest_pivot) {
      sys->smallest_pivot = apiv;
    }
    return TRUE;
  } else {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with small column, underflow.\n");
    return FALSE;
  }
}

static void cpqr_apply_householder(linsolqr_system_t sys,
                                   int32 curcol,
                                   mtx_region_t *reg)
/**
 *** Takes the rectangle region A indicated by reg and does a Householder
 *** transformation on it in place. Curcol should be the same as reg->col.low.
 *** The first column (curcol) is transformed into
 *** the orthogonal Householder vector unless this is impossible.
 *** The householder vector is stored in hhvects and removed from
 *** the matrix.
 ***
 *** By construction the curcolth diagonal element of R is the negative of
 *** alpha[curcol] on exit, but the element is not stored in R.
 *** The remaining columns in reg are transformed as follows:
 ***
 ***     A' = A' - hhcol dot hhcol dot A'
 ***     where A' is A sans column curcol.
 ***
 *** This is equivalent to:       T
 ***    A = [ I  - hhcol dot hhcol ] dot A = H dot A
 *** This will change the sparsity of A.
 ***
 **/
{

 /* what to do with this? scrap?
  if (rng->high < rng->low) {
    return;
  }
 */
  /* Do transform even if last col of square region (to get matrices right). */
  if (cpqr_get_householder(sys,curcol,&(reg->row),&(sys->qrdata->sp))) {
    mtx_sparse_t *sp;
    sp = &(sys->qrdata->sp);

    if (sp->len > 0) {
      mtx_householder_transform_region(sys->factors,sys->qrdata->tau[curcol],
                                       sp,reg,D_ZERO,FALSE);
      mtx_fill_org_col_sparse(sys->qrdata->hhvects,curcol,sp);
    }
  }
  /* else we have a very empty column and what are we doing here? */
}


/**
 ***  FACTORIZATION
 ***  -------------
 ***  The QR factorization calculated of the rectangular
 ***  sys->qrdata->facreg using columnwise Householder.
 ***  If the region is taller than wide, the solution will
 ***  will be that of linear least squares. If the region
 ***  is wider than tall, the solution will be one with
 ***  a basis chosen to give large diagonal
 ***  values in R: a reasonably well conditioned basis
 ***  except in rare circumstances.
 ***
 ***  d = sys->rng,
 ***  Q = H(NR-1)H(NR-2)..H(1),
 ***  R = Q*A  where A is the coef matrix,
 ***  H(i) = I - v(i)*Transpose(v(i)) with d.low <= i <= d.high.
 ***
 ***  The hhvects matrix contains the H(i) in the lower triangle,
 ***  while the diagonal of R is stored densely (in col order)
 ***  as -alpha[] and the off-diagonal elements of R are in the
 ***  superdiagonal triangle of factors.
 ***
 ***  Note we are using a normalized v(i) that avoids extra
 ***  multiplications when handling multiple RHS. This is
 ***  done at the cost of some extra *,sqrt operations during
 ***  factorization. ||v(i)||_2 = 2.0.
 ***  These operations are trivial compared to the cost of
 ***  Householder transformations.
 **/
int cpqr_factor(linsolqr_system_t sys){

  mtx_range_t active; /* range within sys->rng yet to be pivoted */
  mtx_range_t search; /* range of columns to be examined for pivots */
  mtx_region_t ak;    /* reduced A at each step */
  int32 lastmax,col,newcol = (-1);
  int32 colfound = 0, *ibuf;
  real64 *rbuf, *alpha;
  struct qr_auxdata *data;
  int pivotstyle; /* -1, no pivot, 0 ptol pivoting 1, exact col pivoting */

  /* set method variation based on ptol */
  if (sys->ptol == D_ZERO) {
    pivotstyle = -1;
  } else {
    if (sys->ptol == D_ONE) {
      pivotstyle = 1;
    } else {
      pivotstyle = 0;
    }
  }

  /* do initializations need for factoring or refactoring here */
  data = sys->qrdata;
  active = sys->rng;
  search = data->facreg.col;
  ak = data->facreg;
  lastmax = 0;
  data->nu = D_ZERO;
  ibuf = (int *)data->hhrow;
  rbuf = data->hhcol;
  alpha = data->alpha;

  if (pivotstyle >= 0) {
    /* compute starting column norms and matrix norm*/
    data->anu = data->asubnu =
      cpqr_compute_alpha(data->alpha,sys->factors,&(data->facreg));
  }

  for (col = active.low; col <= active.high;) {
    /* select newcol, setting colfound to 1 if pivotable. If
       nothing is pivotable or not pivoting, newcol is active.low */
    switch (pivotstyle) {
      case -1:
        newcol = active.low;
        alpha[newcol] = sqrt(mtx_sum_sqrs_in_col(sys->factors,
                                                 newcol,&(ak.row)));
      break;
      case 0:
        /* get the distance of the pivot away from where we are (col). */
        if (lastmax == 0) {
          newcol = find_pivot_number(&(alpha[col]), search.high-search.low+1,
                                     sys->ptol,sys->pivot_zero,
                                     ibuf,rbuf,&lastmax);
        } else {
          /* the max hasn't moved from where it was. Don't search past it.
             Since col has increased one, the previous value is now the len. */
          newcol = find_pivot_number(&(alpha[col]), lastmax,
                                     sys->ptol,sys->pivot_zero,
                                     ibuf,rbuf,&lastmax);
        }
        newcol += col; /* convert distance to pivot location. */
      break;
      case 1:
        newcol = find_pivot_number(&(alpha[col]), search.high-search.low+1,
                                   sys->ptol,sys->pivot_zero,
                                   ibuf,rbuf,&lastmax);
        lastmax = 0;
        newcol += col; /* convert distance to pivot location. */
      break;
      default: /* not reached */
      break;
    }
    if (alpha[newcol] < sys->pivot_zero) {
      colfound = 0;
      /* jump out of the loop: remaining cols singular, or in case of
         no pivoting, we have hit our first empty column and are stuck. */
      break;
    } else {
      colfound = 1;
    }
    ++(sys->rank);
    cpqr_permute(sys,newcol,&active);
#if CPQR_DEBUG
    mtx_write_region_human_cols(stderr,sys->factors,&ak);
#endif
    cpqr_apply_householder(sys,col,&ak);
    if (pivotstyle >= 0) {
      if (cpqr_reduce_region(sys,&ak,lastmax+col)) lastmax = 0;
    } else {
      ak.col.low = (++ak.row.low);
    }
    search.low = active.low = (++col);
  }

#if CPQR_DEBUG
  CONSOLE_DEBUG("R\n");
  mtx_write_region_human_cols(stderr,sys->factors,&(data->facreg));
  CONSOLE_DEBUG("hhcols\n");
  mtx_write_region_human_cols(stderr,data->hhvects,&(data->facreg));
  for (col = data->facreg.col.low; col <= data->facreg.col.high; col++) {
    CONSOLE_DEBUG("alpha[%d](org %d) = %g\n",
      col,mtx_col_to_org(sys->factors,col),alpha[col]);
  }
#endif
  if (!colfound) {
    /* handle setup for singularity, if any */
    return 1;
  }
  return 0;
}

#define TAU_ONE TRUE
static void cpqr_forward_eliminate(linsolqr_system_t sys,
                                   real64 *arr,
                                   boolean transpose)
/**
 *** cpqr_forward_eliminate(sys,c,transpose)
 ***  convert rhs to c in place (only stored u of H= I-tau u dot Transpose(u)
 ***
 ***  transpose=FALSE
 ***    c=Q.rhs.
 ***    (assuming independent columns/rows 1 to rank)
 ***    for j= 1 to rank (apply H(j) to rhs, HH foward elim)
 ***      if (tau(j)!= 0)
 ***        w=tau(j)* (Transpose(u(j)) dot c)
 ***        if (w!=0)
 ***          c -= w*u(j)
 ***        endif
 ***      endif
 ***    endfor
 ***
 ***  transpose=TRUE
 ***    Solve Transpose(R).c=rhs.  (given R in untransposed form)
 ***    0<=k<r ==> x(k) = [c(k) - R((0..k-1),k) dot x(0..k-1)]/R(k,k)
 ***
 ***  Neither of these operations require range checking the dots and
 ***  adds as in both cases we know the rest of the column in question
 ***  are empty.
 ***
 ***  If !transpose && TAU_ONE, then assumes tau[i]=1 if tau[i]!=0.
 **/
{
  mtx_range_t dot_rng;
  real64 sum;
  mtx_matrix_t mtx;

  dot_rng.high = sys->rng.low + sys->rank -1;

  if (transpose) { /* ok */
    /* arr is indexed by original column number */
    int32 dotlim,col;
    const real64 *diag;
    register int32 org_col;

    mtx = sys->factors;
    diag = sys->qrdata->alpha;
    dot_rng.low = sys->rng.low;
    dotlim = dot_rng.low+sys->rank;
    /* 0 <= k <r */
    for( col=dot_rng.low; col <dotlim; ++col) {
      /* rows of transpose are cols of R */
      dot_rng.high = col-1;
      /* sum = R((0..k-1),k) dot x(0..k-1) */
      sum = mtx_col_dot_full_org_vec(mtx,col,arr,mtx_ALL_ROWS,TRUE);
      org_col =  mtx_col_to_org(mtx,col);
      /* arr[org_col] = (arr[org_col] - sum)/ -diag[k]; */
      arr[org_col] = (sum - arr[org_col])/diag[col];
    }
  } else {

    /* arr is indexed by original row number */
    /* apply Q to it */
    real64 *tau;

    mtx = sys->qrdata->hhvects;
    tau = sys->qrdata->tau;

    for( dot_rng.low = sys->rng.low;
         dot_rng.low <= dot_rng.high;
         dot_rng.low++) {
      if (tau[dot_rng.low]!=D_ZERO) {
        sum = mtx_col_dot_full_org_vec(mtx,dot_rng.low,arr,mtx_ALL_ROWS,FALSE);
        if (sum != D_ZERO) {
#if TAU_ONE
          mtx_org_vec_add_col(mtx,arr,dot_rng.low,-sum,mtx_ALL_ROWS,FALSE);
#else
          mtx_org_vec_add_col(mtx,arr,dot_rng.low,
                              -sum*tau[dot_rng.low],mtx_ALL_ROWS,FALSE);
#endif
        }
      }
    }
  }
}

static void cpqr_backward_substitute(linsolqr_system_t sys,
                                     real64 *arr,
                                     boolean transpose)
/**
 ***  cpqr_backward_substitute(sys,rhs,transpose):
 ***  It is assumed that the R (or Q) part of sys->factors is computed.
 ***  This function converts rhs to c in place by solving one of the
 ***  following:
 ***
 ***  transpose = FALSE               transpose = TRUE
 ***    R.c = rhs                       Q.c = rhs
 ***
 ***  The following formulae hold:
 ***  (for rank=r, upper left is R(0,0) transpose= FALSE
 ***     r>k>=0 --> c(k) = [rhs(k) - R(k,(k+1..r-1)) dot c(k+1..r-1)] / R(k,k)
 ***     -R(k,k) is assumed to be in sys->qrdata->alpha[k]
 ***  or
 ***  (for rank=r, upper left is Q(0,0) transpose= TRUE
 ***    c=Transpose(Q).rhs ==>
 ***    for k = rank..1
 ***      c = H(k).rhs = rhs - tau*(Transpose(uk) dot rhs) *uk
 ***      rhs <-- c
 ***    endfor
 ***
 ***  For transpose == FALSE, this requires filtering out the leftover
 ***  righthand columns of R unless the problem is square or tall and of
 ***  full rank.
 ***  For transpose == TRUE, this uses the fast add and dot.
 ***  This counts on the fact that the diagonal was removed from R during
 ***  processing and is stored in alpha.
 ***  If transpose && TAU_ONE, then assumes tau[i]=1 if tau[i]!=0.
 **/
{
  mtx_range_t dot_rng;
  real64 sum;
  mtx_matrix_t mtx;
  int32 dotlim;

  dot_rng.high = sys->rng.low+sys->rank -1; /* ultimate pivoted row/col */
  dotlim = sys->rng.low;                    /* upleft corner row/col */

  if (transpose) {
    /* arr is indexed by original column number */
    /* apply Q */
    real64 *tau;
    mtx = sys->qrdata->hhvects;
    tau = sys->qrdata->tau;

/***    for k = rank..1
 ***      c = H(k).rhs = rhs - tau*(Transpose(uk) dot rhs) *uk
 ***      rhs <-- c
 ***    endfor
*/
    for (dot_rng.low=dot_rng.high; dot_rng.low >=dotlim; dot_rng.low--) {
      if (tau[dot_rng.low]!=D_ZERO) {
        sum = mtx_col_dot_full_org_vec(mtx,dot_rng.low,arr,mtx_ALL_ROWS,TRUE);
        if (sum != D_ZERO) {
#if TAU_ONE
          mtx_org_vec_add_col(mtx,arr,dot_rng.low,-sum, mtx_ALL_ROWS,TRUE);
#else
          mtx_org_vec_add_col(mtx,arr,dot_rng.low,-sum*tau[dot_rng.low],
                              mtx_ALL_ROWS,TRUE);
#endif
        }
      }
    }
  } else {

    int32 org_row,row;
    real64 *diag;

    mtx = sys->factors;
    diag = sys->qrdata->alpha;

    /* apply R */
    /* r >k>=0 we are working backwards through the pivoted rows.     */
    /* dot_rng is stuff to the right of the current pivot, (row,row),
       and within the basis columns. */

    if ( dot_rng.high == sys->qrdata->facreg.col.high ) {
      /* cool, we can use the fast math since all cols are in basis */
      for( row = dot_rng.high; row >= dotlim; --row) {
        dot_rng.low = row+1; /* our dot left edge is just after the pivot */

        /* sum = R(k,(k+1..r-1)) dot c(k+1..r-1) */
        sum = mtx_row_dot_full_org_vec(mtx,row,arr,mtx_ALL_COLS,TRUE);
        org_row = mtx_row_to_org(mtx,row);

        /* c(k) = [rhs(k) -sum] /R(k,k) */
        /* arr[org_row] = (arr[org_row] - sum) / -diag[row]; */
        arr[org_row] = (sum - arr[org_row])/diag[row];
      }
    } else {
      /* wah! we have to shuffle all the crap in R from either rank
         deficiency or extra cols */
      for( row = dot_rng.high; row >=dotlim; --row) {
        dot_rng.low = row+1; /* our dot left edge is just after the pivot */

        /* sum = R(k,(k+1..r-1)) dot c(k+1..r-1) */
        sum = mtx_row_dot_full_org_vec(mtx,row,arr,&dot_rng,TRUE);
        org_row = mtx_row_to_org(mtx,row);

        /* c(k) = [rhs(k) -sum] /R(k,k) */
        /* arr[org_row] = (arr[org_row] - sum) / -diag[row]; */
        arr[org_row] = (sum - arr[org_row])/diag[row];
      }
    }
  }
}
#undef TAU_ONE

/**
 ***  The region to factor is first isolated by truncating the region
 ***  provided to the largest rectangular region with an upper left
 ***  corner on the diagonal. (I.E. it may be over or under specified.)
 ***  Then the
 ***  It is presumed it will contain no empty rows or columns and that it has
 ***  been previously reordered using linsolqr_reorder(sys,region,tspk1 or
 ***  some other QR friendly method).
 ***  on exit, sys->factors, and sys->inverse will have been
 ***  permuted identically by solution process. sys->coef will not be
 ***  permuted.
 **/
int cpqr_entry(linsolqr_system_t sys,mtx_region_t *region){
   struct rhs_list *rl;
   boolean rank_deficient;
   mtx_region_t factor_region;

   CHECK_SYSTEM(sys);
   if( sys->factored )
      return 0;
   if( sys->fmethod!=plain_qr )
      return 1;
   if(ISNULL(sys->qrdata))
      return 1;

   if (region == mtx_ENTIRE_MATRIX) {
     determine_pivot_range(sys);
     factor_region = sys->reg;
   } else {
     factor_region = *region;
   }
   if (ul_rectangle_region(sys,&factor_region)) return 1;
   sys->qrdata->facreg = factor_region;

   if( NOTNULL(sys->inverse)  ) {
     mtx_destroy(sys->inverse);
     sys->inverse = NULL;
   }
   if( NOTNULL(sys->factors) ) mtx_destroy(sys->factors);

   sys->factors = mtx_copy_region(sys->coef, &(sys->qrdata->facreg));
   sys->qrdata->hhvects = mtx_create_slave(sys->factors);
   sys->rank = 0;
   sys->smallest_pivot = MAXDOUBLE;
   for( rl = sys->rl ; NOTNULL(rl)  ; rl = rl->next )
     rl->solved = FALSE;
   ensure_capacity(sys); /* this should zero the vectors if needed */
   ensure_qr_capacity(sys); /* this should zero the vectors if needed */

   rank_deficient = cpqr_factor(sys);
   if (rank_deficient) {
#if LINSOLQR_DEBUG
     int j;
#endif
     ERROR_REPORTER_HERE(ASC_PROG_WARNING,"cpqr found column rank %d of %d\n",sys->rank,
     sys->rng.high-sys->rng.low+1);
#if LINSOLQR_DEBUG
     CONSOLE_DEBUG("alpha vec:(curcol,val)\n");
       for (j=sys->qrdata->facreg.col.low;
            j<= sys->qrdata->facreg.col.high; j++)
         CONSOLE_DEBUG("alpha[%d] = %.8g\n",j,sys->qrdata->alpha[j]);
     CONSOLE_DEBUG("tau vec:(curcol,val)\n");
       for (j=sys->qrdata->facreg.col.low;
            j<= sys->qrdata->facreg.col.high; j++)
         CONSOLE_DEBUG("tau[%d] = %.8g\n",j,sys->qrdata->tau[j]);
#endif
   }
   sys->factored = TRUE;
   return 0;
}

/**
	Solve a previously qr factorized matrix with a rhs b.
	If b is not transposed (is org row ordered):
	c:=Q.b, then solve R.x=c for x.
*/
int cpqr_solve(linsolqr_system_t sys,struct rhs_list *rl){
  cpqr_forward_eliminate(sys,rl->varvalue,rl->transpose);
  cpqr_backward_substitute(sys,rl->varvalue,rl->transpose);
  /* doesn't the following destroy the least squares solution? */
  zero_unpivoted_vars(sys,rl->varvalue,rl->transpose);
  return 0;
}

/***************************************************************************\
  End of CPQR implementation.
\***************************************************************************/
