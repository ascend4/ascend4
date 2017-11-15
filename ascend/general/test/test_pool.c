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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/pool.h>
#include <ascend/general/list.h>

#include <test/common.h>

#define STR_LEN 100


static void test_pool(void){
  char str1[STR_LEN];
  char str2[STR_LEN];
  char str3[STR_LEN];
  int str1_bad;
  int str2_bad;
  unsigned word;

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

 /* test pool_move_cast() */

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  str1_bad = FALSE;                                   /* test initial condition */
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  pool_move_cast(str2, str1, STR_LEN/2);               /* copy part of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str2_bad && (str2[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  pool_move_cast(str2, str1, STR_LEN);                 /* copy all of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  pool_move_cast(str1, str1+10, 20);                   /* copy overlapping memory block */

  str1_bad = FALSE;
  for (i=0 ; i<20 ; ++i) {
    if (!str1_bad && (str1[i] != '~')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '~' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=20 ; i<30 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  pool_move_cast(str1+10, str1, 20);                   /* copy overlapping memory block */

  str1_bad = FALSE;
  for (i=0 ; i<10 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '~' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=10 ; i<30 ; ++i) {
    if (!str1_bad && (str1[i] != '|')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 and str2 check out.");

  SNPRINTF(str1, STR_LEN-1, "This is yet another dumb string");
  pool_move_cast(str1, str1, strlen(str1));            /* to == from */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

  pool_move_cast(str2, str1, 0);                       /* n = 0 */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

 /* test pool_copy_cast() */

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  str1_bad = FALSE;                                   /* test initial condition */
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  pool_copy_cast(str2, str1, STR_LEN/2);               /* copy part of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str2_bad && (str2[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  pool_copy_cast(str2, str1, STR_LEN);                 /* copy all of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      SNPRINTF(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  SNPRINTF(str1, STR_LEN-1, "This is yet another dumb string");
  pool_copy_cast(str1, str1, strlen(str1));            /* to == from */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

  pool_copy_cast(str2, str1, 0);                       /* n = 0 */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

  /* test pool_repl_byte_cast() */

  memset(str1, '=', STR_LEN);

  pool_repl_byte_cast(str1, '#', STR_LEN/2);           /* replace part of a memory block */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '#')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after pool_repl_byte_cast().");

  memset(str1, '+', STR_LEN);

  pool_zero_byte_cast(str1, '=', 0);                   /* 0 bytes processed */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '+')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '+' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after pool_zero_byte_cast().");

  /* test pool_zero_byte_cast() */

  memset(str1, '=', STR_LEN);

  pool_zero_byte_cast(str1, '#', STR_LEN/2);           /* zero part of a memory block */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after pool_zero_byte_cast().");

  memset(str1, '+', STR_LEN);

  pool_zero_byte_cast(str1, '=', 0);                   /* 0 bytes processed */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '+')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '+' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after pool_zero_byte_cast().");

  /* test pool_repl_word_cast() */

  word = 1234567890;

  memset(str1, '=', STR_LEN);

  pool_repl_word_cast(str1, word, 1);                  /* copy 1 word */

  CU_TEST(*((unsigned *)str1) == word);

  str1_bad = FALSE;
  for (i=sizeof(unsigned) ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after pool_repl_word_cast().");

  memset(str1, '~', STR_LEN);

  pool_repl_word_cast(str1, word, 10);                  /* copy 5 words */

  for (i=0 ; i<10 ; ++i) {
    CU_TEST(*(((unsigned *)str1)+1) == word);
  }

  str1_bad = FALSE;
  for (i=10*sizeof(unsigned) ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '~')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '~' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after pool_repl_word_cast().");

  memset(str1, '?', STR_LEN);

  pool_repl_word_cast(str1, word, 0);                  /* copy 0 words */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '?')) {
      SNPRINTF(str3, STR_LEN-1, "str1[%d] != '?' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after pool_repl_word_cast().");

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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  pool_clear_store(ps);                               /* clear the pool store */
  pool_get_stats(&stats, ps);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(asc_intptr_t) == stats.elt_size);
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

#define TESTS(T) \
	T(pool)

REGISTER_TESTS_SIMPLE(general_pool, TESTS)

