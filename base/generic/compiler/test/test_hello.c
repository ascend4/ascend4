/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Unit test functions for compiler. Nothing here yet.
*/
#include <string.h>
#include <CUnit/CUnit.h>

#include <utilities/ascConfig.h>

#include <compiler/ascCompiler.h>
#include <compiler/module.h>

#include <assertimpl.h>

static void test_init(void){

	CU_ASSERT(0 == Asc_CompilerInit(0));

	Asc_CompilerDestroy();
}

static void test_parse_string_module(void){
	
	const char *model = "\n\
		MODEL test1;\n\
			x IS_A solver_var;\n\
			x - 1 = 0;\n\
		END test1;";

	Asc_CompilerInit(1);

	struct module_t *m;
	int status;
	
	m = Asc_OpenStringModule(model, &status, ""/* name prefix*/);

	CONSOLE_DEBUG("Asc_OpenStringModule returns status=%d",status);
	CU_ASSERT(status==0); /* if successfully created */

	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
	status = zz_parse();

	CONSOLE_DEBUG("zz_parse returns status=%d",status);

	CU_ASSERT(status==0);
		
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T,X) \
	T(init) \
	X T(parse_string_module)

/* you shouldn't need to change the following */

#define TESTDECL(TESTFN) {#TESTFN,test_##TESTFN}

#define X ,

static CU_TestInfo hello_test_list[] = {
	TESTS(TESTDECL,X)
	X CU_TEST_INFO_NULL
};

static CU_SuiteInfo suites[] = {
	{"test_compiler_hello", NULL, NULL, hello_test_list},
	CU_SUITE_INFO_NULL
};

/*-------------------------------------------------------------------*/
CU_ErrorCode test_register_compiler_hello(void){
	return CU_register_suites(suites);
}
