/*
 *  Unit test functions for ASCEND: utilities/ascPanic.c
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
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
/* #include <compiler/redirectFile.h> */
#include "CUnit/CUnit.h"
#include "test_ascPanic.h"
#include "assertimpl.h"
#include "printutil.h"

static int f_callback_called = FALSE;
static int f_callback_status;

/*
	These tests have been disabled as the stream redirection in ASCEND is
	starting to be a bit crazy: error.c --> ascPrint.h --> redirectFile.h
	as well as printutil.c and redirectStdStreams.c. Will set this test to
	failing and we'll come back and reimplement this later.
*/

static void test_ascPanic(void){
	CU_FAIL("test_ascPanic needs to be reimplemented");
}

#ifdef REIMPLEMENT_STREAMS

/*
 *  Callback function for Asc_Panic() during testing.
 *  Sets a flag, then returns non-zero so program continues.
 */
static int set_flag_and_return(int status)
{
  f_callback_called = TRUE;
  f_callback_status = status;
  return TRUE;
}

/*
 *  ascPanic.[ch] is tough to test completely, since
 *    1. most of its action is text formatting & output
 *    2. it normally exits the program
 *
 *  As a first pass, we will disable exiting and just test
 *  whether something is getting written to ASCERR and an
 *  output file.
 */
static void test_ascPanic(void)
{
  FILE *stdoutfile;
  FILE* outfile;
  int i_enabled_printing = FALSE;
  FILE *old_errfile;
  FILE *old_warnfile;
  FILE *old_infofile;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

#ifdef NDEBUG
  CU_FAIL("test_ascPanic() compiled with NDEBUG - some features not tested.");
#endif

  old_errfile = g_ascend_errors;                  /* save so can be restored later */
  old_warnfile = g_ascend_warnings;
  old_infofile = g_ascend_information;

  /* this test requires message printing to be enabled */
  if (FALSE == test_printing_enabled()) {
    test_enable_printing();
    i_enabled_printing = TRUE;
  }

  CU_TEST(NULL == Asc_PanicSetCallback(set_flag_and_return));
  Asc_PanicDisplayMessageBox(FALSE);

#ifndef NDEBUG
  enable_assert_longjmp(TRUE);                        /* prepare to test assertions */

  Asc_RedirectCompilerStreams(NULL, NULL, NULL);      /* make sure ASCERR is NULL */

  f_callback_called = FALSE;
  g_assert_status = ast_passed;
  if (0 == setjmp(g_asc_test_env))
    Asc_Panic(1, "test_ascPanic", "Error message.");  /* error - ASCERR NULL */
  CU_TEST(ast_failed == g_assert_status);
  CU_TEST(FALSE == f_callback_called);

  enable_assert_longjmp(FALSE);                       /* done testing assertions */
#endif    /* !NDEBUG */

  if (NULL != (stdoutfile = fopen("asc_panic_std_tempfile1.txt", "w+"))) {
    Asc_RedirectCompilerStreams(stdoutfile, stdoutfile, stdoutfile); /* send output to a file */
    Asc_Panic(1, "test_ascPanic", "Error message 1.");
    CU_TEST(1 == f_callback_status);
    Asc_Panic(2, "test_ascPanic", "Error message 2.");
    CU_TEST(2 == f_callback_status);
    rewind(stdoutfile);
    CU_TEST(EOF != fgetc(stdoutfile));                /* test that file is not empty */
    fclose(stdoutfile);
    remove("asc_panic_std_tempfile1.txt");
    Asc_RedirectCompilerDefault();                    /* reset reporting streams  */
  }
  else {
    CU_FAIL("Error opening output file 1 in test_ascPanic.c");
  }

  Asc_PanicSetOutfile("asc_panic_extra_tempfile1.txt"); /* turn on output to extra file */

  if (NULL != (stdoutfile = fopen("asc_panic_std_tempfile2.txt", "w+"))) {
    Asc_RedirectCompilerStreams(stdoutfile, stdoutfile, stdoutfile); /* send output to a file */
    Asc_Panic(-1, "test_ascPanic", "Error message %d.", -1);
    CU_TEST(-1 == f_callback_status);
    Asc_Panic(200, "test_ascPanic", "Error message %d.", 200);
    CU_TEST(200 == f_callback_status);
    rewind(stdoutfile);
    CU_TEST(EOF != fgetc(stdoutfile));                /* test that file is not empty */
    fclose(stdoutfile);
    remove("asc_panic_std_tempfile2.txt");
    Asc_RedirectCompilerDefault();                    /* reset reporting streams  */
    if (NULL != (outfile = fopen("asc_panic_extra_tempfile1.txt", "r"))) {
      CU_TEST(EOF != fgetc(outfile));                /* test that extra file is not empty */
      fclose(outfile);
      remove("asc_panic_extra_tempfile1.txt");
    }
    else {
      CU_FAIL("Error opening output file 3 in test_ascPanic.c");
    }
  }
  else {
    CU_FAIL("Error opening output file 2 in test_ascPanic.c");
  }

  Asc_PanicSetOutfile(NULL);                          /* turn off output to extra file */

  if (NULL != (stdoutfile = fopen("asc_panic_std_tempfile3.txt", "w+"))) {
    Asc_RedirectCompilerStreams(stdoutfile, stdoutfile, stdoutfile); /* send output to a file */
    Asc_Panic(-100, "test_ascPanic", "Error message -100.");
    CU_TEST(-100 == f_callback_status);
    Asc_Panic(0, "test_ascPanic", "Error message 0.");
    CU_TEST(0 == f_callback_status);
    rewind(stdoutfile);
    CU_TEST(EOF != fgetc(stdoutfile));                /* test that file is not empty */
    fclose(stdoutfile);
    remove("asc_panic_std_tempfile3.txt");
    Asc_RedirectCompilerDefault();                    /* reset reporting streams  */
  }
  else {
    CU_FAIL("Error opening output file 4 in test_ascPanic.c");
  }

  CU_TEST(set_flag_and_return == Asc_PanicSetCallback(NULL));   /* reset exit on Asc_Panic() */
  Asc_PanicDisplayMessageBox(TRUE);                   /* reset display of the MessageBox */

  if (TRUE == i_enabled_printing) {
    test_disable_printing();
  }

  Asc_RedirectCompilerStreams(old_errfile, old_warnfile, old_infofile); /* restore streams */
  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}
#endif /* REIMPLEMENT_STREAMS */

/*===========================================================================*/
/* Registration information */

static CU_TestInfo ascPanic_test_list[] = {
  {"test_ascPanic", test_ascPanic},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_utilities_ascPanic", NULL, NULL, ascPanic_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_ascPanic(void)
{
  return CU_register_suites(suites);
}
