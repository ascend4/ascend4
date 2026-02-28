#include <string.h>

#include <ascend/general/env.h>
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

static void merge_values_case(const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/merge/merge_values.a4c", &status);
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

static void test_merge_ok_int_constants_same(void){
	merge_values_case("merge_ok_int_constants_same", 0);
}

static void test_merge_fail_int_constants_diff(void){
	merge_values_case("merge_fail_int_constants_diff", 1);
}

static void test_merge_ok_bool_constants_same(void){
	merge_values_case("merge_ok_bool_constants_same", 0);
}

static void test_merge_fail_bool_constants_diff(void){
	merge_values_case("merge_fail_bool_constants_diff", 1);
}

static void test_merge_ok_symbol_constants_same(void){
	merge_values_case("merge_ok_symbol_constants_same", 0);
}

static void test_merge_fail_symbol_constants_diff(void){
	merge_values_case("merge_fail_symbol_constants_diff", 1);
}

static void test_merge_ok_real_constants_same(void){
	merge_values_case("merge_ok_real_constants_same", 0);
}

static void test_merge_fail_real_constants_diff(void){
	merge_values_case("merge_fail_real_constants_diff", 1);
}

static void test_merge_ok_sets_same(void){
	merge_values_case("merge_ok_sets_same", 0);
}

static void test_merge_fail_sets_diff(void){
	merge_values_case("merge_fail_sets_diff", 1);
}

#define TESTS(T) \
	T(merge_ok_int_constants_same) \
	T(merge_fail_int_constants_diff) \
	T(merge_ok_bool_constants_same) \
	T(merge_fail_bool_constants_diff) \
	T(merge_ok_symbol_constants_same) \
	T(merge_fail_symbol_constants_diff) \
	T(merge_ok_real_constants_same) \
	T(merge_fail_real_constants_diff) \
	T(merge_ok_sets_same) \
	T(merge_fail_sets_diff)

REGISTER_TESTS_SIMPLE(compiler_merge_values, TESTS)
