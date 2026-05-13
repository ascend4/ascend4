/*
 * IPOPT-like C interface for callback-based A4SQP solves.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_c.h"

#include "a4sqp_core.h"
#include "a4sqp_hessian.h"
#include "a4sqp_qp_highs.h"
#include "a4sqp_trust.h"
#include "a4sqp_view.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define A4SQP_C_DEFAULT_LOWER_INF (-1e19)
#define A4SQP_C_DEFAULT_UPPER_INF (1e19)

struct A4SqpCOptions {
	int max_iter;
	int max_backtrack;
	int trust_qp_retries;
	int verbosity;
	double feas_tol;
	double step_tol;
	double acceptable_tol;
	int acceptable_iter;
	double merit_tol;
	double armijo_coeff;
	double elastic_penalty;
	int filter_accept;
	double filter_margin;
	int trust_unconstrained;
	int kkt_convergence;
	int restoration;
	int restoration_trigger_iter;
	double restoration_improve;
	double restoration_margin;
	double trust_radius_init;
	double trust_radius_min;
	double trust_radius_max;
	double trust_shrink;
	double trust_grow;
	double trust_accept;
	double trust_good;
	double hess_reg;
	double lower_inf;
	double upper_inf;
	char hessian[32];
	char scaleopt[32];
};

struct A4SqpProblemInfo {
	A4SqpIndex n;
	A4SqpIndex m;
	A4SqpIndex nele_jac;
	A4SqpIndex nele_hess;
	A4SqpIndex index_style;
	A4SqpNumber *x_l;
	A4SqpNumber *x_u;
	A4SqpNumber *g_l;
	A4SqpNumber *g_u;
	A4SqpNumber *x_scale;
	A4SqpNumber *g_scale;
	A4SqpNumber obj_scaling;
	A4SqpEvalFCB eval_f;
	A4SqpEvalGCB eval_g;
	A4SqpEvalGradFCB eval_grad_f;
	A4SqpEvalJacGCB eval_jac_g;
	A4SqpEvalHCB eval_h;
	A4SqpIntermediateCB intermediate_cb;
	struct A4SqpCOptions opt;
	struct A4SqpSolveStats stats;
};

struct A4SqpCSolve {
	A4SqpProblem problem;
	A4SqpUserDataPtr user_data;
	struct A4SqpView view;
	struct A4SqpQp qp;
	struct A4SqpDenseHessian hess;
	double *lambda;
	double trust_radius;
	int has_objective;
	int callback_error;
	int last_ls_trials;
	double last_merit_before;
	double last_merit_after;
	double last_model_merit_after;
	double last_predicted_reduction;
	double last_linearized_violation;
	double last_step_norm;
	double last_alpha;
	double last_trust_ratio;
	double last_elastic_max;
	int accepted_step_count;
	int acceptable_count;
	struct A4SqpCoreRestorationState restoration_state;
};

static int a4sqp_c_streq(const char *a, const char *b){
	if(a == NULL || b == NULL){
		return 0;
	}
	return strcmp(a,b) == 0;
}

static void a4sqp_c_trust_options(
	const struct A4SqpCOptions *copt,
	struct A4SqpTrustOptions *opt
){
	if(copt == NULL || opt == NULL){
		return;
	}
	opt->radius_init = copt->trust_radius_init;
	opt->radius_min = copt->trust_radius_min;
	opt->radius_max = copt->trust_radius_max;
	opt->shrink = copt->trust_shrink;
	opt->grow = copt->trust_grow;
	opt->good_ratio = copt->trust_good;
}

static void a4sqp_c_default_options(struct A4SqpCOptions *opt){
	if(opt == NULL){
		return;
	}
	memset(opt,0,sizeof(*opt));
	opt->max_iter = 200;
	opt->max_backtrack = 20;
	opt->trust_qp_retries = 5;
	opt->verbosity = 0;
	opt->feas_tol = 1e-7;
	opt->step_tol = 1e-7;
	opt->acceptable_tol = 1e-5;
	opt->acceptable_iter = 0;
	opt->merit_tol = 1e-10;
	opt->armijo_coeff = 1e-4;
	opt->elastic_penalty = A4SQP_QP_DEFAULT_ELASTIC_PENALTY;
	opt->filter_accept = 0;
	opt->filter_margin = 1e-4;
	opt->trust_unconstrained = 0;
	opt->kkt_convergence = 0;
	opt->restoration = 0;
	opt->restoration_trigger_iter = 3;
	opt->restoration_improve = 1e-3;
	opt->restoration_margin = 1e-4;
	opt->trust_radius_init = 1.0;
	opt->trust_radius_min = 1e-6;
	opt->trust_radius_max = 100.0;
	opt->trust_shrink = 0.25;
	opt->trust_grow = 2.0;
	opt->trust_accept = 0.1;
	opt->trust_good = 0.75;
	opt->hess_reg = 1e-8;
	opt->lower_inf = A4SQP_C_DEFAULT_LOWER_INF;
	opt->upper_inf = A4SQP_C_DEFAULT_UPPER_INF;
	strcpy(opt->hessian,"BFGS");
	strcpy(opt->scaleopt,"NONE");
}

static double a4sqp_c_map_bound(double value, double lower_inf, double upper_inf){
	if(value <= lower_inf){
		return A4SQP_NO_LOWER_BOUND;
	}
	if(value >= upper_inf){
		return A4SQP_NO_UPPER_BOUND;
	}
	return value;
}

static int a4sqp_c_is_lower_inf(double value){
	return value <= A4SQP_NO_LOWER_BOUND / 10.0;
}

static int a4sqp_c_is_upper_inf(double value){
	return value >= A4SQP_NO_UPPER_BOUND / 10.0;
}

static double a4sqp_c_safe_scale(double value){
	if(!isfinite(value) || value == 0.0){
		return 1.0;
	}
	if(value < 0.0){
		value = -value;
	}
	if(value < 1e-300){
		return 1.0;
	}
	return value;
}

static int a4sqp_c_alloc_array(double **ptr, int n){
	*ptr = NULL;
	if(n <= 0){
		return 0;
	}
	*ptr = A4SQP_NEW_ARRAY_OR_NULL(double,n);
	return *ptr == NULL ? 1 : 0;
}

static int a4sqp_c_alloc_problem_arrays(struct A4SqpProblemInfo *p){
	if(a4sqp_c_alloc_array(&p->x_l,p->n)
		|| a4sqp_c_alloc_array(&p->x_u,p->n)
		|| a4sqp_c_alloc_array(&p->g_l,p->m)
		|| a4sqp_c_alloc_array(&p->g_u,p->m)
	){
		return 1;
	}
	return 0;
}

A4SqpProblem CreateA4SqpProblem(
	A4SqpIndex n,
	A4SqpNumber *x_L,
	A4SqpNumber *x_U,
	A4SqpIndex m,
	A4SqpNumber *g_L,
	A4SqpNumber *g_U,
	A4SqpIndex nele_jac,
	A4SqpIndex nele_hess,
	A4SqpIndex index_style,
	A4SqpEvalFCB eval_f,
	A4SqpEvalGCB eval_g,
	A4SqpEvalGradFCB eval_grad_f,
	A4SqpEvalJacGCB eval_jac_g,
	A4SqpEvalHCB eval_h
){
	struct A4SqpProblemInfo *p;
	int i;
	if(n < 0 || m < 0 || nele_jac < 0 || nele_hess < 0){
		return NULL;
	}
	if(index_style != 0 && index_style != 1){
		return NULL;
	}
	if(n > 0 && (x_L == NULL || x_U == NULL)){
		return NULL;
	}
	if(m > 0 && (g_L == NULL || g_U == NULL || eval_g == NULL || eval_jac_g == NULL)){
		return NULL;
	}
	if(eval_f != NULL && eval_grad_f == NULL){
		return NULL;
	}
	p = A4SQP_NEW_CLEAR(struct A4SqpProblemInfo);
	if(p == NULL){
		return NULL;
	}
	p->n = n;
	p->m = m;
	p->nele_jac = nele_jac;
	p->nele_hess = nele_hess;
	p->index_style = index_style;
	p->eval_f = eval_f;
	p->eval_g = eval_g;
	p->eval_grad_f = eval_grad_f;
	p->eval_jac_g = eval_jac_g;
	p->eval_h = eval_h;
	p->obj_scaling = 1.0;
	a4sqp_c_default_options(&p->opt);
	if(a4sqp_c_alloc_problem_arrays(p)){
		FreeA4SqpProblem(p);
		return NULL;
	}
	for(i = 0; i < n; ++i){
		p->x_l[i] = x_L[i];
		p->x_u[i] = x_U[i];
	}
	for(i = 0; i < m; ++i){
		p->g_l[i] = g_L[i];
		p->g_u[i] = g_U[i];
	}
	return p;
}

void FreeA4SqpProblem(A4SqpProblem problem){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	if(p == NULL){
		return;
	}
	A4SQP_FREE(p->x_l);
	A4SQP_FREE(p->x_u);
	A4SQP_FREE(p->g_l);
	A4SQP_FREE(p->g_u);
	A4SQP_FREE(p->x_scale);
	A4SQP_FREE(p->g_scale);
	A4SQP_FREE(p);
}

A4SqpBool AddA4SqpStrOption(A4SqpProblem problem, char *keyword, char *val){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	if(p == NULL || keyword == NULL || val == NULL){
		return A4SQP_FALSE;
	}
	if(a4sqp_c_streq(keyword,"hessian") || a4sqp_c_streq(keyword,"hessian_approximation")){
		if(a4sqp_c_streq(val,"BFGS") || a4sqp_c_streq(val,"limited-memory")){
			strcpy(p->opt.hessian,"BFGS");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"AUTO")){
			strcpy(p->opt.hessian,p->eval_h != NULL ? "EXACT_LAGRANGIAN" : "BFGS");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"exact") || a4sqp_c_streq(val,"EXACT_LAGRANGIAN")){
			if(p->eval_h == NULL){
				return A4SQP_FALSE;
			}
			strcpy(p->opt.hessian,"EXACT_LAGRANGIAN");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"EXACT_OBJ")){
			if(p->eval_h == NULL){
				return A4SQP_FALSE;
			}
			strcpy(p->opt.hessian,"EXACT_OBJ");
			return A4SQP_TRUE;
		}
		return A4SQP_FALSE;
	}
	if(a4sqp_c_streq(keyword,"scaleopt")){
		if(a4sqp_c_streq(val,"NONE")){
			strcpy(p->opt.scaleopt,"NONE");
			return A4SQP_TRUE;
		}
		return A4SQP_FALSE;
	}
	return A4SQP_FALSE;
}

A4SqpBool AddA4SqpNumOption(A4SqpProblem problem, char *keyword, A4SqpNumber val){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	if(p == NULL || keyword == NULL || !isfinite(val)){
		return A4SQP_FALSE;
	}
	if(a4sqp_c_streq(keyword,"tol")){
		p->opt.feas_tol = val;
		p->opt.step_tol = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"constr_viol_tol") || a4sqp_c_streq(keyword,"feas_tol")){
		p->opt.feas_tol = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"step_tol")){
		p->opt.step_tol = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"acceptable_tol")){
		p->opt.acceptable_tol = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"merit_tol")){
		p->opt.merit_tol = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"armijo_coeff")){
		p->opt.armijo_coeff = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"elastic_penalty")){
		p->opt.elastic_penalty = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"filter_margin")){
		if(val < 0.0 || val >= 1.0){
			return A4SQP_FALSE;
		}
		p->opt.filter_margin = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"restoration_improve")){
		if(val < 0.0 || val >= 1.0){
			return A4SQP_FALSE;
		}
		p->opt.restoration_improve = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"restoration_margin")){
		if(val < 0.0 || val >= 1.0){
			return A4SQP_FALSE;
		}
		p->opt.restoration_margin = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_radius_init")){
		p->opt.trust_radius_init = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_radius_min")){
		p->opt.trust_radius_min = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_radius_max")){
		p->opt.trust_radius_max = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_shrink")){
		p->opt.trust_shrink = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_grow")){
		p->opt.trust_grow = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_accept")){
		p->opt.trust_accept = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_good")){
		p->opt.trust_good = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"hess_reg")){
		if(val < 0.0){
			return A4SQP_FALSE;
		}
		p->opt.hess_reg = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"nlp_lower_bound_inf")){
		p->opt.lower_inf = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"nlp_upper_bound_inf")){
		p->opt.upper_inf = val;
		return A4SQP_TRUE;
	}
	return A4SQP_FALSE;
}

A4SqpBool AddA4SqpIntOption(A4SqpProblem problem, char *keyword, A4SqpInt val){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	if(p == NULL || keyword == NULL){
		return A4SQP_FALSE;
	}
	if(a4sqp_c_streq(keyword,"max_iter")){
		p->opt.max_iter = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"max_backtrack")){
		p->opt.max_backtrack = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"acceptable_iter")){
		p->opt.acceptable_iter = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_qp_retries")){
		p->opt.trust_qp_retries = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"filter_accept")){
		p->opt.filter_accept = val != 0;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_unconstrained")){
		p->opt.trust_unconstrained = val != 0;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"kkt_convergence")){
		p->opt.kkt_convergence = val != 0;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"restoration")){
		p->opt.restoration = val != 0;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"restoration_trigger_iter")){
		if(val < 0){
			return A4SQP_FALSE;
		}
		p->opt.restoration_trigger_iter = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"print_level") || a4sqp_c_streq(keyword,"verbosity")){
		p->opt.verbosity = val;
		return A4SQP_TRUE;
	}
	return A4SQP_FALSE;
}

A4SqpBool OpenA4SqpOutputFile(A4SqpProblem problem, char *file_name, A4SqpInt print_level){
	(void)problem;
	(void)file_name;
	(void)print_level;
	return A4SQP_FALSE;
}

A4SqpBool SetA4SqpProblemScaling(
	A4SqpProblem problem,
	A4SqpNumber obj_scaling,
	A4SqpNumber *x_scaling,
	A4SqpNumber *g_scaling
){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	int i;
	if(p == NULL || !isfinite(obj_scaling)){
		return A4SQP_FALSE;
	}
	p->obj_scaling = obj_scaling;
	A4SQP_FREE(p->x_scale);
	A4SQP_FREE(p->g_scale);
	p->x_scale = NULL;
	p->g_scale = NULL;
	if(x_scaling != NULL && p->n > 0){
		p->x_scale = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
		if(p->x_scale == NULL){
			return A4SQP_FALSE;
		}
		for(i = 0; i < p->n; ++i){
			p->x_scale[i] = a4sqp_c_safe_scale(x_scaling[i]);
		}
	}
	if(g_scaling != NULL && p->m > 0){
		p->g_scale = A4SQP_NEW_ARRAY_OR_NULL(double,p->m);
		if(p->g_scale == NULL){
			return A4SQP_FALSE;
		}
		for(i = 0; i < p->m; ++i){
			p->g_scale[i] = a4sqp_c_safe_scale(g_scaling[i]);
		}
	}
	return A4SQP_TRUE;
}

A4SqpBool SetA4SqpIntermediateCallback(A4SqpProblem problem, A4SqpIntermediateCB intermediate_cb){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	if(p == NULL){
		return A4SQP_FALSE;
	}
	p->intermediate_cb = intermediate_cb;
	return A4SQP_TRUE;
}

static int a4sqp_c_view_alloc(struct A4SqpView *view){
	if(view->n_var > 0){
		view->var_value = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->var_lower = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->var_upper = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->var_nominal = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->var_fixed = A4SQP_NEW_ARRAY_OR_NULL(uint32,view->n_var);
		view->var_scale = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->scaled_var_value = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->scaled_var_lower = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->scaled_var_upper = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->obj_gradient = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		view->scaled_obj_gradient = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_var);
		if(view->var_value == NULL
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
		view->rel_kind = A4SQP_NEW_ARRAY_OR_NULL(enum A4SqpRelKind,view->n_rel);
		view->rel_residual = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->rel_lower = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->rel_upper = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->rel_nominal = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->rel_scale = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->scaled_rel_residual = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->scaled_rel_lower = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->scaled_rel_upper = A4SQP_NEW_ARRAY_OR_NULL(double,view->n_rel);
		view->jac_row_start = A4SQP_NEW_ARRAY_OR_NULL(int32,view->n_rel + 1);
		if(view->rel_kind == NULL
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

static int a4sqp_c_build_view(struct A4SqpCSolve *solve, const double *x, int new_x){
	struct A4SqpProblemInfo *p = solve->problem;
	struct A4SqpView *view = &solve->view;
	int i;
	int k;
	A4SqpIndex *irow = NULL;
	A4SqpIndex *jcol = NULL;
	double *jac = NULL;

	a4sqp_view_destroy(view);
	view->n_var = p->n;
	view->n_rel = p->m;
	view->jac_nnz = p->nele_jac;
	if(a4sqp_c_view_alloc(view)){
		return 1;
	}
	for(i = 0; i < p->n; ++i){
		double scale = p->x_scale != NULL ? p->x_scale[i] : 1.0;
		double lower = a4sqp_c_map_bound(p->x_l[i],p->opt.lower_inf,p->opt.upper_inf);
		double upper = a4sqp_c_map_bound(p->x_u[i],p->opt.lower_inf,p->opt.upper_inf);
		view->var_value[i] = x[i];
		view->var_lower[i] = lower;
		view->var_upper[i] = upper;
		view->var_nominal[i] = scale;
		view->var_fixed[i] = 0;
		view->var_scale[i] = scale;
		view->scaled_var_value[i] = x[i] / scale;
		view->scaled_var_lower[i] = a4sqp_c_is_lower_inf(lower) ? A4SQP_NO_LOWER_BOUND : lower / scale;
		view->scaled_var_upper[i] = a4sqp_c_is_upper_inf(upper) ? A4SQP_NO_UPPER_BOUND : upper / scale;
		view->obj_gradient[i] = 0.0;
		view->scaled_obj_gradient[i] = 0.0;
	}
	if(solve->has_objective){
		double obj = 0.0;
		if(!p->eval_f(p->n,(double *)x,new_x,&obj,solve->user_data)){
			++view->obj_calc_errors;
			solve->callback_error = 1;
			return 0;
		}
		if(!isfinite(obj)){
			++view->obj_calc_errors;
			solve->callback_error = 1;
			return 0;
		}
		view->obj_value = p->obj_scaling * obj;
		if(!isfinite(view->obj_value)){
			++view->obj_calc_errors;
			solve->callback_error = 1;
			return 0;
		}
		if(p->n > 0){
			if(!p->eval_grad_f(p->n,(double *)x,A4SQP_FALSE,view->obj_gradient,solve->user_data)){
				++view->obj_derivative_errors;
				solve->callback_error = 1;
				return 0;
			}
			for(i = 0; i < p->n; ++i){
				if(!isfinite(view->obj_gradient[i])){
					++view->obj_derivative_errors;
					solve->callback_error = 1;
					return 0;
				}
				view->obj_gradient[i] *= p->obj_scaling;
				view->scaled_obj_gradient[i] = view->obj_gradient[i] * view->var_scale[i];
				if(!isfinite(view->scaled_obj_gradient[i])){
					++view->obj_derivative_errors;
					solve->callback_error = 1;
					return 0;
				}
			}
		}
	}else{
		view->obj_value = 0.0;
	}
	if(p->m > 0){
		if(!p->eval_g(p->n,(double *)x,new_x,p->m,view->rel_residual,solve->user_data)){
			++view->calc_errors;
			solve->callback_error = 1;
			return 0;
		}
		for(i = 0; i < p->m; ++i){
			double rscale = p->g_scale != NULL ? p->g_scale[i] : 1.0;
			double lower = a4sqp_c_map_bound(p->g_l[i],p->opt.lower_inf,p->opt.upper_inf);
			double upper = a4sqp_c_map_bound(p->g_u[i],p->opt.lower_inf,p->opt.upper_inf);
			if(!isfinite(view->rel_residual[i])){
				++view->calc_errors;
				solve->callback_error = 1;
				return 0;
			}
			view->rel_kind[i] = (
				!a4sqp_c_is_lower_inf(lower)
				&& !a4sqp_c_is_upper_inf(upper)
				&& fabs(lower - upper) <= p->opt.feas_tol
			) ? A4SQP_REL_KIND_EQUALITY : A4SQP_REL_KIND_INEQUALITY;
			view->rel_lower[i] = lower;
			view->rel_upper[i] = upper;
			view->rel_nominal[i] = 1.0;
			view->rel_scale[i] = rscale;
			view->scaled_rel_residual[i] = view->rel_residual[i] * rscale;
			view->scaled_rel_lower[i] = a4sqp_c_is_lower_inf(lower) ? A4SQP_NO_LOWER_BOUND : lower * rscale;
			view->scaled_rel_upper[i] = a4sqp_c_is_upper_inf(upper) ? A4SQP_NO_UPPER_BOUND : upper * rscale;
		}
		view->jac_col_index = A4SQP_NEW_ARRAY_OR_NULL(int32,p->nele_jac > 0 ? p->nele_jac : 1);
		view->jac_value = A4SQP_NEW_ARRAY_OR_NULL(double,p->nele_jac > 0 ? p->nele_jac : 1);
		view->scaled_jac_value = A4SQP_NEW_ARRAY_OR_NULL(double,p->nele_jac > 0 ? p->nele_jac : 1);
		irow = A4SQP_NEW_ARRAY_OR_NULL(A4SqpIndex,p->nele_jac > 0 ? p->nele_jac : 1);
		jcol = A4SQP_NEW_ARRAY_OR_NULL(A4SqpIndex,p->nele_jac > 0 ? p->nele_jac : 1);
		jac = A4SQP_NEW_ARRAY_OR_NULL(double,p->nele_jac > 0 ? p->nele_jac : 1);
		if(view->jac_col_index == NULL
			|| view->jac_value == NULL || view->scaled_jac_value == NULL
			|| irow == NULL || jcol == NULL || jac == NULL
		){
			A4SQP_FREE(irow);
			A4SQP_FREE(jcol);
			A4SQP_FREE(jac);
			return 1;
		}
		if(!p->eval_jac_g(p->n,(double *)x,new_x,p->m,p->nele_jac,irow,jcol,NULL,solve->user_data)
			|| !p->eval_jac_g(p->n,(double *)x,A4SQP_FALSE,p->m,p->nele_jac,irow,jcol,jac,solve->user_data)
		){
			++view->derivative_errors;
			solve->callback_error = 1;
			A4SQP_FREE(irow);
			A4SQP_FREE(jcol);
			A4SQP_FREE(jac);
			return 0;
		}
		for(i = 0; i <= p->m; ++i){
			view->jac_row_start[i] = 0;
		}
		for(k = 0; k < p->nele_jac; ++k){
			int row = irow[k] - (p->index_style == 1 ? 1 : 0);
			if(row >= 0 && row < p->m){
				++view->jac_row_start[row + 1];
			}
		}
		for(i = 0; i < p->m; ++i){
			view->jac_row_start[i + 1] += view->jac_row_start[i];
		}
		{
			int32 *next = A4SQP_NEW_ARRAY_OR_NULL(int32,p->m > 0 ? p->m : 1);
			if(next == NULL){
				A4SQP_FREE(irow);
				A4SQP_FREE(jcol);
				A4SQP_FREE(jac);
				return 1;
			}
			for(i = 0; i < p->m; ++i){
				next[i] = view->jac_row_start[i];
			}
		for(k = 0; k < p->nele_jac; ++k){
			int row = irow[k] - (p->index_style == 1 ? 1 : 0);
			int col = jcol[k] - (p->index_style == 1 ? 1 : 0);
			if(!isfinite(jac[k])){
				++view->derivative_errors;
				solve->callback_error = 1;
				A4SQP_FREE(next);
				A4SQP_FREE(irow);
				A4SQP_FREE(jcol);
				A4SQP_FREE(jac);
				return 0;
			}
			if(row >= 0 && row < p->m && col >= 0 && col < p->n){
				int pos = next[row]++;
				view->jac_col_index[pos] = col;
				view->jac_value[pos] = jac[k];
					view->scaled_jac_value[pos] = view->rel_scale[row] * jac[k] * view->var_scale[col];
				}
			}
			A4SQP_FREE(next);
		}
		A4SQP_FREE(irow);
		A4SQP_FREE(jcol);
		A4SQP_FREE(jac);
	}
	return 0;
}

static double a4sqp_c_view_violation(const struct A4SqpView *view, double *max_violation){
	return a4sqp_core_view_violation(view,max_violation,NULL);
}

static double a4sqp_c_projected_gradient_inf(struct A4SqpCSolve *solve){
	return a4sqp_core_projected_gradient_inf(
		&solve->view,
		solve->has_objective,
		solve->problem->opt.feas_tol
	);
}

static void a4sqp_c_update_kkt_stats(struct A4SqpCSolve *solve){
	struct A4SqpKktResidual residual;
	if(solve == NULL || solve->problem == NULL){
		return;
	}
	a4sqp_core_kkt_error(
		&solve->view,
		solve->has_objective,
		solve->lambda,
		solve->problem->opt.feas_tol,
		&residual
	);
	solve->problem->stats.kkt_error = residual.kkt_error;
	solve->problem->stats.dual_infeasibility_inf = residual.dual_inf;
	solve->problem->stats.complementarity_inf = residual.complementarity_inf;
	solve->problem->stats.kkt_lambda_sign = residual.lambda_sign;
}

static int a4sqp_c_hess_reset_identity(struct A4SqpCSolve *solve, double diag){
	return a4sqp_dense_hessian_reset_identity(&solve->hess,solve->problem->n,diag);
}

static double a4sqp_c_hess_regularize_psd(struct A4SqpCSolve *solve){
	double min_diag;
	if(solve == NULL){
		return 0.0;
	}
	min_diag = solve->problem->opt.hess_reg;
	return a4sqp_dense_hessian_regularize_psd(&solve->hess,min_diag);
}

static int a4sqp_c_uses_exact_hessian(const struct A4SqpProblemInfo *p){
	return p != NULL
		&& p->eval_h != NULL
		&& (
			a4sqp_c_streq(p->opt.hessian,"EXACT_OBJ")
			|| a4sqp_c_streq(p->opt.hessian,"EXACT_LAGRANGIAN")
		);
}

static int a4sqp_c_hess_update_exact(struct A4SqpCSolve *solve, const double *x){
	struct A4SqpProblemInfo *p = solve->problem;
	A4SqpIndex *irow = NULL;
	A4SqpIndex *jcol = NULL;
	double *values = NULL;
	double *lambda = NULL;
	double obj_factor;
	int k;
	int n = p->n;
	if(!a4sqp_c_uses_exact_hessian(p)){
		return 0;
	}
	if(p->nele_hess <= 0){
		return a4sqp_c_hess_reset_identity(solve,1.0);
	}
	if(solve->hess.dense == NULL || solve->hess.n != n){
		if(a4sqp_c_hess_reset_identity(solve,1.0)){
			return 1;
		}
	}
	memset(solve->hess.dense,0,(size_t)n * (size_t)n * sizeof(*solve->hess.dense));
	irow = A4SQP_NEW_ARRAY_OR_NULL(A4SqpIndex,p->nele_hess);
	jcol = A4SQP_NEW_ARRAY_OR_NULL(A4SqpIndex,p->nele_hess);
	values = A4SQP_NEW_ARRAY_OR_NULL(double,p->nele_hess);
	lambda = A4SQP_NEW_ARRAY_CLEAR(double,p->m > 0 ? p->m : 1);
	if(irow == NULL || jcol == NULL || values == NULL || lambda == NULL){
		A4SQP_FREE(irow);
		A4SQP_FREE(jcol);
		A4SQP_FREE(values);
		A4SQP_FREE(lambda);
		return 1;
	}
	obj_factor = (solve->has_objective && !a4sqp_c_streq(p->opt.hessian,"EXACT_LAGRANGIAN")) ? p->obj_scaling : p->obj_scaling;
	if(a4sqp_c_streq(p->opt.hessian,"EXACT_LAGRANGIAN")){
		for(k = 0; k < p->m; ++k){
			double row_dual = solve->lambda != NULL ? solve->lambda[k] : 0.0;
			double rel_scale = solve->view.rel_scale != NULL ? solve->view.rel_scale[k] : 1.0;
			lambda[k] = row_dual * rel_scale;
		}
	}
	if(!p->eval_h(
		p->n,
		(double *)x,
		A4SQP_TRUE,
		obj_factor,
		p->m,
		lambda,
		A4SQP_TRUE,
		p->nele_hess,
		irow,
		jcol,
		NULL,
		solve->user_data
	) || !p->eval_h(
		p->n,
		(double *)x,
		A4SQP_FALSE,
		obj_factor,
		p->m,
		lambda,
		A4SQP_FALSE,
		p->nele_hess,
		irow,
		jcol,
		values,
		solve->user_data
	)){
		A4SQP_FREE(irow);
		A4SQP_FREE(jcol);
		A4SQP_FREE(values);
		A4SQP_FREE(lambda);
		solve->callback_error = 1;
		return 0;
	}
	for(k = 0; k < p->nele_hess; ++k){
		int row = irow[k] - (p->index_style == 1 ? 1 : 0);
		int col = jcol[k] - (p->index_style == 1 ? 1 : 0);
		double value = values[k];
		if(row < 0 || row >= n || col < 0 || col >= n){
			continue;
		}
		if(!isfinite(value)){
			solve->callback_error = 1;
			continue;
		}
		value *= solve->view.var_scale[row] * solve->view.var_scale[col];
		solve->hess.dense[row * n + col] += value;
		if(row != col){
			solve->hess.dense[col * n + row] += value;
		}
	}
	p->stats.regularization_size = a4sqp_c_hess_regularize_psd(solve);
	A4SQP_FREE(irow);
	A4SQP_FREE(jcol);
	A4SQP_FREE(values);
	A4SQP_FREE(lambda);
	return 0;
}

static void a4sqp_c_hess_bfgs_update(
	struct A4SqpCSolve *solve,
	const double *old_x,
	const double *old_grad
){
	if(solve == NULL || solve->problem == NULL){
		return;
	}
	(void)a4sqp_dense_hessian_bfgs_update(
		&solve->hess,
		solve->view.scaled_var_value,
		solve->view.scaled_obj_gradient,
		old_x,
		old_grad,
		solve->problem->opt.hess_reg
	);
	solve->problem->stats.regularization_size = solve->hess.last_reg;
}

struct A4SqpCLineSearchCtx {
	struct A4SqpCSolve *solve;
};

static int a4sqp_c_ls_evaluate(void *ctx, const real64 *x, struct A4SqpView *view){
	struct A4SqpCLineSearchCtx *ls = (struct A4SqpCLineSearchCtx *)ctx;
	if(ls == NULL || ls->solve == NULL || x == NULL || view == NULL || view != &ls->solve->view){
		return 1;
	}
	if(a4sqp_c_build_view(ls->solve,(double *)x,A4SQP_TRUE)){
		return 1;
	}
	if(ls->solve->callback_error){
		ls->solve->callback_error = 0;
		return 1;
	}
	return 0;
}

static void a4sqp_c_ls_accepted(
	void *ctx,
	const real64 *old_scaled_x,
	const real64 *old_scaled_grad,
	int restoration
){
	struct A4SqpCLineSearchCtx *ls = (struct A4SqpCLineSearchCtx *)ctx;
	if(ls == NULL || ls->solve == NULL || restoration || a4sqp_c_uses_exact_hessian(ls->solve->problem)){
		return;
	}
	a4sqp_c_hess_bfgs_update(ls->solve,old_scaled_x,old_scaled_grad);
}

static void a4sqp_c_record_line_search_result(
	struct A4SqpCSolve *solve,
	const struct A4SqpLineSearchResult *result
){
	struct A4SqpTrustOptions trust_opt;
	if(solve == NULL || solve->problem == NULL || result == NULL){
		return;
	}
	solve->last_merit_before = result->merit_before;
	solve->last_merit_after = result->merit_after;
	solve->last_model_merit_after = result->model_merit_after;
	solve->last_predicted_reduction = result->predicted_reduction;
	solve->last_linearized_violation = a4sqp_core_qp_elastic_sum(&solve->qp);
	solve->last_alpha = result->alpha;
	solve->last_step_norm = result->step_norm;
	solve->last_trust_ratio = result->trust_ratio;
	solve->last_ls_trials = result->trials;
	if(result->accepted && result->alpha > 0.0){
		a4sqp_c_trust_options(&solve->problem->opt,&trust_opt);
		a4sqp_trust_grow_if_good(
			&solve->trust_radius,
			&trust_opt,
			result->alpha,
			result->scaled_step_inf,
			result->trust_ratio
		);
	}
}

static int a4sqp_c_has_converged(struct A4SqpCSolve *solve){
	double maxvio = 0.0;
	double pg = 0.0;
	struct A4SqpConvergencePolicy policy;
	double opt_tol;
	policy.feas_tol = solve->problem->opt.feas_tol;
	policy.step_tol = solve->problem->opt.step_tol;
	policy.last_step_norm = solve->last_step_norm;
	policy.has_objective = solve->has_objective;
	policy.constrained_objective_allows_small_step = solve->accepted_step_count > 0;
	opt_tol = policy.step_tol > policy.feas_tol ? policy.step_tol : policy.feas_tol;
	if(solve->problem->opt.kkt_convergence && solve->has_objective){
		(void)a4sqp_c_view_violation(&solve->view,&maxvio);
		pg = a4sqp_c_projected_gradient_inf(solve);
		solve->problem->stats.max_constraint_violation = maxvio;
		solve->problem->stats.projected_gradient_inf = pg;
		a4sqp_c_update_kkt_stats(solve);
		return maxvio <= policy.feas_tol && solve->problem->stats.kkt_error <= opt_tol;
	}
	if(a4sqp_core_has_converged(&solve->view,&policy,&maxvio,&pg,NULL)){
		solve->problem->stats.max_constraint_violation = maxvio;
		solve->problem->stats.projected_gradient_inf = pg;
		a4sqp_c_update_kkt_stats(solve);
		return 1;
	}
	solve->problem->stats.max_constraint_violation = maxvio;
	solve->problem->stats.projected_gradient_inf = pg;
	a4sqp_c_update_kkt_stats(solve);
	return 0;
}

static int a4sqp_c_acceptably_converged(struct A4SqpCSolve *solve){
	double maxvio = 0.0;
	double pg = 0.0;
	struct A4SqpConvergencePolicy policy;
	if(solve == NULL || solve->problem == NULL || solve->problem->opt.acceptable_iter <= 0){
		return 0;
	}
	if(solve->accepted_step_count <= 0){
		solve->acceptable_count = 0;
		return 0;
	}
	policy.feas_tol = solve->problem->opt.acceptable_tol;
	policy.step_tol = solve->problem->opt.acceptable_tol;
	policy.last_step_norm = solve->last_step_norm;
	policy.has_objective = solve->has_objective;
	policy.constrained_objective_allows_small_step = 1;
	if(solve->problem->opt.kkt_convergence && solve->has_objective){
		(void)a4sqp_c_view_violation(&solve->view,&maxvio);
		pg = a4sqp_c_projected_gradient_inf(solve);
		solve->problem->stats.max_constraint_violation = maxvio;
		solve->problem->stats.projected_gradient_inf = pg;
		a4sqp_c_update_kkt_stats(solve);
		if(maxvio <= policy.feas_tol && solve->problem->stats.kkt_error <= policy.feas_tol){
			++solve->acceptable_count;
		}else{
			solve->acceptable_count = 0;
		}
		return solve->acceptable_count >= solve->problem->opt.acceptable_iter;
	}
	if(a4sqp_core_has_converged(&solve->view,&policy,&maxvio,&pg,NULL)){
		solve->problem->stats.max_constraint_violation = maxvio;
		solve->problem->stats.projected_gradient_inf = pg;
		a4sqp_c_update_kkt_stats(solve);
		++solve->acceptable_count;
	}else{
		solve->problem->stats.max_constraint_violation = maxvio;
		solve->problem->stats.projected_gradient_inf = pg;
		a4sqp_c_update_kkt_stats(solve);
		solve->acceptable_count = 0;
	}
	return solve->acceptable_count >= solve->problem->opt.acceptable_iter;
}

struct A4SqpCCoreStepCtx {
	struct A4SqpCSolve *solve;
	double *x;
	int hessian_memory_error;
};

static int a4sqp_c_core_prepare_hessian(void *vctx, struct A4SqpStepHessian *step_hess){
	struct A4SqpCCoreStepCtx *ctx = (struct A4SqpCCoreStepCtx *)vctx;
	struct A4SqpCSolve *solve;
	struct A4SqpProblemInfo *p;
	if(ctx == NULL || ctx->solve == NULL || step_hess == NULL){
		return 1;
	}
	solve = ctx->solve;
	p = solve->problem;
	memset(step_hess,0,sizeof(*step_hess));
	if(a4sqp_c_hess_update_exact(solve,ctx->x)){
		if(!solve->callback_error){
			ctx->hessian_memory_error = 1;
		}
		return 1;
	}
	if(solve->callback_error){
		return 1;
	}
	step_hess->n = p->n;
	step_hess->dense = solve->hess.dense;
	return 0;
}

static int a4sqp_c_core_solve_qp(void *vctx, struct A4SqpQp *qp){
	struct A4SqpCCoreStepCtx *ctx = (struct A4SqpCCoreStepCtx *)vctx;
	if(ctx == NULL || ctx->solve == NULL || ctx->solve->problem == NULL){
		return 1;
	}
	return a4sqp_qp_solve_highs(qp,ctx->solve->problem->opt.feas_tol,0) == 0 ? 0 : 1;
}

static void a4sqp_c_core_after_qp_solve(void *vctx, const struct A4SqpQp *qp){
	struct A4SqpCCoreStepCtx *ctx = (struct A4SqpCCoreStepCtx *)vctx;
	struct A4SqpCSolve *solve;
	struct A4SqpProblemInfo *p;
	int row;
	if(ctx == NULL || ctx->solve == NULL || qp == NULL){
		return;
	}
	solve = ctx->solve;
	p = solve->problem;
	for(row = 0; row < p->m; ++row){
		solve->lambda[row] = (qp->row_dual != NULL && row < qp->num_row)
			? qp->row_dual[row]
			: 0.0;
	}
	solve->last_elastic_max = a4sqp_core_qp_elastic_max(qp);
	solve->last_linearized_violation = a4sqp_core_qp_elastic_sum(qp);
}

static int a4sqp_c_core_shrink_trust(void *vctx, const char *reason){
	struct A4SqpCCoreStepCtx *ctx = (struct A4SqpCCoreStepCtx *)vctx;
	struct A4SqpTrustOptions trust_opt;
	(void)reason;
	if(ctx == NULL || ctx->solve == NULL){
		return 1;
	}
	a4sqp_c_trust_options(&ctx->solve->problem->opt,&trust_opt);
	(void)a4sqp_trust_shrink_radius(&ctx->solve->trust_radius,&trust_opt);
	return 0;
}

static enum A4SqpApplicationReturnStatus a4sqp_c_solve_impl(struct A4SqpCSolve *solve, double *x){
	struct A4SqpProblemInfo *p = solve->problem;
	struct A4SqpTrustOptions trust_opt;
	int iter;
	if(a4sqp_c_build_view(solve,x,A4SQP_TRUE)){
		return A4SqpInsufficientMemory;
	}
	if(solve->callback_error){
		return A4SqpInvalidNumberDetected;
	}
	if(a4sqp_c_hess_reset_identity(solve,1.0)){
		return A4SqpInsufficientMemory;
	}
	if(p->m > 0){
		solve->lambda = A4SQP_NEW_ARRAY_CLEAR(double,p->m);
		if(solve->lambda == NULL){
			return A4SqpInsufficientMemory;
		}
	}
	a4sqp_c_trust_options(&p->opt,&trust_opt);
	solve->trust_radius = a4sqp_trust_initial_radius(&trust_opt);
	a4sqp_core_restoration_state_init(&solve->restoration_state);
	for(iter = 0; iter < p->opt.max_iter; ++iter){
		struct A4SqpCCoreStepCtx step_ctx;
		struct A4SqpCoreStepOptions step_options;
		struct A4SqpCoreStepOps step_ops;
		struct A4SqpCoreStepStats step_stats;
		struct A4SqpCLineSearchCtx line_ctx;
		struct A4SqpVectorLineSearchOps line_ops;
		struct A4SqpLineSearchOptions line_options;
		struct A4SqpLineSearchResult line_result;
		enum A4SqpCoreStepStatus step_status;
		double maxvio = 0.0;
		if(a4sqp_c_has_converged(solve)){
			p->stats.iterations = iter;
			return A4SqpSolveSucceeded;
		}
		memset(&step_ctx,0,sizeof(step_ctx));
		memset(&step_options,0,sizeof(step_options));
		memset(&step_ops,0,sizeof(step_ops));
		memset(&line_ctx,0,sizeof(line_ctx));
		memset(&line_ops,0,sizeof(line_ops));
		memset(&line_options,0,sizeof(line_options));
		memset(&line_result,0,sizeof(line_result));
		step_ctx.solve = solve;
		step_ctx.x = x;
		line_ctx.solve = solve;
		step_options.trust_qp_retries = p->opt.trust_qp_retries;
		step_options.trust_unconstrained = p->opt.trust_unconstrained;
		step_options.elastic_penalty = p->opt.elastic_penalty;
		step_options.feas_tol = p->opt.feas_tol;
		step_options.merit_tol = p->opt.merit_tol;
		step_options.restoration.enable = p->opt.restoration;
		step_options.restoration.trigger_iter = p->opt.restoration_trigger_iter;
		step_options.restoration.improve = p->opt.restoration_improve;
		step_options.restoration.margin = p->opt.restoration_margin;
		step_options.restoration_state = &solve->restoration_state;
		line_options.max_backtrack = p->opt.max_backtrack;
		line_options.merit_tol = p->opt.merit_tol;
		line_options.feas_tol = p->opt.feas_tol;
		line_options.step_tol = p->opt.step_tol;
		line_options.armijo_coeff = p->opt.armijo_coeff;
		line_options.trust_accept = p->opt.trust_accept;
		line_options.elastic_penalty = p->opt.elastic_penalty;
		line_options.filter_accept = p->opt.filter_accept;
		line_options.filter_margin = p->opt.filter_margin;
		line_ops.evaluate = a4sqp_c_ls_evaluate;
		line_ops.accepted = a4sqp_c_ls_accepted;
		step_ops.prepare_hessian = a4sqp_c_core_prepare_hessian;
		step_ops.solve_qp = a4sqp_c_core_solve_qp;
		step_ops.after_qp_solve = a4sqp_c_core_after_qp_solve;
		step_ops.shrink_trust = a4sqp_c_core_shrink_trust;
		step_status = a4sqp_core_solve_step(
			&solve->view,
			&solve->qp,
			&solve->trust_radius,
			&step_options,
			&step_ops,
			&step_ctx,
			&line_options,
			&line_ops,
			x,
			solve->has_objective,
			&line_result,
			&step_stats
		);
		if(line_result.accepted || step_status == A4SQP_CORE_STEP_LINE_SEARCH_ERROR){
			a4sqp_c_record_line_search_result(solve,&line_result);
		}
		p->stats.qp_solves += step_stats.qp_solves;
		p->stats.qp_failures += step_stats.qp_failures;
		p->stats.line_search_failures += step_stats.line_search_failures;
		if(step_status != A4SQP_CORE_STEP_ACCEPTED){
			if(step_status == A4SQP_CORE_STEP_HESSIAN_ERROR && solve->callback_error){
				return A4SqpInvalidNumberDetected;
			}
			if(step_status == A4SQP_CORE_STEP_HESSIAN_ERROR && step_ctx.hessian_memory_error){
				return A4SqpInsufficientMemory;
			}
			return A4SqpErrorInStepComputation;
		}
		++solve->accepted_step_count;
		p->stats.iterations = iter + 1;
		p->stats.objective = solve->view.obj_value;
		p->stats.final_step_norm = solve->last_step_norm;
		p->stats.final_trust_radius = solve->trust_radius;
		p->stats.final_elastic_max = solve->last_elastic_max;
		a4sqp_c_view_violation(&solve->view,&maxvio);
		p->stats.max_constraint_violation = maxvio;
		p->stats.projected_gradient_inf = a4sqp_c_projected_gradient_inf(solve);
		a4sqp_c_update_kkt_stats(solve);
		if(p->intermediate_cb != NULL){
			if(!p->intermediate_cb(
				solve->restoration_state.active ? A4SqpRestorationPhaseMode : A4SqpRegularMode,
				p->stats.iterations,
				solve->view.obj_value,
				p->stats.max_constraint_violation,
				p->stats.projected_gradient_inf,
				0.0,
				solve->last_step_norm,
				p->stats.regularization_size,
				0.0,
				solve->last_alpha,
				solve->last_ls_trials,
				solve->user_data
			)){
				return A4SqpUserRequestedStop;
			}
		}
		if(a4sqp_c_has_converged(solve)){
			return A4SqpSolveSucceeded;
		}
		if(a4sqp_c_acceptably_converged(solve)){
			return A4SqpSolvedToAcceptableLevel;
		}
	}
	return A4SqpMaximumIterationsExceeded;
}

enum A4SqpApplicationReturnStatus A4SqpSolve(
	A4SqpProblem problem,
	A4SqpNumber *x,
	A4SqpNumber *g,
	A4SqpNumber *obj_val,
	A4SqpNumber *mult_g,
	A4SqpNumber *mult_x_L,
	A4SqpNumber *mult_x_U,
	A4SqpUserDataPtr user_data
){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	struct A4SqpCSolve solve;
	enum A4SqpApplicationReturnStatus status;
	int i;
	if(p == NULL || (p->n > 0 && x == NULL)){
		return A4SqpInvalidProblemDefinition;
	}
	memset(&p->stats,0,sizeof(p->stats));
	memset(&solve,0,sizeof(solve));
	solve.problem = p;
	solve.user_data = user_data;
	solve.has_objective = p->eval_f != NULL;
	a4sqp_dense_hessian_init(&solve.hess);
	a4sqp_view_init(&solve.view);
	a4sqp_qp_init(&solve.qp);
	status = a4sqp_c_solve_impl(&solve,x);
	if(g != NULL && p->m > 0 && solve.view.rel_residual != NULL){
		for(i = 0; i < p->m; ++i){
			g[i] = solve.view.rel_residual[i];
		}
	}
	if(obj_val != NULL){
		*obj_val = solve.has_objective ? solve.view.obj_value : 0.0;
	}
	if(mult_g != NULL){
		for(i = 0; i < p->m; ++i){
			mult_g[i] = (solve.qp.row_dual != NULL && i < solve.qp.num_row) ? solve.qp.row_dual[i] : 0.0;
		}
	}
	if(mult_x_L != NULL){
		for(i = 0; i < p->n; ++i){
			mult_x_L[i] = 0.0;
		}
	}
	if(mult_x_U != NULL){
		for(i = 0; i < p->n; ++i){
			mult_x_U[i] = 0.0;
		}
	}
	if(solve.view.n_var >= 0){
		p->stats.objective = solve.has_objective ? solve.view.obj_value : 0.0;
		p->stats.final_step_norm = solve.last_step_norm;
		p->stats.final_trust_radius = solve.trust_radius;
		p->stats.final_elastic_max = solve.last_elastic_max;
		p->stats.regularization_size = solve.hess.last_reg;
		(void)a4sqp_c_view_violation(&solve.view,&p->stats.max_constraint_violation);
		p->stats.projected_gradient_inf = a4sqp_c_projected_gradient_inf(&solve);
		a4sqp_c_update_kkt_stats(&solve);
	}
	a4sqp_dense_hessian_destroy(&solve.hess);
	A4SQP_FREE(solve.lambda);
	a4sqp_qp_destroy(&solve.qp);
	a4sqp_view_destroy(&solve.view);
	return status;
}

A4SqpBool GetA4SqpSolveStatistics(A4SqpProblem problem, struct A4SqpSolveStats *stats){
	struct A4SqpProblemInfo *p = (struct A4SqpProblemInfo *)problem;
	if(p == NULL || stats == NULL){
		return A4SQP_FALSE;
	}
	*stats = p->stats;
	return A4SQP_TRUE;
}
