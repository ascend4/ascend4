#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../eqm.h"
#include "../eqm_ipopt.h"
#ifdef HAVE_NLOPT
#include "../eqm_slsqp.h"
#endif

static double log10K_from_mu0(const char **names, const double *nu, int ns,
		const char *source, double T, double P0, int *ok){
	double sum = 0.0;
	for(int i = 0; i < ns; ++i){
		double mu0 = 0.0;
		if(!eqm_mu0_ideal_source(names[i], source, T, P0, &mu0)){
			*ok = 0;
			return 0.0;
		}
		sum += nu[i] * mu0;
	}
	*ok = 1;
	return -sum / (8.31446261815324 * T * log(10.0));
}

static double log10K_from_n(const double *n, const double *nu, int ns, double P, double P0){
	double sum = 0.0;
	double nu_sum = 0.0;
	double *logn = (double *)calloc((size_t)ns, sizeof(double));
	double maxv = -HUGE_VAL;
	double logn_tot;
	const double ln10 = log(10.0);

	if(!logn){
		return NAN;
	}
	for(int i = 0; i < ns; ++i){
		if(n[i] <= 0.0){
			free(logn);
			return NAN;
		}
		logn[i] = log(n[i]);
		if(logn[i] > maxv){
			maxv = logn[i];
		}
	}
	if(!isfinite(maxv)){
		free(logn);
		return NAN;
	}
	{
		double sumexp = 0.0;
		for(int i = 0; i < ns; ++i){
			sumexp += exp(logn[i] - maxv);
		}
		if(sumexp <= 0.0){
			free(logn);
			return NAN;
		}
		logn_tot = maxv + log(sumexp);
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

static int log10K_water_table(double T, double *log10K){
	if(fabs(T - 298.0) < 1e-6){
		*log10K = -40.08;
		return 1;
	}
	if(fabs(T - 500.0) < 1e-6){
		*log10K = -22.886;
		return 1;
	}
	if(fabs(T - 1000.0) < 1e-6){
		*log10K = -10.062;
		return 1;
	}
	return 0;
}

static void seed_from_log10K(double log10K, double P, double P0, double *n_seed){
	double Kp = pow(10.0, log10K);
	double factor = sqrt(P / P0) * sqrt(0.5);
	double xi = pow(Kp / factor, 2.0 / 3.0);
	if(xi < 1e-40){
		xi = 1e-40;
	}
	if(xi > 0.1){
		xi = 0.1;
	}
	n_seed[0] = xi;
	n_seed[1] = 0.5 * xi;
	n_seed[2] = 1.0 - xi;
}

static int ipopt_status_ok(int status){
	return status == 0 || status == 1 || status == 6;
}

static int solve_eqm_api_ipopt(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out){
	int status = eqm_solve_elements(names, ns, elements, ne, b, source, T, P,
		"ipopt_logn", n_init, n_out);
	if(ipopt_status_ok(status)){
		return status;
	}
	status = eqm_solve_elements(names, ns, elements, ne, b, source, T, P,
		"ipopt_n", n_init, n_out);
	if(ipopt_status_ok(status)){
		return status;
	}
	status = eqm_solve_elements(names, ns, elements, ne, b, source, T, P,
		"slsqp", n_init, n_out);
	return status;
}

static int solve_eqm_api_reduced(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const double *n_init,
		double *n_out){
	return eqm_solve_elements(names, ns, elements, ne, b, source, T, P,
		"reduced", n_init, n_out);
}

static double max_absdiff(const double *a, const double *b, int n){
	double maxd = 0.0;
	for(int i = 0; i < n; ++i){
		double d = fabs(a[i] - b[i]);
		if(d > maxd){
			maxd = d;
		}
	}
	return maxd;
}

static int find_name(const char **names, int n, const char *name){
	for(int i = 0; i < n; ++i){
		if(0 == strcmp(names[i], name)){
			return i;
		}
	}
	return -1;
}

static int run_wgs_invariance_checks(const char *source_ms, double P0){
	static const char *wgs_names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *wgs_elements[] = {"C", "O", "H"};
	static const double wgs_b[] = {1.0, 2.0, 2.0};
	static const double nu_wgs[] = {1.0, 1.0, -1.0, -1.0};
	static const double pressure_sweep[] = {1e6, 101325.0, 1e4};
	static const char *wgs_names_perm[] = {"hydrogen", "carbondioxide", "carbonmonoxide", "water"};
	static const char *wgs_elements_perm[] = {"H", "C", "O"};
	static const double wgs_b_perm[] = {2.0, 1.0, 2.0};
	static const char *wgs_inert_names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen", "nitrogen"};
	static const char *wgs_inert_elements[] = {"C", "O", "H", "N"};
	static const double wgs_inert_b[] = {1.0, 2.0, 2.0, 2.0}; /* +1 mol N2 */

	const double T = 1000.0;
	const double n_tol = 1e-3;
	const double log10K_tol = 2e-3;
	int ok = 1;
	int status;
	int ok_mu0 = 1;
	double log10K_mu0 = log10K_from_mu0(wgs_names, nu_wgs, 4, source_ms, T, P0, &ok_mu0);
	double n_base[4];
	double n_seed[4];
	double n_sweep[4];
	double n_perm[4];
	double n_elem_perm[4];
	double n_inert[5];
	double n_inert_seed[5];

	printf("\nIdeal-gas invariance checks (WGS, T=%.1f K)\n", T);
	if(!ok_mu0){
		fprintf(stderr, "invariance: failed to evaluate mu0-based Kp at T=%.1f\n", T);
		return 0;
	}

	status = solve_eqm_api_ipopt(wgs_names, 4, wgs_elements, 3, wgs_b,
		source_ms, T, pressure_sweep[1], NULL, n_base);
	if(!ipopt_status_ok(status)){
		fprintf(stderr, "invariance: base solve failed at P=%.0f Pa (status %d)\n",
			pressure_sweep[1], status);
		return 0;
	}
	for(int i = 0; i < 4; ++i){
		n_seed[i] = n_base[i];
	}

#ifdef HAVE_IPOPT
	{
		double n_auto_ns[4];
		double n_auto_no_ns[4];
		double n_ns_only[4];
		double ndiff;
		status = eqm_solve_elements(wgs_names, 4, wgs_elements, 3, wgs_b,
			source_ms, T, pressure_sweep[1], "auto_nullspace", n_seed, n_auto_ns);
		if(!ipopt_status_ok(status)){
			fprintf(stderr, "invariance: auto_nullspace solve failed (status %d)\n", status);
			ok = 0;
		}
		status = eqm_solve_elements(wgs_names, 4, wgs_elements, 3, wgs_b,
			source_ms, T, pressure_sweep[1], "auto_no_nullspace", n_seed, n_auto_no_ns);
		if(!ipopt_status_ok(status)){
			fprintf(stderr, "invariance: auto_no_nullspace solve failed (status %d)\n", status);
			ok = 0;
		}
		status = eqm_solve_elements(wgs_names, 4, wgs_elements, 3, wgs_b,
			source_ms, T, pressure_sweep[1], "nullspace", n_seed, n_ns_only);
		if(!ipopt_status_ok(status)){
			fprintf(stderr, "invariance: nullspace-only solve failed (status %d)\n", status);
			ok = 0;
		}
		ndiff = max_absdiff(n_auto_ns, n_auto_no_ns, 4);
		if(ndiff > 1e-2){
			fprintf(stderr, "invariance: nullspace switch changed composition too much: max|dn|=%.3e\n",
				ndiff);
			ok = 0;
		}
	}
#endif

	for(size_t i = 0; i < sizeof(pressure_sweep)/sizeof(pressure_sweep[0]); ++i){
		double P = pressure_sweep[i];
		double log10K;
		double ndiff;
		status = solve_eqm_api_ipopt(wgs_names, 4, wgs_elements, 3, wgs_b,
			source_ms, T, P, n_seed, n_sweep);
		if(!ipopt_status_ok(status)){
			fprintf(stderr, "invariance: pressure sweep failed at P=%.0f Pa (status %d)\n",
				P, status);
			ok = 0;
			continue;
		}
		log10K = log10K_from_n(n_sweep, nu_wgs, 4, P, P0);
		if(!isfinite(log10K) || fabs(log10K - log10K_mu0) > log10K_tol){
			fprintf(stderr, "invariance: pressure Kp mismatch at P=%.0f Pa: eq=%.5f mu0=%.5f\n",
				P, log10K, log10K_mu0);
			ok = 0;
		}
		ndiff = max_absdiff(n_sweep, n_base, 4);
		if(ndiff > n_tol){
			fprintf(stderr, "invariance: pressure composition shift at P=%.0f Pa: max|dn|=%.3e\n",
				P, ndiff);
			ok = 0;
		}
		for(int j = 0; j < 4; ++j){
			n_seed[j] = n_sweep[j];
		}
	}

	for(int i = 0; i < 4; ++i){
		n_inert_seed[i] = n_base[i];
	}
	{
		double mu0_n2 = 0.0;
		int ok_n2 = eqm_mu0_ideal_source("nitrogen", source_ms, T, P0, &mu0_n2);
		if(ok_n2){
			n_inert_seed[4] = 1.0;
			status = solve_eqm_api_ipopt(wgs_inert_names, 5, wgs_inert_elements, 4, wgs_inert_b,
				source_ms, T, pressure_sweep[1], n_inert_seed, n_inert);
			if(!ipopt_status_ok(status)){
				fprintf(stderr, "invariance: inert-dilution solve failed (status %d)\n", status);
				ok = 0;
			}else{
				double log10K = log10K_from_n(n_inert, nu_wgs, 4, pressure_sweep[1], P0);
				double ndiff = max_absdiff(n_inert, n_base, 4);
				if(!isfinite(log10K) || fabs(log10K - log10K_mu0) > log10K_tol){
					fprintf(stderr, "invariance: inert Kp mismatch: eq=%.5f mu0=%.5f\n",
						log10K, log10K_mu0);
					ok = 0;
				}
				if(ndiff > n_tol){
					fprintf(stderr, "invariance: inert changed reactive composition: max|dn|=%.3e\n",
						ndiff);
					ok = 0;
				}
				if(fabs(n_inert[4] - 1.0) > n_tol){
					fprintf(stderr, "invariance: inert species amount drifted: nN2=%.8f\n", n_inert[4]);
					ok = 0;
				}
			}
		}else{
			fprintf(stderr, "invariance: skip inert-dilution check (nitrogen mu0 unavailable)\n");
		}
	}

	status = solve_eqm_api_ipopt(wgs_names_perm, 4, wgs_elements, 3, wgs_b,
		source_ms, T, pressure_sweep[1], NULL, n_perm);
	if(!ipopt_status_ok(status)){
		fprintf(stderr, "invariance: species-permuted solve failed (status %d)\n", status);
		ok = 0;
	}else{
		double ndiff = 0.0;
		for(int i = 0; i < 4; ++i){
			int idx = find_name(wgs_names_perm, 4, wgs_names[i]);
			double d;
			if(idx < 0){
				fprintf(stderr, "invariance: species map missing '%s'\n", wgs_names[i]);
				ok = 0;
				break;
			}
			d = fabs(n_base[i] - n_perm[idx]);
			if(d > ndiff){
				ndiff = d;
			}
		}
		if(ndiff > n_tol){
			fprintf(stderr, "invariance: species permutation mismatch: max|dn|=%.3e\n", ndiff);
			ok = 0;
		}
	}

	status = solve_eqm_api_ipopt(wgs_names, 4, wgs_elements_perm, 3, wgs_b_perm,
		source_ms, T, pressure_sweep[1], NULL, n_elem_perm);
	if(!ipopt_status_ok(status)){
		fprintf(stderr, "invariance: element-permuted solve failed (status %d)\n", status);
		ok = 0;
	}else{
		double ndiff = max_absdiff(n_elem_perm, n_base, 4);
		if(ndiff > n_tol){
			fprintf(stderr, "invariance: element permutation mismatch: max|dn|=%.3e\n", ndiff);
			ok = 0;
		}
	}

	if(ok){
		printf("  invariance checks: PASS\n");
	}else{
		printf("  invariance checks: FAIL (see stderr)\n");
	}
	return ok;
}

int main(void){
	/* H2O <-> H2 + 0.5 O2 equilibrium validation. */
	static const double b[] = {2.0, 1.0}; /* element totals: H, O */
	static const char *elements[] = {"H", "O"};
	static const char *spec_names[] = {"hydrogen", "oxygen", "water"};
	static const struct{
		double T;
		double log10K;
	} ktab[] = {
		{1000.0, -10.062}
		,{900.0, -11.601}
		,{800.0, -13.427}
		,{700.0, -15.538}
		,{600.0, -17.933}
		,{500.0, -22.886}
		,{298.0, -40.08}
		,{1600.0, -5.180}
		,{2300.0, -2.682}
		,{3000.0, -1.265}
		,{3500.0, -0.712}
	};
	const double P = 101325.0;
	const double P0 = 1e5;
	const double log10_tol = 0.3;
	const double Tmin = 298.0;
	const double Tmax = 1000.0;
	const double log10K_eqm_min = -100.0;
	const char *source_ms = "Moran and Shapiro";
	int status;
	int ok = 1;

	for(size_t i = 0; i < sizeof(ktab)/sizeof(ktab[0]); ++i){
		const double nu_h2o[] = {1.0, 0.5, -1.0};
		double n_out[3];
		static double n_prev[3];
		static int have_prev = 0;
		double n_seed[3];
		const double *n_init = NULL;
		double n_tot;
		double y_h2;
		double y_o2;
		double y_h2o;
		double p_h2;
		double p_o2;
		double p_h2o;
		double Kp;
		double log10K;

		printf("T=%.1f K\n", ktab[i].T);
		if(ktab[i].T < Tmin || ktab[i].T > Tmax){
			printf("  skip: outside Moran/Shapiro cp range\n");
			continue;
		}

		{
			int ok_mu0 = 1;
			double log10Kmu = log10K_from_mu0(spec_names, nu_h2o, 3, source_ms, ktab[i].T, P0, &ok_mu0);
			if(ok_mu0){
				printf("  Kp(mu0) = %.3f (table %.3f)\n", log10Kmu, ktab[i].log10K);
			}else{
				printf("  Kp(mu0) = (failed)\n");
			}
			if(ok_mu0 && fabs(log10Kmu - ktab[i].log10K) > log10_tol){
				fprintf(stderr, "Kp(mu0) mismatch at T=%.1f: got %.3f expected %.3f\n",
					ktab[i].T, log10Kmu, ktab[i].log10K);
				ok = 0;
			}
		}

		if(ktab[i].log10K > log10K_eqm_min){
			printf("  eqm solve: running\n");
			if(ktab[i].log10K < -6.0){
				seed_from_log10K(ktab[i].log10K, P, P0, n_seed);
				n_init = n_seed;
			}else if(have_prev){
				n_init = n_prev;
			}
				status = solve_eqm_api_ipopt(spec_names, 3, elements, 2, b,
					source_ms, ktab[i].T, P, n_init, n_out);
				if(!ipopt_status_ok(status)){
					fprintf(stderr, "eqm_solve(ipopt_logn->ipopt_n->slsqp) failed at T=%.1f (status %d)\n",
						ktab[i].T, status);
					ok = 0;
					continue;
				}
			for(int j = 0; j < 3; ++j){
				n_prev[j] = n_out[j];
			}
			have_prev = 1;

			n_tot = n_out[0] + n_out[1] + n_out[2];
			y_h2 = n_out[0] / n_tot;
			y_o2 = n_out[1] / n_tot;
			y_h2o = n_out[2] / n_tot;
			p_h2 = y_h2 * P;
			p_o2 = y_o2 * P;
			p_h2o = y_h2o * P;
			Kp = (p_h2 * sqrt(p_o2)) / (p_h2o * sqrt(P0));
			log10K = log10(Kp);

			printf("  Kp(eq)  = %.3f (table %.3f)\n", log10K, ktab[i].log10K);
			if(fabs(log10K - ktab[i].log10K) > log10_tol){
				fprintf(stderr, "Kp(eq) mismatch at T=%.1f: got %.3f expected %.3f\n",
					ktab[i].T, log10K, ktab[i].log10K);
				ok = 0;
			}
			{
				double n_red[3];
				int red_status = solve_eqm_api_reduced(spec_names, 3, elements, 2, b,
					source_ms, ktab[i].T, P, n_init, n_red);
				if(ipopt_status_ok(red_status)){
					double log10K_red = log10K_from_n(n_red, nu_h2o, 3, P, P0);
					printf("  Kp(red) = %.3f (table %.3f)\n", log10K_red, ktab[i].log10K);
				}else{
					printf("  Kp(red) = (failed, status %d)\n", red_status);
					ok = 0;
				}
			}
			{
				double n_ns[3];
				if(eqm_ipopt_nullspace_1d(spec_names, elements, source_ms,
						b, ktab[i].T, P, n_ns)){
					double nsum = n_ns[0] + n_ns[1] + n_ns[2];
					double p_h2 = (n_ns[0] / nsum) * P;
					double p_o2 = (n_ns[1] / nsum) * P;
					double p_h2o = (n_ns[2] / nsum) * P;
					double Kp_ns = (p_h2 * sqrt(p_o2)) / (p_h2o * sqrt(P0));
					double log10K_ns = log10(Kp_ns);
					printf("  Kp(ns)  = %.3f (table %.3f)\n", log10K_ns, ktab[i].log10K);
				}else{
					printf("  Kp(ns)  = (failed)\n");
				}
			}
			{
				double n_ns[3];
				int ns_status = eqm_ipopt_nullspace_solve_source(spec_names, 3, elements, 2,
					source_ms, b, ktab[i].T, P, n_ns);
				if(ns_status == 0 || ns_status == 1 || ns_status == 6){
					double nsum = n_ns[0] + n_ns[1] + n_ns[2];
					double p_h2 = (n_ns[0] / nsum) * P;
					double p_o2 = (n_ns[1] / nsum) * P;
					double p_h2o = (n_ns[2] / nsum) * P;
					double Kp_ns = (p_h2 * sqrt(p_o2)) / (p_h2o * sqrt(P0));
					double log10K_ns = log10(Kp_ns);
					printf("  Kp(gmin) = %.3f (table %.3f)\n", log10K_ns, ktab[i].log10K);
				}else{
					printf("  Kp(gmin) = (failed, status %d)\n", ns_status);
				}
			}
#ifdef HAVE_NLOPT
			{
				double n_slsqp[3];
				int slsqp_status = eqm_slsqp_solve_elements_source_init(
					spec_names, 3, elements, 2, source_ms, b, ktab[i].T, P, n_init, n_slsqp);
				if(slsqp_status == 0){
					double nsum = n_slsqp[0] + n_slsqp[1] + n_slsqp[2];
					double p_h2 = (n_slsqp[0] / nsum) * P;
					double p_o2 = (n_slsqp[1] / nsum) * P;
					double p_h2o = (n_slsqp[2] / nsum) * P;
					double Kp_slsqp = (p_h2 * sqrt(p_o2)) / (p_h2o * sqrt(P0));
					double log10K_slsqp = log10(Kp_slsqp);
					printf("  Kp(slsqp)= %.3f (table %.3f)\n", log10K_slsqp, ktab[i].log10K);
				}else{
					printf("  Kp(slsqp)= (failed, status %d)\n", slsqp_status);
				}
			}
#endif
		}else{
			printf("  eqm solve: skipped (Kp too small)\n");
		}
	}

		{
			/* CO2 <-> CO + 0.5 O2 */
			static const char *co2_names[] = {"carbonmonoxide", "oxygen", "carbondioxide"};
			static const char *co2_elements[] = {"C", "O"};
			static const double co2_b[] = {1.0, 2.0};
		static const double nu_co2[] = {1.0, 0.5, -1.0};
		static const struct{
			double T;
			double log10K;
			} co2_tab[] = {
				{1000.0, -10.221}
				,{500.0, -25.050}
				,{298.0, -45.066}
			};
			double n_prev[3] = {0.0, 0.0, 0.0};
			int have_prev = 0;
			printf("\nCO2 <-> CO + 0.5 O2 (source: Moran and Shapiro)\n");
			for(size_t i = 0; i < sizeof(co2_tab)/sizeof(co2_tab[0]); ++i){
				double n_out[3];
				const double *n_init = have_prev ? n_prev : NULL;
				int ok_mu0 = 1;
				double log10Kmu = log10K_from_mu0(co2_names, nu_co2, 3, source_ms,
					co2_tab[i].T, P0, &ok_mu0);
			printf("T=%.1f K\n", co2_tab[i].T);
			printf("  Kp(table) = %.3f\n", co2_tab[i].log10K);
			if(ok_mu0){
				printf("  Kp(mu0)  = %.3f\n", log10Kmu);
			}else{
				printf("  Kp(mu0)  = (failed)\n");
				continue;
			}
				status = eqm_ipopt_nullspace_solve_source(co2_names, 3, co2_elements, 2,
					source_ms, co2_b, co2_tab[i].T, P, n_out);
				if(ipopt_status_ok(status)){
					double log10K = log10K_from_n(n_out, nu_co2, 3, P, P0);
					printf("  Kp(gmin)  = %.3f\n", log10K);
				}else{
					printf("  Kp(gmin)  = (failed, status %d)\n", status);
				}
				status = solve_eqm_api_ipopt(co2_names, 3, co2_elements, 2, co2_b,
					source_ms, co2_tab[i].T, P, n_init, n_out);
				if(ipopt_status_ok(status)){
					double log10K = log10K_from_n(n_out, nu_co2, 3, P, P0);
					printf("  Kp(eqm)   = %.3f\n", log10K);
					for(int j = 0; j < 3; ++j){
						n_prev[j] = n_out[j];
					}
					have_prev = 1;
				}else{
					printf("  Kp(eqm)   = (failed, status %d)\n", status);
				}
				{
					double n_red[3];
					int red_status = solve_eqm_api_reduced(co2_names, 3, co2_elements, 2, co2_b,
						source_ms, co2_tab[i].T, P, n_init, n_red);
					if(ipopt_status_ok(red_status)){
						double log10K = log10K_from_n(n_red, nu_co2, 3, P, P0);
						printf("  Kp(red)   = %.3f\n", log10K);
					}else{
						printf("  Kp(red)   = (failed, status %d)\n", red_status);
						ok = 0;
					}
				}
#ifdef HAVE_NLOPT
				{
				double n_slsqp[3];
				int slsqp_status = eqm_slsqp_solve_elements_source(
					co2_names, 3, co2_elements, 2, source_ms, co2_b, co2_tab[i].T, P, n_slsqp);
				if(slsqp_status == 0){
					double log10K = log10K_from_n(n_slsqp, nu_co2, 3, P, P0);
					printf("  Kp(slsqp) = %.3f\n", log10K);
				}else{
					printf("  Kp(slsqp) = (failed, status %d)\n", slsqp_status);
				}
			}
#endif
		}
	}

	{
		/* CO2 + H2 <-> CO + H2O */
		static const char *wgs_names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
		static const char *wgs_elements[] = {"C", "O", "H"};
		static const double wgs_b[] = {1.0, 2.0, 2.0};
			static const double nu_wgs[] = {1.0, 1.0, -1.0, -1.0};
			static const struct{
				double T;
				double log10K;
			} wgs_tab[] = {
				{1000.0, -0.159}
				,{500.0, -2.139}
				,{298.0, -5.018}
			};
			double n_prev[4] = {0.0, 0.0, 0.0, 0.0};
			int have_prev = 0;
			printf("\nCO2 + H2 <-> CO + H2O (source: Moran and Shapiro)\n");
			for(size_t i = 0; i < sizeof(wgs_tab)/sizeof(wgs_tab[0]); ++i){
				double n_out[4];
				const double *n_init = have_prev ? n_prev : NULL;
				int ok_mu0 = 1;
				double log10Kmu = log10K_from_mu0(wgs_names, nu_wgs, 4, source_ms,
					wgs_tab[i].T, P0, &ok_mu0);
			printf("T=%.1f K\n", wgs_tab[i].T);
			printf("  Kp(table) = %.3f\n", wgs_tab[i].log10K);
			if(ok_mu0){
				printf("  Kp(mu0)  = %.3f\n", log10Kmu);
			}else{
				printf("  Kp(mu0)  = (failed)\n");
				continue;
			}
				status = eqm_ipopt_nullspace_solve_source(wgs_names, 4, wgs_elements, 3,
					source_ms, wgs_b, wgs_tab[i].T, P, n_out);
				if(ipopt_status_ok(status)){
					double log10K = log10K_from_n(n_out, nu_wgs, 4, P, P0);
					printf("  Kp(gmin)  = %.3f\n", log10K);
				}else{
					printf("  Kp(gmin)  = (failed, status %d)\n", status);
				}
				status = solve_eqm_api_ipopt(wgs_names, 4, wgs_elements, 3, wgs_b,
					source_ms, wgs_tab[i].T, P, n_init, n_out);
				if(ipopt_status_ok(status)){
					double log10K = log10K_from_n(n_out, nu_wgs, 4, P, P0);
					printf("  Kp(eqm)   = %.3f\n", log10K);
					for(int j = 0; j < 4; ++j){
						n_prev[j] = n_out[j];
					}
					have_prev = 1;
				}else{
					printf("  Kp(eqm)   = (failed, status %d)\n", status);
				}
				{
					double n_red[4];
					int red_status = solve_eqm_api_reduced(wgs_names, 4, wgs_elements, 3, wgs_b,
						source_ms, wgs_tab[i].T, P, n_init, n_red);
					if(ipopt_status_ok(red_status)){
						double log10K = log10K_from_n(n_red, nu_wgs, 4, P, P0);
						printf("  Kp(red)   = %.3f\n", log10K);
					}else{
						printf("  Kp(red)   = (failed, status %d)\n", red_status);
						ok = 0;
					}
				}
#ifdef HAVE_NLOPT
				{
				double n_slsqp[4];
				int slsqp_status = eqm_slsqp_solve_elements_source(
					wgs_names, 4, wgs_elements, 3, source_ms, wgs_b, wgs_tab[i].T, P, n_slsqp);
				if(slsqp_status == 0){
					double log10K = log10K_from_n(n_slsqp, nu_wgs, 4, P, P0);
					printf("  Kp(slsqp) = %.3f\n", log10K);
				}else{
					printf("  Kp(slsqp) = (failed, status %d)\n", slsqp_status);
				}
			}
#endif
		}
	}

	{
		/* Check linear dependence: CO2<->CO+0.5O2 minus CO2+H2<->CO+H2O gives H2O<->H2+0.5O2. */
		static const struct{
			double T;
			double log10K_co2;
			double log10K_wgs;
		} dep_tab[] = {
			{298.0, -45.066, -5.018}
			,{500.0, -25.050, -2.139}
			,{1000.0, -10.221, -0.159}
		};
		printf("\nLinear dependence check (table values)\n");
		for(size_t i = 0; i < sizeof(dep_tab)/sizeof(dep_tab[0]); ++i){
			double log10K_water = 0.0;
			double log10K_derived = dep_tab[i].log10K_co2 - dep_tab[i].log10K_wgs;
			if(log10K_water_table(dep_tab[i].T, &log10K_water)){
				printf("T=%.1f K: log10K(H2O)=%.3f, derived=%.3f\n",
					dep_tab[i].T, log10K_water, log10K_derived);
			}else{
				printf("T=%.1f K: derived log10K(H2O)=%.3f\n",
					dep_tab[i].T, log10K_derived);
			}
		}
	}

	{
		/* Null-space test with CO, CO2, H2O, H2, O2. */
		static const char *mix_names[] = {"carbonmonoxide", "carbondioxide", "water", "hydrogen", "oxygen"};
		static const char *mix_elements[] = {"C", "O", "H"};
			static const double mix_b[] = {1.0, 2.0, 2.0};
			static const double nu_co2[] = {1.0, -1.0, 0.0, 0.0, 0.5};
			static const double nu_wgs[] = {1.0, -1.0, 1.0, -1.0, 0.0};
			static const double mix_T[] = {1000.0, 500.0, 298.0};
			double n_prev[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
			int have_prev = 0;
			printf("\nCO/CO2/H2O/H2/O2 null-space test (source: Moran and Shapiro)\n");
			for(size_t i = 0; i < 3; ++i){
				double T = mix_T[i];
				double n_out[5];
				const double *n_init = have_prev ? n_prev : NULL;
				int ok_mu0 = 1;
				double log10Kmu_co2 = log10K_from_mu0(mix_names, nu_co2, 5, source_ms, T, P0, &ok_mu0);
				double log10Kmu_wgs = 0.0;
			if(ok_mu0){
				log10Kmu_wgs = log10K_from_mu0(mix_names, nu_wgs, 5, source_ms, T, P0, &ok_mu0);
			}
			printf("T=%.1f K\n", T);
			if(ok_mu0){
				printf("  Kp(mu0) CO2  = %.3f\n", log10Kmu_co2);
				printf("  Kp(mu0) WGS  = %.3f\n", log10Kmu_wgs);
			}else{
				printf("  Kp(mu0) = (failed)\n");
				continue;
			}
				status = eqm_ipopt_nullspace_solve_source(mix_names, 5, mix_elements, 3,
					source_ms, mix_b, T, P, n_out);
				if(ipopt_status_ok(status)){
					double log10K_co2 = log10K_from_n(n_out, nu_co2, 5, P, P0);
					double log10K_wgs = log10K_from_n(n_out, nu_wgs, 5, P, P0);
					printf("  Kp(eq) CO2   = %.3f\n", log10K_co2);
					printf("  Kp(eq) WGS   = %.3f\n", log10K_wgs);
				}else{
					printf("  eqm solve failed (status %d)\n", status);
				}
				status = solve_eqm_api_ipopt(mix_names, 5, mix_elements, 3, mix_b,
					source_ms, T, P, n_init, n_out);
				if(ipopt_status_ok(status)){
					double log10K_co2 = log10K_from_n(n_out, nu_co2, 5, P, P0);
					double log10K_wgs = log10K_from_n(n_out, nu_wgs, 5, P, P0);
					printf("  Kp(eqm) CO2  = %.3f\n", log10K_co2);
					printf("  Kp(eqm) WGS  = %.3f\n", log10K_wgs);
					for(int j = 0; j < 5; ++j){
						n_prev[j] = n_out[j];
					}
					have_prev = 1;
				}else{
					printf("  eqm API solve failed (status %d)\n", status);
				}
				{
					double n_red[5];
					int red_status = solve_eqm_api_reduced(mix_names, 5, mix_elements, 3, mix_b,
						source_ms, T, P, n_init, n_red);
					if(ipopt_status_ok(red_status)){
						double log10K_co2 = log10K_from_n(n_red, nu_co2, 5, P, P0);
						double log10K_wgs = log10K_from_n(n_red, nu_wgs, 5, P, P0);
						printf("  Kp(red) CO2  = %.3f\n", log10K_co2);
						printf("  Kp(red) WGS  = %.3f\n", log10K_wgs);
					}else{
						printf("  reduced solve failed (status %d)\n", red_status);
					}
				}
#ifdef HAVE_NLOPT
				{
				double n_slsqp[5];
				int slsqp_status = eqm_slsqp_solve_elements_source(
					mix_names, 5, mix_elements, 3, source_ms, mix_b, T, P, n_slsqp);
				if(slsqp_status == 0){
					double log10K_co2 = log10K_from_n(n_slsqp, nu_co2, 5, P, P0);
					double log10K_wgs = log10K_from_n(n_slsqp, nu_wgs, 5, P, P0);
					printf("  Kp(slsqp) CO2 = %.3f\n", log10K_co2);
					printf("  Kp(slsqp) WGS = %.3f\n", log10K_wgs);
				}else{
					printf("  slsqp solve failed (status %d)\n", slsqp_status);
				}
			}
#endif
		}
	}

	if(!run_wgs_invariance_checks(source_ms, P0)){
		ok = 0;
	}

	if(!ok){
		return 1;
	}
	return 0;
}
