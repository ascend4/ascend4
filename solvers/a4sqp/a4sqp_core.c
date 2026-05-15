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

	for(i = 0; i < n; ++i){
		real64 value = view->scaled_var_value[i];
		real64 lower = view->scaled_var_lower[i];
		real64 upper = view->scaled_var_upper[i];
		int at_lower = !a4sqp_core_is_lower_inf(lower) && value <= lower + active_tol;
		int at_upper = !a4sqp_core_is_upper_inf(upper) && value >= upper - active_tol;
		if((at_lower || at_upper) && basis_count < n){
			basis[basis_count * n + i] = 1.0;
			++basis_count;
		}
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
	if(view == NULL || view->n_var <= 0){
		return 0.0;
	}
	lag_grad = A4SQP_NEW_ARRAY_CLEAR(real64,view->n_var);
	if(lag_grad == NULL){
		return 0.0;
	}
	if(a4sqp_core_lagrangian_gradient_for_view(view,row_dual,row_sign,lag_grad)){
		A4SQP_FREE(lag_grad);
		return 0.0;
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

int a4sqp_core_lagrangian_gradient_for_view(
	const struct A4SqpCoreView *view,
	const real64 *row_dual,
	real64 row_sign,
	real64 *lag_grad
){
	int32 i;
	int32 row;
	if(view == NULL || lag_grad == NULL || view->n_var < 0){
		return 1;
	}
	if(!isfinite(row_sign) || row_sign == 0.0){
		row_sign = 1.0;
	}
	for(i = 0; i < view->n_var; ++i){
		lag_grad[i] = (view->has_objective && view->scaled_obj_gradient != NULL)
			? view->scaled_obj_gradient[i]
			: 0.0;
	}
	if(row_dual == NULL || view->jac_row_start == NULL || view->jac_col_index == NULL || view->scaled_jac_value == NULL){
		return 0;
	}
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
	return 0;
}

int a4sqp_core_lagrangian_gradient(
	const struct A4SqpView *view,
	int has_objective,
	const real64 *row_dual,
	real64 row_sign,
	real64 *lag_grad
){
	struct A4SqpCoreView core;
	if(view == NULL){
		return 1;
	}
	a4sqp_view_get_core(view,&core);
	core.has_objective = has_objective;
	return a4sqp_core_lagrangian_gradient_for_view(&core,row_dual,row_sign,lag_grad);
}

static real64 a4sqp_core_kkt_complementarity_inf(
	const struct A4SqpCoreView *view,
	const real64 *row_dual,
	real64 active_tol
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
		if(gap <= active_tol){
			gap = 0.0;
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
	residual->complementarity_inf = a4sqp_core_kkt_complementarity_inf(view,row_dual,active_tol);
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

void a4sqp_core_multiplier_estimate_init(
	struct A4SqpCoreMultiplierEstimate *estimate
){
	if(estimate == NULL){
		return;
	}
	memset(estimate,0,sizeof(*estimate));
}

void a4sqp_core_multiplier_estimate_destroy(
	struct A4SqpCoreMultiplierEstimate *estimate
){
	if(estimate == NULL){
		return;
	}
	A4SQP_FREE(estimate->lambda);
	estimate->lambda = NULL;
	estimate->n = 0;
	estimate->ready = 0;
	estimate->good_count = 0;
	estimate->required_count = 0;
	estimate->elastic_max = 0.0;
}

int a4sqp_core_multiplier_estimate_sync(
	struct A4SqpCoreMultiplierEstimate *estimate,
	int32 n
){
	if(estimate == NULL || n < 0){
		return 1;
	}
	if(estimate->n == n && (n == 0 || estimate->lambda != NULL)){
		return 0;
	}
	a4sqp_core_multiplier_estimate_destroy(estimate);
	if(n > 0){
		estimate->lambda = A4SQP_NEW_ARRAY_CLEAR(real64,n);
		if(estimate->lambda == NULL){
			return 1;
		}
	}
	estimate->n = n;
	return 0;
}

void a4sqp_core_multiplier_estimate_reset(
	struct A4SqpCoreMultiplierEstimate *estimate
){
	if(estimate == NULL){
		return;
	}
	if(estimate->lambda != NULL && estimate->n > 0){
		memset(estimate->lambda,0,sizeof(real64) * (size_t)estimate->n);
	}
	estimate->ready = 0;
	estimate->good_count = 0;
	estimate->required_count = 0;
	estimate->elastic_max = 0.0;
}

int a4sqp_core_multiplier_estimate_update(
	struct A4SqpCoreMultiplierEstimate *estimate,
	const struct A4SqpCoreView *view,
	const struct A4SqpQp *qp,
	const struct A4SqpCoreMultiplierOptions *options
){
	int32 row;
	int32 good_count = 0;
	int32 required_count = 0;
	real64 elastic_max = 0.0;
	real64 feas_tol;
	real64 active_tol;
	real64 near_tol;
	real64 elastic_penalty;
	const real64 *row_dual_scale = NULL;
	if(
		estimate == NULL
		|| view == NULL
		|| qp == NULL
		|| view->n_rel <= 0
		|| qp->row_dual == NULL
		|| qp->col_value == NULL
		|| qp->num_row != view->n_rel
	){
		return 1;
	}
	if(a4sqp_core_multiplier_estimate_sync(estimate,view->n_rel)){
		return 1;
	}
	feas_tol = options != NULL ? options->feas_tol : 1e-6;
	if(!isfinite(feas_tol) || feas_tol <= 0.0){
		feas_tol = 1e-6;
	}
	elastic_penalty = options != NULL ? options->elastic_penalty : A4SQP_QP_DEFAULT_ELASTIC_PENALTY;
	if(!isfinite(elastic_penalty) || elastic_penalty <= 0.0){
		elastic_penalty = A4SQP_QP_DEFAULT_ELASTIC_PENALTY;
	}
	row_dual_scale = options != NULL ? options->row_dual_scale : NULL;
	active_tol = fmax(10.0 * feas_tol,1e-8);
	near_tol = fmax(100.0 * feas_tol,10.0 * active_tol);
	for(row = 0; row < view->n_rel; ++row){
		enum A4SqpRowActivity activity;
		int32 lower_col = view->n_var + 2 * row;
		int32 upper_col = lower_col + 1;
		real64 lower_elastic = 0.0;
		real64 upper_elastic = 0.0;
		real64 elastic = 0.0;
		real64 raw_scaled = 0.0;
		real64 sample = 0.0;
		real64 scale = row_dual_scale != NULL ? row_dual_scale[row] : 1.0;
		real64 previous = estimate->lambda != NULL ? estimate->lambda[row] : 0.0;
		real64 next = 0.0;
		int good = 1;
		int required = 0;
		activity = a4sqp_core_row_activity_for_view(view,row,active_tol,near_tol);
		required = activity != A4SQP_ROW_INACTIVE;
		if(required){
			++required_count;
		}
		if(lower_col >= 0 && lower_col < qp->num_col){
			lower_elastic = fabs(qp->col_value[lower_col]);
		}
		if(upper_col >= 0 && upper_col < qp->num_col){
			upper_elastic = fabs(qp->col_value[upper_col]);
		}
		elastic = lower_elastic > upper_elastic ? lower_elastic : upper_elastic;
		if(elastic > elastic_max){
			elastic_max = elastic;
		}
		raw_scaled = qp->row_dual[row];
		if(!isfinite(raw_scaled)){
			raw_scaled = 0.0;
			good = 0;
		}
		if(!isfinite(scale)){
			scale = 1.0;
			good = 0;
		}
		sample = raw_scaled * scale;
		if(!isfinite(sample)){
			sample = 0.0;
			good = 0;
		}
		if(elastic > 10.0 * feas_tol){
			good = 0;
		}
		if(fabs(raw_scaled) >= 0.95 * elastic_penalty){
			good = 0;
		}
		switch(activity){
		case A4SQP_ROW_EQUALITY:
			next = good ? sample : (estimate->ready ? 0.90 * previous : 0.0);
			if(good && estimate->ready){
				next = 0.75 * previous + 0.25 * sample;
			}
			break;
		case A4SQP_ROW_ACTIVE:
			next = good ? sample : (estimate->ready ? 0.80 * previous : 0.0);
			if(good && estimate->ready){
				next = 0.65 * previous + 0.35 * sample;
			}
			break;
		case A4SQP_ROW_NEAR_ACTIVE:
			next = good ? sample : (estimate->ready ? 0.85 * previous : 0.0);
			if(good && estimate->ready){
				next = 0.85 * previous + 0.15 * sample;
			}
			break;
		case A4SQP_ROW_INACTIVE:
		default:
			if(good && fabs(sample) <= near_tol){
				next = estimate->ready ? 0.20 * previous + 0.10 * sample : sample;
			}else{
				next = estimate->ready ? 0.20 * previous : 0.0;
			}
			break;
		}
		if(required){
			if(good){
				++good_count;
			}
		}else if(fabs(next) <= near_tol){
			++good_count;
		}
		if(!isfinite(next) || fabs(next) <= 1e-16){
			next = 0.0;
		}
		estimate->lambda[row] = next;
	}
	estimate->elastic_max = elastic_max;
	estimate->ready = 1;
	estimate->good_count = good_count;
	estimate->required_count = required_count;
	return 0;
}

int a4sqp_core_multiplier_estimate_update_view(
	struct A4SqpCoreMultiplierEstimate *estimate,
	const struct A4SqpView *view,
	const struct A4SqpQp *qp,
	const struct A4SqpCoreMultiplierOptions *options
){
	struct A4SqpCoreView core;
	if(view == NULL){
		return 1;
	}
	a4sqp_view_get_core(view,&core);
	return a4sqp_core_multiplier_estimate_update(estimate,&core,qp,options);
}

static int a4sqp_core_solve_dense_system(real64 *a, real64 *b, int32 n){
	int32 i;
	int32 j;
	int32 k;
	if(a == NULL || b == NULL || n < 0){
		return 1;
	}
	for(k = 0; k < n; ++k){
		int32 pivot = k;
		real64 pivot_abs = fabs(a[(size_t)k * (size_t)n + (size_t)k]);
		for(i = k + 1; i < n; ++i){
			real64 candidate = fabs(a[(size_t)i * (size_t)n + (size_t)k]);
			if(candidate > pivot_abs){
				pivot = i;
				pivot_abs = candidate;
			}
		}
		if(pivot_abs <= 1e-18 || !isfinite(pivot_abs)){
			return 1;
		}
		if(pivot != k){
			for(j = k; j < n; ++j){
				real64 tmp = a[(size_t)k * (size_t)n + (size_t)j];
				a[(size_t)k * (size_t)n + (size_t)j] = a[(size_t)pivot * (size_t)n + (size_t)j];
				a[(size_t)pivot * (size_t)n + (size_t)j] = tmp;
			}
			{
				real64 tmp = b[k];
				b[k] = b[pivot];
				b[pivot] = tmp;
			}
		}
		for(i = k + 1; i < n; ++i){
			real64 factor = a[(size_t)i * (size_t)n + (size_t)k] / a[(size_t)k * (size_t)n + (size_t)k];
			if(factor == 0.0){
				continue;
			}
			a[(size_t)i * (size_t)n + (size_t)k] = 0.0;
			for(j = k + 1; j < n; ++j){
				a[(size_t)i * (size_t)n + (size_t)j] -= factor * a[(size_t)k * (size_t)n + (size_t)j];
			}
			b[i] -= factor * b[k];
		}
	}
	for(i = n - 1; i >= 0; --i){
		real64 sum = b[i];
		for(j = i + 1; j < n; ++j){
			sum -= a[(size_t)i * (size_t)n + (size_t)j] * b[j];
		}
		b[i] = sum / a[(size_t)i * (size_t)n + (size_t)i];
		if(!isfinite(b[i])){
			return 1;
		}
	}
	return 0;
}

static real64 a4sqp_core_jac_value_for_row_col(
	const struct A4SqpCoreView *view,
	int32 row,
	int32 col
){
	int32 k;
	if(
		view == NULL
		|| row < 0
		|| row >= view->n_rel
		|| col < 0
		|| col >= view->n_var
		|| view->jac_row_start == NULL
		|| view->jac_col_index == NULL
		|| view->scaled_jac_value == NULL
	){
		return 0.0;
	}
	for(k = view->jac_row_start[row]; k < view->jac_row_start[row + 1]; ++k){
		if(view->jac_col_index[k] == col && isfinite(view->scaled_jac_value[k])){
			return view->scaled_jac_value[k];
		}
	}
	return 0.0;
}

static int a4sqp_core_multiplier_estimate_solve_column_qr(
	struct A4SqpCoreMultiplierEstimate *estimate,
	const struct A4SqpCoreView *view,
	const int32 *active,
	int32 active_count,
	const int *selected_var
){
	int32 selected_count = 0;
	int32 *selected = NULL;
	real64 *q = NULL;
	real64 *r = NULL;
	real64 *rhs = NULL;
	real64 *y = NULL;
	int32 i;
	int32 j;
	int32 arow;
	int status = 1;
	if(
		estimate == NULL
		|| estimate->lambda == NULL
		|| view == NULL
		|| active == NULL
		|| selected_var == NULL
		|| active_count <= 0
		|| view->n_var <= 0
		|| view->scaled_obj_gradient == NULL
	){
		return 1;
	}
	for(i = 0; i < view->n_var; ++i){
		if(selected_var[i]){
			++selected_count;
		}
	}
	if(selected_count <= 0 || selected_count > active_count){
		return 1;
	}
	selected = A4SQP_NEW_ARRAY_OR_NULL(int32,selected_count);
	q = A4SQP_NEW_ARRAY_CLEAR(real64,(size_t)active_count * (size_t)selected_count);
	r = A4SQP_NEW_ARRAY_CLEAR(real64,(size_t)selected_count * (size_t)selected_count);
	rhs = A4SQP_NEW_ARRAY_OR_NULL(real64,selected_count);
	y = A4SQP_NEW_ARRAY_OR_NULL(real64,selected_count);
	if(selected == NULL || q == NULL || r == NULL || rhs == NULL || y == NULL){
		goto cleanup;
	}
	selected_count = 0;
	for(i = 0; i < view->n_var; ++i){
		if(selected_var[i]){
			selected[selected_count++] = i;
		}
	}
	for(i = 0; i < selected_count; ++i){
		rhs[i] = -view->scaled_obj_gradient[selected[i]];
		for(arow = 0; arow < active_count; ++arow){
			q[(size_t)arow * (size_t)selected_count + (size_t)i] =
				a4sqp_core_jac_value_for_row_col(view,active[arow],selected[i]);
		}
	}
	for(j = 0; j < selected_count; ++j){
		real64 norm = 0.0;
		int32 prev;
		for(prev = 0; prev < j; ++prev){
			real64 dot = 0.0;
			for(arow = 0; arow < active_count; ++arow){
				dot += q[(size_t)arow * (size_t)selected_count + (size_t)prev]
					* q[(size_t)arow * (size_t)selected_count + (size_t)j];
			}
			r[(size_t)prev * (size_t)selected_count + (size_t)j] = dot;
			for(arow = 0; arow < active_count; ++arow){
				q[(size_t)arow * (size_t)selected_count + (size_t)j] -=
					dot * q[(size_t)arow * (size_t)selected_count + (size_t)prev];
			}
		}
		for(prev = 0; prev < j; ++prev){
			real64 dot = 0.0;
			for(arow = 0; arow < active_count; ++arow){
				dot += q[(size_t)arow * (size_t)selected_count + (size_t)prev]
					* q[(size_t)arow * (size_t)selected_count + (size_t)j];
			}
			r[(size_t)prev * (size_t)selected_count + (size_t)j] += dot;
			for(arow = 0; arow < active_count; ++arow){
				q[(size_t)arow * (size_t)selected_count + (size_t)j] -=
					dot * q[(size_t)arow * (size_t)selected_count + (size_t)prev];
			}
		}
		for(arow = 0; arow < active_count; ++arow){
			real64 value = q[(size_t)arow * (size_t)selected_count + (size_t)j];
			norm += value * value;
		}
		norm = sqrt(norm);
		if(!isfinite(norm) || norm <= 1e-14){
			goto cleanup;
		}
		r[(size_t)j * (size_t)selected_count + (size_t)j] = norm;
		for(arow = 0; arow < active_count; ++arow){
			q[(size_t)arow * (size_t)selected_count + (size_t)j] /= norm;
		}
	}
	for(i = 0; i < selected_count; ++i){
		real64 sum = rhs[i];
		for(j = 0; j < i; ++j){
			sum -= r[(size_t)j * (size_t)selected_count + (size_t)i] * y[j];
		}
		y[i] = sum / r[(size_t)i * (size_t)selected_count + (size_t)i];
		if(!isfinite(y[i])){
			goto cleanup;
		}
	}
	memset(estimate->lambda,0,(size_t)estimate->n * sizeof(*estimate->lambda));
	for(arow = 0; arow < active_count; ++arow){
		real64 lambda = 0.0;
		for(i = 0; i < selected_count; ++i){
			lambda += q[(size_t)arow * (size_t)selected_count + (size_t)i] * y[i];
		}
		estimate->lambda[active[arow]] = lambda;
	}
	status = 0;

cleanup:
	A4SQP_FREE(selected);
	A4SQP_FREE(q);
	A4SQP_FREE(r);
	A4SQP_FREE(rhs);
	A4SQP_FREE(y);
	return status;
}

int a4sqp_core_multiplier_estimate_recover_stationarity(
	struct A4SqpCoreMultiplierEstimate *estimate,
	const struct A4SqpCoreView *view,
	real64 active_tol
){
	int32 row;
	int32 i;
	int32 arow;
	int32 active_count = 0;
	int32 *active = NULL;
	int *selected_var = NULL;
	real64 *normal = NULL;
	real64 *rhs = NULL;
	real64 *lag_grad = NULL;
	real64 near_tol;
	int iter;
	if(
		estimate == NULL
		|| view == NULL
		|| view->n_rel <= 0
		|| view->n_var <= 0
		|| view->scaled_obj_gradient == NULL
		|| view->jac_row_start == NULL
		|| view->jac_col_index == NULL
		|| view->scaled_jac_value == NULL
	){
		return 1;
	}
	if(!isfinite(active_tol) || active_tol <= 0.0){
		active_tol = 1e-7;
	}
	near_tol = fmax(10.0 * active_tol,1e-8);
	if(a4sqp_core_multiplier_estimate_sync(estimate,view->n_rel)){
		return 1;
	}
	memset(estimate->lambda,0,(size_t)estimate->n * sizeof(*estimate->lambda));
	active = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_rel);
	selected_var = A4SQP_NEW_ARRAY_OR_NULL(int,view->n_var);
	lag_grad = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
	if(active == NULL || selected_var == NULL || lag_grad == NULL){
		A4SQP_FREE(active);
		A4SQP_FREE(selected_var);
		A4SQP_FREE(lag_grad);
		return 1;
	}
	for(i = 0; i < view->n_var; ++i){
		real64 value = view->scaled_var_value != NULL ? view->scaled_var_value[i] : 0.0;
		real64 lower = view->scaled_var_lower != NULL ? view->scaled_var_lower[i] : A4SQP_NO_LOWER_BOUND;
		real64 upper = view->scaled_var_upper != NULL ? view->scaled_var_upper[i] : A4SQP_NO_UPPER_BOUND;
		int at_lower = !a4sqp_core_is_lower_inf(lower) && value <= lower + active_tol;
		int at_upper = !a4sqp_core_is_upper_inf(upper) && value >= upper - active_tol;
		selected_var[i] = !(at_lower || at_upper);
	}
	for(row = 0; row < view->n_rel; ++row){
		enum A4SqpRowActivity activity;
		activity = a4sqp_core_row_activity_for_view(view,row,active_tol,near_tol);
		if(activity == A4SQP_ROW_EQUALITY || activity == A4SQP_ROW_ACTIVE){
			active[active_count++] = row;
		}
	}
	if(active_count <= 0){
		estimate->ready = 1;
		estimate->good_count = 0;
		estimate->required_count = 0;
		A4SQP_FREE(active);
		A4SQP_FREE(selected_var);
		A4SQP_FREE(lag_grad);
		return 0;
	}
	normal = A4SQP_NEW_ARRAY_CLEAR(real64,(size_t)active_count * (size_t)active_count);
	rhs = A4SQP_NEW_ARRAY_CLEAR(real64,active_count);
	if(normal == NULL || rhs == NULL){
		A4SQP_FREE(active);
		A4SQP_FREE(selected_var);
		A4SQP_FREE(lag_grad);
		A4SQP_FREE(normal);
		A4SQP_FREE(rhs);
		return 1;
	}
	for(iter = 0; iter <= view->n_var; ++iter){
		int added = 0;
		if(a4sqp_core_multiplier_estimate_solve_column_qr(estimate,view,active,active_count,selected_var)){
			memset(normal,0,(size_t)active_count * (size_t)active_count * sizeof(*normal));
			memset(rhs,0,(size_t)active_count * sizeof(*rhs));
			for(arow = 0; arow < active_count; ++arow){
				int32 r = active[arow];
				int32 k;
				for(k = view->jac_row_start[r]; k < view->jac_row_start[r + 1]; ++k){
					int32 col = view->jac_col_index[k];
					real64 jac = view->scaled_jac_value[k];
					if(col < 0 || col >= view->n_var || !selected_var[col] || !isfinite(jac)){
						continue;
					}
					rhs[arow] -= jac * view->scaled_obj_gradient[col];
				}
			}
			for(arow = 0; arow < active_count; ++arow){
				int32 r1 = active[arow];
				int32 brow;
				for(brow = 0; brow < active_count; ++brow){
					int32 r2 = active[brow];
					int32 k1;
					for(k1 = view->jac_row_start[r1]; k1 < view->jac_row_start[r1 + 1]; ++k1){
						int32 k2;
						int32 col1 = view->jac_col_index[k1];
						real64 jac1 = view->scaled_jac_value[k1];
						if(col1 < 0 || col1 >= view->n_var || !selected_var[col1] || !isfinite(jac1)){
							continue;
						}
						for(k2 = view->jac_row_start[r2]; k2 < view->jac_row_start[r2 + 1]; ++k2){
							if(view->jac_col_index[k2] == col1 && isfinite(view->scaled_jac_value[k2])){
								normal[(size_t)arow * (size_t)active_count + (size_t)brow] += jac1 * view->scaled_jac_value[k2];
							}
						}
					}
				}
			}
			for(i = 0; i < active_count; ++i){
				normal[(size_t)i * (size_t)active_count + (size_t)i] += 1e-14;
			}
			if(a4sqp_core_solve_dense_system(normal,rhs,active_count)){
				A4SQP_FREE(active);
				A4SQP_FREE(selected_var);
				A4SQP_FREE(lag_grad);
				A4SQP_FREE(normal);
				A4SQP_FREE(rhs);
				return 1;
			}
			memset(estimate->lambda,0,(size_t)estimate->n * sizeof(*estimate->lambda));
			for(arow = 0; arow < active_count; ++arow){
				estimate->lambda[active[arow]] = rhs[arow];
			}
		}
		if(a4sqp_core_lagrangian_gradient_for_view(view,estimate->lambda,1.0,lag_grad)){
			break;
		}
		for(i = 0; i < view->n_var; ++i){
			real64 value = view->scaled_var_value != NULL ? view->scaled_var_value[i] : 0.0;
			real64 lower = view->scaled_var_lower != NULL ? view->scaled_var_lower[i] : A4SQP_NO_LOWER_BOUND;
			real64 upper = view->scaled_var_upper != NULL ? view->scaled_var_upper[i] : A4SQP_NO_UPPER_BOUND;
			real64 residual;
			int at_lower = !a4sqp_core_is_lower_inf(lower) && value <= lower + active_tol;
			int at_upper = !a4sqp_core_is_upper_inf(upper) && value >= upper - active_tol;
			if(selected_var[i] || !(at_lower || at_upper)){
				continue;
			}
			residual = a4sqp_core_bound_stationarity_residual(lag_grad[i],value,lower,upper,active_tol);
			if(residual > active_tol){
				selected_var[i] = 1;
				added = 1;
			}
		}
		if(!added){
			break;
		}
	}
	estimate->ready = 1;
	estimate->good_count = active_count;
	estimate->required_count = active_count;
	A4SQP_FREE(active);
	A4SQP_FREE(selected_var);
	A4SQP_FREE(lag_grad);
	A4SQP_FREE(normal);
	A4SQP_FREE(rhs);
	return 0;
}

int a4sqp_core_bound_stats_for_view(
	const struct A4SqpCoreView *view,
	const real64 *row_dual,
	real64 row_sign,
	real64 active_tol,
	struct A4SqpCoreBoundStats *stats
){
	real64 *lag_grad = NULL;
	int32 i;
	if(stats == NULL){
		return 1;
	}
	memset(stats,0,sizeof(*stats));
	stats->worst_index = -1;
	if(view == NULL || view->n_var <= 0){
		return 0;
	}
	lag_grad = A4SQP_NEW_ARRAY_CLEAR(real64,view->n_var);
	if(lag_grad == NULL){
		return 1;
	}
	if(a4sqp_core_lagrangian_gradient_for_view(view,row_dual,row_sign,lag_grad)){
		A4SQP_FREE(lag_grad);
		return 1;
	}
	for(i = 0; i < view->n_var; ++i){
		real64 value = view->scaled_var_value != NULL ? view->scaled_var_value[i] : 0.0;
		real64 lower = view->scaled_var_lower != NULL ? view->scaled_var_lower[i] : A4SQP_NO_LOWER_BOUND;
		real64 upper = view->scaled_var_upper != NULL ? view->scaled_var_upper[i] : A4SQP_NO_UPPER_BOUND;
		int at_lower = !a4sqp_core_is_lower_inf(lower) && value <= lower + active_tol;
		int at_upper = !a4sqp_core_is_upper_inf(upper) && value >= upper - active_tol;
		real64 residual;
		if(at_lower && at_upper){
			++stats->fixed_active;
		}else{
			if(at_lower){
				++stats->lower_active;
			}
			if(at_upper){
				++stats->upper_active;
			}
		}
		residual = a4sqp_core_bound_stationarity_residual(lag_grad[i],value,lower,upper,active_tol);
		if(residual > stats->stationarity_inf){
			stats->stationarity_inf = residual;
			stats->worst_index = i;
			stats->worst_lagrangian_gradient = lag_grad[i];
		}
	}
	A4SQP_FREE(lag_grad);
	return 0;
}

int a4sqp_core_bound_stats(
	const struct A4SqpView *view,
	int has_objective,
	const real64 *row_dual,
	real64 row_sign,
	real64 active_tol,
	struct A4SqpCoreBoundStats *stats
){
	struct A4SqpCoreView core;
	if(view == NULL){
		if(stats != NULL){
			memset(stats,0,sizeof(*stats));
			stats->worst_index = -1;
		}
		return 1;
	}
	a4sqp_view_get_core(view,&core);
	core.has_objective = has_objective;
	return a4sqp_core_bound_stats_for_view(&core,row_dual,row_sign,active_tol,stats);
}

int a4sqp_core_rel_stats_for_view(
	const struct A4SqpCoreView *view,
	real64 active_tol,
	real64 near_tol,
	struct A4SqpCoreRelStats *stats
){
	int32 row;
	if(stats == NULL){
		return 1;
	}
	memset(stats,0,sizeof(*stats));
	if(view == NULL || view->n_rel <= 0){
		return 0;
	}
	if(!isfinite(active_tol) || active_tol <= 0.0){
		active_tol = 1e-7;
	}
	if(!isfinite(near_tol) || near_tol < active_tol){
		near_tol = fmax(10.0 * active_tol,1e-8);
	}
	for(row = 0; row < view->n_rel; ++row){
		switch(a4sqp_core_row_activity_for_view(view,row,active_tol,near_tol)){
		case A4SQP_ROW_EQUALITY:
			++stats->equality;
			break;
		case A4SQP_ROW_ACTIVE:
			++stats->active;
			break;
		case A4SQP_ROW_NEAR_ACTIVE:
			++stats->near_active;
			break;
		case A4SQP_ROW_INACTIVE:
		default:
			++stats->inactive;
			break;
		}
	}
	return 0;
}

int a4sqp_core_rel_stats(
	const struct A4SqpView *view,
	real64 active_tol,
	real64 near_tol,
	struct A4SqpCoreRelStats *stats
){
	struct A4SqpCoreView core;
	if(view == NULL){
		if(stats != NULL){
			memset(stats,0,sizeof(*stats));
		}
		return 1;
	}
	a4sqp_view_get_core(view,&core);
	return a4sqp_core_rel_stats_for_view(&core,active_tol,near_tol,stats);
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
	state->entry_violation = HUGE_VAL;
	state->stall_count = 0;
	state->restoration_iter = 0;
	state->active = 0;
	state->handoff = 0;
	state->reentry_hysteresis = 0;
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
	real64 handoff_reduction;
	real64 reentry_factor;
	int was_restoring;
	if(state == NULL){
		return 0;
	}
	if(options == NULL || !options->enable || view == NULL || view->n_rel <= 0){
		a4sqp_core_restoration_state_init(state);
		return 0;
	}
	(void)a4sqp_core_violation(view,&max_violation,NULL);
	exit_tol = feas_tol;
	handoff_reduction = options->handoff_reduction;
	if(!isfinite(handoff_reduction) || handoff_reduction < 0.0 || handoff_reduction >= 1.0){
		handoff_reduction = 0.0;
	}
	reentry_factor = options->reentry_factor;
	if(!isfinite(reentry_factor) || reentry_factor < 1.0){
		reentry_factor = 1.0;
	}
	was_restoring = state->phase == A4SQP_CORE_PHASE_RESTORATION || state->active;
	if(max_violation <= exit_tol){
		if(was_restoring){
			state->best_violation = max_violation;
			state->entry_violation = HUGE_VAL;
			state->stall_count = 0;
			state->restoration_iter = 0;
			state->active = 0;
			state->handoff = 0;
			state->reentry_hysteresis = 1;
			state->phase = A4SQP_CORE_PHASE_RESTORATION_EXIT;
		}else{
			a4sqp_core_restoration_state_init(state);
		}
		return 0;
	}
	if(state->phase == A4SQP_CORE_PHASE_RESTORATION_EXIT){
		state->best_violation = max_violation;
		state->entry_violation = HUGE_VAL;
		state->stall_count = 0;
			state->restoration_iter = 0;
			state->active = 0;
			state->handoff = 0;
			state->reentry_hysteresis = 1;
			state->phase = A4SQP_CORE_PHASE_REGULAR;
			return 0;
	}
	if(was_restoring){
		++state->restoration_iter;
		if(
			!isfinite(state->best_violation)
			|| max_violation <= (1.0 - options->improve) * state->best_violation
		){
			state->best_violation = max_violation;
			state->stall_count = 0;
		}else{
			++state->stall_count;
		}
		if(
			handoff_reduction > 0.0
			&& isfinite(state->entry_violation)
			&& state->entry_violation > exit_tol
			&& max_violation <= (1.0 - handoff_reduction) * state->entry_violation
		){
			state->active = 0;
			state->handoff = 1;
			state->phase = A4SQP_CORE_PHASE_RESTORATION_EXIT;
			return 0;
		}
		if(options->max_iter > 0 && state->restoration_iter >= options->max_iter){
			state->active = 0;
			state->handoff = 1;
			state->phase = A4SQP_CORE_PHASE_RESTORATION_EXIT;
			return 0;
		}
		state->active = 1;
		state->handoff = 0;
		state->phase = A4SQP_CORE_PHASE_RESTORATION;
		return 1;
	}
	if(options->trigger_iter <= 0){
		if(state->reentry_hysteresis && max_violation <= reentry_factor * exit_tol){
			state->stall_count = 0;
			state->restoration_iter = 0;
			state->entry_violation = HUGE_VAL;
			state->best_violation = max_violation;
			state->active = 0;
			state->handoff = 0;
			state->phase = A4SQP_CORE_PHASE_REGULAR;
			return 0;
		}
		state->stall_count = 0;
		state->restoration_iter = 1;
		state->entry_violation = max_violation;
		state->best_violation = max_violation;
		state->active = 1;
		state->handoff = 0;
		state->reentry_hysteresis = 0;
		state->phase = A4SQP_CORE_PHASE_RESTORATION;
		return 1;
	}
	if(
		!isfinite(state->best_violation)
		|| max_violation <= (1.0 - options->improve) * state->best_violation
	){
		state->best_violation = max_violation;
		state->entry_violation = HUGE_VAL;
		state->stall_count = 0;
		state->restoration_iter = 0;
		state->active = 0;
		state->handoff = 0;
		state->reentry_hysteresis = 0;
		state->phase = A4SQP_CORE_PHASE_REGULAR;
		return 0;
	}
	++state->stall_count;
	state->active = state->stall_count >= options->trigger_iter;
	if(state->active){
		state->restoration_iter = 1;
		state->entry_violation = max_violation;
		state->best_violation = max_violation;
		state->reentry_hysteresis = 0;
	}
	state->handoff = 0;
	state->phase = state->active ? A4SQP_CORE_PHASE_RESTORATION : A4SQP_CORE_PHASE_REGULAR;
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

static int a4sqp_core_make_unconstrained_gradient_step(
	const struct A4SqpCoreView *core,
	const struct A4SqpLineSearchOptions *options,
	real64 *physical_step,
	real64 *step_norm2,
	real64 *scaled_step_inf,
	struct A4SqpLineSearchResult *result
){
	real64 grad_inf = 0.0;
	real64 pred = 0.0;
	int32 i;
	if(
		core == NULL
		|| options == NULL
		|| physical_step == NULL
		|| step_norm2 == NULL
		|| scaled_step_inf == NULL
		|| result == NULL
		|| !core->has_objective
		|| core->n_rel > 0
		|| core->scaled_obj_gradient == NULL
		|| core->var_scale == NULL
	){
		return 1;
	}
	for(i = 0; i < core->n_var; ++i){
		real64 g = core->scaled_obj_gradient[i];
		if(fabs(g) > grad_inf){
			grad_inf = fabs(g);
		}
	}
	if(grad_inf <= 0.0 || !isfinite(grad_inf)){
		return 1;
	}
	*step_norm2 = 0.0;
	*scaled_step_inf = 0.0;
	for(i = 0; i < core->n_var; ++i){
		real64 scaled_step = -core->scaled_obj_gradient[i] / fmax(1.0,grad_inf);
		physical_step[i] = core->var_scale[i] * scaled_step;
		pred += core->scaled_obj_gradient[i] * (-scaled_step);
		*step_norm2 += physical_step[i] * physical_step[i];
		if(fabs(scaled_step) > *scaled_step_inf){
			*scaled_step_inf = fabs(scaled_step);
		}
	}
	if(pred <= options->merit_tol || !isfinite(pred)){
		return 1;
	}
	result->predicted_reduction = pred;
	result->model_merit_after = result->merit_before - pred;
	result->step_norm = sqrt(*step_norm2);
	result->scaled_step_inf = *scaled_step_inf;
	return 0;
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
	int using_fallback_direction = 0;
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
retry_line_search:
	if(fabs(result->predicted_reduction) <= options->merit_tol){
		if(!using_fallback_direction && !a4sqp_core_make_unconstrained_gradient_step(
			&core,
			options,
			physical_step,
			&step_norm2,
			&scaled_step_inf,
			result
		)){
			using_fallback_direction = 1;
			alpha = 1.0;
		}else{
			goto cleanup;
		}
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
		if(!using_fallback_direction){
			for(i = 0; i < n; ++i){
				x[i] = old_x[i];
			}
			(void)ops->evaluate(ctx,x,view);
			if(!a4sqp_core_make_unconstrained_gradient_step(
				&core,
				options,
				physical_step,
				&step_norm2,
				&scaled_step_inf,
				result
			)){
				using_fallback_direction = 1;
				alpha = 1.0;
				goto retry_line_search;
			}
		}
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
	real64 *trust_radius,
	struct A4SqpCoreStepStats *stats,
	int force_unconstrained_trust
){
	if(view == NULL || options == NULL || trust_radius == NULL || attempt >= options->trust_qp_retries){
		return 0;
	}
	if(view->n_rel <= 0 && !options->trust_unconstrained && !force_unconstrained_trust){
		return 0;
	}
	if(!a4sqp_trust_shrink_radius(trust_radius,&options->trust)){
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

static real64 a4sqp_core_signed_row_violation(const struct A4SqpCoreView *view, int32 row){
	real64 residual;
	real64 lower;
	real64 upper;
	if(view == NULL || row < 0 || row >= view->n_rel || view->scaled_rel_residual == NULL){
		return 0.0;
	}
	residual = view->scaled_rel_residual[row];
	lower = view->scaled_rel_lower[row];
	upper = view->scaled_rel_upper[row];
	if(!isfinite(residual)){
		return 0.0;
	}
	if(view->rel_kind != NULL && view->rel_kind[row] == A4SQP_REL_KIND_EQUALITY){
		real64 target = 0.0;
		if(!a4sqp_core_is_lower_inf(lower) && !a4sqp_core_is_upper_inf(upper)){
			target = 0.5 * (lower + upper);
		}else if(!a4sqp_core_is_lower_inf(lower)){
			target = lower;
		}else if(!a4sqp_core_is_upper_inf(upper)){
			target = upper;
		}
		return residual - target;
	}
	if(!a4sqp_core_is_lower_inf(lower) && residual < lower){
		return residual - lower;
	}
	if(!a4sqp_core_is_upper_inf(upper) && residual > upper){
		return residual - upper;
	}
	return 0.0;
}

static real64 a4sqp_core_violation_squares(
	const struct A4SqpCoreView *view,
	real64 *max_abs_violation
){
	int32 row;
	real64 merit = 0.0;
	real64 vmax = 0.0;
	if(view == NULL){
		if(max_abs_violation != NULL){
			*max_abs_violation = 0.0;
		}
		return 0.0;
	}
	for(row = 0; row < view->n_rel; ++row){
		real64 signed_violation = a4sqp_core_signed_row_violation(view,row);
		real64 abs_violation = fabs(signed_violation);
		merit += 0.5 * signed_violation * signed_violation;
		if(abs_violation > vmax){
			vmax = abs_violation;
		}
	}
	if(max_abs_violation != NULL){
		*max_abs_violation = vmax;
	}
	return merit;
}

static int a4sqp_core_nonlinear_restoration_step(
	struct A4SqpView *view,
	const struct A4SqpLineSearchOptions *line_options,
	const struct A4SqpVectorLineSearchOps *line_ops,
	void *ctx,
	real64 *x,
	real64 trust_radius,
	struct A4SqpLineSearchResult *result
){
	struct A4SqpCoreView core;
	struct A4SqpCoreView trial_core;
	real64 *start_x = NULL;
	real64 *base_x = NULL;
	real64 *start_scaled_x = NULL;
	real64 *start_scaled_grad = NULL;
	real64 *scaled_step = NULL;
	real64 *physical_step = NULL;
	real64 phi_start;
	real64 max_start;
	real64 phi_current;
	real64 max_current;
	real64 phi_final;
	real64 max_final;
	real64 last_alpha = 0.0;
	real64 net_step_norm2 = 0.0;
	real64 net_scaled_step_inf = 0.0;
	int total_trials = 0;
	int accepted = 0;
	int32 i;
	const int max_inner = 5;

	if(result == NULL){
		return 1;
	}
	if(
		view == NULL
		|| line_options == NULL
		|| line_ops == NULL
		|| line_ops->evaluate == NULL
		|| x == NULL
	){
		return 1;
	}
	a4sqp_view_get_core(view,&core);
	if(core.n_var <= 0 || core.n_rel <= 0 || core.jac_row_start == NULL || core.jac_col_index == NULL || core.scaled_jac_value == NULL){
		return 1;
	}
	phi_start = a4sqp_core_violation_squares(&core,&max_start);
	if(phi_start <= 0.0 || max_start <= line_options->feas_tol || !isfinite(phi_start)){
		return 1;
	}
	start_x = A4SQP_NEW_ARRAY_OR_NULL(real64,core.n_var);
	base_x = A4SQP_NEW_ARRAY_OR_NULL(real64,core.n_var);
	start_scaled_x = A4SQP_NEW_ARRAY_OR_NULL(real64,core.n_var);
	start_scaled_grad = A4SQP_NEW_ARRAY_OR_NULL(real64,core.n_var);
	scaled_step = A4SQP_NEW_ARRAY_OR_NULL(real64,core.n_var);
	physical_step = A4SQP_NEW_ARRAY_OR_NULL(real64,core.n_var);
	if(start_x == NULL || base_x == NULL || start_scaled_x == NULL || start_scaled_grad == NULL || scaled_step == NULL || physical_step == NULL){
		goto cleanup;
	}
	for(i = 0; i < core.n_var; ++i){
		start_x[i] = x[i];
		base_x[i] = x[i];
		start_scaled_x[i] = core.scaled_var_value != NULL ? core.scaled_var_value[i] : 0.0;
		start_scaled_grad[i] = core.scaled_obj_gradient != NULL ? core.scaled_obj_gradient[i] : 0.0;
	}
	phi_current = phi_start;
	max_current = max_start;

	for(i = 0; i < max_inner; ++i){
		real64 grad_inf = 0.0;
		real64 scaled_step_inf = 0.0;
		real64 radius;
		int32 row;
		int inner_accepted = 0;
		int32 j;

		if(max_current <= line_options->feas_tol){
			break;
		}
		a4sqp_view_get_core(view,&core);
		if(core.n_var <= 0 || core.n_rel <= 0 || core.jac_row_start == NULL || core.jac_col_index == NULL || core.scaled_jac_value == NULL){
			break;
		}
		memset(scaled_step,0,(size_t)core.n_var * sizeof(real64));
		for(row = 0; row < core.n_rel; ++row){
			real64 signed_violation = a4sqp_core_signed_row_violation(&core,row);
			real64 row_norm2 = 0.0;
			real64 row_scale;
			int32 k;
			if(signed_violation == 0.0 || !isfinite(signed_violation)){
				continue;
			}
			for(k = core.jac_row_start[row]; k < core.jac_row_start[row + 1]; ++k){
				int32 col = core.jac_col_index[k];
				if(col >= 0 && col < core.n_var && isfinite(core.scaled_jac_value[k])){
					row_norm2 += core.scaled_jac_value[k] * core.scaled_jac_value[k];
				}
			}
			if(row_norm2 <= 1e-24 || !isfinite(row_norm2)){
				continue;
			}
			row_scale = -signed_violation / row_norm2;
			for(k = core.jac_row_start[row]; k < core.jac_row_start[row + 1]; ++k){
				int32 col = core.jac_col_index[k];
				if(col >= 0 && col < core.n_var && isfinite(core.scaled_jac_value[k])){
					scaled_step[col] += row_scale * core.scaled_jac_value[k];
				}
			}
		}
		for(j = 0; j < core.n_var; ++j){
			real64 value = fabs(scaled_step[j]);
			if(value > grad_inf){
				grad_inf = value;
			}
		}
		if(grad_inf <= 0.0 || !isfinite(grad_inf)){
			break;
		}
		radius = (trust_radius > 0.0 && isfinite(trust_radius)) ? trust_radius : 1.0;
		if(grad_inf > radius){
			real64 scale = radius / grad_inf;
			for(j = 0; j < core.n_var; ++j){
				scaled_step[j] *= scale;
			}
		}
		for(j = 0; j < core.n_var; ++j){
			real64 step = scaled_step[j];
			real64 value = core.scaled_var_value != NULL ? core.scaled_var_value[j] : 0.0;
			real64 lower = core.scaled_var_lower != NULL ? core.scaled_var_lower[j] : A4SQP_NO_LOWER_BOUND;
			real64 upper = core.scaled_var_upper != NULL ? core.scaled_var_upper[j] : A4SQP_NO_UPPER_BOUND;
			if(!a4sqp_core_is_lower_inf(lower) && value + step < lower){
				step = lower - value;
			}
			if(!a4sqp_core_is_upper_inf(upper) && value + step > upper){
				step = upper - value;
			}
			scaled_step[j] = step;
			physical_step[j] = (core.var_scale != NULL ? core.var_scale[j] : 1.0) * step;
			if(fabs(step) > scaled_step_inf){
				scaled_step_inf = fabs(step);
			}
		}
		if(scaled_step_inf <= 0.0){
			break;
		}
		for(j = 0; j < line_options->max_backtrack; ++j){
			real64 alpha = ldexp(1.0,-j);
			real64 phi_after;
			real64 max_after;
			int32 k;
			for(k = 0; k < core.n_var; ++k){
				x[k] = base_x[k] + alpha * physical_step[k];
			}
			++total_trials;
			if(line_ops->evaluate(ctx,x,view)){
				continue;
			}
			a4sqp_view_get_core(view,&trial_core);
			phi_after = a4sqp_core_violation_squares(&trial_core,&max_after);
			if(
				isfinite(phi_after)
				&& (
					phi_after <= (1.0 - line_options->restoration_margin) * phi_current
					|| max_after <= (1.0 - line_options->restoration_margin) * max_current
				)
			){
				int32 k2;
				for(k2 = 0; k2 < core.n_var; ++k2){
					base_x[k2] = x[k2];
				}
				phi_current = phi_after;
				max_current = max_after;
				last_alpha = alpha;
				inner_accepted = 1;
				accepted = 1;
				break;
			}
		}
		if(!inner_accepted){
			for(j = 0; j < core.n_var; ++j){
				x[j] = base_x[j];
			}
			(void)line_ops->evaluate(ctx,x,view);
			break;
		}
	}

cleanup:
	if(!accepted && start_x != NULL){
		for(i = 0; i < core.n_var; ++i){
			x[i] = start_x[i];
		}
		(void)line_ops->evaluate(ctx,x,view);
	}else if(accepted){
		for(i = 0; i < core.n_var; ++i){
			real64 step = x[i] - start_x[i];
			real64 scaled_step_value = core.var_scale != NULL && core.var_scale[i] != 0.0 ? step / core.var_scale[i] : step;
			net_step_norm2 += step * step;
			if(fabs(scaled_step_value) > net_scaled_step_inf){
				net_scaled_step_inf = fabs(scaled_step_value);
			}
		}
		a4sqp_view_get_core(view,&trial_core);
		phi_final = a4sqp_core_violation_squares(&trial_core,&max_final);
		(void)max_final;
		if(line_ops->accepted != NULL){
			line_ops->accepted(ctx,start_scaled_x,start_scaled_grad,1);
		}
		memset(result,0,sizeof(*result));
		result->accepted = 1;
		result->trials = total_trials;
		result->merit_before = phi_start;
		result->merit_after = phi_final;
		result->model_merit_after = phi_final;
		result->predicted_reduction = phi_start;
		result->alpha = last_alpha;
		result->step_norm = sqrt(net_step_norm2);
		result->trust_ratio = phi_start > 0.0 ? (phi_start - phi_final) / phi_start : 0.0;
		result->scaled_step_inf = net_scaled_step_inf;
	}
	A4SQP_FREE(start_x);
	A4SQP_FREE(base_x);
	A4SQP_FREE(start_scaled_x);
	A4SQP_FREE(start_scaled_grad);
	A4SQP_FREE(scaled_step);
	A4SQP_FREE(physical_step);
	return accepted ? 0 : 1;
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
	enum A4SqpCorePhase previous_phase = A4SQP_CORE_PHASE_REGULAR;
	enum A4SqpCorePhase current_phase = A4SQP_CORE_PHASE_REGULAR;
	struct A4SqpCoreView initial_view;
	struct A4SqpLineSearchOptions effective_line_options;
	int force_unconstrained_trust = 0;

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
	if(options->restoration_state != NULL){
		previous_phase = options->restoration_state->phase;
	}
	a4sqp_view_get_core(view,&initial_view);
	restoration_active = a4sqp_core_restoration_choose(
		&initial_view,
		&options->restoration,
		options->feas_tol,
		options->restoration_state
	);
	if(options->restoration_state != NULL){
		current_phase = options->restoration_state->phase;
	}
	if(stats != NULL){
		stats->phase = current_phase;
		stats->phase_changed = current_phase != previous_phase;
		if(current_phase == A4SQP_CORE_PHASE_RESTORATION){
			++stats->restoration_iterations;
		}else{
			++stats->regular_iterations;
		}
		if(current_phase == A4SQP_CORE_PHASE_RESTORATION && previous_phase != A4SQP_CORE_PHASE_RESTORATION){
			++stats->restoration_entries;
		}
		if(current_phase == A4SQP_CORE_PHASE_RESTORATION_EXIT){
			++stats->restoration_exits;
			if(options->restoration_state->handoff){
				++stats->restoration_handoffs;
			}
		}
	}
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
		int use_trust_region;
		a4sqp_view_get_core(view,&core_view);
		use_trust_region = core_view.n_rel > 0 || options->trust_unconstrained || force_unconstrained_trust;
		memset(&qp_options,0,sizeof(qp_options));
		qp_options.trust_radius = use_trust_region ? *trust_radius : 0.0;
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
				ops->after_qp_solve(ctx,qp,restoration_active);
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
					if(
						restoration_active
						&& line_search_result != NULL
						&& line_search_result->accepted
						&& line_search_result->alpha > 0.0
						&& line_search_result->alpha <= 1.0 / 1024.0
					){
						struct A4SqpLineSearchResult nonlinear_result;
						memset(&nonlinear_result,0,sizeof(nonlinear_result));
						if(a4sqp_core_nonlinear_restoration_step(
							view,
							&effective_line_options,
							line_search_ops,
							ctx,
							x,
							*trust_radius,
							&nonlinear_result
						) == 0){
							*line_search_result = nonlinear_result;
						}
					}
					if(
						line_search_result != NULL
						&& line_search_result->accepted
						&& line_search_result->alpha > 0.0
					){
						(void)a4sqp_trust_update_after_accept(
							trust_radius,
							&options->trust,
							line_search_result->alpha,
							line_search_result->scaled_step_inf,
							line_search_result->trust_ratio,
							options->trust_tiny_alpha,
							options->trust_tiny_radius_factor
						);
					}
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
				if(
					view->n_rel <= 0
					&& !options->trust_unconstrained
					&& !force_unconstrained_trust
					&& effective_has_objective
				){
					force_unconstrained_trust = 1;
					continue;
				}
				if(a4sqp_core_step_may_retry(view,attempt,options,trust_radius,stats,force_unconstrained_trust)){
					continue;
				}
			return last_error;
		}
		last_error = A4SQP_CORE_STEP_QP_ERROR;
		if(stats != NULL){
			++stats->qp_failures;
		}
			if(
				view->n_rel <= 0
				&& !options->trust_unconstrained
				&& !force_unconstrained_trust
				&& effective_has_objective
			){
				force_unconstrained_trust = 1;
				continue;
			}
			if(a4sqp_core_step_may_retry(view,attempt,options,trust_radius,stats,force_unconstrained_trust)){
				continue;
			}
		return last_error;
	}
	return last_error;
}
