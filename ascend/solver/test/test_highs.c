#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/ascEnvVar.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/packages.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/var.h>
#include <ascend/solver/solver.h>

#include <test/common.h>

struct var_expect{
	const char *name_substr;
	double expected;
	double tol;
};

struct highs_run_options{
	int relaxed;
	int use_iterate;
	int use_resolve;
	int expect_converged;
	double objective_tol;
};

static int find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i = 0; i < pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && 0 == strcmp(pp->parms[i].name,name)){
			return i;
		}
	}
	return -1;
}

static int find_solver_var_value(slv_system_t sys, const char *name_substr, double *value){
	struct var_variable **vars = slv_get_solvers_var_list(sys);
	if(vars == NULL)return 0;
	for(; *vars != NULL; ++vars){
		char *name = var_make_name(sys,*vars);
		int match = (name != NULL && strstr(name,name_substr) != NULL);
		if(match){
			*value = var_value(*vars);
			ascfree(name);
			return 1;
		}
		if(name)ascfree(name);
	}
	return 0;
}

static void check_lp1_instance_tree(struct Instance *root, double expected_objective){
	struct Instance *x;
	struct Instance *y;
	struct Instance *s1;
	struct Instance *s2;
	struct Instance *obj;
	const struct relation *objrel;

	CU_ASSERT_FATAL(root != NULL);
	x = ChildByChar(root,AddSymbol("x"));
	y = ChildByChar(root,AddSymbol("y"));
	s1 = ChildByChar(root,AddSymbol("s1"));
	s2 = ChildByChar(root,AddSymbol("s2"));
	obj = ChildByChar(root,AddSymbol("obj"));
	CU_ASSERT_FATAL(x != NULL);
	CU_ASSERT_FATAL(y != NULL);
	CU_ASSERT_FATAL(s1 != NULL);
	CU_ASSERT_FATAL(s2 != NULL);
	CU_ASSERT_FATAL(obj != NULL);

	CU_ASSERT_DOUBLE_EQUAL(2.0,RealAtomValue(x),1e-7);
	CU_ASSERT_DOUBLE_EQUAL(2.0,RealAtomValue(y),1e-7);
	CU_ASSERT_DOUBLE_EQUAL(0.0,RealAtomValue(s1),1e-7);
	CU_ASSERT_DOUBLE_EQUAL(0.0,RealAtomValue(s2),1e-7);

	objrel = GetInstanceRelationOnly(obj);
	CU_ASSERT_FATAL(objrel != NULL);
	CU_ASSERT_DOUBLE_EQUAL(expected_objective,RelationResidual(objrel),1e-7);
}

static void run_highs_model(
	const char *module_path,
	const char *model_name,
	double expected_objective,
	int check_scalars_in_tree,
	const struct var_expect *vars,
	int nvars,
	const struct highs_run_options *opts
){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	int i;
	int expect_converged = 1;
	double objective_tol = 1e-7;

	if(opts != NULL){
		expect_converged = opts->expect_converged;
		if(opts->objective_tol > 0){
			objective_tol = opts->objective_tol;
		}
	}

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS test: solver package not available");
		goto cleanup;
	}

	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule(module_path,&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(model_name)) != NULL);

	siminst = SimsCreateInstance(AddSymbol(model_name), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
	}

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);
	CU_ASSERT_TRUE(slv_eligible_solver(sys));

	{
		slv_parameters_t pp;
		int nonlin_idx;
		int relaxed_idx;
		slv_get_parameters(sys,&pp);
		nonlin_idx = find_param_index(&pp,"nonlin");
		relaxed_idx = find_param_index(&pp,"relaxed");
		CU_ASSERT_FATAL(nonlin_idx != -1);
		CU_ASSERT_FATAL(relaxed_idx != -1);
		SLV_PARAM_BOOL(&pp,nonlin_idx) = FALSE;
		SLV_PARAM_BOOL(&pp,relaxed_idx) = (opts != NULL && opts->relaxed) ? TRUE : FALSE;
		slv_set_parameters(sys,&pp);
	}

	(void)slv_presolve(sys);
	if(opts != NULL && opts->use_iterate){
		(void)slv_iterate(sys);
	}else if(opts != NULL && opts->use_resolve){
		struct var_variable **vlist = NULL;
		(void)slv_solve(sys);
		vlist = slv_get_solvers_var_list(sys);
		if(vlist != NULL && *vlist != NULL){
			var_set_value(*vlist,var_value(*vlist) + 0.1);
		}
		(void)slv_resolve(sys);
	}else{
		(void)slv_solve(sys);
	}
	slv_get_status(sys,&status);
	if(expect_converged){
		CU_ASSERT_TRUE(status.converged);
		CU_ASSERT_FALSE(status.diverged);
	}else{
		CU_ASSERT_FALSE(status.converged);
		CU_ASSERT_TRUE(status.diverged);
		goto cleanup;
	}

	for(i = 0; i < nvars; ++i){
		double value = 0.0;
		if(!find_solver_var_value(sys,vars[i].name_substr,&value)){
			CU_FAIL("Expected variable not found in solver var list");
			break;
		}
		CU_ASSERT_DOUBLE_EQUAL(vars[i].expected,value,vars[i].tol);
	}
	{
		struct Instance *root = GetSimulationRoot(siminst);
		struct Instance *objinst;
		struct rel_relation *objrel = slv_get_obj_relation(sys);
		CU_ASSERT_FATAL(root != NULL);
		CU_ASSERT_FATAL(objrel != NULL);
		objinst = ChildByChar(root,AddSymbol("obj"));
		CU_ASSERT_FATAL(objinst != NULL);
		CU_ASSERT_DOUBLE_EQUAL(expected_objective,rel_residual(objrel),objective_tol);
		CU_ASSERT_DOUBLE_EQUAL(expected_objective,RelationResidual(GetInstanceRelationOnly(objinst)),objective_tol);
		if(check_scalars_in_tree){
			check_lp1_instance_tree(root,expected_objective);
		}
	}

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_highs_lp1(void){
	/* Baseline LP: verifies parse/compile/presolve/solve and instance-tree writeback. */
	static const struct var_expect expected[] = {
		{"x", 2.0, 1e-7},
		{"y", 2.0, 1e-7},
		{"s1", 0.0, 1e-7},
		{"s2", 0.0, 1e-7}
	};
	run_highs_model("models/test/ipopt/lp1.a4c","lp1",-10.0,1,expected,4,NULL);
}

static void test_highs_lp_structured(void){
	/* Structured LP: exercises array/object path naming for solver variables. */
	static const struct var_expect expected[] = {
		{"x[1]", 2.0, 1e-7},
		{"x[2]", 2.0, 1e-7},
		{"row[1].s", 0.0, 1e-7},
		{"row[2].s", 0.0, 1e-7}
	};
	run_highs_model("models/test/ipopt/lp_structured.a4c","lp_structured",-10.0,0,expected,4,NULL);
}

static void test_highs_mip_mixed(void){
	/* Mixed-integer model: covers HiGHS MIP load path and mixed row operators. */
	static const struct highs_run_options opts = {0,0,0,0,1e-7};
	run_highs_model("models/test/mip/mip_mixed.a4c","mip_mixed",0.0,0,NULL,0,&opts);
}

static void test_highs_mip_mixed_iterate(void){
	/* Same MIP model via the iterate entrypoint (current implementation delegates to solve). */
	static const struct highs_run_options opts = {0,1,0,0,1e-7};
	run_highs_model("models/test/mip/mip_mixed.a4c","mip_mixed",0.0,0,NULL,0,&opts);
}

static void test_highs_mip_mixed_resolve(void){
	/* Same MIP model via resolve, after perturbing variable values. */
	static const struct highs_run_options opts = {1,0,1,0,1e-7};
	run_highs_model("models/test/mip/mip_mixed.a4c","mip_mixed",0.0,0,NULL,0,&opts);
}

static void test_highs_mip_facility_location(void){
	/* Non-trivial MIP: capacitated facility-location with fixed + assignment costs. */
	static const struct var_expect expected[] = {
		{"open[1]", 1.0, 1e-7},
		{"open[2]", 0.0, 1e-7},
		{"open[3]", 1.0, 1e-7},
		{"assign[1][1]", 1.0, 1e-7},
		{"assign[2][3]", 1.0, 1e-7},
		{"assign[3][3]", 1.0, 1e-7},
		{"assign[4][3]", 1.0, 1e-7}
	};
	run_highs_model(
		"models/test/mip/facility_location.a4c",
		"mip_facility_location",
		470.0,
		0,
		expected,
		7,
		NULL
	);
}

static void test_highs_trnsport(void){
	/* GAMS transportation LP benchmark: objective value regression against published optimum. */
	run_highs_model("models/test/highs/trnsport.a4c","trnsport",153.675,0,NULL,0,NULL);
}

static void test_highs_blend_whiskas2(void){
	/* PuLP blending LP benchmark: checks known unique optimum and primal variable values. */
	static const struct var_expect expected[] = {
		{"chicken", 100.0 / 3.0, 1e-7},
		{"beef", 200.0 / 3.0, 1e-7}
	};
	run_highs_model(
		"models/test/highs/blend_whiskas2.a4c",
		"blend_whiskas2",
		0.9666666666666667,
		0,
		expected,
		2,
		NULL
	);
}

static void test_highs_afiro(void){
	/* Netlib AFIRO LP benchmark: verifies solve path on denser benchmark-style LP data. */
	static const struct highs_run_options opts = {0,0,0,1,1e-5};
	run_highs_model("models/test/highs/afiro.a4c","afiro",-464.7531428571429,0,NULL,0,&opts);
}

static void test_highs_ineligible_without_objective(void){
	/* No-objective model: exercises ineligible-solver and presolve rejection path. */
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS no-objective test: solver package not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS no-objective test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/ipopt/lp_noobj.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("lp_noobj")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("lp_noobj"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
	}

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);
	CU_ASSERT_FALSE(slv_eligible_solver(sys));

	(void)slv_presolve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_FALSE(status.ready_to_solve);
	CU_ASSERT_FALSE(status.converged);

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(highs_lp1) \
	T(highs_lp_structured) \
	T(highs_mip_mixed) \
	T(highs_mip_mixed_iterate) \
	T(highs_mip_mixed_resolve) \
	T(highs_mip_facility_location) \
	T(highs_trnsport) \
	T(highs_blend_whiskas2) \
	T(highs_afiro) \
	T(highs_ineligible_without_objective)

REGISTER_TESTS_SIMPLE(solver_highs, TESTS)
