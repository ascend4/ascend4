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
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>

#include <test/common.h>
#include <test/assertimpl.h>

static struct Instance *child_by_name(struct Instance *inst, const char *name){
	struct Instance *child = ChildByChar(inst, AddSymbol(name));
	CU_ASSERT_FATAL(child != NULL);
	return child;
}

static struct Instance *child_by_int_index(struct Instance *arr, long idx){
	struct InstanceName rec;
	unsigned long pos;
	struct Instance *child;

	SetInstanceNameType(rec, IntArrayIndex);
	SetInstanceNameIntIndex(rec, idx);
	pos = ChildSearch(arr, &rec);
	CU_ASSERT_FATAL(pos != 0);
	child = InstanceChild(arr, pos);
	CU_ASSERT_FATAL(child != NULL);
	return child;
}

static void assert_bool_value(struct Instance *inst, int expected){
	CU_ASSERT_FATAL(inst != NULL);
	CU_ASSERT_FATAL(InstanceKind(inst) == BOOLEAN_INST
		|| InstanceKind(inst) == BOOLEAN_ATOM_INST
		|| InstanceKind(inst) == BOOLEAN_CONSTANT_INST);
	CU_ASSERT_FATAL(AtomAssigned(inst));
	CU_ASSERT_EQUAL(GetBooleanAtomValue(inst), expected);
}

static void test_defaults_abc_flowsheet_instantiation(void){
	int status = 0;
	struct Instance *sim = NULL;
	struct Instance *root;
	struct Instance *arr;
	struct Instance *row;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/*m =*/ Asc_OpenModule("abc_flowsheet.a4l", &status);
	CU_ASSERT(status == 0);

	error_reporter_tree_start();
	CU_ASSERT(zz_parse() == 0);
	error_reporter_tree_end();

	CU_ASSERT(FindType(AddSymbol("abc_defaults_probe")) != NULL);
	sim = SimsCreateInstance(AddSymbol("abc_defaults_probe"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim != NULL);

	root = GetSimulationRoot(sim);
	CU_ASSERT_FATAL(root != NULL);

	/* MODEL-level declarative defaults */
	arr = child_by_name(root, "z_on");
	assert_bool_value(child_by_int_index(arr, 1), FALSE);

	arr = child_by_name(root, "hat_on");
	assert_bool_value(child_by_int_index(arr, 1), FALSE);

	arr = child_by_name(root, "hb_on");
	assert_bool_value(child_by_int_index(arr, 1), FALSE);

	arr = child_by_name(root, "Equilibrated");
	assert_bool_value(child_by_int_index(arr, 1), FALSE);

	arr = child_by_name(root, "stream_exists");
	row = child_by_int_index(arr, 1);
	assert_bool_value(child_by_int_index(row, 1), FALSE);
	row = child_by_int_index(arr, 7);
	assert_bool_value(child_by_int_index(row, 7), FALSE);

	/* ATOM-type defaults from atom type definitions */
	assert_bool_value(child_by_name(root, "saturated_liquid_feed"), TRUE);
	assert_bool_value(child_by_name(root, "saturated_vapor_feed"), FALSE);

	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(defaults_abc_flowsheet_instantiation)

REGISTER_TESTS_SIMPLE(compiler_instantiate_defaults, TESTS)
