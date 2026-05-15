/*
 * Least-squares core solve utilities for A4SQP.
 *
 * This file is deliberately independent of ASCEND. Frontends provide
 * residual/Jacobian callbacks and storage for the current iterate.
 */

#ifndef ASC_A4SQP_LSQ_H
#define ASC_A4SQP_LSQ_H

#include "a4sqp_types.h"

enum A4SqpLsqMode {
	A4SQP_LSQ_MODE_GAUSS = 0,
	A4SQP_LSQ_MODE_LM
};

enum A4SqpLsqStatus {
	A4SQP_LSQ_SOLVED = 0,
	A4SQP_LSQ_MAX_ITER = 1,
	A4SQP_LSQ_EVAL_ERROR = 2,
	A4SQP_LSQ_LINEAR_ERROR = 3,
	A4SQP_LSQ_INVALID_PROBLEM = 4
};

struct A4SqpLsqOptions {
	enum A4SqpLsqMode mode;
	int max_iter;
	int max_backtrack;
	real64 grad_tol;
	real64 step_tol;
	real64 lambda_init;
	real64 lambda_min;
	real64 lambda_max;
};

struct A4SqpLsqIteration {
	int iter;
	real64 objective;
	real64 grad_inf;
	real64 step_norm;
	real64 lambda;
	real64 alpha;
	int accepted;
};

struct A4SqpLsqStats {
	int iterations;
	real64 objective;
	real64 grad_inf;
	real64 step_norm;
	real64 lambda;
	int accepted_steps;
};

struct A4SqpLsqProblem {
	int32 n_var;
	int32 n_res;
	const real64 *weights;
	const real64 *x_lower;
	const real64 *x_upper;
	void *userdata;
	int (*eval_residuals)(void *userdata, const real64 *x, real64 *residuals);
	int (*eval_jacobian_row)(
		void *userdata,
		int32 row,
		int32 capacity,
		int32 *columns,
		real64 *values,
		int32 *nnz
	);
	int (*progress)(void *userdata, const struct A4SqpLsqIteration *iteration);
};

A4SQP_CORE_EXPORT enum A4SqpLsqStatus a4sqp_lsq_solve(
	const struct A4SqpLsqProblem *problem,
	const struct A4SqpLsqOptions *options,
	real64 *x,
	struct A4SqpLsqStats *stats
);

#endif
