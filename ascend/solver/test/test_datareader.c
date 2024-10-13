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

#include <ascend/general/config.h>

#include <test/common.h>

/**
	Reusable function for the standard process of loading, initialising, solving
	and testing a model using QRSlv. Any error from loading, solving, testing
	will result in the test failing.
*/
static void dr_load_solve_test_qrslv(const char *librarypath, const char *modelfile, const char *modelname, int instantiatefail){
	char env1[2*PATH_MAX];
	int status;
	int qrslv_index;

	/* initialise the compiler from scratch */
	Asc_CompilerInit(1);

	/* set the needed environment variables so that models, solvers can be found */
	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s",librarypath);
	CU_TEST(0 == Asc_PutEnv(env1));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));
	/* read back and display the ASCENDLIBRARY setting */
	char *lib = Asc_GetEnv(ASC_ENV_LIBRARY);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_LIBRARY,lib);
	ASC_FREE(lib);

	/* load the QRSlv solver, presumably from the ASCENDSOLVERS path */
	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);

	/* load the model file */
	CONSOLE_DEBUG("Opening '%s'",modelfile);
	Asc_OpenModule(modelfile,&status);
	CU_ASSERT(status == 0);
	if(status){
		Asc_CompilerDestroy();
		CU_FAIL_FATAL(failed to load module);
	}

	/* parse it */
	CONSOLE_DEBUG("Parsing '%s'",modelfile);
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(modelname))!=NULL);

	/* instantiate it */
	CONSOLE_DEBUG("Instantiating '%s'",modelfile);
	error_reporter_tree_start();
	struct Instance *siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);

	int has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CONSOLE_DEBUG("has_error = %d",has_error);
	if(instantiatefail){
		CU_TEST(has_error);
	}else{
		CU_TEST(!has_error);
	}
	if(!has_error){
		CU_ASSERT_FATAL(siminst!=NULL);

		/* call on_load method */
		/* FIXME do we check that this method exists first? */
		CONSOLE_DEBUG("RUNNING METHOD 'on_load'");
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);

		/* 'build' the 'system' -- the flattened system of equations */
		slv_system_t sys = system_build(GetSimulationRoot(siminst));
		CU_ASSERT_FATAL(sys != NULL);

		/* assign the solver to the system */
		CU_ASSERT_FATAL(slv_select_solver(sys,qrslv_index));
		CONSOLE_DEBUG("Assigned solver '%s'...",slv_solver_name(slv_get_selected_solver(sys)));

		/* presolve, check it's ready, then solve */
		CU_ASSERT_FATAL(0 == slv_presolve(sys));
		slv_status_t status1;
		slv_get_status(sys, &status1);
		CU_ASSERT_FATAL(status1.ready_to_solve);
		slv_solve(sys);
		/* check that solver status was 'ok' */
		slv_get_status(sys, &status1);
		CU_ASSERT(status1.ok);

		/* clean up the 'system' -- we don't need that any more */
		CONSOLE_DEBUG("Destroying system...");
		if(sys)system_destroy(sys);
		system_free_reused_mem();

		/* run 'self_test' method -- we can check there that the results are as expected */
		CONSOLE_DEBUG("Running self-tests");
		name = CreateIdName(AddSymbol("self_test"));
		pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	/* destroy the compiler data structures, hopefully all dynamically allocated memory */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

/**
	Convenience function to load a model file "name.a4c" and then test the
	model called 'name' within that file, from the directory "models/test/qrslv".
*/
static void test_dr(const char *filenamestem, int instantiatefail){
	/* load the file */
	char modelpath[PATH_MAX];
	strcpy((char *)modelpath,"test/datareader/");
	strncat(modelpath, filenamestem, PATH_MAX - strlen(modelpath));
	strncat(modelpath, ".a4c", PATH_MAX - strlen(modelpath));
	
	dr_load_solve_test_qrslv("models",modelpath,filenamestem,instantiatefail);
}



static void test_allocfree(void){
	int status;
	int qrslv_index;

	/* initialise the compiler from scratch */
	Asc_CompilerInit(1);

	/* set the needed environment variables so that models, solvers can be found */
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));

	/* read back and display the ASCENDLIBRARY setting */
	char *lib = Asc_GetEnv(ASC_ENV_LIBRARY);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_LIBRARY,lib);
	ASC_FREE(lib);

	/* load the QRSlv solver, presumably from the ASCENDSOLVERS path */
	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(qrslv_index != -1);

	/* load the model file */
	Asc_OpenModule("test/datareader/testcsv.a4c",&status);
	CU_ASSERT(status == 0);
	if(status){
		Asc_CompilerDestroy();
		CU_FAIL_FATAL(failed to load module);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol("testcsv"))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol("testcsv"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

	/* call on_load method */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* 'build' the 'system' -- the flattened system of equations */
	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);

	CONSOLE_DEBUG("Destroying system");

	/* clean up the 'system' -- just want to show that we clean our memory */
	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* destroy the compiler data structures, hopefully all dynamically allocated memory */
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_csv(void){
	test_dr("testcsv",0);
}

static void test_interp(void){
	test_dr("testinterp",0);
}

static void test_nofilename(void){
	test_dr("testnofilename",1);
}

static void test_noformat(void){
	test_dr("testnoformat",1);
}

static void test_noparams(void){
	test_dr("testnoparams",1);
}

static void test_energyplus(void){
#ifdef ASC_WITH_ZLIB
	test_dr("testenergyplus",0);
#else
	CU_FAIL("Unable to run 'testenergyplus', requires compilation with zlib");
#endif
}

static void test_tmy3(void){
#ifdef ASC_WITH_ZLIB
	test_dr("testtmy3",0);
#else
	CU_FAIL("Unable to run 'testtmy3', requires compilation with zlib");
#endif
}


/*
class TestCSV(Ascend):
	def test1(self):
		self.L.load('johnpye/datareader/testcsv.a4c')
		M = self.L.findType('testcsv').getSimulation('sim')
		M.solve(ascpy.Solver("QRSlv"),ascpy.SolverReporter())
*/

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(allocfree) \
	T(csv) \
	T(interp) \
	T(nofilename) \
	T(noformat) \
	T(noparams) \
	T(energyplus) \
	T(tmy3)

REGISTER_TESTS_SIMPLE(solver_datareader, TESTS)


