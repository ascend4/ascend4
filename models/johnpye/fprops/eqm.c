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

/* Bound-KKT acceptance settings for boundary-active equilibrium solutions. */
static const double EQM_BOUND_KKT_FREE_TOL = 2e-2;
static const double EQM_BOUND_KKT_DUAL_TOL = 2e-2;
static const double EQM_BOUND_ACTIVE_CUTOFF_FRAC = 1e-22;

static int eqm_active_trace_enabled(void){
	static int enabled = -1;
	if(enabled < 0){
		const char *v = getenv("FPROPS_EQM_ACTIVESET_TRACE");
		enabled = (v && v[0] && strcmp(v, "0") != 0) ? 1 : 0;
	}
	return enabled;
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
	for(i = 0; i < ns; ++i){
		if(!eqm_mu0_ideal_source(names[i], source, T, P0, &mu0[i])){
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
	PureFluid *Pideal;
	double rho;
	double g;
	double molar_mass;
	const EosData *cands[3];
	int ncands = 0;
	int c;

	if(!name || !mu0){
		return 0;
	}
	cands[ncands++] = fprops_eos(name, NULL, source);
	cands[ncands++] = fprops_eos(name, NULL, NULL);
	cands[ncands++] = fprops_eos(name, NULL, "RPP");
	for(c = 0; c < ncands; ++c){
		const EosData *E = cands[c];
		if(!E){
			continue;
		}
		Pideal = ideal_prepare(E, &ref0);
		if(!Pideal){
			continue;
		}
		err = FPROPS_NO_ERROR;
		rho = P0 / (Pideal->data->R * T);
		g = Pideal->g_fn((FluidStateUnion){.Trho={T, rho}}, Pideal->data, &err);
		molar_mass = Pideal->data->M * 1e-3;
		fprops_fluid_destroy(Pideal);
		if(!err){
			*mu0 = g * molar_mass;
			return 1;
		}
	}
	Pideal = (PureFluid *)fprops_fluid(name, "ideal", NULL);
	if(Pideal){
		rho = P0 / (Pideal->data->R * T);
		err = FPROPS_NO_ERROR;
		g = Pideal->g_fn((FluidStateUnion){.Trho={T, rho}}, Pideal->data, &err);
		molar_mass = Pideal->data->M * 1e-3;
		fprops_fluid_destroy(Pideal);
		if(!err){
			*mu0 = g * molar_mass;
			return 1;
		}
	}
	Pideal = (PureFluid *)fprops_fluid(name, "ideal", "RPP");
	if(!Pideal){
		return 0;
	}
	err = FPROPS_NO_ERROR;
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

static int eqm_dense_solve(double *A, double *b, int n){
	const double piv_tol = 1e-14;
	for(int k = 0; k < n; ++k){
		int piv = k;
		double maxabs = fabs(A[k * n + k]);
		for(int i = k + 1; i < n; ++i){
			double v = fabs(A[i * n + k]);
			if(v > maxabs){
				maxabs = v;
				piv = i;
			}
		}
		if(!(maxabs > piv_tol)){
			return 0;
		}
		if(piv != k){
			for(int j = k; j < n; ++j){
				double tmp = A[k * n + j];
				A[k * n + j] = A[piv * n + j];
				A[piv * n + j] = tmp;
			}
			{
				double tmp = b[k];
				b[k] = b[piv];
				b[piv] = tmp;
			}
		}
		{
			double diag = A[k * n + k];
			for(int i = k + 1; i < n; ++i){
				double f = A[i * n + k] / diag;
				A[i * n + k] = 0.0;
				for(int j = k + 1; j < n; ++j){
					A[i * n + j] -= f * A[k * n + j];
				}
				b[i] -= f * b[k];
			}
		}
	}
	for(int i = n - 1; i >= 0; --i){
		double s = b[i];
		double diag = A[i * n + i];
		if(!(fabs(diag) > piv_tol)){
			return 0;
		}
		for(int j = i + 1; j < n; ++j){
			s -= A[i * n + j] * b[j];
		}
		b[i] = s / diag;
	}
	return 1;
}

static void eqm_reduced_compute_n(const double *n0, const double *N, int ns, int r,
		const double *z, double *n){
	for(int i = 0; i < ns; ++i){
		double v = n0[i];
		for(int j = 0; j < r; ++j){
			v += N[i * r + j] * z[j];
		}
		n[i] = v;
	}
}

static int eqm_reduced_bounds_r1_interval(const double *n0, const double *N, int ns,
		double n_floor, double *z_low, double *z_high){
	double zl = -HUGE_VAL;
	double zh = HUGE_VAL;
	for(int i = 0; i < ns; ++i){
		double Ni = N[i];
		double rhs = n_floor - n0[i];
		if(Ni > 0.0){
			double v = rhs / Ni;
			if(v > zl){
				zl = v;
			}
		}else if(Ni < 0.0){
			double v = rhs / Ni;
			if(v < zh){
				zh = v;
			}
		}else if(n0[i] <= n_floor){
			return 0;
		}
	}
	if(!(zl < zh)){
		return 0;
	}
	*z_low = zl;
	*z_high = zh;
	return 1;
}

static int eqm_reduced_bounds_r1_interval_ld(const double *n0, const double *N, int ns,
		long double n_floor, long double *z_low, long double *z_high){
	long double zl = -HUGE_VALL;
	long double zh = HUGE_VALL;
	for(int i = 0; i < ns; ++i){
		long double Ni = (long double)N[i];
		long double rhs = n_floor - (long double)n0[i];
		if(Ni > 0.0L){
			long double v = rhs / Ni;
			if(v > zl){
				zl = v;
			}
		}else if(Ni < 0.0L){
			long double v = rhs / Ni;
			if(v < zh){
				zh = v;
			}
		}else if((long double)n0[i] <= n_floor){
			return 0;
		}
	}
	if(!(zl < zh)){
		return 0;
	}
	*z_low = zl;
	*z_high = zh;
	return 1;
}

static int eqm_reduced_bounds_r1(const double *n0, const double *N, int ns, double n_floor,
		double *z_mid){
	double zl = 0.0;
	double zh = 0.0;
	if(!eqm_reduced_bounds_r1_interval(n0, N, ns, n_floor, &zl, &zh)){
		return 0;
	}
	*z_mid = 0.5 * (zl + zh);
	return 1;
}

static int eqm_reduced_project_ls(const double *n0, const double *N, int ns, int r,
		const double *n_target, double *z_out){
	double *G = NULL;
	double *rhs = NULL;
	int ok = 0;
	G = (double *)calloc((size_t)(r * r), sizeof(double));
	rhs = (double *)calloc((size_t)r, sizeof(double));
	if(!G || !rhs){
		free(G);
		free(rhs);
		return 0;
	}
	for(int j = 0; j < r; ++j){
		double s = 0.0;
		for(int i = 0; i < ns; ++i){
			s += N[i * r + j] * (n_target[i] - n0[i]);
		}
		rhs[j] = s;
		for(int k = 0; k < r; ++k){
			double g = 0.0;
			for(int i = 0; i < ns; ++i){
				g += N[i * r + j] * N[i * r + k];
			}
			G[j * r + k] = g;
		}
	}
	ok = eqm_dense_solve(G, rhs, r);
	if(ok){
		for(int j = 0; j < r; ++j){
			z_out[j] = rhs[j];
		}
	}
	free(G);
	free(rhs);
	return ok;
}

static int eqm_reduced_make_interior(const double *n0, const double *N, int ns, int r,
		double n_floor, double *z){
	double *n = NULL;
	double *p = NULL;
	int pass;
	int iter;
	int ok = 0;

	if(r == 1){
		double zmid = 0.0;
		if(eqm_reduced_bounds_r1(n0, N, ns, n_floor, &zmid)){
			z[0] = zmid;
			return 1;
		}
		return 0;
	}

	n = (double *)calloc((size_t)ns, sizeof(double));
	p = (double *)calloc((size_t)r, sizeof(double));
	if(!n || !p){
		free(n);
		free(p);
		return 0;
	}

	for(pass = 0; pass < 3 && !ok; ++pass){
		if(pass == 1){
			for(int j = 0; j < r; ++j){
				z[j] = 0.0;
			}
		}else if(pass == 2){
			for(int j = 0; j < r; ++j){
				z[j] = (j % 2 == 0) ? 1.0 : -1.0;
			}
		}
		for(iter = 0; iter < 600; ++iter){
			int imin = 0;
			double nmin;
			double norm2 = 0.0;
			double alpha_max = HUGE_VAL;
			double alpha;
			eqm_reduced_compute_n(n0, N, ns, r, z, n);
			nmin = n[0];
			for(int i = 1; i < ns; ++i){
				if(n[i] < nmin){
					nmin = n[i];
					imin = i;
				}
			}
			if(nmin > n_floor){
				ok = 1;
				break;
			}
			for(int j = 0; j < r; ++j){
				p[j] = N[imin * r + j];
				norm2 += p[j] * p[j];
			}
			if(!(norm2 > 1e-24)){
				break;
			}
			for(int i = 0; i < ns; ++i){
				double slope = 0.0;
				for(int j = 0; j < r; ++j){
					slope += N[i * r + j] * p[j];
				}
				if(slope < 0.0){
					double a = 0.9 * (n[i] - n_floor) / (-slope);
					if(a < alpha_max){
						alpha_max = a;
					}
				}
			}
			if(!(alpha_max > 0.0) || !isfinite(alpha_max)){
				break;
			}
			alpha = (10.0 * n_floor - nmin) / norm2;
			if(alpha < 1e-8){
				alpha = 1e-8;
			}
			if(alpha > 0.5 * alpha_max){
				alpha = 0.5 * alpha_max;
			}
			if(!(alpha > 1e-16)){
				break;
			}
			for(int j = 0; j < r; ++j){
				z[j] += alpha * p[j];
			}
		}
	}

	free(n);
	free(p);
	return ok;
}

static int eqm_reduced_eval_obj_mu(const double *n, const double *mu0, int ns,
		double T, double P, double P0, double *obj, double *mu){
	const double RT = gas_R() * T;
	const double logPP0 = log(P / P0);
	double n_tot = 0.0;
	double log_n_tot;
	double f = 0.0;
	for(int i = 0; i < ns; ++i){
		if(!(n[i] > 0.0) || !isfinite(n[i])){
			return 0;
		}
		n_tot += n[i];
	}
	if(!(n_tot > 0.0) || !isfinite(n_tot)){
		return 0;
	}
	log_n_tot = log(n_tot);
	for(int i = 0; i < ns; ++i){
		double lnai = log(n[i]) - log_n_tot + logPP0;
		double mui = mu0[i] + RT * lnai;
		if(!isfinite(mui)){
			return 0;
		}
		mu[i] = mui;
		f += n[i] * mui;
	}
	*obj = f;
	return 1;
}

static void eqm_reduced_eval_grad_hess(const double *n, const double *N, int ns, int r,
		double T, const double *mu, double *grad, double *H){
	const double RT = gas_R() * T;
	const double n_curv_floor = 1e-80;
	double n_tot = 0.0;
	double *c = (double *)calloc((size_t)r, sizeof(double));
	for(int i = 0; i < ns; ++i){
		n_tot += n[i];
	}
	for(int j = 0; j < r; ++j){
		double gj = 0.0;
		double cj = 0.0;
		for(int i = 0; i < ns; ++i){
			double Nij = N[i * r + j];
			gj += Nij * mu[i];
			cj += Nij;
		}
		grad[j] = gj;
		c[j] = cj;
	}
	for(int j = 0; j < r; ++j){
		for(int k = 0; k < r; ++k){
			double s = 0.0;
			for(int i = 0; i < ns; ++i){
				double ni = n[i];
				if(ni < n_curv_floor){
					ni = n_curv_floor;
				}
				s += N[i * r + j] * N[i * r + k] / ni;
			}
			H[j * r + k] = RT * (s - (c[j] * c[k] / n_tot));
		}
	}
	free(c);
}

static int eqm_reduced_eval_phi_r1(const double *n0, const double *v, int ns,
		const double *mu0, double T, double P, double P0, long double z,
		long double *phi, long double *obj){
	const long double RT = (long double)gas_R() * (long double)T;
	const long double logPP0 = logl((long double)P / (long double)P0);
	long double ntot = 0.0L;
	long double f = 0.0L;
	long double p = 0.0L;
	for(int i = 0; i < ns; ++i){
		long double ni = (long double)n0[i] + (long double)v[i] * z;
		if(!(ni > 0.0L) || !isfinite((double)ni)){
			return 0;
		}
		ntot += ni;
	}
	if(!(ntot > 0.0L) || !isfinite((double)ntot)){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		long double ni = (long double)n0[i] + (long double)v[i] * z;
		long double mui = (long double)mu0[i] + RT * (logl(ni) - logl(ntot) + logPP0);
		if(!isfinite((double)mui)){
			return 0;
		}
		f += ni * mui;
		p += (long double)v[i] * mui;
	}
	*obj = f;
	*phi = p;
	return 1;
}

static int eqm_reduced_eval_phi_edge(const long double *n_edge, const double *dir, int ns,
		const double *mu0, double T, double P, double P0, long double u,
		long double *phi, long double *obj){
	const long double RT = (long double)gas_R() * (long double)T;
	const long double logPP0 = logl((long double)P / (long double)P0);
	long double ntot = 0.0L;
	long double f = 0.0L;
	long double p = 0.0L;
	for(int i = 0; i < ns; ++i){
		long double ni = n_edge[i] + (long double)dir[i] * u;
		if(!(ni > 0.0L) || !isfinite((double)ni)){
			return 0;
		}
		ntot += ni;
	}
	if(!(ntot > 0.0L) || !isfinite((double)ntot)){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		long double ni = n_edge[i] + (long double)dir[i] * u;
		long double mui = (long double)mu0[i] + RT * (logl(ni) - logl(ntot) + logPP0);
		if(!isfinite((double)mui)){
			return 0;
		}
		f += ni * mui;
		p += (long double)(-dir[i]) * mui; /* dir = +/-v, phi uses v */
	}
	*obj = f;
	*phi = p;
	return 1;
}

static int eqm_reduced_try_edge_root(const long double *n_edge, const double *dir, int ns,
		const double *mu0, double T, double P, double P0, long double u_max,
		long double *u_best){
	const long double u_min = 1e-300L;
	long double ulo = u_min;
	long double uhi = u_min;
	long double flo = 0.0L;
	long double fhi = 0.0L;
	long double tmp = 0.0L;
	int bracketed = 0;

	if(!(u_max > u_min)){
		return 0;
	}
	if(!eqm_reduced_eval_phi_edge(n_edge, dir, ns, mu0, T, P, P0, ulo, &flo, &tmp)){
		return 0;
	}
	for(int it = 0; it < 600; ++it){
		uhi *= 10.0L;
		if(uhi > u_max){
			uhi = u_max;
		}
		if(!eqm_reduced_eval_phi_edge(n_edge, dir, ns, mu0, T, P, P0, uhi, &fhi, &tmp)){
			return 0;
		}
		if((flo == 0.0L) || (fhi == 0.0L) || (flo * fhi < 0.0L)){
			bracketed = 1;
			break;
		}
		ulo = uhi;
		flo = fhi;
		if(uhi >= u_max){
			break;
		}
	}
	if(!bracketed){
		return 0;
	}
	if(flo == 0.0L){
		*u_best = ulo;
		return 1;
	}
	if(fhi == 0.0L){
		*u_best = uhi;
		return 1;
	}
	for(int it = 0; it < 300; ++it){
		long double um = sqrtl(ulo * uhi);
		long double fm = 0.0L;
		if(!eqm_reduced_eval_phi_edge(n_edge, dir, ns, mu0, T, P, P0, um, &fm, &tmp)){
			return 0;
		}
		if(fabsl(fm) / ((long double)gas_R() * (long double)T) < 1e-10L
				|| (uhi / ulo) < (1.0L + 1e-14L)){
			*u_best = um;
			return 1;
		}
		if(flo * fm <= 0.0L){
			uhi = um;
			fhi = fm;
		}else{
			ulo = um;
			flo = fm;
		}
	}
	*u_best = sqrtl(ulo * uhi);
	return 1;
}

static int eqm_reduced_solve_r1(const double *n0, const double *v, int ns, const double *mu0,
		double T, double P, double P0, double n_floor, double *n_out){
	long double zl;
	long double zh;
	long double span;
	long double eps;
	long double za = 0.0L;
	long double zb = 0.0L;
	long double phi_a = 0.0L;
	long double phi_b = 0.0L;
	long double obj_a = 0.0L;
	long double obj_b = 0.0L;
	long double z_best = 0.0L;
	long double u_edge = 0.0L;
	long double *n_low = NULL;
	long double *n_high = NULL;
	double *dir_low = NULL;
	double *dir_high = NULL;
	int used_edge = 0;
	int edge_high = 0;

	if(!eqm_reduced_bounds_r1_interval_ld(n0, v, ns, (long double)n_floor, &zl, &zh)){
		return 0;
	}
	n_low = (long double *)calloc((size_t)ns, sizeof(long double));
	n_high = (long double *)calloc((size_t)ns, sizeof(long double));
	dir_low = (double *)calloc((size_t)ns, sizeof(double));
	dir_high = (double *)calloc((size_t)ns, sizeof(double));
	if(!n_low || !n_high || !dir_low || !dir_high){
		free(n_low);
		free(n_high);
		free(dir_low);
		free(dir_high);
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		n_low[i] = (long double)n0[i] + (long double)v[i] * zl;
		n_high[i] = (long double)n0[i] + (long double)v[i] * zh;
		dir_low[i] = v[i];
		dir_high[i] = -v[i];
	}

	span = zh - zl;
	eps = 1e-18L * (1.0L + fabsl(zl) + fabsl(zh));
	if(eps > 0.49L * span){
		eps = 0.49L * span;
	}
	za = nextafterl(zl, zh);
	zb = nextafterl(zh, zl);
	if(!(za < zb)){
		za = zl + eps;
		zb = zh - eps;
	}
	if(!(za < zb)){
		za = zl;
		zb = zh;
	}
	if(!eqm_reduced_eval_phi_r1(n0, v, ns, mu0, T, P, P0, za, &phi_a, &obj_a)){
		free(n_low);
		free(n_high);
		free(dir_low);
		free(dir_high);
		return 0;
	}
	if(!eqm_reduced_eval_phi_r1(n0, v, ns, mu0, T, P, P0, zb, &phi_b, &obj_b)){
		free(n_low);
		free(n_high);
		free(dir_low);
		free(dir_high);
		return 0;
	}
	if(phi_a == 0.0L){
		z_best = za;
	}else if(phi_b == 0.0L){
		z_best = zb;
	}else if(phi_a * phi_b < 0.0L){
		long double left = za;
		long double right = zb;
		long double fl = phi_a;
		for(int iter = 0; iter < 300; ++iter){
			long double mid = 0.5L * (left + right);
			long double fm = 0.0L;
			long double om = 0.0L;
			if(!eqm_reduced_eval_phi_r1(n0, v, ns, mu0, T, P, P0, mid, &fm, &om)){
				return 0;
			}
			if(fabsl(fm) / ((long double)gas_R() * (long double)T) < 1e-10L
					|| fabsl(right - left) < 1e-21L * (1.0L + fabsl(mid))){
				z_best = mid;
				break;
			}
			if(fl * fm <= 0.0L){
				right = mid;
			}else{
				left = mid;
				fl = fm;
			}
			z_best = 0.5L * (left + right);
		}
	}else{
		long double ubest = 0.0L;
		int found = 0;
		if(eqm_reduced_try_edge_root(n_high, dir_high, ns, mu0, T, P, P0, span, &ubest)){
			z_best = zh - ubest;
			u_edge = ubest;
			used_edge = 1;
			edge_high = 1;
			found = 1;
		}
		if(!found && eqm_reduced_try_edge_root(n_low, dir_low, ns, mu0, T, P, P0, span, &ubest)){
			z_best = zl + ubest;
			u_edge = ubest;
			used_edge = 1;
			edge_high = 0;
			found = 1;
		}
		if(!found){
			z_best = (obj_a <= obj_b) ? za : zb;
		}
	}
	for(int i = 0; i < ns; ++i){
		long double ni;
		if(used_edge){
			if(edge_high){
				ni = n_high[i] + (long double)dir_high[i] * u_edge;
			}else{
				ni = n_low[i] + (long double)dir_low[i] * u_edge;
			}
		}else{
			ni = (long double)n0[i] + (long double)v[i] * z_best;
		}
		n_out[i] = (double)ni;
		if(!(n_out[i] > 0.0) || !isfinite(n_out[i])){
			free(n_low);
			free(n_high);
			free(dir_low);
			free(dir_high);
			return 0;
		}
	}
	free(n_low);
	free(n_high);
	free(dir_low);
	free(dir_high);
	return 1;
}

static int eqm_reduced_solve_source_init_once(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double n_floor, double *n_out){
	const double P0 = 1e5;
	const double grad_tol = 1e-8;
	const int max_iter = 2000;
	double *mu0 = NULL;
	double *Awork = NULL;
	int *pivots = NULL;
	double *N = NULL;
	double *n0 = NULL;
	double *n = NULL;
	double *n_target = NULL;
	double *z = NULL;
	double *mu = NULL;
	double *grad = NULL;
	double *H = NULL;
	double *Hsys = NULL;
	double *rhs = NULL;
	double *dz = NULL;
	double *dn = NULL;
	int rank = 0;
	int r = 0;
	int status = -13;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}

	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	Awork = (double *)calloc((size_t)(ne * ns), sizeof(double));
	pivots = (int *)calloc((size_t)ne, sizeof(int));
	if(!mu0 || !Awork || !pivots){
		status = -11;
		goto cleanup;
	}
	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		status = -11;
		goto cleanup;
	}
	for(int i = 0; i < ne * ns; ++i){
		Awork[i] = A[i];
	}
	eqm_rref(Awork, ne, ns, pivots, &rank);
	r = ns - rank;
	if(r <= 0){
		if(!eqm_solve_particular(A, b, ne, ns, n_out)){
			status = -13;
			goto cleanup;
		}
		for(int i = 0; i < ns; ++i){
			if(!(n_out[i] > 0.0) || !isfinite(n_out[i])){
				status = -13;
				goto cleanup;
			}
		}
		status = 0;
		goto cleanup;
	}

	N = (double *)calloc((size_t)(ns * r), sizeof(double));
	n0 = (double *)calloc((size_t)ns, sizeof(double));
	n = (double *)calloc((size_t)ns, sizeof(double));
	n_target = (double *)calloc((size_t)ns, sizeof(double));
	z = (double *)calloc((size_t)r, sizeof(double));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	grad = (double *)calloc((size_t)r, sizeof(double));
	H = (double *)calloc((size_t)(r * r), sizeof(double));
	Hsys = (double *)calloc((size_t)(r * r), sizeof(double));
	rhs = (double *)calloc((size_t)r, sizeof(double));
	dz = (double *)calloc((size_t)r, sizeof(double));
	dn = (double *)calloc((size_t)ns, sizeof(double));
	if(!N || !n0 || !n || !n_target || !z || !mu || !grad || !H || !Hsys || !rhs || !dz || !dn){
		status = -11;
		goto cleanup;
	}

	eqm_fill_nullspace(Awork, ne, ns, pivots, rank, N, r);
	if(!eqm_solve_particular(A, b, ne, ns, n0)){
		status = -13;
		goto cleanup;
	}
	if(r == 1){
		if(eqm_reduced_solve_r1(n0, N, ns, mu0, T, P, P0, n_floor, n_out)){
			status = 0;
			goto cleanup;
		}
	}
	eqm_fill_n_est(A, b, ne, ns, n_init, n_target);
	if(!eqm_reduced_project_ls(n0, N, ns, r, n_target, z)){
		for(int j = 0; j < r; ++j){
			z[j] = 0.0;
		}
	}
	eqm_reduced_compute_n(n0, N, ns, r, z, n);
	{
		double nmin = n[0];
		for(int i = 1; i < ns; ++i){
			if(n[i] < nmin){
				nmin = n[i];
			}
		}
		if(nmin <= n_floor){
			if(!eqm_reduced_make_interior(n0, N, ns, r, n_floor, z)){
				status = -13;
				goto cleanup;
			}
			eqm_reduced_compute_n(n0, N, ns, r, z, n);
		}
	}

	for(int iter = 0; iter < max_iter; ++iter){
		double obj = 0.0;
		double grad_inf = 0.0;
		double gdotdz;
		double alpha_max = HUGE_VAL;
		double alpha;
		int accepted = 0;

		if(!eqm_reduced_eval_obj_mu(n, mu0, ns, T, P, P0, &obj, mu)){
			status = -13;
			goto cleanup;
		}
		eqm_reduced_eval_grad_hess(n, N, ns, r, T, mu, grad, H);
		for(int j = 0; j < r; ++j){
			double g = fabs(grad[j]) / (gas_R() * T);
			if(g > grad_inf){
				grad_inf = g;
			}
		}
		if(grad_inf < grad_tol){
			for(int i = 0; i < ns; ++i){
				n_out[i] = n[i];
			}
			status = 0;
			goto cleanup;
		}

		{
			double lambda = 0.0;
			int solved = 0;
			for(int damp = 0; damp < 8; ++damp){
				for(int j = 0; j < r; ++j){
					rhs[j] = -grad[j];
					for(int k = 0; k < r; ++k){
						Hsys[j * r + k] = H[j * r + k];
					}
					Hsys[j * r + j] += lambda;
				}
				if(eqm_dense_solve(Hsys, rhs, r)){
					solved = 1;
					for(int j = 0; j < r; ++j){
						dz[j] = rhs[j];
					}
					break;
				}
				lambda = (lambda == 0.0) ? 1e-12 : (lambda * 100.0);
			}
			if(!solved){
				for(int j = 0; j < r; ++j){
					dz[j] = -grad[j];
				}
			}
		}

		gdotdz = 0.0;
		for(int j = 0; j < r; ++j){
			gdotdz += grad[j] * dz[j];
		}
		if(!(gdotdz < 0.0)){
			gdotdz = 0.0;
			for(int j = 0; j < r; ++j){
				dz[j] = -grad[j];
				gdotdz += grad[j] * dz[j];
			}
			if(!(gdotdz < 0.0)){
				status = -13;
				goto cleanup;
			}
		}

		for(int i = 0; i < ns; ++i){
			double s = 0.0;
			for(int j = 0; j < r; ++j){
				s += N[i * r + j] * dz[j];
			}
			dn[i] = s;
			if(s < 0.0){
				double a = 0.995 * (n[i] - n_floor) / (-s);
				if(a < alpha_max){
					alpha_max = a;
				}
			}
		}
		if(!(alpha_max > 0.0) || !isfinite(alpha_max)){
			status = -13;
			goto cleanup;
		}
		alpha = 1.0;
		if(alpha > alpha_max){
			alpha = alpha_max;
		}
		for(int bt = 0; bt < 80; ++bt){
			double obj_trial = 0.0;
			int valid = 1;
			for(int j = 0; j < r; ++j){
				rhs[j] = z[j] + alpha * dz[j];
			}
			eqm_reduced_compute_n(n0, N, ns, r, rhs, dn);
			for(int i = 0; i < ns; ++i){
				if(!(dn[i] > n_floor) || !isfinite(dn[i])){
					valid = 0;
					break;
				}
			}
			if(valid && eqm_reduced_eval_obj_mu(dn, mu0, ns, T, P, P0, &obj_trial, mu)){
				if(obj_trial <= obj + 1e-4 * alpha * gdotdz || obj_trial < obj){
					accepted = 1;
					for(int j = 0; j < r; ++j){
						z[j] = rhs[j];
					}
					for(int i = 0; i < ns; ++i){
						n[i] = dn[i];
					}
					break;
				}
			}
			alpha *= 0.5;
			if(alpha < 1e-24){
				break;
			}
		}
		if(!accepted){
			status = -13;
			goto cleanup;
		}
	}

	status = -13;

cleanup:
	free(dn);
	free(dz);
	free(rhs);
	free(Hsys);
	free(H);
	free(grad);
	free(mu);
	free(z);
	free(n_target);
	free(n);
	free(n0);
	free(N);
	free(pivots);
	free(Awork);
	free(mu0);
	return status;
}

static int eqm_reduced_eval_reduced_gradients(const double *mu, const double *A, int ns, int ne,
		const int *is_active, double T, double *red){
	double *M = NULL;
	double *Msys = NULL;
	double *rhs = NULL;
	double *lambda = NULL;
	int nfree = 0;

	if(!mu || !A || !is_active || !red || ns <= 0 || ne <= 0 || !(T > 0.0)){
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		if(!is_active[i]){
			++nfree;
		}
	}
	if(nfree <= 0){
		return 0;
	}
	M = (double *)calloc((size_t)(ne * ne), sizeof(double));
	Msys = (double *)calloc((size_t)(ne * ne), sizeof(double));
	rhs = (double *)calloc((size_t)ne, sizeof(double));
	lambda = (double *)calloc((size_t)ne, sizeof(double));
	if(!M || !Msys || !rhs || !lambda){
		free(M);
		free(Msys);
		free(rhs);
		free(lambda);
		return 0;
	}
	for(int p = 0; p < ne; ++p){
		double bp = 0.0;
		for(int i = 0; i < ns; ++i){
			if(is_active[i]){
				continue;
			}
			bp += A[p * ns + i] * mu[i];
		}
		rhs[p] = -bp;
		for(int q = 0; q < ne; ++q){
			double s = 0.0;
			for(int i = 0; i < ns; ++i){
				if(is_active[i]){
					continue;
				}
				s += A[p * ns + i] * A[q * ns + i];
			}
			M[p * ne + q] = s;
		}
	}
	{
		double reg = 0.0;
		int solved = 0;
		for(int damp = 0; damp < 8; ++damp){
			for(int p = 0; p < ne; ++p){
				lambda[p] = rhs[p];
				for(int q = 0; q < ne; ++q){
					Msys[p * ne + q] = M[p * ne + q];
				}
				Msys[p * ne + p] += reg;
			}
			if(eqm_dense_solve(Msys, lambda, ne)){
				solved = 1;
				break;
			}
			reg = (reg == 0.0) ? 1e-18 : (reg * 100.0);
		}
		if(!solved){
			free(M);
			free(Msys);
			free(rhs);
			free(lambda);
			return 0;
		}
	}
	for(int i = 0; i < ns; ++i){
		double r = mu[i];
		for(int p = 0; p < ne; ++p){
			r += A[p * ns + i] * lambda[p];
		}
		red[i] = r / (gas_R() * T);
	}
	free(M);
	free(Msys);
	free(rhs);
	free(lambda);
	return 1;
}

static int eqm_reduced_active_set_seed(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_hint,
		double n_floor, double *n_seed_out){
	const int max_iter = 24;
	const double pin = n_floor;
	const double active_seed_cut = fmax(1e-30, 1e6 * n_floor);
	const double active_n_cut = fmax(1e-30, 1e3 * n_floor);
	const double free_tol = EQM_BOUND_KKT_FREE_TOL;
	const double dual_tol = EQM_BOUND_KKT_DUAL_TOL;
	const double P0 = 1e5;
	const int trace = eqm_active_trace_enabled();
	int *is_active = NULL;
	int *free_idx = NULL;
	const char **names_f = NULL;
	double *A_f = NULL;
	double *b_f = NULL;
	double *n_f = NULL;
	double *init_f = NULL;
	double *n_work = NULL;
	double *n_trial = NULL;
	double *mu0 = NULL;
	double *mu = NULL;
	double *red = NULL;
	double obj_dummy = 0.0;
	int ok = 0;
	int status = -13;

	if(!names || !A || !b || !n_hint || !n_seed_out || ns <= 1 || ne <= 0){
		return 0;
	}
	is_active = (int *)calloc((size_t)ns, sizeof(int));
	free_idx = (int *)calloc((size_t)ns, sizeof(int));
	names_f = (const char **)calloc((size_t)ns, sizeof(const char *));
	A_f = (double *)calloc((size_t)(ne * ns), sizeof(double));
	b_f = (double *)calloc((size_t)ne, sizeof(double));
	n_f = (double *)calloc((size_t)ns, sizeof(double));
	init_f = (double *)calloc((size_t)ns, sizeof(double));
	n_work = (double *)calloc((size_t)ns, sizeof(double));
	n_trial = (double *)calloc((size_t)ns, sizeof(double));
	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	red = (double *)calloc((size_t)ns, sizeof(double));
	if(!is_active || !free_idx || !names_f || !A_f || !b_f || !n_f || !init_f
			|| !n_work || !n_trial || !mu0 || !mu || !red){
		goto cleanup;
	}
	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		goto cleanup;
	}
	for(int i = 0; i < ns; ++i){
		double ni = n_hint[i];
		if(!(ni > 0.0) || !isfinite(ni)){
			ni = 1.0;
		}
		n_work[i] = ni;
		if(ni <= active_seed_cut){
			is_active[i] = 1;
		}
	}
	if(trace){
		fprintf(stderr, "eqm active-set seed start: T=%.6g P=%.6g n_floor=%.3e\n", T, P, n_floor);
	}

	for(int it = 0; it < max_iter; ++it){
		int nf = 0;
		int add_idx = -1;
		int drop_idx = -1;
		double add_score = 0.0;
		double drop_score = 0.0;
		double max_free_resid = 0.0;

		for(int i = 0; i < ns; ++i){
			if(!is_active[i]){
				free_idx[nf++] = i;
			}
		}
		if(nf <= 0){
			goto cleanup;
		}
		if(trace){
			fprintf(stderr, "  it=%d nf=%d active={", it, nf);
			{
				int first = 1;
				for(int i = 0; i < ns; ++i){
					if(is_active[i]){
						fprintf(stderr, "%s%d", first ? "" : ",", i);
						first = 0;
					}
				}
			}
			fprintf(stderr, "}\n");
		}
		for(int e = 0; e < ne; ++e){
			double rhs = b[e];
			for(int i = 0; i < ns; ++i){
				if(is_active[i]){
					rhs -= A[e * ns + i] * pin;
				}
			}
			b_f[e] = rhs;
		}
		for(int j = 0; j < nf; ++j){
			int i = free_idx[j];
			double ni = n_work[i];
			if(!(ni > n_floor) || !isfinite(ni)){
				ni = fmax(10.0 * n_floor, 1e-30);
			}
			names_f[j] = names[i];
			init_f[j] = ni;
			for(int e = 0; e < ne; ++e){
				A_f[e * nf + j] = A[e * ns + i];
			}
		}
		status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
			init_f, n_floor, n_f);
		if(status != 0){
			status = eqm_reduced_solve_source_init_once(names_f, nf, ne, A_f, b_f, source, T, P,
				NULL, n_floor, n_f);
		}
		if(status != 0){
			double nmax = -HUGE_VAL;
			for(int i = 0; i < ns; ++i){
				if(is_active[i] && n_work[i] > nmax){
					nmax = n_work[i];
					drop_idx = i;
				}
			}
			if(drop_idx < 0){
				goto cleanup;
			}
			if(trace){
				fprintf(stderr, "    free solve failed -> drop active species %d\n", drop_idx);
			}
			is_active[drop_idx] = 0;
			continue;
		}
		for(int i = 0; i < ns; ++i){
			n_trial[i] = is_active[i] ? pin : 0.0;
		}
		for(int j = 0; j < nf; ++j){
			n_trial[free_idx[j]] = n_f[j];
		}
		if(!eqm_reduced_eval_obj_mu(n_trial, mu0, ns, T, P, P0, &obj_dummy, mu)){
			goto cleanup;
		}
		if(!eqm_reduced_eval_reduced_gradients(mu, A, ns, ne, is_active, T, red)){
			goto cleanup;
		}

		for(int i = 0; i < ns; ++i){
			if(is_active[i]){
				if(red[i] < -dual_tol && -red[i] > drop_score){
					drop_score = -red[i];
					drop_idx = i;
				}
			}else{
				double ar = fabs(red[i]);
				if(ar > max_free_resid){
					max_free_resid = ar;
				}
				if(n_trial[i] <= active_n_cut && red[i] > dual_tol && red[i] > add_score){
					add_score = red[i];
					add_idx = i;
				}
			}
		}
		if(trace){
			fprintf(stderr,
				"    resid: max_free=%.3e add=%d(%.3e) drop=%d(%.3e)\n",
				max_free_resid, add_idx, add_score, drop_idx, drop_score);
		}
		for(int i = 0; i < ns; ++i){
			n_work[i] = n_trial[i];
		}
		if(add_idx < 0 && drop_idx < 0 && max_free_resid <= free_tol){
			for(int i = 0; i < ns; ++i){
				n_seed_out[i] = n_work[i];
			}
			if(trace){
				fprintf(stderr, "  active-set converged (KKT) in %d iter\n", it + 1);
			}
			ok = 1;
			goto cleanup;
		}
		if(add_idx >= 0 && drop_idx >= 0){
			if(add_score >= drop_score){
				drop_idx = -1;
			}else{
				add_idx = -1;
			}
		}
		if(add_idx >= 0){
			if(trace){
				fprintf(stderr, "    pivot: add species %d to active\n", add_idx);
			}
			is_active[add_idx] = 1;
		}else if(drop_idx >= 0){
			if(trace){
				fprintf(stderr, "    pivot: drop species %d from active\n", drop_idx);
			}
			is_active[drop_idx] = 0;
		}else{
			for(int i = 0; i < ns; ++i){
				n_seed_out[i] = n_work[i];
			}
			if(trace){
				fprintf(stderr, "  active-set accepted without pivot in %d iter\n", it + 1);
			}
			ok = 1;
			goto cleanup;
		}
	}
	if(trace){
		fprintf(stderr, "  active-set seed exhausted max_iter=%d\n", max_iter);
	}

cleanup:
	free(red);
	free(mu);
	free(mu0);
	free(n_trial);
	free(n_work);
	free(init_f);
	free(n_f);
	free(b_f);
	free(A_f);
	free(names_f);
	free(free_idx);
	free(is_active);
	return ok;
}

static int eqm_validate_solution_bounds(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_out){
	const double elem_tol = 1e-6;
	const double free_tol = EQM_BOUND_KKT_FREE_TOL;
	const double dual_tol = EQM_BOUND_KKT_DUAL_TOL;
	const double P0 = 1e5;
	double n_tot = 0.0;
	double n_active_cutoff;
	double *mu0 = NULL;
	double *mu = NULL;
	double *M = NULL;
	double *Msys = NULL;
	double *rhs = NULL;
	double *lambda = NULL;
	int *is_active = NULL;
	int nfree = 0;
	int nactive = 0;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}

	for(int i = 0; i < ns; ++i){
		if(!isfinite(n_out[i]) || n_out[i] <= 0.0){
			return 0;
		}
		n_tot += n_out[i];
	}
	if(!isfinite(n_tot) || n_tot <= 0.0){
		return 0;
	}

	for(int e = 0; e < ne; ++e){
		double lhs = 0.0;
		double resid;
		double denom = fabs(b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		for(int i = 0; i < ns; ++i){
			lhs += A[e * ns + i] * n_out[i];
		}
		resid = lhs - b[e];
		if(!isfinite(resid) || fabs(resid) > elem_tol * denom){
			return 0;
		}
	}

	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	M = (double *)calloc((size_t)(ne * ne), sizeof(double));
	Msys = (double *)calloc((size_t)(ne * ne), sizeof(double));
	rhs = (double *)calloc((size_t)ne, sizeof(double));
	lambda = (double *)calloc((size_t)ne, sizeof(double));
	is_active = (int *)calloc((size_t)ns, sizeof(int));
	if(!mu0 || !mu || !M || !Msys || !rhs || !lambda || !is_active){
		free(mu0);
		free(mu);
		free(M);
		free(Msys);
		free(rhs);
		free(lambda);
		free(is_active);
		return 0;
	}
	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		free(mu0);
		free(mu);
		free(M);
		free(Msys);
		free(rhs);
		free(lambda);
		free(is_active);
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		double y_i = n_out[i] / n_tot;
		double logarg = y_i * P / P0;
		if(!(y_i > 0.0) || !(logarg > 0.0) || !isfinite(logarg)){
			free(mu0);
			free(mu);
			free(M);
			free(Msys);
			free(rhs);
			free(lambda);
			free(is_active);
			return 0;
		}
		mu[i] = mu0[i] + gas_R() * T * log(logarg);
		if(!isfinite(mu[i])){
			free(mu0);
			free(mu);
			free(M);
			free(Msys);
			free(rhs);
			free(lambda);
			free(is_active);
			return 0;
		}
	}

	n_active_cutoff = fmax(1e-60, EQM_BOUND_ACTIVE_CUTOFF_FRAC * n_tot);
	for(int i = 0; i < ns; ++i){
		if(n_out[i] <= n_active_cutoff){
			is_active[i] = 1;
			++nactive;
		}else{
			++nfree;
		}
	}
	if(nactive == 0 || nfree <= 0){
		free(mu0);
		free(mu);
		free(M);
		free(Msys);
		free(rhs);
		free(lambda);
		free(is_active);
		return 0;
	}

	for(int p = 0; p < ne; ++p){
		double bp = 0.0;
		for(int i = 0; i < ns; ++i){
			if(is_active[i]){
				continue;
			}
			bp += A[p * ns + i] * mu[i];
		}
		rhs[p] = -bp;
		for(int q = 0; q < ne; ++q){
			double s = 0.0;
			for(int i = 0; i < ns; ++i){
				if(is_active[i]){
					continue;
				}
				s += A[p * ns + i] * A[q * ns + i];
			}
			M[p * ne + q] = s;
		}
	}
	{
		double reg = 0.0;
		int solved = 0;
		for(int damp = 0; damp < 8; ++damp){
			for(int p = 0; p < ne; ++p){
				lambda[p] = rhs[p];
				for(int q = 0; q < ne; ++q){
					Msys[p * ne + q] = M[p * ne + q];
				}
				Msys[p * ne + p] += reg;
			}
			if(eqm_dense_solve(Msys, lambda, ne)){
				solved = 1;
				break;
			}
			reg = (reg == 0.0) ? 1e-18 : (reg * 100.0);
		}
		if(!solved){
			free(mu0);
			free(mu);
			free(M);
			free(Msys);
			free(rhs);
			free(lambda);
			free(is_active);
			return 0;
		}
	}

	for(int i = 0; i < ns; ++i){
		double red = mu[i];
		for(int p = 0; p < ne; ++p){
			red += A[p * ns + i] * lambda[p];
		}
		red /= (gas_R() * T);
		if(is_active[i]){
			if(red < -dual_tol){
				free(mu0);
				free(mu);
				free(M);
				free(Msys);
				free(rhs);
				free(lambda);
				free(is_active);
				return 0;
			}
		}else{
			if(fabs(red) > free_tol){
				free(mu0);
				free(mu);
				free(M);
				free(Msys);
				free(rhs);
				free(lambda);
				free(is_active);
				return 0;
			}
		}
	}

	free(mu0);
	free(mu);
	free(M);
	free(Msys);
	free(rhs);
	free(lambda);
	free(is_active);
	return 1;
}

static int eqm_reduced_solve_source_init(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out){
	static const double floor_knots[] = {
		1e-12, 1e-18, 1e-24, 1e-30, 1e-40, 1e-50, 1e-60, 1e-80, 1e-100, 1e-120
	};
	double schedule[16];
	int nsteps = 0;
	double *n_seed = NULL;
	double *n_work = NULL;
	double *n_polish = NULL;
	const double *init = n_init;
	const double n_floor_target = 1e-120;
	int status = -13;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return -11;
	}
	status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source, T, P, n_init,
		n_floor_target, n_out);
	if(status == 0){
		return 0;
	}

	{
		/* Relative continuation ladder, avoids hardwired absolute temperature knots. */
		double Tk = 2.5 * T;
		int guard = 0;
		while(Tk > T + 1e-9 && nsteps < (int)(sizeof(schedule) / sizeof(schedule[0])) - 1){
			schedule[nsteps++] = Tk;
			Tk *= 0.82;
			if(++guard > 64){
				break;
			}
		}
	}
	schedule[nsteps++] = T;

	n_seed = (double *)calloc((size_t)ns, sizeof(double));
	n_work = (double *)calloc((size_t)ns, sizeof(double));
	n_polish = (double *)calloc((size_t)ns, sizeof(double));
	if(!n_seed || !n_work || !n_polish){
		free(n_seed);
		free(n_work);
		free(n_polish);
		return status;
	}

	for(int s = 0; s < nsteps; ++s){
		double Tk = schedule[s];
		int any_ok = 0;
		for(size_t fk = 0; fk < sizeof(floor_knots) / sizeof(floor_knots[0]); ++fk){
			double nf = floor_knots[fk];
			if(nf < n_floor_target){
				nf = n_floor_target;
			}
			status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
				Tk, P, init, nf, n_work);
			if(status != 0 && init != NULL){
				status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
					Tk, P, NULL, nf, n_work);
			}
			if(status != 0 && init != NULL && Tk <= 1.08 * T){
				if(eqm_reduced_active_set_seed(names, ns, ne, A, b, source, Tk, P, init, nf, n_work)){
					status = eqm_reduced_solve_source_init_once(names, ns, ne, A, b, source,
						Tk, P, n_work, nf, n_polish);
					if(status == 0){
						for(int i = 0; i < ns; ++i){
							n_work[i] = n_polish[i];
						}
					}else if(eqm_validate_solution_bounds(names, ns, ne, A, b, source, Tk, P, n_work)){
						status = 0;
					}
				}
			}
			if(status != 0){
				/* Small-floor continuation can fail near boundary optima; keep last good floor. */
				if(any_ok){
					continue;
				}
				continue;
			}
			any_ok = 1;
			for(int i = 0; i < ns; ++i){
				n_seed[i] = n_work[i];
			}
			init = n_seed;
		}
			if(!any_ok){
				free(n_seed);
				free(n_work);
				free(n_polish);
				return status;
			}
		for(int i = 0; i < ns; ++i){
			n_seed[i] = n_work[i];
		}
		init = n_seed;
	}
	for(int i = 0; i < ns; ++i){
		n_out[i] = n_work[i];
	}
	free(n_seed);
	free(n_work);
	free(n_polish);
	return 0;
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

static int eqm_alg_exact(const char *algorithm, const char *name){
	return algorithm && strcmp(algorithm, name) == 0;
}

static int eqm_alg_reduced(const char *algorithm){
	return algorithm && eqm_alg_prefix(algorithm, "reduced");
}

static int eqm_alg_auto_reduced(const char *algorithm){
	return eqm_alg_exact(algorithm, "auto_reduced");
}

static int eqm_alg_use_nullspace(const char *algorithm){
	if(eqm_alg_exact(algorithm, "auto_no_nullspace")
			|| eqm_alg_exact(algorithm, "no_nullspace")
			|| eqm_alg_auto_reduced(algorithm)
			|| eqm_alg_reduced(algorithm)){
		return 0;
	}
	if(eqm_alg_prefix(algorithm, "ipopt")){
		return 1;
	}
	if(eqm_alg_auto(algorithm)
			|| eqm_alg_exact(algorithm, "auto_nullspace")
			|| eqm_alg_exact(algorithm, "nullspace")
			|| eqm_alg_exact(algorithm, "ipopt_nullspace")){
		return 1;
	}
	return 0;
}

static int eqm_alg_nullspace_only(const char *algorithm){
	return eqm_alg_exact(algorithm, "nullspace")
		|| eqm_alg_exact(algorithm, "ipopt_nullspace");
}

static const char *eqm_alg_fallback(const char *algorithm){
	if(eqm_alg_exact(algorithm, "auto_nullspace")
			|| eqm_alg_exact(algorithm, "auto_no_nullspace")
			|| eqm_alg_exact(algorithm, "no_nullspace")){
		return "auto";
	}
	return algorithm;
}

#ifdef HAVE_IPOPT
static int eqm_status_ok_ipopt(int status){
	return status == 0 || status == 1 || status == 6;
}
#endif

#ifdef HAVE_NLOPT
static int eqm_status_ok_slsqp(int status){
	return status == 0;
}
#endif

#if defined(HAVE_IPOPT) || defined(HAVE_NLOPT)
static int eqm_validate_solution(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_out){
	const double elem_tol = 1e-6;
	const double stat_tol = 1e-1;
	const double P0 = 1e5;
	double n_tot = 0.0;
	double *mu0 = NULL;
	double *mu = NULL;
	double *Awork = NULL;
	double *N = NULL;
	int *pivots = NULL;
	int rank = 0;
	int r = 0;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0 || !(T > 0.0) || !(P > 0.0)){
		return 0;
	}

	for(int i = 0; i < ns; ++i){
		if(!isfinite(n_out[i]) || n_out[i] <= 0.0){
			fprintf(stderr, "eqm validate failed: invalid n[%d]=%.17g\n", i, n_out[i]);
			return 0;
		}
		n_tot += n_out[i];
	}
	if(!isfinite(n_tot) || n_tot <= 0.0){
		fprintf(stderr, "eqm validate failed: invalid n_tot=%.17g\n", n_tot);
		return 0;
	}

	for(int e = 0; e < ne; ++e){
		double lhs = 0.0;
		double resid;
		double denom = fabs(b[e]);
		if(denom < 1.0){
			denom = 1.0;
		}
		for(int i = 0; i < ns; ++i){
			lhs += A[e * ns + i] * n_out[i];
		}
		resid = lhs - b[e];
		if(!isfinite(resid) || fabs(resid) > elem_tol * denom){
			fprintf(stderr,
				"eqm validate failed: element residual e=%d lhs=%.17g rhs=%.17g resid=%.17g\n",
				e, lhs, b[e], resid);
			return 0;
		}
	}

	mu0 = (double *)calloc((size_t)ns, sizeof(double));
	mu = (double *)calloc((size_t)ns, sizeof(double));
	Awork = (double *)calloc((size_t)(ne * ns), sizeof(double));
	pivots = (int *)calloc((size_t)ne, sizeof(int));
	if(!mu0 || !mu || !Awork || !pivots){
		free(mu0);
		free(mu);
		free(Awork);
		free(pivots);
		return 0;
	}

	if(!eqm_compute_mu0(names, ns, source, T, P0, mu0)){
		free(mu0);
		free(mu);
		free(Awork);
		free(pivots);
		return 0;
	}
	for(int i = 0; i < ns; ++i){
		double y_i = n_out[i] / n_tot;
		double logarg = y_i * P / P0;
		if(!(y_i > 0.0) || !(logarg > 0.0) || !isfinite(logarg)){
			fprintf(stderr, "eqm validate failed: invalid activity for i=%d\n", i);
			free(mu0);
			free(mu);
			free(Awork);
			free(pivots);
			return 0;
		}
		mu[i] = mu0[i] + gas_R() * T * log(logarg);
		if(!isfinite(mu[i])){
			fprintf(stderr, "eqm validate failed: invalid mu[%d]\n", i);
			free(mu0);
			free(mu);
			free(Awork);
			free(pivots);
			return 0;
		}
	}

	for(int i = 0; i < ne * ns; ++i){
		Awork[i] = A[i];
	}
	eqm_rref(Awork, ne, ns, pivots, &rank);
	r = ns - rank;
	if(r > 0){
		N = (double *)calloc((size_t)(ns * r), sizeof(double));
		if(!N){
			free(mu0);
			free(mu);
			free(Awork);
			free(pivots);
			return 0;
		}
		eqm_fill_nullspace(Awork, ne, ns, pivots, rank, N, r);
		for(int j = 0; j < r; ++j){
			double normv = 0.0;
			double dot = 0.0;
			for(int i = 0; i < ns; ++i){
				double v = N[i * r + j];
				double av = fabs(v);
				if(av > normv){
					normv = av;
				}
				dot += v * mu[i];
			}
			if(normv > 0.0){
					double scaled = fabs(dot / normv) / (gas_R() * T);
				if(!isfinite(scaled) || scaled > stat_tol){
					fprintf(stderr,
						"eqm validate failed: stationarity col=%d scaled=%.17g\n",
						j, scaled);
						free(N);
						free(mu0);
						free(mu);
						free(Awork);
					free(pivots);
					return 0;
				}
			}
		}
	}

	free(N);
	free(mu0);
	free(mu);
	free(Awork);
	free(pivots);
	return 1;
}
#endif

static int eqm_solution_valid(const char **names, int ns, int ne, const double *A,
		const double *b, const char *source, double T, double P, const double *n_out){
#if defined(HAVE_IPOPT) || defined(HAVE_NLOPT)
	if(eqm_validate_solution_bounds(names, ns, ne, A, b, source, T, P, n_out)){
		return 1;
	}
	return eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out);
#else
	(void)names;
	(void)ns;
	(void)ne;
	(void)A;
	(void)b;
	(void)source;
	(void)T;
	(void)P;
	(void)n_out;
	return 1;
#endif
}

#ifdef HAVE_IPOPT
static int eqm_try_ipopt(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out){
	int status;
	if(algorithm && strstr(algorithm, "logn")){
		status = eqm_ipopt_solve_logn_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_ipopt(status)
				&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_ipopt(status)){
			return -13;
		}
		return status;
	}
	if(algorithm && strstr(algorithm, "_n")){
		status = eqm_ipopt_solve_n_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_ipopt(status)
				&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_ipopt(status)){
			return -13;
		}
		return status;
	}
	status = eqm_ipopt_solve_logn_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	if(eqm_status_ok_ipopt(status)
			&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
		return status;
	}
	status = eqm_ipopt_solve_n_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	if(eqm_status_ok_ipopt(status)
			&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
		return status;
	}
	status = eqm_ipopt_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
	if(eqm_status_ok_ipopt(status)
			&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
		return status;
	}
	if(eqm_status_ok_ipopt(status)){
		return -13;
	}
	return status;
}
#endif

int eqm_solve(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out){
	int status = -99;

	if(!names || !A || !b || !n_out || ns <= 0 || ne <= 0){
		return -11;
	}
	if(eqm_alg_auto_reduced(algorithm)){
		status = eqm_reduced_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(status == 0 && eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			return 0;
		}
		if(status == 0){
			status = -13;
		}
		algorithm = "auto";
	}
	if(eqm_alg_reduced(algorithm)){
		status = eqm_reduced_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(status == 0 && eqm_solution_valid(names, ns, ne, A, b, source, T, P, n_out)){
			return 0;
		}
		if(status == 0){
			return -13;
		}
		return status;
	}
	if(eqm_alg_auto(algorithm)){
#ifdef HAVE_IPOPT
		status = eqm_try_ipopt(names, ns, ne, A, b, source, T, P, NULL, n_init, n_out);
		if(eqm_status_ok_ipopt(status)){
			return status;
		}
#endif
#ifdef HAVE_NLOPT
		status = eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_slsqp(status)
				&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_slsqp(status)){
			status = -13;
		}
#endif
		return status;
	}
#ifdef HAVE_IPOPT
	if(eqm_alg_prefix(algorithm, "ipopt")){
		status = eqm_try_ipopt(names, ns, ne, A, b, source, T, P, algorithm, n_init, n_out);
		if(eqm_status_ok_ipopt(status)){
			return status;
		}
		return status;
	}
#endif
#ifdef HAVE_NLOPT
	if(eqm_alg_prefix(algorithm, "slsqp")){
		status = eqm_slsqp_solve_source_init(names, ns, ne, A, b, source, T, P, n_init, n_out);
		if(eqm_status_ok_slsqp(status)
				&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
			return status;
		}
		if(eqm_status_ok_slsqp(status)){
			return -13;
		}
		return status;
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
#ifdef HAVE_IPOPT
	if(eqm_alg_use_nullspace(algorithm)){
		status = eqm_ipopt_nullspace_solve_source(names, ns, elements, ne, source, b, T, P, n_out);
		if(eqm_status_ok_ipopt(status)
				&& eqm_validate_solution(names, ns, ne, A, b, source, T, P, n_out)){
			free(A);
			return status;
		}
		if(eqm_alg_nullspace_only(algorithm)){
			free(A);
			if(eqm_status_ok_ipopt(status)){
				return -13;
			}
			return status;
		}
	}
#endif
	status = eqm_solve(names, ns, ne, A, b, source, T, P, eqm_alg_fallback(algorithm), n_init, n_out);
	free(A);
	return status;
}
