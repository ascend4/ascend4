/*
 *  Unit test functions for ASCEND: utilities/set.c
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
#include "utilities/ascMalloc.h"
#include "utilities/set.h"
#include "CUnit/CUnit.h"
#include "assertimpl.h"
#include "test_set.h"

static void test_set(void)
{
  unsigned *set1;
  unsigned int i;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifndef MALLOC_DEBUG
  CU_FAIL("test_set() compiled without MALLOC_DEBUG - memory management not tested.");
#endif

  /* test set_size() */

  CU_TEST(0 == set_size(0));
  for (i=1 ; i<=WORDSIZE ; ++i) {
    CU_TEST(1 == set_size(i));
  }
  for (i=(WORDSIZE+1) ; i<=(WORDSIZE*2) ; ++i) {
    CU_TEST(2 == set_size(i));
  }
  for (i=((500*WORDSIZE)+1) ; i<=(WORDSIZE*501) ; ++i) {
    CU_TEST(501 == set_size(i));
  }

  /* test set_create(), set_destroy() */

  set1 = set_create(-10);                             /* error - negative size */
  CU_TEST(NULL == set1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(set1, 0));
#endif

  if (NULL != set1)
    set_destroy(set1);

  set1 = set_create(0);                               /* error - zero size */
  CU_TEST(NULL == set1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(set1, 0));
#endif
  if (NULL != set1)
    set_destroy(set1);

  set1 = set_create(10);                              /* ok - valid size */
  CU_TEST(NULL != set1);
#ifdef MALLOC_DEBUG
  CU_TEST(2 == AllocatedMemory(set1, sizeof(unsigned)*set_size(10)));
#endif
  if (NULL != set1)
    set_destroy(set1);
#ifdef MALLOC_DEBUG
  CU_TEST(0 == AllocatedMemory(set1, sizeof(unsigned)*set_size(10)));
#endif

  /* test set_ndx() */

  for (i=0 ; i<WORDSIZE ; ++i) {
    CU_TEST(0 == set_ndx(i));
  }
  for (i=WORDSIZE ; i<(WORDSIZE*2) ; ++i) {
    CU_TEST(1 == set_ndx(i));
  }
  for (i=(500*WORDSIZE) ; i<(WORDSIZE*501) ; ++i) {
    CU_TEST(500 == set_ndx(i));
  }

  /* test set_mask() */

  for (i=0 ; i<WORDSIZE ; ++i) {
    CU_TEST(((unsigned)1 << i) == set_mask(i));
  }
  for (i=((500*WORDSIZE)+1) ; i<=(WORDSIZE*501) ; ++i) {
    CU_TEST(((unsigned)1 << (i-(500*WORDSIZE))) == set_mask(i));
  }

  /* test set_is_member() */

  set1 = set_create(10);                              /* create a set & zero it */
  CU_TEST(NULL != set1);

  if (NULL != set1) {
    set_null(set1, 10);

    for (i=0 ; i<10 ; ++i) {
      CU_TEST(FALSE == set_is_member(set1, i));       /* set should not contain member */
      set_change_member(set1, i, TRUE);               /* add member to set */
      CU_TEST(TRUE == set_is_member(set1, i));        /* set should contain member */
    }

    set_destroy(set1);
  }

  /* test set_chk_is_member() */

  set1 = set_create(10);                              /* create a set & zero it */
  CU_TEST(NULL != set1);

  if (NULL != set1) {
    set_null(set1, 10);
    CU_TEST(FALSE == set_chk_is_member(set1, -1, 10));  /* error - negative element */
    CU_TEST(FALSE == set_chk_is_member(set1, 11, 10));  /* error - element larger than size */

    for (i=0 ; i<10 ; ++i) {
      CU_TEST(FALSE == set_chk_is_member(set1, (int)i, 10)); /* set should not contain member */
      set_change_member(set1, i, TRUE);                      /* add member to set */
      CU_TEST(TRUE == set_chk_is_member(set1, (int)i, 10));  /* set should contain member */
    }

    CU_TEST(FALSE == set_chk_is_member(set1, -1, 10));  /* error - negative element */
    CU_TEST(FALSE == set_chk_is_member(set1, 11, 10));  /* error - element lsrger than size */

    set_destroy(set1);
  }

  /* test set_null() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    set_null(NULL, 10);                         /* error - set NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */  

  set1 = set_create(10);                              /* create a set & zero it */
  CU_TEST(NULL != set1);

  if (NULL != set1) {
    set_null(set1, 10);

    for (i=0 ; i<10 ; ++i) {
      CU_TEST(FALSE == set_is_member(set1, i));       /* set should not contain any members */
    }

    for (i=0 ; i<10 ; ++i) {
      set_change_member(set1, i, TRUE);               /* add member to set */
    }

    for (i=0 ; i<10 ; ++i) {
      CU_TEST(TRUE == set_is_member(set1, i));        /* set should contain all members */
    }

    set_null(set1, 10);

    for (i=0 ; i<10 ; ++i) {
      CU_TEST(FALSE == set_is_member(set1, i));       /* set should not contain any members */
    }

    set_destroy(set1);
  }

  /* test set_change_member() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                        /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    set_change_member(NULL, 1, TRUE);
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    set_change_member(NULL, 1, FALSE);
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                       /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  set1 = set_create(10);                              /* create a set & zero it */
  CU_TEST(NULL != set1);

  if (NULL != set1) {
    set_null(set1, 10);

    for (i=0 ; i<10 ; ++i) {
      CU_TEST(FALSE == set_is_member(set1, i));       /* set should not contain member */
      set_change_member(set1, i, TRUE);               /* add member to set */
      CU_TEST(TRUE == set_is_member(set1, i));        /* set should contain member */
      set_change_member(set1, i, FALSE);              /* remove member from set */
      CU_TEST(FALSE == set_is_member(set1, i));       /* set should not contain member */
    }

    set_destroy(set1);
  }

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo set_test_list[] = {
  {"test_set", test_set},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_utilities_set", NULL, NULL, set_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_set(void)
{
  return CU_register_suites(suites);
}
