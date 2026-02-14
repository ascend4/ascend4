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

static void run_highs_model(
	const char *module_path,
	const char *model_name,
	const struct var_expect *vars,
	int nvars
){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	int i;

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

	{
		slv_parameters_t pp;
		int nonlin_idx;
		slv_get_parameters(sys,&pp);
		nonlin_idx = find_param_index(&pp,"nonlin");
		CU_ASSERT_FATAL(nonlin_idx != -1);
		SLV_PARAM_BOOL(&pp,nonlin_idx) = FALSE;
		slv_set_parameters(sys,&pp);
	}

	(void)slv_presolve(sys);
	(void)slv_solve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.converged);
	CU_ASSERT_FALSE(status.diverged);

	for(i = 0; i < nvars; ++i){
		double value = 0.0;
		if(!find_solver_var_value(sys,vars[i].name_substr,&value)){
			CU_FAIL("Expected variable not found in solver var list");
			break;
		}
		CU_ASSERT_DOUBLE_EQUAL(vars[i].expected,value,vars[i].tol);
	}

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_highs_lp1(void){
	static const struct var_expect expected[] = {
		{"x", 2.0, 1e-7},
		{"y", 2.0, 1e-7},
		{"s1", 0.0, 1e-7},
		{"s2", 0.0, 1e-7}
	};
	run_highs_model("models/test/ipopt/lp1.a4c","lp1",expected,4);
}

static void test_highs_lp_structured(void){
	static const struct var_expect expected[] = {
		{"x[1]", 2.0, 1e-7},
		{"x[2]", 2.0, 1e-7},
		{"row[1].s", 0.0, 1e-7},
		{"row[2].s", 0.0, 1e-7}
	};
	run_highs_model("models/test/ipopt/lp_structured.a4c","lp_structured",expected,4);
}

#define TESTS(T) \
	T(highs_lp1) \
	T(highs_lp_structured)

REGISTER_TESTS_SIMPLE(solver_highs, TESTS)
