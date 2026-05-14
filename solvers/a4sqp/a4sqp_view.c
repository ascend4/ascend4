/*
 * A4SQP problem view.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_view.h"

#include <string.h>

void a4sqp_view_init(struct A4SqpView *view){
	if(view == NULL){
		return;
	}
	view->n_var = 0;
	view->n_rel = 0;
	view->vars = NULL;
	view->owns_vars = 0;
	view->rels = NULL;
	view->obj = NULL;
	view->obj_direction = 0;
	view->obj_value = 0.0;
	view->obj_gradient = NULL;
	view->scaled_obj_gradient = NULL;
	view->var_mindex = NULL;
	view->var_sindex = NULL;
	view->var_value = NULL;
	view->var_lower = NULL;
	view->var_upper = NULL;
	view->var_nominal = NULL;
	view->var_fixed = NULL;
	view->var_scale = NULL;
	view->scaled_var_value = NULL;
	view->scaled_var_lower = NULL;
	view->scaled_var_upper = NULL;
	view->rel_sindex = NULL;
	view->relop = NULL;
	view->rel_kind = NULL;
	view->rel_residual = NULL;
	view->rel_lower = NULL;
	view->rel_upper = NULL;
	view->rel_nominal = NULL;
	view->rel_scale = NULL;
	view->scaled_rel_residual = NULL;
	view->scaled_rel_lower = NULL;
	view->scaled_rel_upper = NULL;
	view->jac_nnz = 0;
	view->jac_max_row_nnz = 0;
	view->jac_row_start = NULL;
	view->jac_col_index = NULL;
	view->jac_col_sindex = NULL;
	view->jac_value = NULL;
	view->scaled_jac_value = NULL;
	view->calc_errors = 0;
	view->obj_calc_errors = 0;
	view->unsupported_rels = 0;
	view->derivative_errors = 0;
	view->obj_derivative_errors = 0;
}

void a4sqp_view_destroy(struct A4SqpView *view){
	if(view == NULL){
		return;
	}
	if(view->owns_vars){
		A4SQP_FREE(view->vars);
	}
	A4SQP_FREE(view->var_sindex);
	A4SQP_FREE(view->var_value);
	A4SQP_FREE(view->var_lower);
	A4SQP_FREE(view->var_upper);
	A4SQP_FREE(view->var_nominal);
	A4SQP_FREE(view->var_fixed);
	A4SQP_FREE(view->var_scale);
	A4SQP_FREE(view->scaled_var_value);
	A4SQP_FREE(view->scaled_var_lower);
	A4SQP_FREE(view->scaled_var_upper);
	A4SQP_FREE(view->obj_gradient);
	A4SQP_FREE(view->scaled_obj_gradient);
	A4SQP_FREE(view->var_mindex);
	A4SQP_FREE(view->rel_sindex);
	A4SQP_FREE(view->relop);
	A4SQP_FREE(view->rel_kind);
	A4SQP_FREE(view->rel_residual);
	A4SQP_FREE(view->rel_lower);
	A4SQP_FREE(view->rel_upper);
	A4SQP_FREE(view->rel_nominal);
	A4SQP_FREE(view->rel_scale);
	A4SQP_FREE(view->scaled_rel_residual);
	A4SQP_FREE(view->scaled_rel_lower);
	A4SQP_FREE(view->scaled_rel_upper);
	A4SQP_FREE(view->jac_row_start);
	A4SQP_FREE(view->jac_col_index);
	A4SQP_FREE(view->jac_col_sindex);
	A4SQP_FREE(view->jac_value);
	A4SQP_FREE(view->scaled_jac_value);
	a4sqp_view_init(view);
}

void a4sqp_view_get_core(const struct A4SqpView *view, struct A4SqpCoreView *core){
	if(core == NULL){
		return;
	}
	memset(core,0,sizeof(*core));
	if(view == NULL){
		return;
	}
	core->n_var = view->n_var;
	core->n_rel = view->n_rel;
	core->has_objective = view->obj != NULL;
	core->obj_value = view->obj_value;
	core->var_scale = view->var_scale;
	core->scaled_var_value = view->scaled_var_value;
	core->scaled_var_lower = view->scaled_var_lower;
	core->scaled_var_upper = view->scaled_var_upper;
	core->scaled_obj_gradient = view->scaled_obj_gradient;
	core->rel_kind = view->rel_kind;
	core->scaled_rel_residual = view->scaled_rel_residual;
	core->scaled_rel_lower = view->scaled_rel_lower;
	core->scaled_rel_upper = view->scaled_rel_upper;
	core->jac_nnz = view->jac_nnz;
	core->jac_row_start = view->jac_row_start;
	core->jac_col_index = view->jac_col_index;
	core->scaled_jac_value = view->scaled_jac_value;
}
