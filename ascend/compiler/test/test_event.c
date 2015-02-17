#include <string.h>

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
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/value_type.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/cmpfunc.h>

#include <test/common.h>
#include <test/assertimpl.h>

static void test_test1(void){
	struct module_t *m;
	int status;
	struct Instance *sim, *inst, *root;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	m = Asc_OpenModule("ksenija/event.a4c",&status);
	CU_ASSERT(status==0);

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);
        CU_ASSERT(FindType(AddSymbol("test1"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test2"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test3"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("test4"))!=NULL);

	CONSOLE_DEBUG("Instantiating test1");

        sim = SimsCreateInstance(AddSymbol("test1"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("ev"))) && InstanceKind(inst)==EVENT_INST); 
        WriteInstance(ASCERR,inst);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test2");

        sim = SimsCreateInstance(AddSymbol("test2"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("ev"))) && InstanceKind(inst)==EVENT_INST); 
        WriteInstance(ASCERR,inst);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test3");

        sim = SimsCreateInstance(AddSymbol("test3"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test4");

        sim = SimsCreateInstance(AddSymbol("test4"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	sim_destroy(sim);

	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_event, TESTS)
