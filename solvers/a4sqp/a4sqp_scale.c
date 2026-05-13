/*
 * A4SQP scaling helpers.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_scale.h"

#include <math.h>
#include <string.h>

#include <ascend/general/platform.h>
#include <ascend/system/var.h>

const char *a4sqp_scale_mode_name(const char *mode){
	return mode != 0 ? mode : "ROW_2NORM";
}

static real64 a4sqp_safe_scale(real64 value){
	if(!isfinite(value) || value <= 0.0){
		return 1.0;
	}
	return value;
}

static real64 a4sqp_scale_bound(real64 bound, real64 scale, real64 inf_bound){
	if(bound == inf_bound){
		return bound;
	}
	return bound * scale;
}

static void a4sqp_init_var_scaling(struct A4SqpView *view, int use_nominals){
	int32 i;
	for(i = 0; i < view->n_var; ++i){
		view->var_scale[i] = use_nominals ? a4sqp_safe_scale(view->var_nominal[i]) : 1.0;
		view->scaled_var_value[i] = view->var_value[i] / view->var_scale[i];
		view->scaled_var_lower[i] = a4sqp_scale_bound(view->var_lower[i],1.0 / view->var_scale[i],var_NO_LOWER_BOUND);
		view->scaled_var_upper[i] = a4sqp_scale_bound(view->var_upper[i],1.0 / view->var_scale[i],var_NO_UPPER_BOUND);
	}
}

static void a4sqp_init_rel_scaling(struct A4SqpView *view, const char *mode){
	int32 i;
	int use_relnom = (strcmp(a4sqp_scale_mode_name(mode),"RELNOM") == 0);
	int use_row_2norm = (strcmp(a4sqp_scale_mode_name(mode),"ROW_2NORM") == 0);

	for(i = 0; i < view->n_rel; ++i){
		real64 scale = 1.0;
		if(use_relnom){
			scale = 1.0 / a4sqp_safe_scale(rel_nominal(view->rels[i]));
		}else if(use_row_2norm){
			int32 k;
			real64 sum = 0.0;
			for(k = view->jac_row_start[i]; k < view->jac_row_start[i + 1]; ++k){
				int32 col = view->jac_col_index != NULL ? view->jac_col_index[k] : -1;
				real64 value = view->jac_value[k];
				if(col >= 0){
					value *= view->var_scale[col];
				}
				sum += value * value;
			}
			scale = sum > 0.0 ? 1.0 / sqrt(sum) : 1.0;
		}
		view->rel_scale[i] = scale;
		view->scaled_rel_residual[i] = view->rel_residual[i] * scale;
		view->scaled_rel_lower[i] = a4sqp_scale_bound(view->rel_lower[i],scale,var_NO_LOWER_BOUND);
		view->scaled_rel_upper[i] = a4sqp_scale_bound(view->rel_upper[i],scale,var_NO_UPPER_BOUND);
	}
}

static void a4sqp_scale_jacobian(struct A4SqpView *view){
	int32 row;
	for(row = 0; row < view->n_rel; ++row){
		int32 k;
		for(k = view->jac_row_start[row]; k < view->jac_row_start[row + 1]; ++k){
			int32 col = view->jac_col_index != NULL ? view->jac_col_index[k] : -1;
			real64 col_scale = col >= 0 ? view->var_scale[col] : 1.0;
			view->scaled_jac_value[k] = view->rel_scale[row] * view->jac_value[k] * col_scale;
		}
	}
}

static void a4sqp_scale_objective_gradient(struct A4SqpView *view){
	int32 i;
	for(i = 0; i < view->n_var; ++i){
		view->scaled_obj_gradient[i] = view->obj_gradient[i] * view->var_scale[i];
	}
}

int a4sqp_view_apply_scaling(struct A4SqpView *view, const char *mode){
	int use_nominals;
	const char *scaleopt;
	if(view == NULL){
		return 1;
	}

	scaleopt = a4sqp_scale_mode_name(mode);
	use_nominals = strcmp(scaleopt,"NONE") != 0;
	a4sqp_init_var_scaling(view,use_nominals);
	a4sqp_init_rel_scaling(view,scaleopt);
	a4sqp_scale_jacobian(view);
	a4sqp_scale_objective_gradient(view);
	return 0;
}
