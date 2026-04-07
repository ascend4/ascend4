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

	m = Asc_OpenModule("test/instantiate/for.a4c", &status);
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

static void instantiate_case_from_module(const char *modulepath, const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule(modulepath, &status);
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

static void compile_reject_case(const char *modulepath, const char *modelname){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule(modulepath, &status);
	(void)m;
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(zz_parse() == 0);
	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(has_error);
	CU_ASSERT(FindType(AddSymbol(modelname)) == NULL);

	Asc_CompilerDestroy();
}

static void test_for_ok_int(void){
	instantiate_case("for_ok_int", 0);
}

static void test_for_ok_sym(void){
	instantiate_case("for_ok_sym", 0);
}

static void test_for_fail_unindexed(void){
	instantiate_case("for_fail_unindexed", 1);
}

static void test_for_fail_nonscalar_subscript(void){
	instantiate_case("for_fail_nonscalar_subscript", 1);
}

static void test_for_fail_unindexed_relation_array(void){
	compile_reject_case("test/instantiate/for_compile_fail_unindexed_relation.a4c",
			"for_fail_unindexed_relation_array");
}

static void test_for_ok_empty_create(void){
	instantiate_case("for_ok_empty_create", 0);
}

static void test_for_ok_relation_empty(void){
	instantiate_case("for_ok_relation_empty", 0);
}

static void test_for_ok_indirect_sparse(void){
	instantiate_case("for_ok_indirect_sparse", 0);
}

static void test_for_fail_const_matrix_in_relation(void){
	instantiate_case_from_module("test/compiler/forsum_bug.a4c",
			"forsum_bug_const_matrix", 1);
}

#define TESTS(T) \
	T(for_ok_int) \
	T(for_ok_sym) \
	T(for_fail_unindexed) \
	T(for_fail_nonscalar_subscript) \
	T(for_fail_unindexed_relation_array) \
	T(for_ok_empty_create) \
	T(for_ok_relation_empty) \
	T(for_ok_indirect_sparse) \
	T(for_fail_const_matrix_in_relation)

REGISTER_TESTS_SIMPLE(compiler_instantiate_for, TESTS)
