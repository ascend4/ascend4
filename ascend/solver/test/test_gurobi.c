/* LP/MIP adapter regression tests. Built plugins are tested, including licensed
 * solves. ASCEND_TEST_GUROBI=1 additionally makes a missing plugin an error. */
#include <stdlib.h>
#include <string.h>
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
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/packages.h>
#include <ascend/system/system.h>
#include <ascend/system/lp_utils.h>
#include <ascend/solver/solver.h>
#include <test/common.h>

enum { EXACT, REJECT, SCALED, RESOLVE, RELAX, TANGENT, INFEASIBLE, UNBOUNDED, TIMEOUT, INTERRUPT, BAD_EVAL, PREPARE_ONLY,
	SHOWCASE, SHOWCASE_MB, SHOWCASE_10LB, SELF_TEST, PARAMETERS, LP_TUNED, LP_ITER_LIMIT, LP_BAR_LIMIT,
	MIP, MIP_SCALED, MIP_BENCH, MIP_RESOLVE, MIP_INFEASIBLE, MIP_TIMEOUT,
	MIP_INTERRUPT, MIP_QUIET_INTERRUPT, MIP_SOLUTION_LIMIT, MIP_CALLBACK_INTERRUPT,
	MIP_QUIET, MIP_PREPARE, MIP_NODE_LIMIT, MIP_TANGENT, MIP_TUNED, MIP_WORK_LIMIT, MIP_MEM_LIMIT, MIP_SELF_TEST };

/* Defaults/ranges deliberately checked independently of the adapter mapping. */
static const struct {
	const char *name;
	enum parm_type type;
	double value, low, high;
} tuning_parameters[]={
	{"mip_focus",int_parm,0,0,3},
	{"heuristics",real_parm,0.05,0,1},
	{"cuts",int_parm,-1,-1,3},
	{"symmetry",int_parm,-1,-1,2},
	{"integrality_focus",bool_parm,0,0,1},
	{"numeric_focus",int_parm,0,0,3},
	{"scale_flag",int_parm,-1,-1,3},
	{"aggregate",int_parm,1,0,2},
	{"work_limit",real_parm,1e100,0,1e100},
	{"soft_mem_limit",real_parm,1e100,0,1e100},
	{"iteration_limit",real_parm,1e100,0,1e100},
	{"bar_iter_limit",int_parm,1000,0,2000000000},
	{"bar_conv_tol",real_parm,1e-8,0,1},
	{"crossover",int_parm,-1,-1,4},
	{"node_method",int_parm,-1,-1,2}
};

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
static int interrupt_mip_progress(const char *solver,const char *message,void *data){
	int *seen=data;
	CU_ASSERT_STRING_EQUAL(solver,"Gurobi");
	if(strstr(message,"mip_nodes=")){
		++*seen;
		slv_set_solver_interrupt(1);
	}
	return 0;
}
static int unexpected_progress(const char *solver,const char *message,void *data){
	(void)solver; (void)message; (void)data;
	CU_FAIL("Progress disabled but callback invoked");
	return 0;
}

static void run_case(const char *path,const char *model,int mode,double expected){
	struct Instance *sim=NULL;
	slv_system_t sys=NULL;
	slv_parameters_t p;
	slv_status_t status;
	const slv_status_lp_t *lp;
	double *original_lower=NULL, *original_upper=NULL;
	struct var_variable **vars;
	struct Name *name;
	int opened, selected, licensed=getenv("ASCEND_TEST_GUROBI") && !strcmp(getenv("ASCEND_TEST_GUROBI"),"1");
	int skip=0, seen=0, nvars=0, i;
	int is_mip=mode>=MIP;
	int showcase=mode==SHOWCASE || mode==SHOWCASE_MB || mode==SHOWCASE_10LB;
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
	/* SELF_TEST examples need no setup; solver/OPTION hooks from on_load are
	 * supplied by this harness instead (including a zero relative MIP gap). */
	if(mode!=SELF_TEST && mode!=MIP_SELF_TEST){
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
	vars=slv_get_solvers_var_list(sys);
	while(vars[nvars])++nvars;
	original_lower=ASC_NEW_ARRAY(double,nvars);
	original_upper=ASC_NEW_ARRAY(double,nvars);
	for(i=0;i<nvars;++i){
		original_lower[i]=var_lower_bound(vars[i]);
		original_upper[i]=var_upper_bound(vars[i]);
	}
	CU_ASSERT(slv_select_solver(sys,selected)>=0);
	slv_get_parameters(sys,&p);
	CU_ASSERT(p.whose==selected);
	if(mode==PARAMETERS){
		for(i=0;i<(int)(sizeof(tuning_parameters)/sizeof(tuning_parameters[0]));++i){
			int idx=param(&p,tuning_parameters[i].name), j;
			struct slv_parameter original=p.parms[idx], *q=&p.parms[idx];
			CU_ASSERT(q->type==tuning_parameters[i].type);
			CU_ASSERT(q->description && q->description[0]);
			if(q->type==real_parm){
				CU_ASSERT_DOUBLE_EQUAL(q->info.r.value,tuning_parameters[i].value,0);
				CU_ASSERT_DOUBLE_EQUAL(q->info.r.low,tuning_parameters[i].low,0);
				CU_ASSERT_DOUBLE_EQUAL(q->info.r.high,tuning_parameters[i].high,0);
			}else if(q->type==int_parm){
				CU_ASSERT(q->info.i.value==tuning_parameters[i].value);
				CU_ASSERT(q->info.i.low==tuning_parameters[i].low);
				CU_ASSERT(q->info.i.high==tuning_parameters[i].high);
			}else CU_ASSERT(q->info.b.value==0);
			/* C clients can bypass frontend validation: test both ends, NaN
			 * and IEEE infinity, without ever acquiring a license. */
			for(j=0;j<(q->type==real_parm ? 4 : 2);++j){
				if(q->type==real_parm){
					q->info.r.value=j==0 ? q->info.r.low-1 : j==1 ? q->info.r.high*2+1 : j==2 ? NAN : HUGE_VAL;
				}else if(q->type==int_parm)q->info.i.value=j==0 ? q->info.i.low-1 : q->info.i.high+1;
				else q->info.b.value=j==0 ? -1 : 2;
				slv_set_parameters(sys,&p);
				CU_ASSERT(slv_presolve(sys)!=0);
				slv_get_status(sys,&status);
				CU_ASSERT_FALSE(status.calc_ok);
				CU_ASSERT_FALSE(status.ready_to_solve);
			}
			*q=original;
		}
		slv_set_parameters(sys,&p);
		CU_ASSERT(slv_presolve(sys)==0);
		goto cleanup;
	}
	SLV_PARAM_INT(&p,param(&p,"threads"))=1;
	SLV_PARAM_INT(&p,param(&p,"random_seed"))=42;
	SLV_PARAM_REAL(&p,param(&p,"mip_rel_gap"))=0;
	SLV_PARAM_REAL(&p,param(&p,"mip_abs_gap"))=0;
	SLV_PARAM_REAL(&p,param(&p,"integrality_tol"))=1e-7;
	if(mode==RELAX)SLV_PARAM_BOOL(&p,param(&p,"relaxed"))=1;
	if(mode==TANGENT || mode==MIP_TANGENT)SLV_PARAM_BOOL(&p,param(&p,"nonlin"))=1;
	if(mode==TIMEOUT || mode==MIP_TIMEOUT)SLV_PARAM_REAL(&p,param(&p,"time_limit"))=0;
	if(mode==INFEASIBLE || mode==MIP_INFEASIBLE)SLV_PARAM_BOOL(&p,param(&p,"dual_reductions"))=0;
	if(mode==MIP_SOLUTION_LIMIT)SLV_PARAM_INT(&p,param(&p,"solution_limit"))=1;
	if(mode==MIP_CALLBACK_INTERRUPT)SLV_PARAM_INT(&p,param(&p,"presolve"))=0;
	if(mode==MIP_NODE_LIMIT){
		SLV_PARAM_REAL(&p,param(&p,"node_limit"))=0;
		SLV_PARAM_INT(&p,param(&p,"presolve"))=0;
	}
	if(mode==MIP_TUNED || mode==LP_TUNED){
		SLV_PARAM_INT(&p,param(&p,"mip_focus"))=1;
		SLV_PARAM_REAL(&p,param(&p,"heuristics"))=0.2;
		SLV_PARAM_INT(&p,param(&p,"cuts"))=2;
		SLV_PARAM_INT(&p,param(&p,"symmetry"))=2;
		SLV_PARAM_BOOL(&p,param(&p,"integrality_focus"))=1;
		SLV_PARAM_INT(&p,param(&p,"numeric_focus"))=2;
		SLV_PARAM_INT(&p,param(&p,"scale_flag"))=2;
		SLV_PARAM_INT(&p,param(&p,"aggregate"))=0;
		SLV_PARAM_REAL(&p,param(&p,"work_limit"))=100;
		SLV_PARAM_REAL(&p,param(&p,"soft_mem_limit"))=1;
		SLV_PARAM_REAL(&p,param(&p,"iteration_limit"))=100000;
		SLV_PARAM_INT(&p,param(&p,"bar_iter_limit"))=500;
		SLV_PARAM_REAL(&p,param(&p,"bar_conv_tol"))=1e-10;
		SLV_PARAM_INT(&p,param(&p,"crossover"))=mode==LP_TUNED ? 0 : 1;
		SLV_PARAM_INT(&p,param(&p,"node_method"))=1;
		SLV_PARAM_INT(&p,param(&p,"method"))=mode==LP_TUNED ? 2 : 1;
		SLV_PARAM_INT(&p,param(&p,"presolve"))=0;
	}
	if(mode==LP_ITER_LIMIT || mode==LP_BAR_LIMIT){
		SLV_PARAM_INT(&p,param(&p,"presolve"))=0;
		SLV_PARAM_INT(&p,param(&p,"method"))=mode==LP_ITER_LIMIT ? 1 : 2;
		if(mode==LP_ITER_LIMIT)SLV_PARAM_REAL(&p,param(&p,"iteration_limit"))=0;
		else{
			SLV_PARAM_INT(&p,param(&p,"bar_iter_limit"))=0;
			/* Otherwise crossover can finish the LP despite the barrier limit. */
			SLV_PARAM_INT(&p,param(&p,"crossover"))=0;
		}
	}
	if(mode==MIP_WORK_LIMIT)SLV_PARAM_REAL(&p,param(&p,"work_limit"))=0;
	if(mode==MIP_MEM_LIMIT)SLV_PARAM_REAL(&p,param(&p,"soft_mem_limit"))=0;
	if(mode==MIP_QUIET || mode==MIP_QUIET_INTERRUPT)SLV_PARAM_BOOL(&p,param(&p,"progress_callbacks"))=0;
	if(mode==SCALED || mode==MIP_SCALED || mode==RELAX){
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
	slv_get_status(sys,&status);
	CU_ASSERT(status.kind==(is_mip ? SLV_STATUS_MIP : SLV_STATUS_LP));
	if(mode==MIP_PREPARE)goto cleanup;
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
	if(mode==INTERRUPT || mode==MIP_INTERRUPT)slv_set_progress_callback(interrupt_progress,NULL);
	if(mode==MIP_CALLBACK_INTERRUPT)slv_set_progress_callback(interrupt_mip_progress,&seen);
	if(mode==MIP_QUIET || mode==MIP_QUIET_INTERRUPT)slv_set_progress_callback(unexpected_progress,NULL);
	if(mode==MIP_QUIET_INTERRUPT)slv_set_solver_interrupt(1);
	CU_ASSERT(slv_iterate(sys)==0);
	for(i=0;i<nvars;++i){
		CU_ASSERT_DOUBLE_EQUAL(var_lower_bound(vars[i]),original_lower[i],0);
		CU_ASSERT_DOUBLE_EQUAL(var_upper_bound(vars[i]),original_upper[i],0);
	}
	slv_get_status(sys,&status);
	CU_ASSERT(status.kind==(is_mip ? SLV_STATUS_MIP : SLV_STATUS_LP));
	lp=is_mip ? &status.u.mip.lp : &status.u.lp;
	if(mode==LP_ITER_LIMIT || mode==LP_BAR_LIMIT || mode==MIP_WORK_LIMIT || mode==MIP_MEM_LIMIT){
		CU_ASSERT_FALSE(status.converged);
		CU_ASSERT_FALSE(status.ready_to_solve);
		CU_ASSERT_FALSE(status.inconsistent);
		CU_ASSERT_FALSE(status.panic);
		CU_ASSERT_FALSE(status.time_limit_exceeded);
		CU_ASSERT(lp->have_model_status);
		CU_ASSERT(lp->model_status==(mode==MIP_WORK_LIMIT ? 16 : mode==MIP_MEM_LIMIT ? 17 : 7));
		CU_ASSERT(status.iteration_limit_exceeded==(mode!=MIP_MEM_LIMIT));
		goto cleanup;
	}
	if(mode==MIP_SOLUTION_LIMIT || mode==MIP_CALLBACK_INTERRUPT || mode==MIP_NODE_LIMIT){
		CU_ASSERT_FALSE(status.converged);
		if(mode==MIP_SOLUTION_LIMIT){
			CU_ASSERT(lp->model_status==10); /* GRB_SOLUTION_LIMIT */
			CU_ASSERT_TRUE(status.iteration_limit_exceeded);
			CU_ASSERT(status.u.mip.have_solution_count && status.u.mip.solution_count>=1);
			CU_ASSERT(lp->primal_status==SLV_SOLUTION_STATUS_FEASIBLE);
		}else if(mode==MIP_NODE_LIMIT){
			CU_ASSERT(lp->model_status==8); /* GRB_NODE_LIMIT */
			CU_ASSERT_TRUE(status.iteration_limit_exceeded);
		}else{
			CU_ASSERT(seen>0);
			CU_ASSERT_TRUE(status.panic);
		}
		if(lp->primal_status==SLV_SOLUTION_STATUS_FEASIBLE){
			CU_ASSERT_DOUBLE_EQUAL(rel_residual(slv_get_obj_relation(sys)),lp->objective_value,1e-6);
			CU_ASSERT_TRUE(status.u.mip.have_primal_bound);
			/* This is a minimisation benchmark with known optimum 166. */
			CU_ASSERT(status.u.mip.primal_bound>=166-1e-6);
			if(status.u.mip.have_dual_bound){
				CU_ASSERT(status.u.mip.dual_bound<=166+1e-6);
				CU_ASSERT(status.u.mip.dual_bound<=status.u.mip.primal_bound+1e-6);
			}
			for(i=0;i<nvars;++i){
				struct TypeDescription *type=InstanceTypeDesc(var_instance(vars[i]));
				if(type==MoreRefined(type,FindType(AddSymbol("solver_int")))){
					CU_ASSERT_DOUBLE_EQUAL(var_value(vars[i]),round(var_value(vars[i])),1e-7);
				}
			}
		}
		goto cleanup;
	}
	if(mode==INFEASIBLE || mode==UNBOUNDED || mode==TIMEOUT || mode==INTERRUPT
		|| mode==MIP_INFEASIBLE || mode==MIP_TIMEOUT || mode==MIP_INTERRUPT || mode==MIP_QUIET_INTERRUPT){
		CU_ASSERT_FALSE(status.converged);
		if(mode==INFEASIBLE || mode==MIP_INFEASIBLE)CU_ASSERT_TRUE(status.inconsistent);
		if(mode==UNBOUNDED)CU_ASSERT_TRUE(status.diverged);
		if(mode==TIMEOUT || mode==MIP_TIMEOUT)CU_ASSERT_TRUE(status.time_limit_exceeded);
		if(mode==INTERRUPT || mode==MIP_INTERRUPT || mode==MIP_QUIET_INTERRUPT)CU_ASSERT_TRUE(status.panic);
		CU_ASSERT_FALSE(lp->have_objective);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"x")),expected,1e-8);
		goto cleanup;
	}
	CU_ASSERT_TRUE(status.converged);
	CU_ASSERT_TRUE(lp->have_objective);
	CU_ASSERT_DOUBLE_EQUAL(lp->objective_value,expected,1e-6);
	if(mode==LP_TUNED){
		CU_ASSERT(lp->have_ipm_iterations && lp->ipm_iterations>0);
	}
	if(is_mip){
		const slv_status_mip_t *m=&status.u.mip;
		struct var_variable **v=slv_get_solvers_var_list(sys);
		CU_ASSERT_TRUE(m->have_primal_bound);
		CU_ASSERT_TRUE(m->have_dual_bound);
		CU_ASSERT_TRUE(m->have_gap);
		CU_ASSERT_TRUE(m->have_abs_gap);
		CU_ASSERT_TRUE(m->have_node_count);
		CU_ASSERT(m->have_solution_count && m->solution_count>=1);
		CU_ASSERT_DOUBLE_EQUAL(m->primal_bound,expected,1e-6);
		CU_ASSERT_DOUBLE_EQUAL(m->dual_bound,expected,1e-6);
		CU_ASSERT_DOUBLE_EQUAL(m->gap,0,1e-7);
		CU_ASSERT_DOUBLE_EQUAL(m->abs_gap,0,1e-6);
		for(;*v;++v){
			struct TypeDescription *type=InstanceTypeDesc(var_instance(*v));
			if(type==MoreRefined(type,FindType(AddSymbol("solver_int"))) && !var_relaxed(*v) && !var_fixed(*v)){
				CU_ASSERT_DOUBLE_EQUAL(var_value(*v),round(var_value(*v)),1e-7);
			}
		}
	}
	CU_ASSERT_DOUBLE_EQUAL(rel_residual(slv_get_obj_relation(sys)),expected,1e-6);
	if(showcase || mode==SELF_TEST || mode==MIP_BENCH || mode==MIP_SELF_TEST){
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
	if(mode==MIP_RESOLVE){
		var_set_upper_bound(variable(sys,"x"),1.5);
		CU_ASSERT(slv_resolve(sys)==0);
		slv_get_status(sys,&status);
		CU_ASSERT_TRUE(status.converged);
		CU_ASSERT(status.kind==SLV_STATUS_MIP);
		CU_ASSERT_DOUBLE_EQUAL(status.u.mip.lp.objective_value,1,1e-7);
		CU_ASSERT_DOUBLE_EQUAL(var_value(variable(sys,"x")),1,1e-7);
		/* Rebuild and rescale after switching from MIP to LP relaxation. */
		slv_get_parameters(sys,&p);
		SLV_PARAM_BOOL(&p,param(&p,"relaxed"))=1;
		SLV_PARAM_BOOL(&p,param(&p,"varnom_scale"))=1;
		slv_set_parameters(sys,&p);
		CU_ASSERT(slv_resolve(sys)==0);
		slv_get_status(sys,&status);
		CU_ASSERT_TRUE(status.converged);
		CU_ASSERT(status.kind==SLV_STATUS_LP);
		CU_ASSERT_DOUBLE_EQUAL(status.u.lp.objective_value,1.5,1e-7);
	}
cleanup:
	ASC_FREE(original_lower); ASC_FREE(original_upper);
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
CASE(integer,"gurobi_integer",MIP,2)
CASE(binary,"gurobi_binary",MIP,0)
CASE(integer_scaled,"gurobi_integer",MIP_SCALED,2)
CASE(binary_scaled,"gurobi_binary",MIP_SCALED,0)
CASE(mip_resolve,"gurobi_integer",MIP_RESOLVE,2)
CASE(mip_timeout,"gurobi_integer",MIP_TIMEOUT,0)
CASE(mip_interrupt,"gurobi_integer",MIP_INTERRUPT,0)
CASE(mip_quiet_interrupt,"gurobi_integer",MIP_QUIET_INTERRUPT,0)
CASE(mip_quiet,"gurobi_integer",MIP_QUIET,2)
CASE(mip_presolve,"gurobi_integer",MIP_PREPARE,0)
CASE(integer_relaxed,"gurobi_integer",RELAX,2.5)
CASE(binary_relaxed,"gurobi_binary",RELAX,0.5)
CASE(tangent,"gurobi_quadratic",TANGENT,0)
CASE(interrupt,"gurobi_offset",INTERRUPT,1)
CASE(objective_only_variable,"gurobi_objective_only_variable",EXACT,11)
CASE(bad_evaluation,"gurobi_bad_evaluation",BAD_EVAL,0)
CASE(shared_export,"gurobi_offset",PREPARE_ONLY,0)
CASE(parameters,"gurobi_offset",PARAMETERS,0)
CASE(lp_tuned,"gurobi_offset",LP_TUNED,17)
CASE(lp_iteration_limit,"gurobi_offset",LP_ITER_LIMIT,0)
CASE(lp_barrier_limit,"gurobi_offset",LP_BAR_LIMIT,0)
CASE(mip_work_limit,"gurobi_integer",MIP_WORK_LIMIT,0)
CASE(mip_memory_limit,"gurobi_integer",MIP_MEM_LIMIT,0)
static void test_lp1(void){run_case("models/test/ipopt/lp1.a4c","lp1",EXACT,-10);}
static void test_time_limit(void){run_case("models/test/ipopt/lp1.a4c","lp1",TIMEOUT,0.5);}
static void test_afiro(void){run_case("models/test/highs/afiro.a4c","afiro",EXACT,-464.753142857);}
static void test_alloy_blending(void){run_case("models/alloy_blending.a4c","alloy_blending",SHOWCASE,4.98);}
static void test_alloy_blending_mass_balance(void){run_case("models/alloy_blending.a4c","alloy_blending",SHOWCASE_MB,4.98);}
static void test_alloy_blending_ten_pounds(void){run_case("models/alloy_blending_detailed.a4c","alloy_blending_detailed",SHOWCASE_10LB,49.8);}
static void test_alloy_blending_detailed(void){run_case("models/alloy_blending_detailed.a4c","alloy_blending_detailed",SHOWCASE,4.98);}
static void test_alloy_blending_detailed_mass_balance(void){run_case("models/alloy_blending_detailed.a4c","alloy_blending_detailed",SHOWCASE_MB,4.98);}
static void test_steel_production(void){run_case("models/steel_production.a4c","steel_production",SELF_TEST,515033);}
/* ASCEND's monetary flow dimension is stored in USD/s, not USD/day. */
static void test_refinery(void){run_case("models/refinery.a4c","refinery",SELF_TEST,6588.421476681333/86400);}
static void test_refinery_low_sulfur(void){run_case("models/refinery.a4c","refinery_low_sulfur",SELF_TEST,0);}
static void test_mip_facility(void){run_case("models/test/mip/facility_location.a4c","mip_facility_location",MIP_BENCH,470);}
static void test_mip_facility_labels(void){run_case("models/test/mip/facility_location_table_labels.a4c","mip_facility_location_table_labels",MIP_BENCH,470);}
static void test_mip_tsp(void){run_case("models/test/mip/tsp_mtz8.a4c","mip_tsp_mtz8",MIP_BENCH,166);}
static void test_mip_tsp_labels(void){run_case("models/test/mip/tsp_mtz8_table_labels.a4c","mip_tsp_mtz8_table_labels",MIP_BENCH,166);}
static void test_mip_solution_limit(void){run_case("models/test/mip/tsp_mtz8.a4c","mip_tsp_mtz8",MIP_SOLUTION_LIMIT,0);}
static void test_mip_callback_interrupt(void){run_case("models/test/mip/tsp_mtz8.a4c","mip_tsp_mtz8",MIP_CALLBACK_INTERRUPT,0);}
static void test_mip_node_limit(void){run_case("models/test/mip/tsp_mtz8.a4c","mip_tsp_mtz8",MIP_NODE_LIMIT,0);}
static void test_mip_tuned(void){run_case("models/test/mip/tsp_mtz8.a4c","mip_tsp_mtz8",MIP_TUNED,166);}
static void test_food_manufacture_2(void){run_case("models/food_manufacture_2.a4c","food_manufacture_2",MIP_SELF_TEST,100278.7037037037);}
static void test_job_shop(void){run_case("models/job_shop.a4c","job_shop",MIP_SELF_TEST,97*60);}
static void test_electrical_power_1(void){run_case("models/electrical_power_1.a4c","electrical_power_1",MIP_SELF_TEST,1002540);}
static void test_uc_units(void){run_case("models/test/mip/unit_commitment.a4c","uc_units",MIP_SELF_TEST,227.5);}
static void test_uc_initial(void){run_case("models/test/mip/unit_commitment.a4c","uc_initial",MIP_SELF_TEST,127.5);}
#define DOMAIN_CASE(NAME,MODEL,MODE,OBJ) static void test_##NAME(void){run_case("models/test/mip/domains.a4c",MODEL,MODE,OBJ);}
DOMAIN_CASE(semi_zero,"mip_semi_gap",MIP_SCALED,0)
DOMAIN_CASE(semi_active,"mip_semi_active",MIP_SCALED,2)
DOMAIN_CASE(semi_relaxed,"mip_semi_gap",RELAX,1)
DOMAIN_CASE(semi_individually_relaxed,"mip_semi_individually_relaxed",EXACT,1)
DOMAIN_CASE(integer_individually_relaxed,"mip_integer_individually_relaxed",EXACT,2.5)
DOMAIN_CASE(mip_infeasible,"mip_integer_infeasible",MIP_INFEASIBLE,3)
DOMAIN_CASE(mip_domains,"mip_domains",MIP_SCALED,-1)
DOMAIN_CASE(mip_domains_relaxed,"mip_domains",RELAX,0)
DOMAIN_CASE(mip_domains_partially_relaxed,"mip_domains_partially_relaxed",MIP_SCALED,-0.5)
DOMAIN_CASE(mip_quadratic_rejected,"mip_quadratic",REJECT,0)
DOMAIN_CASE(mip_tangent,"mip_quadratic",MIP_TANGENT,2)
DOMAIN_CASE(binary_wide_bounds,"mip_binary_wide_bounds",MIP,1)
DOMAIN_CASE(binary_wide_bounds_relaxed,"mip_binary_wide_bounds",RELAX,1)
#define TESTS(T) \
	T(lp1) T(offset) T(scaling) T(resolve) T(afiro) \
	T(infeasible) T(unbounded) T(time_limit) T(interrupt) \
	T(quadratic_rejected) T(cubic_rejected) T(quadratic_objective_rejected) \
	T(integer) T(binary) T(integer_scaled) T(binary_scaled) T(integer_relaxed) T(binary_relaxed) T(tangent) \
	T(mip_resolve) T(mip_timeout) T(mip_interrupt) T(mip_quiet_interrupt) T(mip_quiet) T(mip_presolve) \
	T(mip_facility) T(mip_facility_labels) T(mip_tsp) T(mip_tsp_labels) T(mip_solution_limit) T(mip_callback_interrupt) \
	T(mip_node_limit) \
	T(parameters) T(lp_tuned) T(mip_tuned) T(lp_iteration_limit) T(lp_barrier_limit) T(mip_work_limit) T(mip_memory_limit) \
	T(semi_zero) T(semi_active) T(semi_relaxed) T(semi_individually_relaxed) T(integer_individually_relaxed) \
	T(mip_infeasible) T(mip_domains) T(mip_domains_relaxed) T(binary_wide_bounds) T(binary_wide_bounds_relaxed) \
	T(mip_domains_partially_relaxed) T(mip_quadratic_rejected) T(mip_tangent) \
	T(objective_only_variable) T(bad_evaluation) T(shared_export) \
	T(alloy_blending) T(alloy_blending_mass_balance) T(alloy_blending_ten_pounds) \
	T(alloy_blending_detailed) T(alloy_blending_detailed_mass_balance) T(steel_production) \
	T(refinery) T(refinery_low_sulfur) T(food_manufacture_2) T(job_shop) \
	T(electrical_power_1) T(uc_units) T(uc_initial)
REGISTER_TESTS_SIMPLE(solver_gurobi,TESTS)
