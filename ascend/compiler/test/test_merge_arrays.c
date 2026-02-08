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

static void merge_arrays_case(const char *modelname, int expect_error){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/merge/merge_arrays.a4c", &status);
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
		struct Instance *root = GetSimulationRoot(sim);
		struct Instance *a = ChildByChar(root, AddSymbol("a"));
		struct Instance *b = ChildByChar(root, AddSymbol("b"));
		struct Instance *aA = ChildByChar(a, AddSymbol("A"));
		struct Instance *bA = ChildByChar(b, AddSymbol("A"));
		CU_ASSERT(root != NULL);
		CU_ASSERT(a != NULL);
		CU_ASSERT(b != NULL);
		CU_ASSERT(aA != NULL);
		CU_ASSERT(bA != NULL);
		CU_ASSERT(aA == bA);
	}

	if(sim){
		sim_destroy(sim);
	}
	Asc_CompilerDestroy();
}

static void test_merge_arrays_ok(void){
	merge_arrays_case("merge_arrays_ok", 0);
}

static void test_merge_arrays_fail_diff_indices(void){
	merge_arrays_case("merge_arrays_fail_diff_indices", 1);
}

#define TESTS(T) \
	T(merge_arrays_ok) \
	T(merge_arrays_fail_diff_indices)

REGISTER_TESTS_SIMPLE(compiler_merge_arrays, TESTS)
