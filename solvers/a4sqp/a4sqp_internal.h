/*
 * Internal A4SQP structures.
 */

#ifndef ASC_A4SQP_INTERNAL_H
#define ASC_A4SQP_INTERNAL_H

#include "a4sqp_view.h"
#include "a4sqp_params.h"
#include "a4sqp_qp_highs.h"

#include <ascend/system/slv_client.h>
#include <ascend/system/slv_common.h>

struct A4SqpSystem {
	slv_system_t server;
	slv_parameters_t params;
	struct slv_parameter param_data[A4SQP_PARAM_COUNT];
	slv_status_t status;
	struct A4SqpView view;
	struct A4SqpQp qp;
	int32 step_hess_n;
	int step_hess_updates;
	int step_hess_exact;
	real64 *step_hess;
	int32 step_hess_sparse_n;
	int32 step_hess_sparse_nnz;
	int32 *step_hess_sparse_start;
	int32 *step_hess_sparse_index;
	real64 *step_hess_sparse_value;
	int32 lambda_est_n;
	int lambda_est_ready;
	int32 lambda_est_good_count;
	int32 lambda_est_required_count;
	real64 *lambda_est;
	real64 last_elastic_max;
	real64 last_merit_before;
	real64 last_merit_after;
	real64 last_model_merit_after;
	real64 last_predicted_reduction;
	real64 last_linearized_violation;
	real64 last_violation_sum;
	real64 last_violation_max;
	real64 last_alpha;
	real64 last_step_norm;
	real64 trust_radius;
	real64 last_trust_ratio;
	int32 worst_violation_rel;
	int line_search_failed;
};

#endif
