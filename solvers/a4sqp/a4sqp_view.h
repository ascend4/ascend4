/*
 * A4SQP problem view.
 */

#ifndef ASC_A4SQP_VIEW_H
#define ASC_A4SQP_VIEW_H

#include "a4sqp_core_view.h"

struct A4SqpView {
	int32 n_var;
	int32 n_rel;
	void **vars;
	void **rels;
	void *obj;
	int32 obj_direction;
	real64 obj_value;
	real64 *obj_gradient;
	real64 *scaled_obj_gradient;
	int32 *var_mindex;
	int32 *var_sindex;
	real64 *var_value;
	real64 *var_lower;
	real64 *var_upper;
	real64 *var_nominal;
	uint32 *var_fixed;
	real64 *var_scale;
	real64 *scaled_var_value;
	real64 *scaled_var_lower;
	real64 *scaled_var_upper;
	int32 *rel_sindex;
	int32 *relop;
	enum A4SqpRelKind *rel_kind;
	real64 *rel_residual;
	real64 *rel_lower;
	real64 *rel_upper;
	real64 *rel_nominal;
	real64 *rel_scale;
	real64 *scaled_rel_residual;
	real64 *scaled_rel_lower;
	real64 *scaled_rel_upper;
	int32 jac_nnz;
	int32 jac_max_row_nnz;
	int32 *jac_row_start;
	int32 *jac_col_index;
	int32 *jac_col_sindex;
	real64 *jac_value;
	real64 *scaled_jac_value;
	int32 calc_errors;
	int32 obj_calc_errors;
	int32 unsupported_rels;
	int32 derivative_errors;
	int32 obj_derivative_errors;
};

A4SQP_CORE_EXPORT void a4sqp_view_init(struct A4SqpView *view);
A4SQP_CORE_EXPORT void a4sqp_view_destroy(struct A4SqpView *view);
void a4sqp_view_get_core(const struct A4SqpView *view, struct A4SqpCoreView *core);

#endif
