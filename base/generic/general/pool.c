/*
 *  Ascend Pooled Memory Manager
 *  by Benjamin Andrew Allan
 *  Created: 2/96
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: pool.c,v $
 *  Date last modified: $Date: 1997/07/18 11:37:01 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Benjamin Andrew Allan
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/pool.h"

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif
#ifdef NULL
/* ok, so the machine has a NULL defined. */
#ifndef ISNULL
/* and we've not got an ISNULL function */
#define ISNULL(a) ((a) == NULL)
#define NOTNULL(a) ((a) != NULL)
#endif /* isnull */
#endif /* null */
/* get local max and min definitions */
#ifdef PMN
#undef PMN
#undef PMX
#endif
#define PMN(a,b) ((a) < (b) ? (a) : (b))
#define PMX(a,b) ((a) < (b) ? (b) : (a))

/*********************** pool_store code. BAA 5/16/95 ***********************/
/* according to K&R2 char <--> byte and size_t is a byte count.
   We are coding with those assumptions. (sizeof(char)==1) */
#define OK 345676543
#define DESTROYED 765434567
#if pool_DEBUG
/* ground LIGHTENING */
#undef pool_LIGHTENING
#define pool_LIGHTENING FALSE
#endif
#define BYPASS_ASCMALLOC FALSE
#if (pool_LIGHTENING || BYPASS_ASCMALLOC)
/* gonna bypass ascmalloc, eh? ;-) shaame on you! */
#define PMEM_calloc(a,b) calloc(a,b)
#define PMEM_malloc(a) malloc(a)
#define PMEM_free(a) free(a)
#define PMEM_realloc(a,b) realloc(a,b)
#else
#define PMEM_calloc(a,b) asccalloc(a,b)
#define PMEM_malloc(a) ascmalloc(a)
#define PMEM_free(a) ascfree(a)
#define PMEM_realloc(a,b) ascrealloc(a,b)
#endif

/*
   Don't get caught taking the size of struct pool_element,
   It is the head for anonymous elements of any size.
*/
struct pool_element {
  struct pool_element *nextelt;
};

struct pool_store_header {
  int integrity;     /* sanity number. */
  /* actual data */
  int maxlen;        /* current length of pool, not necessarily w/bars full */
  char **pool;       /* array of pointers to bars */
  struct pool_element *list; /* pointer to the most recently freed element */

  /* some interesting book keeping quantities */
#if !pool_LIGHTENING
  long active;		/* number of elements individually requested, ever */
  long retned;		/* number of elements individually returned, ever */
#endif
  size_t eltsize;	/* size of element, often an unsigned (long) */
  size_t eltsize_req;	/* size of element, often an unsigned (long) */
  size_t barsize;	/* size of bar. no point in extra multiplications */

  /* memory management quantities */
  int len;           /* number of bars filled (length of pool filled) */
  int wid;           /* num elements in a bar */
  int expand;        /* number of bars to expand by usually */
  int growpool;      /* expansion increment for pool array */
  int curbar;        /* pool entry from which fresh elements may be had */
  int curelt;        /* number of the next element in the curbar to hand out */
  int onlist;        /* length of recycle list, in elements */
#if !pool_LIGHTENING
  int total;         /* total number of elements in this store */
  int highwater;     /* fresh elements turned loose from store */
  int inuse;         /* current elts user has outstanding */
#endif
};
/* notes on header:
 * list is, as currently coded, null terminated by accident.
 * The intent of the author is that the counter onlist is the
 * authoritative list length.
 */

#define PMEM_MINBARSIZE 5
#define PMEM_MINPOOLSIZE 2
#ifdef __alpha
#define PMEM_MINPOOLGROW 128
/* gotta love those fat pointers. grow by 4k min. */
#else
#define PMEM_MINPOOLGROW 256
#endif

/*
Returns 2 if really bad, 1 if something fishy, 0 otherwise.
*/
static int check_pool_store(const pool_store_t ps)
{
#if pool_LIGHTENING
  return 0;
#else
  int i;

  if (ISNULL(ps)) {
    FPRINTF(ASCERR,"check_pool_store (pool.c): NULL pool_store_t!\n");
    return 2;
  }
  if (ps->integrity != OK) {
    (ps->integrity == DESTROYED) ?
      FPRINTF(ASCERR,
        "check_pool_store (pool.c): pool_store_t recently destroyed!\n")
    : FPRINTF(ASCERR,
        "check_pool_store (pool.c): pool_store_t corrupted!\n");
    return 2;
  }
  if (ps->onlist && ISNULL(ps->list)) {
    FPRINTF(ASCERR, "ERROR: check_pool_store (pool.c): NULL recycle list!\n");
    return 1;
  }
  /* more in than out? */
  if (ps->retned > ps->active) {
    FPRINTF(ASCERR, "ERROR: check_pool_store (pool.c): Imbalanced memory.\n");
    return 1;
  }
  if (ps->onlist + ps->inuse != ps->highwater) {
    FPRINTF(ASCERR,"ERROR: check_pool_store (pool.c): Imbalanced elements.\n");
    return 1;
  }
  /* is pool allocated to ps->len? */
  for (i=0; i < ps->len; i++) {
    if (ISNULL(ps->pool[i])) {
      FPRINTF(ASCERR,
	 "ERROR: check_pool_store (pool.c): Hole found in pool!\n");
      FPRINTF(ASCERR, "                                Bar %d is NULL.\n",i);
      return 2;
    }
  }
  /* do not check for integrity or count of recycle list elements. */
  return 0;
#endif
}

/*
   This should not be called unless all current store is in use.
   Returns 0 if ok, 1 for all other insanities.
   If only partial expansion is possible, we will do it and
   return 0. If change in ps->len is < the expected incremented value
   on return, the user knows he should do some garbage removal.
   incr is provided for times when we know how much we want
   to expand by.

   This should never be called on a totally empty pool.
*/
static int expand_store(pool_store_t ps, int incr)
{
  static int oldsize, newsize,punt,i;
  char **newpool = NULL;
  if (check_pool_store(ps) >1) {
    FPRINTF(ASCERR,"ERROR: (pool.c) expand_store received bad\n");
    FPRINTF(ASCERR,"               pool_store_t. Expansion failed.\n");
    return 1;
  }

#if !pool_LIGHTENING
  /* do not expand elements or pool if all is not in use */
  if (ps->inuse < ps->total) {
    FPRINTF(ASCERR,"ERROR: (pool.c) expand_store called prematurely.\n");
    FPRINTF(ASCERR,"               Expansion will be reported as failed.\n");
    return 1;
  }
#endif

  /* make sure bar expansion is at least the minimum */
  if (incr < ps->expand) incr = ps->expand;
  oldsize = ps->len;
  newsize = oldsize+incr;

  /* expand pool capacity only if all of pool in use  */
  if (newsize > ps->maxlen) {
    i = ps->maxlen + PMX(ps->growpool,incr);
    newpool = (char **)PMEM_realloc(ps->pool, i*sizeof(char *));
    if (ISNULL(newpool)) {
      FPRINTF(ASCERR,"ERROR: (pool.c) expand_store can't realloc pool.\n");
      return 1;
    }
    /* NULL the new pool */
    for (punt = ps->maxlen; punt < i; punt++) {
      newpool[punt] = NULL;
    }
    ps->maxlen = i;
    ps->pool = newpool;
  }
  /* end of pool expansion */

  /* expand elements/bars */
  ps->len = newsize; /* set expanded number of bars filled */
  punt = -1;
  for (i = oldsize; i < newsize; i++) {
#if pool_DEBUG
    ps->pool[i] = (char *)PMEM_calloc(ps->barsize,1);
#else
    ps->pool[i] = (char *)PMEM_malloc(ps->barsize);
#endif
    if (ISNULL(ps->pool[i])) {
      punt = i;
      /* we will return partially expanded if possible */
      break;
    }
  }
  if (punt >= 0) {
    /* incomplete expansion */
    if (punt == oldsize) {
      /* unable to add elements at all. fail */
      FPRINTF(ASCERR,"ERROR: (pool) expand_store:  Insufficient memory.\n");
      ps->len = oldsize;
      return 1;
    } else {
      /* contract pool to the actual expansion size */
      FPRINTF(ASCERR,"WARNING: (pool) expand_store: Insufficient memory.\n");
      FPRINTF(ASCERR,"                            Doing partial expansion.\n");
      ps->len = punt;
    }
  }
#if !pool_LIGHTENING
  ps->total = ps->len * ps->wid;
#endif
  return 0;
}

#if !pool_LIGHTENING
#if pool_DEBUG
/*
Returns 1 if pointer is to an elt of the store, 0 otherwise.
The case of pointer into store reserved space, but not an elt is checked.
*/
static int from_store( pool_store_t ps, void *elt)
{
  char *data, **pool;
  int i,lim;

  if (ISNULL(ps) || ISNULL(elt)) return 0;
  lim = ps->len;
  pool = ps->pool;
  data = (char *)elt;
  for (i=0; i<lim; i++) {
    /* did the char come from the current bar of chars? */
    if (*pool <= data && data < *pool + ps->barsize) {
      /* if so, is it legal? */
      if ( !((data - (*pool)) % ps->eltsize) ) {
        return 1;
      } else {
        FPRINTF(ASCERR,"ERROR: (pool.c) from_store:  Misaligned element\n");
        FPRINTF(ASCERR,"                             pointer detected.\n");
        return 0;
      }
    }
    pool++;
  }
  return 0;
}
#endif
#endif

void pool_get_stats(struct pool_statistics *pss,  pool_store_t m)
{
  if (ISNULL(pss)) {
    FPRINTF(ASCERR,"ERROR: (pool_get_stats)   Called with NULL struct\n");
    FPRINTF(ASCERR,"                          pool_statistics.\n");
    return;
  }
  if (check_pool_store(m)>1 ) {
    ascbzero((void *)pss, sizeof(struct pool_statistics));
    FPRINTF(ASCERR,"ERROR: (pool_get_stats)   Bad pool_store_t given.\n");
    FPRINTF(ASCERR,"                          Returning 0s.\n");
    return;
  }
#if !pool_LIGHTENING
  pss->p_eff =  (double)(m->inuse * m->eltsize_req)/(double)pool_sizeof_store(m);
  pss->p_recycle =
    ( (m->highwater > 0) ? (double)m->active/(double)m->highwater : 0.0 );
  pss->elt_total = m->total;
  pss->elt_taken = m->highwater;
  pss->elt_inuse = m->inuse;
#else
  pss->p_eff = 0.0;
  pss->p_recycle = 0.0;
  pss->elt_total = m->len*m->wid;
  pss->elt_taken = m->curelt+m->curbar*m->wid;
  pss->elt_inuse = 0;
#endif
  pss->elt_onlist = m->onlist;
  pss->elt_size = (int)m->eltsize;
  pss->str_len = m->len;
  pss->str_wid = m->wid;
}

pool_store_t pool_create_store(int length, int width,
                               size_t eltsize, int deltalen, int deltapool)
{
  int i, punt;
  pool_store_t newps=NULL;
  size_t uelt;

  if (length < 1 || width < 1 || deltalen < 1 ) {
    FPRINTF(ASCERR,"ERROR: (pool_create_store) : Bad input detected.\n");
    return NULL;
  }

  /* check minsizes */
  if (length < PMEM_MINPOOLSIZE) length = PMEM_MINPOOLSIZE;
  if (width < PMEM_MINBARSIZE) width = PMEM_MINBARSIZE;
  /* maybe the user gave us length = max he knows he needs, so we
     will not enforce a minimum maxlen on length at creation */

  uelt = eltsize;
  /* check for elt padding needed */
  if (eltsize % sizeof(void *)) {
    size_t ptrperelt;
    ptrperelt = eltsize/sizeof(void *) + 1;
#if pool_DEBUG
    FPRINTF(ASCERR,"(pool_create_store) Elts of size %d padded to %d\n",
      (int)eltsize,(int)(eltsize=ptrperelt*sizeof(void *)));
#else
    eltsize = ptrperelt*sizeof(void *);
#endif
  }
  /* eltsize is now pointer alignable */
  /* it could still be user data misalignable, of course, if pointer
     is not the most restrictive data type for the machine */


  newps = (pool_store_t)PMEM_calloc(1,sizeof(struct pool_store_header));
  if (ISNULL(newps)) {
    FPRINTF(ASCERR,"ERROR: (pool_create_store) : Insufficient memory.\n");
    return NULL;
  }
  /* the following are all initially 0/NULL by calloc, and should be:
  newps->list
  newps->active
  newps->retned
  newps->curbar
  newps->curelt
  newps->highwater
  newps->onlist
  newps->inuse
  newps->pool
  */
  newps->integrity = OK;
  newps->len = length;
  newps->maxlen = length;
  newps->wid = width;
  newps->expand = deltalen;
  newps->eltsize = eltsize;
  newps->barsize = eltsize * width;
#if !pool_LIGHTENING                                         
  newps->total = length * width;
#endif
  newps->growpool = PMX(PMEM_MINPOOLGROW,deltapool);
  newps->eltsize_req = uelt;

  /* get pool */
  asc_assert(0 <= length);
  newps->pool = (char **)PMEM_calloc((size_t)length,sizeof(char *));
  if (ISNULL(newps->pool)) {
    FPRINTF(ASCERR,"ERROR: (pool_create_store) : Insufficient memory.\n");
    newps->integrity = DESTROYED;
    PMEM_free(newps);
    return NULL;
  }

  /* fill it */
  punt = -1;
  for (i=0; i < length; i++) {
    newps->pool[i] = (char *)PMEM_malloc(newps->barsize);
    if (ISNULL(newps->pool[i])) {
      punt = i; /* we will stop cleanup deallocation at punt-1 */
      break;
    }
  }

  /* drain it if can't fill it */
 if (punt != -1) {
    FPRINTF(ASCERR,"ERROR: (pool_create_store) : Insufficient memory.\n");
    for (i = 0; i < punt; i++) {
      PMEM_free(newps->pool[i]);
    }
    newps->integrity = DESTROYED;
    PMEM_free(newps->pool);
    PMEM_free(newps);
    return NULL;
  }
  return newps;
}

void *pool_get_element(pool_store_t ps)
{
  /* no automatic variables please */
  register struct pool_element *elt;
  /* in a test on the alpha, though, making elt static global slowed it */

  if (ISNULL(ps)) {
    FPRINTF(ASCERR,"ERROR: (pool_get_element)  Called with NULL store.\n");
    return NULL;
  }
  /* recycling */
  if (ps->onlist) {
    elt = ps->list; /* get last element put into list */
    ps->list = ps->list->nextelt; /* pop list */
    /* preserves original null if list is empty */
    ps->onlist--;
#if !pool_LIGHTENING
    ps->inuse++;
    ps->active++;
#endif
    return (void *)elt;
  }

  /* fresh element */
  if (ps->curelt == ps->wid) {
    /* bump up pool if bar all allocated */
    ps->curelt = 0;
    ps->curbar++;
  }
  if (ps->curbar == ps->len) {
    /* attempt to expand pool if all allocated */
    if ( expand_store(ps,1) ) {
      FPRINTF(ASCERR,"ERROR: (pool_get_element)  Insufficient memory.\n");
      return NULL;
    }
  }
  /* if we got here, pool is big enough to grab an element from */

  /* get the pointer to an element's worth of char from the pool */
  elt =
    (struct pool_element *) &(ps->pool[ps->curbar][ps->curelt * ps->eltsize]);
  ps->curelt++;
#if !pool_LIGHTENING
  ps->inuse++;
  ps->highwater++;
  ps->active++;
#endif
  return (void *)elt;
}

void pool_get_element_list(pool_store_t ps, int nelts, void **ary)
{
  FPRINTF(ASCERR,"ERROR: pool_get_element_list NOT implemented\n");
  if (ISNULL(ps) || ISNULL(ary)) {
    FPRINTF(ASCERR,"ERROR:   pool_get_element_list   Called with NULL\n");
    FPRINTF(ASCERR,"                                 array or pool_store_t");
    return;
  }
  if (nelts <1) {
    FPRINTF(ASCERR,"WARNING:  pool_get_element_list   Called with request\n");
    FPRINTF(ASCERR,"                                  for 0 elements.");
    return;
  }
  ary[0]=NULL;
}

void pool_free_elementF(pool_store_t ps, void *ptr
#if pool_DEBUG
, CONST char *fn
#endif
)
{
  register struct pool_element *elt;

  if (ISNULL(ptr)) return;
  elt = (struct pool_element *)ptr;

#if !pool_LIGHTENING
  if (ISNULL(ps)) {
    FPRINTF(ASCERR,"ERROR: (pool_free_elementF)  Called with NULL store.\n");
    return;
  }
#if pool_DEBUG
  if (check_pool_store(ps)) {
    FPRINTF(ASCERR,"ERROR: (pool_free_element)  Fishy pool_store_t.\n");
    FPRINTF(ASCERR,"                            Element not recycled.\n");
    FPRINTF(ASCERR,"%s\n",fn);
    return;
    /* at this point we have no way to get back at the abandoned element */
  }
  /* check for belongs to this pool_store_t */
  if (!from_store(ps,ptr)) {
    FPRINTF(ASCERR,"ERROR: (pool_free_element)  Spurious element detected.\n");
    FPRINTF(ASCERR,"                            Element ignored.\n");
    FPRINTF(ASCERR,"%s\n",fn);
    return;
  }
#endif
#endif

  /* recycle him */
  elt->nextelt = ps->list; /* push onto list */
  /* first one in will pick up the null list starts as */
  ps->list = elt;
  /* ptr now on lifo stack in elt linked list form */
  ps->onlist++;

#if !pool_LIGHTENING
  ps->retned++;
  ps->inuse--;
  if (ps->inuse < 0) {
    FPRINTF(ASCERR,"ERROR: (pool_free_element) More elements freed than\n");
    FPRINTF(ASCERR,"                           have been handed out. (%d)\n",
      abs(ps->inuse));
#if pool_DEBUG
    FPRINTF(ASCERR,"%s\n",fn);
#endif
  }
#endif
  return;
}

void pool_clear_storeF(pool_store_t ps
#if pool_DEBUG
, CONST char *fn
#endif
) {
  if ( check_pool_store(ps) > 1 ) {
    FPRINTF(ASCERR,"ERROR: (pool_clear_store)  Bad pool_store_t given.\n");
    FPRINTF(ASCERR,"                           Not cleared.\n");
#if pool_DEBUG
    FPRINTF(ASCERR,"%s\n",fn);
#endif
    return;
  }
#if pool_DEBUG
  if (ps->inuse || ps->highwater - ps->onlist ) {
    FPRINTF(ASCERR,"WARNING: (pool_clear_store)  In use elements in given\n");
    FPRINTF(ASCERR,"                            pool_store_t are cleared.\n");
    FPRINTF(ASCERR,"                            Don't refer to them again.\n");
    FPRINTF(ASCERR,"%s\n",fn);
  }
#endif
#if !pool_LIGHTENING
  ps->retned += ps->inuse;
  if (ps->active - ps->retned ||
      ps->onlist + ps->inuse - ps->highwater ||
      ps->curelt + ps->curbar*ps->wid - ps->highwater) {
    FPRINTF(ASCERR,"Warning: pool_clear_store: Element imbalance detected.\n");
#if pool_DEBUG
    FPRINTF(ASCERR,"%s\n",fn);
#endif
  }
#endif
#if !pool_LIGHTENING
  ps->inuse = 0;
  ps->highwater = 0;
#endif
  ps->curbar = 0;
  ps->curelt = 0;
  ps->onlist = 0;
  ps->list = NULL;
}

void pool_destroy_store(pool_store_t ps)
{
  int i;
#if pool_DEBUG
  if ( (i=check_pool_store(ps))==2 ) {
    FPRINTF(ASCERR,"ERROR: (pool_destroy_store)  Bad pool_store_t given.\n");
    FPRINTF(ASCERR,"                             Not destroyed.\n");
    return;
  }
  if ( i ) {
    FPRINTF(ASCERR,
      "WARNING: (pool_destroy_store)  Suspicious pool_store_t given.\n");
    FPRINTF(ASCERR,"                             Destroyed anyway.\n");
    return;
  }
  if (ps->inuse || ps->highwater - ps->onlist ) {
    FPRINTF(ASCERR,"WARNING: (pool_destroy_store) In use elements in given\n");
    FPRINTF(ASCERR,"                             pool_store_t are cleared.\n");
    FPRINTF(ASCERR,
       "                             Don't refer to them again.\n");
  }
#else
  if (ISNULL(ps)  || ps->integrity != OK) {
    FPRINTF(ASCERR,"ERROR: (pool_destroy_store)  Bad pool_store_t given.\n");
    FPRINTF(ASCERR,"                             Not destroyed.\n");
    return;
  }
#endif
  for (i=0; i < ps->len; i++) {
    PMEM_free(ps->pool[i]);
  }
  PMEM_free(ps->pool);
  ps->integrity = DESTROYED;
  PMEM_free(ps);
  return;
}

void pool_print_store(FILE *fp, pool_store_t ps, unsigned detail)
{
  if (ISNULL(fp) || ISNULL(ps)) {
    FPRINTF(ASCERR,"ERROR: (pool_print_store) Called with NULL\n");
    FPRINTF(ASCERR,"                          FILE or pool_store_t\n");
    return;
  }
  if (check_pool_store(ps)>1) {
    FPRINTF(ASCERR,"ERROR: (pool_print_store) Called with bad pool_store_t\n");
    return;
  }                                
  FPRINTF(fp,"pool_store_t statistics:\n");
  if (detail) {
    FPRINTF(fp,"INTERNAL (integrity OK if = %d):\n",OK);
    FPRINTF(fp,"%-30s %-20d\n","integrity",ps->integrity);
#if !pool_LIGHTENING
    FPRINTF(fp,"%-30s %-20ld\n","active",ps->active);
    FPRINTF(fp,"%-30s %-20ld\n","returned",ps->retned);
#endif
    FPRINTF(fp,"%-30s %-20lu\n","eltsize",(unsigned long)ps->eltsize);
    FPRINTF(fp,"%-30s %-20lu\n","barsize",(unsigned long)ps->barsize);
    FPRINTF(fp,"%-30s %-20d\n","pool length",ps->len);
    FPRINTF(fp,"%-30s %-20d\n","pool maxlength",ps->maxlen);
    FPRINTF(fp,"%-30s %-20d\n","bar width",ps->wid);
    FPRINTF(fp,"%-30s %-20d\n","pool extension",ps->growpool);
    FPRINTF(fp,"%-30s %-20d\n","pool fill rate",ps->expand);
    FPRINTF(fp,"%-30s %-20d\n","current bar",ps->curbar);
    FPRINTF(fp,"%-30s %-20d\n","current elt",ps->curelt);
#if !pool_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","total elts",ps->total);
    FPRINTF(fp,"%-30s %-20d\n","highwater",ps->highwater);
#endif
    FPRINTF(fp,"%-30s %-20d\n","elt on list",ps->onlist);
#if !pool_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","elt in use",ps->inuse);
#endif
  }
  if (!detail || detail > 1) {
    FPRINTF(fp,"SUMMARY:\n");
    FPRINTF(fp,"%-30s %-20d\n","Pointers in pool",ps->maxlen);
    FPRINTF(fp,"%-30s %-20d\n","Pointers allocated",ps->len);
#if !pool_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","Elements in pool",ps->total);
    FPRINTF(fp,"%-30s %-20d\n","Elements in use",ps->inuse);
#endif
    FPRINTF(fp,"%-30s %-20d\n","Elements waiting recycle",ps->onlist);
#if !pool_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","Elements unused",ps->total - ps->highwater);
#endif
    FPRINTF(fp,"%-30s %-20d\n","Working deltapool",ps->growpool);
    FPRINTF(fp,"%-30s %-20d\n","Working deltalen",ps->expand);
    FPRINTF(fp,"%-30s %-20g\n","Element efficiency",
      ps->eltsize_req/(double)ps->eltsize);
#if !pool_LIGHTENING
    FPRINTF(fp,"%-30s %-20g\n","Memory efficiency (w/recycle)",
      ps->highwater*ps->eltsize_req/(double)pool_sizeof_store(ps));
    FPRINTF(fp,"%-30s %-20g\n","Memory efficiency (instant)",
      ps->inuse*ps->eltsize_req/(double)pool_sizeof_store(ps));
    FPRINTF(fp,"%-30s %-20g\n","Recycle rate",
       ((ps->highwater > 0) ? ps->active/(double)ps->highwater : 0) );
#endif
  }
  FPRINTF(fp,"%-30s %-20lu\n","Total bytes in store",
    (unsigned long)pool_sizeof_store(ps));

  return;
}

size_t pool_sizeof_store(pool_store_t ps)
{
  register size_t siz;
  if (check_pool_store(ps)>1) return (size_t)0;
  siz = sizeof(struct pool_store_header);    /* header */
  siz += ps->barsize * ps->len;             /* elt data */
  return (siz += ps->maxlen*sizeof(char*)); /* pool vector */
}
