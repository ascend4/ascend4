/*
 *  Unit test functions for ASCEND: general/tm_time.c
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

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/tm_time.h>
#include "CUnit/CUnit.h"
#include "assertimpl.h"
#include "test_tm_time.h"

/* 
 *  This is pretty simplistic, but so is tm_time.[ch].
 *  We just check for valid values, do a simple timing test, and stop.
 */
static void test_tm_time(void)
{
  unsigned long i;
  double start;
  double elapsed[7];
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifdef NDEBUG
  CU_FAIL("test_tm_time() compiled with NDEBUG - some features not tested.");
#endif

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    tm_cpu_time_ftn_(NULL);             /* error if NULL time */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    aftime_(NULL);                      /* error if NULL time */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    tm_cpu_time_ftn(NULL);              /* error if NULL time */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    aftime(NULL);                       /* error if NULL time */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    TM_CPU_TIME_FTN(NULL);              /* error if NULL time */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    AFTIME(NULL);                       /* error if NULL time */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  start = tm_cpu_time();                /* record the initial time */

  for (i=0 ; i<100000000 ; i += 2) {    /* consume some CPU time */
    --i;
  }

  CU_TEST(tm_cpu_time() - start > 0.0); /* should see an increase in elapsed PU time */

  elapsed[0] = tm_cpu_time();           /* the timer variants should all return ~the same elapsed time */
  tm_cpu_time_ftn_(&elapsed[1]);        /* this assumes a fast CPU */
  aftime_(&elapsed[2]);
  tm_cpu_time_ftn(&elapsed[3]);
  aftime(&elapsed[4]);
  TM_CPU_TIME_FTN(&elapsed[5]);
  AFTIME(&elapsed[6]);

  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[1], 0.01);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[2], 0.01);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[3], 0.01);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[4], 0.01);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[5], 0.01);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[6], 0.01);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo tm_time_test_list[] = {
  {"test_tm_time", test_tm_time},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_tm_time", NULL, NULL, tm_time_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_tm_time(void)
{
  return CU_register_suites(suites);
}
