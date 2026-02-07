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

static void instantiate_case(const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/instantiate/alias.a4c", &status);
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

static void test_alias_ok_simple(void){
	instantiate_case("alias_ok_simple", 0);
}

static void test_alias_fail_unknown_child(void){
	compile_reject_case("test/instantiate/alias_compile_fail_unknown.a4c",
			"alias_fail_unknown_child");
}

static void test_alias_fail_for_unindexed(void){
	instantiate_case("alias_fail_for_unindexed", 1);
}

#define TESTS(T) \
	T(alias_ok_simple) \
	T(alias_fail_unknown_child) \
	T(alias_fail_for_unindexed)

REGISTER_TESTS_SIMPLE(compiler_instantiate_alias, TESTS)
