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
static void test_bintok(char *filenamestem,int usebintok){
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");
	Asc_PutEnv(ASC_ENV_BTINC "=.");
	Asc_PutEnv(ASC_ENV_BTLIB "=.");

	/* load and parse */
	char path[PATH_MAX];
	strcpy((char *)path,"test/bintok/");
	strncat(path, filenamestem, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
	int ostatus;
	Asc_OpenModule(path,&ostatus); 
	CU_ASSERT(ostatus == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(filenamestem))!=NULL);

	/* instantiate it */
	if(usebintok){
		CU_TEST(0==BinTokenSetOptionsDefault());
	}else{
		CU_TEST(0==BinTokenClearOptions());
	}
	error_reporter_tree_t *T1 = error_reporter_tree_start(0);
	struct Instance *siminst = SimsCreateInstance(AddSymbol(filenamestem), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);
	CU_TEST(!error_reporter_tree_has_error(T1));
	error_reporter_tree_end(T1);

	/* initialise */
#define ONLOAD "on_load"
	struct Name *name = CreateIdName(AddSymbol(ONLOAD));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,ONLOAD, ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);

	/* solve */
	CU_ASSERT_FATAL(slv_select_solver(sys,slv_lookup_client("QRSlv")));
	CU_ASSERT_FATAL(0 == slv_presolve(sys));
	slv_status_t status;
	slv_get_status(sys, &status);
	CU_ASSERT_FATAL(status.ready_to_solve);
	slv_solve(sys);
	slv_get_status(sys, &status);
	CU_ASSERT(status.ok);
	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* run 'self_test' method */
#define SELFTEST "self_test"
	name = CreateIdName(AddSymbol(SELFTEST));
	pe = Initialize(GetSimulationRoot(siminst),name,SELFTEST, ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* cleanup */
	CU_ASSERT(siminst != NULL);
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_test1(){
	test_bintok("test1",1);
}

static void test_nobintok(){
	test_bintok("test1",0);
}

static void test_gradient(){
	test_bintok("gradient",1);
}

static void test_gradient_nobintok(){
	test_bintok("gradient",0);
}


/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1) \
	T(nobintok) \
	T(gradient) \
	T(gradient_nobintok)

REGISTER_TESTS_SIMPLE(compiler_bintok, TESTS)

