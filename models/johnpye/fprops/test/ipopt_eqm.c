#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "eqm.h"
#include "eqm_ipopt.h"
#ifdef HAVE_NLOPT
#include "eqm_slsqp.h"
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
			const double nu_h2o[] = {1.0, 0.5, -1.0};
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
			status = eqm_ipopt_solve_elements_source_init(spec_names, 3, elements, 2,
				source_ms, b, ktab[i].T, P, n_init, n_out);
			if(!(status == 0 || status == 1 || status == 6)){
				fprintf(stderr, "IPOPT solve failed at T=%.1f (status %d)\n", ktab[i].T, status);
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
			{298.0, -45.066}
			,{500.0, -25.050}
			,{1000.0, -10.221}
		};
		printf("\nCO2 <-> CO + 0.5 O2 (source: Moran and Shapiro)\n");
		for(size_t i = 0; i < sizeof(co2_tab)/sizeof(co2_tab[0]); ++i){
			double n_out[3];
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
			if(status == 0 || status == 1 || status == 6){
				double log10K = log10K_from_n(n_out, nu_co2, 3, P, P0);
				printf("  Kp(gmin)  = %.3f\n", log10K);
			}else{
				printf("  Kp(gmin)  = (failed, status %d)\n", status);
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
			{298.0, -5.018}
			,{500.0, -2.139}
			,{1000.0, -0.159}
		};
		printf("\nCO2 + H2 <-> CO + H2O (source: Moran and Shapiro)\n");
		for(size_t i = 0; i < sizeof(wgs_tab)/sizeof(wgs_tab[0]); ++i){
			double n_out[4];
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
			if(status == 0 || status == 1 || status == 6){
				double log10K = log10K_from_n(n_out, nu_wgs, 4, P, P0);
				printf("  Kp(gmin)  = %.3f\n", log10K);
			}else{
				printf("  Kp(gmin)  = (failed, status %d)\n", status);
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
		printf("\nCO/CO2/H2O/H2/O2 null-space test (source: Moran and Shapiro)\n");
		for(size_t i = 0; i < 3; ++i){
			double T = (i == 0) ? 298.0 : (i == 1 ? 500.0 : 1000.0);
			double n_out[5];
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
			if(status == 0 || status == 1 || status == 6){
				double log10K_co2 = log10K_from_n(n_out, nu_co2, 5, P, P0);
				double log10K_wgs = log10K_from_n(n_out, nu_wgs, 5, P, P0);
				printf("  Kp(eq) CO2   = %.3f\n", log10K_co2);
				printf("  Kp(eq) WGS   = %.3f\n", log10K_wgs);
			}else{
				printf("  eqm solve failed (status %d)\n", status);
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

	if(!ok){
		return 1;
	}
	return 0;
}
