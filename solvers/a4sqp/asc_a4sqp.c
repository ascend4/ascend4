/*
 * A4SQP solver registration and minimal lifecycle.
 */

#define ASC_BUILDING_INTERFACE

#include "asc_a4sqp.h"

#include "asc_a4sqp_adapter.h"
#include "a4sqp_core.h"
#include "asc_a4sqp_diag.h"
#include "a4sqp_hessian.h"
#include "asc_a4sqp_internal.h"
#include "a4sqp_qp_highs.h"
#include "asc_a4sqp_report.h"
#include "a4sqp_trust.h"

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/safe.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/ltmatrix.h>
#include <ascend/general/mem.h>
#include <ascend/system/relman.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/slv_param.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/var.h>
#include <ascend/utilities/error.h>

#ifdef A4SQP_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...)
#endif

enum A4SqpHessianMode {
	A4SQP_HESS_BFGS = 0,
	A4SQP_HESS_EXACT_OBJ,
	A4SQP_HESS_EXACT_LAGRANGIAN,
	A4SQP_HESS_AUTO
};

struct A4SqpHessTripletEntry {
	int32 col;
	int32 row;
	real64 value;
};

struct A4SqpHessTriplet {
	int32 n;
	int32 nnz;
	int32 cap;
	struct A4SqpHessTripletEntry *entry;
};

static int32 a4sqp_view_var_col(const struct A4SqpView *view, int32 sindex);
static int32 a4sqp_view_var_col_from_var(const struct A4SqpView *view, const struct var_variable *var);
static int a4sqp_eval_objective_gradient_current(const struct A4SqpSystem *sys, int safe, real64 *grad);
static int a4sqp_step_hess_accumulate_objective_fd(struct A4SqpSystem *sys, struct A4SqpHessTriplet *triplet, real64 coeff, int safe, int *mapped, real64 *local_maxabs);
static void a4sqp_step_hess_sparse_destroy(struct A4SqpSystem *sys);
static void a4sqp_step_hess_spec(const struct A4SqpSystem *sys, struct A4SqpStepHessian *spec);
static void a4sqp_lambda_est_destroy(struct A4SqpSystem *sys);
static int a4sqp_lambda_est_sync(struct A4SqpSystem *sys);
static void a4sqp_lambda_est_reset(struct A4SqpSystem *sys);
static void a4sqp_lambda_est_update(struct A4SqpSystem *sys);
static void a4sqp_x_destroy(struct A4SqpSystem *sys);
static int a4sqp_x_sync_from_view(struct A4SqpSystem *sys);
static int a4sqp_x_push_to_ascend(struct A4SqpSystem *sys, const real64 *x);
static void a4sqp_init_status(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	memset(&sys->status,0,sizeof(sys->status));
	sys->status.kind = SLV_STATUS_NLP;
	sys->status.ok = TRUE;
	sys->status.calc_ok = TRUE;
	sys->status.ready_to_solve = FALSE;
	sys->status.converged = FALSE;
	sys->status.iteration = 0;
	sys->status.cpu_elapsed = 0.0;
	sys->last_merit_before = 0.0;
	sys->last_merit_after = 0.0;
	sys->last_model_merit_after = 0.0;
	sys->last_predicted_reduction = 0.0;
	sys->last_linearized_violation = 0.0;
	sys->last_violation_sum = 0.0;
	sys->last_violation_max = 0.0;
	sys->last_alpha = 0.0;
	sys->last_step_norm = 0.0;
	sys->trust_radius = 0.0;
	sys->last_trust_ratio = 0.0;
	sys->worst_violation_rel = -1;
	sys->line_search_failed = 0;
	sys->last_elastic_max = 0.0;
}

static void a4sqp_trust_options_from_params(
	const struct A4SqpSystem *sys,
	struct A4SqpTrustOptions *opt
){
	if(sys == NULL || opt == NULL){
		return;
	}
	opt->radius_init = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_TRUST_RADIUS_INIT);
	opt->radius_min = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_TRUST_RADIUS_MIN);
	opt->radius_max = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_TRUST_RADIUS_MAX);
	opt->shrink = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_TRUST_SHRINK);
	opt->grow = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_TRUST_GROW);
	opt->good_ratio = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_TRUST_GOOD);
}

static void a4sqp_trust_reset(struct A4SqpSystem *sys){
	struct A4SqpTrustOptions opt;
	if(sys == NULL){
		return;
	}
	a4sqp_trust_options_from_params(sys,&opt);
	sys->trust_radius = a4sqp_trust_initial_radius(&opt);
	sys->last_trust_ratio = 0.0;
}

static int a4sqp_trust_shrink(struct A4SqpSystem *sys){
	struct A4SqpTrustOptions opt;
	if(sys == NULL){
		return 0;
	}
	a4sqp_trust_options_from_params(sys,&opt);
	return a4sqp_trust_shrink_radius(&sys->trust_radius,&opt);
}

static void a4sqp_system_trust_grow_if_good(
	struct A4SqpSystem *sys,
	real64 scaled_step_inf
){
	struct A4SqpTrustOptions opt;
	if(sys == NULL){
		return;
	}
	a4sqp_trust_options_from_params(sys,&opt);
	a4sqp_trust_grow_if_good(
		&sys->trust_radius,
		&opt,
		sys->last_alpha,
		scaled_step_inf,
		sys->last_trust_ratio
	);
}

static void a4sqp_step_hess_destroy(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	ASC_FREE(sys->step_hess);
	sys->step_hess = NULL;
	a4sqp_step_hess_sparse_destroy(sys);
	sys->step_hess_n = 0;
	sys->step_hess_updates = 0;
	sys->step_hess_exact = 0;
}

static void a4sqp_lambda_est_destroy(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	ASC_FREE(sys->lambda_est);
	sys->lambda_est = NULL;
	sys->lambda_est_n = 0;
	sys->lambda_est_ready = 0;
	sys->lambda_est_good_count = 0;
	sys->lambda_est_required_count = 0;
	sys->last_elastic_max = 0.0;
}

static void a4sqp_x_destroy(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	ASC_FREE(sys->x);
	sys->x = NULL;
	sys->x_n = 0;
}

static int a4sqp_x_sync_from_view(struct A4SqpSystem *sys){
	int32 i;
	if(sys == NULL){
		return 1;
	}
	if(sys->x_n != sys->view.n_var || (sys->view.n_var > 0 && sys->x == NULL)){
		a4sqp_x_destroy(sys);
		if(sys->view.n_var > 0){
			sys->x = ASC_NEW_ARRAY_OR_NULL(real64,sys->view.n_var);
			if(sys->x == NULL){
				return 1;
			}
		}
		sys->x_n = sys->view.n_var;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		sys->x[i] = sys->view.var_value[i];
	}
	return 0;
}

static int a4sqp_x_push_to_ascend(struct A4SqpSystem *sys, const real64 *x){
	int32 i;
	if(sys == NULL || x == NULL || sys->view.vars == NULL || sys->view.n_var != sys->x_n){
		return 1;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		var_set_value((struct var_variable *)sys->view.vars[i],x[i]);
	}
	return 0;
}

static int a4sqp_lambda_est_sync(struct A4SqpSystem *sys){
	int32 n;
	if(sys == NULL){
		return 1;
	}
	n = sys->view.n_rel;
	if(sys->lambda_est_n == n && (n == 0 || sys->lambda_est != NULL)){
		return 0;
	}
	a4sqp_lambda_est_destroy(sys);
	if(n > 0){
		sys->lambda_est = ASC_NEW_ARRAY_CLEAR(real64,n);
		if(sys->lambda_est == NULL){
			return 1;
		}
	}
	sys->lambda_est_n = n;
	sys->lambda_est_ready = 0;
	sys->lambda_est_good_count = 0;
	sys->lambda_est_required_count = 0;
	sys->last_elastic_max = 0.0;
	return 0;
}

static void a4sqp_lambda_est_reset(struct A4SqpSystem *sys){
	int32 i;
	if(a4sqp_lambda_est_sync(sys)){
		return;
	}
	for(i = 0; i < sys->lambda_est_n; ++i){
		sys->lambda_est[i] = 0.0;
	}
	sys->lambda_est_ready = 0;
	sys->lambda_est_good_count = 0;
	sys->lambda_est_required_count = 0;
	sys->last_elastic_max = 0.0;
}

static void a4sqp_step_hess_sparse_destroy(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	ASC_FREE(sys->step_hess_sparse_start);
	ASC_FREE(sys->step_hess_sparse_index);
	ASC_FREE(sys->step_hess_sparse_value);
	sys->step_hess_sparse_start = NULL;
	sys->step_hess_sparse_index = NULL;
	sys->step_hess_sparse_value = NULL;
	sys->step_hess_sparse_n = 0;
	sys->step_hess_sparse_nnz = 0;
}

static int a4sqp_step_hess_reset_identity(struct A4SqpSystem *sys, real64 diag){
	int32 i;
	int32 n;
	if(sys == NULL){
		return 1;
	}
	n = sys->view.n_var;
	if(n < 0){
		return 1;
	}
	if(sys->step_hess_n != n || sys->step_hess == NULL){
		a4sqp_step_hess_destroy(sys);
		if(n > 0){
			sys->step_hess = ASC_NEW_ARRAY_OR_NULL(real64,n * n);
			if(sys->step_hess == NULL){
				return 1;
			}
		}
		sys->step_hess_n = n;
	}
	if(diag <= 0.0 || !isfinite(diag)){
		diag = 1.0;
	}
	a4sqp_step_hess_sparse_destroy(sys);
	for(i = 0; i < n * n; ++i){
		sys->step_hess[i] = 0.0;
	}
	for(i = 0; i < n; ++i){
		sys->step_hess[i * n + i] = diag;
	}
	sys->step_hess_updates = 0;
	sys->step_hess_exact = 0;
	return 0;
}

static int a4sqp_step_hess_sync(struct A4SqpSystem *sys){
	if(sys == NULL){
		return 1;
	}
	if(sys->step_hess_n == sys->view.n_var && (sys->view.n_var == 0 || sys->step_hess != NULL)){
		return 0;
	}
	return a4sqp_step_hess_reset_identity(sys,1.0);
}

static enum A4SqpHessianMode a4sqp_step_hess_mode(const struct A4SqpSystem *sys){
	const char *mode;
	if(sys == NULL){
		return A4SQP_HESS_AUTO;
	}
	mode = SLV_PARAM_CHAR(&sys->params,A4SQP_PARAM_HESS_MODE);
	if(mode == NULL || strcmp(mode,"AUTO") == 0){
		return A4SQP_HESS_AUTO;
	}
	if(strcmp(mode,"BFGS") == 0){
		return A4SQP_HESS_BFGS;
	}
	if(strcmp(mode,"EXACT_OBJ") == 0){
		return A4SQP_HESS_EXACT_OBJ;
	}
	if(strcmp(mode,"EXACT_LAGRANGIAN") == 0){
		return A4SQP_HESS_EXACT_LAGRANGIAN;
	}
	return A4SQP_HESS_AUTO;
}

static int a4sqp_step_hess_mode_uses_exact(
	const struct A4SqpSystem *sys,
	int *include_constraints
){
	enum A4SqpHessianMode mode;
	if(include_constraints != NULL){
		*include_constraints = 0;
	}
	if(sys == NULL || sys->view.obj == NULL){
		return 0;
	}
	mode = a4sqp_step_hess_mode(sys);
	switch(mode){
	case A4SQP_HESS_BFGS:
		return 0;
	case A4SQP_HESS_EXACT_OBJ:
		return 1;
	case A4SQP_HESS_EXACT_LAGRANGIAN:
		if(include_constraints != NULL){
			*include_constraints = 1;
		}
		return 1;
	case A4SQP_HESS_AUTO:
	default:
		if(sys->view.n_rel == 0){
			return 1;
		}
		if(
			sys->view.n_var <= 64
			&& sys->view.n_rel <= 64
			&& sys->lambda_est_ready
			&& sys->lambda_est != NULL
			&& sys->lambda_est_good_count >= sys->lambda_est_required_count
			&& sys->last_elastic_max <= 10.0 * SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL)
			&& !sys->line_search_failed
		){
			if(include_constraints != NULL){
				*include_constraints = 1;
			}
			return 1;
		}
		return 0;
	}
}

static int a4sqp_step_hess_clear(struct A4SqpSystem *sys){
	int32 n;
	if(a4sqp_step_hess_sync(sys)){
		return 1;
	}
	n = sys->step_hess_n;
	if(n > 0 && sys->step_hess != NULL){
		memset(sys->step_hess,0,sizeof(real64) * n * n);
	}
	a4sqp_step_hess_sparse_destroy(sys);
	sys->step_hess_updates = 0;
	sys->step_hess_exact = 0;
	return 0;
}

static void a4sqp_hess_triplet_init(struct A4SqpHessTriplet *triplet, int32 n){
	if(triplet == NULL){
		return;
	}
	triplet->n = n;
	triplet->nnz = 0;
	triplet->cap = 0;
	triplet->entry = NULL;
}

static void a4sqp_hess_triplet_destroy(struct A4SqpHessTriplet *triplet){
	if(triplet == NULL){
		return;
	}
	ASC_FREE(triplet->entry);
	triplet->n = 0;
	triplet->nnz = 0;
	triplet->cap = 0;
}

static int a4sqp_hess_triplet_reserve(struct A4SqpHessTriplet *triplet, int32 need){
	struct A4SqpHessTripletEntry *entry;
	int32 cap;
	if(triplet == NULL){
		return 1;
	}
	if(need <= triplet->cap){
		return 0;
	}
	cap = triplet->cap > 0 ? triplet->cap : 16;
	while(cap < need){
		if(cap > INT_MAX / 2){
			cap = need;
			break;
		}
		cap *= 2;
	}
	entry = ASC_NEW_ARRAY_OR_NULL(struct A4SqpHessTripletEntry,cap);
	if(entry == NULL){
		return 1;
	}
	if(triplet->entry != NULL && triplet->nnz > 0){
		memcpy(entry,triplet->entry,sizeof(*entry) * triplet->nnz);
	}
	ASC_FREE(triplet->entry);
	triplet->entry = entry;
	triplet->cap = cap;
	return 0;
}

static int a4sqp_hess_triplet_append(
	struct A4SqpHessTriplet *triplet,
	int32 row,
	int32 col,
	real64 value
){
	struct A4SqpHessTripletEntry *entry;
	if(
		triplet == NULL
		|| row < 0
		|| col < 0
		|| row >= triplet->n
		|| col >= triplet->n
		|| !isfinite(value)
		|| fabs(value) <= 1e-18
	){
		return 0;
	}
	if(row < col){
		int32 swap = row;
		row = col;
		col = swap;
	}
	if(a4sqp_hess_triplet_reserve(triplet,triplet->nnz + 1)){
		return 1;
	}
	entry = &triplet->entry[triplet->nnz++];
	entry->col = col;
	entry->row = row;
	entry->value = value;
	return 0;
}

static int a4sqp_hess_triplet_cmp(const void *a, const void *b){
	const struct A4SqpHessTripletEntry *ea = (const struct A4SqpHessTripletEntry *)a;
	const struct A4SqpHessTripletEntry *eb = (const struct A4SqpHessTripletEntry *)b;
	if(ea->col < eb->col)return -1;
	if(ea->col > eb->col)return 1;
	if(ea->row < eb->row)return -1;
	if(ea->row > eb->row)return 1;
	return 0;
}

static int a4sqp_step_hess_sparse_store_triplet(
	struct A4SqpSystem *sys,
	struct A4SqpHessTriplet *triplet
){
	int32 col;
	int32 i;
	int32 out_nnz = 0;
	int32 nnz;
	int32 *counts = NULL;
	int32 *start = NULL;
	int32 *index = NULL;
	real64 *value = NULL;

	if(sys == NULL || triplet == NULL){
		return 1;
	}
	a4sqp_step_hess_sparse_destroy(sys);
	sys->step_hess_sparse_n = triplet->n;
	nnz = triplet->nnz;
	if(nnz <= 0){
		start = ASC_NEW_ARRAY_OR_NULL(int32,triplet->n + 1);
		index = ASC_NEW_ARRAY_OR_NULL(int32,1);
		value = ASC_NEW_ARRAY_OR_NULL(real64,1);
		if(start == NULL || index == NULL || value == NULL){
			ASC_FREE(start);
			ASC_FREE(index);
			ASC_FREE(value);
			return 1;
		}
		for(col = 0; col <= triplet->n; ++col){
			start[col] = 0;
		}
		sys->step_hess_sparse_start = start;
		sys->step_hess_sparse_index = index;
		sys->step_hess_sparse_value = value;
		sys->step_hess_sparse_nnz = 0;
		return 0;
	}

	qsort(triplet->entry,(size_t)nnz,sizeof(triplet->entry[0]),&a4sqp_hess_triplet_cmp);
	for(i = 0; i < nnz; ++i){
		if(fabs(triplet->entry[i].value) <= 1e-18){
			continue;
		}
		if(
			out_nnz > 0
			&& triplet->entry[i].col == triplet->entry[out_nnz - 1].col
			&& triplet->entry[i].row == triplet->entry[out_nnz - 1].row
		){
			triplet->entry[out_nnz - 1].value += triplet->entry[i].value;
		}else{
			triplet->entry[out_nnz++] = triplet->entry[i];
		}
	}
	nnz = 0;
	for(i = 0; i < out_nnz; ++i){
		if(fabs(triplet->entry[i].value) > 1e-18){
			triplet->entry[nnz++] = triplet->entry[i];
		}
	}
	counts = ASC_NEW_ARRAY_CLEAR(int32,triplet->n);
	start = ASC_NEW_ARRAY_OR_NULL(int32,triplet->n + 1);
	index = ASC_NEW_ARRAY_OR_NULL(int32,nnz > 0 ? nnz : 1);
	value = ASC_NEW_ARRAY_OR_NULL(real64,nnz > 0 ? nnz : 1);
	if(counts == NULL || start == NULL || index == NULL || value == NULL){
		ASC_FREE(counts);
		ASC_FREE(start);
		ASC_FREE(index);
		ASC_FREE(value);
		return 1;
	}
	for(i = 0; i < nnz; ++i){
		++counts[triplet->entry[i].col];
	}
	start[0] = 0;
	for(col = 0; col < triplet->n; ++col){
		start[col + 1] = start[col] + counts[col];
		counts[col] = start[col];
	}
	for(i = 0; i < nnz; ++i){
		int32 pos = counts[triplet->entry[i].col]++;
		index[pos] = triplet->entry[i].row;
		value[pos] = triplet->entry[i].value;
	}
	ASC_FREE(counts);
	sys->step_hess_sparse_start = start;
	sys->step_hess_sparse_index = index;
	sys->step_hess_sparse_value = value;
	sys->step_hess_sparse_nnz = nnz;
	return 0;
}

static int a4sqp_step_hess_dense_store_triplet(
	struct A4SqpSystem *sys,
	const struct A4SqpHessTriplet *triplet
){
	int32 i;
	int32 n;
	if(sys == NULL || triplet == NULL){
		return 1;
	}
	n = triplet->n;
	if(a4sqp_step_hess_sync(sys)){
		return 1;
	}
	if(sys->step_hess_n != n || (n > 0 && sys->step_hess == NULL)){
		return 1;
	}
	a4sqp_step_hess_sparse_destroy(sys);
	if(n > 0){
		memset(sys->step_hess,0,sizeof(real64) * n * n);
	}
	for(i = 0; i < triplet->nnz; ++i){
		int32 col = triplet->entry[i].col;
		int32 row = triplet->entry[i].row;
		real64 value = triplet->entry[i].value;
		if(
			col < 0 || row < 0 || col >= n || row >= n
			|| !isfinite(value) || fabs(value) <= 1e-18
		){
			continue;
		}
		sys->step_hess[row * n + col] += value;
		if(row != col){
			sys->step_hess[col * n + row] += value;
		}
	}
	return 0;
}

static int a4sqp_step_hess_accumulate_relation(
	struct A4SqpSystem *sys,
	struct A4SqpHessTriplet *triplet,
	struct rel_relation *rel,
	real64 coeff,
	int safe
){
	const struct var_variable **incidence;
	real64 *row2nd = NULL;
	int32 len;
	int32 i;
	int32 j;
	int32 n;
	int status = 0;
	int mapped = 0;
	real64 local_maxabs = 0.0;
	if(
		sys == NULL
		|| triplet == NULL
		|| rel == NULL
		|| triplet->n <= 0
		|| !isfinite(coeff)
		|| fabs(coeff) <= 1e-18
	){
		return 0;
	}
	n = triplet->n;
	len = rel_n_incidences(rel);
	if(len <= 0){
		if(SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_PROGRESS_LOG)){
			char message[160];
			snprintf(message,sizeof(message),"hessian_relation: empty incidence len=%ld coeff=%g",(long)len,coeff);
			a4sqp_report_progress(&sys->params,message);
		}
		return 0;
	}
	incidence = (const struct var_variable **)rel_incidence_list(rel);
	if(incidence == NULL){
		if(SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_PROGRESS_LOG)){
			char message[160];
			snprintf(message,sizeof(message),"hessian_relation: null incidence len=%ld coeff=%g",(long)len,coeff);
			a4sqp_report_progress(&sys->params,message);
		}
		return 1;
	}
	row2nd = ASC_NEW_ARRAY_OR_NULL(real64,len);
	if(row2nd == NULL){
		return 1;
	}

	for(i = 0; i < len; ++i){
		int32 row = a4sqp_view_var_col_from_var(&sys->view,(const struct var_variable *)incidence[i]);
		real64 row_scale;
		if(safe){
			enum safe_err serr = RelationCalcSecondDerivSafe(rel_instance(rel),row2nd,(unsigned long)i);
			if(serr != safe_ok){
				safe_error_to_stderr(&serr);
				status = 1;
			}
		}else{
			status = RelationCalcSecondDeriv(rel_instance(rel),row2nd,(unsigned long)i);
		}
		if(status){
			ASC_FREE(row2nd);
			return 1;
		}
		if(row < 0 || row >= n){
			continue;
		}
		row_scale = sys->view.var_scale[row];
		for(j = 0; j <= i; ++j){
			int32 col = a4sqp_view_var_col_from_var(&sys->view,(const struct var_variable *)incidence[j]);
			real64 value;
			real64 scaled_value;
			real64 col_scale;
			if(col < 0 || col >= n){
				continue;
			}
			value = row2nd[j];
			if(fabs(value) > local_maxabs){
				local_maxabs = fabs(value);
			}
			if(!isfinite(value) || fabs(value) <= 1e-18){
				continue;
			}
			col_scale = sys->view.var_scale[col];
			scaled_value = coeff * value * row_scale * col_scale;
			if(a4sqp_hess_triplet_append(triplet,row,col,scaled_value)){
				ASC_FREE(row2nd);
				return 1;
			}
			++mapped;
		}
	}
	if(SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_PROGRESS_LOG)){
		char message[256];
		snprintf(
			message,
			sizeof(message),
			"hessian_relation: len=%ld mapped=%d local_maxabs=%g coeff=%g",
			(long)len,
			mapped,
			local_maxabs,
			coeff
		);
		a4sqp_report_progress(&sys->params,message);
	}
	if(mapped == 0 && local_maxabs == 0.0 && rel == (struct rel_relation *)sys->view.obj){
		if(a4sqp_step_hess_accumulate_objective_fd(sys,triplet,coeff,safe,&mapped,&local_maxabs)){
			ASC_FREE(row2nd);
			return 1;
		}
		if(SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_PROGRESS_LOG)){
			char message[256];
			snprintf(
				message,
				sizeof(message),
				"hessian_relation_fd: mapped=%d local_maxabs=%g coeff=%g",
				mapped,
				local_maxabs,
				coeff
			);
			a4sqp_report_progress(&sys->params,message);
		}
	}

	ASC_FREE(row2nd);
	return 0;
}

static int a4sqp_eval_objective_gradient_current(const struct A4SqpSystem *sys, int safe, real64 *grad){
	int32 i;
	int32 count = 0;
	int32 *vars = NULL;
	real64 *derivs = NULL;
	var_filter_t vfilter;
	real64 sign = 1.0;
	if(sys == NULL || grad == NULL || sys->view.obj == NULL){
		return 1;
	}
	for(i = 0; i < sys->view.n_var; ++i){
		grad[i] = 0.0;
	}
	if(sys->view.n_var <= 0){
		return 0;
	}
	derivs = ASC_NEW_ARRAY_OR_NULL(real64,sys->view.n_var);
	vars = ASC_NEW_ARRAY_OR_NULL(int32,sys->view.n_var);
	if(derivs == NULL || vars == NULL){
		ASC_FREE(derivs);
		ASC_FREE(vars);
		return 1;
	}
	if(sys->view.obj_direction > 0){
		sign = -1.0;
	}
	vfilter.matchbits = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR | VAR_FIXED;
	vfilter.matchvalue = VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR;
	if(relman_diff2_rev((struct rel_relation *)sys->view.obj,&vfilter,derivs,vars,&count,safe)){
		ASC_FREE(derivs);
		ASC_FREE(vars);
		return 1;
	}
	for(i = 0; i < count; ++i){
		int32 col = a4sqp_view_var_col(&sys->view,vars[i]);
		if(col >= 0 && col < sys->view.n_var){
			grad[col] = sign * derivs[i];
		}
	}
	ASC_FREE(derivs);
	ASC_FREE(vars);
	return 0;
}

static int a4sqp_step_hess_accumulate_objective_fd(
	struct A4SqpSystem *sys,
	struct A4SqpHessTriplet *triplet,
	real64 coeff,
	int safe,
	int *mapped,
	real64 *local_maxabs
){
	int32 i;
	int32 j;
	int32 n;
	real64 *gplus = NULL;
	real64 *gminus = NULL;
	if(
		sys == NULL
		|| triplet == NULL
		|| sys->view.obj == NULL
		|| sys->view.n_var <= 0
		|| sys->view.vars == NULL
	){
		return 1;
	}
	n = sys->view.n_var;
	gplus = ASC_NEW_ARRAY_OR_NULL(real64,n);
	gminus = ASC_NEW_ARRAY_OR_NULL(real64,n);
	if(gplus == NULL || gminus == NULL){
		ASC_FREE(gplus);
		ASC_FREE(gminus);
		return 1;
	}
	for(i = 0; i < n; ++i){
		real64 old_value = sys->view.var_value[i];
		real64 base = fabs(old_value);
		real64 step;
		if(sys->view.var_scale != NULL && sys->view.var_scale[i] > base){
			base = sys->view.var_scale[i];
		}
		if(base < 1.0){
			base = 1.0;
		}
		step = 1e-6 * base;
		var_set_value((struct var_variable *)sys->view.vars[i],old_value + step);
		if(a4sqp_eval_objective_gradient_current(sys,safe,gplus)){
			var_set_value((struct var_variable *)sys->view.vars[i],old_value);
			ASC_FREE(gplus);
			ASC_FREE(gminus);
			return 1;
		}
		var_set_value((struct var_variable *)sys->view.vars[i],old_value - step);
		if(a4sqp_eval_objective_gradient_current(sys,safe,gminus)){
			var_set_value((struct var_variable *)sys->view.vars[i],old_value);
			ASC_FREE(gplus);
			ASC_FREE(gminus);
			return 1;
		}
		var_set_value((struct var_variable *)sys->view.vars[i],old_value);
		for(j = 0; j <= i; ++j){
			real64 value = (gplus[j] - gminus[j]) / (2.0 * step);
			real64 scaled_value = coeff * value * sys->view.var_scale[i] * sys->view.var_scale[j];
			if(local_maxabs != NULL && fabs(value) > *local_maxabs){
				*local_maxabs = fabs(value);
			}
			if(!isfinite(value) || fabs(value) <= 1e-18){
				continue;
			}
			if(a4sqp_hess_triplet_append(triplet,i,j,scaled_value)){
				var_set_value((struct var_variable *)sys->view.vars[i],old_value);
				ASC_FREE(gplus);
				ASC_FREE(gminus);
				return 1;
			}
			if(mapped != NULL){
				++(*mapped);
			}
		}
	}
	ASC_FREE(gplus);
	ASC_FREE(gminus);
	return 0;
}

static real64 a4sqp_step_hess_regularize_psd(struct A4SqpSystem *sys){
	real64 min_diag;
	if(sys == NULL || sys->step_hess == NULL){
		return 0.0;
	}
	min_diag = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_HESS_REG);
	return a4sqp_hessian_dense_regularize_psd(sys->step_hess,sys->step_hess_n,min_diag);
}

static int a4sqp_step_hess_sparse_add_diagonal_shift(struct A4SqpSystem *sys, real64 shift){
	int32 col;
	int32 old_pos;
	int32 new_pos = 0;
	int32 n;
	int32 missing_diag = 0;
	int32 *start = NULL;
	int32 *index = NULL;
	real64 *value = NULL;

	if(sys == NULL){
		return 1;
	}
	n = sys->step_hess_sparse_n;
	if(n < 0 || sys->step_hess_sparse_start == NULL){
		return 1;
	}
	for(col = 0; col < n; ++col){
		int found = 0;
		for(old_pos = sys->step_hess_sparse_start[col]; old_pos < sys->step_hess_sparse_start[col + 1]; ++old_pos){
			if(sys->step_hess_sparse_index[old_pos] == col){
				found = 1;
				break;
			}
			if(sys->step_hess_sparse_index[old_pos] > col){
				break;
			}
		}
		if(!found){
			++missing_diag;
		}
	}
	start = ASC_NEW_ARRAY_OR_NULL(int32,n + 1);
	index = ASC_NEW_ARRAY_OR_NULL(int32,sys->step_hess_sparse_nnz + missing_diag);
	value = ASC_NEW_ARRAY_OR_NULL(real64,sys->step_hess_sparse_nnz + missing_diag);
	if(start == NULL || index == NULL || value == NULL){
		ASC_FREE(start);
		ASC_FREE(index);
		ASC_FREE(value);
		return 1;
	}
	start[0] = 0;
	for(col = 0; col < n; ++col){
		int diag_done = 0;
		start[col] = new_pos;
		for(old_pos = sys->step_hess_sparse_start[col]; old_pos < sys->step_hess_sparse_start[col + 1]; ++old_pos){
			int32 row = sys->step_hess_sparse_index[old_pos];
			if(!diag_done && row > col){
				index[new_pos] = col;
				value[new_pos] = shift;
				++new_pos;
				diag_done = 1;
			}
			index[new_pos] = row;
			value[new_pos] = sys->step_hess_sparse_value[old_pos];
			if(row == col){
				value[new_pos] += shift;
				diag_done = 1;
			}
			++new_pos;
		}
		if(!diag_done){
			index[new_pos] = col;
			value[new_pos] = shift;
			++new_pos;
		}
		start[col + 1] = new_pos;
	}
	ASC_FREE(sys->step_hess_sparse_start);
	ASC_FREE(sys->step_hess_sparse_index);
	ASC_FREE(sys->step_hess_sparse_value);
	sys->step_hess_sparse_start = start;
	sys->step_hess_sparse_index = index;
	sys->step_hess_sparse_value = value;
	sys->step_hess_sparse_nnz = new_pos;
	return 0;
}

static real64 a4sqp_step_hess_sparse_regularize_psd(struct A4SqpSystem *sys){
	int32 col;
	int32 k;
	int32 n;
	real64 *diag = NULL;
	real64 *offsum = NULL;
	real64 min_diag;
	real64 shift = 0.0;

	if(
		sys == NULL
		|| sys->step_hess_sparse_n <= 0
		|| sys->step_hess_sparse_start == NULL
		|| sys->step_hess_sparse_index == NULL
		|| sys->step_hess_sparse_value == NULL
	){
		return 0.0;
	}
	n = sys->step_hess_sparse_n;
	min_diag = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_HESS_REG);
	if(!isfinite(min_diag) || min_diag < 0.0){
		min_diag = 1e-8;
	}
	diag = ASC_NEW_ARRAY_CLEAR(real64,n);
	offsum = ASC_NEW_ARRAY_CLEAR(real64,n);
	if(diag == NULL || offsum == NULL){
		ASC_FREE(diag);
		ASC_FREE(offsum);
		return 0.0;
	}
	for(col = 0; col < n; ++col){
		for(k = sys->step_hess_sparse_start[col]; k < sys->step_hess_sparse_start[col + 1]; ++k){
			int32 row = sys->step_hess_sparse_index[k];
			real64 v = sys->step_hess_sparse_value[k];
			if(!isfinite(v)){
				v = 0.0;
				sys->step_hess_sparse_value[k] = 0.0;
			}
			if(row == col){
				diag[col] += v;
			}else{
				real64 a = fabs(v);
				offsum[col] += a;
				if(row >= 0 && row < n){
					offsum[row] += a;
				}
			}
		}
	}
	for(col = 0; col < n; ++col){
		real64 need = offsum[col] + min_diag - diag[col];
		if(need > shift){
			shift = need;
		}
	}
	if(shift < 0.0 || !isfinite(shift)){
		shift = 0.0;
	}
	if(shift > 0.0 || min_diag > 0.0){
		if(a4sqp_step_hess_sparse_add_diagonal_shift(sys,shift > 0.0 ? shift : min_diag)){
			shift = 0.0;
		}else if(shift <= 0.0){
			shift = min_diag;
		}
	}
	ASC_FREE(diag);
	ASC_FREE(offsum);
	return shift;
}

static int a4sqp_step_hess_build_exact(struct A4SqpSystem *sys){
	int safe;
	int include_constraints = 0;
	int row;
	int status = 0;
	real64 obj_coeff = 1.0;
	real64 reg_shift = 0.0;
	struct A4SqpHessTriplet triplet;
	if(sys == NULL || !a4sqp_step_hess_mode_uses_exact(sys,&include_constraints)){
		return 1;
	}
	if(a4sqp_step_hess_clear(sys)){
		return 1;
	}
	a4sqp_hess_triplet_init(&triplet,sys->view.n_var);
	safe = SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_SAFE_CALC);
	if(sys->view.obj_direction > 0){
		obj_coeff = -1.0;
	}
	status = a4sqp_step_hess_accumulate_relation(sys,&triplet,(struct rel_relation *)sys->view.obj,obj_coeff,safe);
	if(status){
		a4sqp_hess_triplet_destroy(&triplet);
		return 1;
	}
	if(include_constraints && sys->view.n_rel > 0){
		for(row = 0; row < sys->view.n_rel; ++row){
			real64 lambda = 0.0;
			real64 coeff;
			if(
				sys->lambda_est == NULL
				|| sys->lambda_est_n != sys->view.n_rel
				|| !sys->lambda_est_ready
			){
				continue;
			}
			lambda = sys->lambda_est[row];
			if(!isfinite(lambda) || fabs(lambda) <= 1e-18){
				continue;
			}
			coeff = lambda;
			if(a4sqp_step_hess_accumulate_relation(sys,&triplet,(struct rel_relation *)sys->view.rels[row],coeff,safe)){
				a4sqp_hess_triplet_destroy(&triplet);
				return 1;
			}
		}
	}
	if(include_constraints){
		if(a4sqp_step_hess_sparse_store_triplet(sys,&triplet)){
			a4sqp_hess_triplet_destroy(&triplet);
			return 1;
		}
		reg_shift = a4sqp_step_hess_sparse_regularize_psd(sys);
	}else{
		if(a4sqp_step_hess_dense_store_triplet(sys,&triplet)){
			a4sqp_hess_triplet_destroy(&triplet);
			return 1;
		}
		reg_shift = a4sqp_step_hess_regularize_psd(sys);
	}
	a4sqp_hess_triplet_destroy(&triplet);
	if(SLV_PARAM_BOOL(&sys->params,A4SQP_PARAM_PROGRESS_LOG)){
		char message[256];
		snprintf(
			message,
			sizeof(message),
			"hessian: exact mode=%s constraints=%d reg_shift=%g sparse_nnz=%ld",
			SLV_PARAM_CHAR(&sys->params,A4SQP_PARAM_HESS_MODE),
			include_constraints,
			reg_shift,
			(long)(include_constraints ? sys->step_hess_sparse_nnz : 0)
		);
		a4sqp_report_progress(&sys->params,message);
	}
	sys->step_hess_exact = 1;
	return 0;
}

static int a4sqp_step_hess_prepare(struct A4SqpSystem *sys){
	int include_constraints = 0;
	if(sys == NULL){
		return 1;
	}
	if(a4sqp_step_hess_mode_uses_exact(sys,&include_constraints)){
		if(a4sqp_step_hess_build_exact(sys) == 0){
			return 0;
		}
		return a4sqp_step_hess_reset_identity(sys,1.0);
	}
	return a4sqp_step_hess_sync(sys);
}

static void a4sqp_step_hess_spec(const struct A4SqpSystem *sys, struct A4SqpStepHessian *spec){
	if(spec == NULL){
		return;
	}
	memset(spec,0,sizeof(*spec));
	if(sys == NULL){
		return;
	}
	spec->n = sys->view.n_var;
	if(
		sys->step_hess_exact
		&& sys->step_hess_sparse_n == sys->view.n_var
		&& sys->step_hess_sparse_start != NULL
		&& sys->step_hess_sparse_index != NULL
		&& sys->step_hess_sparse_value != NULL
	){
		spec->is_sparse = 1;
		spec->nnz = sys->step_hess_sparse_nnz;
		spec->start = sys->step_hess_sparse_start;
		spec->index = sys->step_hess_sparse_index;
		spec->value = sys->step_hess_sparse_value;
		return;
	}
	spec->dense = sys->step_hess;
}

static void a4sqp_step_hess_mul(
	const struct A4SqpSystem *sys,
	const real64 *x,
	real64 *y
){
	int32 n;
	if(sys == NULL || x == NULL || y == NULL){
		return;
	}
	n = sys->step_hess_n;
	a4sqp_hessian_dense_mul(sys->step_hess,n,x,y);
}

static void a4sqp_step_hess_update_objective_only(
	struct A4SqpSystem *sys,
	const real64 *old_scaled_x,
	const real64 *old_scaled_grad
){
	int32 i;
	int32 j;
	int32 n;
	real64 *s = NULL;
	real64 *y = NULL;
	real64 *bs = NULL;
	real64 *r = NULL;
	real64 sty = 0.0;
	real64 yty = 0.0;
	real64 stbs = 0.0;
	real64 str = 0.0;
	real64 theta = 1.0;
	real64 gamma = 1.0;
	const real64 eps = 1e-12;

	if(
		sys == NULL
		|| sys->view.obj == NULL
		|| sys->view.n_rel != 0
		|| sys->view.n_var <= 0
		|| old_scaled_x == NULL
		|| old_scaled_grad == NULL
	){
		return;
	}
	n = sys->view.n_var;
	if(a4sqp_step_hess_sync(sys)){
		return;
	}
	sys->step_hess_exact = 0;

	s = ASC_NEW_ARRAY_OR_NULL(real64,n);
	y = ASC_NEW_ARRAY_OR_NULL(real64,n);
	bs = ASC_NEW_ARRAY_OR_NULL(real64,n);
	r = ASC_NEW_ARRAY_OR_NULL(real64,n);
	if(s == NULL || y == NULL || bs == NULL || r == NULL){
		ASC_FREE(s);
		ASC_FREE(y);
		ASC_FREE(bs);
		ASC_FREE(r);
		return;
	}

	for(i = 0; i < n; ++i){
		s[i] = sys->view.scaled_var_value[i] - old_scaled_x[i];
		y[i] = sys->view.scaled_obj_gradient[i] - old_scaled_grad[i];
		sty += s[i] * y[i];
		yty += y[i] * y[i];
	}
	if(sty <= eps || yty <= eps){
		goto cleanup;
	}

	if(sys->step_hess_updates == 0){
		gamma = yty / sty;
		if(!isfinite(gamma) || gamma < 1e-8){
			gamma = 1e-8;
		}else if(gamma > 1e8){
			gamma = 1e8;
		}
		if(a4sqp_step_hess_reset_identity(sys,gamma)){
			goto cleanup;
		}
	}

	a4sqp_step_hess_mul(sys,s,bs);
	for(i = 0; i < n; ++i){
		stbs += s[i] * bs[i];
	}
	if(stbs <= eps){
		goto cleanup;
	}

	if(sty < 0.2 * stbs){
		theta = 0.8 * stbs / (stbs - sty);
	}
	for(i = 0; i < n; ++i){
		r[i] = theta * y[i] + (1.0 - theta) * bs[i];
		str += s[i] * r[i];
	}
	if(str <= eps){
		goto cleanup;
	}

	for(i = 0; i < n; ++i){
		for(j = 0; j < n; ++j){
			sys->step_hess[i * n + j]
				+= (r[i] * r[j]) / str
				- (bs[i] * bs[j]) / stbs;
		}
	}
	for(i = 0; i < n; ++i){
		for(j = i + 1; j < n; ++j){
			real64 sym = 0.5 * (sys->step_hess[i * n + j] + sys->step_hess[j * n + i]);
			sys->step_hess[i * n + j] = sym;
			sys->step_hess[j * n + i] = sym;
		}
		if(!isfinite(sys->step_hess[i * n + i]) || sys->step_hess[i * n + i] < 1e-8){
			sys->step_hess[i * n + i] = 1e-8;
		}
	}
	++sys->step_hess_updates;

cleanup:
	ASC_FREE(s);
	ASC_FREE(y);
	ASC_FREE(bs);
	ASC_FREE(r);
}

static void a4sqp_spoof_block_status(struct A4SqpSystem *sys){
	struct slv__block_status_structure *block;
	int32 size;
	if(sys == NULL){
		return;
	}
	block = &sys->status.block;
	size = sys->view.n_var > 0 ? sys->view.n_var : 0;
	block->number_of = 1;
	block->current_block = 0;
	block->current_reordered_block = 0;
	block->current_size = size;
	block->previous_total_size = 0;
	block->previous_total_size_vars = 0;
	block->iteration = sys->status.iteration;
	block->funcs = 0;
	block->jacs = 0;
	block->cpu_elapsed = sys->status.cpu_elapsed;
	block->functime = 0.0;
	block->jactime = 0.0;
	block->residual = sys->last_violation_max;
}

static SlvClientToken a4sqp_create(slv_system_t server, int *statusindex){
	struct A4SqpSystem *sys;

	sys = ASC_NEW_CLEAR(struct A4SqpSystem);
	if(sys == NULL){
		if(statusindex != NULL){
			*statusindex = 1;
		}
		return NULL;
	}

	sys->server = server;
	sys->params.parms = sys->param_data;
	sys->params.dynamic_parms = 0;
	a4sqp_get_default_parameters(server,(SlvClientToken)sys,&sys->params);
	if(statusindex != NULL){
		sys->params.whose = *statusindex;
	}

	a4sqp_view_init(&sys->view);
	a4sqp_qp_init(&sys->qp);
	a4sqp_init_status(sys);

	if(statusindex != NULL){
		*statusindex = 0;
	}
	
	MSG("System created");

	return (SlvClientToken)sys;
}

static int a4sqp_destroy(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;

	if(sys == NULL){
		return 0;
	}

	a4sqp_view_destroy(&sys->view);
	a4sqp_qp_destroy(&sys->qp);
	a4sqp_x_destroy(sys);
	a4sqp_step_hess_destroy(sys);
	a4sqp_lambda_est_destroy(sys);
	slv_destroy_parms(&sys->params);
	ascfree(sys);
	
	MSG("System destroyed");
	
	return 0;
}

static int a4sqp_eligible(slv_system_t server){
	(void)server;
	return 1;
}

static void a4sqp_get_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || parameters == NULL){
		return;
	}
	mem_copy_cast(&sys->params,parameters,sizeof(slv_parameters_t));
}

static void a4sqp_set_parameters(slv_system_t server, SlvClientToken asys, slv_parameters_t *parameters){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || parameters == NULL){
		return;
	}
	if(parameters->whose == A4SQP_SOLVER_NUMBER){
		mem_copy_cast(parameters,&sys->params,sizeof(slv_parameters_t));
	}
}

static int a4sqp_get_status(slv_system_t server, SlvClientToken asys, slv_status_t *status){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || status == NULL){
		return 1;
	}
	mem_copy_cast(&sys->status,status,sizeof(slv_status_t));
	return 0;
}

static int32 a4sqp_view_var_col(const struct A4SqpView *view, int32 sindex){
	int32 i;
	if(view == NULL){
		return -1;
	}
	for(i = 0; i < view->n_var; ++i){
		if(view->var_sindex[i] == sindex){
			return i;
		}
	}
	return -1;
}

static int32 a4sqp_view_var_col_from_var(const struct A4SqpView *view, const struct var_variable *var){
	int32 i;
	if(view == NULL || var == NULL){
		return -1;
	}
	for(i = 0; i < view->n_var; ++i){
		if((struct var_variable *)view->vars[i] == var){
			return i;
		}
		if(var_instance((struct var_variable *)view->vars[i]) == var_instance(var)){
			return i;
		}
		if(view->var_mindex != NULL && view->var_mindex[i] == var_mindex(var)){
			return i;
		}
		if(view->var_sindex[i] == var_sindex(var)){
			return i;
		}
	}
	return -1;
}

static void a4sqp_lambda_est_update(struct A4SqpSystem *sys){
	int32 row;
	int32 good_count = 0;
	int32 required_count = 0;
	real64 elastic_max = 0.0;
	real64 active_tol;
	real64 near_tol;
	real64 feas_tol;
	real64 elastic_penalty;
	if(
		sys == NULL
		|| sys->view.n_rel <= 0
		|| a4sqp_lambda_est_sync(sys)
		|| sys->qp.row_dual == NULL
		|| sys->qp.col_value == NULL
		|| sys->qp.num_row != sys->view.n_rel
	){
		return;
	}
	feas_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
	active_tol = fmax(10.0 * feas_tol,1e-8);
	near_tol = fmax(100.0 * feas_tol,10.0 * active_tol);
	elastic_penalty = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_ELASTIC_PENALTY);
	for(row = 0; row < sys->view.n_rel; ++row){
		enum A4SqpRowActivity activity;
		int32 lower_col = sys->view.n_var + 2 * row;
		int32 upper_col = lower_col + 1;
		real64 lower_elastic = 0.0;
		real64 upper_elastic = 0.0;
		real64 elastic = 0.0;
		real64 raw_scaled = 0.0;
		real64 sample = 0.0;
		real64 estimate = 0.0;
		int good = 1;
		int required = 0;
		activity = a4sqp_core_row_activity(&sys->view,row,active_tol,near_tol);
		required = activity != A4SQP_ROW_INACTIVE;
		if(required){
			++required_count;
		}
		if(lower_col >= 0 && lower_col < sys->qp.num_col){
			lower_elastic = fabs(sys->qp.col_value[lower_col]);
		}
		if(upper_col >= 0 && upper_col < sys->qp.num_col){
			upper_elastic = fabs(sys->qp.col_value[upper_col]);
		}
		elastic = lower_elastic > upper_elastic ? lower_elastic : upper_elastic;
		if(elastic > elastic_max){
			elastic_max = elastic;
		}
		raw_scaled = sys->qp.row_dual[row];
		if(!isfinite(raw_scaled)){
			raw_scaled = 0.0;
			good = 0;
		}
		sample = raw_scaled * sys->view.rel_scale[row];
		if(!isfinite(sample)){
			sample = 0.0;
			good = 0;
		}
		if(elastic > 10.0 * feas_tol){
			good = 0;
		}
		if(fabs(raw_scaled) >= 0.95 * elastic_penalty){
			good = 0;
		}
		switch(activity){
		case A4SQP_ROW_EQUALITY:
			if(good){
				estimate = sample;
				if(sys->lambda_est_ready){
					estimate = 0.75 * sys->lambda_est[row] + 0.25 * estimate;
				}
			}else{
				estimate = sys->lambda_est_ready ? 0.90 * sys->lambda_est[row] : 0.0;
			}
			break;
		case A4SQP_ROW_ACTIVE:
			if(good){
				estimate = sample;
				if(sys->lambda_est_ready){
					estimate = 0.65 * sys->lambda_est[row] + 0.35 * estimate;
				}
			}else{
				estimate = sys->lambda_est_ready ? 0.80 * sys->lambda_est[row] : 0.0;
			}
			break;
		case A4SQP_ROW_NEAR_ACTIVE:
			if(good){
				estimate = sample;
				if(sys->lambda_est_ready){
					estimate = 0.85 * sys->lambda_est[row] + 0.15 * estimate;
				}
			}else{
				estimate = sys->lambda_est_ready ? 0.85 * sys->lambda_est[row] : 0.0;
			}
			break;
		case A4SQP_ROW_INACTIVE:
		default:
			if(good && fabs(sample) <= near_tol){
				estimate = sys->lambda_est_ready ? 0.20 * sys->lambda_est[row] + 0.10 * sample : sample;
			}else{
				estimate = sys->lambda_est_ready ? 0.20 * sys->lambda_est[row] : 0.0;
			}
			break;
		}
		if(required){
			if(good){
				++good_count;
			}
		}else if(fabs(estimate) <= near_tol){
			++good_count;
		}
		if(!isfinite(estimate) || fabs(estimate) <= 1e-16){
			estimate = 0.0;
		}
		sys->lambda_est[row] = estimate;
	}
	sys->last_elastic_max = elastic_max;
	sys->lambda_est_ready = 1;
	sys->lambda_est_good_count = good_count;
	sys->lambda_est_required_count = required_count;
}

static void a4sqp_update_metrics(struct A4SqpSystem *sys){
	if(sys == NULL){
		return;
	}
	sys->last_violation_sum = a4sqp_core_view_violation(
		&sys->view,
		&sys->last_violation_max,
		&sys->worst_violation_rel
	);
	a4sqp_spoof_block_status(sys);
}

static int a4sqp_has_converged(struct A4SqpSystem *sys){
	struct A4SqpConvergencePolicy policy;
	real64 feas_tol;
	real64 step_tol;
	real64 proj_grad_inf = 0.0;
	if(sys == NULL){
		return 0;
	}
	feas_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
	step_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_STEP_TOL);
	a4sqp_update_metrics(sys);
	policy.feas_tol = feas_tol;
	policy.step_tol = step_tol;
	policy.last_step_norm = sys->last_step_norm;
	policy.has_objective = sys->view.obj != NULL;
	policy.constrained_objective_allows_small_step = 1;
	return a4sqp_core_has_converged(
		&sys->view,
		&policy,
		&sys->last_violation_max,
		&proj_grad_inf,
		&sys->worst_violation_rel
	);
}

struct A4SqpAscendLineSearchCtx {
	slv_system_t server;
	struct A4SqpSystem *sys;
};

static int asc_a4sqp_ls_evaluate(void *ctx, const real64 *x, struct A4SqpView *view){
	struct A4SqpAscendLineSearchCtx *ls = (struct A4SqpAscendLineSearchCtx *)ctx;
	if(ls == NULL || ls->sys == NULL || x == NULL || view == NULL || view != &ls->sys->view){
		return 1;
	}
	if(a4sqp_x_push_to_ascend(ls->sys,x)){
		return 1;
	}
	if(asc_a4sqp_build_view(ls->sys,ls->server)){
		return 1;
	}
	a4sqp_update_metrics(ls->sys);
	return 0;
}

static void asc_a4sqp_ls_accepted(
	void *ctx,
	const real64 *old_scaled_x,
	const real64 *old_scaled_grad
){
	struct A4SqpAscendLineSearchCtx *ls = (struct A4SqpAscendLineSearchCtx *)ctx;
	if(ls == NULL || ls->sys == NULL || ls->sys->step_hess_exact){
		return;
	}
	a4sqp_step_hess_update_objective_only(ls->sys,old_scaled_x,old_scaled_grad);
}

static void a4sqp_record_line_search_result(
	struct A4SqpSystem *sys,
	const struct A4SqpLineSearchResult *result
){
	if(sys == NULL || result == NULL){
		return;
	}
	sys->last_merit_before = result->merit_before;
	sys->last_merit_after = result->merit_after;
	sys->last_model_merit_after = result->model_merit_after;
	sys->last_predicted_reduction = result->predicted_reduction;
	sys->last_alpha = result->alpha;
	sys->last_step_norm = result->step_norm;
	sys->last_trust_ratio = result->trust_ratio;
	if(result->accepted){
		if(result->alpha > 0.0){
			a4sqp_system_trust_grow_if_good(sys,result->scaled_step_inf);
		}
	}else{
		a4sqp_update_metrics(sys);
		sys->line_search_failed = 1;
	}
}

struct A4SqpAscendCoreStepCtx {
	slv_system_t server;
	struct A4SqpSystem *sys;
};

static int asc_a4sqp_core_prepare_hessian(void *vctx, struct A4SqpStepHessian *step_hess){
	struct A4SqpAscendCoreStepCtx *ctx = (struct A4SqpAscendCoreStepCtx *)vctx;
	if(ctx == NULL || ctx->sys == NULL || step_hess == NULL){
		return 1;
	}
	if(a4sqp_step_hess_prepare(ctx->sys)){
		return 1;
	}
	a4sqp_step_hess_spec(ctx->sys,step_hess);
	return 0;
}

static int asc_a4sqp_core_solve_qp(void *vctx, struct A4SqpQp *qp){
	struct A4SqpAscendCoreStepCtx *ctx = (struct A4SqpAscendCoreStepCtx *)vctx;
	if(ctx == NULL || ctx->sys == NULL){
		return 1;
	}
	return a4sqp_qp_solve_highs(
		qp,
		SLV_PARAM_REAL(&ctx->sys->params,A4SQP_PARAM_FEAS_TOL),
		SLV_PARAM_BOOL(&ctx->sys->params,A4SQP_PARAM_PROGRESS_LOG)
	) == 0 ? 0 : 1;
}

static void asc_a4sqp_core_after_qp_solve(void *vctx, const struct A4SqpQp *qp){
	struct A4SqpAscendCoreStepCtx *ctx = (struct A4SqpAscendCoreStepCtx *)vctx;
	(void)qp;
	if(ctx == NULL || ctx->sys == NULL){
		return;
	}
	a4sqp_lambda_est_update(ctx->sys);
}

static int asc_a4sqp_core_shrink_trust(void *vctx, const char *reason){
	struct A4SqpAscendCoreStepCtx *ctx = (struct A4SqpAscendCoreStepCtx *)vctx;
	char message[256];
	if(ctx == NULL || ctx->sys == NULL){
		return 1;
	}
	if(!a4sqp_trust_shrink(ctx->sys)){
		return 1;
	}
	if(reason != NULL && strcmp(reason,"line-search") == 0){
		snprintf(
			message,
			sizeof(message),
			"trust: shrinking radius after line-search failure to %g",
			ctx->sys->trust_radius
		);
	}else{
		snprintf(
			message,
			sizeof(message),
			"trust: shrinking radius after QP failure to %g",
			ctx->sys->trust_radius
		);
	}
	a4sqp_report_progress(&ctx->sys->params,message);
	return 0;
}

static int a4sqp_presolve(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	int32 sorted_rels = 0;
	int32 sorted_vars = 0;

	if(sys == NULL){
		return 1;
	}

	sys->server = server;
	a4sqp_init_status(sys);

	MSG("Presolve...");

	slv_sort_rels_and_vars(server,&sorted_rels,&sorted_vars);
	if(sorted_vars < 0 || sorted_rels < 0){
		sys->status.ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to sort ASCEND solver variable/relation lists.");
		return 1;
	}

	if(asc_a4sqp_build_view(sys,server)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to build the ASCEND problem view.");
		return 1;
	}
	if(a4sqp_x_sync_from_view(sys)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to initialize the core iterate vector.");
		return 1;
	}
	if(a4sqp_step_hess_prepare(sys)){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to initialize the step Hessian model.");
		return 1;
	}
	a4sqp_trust_reset(sys);
	a4sqp_lambda_est_reset(sys);

	if(sys->view.unsupported_rels > 0){
		sys->status.ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP Phase 1 does not support %ld relation(s) in the selected solver list.",
			(long)sys->view.unsupported_rels
		);
		asc_a4sqp_report_view(sys);
		return 1;
	}

	if(sys->view.calc_errors > 0){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP encountered residual evaluation errors in %ld relation(s).",
			(long)sys->view.calc_errors
		);
		asc_a4sqp_report_view(sys);
		return 1;
	}

	if(sys->view.derivative_errors > 0){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP encountered derivative evaluation errors in %ld relation(s).",
			(long)sys->view.derivative_errors
		);
		asc_a4sqp_report_view(sys);
		return 1;
	}

	sys->status.ready_to_solve = TRUE;
	a4sqp_update_metrics(sys);
	asc_a4sqp_report_view(sys);
	MSG("Presolve completed");
	return 0;
}

static int a4sqp_iterate(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	struct A4SqpAscendCoreStepCtx step_ctx;
	struct A4SqpCoreStepOptions step_options;
	struct A4SqpCoreStepOps step_ops;
	struct A4SqpCoreStepStats step_stats;
	struct A4SqpAscendLineSearchCtx line_ctx;
	struct A4SqpVectorLineSearchOps line_ops;
	struct A4SqpLineSearchOptions line_options;
	struct A4SqpLineSearchResult line_result;
	enum A4SqpCoreStepStatus step_status;

	MSG("Iterate...");

	if(sys == NULL){
		return 1;
	}
	if(!sys->status.ready_to_solve){
		if(a4sqp_presolve(server,asys)){
			return 1;
		}
	}
	memset(&step_ctx,0,sizeof(step_ctx));
	memset(&step_options,0,sizeof(step_options));
	memset(&step_ops,0,sizeof(step_ops));
	memset(&line_ctx,0,sizeof(line_ctx));
	memset(&line_ops,0,sizeof(line_ops));
	memset(&line_options,0,sizeof(line_options));
	memset(&line_result,0,sizeof(line_result));
	step_ctx.server = server;
	step_ctx.sys = sys;
	line_ctx.server = server;
	line_ctx.sys = sys;
	step_options.trust_qp_retries = SLV_PARAM_INT(&sys->params,A4SQP_PARAM_TRUST_QP_RETRIES);
	step_options.elastic_penalty = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_ELASTIC_PENALTY);
	step_options.feas_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
	step_options.merit_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_MERIT_TOL);
	line_options.max_backtrack = SLV_PARAM_INT(&sys->params,A4SQP_PARAM_MAX_BACKTRACK);
	line_options.merit_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_MERIT_TOL);
	line_options.feas_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_FEAS_TOL);
	line_options.step_tol = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_STEP_TOL);
	line_options.armijo_coeff = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_ARMIJO_COEFF);
	line_options.trust_accept = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_TRUST_ACCEPT);
	line_options.elastic_penalty = SLV_PARAM_REAL(&sys->params,A4SQP_PARAM_ELASTIC_PENALTY);
	line_ops.evaluate = asc_a4sqp_ls_evaluate;
	line_ops.accepted = asc_a4sqp_ls_accepted;
	sys->line_search_failed = 0;
	step_ops.prepare_hessian = asc_a4sqp_core_prepare_hessian;
	step_ops.solve_qp = asc_a4sqp_core_solve_qp;
	step_ops.after_qp_solve = asc_a4sqp_core_after_qp_solve;
	step_ops.shrink_trust = asc_a4sqp_core_shrink_trust;
	step_status = a4sqp_core_solve_step(
		&sys->view,
		&sys->qp,
		&sys->trust_radius,
		&step_options,
		&step_ops,
		&step_ctx,
		&line_options,
		&line_ops,
		sys->x,
		sys->view.obj != NULL,
		&line_result,
		&step_stats
	);
	if(line_result.accepted || step_status == A4SQP_CORE_STEP_LINE_SEARCH_ERROR){
		a4sqp_record_line_search_result(sys,&line_result);
	}
	if(step_status == A4SQP_CORE_STEP_HESSIAN_ERROR){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to prepare the step Hessian model.");
		return 1;
	}
	if(step_status == A4SQP_CORE_STEP_QP_BUILD_ERROR){
		sys->status.ok = FALSE;
		sys->status.calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP failed to assemble the HiGHS QP subproblem.");
		return 1;
	}
	if(step_status == A4SQP_CORE_STEP_LINE_SEARCH_ERROR){
		sys->status.converged = FALSE;
		sys->status.diverged = TRUE;
		asc_a4sqp_report_qp(sys);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,
			"A4SQP line search failed to find a merit-improving step (merit_before=%g, merit_after=%g, predicted_reduction=%g, rho=%g, step=%g, viol_max=%g).",
			sys->last_merit_before,
			sys->last_merit_after,
			sys->last_predicted_reduction,
			sys->last_trust_ratio,
			sys->last_step_norm,
			sys->last_violation_max
		);
		return 1;
	}
	if(step_status != A4SQP_CORE_STEP_ACCEPTED){
		sys->status.converged = FALSE;
		sys->status.diverged = TRUE;
		asc_a4sqp_report_qp(sys);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"A4SQP HiGHS QP subproblem did not solve to optimality.");
		return 1;
	}

	++sys->status.iteration;
	asc_a4sqp_report_qp(sys);
	asc_a4sqp_report_iteration(sys);
	asc_a4sqp_report_view(sys);
	if(a4sqp_has_converged(sys)){
		sys->status.converged = TRUE;
		sys->status.diverged = FALSE;
		sys->status.iteration_limit_exceeded = FALSE;
		sys->status.ready_to_solve = FALSE;
	}else if(sys->status.iteration >= SLV_PARAM_INT(&sys->params,A4SQP_PARAM_MAX_ITER)){
		sys->status.converged = FALSE;
		sys->status.diverged = FALSE;
		sys->status.iteration_limit_exceeded = TRUE;
		sys->status.ready_to_solve = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,
			"A4SQP reached the maximum iteration count before satisfying convergence tests."
		);
	}else{
		sys->status.converged = FALSE;
		sys->status.diverged = FALSE;
		sys->status.iteration_limit_exceeded = FALSE;
		sys->status.ready_to_solve = TRUE;
	}
	return 0;
}

static int a4sqp_solve(slv_system_t server, SlvClientToken asys){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;

	if(sys == NULL){
		return 1;
	}
	if(!sys->status.ready_to_solve){
		if(a4sqp_presolve(server,asys)){
			return 1;
		}
	}

	while(sys->status.ready_to_solve){
		if(a4sqp_iterate(server,asys)){
			sys->status.ready_to_solve = FALSE;
			return 1;
		}
	}
	return 0;
}

static int a4sqp_resolve(slv_system_t server, SlvClientToken asys){
	return a4sqp_solve(server,asys);
}

static void a4sqp_dumpinternals(slv_system_t server, SlvClientToken asys, int level){
	struct A4SqpSystem *sys = (struct A4SqpSystem *)asys;
	(void)server;
	if(sys == NULL || level <= 0){
		return;
	}
	asc_a4sqp_report_view(sys);
}

static const SlvFunctionsT a4sqp_internals = {
	A4SQP_SOLVER_NUMBER,
	A4SQP_SOLVER_NAME,
	a4sqp_create,
	a4sqp_destroy,
	a4sqp_eligible,
	a4sqp_get_default_parameters,
	a4sqp_get_parameters,
	a4sqp_set_parameters,
	a4sqp_get_status,
	a4sqp_solve,
	a4sqp_presolve,
	a4sqp_iterate,
	a4sqp_resolve,
	NULL,
	NULL,
	a4sqp_dumpinternals
};

ASC_EXPORT int a4sqp_register(void){
	return solver_register(&a4sqp_internals);
}
