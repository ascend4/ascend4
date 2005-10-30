/*
 *  Unit test functions for ASCEND: general/listio.c
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
#include <io.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "general/listio.h"
#include "CUnit/CUnit.h"
#include "assertimpl.h"
#include "test_listio.h"
#include "redirectStdStreams.h"

/*
 *  listio.[ch] implements writing of a list's details to file.
 *  Because there is no specification on the file contents, this
 *  test routine does not do more than check boundary conditions
 *  and make sure something is written to a specified file stream.
 */
static void test_listio(void)
{
  struct gl_list_t *p_list1;
  unsigned long *pint_array[20];
  unsigned long i;
  FILE *file_normal;
  FILE *file_stderr;
  int i_initialized_lists = FALSE;
  unsigned long prior_meminuse;

  prior_meminuse = ascmeminuse();       /* save meminuse() at start of test function */

#ifdef NDEBUG
  CU_FAIL("test_listio() compiled with NDEBUG - some features not tested.");
#endif

  /* set up pooling & recycling */
  if (FALSE == gl_pool_initialized()) {
    gl_init();
    gl_init_pool();
    i_initialized_lists = TRUE;
  }

  for (i=0 ; i<20 ; ++i) {              /* create some test data */
      pint_array[i] = (unsigned long*)ascmalloc(sizeof(unsigned long));
      *pint_array[i] = i;
  }

  p_list1 = gl_create(20);              /* create and fill a list with data */
  for (i=0 ; i<20 ; ++i) {
      gl_append_ptr(p_list1, pint_array[i]);
  }

  /* test gl_write_list() */

#ifndef ASC_NO_ASSERTIONS
  asc_assert_catch(TRUE);          /* prepare to test assertions */

  asc_assert_reset();
  if (0 == setjmp(g_asc_test_env))
    gl_write_list(NULL, NULL);          /* error if NULL list* */
  CU_TEST(TRUE == asc_assert_failed());

  asc_assert_catch(FALSE);         /* done testing assertions */
#endif    /* !ASC_NO_ASSERTIONS */

  if (NULL != (file_normal = fopen("listiotempfile.tmp", "w+"))) {

    gl_write_list(file_normal, p_list1);/* write to normal open file */
    rewind(file_normal);
    CU_TEST(EOF != fgetc(file_normal)); /* test that file is not empty */
    fclose(file_normal);
  }
  else {
    CU_FAIL("Error opening output file 1 in test_listio.c");
  }

  if (NULL != (file_stderr = redirect_stderr("listiotempstderr.tmp"))) {

    gl_write_list(NULL, p_list1);       /* write to stderr */
    rewind(file_stderr);

    if (NULL != (file_normal = fopen("listiotempfile.tmp", "r"))) {
      int ch1 = fgetc(file_stderr);
      int ch2 = fgetc(file_normal);
      while ((EOF != ch1) && (EOF != ch2)) {  /* test that files are the same */
        if (ch1 != ch2) {
          CU_FAIL("Files differ in test_listio.");
          break;
        }
        ch1 = fgetc(file_stderr);
        ch2 = fgetc(file_normal);
      }
      if (ch1 == ch2) {
        CU_PASS("Files are the same in test_listio.");
      }
      fclose(file_normal);
    }
    reset_stderr();
    remove("listiotempstderr.tmp");
  }
  else {
    CU_FAIL("Error opening output file 2 in test_listio.c");
  }
  remove("listiotempfile.tmp");

  gl_destroy(p_list1);                  /* clean up the list, preserving data */

  /* clean up and exit */
  for (i=0 ; i<20 ; ++i)
    ascfree(pint_array[i]);
 
  if (TRUE == i_initialized_lists) {
    gl_destroy_pool();
  }
  
  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo list_test_listio[] = {
  {"test_listio", test_listio},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_general_listio", NULL, NULL, list_test_listio},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_general_listio(void)
{
  return CU_register_suites(suites);
}
