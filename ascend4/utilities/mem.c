/*
 *  Memory module
 *  by Karl Westerberg, Ben Allan
 *  Created: 6/90
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: mem.c,v $
 *  Date last modified: $Date: 1998/01/10 18:00:06 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
#include "utilities/mem.h"

static void move_fwd(POINTER from, POINTER too, unsigned nbytes)
/**
 ***  Copies bytes from --> too in forward direction.
 **/
{
   while( nbytes-- > 0 )
      *(too++) = *(from++);
}

static void move_bwd(POINTER from, POINTER too,unsigned nbytes)
/**
 ***  Copies bytes from --> too in backward direction.
 **/
{
   from += nbytes;
   too  += nbytes;
   while( nbytes-- > 0 )
      *(--too) = *(--from);
}

void mem_move_disjoint(POINTER from, POINTER too, int nbytes)
{
   ascbcopy((char *)from,(char *)too,nbytes);
}

void mem_move(POINTER from, POINTER too,unsigned nbytes)
{
   if( from < too )
      move_bwd(from,too,nbytes);
   else
      move_fwd(from,too,nbytes);
}

/*  zeroes nbytes of memory pointed at by too. byte is ignored but
 *  there for interchangability with mem_repl_byte
 */
void mem_zero_byte(POINTER too, unsigned byte, unsigned nbytes)
{
  (void)byte;
  ascbzero((void *)too,(int)nbytes);
  /*
   *   while( nbytes-- > 0 )
   *         *(too++) = 0;
   */
}

void mem_repl_byte(POINTER too, unsigned byte, unsigned nbytes)
{
   while( nbytes-- > 0 )
      *(too++) = byte;
}

void mem_repl_word(POINTER too,unsigned word, unsigned nwords)
{
   unsigned *pw = (unsigned *)too;
   while( nwords-- > 0 )
      *(pw++) = word;
}

#define mv_get(too,from,nbytes) move_fwd((POINTER)(from),(POINTER)(too),nbytes)
#define mv_set(from,too,nbytes) move_fwd((POINTER)(from),(POINTER)(too),nbytes)

#if 0
int	mem_get_byte(long from)
{
   char	c;
   mv_get(&c,from,1);
   return( ((int)c) & mask_I_L(BYTESIZE) );
}
#endif /*  0  */


int	mem_get_int(long from)
{
   int	i;
   mv_get(&i,from,sizeof(int));
   return(i);
}

long	mem_get_long(long from)
{
   long	l;
   mv_get(&l,from,sizeof(long));
   return(l);
}

double	mem_get_float(long from)
{
   float	f;
   mv_get(&f,from,sizeof(float));
   return((double)f);
}

double	mem_get_double(long from)
{
   double	d;
   mv_get(&d,from,sizeof(double));
   return(d);
}

void mem_set_byte(long from, int b)
{
   char	c=b;
   mv_set(&c,from,1);
}

void mem_set_int(long from, int i)
{
   mv_set(&i,from,sizeof(int));
}

void mem_set_long(long from, long l)
{
   mv_set(&l,from,sizeof(long));
}

void mem_set_float(long from, double f)
{
   float	ff=f;
   mv_set(&ff,from,sizeof(float));
}

void mem_set_double(long from, double d)
{
   mv_set(&d,from,sizeof(double));
}

/*********************** mem_store code. BAA 5/16/95 ***********************/
/* according to K&R2 char <--> byte and size_t is a byte count.
   We are coding with those assumptions. (sizeof(char)==1) */
#define OK 345676543
#define DESTROYED 765434567
#if mem_DEBUG
/* ground LIGHTENING */
#undef mem_LIGHTENING
#define mem_LIGHTENING FALSE
#endif
#define BYPASS_ASCMALLOC FALSE
#if (mem_LIGHTENING || BYPASS_ASCMALLOC)
/* gonna bypass ascmalloc, eh? ;-) shaame on you! */
#define AMEM_calloc(a,b) calloc(a,b)
#define AMEM_malloc(a) malloc(a)
#define AMEM_free(a) free(a)
#define AMEM_realloc(a,b) realloc(a,b)
#else
#define AMEM_calloc(a,b) asccalloc(a,b)
#define AMEM_malloc(a) ascmalloc(a)
#define AMEM_free(a) ascfree(a)
#define AMEM_realloc(a,b) ascrealloc(a,b)
#endif

/*
   Don't get caught taking the size of struct mem_element,
   It is the head for anonymous elements of any size.
*/
struct mem_element {
  struct mem_element *nextelt;
};

struct mem_store_header {
  int integrity;     /* sanity number. */
  /* actual data */
  int maxlen;        /* current length of pool, not necessarily w/bars full */
  char **pool;       /* array of pointers to bars */
  struct mem_element *list; /* pointer to the most recently freed element */

  /* some interesting book keeping quantities */
#if !mem_LIGHTENING
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
#if !mem_LIGHTENING
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

#define AMEM_MINBARSIZE 5
#define AMEM_MINPOOLSIZE 2
#ifdef __alpha
#define AMEM_MINPOOLGROW 512
/* gotta love those fat pointers. grow by 4k min. */
#else
#define AMEM_MINPOOLGROW 1024
#endif

/*
Returns 2 if really bad, 1 if something fishy, 0 otherwise.
*/
static int check_mem_store(const mem_store_t ms)
{
#if mem_LIGHTENING
  return 0;
#else
  int i;

  if (ISNULL(ms)) {
    FPRINTF(stderr,"check_mem_store (mem.c): NULL mem_store_t!\n");
    return 2;
  }
  if (ms->integrity != OK) {
    (ms->integrity == DESTROYED) ?
      FPRINTF(stderr,
        "check_mem_store (mem.c): mem_store_t recently destroyed!\n")
    : FPRINTF(stderr,
        "check_mem_store (mem.c): mem_store_t corrupted!\n");
    return 2;
  }
  if (ms->onlist && ISNULL(ms->list)) {
    FPRINTF(stderr, "ERROR: check_mem_store (mem.c): NULL recycle list!\n");
    return 1;
  }
  /* more in than out? */
  if (ms->retned > ms->active) {
    FPRINTF(stderr, "ERROR: check_mem_store (mem.c): Imbalanced memory.\n");
    return 1;
  }
  if (ms->onlist + ms->inuse != ms->highwater) {
    FPRINTF(stderr, "ERROR: check_mem_store (mem.c): Imbalanced elements.\n");
    return 1;
  }
  /* is pool allocated to ms->len? */
  for (i=0; i < ms->len; i++) {
    if (ISNULL(ms->pool[i])) {
      FPRINTF(stderr, "ERROR: check_mem_store (mem.c): Hole found in pool!\n");
      FPRINTF(stderr, "                                Bar %d is NULL.\n",i);
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
   return 0. If change in ms->len is < the expected incremented value
   on return, the user knows he should do some garbage removal.
   incr is provided for times when we know how much we want
   to expand by.
*/
static int expand_store(mem_store_t ms, int incr)
{
  static int oldsize, newsize,punt,i;
  char **newpool = NULL;
  if (check_mem_store(ms) >1) {
    FPRINTF(stderr,"ERROR: (mem.c) expand_store received bad\n");
    FPRINTF(stderr,"               mem_store_t. Expansion failed.\n");
    return 1;
  }

#if !mem_LIGHTENING
  /* do not expand elements or pool if all is not in use */
  if (ms->inuse < ms->total) {
    FPRINTF(stderr,"ERROR: (mem.c) expand_store called prematurely.\n");
    FPRINTF(stderr,"               Expansion will be reported as failed.\n");
    return 1;
  }
#endif

  /* make sure bar expansion is at least the minimum */
  if (incr < ms->expand) incr = ms->expand;
  oldsize = ms->len;
  newsize = oldsize+incr;

  /* expand pool capacity only if all of pool in use  */
  if (newsize > ms->maxlen) {
    i = ms->maxlen + MAX(ms->growpool,incr);
    newpool = (char **)AMEM_realloc(ms->pool, i*sizeof(char *));
    if (ISNULL(newpool)) {
      FPRINTF(stderr,"ERROR: (mem.c) expand_store can't realloc pool.\n");
      return 1;
    }
    /* NULL the new pool */
    for (punt = ms->maxlen; punt < i; punt++) {
      newpool[punt] = NULL;
    }
    ms->maxlen = i;
    ms->pool = newpool;
  }
  /* end of pool expansion */

  /* expand elements/bars */
  ms->len = newsize; /* set expanded number of bars filled */
  punt = -1;
  for (i = oldsize; i < newsize; i++) {
#if mem_DEBUG
    ms->pool[i] = (char *)AMEM_calloc(ms->barsize,1);
#else
    ms->pool[i] = (char *)AMEM_malloc(ms->barsize);
#endif
    if (ISNULL(ms->pool[i])) {
      punt = i;
      /* we will return partially expanded if possible */
      break;
    }
  }
  if (punt >= 0) {
    /* incomplete expansion */
    if (punt == oldsize) {
      /* unable to add elements at all. fail */
      FPRINTF(stderr,"ERROR: (mem) expand_store:  Insufficient memory.\n");
      ms->len = oldsize;
      return 1;
    } else {
      /* contract pool to the actual expansion size */
      FPRINTF(stderr,"WARNING: (mem) expand_store: Insufficient memory.\n");
      FPRINTF(stderr,"                             Doing partial expansion.\n");
      ms->len = punt;
    }
  }
#if !mem_LIGHTENING
  ms->total = ms->len * ms->wid;
#endif
  return 0;
}

#if !mem_LIGHTENING
#if mem_DEBUG
/*
Returns 1 if pointer is to an elt of the store, 0 otherwise.
The case of pointer into store reserved space, but not an elt is checked.
*/
static int from_store( mem_store_t ms, void *elt)
{
  char *data;
  char **pool;
  int i;
  int lim;

  if (ISNULL(ms) || ISNULL(elt)) return 0;
  lim = ms->len;
  pool = ms->pool;
  data = (char *)elt;
  for (i=0; i<lim; i++) {
    /* did the char come from the current bar of chars? */
    if (*pool <= data && data < *pool + ms->barsize) {
      /* if so, is it legal? */
      if ( !((data - (*pool)) % ms->eltsize) ) {
        return 1;
      } else {
        FPRINTF(stderr,"ERROR: (mem.c) from_store:  Misaligned element\n");
        FPRINTF(stderr,"                            pointer detected.\n");
        return 0;
      }
    }
    pool++;
  }
  return 0;
}
#endif
#endif

void mem_get_stats(struct mem_statistics *mss,  mem_store_t m)
{
  if (ISNULL(mss)) {
    FPRINTF(stderr,"ERROR: (mem_get_stats)   Called with NULL struct\n");
    FPRINTF(stderr,"                         mem_statistics.\n");
    return;
  }
  if (check_mem_store(m)>1 ) {
    ascbzero((void *)mss,(int)sizeof(struct mem_statistics));
    FPRINTF(stderr,"ERROR: (mem_get_stats)   Bad mem_store_t given.\n");
    FPRINTF(stderr,"                         Returning 0s.\n");
    return;
  }
#if !mem_LIGHTENING
  mss->m_eff =  m->inuse*m->eltsize_req/(double)mem_sizeof_store(m);
  mss->m_recycle =
    ( (m->highwater > 0) ? m->active/(double)m->highwater : 0.0 );
  mss->elt_total = m->total;
  mss->elt_taken = m->highwater;
  mss->elt_inuse = m->inuse;
#else
  mss->m_eff = 0.0;
  mss->m_recycle = 0.0;
  mss->elt_total = m->len*m->wid;
  mss->elt_taken = m->curelt+m->curbar*m->wid;
  mss->elt_inuse = 0;
#endif
  mss->elt_onlist = m->onlist;
  mss->elt_size = m->eltsize;
  mss->str_len = m->len;
  mss->str_wid = m->wid;
}

mem_store_t mem_create_store(int length, int width,
                             size_t eltsize, int deltalen, int deltapool)
{
  int i, punt;
  mem_store_t newms=NULL;
  size_t uelt;

  if (length < 1 || width < 1 || deltalen < 1 ) {
    FPRINTF(stderr,"ERROR: (mem_create_store) : Bad input detected.\n");
    return NULL;
  }

  /* check minsizes */
  if (length < AMEM_MINPOOLSIZE) length = AMEM_MINPOOLSIZE;
  if (width < AMEM_MINBARSIZE) width = AMEM_MINBARSIZE;
  /* maybe the user gave us length = max he knows he needs, so we
     will not enforce a minimum maxlen on length at creation */

  uelt = eltsize;
  /* check for elt padding needed */
  if (eltsize % sizeof(void *)) {
    int ptrperelt;
    ptrperelt = eltsize/sizeof(void *) + 1;
#if mem_DEBUG
    FPRINTF(stderr,"(mem_create_store) Elts of size %d padded to %d\n",
      eltsize,(eltsize=ptrperelt*sizeof(void *)));
#else
    eltsize = ptrperelt*sizeof(void *);
#endif
  }
  /* eltsize is now pointer alignable */
  /* it could still be user data misalignable, of course, if pointer
     is not the most restrictive data type for the machine */


  newms = (mem_store_t)AMEM_calloc(1,sizeof(struct mem_store_header));
  if (ISNULL(newms)) {
    FPRINTF(stderr,"ERROR: (mem_create_store) : Insufficient memory.\n");
    return NULL;
  }
  /* the following are all initially 0/NULL by calloc, and should be:
  newms->list
  newms->active
  newms->retned
  newms->curbar
  newms->curelt
  newms->highwater
  newms->onlist
  newms->inuse
  newms->pool
  */
  newms->integrity = OK;
  newms->len = length;
  newms->maxlen = length;
  newms->wid = width;
  newms->expand = deltalen;
  newms->eltsize = eltsize;
  newms->barsize = eltsize * width;
#if !mem_LIGHTENING
  newms->total = length * width;
#endif
  newms->growpool = MAX(AMEM_MINPOOLGROW,deltapool);
  newms->eltsize_req = uelt;

  /* get pool */
  newms->pool = (char **)AMEM_calloc(length,sizeof(char *));
  if (ISNULL(newms->pool)) {
    FPRINTF(stderr,"ERROR: (mem_create_store) : Insufficient memory.\n");
    newms->integrity = DESTROYED;
    AMEM_free(newms);
    return NULL;
  }

  /* fill it */
  punt = -1;
  for (i=0; i < length; i++) {
    newms->pool[i] = (char *)AMEM_malloc(newms->barsize);
    if (ISNULL(newms->pool[i])) {
      punt = i; /* we will stop cleanup deallocation at punt-1 */
      break;
    }
  }

  /* drain it if can't fill it */
 if (punt != -1) {
    FPRINTF(stderr,"ERROR: (mem_create_store) : Insufficient memory.\n");
    for (i = 0; i < punt; i++) {
      AMEM_free(newms->pool[i]);
    }
    newms->integrity = DESTROYED;
    AMEM_free(newms->pool);
    AMEM_free(newms);
    return NULL;
  }
  return newms;
}

void *mem_get_element(mem_store_t ms)
{
  /* no automatic variables please */
  register struct mem_element *elt;
  /* in a test on the alpha, though, making elt static global slowed it */

  if (ISNULL(ms)) {
    FPRINTF(stderr,"ERROR: (mem_get_element)  Called with NULL store.\n");
    return NULL;
  }
  /* recycling */
  if (ms->onlist) {
    elt = ms->list; /* get last element put into list */
    ms->list = ms->list->nextelt; /* pop list */
    /* preserves original null if list is empty */
    ms->onlist--;
#if !mem_LIGHTENING
    ms->inuse++;
    ms->active++;
#endif
    return (void *)elt;
  }

  /* fresh element */
  if (ms->curelt == ms->wid) {
    /* bump up pool if bar all allocated */
    ms->curelt = 0;
    ms->curbar++;
  }
  if (ms->curbar == ms->len) {
    /* attempt to expand pool if all allocated */
    if ( expand_store(ms,1) ) {
      FPRINTF(stderr,"ERROR: (mem_get_element)  Insufficient memory.\n");
      return NULL;
    }
  }
  /* if we got here, pool is big enough to grab an element from */

  /* get the pointer to an element's worth of char from the pool */
  elt =
    (struct mem_element *) &(ms->pool[ms->curbar][ms->curelt * ms->eltsize]);
  ms->curelt++;
#if !mem_LIGHTENING
  ms->inuse++;
  ms->highwater++;
  ms->active++;
#endif
  return (void *)elt;
}

void mem_get_element_list(mem_store_t ms, int nelts, void **ary)
{
  FPRINTF(stderr,"ERROR: mem_get_element_list NOT implemented\n");
  if (ISNULL(ms) || ISNULL(ary)) {
    FPRINTF(stderr,"ERROR:   mem_get_element_list   Called with NULL\n");
    FPRINTF(stderr,"                                array or mem_store_t");
    return;
  }
  if (nelts <1) {
    FPRINTF(stderr,"WARNING:  mem_get_element_list   Called with request\n");
    FPRINTF(stderr,"                                 for 0 elements.");
    return;
  }
  ary[0]=NULL;
}

void mem_free_element(mem_store_t ms, void *ptr)
{
  register struct mem_element *elt;

  if (ISNULL(ptr)) return;
  elt = (struct mem_element *)ptr;

#if !mem_LIGHTENING
#if mem_DEBUG
  if (check_mem_store(ms)) {
    FPRINTF(stderr,"ERROR: (mem_free_element)  Fishy mem_store_t.\n");
    FPRINTF(stderr,"                           Element not recycled.\n");
    return;
    /* at this point we have no way to get back at the abandoned element */
  }
  /* check for belongs to this mem_store_t */
  if (!from_store(ms,ptr)) {
    FPRINTF(stderr,"ERROR: (mem_free_element)  Spurious element detected.\n");
    FPRINTF(stderr,"                           Element ignored.\n");
    return;
  }
#endif
#endif

  /* recycle him */
  elt->nextelt = ms->list; /* push onto list */
  /* first one in will pick up the null list starts as */
  ms->list = elt;
  /* ptr now on lifo stack in elt linked list form */
  ms->onlist++;

#if !mem_LIGHTENING
  ms->retned++;
  ms->inuse--;
  if (ms->inuse < 0) {
    FPRINTF(stderr,"ERROR: (mem_free_element) More elements freed than\n");
    FPRINTF(stderr,"                          have been handed out. (%d)\n",
      abs(ms->inuse));
  }
#endif
  return;
}

void mem_clear_store(mem_store_t ms) {
  if ( check_mem_store(ms) > 1 ) {
    FPRINTF(stderr,"ERROR: (mem_clear_store)  Bad mem_store_t given.\n");
    FPRINTF(stderr,"                          Not cleared.\n");
    return;
  }
#if mem_DEBUG
  if (ms->inuse || ms->highwater - ms->onlist ) {
    FPRINTF(stderr,"WARNING: (mem_clear_store)  In use elements in given\n");
    FPRINTF(stderr,"                            mem_store_t are cleared.\n");
    FPRINTF(stderr,"                            Don't refer to them again.\n");
  }
#endif
  ms->retned += ms->inuse;
  if (ms->active - ms->retned ||
      ms->onlist + ms->inuse - ms->highwater ||
      ms->curelt + ms->curbar*ms->wid - ms->highwater) {
    FPRINTF(stderr,"Warning: mem_clear_store: Element imbalance detected.\n");
  }
  ms->inuse = 0;
  ms->curbar = 0;
  ms->curelt = 0;
  ms->highwater = 0;
  ms->onlist = 0;
  ms->list = NULL;
}

void mem_destroy_store(mem_store_t ms)
{
  int i;
#if mem_DEBUG
  if ( (i=check_mem_store(ms))==2 ) {
    FPRINTF(stderr,"ERROR: (mem_destroy_store)  Bad mem_store_t given.\n");
    FPRINTF(stderr,"                            Not destroyed.\n");
    return;
  }
  if ( i ) {
    FPRINTF(stderr,
      "WARNING: (mem_destroy_store)  Suspicious mem_store_t given.\n");
    FPRINTF(stderr,"                            Destroyed anyway.\n");
    return;
  }
  if (ms->inuse || ms->highwater - ms->onlist ) {
    FPRINTF(stderr,"WARNING: (mem_destroy_store) In use elements in given\n");
    FPRINTF(stderr,"                             mem_store_t are cleared.\n");
    FPRINTF(stderr,"                             Don't refer to them again.\n");
  }
#else
  if (ISNULL(ms)  || ms->integrity != OK) {
    FPRINTF(stderr,"ERROR: (mem_destroy_store)  Bad mem_store_t given.\n");
    FPRINTF(stderr,"                            Not destroyed.\n");
    return;
  }
#endif
  for (i=0; i < ms->len; i++) {
    AMEM_free(ms->pool[i]);
  }
  AMEM_free(ms->pool);
  ms->integrity = DESTROYED;
  AMEM_free(ms);
  return;
}

void mem_print_store(FILE *fp, mem_store_t ms, unsigned detail)
{
  if (ISNULL(fp) || ISNULL(ms)) {
    FPRINTF(stderr,"ERROR: (mem_print_store) Called with NULL\n");
    FPRINTF(stderr,"                         FILE or mem_store_t\n");
    return;
  }
  if (check_mem_store(ms)>1) {
    FPRINTF(stderr,"ERROR: (mem_print_store) Called with bad mem_store_t\n");
    return;
  }
  FPRINTF(fp,"mem_store_t statistics:\n");
  if (detail) {
    FPRINTF(fp,"INTERNAL (integrity OK if = %d):\n",OK);
    FPRINTF(fp,"%-30s %-20ld\n","integrity", (long)ms->integrity);
#if !mem_LIGHTENING
    FPRINTF(fp,"%-30s %-20ld\n","active",ms->active);
    FPRINTF(fp,"%-30s %-20ld\n","returned",ms->retned);
#endif
    FPRINTF(fp,"%-30s %-20lu\n","eltsize",(unsigned long)ms->eltsize);
    FPRINTF(fp,"%-30s %-20lu\n","barsize",(unsigned long)ms->barsize);
    FPRINTF(fp,"%-30s %-20d\n","pool length",ms->len);
    FPRINTF(fp,"%-30s %-20d\n","pool maxlength",ms->maxlen);
    FPRINTF(fp,"%-30s %-20d\n","bar width",ms->wid);
    FPRINTF(fp,"%-30s %-20d\n","pool extension",ms->growpool);
    FPRINTF(fp,"%-30s %-20d\n","pool fill rate",ms->expand);
    FPRINTF(fp,"%-30s %-20d\n","current bar",ms->curbar);
    FPRINTF(fp,"%-30s %-20d\n","current elt",ms->curelt);
#if !mem_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","total elts",ms->total);
    FPRINTF(fp,"%-30s %-20d\n","highwater",ms->highwater);
#endif
    FPRINTF(fp,"%-30s %-20d\n","elt on list",ms->onlist);
#if !mem_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","elt in use",ms->inuse);
#endif
  }
  if (!detail || detail > 1) {
    FPRINTF(fp,"SUMMARY:\n");
    FPRINTF(fp,"%-30s %-20d\n","Pointers in pool",ms->maxlen);
    FPRINTF(fp,"%-30s %-20d\n","Pointers allocated",ms->len);
#if !mem_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","Elements in pool",ms->total);
    FPRINTF(fp,"%-30s %-20d\n","Elements in use",ms->inuse);
#endif
    FPRINTF(fp,"%-30s %-20d\n","Elements waiting recycle",ms->onlist);
#if !mem_LIGHTENING
    FPRINTF(fp,"%-30s %-20d\n","Elements unused",ms->total - ms->highwater);
#endif
    FPRINTF(fp,"%-30s %-20d\n","Working deltapool",ms->growpool);
    FPRINTF(fp,"%-30s %-20d\n","Working deltalen",ms->expand);
    FPRINTF(fp,"%-30s %-20g\n","Element efficiency",
      ms->eltsize_req/(double)ms->eltsize);
#if !mem_LIGHTENING
    FPRINTF(fp,"%-30s %-20g\n","Memory efficiency (w/recycle)",
      ms->highwater*ms->eltsize_req/(double)mem_sizeof_store(ms));
    FPRINTF(fp,"%-30s %-20g\n","Memory efficiency (instant)",
      ms->inuse*ms->eltsize_req/(double)mem_sizeof_store(ms));
    FPRINTF(fp,"%-30s %-20g\n","Recycle rate",
       ((ms->highwater > 0) ? ms->active/(double)ms->highwater : 0) );
#endif
  }
  FPRINTF(fp,"%-30s %-20lu\n","Total bytes in store",
    (unsigned long)mem_sizeof_store(ms));

  return;
}

size_t mem_sizeof_store(mem_store_t ms)
{
  register size_t siz;
  if (check_mem_store(ms)>1) return (size_t)0;
  siz = sizeof(struct mem_store_header);    /* header */
  siz += ms->barsize * ms->len;             /* elt data */
  return (siz += ms->maxlen*sizeof(char*)); /* pool vector */
}
