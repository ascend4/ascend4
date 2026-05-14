/*
 * Shared Hessian utilities for A4SQP frontends.
 */

#ifndef ASC_A4SQP_HESSIAN_H
#define ASC_A4SQP_HESSIAN_H

#include "a4sqp_types.h"

struct A4SqpDenseHessian {
	int32 n;
	int updates;
	real64 last_reg;
	real64 *dense;
};

typedef int (*A4SqpHessianEntryFn)(void *ctx, int32 row, int32 col, real64 value);
typedef int (*A4SqpRelationHessianEvalFn)(
	void *ctx,
	int32 relation_index,
	void *entry_ctx,
	A4SqpHessianEntryFn entry
);

A4SQP_CORE_EXPORT void a4sqp_dense_hessian_init(struct A4SqpDenseHessian *model);
A4SQP_CORE_EXPORT void a4sqp_dense_hessian_destroy(struct A4SqpDenseHessian *model);
A4SQP_CORE_EXPORT int a4sqp_dense_hessian_reset_identity(
	struct A4SqpDenseHessian *model,
	int32 n,
	real64 diag
);
A4SQP_CORE_EXPORT int a4sqp_dense_hessian_sync_identity(struct A4SqpDenseHessian *model, int32 n);
A4SQP_CORE_EXPORT real64 a4sqp_dense_hessian_regularize_psd(
	struct A4SqpDenseHessian *model,
	real64 min_diag
);
A4SQP_CORE_EXPORT void a4sqp_dense_hessian_mul(
	const struct A4SqpDenseHessian *model,
	const real64 *x,
	real64 *y
);
A4SQP_CORE_EXPORT int a4sqp_dense_hessian_bfgs_update(
	struct A4SqpDenseHessian *model,
	const real64 *new_scaled_x,
	const real64 *new_scaled_grad,
	const real64 *old_scaled_x,
	const real64 *old_scaled_grad,
	real64 min_diag
);

A4SQP_CORE_EXPORT real64 a4sqp_hessian_dense_regularize_psd(real64 *hess, int32 n, real64 min_diag);
A4SQP_CORE_EXPORT void a4sqp_hessian_dense_mul(const real64 *hess, int32 n, const real64 *x, real64 *y);
A4SQP_CORE_EXPORT int32 a4sqp_hessian_lower_triangle_nnz(int32 n);
A4SQP_CORE_EXPORT int a4sqp_hessian_lower_triangle_structure(
	int32 n,
	int32 *irow,
	int32 *jcol
);
A4SQP_CORE_EXPORT int a4sqp_hessian_build_dense_lower_from_relations(
	int32 n,
	int32 m,
	real64 obj_factor,
	const real64 *lambda,
	A4SqpRelationHessianEvalFn eval_relation_hessian,
	void *ctx,
	real64 *values
);

#endif
