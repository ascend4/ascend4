/*
 * Shared SQP core utilities for A4SQP frontends.
 */

#ifndef ASC_A4SQP_CORE_H
#define ASC_A4SQP_CORE_H

#include "a4sqp_qp_highs.h"
#include "a4sqp_core_view.h"

struct A4SqpView;

enum A4SqpRowActivity {
	A4SQP_ROW_INACTIVE = 0,
	A4SQP_ROW_NEAR_ACTIVE,
	A4SQP_ROW_ACTIVE,
	A4SQP_ROW_EQUALITY
};

struct A4SqpConvergencePolicy {
	real64 feas_tol;
	real64 step_tol;
	real64 last_step_norm;
	int has_objective;
	int constrained_objective_allows_small_step;
};

struct A4SqpLineSearchOptions {
	int max_backtrack;
	real64 merit_tol;
	real64 feas_tol;
	real64 step_tol;
	real64 armijo_coeff;
	real64 trust_accept;
	real64 elastic_penalty;
};

struct A4SqpLineSearchResult {
	int accepted;
	int trials;
	real64 merit_before;
	real64 merit_after;
	real64 model_merit_after;
	real64 predicted_reduction;
	real64 alpha;
	real64 step_norm;
	real64 trust_ratio;
	real64 scaled_step_inf;
};

struct A4SqpVectorLineSearchOps {
	int (*evaluate)(void *ctx, const real64 *x, struct A4SqpView *view);
	void (*accepted)(void *ctx, const real64 *old_scaled_x, const real64 *old_scaled_grad);
};

enum A4SqpCoreStepStatus {
	A4SQP_CORE_STEP_ACCEPTED = 0,
	A4SQP_CORE_STEP_HESSIAN_ERROR,
	A4SQP_CORE_STEP_QP_BUILD_ERROR,
	A4SQP_CORE_STEP_QP_ERROR,
	A4SQP_CORE_STEP_LINE_SEARCH_ERROR
};

struct A4SqpCoreStepOptions {
	int trust_qp_retries;
	real64 elastic_penalty;
	real64 feas_tol;
	real64 merit_tol;
};

struct A4SqpCoreStepStats {
	int qp_solves;
	int qp_failures;
	int line_search_failures;
	int trust_shrinks;
};

struct A4SqpCoreStepOps {
	int (*prepare_hessian)(void *ctx, struct A4SqpStepHessian *step_hess);
	int (*solve_qp)(void *ctx, struct A4SqpQp *qp);
	void (*after_qp_solve)(void *ctx, const struct A4SqpQp *qp);
	int (*shrink_trust)(void *ctx, const char *reason);
};

int a4sqp_core_is_lower_inf(real64 value);
int a4sqp_core_is_upper_inf(real64 value);
real64 a4sqp_core_violation(
	const struct A4SqpCoreView *view,
	real64 *max_violation,
	int32 *worst_rel
);
A4SQP_CORE_EXPORT real64 a4sqp_core_view_violation(
	const struct A4SqpView *view,
	real64 *max_violation,
	int32 *worst_rel
);
real64 a4sqp_core_merit(
	const struct A4SqpCoreView *view,
	real64 penalty
);
real64 a4sqp_core_view_merit(
	const struct A4SqpView *view,
	int has_objective,
	real64 penalty
);
real64 a4sqp_core_projected_gradient_inf_for_view(
	const struct A4SqpCoreView *view,
	real64 active_tol
);
real64 a4sqp_core_projected_gradient_inf(
	const struct A4SqpView *view,
	int has_objective,
	real64 active_tol
);
real64 a4sqp_core_qp_elastic_sum(const struct A4SqpQp *qp);
real64 a4sqp_core_qp_elastic_max(const struct A4SqpQp *qp);
enum A4SqpRowActivity a4sqp_core_row_activity_for_view(
	const struct A4SqpCoreView *view,
	int32 row,
	real64 active_tol,
	real64 near_tol
);
A4SQP_CORE_EXPORT enum A4SqpRowActivity a4sqp_core_row_activity(
	const struct A4SqpView *view,
	int32 row,
	real64 active_tol,
	real64 near_tol
);
int a4sqp_core_has_converged_for_view(
	const struct A4SqpCoreView *view,
	const struct A4SqpConvergencePolicy *policy,
	real64 *max_violation,
	real64 *projected_gradient_inf,
	int32 *worst_rel
);
A4SQP_CORE_EXPORT int a4sqp_core_has_converged(
	const struct A4SqpView *view,
	const struct A4SqpConvergencePolicy *policy,
	real64 *max_violation,
	real64 *projected_gradient_inf,
	int32 *worst_rel
);
int a4sqp_core_line_search_vector(
	struct A4SqpView *view,
	const struct A4SqpQp *qp,
	const struct A4SqpLineSearchOptions *options,
	const struct A4SqpVectorLineSearchOps *ops,
	void *ctx,
	real64 *x,
	int has_objective,
	struct A4SqpLineSearchResult *result
);
A4SQP_CORE_EXPORT enum A4SqpCoreStepStatus a4sqp_core_solve_step(
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
);

#endif
