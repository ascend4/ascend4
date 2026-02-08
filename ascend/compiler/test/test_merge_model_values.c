#include <ascend/general/env.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/bit.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/mergeinst.h>

#include <test/common.h>
#include <test/assertimpl.h>

static struct Instance *load_model(const char *modelname){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/merge/merge_model_values.a4c", &status);
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

static void test_merge_model_values_bitlist(void){
	struct Instance *sim = load_model("merge_model_values");
	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *a = ChildByChar(root, AddSymbol("a"));
	struct Instance *b = ChildByChar(root, AddSymbol("b"));

	CU_ASSERT(root != NULL);
	CU_ASSERT(a != NULL);
	CU_ASSERT(b != NULL);

	struct BitList *bl_a = InstanceBitList(a);
	struct BitList *bl_b = InstanceBitList(b);
	CU_ASSERT(bl_a != NULL);
	CU_ASSERT(bl_b != NULL);

	unsigned long len = BLength(bl_a);
	CU_ASSERT(len == BLength(bl_b));
	CU_ASSERT(len >= 2);

	for(unsigned long c = 0; c < len; ++c){
		SetBit(bl_a, c);
		ClearBit(bl_b, c);
	}
	SetBit(bl_b, 0);

	struct Instance *merged = MergeInstances(a, b);
	CU_ASSERT(merged != NULL);

	struct BitList *bl_m = InstanceBitList(merged);
	CU_ASSERT(bl_m != NULL);
	CU_ASSERT(BLength(bl_m) == len);
	for(unsigned long c = 0; c < len; ++c){
		int expect = (c == 0);
		CU_ASSERT((ReadBit(bl_m, c) != 0) == expect);
	}

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(merge_model_values_bitlist)

REGISTER_TESTS_SIMPLE(compiler_merge_model_values, TESTS)
