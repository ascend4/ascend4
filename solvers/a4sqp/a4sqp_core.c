/*
 * Shared SQP core utilities for A4SQP frontends.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_core.h"

#include "a4sqp_view.h"

#include <math.h>
#include <string.h>

int a4sqp_core_is_lower_inf(real64 value){
	return value <= A4SQP_NO_LOWER_BOUND / 10.0;
}

int a4sqp_core_is_upper_inf(real64 value){
	return value >= A4SQP_NO_UPPER_BOUND / 10.0;
}

real64 a4sqp_core_violation(
	const struct A4SqpCoreView *view,
	real64 *max_violation,
	int32 *worst_rel
){
	int32 i;
	real64 violation = 0.0;
	real64 vmax = 0.0;
	int32 worst = -1;
	if(view == NULL){
		if(max_violation != NULL){
			*max_violation = 0.0;
		}
		if(worst_rel != NULL){
			*worst_rel = -1;
		}
		return 0.0;
	}
	for(i = 0; i < view->n_rel; ++i){
		real64 row_violation = 0.0;
		if(!a4sqp_core_is_lower_inf(view->scaled_rel_lower[i])
			&& view->scaled_rel_residual[i] < view->scaled_rel_lower[i]
		){
			row_violation += view->scaled_rel_lower[i] - view->scaled_rel_residual[i];
		}
		if(!a4sqp_core_is_upper_inf(view->scaled_rel_upper[i])
			&& view->scaled_rel_residual[i] > view->scaled_rel_upper[i]
		){
			row_violation += view->scaled_rel_residual[i] - view->scaled_rel_upper[i];
		}
		violation += row_violation;
		if(row_violation > vmax){
			vmax = row_violation;
			worst = i;
		}
	}
	if(max_violation != NULL){
		*max_violation = vmax;
	}
	if(worst_rel != NULL){
		*worst_rel = worst;
	}
	return violation;
}

real64 a4sqp_core_view_violation(
	const struct A4SqpView *view,
	real64 *max_violation,
	int32 *worst_rel
){
	struct A4SqpCoreView core;
	a4sqp_view_get_core(view,&core);
	return a4sqp_core_violation(&core,max_violation,worst_rel);
}

real64 a4sqp_core_merit(
	const struct A4SqpCoreView *view,
	real64 penalty
){
	real64 violation = a4sqp_core_violation(view,NULL,NULL);
	if(view == NULL){
		return 0.0;
	}
	return (view->has_objective ? view->obj_value : 0.0) + penalty * violation;
}

real64 a4sqp_core_view_merit(
	const struct A4SqpView *view,
	int has_objective,
	real64 penalty
){
	struct A4SqpCoreView core;
	a4sqp_view_get_core(view,&core);
	core.has_objective = has_objective;
	return a4sqp_core_merit(&core,penalty);
}

real64 a4sqp_core_projected_gradient_inf_for_view(
	const struct A4SqpCoreView *view,
	real64 active_tol
){
	real64 *basis = NULL;
	real64 *normal = NULL;
	real64 *proj = NULL;
	int32 basis_count = 0;
	int32 n;
	int32 i;
	int32 row;
	real64 proj_inf = 0.0;
	const real64 eps = 1e-12;

	if(view == NULL || !view->has_objective){
		return 0.0;
	}
	n = view->n_var;
	if(n <= 0){
		return 0.0;
	}

	basis = A4SQP_NEW_ARRAY_CLEAR(real64,(size_t)n * (size_t)n);
	normal = A4SQP_NEW_ARRAY_CLEAR(real64,n);
	proj = A4SQP_NEW_ARRAY_CLEAR(real64,n);
	if(basis == NULL || normal == NULL || proj == NULL){
		A4SQP_FREE(basis);
		A4SQP_FREE(normal);
		A4SQP_FREE(proj);
		return 0.0;
	}

	for(i = 0; i < n; ++i){
		proj[i] = view->scaled_obj_gradient != NULL ? view->scaled_obj_gradient[i] : 0.0;
	}

	for(row = 0; row < view->n_rel; ++row){
		int include = 0;
		int32 k;
		memset(normal,0,sizeof(real64) * (size_t)n);
		if(view->rel_kind != NULL && view->rel_kind[row] == A4SQP_REL_KIND_EQUALITY){
			include = 1;
		}else{
			if(
				!a4sqp_core_is_lower_inf(view->scaled_rel_lower[row])
				&& view->scaled_rel_residual[row] <= view->scaled_rel_lower[row] + active_tol
			){
				include = 1;
			}
			if(
				!a4sqp_core_is_upper_inf(view->scaled_rel_upper[row])
				&& view->scaled_rel_residual[row] >= view->scaled_rel_upper[row] - active_tol
			){
				include = 1;
			}
		}
		if(!include){
			continue;
		}
		for(k = view->jac_row_start[row]; k < view->jac_row_start[row + 1]; ++k){
			int32 col = view->jac_col_index != NULL ? view->jac_col_index[k] : -1;
			if(col >= 0){
				normal[col] = view->scaled_jac_value[k];
			}
		}
		for(i = 0; i < basis_count; ++i){
			int32 j;
			real64 dot = 0.0;
			for(j = 0; j < n; ++j){
				dot += basis[i * n + j] * normal[j];
			}
			for(j = 0; j < n; ++j){
				normal[j] -= dot * basis[i * n + j];
			}
		}
		{
			real64 norm2 = 0.0;
			int32 j;
			for(j = 0; j < n; ++j){
				norm2 += normal[j] * normal[j];
			}
			if(norm2 > eps && basis_count < n){
				real64 inv_norm = 1.0 / sqrt(norm2);
				for(j = 0; j < n; ++j){
					basis[basis_count * n + j] = normal[j] * inv_norm;
				}
				++basis_count;
			}
		}
	}

	for(i = 0; i < basis_count; ++i){
		int32 j;
		real64 dot = 0.0;
		for(j = 0; j < n; ++j){
			dot += basis[i * n + j] * proj[j];
		}
		for(j = 0; j < n; ++j){
			proj[j] -= dot * basis[i * n + j];
		}
	}

	for(i = 0; i < n; ++i){
		real64 grad = proj[i];
		real64 value = view->scaled_var_value[i];
		real64 lower = view->scaled_var_lower[i];
		real64 upper = view->scaled_var_upper[i];
		int at_lower = !a4sqp_core_is_lower_inf(lower) && value <= lower + active_tol;
		int at_upper = !a4sqp_core_is_upper_inf(upper) && value >= upper - active_tol;
		if((at_lower && grad > 0.0) || (at_upper && grad < 0.0)){
			grad = 0.0;
		}
		if(fabs(grad) > proj_inf){
			proj_inf = fabs(grad);
		}
	}

	A4SQP_FREE(basis);
	A4SQP_FREE(normal);
	A4SQP_FREE(proj);
	return proj_inf;
}

real64 a4sqp_core_projected_gradient_inf(
	const struct A4SqpView *view,
	int has_objective,
	real64 active_tol
){
	struct A4SqpCoreView core;
	a4sqp_view_get_core(view,&core);
	core.has_objective = has_objective;
	return a4sqp_core_projected_gradient_inf_for_view(&core,active_tol);
}

static real64 a4sqp_core_bound_stationarity_residual(
	real64 grad,
	real64 value,
	real64 lower,
	real64 upper,
	real64 active_tol
){
	int at_lower = !a4sqp_core_is_lower_inf(lower) && value <= lower + active_tol;
	int at_upper = !a4sqp_core_is_upper_inf(upper) && value >= upper - active_tol;
	if(at_lower && at_upper){
		return 0.0;
	}
	if(at_lower){
		return grad < 0.0 ? -grad : 0.0;
	}
	if(at_upper){
		return grad > 0.0 ? grad : 0.0;
	}
	return fabs(grad);
}

static real64 a4sqp_core_kkt_dual_inf_for_sign(
	const struct A4SqpCoreView *view,
	const real64 *row_dual,
	real64 active_tol,
	real64 row_sign
){
	real64 *lag_grad = NULL;
	real64 dual_inf = 0.0;
	int32 i;
	int32 row;
	if(view == NULL || view->n_var <= 0){
		return 0.0;
	}
	lag_grad = A4SQP_NEW_ARRAY_CLEAR(real64,view->n_var);
	if(lag_grad == NULL){
		return 0.0;
	}
	for(i = 0; i < view->n_var; ++i){
		lag_grad[i] = (view->has_objective && view->scaled_obj_gradient != NULL)
			? view->scaled_obj_gradient[i]
			: 0.0;
	}
	if(row_dual != NULL && view->jac_row_start != NULL && view->jac_col_index != NULL && view->scaled_jac_value != NULL){
		for(row = 0; row < view->n_rel; ++row){
			int32 k;
			real64 lambda = row_sign * row_dual[row];
			if(!isfinite(lambda) || fabs(lambda) <= 0.0){
				continue;
			}
			for(k = view->jac_row_start[row]; k < view->jac_row_start[row + 1]; ++k){
				int32 col = view->jac_col_index[k];
				if(col >= 0 && col < view->n_var){
					lag_grad[col] += lambda * view->scaled_jac_value[k];
				}
			}
		}
	}
	for(i = 0; i < view->n_var; ++i){
		real64 residual = a4sqp_core_bound_stationarity_residual(
			lag_grad[i],
			view->scaled_var_value[i],
			view->scaled_var_lower[i],
			view->scaled_var_upper[i],
			active_tol
		);
		if(residual > dual_inf){
			dual_inf = residual;
		}
	}
	A4SQP_FREE(lag_grad);
	return dual_inf;
}

static real64 a4sqp_core_kkt_complementarity_inf(
	const struct A4SqpCoreView *view,
	const real64 *row_dual
){
	real64 comp_inf = 0.0;
	int32 row;
	if(view == NULL || row_dual == NULL){
		return 0.0;
	}
	for(row = 0; row < view->n_rel; ++row){
		real64 lambda = fabs(row_dual[row]);
		real64 gap = HUGE_VAL;
		real64 comp;
		if(view->rel_kind != NULL && view->rel_kind[row] == A4SQP_REL_KIND_EQUALITY){
			continue;
		}
		if(!a4sqp_core_is_lower_inf(view->scaled_rel_lower[row])){
			real64 lower_gap = fabs(view->scaled_rel_residual[row] - view->scaled_rel_lower[row]);
			if(lower_gap < gap){
				gap = lower_gap;
			}
		}
		if(!a4sqp_core_is_upper_inf(view->scaled_rel_upper[row])){
			real64 upper_gap = fabs(view->scaled_rel_upper[row] - view->scaled_rel_residual[row]);
			if(upper_gap < gap){
				gap = upper_gap;
			}
		}
		if(gap == HUGE_VAL || !isfinite(lambda)){
			continue;
		}
		comp = lambda * gap;
		if(comp > comp_inf){
			comp_inf = comp;
		}
	}
	return comp_inf;
}

real64 a4sqp_core_kkt_error_for_view(
	const struct A4SqpCoreView *view,
	const real64 *row_dual,
	real64 active_tol,
	struct A4SqpKktResidual *residual
){
	struct A4SqpKktResidual local;
	real64 dual_pos;
	real64 dual_neg;
	if(residual == NULL){
		residual = &local;
	}
	memset(residual,0,sizeof(*residual));
	if(view == NULL){
		return 0.0;
	}
	residual->primal_inf = 0.0;
	a4sqp_core_violation(view,&residual->primal_inf,NULL);
	dual_pos = a4sqp_core_kkt_dual_inf_for_sign(view,row_dual,active_tol,1.0);
	dual_neg = a4sqp_core_kkt_dual_inf_for_sign(view,row_dual,active_tol,-1.0);
	if(row_dual != NULL && dual_neg < dual_pos){
		residual->dual_inf = dual_neg;
		residual->lambda_sign = -1;
	}else{
		residual->dual_inf = dual_pos;
		residual->lambda_sign = 1;
	}
	residual->complementarity_inf = a4sqp_core_kkt_complementarity_inf(view,row_dual);
	residual->kkt_error = residual->primal_inf;
	if(residual->dual_inf > residual->kkt_error){
		residual->kkt_error = residual->dual_inf;
	}
	if(residual->complementarity_inf > residual->kkt_error){
		residual->kkt_error = residual->complementarity_inf;
	}
	return residual->kkt_error;
}

real64 a4sqp_core_kkt_error(
	const struct A4SqpView *view,
	int has_objective,
	const real64 *row_dual,
	real64 active_tol,
	struct A4SqpKktResidual *residual
){
	struct A4SqpCoreView core;
	a4sqp_view_get_core(view,&core);
	core.has_objective = has_objective;
	return a4sqp_core_kkt_error_for_view(&core,row_dual,active_tol,residual);
}

real64 a4sqp_core_qp_elastic_sum(const struct A4SqpQp *qp){
	int32 c;
	real64 violation = 0.0;
	if(qp == NULL){
		return 0.0;
	}
	for(c = 0; c < qp->num_col; ++c){
		if(qp->col_kind[c] == A4SQP_QP_COL_ELASTIC_LOWER
			|| qp->col_kind[c] == A4SQP_QP_COL_ELASTIC_UPPER
		){
			violation += qp->col_value[c];
		}
	}
	return violation;
}

real64 a4sqp_core_qp_elastic_max(const struct A4SqpQp *qp){
	int32 c;
	real64 maxv = 0.0;
	if(qp == NULL){
		return 0.0;
	}
	for(c = 0; c < qp->num_col; ++c){
		if(qp->col_kind[c] == A4SQP_QP_COL_ELASTIC_LOWER
			|| qp->col_kind[c] == A4SQP_QP_COL_ELASTIC_UPPER
		){
			if(fabs(qp->col_value[c]) > maxv){
				maxv = fabs(qp->col_value[c]);
			}
		}
	}
	return maxv;
}

void a4sqp_core_restoration_state_init(struct A4SqpCoreRestorationState *state){
	if(state == NULL){
		return;
	}
	state->best_violation = HUGE_VAL;
	state->stall_count = 0;
	state->active = 0;
	state->phase = A4SQP_CORE_PHASE_REGULAR;
}

static int a4sqp_core_restoration_choose(
	const struct A4SqpCoreView *view,
	const struct A4SqpCoreRestorationOptions *options,
	real64 feas_tol,
	struct A4SqpCoreRestorationState *state
){
	real64 max_violation = 0.0;
	real64 exit_tol;
	if(state == NULL){
		return 0;
	}
	if(options == NULL || !options->enable || view == NULL || view->n_rel <= 0){
		state->active = 0;
		return 0;
	}
	(void)a4sqp_core_violation(view,&max_violation,NULL);
	exit_tol = 10.0 * feas_tol;
	if(feas_tol > 0.0 && feas_tol < 1.0){
		exit_tol = fmax(exit_tol,sqrt(feas_tol));
	}
	if(max_violation <= exit_tol){
		a4sqp_core_restoration_state_init(state);
		return 0;
	}
	if(
		!isfinite(state->best_violation)
		|| max_violation <= (1.0 - options->improve) * state->best_violation
	){
		state->best_violation = max_violation;
		state->stall_count = 0;
		state->active = 0;
		return 0;
	}
	++state->stall_count;
	state->active = state->stall_count >= options->trigger_iter;
	return state->active;
}

enum A4SqpRowActivity a4sqp_core_row_activity_for_view(
	const struct A4SqpCoreView *view,
	int32 row,
	real64 active_tol,
	real64 near_tol
){
	real64 lower_gap = HUGE_VAL;
	real64 upper_gap = HUGE_VAL;
	if(view == NULL || row < 0 || row >= view->n_rel){
		return A4SQP_ROW_INACTIVE;
	}
	if(view->rel_kind != NULL && view->rel_kind[row] == A4SQP_REL_KIND_EQUALITY){
		return A4SQP_ROW_EQUALITY;
	}
	if(
		!a4sqp_core_is_lower_inf(view->scaled_rel_lower[row])
		&& isfinite(view->scaled_rel_residual[row])
	){
		lower_gap = view->scaled_rel_residual[row] - view->scaled_rel_lower[row];
	}
	if(
		!a4sqp_core_is_upper_inf(view->scaled_rel_upper[row])
		&& isfinite(view->scaled_rel_residual[row])
	){
		upper_gap = view->scaled_rel_upper[row] - view->scaled_rel_residual[row];
	}
	if(lower_gap <= active_tol || upper_gap <= active_tol){
		return A4SQP_ROW_ACTIVE;
	}
	if(lower_gap <= near_tol || upper_gap <= near_tol){
		return A4SQP_ROW_NEAR_ACTIVE;
	}
	return A4SQP_ROW_INACTIVE;
}

enum A4SqpRowActivity a4sqp_core_row_activity(
	const struct A4SqpView *view,
	int32 row,
	real64 active_tol,
	real64 near_tol
){
	struct A4SqpCoreView core;
	a4sqp_view_get_core(view,&core);
	return a4sqp_core_row_activity_for_view(&core,row,active_tol,near_tol);
}

int a4sqp_core_has_converged_for_view(
	const struct A4SqpCoreView *view,
	const struct A4SqpConvergencePolicy *policy,
	real64 *max_violation,
	real64 *projected_gradient_inf,
	int32 *worst_rel
){
	real64 maxvio = 0.0;
	real64 pg = 0.0;
	real64 feas_tol;
	real64 step_tol;
	real64 opt_tol;
	if(view == NULL || policy == NULL){
		return 0;
	}
	feas_tol = policy->feas_tol;
	step_tol = policy->step_tol;
	opt_tol = (step_tol > feas_tol) ? step_tol : feas_tol;
	a4sqp_core_violation(view,&maxvio,worst_rel);
	{
		struct A4SqpCoreView stationarity_view = *view;
		stationarity_view.has_objective = policy->has_objective;
		pg = a4sqp_core_projected_gradient_inf_for_view(&stationarity_view,feas_tol);
	}
	if(max_violation != NULL){
		*max_violation = maxvio;
	}
	if(projected_gradient_inf != NULL){
		*projected_gradient_inf = pg;
	}
	if(policy->has_objective && view->n_rel == 0){
		return maxvio <= feas_tol && pg <= feas_tol;
	}
	if(policy->has_objective){
		return maxvio <= feas_tol
			&& (
				pg <= opt_tol
				|| (
					policy->constrained_objective_allows_small_step
					&& policy->last_step_norm <= step_tol
				)
			);
	}
	return maxvio <= feas_tol && policy->last_step_norm <= step_tol;
}

int a4sqp_core_has_converged(
	const struct A4SqpView *view,
	const struct A4SqpConvergencePolicy *policy,
	real64 *max_violation,
	real64 *projected_gradient_inf,
	int32 *worst_rel
){
	struct A4SqpCoreView core;
	a4sqp_view_get_core(view,&core);
	return a4sqp_core_has_converged_for_view(&core,policy,max_violation,projected_gradient_inf,worst_rel);
}

int a4sqp_core_line_search_vector(
	struct A4SqpView *view,
	const struct A4SqpQp *qp,
	const struct A4SqpLineSearchOptions *options,
	const struct A4SqpVectorLineSearchOps *ops,
	void *ctx,
	real64 *x,
	int has_objective,
	struct A4SqpLineSearchResult *result
){
	struct A4SqpCoreView core;
	struct A4SqpCoreView current;
	real64 *old_x = NULL;
	real64 *old_scaled_x = NULL;
	real64 *old_scaled_grad = NULL;
	real64 *physical_step = NULL;
	real64 alpha = 1.0;
	real64 step_norm2 = 0.0;
	real64 scaled_step_inf = 0.0;
	real64 violation_before = 0.0;
	real64 max_violation_before = 0.0;
	int32 i;
	int trial;
	int accepted = 0;
	int n;
	if(result == NULL){
		return 1;
	}
	memset(result,0,sizeof(*result));
	if(view == NULL || qp == NULL || options == NULL || ops == NULL || x == NULL || view->n_var <= 0){
		return 1;
	}
	if(ops->evaluate == NULL){
		return 1;
	}
	a4sqp_view_get_core(view,&core);
	core.has_objective = has_objective;
	n = core.n_var;
	old_x = A4SQP_NEW_ARRAY_OR_NULL(real64,n);
	old_scaled_x = A4SQP_NEW_ARRAY_OR_NULL(real64,n);
	old_scaled_grad = A4SQP_NEW_ARRAY_OR_NULL(real64,n);
	physical_step = A4SQP_NEW_ARRAY_OR_NULL(real64,n);
	if(old_x == NULL || old_scaled_x == NULL || old_scaled_grad == NULL || physical_step == NULL){
		A4SQP_FREE(old_x);
		A4SQP_FREE(old_scaled_x);
		A4SQP_FREE(old_scaled_grad);
		A4SQP_FREE(physical_step);
		return 1;
	}
	for(i = 0; i < n; ++i){
		real64 qstep = qp->col_value[i];
		old_x[i] = x[i];
		old_scaled_x[i] = core.scaled_var_value[i];
		old_scaled_grad[i] = core.scaled_obj_gradient != NULL ? core.scaled_obj_gradient[i] : 0.0;
		physical_step[i] = core.var_scale[i] * qstep;
		step_norm2 += physical_step[i] * physical_step[i];
		if(fabs(qstep) > scaled_step_inf){
			scaled_step_inf = fabs(qstep);
		}
	}
	result->merit_before = a4sqp_core_merit(&core,options->elastic_penalty);
	violation_before = a4sqp_core_violation(&core,&max_violation_before,NULL);
	result->model_merit_after = (core.has_objective ? core.obj_value : 0.0) + qp->objective_value;
	result->predicted_reduction = result->merit_before - result->model_merit_after;
	result->step_norm = sqrt(step_norm2);
	result->scaled_step_inf = scaled_step_inf;
	if(view->n_rel > 0 && result->step_norm <= options->step_tol){
		real64 max_violation = 0.0;
		(void)a4sqp_core_violation(&core,&max_violation,NULL);
		if(max_violation <= options->feas_tol){
			accepted = 1;
			result->accepted = 1;
			result->merit_after = result->merit_before;
			result->alpha = 0.0;
			result->trust_ratio = 1.0;
			goto cleanup;
		}
	}
	if(fabs(result->predicted_reduction) <= options->merit_tol){
		accepted = 1;
		result->accepted = 1;
		result->merit_after = result->merit_before;
		result->trust_ratio = 1.0;
		goto cleanup;
	}
	for(trial = 0; trial < options->max_backtrack; ++trial){
		real64 merit_decrease;
		real64 required_decrease;
		real64 trust_ratio = 0.0;
		real64 trial_step_norm2 = 0.0;
		real64 violation_after = 0.0;
		real64 max_violation_after = 0.0;
		++result->trials;
		for(i = 0; i < n; ++i){
			x[i] = old_x[i] + alpha * physical_step[i];
		}
		if(ops->evaluate(ctx,x,view)){
			alpha *= 0.5;
			continue;
		}
		a4sqp_view_get_core(view,&current);
		current.has_objective = core.has_objective;
		result->merit_after = a4sqp_core_merit(&current,options->elastic_penalty);
		violation_after = a4sqp_core_violation(&current,&max_violation_after,NULL);
		merit_decrease = result->merit_before - result->merit_after;
		required_decrease = options->armijo_coeff * alpha * result->predicted_reduction;
		if(required_decrease < 0.0){
			required_decrease = 0.0;
		}
		if(alpha * result->predicted_reduction > options->merit_tol){
			trust_ratio = merit_decrease / (alpha * result->predicted_reduction);
		}
		if(
			options->restoration
			&& view->n_rel > 0
			&& violation_before > options->feas_tol
			&& (
				violation_after <= (1.0 - options->restoration_margin) * violation_before
				|| max_violation_after <= (1.0 - options->restoration_margin) * max_violation_before
			)
		){
			for(i = 0; i < n; ++i){
				real64 s = alpha * physical_step[i];
				trial_step_norm2 += s * s;
			}
			if(ops->accepted != NULL){
				ops->accepted(ctx,old_scaled_x,old_scaled_grad,options->restoration);
			}
			result->accepted = 1;
			result->alpha = alpha;
			result->step_norm = sqrt(trial_step_norm2);
			result->trust_ratio = trust_ratio;
			result->scaled_step_inf = alpha * scaled_step_inf;
			accepted = 1;
			break;
		}
		if(merit_decrease >= required_decrease && trust_ratio >= options->trust_accept){
			for(i = 0; i < n; ++i){
				real64 s = alpha * physical_step[i];
				trial_step_norm2 += s * s;
			}
			if(ops->accepted != NULL){
				ops->accepted(ctx,old_scaled_x,old_scaled_grad,options->restoration);
			}
			result->accepted = 1;
			result->alpha = alpha;
			result->step_norm = sqrt(trial_step_norm2);
			result->trust_ratio = trust_ratio;
			result->scaled_step_inf = alpha * scaled_step_inf;
			accepted = 1;
			break;
		}
		if(
			options->filter_accept
			&& view->n_rel > 0
			&& violation_before > options->feas_tol
			&& merit_decrease > options->merit_tol
			&& (
				violation_after <= (1.0 - options->filter_margin) * violation_before
				|| max_violation_after <= (1.0 - options->filter_margin) * max_violation_before
			)
		){
			for(i = 0; i < n; ++i){
				real64 s = alpha * physical_step[i];
				trial_step_norm2 += s * s;
			}
			if(ops->accepted != NULL){
				ops->accepted(ctx,old_scaled_x,old_scaled_grad,options->restoration);
			}
			result->accepted = 1;
			result->alpha = alpha;
			result->step_norm = sqrt(trial_step_norm2);
			result->trust_ratio = trust_ratio;
			result->scaled_step_inf = alpha * scaled_step_inf;
			accepted = 1;
			break;
		}
		alpha *= 0.5;
	}
	if(!accepted){
		for(i = 0; i < n; ++i){
			x[i] = old_x[i];
		}
		(void)ops->evaluate(ctx,x,view);
	}

cleanup:
	A4SQP_FREE(old_x);
	A4SQP_FREE(old_scaled_x);
	A4SQP_FREE(old_scaled_grad);
	A4SQP_FREE(physical_step);
	return accepted ? 0 : 1;
}

static int a4sqp_core_step_may_retry(
	const struct A4SqpView *view,
	int attempt,
	const struct A4SqpCoreStepOptions *options,
	const struct A4SqpCoreStepOps *ops,
	void *ctx,
	const char *reason,
	struct A4SqpCoreStepStats *stats
){
	if(view == NULL || options == NULL || ops == NULL || attempt >= options->trust_qp_retries){
		return 0;
	}
	if((view->n_rel <= 0 && !options->trust_unconstrained) || ops->shrink_trust == NULL){
		return 0;
	}
	if(ops->shrink_trust(ctx,reason)){
		return 0;
	}
	if(stats != NULL){
		++stats->trust_shrinks;
	}
	return 1;
}

static void a4sqp_core_step_record_zero_accept(
	struct A4SqpLineSearchResult *result,
	real64 current_merit
){
	if(result == NULL){
		return;
	}
	memset(result,0,sizeof(*result));
	result->accepted = 1;
	result->merit_before = current_merit;
	result->merit_after = current_merit;
	result->model_merit_after = current_merit;
	result->predicted_reduction = 0.0;
	result->alpha = 0.0;
	result->step_norm = 0.0;
	result->trust_ratio = 1.0;
	result->scaled_step_inf = 0.0;
}

enum A4SqpCoreStepStatus a4sqp_core_solve_step(
	struct A4SqpView *view,
	struct A4SqpQp *qp,
	real64 *trust_radius,
	const struct A4SqpCoreStepOptions *options,
	const struct A4SqpCoreStepOps *ops,
	void *ctx,
	const struct A4SqpLineSearchOptions *line_search_options,
	const struct A4SqpVectorLineSearchOps *line_search_ops,
	real64 *x,
	int has_objective,
	struct A4SqpLineSearchResult *line_search_result,
	struct A4SqpCoreStepStats *stats
){
	int attempt;
	struct A4SqpStepHessian step_hess;
	enum A4SqpCoreStepStatus last_error = A4SQP_CORE_STEP_QP_ERROR;
	int effective_has_objective;
	int restoration_active = 0;
	struct A4SqpCoreView initial_view;
	struct A4SqpLineSearchOptions effective_line_options;

	if(stats != NULL){
		memset(stats,0,sizeof(*stats));
	}
	if(view == NULL || qp == NULL || trust_radius == NULL || options == NULL || ops == NULL
		|| line_search_options == NULL || line_search_ops == NULL || x == NULL
	){
		return A4SQP_CORE_STEP_QP_ERROR;
	}
	if(ops->prepare_hessian == NULL || ops->solve_qp == NULL){
		return A4SQP_CORE_STEP_QP_ERROR;
	}
	a4sqp_view_get_core(view,&initial_view);
	restoration_active = a4sqp_core_restoration_choose(
		&initial_view,
		&options->restoration,
		options->feas_tol,
		options->restoration_state
	);
	effective_has_objective = has_objective && !restoration_active;
	effective_line_options = *line_search_options;
	effective_line_options.restoration = restoration_active;
	effective_line_options.restoration_margin = options->restoration.margin;

	memset(&step_hess,0,sizeof(step_hess));
	if(ops->prepare_hessian(ctx,&step_hess)){
		return A4SQP_CORE_STEP_HESSIAN_ERROR;
	}

	for(attempt = 0; attempt <= options->trust_qp_retries; ++attempt){
		struct A4SqpCoreView core_view;
		struct A4SqpQpBuildOptions qp_options;
		a4sqp_view_get_core(view,&core_view);
		memset(&qp_options,0,sizeof(qp_options));
			qp_options.trust_radius = (core_view.n_rel > 0 || options->trust_unconstrained) ? *trust_radius : 0.0;
			qp_options.elastic_penalty = options->elastic_penalty;
			qp_options.feas_tol = options->feas_tol;
			qp_options.objective_weight = restoration_active ? 0.0 : 1.0;
		if(a4sqp_qp_build_from_core_view_options(
			qp,
			&core_view,
			&step_hess,
			&qp_options
		)){
			return A4SQP_CORE_STEP_QP_BUILD_ERROR;
		}
		if(stats != NULL){
			++stats->qp_solves;
		}
		if(ops->solve_qp(ctx,qp) == 0){
			real64 max_violation = 0.0;
			real64 current_merit = 0.0;
			real64 null_qp_tol;
			if(ops->after_qp_solve != NULL){
				ops->after_qp_solve(ctx,qp);
			}
			if(view->n_rel > 0){
				struct A4SqpCoreView current_view;
				a4sqp_view_get_core(view,&current_view);
				current_view.has_objective = effective_has_objective;
				current_merit = a4sqp_core_merit(&current_view,options->elastic_penalty);
				(void)a4sqp_core_violation(&current_view,&max_violation,NULL);
				null_qp_tol = 1e-8 * fmax(1.0,fabs(current_merit));
				if(null_qp_tol < options->merit_tol){
					null_qp_tol = options->merit_tol;
				}
				if(
					fabs(qp->objective_value) <= null_qp_tol
					&& max_violation <= options->feas_tol
				){
					a4sqp_core_step_record_zero_accept(line_search_result,current_merit);
					return A4SQP_CORE_STEP_ACCEPTED;
				}
			}
			if(a4sqp_core_line_search_vector(
				view,
				qp,
				&effective_line_options,
				line_search_ops,
				ctx,
				x,
				effective_has_objective,
				line_search_result
			) == 0){
				return A4SQP_CORE_STEP_ACCEPTED;
			}
			last_error = A4SQP_CORE_STEP_LINE_SEARCH_ERROR;
			if(stats != NULL){
				++stats->line_search_failures;
			}
			if(view->n_rel > 0){
				struct A4SqpCoreView current_view;
				a4sqp_view_get_core(view,&current_view);
				current_view.has_objective = effective_has_objective;
				current_merit = a4sqp_core_merit(&current_view,options->elastic_penalty);
				(void)a4sqp_core_violation(&current_view,&max_violation,NULL);
				if(
					max_violation <= options->feas_tol
					&& a4sqp_core_qp_elastic_sum(qp) <= options->feas_tol
				){
					a4sqp_core_step_record_zero_accept(line_search_result,current_merit);
					return A4SQP_CORE_STEP_ACCEPTED;
				}
			}
			if(a4sqp_core_step_may_retry(view,attempt,options,ops,ctx,"line-search",stats)){
				continue;
			}
			return last_error;
		}
		last_error = A4SQP_CORE_STEP_QP_ERROR;
		if(stats != NULL){
			++stats->qp_failures;
		}
		if(a4sqp_core_step_may_retry(view,attempt,options,ops,ctx,"qp",stats)){
			continue;
		}
		return last_error;
	}
	return last_error;
}
