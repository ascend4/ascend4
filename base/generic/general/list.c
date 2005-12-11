/*
 *  Implementation
 *  of List Module
 *  Tom Epperly
 *  Part of Ascend
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: list.c,v $
 *  Date last modified: $Date: 1998/01/06 12:06:55 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of the
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
 *
 */

#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#if LISTUSESPOOL
#include "general/pool.h"
#endif

#ifndef lint
static CONST char ListID[] = "$Id: list.c,v 1.3 1998/01/06 12:06:55 ballan Exp $";
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#define address(a,b) \
(((unsigned long)(a) + ((b)*sizeof(VOIDPTR))-sizeof(VOIDPTR)))
#define GL_LENGTH(l) ((l)->length)
#define LOWCAPACITY 2
/* Shortest list we will allow. It must not be 0! */
static const unsigned long MIN_INCREMENT = 8;
/* Minimum number of elements to increase when expanding list. */

/*
 * Be a bit more informative when dying on a list range error.
 * No printf we know of returns -2, so the print will always be
 * just before assertion acts to handle the range error.
 */
#define ASSERTRANGE(list,pos,n) \
  asc_assert(((pos >= 1) && (pos <= GL_LENGTH(list))) || \
             (PRINTF("%s called with illegal length %lu\n",n,pos) == -2));

#ifndef MOD_REALLOC
#define REALLOCDEBUG FALSE
#else
#define REALLOCDEBUG TRUE
#endif
/* if REALLOCDEBUG TRUE, then we use our own pricey version of realloc
 * instead of the standard library version.
 * This is to test for a bug in purify/os interactions.
 */
/*
 * We have modified gl_destroy to recycle lists of up to a certain size
 * MAXRECYCLESIZE.
 * The data structure for holding the recycled lists is a vector
 * of pointers to gl_list_t. Item 1 in a recycled list is the pointer
 * to the next recycled list, which may be NULL. This is a change
 * from Tom's initial implementation which used a dense array to store
 * pointers to lists.
 *
 * The number of recycled lists of a particular
 * capacity i to be allowed is found in the AllowedContents[i].
 *
 * It is intended that RecycledContents[i] is the current count of
 * lists of capacity i. It is a coincidence of implementation that
 * when RecycledContents[i] <=0 RecycledList[i]==NULL.
 *
 * If LISTRECYCLERDEBUG,
 *    HighWaterMark[i] is the most of capacity i ever recycled.
 *    ListsDestroyed[i] is likewise lists not recycled because full.
 */
#define LISTRECYCLERDEBUG 0
#define MAXRECYCLESIZE 500	/* the maximum list size to be recycled */
#define MAXRECYCLESMALLITEMS 300/* the maximum number of size < LARGEITEM */
#define MAXRECYCLELARGEITEMS 25	/* the maximum number of each large size */
#define MAXRECYCLESEARCH 2	/* how much larger a list will be searched for */
/* search performance. 0 and 2 are best values, 2 slightly better. alphaosf2 */
#define LARGEITEM 21		 /* smallest large item */
/* note: baa, 3/8/96 LARGEITEM and MAXRECYCLELARGEITEMS not in use yet */
static struct gl_list_t *RecycledList[MAXRECYCLESIZE+1];
static int RecycledContents[MAXRECYCLESIZE+1];
static int AllowedContents[MAXRECYCLESIZE+1];
/*
 * It is assumed that these arrays are initialized to zero according to the
 * rules given in "The C Programming Language", Brian W. Kernighan and Dennis
 * M. Richie, second edition.
 */
#if LISTRECYCLERDEBUG
static int HighWaterMark[MAXRECYCLESIZE+1];
/* most ever in recycle list of size i */
static int ListsDestroyed[MAXRECYCLESIZE+1];
/* number of lists destroyed because Allowed was exceeded of size i */

static int ListsDestroyedTotal = 0;
/* number of lists destroyed because Allowed was exceeded all sizes */
static int NewListsUsed = 0;
/* calls to gl_create that required malloc */
static int RecycledListsUsed = 0;
/* calls to gl_create that used recycle */
#endif

/* If this function is not called, no recycling will take place
 * because all the Alloweds will be at default of 0.
 * This initializes the AllowedContents array, and nothing else.
 * General limits are set based on MAXRECYCLE*ITEMS.
 * Specific ones are done after and may be tuned.
 */
void gl_init(void)
{
  int i;
  if (AllowedContents[0] &&
      AllowedContents[0] != MAXRECYCLESMALLITEMS) {
    PRINTF("gl_init recalled after data corrupted!\n");
    return;
    /* somebody called us twice, with intervening nonsense. punt */
  }
  for (i = LOWCAPACITY; i < LARGEITEM; i++) {
    AllowedContents[i] = MAXRECYCLESMALLITEMS;
  }
  for (i = LARGEITEM; i <= MAXRECYCLESIZE; i++) {
    AllowedContents[i] = MAXRECYCLELARGEITEMS;
  }
#define LARGESYSTEM 1
#if LARGESYSTEM
  AllowedContents[2] = 40000;
  AllowedContents[3] = 20000;
  AllowedContents[4] = 8000;
  AllowedContents[5] = 2000;
  AllowedContents[6] = 2000;
  AllowedContents[7] = 5000;
  AllowedContents[8] = 1000;
#else
  AllowedContents[2] = 10000;
  AllowedContents[3] = 5000;
  AllowedContents[4] = 5000;
  AllowedContents[7] = 5000;
#endif
  AllowedContents[23] = 50;
}

#if LISTUSESPOOL
static pool_store_t g_list_head_pool = NULL;
/* global for our memory manager */
#define POOL_ALLOCHEAD (struct gl_list_t *)(pool_get_element(g_list_head_pool))
/* get a token. Token is the size of the struct struct gl_list_t */
#define POOL_FREEHEAD(p) (pool_free_element(g_list_head_pool,((void *)p)))
/* return a struct gl_list_t */

#define GLP_LEN 10
#ifdef __alpha
#define GLP_WID 72
#else
#define GLP_WID 127
#endif
/* retune rpwid if the size of pending_t changes dramatically */
#define GLP_ELT_SIZE (sizeof(struct gl_list_t))
#define GLP_MORE_ELTS 10
/*
 *  Number of slots filled if more elements needed.
 *  So if the pool grows, it grows by GLP_MORE_ELTS*GLP_WID elements at a time.
 */
#define GLP_MORE_BARS 500
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */
#else

/* we default back to malloc/free if LISTUSESPOOL == FALSE */
#define POOL_ALLOCHEAD ((struct gl_list_t*)ascmalloc(sizeof(struct gl_list_t)))
#define POOL_FREEHEAD(p) ascfree((char *)(p))

#endif

/* This function is called at compiler startup time and destroy at shutdown. */
void gl_init_pool(void) {
#if LISTUSESPOOL
  if (g_list_head_pool != NULL) {
    Asc_Panic(2, NULL, "ERROR: gl_init_pool called twice.\n");
  }
  g_list_head_pool = pool_create_store(GLP_LEN, GLP_WID, GLP_ELT_SIZE,
    GLP_MORE_ELTS, GLP_MORE_BARS);
  if (g_list_head_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: gl_init_pool unable to allocate pool.\n");
  }                     
#else
  PRINTF("list.[ch] built without pooling of overheads\n");
#endif
}

void gl_destroy_pool(void) {
#if LISTUSESPOOL
  if (g_list_head_pool==NULL) return;
  gl_emptyrecycler();    /* deallocate data in recycled lists, zero RecycledContents[] */
  pool_clear_store(g_list_head_pool);
  pool_destroy_store(g_list_head_pool);
  g_list_head_pool = NULL;
#else
  PRINTF("list.[ch] built without pooling of overheads\n");
#endif
}                                                              


int gl_pool_initialized(void)
{
#if LISTUSESPOOL
  return (g_list_head_pool == NULL) ? FALSE : TRUE;
#else
  PRINTF("list.[ch] built without pooling of overheads\n");
  return TRUE;
#endif
}


void gl_report_pool(FILE *f)
{
#if LISTUSESPOOL
  if (g_list_head_pool==NULL)
    FPRINTF(f,"ListHeadPool is empty\n");
  FPRINTF(f,"ListHeadPool ");
  pool_print_store(f,g_list_head_pool,0);
#else
  FPRINTF(f,"list.[ch] built without pooling of overheads\n");
#endif
}

/*
 * Guess of the capacity of the list.  If the
 * list capacity needs to be expanded it will
 * be.  It is good to be close though
 */
struct gl_list_t *gl_create(unsigned long int capacity)
{
  struct gl_list_t *new;
  unsigned long i;
  i = capacity = MAX((unsigned long)LOWCAPACITY,capacity);
  while(i <= capacity+MAXRECYCLESEARCH &&
        i <= MAXRECYCLESIZE){
    asc_assert(RecycledContents[i] >= 0 &&
               RecycledContents[i] <= AllowedContents[i]);
    if (RecycledContents[i] > 0){
#if LISTRECYCLERDEBUG
      RecycledListsUsed++;
#endif
      new = RecycledList[i]; 		/* fetch last recycled list */
      asc_assert(new != NULL);
      RecycledList[i] = (struct gl_list_t *)new->data[0]; /* point to next */
      --RecycledContents[i];		/* reduce list count for this size */
      new->length = 0;
      new->flags = (unsigned int)(gsf_SORTED | gsf_EXPANDABLE);
#ifndef NDEBUG
      if (i>0) new->data[0]=NULL;
#endif
      return new;
    }
    i++;
  }
  /*FPRINTF(ASCERR,"ABOUT TO ALLOCHEAD\n");*/
  new=POOL_ALLOCHEAD;
  /*FPRINTF(ASCERR,"GL_CREATE: COULDN'T ALLOC SIZE %d, new = %p\n",sizeof(struct gl_list_t),POOL_ALLOCHEAD);*/
  if (new != NULL) {
    new->length = 0;
#if LISTRECYCLERDEBUG
    NewListsUsed++;
#endif
#ifndef NDEBUG
    new->data = (VOIDPTR *)asccalloc((unsigned)capacity,sizeof(VOIDPTR));
#else
    new->data = (VOIDPTR *)ascmalloc((unsigned)capacity*sizeof(VOIDPTR));
#endif
    new->capacity = capacity;
    new->flags = (unsigned int)(gsf_SORTED | gsf_EXPANDABLE);
    return new;
  }
  else {
    FPRINTF(ASCERR,"UNABLE TO ALLOCATE MEMORY FOR LIST\n");
    return NULL;
  }
}

void gl_free_and_destroy(struct gl_list_t *list)
{
  unsigned long c;
  if (list == NULL) return;
#if LISTUSESPOOL
  AssertMemory(list);
#else
  AssertAllocatedMemory(list,sizeof(struct gl_list_t));
#endif
  for(c=0;c<GL_LENGTH(list);c++) {
    AssertContainedMemory(list->data[c],0);
    ascfree(list->data[c]);
  }
#ifndef NDEBUG
  ascbzero(list->data,sizeof(VOIDPTR)*list->capacity);
#endif
  c = list->capacity; 		/* gonna use it a lot */
  if (c <= MAXRECYCLESIZE && RecycledContents[c] < AllowedContents[c]) {
    asc_assert(RecycledContents[c] >= 0);
    /* push list into LIFO list of lists, and bump the count. */
    list->data[0] = RecycledList[c];
    RecycledList[c] = list;
    RecycledContents[c]++;
#if LISTRECYCLERDEBUG
    if (RecycledContents[c] > HighWaterMark[c]) {
      HighWaterMark[c] = RecycledContents[c];
    }
#endif
  } else{
#if LISTRECYCLERDEBUG
    if (c<=MAXRECYCLESIZE) {
      ListsDestroyed[c]++;
    }
#endif
    ascfree(list->data);
    list->data = NULL;
    list->capacity = list->length = 0;
    POOL_FREEHEAD(list);
  }
}

void gl_destroy(struct gl_list_t *list)
{
  unsigned long c;
  if (list == NULL) return;
#if LISTUSESPOOL
  AssertMemory(list);
#else
  AssertAllocatedMemory(list,sizeof(struct gl_list_t));
#endif
  c = list->capacity; 		/* gonna use it a lot */
  if (c <= MAXRECYCLESIZE && RecycledContents[c] < AllowedContents[c]) {
    asc_assert(RecycledContents[c] >= 0);
    /* push list into LIFO list of lists, and bump the count. */
    list->data[0] = RecycledList[c];
    RecycledList[c] = list;
    RecycledContents[c]++;
#if LISTRECYCLERDEBUG
    if (RecycledContents[c] > HighWaterMark[c]) {
      HighWaterMark[c] = RecycledContents[c];
    }
#endif
  } else {
#if LISTRECYCLERDEBUG
    if (c<=MAXRECYCLESIZE) {
      ListsDestroyed[c]++;
    }
#endif
#ifndef NDEBUG
    if (list->capacity>0) list->data[0]=NULL;
#endif
    ascfree(list->data);
    list->data = NULL;
    list->capacity = list->length = 0;
    POOL_FREEHEAD(list);
  }
}

VOIDPTR gl_fetchF(CONST struct gl_list_t *list, unsigned long int pos)
{
  asc_assert(NULL != list);
  ASSERTRANGE(list,pos,"gl_fetchF");
  return list->data[pos-1];
}

#define SORTED_OFF(l) (l)->flags &= (~gsf_SORTED)
#define EXPAND_OFF(l) (l)->flags &= (~gsf_EXPANDABLE)

/* for internal use only, where we can verify that STUFF is correct
 * and it is SOMEONE else job to maintain the integrity of
 * flags.sorted and list-> length.
 */
#ifdef NDEBUG
#define GL_STORE(list,pos,ptr) ((list)->data[(pos)-1] = (ptr))
#else
#define GL_STORE(list,pos,ptr) gl_store((list),(pos),(ptr))
#endif
void gl_store(struct gl_list_t *list, unsigned long int pos, VOIDPTR ptr)
{
  asc_assert(NULL != list);
  if (GL_LENGTH(list)>1) SORTED_OFF(list);
  ASSERTRANGE(list,pos,"gl_store");
  list->data[pos-1] = ptr;
}

#if REALLOCDEBUG
/* this we know to not leak if MOD_REALLOC is defined */
#define DATAREALLOC(l,incr) \
  ascreallocPURE((char *)(l)->data, \
                 (size_t)(((l)->capacity-(incr))*sizeof(VOIDPTR)), \
                 (size_t)((l)->capacity*sizeof(VOIDPTR)))
#else
/* this we think might leak under hpux, sunos4. more like purify lies. */
#define DATAREALLOC(l,incr) \
  ascrealloc((l)->data,(unsigned)((l)->capacity*sizeof(VOIDPTR)))
#endif
/* actually, when all said and done, a static structure in solver/analyze.c
 * was being left out of the cleanup process at shutdown.
 */

static void gl_expand_list(struct gl_list_t *list)
{
  VOIDPTR *tmp;
  unsigned long increment;
  increment = (list->capacity*50)/100; /* 10% is too small. try 50% baa */
  increment = MAX(MIN_INCREMENT,increment);
  list->capacity += increment;
  tmp = (VOIDPTR *) DATAREALLOC(list,increment);

  if (tmp==NULL) {
    PRINTF("gl_expand_list: memory allocation failed\n");
    list->capacity -= increment;
  } else {
    list->data = tmp;
  }
}

/* changes only the capacity of the list, and possibly data pointer */
static void gl_expand_list_by(struct gl_list_t *list,unsigned long addlen)
{
  if (!addlen) return;
  addlen = MAX(MIN_INCREMENT,addlen); /* expand by at least 8 */
  list->capacity += addlen;
  list->data = (VOIDPTR *)DATAREALLOC(list,addlen);

  if (list->data==NULL) PRINTF("gl_expand_list_by: memory allocation failed\n");
  asc_assert(list->data!=NULL);
}

void gl_append_ptr(struct gl_list_t *list, VOIDPTR ptr)
{
  asc_assert((NULL != list) && (0 != gl_expandable(list)));
  if (list->length > 0) SORTED_OFF(list);
  if (++(list->length) > list->capacity) /* expand list capacity*/
    gl_expand_list(list);
  list->data[list->length-1] = ptr;
}

void gl_append_list(struct gl_list_t *extendlist, struct gl_list_t *list)
{
  register unsigned long c,len,oldlen,newlen;

  asc_assert((NULL != extendlist) &&
             (0 != gl_expandable(extendlist)) &&
             (NULL != list));
  if (extendlist->length >0 || list->length > 0) {
    SORTED_OFF(extendlist);
  }
  newlen = extendlist->length + list->length;
  if (newlen > extendlist->capacity) {
    gl_expand_list_by(extendlist,newlen - extendlist->capacity);
  }
  len = list->length;
  oldlen = extendlist->length;
  extendlist->length += len;
  for (c=0;c<len;c++) {
    extendlist->data[oldlen] = list->data[c];
    oldlen++;
  }
}

void gl_fast_append_ptr(struct gl_list_t *list, VOIDPTR ptr)
{
  asc_assert(NULL != list);
  SORTED_OFF(list);
  list->data[list->length] = ptr;
  ++(list->length);
}

unsigned long gl_safe_length(CONST struct gl_list_t *list)
{
  if (list !=NULL) {
    return list->length;
  } else {
    return 0L;
  }
}
unsigned long gl_lengthF(CONST struct gl_list_t *list)
{
  asc_assert(NULL != list);
  return list->length;
}

unsigned long gl_capacity(CONST struct gl_list_t *list)
{
  if (list==NULL) return 0;
  return list->capacity;
}


int gl_sorted(CONST struct gl_list_t *list)
{
  asc_assert(NULL != list);
  return (int)(list->flags & gsf_SORTED);
}

#if LISTIMPLEMENTED
/* I believe this function is ok */
static void *gl_choose_ptrpivot(struct gl_list_t *list,
			      unsigned long int lower,
			      unsigned long int upper)
/*********************************************************************\
* This function will choose a pivot.  It can actually return any
* number between and including upper and lower.  However, the goal
* is to return a pivot that will not cause Quick Sort to have
* O(n^2) behavior. This returns the value rather than the address
* of the pivot.
*
* The heuristic I am choosing is to pick the middle of three test
* sites which are upper,lower, and (upper+lower)/2.
\*********************************************************************/
{
  unsigned long middle;
  middle = (upper+lower)/2;
  if  (gl_fetch(list,middle) > gl_fetch(list,lower)) {
    /* middle > lower */
    if (gl_fetch(list,upper) > gl_fetch(list,middle)) {
      /* middle > lower && upper > middle */
      return gl_fetch(list,middle);
    }
    else {
      /* middle > lower && middle >= upper */
      if (gl_fetch(list,upper) > gl_fetch(list,lower)) {
	/* middle > lower && middle >= upper && upper > lower */
	return gl_fetch(list,upper);
      }
      else {
	/* middle > lower && middle >= upper && lower >= upper */
	return gl_fetch(list,lower);
      }
    }
  }
  else {
    /* lower >= middle */
    if (gl_fetch(list,upper) > gl_fetch(list,lower)) {
      /* lower >= middle && upper > lower */
      return gl_fetch(list,lower);
    }
    else {
      /* lower >= middle && lower >= upper */
      if (gl_fetch(list,middle) > gl_fetch(list,upper)) {
	/* lower >= middle && lower >= upper && middle > upper */
	return gl_fetch(list,middle);
      }
      else {
	/* lower >= middle && lower >= upper && upper >= middle */
	return gl_fetch(list,upper);
      }
    }
  }
}
#endif

static
unsigned long gl_choose_pivot(struct gl_list_t *list,
			      unsigned long int lower,
			      unsigned long int upper,
			      CmpFunc func)
/*
 * This function will choose a pivot.  It can actually return any
 * number between and including upper and lower.  However, the goal
 * is to return a pivot that will not cause Quick Sort to have
 * O(n^2) behavior.
 *
 * The heuristic I am choosing is to pick the middle of three test
 * sites which are upper,lower, and (upper+lower)/2.
 */
{
  unsigned long middle;
  middle = (upper+lower)/2;
  if ((*func)(gl_fetch(list,middle),gl_fetch(list,lower)) > 0) {
    /* middle > lower */
    if ((*func)(gl_fetch(list,upper),gl_fetch(list,middle)) > 0) {
      /* middle > lower && upper > middle */
      return middle;
    }
    else {
      /* middle > lower && middle >= upper */
      if ((*func)(gl_fetch(list,upper),gl_fetch(list,lower)) > 0) {
	/* middle > lower && middle >= upper && upper > lower */
	return upper;
      }
      else {
	/* middle > lower && middle >= upper && lower >= upper */
	return lower;
      }
    }
  }
  else {
    /* lower >= middle */
    if ((*func)(gl_fetch(list,upper),gl_fetch(list,lower)) > 0) {
      /* lower >= middle && upper > lower */
      return lower;
    }
    else {
      /* lower >= middle && lower >= upper */
      if ((*func)(gl_fetch(list,middle),gl_fetch(list,upper)) > 0) {
	/* lower >= middle && lower >= upper && middle > upper */
	return middle;
      }
      else {
	/* lower >= middle && lower >= upper && upper >= middle */
	return upper;
      }
    }
  }
}

/* do not export. makes rosy assumptions. callers of this function
 * are responsible for making sure list.sorted is set appropriately.
 */
static void gl_swap(struct gl_list_t *list,
                    unsigned long int i, unsigned long int j)
{
  VOIDPTR temp;
  temp = gl_fetch(list,i);
  GL_STORE(list,i,gl_fetch(list,j));
  GL_STORE(list,j,temp);
}

#if LISTIMPLEMENTED
/* this function is probably ok */
void gl_upsort(struct gl_list_t *list,
               unsigned long int lower,
               unsigned long int upper)
{
  register unsigned long i,j;
  VOIDPTR pivot_element;
  j = upper;
  i = lower;
  pivot_element = gl_choose_ptrpivot(list,lower,upper);
  do {
    while (pivot_element > gl_fetch(list,i)) i += 1;
    while (gl_fetch(list,j) > pivot_element)  j -= 1;
    if (i <= j) {
      register VOIDPTR temp;
      temp = gl_fetch(list,i);
      GL_STORE(list,i,gl_fetch(list,j));
      GL_STORE(list,j,temp);
      i++; j--;
    }
  } while(i <= j);
  if (lower < j) gl_upsort(list,lower,j);
  if (i < upper) gl_upsort(list,i,upper);
}

/* this function is not ok */
void gl_downsort(struct gl_list_t *list,
                 unsigned long int lower,
                 unsigned long int upper)
{
  unsigned long pivot,i,j;
  VOIDPTR pivot_element;
  j = upper;
  i = lower;
  pivot_element = gl_choose_ptrpivot(list,lower,upper);
  do {
    while ((*func)(pivot_element,gl_fetch(list,i))>0) i += 1;
    while ((*func)(gl_fetch(list,j),pivot_element)>0) j -= 1;
    if (i <= j) {
      gl_swap(list,i++,j--);
    }
  } while(i <= j);
  if (lower < j) gl_downsort(list,lower,j);
  if (i<upper)   gl_downsort(list,i,upper);
}
#endif

static
void gl_qsort(struct gl_list_t *list,
              unsigned long int lower,
              unsigned long int upper,
              CmpFunc func)
{
  unsigned long pivot,i,j;
  VOIDPTR pivot_element;
  j = upper;
  i = lower;
  pivot = gl_choose_pivot(list,lower,upper,func);
  pivot_element = gl_fetch(list,pivot);
  do {
    while ((*func)(pivot_element,gl_fetch(list,i))>0) i += 1;
    while ((*func)(gl_fetch(list,j),pivot_element)>0) j -= 1;
    if (i <= j) {
      gl_swap(list,i++,j--);
    }
  } while(i <= j);
  if (lower < j) gl_qsort(list,lower,j,func);
  if (i<upper)   gl_qsort(list,i,upper,func);
}

#if LISTIMPLEMENTED
/* ok once its children are ok */
void gl_ptr_sort(struct gl_list_t *list, int increasing)
{
  if (GL_LENGTH(list) > 1) {
    if (increasing) {
      gl_upsort(list,1,GL_LENGTH(list));
    } else {
      gl_downsort(list,1,GL_LENGTH(list));
    }
  }
  list->flags |= gsf_SORTED;
}
#endif

#if LISTIMPLEMENTED
/* broken */
void gl_insert_ptr_sorted(struct gl_list_t *,VOIDPTR,int)
{
}
#endif

void gl_sort(struct gl_list_t *list, CmpFunc func)
{
  asc_assert((NULL != list) && (NULL != func));
  if (GL_LENGTH(list) > 1) {
    gl_qsort(list,1,GL_LENGTH(list),func);
  }
  list->flags |= gsf_SORTED;
}

void gl_insert_sorted(struct gl_list_t *list,
		      VOIDPTR ptr, CmpFunc func)
{
  int comparison;
  register unsigned long lower,upper,search=0L;
  
  asc_assert((NULL != list) &&
             (0 != gl_expandable(list)) &&
             (NULL != func));
  if (list->flags & gsf_SORTED) {
    if (GL_LENGTH(list)==0) {
      gl_append_ptr(list,ptr);
      list->flags |= gsf_SORTED;
      return;
    }
    if (++list->length > list->capacity) /* expand list capacity */
      gl_expand_list(list);
    lower = 1;
    upper = GL_LENGTH(list)-1;
    while (lower<=upper) {
      search = ( lower + upper) >> 1; /* divide by two */
      comparison = (*func)(gl_fetch(list,search),ptr);
      if (comparison==0) {
	/* make room */
	for(lower=GL_LENGTH(list);lower>search;lower--)
	  GL_STORE(list,lower,gl_fetch(list,lower-1));
	/* store it */
	GL_STORE(list,search+1,ptr);
        list->flags |= gsf_SORTED;
	return;
      }
      if (comparison < 0) lower = search + 1;
      else upper = search - 1;
    }
    if ((*func)(gl_fetch(list,search),ptr) > 0) {
      for(lower=GL_LENGTH(list);lower>search;lower--) {
	GL_STORE(list,lower,gl_fetch(list,lower-1));
      }
      GL_STORE(list,search,ptr);
    }
    else {
      for(lower=GL_LENGTH(list);lower>(search+1);lower--) {
	GL_STORE(list,lower,gl_fetch(list,lower-1));
      }
      GL_STORE(list,search+1,ptr);
    }
    list->flags |= gsf_SORTED;
  }
  else {
    PRINTF(
      "Warning gl_insert_sorted called on unsorted list.\nSorting list.\n");
    gl_append_ptr(list,ptr);
    gl_sort(list,func);
  }
}

void gl_iterate(struct gl_list_t *list, void (*func) (VOIDPTR))
{
#ifdef NDEBUG
  register unsigned long length,counter;
  length = GL_LENGTH(list);
  for (counter=0;counter<length;counter++)
    (*func)(list->data[counter]);
#else
  unsigned long length,counter;
  asc_assert(NULL != list);
  asc_assert(NULL != func);
  length = GL_LENGTH(list);
  for (counter=1;counter<=length;counter++)
    (*func)(gl_fetch(list,counter));
#endif
}

/* this will probably need casts to compile or an intermediate
 * void *array variable
 * this function is on the critical path in the compiler, or
 * should be, so it is highly tweaked here.
 */
unsigned long gl_ptr_search(CONST struct gl_list_t *list,
			CONST VOIDPTR match, int increasing)
{
#define SHORTSEARCH 5
/* if list is short, use linear instead of binary search.
 * SHORTSEARCH is the minimum size for a binary search.
 * SHORTSEARCH must be >=0
 */
  register unsigned long lower,upper,search;

  asc_assert(NULL != list);
  if (!GL_LENGTH(list)) return 0;
  if ( (list->flags & gsf_SORTED)
#if (SHORTSEARCH > 0)
       && (upper = GL_LENGTH(list)-1) > SHORTSEARCH
#endif
     ) {		/* use binary search */
    register long comparison;
    lower = 0;
#if (SHORTSEARCH <= 0)
    upper = GL_LENGTH(list)-1;
#endif
    if (increasing) {
      while (lower <= upper) {
        search = (lower+upper) / 2;
        comparison =
          ((char *)list->data[search]-(char *)match); /* pointer difference */
        if (comparison==0) {
          return (search + 1);
        }
        else if (comparison < 0) {
          lower = search + 1;
        }
        else if (search > 0) {
          upper = search - 1;
        }
        else {
          return 0;
        }
      }
    } else {
      while (lower <= upper) {
        search = (lower+upper) / 2;
        comparison =
          ((char *)match-(char *)list->data[search]); /* pointer difference */
        if (comparison==0) {
          return (search + 1);
        }
        else if (comparison < 0) {
          lower = search + 1;
        } 
        else if (search > 0) {
          upper = search - 1;
        } 
        else {
          return 0;
        }
      }
    }
  }
  else {				/* use linear search */
    upper = GL_LENGTH(list);
    for(search=0; search < upper; search++) {
      if (list->data[search]==match) return search+1;
    }
  }
  return 0;				/* search failed */
}

unsigned long gl_search(CONST struct gl_list_t *list,
			CONST VOIDPTR match, CmpFunc func)
{
  register unsigned long lower,upper,search;
#ifdef __alpha
  long comparison;
#else
  int comparison;
#endif
  asc_assert((NULL != list) && (NULL != func));
  if (list->flags & gsf_SORTED) {		/* use binary search */
    lower = 1;
    upper = GL_LENGTH(list);
    while (lower <= upper) {
      search = (lower+upper) / 2;
      comparison = (*func)(gl_fetch(list,search),match);
      if (comparison==0) return search;
      if (comparison < 0) lower = search + 1;
      else upper = search -1;
    }
  }
  else {				/* use linear search */
    upper = GL_LENGTH(list);
    for(search=0; search < upper; search++) {
      if ((*func)(list->data[search],match)==0) return search+1;
    }
  }
  return 0;				/* search failed */
}


unsigned long gl_search_reverse(CONST struct gl_list_t *list,
                                CONST VOIDPTR match, CmpFunc func)
{
  register unsigned long search;

  asc_assert((NULL != list) && (NULL != func));
  if (list->flags & gsf_SORTED) {	/* use binary search */
    return gl_search(list, match, func);
  } else {				/* use linear search */
    /* since search is unsigned, the FOR loop cannot go to
     * zero, since 0-- is ULONG_MAX.  Must do zero separately.
     */
    for( search = (GL_LENGTH(list)-1); search > 0; search-- ) {
      if ((*func)(list->data[search],match)==0) return search+1;
    }
    search = 0;
    if ((*func)(list->data[search],match)==0) return search+1;
  }
  return 0;				/* search failed */
}


int gl_unique_list(CONST struct gl_list_t *list)
{
  unsigned long i,j,len,lm1;
  VOIDPTR e;
  VOIDPTR *d;

  if (list==NULL || GL_LENGTH(list)<2) {
    return 1;
  }
  len = GL_LENGTH(list);
  lm1 = len-1;
  d = list->data;
  for (i=0; i<lm1; i++) {
    e = d[i];
    for (j=i+1; j < len; j++) {
      if (d[j]==e) {
        return 0;
      }
    }
  }
  return 1;
}

int gl_empty(CONST struct gl_list_t *list)
{
  asc_assert(NULL != list);
  return(GL_LENGTH(list)==0);
}

void gl_delete(struct gl_list_t *list, unsigned long int pos, int dispose)
{
  unsigned long c,length;
  int sorted;
  VOIDPTR ptr;

  asc_assert(NULL != list);
  /* should be ASSERTRANGE(list,pos,"gl_delete");  
     The check for (pos > 0) below is suspicious, though.
     It suggests this function may be called with pos == 0. */
  asc_assert(pos <= gl_length(list));

  if (pos > 0) {
    sorted = (int)(list->flags & gsf_SORTED);
    if (dispose) {
      ptr = gl_fetch(list,pos);
      ascfree(ptr);
    }
    length = GL_LENGTH(list);
#ifdef NDEBUG
    /* the copyleft the remainder of the array. easy-eye version */
    for (c = pos; c < length; c++) {
      list->data[c-1] = list->data[c];
    }
#else
    /* shift the remainder of the array using the macros  */
    for (c=pos+1;c<=length;c++) {
      GL_STORE(list,c-1,gl_fetch(list,c));
    }
#endif
    list->length--;
    if (sorted) {
      list->flags |= gsf_SORTED;
    } else {
      SORTED_OFF(list);
    }
  }
}

void gl_reverse(struct gl_list_t *list)
{
  VOIDPTR *tmpdata;
  unsigned long c,len;

  if (list==NULL || list->length <2L) return;
  tmpdata = (VOIDPTR *)ascmalloc(list->capacity*sizeof(VOIDPTR));
  if (tmpdata==NULL) {
    Asc_Panic(2, NULL, "gl_reverse out of memory. Bye!\n");
  }
  len = list->length;
  for (c=0;c < len;c++) {
    tmpdata[len - (c+1)] = list->data[c];
  }
  ascfree(list->data);
  list->data = tmpdata;
}

void gl_reset(struct gl_list_t *list)
{
  asc_assert(NULL != list);
  list->flags = (unsigned int)(gsf_SORTED | gsf_EXPANDABLE);
  list->length = 0;
}


struct gl_list_t *gl_copy(CONST struct gl_list_t *list)
{
  struct gl_list_t *new;
  unsigned long counter,length;

  asc_assert(NULL != list);
  
  length = GL_LENGTH(list);
  new = gl_create(length);
  for (counter = 1;counter <= length;counter++)
    gl_append_ptr(new,gl_fetch(list,counter));
  new->flags = list->flags;
  return new;
}

struct gl_list_t *gl_concat(CONST struct gl_list_t *list1,
			    CONST struct gl_list_t *list2)
{
  struct gl_list_t *new;
  unsigned long counter,length1,length2;
  
  asc_assert ((NULL != list1) && (NULL != list2));

  length1 = GL_LENGTH(list1);
  length2 = GL_LENGTH(list2);
  new = gl_create(length1+length2);
  for (counter = 1;counter<= length1; counter++) {
    gl_append_ptr(new,gl_fetch(list1,counter));
  }
  for (counter = 1;counter<= length2; counter++) {
    gl_append_ptr(new,gl_fetch(list2,counter));
  }
  return new;
}

int gl_compare_ptrs(CONST struct gl_list_t *l1, CONST struct gl_list_t *l2)
{
  unsigned long len,c;
  register unsigned long i1,i2;
  if (l1==NULL || l2 == NULL) {
    Asc_Panic(2,"gl_compare_ptrs","Called with NULL input");
  }
  len = MIN(GL_LENGTH(l1),GL_LENGTH(l2));
  for (c=0;c < len; c++) {
    i1 = (unsigned long)(l1->data[c]);
    i2 = (unsigned long)(l2->data[c]);
    if (i1<i2) {
      return -1;
    }
    if (i1>i2) {
      return 1;
    }
  }
  return (GL_LENGTH(l1) == GL_LENGTH(l2)) ? 
          0 : 
          (( GL_LENGTH(l2) > len) ? -1 : 1);
}

void gl_set_sorted(struct gl_list_t *list, int TRUE_or_FALSE)
{
  asc_assert(NULL != list);
  if (FALSE == TRUE_or_FALSE)
    list->flags &= ~gsf_SORTED;
  else
    list->flags |= gsf_SORTED;
}

int gl_expandable(struct gl_list_t *list)
{
  asc_assert(NULL != list);
  return (int)(list->flags & gsf_EXPANDABLE);
}

void gl_set_expandable(struct gl_list_t *list, int TRUE_or_FALSE)
{
  asc_assert(NULL != list);
  if (FALSE == TRUE_or_FALSE)
    list->flags &= ~gsf_EXPANDABLE;
  else
    list->flags |= gsf_EXPANDABLE;
}


VOIDPTR *gl_fetchaddr(CONST struct gl_list_t *list,
		     unsigned long int pos)
{
  asc_assert(NULL != list);
  ASSERTRANGE(list,pos,"gl_fetchaddr");
  return (VOIDPTR *)&(list->data[pos-1]);
}


void gl_emptyrecycler()
{
  int i;
#if LISTRECYCLERDEBUG
  unsigned long bytecount = 0;
#endif
  struct gl_list_t *list;

#if LISTRECYCLERDEBUG                                         
  PRINTF("LIST RECYCLER SUMMARY\n");
  PRINTF("=====================\n");
  PRINTF("New lists created  : %d\n", NewListsUsed);
  PRINTF("Recycled lists used: %d\n", RecycledListsUsed);
  ListsDestroyedTotal = 0;
  for (i=0;i <=MAXRECYCLESIZE; i++) {
     ListsDestroyedTotal += ListsDestroyed[i];
  }
  PRINTF("Lists destroyed    : %d\n", ListsDestroyedTotal);
  PRINTF("Size\tAllowed\tCurr.\tMax\tDestroyed\n");
#endif
  for(i = 0 ; i <= MAXRECYCLESIZE ; i++){
#if LISTRECYCLERDEBUG
    if (HighWaterMark[i] >0) {
      PRINTF("%d\t%d\t%d\t%d\t%d\n",i,AllowedContents[i], RecycledContents[i],
        HighWaterMark[i], ListsDestroyed[i]);
    }
    bytecount += sizeof(struct gl_list_t)*RecycledContents[i];
    bytecount += sizeof(void *)*i*RecycledContents[i];
#endif
    while (RecycledContents[i] > 0){
      /* pop a list off the LIFO list */
      list = RecycledList[i];
      RecycledList[i] = (struct gl_list_t *)list->data[0];
#ifndef NDEBUG
      list->data[0] = NULL;
#endif
      --RecycledContents[i];
      ascfree(list->data);
#ifndef NDEBUG
      list->data = NULL;
      list->capacity = list->length = 0;
#endif
      POOL_FREEHEAD(list);
    }
    asc_assert(RecycledList[i]==NULL); /* someone forgot to empty the last one */
  }
#if LISTRECYCLERDEBUG
  bytecount += MAXRECYCLESIZE*sizeof(void *);
  PRINTF("Total bytes:\t%lu\n",bytecount);
#endif
}

void gl_reportrecycler(FILE *fp)
{
  int i;
  unsigned long bytecount = 0;
#if !LISTRECYCLERDEBUG
  (void) fp;
#endif

#if LISTRECYCLERDEBUG
  asc_assert(NULL != fp);
  FPRINTF(fp,"LIST RECYCLER SUMMARY\n");
  FPRINTF(fp,"=====================\n");
  FPRINTF(fp,"New lists created  : %d\n", NewListsUsed);
  FPRINTF(fp,"Recycled lists used: %d\n", RecycledListsUsed);
  for (i=0;i <=MAXRECYCLESIZE; i++) {
     ListsDestroyedTotal += ListsDestroyed[i];
  }
  FPRINTF(fp,"Lists destroyed    : %d\n", ListsDestroyedTotal);
  FPRINTF(fp,"Size\tAllowed\tCurr.\tMax\tDestroyed\n");
#endif

  for(i = 0 ; i <= MAXRECYCLESIZE ; i++){
#if LISTRECYCLERDEBUG
    if (HighWaterMark[i] >0) {
      FPRINTF(fp,"%d\t%d\t%d\t%d\t%d\n",i,AllowedContents[i],
        RecycledContents[i], HighWaterMark[i], ListsDestroyed[i]);
      bytecount += sizeof(struct gl_list_t)*RecycledContents[i];
      bytecount += sizeof(void *)*i*RecycledContents[i];
    }
#else
    bytecount += sizeof(struct gl_list_t)*RecycledContents[i];
    bytecount += sizeof(void *)*RecycledContents[i]*i;
#endif
  }
#if LISTRECYCLERDEBUG
  bytecount += MAXRECYCLESIZE*sizeof(void *);
  FPRINTF(fp,"Total bytes:\t%lu\n",bytecount);
#endif
}
