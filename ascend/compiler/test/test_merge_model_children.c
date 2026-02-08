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

static struct Instance *load_model(const char *modelname){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/merge/merge_model_children.a4c", &status);
	(void)m;
	CU_ASSERT(status == 0);
	CU_ASSERT(zz_parse() == 0);
	CU_ASSERT(FindType(AddSymbol(modelname)) != NULL);

	error_reporter_tree_start();
	struct Instance *sim = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();

	CU_ASSERT(sim != NULL);
	CU_ASSERT(!has_error);
	return sim;
}

static void test_merge_model_children_common(void){
	struct Instance *sim = load_model("merge_model_children_common");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *m1 = ChildByChar(root, AddSymbol("m1"));
	struct Instance *m2 = ChildByChar(root, AddSymbol("m2"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(m1 != NULL);
	CU_ASSERT(m2 != NULL);
	CU_ASSERT(m1 == m2);
	CU_ASSERT(NumberChildren(m1) == 1);
	CU_ASSERT(ChildByChar(m1, AddSymbol("a")) != NULL);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_merge_model_children_uncommon(void){
	struct Instance *sim = load_model("merge_model_children_uncommon");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *m1 = ChildByChar(root, AddSymbol("m1"));
	struct Instance *m2 = ChildByChar(root, AddSymbol("m2"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(m1 != NULL);
	CU_ASSERT(m2 != NULL);
	CU_ASSERT(m1 == m2);
	CU_ASSERT(NumberChildren(m1) == 2);
	CU_ASSERT(ChildByChar(m1, AddSymbol("a")) != NULL);
	CU_ASSERT(ChildByChar(m1, AddSymbol("b")) != NULL);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(merge_model_children_common) \
	T(merge_model_children_uncommon)

REGISTER_TESTS_SIMPLE(compiler_merge_model_children, TESTS)
