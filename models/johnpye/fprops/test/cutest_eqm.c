#include "../test.h"
#include "../eqm.h"
#include "../fluids.h"
#include "../constcp_species.h"
#include "../solution.h"
#include "../wustite_hidayat.h"

#include <math.h>
#include <string.h>

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
	double n[4];
	assert_reduced_solve_ok(names, 4, elements, 3, b, n);
	assert_log10K_consistent(names, nu, 4, n);
}

static void test_eqm_wgs_permutation_invariance(void){
	static const char *base_names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *base_elements[] = {"C", "O", "H"};
	static const double base_b[] = {1.0, 2.0, 2.0};

	static const char *perm_names[] = {"hydrogen", "carbondioxide", "carbonmonoxide", "water"};
	static const char *perm_elements[] = {"H", "C", "O"};
	static const double perm_b[] = {2.0, 1.0, 2.0};

	double n_base[4];
	double n_perm[4];
	double ndiff_max = 0.0;
	int i;

	assert_reduced_solve_ok(base_names, 4, base_elements, 3, base_b, n_base);
	assert_reduced_solve_ok(perm_names, 4, perm_elements, 3, perm_b, n_perm);

	for(i = 0; i < 4; ++i){
		int j = find_name(perm_names, 4, base_names[i]);
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
	if(NULL == CU_add_test(s, "wgs_permutation_invariance", test_eqm_wgs_permutation_invariance)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "multi_reaction_mixed_system", test_eqm_multi_reaction_mixed_system)){
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
	if(NULL == CU_add_test(s, "feo_pragmatic_low_oxygen_smoke_1400K",
				test_eqm_feo_pragmatic_low_oxygen_smoke_1400K)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "feoh_reaktoro_clone_boundary_912C",
				test_eqm_feoh_reaktoro_clone_boundary_912C)){
		return CUE_NOTEST;
	}
	return CUE_SUCCESS;
}
