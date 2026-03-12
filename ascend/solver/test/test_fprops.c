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
#include <ascend/compiler/rel_blackbox.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/instance_io.h>

#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/system/slv_server.h>

#include <test/common.h>



typedef struct SlvReqC_struct{
	struct Instance *siminst;
	slv_system_t sys;
	slv_status_t last_status;
	int last_solve_result;
	int last_slv_solve_return;
	int solve_calls;
} SlvReqC;

static SlvReqSetSolverFn slvreq_c_set_solver;
static SlvReqSetOptionFn slvreq_c_set_option;
static SlvReqDoSolveFn slvreq_c_do_solve;

/*
	This function actually does the job of setting the solver in our little
	'test' simulation environment.
*/
int slvreq_c_set_solver(const char *solvername, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	int index = slv_lookup_client(solvername);
	if(index == -1){
		CONSOLE_DEBUG("Failed to locate solver '%s'",solvername);
		return 1;
	}

	if(S->sys == NULL){
		CONSOLE_DEBUG("Building system...");
		S->sys = system_build(GetSimulationRoot(S->siminst));
	}

	if(slv_select_solver(S->sys,index) == -1){
		CONSOLE_DEBUG("Failed to select solver '%s' (solver was found, though)",solvername);
		return 2;
	}
	CONSOLE_DEBUG("Assigned solver '%s'...",solvername);
	return 0;
}

/*
	This function actually does the job of setting solver options in our little
	'test' simulation environment.
*/
int slvreq_c_set_option(const char *optionname, struct value_t *val, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	slv_parameters_t pp;
	if(S->sys == NULL){
		/* no solver has been assigned so the system isn't built*/
		return SLVREQ_OPTIONS_UNAVAILABLE;
	}
	slv_get_parameters(S->sys, &pp);
	/* look for the parameter by name */
	int i, index = -1;
	for(i=0;i<pp.num_parms;++i){
		if(strcmp(pp.parms[i].name,optionname)==0){
			index = i;
		}
	}
	if(index == -1){
		/* not found */
		return SLVREQ_INVALID_OPTION_NAME;
	}
	/* parameter has been found */
	switch(SLV_PARAM_TYPE(&pp,index)){
	case int_parm:
		if(ValueKind(*val)!=integer_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting integer type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = %ld...",optionname, IntegerValue(*val));
		SLV_PARAM_INT(&pp,index) = IntegerValue(*val);
		break;
	case bool_parm:
		if(ValueKind(*val)!=boolean_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting boolean type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = %s...",optionname, BooleanValue(*val)?"TRUE":"FALSE");
		SLV_PARAM_BOOL(&pp,index) = BooleanValue(*val);
		break;
	case real_parm:
		if(ValueKind(*val)!=real_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting real type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = %f...",optionname, RealValue(*val));
		SLV_PARAM_REAL(&pp,index) = RealValue(*val);
		break;
	case char_parm:
		if(ValueKind(*val)!=symbol_value){
			CONSOLE_DEBUG("Wrong parameter type: expecting string (i.e. symbol) type");
			return SLVREQ_WRONG_OPTION_VALUE_TYPE;
		}
		CONSOLE_DEBUG("Setting %s = '%s'...",optionname, SCP(SymbolValue(*val)));
		slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,index)),SCP(SymbolValue(*val)));
		break;
	}

	slv_set_parameters(S->sys, &pp);

	return 0;
}

/*
	This function actually does the job of running solver in our little
	'test' simulation environment, and reporting the result.
*/
int slvreq_c_do_solve(struct Instance *instance, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	int res;
	++S->solve_calls;
	if(S->sys==NULL){
		S->last_solve_result = SLVREQ_NO_SOLVER_SELECTED;
		S->last_slv_solve_return = 0;
		memset(&S->last_status, 0, sizeof(S->last_status));
		return SLVREQ_NO_SOLVER_SELECTED;
	}

	res = slv_presolve(S->sys);
	if(res){
		S->last_solve_result = SLVREQ_PRESOLVE_FAIL;
		S->last_slv_solve_return = 0;
		slv_get_status(S->sys, &S->last_status);
		return SLVREQ_PRESOLVE_FAIL;
	}

	slv_status_t status;

	slv_get_status(S->sys, &status);
	if(status.ready_to_solve){
		res = slv_solve(S->sys);
		if(res){
			CONSOLE_DEBUG("slv_solve returned %d",res);
		}
	}
	S->last_slv_solve_return = res;

	slv_get_status(S->sys, &status);
	S->last_status = status;
	if(status.ok){
		CONSOLE_DEBUG("Solver completed OK");
		S->last_solve_result = 0;
		return 0;
	}

	if(status.diverged)CONSOLE_DEBUG("Solver diverged");
	if(status.inconsistent)CONSOLE_DEBUG("System is inconsistent");
	if(status.iteration_limit_exceeded)CONSOLE_DEBUG("Solver exceeded iteration limit");
	if(status.calc_ok == 0)CONSOLE_DEBUG("Solver had residual calculation errors");
	if(status.over_defined)CONSOLE_DEBUG("Solver system is over-defined");
	if(status.under_defined)CONSOLE_DEBUG("Solver system is under-defined");
	if(status.time_limit_exceeded)CONSOLE_DEBUG("Solver exceeded time limit");

	S->last_solve_result = SLVREQ_SOLVE_FAIL;
	return SLVREQ_SOLVE_FAIL;
}

static struct Instance *child_by_name(struct Instance *inst, const char *name){
	struct Instance *child = ChildByChar(inst, AddSymbol(name));
	CU_ASSERT_FATAL(child != NULL);
	return child;
}

static enum Proc_enum run_method(struct Instance *siminst, const char *method){
	struct Name *name = CreateIdName(AddSymbol(method));
	return Initialize(GetSimulationRoot(siminst), name, "sim1", ASCERR, WP_STOPONERR, NULL, NULL);
}

static void destroy_test_simulation(SlvReqC *S){
	if(!S){
		return;
	}
	CONSOLE_DEBUG("Destroying system...");
	if(S->sys){
		system_destroy(S->sys);
		S->sys = NULL;
	}
	system_free_reused_mem();
	CONSOLE_DEBUG("Destroy solver engines");
	solver_destroy_engines();
	CONSOLE_DEBUG("Destroying instance tree");
	if(S->siminst){
		sim_destroy(S->siminst);
		S->siminst = NULL;
	}
	Asc_CompilerDestroy();
}

static void load_fprops_model(const char *modelfile, const char *modelname, SlvReqC *S){
	int status;

	memset(S, 0, sizeof(*S));
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	Asc_OpenModule(modelfile,&status);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol(modelname))!=NULL);

	S->siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S->siminst!=NULL);

	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.user_data = S
		};
		slvreq_assign_hooks(S->siminst, &hooks);
	}
}

static void test_nox_air_debug_single_point(double T, int expect_success){
	static const char *modelfile = "johnpye/fprops/reactive_equil_nox_air_demo.a4c";
	static const char *modelname = "reactive_equil_nox_air_debug";
	SlvReqC S;
	struct Instance *root;
	enum Proc_enum pe;
	int bbox_before = BlackBoxCacheAlive();

	load_fprops_model(modelfile, modelname, &S);
	root = GetSimulationRoot(S.siminst);
	CU_ASSERT_FATAL(root != NULL);

	pe = run_method(S.siminst, "default_self");
	CU_ASSERT_FATAL(pe == Proc_all_ok);
	SetRealAtomValue(child_by_name(root, "T_reactor"), T, 0);
	pe = run_method(S.siminst, "solve_case");

	fprintf(stderr,
		"NOx ASCEND characterization: T=%.0f pe=%d solve_calls=%d ok=%u ready=%u calc_ok=%u div=%u iterlim=%u over=%u under=%u\n",
		T, (int)pe, S.solve_calls, S.last_status.ok, S.last_status.ready_to_solve,
		S.last_status.calc_ok, S.last_status.diverged, S.last_status.iteration_limit_exceeded,
		S.last_status.over_defined, S.last_status.under_defined);

	if(expect_success){
		CU_ASSERT(pe == Proc_all_ok);
		CU_ASSERT_EQUAL(S.last_solve_result, 0);
		CU_ASSERT_TRUE(S.last_status.ok);
		CU_ASSERT_TRUE(S.last_status.calc_ok);
	}else{
		CU_ASSERT_NOT_EQUAL(S.last_solve_result, 0);
		CU_ASSERT_FALSE(S.last_status.ok);
	}

	destroy_test_simulation(&S);
	CU_ASSERT_EQUAL(BlackBoxCacheAlive(), bbox_before);
}

static void test_nox_air_debug_1100K(void){
	test_nox_air_debug_single_point(1100.0, 1);
}

static void test_nox_air_debug_lowT_fresh_characterization(void){
	static const double temps[] = {700.0, 650.0, 300.0};
	static const int expect_success[] = {1, 0, 0};
	size_t i;
	for(i = 0; i < sizeof(temps) / sizeof(temps[0]); ++i){
		test_nox_air_debug_single_point(temps[i], expect_success[i]);
	}
}

static void test_nox_air_debug_reuse_descending_pathology(void){
	static const char *modelfile = "johnpye/fprops/reactive_equil_nox_air_demo.a4c";
	static const char *modelname = "reactive_equil_nox_air_debug";
	static const double temps[] = {1100.0, 1000.0, 900.0, 800.0, 700.0, 650.0};
	SlvReqC S;
	struct Instance *root;
	struct Instance *T_reactor;
	struct Instance *NO2_out;
	enum Proc_enum pe;
	double no2_1100 = NAN;
	double no2_650 = NAN;
	int solve_ret_650 = 0;
	size_t i;

	load_fprops_model(modelfile, modelname, &S);
	root = GetSimulationRoot(S.siminst);
	CU_ASSERT_FATAL(root != NULL);
	T_reactor = child_by_name(root, "T_reactor");
	NO2_out = child_by_name(root, "NO2_out");

	pe = run_method(S.siminst, "default_self");
	CU_ASSERT_FATAL(pe == Proc_all_ok);

	for(i = 0; i < sizeof(temps) / sizeof(temps[0]); ++i){
		SetRealAtomValue(T_reactor, temps[i], 0);
		pe = run_method(S.siminst, "solve_case");
		fprintf(stderr,
			"NOx ASCEND reuse characterization: T=%.0f pe=%d solve_calls=%d ok=%u calc_ok=%u div=%u no2=%.17g\n",
			temps[i], (int)pe, S.solve_calls, S.last_status.ok, S.last_status.calc_ok,
			S.last_status.diverged, RealAtomValue(NO2_out));
		if(temps[i] == 1100.0){
			no2_1100 = RealAtomValue(NO2_out);
		}
		if(temps[i] == 650.0){
			no2_650 = RealAtomValue(NO2_out);
			solve_ret_650 = S.last_slv_solve_return;
		}
	}

	CU_ASSERT_TRUE(isfinite(no2_1100));
	CU_ASSERT_TRUE(isfinite(no2_650));
	CU_ASSERT_TRUE(no2_1100 < 1e-3);
	CU_ASSERT_TRUE(no2_650 > 1e4 * no2_1100);
	CU_ASSERT_NOT_EQUAL(solve_ret_650, 0);

	destroy_test_simulation(&S);
}



/*
	Test that the slvreq mechanism works within the C layer
*/
static void test_fprops_model(const char *modelname){
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");
	char *lib = Asc_GetEnv(ASC_ENV_SOLVERS);
	CONSOLE_DEBUG("%s = %s\n",ASC_ENV_SOLVERS,lib);
	ASC_FREE(lib);

	/* load the file */
	Asc_OpenModule("test/fprops/fprops_test_base.a4c",&status);
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(modelname))!=NULL);

	/* instantiate it */
	SlvReqC S;
	S.siminst = SimsCreateInstance(AddSymbol(modelname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
#undef TESTFILE

	/* do the solver hooks */
	S.sys = NULL;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

    CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* assume that this clever model will have already solved and self-tested itself */

	CONSOLE_DEBUG("Destroying system...");
	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();

	CONSOLE_DEBUG("Destroy solver engines");
	solver_destroy_engines();

	/* destroy all that stuff */
	CONSOLE_DEBUG("Destroying instance tree");
	CU_ASSERT(S.siminst != NULL);
	sim_destroy(S.siminst);

	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */


#define TESTS1(T,X) \
	T(stream_state_test) \
	X T(stream_state_test_co2) \
	X T(stream_state_test_toluene) \
	X T(stream_state_test_sodium_h) \
	X T(stream_state_test_sodium_pT) \
	X T(pump_simple_test) \
	X T(sodium_test)

/* need a little function for each model we're testing... */
#define T(N) static void test_##N(void){\
		test_fprops_model(#N);\
	}
#define X
TESTS1(T,X)
#undef T
#undef X

#define X
#define TESTS(T) \
	TESTS1(T,X) \
	X T(nox_air_debug_1100K) \
	X T(nox_air_debug_lowT_fresh_characterization) \
	X T(nox_air_debug_reuse_descending_pathology)

REGISTER_TESTS_SIMPLE(solver_fprops, TESTS)
#undef X
