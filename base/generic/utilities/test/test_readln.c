/*
 *  Unit test functions for ASCEND: utilities/readln.c
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
#include <stdarg.h>
#include <utilities/ascConfig.h>
#ifdef __WIN32__
#include <io.h>
#endif
#include <utilities/ascMalloc.h>
#include <utilities/readln.h>
#include "CUnit/CUnit.h"
#include "test_readln.h"
#include "printutil.h"
#include "redirectStdStreams.h"

#define STR_LEN 100

static void test_readln(void)
{
  FILE *infile;
  char infilename[] = "test_readln.txt";
  char str1[STR_LEN];
  char str2[STR_LEN];
  char *pstr;
  long long_value;
  double double_value;
  int i_enabled_printing = FALSE;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

  /* this test requires message printing to be enabled */
  if (FALSE == test_printing_enabled()) {
    test_enable_printing();
    i_enabled_printing = TRUE;
  }

  /* test readln() */

  if (NULL != (infile = fopen(infilename, "w"))) {
    fclose(infile);
    infile = redirect_stdin(infilename);
    CU_TEST(-1 == readln(NULL, STR_LEN));             /* NULL str */
    rewind(infile);
    CU_TEST(0 == readln(str1, 0));                    /* max = 0 */
    rewind(infile);
    CU_TEST(-1 == readln(str1, STR_LEN));             /* read, but stream empty */
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "This is the string we expect back from readln()?\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    CU_TEST(-1 == readln(NULL, STR_LEN));             /* NULL str */
    rewind(infile);
    CU_TEST(0 == readln(str1, 0));                    /* max = 0 */
    rewind(infile);
    CU_TEST((int)(strlen(str2)-1) == readln(str1, STR_LEN)); /* read from stream */
    CU_TEST(0 == strncmp(str1, str2, strlen(str2)-1));
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "\nThis is the string we expect back from readln()?\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    CU_TEST(0 == readln(str1, 0));                    /* max = 0 */
    rewind(infile);
    CU_TEST(0 == readln(str1, STR_LEN));              /* read from stream with '\n' at start */
    CU_TEST(0 == strlen(str1));
    CU_TEST((int)(strlen(str2)-2) == readln(str1, STR_LEN));  /* read more from stream */
    CU_TEST(0 == strncmp(str1, str2+1, strlen(str2)-2));
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  /* test freadln() */

  if (NULL != (infile = fopen(infilename, "w+"))) {
    CU_TEST(-1 == freadln(NULL, STR_LEN, infile));    /* NULL str */
    rewind(infile);
    CU_TEST(0 == freadln(str1, 0, infile));           /* max = 0 */
    rewind(infile);
    CU_TEST(-1 == freadln(str1, STR_LEN, infile));    /* read, but stream empty */
    fclose(infile);
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w+"))) {
    snprintf(str2, STR_LEN, "This is the string we expect back from readln()?\n");
    fputs(str2, infile);
    rewind(infile);
    CU_TEST(-1 == freadln(NULL, STR_LEN, infile));    /* NULL str */
    rewind(infile);
    CU_TEST(0 == freadln(str1, 0, infile));           /* max = 0 */
    rewind(infile);
    CU_TEST((int)(strlen(str2)-1) == freadln(str1, STR_LEN, infile)); /* read from stream */
    CU_TEST(0 == strncmp(str1, str2, strlen(str2)-1));
    fclose(infile);
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w+"))) {
    snprintf(str2, STR_LEN, "\nThis is the string we expect back from readln()?\n");
    fputs(str2, infile);
    rewind(infile);
    CU_TEST(0 == freadln(str1, 0, infile));           /* max = 0 */
    rewind(infile);
    CU_TEST(0 == freadln(str1, STR_LEN, infile));     /* read from stream with '\n' at start */
    CU_TEST(0 == strlen(str1));
    CU_TEST((int)(strlen(str2)-2) == freadln(str1, STR_LEN, infile));  /* read more from stream */
    CU_TEST(0 == strncmp(str1, str2+1, strlen(str2)-2));
    fclose(infile);
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  /* test areadln() */

  if (NULL != (infile = fopen(infilename, "w"))) {
    fclose(infile);
    infile = redirect_stdin(infilename);
    pstr = areadln();                                 /* read from empty stream */
    CU_TEST(NULL == pstr);
    if (NULL != pstr)
      ascfree(pstr);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "This is the string we expect back from areadln()?\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    pstr = areadln();                                 /* read from typical stream */
    CU_TEST(NULL != pstr);
    CU_TEST((strlen(str2)-1) == strlen(pstr));
    CU_TEST(0 == strncmp(pstr, str2, strlen(str2)-1));
    if (NULL != pstr)
      ascfree(pstr);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "\nThis is the string we expect back from areadln()?\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    pstr = areadln();                                 /* read from stream with '\n' at start */
    CU_TEST(NULL != pstr)
    CU_TEST(0 == strlen(pstr));
    if (NULL != pstr)
      ascfree(pstr);
    pstr = areadln();                                 /* read more from stream */
    CU_TEST(NULL != pstr)
    CU_TEST((strlen(str2)-2) == strlen(pstr));
    CU_TEST(0 == strncmp(pstr, str2+1, strlen(str2)-2));
    if (NULL != pstr)
      ascfree(pstr);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  /* test afreadln() */

  if (NULL != (infile = fopen(infilename, "w+"))) {
    rewind(infile);
    pstr = afreadln(infile);                          /* read from empty stream */
    CU_TEST(NULL == pstr);
    fclose(infile);
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w+"))) {
    snprintf(str2, STR_LEN, "This is the string we expect back from readln()?\n");
    fputs(str2, infile);
    rewind(infile);
    pstr = afreadln(infile);                          /* read from typical stream */
    CU_TEST(NULL != pstr);
    CU_TEST((strlen(str2)-1) == strlen(pstr));
    CU_TEST(0 == strncmp(pstr, str2, strlen(str2)-1));
    if (NULL != pstr)
      ascfree(pstr);
    fclose(infile);
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w+"))) {
    snprintf(str2, STR_LEN, "\nThis is the string we expect back from readln()?\n");
    fputs(str2, infile);
    rewind(infile);
    pstr = afreadln(infile);                          /* read from stream with '\n' at start */
    CU_TEST(NULL != pstr);
    CU_TEST(0 == strlen(pstr));
    if (NULL != pstr)
      ascfree(pstr);
    pstr = afreadln(infile);                          /* read more from stream */
    CU_TEST(NULL != pstr)
    CU_TEST((strlen(str2)-2) == strlen(pstr));
    CU_TEST(0 == strncmp(pstr, str2+1, strlen(str2)-2));
    if (NULL != pstr)
      ascfree(pstr);
    fclose(infile);
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  /* test readlong() */

  if (NULL != (infile = fopen(infilename, "w"))) {
    fclose(infile);
    infile = redirect_stdin(infilename);
    long_value = readlong(123456);                    /* read from empty stream */
    CU_TEST(123456 == long_value);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "98765This is the string we expect back from areadln()?\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    long_value = readlong(0);                         /* read from typical stream */
    CU_TEST(98765 == long_value);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "\n-837\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    long_value = readlong(123456);                    /* read from stream with '\n' at start */
    CU_TEST(123456 == long_value);
    long_value = readlong(123456);                    /* read more from stream */
    CU_TEST(-837 == long_value);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  /* test readdouble() */

  if (NULL != (infile = fopen(infilename, "w"))) {
    fclose(infile);
    infile = redirect_stdin(infilename);
    double_value = readdouble(1.45734);               /* read from empty stream */
    CU_ASSERT_DOUBLE_EQUAL(1.45734, double_value, 0.00001);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "-3.5670384e199This is the string we expect back from areadln()?\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    double_value = readdouble(0.0);                   /* read from typical stream */
    CU_ASSERT_DOUBLE_EQUAL(-3.5670384e199, double_value, 0.00001);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (NULL != (infile = fopen(infilename, "w"))) {
    snprintf(str2, STR_LEN, "\n-642542146Good bye!\n");
    fputs(str2, infile);
    fclose(infile);
    infile = redirect_stdin(infilename);
    double_value = readdouble(0.0);                   /* read from stream with '\n' at start */
    CU_ASSERT_DOUBLE_EQUAL(0.0, double_value, 0.00001);
    double_value = readdouble(0.0);                   /* read more from stream */
    CU_ASSERT_DOUBLE_EQUAL(-642542146, double_value, 0.00001);
    reset_stdin();
  }
  else {
    CU_FAIL("Output file could not be opened in test_readln().");
  }

  if (TRUE == i_enabled_printing) {
    test_disable_printing();
  }

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo readln_test_list[] = {
  {"test_readln", test_readln},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_utilities_readln", NULL, NULL, readln_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_readln(void)
{
  return CU_register_suites(suites);
}
