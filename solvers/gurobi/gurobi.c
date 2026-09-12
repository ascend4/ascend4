/* Gurobi continuous LP adapter for ASCEND.
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
	PRESOLVE, FEAS_TOL, OPT_TOL, PROGRESS, DUAL_REDUCTIONS, PARAM_COUNT };
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
	BOOL(NONLIN,"nonlin","Linearise nonlinear expressions?","Solve one tangent LP at the current point. This does not solve the original nonlinear model.",FALSE);
	BOOL(RELAXED,"relaxed","Solve continuous relaxation?","Explicitly relax discrete solver variables. Otherwise this LP-only adapter rejects them.",FALSE);
	BOOL(SCALE_VAR,"varnom_scale","Scale variables?","Scale continuous variables using ASCEND nominal values.",FALSE);
	BOOL(SCALE_REL,"relnom_scale","Scale relations?","Scale rows using ASCEND nominal values and coefficient magnitudes.",FALSE);
	BOOL(PROGRESS,"progress_callbacks","Progress callbacks?","Report LP progress through ASCEND. Interrupts are always checked.",TRUE);
	BOOL(DUAL_REDUCTIONS,"dual_reductions","Dual reductions?","Disable to distinguish infeasible from unbounded when presolve reports INF_OR_UNBD.",TRUE);
	REAL(TIME_LIMIT,"time_limit","Time limit (s)","Gurobi optimization time limit.",GRB_INFINITY,0,GRB_INFINITY);
	INT(THREADS,"threads","Threads","Zero lets Gurobi choose the number of threads.",0,0,1024);
	INT(METHOD,"method","LP algorithm","-1 automatic, 0 primal simplex, 1 dual simplex, 2 barrier, 3 concurrent, 4 deterministic concurrent.",-1,-1,4);
	INT(PRESOLVE,"presolve","Presolve","-1 automatic, 0 off, 1 conservative, 2 aggressive.",-1,-1,2);
	REAL(FEAS_TOL,"feasibility_tol","Feasibility tolerance","Primal feasibility tolerance.",1e-6,1e-9,1e-2);
	REAL(OPT_TOL,"optimality_tol","Optimality tolerance","Dual feasibility tolerance.",1e-6,1e-9,1e-2);
#undef BOOL
#undef INT
#undef REAL
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
	int relaxed=s && SLV_PARAM_BOOL(&s->params,RELAXED);
	filter.matchbits=VAR_FIXED|VAR_ACTIVE;
	filter.matchvalue=VAR_ACTIVE;
	if(!obj || !r || !v){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP requires an objective and solver lists.");
		return 0;
	}
	if(slv_get_num_solvers_whens(server) || slv_get_num_solvers_logrels(server)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP does not translate WHENs or logical relations.");
		return 0;
	}
	for(;*v;++v){
		if(!relaxed && lp_var_needs_relaxation(*v)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi currently supports continuous LPs only; use relaxed explicitly for a continuous relaxation.");
			return 0;
		}
	}
	if(!linearise){
		for(;*r;++r){
			if(lp_inc_rel_filter(*r) && !lp_relation_is_affine(*r,&filter)){
				char *name=rel_make_name(server,*r);
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP cannot establish an affine form for relation '%s'.",name?name:"?");
				ASC_FREE(name); return 0;
			}
		}
		if(!lp_relation_is_affine(obj,&filter)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP requires an affine objective; enable nonlin only to solve a tangent approximation.");
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
	if(!gurobi_eligible(server))return 1;
	if(lp_prepare(server,&s->mps,&s->status,SLV_PARAM_BOOL(&s->params,SCALE_VAR),SLV_PARAM_BOOL(&s->params,SCALE_REL))
		|| lp_sparse_build(&s->lp,&s->mps,slv_get_solvers_var_list(server),slv_get_obj_relation(server),-1e20,1e20)){
		s->status.calc_ok=FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi LP preparation failed; check solver lists and finite model evaluations.");
		return 1;
	}
	s->status.ok=s->status.calc_ok=s->status.ready_to_solve=TRUE;
	s->status.block.number_of=1;
	s->status.block.current_size=s->lp.num_col;
	s->prepared=TRUE;
	return 0;
}

static int __stdcall gurobi_callback(GRBmodel *model, void *cbdata, int where, void *userdata){
	GurobiSystem *s=userdata;
	double runtime=0, iterations=0, objective=0;
	char message[160];
	if(slv_get_solver_interrupt()){GRBterminate(model);return 0;}
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
		snprintf(message,sizeof(message),"iteration=%d objective=%.12g elapsed=%.3g",s->status.iteration,objective,runtime);
		slv_report_progress("Gurobi",message);
		if(slv_get_solver_interrupt())GRBterminate(model);
	}
	return 0;
}

static int gurobi_solve(slv_system_t server, SlvClientToken token){
	GurobiSystem *s=token;
	lp_sparse_t *p=&s->lp;
	slv_status_lp_t *lp=&s->status.u.lp;
	int err=0, status=GRB_LOADED, count=0, i;
	int *start=NULL, *length=NULL, *index=NULL;
	char *sense=NULL;
	double *rhs=NULL, *lower=NULL, *upper=NULL, *values=NULL;
	double iterations=0, violation=0;
	GRBenv *env;
#define TRY(CALL) do {err=(CALL); if(err)goto fail;} while(0)
	if((!s->prepared || !s->status.ready_to_solve) && gurobi_presolve(server,token))return 1;
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
#undef ALLOC
	for(i=0;i<p->num_col;++i){
		start[i]=p->start[i]; length[i]=p->start[i+1]-p->start[i];
		lower[i]=isinf(p->lower[i]) ? (p->lower[i]<0 ? -GRB_INFINITY : GRB_INFINITY) : p->lower[i];
		upper[i]=isinf(p->upper[i]) ? (p->upper[i]<0 ? -GRB_INFINITY : GRB_INFINITY) : p->upper[i];
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
		start,length,index,p->value,lower,upper,NULL,NULL,NULL));
	env=GRBgetenv(s->model);
	TRY(GRBsetdblparam(env,GRB_DBL_PAR_TIMELIMIT,SLV_PARAM_REAL(&s->params,TIME_LIMIT)));
	TRY(GRBsetintparam(env,GRB_INT_PAR_THREADS,SLV_PARAM_INT(&s->params,THREADS)));
	TRY(GRBsetintparam(env,GRB_INT_PAR_METHOD,SLV_PARAM_INT(&s->params,METHOD)));
	TRY(GRBsetintparam(env,GRB_INT_PAR_PRESOLVE,SLV_PARAM_INT(&s->params,PRESOLVE)));
	TRY(GRBsetintparam(env,GRB_INT_PAR_DUALREDUCTIONS,SLV_PARAM_BOOL(&s->params,DUAL_REDUCTIONS)));
	TRY(GRBsetdblparam(env,GRB_DBL_PAR_FEASIBILITYTOL,SLV_PARAM_REAL(&s->params,FEAS_TOL)));
	TRY(GRBsetdblparam(env,GRB_DBL_PAR_OPTIMALITYTOL,SLV_PARAM_REAL(&s->params,OPT_TOL)));
	TRY(GRBsetcallbackfunc(s->model,gurobi_callback,s));
	s->next_progress=0;
	if(SLV_PARAM_BOOL(&s->params,PROGRESS))slv_report_progress("Gurobi","Starting LP optimization");
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
		if(lp->max_primal_infeasibility<=SLV_PARAM_REAL(&s->params,FEAS_TOL)){
			lp->primal_status=SLV_SOLUTION_STATUS_FEASIBLE;
			TRY(GRBgetdblattrarray(s->model,GRB_DBL_ATTR_X,0,p->num_col,values));
			s->status.calc_ok=lp_write_solution(server,&s->mps,values,1)==0;
		}else lp->primal_status=SLV_SOLUTION_STATUS_INFEASIBLE;
	}
finished:
	lp->have_model_status=1;lp->model_status=status;
	s->status.converged=status==GRB_OPTIMAL && s->status.calc_ok
		&& lp->primal_status==SLV_SOLUTION_STATUS_FEASIBLE;
	s->status.inconsistent=status==GRB_INFEASIBLE;
	s->status.time_limit_exceeded=status==GRB_TIME_LIMIT;
	s->status.iteration_limit_exceeded=status==GRB_ITERATION_LIMIT;
	s->status.panic=status==GRB_INTERRUPTED;
	s->status.diverged=status==GRB_UNBOUNDED || status==GRB_INF_OR_UNBD || status==GRB_NUMERIC;
	s->status.ok=s->status.converged;
	if(!s->status.converged)ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Gurobi LP terminated: %s, status %d (feasible solution: %s).",gurobi_status_name(status),status,lp->primal_status==SLV_SOLUTION_STATUS_FEASIBLE?"yes":"no");
	if(SLV_PARAM_BOOL(&s->params,PROGRESS))slv_report_progress("Gurobi",gurobi_status_name(status));
	if(SLV_PARAM_BOOL(&s->params,NONLIN))ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Gurobi status refers to the tangent LP, not the original nonlinear model.");
	goto cleanup;
fail:
	s->status.ok=s->status.converged=FALSE;
	s->status.calc_ok=FALSE;
	/* Native messages can include credentials during environment startup. */
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Gurobi C API error %d. Check parameters and Gurobi license configuration (GRB_LICENSE_FILE).",err);
	if(!s->model && s->env){GRBfreeenv(s->env);s->env=NULL;}
cleanup:
	ASC_FREE(start); ASC_FREE(length); ASC_FREE(index); ASC_FREE(sense);
	ASC_FREE(rhs); ASC_FREE(lower); ASC_FREE(upper); ASC_FREE(values);
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
