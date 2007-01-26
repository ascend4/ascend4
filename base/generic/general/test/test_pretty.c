/*
 *  Unit test functions for ASCEND: general/pretty.c
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
#include <general/pretty.h>
#include "CUnit/CUnit.h"
#include "printutil.h"

#define MAX_STRING_LEN 100

static void test_pretty(void)
{
  FILE *file;
  int chr;
  int i;
  int i_enabled_printing = FALSE;

  char str1[MAX_STRING_LEN];
  char str1_4_0[MAX_STRING_LEN];
  char str1_8_0[MAX_STRING_LEN];
  char str1_12_0[MAX_STRING_LEN];
  char str1_20_5[MAX_STRING_LEN];

  char str2[MAX_STRING_LEN];
  char str2_15_0[MAX_STRING_LEN];
  char str2_20_5[MAX_STRING_LEN];

  char str3[MAX_STRING_LEN];
  char str3_0[MAX_STRING_LEN];
  char str3_5[MAX_STRING_LEN];
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

  /* can't use constant strings, i.e. can't assign in declaration (on gcc/mingw, anyway) */
  strcpy(str1, "This is a string that might be broken into multiple lines.");
  strcpy(str1_4_0, "This\nis a\nstring\nthat\nmight\nbe\nbroken\ninto\nmultiple\nlines.\n");
  strcpy(str1_8_0, "This is\na string\nthat\nmight be\nbroken\ninto\nmultiple\nlines.\n");
  strcpy(str1_12_0, "This is a\nstring that\nmight be\nbroken into\nmultiple\nlines.\n");
  strcpy(str1_20_5, "     This is a string\n     that might be broken\n     into multiple lines.\n");

  strcpy(str2, "This is a string that might be broken into multiple lines.\n");
  strcpy(str2_15_0, "This is a\nstring that\nmight be broken\ninto multiple\nlines.\n\n");
  strcpy(str2_20_5, "     This is a string\n     that might be broken\n     into multiple lines.\n");

  strcpy(str3, "This is a string/*EOL*/ that might be broken /*EOL*/into multiple lines./*EOL*/");
  strcpy(str3_0, "This is a string\n that might be broken \ninto multiple lines.\n");
  strcpy(str3_5, "     This is a string\n      that might be broken \n     into multiple lines.\n");

  /* this test requires message printing to be enabled */
  if (FALSE == test_printing_enabled()) {
    test_enable_printing();
    i_enabled_printing = TRUE;
  }

  /* test print_long_string() */

  CU_TEST(0 == print_long_string(NULL, str1, 8, 0));  /*  NULL pointers ok, but no output */
  CU_TEST(0 == print_long_string(stdout, NULL, 8, 0));

  if (NULL != (file = fopen("testpretty1.txt", "w+"))) {
    CU_TEST((int)strlen(str1_4_0) == print_long_string(file, str1, 4, 0));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str1_4_0))) {
      if (chr != str1_4_0[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty1.txt");
  }

  if (NULL != (file = fopen("testpretty2.txt", "w+"))) {
    CU_TEST((int)strlen(str1_8_0) == print_long_string(file, str1, 8, 0));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str1_8_0))) {
      if (chr != str1_8_0[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty2.txt");
  }

  if (NULL != (file = fopen("testpretty3.txt", "w+"))) {
    CU_TEST((int)strlen(str1_12_0) == print_long_string(file, str1, 12, 0));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str1_12_0))) {
      if (chr != str1_12_0[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty3.txt");
  }

  if (NULL != (file = fopen("testpretty4.txt", "w+"))) {
    CU_TEST((int)strlen(str1_20_5) == print_long_string(file, str1, 20, 5));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str1_20_5))) {
      if (chr != str1_20_5[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty4.txt");
  }

  if (NULL != (file = fopen("testpretty5.txt", "w+"))) {
    CU_TEST((int)strlen(str2_15_0) == print_long_string(file, str2, 15, 0));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str2_15_0))) {
      if (chr != str2_15_0[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty5.txt");
  }

  if (NULL != (file = fopen("testpretty6.txt", "w+"))) {
    CU_TEST((int)strlen(str2_20_5) == print_long_string(file, str2, 20, 5));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str2_20_5))) {
      if (chr != str2_20_5[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty6.txt");
  }

  /* test print_long_string_EOL() */

  CU_TEST(0 == print_long_string_EOL(NULL, str1, 0));  /*  NULL pointers ok, but no output */
  CU_TEST(0 == print_long_string_EOL(stdout, NULL, 0));

  if (NULL != (file = fopen("testpretty7.txt", "w+"))) {
    CU_TEST((int)strlen(str3_0) == print_long_string_EOL(file, str3, 0));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str3_0))) {
      if (chr != str3_0[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty7.txt");
  }

  if (NULL != (file = fopen("testpretty8.txt", "w+"))) {
    CU_TEST((int)strlen(str3_5) == print_long_string_EOL(file, str3, 5));
    rewind(file);
    chr = fgetc(file);
    i = 0;
    while ((EOF != chr) && (i < (int)strlen(str3_5))) {
      if (chr != str3_5[i]) {
        CU_FAIL("String and file are not equal in test_pretty().");
        i = -1;
        break;
      }
      chr = fgetc(file);
      ++i;
    }
    if (i >= 0) {
      CU_PASS("String and file are equal in test_pretty().");
    }
    fclose(file);
    remove("testpretty8.txt");
  }

  if (TRUE == i_enabled_printing) {
    test_disable_printing();
  }

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo list_test_pretty[] = {
  {"test_pretty", test_pretty},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_pretty", NULL, NULL, list_test_pretty},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_pretty(void)
{
  return CU_register_suites(suites);
}
