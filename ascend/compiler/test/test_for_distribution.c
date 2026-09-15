/* Declarative loop distribution: values, structure and phase ordering. */
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
#include <ascend/compiler/arrayinst.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <test/common.h>

static void run_case(const char *type, long expected, int method, int pending){
	int status, errors;
	struct Instance *sim, *root, *result;
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_OpenModule("test/instantiate/for_distribution.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	error_reporter_tree_start();
	status = zz_parse();
	errors = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(status == 0 && !errors);
	if(status || errors){
		Asc_CompilerDestroy();
		return;
	}
	CU_ASSERT_FATAL(FindType(AddSymbol(type)) != NULL);
	error_reporter_tree_start();
	sim = SimsCreateInstance(AddSymbol(type),AddSymbol("sim1"),e_normal,NULL);
	errors = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT_FATAL(sim != NULL);
	root = GetSimulationRoot(sim);
	result = ChildByChar(root,AddSymbol("result"));
	CU_ASSERT_FATAL(result != NULL);
	/* A non-NULL simulation alone does not demonstrate completed compilation. */
	CU_ASSERT(AtomAssigned(result) == !pending);
	if(!pending){
		CU_ASSERT(!errors);
		if(AtomAssigned(result)) CU_ASSERT(GetIntegerAtomValue(result) == expected);
	}
	if(method){
		struct Name *name = CreateIdName(AddSymbol("self_test"));
		CU_ASSERT(Initialize(root,name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL) == Proc_all_ok);
		DestroyName(name);
	}
	if(strcmp(type,"fd_equations") == 0){
		struct Instance *eq = ChildByChar(root,AddSymbol("eq"));
		struct Instance *lr = ChildByChar(root,AddSymbol("lr"));
		unsigned long j;
		CU_ASSERT_FATAL(eq != NULL && lr != NULL);
		CU_ASSERT(NumberChildren(eq) == 3 && NumberChildren(lr) == 3);
		for(j = 1; j <= 3; ++j){
			CU_ASSERT(InstanceKind(InstanceChild(eq,j)) == REL_INST);
			CU_ASSERT(InstanceKind(InstanceChild(lr,j)) == LREL_INST);
		}
	}
	if(strcmp(type,"fd_merge") == 0 || strcmp(type,"fd_alike") == 0){
		struct Instance *c = ChildByChar(root,AddSymbol("c"));
		struct Instance *other = ChildByChar(root,AddSymbol("other"));
		unsigned long j;
		CU_ASSERT_FATAL(c != NULL && other != NULL);
		for(j = 1; j <= 3; ++j){
			CU_ASSERT((InstanceChild(c,j) == InstanceChild(other,j)) == (strcmp(type,"fd_merge") == 0));
			CU_ASSERT(ChildByChar(InstanceChild(other,j),AddSymbol("extra")) != NULL);
		}
	}
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define CASE(NAME,EXPECTED,METHOD,PENDING) \
static void test_##NAME(void){run_case("fd_" #NAME,EXPECTED,METHOD,PENDING);}
CASE(constants,6,0,0)
CASE(reverse,12,0,0)
CASE(children,6,0,0)
CASE(array_parameter,6,0,0)
CASE(parameters,12,0,0)
CASE(ragged,54,0,0)
CASE(symbols,6,0,0)
CASE(merge,12,0,0)
CASE(alike,60,0,0)
CASE(select_true,6,0,0)
CASE(select_false,60,0,0)
CASE(nested_select,6,0,0)
CASE(refines,6,1,0)
CASE(equations,6,0,0)
CASE(empty,0,0,0)
CASE(defaults,6,1,0)
CASE(method,1,1,0)
CASE(recurrence,0,0,1)
CASE(cycle,0,0,1)
#undef CASE

#define TESTS(T) \
	T(constants) T(reverse) T(children) T(array_parameter) T(parameters) T(ragged) T(symbols) \
	T(merge) T(alike) T(select_true) T(select_false) T(nested_select) T(refines) T(equations) \
	T(empty) T(defaults) T(method) T(recurrence) T(cycle)
REGISTER_TESTS_SIMPLE(compiler_for_distribution,TESTS)
