/*
 * HiGHS QP backend experiments for A4SQP.
 */

#ifndef ASC_A4SQP_QP_HIGHS_H
#define ASC_A4SQP_QP_HIGHS_H

#include <ascend/general/platform.h>
#include <ascend/system/slv_client.h>

#include "a4sqp_view.h"

#define A4SQP_QP_COL_STEP 1
#define A4SQP_QP_COL_ELASTIC_LOWER 2
#define A4SQP_QP_COL_ELASTIC_UPPER 3

#define A4SQP_QP_DEFAULT_ELASTIC_PENALTY 100.0

struct A4SqpStepHessian {
	int32 n;
	int is_sparse;
	int32 nnz;
	const real64 *dense;
	const int32 *start;
	const int32 *index;
	const real64 *value;
};

struct A4SqpQp {
	int32 num_step_col;
	int32 num_elastic_pair;
	int32 num_col;
	int32 num_row;
	int32 num_nz;
	int32 q_num_nz;
	uint32 *col_kind;
	int32 *col_var_index;
	int32 *col_rel_index;
	int32 *row_rel_index;
	real64 *col_cost;
	real64 *col_lower;
	real64 *col_upper;
	real64 *row_lower;
	real64 *row_upper;
	int32 *a_start;
	int32 *a_index;
	real64 *a_value;
	int32 *q_start;
	int32 *q_index;
	real64 *q_value;
	real64 *col_value;
	real64 *col_dual;
	real64 *row_value;
	real64 *row_dual;
	int highs_status;
	int highs_model_status;
	real64 objective_value;
};

struct A4SqpQpSpikeResult {
	int highs_status;
	int highs_model_status;
	double objective_value;
	double col_value[3];
	double col_dual[3];
	double row_value[2];
	double row_dual[2];
};

ASC_EXPORT int a4sqp_qp_highs_spike(struct A4SqpQpSpikeResult *result);

void a4sqp_qp_init(struct A4SqpQp *qp);
void a4sqp_qp_destroy(struct A4SqpQp *qp);
int a4sqp_qp_build_from_view(
	struct A4SqpQp *qp,
	const struct A4SqpView *view,
	const struct A4SqpStepHessian *step_hess,
	real64 trust_radius,
	real64 elastic_penalty,
	real64 feas_tol
);
int a4sqp_qp_solve_highs(struct A4SqpQp *qp, slv_parameters_t *params);

#endif
