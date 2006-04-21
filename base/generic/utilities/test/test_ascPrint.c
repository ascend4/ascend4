/*
 *  Unit test functions for ASCEND: utilities/ascPrint.c
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
#ifdef __WIN32__
#include <io.h>
#endif
#include <stdarg.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPrint.h>
#include "CUnit/CUnit.h"
#include "test_ascPrint.h"
#include "printutil.h"

#undef STR_LEN
#define STR_LEN 4096

/* flag for whether test_print1() was called. */
static int test_print1_called = FALSE;
/* static string to hold result of call to test_print1() */
static char test_print1_str[STR_LEN];
/* static FILE * to hold result of call to test_print2() */
static FILE *test_print1_fp;
/* print function for test_ascPrint_vtable1 */
static int test_print1(FILE *fp, CONST char *format, va_list args)
{
  int result;

  test_print1_called = TRUE;
  test_print1_fp = fp;
  result = vsnprintf(test_print1_str, STR_LEN, format, args);
  test_print1_str[STR_LEN-1] = '\0';
  return result;
}

/* flag for whether test_flush1() was called. */
static int test_flush1_called = FALSE;
/* static FILE * to hold result of call to test_flush1() */
static FILE *test_flush1_fp;
/* flush function for test_ascPrint_vtable1 */
static int test_flush1(FILE *fp)
{
  test_flush1_fp = fp;
  test_flush1_called = TRUE;
  return 0;
}

/*
 *  Vtable for testing of ascPrint.
 *  print and flush point to special functions which record the
 *  fact and details of their invocation using static variables.
 */
static struct Asc_PrintVTable f_test_ascPrint_vtable1 = { "test_vtable1", test_print1, test_flush1, NULL };

/* flag for whether test_print2() was called. */
static int test_print2_called = FALSE;
/* static string to hold result of call to test_print2() */
static char test_print2_str[STR_LEN];
/* static FILE * to hold result of call to test_print2() */
static FILE *test_print2_fp;
/* print function for test_ascPrint_vtable2 */
static int test_print2(FILE *fp, CONST char *format, va_list args)
{
  int result;

  test_print2_called = TRUE;
  test_print2_fp = fp;
  result = vsnprintf(test_print2_str, STR_LEN, format, args);
  test_print2_str[STR_LEN-1] = '\0';
  return result;
}

/* flag for whether test_flush2() was called. */
static int test_flush2_called = FALSE;
/* static FILE * to hold result of call to test_flush2() */
static FILE *test_flush2_fp;
/* flush function for test_ascPrint_vtable1 */
static int test_flush2(FILE *fp)
{
  test_flush2_fp = fp;
  test_flush2_called = TRUE;
  return 0;
}

/*
 *  Another vtable for testing of ascPrint.
 *  print and flush point to special functions which record the
 *  fact and details of their invocation using static variables.
 */
static struct Asc_PrintVTable f_test_ascPrint_vtable2 = { "test_vtable2", test_print2, test_flush2, NULL };

static struct Asc_PrintVTable f_bad_vtable1 = { NULL, test_print1, test_flush1, NULL };
static struct Asc_PrintVTable f_bad_vtable2 = { "Bad_vtable", NULL, test_flush1, NULL };
static struct Asc_PrintVTable f_bad_vtable3 = { "Bad_vtable", test_print1, NULL, NULL };
static struct Asc_PrintVTable f_bad_vtable4 = { "Bad_vtable", test_print1, test_flush1, &f_bad_vtable1 };

/*
 *  We direct output to to special functions that keep track of whether
 *  they were called and with which parameters.  This allows checking
 *  the marshalling of output via the registered vtables.
 */
static void test_ascPrint(void)
{
  char str[STR_LEN];
  int nchars;
  int i_disabled_printing = FALSE;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

  if (test_printing_enabled()) {    /* turn off printing (i.e. remove global vtable) */
    i_disabled_printing = TRUE;
    test_disable_printing();
  }

  /* test Asc_PrintPushVTable() */

  CU_TEST(1 == Asc_PrintPushVTable(NULL));                /* error - NULL vtable */
  CU_TEST(1 == Asc_PrintPushVTable(&f_bad_vtable1));      /* error - NULL name */
  CU_TEST(1 == Asc_PrintPushVTable(&f_bad_vtable2));      /* error - NULL print */
  CU_TEST(1 == Asc_PrintPushVTable(&f_bad_vtable3));      /* error - NULL flush */
  CU_TEST(1 == Asc_PrintPushVTable(&f_bad_vtable4));      /* error - non-NULL next */
  CU_TEST(FALSE == Asc_PrintHasVTable("Bad_vtable"));
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable1"));
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable2"));

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable1));  /* push a normal vtable */
  CU_TEST(TRUE == Asc_PrintHasVTable("test_vtable1"));
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable2"));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  Asc_Printf("This is test string #1.");                /* print - should only go to 1st vtable */
  CU_TEST(TRUE == test_print1_called);
  CU_TEST(0 == strcmp(test_print1_str, "This is test string #1."));
  CU_TEST(stdout == test_print1_fp);
  CU_TEST(FALSE == test_print2_called);
  CU_TEST(test_print2_str[0] == '\0');
  CU_TEST(NULL == test_print2_fp);

  test_flush1_called = FALSE;
  test_flush1_fp = NULL;
  test_flush2_called = FALSE;
  test_flush2_fp = NULL;

  Asc_FFlush(stdout);                                   /* flush - should only go to 1st vtable */
  CU_TEST(TRUE == test_flush1_called);
  CU_TEST(stdout == test_flush1_fp);
  CU_TEST(FALSE == test_flush2_called);
  CU_TEST(NULL == test_flush2_fp);

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));  /* push another vtable */
  CU_TEST(TRUE == Asc_PrintHasVTable("test_vtable1"));
  CU_TEST(TRUE == Asc_PrintHasVTable("test_vtable2"));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  Asc_Printf("This is test string #2.");                /* print - should go to both vtables */
  CU_TEST(TRUE == test_print1_called);
  CU_TEST(0 == strcmp(test_print1_str, "This is test string #2."));
  CU_TEST(stdout == test_print1_fp);
  CU_TEST(TRUE == test_print2_called);
  CU_TEST(0 == strcmp(test_print2_str, "This is test string #2."));
  CU_TEST(stdout == test_print2_fp);

  test_flush1_called = FALSE;
  test_flush1_fp = NULL;
  test_flush2_called = FALSE;
  test_flush2_fp = NULL;

  Asc_FFlush(stdout);                                   /* flush - should go to both vtables */
  CU_TEST(TRUE == test_flush1_called);
  CU_TEST(stdout == test_flush1_fp);
  CU_TEST(TRUE == test_flush2_called);
  CU_TEST(stdout == test_flush2_fp);

  CU_TEST(&f_test_ascPrint_vtable1 == Asc_PrintRemoveVTable("test_vtable1")); /* remove a vtable */
  f_test_ascPrint_vtable1.next = NULL;
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable1"));
  CU_TEST(TRUE == Asc_PrintHasVTable("test_vtable2"));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  Asc_Printf("This is test string last.");              /* print - should only go to 2nd vtable */
  CU_TEST(FALSE == test_print1_called);
  CU_TEST(test_print1_str[0] == '\0');
  CU_TEST(NULL == test_print1_fp);
  CU_TEST(TRUE == test_print2_called);
  CU_TEST(0 == strcmp(test_print2_str, "This is test string last."));
  CU_TEST(stdout == test_print2_fp);

  test_flush1_called = FALSE;
  test_flush1_fp = NULL;
  test_flush2_called = FALSE;
  test_flush2_fp = NULL;

  Asc_FFlush(stdout);                                   /* flush - should only go to 2nd vtable */
  CU_TEST(FALSE == test_flush1_called);
  CU_TEST(NULL == test_flush1_fp);
  CU_TEST(TRUE == test_flush2_called);
  CU_TEST(stdout == test_flush2_fp);

  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2")); /* remove a vtable */
  f_test_ascPrint_vtable2.next = NULL;
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable1"));
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable2"));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  Asc_Printf("No one should get this one.");            /* print - no printing should occur */
  CU_TEST(FALSE == test_print1_called);
  CU_TEST(test_print1_str[0] == '\0');
  CU_TEST(NULL == test_print1_fp);
  CU_TEST(FALSE == test_print2_called);
  CU_TEST(test_print2_str[0] == '\0');
  CU_TEST(NULL == test_print2_fp);

  test_flush1_called = FALSE;
  test_flush1_fp = NULL;
  test_flush2_called = FALSE;
  test_flush2_fp = NULL;

  Asc_FFlush(stdout);                                   /* flush - no flushing should occur */
  CU_TEST(FALSE == test_flush1_called);
  CU_TEST(NULL == test_flush1_fp);
  CU_TEST(FALSE == test_flush2_called);
  CU_TEST(NULL == test_flush2_fp);

  /* test Asc_PrintRemoveVTable() */
  /* basic functionality tested in other sections - focus on error conditions */

  CU_TEST(NULL == Asc_PrintRemoveVTable(NULL));          /* error - NULL name */
  CU_TEST(NULL == Asc_PrintRemoveVTable("test_vtable1"));/* name not registered */
  CU_TEST(NULL == Asc_PrintRemoveVTable("test_vtable2"));/* name not registered */

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));  /* push a vtable */
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable1"));
  CU_TEST(TRUE == Asc_PrintHasVTable("test_vtable2"));
  CU_TEST(NULL == Asc_PrintRemoveVTable(NULL));          /* error - NULL name */
  CU_TEST(NULL == Asc_PrintRemoveVTable("test_vtable1"));/* name not registered */
  CU_TEST(NULL == Asc_PrintRemoveVTable(""));            /* name empty (not registered) */
  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2"));  /* normal removal */
  f_test_ascPrint_vtable2.next = NULL;
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable1"));
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable2"));

  /* test Asc_PrintHasVTable() */
  /* basic functionality tested in other sections - focus on error conditions */

  CU_TEST(FALSE == Asc_PrintHasVTable(NULL));           /* error - NULL name */
  CU_TEST(FALSE == Asc_PrintHasVTable(""));             /* empty name */
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable1")); /* name not registered */
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable2")); /* name not registered */

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable1));  /* push vtables */
  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));

  CU_TEST(FALSE == Asc_PrintHasVTable(NULL));           /* error - NULL name */
  CU_TEST(FALSE == Asc_PrintHasVTable(""));             /* empty name */
  CU_TEST(TRUE == Asc_PrintHasVTable("test_vtable1"));  /* name registered */
  CU_TEST(TRUE == Asc_PrintHasVTable("test_vtable2"));  /* name registered */

  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2"));  /* normal removal */
  CU_TEST(&f_test_ascPrint_vtable1 == Asc_PrintRemoveVTable("test_vtable1"));  /* normal removal */
  f_test_ascPrint_vtable1.next = NULL;
  f_test_ascPrint_vtable2.next = NULL;

  CU_TEST(FALSE == Asc_PrintHasVTable(NULL));           /* error - NULL name */
  CU_TEST(FALSE == Asc_PrintHasVTable(""));             /* empty name */
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable1")); /* name not registered */
  CU_TEST(FALSE == Asc_PrintHasVTable("test_vtable2")); /* name not registered */

  /* test Asc_Printf() */

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  nchars = snprintf(str, STR_LEN, "%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12);
  CU_TEST(0 == Asc_Printf("%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12)); /* no vtables registered */
  CU_TEST(FALSE == test_print1_called);
  CU_TEST('\0' == test_print1_str[0]);
  CU_TEST(NULL == test_print1_fp);
  CU_TEST(FALSE == test_print2_called);
  CU_TEST('\0' == test_print2_str[0]);
  CU_TEST(NULL == test_print2_fp);

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable1));  /* push vtables */
  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  nchars = snprintf(str, STR_LEN, "%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12);
  CU_TEST(nchars == Asc_Printf("%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12)); /* print something */
  CU_TEST(TRUE == test_print1_called);
  CU_TEST(0 == strcmp(test_print1_str, str));
  CU_TEST(stdout == test_print1_fp);
  CU_TEST(TRUE == test_print2_called);
  CU_TEST(0 == strcmp(test_print2_str, str));
  CU_TEST(stdout == test_print2_fp);

  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2")); /* cleanup */
  CU_TEST(&f_test_ascPrint_vtable1 == Asc_PrintRemoveVTable("test_vtable1"));
  f_test_ascPrint_vtable1.next = NULL;
  f_test_ascPrint_vtable2.next = NULL;

  /* test Asc_FPrintf() */

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  nchars = snprintf(str, STR_LEN, "%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12);
  CU_TEST(0 == Asc_FPrintf((FILE *)10, "%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12)); /* no vtables registered */
  CU_TEST(FALSE == test_print1_called);
  CU_TEST('\0' == test_print1_str[0]);
  CU_TEST(NULL == test_print1_fp);
  CU_TEST(FALSE == test_print2_called);
  CU_TEST('\0' == test_print2_str[0]);
  CU_TEST(NULL == test_print2_fp);

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable1));  /* push vtables */
  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  memset(str, '?', STR_LEN-1);
  str[STR_LEN-1] = '\0';
  CU_TEST(STR_LEN-1 == Asc_FPrintf(stderr, "%s", str));/* print something */
  CU_TEST(TRUE == test_print1_called);
  CU_TEST(0 == strcmp(test_print1_str, str));
  CU_TEST(stderr == test_print1_fp);
  CU_TEST(TRUE == test_print2_called);
  CU_TEST(0 == strcmp(test_print2_str, str));
  CU_TEST(stderr == test_print2_fp);

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  nchars = snprintf(str, STR_LEN, "%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12);
  CU_TEST(nchars == Asc_FPrintf((FILE *)10, "%s%d%c%s%f", "my_float[", 7, ']', " = ", 8.12)); /* print something */
  CU_TEST(TRUE == test_print1_called);
  CU_TEST(0 == strcmp(test_print1_str, str));
  CU_TEST((FILE *)10 == test_print1_fp);
  CU_TEST(TRUE == test_print2_called);
  CU_TEST(0 == strcmp(test_print2_str, str));
  CU_TEST((FILE *)10 == test_print2_fp);

  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2")); /* cleanup */
  CU_TEST(&f_test_ascPrint_vtable1 == Asc_PrintRemoveVTable("test_vtable1"));
  f_test_ascPrint_vtable1.next = NULL;
  f_test_ascPrint_vtable2.next = NULL;

  /* test Asc_FFlush() */

  test_flush1_called = FALSE;
  test_flush1_fp = NULL;
  test_flush2_called = FALSE;
  test_flush2_fp = NULL;

  CU_TEST(0 == Asc_FFlush(stderr));                      /* no vtables registered */
  CU_TEST(FALSE == test_flush1_called);
  CU_TEST(NULL == test_flush1_fp);
  CU_TEST(FALSE == test_flush2_called);
  CU_TEST(NULL == test_flush2_fp);

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable1));  /* push vtables */
  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));

  test_flush1_called = FALSE;
  test_flush1_fp = NULL;
  test_flush2_called = FALSE;
  test_flush2_fp = NULL;

  CU_TEST(0 == Asc_FFlush((FILE *)100));                /* flush a (FILE *) */
  CU_TEST(TRUE == test_flush1_called);
  CU_TEST((FILE *)100 == test_flush1_fp);
  CU_TEST(TRUE == test_flush2_called);
  CU_TEST((FILE *)100 == test_flush2_fp);

  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2")); /* cleanup */
  CU_TEST(&f_test_ascPrint_vtable1 == Asc_PrintRemoveVTable("test_vtable1"));
  f_test_ascPrint_vtable1.next = NULL;
  f_test_ascPrint_vtable2.next = NULL;

  /* test Asc_FPutc() */

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  CU_TEST(0 == Asc_FPutc('Y', stdout));                 /* no vtables registered */
  CU_TEST(FALSE == test_print1_called);
  CU_TEST('\0' == test_print1_str[0]);
  CU_TEST(NULL == test_print1_fp);
  CU_TEST(FALSE == test_print2_called);
  CU_TEST('\0' == test_print2_str[0]);
  CU_TEST(NULL == test_print2_fp);

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable1));  /* push vtables */
  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  CU_TEST(0 != Asc_FPutc('Y', stderr));                 /* print a char */
  CU_TEST(TRUE == test_print1_called);
  CU_TEST(0 == strcmp("Y", test_print1_str));
  CU_TEST(stderr == test_print1_fp);
  CU_TEST(TRUE == test_print2_called);
  CU_TEST(0 == strcmp("Y", test_print2_str));
  CU_TEST(stderr == test_print2_fp);

  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2")); /* cleanup */
  CU_TEST(&f_test_ascPrint_vtable1 == Asc_PrintRemoveVTable("test_vtable1"));
  f_test_ascPrint_vtable1.next = NULL;
  f_test_ascPrint_vtable2.next = NULL;

  /* test Asc_FPutc() */

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  CU_TEST(0 == Asc_Putchar('N'));                       /* no vtables registered */
  CU_TEST(FALSE == test_print1_called);
  CU_TEST('\0' == test_print1_str[0]);
  CU_TEST(NULL == test_print1_fp);
  CU_TEST(FALSE == test_print2_called);
  CU_TEST('\0' == test_print2_str[0]);
  CU_TEST(NULL == test_print2_fp);

  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable1));  /* push vtables */
  CU_TEST(0 == Asc_PrintPushVTable(&f_test_ascPrint_vtable2));

  test_print1_called = FALSE;
  test_print1_str[0] = '\0';
  test_print1_fp = NULL;
  test_print2_called = FALSE;
  test_print2_str[0] = '\0';
  test_print2_fp = NULL;

  CU_TEST(1 == Asc_Putchar('N'));                       /* print a char */
  CU_TEST(TRUE == test_print1_called);
  CU_TEST(0 == strcmp("N", test_print1_str));
  CU_TEST(stdout == test_print1_fp);
  CU_TEST(TRUE == test_print2_called);
  CU_TEST(0 == strcmp("N", test_print2_str));
  CU_TEST(stdout == test_print2_fp);

  CU_TEST(&f_test_ascPrint_vtable2 == Asc_PrintRemoveVTable("test_vtable2")); /* cleanup */
  CU_TEST(&f_test_ascPrint_vtable1 == Asc_PrintRemoveVTable("test_vtable1"));
  f_test_ascPrint_vtable1.next = NULL;
  f_test_ascPrint_vtable2.next = NULL;

  if (TRUE == i_disabled_printing) {    /* restore global vtable if necessary */
    test_enable_printing();
  }

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo ascPrint_test_list[] = {
  {"test_ascPrint", test_ascPrint},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_utilities_ascPrint", NULL, NULL, ascPrint_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_ascPrint(void)
{
  return CU_register_suites(suites);
}
