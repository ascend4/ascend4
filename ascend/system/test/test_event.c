#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>
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
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/reverse_ad.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/watchpt.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/instance_io.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>

static void test_event_ok1(){
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");

	/* load the file, parse it, and find our type definition */
	Asc_OpenModule("ksenija/event.a4c",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(FindType(AddSymbol("test1"))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol("test1"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT(sys != NULL);

 	CONSOLE_DEBUG("System has been built.");
        struct rel_relation **rels = slv_get_solvers_rel_list(sys);
	CU_ASSERT(slv_get_num_solvers_rels(sys)==2);
	CU_ASSERT(rel_active(rels[0]));
	CU_ASSERT(!rel_active(rels[1]));

	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* destroy, clean up */
	CU_ASSERT(NULL != siminst)
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(event_ok1)

REGISTER_TESTS_SIMPLE(system_event, TESTS)

