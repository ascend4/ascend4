/*
 *  Unit test functions for ASCEND: general/list.c
 *
 *  Copyright (C) 2005 Jerry St.Clair
 *
 *  This file is part of the Ascend Environment.
 *
 *  The Ascend Environment is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Environment is distributed in hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#include <stdlib.h>
#include <stdio.h>
#include "utilities/ascConfig.h"
#include "utilities/AscMalloc.h"
#include "utilities/ascPrintType.h"
#include "utilities/ascPrint.h"
#include "general/list.h"
#include "CUnit/CUnit.h"
#include "assertimpl.h"
#include "test_list.h"

/* comparison function used in test_list(). */
static
int compare_addresses(CONST VOIDPTR p1, CONST VOIDPTR p2)
{
  return (int)((int*)p1 - (int*)p2);
}

/* comparison function used in test_list(). */
static
int compare_addresses_reverse(CONST VOIDPTR p1, CONST VOIDPTR p2)
{
  return (int)((int*)p2 - (int*)p1);
}

/* comparison function used in test_list(). */
static
int compare_ints(CONST VOIDPTR p1, CONST VOIDPTR p2)
{
  assert((NULL != p1) && (NULL != p2));
  return *((int*)p1) - *((int*)p2);
}

/* transform function used in test_list(). */
static
void mult_by_2(VOIDPTR p)
{
  if (NULL != p)
    *((unsigned long*)p) = *((unsigned long*)p) * 2;
}

/* Returns the position [1..gl_length(list)] of a pointer in a list, or 0 if not found. */
static
unsigned long find_ptr_pos(const struct gl_list_t *list, VOIDPTR ptr)
{
  unsigned long i;

  assert(NULL != list);
  for(i=0 ; i<gl_length(list) ; ++i) {
    if (ptr == gl_fetch(list, (i+1)))
      return (i+1);
  }
  return 0;
}

/* Returns the position [1..gl_length(list)] of a data value in a list, or 0 if not found. */
static
unsigned long find_int_pos(const struct gl_list_t *list, unsigned long value)
{
  unsigned long i;
  unsigned long* ptr;

  assert(NULL != list);
  for(i=0 ; i<gl_length(list) ; ++i) {
    ptr = (unsigned long*)gl_fetch(list, (i+1));
    if ((NULL != ptr) && (value == *ptr))
      return (i+1);
  }
  return 0;
}

/* number of discarded lists to track for memory deallocation */
#define MAX_LISTS_TO_TRACK 50

/*  Note - this function tests EITHER the pooled or unpooled versions
 *  of list.c.  It tests whichever version was compiled into the ASCEND
 *  base library.  This is usually the pooled version.
 */
static void test_list(void)
{
  /* array to hold discarded list pointers to check for deallocation */
  struct gl_list_t *p_used_lists[MAX_LISTS_TO_TRACK];
  unsigned long n_used_lists = 0;       /* # of lists stored in same */

  struct gl_list_t *p_list1;
  struct gl_list_t *p_list2;
  struct gl_list_t *p_list3;
  unsigned long capacity;
  unsigned long i;
  unsigned long *pint_array[20];
  int lists_were_active = FALSE;
  int i_initialized_lists = FALSE;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();                     /* save meminuse() at start of function*/

#ifdef NDEBUG
  CU_FAIL("test_list() compiled with NDEBUG - some features not tested.");
#endif
#ifndef MALLOC_DEBUG
  CU_FAIL("test_list() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  /* set up pooling & recycling */
#ifdef LISTUSESPOOL
  CU_TEST(FALSE == gl_pool_initialized());
#else
  CU_TEST(TRUE == gl_pool_initialized());
#endif

  if (FALSE == gl_pool_initialized()) {                 /* initialize list system if necessary */
    gl_init();
    gl_init_pool();
    i_initialized_lists = TRUE;
  }
  else {
    lists_were_active = TRUE;
  }

  CU_TEST(TRUE == gl_pool_initialized());

  /*  NOTE:  Each test section assumes that
   *    1. pooling & recycling has been set up
   *    2. the local gl_list_t* have been destroyed
   *    3. pint_array[0..19] is allocated and initialized to [0..19]
   *                                                                             
   *  If a test section messes with any of these, then it must restore
   *  this state before finishing.
   */

  for (i=0 ; i<20 ; ++i) {              /* create some test data */
      pint_array[i] = (unsigned long*)ascmalloc(sizeof(unsigned long));
      *pint_array[i] = i;
  }

  for (i=0 ; i<MAX_LISTS_TO_TRACK ; ++i) {  /* initialize the array of used lists */
    p_used_lists[i] = NULL;
  }

  /* test gl_create(), gl_destroy(), gl_free_and_destroy() */

  p_list1 = gl_create(0);               /* create a list having initial capacity = 0 */
  CU_TEST(0 < gl_capacity(p_list1));
  CU_TEST(0 == gl_length(p_list1));
  CU_TEST(0 == gl_safe_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(0 != gl_expandable(p_list1));
  CU_TEST(0 != gl_empty(p_list1));
  CU_TEST(TRUE == gl_unique_list(p_list1));

  capacity = gl_capacity(p_list1);      /* store the minimum capacity for later use  */
#ifdef MALLOC_DEBUG
  CU_TEST(0 != AllocatedMemory((VOIDPTR)p_list1, capacity * sizeof(VOIDPTR)));
#endif

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* destroy the list and check for deallocation */
#ifdef MALLOC_DEBUG
#ifdef LISTUSESPOOL
  CU_TEST(0 != AllocatedMemory((VOIDPTR)p_list1, capacity * sizeof(VOIDPTR)));
#else
  CU_TEST(0 == AllocatedMemory((VOIDPTR)p_list1, capacity * sizeof(VOIDPTR)));
#endif  /* LISTUSESPOOL */
#endif  /* MALLOC_DEBUG */

  p_list1 = gl_create(10);              /* create a new list with capacity = 10 */
  CU_TEST(10 <= gl_capacity(p_list1));
  CU_TEST(0 == gl_length(p_list1));
  CU_TEST(0 == gl_safe_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(0 != gl_expandable(p_list1));
  CU_TEST(0 != gl_empty(p_list1));
  CU_TEST(TRUE == gl_unique_list(p_list1));

  for (i=0 ; i<10 ; ++i) {              /* fill the list with data */
      gl_append_ptr(p_list1, pint_array[i]);
  }
  CU_TEST(10 <= gl_capacity(p_list1));
  CU_TEST(10 == gl_length(p_list1));
  CU_TEST(10 == gl_safe_length(p_list1));
  CU_TEST(0 == gl_sorted(p_list1));
  CU_TEST(0 != gl_expandable(p_list1));
  CU_TEST(0 == gl_empty(p_list1));
  CU_TEST(TRUE == gl_unique_list(p_list1));

#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {              /* check that all pointers are still active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up, leaving data in tact */

#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {              /* check that data is still in tact */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  p_list1 = gl_create(5);               /* create the list again, fill it with half the data */
  for (i=0 ; i<5 ; ++i) {
      gl_append_ptr(p_list1, pint_array[i]);
  }

#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* check that all pointers are still active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_free_and_destroy(p_list1);         /* clean up, deallocating data stored in list */

#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* check that some data was deallocated */
    if (i < 5) {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  for (i=5 ; i<10 ; ++i) {              /* deallocate the rest of the data */
    ascfree(pint_array[i]);
  }

#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {               /* confirm that all data is now deallocated */
    if (i < 10) {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  for (i=0 ; i<10 ; ++i) {               /* restore test array */
      pint_array[i] = (unsigned long*)ascmalloc(sizeof(unsigned long));
      *pint_array[i] = i;
  }

  /* test gl_fetch() */

  p_list1 = gl_create(10);              /* create and fill a list */

  for (i=0 ; i<10 ; ++i) {
      gl_append_ptr(p_list1, pint_array[i]);
  }

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_fetch(p_list1, 0);               /* error if pos out of range (< 1) */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_fetch(p_list1, 11);               /* error if pos out of range (> gl_length(list) */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_fetch(NULL, 1);                  /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  for (i=0 ; i<10 ; ++i) {
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(i == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_store() */

  p_list1 = gl_create(10);              /* create and fill a list */

  for (i=0 ; i<10 ; ++i) {
      gl_append_ptr(p_list1, pint_array[i]);
  }

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_store(p_list1, 0, NULL);         /* error if pos out of range (< 1) */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_store(p_list1, 11, NULL);        /* error if pos out of range (> gl_length(list) */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_store(NULL, 1, NULL);            /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  for (i=0 ; i<10 ; ++i) {              /* modify the list data & check for accuracy */
    gl_store(p_list1, i+1, pint_array[9-i]);
  }

  for (i=10 ; i>0 ; --i) {
    CU_TEST(pint_array[10-i] == gl_fetch(p_list1, i));
    CU_TEST((10-i) == *((unsigned long*)gl_fetch(p_list1, i)));
  }

  for (i=5 ; i<10 ; ++i) {              /* should be able to store NULL pointers also */
    gl_store(p_list1, i+1, NULL);
  }

  for (i=10 ; i>5 ; --i) {
    CU_TEST(NULL == gl_fetch(p_list1, i));
  }

  for (i=5 ; i>0 ; --i) {
    CU_TEST(pint_array[10-i] == gl_fetch(p_list1, i));
    CU_TEST((10-i) == *((unsigned long*)gl_fetch(p_list1, i)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_append_ptr() */

  p_list1 = gl_create(0);               /* create an empty list with minimal capacity */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  CU_TEST(0 != gl_expandable(p_list1));
  gl_set_expandable(p_list1, FALSE);    /* set list non-expandable */
  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_append_ptr(p_list1, NULL);       /* error if list not expandable */
  CU_TEST(TRUE == asc_assert_failed());
  CU_TEST(0 == gl_length(p_list1));

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_append_ptr(NULL, NULL);          /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gl_set_expandable(p_list1, TRUE);     /* reset list to expandable */

  CU_TEST(0 == gl_length(p_list1));

  for (i=0 ; i<10 ; ++i) {              /* append some data to the list */
    gl_append_ptr(p_list1, pint_array[i]);
    CU_TEST((i+1) == gl_length(p_list1));
  }

  for (i=0 ; i<10 ; ++i) {              /* confirm the data in the list */
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }

  for (i=10 ; i<15 ; ++i) {             /* should be able to append NULL pointers also */
    gl_append_ptr(p_list1, NULL);
    CU_TEST((i+1) == gl_length(p_list1));
  }

  for (i=0 ; i<10 ; ++i) {              /* confirm the data in the list */
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }

  for (i=10 ; i<15 ; ++i) {
    CU_TEST(NULL == gl_fetch(p_list1, i+1));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_fast_append_ptr() */

  p_list1 = gl_create(10);               /* create an emptylist */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  CU_TEST(0 != gl_expandable(p_list1));
  gl_set_expandable(p_list1, FALSE);    /* set list non-expandable */
  CU_TEST(0 == gl_length(p_list1));

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_fast_append_ptr(NULL, NULL);     /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gl_set_expandable(p_list1, TRUE);     /* reset list to expandable */

  CU_TEST(0 == gl_length(p_list1));

  for (i=0 ; i<8 ; ++i) {               /* append some data to the list */
    gl_fast_append_ptr(p_list1, pint_array[i]);
    CU_TEST((i+1) == gl_length(p_list1));
  }

  for (i=0 ; i<8 ; ++i) {               /* confirm the data in the list */
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }

  for (i=8 ; i<10 ; ++i) {              /* should be able to append NULL pointers also */
    gl_fast_append_ptr(p_list1, NULL);
    CU_TEST((i+1) == gl_length(p_list1));
  }

  for (i=0 ; i<8 ; ++i) {               /* confirm the data in the list */
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }

  for (i=8 ; i<10 ; ++i) {
    CU_TEST(NULL == gl_fetch(p_list1, i+1));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_append_list() */

  p_list1 = gl_create(0);               /* create 2 empty lists */
  p_list2 = gl_create(0);

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  CU_TEST(0 != gl_expandable(p_list1));
  CU_TEST(0 != gl_expandable(p_list2));
  gl_set_expandable(p_list1, FALSE);    /* set list non-expandable */
  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_append_list(p_list1, p_list2);   /* error if 1st list not expandable */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_append_list(p_list2, p_list1);   /* ok if 2nd list not expandable */
  CU_TEST(FALSE == asc_assert_failed());

  CU_TEST(0 == gl_length(p_list1));     /* but, appending empty lists should have no effect */
  CU_TEST(0 == gl_length(p_list2));

  gl_set_expandable(p_list1, TRUE);     /* reset list to expandable */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_append_list(NULL, p_list2);      /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_append_list(p_list1, NULL);      /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  for (i=0 ; i<10 ; ++i) {              /* append some data to the lists */
    gl_append_ptr(p_list1, pint_array[i]);
  }
  CU_TEST(10 == gl_length(p_list1));
  CU_TEST(0 == gl_length(p_list2));

  gl_append_list(p_list1, p_list2);     /* appending empty list has no effect */
  CU_TEST(10 == gl_length(p_list1));
  CU_TEST(0 == gl_length(p_list2));

  gl_append_list(p_list2, p_list1);     /* appending to an empty list sould be ok */
  CU_TEST(10 == gl_length(p_list1));
  CU_TEST(10 == gl_length(p_list2));

  for (i=0 ; i<10 ; ++i) {              /* check the list contents */
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+1)));
    CU_TEST(pint_array[i] == gl_fetch(p_list2, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list2, i+1)));
  }

  gl_append_list(p_list1, p_list2);     /* appending full list should be ok */
  CU_TEST(20 == gl_length(p_list1));
  CU_TEST(10 == gl_length(p_list2));

  for (i=0 ; i<10 ; ++i) {              /* check the list contents */
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+1)));
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+11));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+11)));
    CU_TEST(pint_array[i] == gl_fetch(p_list2, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list2, i+1)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_destroy(p_list2);                  /* create a list with NULL pointer data */
  p_list2 = gl_create(2);
  gl_append_ptr(p_list2, NULL);
  gl_append_ptr(p_list2, NULL);

  gl_append_list(p_list1, p_list2);     /* appending list holding NULL pointers should be ok */
  CU_TEST(22 == gl_length(p_list1));
  CU_TEST(2 == gl_length(p_list2));

  for (i=0 ; i<10 ; ++i) {              /* check the list contents */
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+1));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+1)));
    CU_TEST(pint_array[i] == gl_fetch(p_list1, i+11));
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list1, i+11)));
  }

  for (i=0 ; i<2 ; ++i) {              /* check the list contents */
    CU_TEST(NULL == gl_fetch(p_list1, i+21));
    CU_TEST(NULL == gl_fetch(p_list2, i+1));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the lists, preserving data */
  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_destroy(p_list2);

  /* test (sort-of) gl_init(), gl_init_pool(), gl_destroy_pool() */

#ifdef MALLOC_DEBUG
  for (i=0 ; i<(MIN(n_used_lists, MAX_LISTS_TO_TRACK)) ; ++i) {
    CU_TEST(0 != AllocatedMemory((VOIDPTR)p_used_lists[i], capacity * sizeof(VOIDPTR)));
  }
#endif
  gl_destroy_pool();
#ifdef MALLOC_DEBUG
  for (i=0 ; i<(MIN(n_used_lists, MAX_LISTS_TO_TRACK)) ; ++i) {
    CU_TEST(0 == AllocatedMemory((VOIDPTR)p_used_lists[i], capacity * sizeof(VOIDPTR)));
  }
#endif

  gl_init_pool();
  gl_init();                            /* should be able to call this again without problems */

  n_used_lists = 0;

  /* test gl_length(), gl_safe_length() */

  p_list1 = gl_create(10);              /* create a list */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)gl_length(NULL);              /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    CU_TEST(0 == gl_safe_length(NULL)); /* no error if NULL list* for gl_safe_length() */
  CU_TEST(FALSE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  CU_TEST(0 == gl_length(p_list1));
  CU_TEST(0 == gl_safe_length(p_list1));
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
    CU_TEST((i+1) == gl_length(p_list1));
    CU_TEST((i+1) == gl_safe_length(p_list1));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_capacity() */

  p_list1 = gl_create(0);               /* create a list having minimum capacity */
  capacity = gl_capacity(p_list1);

  CU_TEST(0 == gl_capacity(NULL));      /* NULL is ok for gl_capacity() */

  for (i=0 ; i<=capacity ; ++i) {       /* append pointers to force capacity to expand */
    gl_append_ptr(p_list1, NULL);
  }
  CU_TEST(capacity < gl_capacity(p_list1));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  p_list1 = gl_create(capacity+1);      /* create a list having larger capacity */
  CU_TEST(capacity < gl_capacity(p_list1));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_sort(), gl_sorted(), gl_insert_sorted(), gl_set_sorted() */

  p_list1 = gl_create(0);               /* create a list having minimum capacity */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)gl_sorted(NULL);              /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_sort(NULL, compare_addresses);   /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_sort(p_list1, NULL);             /* error if NULL func */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_insert_sorted(NULL, NULL, compare_addresses);  /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_insert_sorted(p_list1, NULL, NULL);            /* error if NULL func */
  CU_TEST(TRUE == asc_assert_failed());

  CU_TEST(0 != gl_expandable(p_list1));
  gl_set_expandable(p_list1, FALSE);    /* set list non-expandable */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_insert_sorted(p_list1, NULL, compare_addresses);/* error if list not expandable */
  CU_TEST(TRUE == asc_assert_failed());

  gl_set_expandable(p_list1, TRUE);     /* reset list to expandable */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_set_sorted(NULL, TRUE);          /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  CU_TEST(0 != gl_sorted(p_list1));
  gl_sort(p_list1, compare_addresses);  /* sorting an empty list is ok but has no effect */
  CU_TEST(0 != gl_sorted(p_list1));

  gl_append_ptr(p_list1, NULL);
  CU_TEST(0 != gl_sorted(p_list1));
  gl_sort(p_list1, compare_addresses);  /* sorting a list with 1 element is ok but has no effect */
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(NULL == gl_fetch(p_list1, 1));
  gl_reset(p_list1);

  gl_insert_sorted(p_list1, pint_array[5], compare_addresses);
  CU_TEST(1 == gl_length(p_list1));     /* should be able to insert into an empty (sorted) list */
  CU_TEST(0 != gl_sorted(p_list1));

  gl_insert_sorted(p_list1, pint_array[2], compare_addresses);
  CU_TEST(2 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(gl_fetch(p_list1, 2) >= gl_fetch(p_list1, 1));

  gl_insert_sorted(p_list1, NULL, compare_addresses);
  CU_TEST(3 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(gl_fetch(p_list1, 3) >= gl_fetch(p_list1, 2));
  CU_TEST(gl_fetch(p_list1, 2) >= gl_fetch(p_list1, 1));

  gl_reset(p_list1);                    /* clear list for next test */

  gl_insert_sorted(p_list1, pint_array[5], compare_ints);
  CU_TEST(1 == gl_length(p_list1));     /* should be able to insert into an empty (sorted) list */
  CU_TEST(0 != gl_sorted(p_list1));

  gl_insert_sorted(p_list1, pint_array[2], compare_ints);
  CU_TEST(2 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(*((unsigned long*)gl_fetch(p_list1, 2)) >= *((unsigned long*)gl_fetch(p_list1, 1)));

  gl_insert_sorted(p_list1, pint_array[0], compare_ints);
  CU_TEST(3 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(*((unsigned long*)gl_fetch(p_list1, 3)) >= *((unsigned long*)gl_fetch(p_list1, 2)));
  CU_TEST(*((unsigned long*)gl_fetch(p_list1, 2)) >= *((unsigned long*)gl_fetch(p_list1, 1)));

  gl_set_sorted(p_list1, FALSE);        /* make sure gl_set_sorted() works */
  CU_TEST(0 == gl_sorted(p_list1));
  gl_set_sorted(p_list1, TRUE);
  CU_TEST(0 != gl_sorted(p_list1));
  gl_set_sorted(p_list1, FALSE);
  CU_TEST(0 == gl_sorted(p_list1));

  gl_reset(p_list1);                    /* clear list for next test */

  for (i=0 ; i<10 ; ++i) {              /* fill the list with unsorted data */
    gl_append_ptr(p_list1, pint_array[9-i]);
    if (i < 1) {
      CU_TEST(0 != gl_sorted(p_list1));
    }
    else {
      CU_TEST(0 == gl_sorted(p_list1));
    }
  }

  gl_sort(p_list1, compare_addresses);  /* sort the list by pointer address */
  CU_TEST(0 != gl_sorted(p_list1));
  for (i=0 ; i<9 ; ++i) {
    CU_TEST(gl_fetch(p_list1, i+2) >= gl_fetch(p_list1, i+1));
  }

  for (i=0 ; i<10 ; ++i) {              /* fill the list with unsorted data again */
    gl_store(p_list1, i+1, pint_array[9-i]);
    CU_TEST(0 == gl_sorted(p_list1));
  }

  gl_sort(p_list1, compare_ints);       /* sort the list by data value */
  CU_TEST(0 != gl_sorted(p_list1));
  for (i=0 ; i<9 ; ++i) {
    CU_TEST(*((unsigned long*)gl_fetch(p_list1, i+2)) >= *((unsigned long*)gl_fetch(p_list1, i+1)));
  }
  
  for (i=0 ; i<10 ; ++i) {              /* fill the list with unsorted data again */
    gl_store(p_list1, i+1, pint_array[9-i]);
    CU_TEST(0 == gl_sorted(p_list1));
  }
  gl_store(p_list1, 3, NULL);           /* add a couple of NULL pointers to the list */
  gl_store(p_list1, 8, NULL);

  gl_sort(p_list1, compare_addresses);  /* sort the list by pointer address (can handle NULL's) */
  CU_TEST(0 != gl_sorted(p_list1));
  for (i=0 ; i<9 ; ++i) {
    CU_TEST(gl_fetch(p_list1, i+2) >= gl_fetch(p_list1, i+1));
  }

  for (i=0 ; i<10 ; ++i) {              /* fill the list with unsorted data again */
    gl_store(p_list1, i+1, pint_array[9-i]);
    CU_TEST(0 == gl_sorted(p_list1));
  }

  CU_TEST(0 == gl_sorted(p_list1));     /* calling gl_insert_sorted() on unsorted list results in sort */
  gl_insert_sorted(p_list1, pint_array[11], compare_addresses);
  CU_TEST(11 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(gl_fetch(p_list1, i+2) >= gl_fetch(p_list1, i+1));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_iterate() */

  p_list1 = gl_create(10);              /* create a list and fill with data */
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
  }

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_iterate(NULL, mult_by_2);        /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_iterate(p_list1, NULL);          /* error if NULL func* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gl_iterate(p_list1, mult_by_2);       /* execute function on each data element */

  for (i=0 ; i<10 ; ++i) {
    CU_TEST((2*i) == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */
  for (i=0 ; i<20 ; ++i)                /* need to restore our integer array */
    *pint_array[i] = i;

  /* test gl_ptr_search(), gl_search(), gl_search_reverse */

  p_list1 = gl_create(10);              /* create an empty list */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_ptr_search(NULL, NULL, FALSE);   /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_search(NULL, NULL, compare_addresses); /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_search(p_list1, NULL, NULL);     /* error if NULL func */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_search_reverse(NULL, NULL, compare_addresses); /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_search_reverse(p_list1, NULL, NULL);   /* error if NULL func */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  CU_TEST(0 == gl_ptr_search(p_list1, NULL, FALSE));      /* search empty list */
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[9], FALSE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[0], FALSE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[5], FALSE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[10], FALSE));

  CU_TEST(0 == gl_search(p_list1, NULL, compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[9], compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[0], compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[10], compare_addresses));

  CU_TEST(0 == gl_search_reverse(p_list1, NULL, compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[9], compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[0], compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[10], compare_addresses));

  gl_append_ptr(p_list1, pint_array[0]);

  CU_TEST(0 == gl_ptr_search(p_list1, NULL, TRUE));      /* search list having 1 element*/
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[9], TRUE));
  CU_TEST(1 == gl_ptr_search(p_list1, pint_array[0], TRUE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[5], TRUE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[10], TRUE));

  CU_TEST(0 == gl_search(p_list1, NULL, compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[9], compare_addresses));
  CU_TEST(1 == gl_search(p_list1, pint_array[0], compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[10], compare_addresses));

  CU_TEST(0 == gl_search_reverse(p_list1, NULL, compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[9], compare_addresses));
  CU_TEST(1 == gl_search_reverse(p_list1, pint_array[0], compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[10], compare_addresses));

  gl_reset(p_list1);
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[9-i]);
  }

  CU_TEST(0 == gl_ptr_search(p_list1, NULL, FALSE));      /* search unsorted list */
  CU_TEST(1 == gl_ptr_search(p_list1, pint_array[9], FALSE));
  CU_TEST(10 == gl_ptr_search(p_list1, pint_array[0], FALSE));
  CU_TEST(5 == gl_ptr_search(p_list1, pint_array[5], FALSE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[10], FALSE));

  CU_TEST(0 == gl_search(p_list1, NULL, compare_addresses));
  CU_TEST(1 == gl_search(p_list1, pint_array[9], compare_addresses));
  CU_TEST(10 == gl_search(p_list1, pint_array[0], compare_addresses));
  CU_TEST(5 == gl_search(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[10], compare_addresses));

  CU_TEST(0 == gl_search_reverse(p_list1, NULL, compare_addresses));
  CU_TEST(1 == gl_search_reverse(p_list1, pint_array[9], compare_addresses));
  CU_TEST(10 == gl_search_reverse(p_list1, pint_array[0], compare_addresses));
  CU_TEST(5 == gl_search_reverse(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[10], compare_addresses));

  gl_sort(p_list1, compare_addresses);                      /* sort in increasing pointer order */
  CU_TEST(0 != gl_sorted(p_list1));

  CU_TEST(0 == gl_ptr_search(p_list1, NULL, TRUE));         /* search sorted list - correct increasing */
  CU_TEST(find_ptr_pos(p_list1, pint_array[9]) ==
      gl_ptr_search(p_list1, pint_array[9], TRUE));
  CU_TEST(find_ptr_pos(p_list1, pint_array[0]) ==
      gl_ptr_search(p_list1, pint_array[0], TRUE));
  CU_TEST(find_ptr_pos(p_list1, pint_array[5]) ==
      gl_ptr_search(p_list1, pint_array[5], TRUE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[10], TRUE));

  CU_TEST(0 == gl_ptr_search(p_list1, NULL, FALSE));       /* search sorted list - incorrect increasing */
  CU_TEST((0 == gl_ptr_search(p_list1, pint_array[9], FALSE)) ||
          (find_ptr_pos(p_list1, pint_array[9]) ==
                gl_ptr_search(p_list1, pint_array[9], TRUE)));
  CU_TEST((0 == gl_ptr_search(p_list1, pint_array[0], FALSE)) ||
          (find_ptr_pos(p_list1, pint_array[0]) ==
                gl_ptr_search(p_list1, pint_array[0], TRUE)));
  CU_TEST((0 == gl_ptr_search(p_list1, pint_array[5], FALSE)) ||
            (find_ptr_pos(p_list1, pint_array[5]) ==
                gl_ptr_search(p_list1, pint_array[5], TRUE)));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[10], FALSE));

  CU_TEST(0 == gl_search(p_list1, NULL, compare_addresses));
  CU_TEST(find_ptr_pos(p_list1, pint_array[9]) ==
      gl_search(p_list1, pint_array[9], compare_addresses));
  CU_TEST(find_ptr_pos(p_list1, pint_array[0]) ==
      gl_search(p_list1, pint_array[0], compare_addresses));
  CU_TEST(find_ptr_pos(p_list1, pint_array[5]) ==
      gl_search(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search(p_list1, pint_array[10], compare_addresses));

  CU_TEST(0 == gl_search_reverse(p_list1, NULL, compare_addresses));
  CU_TEST(find_ptr_pos(p_list1, pint_array[9]) ==
      gl_search_reverse(p_list1, pint_array[9], compare_addresses));
  CU_TEST(find_ptr_pos(p_list1, pint_array[0]) ==
      gl_search_reverse(p_list1, pint_array[0], compare_addresses));
  CU_TEST(find_ptr_pos(p_list1, pint_array[5]) ==
      gl_search_reverse(p_list1, pint_array[5], compare_addresses));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[10], compare_addresses));

  gl_sort(p_list1, compare_addresses_reverse);              /* sort in decreasing pointer order */
  CU_TEST(0 != gl_sorted(p_list1));

  CU_TEST(0 == gl_ptr_search(p_list1, NULL, FALSE));       /* search sorted list - correct increasing */
  CU_TEST(find_ptr_pos(p_list1, pint_array[9]) ==
      gl_ptr_search(p_list1, pint_array[9], FALSE));
  CU_TEST(find_ptr_pos(p_list1, pint_array[0]) ==
      gl_ptr_search(p_list1, pint_array[0], FALSE));
  CU_TEST(find_ptr_pos(p_list1, pint_array[5]) ==
      gl_ptr_search(p_list1, pint_array[5], FALSE));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[10], FALSE));

  CU_TEST(0 == gl_ptr_search(p_list1, NULL, TRUE));        /* search sorted list - incorrect increasing */
  CU_TEST((0 == gl_ptr_search(p_list1, pint_array[9], TRUE)) ||
          (find_ptr_pos(p_list1, pint_array[9]) ==
                gl_ptr_search(p_list1, pint_array[9], FALSE)));
  CU_TEST((0 == gl_ptr_search(p_list1, pint_array[0], TRUE)) ||
          (find_ptr_pos(p_list1, pint_array[0]) ==
                gl_ptr_search(p_list1, pint_array[0], FALSE)));
  CU_TEST((0 == gl_ptr_search(p_list1, pint_array[5], TRUE)) ||
          (find_ptr_pos(p_list1, pint_array[5]) ==
                gl_ptr_search(p_list1, pint_array[5], FALSE)));
  CU_TEST(0 == gl_ptr_search(p_list1, pint_array[10], TRUE));

  CU_TEST(0 == gl_search(p_list1, NULL, compare_addresses_reverse));
  CU_TEST(find_ptr_pos(p_list1, pint_array[9]) ==
      gl_search(p_list1, pint_array[9], compare_addresses_reverse));
  CU_TEST(find_ptr_pos(p_list1, pint_array[0]) ==
      gl_search(p_list1, pint_array[0], compare_addresses_reverse));
  CU_TEST(find_ptr_pos(p_list1, pint_array[5]) ==
      gl_search(p_list1, pint_array[5], compare_addresses_reverse));
  CU_TEST(0 == gl_search(p_list1, pint_array[10], compare_addresses_reverse));

  CU_TEST(0 == gl_search_reverse(p_list1, NULL, compare_addresses_reverse));
  CU_TEST(find_ptr_pos(p_list1, pint_array[9]) ==
      gl_search_reverse(p_list1, pint_array[9], compare_addresses_reverse));
  CU_TEST(find_ptr_pos(p_list1, pint_array[0]) ==
      gl_search_reverse(p_list1, pint_array[0], compare_addresses_reverse));
  CU_TEST(find_ptr_pos(p_list1, pint_array[5]) ==
      gl_search_reverse(p_list1, pint_array[5], compare_addresses_reverse));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[10], compare_addresses_reverse));

  gl_sort(p_list1, compare_ints);                          /* sort by pointed-to integer */
  CU_TEST(0 != gl_sorted(p_list1));

  CU_TEST(find_int_pos(p_list1, *pint_array[9]) ==
      gl_search(p_list1, pint_array[9], compare_ints));
  CU_TEST(find_int_pos(p_list1, *pint_array[0]) ==
      gl_search(p_list1, pint_array[0], compare_ints));
  CU_TEST(find_int_pos(p_list1, *pint_array[5]) ==
      gl_search(p_list1, pint_array[5], compare_ints));
  CU_TEST(0 == gl_search(p_list1, pint_array[10], compare_ints));

  CU_TEST(find_int_pos(p_list1, *pint_array[9]) ==
      gl_search_reverse(p_list1, pint_array[9], compare_ints));
  CU_TEST(find_int_pos(p_list1, *pint_array[0]) ==
      gl_search_reverse(p_list1, pint_array[0], compare_ints));
  CU_TEST(find_int_pos(p_list1, *pint_array[5]) ==
      gl_search_reverse(p_list1, pint_array[5], compare_ints));
  CU_TEST(0 == gl_search_reverse(p_list1, pint_array[10], compare_ints));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_empty() */

  p_list1 = gl_create(10);              /* create an empty list */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_empty(NULL);                     /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  CU_TEST(TRUE == gl_empty(p_list1));
  gl_append_ptr(p_list1, pint_array[0]);
  CU_TEST(FALSE == gl_empty(p_list1));
  gl_reset(p_list1);
  CU_TEST(TRUE == gl_empty(p_list1));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_unique_list() */

  CU_TEST(TRUE == gl_unique_list(NULL));
  p_list1 = gl_create(10);              /* create an empty list */
  CU_TEST(TRUE == gl_unique_list(p_list1));
  gl_append_ptr(p_list1, pint_array[0]);
  CU_TEST(TRUE == gl_unique_list(p_list1));
  gl_append_ptr(p_list1, pint_array[0]);
  CU_TEST(FALSE == gl_unique_list(p_list1));
  gl_reset(p_list1);
  CU_TEST(TRUE == gl_unique_list(p_list1));
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
  }
  CU_TEST(TRUE == gl_unique_list(p_list1));
  gl_append_ptr(p_list1, pint_array[5]);
  CU_TEST(FALSE == gl_unique_list(p_list1));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_delete() */

  p_list1 = gl_create(10);              /* create and fill a list */
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
  }

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_delete(NULL, 1, 0);              /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_delete(p_list1, 11, 0);          /* error if pos > gl_length(p_list1) */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gl_delete(p_list1, 0, 0);             /* pos == 0 is tolerated */
  CU_TEST(10 == gl_length(p_list1));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all pointers should still be active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  gl_delete(p_list1, 1, 0);             /* deleting 1st pos in list, keep data */
  CU_TEST(9 == gl_length(p_list1));
  for (i=1 ; i<10 ; ++i) {
    CU_TEST(i == *((unsigned long*)gl_fetch(p_list1, i)));
  }
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all pointers should still be active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  gl_delete(p_list1, 9, 0);             /* deleting last pos in list, keep data */
  CU_TEST(8 == gl_length(p_list1));
  for (i=1 ; i<9 ; ++i) {
    CU_TEST(i == *((unsigned long*)gl_fetch(p_list1, i)));
  }
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all pointers should still be active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  gl_delete(p_list1, 4, 0);             /* delete something in the middle, keep data */
  CU_TEST(7 == gl_length(p_list1));
  CU_TEST(1 == *((unsigned long*)gl_fetch(p_list1, 1)));
  CU_TEST(2 == *((unsigned long*)gl_fetch(p_list1, 2)));
  CU_TEST(3 == *((unsigned long*)gl_fetch(p_list1, 3)));
  CU_TEST(5 == *((unsigned long*)gl_fetch(p_list1, 4)));
  CU_TEST(6 == *((unsigned long*)gl_fetch(p_list1, 5)));
  CU_TEST(7 == *((unsigned long*)gl_fetch(p_list1, 6)));
  CU_TEST(8 == *((unsigned long*)gl_fetch(p_list1, 7)));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all pointers should still be active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  gl_delete(p_list1, 1, 1);             /* deleting 1st pos in list, dispose data */
  CU_TEST(6 == gl_length(p_list1));
  CU_TEST(2 == *((unsigned long*)gl_fetch(p_list1, 1)));
  CU_TEST(3 == *((unsigned long*)gl_fetch(p_list1, 2)));
  CU_TEST(5 == *((unsigned long*)gl_fetch(p_list1, 3)));
  CU_TEST(6 == *((unsigned long*)gl_fetch(p_list1, 4)));
  CU_TEST(7 == *((unsigned long*)gl_fetch(p_list1, 5)));
  CU_TEST(8 == *((unsigned long*)gl_fetch(p_list1, 6)));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all other pointers should still be active */
    if (i != 1) {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  gl_delete(p_list1, 6, 1);             /* deleting last pos in list, dispose data */
  CU_TEST(5 == gl_length(p_list1));
  CU_TEST(2 == *((unsigned long*)gl_fetch(p_list1, 1)));
  CU_TEST(3 == *((unsigned long*)gl_fetch(p_list1, 2)));
  CU_TEST(5 == *((unsigned long*)gl_fetch(p_list1, 3)));
  CU_TEST(6 == *((unsigned long*)gl_fetch(p_list1, 4)));
  CU_TEST(7 == *((unsigned long*)gl_fetch(p_list1, 5)));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all other pointers should still be active */
    if ((i != 1) && (i != 8)) {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  gl_delete(p_list1, 3, 1);             /* delete something in the middle, dispose data */
  CU_TEST(4 == gl_length(p_list1));
  CU_TEST(2 == *((unsigned long*)gl_fetch(p_list1, 1)));
  CU_TEST(3 == *((unsigned long*)gl_fetch(p_list1, 2)));
  CU_TEST(6 == *((unsigned long*)gl_fetch(p_list1, 3)));
  CU_TEST(7 == *((unsigned long*)gl_fetch(p_list1, 4)));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all other pointers should still be active */
    if ((i != 1) && (i != 8) && (i != 5)) {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  for (i=0 ; i<4 ; ++i) {
    gl_delete(p_list1, 1, 0);           /* delete rest of list members, keeping data */
  }
  CU_TEST(0 == gl_length(p_list1));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all other pointers should still be active */
    if ((i != 1) && (i != 8) && (i != 5)) {
      CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
    else {
      CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
    }
  }
#endif

  pint_array[1] = (unsigned long*)ascmalloc(sizeof(unsigned long));
  *pint_array[1] = 1;
  pint_array[5] = (unsigned long*)ascmalloc(sizeof(unsigned long));
  *pint_array[5] = 5;
  pint_array[8] = (unsigned long*)ascmalloc(sizeof(unsigned long));
  *pint_array[8] = 8;

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_reverse() */

  gl_reverse(NULL);                     /* reversing a NULL list is ok, although no effect */

  p_list1 = gl_create(20);              /* create an empty list */

  gl_reverse(p_list1);                  /* reversing an empty list is ok, although no effect */
  CU_TEST(0 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));

  gl_append_ptr(p_list1, pint_array[10]);

  gl_reverse(p_list1);                  /* reversing a list with 1 element is ok, although no effect */
  CU_TEST(1 == gl_length(p_list1));
  CU_TEST(1 == gl_length(p_list1));
  CU_TEST(10 == *((unsigned long*)gl_fetch(p_list1, 1)));
  CU_TEST(0 != gl_sorted(p_list1));

  gl_append_ptr(p_list1, pint_array[8]);

  gl_reverse(p_list1);                  /* reversing a list with 2 elements is ok */
  CU_TEST(2 == gl_length(p_list1));
  CU_TEST(8 == *((unsigned long*)gl_fetch(p_list1, 1)));
  CU_TEST(10 == *((unsigned long*)gl_fetch(p_list1, 2)));
  CU_TEST(0 == gl_sorted(p_list1));

  gl_reset(p_list1);

  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[9-i]);
  }

  gl_reverse(p_list1);                  /* reversing a longer unsorted list */
  CU_TEST(10 == gl_length(p_list1));
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(i == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }
  CU_TEST(0 == gl_sorted(p_list1));

  gl_sort(p_list1, compare_ints);

  gl_reverse(p_list1);                  /* reversing a longer sorted list */
  CU_TEST(10 == gl_length(p_list1));
  for (i=0 ; i<10 ; ++i) {
    CU_TEST((9-i) == *((unsigned long*)gl_fetch(p_list1, i+1)));
  }
  CU_TEST(0 != gl_sorted(p_list1));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_reset() */

  p_list1 = gl_create(10);              /* create and fill a list */
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
  }

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_reset(NULL);                     /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gl_reset(p_list1);                    /* reset an unsorted, expandable list */
  CU_TEST(0 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(0 != gl_expandable(p_list1));
#ifdef MALLOC_DEBUG
  for (i=0 ; i<20 ; ++i) {              /* all pointers should still be active */
    CU_TEST(2 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(unsigned long)));
  }
#endif

  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
  }
  gl_set_sorted(p_list1, TRUE);
  gl_set_expandable(p_list1, FALSE);    /* set list non-expandable */

  gl_reset(p_list1);                    /* reset a sorted, unexpandable list */
  CU_TEST(0 == gl_length(p_list1));
  CU_TEST(0 != gl_sorted(p_list1));
  CU_TEST(0 != gl_expandable(p_list1));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_copy() */

  p_list1 = gl_create(10);              /* create and fill a list */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_copy(NULL);                      /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  p_list2= gl_copy(p_list1);            /* copying an empty list should be ok */
  CU_TEST(0 == gl_length(p_list2));
  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_destroy(p_list2);

  gl_append_ptr(p_list1, pint_array[5]);

  p_list2= gl_copy(p_list1);            /* copying a list having 1 element should be ok */
  CU_TEST(1 == gl_length(p_list2));
  CU_TEST(5 == *((unsigned long*)gl_fetch(p_list1, 1)));
  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_destroy(p_list2);

  gl_reset(p_list1);
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
  }

  p_list2= gl_copy(p_list1);           /* copying a longer, unsorted list */
  CU_TEST(10 == gl_length(p_list2));
  CU_TEST(0 == gl_sorted(p_list2));
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(*((unsigned long*)gl_fetch(p_list1, i+1)) == *((unsigned long*)gl_fetch(p_list2, i+1)));
  }
  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_destroy(p_list2);

  gl_sort(p_list1, compare_addresses_reverse);

  p_list2= gl_copy(p_list1);           /* copying a longer, sorted list */
  CU_TEST(10 == gl_length(p_list2));
  CU_TEST(0 != gl_sorted(p_list2));
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(*((unsigned long*)gl_fetch(p_list1, i+1)) == *((unsigned long*)gl_fetch(p_list2, i+1)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_free_and_destroy(p_list2);         /* shared data should be destroyed also */
#ifdef MALLOC_DEBUG
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(0 == AllocatedMemory(gl_fetch(p_list1, i+1), sizeof(unsigned long)));
  }
#endif

  for (i=0 ; i<10 ; ++i) {
    pint_array[i] = (unsigned long*)ascmalloc(sizeof(unsigned long));
    *pint_array[i] = i;
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_concat() */

  p_list1 = gl_create(10);              /* create 2 empty lists */
  p_list2 = gl_create(10);

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    p_list3 = gl_concat(NULL, p_list2); /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    p_list3 = gl_concat(p_list1, NULL);   /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  p_list3 = gl_concat(p_list1, p_list2);  /* concatenating empty list should be ok */
  CU_TEST(0 == gl_length(p_list3));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list3;
  }
  gl_destroy(p_list3);

  gl_append_ptr(p_list1, pint_array[3]);

  p_list3 = gl_concat(p_list1, p_list2);  /* concatenating a 1-element list with empty list */
  CU_TEST(1 == gl_length(p_list3));
  CU_TEST(*pint_array[3] == *((unsigned long*)gl_fetch(p_list3, 1)));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list3;
  }
  gl_destroy(p_list3);

  p_list3 = gl_concat(p_list2, p_list1);  /* concatenating empty list with a 1-element list */
  CU_TEST(1 == gl_length(p_list3));
  CU_TEST(*pint_array[3] == *((unsigned long*)gl_fetch(p_list3, 1)));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list3;
  }
  gl_destroy(p_list3);

  p_list3 = gl_concat(p_list1, p_list1);  /* concatenating two 1-element lists */
  CU_TEST(2 == gl_length(p_list3));
  CU_TEST(*pint_array[3] == *((unsigned long*)gl_fetch(p_list3, 1)));
  CU_TEST(*pint_array[3] == *((unsigned long*)gl_fetch(p_list3, 2)));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list3;
  }
  gl_destroy(p_list3);

  gl_reset(p_list1);
  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);
  }

  p_list3 = gl_concat(p_list1, p_list2);  /* concatenating a list with an empty list */
  CU_TEST(10 == gl_length(p_list3));
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list3, i+1)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list3;
  }
  gl_destroy(p_list3);

  p_list3 = gl_concat(p_list2, p_list1);  /* concatenating empty list with a 1-element list */
  CU_TEST(10 == gl_length(p_list3));
  for (i=0 ; i<10 ; ++i) {
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list3, i+1)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list3;
  }
  gl_destroy(p_list3);

  for (i=0 ; i<10 ; ++i) {
    gl_append_ptr(p_list2, pint_array[i+10]);
  }

  p_list3 = gl_concat(p_list1, p_list2);  /* concatenating two lists */
  CU_TEST(20 == gl_length(p_list3));
  for (i=0 ; i<20 ; ++i) {
    CU_TEST(*pint_array[i] == *((unsigned long*)gl_fetch(p_list3, i+1)));
  }

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */
  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_destroy(p_list2);
  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list3;
  }
  gl_destroy(p_list3);

  /* test gl_compare_ptrs() */

  p_list1 = gl_create(10);              /* create 2 empty lists */
  p_list2 = gl_create(10);

  CU_TEST(0 == gl_compare_ptrs(p_list1, p_list2));   /* compare 2 empty lists */
  CU_TEST(0 == gl_compare_ptrs(p_list1, p_list1));   /* compare empty list with itself */

  gl_append_ptr(p_list1, pint_array[9]);

  CU_TEST(0 < gl_compare_ptrs(p_list1, p_list2));    /* compare 1-element with empty list */
  CU_TEST(0 > gl_compare_ptrs(p_list2, p_list1));    /* compare empty with 1-element list */
  CU_TEST(0 == gl_compare_ptrs(p_list1, p_list1));   /* compare 1-element list with itself */

  gl_append_ptr(p_list2, pint_array[9]);

  CU_TEST(0 == gl_compare_ptrs(p_list1, p_list2));   /* compare equal 1-element lists */
  CU_TEST(0 == gl_compare_ptrs(p_list2, p_list1));

  gl_reset(p_list2);
  gl_append_ptr(p_list2, pint_array[3]);

  if (pint_array[9] > pint_array[3]) {
    CU_TEST(0 < gl_compare_ptrs(p_list1, p_list2));
  }
  else {
    CU_TEST(0 > gl_compare_ptrs(p_list1, p_list2));
  }
  
  gl_reset(p_list1);
  gl_reset(p_list2);

  for (i=0 ; i<20 ; ++i) {
    gl_append_ptr(p_list1, pint_array[i]);                           
    gl_append_ptr(p_list2, pint_array[i]);
  }
  
  CU_TEST(0 == gl_compare_ptrs(p_list1, p_list2));   /* compare equal lists */
  CU_TEST(0 == gl_compare_ptrs(p_list2, p_list1));   /* compare equal lists */

  gl_sort(p_list1, compare_addresses);
  gl_sort(p_list2, compare_addresses_reverse);

  CU_TEST(0 > gl_compare_ptrs(p_list1, p_list2));   /* compare equal lists */
  CU_TEST(0 < gl_compare_ptrs(p_list2, p_list1));   /* compare equal lists */

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */
  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list2;
  }
  gl_destroy(p_list2);

  /* test gl_expandable(), gl_set_expandable() */

  p_list1 = gl_create(10);              /* create an empty list */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    (void)gl_expandable(NULL);          /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_set_expandable(NULL, TRUE);      /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  CU_TEST(0 != gl_expandable(p_list1)); /* make sure functions work */
  gl_set_expandable(p_list1, FALSE);
  CU_TEST(0 == gl_expandable(p_list1));
  gl_set_expandable(p_list1, TRUE);
  CU_TEST(0 != gl_expandable(p_list1));
  gl_set_expandable(p_list1, FALSE);
  CU_TEST(0 == gl_expandable(p_list1));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* test gl_fetchaddr() */

  p_list1 = gl_create(10);              /* create an empty list */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);               /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_fetchaddr(p_list1, 0);           /* error if pos out of range (< 1) */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_fetchaddr(p_list1, 11);          /* error if pos out of range (> gl_length(list) */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_fetchaddr(NULL, 1);              /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);              /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  gl_append_ptr(p_list1, pint_array[10]);

  CU_TEST(&p_list1->data[0] == gl_fetchaddr(p_list1, 1));

  gl_append_ptr(p_list1, pint_array[10]);
  gl_append_ptr(p_list1, pint_array[10]);

  CU_TEST(&p_list1->data[0] == gl_fetchaddr(p_list1, 1));
  CU_TEST(&p_list1->data[1] == gl_fetchaddr(p_list1, 2));
  CU_TEST(&p_list1->data[2] == gl_fetchaddr(p_list1, 3));

  if (n_used_lists < MAX_LISTS_TO_TRACK) {
    p_used_lists[n_used_lists++] = p_list1;
  }
  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* gl_report_pool - not tested */

  /* clean up and exit */
  for (i=0 ; i<20 ; ++i)
    ascfree(pint_array[i]);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<(MIN(n_used_lists, MAX_LISTS_TO_TRACK)) ; ++i) {
    CU_TEST(0 != AllocatedMemory((VOIDPTR)p_used_lists[i], capacity * sizeof(VOIDPTR)));
  }
#endif
  gl_destroy_pool();
#ifdef MALLOC_DEBUG
  for (i=0 ; i<(MIN(n_used_lists, MAX_LISTS_TO_TRACK)) ; ++i) {
    CU_TEST(0 == AllocatedMemory((VOIDPTR)p_used_lists[i], capacity * sizeof(VOIDPTR)));
  }
#endif

#ifdef LISTUSESPOOL
  CU_TEST(FALSE == gl_pool_initialized());
#else
  CU_TEST(TRUE == gl_pool_initialized());
#endif

  if (TRUE == lists_were_active) {        /* if list was already initialized, restore it */
    gl_init_pool();
  }

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo list_test_list[] = {
  {"test_list", test_list},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_list", NULL, NULL, list_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_list(void)
{
  return CU_register_suites(suites);
}
