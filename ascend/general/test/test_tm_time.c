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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/tm_time.h>

#ifndef __WIN32__
# include <time.h>
#endif

#include <test/common.h>
#include <test/assertimpl.h>

#ifndef __WIN32__
static double test_wall_time_monotonic(void)
{
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    return 0.0;
  }
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

static double test_cpu_time_resolution(void)
{
  struct timespec ts;
  if (clock_getres(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) {
    return 0.01;
  }
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}
#endif

/*
 *  This is pretty simplistic, but so is tm_time.[ch].
 *  We just check for valid values, do a simple timing test, and stop.
 */
static void test_tm_time(void)
{
  unsigned long i;
  unsigned long retry;
  double start;
  double end;
  double elapsed[7];
  unsigned long prior_meminuse;
  volatile unsigned long burn = 0;
#ifndef __WIN32__
  double wall_deadline;
#endif

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

//  tm_reset_cpu_time();
  start = tm_cpu_time();                /* record the initial time */
  //CU_TEST(start == 0.0);

  for (i=0 ; i<10000000 ; ++i) {         /* consume some CPU time deterministically */
    burn += (i & 1U);
  }
 
  end = tm_cpu_time();
#ifndef __WIN32__
  wall_deadline = test_wall_time_monotonic() + 1.0;
#endif
  for (retry = 0; retry < 200 && end <= start; ++retry) {
    for (i = 0; i < 1000000; ++i) {
      burn += ((i + retry) & 1U);
    }
    end = tm_cpu_time();
#ifndef __WIN32__
    if (test_wall_time_monotonic() >= wall_deadline) {
      break;
    }
#endif
  }
  CU_TEST(burn > 0);
  CU_TEST(end > start); /* should see an increase in elapsed CPU time */

  elapsed[0] = tm_cpu_time();           /* the timer variants should all return approx the same elapsed time */
  tm_cpu_time_ftn_(&elapsed[1]);        /* this assumes a fast CPU */
  aftime_(&elapsed[2]);
  tm_cpu_time_ftn(&elapsed[3]);
  aftime(&elapsed[4]);
  TM_CPU_TIME_FTN(&elapsed[5]);
  AFTIME(&elapsed[6]);

#if 0
  for(i=0;i<7;++i){
  	fprintf(stderr,"elapsed[%lu] = %lf\n",i,elapsed[i]);
  }
#endif

#ifdef __WIN32__
  double dtmin = 0.04;
#else
  double dtmin = test_cpu_time_resolution();
  if (dtmin < 0.01) {
    dtmin = 0.01;
  }
#endif

  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[1], dtmin);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[2], dtmin);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[3], dtmin);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[4], dtmin);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[5], dtmin);
  CU_ASSERT_DOUBLE_EQUAL(elapsed[0], elapsed[6], dtmin);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(tm_time)

REGISTER_TESTS_SIMPLE(general_tm_time, TESTS)
