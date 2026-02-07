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

static void instantiate_case(const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/array.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);

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

static void test_array_ok_int(void){
	instantiate_case("array_ok_int", 0);
}

static void test_array_ok_sym(void){
	instantiate_case("array_ok_sym", 0);
}

static void test_array_fail_real_range(void){
	instantiate_case("array_fail_real_range", 1);
}

static void test_array_fail_mixed_set(void){
	instantiate_case("array_fail_mixed_set", 1);
}

static void test_array_ok_range_constants(void){
	instantiate_case("array_ok_range_constants", 0);
}

static void test_array_fail_const_indices(void){
	instantiate_case("array_fail_const_indices", 1);
}

static void test_array_fail_bool_set(void){
	instantiate_case("array_fail_bool_set", 1);
}

static void test_array_fail_real_set(void){
	instantiate_case("array_fail_real_set", 1);
}

static void test_array_fail_direct_index_types(void){
	instantiate_case("array_fail_direct_index_types", 1);
}

static void test_array_ok_empty_range(void){
	instantiate_case("array_ok_empty_range", 0);
}

#define TESTS(T) \
	T(array_ok_int) \
	T(array_ok_sym) \
	T(array_fail_real_range) \
	T(array_fail_mixed_set) \
	T(array_ok_range_constants) \
	T(array_fail_const_indices) \
	T(array_fail_bool_set) \
	T(array_fail_real_set) \
	T(array_fail_direct_index_types) \
	T(array_ok_empty_range)

REGISTER_TESTS_SIMPLE(compiler_instantiate_array, TESTS)
