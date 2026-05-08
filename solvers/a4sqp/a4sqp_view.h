/*
 * A4SQP problem view.
 */

#ifndef ASC_A4SQP_VIEW_H
#define ASC_A4SQP_VIEW_H

#include <ascend/general/platform.h>
#include <ascend/system/rel.h>
#include <ascend/system/slv_client.h>

struct A4SqpView {
	int32 n_var;
	int32 n_rel;
	struct var_variable **vars;
	struct rel_relation **rels;
	struct rel_relation *obj;
	int32 obj_direction;
	real64 obj_value;
	real64 *obj_gradient;
	real64 *scaled_obj_gradient;
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
	enum rel_enum *relop;
	real64 *rel_residual;
	real64 *rel_lower;
	real64 *rel_upper;
	real64 *rel_scale;
	real64 *scaled_rel_residual;
	real64 *scaled_rel_lower;
	real64 *scaled_rel_upper;
	int32 jac_nnz;
	int32 jac_max_row_nnz;
	int32 *jac_row_start;
	int32 *jac_col_sindex;
	real64 *jac_value;
	real64 *scaled_jac_value;
	int32 calc_errors;
	int32 obj_calc_errors;
	int32 unsupported_rels;
	int32 derivative_errors;
	int32 obj_derivative_errors;
};

void a4sqp_view_init(struct A4SqpView *view);
void a4sqp_view_destroy(struct A4SqpView *view);
int a4sqp_view_build(struct A4SqpView *view, slv_system_t server, int safe, const char *scaleopt);

#endif
