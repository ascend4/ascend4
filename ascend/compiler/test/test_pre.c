#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/pre.h>
#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/value_type.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/cmpfunc.h>

#include <test/common.h>
#include <test/assertimpl.h>

static void test_test1(void){
	struct module_t *m;
	int status;
	struct Instance *sim, *inst, *root, *inst1;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	m = Asc_OpenModule("ksenija/pre.a4c",&status);
	CU_ASSERT(status==0);

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);
        CU_ASSERT(FindType(AddSymbol("test_pre1"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test_pre2"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test_pre3"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test_pre4"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test_pre5"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test_pre6"))!=NULL);


	CONSOLE_DEBUG("Instantiating test_pre1");

        sim = SimsCreateInstance(AddSymbol("test_pre1"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("pre(x)"))) && InstanceKind(inst)==REAL_ATOM_INST); 
        WritePreInfo(ASCERR,inst);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test_pre2");

        sim = SimsCreateInstance(AddSymbol("test_pre2"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test_pre3");

        sim = SimsCreateInstance(AddSymbol("test_pre3"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test_pre4");

        sim = SimsCreateInstance(AddSymbol("test_pre4"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("pre(x)"))) && InstanceKind(inst)==ARRAY_INT_INST); 
	inst1 = InstanceChild(inst,1);
	WritePreInfo(ASCERR,inst1);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("x"))) && InstanceKind(inst)==ARRAY_INT_INST); 
	inst1 = InstanceChild(inst,3);
	WritePreInfo(ASCERR,inst1);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test_pre5");

        sim = SimsCreateInstance(AddSymbol("test_pre5"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("pre(x)"))) != NULL);
	WritePreInfo(ASCERR,inst);
	inst1 = ChildByChar(root,AddSymbol("x"));
	CU_ASSERT(FindPreByArg(inst1) == inst);
	inst1 = ChildByChar(root,AddSymbol("y"));
	CU_ASSERT(FindPreByArg(inst1) == inst);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test_pre6");

        sim = SimsCreateInstance(AddSymbol("test_pre6"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("pre(x)"))) && InstanceKind(inst)==ARRAY_INT_INST); 
	inst1 = InstanceChild(inst,1);
	WritePreInfo(ASCERR,inst1);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test_pre7");

        sim = SimsCreateInstance(AddSymbol("test_pre7"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("pre(x)"))) != NULL);
	CU_ASSERT(InstanceTypeDesc(inst) == FindType(AddSymbol("distance")));

	sim_destroy(sim);

	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_pre, TESTS)
