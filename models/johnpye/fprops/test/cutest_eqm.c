#include "../test.h"
#include "../eqm.h"

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

static double log10K_from_mu0(const char **names, const double *nu, int ns){
	const double R = 8.31446261815324;
	double sum = 0.0;
	int i;
	for(i = 0; i < ns; ++i){
		double mu0 = 0.0;
		if(!eqm_mu0_source(names[i], g_eqm.source, g_eqm.T, g_eqm.P0, &mu0)){
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
	double log10_mu0 = log10K_from_mu0(names, nu, ns);
	double log10_eqm = log10K_from_n(n, nu, ns);
	CU_ASSERT_TRUE_FATAL(isfinite(log10_mu0));
	CU_ASSERT_TRUE_FATAL(isfinite(log10_eqm));
	CU_ASSERT_TRUE(fabs(log10_eqm - log10_mu0) <= g_eqm.log10_tol);
}

static void test_eqm_mu0_core_species(void){
	static const char *species[] = {
		"hydrogen", "oxygen", "water", "carbonmonoxide",
		"carbondioxide", "nitrogen", "methane"
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
	return CUE_SUCCESS;
}
