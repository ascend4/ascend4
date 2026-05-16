/*
 * IPOPT-like C interface for callback-based A4SQP solves.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_c.h"

#include "a4sqp_core.h"
#include "a4sqp_hessian.h"
#include "a4sqp_qp_highs.h"
#include "a4sqp_scale.h"
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
	double elastic_penalty_growth;
	double elastic_penalty_max;
	int filter_accept;
	double filter_margin;
	int trust_unconstrained;
	int kkt_convergence;
	int restoration;
	int active_bound_restoration;
	int active_bound_release;
	int reduced_gradient_polish;
	int restoration_trigger_iter;
	int restoration_max_iter;
	double restoration_improve;
	double restoration_margin;
	double restoration_handoff_reduction;
	double restoration_reentry_factor;
	double trust_radius_init;
	double trust_radius_min;
	double trust_radius_max;
	double trust_shrink;
	double trust_grow;
	double trust_accept;
	double trust_good;
	double trust_tiny_alpha;
	double trust_tiny_radius_factor;
	double hess_reg;
	double hess_fallback_ratio;
	double bound_push;
	double qp_time_limit;
	int qp_iteration_limit;
	double lower_inf;
	double upper_inf;
	char hessian[32];
	char exact_lagrangian_multipliers[32];
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

#define A4SQP_OPT_INT(NAME,LABEL,PAGE,DESC,DEFAULT,LOW,HIGH,PROBLEM) \
	{NAME,LABEL,DESC,PAGE,A4SqpOptionInteger,(A4SqpNumber)(DEFAULT),(A4SqpNumber)(LOW),(A4SqpNumber)(HIGH),NULL,NULL,PROBLEM}
#define A4SQP_OPT_BOOL(NAME,LABEL,PAGE,DESC,DEFAULT,PROBLEM) \
	{NAME,LABEL,DESC,PAGE,A4SqpOptionBool,(A4SqpNumber)(DEFAULT),0.0,1.0,NULL,NULL,PROBLEM}
#define A4SQP_OPT_NUM(NAME,LABEL,PAGE,DESC,DEFAULT,LOW,HIGH,PROBLEM) \
	{NAME,LABEL,DESC,PAGE,A4SqpOptionNumber,(A4SqpNumber)(DEFAULT),(A4SqpNumber)(LOW),(A4SqpNumber)(HIGH),NULL,NULL,PROBLEM}
#define A4SQP_OPT_STR(NAME,LABEL,PAGE,DESC,DEFAULT,CHOICES,PROBLEM) \
	{NAME,LABEL,DESC,PAGE,A4SqpOptionString,0.0,0.0,0.0,DEFAULT,CHOICES,PROBLEM}

static const struct A4SqpOptionInfo a4sqp_c_option_info[] = {
	A4SQP_OPT_STR("scaleopt","Scaling option",1,
		"Scaling option for the A4SQP problem view.",
		"ROW_2NORM",((const char *const[]){"AUTO","NONE","ROW_2NORM","ROW_2NORM_T2","ROW_2NORM_T5","ROW_2NORM_T10","ROW_2NORM_T100","ROW_2NORM_F1E-2","ROW_2NORM_F1E-4","RELNOM",NULL}),A4SQP_TRUE),
	A4SQP_OPT_STR("hessian","Hessian model",1,
		"Step Hessian model for the SQP QP: BFGS, EXACT_OBJ, EXACT_LAGRANGIAN, or AUTO.",
		"BFGS",((const char *const[]){"AUTO","BFGS","EXACT_OBJ","EXACT_LAGRANGIAN",NULL}),A4SQP_TRUE),
	A4SQP_OPT_STR("exact_lagrangian_multipliers","Exact Hessian multipliers",3,
		"Multiplier source for exact Lagrangian Hessian callbacks: ROW_DUAL_SIGNED, ROW_DUAL, or RECOVERED.",
		"ROW_DUAL_SIGNED",((const char *const[]){"ROW_DUAL_SIGNED","ROW_DUAL","RECOVERED",NULL}),A4SQP_TRUE),
	A4SQP_OPT_STR("try_lsq","Try least squares",1,
		"Attempt the dedicated least-squares path for recognised unconstrained sum-of-squares objectives: OFF, GAUSS, or LM.",
		"LM",((const char *const[]){"OFF","GAUSS","LM",NULL}),A4SQP_FALSE),
	A4SQP_OPT_INT("lsq_max_iter","LSQ maximum iterations",1,
		"Maximum iterations for the least-squares pre-solve; zero reuses max_iter.",
		0,0,1000000,A4SQP_FALSE),
	A4SQP_OPT_NUM("hess_reg","Hessian regularization",2,
		"Minimum diagonal margin enforced when regularizing the step Hessian to a convex QP model.",
		1e-8,0.0,1e12,A4SQP_TRUE),
	A4SQP_OPT_NUM("hess_fallback_ratio","Exact Hessian fallback ratio",2,
		"Use the BFGS fallback model for an exact-Hessian step when PSD regularization divided by the Hessian infinity norm exceeds this ratio; zero disables fallback.",
		1.0,0.0,1e12,A4SQP_TRUE),
	A4SQP_OPT_NUM("bound_push","Bound push",2,
		"Distance used when projecting an out-of-bounds initial value just inside finite variable bounds.",
		1e-8,0.0,1e3,A4SQP_TRUE),
	A4SQP_OPT_INT("verbosity","Verbosity",2,
		"A4SQP diagnostic verbosity level.",
		0,0,5,A4SQP_TRUE),
	A4SQP_OPT_INT("max_iter","Maximum iterations",1,
		"Maximum number of A4SQP major SQP iterations.",
		200,1,1000000,A4SQP_TRUE),
	A4SQP_OPT_INT("max_backtrack","Maximum backtracking steps",2,
		"Maximum number of merit line-search backtracking steps per SQP iteration.",
		20,1,100,A4SQP_TRUE),
	A4SQP_OPT_NUM("feas_tol","Feasibility tolerance",1,
		"Convergence tolerance for maximum scaled relation violation.",
		1e-7,1e-14,1e3,A4SQP_TRUE),
	A4SQP_OPT_NUM("step_tol","Step tolerance",1,
		"Convergence tolerance for physical accepted step norm.",
		1e-7,1e-14,1e3,A4SQP_TRUE),
	A4SQP_OPT_NUM("acceptable_tol","Acceptable tolerance",1,
		"Relaxed feasibility/stationarity tolerance for acceptable solves.",
		1e-5,1e-14,1e3,A4SQP_TRUE),
	A4SQP_OPT_INT("acceptable_iter","Acceptable iterations",1,
		"Number of consecutive acceptable iterations required before accepting a relaxed solve.",
		0,0,10000,A4SQP_TRUE),
	A4SQP_OPT_BOOL("kkt_convergence","KKT convergence",2,
		"Use the core KKT residual, rather than constrained small-step acceptance, for objective stationarity convergence.",
		0,A4SQP_TRUE),
	A4SQP_OPT_NUM("merit_tol","Merit tolerance",2,
		"Minimum merit decrease regarded as meaningful by the line search.",
		1e-14,0.0,1e3,A4SQP_TRUE),
	A4SQP_OPT_NUM("armijo_coeff","Armijo coefficient",2,
		"Fraction of predicted merit reduction required by the line search.",
		1e-4,1e-12,0.5,A4SQP_TRUE),
	A4SQP_OPT_NUM("elastic_penalty","Elastic penalty",1,
		"Linear penalty used for lower and upper elastic QP slacks and the merit function.",
		100.0,1e-12,1e12,A4SQP_TRUE),
	A4SQP_OPT_NUM("elastic_penalty_growth","Elastic penalty growth",2,
		"Multiplier applied when the core detects saturated elastic-QP duals while infeasibility remains.",
		10.0,1.0,1e6,A4SQP_TRUE),
	A4SQP_OPT_NUM("elastic_penalty_max","Maximum elastic penalty",2,
		"Upper bound for automatic elastic/merit penalty increases.",
		1e8,1e-12,1e16,A4SQP_TRUE),
	A4SQP_OPT_BOOL("filter_accept","Filter-lite acceptance",2,
		"Permit feasibility-improving constrained steps that reduce merit but fail the predicted-reduction ratio.",
		0,A4SQP_TRUE),
	A4SQP_OPT_NUM("filter_margin","Filter-lite margin",2,
		"Required fractional constraint-violation reduction for filter-lite acceptance.",
		1e-4,0.0,0.999999,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_radius_init","Initial trust radius",2,
		"Initial scaled infinity-norm trust-region radius for the primal SQP step.",
		1.0,1e-12,1e12,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_radius_min","Minimum trust radius",2,
		"Minimum scaled trust-region radius before A4SQP gives up shrinking the QP step.",
		1e-6,1e-12,1e12,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_radius_max","Maximum trust radius",2,
		"Maximum scaled trust-region radius for the primal SQP step.",
		100.0,1e-12,1e12,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_shrink","Trust shrink factor",2,
		"Factor applied to the trust radius after QP or agreement failure.",
		0.25,1e-6,0.999999,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_grow","Trust grow factor",2,
		"Factor applied to the trust radius after a good boundary-active step.",
		2.0,1.000001,1e6,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_accept","Trust acceptance ratio",2,
		"Minimum actual-to-predicted merit reduction ratio required for line-search acceptance.",
		0.0,-1e6,1e6,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_good","Trust good ratio",2,
		"Actual-to-predicted merit reduction ratio that triggers trust-region growth when the step is trust-active.",
		0.75,-1e6,1e6,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_tiny_alpha","Tiny-alpha trust shrink",2,
		"Accepted regular steps with alpha at or below this value shrink the trust radius toward the accepted step size; zero disables this update.",
		0.0,0.0,1.0,A4SQP_TRUE),
	A4SQP_OPT_NUM("trust_tiny_radius_factor","Tiny-alpha trust factor",2,
		"Multiplier on accepted scaled step size used as the new trust-radius target after a tiny-alpha accepted step.",
		2.0,1.0,100.0,A4SQP_TRUE),
	A4SQP_OPT_INT("trust_qp_retries","Trust-region retries",2,
		"Maximum number of trust-radius reductions and QP rebuild retries per SQP iteration.",
		5,0,100,A4SQP_TRUE),
	A4SQP_OPT_BOOL("trust_unconstrained","Trust unconstrained",2,
		"Apply the scaled trust-region radius and trust retries to unconstrained objective-only problems.",
		0,A4SQP_TRUE),
	A4SQP_OPT_BOOL("restoration","Restoration phase",2,
		"Enable feasibility restoration steps after repeated lack of constraint-violation progress.",
		0,A4SQP_TRUE),
	A4SQP_OPT_BOOL("active_bound_restoration","Active-bound restoration probe",3,
		"Enable the experimental core probe that moves a stationarity-blocking variable to a bound before a short restoration attempt.",
		1,A4SQP_TRUE),
	A4SQP_OPT_BOOL("active_bound_release","Active-bound release probe",3,
		"Enable the experimental core probe that releases a stationarity-blocking active bound before a short restoration attempt.",
		0,A4SQP_TRUE),
	A4SQP_OPT_BOOL("reduced_gradient_polish","Reduced-gradient polish",3,
		"Enable an experimental feasible-point reduced-gradient polish followed by restoration.",
		0,A4SQP_TRUE),
	A4SQP_OPT_INT("restoration_trigger_iter","Restoration trigger",2,
		"Number of consecutive materially infeasible non-improving iterations before restoration steps are requested; zero uses the conservative default.",
		3,0,10000,A4SQP_TRUE),
	A4SQP_OPT_INT("restoration_max_iter","Restoration handoff",2,
		"Maximum consecutive restoration iterations before handing back to the regular SQP objective model; zero disables the cap.",
		0,0,10000,A4SQP_TRUE),
	A4SQP_OPT_NUM("restoration_improve","Restoration progress",2,
		"Required fractional maximum-violation improvement to reset the restoration stall counter.",
		1e-3,0.0,0.999999,A4SQP_TRUE),
	A4SQP_OPT_NUM("restoration_margin","Restoration acceptance",2,
		"Required fractional constraint-violation reduction for restoration line-search acceptance.",
		1e-4,0.0,0.999999,A4SQP_TRUE),
	A4SQP_OPT_NUM("restoration_handoff_reduction","Restoration handoff reduction",2,
		"Fractional constraint-violation reduction from restoration entry required before trying the regular SQP objective model again; zero disables improvement-based handoff.",
		0.5,0.0,0.999999,A4SQP_TRUE),
	A4SQP_OPT_NUM("restoration_reentry_factor","Restoration re-entry factor",2,
		"Multiplier on the last restoration handoff violation before further restoration re-entry is allowed after repeated handoffs.",
		2.0,1.0,1e6,A4SQP_TRUE),
	A4SQP_OPT_NUM("qp_time_limit","QP time limit",3,
		"Per-QP HiGHS time limit in seconds; zero leaves HiGHS unbounded.",
		0.0,0.0,1e12,A4SQP_TRUE),
	A4SQP_OPT_INT("qp_iteration_limit","QP iteration limit",3,
		"Per-QP HiGHS iteration limit; zero leaves HiGHS unbounded.",
		0,0,100000000,A4SQP_TRUE),
	A4SQP_OPT_NUM("nlp_lower_bound_inf","NLP lower infinity",3,
		"Values less than or equal to this are treated as absent lower bounds.",
		A4SQP_C_DEFAULT_LOWER_INF,-1e300,0.0,A4SQP_TRUE),
	A4SQP_OPT_NUM("nlp_upper_bound_inf","NLP upper infinity",3,
		"Values greater than or equal to this are treated as absent upper bounds.",
		A4SQP_C_DEFAULT_UPPER_INF,0.0,1e300,A4SQP_TRUE)
};

#undef A4SQP_OPT_INT
#undef A4SQP_OPT_BOOL
#undef A4SQP_OPT_NUM
#undef A4SQP_OPT_STR

struct A4SqpCSolve {
	A4SqpProblem problem;
	A4SqpUserDataPtr user_data;
	struct A4SqpView view;
	struct A4SqpQp qp;
	struct A4SqpDenseHessian hess;
	struct A4SqpDenseHessian bfgs_hess;
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
	double last_scaled_step_inf;
	double last_alpha;
	double last_trust_ratio;
	double last_elastic_max;
	double elastic_penalty;
	int elastic_penalty_saturated;
	int accepted_step_count;
	int acceptable_count;
	double exact_hess_norm_inf;
	double exact_hess_reg_ratio;
	int using_bfgs_fallback;
	int bfgs_fallback_count;
	struct A4SqpCoreRestorationState restoration_state;
	struct A4SqpCoreMultiplierEstimate lambda_est;
};

static void a4sqp_c_refresh_stats(struct A4SqpCSolve *solve, int iterations);

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
	opt->merit_tol = 1e-14;
	opt->armijo_coeff = 1e-4;
	opt->elastic_penalty = A4SQP_QP_DEFAULT_ELASTIC_PENALTY;
	opt->elastic_penalty_growth = 10.0;
	opt->elastic_penalty_max = 1e8;
	opt->filter_accept = 0;
	opt->filter_margin = 1e-4;
	opt->trust_unconstrained = 0;
	opt->kkt_convergence = 0;
	opt->restoration = 0;
	opt->active_bound_restoration = 1;
	opt->active_bound_release = 0;
	opt->reduced_gradient_polish = 0;
	opt->restoration_trigger_iter = 3;
	opt->restoration_max_iter = 0;
	opt->restoration_improve = 1e-3;
	opt->restoration_margin = 1e-4;
	opt->restoration_handoff_reduction = 0.5;
	opt->restoration_reentry_factor = 2.0;
	opt->trust_radius_init = 1.0;
	opt->trust_radius_min = 1e-6;
	opt->trust_radius_max = 100.0;
	opt->trust_shrink = 0.25;
	opt->trust_grow = 2.0;
	opt->trust_accept = 0.0;
	opt->trust_good = 0.75;
	opt->trust_tiny_alpha = 0.0;
	opt->trust_tiny_radius_factor = 2.0;
	opt->hess_reg = 1e-8;
	opt->hess_fallback_ratio = 1.0;
	opt->bound_push = 1e-8;
	opt->qp_time_limit = 0.0;
	opt->qp_iteration_limit = 0;
	opt->lower_inf = A4SQP_C_DEFAULT_LOWER_INF;
	opt->upper_inf = A4SQP_C_DEFAULT_UPPER_INF;
	strcpy(opt->hessian,"BFGS");
	strcpy(opt->exact_lagrangian_multipliers,"ROW_DUAL_SIGNED");
	strcpy(opt->scaleopt,"ROW_2NORM");
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
		if(a4sqp_c_streq(val,"AUTO")){
			strcpy(p->opt.scaleopt,"AUTO");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"NONE")){
			strcpy(p->opt.scaleopt,"NONE");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"RELNOM")){
			strcpy(p->opt.scaleopt,"RELNOM");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"ROW_2NORM")
			|| a4sqp_c_streq(val,"ROW_2NORM_T2")
			|| a4sqp_c_streq(val,"ROW_2NORM_T5")
			|| a4sqp_c_streq(val,"ROW_2NORM_T10")
			|| a4sqp_c_streq(val,"ROW_2NORM_T100")
			|| a4sqp_c_streq(val,"ROW_2NORM_F1E-2")
			|| a4sqp_c_streq(val,"ROW_2NORM_F1E-4")
		){
			strncpy(p->opt.scaleopt,val,sizeof(p->opt.scaleopt) - 1);
			p->opt.scaleopt[sizeof(p->opt.scaleopt) - 1] = '\0';
			return A4SQP_TRUE;
		}
		return A4SQP_FALSE;
	}
	if(a4sqp_c_streq(keyword,"exact_lagrangian_multipliers")){
		if(a4sqp_c_streq(val,"ROW_DUAL_SIGNED") || a4sqp_c_streq(val,"SIGNED_ROW_DUAL")){
			strcpy(p->opt.exact_lagrangian_multipliers,"ROW_DUAL_SIGNED");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"ROW_DUAL") || a4sqp_c_streq(val,"UNSIGNED_ROW_DUAL")){
			strcpy(p->opt.exact_lagrangian_multipliers,"ROW_DUAL");
			return A4SQP_TRUE;
		}
		if(a4sqp_c_streq(val,"RECOVERED")){
			strcpy(p->opt.exact_lagrangian_multipliers,"RECOVERED");
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
	if(a4sqp_c_streq(keyword,"elastic_penalty_growth")){
		if(val < 1.0){
			return A4SQP_FALSE;
		}
		p->opt.elastic_penalty_growth = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"elastic_penalty_max")){
		if(val < p->opt.elastic_penalty){
			return A4SQP_FALSE;
		}
		p->opt.elastic_penalty_max = val;
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
	if(a4sqp_c_streq(keyword,"restoration_handoff_reduction")){
		if(val < 0.0 || val >= 1.0){
			return A4SQP_FALSE;
		}
		p->opt.restoration_handoff_reduction = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"restoration_reentry_factor")){
		if(val < 1.0 || !isfinite(val)){
			return A4SQP_FALSE;
		}
		p->opt.restoration_reentry_factor = val;
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
	if(a4sqp_c_streq(keyword,"trust_tiny_alpha")){
		if(val < 0.0 || val > 1.0 || !isfinite(val)){
			return A4SQP_FALSE;
		}
		p->opt.trust_tiny_alpha = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"trust_tiny_radius_factor")){
		if(val < 1.0 || !isfinite(val)){
			return A4SQP_FALSE;
		}
		p->opt.trust_tiny_radius_factor = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"hess_reg")){
		if(val < 0.0 || !isfinite(val)){
			return A4SQP_FALSE;
		}
		p->opt.hess_reg = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"hess_fallback_ratio")){
		if(val < 0.0 || !isfinite(val)){
			return A4SQP_FALSE;
		}
		p->opt.hess_fallback_ratio = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"bound_push")){
		if(val < 0.0){
			return A4SQP_FALSE;
		}
		p->opt.bound_push = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"qp_time_limit")){
		if(val < 0.0){
			return A4SQP_FALSE;
		}
		p->opt.qp_time_limit = val;
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
	if(a4sqp_c_streq(keyword,"active_bound_restoration")){
		p->opt.active_bound_restoration = val != 0;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"active_bound_release")){
		p->opt.active_bound_release = val != 0;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"reduced_gradient_polish")){
		p->opt.reduced_gradient_polish = val != 0;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"restoration_trigger_iter")){
		if(val < 0){
			return A4SQP_FALSE;
		}
		p->opt.restoration_trigger_iter = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"restoration_max_iter")){
		if(val < 0){
			return A4SQP_FALSE;
		}
		p->opt.restoration_max_iter = val;
		return A4SQP_TRUE;
	}
	if(a4sqp_c_streq(keyword,"qp_iteration_limit")){
		if(val < 0){
			return A4SQP_FALSE;
		}
		p->opt.qp_iteration_limit = val;
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

A4SqpIndex GetA4SqpOptionCount(void){
	return (A4SqpIndex)(sizeof(a4sqp_c_option_info) / sizeof(a4sqp_c_option_info[0]));
}

A4SqpBool GetA4SqpOptionInfo(A4SqpIndex index, struct A4SqpOptionInfo *info){
	if(index < 0 || index >= GetA4SqpOptionCount() || info == NULL){
		return A4SQP_FALSE;
	}
	*info = a4sqp_c_option_info[index];
	return A4SQP_TRUE;
}

A4SqpBool GetA4SqpOptionInfoByName(char *keyword, struct A4SqpOptionInfo *info){
	A4SqpIndex i;
	if(keyword == NULL || info == NULL){
		return A4SQP_FALSE;
	}
	for(i = 0; i < GetA4SqpOptionCount(); ++i){
		if(a4sqp_c_streq(keyword,a4sqp_c_option_info[i].keyword)){
			*info = a4sqp_c_option_info[i];
			return A4SQP_TRUE;
		}
	}
	return A4SQP_FALSE;
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
	/* Explicit row scaling is authoritative. Explicit variable scaling can
	 * still be combined with automatic row scaling, because the row 2-norm is
	 * computed after the variable scale has been applied. ASCEND supplies both
	 * variable and row presolve scales through SetA4SqpProblemScaling, so this
	 * condition avoids double-scaling the ASCEND problem view.
	 */
	if(
		p->g_scale == NULL
		&& !a4sqp_c_streq(p->opt.scaleopt,"NONE")
		&& a4sqp_view_apply_scaling(view,p->opt.scaleopt)
	){
		return 1;
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

static const double *a4sqp_c_stationarity_multipliers(struct A4SqpCSolve *solve){
	struct A4SqpCoreView core;
	if(
		solve == NULL
		|| solve->problem == NULL
		|| !solve->has_objective
		|| solve->problem->m <= 0
		|| solve->lambda == NULL
	){
		return solve != NULL ? solve->lambda : NULL;
	}
	if(
		!solve->problem->opt.kkt_convergence
		&& !a4sqp_c_streq(solve->problem->opt.hessian,"EXACT_LAGRANGIAN")
	){
		return solve->lambda;
	}
	a4sqp_view_get_core(&solve->view,&core);
	core.has_objective = solve->has_objective;
	if(a4sqp_core_multiplier_estimate_recover_stationarity(
		&solve->lambda_est,
		&core,
		solve->problem->opt.feas_tol
	)){
		return solve->lambda;
	}
	if(solve->lambda_est.lambda == NULL || solve->lambda_est.n != solve->problem->m){
		return solve->lambda;
	}
	return solve->lambda_est.lambda;
}

static void a4sqp_c_update_kkt_stats(struct A4SqpCSolve *solve){
	struct A4SqpKktResidual residual;
	struct A4SqpCoreBoundStats bound_stats;
	const double *stat_lambda;
	if(solve == NULL || solve->problem == NULL){
		return;
	}
	/* KKT reporting may use recovered stationarity multipliers, but those
	 * estimates must not overwrite the QP row duals used by subsequent Hessian
	 * construction.
	 */
	stat_lambda = a4sqp_c_stationarity_multipliers(solve);
	a4sqp_core_kkt_error(
		&solve->view,
		solve->has_objective,
		stat_lambda,
		solve->problem->opt.feas_tol,
		&residual
	);
	solve->problem->stats.kkt_error = residual.kkt_error;
	solve->problem->stats.dual_infeasibility_inf = residual.dual_inf;
	solve->problem->stats.complementarity_inf = residual.complementarity_inf;
	solve->problem->stats.kkt_lambda_sign = residual.lambda_sign;
	if(!a4sqp_core_bound_stats(
		&solve->view,
		solve->has_objective,
		stat_lambda,
		residual.lambda_sign != 0 ? (double)residual.lambda_sign : 1.0,
		solve->problem->opt.feas_tol,
		&bound_stats
	)){
		solve->problem->stats.bound_lower_active = bound_stats.lower_active;
		solve->problem->stats.bound_upper_active = bound_stats.upper_active;
		solve->problem->stats.bound_fixed_active = bound_stats.fixed_active;
		solve->problem->stats.bound_worst_index = bound_stats.worst_index;
		solve->problem->stats.bound_stationarity_inf = bound_stats.stationarity_inf;
		solve->problem->stats.bound_worst_lagrangian_gradient = bound_stats.worst_lagrangian_gradient;
	}
}

static int a4sqp_c_hess_reset_identity(struct A4SqpCSolve *solve, double diag){
	return a4sqp_dense_hessian_reset_identity(&solve->hess,solve->problem->n,diag);
}

static int a4sqp_c_dense_cholesky_ok(const double *hess, int n, double pivot_floor){
	double *l = NULL;
	int i;
	int j;
	int k;
	int ok = 0;
	if(hess == NULL || n <= 0){
		return 0;
	}
	if(!isfinite(pivot_floor) || pivot_floor <= 0.0){
		pivot_floor = 1e-12;
	}
	l = A4SQP_NEW_ARRAY_CLEAR(double,(size_t)n * (size_t)n);
	if(l == NULL){
		return 0;
	}
	for(i = 0; i < n; ++i){
		for(j = 0; j <= i; ++j){
			double sum = hess[(size_t)i * (size_t)n + (size_t)j];
			for(k = 0; k < j; ++k){
				sum -= l[(size_t)i * (size_t)n + (size_t)k] * l[(size_t)j * (size_t)n + (size_t)k];
			}
			if(i == j){
				if(!isfinite(sum) || sum < pivot_floor){
					goto cleanup;
				}
				l[(size_t)i * (size_t)n + (size_t)i] = sqrt(sum);
			}else{
				double ljj = l[(size_t)j * (size_t)n + (size_t)j];
				if(!isfinite(ljj) || ljj <= 0.0){
					goto cleanup;
				}
				l[(size_t)i * (size_t)n + (size_t)j] = sum / ljj;
			}
		}
	}
	ok = 1;

cleanup:
	A4SQP_FREE(l);
	return ok;
}

static int a4sqp_c_solve_dense_system(double *a, double *b, int n){
	int i;
	int j;
	int k;
	if(a == NULL || b == NULL || n < 0){
		return 1;
	}
	for(k = 0; k < n; ++k){
		int pivot = k;
		double pivot_abs = fabs(a[(size_t)k * (size_t)n + (size_t)k]);
		for(i = k + 1; i < n; ++i){
			double candidate = fabs(a[(size_t)i * (size_t)n + (size_t)k]);
			if(candidate > pivot_abs){
				pivot = i;
				pivot_abs = candidate;
			}
		}
		if(pivot_abs <= 1e-18 || !isfinite(pivot_abs)){
			return 1;
		}
		if(pivot != k){
			for(j = k; j < n; ++j){
				double tmp = a[(size_t)k * (size_t)n + (size_t)j];
				a[(size_t)k * (size_t)n + (size_t)j] = a[(size_t)pivot * (size_t)n + (size_t)j];
				a[(size_t)pivot * (size_t)n + (size_t)j] = tmp;
			}
			{
				double tmp = b[k];
				b[k] = b[pivot];
				b[pivot] = tmp;
			}
		}
		for(i = k + 1; i < n; ++i){
			double factor = a[(size_t)i * (size_t)n + (size_t)k] / a[(size_t)k * (size_t)n + (size_t)k];
			if(factor == 0.0){
				continue;
			}
			a[(size_t)i * (size_t)n + (size_t)k] = 0.0;
			for(j = k + 1; j < n; ++j){
				a[(size_t)i * (size_t)n + (size_t)j] -= factor * a[(size_t)k * (size_t)n + (size_t)j];
			}
			b[i] -= factor * b[k];
		}
	}
	for(i = n - 1; i >= 0; --i){
		double sum = b[i];
		for(j = i + 1; j < n; ++j){
			sum -= a[(size_t)i * (size_t)n + (size_t)j] * b[j];
		}
		b[i] = sum / a[(size_t)i * (size_t)n + (size_t)i];
		if(!isfinite(b[i])){
			return 1;
		}
	}
	return 0;
}

static int a4sqp_c_hess_add_scaled_equality_normal_matrix(
	const struct A4SqpView *view,
	double scale,
	double *target
){
	int row;
	if(view == NULL || target == NULL || view->n_var <= 0 || view->n_rel <= 0 || !isfinite(scale) || scale == 0.0){
		return 1;
	}
	for(row = 0; row < view->n_rel; ++row){
		int k1;
		if(view->rel_kind == NULL || view->rel_kind[row] != A4SQP_REL_KIND_EQUALITY){
			continue;
		}
		for(k1 = view->jac_row_start[row]; k1 < view->jac_row_start[row + 1]; ++k1){
			int k2;
			int col1 = view->jac_col_index[k1];
			double jac1 = view->scaled_jac_value[k1];
			if(col1 < 0 || col1 >= view->n_var || !isfinite(jac1)){
				continue;
			}
			for(k2 = view->jac_row_start[row]; k2 < view->jac_row_start[row + 1]; ++k2){
				int col2 = view->jac_col_index[k2];
				double jac2 = view->scaled_jac_value[k2];
				if(col2 < 0 || col2 >= view->n_var || !isfinite(jac2)){
					continue;
				}
				target[(size_t)col1 * (size_t)view->n_var + (size_t)col2] += scale * jac1 * jac2;
			}
		}
	}
	return 0;
}

static double a4sqp_c_hess_regularize_equality_normal(struct A4SqpCSolve *solve, double min_diag){
	double *candidate = NULL;
	double beta = 0.0;
	double scale = 1.0;
	int i;
	int n;
	int tries;
	int has_equality = 0;
	if(solve == NULL || solve->problem == NULL || solve->hess.dense == NULL){
		return -1.0;
	}
	n = solve->problem->n;
	if(n <= 0 || solve->problem->m <= 0 || solve->view.n_rel <= 0){
		return -1.0;
	}
	for(i = 0; i < solve->view.n_rel; ++i){
		if(solve->view.rel_kind != NULL && solve->view.rel_kind[i] == A4SQP_REL_KIND_EQUALITY){
			has_equality = 1;
			break;
		}
	}
	if(!has_equality){
		return -1.0;
	}
	if(!isfinite(min_diag) || min_diag < 0.0){
		min_diag = 1e-8;
	}
	for(i = 0; i < n * n; ++i){
		double value = solve->hess.dense[i];
		if(!isfinite(value)){
			value = 0.0;
		}
		if(fabs(value) > scale){
			scale = fabs(value);
		}
	}
	candidate = A4SQP_NEW_ARRAY_OR_NULL(double,(size_t)n * (size_t)n);
	if(candidate == NULL){
		return -1.0;
	}
	for(tries = 0; tries < 12; ++tries){
		memcpy(candidate,solve->hess.dense,(size_t)n * (size_t)n * sizeof(*candidate));
		if(beta > 0.0){
			(void)a4sqp_c_hess_add_scaled_equality_normal_matrix(&solve->view,beta,candidate);
		}
		if(a4sqp_c_dense_cholesky_ok(candidate,n,min_diag)){
			memcpy(solve->hess.dense,candidate,(size_t)n * (size_t)n * sizeof(*candidate));
			A4SQP_FREE(candidate);
			return beta;
		}
		if(beta <= 0.0){
			beta = fmax(min_diag,1e-8 * scale);
		}else{
			beta *= 10.0;
		}
	}
	A4SQP_FREE(candidate);
	return -1.0;
}

static void a4sqp_c_project_x_to_bounds(struct A4SqpProblemInfo *p, double *x){
	int i;
	double bound_push;
	if(p == NULL || x == NULL){
		return;
	}
	bound_push = p->opt.bound_push;
	for(i = 0; i < p->n; ++i){
		double lower = a4sqp_c_map_bound(p->x_l[i],p->opt.lower_inf,p->opt.upper_inf);
		double upper = a4sqp_c_map_bound(p->x_u[i],p->opt.lower_inf,p->opt.upper_inf);
		if(!a4sqp_c_is_lower_inf(lower) && x[i] < lower){
			x[i] = (!a4sqp_c_is_upper_inf(upper) && lower + bound_push > upper)
				? lower
				: lower + bound_push;
		}
		if(!a4sqp_c_is_upper_inf(upper) && x[i] > upper){
			x[i] = (!a4sqp_c_is_lower_inf(lower) && upper - bound_push < lower)
				? upper
				: upper - bound_push;
		}
	}
}

static double a4sqp_c_hess_dense_inf_norm(const double *hess, int n){
	double norm = 0.0;
	int i;
	int j;
	if(hess == NULL || n <= 0){
		return 0.0;
	}
	for(i = 0; i < n; ++i){
		double row_sum = 0.0;
		for(j = 0; j < n; ++j){
			double value = hess[(size_t)i * (size_t)n + (size_t)j];
			if(!isfinite(value)){
				value = 0.0;
			}
			row_sum += fabs(value);
		}
		if(row_sum > norm){
			norm = row_sum;
		}
	}
	return norm;
}

static double a4sqp_c_hess_regularize_psd(struct A4SqpCSolve *solve){
	double min_diag;
	double normal_reg;
	if(solve == NULL){
		return 0.0;
	}
	min_diag = solve->problem->opt.hess_reg;
	normal_reg = a4sqp_c_hess_regularize_equality_normal(solve,min_diag);
	if(normal_reg >= 0.0){
		solve->hess.last_reg = normal_reg;
		return normal_reg;
	}
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

static int a4sqp_c_hess_values_are_zero(const double *values, int nnz){
	int k;
	if(values == NULL || nnz <= 0){
		return 1;
	}
	for(k = 0; k < nnz; ++k){
		if(!isfinite(values[k])){
			return 0;
		}
		if(fabs(values[k]) > 1e-18){
			return 0;
		}
	}
	return 1;
}

static int a4sqp_c_hess_fill_objective_fd(
	struct A4SqpCSolve *solve,
	const double *x,
	const A4SqpIndex *irow,
	const A4SqpIndex *jcol,
	double *values
){
	struct A4SqpProblemInfo *p;
	double *xp = NULL;
	double *xm = NULL;
	double *gp = NULL;
	double *gm = NULL;
	double *g0 = NULL;
	double *dense = NULL;
	int i;
	int j;
	int k;
	int status = 1;
	if(solve == NULL || solve->problem == NULL || x == NULL || values == NULL){
		return 1;
	}
	p = solve->problem;
	if(p->eval_grad_f == NULL || p->n <= 0){
		return 1;
	}
	xp = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
	xm = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
	gp = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
	gm = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
	g0 = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
	dense = A4SQP_NEW_ARRAY_CLEAR(double,(size_t)p->n * (size_t)p->n);
	if(xp == NULL || xm == NULL || gp == NULL || gm == NULL || g0 == NULL || dense == NULL){
		goto cleanup;
	}
	memcpy(xp,x,(size_t)p->n * sizeof(*xp));
	memcpy(xm,x,(size_t)p->n * sizeof(*xm));
	for(j = 0; j < p->n; ++j){
		double old = x[j];
		double h = sqrt(1e-12) * (fabs(old) > 1.0 ? fabs(old) : 1.0);
		if(!isfinite(h) || h <= 0.0){
			h = 1e-6;
		}
		xp[j] = old + h;
		xm[j] = old - h;
		if(
			!p->eval_grad_f(p->n,xp,A4SQP_TRUE,gp,solve->user_data)
			|| !p->eval_grad_f(p->n,xm,A4SQP_TRUE,gm,solve->user_data)
		){
			solve->callback_error = 1;
			goto cleanup;
		}
		for(i = 0; i < p->n; ++i){
			double value = p->obj_scaling * (gp[i] - gm[i]) / (2.0 * h);
			if(!isfinite(value)){
				solve->callback_error = 1;
				goto cleanup;
			}
			dense[i * p->n + j] = value;
		}
		xp[j] = old;
		xm[j] = old;
	}
	for(i = 0; i < p->n; ++i){
		for(j = i + 1; j < p->n; ++j){
			double sym = 0.5 * (dense[i * p->n + j] + dense[j * p->n + i]);
			dense[i * p->n + j] = sym;
			dense[j * p->n + i] = sym;
		}
	}
	for(k = 0; k < p->nele_hess; ++k){
		int row = irow[k] - (p->index_style == 1 ? 1 : 0);
		int col = jcol[k] - (p->index_style == 1 ? 1 : 0);
		if(row < 0 || row >= p->n || col < 0 || col >= p->n){
			values[k] = 0.0;
		}else{
			values[k] = dense[row * p->n + col];
		}
	}
	(void)p->eval_grad_f(p->n,(double *)x,A4SQP_TRUE,g0,solve->user_data);
	status = 0;

cleanup:
	A4SQP_FREE(xp);
	A4SQP_FREE(xm);
	A4SQP_FREE(gp);
	A4SQP_FREE(gm);
	A4SQP_FREE(g0);
	A4SQP_FREE(dense);
	return status;
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
			if(
				a4sqp_c_streq(p->opt.exact_lagrangian_multipliers,"RECOVERED")
				&& solve->lambda_est.ready
				&& solve->lambda_est.lambda != NULL
				&& solve->lambda_est.n == p->m
			){
				lambda[k] = solve->lambda_est.lambda[k];
			}else if(a4sqp_c_streq(p->opt.exact_lagrangian_multipliers,"ROW_DUAL")){
				lambda[k] = row_dual * rel_scale;
			}else{
				/* CUTEst/IPOPT-style eval_h uses the opposite constraint multiplier
				 * sign to the HiGHS row-dual convention used internally here.
				 */
				lambda[k] = -row_dual * rel_scale;
			}
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
	if(
		a4sqp_c_streq(p->opt.hessian,"EXACT_OBJ")
		&& solve->has_objective
		&& a4sqp_c_hess_values_are_zero(values,p->nele_hess)
	){
		if(a4sqp_c_hess_fill_objective_fd(solve,x,irow,jcol,values)){
			A4SQP_FREE(irow);
			A4SQP_FREE(jcol);
			A4SQP_FREE(values);
			A4SQP_FREE(lambda);
			return 0;
		}
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
	solve->exact_hess_norm_inf = a4sqp_c_hess_dense_inf_norm(solve->hess.dense,n);
	p->stats.regularization_size = a4sqp_c_hess_regularize_psd(solve);
	if(solve->exact_hess_norm_inf > 0.0 && isfinite(solve->exact_hess_norm_inf)){
		solve->exact_hess_reg_ratio = solve->hess.last_reg / fmax(1.0,solve->exact_hess_norm_inf);
	}else{
		solve->exact_hess_reg_ratio = solve->hess.last_reg;
	}
	A4SQP_FREE(irow);
	A4SQP_FREE(jcol);
	A4SQP_FREE(values);
	A4SQP_FREE(lambda);
	return 0;
}

static void a4sqp_c_hess_bfgs_update_model(
	struct A4SqpCSolve *solve,
	struct A4SqpDenseHessian *model,
	const double *old_x,
	const double *old_grad
){
	if(solve == NULL || solve->problem == NULL || model == NULL){
		return;
	}
	(void)a4sqp_dense_hessian_bfgs_update(
		model,
		solve->view.scaled_var_value,
		solve->view.scaled_obj_gradient,
		old_x,
		old_grad,
		solve->problem->opt.hess_reg
	);
	if(model == &solve->hess){
		solve->problem->stats.regularization_size = solve->hess.last_reg;
	}
}

static void a4sqp_c_hess_bfgs_update(
	struct A4SqpCSolve *solve,
	const double *old_x,
	const double *old_grad
){
	if(solve == NULL || solve->problem == NULL){
		return;
	}
	a4sqp_c_hess_bfgs_update_model(solve,&solve->hess,old_x,old_grad);
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
	if(ls == NULL || ls->solve == NULL || restoration){
		return;
	}
	if(a4sqp_c_uses_exact_hessian(ls->solve->problem)){
		a4sqp_c_hess_bfgs_update_model(ls->solve,&ls->solve->bfgs_hess,old_scaled_x,old_scaled_grad);
	}else{
		a4sqp_c_hess_bfgs_update(ls->solve,old_scaled_x,old_scaled_grad);
	}
}

static void a4sqp_c_record_line_search_result(
	struct A4SqpCSolve *solve,
	const struct A4SqpLineSearchResult *result
){
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
	solve->last_scaled_step_inf = result->scaled_step_inf;
	solve->last_trust_ratio = result->trust_ratio;
	solve->last_ls_trials = result->trials;
}

static int a4sqp_c_try_stationarity_correction(struct A4SqpCSolve *solve, double *x){
	struct A4SqpProblemInfo *p;
	struct A4SqpCoreView core;
	double *old_x = NULL;
	double *lag_grad = NULL;
	double *system = NULL;
	double *rhs = NULL;
	int *eq_rows = NULL;
	double old_kkt;
	double old_vio;
	double old_merit;
	const double *stat_lambda;
	int n;
	int nactive = 0;
	int row;
	int i;
	int dim;
	int accepted = 0;
	if(solve == NULL || solve->problem == NULL || x == NULL || !solve->has_objective){
		return 0;
	}
	p = solve->problem;
	n = p->n;
	if(n <= 0 || p->m <= 0 || solve->lambda == NULL || solve->view.n_rel <= 0){
		return 0;
	}
	if(p->stats.max_constraint_violation > p->opt.acceptable_tol){
		return 0;
	}
	if(p->stats.kkt_error <= p->opt.feas_tol || p->stats.kkt_error <= 0.0 || !isfinite(p->stats.kkt_error)){
		return 0;
	}
	if(solve->last_alpha > 0.0){
		double stationarity_step_trigger = fmax(10.0 * p->opt.step_tol,1e-8);
		if(p->opt.acceptable_tol > 0.0 && p->opt.acceptable_tol < 1.0){
			stationarity_step_trigger = fmax(stationarity_step_trigger,sqrt(p->opt.acceptable_tol));
		}
		if(solve->last_scaled_step_inf > stationarity_step_trigger){
			return 0;
		}
	}
	a4sqp_view_get_core(&solve->view,&core);
	core.has_objective = solve->has_objective;
	for(row = 0; row < solve->view.n_rel; ++row){
		enum A4SqpRowActivity activity = a4sqp_core_row_activity_for_view(
			&core,
			row,
			p->opt.acceptable_tol,
			fmax(10.0 * p->opt.acceptable_tol,1e-8)
		);
		if(activity == A4SQP_ROW_EQUALITY || activity == A4SQP_ROW_ACTIVE){
			++nactive;
		}
	}
	if(nactive <= 0){
		return 0;
	}
	old_kkt = p->stats.kkt_error;
	old_vio = p->stats.max_constraint_violation;
	old_merit = a4sqp_core_view_merit(&solve->view,solve->has_objective,solve->elastic_penalty);
	stat_lambda = a4sqp_c_stationarity_multipliers(solve);
	if(a4sqp_c_uses_exact_hessian(p)){
		if(a4sqp_c_hess_update_exact(solve,x) || solve->callback_error){
			solve->callback_error = 0;
			return 0;
		}
	}
	if(solve->hess.dense == NULL || solve->hess.n != n){
		return 0;
	}
	old_x = A4SQP_NEW_ARRAY_OR_NULL(double,n);
	lag_grad = A4SQP_NEW_ARRAY_CLEAR(double,n);
	eq_rows = A4SQP_NEW_ARRAY_OR_NULL(int,nactive);
	dim = n + nactive;
	system = A4SQP_NEW_ARRAY_CLEAR(double,(size_t)dim * (size_t)dim);
	rhs = A4SQP_NEW_ARRAY_CLEAR(double,dim);
	if(old_x == NULL || lag_grad == NULL || eq_rows == NULL || system == NULL || rhs == NULL){
		goto cleanup;
	}
	memcpy(old_x,x,(size_t)n * sizeof(*old_x));
	if(a4sqp_core_lagrangian_gradient_for_view(
		&core,
		stat_lambda,
		p->stats.kkt_lambda_sign != 0 ? (double)p->stats.kkt_lambda_sign : 1.0,
		lag_grad
	)){
		goto cleanup;
	}
	nactive = 0;
	for(row = 0; row < solve->view.n_rel; ++row){
		enum A4SqpRowActivity activity = a4sqp_core_row_activity_for_view(
			&core,
			row,
			p->opt.acceptable_tol,
			fmax(10.0 * p->opt.acceptable_tol,1e-8)
		);
		if(activity == A4SQP_ROW_EQUALITY || activity == A4SQP_ROW_ACTIVE){
			eq_rows[nactive++] = row;
		}
	}
	for(i = 0; i < n; ++i){
		int j;
		for(j = 0; j < n; ++j){
			system[(size_t)i * (size_t)dim + (size_t)j] = solve->hess.dense[(size_t)i * (size_t)n + (size_t)j];
		}
		system[(size_t)i * (size_t)dim + (size_t)i] += p->opt.hess_reg > 0.0 ? p->opt.hess_reg : 1e-8;
		rhs[i] = -lag_grad[i];
	}
	for(i = 0; i < nactive; ++i){
		int erow = eq_rows[i];
		int k;
		double lower = solve->view.scaled_rel_lower[erow];
		double upper = solve->view.scaled_rel_upper[erow];
		double residual = solve->view.scaled_rel_residual[erow];
		double lower_gap = a4sqp_c_is_lower_inf(lower) ? HUGE_VAL : fabs(residual - lower);
		double upper_gap = a4sqp_c_is_upper_inf(upper) ? HUGE_VAL : fabs(upper - residual);
		double target = lower_gap <= upper_gap ? lower : upper;
		int sys_row = n + i;
		rhs[sys_row] = -(solve->view.scaled_rel_residual[erow] - target);
		for(k = solve->view.jac_row_start[erow]; k < solve->view.jac_row_start[erow + 1]; ++k){
			int col = solve->view.jac_col_index[k];
			double jac = solve->view.scaled_jac_value[k];
			if(col < 0 || col >= n || !isfinite(jac)){
				continue;
			}
			system[(size_t)col * (size_t)dim + (size_t)sys_row] = jac;
			system[(size_t)sys_row * (size_t)dim + (size_t)col] = jac;
		}
	}
	if(a4sqp_c_solve_dense_system(system,rhs,dim)){
		goto cleanup;
	}
	{
		double step_inf = 0.0;
		double limit = solve->trust_radius > 0.0 ? solve->trust_radius : 1.0;
		double step_scale = 1.0;
		double alpha = 1.0;
		for(i = 0; i < n; ++i){
			if(fabs(rhs[i]) > step_inf){
				step_inf = fabs(rhs[i]);
			}
		}
		if(step_inf > limit && limit > 0.0){
			step_scale = limit / step_inf;
		}
		for(; alpha >= 1.0 / 1024.0; alpha *= 0.5){
			double new_maxvio = 0.0;
			double new_merit;
			double step_norm2 = 0.0;
			double scaled_step_inf = 0.0;
			for(i = 0; i < n; ++i){
				double scaled_step = alpha * step_scale * rhs[i];
				double physical_step = scaled_step * solve->view.var_scale[i];
				x[i] = old_x[i] + physical_step;
				step_norm2 += physical_step * physical_step;
				if(fabs(scaled_step) > scaled_step_inf){
					scaled_step_inf = fabs(scaled_step);
				}
			}
			a4sqp_c_project_x_to_bounds(p,x);
			if(a4sqp_c_build_view(solve,x,A4SQP_TRUE) || solve->callback_error){
				solve->callback_error = 0;
				memcpy(x,old_x,(size_t)n * sizeof(*x));
				(void)a4sqp_c_build_view(solve,x,A4SQP_TRUE);
				continue;
			}
			a4sqp_c_refresh_stats(solve,p->stats.iterations);
			new_maxvio = p->stats.max_constraint_violation;
			new_merit = a4sqp_core_view_merit(&solve->view,solve->has_objective,solve->elastic_penalty);
			if(
				new_maxvio <= fmax(p->opt.acceptable_tol,10.0 * old_vio)
				&& p->stats.kkt_error < old_kkt
				&& (p->stats.kkt_error <= 0.5 * old_kkt || new_merit <= old_merit + p->opt.acceptable_tol)
			){
				solve->last_alpha = alpha;
				solve->last_step_norm = sqrt(step_norm2);
				solve->last_scaled_step_inf = scaled_step_inf;
				solve->last_trust_ratio = 1.0;
				accepted = 1;
				break;
			}
		}
	}
	if(!accepted){
		memcpy(x,old_x,(size_t)n * sizeof(*x));
		(void)a4sqp_c_build_view(solve,x,A4SQP_TRUE);
		a4sqp_c_refresh_stats(solve,p->stats.iterations);
	}

cleanup:
	A4SQP_FREE(old_x);
	A4SQP_FREE(lag_grad);
	A4SQP_FREE(eq_rows);
	A4SQP_FREE(system);
	A4SQP_FREE(rhs);
	return accepted;
}

static int a4sqp_c_try_reduced_gradient_polish(
	struct A4SqpCSolve *solve,
	double *x,
	const struct A4SqpLineSearchOptions *base_line_options,
	const struct A4SqpVectorLineSearchOps *line_ops,
	void *line_ctx
){
	struct A4SqpProblemInfo *p;
	struct A4SqpCoreView core;
	struct A4SqpLineSearchOptions line_options;
	struct A4SqpLineSearchResult result;
	const double *stat_lambda;
	double *old_x = NULL;
	double *grad = NULL;
	double *proj = NULL;
	double *basis = NULL;
	double *normal = NULL;
	double old_kkt;
	double old_vio;
	double max_step = 0.0;
	int basis_count = 0;
	int n;
	int i;
	int row;
	int accepted = 0;
	if(
		solve == NULL
		|| solve->problem == NULL
		|| x == NULL
		|| base_line_options == NULL
		|| line_ops == NULL
		|| line_ops->evaluate == NULL
		|| !solve->has_objective
	){
		return 0;
	}
	p = solve->problem;
	n = p->n;
	if(
		!p->opt.restoration
		|| !p->opt.kkt_convergence
		|| !p->opt.reduced_gradient_polish
		|| n <= 0
		|| p->m <= 0
		|| p->stats.max_constraint_violation > p->opt.acceptable_tol
		|| p->stats.kkt_error <= p->opt.acceptable_tol
		|| solve->last_scaled_step_inf > fmax(sqrt(p->opt.acceptable_tol),10.0 * p->opt.step_tol)
	){
		return 0;
	}
	a4sqp_view_get_core(&solve->view,&core);
	core.has_objective = solve->has_objective;
	stat_lambda = a4sqp_c_stationarity_multipliers(solve);
	old_x = A4SQP_NEW_ARRAY_OR_NULL(double,n);
	grad = A4SQP_NEW_ARRAY_CLEAR(double,n);
	proj = A4SQP_NEW_ARRAY_CLEAR(double,n);
	basis = A4SQP_NEW_ARRAY_CLEAR(double,(size_t)n * (size_t)n);
	normal = A4SQP_NEW_ARRAY_CLEAR(double,n);
	if(old_x == NULL || grad == NULL || proj == NULL || basis == NULL || normal == NULL){
		goto cleanup;
	}
	memcpy(old_x,x,(size_t)n * sizeof(*old_x));
	if(a4sqp_core_lagrangian_gradient_for_view(
		&core,
		stat_lambda,
		p->stats.kkt_lambda_sign != 0 ? (double)p->stats.kkt_lambda_sign : 1.0,
		grad
	)){
		goto cleanup;
	}
	for(i = 0; i < n; ++i){
		double value = core.scaled_var_value[i];
		double lower = core.scaled_var_lower[i];
		double upper = core.scaled_var_upper[i];
		int at_lower = !a4sqp_c_is_lower_inf(lower) && value <= lower + p->opt.acceptable_tol;
		int at_upper = !a4sqp_c_is_upper_inf(upper) && value >= upper - p->opt.acceptable_tol;
		proj[i] = grad[i];
		if((at_lower || at_upper) && basis_count < n){
			basis[(size_t)basis_count * (size_t)n + (size_t)i] = 1.0;
			++basis_count;
		}
	}
	for(row = 0; row < core.n_rel; ++row){
		enum A4SqpRowActivity activity = a4sqp_core_row_activity_for_view(
			&core,
			row,
			p->opt.acceptable_tol,
			fmax(10.0 * p->opt.acceptable_tol,1e-8)
		);
		int k;
		if(activity != A4SQP_ROW_EQUALITY && activity != A4SQP_ROW_ACTIVE){
			continue;
		}
		memset(normal,0,(size_t)n * sizeof(*normal));
		for(k = core.jac_row_start[row]; k < core.jac_row_start[row + 1]; ++k){
			int col = core.jac_col_index[k];
			if(col >= 0 && col < n){
				normal[col] = core.scaled_jac_value[k];
			}
		}
		for(i = 0; i < basis_count; ++i){
			int j;
			double dot = 0.0;
			for(j = 0; j < n; ++j){
				dot += basis[(size_t)i * (size_t)n + (size_t)j] * normal[j];
			}
			for(j = 0; j < n; ++j){
				normal[j] -= dot * basis[(size_t)i * (size_t)n + (size_t)j];
			}
		}
		{
			double norm2 = 0.0;
			int j;
			for(j = 0; j < n; ++j){
				norm2 += normal[j] * normal[j];
			}
			if(norm2 > 1e-20 && basis_count < n){
				double inv_norm = 1.0 / sqrt(norm2);
				for(j = 0; j < n; ++j){
					basis[(size_t)basis_count * (size_t)n + (size_t)j] = normal[j] * inv_norm;
				}
				++basis_count;
			}
		}
	}
	for(i = 0; i < basis_count; ++i){
		int j;
		double dot = 0.0;
		for(j = 0; j < n; ++j){
			dot += basis[(size_t)i * (size_t)n + (size_t)j] * proj[j];
		}
		for(j = 0; j < n; ++j){
			proj[j] -= dot * basis[(size_t)i * (size_t)n + (size_t)j];
		}
	}
	for(i = 0; i < n; ++i){
		double physical = fabs(proj[i] * (solve->view.var_scale != NULL ? solve->view.var_scale[i] : 1.0));
		if(physical > max_step){
			max_step = physical;
		}
	}
	if(max_step <= p->opt.acceptable_tol || !isfinite(max_step)){
		goto cleanup;
	}
	old_kkt = p->stats.kkt_error;
	old_vio = p->stats.max_constraint_violation;
	line_options = *base_line_options;
	line_options.restoration = 1;
	line_options.restoration_margin = p->opt.restoration_margin;
	for(i = 0; i < 12 && !accepted; ++i){
		double alpha = fmin(0.25,solve->trust_radius > 0.0 ? solve->trust_radius : 0.25) / max_step;
		int j;
		int iter;
		alpha /= pow(2.0,(double)i);
		memcpy(x,old_x,(size_t)n * sizeof(*x));
		for(j = 0; j < n; ++j){
			double scale = solve->view.var_scale != NULL ? solve->view.var_scale[j] : 1.0;
			x[j] -= alpha * proj[j] * scale;
		}
		a4sqp_c_project_x_to_bounds(p,x);
		if(a4sqp_c_build_view(solve,x,A4SQP_TRUE) || solve->callback_error){
			solve->callback_error = 0;
			continue;
		}
		memset(&result,0,sizeof(result));
		for(iter = 0; iter < 20; ++iter){
			if(a4sqp_core_nonlinear_restoration_step(
				&solve->view,
				&line_options,
				line_ops,
				line_ctx,
				x,
				solve->trust_radius,
				&result
			)){
				break;
			}
			a4sqp_c_refresh_stats(solve,p->stats.iterations);
			if(p->stats.max_constraint_violation <= p->opt.acceptable_tol){
				break;
			}
		}
		a4sqp_c_refresh_stats(solve,p->stats.iterations);
		if(
			p->stats.max_constraint_violation <= fmax(p->opt.acceptable_tol,old_vio)
			&& p->stats.kkt_error < old_kkt - fmax(p->opt.feas_tol,1e-12)
		){
			solve->last_alpha = alpha;
			solve->last_step_norm = result.step_norm;
			solve->last_scaled_step_inf = result.scaled_step_inf;
			solve->last_trust_ratio = result.trust_ratio;
			solve->last_ls_trials = result.trials;
			accepted = 1;
		}
	}
	if(!accepted){
		memcpy(x,old_x,(size_t)n * sizeof(*x));
		(void)a4sqp_c_build_view(solve,x,A4SQP_TRUE);
		a4sqp_c_refresh_stats(solve,p->stats.iterations);
	}

cleanup:
	A4SQP_FREE(old_x);
	A4SQP_FREE(grad);
	A4SQP_FREE(proj);
	A4SQP_FREE(basis);
	A4SQP_FREE(normal);
	return accepted;
}

static int a4sqp_c_try_active_bound_restoration(
	struct A4SqpCSolve *solve,
	double *x,
	const struct A4SqpLineSearchOptions *base_line_options,
	const struct A4SqpVectorLineSearchOps *line_ops,
	void *line_ctx
){
	struct A4SqpProblemInfo *p;
	struct A4SqpLineSearchOptions line_options;
	struct A4SqpLineSearchResult result;
	double *old_x = NULL;
	double old_obj;
	double old_kkt;
	double old_vio;
	double best_score = 0.0;
	int require_kkt_improvement;
	int best_col = -1;
	double best_target = 0.0;
	int i;
	int accepted = 0;
	if(
		solve == NULL
		|| solve->problem == NULL
		|| x == NULL
		|| base_line_options == NULL
		|| line_ops == NULL
		|| line_ops->evaluate == NULL
		|| !solve->has_objective
	){
		return 0;
	}
	p = solve->problem;
	if(
		!p->opt.restoration
		|| !p->opt.kkt_convergence
		|| p->n <= 0
		|| p->m <= 0
		|| p->stats.max_constraint_violation > p->opt.acceptable_tol
		|| p->stats.kkt_error <= p->opt.acceptable_tol
		|| solve->view.obj_gradient == NULL
	){
		return 0;
	}
	if(p->opt.kkt_convergence && p->stats.max_constraint_violation <= p->opt.acceptable_tol){
		return 0;
	}
	for(i = 0; i < p->n; ++i){
		double value = x[i];
		double grad = solve->view.obj_gradient[i];
		double lower = a4sqp_c_map_bound(p->x_l[i],p->opt.lower_inf,p->opt.upper_inf);
		double upper = a4sqp_c_map_bound(p->x_u[i],p->opt.lower_inf,p->opt.upper_inf);
		double score = 0.0;
		double target = value;
		if(!isfinite(value) || !isfinite(grad)){
			continue;
		}
		if(!a4sqp_c_is_lower_inf(lower) && value > lower + p->opt.acceptable_tol && grad > p->opt.feas_tol){
			score = grad * (value - lower);
			target = lower;
		}
		if(!a4sqp_c_is_upper_inf(upper) && value < upper - p->opt.acceptable_tol && grad < -p->opt.feas_tol){
			double upper_score = (-grad) * (upper - value);
			if(upper_score > score){
				score = upper_score;
				target = upper;
			}
		}
		if(score > best_score){
			best_score = score;
			best_col = i;
			best_target = target;
		}
	}
	if(best_col < 0 || best_score <= p->opt.acceptable_tol || !isfinite(best_score)){
		return 0;
	}
	if(p->opt.verbosity > 0){
		fprintf(stderr,
			"A4SQP active-bound probe: col=%d value=%.17g target=%.17g score=%.17g obj=%.17g kkt=%.17g vio=%.17g\n",
			best_col,
			x[best_col],
			best_target,
			best_score,
			solve->view.obj_value,
			p->stats.kkt_error,
			p->stats.max_constraint_violation
		);
	}
	old_x = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
	if(old_x == NULL){
		return 0;
	}
	memcpy(old_x,x,(size_t)p->n * sizeof(*old_x));
	old_obj = solve->view.obj_value;
	old_kkt = p->stats.kkt_error;
	old_vio = p->stats.max_constraint_violation;
	require_kkt_improvement = p->opt.kkt_convergence && old_vio <= p->opt.acceptable_tol;
	x[best_col] = best_target;
	if(a4sqp_c_build_view(solve,x,A4SQP_TRUE) || solve->callback_error){
		solve->callback_error = 0;
		goto cleanup;
	}
	line_options = *base_line_options;
	line_options.restoration = 1;
	line_options.restoration_margin = p->opt.restoration_margin;
	memset(&result,0,sizeof(result));
	for(i = 0; i < 200; ++i){
		if(a4sqp_core_nonlinear_restoration_step(
			&solve->view,
			&line_options,
			line_ops,
			line_ctx,
			x,
			solve->trust_radius,
			&result
		)){
			break;
		}
		a4sqp_c_refresh_stats(solve,p->stats.iterations);
		if(p->stats.max_constraint_violation <= p->opt.acceptable_tol){
			break;
		}
	}
	a4sqp_c_refresh_stats(solve,p->stats.iterations);
	if(
		isfinite(p->stats.max_constraint_violation)
		&& result.accepted
		&& (!require_kkt_improvement || p->stats.kkt_error < old_kkt)
		&& p->stats.max_constraint_violation <= fmax(
			p->opt.acceptable_tol,
			fmin(10.0,sqrt(best_score))
		)
			&& solve->view.obj_value <= old_obj - fmax(p->opt.acceptable_tol,1e-12)
			&& (
				p->stats.max_constraint_violation <= p->opt.acceptable_tol
				||
				p->stats.kkt_error < old_kkt
				|| solve->view.obj_value <= old_obj - 0.1 * best_score
				|| p->stats.max_constraint_violation <= old_vio
		)
	){
		solve->last_alpha = result.alpha;
		solve->last_step_norm = result.step_norm;
		solve->last_scaled_step_inf = result.scaled_step_inf;
		solve->last_trust_ratio = result.trust_ratio;
		solve->last_ls_trials = result.trials;
		accepted = 1;
	}
	if(p->opt.verbosity > 0){
		fprintf(stderr,
			"A4SQP active-bound probe: %s obj=%.17g kkt=%.17g vio=%.17g step=%.17g trials=%d\n",
			accepted ? "accepted" : "rejected",
			solve->view.obj_value,
			p->stats.kkt_error,
			p->stats.max_constraint_violation,
			result.step_norm,
			result.trials
		);
	}

cleanup:
	if(!accepted){
		memcpy(x,old_x,(size_t)p->n * sizeof(*x));
		(void)a4sqp_c_build_view(solve,x,A4SQP_TRUE);
		a4sqp_c_refresh_stats(solve,p->stats.iterations);
	}
	A4SQP_FREE(old_x);
	return accepted;
}

static int a4sqp_c_try_active_bound_release(
	struct A4SqpCSolve *solve,
	double *x,
	const struct A4SqpLineSearchOptions *base_line_options,
	const struct A4SqpVectorLineSearchOps *line_ops,
	void *line_ctx
){
	struct A4SqpProblemInfo *p;
	struct A4SqpLineSearchOptions line_options;
	struct A4SqpLineSearchResult result;
	double *old_x = NULL;
	double old_kkt;
	double old_vio;
	double lower;
	double upper;
	double value;
	double grad;
	double direction = 0.0;
	double range;
	double base_step;
	int col;
	int trial;
	int accepted = 0;
	if(
		solve == NULL
		|| solve->problem == NULL
		|| x == NULL
		|| base_line_options == NULL
		|| line_ops == NULL
		|| line_ops->evaluate == NULL
		|| !solve->has_objective
	){
		return 0;
	}
	p = solve->problem;
	if(
		!p->opt.restoration
		|| !p->opt.kkt_convergence
		|| !p->opt.active_bound_release
		|| p->n <= 0
		|| p->m <= 0
		|| p->stats.max_constraint_violation > p->opt.acceptable_tol
		|| p->stats.kkt_error <= p->opt.acceptable_tol
		|| p->stats.bound_stationarity_inf <= p->opt.acceptable_tol
	){
		return 0;
	}
	col = p->stats.bound_worst_index;
	if(col < 0 || col >= p->n){
		return 0;
	}
	value = x[col];
	grad = p->stats.bound_worst_lagrangian_gradient;
	lower = a4sqp_c_map_bound(p->x_l[col],p->opt.lower_inf,p->opt.upper_inf);
	upper = a4sqp_c_map_bound(p->x_u[col],p->opt.lower_inf,p->opt.upper_inf);
	if(!isfinite(value) || !isfinite(grad)){
		return 0;
	}
	if(!a4sqp_c_is_lower_inf(lower) && value <= lower + p->opt.feas_tol && grad < -p->opt.acceptable_tol){
		direction = 1.0;
	}else if(!a4sqp_c_is_upper_inf(upper) && value >= upper - p->opt.feas_tol && grad > p->opt.acceptable_tol){
		direction = -1.0;
	}else{
		if(p->opt.verbosity > 0){
			fprintf(stderr,
				"A4SQP active-bound release: skipped col=%d value=%.17g lower=%.17g upper=%.17g grad=%.17g stationarity=%.17g\n",
				col,
				value,
				lower,
				upper,
				grad,
				p->stats.bound_stationarity_inf
			);
		}
		return 0;
	}
	range = (!a4sqp_c_is_lower_inf(lower) && !a4sqp_c_is_upper_inf(upper) && upper > lower)
		? upper - lower
		: 0.0;
	base_step = range > 0.0 ? 0.05 * range : 0.05;
	if(solve->trust_radius > 0.0 && solve->view.var_scale != NULL && solve->view.var_scale[col] > 0.0){
		double trust_step = 0.25 * solve->trust_radius * solve->view.var_scale[col];
		if(trust_step > 0.0 && trust_step < base_step){
			base_step = trust_step;
		}
	}
	if(!isfinite(base_step) || base_step <= 0.0){
		return 0;
	}
	if(p->opt.verbosity > 0){
		fprintf(stderr,
			"A4SQP active-bound release: col=%d value=%.17g grad=%.17g direction=%.0f kkt=%.17g vio=%.17g\n",
			col,
			value,
			grad,
			direction,
			p->stats.kkt_error,
			p->stats.max_constraint_violation
		);
	}
	old_x = A4SQP_NEW_ARRAY_OR_NULL(double,p->n);
	if(old_x == NULL){
		return 0;
	}
	memcpy(old_x,x,(size_t)p->n * sizeof(*old_x));
	old_kkt = p->stats.kkt_error;
	old_vio = p->stats.max_constraint_violation;
	line_options = *base_line_options;
	line_options.restoration = 1;
	line_options.restoration_margin = p->opt.restoration_margin;
	for(trial = 0; trial < 8 && !accepted; ++trial){
		double step = base_step / pow(2.0,(double)trial);
		double target = old_x[col] + direction * step;
		int iter;
		if(!a4sqp_c_is_lower_inf(lower) && target < lower + p->opt.bound_push){
			target = lower + p->opt.bound_push;
		}
		if(!a4sqp_c_is_upper_inf(upper) && target > upper - p->opt.bound_push){
			target = upper - p->opt.bound_push;
		}
		if(target == old_x[col] || !isfinite(target)){
			continue;
		}
		memcpy(x,old_x,(size_t)p->n * sizeof(*x));
		x[col] = target;
		if(a4sqp_c_build_view(solve,x,A4SQP_TRUE) || solve->callback_error){
			solve->callback_error = 0;
			continue;
		}
		memset(&result,0,sizeof(result));
		for(iter = 0; iter < 30; ++iter){
			if(a4sqp_core_nonlinear_restoration_step(
				&solve->view,
				&line_options,
				line_ops,
				line_ctx,
				x,
				solve->trust_radius,
				&result
			)){
				break;
			}
			a4sqp_c_refresh_stats(solve,p->stats.iterations);
			if(p->stats.max_constraint_violation <= p->opt.acceptable_tol){
				break;
			}
		}
		a4sqp_c_refresh_stats(solve,p->stats.iterations);
		if(
			result.accepted
			&& p->stats.max_constraint_violation <= fmax(p->opt.acceptable_tol,old_vio)
			&& p->stats.kkt_error < old_kkt - fmax(p->opt.feas_tol,1e-12)
		){
			solve->last_alpha = result.alpha;
			solve->last_step_norm = fabs(x[col] - old_x[col]);
			solve->last_scaled_step_inf = solve->view.var_scale != NULL && solve->view.var_scale[col] > 0.0
				? solve->last_step_norm / solve->view.var_scale[col]
				: solve->last_step_norm;
			solve->last_trust_ratio = result.trust_ratio;
			solve->last_ls_trials = result.trials;
			accepted = 1;
		}
	}
	if(!accepted){
		memcpy(x,old_x,(size_t)p->n * sizeof(*x));
		(void)a4sqp_c_build_view(solve,x,A4SQP_TRUE);
		a4sqp_c_refresh_stats(solve,p->stats.iterations);
	}
	if(p->opt.verbosity > 0){
		fprintf(stderr,
			"A4SQP active-bound release: %s kkt=%.17g vio=%.17g\n",
			accepted ? "accepted" : "rejected",
			p->stats.kkt_error,
			p->stats.max_constraint_violation
		);
	}
	A4SQP_FREE(old_x);
	return accepted;
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

static int a4sqp_c_acceptability_satisfied(struct A4SqpCSolve *solve){
	double maxvio = 0.0;
	double pg = 0.0;
	struct A4SqpConvergencePolicy policy;
	if(solve == NULL || solve->problem == NULL || solve->problem->opt.acceptable_iter <= 0){
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
		return maxvio <= policy.feas_tol && solve->problem->stats.kkt_error <= policy.feas_tol;
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

struct A4SqpCCoreStepCtx {
	struct A4SqpCSolve *solve;
	double *x;
	int hessian_memory_error;
	int force_bfgs_fallback;
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
	solve->using_bfgs_fallback = 0;
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
	if(
		a4sqp_c_uses_exact_hessian(p)
		&& p->opt.hess_fallback_ratio > 0.0
		&& solve->hess.last_reg > 0.0
		&& solve->exact_hess_reg_ratio >= p->opt.hess_fallback_ratio
		&& solve->bfgs_hess.dense != NULL
		&& solve->bfgs_hess.n == p->n
	){
		solve->using_bfgs_fallback = 1;
		++solve->bfgs_fallback_count;
		step_hess->dense = solve->bfgs_hess.dense;
	}else if(
		ctx->force_bfgs_fallback
		&& a4sqp_c_uses_exact_hessian(p)
		&& solve->bfgs_hess.dense != NULL
		&& solve->bfgs_hess.n == p->n
	){
		solve->using_bfgs_fallback = 1;
		++solve->bfgs_fallback_count;
		step_hess->dense = solve->bfgs_hess.dense;
	}else{
		step_hess->dense = solve->hess.dense;
	}
	return 0;
}

static int a4sqp_c_core_solve_qp(void *vctx, struct A4SqpQp *qp){
	struct A4SqpCCoreStepCtx *ctx = (struct A4SqpCCoreStepCtx *)vctx;
	struct A4SqpQpSolveOptions options;
	if(ctx == NULL || ctx->solve == NULL || ctx->solve->problem == NULL){
		return 1;
	}
	memset(&options,0,sizeof(options));
	options.tolerance = ctx->solve->problem->opt.feas_tol;
	options.output_flag = 0;
	options.time_limit = ctx->solve->problem->opt.qp_time_limit;
	options.iteration_limit = ctx->solve->problem->opt.qp_iteration_limit;
	return a4sqp_qp_solve_highs_options(qp,&options) == 0 ? 0 : 1;
}

static void a4sqp_c_core_after_qp_solve(void *vctx, const struct A4SqpQp *qp, int restoration){
	struct A4SqpCCoreStepCtx *ctx = (struct A4SqpCCoreStepCtx *)vctx;
	struct A4SqpCSolve *solve;
	struct A4SqpProblemInfo *p;
	int row;
	if(ctx == NULL || ctx->solve == NULL || qp == NULL){
		return;
	}
	solve = ctx->solve;
	p = solve->problem;
	if(!restoration){
		struct A4SqpCoreMultiplierOptions options;
		for(row = 0; row < p->m; ++row){
			solve->lambda[row] = (qp->row_dual != NULL && row < qp->num_row)
				? qp->row_dual[row]
				: 0.0;
			}
			options.feas_tol = p->opt.feas_tol;
			options.elastic_penalty = solve->elastic_penalty;
			options.row_dual_scale = solve->view.rel_scale;
			(void)a4sqp_core_multiplier_estimate_update_view(
				&solve->lambda_est,
			&solve->view,
			qp,
			&options
		);
	}
	solve->last_elastic_max = a4sqp_core_qp_elastic_max(qp);
	solve->last_linearized_violation = a4sqp_core_qp_elastic_sum(qp);
	solve->elastic_penalty_saturated = 0;
	for(row = 0; row < qp->num_row; ++row){
		double dual = (qp->row_dual != NULL) ? fabs(qp->row_dual[row]) : 0.0;
		if(dual >= 0.95 * solve->elastic_penalty){
			solve->elastic_penalty_saturated = 1;
			break;
		}
	}
}

static void a4sqp_c_refresh_stats(struct A4SqpCSolve *solve, int iterations){
	double maxvio = 0.0;
	if(solve == NULL || solve->problem == NULL){
		return;
	}
	solve->problem->stats.iterations = iterations;
	solve->problem->stats.objective = solve->view.obj_value;
	solve->problem->stats.final_step_norm = solve->last_step_norm;
	solve->problem->stats.final_trust_radius = solve->trust_radius;
	solve->problem->stats.final_elastic_max = solve->last_elastic_max;
	a4sqp_c_view_violation(&solve->view,&maxvio);
	solve->problem->stats.max_constraint_violation = maxvio;
	solve->problem->stats.projected_gradient_inf = a4sqp_c_projected_gradient_inf(solve);
	a4sqp_c_update_kkt_stats(solve);
}

static void a4sqp_c_accumulate_core_step_stats(
	struct A4SqpProblemInfo *p,
	const struct A4SqpCoreStepStats *step_stats
){
	if(p == NULL || step_stats == NULL){
		return;
	}
	p->stats.qp_solves += step_stats->qp_solves;
	p->stats.qp_failures += step_stats->qp_failures;
	p->stats.line_search_failures += step_stats->line_search_failures;
	p->stats.algorithm_mode = step_stats->phase == A4SQP_CORE_PHASE_RESTORATION
		? A4SqpRestorationPhaseMode
		: A4SqpRegularMode;
	p->stats.mode_switches += step_stats->phase_changed;
	p->stats.regular_iterations += step_stats->regular_iterations;
	p->stats.restoration_iterations += step_stats->restoration_iterations;
	p->stats.restoration_entries += step_stats->restoration_entries;
	p->stats.restoration_exits += step_stats->restoration_exits;
	p->stats.restoration_handoffs += step_stats->restoration_handoffs;
}

static enum A4SqpApplicationReturnStatus a4sqp_c_solve_impl(struct A4SqpCSolve *solve, double *x){
	struct A4SqpProblemInfo *p = solve->problem;
	struct A4SqpTrustOptions trust_opt;
	int iter;
	a4sqp_c_project_x_to_bounds(p,x);
	if(a4sqp_c_build_view(solve,x,A4SQP_TRUE)){
		return A4SqpInsufficientMemory;
	}
	if(solve->callback_error){
		return A4SqpInvalidNumberDetected;
	}
	if(a4sqp_c_hess_reset_identity(solve,1.0)){
		return A4SqpInsufficientMemory;
	}
	if(a4sqp_dense_hessian_reset_identity(&solve->bfgs_hess,p->n,1.0)){
		return A4SqpInsufficientMemory;
	}
	solve->elastic_penalty = p->opt.elastic_penalty;
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
		a4sqp_c_trust_options(&p->opt,&step_options.trust);
		step_options.trust_tiny_alpha = p->opt.trust_tiny_alpha;
		step_options.trust_tiny_radius_factor = p->opt.trust_tiny_radius_factor;
		step_options.elastic_penalty = solve->elastic_penalty;
		step_options.feas_tol = p->opt.feas_tol;
		step_options.merit_tol = p->opt.merit_tol;
		step_options.restoration.enable = p->opt.restoration;
		step_options.restoration.trigger_iter = p->opt.restoration_trigger_iter;
		step_options.restoration.max_iter = p->opt.restoration_max_iter;
		step_options.restoration.improve = p->opt.restoration_improve;
		step_options.restoration.margin = p->opt.restoration_margin;
		step_options.restoration.handoff_reduction = p->opt.restoration_handoff_reduction;
		step_options.restoration.reentry_factor = p->opt.restoration_reentry_factor;
		step_options.restoration_state = &solve->restoration_state;
		line_options.max_backtrack = p->opt.max_backtrack;
		line_options.merit_tol = p->opt.merit_tol;
		line_options.feas_tol = p->opt.feas_tol;
		line_options.step_tol = p->opt.step_tol;
		line_options.armijo_coeff = p->opt.armijo_coeff;
		line_options.trust_accept = p->opt.trust_accept;
		line_options.elastic_penalty = solve->elastic_penalty;
		line_options.filter_accept = p->opt.filter_accept;
		line_options.filter_margin = p->opt.filter_margin;
		line_ops.evaluate = a4sqp_c_ls_evaluate;
		line_ops.accepted = a4sqp_c_ls_accepted;
		step_ops.prepare_hessian = a4sqp_c_core_prepare_hessian;
		step_ops.solve_qp = a4sqp_c_core_solve_qp;
		step_ops.after_qp_solve = a4sqp_c_core_after_qp_solve;
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
		a4sqp_c_accumulate_core_step_stats(p,&step_stats);
		if(step_status != A4SQP_CORE_STEP_ACCEPTED){
			if(
				step_status == A4SQP_CORE_STEP_LINE_SEARCH_ERROR
				&& a4sqp_c_uses_exact_hessian(p)
				&& solve->bfgs_hess.dense != NULL
				&& solve->bfgs_hess.n == p->n
			){
				(void)a4sqp_dense_hessian_reset_identity(&solve->bfgs_hess,p->n,1.0);
				memset(&step_stats,0,sizeof(step_stats));
				memset(&line_result,0,sizeof(line_result));
				step_ctx.force_bfgs_fallback = 1;
				step_ctx.hessian_memory_error = 0;
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
				a4sqp_c_accumulate_core_step_stats(p,&step_stats);
			}
		}
		if(step_status != A4SQP_CORE_STEP_ACCEPTED){
			a4sqp_c_refresh_stats(solve,iter + 1);
			if(a4sqp_c_has_converged(solve)){
				return A4SqpSolveSucceeded;
			}
			if(a4sqp_c_acceptability_satisfied(solve)){
				return A4SqpSolvedToAcceptableLevel;
			}
			if(step_status == A4SQP_CORE_STEP_HESSIAN_ERROR && solve->callback_error){
				return A4SqpInvalidNumberDetected;
			}
			if(step_status == A4SQP_CORE_STEP_HESSIAN_ERROR && step_ctx.hessian_memory_error){
				return A4SqpInsufficientMemory;
			}
			return A4SqpErrorInStepComputation;
		}
		++solve->accepted_step_count;
		a4sqp_c_refresh_stats(solve,iter + 1);
		if(a4sqp_c_try_stationarity_correction(solve,x)){
			a4sqp_c_refresh_stats(solve,iter + 1);
		}
		if(a4sqp_c_try_reduced_gradient_polish(solve,x,&line_options,&line_ops,&line_ctx)){
			a4sqp_c_refresh_stats(solve,iter + 1);
		}
		if(a4sqp_c_try_active_bound_release(solve,x,&line_options,&line_ops,&line_ctx)){
			a4sqp_c_refresh_stats(solve,iter + 1);
		}
		if(p->opt.active_bound_restoration && a4sqp_c_try_active_bound_restoration(solve,x,&line_options,&line_ops,&line_ctx)){
			a4sqp_c_refresh_stats(solve,iter + 1);
		}
		maxvio = p->stats.max_constraint_violation;
		if(
			solve->elastic_penalty_saturated
			&& maxvio > p->opt.feas_tol
			&& solve->elastic_penalty < p->opt.elastic_penalty_max
		){
			double next_penalty = solve->elastic_penalty * p->opt.elastic_penalty_growth;
			if(next_penalty > p->opt.elastic_penalty_max || !isfinite(next_penalty)){
				next_penalty = p->opt.elastic_penalty_max;
			}
			if(next_penalty > solve->elastic_penalty){
				solve->elastic_penalty = next_penalty;
			}
		}
		if(p->intermediate_cb != NULL){
			if(!p->intermediate_cb(
				solve->restoration_state.active ? A4SqpRestorationPhaseMode : A4SqpRegularMode,
				p->stats.iterations,
				solve->view.obj_value,
				p->stats.max_constraint_violation,
				p->stats.dual_infeasibility_inf,
				p->stats.complementarity_inf,
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

static void a4sqp_c_solve_state_init(
	struct A4SqpCSolve *solve,
	struct A4SqpProblemInfo *problem,
	A4SqpUserDataPtr user_data
){
	memset(solve,0,sizeof(*solve));
	solve->problem = problem;
	solve->user_data = user_data;
	solve->has_objective = problem != NULL && problem->eval_f != NULL;
	a4sqp_dense_hessian_init(&solve->hess);
	a4sqp_dense_hessian_init(&solve->bfgs_hess);
	a4sqp_view_init(&solve->view);
	a4sqp_qp_init(&solve->qp);
	a4sqp_core_multiplier_estimate_init(&solve->lambda_est);
}

static void a4sqp_c_solve_state_destroy(struct A4SqpCSolve *solve){
	if(solve == NULL){
		return;
	}
	a4sqp_dense_hessian_destroy(&solve->hess);
	a4sqp_dense_hessian_destroy(&solve->bfgs_hess);
	A4SQP_FREE(solve->lambda);
	solve->lambda = NULL;
	a4sqp_core_multiplier_estimate_destroy(&solve->lambda_est);
	a4sqp_qp_destroy(&solve->qp);
	a4sqp_view_destroy(&solve->view);
}

static int a4sqp_c_should_restart_with_bfgs(
	const struct A4SqpProblemInfo *p,
	enum A4SqpApplicationReturnStatus status
){
	if(p == NULL || !a4sqp_c_uses_exact_hessian(p) || p->opt.hess_fallback_ratio <= 0.0){
		return 0;
	}
	if(status == A4SqpErrorInStepComputation){
		return 1;
	}
	if(status != A4SqpMaximumIterationsExceeded){
		return 0;
	}
	return p->stats.kkt_error > p->opt.acceptable_tol
		|| p->stats.max_constraint_violation > p->opt.acceptable_tol;
}

static int a4sqp_c_should_restart_with_strict_row_scaling(
	const struct A4SqpProblemInfo *p,
	const char *original_scaleopt,
	enum A4SqpApplicationReturnStatus status
){
	if(p == NULL || original_scaleopt == NULL || !a4sqp_c_streq(original_scaleopt,"AUTO")){
		return 0;
	}
	if(p->x_scale != NULL || p->g_scale != NULL){
		return 0;
	}
	return status != A4SqpSolveSucceeded
		&& status != A4SqpSolvedToAcceptableLevel;
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
	A4SqpNumber *x_initial = NULL;
	char original_hessian[32];
	char original_scaleopt[32];
	int exact_restart_allowed;
	int i;
	if(p == NULL || (p->n > 0 && x == NULL)){
		return A4SqpInvalidProblemDefinition;
	}
	original_hessian[0] = '\0';
	original_scaleopt[0] = '\0';
	strcpy(original_hessian,p->opt.hessian);
	strcpy(original_scaleopt,p->opt.scaleopt);
	exact_restart_allowed = a4sqp_c_uses_exact_hessian(p);
	if(p->n > 0){
		x_initial = A4SQP_NEW_ARRAY_OR_NULL(A4SqpNumber,p->n);
		if(x_initial == NULL){
			return A4SqpInsufficientMemory;
		}
		memcpy(x_initial,x,(size_t)p->n * sizeof(*x_initial));
	}
	memset(&p->stats,0,sizeof(p->stats));
	if(a4sqp_c_streq(original_scaleopt,"AUTO")){
		strcpy(p->opt.scaleopt,"ROW_2NORM_T5");
	}
	a4sqp_c_solve_state_init(&solve,p,user_data);
	status = a4sqp_c_solve_impl(&solve,x);
	if(
		a4sqp_c_should_restart_with_strict_row_scaling(p,original_scaleopt,status)
		&& x_initial != NULL
	){
		a4sqp_c_solve_state_destroy(&solve);
		memcpy(x,x_initial,(size_t)p->n * sizeof(*x));
		memset(&p->stats,0,sizeof(p->stats));
		strcpy(p->opt.hessian,original_hessian);
		strcpy(p->opt.scaleopt,"ROW_2NORM");
		a4sqp_c_solve_state_init(&solve,p,user_data);
		status = a4sqp_c_solve_impl(&solve,x);
	}
	if(
		exact_restart_allowed
		&& a4sqp_c_should_restart_with_bfgs(p,status)
		&& x_initial != NULL
	){
		a4sqp_c_solve_state_destroy(&solve);
		memcpy(x,x_initial,(size_t)p->n * sizeof(*x));
		memset(&p->stats,0,sizeof(p->stats));
		strcpy(p->opt.hessian,"BFGS");
		a4sqp_c_solve_state_init(&solve,p,user_data);
		status = a4sqp_c_solve_impl(&solve,x);
		strcpy(p->opt.hessian,original_hessian);
	}else{
		strcpy(p->opt.hessian,original_hessian);
	}
	strcpy(p->opt.scaleopt,original_scaleopt);
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
	a4sqp_c_solve_state_destroy(&solve);
	A4SQP_FREE(x_initial);
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
