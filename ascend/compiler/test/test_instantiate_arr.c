#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>

#include <test/common.h>
#include <test/assertimpl.h>

static void instantiate_case_file(const char *filename, const char *modelname, int expect_error, int expect_type){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule(filename, &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	if(expect_type){
		CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);
	}else{
		CU_ASSERT(FindType(AddSymbol(modelname)) == NULL);
		Asc_CompilerDestroy();
		return;
	}

	error_reporter_tree_start();
	struct Instance *sim = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	if(expect_error){
		CU_ASSERT(has_error || sim == NULL);
	}else{
		CU_ASSERT(sim != NULL);
		CU_ASSERT(!has_error);
	}

	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void compile_fail_case(const char *filename, const char *modelname){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule(filename, &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	CU_ASSERT(FindType(AddSymbol(modelname)) == NULL);

	Asc_CompilerDestroy();
}

static void test_arr_ok_int_generated(void){
	instantiate_case_file("test/instantiate/arr_ok.a4c", "arr_ok_int_generated", 0, 1);
}

static void test_arr_ok_sym_generated(void){
	instantiate_case_file("test/instantiate/arr_ok.a4c", "arr_ok_sym_generated", 0, 1);
}

static void test_arr_ok_with_value(void){
	instantiate_case_file("test/instantiate/arr_ok.a4c", "arr_ok_with_value", 0, 1);
}

static void test_arr_fail_with_value_len(void){
	instantiate_case_file("test/instantiate/arr_fail.a4c", "arr_fail_with_value_len", 1, 1);
}

static void test_arr_fail_with_value_type(void){
	instantiate_case_file("test/instantiate/arr_fail.a4c", "arr_fail_with_value_type", 1, 1);
}

static void test_arr_fail_rhs_unknown(void){
	compile_fail_case("test/instantiate/arr_fail.a4c", "arr_fail_rhs_unknown");
}

static void test_arr_fail_lhs_multiple(void){
	compile_fail_case("test/instantiate/arr_fail.a4c", "arr_fail_lhs_multiple");
}

static void test_arr_fail_setname_multiple(void){
	compile_fail_case("test/instantiate/arr_fail.a4c", "arr_fail_setname_multiple");
}

#define TESTS(T) \
	T(arr_ok_int_generated) \
	T(arr_ok_sym_generated) \
	T(arr_ok_with_value) \
	T(arr_fail_with_value_len) \
	T(arr_fail_with_value_type) \
	T(arr_fail_rhs_unknown) \
	T(arr_fail_lhs_multiple) \
	T(arr_fail_setname_multiple)

REGISTER_TESTS_SIMPLE(compiler_instantiate_arr, TESTS)
