#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

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
#include <ascend/compiler/derivinst.h>

#include <ascend/utilities/error.h>

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
	int set_runtime_options;
	int check_parameter_surface;
	double time_limit;
	int threads;
	double mip_rel_gap;
	double mip_abs_gap;
	int random_seed;
	const char *presolve;
	const char *solver;
	const char *parallel;
	const char *initial_method; /* NULL: on_load; empty: no initialisation. */
	const char *setup_method;
	int run_self_test;
	int toggle_relaxation_after_presolve;
};

struct reporter_capture{
	char *buffer;
	size_t cap;
	size_t len;
	int echo;
};

static struct reporter_capture g_reporter_capture = {NULL,0,0,0};

struct progress_capture{
	char *buffer;
	size_t cap;
	size_t len;
	int echo;
};

static struct progress_capture g_progress_capture = {NULL,0,0,0};

static void reporter_capture_begin_ex(char *buffer, size_t cap, int echo){
	g_reporter_capture.buffer = buffer;
	g_reporter_capture.cap = cap;
	g_reporter_capture.len = 0;
	g_reporter_capture.echo = echo ? 1 : 0;
	if(buffer != NULL && cap > 0){
		buffer[0] = '\0';
	}
}

static void reporter_capture_begin(char *buffer, size_t cap){
	reporter_capture_begin_ex(buffer,cap,0);
}

static void reporter_capture_end(void){
	g_reporter_capture.buffer = NULL;
	g_reporter_capture.cap = 0;
	g_reporter_capture.len = 0;
	g_reporter_capture.echo = 0;
}

static void progress_capture_begin_ex(char *buffer, size_t cap, int echo){
	g_progress_capture.buffer = buffer;
	g_progress_capture.cap = cap;
	g_progress_capture.len = 0;
	g_progress_capture.echo = echo ? 1 : 0;
	if(buffer != NULL && cap > 0){
		buffer[0] = '\0';
	}
}

static void progress_capture_end(void){
	g_progress_capture.buffer = NULL;
	g_progress_capture.cap = 0;
	g_progress_capture.len = 0;
	g_progress_capture.echo = 0;
}

static int highs_capture_progress_callback(const char *solver_name, const char *message, void *user_data){
	int wrote;
	char line[1024];
	(void)user_data;
	wrote = snprintf(
		line,sizeof(line),"HiGHS progress: solver=%s, %s"
		,(solver_name != NULL ? solver_name : "")
		,(message != NULL ? message : "")
	);
	if(wrote < 0)return 0;

	if(g_progress_capture.echo){
		fprintf(stderr,"%s\n",line);
	}
	if(g_progress_capture.buffer == NULL || g_progress_capture.cap == 0){
		return 0;
	}
	if(g_progress_capture.len >= g_progress_capture.cap - 1){
		return 0;
	}
	if((size_t)wrote >= g_progress_capture.cap - g_progress_capture.len){
		g_progress_capture.len = g_progress_capture.cap - 1;
		g_progress_capture.buffer[g_progress_capture.len] = '\0';
		return 0;
	}
	memcpy(g_progress_capture.buffer + g_progress_capture.len,line,(size_t)wrote);
	g_progress_capture.len += (size_t)wrote;
	if(g_progress_capture.len + 1 < g_progress_capture.cap){
		g_progress_capture.buffer[g_progress_capture.len++] = '\n';
		g_progress_capture.buffer[g_progress_capture.len] = '\0';
	}
	return 0;
}

static int highs_capture_error_reporter(ERROR_REPORTER_CALLBACK_ARGS);

static int highs_capture_error_reporter(
	const error_severity_t sev, const char *filename,
	const int line, const char *funcname,
	const char *fmt, va_list args
){
	int wrote;
	(void)sev;
	(void)filename;
	(void)line;
	(void)funcname;

	if(g_reporter_capture.echo){
		va_list args_copy;
		va_copy(args_copy,args);
		vfprintf(stderr,fmt,args_copy);
		fputc('\n',stderr);
		va_end(args_copy);
	}

	if(g_reporter_capture.buffer == NULL || g_reporter_capture.cap == 0){
		return 0;
	}
	if(g_reporter_capture.len >= g_reporter_capture.cap - 1){
		return 0;
	}

	wrote = vsnprintf(
		g_reporter_capture.buffer + g_reporter_capture.len,
		g_reporter_capture.cap - g_reporter_capture.len,
		fmt,args
	);
	if(wrote < 0){
		return 0;
	}
	if((size_t)wrote >= g_reporter_capture.cap - g_reporter_capture.len){
		g_reporter_capture.len = g_reporter_capture.cap - 1;
		g_reporter_capture.buffer[g_reporter_capture.len] = '\0';
		return wrote;
	}
	g_reporter_capture.len += (size_t)wrote;
	if(g_reporter_capture.len + 1 < g_reporter_capture.cap){
		g_reporter_capture.buffer[g_reporter_capture.len++] = '\n';
		g_reporter_capture.buffer[g_reporter_capture.len] = '\0';
	}
	return wrote;
}

static int find_param_index(const slv_parameters_t *pp, const char *name){
	int i;
	for(i = 0; i < pp->num_parms; ++i){
		if(pp->parms[i].name != NULL && 0 == strcmp(pp->parms[i].name,name)){
			return i;
		}
	}
	return -1;
}

static int highs_ci_serial_mode_enabled(void){
	const char *v = getenv("ASCEND_CI_HIGHS_SERIAL");
	if(v == NULL || v[0] == '\0'){
		v = getenv("GITHUB_ACTIONS");
	}
	return (v != NULL && v[0] != '\0' && !(v[0] == '0' && v[1] == '\0'));
}

static void highs_apply_ci_serial_overrides(slv_parameters_t *pp){
	int idx;
	if(pp == NULL || !highs_ci_serial_mode_enabled()){
		return;
	}
	idx = find_param_index(pp,"threads");
	if(idx != -1){
		SLV_PARAM_INT(pp,idx) = 1;
	}
	idx = find_param_index(pp,"parallel");
	if(idx != -1){
		slv_set_char_parameter(&(SLV_PARAM_CHAR(pp,idx)),"off");
	}
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

	if(!(opts && opts->initial_method && !opts->initial_method[0])){
		struct Name *name = CreateIdName(AddSymbol(opts && opts->initial_method ? opts->initial_method : "on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
		DestroyName(name);
	}
	if(opts && opts->setup_method){
		struct Name *name = CreateIdName(AddSymbol(opts->setup_method));
		CU_ASSERT(Initialize(GetSimulationRoot(siminst),name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL)==Proc_all_ok);
		DestroyName(name);
	}

	sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);
	CU_ASSERT_FATAL(slv_select_solver(sys,solver_index) != -1);
	CU_ASSERT_TRUE(slv_eligible_solver(sys));

	{
		slv_parameters_t pp;
		int nonlin_idx;
		int relaxed_idx;
		int i;
		slv_get_parameters(sys,&pp);
		nonlin_idx = find_param_index(&pp,"nonlin");
		relaxed_idx = find_param_index(&pp,"relaxed");
		CU_ASSERT_FATAL(nonlin_idx != -1);
		CU_ASSERT_FATAL(relaxed_idx != -1);

		if(opts != NULL && opts->check_parameter_surface){
				static const char *required[] = {
					"safeeval","nonlin","relaxed",
					"progress_callbacks",
					"time_limit","threads","presolve","solver","parallel",
					"mip_rel_gap","mip_abs_gap","random_seed",
					"pinf","minf",
				NULL
			};
			static const char *legacy[] = {
				"nonneg","obj","binary","integer","semi",
				"sos1","sos2","sos3","bo","eps",
				"boval","epsval","filename",
				NULL
			};
			for(i = 0; required[i] != NULL; ++i){
				CU_ASSERT_FATAL(find_param_index(&pp,required[i]) != -1);
			}
			for(i = 0; legacy[i] != NULL; ++i){
				CU_ASSERT_FATAL(find_param_index(&pp,legacy[i]) == -1);
			}
		}

		SLV_PARAM_BOOL(&pp,nonlin_idx) = FALSE;
		SLV_PARAM_BOOL(&pp,relaxed_idx) = (opts != NULL && opts->relaxed) ? TRUE : FALSE;

		if(opts != NULL && opts->set_runtime_options){
			int idx;
			idx = find_param_index(&pp,"time_limit");
			CU_ASSERT_FATAL(idx != -1);
			SLV_PARAM_REAL(&pp,idx) = opts->time_limit;

			idx = find_param_index(&pp,"threads");
			CU_ASSERT_FATAL(idx != -1);
			SLV_PARAM_INT(&pp,idx) = opts->threads;

			idx = find_param_index(&pp,"mip_rel_gap");
			CU_ASSERT_FATAL(idx != -1);
			SLV_PARAM_REAL(&pp,idx) = opts->mip_rel_gap;

			idx = find_param_index(&pp,"mip_abs_gap");
			CU_ASSERT_FATAL(idx != -1);
			SLV_PARAM_REAL(&pp,idx) = opts->mip_abs_gap;

			idx = find_param_index(&pp,"random_seed");
			CU_ASSERT_FATAL(idx != -1);
			SLV_PARAM_INT(&pp,idx) = opts->random_seed;

			idx = find_param_index(&pp,"presolve");
			CU_ASSERT_FATAL(idx != -1);
			slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,idx))
				,(opts->presolve != NULL ? opts->presolve : "choose")
			);

			idx = find_param_index(&pp,"solver");
			CU_ASSERT_FATAL(idx != -1);
			slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,idx))
				,(opts->solver != NULL ? opts->solver : "choose")
			);

			idx = find_param_index(&pp,"parallel");
			CU_ASSERT_FATAL(idx != -1);
			slv_set_char_parameter(&(SLV_PARAM_CHAR(&pp,idx))
				,(opts->parallel != NULL ? opts->parallel : "choose")
			);
		}

		highs_apply_ci_serial_overrides(&pp);
		slv_set_parameters(sys,&pp);
	}

	(void)slv_presolve(sys);
	if(opts && opts->toggle_relaxation_after_presolve){
		slv_parameters_t pp;
		slv_get_parameters(sys,&pp);
		SLV_PARAM_BOOL(&pp,find_param_index(&pp,"relaxed"))=!opts->relaxed;
		slv_set_parameters(sys,&pp);
	}
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
	if(opts && opts->run_self_test){
		struct Name *name = CreateIdName(AddSymbol("self_test"));
		CU_ASSERT(Initialize(GetSimulationRoot(siminst),name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL)==Proc_all_ok);
		DestroyName(name);
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

static int highs_get_kind_after_presolve(
	const char *module_path, const char *model_name, int relaxed, slv_status_kind_t *kind_out
){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	int ran = 0;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS kind test: solver package not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS kind test: solver not registered");
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
		SLV_PARAM_BOOL(&pp,relaxed_idx) = (relaxed ? TRUE : FALSE);
		highs_apply_ci_serial_overrides(&pp);
		slv_set_parameters(sys,&pp);
	}

	(void)slv_presolve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.ready_to_solve);
	if(kind_out != NULL){
		*kind_out = status.kind;
	}
	ran = 1;

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
	return ran;
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

static void test_highs_lp_structured_table(void){
	/* Structured LP TABLE variant: same solve/path expectations with TABLE-loaded constants. */
	static const struct var_expect expected[] = {
		{"x[1]", 2.0, 1e-7},
		{"x[2]", 2.0, 1e-7},
		{"row[1].s", 0.0, 1e-7},
		{"row[2].s", 0.0, 1e-7}
	};
	run_highs_model("models/test/ipopt/lp_structured_table.a4c","lp_structured_table",-10.0,0,expected,4,NULL);
}

static void test_highs_kind_after_presolve_lp(void){
	/* LP model should classify as LP right after presolve. */
	slv_status_kind_t kind = SLV_STATUS_UNKNOWN;
	int ran = highs_get_kind_after_presolve("models/test/ipopt/lp1.a4c","lp1",0,&kind);
	if(!ran)return;
	CU_ASSERT_EQUAL(kind, SLV_STATUS_LP);
}

static void test_highs_mip_mixed(void){
	/* Mixed-integer model: covers HiGHS MIP load path and mixed row operators. */
	static const struct highs_run_options opts = {0,0,0,0,1e-7};
	run_highs_model("models/test/mip/mip_mixed.a4c","mip_mixed",0.0,0,NULL,0,&opts);
}

static void test_highs_kind_after_presolve_mip(void){
	/* MIP model should classify as MIP right after presolve. */
	slv_status_kind_t kind = SLV_STATUS_UNKNOWN;
	int ran = highs_get_kind_after_presolve("models/test/mip/mip_mixed.a4c","mip_mixed",0,&kind);
	if(!ran)return;
	CU_ASSERT_EQUAL(kind, SLV_STATUS_MIP);
}

static void test_highs_kind_after_presolve_relaxed_mip(void){
	/* Relaxed MIP should classify as LP right after presolve. */
	slv_status_kind_t kind = SLV_STATUS_UNKNOWN;
	int ran = highs_get_kind_after_presolve("models/test/mip/mip_mixed.a4c","mip_mixed",1,&kind);
	if(!ran)return;
	CU_ASSERT_EQUAL(kind, SLV_STATUS_LP);
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

/* Both adapters use the same exported domains, including convex-hull bounds
 * for semicontinuous relaxation. Keep fixtures solver-independent. */
#define DOMAIN_CASE(NAME,MODEL,RELAXED,OBJ) \
static void test_##NAME(void){ \
	struct highs_run_options opts={0}; \
	opts.relaxed=RELAXED; opts.objective_tol=1e-7; opts.expect_converged=1; \
	run_highs_model("models/test/mip/domains.a4c",MODEL,OBJ,0,NULL,0,&opts); \
}
DOMAIN_CASE(highs_semi_zero,"mip_semi_gap",0,0)
DOMAIN_CASE(highs_semi_active,"mip_semi_active",0,2)
DOMAIN_CASE(highs_semi_relaxed,"mip_semi_gap",1,1)
DOMAIN_CASE(highs_semi_individually_relaxed,"mip_semi_individually_relaxed",0,1)
DOMAIN_CASE(highs_integer_individually_relaxed,"mip_integer_individually_relaxed",0,2.5)
DOMAIN_CASE(highs_domains,"mip_domains",0,-1)
DOMAIN_CASE(highs_domains_relaxed,"mip_domains",1,0)
DOMAIN_CASE(highs_domains_partially_relaxed,"mip_domains_partially_relaxed",0,-0.5)
DOMAIN_CASE(highs_binary_wide_bounds,"mip_binary_wide_bounds",0,1)
DOMAIN_CASE(highs_binary_wide_bounds_relaxed,"mip_binary_wide_bounds",1,1)
#undef DOMAIN_CASE

static void test_highs_relax_after_presolve(void){
	struct highs_run_options opts={0};
	opts.expect_converged=1;
	opts.toggle_relaxation_after_presolve=1;
	run_highs_model("models/test/mip/domains.a4c","mip_semi_gap",1,0,NULL,0,&opts);
}

static void test_highs_unrelax_after_presolve(void){
	struct highs_run_options opts={0};
	opts.expect_converged=1;
	opts.relaxed=1;
	opts.toggle_relaxation_after_presolve=1;
	run_highs_model("models/test/mip/domains.a4c","mip_semi_gap",0,0,NULL,0,&opts);
}

static void test_highs_mip_facility_location_table_labels(void){
	/* String-labeled TABLE variant of facility-location benchmark. */
	static const struct var_expect expected[] = {
		{"open['alpha']", 1.0, 1e-7},
		{"open['beta']", 0.0, 1e-7},
		{"open['gamma']", 1.0, 1e-7},
		{"assign['cust1']['alpha']", 1.0, 1e-7},
		{"assign['cust2']['gamma']", 1.0, 1e-7},
		{"assign['cust3']['gamma']", 1.0, 1e-7},
		{"assign['cust4']['gamma']", 1.0, 1e-7}
	};
	run_highs_model(
		"models/test/mip/facility_location_table_labels.a4c",
		"mip_facility_location_table_labels",
		470.0,
		0,
		expected,
		7,
		NULL
	);
}

static void test_highs_mip_tsp_mtz8(void){
	/* Classic TSP MIP with MTZ subtour constraints on an asymmetric 8-city instance. */
	static const struct var_expect expected[] = {
		{"x[1][4]", 1.0, 1e-7},
		{"x[4][2]", 1.0, 1e-7},
		{"x[2][5]", 1.0, 1e-7},
		{"x[5][6]", 1.0, 1e-7},
		{"x[6][7]", 1.0, 1e-7},
		{"x[7][3]", 1.0, 1e-7},
		{"x[3][8]", 1.0, 1e-7},
		{"x[8][1]", 1.0, 1e-7}
	};
	run_highs_model(
		"models/test/mip/tsp_mtz8.a4c",
		"mip_tsp_mtz8",
		166.0,
		0,
		expected,
		8,
		NULL
	);
}

static void test_highs_mip_tsp_mtz8_table_labels(void){
	/* String-labeled TABLE variant of the MTZ 8-city TSP benchmark. */
	static const struct var_expect expected[] = {
		{"x['a']['d']", 1.0, 1e-7},
		{"x['d']['b']", 1.0, 1e-7},
		{"x['b']['e']", 1.0, 1e-7},
		{"x['e']['f']", 1.0, 1e-7},
		{"x['f']['g']", 1.0, 1e-7},
		{"x['g']['c']", 1.0, 1e-7},
		{"x['c']['h']", 1.0, 1e-7},
		{"x['h']['a']", 1.0, 1e-7}
	};
	run_highs_model(
		"models/test/mip/tsp_mtz8_table_labels.a4c",
		"mip_tsp_mtz8_table_labels",
		166.0,
		0,
		expected,
		8,
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

static void test_highs_option_surface(void){
	/* Ensure only curated HiGHS options are exposed, and they can be applied in a solve. */
	static const struct var_expect expected[] = {
		{"x", 2.0, 1e-7},
		{"y", 2.0, 1e-7},
		{"s1", 0.0, 1e-7},
		{"s2", 0.0, 1e-7}
	};
	static const struct highs_run_options opts = {
		0,0,0,1,1e-7,
		1,1,
		5.0,1,
		1e-6,1e-7,7,
		"on","simplex","off"
	};
	run_highs_model("models/test/ipopt/lp1.a4c","lp1",-10.0,1,expected,4,&opts);
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

static void test_highs_deriv_trivial_objective(void){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	struct Instance *root = NULL;
	struct Instance *x = NULL;
	struct Instance *y = NULL;
	struct Instance *d = NULL;
	struct Instance *objinst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS derivative-trivial-objective test: solver package not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS derivative-trivial-objective test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/highs/deriv_lp.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("highs_deriv_trivial")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("highs_deriv_trivial"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
	}

	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	x = ChildByChar(root,AddSymbol("x"));
	y = ChildByChar(root,AddSymbol("y"));
	objinst = ChildByChar(root,AddSymbol("obj"));
	CU_ASSERT_FATAL(x != NULL);
	CU_ASSERT_FATAL(y != NULL);
	CU_ASSERT_FATAL(objinst != NULL);

	sys = system_build(root);
	CU_ASSERT_FATAL(sys != NULL);
	d = InstanceGetDerivative(x);
	CU_ASSERT_FATAL(d != NULL);
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
		SLV_PARAM_BOOL(&pp,relaxed_idx) = FALSE;
		highs_apply_ci_serial_overrides(&pp);
		slv_set_parameters(sys,&pp);
	}

	(void)slv_presolve(sys);
	(void)slv_solve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.converged);
	CU_ASSERT_DOUBLE_EQUAL(0.0,RealAtomValue(y),1e-7);
	CU_ASSERT_DOUBLE_EQUAL(0.0,RealAtomValue(d),1e-7);
	CU_ASSERT_DOUBLE_EQUAL(0.0,RelationResidual(GetInstanceRelationOnly(objinst)),1e-7);

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_highs_deriv_free_variable(void){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	struct Instance *root = NULL;
	struct Instance *x = NULL;
	struct Instance *d = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	struct Instance *y = NULL;
	struct Instance *objinst = NULL;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS derivative-free-variable test: solver package not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS derivative-free-variable test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/highs/deriv_lp.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("highs_deriv_free")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("highs_deriv_free"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst != NULL);
	{
		struct Name *name = CreateIdName(AddSymbol("on_load"));
		enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
		CU_ASSERT(pe == Proc_all_ok);
	}

	root = GetSimulationRoot(siminst);
	CU_ASSERT_FATAL(root != NULL);
	x = ChildByChar(root,AddSymbol("x"));
	y = ChildByChar(root,AddSymbol("y"));
	objinst = ChildByChar(root,AddSymbol("obj"));
	CU_ASSERT_FATAL(x != NULL);
	CU_ASSERT_FATAL(y != NULL);
	CU_ASSERT_FATAL(objinst != NULL);
	d = InstanceGetDerivative(x);
	CU_ASSERT_FATAL(d != NULL);
	CU_ASSERT_FALSE(GetBooleanAtomValue(ChildByChar(d,AddSymbol("fixed"))));

	sys = system_build(root);
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
		SLV_PARAM_BOOL(&pp,relaxed_idx) = FALSE;
		highs_apply_ci_serial_overrides(&pp);
		slv_set_parameters(sys,&pp);
	}

	(void)slv_presolve(sys);
	(void)slv_solve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.converged);
	CU_ASSERT_DOUBLE_EQUAL(3.0,RealAtomValue(d),1e-7);
	CU_ASSERT_DOUBLE_EQUAL(3.0,RealAtomValue(y),1e-7);
	CU_ASSERT_DOUBLE_EQUAL(3.0,RelationResidual(GetInstanceRelationOnly(objinst)),1e-7);

cleanup:
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void test_highs_infeasible_diagnostics(void){
	/* Infeasible MIP should set status flags and emit non-optimal diagnostics. */
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	char captured[16384];
	int has_error = 0;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS infeasible diagnostic test: solver package not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS infeasible diagnostic test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/mip/mip_mixed.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("mip_mixed")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("mip_mixed"), AddSymbol("sim1"), e_normal, NULL);
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
		SLV_PARAM_BOOL(&pp,relaxed_idx) = FALSE;
		highs_apply_ci_serial_overrides(&pp);
		slv_set_parameters(sys,&pp);
	}

	reporter_capture_begin(captured,sizeof(captured));
	error_reporter_set_callback(&highs_capture_error_reporter);
	error_reporter_tree_start();
	(void)slv_presolve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.ready_to_solve);
	(void)slv_solve(sys);
	slv_get_status(sys,&status);
	has_error = error_reporter_tree_has_error();
	error_reporter_tree_end();
	error_reporter_set_callback(NULL);
	reporter_capture_end();

	CU_ASSERT_FALSE(has_error);
	CU_ASSERT_FALSE(status.converged);
	CU_ASSERT_TRUE(status.diverged);
	CU_ASSERT_TRUE(status.inconsistent);
	CU_ASSERT_FALSE(status.time_limit_exceeded);
	CU_ASSERT_FALSE(status.iteration_limit_exceeded);

	CU_ASSERT_FATAL(strstr(captured,"HiGHS terminated with status Infeasible") != NULL);
	CU_ASSERT_FATAL(strstr(captured,"HiGHS diagnostics:") != NULL);
	CU_ASSERT_FATAL(strstr(captured,"HiGHS did not return a feasible primal solution.") != NULL);

cleanup:
	error_reporter_set_callback(NULL);
	reporter_capture_end();
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static int solve_tsp_with_progress_option(
	int progress_callbacks_enabled, char *captured, size_t captured_cap
){
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;
	int ran = 0;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS progress callback test: solver package not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS progress callback test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/mip/tsp_mtz8.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("mip_tsp_mtz8")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("mip_tsp_mtz8"), AddSymbol("sim1"), e_normal, NULL);
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
		int idx;
		slv_get_parameters(sys,&pp);
		idx = find_param_index(&pp,"progress_callbacks");
		CU_ASSERT_FATAL(idx != -1);
		SLV_PARAM_BOOL(&pp,idx) = (progress_callbacks_enabled ? TRUE : FALSE);
		highs_apply_ci_serial_overrides(&pp);
		slv_set_parameters(sys,&pp);
	}

	progress_capture_begin_ex(captured,captured_cap,1);
	slv_set_progress_callback(&highs_capture_progress_callback,NULL);
	(void)slv_presolve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.ready_to_solve);
	(void)slv_solve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.converged);
	ran = 1;

cleanup:
	slv_clear_progress_callback();
	progress_capture_end();
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
	return ran;
}

static void test_highs_progress_callback_reporting(void){
	/* Progress callbacks should emit text reports when enabled, and be quiet when disabled. */
	char captured_on[32768];
	char captured_off[32768];
	int ran;

	ran = solve_tsp_with_progress_option(1,captured_on,sizeof(captured_on));
	if(!ran)return;
	CU_ASSERT_FATAL(strstr(captured_on,"HiGHS progress:") != NULL);

	ran = solve_tsp_with_progress_option(0,captured_off,sizeof(captured_off));
	if(!ran)return;
	CU_ASSERT_TRUE(strstr(captured_off,"HiGHS progress:") == NULL);
}

static void test_highs_interrupt_request(void){
	/* Solver interrupt flag should be honoured via HiGHS callback. */
	int solver_index = -1;
	struct Instance *siminst = NULL;
	slv_system_t sys = NULL;
	slv_status_t status;

	Asc_CompilerInit(1);
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_LIBRARY "=models"));
	CU_TEST(0 == Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/highs"));

	if(0 != package_load("highs",NULL)){
		CONSOLE_DEBUG("Skipping HiGHS interrupt test: solver package not available");
		goto cleanup;
	}
	solver_index = slv_lookup_client("HiGHS");
	if(solver_index == -1){
		CONSOLE_DEBUG("Skipping HiGHS interrupt test: solver not registered");
		goto cleanup;
	}

	{
		int status_open;
		Asc_OpenModule("models/test/mip/tsp_mtz8.a4c",&status_open);
		CU_ASSERT_FATAL(status_open == 0);
	}
	CU_ASSERT(0 == zz_parse());
	CU_ASSERT_FATAL(FindType(AddSymbol("mip_tsp_mtz8")) != NULL);

	siminst = SimsCreateInstance(AddSymbol("mip_tsp_mtz8"), AddSymbol("sim1"), e_normal, NULL);
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
		slv_get_parameters(sys,&pp);
		highs_apply_ci_serial_overrides(&pp);
		slv_set_parameters(sys,&pp);
	}

	(void)slv_presolve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.ready_to_solve);

	slv_set_solver_interrupt(1);
	(void)slv_solve(sys);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.panic);
	CU_ASSERT_FALSE(status.converged);

cleanup:
	slv_set_solver_interrupt(0);
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(siminst)sim_destroy(siminst);
	Asc_CompilerDestroy();
}

static void run_alloy_showcase(int detailed, const char *setup_method, double objective){
	struct highs_run_options opts={0};
	opts.initial_method="initialise";
	opts.setup_method=setup_method;
	opts.run_self_test=1;
	opts.expect_converged=1;
	run_highs_model(detailed ? "models/alloy_blending_detailed.a4c" : "models/alloy_blending.a4c",
		detailed ? "alloy_blending_detailed" : "alloy_blending",objective,0,NULL,0,&opts);
}
static void test_highs_alloy_blending(void){run_alloy_showcase(0,NULL,4.98);}
static void test_highs_alloy_blending_mass_balance(void){run_alloy_showcase(0,"with_mass_balance",4.98);}
static void test_highs_alloy_blending_ten_pounds(void){run_alloy_showcase(1,"ten_pound_batch",49.8);}
static void test_highs_alloy_blending_detailed(void){run_alloy_showcase(1,NULL,4.98);}
static void test_highs_alloy_blending_detailed_mass_balance(void){run_alloy_showcase(1,"with_mass_balance",4.98);}

static void test_highs_steel_production(void){
	struct highs_run_options opts={0};
	/* No setup needed; select the solver here without frontend SOLVER hooks. */
	opts.initial_method="";
	opts.run_self_test=1;
	opts.expect_converged=1;
	opts.objective_tol=1e-6;
	run_highs_model("models/steel_production.a4c","steel_production_highs",515033,0,NULL,0,&opts);
}

static void test_highs_food_manufacture_2(void){
	struct highs_run_options opts={0};
	opts.initial_method="";
	opts.run_self_test=1;
	opts.expect_converged=1;
	opts.objective_tol=1e-6;
	/* Request a proof rather than stopping at the default relative MIP gap. */
	opts.set_runtime_options=1;
	opts.time_limit=60;
	opts.threads=1;
	opts.mip_rel_gap=0;
	opts.mip_abs_gap=1e-7;
	run_highs_model("models/food_manufacture_2.a4c","food_manufacture_2_highs",100278.7037037037,0,NULL,0,&opts);
}

static void test_highs_job_shop(void){
	struct highs_run_options opts={0};
	opts.initial_method="";
	opts.run_self_test=1;
	opts.expect_converged=1;
	opts.objective_tol=1e-6;
	opts.set_runtime_options=1;
	opts.time_limit=60;
	opts.threads=1;
	opts.mip_rel_gap=0;
	opts.mip_abs_gap=1e-7;
	/* ASCEND stores the time-valued objective in seconds, not minutes. */
	run_highs_model("models/job_shop.a4c","job_shop_highs",97*60,0,NULL,0,&opts);
}

static void run_refinery_showcase(const char *model, double profit_per_day){
	struct highs_run_options opts={0};
	opts.initial_method="";
	opts.run_self_test=1;
	opts.expect_converged=1;
	/* The model uses currency/time: the native objective is USD/s. */
	run_highs_model("models/refinery.a4c",model,profit_per_day/86400,0,NULL,0,&opts);
}
static void test_highs_refinery(void){run_refinery_showcase("refinery_highs",6588.421476681333);}
static void test_highs_refinery_low_sulfur(void){run_refinery_showcase("refinery_low_sulfur_highs",0);}

#define TESTS(T) \
	T(highs_job_shop) \
	T(highs_food_manufacture_2) \
	T(highs_refinery) \
	T(highs_refinery_low_sulfur) \
	T(highs_steel_production) \
	T(highs_alloy_blending) \
	T(highs_alloy_blending_mass_balance) \
	T(highs_alloy_blending_ten_pounds) \
	T(highs_alloy_blending_detailed) \
	T(highs_alloy_blending_detailed_mass_balance) \
	T(highs_lp1) \
	T(highs_lp_structured) \
	T(highs_lp_structured_table) \
	T(highs_kind_after_presolve_lp) \
	T(highs_mip_mixed) \
	T(highs_kind_after_presolve_mip) \
	T(highs_kind_after_presolve_relaxed_mip) \
	T(highs_mip_mixed_iterate) \
	T(highs_mip_mixed_resolve) \
	T(highs_mip_facility_location) \
	T(highs_semi_zero) T(highs_semi_active) T(highs_semi_relaxed) \
	T(highs_semi_individually_relaxed) T(highs_integer_individually_relaxed) \
	T(highs_domains) T(highs_domains_relaxed) T(highs_binary_wide_bounds) T(highs_binary_wide_bounds_relaxed) \
	T(highs_domains_partially_relaxed) \
	T(highs_relax_after_presolve) T(highs_unrelax_after_presolve) \
	T(highs_mip_facility_location_table_labels) \
	T(highs_mip_tsp_mtz8) \
	T(highs_mip_tsp_mtz8_table_labels) \
	T(highs_trnsport) \
	T(highs_blend_whiskas2) \
	T(highs_afiro) \
	T(highs_option_surface) \
	T(highs_ineligible_without_objective) \
	T(highs_deriv_trivial_objective) \
	T(highs_deriv_free_variable) \
	T(highs_infeasible_diagnostics) \
	T(highs_progress_callback_reporting) \
	T(highs_interrupt_request)

REGISTER_TESTS_SIMPLE(solver_highs, TESTS)
