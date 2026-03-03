#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "IpStdCInterface.h"
#include "fprops.h"
#include "fluids.h"
#include "eqm_internal.h"
#include "eqm_ipopt.h"
#ifdef HAVE_NLOPT
#include "eqm_slsqp.h"
#endif

static int eqm_low_temperature(double T){
	return T < 700.0;
}

static void eqm_ipopt_apply_options(IpoptProblem prob, double T, const char *hess_approx,
		int with_mu_target, int with_bound_push){
	const int lowT = eqm_low_temperature(T);
	const double tol = lowT ? 1e-8 : 1e-10;
	const double acceptable_tol = lowT ? 1e-7 : 1e-9;
	const double mu_target = lowT ? 1e-10 : 1e-12;
	const double bound_push = lowT ? 1e-10 : 1e-14;
	const int max_iter = lowT ? 800 : 500;

	AddIpoptNumOption(prob, "tol", tol);
	AddIpoptNumOption(prob, "constr_viol_tol", tol);
	AddIpoptNumOption(prob, "dual_inf_tol", tol);
	AddIpoptNumOption(prob, "compl_inf_tol", tol);
	AddIpoptNumOption(prob, "acceptable_tol", acceptable_tol);
	AddIpoptNumOption(prob, "acceptable_constr_viol_tol", acceptable_tol);
	AddIpoptIntOption(prob, "acceptable_iter", 5);
	if(with_mu_target){
		AddIpoptNumOption(prob, "mu_target", mu_target);
	}
	if(with_bound_push){
		AddIpoptNumOption(prob, "bound_relax_factor", 0.0);
		AddIpoptNumOption(prob, "bound_push", bound_push);
		AddIpoptNumOption(prob, "bound_frac", bound_push);
		AddIpoptNumOption(prob, "slack_bound_push", bound_push);
	}
	AddIpoptIntOption(prob, "max_iter", max_iter);
	AddIpoptStrOption(prob, "hessian_approximation", (char *)hess_approx);
	AddIpoptStrOption(prob, "mu_strategy", (char *)"adaptive");
	AddIpoptIntOption(prob, "print_level", 0);
}

static void eqm_compute_x(const EqmData *D, const Number *xvars, double *S, double *x, double *Z){
	double sum = 0.0;
	int i;
	for(i = 0; i < D->ns; ++i){
		x[i] = exp(xvars[i]);
		sum += x[i];
	}
	for(i = 0; i < D->ns; ++i){
		x[i] /= sum;
	}
	*S = exp(xvars[D->ns]);
	*Z = sum;
}

static int eval_f(Index n, Number *x, Bool new_x, Number *obj_value, UserDataPtr user_data){
	EqmData *D = (EqmData *)user_data;
	double *x_i = NULL;
	double *n_i = NULL;
	double *mu = NULL;
	double S = 0.0;
	double Z = 0.0;
	double G = 0.0;
	(void)new_x;
	(void)n;

	x_i = (double *)calloc((size_t)D->ns, sizeof(double));
	n_i = (double *)calloc((size_t)D->ns, sizeof(double));
	mu = (double *)calloc((size_t)D->ns, sizeof(double));
	if(!x_i || !n_i || !mu){
		free(mu);
		free(n_i);
		free(x_i);
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	eqm_compute_x(D, x, &S, x_i, &Z);
	for(int i = 0; i < D->ns; ++i){
		n_i[i] = S * x_i[i];
	}
	if(!eqm_eval_obj_mu(n_i, D->mu0, D->is_condensed, D->solution_phase_id,
			D->binary_phases, D->nbinary_phases, D->ns, D->T, D->P, D->P0, &G, mu, NULL)){
		free(mu);
		free(n_i);
		free(x_i);
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	free(mu);
	free(n_i);
	free(x_i);
	*obj_value = G / (gas_R() * D->T);
	return TRUE;
}

static int eval_grad_f(Index n, Number *x, Bool new_x, Number *grad_f, UserDataPtr user_data){
	EqmData *D = (EqmData *)user_data;
	const int nvars = D->ns + 1;
	const double eps = 1e-6;
	double *xwork = (double *)calloc((size_t)nvars, sizeof(double));
	(void)new_x;

	if(!xwork){
		for(int i = 0; i < nvars; ++i){
			grad_f[i] = 0.0;
		}
		return TRUE;
	}
	for(int i = 0; i < nvars; ++i){
		xwork[i] = x[i];
	}
	for(int j = 0; j < nvars; ++j){
		Number fp = 0.0;
		Number fm = 0.0;
		xwork[j] = x[j] + eps;
		(void)eval_f(n, xwork, TRUE, &fp, user_data);
		xwork[j] = x[j] - eps;
		(void)eval_f(n, xwork, TRUE, &fm, user_data);
		xwork[j] = x[j];
		if(isfinite(fp) && isfinite(fm)){
			grad_f[j] = (fp - fm) / (2.0 * eps);
		}else{
			grad_f[j] = 0.0;
		}
		if(j < D->ns){
			grad_f[j] *= D->n_scale[j];
		}
	}
	free(xwork);
	return TRUE;
}

static int eval_g(Index n, Number *x, Bool new_x, Index m, Number *g, UserDataPtr user_data){
	EqmData *D = (EqmData *)user_data;
	double *x_i = NULL;
	double S = 0.0;
	double Z = 0.0;
	int e;
	int i;
	(void)n;
	(void)new_x;

	x_i = (double *)calloc((size_t)D->ns, sizeof(double));
	eqm_compute_x(D, x, &S, x_i, &Z);
	for(e = 0; e < D->ne; ++e){
		double sum = 0.0;
		for(i = 0; i < D->ns; ++i){
			sum += D->A[e * D->ns + i] * x_i[i];
		}
		g[e] = (S * sum - D->b[e]) * D->b_scale[e];
	}
	free(x_i);
	return TRUE;
}

static int eval_f_logn(Index n, Number *x, Bool new_x, Number *obj_value, UserDataPtr user_data){
	EqmLogN *D = (EqmLogN *)user_data;
	double *n_i = (double *)calloc((size_t)D->ns, sizeof(double));
	double *mu = (double *)calloc((size_t)D->ns, sizeof(double));
	double logn_tot;
	double G = 0.0;
	(void)new_x;

	(void)logn_tot;
	if(!n_i || !mu){
		free(n_i);
		free(mu);
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	for(int i = 0; i < D->ns; ++i){
		if(D->n_est[i] <= 0.0){
			free(n_i);
			free(mu);
			*obj_value = HUGE_VAL;
			return TRUE;
		}
		n_i[i] = D->n_est[i] * exp(x[i]);
	}
	if(!eqm_eval_obj_mu(n_i, D->mu0, D->is_condensed, D->solution_phase_id,
			D->binary_phases, D->nbinary_phases, D->ns, D->T, D->P, D->P0, &G, mu, NULL)){
		free(n_i);
		free(mu);
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	free(mu);
	free(n_i);
	*obj_value = D->obj_scale * (G / (gas_R() * D->T));
	return TRUE;
}

static int eval_f_n(Index n, Number *x, Bool new_x, Number *obj_value, UserDataPtr user_data){
	EqmN *D = (EqmN *)user_data;
	double *mu = (double *)calloc((size_t)D->ns, sizeof(double));
	double logn_tot;
	double G = 0.0;
	(void)new_x;
	(void)logn_tot;

	if(!mu){
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	for(int i = 0; i < D->ns; ++i){
		if(x[i] <= 0.0){
			free(mu);
			*obj_value = HUGE_VAL;
			return TRUE;
		}
	}
	if(!eqm_eval_obj_mu(x, D->mu0, D->is_condensed, D->solution_phase_id,
			D->binary_phases, D->nbinary_phases, D->ns, D->T, D->P, D->P0, &G, mu, NULL)){
		free(mu);
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	free(mu);
	*obj_value = D->obj_scale * (G / (gas_R() * D->T));
	return TRUE;
}

static int eval_grad_f_logn(Index n, Number *x, Bool new_x, Number *grad_f, UserDataPtr user_data){
	EqmLogN *D = (EqmLogN *)user_data;
	double *n_i = (double *)calloc((size_t)D->ns, sizeof(double));
	double *mu = (double *)calloc((size_t)D->ns, sizeof(double));
	double logn_tot;
	(void)new_x;
	(void)logn_tot;

	if(!n_i || !mu){
		free(n_i);
		free(mu);
		for(int i = 0; i < D->ns; ++i){
			grad_f[i] = 0.0;
		}
		return TRUE;
	}
	for(int i = 0; i < D->ns; ++i){
		if(D->n_est[i] <= 0.0){
			free(n_i);
			free(mu);
			for(int j = 0; j < D->ns; ++j){
				grad_f[j] = 0.0;
			}
			return TRUE;
		}
		n_i[i] = D->n_est[i] * exp(x[i]);
	}
	if(!eqm_eval_obj_mu(n_i, D->mu0, D->is_condensed, D->solution_phase_id,
			D->binary_phases, D->nbinary_phases, D->ns, D->T, D->P, D->P0, NULL, mu, NULL)){
		free(n_i);
		free(mu);
		for(int i = 0; i < D->ns; ++i){
			grad_f[i] = 0.0;
		}
		return TRUE;
	}
	for(int i = 0; i < D->ns; ++i){
		grad_f[i] = D->obj_scale * n_i[i] * (mu[i] / (gas_R() * D->T));
	}
	free(mu);
	free(n_i);
	return TRUE;
}

static int eval_grad_f_n(Index n, Number *x, Bool new_x, Number *grad_f, UserDataPtr user_data){
	EqmN *D = (EqmN *)user_data;
	double *mu = (double *)calloc((size_t)D->ns, sizeof(double));
	double logn_tot;
	(void)new_x;
	(void)logn_tot;

	if(!mu){
		for(int i = 0; i < D->ns; ++i){
			grad_f[i] = 0.0;
		}
		return TRUE;
	}
	for(int i = 0; i < D->ns; ++i){
		if(x[i] <= 0.0){
			free(mu);
			for(int j = 0; j < D->ns; ++j){
				grad_f[j] = 0.0;
			}
			return TRUE;
		}
	}
	if(!eqm_eval_obj_mu(x, D->mu0, D->is_condensed, D->solution_phase_id,
			D->binary_phases, D->nbinary_phases, D->ns, D->T, D->P, D->P0, NULL, mu, NULL)){
		free(mu);
		for(int i = 0; i < D->ns; ++i){
			grad_f[i] = 0.0;
		}
		return TRUE;
	}
	for(int i = 0; i < D->ns; ++i){
		grad_f[i] = D->obj_scale * (mu[i] / (gas_R() * D->T));
	}
	free(mu);
	return TRUE;
}

static int eval_g_logn(Index n, Number *x, Bool new_x, Index m, Number *g, UserDataPtr user_data){
	EqmLogN *D = (EqmLogN *)user_data;
	(void)n;
	(void)new_x;
	(void)m;

	for(int e = 0; e < D->ne; ++e){
		double sum = 0.0;
		for(int i = 0; i < D->ns; ++i){
			sum += D->A[e * D->ns + i] * D->n_est[i] * exp(x[i]);
		}
		g[e] = (sum - D->b[e]) * D->b_scale[e];
	}
	return TRUE;
}

static int eval_g_n(Index n, Number *x, Bool new_x, Index m, Number *g, UserDataPtr user_data){
	EqmN *D = (EqmN *)user_data;
	(void)n;
	(void)new_x;
	(void)m;

	for(int e = 0; e < D->ne; ++e){
		double sum = 0.0;
		for(int i = 0; i < D->ns; ++i){
			sum += D->A[e * D->ns + i] * x[i];
		}
		g[e] = (sum - D->b[e]) * D->b_scale[e];
	}
	return TRUE;
}

static int eval_jac_g_logn(Index n, Number *x, Bool new_x, Index m,
		Index nele_jac, Index *iRow, Index *jCol, Number *values,
		UserDataPtr user_data){
	EqmLogN *D = (EqmLogN *)user_data;
	int e;
	int i;
	(void)n;
	(void)x;
	(void)new_x;
	(void)m;
	(void)nele_jac;

	if(values == NULL){
		Index k = 0;
		for(e = 0; e < D->ne; ++e){
			for(i = 0; i < D->ns; ++i){
				iRow[k] = e;
				jCol[k] = i;
				++k;
			}
		}
	}else{
		Index k = 0;
		for(e = 0; e < D->ne; ++e){
			for(i = 0; i < D->ns; ++i){
				values[k] = D->A[e * D->ns + i] * D->n_est[i] * exp(x[i]) * D->b_scale[e];
				++k;
			}
		}
	}
	return TRUE;
}

static int eval_jac_g_n(Index n, Number *x, Bool new_x, Index m,
		Index nele_jac, Index *iRow, Index *jCol, Number *values,
		UserDataPtr user_data){
	EqmN *D = (EqmN *)user_data;
	int e;
	int i;
	(void)n;
	(void)x;
	(void)new_x;
	(void)m;
	(void)nele_jac;

	if(values == NULL){
		Index k = 0;
		for(e = 0; e < D->ne; ++e){
			for(i = 0; i < D->ns; ++i){
				iRow[k] = e;
				jCol[k] = i;
				++k;
			}
		}
	}else{
		Index k = 0;
		for(e = 0; e < D->ne; ++e){
			for(i = 0; i < D->ns; ++i){
				values[k] = D->A[e * D->ns + i] * D->b_scale[e];
				++k;
			}
		}
	}
	return TRUE;
}

static void eval_grad_L_logn(const EqmLogN *D, const Number *xvars, Number obj_factor,
		const Number *lambda, double *grad){
	double *n_i = (double *)calloc((size_t)D->ns, sizeof(double));
	double *mu = (double *)calloc((size_t)D->ns, sizeof(double));
	double logn_tot;
	(void)logn_tot;

	if(!n_i || !mu){
		free(n_i);
		free(mu);
		for(int i = 0; i < D->ns; ++i){
			grad[i] = 0.0;
		}
		return;
	}
	for(int i = 0; i < D->ns; ++i){
		if(D->n_est[i] <= 0.0){
			free(n_i);
			free(mu);
			for(int j = 0; j < D->ns; ++j){
				grad[j] = 0.0;
			}
			return;
		}
		n_i[i] = D->n_est[i] * exp(xvars[i]);
	}
	if(!eqm_eval_obj_mu(n_i, D->mu0, D->is_condensed, D->solution_phase_id,
			D->binary_phases, D->nbinary_phases, D->ns, D->T, D->P, D->P0, NULL, mu, NULL)){
		free(n_i);
		free(mu);
		for(int i = 0; i < D->ns; ++i){
			grad[i] = 0.0;
		}
		return;
	}
	for(int i = 0; i < D->ns; ++i){
		double val = obj_factor * D->obj_scale * n_i[i] * (mu[i] / (gas_R() * D->T));
		for(int e = 0; e < D->ne; ++e){
			val += lambda[e] * D->A[e * D->ns + i] * n_i[i] * D->b_scale[e];
		}
		grad[i] = val;
	}
	free(mu);
	free(n_i);
}

static void eval_grad_L_n(const EqmN *D, const Number *xvars, Number obj_factor,
		const Number *lambda, double *grad){
	double *mu = (double *)calloc((size_t)D->ns, sizeof(double));
	double logn_tot;
	(void)logn_tot;

	if(!mu){
		for(int i = 0; i < D->ns; ++i){
			grad[i] = 0.0;
		}
		return;
	}
	for(int i = 0; i < D->ns; ++i){
		if(xvars[i] <= 0.0){
			free(mu);
			for(int j = 0; j < D->ns; ++j){
				grad[j] = 0.0;
			}
			return;
		}
	}
	if(!eqm_eval_obj_mu(xvars, D->mu0, D->is_condensed, D->solution_phase_id,
			D->binary_phases, D->nbinary_phases, D->ns, D->T, D->P, D->P0, NULL, mu, NULL)){
		free(mu);
		for(int i = 0; i < D->ns; ++i){
			grad[i] = 0.0;
		}
		return;
	}
	for(int i = 0; i < D->ns; ++i){
		double val = obj_factor * D->obj_scale * (mu[i] / (gas_R() * D->T));
		for(int e = 0; e < D->ne; ++e){
			val += lambda[e] * D->A[e * D->ns + i] * D->b_scale[e];
		}
		grad[i] = val;
	}
	free(mu);
}

static int eval_h_logn(Index n, Number *x, Bool new_x, Number obj_factor,
		Index m, Number *lambda, Bool new_lambda,
		Index nele_hess, Index *iRow, Index *jCol, Number *values,
		UserDataPtr user_data){
	EqmLogN *D = (EqmLogN *)user_data;
	(void)new_x;
	(void)new_lambda;
	(void)m;
	(void)nele_hess;

	if(values == NULL){
		Index k = 0;
		for(Index j = 0; j < n; ++j){
			for(Index i = j; i < n; ++i){
				iRow[k] = i;
				jCol[k] = j;
				++k;
			}
		}
	}else{
		const double eps = 1e-6;
		double *gradp = (double *)calloc((size_t)n, sizeof(double));
		double *gradm = (double *)calloc((size_t)n, sizeof(double));
		double *xwork = (double *)calloc((size_t)n, sizeof(double));
		Index k = 0;
		for(Index i = 0; i < n; ++i){
			xwork[i] = x[i];
		}
		for(Index j = 0; j < n; ++j){
			xwork[j] = x[j] + eps;
			eval_grad_L_logn(D, xwork, obj_factor, lambda, gradp);
			xwork[j] = x[j] - eps;
			eval_grad_L_logn(D, xwork, obj_factor, lambda, gradm);
			xwork[j] = x[j];
			for(Index i = j; i < n; ++i){
				values[k] = (gradp[i] - gradm[i]) / (2.0 * eps);
				++k;
			}
		}
		free(xwork);
		free(gradm);
		free(gradp);
	}
	return TRUE;
}

static int eval_h_n(Index n, Number *x, Bool new_x, Number obj_factor,
		Index m, Number *lambda, Bool new_lambda,
		Index nele_hess, Index *iRow, Index *jCol, Number *values,
		UserDataPtr user_data){
	EqmN *D = (EqmN *)user_data;
	(void)new_x;
	(void)new_lambda;
	(void)m;
	(void)nele_hess;

	if(values == NULL){
		Index k = 0;
		for(Index j = 0; j < n; ++j){
			for(Index i = j; i < n; ++i){
				iRow[k] = i;
				jCol[k] = j;
				++k;
			}
		}
	}else{
		const double eps = 1e-6;
		double *gradp = (double *)calloc((size_t)n, sizeof(double));
		double *gradm = (double *)calloc((size_t)n, sizeof(double));
		double *xwork = (double *)calloc((size_t)n, sizeof(double));
		Index k = 0;
		for(Index i = 0; i < n; ++i){
			xwork[i] = x[i];
		}
		for(Index j = 0; j < n; ++j){
			xwork[j] = x[j] + eps;
			eval_grad_L_n(D, xwork, obj_factor, lambda, gradp);
			xwork[j] = x[j] - eps;
			eval_grad_L_n(D, xwork, obj_factor, lambda, gradm);
			xwork[j] = x[j];
			for(Index i = j; i < n; ++i){
				values[k] = (gradp[i] - gradm[i]) / (2.0 * eps);
				++k;
			}
		}
		free(xwork);
		free(gradm);
		free(gradp);
	}
	return TRUE;
}

static int eval_jac_g(Index n, Number *x, Bool new_x, Index m,
		Index nele_jac, Index *iRow, Index *jCol, Number *values,
		UserDataPtr user_data){
	EqmData *D = (EqmData *)user_data;
	int e;
	int i;
	(void)n;
	(void)x;
	(void)new_x;
	(void)m;
	(void)nele_jac;

	if(values == NULL){
		Index k = 0;
		for(e = 0; e < D->ne; ++e){
			for(i = 0; i < D->ns; ++i){
				iRow[k] = e;
				jCol[k] = i;
				++k;
			}
			iRow[k] = e;
			jCol[k] = D->ns;
			++k;
		}
	}else{
		double *x_i = (double *)calloc((size_t)D->ns, sizeof(double));
		double *B_e = (double *)calloc((size_t)D->ne, sizeof(double));
		double S = 0.0;
		double Z = 0.0;
		Index k = 0;
		eqm_compute_x(D, x, &S, x_i, &Z);
		for(e = 0; e < D->ne; ++e){
			for(i = 0; i < D->ns; ++i){
				B_e[e] += D->A[e * D->ns + i] * x_i[i];
			}
		}
		for(e = 0; e < D->ne; ++e){
			for(i = 0; i < D->ns; ++i){
				values[k] = S * x_i[i] * (D->A[e * D->ns + i] - B_e[e]) * D->b_scale[e] * D->n_scale[i];
				++k;
			}
			values[k] = S * B_e[e] * D->b_scale[e];
			++k;
		}
		free(B_e);
		free(x_i);
	}
	return TRUE;
}

static void eval_grad_L(const EqmData *D, const Number *xvars, Number obj_factor,
		const Number *lambda, double *grad){
	const int nvars = D->ns + 1;
	double *grad_obj = (double *)calloc((size_t)nvars, sizeof(double));
	double *x_i = (double *)calloc((size_t)D->ns, sizeof(double));
	double *B_e = (double *)calloc((size_t)D->ne, sizeof(double));
	double S = 0.0;
	double Z = 0.0;
	int i;
	int e;

	if(!grad_obj || !x_i || !B_e){
		free(grad_obj);
		free(B_e);
		free(x_i);
		for(i = 0; i < nvars; ++i){
			grad[i] = 0.0;
		}
		return;
	}

	(void)eval_grad_f((Index)nvars, (Number *)xvars, FALSE, grad_obj, (UserDataPtr)D);
	eqm_compute_x(D, xvars, &S, x_i, &Z);
	for(e = 0; e < D->ne; ++e){
		for(i = 0; i < D->ns; ++i){
			B_e[e] += D->A[e * D->ns + i] * x_i[i];
		}
	}
	for(i = 0; i < D->ns; ++i){
		double val = obj_factor * grad_obj[i];
		for(e = 0; e < D->ne; ++e){
			val += lambda[e] * S * x_i[i] * (D->A[e * D->ns + i] - B_e[e]) * D->b_scale[e] * D->n_scale[i];
		}
		grad[i] = val;
	}
	grad[D->ns] = obj_factor * grad_obj[D->ns];
	for(e = 0; e < D->ne; ++e){
		grad[D->ns] += lambda[e] * S * B_e[e] * D->b_scale[e];
	}

	free(grad_obj);
	free(B_e);
	free(x_i);
}

static int eval_h(Index n, Number *x, Bool new_x, Number obj_factor,
		Index m, Number *lambda, Bool new_lambda,
		Index nele_hess, Index *iRow, Index *jCol, Number *values,
		UserDataPtr user_data){
	EqmData *D = (EqmData *)user_data;
	int i;
	int j;
	(void)n;
	(void)new_x;
	(void)m;
	(void)new_lambda;
	(void)nele_hess;

	if(values == NULL){
		Index k = 0;
		const int nvars = D->ns + 1;
		for(j = 0; j < nvars; ++j){
			for(i = j; i < nvars; ++i){
				iRow[k] = i;
				jCol[k] = j;
				++k;
			}
		}
	}else{
		const double eps = 1e-6;
		const int nvars = D->ns + 1;
		double *gradp = (double *)calloc((size_t)nvars, sizeof(double));
		double *gradm = (double *)calloc((size_t)nvars, sizeof(double));
		double *xwork = (double *)calloc((size_t)nvars, sizeof(double));
		Index k = 0;

		for(i = 0; i < nvars; ++i){
			xwork[i] = x[i];
		}
		for(j = 0; j < nvars; ++j){
			xwork[j] = x[j] + eps;
			eval_grad_L(D, xwork, obj_factor, lambda, gradp);
			xwork[j] = x[j] - eps;
			eval_grad_L(D, xwork, obj_factor, lambda, gradm);
			xwork[j] = x[j];
			for(i = j; i < nvars; ++i){
				values[k] = (gradp[i] - gradm[i]) / (2.0 * eps);
				++k;
			}
		}
		free(xwork);
		free(gradm);
		free(gradp);
	}
	return TRUE;
}

static double eqm_obj_1d(double z, const double *n0, const double *v, int ns,
		const double *mu0, const int *is_condensed, double T, double P, double P0){
	double G = 0.0;
	double *n = (double *)calloc((size_t)ns, sizeof(double));
	double *mu = (double *)calloc((size_t)ns, sizeof(double));
	if(!n || !mu){
		free(n);
		free(mu);
		return HUGE_VAL;
	}
	for(int i = 0; i < ns; ++i){
		double n_i = n0[i] + v[i] * z;
		if(n_i <= 0.0){
			free(n);
			free(mu);
			return HUGE_VAL;
		}
		n[i] = n_i;
	}
	if(!eqm_eval_obj_mu(n, mu0, is_condensed, NULL, NULL, 0, ns, T, P, P0, &G, mu, NULL)){
		free(n);
		free(mu);
		return HUGE_VAL;
	}
	free(mu);
	free(n);
	return G;
}

static double eqm_log10K_from_n(const double *n, double P, double P0){
	double logn[3];
	double logn_tot;
	double logp_h2;
	double logp_o2;
	double logp_h2o;
	const double ln10 = log(10.0);

	if(n[2] <= 0.0){
		return HUGE_VAL;
	}
	if(n[0] <= 0.0 || n[1] <= 0.0){
		return -HUGE_VAL;
	}
	for(int i = 0; i < 3; ++i){
		logn[i] = log(n[i]);
	}
	logn_tot = eqm_logsumexp(logn, 3);
	if(!isfinite(logn_tot)){
		return HUGE_VAL;
	}
	logp_h2 = logn[0] - logn_tot + log(P);
	logp_o2 = logn[1] - logn_tot + log(P);
	logp_h2o = logn[2] - logn_tot + log(P);
	return (logp_h2 / ln10) + 0.5 * (logp_o2 / ln10) - (logp_h2o / ln10)
		- 0.5 * log10(P0);
}

static int eqm_sign(double x){
	if(x > 0.0){
		return 1;
	}
	if(x < 0.0){
		return -1;
	}
	return 0;
}

static double eqm_log10K_from_nu(const double *n, const double *nu, int ns, double P, double P0){
	double sum = 0.0;
	double nu_sum = 0.0;
	double *logn = (double *)calloc((size_t)ns, sizeof(double));
	double logn_tot;
	const double ln10 = log(10.0);

	if(!logn){
		return HUGE_VAL;
	}
	for(int i = 0; i < ns; ++i){
		if(n[i] <= 0.0){
			free(logn);
			if(nu[i] > 0.0){
				return -HUGE_VAL;
			}
			if(nu[i] < 0.0){
				return HUGE_VAL;
			}
			return HUGE_VAL;
		}
		logn[i] = log(n[i]);
	}
	logn_tot = eqm_logsumexp(logn, ns);
	if(!isfinite(logn_tot)){
		free(logn);
		return HUGE_VAL;
	}
	for(int i = 0; i < ns; ++i){
		double logp = logn[i] - logn_tot + log(P);
		sum += nu[i] * (logp / ln10);
		nu_sum += nu[i];
	}
	free(logn);
	sum -= log10(P0) * nu_sum;
	return sum;
}

static double eqm_phi_1d(const double *n0, const double *v, int ns, const double *mu0,
		const int *is_condensed, double T, double P, double P0, double z){
	double phi = 0.0;
	double *n = (double *)calloc((size_t)ns, sizeof(double));
	double *mu = (double *)calloc((size_t)ns, sizeof(double));
	if(!n){
		free(mu);
		return HUGE_VAL;
	}
	if(!mu){
		free(n);
		return HUGE_VAL;
	}
	for(int i = 0; i < ns; ++i){
		n[i] = n0[i] + v[i] * z;
		if(n[i] < DBL_MIN){
			n[i] = DBL_MIN;
		}
	}
	if(!eqm_eval_obj_mu(n, mu0, is_condensed, NULL, NULL, 0, ns, T, P, P0, NULL, mu, NULL)){
		free(n);
		free(mu);
		return HUGE_VAL;
	}
	for(int i = 0; i < ns; ++i){
		phi += v[i] * mu[i];
	}
	free(n);
	free(mu);
	return phi;
}

static int eqm_phi_root_1d(const double *n0, const double *v, int ns, const double *mu0,
		const int *is_condensed, double T, double P, double P0, double zmin, double zmax,
		double *z_out){
	double a = zmin;
	double bnd = zmax;
	double fa = eqm_phi_1d(n0, v, ns, mu0, is_condensed, T, P, P0, a);
	double fb = eqm_phi_1d(n0, v, ns, mu0, is_condensed, T, P, P0, bnd);
	if(eqm_sign(fa) * eqm_sign(fb) > 0){
		return 0;
	}
	for(int i = 0; i < 200; ++i){
		double mid = 0.5 * (a + bnd);
		double fmid = eqm_phi_1d(n0, v, ns, mu0, is_condensed, T, P, P0, mid);
		if(fmid == 0.0){
			*z_out = mid;
			return 1;
		}
		if(eqm_sign(fa) * eqm_sign(fmid) < 0){
			bnd = mid;
			fb = fmid;
		}else{
			a = mid;
			fa = fmid;
		}
	}
	*z_out = 0.5 * (a + bnd);
	return 1;
}

static double eqm_log10K_from_delta_nu(const double *n0, const double *v, int ns,
		double z_edge, double delta, int sign, const double *nu, double P, double P0){
	double *n_tmp = (double *)calloc((size_t)ns, sizeof(double));
	double log10K = HUGE_VAL;
	if(!n_tmp){
		return HUGE_VAL;
	}
	for(int i = 0; i < ns; ++i){
		n_tmp[i] = n0[i] + v[i] * (z_edge + sign * delta);
	}
	log10K = eqm_log10K_from_nu(n_tmp, nu, ns, P, P0);
	free(n_tmp);
	return log10K;
}

static double eqm_logK_resid_1d(double z, const double *n0, const double *v,
		double log10K_target, double P, double P0){
	double n_tmp[3];
	double log10K;

	n_tmp[0] = n0[0] + v[0] * z;
	n_tmp[1] = n0[1] + v[1] * z;
	n_tmp[2] = n0[2] + v[2] * z;
	if(n_tmp[0] <= 0.0 || n_tmp[1] <= 0.0 || n_tmp[2] <= 0.0){
		return HUGE_VAL;
	}
	log10K = eqm_log10K_from_n(n_tmp, P, P0);
	if(!isfinite(log10K)){
		return HUGE_VAL;
	}
	log10K -= log10K_target;
	return log10K * log10K;
}

static double eqm_log10K_from_delta(const double *n0, const double *v, double zmax,
		double delta, double P, double P0){
	double n_tmp[3];
	int i;
	for(i = 0; i < 3; ++i){
		double n_bound = n0[i] + v[i] * zmax;
		n_tmp[i] = n_bound - v[i] * delta;
	}
	return eqm_log10K_from_n(n_tmp, P, P0);
}

static int eqm_nullspace_logK_root(const double *n0, const double *v, double zmin, double zmax,
		double log10K_target, double P, double P0, double *z_out){
	double a = zmin;
	double bnd = zmax;
	double fa;
	double fb;
	double mid;
	double fmid;
	int i;
	double n_tmp[3];

	n_tmp[0] = n0[0] + v[0] * a;
	n_tmp[1] = n0[1] + v[1] * a;
	n_tmp[2] = n0[2] + v[2] * a;
	fa = eqm_log10K_from_n(n_tmp, P, P0) - log10K_target;

	n_tmp[0] = n0[0] + v[0] * bnd;
	n_tmp[1] = n0[1] + v[1] * bnd;
	n_tmp[2] = n0[2] + v[2] * bnd;
	fb = eqm_log10K_from_n(n_tmp, P, P0) - log10K_target;

	if(!(fa == fa) || !(fb == fb) || fa * fb > 0.0){
		return 0;
	}

	for(i = 0; i < 80; ++i){
		mid = 0.5 * (a + bnd);
		n_tmp[0] = n0[0] + v[0] * mid;
		n_tmp[1] = n0[1] + v[1] * mid;
		n_tmp[2] = n0[2] + v[2] * mid;
		fmid = eqm_log10K_from_n(n_tmp, P, P0) - log10K_target;
		if(fmid == 0.0 || fabs(bnd - a) < 1e-12){
			*z_out = mid;
			return 1;
		}
		if(fa * fmid < 0.0){
			bnd = mid;
			fb = fmid;
		}else{
			a = mid;
			fa = fmid;
		}
	}
	*z_out = 0.5 * (a + bnd);
	return 1;
}

static int eqm_nullspace_logK_root_nu(const double *n0, const double *v, int ns, double zmin,
		double zmax, const double *nu, double log10K_target, double P, double P0, double *z_out){
	double a = zmin;
	double bnd = zmax;
	double fa;
	double fb;
	double mid;
	double fmid;
	double *n_tmp = (double *)calloc((size_t)ns, sizeof(double));
	if(!n_tmp){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		n_tmp[i] = n0[i] + v[i] * a;
	}
	fa = eqm_log10K_from_nu(n_tmp, nu, ns, P, P0) - log10K_target;
	for(int i = 0; i < ns; ++i){
		n_tmp[i] = n0[i] + v[i] * bnd;
	}
	fb = eqm_log10K_from_nu(n_tmp, nu, ns, P, P0) - log10K_target;
	if(!(fa == fa) || !(fb == fb) || fa * fb > 0.0){
		free(n_tmp);
		return 0;
	}
	for(int i = 0; i < 80; ++i){
		mid = 0.5 * (a + bnd);
		for(int j = 0; j < ns; ++j){
			n_tmp[j] = n0[j] + v[j] * mid;
		}
		fmid = eqm_log10K_from_nu(n_tmp, nu, ns, P, P0) - log10K_target;
		if(fmid == 0.0 || fabs(bnd - a) < 1e-12){
			*z_out = mid;
			free(n_tmp);
			return 1;
		}
		if(fa * fmid < 0.0){
			bnd = mid;
			fb = fmid;
		}else{
			a = mid;
			fa = fmid;
		}
	}
	*z_out = 0.5 * (a + bnd);
	free(n_tmp);
	return 1;
}

static int eqm_nullspace_logK_min_edge_nu(const double *n0, const double *v, int ns,
		double z_edge, int sign, double delta_max, const double *nu, double log10K_target,
		double P, double P0, double *z_out, double *resid_out){
	const double delta_min = 1e-300;
	double s_lo;
	double s_hi;
	double phi = (sqrt(5.0) - 1.0) / 2.0;
	double a;
	double bnd;
	double c;
	double d;
	double fc;
	double fd;
	if(delta_max <= delta_min){
		return 0;
	}
	s_lo = log(delta_min);
	s_hi = log(delta_max);
	a = s_lo;
	bnd = s_hi;
	c = bnd - phi * (bnd - a);
	d = a + phi * (bnd - a);
	fc = eqm_log10K_from_delta_nu(n0, v, ns, z_edge, exp(c), sign, nu, P, P0) - log10K_target;
	fd = eqm_log10K_from_delta_nu(n0, v, ns, z_edge, exp(d), sign, nu, P, P0) - log10K_target;
	fc *= fc;
	fd *= fd;
	for(int i = 0; i < 80; ++i){
		if(fc < fd){
			bnd = d;
			d = c;
			fd = fc;
			c = bnd - phi * (bnd - a);
			fc = eqm_log10K_from_delta_nu(n0, v, ns, z_edge, exp(c), sign, nu, P, P0) - log10K_target;
			fc *= fc;
		}else{
			a = c;
			c = d;
			fc = fd;
			d = a + phi * (bnd - a);
			fd = eqm_log10K_from_delta_nu(n0, v, ns, z_edge, exp(d), sign, nu, P, P0) - log10K_target;
			fd *= fd;
		}
	}
	{
		double s = 0.5 * (a + bnd);
		double delta = exp(s);
		*z_out = z_edge + sign * delta;
		*resid_out = eqm_log10K_from_delta_nu(n0, v, ns, z_edge, delta, sign, nu, P, P0)
			- log10K_target;
		*resid_out *= *resid_out;
	}
	return 1;
}

static int eqm_nullspace_logK_root_upper(const double *n0, const double *v, double zmin, double zmax,
		double log10K_target, double P, double P0, double *z_out, double *delta_out){
	double delta_max = zmax - zmin;
	double delta_min = DBL_MIN;
	double s_lo;
	double s_hi;
	double f_lo;
	double f_hi;
	int i;

	if(delta_max <= 0.0){
		return 0;
	}
	if(delta_min >= delta_max){
		delta_min = delta_max * 1e-12;
		if(delta_min <= 0.0){
			return 0;
		}
	}
	s_lo = log(delta_min);
	s_hi = log(delta_max);
	f_lo = eqm_log10K_from_delta(n0, v, zmax, exp(s_lo), P, P0) - log10K_target;
	f_hi = eqm_log10K_from_delta(n0, v, zmax, exp(s_hi), P, P0) - log10K_target;
	if(!(f_lo == f_lo) || !(f_hi == f_hi) || f_lo * f_hi > 0.0){
		return 0;
	}
	for(i = 0; i < 90; ++i){
		double s_mid = 0.5 * (s_lo + s_hi);
		double delta = exp(s_mid);
		double f_mid = eqm_log10K_from_delta(n0, v, zmax, delta, P, P0) - log10K_target;
		if(f_mid == 0.0 || fabs(s_hi - s_lo) < 1e-12){
			*z_out = zmax - delta;
			*delta_out = delta;
			return 1;
		}
		if(f_mid < 0.0){
			s_lo = s_mid;
			f_lo = f_mid;
		}else{
			s_hi = s_mid;
			f_hi = f_mid;
		}
	}
	*z_out = zmax - exp(0.5 * (s_lo + s_hi));
	*delta_out = exp(0.5 * (s_lo + s_hi));
	return 1;
}

static int eqm_nullspace_logK_bracket(const double *n0, const double *v, double zmin, double zmax,
		double log10K_target, double P, double P0, double *zlo, double *zhi){
	int i;
	double n_tmp[3];
	double za = zmin;
	double zb = zmax;
	double fa;
	double fb;

	n_tmp[0] = n0[0] + v[0] * za;
	n_tmp[1] = n0[1] + v[1] * za;
	n_tmp[2] = n0[2] + v[2] * za;
	fa = eqm_log10K_from_n(n_tmp, P, P0) - log10K_target;

	n_tmp[0] = n0[0] + v[0] * zb;
	n_tmp[1] = n0[1] + v[1] * zb;
	n_tmp[2] = n0[2] + v[2] * zb;
	fb = eqm_log10K_from_n(n_tmp, P, P0) - log10K_target;

	if(fa * fb < 0.0){
		*zlo = za;
		*zhi = zb;
		return 1;
	}

	for(i = 1; i <= 60; ++i){
		double t = (double)i / 60.0;
		double z = za + t * (zb - za);
		n_tmp[0] = n0[0] + v[0] * z;
		n_tmp[1] = n0[1] + v[1] * z;
		n_tmp[2] = n0[2] + v[2] * z;
		double f = eqm_log10K_from_n(n_tmp, P, P0) - log10K_target;
		if(fa * f < 0.0){
			*zlo = za;
			*zhi = z;
			return 1;
		}
		za = z;
		fa = f;
	}

	return 0;
}

static int eqm_nullspace_bounds(const double *n0, const double *v, int ns,
		double *zmin, double *zmax){
	int i;
	double lo = -HUGE_VAL;
	double hi = HUGE_VAL;
	for(i = 0; i < ns; ++i){
		if(v[i] > 0.0){
			double z = -n0[i] / v[i];
			if(z > lo){
				lo = z;
			}
		}else if(v[i] < 0.0){
			double z = -n0[i] / v[i];
			if(z < hi){
				hi = z;
			}
		}else if(n0[i] <= 0.0){
			return 0;
		}
	}
	if(lo >= hi){
		return 0;
	}
	*zmin = lo;
	*zmax = hi;
	return 1;
}

static int eqm_nullspace_n0(const double *A, const double *b, double *n0){
	/* Solve A n = b with one variable fixed to 0, for ns=3, ne=2. */
	int k;
	for(k = 0; k < 3; ++k){
		int i0 = (k + 1) % 3;
		int i1 = (k + 2) % 3;
		double a00 = A[0 * 3 + i0];
		double a01 = A[0 * 3 + i1];
		double a10 = A[1 * 3 + i0];
		double a11 = A[1 * 3 + i1];
		double det = a00 * a11 - a01 * a10;
		if(fabs(det) < 1e-12){
			continue;
		}
		n0[k] = 0.0;
		n0[i0] = (b[0] * a11 - b[1] * a01) / det;
		n0[i1] = (a00 * b[1] - a10 * b[0]) / det;
		if(n0[i0] > 0.0 && n0[i1] > 0.0){
			return 1;
		}
	}
	return 0;
}

static void eqm_nullspace_v(const double *A, double *v){
	/* v = row0 x row1 for ns=3, ne=2. */
	double a0 = A[0 * 3 + 0];
	double a1 = A[0 * 3 + 1];
	double a2 = A[0 * 3 + 2];
	double b0 = A[1 * 3 + 0];
	double b1 = A[1 * 3 + 1];
	double b2 = A[1 * 3 + 2];
	v[0] = a1 * b2 - a2 * b1;
	v[1] = a2 * b0 - a0 * b2;
	v[2] = a0 * b1 - a1 * b0;
}

int eqm_ipopt_nullspace_1d(const char **names, const char **elements, const char *source,
		const double *b, double T, double P, double *n_out){
	double A[6];
	double mu0[3];
	double n0[3];
	double v[3];
	double zmin;
	double zmax;
	double P0 = 1e5;
	double log10K_target;
	double phi = (sqrt(5.0) - 1.0) / 2.0;
	double a;
	double bnd;
	double c;
	double d;
	double fc;
	double fd;
	int i;
	int idx_h2 = -1;
	int idx_o2 = -1;
	int idx_h2o = -1;

	if(!fprops_build_element_matrix_source(names, 3, elements, 2, source, A)){
		return 0;
	}
	if(!eqm_compute_mu0(names, 3, source, T, P0, mu0)){
		return 0;
	}
	for(i = 0; i < 3; ++i){
		if(0 == strcmp(names[i], "hydrogen")) idx_h2 = i;
		if(0 == strcmp(names[i], "oxygen")) idx_o2 = i;
		if(0 == strcmp(names[i], "water")) idx_h2o = i;
	}
	if(!eqm_nullspace_n0(A, b, n0)){
		return 0;
	}
	eqm_nullspace_v(A, v);
	if(!eqm_nullspace_bounds(n0, v, 3, &zmin, &zmax)){
		return 0;
	}

	if(idx_h2 >= 0 && idx_o2 >= 0 && idx_h2o >= 0){
		double dG = mu0[idx_h2] + 0.5 * mu0[idx_o2] - mu0[idx_h2o];
		log10K_target = -dG / (gas_R() * T * log(10.0));
		if(isfinite(log10K_target)){
			if(eqm_nullspace_logK_bracket(n0, v, zmin, zmax, log10K_target, P, P0, &a, &bnd) &&
					eqm_nullspace_logK_root(n0, v, a, bnd, log10K_target, P, P0, &a)){
				double n_tmp[3];
				double log10K_check;
				for(i = 0; i < 3; ++i){
					n_tmp[i] = n0[i] + v[i] * a;
				}
				log10K_check = eqm_log10K_from_n(n_tmp, P, P0);
				if(isfinite(log10K_check) && fabs(log10K_check - log10K_target) <= 1e-2){
					for(i = 0; i < 3; ++i){
						n_out[i] = n_tmp[i];
					}
					return 1;
				}
			}
			{
				double delta = 0.0;
				if(eqm_nullspace_logK_root_upper(n0, v, zmin, zmax, log10K_target, P, P0, &a, &delta)){
					for(i = 0; i < 3; ++i){
						double n_bound = n0[i] + v[i] * zmax;
						n_out[i] = n_bound - v[i] * delta;
					}
					return 1;
				}
			}
			a = zmin;
			bnd = zmax;
			c = bnd - phi * (bnd - a);
			d = a + phi * (bnd - a);
			fc = eqm_logK_resid_1d(c, n0, v, log10K_target, P, P0);
			fd = eqm_logK_resid_1d(d, n0, v, log10K_target, P, P0);
			for(i = 0; i < 80; ++i){
				if(fc < fd){
					bnd = d;
					d = c;
					fd = fc;
					c = bnd - phi * (bnd - a);
					fc = eqm_logK_resid_1d(c, n0, v, log10K_target, P, P0);
				}else{
					a = c;
					c = d;
					fc = fd;
					d = a + phi * (bnd - a);
					fd = eqm_logK_resid_1d(d, n0, v, log10K_target, P, P0);
				}
			}
			a = 0.5 * (a + bnd);
			if(isfinite(eqm_logK_resid_1d(a, n0, v, log10K_target, P, P0))){
				for(i = 0; i < 3; ++i){
					n_out[i] = n0[i] + v[i] * a;
				}
				return 1;
			}
		}
	}

	a = zmin;
	bnd = zmax;
	c = bnd - phi * (bnd - a);
	d = a + phi * (bnd - a);
	fc = eqm_obj_1d(c, n0, v, 3, mu0, NULL, T, P, P0);
	fd = eqm_obj_1d(d, n0, v, 3, mu0, NULL, T, P, P0);
	for(i = 0; i < 80; ++i){
		if(fc < fd){
			bnd = d;
			d = c;
			fd = fc;
			c = bnd - phi * (bnd - a);
			fc = eqm_obj_1d(c, n0, v, 3, mu0, NULL, T, P, P0);
		}else{
			a = c;
			c = d;
			fc = fd;
			d = a + phi * (bnd - a);
			fd = eqm_obj_1d(d, n0, v, 3, mu0, NULL, T, P, P0);
		}
	}
	{
		double z = 0.5 * (a + bnd);
		for(i = 0; i < 3; ++i){
			n_out[i] = n0[i] + v[i] * z;
		}
	}
	return 1;
}

int eqm_ipopt_nullspace_seed_r1(const EqmNullspace *M, double *n_seed){
	double zmin;
	double zmax;
	double z;
	double log10K_target;
	double sum = 0.0;
	double span;
	double z_best = 0.0;
	double r_best = HUGE_VAL;
	double z_try;
	double r_try;
	const double *nu;

	if(!M || !n_seed || M->r != 1){
		return 0;
	}
	if(!eqm_nullspace_bounds(M->n0, M->N, M->ns, &zmin, &zmax)){
		return 0;
	}
	if(eqm_phi_root_1d(M->n0, M->N, M->ns, M->mu0, M->is_condensed,
			M->T, M->P, M->P0, zmin, zmax, &z)){
		for(int i = 0; i < M->ns; ++i){
			n_seed[i] = M->n0[i] + M->N[i] * z;
		}
		return 1;
	}
	for(int i = 0; i < M->ns; ++i){
		if(M->is_condensed && M->is_condensed[i]){
			return 0;
		}
	}
	nu = M->N;
	for(int i = 0; i < M->ns; ++i){
		sum += nu[i] * M->mu0[i];
	}
	log10K_target = -sum / (gas_R() * M->T * log(10.0));
	if(eqm_nullspace_logK_root_nu(M->n0, M->N, M->ns, zmin, zmax, nu, log10K_target,
			M->P, M->P0, &z)){
		for(int i = 0; i < M->ns; ++i){
			n_seed[i] = M->n0[i] + M->N[i] * z;
		}
		return 1;
	}
	span = zmax - zmin;
	if(eqm_nullspace_logK_min_edge_nu(M->n0, M->N, M->ns, zmax, -1, span, nu, log10K_target,
			M->P, M->P0, &z_try, &r_try)){
		if(r_try < r_best){
			r_best = r_try;
			z_best = z_try;
		}
	}
	if(eqm_nullspace_logK_min_edge_nu(M->n0, M->N, M->ns, zmin, 1, span, nu, log10K_target,
			M->P, M->P0, &z_try, &r_try)){
		if(r_try < r_best){
			r_best = r_try;
			z_best = z_try;
		}
	}
	if(isfinite(r_best)){
		for(int i = 0; i < M->ns; ++i){
			n_seed[i] = M->n0[i] + M->N[i] * z_best;
		}
		return 1;
	}
	return 0;
}

static double eqm_log10K_target_nu(const double *nu, const double *mu0, int ns, double T){
	double sum = 0.0;
	for(int i = 0; i < ns; ++i){
		sum += nu[i] * mu0[i];
	}
	return -sum / (gas_R() * T * log(10.0));
}

int eqm_ipopt_nullspace_logK_solution_r1(const EqmNullspace *M, double *n_out){
	double zmin;
	double zmax;
	double z;
	double span;
	double z_best = 0.0;
	double r_best = HUGE_VAL;
	double z_try;
	double r_try;
	double log10K_target;
	double v_scale = 0.0;
	double *v = NULL;
	double phi = (sqrt(5.0) - 1.0) / 2.0;

	if(!M || !n_out || M->r != 1){
		return 0;
	}
	for(int i = 0; i < M->ns; ++i){
		if(M->is_condensed && M->is_condensed[i]){
			return 0;
		}
	}
	for(int i = 0; i < M->ns; ++i){
		double vabs = fabs(M->N[i]);
		if(vabs > v_scale){
			v_scale = vabs;
		}
	}
	if(v_scale <= 0.0){
		return 0;
	}
	v = (double *)calloc((size_t)M->ns, sizeof(double));
	if(!v){
		return 0;
	}
	for(int i = 0; i < M->ns; ++i){
		v[i] = M->N[i] / v_scale;
	}
	if(!eqm_nullspace_bounds(M->n0, v, M->ns, &zmin, &zmax)){
		free(v);
		return 0;
	}
	log10K_target = eqm_log10K_target_nu(M->N, M->mu0, M->ns, M->T);
	if(eqm_nullspace_logK_root_nu(M->n0, v, M->ns, zmin, zmax, M->N,
			log10K_target, M->P, M->P0, &z)){
		for(int i = 0; i < M->ns; ++i){
			n_out[i] = M->n0[i] + v[i] * z;
		}
		free(v);
		return 1;
	}
	{
		double a = zmin;
		double b = zmax;
		double c = b - phi * (b - a);
		double d = a + phi * (b - a);
		double *n_tmp = (double *)calloc((size_t)M->ns, sizeof(double));
		double fc;
		double fd;
		if(!n_tmp){
			free(v);
			return 0;
		}
		for(int i = 0; i < M->ns; ++i){
			n_tmp[i] = M->n0[i] + v[i] * c;
		}
		fc = eqm_log10K_from_nu(n_tmp, M->N, M->ns, M->P, M->P0) - log10K_target;
		fc = fc * fc;
		for(int i = 0; i < M->ns; ++i){
			n_tmp[i] = M->n0[i] + v[i] * d;
		}
		fd = eqm_log10K_from_nu(n_tmp, M->N, M->ns, M->P, M->P0) - log10K_target;
		fd = fd * fd;
		for(int iter = 0; iter < 100; ++iter){
			if(fc < fd){
				b = d;
				d = c;
				fd = fc;
				c = b - phi * (b - a);
				for(int i = 0; i < M->ns; ++i){
					n_tmp[i] = M->n0[i] + v[i] * c;
				}
				fc = eqm_log10K_from_nu(n_tmp, M->N, M->ns, M->P, M->P0) - log10K_target;
				fc = fc * fc;
			}else{
				a = c;
				c = d;
				fc = fd;
				d = a + phi * (b - a);
				for(int i = 0; i < M->ns; ++i){
					n_tmp[i] = M->n0[i] + v[i] * d;
				}
				fd = eqm_log10K_from_nu(n_tmp, M->N, M->ns, M->P, M->P0) - log10K_target;
				fd = fd * fd;
			}
		}
		z_best = 0.5 * (a + b);
		for(int i = 0; i < M->ns; ++i){
			n_out[i] = M->n0[i] + v[i] * z_best;
		}
		free(n_tmp);
		free(v);
		return 1;
	}
	span = zmax - zmin;
	if(eqm_nullspace_logK_min_edge_nu(M->n0, v, M->ns, zmax, -1, span, M->N, log10K_target,
			M->P, M->P0, &z_try, &r_try)){
		if(r_try < r_best){
			r_best = r_try;
			z_best = z_try;
		}
	}
	if(eqm_nullspace_logK_min_edge_nu(M->n0, v, M->ns, zmin, 1, span, M->N, log10K_target,
			M->P, M->P0, &z_try, &r_try)){
		if(r_try < r_best){
			r_best = r_try;
			z_best = z_try;
		}
	}
	if(isfinite(r_best)){
		for(int i = 0; i < M->ns; ++i){
			n_out[i] = M->n0[i] + v[i] * z_best;
		}
		free(v);
		return 1;
	}
	free(v);
	return 0;
}

static void eqm_nullspace_compute_n(const EqmNullspace *M, const Number *z, double *n, double *n_tot){
	double total = 0.0;
	for(int i = 0; i < M->ns; ++i){
		double val = M->n0[i];
		for(int j = 0; j < M->r; ++j){
			val += M->N[i * M->r + j] * z[j];
		}
		n[i] = val;
		total += val;
	}
	*n_tot = total;
}

static int eqm_solve_linear(double *A, double *b, int n, double *x){
	const double tol = 1e-14;
	for(int i = 0; i < n; ++i){
		int piv = i;
		double maxabs = fabs(A[i * n + i]);
		for(int r = i + 1; r < n; ++r){
			double val = fabs(A[r * n + i]);
			if(val > maxabs){
				maxabs = val;
				piv = r;
			}
		}
		if(maxabs < tol){
			return 0;
		}
		if(piv != i){
			for(int c = i; c < n; ++c){
				double tmp = A[i * n + c];
				A[i * n + c] = A[piv * n + c];
				A[piv * n + c] = tmp;
			}
			{
				double tmpb = b[i];
				b[i] = b[piv];
				b[piv] = tmpb;
			}
		}
		for(int r = i + 1; r < n; ++r){
			double factor = A[r * n + i] / A[i * n + i];
			if(fabs(factor) < tol){
				continue;
			}
			for(int c = i; c < n; ++c){
				A[r * n + c] -= factor * A[i * n + c];
			}
			b[r] -= factor * b[i];
		}
	}
	for(int i = n - 1; i >= 0; --i){
		double sum = b[i];
		for(int c = i + 1; c < n; ++c){
			sum -= A[i * n + c] * x[c];
		}
		x[i] = sum / A[i * n + i];
	}
	return 1;
}

static int eqm_next_combination(int *idx, int k, int n){
	for(int i = k - 1; i >= 0; --i){
		if(idx[i] < n - k + i){
			++idx[i];
			for(int j = i + 1; j < k; ++j){
				idx[j] = idx[j - 1] + 1;
			}
			return 1;
		}
	}
	return 0;
}

static int eqm_nullspace_lp_init(const EqmNullspace *M, double *z_out){
	const int nvars = M->r + 1;
	const int mcon = M->ns + 1;
	const double tol = 1e-10;
	double best_t = -HUGE_VAL;
	int found = 0;
	double *A = (double *)calloc((size_t)(mcon * nvars), sizeof(double));
	double *b = (double *)calloc((size_t)mcon, sizeof(double));
	int *idx = (int *)calloc((size_t)nvars, sizeof(int));
	double *Aeq = (double *)calloc((size_t)(nvars * nvars), sizeof(double));
	double *beq = (double *)calloc((size_t)nvars, sizeof(double));
	double *x = (double *)calloc((size_t)nvars, sizeof(double));
	if(!A || !b || !idx || !Aeq || !beq || !x){
		free(A);
		free(b);
		free(idx);
		free(Aeq);
		free(beq);
		free(x);
		return 0;
	}
	for(int i = 0; i < M->ns; ++i){
		for(int j = 0; j < M->r; ++j){
			A[i * nvars + j] = M->N[i * M->r + j];
		}
		A[i * nvars + M->r] = -1.0;
		b[i] = -M->n0[i];
	}
	for(int j = 0; j < M->r; ++j){
		A[M->ns * nvars + j] = 0.0;
	}
	A[M->ns * nvars + M->r] = 1.0;
	b[M->ns] = M->n_min;

	for(int i = 0; i < nvars; ++i){
		idx[i] = i;
	}
	do{
		for(int r = 0; r < nvars; ++r){
			for(int c = 0; c < nvars; ++c){
				Aeq[r * nvars + c] = A[idx[r] * nvars + c];
			}
			beq[r] = b[idx[r]];
		}
		for(int i = 0; i < nvars; ++i){
			x[i] = 0.0;
		}
		if(!eqm_solve_linear(Aeq, beq, nvars, x)){
			continue;
		}
		{
			int ok = 1;
			for(int r = 0; r < mcon; ++r){
				double lhs = 0.0;
				for(int c = 0; c < nvars; ++c){
					lhs += A[r * nvars + c] * x[c];
				}
				if(lhs + tol < b[r]){
					ok = 0;
					break;
				}
			}
			if(ok && x[M->r] > best_t){
				best_t = x[M->r];
				for(int j = 0; j < M->r; ++j){
					z_out[j] = x[j];
				}
				found = 1;
			}
		}
	}while(eqm_next_combination(idx, nvars, mcon));

	free(A);
	free(b);
	free(idx);
	free(Aeq);
	free(beq);
	free(x);
	return found;
}

static Bool eval_f_ns(Index n, Number *x, Bool new_x, Number *obj_value, UserDataPtr user_data){
	EqmNullspace *M = (EqmNullspace *)user_data;
	double *n_i = (double *)calloc((size_t)M->ns, sizeof(double));
	double *mu = (double *)calloc((size_t)M->ns, sizeof(double));
	double G = 0.0;
	double barrier = 0.0;
	double n_tot = 0.0;
	(void)new_x;

	if(!n_i || !mu){
		free(n_i);
		free(mu);
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	eqm_nullspace_compute_n(M, x, n_i, &n_tot);
	if(n_tot <= 0.0 || !eqm_eval_obj_mu(n_i, M->mu0, M->is_condensed, M->solution_phase_id,
			M->binary_phases, M->nbinary_phases, M->ns, M->T, M->P, M->P0, &G, mu, NULL)){
		free(n_i);
		free(mu);
		*obj_value = HUGE_VAL;
		return TRUE;
	}
	for(int i = 0; i < M->ns; ++i){
		if(n_i[i] <= 0.0){
			free(n_i);
			free(mu);
			*obj_value = HUGE_VAL;
			return TRUE;
		}
		barrier += log(n_i[i]);
	}
	free(mu);
	free(n_i);
	*obj_value = M->obj_scale * (G / (gas_R() * M->T)) - M->barrier_tau * barrier;
	return TRUE;
}

static Bool eval_grad_f_ns(Index n, Number *x, Bool new_x, Number *grad_f, UserDataPtr user_data){
	EqmNullspace *M = (EqmNullspace *)user_data;
	double *n_i = (double *)calloc((size_t)M->ns, sizeof(double));
	double n_tot = 0.0;
	double *mu = (double *)calloc((size_t)M->ns, sizeof(double));
	(void)new_x;

	if(!n_i || !mu){
		free(n_i);
		free(mu);
		for(int j = 0; j < M->r; ++j){
			grad_f[j] = 0.0;
		}
		return TRUE;
	}
	eqm_nullspace_compute_n(M, x, n_i, &n_tot);
	if(n_tot <= 0.0
			|| !eqm_eval_obj_mu(n_i, M->mu0, M->is_condensed, M->solution_phase_id,
				M->binary_phases, M->nbinary_phases, M->ns, M->T, M->P, M->P0,
				NULL, mu, NULL)){
		free(n_i);
		free(mu);
		for(int j = 0; j < M->r; ++j){
			grad_f[j] = 0.0;
		}
		return TRUE;
	}
	for(int i = 0; i < M->ns; ++i){
		if(n_i[i] <= 0.0){
			free(n_i);
			free(mu);
			for(int j = 0; j < M->r; ++j){
				grad_f[j] = 0.0;
			}
			return TRUE;
		}
	}
	for(int j = 0; j < M->r; ++j){
		double sum = 0.0;
		double barrier = 0.0;
		for(int i = 0; i < M->ns; ++i){
			sum += mu[i] * M->N[i * M->r + j];
			barrier += M->N[i * M->r + j] / n_i[i];
		}
		grad_f[j] = M->obj_scale * (sum / (gas_R() * M->T)) - M->barrier_tau * barrier;
	}
	free(n_i);
	free(mu);
	return TRUE;
}

static Bool eval_g_ns(Index n, Number *x, Bool new_x, Index m, Number *g, UserDataPtr user_data){
	EqmNullspace *M = (EqmNullspace *)user_data;
	double *n_i = (double *)calloc((size_t)M->ns, sizeof(double));
	double n_tot = 0.0;
	(void)new_x;
	(void)m;

	eqm_nullspace_compute_n(M, x, n_i, &n_tot);
	if(n_tot <= 0.0){
		free(n_i);
		for(int i = 0; i < M->ns; ++i){
			g[i] = -HUGE_VAL;
		}
		return TRUE;
	}
	for(int i = 0; i < M->ns; ++i){
		g[i] = n_i[i];
	}
	free(n_i);
	return TRUE;
}

static Bool eval_jac_g_ns(Index n, Number *x, Bool new_x, Index m, Index nele_jac,
		Index *iRow, Index *jCol, Number *values, UserDataPtr user_data){
	EqmNullspace *M = (EqmNullspace *)user_data;
	int idx = 0;
	(void)n;
	(void)x;
	(void)new_x;
	(void)m;
	(void)nele_jac;

	if(values == NULL){
		for(int i = 0; i < M->ns; ++i){
			for(int j = 0; j < M->r; ++j){
				iRow[idx] = i;
				jCol[idx] = j;
				++idx;
			}
		}
	}else{
		for(int i = 0; i < M->ns; ++i){
			for(int j = 0; j < M->r; ++j){
				values[idx] = M->N[i * M->r + j];
				++idx;
			}
		}
	}
	return TRUE;
}

static void eqm_nullspace_grad_L(const EqmNullspace *M, const Number *x, Number obj_factor,
		const Number *lambda, Number *grad_L){
	double *n_i = (double *)calloc((size_t)M->ns, sizeof(double));
	double n_tot = 0.0;
	double *mu = (double *)calloc((size_t)M->ns, sizeof(double));
	if(!n_i || !mu){
		free(n_i);
		free(mu);
		for(int j = 0; j < M->r; ++j){
			grad_L[j] = 0.0;
		}
		return;
	}

	eqm_nullspace_compute_n(M, x, n_i, &n_tot);
	if(n_tot <= 0.0
			|| !eqm_eval_obj_mu(n_i, M->mu0, M->is_condensed, M->solution_phase_id,
				M->binary_phases, M->nbinary_phases, M->ns, M->T, M->P, M->P0,
				NULL, mu, NULL)){
		free(n_i);
		free(mu);
		for(int j = 0; j < M->r; ++j){
			grad_L[j] = 0.0;
		}
		return;
	}
	for(int j = 0; j < M->r; ++j){
		double sum = 0.0;
		double barrier = 0.0;
		for(int i = 0; i < M->ns; ++i){
			sum += mu[i] * M->N[i * M->r + j];
			barrier += M->N[i * M->r + j] / n_i[i];
		}
		grad_L[j] = obj_factor * (M->obj_scale * (sum / (gas_R() * M->T)));
		grad_L[j] -= obj_factor * M->barrier_tau * barrier;
		for(int i = 0; i < M->ns; ++i){
			grad_L[j] += lambda[i] * M->N[i * M->r + j];
		}
	}
	free(n_i);
	free(mu);
}

static Bool eval_h_ns(Index n, Number *x, Bool new_x, Number obj_factor,
		Index m, Number *lambda, Bool new_lambda, Index nele_hess,
		Index *iRow, Index *jCol, Number *values, UserDataPtr user_data){
	EqmNullspace *M = (EqmNullspace *)user_data;
	(void)new_x;
	(void)new_lambda;
	(void)m;
	(void)nele_hess;
	if(values == NULL){
		int idx = 0;
		for(Index j = 0; j < n; ++j){
			for(Index i = j; i < n; ++i){
				iRow[idx] = i;
				jCol[idx] = j;
				++idx;
			}
		}
	}else{
		const double eps = 1e-6;
		Number *xwork = (Number *)calloc((size_t)n, sizeof(Number));
		Number *gradm = (Number *)calloc((size_t)n, sizeof(Number));
		Number *gradp = (Number *)calloc((size_t)n, sizeof(Number));
		int idx = 0;
		for(Index k = 0; k < n; ++k){
			xwork[k] = x[k];
		}
		for(Index j = 0; j < n; ++j){
			xwork[j] = x[j] + eps;
			eqm_nullspace_grad_L(M, xwork, obj_factor, lambda, gradp);
			xwork[j] = x[j] - eps;
			eqm_nullspace_grad_L(M, xwork, obj_factor, lambda, gradm);
			xwork[j] = x[j];
			for(Index i = j; i < n; ++i){
				values[idx] = (gradp[i] - gradm[i]) / (2.0 * eps);
				++idx;
			}
		}
		free(xwork);
		free(gradm);
		free(gradp);
	}
	return TRUE;
}

int eqm_ipopt_nullspace_create(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, EqmNullspace **out){
	EqmNullspace *M;
	int r = 0;
	double max_mu0 = 0.0;

	if(!names || !elements || !b || !out || ns <= 0 || ne <= 0){
		return 0;
	}
	if(eqm_has_solution_phases(names, ns, source)){
		return 0;
	}
	M = (EqmNullspace *)calloc(1, sizeof(*M));
	if(!M){
		return 0;
	}
	M->ns = ns;
	M->ne = ne;
	M->T = T;
	M->P = P;
	M->P0 = 1e5;
	M->n_min = 1e-30;
	M->obj_scale = 1.0;
	M->barrier_tau = 1e-6;
	M->A = (double *)calloc((size_t)(ne * ns), sizeof(double));
	M->b = (double *)calloc((size_t)ne, sizeof(double));
	M->mu0 = (double *)calloc((size_t)ns, sizeof(double));
	M->is_condensed = (int *)calloc((size_t)ns, sizeof(int));
	M->n0 = (double *)calloc((size_t)ns, sizeof(double));
	if(!M->A || !M->b || !M->mu0 || !M->is_condensed || !M->n0){
		eqm_ipopt_nullspace_destroy(M);
		return 0;
	}
	for(int e = 0; e < ne; ++e){
		M->b[e] = b[e];
	}
	if(!fprops_build_element_matrix_source(names, ns, elements, ne, source, M->A)){
		eqm_ipopt_nullspace_destroy(M);
		return 0;
	}
	if(!eqm_compute_mu0(names, ns, source, T, M->P0, M->mu0)){
		eqm_ipopt_nullspace_destroy(M);
		return 0;
	}
	if(!eqm_compute_is_condensed(names, ns, source, M->is_condensed)){
		eqm_ipopt_nullspace_destroy(M);
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		double v = fabs(M->mu0[i]);
		if(v > max_mu0){
			max_mu0 = v;
		}
	}
	if(max_mu0 > 0.0){
		double denom = max_mu0 / (gas_R() * T);
		if(denom > 1.0){
			M->obj_scale = 1.0 / denom;
		}
	}
	if(!eqm_solve_particular(M->A, M->b, ne, ns, M->n0)){
		eqm_ipopt_nullspace_destroy(M);
		return 0;
	}
	{
		int *pivots = (int *)calloc((size_t)ne, sizeof(int));
		double *Awork = (double *)calloc((size_t)(ne * ns), sizeof(double));
		int rank = 0;
		if(!pivots || !Awork){
			free(pivots);
			free(Awork);
			eqm_ipopt_nullspace_destroy(M);
			return 0;
		}
		for(int i = 0; i < ne * ns; ++i){
			Awork[i] = M->A[i];
		}
		eqm_rref(Awork, ne, ns, pivots, &rank);
		r = ns - rank;
		M->r = r;
		if(M->r > 0){
			M->N = (double *)calloc((size_t)(ns * M->r), sizeof(double));
			if(!M->N){
				free(pivots);
				free(Awork);
				eqm_ipopt_nullspace_destroy(M);
				return 0;
			}
			eqm_fill_nullspace(Awork, ne, ns, pivots, rank, M->N, M->r);
		}else{
			M->N = NULL;
		}
		free(pivots);
		free(Awork);
	}
	*out = M;
	return 1;
}

int eqm_seed_from_nullspace_r1(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_seed){
	EqmNullspace *M = NULL;
	int ok = 0;

	if(!n_seed){
		return 0;
	}
	if(!eqm_ipopt_nullspace_create(names, ns, elements, ne, source, b, T, P, &M)){
		return 0;
	}
	if(M->r == 1){
		if(eqm_ipopt_nullspace_seed_r1(M, n_seed) ||
				eqm_ipopt_nullspace_logK_solution_r1(M, n_seed)){
			ok = 1;
		}
	}
	eqm_ipopt_nullspace_destroy(M);
	return ok;
}

int eqm_ipopt_nullspace_solve(EqmNullspace *M, const double *z_init, double *n_out){
	Index nvars;
	Index mcon;
	Index nele_jac;
	Index nele_hess;
	Number *x_L;
	Number *x_U;
	Number *g_L;
	Number *g_U;
	Number *x;
	IpoptProblem prob;
	Number obj;
	int status;
	int have_best = 0;
	int best_status = -1;
	Number *x_best = NULL;
	static const double tau_seq[] = {1e-12, 1e-14, 1e-16};
	const size_t tau_count = sizeof(tau_seq) / sizeof(tau_seq[0]);

	if(!M || !n_out){
		return -11;
	}
	if(M->r == 0){
		for(int i = 0; i < M->ns; ++i){
			if(M->n0[i] <= 0.0){
				return -12;
			}
			n_out[i] = M->n0[i];
		}
		return 0;
	}
	nvars = (Index)M->r;
	mcon = (Index)M->ns;
	nele_jac = nvars * mcon;
	nele_hess = nvars * (nvars + 1) / 2;

	x_L = (Number *)calloc((size_t)nvars, sizeof(Number));
	x_U = (Number *)calloc((size_t)nvars, sizeof(Number));
	g_L = (Number *)calloc((size_t)mcon, sizeof(Number));
	g_U = (Number *)calloc((size_t)mcon, sizeof(Number));
	x = (Number *)calloc((size_t)nvars, sizeof(Number));
	x_best = (Number *)calloc((size_t)nvars, sizeof(Number));
	if(!x_L || !x_U || !g_L || !g_U || !x || !x_best){
		free(x_L);
		free(x_U);
		free(g_L);
		free(g_U);
		free(x);
		free(x_best);
		return -13;
	}
	for(int i = 0; i < nvars; ++i){
		x_L[i] = -1e19;
		x_U[i] = 1e19;
		x[i] = z_init ? z_init[i] : 0.0;
	}
	for(int i = 0; i < mcon; ++i){
		g_L[i] = M->n_min;
		g_U[i] = 1e19;
	}
	if(!z_init){
		(void)eqm_nullspace_lp_init(M, x);
	}
	{
		Number obj = 0.0;
		Number *grad = (Number *)calloc((size_t)nvars, sizeof(Number));
		Number *g = (Number *)calloc((size_t)mcon, sizeof(Number));
		Number *jac = (Number *)calloc((size_t)nele_jac, sizeof(Number));
		Number *hess = (Number *)calloc((size_t)nele_hess, sizeof(Number));
		Number *lambda = (Number *)calloc((size_t)mcon, sizeof(Number));
		double *n_chk = (double *)calloc((size_t)M->ns, sizeof(double));
		double n_tot = 0.0;
		double n_min = HUGE_VAL;
		int n_min_idx = -1;
		int bad = 0;
		if(!grad || !g || !jac || !hess || !lambda || !n_chk){
			free(n_chk);
			free(grad);
			free(g);
			free(jac);
			free(hess);
			free(lambda);
			free(x_L);
			free(x_U);
			free(g_L);
			free(g_U);
			free(x);
			return -13;
		}
		eqm_nullspace_compute_n(M, x, n_chk, &n_tot);
		for(int i = 0; i < M->ns; ++i){
			if(n_chk[i] < n_min){
				n_min = n_chk[i];
				n_min_idx = i;
			}
		}
		eval_f_ns(nvars, x, FALSE, &obj, (UserDataPtr)M);
		if(!isfinite(obj)){
			bad = 1;
		}
		eval_grad_f_ns(nvars, x, FALSE, grad, (UserDataPtr)M);
		for(Index i = 0; i < nvars; ++i){
			if(!isfinite(grad[i])){
				bad = 1;
				break;
			}
		}
		eval_g_ns(nvars, x, FALSE, mcon, g, (UserDataPtr)M);
		for(Index i = 0; i < mcon; ++i){
			if(!isfinite(g[i])){
				bad = 1;
				break;
			}
		}
		eval_jac_g_ns(nvars, x, FALSE, mcon, nele_jac, NULL, NULL, jac, (UserDataPtr)M);
		for(Index i = 0; i < nele_jac; ++i){
			if(!isfinite(jac[i])){
				bad = 1;
				break;
			}
		}
		eval_h_ns(nvars, x, FALSE, 1.0, mcon, lambda, FALSE, nele_hess, NULL, NULL, hess,
			(UserDataPtr)M);
		for(Index i = 0; i < nele_hess; ++i){
			if(!isfinite(hess[i])){
				bad = 1;
				break;
			}
		}
		free(grad);
		free(g);
		free(jac);
		free(hess);
		free(lambda);
		free(n_chk);
		if(bad){
			fprintf(stderr, "eqm_ipopt_nullspace_solve: precheck failed for initial z\n");
			fprintf(stderr, "  n_tot=%.6g, n_min=%.6g (idx %d)\n", n_tot, n_min, n_min_idx);
			fprintf(stderr, "  obj=%g\n", obj);
			free(x_L);
			free(x_U);
			free(g_L);
			free(g_U);
			free(x);
			return -31;
		}
	}
	prob = CreateIpoptProblem(nvars, x_L, x_U, mcon, g_L, g_U,
			nele_jac, nele_hess, 0,
			eval_f_ns, eval_g_ns, eval_grad_f_ns, eval_jac_g_ns, eval_h_ns);
	if(!prob){
		free(x_L);
		free(x_U);
		free(g_L);
		free(g_U);
		free(x);
		free(x_best);
		return -14;
	}
	eqm_ipopt_apply_options(prob, M->T, "exact", 0, 0);
	for(size_t t = 0; t < tau_count; ++t){
		M->barrier_tau = tau_seq[t];
		status = IpoptSolve(prob, x, NULL, &obj, NULL, NULL, NULL, (UserDataPtr)M);
		if(status == 0 || status == 1 || status == 6){
			for(int i = 0; i < nvars; ++i){
				x_best[i] = x[i];
			}
			have_best = 1;
			best_status = status;
		}else{
			break;
		}
	}
	FreeIpoptProblem(prob);
	if(have_best){
		for(int i = 0; i < nvars; ++i){
			x[i] = x_best[i];
		}
		status = best_status;
	}
	for(int i = 0; i < M->ns; ++i){
		double val = M->n0[i];
		for(int j = 0; j < M->r; ++j){
			val += M->N[i * M->r + j] * x[j];
		}
		n_out[i] = val;
	}
	free(x_L);
	free(x_U);
	free(g_L);
	free(g_U);
	free(x);
	free(x_best);
	return status;
}

void eqm_ipopt_nullspace_destroy(EqmNullspace *M){
	if(!M){
		return;
	}
	free(M->A);
	free(M->b);
	free(M->mu0);
	free(M->is_condensed);
	eqm_free_solution_phases(&M->solution_phase_id, &M->solution_member_index, &M->binary_phases);
	free(M->n0);
	free(M->N);
	free(M);
}

static int eqm_ipopt_r1_logK_ok(const EqmNullspace *M, const double *n, double log10_tol){
	double log10K_target;
	double log10K_check;
	if(!M || !n || M->r != 1){
		return 1;
	}
	for(int i = 0; i < M->ns; ++i){
		if(M->is_condensed && M->is_condensed[i]){
			return 1;
		}
	}
	log10K_target = eqm_log10K_target_nu(M->N, M->mu0, M->ns, M->T);
	log10K_check = eqm_log10K_from_nu(n, M->N, M->ns, M->P, M->P0);
	if(!isfinite(log10K_target) || !isfinite(log10K_check)){
		return 0;
	}
	return fabs(log10K_check - log10K_target) <= log10_tol;
}

int eqm_ipopt_nullspace_solve_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out){
	EqmNullspace *M = NULL;
	double *n_seed = NULL;
	const double log10_tol_r1 = 0.2;
	int status;

	if(!eqm_ipopt_nullspace_create(names, ns, elements, ne, source, b, T, P, &M)){
		return -21;
	}
	if(M->r == 1){
		n_seed = (double *)calloc((size_t)ns, sizeof(double));
		if(n_seed && eqm_ipopt_nullspace_seed_r1(M, n_seed)){
			status = eqm_ipopt_solve_elements_source_init(names, ns, elements, ne,
				source, b, T, P, n_seed, n_out);
			if(status == 0 || status == 1 || status == 6){
				if(eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return status;
				}
				if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
						&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return 0;
				}
			}
			status = eqm_ipopt_solve_elements_n_source_init(names, ns, elements, ne,
				source, b, T, P, n_seed, n_out);
			if(status == 0 || status == 1 || status == 6){
				if(eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return status;
				}
				if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
						&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return 0;
				}
			}
			status = eqm_ipopt_solve_elements_logn_source_init(names, ns, elements, ne,
				source, b, T, P, n_seed, n_out);
			if(status == 0 || status == 1 || status == 6){
				if(eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return status;
				}
				if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
						&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return 0;
				}
			}
#ifdef HAVE_NLOPT
			status = eqm_slsqp_solve_elements_source_init(names, ns, elements, ne,
				source, b, T, P, n_seed, n_out);
			if(status == 0){
				if(eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return 0;
				}
				if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
						&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					free(n_seed);
					eqm_ipopt_nullspace_destroy(M);
					return 0;
				}
			}
#endif
		}
		status = eqm_ipopt_solve_elements_source(names, ns, elements, ne, source, b, T, P, n_out);
		if(status == 0 || status == 1 || status == 6){
			if(eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
				free(n_seed);
				eqm_ipopt_nullspace_destroy(M);
				return status;
			}
			if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
					&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
				free(n_seed);
				eqm_ipopt_nullspace_destroy(M);
				return 0;
			}
		}
	}
	status = eqm_ipopt_solve_elements_n_source(names, ns, elements, ne, source, b, T, P, n_out);
	if(status == 0 || status == 1 || status == 6){
		if(M->r == 1){
			if(!eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
				if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
						&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					status = 0;
				}else{
					status = -22;
				}
			}
		}
		if(status == 0 || status == 1 || status == 6){
			free(n_seed);
			eqm_ipopt_nullspace_destroy(M);
			return status;
		}
	}
	status = eqm_ipopt_solve_elements_logn_source(names, ns, elements, ne, source, b, T, P, n_out);
	if(status == 0 || status == 1 || status == 6){
		if(M->r == 1){
			if(!eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
				if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
						&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					status = 0;
				}else{
					status = -22;
				}
			}
		}
		if(status == 0 || status == 1 || status == 6){
			free(n_seed);
			eqm_ipopt_nullspace_destroy(M);
			return status;
		}
	}
#ifdef HAVE_NLOPT
	status = eqm_slsqp_solve_elements_source(names, ns, elements, ne, source, b, T, P, n_out);
	if(status == 0){
		free(n_seed);
		eqm_ipopt_nullspace_destroy(M);
		return 0;
	}
#endif
	if(M->r >= 1){
		status = eqm_ipopt_nullspace_solve(M, NULL, n_out);
		if(status == 0 || status == 1 || status == 6){
			if(M->r == 1 && !eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
				if(eqm_ipopt_nullspace_logK_solution_r1(M, n_out)
						&& eqm_ipopt_r1_logK_ok(M, n_out, log10_tol_r1)){
					status = 0;
				}else{
					status = -22;
				}
			}
		}
		if(status == 0 || status == 1 || status == 6){
			free(n_seed);
			eqm_ipopt_nullspace_destroy(M);
			return status;
		}
	}
	if(M->r == 1){
		double zmin;
		double zmax;
		double zmin_full;
		double zmax_full;
		double z;
		double log10K_target = 0.0;
		double log10K_check = 0.0;
		double sum = 0.0;
		if(!eqm_nullspace_bounds(M->n0, M->N, M->ns, &zmin, &zmax)){
			eqm_ipopt_nullspace_destroy(M);
			return -22;
		}
		zmin_full = zmin;
		zmax_full = zmax;
		if(eqm_phi_root_1d(M->n0, M->N, M->ns, M->mu0, M->is_condensed,
				T, P, M->P0, zmin, zmax, &z)){
			for(int i = 0; i < M->ns; ++i){
				n_out[i] = M->n0[i] + M->N[i] * z;
			}
			status = 0;
		}else{
			status = -22;
		}
		if(status != 0){
			double span = zmax_full - zmin_full;
			double z_best = 0.0;
			double r_best = HUGE_VAL;
			double z_try;
			double r_try;
			for(int i = 0; i < M->ns; ++i){
				sum += M->N[i] * M->mu0[i];
			}
			log10K_target = -sum / (gas_R() * T * log(10.0));
			if(eqm_nullspace_logK_min_edge_nu(M->n0, M->N, M->ns, zmax_full, -1,
					span, M->N, log10K_target, P, M->P0, &z_try, &r_try)){
				if(r_try < r_best){
					r_best = r_try;
					z_best = z_try;
				}
			}
			if(eqm_nullspace_logK_min_edge_nu(M->n0, M->N, M->ns, zmin_full, 1,
					span, M->N, log10K_target, P, M->P0, &z_try, &r_try)){
				if(r_try < r_best){
					r_best = r_try;
					z_best = z_try;
				}
			}
			if(isfinite(r_best)){
				for(int i = 0; i < M->ns; ++i){
					n_out[i] = M->n0[i] + M->N[i] * z_best;
				}
				log10K_check = eqm_log10K_from_nu(n_out, M->N, M->ns, P, M->P0);
				if(isfinite(log10K_check) && fabs(log10K_check - log10K_target) <= 0.2){
					status = 0;
				}else{
					status = -22;
				}
			}else{
				status = -22;
			}
		}
	}else{
		status = -22;
	}
	free(n_seed);
	eqm_ipopt_nullspace_destroy(M);
	return status;
}

int eqm_ipopt_solve_source_init(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const double *n_init, double *n_out){
	EqmData D;
	Index n;
	Index m;
	Index nele_jac;
	Index nele_hess;
	Number *x_L;
	Number *x_U;
	Number *g_L;
	Number *g_U;
	Number *x;
	IpoptProblem prob;
	Number obj;
	int status;
	int i;

	if(ns <= 0 || ne <= 0 || !names || !A || !b || !n_out){
		return -11;
	}

	D.ns = ns;
	D.ne = ne;
	D.T = T;
	D.P = P;
	D.P0 = 1e5;
	D.A = A;
	D.b = b;
	D.solution_phase_id = NULL;
	D.solution_member_index = NULL;
	D.nbinary_phases = 0;
	D.binary_phases = NULL;
	D.mu0 = (double *)calloc((size_t)D.ns, sizeof(double));
	D.is_condensed = (int *)calloc((size_t)D.ns, sizeof(int));
	eqm_apply_bscale(&D);
	eqm_apply_nscale(&D, n_init);
	if(!D.mu0 || !D.is_condensed
			|| !eqm_compute_mu0(names, D.ns, source, D.T, D.P0, D.mu0)
			|| !eqm_compute_is_condensed(names, D.ns, source, D.is_condensed)){
		free(D.mu0);
		free(D.is_condensed);
		free(D.b_scale);
		free(D.n_scale);
		return -11;
	}
	if(!eqm_compute_solution_phases(names, D.ns, source, &D.solution_phase_id,
			&D.solution_member_index, &D.binary_phases, &D.nbinary_phases)){
		free(D.mu0);
		free(D.is_condensed);
		free(D.b_scale);
		free(D.n_scale);
		return -11;
	}

	n = D.ns + 1;
	m = D.ne;
	nele_jac = (Index)(D.ne * (D.ns + 1));
	nele_hess = (Index)((D.ns + 1) * (D.ns + 2) / 2);
	x_L = (Number *)calloc((size_t)n, sizeof(Number));
	x_U = (Number *)calloc((size_t)n, sizeof(Number));
	g_L = (Number *)calloc((size_t)m, sizeof(Number));
	g_U = (Number *)calloc((size_t)m, sizeof(Number));
	x = (Number *)calloc((size_t)n, sizeof(Number));

	for(i = 0; i < D.ns; ++i){
		x_L[i] = -100.0; /* log(x_i) */
		x_U[i] = 30.0;
		x[i] = 0.0;
	}
	x_L[D.ns] = -100.0; /* log(n_tot) */
	x_U[D.ns] = 30.0;
	x[D.ns] = 0.0;
	if(n_init){
		double n_tot = 0.0;
		int ok_init = 1;
		for(i = 0; i < D.ns; ++i){
			if(n_init[i] <= 0.0){
				ok_init = 0;
				break;
			}
			n_tot += n_init[i];
		}
		if(ok_init && n_tot > 0.0){
			for(i = 0; i < D.ns; ++i){
				double xi = n_init[i] / n_tot;
				if(xi <= 0.0){
					ok_init = 0;
					break;
				}
				x[i] = log(xi);
			}
			if(ok_init){
				x[D.ns] = log(n_tot);
			}
		}
	}
	for(i = 0; i < D.ne; ++i){
		g_L[i] = 0.0;
		g_U[i] = 0.0;
	}

	prob = CreateIpoptProblem(
		n, x_L, x_U,
		m, g_L, g_U,
		nele_jac, nele_hess, 0,
		eval_f, eval_g, eval_grad_f, eval_jac_g, eval_h
	);
	eqm_ipopt_apply_options(prob, D.T, "limited-memory", 0, 0);

	status = IpoptSolve(prob, x, NULL, &obj, NULL, NULL, NULL, (UserDataPtr)&D);
	{
		double sumx = 0.0;
		double S = exp(x[D.ns]);
		for(i = 0; i < D.ns; ++i){
			sumx += exp(x[i]);
		}
		if(!(sumx > 0.0) || !isfinite(sumx) || !isfinite(S)){
			status = -13;
			for(i = 0; i < D.ns; ++i){
				n_out[i] = NAN;
			}
		}else{
			for(i = 0; i < D.ns; ++i){
				n_out[i] = S * exp(x[i]) / sumx;
			}
		}
	}

	FreeIpoptProblem(prob);
	free(D.mu0);
	free(D.is_condensed);
	eqm_free_solution_phases(&D.solution_phase_id, &D.solution_member_index, &D.binary_phases);
	free(x_L);
	free(x_U);
	free(g_L);
	free(g_U);
	free(x);
	free(D.n_scale);
	free(D.b_scale);
	return status;
}

int eqm_ipopt_solve_logn_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out){
	EqmLogN D;
	const size_t ns_count = (size_t)ns;
	const size_t ne_count = (size_t)ne;
	double max_mu0 = 0.0;
	Index n;
	Index m;
	Index nele_jac;
	Index nele_hess;
	Number *x_L;
	Number *x_U;
	Number *g_L;
	Number *g_U;
	Number *x;
	IpoptProblem prob;
	Number obj;
	int status;
	int i;

	if(ns <= 0 || ne <= 0 || !names || !A || !b || !n_out){
		return -11;
	}

	D.ns = ns;
	D.ne = ne;
	D.T = T;
	D.P = P;
	D.P0 = 1e5;
	D.obj_scale = 1.0;
	D.A = A;
	D.b = b;
	D.solution_phase_id = NULL;
	D.solution_member_index = NULL;
	D.nbinary_phases = 0;
	D.binary_phases = NULL;
	D.mu0 = (double *)calloc((size_t)D.ns, sizeof(double));
	D.is_condensed = (int *)calloc((size_t)D.ns, sizeof(int));
	D.n_est = (double *)calloc((size_t)D.ns, sizeof(double));
	eqm_apply_bscale_logn(&D);
	if(!D.mu0 || !D.is_condensed || !D.n_est){
		free(D.mu0);
		free(D.is_condensed);
		free(D.n_est);
		free(D.b_scale);
		return -11;
	}
	if(!eqm_compute_solution_phases(names, D.ns, source, &D.solution_phase_id,
			&D.solution_member_index, &D.binary_phases, &D.nbinary_phases)){
		free(D.mu0);
		free(D.is_condensed);
		free(D.n_est);
		free(D.b_scale);
		return -11;
	}
	if(!eqm_compute_mu0(names, D.ns, source, D.T, D.P0, D.mu0)
			|| !eqm_compute_is_condensed(names, D.ns, source, D.is_condensed)){
		free(D.mu0);
		free(D.is_condensed);
		free(D.n_est);
		free(D.b_scale);
		return -11;
	}
	for(int j = 0; j < D.ns; ++j){
		double v = fabs(D.mu0[j]);
		if(v > max_mu0){
			max_mu0 = v;
		}
	}
	if(max_mu0 > 0.0){
		double denom = max_mu0 / (gas_R() * T);
		if(denom > 1.0){
			D.obj_scale = 1.0 / denom;
		}
	}
	for(i = 0; i < D.ns; ++i){
		D.n_est[i] = 1.0;
	}
	if(n_init){
		int ok_init = 1;
		for(i = 0; i < D.ns; ++i){
			if(n_init[i] <= 0.0){
				ok_init = 0;
				break;
			}
		}
		if(ok_init){
			for(i = 0; i < D.ns; ++i){
				D.n_est[i] = n_init[i];
			}
		}
	}
	if(!n_init){
		double *n0 = (double *)calloc((size_t)D.ns, sizeof(double));
		int ok_n0 = 1;
		if(n0 && eqm_solve_particular(D.A, D.b, D.ne, D.ns, n0)){
			for(i = 0; i < D.ns; ++i){
				if(n0[i] <= 0.0){
					ok_n0 = 0;
					break;
				}
			}
			if(ok_n0){
				for(i = 0; i < D.ns; ++i){
					D.n_est[i] = n0[i];
				}
			}
		}
		free(n0);
	}
	if(!n_init){
		for(i = 0; i < D.ns; ++i){
			if(D.n_est[i] <= 0.0 || !isfinite(D.n_est[i])){
				double bound = HUGE_VAL;
				for(int e = 0; e < D.ne; ++e){
					double aei = D.A[e * D.ns + i];
					if(aei > 0.0){
						double val = D.b[e] / aei;
						if(val < bound){
							bound = val;
						}
					}
				}
				if(!isfinite(bound) || bound <= 0.0){
					bound = 1.0;
				}
				D.n_est[i] = bound;
			}
		}
	}
	for(i = 0; i < D.ns; ++i){
		if(D.n_est[i] < 1e-30){
			D.n_est[i] = 1e-30;
		}
		if(D.n_est[i] > 1e30){
			D.n_est[i] = 1e30;
		}
	}

	n = D.ns;
	m = D.ne;
	nele_jac = (Index)(D.ne * D.ns);
	nele_hess = (Index)(D.ns * (D.ns + 1) / 2);
	x_L = (Number *)calloc(ns_count, sizeof(Number));
	x_U = (Number *)calloc(ns_count, sizeof(Number));
	g_L = (Number *)calloc(ne_count, sizeof(Number));
	g_U = (Number *)calloc(ne_count, sizeof(Number));
	x = (Number *)calloc(ns_count, sizeof(Number));

	for(i = 0; i < D.ns; ++i){
		x_L[i] = -700.0;
		x_U[i] = 100.0;
		x[i] = 0.0;
	}
	if(n_init){
		int ok_init = 1;
		for(i = 0; i < D.ns; ++i){
			if(n_init[i] <= 0.0 || D.n_est[i] <= 0.0){
				ok_init = 0;
				break;
			}
		}
		if(ok_init){
			for(i = 0; i < D.ns; ++i){
				x[i] = log(n_init[i] / D.n_est[i]);
			}
		}
	}else{
		double *n0 = (double *)calloc((size_t)D.ns, sizeof(double));
		int ok_n0 = 1;
		if(n0 && eqm_solve_particular(D.A, D.b, D.ne, D.ns, n0)){
			for(i = 0; i < D.ns; ++i){
				if(n0[i] <= 0.0 || D.n_est[i] <= 0.0){
					ok_n0 = 0;
					break;
				}
			}
			if(ok_n0){
				for(i = 0; i < D.ns; ++i){
					x[i] = log(n0[i] / D.n_est[i]);
				}
			}
		}
		free(n0);
	}
	for(i = 0; i < D.ne; ++i){
		g_L[i] = 0.0;
		g_U[i] = 0.0;
	}

	prob = CreateIpoptProblem(
		n, x_L, x_U,
		m, g_L, g_U,
		nele_jac, nele_hess, 0,
		eval_f_logn, eval_g_logn, eval_grad_f_logn, eval_jac_g_logn, eval_h_logn
	);
	eqm_ipopt_apply_options(prob, D.T, "limited-memory", 1, 1);

	status = IpoptSolve(prob, x, NULL, &obj, NULL, NULL, NULL, (UserDataPtr)&D);
	for(i = 0; i < D.ns; ++i){
		n_out[i] = D.n_est[i] * exp(x[i]);
	}

	FreeIpoptProblem(prob);
	free(D.mu0);
	free(D.is_condensed);
	eqm_free_solution_phases(&D.solution_phase_id, &D.solution_member_index, &D.binary_phases);
	free(D.n_est);
	free(D.b_scale);
	free(x_L);
	free(x_U);
	free(g_L);
	free(g_U);
	free(x);
	return status;
}

int eqm_ipopt_solve_n_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out){
	EqmN D;
	const size_t ns_count = (size_t)ns;
	const size_t ne_count = (size_t)ne;
	double max_mu0 = 0.0;
	Index n;
	Index m;
	Index nele_jac;
	Index nele_hess;
	Number *x_L;
	Number *x_U;
	Number *g_L;
	Number *g_U;
	Number *x;
	IpoptProblem prob;
	Number obj;
	int status;
	int i;

	if(ns <= 0 || ne <= 0 || !names || !A || !b || !n_out){
		return -11;
	}

	D.ns = ns;
	D.ne = ne;
	D.T = T;
	D.P = P;
	D.P0 = 1e5;
	D.obj_scale = 1.0;
	D.n_min = 1e-200;
	D.A = A;
	D.b = b;
	D.solution_phase_id = NULL;
	D.solution_member_index = NULL;
	D.nbinary_phases = 0;
	D.binary_phases = NULL;
	D.mu0 = (double *)calloc((size_t)D.ns, sizeof(double));
	D.is_condensed = (int *)calloc((size_t)D.ns, sizeof(int));
	eqm_apply_bscale_n(&D);
	if(!D.mu0 || !D.is_condensed
			|| !eqm_compute_mu0(names, D.ns, source, D.T, D.P0, D.mu0)
			|| !eqm_compute_is_condensed(names, D.ns, source, D.is_condensed)){
		free(D.mu0);
		free(D.is_condensed);
		free(D.b_scale);
		return -11;
	}
	if(!eqm_compute_solution_phases(names, D.ns, source, &D.solution_phase_id,
			&D.solution_member_index, &D.binary_phases, &D.nbinary_phases)){
		free(D.mu0);
		free(D.is_condensed);
		free(D.b_scale);
		return -11;
	}
	for(int j = 0; j < D.ns; ++j){
		double v = fabs(D.mu0[j]);
		if(v > max_mu0){
			max_mu0 = v;
		}
	}
	if(max_mu0 > 0.0){
		double denom = max_mu0 / (gas_R() * T);
		if(denom > 1.0){
			D.obj_scale = 1.0 / denom;
		}
	}

	n = D.ns;
	m = D.ne;
	nele_jac = (Index)(D.ne * D.ns);
	nele_hess = (Index)(D.ns * (D.ns + 1) / 2);
	x_L = (Number *)calloc(ns_count, sizeof(Number));
	x_U = (Number *)calloc(ns_count, sizeof(Number));
	g_L = (Number *)calloc(ne_count, sizeof(Number));
	g_U = (Number *)calloc(ne_count, sizeof(Number));
	x = (Number *)calloc(ns_count, sizeof(Number));

	for(i = 0; i < D.ns; ++i){
		x_L[i] = D.n_min;
		x_U[i] = 1e20;
		x[i] = 1.0;
	}
	if(n_init){
		for(i = 0; i < D.ns; ++i){
			if(n_init[i] > D.n_min){
				x[i] = n_init[i];
			}
		}
	}else{
		double *n0 = (double *)calloc((size_t)D.ns, sizeof(double));
		int ok_n0 = 1;
		if(n0 && eqm_solve_particular(D.A, D.b, D.ne, D.ns, n0)){
			for(i = 0; i < D.ns; ++i){
				if(n0[i] <= D.n_min){
					ok_n0 = 0;
					break;
				}
			}
			if(ok_n0){
				for(i = 0; i < D.ns; ++i){
					x[i] = n0[i];
				}
			}
		}
		free(n0);
	}
	for(i = 0; i < D.ne; ++i){
		g_L[i] = 0.0;
		g_U[i] = 0.0;
	}

	prob = CreateIpoptProblem(
		n, x_L, x_U,
		m, g_L, g_U,
		nele_jac, nele_hess, 0,
		eval_f_n, eval_g_n, eval_grad_f_n, eval_jac_g_n, eval_h_n
	);
	eqm_ipopt_apply_options(prob, D.T, "limited-memory", 0, 0);

	status = IpoptSolve(prob, x, NULL, &obj, NULL, NULL, NULL, (UserDataPtr)&D);
	for(i = 0; i < D.ns; ++i){
		n_out[i] = x[i];
	}

	FreeIpoptProblem(prob);
	free(D.mu0);
	free(D.is_condensed);
	eqm_free_solution_phases(&D.solution_phase_id, &D.solution_member_index, &D.binary_phases);
	free(D.b_scale);
	free(x_L);
	free(x_U);
	free(g_L);
	free(g_U);
	free(x);
	return status;
}

int eqm_ipopt_solve_n_source(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, double *n_out){
	return eqm_ipopt_solve_n_source_init(names, ns, ne, A, b, source, T, P, NULL, n_out);
}

int eqm_ipopt_solve_n(const char **names, int ns, int ne, const double *A, const double *b,
		double T, double P, double *n_out){
	return eqm_ipopt_solve_n_source(names, ns, ne, A, b, NULL, T, P, n_out);
}

int eqm_ipopt_solve_elements_n_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init,
		double *n_out){
	double *A = NULL;
	int status;

	if(!names || !elements || !b || !n_out || ns <= 0 || ne <= 0){
		return -11;
	}

	A = (double *)calloc((size_t)(ne * ns), sizeof(double));
	if(!A){
		return -11;
	}
	if(!fprops_build_element_matrix_source(names, ns, elements, ne, source, A)){
		free(A);
		return -11;
	}
	status = eqm_ipopt_solve_n_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	free(A);
	return status;
}

int eqm_ipopt_solve_elements_n_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out){
	return eqm_ipopt_solve_elements_n_source_init(names, ns, elements, ne, source, b, T, P, NULL,
		n_out);
}

int eqm_ipopt_solve_elements_n(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out){
	return eqm_ipopt_solve_elements_n_source(names, ns, elements, ne, NULL, b, T, P, n_out);
}

int eqm_ipopt_solve_logn_source(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, double *n_out){
	return eqm_ipopt_solve_logn_source_init(names, ns, ne, A, b, source, T, P, NULL, n_out);
}

int eqm_ipopt_solve_logn(const char **names, int ns, int ne, const double *A, const double *b,
		double T, double P, double *n_out){
	return eqm_ipopt_solve_logn_source(names, ns, ne, A, b, NULL, T, P, n_out);
}

int eqm_ipopt_solve_elements_logn_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init,
		double *n_out){
	double *A = NULL;
	int status;

	if(!names || !elements || !b || !n_out || ns <= 0 || ne <= 0){
		return -11;
	}

	A = (double *)calloc((size_t)(ne * ns), sizeof(double));
	if(!A){
		return -11;
	}
	if(!fprops_build_element_matrix_source(names, ns, elements, ne, source, A)){
		free(A);
		return -11;
	}
	status = eqm_ipopt_solve_logn_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	free(A);
	return status;
}

int eqm_ipopt_solve_elements_logn_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out){
	return eqm_ipopt_solve_elements_logn_source_init(names, ns, elements, ne, source, b, T, P, NULL,
		n_out);
}

int eqm_ipopt_solve_elements_logn(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out){
	return eqm_ipopt_solve_elements_logn_source(names, ns, elements, ne, NULL, b, T, P, n_out);
}

int eqm_ipopt_solve_source(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, double *n_out){
	return eqm_ipopt_solve_source_init(names, ns, ne, A, b, source, T, P, NULL, n_out);
}

int eqm_ipopt_solve(const char **names, int ns, int ne, const double *A, const double *b,
		double T, double P, double *n_out){
	return eqm_ipopt_solve_source(names, ns, ne, A, b, NULL, T, P, n_out);
}

int eqm_ipopt_solve_elements_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init, double *n_out){
	double *A = NULL;
	int status;

	if(!names || !elements || !b || !n_out || ns <= 0 || ne <= 0){
		return -11;
	}

	A = (double *)calloc((size_t)(ne * ns), sizeof(double));
	if(!A){
		return -102;
	}
	if(!fprops_build_element_matrix_source(names, ns, elements, ne, source, A)){
		free(A);
		return -11;
	}

	status = eqm_ipopt_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	free(A);
	return status;
}

int eqm_ipopt_solve_elements_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out){
	return eqm_ipopt_solve_elements_source_init(names, ns, elements, ne, source, b, T, P, NULL, n_out);
}

int eqm_ipopt_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out){
	return eqm_ipopt_solve_elements_source(names, ns, elements, ne, NULL, b, T, P, n_out);
}
