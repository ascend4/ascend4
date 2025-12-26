#include <math.h>
#include <stdlib.h>
#include <float.h>

#include <nlopt.h>

#include "fprops.h"
#include "fluids.h"
#include "eqm_internal.h"
#include "eqm_slsqp.h"

typedef struct EqmSlsqp{
	EqmN D;
	double *n_est;
} EqmSlsqp;

typedef struct EqmNConstraint{
	EqmSlsqp *S;
	int e;
} EqmNConstraint;

static double eqm_mu_ideal(const EqmN *D, const double *logn, double logn_tot, int i){
	double logy = logn[i] - logn_tot;
	return D->mu0[i] + gas_R() * D->T * (logy + log(D->P / D->P0));
}

static double eqm_gibbs_nlopt(unsigned n, const double *x, double *grad, void *data){
	EqmSlsqp *S = (EqmSlsqp *)data;
	EqmN *D = &S->D;
	double *logn = (double *)calloc((size_t)D->ns, sizeof(double));
	double logn_tot;
	double G = 0.0;

	if(!logn){
		return HUGE_VAL;
	}
	for(unsigned i = 0; i < n; ++i){
		double n_i = S->n_est[i] * x[i];
		if(n_i <= 0.0){
			free(logn);
			return HUGE_VAL;
		}
		logn[i] = log(n_i);
	}
	logn_tot = eqm_logsumexp(logn, (int)n);
	if(!isfinite(logn_tot)){
		free(logn);
		return HUGE_VAL;
	}
	for(unsigned i = 0; i < n; ++i){
		double mu = eqm_mu_ideal(D, logn, logn_tot, (int)i);
		double n_i = S->n_est[i] * x[i];
		G += n_i * mu;
		if(grad){
			grad[i] = D->obj_scale * (mu / (gas_R() * D->T)) * S->n_est[i];
		}
	}
	free(logn);
	return D->obj_scale * (G / (gas_R() * D->T));
}

static double eqm_constr_nlopt(unsigned n, const double *x, double *grad, void *data){
	EqmNConstraint *C = (EqmNConstraint *)data;
	EqmSlsqp *S = C->S;
	EqmN *D = &S->D;
	int e = C->e;
	double sum = 0.0;
	for(unsigned i = 0; i < n; ++i){
		sum += D->A[e * D->ns + (int)i] * S->n_est[i] * x[i];
		if(grad){
			grad[i] = D->A[e * D->ns + (int)i] * S->n_est[i] * D->b_scale[e];
		}
	}
	return (sum - D->b[e]) * D->b_scale[e];
}

int eqm_slsqp_solve_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out){
	EqmSlsqp S;
	EqmN *D = &S.D;
	double max_mu0 = 0.0;
	nlopt_opt opt = NULL;
	EqmNConstraint *cons = NULL;
	double *x = NULL;
	double *lb = NULL;
	double *ub = NULL;
	double minf = 0.0;
	int status = -1;

	if(ns <= 0 || ne <= 0 || !names || !A || !b || !n_out){
		return -11;
	}

	D->ns = ns;
	D->ne = ne;
	D->T = T;
	D->P = P;
	D->P0 = 1e5;
	D->obj_scale = 1.0;
	D->n_min = 1e-200;
	D->A = A;
	D->b = b;
	D->mu0 = (double *)calloc((size_t)D->ns, sizeof(double));
	eqm_apply_bscale_n(D);
	S.n_est = (double *)calloc((size_t)D->ns, sizeof(double));
	if(!D->mu0 || !D->b_scale || !S.n_est){
		free(D->mu0);
		free(D->b_scale);
		free(S.n_est);
		return -11;
	}
	if(!eqm_compute_mu0(names, D->ns, source, D->T, D->P0, D->mu0)){
		free(D->mu0);
		free(D->b_scale);
		free(S.n_est);
		return -11;
	}
	for(int j = 0; j < D->ns; ++j){
		double v = fabs(D->mu0[j]);
		if(v > max_mu0){
			max_mu0 = v;
		}
	}
	if(max_mu0 > 0.0){
		double denom = max_mu0 / (gas_R() * T);
		if(denom > 1.0){
			D->obj_scale = 1.0 / denom;
		}
	}
	eqm_fill_n_est(D->A, D->b, D->ne, D->ns, n_init, S.n_est);

	opt = nlopt_create(NLOPT_LD_SLSQP, (unsigned)D->ns);
	if(!opt){
		free(D->mu0);
		free(D->b_scale);
		free(S.n_est);
		return -12;
	}
	x = (double *)calloc((size_t)D->ns, sizeof(double));
	lb = (double *)calloc((size_t)D->ns, sizeof(double));
	ub = (double *)calloc((size_t)D->ns, sizeof(double));
	cons = (EqmNConstraint *)calloc((size_t)D->ne, sizeof(*cons));
	if(!x || !lb || !ub || !cons){
		free(x);
		free(lb);
		free(ub);
		free(cons);
		nlopt_destroy(opt);
		free(D->mu0);
		free(D->b_scale);
		free(S.n_est);
		return -12;
	}
	for(int i = 0; i < D->ns; ++i){
		lb[i] = D->n_min / S.n_est[i];
		ub[i] = 1e20 / S.n_est[i];
		x[i] = 1.0;
	}
	if(n_init){
		for(int i = 0; i < D->ns; ++i){
			if(n_init[i] > D->n_min){
				x[i] = n_init[i] / S.n_est[i];
			}
		}
	}else{
		double *n0 = (double *)calloc((size_t)D->ns, sizeof(double));
		int ok_n0 = 1;
		if(n0 && eqm_solve_particular(D->A, D->b, D->ne, D->ns, n0)){
			for(int i = 0; i < D->ns; ++i){
				if(n0[i] <= D->n_min){
					ok_n0 = 0;
					break;
				}
			}
			if(ok_n0){
				for(int i = 0; i < D->ns; ++i){
					x[i] = n0[i] / S.n_est[i];
				}
			}
		}
		free(n0);
	}
	nlopt_set_lower_bounds(opt, lb);
	nlopt_set_upper_bounds(opt, ub);
	nlopt_set_min_objective(opt, eqm_gibbs_nlopt, &S);
	for(int e = 0; e < D->ne; ++e){
		cons[e].S = &S;
		cons[e].e = e;
		nlopt_add_equality_constraint(opt, eqm_constr_nlopt, &cons[e], 1e-10);
	}
	nlopt_set_ftol_rel(opt, 1e-13);
	nlopt_set_xtol_rel(opt, 1e-12);
	nlopt_set_maxeval(opt, 5000);

	{
		nlopt_result res = nlopt_optimize(opt, x, &minf);
		if(res > 0){
			for(int i = 0; i < D->ns; ++i){
				n_out[i] = x[i] * S.n_est[i];
			}
			status = 0;
		}else{
			status = -1;
		}
	}

	free(x);
	free(lb);
	free(ub);
	free(cons);
	nlopt_destroy(opt);
	free(D->mu0);
	free(D->b_scale);
	free(S.n_est);
	return status;
}

int eqm_slsqp_solve_source(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, double *n_out){
	return eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, NULL, n_out);
}

int eqm_slsqp_solve(const char **names, int ns, int ne, const double *A,
		const double *b, double T, double P, double *n_out){
	return eqm_slsqp_solve_source(names, ns, ne, A, b, NULL, T, P, n_out);
}

int eqm_slsqp_solve_elements_source_init(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, const double *n_init,
		double *n_out){
	double *A = NULL;
	int status;
	double *n_seed = NULL;
	const double *n_use = n_init;

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
	if(!n_use){
		n_seed = (double *)calloc((size_t)ns, sizeof(double));
		if(n_seed && eqm_seed_from_nullspace_r1(names, ns, elements, ne, source, b, T, P, n_seed)){
			n_use = n_seed;
		}
	}
	status = eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, n_use, n_out);
	free(n_seed);
	free(A);
	return status;
}

int eqm_slsqp_solve_elements_source(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_out){
	return eqm_slsqp_solve_elements_source_init(names, ns, elements, ne, source, b, T, P, NULL,
		n_out);
}

int eqm_slsqp_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, double T, double P, double *n_out){
	return eqm_slsqp_solve_elements_source(names, ns, elements, ne, NULL, b, T, P, n_out);
}
