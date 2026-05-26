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

static real64 a4sqp_lsq_stationarity_inf(
	int32 n,
	const real64 *normal,
	const real64 *gradient,
	int scaled
){
	int32 i;
	real64 v = 0.0;
	if(gradient == NULL){
		return 0.0;
	}
	if(!scaled){
		return a4sqp_lsq_inf_norm(n,gradient);
	}
	for(i = 0; i < n; ++i){
		real64 diag = normal != NULL ? fabs(normal[i * n + i]) : 0.0;
		real64 denom = sqrt(diag > 1.0 ? diag : 1.0);
		real64 a = fabs(gradient[i]) / denom;
		if(a > v){
			v = a;
		}
	}
	return v;
}

static real64 a4sqp_lsq_stall_step_tol(int32 n, const real64 *x, real64 requested_tol){
	real64 scale = 1.0 + a4sqp_lsq_norm2(n,x);
	real64 floor_tol = 1e-12 * scale;
	if(floor_tol < 1e-14){
		floor_tol = 1e-14;
	}
	if(requested_tol > 0.0 && requested_tol < floor_tol){
		return requested_tol;
	}
	return floor_tol;
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

static real64 a4sqp_lsq_quadratic_reduction(
	int32 n,
	const real64 *normal,
	const real64 *gradient,
	const real64 *step
){
	int32 i;
	int32 j;
	real64 linear = 0.0;
	real64 quadratic = 0.0;
	if(normal == NULL || gradient == NULL || step == NULL){
		return 0.0;
	}
	for(i = 0; i < n; ++i){
		linear += gradient[i] * step[i];
		for(j = 0; j < n; ++j){
			quadratic += step[i] * normal[i * n + j] * step[j];
		}
	}
	return -linear - 0.5 * quadratic;
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

static int a4sqp_lsq_solve_dense_qr(
	int32 rows,
	int32 cols,
	const real64 *matrix,
	const real64 *rhs,
	real64 *solution
){
	real64 *a = NULL;
	real64 *b = NULL;
	real64 *col_norm = NULL;
	int32 *perm = NULL;
	real64 max_col_norm = 0.0;
	real64 eps;
	int32 i;
	int32 j;
	int32 k;
	if(rows < cols || cols < 0 || matrix == NULL || rhs == NULL || solution == NULL){
		return 1;
	}
	a = A4SQP_NEW_ARRAY(real64,(size_t)rows * (size_t)cols);
	b = A4SQP_NEW_ARRAY(real64,rows);
	col_norm = A4SQP_NEW_ARRAY(real64,cols);
	perm = A4SQP_NEW_ARRAY(int32,cols);
	if(a == NULL || b == NULL || col_norm == NULL || perm == NULL){
		A4SQP_FREE(a);
		A4SQP_FREE(b);
		A4SQP_FREE(col_norm);
		A4SQP_FREE(perm);
		return 1;
	}
	memcpy(a,matrix,sizeof(real64) * (size_t)rows * (size_t)cols);
	memcpy(b,rhs,sizeof(real64) * (size_t)rows);
	for(j = 0; j < cols; ++j){
		real64 col_norm2 = 0.0;
		for(i = 0; i < rows; ++i){
			col_norm2 += a[(size_t)i * (size_t)cols + (size_t)j]
				* a[(size_t)i * (size_t)cols + (size_t)j];
		}
		col_norm[j] = sqrt(col_norm2);
		perm[j] = j;
		if(col_norm[j] > max_col_norm){
			max_col_norm = col_norm[j];
		}
	}
	eps = 1e-12 * (max_col_norm > 1.0 ? max_col_norm : 1.0);
	for(i = 0; i < cols; ++i){
		solution[i] = 0.0;
	}
	for(k = 0; k < cols; ++k){
		int32 pivot = k;
		real64 norm2 = 0.0;
		real64 norm;
		real64 alpha;
		real64 v0;
		real64 vnorm2;
		real64 tau;
		real64 dot;
		real64 factor;
		for(j = k + 1; j < cols; ++j){
			if(col_norm[j] > col_norm[pivot]){
				pivot = j;
			}
		}
		if(pivot != k){
			int32 tmp_perm = perm[k];
			real64 tmp_norm = col_norm[k];
			perm[k] = perm[pivot];
			perm[pivot] = tmp_perm;
			col_norm[k] = col_norm[pivot];
			col_norm[pivot] = tmp_norm;
			for(i = 0; i < rows; ++i){
				real64 tmp = a[(size_t)i * (size_t)cols + (size_t)k];
				a[(size_t)i * (size_t)cols + (size_t)k] =
					a[(size_t)i * (size_t)cols + (size_t)pivot];
				a[(size_t)i * (size_t)cols + (size_t)pivot] = tmp;
			}
		}
		for(i = k; i < rows; ++i){
			real64 value = a[(size_t)i * (size_t)cols + (size_t)k];
			norm2 += value * value;
		}
		norm = sqrt(norm2);
		if(!isfinite(norm) || norm <= eps){
			A4SQP_FREE(a);
			A4SQP_FREE(b);
			A4SQP_FREE(col_norm);
			A4SQP_FREE(perm);
			return 1;
		}
		alpha = a[(size_t)k * (size_t)cols + (size_t)k] >= 0.0 ? -norm : norm;
		v0 = a[(size_t)k * (size_t)cols + (size_t)k] - alpha;
		vnorm2 = v0 * v0;
		for(i = k + 1; i < rows; ++i){
			real64 value = a[(size_t)i * (size_t)cols + (size_t)k];
			vnorm2 += value * value;
		}
		if(!isfinite(vnorm2) || vnorm2 <= eps * eps){
			A4SQP_FREE(a);
			A4SQP_FREE(b);
			A4SQP_FREE(col_norm);
			A4SQP_FREE(perm);
			return 1;
		}
		tau = 2.0 / vnorm2;
		dot = v0 * b[k];
		for(i = k + 1; i < rows; ++i){
			dot += a[(size_t)i * (size_t)cols + (size_t)k] * b[i];
		}
		factor = tau * dot;
		b[k] -= factor * v0;
		for(i = k + 1; i < rows; ++i){
			b[i] -= factor * a[(size_t)i * (size_t)cols + (size_t)k];
		}
		for(j = k + 1; j < cols; ++j){
			dot = v0 * a[(size_t)k * (size_t)cols + (size_t)j];
			for(i = k + 1; i < rows; ++i){
				dot += a[(size_t)i * (size_t)cols + (size_t)k]
					* a[(size_t)i * (size_t)cols + (size_t)j];
			}
			factor = tau * dot;
			a[(size_t)k * (size_t)cols + (size_t)j] -= factor * v0;
			for(i = k + 1; i < rows; ++i){
				a[(size_t)i * (size_t)cols + (size_t)j] -=
					factor * a[(size_t)i * (size_t)cols + (size_t)k];
			}
			{
				real64 trailing_norm2 = 0.0;
				for(i = k + 1; i < rows; ++i){
					real64 value = a[(size_t)i * (size_t)cols + (size_t)j];
					trailing_norm2 += value * value;
				}
				col_norm[j] = sqrt(trailing_norm2);
			}
		}
		a[(size_t)k * (size_t)cols + (size_t)k] = alpha;
		for(i = k + 1; i < rows; ++i){
			a[(size_t)i * (size_t)cols + (size_t)k] = 0.0;
		}
	}
	for(i = cols - 1; i >= 0; --i){
		real64 sum = b[i];
		real64 diag = a[(size_t)i * (size_t)cols + (size_t)i];
		if(fabs(diag) <= eps || !isfinite(diag)){
			A4SQP_FREE(a);
			A4SQP_FREE(b);
			A4SQP_FREE(col_norm);
			A4SQP_FREE(perm);
			return 1;
		}
		for(j = i + 1; j < cols; ++j){
			sum -= a[(size_t)i * (size_t)cols + (size_t)j] * solution[j];
		}
		solution[i] = sum / diag;
		if(!isfinite(solution[i])){
			A4SQP_FREE(a);
			A4SQP_FREE(b);
			A4SQP_FREE(col_norm);
			A4SQP_FREE(perm);
			return 1;
		}
	}
	for(i = 0; i < cols; ++i){
		b[perm[i]] = solution[i];
	}
	for(i = 0; i < cols; ++i){
		solution[i] = b[i];
	}
	A4SQP_FREE(a);
	A4SQP_FREE(b);
	A4SQP_FREE(col_norm);
	A4SQP_FREE(perm);
	return 0;
}

static int a4sqp_lsq_build_qr_system(
	const struct A4SqpLsqProblem *problem,
	const real64 *residuals,
	const real64 *scale,
	real64 *matrix,
	real64 *rhs,
	int32 *columns,
	real64 *values
){
	int32 row;
	int32 n = problem->n_var;
	int32 m = problem->n_res;
	if(matrix == NULL || rhs == NULL){
		return 1;
	}
	memset(matrix,0,sizeof(real64) * (size_t)m * (size_t)n);
	memset(rhs,0,sizeof(real64) * (size_t)m);
	for(row = 0; row < m; ++row){
		int32 nnz = 0;
		int32 i;
		real64 weight = a4sqp_lsq_weight(problem,row);
		real64 sqrt_weight;
		if(weight < 0.0 || !isfinite(weight)){
			return 1;
		}
		sqrt_weight = sqrt(weight);
		rhs[row] = -sqrt_weight * residuals[row];
		if(problem->eval_jacobian_row(problem->userdata,row,n,columns,values,&nnz)){
			return 1;
		}
		if(nnz < 0 || nnz > n){
			return 1;
		}
		for(i = 0; i < nnz; ++i){
			int32 col = columns[i];
			real64 denom;
			if(col < 0 || col >= n){
				return 1;
			}
			denom = (scale != NULL && scale[col] > 0.0) ? scale[col] : 1.0;
			matrix[(size_t)row * (size_t)n + (size_t)col] += sqrt_weight * values[i] / denom;
		}
	}
	return 0;
}

static void a4sqp_lsq_defaults(struct A4SqpLsqOptions *dst, const struct A4SqpLsqOptions *src){
	memset(dst,0,sizeof(*dst));
	if(src != NULL){
		*dst = *src;
	}
	if(dst->linear_solver != A4SQP_LSQ_LINEAR_DENSE_QR){
		dst->linear_solver = A4SQP_LSQ_LINEAR_NORMAL;
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
	real64 *scale = NULL;
	real64 *qr_matrix = NULL;
	real64 *qr_rhs = NULL;
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
	scale = A4SQP_NEW_ARRAY(real64,n);
	if(opt.linear_solver == A4SQP_LSQ_LINEAR_DENSE_QR){
		qr_matrix = A4SQP_NEW_ARRAY(real64,(size_t)(problem->n_res + n) * (size_t)n);
		qr_rhs = A4SQP_NEW_ARRAY(real64,problem->n_res + n);
	}
	x_trial = A4SQP_NEW_ARRAY(real64,n);
	columns = A4SQP_NEW_ARRAY(int32,n);
	values = A4SQP_NEW_ARRAY(real64,n);
	if(r == NULL || r_trial == NULL || normal == NULL || damped == NULL
		|| gradient == NULL || rhs == NULL || step == NULL || scale == NULL
		|| (opt.linear_solver == A4SQP_LSQ_LINEAR_DENSE_QR && (qr_matrix == NULL || qr_rhs == NULL))
		|| x_trial == NULL || columns == NULL || values == NULL
	){
		A4SQP_FREE(r);
		A4SQP_FREE(r_trial);
		A4SQP_FREE(normal);
		A4SQP_FREE(damped);
		A4SQP_FREE(gradient);
		A4SQP_FREE(rhs);
		A4SQP_FREE(step);
		A4SQP_FREE(scale);
		A4SQP_FREE(qr_matrix);
		A4SQP_FREE(qr_rhs);
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
		grad_inf = a4sqp_lsq_stationarity_inf(n,normal,gradient,opt.scaled_stationarity);
		if(grad_inf <= opt.grad_tol || obj <= 0.5 * opt.grad_tol * opt.grad_tol){
			a4sqp_lsq_fill_stats(stats,iter,obj,grad_inf,step_norm,lambda,accepted_steps);
			goto solved;
		}
		for(i = 0; i < n; ++i){
			real64 diag = fabs(normal[i * n + i]);
			scale[i] = sqrt(diag > 1.0 ? diag : 1.0);
			rhs[i] = -gradient[i] / scale[i];
		}
		if(opt.linear_solver == A4SQP_LSQ_LINEAR_DENSE_QR){
			if(a4sqp_lsq_build_qr_system(
				problem,
				r,
				scale,
				qr_matrix,
				qr_rhs,
				columns,
				values
			)){
				a4sqp_lsq_fill_stats(stats,iter,obj,0.0,step_norm,lambda,accepted_steps);
				goto eval_error;
			}
		}

		for(trial = 0; trial < opt.max_backtrack; ++trial){
			/* Solve the LM system in diagonally scaled coordinates; this is
			 * equivalent to damping by diag(J'J) but better conditioned. */
			for(i = 0; i < n; ++i){
				step[i] = 0.0;
			}
			for(i = 0; i < n; ++i){
				int32 j;
				for(j = 0; j < n; ++j){
					damped[i * n + j] = normal[i * n + j] / (scale[i] * scale[j]);
				}
				if(local_lambda > 0.0){
					damped[i * n + i] += local_lambda;
				}
			}
			if(opt.linear_solver == A4SQP_LSQ_LINEAR_DENSE_QR){
				int32 qr_rows = problem->n_res + (local_lambda > 0.0 ? n : 0);
				memset(qr_matrix + (size_t)problem->n_res * (size_t)n,0,sizeof(real64) * (size_t)n * (size_t)n);
				memset(qr_rhs + problem->n_res,0,sizeof(real64) * (size_t)n);
				if(local_lambda > 0.0){
					real64 sqrt_lambda = sqrt(local_lambda);
					if(!isfinite(sqrt_lambda)){
						fail_status = A4SQP_LSQ_LINEAR_ERROR;
						break;
					}
					for(i = 0; i < n; ++i){
						qr_matrix[(size_t)(problem->n_res + i) * (size_t)n + (size_t)i] = sqrt_lambda;
					}
				}
				if(a4sqp_lsq_solve_dense_qr(qr_rows,n,qr_matrix,qr_rhs,step)){
					fail_status = A4SQP_LSQ_LINEAR_ERROR;
				}else{
					fail_status = A4SQP_LSQ_SOLVED;
				}
			}else{
				if(a4sqp_lsq_solve_dense(n,damped,rhs,step)){
					fail_status = A4SQP_LSQ_LINEAR_ERROR;
				}else{
					fail_status = A4SQP_LSQ_SOLVED;
				}
			}
			if(fail_status == A4SQP_LSQ_LINEAR_ERROR){
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
			if(fail_status == A4SQP_LSQ_EVAL_ERROR){
				break;
			}
			for(i = 0; i < n; ++i){
				step[i] /= scale[i];
			}
			step_norm = a4sqp_lsq_norm2(n,step);
			if(step_norm <= opt.step_tol && grad_inf <= opt.grad_tol){
				a4sqp_lsq_fill_stats(stats,iter,obj,grad_inf,step_norm,local_lambda,accepted_steps);
				goto solved;
			}
			if(step_norm <= a4sqp_lsq_stall_step_tol(n,x,opt.step_tol)){
				a4sqp_lsq_fill_stats(stats,iter,obj,grad_inf,step_norm,local_lambda,accepted_steps);
				goto stalled;
			}
			for(i = 0; i < n; ++i){
				real64 lower = problem->x_lower != NULL ? problem->x_lower[i] : A4SQP_NO_LOWER_BOUND;
				real64 upper = problem->x_upper != NULL ? problem->x_upper[i] : A4SQP_NO_UPPER_BOUND;
				x_trial[i] = a4sqp_lsq_project_value(x[i] + alpha * step[i],lower,upper);
			}
			{
				real64 trial_obj = 0.0;
				real64 actual_reduction = 0.0;
				real64 predicted_reduction = a4sqp_lsq_quadratic_reduction(n,normal,gradient,step);
				real64 rho = 0.0;
				int trial_eval_failed = problem->eval_residuals(problem->userdata,x_trial,r_trial);
				if(!trial_eval_failed){
					trial_obj = a4sqp_lsq_objective(problem,r_trial);
					actual_reduction = obj - trial_obj;
				}
				if(predicted_reduction > 0.0 && isfinite(predicted_reduction)){
					rho = actual_reduction / predicted_reduction;
				}
				if(!trial_eval_failed && isfinite(trial_obj) && actual_reduction >= 0.0){
					memcpy(x,x_trial,sizeof(real64) * (size_t)n);
					memcpy(r,r_trial,sizeof(real64) * (size_t)problem->n_res);
					obj = trial_obj;
					accepted = 1;
					accepted_steps++;
					if(opt.mode == A4SQP_LSQ_MODE_LM){
						/* Nielsen-style damping update from model agreement. */
						if(predicted_reduction > 0.0 && isfinite(rho) && rho > 0.0){
							real64 factor = 1.0 - pow(2.0 * rho - 1.0, 3.0);
							if(factor < 1.0 / 3.0){
								factor = 1.0 / 3.0;
							}else if(factor > 3.0){
								factor = 3.0;
							}
							lambda = fmax(opt.lambda_min,local_lambda * factor);
						}else{
							lambda = fmax(opt.lambda_min,local_lambda * 0.3);
						}
					}else{
						lambda = local_lambda;
					}
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
	grad_inf = a4sqp_lsq_stationarity_inf(n,normal,gradient,opt.scaled_stationarity);
	a4sqp_lsq_fill_stats(stats,opt.max_iter,obj,grad_inf,step_norm,lambda,accepted_steps);
	A4SQP_FREE(r);
	A4SQP_FREE(r_trial);
	A4SQP_FREE(normal);
	A4SQP_FREE(damped);
	A4SQP_FREE(gradient);
	A4SQP_FREE(rhs);
	A4SQP_FREE(step);
	A4SQP_FREE(scale);
	A4SQP_FREE(qr_matrix);
	A4SQP_FREE(qr_rhs);
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
	A4SQP_FREE(scale);
	A4SQP_FREE(qr_matrix);
	A4SQP_FREE(qr_rhs);
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
	A4SQP_FREE(scale);
	A4SQP_FREE(qr_matrix);
	A4SQP_FREE(qr_rhs);
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
	A4SQP_FREE(scale);
	A4SQP_FREE(qr_matrix);
	A4SQP_FREE(qr_rhs);
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
	A4SQP_FREE(scale);
	A4SQP_FREE(qr_matrix);
	A4SQP_FREE(qr_rhs);
	A4SQP_FREE(x_trial);
	A4SQP_FREE(columns);
	A4SQP_FREE(values);
	return A4SQP_LSQ_LINEAR_ERROR;
}
