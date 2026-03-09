#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>

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
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>

typedef struct SlvReqC_struct{
	struct Instance *siminst;
	slv_system_t sys;
	struct Instance *buildroot;
	char solvername[64];
	int delete_count;
} SlvReqC;

static SlvReqSetSolverFn slvreq_c_set_solver;
static SlvReqDoSolveFn slvreq_c_do_solve;
static SlvReqDeleteSystemFn slvreq_c_delete_system;

int slvreq_c_set_solver(const char *solvername, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	int index = slv_lookup_client(solvername);
	if(index == -1){
		CONSOLE_DEBUG("Failed to locate solver '%s'",solvername);
		return 1;
	}

	if(S->sys == NULL){
		S->sys = system_build(GetSimulationRoot(S->siminst));
		S->buildroot = GetSimulationRoot(S->siminst);
	}
	snprintf(S->solvername,sizeof(S->solvername),"%s",solvername);

	if(slv_select_solver(S->sys,index) == -1){
		CONSOLE_DEBUG("Failed to select solver '%s'",solvername);
		return 2;
	}
	return 0;
}

int slvreq_c_do_solve(struct Instance *instance, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	int res;

	if(instance == NULL){
		instance = GetSimulationRoot(S->siminst);
	}
	if(S->sys != NULL && S->buildroot != instance){
		system_destroy(S->sys);
		S->sys = NULL;
		S->buildroot = NULL;
		++S->delete_count;
	}
	if(S->sys == NULL){
		int index;
		if(S->solvername[0] == '\0'){
			return SLVREQ_NO_SOLVER_SELECTED;
		}
		S->sys = system_build(instance);
		S->buildroot = instance;
		if(S->sys == NULL){
			return SLVREQ_PRESOLVE_FAIL;
		}
		index = slv_lookup_client(S->solvername);
		if(index == -1 || slv_select_solver(S->sys,index) == -1){
			return SLVREQ_NO_SOLVER_SELECTED;
		}
	}

	res = slv_presolve(S->sys);
	if(res){
		return SLVREQ_PRESOLVE_FAIL;
	}

	{
		slv_status_t status;
		slv_get_status(S->sys, &status);
		if(status.ready_to_solve){
			res = slv_solve(S->sys);
			if(res){
				CONSOLE_DEBUG("slv_solve returned %d",res);
			}
		}
		slv_get_status(S->sys, &status);
		if(status.ok){
			return 0;
		}
	}

	return SLVREQ_SOLVE_FAIL;
}

int slvreq_c_delete_system(void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	if(S->sys != NULL){
		system_destroy(S->sys);
		S->sys = NULL;
		S->buildroot = NULL;
		++S->delete_count;
	}
	return 0;
}

static void test_column_model(const char *modelname){
	int status;
	SlvReqC S;
	struct Name *name;
	enum Proc_enum pe;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv"));
	CU_ASSERT_FATAL(0 == package_load("qrslv",NULL));

	Asc_OpenModule("column.a4l",&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(modelname)) != NULL);

	S.siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst != NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_EQUAL(pe, Proc_all_ok);

	name = CreateIdName(AddSymbol("self_test"));
	pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT_EQUAL(pe, Proc_all_ok);

	CU_ASSERT_EQUAL(S.delete_count, 3);

	if(S.sys){
		system_destroy(S.sys);
	}
	system_free_reused_mem();
	solver_destroy_engines();
	sim_destroy(S.siminst);
	Asc_CompilerDestroy();
}

static void test_column_simple(void){
	test_column_model("test_simple_column");
}

static void test_column_mw_demo(void){
	test_column_model("mw_demo_column");
}

static void test_column_abc_demo(void){
	test_column_model("abc_demo_column");
}

static void test_column_c567_demo(void){
	test_column_model("c567_demo_column");
}

#define TESTS(T) \
	T(column_simple) \
	T(column_mw_demo) \
	T(column_abc_demo) \
	T(column_c567_demo)

REGISTER_TESTS_SIMPLE(solver_column, TESTS)
