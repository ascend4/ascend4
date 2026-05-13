/*
 * Solver-neutral numeric problem view for A4SQP core code.
 */

#ifndef ASC_A4SQP_CORE_VIEW_H
#define ASC_A4SQP_CORE_VIEW_H

#include <ascend/general/platform.h>

enum A4SqpRelKind {
	A4SQP_REL_KIND_INEQUALITY = 0,
	A4SQP_REL_KIND_EQUALITY
};

struct A4SqpCoreView {
	int32 n_var;
	int32 n_rel;
	int has_objective;
	real64 obj_value;
	const real64 *var_scale;
	const real64 *scaled_var_value;
	const real64 *scaled_var_lower;
	const real64 *scaled_var_upper;
	const real64 *scaled_obj_gradient;
	const enum A4SqpRelKind *rel_kind;
	const real64 *scaled_rel_residual;
	const real64 *scaled_rel_lower;
	const real64 *scaled_rel_upper;
	int32 jac_nnz;
	const int32 *jac_row_start;
	const int32 *jac_col_index;
	const real64 *scaled_jac_value;
};

#endif
