#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

#include "fprops.h"
#include "ideal.h"
#include "fluids.h"
#include "eqm.h"
#include "eqm_internal.h"

#ifdef HAVE_IPOPT
#include "eqm_ipopt.h"
#endif
#ifdef HAVE_NLOPT
#include "eqm_slsqp.h"
#endif

double gas_R(void){
	return 8.31446261815324;
}

void eqm_apply_bscale(EqmData *D){
	int e;
	D->b_scale = (double *)calloc((size_t)D->ne, sizeof(double));
	for(e = 0; e < D->ne; ++e){
		double denom = fabs(D->b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		D->b_scale[e] = 1.0 / denom;
	}
}

void eqm_apply_bscale_logn(EqmLogN *D){
	int e;
	D->b_scale = (double *)calloc((size_t)D->ne, sizeof(double));
	for(e = 0; e < D->ne; ++e){
		double denom = fabs(D->b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		D->b_scale[e] = 1.0 / denom;
	}
}

void eqm_apply_bscale_n(EqmN *D){
	int e;
	D->b_scale = (double *)calloc((size_t)D->ne, sizeof(double));
	for(e = 0; e < D->ne; ++e){
		double denom = fabs(D->b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		D->b_scale[e] = 1.0 / denom;
	}
}

void eqm_apply_nscale(EqmData *D, const double *n_init){
	int i;
	D->n_scale = (double *)calloc((size_t)D->ns, sizeof(double));
	for(i = 0; i < D->ns; ++i){
		double denom = 1.0;
		if(n_init && n_init[i] > 0.0){
			denom = n_init[i];
		}
		if(denom < 1e-30){
			denom = 1e-30;
		}
		D->n_scale[i] = 1.0 / denom;
	}
}

int eqm_compute_mu0(const char **names, int ns, const char *source, double T, double P0, double *mu0){
	int i;
	ReferenceState ref0 = {FPROPS_REF_REF0};
	for(i = 0; i < ns; ++i){
		FpropsError err = FPROPS_NO_ERROR;
		PureFluid *Pideal = NULL;
		const EosData *E = fprops_eos(names[i], NULL, source);
		if(E){
			Pideal = ideal_prepare(E, &ref0);
		}
		if(!Pideal){
			Pideal = (PureFluid *)fprops_fluid(names[i], "ideal", NULL);
		}
		if(Pideal){
			double rho = P0 / (Pideal->data->R * T);
			double molar_mass = Pideal->data->M * 1e-3;
			double g = Pideal->g_fn((FluidStateUnion){.Trho={T, rho}}, Pideal->data, &err);
			fprops_fluid_destroy(Pideal);
			if(err){
				fprintf(stderr, "eqm mu0 failed: g eval error for '%s'\n", names[i]);
				return 0;
			}
			mu0[i] = g * molar_mass;
		}else{
			fprintf(stderr, "eqm mu0 failed: no ideal data for '%s'\n", names[i]);
			return 0;
		}
	}
	return 1;
}

int eqm_mu0_ideal_source(const char *name, const char *source, double T, double P0,
		double *mu0){
	FpropsError err = FPROPS_NO_ERROR;
	ReferenceState ref0 = {FPROPS_REF_REF0};
	const EosData *E;
	PureFluid *Pideal;
	double rho;
	double g;
	double molar_mass;

	if(!name || !mu0){
		return 0;
	}
	E = fprops_eos(name, NULL, source);
	if(!E){
		return 0;
	}
	Pideal = ideal_prepare(E, &ref0);
	if(!Pideal){
		return 0;
	}
	rho = P0 / (Pideal->data->R * T);
	g = Pideal->g_fn((FluidStateUnion){.Trho={T, rho}}, Pideal->data, &err);
	molar_mass = Pideal->data->M * 1e-3;
	fprops_fluid_destroy(Pideal);
	if(err){
		return 0;
	}
	*mu0 = g * molar_mass;
	return 1;
}

int eqm_rref(double *A, int m, int n, int *pivots, int *rank){
	const double tol = 1e-12;
	int row = 0;
	int col;
	*rank = 0;
	for(col = 0; col < n && row < m; ++col){
		int piv = -1;
		double maxabs = 0.0;
		int r;
		for(r = row; r < m; ++r){
			double val = fabs(A[r * n + col]);
			if(val > maxabs){
				maxabs = val;
				piv = r;
			}
		}
		if(piv < 0 || maxabs < tol){
			continue;
		}
		if(piv != row){
			for(int c = col; c < n; ++c){
				double tmp = A[row * n + c];
				A[row * n + c] = A[piv * n + c];
				A[piv * n + c] = tmp;
			}
		}
		{
			double diag = A[row * n + col];
			for(int c = col; c < n; ++c){
				A[row * n + c] /= diag;
			}
		}
		for(r = 0; r < m; ++r){
			if(r == row){
				continue;
			}
			double factor = A[r * n + col];
			if(fabs(factor) < tol){
				continue;
			}
			for(int c = col; c < n; ++c){
				A[r * n + c] -= factor * A[row * n + c];
			}
		}
		pivots[*rank] = col;
		(*rank)++;
		row++;
	}
	return *rank;
}

static int eqm_is_pivot(int col, const int *pivots, int rank){
	for(int i = 0; i < rank; ++i){
		if(pivots[i] == col){
			return 1;
		}
	}
	return 0;
}

void eqm_fill_nullspace(const double *A_rref, int m, int n,
		const int *pivots, int rank, double *N_out, int r){
	int fidx = 0;
	(void)m;
	for(int col = 0; col < n; ++col){
		if(eqm_is_pivot(col, pivots, rank)){
			continue;
		}
		for(int i = 0; i < n; ++i){
			N_out[i * r + fidx] = 0.0;
		}
		N_out[col * r + fidx] = 1.0;
		for(int prow = 0; prow < rank; ++prow){
			int pcol = pivots[prow];
			N_out[pcol * r + fidx] = -A_rref[prow * n + col];
		}
		++fidx;
	}
}

int eqm_solve_particular(const double *A_in, const double *b_in, int m, int n, double *n0_out){
	const double tol = 1e-12;
	int ncols = n + 1;
	double *A = (double *)calloc((size_t)(m * ncols), sizeof(double));
	int *pivots = (int *)calloc((size_t)m, sizeof(int));
	int rank = 0;

	if(!A || !pivots){
		free(A);
		free(pivots);
		return 0;
	}
	for(int i = 0; i < m; ++i){
		for(int j = 0; j < n; ++j){
			A[i * ncols + j] = A_in[i * n + j];
		}
		A[i * ncols + n] = b_in[i];
	}
	eqm_rref(A, m, ncols, pivots, &rank);
	for(int i = 0; i < m; ++i){
		double row_norm = 0.0;
		for(int j = 0; j < n; ++j){
			row_norm += fabs(A[i * ncols + j]);
		}
		if(row_norm < tol && fabs(A[i * ncols + n]) > tol){
			free(A);
			free(pivots);
			return 0;
		}
	}
	for(int i = 0; i < n; ++i){
		n0_out[i] = 0.0;
	}
	for(int prow = 0; prow < rank; ++prow){
		int pcol = pivots[prow];
		n0_out[pcol] = A[prow * ncols + n];
	}
	free(A);
	free(pivots);
	return 1;
}

void eqm_fill_n_est(const double *A, const double *b, int ne, int ns,
		const double *n_init, double *n_est){
	for(int i = 0; i < ns; ++i){
		n_est[i] = 1.0;
	}
	if(n_init){
		int ok_init = 1;
		for(int i = 0; i < ns; ++i){
			if(n_init[i] <= 0.0){
				ok_init = 0;
				break;
			}
		}
		if(ok_init){
			for(int i = 0; i < ns; ++i){
				n_est[i] = n_init[i];
			}
			return;
		}
	}
	{
		double *n0 = (double *)calloc((size_t)ns, sizeof(double));
		int ok_n0 = 1;
		if(n0 && eqm_solve_particular(A, b, ne, ns, n0)){
			for(int i = 0; i < ns; ++i){
				if(n0[i] <= 0.0){
					ok_n0 = 0;
					break;
				}
			}
			if(ok_n0){
				for(int i = 0; i < ns; ++i){
					n_est[i] = n0[i];
				}
				free(n0);
				goto clamp;
			}
		}
		free(n0);
	}
	for(int i = 0; i < ns; ++i){
		double bound = HUGE_VAL;
		for(int e = 0; e < ne; ++e){
			double aei = A[e * ns + i];
			if(aei > 0.0){
				double val = b[e] / aei;
				if(val < bound){
					bound = val;
				}
			}
		}
		if(!isfinite(bound) || bound <= 0.0){
			bound = 1.0;
		}
		n_est[i] = bound;
	}
clamp:
	for(int i = 0; i < ns; ++i){
		if(n_est[i] < 1e-30){
			n_est[i] = 1e-30;
		}
		if(n_est[i] > 1e30){
			n_est[i] = 1e30;
		}
	}
}

double eqm_logsumexp(const double *logv, int n){
	double maxv = -HUGE_VAL;
	double sum = 0.0;
	for(int i = 0; i < n; ++i){
		if(logv[i] > maxv){
			maxv = logv[i];
		}
	}
	if(!isfinite(maxv)){
		return -HUGE_VAL;
	}
	for(int i = 0; i < n; ++i){
		sum += exp(logv[i] - maxv);
	}
	if(sum <= 0.0){
		return -HUGE_VAL;
	}
	return maxv + log(sum);
}

#ifndef HAVE_IPOPT
int eqm_seed_from_nullspace_r1(const char **names, int ns, const char **elements, int ne,
		const char *source, const double *b, double T, double P, double *n_seed){
	(void)names;
	(void)ns;
	(void)elements;
	(void)ne;
	(void)source;
	(void)b;
	(void)T;
	(void)P;
	(void)n_seed;
	return 0;
}
#endif

static int eqm_alg_auto(const char *algorithm){
	return !algorithm || !algorithm[0] || strcmp(algorithm, "auto") == 0;
}

static int eqm_alg_prefix(const char *algorithm, const char *prefix){
	size_t len = strlen(prefix);
	return algorithm && strncmp(algorithm, prefix, len) == 0;
}

int eqm_solve(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out){
	int status = -99;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0){
		return -11;
	}
	if(eqm_alg_auto(algorithm)){
#ifdef HAVE_IPOPT
		status = eqm_ipopt_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(status == 0){
			return status;
		}
#endif
#ifdef HAVE_NLOPT
		status = eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(status == 0){
			return status;
		}
#endif
		return status;
	}
#ifdef HAVE_IPOPT
	if(eqm_alg_prefix(algorithm, "ipopt")){
		if(strstr(algorithm, "logn")){
			return eqm_ipopt_solve_logn_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		}
		if(strstr(algorithm, "_n")){
			return eqm_ipopt_solve_n_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		}
		return eqm_ipopt_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	}
#endif
#ifdef HAVE_NLOPT
	if(eqm_alg_prefix(algorithm, "slsqp")){
		return eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	}
#endif
	return -12;
}

int eqm_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const char *algorithm,
		const double *n_init, double *n_out){
	double *A = NULL;
	int status = -11;

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
	status = eqm_solve(names, ns, ne, A, b, source, T, P, algorithm, n_init, n_out);
	free(A);
	return status;
}
