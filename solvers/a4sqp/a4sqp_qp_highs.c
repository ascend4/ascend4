/*
 * Small HiGHS QP exercises used while bringing up the A4SQP backend.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_qp_highs.h"

#include "a4sqp_view.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <interfaces/highs_c_api.h>

#ifdef A4SQP_DEBUG
# define MSG(...) fprintf(stderr,__VA_ARGS__)
#else
# define MSG(...)
#endif

static int a4sqp_qp_is_lower_inf(real64 value){
	return value <= A4SQP_NO_LOWER_BOUND / 10.0;
}

static int a4sqp_qp_is_upper_inf(real64 value){
	return value >= A4SQP_NO_UPPER_BOUND / 10.0;
}

static real64 a4sqp_qp_snap_small_bound(real64 value, real64 tol){
	if(!isfinite(value)){
		return value;
	}
	if(tol > 0.0 && fabs(value) <= tol){
		return 0.0;
	}
	return value;
}

static real64 a4sqp_qp_to_highs_bound(real64 value, real64 inf){
	if(a4sqp_qp_is_lower_inf(value)){
		return -inf;
	}
	if(a4sqp_qp_is_upper_inf(value)){
		return inf;
	}
	return value;
}

void a4sqp_qp_init(struct A4SqpQp *qp){
	if(qp == NULL){
		return;
	}
	memset(qp,0,sizeof(*qp));
	qp->highs_status = kHighsStatusError;
	qp->highs_model_status = kHighsModelStatusNotset;
}

void a4sqp_qp_destroy(struct A4SqpQp *qp){
	if(qp == NULL){
		return;
	}
	A4SQP_FREE(qp->col_kind);
	A4SQP_FREE(qp->col_var_index);
	A4SQP_FREE(qp->col_rel_index);
	A4SQP_FREE(qp->row_rel_index);
	A4SQP_FREE(qp->col_cost);
	A4SQP_FREE(qp->col_lower);
	A4SQP_FREE(qp->col_upper);
	A4SQP_FREE(qp->row_lower);
	A4SQP_FREE(qp->row_upper);
	A4SQP_FREE(qp->a_start);
	A4SQP_FREE(qp->a_index);
	A4SQP_FREE(qp->a_value);
	A4SQP_FREE(qp->q_start);
	A4SQP_FREE(qp->q_index);
	A4SQP_FREE(qp->q_value);
	A4SQP_FREE(qp->col_value);
	A4SQP_FREE(qp->col_dual);
	A4SQP_FREE(qp->row_value);
	A4SQP_FREE(qp->row_dual);
	a4sqp_qp_init(qp);
}

static int a4sqp_qp_alloc(struct A4SqpQp *qp){
	int32 row_alloc;
	if(qp == NULL){
		return 1;
	}
	row_alloc = qp->num_row > 0 ? qp->num_row : 1;
	qp->col_kind = A4SQP_NEW_ARRAY_OR_NULL(uint32,qp->num_col);
	qp->col_var_index = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	qp->col_rel_index = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	qp->row_rel_index = A4SQP_NEW_ARRAY_OR_NULL(int32,row_alloc);
	qp->col_cost = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->col_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->col_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->row_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,row_alloc);
	qp->row_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,row_alloc);
	qp->a_start = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->num_col + 1);
	qp->a_index = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->num_nz > 0 ? qp->num_nz : 1);
	qp->a_value = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_nz > 0 ? qp->num_nz : 1);
	qp->q_start = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->num_col + 1);
	qp->q_index = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->q_num_nz > 0 ? qp->q_num_nz : 1);
	qp->q_value = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->q_num_nz > 0 ? qp->q_num_nz : 1);
	qp->col_value = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->col_dual = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->row_value = A4SQP_NEW_ARRAY_OR_NULL(real64,row_alloc);
	qp->row_dual = A4SQP_NEW_ARRAY_OR_NULL(real64,row_alloc);

	if(qp->col_kind == NULL || qp->col_var_index == NULL || qp->col_rel_index == NULL
		|| qp->row_rel_index == NULL || qp->col_cost == NULL || qp->col_lower == NULL
		|| qp->col_upper == NULL || qp->row_lower == NULL || qp->row_upper == NULL
		|| qp->a_start == NULL || qp->a_index == NULL || qp->a_value == NULL
		|| qp->q_start == NULL || qp->q_index == NULL || qp->q_value == NULL
		|| qp->col_value == NULL || qp->col_dual == NULL || qp->row_value == NULL
		|| qp->row_dual == NULL
	){
		return 1;
	}
	return 0;
}

static void a4sqp_qp_init_col_metadata(struct A4SqpQp *qp){
	int32 i;
	for(i = 0; i < qp->num_col; ++i){
		qp->col_kind[i] = 0;
		qp->col_var_index[i] = -1;
		qp->col_rel_index[i] = -1;
		qp->col_cost[i] = 0.0;
		qp->col_lower[i] = 0.0;
		qp->col_upper[i] = 0.0;
		qp->col_value[i] = 0.0;
		qp->col_dual[i] = 0.0;
	}
	for(i = 0; i < qp->num_row; ++i){
		qp->row_rel_index[i] = i;
		qp->row_value[i] = 0.0;
		qp->row_dual[i] = 0.0;
	}
}

static int a4sqp_qp_fill_matrix(const struct A4SqpCoreView *view, struct A4SqpQp *qp){
	int32 r;
	int32 k;
	int32 c;
	int32 *counts;
	int32 *next;

	counts = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	next = A4SQP_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	if(counts == NULL || next == NULL){
		A4SQP_FREE(counts);
		A4SQP_FREE(next);
		return 1;
	}
	for(c = 0; c < qp->num_col; ++c){
		counts[c] = 0;
	}
	for(r = 0; r < view->n_rel; ++r){
		for(k = view->jac_row_start[r]; k < view->jac_row_start[r + 1]; ++k){
			c = view->jac_col_index != NULL ? view->jac_col_index[k] : -1;
			if(c >= 0){
				++counts[c];
			}
		}
		++counts[view->n_var + 2 * r];
		++counts[view->n_var + 2 * r + 1];
	}

	qp->a_start[0] = 0;
	for(c = 0; c < qp->num_col; ++c){
		qp->a_start[c + 1] = qp->a_start[c] + counts[c];
		next[c] = qp->a_start[c];
	}

	for(r = 0; r < view->n_rel; ++r){
		for(k = view->jac_row_start[r]; k < view->jac_row_start[r + 1]; ++k){
			c = view->jac_col_index != NULL ? view->jac_col_index[k] : -1;
			if(c >= 0){
				int32 pos = next[c]++;
				qp->a_index[pos] = r;
				qp->a_value[pos] = view->scaled_jac_value[k];
			}
		}
		c = view->n_var + 2 * r;
		qp->a_index[next[c]] = r;
		qp->a_value[next[c]] = 1.0;
		++next[c];
		c = view->n_var + 2 * r + 1;
		qp->a_index[next[c]] = r;
		qp->a_value[next[c]] = -1.0;
		++next[c];
	}

	A4SQP_FREE(counts);
	A4SQP_FREE(next);
	return 0;
}

static int32 a4sqp_qp_count_hessian_nz(const struct A4SqpCoreView *view, const struct A4SqpStepHessian *step_hess){
	int32 col;
	int32 row;
	int32 nnz = 0;
	if(view == NULL){
		return 0;
	}
	if(
		step_hess != NULL
		&& step_hess->is_sparse
		&& step_hess->start != NULL
		&& step_hess->index != NULL
		&& step_hess->value != NULL
	){
		return step_hess->nnz;
	}
	for(col = 0; col < view->n_var; ++col){
		for(row = col; row < view->n_var; ++row){
			real64 value = (step_hess != NULL && step_hess->dense != NULL)
				? step_hess->dense[row * view->n_var + col]
				: (row == col ? 1.0 : 0.0);
			if(fabs(value) > 1e-14){
				++nnz;
			}
		}
	}
	return nnz;
}

static void a4sqp_qp_fill_hessian(
	const struct A4SqpCoreView *view,
	const struct A4SqpStepHessian *step_hess,
	struct A4SqpQp *qp
){
	int32 col;
	int32 row;
	int32 nnz = 0;
	for(col = 0; col < qp->num_col; ++col){
		qp->q_start[col] = nnz;
		if(col < view->n_var){
			if(
				step_hess != NULL
				&& step_hess->is_sparse
				&& step_hess->start != NULL
				&& step_hess->index != NULL
				&& step_hess->value != NULL
			){
				int32 k;
				for(k = step_hess->start[col]; k < step_hess->start[col + 1]; ++k){
					qp->q_index[nnz] = step_hess->index[k];
					qp->q_value[nnz] = step_hess->value[k];
					++nnz;
				}
			}else{
				for(row = col; row < view->n_var; ++row){
					real64 value = (step_hess != NULL && step_hess->dense != NULL)
						? step_hess->dense[row * view->n_var + col]
						: (row == col ? 1.0 : 0.0);
					if(fabs(value) <= 1e-14){
						continue;
					}
					qp->q_index[nnz] = row;
					qp->q_value[nnz] = value;
					++nnz;
				}
			}
		}
	}
	qp->q_start[qp->num_col] = nnz;
}

int a4sqp_qp_build_from_core_view(
	struct A4SqpQp *qp,
	const struct A4SqpCoreView *view,
	const struct A4SqpStepHessian *step_hess,
	real64 trust_radius,
	real64 elastic_penalty,
	real64 feas_tol
){
	int32 i;
	if(qp == NULL || view == NULL){
		return 1;
	}

	a4sqp_qp_destroy(qp);
	qp->num_step_col = view->n_var;
	qp->num_elastic_pair = view->n_rel;
	qp->num_col = view->n_var + 2 * view->n_rel;
	qp->num_row = view->n_rel;
	qp->num_nz = view->jac_nnz + 2 * view->n_rel;
	qp->q_num_nz = a4sqp_qp_count_hessian_nz(view,step_hess);
	if(a4sqp_qp_alloc(qp)){
		a4sqp_qp_destroy(qp);
		return 1;
	}
	a4sqp_qp_init_col_metadata(qp);

	for(i = 0; i < view->n_var; ++i){
		qp->col_kind[i] = A4SQP_QP_COL_STEP;
		qp->col_var_index[i] = i;
		qp->col_cost[i] = view->scaled_obj_gradient != NULL ? view->scaled_obj_gradient[i] : 0.0;
		qp->col_lower[i] = a4sqp_qp_is_lower_inf(view->scaled_var_lower[i])
			? A4SQP_NO_LOWER_BOUND
			: view->scaled_var_lower[i] - view->scaled_var_value[i];
		qp->col_upper[i] = a4sqp_qp_is_upper_inf(view->scaled_var_upper[i])
			? A4SQP_NO_UPPER_BOUND
			: view->scaled_var_upper[i] - view->scaled_var_value[i];
		qp->col_lower[i] = a4sqp_qp_is_lower_inf(qp->col_lower[i])
			? qp->col_lower[i]
			: a4sqp_qp_snap_small_bound(qp->col_lower[i],feas_tol);
		qp->col_upper[i] = a4sqp_qp_is_upper_inf(qp->col_upper[i])
			? qp->col_upper[i]
			: a4sqp_qp_snap_small_bound(qp->col_upper[i],feas_tol);
		if(
			!a4sqp_qp_is_lower_inf(qp->col_lower[i])
			&& !a4sqp_qp_is_upper_inf(qp->col_upper[i])
			&& qp->col_lower[i] > qp->col_upper[i]
			&& qp->col_lower[i] - qp->col_upper[i] <= 2.0 * feas_tol
		){
			real64 midpoint = 0.5 * (qp->col_lower[i] + qp->col_upper[i]);
			qp->col_lower[i] = midpoint;
			qp->col_upper[i] = midpoint;
		}
		if(trust_radius > 0.0 && isfinite(trust_radius)){
			real64 local_radius = trust_radius;
			if(!a4sqp_qp_is_lower_inf(qp->col_lower[i]) && qp->col_lower[i] > local_radius){
				local_radius = qp->col_lower[i];
			}
			if(!a4sqp_qp_is_upper_inf(qp->col_upper[i]) && -qp->col_upper[i] > local_radius){
				local_radius = -qp->col_upper[i];
			}
			if(a4sqp_qp_is_lower_inf(qp->col_lower[i]) || qp->col_lower[i] < -local_radius){
				qp->col_lower[i] = -local_radius;
			}
			if(a4sqp_qp_is_upper_inf(qp->col_upper[i]) || qp->col_upper[i] > local_radius){
				qp->col_upper[i] = local_radius;
			}
		}
	}

	for(i = 0; i < view->n_rel; ++i){
		int32 lower_col = view->n_var + 2 * i;
		int32 upper_col = lower_col + 1;

		qp->row_lower[i] = a4sqp_qp_is_lower_inf(view->scaled_rel_lower[i])
			? A4SQP_NO_LOWER_BOUND
			: view->scaled_rel_lower[i] - view->scaled_rel_residual[i];
		qp->row_upper[i] = a4sqp_qp_is_upper_inf(view->scaled_rel_upper[i])
			? A4SQP_NO_UPPER_BOUND
			: view->scaled_rel_upper[i] - view->scaled_rel_residual[i];
		qp->row_lower[i] = a4sqp_qp_is_lower_inf(qp->row_lower[i])
			? qp->row_lower[i]
			: a4sqp_qp_snap_small_bound(qp->row_lower[i],feas_tol);
		qp->row_upper[i] = a4sqp_qp_is_upper_inf(qp->row_upper[i])
			? qp->row_upper[i]
			: a4sqp_qp_snap_small_bound(qp->row_upper[i],feas_tol);
		if(
			!a4sqp_qp_is_lower_inf(qp->row_lower[i])
			&& !a4sqp_qp_is_upper_inf(qp->row_upper[i])
			&& qp->row_lower[i] > qp->row_upper[i]
			&& qp->row_lower[i] - qp->row_upper[i] <= 2.0 * feas_tol
		){
			real64 midpoint = 0.5 * (qp->row_lower[i] + qp->row_upper[i]);
			qp->row_lower[i] = midpoint;
			qp->row_upper[i] = midpoint;
		}

		qp->col_kind[lower_col] = A4SQP_QP_COL_ELASTIC_LOWER;
		qp->col_rel_index[lower_col] = i;
		qp->col_cost[lower_col] = elastic_penalty;
		qp->col_lower[lower_col] = 0.0;
		qp->col_upper[lower_col] = A4SQP_NO_UPPER_BOUND;

		qp->col_kind[upper_col] = A4SQP_QP_COL_ELASTIC_UPPER;
		qp->col_rel_index[upper_col] = i;
		qp->col_cost[upper_col] = elastic_penalty;
		qp->col_lower[upper_col] = 0.0;
		qp->col_upper[upper_col] = A4SQP_NO_UPPER_BOUND;
	}

	if(a4sqp_qp_fill_matrix(view,qp)){
		a4sqp_qp_destroy(qp);
		return 1;
	}
	a4sqp_qp_fill_hessian(view,step_hess,qp);
	return 0;
}

int a4sqp_qp_build_from_view(
	struct A4SqpQp *qp,
	const struct A4SqpView *view,
	const struct A4SqpStepHessian *step_hess,
	real64 trust_radius,
	real64 elastic_penalty,
	real64 feas_tol
){
	struct A4SqpCoreView core;
	a4sqp_view_get_core(view,&core);
	return a4sqp_qp_build_from_core_view(qp,&core,step_hess,trust_radius,elastic_penalty,feas_tol);
}

static HighsInt *a4sqp_qp_copy_int_array(const int32 *src, int32 len){
	int32 i;
	HighsInt *dest = A4SQP_NEW_ARRAY_OR_NULL(HighsInt,len > 0 ? len : 1);
	if(dest == NULL){
		return NULL;
	}
	for(i = 0; i < len; ++i){
		dest[i] = (HighsInt)src[i];
	}
	return dest;
}

int a4sqp_qp_solve_highs(struct A4SqpQp *qp, real64 qp_tol, int output_flag){
	void *highs;
	HighsInt status;
	HighsInt model_status;
	HighsInt *a_start;
	HighsInt *a_index;
	HighsInt *q_start;
	HighsInt *q_index;
	real64 *col_lower;
	real64 *col_upper;
	real64 *row_lower;
	real64 *row_upper;
	real64 inf;
	int32 i;

	if(qp == NULL || qp->num_col <= 0 || qp->num_row < 0){
		return 1;
	}

	highs = Highs_create();
	if(highs == NULL){
		return 1;
	}
	inf = Highs_getInfinity(highs);

	a_start = a4sqp_qp_copy_int_array(qp->a_start,qp->num_col + 1);
	a_index = a4sqp_qp_copy_int_array(qp->a_index,qp->num_nz);
	q_start = a4sqp_qp_copy_int_array(qp->q_start,qp->num_col + 1);
	q_index = a4sqp_qp_copy_int_array(qp->q_index,qp->q_num_nz);
	col_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	col_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	row_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_row > 0 ? qp->num_row : 1);
	row_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,qp->num_row > 0 ? qp->num_row : 1);
	if(a_start == NULL || a_index == NULL || q_start == NULL || q_index == NULL
		|| col_lower == NULL || col_upper == NULL || row_lower == NULL || row_upper == NULL
	){
		A4SQP_FREE(a_start);
		A4SQP_FREE(a_index);
		A4SQP_FREE(q_start);
		A4SQP_FREE(q_index);
		A4SQP_FREE(col_lower);
		A4SQP_FREE(col_upper);
		A4SQP_FREE(row_lower);
		A4SQP_FREE(row_upper);
		Highs_destroy(highs);
		return 1;
	}
	for(i = 0; i < qp->num_col; ++i){
		col_lower[i] = a4sqp_qp_to_highs_bound(qp->col_lower[i],inf);
		col_upper[i] = a4sqp_qp_to_highs_bound(qp->col_upper[i],inf);
	}
	for(i = 0; i < qp->num_row; ++i){
		row_lower[i] = a4sqp_qp_to_highs_bound(qp->row_lower[i],inf);
		row_upper[i] = a4sqp_qp_to_highs_bound(qp->row_upper[i],inf);
	}

	MSG("Solving QP...");

	(void)Highs_setBoolOptionValue(
		highs,
		"output_flag",
		output_flag ? 1 : 0
	);
	if(qp_tol < 1e-4){
		qp_tol = 1e-4;
	}
	(void)Highs_setDoubleOptionValue(highs,"kkt_tolerance",qp_tol);
	(void)Highs_setDoubleOptionValue(highs,"primal_feasibility_tolerance",qp_tol);
	(void)Highs_setDoubleOptionValue(highs,"dual_feasibility_tolerance",qp_tol);
	(void)Highs_setDoubleOptionValue(highs,"optimality_tolerance",qp_tol);
	(void)Highs_setIntOptionValue(highs,"user_bound_scale",8);
	status = Highs_passModel(
		highs,
		(HighsInt)qp->num_col,
		(HighsInt)qp->num_row,
		(HighsInt)qp->num_nz,
		(HighsInt)qp->q_num_nz,
		kHighsMatrixFormatColwise,
		kHighsHessianFormatTriangular,
		kHighsObjSenseMinimize,
		0.0,
		qp->col_cost,
		col_lower,
		col_upper,
		row_lower,
		row_upper,
		a_start,
		a_index,
		qp->a_value,
		q_start,
		q_index,
		qp->q_value,
		NULL
	);
	if(status != kHighsStatusError){
		status = Highs_run(highs);
	}

	MSG("Getting QP status...");

	model_status = Highs_getModelStatus(highs);
	qp->highs_status = (int)status;
	qp->highs_model_status = (int)model_status;
	qp->objective_value = 0.0;
	if(status != kHighsStatusError && model_status == kHighsModelStatusOptimal){
		if(Highs_getSolution(highs,qp->col_value,qp->col_dual,qp->row_value,qp->row_dual) == kHighsStatusError){
			status = kHighsStatusError;
			qp->highs_status = (int)status;
		}else{
				for(i = 0; i < qp->num_col; ++i){
					qp->objective_value += qp->col_cost[i] * qp->col_value[i];
				}
				for(i = 0; i < qp->num_step_col; ++i){
					int32 k;
					for(k = qp->q_start[i]; k < qp->q_start[i + 1]; ++k){
						int32 row = qp->q_index[k];
						if(row >= 0 && row < qp->num_col){
							real64 coeff = (row == i) ? 0.5 : 1.0;
							qp->objective_value += coeff * qp->q_value[k] * qp->col_value[i] * qp->col_value[row];
						}
					}
				}
			}
				}

	A4SQP_FREE(a_start);
	A4SQP_FREE(a_index);
	A4SQP_FREE(q_start);
	A4SQP_FREE(q_index);
	A4SQP_FREE(col_lower);
	A4SQP_FREE(col_upper);
	A4SQP_FREE(row_lower);
	A4SQP_FREE(row_upper);
	Highs_destroy(highs);
	return (status != kHighsStatusError && model_status == kHighsModelStatusOptimal) ? 0 : 1;
}

int a4sqp_qp_highs_spike(struct A4SqpQpSpikeResult *result){
	void *highs;
	HighsInt status;
	HighsInt model_status;
	double inf;
	const HighsInt num_col = 3;
	const HighsInt num_row = 2;
	const HighsInt num_nz = 4;
	const HighsInt q_num_nz = 1;
	const double col_cost[3] = {0.0, 10.0, 10.0};
	double col_lower[3];
	double col_upper[3];
	double row_lower[2];
	double row_upper[2];
	const HighsInt a_start[4] = {0, 2, 3, 4};
	const HighsInt a_index[4] = {0, 1, 0, 1};
	const double a_value[4] = {1.0, 1.0, 1.0, -1.0};
	const HighsInt q_start[4] = {0, 1, 1, 1};
	const HighsInt q_index[1] = {0};
	const double q_value[1] = {1.0};
	double col_value[3] = {0.0, 0.0, 0.0};
	double col_dual[3] = {0.0, 0.0, 0.0};
	double row_value[2] = {0.0, 0.0};
	double row_dual[2] = {0.0, 0.0};

	if(result == NULL){
		return 1;
	}
	memset(result,0,sizeof(*result));
	result->highs_status = kHighsStatusError;
	result->highs_model_status = kHighsModelStatusNotset;

	highs = Highs_create();
	if(highs == NULL){
		return 1;
	}

	inf = Highs_getInfinity(highs);
	col_lower[0] = 0.0;
	col_lower[1] = 0.0;
	col_lower[2] = 0.0;
	col_upper[0] = 0.25;
	col_upper[1] = inf;
	col_upper[2] = inf;
	row_lower[0] = 1.0;
	row_upper[0] = inf;
	row_lower[1] = -inf;
	row_upper[1] = 2.0;

	(void)Highs_setBoolOptionValue(highs,"output_flag",0);

	status = Highs_passModel(
		highs,
		num_col,
		num_row,
		num_nz,
		q_num_nz,
		kHighsMatrixFormatColwise,
		kHighsHessianFormatTriangular,
		kHighsObjSenseMinimize,
		0.0,
		col_cost,
		col_lower,
		col_upper,
		row_lower,
		row_upper,
		a_start,
		a_index,
		a_value,
		q_start,
		q_index,
		q_value,
		NULL
	);
	if(status != kHighsStatusError){
		status = Highs_run(highs);
	}

	model_status = Highs_getModelStatus(highs);
	result->highs_status = (int)status;
	result->highs_model_status = (int)model_status;

	if(status != kHighsStatusError && model_status == kHighsModelStatusOptimal){
		if(Highs_getSolution(highs,col_value,col_dual,row_value,row_dual) == kHighsStatusError){
			Highs_destroy(highs);
			return 1;
		}
		memcpy(result->col_value,col_value,sizeof(col_value));
		memcpy(result->col_dual,col_dual,sizeof(col_dual));
		memcpy(result->row_value,row_value,sizeof(row_value));
		memcpy(result->row_dual,row_dual,sizeof(row_dual));
		result->objective_value =
			0.5 * col_value[0] * col_value[0]
			+ 10.0 * col_value[1]
			+ 10.0 * col_value[2];
		Highs_destroy(highs);
		return 0;
	}

	Highs_destroy(highs);
	return 1;
}
