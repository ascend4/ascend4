/*
 *  Unit test functions for ASCEND: general/pool.c
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

#include <stdio.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/pool.h>
#include <general/list.h>
#include "CUnit/CUnit.h"

static void test_pool(void)
{
  pool_store_t ps;
  struct pool_statistics stats;
  int i;
  int *pint_array[100];
  double *pdoub_array[100];
  char *pstr_array[100];
  struct gl_list_t *pint_list;
#ifdef MALLOC_DEBUG
  char str_error_msg[100];
#endif
  int i_initialized_lists = FALSE;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifndef MALLOC_DEBUG
  CU_FAIL("test_pool() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  /* test pool_create_store(), pool_destroy_store() */

  ps = pool_create_store(0, 10, sizeof(int), 10, 10); /* create a store - length < 1 (error) */
  CU_TEST(NULL == ps);
  if (NULL != ps)
    pool_destroy_store(ps);

  ps = pool_create_store(10, 0, sizeof(int), 10, 10); /* create a store - width < 1 (error) */
  CU_TEST(NULL == ps);
  if (NULL != ps)
    pool_destroy_store(ps);

  ps = pool_create_store(10, 10, 0, 10, 10);          /* create a store - eltsze < 1 (ok) */
  CU_TEST(NULL != ps);
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(0 == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ps)
    pool_destroy_store(ps);

  ps = pool_create_store(10, 10, sizeof(int), 10, 10);/* create a store for ints */
  CU_TEST(NULL != ps);
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<100 ; ++i) {                           /* check out some elements */
    pint_array[i] = (int*)pool_get_element(ps);
#ifdef MALLOC_DEBUG
    CU_TEST(0 != AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
#endif
  }

  if (NULL != ps)                                     /* destroy the store */
    pool_destroy_store(ps);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
  }
#endif

  /* test pool_get_element(), pool_free_element() */

  ps = pool_create_store(10, 10, sizeof(int), 10, 10);/* create a store for ints */
  CU_TEST(NULL != ps);

  CU_TEST(NULL == pool_get_element(NULL));            /* requesting element of a NULL store has no effect */
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pint_array[i] = (int*)pool_get_element(ps);
    *pint_array[i] = i;
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(50 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=50 ; i<100 ; ++i) {                          /* check out & assign more elements */
    pint_array[i] = (int*)pool_get_element(ps);
    *pint_array[i] = i;
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<100 ; ++i) {
    CU_TEST(i == *pint_array[i]);                     /* make sure element values are ok */
  }

  pool_free_element(ps, NULL);                        /* returning a NULL element has no effect */
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<50 ; ++i) {                            /* return some elements */
    pool_free_element(ps, pint_array[i]);
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(50 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);
#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* nothing should be deallocated yet */
    CU_TEST(0 != AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
  }
#endif

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pint_array[i] = (int*)pool_get_element(ps);
    *pint_array[i] = i;
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ps)                                     /* destroy the store */
    pool_destroy_store(ps);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
  }
#endif

  ps = pool_create_store(10, 10, sizeof(double), 10, 10);/* create a store for doubles */
  CU_TEST(NULL != ps);

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pdoub_array[i] = (double*)pool_get_element(ps);
    *pdoub_array[i] = (double)i;
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(50 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(double) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=50 ; i<100 ; ++i) {                          /* check out & assign more elements */
    pdoub_array[i] = (double*)pool_get_element(ps);
    *pdoub_array[i] = (double)i;
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(double) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<100 ; ++i) {
    CU_ASSERT_DOUBLE_EQUAL((double)i, *pdoub_array[i], 0.001);  /* make sure element values are ok */
  }

  for (i=0 ; i<50 ; ++i) {                            /* return some elements */
    pool_free_element(ps, pdoub_array[i]);
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(50 == stats.elt_onlist);
  CU_TEST(sizeof(double) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);
#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* nothing should be deallocated yet */
    CU_TEST(0 != AllocatedMemory((VOIDPTR)pdoub_array[i], sizeof(VOIDPTR)));
  }
#endif

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pdoub_array[i] = (double*)pool_get_element(ps);
    *pdoub_array[i] = (double)i;
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(double) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ps)                                     /* destroy the store */
    pool_destroy_store(ps);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pdoub_array[i], sizeof(VOIDPTR)));
  }
#endif

  ps = pool_create_store(10, 10, 12*sizeof(char), 10, 10);/* create a store for strings */
  CU_TEST(NULL != ps);

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pstr_array[i] = (char*)pool_get_element(ps);
    strcpy(pstr_array[i], "01234567890");
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(50 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST((12*sizeof(char) + (12*sizeof(char)%sizeof(void*))) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=50 ; i<100 ; ++i) {                          /* check out & assign more elements */
    pstr_array[i] = (char*)pool_get_element(ps);
    strcpy(pstr_array[i], "01234567890");
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST((12*sizeof(char) + (12*sizeof(char)%sizeof(void*))) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<100 ; ++i) {
    CU_ASSERT_STRING_EQUAL("01234567890", pstr_array[i]);  /* make sure element values are ok */
  }

  for (i=0 ; i<50 ; ++i) {                            /* return some elements */
    pool_free_element(ps, pstr_array[i]);
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(50 == stats.elt_onlist);
  CU_TEST((12*sizeof(char) + (12*sizeof(char)%sizeof(void*))) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);
#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* nothing should be deallocated yet */
    CU_TEST(0 != AllocatedMemory((VOIDPTR)pstr_array[i], sizeof(VOIDPTR)));
  }
#endif

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pstr_array[i] = (char*)pool_get_element(ps);
    strcpy(pstr_array[i], "01234567890");
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST((12*sizeof(char) + (12*sizeof(char)%sizeof(void*))) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ps)                                     /* destroy the store */
    pool_destroy_store(ps);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    if (0 != AllocatedMemory((VOIDPTR)pstr_array[i], sizeof(VOIDPTR))) {
      snprintf(str_error_msg, 100, "%s%d%s", "pstr_array[", i, "] not deallocated.");
      CU_FAIL(str_error_msg);
    }
  }
#endif

  /* test pool_clear_store() */

  ps = pool_create_store(10, 10, sizeof(int), 10, 10);/* create a store for ints */
  CU_TEST(NULL != ps);

  /* set up pooling & recycling */
  if (FALSE == gl_pool_initialized()) {
    gl_init();
    gl_init_pool();
    i_initialized_lists = TRUE;
  }
  pint_list = gl_create(100);

  for (i=0 ; i<100 ; ++i) {                           /* check out the elements */
    gl_append_ptr(pint_list, pool_get_element(ps));
  }
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  pool_clear_store(ps);                               /* clear the pool store */
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  CU_TEST(0 == gl_sorted(pint_list));                 /* make sure list isn't sorted */
  for (i=0 ; i<100 ; ++i) {                           /* check out the elements again & confirm they are the same */
    pint_array[i] = (int*)pool_get_element(ps);
    CU_TEST(0 != gl_ptr_search(pint_list, (VOIDPTR)pint_array[i], FALSE));
#ifdef MALLOC_DEBUG
    CU_TEST(0 != AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
#endif
  }

  gl_destroy(pint_list);
  if (TRUE == i_initialized_lists) {
    gl_destroy_pool();
  }

  if (NULL != ps)                                     /* destroy the store */
    pool_destroy_store(ps);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
  }
#endif

  CU_TEST(prior_meminuse == ascmeminuse());           /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo list_test_pool[] = {
  {"test_pool", test_pool},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_pool", NULL, NULL, list_test_pool},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_pool(void)
{
  return CU_register_suites(suites);
}
