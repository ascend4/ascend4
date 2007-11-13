/*
 *  Unit test functions for ASCEND: general/hashpjw.c
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
#include <general/hashpjw.h>
#include "CUnit/CUnit.h"
#include "assertimpl.h"

static void test_hashpjw(void)
{
  unsigned long hash1;
  unsigned long hash2;

  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();

    /* test hashpjw */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                 /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    hash1 = hashpjw(NULL, 1);             /* error - str NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    hash1 = hashpjw("string", 0);         /* error - size == 0 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  hash1 = hashpjw("", 1);                 /* empty string, minimum size */
  CU_TEST(1 > hash1);

  hash1 = hashpjw("", 500);               /* empty string */
  CU_TEST(500 > hash1);

  hash1 = hashpjw("string1", 500);        /* regular string */
  hash2 = hashpjw("string1", 500);
  CU_TEST(500 > hash1);
  CU_TEST(500 > hash2);
  CU_TEST(hash1 == hash2);

  hash2 = hashpjw("string1", 10);

  CU_TEST(10 > hash2);
  CU_TEST(hash1 != hash2);

  hash2 = hashpjw("string2", 500);

  CU_TEST(500 > hash2);
  CU_TEST(hash1 != hash2);

  hash2 = hashpjw("string2", 1);

  CU_TEST(1 > hash2);
  CU_TEST(hash1 != hash2);

  /* test hashpjw_int */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                 /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    hash1 = hashpjw_int(100, 0);          /* error - size == 0 */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  hash1 = hashpjw_int(0, 1);              /* minimum size */
  CU_TEST(1 > hash1);

  hash1 = hashpjw_int(0, 500);
  CU_TEST(500 > hash1);

  hash1 = hashpjw_int(100, 500);
  hash2 = hashpjw_int(100, 500);
  CU_TEST(500 > hash1);
  CU_TEST(500 > hash2);
  CU_TEST(hash1 == hash2);

  hash2 = hashpjw_int(100, 10);

  CU_TEST(10 > hash2);
  CU_TEST(hash1 != hash2);

  hash2 = hashpjw_int(-23464236, 500);

  CU_TEST(500 > hash2);
  CU_TEST(hash1 != hash2);

  hash2 = hashpjw_int(-23464236, 1);

  CU_TEST(1 > hash2);
  CU_TEST(hash1 != hash2);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo hashpjw_test_list[] = {
  {"hashpjw", test_hashpjw},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"general_hashpjw", NULL, NULL, hashpjw_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_hashpjw(void)
{
  return CU_register_suites(suites);
}
