/*
 * ASCEND slv_system_t construction for A4SQP problem views.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_view.h"

#include "asc_a4sqp_adapter.h"
#include "a4sqp_scale.h"

#include <ascend/system/relman.h>
#include <ascend/system/rel.h>
#include <ascend/system/slv_client.h>
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

static int a4sqp_view_capture_solver_vars(struct A4SqpView *view, struct var_variable **vars){
	var_filter_t vfilter;
	int32 total;
	int32 i;
	int32 n = 0;
	if(view == NULL || vars == NULL){
		return 1;
	}
	total = a4sqp_count_vars(vars);
	view->vars = A4SQP_NEW_ARRAY_OR_NULL(void *,total + 1);
	if(view->vars == NULL){
		return 1;
	}
	view->owns_vars = 1;
	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	for(i = 0; i < total; ++i){
		if(var_apply_filter(vars[i],&vfilter)){
			view->vars[n++] = vars[i];
		}
	}
	view->vars[n] = NULL;
	view->n_var = n;
	return 0;
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

static int a4sqp_view_alloc(struct A4SqpView *view){
	if(view->n_var > 0){
		view->var_mindex = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_var);
		view->var_sindex = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_var);
		view->var_value = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_nominal = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->var_fixed = A4SQP_NEW_ARRAY_OR_NULL(uint32,view->n_var);
		view->var_scale = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_var_value = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_var_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_var_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->obj_gradient = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		view->scaled_obj_gradient = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
		if(view->var_mindex == NULL || view->var_sindex == NULL || view->var_value == NULL
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
		view->rel_sindex = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_rel);
		view->relop = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_rel);
		view->rel_kind = A4SQP_NEW_ARRAY_OR_NULL(enum A4SqpRelKind,view->n_rel);
		view->rel_residual = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->rel_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->rel_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->rel_nominal = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->rel_scale = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->scaled_rel_residual = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->scaled_rel_lower = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->scaled_rel_upper = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_rel);
		view->jac_row_start = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_rel + 1);
		if(view->rel_sindex == NULL || view->relop == NULL || view->rel_kind == NULL
			|| view->rel_residual == NULL || view->rel_lower == NULL
			|| view->rel_upper == NULL || view->rel_nominal == NULL || view->rel_scale == NULL
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
	if(view != NULL && sindex >= 0 && sindex < view->n_var && view->var_sindex != NULL && view->var_sindex[sindex] == sindex){
		return sindex;
	}
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
		struct var_variable *var = (struct var_variable *)view->vars[i];
		view->var_mindex[i] = var_mindex(var);
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
		struct rel_relation *rel = (struct rel_relation *)view->rels[i];
		view->rel_sindex[i] = rel_sindex(rel);
		view->relop[i] = (int32)rel_relop(rel);
		view->rel_kind[i] = view->relop[i] == (int32)e_rel_equal
			? A4SQP_REL_KIND_EQUALITY
			: A4SQP_REL_KIND_INEQUALITY;
		if(a4sqp_map_rel_bounds((enum rel_enum)view->relop[i],&view->rel_lower[i],&view->rel_upper[i])){
			++view->unsupported_rels;
		}
		view->rel_nominal[i] = rel_nominal(rel);
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

	row_derivs = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var > 0 ? view->n_var : 1);
	row_vars = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_var > 0 ? view->n_var : 1);
	if(row_derivs == NULL || row_vars == NULL){
		A4SQP_FREE(row_derivs);
		A4SQP_FREE(row_vars);
		return 1;
	}

	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	view->jac_row_start[0] = 0;

	for(i = 0; i < view->n_rel; ++i){
		int32 mapped_count = 0;
		int32 j;
		int32 count = 0;
		struct rel_relation *rel = (struct rel_relation *)view->rels[i];
		int err = relman_diff2_rev(rel,&vfilter,row_derivs,row_vars,&count,safe);
		if(err){
			++view->derivative_errors;
			count = 0;
		}
		for(j = 0; j < count; ++j){
			if(a4sqp_view_find_var_index(view,row_vars[j]) >= 0){
				++mapped_count;
			}
		}
		if(mapped_count > view->jac_max_row_nnz){
			view->jac_max_row_nnz = mapped_count;
		}
		nnz += mapped_count;
		view->jac_row_start[i + 1] = nnz;
	}

	view->jac_nnz = nnz;
	if(nnz > 0){
		view->jac_col_index = A4SQP_NEW_ARRAY_OR_NULL(int32,nnz);
		view->jac_col_sindex = A4SQP_NEW_ARRAY_OR_NULL(int32,nnz);
		view->jac_value = A4SQP_NEW_ARRAY_OR_NULL(real64,nnz);
		view->scaled_jac_value = A4SQP_NEW_ARRAY_OR_NULL(real64,nnz);
		if(view->jac_col_index == NULL || view->jac_col_sindex == NULL
			|| view->jac_value == NULL || view->scaled_jac_value == NULL
		){
			A4SQP_FREE(row_derivs);
			A4SQP_FREE(row_vars);
			return 1;
		}

		nnz = 0;
		for(i = 0; i < view->n_rel; ++i){
			int32 j;
			int32 count = 0;
			struct rel_relation *rel = (struct rel_relation *)view->rels[i];
			int err = relman_diff2_rev(rel,&vfilter,row_derivs,row_vars,&count,safe);
				if(err){
					count = 0;
				}
				for(j = 0; j < count; ++j){
					int32 col = a4sqp_view_find_var_index(view,row_vars[j]);
					if(col < 0){
						continue;
					}
					view->jac_col_sindex[nnz] = row_vars[j];
					view->jac_col_index[nnz] = col;
					view->jac_value[nnz] = row_derivs[j];
					++nnz;
				}
		}
	}

	A4SQP_FREE(row_derivs);
	A4SQP_FREE(row_vars);
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

	view->obj_direction = relman_obj_direction((struct rel_relation *)view->obj);
	if(view->obj_direction > 0){
		obj_sign = -1.0;
	}
	view->obj_value = obj_sign * relman_eval((struct rel_relation *)view->obj,&calc_ok,safe);
	if(!calc_ok){
		++view->obj_calc_errors;
		return 0;
	}

	if(view->n_var <= 0){
		return 0;
	}

	derivs = A4SQP_NEW_ARRAY_OR_NULL(real64,view->n_var);
	vars = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_var);
	if(derivs == NULL || vars == NULL){
		A4SQP_FREE(derivs);
		A4SQP_FREE(vars);
		return 1;
	}
	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	if(relman_diff2_rev((struct rel_relation *)view->obj,&vfilter,derivs,vars,&count,safe)){
		++view->obj_derivative_errors;
		A4SQP_FREE(derivs);
		A4SQP_FREE(vars);
		return 0;
	}

	for(i = 0; i < count; ++i){
		int32 col = a4sqp_view_find_var_index(view,vars[i]);
		if(col >= 0){
			view->obj_gradient[col] = obj_sign * derivs[i];
		}
	}

	A4SQP_FREE(derivs);
	A4SQP_FREE(vars);
	return 0;
}

int a4sqp_view_build(struct A4SqpView *view, slv_system_t server, int safe, const char *scaleopt){
	struct var_variable **vars;
	if(view == NULL || server == NULL){
		return 1;
	}

	a4sqp_view_destroy(view);
	vars = slv_get_solvers_var_list(server);
	view->rels = (void **)slv_get_solvers_rel_list(server);
	view->obj = (void *)slv_get_obj_relation(server);
	view->n_rel = a4sqp_count_rels((struct rel_relation **)view->rels);

	if(vars == NULL || view->rels == NULL){
		return 1;
	}
	if(a4sqp_view_capture_solver_vars(view,vars)){
		a4sqp_view_destroy(view);
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
