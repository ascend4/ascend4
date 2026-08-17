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
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>

#include <test/common.h>
#include <test/assertimpl.h>

static struct Instance *instantiate_parameterized_merge_case(
		const char *modelname, int expect_error){
	struct module_t *m;
	struct Instance *sim;
	int status;
	int has_error;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/merge/merge_parameterized.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);

	error_reporter_tree_start();
	sim = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"),
			e_normal, NULL);
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	if(expect_error){
		CU_ASSERT(has_error || sim == NULL);
	}else{
		CU_ASSERT(sim != NULL);
		CU_ASSERT(!has_error);
	}
	return sim;
}

static void test_merge_param_fail_same_argument(void){
	struct Instance *sim = instantiate_parameterized_merge_case(
			"merge_param_fail_same_argument", 1);
	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void test_merge_param_fail_equal_distinct_arguments(void){
	struct Instance *sim = instantiate_parameterized_merge_case(
			"merge_param_fail_equal_distinct_arguments", 1);
	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void test_merge_param_ok_merge_base_then_refine(void){
	struct Instance *sim = instantiate_parameterized_merge_case(
			"merge_param_ok_merge_base_then_refine", 0);
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *u1 = ChildByChar(root, AddSymbol("U1"));
	struct Instance *u2 = ChildByChar(root, AddSymbol("U2"));
	struct Instance *outlet = ChildByChar(u1, AddSymbol("outlet"));
	struct Instance *inlet = ChildByChar(u2, AddSymbol("inlet"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(u1 != NULL);
	CU_ASSERT(u2 != NULL);
	CU_ASSERT(outlet != NULL);
	CU_ASSERT(inlet != NULL);
	CU_ASSERT(outlet == inlet);
	CU_ASSERT(ChildByChar(outlet, AddSymbol("f")) != NULL);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_merge_param_ok_shared_stream_parameter(void){
	struct Instance *sim = instantiate_parameterized_merge_case(
			"merge_param_ok_shared_stream_parameter", 0);
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *s1 = ChildByChar(root, AddSymbol("S1"));
	struct Instance *u1 = ChildByChar(root, AddSymbol("U1"));
	struct Instance *u2 = ChildByChar(root, AddSymbol("U2"));
	struct Instance *u1_outlet = ChildByChar(u1, AddSymbol("outlet"));
	struct Instance *u2_inlet = ChildByChar(u2, AddSymbol("inlet"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(s1 != NULL);
	CU_ASSERT(u1 != NULL);
	CU_ASSERT(u2 != NULL);
	CU_ASSERT(u1_outlet == s1);
	CU_ASSERT(u2_inlet == s1);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(merge_param_fail_same_argument) \
	T(merge_param_fail_equal_distinct_arguments) \
	T(merge_param_ok_merge_base_then_refine) \
	T(merge_param_ok_shared_stream_parameter)

REGISTER_TESTS_SIMPLE(compiler_merge_parameterized, TESTS)
