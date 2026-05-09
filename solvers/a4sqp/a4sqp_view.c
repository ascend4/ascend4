/*
 * A4SQP problem view.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_view.h"

#include "a4sqp_scale.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/system/relman.h>
#include <ascend/system/rel.h>
#include <ascend/system/var.h>

static int32 a4sqp_count_vars(struct var_variable **vars){
	int32 n = 0;
	if(vars == NULL){
		return 0;
	}
	while(vars[n] != NULL){
		++n;
	}
	return n;
}

static int32 a4sqp_count_rels(struct rel_relation **rels){
	int32 n = 0;
	if(rels == NULL){
		return 0;
	}
	while(rels[n] != NULL){
		++n;
	}
	return n;
}

void a4sqp_view_init(struct A4SqpView *view){
	if(view == NULL){
		return;
	}
	view->n_var = 0;
	view->n_rel = 0;
	view->vars = NULL;
	view->rels = NULL;
	view->obj = NULL;
	view->obj_direction = 0;
	view->obj_value = 0.0;
	view->obj_gradient = NULL;
	view->scaled_obj_gradient = NULL;
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
	view->rel_residual = NULL;
	view->rel_lower = NULL;
	view->rel_upper = NULL;
	view->rel_scale = NULL;
	view->scaled_rel_residual = NULL;
	view->scaled_rel_lower = NULL;
	view->scaled_rel_upper = NULL;
	view->jac_nnz = 0;
	view->jac_max_row_nnz = 0;
	view->jac_row_start = NULL;
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
	ASC_FREE(view->var_sindex);
	ASC_FREE(view->var_value);
	ASC_FREE(view->var_lower);
	ASC_FREE(view->var_upper);
	ASC_FREE(view->var_nominal);
	ASC_FREE(view->var_fixed);
	ASC_FREE(view->var_scale);
	ASC_FREE(view->scaled_var_value);
	ASC_FREE(view->scaled_var_lower);
	ASC_FREE(view->scaled_var_upper);
	ASC_FREE(view->obj_gradient);
	ASC_FREE(view->scaled_obj_gradient);
	ASC_FREE(view->rel_sindex);
	ASC_FREE(view->relop);
	ASC_FREE(view->rel_residual);
	ASC_FREE(view->rel_lower);
	ASC_FREE(view->rel_upper);
	ASC_FREE(view->rel_scale);
	ASC_FREE(view->scaled_rel_residual);
	ASC_FREE(view->scaled_rel_lower);
	ASC_FREE(view->scaled_rel_upper);
	ASC_FREE(view->jac_row_start);
	ASC_FREE(view->jac_col_sindex);
	ASC_FREE(view->jac_value);
	ASC_FREE(view->scaled_jac_value);
	a4sqp_view_init(view);
}

static int a4sqp_view_alloc(struct A4SqpView *view){
	if(view->n_var > 0){
		view->var_sindex = ASC_NEW_ARRAY_OR_NULL(int32,view->n_var);
		view->var_value = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_lower = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_upper = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_nominal = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_fixed = ASC_NEW_ARRAY_OR_NULL(uint32,view->n_var);
		view->var_scale = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_var_value = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_var_lower = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_var_upper = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->obj_gradient = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_obj_gradient = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
		if(view->var_sindex == NULL || view->var_value == NULL
			|| view->var_lower == NULL || view->var_upper == NULL
			|| view->var_nominal == NULL || view->var_fixed == NULL
			|| view->var_scale == NULL || view->scaled_var_value == NULL
			|| view->scaled_var_lower == NULL || view->scaled_var_upper == NULL
			|| view->obj_gradient == NULL || view->scaled_obj_gradient == NULL
		){
			return 1;
		}
	}

	if(view->n_rel > 0){
		view->rel_sindex = ASC_NEW_ARRAY_OR_NULL(int32,view->n_rel);
		view->relop = ASC_NEW_ARRAY_OR_NULL(enum rel_enum,view->n_rel);
		view->rel_residual = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->rel_lower = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->rel_upper = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->rel_scale = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->scaled_rel_residual = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->scaled_rel_lower = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->scaled_rel_upper = ASC_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->jac_row_start = ASC_NEW_ARRAY_OR_NULL(int32,view->n_rel + 1);
		if(view->rel_sindex == NULL || view->relop == NULL
			|| view->rel_residual == NULL || view->rel_lower == NULL
			|| view->rel_upper == NULL || view->rel_scale == NULL
			|| view->scaled_rel_residual == NULL || view->scaled_rel_lower == NULL
			|| view->scaled_rel_upper == NULL || view->jac_row_start == NULL
		){
			return 1;
		}
	}

	return 0;
}

static int a4sqp_map_rel_bounds(enum rel_enum relop, real64 *lower, real64 *upper){
	switch(relop){
	case e_rel_equal:
		*lower = 0.0;
		*upper = 0.0;
		return 0;
	case e_rel_less:
	case e_rel_lesseq:
		*lower = var_NO_LOWER_BOUND;
		*upper = 0.0;
		return 0;
	case e_rel_greater:
	case e_rel_greatereq:
		*lower = 0.0;
		*upper = var_NO_UPPER_BOUND;
		return 0;
	default:
		*lower = 0.0;
		*upper = 0.0;
		return 1;
	}
}

static int32 a4sqp_view_find_var_index(const struct A4SqpView *view, int32 sindex){
	int32 i;
	for(i = 0; i < view->n_var; ++i){
		if(view->var_sindex[i] == sindex){
			return i;
		}
	}
	return -1;
}

static void a4sqp_view_capture_vars(struct A4SqpView *view){
	int32 i;
	for(i = 0; i < view->n_var; ++i){
		struct var_variable *var = view->vars[i];
		view->var_sindex[i] = var_sindex(var);
		view->var_value[i] = var_value(var);
		view->var_lower[i] = var_lower_bound(var);
		view->var_upper[i] = var_upper_bound(var);
		view->var_nominal[i] = var_nominal(var);
		view->var_fixed[i] = var_fixed(var);
	}
}

static void a4sqp_view_capture_rels(struct A4SqpView *view, int safe){
	int32 i;
	for(i = 0; i < view->n_rel; ++i){
		int32 calc_ok = 0;
		struct rel_relation *rel = view->rels[i];
		view->rel_sindex[i] = rel_sindex(rel);
		view->relop[i] = rel_relop(rel);
		if(a4sqp_map_rel_bounds(view->relop[i],&view->rel_lower[i],&view->rel_upper[i])){
			++view->unsupported_rels;
		}
		view->rel_residual[i] = relman_eval(rel,&calc_ok,safe);
		if(!calc_ok){
			++view->calc_errors;
		}
	}
}

static int a4sqp_view_capture_jacobian(struct A4SqpView *view, int safe){
	int32 i;
	int32 nnz = 0;
	real64 *row_derivs = NULL;
	int32 *row_vars = NULL;
	var_filter_t vfilter;

	view->jac_nnz = 0;
	view->jac_max_row_nnz = 0;
	if(view->n_rel <= 0){
		return 0;
	}

	row_derivs = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var > 0 ? view->n_var : 1);
	row_vars = ASC_NEW_ARRAY_OR_NULL(int32,view->n_var > 0 ? view->n_var : 1);
	if(row_derivs == NULL || row_vars == NULL){
		ASC_FREE(row_derivs);
		ASC_FREE(row_vars);
		return 1;
	}

	vfilter.matchbits = VAR_SVAR;
	vfilter.matchvalue = VAR_SVAR;
	view->jac_row_start[0] = 0;

	for(i = 0; i < view->n_rel; ++i){
		int32 count = 0;
		int err = relman_diff2_rev(view->rels[i],&vfilter,row_derivs,row_vars,&count,safe);
		if(err){
			++view->derivative_errors;
			count = 0;
		}
		if(count > view->jac_max_row_nnz){
			view->jac_max_row_nnz = count;
		}
		nnz += count;
		view->jac_row_start[i + 1] = nnz;
	}

	view->jac_nnz = nnz;
	if(nnz > 0){
		view->jac_col_sindex = ASC_NEW_ARRAY_OR_NULL(int32,nnz);
		view->jac_value = ASC_NEW_ARRAY_OR_NULL(real64,nnz);
		view->scaled_jac_value = ASC_NEW_ARRAY_OR_NULL(real64,nnz);
		if(view->jac_col_sindex == NULL || view->jac_value == NULL || view->scaled_jac_value == NULL){
			ASC_FREE(row_derivs);
			ASC_FREE(row_vars);
			return 1;
		}

		nnz = 0;
		for(i = 0; i < view->n_rel; ++i){
			int32 j;
			int32 count = 0;
			int err = relman_diff2_rev(view->rels[i],&vfilter,row_derivs,row_vars,&count,safe);
			if(err){
				count = 0;
			}
			for(j = 0; j < count; ++j){
				view->jac_col_sindex[nnz] = row_vars[j];
				view->jac_value[nnz] = row_derivs[j];
				++nnz;
			}
		}
	}

	ASC_FREE(row_derivs);
	ASC_FREE(row_vars);
	return 0;
}

static int a4sqp_view_capture_objective(struct A4SqpView *view, int safe){
	int32 i;
	int32 count = 0;
	int32 calc_ok = 0;
	real64 obj_sign = 1.0;
	real64 *derivs = NULL;
	int32 *vars = NULL;
	var_filter_t vfilter;

	if(view->n_var > 0){
		for(i = 0; i < view->n_var; ++i){
			view->obj_gradient[i] = 0.0;
			view->scaled_obj_gradient[i] = 0.0;
		}
	}
	if(view->obj == NULL){
		view->obj_direction = 0;
		view->obj_value = 0.0;
		return 0;
	}

	view->obj_direction = relman_obj_direction(view->obj);
	if(view->obj_direction > 0){
		obj_sign = -1.0;
	}
	view->obj_value = obj_sign * relman_eval(view->obj,&calc_ok,safe);
	if(!calc_ok){
		++view->obj_calc_errors;
		return 0;
	}

	if(view->n_var <= 0){
		return 0;
	}

	derivs = ASC_NEW_ARRAY_OR_NULL(real64,view->n_var);
	vars = ASC_NEW_ARRAY_OR_NULL(int32,view->n_var);
	if(derivs == NULL || vars == NULL){
		ASC_FREE(derivs);
		ASC_FREE(vars);
		return 1;
	}
	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	if(relman_diff2_rev(view->obj,&vfilter,derivs,vars,&count,safe)){
		++view->obj_derivative_errors;
		ASC_FREE(derivs);
		ASC_FREE(vars);
		return 0;
	}

	for(i = 0; i < count; ++i){
		int32 col = a4sqp_view_find_var_index(view,vars[i]);
		if(col >= 0){
			view->obj_gradient[col] = obj_sign * derivs[i];
		}
	}

	ASC_FREE(derivs);
	ASC_FREE(vars);
	return 0;
}

int a4sqp_view_build(struct A4SqpView *view, slv_system_t server, int safe, const char *scaleopt){
	if(view == NULL || server == NULL){
		return 1;
	}

	a4sqp_view_destroy(view);
	view->vars = slv_get_solvers_var_list(server);
	view->rels = slv_get_solvers_rel_list(server);
	view->obj = slv_get_obj_relation(server);
	view->n_var = a4sqp_count_vars(view->vars);
	view->n_rel = a4sqp_count_rels(view->rels);

	if(view->vars == NULL || view->rels == NULL){
		return 1;
	}

	if(a4sqp_view_alloc(view)){
		a4sqp_view_destroy(view);
		return 1;
	}

	a4sqp_view_capture_vars(view);
	a4sqp_view_capture_rels(view,safe);
	if(a4sqp_view_capture_jacobian(view,safe)){
		a4sqp_view_destroy(view);
		return 1;
	}
	if(a4sqp_view_capture_objective(view,safe)){
		a4sqp_view_destroy(view);
		return 1;
	}
	if(a4sqp_view_apply_scaling(view,scaleopt)){
		a4sqp_view_destroy(view);
		return 1;
	}

	return 0;
}
