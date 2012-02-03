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

static void test_boundaries(){

	struct module_t *m;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models" OSPATH_DIV "solvers/lrslv");
	CU_TEST_FATAL(0 == package_load("lrslv",NULL));

	/* load the file */
	char path[PATH_MAX] = "test/ida/boundaries.a4c";
	{
		int status;
		m = Asc_OpenModule(path,&status);
		CU_ASSERT(status == 0);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* instantiate it */
	struct Instance *sim = SimsCreateInstance(AddSymbol("boundaries"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

    /** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* assign solver */
	const char *solvername = "LRSlv";
	int index = slv_lookup_client(solvername);
	CU_ASSERT_FATAL(index != -1);

	slv_system_t sys = system_build(GetSimulationRoot(sim));
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

	/** Call check_satbefore */
	name = CreateIdName(AddSymbol("check_satbefore"));
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* Set t := 4 {s} */
	struct Instance *inst;
	CU_ASSERT((inst = ChildByChar(GetSimulationRoot(sim),AddSymbol("t"))) && InstanceKind(inst)==REAL_ATOM_INST);
	SetRealAtomValue(inst, 4.0, 0);
	CU_ASSERT(RealAtomValue(inst)==4.0);

	/* re-solve */
	CONSOLE_DEBUG("Attempting to re-solve model...");
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_get_status(sys, &status);
	CU_ASSERT_FATAL(status.ready_to_solve);
	slv_solve(sys);
	slv_get_status(sys, &status);
	CU_ASSERT(status.ok);

	CONSOLE_DEBUG("t = %f", RealAtomValue(inst));

	/** Call check_satduring */
	name = CreateIdName(AddSymbol("check_satduring"));
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	CONSOLE_DEBUG("Destroying system...");
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	CU_ASSERT(sim != NULL);
	solver_destroy_engines();
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(boundaries)

REGISTER_TESTS_SIMPLE(solver_lrslv, TESTS)

