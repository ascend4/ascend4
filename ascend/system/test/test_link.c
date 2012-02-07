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

#define BUILDFAIL 1

static void load_and_initialise(char *filename, char *modelname, int buildfail){
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");

	CONSOLE_DEBUG("Opening '%s'...",filename);

	/* load the file, parse it, and find our type definition */
	Asc_OpenModule(filename,&status);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT(FindType(AddSymbol(modelname))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

    CONSOLE_DEBUG("Running 'on_load'...");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	slv_system_t sys = system_build(siminst);
	if(buildfail){
		CU_ASSERT(sys==NULL);
	}else{
		CU_ASSERT(sys != NULL);
	}

 	CONSOLE_DEBUG("system has been built... any errors???");

	if(sys)system_destroy(sys);
	system_free_reused_mem();


	/* destroy, clean up */
	CU_ASSERT(NULL != siminst)
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_simple_ok1(){
	load_and_initialise("test/link/simple.a4c","simple_ok1",0);
}

static void test_simple_ok2(){
	load_and_initialise("test/link/simple.a4c","simple_ok2",0);
}

static void test_simple_fail1(){
	load_and_initialise("test/link/simple.a4c","simple_fail1",BUILDFAIL);
}

static void test_simple_fail2(){
	load_and_initialise("test/link/simple.a4c","simple_fail2",BUILDFAIL);
}

static void test_simple_fail3(){
	load_and_initialise("test/link/simple.a4c","simple_fail3",BUILDFAIL);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(simple_ok1) \
	T(simple_ok2) \
	T(simple_fail1) \
	T(simple_fail2) \
	T(simple_fail3)

REGISTER_TESTS_SIMPLE(system_link, TESTS)

