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
#include <ascend/system/slv_param.h>
#include <ascend/integrator/integrator.h>

#include <test/common.h>

/* a simple integrator reporter for testing */
static int test_lsode_reporter_init(struct IntegratorSystemStruct *integ){
	return 0;
}

static int test_lsode_reporter_write(struct IntegratorSystemStruct *integ){
	return 0; /* no interrupt */
}

static int test_lsode_reporter_writeobs(struct IntegratorSystemStruct *integ){
	CONSOLE_DEBUG("x = %f", var_value(integ->x));
	return 0;
}

static int test_lsode_reporter_close(struct IntegratorSystemStruct *integ){
	return 0;
}

static IntegratorReporter test_lsode_reporter = {
	test_lsode_reporter_init
	,test_lsode_reporter_write
	,test_lsode_reporter_writeobs
	,test_lsode_reporter_close
};

/*
	Test solving a simple IPOPT model
*/
static void test_bounds(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	char *lib = Asc_GetEnv(ASC_ENV_SOLVERS);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_SOLVERS,lib);
	ASC_FREE(lib);

	CU_TEST_FATAL(0 == package_load("qrslv",NULL));

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *)path,"test/lsode/");
#define FILESTEM "bounds"
	strncat(path, FILESTEM, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
	{
		int status;
		Asc_OpenModule(path,&status);
		CU_ASSERT_FATAL(status == 0);
	}

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
#define MODELNAME "boundsfail"
	CU_ASSERT(FindType(AddSymbol(MODELNAME))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(MODELNAME), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

    CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* assign solver */
	const char *solvername = "QRSlv";
	int index = slv_lookup_client(solvername);
	CU_ASSERT_FATAL(index != -1);

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);

	CU_ASSERT_FATAL(slv_select_solver(sys,index));
	CONSOLE_DEBUG("Assigned solver '%s'...",solvername);

	/* create the integrator */

	IntegratorSystem *integ = integrator_new(sys,siminst);

	CU_ASSERT_FATAL(0 == integrator_set_engine(integ,"LSODE"));
	CONSOLE_DEBUG("Assigned integrator '%s'...",integ->internals->name);

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));
	/* TODO set some parameters? */

	/* perform problem analysis */
	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	CONSOLE_DEBUG("Assigning reporter and step sizes...");
	integrator_set_reporter(integ, &test_lsode_reporter);
	integrator_set_minstep(integ,0);
	integrator_set_maxstep(integ,0);
	integrator_set_stepzero(integ,0);
	integrator_set_maxsubsteps(integ,0);

	/* set a linearly-distributed samplelist */
	CONSOLE_DEBUG("Preparing samplelist...");
	double start = 0, end = 10;
	int num = 20;
	dim_type d;
	SetDimFraction(d,D_TIME,CreateFraction(1,1));
	SampleList *samplelist = samplelist_new(num+1, &d);
	double val = start;
	double inc = (end-start)/(num);
	unsigned long i;
	for(i=0; i<=num; ++i){
		samplelist_set(samplelist,i,val);
		val += inc;
	}
	integrator_set_samples(integ,samplelist);

	CONSOLE_DEBUG("Commencing solve...");
	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);

	solver_destroy_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(bounds)

REGISTER_TESTS_SIMPLE(integrator_lsode, TESTS)

