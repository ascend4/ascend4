/*
 *  Unit test functions for ASCEND: utilities/mem.c
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
#include "utilities/ascConfig.h"
#include "utilities/AscMalloc.h"
#include "utilities/mem.h"
#include "general/list.h"
#include "CUnit/CUnit.h"
#include "test_mem.h"

#define STR_LEN 100

static void test_mem(void)
{
  char str1[STR_LEN];
  char str2[STR_LEN];
  char str3[STR_LEN];
  int str1_bad;
  int str2_bad;
  unsigned word;

  mem_store_t ms;
  struct mem_statistics stats;
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
  CU_FAIL("test_mem() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

 /* test mem_move_cast() */

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  str1_bad = FALSE;                                   /* test initial condition */
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  mem_move_cast(str2, str1, STR_LEN/2);               /* copy part of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  mem_move_cast(str2, str1, STR_LEN);                 /* copy all of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  mem_move_cast(str1, str1+10, 20);                   /* copy overlapping memory block */

  str1_bad = FALSE;
  for (i=0 ; i<20 ; ++i) {
    if (!str1_bad && (str1[i] != '~')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '~' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=20 ; i<30 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  mem_move_cast(str1+10, str1, 20);                   /* copy overlapping memory block */

  str1_bad = FALSE;
  for (i=0 ; i<10 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '~' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=10 ; i<30 ; ++i) {
    if (!str1_bad && (str1[i] != '|')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 and str2 check out.");

  snprintf(str1, STR_LEN-1, "This is yet another dumb string");
  mem_move_cast(str1, str1, strlen(str1));            /* to == from */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

  mem_move_cast(str2, str1, 0);                       /* n = 0 */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

 /* test mem_copy_cast() */

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  str1_bad = FALSE;                                   /* test initial condition */
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  mem_copy_cast(str2, str1, STR_LEN/2);               /* copy part of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  mem_copy_cast(str2, str1, STR_LEN);                 /* copy all of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%d] != '*' in test_mem().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  snprintf(str1, STR_LEN-1, "This is yet another dumb string");
  mem_copy_cast(str1, str1, strlen(str1));            /* to == from */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

  mem_copy_cast(str2, str1, 0);                       /* n = 0 */
  CU_TEST(0 == strcmp(str1, "This is yet another dumb string"));

  /* test mem_repl_byte_cast() */

  memset(str1, '=', STR_LEN);

  mem_repl_byte_cast(str1, '#', STR_LEN/2);           /* replace part of a memory block */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '#')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after mem_repl_byte_cast().");

  memset(str1, '+', STR_LEN);

  mem_zero_byte_cast(str1, '=', 0);                   /* 0 bytes processed */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '+')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '+' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after mem_zero_byte_cast().");

  /* test mem_zero_byte_cast() */

  memset(str1, '=', STR_LEN);

  mem_zero_byte_cast(str1, '#', STR_LEN/2);           /* zero part of a memory block */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '\\0' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after mem_zero_byte_cast().");

  memset(str1, '+', STR_LEN);

  mem_zero_byte_cast(str1, '=', 0);                   /* 0 bytes processed */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '+')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '+' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after mem_zero_byte_cast().");

  /* test mem_repl_word_cast() */

  word = 1234567890;

  memset(str1, '=', STR_LEN);

  mem_repl_word_cast(str1, word, 1);                  /* copy 1 word */

  CU_TEST(*((unsigned *)str1) == word);

  str1_bad = FALSE;
  for (i=sizeof(unsigned) ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '=' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after mem_repl_word_cast().");

  memset(str1, '~', STR_LEN);

  mem_repl_word_cast(str1, word, 10);                  /* copy 5 words */

  for (i=0 ; i<10 ; ++i) {
    CU_TEST(*(((unsigned *)str1)+1) == word);
  }

  str1_bad = FALSE;
  for (i=10*sizeof(unsigned) ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '~')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '~' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after mem_repl_word_cast().");

  memset(str1, '?', STR_LEN);

  mem_repl_word_cast(str1, word, 0);                  /* copy 0 words */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '?')) {
      snprintf(str3, STR_LEN-1, "str1[%d] != '?' in test_mem().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after mem_repl_word_cast().");

  /* test mem_set_byte(), mem_get_byte() */

  memset(str1, '\0', STR_LEN);
  CU_TEST('\0' == mem_get_byte((long)str1));

  mem_set_byte((long)str1, 0xff);
  CU_TEST(0xff == mem_get_byte((long)str1));

  mem_set_byte((long)str1, '\t');
  CU_TEST('\t' == mem_get_byte((long)str1));

  mem_set_byte((long)str1, '\0');
  CU_TEST('\0' == mem_get_byte((long)str1));

  /* test mem_set_int(), mem_get_int() */

  memset(str1, '\0', STR_LEN);
  CU_TEST(0 == mem_get_int((long)str1));

  mem_set_int((long)str1, 0xff72);
  CU_TEST(0xff72 == mem_get_int((long)str1));

  mem_set_int((long)str1, '\t');
  CU_TEST('\t' == mem_get_int((long)str1));

  mem_set_int((long)str1, 0);
  CU_TEST(0 == mem_get_int((long)str1));

  /* test mem_set_long(), mem_get_long() */

  memset(str1, '\0', STR_LEN);
  CU_TEST(0 == mem_get_long((long)str1));

  mem_set_long((long)str1, 0xff72);
  CU_TEST(0xff72 == mem_get_long((long)str1));

  mem_set_long((long)str1, '\t');
  CU_TEST('\t' == mem_get_long((long)str1));

  mem_set_long((long)str1, 0);
  CU_TEST(0 == mem_get_long((long)str1));

  /* test mem_set_float(), mem_get_float() */

  memset(str1, '\0', STR_LEN);
  CU_ASSERT_DOUBLE_EQUAL(0.0, mem_get_float((long)str1), 0.001);

  mem_set_float((long)str1, 1.501436);
  CU_ASSERT_DOUBLE_EQUAL(1.501436, mem_get_float((long)str1), 0.00001);

  mem_set_float((long)str1, 9.3e-10);
  CU_ASSERT_DOUBLE_EQUAL(9.3e-10, mem_get_float((long)str1), 0.001);

  mem_set_float((long)str1, 0.0);
  CU_ASSERT_DOUBLE_EQUAL(0.0, mem_get_float((long)str1), 0.001);

  /* test mem_set_double(), mem_get_double() */

  memset(str1, '\0', STR_LEN);
  CU_ASSERT_DOUBLE_EQUAL(0.0, mem_get_double((long)str1), 0.001);

  mem_set_double((long)str1, 1.501436872625);
  CU_ASSERT_DOUBLE_EQUAL(1.501436872625, mem_get_double((long)str1), 0.000000000001);

  mem_set_double((long)str1, 9.32626e-154);
  CU_ASSERT_DOUBLE_EQUAL(9.32626e-154, mem_get_double((long)str1), 0.00001);

  mem_set_double((long)str1, 0.0);
  CU_ASSERT_DOUBLE_EQUAL(0.0, mem_get_double((long)str1), 0.001);

  /* test mem_set_unsigned(), mem_get_unsigned() */

  memset(str1, '\0', STR_LEN);
  CU_TEST(0 == mem_get_unsigned((long)str1));

  mem_set_unsigned((long)str1, 0xff72);
  CU_TEST(0xff72 == mem_get_unsigned((long)str1));

  mem_set_unsigned((long)str1, '\t');
  CU_TEST('\t' == mem_get_unsigned((long)str1));

  mem_set_unsigned((long)str1, 0);
  CU_TEST(0 == mem_get_unsigned((long)str1));

  /* test mem_create_store(), mem_destroy_store() */

  ms = mem_create_store(0, 10, sizeof(int), 10, 10); /* create a store - length < 1 (error) */
  CU_TEST(NULL == ms);
  if (NULL != ms)
    mem_destroy_store(ms);

  ms = mem_create_store(10, 0, sizeof(int), 10, 10); /* create a store - width < 1 (error) */
  CU_TEST(NULL == ms);
  if (NULL != ms)
    mem_destroy_store(ms);

  ms = mem_create_store(10, 10, 0, 10, 10);          /* create a store - eltsze < 1 (ok) */
  CU_TEST(NULL != ms);
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(0 == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ms)
    mem_destroy_store(ms);

  ms = mem_create_store(10, 10, sizeof(int), 10, 10);/* create a store for ints */
  CU_TEST(NULL != ms);
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<100 ; ++i) {                           /* check out some elements */
    pint_array[i] = (int*)mem_get_element(ms);
#ifdef MALLOC_DEBUG
    CU_TEST(0 != AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
#endif
  }

  if (NULL != ms)                                     /* destroy the store */
    mem_destroy_store(ms);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
  }
#endif

  /* test mem_get_element(), mem_free_element() */

  ms = mem_create_store(10, 10, sizeof(int), 10, 10);/* create a store for ints */
  CU_TEST(NULL != ms);

  CU_TEST(NULL == mem_get_element(NULL));            /* requesting element of a NULL store has no effect */
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pint_array[i] = (int*)mem_get_element(ms);
    *pint_array[i] = i;
  }
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(50 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=50 ; i<100 ; ++i) {                          /* check out & assign more elements */
    pint_array[i] = (int*)mem_get_element(ms);
    *pint_array[i] = i;
  }
  mem_get_stats(&stats, ms);
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

  mem_free_element(ms, NULL);                         /* returning a NULL element has no effect */
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=0 ; i<50 ; ++i) {                            /* return some elements */
    mem_free_element(ms, pint_array[i]);
  }
  mem_get_stats(&stats, ms);
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
    pint_array[i] = (int*)mem_get_element(ms);
    *pint_array[i] = i;
  }
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ms)                                     /* destroy the store */
    mem_destroy_store(ms);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
  }
#endif

  ms = mem_create_store(10, 10, sizeof(double), 10, 10);/* create a store for doubles */
  CU_TEST(NULL != ms);

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pdoub_array[i] = (double*)mem_get_element(ms);
    *pdoub_array[i] = (double)i;
  }
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(50 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(double) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=50 ; i<100 ; ++i) {                          /* check out & assign more elements */
    pdoub_array[i] = (double*)mem_get_element(ms);
    *pdoub_array[i] = (double)i;
  }
  mem_get_stats(&stats, ms);
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
    mem_free_element(ms, pdoub_array[i]);
  }
  mem_get_stats(&stats, ms);
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
    pdoub_array[i] = (double*)mem_get_element(ms);
    *pdoub_array[i] = (double)i;
  }
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(double) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ms)                                     /* destroy the store */
    mem_destroy_store(ms);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pdoub_array[i], sizeof(VOIDPTR)));
  }
#endif

  ms = mem_create_store(10, 10, 12*sizeof(char), 10, 10);/* create a store for strings */
  CU_TEST(NULL != ms);

  for (i=0 ; i<50 ; ++i) {                            /* check out & assign some elements */
    pstr_array[i] = (char*)mem_get_element(ms);
    strcpy(pstr_array[i], "01234567890");
  }
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(50 == stats.elt_taken);
  CU_TEST(50 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST((12*sizeof(char) + (12*sizeof(char)%sizeof(void*))) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  for (i=50 ; i<100 ; ++i) {                          /* check out & assign more elements */
    pstr_array[i] = (char*)mem_get_element(ms);
    strcpy(pstr_array[i], "01234567890");
  }
  mem_get_stats(&stats, ms);
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
    mem_free_element(ms, pstr_array[i]);
  }
  mem_get_stats(&stats, ms);
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
    pstr_array[i] = (char*)mem_get_element(ms);
    strcpy(pstr_array[i], "01234567890");
  }
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST((12*sizeof(char) + (12*sizeof(char)%sizeof(void*))) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  if (NULL != ms)                                     /* destroy the store */
    mem_destroy_store(ms);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    if (0 != AllocatedMemory((VOIDPTR)pstr_array[i], sizeof(VOIDPTR))) {
      snprintf(str_error_msg, 100, "%s%d%s", "pstr_array[", i, "] not deallocated.");
      CU_FAIL(str_error_msg);
    }
  }
#endif

  /* test mem_clear_store() */

  ms = mem_create_store(10, 10, sizeof(int), 10, 10);/* create a store for ints */
  CU_TEST(NULL != ms);

  /* set up pooling & recycling */
  if (FALSE == gl_pool_initialized()) {
    gl_init();
    gl_init_pool();
    i_initialized_lists = TRUE;
  }
  pint_list = gl_create(100);

  for (i=0 ; i<100 ; ++i) {                           /* check out the elements */
    gl_append_ptr(pint_list, mem_get_element(ms));
  }
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(100 == stats.elt_taken);
  CU_TEST(100 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  mem_clear_store(ms);                                /* clear the pool store */
  mem_get_stats(&stats, ms);
  CU_TEST(100 == stats.elt_total);
  CU_TEST(0 == stats.elt_taken);
  CU_TEST(0 == stats.elt_inuse);
  CU_TEST(0 == stats.elt_onlist);
  CU_TEST(sizeof(int) == stats.elt_size);
  CU_TEST(10 == stats.str_len);
  CU_TEST(10 == stats.str_wid);

  CU_TEST(0 == gl_sorted(pint_list));                 /* make sure list isn't sorted */
  for (i=0 ; i<100 ; ++i) {                           /* check out the elements again & confirm they are the same */
    pint_array[i] = (int*)mem_get_element(ms);
    CU_TEST(0 != gl_ptr_search(pint_list, (VOIDPTR)pint_array[i], FALSE));
#ifdef MALLOC_DEBUG
    CU_TEST(0 != AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
#endif
  }

  gl_destroy(pint_list);

  if (TRUE == i_initialized_lists) {
    gl_destroy_pool();
  }

  if (NULL != ms)                                     /* destroy the store */
    mem_destroy_store(ms);

#ifdef MALLOC_DEBUG
  for (i=0 ; i<100 ; ++i) {                           /* confirm destruction of the elements */
    CU_TEST(0 == AllocatedMemory((VOIDPTR)pint_array[i], sizeof(VOIDPTR)));
  }
#endif

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo mem_test_list[] = {
  {"test_mem", test_mem},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_utilities_mem", NULL, NULL, mem_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_mem(void)
{
  return CU_register_suites(suites);
}
