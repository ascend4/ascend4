/* Gurobi linear LP/MIP adapter for ASCEND.
 * Copyright (C) 2026 John Pye
 * GNU GPL version 2 or (at your option) any later version.
 */
#define ASC_BUILDING_INTERFACE
#include <ascend/solver/solver.h>
#include <ascend/system/lp_utils.h>
#include <ascend/system/relman.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>
#include <gurobi_c.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

enum { NONLIN, RELAXED, SCALE_VAR, SCALE_REL, TIME_LIMIT, THREADS, METHOD,
	PRESOLVE, FEAS_TOL, OPT_TOL, PROGRESS, DUAL_REDUCTIONS,
	MIP_REL_GAP, MIP_ABS_GAP, INT_TOL, NODE_LIMIT, SOLUTION_LIMIT, RANDOM_SEED,
	MIP_FOCUS, HEURISTICS, CUTS, SYMMETRY, INTEGRALITY_FOCUS, NUMERIC_FOCUS,
	NATIVE_SCALE, AGGREGATE, WORK_LIMIT, SOFT_MEM_LIMIT, ITERATION_LIMIT,
	BAR_ITER_LIMIT, BAR_CONV_TOL, CROSSOVER, NODE_METHOD, PARAM_COUNT };
#define GUROBI_SOLVER_NUMBER 63
struct gurobi_system {
	slv_system_t server;
	slv_parameters_t params;
	slv_status_t status;
	mps_data_t mps;
	lp_sparse_t lp;
	GRBenv *env;
	GRBmodel *model;
	double next_progress;
	int prepared;
};
typedef struct gurobi_system GurobiSystem;

/* Shared by solve and the explicit license-details probe. In particular, do
 * not use GRBloadenv: it can print license credentials before logging is off.
 */
static int gurobi_open_environment(GRBenv **env){
	int err=GRBemptyenv(env);
	if(!err)err=GRBsetintparam(*env,GRB_INT_PAR_OUTPUTFLAG,0);
	if(!err)err=GRBstartenv(*env);
	if(err && *env){GRBfreeenv(*env);*env=NULL;}
	return err;
}

static int32 gurobi_defaults(slv_system_t server, SlvClientToken token, slv_parameters_t *p){
	(void)server; (void)token;
	if(!p->parms){
		p->parms=ASC_NEW_ARRAY_CLEAR(struct slv_parameter,PARAM_COUNT);
		if(!p->parms)return -1;
		p->dynamic_parms=1;
	}
	p->num_parms=0;
	p->whose=GUROBI_SOLVER_NUMBER;
#define BOOL(I,N,L,D,V) slv_param_bool(p,I,(SlvParameterInitBool){{N,L,1,D},V})
#define INT(I,N,L,D,V,LO,HI) slv_param_int(p,I,(SlvParameterInitInt){{N,L,2,D},V,LO,HI})
#define REAL(I,N,L,D,V,LO,HI) slv_param_real(p,I,(SlvParameterInitReal){{N,L,2,D},V,LO,HI})
	BOOL(NONLIN,"nonlin","Linearise nonlinear expressions?","Solve one tangent LP/MIP at the current point. This does not solve the original nonlinear model.",FALSE);
	BOOL(RELAXED,"relaxed","Solve continuous relaxation?","Relax all discrete solver variables, including the zero-or-interval semicontinuous domain.",FALSE);
	BOOL(SCALE_VAR,"varnom_scale","Scale variables?","Scale continuous variables using ASCEND nominal values.",FALSE);
	BOOL(SCALE_REL,"relnom_scale","Scale relations?","Scale rows using ASCEND nominal values and coefficient magnitudes.",FALSE);
	BOOL(PROGRESS,"progress_callbacks","Progress callbacks?","Report LP/MIP progress through ASCEND. Interrupts are always checked.",TRUE);
	BOOL(DUAL_REDUCTIONS,"dual_reductions","Dual reductions?","Disable to distinguish infeasible from unbounded when presolve reports INF_OR_UNBD.",TRUE);
	REAL(TIME_LIMIT,"time_limit","Time limit (s)","Gurobi optimization time limit.",GRB_INFINITY,0,GRB_INFINITY);
	INT(THREADS,"threads","Threads","Zero lets Gurobi choose the number of threads.",0,0,1024);
	INT(METHOD,"method","LP/root algorithm","LP or MIP root relaxation: -1 automatic, 0 primal simplex, 1 dual simplex, 2 barrier, 3 concurrent, 4 deterministic concurrent.",-1,-1,4);
	INT(PRESOLVE,"presolve","Presolve","-1 automatic, 0 off, 1 conservative, 2 aggressive.",-1,-1,2);
	REAL(FEAS_TOL,"feasibility_tol","Feasibility tolerance","Primal feasibility tolerance.",1e-6,1e-9,1e-2);
	REAL(OPT_TOL,"optimality_tol","Optimality tolerance","Dual feasibility tolerance.",1e-6,1e-9,1e-2);
	REAL(MIP_REL_GAP,"mip_rel_gap","MIP relative gap","Relative optimality gap target for MIP.",1e-4,0,GRB_INFINITY);
	REAL(MIP_ABS_GAP,"mip_abs_gap","MIP absolute gap","Absolute optimality gap target, in the objective's ASCEND internal units.",1e-6,0,GRB_INFINITY);
	REAL(INT_TOL,"integrality_tol","Integrality tolerance","Integer and semicontinuous feasibility tolerance.",1e-5,1e-9,1e-1);
	REAL(NODE_LIMIT,"node_limit","MIP node limit","Maximum branch-and-bound nodes; infinity means unlimited.",GRB_INFINITY,0,GRB_INFINITY);
	INT(SOLUTION_LIMIT,"solution_limit","MIP solution limit","Stop after this many feasible solutions have been found.",2000000000,1,2000000000);
	INT(RANDOM_SEED,"random_seed","Random seed","Gurobi random seed.",0,0,2000000000);
	INT(MIP_FOCUS,"mip_focus","MIP search focus","0 balanced, 1 find feasible solutions, 2 prove optimality, 3 improve the objective bound.",0,0,3);
	REAL(HEURISTICS,"heuristics","MIP heuristic effort","Target fraction of effort devoted to finding feasible solutions; 0 disables these heuristics.",0.05,0,1);
	INT(CUTS,"cuts","MIP cut aggressiveness","-1 automatic, 0 off, 1 conservative, 2 aggressive, 3 very aggressive.",-1,-1,3);
	INT(SYMMETRY,"symmetry","MIP symmetry detection","-1 automatic, 0 off, 1 conservative, 2 aggressive.",-1,-1,2);
	BOOL(INTEGRALITY_FOCUS,"integrality_focus","Stricter integrality handling?","Reduce effects of small integrality violations, particularly in big-M formulations; may increase solve time.",FALSE);
	INT(NUMERIC_FOCUS,"numeric_focus","Numerical care","0 automatic; 1 through 3 spend increasing effort handling numerical difficulties.",0,0,3);
	INT(NATIVE_SCALE,"scale_flag","Gurobi scaling","-1 automatic, 0 off, 1-3 alternative strategies (2 geometric mean). Independent of ASCEND nominal scaling.",-1,-1,3);
	INT(AGGREGATE,"aggregate","Presolve aggregation","0 off, 1 moderate, 2 aggressive. Disabling aggregation can help numerically difficult models.",1,0,2);
	REAL(WORK_LIMIT,"work_limit","Work limit","Deterministic work-unit budget, not seconds. Repeatability requires the same model, hardware and settings.",GRB_INFINITY,0,GRB_INFINITY);
	REAL(SOFT_MEM_LIMIT,"soft_mem_limit","Soft memory limit (GB)","Graceful solver memory limit in decimal GB, across the environment; may overshoot. Does not limit ASCEND's memory.",GRB_INFINITY,0,GRB_INFINITY);
	REAL(ITERATION_LIMIT,"iteration_limit","Simplex iteration limit","Maximum simplex iterations, including MIP node relaxations.",GRB_INFINITY,0,GRB_INFINITY);
	INT(BAR_ITER_LIMIT,"bar_iter_limit","Barrier iteration limit","Maximum barrier iterations.",1000,0,2000000000);
	REAL(BAR_CONV_TOL,"bar_conv_tol","Barrier convergence tolerance","Relative barrier convergence tolerance; distinct from primal/dual feasibility tolerances.",1e-8,0,1);
	INT(CROSSOVER,"crossover","Barrier crossover","-1 automatic, 0 disabled, 1/2 dual push with primal/dual cleanup, 3/4 primal push with primal/dual cleanup.",-1,-1,4);
	INT(NODE_METHOD,"node_method","MIP node algorithm","-1 automatic, 0 primal simplex, 1 dual simplex, 2 barrier. Method controls the root relaxation.",-1,-1,2);
#undef BOOL
#undef INT
#undef REAL
	return 0;
}

/* Only a curated, numeric algorithmic subset is exposed. In particular this
 * mapping must not include license credentials, remote hosts or logging paths.
 */
static const char *const gurobi_native_parameter[PARAM_COUNT]={
	[TIME_LIMIT]=GRB_DBL_PAR_TIMELIMIT, [THREADS]=GRB_INT_PAR_THREADS,
	[METHOD]=GRB_INT_PAR_METHOD, [PRESOLVE]=GRB_INT_PAR_PRESOLVE,
	[DUAL_REDUCTIONS]=GRB_INT_PAR_DUALREDUCTIONS,
	[FEAS_TOL]=GRB_DBL_PAR_FEASIBILITYTOL, [OPT_TOL]=GRB_DBL_PAR_OPTIMALITYTOL,
	[MIP_REL_GAP]=GRB_DBL_PAR_MIPGAP, [MIP_ABS_GAP]=GRB_DBL_PAR_MIPGAPABS,
	[INT_TOL]=GRB_DBL_PAR_INTFEASTOL, [NODE_LIMIT]=GRB_DBL_PAR_NODELIMIT,
	[SOLUTION_LIMIT]=GRB_INT_PAR_SOLUTIONLIMIT, [RANDOM_SEED]=GRB_INT_PAR_SEED,
	[MIP_FOCUS]=GRB_INT_PAR_MIPFOCUS, [HEURISTICS]=GRB_DBL_PAR_HEURISTICS,
	[CUTS]=GRB_INT_PAR_CUTS, [SYMMETRY]=GRB_INT_PAR_SYMMETRY,
	[INTEGRALITY_FOCUS]=GRB_INT_PAR_INTEGRALITYFOCUS, [NUMERIC_FOCUS]=GRB_INT_PAR_NUMERICFOCUS,
	[NATIVE_SCALE]=GRB_INT_PAR_SCALEFLAG, [AGGREGATE]=GRB_INT_PAR_AGGREGATE,
	[WORK_LIMIT]=GRB_DBL_PAR_WORKLIMIT, [SOFT_MEM_LIMIT]=GRB_DBL_PAR_SOFTMEMLIMIT,
	[ITERATION_LIMIT]=GRB_DBL_PAR_ITERATIONLIMIT, [BAR_ITER_LIMIT]=GRB_INT_PAR_BARITERLIMIT,
	[BAR_CONV_TOL]=GRB_DBL_PAR_BARCONVTOL, [CROSSOVER]=GRB_INT_PAR_CROSSOVER,
	[NODE_METHOD]=GRB_INT_PAR_NODEMETHOD
};

/* Direct C callers can bypass frontend range checks. Reject invalid values
 * before model assembly or opening a licensed environment (including NaN).
 */
static int gurobi_validate_options(const slv_parameters_t *p){
	int i;
	for(i=0;i<p->num_parms;++i){
		const struct slv_parameter *q=&p->parms[i];
		int valid=0;
		switch(q->type){
		case bool_parm: valid=q->info.b.value==0 || q->info.b.value==1; break;
		case int_parm: valid=q->info.i.value>=q->info.i.low && q->info.i.value<=q->info.i.high; break;
		case real_parm: valid=isfinite(q->info.r.value) && q->info.r.value>=q->info.r.low && q->info.r.value<=q->info.r.high; break;
		default: break;
		}
		if(!valid){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi parameter '%s' is outside its supported range.",q->name);
			return 1;
		}
	}
	return 0;
}

static int gurobi_apply_options(GRBenv *env, const slv_parameters_t *p){
	int i, err;
	for(i=0;i<PARAM_COUNT;++i){
		const char *native=gurobi_native_parameter[i];
		const struct slv_parameter *q=&p->parms[i];
		if(!native)continue;
		if(q->type==real_parm){
			double actual;
			err=GRBsetdblparam(env,native,q->info.r.value);
			if(!err)err=GRBgetdblparam(env,native,&actual);
			if(!err && actual!=q->info.r.value)err=GRB_ERROR_INVALID_ARGUMENT;
		}else{
			int value=q->type==bool_parm ? q->info.b.value : q->info.i.value, actual;
			err=GRBsetintparam(env,native,value);
			if(!err)err=GRBgetintparam(env,native,&actual);
			if(!err && actual!=value)err=GRB_ERROR_INVALID_ARGUMENT;
		}
		if(err){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi could not apply parameter '%s' (%s), error %d.",q->name,native,err);
			return err;
		}
	}
	return 0;
}

static void gurobi_release_model(GurobiSystem *s){
	if(s->model)GRBfreemodel(s->model);
	s->model=NULL;
}
static SlvClientToken gurobi_create(slv_system_t server, int *status){
	GurobiSystem *s=ASC_NEW_CLEAR(GurobiSystem);
	*status=1;
	if(!s)return NULL;
	s->server=server;
	if(gurobi_defaults(server,s,&s->params)){ascfree(s);return NULL;}
	s->status.kind=SLV_STATUS_LP;
	*status=0;
	return s;
}
static int gurobi_destroy(slv_system_t server, SlvClientToken token){
	GurobiSystem *s=token;
	(void)server;
	if(!s)return 0;
	gurobi_release_model(s);
	if(s->env)GRBfreeenv(s->env);
	lp_sparse_destroy(&s->lp);
	lp_nuke_pointers(&s->mps);
	slv_destroy_parms(&s->params);
	ascfree(s);
	return 0;
}
static void gurobi_getparams(slv_system_t server, SlvClientToken token, slv_parameters_t *p){
	(void)server; *p=((GurobiSystem *)token)->params;
}
static void gurobi_setparams(slv_system_t server, SlvClientToken token, slv_parameters_t *p){
	GurobiSystem *s=token;
	(void)server;
	s->params=*p;
	s->prepared=FALSE;
}
static int gurobi_status(slv_system_t server, SlvClientToken token, slv_status_t *status){
	GurobiSystem *s=token;
	(void)server;
	s->status.block.iteration=s->status.iteration;
	s->status.block.cpu_elapsed=s->status.cpu_elapsed;
	*status=s->status;
	return 0;
}

static const char *gurobi_status_name(int status){
	switch(status){
	case GRB_OPTIMAL: return "optimal";
	case GRB_INFEASIBLE: return "infeasible";
	case GRB_INF_OR_UNBD: return "infeasible or unbounded (disable dual_reductions to distinguish)";
	case GRB_UNBOUNDED: return "unbounded";
	case GRB_TIME_LIMIT: return "time limit";
	case GRB_ITERATION_LIMIT: return "iteration limit";
	case GRB_NODE_LIMIT: return "node limit";
	case GRB_SOLUTION_LIMIT: return "solution limit";
	case GRB_WORK_LIMIT: return "work limit";
	case GRB_MEM_LIMIT: return "soft memory limit";
	case GRB_INTERRUPTED: return "interrupted";
	case GRB_NUMERIC: return "numerical failure";
	case GRB_SUBOPTIMAL: return "suboptimal";
	default: return "other termination";
	}
}

static int gurobi_eligible(slv_system_t server){
	GurobiSystem *s=slv_get_client_token(server);
	struct rel_relation **r=slv_get_solvers_rel_list(server), *obj=slv_get_obj_relation(server);
	struct var_variable **v=slv_get_solvers_var_list(server);
	var_filter_t filter;
	int linearise=s && SLV_PARAM_BOOL(&s->params,NONLIN);
	filter.matchbits=VAR_FIXED|VAR_ACTIVE;
	filter.matchvalue=VAR_ACTIVE;
	if(!obj || !r || !v){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP/MIP requires an objective and solver lists.");
		return 0;
	}
	if(slv_get_num_solvers_whens(server) || slv_get_num_solvers_logrels(server)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP/MIP does not translate WHENs or logical relations.");
		return 0;
	}
	if(!linearise){
		for(;*r;++r){
			if(lp_inc_rel_filter(*r) && !lp_relation_is_affine(*r,&filter)){
				char *name=rel_make_name(server,*r);
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP/MIP cannot establish an affine form for relation '%s'.",name?name:"?");
				ASC_FREE(name); return 0;
			}
		}
		if(!lp_relation_is_affine(obj,&filter)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP/MIP requires an affine objective; enable nonlin only to solve a tangent approximation.");
			return 0;
		}
	}
	return 1;
}

static int gurobi_presolve(slv_system_t server, SlvClientToken token){
	GurobiSystem *s=token;
	s->prepared=FALSE;
	gurobi_release_model(s);
	lp_sparse_destroy(&s->lp);
	lp_nuke_pointers(&s->mps);
	memset(&s->status,0,sizeof(s->status));
	s->status.kind=SLV_STATUS_LP;
	if(gurobi_validate_options(&s->params))return 1;
	if(!gurobi_eligible(server))return 1;
	if(lp_prepare_relaxed(server,&s->mps,&s->status,SLV_PARAM_BOOL(&s->params,SCALE_VAR),SLV_PARAM_BOOL(&s->params,SCALE_REL),SLV_PARAM_BOOL(&s->params,RELAXED))
		|| lp_sparse_build(&s->lp,&s->mps,slv_get_solvers_var_list(server),slv_get_obj_relation(server),-1e20,1e20)){
		s->status.calc_ok=FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP/MIP preparation failed; check solver lists and finite model evaluations.");
		return 1;
	}
	s->status.kind=lp_problem_is_mip(&s->mps) ? SLV_STATUS_MIP : SLV_STATUS_LP;
	s->status.ok=s->status.calc_ok=s->status.ready_to_solve=TRUE;
	s->status.block.number_of=1;
	s->status.block.current_size=s->lp.num_col;
	s->prepared=TRUE;
	return 0;
}

/* Gurobi uses finite sentinels for unavailable bounds, not IEEE infinity. */
static int gurobi_finite(double value){
	return isfinite(value) && fabs(value)<GRB_INFINITY;
}

static void gurobi_mip_bounds(slv_status_mip_t *mip, double primal, double dual){
	mip->have_primal_bound=gurobi_finite(primal);
	mip->have_dual_bound=gurobi_finite(dual);
	if(mip->have_primal_bound)mip->primal_bound=primal;
	if(mip->have_dual_bound)mip->dual_bound=dual;
	mip->have_abs_gap=mip->have_primal_bound && mip->have_dual_bound;
	mip->have_gap=0;
	if(mip->have_abs_gap){
		mip->abs_gap=fabs(primal-dual);
		if(primal!=0 || mip->abs_gap==0){
			mip->gap=primal==0 ? 0 : mip->abs_gap/fabs(primal);
			mip->have_gap=isfinite(mip->gap);
		}
	}
}

static int __stdcall gurobi_callback(GRBmodel *model, void *cbdata, int where, void *userdata){
	GurobiSystem *s=userdata;
	double runtime=0, iterations=0, objective=0;
	char message[256];
	if(slv_get_solver_interrupt()){GRBterminate(model);return 0;}
	if(where==GRB_CB_MIP){
		slv_status_mip_t *mip=slv_status_mip_rw(&s->status);
		double primal=GRB_INFINITY, dual=GRB_INFINITY, nodes=0;
		int solutions=0;
		char bound_text[40], gap_text[40], obj_text[40];
		if(!mip || GRBcbget(cbdata,where,GRB_CB_RUNTIME,&runtime))return 0;
		s->status.cpu_elapsed=runtime;
		GRBcbget(cbdata,where,GRB_CB_MIP_OBJBST,&primal);
		GRBcbget(cbdata,where,GRB_CB_MIP_OBJBND,&dual);
		GRBcbget(cbdata,where,GRB_CB_MIP_SOLCNT,&solutions);
		gurobi_mip_bounds(mip,solutions>0 ? primal : GRB_INFINITY,dual);
		if(!GRBcbget(cbdata,where,GRB_CB_MIP_NODCNT,&nodes)){
			mip->have_node_count=1;
			mip->node_count=nodes>=(double)LLONG_MAX ? LLONG_MAX : (long long)nodes;
		}
		if(!GRBcbget(cbdata,where,GRB_CB_MIP_ITRCNT,&iterations)){
			s->status.iteration=iterations>INT_MAX ? INT_MAX : (int32)iterations;
			mip->have_total_lp_iterations=1;
			mip->total_lp_iterations=s->status.iteration;
		}
		if(runtime>=s->next_progress && SLV_PARAM_BOOL(&s->params,PROGRESS)){
			s->next_progress=runtime+0.25;
			if(mip->have_primal_bound)snprintf(obj_text,sizeof(obj_text),"%.12g",primal);
			else strcpy(obj_text,"none");
			if(mip->have_dual_bound)snprintf(bound_text,sizeof(bound_text),"%.12g",dual);
			else strcpy(bound_text,"unknown");
			if(mip->have_gap)snprintf(gap_text,sizeof(gap_text),"%.6g",mip->gap);
			else strcpy(gap_text,"unknown");
			snprintf(message,sizeof(message),"mip_nodes=%lld, obj=%s, mip_bound=%s, mip_gap=%s, t=%.3g",
				mip->node_count,obj_text,bound_text,gap_text,runtime);
			slv_report_progress("Gurobi",message);
			if(slv_get_solver_interrupt())GRBterminate(model);
		}
		return 0;
	}
	if(where!=GRB_CB_SIMPLEX && where!=GRB_CB_BARRIER)return 0;
	if(GRBcbget(cbdata,where,GRB_CB_RUNTIME,&runtime))return 0;
	s->status.cpu_elapsed=runtime;
	if(where==GRB_CB_SIMPLEX){
		GRBcbget(cbdata,where,GRB_CB_SPX_ITRCNT,&iterations);
		GRBcbget(cbdata,where,GRB_CB_SPX_OBJVAL,&objective);
	}else{
		int barrier_iterations=0;
		GRBcbget(cbdata,where,GRB_CB_BARRIER_ITRCNT,&barrier_iterations);
		iterations=barrier_iterations;
		GRBcbget(cbdata,where,GRB_CB_BARRIER_PRIMOBJ,&objective);
	}
	s->status.iteration=iterations>INT_MAX ? INT_MAX : (int32)iterations;
	if(runtime>=s->next_progress && SLV_PARAM_BOOL(&s->params,PROGRESS)){
		s->next_progress=runtime+0.25;
		snprintf(message,sizeof(message),"iter=%d, obj=%.12g, t=%.3g",s->status.iteration,objective,runtime);
		slv_report_progress("Gurobi",message);
		if(slv_get_solver_interrupt())GRBterminate(model);
	}
	return 0;
}

static int gurobi_solve(slv_system_t server, SlvClientToken token){
	GurobiSystem *s=token;
	lp_sparse_t *p=&s->lp;
	slv_status_lp_t *lp;
	slv_status_mip_t *mip;
	int err=0, status=GRB_LOADED, count=0, i;
	int *start=NULL, *length=NULL, *index=NULL;
	char *sense=NULL, *type=NULL;
	double *rhs=NULL, *lower=NULL, *upper=NULL, *values=NULL;
	double iterations=0, violation=0;
	GRBenv *env;
#define TRY(CALL) do {err=(CALL); if(err)goto fail;} while(0)
	if((!s->prepared || !s->status.ready_to_solve) && gurobi_presolve(server,token))return 1;
	mip=slv_status_mip_rw(&s->status);
	lp=mip ? &mip->lp : &s->status.u.lp;
	s->status.ready_to_solve=FALSE;
	if(slv_get_solver_interrupt()){
		s->status.panic=TRUE; s->status.ok=FALSE; return 0;
	}
	if(!s->env){
		TRY(gurobi_open_environment(&s->env));
	}
#define ALLOC(F,T,N) do { F=ASC_NEW_ARRAY(T,(N)>0?(N):1); if(!F){err=GRB_ERROR_OUT_OF_MEMORY;goto fail;} } while(0)
	ALLOC(start,int,p->num_col); ALLOC(length,int,p->num_col); ALLOC(index,int,p->num_nz);
	ALLOC(sense,char,p->num_row); ALLOC(rhs,double,p->num_row);
	ALLOC(lower,double,p->num_col); ALLOC(upper,double,p->num_col); ALLOC(values,double,p->num_col);
	ALLOC(type,char,p->num_col);
#undef ALLOC
	for(i=0;i<p->num_col;++i){
		start[i]=p->start[i]; length[i]=p->start[i+1]-p->start[i];
		lower[i]=isinf(p->lower[i]) ? (p->lower[i]<0 ? -GRB_INFINITY : GRB_INFINITY) : p->lower[i];
		upper[i]=isinf(p->upper[i]) ? (p->upper[i]<0 ? -GRB_INFINITY : GRB_INFINITY) : p->upper[i];
		switch(p->type[i]){
		case MPS_INT: type[i]=GRB_INTEGER; break;
		case MPS_BINARY: type[i]=GRB_BINARY; break;
		case MPS_SEMI: type[i]=GRB_SEMICONT; break;
		default: type[i]=GRB_CONTINUOUS; break;
		}
	}
	for(i=0;i<p->num_nz;++i)index[i]=p->index[i];
	for(i=0;i<p->num_row;++i){
		if(p->row_lower[i]==p->row_upper[i]){sense[i]=GRB_EQUAL;rhs[i]=p->row_lower[i];}
		else if(isfinite(p->row_upper[i])){sense[i]=GRB_LESS_EQUAL;rhs[i]=p->row_upper[i];}
		else {sense[i]=GRB_GREATER_EQUAL;rhs[i]=p->row_lower[i];}
	}
	gurobi_release_model(s);
	TRY(GRBloadmodel(s->env,&s->model,"ASCEND",p->num_col,p->num_row,
		p->maximize?GRB_MAXIMIZE:GRB_MINIMIZE,p->objective_offset,p->cost,sense,rhs,
		start,length,index,p->value,lower,upper,type,NULL,NULL));
	env=GRBgetenv(s->model);
	TRY(gurobi_apply_options(env,&s->params));
	TRY(GRBsetcallbackfunc(s->model,gurobi_callback,s));
	s->next_progress=0;
	if(SLV_PARAM_BOOL(&s->params,PROGRESS))slv_report_progress("Gurobi",mip ? "Starting MIP optimization" : "Starting LP optimization");
	if(slv_get_solver_interrupt()){status=GRB_INTERRUPTED;goto finished;}
	TRY(GRBoptimize(s->model));
	TRY(GRBgetintattr(s->model,GRB_INT_ATTR_STATUS,&status));
	TRY(GRBgetintattr(s->model,GRB_INT_ATTR_SOLCOUNT,&count));
	GRBgetdblattr(s->model,GRB_DBL_ATTR_RUNTIME,&s->status.cpu_elapsed);
	if(!GRBgetdblattr(s->model,GRB_DBL_ATTR_ITERCOUNT,&iterations)){
		s->status.iteration=iterations>INT_MAX ? INT_MAX : (int32)iterations;
		lp->have_simplex_iterations=1; lp->simplex_iterations=s->status.iteration;
	}
	if(!GRBgetintattr(s->model,GRB_INT_ATTR_BARITERCOUNT,&i)){
		lp->have_ipm_iterations=1;lp->ipm_iterations=i;
	}
	lp->have_primal_status=1;lp->primal_status=SLV_SOLUTION_STATUS_NONE;
	if(count>0){
		TRY(GRBgetdblattr(s->model,GRB_DBL_ATTR_OBJVAL,&lp->objective_value));
		lp->have_objective=1;
		TRY(GRBgetdblattr(s->model,GRB_DBL_ATTR_CONSTR_VIO,&violation));
		lp->max_primal_infeasibility=violation;
		TRY(GRBgetdblattr(s->model,GRB_DBL_ATTR_BOUND_VIO,&violation));
		lp->max_primal_infeasibility=fmax(lp->max_primal_infeasibility,violation);
		lp->have_max_primal_infeas=1;
		violation=0;
		if(mip)TRY(GRBgetdblattr(s->model,GRB_DBL_ATTR_INT_VIO,&violation));
		if(lp->max_primal_infeasibility<=SLV_PARAM_REAL(&s->params,FEAS_TOL)
			&& violation<=SLV_PARAM_REAL(&s->params,INT_TOL)){
			lp->primal_status=SLV_SOLUTION_STATUS_FEASIBLE;
			TRY(GRBgetdblattrarray(s->model,GRB_DBL_ATTR_X,0,p->num_col,values));
			s->status.calc_ok=lp_write_solution(server,&s->mps,values,1)==0;
		}else lp->primal_status=SLV_SOLUTION_STATUS_INFEASIBLE;
	}
	if(mip){
		double dual=GRB_INFINITY, nodes=0, gap=GRB_INFINITY;
		GRBgetdblattr(s->model,GRB_DBL_ATTR_OBJBOUND,&dual);
		gurobi_mip_bounds(mip,count>0 ? lp->objective_value : GRB_INFINITY,dual);
		if(!GRBgetdblattr(s->model,GRB_DBL_ATTR_MIPGAP,&gap) && gurobi_finite(gap)){
			mip->have_gap=1; mip->gap=gap;
		}
		mip->have_solution_count=1; mip->solution_count=count;
		if(!GRBgetdblattr(s->model,GRB_DBL_ATTR_NODECOUNT,&nodes)){
			mip->have_node_count=1;
			mip->node_count=nodes>=(double)LLONG_MAX ? LLONG_MAX : (long long)nodes;
		}
		mip->have_total_lp_iterations=lp->have_simplex_iterations;
		mip->total_lp_iterations=lp->simplex_iterations;
	}
finished:
	lp->have_model_status=1;lp->model_status=status;
	s->status.converged=status==GRB_OPTIMAL && s->status.calc_ok
		&& lp->primal_status==SLV_SOLUTION_STATUS_FEASIBLE;
	s->status.inconsistent=status==GRB_INFEASIBLE;
	s->status.time_limit_exceeded=status==GRB_TIME_LIMIT;
	/* The generic API has no separate node/solution/work-limit flag. The native
	 * model status preserves the exact reason; these are computational limits.
	 */
	s->status.iteration_limit_exceeded=status==GRB_ITERATION_LIMIT || status==GRB_NODE_LIMIT || status==GRB_SOLUTION_LIMIT || status==GRB_WORK_LIMIT;
	s->status.panic=status==GRB_INTERRUPTED;
	s->status.diverged=status==GRB_UNBOUNDED || status==GRB_INF_OR_UNBD || status==GRB_NUMERIC;
	s->status.ok=s->status.converged;
	if(!s->status.converged)ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Gurobi %s terminated: %s, status %d (feasible solution: %s).",mip ? "MIP" : "LP",gurobi_status_name(status),status,lp->primal_status==SLV_SOLUTION_STATUS_FEASIBLE?"yes":"no");
	if(SLV_PARAM_BOOL(&s->params,PROGRESS))slv_report_progress("Gurobi",gurobi_status_name(status));
	if(SLV_PARAM_BOOL(&s->params,NONLIN))ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Gurobi status refers to the tangent LP/MIP, not the original nonlinear model.");
	goto cleanup;
fail:
	s->status.ok=s->status.converged=FALSE;
	s->status.calc_ok=FALSE;
	/* Native messages can include credentials during environment startup. */
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi C API error %d. Check parameters and Gurobi license configuration (GRB_LICENSE_FILE).",err);
	if(!s->model && s->env){GRBfreeenv(s->env);s->env=NULL;}
cleanup:
	ASC_FREE(start); ASC_FREE(length); ASC_FREE(index); ASC_FREE(sense);
	ASC_FREE(rhs); ASC_FREE(lower); ASC_FREE(upper); ASC_FREE(values); ASC_FREE(type);
	return err ? 1 : 0;
#undef TRY
}

static int gurobi_resolve(slv_system_t server, SlvClientToken token){
	if(gurobi_presolve(server,token))return 1;
	return gurobi_solve(server,token);
}
static int gurobi_version(char *buf, size_t size){
	int major,minor,technical;
	if(!buf || !size)return 1;
	GRBversion(&major,&minor,&technical);
	snprintf(buf,size,"Gurobi %d.%d.%d",major,minor,technical);
	return 0;
}

static int gurobi_details(char *buf, size_t size){
	GRBenv *env=NULL;
	int err, license_id=0;
	if(!buf || !size)return 1;
	/* Ask Gurobi to validate its selected license, including native locations
	 * and server licenses. File existence alone cannot establish validity.
	 * Release the environment immediately; do not solve or cache the result.
	 */
	err=gurobi_open_environment(&env);
	if(!err){
		/* Only expose the numeric identifier, never WLS access IDs, secrets or
		 * tokens. LicenseID may be unset for non-WLS licensing schemes.
		 */
		if(!GRBgetintparam(env,GRB_INT_PAR_LICENSEID,&license_id) && license_id>0){
			snprintf(buf,size,"licensed; LICENSEID=%d",license_id);
		}else{
			snprintf(buf,size,"licensed; LICENSEID unavailable");
		}
	}else if(err==GRB_ERROR_NO_LICENSE){
		/* This also covers some service failures, not just a missing file. */
		snprintf(buf,size,"license unavailable (Gurobi error %d)",err);
	}else{
		snprintf(buf,size,"license validation inconclusive (Gurobi error %d)",err);
	}
	if(env)GRBfreeenv(env);
	/* Return details even on validation failure, so listings show the state.
	 * Never return GRBgeterrormsg: it can contain credentials and private paths.
	 */
	return 0;
}
static const SlvFunctionsT gurobi_functions={
	GUROBI_SOLVER_NUMBER,"Gurobi",gurobi_create,gurobi_destroy,gurobi_eligible,gurobi_defaults,
	gurobi_getparams,gurobi_setparams,gurobi_status,gurobi_solve,
	gurobi_presolve,gurobi_solve,gurobi_resolve,NULL,NULL,NULL
};
ASC_EXPORT int gurobi_register(void){
	if(solver_register(&gurobi_functions))return 1;
	solver_register_version("Gurobi",gurobi_version);
	solver_register_details("Gurobi",gurobi_details);
	return 0;
}
