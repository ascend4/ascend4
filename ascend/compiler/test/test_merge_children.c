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

	m = Asc_OpenModule("test/merge/merge_children.a4c", &status);
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

static void test_merge_children_common(void){
	struct Instance *sim = load_model("merge_children_common");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *a = ChildByChar(root, AddSymbol("a"));
	struct Instance *b = ChildByChar(root, AddSymbol("b"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(a != NULL);
	CU_ASSERT(b != NULL);
	CU_ASSERT(a == b);
	CU_ASSERT(NumberChildren(a) == 2);
	CU_ASSERT(ChildByChar(a, AddSymbol("c1")) != NULL);
	CU_ASSERT(ChildByChar(a, AddSymbol("c2")) != NULL);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_merge_children_uncommon(void){
	struct Instance *sim = load_model("merge_children_uncommon");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *a = ChildByChar(root, AddSymbol("a"));
	struct Instance *b = ChildByChar(root, AddSymbol("b"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(a != NULL);
	CU_ASSERT(b != NULL);
	CU_ASSERT(a == b);
	CU_ASSERT(NumberChildren(a) == 3);
	CU_ASSERT(ChildByChar(a, AddSymbol("c1")) != NULL);
	CU_ASSERT(ChildByChar(a, AddSymbol("c2")) != NULL);
	CU_ASSERT(ChildByChar(a, AddSymbol("c3")) != NULL);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(merge_children_common) \
	T(merge_children_uncommon)

REGISTER_TESTS_SIMPLE(compiler_merge_children, TESTS)
