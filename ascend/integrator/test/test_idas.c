#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/utilities/ascEnvVar.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/slvreq.h>
#include <ascend/compiler/symtab.h>

#include <ascend/integrator/integrator.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/slv_server.h>
#include <ascend/system/system.h>
#include <ascend/system/rel.h>
#include <ascend/system/relman.h>
#include <ascend/system/var.h>

#include <test/common.h>

typedef struct IdasTestSystemStruct{
	struct Instance *siminst;
	slv_system_t sys;
	IntegratorSystem *integ;
} IdasTestSystem;

typedef struct IdasMethodHooksStruct{
	int solver_selected;
	int integrator_selected;
} IdasMethodHooks;

typedef struct TwinslabsAnalyticStruct{
	double t1;
	double t2;
	double dt1_dhc;
	double dt2_dhc;
} TwinslabsAnalytic;

static int idas_hook_set_solver(const char *solvername, void *user_data){
	IdasMethodHooks *hooks = (IdasMethodHooks *)user_data;
	if(slv_lookup_client(solvername) == -1){
		return SLVREQ_UNKNOWN_SOLVER;
	}
	hooks->solver_selected = 1;
	return 0;
}

static int idas_hook_set_integrator(const char *integratorname, void *user_data){
	IdasMethodHooks *hooks = (IdasMethodHooks *)user_data;
	if(0 == strcmp(integratorname, "IDA") || 0 == strcmp(integratorname, "IDAS")){
		hooks->integrator_selected = 1;
		return 0;
	}
	return SLVREQ_UNKNOWN_INTEGRATOR;
}

static int idas_hook_set_option(const char *optionname, struct value_t *val, void *user_data){
	IdasMethodHooks *hooks = (IdasMethodHooks *)user_data;
	(void)optionname;
	(void)val;
	if(hooks->integrator_selected || hooks->solver_selected){
		return 0;
	}
	return SLVREQ_OPTIONS_UNAVAILABLE;
}

static int idas_hook_do_solve(struct Instance *instance, void *user_data){
	IdasMethodHooks *hooks = (IdasMethodHooks *)user_data;
	(void)instance;
	if(!hooks->solver_selected){
		return SLVREQ_NO_SOLVER_SELECTED;
	}
	return 0;
}

static int idas_hook_do_observe(const SlvReqObserveRequest *request, void *user_data){
	(void)request;
	(void)user_data;
	return 0;
}

static int idas_hook_do_study(const SlvReqStudyRequest *request, void *user_data){
	(void)request;
	(void)user_data;
	return 0;
}

static int idas_hook_do_integrate(const SlvReqIntegrateRequest *request, void *user_data){
	IdasMethodHooks *hooks = (IdasMethodHooks *)user_data;
	(void)request;
	if(!hooks->integrator_selected){
		return SLVREQ_NO_INTEGRATOR_SELECTED;
	}
	return 0;
}

static int idas_hook_delete_system(void *user_data){
	(void)user_data;
	return 0;
}

static void idas_assign_method_hooks(struct Instance *siminst, IdasMethodHooks *hookstate){
	SlvReqHooks hooks = {
		.set_solver_fn = &idas_hook_set_solver,
		.set_integrator_fn = &idas_hook_set_integrator,
		.set_option_fn = &idas_hook_set_option,
		.do_solve_fn = &idas_hook_do_solve,
		.do_observe_fn = &idas_hook_do_observe,
		.do_study_fn = &idas_hook_do_study,
		.do_integrate_fn = &idas_hook_do_integrate,
		.delete_system_fn = &idas_hook_delete_system,
		.user_data = hookstate
	};
	slvreq_assign_hooks(siminst, &hooks);
}

static int idas_reporter_init(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static int idas_reporter_write(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static int idas_reporter_writeobs(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static int idas_reporter_close(struct IntegratorSystemStruct *integ) {
	(void)integ;
	return 0;
}

static IntegratorReporter idas_reporter = {
	idas_reporter_init,
	idas_reporter_write,
	idas_reporter_writeobs,
	idas_reporter_close
};

static int idas_run_method(struct Instance *root, const char *method){
	struct Name *name = CreateIdName(AddSymbol(method));
	return Initialize(root, name, "sim_idas", ASCERR, WP_STOPONERR, NULL, NULL);
}

static SampleList *idas_create_samplelist(double start, double end, int num_steps){
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

static int idas_find_param(const slv_parameters_t *params, const char *name){
	unsigned long i;
	for(i = 0; i < params->num_parms; ++i){
		if(params->parms[i].name != NULL && 0 == strcmp(params->parms[i].name, name)){
			return (int)i;
		}
	}
	return -1;
}

static void idas_set_char_option(IntegratorSystem *integ, const char *name, const char *value){
	slv_parameters_t params;
	int idx;

	CU_ASSERT_FATAL(0 == integrator_params_get(integ, &params));
	idx = idas_find_param(&params, name);
	CU_ASSERT_FATAL(idx >= 0);
	slv_set_char_parameter(&(SLV_PARAM_CHAR(&params, idx)), value);
	CU_ASSERT_FATAL(0 == integrator_params_set(integ, &params));
}

static void idas_set_real_option(IntegratorSystem *integ, const char *name, double value){
	slv_parameters_t params;
	int idx;

	CU_ASSERT_FATAL(0 == integrator_params_get(integ, &params));
	idx = idas_find_param(&params, name);
	CU_ASSERT_FATAL(idx >= 0);
	SLV_PARAM_REAL(&params, idx) = value;
	CU_ASSERT_FATAL(0 == integrator_params_set(integ, &params));
}

static void idas_cleanup(IdasTestSystem *testsys){
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

static struct Instance *idas_child(struct Instance *root, const char *name){
	struct Instance *child = ChildByChar(root, AddSymbol(name));
	CU_ASSERT_FATAL(child != NULL);
	return child;
}

static int idas_load_twinslabs(IdasTestSystem *testsys){
	int status;
	struct Instance *root;
	IdasMethodHooks hookstate;

	memset(testsys, 0, sizeof(*testsys));
	memset(&hookstate, 0, sizeof(hookstate));

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/ida" OSPATH_DIV "solvers/qrslv");

	Asc_OpenModule("twinslabs_der.a4c", &status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("twinslabs_der")) != NULL);

	testsys->siminst = SimsCreateInstance(AddSymbol("twinslabs_der"), AddSymbol("sim_idas"), e_normal, NULL);
	CU_ASSERT_FATAL(testsys->siminst != NULL);
	idas_assign_method_hooks(testsys->siminst, &hookstate);
	root = GetSimulationRoot(testsys->siminst);
	CU_ASSERT_FATAL(root != NULL);
	CU_ASSERT_FATAL(Proc_all_ok == idas_run_method(root, "on_load"));

	testsys->sys = system_build(root);
	CU_ASSERT_FATAL(testsys->sys != NULL);
	testsys->integ = integrator_new(testsys->sys, root);
	CU_ASSERT_FATAL(testsys->integ != NULL);
	testsys->sys = NULL;

	if(0 != integrator_set_engine(testsys->integ, "IDAS")){
		ASC_TEST_PARTIAL_SKIP("IDAS integrator unavailable");
		idas_cleanup(testsys);
		return 1;
	}

	idas_set_char_option(testsys->integ, "linsolver", "DENSE");
	idas_set_char_option(testsys->integ, "prec", "NONE");
	idas_set_real_option(testsys->integ, "rtol", 1e-8);
	idas_set_real_option(testsys->integ, "atol", 1e-8);
	return 0;
}

static TwinslabsAnalytic twinslabs_analytic(double t, double hc){
	const double c_heat = 50000.0;
	const double hbar = 20.0;
	const double tinf = 313.0;
	const double x10 = 290.0 - 313.0;
	const double x20 = 285.0 - 313.0;
	const double alpha = hc / c_heat;
	const double beta = hbar / c_heat;
	const double disc = sqrt(4.0 * alpha * alpha + beta * beta);
	const double lambda1 = (-(2.0 * alpha + beta) + disc) / 2.0;
	const double lambda2 = (-(2.0 * alpha + beta) - disc) / 2.0;
	const double q1 = 1.0 + lambda1 / alpha;
	const double q2 = 1.0 + lambda2 / alpha;
	const double den = q1 - q2;
	const double acoef = (x20 - q2 * x10) / den;
	const double bcoef = x10 - acoef;
	const double e1 = exp(lambda1 * t);
	const double e2 = exp(lambda2 * t);
	const double disc_a = 4.0 * alpha / disc;
	const double lambda1_a = -1.0 + 0.5 * disc_a;
	const double lambda2_a = -1.0 - 0.5 * disc_a;
	const double q1_a = lambda1_a / alpha - lambda1 / (alpha * alpha);
	const double q2_a = lambda2_a / alpha - lambda2 / (alpha * alpha);
	const double num = x20 - q2 * x10;
	const double num_a = -q2_a * x10;
	const double den_a = q1_a - q2_a;
	const double acoef_a = (num_a * den - num * den_a) / (den * den);
	const double bcoef_a = -acoef_a;
	TwinslabsAnalytic result;

	result.t1 = tinf + acoef * e1 + bcoef * e2;
	result.t2 = tinf + acoef * q1 * e1 + bcoef * q2 * e2;
	result.dt1_dhc = (
		acoef_a * e1 + acoef * e1 * lambda1_a * t
		+ bcoef_a * e2 + bcoef * e2 * lambda2_a * t
	) / c_heat;
	result.dt2_dhc = (
		(acoef_a * q1 + acoef * q1_a + acoef * q1 * lambda1_a * t) * e1
		+ (bcoef_a * q2 + bcoef * q2_a + bcoef * q2 * lambda2_a * t) * e2
	) / c_heat;
	return result;
}

static struct rel_relation *idas_find_solver_relation(slv_system_t sys,
		struct Instance *relinst){
	struct rel_relation **rels;
	int32 nrels, i;

	rels = slv_get_solvers_rel_list(sys);
	nrels = slv_get_num_solvers_rels(sys);
	CU_ASSERT_FATAL(rels != NULL);
	for(i = 0; i < nrels; ++i){
		if((struct Instance *)rel_instance(rels[i]) == relinst){
			return rels[i];
		}
	}
	CU_ASSERT_FATAL(0 && "solver relation not found");
	return NULL;
}

static double idas_relation_implicit_derivative(slv_system_t sys,
		struct Instance *root, const char *relname, struct Instance *dependent,
		struct Instance *parameter){
	struct Instance *relinst;
	struct rel_relation *rel;
	struct var_variable **variables;
	double *derivatives;
	var_filter_t filter;
	double grad_dep = NAN, grad_param = NAN;
	int32 i, nvar, count = 0, status;

	relinst = idas_child(root, relname);
	rel = idas_find_solver_relation(sys, relinst);
	CU_ASSERT_FATAL(rel != NULL);
	nvar = slv_get_num_solvers_vars(sys);
	CU_ASSERT_FATAL(nvar > 0);
	derivatives = ASC_NEW_ARRAY(double, nvar);
	variables = ASC_NEW_ARRAY(struct var_variable *, nvar);
	CU_ASSERT_FATAL(derivatives != NULL);
	CU_ASSERT_FATAL(variables != NULL);

	filter.matchbits = VAR_SVAR | VAR_INCIDENT;
	filter.matchvalue = VAR_SVAR | VAR_INCIDENT;
	status = relman_diff3(rel, &filter, derivatives, variables, &count, 0);
	CU_ASSERT_FATAL(status == 0);

	for(i = 0; i < count; ++i){
		struct Instance *inst = (struct Instance *)var_instance(variables[i]);
		if(inst == dependent){
			grad_dep = derivatives[i];
		}
		if(inst == parameter){
			grad_param = derivatives[i];
		}
	}
	ASC_FREE(derivatives);
	ASC_FREE(variables);
	CU_ASSERT_FATAL(!isnan(grad_dep));
	CU_ASSERT_FATAL(!isnan(grad_param));
	CU_ASSERT_FATAL(fabs(grad_dep) > 1e-12);
	return -grad_param / grad_dep;
}

static void idas_twinslabs_exact_sensitivities(double *dt1_dhc, double *dt2_dhc){
	struct Instance *siminst, *root, *it1, *it2, *ihc;
	slv_system_t sys;

	siminst = SimsCreateInstance(AddSymbol("twinslabs_der_exact"),
			AddSymbol("sim_idas_exact"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	CU_ASSERT_FATAL(Proc_all_ok == idas_run_method(root, "on_load"));
	it1 = idas_child(root, "T_1");
	it2 = idas_child(root, "T_2");
	ihc = idas_child(root, "h_c");
	sys = system_build(root);
	CU_ASSERT_FATAL(sys != NULL);

	*dt1_dhc = idas_relation_implicit_derivative(sys, root, "t1_exact", it1, ihc);
	*dt2_dhc = idas_relation_implicit_derivative(sys, root, "t2_exact", it2, ihc);
	system_destroy(sys);
	sim_destroy(siminst);
}

static void test_twinslabs_hc_sensitivity(void){
	IdasTestSystem testsys;
	struct Instance *root, *it1, *it2, *ihc;
	struct Instance *params[1];
	SampleList *samplelist;
	TwinslabsAnalytic exact;
	double s_t1_hc = 0.0, s_t2_hc = 0.0;
	double ascend_dt1_dhc = 0.0, ascend_dt2_dhc = 0.0;

	if(idas_load_twinslabs(&testsys)){
		return;
	}

	root = GetSimulationRoot(testsys.siminst);
	it1 = idas_child(root, "T_1");
	it2 = idas_child(root, "T_2");
	ihc = idas_child(root, "h_c");
	params[0] = ihc;
	CU_ASSERT_FATAL(0 == integrator_set_sensitivity_parameters(testsys.integ, params, 1));

	CU_ASSERT_FATAL(0 == integrator_analyse(testsys.integ));
	integrator_set_reporter(testsys.integ, &idas_reporter);
	samplelist = idas_create_samplelist(0.0, 36000.0, 100);
	integrator_set_samples(testsys.integ, samplelist);

	CU_ASSERT_FATAL(0 == integrator_solve(testsys.integ, 0, samplelist_length(samplelist) - 1));

	exact = twinslabs_analytic(36000.0, 5.0);
	idas_twinslabs_exact_sensitivities(&ascend_dt1_dhc, &ascend_dt2_dhc);
	CU_TEST(fabs(RealAtomValue(it1) - exact.t1) < 2e-4);
	CU_TEST(fabs(RealAtomValue(it2) - exact.t2) < 2e-4);
	CU_TEST(fabs(ascend_dt1_dhc - exact.dt1_dhc) < 1e-9);
	CU_TEST(fabs(ascend_dt2_dhc - exact.dt2_dhc) < 1e-9);
	CU_TEST(integrator_get_num_sensitivity_parameters(testsys.integ) == 1);
	CU_TEST(integrator_get_num_sensitivity_observations(testsys.integ) == 2);
	CU_ASSERT_FATAL(0 == integrator_get_observation_sensitivity(testsys.integ, 0, 0, &s_t1_hc));
	CU_ASSERT_FATAL(0 == integrator_get_observation_sensitivity(testsys.integ, 1, 0, &s_t2_hc));
	CU_TEST(fabs(s_t1_hc - ascend_dt1_dhc) < 2e-4);
	CU_TEST(fabs(s_t2_hc - ascend_dt2_dhc) < 2e-4);

	samplelist_free(samplelist);
	integrator_set_samples(testsys.integ, NULL);
	idas_cleanup(&testsys);
}

#define TESTS(T) \
	T(twinslabs_hc_sensitivity)

REGISTER_TESTS_SIMPLE(integrator_idas, TESTS)
