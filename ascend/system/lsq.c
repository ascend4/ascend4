/*
 * Optional least-squares system view.
 */

#include "lsq.h"

#include <string.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_util.h>

#include "system_impl.h"
#include "slv_client.h"
#include "rel.h"
#include "var.h"

struct system_lsq_build_state {
	struct system_lsq_view *view;
	struct rel_relation *objective;
	unsigned long next;
};

static struct rel_relation *system_lsq_first_objective(slv_system_t sys){
	struct rel_relation *obj;
	struct rel_relation **objs;

	if(sys == NULL){
		return NULL;
	}
	obj = slv_get_obj_relation(sys);
	if(obj != NULL){
		return obj;
	}
	objs = slv_get_solvers_obj_list(sys);
	if(objs == NULL){
		return NULL;
	}
	return objs[0];
}

static const struct relation *system_lsq_compiler_relation(struct rel_relation *obj){
	if(obj == NULL || rel_instance(obj) == NULL){
		return NULL;
	}
	return GetInstanceRelationOnly((struct Instance *)rel_instance(obj));
}

static struct var_variable *system_lsq_find_solver_var(slv_system_t sys, struct Instance *inst){
	struct var_variable **vars;
	if(sys == NULL || inst == NULL){
		return NULL;
	}
	vars = slv_get_solvers_var_list(sys);
	if(vars == NULL){
		return NULL;
	}
	for(; *vars != NULL; ++vars){
		if((struct Instance *)var_instance(*vars) == inst){
			return *vars;
		}
	}
	return NULL;
}

static int system_lsq_store_residual(CONST struct relation_term *residual_term, double weight, void *userdata){
	struct system_lsq_build_state *state = (struct system_lsq_build_state *)userdata;
	struct system_lsq_residual *residual;

	if(state == NULL || state->view == NULL || state->next >= state->view->nresiduals){
		return 1;
	}
	residual = &state->view->residuals[state->next++];
	residual->objective = state->objective;
	residual->residual_term = residual_term;
	residual->weight = weight;
	return 0;
}

void system_clear_lsq_view(slv_system_t sys){
	if(sys == NULL || sys->lsq_view == NULL){
		return;
	}
	if(sys->lsq_view->residuals != NULL){
		ASC_FREE(sys->lsq_view->residuals);
		sys->lsq_view->residuals = NULL;
	}
	if(sys->lsq_view->projected_sindex != NULL){
		ASC_FREE(sys->lsq_view->projected_sindex);
		sys->lsq_view->projected_sindex = NULL;
	}
	ASC_FREE(sys->lsq_view);
	sys->lsq_view = NULL;
}

const struct system_lsq_view *system_get_lsq_view(slv_system_t sys){
	if(sys == NULL || sys->lsq_view == NULL || !sys->lsq_view->valid){
		return NULL;
	}
	return sys->lsq_view;
}

int system_lsq_eval_residual(slv_system_t sys, unsigned long index, double *residual){
	const struct system_lsq_view *view;
	const struct system_lsq_residual *entry;
	const struct relation *rel;
	enum safe_err status;

	if(residual == NULL){
		return 1;
	}
	view = system_get_lsq_view(sys);
	if(view == NULL || index >= view->nresiduals){
		return 1;
	}
	entry = &view->residuals[index];
	rel = system_lsq_compiler_relation(entry->objective);
	if(rel == NULL || entry->residual_term == NULL){
		return 1;
	}
	status = RelationEvaluateTermSafe(rel, entry->residual_term, residual);
	return status == safe_ok ? 0 : 1;
}

int system_lsq_eval_residuals(slv_system_t sys, double *residuals){
	const struct system_lsq_view *view;
	unsigned long i;

	if(residuals == NULL){
		return 1;
	}
	view = system_get_lsq_view(sys);
	if(view == NULL){
		return 1;
	}
	for(i = 0; i < view->nresiduals; ++i){
		if(system_lsq_eval_residual(sys, i, &residuals[i])){
			return 1;
		}
	}
	return 0;
}

int system_lsq_eval_jacobian_row(
	slv_system_t sys,
	unsigned long index,
	int *columns,
	double *values,
	unsigned long capacity,
	unsigned long *nnz
){
	const struct system_lsq_view *view;
	const struct system_lsq_residual *entry;
	const struct relation *rel;
	enum safe_err status;
	unsigned long nvar;
	unsigned long i;
	unsigned long count = 0;
	double residual;
	double *gradient;

	if(nnz == NULL){
		return 1;
	}
	*nnz = 0;
	view = system_get_lsq_view(sys);
	if(view == NULL || index >= view->nresiduals){
		return 1;
	}
	entry = &view->residuals[index];
	rel = system_lsq_compiler_relation(entry->objective);
	if(rel == NULL || entry->residual_term == NULL){
		return 1;
	}

	nvar = NumberVariables(rel);
	gradient = ASC_NEW_ARRAY_CLEAR(double, nvar);
	if(gradient == NULL){
		return 1;
	}
	status = RelationEvaluateTermGradientSafe(rel, entry->residual_term, &residual, gradient, nvar);
	if(status != safe_ok){
		ASC_FREE(gradient);
		return 1;
	}

	for(i = 1; i <= nvar; ++i){
		struct Instance *inst;
		struct var_variable *var;
		int sindex;
		double value = gradient[i - 1];
		if(value == 0.0){
			continue;
		}
		inst = RelationVariable(rel, i);
		var = system_lsq_find_solver_var(sys, inst);
		if(var == NULL){
			ASC_FREE(gradient);
			return 1;
		}
		sindex = var_sindex(var);
		if(sindex < 0){
			ASC_FREE(gradient);
			return 1;
		}
		if(count < capacity){
			if(columns == NULL || values == NULL){
				ASC_FREE(gradient);
				return 1;
			}
			columns[count] = sindex;
			values[count] = value;
		}
		count++;
	}
	*nnz = count;
	ASC_FREE(gradient);
	if(capacity > 0 && count > capacity){
		return 1;
	}
	return 0;
}

static int system_lsq_build_projection(slv_system_t sys, struct system_lsq_view *view){
	const struct relation *rel;
	unsigned long nvar;
	unsigned long i;
	unsigned long row;
	unsigned long count = 0;
	unsigned char *candidate = NULL;
	unsigned char *single = NULL;

	if(sys == NULL || view == NULL || view->objective == NULL || view->nresiduals == 0){
		return 0;
	}
	rel = system_lsq_compiler_relation(view->objective);
	if(rel == NULL){
		return 0;
	}
	nvar = NumberVariables(rel);
	if(nvar == 0){
		return 0;
	}
	candidate = ASC_NEW_ARRAY_CLEAR(unsigned char,nvar + 1);
	single = ASC_NEW_ARRAY_CLEAR(unsigned char,nvar + 1);
	if(candidate == NULL || single == NULL){
		ASC_FREE(candidate);
		ASC_FREE(single);
		return 0;
	}
	for(i = 1; i <= nvar; ++i){
		struct Instance *inst = RelationVariable(rel,i);
		struct var_variable *var = system_lsq_find_solver_var(sys,inst);
		int affine = 1;
		if(var == NULL || var_sindex(var) < 0){
			continue;
		}
		memset(single,0,nvar + 1);
		single[i] = 1;
		for(row = 0; row < view->nresiduals; ++row){
			if(!RelationTermIsAffineInVariables(view->residuals[row].residual_term,single,nvar + 1)){
				affine = 0;
				break;
			}
		}
		if(affine){
			candidate[i] = 1;
			count++;
		}
	}
	if(count == 0 || count >= nvar){
		ASC_FREE(candidate);
		ASC_FREE(single);
		return 0;
	}
	for(row = 0; row < view->nresiduals; ++row){
		if(!RelationTermIsAffineInVariables(view->residuals[row].residual_term,candidate,nvar + 1)){
			ASC_FREE(candidate);
			ASC_FREE(single);
			return 0;
		}
	}
	view->projected_sindex = ASC_NEW_ARRAY_OR_NULL(int,count);
	if(view->projected_sindex == NULL){
		ASC_FREE(candidate);
		ASC_FREE(single);
		return 0;
	}
	view->nprojected = 0;
	for(i = 1; i <= nvar; ++i){
		if(candidate[i]){
			struct var_variable *var = system_lsq_find_solver_var(sys,RelationVariable(rel,i));
			if(var == NULL || var_sindex(var) < 0){
				ASC_FREE(view->projected_sindex);
				view->projected_sindex = NULL;
				view->nprojected = 0;
				ASC_FREE(candidate);
				ASC_FREE(single);
				return 0;
			}
			view->projected_sindex[view->nprojected++] = var_sindex(var);
		}
	}
	ASC_FREE(candidate);
	ASC_FREE(single);
	return view->nprojected > 0;
}

int system_analyse_lsq_objective(slv_system_t sys, unsigned flags, struct RelationLeastSquaresAnalysis *analysis){
	struct RelationLeastSquaresAnalysis local;
	struct rel_relation *obj;
	const struct relation *rel;
	struct system_lsq_view *view;
	struct system_lsq_build_state state;
	int is_lsq;

	if(analysis == NULL){
		analysis = &local;
	}
	memset(analysis, 0, sizeof(*analysis));

	if(flags & SYSTEM_LSQ_ANALYSE_BUILD_VIEW){
		system_clear_lsq_view(sys);
	}

	obj = system_lsq_first_objective(sys);
	rel = system_lsq_compiler_relation(obj);
	is_lsq = RelationAnalyzeLeastSquaresObjective(rel, analysis);
	if(!is_lsq || !(flags & SYSTEM_LSQ_ANALYSE_BUILD_VIEW)){
		return is_lsq;
	}

	view = ASC_NEW_CLEAR(struct system_lsq_view);
	view->valid = 0;
	view->objective = obj;
	view->analysis = *analysis;
	view->nresiduals = analysis->residual_count;
	view->residuals = ASC_NEW_ARRAY_CLEAR(struct system_lsq_residual, view->nresiduals);

	state.view = view;
	state.objective = obj;
	state.next = 0;

	is_lsq = RelationAnalyzeLeastSquaresObjectiveWithResiduals(
		rel,
		&view->analysis,
		&system_lsq_store_residual,
		&state
	);
	if(!is_lsq || state.next != view->nresiduals){
		*analysis = view->analysis;
		if(view->residuals != NULL){
			ASC_FREE(view->residuals);
		}
		ASC_FREE(view);
		return 0;
	}

	if(flags & SYSTEM_LSQ_ANALYSE_BUILD_PROJECTION){
		system_lsq_build_projection(sys,view);
	}
	view->valid = 1;
	sys->lsq_view = view;
	*analysis = view->analysis;
	return 1;
}
