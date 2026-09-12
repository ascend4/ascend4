/* LP adapter regression tests. Licensed solves are opt-in so ordinary test
 * runs do not unexpectedly contact a license server or consume a token. */
#include <stdlib.h>
#include <string.h>
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
#include <ascend/system/lp_utils.h>
#include <ascend/solver/solver.h>
#include <test/common.h>

enum { EXACT, REJECT, SCALED, RESOLVE, RELAX, TANGENT, INFEASIBLE, UNBOUNDED, TIMEOUT, INTERRUPT, BAD_EVAL, PREPARE_ONLY,
	SHOWCASE, SHOWCASE_MB, SHOWCASE_10LB, SELF_TEST };

static int param(slv_parameters_t *p, const char *name){
	int i;
	for(i=0;i<p->num_parms;++i)if(!strcmp(p->parms[i].name,name))return i;
	CU_FAIL("Missing Gurobi parameter");
	return 0;
}
static struct var_variable *variable(slv_system_t sys,const char *name){
	struct var_variable **v=slv_get_solvers_var_list(sys);
	for(;*v;++v){
		char *n=var_make_name(sys,*v);
		int match=!strcmp(n,name) || (strlen(n)>strlen(name)
			&& n[strlen(n)-strlen(name)-1]=='.'
			&& !strcmp(n+strlen(n)-strlen(name),name));
		ASC_FREE(n);
		if(match)return *v;
	}
	return NULL;
}
static int interrupt_progress(const char *solver,const char *message,void *data){
	(void)solver;(void)message;(void)data;
	slv_set_solver_interrupt(1);
	return 0;
}

static void run_case(const char *path,const char *model,int mode,double expected){
	struct Instance *sim=NULL;
	slv_system_t sys=NULL;
	slv_parameters_t p;
	slv_status_t status;
	struct Name *name;
	int opened, selected, licensed=getenv("ASCEND_TEST_GUROBI") && !strcmp(getenv("ASCEND_TEST_GUROBI"),"1");
	int skip=0;
	int showcase=mode==SHOWCASE || mode==SHOWCASE_MB || mode==SHOWCASE_10LB;
	if(mode!=REJECT && mode!=BAD_EVAL && mode!=PREPARE_ONLY && !licensed){CU_SKIP("Set ASCEND_TEST_GUROBI=1 to run licensed Gurobi solves");return;}
	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/gurobi");
	if(package_load("gurobi",NULL)){
		if(licensed){CU_FAIL("Requested Gurobi plugin is unavailable");}
		else {skip=1;}
		goto cleanup;
	}
	selected=slv_lookup_client("Gurobi");
	CU_ASSERT(selected>=0); if(selected<0)goto cleanup;
	Asc_OpenModule(path,&opened);
	CU_ASSERT(opened==0); if(opened)goto cleanup;
	CU_ASSERT(zz_parse()==0);
	sim=SimsCreateInstance(AddSymbol(model),AddSymbol("sim1"),e_normal,NULL);
	CU_ASSERT(sim!=NULL); if(!sim)goto cleanup;
	/* SELF_TEST examples need no setup; their on_load only selects a solver
	 * through frontend hooks, which this C harness does not install. */
	if(mode!=SELF_TEST){
		name=CreateIdName(AddSymbol(showcase ? "initialise" : "on_load"));
		CU_ASSERT(Initialize(GetSimulationRoot(sim),name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL)==Proc_all_ok);
		DestroyName(name);
	}
	if(mode==SHOWCASE_MB || mode==SHOWCASE_10LB){
		name=CreateIdName(AddSymbol(mode==SHOWCASE_MB ? "with_mass_balance" : "ten_pound_batch"));
		CU_ASSERT(Initialize(GetSimulationRoot(sim),name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL)==Proc_all_ok);
		DestroyName(name);
	}
	sys=system_build(GetSimulationRoot(sim));
	CU_ASSERT(sys!=NULL); if(!sys)goto cleanup;
	CU_ASSERT(slv_select_solver(sys,selected)>=0);
	slv_get_parameters(sys,&p);
	CU_ASSERT(p.whose==selected);
	SLV_PARAM_INT(&p,param(&p,"threads"))=1;
	if(mode==RELAX)SLV_PARAM_BOOL(&p,param(&p,"relaxed"))=1;
	if(mode==TANGENT)SLV_PARAM_BOOL(&p,param(&p,"nonlin"))=1;
	if(mode==TIMEOUT)SLV_PARAM_REAL(&p,param(&p,"time_limit"))=0;
	if(mode==INFEASIBLE)SLV_PARAM_BOOL(&p,param(&p,"dual_reductions"))=0;
	if(mode==SCALED){
		SLV_PARAM_BOOL(&p,param(&p,"varnom_scale"))=1;
		SLV_PARAM_BOOL(&p,param(&p,"relnom_scale"))=1;
	}
	slv_set_parameters(sys,&p);
	if(mode==REJECT){
		CU_ASSERT_FALSE(slv_eligible_solver(sys));
		CU_ASSERT(slv_presolve(sys)!=0);
		slv_get_status(sys,&status);
		CU_ASSERT_FALSE(status.ready_to_solve);
		goto cleanup;
	}
	CU_ASSERT_TRUE(slv_eligible_solver(sys));
	if(mode==BAD_EVAL){
		CU_ASSERT(slv_presolve(sys)!=0);
		slv_get_status(sys,&status);
		CU_ASSERT_FALSE(status.ready_to_solve);
		CU_ASSERT_FALSE(status.calc_ok);
		goto cleanup;
	}
	CU_ASSERT(slv_presolve(sys)==0);
	if(mode==PREPARE_ONLY){
		mps_data_t m={0}; lp_sparse_t sparse={0};
		CU_ASSERT(lp_prepare(sys,&m,&status,1,1)==0);
		CU_ASSERT(lp_sparse_build(&sparse,&m,slv_get_solvers_var_list(sys),slv_get_obj_relation(sys),-1e20,1e20)==0);
		CU_ASSERT(sparse.num_row==2);
		CU_ASSERT(sparse.num_col==3);
		CU_ASSERT(sparse.maximize);
		CU_ASSERT_DOUBLE_EQUAL(sparse.objective_offset,7,1e-10);
		CU_ASSERT(sparse.num_nz==3);
		lp_sparse_destroy(&sparse);lp_nuke_pointers(&m);
		goto cleanup;
	}
	/* Frontends can change options after presolve, before iterate. */
	slv_get_parameters(sys,&p);
	slv_set_parameters(sys,&p);
	slv_get_status(sys,&status);
	CU_ASSERT_TRUE(status.ready_to_solve);
	if(mode==INTERRUPT)slv_set_progress_callback(interrupt_progress,NULL);
	CU_ASSERT(slv_iterate(sys)==0);
	slv_get_status(sys,&status);
	CU_ASSERT(status.kind==SLV_STATUS_LP);
	if(mode==INFEASIBLE || mode==UNBOUNDED || mode==TIMEOUT || mode==INTERRUPT){
		CU_ASSERT_FALSE(status.converged);
		if(mode==INFEASIBLE)CU_ASSERT_TRUE(status.inconsistent);
		if(mode==UNBOUNDED)CU_ASSERT_TRUE(status.diverged);
		if(mode==TIMEOUT)CU_ASSERT_TRUE(status.time_limit_exceeded);
		if(mode==INTERRUPT)CU_ASSERT_TRUE(status.panic);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"x")),expected,1e-8);
		goto cleanup;
	}
	CU_ASSERT_TRUE(status.converged);
	CU_ASSERT_TRUE(status.u.lp.have_objective);
	CU_ASSERT_DOUBLE_EQUAL(status.u.lp.objective_value,expected,1e-6);
	CU_ASSERT_DOUBLE_EQUAL(rel_residual(slv_get_obj_relation(sys)),expected,1e-6);
	if(showcase || mode==SELF_TEST){
		name=CreateIdName(AddSymbol("self_test"));
		CU_ASSERT(Initialize(GetSimulationRoot(sim),name,"sim1",ASCERR,WP_STOPONERR,NULL,NULL)==Proc_all_ok);
		DestroyName(name);
	}
	if(mode==SCALED || mode==RESOLVE){
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"p")),4,1e-8);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"x")),2,1e-8);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"y")),2,1e-8);
	}
	if(mode==RESOLVE){
		var_set_upper_bound(variable(sys,"x"),1);
		var_set_value(variable(sys,"p"),6);
		CU_ASSERT(slv_resolve(sys)==0);
		slv_get_status(sys,&status);
		CU_ASSERT_TRUE(status.converged);
		CU_ASSERT_DOUBLE_EQUAL(status.u.lp.objective_value,20,1e-7);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"x")),1,1e-8);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"y")),5,1e-8);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"p")),6,1e-8);
	}
cleanup:
	slv_set_progress_callback(NULL,NULL);
	slv_set_solver_interrupt(0);
	if(sys)system_destroy(sys);
	system_free_reused_mem();
	solver_destroy_engines();
	if(sim)sim_destroy(sim);
	Asc_CompilerDestroy();
	if(skip){CU_SKIP("Gurobi plugin is not built");}
}
#define CASE(NAME,MODEL,MODE,OBJ) static void test_##NAME(void){run_case("models/test/gurobi/lp.a4c",MODEL,MODE,OBJ);}
CASE(offset,"gurobi_offset",EXACT,17)
CASE(scaling,"gurobi_offset",SCALED,17)
CASE(resolve,"gurobi_offset",RESOLVE,17)
CASE(infeasible,"gurobi_infeasible",INFEASIBLE,0)
CASE(unbounded,"gurobi_unbounded",UNBOUNDED,3)
CASE(quadratic_rejected,"gurobi_quadratic",REJECT,0)
CASE(cubic_rejected,"gurobi_cubic",REJECT,0)
CASE(quadratic_objective_rejected,"gurobi_quadratic_objective",REJECT,0)
CASE(integer_rejected,"gurobi_integer",REJECT,0)
CASE(binary_rejected,"gurobi_binary",REJECT,0)
CASE(integer_relaxed,"gurobi_integer",RELAX,2.5)
CASE(binary_relaxed,"gurobi_binary",RELAX,0.5)
CASE(tangent,"gurobi_quadratic",TANGENT,0)
CASE(interrupt,"gurobi_offset",INTERRUPT,1)
CASE(objective_only_variable,"gurobi_objective_only_variable",EXACT,11)
CASE(bad_evaluation,"gurobi_bad_evaluation",BAD_EVAL,0)
CASE(shared_export,"gurobi_offset",PREPARE_ONLY,0)
static void test_lp1(void){run_case("models/test/ipopt/lp1.a4c","lp1",EXACT,-10);}
static void test_time_limit(void){run_case("models/test/ipopt/lp1.a4c","lp1",TIMEOUT,0.5);}
static void test_afiro(void){run_case("models/test/highs/afiro.a4c","afiro",EXACT,-464.753142857);}
static void test_alloy_blending(void){run_case("models/alloy_blending.a4c","alloy_blending",SHOWCASE,4.98);}
static void test_alloy_blending_mass_balance(void){run_case("models/alloy_blending.a4c","alloy_blending",SHOWCASE_MB,4.98);}
static void test_alloy_blending_ten_pounds(void){run_case("models/alloy_blending_detailed.a4c","alloy_blending_detailed",SHOWCASE_10LB,49.8);}
static void test_alloy_blending_detailed(void){run_case("models/alloy_blending_detailed.a4c","alloy_blending_detailed",SHOWCASE,4.98);}
static void test_alloy_blending_detailed_mass_balance(void){run_case("models/alloy_blending_detailed.a4c","alloy_blending_detailed",SHOWCASE_MB,4.98);}
static void test_steel_production(void){run_case("models/steel_production.a4c","steel_production",SELF_TEST,515033);}
#define TESTS(T) \
	T(lp1) T(offset) T(scaling) T(resolve) T(afiro) \
	T(infeasible) T(unbounded) T(time_limit) T(interrupt) \
	T(quadratic_rejected) T(cubic_rejected) T(quadratic_objective_rejected) \
	T(integer_rejected) T(binary_rejected) T(integer_relaxed) T(binary_relaxed) T(tangent) \
	T(objective_only_variable) T(bad_evaluation) T(shared_export) \
	T(alloy_blending) T(alloy_blending_mass_balance) T(alloy_blending_ten_pounds) \
	T(alloy_blending_detailed) T(alloy_blending_detailed_mass_balance) T(steel_production)
REGISTER_TESTS_SIMPLE(solver_gurobi,TESTS)
