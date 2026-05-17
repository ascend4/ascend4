/*
 * NLopt SLSQP solver adapter for ASCEND.
 */

#define ASC_BUILDING_INTERFACE

#include "asc_slsqp.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <nlopt.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/general/mem.h>
#include <ascend/general/platform.h>
#include <ascend/general/tm_time.h>
#include <ascend/system/rel.h>
#include <ascend/system/relman.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/var.h>
#include <ascend/utilities/error.h>

ASC_DLLSPEC SolverRegisterFn slsqp_register;

enum SlsqpParam {
	SLSQP_PARAM_SAFE_CALC = 0,
	SLSQP_PARAM_PROGRESS_CALLBACKS,
	SLSQP_PARAM_PROGRESS_LOG,
	SLSQP_PARAM_MAX_ITER,
	SLSQP_PARAM_MAX_TIME,
	SLSQP_PARAM_FTOL_REL,
	SLSQP_PARAM_FTOL_ABS,
	SLSQP_PARAM_XTOL_REL,
	SLSQP_PARAM_XTOL_ABS,
	SLSQP_PARAM_CONSTRAINT_TOL,
	SLSQP_PARAM_STOPVAL,
	SLSQP_PARAM_FINITE_DIFF,
	SLSQP_PARAM_PRINT_LEVEL,
	SLSQP_PARAM_SCALEOPT,
	SLSQP_PARAM_BACKEND,
	SLSQP_PARAM_COUNT
};

struct SlsqpView {
	void **vars;
	void **rels;
	void *obj;
	int32 n_var;
	int32 n_rel;
	int32 n_eq;
	int32 n_ineq;
	int32 *eq_row;
	int32 *ineq_row;
	real64 *ineq_sign;
	int32 *var_sindex;
	int32 *var_mindex;
	real64 *x;
	real64 *lb;
	real64 *ub;
	real64 obj_value;
	real64 obj_sign;
	real64 *obj_grad;
	real64 *rel_residual;
	enum rel_enum *relop;
	int32 calc_errors;
	int32 derivative_errors;
	int32 unsupported_rels;
};

struct SlsqpSystem {
	slv_system_t server;
	slv_parameters_t params;
	struct slv_parameter param_data[SLSQP_PARAM_COUNT];
	slv_status_t status;
	struct SlsqpView view;
	nlopt_opt opt;
	nlopt_result result;
	int obj_evals;
	int grad_evals;
	int con_evals;
	int jac_evals;
	real64 last_obj;
	real64 last_max_viol;
	real64 clock;
	real64 last_progress_time;
	const char *callback_error;
};

#define SYS(s) ((struct SlsqpSystem *)(s))

static int32 slsqp_count_vars(struct var_variable **vars){
	int32 n = 0;
	if(vars != NULL){
		while(vars[n] != NULL){
			++n;
		}
	}
	return n;
}

static int32 slsqp_count_rels(struct rel_relation **rels){
	int32 n = 0;
	if(rels != NULL){
		while(rels[n] != NULL){
			++n;
		}
	}
	return n;
}

static void slsqp_view_destroy(struct SlsqpView *view){
	if(view == NULL){
		return;
	}
	ASC_FREE(view->vars);
	ASC_FREE(view->eq_row);
	ASC_FREE(view->ineq_row);
	ASC_FREE(view->ineq_sign);
	ASC_FREE(view->var_sindex);
	ASC_FREE(view->var_mindex);
	ASC_FREE(view->x);
	ASC_FREE(view->lb);
	ASC_FREE(view->ub);
	ASC_FREE(view->obj_grad);
	ASC_FREE(view->rel_residual);
	ASC_FREE(view->relop);
	memset(view,0,sizeof(*view));
}

static int32 slsqp_view_find_col(const struct SlsqpView *view, int32 sindex){
	int32 i;
	if(view == NULL){
		return -1;
	}
	for(i = 0; i < view->n_var; ++i){
		if(view->var_sindex[i] == sindex){
			return i;
		}
	}
	return -1;
}

static int slsqp_capture_vars(struct SlsqpView *view, struct var_variable **allvars){
	var_filter_t vfilter;
	int32 total;
	int32 i;
	int32 n = 0;
	if(view == NULL || allvars == NULL){
		return 1;
	}
	total = slsqp_count_vars(allvars);
	view->vars = ASC_NEW_ARRAY_OR_NULL(void *,total + 1);
	if(view->vars == NULL){
		return 1;
	}
	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	for(i = 0; i < total; ++i){
		if(var_apply_filter(allvars[i],&vfilter)){
			view->vars[n++] = allvars[i];
		}
	}
	view->vars[n] = NULL;
	view->n_var = n;
	if(n <= 0){
		return 0;
	}
	view->var_sindex = ASC_NEW_ARRAY_OR_NULL(int32,n);
	view->var_mindex = ASC_NEW_ARRAY_OR_NULL(int32,n);
	view->x = ASC_NEW_ARRAY_OR_NULL(real64,n);
	view->lb = ASC_NEW_ARRAY_OR_NULL(real64,n);
	view->ub = ASC_NEW_ARRAY_OR_NULL(real64,n);
	view->obj_grad = ASC_NEW_ARRAY_OR_NULL(real64,n);
	if(view->var_sindex == NULL || view->var_mindex == NULL || view->x == NULL
		|| view->lb == NULL || view->ub == NULL || view->obj_grad == NULL
	){
		return 1;
	}
	for(i = 0; i < n; ++i){
		struct var_variable *var = (struct var_variable *)view->vars[i];
		view->var_sindex[i] = var_sindex(var);
		view->var_mindex[i] = var_mindex(var);
		view->x[i] = var_value(var);
		view->lb[i] = var_lower_bound(var);
		view->ub[i] = var_upper_bound(var);
		if(view->lb[i] <= var_NO_LOWER_BOUND / 4.0){
			view->lb[i] = -HUGE_VAL;
		}
		if(view->ub[i] >= var_NO_UPPER_BOUND / 4.0){
			view->ub[i] = HUGE_VAL;
		}
	}
	return 0;
}

static int slsqp_capture_rels(struct SlsqpView *view, struct rel_relation **rels, int safe){
	int32 i;
	int32 ieq = 0;
	int32 iineq = 0;
	if(view == NULL){
		return 1;
	}
	view->rels = (void **)rels;
	view->n_rel = slsqp_count_rels(rels);
	if(view->n_rel <= 0){
		return 0;
	}
	view->rel_residual = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
	view->relop = ASC_NEW_ARRAY_OR_NULL(enum rel_enum,view->n_rel);
	view->eq_row = ASC_NEW_ARRAY_OR_NULL(int32,view->n_rel);
	view->ineq_row = ASC_NEW_ARRAY_OR_NULL(int32,view->n_rel);
	view->ineq_sign = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
	if(view->rel_residual == NULL || view->relop == NULL || view->eq_row == NULL
		|| view->ineq_row == NULL || view->ineq_sign == NULL
	){
		return 1;
	}
	for(i = 0; i < view->n_rel; ++i){
		int32 calc_ok = 0;
		struct rel_relation *rel = (struct rel_relation *)view->rels[i];
		view->relop[i] = rel_relop(rel);
		view->rel_residual[i] = relman_eval(rel,&calc_ok,safe);
		if(!calc_ok){
			++view->calc_errors;
		}
		switch(view->relop[i]){
		case e_rel_equal:
			view->eq_row[ieq++] = i;
			break;
		case e_rel_less:
		case e_rel_lesseq:
			view->ineq_row[iineq] = i;
			view->ineq_sign[iineq] = 1.0;
			++iineq;
			break;
		case e_rel_greater:
		case e_rel_greatereq:
			view->ineq_row[iineq] = i;
			view->ineq_sign[iineq] = -1.0;
			++iineq;
			break;
		default:
			++view->unsupported_rels;
			break;
		}
	}
	view->n_eq = ieq;
	view->n_ineq = iineq;
	return 0;
}

static int slsqp_capture_objective(struct SlsqpView *view, int safe){
	int32 calc_ok = 0;
	if(view == NULL){
		return 1;
	}
	view->obj_sign = 1.0;
	if(view->obj == NULL){
		view->obj_value = 0.0;
		return 0;
	}
	if(relman_obj_direction((struct rel_relation *)view->obj) > 0){
		view->obj_sign = -1.0;
	}
	view->obj_value = view->obj_sign * relman_eval((struct rel_relation *)view->obj,&calc_ok,safe);
	if(!calc_ok){
		++view->calc_errors;
		return 0;
	}
	return 0;
}

static int slsqp_build_view(struct SlsqpSystem *sys){
	struct var_variable **vars;
	struct rel_relation **rels;
	int safe;
	if(sys == NULL || sys->server == NULL){
		return 1;
	}
	slsqp_view_destroy(&sys->view);
	vars = slv_get_solvers_var_list(sys->server);
	rels = slv_get_solvers_rel_list(sys->server);
	sys->view.obj = (void *)slv_get_obj_relation(sys->server);
	safe = SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_SAFE_CALC);
	if(vars == NULL){
		return 1;
	}
	if(slsqp_capture_vars(&sys->view,vars)){
		return 1;
	}
	if(slsqp_capture_rels(&sys->view,rels,safe)){
		return 1;
	}
	if(slsqp_capture_objective(&sys->view,safe)){
		return 1;
	}
	return 0;
}

static int slsqp_push_x(struct SlsqpSystem *sys, const double *x){
	int32 i;
	if(sys == NULL || x == NULL){
		return 1;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		var_set_value((struct var_variable *)sys->view.vars[i],x[i]);
		sys->view.x[i] = x[i];
	}
	return 0;
}

static int slsqp_refresh(struct SlsqpSystem *sys, unsigned n, const double *x){
	int safe;
	int32 i;
	if(sys == NULL || x == NULL || (int32)n != sys->view.n_var){
		return 1;
	}
	if(slsqp_push_x(sys,x)){
		return 1;
	}
	safe = SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_SAFE_CALC);
	sys->view.calc_errors = 0;
	for(i = 0; i < sys->view.n_rel; ++i){
		int32 calc_ok = 0;
		sys->view.rel_residual[i] = relman_eval((struct rel_relation *)sys->view.rels[i],&calc_ok,safe);
		if(!calc_ok){
			++sys->view.calc_errors;
		}
	}
	if(sys->view.obj != NULL){
		int32 calc_ok = 0;
		sys->view.obj_value = sys->view.obj_sign
			* relman_eval((struct rel_relation *)sys->view.obj,&calc_ok,safe);
		if(!calc_ok){
			++sys->view.calc_errors;
		}
	}else{
		sys->view.obj_value = 0.0;
	}
	return sys->view.calc_errors == 0 ? 0 : 1;
}

static int slsqp_fill_obj_grad(struct SlsqpSystem *sys, double *grad){
	real64 *derivs = NULL;
	int32 *vars = NULL;
	int32 count = 0;
	int32 cap;
	int32 i;
	var_filter_t vfilter;
	if(sys == NULL || grad == NULL){
		return 1;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		grad[i] = 0.0;
	}
	if(sys->view.obj == NULL || sys->view.n_var <= 0){
		return 0;
	}
	cap = slv_get_num_solvers_vars(sys->server);
	if(cap < sys->view.n_var){
		cap = sys->view.n_var;
	}
	derivs = ASC_NEW_ARRAY_OR_NULL(real64,cap > 0 ? cap : 1);
	vars = ASC_NEW_ARRAY_OR_NULL(int32,cap > 0 ? cap : 1);
	if(derivs == NULL || vars == NULL){
		ASC_FREE(derivs);
		ASC_FREE(vars);
		return 1;
	}
	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	if(relman_diff2_rev((struct rel_relation *)sys->view.obj,&vfilter,derivs,vars,&count,SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_SAFE_CALC))){
		count = 0;
		if(relman_diff2((struct rel_relation *)sys->view.obj,&vfilter,derivs,vars,&count,SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_SAFE_CALC))){
			ASC_FREE(derivs);
			ASC_FREE(vars);
			return 1;
		}
	}
	for(i = 0; i < count; ++i){
		int32 col = slsqp_view_find_col(&sys->view,vars[i]);
		if(col >= 0){
			grad[col] = sys->view.obj_sign * derivs[i];
		}
	}
	ASC_FREE(derivs);
	ASC_FREE(vars);
	return 0;
}

static int slsqp_fill_rel_grad(struct SlsqpSystem *sys, int32 row, real64 sign, double *grad){
	real64 *derivs = NULL;
	int32 *vars = NULL;
	int32 count = 0;
	int32 cap;
	int32 i;
	var_filter_t vfilter;
	struct rel_relation *rel;
	if(sys == NULL || grad == NULL || row < 0 || row >= sys->view.n_rel){
		return 1;
	}
	rel = (struct rel_relation *)sys->view.rels[row];
	cap = slv_get_num_solvers_vars(sys->server);
	if(cap < sys->view.n_var){
		cap = sys->view.n_var;
	}
	derivs = ASC_NEW_ARRAY_OR_NULL(real64,cap > 0 ? cap : 1);
	vars = ASC_NEW_ARRAY_OR_NULL(int32,cap > 0 ? cap : 1);
	if(derivs == NULL || vars == NULL){
		ASC_FREE(derivs);
		ASC_FREE(vars);
		return 1;
	}
	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	if(relman_diff2_rev(rel,&vfilter,derivs,vars,&count,SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_SAFE_CALC))){
		count = 0;
		if(relman_diff2(rel,&vfilter,derivs,vars,&count,SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_SAFE_CALC))){
			ASC_FREE(derivs);
			ASC_FREE(vars);
			return 1;
		}
	}
	for(i = 0; i < count; ++i){
		int32 col = slsqp_view_find_col(&sys->view,vars[i]);
		if(col >= 0){
			grad[col] = sign * derivs[i];
		}
	}
	ASC_FREE(derivs);
	ASC_FREE(vars);
	return 0;
}

static void slsqp_maybe_progress(struct SlsqpSystem *sys){
	double now;
	char msg[512];
	if(sys == NULL || !SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_PROGRESS_CALLBACKS)){
		return;
	}
	now = tm_cpu_time();
	if(sys->last_progress_time > 0.0 && now - sys->last_progress_time < 1.0){
		return;
	}
	sys->last_progress_time = now;
	snprintf(msg,sizeof(msg),
		"eval=%d obj=%.17g viol=%.17g obj_eval=%d con_eval=%d",
		sys->obj_evals + sys->con_evals,
		sys->last_obj,
		sys->last_max_viol,
		sys->obj_evals,
		sys->con_evals
	);
	if(SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_PROGRESS_LOG)){
		ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"(SLSQP progress) %s",msg);
	}
	(void)slv_report_progress(SLSQP_SOLVER_NAME,msg);
}

static double slsqp_objective_cb(unsigned n, const double *x, double *grad, void *data){
	struct SlsqpSystem *sys = (struct SlsqpSystem *)data;
	if(sys == NULL || slsqp_refresh(sys,n,x)){
		if(sys != NULL && sys->opt != NULL){
			sys->callback_error = "objective refresh failed";
			nlopt_force_stop(sys->opt);
		}
		return HUGE_VAL;
	}
	++sys->obj_evals;
	if(grad != NULL){
		++sys->grad_evals;
		if(slsqp_fill_obj_grad(sys,grad)){
			sys->callback_error = "objective gradient failed";
			nlopt_force_stop(sys->opt);
			return HUGE_VAL;
		}
	}
	sys->last_obj = sys->view.obj_value;
	slsqp_maybe_progress(sys);
	if(slv_get_solver_interrupt()){
		nlopt_force_stop(sys->opt);
	}
	return sys->view.obj_value;
}

static void slsqp_eq_cb(unsigned m, double *result, unsigned n, const double *x, double *grad, void *data){
	struct SlsqpSystem *sys = (struct SlsqpSystem *)data;
	unsigned i;
	if(sys == NULL || result == NULL || slsqp_refresh(sys,n,x)){
		if(sys != NULL && sys->opt != NULL){
			sys->callback_error = "equality refresh failed";
			nlopt_force_stop(sys->opt);
		}
		return;
	}
	++sys->con_evals;
	if(grad != NULL){
		memset(grad,0,sizeof(double) * m * n);
		++sys->jac_evals;
	}
	for(i = 0; i < m && i < (unsigned)sys->view.n_eq; ++i){
		int32 row = sys->view.eq_row[i];
		result[i] = sys->view.rel_residual[row];
		if(grad != NULL && slsqp_fill_rel_grad(sys,row,1.0,&grad[i * n])){
			sys->callback_error = "equality gradient failed";
			nlopt_force_stop(sys->opt);
			return;
		}
	}
	slsqp_maybe_progress(sys);
	if(slv_get_solver_interrupt()){
		nlopt_force_stop(sys->opt);
	}
}

static void slsqp_ineq_cb(unsigned m, double *result, unsigned n, const double *x, double *grad, void *data){
	struct SlsqpSystem *sys = (struct SlsqpSystem *)data;
	unsigned i;
	if(sys == NULL || result == NULL || slsqp_refresh(sys,n,x)){
		if(sys != NULL && sys->opt != NULL){
			sys->callback_error = "inequality refresh failed";
			nlopt_force_stop(sys->opt);
		}
		return;
	}
	++sys->con_evals;
	if(grad != NULL){
		memset(grad,0,sizeof(double) * m * n);
		++sys->jac_evals;
	}
	for(i = 0; i < m && i < (unsigned)sys->view.n_ineq; ++i){
		int32 row = sys->view.ineq_row[i];
		real64 sign = sys->view.ineq_sign[i];
		result[i] = sign * sys->view.rel_residual[row];
		if(grad != NULL && slsqp_fill_rel_grad(sys,row,sign,&grad[i * n])){
			sys->callback_error = "inequality gradient failed";
			nlopt_force_stop(sys->opt);
			return;
		}
	}
	slsqp_maybe_progress(sys);
	if(slv_get_solver_interrupt()){
		nlopt_force_stop(sys->opt);
	}
}

static real64 slsqp_max_violation(struct SlsqpSystem *sys){
	real64 maxviol = 0.0;
	int32 i;
	if(sys == NULL){
		return HUGE_VAL;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		if(sys->view.x[i] < sys->view.lb[i] && isfinite(sys->view.lb[i])){
			maxviol = fmax(maxviol,sys->view.lb[i] - sys->view.x[i]);
		}
		if(sys->view.x[i] > sys->view.ub[i] && isfinite(sys->view.ub[i])){
			maxviol = fmax(maxviol,sys->view.x[i] - sys->view.ub[i]);
		}
	}
	for(i = 0; i < sys->view.n_rel; ++i){
		real64 r = sys->view.rel_residual[i];
		switch(sys->view.relop[i]){
		case e_rel_equal:
			maxviol = fmax(maxviol,fabs(r));
			break;
		case e_rel_less:
		case e_rel_lesseq:
			maxviol = fmax(maxviol,fmax(0.0,r));
			break;
		case e_rel_greater:
		case e_rel_greatereq:
			maxviol = fmax(maxviol,fmax(0.0,-r));
			break;
		default:
			break;
		}
	}
	sys->last_max_viol = maxviol;
	return maxviol;
}

static void slsqp_spoof_block_status(struct SlsqpSystem *sys){
	if(sys == NULL){
		return;
	}
	sys->status.block.number_of = 1;
	sys->status.block.current_block = 0;
	sys->status.block.current_reordered_block = 0;
	sys->status.block.current_size = sys->view.n_var;
	sys->status.block.previous_total_size = 0;
	sys->status.block.previous_total_size_vars = 0;
	sys->status.block.iteration = sys->status.iteration;
	sys->status.block.funcs = sys->obj_evals + sys->con_evals;
	sys->status.block.jacs = sys->grad_evals + sys->jac_evals;
	sys->status.block.cpu_elapsed = sys->status.cpu_elapsed;
	sys->status.block.functime = 0.0;
	sys->status.block.jactime = 0.0;
	sys->status.block.residual = sys->last_max_viol;
}

static void slsqp_init_status(struct SlsqpSystem *sys){
	if(sys == NULL){
		return;
	}
	memset(&sys->status,0,sizeof(sys->status));
	sys->status.kind = SLV_STATUS_NLP;
	sys->status.ok = TRUE;
	sys->status.calc_ok = TRUE;
	sys->status.ready_to_solve = FALSE;
	sys->status.converged = FALSE;
	sys->status.diverged = FALSE;
	sys->status.iteration_limit_exceeded = FALSE;
	sys->status.time_limit_exceeded = FALSE;
	sys->last_max_viol = 0.0;
	sys->last_obj = 0.0;
	slsqp_spoof_block_status(sys);
}

static int32 slsqp_get_default_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct slv_parameter *new_parms = NULL;
	(void)server;
	(void)asys;
	if(parameters == NULL){
		return 1;
	}
	if(parameters->parms == NULL){
		new_parms = ASC_NEW_ARRAY_OR_NULL(struct slv_parameter,SLSQP_PARAM_COUNT);
		if(new_parms == NULL){
			return 1;
		}
		parameters->parms = new_parms;
		parameters->dynamic_parms = 1;
	}
	parameters->num_parms = 0;
	slv_param_bool(parameters,SLSQP_PARAM_SAFE_CALC,
		(SlvParameterInitBool){{"safeeval","Use safe evaluation?",1,
		"Use ASCEND safe residual and derivative evaluation."}, TRUE}
	);
	slv_param_bool(parameters,SLSQP_PARAM_PROGRESS_CALLBACKS,
		(SlvParameterInitBool){{"progress","Report progress?",2,
		"Emit callback/evaluation-level SLSQP progress through slv_report_progress."}, TRUE}
	);
	slv_param_bool(parameters,SLSQP_PARAM_PROGRESS_LOG,
		(SlvParameterInitBool){{"progress_log","Log progress?",2,
		"Emit SLSQP progress lines to the console/error reporter."}, FALSE}
	);
	slv_param_int(parameters,SLSQP_PARAM_MAX_ITER,
		(SlvParameterInitInt){{"max_iter","Maximum evaluations",1,
		"Maximum NLopt function evaluations. NLopt may exceed this slightly."}, 200, 0, 100000000}
	);
	slv_param_real(parameters,SLSQP_PARAM_MAX_TIME,
		(SlvParameterInitReal){{"max_time","Maximum time (s)",1,
		"Maximum NLopt solve time in seconds. Zero disables."}, 0.0, 0.0, 1e100}
	);
	slv_param_real(parameters,SLSQP_PARAM_FTOL_REL,
		(SlvParameterInitReal){{"ftol_rel","Relative objective tolerance",1,
		"Relative objective stopping tolerance."}, 1e-8, 0.0, 1e100}
	);
	slv_param_real(parameters,SLSQP_PARAM_FTOL_ABS,
		(SlvParameterInitReal){{"ftol_abs","Absolute objective tolerance",1,
		"Absolute objective stopping tolerance. Zero disables."}, 0.0, 0.0, 1e100}
	);
	slv_param_real(parameters,SLSQP_PARAM_XTOL_REL,
		(SlvParameterInitReal){{"xtol_rel","Relative variable tolerance",1,
		"Relative variable stopping tolerance."}, 1e-8, 0.0, 1e100}
	);
	slv_param_real(parameters,SLSQP_PARAM_XTOL_ABS,
		(SlvParameterInitReal){{"xtol_abs","Absolute variable tolerance",1,
		"Uniform absolute variable stopping tolerance. Zero disables."}, 0.0, 0.0, 1e100}
	);
	slv_param_real(parameters,SLSQP_PARAM_CONSTRAINT_TOL,
		(SlvParameterInitReal){{"constraint_tol","Constraint tolerance",1,
		"Feasibility tolerance for NLopt constraints and final ASCEND residual check."}, 1e-8, 0.0, 1e100}
	);
	slv_param_real(parameters,SLSQP_PARAM_STOPVAL,
		(SlvParameterInitReal){{"stopval","Target objective",2,
		"Target objective value. Values <= -1e99 disable for minimization."}, -HUGE_VAL, -HUGE_VAL, HUGE_VAL}
	);
	slv_param_bool(parameters,SLSQP_PARAM_FINITE_DIFF,
		(SlvParameterInitBool){{"finite_diff","Use finite differences?",3,
		"Reserved for a future finite-difference fallback; currently analytic derivatives are used."}, FALSE}
	);
	slv_param_int(parameters,SLSQP_PARAM_PRINT_LEVEL,
		(SlvParameterInitInt){{"print_level","Print level",2,
		"ASCEND-side SLSQP verbosity."}, 0, 0, 10}
	);
	slv_param_char(parameters,SLSQP_PARAM_SCALEOPT,
		(SlvParameterInitChar){{"scaleopt","Scaling option",2,
		"Scaling mode. The initial NLopt adapter supports NONE only."}, "NONE"},
		(char *[]){"NONE",NULL}
	);
	slv_param_char(parameters,SLSQP_PARAM_BACKEND,
		(SlvParameterInitChar){{"backend","Backend",3,
		"SLSQP backend implementation."}, "NLOPT"},
		(char *[]){"NLOPT",NULL}
	);
	return 0;
}

static SlvClientToken slsqp_create(slv_system_t server, int *statusindex){
	struct SlsqpSystem *sys = ASC_NEW_CLEAR(struct SlsqpSystem);
	if(sys == NULL){
		if(statusindex != NULL){
			*statusindex = 1;
		}
		return NULL;
	}
	sys->server = server;
	sys->params.parms = sys->param_data;
	sys->params.dynamic_parms = 0;
	slsqp_get_default_parameters(server,(SlvClientToken)sys,&sys->params);
	if(statusindex != NULL){
		sys->params.whose = *statusindex;
		*statusindex = 0;
	}
	slsqp_init_status(sys);
	return (SlvClientToken)sys;
}

static int slsqp_destroy(slv_system_t server, SlvClientToken asys){
	struct SlsqpSystem *sys = SYS(asys);
	(void)server;
	if(sys == NULL){
		return 0;
	}
	if(sys->opt != NULL){
		nlopt_destroy(sys->opt);
	}
	slsqp_view_destroy(&sys->view);
	slv_destroy_parms(&sys->params);
	ASC_FREE(sys);
	return 0;
}

static int slsqp_eligible(slv_system_t server){
	(void)server;
	return 1;
}

static void slsqp_get_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct SlsqpSystem *sys = SYS(asys);
	(void)server;
	if(sys != NULL && parameters != NULL){
		mem_copy_cast(&sys->params,parameters,sizeof(slv_parameters_t));
	}
}

static void slsqp_set_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct SlsqpSystem *sys = SYS(asys);
	(void)server;
	if(sys != NULL && parameters != NULL && parameters->whose == SLSQP_SOLVER_NUMBER){
		mem_copy_cast(parameters,&sys->params,sizeof(slv_parameters_t));
	}
}

static int slsqp_get_status(slv_system_t server, SlvClientToken asys, slv_status_t *status){
	struct SlsqpSystem *sys = SYS(asys);
	(void)server;
	if(sys == NULL || status == NULL){
		return 1;
	}
	mem_copy_cast(&sys->status,status,sizeof(slv_status_t));
	return 0;
}

static int slsqp_presolve(slv_system_t server, SlvClientToken asys){
	struct SlsqpSystem *sys = SYS(asys);
	int32 sorted_rels = 0;
	int32 sorted_vars = 0;
	if(sys == NULL){
		return 1;
	}
	sys->server = server;
	slsqp_init_status(sys);
	slv_sort_rels_and_vars(server,&sorted_rels,&sorted_vars);
	if(sorted_rels < 0 || sorted_vars < 0){
		sys->status.ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SLSQP failed to sort ASCEND solver variable/relation lists.");
		return 1;
	}
	if(slsqp_build_view(sys)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SLSQP failed to build the ASCEND problem view.");
		return 1;
	}
	if(sys->view.unsupported_rels > 0){
		sys->status.ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SLSQP found unsupported relation operators in the selected solver list.");
		return 1;
	}
	if(sys->view.calc_errors > 0 || sys->view.derivative_errors > 0){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SLSQP encountered residual or derivative evaluation errors during presolve.");
		return 1;
	}
	sys->last_max_viol = slsqp_max_violation(sys);
	sys->last_obj = sys->view.obj_value;
	sys->status.ready_to_solve = TRUE;
	slsqp_spoof_block_status(sys);
	return 0;
}

static int slsqp_configure_nlopt(struct SlsqpSystem *sys){
	double *eq_tol = NULL;
	double *ineq_tol = NULL;
	int32 i;
	double tol;
	if(sys == NULL){
		return 1;
	}
	if(sys->opt != NULL){
		nlopt_destroy(sys->opt);
		sys->opt = NULL;
	}
	sys->opt = nlopt_create(NLOPT_LD_SLSQP,(unsigned)sys->view.n_var);
	if(sys->opt == NULL){
		return 1;
	}
	if(nlopt_set_min_objective(sys->opt,slsqp_objective_cb,sys) < 0){
		return 1;
	}
	if(nlopt_set_lower_bounds(sys->opt,sys->view.lb) < 0 || nlopt_set_upper_bounds(sys->opt,sys->view.ub) < 0){
		return 1;
	}
	tol = SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_CONSTRAINT_TOL);
	if(sys->view.n_eq > 0){
		eq_tol = ASC_NEW_ARRAY_OR_NULL(double,sys->view.n_eq);
		if(eq_tol == NULL){
			return 1;
		}
		for(i = 0; i < sys->view.n_eq; ++i){
			eq_tol[i] = tol;
		}
		if(nlopt_add_equality_mconstraint(sys->opt,(unsigned)sys->view.n_eq,slsqp_eq_cb,sys,eq_tol) < 0){
			ASC_FREE(eq_tol);
			return 1;
		}
		ASC_FREE(eq_tol);
	}
	if(sys->view.n_ineq > 0){
		ineq_tol = ASC_NEW_ARRAY_OR_NULL(double,sys->view.n_ineq);
		if(ineq_tol == NULL){
			return 1;
		}
		for(i = 0; i < sys->view.n_ineq; ++i){
			ineq_tol[i] = tol;
		}
		if(nlopt_add_inequality_mconstraint(sys->opt,(unsigned)sys->view.n_ineq,slsqp_ineq_cb,sys,ineq_tol) < 0){
			ASC_FREE(ineq_tol);
			return 1;
		}
		ASC_FREE(ineq_tol);
	}
	nlopt_set_maxeval(sys->opt,SLV_PARAM_INT(&sys->params,SLSQP_PARAM_MAX_ITER));
	nlopt_set_maxtime(sys->opt,SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_MAX_TIME));
	nlopt_set_ftol_rel(sys->opt,SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_FTOL_REL));
	nlopt_set_ftol_abs(sys->opt,SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_FTOL_ABS));
	nlopt_set_xtol_rel(sys->opt,SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_XTOL_REL));
	nlopt_set_xtol_abs1(sys->opt,SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_XTOL_ABS));
	if(isfinite(SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_STOPVAL))){
		nlopt_set_stopval(sys->opt,SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_STOPVAL));
	}
	return 0;
}

static int slsqp_solve(slv_system_t server, SlvClientToken asys){
	struct SlsqpSystem *sys = SYS(asys);
	double obj = 0.0;
	double start;
	double feas_tol;
	int ok_status = 0;
	if(sys == NULL){
		return 1;
	}
	if(!sys->status.ready_to_solve && slsqp_presolve(server,asys)){
		return 1;
	}
	if(SLV_PARAM_BOOL(&sys->params,SLSQP_PARAM_FINITE_DIFF)){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"SLSQP finite_diff is not implemented; using ASCEND analytic derivatives.");
	}
	sys->obj_evals = 0;
	sys->grad_evals = 0;
	sys->con_evals = 0;
	sys->jac_evals = 0;
	sys->callback_error = NULL;
	sys->last_progress_time = 0.0;
	if(slsqp_configure_nlopt(sys)){
		sys->status.ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SLSQP failed to configure NLopt.");
		return 1;
	}
	start = tm_cpu_time();
	{
		int32 i;
		for(i = 0; i < sys->view.n_var; ++i){
			if(isfinite(sys->view.lb[i]) && sys->view.x[i] < sys->view.lb[i]){
				sys->view.x[i] = sys->view.lb[i];
			}
			if(isfinite(sys->view.ub[i]) && sys->view.x[i] > sys->view.ub[i]){
				sys->view.x[i] = sys->view.ub[i];
			}
		}
		(void)slsqp_push_x(sys,sys->view.x);
	}
	sys->result = nlopt_optimize(sys->opt,sys->view.x,&obj);
	sys->clock = tm_cpu_time() - start;
	(void)slsqp_push_x(sys,sys->view.x);
	if(slsqp_build_view(sys)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"SLSQP failed to refresh final ASCEND state.");
		return 1;
	}
	sys->last_obj = sys->view.obj_value;
	sys->last_max_viol = slsqp_max_violation(sys);
	sys->status.cpu_elapsed = sys->clock;
	sys->status.iteration = nlopt_get_numevals(sys->opt);
	sys->status.block.iteration = sys->status.iteration;
	sys->status.ready_to_solve = FALSE;
	sys->status.calc_ok = sys->view.calc_errors == 0;
	feas_tol = SLV_PARAM_REAL(&sys->params,SLSQP_PARAM_CONSTRAINT_TOL);
	ok_status = sys->result > 0 && sys->last_max_viol <= fmax(feas_tol,1e-12);
	sys->status.converged = ok_status ? TRUE : FALSE;
	sys->status.iteration_limit_exceeded = sys->result == NLOPT_MAXEVAL_REACHED;
	sys->status.time_limit_exceeded = sys->result == NLOPT_MAXTIME_REACHED;
	sys->status.diverged = (!ok_status && !sys->status.iteration_limit_exceeded && !sys->status.time_limit_exceeded) ? TRUE : FALSE;
	if(sys->result == NLOPT_FORCED_STOP && slv_get_solver_interrupt()){
		sys->status.panic = TRUE;
	}
	slsqp_spoof_block_status(sys);
	if(SLV_PARAM_INT(&sys->params,SLSQP_PARAM_PRINT_LEVEL) > 0){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,
			"SLSQP result=%s objective=%.17g max_viol=%.17g evals=%d.",
			nlopt_result_to_string(sys->result),
			sys->last_obj,
			sys->last_max_viol,
			sys->status.iteration
		);
	}
	if(ok_status || sys->result == NLOPT_MAXEVAL_REACHED || sys->result == NLOPT_MAXTIME_REACHED){
		return 0;
	}
	ERROR_REPORTER_HERE(ASC_PROG_ERR,
		"SLSQP/NLopt failed with %s; objective=%.17g max_viol=%.17g%s%s.",
		nlopt_result_to_string(sys->result),
		sys->last_obj,
		sys->last_max_viol,
		sys->callback_error != NULL ? "; callback_error=" : "",
		sys->callback_error != NULL ? sys->callback_error : ""
	);
	return 1;
}

static int slsqp_iterate(slv_system_t server, SlvClientToken asys){
	return slsqp_solve(server,asys);
}

static int slsqp_resolve(slv_system_t server, SlvClientToken asys){
	return slsqp_solve(server,asys);
}

static void slsqp_dumpinternals(slv_system_t server, SlvClientToken asys, int level){
	struct SlsqpSystem *sys = SYS(asys);
	(void)server;
	if(sys == NULL || level <= 0){
		return;
	}
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,
		"SLSQP view: vars=%ld rels=%ld eq=%ld ineq=%ld obj=%s obj_eval=%d con_eval=%d max_viol=%.17g",
		(long)sys->view.n_var,
		(long)sys->view.n_rel,
		(long)sys->view.n_eq,
		(long)sys->view.n_ineq,
		sys->view.obj != NULL ? "yes" : "no",
		sys->obj_evals,
		sys->con_evals,
		sys->last_max_viol
	);
}

static const SlvFunctionsT slsqp_internals = {
	SLSQP_SOLVER_NUMBER,
	SLSQP_SOLVER_NAME,
	slsqp_create,
	slsqp_destroy,
	slsqp_eligible,
	slsqp_get_default_parameters,
	slsqp_get_parameters,
	slsqp_set_parameters,
	slsqp_get_status,
	slsqp_solve,
	slsqp_presolve,
	slsqp_iterate,
	slsqp_resolve,
	NULL,
	NULL,
	slsqp_dumpinternals
};

ASC_EXPORT int slsqp_register(void){
	return solver_register(&slsqp_internals);
}
