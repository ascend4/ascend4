/*
 * CUTEst driver for the IPOPT-like A4SQP C API.
 *
 * This file is intended to be built by CUTEst's package mechanism. It does not
 * participate in normal ASCEND builds.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GENCMA

#ifdef __cplusplus
extern "C" {
#endif

#include "cutest.h"
#include "cutest_routines.h"
#include "solvers/a4sqp/a4sqp_c.h"
#include "solvers/a4sqp/a4sqp_lsq.h"

void a4sqp_cutest_lsq_dim(int n, int *nres, int *max_row_nnz, int *status);
void a4sqp_cutest_lsq_weights(int nres, double *weights, int *status);
void a4sqp_cutest_lsq_residuals(int n, const double *x, int nres, double *residuals, int *status);
void a4sqp_cutest_lsq_jacobian_row(
	int n,
	const double *x,
	int row,
	int capacity,
	int *columns,
	double *values,
	int *nnz,
	int *status
);

struct A4SqpCutestContext {
	integer n;
	integer m;
	integer nele_jac;
	integer nele_hess;
	logical constrained;
	logical noobj;
	rp_ *c_work;
	rp_ *g_work;
	rp_ *hess_work;
	rp_ *hess_x_work;
	rp_ *lambda_work;
	integer *hess_row;
	integer *hess_col;
	int lsq_nres;
	int lsq_max_row_nnz;
	int lsq_probe_status;
	int lsq_status;
	int lsq_handoff_improved;
	struct A4SqpLsqStats lsq_stats;
	double *lsq_weights;
};

static A4SqpBool a4sqp_cutest_eval_f(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *obj_value,
	A4SqpUserDataPtr user_data
);

static void a4sqp_cutest_json_string(const char *s){
	const unsigned char *p = (const unsigned char *)(s != NULL ? s : "");
	putchar('"');
	while(*p != '\0'){
		if(*p == '"' || *p == '\\'){
			putchar('\\');
			putchar((int)*p);
		}else if(*p >= 32 && *p < 127){
			putchar((int)*p);
		}else{
			printf("\\u%04x",(unsigned int)*p);
		}
		++p;
	}
	putchar('"');
}

static void a4sqp_cutest_json_number(double value){
	if(isfinite(value)){
		printf("%.17g",value);
	}else{
		printf("null");
	}
}

static double a4sqp_cutest_inf_norm(integer n, const rp_ *v){
	double result = 0.0;
	integer i;
	if(v == NULL){
		return NAN;
	}
	for(i = 0; i < n; ++i){
		double a = fabs((double)v[i]);
		if(a > result){
			result = a;
		}
	}
	return result;
}

static double a4sqp_cutest_bound_projected_grad_inf(
	integer n,
	const rp_ *x,
	const rp_ *x_l,
	const rp_ *x_u,
	const rp_ *grad
){
	double result = 0.0;
	integer i;
	const double active_tol = 1e-8;
	if(x == NULL || grad == NULL){
		return NAN;
	}
	for(i = 0; i < n; ++i){
		double g = (double)grad[i];
		double lower = x_l != NULL ? (double)x_l[i] : -INFINITY;
		double upper = x_u != NULL ? (double)x_u[i] : INFINITY;
		int at_lower = isfinite(lower) && (double)x[i] <= lower + active_tol;
		int at_upper = isfinite(upper) && (double)x[i] >= upper - active_tol;
		if((at_lower && g > 0.0) || (at_upper && g < 0.0)){
			g = 0.0;
		}
		if(fabs(g) > result){
			result = fabs(g);
		}
	}
	return result;
}

static int a4sqp_cutest_final_objective_gradient(
	struct A4SqpCutestContext *ctx,
	const rp_ *x,
	rp_ *obj,
	rp_ *grad_out
){
	integer status = 0;
	logical grad = TRUE_;
	rp_ f = 0.0;
	if(ctx == NULL || x == NULL || obj == NULL || grad_out == NULL){
		return 1;
	}
	if(ctx->noobj){
		memset(grad_out,0,(size_t)ctx->n * sizeof(*grad_out));
		*obj = 0.0;
		return 0;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,x,&f,grad_out,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,x,&f,grad_out,&grad);
	}
	if(status != 0){
		return 1;
	}
	*obj = f;
	return 0;
}

static void a4sqp_cutest_print_x_if_requested(const char *env_name, integer n, const rp_ *x){
	integer i;
	const char *dump = getenv(env_name);
	if(dump == NULL || *dump == '\0' || strcmp(dump,"0") == 0){
		return;
	}
	printf(",\"final_x\":[");
	for(i = 0; i < n; ++i){
		if(i > 0){
			printf(",");
		}
		a4sqp_cutest_json_number((double)x[i]);
	}
	printf("]");
}

static int a4sqp_cutest_env_int(const char *name, int fallback){
	const char *value = getenv(name);
	char *end = NULL;
	long parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtol(value,&end,10);
	if(end == value){
		return fallback;
	}
	return (int)parsed;
}

static double a4sqp_cutest_env_double(const char *name, double fallback){
	const char *value = getenv(name);
	char *end = NULL;
	double parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtod(value,&end);
	if(end == value || !isfinite(parsed)){
		return fallback;
	}
	return parsed;
}

static const char *a4sqp_cutest_env_string(const char *name, const char *fallback){
	const char *value = getenv(name);
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	return value;
}

static int a4sqp_cutest_option_env_name(const char *keyword, char *buffer, size_t capacity){
	static const char prefix[] = "A4SQP_";
	size_t i;
	size_t pos = 0;
	if(keyword == NULL || buffer == NULL || capacity == 0){
		return 1;
	}
	for(i = 0; prefix[i] != '\0'; ++i){
		if(pos + 1 >= capacity){
			return 1;
		}
		buffer[pos++] = prefix[i];
	}
	for(i = 0; keyword[i] != '\0'; ++i){
		char ch = keyword[i];
		if(pos + 1 >= capacity){
			return 1;
		}
		if(ch >= 'a' && ch <= 'z'){
			ch = (char)(ch - 'a' + 'A');
		}
		buffer[pos++] = ch;
	}
	buffer[pos] = '\0';
	return 0;
}

static const char *a4sqp_cutest_env_for_option(const struct A4SqpOptionInfo *info, char *buffer, size_t capacity){
	const char *value;
	if(info == NULL || a4sqp_cutest_option_env_name(info->keyword,buffer,capacity)){
		return NULL;
	}
	value = getenv(buffer);
	if(value != NULL && *value != '\0'){
		return value;
	}
	/* Compatibility aliases used by older A4SQP CUTEst scripts. */
	if(strcmp(info->keyword,"feas_tol") == 0 || strcmp(info->keyword,"step_tol") == 0){
		value = getenv("A4SQP_TOL");
		if(value != NULL && *value != '\0'){
			return value;
		}
	}
	if(strcmp(info->keyword,"verbosity") == 0){
		value = getenv("A4SQP_PRINT_LEVEL");
		if(value != NULL && *value != '\0'){
			return value;
		}
	}
	return NULL;
}

static int a4sqp_cutest_parse_env_int(const char *value, int fallback){
	char *end = NULL;
	long parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtol(value,&end,10);
	if(end == value){
		return fallback;
	}
	return (int)parsed;
}

static double a4sqp_cutest_parse_env_double(const char *value, double fallback){
	char *end = NULL;
	double parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtod(value,&end);
	if(end == value || !isfinite(parsed)){
		return fallback;
	}
	return parsed;
}

static A4SqpBool a4sqp_cutest_apply_core_options(A4SqpProblem problem){
	A4SqpIndex i;
	A4SqpIndex count;
	if(problem == NULL){
		return A4SQP_FALSE;
	}
	count = GetA4SqpOptionCount();
	for(i = 0; i < count; ++i){
		struct A4SqpOptionInfo info;
		char env_name[128];
		const char *env_value;
		A4SqpBool ok = A4SQP_FALSE;
		if(!GetA4SqpOptionInfo(i,&info)){
			return A4SQP_FALSE;
		}
		if(!info.problem_option){
			continue;
		}
		env_value = a4sqp_cutest_env_for_option(&info,env_name,sizeof(env_name));
		switch(info.type){
		case A4SqpOptionInteger:
			ok = AddA4SqpIntOption(problem,(char *)info.keyword,
				a4sqp_cutest_parse_env_int(env_value,(int)info.default_number)
			);
			break;
		case A4SqpOptionBool:
			ok = AddA4SqpIntOption(problem,(char *)info.keyword,
				a4sqp_cutest_parse_env_int(env_value,info.default_number != 0.0)
			);
			break;
		case A4SqpOptionNumber:
			ok = AddA4SqpNumOption(problem,(char *)info.keyword,
				a4sqp_cutest_parse_env_double(env_value,info.default_number)
			);
			break;
		case A4SqpOptionString:
			ok = AddA4SqpStrOption(problem,(char *)info.keyword,
				(char *)(env_value != NULL ? env_value : info.default_string)
			);
			break;
		default:
			return A4SQP_FALSE;
		}
		if(!ok){
			fprintf(stderr,"A4SQP-CUTEst: invalid A4SQP option '%s'\n",info.keyword);
			return A4SQP_FALSE;
		}
	}
	return A4SQP_TRUE;
}

static int a4sqp_cutest_lsq_eval_residuals(void *userdata, const real64 *x, real64 *residuals){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)userdata;
	int status = 0;
	if(ctx == NULL || x == NULL || residuals == NULL || ctx->lsq_nres <= 0){
		return 1;
	}
	memcpy(ctx->hess_x_work,x,(size_t)ctx->n * sizeof(*x));
	a4sqp_cutest_lsq_residuals((int)ctx->n,(const double *)x,ctx->lsq_nres,(double *)residuals,&status);
	return status == 0 ? 0 : 1;
}

static int a4sqp_cutest_lsq_eval_jacobian_row(
	void *userdata,
	int32 row,
	int32 capacity,
	int32 *columns,
	real64 *values,
	int32 *nnz
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)userdata;
	int status = 0;
	int nnz_local = 0;
	if(ctx == NULL || row < 0 || row >= ctx->lsq_nres || capacity < ctx->lsq_max_row_nnz
		|| columns == NULL || values == NULL || nnz == NULL
	){
		return 1;
	}
	a4sqp_cutest_lsq_jacobian_row(
		(int)ctx->n,
		(const double *)ctx->hess_x_work,
		(int)row,
		(int)capacity,
		(int *)columns,
		(double *)values,
		&nnz_local,
		&status
	);
	if(status != 0){
		return 1;
	}
	*nnz = (int32)nnz_local;
	return 0;
}

static int a4sqp_cutest_try_lsq(
	struct A4SqpCutestContext *ctx,
	rp_ *x,
	rp_ *x_l,
	rp_ *x_u,
	const char *classification,
	rp_ *obj,
	struct A4SqpSolveStats *stats,
	enum A4SqpApplicationReturnStatus *solve_status
){
	const char *mode_name = a4sqp_cutest_env_string("A4SQP_TRY_LSQ","OFF");
	const char *fallback_start = a4sqp_cutest_env_string("A4SQP_LSQ_FALLBACK_START","ORIGINAL");
	int probe_status = 0;
	int keep_improved = 0;
	struct A4SqpLsqProblem problem;
	struct A4SqpLsqOptions options;
	struct A4SqpLsqStats lsq_stats;
	enum A4SqpLsqStatus lsq_status;
	rp_ *x_backup = NULL;

	if(ctx == NULL || x == NULL || x_l == NULL || x_u == NULL || stats == NULL || solve_status == NULL){
		return -1;
	}
	if(mode_name == NULL || strcmp(mode_name,"OFF") == 0 || strcmp(mode_name,"0") == 0){
		return -1;
	}
	if(ctx->constrained || ctx->noobj){
		return -1;
	}
	if(classification == NULL || classification[0] != 'S'){
		ctx->lsq_probe_status = -2;
		return -1;
	}
	a4sqp_cutest_lsq_dim((int)ctx->n,&ctx->lsq_nres,&ctx->lsq_max_row_nnz,&probe_status);
	ctx->lsq_probe_status = probe_status;
	if(probe_status != 0 || ctx->lsq_nres <= 0){
		return -1;
	}
	MALLOC(x_backup,ctx->n,rp_);
	MALLOC(ctx->lsq_weights,ctx->lsq_nres,double);
	if(x_backup == NULL || ctx->lsq_weights == NULL){
		FREE(x_backup);
		FREE(ctx->lsq_weights);
		ctx->lsq_weights = NULL;
		return 1;
	}
	memcpy(x_backup,x,(size_t)ctx->n * sizeof(*x_backup));
	a4sqp_cutest_lsq_weights(ctx->lsq_nres,ctx->lsq_weights,&probe_status);
	if(probe_status != 0){
		memcpy(x,x_backup,(size_t)ctx->n * sizeof(*x));
		FREE(x_backup);
		FREE(ctx->lsq_weights);
		ctx->lsq_weights = NULL;
		return -1;
	}

	memset(&problem,0,sizeof(problem));
	problem.n_var = (int32)ctx->n;
	problem.n_res = (int32)ctx->lsq_nres;
	problem.weights = ctx->lsq_weights;
	problem.x_lower = (const real64 *)x_l;
	problem.x_upper = (const real64 *)x_u;
	problem.userdata = ctx;
	problem.eval_residuals = a4sqp_cutest_lsq_eval_residuals;
	problem.eval_jacobian_row = a4sqp_cutest_lsq_eval_jacobian_row;

	memset(&options,0,sizeof(options));
	options.mode = strcmp(mode_name,"GAUSS") == 0 ? A4SQP_LSQ_MODE_GAUSS : A4SQP_LSQ_MODE_LM;
	options.max_iter = a4sqp_cutest_env_int(
		"A4SQP_LSQ_MAX_ITER",
		a4sqp_cutest_env_int("A4SQP_MAX_ITER",200)
	);
	options.max_backtrack = a4sqp_cutest_env_int("A4SQP_MAX_BACKTRACK",20);
	options.grad_tol = a4sqp_cutest_env_double("A4SQP_TOL",1e-7);
	options.step_tol = a4sqp_cutest_env_double("A4SQP_STEP_TOL",1e-8);

	memset(&lsq_stats,0,sizeof(lsq_stats));
	memcpy(ctx->hess_x_work,x,(size_t)ctx->n * sizeof(*x));
	lsq_status = a4sqp_lsq_solve(&problem,&options,(real64 *)x,&lsq_stats);
	ctx->lsq_status = (int)lsq_status;
	ctx->lsq_stats = lsq_stats;
	memcpy(ctx->hess_x_work,x,(size_t)ctx->n * sizeof(*x));
	memset(stats,0,sizeof(*stats));
	stats->iterations = lsq_stats.iterations;
	stats->projected_gradient_inf = lsq_stats.grad_inf;
	stats->kkt_error = lsq_stats.grad_inf;
	stats->dual_infeasibility_inf = lsq_stats.grad_inf;
	stats->final_step_norm = lsq_stats.step_norm;
	stats->regularization_size = lsq_stats.lambda;
	if(obj != NULL){
		A4SqpNumber obj_value = 0.0;
		if(a4sqp_cutest_eval_f((A4SqpIndex)ctx->n,(A4SqpNumber *)x,A4SQP_TRUE,&obj_value,ctx)){
			*obj = (rp_)obj_value;
		}else{
			*obj = (rp_)lsq_stats.objective;
		}
	}
	if(lsq_status == A4SQP_LSQ_SOLVED){
		*solve_status = A4SqpSolveSucceeded;
		FREE(x_backup);
		return 0;
	}
	keep_improved = (lsq_status == A4SQP_LSQ_MAX_ITER || lsq_status == A4SQP_LSQ_LINEAR_ERROR)
		&& fallback_start != NULL
		&& (
			strcmp(fallback_start,"IMPROVED") == 0
			|| strcmp(fallback_start,"improved") == 0
			|| strcmp(fallback_start,"1") == 0
		);
	if(!keep_improved){
		memcpy(x,x_backup,(size_t)ctx->n * sizeof(*x));
		memcpy(ctx->hess_x_work,x,(size_t)ctx->n * sizeof(*x));
	}else{
		ctx->lsq_handoff_improved = 1;
	}
	FREE(x_backup);
	fprintf(stderr,
		"A4SQP-CUTEst: least-squares attempt using %s did not converge "
		"(status=%d, iter=%d, obj=%.17g, grad=%.17g, step=%.17g, lambda=%.17g); "
		"falling back to SQP from %s point.\n",
		mode_name,
		(int)lsq_status,
		lsq_stats.iterations,
		(double)lsq_stats.objective,
		(double)lsq_stats.grad_inf,
		(double)lsq_stats.step_norm,
		(double)lsq_stats.lambda,
		keep_improved ? "improved" : "original"
	);
	return -1;
}

static A4SqpBool a4sqp_cutest_eval_f(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *obj_value,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical grad = FALSE_;
	rp_ f = 0.0;
	(void)n;
	(void)new_x;
	if(ctx == NULL || obj_value == NULL){
		return A4SQP_FALSE;
	}
	if(ctx->noobj){
		*obj_value = 0.0;
		return A4SQP_TRUE;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->g_work,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->g_work,&grad);
	}
	if(status != 0){
		return A4SQP_FALSE;
	}
	*obj_value = f;
	return A4SQP_TRUE;
}

static A4SqpBool a4sqp_cutest_eval_grad_f(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *grad_f,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical grad = TRUE_;
	rp_ f = 0.0;
	(void)n;
	(void)new_x;
	if(ctx == NULL || grad_f == NULL){
		return A4SQP_FALSE;
	}
	if(ctx->noobj){
		memset(grad_f,0,(size_t)ctx->n * sizeof(*grad_f));
		return A4SQP_TRUE;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)grad_f,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)grad_f,&grad);
	}
	return status == 0 ? A4SQP_TRUE : A4SQP_FALSE;
}

static A4SqpBool a4sqp_cutest_eval_g(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpNumber *g,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical jtrans = FALSE_;
	logical grad = FALSE_;
	integer one = 1;
	rp_ scratch = 0.0;
	(void)n;
	(void)new_x;
	(void)m;
	if(ctx == NULL || g == NULL){
		return A4SQP_FALSE;
	}
	if(ctx->m <= 0){
		return A4SQP_TRUE;
	}
	CUTEST_ccfg(&status,&ctx->n,&ctx->m,(const rp_ *)x,(rp_ *)g,&jtrans,&one,&one,&scratch,&grad);
	return status == 0 ? A4SQP_TRUE : A4SQP_FALSE;
}

static A4SqpBool a4sqp_cutest_eval_jac_g(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpIndex nele_jac,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical grad = TRUE_;
	integer nnzj = 0;
	(void)n;
	(void)new_x;
	(void)m;
	if(ctx == NULL || ctx->m <= 0 || nele_jac < ctx->nele_jac){
		return A4SQP_FALSE;
	}
	if(values == NULL){
		rp_ *jac = NULL;
		MALLOC(jac,ctx->nele_jac > 0 ? ctx->nele_jac : 1,rp_);
		if(jac == NULL){
			return A4SQP_FALSE;
		}
		CUTEST_ccfsg(
			&status,
			&ctx->n,
			&ctx->m,
			(const rp_ *)x,
			ctx->c_work,
			&nnzj,
			&ctx->nele_jac,
			jac,
			(integer *)jCol,
			(integer *)iRow,
			&grad
		);
		FREE(jac);
	}else{
		CUTEST_ccfsg(
			&status,
			&ctx->n,
			&ctx->m,
			(const rp_ *)x,
			ctx->c_work,
			&nnzj,
			&ctx->nele_jac,
			(rp_ *)values,
			(integer *)jCol,
			(integer *)iRow,
			&grad
		);
	}
	if(status != 0 || nnzj > ctx->nele_jac){
		return A4SQP_FALSE;
	}
	return A4SQP_TRUE;
}

static A4SqpBool a4sqp_cutest_eval_h(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber obj_factor,
	A4SqpIndex m,
	A4SqpNumber *lambda,
	A4SqpBool new_lambda,
	A4SqpIndex nele_hess,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	integer nnzh = 0;
	rp_ *x_use;
	(void)n;
	(void)new_x;
	(void)m;
	(void)new_lambda;
	if(ctx == NULL || nele_hess < ctx->nele_hess){
		return A4SQP_FALSE;
	}
	x_use = x != NULL ? (rp_ *)x : ctx->hess_x_work;
	if(!ctx->constrained){
		CUTEST_ush(
			&status,
			&ctx->n,
			x_use,
			&nnzh,
			&ctx->nele_hess,
			values == NULL ? ctx->hess_work : (rp_ *)values,
			ctx->hess_row,
			ctx->hess_col
		);
		if(status != 0){
			return A4SQP_FALSE;
		}
		if(values == NULL){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				iRow[k] = (A4SqpIndex)ctx->hess_row[k];
				jCol[k] = (A4SqpIndex)ctx->hess_col[k];
			}
		}else if(obj_factor != 1.0){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				values[k] *= obj_factor;
			}
		}
		return A4SQP_TRUE;
	}
	if(values == NULL){
		for(integer i = 0; i < ctx->m; ++i){
			ctx->lambda_work[i] = 0.0;
		}
		CUTEST_csh(
			&status,
			&ctx->n,
			&ctx->m,
			x_use,
			ctx->lambda_work,
			&nnzh,
			&ctx->nele_hess,
			ctx->hess_work,
			ctx->hess_row,
			ctx->hess_col
		);
		if(status != 0){
			return A4SQP_FALSE;
		}
		for(integer k = 0; k < ctx->nele_hess; ++k){
			iRow[k] = (A4SqpIndex)ctx->hess_row[k];
			jCol[k] = (A4SqpIndex)ctx->hess_col[k];
		}
		return A4SQP_TRUE;
	}
	if(obj_factor == 0.0){
		for(integer i = 0; i < ctx->m; ++i){
			ctx->lambda_work[i] = lambda != NULL ? (rp_)lambda[i] : 0.0;
		}
		CUTEST_cshc(
			&status,
			&ctx->n,
			&ctx->m,
			x_use,
			ctx->lambda_work,
			&nnzh,
			&ctx->nele_hess,
			(rp_ *)values,
			ctx->hess_row,
			ctx->hess_col
		);
	}else{
		for(integer i = 0; i < ctx->m; ++i){
			ctx->lambda_work[i] = lambda != NULL ? (rp_)(lambda[i] / obj_factor) : 0.0;
		}
		CUTEST_csh(
			&status,
			&ctx->n,
			&ctx->m,
			x_use,
			ctx->lambda_work,
			&nnzh,
			&ctx->nele_hess,
			(rp_ *)values,
			ctx->hess_row,
			ctx->hess_col
		);
		if(status == 0 && obj_factor != 1.0){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				values[k] *= obj_factor;
			}
		}
	}
	return status == 0 ? A4SQP_TRUE : A4SQP_FALSE;
}

int MAINENTRY(void){
	char *fname = "OUTSDIF.d";
	integer funit = 42;
	integer iout = 6;
	integer io_buffer = 11;
	integer status = 0;
	integer ierr = 0;
	integer e_order = 1;
	integer l_order = 0;
	integer v_order = 0;
	integer nnzj_raw = 0;
	integer nonlin_obj = 0;
	integer nonlin_con = 0;
	integer neq = 0;
	integer nlinear = 0;
	rp_ *x = NULL;
	rp_ *x_l = NULL;
	rp_ *x_u = NULL;
	rp_ *lambda = NULL;
	rp_ *g_l = NULL;
	rp_ *g_u = NULL;
	logical *equatn = NULL;
	logical *linear = NULL;
	rp_ obj = 0.0;
	rp_ *g = NULL;
	rp_ calls[7] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	rp_ cpu[4] = {0.0,0.0,0.0,0.0};
	char *pname = NULL;
	char *classification = NULL;
	struct A4SqpCutestContext ctx;
	A4SqpProblem problem = NULL;
	struct A4SqpSolveStats stats;
	enum A4SqpApplicationReturnStatus solve_status;
	rp_ final_obj_check = 0.0;
	rp_ *final_grad = NULL;
	double final_objective_gradient_inf = NAN;
	double final_objective_projected_gradient_inf = NAN;
	int finite_lower = 0;
	int finite_upper = 0;
	int used_lsq = 0;
	int i;

	memset(&ctx,0,sizeof(ctx));
	ctx.lsq_probe_status = -1;
	ctx.lsq_status = -1;
	FORTRAN_open(&funit,fname,&ierr);
	if(ierr != 0){
		fprintf(stderr,"A4SQP-CUTEst: failed to open OUTSDIF.d\n");
		return 1;
	}
	CUTEST_cdimen(&status,&funit,&ctx.n,&ctx.m);
	if(status != 0){
		fprintf(stderr,"A4SQP-CUTEst: CUTEST_cdimen status=%d\n",(int)status);
		return (int)status;
	}
	ctx.constrained = ctx.m > 0 ? TRUE_ : FALSE_;
	CUTEST_cnoobj(&status,&funit,&ctx.noobj);
	if(status != 0){
		ctx.noobj = FALSE_;
	}
	MALLOC(classification,FCSTRING_LEN + 1,char);
	MALLOC(pname,FSTRING_LEN + 1,char);
	MALLOC(x,ctx.n,rp_);
	MALLOC(x_l,ctx.n,rp_);
	MALLOC(x_u,ctx.n,rp_);
	MALLOC(ctx.g_work,ctx.n,rp_);
	if(classification == NULL || pname == NULL || x == NULL || x_l == NULL || x_u == NULL || ctx.g_work == NULL){
		fprintf(stderr,"A4SQP-CUTEst: allocation failure\n");
		return 2;
	}
	CUTEST_classification(&status,&funit,classification);
	if(status != 0){
		strcpy(classification,"unknown");
	}else{
		classification[FCSTRING_LEN] = '\0';
	}
	if(ctx.constrained){
		MALLOC(lambda,ctx.m,rp_);
		MALLOC(g_l,ctx.m,rp_);
		MALLOC(g_u,ctx.m,rp_);
		MALLOC(g,ctx.m,rp_);
		MALLOC(ctx.c_work,ctx.m,rp_);
		MALLOC(ctx.lambda_work,ctx.m,rp_);
		MALLOC(equatn,ctx.m,logical);
		MALLOC(linear,ctx.m,logical);
		if(lambda == NULL || g_l == NULL || g_u == NULL || g == NULL || ctx.c_work == NULL
			|| ctx.lambda_work == NULL || equatn == NULL || linear == NULL
		){
			fprintf(stderr,"A4SQP-CUTEst: constrained allocation failure\n");
			return 2;
		}
		CUTEST_csetup(
			&status,
			&funit,
			&iout,
			&io_buffer,
			&ctx.n,
			&ctx.m,
			x,
			x_l,
			x_u,
			lambda,
			g_l,
			g_u,
			equatn,
			linear,
			&e_order,
			&l_order,
			&v_order
		);
		CUTEST_cdimsj(&status,&nnzj_raw);
		ctx.nele_jac = nnzj_raw - ctx.n;
		if(ctx.nele_jac < 0){
			ctx.nele_jac = 0;
		}
		CUTEST_cdimsh(&status,&ctx.nele_hess);
		CUTEST_cstats(&status,&nonlin_obj,&nonlin_con,&neq,&nlinear);
		MALLOC(ctx.hess_work,ctx.nele_hess > 0 ? ctx.nele_hess : 1,rp_);
		MALLOC(ctx.hess_row,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_col,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_x_work,ctx.n > 0 ? ctx.n : 1,rp_);
		if(ctx.hess_work == NULL || ctx.hess_row == NULL || ctx.hess_col == NULL || ctx.hess_x_work == NULL){
			fprintf(stderr,"A4SQP-CUTEst: Hessian allocation failure\n");
			return 2;
		}
		for(i = 0; i < ctx.n; ++i){
			ctx.hess_x_work[i] = 0.0;
		}
	}else{
		CUTEST_usetup(&status,&funit,&iout,&io_buffer,&ctx.n,x,x_l,x_u);
		ctx.nele_jac = 0;
		CUTEST_udimsh(&status,&ctx.nele_hess);
		MALLOC(ctx.hess_work,ctx.nele_hess > 0 ? ctx.nele_hess : 1,rp_);
		MALLOC(ctx.hess_row,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_col,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_x_work,ctx.n > 0 ? ctx.n : 1,rp_);
		if(ctx.hess_work == NULL || ctx.hess_row == NULL || ctx.hess_col == NULL || ctx.hess_x_work == NULL){
			fprintf(stderr,"A4SQP-CUTEst: unconstrained Hessian allocation failure\n");
			return 2;
		}
		for(i = 0; i < ctx.n; ++i){
			ctx.hess_x_work[i] = 0.0;
		}
	}
	if(status != 0){
		fprintf(stderr,"A4SQP-CUTEst: setup status=%d\n",(int)status);
		return (int)status;
	}
	CUTEST_pname(&status,&funit,pname);
	if(status != 0){
		strcpy(pname,"unknown");
	}
	pname[FSTRING_LEN] = '\0';
	for(i = 0; i < ctx.n; ++i){
		if(x_l[i] > -1e19){
			++finite_lower;
		}
		if(x_u[i] < 1e19){
			++finite_upper;
		}
	}
	{
		int lsq_attempt = a4sqp_cutest_try_lsq(&ctx,x,x_l,x_u,classification,&obj,&stats,&solve_status);
		if(lsq_attempt == 0){
			used_lsq = 1;
			goto report;
		}
		if(lsq_attempt > 0){
			fprintf(stderr,"A4SQP-CUTEst: least-squares setup failed\n");
			return 3;
		}
	}
	problem = CreateA4SqpProblem(
		(A4SqpIndex)ctx.n,
		(double *)x_l,
		(double *)x_u,
		(A4SqpIndex)ctx.m,
		(double *)g_l,
		(double *)g_u,
		(A4SqpIndex)ctx.nele_jac,
		(A4SqpIndex)ctx.nele_hess,
		1,
		ctx.noobj ? NULL : a4sqp_cutest_eval_f,
		ctx.constrained ? a4sqp_cutest_eval_g : NULL,
		ctx.noobj ? NULL : a4sqp_cutest_eval_grad_f,
		ctx.constrained ? a4sqp_cutest_eval_jac_g : NULL,
		ctx.noobj ? NULL : a4sqp_cutest_eval_h
	);
	if(problem == NULL){
		fprintf(stderr,"A4SQP-CUTEst: CreateA4SqpProblem failed\n");
		return 3;
	}
	{
		double uniform_x_scale = a4sqp_cutest_env_double("A4SQP_X_SCALE",0.0);
		if(uniform_x_scale > 0.0 && isfinite(uniform_x_scale)){
			double *x_scale = (double *)malloc(sizeof(double) * (size_t)ctx.n);
			int i;
			if(x_scale == NULL){
				fprintf(stderr,"A4SQP-CUTEst: x scaling allocation failure\n");
				FreeA4SqpProblem(problem);
				return 3;
			}
			for(i = 0; i < ctx.n; ++i){
				x_scale[i] = uniform_x_scale;
			}
			if(!SetA4SqpProblemScaling(problem,1.0,x_scale,NULL)){
				fprintf(stderr,"A4SQP-CUTEst: SetA4SqpProblemScaling failed\n");
				free(x_scale);
				FreeA4SqpProblem(problem);
				return 3;
			}
			free(x_scale);
		}
	}
	if(!a4sqp_cutest_apply_core_options(problem)){
		FreeA4SqpProblem(problem);
		return 3;
	}
	solve_status = A4SqpSolve(
		problem,
		(double *)x,
		(double *)g,
		(double *)&obj,
		(double *)lambda,
		NULL,
		NULL,
		&ctx
	);
	GetA4SqpSolveStatistics(problem,&stats);
report:
	final_grad = (rp_ *)malloc(sizeof(*final_grad) * (size_t)ctx.n);
	if(final_grad != NULL && !a4sqp_cutest_final_objective_gradient(&ctx,x,&final_obj_check,final_grad)){
		final_objective_gradient_inf = a4sqp_cutest_inf_norm(ctx.n,final_grad);
		final_objective_projected_gradient_inf =
			a4sqp_cutest_bound_projected_grad_inf(ctx.n,x,x_l,x_u,final_grad);
	}
	if(ctx.constrained){
		CUTEST_creport(&status,calls,cpu);
		CUTEST_cterminate(&status);
	}else{
		CUTEST_ureport(&status,calls,cpu);
		CUTEST_uterminate(&status);
	}
	printf("{");
	printf("\"solver\":\"A4SQP\",");
	printf("\"problem\":");
	a4sqp_cutest_json_string(pname);
	printf(",");
	printf("\"classification\":");
	a4sqp_cutest_json_string(classification);
	printf(",");
	printf("\"n\":%d,\"m\":%d,\"equalities\":%d,\"linear_constraints\":%d,",
		(int)ctx.n,
		(int)ctx.m,
		(int)neq,
		(int)nlinear
	);
	printf("\"finite_var_lower\":%d,\"finite_var_upper\":%d,",finite_lower,finite_upper);
	printf("\"jac_nnz\":%d,\"hess_nnz\":%d,",(int)ctx.nele_jac,(int)ctx.nele_hess);
	printf("\"used_lsq\":%d,\"lsq_residuals\":%d,\"lsq_probe_status\":%d,",
		used_lsq,
		ctx.lsq_nres,
		ctx.lsq_probe_status
	);
	printf("\"lsq_status\":%d,\"lsq_handoff_improved\":%d,",ctx.lsq_status,ctx.lsq_handoff_improved);
	printf("\"lsq_iterations\":%d,\"lsq_objective\":",ctx.lsq_stats.iterations);
	a4sqp_cutest_json_number((double)ctx.lsq_stats.objective);
	printf(",\"lsq_grad_inf\":");
	a4sqp_cutest_json_number((double)ctx.lsq_stats.grad_inf);
	printf(",\"lsq_step_norm\":");
	a4sqp_cutest_json_number((double)ctx.lsq_stats.step_norm);
	printf(",\"lsq_lambda\":");
	a4sqp_cutest_json_number((double)ctx.lsq_stats.lambda);
	printf(",");
	printf("\"status\":%d,\"objective\":",(int)solve_status);
	a4sqp_cutest_json_number((double)obj);
	printf(",");
	printf("\"final_objective_check\":");
	a4sqp_cutest_json_number((double)final_obj_check);
	printf(",\"objective_gradient_inf\":");
	a4sqp_cutest_json_number(final_objective_gradient_inf);
	printf(",\"objective_projected_gradient_inf\":");
	a4sqp_cutest_json_number(final_objective_projected_gradient_inf);
	printf(",");
	printf("\"max_constraint_violation\":");
	a4sqp_cutest_json_number(stats.max_constraint_violation);
	printf(",");
	printf("\"projected_gradient_inf\":");
	a4sqp_cutest_json_number(stats.projected_gradient_inf);
	printf(",");
	printf("\"kkt_error\":");
	a4sqp_cutest_json_number(stats.kkt_error);
	printf(",");
	printf("\"dual_infeasibility_inf\":");
	a4sqp_cutest_json_number(stats.dual_infeasibility_inf);
	printf(",");
	printf("\"complementarity_inf\":");
	a4sqp_cutest_json_number(stats.complementarity_inf);
	printf(",");
	printf("\"kkt_lambda_sign\":%d,",(int)stats.kkt_lambda_sign);
	printf("\"bound_lower_active\":%d,\"bound_upper_active\":%d,\"bound_fixed_active\":%d,\"bound_worst_index\":%d,",
		(int)stats.bound_lower_active,
		(int)stats.bound_upper_active,
		(int)stats.bound_fixed_active,
		(int)stats.bound_worst_index
	);
	printf("\"bound_stationarity_inf\":");
	a4sqp_cutest_json_number(stats.bound_stationarity_inf);
	printf(",");
	printf("\"bound_worst_lagrangian_gradient\":");
	a4sqp_cutest_json_number(stats.bound_worst_lagrangian_gradient);
	printf(",");
	printf("\"iterations\":%d,\"qp_solves\":%d,\"qp_failures\":%d,\"line_search_failures\":%d,",
		stats.iterations,
		stats.qp_solves,
		stats.qp_failures,
		stats.line_search_failures
	);
	printf(
		"\"algorithm_mode\":%d,\"mode_switches\":%d,\"regular_iterations\":%d,\"restoration_iterations\":%d,\"restoration_entries\":%d,\"restoration_exits\":%d,\"restoration_handoffs\":%d,",
		(int)stats.algorithm_mode,
		(int)stats.mode_switches,
		(int)stats.regular_iterations,
		(int)stats.restoration_iterations,
		(int)stats.restoration_entries,
		(int)stats.restoration_exits,
		(int)stats.restoration_handoffs
	);
	printf("\"final_step_norm\":");
	a4sqp_cutest_json_number(stats.final_step_norm);
	printf(",\"final_trust_radius\":");
	a4sqp_cutest_json_number(stats.final_trust_radius);
	printf(",\"final_elastic_max\":");
	a4sqp_cutest_json_number(stats.final_elastic_max);
	printf(",\"regularization_size\":");
	a4sqp_cutest_json_number(stats.regularization_size);
	printf(
		",\"reduced_gradient_polish_mode\":%d,\"reduced_gradient_polish_attempts\":%d,\"reduced_gradient_polish_accepts\":%d,",
		(int)stats.reduced_gradient_polish_mode,
		(int)stats.reduced_gradient_polish_attempts,
		(int)stats.reduced_gradient_polish_accepts
	);
	printf("\"cutest_calls_obj\":%.17g,\"cutest_calls_objgrad\":%.17g,\"cutest_calls_con\":%.17g,\"cutest_calls_congrad\":%.17g,",
		(double)calls[0],
		(double)calls[1],
		(double)calls[4],
		(double)calls[5]
	);
	printf("\"cutest_setup_time\":%.17g,\"cutest_solve_time\":%.17g", (double)cpu[0], (double)cpu[1]);
	a4sqp_cutest_print_x_if_requested("A4SQP_DUMP_X",ctx.n,x);
	printf("}\n");
	if(problem != NULL){
		FreeA4SqpProblem(problem);
	}
	FREE(x);
	FREE(x_l);
	FREE(x_u);
	FREE(lambda);
	FREE(g_l);
	FREE(g_u);
	FREE(g);
	FREE(equatn);
	FREE(linear);
	FREE(final_grad);
	FREE(ctx.c_work);
	FREE(ctx.g_work);
	FREE(ctx.hess_work);
	FREE(ctx.hess_x_work);
	FREE(ctx.lambda_work);
	FREE(ctx.hess_row);
	FREE(ctx.hess_col);
	FREE(ctx.lsq_weights);
	FREE(pname);
	FREE(classification);
	return (int)solve_status;
}

#ifdef __cplusplus
}
#endif
