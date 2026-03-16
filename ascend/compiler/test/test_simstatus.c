#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/slvreq.h>
#include <ascend/compiler/simstatus.h>

#include <test/common.h>
#include <test/assertimpl.h>

typedef struct SimStatusHooksData{
	struct Instance *siminst;
	int solve_count;
	int study_count;
	unsigned solve_method_depth;
	unsigned study_method_depth;
	struct Instance *last_target;
} SimStatusHooksData;

static struct Instance *
load_model(const char *name){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	m = Asc_OpenModule("test/compiler/simstatus.a4c",&status);
	CU_ASSERT_FATAL(m != NULL);
	CU_ASSERT_FATAL(status == 0);
	CU_ASSERT_FATAL(zz_parse() == 0);

	struct Instance *sim = SimsCreateInstance(AddSymbol(name), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim != NULL);
	return sim;
}

static enum Proc_enum
run_method(struct Instance *sim, const char *method){
	struct Name *name = CreateIdName(AddSymbol(method));
	return Initialize(GetSimulationRoot(sim), name, "sim1", ASCERR, WP_STOPONERR, NULL, NULL);
}

static int
simstatus_set_solver(const char *solvername, void *user_data){
	(void)solvername;
	asc_simstatus_mark_dirty(((SimStatusHooksData *)user_data)->siminst);
	return 0;
}

static int
simstatus_set_option(const char *optionname, struct value_t *val, void *user_data){
	(void)optionname;
	(void)val;
	asc_simstatus_mark_dirty(((SimStatusHooksData *)user_data)->siminst);
	return 0;
}

static int
simstatus_do_solve(struct Instance *instance, void *user_data){
	SimStatusHooksData *data = (SimStatusHooksData *)user_data;
	if(instance == NULL){
		instance = GetSimulationRoot(data->siminst);
	}
	++data->solve_count;
	data->solve_method_depth = asc_simstatus_method_depth(data->siminst);
	data->last_target = instance;
	asc_simstatus_mark_clean(data->siminst, instance);
	return 0;
}

static int
simstatus_do_study(const SlvReqStudyRequest *request, void *user_data){
	SimStatusHooksData *data = (SimStatusHooksData *)user_data;
	(void)request;
	++data->study_count;
	data->study_method_depth = asc_simstatus_method_depth(data->siminst);
	data->last_target = GetSimulationRoot(data->siminst);
	asc_simstatus_mark_clean(data->siminst, data->last_target);
	return 0;
}

static int
simstatus_delete_system(void *user_data){
	asc_simstatus_mark_dirty(((SimStatusHooksData *)user_data)->siminst);
	return 0;
}

static void
assign_hooks(struct Instance *sim, SimStatusHooksData *data){
	SlvReqHooks hooks = SLVREQ_HOOKS_EMPTY;
	memset(data, 0, sizeof(*data));
	data->siminst = sim;
	hooks.set_solver_fn = &simstatus_set_solver;
	hooks.set_option_fn = &simstatus_set_option;
	hooks.do_solve_fn = &simstatus_do_solve;
	hooks.do_study_fn = &simstatus_do_study;
	hooks.delete_system_fn = &simstatus_delete_system;
	hooks.user_data = data;
	CU_ASSERT_FATAL(slvreq_assign_hooks(sim, &hooks) == 0);
}

static void test_initial_dirty(void){
	struct Instance *sim = load_model("simstatus_test");
	CU_ASSERT(asc_simstatus_is_dirty(sim));
	CU_ASSERT(asc_simstatus_method_depth(sim) == 0);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_dirty_assign(void){
	struct Instance *sim = load_model("simstatus_test");
	CU_ASSERT(run_method(sim, "dirty_assign") == Proc_all_ok);
	CU_ASSERT(asc_simstatus_is_dirty(sim));
	CU_ASSERT(asc_simstatus_method_depth(sim) == 0);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_dirty_fix(void){
	struct Instance *sim = load_model("simstatus_test");
	CU_ASSERT(run_method(sim, "dirty_fix") == Proc_all_ok);
	CU_ASSERT(asc_simstatus_is_dirty(sim));
	CU_ASSERT(asc_simstatus_method_depth(sim) == 0);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_dirty_then_solve_cleans(void){
	struct Instance *sim = load_model("simstatus_test");
	SimStatusHooksData data;
	assign_hooks(sim, &data);
	CU_ASSERT(run_method(sim, "dirty_then_solve") == Proc_all_ok);
	CU_ASSERT(!asc_simstatus_is_dirty(sim));
	CU_ASSERT(data.solve_count == 1);
	CU_ASSERT(data.solve_method_depth > 0);
	CU_ASSERT(asc_simstatus_method_depth(sim) == 0);
	CU_ASSERT(asc_simstatus_get_last_solve_target(sim) == GetSimulationRoot(sim));
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_solve_then_dirty_stays_dirty(void){
	struct Instance *sim = load_model("simstatus_test");
	SimStatusHooksData data;
	assign_hooks(sim, &data);
	CU_ASSERT(run_method(sim, "solve_then_dirty") == Proc_all_ok);
	CU_ASSERT(asc_simstatus_is_dirty(sim));
	CU_ASSERT(data.solve_count == 1);
	CU_ASSERT(data.solve_method_depth > 0);
	CU_ASSERT(asc_simstatus_method_depth(sim) == 0);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_solve_child_records_target(void){
	struct Instance *sim = load_model("simstatus_test");
	struct Instance *child;
	SimStatusHooksData data;
	assign_hooks(sim, &data);
	child = ChildByChar(GetSimulationRoot(sim), AddSymbol("child"));
	CU_ASSERT_FATAL(child != NULL);
	CU_ASSERT(run_method(sim, "solve_child") == Proc_all_ok);
	CU_ASSERT(!asc_simstatus_is_dirty(sim));
	CU_ASSERT(data.last_target == child);
	CU_ASSERT(asc_simstatus_get_last_solve_target(sim) == child);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_study_now_cleans(void){
	struct Instance *sim = load_model("simstatus_test");
	SimStatusHooksData data;
	assign_hooks(sim, &data);
	CU_ASSERT(run_method(sim, "study_now") == Proc_all_ok);
	CU_ASSERT(!asc_simstatus_is_dirty(sim));
	CU_ASSERT(data.study_count == 1);
	CU_ASSERT(data.study_method_depth > 0);
	CU_ASSERT(asc_simstatus_method_depth(sim) == 0);
	CU_ASSERT(asc_simstatus_get_last_solve_target(sim) == GetSimulationRoot(sim));
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

static void test_study_then_dirty_stays_dirty(void){
	struct Instance *sim = load_model("simstatus_test");
	SimStatusHooksData data;
	assign_hooks(sim, &data);
	CU_ASSERT(run_method(sim, "study_then_dirty") == Proc_all_ok);
	CU_ASSERT(asc_simstatus_is_dirty(sim));
	CU_ASSERT(data.study_count == 1);
	CU_ASSERT(data.study_method_depth > 0);
	CU_ASSERT(asc_simstatus_method_depth(sim) == 0);
	sim_destroy(sim);
	Asc_CompilerDestroy();
}

#define TESTS(T) \
	T(initial_dirty) \
	T(dirty_assign) \
	T(dirty_fix) \
	T(dirty_then_solve_cleans) \
	T(solve_then_dirty_stays_dirty) \
	T(solve_child_records_target) \
	T(study_now_cleans) \
	T(study_then_dirty_stays_dirty)

REGISTER_TESTS_SIMPLE(compiler_simstatus, TESTS)
