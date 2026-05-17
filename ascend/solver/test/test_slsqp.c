#include <stdio.h>
#include <string.h>

#include <ascend/general/env.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/slvreq.h>
#include <ascend/compiler/symtab.h>
#include <ascend/solver/solver.h>
#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/system/slv_param.h>

#include <test/common.h>

static int slsqp_find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i = 0; i < pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && strcmp(pp->parms[i].name,name) == 0){
			return i;
		}
	}
	return -1;
}

struct SlsqpModelRunner {
	struct Instance *siminst;
	struct Instance *buildroot;
	slv_system_t sys;
};

static int slsqp_model_runner_select(struct SlsqpModelRunner *runner){
	int solver_index;
	if(runner == NULL || runner->siminst == NULL){
		return SLVREQ_UNKNOWN_SOLVER;
	}
	solver_index = slv_lookup_client("SLSQP");
	if(solver_index == -1){
		return SLVREQ_UNKNOWN_SOLVER;
	}
	if(runner->sys == NULL){
		runner->sys = system_build(GetSimulationRoot(runner->siminst));
		runner->buildroot = GetSimulationRoot(runner->siminst);
		if(runner->sys == NULL){
			return SLVREQ_UNKNOWN_SOLVER;
		}
	}
	if(slv_select_solver(runner->sys,solver_index) == -1){
		return SLVREQ_UNKNOWN_SOLVER;
	}
	return 0;
}

static int slsqp_model_runner_set_solver(const char *solvername, void *user_data){
	(void)solvername;
	return slsqp_model_runner_select((struct SlsqpModelRunner *)user_data);
}

static int slsqp_model_runner_set_option(const char *optionname, struct value_t *val, void *user_data){
	struct SlsqpModelRunner *runner = (struct SlsqpModelRunner *)user_data;
	slv_parameters_t params;
	int idx;
	if(runner == NULL || runner->sys == NULL){
		return SLVREQ_OPTIONS_UNAVAILABLE;
	}
	if(optionname == NULL || val == NULL){
		return SLVREQ_INVALID_OPTION_NAME;
	}
	slv_get_parameters(runner->sys,&params);
	idx = slsqp_find_param_index(&params,optionname);
	if(idx < 0){
		/* The reused A4SQP model corpus contains A4SQP-specific options. */
		return 0;
	}
	switch(SLV_PARAM_TYPE(&params,idx)){
	case int_parm:
		if(ValueKind(*val) == integer_value){
			SLV_PARAM_INT(&params,idx) = IntegerValue(*val);
		}
		break;
	case bool_parm:
		if(ValueKind(*val) == boolean_value){
			SLV_PARAM_BOOL(&params,idx) = BooleanValue(*val);
		}
		break;
	case real_parm:
		if(ValueKind(*val) == real_value){
			SLV_PARAM_REAL(&params,idx) = RealValue(*val);
		}
		break;
	case char_parm:
		if(ValueKind(*val) == symbol_value){
			slv_set_char_parameter(&(SLV_PARAM_CHAR(&params,idx)),SCP(SymbolValue(*val)));
		}
		break;
	}
	slv_set_parameters(runner->sys,&params);
	return 0;
}

static int slsqp_model_runner_do_solve(struct Instance *instance, void *user_data){
	struct SlsqpModelRunner *runner = (struct SlsqpModelRunner *)user_data;
	int res;
	if(runner == NULL || runner->siminst == NULL){
		return SLVREQ_NO_SOLVER_SELECTED;
	}
	if(instance == NULL){
		instance = GetSimulationRoot(runner->siminst);
	}
	if(runner->sys != NULL && runner->buildroot != instance){
		system_destroy(runner->sys);
		runner->sys = NULL;
		runner->buildroot = NULL;
	}
	if(runner->sys == NULL){
		if(slsqp_model_runner_select(runner) != 0){
			return SLVREQ_NO_SOLVER_SELECTED;
		}
	}
	res = slv_solve(runner->sys);
	return res == 0 ? 0 : SLVREQ_SOLVE_FAIL;
}

static int slsqp_model_runner_delete_system(void *user_data){
	struct SlsqpModelRunner *runner = (struct SlsqpModelRunner *)user_data;
	if(runner != NULL && runner->sys != NULL){
		system_destroy(runner->sys);
		runner->sys = NULL;
		runner->buildroot = NULL;
	}
	return 0;
}

static int slsqp_run_model_self_test(const char *module, const char *model, const char *sim){
	int status;
	int result = 1;
	struct Instance *siminst = NULL;
	struct SlsqpModelRunner runner = {0};
	SlvReqHooks hooks = SLVREQ_HOOKS_EMPTY;
	slv_status_t slvstatus;
	struct Name *name = NULL;
	enum Proc_enum pe;

	Asc_CompilerInit(1);
	if(0 != Asc_PutEnv(ASC_ENV_LIBRARY "=models")){
		goto cleanup;
	}
	if(0 != Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/slsqp")){
		goto cleanup;
	}
	solver_destroy_engines();
	if(0 != package_load("slsqp",NULL)){
		CONSOLE_DEBUG("Skipping SLSQP test: solver package not available");
		result = 0;
		goto cleanup;
	}

	Asc_OpenModule(module,&status);
	if(status != 0 || 0 != zz_parse() || FindType(AddSymbol(model)) == NULL){
		goto cleanup;
	}
	siminst = SimsCreateInstance(AddSymbol(model), AddSymbol(sim), e_normal, NULL);
	if(siminst == NULL){
		goto cleanup;
	}
	runner.siminst = siminst;
	hooks.set_solver_fn = &slsqp_model_runner_set_solver;
	hooks.set_option_fn = &slsqp_model_runner_set_option;
	hooks.do_solve_fn = &slsqp_model_runner_do_solve;
	hooks.delete_system_fn = &slsqp_model_runner_delete_system;
	hooks.user_data = &runner;
	if(slvreq_assign_hooks(siminst,&hooks) != 0){
		goto cleanup;
	}

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(GetSimulationRoot(siminst),name,sim,ASCERR,WP_STOPONERR,NULL,NULL);
	if(pe != Proc_all_ok){
		goto cleanup;
	}
	if(runner.sys == NULL && slsqp_model_runner_select(&runner) != 0){
		goto cleanup;
	}
	slv_get_status(runner.sys,&slvstatus);
	if(!slvstatus.converged && 0 != slv_solve(runner.sys)){
		goto cleanup;
	}
	slv_get_status(runner.sys,&slvstatus);
	if(!slvstatus.converged || slvstatus.diverged || slvstatus.iteration_limit_exceeded){
		goto cleanup;
	}

	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(siminst),name,sim,ASCERR,WP_STOPONERR,NULL,NULL);
	if(pe != Proc_all_ok){
		goto cleanup;
	}
	result = 0;

cleanup:
	if(runner.sys != NULL){
		system_destroy(runner.sys);
		system_free_reused_mem();
	}
	if(siminst != NULL){
		sim_destroy(siminst);
	}
	solver_destroy_engines();
	Asc_CompilerDestroy();
	return result;
}

static void test_slsqp_register(void){
	Asc_CompilerInit(1);
	CU_ASSERT(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/slsqp"));
	solver_destroy_engines();
	CU_ASSERT(0 == package_load("slsqp",NULL));
	CU_ASSERT(-1 != slv_lookup_client("SLSQP"));
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slsqp_bt10_solve(void){
	CU_ASSERT_EQUAL(slsqp_run_model_self_test("test/a4sqp/bt10.a4c","bt10","sim_bt10"),0);
}

static void test_slsqp_bt2_solve(void){
	CU_ASSERT_EQUAL(slsqp_run_model_self_test("test/a4sqp/bt2.a4c","bt2","sim_bt2"),0);
}

static void test_slsqp_cb3_solve(void){
	CU_ASSERT_EQUAL(slsqp_run_model_self_test("test/a4sqp/cb3.a4c","cb3","sim_cb3"),0);
}

static void test_slsqp_hs21_solve(void){
	CU_ASSERT_EQUAL(slsqp_run_model_self_test("test/a4sqp/hs21.a4c","hs21","sim_hs21"),0);
}

static void test_slsqp_alsotame_solve(void){
	CU_ASSERT_EQUAL(slsqp_run_model_self_test("test/a4sqp/alsotame.a4c","alsotame","sim_alsotame"),0);
}

static void test_slsqp_avgasa_solve(void){
	CU_ASSERT_EQUAL(slsqp_run_model_self_test("test/a4sqp/avgasa.a4c","avgasa","sim_avgasa"),0);
}

static void test_slsqp_avgasb_solve(void){
	CU_ASSERT_EQUAL(slsqp_run_model_self_test("test/a4sqp/avgasb.a4c","avgasb","sim_avgasb"),0);
}

#define TESTS(T) \
	T(slsqp_register) \
	T(slsqp_bt10_solve) \
	T(slsqp_bt2_solve) \
	T(slsqp_cb3_solve) \
	T(slsqp_hs21_solve) \
	T(slsqp_alsotame_solve) \
	T(slsqp_avgasa_solve) \
	T(slsqp_avgasb_solve)

REGISTER_TESTS_SIMPLE(solver_slsqp, TESTS)
