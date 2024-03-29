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
#include <ascend/compiler/packages.h>

#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>

/*
	Test solving a simple CONOPT model
*/
static void test_conopt(const char *filenamestem){
	char env1[2*PATH_MAX];
	int solver_index;

	Asc_CompilerInit(1);

	/* set the needed environment variables so that models, solvers can be found */
	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s","models");
	CU_TEST(0 == Asc_PutEnv(env1));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/conopt" OSPATH_DIV "solvers/qrslv" OSPATH_DIV "solver/cmslv"));
	/* read back and display the ASCENDLIBRARY setting */
	char *lib = Asc_GetEnv(ASC_ENV_LIBRARY);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_LIBRARY,lib);
	ASC_FREE(lib);

	/* load the CONOPT solver, presumably from the ASCENDSOLVERS path */
	package_load("conopt",NULL);
	solver_index = slv_lookup_client("CONOPT");
	if(solver_index==-1){
		/* cleanup compiler if we're about to fail */
		Asc_CompilerDestroy();
	}
	CU_ASSERT_FATAL(solver_index != -1);

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *)path,"test/conopt/");
	strncat(path, filenamestem, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
	{
		int status;
		Asc_OpenModule(path,&status);
		CU_ASSERT(status == 0);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(filenamestem))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(filenamestem), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

    CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* assign solver */
	const char *solvername = "CONOPT";
	int index = slv_lookup_client(solvername);
	if(-1 == index){
		Asc_CompilerDestroy();
		CU_FAIL("unable to look up solver 'CONOPT'");
		return;
	}

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);

	CU_ASSERT_FATAL(slv_select_solver(sys,index));
	CONSOLE_DEBUG("Assigned solver '%s'...",solvername);

	CU_ASSERT_FATAL(0 == slv_presolve(sys));

	slv_status_t status;
	slv_get_status(sys, &status);
	CU_ASSERT_FATAL(status.ready_to_solve);

	slv_solve(sys);

	slv_get_status(sys, &status);
	CU_ASSERT(status.ok);

	CONSOLE_DEBUG("Destroying system...");
	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* run 'self_test' method */
	CONSOLE_DEBUG("Running self-tests");
	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);

	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

#define TESTS1(T) \
	T(test11) \
	T(test12) \
	T(test13) \
	T(test14) \
	T(conopttest)

/* define the tests: each test loads the model, solves with CONOPT, then runs the
self_test method. */
#define T(N) static void test_##N(void){\
		test_conopt(#N);\
	}
TESTS1(T)
#undef T

#define TESTS(T) \
	TESTS1(T)

REGISTER_TESTS_SIMPLE(solver_conopt, TESTS)

