/*
 * Small HiGHS QP exercises used while bringing up the A4SQP backend.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_qp_highs.h"

#include "a4sqp_diag.h"
#include "a4sqp_params.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/solver/solver.h>
#include <ascend/system/var.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <interfaces/highs_c_api.h>

#define A4SQP_HIGHS_PROGRESS_INTERVAL 5.0

#include <ascend/utilities/error.h>
#ifdef A4SQP_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...)
#endif

struct A4SqpHighsProgress {
	slv_parameters_t *params;
	double next_report_time;
	int report_count;
};

static int a4sqp_highs_total_iteration_count(const HighsCallbackDataOut *data_out){
	long long total = 0;
	if(data_out == NULL)return 0;
	if(data_out->simplex_iteration_count > 0){
		total += (long long)data_out->simplex_iteration_count;
	}
	if(data_out->ipm_iteration_count > 0){
		total += (long long)data_out->ipm_iteration_count;
	}
	if(data_out->pdlp_iteration_count > 0){
		total += (long long)data_out->pdlp_iteration_count;
	}
	if(total > INT_MAX)return INT_MAX;
	if(total < 0)return 0;
	return (int)total;
}

static void a4sqp_highs_report_callback(
	struct A4SqpHighsProgress *progress,
	int callback_type,
	const HighsCallbackDataOut *data_out
){
	char message[256];
	const char *tag = "highs";
	double running_time = 0.0;
	int iter;

	if(progress == NULL || data_out == NULL){
		return;
	}
	if(callback_type == kHighsCallbackLogging){
		return;
	}
	switch(callback_type){
		case kHighsCallbackSimplexInterrupt: tag = "highs_simplex"; break;
		case kHighsCallbackIpmInterrupt: tag = "highs_ipm"; break;
		case kHighsCallbackLogging: tag = "highs_log"; break;
		default: break;
	}
	if(isfinite(data_out->running_time) && data_out->running_time >= 0.0){
		running_time = data_out->running_time;
	}
	if(progress->report_count > 0 && running_time < progress->next_report_time){
		return;
	}
	iter = a4sqp_highs_total_iteration_count(data_out);
	if(isfinite(data_out->objective_function_value)){
		snprintf(message,sizeof(message),
			"qp_solve: tag=%s, t=%.3gs, iter=%d, obj=%.17g",
			tag,running_time,iter,data_out->objective_function_value
		);
	}else{
		snprintf(message,sizeof(message),
			"qp_solve: tag=%s, t=%.3gs, iter=%d",
			tag,running_time,iter
		);
	}
	a4sqp_report_progress(progress->params,message);
	progress->next_report_time = running_time + A4SQP_HIGHS_PROGRESS_INTERVAL;
	++progress->report_count;
}

static void a4sqp_highs_callback(
	int callback_type,
	const char *message,
	const HighsCallbackDataOut *data_out,
	HighsCallbackDataIn *data_in,
	void *user_data
){
	struct A4SqpHighsProgress *progress = (struct A4SqpHighsProgress *)user_data;
	(void)message;
	if(progress != NULL && data_out != NULL){
		a4sqp_highs_report_callback(progress,callback_type,data_out);
	}
	if(data_in != NULL && slv_get_solver_interrupt()){
		data_in->user_interrupt = 1;
	}
}

static void a4sqp_highs_enable_callbacks(void *highs, struct A4SqpHighsProgress *progress){
	HighsInt status;
	int progress_enabled;
	if(highs == NULL || progress == NULL){
		return;
	}
	progress_enabled = (
		progress->params == NULL
		|| SLV_PARAM_BOOL(progress->params,A4SQP_PARAM_PROGRESS_CALLBACKS)
		|| SLV_PARAM_BOOL(progress->params,A4SQP_PARAM_PROGRESS_LOG)
	);
	status = Highs_setCallback(highs,&a4sqp_highs_callback,(void *)progress);
	if(status == kHighsStatusError){
		a4sqp_report_progress(progress->params,"qp_solve: unable to install HiGHS callback");
		return;
	}
	(void)Highs_startCallback(highs,kHighsCallbackSimplexInterrupt);
	(void)Highs_startCallback(highs,kHighsCallbackIpmInterrupt);
	if(progress_enabled){
		(void)Highs_startCallback(highs,kHighsCallbackLogging);
	}
}

static void a4sqp_qp_report_failure(struct A4SqpQp *qp, slv_parameters_t *params){
	char message[256];
	int32 col;
	int32 row;
	if(qp == NULL){
		return;
	}
	snprintf(
		message,
		sizeof(message),
		"qp_failure: cols=%ld rows=%ld nnz=%ld q_nnz=%ld status=%d model_status=%d",
		(long)qp->num_col,
		(long)qp->num_row,
		(long)qp->num_nz,
		(long)qp->q_num_nz,
		qp->highs_status,
		qp->highs_model_status
	);
	a4sqp_report_progress(params,message);
	if(qp->num_col > 16 || qp->num_row > 16){
		return;
	}
	for(col = 0; col < qp->num_col; ++col){
		snprintf(
			message,
			sizeof(message),
			"qp_failure: col[%ld] cost=%g lower=%g upper=%g kind=%u var=%ld rel=%ld",
			(long)col,
			qp->col_cost[col],
			qp->col_lower[col],
			qp->col_upper[col],
			qp->col_kind != NULL ? (unsigned)qp->col_kind[col] : 0U,
			(long)(qp->col_var_index != NULL ? qp->col_var_index[col] : -1),
			(long)(qp->col_rel_index != NULL ? qp->col_rel_index[col] : -1)
		);
		a4sqp_report_progress(params,message);
	}
	for(row = 0; row < qp->num_row; ++row){
		snprintf(
			message,
			sizeof(message),
			"qp_failure: row[%ld] lower=%g upper=%g rel=%ld value=%g dual=%g",
			(long)row,
			qp->row_lower[row],
			qp->row_upper[row],
			(long)(qp->row_rel_index != NULL ? qp->row_rel_index[row] : -1),
			qp->row_value != NULL ? qp->row_value[row] : 0.0,
			qp->row_dual != NULL ? qp->row_dual[row] : 0.0
		);
		a4sqp_report_progress(params,message);
	}
	for(col = 0; col < qp->num_col; ++col){
		int32 k;
		for(k = qp->a_start[col]; k < qp->a_start[col + 1]; ++k){
			snprintf(
				message,
				sizeof(message),
				"qp_failure: a[%ld,%ld]=%g",
				(long)qp->a_index[k],
				(long)col,
				qp->a_value[k]
			);
			a4sqp_report_progress(params,message);
		}
	}
	for(col = 0; col < qp->num_step_col; ++col){
		int32 k;
		for(k = qp->q_start[col]; k < qp->q_start[col + 1]; ++k){
			snprintf(
				message,
				sizeof(message),
				"qp_failure: q[%ld,%ld]=%g",
				(long)qp->q_index[k],
				(long)col,
				qp->q_value[k]
			);
			a4sqp_report_progress(params,message);
		}
	}
}

static int a4sqp_qp_is_lower_inf(real64 value){
	return value <= var_NO_LOWER_BOUND / 10.0;
}

static int a4sqp_qp_is_upper_inf(real64 value){
	return value >= var_NO_UPPER_BOUND / 10.0;
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

static int32 a4sqp_qp_find_var_index(const struct A4SqpView *view, int32 sindex){
	int32 i;
	for(i = 0; i < view->n_var; ++i){
		if(view->var_sindex[i] == sindex){
			return i;
		}
	}
	return -1;
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
	ASC_FREE(qp->col_kind);
	ASC_FREE(qp->col_var_index);
	ASC_FREE(qp->col_rel_index);
	ASC_FREE(qp->row_rel_index);
	ASC_FREE(qp->col_cost);
	ASC_FREE(qp->col_lower);
	ASC_FREE(qp->col_upper);
	ASC_FREE(qp->row_lower);
	ASC_FREE(qp->row_upper);
	ASC_FREE(qp->a_start);
	ASC_FREE(qp->a_index);
	ASC_FREE(qp->a_value);
	ASC_FREE(qp->q_start);
	ASC_FREE(qp->q_index);
	ASC_FREE(qp->q_value);
	ASC_FREE(qp->col_value);
	ASC_FREE(qp->col_dual);
	ASC_FREE(qp->row_value);
	ASC_FREE(qp->row_dual);
	a4sqp_qp_init(qp);
}

static int a4sqp_qp_alloc(struct A4SqpQp *qp){
	int32 row_alloc;
	if(qp == NULL){
		return 1;
	}
	row_alloc = qp->num_row > 0 ? qp->num_row : 1;
	qp->col_kind = ASC_NEW_ARRAY_OR_NULL(uint32,qp->num_col);
	qp->col_var_index = ASC_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	qp->col_rel_index = ASC_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	qp->row_rel_index = ASC_NEW_ARRAY_OR_NULL(int32,row_alloc);
	qp->col_cost = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->col_lower = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->col_upper = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->row_lower = ASC_NEW_ARRAY_OR_NULL(real64,row_alloc);
	qp->row_upper = ASC_NEW_ARRAY_OR_NULL(real64,row_alloc);
	qp->a_start = ASC_NEW_ARRAY_OR_NULL(int32,qp->num_col + 1);
	qp->a_index = ASC_NEW_ARRAY_OR_NULL(int32,qp->num_nz > 0 ? qp->num_nz : 1);
	qp->a_value = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_nz > 0 ? qp->num_nz : 1);
	qp->q_start = ASC_NEW_ARRAY_OR_NULL(int32,qp->num_col + 1);
	qp->q_index = ASC_NEW_ARRAY_OR_NULL(int32,qp->q_num_nz > 0 ? qp->q_num_nz : 1);
	qp->q_value = ASC_NEW_ARRAY_OR_NULL(real64,qp->q_num_nz > 0 ? qp->q_num_nz : 1);
	qp->col_value = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->col_dual = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	qp->row_value = ASC_NEW_ARRAY_OR_NULL(real64,row_alloc);
	qp->row_dual = ASC_NEW_ARRAY_OR_NULL(real64,row_alloc);

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

static int a4sqp_qp_fill_matrix(const struct A4SqpView *view, struct A4SqpQp *qp){
	int32 r;
	int32 k;
	int32 c;
	int32 *counts;
	int32 *next;

	counts = ASC_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	next = ASC_NEW_ARRAY_OR_NULL(int32,qp->num_col);
	if(counts == NULL || next == NULL){
		ASC_FREE(counts);
		ASC_FREE(next);
		return 1;
	}
	for(c = 0; c < qp->num_col; ++c){
		counts[c] = 0;
	}
	for(r = 0; r < view->n_rel; ++r){
		for(k = view->jac_row_start[r]; k < view->jac_row_start[r + 1]; ++k){
			c = a4sqp_qp_find_var_index(view,view->jac_col_sindex[k]);
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
			c = a4sqp_qp_find_var_index(view,view->jac_col_sindex[k]);
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

	ASC_FREE(counts);
	ASC_FREE(next);
	return 0;
}

static int32 a4sqp_qp_count_hessian_nz(const struct A4SqpView *view, const struct A4SqpStepHessian *step_hess){
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
	const struct A4SqpView *view,
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

int a4sqp_qp_build_from_view(
	struct A4SqpQp *qp,
	const struct A4SqpView *view,
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
			? var_NO_LOWER_BOUND
			: view->scaled_var_lower[i] - view->scaled_var_value[i];
		qp->col_upper[i] = a4sqp_qp_is_upper_inf(view->scaled_var_upper[i])
			? var_NO_UPPER_BOUND
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
			? var_NO_LOWER_BOUND
			: view->scaled_rel_lower[i] - view->scaled_rel_residual[i];
		qp->row_upper[i] = a4sqp_qp_is_upper_inf(view->scaled_rel_upper[i])
			? var_NO_UPPER_BOUND
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
		qp->col_upper[lower_col] = var_NO_UPPER_BOUND;

		qp->col_kind[upper_col] = A4SQP_QP_COL_ELASTIC_UPPER;
		qp->col_rel_index[upper_col] = i;
		qp->col_cost[upper_col] = elastic_penalty;
		qp->col_lower[upper_col] = 0.0;
		qp->col_upper[upper_col] = var_NO_UPPER_BOUND;
	}

	if(a4sqp_qp_fill_matrix(view,qp)){
		a4sqp_qp_destroy(qp);
		return 1;
	}
	a4sqp_qp_fill_hessian(view,step_hess,qp);
	return 0;
}

static HighsInt *a4sqp_qp_copy_int_array(const int32 *src, int32 len){
	int32 i;
	HighsInt *dest = ASC_NEW_ARRAY_OR_NULL(HighsInt,len > 0 ? len : 1);
	if(dest == NULL){
		return NULL;
	}
	for(i = 0; i < len; ++i){
		dest[i] = (HighsInt)src[i];
	}
	return dest;
}

int a4sqp_qp_solve_highs(struct A4SqpQp *qp, slv_parameters_t *params){
	void *highs;
	struct A4SqpHighsProgress progress;
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
	real64 qp_tol;
	int32 i;

	if(qp == NULL || qp->num_col <= 0 || qp->num_row < 0){
		return 1;
	}

	highs = Highs_create();
	if(highs == NULL){
		return 1;
	}
	progress.params = params;
	progress.next_report_time = 0.0;
	progress.report_count = 0;
	a4sqp_highs_enable_callbacks(highs,&progress);
	inf = Highs_getInfinity(highs);

	a_start = a4sqp_qp_copy_int_array(qp->a_start,qp->num_col + 1);
	a_index = a4sqp_qp_copy_int_array(qp->a_index,qp->num_nz);
	q_start = a4sqp_qp_copy_int_array(qp->q_start,qp->num_col + 1);
	q_index = a4sqp_qp_copy_int_array(qp->q_index,qp->q_num_nz);
	col_lower = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	col_upper = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_col);
	row_lower = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_row > 0 ? qp->num_row : 1);
	row_upper = ASC_NEW_ARRAY_OR_NULL(real64,qp->num_row > 0 ? qp->num_row : 1);
	if(a_start == NULL || a_index == NULL || q_start == NULL || q_index == NULL
		|| col_lower == NULL || col_upper == NULL || row_lower == NULL || row_upper == NULL
	){
		ASC_FREE(a_start);
		ASC_FREE(a_index);
		ASC_FREE(q_start);
		ASC_FREE(q_index);
		ASC_FREE(col_lower);
		ASC_FREE(col_upper);
		ASC_FREE(row_lower);
		ASC_FREE(row_upper);
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
		(params != NULL && SLV_PARAM_BOOL(params,A4SQP_PARAM_PROGRESS_LOG)) ? 1 : 0
	);
	qp_tol = params != NULL ? SLV_PARAM_REAL(params,A4SQP_PARAM_FEAS_TOL) : 1e-7;
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
		a4sqp_report_progress(params,"qp_solve: starting HiGHS");
		status = Highs_run(highs);
		a4sqp_report_progress(params,"qp_solve: HiGHS returned");
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

	if(status == kHighsStatusError || model_status != kHighsModelStatusOptimal){
		a4sqp_qp_report_failure(qp,params);
	}

	ASC_FREE(a_start);
	ASC_FREE(a_index);
	ASC_FREE(q_start);
	ASC_FREE(q_index);
	ASC_FREE(col_lower);
	ASC_FREE(col_upper);
	ASC_FREE(row_lower);
	ASC_FREE(row_upper);
	Highs_destroy(highs);
	return (status != kHighsStatusError && model_status == kHighsModelStatusOptimal) ? 0 : 1;
}

ASC_EXPORT int a4sqp_qp_highs_spike(struct A4SqpQpSpikeResult *result){
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
