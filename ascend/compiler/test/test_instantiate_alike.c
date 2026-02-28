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

	m = Asc_OpenModule("test/instantiate/alike.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	if(!expect_error){
		CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);
	}else if(FindType(AddSymbol(modelname)) == NULL){
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

static void test_alike_fail_atom_children(void){
	instantiate_case("alike_fail_atom_children", 1);
}

static void test_alike_fail_missing_children(void){
	instantiate_case("alike_fail_missing_children", 1);
}

static void test_alike_fail_param_down(void){
	instantiate_case("alike_fail_param_down", 1);
}

static void test_alike_fail_param_object(void){
	instantiate_case("alike_fail_param_object", 1);
}

static void test_alike_fail_array_real(void){
	instantiate_case("alike_fail_array_real", 1);
}

static void test_alike_fail_relation(void){
	instantiate_case("alike_fail_relation", 1);
}

static void test_alike_pass_simple(void){
	instantiate_case("alike_pass_simple", 0);
}

static void test_alike_pass_unparam_down(void){
	instantiate_case("alike_pass_unparam_down", 0);
}

#define TESTS(T) \
	T(alike_fail_atom_children) \
	T(alike_fail_missing_children) \
	T(alike_fail_param_down) \
	T(alike_fail_param_object) \
	T(alike_fail_array_real) \
	T(alike_fail_relation) \
	T(alike_pass_simple) \
	T(alike_pass_unparam_down)

REGISTER_TESTS_SIMPLE(compiler_instantiate_alike, TESTS)
