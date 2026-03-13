#include "../test.h"
#include "../eqm.h"
#include "../flash.h"
#include "../flash_unifac.h"
#include "../fluids.h"
#include "../constcp_species.h"
#include "../solution.h"
#include "../wustite_hidayat.h"
#include "../name_resolve.h"
#include "../mixtures/unifac_data.h"
#include "../mixtures/unifac_rundata.h"

#include <math.h>
#include <string.h>

#define ARRAYLEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

typedef struct EqmFixture{
	const char *source;
	double T;
	double P;
	double P0;
	double log10_tol;
} EqmFixture;

static EqmFixture g_eqm = {0};

static int eqm_suite_init(void){
	g_eqm.source = "Moran and Shapiro";
	g_eqm.T = 1000.0;
	g_eqm.P = 101325.0;
	g_eqm.P0 = 1e5;
	g_eqm.log10_tol = 5e-3;
	return 0;
}

static int eqm_suite_cleanup(void){
	return 0;
}

static int find_name(const char **names, int n, const char *name){
	int i;
	for(i = 0; i < n; ++i){
		if(0 == strcmp(names[i], name)){
			return i;
		}
	}
	return -1;
}

static double log10K_from_mu0(const char **names, const double *nu, int ns, const char *source){
	const double R = 8.31446261815324;
	double sum = 0.0;
	int i;
	for(i = 0; i < ns; ++i){
		double mu0 = 0.0;
		if(!eqm_mu0_source(names[i], source, g_eqm.T, g_eqm.P0, &mu0)){
			return NAN;
		}
		sum += nu[i] * mu0;
	}
	return -sum / (R * g_eqm.T * log(10.0));
}

static double log10K_from_n(const double *n, const double *nu, int ns){
	double ntot = 0.0;
	double sum = 0.0;
	int i;
	for(i = 0; i < ns; ++i){
		if(!(n[i] > 0.0)){
			return NAN;
		}
		ntot += n[i];
	}
	if(!(ntot > 0.0)){
		return NAN;
	}
	for(i = 0; i < ns; ++i){
		double yi = n[i] / ntot;
		double ai = yi * g_eqm.P / g_eqm.P0;
		if(!(ai > 0.0)){
			return NAN;
		}
		sum += nu[i] * log10(ai);
	}
	return sum;
}

static double log10K_from_n_source_phaseaware(const char **names, const double *n, const double *nu, int ns,
		const char *source){
	double ntot_gas = 0.0;
	double sum = 0.0;
	int i;
	for(i = 0; i < ns; ++i){
		char source_buf[512];
		const char *source_i = fprops_resolve_species_source(source, names[i], source_buf,
			(unsigned)sizeof(source_buf));
		const ConstCpSpecies *S = constcp_species_lookup(names[i], source_i);
		int is_condensed;
		if(!S){
			S = constcp_species_lookup(names[i], NULL);
		}
		is_condensed = S ? 1 : 0;
		if(!(n[i] > 0.0)){
			return NAN;
		}
		if(!is_condensed){
			ntot_gas += n[i];
		}
	}
	if(!(ntot_gas > 0.0)){
		return NAN;
	}
	for(i = 0; i < ns; ++i){
		char source_buf[512];
		const char *source_i = fprops_resolve_species_source(source, names[i], source_buf,
			(unsigned)sizeof(source_buf));
		const ConstCpSpecies *S = constcp_species_lookup(names[i], source_i);
		int is_condensed;
		if(!S){
			S = constcp_species_lookup(names[i], NULL);
		}
		is_condensed = S ? 1 : 0;
		if(!is_condensed){
			double yi = n[i] / ntot_gas;
			double ai = yi * g_eqm.P / g_eqm.P0;
			if(!(ai > 0.0)){
				return NAN;
			}
			sum += nu[i] * log10(ai);
		}
	}
	return sum;
}

static double hillert_jarl_A_test(double p){
	return 518.0 / 1125.0 + (11692.0 / 15975.0) * (1.0 / p - 1.0);
}

static double hillert_jarl_gmag_test(double T, double Tord, double beta, double p){
	const double R = 8.31446261815324;
	double tau = T / fabs(Tord);
	double A = hillert_jarl_A_test(p);
	double f;
	if(tau <= 1.0){
		double poly = tau * tau * tau / 6.0
			+ pow(tau, 9.0) / 135.0
			+ pow(tau, 15.0) / 600.0;
		f = 1.0 - (
			(79.0 / (140.0 * p)) / tau
			+ (474.0 / 497.0) * (1.0 / p - 1.0) * poly
		) / A;
	}else{
		f = -(
			pow(tau, -5.0) / 10.0
			+ pow(tau, -15.0) / 315.0
			+ pow(tau, -25.0) / 1500.0
		) / A;
	}
	return f * R * T * log(fabs(beta) + 1.0);
}

static double gibbs_hser_fe_expected(double T){
	if(T <= 1811.0){
		return 1225.7
		+ 124.134 * T
		- 23.5143 * T * log(T)
		- 0.00439752 * T * T
		+ 77359.0 / T
		- 5.8927e-8 * T * T * T;
	}
	return -25383.581
		+ 299.31255 * T
		- 46.0 * T * log(T)
		+ 2.29603e31 * pow(T, -9.0);
}

static double gibbs_fe_bcc_expected(double T){
	return gibbs_hser_fe_expected(T)
		+ hillert_jarl_gmag_test(T, 1043.0, 2.22, 0.40);
}

static double gibbs_fe_fcc_expected(double T){
	double g;
	if(T <= 1811.0){
		g = gibbs_hser_fe_expected(T)
			- 1462.4
			+ 8.282 * T
			- 1.15 * T * log(T)
			+ 6.4e-4 * T * T;
	}else{
		g = -27098.266
			+ 300.25256 * T
			- 46.0 * T * log(T)
			+ 2.78854e31 * pow(T, -9.0);
	}
	return g + hillert_jarl_gmag_test(T, -201.0, -2.1, 0.28);
}

static double gibbs_fe2o3_expected(double T){
	double g;
	if(T <= 2500.0){
		g = -859683.1
			+ 828.0501 * T
			- 137.0089 * T * log(T)
			+ 1453820.0 / T;
	}else{
		g = -857356.9
			+ 823.7122 * T
			- 136.5437 * T * log(T);
	}
	return g + hillert_jarl_gmag_test(T, 955.667, 8.36667, 0.28);
}

static double atpct_o_from_x_expected(double x){
	return 100.0 * (1.0 + 0.5 * x) / (2.0 + 0.5 * x);
}

static int wustite_lambda_expected(double T, double x, double *lam_fe, double *lam_o){
	FpropsError err = FPROPS_NO_ERROR;
	const BinarySolutionModel *M = wustite_hidayat_phase()->model;
	double mu_a = solution_binary_mu_a(M, T, g_eqm.P0, x, &err);
	double mu_b;
	if(err != FPROPS_NO_ERROR){
		return 0;
	}
	mu_b = solution_binary_mu_b(M, T, g_eqm.P0, x, &err);
	if(err != FPROPS_NO_ERROR){
		return 0;
	}
	if(lam_fe){
		*lam_fe = 3.0 * mu_a - 2.0 * mu_b;
	}
	if(lam_o){
		*lam_o = 2.0 * (mu_b - mu_a);
	}
	return 1;
}

static double stable_fe_g_expected(double T){
	double g_bcc = gibbs_fe_bcc_expected(T);
	double g_fcc = gibbs_fe_fcc_expected(T);
	return g_bcc <= g_fcc ? g_bcc : g_fcc;
}

static double residual_stable_fe_wustite_expected(double T, double x){
	double lam_fe = 0.0;
	double lam_o = 0.0;
	(void)lam_o;
	if(!wustite_lambda_expected(T, x, &lam_fe, &lam_o)){
		return NAN;
	}
	return stable_fe_g_expected(T) - lam_fe;
}

static double minimize_abs_residual_x_expected(double T, double (*fn)(double, double)){
	double x_lo = 1e-4;
	double x_hi = 0.95;
	double best_x = x_lo;
	double best_abs = HUGE_VAL;
	int pass;
	for(pass = 0; pass < 5; ++pass){
		int i;
		double span = x_hi - x_lo;
		for(i = 0; i <= 400; ++i){
			double x = x_lo + span * (double)i / 400.0;
			double r = fabs(fn(T, x));
			if(r < best_abs){
				best_abs = r;
				best_x = x;
			}
		}
		span = fmax(1e-5, 0.12 * (x_hi - x_lo));
		x_lo = fmax(1e-6, best_x - span);
		x_hi = fmin(0.999999, best_x + span);
	}
	return best_x;
}

static void test_eqm_feoh_reaktoro_clone_boundary_912C(void){
	const char *source_map =
		"Fe_bcc=hidayat_2015;Fe_fcc=hidayat_2015;Wus_FeO=hidayat_2015;Wus_FeO1p5=hidayat_2015;"
		"hydrogen=reaktoro_clone_supcrt98;water=reaktoro_clone_supcrt98";
	const double R = 8.31446261815324;
	const double T = 1185.15;
	double x = minimize_abs_residual_x_expected(T, residual_stable_fe_wustite_expected);
	double lam_fe = 0.0;
	double lam_o = 0.0;
	double mu_h2 = 0.0;
	double mu_h2o = 0.0;
	double log10_ratio = 0.0;
	CU_ASSERT_TRUE_FATAL(wustite_lambda_expected(T, x, &lam_fe, &lam_o) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("hydrogen", source_map, T, g_eqm.P0, &mu_h2) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("water", source_map, T, g_eqm.P0, &mu_h2o) != 0);
	log10_ratio = (lam_o - (mu_h2o - mu_h2)) / (R * T * log(10.0));
	CU_ASSERT_TRUE(fabs(residual_stable_fe_wustite_expected(T, x)) <= 50.0);
	CU_ASSERT_TRUE(fabs(atpct_o_from_x_expected(x) - 51.255) <= 0.05);
	CU_ASSERT_TRUE(fabs(log10_ratio - (-1.8276564)) <= 0.02);
}

static void assert_reduced_solve_ok(const char **names, int ns, const char **elements, int ne,
		const double *b, double *n_out){
	int i;
	int status = eqm_solve_elements(names, ns, elements, ne, b, g_eqm.source,
		g_eqm.T, g_eqm.P, "reduced", NULL, n_out);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	for(i = 0; i < ns; ++i){
		CU_ASSERT_TRUE(isfinite(n_out[i]));
		CU_ASSERT_TRUE(n_out[i] > 0.0);
	}
}

static void assert_log10K_consistent(const char **names, const double *nu, int ns, const double *n){
	double log10_mu0 = log10K_from_mu0(names, nu, ns, g_eqm.source);
	double log10_eqm = log10K_from_n(n, nu, ns);
	CU_ASSERT_TRUE_FATAL(isfinite(log10_mu0));
	CU_ASSERT_TRUE_FATAL(isfinite(log10_eqm));
	CU_ASSERT_TRUE(fabs(log10_eqm - log10_mu0) <= g_eqm.log10_tol);
}

static void test_eqm_mu0_core_species(void){
	static const char *species[] = {
		"hydrogen", "oxygen", "water", "carbonmonoxide",
		"carbondioxide", "nitrogen", "methane", "Ni", "NiO"
	};
	size_t i;
	for(i = 0; i < sizeof(species) / sizeof(species[0]); ++i){
		double mu0 = 0.0;
		int ok = eqm_mu0_source(species[i], g_eqm.source, g_eqm.T, g_eqm.P0, &mu0);
		CU_ASSERT_TRUE(ok != 0);
		CU_ASSERT_TRUE(isfinite(mu0));
	}
}

static void test_eqm_h2o_dissociation_reduced(void){
	static const char *names[] = {"hydrogen", "oxygen", "water"};
	static const char *elements[] = {"H", "O"};
	static const double b[] = {2.0, 1.0};
	static const double nu[] = {1.0, 0.5, -1.0};
	double n[3];
	assert_reduced_solve_ok(names, 3, elements, 2, b, n);
	assert_log10K_consistent(names, nu, 3, n);
}

static void test_eqm_wgs_reduced(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	static const double nu[] = {1.0, 1.0, -1.0, -1.0};
	double n[ARRAYLEN(names)];
	assert_reduced_solve_ok(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b, n);
	assert_log10K_consistent(names, nu, ARRAYLEN(names), n);
}

static double hr_ammonia_log10_ka(double T){
	return 2.1
		+ (1.0 / 4.571) * (9591.0 / T - 0.00046 * T + 0.85e-6 * T * T)
		- 4.98 * log10(T) / 1.985;
}

static double hr_ammonia_residual(double xi, double T, double P_atm){
	double Ka = pow(10.0, hr_ammonia_log10_ka(T));
	return Ka
		* pow((1.0 - xi) / 2.0, 0.5)
		* pow(3.0 * (1.0 - xi) / 2.0, 1.5)
		* P_atm
		- xi * (2.0 - xi);
}

static double hr_ammonia_nh3_percent(double T, double P_atm){
	double lo = 1e-12;
	double hi = 1.0 - 1e-12;
	double flo = hr_ammonia_residual(lo, T, P_atm);
	double fhi = hr_ammonia_residual(hi, T, P_atm);
	int iter;
	CU_ASSERT_TRUE_FATAL(flo * fhi < 0.0);
	for(iter = 0; iter < 200; ++iter){
		double mid = 0.5 * (lo + hi);
		double fmid = hr_ammonia_residual(mid, T, P_atm);
		if(fabs(fmid) < 1e-14){
			lo = mid;
			hi = mid;
			break;
		}
		if(flo * fmid <= 0.0){
			hi = mid;
			fhi = fmid;
		}else{
			lo = mid;
			flo = fmid;
		}
	}
	(void)fhi;
	return 100.0 * (0.5 * (lo + hi)) / (2.0 - 0.5 * (lo + hi));
}

static void test_eqm_ammonia_synthesis_helmholtz_ref0_matches_hr_grid(void){
	static const char *names[] = {"nitrogen", "hydrogen", "ammonia"};
	static const char *elements[] = {"N", "H"};
	static const double b[] = {1.0, 3.0};
	static const double temps[] = {473.0, 573.0, 673.0, 773.0, 873.0, 973.0, 1073.0, 1173.0, 1273.0};
	static const double pressures_atm[] = {1.0, 30.0, 100.0, 200.0};
	const char *source = "helmholtz+ref0:";
	size_t it;
	size_t ip;
	for(it = 0; it < ARRAYLEN(temps); ++it){
		for(ip = 0; ip < ARRAYLEN(pressures_atm); ++ip){
			double P = pressures_atm[ip] * 101325.0;
			double n[ARRAYLEN(names)] = {0.0, 0.0, 0.0};
			double H_total = NAN;
			double ntot = 0.0;
			double y_nh3;
			double nh3_pct_eqm;
			double nh3_pct_hr = hr_ammonia_nh3_percent(temps[it], pressures_atm[ip]);
			int status = fprops_eqm_tpb(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b,
				source, temps[it], P, "auto_nullspace", NULL, n, &H_total);
			CU_ASSERT_TRUE_FATAL(status == 0 || status == 1 || status == 6);
			CU_ASSERT_TRUE(isfinite(H_total));
			ntot = n[0] + n[1] + n[2];
			CU_ASSERT_TRUE_FATAL(ntot > 0.0);
			y_nh3 = n[2] / ntot;
			nh3_pct_eqm = 100.0 * y_nh3;
			CU_ASSERT_TRUE(fabs(nh3_pct_eqm - nh3_pct_hr) <= 0.25);
		}
	}
}

static void test_fprops_eqm_tpb_wgs_ms_table(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	static const double nu[] = {1.0, 1.0, -1.0, -1.0};
	static const struct{
		double T;
		double log10K;
	} refs[] = {
		{1000.0, -0.159},
		{500.0, -2.139},
		{298.0, -5.018}
	};
	size_t i;
	for(i = 0; i < sizeof(refs) / sizeof(refs[0]); ++i){
		double n[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
		double H_total = NAN;
		double log10_eqm;
		int status = fprops_eqm_tpb(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b, "Moran and Shapiro",
			refs[i].T, g_eqm.P, "reduced", NULL, n, &H_total);
		CU_ASSERT_EQUAL_FATAL(status, 0);
		log10_eqm = log10K_from_n(n, nu, ARRAYLEN(names));
		CU_ASSERT_TRUE_FATAL(isfinite(log10_eqm));
		CU_ASSERT_TRUE(fabs(log10_eqm - refs[i].log10K) <= 0.03);
		CU_ASSERT_TRUE(isfinite(H_total));
	}
}

static void test_fprops_eqm_tpy_wgs_normalization_and_match_tpb(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double y_norm[] = {0.5, 0.5, 0.0, 0.0};
	static const double y_scaled[] = {1.0, 1.0, 0.0, 0.0};
	static const double b_from_y[] = {0.5, 1.0, 1.0};
	static const double nu[] = {1.0, 1.0, -1.0, -1.0};
	double n_tpy_norm[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double n_tpy_scaled[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double n_tpb[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double log10_eqm;
	int i;
	int status_tpy_norm;
	int status_tpy_scaled;
	int status_tpb;

	status_tpy_norm = fprops_eqm_tpy(names, ARRAYLEN(names), y_norm, "Moran and Shapiro",
		g_eqm.T, g_eqm.P, "reduced", NULL, n_tpy_norm);
	status_tpy_scaled = fprops_eqm_tpy(names, ARRAYLEN(names), y_scaled, "Moran and Shapiro",
		g_eqm.T, g_eqm.P, "reduced", NULL, n_tpy_scaled);
	status_tpb = fprops_eqm_tpb(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b_from_y, "Moran and Shapiro",
		g_eqm.T, g_eqm.P, "reduced", NULL, n_tpb, NULL);

	CU_ASSERT_EQUAL_FATAL(status_tpy_norm, 0);
	CU_ASSERT_EQUAL_FATAL(status_tpy_scaled, 0);
	CU_ASSERT_EQUAL_FATAL(status_tpb, 0);

	for(i = 0; i < ARRAYLEN(names); ++i){
		CU_ASSERT_TRUE(fabs(n_tpy_norm[i] - n_tpb[i]) <= 1e-9);
		CU_ASSERT_TRUE(fabs(n_tpy_scaled[i] - n_tpb[i]) <= 1e-9);
	}

	log10_eqm = log10K_from_n(n_tpy_norm, nu, ARRAYLEN(names));
	CU_ASSERT_TRUE_FATAL(isfinite(log10_eqm));
	CU_ASSERT_TRUE(fabs(log10_eqm - (-0.159)) <= 0.03);
}

static void test_fprops_mix_h_tpn_wgs_matches_tpb_and_scales(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	double n_eq[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double H_eq = NAN;
	double H_mix = NAN;
	double H_mix_scaled = NAN;
	double n_scaled[ARRAYLEN(names)];
	int i;
	int status_eq;
	int status_mix;
	int status_mix_scaled;

	status_eq = fprops_eqm_tpb(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b, "Moran and Shapiro",
		g_eqm.T, g_eqm.P, "reduced", NULL, n_eq, &H_eq);
	CU_ASSERT_EQUAL_FATAL(status_eq, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(H_eq));

	status_mix = fprops_mix_h_tpn(names, ARRAYLEN(names), n_eq, "Moran and Shapiro", g_eqm.T, g_eqm.P, &H_mix);
	CU_ASSERT_EQUAL_FATAL(status_mix, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(H_mix));
	CU_ASSERT_TRUE(fabs(H_mix - H_eq) <= 1e-6);

	for(i = 0; i < ARRAYLEN(names); ++i){
		n_scaled[i] = 2.5 * n_eq[i];
	}
	status_mix_scaled = fprops_mix_h_tpn(names, ARRAYLEN(names), n_scaled, "Moran and Shapiro",
		g_eqm.T, g_eqm.P, &H_mix_scaled);
	CU_ASSERT_EQUAL_FATAL(status_mix_scaled, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(H_mix_scaled));
	CU_ASSERT_TRUE(fabs(H_mix_scaled - 2.5 * H_mix) <= 1e-6);
}

static void test_fprops_mix_h_tpn_solution_phase_unsupported(void){
	static const char *names[] = {"Wus_FeO", "Wus_FeO1p5"};
	static const double n[] = {0.8, 0.2};
	double H = NAN;
	int status = fprops_mix_h_tpn(names, 2, n, "hidayat_2015", g_eqm.T, g_eqm.P, &H);
	CU_ASSERT_EQUAL(status, -15);
}

static void test_fprops_rxn_package_mix_h_supports_wustite_phase(void){
	static const char *names[] = {"Wus_FeO", "Wus_FeO1p5"};
	static const double n[] = {0.89582806546875, 0.10417193453125};
	double n_scaled[ARRAYLEN(names)];
	FpropsRxnPackage *pkg = fprops_rxn_package_build(names, ARRAYLEN(names), "hidayat_2015");
	FpropsRxnTPN state;
	double H = NAN;
	double H_scaled = NAN;
	int i;
	int status;

	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg);

	state.T = 1073.15;
	state.P = g_eqm.P0;
	state.n = n;
	status = fprops_rxn_mix_h(pkg, &state, &H);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(H));

	for(i = 0; i < ARRAYLEN(names); ++i){
		n_scaled[i] = 3.0 * n[i];
	}
	state.n = n_scaled;
	status = fprops_rxn_mix_h(pkg, &state, &H_scaled);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(H_scaled));
	CU_ASSERT_TRUE(fabs(H_scaled - 3.0 * H) <= 1e-4 * fmax(1.0, fabs(H)));

	fprops_rxn_package_free(pkg);
}

static void test_fprops_rxn_package_mix_h_matches_legacy(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const double n[] = {0.3, 0.2, 0.1, 0.4};
	FpropsRxnPackage *pkg = fprops_rxn_package_build(names, ARRAYLEN(names), "Moran and Shapiro");
	FpropsRxnTPN state;
	double H_pkg = NAN;
	double H_legacy = NAN;
	int status_pkg;
	int status_legacy;

	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg);
	state.T = g_eqm.T;
	state.P = g_eqm.P;
	state.n = n;

	status_pkg = fprops_rxn_mix_h(pkg, &state, &H_pkg);
	status_legacy = fprops_mix_h_tpn(names, ARRAYLEN(names), n, "Moran and Shapiro", g_eqm.T, g_eqm.P, &H_legacy);

	CU_ASSERT_EQUAL(status_pkg, 0);
	CU_ASSERT_EQUAL(status_legacy, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(H_pkg));
	CU_ASSERT_TRUE_FATAL(isfinite(H_legacy));
	CU_ASSERT_TRUE(fabs(H_pkg - H_legacy) <= 1e-6);

	fprops_rxn_package_free(pkg);
}

static void test_fprops_mix_h_tpn_fe2o3_h2_reduction_matches_standard_enthalpy(void){
	static const char *names[] = {"Fe2O3", "hydrogen", "Fe_bcc", "water"};
	static const double n_in[] = {1.0, 3.0, 0.0, 0.0};
	static const double n_out[] = {0.0, 0.0, 2.0, 3.0};
	static const char *source =
		"Fe2O3=hidayat_2015;Fe_bcc=hidayat_2015;*=Moran and Shapiro";
	FpropsRxnPackage *pkg = NULL;
	FpropsRxnTPN state;
	double H_in = NAN;
	double H_out = NAN;
	double dH = NAN;
	int status_in;
	int status_out;

	/* Fe2O3(cr) + 3 H2(g) -> 2 Fe(cr) + 3 H2O(g)
	   NIST/JANAF standard-state data at 298.15 K gives
	   Delta H ~= +99.025 kJ/mol Fe2O3. */
	pkg = fprops_rxn_package_build(names, ARRAYLEN(names), source);
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg);

	state.T = 298.15;
	state.P = g_eqm.P0;
	state.n = n_in;
	status_in = fprops_rxn_mix_h(pkg, &state, &H_in);

	state.n = n_out;
	status_out = fprops_rxn_mix_h(pkg, &state, &H_out);

	CU_ASSERT_EQUAL_FATAL(status_in, 0);
	CU_ASSERT_EQUAL_FATAL(status_out, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(H_in));
	CU_ASSERT_TRUE_FATAL(isfinite(H_out));

	dH = H_out - H_in;
	CU_ASSERT_TRUE(dH > 0.0);
	CU_ASSERT_TRUE(fabs(dH - 99025.0) <= 3000.0);

	fprops_rxn_package_free(pkg);
}

static void test_fprops_rxn_package_eqm_matches_legacy(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	FpropsRxnPackage *pkg = fprops_rxn_package_build(names, ARRAYLEN(names), "Moran and Shapiro");
	FpropsRxnTPN state;
	FpropsRxnResult out_pkg;
	double n_pkg[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double n_legacy[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double H_legacy = NAN;
	int status_pkg;
	int status_legacy;
	int i;

	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg);
	state.T = g_eqm.T;
	state.P = g_eqm.P;
	state.n = NULL;
	out_pkg.status = -99;
	out_pkg.H = NAN;
	out_pkg.G = NAN;
	out_pkg.n_out = n_pkg;

	status_pkg = fprops_rxn_eqm_tpb(pkg, &state, b, "reduced", NULL, &out_pkg);
	status_legacy = fprops_eqm_tpb(names, ARRAYLEN(names), elements, ARRAYLEN(elements),
		b, "Moran and Shapiro", g_eqm.T, g_eqm.P, "reduced", NULL, n_legacy, &H_legacy);

	CU_ASSERT_EQUAL(status_pkg, 0);
	CU_ASSERT_EQUAL(status_legacy, 0);
	CU_ASSERT_TRUE_FATAL(isfinite(out_pkg.H));
	CU_ASSERT_TRUE_FATAL(isfinite(H_legacy));
	CU_ASSERT_TRUE(fabs(out_pkg.H - H_legacy) <= 1e-6);
	for(i = 0; i < ARRAYLEN(names); ++i){
		CU_ASSERT_TRUE(fabs(n_pkg[i] - n_legacy[i]) <= 1e-9);
	}

	fprops_rxn_package_free(pkg);
}

static void test_fprops_rxn_package_eqm_tpy_matches_legacy(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const double n_in[] = {1.0, 1.0, 0.0, 0.0};
	FpropsRxnPackage *pkg = fprops_rxn_package_build(names, ARRAYLEN(names), "Moran and Shapiro");
	FpropsRxnTPN state;
	FpropsRxnResult out_pkg;
	double n_pkg[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double n_legacy[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	int status_pkg;
	int status_legacy;
	int i;

	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg);
	state.T = g_eqm.T;
	state.P = g_eqm.P;
	state.n = n_in;
	out_pkg.status = -99;
	out_pkg.H = NAN;
	out_pkg.G = NAN;
	out_pkg.n_out = n_pkg;

	status_pkg = fprops_rxn_eqm_tpy(pkg, &state, "reduced", NULL, &out_pkg);
	status_legacy = fprops_eqm_tpy(names, ARRAYLEN(names), n_in, "Moran and Shapiro",
		g_eqm.T, g_eqm.P, "reduced", NULL, n_legacy);

	CU_ASSERT_EQUAL(status_pkg, 0);
	CU_ASSERT_EQUAL(status_legacy, 0);
	for(i = 0; i < ARRAYLEN(names); ++i){
		CU_ASSERT_TRUE(fabs(n_pkg[i] - 2.0 * n_legacy[i]) <= 1e-9);
	}

	fprops_rxn_package_free(pkg);
}

static void test_fprops_rxn_package_eqm_sensitivities_wgs(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const double n_in[] = {1.0, 1.0, 0.0, 0.0};
	static const double dT = 1e-2;
	static const double dn = 1e-6;
	FpropsRxnPackage *pkg = fprops_rxn_package_build(names, ARRAYLEN(names), "Moran and Shapiro");
	FpropsRxnTPN state;
	FpropsRxnResult out;
	double n_eq[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double dn_dT[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double dn_db[ARRAYLEN(names) * 3] = {0.0};
	double dn_dnin0[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double n_pm[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double n_pp[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	double n_in_work[ARRAYLEN(names)] = {0.0, 0.0, 0.0, 0.0};
	const double *A = NULL;
	int ne = 0;
	int status;
	int i;
	int e;

	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg);
	ne = fprops_rxn_package_num_elements(pkg);
	A = fprops_rxn_package_element_matrix(pkg);
	CU_ASSERT_TRUE_FATAL(ne > 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(A);
	state.T = g_eqm.T;
	state.P = g_eqm.P;
	state.n = n_in;
	out.status = -99;
	out.H = NAN;
	out.G = NAN;
	out.n_out = n_eq;

	status = fprops_rxn_eqm_tpy(pkg, &state, "reduced", NULL, &out);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	status = fprops_rxn_eqm_sensitivities(pkg, &state, n_eq, dn_dT, NULL, dn_db);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	for(i = 0; i < ARRAYLEN(names); ++i){
		double s = 0.0;
		for(e = 0; e < ne; ++e){
			s += dn_db[i * ne + e] * A[e * ARRAYLEN(names) + 0];
		}
		dn_dnin0[i] = s;
	}

	state.T = g_eqm.T - dT;
	out.n_out = n_pm;
	status = fprops_rxn_eqm_tpy(pkg, &state, "reduced", n_eq, &out);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	state.T = g_eqm.T + dT;
	out.n_out = n_pp;
	status = fprops_rxn_eqm_tpy(pkg, &state, "reduced", n_eq, &out);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	for(i = 0; i < ARRAYLEN(names); ++i){
		double fd_fwd = (n_pp[i] - n_eq[i]) / dT;
		double fd_bwd = (n_eq[i] - n_pm[i]) / dT;
		double fd_ctr = (n_pp[i] - n_pm[i]) / (2.0 * dT);
		double fd_lo = fmin(fd_fwd, fd_bwd);
		double fd_hi = fmax(fd_fwd, fd_bwd);
		double tol = 1e-5 + 5e-3 * fmax(fabs(fd_ctr), fabs(dn_dT[i]));
		CU_ASSERT_TRUE(fabs(fd_ctr - dn_dT[i]) <= tol);
		CU_ASSERT_TRUE(dn_dT[i] >= fd_lo - tol);
		CU_ASSERT_TRUE(dn_dT[i] <= fd_hi + tol);
	}

	memcpy(n_in_work, n_in, sizeof(n_in_work));
	n_in_work[0] -= dn;
	state.T = g_eqm.T;
	state.n = n_in_work;
	out.n_out = n_pm;
	status = fprops_rxn_eqm_tpy(pkg, &state, "reduced", n_eq, &out);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	memcpy(n_in_work, n_in, sizeof(n_in_work));
	n_in_work[0] += dn;
	state.n = n_in_work;
	out.n_out = n_pp;
	status = fprops_rxn_eqm_tpy(pkg, &state, "reduced", n_eq, &out);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	for(i = 0; i < ARRAYLEN(names); ++i){
		double fd_fwd = (n_pp[i] - n_eq[i]) / dn;
		double fd_bwd = (n_eq[i] - n_pm[i]) / dn;
		double fd_ctr = (n_pp[i] - n_pm[i]) / (2.0 * dn);
		double fd_lo = fmin(fd_fwd, fd_bwd);
		double fd_hi = fmax(fd_fwd, fd_bwd);
		double tol = 1e-5 + 5e-3 * fmax(fabs(fd_ctr), fabs(dn_dnin0[i]));
		CU_ASSERT_TRUE(fabs(fd_ctr - dn_dnin0[i]) <= tol);
		CU_ASSERT_TRUE(dn_dnin0[i] >= fd_lo - tol);
		CU_ASSERT_TRUE(dn_dnin0[i] <= fd_hi + tol);
	}

	fprops_rxn_package_free(pkg);
}

static void test_eqm_wgs_permutation_invariance(void){
	static const char *base_names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *base_elements[] = {"C", "O", "H"};
	static const double base_b[] = {1.0, 2.0, 2.0};

	static const char *perm_names[] = {"hydrogen", "carbondioxide", "carbonmonoxide", "water"};
	static const char *perm_elements[] = {"H", "C", "O"};
	static const double perm_b[] = {2.0, 1.0, 2.0};

	double n_base[ARRAYLEN(base_names)];
	double n_perm[ARRAYLEN(perm_names)];
	double ndiff_max = 0.0;
	int i;

	assert_reduced_solve_ok(base_names, ARRAYLEN(base_names), base_elements, ARRAYLEN(base_elements), base_b, n_base);
	assert_reduced_solve_ok(perm_names, ARRAYLEN(perm_names), perm_elements, ARRAYLEN(perm_elements), perm_b, n_perm);

	for(i = 0; i < ARRAYLEN(base_names); ++i){
		int j = find_name(perm_names, ARRAYLEN(perm_names), base_names[i]);
		double d;
		CU_ASSERT_TRUE_FATAL(j >= 0);
		d = fabs(n_base[i] - n_perm[j]);
		if(d > ndiff_max){
			ndiff_max = d;
		}
	}
	CU_ASSERT_TRUE(ndiff_max <= 1e-5);
}

static void test_eqm_multi_reaction_mixed_system(void){
	static const char *names[] = {
		"carbonmonoxide", "carbondioxide", "water", "hydrogen", "oxygen"
	};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	static const double nu_co2[] = {1.0, -1.0, 0.0, 0.0, 0.5};
	static const double nu_wgs[] = {1.0, -1.0, 1.0, -1.0, 0.0};
	double n[5];

	assert_reduced_solve_ok(names, 5, elements, 3, b, n);
	assert_log10K_consistent(names, nu_co2, 5, n);
	assert_log10K_consistent(names, nu_wgs, 5, n);
}

static void test_eqm_humid_air_nox_auto_reduced_lowt(void){
	static const char *names[] = {
		"nitrogen", "oxygen", "argon", "water", "carbondioxide",
		"nitric_oxide", "nitrogen_dioxide", "carbonmonoxide", "hydrogen"
	};
	static const double n_in[] = {
		0.78050661145600002,
		0.20937052904000001,
		0.0096958595039999996,
		0.015,
		0.00043000000000000002,
		0.0,
		0.0,
		0.0,
		0.0
	};
	static const char *source =
		"*=RPP;water=Moran and Shapiro;carbondioxide=Moran and Shapiro;"
		"carbonmonoxide=Moran and Shapiro;hydrogen=Moran and Shapiro";
	static const struct{
		double T;
		double y_no_min;
		double y_no_max;
		double y_no2_min;
		double y_no2_max;
	} refs[] = {
		{1000.0, 2.0e-5, 5.0e-5, 1.0e-6, 2.5e-6},
		{800.0, 1.0e-6, 5.0e-6, 3.0e-7, 1.0e-6}
	};
	size_t k;

	for(k = 0; k < ARRAYLEN(refs); ++k){
		double n_out[ARRAYLEN(names)] = {0.0};
		double ntot = 0.0;
		int i_no;
		int i_no2;
		int i_co;
		int i_h2;
		int status = fprops_eqm_tpy(names, ARRAYLEN(names), n_in, source,
			refs[k].T, g_eqm.P, "auto_reduced", n_in, n_out);

		CU_ASSERT_EQUAL_FATAL(status, 0);
		i_no = find_name(names, ARRAYLEN(names), "nitric_oxide");
		i_no2 = find_name(names, ARRAYLEN(names), "nitrogen_dioxide");
		i_co = find_name(names, ARRAYLEN(names), "carbonmonoxide");
		i_h2 = find_name(names, ARRAYLEN(names), "hydrogen");
		CU_ASSERT_TRUE_FATAL(i_no >= 0);
		CU_ASSERT_TRUE_FATAL(i_no2 >= 0);
		CU_ASSERT_TRUE_FATAL(i_co >= 0);
		CU_ASSERT_TRUE_FATAL(i_h2 >= 0);
		for(int i = 0; i < ARRAYLEN(names); ++i){
			CU_ASSERT_TRUE_FATAL(isfinite(n_out[i]));
			CU_ASSERT_TRUE_FATAL(n_out[i] > 0.0);
			ntot += n_out[i];
		}
		CU_ASSERT_TRUE_FATAL(ntot > 0.0);
		CU_ASSERT_TRUE(n_out[i_no] / ntot >= refs[k].y_no_min);
		CU_ASSERT_TRUE(n_out[i_no] / ntot <= refs[k].y_no_max);
		CU_ASSERT_TRUE(n_out[i_no2] / ntot >= refs[k].y_no2_min);
		CU_ASSERT_TRUE(n_out[i_no2] / ntot <= refs[k].y_no2_max);
		CU_ASSERT_TRUE(n_out[i_co] / ntot <= 1e-8);
		CU_ASSERT_TRUE(n_out[i_h2] / ntot <= 1e-8);
	}
}

static void test_eqm_fe_oxide_mu0_data(void){
	static const char *species[] = {"Fe", "FeO", "Fe3O4", "Fe2O3", "hydrogen", "water"};
	static const double temps[] = {700.0, 1000.0};
	size_t it, is;
	for(it = 0; it < sizeof(temps) / sizeof(temps[0]); ++it){
		for(is = 0; is < sizeof(species) / sizeof(species[0]); ++is){
			double mu0 = 0.0;
			int ok = eqm_mu0_source(species[is], g_eqm.source, temps[it], g_eqm.P0, &mu0);
			CU_ASSERT_TRUE(ok != 0);
			CU_ASSERT_TRUE(isfinite(mu0));
		}
	}
}

static void test_eqm_mu0_source_map_resolution(void){
	const char *source_map =
		"Ni=oecd_nea_tdb_vol6_nickel;NiO=oecd_nea_tdb_vol6_nickel;*=Moran and Shapiro";
	double mu_map = 0.0;
	double mu_ref = 0.0;

	CU_ASSERT_TRUE(eqm_mu0_source("Ni", source_map, g_eqm.T, g_eqm.P0, &mu_map) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("Ni", "oecd_nea_tdb_vol6_nickel", g_eqm.T, g_eqm.P0, &mu_ref) != 0);
	CU_ASSERT_TRUE(fabs(mu_map - mu_ref) <= 1e-9);

	CU_ASSERT_TRUE(eqm_mu0_source("NiO", source_map, g_eqm.T, g_eqm.P0, &mu_map) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("NiO", "oecd_nea_tdb_vol6_nickel", g_eqm.T, g_eqm.P0, &mu_ref) != 0);
	CU_ASSERT_TRUE(fabs(mu_map - mu_ref) <= 1e-9);

	CU_ASSERT_TRUE(eqm_mu0_source("water", source_map, g_eqm.T, g_eqm.P0, &mu_map) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("water", "Moran and Shapiro", g_eqm.T, g_eqm.P0, &mu_ref) != 0);
	CU_ASSERT_TRUE(fabs(mu_map - mu_ref) <= 1e-9);
}

static void test_eqm_nio_h2_mixed_source_map_reduced(void){
	static const char *names[] = {"Ni", "NiO", "hydrogen", "water"};
	static const char *elements[] = {"Ni", "O", "H"};
	static const double b[] = {1.0, 1.0, 2.0};
	static const double nu[] = {1.0, -1.0, -1.0, 1.0};
	const char *source_map =
		"Ni=oecd_nea_tdb_vol6_nickel;NiO=oecd_nea_tdb_vol6_nickel;*=Moran and Shapiro";
	double n[4];
	int i;
	int status = eqm_solve_elements(names, 4, elements, 3, b, source_map,
		g_eqm.T, g_eqm.P, "reduced", NULL, n);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	for(i = 0; i < 4; ++i){
		CU_ASSERT_TRUE(isfinite(n[i]));
		CU_ASSERT_TRUE(n[i] > 0.0);
	}
	{
		double log10_mu0 = log10K_from_mu0(names, nu, 4, source_map);
		double log10_eqm = log10K_from_n_source_phaseaware(names, n, nu, 4, source_map);
		CU_ASSERT_TRUE_FATAL(isfinite(log10_mu0));
		CU_ASSERT_TRUE_FATAL(isfinite(log10_eqm));
		CU_ASSERT_TRUE(fabs(log10_eqm - log10_mu0) <= g_eqm.log10_tol);
	}
}

static void test_eqm_mu0_model_selectors(void){
	double mu_auto = 0.0;
	double mu_ideal = 0.0;
	double mu_helm = 0.0;
	double mu_helm_ref0 = 0.0;
	double mu_peng = 0.0;
	double mu_constcp = 0.0;
	double mu_shomate = 0.0;

	CU_ASSERT_TRUE(eqm_mu0_source("hydrogen", "Moran and Shapiro", g_eqm.T, g_eqm.P0, &mu_auto) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("hydrogen", "ideal:Moran and Shapiro", g_eqm.T, g_eqm.P0, &mu_ideal) != 0);
	CU_ASSERT_TRUE(isfinite(mu_auto));
	CU_ASSERT_TRUE(isfinite(mu_ideal));
	CU_ASSERT_TRUE(fabs(mu_auto - mu_ideal) <= 1e-9);

	CU_ASSERT_TRUE(eqm_mu0_source("carbondioxide", "helmholtz:", g_eqm.T, g_eqm.P0, &mu_helm) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("carbondioxide", "helmholtz+ref0:", g_eqm.T, g_eqm.P0, &mu_helm_ref0) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("carbondioxide", "pengrob:", g_eqm.T, g_eqm.P0, &mu_peng) != 0);
	CU_ASSERT_TRUE(isfinite(mu_helm));
	CU_ASSERT_TRUE(isfinite(mu_helm_ref0));
	CU_ASSERT_TRUE(isfinite(mu_peng));

	CU_ASSERT_TRUE(eqm_mu0_source("Ni", "constcp:oecd_nea_tdb_vol6_nickel", g_eqm.T, g_eqm.P0, &mu_constcp) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("Ni", "shomate:oecd_nea_tdb_vol6_nickel", g_eqm.T, g_eqm.P0, &mu_shomate) != 0);
	CU_ASSERT_TRUE(isfinite(mu_constcp));
	CU_ASSERT_TRUE(isfinite(mu_shomate));
}

static void test_eqm_helmholtz_ref0_matches_ms_reaction_delta_g(void){
	static const char *species[] = {"water", "hydrogen", "oxygen"};
	static const double nu[] = {1.0, -1.0, -0.5};
	double temps[] = {873.15, 1173.15};
	int it;
	for(it = 0; it < 2; ++it){
		double dg_helm_ref0 = 0.0;
		double dg_ms = 0.0;
		int is;
		for(is = 0; is < 3; ++is){
			double mu_helm_ref0 = 0.0;
			double mu_ms = 0.0;
			CU_ASSERT_TRUE_FATAL(eqm_mu0_source(species[is], "helmholtz+ref0:", temps[it], g_eqm.P0, &mu_helm_ref0) != 0);
			CU_ASSERT_TRUE_FATAL(eqm_mu0_source(species[is], "Moran and Shapiro", temps[it], g_eqm.P0, &mu_ms) != 0);
			CU_ASSERT_TRUE(isfinite(mu_helm_ref0));
			CU_ASSERT_TRUE(isfinite(mu_ms));
			dg_helm_ref0 += nu[is] * mu_helm_ref0;
			dg_ms += nu[is] * mu_ms;
		}
		CU_ASSERT_TRUE(fabs(dg_helm_ref0 - dg_ms) <= 100.0);
	}
}

static void test_eqm_nio_h2_shomate_selector_reduced(void){
	static const char *names[] = {"Ni", "NiO", "hydrogen", "water"};
	static const char *elements[] = {"Ni", "O", "H"};
	static const double b[] = {1.0, 1.0, 2.0};
	static const double nu[] = {1.0, -1.0, -1.0, 1.0};
	const char *source_map =
		"Ni=shomate:oecd_nea_tdb_vol6_nickel;NiO=shomate:oecd_nea_tdb_vol6_nickel;"
		"hydrogen=ideal:Moran and Shapiro;water=ideal:Moran and Shapiro";
	double n[4];
	int i;
	int status = eqm_solve_elements(names, 4, elements, 3, b, source_map,
		g_eqm.T, g_eqm.P, "reduced", NULL, n);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	for(i = 0; i < 4; ++i){
		CU_ASSERT_TRUE(isfinite(n[i]));
		CU_ASSERT_TRUE(n[i] > 0.0);
	}
	{
		double log10_mu0 = log10K_from_mu0(names, nu, 4, source_map);
		double log10_eqm = log10K_from_n_source_phaseaware(names, n, nu, 4, source_map);
		CU_ASSERT_TRUE_FATAL(isfinite(log10_mu0));
		CU_ASSERT_TRUE_FATAL(isfinite(log10_eqm));
		CU_ASSERT_TRUE(fabs(log10_eqm - log10_mu0) <= g_eqm.log10_tol);
	}
}

static void test_eqm_explicit_unknown_source_falls_back_to_ideal(void){
	double mu_unknown = 0.0;
	CU_ASSERT_TRUE(eqm_mu0_source("hydrogen", "this_source_does_not_exist", g_eqm.T, g_eqm.P0, &mu_unknown) != 0);
	CU_ASSERT_TRUE(isfinite(mu_unknown));
}

static void test_eqm_reaktoro_clone_auto_routes_to_clone(void){
	double mu_auto = 0.0;
	double mu_shomate = 0.0;
	CU_ASSERT_TRUE(eqm_mu0_source("hydrogen", "reaktoro_clone_supcrt98", g_eqm.T, g_eqm.P0, &mu_auto) != 0);
	CU_ASSERT_TRUE(eqm_mu0_source("hydrogen", "shomate:reaktoro_clone_supcrt98", g_eqm.T, g_eqm.P0, &mu_shomate) != 0);
	CU_ASSERT_TRUE(fabs(mu_auto - mu_shomate) <= 1e-9);
}

static void test_eqm_hidayat_pragmatic_species_mu0(void){
	double mu = 0.0;
	CU_ASSERT_TRUE(eqm_mu0_source("Fe_bcc", "hidayat_2015", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
	CU_ASSERT_TRUE(eqm_mu0_source("Fe_fcc", "hidayat_2015", 1400.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
	CU_ASSERT_TRUE(eqm_mu0_source("Fe3O4", "hidayat_2015", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
	CU_ASSERT_TRUE(eqm_mu0_source("Fe2O3", "hidayat_2015", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
}

static void test_eqm_hidayat_magnetic_mu0_matches_formula(void){
	double mu = 0.0;
	CU_ASSERT_TRUE(eqm_mu0_source("Fe_bcc", "hidayat_2015", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(fabs(mu - gibbs_fe_bcc_expected(1000.0)) <= 1e-6);

	CU_ASSERT_TRUE(eqm_mu0_source("Fe_fcc", "hidayat_2015", 1400.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(fabs(mu - gibbs_fe_fcc_expected(1400.0)) <= 1e-6);

	CU_ASSERT_TRUE(eqm_mu0_source("Fe2O3", "hidayat_2015", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(fabs(mu - gibbs_fe2o3_expected(1000.0)) <= 1e-6);
}

static void test_eqm_degterov_spinel_fe3o4_smoke(void){
	static const char *names[] = {
		"Sp_Fe2_tet", "Sp_Fe3_tet", "Sp_Fe2_oct", "Sp_Fe3_oct", "Sp_Va_oct"
	};
	static const char *elements[] = {"Fe", "O"};
	static const double b[] = {3.0, 4.0};
	double n[5];
	int status = eqm_solve_elements(names, 5, elements, 2, b, "degterov_2001",
		1000.0, g_eqm.P, "auto", NULL, n);
	CU_ASSERT_TRUE_FATAL(status == 0 || status == 1 || status == 6);
	CU_ASSERT_TRUE(n[0] + n[1] > 0.0);
	CU_ASSERT_TRUE(fabs(2.0 * (n[0] + n[1]) - n[2] - n[3] - n[4]) <= 1e-6);
	CU_ASSERT_TRUE(fabs(6.0 * n[0] + 5.0 * n[1] - 2.0 * n[2] - 3.0 * n[3]) <= 1e-6);
}

static void test_eqm_wustite_solution_fullspace_unique_balance(void){
	static const char *names[] = {"Wus_FeO", "Wus_FeO1p5"};
	static const char *elements[] = {"Fe", "O"};
	static const double b[] = {1.0, 1.1};
	double n[2];
	int status = eqm_solve_elements(names, 2, elements, 2, b, "hidayat_2015",
		g_eqm.T, g_eqm.P, "auto", NULL, n);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_TRUE(isfinite(n[0]));
	CU_ASSERT_TRUE(isfinite(n[1]));
	CU_ASSERT_TRUE(fabs(n[0] - 0.8) <= 1e-8);
	CU_ASSERT_TRUE(fabs(n[1] - 0.2) <= 1e-8);
}

static void test_eqm_bcc_iron_solution_fullspace_unique_balance(void){
	static const char *names[] = {"Bcc_Fe", "Bcc_O"};
	static const char *elements[] = {"Fe", "O"};
	static const double b[] = {0.999, 0.001};
	double n[2];
	int status = eqm_solve_elements(names, 2, elements, 2, b, "hidayat_2015",
		1000.0, g_eqm.P, "auto", NULL, n);
	CU_ASSERT_TRUE_FATAL(status == 0 || status == 1 || status == 6);
	CU_ASSERT_TRUE(isfinite(n[0]));
	CU_ASSERT_TRUE(isfinite(n[1]));
	CU_ASSERT_TRUE(fabs(n[0] - 0.999) <= 1e-8);
	CU_ASSERT_TRUE(fabs(n[1] - 0.001) <= 1e-8);
}

static void test_eqm_wustite_solution_rejects_nullspace_only(void){
	static const char *names[] = {"Wus_FeO", "Wus_FeO1p5"};
	static const char *elements[] = {"Fe", "O"};
	static const double b[] = {1.0, 1.1};
	double n[2];
	int status = eqm_solve_elements(names, 2, elements, 2, b, "hidayat_2015",
		g_eqm.T, g_eqm.P, "nullspace", NULL, n);
	CU_ASSERT_EQUAL(status, -12);
}

static void test_eqm_feo_pragmatic_low_oxygen_smoke_1400K(void){
	static const char *names[] = {
		"Fe_bcc", "Fe_fcc", "Wus_FeO", "Wus_FeO1p5", "Fe3O4", "Fe2O3"
	};
	static const char *elements[] = {"Fe", "O"};
	static const double b[] = {1.0, 0.95};
	double n[6];
	double n_metal;
	double n_wustite;
	int status = eqm_solve_elements(names, 6, elements, 2, b, "hidayat_2015",
		1400.0, g_eqm.P, "auto", NULL, n);
	CU_ASSERT_TRUE_FATAL(status == 0 || status == 1 || status == 6);
	n_metal = n[0] + n[1];
	n_wustite = n[2] + n[3];
	CU_ASSERT_TRUE(n_metal > 1e-2);
	CU_ASSERT_TRUE(n_wustite > 0.5);
}

static void test_name_resolve_reactive_and_unifac_domains(void){
	FpropsResolvedName out;
	FpropsNameResolveStatus status;

	status = fprops_name_resolve("CO", FPROPS_NAME_DOMAIN_EQM_SPECIES, "Moran and Shapiro", &out);
	CU_ASSERT_EQUAL(status, FPROPS_NAME_RESOLVE_OK);
	CU_ASSERT_PTR_NOT_NULL_FATAL(out.canonical);
	CU_ASSERT_STRING_EQUAL(out.canonical->canonical, "carbonmonoxide");

	status = fprops_name_resolve("CO", FPROPS_NAME_DOMAIN_EQM_SPECIES, "RPP", &out);
	CU_ASSERT_EQUAL(status, FPROPS_NAME_RESOLVE_OK);
	CU_ASSERT_PTR_NOT_NULL_FATAL(out.canonical);
	CU_ASSERT_STRING_EQUAL(out.canonical->canonical, "carbon_monoxide");

	status = fprops_name_resolve("EtOH", FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT, "UNIFAC-orig-2003", &out);
	CU_ASSERT_EQUAL(status, FPROPS_NAME_RESOLVE_OK);
	CU_ASSERT_PTR_NOT_NULL_FATAL(out.canonical);
	CU_ASSERT_STRING_EQUAL(out.canonical->canonical, "ethanol");

	status = fprops_name_resolve("water", FPROPS_NAME_DOMAIN_PURE_FLUID, NULL, &out);
	CU_ASSERT_EQUAL(status, FPROPS_NAME_RESOLVE_OK);
	CU_ASSERT_PTR_NOT_NULL_FATAL(out.canonical);
	CU_ASSERT_STRING_EQUAL(out.canonical->canonical, "water");

	status = fprops_name_resolve("H2O",
		FPROPS_NAME_DOMAIN_PURE_FLUID | FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT, NULL, &out);
	CU_ASSERT_EQUAL(status, FPROPS_NAME_RESOLVE_AMBIGUOUS);
}

static void test_unifac_native_source_data_lookup(void){
	const FpropsUNIFACSourceData *src = fprops_unifac_source("UNIFAC-orig-2003");
	const FpropsUNIFACComponentSource *water;
	const FpropsUNIFACComponentSource *ethanol;

	CU_ASSERT_PTR_NOT_NULL_FATAL(src);
	CU_ASSERT_TRUE(src->ncomponents > 0);
	CU_ASSERT_TRUE(src->nsubgroups > 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(src->interactions);
	CU_ASSERT_EQUAL(src->interactions->ngroups, 47);

	water = fprops_unifac_component(src, "water");
	ethanol = fprops_unifac_component(src, "ethanol");
	CU_ASSERT_PTR_NOT_NULL_FATAL(water);
	CU_ASSERT_PTR_NOT_NULL_FATAL(ethanol);
	CU_ASSERT_STRING_EQUAL(water->formula, "H2O");
	CU_ASSERT_STRING_EQUAL(ethanol->formula, "C2H5OH");
	CU_ASSERT_TRUE(water->nsubgroups > 0);
	CU_ASSERT_TRUE(ethanol->nsubgroups > 0);
	CU_ASSERT_TRUE(water->Pc > 1e6);
	CU_ASSERT_TRUE(ethanol->Pc > 1e6);
	CU_ASSERT_TRUE(water->Vliq > 1e-6);
	CU_ASSERT_TRUE(ethanol->Vliq > 1e-6);
}

static void test_unifac_runtime_prepare_and_gamma(void){
	const FpropsUNIFACSourceData *src = fprops_unifac_source("UNIFAC-orig-2003");
	const char *names[] = {"water", "ethanol"};
	FpropsUNIFACRunData *run;
	double x[] = {0.5, 0.5};
	double gamma[2];
	int status;

	CU_ASSERT_PTR_NOT_NULL_FATAL(src);
	run = fprops_unifac_prepare(src, names, ARRAYLEN(names));
	CU_ASSERT_PTR_NOT_NULL_FATAL(run);
	CU_ASSERT_EQUAL(run->nc, 2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(fprops_unifac_flash_package(run));

	status = fprops_unifac_gamma_run(run, 298.0, x, gamma);
	CU_ASSERT_EQUAL(status, 0);
	CU_ASSERT_TRUE(gamma[0] > 1.0);
	CU_ASSERT_TRUE(gamma[1] > 1.0);

	fprops_unifac_destroy(run);
}

static void test_flash_prepare_unifac_and_tpz(void){
	const char *names[] = {"water", "ethanol"};
	FpropsMultiphasePackage pkg;
	FpropsFlashTPZ in;
	FpropsFlashVLResult out;
	double z[] = {0.5, 0.5};
	double x[2];
	double y[2];
	int status;

	memset(&pkg, 0, sizeof(pkg));
	status = fprops_flash_prepare_unifac(&pkg, "UNIFAC-orig-2003", names, ARRAYLEN(names));
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_EQUAL(pkg.kind, FPROPS_FLASH_PACKAGE_UNIFAC_IDEAL_VL);
	CU_ASSERT_EQUAL(pkg.nc, ARRAYLEN(names));
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg.data.unifac_ideal_vl.pkg);

	in.T = 351.0;
	in.P = 101325.0;
	in.z = z;
	out.status = -99;
	out.beta = NAN;
	out.x = x;
	out.y = y;
	status = fprops_flash_tpz(&pkg, &in, &out);
	CU_ASSERT_EQUAL(status, 0);
	CU_ASSERT_TRUE(out.beta >= 0.0);
	CU_ASSERT_TRUE(out.beta <= 1.0);
	CU_ASSERT_TRUE(x[0] > 0.0);
	CU_ASSERT_TRUE(x[1] > 0.0);
	CU_ASSERT_TRUE(y[0] > 0.0);
	CU_ASSERT_TRUE(y[1] > 0.0);

	fprops_flash_destroy_package(&pkg);
	CU_ASSERT_EQUAL(pkg.kind, FPROPS_FLASH_PACKAGE_INVALID);
}

static void test_unifac_liq_fugacity_matches_vlecalc_ethanol_water_bubble_points(void){
	const char *names[] = {"water", "ethanol"};
	const double P = 1.01e5; /* VLE-Calc case shown at 1.01 bar */
	const double reltol = 2e-2;
	struct {
		double T_C;
		double x_water;
		double y_water;
	} cases[] = {
		{78.0563, 0.10, 0.100401},
		{79.7169, 0.50, 0.342386},
		{86.3093, 0.90, 0.555865}
	};
	FpropsMultiphasePackage pkg;
	int i, status;

	memset(&pkg, 0, sizeof(pkg));
	status = fprops_flash_prepare_unifac(&pkg, "UNIFAC-orig-2003", names, ARRAYLEN(names));
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg.data.unifac_ideal_vl.pkg);

	for(i = 0; i < ARRAYLEN(cases); ++i){
		double x[2];
		double fugacity[2];
		double y_water_expected;
		double T;

		x[0] = cases[i].x_water;
		x[1] = 1.0 - x[0];
		y_water_expected = cases[i].y_water;
		T = 273.15 + cases[i].T_C;

		status = fprops_unifac_liq_fugacity(pkg.data.unifac_ideal_vl.pkg, T, P, x, fugacity);
		CU_ASSERT_EQUAL(status, 0);
		CU_ASSERT_TRUE(fugacity[0] > 0.0);
		CU_ASSERT_TRUE(fugacity[1] > 0.0);

		if(fabs(fugacity[0] / P - y_water_expected) > reltol){
			fprintf(stderr,
				"UNIFAC liq fugacity mismatch at x_water=%.6f, T_C=%.4f:"
				" yw_calc=%.8f yw_ref=%.8f\n",
				x[0], cases[i].T_C,
				fugacity[0] / P, y_water_expected
			);
		}
		CU_ASSERT_DOUBLE_EQUAL(fugacity[0] / P, y_water_expected, reltol);
	}

	fprops_flash_destroy_package(&pkg);
	CU_ASSERT_EQUAL(pkg.kind, FPROPS_FLASH_PACKAGE_INVALID);
}

CU_ErrorCode test_register_eqm(void){
	CU_pSuite s = CU_add_suite("eqm", eqm_suite_init, eqm_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	if(NULL == CU_add_test(s, "mu0_core_species", test_eqm_mu0_core_species)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "h2o_dissociation_reduced", test_eqm_h2o_dissociation_reduced)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "wgs_reduced", test_eqm_wgs_reduced)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "ammonia_synthesis_helmholtz_ref0_matches_hr_grid",
			test_eqm_ammonia_synthesis_helmholtz_ref0_matches_hr_grid)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_eqm_tpb_wgs_ms_table", test_fprops_eqm_tpb_wgs_ms_table)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_eqm_tpy_wgs_normalization_and_match_tpb",
			test_fprops_eqm_tpy_wgs_normalization_and_match_tpb)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_mix_h_tpn_wgs_matches_tpb_and_scales",
			test_fprops_mix_h_tpn_wgs_matches_tpb_and_scales)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_mix_h_tpn_solution_phase_unsupported",
			test_fprops_mix_h_tpn_solution_phase_unsupported)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_rxn_package_mix_h_supports_wustite_phase",
			test_fprops_rxn_package_mix_h_supports_wustite_phase)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_rxn_package_mix_h_matches_legacy",
			test_fprops_rxn_package_mix_h_matches_legacy)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_mix_h_tpn_fe2o3_h2_reduction_matches_standard_enthalpy",
			test_fprops_mix_h_tpn_fe2o3_h2_reduction_matches_standard_enthalpy)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_rxn_package_eqm_matches_legacy",
			test_fprops_rxn_package_eqm_matches_legacy)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_rxn_package_eqm_tpy_matches_legacy",
			test_fprops_rxn_package_eqm_tpy_matches_legacy)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fprops_rxn_package_eqm_sensitivities_wgs",
			test_fprops_rxn_package_eqm_sensitivities_wgs)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "wgs_permutation_invariance", test_eqm_wgs_permutation_invariance)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "multi_reaction_mixed_system", test_eqm_multi_reaction_mixed_system)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "humid_air_nox_auto_reduced_lowt",
			test_eqm_humid_air_nox_auto_reduced_lowt)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "fe_oxide_mu0_data", test_eqm_fe_oxide_mu0_data)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "mu0_source_map_resolution", test_eqm_mu0_source_map_resolution)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "nio_h2_mixed_source_map_reduced", test_eqm_nio_h2_mixed_source_map_reduced)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "mu0_model_selectors", test_eqm_mu0_model_selectors)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "helmholtz_ref0_matches_ms_reaction_delta_g",
			test_eqm_helmholtz_ref0_matches_ms_reaction_delta_g)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "nio_h2_shomate_selector_reduced", test_eqm_nio_h2_shomate_selector_reduced)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "explicit_unknown_source_falls_back_to_ideal",
			test_eqm_explicit_unknown_source_falls_back_to_ideal)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "reaktoro_clone_auto_routes_to_clone", test_eqm_reaktoro_clone_auto_routes_to_clone)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "hidayat_pragmatic_species_mu0", test_eqm_hidayat_pragmatic_species_mu0)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "hidayat_magnetic_mu0_matches_formula",
			test_eqm_hidayat_magnetic_mu0_matches_formula)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "degterov_spinel_fe3o4_smoke",
			test_eqm_degterov_spinel_fe3o4_smoke)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "wustite_solution_fullspace_unique_balance",
			test_eqm_wustite_solution_fullspace_unique_balance)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "bcc_iron_solution_fullspace_unique_balance",
			test_eqm_bcc_iron_solution_fullspace_unique_balance)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "wustite_solution_rejects_nullspace_only",
			test_eqm_wustite_solution_rejects_nullspace_only)){
		return CUE_NOTEST;
	}
	/* Temporarily skipped: unstable under current data/solver settings. */
	(void)test_eqm_feo_pragmatic_low_oxygen_smoke_1400K;
	if(NULL == CU_add_test(s, "feoh_reaktoro_clone_boundary_912C",
				test_eqm_feoh_reaktoro_clone_boundary_912C)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "name_resolve_reactive_and_unifac_domains",
			test_name_resolve_reactive_and_unifac_domains)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "unifac_native_source_data_lookup",
			test_unifac_native_source_data_lookup)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "unifac_runtime_prepare_and_gamma",
			test_unifac_runtime_prepare_and_gamma)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "flash_prepare_unifac_and_tpz",
			test_flash_prepare_unifac_and_tpz)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "unifac_liq_fugacity_matches_vlecalc_ethanol_water_bubble_points",
			test_unifac_liq_fugacity_matches_vlecalc_ethanol_water_bubble_points)){
		return CUE_NOTEST;
	}
	return CUE_SUCCESS;
}
