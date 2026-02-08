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

	m = Asc_OpenModule("test/instantiate/context.a4c", &status);
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

static void test_context_fail_method_declare(void){
	instantiate_case("context_fail_method_declare", 1);
}

static void test_context_fail_method_when(void){
	instantiate_case("context_fail_method_when", 1);
}

static void test_context_fail_method_select(void){
	instantiate_case("context_fail_method_select", 1);
}

static void test_context_fail_method_rel(void){
	instantiate_case("context_fail_method_rel", 1);
}

static void test_context_fail_switch_declarative(void){
	instantiate_case("context_fail_switch_declarative", 1);
}

static void test_context_fail_if_declarative(void){
	instantiate_case("context_fail_if_declarative", 1);
}

static void test_context_fail_select_declarative(void){
	instantiate_case("context_fail_select_declarative", 1);
}

static void test_context_pass_where_create(void){
	instantiate_case("context_pass_where_create_wrap", 0);
}

static void test_context_failcheck_where_create(void){
	instantiate_case("context_failcheck_where_create", 1);
}

static void test_context_faildo_where_create(void){
	instantiate_case("context_faildo_where_create", 1);
}

#define TESTS(T) \
	T(context_fail_method_declare) \
	T(context_fail_method_when) \
	T(context_fail_method_select) \
	T(context_fail_method_rel) \
	T(context_fail_switch_declarative) \
	T(context_fail_if_declarative) \
	T(context_fail_select_declarative) \
	T(context_pass_where_create) \
	T(context_failcheck_where_create) \
	T(context_faildo_where_create)

REGISTER_TESTS_SIMPLE(compiler_instantiate_context, TESTS)
