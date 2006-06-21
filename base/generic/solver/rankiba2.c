/*
 *  ranki_ba2 implementation functions.
 *  By:		Benjamin Andrew Allan
 *  Created:	11/96
 *  Copyright 1996 Benjamin A. Allan
 *
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: rankiba2.c,v $
 *  Date last modified: $Date: 1997/07/18 12:15:24 $
 *  Last modified by: $Author: mthomas $
 *
 *  The functions and algorithms implemented here should be exactly numerically
 *  equivalent to ranki_kw2, only the data structures for pivots and
 *  elimination differing.
 *  Here we will work in _really_ sparse elimination and a linked list
 *  representation of the diagonal until factorization is finished.
 *  This beast (with good reordering) should be competitive with ma28,
 *  even though it uses mtx which is much more flexible.
 *  It should beat the living daylights out of lu1sol in application,
 *  given that lu1sol can't solve multiple rhs and this will.
 *
 *  The fundamental notion that makes this factorization work is that
 *  in matrix algebra and LU in particular
 *  permutation is really irrelevant EXCEPT
 *  when executing the triangular backsolve (or equivalent) during
 *  the elimination of stuff under the diagonal.
 *  The already pivoted rows must be used in the correct sequence, and
 *  the nonzeros to be eliminated must be eliminated in the order that
 *  introduces no fill after elimination.
 *  Since we can use fancy data structures to track the absolute minimum
 *  of sequencing information, we can in fact do all the real work
 *  based on any consistent column/row indexing. The most convenient,
 *  and the one used is the org_ index.
 *  No sorting happens until the very latest time it is needed,
 *  which insures that only the minimum required sorting ever happens.
 *  Dual access to the nonzeros (by orgcol and by linked list) makes
 *  things really fast at a cost of memory = k*sys->capacity ints.
 *  k is a small 2 digit integer.
 */


#ifndef RBADEBUG
#define RBADEBUG 0
#endif
/* turns on more spew than you can possibly imagine.
 * has user (ballan) hardcoded file name dependencies.
 * Once the code is proven to work as kw2 on singular
 * systems, all the RBA stuff should be stripped.
 */

/* LINQR_DROP_TOLERANCE defined in main. */

#define EMPTY -1
/* the end of lists of pivots (possibly rejected)
 * or of nonzeros in elimination.
 * any nzdata with an orgcol of EMPTY is not
 * to be eliminated because it is not real.
 */

#define INSERTED(a) ((a).orgcol >= 0)
/* tell if an element a is active.
 * works for nzlistentries and pivlistentry.
 */

struct NZListEntry {
  int32 leftnz;		/* link to next nz to the left in the imaginary row. */
  int32 rightnz;	/* link to nz to the right in the imaginary row. */
                        /* left and right nz links are the orgcols, or EMPTY */
  int32 pseudo_curcol;	/* pseudo column number. = actual curcol right of pivot,
                         * = calculated from relative_location in elimination
                         * region left of pivot, and would be actual curcol
                         * if this was the older rankikw implementations.
                         */
  int32 orgcol;		/* org col of this nz. if  <0, element not inserted. */
  /*
   * Efficiency note:
   * at the present time (11/5/96) the code uses (extensively in debug mode)
   * EMPTY to denote the beginning and ending of nz lists.
   * This is unnecessarily wasteful and should be #if'd out in
   * favor counters which keep track of the list lengths. then
   * all that is needed to print the list is the head (or tail) and
   * the expected length.
   */
};
/* data structure for tracking nonzeros in the row being eliminated. */

struct PivListEntry {
  int32 orgrow; /* org row of pivot for this orgcol */
  int32 prev;   /* org col of "prev" pivot. prev -> toward upper left */
  int32 next;   /* org col of "next" pivot. next -> toward lower right */
  int32 relative_location;
  /* DEFAULT should be INT_MIN, which signals the org_col was
   * never used or attempted as a pivot.
   * For accepted pivots:
   * relative_location is the location in the final diagonal
   * we will obtain relative to the first accepted pivot.
   * The first accepted pivot has relative_location 0.
   * Pivots picked up just marching down the diagonal have
   * increasingly negative relative_location.
   * Pivots picked up after an elimination call will have
   * increasingly positive relative_location since they are
   * "dragged" to the upper left in the ranki scheme.
   * The whole point of this list data structure choice is to eliminate
   * the explicit data vector rotation that kills ranki_kw and jz on
   * larger problems.
   *
   * For reject pivots:
   * The rejected rows/cols accumulate at the bottom-right of the
   * "diagonal." Since they aren't really used, the relative_location
   * is meaningless, therefore it is DIAG_MIN just so we can distinguish
   * between rejects and never-touched cols.
   */
};
/* depending on efficiency, we may want to break up the pivlistentry
 * type into simple a bunch of parallel integer vectors.
 * As is, it makes the memory management easy and keeps most
 * accesses relatively page local.
 */

#define DIAG_MIN (INT_MIN+10)
/*
 *  Define a gap of 10 for good measure between what we consider
 * 'lower-rightmost' and the most negative 32 bit integer.
 */

#define EDSC 1234567890
#define EDEC 987654321
static
struct elimination_data {
  long startcheck;
  struct PivListEntry *pivdata;	/* org col indexed */
  struct NZListEntry *nzdata;	/* org col indexed */
  size_t capacity;	/* total entries allocated in pivdata */
  size_t length;	/* entries currently in use (possibly) */
  long endcheck;
} g_ba_elimdata = {EDSC,
                   NULL, NULL, (size_t)0,  (size_t)0,
                   EDEC};
/*
 * This provides a workspace for the pivot list while it is being
 * constructed during factorization. The contents of this
 * data space are indeterminate between calls to rankiba2_factor,
 * particularly if a floating point exception occurred.
 * pvidata is an array that is indexed by org_col of the matrix
 * being factored.
 * The reused nzdata space is also here. Between eliminations,
 * nzdata has certain expectation values that we maintain.
 * Between factorizations, nzdata content is undefined due to
 * possibility of floating point exceptions.
 */

/*
 * Conceptually we are growing a pivot list during factoring:
 *    eliminated & dragged pivots <- (first pivot) -> simply taken pivots
 * where (first pivot) has a relative location 0.
 * The pivot list is reduced to a vector only at the end of factorization,
 * for easy use by the triangular solution functions.
 * The matrix is not permuted to its final form until the factorization is
 * completed since we are doing everything off of the pivot list and
 * org row/col indices.
 * The yet to be pivoted columns and rows WILL be permuted to keep the
 * pivot search and rank defect handling the same.
 */

#if RBADEBUG
FILE *gscr = NULL;
#endif
/* file ptr for debugging */

/*
 * Allocates g_ba_elimdata and members to the size given.
 * If size given is 0, frees any allocated memory.
 * If init !=0, will do any appropriate initializations.
 * Note that g_ba_elimdata is a memory recycle structure,
 * so that it only grows in size, it never shrinks because
 * someone is likely to come along and ask it to be big
 * again. If memory is at a premium, it might pay to
 * call reset periodically instead of only at shutdown.
 *
 * Returns 0 if successful.
 * Returns 1 if insufficient memory.
 * If memory corruption is detected, we will reinit from
 * scratch, possibly discarding memory, and return 0 after
 * whining about the error.
 */
static
int reset_elimination_data(int32 size, int init)
{
  int i;
  struct NZListEntry *tmpn = NULL;
  struct PivListEntry *tmpp = NULL;

  /* check for errors */
  if (g_ba_elimdata.startcheck != EDSC || g_ba_elimdata.endcheck != EDEC ) {
    FPRINTF(stderr,"g_ba_elimdata corrupted! Reset may leak memory\n");
    g_ba_elimdata.pivdata = NULL;
    g_ba_elimdata.nzdata = NULL;
    g_ba_elimdata.capacity = (size_t)0;
    g_ba_elimdata.length = (size_t)0;
    g_ba_elimdata.startcheck = EDSC;
    g_ba_elimdata.endcheck = EDEC;
  }
  /* check for free */
  if (size == (size_t)0) {
    if (g_ba_elimdata.capacity != (size_t)0) {
      if (g_ba_elimdata.nzdata != NULL) {
        ascfree(g_ba_elimdata.nzdata);
      }
      if (g_ba_elimdata.pivdata != NULL) {
        ascfree(g_ba_elimdata.pivdata);
      }
      g_ba_elimdata.nzdata = NULL;
      g_ba_elimdata.pivdata = NULL;
      g_ba_elimdata.capacity = (size_t)0;
      g_ba_elimdata.length = (size_t)0;
    }
    return 0;
  }
  /* check for reuse/expansion */
  if ((size_t)size <= g_ba_elimdata.capacity) {
    g_ba_elimdata.length = size;
  } else {
     /* expand both */
    if (g_ba_elimdata.capacity == (size_t)0) {
      tmpp = ASC_NEW_ARRAY(struct PivListEntry,size);
      tmpn = ASC_NEW_ARRAY(struct NZListEntry,size);
    } else {
      tmpp = (struct PivListEntry *)
        ascrealloc(g_ba_elimdata.pivdata,size*sizeof(struct PivListEntry));
      tmpn = (struct NZListEntry *)
        ascrealloc(g_ba_elimdata.nzdata,size*sizeof(struct NZListEntry));
    }
    if (tmpp==NULL || tmpn==NULL) {
      return 1;
    } else {
      g_ba_elimdata.pivdata = tmpp;
      g_ba_elimdata.nzdata = tmpn;
      g_ba_elimdata.length = size;
      g_ba_elimdata.capacity = size;
    }
  }
  if (init) {
    for (i = 0; i <size; i++) {
      g_ba_elimdata.pivdata[i].relative_location = INT_MIN;
      g_ba_elimdata.nzdata[i].orgcol = EMPTY;
    }
  }
  return 0;
}

/*
 * Assumes nothing about leftorg.
 * Returns orgcol of leftmost element.
 * newi and orgcol must correspond to an element
 * definitely NOT already in the elimination nzlist.
 */
static
int32 rankiba_firstinsert_nz(int32 leftorg,
                             int32 newi,
                             int32 orgcol,
                             struct NZListEntry *nzdata,
                             int32 *rightorg)
{
  int32 pcc,searchcol;

  nzdata[orgcol].pseudo_curcol = newi;
  nzdata[orgcol].orgcol = orgcol;

  if (leftorg >= 0) {
    /*  not first element */
    pcc = nzdata[leftorg].pseudo_curcol;
#if RBADEBUG
    assert(pcc!=newi);
#endif
    if (pcc > newi) {
      /* newi is new left edge. right edge unchanged */
#if RBADEBUG
      assert(nzdata[leftorg].leftnz == EMPTY);
#endif
      nzdata[leftorg].leftnz = orgcol;
      nzdata[orgcol].rightnz = leftorg;
      nzdata[orgcol].leftnz = EMPTY;
      return orgcol;
    } else {
      /* hunt right for insertion point */
      searchcol = leftorg;
      while (searchcol != EMPTY &&
             (pcc = nzdata[searchcol].pseudo_curcol) < newi) {
        searchcol = nzdata[searchcol].rightnz;
      }

#if RBADEBUG
      assert(pcc!=newi);
#endif
      if (searchcol == EMPTY) {
        /* newi is new right edge. left edge unchanged */
#if RBADEBUG
        assert( nzdata[*rightorg].rightnz == EMPTY);
#endif
        nzdata[*rightorg].rightnz = orgcol;
        nzdata[orgcol].leftnz = *rightorg;
        nzdata[orgcol].rightnz = EMPTY;
        *rightorg = orgcol;
      } else {
        /* search col is now the element to the right of
         * where newi should be inserted.
         */
        /* point new element at neighbors */
        nzdata[orgcol].rightnz = searchcol;
        nzdata[orgcol].leftnz = nzdata[searchcol].leftnz;
        /* repoint neighbors */
        nzdata[searchcol].leftnz = orgcol;
        nzdata[nzdata[orgcol].leftnz].rightnz = orgcol;
      }
      return leftorg; /* unchanged left */
    }
#ifndef NDEBUG
    Asc_Panic(2, "rankiba_firstinsert_nz", "fix me");
    abort(); /* NOTREACHED */
#endif
  } else {
    /* first element in elimination section. */
    /* the nzlinks will be -1 still, though perhaps we
     * could avoid the init task by setting them here.
     */
    nzdata[orgcol].leftnz = EMPTY;
    nzdata[orgcol].rightnz = EMPTY;
    *rightorg = orgcol;
    return orgcol;
  }
}

#if RBADEBUG
/* checks that we haven't already added it */
static
int onright(int32 addtail, int32 orgcol, struct NZListEntry *nzdata)
{
  while (addtail!=EMPTY) {
    if (nzdata[addtail].orgcol == orgcol) return 1;
    addtail = nzdata[addtail].rightnz;
  }
  return 0;
}
/*
 * write, starting on left and moving right, an nzlist.
 * assumes EMPTY is used to terminate lists.
 */
static
void right_nzlist(FILE *fp, struct NZListEntry *nzd, int32 oc, real64 *d)
{
  assert(fp!=NULL && nzd != NULL);
  while (oc!= EMPTY) {
    FPRINTF(fp,"nz[%d]: lnz %4d, rnz %4d, pcc %4d, oc %4d  %12.8g\n",
      oc,nzd[oc].leftnz,nzd[oc].rightnz,nzd[oc].pseudo_curcol,nzd[oc].orgcol,
      d[oc]);
    oc = nzd[oc].rightnz;
  }
}
/*
 * write, starting on left and moving right, an nzlist.
 * assumes EMPTY is used to terminate lists.
 */
static
void right_pivlist(FILE *fp, struct PivListEntry *nzd, int32 oc)
{
  assert(fp!=NULL && nzd != NULL);
  while (oc!= EMPTY) {
    FPRINTF(fp,"piv[%d]: prev %d, next %d, orow %d, relloc %d\n",
      oc,nzd[oc].prev,nzd[oc].next,nzd[oc].orgrow,nzd[oc].relative_location);
    oc = nzd[oc].next;
    if (oc == nzd[oc].next) {
      FPRINTF(fp,"right_pivlist miscalled\n");
      break;
    }
  }
}
#endif

/*
 * assumes leftorg > 0.
 * Further assumes that we will never try to add anything on the
 * right edge of the list and will die if we do.
 * Returns orgcol of leftmost element.
 * newi and orgcol must correspond to an element
 * definitely NOT already in the elimination nzlist.
 */
static
int32 rankiba_insert_nz(int32 leftorg,
                        int32 newi,
                        int32 orgcol,
                        struct NZListEntry *nzdata)
{
  int32 pcc,searchcol;

#if RBADEBUG
  assert(!onright(leftorg,orgcol,nzdata));
#endif
  nzdata[orgcol].pseudo_curcol = newi;
  nzdata[orgcol].orgcol = orgcol;

  pcc = nzdata[leftorg].pseudo_curcol;
#if RBADEBUG
  assert(pcc!=newi);
#endif
  if (pcc > newi) {
    /* newi is new left edge. right edge unchanged */
#if RBADEBUG
    assert(nzdata[leftorg].leftnz == EMPTY);
#endif
    nzdata[leftorg].leftnz = orgcol;
    nzdata[orgcol].rightnz = leftorg;
    nzdata[orgcol].leftnz = EMPTY;
    return orgcol;
  } else {
    /* hunt right for insertion point. we must find it before
     * reaching the end of the row (per assumptions) or we
     * eventually will seg. fault by reading outside nzdata.
     */
    searchcol = leftorg;
    while ( pcc < newi) {
      searchcol = nzdata[searchcol].rightnz;
#if RBADEBUG
      assert(searchcol!=EMPTY);
#endif
      pcc = nzdata[searchcol].pseudo_curcol;
    }
#if RBADEBUG
    assert(pcc!=newi); /* shouldn't call this if nz already there */
    assert(searchcol != EMPTY);
#endif
    /* search col is now the element to the right of
     * where newi should be inserted.
     */
    /* point new element at neighbors */
    nzdata[orgcol].rightnz = searchcol;
    nzdata[orgcol].leftnz = nzdata[searchcol].leftnz;
    /* repoint neighbors */
    nzdata[nzdata[orgcol].leftnz].rightnz = orgcol;
    nzdata[searchcol].leftnz = orgcol;
    return leftorg; /* unchanged left */
  }
}

/* We insert (lifo) the rightside element at the left of the
 * rightside nzlist
 * assuming the element is not there. We do not assume any
 * element IS there.
 * User should set nzdata[orgcol].pseudo_curcol before calling
 * by using SETPCCRIGHT.
 */
static
int32 rankiba_addright_nz(int32 addtail,
                          int32 orgcol,
                          struct NZListEntry *nzdata
                          )
{
  nzdata[orgcol].orgcol = orgcol;
  if (addtail != EMPTY) {
    /* link existing tail to new element */
#if RBADEBUG
    assert(nzdata[addtail].leftnz == EMPTY);
    assert(!onright(addtail,orgcol,nzdata));
#endif
    nzdata[addtail].leftnz = orgcol;
  }
  nzdata[orgcol].leftnz = EMPTY; /* always adding to left */
  nzdata[orgcol].rightnz = addtail; /* works even for addtail = EMPTY */
  return orgcol;
}

#define RELLOC(pd,ocol) ((pd)[(ocol)].relative_location)
/* returns the relative location (in pivot list pd) given the orgcol.
 * Columns not yet IN pivot list will have RELLOC  = INT_MIN.
 * Columns rejected from pivot list will have RELLOC  = DIAG_MIN.
 * orgcol must be between 0 and sys->capacity.
 */

/*
 * eliminate_row2_ba does no permutation of any sort.
 * sp, pdata,nzdata should be of size sys->capacity.
 * pdata and nzdata are assumed to be orgcol indexed
 * as are tmp and pivots.
 * On normal exits (no floating point exceptions)
 * all nzdata entries will have orgcol == EMPTY,
 * as it was supposed to on entry.
 * On entry, must have row > rng->low.
 */
#if RBADEBUG
static int dbg =0;
static
int check_nzdata( struct NZListEntry *nzd,int len)
{
  int i,errs=0;
  for (i=0;i<len;i++) {
   if( nzd[i].orgcol != EMPTY) {
     errs++;
     FPRINTF(stderr,"foo at %d: l=%d r=%d p=%d o=%d\n",i,
        nzd[i].leftnz,nzd[i].rightnz,nzd[i].pseudo_curcol,nzd[i].orgcol);
     nzd[i].orgcol=EMPTY;
   }
  }
  return errs;
}
#endif

static
void eliminate_row2_ba(	mtx_matrix_t mtx,
			mtx_matrix_t upper_mtx,
			const mtx_range_t *rng,
			const int32 row,	/* row to eliminate */
			real64 *tmp,		/* temporary array */
			real64 *pivots,		/* prior pivots array */
                        const real64 dtol,	/* drop tolerance */
                        const int32 tail_relloc,/* not used if row = rng->low */
                        mtx_sparse_t *sp,	/* scratch space */
                        struct PivListEntry *pdata,
                        struct NZListEntry *nzdata
                       )
{

#define PCCLEFT(rloc) (low + (tail_relloc - (rloc)))
/* Returns the pseudo curcol index for a column given that
 * a) it has already been pivotted,
 * b) rloc is the RELLOC of its orgcol index.
 * low and tail_relloc are variables declared/passed in this
 * function.
 */
#define SETPCCRIGHT(nzd,m,ocol) \
  ((nzd)[(ocol)].pseudo_curcol = mtx_org_to_col((m),(ocol)))
/* Returns the pseudo curcol index for a column given that
 * a) it has NOT been pivotted,
 * b) ocol is the orgcol index.
 * m is the matrix and nzd is the nzdata.
 * As is obvious, in right side columns, pseudo and actual
 * cur cols are the same.
 */

  /* new */
  mtx_coord_t org;
  const real64 *data;	/* tmp to help out the optimizer */
  int32 i,k,len,newi;
  int32 elimwithrow;	/* orgcol of next nz to eliminate. from it get row */
  int32 useorgrow;	/* orgrow of row to add next during elimination */
  int32 left = EMPTY;	/* orgcol of leftmost (in pcc coord) nz to eliminate */
  int32 right = EMPTY;	/* orgcol of rightmost (in pcc coord) nz to eliminate */
  int32 addtail = EMPTY;	/* orgcol of head of rightside collist.
                         	* since order of the columns right of origin
                         	* matters not, it's a lifo list. */

  /* possibly helpful picture : (N is a nonzero)
   * row to be eliminated might look initially like:
   * |---------------N--------N---------||---N-------N------------|
   * low(pcc)     left(org)   right    ^     addtail              high(pcc)
   *              leftpcc     (org)    origin
   * as fill is created, left may move leftward.
   * right, once established doesn't change by row ops.
   * all N from low to origin will be eliminated.
   */
  /* old */
  int32 j,low,high;
  double tmpval;

  /* pseudo curcol limits */
  low = rng->low;	/* don't change this. pccleft needs it */
  high = rng->high;

#if RBADEBUG
   FPRINTF(stderr,"Elim Row: %d\n",row);
   if (check_nzdata(nzdata,mtx_order(mtx))) FPRINTF(stderr,"dirty data1\n");
/*
   if (mtx_row_to_org(mtx,row) == 844 ||
       mtx_row_to_org(mtx,row) == 913
      ) dbg=1;
*/
#endif
  /*
   * Move all non-zeros from current row to full array.
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
  /* get nonzeros in the row to be eliminated
   * and expand row into nzdata structure
   * Since we're working from sparsity patterns exclusively,
   * initialization of tmp doesn't matter.
   */
  mtx_steal_org_row_sparse(mtx,row,sp,mtx_ALL_COLS);

  len = sp->len;
  for (i=0; i < len; i++) {
    /* the mtx sparse steal does not copy 0s, so don't check. */
    /* maybe use the droptol here. */
    j = sp->idata[i];	/* j is orgcol */
    tmp[j] = sp->data[i];
    k = RELLOC(pdata,j);	/* k is pivot list relative location */
    if (k > DIAG_MIN) {
      newi = PCCLEFT(k);
      left = rankiba_firstinsert_nz(left, newi, j, nzdata, &right);
    } else {
      SETPCCRIGHT(nzdata,mtx,j);
      addtail = rankiba_addright_nz(addtail, j, nzdata);
    }
  }
  /* done with sp at this point. will use again later. */

  if (left == EMPTY) {
    /* punt. there's nothing to be done.
     * clean up data structures and get out. only addtail might be dirty.
     */
    org.row = mtx_row_to_org(mtx,row);
#if (!RBADEBUG)
/* standard L refill method */
    for (j = addtail; j != EMPTY; j = nzdata[j].rightnz) {
      /* don't fill on cancellations */
      if (fabs(tmp[j]) > dtol) {
        org.col = j;
        mtx_fill_org_value(mtx,&org,tmp[j]);
      }
      nzdata[j].orgcol = EMPTY;
    }
#else
/* gross comparable, debuggable L refill method */
    /* a nasty nasty loop to put stuff back in curcol order so we can do
     * direct comparisons by diffing files.
     */
    {
      int32 maxcurcol, tmpmax,maxold;

      if (addtail == EMPTY) {
        if (check_nzdata(nzdata,mtx_order(mtx))) {
          FPRINTF(stderr,"dirty data2\n");
        }
        return; /*nothing to do */
      }
      maxold = INT_MAX;
      while (1) {
        maxcurcol = -1;
        for (j = addtail; j != EMPTY; j = nzdata[j].rightnz) {
          tmpmax = mtx_org_to_col(mtx,j);
          if (tmpmax > maxcurcol && tmpmax < maxold) {
            maxcurcol = tmpmax;
          }
        }
        if (maxcurcol == -1) {
          /* everything printed. exit loop */
          break;
        }
        maxold = maxcurcol;
        j = mtx_col_to_org(mtx,maxcurcol);

        /* don't fill on cancellations */
        if (fabs(tmp[j]) > dtol) {
          org.col = j;
          mtx_fill_org_value(mtx,&org,tmp[j]);
          FPRINTF(gscr,"fillingL: or %d oc %d %24.18g\n",
            org.row,org.col,tmp[j]);
        }
        nzdata[j].orgcol = EMPTY;
      }
    }
#endif

#if (RBADEBUG)
    if (check_nzdata(nzdata,mtx_order(mtx))) {
      FPRINTF(stderr,"dirty data3\n");
    }
#endif
    return;
  }
  /* now we can safely assume there is at least one nonzero in the
   * elimination section of the row.
   */
  /*
   * Eliminates nzs from pivot, one by one, filling tmp with multipliers
   * We now operate on the entire row, since we are not writing the
   * multipliers back to the matrix.
   */
  for (elimwithrow = right ;
       elimwithrow != EMPTY ;
       elimwithrow = nzdata[elimwithrow].leftnz ) {
    if (tmp[elimwithrow] != D_ZERO) {
      /* Something to do for this row. things not cancelled */
      useorgrow = pdata[elimwithrow].orgrow;
      tmpval = tmp[elimwithrow] / pivots[elimwithrow];	/* Compute multiplier */

#if RBADEBUG
      if (dbg) {
        FPRINTF(stderr,"elimwithrow %d useorgrow %d tmpval %24.18g\n",
          elimwithrow,useorgrow,tmpval);
      }
#endif

      /*
       * tmpval is now the multiplier. We use it to eliminate the row,
       * but backpatch it to tmp, *after* we do the elimination, as
       * adding row over all columns will stomp on tmp[elimwithrow]
       * because we don't want to pay for the if test that would
       * avoid stomping tmp[elimwithrow].
       */
      /* was:
       *  mtx_cur_vec_add_row(mtx,tmp,j,-tmpval,mtx_ALL_COLS,FALSE);
       */
      mtx_org_row_sparse(mtx,
                         mtx_org_to_row(mtx,useorgrow),
                         sp,mtx_ALL_COLS,mtx_IGNORE_ZEROES);
      data = sp->data;
      len = sp->len;
      for (i=0; i < len; i++) {
        /* the mtx sparse fetch does not copy 0s, so don't check. */
        /* Maybe use the droptol here?
         * droptol here would amount to clearing epsilons from A
         * before factoring.
         */
        j = sp->idata[i];
        if (INSERTED(nzdata[j])) {
          /* add on top of existing stuff */
          tmp[j] -= tmpval * data[i];
#if RBADEBUG
        if (dbg) {
          FPRINTF(stderr,"accum: ");
        }
#endif
        } else {
          /* create fill */
          k = RELLOC(pdata,j);
          tmp[j] = -tmpval * data[i];
          if ( k > DIAG_MIN ) {
            /* stuff in elimination region. insert fill to be eliminated */
            newi = PCCLEFT(k);
            left = rankiba_insert_nz(left, newi, j, nzdata);
#if RBADEBUG
            if (dbg) {
              FPRINTF(stderr,"2elim: ");
            }
#endif
          } else {
            /* insert new potential pivot */
            SETPCCRIGHT(nzdata,mtx,j);
            addtail = rankiba_addright_nz(addtail, j, nzdata);
#if RBADEBUG
            if (dbg) {
              FPRINTF(stderr,"right: ");
            }
#endif
          }
        }
#if RBADEBUG
        if (dbg) {
          FPRINTF(stderr,"tmp[%d] = %12.7g\n",j,tmp[j]);
        }
#endif
      }
#if RBADEBUG
      if (dbg) {
        right_nzlist(stderr,nzdata,left,tmp);
      }
#endif
      tmp[elimwithrow] = tmpval;		/* patch the diagonal */
    }
  }
#if RBADEBUG
  if (dbg) {
    dbg=0;
  }
#endif

  /*
   * Fill up the upper triangular matrix.
   * refill the pcc range [low .. row-1].
   * OLD:
   * Remember that we have to zero all nnz's in tmp, effectively.
   */
  org.row = mtx_row_to_org(mtx,row);
  for (j = left; j != EMPTY; j = nzdata[j].rightnz) {
    if (tmp[j] != D_ZERO) {
      org.col = j;
      mtx_fill_org_value(upper_mtx,&org,tmp[j]);

#if (RBADEBUG)
      FPRINTF(gscr,"fillingU: or %d oc %d (cc %d) %24.18g\n",
              org.row,org.col,
              PCCLEFT(RELLOC(pdata,j)),tmp[j]);
#endif
    }
    nzdata[j].orgcol = EMPTY;
#if (RBADEBUG)
    FPRINTF(gscr,"nzdata[%4d] EMPTYd\n",j);
#endif
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
#if (!RBADEBUG)
/* Standard refill L */
  for (j=addtail; j != EMPTY; j = nzdata[j].rightnz) {
    /* don't fill on cancellations */
    if (fabs(tmp[j]) > dtol) {
      org.col = j;
      mtx_fill_org_value(mtx,&org,tmp[j]);
    }
    nzdata[j].orgcol = EMPTY;
  }
#else
/* Nonstandard refill L comparable to rankikw via diff on output */
  /* a nasty nasty loop to put stuff back in curcol order */
  {
    int32 maxcurcol, tmpmax,maxold;

    if (addtail == EMPTY) {
      if (check_nzdata(nzdata,mtx_order(mtx))) {
        FPRINTF(stderr,"dirty data4\n");
      }
      return; /*nothing to do */
    }
    maxold = INT_MAX;
    while (1) {
      maxcurcol = -1;
      for (j = addtail; j != EMPTY; j = nzdata[j].rightnz) {
        tmpmax = mtx_org_to_col(mtx,j);
        /*  bugs? */
        nzdata[j].orgcol = EMPTY;
        if (tmpmax > maxcurcol && tmpmax < maxold) {
          maxcurcol = tmpmax;
        }
      }
      if (maxcurcol == -1) {
        /* everything printed. exit loop */
        break;
      }
      maxold = maxcurcol;
      j = mtx_col_to_org(mtx,maxcurcol);

      /* don't fill on cancellations */
      if (fabs(tmp[j]) > dtol) {
        org.col = j;
        mtx_fill_org_value(mtx,&org,tmp[j]);
        FPRINTF(gscr,"fillingL: or %d oc %d %24.18g\n",
          org.row,org.col,tmp[j]);
      }
      nzdata[j].orgcol = EMPTY;
      FPRINTF(gscr,"nzdata[%4d] EMPTYd\n",j);
    }
  }
#endif

#if (RBADEBUG)
  if (check_nzdata(nzdata,mtx_order(mtx))) {
    FPRINTF(stderr,"dirty data5\n");
  }
#endif
}

/*
 * This function is the same as rankikw2_factor except
 * that it uses the 2 matrices, one to store L and one for U,
 * with minimum permutation.
 * As such it uses eliminate_row2_ba rather than eliminate_row2.
 * We assume there is NO incidence outside the region to be factored
 * in sys->factors.
 *
 * There is no rankiba_factor.
 */

static
void rankiba2_factor(linsolqr_system_t sys)
{

  /* old stuff */
  mtx_coord_t nz, org;
  mtx_matrix_t mtx;		/* Acopy converted to L */
  mtx_matrix_t upper_mtx;	/* U */
  int32 last_row; 		/* last row not yet factored (cur coords) */
  int defect;			/* rank defect encountered */
  mtx_range_t pivot_candidates;	/* columns (cur) to take pivot search over */
  real64 *tmp;			/* elimination work space */
  real64 pivot, pivotsize;	/* pivotsize = fabs(pivot) always */
  real64 *pivots;	/* pivot vector (cur) when all done, orgrow during */

  /* added */
  int32 pivlisthead = EMPTY, pivlisttail = EMPTY, rejectpivtail = EMPTY;
  /* org_col indices of current pivot data structure entry points. */
  int32 oldhead, oldtail;	/* org_col scratch indices */
  struct PivListEntry *pdata;
  struct NZListEntry *nzdata;
  mtx_sparse_t *scratchrow = NULL;
  int32 i,j,high;
  int32 tail_relloc;

  /**
   ** tmp and pivots are cur indexed, tmp by cols, pivots by rows.
   ** rather than allocating tmp every time, which can get expensive,
   ** we allocate it once *elsewhere* and zero it all here.
   ** Because we don't know the state of it on entry (due to exceptions,
   ** etc, the rezeroing is unavoidable.
   ** Eliminate_row2 is responsible for returning the working region
   ** zeroed.
   **
   ** Actually, during factorization, pivots is indexed by orgcol.
   ** Afterwards we fix it up to cur row.
   **
   **
   **/

  tmp = sys->ludata->tmp; /* uninitialized -- sparsity in charge */
  scratchrow = mtx_create_sparse(sys->capacity);
  if (scratchrow == NULL) {
    FPRINTF(stderr,"rankiba2_factor: (%s) Insufficient memory (1)\n",__FILE__);
    return;
  }
  pivots = sys->ludata->pivlist;
  mtx = sys->factors;
  upper_mtx = sys->inverse;
  defect = 0;

  sys->smallest_pivot = DBL_MAX;
  last_row = sys->rng.high; /* last_row decreases as singularity increases */
  high = last_row;		/* high stays constant from here */
  pivot_candidates.high = sys->rng.high;

  if (reset_elimination_data(sys->capacity,1)) {
    FPRINTF(stderr,"rankiba2_factor: (%s) Insufficient memory (2)\n",__FILE__);
    mtx_destroy_sparse(scratchrow);
    return;
  }

#if RBADEBUG
  gscr = fopen("/usr1/ballan/tmp/lu/rba","w+");
#endif

  pdata = g_ba_elimdata.pivdata;
  nzdata = g_ba_elimdata.nzdata;
  assert(pdata!=NULL);
  assert(nzdata!=NULL);


  for( nz.row = sys->rng.low ; nz.row <= last_row ; ) {

#if (RBADEBUG && 0)
    mtx_write_region_human_orgrows(gscr,mtx,mtx_ENTIRE_MATRIX);
#endif

    pivot_candidates.low = nz.col = nz.row;
    pivot = mtx_value(mtx,&nz);
    pivotsize = fabs(pivot);

    if (pivotsize > sys->pivot_zero &&
        pivotsize >= sys->ptol * mtx_row_max(mtx,&nz,&pivot_candidates,NULL) &&
       !col_is_a_spike(sys,nz.row) ) {
      /* Good pivot and not a spike: continue with next row */
      if( pivotsize < sys->smallest_pivot ) {
	sys->smallest_pivot = pivotsize;
      }
      /* get unpermuted location of pivot */
      org.col = mtx_col_to_org(mtx,nz.row);
      org.row = mtx_row_to_org(mtx,nz.row);
      /* insert pivot at list head */
      if (pivlisthead >= 0) {
        /* usual case - add to lower right */
        oldhead = pivlisthead;
#if RBADEBUG
        assert(pdata[oldhead].next == EMPTY);
#endif
        pdata[oldhead].next = org.col;
        pdata[org.col].prev = oldhead;
        pdata[org.col].next = EMPTY; /* only for debug, really? */
        pdata[org.col].orgrow = org.row;
        pdata[org.col].relative_location = pdata[oldhead].relative_location - 1;
        pivlisthead = org.col;
      } else {
        /* very first pivot case */
        pdata[org.col].orgrow = org.row;
        pdata[org.col].next = pdata[org.col].prev = EMPTY;
        pivlisthead = pivlisttail = org.col;
        pdata[org.col].relative_location = 0;
      }
      pivots[org.col] = pivot;
#if RBADEBUG
      FPRINTF(gscr,"Cheap pivot col %d (org %d)\n",nz.row,org.col);
#endif
      /* we will have to sort pivots back to cur order at end using tmp */
      ++(nz.row);
      continue;
    }
    /* else we need to possibly eliminate and search for pivot */

    /* pivots for rows nz.row back to sys->rng.low are stored in pivots,
     * but now in the corresponding orgcol positions determined by pdata.
     */
    /**
     ***  Row is a spike row or will
     ***  be when a necessary column
     ***  exchange occurs. Or maybe row is singular.
     **/
    if (nz.row == 176) {
      nz.row = 176;
    }
    if (pivlisttail != EMPTY) {
      tail_relloc = RELLOC(pdata,pivlisttail);
      eliminate_row2_ba(mtx,upper_mtx,&(sys->rng),nz.row,
                        tmp,pivots,sys->dtol,tail_relloc,
                        scratchrow,pdata,nzdata);
    }
    /* pivot will be leftmost of those that pass ptol and eps, or 0.0. */
    if (!defect) {
      /* no singular cols, and eliminate moved all left of diag crap out, so */
      pivotsize = mtx_get_pivot_col(mtx,&nz,mtx_ALL_COLS,
                                    &pivot,sys->ptol,sys->pivot_zero);
    } else {
      /* unfortunately, can't use mtx_ALL_COLS in search now because we must
       * ignore columns that were dragged out with previous singular rows.
       * The columns in pivot range have not been 'disordered' from
       * the sequence they would have in the old ranki implementations,
       * so this search will work and get nice close neighbor if
       * available.
       */
      pivotsize = mtx_get_pivot_col(mtx,&nz,&pivot_candidates,
                                    &pivot, sys->ptol,sys->pivot_zero);
    }
#if RBADEBUG
    FPRINTF(gscr,"Pivot column found %d (org %d)\n",
            nz.col, mtx_col_to_org(mtx,nz.col));
#endif

    if ( pivotsize < sys->pivot_zero ) { /* pivot is an epsilon */
      /*
       * Dependent row, drag to the end. The upper_mtx is a slave
       * of mtx, and will be dragged automatically.
       * If we expect to routinely handle bad matrices,
       * this needs to be reimplemented to work on linked list stuff,
       * in which case pivot selection gets hairy.
       */
      mtx_drag(mtx,nz.row,last_row);
      /* rejected rows and columns are still explicitly on the lower right */
      /* rejectpivtail
       * is the orgcol of the upper left most of rejected pivots (and
       * also the most recently rejected).
       */
      org.col = mtx_col_to_org(mtx,last_row);
      org.row = mtx_row_to_org(mtx,last_row);
      if (rejectpivtail == EMPTY) {
        pdata[org.col].next = pdata[org.col].prev = EMPTY;
        pdata[org.col].orgrow = org.row;
        pdata[org.col].relative_location = DIAG_MIN;
      } else {
        oldtail = rejectpivtail;
#if RBADEBUG
        assert(pdata[oldtail].prev == EMPTY);
#endif
        pdata[oldtail].prev = org.col;
        pdata[org.col].next = oldtail;
        pdata[org.col].prev = EMPTY;
        pdata[org.col].orgrow = org.row;
        pdata[org.col].relative_location = DIAG_MIN;
      }
      rejectpivtail = org.col;
      --last_row;
      defect = 1;
    } else {
      /* Independent row: nz contains selected pivot */
      /* Move pivot to diagonal */
      mtx_swap_cols(mtx,nz.row,nz.col);		/* this Fixes U as well */
      /* hah! killed it! killed the quadratic drag! */
      /* instead, insert at upper left of linked list */
      org.col = mtx_col_to_org(mtx,nz.row);
      org.row = mtx_row_to_org(mtx,nz.row);
      if (pivlisttail != EMPTY) {
        oldtail = pivlisttail;
#if RBADEBUG
        assert(pdata[oldtail].prev == EMPTY);
#endif
        pdata[oldtail].prev = org.col;
        pdata[org.col].next = oldtail;
        pdata[org.col].prev = EMPTY;
        pdata[org.col].relative_location = pdata[oldtail].relative_location + 1;
        pdata[org.col].orgrow = org.row;
        pivlisttail = org.col;
      } else {
        /* very first pivot case */
        pdata[org.col].orgrow = org.row;
        pdata[org.col].next = pdata[org.col].prev = EMPTY;
        pivlisthead = pivlisttail = org.col;
        pdata[org.col].relative_location = 0;
      }
      if( pivotsize < sys->smallest_pivot ) {
	sys->smallest_pivot = pivotsize;
      }
      ++(nz.row);
    }
    pivots[org.col] = pivot;
  }
  /* done with math */

  /* need to repermute matrices, pivots here */
  /* rejects should already have a proper matrix location,
   * but we need to run down the diagonal and fix things.
   */
  i = sys->rng.low;
  /* assuming square region */
  for (j = pivlisttail; j != EMPTY; j = pdata[j].next) {
    /* fix row order */
    if (pdata[j].orgrow != mtx_row_to_org(mtx,i)) {
      mtx_swap_rows(mtx,i,mtx_org_to_row(mtx,pdata[j].orgrow));
    }
    /* fix row order. by construction, j = pdata[j].orgcol */
    if (j != mtx_col_to_org(mtx,i)) {
      mtx_swap_cols(mtx,i,mtx_org_to_col(mtx,j));
    }
    i++;
  }
  assert((i-1)==last_row);
  /* mtx ok. fix pivots. tmp is out of use, so copy from org pivots
   * to cur locations in tmp, and then copy back the tmp range
   * in a separate loop. pivots is initially in orgcol order.
   */
  for (j = pivlisttail; j != EMPTY; j = pdata[j].next) {
    /* fixing pivots */
#if RBADEBUG
    assert( mtx_org_to_row(mtx,pdata[j].orgrow) >= sys->rng.low );
    assert( mtx_org_to_row(mtx,pdata[j].orgrow) <= last_row );
#endif
    tmp[mtx_org_to_row(mtx,pdata[j].orgrow)] = pivots[j];
  }
  for (j = rejectpivtail; j != EMPTY; j = pdata[j].next) {
    /* fixing rejected pivots */
#if RBADEBUG
    assert( mtx_org_to_row(mtx,pdata[j].orgrow) >last_row );
    assert( mtx_org_to_row(mtx,pdata[j].orgrow) <= sys->rng.high);
#endif
    tmp[mtx_org_to_row(mtx,pdata[j].orgrow)] = pivots[j];
  }
  for (i = sys->rng.low; i <= high;  i++) {
    pivots[i] = tmp[i];
  }

#if (RBADEBUG)
/* get rid of this */
  {
    FILE *fp;
    int32 cc;
    fp = fopen("/usr1/ballan/tmp/lu/ba2p","w+");
    FPRINTF(fp,"ba2 recorded pivot sequence:\n");
    for (cc= sys->rng.low; cc <= last_row; cc++) {
        FPRINTF(fp,"orgrow,orgcol = %d,%d,%24.18g\n",
          mtx_row_to_org(mtx,cc),
          mtx_col_to_org(mtx,cc), pivots[cc]);
    }
    fclose(fp);
  }
#endif

  zero_diagonal_elements(mtx,sys->rng.low,sys->rng.high);
  sys->rank = last_row - sys->rng.low + 1;
  mtx_destroy_sparse(scratchrow);
#if (RBADEBUG)
  {
    FILE *fp;
    fp = fopen("/usr1/ballan/tmp/lu/ba2m","w+");
    mtx_write_region_human_rows(fp,mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
    fp = fopen("/usr1/ballan/tmp/lu/ba2um","w+");
    mtx_write_region_human_rows(fp,upper_mtx,mtx_ENTIRE_MATRIX);
    fclose(fp);
  }
#endif
#if (RBADEBUG)
  fclose(gscr);
#endif
}

