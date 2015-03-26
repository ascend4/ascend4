/**
	A minimal driver for ASCEND, using QRSlv. Console-based; no GUI. This can
	be useful for checking low-level features in ASCEND, or working around 
	problems with reporting of errors/messages etc from the GUI.

	See also our CUnit test suites eg in the directory ascend/solver/test.
	See also some C++ driver code in directory ascxx. See also 
	ascend/solver/slv_interface.c for another 'lite' command-line interface, no
	longer maintained but possibly of interest/useful.

	Starting in the directory where this file is located, it can be compiled
	using
	  gcc -orunqrslv runqrslv.c -lascend
	providing ASCEND itself has already been compiled 
	(see http://ascend4.org/Building_ASCEND for details on how to do that)
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <ascend/utilities/config.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>

#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

/* FIXME check whether we need all these includes? */
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/relation_io.h>
//#include <ascend/compiler/reverse_ad.h>
//#include <ascend/compiler/relation_util.h>
//#include <ascend/compiler/mathinst.h>
//#include <ascend/compiler/watchpt.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/packages.h>

#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

void usage(char *n){
	fprintf(stderr,"%s FILENAME\n",n);
	fprintf(stderr,
"  Simple ascend driver\n"
"  Loads and parses the model file FILENAME (eg path/to/myfile.a4c)\n"
"  then attempts to instantiate the model with the name 'myfile'.\n"
"  If the model file loads, it runs the 'on_load' method, then attempts\n"
"  to solve the model using the default QRSlv solver. If it solves,\n"
"  then the method 'self_test' will be run, which can be used to check\n"
"  whether the expected results were found via ASSERT statements in that\n"
"  method.\n");
}

int main(int argc, char *argv[]){
	char env1[2*PATH_MAX];
	int status;
	int qrslv_index;
	int simplify = 1;

	/* get the filename from the commandline */
	if(argc<2){
		usage(argv[0]);
		return 1;
	}
	const char *modelfile = argv[1];
	const char *librarypath = ".:models";

	/* get the file stem as the model name we'll look for */
	struct FilePath *fp = ospath_new(modelfile);
	char *modelname = ospath_getfilestem(fp);

	/* initialise the compiler from scratch */
	Asc_CompilerInit(simplify);

	/* set the needed environment variables so that models, solvers can be found */
	snprintf(env1,2*PATH_MAX,ASC_ENV_LIBRARY "=%s",librarypath);
	assert(0 == Asc_PutEnv(env1));
	assert(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));
	/* read back and display the ASCENDLIBRARY setting */
	char *lib = Asc_GetEnv(ASC_ENV_LIBRARY);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_LIBRARY,lib);
	ASC_FREE(lib);

	/* load the QRSlv solver, presumably from the ASCENDSOLVERS path */
	package_load("qrslv",NULL);
	qrslv_index = slv_lookup_client("QRSlv");
	assert(qrslv_index != -1);

	/* load the model file */
	Asc_OpenModule(modelfile,&status);
	assert(status == 0);

	/* parse it */
	assert(0 == zz_parse());
	/* FIXME, somehow this is failing to detect parser errors; we need to review
	this assertion */

	/* find the model */
	assert(NULL != FindType(AddSymbol(modelname)));

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	assert(siminst!=NULL);

	/* call the on_load method */
	CONSOLE_DEBUG("RUNNING METHOD 'on_load'");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	/* TODO do we check that the method exists first? */
	/* TODO, note that we have not hooked 'slvreq', here, so any calls to SOLVE, OPTION, SOLVER will fail */
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	assert(pe==Proc_all_ok);

	/* 'build' the 'system' -- the flattened system of equations */
	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	assert(sys != NULL);

	/* assign the solver to the system */
	assert(slv_select_solver(sys,qrslv_index));
	CONSOLE_DEBUG("Assigned solver '%s'...",slv_solver_name(slv_get_selected_solver(sys)));

	/* presolve, check it's ready, then solve */
	assert(0 == slv_presolve(sys));
	slv_status_t status1;
	slv_get_status(sys, &status1);
	assert(status1.ready_to_solve);
	slv_solve(sys);
	/* check that solver status was 'ok' */
	slv_get_status(sys, &status1);
	assert(status1.ok);

	/* clean up the 'system' -- we don't need that any more */
	CONSOLE_DEBUG("Destroying system...");
	if(sys)system_destroy(sys);
	system_free_reused_mem();

	/* run 'self_test' method -- we can check there that the results are as expected */
	CONSOLE_DEBUG("Running self-tests");
	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	assert(pe==Proc_all_ok);

	/* destroy the compiler data structures, hopefully all dynamically allocated memory */
	CONSOLE_DEBUG("Destroying instance tree");
	assert(siminst != NULL);
	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();

	/* clean up some locally declared vars */
	ASC_FREE(modelname);
}

