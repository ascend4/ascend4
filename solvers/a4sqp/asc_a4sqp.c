/*
 * A4SQP solver registration and minimal lifecycle.
 */

#define ASC_BUILDING_INTERFACE

#include "asc_a4sqp.h"

#include "asc_a4sqp_adapter.h"
#include "a4sqp_c.h"
#include "a4sqp_core.h"
#include "asc_a4sqp_diag.h"
#include "a4sqp_hessian.h"
#include "a4sqp_lsq.h"
#include "asc_a4sqp_internal.h"
#include "asc_a4sqp_report.h"

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/safe.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/ltmatrix.h>
#include <ascend/general/mem.h>
#include <ascend/system/relman.h>
#include <ascend/system/lsq.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/var.h>
#include <ascend/utilities/error.h>

#ifdef A4SQP_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...)
#endif

static int32 a4sqp_view_var_col_from_var(const struct A4SqpView *view, const struct var_variable *var);
static void a4sqp_x_destroy(struct A4SqpSystem *sys);
static int a4sqp_x_sync_from_view(struct A4SqpSystem *sys);
static int a4sqp_x_push_to_ascend(struct A4SqpSystem *sys, const real64 *x);
static void a4sqp_init_status(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	memset(&sys->status,0,sizeof(sys->status));
	sys->status.kind = SLV_STATUS_NLP;
	sys->status.ok = TRUE;
	sys->status.calc_ok = TRUE;
	sys->status.ready_to_solve = FALSE;
	sys->status.converged = FALSE;
	sys->status.iteration = 0;
	sys->status.cpu_elapsed = 0.0;
	sys->last_merit_before = 0.0;
	sys->last_merit_after = 0.0;
	sys->last_model_merit_after = 0.0;
	sys->last_predicted_reduction = 0.0;
	sys->last_linearized_violation = 0.0;
	sys->last_violation_sum = 0.0;
	sys->last_violation_max = 0.0;
	sys->last_dual_infeasibility = 0.0;
	sys->last_complementarity = 0.0;
	sys->last_kkt_error = 0.0;
	sys->last_regularization_size = 0.0;
	sys->last_alpha = 0.0;
	sys->last_step_norm = 0.0;
	sys->trust_radius = 0.0;
	sys->last_trust_ratio = 0.0;
	sys->worst_violation_rel = -1;
	sys->line_search_failed = 0;
	memset(&sys->bound_stats,0,sizeof(sys->bound_stats));
	sys->bound_stats.worst_index = -1;
	memset(&sys->rel_stats,0,sizeof(sys->rel_stats));
	sys->acceptable_count = 0;
	sys->solved_acceptable = 0;
	a4sqp_core_restoration_state_init(&sys->restoration_state);
	sys->last_phase = A4SQP_CORE_PHASE_REGULAR;
	sys->phase_switches = 0;
	sys->regular_iterations = 0;
	sys->restoration_iterations = 0;
	sys->restoration_entries = 0;
	sys->restoration_exits = 0;
	sys->restoration_handoffs = 0;
}

static void a4sqp_x_destroy(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	ASC_FREE(sys->x);
	sys->x = NULL;
	sys->x_n = 0;
}

static int a4sqp_x_sync_from_view(struct A4SqpSystem *sys){
	int32 i;
	if(sys == NULL){
		return 1;
	}
	if(sys->x_n != sys->view.n_var || (sys->view.n_var > 0 && sys->x == NULL)){
		a4sqp_x_destroy(sys);
		if(sys->view.n_var > 0){
			sys->x = ASC_NEW_ARRAY_OR_NULL(real64,sys->view.n_var);
			if(sys->x == NULL){
				return 1;
			}
		}
		sys->x_n = sys->view.n_var;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		sys->x[i] = sys->view.var_value[i];
	}
	return 0;
}

static int a4sqp_x_push_to_ascend(struct A4SqpSystem *sys, const real64 *x){
	int32 i;
	if(sys == NULL || x == NULL || sys->view.vars == NULL || sys->view.n_var != sys->x_n){
		return 1;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		var_set_value((struct var_variable *)sys->view.vars[i],x[i]);
		sys->x[i] = x[i];
	}
	return 0;
}

static void a4sqp_spoof_block_status(struct A4SqpSystem *sys){
	struct slv__block_status_structure *block;
	int32 size;
	if(sys == NULL){
		return;
	}
	block = &sys->status.block;
	size = sys->view.n_var > 0 ? sys->view.n_var : 0;
	block->number_of = 1;
	block->current_block = 0;
	block->current_reordered_block = 0;
	block->current_size = size;
	block->previous_total_size = 0;
	block->previous_total_size_vars = 0;
	block->iteration = sys->status.iteration;
	block->funcs = 0;
	block->jacs = 0;
	block->cpu_elapsed = sys->status.cpu_elapsed;
	block->functime = 0.0;
	block->jactime = 0.0;
	block->residual = sys->last_violation_max;
}

static SlvClientToken a4sqp_create(slv_system_t server, int *statusindex){
	struct A4SqpSystem *sys;

	sys = ASC_NEW_CLEAR(struct A4SqpSystem);
	if(sys == NULL){
		if(statusindex != NULL){
			*statusindex = 1;
		}
		return NULL;
	}

	sys->server = server;
	sys->params.parms = sys->param_data;
	sys->params.dynamic_parms = 0;
	a4sqp_get_default_parameters(server,(SlvClientToken)sys,&sys->params);
	if(statusindex != NULL){
		sys->params.whose = *statusindex;
	}

	a4sqp_view_init(&sys->view);
	a4sqp_init_status(sys);

	if(statusindex != NULL){
		*statusindex = 0;
	}
	
	MSG("System created");

	return (SlvClientToken)sys;
}

static int a4sqp_destroy(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;

	if(sys == NULL){
		return 0;
	}

	a4sqp_view_destroy(&sys->view);
	a4sqp_x_destroy(sys);
	slv_destroy_parms(&sys->params);
	ascfree(sys);
	
	MSG("System destroyed");
	
	return 0;
}

static int a4sqp_eligible(slv_system_t server){
	(void)server;
	return 1;
}

static void a4sqp_get_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || parameters == NULL){
		return;
	}
	mem_copy_cast(&sys->params,parameters,sizeof(slv_parameters_t));
}

static void a4sqp_set_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || parameters == NULL){
		return;
	}
	if(parameters->whose == A4SQP_SOLVER_NUMBER){
		mem_copy_cast(parameters,&sys->params,sizeof(slv_parameters_t));
	}
}

static int a4sqp_get_status(slv_system_t server, SlvClientToken asys, slv_status_t *status){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || status == NULL){
		return 1;
	}
	mem_copy_cast(&sys->status,status,sizeof(slv_status_t));
	return 0;
}

static int32 a4sqp_view_var_col_from_var(const struct A4SqpView *view, const struct var_variable *var){
	int32 i;
	if(view == NULL || var == NULL){
		return -1;
	}
	for(i = 0; i < view->n_var; ++i){
		if((struct var_variable *)view->vars[i] == var){
			return i;
		}
		if(var_instance((struct var_variable *)view->vars[i]) == var_instance(var)){
			return i;
		}
		if(view->var_mindex != NULL && view->var_mindex[i] == var_mindex(var)){
			return i;
		}
		if(view->var_sindex[i] == var_sindex(var)){
			return i;
		}
	}
	return -1;
}

static void a4sqp_update_metrics(struct A4SqpSystem *sys){
	struct A4SqpKktResidual kkt;
	real64 feas_tol;
	if(sys == NULL){
		return;
	}
	sys->last_violation_sum = a4sqp_core_view_violation(
		&sys->view,
		&sys->last_violation_max,
		&sys->worst_violation_rel
	);
	feas_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
	a4sqp_core_kkt_error(
		&sys->view,
		sys->view.obj != NULL,
		NULL,
		feas_tol,
		&kkt
	);
	(void)a4sqp_core_bound_stats(
		&sys->view,
		sys->view.obj != NULL,
		NULL,
		kkt.lambda_sign != 0 ? (real64)kkt.lambda_sign : -1.0,
		feas_tol,
		&sys->bound_stats
	);
	(void)a4sqp_core_rel_stats(
		&sys->view,
		feas_tol,
		fmax(10.0 * feas_tol,1e-8),
		&sys->rel_stats
	);
	a4sqp_spoof_block_status(sys);
}

struct A4SqpAscendProblemCtx {
	slv_system_t server;
	struct A4SqpSystem *sys;
};

static int asc_a4sqp_view_matches_x(const struct A4SqpSystem *sys, A4SqpIndex n, const A4SqpNumber *x){
	int32 i;
	if(sys == NULL || x == NULL || n != sys->view.n_var || sys->view.var_value == NULL){
		return 0;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		if(sys->view.var_value[i] != x[i]){
			return 0;
		}
	}
	return 1;
}

static int asc_a4sqp_refresh_view(
	struct A4SqpAscendProblemCtx *ctx,
	A4SqpIndex n,
	const A4SqpNumber *x,
	A4SqpBool new_x
){
	struct A4SqpSystem *sys;
	if(ctx == NULL || ctx->sys == NULL || x == NULL){
		return 1;
	}
	sys = ctx->sys;
	if(!new_x && asc_a4sqp_view_matches_x(sys,n,x)){
		return 0;
	}
	if(n != sys->x_n || a4sqp_x_push_to_ascend(sys,x)){
		return 1;
	}
	if(asc_a4sqp_build_view(sys,ctx->server)){
		return 1;
	}
	a4sqp_update_metrics(sys);
	return 0;
}

static A4SqpBool asc_a4sqp_eval_f(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *obj_value,
	A4SqpUserDataPtr user_data
){
	struct A4SqpAscendProblemCtx *ctx = (struct A4SqpAscendProblemCtx *)user_data;
	if(ctx == NULL || ctx->sys == NULL || obj_value == NULL){
		return A4SQP_FALSE;
	}
	if(asc_a4sqp_refresh_view(ctx,n,x,new_x)){
		return A4SQP_FALSE;
	}
	*obj_value = ctx->sys->view.obj != NULL ? ctx->sys->view.obj_value : 0.0;
	return A4SQP_TRUE;
}

static A4SqpBool asc_a4sqp_eval_grad_f(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *grad_f,
	A4SqpUserDataPtr user_data
){
	struct A4SqpAscendProblemCtx *ctx = (struct A4SqpAscendProblemCtx *)user_data;
	int32 i;
	if(ctx == NULL || ctx->sys == NULL || grad_f == NULL){
		return A4SQP_FALSE;
	}
	if(asc_a4sqp_refresh_view(ctx,n,x,new_x)){
		return A4SQP_FALSE;
	}
	for(i = 0; i < ctx->sys->view.n_var; ++i){
		grad_f[i] = ctx->sys->view.obj_gradient != NULL ? ctx->sys->view.obj_gradient[i] : 0.0;
	}
	return A4SQP_TRUE;
}

static A4SqpBool asc_a4sqp_eval_g(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpNumber *g,
	A4SqpUserDataPtr user_data
){
	struct A4SqpAscendProblemCtx *ctx = (struct A4SqpAscendProblemCtx *)user_data;
	int32 i;
	if(ctx == NULL || ctx->sys == NULL || g == NULL){
		return A4SQP_FALSE;
	}
	if(asc_a4sqp_refresh_view(ctx,n,x,new_x) || m != ctx->sys->view.n_rel){
		return A4SQP_FALSE;
	}
	for(i = 0; i < ctx->sys->view.n_rel; ++i){
		g[i] = ctx->sys->view.rel_residual[i];
	}
	return A4SQP_TRUE;
}

static A4SqpBool asc_a4sqp_eval_jac_g(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpIndex nele_jac,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
){
	struct A4SqpAscendProblemCtx *ctx = (struct A4SqpAscendProblemCtx *)user_data;
	struct A4SqpSystem *sys;
	int32 row;
	int32 k;
	if(ctx == NULL || ctx->sys == NULL){
		return A4SQP_FALSE;
	}
	sys = ctx->sys;
	if(values != NULL && asc_a4sqp_refresh_view(ctx,n,x,new_x)){
		return A4SQP_FALSE;
	}
	if(m != sys->view.n_rel || nele_jac != sys->view.jac_nnz){
		return A4SQP_FALSE;
	}
	for(row = 0; row < sys->view.n_rel; ++row){
		for(k = sys->view.jac_row_start[row]; k < sys->view.jac_row_start[row + 1]; ++k){
			if(iRow != NULL){
				iRow[k] = row;
			}
			if(jCol != NULL){
				jCol[k] = sys->view.jac_col_index[k];
			}
			if(values != NULL){
				values[k] = sys->view.jac_value[k];
			}
		}
	}
	return A4SQP_TRUE;
}

static int asc_a4sqp_eval_relation_hessian_entries(
	void *vctx,
	int32 relation_index,
	void *entry_ctx,
	A4SqpHessianEntryFn entry
){
	struct A4SqpAscendProblemCtx *ctx = (struct A4SqpAscendProblemCtx *)vctx;
	struct rel_relation *rel;
	const struct var_variable **incidence;
	real64 *row2nd = NULL;
	real64 sign = 1.0;
	int32 len;
	int32 i;
	int32 j;
	int safe;
	if(ctx == NULL || ctx->sys == NULL || entry == NULL){
		return 1;
	}
	if(relation_index < 0){
		rel = (struct rel_relation *)ctx->sys->view.obj;
		if(ctx->sys->view.obj_direction > 0){
			sign = -1.0;
		}
	}else if(relation_index < ctx->sys->view.n_rel){
		rel = (struct rel_relation *)ctx->sys->view.rels[relation_index];
	}else{
		return 1;
	}
	if(rel == NULL){
		return 0;
	}
	len = rel_n_incidences(rel);
	if(len <= 0){
		return 0;
	}
	incidence = (const struct var_variable **)rel_incidence_list(rel);
	if(incidence == NULL){
		return 1;
	}
	row2nd = ASC_NEW_ARRAY_OR_NULL(real64,len);
	if(row2nd == NULL){
		return 1;
	}
	safe = SLV_PARAM_BOOL(&ctx->sys->params,A4SQP_PARAM_SAFE_CALC);
	for(i = 0; i < len; ++i){
		int32 row = a4sqp_view_var_col_from_var(&ctx->sys->view,(const struct var_variable *)incidence[i]);
		if(safe){
			enum safe_err serr = RelationCalcSecondDerivSafe(rel_instance(rel),row2nd,(unsigned long)i);
			if(serr != safe_ok){
				safe_error_to_stderr(&serr);
				ASC_FREE(row2nd);
				return 1;
			}
		}else if(RelationCalcSecondDeriv(rel_instance(rel),row2nd,(unsigned long)i)){
			ASC_FREE(row2nd);
			return 1;
		}
		if(row < 0 || row >= ctx->sys->view.n_var){
			continue;
		}
		for(j = 0; j <= i; ++j){
			int32 col = a4sqp_view_var_col_from_var(&ctx->sys->view,(const struct var_variable *)incidence[j]);
			real64 value;
			if(col < 0 || col >= ctx->sys->view.n_var){
				continue;
			}
			value = sign * row2nd[j];
			if(entry(entry_ctx,row,col,value)){
				ASC_FREE(row2nd);
				return 1;
			}
		}
	}
	ASC_FREE(row2nd);
	return 0;
}

static A4SqpBool asc_a4sqp_eval_h(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber obj_factor,
	A4SqpIndex m,
	A4SqpNumber *lambda,
	A4SqpBool new_lambda,
	A4SqpIndex nele_hess,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
){
	struct A4SqpAscendProblemCtx *ctx = (struct A4SqpAscendProblemCtx *)user_data;
	(void)new_lambda;
	if(ctx == NULL || ctx->sys == NULL || n < 0 || nele_hess != a4sqp_hessian_lower_triangle_nnz(n)){
		return A4SQP_FALSE;
	}
	if(values == NULL){
		if(a4sqp_hessian_lower_triangle_structure(n,iRow,jCol)){
			return A4SQP_FALSE;
		}
		return A4SQP_TRUE;
	}
	if(asc_a4sqp_refresh_view(ctx,n,x,new_x) || m != ctx->sys->view.n_rel){
		return A4SQP_FALSE;
	}
	return a4sqp_hessian_build_dense_lower_from_relations(
		n,
		m,
		obj_factor,
		lambda,
		asc_a4sqp_eval_relation_hessian_entries,
		ctx,
		values
	) ? A4SQP_FALSE : A4SQP_TRUE;
}

static A4SqpBool asc_a4sqp_intermediate_cb(
	A4SqpIndex alg_mod,
	A4SqpIndex iter_count,
	A4SqpNumber obj_value,
	A4SqpNumber inf_pr,
	A4SqpNumber inf_du,
	A4SqpNumber mu,
	A4SqpNumber d_norm,
	A4SqpNumber regularization_size,
	A4SqpNumber alpha_du,
	A4SqpNumber alpha_pr,
	A4SqpIndex ls_trials,
	A4SqpUserDataPtr user_data
){
	struct A4SqpAscendProblemCtx *ctx = (struct A4SqpAscendProblemCtx *)user_data;
	(void)obj_value;
	(void)inf_du;
	(void)mu;
	(void)regularization_size;
	(void)alpha_du;
	(void)ls_trials;
	if(ctx == NULL || ctx->sys == NULL){
		return A4SQP_FALSE;
	}
	ctx->sys->status.iteration = iter_count;
	ctx->sys->last_phase = alg_mod == A4SqpRestorationPhaseMode
		? A4SQP_CORE_PHASE_RESTORATION
		: A4SQP_CORE_PHASE_REGULAR;
	ctx->sys->last_violation_max = inf_pr;
	ctx->sys->last_dual_infeasibility = inf_du;
	ctx->sys->last_complementarity = mu;
	ctx->sys->last_kkt_error = inf_pr;
	ctx->sys->bound_stats.stationarity_inf = inf_du;
	ctx->sys->bound_stats.worst_index = -1;
	if(ctx->sys->last_dual_infeasibility > ctx->sys->last_kkt_error){
		ctx->sys->last_kkt_error = ctx->sys->last_dual_infeasibility;
	}
	if(ctx->sys->last_complementarity > ctx->sys->last_kkt_error){
		ctx->sys->last_kkt_error = ctx->sys->last_complementarity;
	}
	ctx->sys->last_step_norm = d_norm;
	ctx->sys->last_regularization_size = regularization_size;
	ctx->sys->last_alpha = alpha_pr;
	{
		real64 feas_tol = SLV_PARAM_REAL(&ctx->sys->params,A4SQP_PARAM_FEAS_TOL);
		(void)a4sqp_core_rel_stats(
			&ctx->sys->view,
			feas_tol,
			fmax(10.0 * feas_tol,1e-8),
			&ctx->sys->rel_stats
		);
	}
	if(SLV_PARAM_BOOL(&ctx->sys->params,A4SQP_PARAM_PROGRESS_CALLBACKS)){
		asc_a4sqp_report_iteration(ctx->sys);
	}
	return A4SQP_TRUE;
}

static int asc_a4sqp_set_core_options(A4SqpProblem problem, struct A4SqpSystem *sys){
	A4SqpIndex i;
	A4SqpIndex count;
	if(problem == NULL || sys == NULL){
		return 1;
	}
	count = GetA4SqpOptionCount();
	if(A4SQP_PARAM_CORE_BASE + count != A4SQP_PARAM_COUNT){
		return 1;
	}
	for(i = 0; i < count; ++i){
		struct A4SqpOptionInfo info;
		int pindex = A4SQP_PARAM_CORE_BASE + i;
		A4SqpBool ok = A4SQP_FALSE;
		if(!GetA4SqpOptionInfo(i,&info)){
			return 1;
		}
		if(!info.problem_option){
			continue;
		}
		switch(info.type){
		case A4SqpOptionInteger:
			ok = AddA4SqpIntOption(problem,(char *)info.keyword,SLV_PARAM_INT(&sys->params,pindex));
			break;
		case A4SqpOptionBool:
			ok = AddA4SqpIntOption(problem,(char *)info.keyword,SLV_PARAM_BOOL(&sys->params,pindex));
			break;
		case A4SqpOptionNumber:
			ok = AddA4SqpNumOption(problem,(char *)info.keyword,SLV_PARAM_REAL(&sys->params,pindex));
			break;
		case A4SqpOptionString:
			ok = AddA4SqpStrOption(problem,(char *)info.keyword,SLV_PARAM_CHAR(&sys->params,pindex));
			break;
		default:
			return 1;
		}
		if(!ok){
			return 1;
		}
	}
	return 0;
}

static void asc_a4sqp_apply_solve_stats(
	struct A4SqpSystem *sys,
	const struct A4SqpSolveStats *stats
){
	if(sys == NULL || stats == NULL){
		return;
	}
	sys->status.iteration = stats->iterations;
	sys->last_phase = stats->algorithm_mode == A4SqpRestorationPhaseMode
		? A4SQP_CORE_PHASE_RESTORATION
		: A4SQP_CORE_PHASE_REGULAR;
	sys->phase_switches = stats->mode_switches;
	sys->regular_iterations = stats->regular_iterations;
	sys->restoration_iterations = stats->restoration_iterations;
	sys->restoration_entries = stats->restoration_entries;
	sys->restoration_exits = stats->restoration_exits;
	sys->restoration_handoffs = stats->restoration_handoffs;
	sys->last_violation_max = stats->max_constraint_violation;
	sys->last_dual_infeasibility = stats->dual_infeasibility_inf;
	sys->last_complementarity = stats->complementarity_inf;
	sys->last_kkt_error = stats->kkt_error;
	sys->last_step_norm = stats->final_step_norm;
	sys->trust_radius = stats->final_trust_radius;
	sys->last_regularization_size = stats->regularization_size;
	{
		real64 feas_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
		(void)a4sqp_core_rel_stats(
			&sys->view,
			feas_tol,
			fmax(10.0 * feas_tol,1e-8),
			&sys->rel_stats
		);
	}
}

struct A4SqpAscendLsqCtx {
	slv_system_t server;
	struct A4SqpSystem *sys;
	const struct system_lsq_view *lsq;
};

static int asc_a4sqp_lsq_col_from_sindex(const struct A4SqpView *view, int sindex){
	int32 i;
	if(view == NULL || view->var_sindex == NULL){
		return -1;
	}
	for(i = 0; i < view->n_var; ++i){
		if(view->var_sindex[i] == sindex){
			return i;
		}
	}
	return -1;
}

static int asc_a4sqp_lsq_eval_residuals(void *userdata, const real64 *x, real64 *residuals){
	struct A4SqpAscendLsqCtx *ctx = (struct A4SqpAscendLsqCtx *)userdata;
	int32 i;
	if(ctx == NULL || ctx->sys == NULL || residuals == NULL){
		return 1;
	}
	if(x == NULL || ctx->sys->view.vars == NULL || ctx->sys->view.n_var != ctx->sys->x_n){
		return 1;
	}
	for(i = 0; i < ctx->sys->view.n_var; ++i){
		var_set_value((struct var_variable *)ctx->sys->view.vars[i],x[i]);
	}
	return system_lsq_eval_residuals(ctx->server,residuals);
}

static int asc_a4sqp_lsq_eval_jacobian_row(
	void *userdata,
	int32 row,
	int32 capacity,
	int32 *columns,
	real64 *values,
	int32 *nnz
){
	struct A4SqpAscendLsqCtx *ctx = (struct A4SqpAscendLsqCtx *)userdata;
	int *sindex_columns = NULL;
	unsigned long row_nnz = 0;
	unsigned long i;
	if(ctx == NULL || ctx->sys == NULL || nnz == NULL || capacity < 0){
		return 1;
	}
	*nnz = 0;
	if(capacity > 0){
		sindex_columns = ASC_NEW_ARRAY_OR_NULL(int,capacity);
		if(sindex_columns == NULL){
			return 1;
		}
	}
	if(system_lsq_eval_jacobian_row(
		ctx->server,
		(unsigned long)row,
		sindex_columns,
		values,
		(unsigned long)capacity,
		&row_nnz
	)){
		ASC_FREE(sindex_columns);
		return 1;
	}
	if(row_nnz > (unsigned long)capacity){
		ASC_FREE(sindex_columns);
		return 1;
	}
	for(i = 0; i < row_nnz; ++i){
		int col = asc_a4sqp_lsq_col_from_sindex(&ctx->sys->view,sindex_columns[i]);
		if(col < 0){
			ASC_FREE(sindex_columns);
			return 1;
		}
		columns[i] = col;
	}
	*nnz = (int32)row_nnz;
	ASC_FREE(sindex_columns);
	return 0;
}

static int asc_a4sqp_lsq_progress(void *userdata, const struct A4SqpLsqIteration *iteration){
	struct A4SqpAscendLsqCtx *ctx = (struct A4SqpAscendLsqCtx *)userdata;
	char message[256];
	if(ctx == NULL || ctx->sys == NULL || iteration == NULL){
		return 1;
	}
	ctx->sys->status.iteration = iteration->iter;
	ctx->sys->last_phase = A4SQP_CORE_PHASE_REGULAR;
	ctx->sys->last_merit_after = iteration->objective;
	ctx->sys->last_kkt_error = iteration->grad_inf;
	ctx->sys->last_dual_infeasibility = iteration->grad_inf;
	ctx->sys->last_step_norm = iteration->step_norm;
	ctx->sys->last_regularization_size = iteration->lambda;
	ctx->sys->last_alpha = iteration->alpha;
	snprintf(
		message,
		sizeof(message),
		"lsq_iter=%ld obj=%g grad=%g lambda=%g alpha=%g step=%g accepted=%d",
		(long)iteration->iter,
		iteration->objective,
		iteration->grad_inf,
		iteration->lambda,
		iteration->alpha,
		iteration->step_norm,
		iteration->accepted
	);
	a4sqp_report_progress(&ctx->sys->params,message);
	return 0;
}

static int asc_a4sqp_try_lsq_solve(slv_system_t server, struct A4SqpSystem *sys){
	const char *mode_name;
	struct RelationLeastSquaresAnalysis analysis;
	const struct system_lsq_view *lsq;
	struct A4SqpAscendLsqCtx ctx;
	struct A4SqpLsqProblem problem;
	struct A4SqpLsqOptions options;
	struct A4SqpLsqStats stats;
	real64 *weights = NULL;
	enum A4SqpLsqStatus status;
	unsigned long i;

	if(sys == NULL || server == NULL){
		return -1;
	}
	mode_name = SLV_PARAM_CHAR(&sys->params,A4SQP_PARAM_TRY_LSQ);
	if(mode_name == NULL || strcmp(mode_name,"OFF") == 0){
		return -1;
	}
	if(sys->view.obj == NULL || sys->view.n_rel != 0){
		return -1;
	}
	if(!system_analyse_lsq_objective(server,SYSTEM_LSQ_ANALYSE_BUILD_VIEW,&analysis)){
		return -1;
	}
	lsq = system_get_lsq_view(server);
	if(lsq == NULL || lsq->nresiduals == 0){
		return -1;
	}
	weights = ASC_NEW_ARRAY_OR_NULL(real64,lsq->nresiduals);
	if(weights == NULL){
		return 1;
	}
	for(i = 0; i < lsq->nresiduals; ++i){
		weights[i] = lsq->residuals[i].weight;
	}

	memset(&ctx,0,sizeof(ctx));
	ctx.server = server;
	ctx.sys = sys;
	ctx.lsq = lsq;

	memset(&problem,0,sizeof(problem));
	problem.n_var = sys->view.n_var;
	problem.n_res = (int32)lsq->nresiduals;
	problem.weights = weights;
	problem.x_lower = sys->view.var_lower;
	problem.x_upper = sys->view.var_upper;
	problem.userdata = &ctx;
	problem.eval_residuals = asc_a4sqp_lsq_eval_residuals;
	problem.eval_jacobian_row = asc_a4sqp_lsq_eval_jacobian_row;
	problem.progress = SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_PROGRESS_CALLBACKS)
		? asc_a4sqp_lsq_progress
		: NULL;

	memset(&options,0,sizeof(options));
	options.mode = strcmp(mode_name,"LM") == 0 ? A4SQP_LSQ_MODE_LM : A4SQP_LSQ_MODE_GAUSS;
	options.max_iter = SLV_PARAM_INT(&sys->params,A4SQP_PARAM_LSQ_MAX_ITER);
	if(options.max_iter <= 0){
		options.max_iter = SLV_PARAM_INT(&sys->params,A4SQP_PARAM_MAX_ITER);
	}
	options.max_backtrack = SLV_PARAM_INT(&sys->params,A4SQP_PARAM_MAX_BACKTRACK);
	options.grad_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
	options.step_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_STEP_TOL);

	memset(&stats,0,sizeof(stats));
	status = a4sqp_lsq_solve(&problem,&options,sys->x,&stats);
	ASC_FREE(weights);
	if(a4sqp_x_push_to_ascend(sys,sys->x) || asc_a4sqp_build_view(sys,server)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to sync the final least-squares iterate back to ASCEND.");
		return 1;
	}
	a4sqp_update_metrics(sys);
	sys->status.iteration = stats.iterations;
	sys->last_merit_after = stats.objective;
	sys->last_kkt_error = stats.grad_inf;
	sys->last_dual_infeasibility = stats.grad_inf;
	sys->last_step_norm = stats.step_norm;
	sys->last_regularization_size = stats.lambda;
	if(status == A4SQP_LSQ_SOLVED){
		sys->status.converged = TRUE;
		sys->status.diverged = FALSE;
		sys->status.iteration_limit_exceeded = FALSE;
		sys->status.ready_to_solve = FALSE;
		asc_a4sqp_report_view(sys);
		return 0;
	}
	if(status == A4SQP_LSQ_MAX_ITER || status == A4SQP_LSQ_LINEAR_ERROR){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,
			"A4SQP least-squares attempt using %s did not converge; falling back to SQP.",
			mode_name
		);
		return -1;
	}
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,
		"A4SQP least-squares attempt using %s failed with status %d; falling back to SQP.",
		mode_name,
		(int)status
	);
	return -1;
}

static int a4sqp_presolve(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	int32 sorted_rels = 0;
	int32 sorted_vars = 0;

	if(sys == NULL){
		return 1;
	}

	sys->server = server;
	a4sqp_init_status(sys);

	MSG("Presolve...");

	slv_sort_rels_and_vars(server,&sorted_rels,&sorted_vars);
	if(sorted_vars < 0 || sorted_rels < 0){
		sys->status.ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to sort ASCEND solver variable/relation lists.");
		return 1;
	}
	if(asc_a4sqp_build_view(sys,server)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to build the ASCEND problem view.");
		return 1;
	}
	if(a4sqp_x_sync_from_view(sys)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to initialize the core iterate vector.");
		return 1;
	}
	if(sys->view.unsupported_rels > 0){
		sys->status.ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP Phase 1 does not support %ld relation(s) in the selected solver list.",
			(long)sys->view.unsupported_rels
		);
		asc_a4sqp_report_view(sys);
		return 1;
	}

	if(sys->view.calc_errors > 0){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP encountered residual evaluation errors in %ld relation(s).",
			(long)sys->view.calc_errors
		);
		asc_a4sqp_report_view(sys);
		return 1;
	}

	if(sys->view.derivative_errors > 0){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP encountered derivative evaluation errors in %ld relation(s).",
			(long)sys->view.derivative_errors
		);
		asc_a4sqp_report_view(sys);
		return 1;
	}

	sys->status.ready_to_solve = TRUE;
	a4sqp_update_metrics(sys);
	asc_a4sqp_report_view(sys);
	MSG("Presolve completed");
	return 0;
}

static int a4sqp_solve(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	struct A4SqpAscendProblemCtx ctx;
	struct A4SqpSolveStats stats;
	A4SqpProblem problem = NULL;
	enum A4SqpApplicationReturnStatus solve_status;
	real64 obj_value = 0.0;
	real64 *g = NULL;
	int hess_nnz;

	if(sys == NULL){
		return 1;
	}
	if(!sys->status.ready_to_solve){
		if(a4sqp_presolve(server,asys)){
			return 1;
		}
	}

	memset(&ctx,0,sizeof(ctx));
	memset(&stats,0,sizeof(stats));
	ctx.server = server;
	ctx.sys = sys;

	{
		int lsq_status = asc_a4sqp_try_lsq_solve(server,sys);
		if(lsq_status == 0){
			return 0;
		}
		if(lsq_status > 0){
			return 1;
		}
	}

	hess_nnz = sys->view.obj != NULL ? a4sqp_hessian_lower_triangle_nnz(sys->view.n_var) : 0;
	if(sys->view.n_rel > 0){
		g = ASC_NEW_ARRAY_OR_NULL(real64,sys->view.n_rel);
		if(g == NULL){
			sys->status.ok = FALSE;
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to allocate constraint result storage.");
			return 1;
		}
	}
	problem = CreateA4SqpProblem(
		sys->view.n_var,
		sys->view.var_lower,
		sys->view.var_upper,
		sys->view.n_rel,
		sys->view.rel_lower,
		sys->view.rel_upper,
		sys->view.jac_nnz,
		hess_nnz,
		0,
		sys->view.obj != NULL ? asc_a4sqp_eval_f : NULL,
		sys->view.n_rel > 0 ? asc_a4sqp_eval_g : NULL,
		sys->view.obj != NULL ? asc_a4sqp_eval_grad_f : NULL,
		sys->view.n_rel > 0 ? asc_a4sqp_eval_jac_g : NULL,
		sys->view.obj != NULL ? asc_a4sqp_eval_h : NULL
	);
	if(problem == NULL){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ASC_FREE(g);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to create the core C API problem.");
		return 1;
	}
	asc_a4sqp_set_core_options(problem,sys);
	SetA4SqpProblemScaling(problem,1.0,sys->view.var_scale,sys->view.rel_scale);
	SetA4SqpIntermediateCallback(problem,asc_a4sqp_intermediate_cb);
	solve_status = A4SqpSolve(problem,sys->x,g,&obj_value,NULL,NULL,NULL,&ctx);
	GetA4SqpSolveStatistics(problem,&stats);
	asc_a4sqp_apply_solve_stats(sys,&stats);
	FreeA4SqpProblem(problem);
	if(a4sqp_x_push_to_ascend(sys,sys->x) || asc_a4sqp_build_view(sys,server)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ASC_FREE(g);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to sync the final core solution back to ASCEND.");
		return 1;
	}
	a4sqp_update_metrics(sys);
	asc_a4sqp_report_view(sys);
	ASC_FREE(g);

	if(solve_status == A4SqpSolveSucceeded || solve_status == A4SqpSolvedToAcceptableLevel){
		sys->status.converged = TRUE;
		sys->status.diverged = FALSE;
		sys->status.iteration_limit_exceeded = FALSE;
		sys->status.ready_to_solve = FALSE;
		sys->solved_acceptable = solve_status == A4SqpSolvedToAcceptableLevel;
		if(sys->solved_acceptable){
			ERROR_REPORTER_HERE(ASC_PROG_NOTE,"A4SQP stopped at acceptable level.");
		}
		return 0;
	}
	if(solve_status == A4SqpMaximumIterationsExceeded){
		sys->status.converged = FALSE;
		sys->status.diverged = FALSE;
		sys->status.iteration_limit_exceeded = TRUE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,
			"A4SQP reached the maximum iteration count before satisfying convergence tests."
		);
		return 0;
	}
	if(solve_status == A4SqpInvalidNumberDetected){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP callback evaluation failed or returned an invalid number.");
		return 1;
	}
	{
		sys->status.converged = FALSE;
		sys->status.diverged = TRUE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP core solve failed with status %d.",(int)solve_status);
		return 1;
	}
}

static int a4sqp_iterate(slv_system_t server, SlvClientToken asys){
	return a4sqp_solve(server,asys);
}

static int a4sqp_resolve(slv_system_t server, SlvClientToken asys){
	return a4sqp_solve(server,asys);
}

static void a4sqp_dumpinternals(slv_system_t server, SlvClientToken asys, int level){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || level <= 0){
		return;
	}
	asc_a4sqp_report_view(sys);
}

static const SlvFunctionsT a4sqp_internals = {
	A4SQP_SOLVER_NUMBER,
	A4SQP_SOLVER_NAME,
	a4sqp_create,
	a4sqp_destroy,
	a4sqp_eligible,
	a4sqp_get_default_parameters,
	a4sqp_get_parameters,
	a4sqp_set_parameters,
	a4sqp_get_status,
	a4sqp_solve,
	a4sqp_presolve,
	a4sqp_iterate,
	a4sqp_resolve,
	NULL,
	NULL,
	a4sqp_dumpinternals
};

ASC_EXPORT int a4sqp_register(void){
	return solver_register(&a4sqp_internals);
}
