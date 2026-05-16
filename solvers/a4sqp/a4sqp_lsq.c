/*
 * Least-squares core solve utilities for A4SQP.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_lsq.h"

#include "a4sqp_core.h"

#include <math.h>
#include <string.h>

static real64 a4sqp_lsq_weight(const struct A4SqpLsqProblem *problem, int32 row){
	if(problem == NULL || problem->weights == NULL){
		return 1.0;
	}
	return problem->weights[row];
}

static real64 a4sqp_lsq_norm2(int32 n, const real64 *x){
	int32 i;
	real64 s = 0.0;
	if(x == NULL){
		return 0.0;
	}
	for(i = 0; i < n; ++i){
		s += x[i] * x[i];
	}
	return sqrt(s);
}

static real64 a4sqp_lsq_inf_norm(int32 n, const real64 *x){
	int32 i;
	real64 v = 0.0;
	if(x == NULL){
		return 0.0;
	}
	for(i = 0; i < n; ++i){
		real64 a = fabs(x[i]);
		if(a > v){
			v = a;
		}
	}
	return v;
}

static real64 a4sqp_lsq_objective(
	const struct A4SqpLsqProblem *problem,
	const real64 *residuals
){
	int32 i;
	real64 obj = 0.0;
	for(i = 0; i < problem->n_res; ++i){
		obj += 0.5 * a4sqp_lsq_weight(problem,i) * residuals[i] * residuals[i];
	}
	return obj;
}

static real64 a4sqp_lsq_project_value(
	real64 value,
	real64 lower,
	real64 upper
){
	if(!a4sqp_core_is_lower_inf(lower) && value < lower){
		value = lower;
	}
	if(!a4sqp_core_is_upper_inf(upper) && value > upper){
		value = upper;
	}
	return value;
}

static int a4sqp_lsq_build_normal_equations(
	const struct A4SqpLsqProblem *problem,
	const real64 *residuals,
	real64 *normal,
	real64 *gradient,
	int32 *columns,
	real64 *values
){
	int32 row;
	int32 n = problem->n_var;

	memset(normal,0,sizeof(real64) * (size_t)n * (size_t)n);
	memset(gradient,0,sizeof(real64) * (size_t)n);

	for(row = 0; row < problem->n_res; ++row){
		int32 nnz = 0;
		int32 i;
		real64 weight = a4sqp_lsq_weight(problem,row);
		if(problem->eval_jacobian_row(problem->userdata,row,n,columns,values,&nnz)){
			return 1;
		}
		if(nnz < 0 || nnz > n){
			return 1;
		}
		for(i = 0; i < nnz; ++i){
			int32 col_i = columns[i];
			int32 j;
			if(col_i < 0 || col_i >= n){
				return 1;
			}
			gradient[col_i] += weight * values[i] * residuals[row];
			for(j = 0; j < nnz; ++j){
				int32 col_j = columns[j];
				if(col_j < 0 || col_j >= n){
					return 1;
				}
				normal[col_i * n + col_j] += weight * values[i] * values[j];
			}
		}
	}
	return 0;
}

static int a4sqp_lsq_solve_dense(
	int32 n,
	const real64 *matrix,
	const real64 *rhs,
	real64 *solution
){
	real64 *a = NULL;
	real64 *b = NULL;
	int32 i;
	int32 j;
	int32 k;
	const real64 eps = 1e-20;

	a = A4SQP_NEW_ARRAY(real64,(size_t)n * (size_t)n);
	b = A4SQP_NEW_ARRAY(real64,n);
	if(a == NULL || b == NULL){
		A4SQP_FREE(a);
		A4SQP_FREE(b);
		return 1;
	}
	memcpy(a,matrix,sizeof(real64) * (size_t)n * (size_t)n);
	memcpy(b,rhs,sizeof(real64) * (size_t)n);

	for(k = 0; k < n; ++k){
		int32 pivot = k;
		real64 pivot_abs = fabs(a[k * n + k]);
		for(i = k + 1; i < n; ++i){
			real64 candidate = fabs(a[i * n + k]);
			if(candidate > pivot_abs){
				pivot_abs = candidate;
				pivot = i;
			}
		}
		if(pivot_abs <= eps || !isfinite(pivot_abs)){
			A4SQP_FREE(a);
			A4SQP_FREE(b);
			return 1;
		}
		if(pivot != k){
			for(j = k; j < n; ++j){
				real64 tmp = a[k * n + j];
				a[k * n + j] = a[pivot * n + j];
				a[pivot * n + j] = tmp;
			}
			{
				real64 tmp = b[k];
				b[k] = b[pivot];
				b[pivot] = tmp;
			}
		}
		for(i = k + 1; i < n; ++i){
			real64 factor = a[i * n + k] / a[k * n + k];
			a[i * n + k] = 0.0;
			for(j = k + 1; j < n; ++j){
				a[i * n + j] -= factor * a[k * n + j];
			}
			b[i] -= factor * b[k];
		}
	}

	for(i = n - 1; i >= 0; --i){
		real64 sum = b[i];
		for(j = i + 1; j < n; ++j){
			sum -= a[i * n + j] * solution[j];
		}
		solution[i] = sum / a[i * n + i];
	}
	A4SQP_FREE(a);
	A4SQP_FREE(b);
	return 0;
}

static void a4sqp_lsq_defaults(struct A4SqpLsqOptions *dst, const struct A4SqpLsqOptions *src){
	memset(dst,0,sizeof(*dst));
	if(src != NULL){
		*dst = *src;
	}
	if(dst->max_iter <= 0){
		dst->max_iter = 50;
	}
	if(dst->max_backtrack <= 0){
		dst->max_backtrack = 20;
	}
	if(dst->grad_tol <= 0.0){
		dst->grad_tol = 1e-7;
	}
	if(dst->step_tol <= 0.0){
		dst->step_tol = 1e-8;
	}
	if(dst->lambda_init <= 0.0){
		dst->lambda_init = dst->mode == A4SQP_LSQ_MODE_LM ? 1e-3 : 0.0;
	}
	if(dst->lambda_min <= 0.0){
		dst->lambda_min = 1e-12;
	}
	if(dst->lambda_max <= 0.0){
		dst->lambda_max = 1e12;
	}
}

static void a4sqp_lsq_fill_stats(
	struct A4SqpLsqStats *stats,
	int iter,
	real64 objective,
	real64 grad_inf,
	real64 step_norm,
	real64 lambda,
	int accepted_steps
){
	if(stats == NULL){
		return;
	}
	stats->iterations = iter;
	stats->objective = objective;
	stats->grad_inf = grad_inf;
	stats->step_norm = step_norm;
	stats->lambda = lambda;
	stats->accepted_steps = accepted_steps;
}

enum A4SqpLsqStatus a4sqp_lsq_solve(
	const struct A4SqpLsqProblem *problem,
	const struct A4SqpLsqOptions *options,
	real64 *x,
	struct A4SqpLsqStats *stats
){
	struct A4SqpLsqOptions opt;
	real64 *r = NULL;
	real64 *r_trial = NULL;
	real64 *normal = NULL;
	real64 *damped = NULL;
	real64 *gradient = NULL;
	real64 *rhs = NULL;
	real64 *step = NULL;
	real64 *x_trial = NULL;
	int32 *columns = NULL;
	real64 *values = NULL;
	real64 obj;
	real64 grad_inf;
	real64 step_norm = 0.0;
	real64 lambda;
	int accepted_steps = 0;
	int iter;
	int32 i;
	int32 n;

	if(problem == NULL || x == NULL || problem->n_var < 0 || problem->n_res <= 0
		|| problem->eval_residuals == NULL || problem->eval_jacobian_row == NULL
	){
		return A4SQP_LSQ_INVALID_PROBLEM;
	}
	n = problem->n_var;
	a4sqp_lsq_defaults(&opt,options);
	lambda = opt.lambda_init;
	if(n == 0){
		a4sqp_lsq_fill_stats(stats,0,0.0,0.0,0.0,lambda,0);
		return A4SQP_LSQ_SOLVED;
	}

	r = A4SQP_NEW_ARRAY(real64,problem->n_res);
	r_trial = A4SQP_NEW_ARRAY(real64,problem->n_res);
	normal = A4SQP_NEW_ARRAY(real64,(size_t)n * (size_t)n);
	damped = A4SQP_NEW_ARRAY(real64,(size_t)n * (size_t)n);
	gradient = A4SQP_NEW_ARRAY(real64,n);
	rhs = A4SQP_NEW_ARRAY(real64,n);
	step = A4SQP_NEW_ARRAY(real64,n);
	x_trial = A4SQP_NEW_ARRAY(real64,n);
	columns = A4SQP_NEW_ARRAY(int32,n);
	values = A4SQP_NEW_ARRAY(real64,n);
	if(r == NULL || r_trial == NULL || normal == NULL || damped == NULL
		|| gradient == NULL || rhs == NULL || step == NULL || x_trial == NULL
		|| columns == NULL || values == NULL
	){
		A4SQP_FREE(r);
		A4SQP_FREE(r_trial);
		A4SQP_FREE(normal);
		A4SQP_FREE(damped);
		A4SQP_FREE(gradient);
		A4SQP_FREE(rhs);
		A4SQP_FREE(step);
		A4SQP_FREE(x_trial);
		A4SQP_FREE(columns);
		A4SQP_FREE(values);
		return A4SQP_LSQ_INVALID_PROBLEM;
	}

	if(problem->eval_residuals(problem->userdata,x,r)){
		a4sqp_lsq_fill_stats(stats,0,0.0,0.0,0.0,lambda,0);
		goto eval_error;
	}
	obj = a4sqp_lsq_objective(problem,r);

	for(iter = 0; iter < opt.max_iter; ++iter){
		int accepted = 0;
		int trial;
		real64 alpha = 1.0;
		real64 local_lambda = lambda;
		enum A4SqpLsqStatus fail_status = A4SQP_LSQ_LINEAR_ERROR;

		if(a4sqp_lsq_build_normal_equations(problem,r,normal,gradient,columns,values)){
			a4sqp_lsq_fill_stats(stats,iter,obj,0.0,step_norm,lambda,accepted_steps);
			goto eval_error;
		}
		grad_inf = a4sqp_lsq_inf_norm(n,gradient);
		if(grad_inf <= opt.grad_tol || obj <= 0.5 * opt.grad_tol * opt.grad_tol){
			a4sqp_lsq_fill_stats(stats,iter,obj,grad_inf,step_norm,lambda,accepted_steps);
			goto solved;
		}

		for(trial = 0; trial < opt.max_backtrack; ++trial){
			memcpy(damped,normal,sizeof(real64) * (size_t)n * (size_t)n);
			for(i = 0; i < n; ++i){
				real64 diag = fabs(normal[i * n + i]);
				if(local_lambda > 0.0){
					damped[i * n + i] += local_lambda * (diag > 1.0 ? diag : 1.0);
				}
				rhs[i] = -gradient[i];
				step[i] = 0.0;
			}
			if(a4sqp_lsq_solve_dense(n,damped,rhs,step)){
				if(opt.mode == A4SQP_LSQ_MODE_GAUSS && local_lambda <= 0.0){
					local_lambda = opt.lambda_min;
				}else{
					local_lambda *= 10.0;
					if(local_lambda <= 0.0){
						local_lambda = opt.lambda_min;
					}
				}
				if(local_lambda > opt.lambda_max){
					fail_status = A4SQP_LSQ_LINEAR_ERROR;
					break;
				}
				continue;
			}
			step_norm = a4sqp_lsq_norm2(n,step);
			if(step_norm <= opt.step_tol){
				a4sqp_lsq_fill_stats(stats,iter,obj,grad_inf,step_norm,local_lambda,accepted_steps);
				if(grad_inf <= opt.grad_tol){
					goto solved;
				}
				goto stalled;
			}
			for(i = 0; i < n; ++i){
				real64 lower = problem->x_lower != NULL ? problem->x_lower[i] : A4SQP_NO_LOWER_BOUND;
				real64 upper = problem->x_upper != NULL ? problem->x_upper[i] : A4SQP_NO_UPPER_BOUND;
				x_trial[i] = a4sqp_lsq_project_value(x[i] + alpha * step[i],lower,upper);
			}
			{
				real64 trial_obj = 0.0;
				int trial_eval_failed = problem->eval_residuals(problem->userdata,x_trial,r_trial);
				if(!trial_eval_failed){
					trial_obj = a4sqp_lsq_objective(problem,r_trial);
				}
				if(!trial_eval_failed && isfinite(trial_obj) && trial_obj <= obj){
					memcpy(x,x_trial,sizeof(real64) * (size_t)n);
					memcpy(r,r_trial,sizeof(real64) * (size_t)problem->n_res);
					obj = trial_obj;
					accepted = 1;
					accepted_steps++;
					lambda = opt.mode == A4SQP_LSQ_MODE_LM
						? fmax(opt.lambda_min,local_lambda * 0.3)
						: local_lambda;
					if(problem->progress != NULL){
						struct A4SqpLsqIteration progress;
						progress.iter = iter + 1;
						progress.objective = obj;
						progress.grad_inf = grad_inf;
						progress.step_norm = step_norm;
						progress.lambda = lambda;
						progress.alpha = alpha;
						progress.accepted = 1;
						if(problem->progress(problem->userdata,&progress)){
							fail_status = A4SQP_LSQ_EVAL_ERROR;
							break;
						}
					}
					break;
				}
			}
			if(opt.mode == A4SQP_LSQ_MODE_LM){
				local_lambda *= 10.0;
				if(local_lambda <= 0.0){
					local_lambda = opt.lambda_min;
				}
				if(local_lambda > opt.lambda_max){
					fail_status = A4SQP_LSQ_LINEAR_ERROR;
					break;
				}
			}else{
				alpha *= 0.5;
				if(alpha <= 1e-12){
					fail_status = A4SQP_LSQ_LINEAR_ERROR;
					break;
				}
			}
		}
		if(!accepted){
			a4sqp_lsq_fill_stats(stats,iter,obj,grad_inf,step_norm,lambda,accepted_steps);
			if(fail_status == A4SQP_LSQ_EVAL_ERROR){
				goto eval_error;
			}
			goto linear_error;
		}
	}

	if(a4sqp_lsq_build_normal_equations(problem,r,normal,gradient,columns,values)){
		a4sqp_lsq_fill_stats(stats,opt.max_iter,obj,0.0,step_norm,lambda,accepted_steps);
		goto eval_error;
	}
	grad_inf = a4sqp_lsq_inf_norm(n,gradient);
	a4sqp_lsq_fill_stats(stats,opt.max_iter,obj,grad_inf,step_norm,lambda,accepted_steps);
	A4SQP_FREE(r);
	A4SQP_FREE(r_trial);
	A4SQP_FREE(normal);
	A4SQP_FREE(damped);
	A4SQP_FREE(gradient);
	A4SQP_FREE(rhs);
	A4SQP_FREE(step);
	A4SQP_FREE(x_trial);
	A4SQP_FREE(columns);
	A4SQP_FREE(values);
	return A4SQP_LSQ_MAX_ITER;

solved:
	A4SQP_FREE(r);
	A4SQP_FREE(r_trial);
	A4SQP_FREE(normal);
	A4SQP_FREE(damped);
	A4SQP_FREE(gradient);
	A4SQP_FREE(rhs);
	A4SQP_FREE(step);
	A4SQP_FREE(x_trial);
	A4SQP_FREE(columns);
	A4SQP_FREE(values);
	return A4SQP_LSQ_SOLVED;

eval_error:
	A4SQP_FREE(r);
	A4SQP_FREE(r_trial);
	A4SQP_FREE(normal);
	A4SQP_FREE(damped);
	A4SQP_FREE(gradient);
	A4SQP_FREE(rhs);
	A4SQP_FREE(step);
	A4SQP_FREE(x_trial);
	A4SQP_FREE(columns);
	A4SQP_FREE(values);
	return A4SQP_LSQ_EVAL_ERROR;

stalled:
	A4SQP_FREE(r);
	A4SQP_FREE(r_trial);
	A4SQP_FREE(normal);
	A4SQP_FREE(damped);
	A4SQP_FREE(gradient);
	A4SQP_FREE(rhs);
	A4SQP_FREE(step);
	A4SQP_FREE(x_trial);
	A4SQP_FREE(columns);
	A4SQP_FREE(values);
	return A4SQP_LSQ_MAX_ITER;

linear_error:
	A4SQP_FREE(r);
	A4SQP_FREE(r_trial);
	A4SQP_FREE(normal);
	A4SQP_FREE(damped);
	A4SQP_FREE(gradient);
	A4SQP_FREE(rhs);
	A4SQP_FREE(step);
	A4SQP_FREE(x_trial);
	A4SQP_FREE(columns);
	A4SQP_FREE(values);
	return A4SQP_LSQ_LINEAR_ERROR;
}
