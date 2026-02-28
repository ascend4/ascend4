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

	m = Asc_OpenModule("test/merge/merge_relparents.a4c", &status);
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

static void test_merge_relparents(void){
	struct Instance *sim = load_model("merge_relparents");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *a = ChildByChar(root, AddSymbol("a"));
	struct Instance *b = ChildByChar(root, AddSymbol("b"));
	struct Instance *ar = ChildByChar(a, AddSymbol("r"));
	struct Instance *alr = ChildByChar(a, AddSymbol("lr"));
	struct Instance *br = ChildByChar(b, AddSymbol("r"));
	struct Instance *blr = ChildByChar(b, AddSymbol("lr"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(a != NULL);
	CU_ASSERT(b != NULL);
	CU_ASSERT(ar != NULL);
	CU_ASSERT(alr != NULL);

	if(br != NULL){
		CU_ASSERT(ar == br);
	}
	if(blr != NULL){
		CU_ASSERT(alr == blr);
	}

	CU_ASSERT(NumberParents(ar) == 1);
	CU_ASSERT(parent_list_contains(ar, a) || parent_list_contains(ar, b));

	CU_ASSERT(NumberParents(alr) == 1);
	CU_ASSERT(parent_list_contains(alr, a) || parent_list_contains(alr, b));

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(merge_relparents)

REGISTER_TESTS_SIMPLE(compiler_merge_relparents, TESTS)
