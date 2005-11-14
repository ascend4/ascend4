/*
 *  Unit test functions for ASCEND: utilities/ascDynaLoad.c
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
#ifdef __WIN32__
#include <io.h>
#endif
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascDynaLoad.h"
#include "CUnit/CUnit.h"
#include "test_ascDynaLoad.h"
#include "test_ascDynaLoad_shlib.h"

/*
 *  ascDynaLoad.[ch] has several different platform-dependent
 *  implementations.  This test function only tests the platform it
 *  is being run on.  At present, testing is not implemented for all
 *  ASCEND platforms.
 *
 *  This test function attempts to open and execute a shared library
 *  whose path is defined in variable shlib_name.  Modify this variable
 *  for the shared library you want to use.  Minimally, the shared
 *  library must provide the following public symbols:
 *
 *  - init          function, [int (*)(void)], returns -5.
 *  - isInitialized function, [int (*)(void)], returns TRUE if init()
 *                  has been called, FALSE if cleanup or nothing has
 *                  been called.
 *  - cleanup       function, [void (*)(void)], no return.
 *  - value         data [int] == FALSE initially, TRUE after init() has
 *                  been called, FALSE after cleanup() has been called.
 */
static void test_ascDynaLoad(void)
{
  FILE *file;
  initFunc          init_func;
  isInitializedFunc isInitialized_func;
  cleanupFunc       cleanup_func;
  valuetype        *value;
  unsigned long prior_meminuse;

#ifdef __WIN32__
  const char *shlib_name = "..\\..\\..\\generic\\utilities\\test\\test_ascDynaLoad_shlib.dll";
#else
  const char *shlib_name = "../../../generic/utilities/test/test_ascDynaLoad_shlib.so";
#endif  /* __WIN32__ */


  prior_meminuse = ascmeminuse();             /* save meminuse() at start of test function */

  /* test Asc_DynamicLoad(), Asc_DynamicUnLoad() */
  CU_TEST(1 == Asc_DynamicLoad(NULL, NULL));    /* NULL path */
  CU_TEST(-3 == Asc_DynamicUnLoad(NULL));

  CU_TEST(1 == Asc_DynamicLoad("dummy", NULL)); /* nonexistent shared lib */
  CU_TEST(-3 == Asc_DynamicUnLoad("dummy"));

  if (NULL == (file = fopen(shlib_name, "r"))) {  /* make sure we can open the test shared library */
    CU_FAIL("Could not find test shared library.  Aborting test.");
  } else {
  
    fclose(file);
    
    if (0 == Asc_DynamicLoad(shlib_name, NULL)) {  /* shared lib with no init func */
      CU_PASS("Opening of shared library succeeded.");
  
      CU_TEST(NULL != (init_func          = (initFunc)Asc_DynamicSymbol(shlib_name, "init")));
      CU_TEST(NULL != (isInitialized_func = (isInitializedFunc)Asc_DynamicSymbol(shlib_name, "isInitialized")));
      CU_TEST(NULL != (cleanup_func       = (cleanupFunc)Asc_DynamicSymbol(shlib_name, "cleanup")));
      CU_TEST(NULL != (value              = (valuetype*)Asc_DynamicSymbol(shlib_name, "value")));
      if ((NULL != init_func) &&
          (NULL != isInitialized_func) &&
          (NULL != cleanup_func) &&
          (NULL != value)) {
        CU_TEST(FALSE == (*isInitialized_func)());
        CU_TEST(FALSE == (*value));
        CU_TEST(-5 == (*init_func)());
        CU_TEST(TRUE == (*isInitialized_func)());
        CU_TEST(TRUE == (*value));
        (*cleanup_func)();
        CU_TEST(FALSE == (*isInitialized_func)());
        CU_TEST(FALSE == (*value));
      }
      CU_TEST(0 == Asc_DynamicUnLoad(shlib_name));
    }
    else {
      CU_FAIL("Opening of shared library failed.");
    }
  
    if (-5 == Asc_DynamicLoad(shlib_name, "init")) {  /* shared lib with init func */
      CU_PASS("Opening of shared library succeeded.");
  
      CU_TEST(NULL != (init_func =          (initFunc)Asc_DynamicSymbol(shlib_name, "init")));
      CU_TEST(NULL != (isInitialized_func = (isInitializedFunc)Asc_DynamicSymbol(shlib_name, "isInitialized")));
      CU_TEST(NULL != (cleanup_func =       (cleanupFunc)Asc_DynamicSymbol(shlib_name, "cleanup")));
      CU_TEST(NULL != (value              = (valuetype*)Asc_DynamicSymbol(shlib_name, "value")));
      if ((NULL != init_func) &&
          (NULL != isInitialized_func) &&
          (NULL != cleanup_func) &&
          (NULL != value)) {
        CU_TEST(TRUE == (*isInitialized_func)());
        CU_TEST(TRUE == (*value));
        (*cleanup_func)();
        CU_TEST(FALSE == (*isInitialized_func)());
        CU_TEST(FALSE == (*value));
      }
      CU_TEST(0 == Asc_DynamicUnLoad(shlib_name));
    }
    else {
      CU_FAIL("Opening of shared library failed.");
    }
  }
  
  /* test Asc_DynamicSymbol() - normal operation tested in previous tests */

  CU_TEST(NULL == Asc_DynamicSymbol(NULL, "init"));       /* NULL libname */
  CU_TEST(NULL == Asc_DynamicSymbol(shlib_name, NULL));   /* NULL symbol */
  CU_TEST(NULL == Asc_DynamicSymbol(shlib_name, "init")); /* library not open */

  CU_TEST(prior_meminuse == ascmeminuse());   /* make sure we cleaned up after ourselves */
}

/*===========================================================================*/
/* Registration information */

static CU_TestInfo ascDynaLoad_test_list[] = {
  {"test_ascDynaLoad", test_ascDynaLoad},
  CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
  {"test_utilities_ascDynaLoad", NULL, NULL, ascDynaLoad_test_list},
  CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_utilities_ascDynaLoad(void)
{
  return CU_register_suites(suites);
}
