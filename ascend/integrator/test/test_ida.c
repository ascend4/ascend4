#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ascend/general/env.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>
#include <ascend/general/ospath.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/reverse_ad.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/slvreq.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/watchpt.h>

#include <ascend/integrator/integrator.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/slv_server.h>
#include <ascend/system/system.h>

#include <test/common.h>
#include <test/test_globals.h>

#ifndef PI
# define PI 3.14159265358979
#endif

typedef struct IdaTestSystemStruct{
	struct Instance *siminst;
	slv_system_t sys;
	IntegratorSystem *integ;
} IdaTestSystem;

static int test_ida_reporter_init(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static int test_ida_reporter_write(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static int test_ida_reporter_writeobs(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static int test_ida_reporter_close(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static IntegratorReporter test_ida_reporter = {
	test_ida_reporter_init,
	test_ida_reporter_write,
	test_ida_reporter_writeobs,
	test_ida_reporter_close
};

static int ida_find_param(const slv_parameters_t *params, const char *name){
	unsigned long i;
	for(i = 0; i < params->num_parms; ++i){
		if(params->parms[i].name != NULL && 0 == strcmp(params->parms[i].name, name)){
			return (int)i;
		}
	}
	return -1;
}

static void ida_set_char_option(IntegratorSystem *integ, const char *name, const char *value){
	slv_parameters_t params;
	int idx;

	CU_ASSERT_FATAL(0 == integrator_params_get(integ, &params));
	idx = ida_find_param(&params, name);
	CU_ASSERT_FATAL(idx >= 0);
	slv_set_char_parameter(&(SLV_PARAM_CHAR(&params, idx)), value);
	CU_ASSERT_FATAL(0 == integrator_params_set(integ, &params));
}

static SampleList *ida_create_samplelist(double start, double end, int num_steps){
	dim_type d;
	SampleList *samplelist;
	double value, inc;
	int i;

	SetDimFraction(d, D_TIME, CreateFraction(1,1));
	samplelist = samplelist_new(num_steps + 1, &d);
	value = start;
	inc = (end - start) / num_steps;
	for(i = 0; i <= num_steps; ++i){
		samplelist_set(samplelist, i, value);
		value += inc;
	}
	return samplelist;
}

static void ida_cleanup(IdaTestSystem *testsys){
	slv_system_t sys = NULL;
	if(testsys->integ != NULL){
		sys = testsys->integ->system;
		integrator_free(testsys->integ);
		testsys->integ = NULL;
	}else{
		sys = testsys->sys;
	}
	if(sys != NULL){
		system_destroy(sys);
	}
	if(testsys->sys != NULL && testsys->sys != sys){
		system_destroy(testsys->sys);
	}
	testsys->sys = NULL;
	system_free_reused_mem();
	solver_destroy_engines();
	integrator_free_engines();
	if(testsys->siminst != NULL){
		sim_destroy(testsys->siminst);
		testsys->siminst = NULL;
	}
	Asc_CompilerDestroy();
}

static int ida_run_method(struct Instance *root, const char *method){
	struct Name *name = CreateIdName(AddSymbol(method));
	return Initialize(root, name, "sim1", ASCERR, WP_STOPONERR, NULL, NULL);
}

static int ida_test_load(const char *module_path, const char *type_name, int need_lrslv, IdaTestSystem *testsys){
	int status;
	struct Instance *root;

	memset(testsys, 0, sizeof(*testsys));

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lrslv" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");

	if(need_lrslv && 0 != package_load("lrslv", NULL)){
		ida_cleanup(testsys);
		CONSOLE_DEBUG("Skipping IDA test: lrslv not available");
		return 1;
	}

	Asc_OpenModule(module_path, &status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(type_name)) != NULL);

	testsys->siminst = SimsCreateInstance(AddSymbol(type_name), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(testsys->siminst != NULL);
	root = GetSimulationRoot(testsys->siminst);
	CU_ASSERT_FATAL(root != NULL);

	CU_ASSERT_FATAL(Proc_all_ok == ida_run_method(root, "on_load"));

	testsys->sys = system_build(root);
	CU_ASSERT_FATAL(testsys->sys != NULL);

	testsys->integ = integrator_new(testsys->sys, root);
	CU_ASSERT_FATAL(testsys->integ != NULL);
	testsys->sys = NULL;

	if(0 != integrator_set_engine(testsys->integ, "IDA")){
		ida_cleanup(testsys);
		CONSOLE_DEBUG("Skipping IDA test: integrator not available");
		return 1;
	}

	ida_set_char_option(testsys->integ, "linsolver", "DENSE");
	ida_set_char_option(testsys->integ, "prec", "NONE");
	return 0;
}

static void ida_expect_system_build_failure(const char *module_path, const char *type_name, int need_lrslv){
	IdaTestSystem testsys;
	int status;
	struct Instance *root;

	memset(&testsys, 0, sizeof(testsys));

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lrslv" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");

	if(need_lrslv && 0 != package_load("lrslv", NULL)){
		ida_cleanup(&testsys);
		CONSOLE_DEBUG("Skipping IDA test: lrslv not available");
		return;
	}

	Asc_OpenModule(module_path, &status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(type_name)) != NULL);

	testsys.siminst = SimsCreateInstance(AddSymbol(type_name), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(testsys.siminst != NULL);
	root = GetSimulationRoot(testsys.siminst);
	CU_ASSERT_FATAL(root != NULL);
	CU_ASSERT_FATAL(Proc_all_ok == ida_run_method(root, "on_load"));

	testsys.sys = system_build(root);
	CU_TEST(testsys.sys == NULL);

	ida_cleanup(&testsys);
}

static void ida_expect_instantiation_error(const char *module_path, const char *type_name, int need_lrslv){
	IdaTestSystem testsys;
	int status;
	int has_error;

	memset(&testsys, 0, sizeof(testsys));

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/lrslv" OSPATH_DIV "solvers/lsode" OSPATH_DIV "solvers/qrslv");

	if(need_lrslv && 0 != package_load("lrslv", NULL)){
		ida_cleanup(&testsys);
		CONSOLE_DEBUG("Skipping IDA test: lrslv not available");
		return;
	}

	Asc_OpenModule(module_path, &status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(type_name)) != NULL);

	error_reporter_tree_start();
	testsys.siminst = SimsCreateInstance(AddSymbol(type_name), AddSymbol("sim1"), e_normal, NULL);
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error);

	ida_cleanup(&testsys);
}

static void ida_expect_system_build_error(const char *module_path, const char *type_name, int need_lrslv){
	IdaTestSystem testsys;
	int has_error;

	if(ida_test_load(module_path, type_name, need_lrslv, &testsys)){
		return;
	}

	error_reporter_tree_start();
	testsys.sys = system_build(GetSimulationRoot(testsys.siminst));
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	CU_ASSERT(has_error);

	ida_cleanup(&testsys);
}

static void ida_configure_runtime(IntegratorSystem *integ, double start, double end, int num_steps){
	SampleList *samplelist = ida_create_samplelist(start, end, num_steps);
	integrator_set_reporter(integ, &test_ida_reporter);
	integrator_set_minstep(integ, 1e-4);
	integrator_set_maxstep(integ, 0.5);
	integrator_set_stepzero(integ, 1e-3);
	integrator_set_maxsubsteps(integ, 1000);
	integrator_set_samples(integ, samplelist);
}

static void ida_free_runtime(IntegratorSystem *integ){
	SampleList *samplelist = integ->samples;
	if(samplelist != NULL){
		samplelist_free(samplelist);
		integrator_set_samples(integ, NULL);
	}
}

static struct Instance *ida_child(struct Instance *root, const char *name){
	struct Instance *child = ChildByChar(root, AddSymbol(name));
	CU_ASSERT_FATAL(child != NULL);
	return child;
}

static char *ida_capture_pantelides_report(slv_system_t sys){
	char *buf = NULL;
	size_t len = 0;
	FILE *fp = open_memstream(&buf, &len);
	CU_ASSERT_FATAL(fp != NULL);
	CU_ASSERT_FATAL(0 == integrator_pantelides_advisory(sys, fp));
	CU_ASSERT_FATAL(0 == fclose(fp));
	CU_ASSERT_FATAL(buf != NULL);
	return buf;
}

static void test_shm(){
	IdaTestSystem testsys;
	struct Instance *root, *ix, *iv;

	if(ida_test_load("test/ida/shm.a4c", "shm", 0, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, PI, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	ix = ida_child(root, "x");
	iv = ida_child(root, "v");
	CU_TEST(fabs(RealAtomValue(ix) + 10.0) < 3e-3);
	CU_TEST(fabs(RealAtomValue(iv)) < 4e-4);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_boundary(){
	IdaTestSystem testsys;
	struct Instance *root, *iy, *ir, *iv, *im, *ig, *ik1;
	double y, r, v, m, g, k1, yeq;

	if(ida_test_load("test/ida/leon/bouncingball.a4c", "bouncingball", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 30.0, 120);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");
	ir = ida_child(root, "r");
	iv = ida_child(root, "v");
	im = ida_child(root, "m");
	ig = ida_child(root, "g");
	ik1 = ida_child(root, "k1");
	y = RealAtomValue(iy);
	r = RealAtomValue(ir);
	v = RealAtomValue(iv);
	m = RealAtomValue(im);
	g = RealAtomValue(ig);
	k1 = RealAtomValue(ik1);
	yeq = r - m * g / k1;

	CU_TEST(fabs(y - yeq) < 5e-4);
	CU_TEST(y < 25.0);
	CU_TEST(fabs(v) < 1e-3);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_integ1(){
	IdaTestSystem testsys;
	struct Instance *root, *iy, *ix, *it;

	if(ida_test_load("test/ida/integ1.a4c", "integ1", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 10.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");
	ix = ida_child(root, "x");
	it = ida_child(root, "t");

	CU_TEST(fabs(RealAtomValue(it) - 10.0) < 1e-5);
	CU_TEST(fabs(RealAtomValue(ix) - 1.0) < 1e-10);
	CU_TEST(fabs(RealAtomValue(iy) - 10.0) < 1e-5);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_reflect(){
	IdaTestSystem testsys;
	struct Instance *root, *iy, *iv, *it, *itlast;
	int i;

	if(ida_test_load("test/ida/reinit.a4c", "ida_reinit_reflect", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");
	iv = ida_child(root, "v");
	it = ida_child(root, "t");
	itlast = ida_child(root, "t_last_event");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != itlast);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(iv) - 1.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(itlast) - 1.0) < 5e-5);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_discrete_sawtooth(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *iperiod, *itlast;
	int i;

	if(ida_test_load("test/ida/reinit_discrete.a4c", "ida_reinit_discrete_sawtooth", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 4.0, 80);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	iperiod = ida_child(root, "period");
	itlast = ida_child(root, "t_last_event");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != iperiod);
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != itlast);
	}

	CU_TEST(fabs(RealAtomValue(it) - 4.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 1.5) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 1.5) < 5e-5);
	CU_TEST(fabs(RealAtomValue(iperiod) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(itlast) - 2.5) < 5e-5);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_multi_boundary_same_direction(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *iout1, *iout2, *isw1, *isw2;

	if(ida_test_load("test/ida/multi_boundary.a4c", "ida_multi_boundary_same_direction", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 1.2, 24);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	iout1 = ida_child(root, "out1");
	iout2 = ida_child(root, "out2");
	isw1 = ida_child(root, "switch1");
	isw2 = ida_child(root, "switch2");

	CU_TEST(fabs(RealAtomValue(it) - 1.2) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 1.2) < 5e-5);
	CU_TEST(fabs(RealAtomValue(iout1) - 1.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(iout2) - 1.0) < 5e-5);
	CU_TEST(GetBooleanAtomValue(isw1));
	CU_TEST(GetBooleanAtomValue(isw2));

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_boolean_latch(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *itrigger, *ilatched;

	if(ida_test_load("test/ida/reinit_bool.a4c", "ida_reinit_boolean_latch", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	itrigger = ida_child(root, "trigger");
	ilatched = ida_child(root, "latched");

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 2.0) < 5e-5);
	CU_TEST(GetBooleanAtomValue(itrigger));
	CU_TEST(GetBooleanAtomValue(ilatched));

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_boolean_cascade(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *istage, *itrigger, *ilatched;
	int i;

	if(ida_test_load("test/ida/reinit_bool_cascade.a4c", "ida_reinit_boolean_cascade", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	istage = ida_child(root, "stage");
	itrigger = ida_child(root, "trigger");
	ilatched = ida_child(root, "latched");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != istage);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(istage) - 2.0) < 5e-5);
	CU_TEST(GetBooleanAtomValue(itrigger));
	CU_TEST(GetBooleanAtomValue(ilatched));

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_algebraic_target_rejected(){
	ida_expect_system_build_failure("test/ida/reinit_algebraic_bad.a4c", "ida_reinit_bad_algebraic", 1);
}

static void test_reinit_indep_target_rejected(){
	ida_expect_system_build_failure("test/ida/reinit_bad_targets.a4c", "ida_reinit_bad_indep", 1);
}

static void test_reinit_derivative_target_rejected(){
	ida_expect_system_build_failure("test/ida/reinit_bad_targets.a4c", "ida_reinit_bad_derivative", 1);
}

static void test_reinit_boolean_rhs_type_rejected(){
	IdaTestSystem testsys;

	if(ida_test_load("test/ida/reinit_bad_targets.a4c", "ida_reinit_bad_boolean_rhs", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_NOT_EQUAL(integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1), 0);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_real_rhs_type_rejected(){
	IdaTestSystem testsys;

	if(ida_test_load("test/ida/reinit_bad_targets.a4c", "ida_reinit_bad_real_rhs", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_NOT_EQUAL(integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1), 0);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_integer_mode_switch(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *istage;
	int i;

	if(ida_test_load("test/ida/reinit_selector_prelude.a4c", "ida_reinit_integer_mode_switch", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	istage = ida_child(root, "stage");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != istage);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 12.0) < 5e-5);
	CU_TEST(GetIntegerAtomValue(istage) == 1);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_reinit_symbol_mode_switch(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *imode;
	int i;

	if(ida_test_load("test/ida/reinit_selector_prelude.a4c", "ida_reinit_symbol_mode_switch", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	imode = ida_child(root, "mode");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != imode);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 12.0) < 5e-5);
	CU_ASSERT_STRING_EQUAL(SCP(GetSymbolAtomValue(imode)), "high");

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_switchto_integer_mode(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *istage;
	int i;

	if(ida_test_load("test/ida/switchto.a4c", "ida_switchto_integer_mode", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	istage = ida_child(root, "stage");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != istage);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 12.0) < 5e-5);
	CU_TEST(GetIntegerAtomValue(istage) == 1);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_switchto_symbol_mode(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *imode;
	int i;

	if(ida_test_load("test/ida/switchto.a4c", "ida_switchto_symbol_mode", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	imode = ida_child(root, "mode");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != imode);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 12.0) < 5e-5);
	CU_ASSERT_STRING_EQUAL(SCP(GetSymbolAtomValue(imode)), "high");

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_switchto_selector_mode(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *imode;
	int i;

	if(ida_test_load("test/ida/switchto.a4c", "ida_switchto_selector_mode", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	imode = ida_child(root, "mode");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != imode);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 12.0) < 5e-5);
	CU_ASSERT_STRING_EQUAL(SCP(GetSymbolAtomValue(imode)), "high");

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_switchto_selector_bad_switch_rejected(){
	ida_expect_system_build_error("test/ida/switchto.a4c", "ida_switchto_selector_bad_switch", 1);
}

static void test_switchto_selector_bad_case_rejected(){
	ida_expect_system_build_error("test/ida/switchto.a4c", "ida_switchto_selector_bad_case", 1);
}

static void test_switchto_selector_bad_default_rejected(){
	ida_expect_instantiation_error("test/ida/switchto.a4c", "ida_switchto_selector_bad_default", 1);
}

static void test_switchto_nonselector_default_rejected(){
	ida_expect_instantiation_error("test/ida/switchto.a4c", "ida_switchto_nonselector_default_bad", 1);
}

static void test_when_integer_initial_case(){
	IdaTestSystem testsys;
	struct Instance *root, *iy, *isample, *istage;
	int i;

	if(ida_test_load("test/ida/when_nonboolean_probe.a4c", "ida_when_integer_initial_case", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	istage = ida_child(root, "stage");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != istage);
	}

	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 12.0) < 5e-5);
	CU_TEST(GetIntegerAtomValue(istage) == 1);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_when_symbol_initial_case(){
	IdaTestSystem testsys;
	struct Instance *root, *iy, *isample, *imode;
	int i;

	if(ida_test_load("test/ida/when_nonboolean_probe.a4c", "ida_when_symbol_initial_case", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	imode = ida_child(root, "mode");

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != imode);
	}

	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 12.0) < 5e-5);
	CU_ASSERT_STRING_EQUAL(SCP(GetSymbolAtomValue(imode)), "high");

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_example_ideal_rebound(){
	IdaTestSystem testsys;
	struct Instance *root, *iy, *iv, *it, *itlast;
	int i;

	if(ida_test_load("johnpye/dyn/ideal_rebound.a4c", "ideal_rebound", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 2.0, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");
	iv = ida_child(root, "v");
	it = ida_child(root, "t");
	itlast = ida_child(root, "t_last_event");

	CU_ASSERT_EQUAL(testsys.integ->n_obs, 4);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[0]), it);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[1]), iy);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[2]), iv);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[3]), itlast);

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != itlast);
	}

	CU_TEST(fabs(RealAtomValue(it) - 2.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(iv) - 1.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(itlast) - 1.0) < 5e-5);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_example_lengthening_sawtooth(){
	IdaTestSystem testsys;
	struct Instance *root, *it, *iy, *isample, *iperiod, *itlast;
	int i;

	if(ida_test_load("johnpye/dyn/lengthening_sawtooth.a4c", "lengthening_sawtooth", 1, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 4.0, 80);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));

	root = GetSimulationRoot(testsys.siminst);
	it = ida_child(root, "t");
	iy = ida_child(root, "y");
	isample = ida_child(root, "sample");
	iperiod = ida_child(root, "period");
	itlast = ida_child(root, "t_last_event");

	CU_ASSERT_EQUAL(testsys.integ->n_obs, 5);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[0]), it);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[1]), iy);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[2]), isample);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[3]), iperiod);
	CU_ASSERT_PTR_EQUAL(var_instance(testsys.integ->obs[4]), itlast);

	for(i = 0; i < testsys.integ->n_y; ++i){
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != iperiod);
		CU_TEST(testsys.integ->y[i] == NULL || var_instance(testsys.integ->y[i]) != itlast);
	}

	CU_TEST(fabs(RealAtomValue(it) - 4.0) < 1e-8);
	CU_TEST(fabs(RealAtomValue(iy) - 1.5) < 5e-5);
	CU_TEST(fabs(RealAtomValue(isample) - 1.5) < 5e-5);
	CU_TEST(fabs(RealAtomValue(iperiod) - 2.0) < 5e-5);
	CU_TEST(fabs(RealAtomValue(itlast) - 2.5) < 5e-5);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_high_index(){
	IdaTestSystem testsys;

	if(ida_test_load("test/ida/highindex.a4c", "ida_highindex", 0, &testsys)){
		return;
	}

	CU_ASSERT_NOT_EQUAL(integrator_analyse(testsys.integ), 0);
	ida_cleanup(&testsys);
}

static void test_pantelides_pendulum_high_index(){
	IdaTestSystem testsys;
	char *report;

	if(ida_test_load("test/pantelides/pendulum.a4c", "pantelides_pendulum", 0, &testsys)){
		return;
	}

	CU_ASSERT_NOT_EQUAL(integrator_analyse(testsys.integ), 0);
	report = ida_capture_pantelides_report(testsys.integ->system);
	CU_ASSERT_PTR_NOT_NULL_FATAL(report);
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "Current derivative chains"));
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "eq5:"));
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "vx represents der(x) via eq1"));
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "vy represents der(y) via eq2"));
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "Differentiate eq5"));
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "Differentiate d/dt(eq5)"));
	free(report);
	ida_cleanup(&testsys);
}

static void test_pantelides_reactor_high_index(){
	IdaTestSystem testsys;
	char *report;

	if(ida_test_load("test/pantelides/reactor.a4c", "pantelides_reactor", 0, &testsys)){
		return;
	}

	CU_ASSERT_NOT_EQUAL(integrator_analyse(testsys.integ), 0);
	report = ida_capture_pantelides_report(testsys.integ->system);
	CU_ASSERT_PTR_NOT_NULL_FATAL(report);
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "Current derivative chains"));
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "input_constraint"));
	CU_ASSERT_PTR_NOT_NULL(strstr(report, "Advisory analysis limit reached"));
	free(report);
	ida_cleanup(&testsys);
}

static void test_initial_decay(){
	IdaTestSystem testsys;
	struct Instance *root, *iy;

	if(ida_test_load("test/ida/initial.a4c", "ida_initial_decay", 0, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");

	ida_configure_runtime(testsys.integ, 0.0, 1.0, 20);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));
	CU_TEST(fabs(RealAtomValue(iy) - exp(-4.0)) < 2e-4);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_initial_shm(){
	IdaTestSystem testsys;
	struct Instance *root, *ix, *iv;

	if(ida_test_load("test/ida/initial.a4c", "ida_initial_shm", 0, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	root = GetSimulationRoot(testsys.siminst);
	ix = ida_child(root, "x");
	iv = ida_child(root, "v");

	ida_configure_runtime(testsys.integ, 0.0, PI, 40);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));
	CU_TEST(fabs(RealAtomValue(ix) + 10.0) < 3e-3);
	CU_TEST(fabs(RealAtomValue(iv)) < 4e-4);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_initial_hier_decay(){
	IdaTestSystem testsys;
	struct Instance *root, *child, *iy;

	if(ida_test_load("test/ida/initial.a4c", "ida_initial_hier_decay", 0, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	root = GetSimulationRoot(testsys.siminst);
	child = ida_child(root, "c");
	iy = ida_child(child, "y");

	ida_configure_runtime(testsys.integ, 0.0, 1.0, 20);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));
	CU_TEST(fabs(RealAtomValue(iy) - exp(-4.0)) < 2e-4);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_initial_dae(){
	IdaTestSystem testsys;
	struct Instance *root, *iy, *iz;

	if(ida_test_load("test/ida/initial.a4c", "ida_initial_dae", 0, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	root = GetSimulationRoot(testsys.siminst);
	iy = ida_child(root, "y");
	iz = ida_child(root, "z");

	ida_configure_runtime(testsys.integ, 0.0, 1.0, 20);
	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1));
	CU_TEST(fabs(RealAtomValue(iy) - exp(-2.0)) < 3e-4);
	CU_TEST(fabs(RealAtomValue(iz) - 2.0 * exp(-2.0)) < 8e-4);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
}

static void test_initial_bad_overdetermined(){
	IdaTestSystem testsys;
	int solve_res;

	if(ida_test_load("test/ida/initial.a4c", "ida_initial_bad_overdetermined", 0, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 1.0, 20);
	solve_res = integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
	CU_ASSERT(0 != solve_res);
}

static void test_initial_alias_binding_bug(){
	IdaTestSystem testsys;
	int solve_res;

	if(ida_test_load("test/ida/initial_alias.a4c", "ida_initial_alias_binding_bug", 0, &testsys)){
		return;
	}

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	ida_configure_runtime(testsys.integ, 0.0, 1.0, 20);
	solve_res = integrator_solve(testsys.integ, 0, samplelist_length(testsys.integ->samples) - 1);

	ida_free_runtime(testsys.integ);
	ida_cleanup(&testsys);
	CU_ASSERT_FATAL(0 == solve_res);
}

#define TESTS(T) \
	T(shm) \
	T(boundary) \
	T(integ1) \
	T(reinit_reflect) \
	T(reinit_discrete_sawtooth) \
	T(multi_boundary_same_direction) \
	T(reinit_boolean_latch) \
	T(reinit_boolean_cascade) \
	T(reinit_algebraic_target_rejected) \
	T(reinit_indep_target_rejected) \
	T(reinit_derivative_target_rejected) \
	T(reinit_boolean_rhs_type_rejected) \
	T(reinit_real_rhs_type_rejected) \
	T(reinit_integer_mode_switch) \
	T(reinit_symbol_mode_switch) \
	T(switchto_integer_mode) \
	T(switchto_symbol_mode) \
	T(switchto_selector_mode) \
	T(switchto_selector_bad_switch_rejected) \
	T(switchto_selector_bad_case_rejected) \
	T(switchto_selector_bad_default_rejected) \
	T(switchto_nonselector_default_rejected) \
	T(when_integer_initial_case) \
	T(when_symbol_initial_case) \
	T(example_ideal_rebound) \
	T(example_lengthening_sawtooth) \
	T(high_index) \
	T(pantelides_pendulum_high_index) \
	T(pantelides_reactor_high_index) \
	T(initial_decay) \
	T(initial_shm) \
	T(initial_hier_decay) \
	T(initial_dae) \
	T(initial_bad_overdetermined) \
	T(initial_alias_binding_bug)

REGISTER_TESTS_SIMPLE(integrator_ida, TESTS)
