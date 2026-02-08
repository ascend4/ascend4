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

	m = Asc_OpenModule("test/merge/merge_parents.a4c", &status);
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

static int parent_list_contains(CONST struct Instance *child, CONST struct Instance *parent){
	unsigned long c;
	unsigned long n = NumberParents(child);
	for(c = 1; c <= n; ++c){
		if(InstanceParent(child, c) == parent){
			return 1;
		}
	}
	return 0;
}

static void test_merge_parents_real(void){
	struct Instance *sim = load_model("merge_parents_real");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *a = ChildByChar(root, AddSymbol("a"));
	struct Instance *b = ChildByChar(root, AddSymbol("b"));
	struct Instance *ax = ChildByChar(a, AddSymbol("x"));
	struct Instance *bx = ChildByChar(b, AddSymbol("x"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(a != NULL);
	CU_ASSERT(b != NULL);
	CU_ASSERT(ax != NULL);
	CU_ASSERT(bx != NULL);

	CU_ASSERT(NumberParents(ax) == 2);
	CU_ASSERT(parent_list_contains(ax, a));
	CU_ASSERT(parent_list_contains(ax, b));

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_merge_parents_array(void){
	struct Instance *sim = load_model("merge_parents_array");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *p = ChildByChar(root, AddSymbol("p"));
	struct Instance *q = ChildByChar(root, AddSymbol("q"));
	struct Instance *pa = ChildByChar(p, AddSymbol("a"));
	struct Instance *qa = ChildByChar(q, AddSymbol("a"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(p != NULL);
	CU_ASSERT(q != NULL);
	CU_ASSERT(pa != NULL);
	CU_ASSERT(qa != NULL);

	CU_ASSERT(NumberParents(pa) == 2);
	CU_ASSERT(parent_list_contains(pa, p));
	CU_ASSERT(parent_list_contains(pa, q));

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(merge_parents_real) \
	T(merge_parents_array)

REGISTER_TESTS_SIMPLE(compiler_merge_parents, TESTS)
