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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ascend/general/platform.h>
#ifdef __WIN32__
#include <io.h>
#endif
#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/ascDynaLoad.h>

#include <test/common.h>
#include <test/printutil.h>

#include "shlib_test.h"

int error_reporter_callback_silent(ERROR_REPORTER_CALLBACK_ARGS){
	return 0;
}

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
static void test_single(void){
	FILE *file;
	initFunc          init_func;
	isInitializedFunc isInitialized_func;
	cleanupFunc       cleanup_func;
	valuetype        *value1, *value2;
	unsigned long prior_meminuse;
#ifdef __WIN32__
	const char *shlib_name = "ascend\\utilities\\test\\testdynaload.dll";
#elif defined(__APPLE__)
	const char *shlib_name = "ascend/utilities/test/libtestdynaload.dylib";
#else
	const char *shlib_name = "ascend/utilities/test/libtestdynaload.so";
#endif  /* __WIN32__ */

	error_reporter_set_callback(&error_reporter_callback_silent);

#ifdef MALLOC_DEBUG
	prior_meminuse = ascmeminuse();
#endif

	/* test Asc_DynamicLoad(), Asc_DynamicUnLoad() */

	CU_TEST(1 == Asc_DynamicLoad(NULL, NULL));    /* NULL path */
	CU_TEST(-3 == Asc_DynamicUnLoad(NULL));

	CU_TEST(1 == Asc_DynamicLoad("dummy", NULL)); /* nonexistent shared lib */
	CU_TEST(-3 == Asc_DynamicUnLoad("dummy"));

	if(NULL == (file = fopen(shlib_name, "r"))) {  /* make sure we can open the test shared library */
		CU_FAIL("Could not find test shared library. Aborting test.");
	}else{
		fclose(file);
		if(0 == Asc_DynamicLoad(shlib_name, NULL)) {  /* shared lib with no init func */
			CU_PASS("Opening of shared library succeeded.");

			CU_TEST(NULL != (init_func          = (initFunc)Asc_DynamicFunction(shlib_name, "init")));
			CU_TEST(NULL != (isInitialized_func = (isInitializedFunc)Asc_DynamicFunction(shlib_name, "isInitialized")));
			CU_TEST(NULL != (cleanup_func       = (cleanupFunc)Asc_DynamicFunction(shlib_name, "cleanup")));
			CU_TEST(NULL != (value1             = (valuetype*)Asc_DynamicVariable(shlib_name, "value")));
			CU_TEST(NULL != (value2             = (valuetype*)Asc_DynamicSymbol(shlib_name, "value")));
			if((NULL != init_func) && (NULL != isInitialized_func) 
				&& (NULL != cleanup_func) && (NULL != value1) 
				&& (NULL != value2)
			){
				CU_TEST(FALSE == (*isInitialized_func)());
				CU_TEST(FALSE == (*value1));
				CU_TEST(FALSE == (*value2));
				CU_TEST(-5 == (*init_func)());
				CU_TEST(TRUE == (*isInitialized_func)());
				CU_TEST(TRUE == (*value1));
				CU_TEST(TRUE == (*value2));
				(*cleanup_func)();
				CU_TEST(FALSE == (*isInitialized_func)());
				CU_TEST(FALSE == (*value1));
				CU_TEST(FALSE == (*value2));
			}
			CU_TEST(0 == Asc_DynamicUnLoad(shlib_name));
		}else{
			CU_FAIL("Opening of shared library failed.");
		}

		if(-5 == Asc_DynamicLoad(shlib_name, "init")) {  /* shared lib with init func */
			CU_PASS("Opening of shared library succeeded.");

			CU_TEST(NULL != (init_func =          (initFunc)Asc_DynamicFunction(shlib_name, "init")));
			CU_TEST(NULL != (isInitialized_func = (isInitializedFunc)Asc_DynamicFunction(shlib_name, "isInitialized")));
			CU_TEST(NULL != (cleanup_func =       (cleanupFunc)Asc_DynamicFunction(shlib_name, "cleanup")));
			CU_TEST(NULL != (value1             = (valuetype*)Asc_DynamicVariable(shlib_name, "value")));
			CU_TEST(NULL != (value2             = (valuetype*)Asc_DynamicSymbol(shlib_name, "value")));
			if((NULL != init_func) && (NULL != isInitialized_func) 
				&& (NULL != cleanup_func) && (NULL != value1)
				&& (NULL != value2)
			){
				CU_TEST(TRUE == (*isInitialized_func)());
				CU_TEST(TRUE == (*value1));
				CU_TEST(TRUE == (*value2));
				(*cleanup_func)();
				CU_TEST(FALSE == (*isInitialized_func)());
				CU_TEST(FALSE == (*value1));
				CU_TEST(FALSE == (*value2));
			}
			CU_TEST(0 == Asc_DynamicUnLoad(shlib_name));
		}else{
			CU_FAIL("Opening of shared library failed.");
		}
	}

	/* test Asc_DynamicVariable(), test Asc_DynamicFunction()
		, test Asc_DynamicSymbol() - normal operation tested in previous tests
	*/
	CU_TEST(NULL == Asc_DynamicVariable(NULL, "value"));       /* NULL libname */
	CU_TEST(NULL == Asc_DynamicVariable(shlib_name, NULL));   /* NULL symbol */
	CU_TEST(NULL == Asc_DynamicVariable(shlib_name, "value")); /* library not open */

	CU_TEST(NULL == Asc_DynamicSymbol(NULL, "value"));       /* NULL libname */
	CU_TEST(NULL == Asc_DynamicSymbol(shlib_name, NULL));   /* NULL symbol */
	CU_TEST(NULL == Asc_DynamicSymbol(shlib_name, "value")); /* library not open */

	CU_TEST(NULL == Asc_DynamicFunction(NULL, "init"));       /* NULL libname */
	CU_TEST(NULL == Asc_DynamicFunction(shlib_name, NULL));   /* NULL symbol */
	CU_TEST(NULL == Asc_DynamicFunction(shlib_name, "init")); /* library not open */

#ifdef MALLOC_DEBUG
	CU_TEST(prior_meminuse == ascmeminuse());
#endif
	error_reporter_set_callback(NULL);
}



static void test_multi(void){
	unsigned long prior_meminuse;
#ifdef __WIN32__
	const char *shlib_name[3] = {"ascend\\utilities\\test\\testdynaload.dll"
		,"ascend\\utilities\\test\\testdynaload2.dll"
		,"ascend\\utilities\\test\\testdynaload3.dll"
	};
#elif defined(__APPLE__)
	const char *shlib_name[3] = {"ascend/utilities/test/libtestdynaload.dylib"
		,"ascend/utilities/test/libtestdynaload2.dylib"
		,"ascend/utilities/test/libtestdynaload3.dylib"
	};
#else /* linux, maybe others */
	const char *shlib_name[3] = {"ascend/utilities/test/libtestdynaload.so"
		,"ascend/utilities/test/libtestdynaload2.so"
		,"ascend/utilities/test/libtestdynaload3.so"
	};
#endif
	
#ifdef MALLOC_DEBUG
	prior_meminuse = ascmeminuse();
#endif

	FILE *f[3];
	int i, ok=1;
	for(i=0;i<3;i++){
		CU_TEST(NULL != (f[i] = fopen(shlib_name[i], "r")));
		if(f[i])fclose(f[i]);
		else ok = 0;
	}
	if(ok){// all three files exist and could be opened
		for(i=0;i<3;i++){
			CU_TEST(-5+i == Asc_DynamicLoad(shlib_name[i], "init"));
		}
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[0]));
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[1]));
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[2]));

		for(i=0;i<3;i++){
			CU_TEST(-5+i == Asc_DynamicLoad(shlib_name[i], "init"));
		}
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[2]));
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[1]));
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[0]));

		for(i=0;i<3;i++){
			CU_TEST(-5+i == Asc_DynamicLoad(shlib_name[i], "init"));
		}
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[1]));
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[0]));
		CU_TEST(0==Asc_DynamicUnLoad(shlib_name[2]));

	}

#ifdef MALLOC_DEBUG
	CU_TEST(prior_meminuse == ascmeminuse());
#endif
}	


/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(single) \
	T(multi)

REGISTER_TESTS_SIMPLE(utilities_ascDynaLoad, TESTS)

