/*
 * A4SQP scaling helpers.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_scale.h"

#include <math.h>
#include <string.h>

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
		view->scaled_var_lower[i] = a4sqp_scale_bound(view->var_lower[i],1.0 / view->var_scale[i],A4SQP_NO_LOWER_BOUND);
		view->scaled_var_upper[i] = a4sqp_scale_bound(view->var_upper[i],1.0 / view->var_scale[i],A4SQP_NO_UPPER_BOUND);
	}
}

static void a4sqp_init_rel_scaling(struct A4SqpView *view, const char *mode){
	int32 i;
	const char *scaleopt = a4sqp_scale_mode_name(mode);
	int use_relnom = (strcmp(scaleopt,"RELNOM") == 0);
	int use_row_2norm = (
		strcmp(scaleopt,"ROW_2NORM") == 0
		|| strcmp(scaleopt,"AUTO") == 0
		|| strcmp(scaleopt,"ROW_2NORM_T2") == 0
		|| strcmp(scaleopt,"ROW_2NORM_T5") == 0
		|| strcmp(scaleopt,"ROW_2NORM_T10") == 0
		|| strcmp(scaleopt,"ROW_2NORM_T100") == 0
		|| strcmp(scaleopt,"ROW_2NORM_F1E-2") == 0
		|| strcmp(scaleopt,"ROW_2NORM_F1E-4") == 0
	);
	real64 row_target = 1.0;
	real64 row_floor = 0.0;

	if(strcmp(scaleopt,"AUTO") == 0){
		row_target = 5.0;
	}else if(strcmp(scaleopt,"ROW_2NORM_T2") == 0){
		row_target = 2.0;
	}else if(strcmp(scaleopt,"ROW_2NORM_T5") == 0){
		row_target = 5.0;
	}else if(strcmp(scaleopt,"ROW_2NORM_T10") == 0){
		row_target = 10.0;
	}else if(strcmp(scaleopt,"ROW_2NORM_T100") == 0){
		row_target = 100.0;
	}
	if(strcmp(scaleopt,"ROW_2NORM_F1E-2") == 0){
		row_floor = 1e-2;
	}else if(strcmp(scaleopt,"ROW_2NORM_F1E-4") == 0){
		row_floor = 1e-4;
	}

	for(i = 0; i < view->n_rel; ++i){
		real64 scale = 1.0;
		if(use_relnom){
			scale = 1.0 / a4sqp_safe_scale(view->rel_nominal != NULL ? view->rel_nominal[i] : 1.0);
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
			/*
			 * Row scaling is intended to reduce oversized rows, not amplify
			 * constraints whose Jacobian norm becomes small at a degenerate
			 * solution. Scaling up those rows makes the feasibility test
			 * singular, as in BT13 where the active equality gradient vanishes
			 * at the optimum.
			 */
			if(sum > row_target * row_target){
				scale = row_target / sqrt(sum);
			}
			if(row_floor > 0.0 && scale < row_floor){
				scale = row_floor;
			}
		}
		view->rel_scale[i] = scale;
		view->scaled_rel_residual[i] = view->rel_residual[i] * scale;
		view->scaled_rel_lower[i] = a4sqp_scale_bound(view->rel_lower[i],scale,A4SQP_NO_LOWER_BOUND);
		view->scaled_rel_upper[i] = a4sqp_scale_bound(view->rel_upper[i],scale,A4SQP_NO_UPPER_BOUND);
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
