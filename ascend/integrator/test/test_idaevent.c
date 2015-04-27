#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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
#include <test/test_globals.h>

/* a simple integrator reporter for testing */
static int test_ida_reporter_init(struct IntegratorSystemStruct *integ) {
	return 0;
}

static int test_ida_reporter_write(struct IntegratorSystemStruct *integ) {
#if 0
	double y0 = var_value(integ->y[0]);
	double x = var_value(integ->x);
	CONSOLE_DEBUG("x = %g: y[0] = %g", x, y0);
#endif
	return 1; /* no interrupt */
}

static int test_ida_reporter_writeobs(struct IntegratorSystemStruct *integ) {
	//CONSOLE_DEBUG("x = %f", var_value(integ->x));
	if(integ->n_obs){
		int i;
		fprintf(stderr,"x=%g: (",var_value(integ->x));
		for(i=0;i<integ->n_obs;++i){
			char *n = var_make_name(integ->system,integ->obs[i]);
			fprintf(stderr,"%s%s=%g",(i?", ":""),n,var_value(integ->obs[i]));
			ASC_FREE(n);
		}
		fprintf(stderr,")\n");
	}
	return 0;
}

static int test_ida_reporter_writeevent(struct IntegratorSystemStruct *integ) {
	CONSOLE_DEBUG("EVENT AT x = %f", var_value(integ->x));
	return 0;
}

static int test_ida_reporter_close(struct IntegratorSystemStruct *integ) {
	return 0;
}

static IntegratorReporter test_ida_reporter = { test_ida_reporter_init,
		test_ida_reporter_write, test_ida_reporter_writeobs,
		test_ida_reporter_writeevent, test_ida_reporter_close
};


#define LOAD_AND_INITIALISE(MODELPATH,FILESTEM) \
	Asc_CompilerInit(1);\
	/* set paths relative to test executable */\
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");\
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lrslv:solvers/qrslv");\
	CU_TEST_FATAL(0 == package_load("lrslv",NULL));\
	CU_TEST_FATAL(0 == package_load("qrslv",NULL));\
	/* load the file */\
	char path[PATH_MAX] = MODELPATH "/" FILESTEM ".a4c";\
	{int status;Asc_OpenModule(path, &status);CU_ASSERT_FATAL(status == 0);}\
	/* parse it */\
	CU_ASSERT(0 == zz_parse());\
	/* find the model */\
	CU_ASSERT(FindType(AddSymbol(FILESTEM))!=NULL);\
	/* instantiate it */\
	struct Instance *siminst = SimsCreateInstance(AddSymbol(FILESTEM), AddSymbol("sim1"), e_normal, NULL);\
	CU_ASSERT_FATAL(siminst!=NULL);\
	CONSOLE_DEBUG("Running 'on_load' method...");\
	/** Call on_load */\
	struct Name *name = CreateIdName(AddSymbol("on_load"));\
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);\
	CU_ASSERT(pe==Proc_all_ok);\
	/* create the integrator */\
	g_use_dersyntax = 1;\
	CONSOLE_DEBUG("Building system...");\
	slv_system_t sys = system_build(GetSimulationRoot(siminst));\
	CU_ASSERT_FATAL(sys != NULL);\
	IntegratorSystem *integ = integrator_new(sys,siminst);\
	CU_ASSERT_FATAL(0 == integrator_set_engine(integ,"IDA"));\
	/*CONSOLE_DEBUG("Assigned integrator '%s'...",integ->internals->name);*/\

#define SET_LINEAR_SAMPLELIST(START,END,NUM)\
	SampleList *samplelist = NULL;\
	{\
		/* set a linearly-distributed samplelist */\
		double start = (START), end = (END);\
		int num = (NUM);\
		dim_type d;\
		SetDimFraction(d,D_TIME,CreateFraction(1,1));\
		samplelist = samplelist_new(num+1, &d);\
		double val = start;\
		double inc = (end-start)/(num);\
		unsigned long i;\
		for(i=0; i<=num; ++i){\
			samplelist_set(samplelist,i,val);\
			val += inc;\
		}\
		integrator_set_samples(integ,samplelist);\
	}

#define CLEAN_UP(SIMINST)\
	/* destroy all that stuff */\
	CU_ASSERT((SIMINST) != NULL);\
	solver_destroy_engines();\
	integrator_free_engines();\
	sim_destroy(SIMINST);\
	Asc_CompilerDestroy();

static void test_hysteron(){
	LOAD_AND_INITIALISE("ksenija","hysteron_event");

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));
	/* TODO set some parameters? */

	/* perform problem analysis */
	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_ida_reporter);

	integrator_set_minstep(integ,0.00001);
	integrator_set_maxstep(integ,0.01);
	integrator_set_stepzero(integ,0.0001);
	integrator_set_maxsubsteps(integ,200);

	SET_LINEAR_SAMPLELIST(0., 30., 60);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	struct Instance *simroot = GetSimulationRoot(siminst);
	CU_TEST(simroot != NULL);
	struct Instance *ix = ChildByChar(simroot,AddSymbol("x"));
	struct Instance *ix0 = ChildByChar(simroot,AddSymbol("x0"));
	CU_TEST(ix != NULL);
	CU_TEST(ix0 != NULL);

	CONSOLE_DEBUG("Final x = %e",RealAtomValue(ix));
	CONSOLE_DEBUG("Final x0 = %e",RealAtomValue(ix0));

	CU_TEST(fabs(RealAtomValue(ix) - 0.7756) < 1e-4);
	CU_TEST(fabs(RealAtomValue(ix0) - -3.0353) < 1e-4);

	CLEAN_UP(siminst);
}

static void test_test1(){
	LOAD_AND_INITIALISE("ksenija","test_event1");

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));

	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_ida_reporter);

	integrator_set_minstep(integ,0.00001);
	integrator_set_maxstep(integ,0.01);
	integrator_set_stepzero(integ,0.0001);
	integrator_set_maxsubsteps(integ,200);

	SET_LINEAR_SAMPLELIST(0., 30., 100);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	struct Instance *simroot = GetSimulationRoot(siminst);
	CU_TEST(simroot != NULL);
	struct Instance *ix = ChildByChar(simroot,AddSymbol("x"));
	CU_TEST(ix != NULL);

	CONSOLE_DEBUG("Final x = %e",RealAtomValue(ix));

	CU_TEST(fabs(RealAtomValue(ix) - 16.2269) < 1e-4);

	CLEAN_UP(siminst);
}

static void test_test2(){
	LOAD_AND_INITIALISE("ksenija","test_event2");

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));

	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_ida_reporter);

	integrator_set_minstep(integ,0.00001);
	integrator_set_maxstep(integ,0.01);
	integrator_set_stepzero(integ,0.0001);
	integrator_set_maxsubsteps(integ,200);

	SET_LINEAR_SAMPLELIST(0., 5., 100);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	struct Instance *simroot = GetSimulationRoot(siminst);
	CU_TEST(simroot != NULL);
	struct Instance *ix = ChildByChar(simroot,AddSymbol("x"));
	CU_TEST(ix != NULL);

	CONSOLE_DEBUG("Final x = %e",RealAtomValue(ix));

	CU_TEST(fabs(RealAtomValue(ix) - 208.966) < 1e-4);

	CLEAN_UP(siminst);
}


static void test_successive(){
	LOAD_AND_INITIALISE("ksenija","successive");

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));
	/* TODO set some parameters? */

	/* perform problem analysis */
	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_ida_reporter);

	integrator_set_minstep(integ,0.00001);
	integrator_set_maxstep(integ,0.01);
	integrator_set_stepzero(integ,0.0001);
	integrator_set_maxsubsteps(integ,200);

	SET_LINEAR_SAMPLELIST(0., 10., 60);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	struct Instance *simroot = GetSimulationRoot(siminst);
	CU_TEST(simroot != NULL);
	struct Instance *ix = ChildByChar(simroot,AddSymbol("y"));
	CU_TEST(ix != NULL);

	CONSOLE_DEBUG("Final y = %e",RealAtomValue(ix));

	CU_TEST(fabs(RealAtomValue(ix) - 210) < 1e-4);

	CLEAN_UP(siminst);
}

static void test_bball(){
	LOAD_AND_INITIALISE("ksenija","bball_event3");

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));
	/* TODO set some parameters? */

	/* perform problem analysis */
	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_ida_reporter);

	integrator_set_minstep(integ,0.00001);
	integrator_set_maxstep(integ,0.01);
	integrator_set_stepzero(integ,0.0001);
	integrator_set_maxsubsteps(integ,200);

	SET_LINEAR_SAMPLELIST(0., 60., 100);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	struct Instance *simroot = GetSimulationRoot(siminst);
	CU_TEST(simroot != NULL);
	struct Instance *ix = ChildByChar(simroot,AddSymbol("y"));
	CU_TEST(ix != NULL);
	struct Instance *ir = ChildByChar(simroot,AddSymbol("r"));
	CU_TEST(ix != NULL);

	CONSOLE_DEBUG("Final y = %e",RealAtomValue(ix));

	CU_TEST(fabs(RealAtomValue(ix) - RealAtomValue(ir)) < 1e-4);

	CLEAN_UP(siminst);
}


static void test_solardynamics(){
	LOAD_AND_INITIALISE("johnpye","solardynamics_simple");

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));
	/* TODO set some parameters? */

	/* perform problem analysis */
	CONSOLE_DEBUG("Analysing problem...");
	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_ida_reporter);
	integrator_set_maxstep(integ,60.);
	integrator_set_stepzero(integ,1.);
	integrator_set_maxsubsteps(integ,10000);

	SET_LINEAR_SAMPLELIST(0., 23.*3600., 100);

	CONSOLE_DEBUG("Starting integration...");
	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	CLEAN_UP(siminst);
}


static void test_solardynamics2(){
	LOAD_AND_INITIALISE("johnpye","solardynamics");

	slv_parameters_t p;
	CU_ASSERT(0 == integrator_params_get(integ,&p));

	/* perform problem analysis */
	CONSOLE_DEBUG("Analysing problem...");
	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_ida_reporter);
	integrator_set_maxstep(integ,3600.);
	integrator_set_stepzero(integ,10.);
	integrator_set_maxsubsteps(integ,1000);

	SET_LINEAR_SAMPLELIST(10.*24*3600, 11.*24.*3600., 400);

	CONSOLE_DEBUG("Starting integration...");
	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

#if 0
	struct Instance *simroot = GetSimulationRoot(siminst);
	CU_TEST(simroot != NULL);
	struct Instance *ix = ChildByChar(simroot,AddSymbol("y"));
	CU_TEST(ix != NULL);
	struct Instance *ir = ChildByChar(simroot,AddSymbol("r"));
	CU_TEST(ix != NULL);

	//CONSOLE_DEBUG("Final y = %e",RealAtomValue(ix));
	//CU_TEST(fabs(RealAtomValue(ix) - RealAtomValue(ir)) < 1e-4);
#endif

	CLEAN_UP(siminst);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(hysteron) \
	T(test1) \
	T(test2) \
	T(successive) \
	T(bball) \
	T(solardynamics) \
	T(solardynamics2)

REGISTER_TESTS_SIMPLE(integrator_idaevent, TESTS)

