/*
 *  Unit test functions for ASCEND: utilities/ascMalloc.c
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
#include <utilities/error.h>
#ifdef __WIN32__
#include <io.h>
#endif
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <CUnit/CUnit.h>
#include "test_ascMalloc.h"
#include "assertimpl.h"

#undef STR_LEN
#define STR_LEN 100

/*
 *  ascMalloc.c is a challenging module to test.  There are 
 *  numerous different definitions for the function macros 
 *  depending on the definitions of MOD_ASCMALLOC, MOD_REALLOC,
 *  MALLOC_DEBUG, and ALLOCATED_TESTS.
 *
 *  As a first pass, we will only test a single set of definitions.
 *  This is whatever set is defined based on the settings of these
 *  macros at compilation time.
 */
static void test_ascMalloc(void)
{
  char str1[STR_LEN];
  char str2[STR_LEN];
  char str3[STR_LEN];
  char *p_str1;
  char *p_str2;
  char *p_str3;
  unsigned long i;
  int str1_bad;
  int str2_bad;
  int *p_int1;
  int *p_int2;
  int *p_int3;
  double *p_doub1;
  double *p_doub2;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */
  CONSOLE_DEBUG("IN USE = %lu",prior_meminuse);

#ifdef NDEBUG
  CU_FAIL("test_ascMalloc() compiled with NDEBUG - some features not tested.");
#endif
#ifndef MALLOC_DEBUG
  CU_FAIL("test_ascMalloc() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  /* test ASC_STRDUP() */
  CU_ASSERT(NULL == ASC_STRDUP(NULL));                   /* NULL str */

  p_str1 = ASC_STRDUP("Just a simple little string.");  /* normal operation with literal*/
  CU_ASSERT(NULL != p_str1);
  CU_ASSERT(0 == strcmp(p_str1, "Just a simple little string."));
  CU_ASSERT(0 != ascmeminuse());
  ascfree(p_str1);
  CU_ASSERT(0 == ascmeminuse());

  snprintf(str1, STR_LEN-1, "I'm a simple string.");
  p_str1 = ASC_STRDUP(str1);                            /* normal operation with literal*/
  CU_ASSERT(NULL != p_str1);
  CU_ASSERT(0 == strcmp(p_str1, str1));
  CU_ASSERT(0 != ascmeminuse());
  ascfree(p_str1);
  CU_ASSERT(0 == ascmeminuse());

  /* test asc_memcpy() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    asc_memcpy(NULL, str2, STR_LEN);                  /* error - NULL dest */
  CU_ASSERT(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    asc_memcpy(str1, NULL, STR_LEN);                  /* error - NULL src */
  CU_ASSERT(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  str1_bad = FALSE;                                  /* test initial condition */
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '\\0' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%lu] != '*' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  CU_ASSERT(str1 == asc_memcpy(str1, str2, STR_LEN/2)); /* copy part of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '*' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '\\0' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%lu] != '*' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  CU_ASSERT(str1 == asc_memcpy(str1, str2, STR_LEN));  /* copy all of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '*' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%lu] != '*' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  CU_ASSERT((str1+10) == asc_memcpy(str1+10, str1, 20)); /* copy overlapping memory block */

  str1_bad = FALSE;
  for (i=0 ; i<20 ; ++i) {
    if (!str1_bad && (str1[i] != '~')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '~' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=20 ; i<30 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '=' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }                                                                       
  }
  if (!str1_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '~', 10);
  memset(str1+10, '=', 10);
  memset(str1+20, '|', 10);

  CU_ASSERT(str1 == asc_memcpy(str1, str1+10, 20)); /* copy overlapping memory block */

  str1_bad = FALSE;
  for (i=0 ; i<10 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '~' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=10 ; i<30 ; ++i) {
    if (!str1_bad && (str1[i] != '|')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '=' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 and str2 check out.");

  snprintf(str1, STR_LEN-1, "This is yet another dumb string");
  CU_ASSERT(str1 == asc_memcpy(str1, str1, strlen(str1))); /* to == from */
  CU_ASSERT(0 == strcmp(str1, "This is yet another dumb string"));

  CU_ASSERT(str1 == asc_memcpy(str1, str2, 0));           /* n = 0 */
  CU_ASSERT(0 == strcmp(str1, "This is yet another dumb string"));

  /* test ascreallocPURE() */

  p_str1 = ASC_NEW_ARRAY(char,50);                       /* allocate a block & check status */
  CU_TEST_FATAL(NULL != p_str1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(50 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_str1, 50));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 50));
#endif

  snprintf(p_str1, 49, "I should survive a reallocation!");
  p_str1 = ascreallocPURE(p_str1, 50, 100);
  CU_TEST_FATAL(NULL != p_str1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(100 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_str1, 100));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));
#endif
  CU_ASSERT(0 == strcmp(p_str1, "I should survive a reallocation!"));

  ascfree(p_str1);
  CU_ASSERT(0 == ascmeminuse());
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == AllocatedMemory(p_str1, 0));
#else
  CU_ASSERT(1 == AllocatedMemory(p_str1, 0));
#endif

  /* ascstatus(), ascstatus_detail() - reporting functions, not tested */
  /* ascshutdown() - do not call here or will result in closure of test memory log */
  /* ascmeminuse() - tested adequately by other tests */

  /* test asccalloc() */

  p_int1 = (int *)asccalloc(0, sizeof(int));         /* 0 elements requested */
  CU_TEST_FATAL(NULL != p_int1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(AT_LEAST_1(0) == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int1, 0));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  ascfree(p_int1);
  CU_ASSERT(0 == ascmeminuse());
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == AllocatedMemory(p_int1, 0));
#else
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  p_int1 = (int *)asccalloc(100, 0);                 /* 0 size requested */
  CU_TEST_FATAL(NULL != p_int1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int1, 0));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  ascfree(p_int1);
  CU_ASSERT(0 == ascmeminuse());
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == AllocatedMemory(p_int1, 0));
#else
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  p_int1 = (int *)asccalloc(100, sizeof(int));       /* 100 elements requested */
  CU_TEST_FATAL(NULL != p_int1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(100*sizeof(int) == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int1, 100*sizeof(int)));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  p_int2 = (int *)asccalloc(200, sizeof(int));       /* 200 more elements requested */
  CU_TEST_FATAL(NULL != p_int2);
#ifdef MALLOC_DEBUG
  CU_ASSERT(300*sizeof(int) == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int2, 200*sizeof(int)));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int2, 0));
#endif

  p_int3 = (int *)asccalloc(10, sizeof(int));        /* 10 more elements requested */
  CU_TEST_FATAL(NULL != p_int3);
#ifdef MALLOC_DEBUG
  CU_ASSERT(310*sizeof(int) == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int3, 10*sizeof(int)));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int3, 0));
#endif

  ascfree(p_int2);
#ifdef MALLOC_DEBUG
  CU_ASSERT(110*sizeof(int) == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int1, 100*sizeof(int)));
  CU_ASSERT(0 == AllocatedMemory(p_int2, 200*sizeof(int)));
  CU_ASSERT(2 == AllocatedMemory(p_int3, 10*sizeof(int)));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 100*sizeof(int)));
  CU_ASSERT(1 == AllocatedMemory(p_int2, 200*sizeof(int)));
  CU_ASSERT(1 == AllocatedMemory(p_int3, 10*sizeof(int)));
#endif

  ascfree(p_int1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(10*sizeof(int) == ascmeminuse());
  CU_ASSERT(0 == AllocatedMemory(p_int1, 100*sizeof(int)));
  CU_ASSERT(0 == AllocatedMemory(p_int2, 200*sizeof(int)));
  CU_ASSERT(2 == AllocatedMemory(p_int3, 10*sizeof(int)));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 100*sizeof(int)));
  CU_ASSERT(1 == AllocatedMemory(p_int2, 200*sizeof(int)));
  CU_ASSERT(1 == AllocatedMemory(p_int3, 10*sizeof(int)));
#endif

  ascfree(p_int3);
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(0 == AllocatedMemory(p_int1, 100*sizeof(int)));
  CU_ASSERT(0 == AllocatedMemory(p_int2, 200*sizeof(int)));
  CU_ASSERT(0 == AllocatedMemory(p_int3, 10*sizeof(int)));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 100*sizeof(int)));
  CU_ASSERT(1 == AllocatedMemory(p_int2, 200*sizeof(int)));
  CU_ASSERT(1 == AllocatedMemory(p_int3, 10*sizeof(int)));
#endif

  /* test ascmalloc() */

  p_int1 = (int *)ascmalloc(0);                      /* 0 bytes requested */
  CU_TEST_FATAL(NULL != p_int1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(AT_LEAST_1(0) == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int1, 0));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  ascfree(p_int1);
  CU_ASSERT(0 == ascmeminuse());
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == AllocatedMemory(p_int1, 0));
#else
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  p_int1 = (int *)ascmalloc(100);                    /* 100 bytes requested */
  CU_TEST_FATAL(NULL != p_int1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(100 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int1, 100));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 0));
#endif

  p_int2 = (int *)ascmalloc(200);                    /* 200 more bytes requested */
  CU_TEST_FATAL(NULL != p_int2);
#ifdef MALLOC_DEBUG
  CU_ASSERT(300 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int2, 200));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int2, 0));
#endif

  p_int3 = (int *)ascmalloc(10);                     /* 10 more bytes requested */
  CU_TEST_FATAL(NULL != p_int3);
#ifdef MALLOC_DEBUG
  CU_ASSERT(310 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int3, 10));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int3, 0));
#endif

  ascfree(p_int2);
#ifdef MALLOC_DEBUG
  CU_ASSERT(110 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_int1, 100));
  CU_ASSERT(0 == AllocatedMemory(p_int2, 200));
  CU_ASSERT(2 == AllocatedMemory(p_int3, 10));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_int2, 200));
  CU_ASSERT(1 == AllocatedMemory(p_int3, 10));
#endif

  ascfree(p_int1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(10 == ascmeminuse());
  CU_ASSERT(0 == AllocatedMemory(p_int1, 100));
  CU_ASSERT(0 == AllocatedMemory(p_int2, 200));
  CU_ASSERT(2 == AllocatedMemory(p_int3, 10));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_int2, 200));
  CU_ASSERT(1 == AllocatedMemory(p_int3, 10));
#endif

  ascfree(p_int3);
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(0 == AllocatedMemory(p_int1, 100));
  CU_ASSERT(0 == AllocatedMemory(p_int2, 200));
  CU_ASSERT(0 == AllocatedMemory(p_int3, 10));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_int1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_int2, 200));
  CU_ASSERT(1 == AllocatedMemory(p_int3, 10));
#endif

  /* test ascrealloc() */

  p_str1 = ASC_NEW_ARRAY(char,50);                       /* allocate several blocks & check status */
  p_str2 = ASC_NEW_ARRAY(char,20);
  p_str3 = ASC_NEW_ARRAY(char,25);
  CU_TEST_FATAL(NULL != p_str1);
  CU_TEST_FATAL(NULL != p_str2);
  CU_TEST_FATAL(NULL != p_str3);
#ifdef MALLOC_DEBUG
  CU_ASSERT(95 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_str1, 50));
  CU_ASSERT(2 == AllocatedMemory(p_str2, 20));
  CU_ASSERT(2 == AllocatedMemory(p_str3, 25));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 50));
  CU_ASSERT(1 == AllocatedMemory(p_str2, 20));
  CU_ASSERT(1 == AllocatedMemory(p_str3, 25));
#endif

  snprintf(p_str1, 49, "I should survive a reallocation!");
  snprintf(p_str2, 19, "Me too?");
  snprintf(p_str3, 24, "Realloc me away.");

  p_str1 = ascrealloc(p_str1, 100);                     /* realloc to larger size */
  CU_TEST_FATAL(NULL != p_str1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(145 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(2 == AllocatedMemory(p_str2, 20));
  CU_ASSERT(2 == AllocatedMemory(p_str3, 25));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_str2, 20));
  CU_ASSERT(1 == AllocatedMemory(p_str3, 25));
#endif
  CU_ASSERT(0 == strcmp(p_str1, "I should survive a reallocation!"));
  CU_ASSERT(0 == strcmp(p_str2, "Me too?"));
  CU_ASSERT(0 == strcmp(p_str3, "Realloc me away."));

  p_str2 = ascrealloc(p_str2, 10);                       /* realloc to smaller size */
  CU_TEST_FATAL(NULL != p_str2);
#ifdef MALLOC_DEBUG
  CU_ASSERT(135 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(2 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(2 == AllocatedMemory(p_str3, 25));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(1 == AllocatedMemory(p_str3, 25));
#endif
  CU_ASSERT(0 == strcmp(p_str1, "I should survive a reallocation!"));
  CU_ASSERT(0 == strcmp(p_str2, "Me too?"));
  CU_ASSERT(0 == strcmp(p_str3, "Realloc me away."));

  p_str3 = ascrealloc(p_str3, 0);                       /* realloc to zero */
  CU_ASSERT(NULL == p_str3);
#ifdef MALLOC_DEBUG
  CU_ASSERT(110+AT_LEAST_1(0) == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(2 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(0 == AllocatedMemory(p_str3, 0));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(1 == AllocatedMemory(p_str3, 0));
#endif
  CU_ASSERT(0 == strcmp(p_str1, "I should survive a reallocation!"));
  CU_ASSERT(0 == strcmp(p_str2, "Me too?"));

  ascfree(p_str3);
#ifdef MALLOC_DEBUG
  CU_ASSERT(110 == ascmeminuse());
  CU_ASSERT(2 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(2 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(0 == AllocatedMemory(p_str3, 0));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(1 == AllocatedMemory(p_str3, 0));
#endif

  ascfree(p_str1);
#ifdef MALLOC_DEBUG
  CU_ASSERT(10 == ascmeminuse());
  CU_ASSERT(0 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(2 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(0 == AllocatedMemory(p_str3, 0));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(1 == AllocatedMemory(p_str3, 0));
#endif

  ascfree(p_str2);
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(0 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(0 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(0 == AllocatedMemory(p_str3, 0));
#else
  CU_ASSERT(0 == ascmeminuse());
  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_str2, 10));
  CU_ASSERT(1 == AllocatedMemory(p_str3, 0));
#endif

  /* ascfree() tested adequately by other tests */

  /* test ascbcopy() */

  memset(str1, '\0', STR_LEN);
  memset(str2, '*', STR_LEN);

  CU_ASSERT(str2 == ascbcopy(str1, str2, STR_LEN/2));     /* copy part of a memory block */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str2_bad && (str2[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str2[%lu] != '\\0' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str2_bad && (str2[i] != '*')) {
      snprintf(str3, STR_LEN-1, "str2[%lu] != '*' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '\\0' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  memset(str1, '+', STR_LEN);
  memset(str2, '-', STR_LEN);

  CU_ASSERT(str2 == ascbcopy(str1, str2, 0));             /* 0 bytes copied */

  str1_bad = FALSE;
  str2_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '+')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '+' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
    if (!str2_bad && (str2[i] != '-')) {
      snprintf(str3, STR_LEN-1, "str2[%lu] != '-' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str2_bad = TRUE;
    }
  }
  if (!str1_bad && !str2_bad) CU_PASS("str1 and str2 check out.");

  /* test asczero() */

  memset(str1, '=', STR_LEN);

  CU_ASSERT(str1 == ascbzero(str1, STR_LEN/2));           /* zero part of a memory block */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '\0')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '\\0' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '=')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '=' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after ascbzero().");

  memset(str1, '+', STR_LEN);

  CU_ASSERT(str1 == ascbzero(str1, 0));                   /* 0 bytes processed */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '+')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '+' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after ascbzero.");

 /* test ascbfill() */

  memset(str1, '@', STR_LEN);

  CU_ASSERT(str1 == ascbfill(str1, STR_LEN/2));           /* fill part of a memory block */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN/2 ; ++i) {
    if (!str1_bad && (str1[i] != '\xff')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != 255 in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  for (i=STR_LEN/2 ; i<STR_LEN-1 ; ++i) {
    if (!str1_bad && (str1[i] != '@')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '@' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after ascbfill().");

  memset(str1, '#', STR_LEN);

  CU_ASSERT(str1 == ascbfill(str1, 0));                   /* 0 bytes processed */

  str1_bad = FALSE;
  for (i=0 ; i<STR_LEN ; ++i) {
    if (!str1_bad && (str1[i] != '#')) {
      snprintf(str3, STR_LEN-1, "str1[%lu] != '#' in test_ascMalloc().", i);
      CU_FAIL(str3);
      str1_bad = TRUE;
    }
  }
  if (!str1_bad) CU_PASS("str1 checks out after ascbfill.");

  /* test AllocatedMemory() */

#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == AllocatedMemory(NULL, 0));               /* NULL pointer, nothing allocated */
  CU_ASSERT(0 == AllocatedMemory(&str1_bad, 0));          /* non-NULL pointer, nothing allocated */
#else
  CU_ASSERT(1 == AllocatedMemory(NULL, 0));               /* NULL pointer, nothing allocated */
  CU_ASSERT(1 == AllocatedMemory(&str1_bad, 0));          /* non-NULL pointer, nothing allocated */
#endif

  p_str1 = ASC_NEW_ARRAY(char,100);                      /* allocate 1 block */
  CU_TEST_FATAL(NULL != p_str1);

  p_str2 = (char *)malloc(100);                         /* allocate another outside ascMalloc */
  CU_TEST_FATAL(NULL != p_str2);

#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == AllocatedMemory(NULL, 0));               /* NULL pointer */

  CU_ASSERT(0 == AllocatedMemory(p_str2, 0));             /* pointer allocated outside ascMalloc */

  CU_ASSERT(2 == AllocatedMemory(p_str1, 100));           /* complete blocks */

  CU_ASSERT(1 == AllocatedMemory(p_str1, 99));            /* contained blocks */
  CU_ASSERT(1 == AllocatedMemory(p_str1+1, 99));

  CU_ASSERT(-1 == AllocatedMemory(p_str1, 101));          /* overlapping blocks */
  CU_ASSERT(-1 == AllocatedMemory(p_str1+1, 100));
  CU_ASSERT(-1 == AllocatedMemory(p_str1-1, 2));
  CU_ASSERT(-1 == AllocatedMemory(p_str1-1, 150));

  CU_ASSERT(0 == AllocatedMemory(p_str1-10, 10));         /* non-overlapping blocks */
  CU_ASSERT(0 == AllocatedMemory(p_str1-1, 1));

#else
  CU_ASSERT(1 == AllocatedMemory(NULL, 0));               /* NULL pointer */

  CU_ASSERT(1 == AllocatedMemory(p_str2, 0));             /* pointer allocated outside ascMalloc */

  CU_ASSERT(1 == AllocatedMemory(p_str1, 100));           /* complete blocks */

  CU_ASSERT(1 == AllocatedMemory(p_str1, 99));            /* contained blocks */
  CU_ASSERT(1 == AllocatedMemory(p_str1+1, 99));

  CU_ASSERT(1 == AllocatedMemory(p_str1, 101));           /* overlapping blocks */
  CU_ASSERT(1 == AllocatedMemory(p_str1+1, 100));
  CU_ASSERT(1 == AllocatedMemory(p_str1-1, 2));
  CU_ASSERT(1 == AllocatedMemory(p_str1-1, 150));

  CU_ASSERT(1 == AllocatedMemory(p_str1-10, 10));         /* non-overlapping blocks */
  CU_ASSERT(1 == AllocatedMemory(p_str1-1, 1));

#endif

  ascfree(p_str1);
  free(p_str2);
  CU_ASSERT(0 == ascmeminuse());

  p_doub1 = (double *)ascmalloc(sizeof(double));        /* allocate 1 block */
  CU_TEST_FATAL(NULL != p_doub1);

  p_doub2 = (double *)malloc(sizeof(double));           /* allocate another outside ascMalloc */
  CU_TEST_FATAL(NULL != p_doub2);

#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == AllocatedMemory(NULL, 0));               /* NULL pointer */

  CU_ASSERT(0 == AllocatedMemory(p_doub2, 0));            /* pointer allocated outside ascMalloc */

  CU_ASSERT(2 == AllocatedMemory(p_doub1, sizeof(double)));/* complete blocks */

  CU_ASSERT(1 == AllocatedMemory(p_doub1, 0));            /* contained blocks */
  CU_ASSERT(1 == AllocatedMemory((char *)p_doub1+1, sizeof(double)-1));

  CU_ASSERT(-1 == AllocatedMemory(p_doub1, sizeof(double)+1)); /* overlapping blocks */
  CU_ASSERT(-1 == AllocatedMemory((char *)p_doub1+1, sizeof(double)));
  CU_ASSERT(-1 == AllocatedMemory((char *)p_doub1-1, 2));

  CU_ASSERT(0 == AllocatedMemory((char *)p_doub1-1, 1));  /* non-overlapping blocks */
  CU_ASSERT(0 == AllocatedMemory((char *)p_doub1-100, 100));

#else
  CU_ASSERT(1 == AllocatedMemory(NULL, 0));               /* NULL pointer */

  CU_ASSERT(1 == AllocatedMemory(p_doub2, 0));            /* pointer allocated outside ascMalloc */

  CU_ASSERT(1 == AllocatedMemory(p_doub1, sizeof(double)));/* complete blocks */

  CU_ASSERT(1 == AllocatedMemory(p_doub1, 0));            /* contained blocks */
  CU_ASSERT(1 == AllocatedMemory((char *)p_doub1+1, sizeof(double)-1));

  CU_ASSERT(1 == AllocatedMemory(p_doub1, sizeof(double)+1)); /* overlapping blocks */
  CU_ASSERT(1 == AllocatedMemory((char *)p_doub1+1, sizeof(double)));
  CU_ASSERT(1 == AllocatedMemory((char *)p_doub1-1, 2));

  CU_ASSERT(1 == AllocatedMemory((char *)p_doub1-1, 1));  /* non-overlapping blocks */
  CU_ASSERT(1 == AllocatedMemory((char *)p_doub1-100, 100));

#endif

  ascfree(p_doub1);
  free(p_doub2);
  CU_ASSERT(0 == ascmeminuse());

  /* test InMemoryBlock() */

  p_str1 = ASC_NEW_ARRAY(char,100);                      /* allocate 1 block */
  CU_TEST_FATAL(NULL != p_str1);
  CU_ASSERT(0 != InMemoryBlock(p_str1, p_str1));
  CU_ASSERT(0 != InMemoryBlock(p_str1, p_str1+1));
  CU_ASSERT(0 != InMemoryBlock(p_str1, p_str1+50));
  CU_ASSERT(0 != InMemoryBlock(p_str1, p_str1+99));
#ifdef MALLOC_DEBUG
  CU_ASSERT(0 == InMemoryBlock(p_str1, p_str1-1));
  CU_ASSERT(0 == InMemoryBlock(p_str1, p_str1+100));
  CU_ASSERT(0 == InMemoryBlock(p_str1, p_str1+101));
  CU_ASSERT(0 == InMemoryBlock(p_str1, (VOIDPTR)0));
#else
  CU_ASSERT(1 == InMemoryBlock(p_str1, p_str1-1));
  CU_ASSERT(1 == InMemoryBlock(p_str1, p_str1+100));
  CU_ASSERT(1 == InMemoryBlock(p_str1, p_str1+101));
  CU_ASSERT(1 == InMemoryBlock(p_str1, (VOIDPTR)0));
#endif

  ascfree(p_str1);
  CU_ASSERT(0 == ascmeminuse());

  /* test AssertAllocatedMemory() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_str1, 100);               /* error - no memory allocated */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  p_str1 = ASC_NEW_ARRAY(char,100);
  p_int1 = (int *)asccalloc(10, sizeof(int));

  p_str2 = (char *)malloc(100);                       /* don't use asc*alloc! */
  p_int2 = (int *)calloc(10, sizeof(int));

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(NULL, 100);                 /* error - NULL ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_str1, 100);               /* ok - allocated block, correct size*/
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_str1, 99);                /* error - incorrect size */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_str1, 101);               /* error - incorrect size */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_str2, 100);               /* error - invalid ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_int1, 10*sizeof(int));    /* ok - allocated block, correct size*/
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_int1, 10*sizeof(int)-1);  /* error - incorrect size */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_int1, 10*sizeof(int)+1);  /* error - incorrect size */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertAllocatedMemory(p_int2, 10*sizeof(int));    /* error - invalid ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  ascfree(p_str1);
  ascfree(p_int1);
  free(p_str2);
  free(p_int2);

  /* test AssertMemory() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertMemory(p_str1);                             /* error - no memory allocated */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  p_str1 = ASC_NEW_ARRAY(char,100);
  p_int1 = (int *)asccalloc(10, sizeof(int));

  p_str2 = (char *)malloc(100);                       /* don't use asc*alloc! */
  p_int2 = (int *)calloc(10, sizeof(int));

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertMemory(NULL);                               /* error -  NULL ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertMemory(p_str1);                             /* ok - start of allocated block*/
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertMemory(p_str1+10);                          /* ok - in allocated block*/
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertMemory(p_int1);                             /* ok - start of allocated block */
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertMemory(p_str2);                             /* error - not allocated using asc*alloc */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertMemory(p_int2);                             /* error - not allocated using asc*alloc */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  ascfree(p_str1);
  ascfree(p_int1);
  free(p_str2);
  free(p_int2);

  /* test AssertContainedMemory() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_str1, 100);               /* error - no memory allocated */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  p_str1 = ASC_NEW_ARRAY(char,100);
  p_int1 = (int *)asccalloc(10, sizeof(int));

  p_str2 = (char *)malloc(100);                       /* don't use asc*alloc! */
  p_int2 = (int *)calloc(10, sizeof(int));

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(NULL, 100);                 /* error - NULL ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_str1, 100);               /* ok - allocated block, correct size*/
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_str1, 0);                 /* ok - contained in a block*/
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_str1+10, 50);             /* ok - contained in a block */
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_str1, 101);               /* error - incorrect size */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_str2, 0);                 /* error - invalid ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_int1, 10*sizeof(int));    /* ok - allocated block, correct size*/
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_int1, sizeof(int)-1);     /* ok - incorrect size */
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_int1, 10*sizeof(int)+1);  /* error - incorrect size */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedMemory(p_int2, 10*sizeof(int));    /* error - invalid ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  ascfree(p_str1);
  ascfree(p_int1);
  free(p_str2);
  free(p_int2);

  /* test AssertContainedIn() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1);                /* error - no memory allocated */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  p_str1 = ASC_NEW_ARRAY(char,100);                    /* allocate 1 block */
  CU_TEST_FATAL(NULL != p_str1);

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1);                /* ok - same pointer */
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1+1);              /* ok - in block */
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1+50);             /* ok - in block */
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1+99);             /* ok - in block */
  CU_ASSERT(FALSE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1-1);              /* error - outside block */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1+100);            /* error - outside block */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, p_str1+101);            /* error - outside block */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(p_str1, NULL);                  /* error - NULL ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AssertContainedIn(NULL, p_str1);                  /* error - NULL ptr */
#if defined(MALLOC_DEBUG) && defined(ALLOCATED_TESTS)
  CU_ASSERT(TRUE == asc_assert_failed());
#else
  CU_ASSERT(FALSE == asc_assert_failed());
#endif

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  ascfree(p_str1);

  CU_ASSERT(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo ascMalloc_test_list[] = {
  {"ascMalloc", test_ascMalloc},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"utilities_ascMalloc", NULL, NULL, ascMalloc_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_ascMalloc(void)
{
  return CU_register_suites(suites);
}
