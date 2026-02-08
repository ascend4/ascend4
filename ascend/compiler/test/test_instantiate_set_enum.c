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

	m = Asc_OpenModule("test/instantiate/set_enum_union.a4c", &status);
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

static void test_set_ok(void){
	instantiate_case("set_ok", 0);
}

static void test_set_fail_missing_type(void){
	instantiate_case("set_fail_missing_type", 1);
}

static void test_set_fail_bad_type(void){
	instantiate_case("set_fail_bad_type", 1);
}

static void test_enum_ok(void){
	instantiate_case("enum_ok", 0);
}

static void test_enum_fail_bad_value(void){
	instantiate_case("enum_fail_bad_value", 1);
}

static void test_union_ok(void){
	instantiate_case("union_ok", 0);
}

static void test_union_fail_nonset(void){
	instantiate_case("union_fail_nonset", 1);
}

#define TESTS(T) \
	T(set_ok) \
	T(set_fail_missing_type) \
	T(set_fail_bad_type) \
	T(enum_ok) \
	T(enum_fail_bad_value) \
	T(union_ok) \
	T(union_fail_nonset)

REGISTER_TESTS_SIMPLE(compiler_instantiate_set_enum, TESTS)
