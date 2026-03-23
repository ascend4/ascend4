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

#include <test/common.h>
//#define TSR_DEBUG
#ifdef TSR_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

typedef struct SlvReqC_struct{
	struct Instance *siminst;
	slv_system_t sys;
	struct Instance *buildroot;
	char solvername[64];
	int delete_count;
	int study_count;
	unsigned long study_n_observed;
	long study_steps;
	enum SlvReqStudyMode study_mode;
	enum SlvReqStudyDistribution study_dist;
	double study_lower;
	double study_upper;
	double study_value;
	unsigned int study_now;
	char study_vary[128];
	char study_obs0[128];
	char study_obs1[128];
	char study_run_method[64];
	char study_filename[256];
} SlvReqC;

SlvReqSetSolverFn slvreq_c_set_solver;
SlvReqSetOptionFn slvreq_c_set_option;
SlvReqDoSolveFn slvreq_c_do_solve;
SlvReqDoStudyFn slvreq_c_do_study;
SlvReqDeleteSystemFn slvreq_c_delete_system;

static int find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i=0; i<pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && strcmp(pp->parms[i].name,name)==0){
			return i;
		}
	}
	return -1;
}

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
		S->buildroot = GetSimulationRoot(S->siminst);
	}
	snprintf(S->solvername,sizeof(S->solvername),"%s",solvername);

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
		S->sys = system_build(instance);
		S->buildroot = instance;
		if(S->sys == NULL){
			return SLVREQ_PRESOLVE_FAIL;
		}
		if(S->solvername[0] == '\0'){
			return SLVREQ_NO_SOLVER_SELECTED;
		}
		int index = slv_lookup_client(S->solvername);
		if(index == -1 || slv_select_solver(S->sys,index) == -1){
			return SLVREQ_NO_SOLVER_SELECTED;
		}
	}

	res = slv_presolve(S->sys);
	if(res)return SLVREQ_PRESOLVE_FAIL;

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
		CONSOLE_DEBUG("Solver completed OK");
		return 0;
	}

	if(status.diverged)CONSOLE_DEBUG("Solver diverged");
	if(status.inconsistent)CONSOLE_DEBUG("System is inconsistent");
	if(status.iteration_limit_exceeded)CONSOLE_DEBUG("Solver exceeded iteration limit");
	if(status.calc_ok == 0)CONSOLE_DEBUG("Solver had residual calculation errors");
	if(status.over_defined)CONSOLE_DEBUG("Solver system is over-defined");
	if(status.under_defined)CONSOLE_DEBUG("Solver system is under-defined");
	if(status.time_limit_exceeded)CONSOLE_DEBUG("Solver exceeded time limit");

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

int slvreq_c_do_study(const SlvReqStudyRequest *request, void *user_data){
	SlvReqC *S = (SlvReqC *)user_data;
	struct Instance *root = GetSimulationRoot(S->siminst);
	char *name = NULL;

	++S->study_count;
	S->study_n_observed = request->n_observed;
	S->study_steps = request->steps;
	S->study_mode = request->mode;
	S->study_dist = request->distribution;
	S->study_lower = RealValue(request->lower);
	S->study_upper = RealValue(request->upper);
	S->study_value = (ValueKind(request->value) == real_value) ? RealValue(request->value) : 0.0;
	S->study_now = request->now;
	S->study_vary[0] = '\0';
	S->study_obs0[0] = '\0';
	S->study_obs1[0] = '\0';
	S->study_run_method[0] = '\0';
	S->study_filename[0] = '\0';

	if(request->vary != NULL){
		name = WriteInstanceNameString(request->vary, root);
		snprintf(S->study_vary,sizeof(S->study_vary),"%s",name);
		ascfree(name);
	}
	if(request->n_observed > 0){
		name = WriteInstanceNameString(request->observed[0], root);
		snprintf(S->study_obs0,sizeof(S->study_obs0),"%s",name);
		ascfree(name);
	}
	if(request->n_observed > 1){
		name = WriteInstanceNameString(request->observed[1], root);
		snprintf(S->study_obs1,sizeof(S->study_obs1),"%s",name);
		ascfree(name);
	}
	if(request->run_method != NULL){
		snprintf(S->study_run_method,sizeof(S->study_run_method),"%s",request->run_method);
	}
	if(request->filename != NULL){
		snprintf(S->study_filename,sizeof(S->study_filename),"%s",request->filename);
	}
	return 0;
}

/*
	Test that the slvreq mechanism works within the C layer
*/
static void test_slvreq_c(void){

	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	/* load the file */
#define TESTFILE "test2"
	m = Asc_OpenModule("test/slvreq/" TESTFILE ".a4c",&status);
	if (!m) {
		MSG("test_ipopt: failed OpenModule");
	}
	CU_ASSERT(status == 0);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(TESTFILE))!=NULL);

	/* instantiate it */
	SlvReqC S;
	S.siminst = SimsCreateInstance(AddSymbol(TESTFILE), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
#undef TESTFILE

	/* do the solver hooks */
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

    CONSOLE_DEBUG("RUNNING ON_LOAD");

	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* maybe if the model is really clever, it will have solved itself now! */

	/* all sorts of destruction */
	CONSOLE_DEBUG("DESTROYING NOW...");
	CU_ASSERT(NULL != S.siminst)
	if(S.sys)system_destroy(S.sys);

	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_target_switch(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	m = Asc_OpenModule("test/slvreq/test3.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test3"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test3"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_EQUAL(S.delete_count, 2);

	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_delete_system(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	m = Asc_OpenModule("test/slvreq/test4.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test4"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test4"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_EQUAL(S.delete_count, 2);

	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_study(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	m = Asc_OpenModule("test/slvreq/test5.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test5"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test5"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	S.study_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.do_study_fn = &slvreq_c_do_study,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_EQUAL(S.study_count, 1);
	CU_ASSERT_EQUAL(S.study_n_observed, 2);
	CU_ASSERT_EQUAL(S.study_mode, SLVREQ_STUDY_STEPS);
	CU_ASSERT_EQUAL(S.study_dist, SLVREQ_STUDY_DIST_DEFAULT);
	CU_ASSERT_EQUAL(S.study_steps, 4);
	CU_ASSERT_DOUBLE_EQUAL(S.study_lower, 2.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(S.study_upper, 6.0, 1e-12);
	CU_ASSERT_STRING_EQUAL(S.study_vary, "time_run");
	CU_ASSERT_STRING_EQUAL(S.study_obs0, "accel_required");
	CU_ASSERT_STRING_EQUAL(S.study_obs1, "time_run");
	CU_ASSERT_STRING_EQUAL(S.study_run_method, "");
	CU_ASSERT_STRING_EQUAL(S.study_filename, "study.tsv");

	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_study_observe_only(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/slvreq/test6.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test6"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test6"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	S.study_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.do_study_fn = &slvreq_c_do_study,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_EQUAL(S.study_count, 1);
	CU_ASSERT_EQUAL(S.study_n_observed, 2);
	CU_ASSERT_EQUAL(S.study_mode, SLVREQ_STUDY_NONE);
	CU_ASSERT_EQUAL(S.study_dist, SLVREQ_STUDY_DIST_DEFAULT);
	CU_ASSERT_STRING_EQUAL(S.study_vary, "");
	CU_ASSERT_STRING_EQUAL(S.study_obs0, "y");
	CU_ASSERT_STRING_EQUAL(S.study_obs1, "x");
	CU_ASSERT_STRING_EQUAL(S.study_run_method, "");
	CU_ASSERT_STRING_EQUAL(S.study_filename, "");

	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_study_log(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/slvreq/test7.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test7"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test7"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	S.study_count = 0;
	{
		SlvReqHooks hooks = {
			.do_study_fn = &slvreq_c_do_study,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_EQUAL(S.study_count, 1);
	CU_ASSERT_EQUAL(S.study_mode, SLVREQ_STUDY_STEPS);
	CU_ASSERT_EQUAL(S.study_dist, SLVREQ_STUDY_DIST_LOG);
	CU_ASSERT_EQUAL(S.study_steps, 2);
	CU_ASSERT_EQUAL(S.study_now, 1);
	CU_ASSERT_DOUBLE_EQUAL(S.study_lower, 1.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(S.study_upper, 100.0, 1e-12);
	CU_ASSERT_STRING_EQUAL(S.study_vary, "x");

	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_study_ratio(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/slvreq/test8.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test8"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test8"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	S.study_count = 0;
	{
		SlvReqHooks hooks = {
			.do_study_fn = &slvreq_c_do_study,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_EQUAL(S.study_count, 1);
	CU_ASSERT_EQUAL(S.study_mode, SLVREQ_STUDY_RATIO);
	CU_ASSERT_EQUAL(S.study_dist, SLVREQ_STUDY_DIST_LOG);
	CU_ASSERT_EQUAL(S.study_now, 0);
	CU_ASSERT_DOUBLE_EQUAL(S.study_lower, 1.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(S.study_upper, 16.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(S.study_value, 2.0, 1e-12);
	CU_ASSERT_STRING_EQUAL(S.study_vary, "x");

	system_free_reused_mem();
	sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_highs_options(void){
	struct module_t *m = NULL;
	int status = 0;
	SlvReqC S;
	S.siminst = NULL;
	S.sys = NULL;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs");

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping slvreq HiGHS option test: solver package not available");
		goto cleanup;
	}
	if(slv_lookup_client("HiGHS") == -1){
		CONSOLE_DEBUG("Skipping slvreq HiGHS option test: solver not registered");
		goto cleanup;
	}

	m = Asc_OpenModule("test/slvreq/highs_opts.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("highs_opts"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("highs_opts"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_FATAL(S.sys != NULL);
	{
		slv_parameters_t pp;
		int idx;
		slv_get_parameters(S.sys,&pp);

		idx = find_param_index(&pp,"threads");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT_EQUAL(SLV_PARAM_INT(&pp,idx),1);

		idx = find_param_index(&pp,"random_seed");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT_EQUAL(SLV_PARAM_INT(&pp,idx),7);

		idx = find_param_index(&pp,"time_limit");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT_DOUBLE_EQUAL(SLV_PARAM_REAL(&pp,idx),5.0,1e-12);

		idx = find_param_index(&pp,"mip_rel_gap");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT_DOUBLE_EQUAL(SLV_PARAM_REAL(&pp,idx),1e-6,1e-12);

		idx = find_param_index(&pp,"mip_abs_gap");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT_DOUBLE_EQUAL(SLV_PARAM_REAL(&pp,idx),1e-7,1e-12);

		idx = find_param_index(&pp,"presolve");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT(0 == strcmp(SLV_PARAM_CHAR(&pp,idx),"on"));

		idx = find_param_index(&pp,"solver");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT(0 == strcmp(SLV_PARAM_CHAR(&pp,idx),"simplex"));

			idx = find_param_index(&pp,"parallel");
			CU_ASSERT_FATAL(idx != -1);
			CU_ASSERT(0 == strcmp(SLV_PARAM_CHAR(&pp,idx),"off"));

			idx = find_param_index(&pp,"progress_callbacks");
			CU_ASSERT_FATAL(idx != -1);
			CU_ASSERT_FALSE(SLV_PARAM_BOOL(&pp,idx));
		}

cleanup:
	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	if(S.siminst)sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_initial_mode_toggle(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	m = Asc_OpenModule("test/slvreq/test9.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test9"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test9"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	CU_ASSERT_EQUAL(S.delete_count, 2);

	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	if(S.siminst)sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_fixed_derivative_operating_point(void){
	struct module_t *m;
	int status;
	SlvReqC S;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	m = Asc_OpenModule("test/slvreq/test10.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("test10"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("test10"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe==Proc_all_ok);
	}

	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	if(S.siminst)sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_slvreq_highs_options_invalid(void){
	struct module_t *m = NULL;
	int status = 0;
	SlvReqC S;
	S.siminst = NULL;
	S.sys = NULL;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs");

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping slvreq HiGHS invalid-option test: solver package not available");
		goto cleanup;
	}
	if(slv_lookup_client("HiGHS") == -1){
		CONSOLE_DEBUG("Skipping slvreq HiGHS invalid-option test: solver not registered");
		goto cleanup;
	}

	m = Asc_OpenModule("test/slvreq/highs_opts_invalid.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT(status == 0);
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("highs_opts_invalid"))!=NULL);

	S.siminst = SimsCreateInstance(AddSymbol("highs_opts_invalid"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(S.siminst!=NULL);
	S.sys = NULL;
	S.buildroot = NULL;
	S.solvername[0] = '\0';
	S.delete_count = 0;
	{
		SlvReqHooks hooks = {
			.set_solver_fn = &slvreq_c_set_solver,
			.set_option_fn = &slvreq_c_set_option,
			.do_solve_fn = &slvreq_c_do_solve,
			.delete_system_fn = &slvreq_c_delete_system,
			.user_data = &S
		};
		slvreq_assign_hooks(S.siminst, &hooks);
	}

	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		int has_error = 0;
		enum Proc_enum pe;
		error_reporter_tree_start();
		pe = Initialize(GetSimulationRoot(S.siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		has_error = error_reporter_tree_has_error();
		error_reporter_tree_end();
		(void)pe;
		CU_ASSERT_TRUE(has_error);
	}

	CU_ASSERT_FATAL(S.sys != NULL);
	{
		slv_parameters_t pp;
		slv_status_t st;
		int idx;
		slv_get_status(S.sys,&st);
		CU_ASSERT_FALSE(st.ok);
		CU_ASSERT_TRUE(st.diverged);
		slv_get_parameters(S.sys,&pp);
		idx = find_param_index(&pp,"solver");
		CU_ASSERT_FATAL(idx != -1);
		CU_ASSERT(0 == strcmp(SLV_PARAM_CHAR(&pp,idx),"definitely_invalid"));
	}

cleanup:
	if(S.sys)system_destroy(S.sys);
	system_free_reused_mem();
	if(S.siminst)sim_destroy(S.siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}



/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(slvreq_c) \
	T(slvreq_target_switch) \
	T(slvreq_delete_system) \
	T(slvreq_initial_mode_toggle) \
	T(slvreq_fixed_derivative_operating_point) \
	T(slvreq_study) \
	T(slvreq_study_observe_only) \
	T(slvreq_study_log) \
	T(slvreq_study_ratio) \
	T(slvreq_highs_options) \
	T(slvreq_highs_options_invalid)

REGISTER_TESTS_SIMPLE(solver_slvreq, TESTS)
