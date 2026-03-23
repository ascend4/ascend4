#include <math.h>
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
	return 1; /* no interrupt */
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

#ifndef PI
# define PI 3.14159265358979
#endif

static SampleList *test_lsode_create_samplelist(double start, double end, int num){
	dim_type d;
	SampleList *samplelist;
	double val, inc;
	unsigned long i;

	SetDimFraction(d,D_TIME,CreateFraction(1,1));
	samplelist = samplelist_new(num + 1, &d);
	val = start;
	inc = (end - start) / num;
	for(i = 0; i <= (unsigned long)num; ++i){
		samplelist_set(samplelist, i, val);
		val += inc;
	}
	return samplelist;
}

static struct Instance *test_lsode_load_model(const char *path, const char *modelname){
	int status;
	struct Instance *siminst;

	Asc_OpenModule(path, &status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(modelname)) != NULL);

	siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	return siminst;
}

static IntegratorSystem *test_lsode_prepare_integrator(
	struct Instance *siminst,
	double minstep,
	double maxstep,
	double stepzero,
	int maxsubsteps
){
	slv_system_t sys;
	IntegratorSystem *integ;
	int index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(index != -1);

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,index));

	integ = integrator_new(sys,siminst);
	CU_ASSERT_FATAL(integ != NULL);
	CU_ASSERT_FATAL(0 == integrator_set_engine(integ,"LSODE"));
	CU_ASSERT_FATAL(0 == integrator_analyse(integ));

	integrator_set_reporter(integ, &test_lsode_reporter);
	integrator_set_minstep(integ,minstep);
	integrator_set_maxstep(integ,maxstep);
	integrator_set_stepzero(integ,stepzero);
	integrator_set_maxsubsteps(integ,maxsubsteps);
	return integ;
}

static void test_lsode_destroy_integrator(IntegratorSystem *integ){
	struct Instance *siminst;
	slv_system_t sys;
	if(integ == NULL){
		return;
	}
	sys = integ->system;
	siminst = integ->instance;
	integrator_free(integ);
	if(sys != NULL){
		system_destroy(sys);
	}
	system_free_reused_mem();
	solver_destroy_engines();
	integrator_free_engines();
	if(siminst != NULL){
		sim_destroy(siminst);
	}
	Asc_CompilerDestroy();
}

/*
	Test solving a simple LSODE model. This test integrates a model that deliberately
	goes out of bounds, and checks that LSODE catches and aborts.

	TODO we need to add a test for normal completion of LSODE!
*/
static void test_bounds(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_TEST_FATAL(0 == package_load("qrslv",NULL));

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *)path,"test/lsode/");
#define FILESTEM "bounds"
	strncat(path, FILESTEM, PATH_MAX - strlen(path));
	strncat(path, ".a4c", PATH_MAX - strlen(path));
#define MODELNAME "boundsfail"
		struct Instance *siminst = test_lsode_load_model(path, MODELNAME);

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
		SampleList *samplelist = test_lsode_create_samplelist(0, 10, 20);
		integrator_set_samples(integ,samplelist);

	CONSOLE_DEBUG("Commencing solve...");
	CU_ASSERT_FATAL(integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	integrator_free(integ);
	samplelist_free(samplelist);

	CU_ASSERT_FATAL(NULL != sys);
	system_destroy(sys);
	system_free_reused_mem();

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(siminst != NULL);

	solver_destroy_engines();
	integrator_free_engines();
	sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_shm(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/ida/shm.a4c", "shm");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	int index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(index != -1);
	IntegratorSystem *integ = test_lsode_prepare_integrator(siminst,0,0.5,1e-3,1000);
	slv_system_t sys = integ->system;

	SampleList *samplelist = test_lsode_create_samplelist(0.0, PI, 40);
	integrator_set_samples(integ,samplelist);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	struct Instance *root = GetSimulationRoot(siminst);
	struct Instance *ix = ChildByChar(root, AddSymbol("x"));
	struct Instance *iv = ChildByChar(root, AddSymbol("v"));
	CU_ASSERT_FATAL(ix != NULL);
	CU_ASSERT_FATAL(iv != NULL);
	CU_TEST(fabs(RealAtomValue(ix) + 10.0) < 3e-3);
	CU_TEST(fabs(RealAtomValue(iv)) < 7e-4);

	samplelist_free(samplelist);
	test_lsode_destroy_integrator(integ);
}

static void test_der_decay(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/lsode/deriv.a4c", "der_decay");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	IntegratorSystem *integ = test_lsode_prepare_integrator(siminst,0,0.1,1e-3,1000);
	SampleList *samplelist = test_lsode_create_samplelist(0.0, 1.0, 20);
	integrator_set_samples(integ,samplelist);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	struct Instance *root = GetSimulationRoot(siminst);
	struct Instance *iy = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(iy != NULL);
	CU_TEST(fabs(RealAtomValue(iy) - exp(-4.0)) < 2e-4);

	samplelist_free(samplelist);
	test_lsode_destroy_integrator(integ);
}

static void test_der_shm(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/lsode/deriv.a4c", "der_shm");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	IntegratorSystem *integ = test_lsode_prepare_integrator(siminst,0,0.5,1e-3,1000);
	SampleList *samplelist = test_lsode_create_samplelist(0.0, PI, 40);
	integrator_set_samples(integ,samplelist);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));

	struct Instance *root = GetSimulationRoot(siminst);
	struct Instance *ix = ChildByChar(root, AddSymbol("x"));
	struct Instance *iv = ChildByChar(root, AddSymbol("v"));
	CU_ASSERT_FATAL(ix != NULL);
	CU_ASSERT_FATAL(iv != NULL);
	CU_TEST(fabs(RealAtomValue(ix) + 10.0) < 3e-3);
	CU_TEST(fabs(RealAtomValue(iv)) < 7e-4);

	samplelist_free(samplelist);
	test_lsode_destroy_integrator(integ);
}

static void test_initial_decay(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/lsode/deriv.a4c", "initial_decay");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	IntegratorSystem *integ = test_lsode_prepare_integrator(siminst,0,0.1,1e-3,1000);
	struct Instance *root = GetSimulationRoot(siminst);
	struct Instance *iy = ChildByChar(root, AddSymbol("y"));
	CU_ASSERT_FATAL(iy != NULL);

	SampleList *samplelist = test_lsode_create_samplelist(0.0, 1.0, 20);
	integrator_set_samples(integ,samplelist);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));
	CU_TEST(fabs(RealAtomValue(iy) - exp(-4.0)) < 2e-4);

	samplelist_free(samplelist);
	test_lsode_destroy_integrator(integ);
}

static void test_initial_shm(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/lsode/deriv.a4c", "initial_shm");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	IntegratorSystem *integ = test_lsode_prepare_integrator(siminst,0,0.5,1e-3,1000);
	struct Instance *root = GetSimulationRoot(siminst);
	struct Instance *ix = ChildByChar(root, AddSymbol("x"));
	struct Instance *iv = ChildByChar(root, AddSymbol("v"));
	CU_ASSERT_FATAL(ix != NULL);
	CU_ASSERT_FATAL(iv != NULL);

	SampleList *samplelist = test_lsode_create_samplelist(0.0, PI, 40);
	integrator_set_samples(integ,samplelist);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));
	CU_TEST(fabs(RealAtomValue(ix) + 10.0) < 3e-3);
	CU_TEST(fabs(RealAtomValue(iv)) < 7e-4);

	samplelist_free(samplelist);
	test_lsode_destroy_integrator(integ);
}

static void test_initial_hier_decay(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/lsode/deriv.a4c", "initial_hier_decay");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	IntegratorSystem *integ = test_lsode_prepare_integrator(siminst,0,0.1,1e-3,1000);
	struct Instance *root = GetSimulationRoot(siminst);
	struct Instance *child = ChildByChar(root, AddSymbol("c"));
	struct Instance *iy = child ? ChildByChar(child, AddSymbol("y")) : NULL;
	CU_ASSERT_FATAL(child != NULL);
	CU_ASSERT_FATAL(iy != NULL);

	SampleList *samplelist = test_lsode_create_samplelist(0.0, 1.0, 20);
	integrator_set_samples(integ,samplelist);

	CU_ASSERT_FATAL(0 == integrator_solve(integ, 0, samplelist_length(samplelist)-1));
	CU_TEST(fabs(RealAtomValue(iy) - exp(-4.0)) < 2e-4);

	samplelist_free(samplelist);
	test_lsode_destroy_integrator(integ);
}

static void test_initial_bad_overdetermined(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/lsode/deriv.a4c", "initial_bad_overdetermined");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	IntegratorSystem *integ = test_lsode_prepare_integrator(siminst,0,0.1,1e-3,1000);
	SampleList *samplelist = test_lsode_create_samplelist(0.0, 1.0, 20);
	int solve_res;
	integrator_set_samples(integ,samplelist);

	solve_res = integrator_solve(integ, 0, samplelist_length(samplelist)-1);

	samplelist_free(samplelist);
	test_lsode_destroy_integrator(integ);
	CU_ASSERT(0 != solve_res);
}

static void test_pantelides_pendulum_high_index(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/pantelides/pendulum.a4c", "pantelides_pendulum");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	int index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(index != -1);

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,index));

	IntegratorSystem *integ = integrator_new(sys,siminst);
	CU_ASSERT_FATAL(integ != NULL);
	CU_ASSERT_FATAL(0 == integrator_set_engine(integ,"LSODE"));

	CU_ASSERT_NOT_EQUAL(integrator_analyse(integ), 0);

	test_lsode_destroy_integrator(integ);
}

static void test_pantelides_reactor_high_index(){
	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv" OSPATH_DIV "solvers/lsode"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	struct Instance *siminst = test_lsode_load_model("test/pantelides/reactor.a4c", "pantelides_reactor");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe == Proc_all_ok);

	int index = slv_lookup_client("QRSlv");
	CU_ASSERT_FATAL(index != -1);

	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,index));

	IntegratorSystem *integ = integrator_new(sys,siminst);
	CU_ASSERT_FATAL(integ != NULL);
	CU_ASSERT_FATAL(0 == integrator_set_engine(integ,"LSODE"));

	CU_ASSERT_NOT_EQUAL(integrator_analyse(integ), 0);

	test_lsode_destroy_integrator(integ);
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(bounds) \
	T(shm) \
	T(der_decay) \
	T(der_shm) \
	T(initial_decay) \
	T(initial_shm) \
	T(initial_hier_decay) \
	T(initial_bad_overdetermined) \
	T(pantelides_pendulum_high_index) \
	T(pantelides_reactor_high_index)

REGISTER_TESTS_SIMPLE(integrator_lsode, TESTS)
