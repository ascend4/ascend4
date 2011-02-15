#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>

#include <ascend/general/platform.h>
#include <ascend/general/ospath.h>
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
#include <ascend/compiler/bintoken.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>

/*
	Test solving a simple model with 'bintoken' support
*/
static void test_test1(){

	struct module_t *m;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models:solvers/qrslv");
	
	/* load the file */
	char path[PATH_MAX];
#define filenamestem "test1"
	strcpy((char *)path,"test/bintok/");
	strncat(path, filenamestem, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
	{
		int status;
		m = Asc_OpenModule(path,&status);
		CU_ASSERT(status == 0);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(filenamestem))!=NULL);

#define T_DIR "/tmp"
#define T_BTSRC T_DIR "/btsrc.c"
#define T_BTOBJ T_DIR "/btsrc.o"
#define T_BTLIB T_DIR "/btsrc.so"

	struct FilePath *fp1 = ospath_new("ascend/bintokens/Makefile");
	struct FilePath *fp2 = ospath_getabs(fp1);
	char *fp2str = ospath_str(fp2);
	char makecmd[2 * PATH_MAX];
	strcpy(makecmd, "make -f ");
	strncat(makecmd, fp2str, PATH_MAX - strlen(makecmd));
	strncat(makecmd, " ASCBT_SRC=" T_BTSRC " ASCBT_TARGET=" T_BTLIB, PATH_MAX - strlen(makecmd));
	
	BinTokenSetOptions(
		T_BTSRC, T_BTOBJ, T_BTLIB
		,makecmd
		,"/bin/rm"
		,1000/*maxrels*/,1/*verbose*/,0/*housekeep*/
	);
	ASC_FREE(fp2str);
	ospath_free(fp1);
	ospath_free(fp2);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(filenamestem), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

    CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
#define ONLOAD "on_load"
	struct Name *name = CreateIdName(AddSymbol(ONLOAD));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,ONLOAD, ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	
	/* assign solver */
	const char *solvername = "QRSlv";
	int index = slv_lookup_client(solvername);
	CU_ASSERT_FATAL(index != -1);

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
#define SELFTEST "self_test"
	name = CreateIdName(AddSymbol(SELFTEST));
	pe = Initialize(GetSimulationRoot(siminst),name,SELFTEST, ASCERR, WP_STOPONERR, NULL, NULL);
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

/* the list of tests */

#define TESTS(T) \
	T(test1)

REGISTER_TESTS_SIMPLE(compiler_bintok, TESTS)

