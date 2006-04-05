/*
 *  Unit test functions for ASCEND: general/dstring.c
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

#include <string.h>
#include <CUnit/CUnit.h>

#include <utilities/ascConfig.h>
#include <general/dstring.h>
#include <utilities/ascMalloc.h>

#include "test_dstring.h"
#include <assertimpl.h>

static void test_dstring(void)
{
  Asc_DString ds1;
  const char NULL_STRING[] = "";
  const char str1[] = "This string doesn't have much to say.";
  char str_statsize_m1[ASC_DSTRING_STATIC_SIZE-1];
  char str_statsize[ASC_DSTRING_STATIC_SIZE];
  char str_statsize_p1[ASC_DSTRING_STATIC_SIZE+1];
  char str_statcat1[2*ASC_DSTRING_STATIC_SIZE];
  char str_statcat2[2*ASC_DSTRING_STATIC_SIZE+2];
  char *my_str = NULL;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();

  /* set up test strings */
  memset(&str_statsize_m1, '?', ASC_DSTRING_STATIC_SIZE-3);
  str_statsize_m1[ASC_DSTRING_STATIC_SIZE-2] = '\0';

  memset(&str_statsize, '=', ASC_DSTRING_STATIC_SIZE-2);
  str_statsize[ASC_DSTRING_STATIC_SIZE-1] = '\0';

  memset(&str_statsize_p1, '|', ASC_DSTRING_STATIC_SIZE-1);
  str_statsize_p1[ASC_DSTRING_STATIC_SIZE] = '\0';

  strcpy(str_statcat1, str_statsize_m1);
  strcat(str_statcat1, str_statsize);

  strcpy(str_statcat2, str_statsize);
  strcat(str_statcat2, str_statsize_p1);

#ifdef NDEBUG
  CU_FAIL("test_dstring() compiled with NDEBUG - some features not tested.");
#endif

  /* test Asc_DStrintInit(), Asc_DStringFree() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                 /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringInit(NULL);                /* error if dstring* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringFree(NULL);                /* error if dstring* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  /* Initialize the dstrings and test initial conditions */
  Asc_DStringInit(&ds1);

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  /* test Asc_DStringSet */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                 /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringSet(NULL, str1);           /* error if dstring* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringSet(&ds1, NULL);           /* error if string* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  Asc_DStringSet(&ds1, NULL_STRING);      /* empty string */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringSet(&ds1, str1);             /* regular empty string */

  CU_TEST(Asc_DStringLength(&ds1) == (int) strlen(str1));
  CU_TEST(strlen(str1) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), str1);

  Asc_DStringSet(&ds1, str_statsize_m1);   /* regular string, length near static size */

  CU_TEST(Asc_DStringLength(&ds1) == (int) strlen(str_statsize_m1));
  CU_TEST(strlen(str_statsize_m1) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), str_statsize_m1);

  Asc_DStringSet(&ds1, str_statsize);      /* regular string, length at static size */

  CU_TEST(Asc_DStringLength(&ds1) == (int) strlen(str_statsize));
  CU_TEST(strlen(str_statsize) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), str_statsize);

  Asc_DStringSet(&ds1, str_statsize_p1);   /* regular string, length near static size */

  CU_TEST(Asc_DStringLength(&ds1) == (int) strlen(str_statsize_p1));
  CU_TEST(strlen(str_statsize_p1) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), str_statsize_p1);

  Asc_DStringSet(&ds1, NULL_STRING);       /* empty string again */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  /* test Asc_DStringAppend */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                     /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringAppend(NULL, str1, 1);         /* error if dstring* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringAppend(&ds1, NULL, 1);         /* error if string* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                    /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  Asc_DStringFree(&ds1);                      /* free the string */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, NULL_STRING, 0);    /* empty string, length = 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, NULL_STRING, -10);  /* empty string, length < 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, NULL_STRING, 10);   /* empty string, length > strlen() */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, str1, 0);           /* regular str, length = 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, str1, 1);           /* regular str, length = 1 */

  CU_TEST(Asc_DStringLength(&ds1) == 1);
  CU_TEST(1 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), "T");

  Asc_DStringAppend(&ds1, str1, -1);          /* regular str, length < 0 */

  CU_TEST(Asc_DStringLength(&ds1) == ((int)strlen(str1) + 1));
  CU_TEST((strlen(str1) + 1) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1),
                         "TThis string doesn't have much to say.");

  Asc_DStringAppend(&ds1, str1, strlen(str1));  /* regular str, length = strlen() */

  CU_TEST(Asc_DStringLength(&ds1) == (2 * (int)strlen(str1) + 1));
  CU_TEST((2 * strlen(str1) + 1) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1),
                         "TThis string doesn't have much to say.This string doesn't have much to say.");

  Asc_DStringFree(&ds1);                     /* free the string */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, str_statsize_m1, -1); /* string near static size */
  Asc_DStringAppend(&ds1, str_statsize, -1);

  CU_TEST(Asc_DStringLength(&ds1) == (int) strlen(str_statcat1));
  CU_TEST(strlen(str_statcat1) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), str_statcat1);

  Asc_DStringFree(&ds1);                  /* free the string */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, str_statsize, -1); /* string near static size */
  Asc_DStringAppend(&ds1, str_statsize_p1, -1);

  CU_TEST(Asc_DStringLength(&ds1) == (int)strlen(str_statcat2));
  CU_TEST(strlen(str_statcat2) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), str_statcat2);

  Asc_DStringFree(&ds1);                  /* free the string */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringAppend(&ds1, "string 1", -1);  /* append string to itself */
  Asc_DStringAppend(&ds1, Asc_DStringValue(&ds1), -1);

  CU_TEST(Asc_DStringLength(&ds1) == (int)strlen("string 1string 1"));
  CU_TEST(strlen("string 1string 1") == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), "string 1string 1");

  /* test Asc_DStringTrunc */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                 /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringTrunc(NULL, 1);            /* error if dstring* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  Asc_DStringFree(&ds1);
  Asc_DStringTrunc(&ds1, 0);              /* empty dstring, length = 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringFree(&ds1);
  Asc_DStringTrunc(&ds1, 10);             /* empty dstring, length > 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringFree(&ds1);
  Asc_DStringTrunc(&ds1, -1);             /* empty dstring, length < 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringSet(&ds1, str1);
  Asc_DStringTrunc(&ds1, 0);              /* length = 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  Asc_DStringSet(&ds1, str1);
  Asc_DStringTrunc(&ds1, 10);             /* length > 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 10);
  CU_TEST(10 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), "This strin");

  Asc_DStringSet(&ds1, str1);
  Asc_DStringTrunc(&ds1, strlen(str1) + 10);  /* length > strlen() */

  CU_TEST(Asc_DStringLength(&ds1) == (int)strlen(str1));
  CU_TEST(strlen(str1) == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), str1);

  Asc_DStringSet(&ds1, str1);
  Asc_DStringTrunc(&ds1, -1);             /* length < 0 */

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  /* test Asc_DStringResult */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);                 /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    Asc_DStringResult(NULL);              /* error if dstring* is NULL */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);                /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  Asc_DStringFree(&ds1);
  my_str = Asc_DStringResult(&ds1);       /* empty string */

  CU_TEST(NULL != my_str);
  CU_TEST(0 == strlen(my_str));
  CU_ASSERT_STRING_EQUAL(my_str, NULL_STRING);

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  ascfree(my_str);

  Asc_DStringSet(&ds1, str_statsize_m1);
  my_str = Asc_DStringResult(&ds1);       /* string near static size */

  CU_TEST(NULL != my_str);
  CU_TEST(strlen(my_str) == strlen(str_statsize_m1));
  CU_ASSERT_STRING_EQUAL(my_str, str_statsize_m1);

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  ascfree(my_str);

  Asc_DStringSet(&ds1, str_statsize);
  my_str = Asc_DStringResult(&ds1);       /* string at static size */

  CU_TEST(NULL != my_str);
  CU_TEST(strlen(my_str) == strlen(str_statsize));
  CU_ASSERT_STRING_EQUAL(my_str, str_statsize);

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  ascfree(my_str);

  Asc_DStringSet(&ds1, str_statsize_p1);
  my_str = Asc_DStringResult(&ds1);       /* string above static size */

  CU_TEST(NULL != my_str);
  CU_TEST(strlen(my_str) == strlen(str_statsize_p1));
  CU_ASSERT_STRING_EQUAL(my_str, str_statsize_p1);

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  ascfree(my_str);

  /* free the dynamic strings */
  Asc_DStringFree(&ds1);

  CU_TEST(Asc_DStringLength(&ds1) == 0);
  CU_TEST(0 == strlen(Asc_DStringValue(&ds1)));
  CU_ASSERT_STRING_EQUAL(Asc_DStringValue(&ds1), NULL_STRING);

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo dstring_test_list[] = {
  {"test_dstring", test_dstring},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_dstring", NULL, NULL, dstring_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_dstring(void)
{
  return CU_register_suites(suites);
}
