#include "../test.h"
#include "../eqm.h"
#include "../eqm_internal.h"
#include "../eqm_phase.h"
#include "../eqm_phase_internal.h"
#include "../flash.h"
#include "../flash_unifac.h"
#include "../fluids.h"
#include "../constcp_species.h"
#include "../gibbs_species.h"
#include "../shomate_data.h"
#include "../shomate_species.h"
#include "../solution.h"
#include "../solution_data.h"
#include "../spinel_data.h"
#include "../wustite_hidayat.h"
#include "../name_resolve.h"
#include "../mixtures/unifac_data.h"
#include "../mixtures/unifac_rundata.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define ARRAYLEN(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define CU_ASSERT_EQM_STATUS_OK_FATAL(STATUS) do{ \
		int _eqm_status = (STATUS); \
		if(!fprops_eqm_status_ok(_eqm_status)){ \
			fprintf(stderr, "unexpected equilibrium status %d (%s) at %s:%d\n", \
				_eqm_status, fprops_eqm_status_text(_eqm_status), __FILE__, __LINE__); \
		} \
		CU_ASSERT_TRUE_FATAL(fprops_eqm_status_ok(_eqm_status)); \
	}while(0)

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

static int find_name(const char *const *names, int n, const char *name){
	int i;
	for(i = 0; i < n; ++i){
		if(0 == strcmp(names[i], name)){
			return i;
		}
	}
	return -1;
}

static double log10K_from_mu0(const char **names, const double *nu, int ns, const char *source){
	double sum = 0.0;
	int i;
	for(i = 0; i < ns; ++i){
		double mu0 = 0.0;
		if(!eqm_mu0_source(names[i], source, g_eqm.T, g_eqm.P0, &mu0)){
			return NAN;
		}
		sum += nu[i] * mu0;
	}
	return -sum / (FPROPS_R * g_eqm.T * log(10.0));
}

static double qfm_oneill_1987_log10fo2_low_branch(double T){
	double mu_o2;
	if(!(T > 900.0) || !(T < 1420.0)){
		return NAN;
	}
	mu_o2 = -587474.0 + 1584.427 * T - 203.3164 * T * log(T) + 0.09271 * T * T;
	return mu_o2 / (FPROPS_R * T * log(10.0));
}

static double qfi_oneill_1987_log10fo2(double T){
	double mu_o2;
	if(!(T > 900.0) || !(T < 1420.0)){
		return NAN;
	}
	if(T < 1042.0){
		mu_o2 = -542941.0 - 33.182 * T + 22.446 * T * log(T);
	}else if(T <= 1184.0){
		mu_o2 = -562377.0 + 103.384 * T + 5.4771 * T * log(T);
	}else{
		mu_o2 = -602739.0 + 369.704 * T - 27.3443 * T * log(T);
	}
	return mu_o2 / (FPROPS_R * T * log(10.0));
}

static double stable_fe_mu0_hidayat(double T){
	double mu_bcc = NAN;
	double mu_fcc = NAN;
	int has_bcc = eqm_mu0_source("Fe_bcc", "hidayat_2015", T, g_eqm.P0, &mu_bcc);
	int has_fcc = eqm_mu0_source("Fe_fcc", "hidayat_2015", T, g_eqm.P0, &mu_fcc);
	if(has_bcc && has_fcc){
		return mu_bcc <= mu_fcc ? mu_bcc : mu_fcc;
	}
	if(has_bcc){
		return mu_bcc;
	}
	if(has_fcc){
		return mu_fcc;
	}
	return NAN;
}

static void test_eqm_status_text_public_api(void){
	FpropsEqmNlpSolver solver;
	CU_ASSERT_TRUE(fprops_eqm_status_ok(0));
	CU_ASSERT_TRUE(fprops_eqm_status_ok(1));
	CU_ASSERT_TRUE(fprops_eqm_status_ok(6));
	CU_ASSERT_TRUE(!fprops_eqm_status_ok(2));
	CU_ASSERT_DOUBLE_EQUAL(FPROPS_R, 8.31446261815324, 1e-15);
	CU_ASSERT_STRING_EQUAL(fprops_eqm_status_text(0), "solved");
	CU_ASSERT_STRING_EQUAL(fprops_eqm_status_text(1), "solved to acceptable level");
	CU_ASSERT_STRING_EQUAL(fprops_eqm_status_text(2), "infeasible problem detected");
	CU_ASSERT_STRING_EQUAL(fprops_eqm_status_text(-22), "equilibrium validation failed");
	CU_ASSERT_STRING_EQUAL(fprops_eqm_status_text(12345), "unknown equilibrium status");
	CU_ASSERT_STRING_EQUAL(fprops_eqm_nlp_solver_name(FPROPS_EQM_NLP_DEFAULT), "auto");
	CU_ASSERT_STRING_EQUAL(fprops_eqm_nlp_solver_name(FPROPS_EQM_NLP_SLSQP), "slsqp");
	CU_ASSERT_STRING_EQUAL(fprops_eqm_nlp_solver_name(FPROPS_EQM_NLP_IPOPT), "ipopt");
	CU_ASSERT_TRUE(fprops_eqm_nlp_solver_from_name("ipopt_scaled_n", &solver));
	CU_ASSERT_EQUAL(solver, FPROPS_EQM_NLP_IPOPT_SCALED_N);
	CU_ASSERT_TRUE(!fprops_eqm_nlp_solver_from_name("not_a_solver", &solver));
}

static void test_eqm_phase_registry_inspection_feoh(void){
	FpropsEqmPhaseModel wustite;
	FpropsEqmPhaseModel spinel;
	FpropsEqmPhaseModel gas;
	FpropsEqmPhaseModel hematite;
	int i_fe;
	int i_o;
	int i_h;

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("phase:wustite=hidayat_2015", NULL, &wustite));
	CU_ASSERT_EQUAL(wustite.kind, FPROPS_EQM_PHASE_BINARY_SOLUTION);
	CU_ASSERT_STRING_EQUAL(wustite.name, "wustite");
	CU_ASSERT_STRING_EQUAL(wustite.source, "hidayat_2015");
	CU_ASSERT_EQUAL(wustite.nmember, 2);
	CU_ASSERT_STRING_EQUAL(wustite.members[0], "Wus_FeO");
	CU_ASSERT_STRING_EQUAL(wustite.members[1], "Wus_FeO1p5");
	CU_ASSERT_EQUAL(wustite.nvar, 1);
	CU_ASSERT_STRING_EQUAL(wustite.var_names[0], "x_FeO1p5");
	i_fe = fprops_eqm_phase_find_element(&wustite, "Fe");
	i_o = fprops_eqm_phase_find_element(&wustite, "O");
	CU_ASSERT_TRUE(i_fe >= 0);
	CU_ASSERT_TRUE(i_o >= 0);

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("phase:spinel=degterov_2001", NULL, &spinel));
	CU_ASSERT_EQUAL(spinel.kind, FPROPS_EQM_PHASE_SITE_SOLUTION);
	CU_ASSERT_STRING_EQUAL(spinel.source, "degterov_2001");
	CU_ASSERT_EQUAL(spinel.nmember, 5);
	CU_ASSERT_STRING_EQUAL(spinel.members[0], "Sp_Fe2_tet");
	CU_ASSERT_STRING_EQUAL(spinel.members[4], "Sp_Va_oct");
	CU_ASSERT_EQUAL(spinel.nvar, 2);
	CU_ASSERT_STRING_EQUAL(spinel.var_names[0], "y_tet_fe2");
	CU_ASSERT_STRING_EQUAL(spinel.var_names[1], "y_oct_fe2");

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &gas));
	CU_ASSERT_EQUAL(gas.kind, FPROPS_EQM_PHASE_IDEAL_GAS);
	CU_ASSERT_EQUAL(gas.nmember, 2);
	CU_ASSERT_STRING_EQUAL(gas.source, "helmholtz+ref0:");
	i_h = fprops_eqm_phase_find_element(&gas, "H");
	i_o = fprops_eqm_phase_find_element(&gas, "O");
	CU_ASSERT_TRUE(i_h >= 0);
	CU_ASSERT_TRUE(i_o >= 0);

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe2O3=hidayat_2015", NULL, &hematite));
	CU_ASSERT_EQUAL(hematite.kind, FPROPS_EQM_PHASE_STOICHIOMETRIC);
	CU_ASSERT_STRING_EQUAL(hematite.name, "Fe2O3");
	CU_ASSERT_STRING_EQUAL(hematite.source, "hidayat_2015");
}

static void test_eqm_phase_package_resolver_feoh(void){
	const char *specs[] = {
		"Fe_bcc=hidayat_2015",
		"wustite=hidayat_2015",
		"spinel=degterov_2001",
		"Fe2O3=hidayat_2015",
		"gas:ideal(hydrogen,water)=helmholtz+ref0:"
	};
	FpropsEqmPhaseModel phases[5];
	int nresolved = fprops_eqm_phase_resolve_package(specs, NULL, ARRAYLEN(specs), phases);
	CU_ASSERT_EQUAL(nresolved, 5);
	CU_ASSERT_STRING_EQUAL(phases[0].name, "Fe_bcc");
	CU_ASSERT_STRING_EQUAL(phases[1].name, "wustite");
	CU_ASSERT_STRING_EQUAL(phases[2].name, "spinel");
	CU_ASSERT_STRING_EQUAL(phases[3].name, "Fe2O3");
	CU_ASSERT_STRING_EQUAL(phases[4].name, "gas:ideal");
	CU_ASSERT_EQUAL(fprops_eqm_phase_total_members(phases, ARRAYLEN(phases)), 11);
}

static void test_eqm_phase_gibbs_and_elements_feoh(void){
	FpropsEqmPhaseModel wustite;
	FpropsEqmPhaseModel spinel;
	FpropsEqmPhaseModel gas;
	double y_wus[1] = {0.1};
	double y_sp[2] = {0.5, 0.2};
	double y_gas[2] = {0.97, 0.03};
	double g;
	double a[4];
	int i_fe;
	int i_o;
	int i_h;

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &wustite));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_gibbs(&wustite, 1173.15, g_eqm.P, y_wus, &g));
	CU_ASSERT_TRUE(isfinite(g));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&wustite, y_wus, a));
	i_fe = fprops_eqm_phase_find_element(&wustite, "Fe");
	i_o = fprops_eqm_phase_find_element(&wustite, "O");
	CU_ASSERT_DOUBLE_EQUAL(a[i_fe], 1.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a[i_o], 1.05, 1e-12);

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel", "degterov_2001", &spinel));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_gibbs(&spinel, 1173.15, g_eqm.P, y_sp, &g));
	CU_ASSERT_TRUE(isfinite(g));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&spinel, y_sp, a));
	i_fe = fprops_eqm_phase_find_element(&spinel, "Fe");
	i_o = fprops_eqm_phase_find_element(&spinel, "O");
	CU_ASSERT_TRUE(a[i_fe] > 2.0 && a[i_fe] < 3.0);
	CU_ASSERT_DOUBLE_EQUAL(a[i_o], 4.0, 1e-12);

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &gas));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_gibbs(&gas, 1173.15, g_eqm.P, y_gas, &g));
	CU_ASSERT_TRUE(isfinite(g));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&gas, y_gas, a));
	i_h = fprops_eqm_phase_find_element(&gas, "H");
	i_o = fprops_eqm_phase_find_element(&gas, "O");
	CU_ASSERT_DOUBLE_EQUAL(a[i_h], 2.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(a[i_o], 0.03, 1e-12);
}

static void test_eqm_phase_entry_fe_wustite_900c_anchor(void){
	const double T = 1173.15;
	const double log10_h2o_h2 = -0.225974;
	const double R = FPROPS_R;
	FpropsEqmPhaseModel wustite;
	double mu_h2;
	double mu_h2o;
	double mu_fe;
	double lam_o_thermo;
	double lambda[2];
	double phi;
	double y_min[1] = {NAN};
	int i_fe;
	int i_o;

	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("Fe_bcc", "hidayat_2015", T, g_eqm.P0, &mu_fe));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &wustite));
	i_fe = fprops_eqm_phase_find_element(&wustite, "Fe");
	i_o = fprops_eqm_phase_find_element(&wustite, "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0);
	CU_ASSERT_TRUE_FATAL(i_o >= 0);

	lam_o_thermo = R * T * log(10.0) * log10_h2o_h2 + (mu_h2o - mu_h2);
	lambda[i_fe] = mu_fe;
	lambda[i_o] = lam_o_thermo;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_entry_residual(&wustite, T, g_eqm.P, lambda, &phi, y_min));
	CU_ASSERT_TRUE(fabs(phi) < 2e-3);
	CU_ASSERT_TRUE(y_min[0] > 0.0 && y_min[0] < 1.0);
}

static void test_eqm_phase_entry_wustite_spinel_900c_anchor(void){
	const double T = 1173.15;
	const double log10_h2o_h2 = 0.665196;
	const double R = FPROPS_R;
	FpropsEqmPhaseModel wustite;
	FpropsEqmPhaseModel spinel;
	double mu_h2;
	double mu_h2o;
	double lam_o_thermo;
	double lambda_wus[2];
	double lambda_sp[2];
	double phi_wus0;
	double phi_sp;
	double y_wus[1] = {NAN};
	double y_sp[2] = {NAN, NAN};
	int i_fe_wus;
	int i_o_wus;
	int i_fe_sp;
	int i_o_sp;

	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &wustite));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel", "degterov_2001", &spinel));

	i_fe_wus = fprops_eqm_phase_find_element(&wustite, "Fe");
	i_o_wus = fprops_eqm_phase_find_element(&wustite, "O");
	i_fe_sp = fprops_eqm_phase_find_element(&spinel, "Fe");
	i_o_sp = fprops_eqm_phase_find_element(&spinel, "O");
	CU_ASSERT_TRUE_FATAL(i_fe_wus >= 0 && i_o_wus >= 0);
	CU_ASSERT_TRUE_FATAL(i_fe_sp >= 0 && i_o_sp >= 0);

	lam_o_thermo = R * T * log(10.0) * log10_h2o_h2 + (mu_h2o - mu_h2);
	lambda_wus[i_fe_wus] = 0.0;
	lambda_wus[i_o_wus] = lam_o_thermo;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_entry_residual(&wustite, T, g_eqm.P,
			lambda_wus, &phi_wus0, y_wus));

	lambda_sp[i_fe_sp] = phi_wus0 * R * T;
	lambda_sp[i_o_sp] = lam_o_thermo;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_entry_residual(&spinel, T, g_eqm.P,
			lambda_sp, &phi_sp, y_sp));
	CU_ASSERT_TRUE(fabs(phi_sp) < 5e-2);
	CU_ASSERT_TRUE(y_wus[0] > 0.0 && y_wus[0] < 1.0);
	CU_ASSERT_TRUE(y_sp[0] >= 0.0 && y_sp[0] <= 1.0);
	CU_ASSERT_TRUE(y_sp[1] >= 0.0 && y_sp[1] <= 1.0);
}

static void test_eqm_phase_fixed_linear_fe_gas_reducing_case(void){
	FpropsEqmPhaseModel phases[2];
	const char *elements[] = {"Fe", "O", "H"};
	double b[] = {2.0, 3.0, 200.0};
	double phase_amounts[2] = {NAN, NAN};
	double member_amounts[3] = {NAN, NAN, NAN};
	int status;

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe_bcc=hidayat_2015", NULL, &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &phases[1]));
	status = fprops_eqm_phase_solve_fixed_linear(phases, 2, elements, ARRAYLEN(elements),
		b, 1173.15, 101325.0, phase_amounts, member_amounts);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[0], 2.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 100.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(member_amounts[0], 2.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(member_amounts[1], 97.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(member_amounts[2], 3.0, 1e-10);
}

static int test_phase_find_lambda_fe_for_entry(const FpropsEqmPhaseModel *phase,
	double T, double P, double lambda_o, double *lambda_fe_out, double *y_out){
	double lambda[2];
	int i_fe;
	int i_o;
	double lo = -1000000.0;
	double hi = 1000000.0;
	double r_lo;
	double r_hi;
	int k;
	i_fe = fprops_eqm_phase_find_element(phase, "Fe");
	i_o = fprops_eqm_phase_find_element(phase, "O");
	if(i_fe < 0 || i_o < 0){
		return 0;
	}
	lambda[i_o] = lambda_o;
	lambda[i_fe] = lo;
	if(!fprops_eqm_phase_entry_residual(phase, T, P, lambda, &r_lo, y_out)){
		return 0;
	}
	lambda[i_fe] = hi;
	if(!fprops_eqm_phase_entry_residual(phase, T, P, lambda, &r_hi, y_out)){
		return 0;
	}
	if(r_lo * r_hi > 0.0){
		return 0;
	}
	for(k = 0; k < 80; ++k){
		double mid = 0.5 * (lo + hi);
		double r_mid;
		lambda[i_fe] = mid;
		if(!fprops_eqm_phase_entry_residual(phase, T, P, lambda, &r_mid, y_out)){
			return 0;
		}
		if(fabs(r_mid) < 1e-8){
			*lambda_fe_out = mid;
			return 1;
		}
		if(r_lo * r_mid <= 0.0){
			hi = mid;
			r_hi = r_mid;
		}else{
			lo = mid;
			r_lo = r_mid;
		}
	}
	*lambda_fe_out = 0.5 * (lo + hi);
	return 1;
}

static double test_log10_from_god(double god){
	return log10(god / (1.0 - god));
}

static double test_interp_log10_god(double t, double god0, double tc0, double god1,
		double tc1){
	double god = god0 + (t - tc0) * (god1 - god0) / (tc1 - tc0);
	return test_log10_from_god(god);
}

static double test_bg_fe_wustite_log10_h2o_h2_600c(void){
	return test_interp_log10_god(600.0,
		0.245296282085, 592.590131422, 0.255653986024, 614.912837142);
}

static double test_bg_wustite_spinel_log10_h2o_h2_600c(void){
	return test_interp_log10_god(600.0,
		0.302575011377, 599.15855048, 0.323005844537, 607.571969383);
}

static double test_bg_fe_wustite_log10_h2o_h2_700c(void){
	return test_interp_log10_god(700.0,
		0.28587264008, 682.801879077, 0.295194336274, 704.894663517);
}

static double test_bg_wustite_spinel_log10_h2o_h2_700c(void){
	return test_interp_log10_god(700.0,
		0.5333888167, 698.63992637, 0.553100275071, 707.819009833);
}

static double test_bg_fe_wustite_log10_h2o_h2_900c(void){
	return test_interp_log10_god(900.0,
		0.363887111598, 892.091017732, 0.371545484269, 917.140024584);
}

static double test_bg_wustite_spinel_log10_h2o_h2_900c(void){
	return test_interp_log10_god(900.0,
		0.821795065664, 889.906116431, 0.837937983872, 909.781941876);
}

static double test_bg_fe_spinel_log10_h2o_h2_400c(void){
	return test_interp_log10_god(400.0,
		0.0789270759127, 388.298687389, 0.0934618420789, 409.099758867);
}

static double test_bg_fe_spinel_log10_h2o_h2_500c(void){
	return test_interp_log10_god(500.0,
		0.168845620361, 498.137780549, 0.185316158674, 515.570661982);
}

static double test_bg_fe_spinel_log10_h2o_h2_540c(void){
	return test_interp_log10_god(540.0,
		0.202668265865, 532.581218262, 0.219718975701, 549.656298345);
}

static double test_bg_fe_spinel_log10_h2o_h2_560c(void){
	return test_interp_log10_god(560.0,
		0.219718975701, 549.656298345, 0.235228173231, 564.783638887);
}

static double test_bg_fe_spinel_log10_h2o_h2_564c(void){
	return test_interp_log10_god(564.0,
		0.219718975701, 549.656298345, 0.235228173231, 564.783638887);
}

static double test_bg_fe_wustite_log10_h2o_h2_near_fork(double tc){
	return test_interp_log10_god(tc,
		0.23456245187, 570.019013451, 0.245296282085, 592.590131422);
}

static double test_bg_wustite_spinel_log10_h2o_h2_near_fork(double tc){
	return test_interp_log10_god(tc,
		0.26050174532, 580.54667932, 0.280873172483, 589.174170906);
}

static double test_bg_fe_wustite_log10_h2o_h2_585c(void){
	return test_bg_fe_wustite_log10_h2o_h2_near_fork(585.0);
}

static double test_bg_wustite_spinel_log10_h2o_h2_585c(void){
	return test_bg_wustite_spinel_log10_h2o_h2_near_fork(585.0);
}

static double test_bg_fe_wustite_log10_co2_co_900c(void){
	return test_interp_log10_god(900.0,
		0.325713425546, 895.827936348, 0.323145197153, 905.433994412);
}

static double test_bg_wustite_spinel_log10_co2_co_900c(void){
	return test_interp_log10_god(900.0,
		0.801832125876, 899.918173303, 0.804776213998, 906.836870797);
}

static double test_bg_fe_spinel_log10_co2_co_400c(void){
	return test_interp_log10_god(400.0,
		0.494836284157, 392.811816123, 0.494579997181, 402.638693675);
}

static double test_bg_fe_spinel_log10_co2_co_500c(void){
	return test_interp_log10_god(500.0,
		0.494745749857, 492.138725534, 0.494649216265, 503.271581932);
}

static double test_bg_fe_spinel_log10_co2_co_540c(void){
	return test_interp_log10_god(540.0,
		0.494542792551, 532.139625675, 0.494809631148, 540.945689211);
}

static double test_bg_fe_spinel_log10_co2_co_560c(void){
	return test_interp_log10_god(560.0,
		0.494486035475, 559.113203095, 0.494618004345, 569.754945703);
}

static double test_bg_fe_spinel_log10_co2_co_565c(void){
	return test_interp_log10_god(565.0,
		0.494486035475, 559.113203095, 0.494618004345, 569.754945703);
}

static double test_bg_fe_wustite_log10_co2_co_near_fork(double tc){
	return test_interp_log10_god(tc,
		0.485399157248, 579.938513882, 0.481654756128, 585.359738343);
}

static double test_bg_wustite_spinel_log10_co2_co_near_fork(double tc){
	return test_interp_log10_god(tc,
		0.515790749521, 583.171121861, 0.521979204398, 587.265811035);
}

static double test_bg_fe_wustite_log10_co2_co_585c(void){
	return test_bg_fe_wustite_log10_co2_co_near_fork(585.0);
}

static double test_bg_wustite_spinel_log10_co2_co_585c(void){
	return test_bg_wustite_spinel_log10_co2_co_near_fork(585.0);
}

static double test_bg_fe_wustite_log10_co2_co_700c(void){
	return test_interp_log10_god(700.0,
		0.411802246834, 697.117244772, 0.406437428396, 706.669129735);
}

static double test_bg_wustite_spinel_log10_co2_co_700c(void){
	return test_interp_log10_god(700.0,
		0.656823398384, 698.497432348, 0.661738081109, 703.658005109);
}

static void test_prepare_feoh_phase_package(FpropsEqmPhaseModel *phases){
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe_bcc=hidayat_2015", NULL, &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &phases[1]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel", "degterov_2001", &phases[2]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe2O3=hidayat_2015", NULL, &phases[3]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &phases[4]));
}

static void test_prepare_feoh_phase_package_spinel_source(FpropsEqmPhaseModel *phases,
		const char *spinel_source){
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe_bcc=hidayat_2015", NULL, &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &phases[1]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel", spinel_source, &phases[2]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe2O3=hidayat_2015", NULL, &phases[3]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &phases[4]));
}

static void test_prepare_feoc_phase_package(FpropsEqmPhaseModel *phases){
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe_bcc=hidayat_2015", NULL, &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &phases[1]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel", "degterov_2001", &phases[2]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe2O3=hidayat_2015", NULL, &phases[3]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(carbonmonoxide,carbondioxide)",
			"Moran and Shapiro", &phases[4]));
}

static void test_eqm_phase_fixed_expanded_fe_gas_reducing_case(void){
	FpropsEqmPhaseModel phases[2];
	const char *elements[] = {"Fe", "O", "H"};
	double b[] = {2.0, 3.0, 200.0};
	double phase_amounts[2] = {NAN, NAN};
	double phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[3] = {NAN, NAN, NAN};
	double init[] = {2.0, 97.0, 3.0};
	int nmember = 0;
	int status;

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe_bcc=hidayat_2015", NULL, &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &phases[1]));
	status = fprops_eqm_phase_solve_fixed_expanded(phases, 2, elements, ARRAYLEN(elements),
		b, 1173.15, 101325.0, "auto", init, phase_amounts, phase_y, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 3);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[0], 2.0, 1e-7);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 100.0, 1e-7);
	CU_ASSERT_DOUBLE_EQUAL(member_amounts[1], 97.0, 1e-6);
	CU_ASSERT_DOUBLE_EQUAL(member_amounts[2], 3.0, 1e-6);
}

static void test_eqm_phase_fixed_expanded_wustite_gas(void){
	const double T = 1173.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	const double log10_ratio = 0.0;
	double ratio = pow(10.0, log10_ratio);
	double n_h2o = gas_total * ratio / (1.0 + ratio);
	double n_h2 = gas_total - n_h2o;
	FpropsEqmPhaseModel phases[2];
	double mu_h2;
	double mu_h2o;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[1] = {NAN};
	double elem_w[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "H"};
	double init[4];
	double phase_amounts[2] = {NAN, NAN};
	double phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[4] = {NAN, NAN, NAN, NAN};
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &phases[1]));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_h2o - mu_h2);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[0], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[0], y_entry, elem_w));
	i_fe = fprops_eqm_phase_find_element(&phases[0], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[0], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);

	b[0] = elem_w[i_fe];
	b[1] = elem_w[i_o] + n_h2o;
	b[2] = 2.0 * gas_total;
	init[0] = 1.0 - y_entry[0];
	init[1] = y_entry[0];
	init[2] = n_h2;
	init[3] = n_h2o;
	status = fprops_eqm_phase_solve_fixed_expanded(phases, 2, elements, ARRAYLEN(elements),
		b, T, P, "auto", init, phase_amounts, phase_y, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 4);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[0], 1.0, 1e-6);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], gas_total, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[0], y_entry[0], 2e-3);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[FPROPS_EQM_PHASE_MAX_VARS], 0.5, 2e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[FPROPS_EQM_PHASE_MAX_VARS + 1], 0.5, 2e-5);
}

static void test_eqm_phase_fixed_expanded_spinel_gas(void){
	const double T = 1173.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	const double log10_ratio = -2.0;
	double ratio = pow(10.0, log10_ratio);
	double n_h2o = gas_total * ratio / (1.0 + ratio);
	double n_h2 = gas_total - n_h2o;
	FpropsEqmPhaseModel phases[2];
	double mu_h2;
	double mu_h2o;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[2] = {NAN, NAN};
	double elem_sp[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "H"};
	double init[7];
	double phase_amounts[2] = {NAN, NAN};
	double phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[7];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;
	double a;
	double bo;
	double c;
	double v;

	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel", "degterov_2001", &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(hydrogen,water)",
			"helmholtz+ref0:", &phases[1]));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_h2o - mu_h2);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[0], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[0], y_entry, elem_sp));
	i_fe = fprops_eqm_phase_find_element(&phases[0], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[0], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);

	a = y_entry[0];
	bo = y_entry[1];
	c = (a + 5.0 - 4.0 * bo) / 6.0;
	v = (1.0 - a - 2.0 * bo) / 6.0;
	CU_ASSERT_TRUE_FATAL(c >= 0.0 && v >= 0.0);
	b[0] = elem_sp[i_fe];
	b[1] = elem_sp[i_o] + n_h2o;
	b[2] = 2.0 * gas_total;
	init[0] = a;
	init[1] = 1.0 - a;
	init[2] = 2.0 * bo;
	init[3] = 2.0 * c;
	init[4] = 2.0 * v;
	init[5] = n_h2;
	init[6] = n_h2o;
	status = fprops_eqm_phase_solve_fixed_expanded(phases, 2, elements, ARRAYLEN(elements),
		b, T, P, "auto", init, phase_amounts, phase_y, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 7);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[0], 1.0, 2e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], gas_total, 1e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[0], y_entry[0], 5e-2);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[1], y_entry[1], 5e-2);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[FPROPS_EQM_PHASE_MAX_VARS], n_h2 / gas_total, 2e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[FPROPS_EQM_PHASE_MAX_VARS + 1], n_h2o / gas_total, 2e-4);
}

static void test_eqm_phase_auto_fe_gas_reducing_case(void){
	FpropsEqmPhaseModel phases[5];
	const char *elements[] = {"Fe", "O", "H"};
	double b[] = {2.0, 3.0, 200.0};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;

	test_prepare_feoh_phase_package(phases);
	status = fprops_eqm_phase_solve_auto(phases, ARRAYLEN(phases), elements, ARRAYLEN(elements),
		b, 1173.15, 101325.0, "auto", phase_amounts, phase_y, active, member_amounts,
		&nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(active[0]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_TRUE(!active[1]);
	CU_ASSERT_TRUE(!active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[0], 2.0, 1e-6);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[4], 100.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS], 0.97, 2e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1], 0.03, 2e-5);
}

static void test_eqm_phase_active_set_fe_gas_reducing_case(void){
	FpropsEqmPhaseModel phases[5];
	const char *elements[] = {"Fe", "O", "H"};
	double b[] = {2.0, 3.0, 200.0};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;

	test_prepare_feoh_phase_package(phases);
	status = fprops_eqm_phase_solve_active_set(phases, ARRAYLEN(phases),
		elements, ARRAYLEN(elements), b, 1173.15, 101325.0, "auto", NULL,
		phase_amounts, phase_y, active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(active[0]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_TRUE(!active[1]);
	CU_ASSERT_TRUE(!active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[0], 2.0, 1e-6);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[4], 100.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS], 0.97, 2e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1], 0.03, 2e-5);
}

static void test_eqm_phase_active_set_result_status_wrapper(void){
	FpropsEqmPhaseModel phases[5];
	const char *elements[] = {"Fe", "O", "H"};
	double b[] = {2.0, 3.0, 200.0};
	FpropsEqmPhaseResult result;
	int gas_member_offset;
	int status;

	test_prepare_feoh_phase_package(phases);
	status = fprops_eqm_phase_solve_active_set_result(phases, ARRAYLEN(phases),
		elements, ARRAYLEN(elements), b, 1173.15, 101325.0, "auto", NULL, &result);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_EQUAL(result.status, 0);
	CU_ASSERT_TRUE(fprops_eqm_status_ok(result.solver_status));
	CU_ASSERT_EQUAL(result.nphase, 5);
	CU_ASSERT_EQUAL(result.nmember, 11);
	gas_member_offset = result.nmember - phases[4].nmember;
	CU_ASSERT_TRUE(result.phase_active[0]);
	CU_ASSERT_TRUE(result.phase_active[4]);
	CU_ASSERT_DOUBLE_EQUAL(result.phase_amounts[0], 2.0, 1e-6);
	CU_ASSERT_DOUBLE_EQUAL(result.member_amounts[gas_member_offset], 97.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(result.member_amounts[gas_member_offset + 1], 3.0, 1e-5);
}

static void test_eqm_phase_problem_api_fe_gas_reducing_case(void){
	FpropsEqm eqm;
	FpropsEqm eqm2;
	FpropsEqmPhaseResult result;
	const char *names[] = {"Fe2O3", "H2"};
	const char *coord_names[2];
	const char *member_names[5];
	double member_amounts[2];
	double amounts[] = {1.0, 100.0};
	int status;

	fprops_eqm_init(&eqm);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_add_phase(&eqm, "Fe_bcc=hidayat_2015", NULL) >= 0);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_add_phase(&eqm,
		"gas:ideal(hydrogen,water)=helmholtz+ref0:", NULL) >= 0);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_comp_list(&eqm, ARRAYLEN(names), names, amounts), 0);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "Fe"), 2.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "O"), 3.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "H"), 200.0, 1e-12);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_set_TP(&eqm, 1173.15, 101325.0), 0);
	CU_ASSERT_EQUAL(fprops_eqm_nlp_solver(&eqm), FPROPS_EQM_NLP_DEFAULT);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_set_nlp_solver(&eqm, FPROPS_EQM_NLP_SLSQP), 0);
	CU_ASSERT_EQUAL(fprops_eqm_nlp_solver(&eqm), FPROPS_EQM_NLP_SLSQP);

	status = fprops_eqm_solve(&eqm, &result);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_EQUAL(result.status, 0);
	CU_ASSERT_PTR_EQUAL(result.eqm, &eqm);
	CU_ASSERT_TRUE(fprops_eqm_status_ok(result.solver_status));
	CU_ASSERT_EQUAL(fprops_eqm_phase_count(&eqm), 2);
	CU_ASSERT_STRING_EQUAL(fprops_eqm_phase_name(&eqm, 1), "gas:ideal");
	CU_ASSERT_EQUAL(fprops_eqm_phase_member_count(&eqm, "gas:ideal"), 2);
	CU_ASSERT_STRING_EQUAL(fprops_eqm_phase_member_name(&eqm, "gas:ideal", 0), "hydrogen");
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_phase_amount(&result, "Fe_bcc"), 2.0, 1e-6);
	CU_ASSERT_EQUAL(fprops_eqm_phase_member_names(&eqm, "gas:ideal", member_names), 2);
	CU_ASSERT_STRING_EQUAL(member_names[1], "water");
	CU_ASSERT_EQUAL(fprops_eqm_phase_member_amounts(&result, "gas:ideal", member_amounts), 2);
	CU_ASSERT_DOUBLE_EQUAL(member_amounts[0], 97.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(member_amounts[1], 3.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_phase_member_amount(&result, "gas:ideal", "hydrogen"),
		97.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_phase_member_amount(&result, "gas:ideal", "water"),
		3.0, 1e-5);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_set_nlp_solver_name(&eqm, "default"), 0);
	CU_ASSERT_EQUAL(fprops_eqm_nlp_solver(&eqm), FPROPS_EQM_NLP_DEFAULT);

	fprops_eqm_clear_feed(&eqm);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_comps(&eqm, "Fe", 2, "O", 3, "H", 200), 0);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "Fe"), 2.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "O"), 3.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "H"), 200.0, 1e-12);

	fprops_eqm_clear_feed(&eqm);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_comps(&eqm, "Fe2O3", 1, "H2", 100), 0);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "Fe"), 2.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "O"), 3.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "H"), 200.0, 1e-12);

	fprops_eqm_clear_feed(&eqm);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_comps(&eqm, "Fe2O3", 1, "hydrogen", 100), 0);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "Fe"), 2.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "O"), 3.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm, "H"), 200.0, 1e-12);

	fprops_eqm_init(&eqm2);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_phases(&eqm2,
		"wustite=hidayat_2015", "spinel=degterov_2001"), 0);
	CU_ASSERT_EQUAL(fprops_eqm_phase_coord_count(&eqm2, "wustite"), 1);
	CU_ASSERT_STRING_EQUAL(fprops_eqm_phase_coord_name(&eqm2, "wustite", 0), "x_FeO1p5");
	CU_ASSERT_EQUAL(fprops_eqm_phase_coord_names(&eqm2, "spinel", coord_names), 2);
	CU_ASSERT_STRING_EQUAL(coord_names[0], "y_tet_fe2");
	CU_ASSERT_STRING_EQUAL(coord_names[1], "y_oct_fe2");
	CU_ASSERT_EQUAL(fprops_eqm_phase_member_names(&eqm2, "wustite", member_names), 2);
	CU_ASSERT_STRING_EQUAL(member_names[0], "Wus_FeO");
	CU_ASSERT_STRING_EQUAL(member_names[1], "Wus_FeO1p5");
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_phase_feed(&eqm2, "wustite", 2.0, 0.25), 0);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm2, "Fe"), 2.0, 1e-12);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm2, "O"), 2.25, 1e-12);
	fprops_eqm_clear_feed(&eqm2);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_phase_feed_vars(&eqm2, "spinel", 1.0,
		"y_oct_fe2", 0.20, "y_tet_fe2", 0.40), 0);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_element_amount(&eqm2, "O"), 4.0, 1e-12);
}

#if defined(HAVE_IPOPT)
static void test_eqm_phase_problem_api_ipopt_smoke(void){
	FpropsEqm eqm;
	FpropsEqmPhaseResult result;
	int status;

	fprops_eqm_init(&eqm);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_add_phase(&eqm, "Fe_bcc=hidayat_2015", NULL) >= 0);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_add_phase(&eqm,
		"gas:ideal(hydrogen,water)=helmholtz+ref0:", NULL) >= 0);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_add_comps(&eqm, "Fe2O3", 1, "H2", 100), 0);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_set_TP(&eqm, 1173.15, 101325.0), 0);
	CU_ASSERT_EQUAL_FATAL(fprops_eqm_set_nlp_solver(&eqm, FPROPS_EQM_NLP_IPOPT), 0);
	CU_ASSERT_EQUAL(fprops_eqm_nlp_solver(&eqm), FPROPS_EQM_NLP_IPOPT);

	status = fprops_eqm_solve(&eqm, &result);
	CU_ASSERT_EQUAL_FATAL(status, 0);
	CU_ASSERT_EQUAL(result.status, 0);
	CU_ASSERT_TRUE(fprops_eqm_status_ok(result.solver_status));
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_phase_amount(&result, "Fe_bcc"), 2.0, 1e-6);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_phase_member_amount(&result, "gas:ideal", "hydrogen"),
		97.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(fprops_eqm_phase_member_amount(&result, "gas:ideal", "water"),
		3.0, 1e-5);
}
#endif

static void test_run_feoh_fe_spinel_bg_active_set(double tc, double log10_ratio){
	const double T = tc + 273.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 2.0;
	const double metal_amount = 2.0;
	double ratio = pow(10.0, log10_ratio);
	double n_h2o = gas_total * ratio / (1.0 + ratio);
	double n_h2 = gas_total - n_h2o;
	FpropsEqmPhaseModel phases[5];
	double mu_h2;
	double mu_h2o;
	double mu_fe;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[2] = {NAN, NAN};
	double elem_sp[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "H"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	test_prepare_feoh_phase_package_spinel_source(phases, "fe_spinel_bg_tuned_2026");
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("Fe_bcc", "hidayat_2015", T, g_eqm.P0, &mu_fe));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_h2o - mu_h2);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[2], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	CU_ASSERT_DOUBLE_EQUAL(lambda_fe, mu_fe, 2e3);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[2], y_entry, elem_sp));
	i_fe = fprops_eqm_phase_find_element(&phases[2], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[2], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = metal_amount + elem_sp[i_fe];
	b[1] = elem_sp[i_o] + n_h2o;
	b[2] = 2.0 * gas_total;

	status = fprops_eqm_phase_solve_active_set(phases, ARRAYLEN(phases),
		elements, ARRAYLEN(elements), b, T, P, "auto", NULL, phase_amounts, phase_y,
		active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(active[0]);
	CU_ASSERT_TRUE(!active[1]);
	CU_ASSERT_TRUE(active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_TRUE(phase_amounts[0] > 0.0);
	CU_ASSERT_TRUE(phase_amounts[2] > 0.0);
	CU_ASSERT_TRUE(phase_amounts[4] > 0.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS] >= 0.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS] <= 1.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS + 1] >= 0.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS + 1] <= 0.5);
	CU_ASSERT_TRUE(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS] > 0.0);
	CU_ASSERT_TRUE(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1] > 0.0);
	(void)metal_amount;
	(void)n_h2;
	(void)n_h2o;
}

static void test_eqm_phase_active_set_fe_spinel_bg_400c(void){
	test_run_feoh_fe_spinel_bg_active_set(400.0,
		test_bg_fe_spinel_log10_h2o_h2_400c());
}

static void test_eqm_phase_active_set_fe_spinel_bg_500c(void){
	test_run_feoh_fe_spinel_bg_active_set(500.0,
		test_bg_fe_spinel_log10_h2o_h2_500c());
}

static void test_eqm_phase_active_set_fe_spinel_bg_540c(void){
	test_run_feoh_fe_spinel_bg_active_set(540.0,
		test_bg_fe_spinel_log10_h2o_h2_540c());
}

static void test_eqm_phase_active_set_fe_spinel_bg_560c(void){
	test_run_feoh_fe_spinel_bg_active_set(560.0,
		test_bg_fe_spinel_log10_h2o_h2_560c());
}

static void test_eqm_phase_active_set_fe_spinel_bg_564c(void){
	test_run_feoh_fe_spinel_bg_active_set(564.0,
		test_bg_fe_spinel_log10_h2o_h2_564c());
}

static void test_run_feoc_fe_spinel_bg_active_set(double tc, double log10_ratio){
	const double T = tc + 273.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 2.0;
	const double metal_amount = 2.0;
	double ratio = pow(10.0, log10_ratio);
	double n_co2 = gas_total * ratio / (1.0 + ratio);
	double n_co = gas_total - n_co2;
	FpropsEqmPhaseModel phases[5];
	double mu_co;
	double mu_co2;
	double mu_fe;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[2] = {NAN, NAN};
	double elem_sp[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "C"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe_bcc=hidayat_2015", NULL, &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("wustite", "hidayat_2015", &phases[1]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel", "fe_spinel_bg_tuned_2026",
			&phases[2]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe2O3=hidayat_2015", NULL, &phases[3]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(carbonmonoxide,carbondioxide)",
			"Moran and Shapiro", &phases[4]));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbonmonoxide", "Moran and Shapiro", T, g_eqm.P0,
			&mu_co));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbondioxide", "Moran and Shapiro", T, g_eqm.P0,
			&mu_co2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("Fe_bcc", "hidayat_2015", T, g_eqm.P0, &mu_fe));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_co2 - mu_co);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[2], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	CU_ASSERT_DOUBLE_EQUAL(lambda_fe, mu_fe, 2e3);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[2], y_entry, elem_sp));
	i_fe = fprops_eqm_phase_find_element(&phases[2], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[2], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = metal_amount + elem_sp[i_fe];
	b[1] = elem_sp[i_o] + n_co + 2.0 * n_co2;
	b[2] = gas_total;

	status = fprops_eqm_phase_solve_active_set(phases, ARRAYLEN(phases),
		elements, ARRAYLEN(elements), b, T, P, "auto", NULL, phase_amounts, phase_y,
		active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(active[0]);
	CU_ASSERT_TRUE(!active[1]);
	CU_ASSERT_TRUE(active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_TRUE(phase_amounts[0] > 0.0);
	CU_ASSERT_TRUE(phase_amounts[2] > 0.0);
	CU_ASSERT_TRUE(phase_amounts[4] > 0.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS] >= 0.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS] <= 1.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS + 1] >= 0.0);
	CU_ASSERT_TRUE(phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS + 1] <= 0.5);
	CU_ASSERT_TRUE(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS] > 0.0);
	CU_ASSERT_TRUE(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1] > 0.0);
	(void)metal_amount;
	(void)n_co;
	(void)n_co2;
}

static void test_eqm_phase_active_set_fe_spinel_co_bg_400c(void){
	test_run_feoc_fe_spinel_bg_active_set(400.0,
		test_bg_fe_spinel_log10_co2_co_400c());
}

static void test_eqm_phase_active_set_fe_spinel_co_bg_500c(void){
	test_run_feoc_fe_spinel_bg_active_set(500.0,
		test_bg_fe_spinel_log10_co2_co_500c());
}

static void test_eqm_phase_active_set_fe_spinel_co_bg_540c(void){
	test_run_feoc_fe_spinel_bg_active_set(540.0,
		test_bg_fe_spinel_log10_co2_co_540c());
}

static void test_eqm_phase_active_set_fe_spinel_co_bg_560c(void){
	test_run_feoc_fe_spinel_bg_active_set(560.0,
		test_bg_fe_spinel_log10_co2_co_560c());
}

static void test_eqm_phase_active_set_fe_spinel_co_bg_565c(void){
	test_run_feoc_fe_spinel_bg_active_set(565.0,
		test_bg_fe_spinel_log10_co2_co_565c());
}

static void test_run_feoh_fe_spinel_bg_entry(double tc, double log10_ratio){
	const double T = tc + 273.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	FpropsEqmPhaseModel spinel;
	double mu_h2;
	double mu_h2o;
	double mu_fe;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[2] = {NAN, NAN};

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("spinel",
			"fe_spinel_bg_tuned_2026", &spinel));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("Fe_bcc", "hidayat_2015", T, g_eqm.P0, &mu_fe));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_h2o - mu_h2);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&spinel, T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	CU_ASSERT_DOUBLE_EQUAL(lambda_fe, mu_fe, 2e3);
	CU_ASSERT_TRUE(isfinite(y_entry[0]));
	CU_ASSERT_TRUE(isfinite(y_entry[1]));
	CU_ASSERT_TRUE(y_entry[0] >= 0.0);
	CU_ASSERT_TRUE(y_entry[0] <= 1.0);
	CU_ASSERT_TRUE(y_entry[1] >= 0.0);
	CU_ASSERT_TRUE(y_entry[1] <= 0.5);
}

static void test_eqm_phase_entry_fe_spinel_bg_400c(void){
	test_run_feoh_fe_spinel_bg_entry(400.0,
		test_bg_fe_spinel_log10_h2o_h2_400c());
}

static void test_eqm_phase_entry_fe_spinel_bg_500c(void){
	test_run_feoh_fe_spinel_bg_entry(500.0,
		test_bg_fe_spinel_log10_h2o_h2_500c());
}

static void test_eqm_phase_entry_fe_spinel_bg_540c(void){
	test_run_feoh_fe_spinel_bg_entry(540.0,
		test_bg_fe_spinel_log10_h2o_h2_540c());
}

static void test_eqm_phase_entry_fe_spinel_bg_560c(void){
	test_run_feoh_fe_spinel_bg_entry(560.0,
		test_bg_fe_spinel_log10_h2o_h2_560c());
}

static void test_eqm_phase_entry_fe_spinel_bg_564c(void){
	test_run_feoh_fe_spinel_bg_entry(564.0,
		test_bg_fe_spinel_log10_h2o_h2_564c());
}

static void test_eqm_phase_validate_fe_gas_reducing_case(void){
	FpropsEqmPhaseModel phases[5];
	const char *elements[] = {"Fe", "O", "H"};
	double b[] = {2.0, 3.0, 200.0};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	double lambda[3] = {NAN, NAN, NAN};
	double residuals[5] = {NAN, NAN, NAN, NAN, NAN};
	double rms = NAN;
	int active[5];
	int nmember = 0;
	int status;

	test_prepare_feoh_phase_package(phases);
	status = fprops_eqm_phase_solve_auto(phases, ARRAYLEN(phases), elements, ARRAYLEN(elements),
		b, 1173.15, 101325.0, "auto", phase_amounts, phase_y, active, member_amounts,
		&nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_reconstruct_lambda(phases, ARRAYLEN(phases),
			elements, ARRAYLEN(elements), 1173.15, 101325.0, phase_amounts, phase_y,
			active, lambda, &rms));
	CU_ASSERT_TRUE(rms < 1e-8);
	CU_ASSERT_TRUE(isfinite(lambda[0]) && isfinite(lambda[1]) && isfinite(lambda[2]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_validate_entry_residuals(phases, ARRAYLEN(phases),
			elements, ARRAYLEN(elements), 1173.15, 101325.0, phase_amounts, phase_y,
			active, lambda, residuals));
	CU_ASSERT_TRUE(fabs(residuals[0]) < 1e-8);
	CU_ASSERT_TRUE(fabs(residuals[4]) < 1e-8);
	CU_ASSERT_TRUE(residuals[1] > -5e-2);
	CU_ASSERT_TRUE(residuals[2] > -5e-2);
	CU_ASSERT_TRUE(residuals[3] > -5e-2);
}

static void test_eqm_phase_auto_fe_co_co2_gas_smoke(void){
	FpropsEqmPhaseModel phases[2];
	const char *elements[] = {"Fe", "O", "C"};
	double b[] = {2.0, 103.0, 100.0};
	double phase_amounts[2];
	double phase_y[2 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[4];
	int active[2];
	int nmember = 0;
	int status;

	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("Fe_bcc=hidayat_2015", NULL, &phases[0]));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_resolve("gas:ideal(carbonmonoxide,carbondioxide)",
			"Moran and Shapiro", &phases[1]));
	status = fprops_eqm_phase_solve_auto(phases, ARRAYLEN(phases), elements, ARRAYLEN(elements),
		b, 1173.15, 101325.0, "auto", phase_amounts, phase_y, active, member_amounts,
		&nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 3);
	CU_ASSERT_TRUE(active[0]);
	CU_ASSERT_TRUE(active[1]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[0], 2.0, 1e-6);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 100.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[FPROPS_EQM_PHASE_MAX_VARS], 0.97, 2e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[FPROPS_EQM_PHASE_MAX_VARS + 1], 0.03, 2e-5);
}

static void test_eqm_phase_auto_wustite_co_bg_900c_classification(void){
	const double T = 1173.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	double log10_ratio = 0.5 * (test_bg_fe_wustite_log10_co2_co_900c()
		+ test_bg_wustite_spinel_log10_co2_co_900c());
	double ratio = pow(10.0, log10_ratio);
	double n_co2 = gas_total * ratio / (1.0 + ratio);
	double n_co = gas_total - n_co2;
	FpropsEqmPhaseModel phases[5];
	double mu_co;
	double mu_co2;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[1] = {NAN};
	double elem_w[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "C"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	CU_ASSERT_TRUE(test_bg_fe_wustite_log10_co2_co_900c() < log10_ratio);
	CU_ASSERT_TRUE(log10_ratio < test_bg_wustite_spinel_log10_co2_co_900c());
	test_prepare_feoc_phase_package(phases);
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbonmonoxide", "Moran and Shapiro", T, g_eqm.P0, &mu_co));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbondioxide", "Moran and Shapiro", T, g_eqm.P0, &mu_co2));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_co2 - mu_co);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[1], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[1], y_entry, elem_w));
	i_fe = fprops_eqm_phase_find_element(&phases[1], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[1], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = elem_w[i_fe];
	b[1] = elem_w[i_o] + n_co + 2.0 * n_co2;
	b[2] = gas_total;

	status = fprops_eqm_phase_solve_auto(phases, ARRAYLEN(phases), elements, ARRAYLEN(elements),
		b, T, P, "auto", phase_amounts, phase_y, active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(!active[0]);
	CU_ASSERT_TRUE(active[1]);
	CU_ASSERT_TRUE(!active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 1.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[4], gas_total, 1e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[1 * FPROPS_EQM_PHASE_MAX_VARS], y_entry[0], 4e-3);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS], n_co / gas_total, 2e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1], n_co2 / gas_total, 2e-4);
}

static void test_eqm_phase_active_set_wustite_co_bg_900c(void){
	const double T = 1173.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	double log10_ratio = 0.5 * (test_bg_fe_wustite_log10_co2_co_900c()
		+ test_bg_wustite_spinel_log10_co2_co_900c());
	double ratio = pow(10.0, log10_ratio);
	double n_co2 = gas_total * ratio / (1.0 + ratio);
	double n_co = gas_total - n_co2;
	FpropsEqmPhaseModel phases[5];
	double mu_co;
	double mu_co2;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[1] = {NAN};
	double elem_w[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "C"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	test_prepare_feoc_phase_package(phases);
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbonmonoxide", "Moran and Shapiro", T, g_eqm.P0, &mu_co));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbondioxide", "Moran and Shapiro", T, g_eqm.P0, &mu_co2));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_co2 - mu_co);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[1], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[1], y_entry, elem_w));
	i_fe = fprops_eqm_phase_find_element(&phases[1], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[1], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = elem_w[i_fe];
	b[1] = elem_w[i_o] + n_co + 2.0 * n_co2;
	b[2] = gas_total;

	status = fprops_eqm_phase_solve_active_set(phases, ARRAYLEN(phases),
		elements, ARRAYLEN(elements), b, T, P, "auto", NULL, phase_amounts, phase_y,
		active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(!active[0]);
	CU_ASSERT_TRUE(active[1]);
	CU_ASSERT_TRUE(!active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 1.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[4], gas_total, 1e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[1 * FPROPS_EQM_PHASE_MAX_VARS], y_entry[0], 4e-3);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS], n_co / gas_total, 2e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1], n_co2 / gas_total, 2e-4);
}

static void test_run_feoc_wustite_bg_classification(double tc, double log10_fe_wus,
		double log10_wus_spin, int active_set){
	const double T = tc + 273.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	double log10_ratio = 0.5 * (log10_fe_wus + log10_wus_spin);
	double ratio = pow(10.0, log10_ratio);
	double n_co2 = gas_total * ratio / (1.0 + ratio);
	double n_co = gas_total - n_co2;
	FpropsEqmPhaseModel phases[5];
	double mu_co;
	double mu_co2;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[1] = {NAN};
	double elem_w[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "C"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	CU_ASSERT_TRUE(log10_fe_wus < log10_ratio);
	CU_ASSERT_TRUE(log10_ratio < log10_wus_spin);
	test_prepare_feoc_phase_package(phases);
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbonmonoxide", "Moran and Shapiro", T, g_eqm.P0,
			&mu_co));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("carbondioxide", "Moran and Shapiro", T, g_eqm.P0,
			&mu_co2));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_co2 - mu_co);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[1], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[1], y_entry, elem_w));
	i_fe = fprops_eqm_phase_find_element(&phases[1], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[1], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = elem_w[i_fe];
	b[1] = elem_w[i_o] + n_co + 2.0 * n_co2;
	b[2] = gas_total;

	if(active_set){
		status = fprops_eqm_phase_solve_active_set(phases, ARRAYLEN(phases),
			elements, ARRAYLEN(elements), b, T, P, "auto", NULL, phase_amounts,
			phase_y, active, member_amounts, &nmember);
	}else{
		status = fprops_eqm_phase_solve_auto(phases, ARRAYLEN(phases),
			elements, ARRAYLEN(elements), b, T, P, "auto", phase_amounts, phase_y,
			active, member_amounts, &nmember);
	}
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(!active[0]);
	CU_ASSERT_TRUE(active[1]);
	CU_ASSERT_TRUE(!active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 1.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[4], gas_total, 1e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[1 * FPROPS_EQM_PHASE_MAX_VARS], y_entry[0], 4e-3);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS], n_co / gas_total, 2e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1], n_co2 / gas_total,
		2e-4);
}

static void test_eqm_phase_auto_wustite_co_bg_585c_classification(void){
	test_run_feoc_wustite_bg_classification(585.0,
		test_bg_fe_wustite_log10_co2_co_585c(),
		test_bg_wustite_spinel_log10_co2_co_585c(), 0);
}

static void test_eqm_phase_auto_wustite_co_bg_584c_classification(void){
	test_run_feoc_wustite_bg_classification(584.0,
		test_bg_fe_wustite_log10_co2_co_near_fork(584.0),
		test_bg_wustite_spinel_log10_co2_co_near_fork(584.0), 0);
}

static void test_eqm_phase_auto_wustite_co_bg_586c_classification(void){
	test_run_feoc_wustite_bg_classification(586.0,
		test_bg_fe_wustite_log10_co2_co_near_fork(586.0),
		test_bg_wustite_spinel_log10_co2_co_near_fork(586.0), 0);
}

static void test_eqm_phase_active_set_wustite_co_bg_585c(void){
	test_run_feoc_wustite_bg_classification(585.0,
		test_bg_fe_wustite_log10_co2_co_585c(),
		test_bg_wustite_spinel_log10_co2_co_585c(), 1);
}

static void test_eqm_phase_active_set_wustite_co_bg_584c(void){
	test_run_feoc_wustite_bg_classification(584.0,
		test_bg_fe_wustite_log10_co2_co_near_fork(584.0),
		test_bg_wustite_spinel_log10_co2_co_near_fork(584.0), 1);
}

static void test_eqm_phase_active_set_wustite_co_bg_586c(void){
	test_run_feoc_wustite_bg_classification(586.0,
		test_bg_fe_wustite_log10_co2_co_near_fork(586.0),
		test_bg_wustite_spinel_log10_co2_co_near_fork(586.0), 1);
}

static void test_eqm_phase_auto_wustite_co_bg_700c_classification(void){
	test_run_feoc_wustite_bg_classification(700.0,
		test_bg_fe_wustite_log10_co2_co_700c(),
		test_bg_wustite_spinel_log10_co2_co_700c(), 0);
}

static void test_eqm_phase_active_set_wustite_co_bg_700c(void){
	test_run_feoc_wustite_bg_classification(700.0,
		test_bg_fe_wustite_log10_co2_co_700c(),
		test_bg_wustite_spinel_log10_co2_co_700c(), 1);
}

static void test_run_feoh_wustite_bg_classification(double tc, double log10_fe_wus,
		double log10_wus_spin){
	const double T = tc + 273.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	double log10_ratio = 0.5 * (log10_fe_wus + log10_wus_spin);
	double ratio = pow(10.0, log10_ratio);
	double n_h2o = gas_total * ratio / (1.0 + ratio);
	double n_h2 = gas_total - n_h2o;
	FpropsEqmPhaseModel phases[5];
	double mu_h2;
	double mu_h2o;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[1] = {NAN};
	double elem_w[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "H"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	CU_ASSERT_TRUE(log10_fe_wus < log10_ratio);
	CU_ASSERT_TRUE(log10_ratio < log10_wus_spin);
	test_prepare_feoh_phase_package(phases);
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_h2o - mu_h2);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[1], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[1], y_entry, elem_w));
	i_fe = fprops_eqm_phase_find_element(&phases[1], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[1], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = elem_w[i_fe];
	b[1] = elem_w[i_o] + n_h2o;
	b[2] = 2.0 * gas_total;

	status = fprops_eqm_phase_solve_auto(phases, ARRAYLEN(phases), elements, ARRAYLEN(elements),
		b, T, P, "auto", phase_amounts, phase_y, active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(!active[0]);
	CU_ASSERT_TRUE(active[1]);
	CU_ASSERT_TRUE(!active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 1.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[4], gas_total, 1e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[1 * FPROPS_EQM_PHASE_MAX_VARS], y_entry[0], 4e-3);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS], n_h2 / gas_total, 2e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1], n_h2o / gas_total, 2e-4);
}

static void test_run_feoh_wustite_bg_active_set_with_init(double tc, double log10_fe_wus,
		double log10_wus_spin, const int *phase_active_init){
	const double T = tc + 273.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	double log10_ratio = 0.5 * (log10_fe_wus + log10_wus_spin);
	double ratio = pow(10.0, log10_ratio);
	double n_h2o = gas_total * ratio / (1.0 + ratio);
	double n_h2 = gas_total - n_h2o;
	FpropsEqmPhaseModel phases[5];
	double mu_h2;
	double mu_h2o;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[1] = {NAN};
	double elem_w[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "H"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	CU_ASSERT_TRUE(log10_fe_wus < log10_ratio);
	CU_ASSERT_TRUE(log10_ratio < log10_wus_spin);
	test_prepare_feoh_phase_package(phases);
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_h2o - mu_h2);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[1], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[1], y_entry, elem_w));
	i_fe = fprops_eqm_phase_find_element(&phases[1], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[1], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = elem_w[i_fe];
	b[1] = elem_w[i_o] + n_h2o;
	b[2] = 2.0 * gas_total;

	status = fprops_eqm_phase_solve_active_set(phases, ARRAYLEN(phases),
		elements, ARRAYLEN(elements), b, T, P, "auto", phase_active_init,
		phase_amounts, phase_y, active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_EQUAL(nmember, 11);
	CU_ASSERT_TRUE(!active[0]);
	CU_ASSERT_TRUE(active[1]);
	CU_ASSERT_TRUE(!active[2]);
	CU_ASSERT_TRUE(!active[3]);
	CU_ASSERT_TRUE(active[4]);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[1], 1.0, 1e-5);
	CU_ASSERT_DOUBLE_EQUAL(phase_amounts[4], gas_total, 1e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[1 * FPROPS_EQM_PHASE_MAX_VARS], y_entry[0], 4e-3);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS], n_h2 / gas_total, 2e-4);
	CU_ASSERT_DOUBLE_EQUAL(phase_y[4 * FPROPS_EQM_PHASE_MAX_VARS + 1], n_h2o / gas_total, 2e-4);
}

static void test_run_feoh_wustite_bg_active_set(double tc, double log10_fe_wus,
		double log10_wus_spin){
	test_run_feoh_wustite_bg_active_set_with_init(tc, log10_fe_wus, log10_wus_spin, NULL);
}

static void test_eqm_phase_auto_wustite_bg_600c_classification(void){
	test_run_feoh_wustite_bg_classification(600.0,
		test_bg_fe_wustite_log10_h2o_h2_600c(),
		test_bg_wustite_spinel_log10_h2o_h2_600c());
}

static void test_eqm_phase_auto_wustite_bg_585c_classification(void){
	test_run_feoh_wustite_bg_classification(585.0,
		test_bg_fe_wustite_log10_h2o_h2_585c(),
		test_bg_wustite_spinel_log10_h2o_h2_585c());
}

static void test_eqm_phase_auto_wustite_bg_584c_classification(void){
	test_run_feoh_wustite_bg_classification(584.0,
		test_bg_fe_wustite_log10_h2o_h2_near_fork(584.0),
		test_bg_wustite_spinel_log10_h2o_h2_near_fork(584.0));
}

static void test_eqm_phase_auto_wustite_bg_586c_classification(void){
	test_run_feoh_wustite_bg_classification(586.0,
		test_bg_fe_wustite_log10_h2o_h2_near_fork(586.0),
		test_bg_wustite_spinel_log10_h2o_h2_near_fork(586.0));
}

static void test_eqm_phase_auto_wustite_bg_700c_classification(void){
	test_run_feoh_wustite_bg_classification(700.0,
		test_bg_fe_wustite_log10_h2o_h2_700c(),
		test_bg_wustite_spinel_log10_h2o_h2_700c());
}

static void test_eqm_phase_active_set_wustite_bg_600c(void){
	test_run_feoh_wustite_bg_active_set(600.0,
		test_bg_fe_wustite_log10_h2o_h2_600c(),
		test_bg_wustite_spinel_log10_h2o_h2_600c());
}

static void test_eqm_phase_active_set_wustite_bg_585c(void){
	test_run_feoh_wustite_bg_active_set(585.0,
		test_bg_fe_wustite_log10_h2o_h2_585c(),
		test_bg_wustite_spinel_log10_h2o_h2_585c());
}

static void test_eqm_phase_active_set_wustite_bg_584c(void){
	test_run_feoh_wustite_bg_active_set(584.0,
		test_bg_fe_wustite_log10_h2o_h2_near_fork(584.0),
		test_bg_wustite_spinel_log10_h2o_h2_near_fork(584.0));
}

static void test_eqm_phase_active_set_wustite_bg_586c(void){
	test_run_feoh_wustite_bg_active_set(586.0,
		test_bg_fe_wustite_log10_h2o_h2_near_fork(586.0),
		test_bg_wustite_spinel_log10_h2o_h2_near_fork(586.0));
}

static void test_eqm_phase_active_set_wustite_bg_700c(void){
	test_run_feoh_wustite_bg_active_set(700.0,
		test_bg_fe_wustite_log10_h2o_h2_700c(),
		test_bg_wustite_spinel_log10_h2o_h2_700c());
}

static void test_eqm_phase_active_set_wustite_bg_700c_initial_masks(void){
	const int fe_gas[5] = {1, 0, 0, 0, 1};
	const int wus_gas[5] = {0, 1, 0, 0, 1};
	const int broad[5] = {1, 1, 1, 1, 1};
	test_run_feoh_wustite_bg_active_set_with_init(700.0,
		test_bg_fe_wustite_log10_h2o_h2_700c(),
		test_bg_wustite_spinel_log10_h2o_h2_700c(), fe_gas);
	test_run_feoh_wustite_bg_active_set_with_init(700.0,
		test_bg_fe_wustite_log10_h2o_h2_700c(),
		test_bg_wustite_spinel_log10_h2o_h2_700c(), wus_gas);
	test_run_feoh_wustite_bg_active_set_with_init(700.0,
		test_bg_fe_wustite_log10_h2o_h2_700c(),
		test_bg_wustite_spinel_log10_h2o_h2_700c(), broad);
}

static void test_eqm_phase_validate_wustite_bg_700c(void){
	const double tc = 700.0;
	const double T = tc + 273.15;
	const double P = 101325.0;
	const double R = FPROPS_R;
	const double gas_total = 10.0;
	double log10_ratio = 0.5 * (test_bg_fe_wustite_log10_h2o_h2_700c()
		+ test_bg_wustite_spinel_log10_h2o_h2_700c());
	double ratio = pow(10.0, log10_ratio);
	double n_h2o = gas_total * ratio / (1.0 + ratio);
	double n_h2 = gas_total - n_h2o;
	FpropsEqmPhaseModel phases[5];
	double mu_h2;
	double mu_h2o;
	double lambda_o_thermo;
	double lambda_fe;
	double y_entry[1] = {NAN};
	double elem_w[2];
	double b[3];
	const char *elements[] = {"Fe", "O", "H"};
	double phase_amounts[5];
	double phase_y[5 * FPROPS_EQM_PHASE_MAX_VARS];
	double member_amounts[16];
	double lambda[3] = {NAN, NAN, NAN};
	double residuals[5] = {NAN, NAN, NAN, NAN, NAN};
	double rms = NAN;
	int active[5];
	int nmember = 0;
	int status;
	int i_fe;
	int i_o;

	test_prepare_feoh_phase_package(phases);
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("hydrogen", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2));
	CU_ASSERT_TRUE_FATAL(eqm_mu0_source("water", "helmholtz+ref0:", T, g_eqm.P0, &mu_h2o));
	lambda_o_thermo = R * T * log(10.0) * log10_ratio + (mu_h2o - mu_h2);
	CU_ASSERT_TRUE_FATAL(test_phase_find_lambda_fe_for_entry(&phases[1], T, P,
			lambda_o_thermo, &lambda_fe, y_entry));
	(void)lambda_fe;
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_elements(&phases[1], y_entry, elem_w));
	i_fe = fprops_eqm_phase_find_element(&phases[1], "Fe");
	i_o = fprops_eqm_phase_find_element(&phases[1], "O");
	CU_ASSERT_TRUE_FATAL(i_fe >= 0 && i_o >= 0);
	b[0] = elem_w[i_fe];
	b[1] = elem_w[i_o] + n_h2o;
	b[2] = 2.0 * gas_total;
	(void)n_h2;

	status = fprops_eqm_phase_solve_auto(phases, ARRAYLEN(phases), elements, ARRAYLEN(elements),
		b, T, P, "auto", phase_amounts, phase_y, active, member_amounts, &nmember);
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_TRUE_FATAL(active[1]);
	CU_ASSERT_TRUE_FATAL(active[4]);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_reconstruct_lambda(phases, ARRAYLEN(phases),
			elements, ARRAYLEN(elements), T, P, phase_amounts, phase_y, active, lambda, &rms));
	CU_ASSERT_TRUE(rms < 1e-8);
	CU_ASSERT_TRUE_FATAL(fprops_eqm_phase_validate_entry_residuals(phases, ARRAYLEN(phases),
			elements, ARRAYLEN(elements), T, P, phase_amounts, phase_y, active, lambda,
			residuals));
	CU_ASSERT_TRUE(fabs(residuals[1]) < 2e-3);
	CU_ASSERT_TRUE(fabs(residuals[4]) < 1e-8);
	CU_ASSERT_TRUE(residuals[0] > -5e-2);
	CU_ASSERT_TRUE(residuals[2] > -5e-2);
	CU_ASSERT_TRUE(residuals[3] > -5e-2);
}

static void test_eqm_phase_auto_wustite_bg_900c_classification(void){
	test_run_feoh_wustite_bg_classification(900.0,
		test_bg_fe_wustite_log10_h2o_h2_900c(),
		test_bg_wustite_spinel_log10_h2o_h2_900c());
}

static void test_eqm_phase_active_set_wustite_bg_900c(void){
	test_run_feoh_wustite_bg_active_set(900.0,
		test_bg_fe_wustite_log10_h2o_h2_900c(),
		test_bg_wustite_spinel_log10_h2o_h2_900c());
}

static double qfm_log10fo2_from_mu0(double T){
	const double R = FPROPS_R;
	double mu_fe3o4 = NAN;
	double mu_sio2 = NAN;
	double mu_fayalite = NAN;
	double mu_o2 = NAN;
	double mu_buffer;
	if(!eqm_mu0_source("Fe3O4", "hidayat_2015", T, g_eqm.P0, &mu_fe3o4)){
		return NAN;
	}
	if(!eqm_mu0_source("SiO2", "slag_pragmatic_2026", T, g_eqm.P0, &mu_sio2)){
		return NAN;
	}
	if(!eqm_mu0_source("Fe2SiO4", "slag_pragmatic_2026", T, g_eqm.P0, &mu_fayalite)){
		return NAN;
	}
	if(!eqm_mu0_source("oxygen", "reaktoro_clone_supcrt98", T, g_eqm.P0, &mu_o2)){
		return NAN;
	}
	mu_buffer = 2.0 * mu_fe3o4 + 3.0 * mu_sio2 - 3.0 * mu_fayalite - mu_o2;
	return mu_buffer / (R * T * log(10.0));
}

static double qfi_log10fo2_from_mu0(double T){
	const double R = FPROPS_R;
	double mu_fe = stable_fe_mu0_hidayat(T);
	double mu_sio2 = NAN;
	double mu_fayalite = NAN;
	double mu_o2 = NAN;
	double mu_buffer;
	if(!isfinite(mu_fe)){
		return NAN;
	}
	if(!eqm_mu0_source("SiO2", "slag_pragmatic_2026", T, g_eqm.P0, &mu_sio2)){
		return NAN;
	}
	if(!eqm_mu0_source("Fe2SiO4", "slag_pragmatic_2026", T, g_eqm.P0, &mu_fayalite)){
		return NAN;
	}
	if(!eqm_mu0_source("oxygen", "helmholtz+ref0:", T, g_eqm.P0, &mu_o2)){
		return NAN;
	}
	mu_buffer = mu_fayalite - mu_sio2 - 2.0 * mu_fe - mu_o2;
	return mu_buffer / (R * T * log(10.0));
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
		const BinarySolutionPhaseDef *phase = NULL;
		const FeSpinelPhaseDef *spinel = NULL;
		const GibbsSpecies *G = NULL;
		const ConstCpSpecies *S = NULL;
		const ShomateSpecies *Sh = NULL;
		unsigned member_index = 0;
		int is_condensed = 0;
		if(solution_phase_lookup_member(names[i], source_i, &phase, &member_index)
				|| solution_phase_lookup_member(names[i], NULL, &phase, &member_index)){
			is_condensed = 1;
		}else if(spinel_phase_lookup_member(names[i], source_i, &spinel, &member_index)
				|| spinel_phase_lookup_member(names[i], NULL, &spinel, &member_index)){
			is_condensed = 1;
		}else{
			G = gibbs_species_lookup(names[i], source_i);
			if(!G){
				G = gibbs_species_lookup(names[i], NULL);
			}
			S = constcp_species_lookup(names[i], source_i);
			if(!S){
				S = constcp_species_lookup(names[i], NULL);
			}
			Sh = shomate_species_lookup(names[i], source_i);
			if(!Sh){
				Sh = shomate_species_lookup(names[i], NULL);
			}
			is_condensed = (G || S || (Sh && Sh->phase != FPROPS_PHASE_GAS)) ? 1 : 0;
		}
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
		const BinarySolutionPhaseDef *phase = NULL;
		const FeSpinelPhaseDef *spinel = NULL;
		const GibbsSpecies *G = NULL;
		const ConstCpSpecies *S = NULL;
		const ShomateSpecies *Sh = NULL;
		unsigned member_index = 0;
		int is_condensed = 0;
		if(solution_phase_lookup_member(names[i], source_i, &phase, &member_index)
				|| solution_phase_lookup_member(names[i], NULL, &phase, &member_index)){
			is_condensed = 1;
		}else if(spinel_phase_lookup_member(names[i], source_i, &spinel, &member_index)
				|| spinel_phase_lookup_member(names[i], NULL, &spinel, &member_index)){
			is_condensed = 1;
		}else{
			G = gibbs_species_lookup(names[i], source_i);
			if(!G){
				G = gibbs_species_lookup(names[i], NULL);
			}
			S = constcp_species_lookup(names[i], source_i);
			if(!S){
				S = constcp_species_lookup(names[i], NULL);
			}
			Sh = shomate_species_lookup(names[i], source_i);
			if(!Sh){
				Sh = shomate_species_lookup(names[i], NULL);
			}
			is_condensed = (G || S || (Sh && Sh->phase != FPROPS_PHASE_GAS)) ? 1 : 0;
		}
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
	const double R = FPROPS_R;
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

static double shomate_cp_species_test(const ShomateSpecies *S, double T){
	FpropsError err = FPROPS_NO_ERROR;
	unsigned i;
	const ShomateRange *R = NULL;
	if(!S || !S->ranges || S->nranges == 0){
		return NAN;
	}
	for(i = 0; i < S->nranges; ++i){
		if(shomate_range_contains(&S->ranges[i], T)){
			R = &S->ranges[i];
			break;
		}
	}
	if(!R){
		R = (T < S->ranges[0].T_min) ? &S->ranges[0] : &S->ranges[S->nranges - 1];
	}
	return shomate_cp_molar(R, T, &err);
}

static void assert_shomate_species_anchor(const ShomateSpecies *S, double h_ref, double s_ref){
	FpropsError err = FPROPS_NO_ERROR;
	double h = NAN;
	double s = NAN;
	CU_ASSERT_PTR_NOT_NULL_FATAL(S);
	h = shomate_species_h_molar(S, S->T_ref, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	s = shomate_species_s_molar(S, S->T_ref, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(isfinite(h));
	CU_ASSERT_TRUE(isfinite(s));
	CU_ASSERT_TRUE(fabs(h - h_ref) <= 1e-9);
	CU_ASSERT_TRUE(fabs(s - s_ref) <= 1e-9);
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
	const double R = FPROPS_R;
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

static void assert_reduced_solve_ok_source(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double *n_out){
	int i;
	int status = eqm_solve_elements(names, ns, elements, ne, b, source,
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

static void assert_log10K_consistent_source(const char **names, const double *nu, int ns,
		const char *source, const double *n){
	double log10_mu0 = log10K_from_mu0(names, nu, ns, source);
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

static void test_eqm_slag_species_cp_reference_points(void){
	const ShomateSpecies *sio2 = shomate_data_lookup("SiO2", "slag_pragmatic_2026");
	const ShomateSpecies *al2o3 = shomate_data_lookup("Al2O3", "slag_pragmatic_2026");
	const ShomateSpecies *fayalite = shomate_data_lookup("Fe2SiO4", "slag_pragmatic_2026");
	const ShomateSpecies *hercynite = shomate_data_lookup("FeAl2O4", "slag_pragmatic_2026");

	CU_ASSERT_PTR_NOT_NULL_FATAL(sio2);
	CU_ASSERT_PTR_NOT_NULL_FATAL(al2o3);
	CU_ASSERT_PTR_NOT_NULL_FATAL(fayalite);
	CU_ASSERT_PTR_NOT_NULL_FATAL(hercynite);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(sio2, 298.0) - 44.57) <= 0.05);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(sio2, 1000.0) - 68.95) <= 0.05);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(al2o3, 298.0) - 78.77) <= 0.05);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(al2o3, 1000.0) - 124.9) <= 0.1);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(fayalite, 298.15) - 131.9) <= 0.1);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(fayalite, 1000.0) - 190.64) <= 0.05);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(hercynite, 298.15) - 124.4) <= 0.1);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(hercynite, 1000.0) - 351.138) <= 0.1);
}

static void test_eqm_slag_species_reference_state_anchors(void){
	const ShomateSpecies *sio2 = shomate_data_lookup("SiO2", "slag_pragmatic_2026");
	const ShomateSpecies *al2o3 = shomate_data_lookup("Al2O3", "slag_pragmatic_2026");
	const ShomateSpecies *fayalite = shomate_data_lookup("Fe2SiO4", "slag_pragmatic_2026");
	const ShomateSpecies *hercynite = shomate_data_lookup("FeAl2O4", "slag_pragmatic_2026");

	assert_shomate_species_anchor(sio2, -910856.8, 41.44);
	assert_shomate_species_anchor(al2o3, -1675690.0, 50.92);
	assert_shomate_species_anchor(fayalite, -1478170.0, 151.00);
	assert_shomate_species_anchor(hercynite, -1947681.0, 115.362);
}

static void test_eqm_slag_species_mu0(void){
	static const char *species[] = {"SiO2", "Al2O3", "Fe2SiO4", "FeAl2O4"};
	size_t i;
	for(i = 0; i < ARRAYLEN(species); ++i){
		double mu0 = NAN;
		CU_ASSERT_TRUE(eqm_mu0_source(species[i], "slag_pragmatic_2026", 1000.0, 1e5, &mu0) != 0);
		CU_ASSERT_TRUE(isfinite(mu0));
	}
}

static void test_eqm_alumina_serena_species_cp_reference_points(void){
	const ShomateSpecies *gibbsite = shomate_data_lookup("gibbsite", "serena_2009");
	const ShomateSpecies *boehmite = shomate_data_lookup("boehmite", "serena_2009");
	const ShomateSpecies *al2o3 = shomate_data_lookup("Al2O3", "serena_2009");

	CU_ASSERT_PTR_NOT_NULL_FATAL(gibbsite);
	CU_ASSERT_PTR_NOT_NULL_FATAL(boehmite);
	CU_ASSERT_PTR_NOT_NULL_FATAL(al2o3);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(gibbsite, 298.15) - 182.2777315) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(gibbsite, 400.0) - 233.3513163) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(gibbsite, 500.0) - 271.6154544) <= 1e-6);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(boehmite, 298.15) - 105.8609241) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(boehmite, 400.0) - 132.9945956) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(boehmite, 500.0) - 151.1075612) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(boehmite, 700.0) - 161.9736129) <= 1e-6);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(al2o3, 298.0) - 78.77) <= 0.05);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(al2o3, 1000.0) - 124.9) <= 0.1);
}

static void test_eqm_alumina_serena_species_reference_state_anchors(void){
	const ShomateSpecies *gibbsite = shomate_data_lookup("gibbsite", "serena_2009");
	const ShomateSpecies *boehmite = shomate_data_lookup("boehmite", "serena_2009");
	const ShomateSpecies *al2o3 = shomate_data_lookup("Al2O3", "serena_2009");

	assert_shomate_species_anchor(gibbsite, -2594300.0, 139.4);
	assert_shomate_species_anchor(boehmite, -1981100.0, 96.86);
	assert_shomate_species_anchor(al2o3, -1675690.0, 50.92);
}

static void test_eqm_alumina_serena_species_mu0(void){
	static const char *species[] = {"gibbsite", "boehmite", "Al2O3"};
	static const double temps[] = {350.0, 500.0};
	size_t i, j;
	for(i = 0; i < ARRAYLEN(species); ++i){
		for(j = 0; j < ARRAYLEN(temps); ++j){
			double mu0 = NAN;
			CU_ASSERT_TRUE(eqm_mu0_source(species[i], "serena_2009", temps[j], 1e5, &mu0) != 0);
			CU_ASSERT_TRUE(isfinite(mu0));
		}
	}
}

static void test_eqm_gamma_alumina_usgs_species(void){
	const ShomateSpecies *gamma = shomate_data_lookup("gamma-Al2O3", "usgs_bull_1452_1978");
	double mu0 = NAN;

	assert_shomate_species_anchor(gamma, -1653517.0, 59.83);

	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(gamma, 298.15) - 79.0136097) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(gamma, 400.0) - 96.4938650) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(gamma, 1000.0) - 124.8869787) <= 1e-6);
	CU_ASSERT_TRUE(fabs(shomate_cp_species_test(gamma, 1800.0) - 135.1177681) <= 1e-6);

	CU_ASSERT_TRUE(eqm_mu0_source("gamma-Al2O3", "usgs_bull_1452_1978", 1000.0, 1e5, &mu0) != 0);
	CU_ASSERT_TRUE(isfinite(mu0));
}

static void test_eqm_feohsial_pure_capture_1000k(void){
	static const char *names[] = {
		"Fe_bcc", "Fe_fcc", "Fe3O4", "Fe2O3", "SiO2",
		"Fe2SiO4", "Al2O3", "FeAl2O4", "hydrogen", "water"
	};
	static const char *elements[] = {"Fe", "O", "Si", "Al", "H"};
	static const double b[] = {2.0, 4.2, 0.3, 0.2, 2.0};
	static const char *source_map =
		"Fe_bcc=hidayat_2015;Fe_fcc=hidayat_2015;Fe3O4=hidayat_2015;Fe2O3=hidayat_2015;"
		"SiO2=slag_pragmatic_2026;Fe2SiO4=slag_pragmatic_2026;"
		"Al2O3=slag_pragmatic_2026;FeAl2O4=slag_pragmatic_2026;"
		"hydrogen=Moran and Shapiro;water=Moran and Shapiro";
	double n[ARRAYLEN(names)];
	double n_metal;
	double n_locked_fe;
	int i_fe_bcc = find_name(names, ARRAYLEN(names), "Fe_bcc");
	int i_fe_fcc = find_name(names, ARRAYLEN(names), "Fe_fcc");
	int i_fe3o4 = find_name(names, ARRAYLEN(names), "Fe3O4");
	int i_fe2o3 = find_name(names, ARRAYLEN(names), "Fe2O3");
	int i_sio2 = find_name(names, ARRAYLEN(names), "SiO2");
	int i_fe2sio4 = find_name(names, ARRAYLEN(names), "Fe2SiO4");
	int i_al2o3 = find_name(names, ARRAYLEN(names), "Al2O3");
	int i_feal2o4 = find_name(names, ARRAYLEN(names), "FeAl2O4");
	int i_h2 = find_name(names, ARRAYLEN(names), "hydrogen");
	int i_h2o = find_name(names, ARRAYLEN(names), "water");
	int status = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b, source_map,
		1000.0, g_eqm.P, "auto", NULL, n);

	CU_ASSERT_EQM_STATUS_OK_FATAL(status);

	n_metal = n[i_fe_bcc] + n[i_fe_fcc];
	n_locked_fe = 2.0 * n[i_fe2sio4] + n[i_feal2o4];

	CU_ASSERT_TRUE(n[i_fe2sio4] > 0.25);
	CU_ASSERT_TRUE(n[i_fe2sio4] < 0.35);
	CU_ASSERT_TRUE(n[i_feal2o4] > 0.08);
	CU_ASSERT_TRUE(n[i_feal2o4] < 0.12);
	CU_ASSERT_TRUE(n[i_fe3o4] > 0.35);
	CU_ASSERT_TRUE(n[i_fe3o4] < 0.50);
	CU_ASSERT_TRUE(n[i_h2o] > 0.80);
	CU_ASSERT_TRUE(n[i_h2] < 0.20);
	CU_ASSERT_TRUE(n[i_h2o] > n[i_h2]);
	CU_ASSERT_TRUE(n[i_sio2] < 1e-6);
	CU_ASSERT_TRUE(n[i_al2o3] < 1e-6);
	CU_ASSERT_TRUE(n[i_fe2o3] < 1e-6);
	CU_ASSERT_TRUE(n_metal < 1e-6);
	CU_ASSERT_TRUE(n_locked_fe > 0.65);
	CU_ASSERT_TRUE(n_locked_fe < 0.75);
}

static void test_eqm_qfm_buffer_log10fo2_1000k(void){
	double log10fo2_model = qfm_log10fo2_from_mu0(1000.0);
	double log10fo2_ref = qfm_oneill_1987_log10fo2_low_branch(1000.0);
	double delta = log10fo2_model - log10fo2_ref;

	CU_ASSERT_TRUE_FATAL(isfinite(log10fo2_model));
	CU_ASSERT_TRUE_FATAL(isfinite(log10fo2_ref));
	if(fabs(delta) > 1.2){
		fprintf(stderr,
			"QFM mismatch at 1000 K: model=%.6f ref=%.6f delta=%.6f log10 units\n",
			log10fo2_model, log10fo2_ref, delta);
	}
	CU_ASSERT_TRUE(log10fo2_model < -16.0);
	CU_ASSERT_TRUE(log10fo2_model > -19.0);
	CU_ASSERT_TRUE(fabs(delta) <= 1.2);
}

static void test_eqm_qfi_buffer_log10fo2_1000k(void){
	double log10fo2_model = qfi_log10fo2_from_mu0(1000.0);
	double log10fo2_ref = qfi_oneill_1987_log10fo2(1000.0);
	double delta = log10fo2_model - log10fo2_ref;

	CU_ASSERT_TRUE_FATAL(isfinite(log10fo2_model));
	CU_ASSERT_TRUE_FATAL(isfinite(log10fo2_ref));
	if(fabs(delta) > 1.2){
		fprintf(stderr,
			"QFI mismatch at 1000 K: model=%.6f ref=%.6f delta=%.6f log10 units\n",
			log10fo2_model, log10fo2_ref, delta);
	}
	CU_ASSERT_TRUE(log10fo2_model < -21.0);
	CU_ASSERT_TRUE(log10fo2_model > -23.0);
	CU_ASSERT_TRUE(fabs(delta) <= 0.2);
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

#if defined(HAVE_IPOPT) && defined(HAVE_NLOPT)
static void test_eqm_wgs_ipopt_scaled_n_matches_slsqp(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	double n_ipopt[ARRAYLEN(names)] = {0};
	double n_slsqp[ARRAYLEN(names)] = {0};
	int status_ipopt = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements),
		b, g_eqm.source, g_eqm.T, g_eqm.P, "ipopt_scaled_n", NULL, n_ipopt);
	int status_slsqp = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements),
		b, g_eqm.source, g_eqm.T, g_eqm.P, "slsqp", NULL, n_slsqp);

	CU_ASSERT_TRUE_FATAL(status_ipopt == 0 || status_ipopt == 1 || status_ipopt == 6);
	CU_ASSERT_TRUE_FATAL(status_slsqp == 0);
	for(int i = 0; i < ARRAYLEN(names); ++i){
		CU_ASSERT_TRUE(fabs(n_ipopt[i] - n_slsqp[i]) < 2e-6);
	}
}
#endif

#if defined(HAVE_IPOPT)
static void test_eqm_wgs_ipopt_selector_matches_scaled_n(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	double n_ipopt[ARRAYLEN(names)] = {0};
	double n_scaled[ARRAYLEN(names)] = {0};
	int status_ipopt = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements),
		b, g_eqm.source, g_eqm.T, g_eqm.P, "ipopt", NULL, n_ipopt);
	int status_scaled = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements),
		b, g_eqm.source, g_eqm.T, g_eqm.P, "ipopt_scaled_n", NULL, n_scaled);

	CU_ASSERT_TRUE_FATAL(fprops_eqm_status_ok(status_ipopt));
	CU_ASSERT_TRUE_FATAL(fprops_eqm_status_ok(status_scaled));
	for(int i = 0; i < ARRAYLEN(names); ++i){
		CU_ASSERT_TRUE(fabs(n_ipopt[i] - n_scaled[i]) < 1e-9);
	}
}

static void test_eqm_wgs_ipopt_nullspace_matches_slsqp(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	double n_nullspace[ARRAYLEN(names)] = {0};
	double n_slsqp[ARRAYLEN(names)] = {0};
	int status_nullspace = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements),
		b, g_eqm.source, g_eqm.T, g_eqm.P, "nullspace", NULL, n_nullspace);
	int status_slsqp = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements),
		b, g_eqm.source, g_eqm.T, g_eqm.P, "slsqp", NULL, n_slsqp);

	CU_ASSERT_TRUE_FATAL(fprops_eqm_status_ok(status_nullspace));
	CU_ASSERT_EQUAL_FATAL(status_slsqp, 0);
	for(int i = 0; i < ARRAYLEN(names); ++i){
		CU_ASSERT_TRUE(fabs(n_nullspace[i] - n_slsqp[i]) < 2e-6);
	}
}
#endif

static void test_eqm_co2_dissociation_clone_reduced(void){
	static const char *names[] = {"carbonmonoxide", "oxygen", "carbondioxide"};
	static const char *elements[] = {"C", "O"};
	static const double b[] = {1.0, 2.0};
	static const double nu[] = {1.0, 0.5, -1.0};
	double n[ARRAYLEN(names)];
	const char *source = "reaktoro_clone_supcrt98";
	assert_reduced_solve_ok_source(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b, source, n);
	assert_log10K_consistent_source(names, nu, ARRAYLEN(names), source, n);
}

static void test_eqm_wgs_clone_reduced(void){
	static const char *names[] = {"carbonmonoxide", "water", "carbondioxide", "hydrogen"};
	static const char *elements[] = {"C", "O", "H"};
	static const double b[] = {1.0, 2.0, 2.0};
	static const double nu[] = {1.0, 1.0, -1.0, -1.0};
	double n[ARRAYLEN(names)];
	const char *source = "reaktoro_clone_supcrt98";
	assert_reduced_solve_ok_source(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b, source, n);
	assert_log10K_consistent_source(names, nu, ARRAYLEN(names), source, n);
}

static void test_eqm_feoc_clone_redox_1000k(void){
	static const char *names[] = {
		"Fe_bcc", "Fe_fcc", "Fe3O4", "carbonmonoxide", "carbondioxide"
	};
	static const char *elements[] = {"Fe", "C", "O"};
	static const double b[] = {3.0, 4.0, 8.0};
	static const double nu_redox[] = {3.0, 0.0, -1.0, -4.0, 4.0};
	static const char *source_map =
		"Fe_bcc=hidayat_2015;Fe_fcc=hidayat_2015;Fe3O4=hidayat_2015;"
		"carbonmonoxide=reaktoro_clone_supcrt98;carbondioxide=reaktoro_clone_supcrt98";
	double n[ARRAYLEN(names)] = {0.0};
	int i_fe_bcc = find_name(names, ARRAYLEN(names), "Fe_bcc");
	int i_fe_fcc = find_name(names, ARRAYLEN(names), "Fe_fcc");
	int i_fe3o4 = find_name(names, ARRAYLEN(names), "Fe3O4");
	int i_co = find_name(names, ARRAYLEN(names), "carbonmonoxide");
	int i_co2 = find_name(names, ARRAYLEN(names), "carbondioxide");
	int status = eqm_solve_elements(names, ARRAYLEN(names), elements, ARRAYLEN(elements), b, source_map,
		1000.0, g_eqm.P, "auto", NULL, n);

	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_TRUE(isfinite(n[i_fe_bcc]));
	CU_ASSERT_TRUE(isfinite(n[i_fe_fcc]));
	CU_ASSERT_TRUE(isfinite(n[i_fe3o4]));
	CU_ASSERT_TRUE(isfinite(n[i_co]));
	CU_ASSERT_TRUE(isfinite(n[i_co2]));
	CU_ASSERT_TRUE(n[i_fe3o4] > 0.0);
	CU_ASSERT_TRUE(n[i_fe3o4] > 0.90);
	CU_ASSERT_TRUE(n[i_fe_bcc] + n[i_fe_fcc] > 0.05);
	CU_ASSERT_TRUE(n[i_fe_bcc] > n[i_fe_fcc]);
	CU_ASSERT_TRUE(n[i_co] > 0.0);
	CU_ASSERT_TRUE(n[i_co2] > 0.0);
	CU_ASSERT_TRUE(n[i_co] > n[i_co2]);
	CU_ASSERT_TRUE(n[i_co] > 3.5);
	CU_ASSERT_TRUE(n[i_co2] > 0.05);
	{
		double log10_mu0 = log10K_from_mu0(names, nu_redox, ARRAYLEN(names), source_map);
		double log10_eqm = log10K_from_n_source_phaseaware(names, n, nu_redox, ARRAYLEN(names), source_map);
		CU_ASSERT_TRUE_FATAL(isfinite(log10_mu0));
		CU_ASSERT_TRUE_FATAL(isfinite(log10_eqm));
		CU_ASSERT_TRUE(fabs(log10_eqm - log10_mu0) <= g_eqm.log10_tol);
	}
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
					source, temps[it], P, "auto", NULL, n, &H_total);
			CU_ASSERT_EQM_STATUS_OK_FATAL(status);
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

static void test_fprops_rxn_package_ammonia_helmholtz_ref0_builds_and_solves(void){
	static const char *names[] = {"NH3", "H2", "N2"};
	static const char *source = "helmholtz+ref0:";
	FpropsRxnPackage *pkg = NULL;
	FpropsRxnTPN state;
	FpropsRxnResult out_pkg;
	double b[2] = {0.0, 0.0};
	double n_out[ARRAYLEN(names)] = {0.0, 0.0, 0.0};
	const double *A = NULL;
	int status_pkg;
	int ne;
	int e;

	pkg = fprops_rxn_package_build(names, ARRAYLEN(names), source);
	CU_ASSERT_PTR_NOT_NULL_FATAL(pkg);
	ne = fprops_rxn_package_num_elements(pkg);
	CU_ASSERT_EQUAL_FATAL(ne, 2);
	A = fprops_rxn_package_element_matrix(pkg);
	CU_ASSERT_PTR_NOT_NULL_FATAL(A);
	for(e = 0; e < ne; ++e){
		const double *row = &A[e * ARRAYLEN(names)];
		if(fabs(row[0] - 1.0) < 1e-12 && fabs(row[1]) < 1e-12 && fabs(row[2] - 2.0) < 1e-12){
			b[e] = 1.0;
		}else if(fabs(row[0] - 3.0) < 1e-12 && fabs(row[1] - 2.0) < 1e-12 && fabs(row[2]) < 1e-12){
			b[e] = 3.0;
		}else{
			CU_FAIL_FATAL("Unexpected ammonia package element");
		}
	}

	state.T = 573.0;
	state.P = 200.0 * 101325.0;
	state.n = NULL;
	out_pkg.status = -99;
	out_pkg.H = NAN;
	out_pkg.G = NAN;
	out_pkg.n_out = n_out;

	status_pkg = fprops_rxn_eqm_tpb(pkg, &state, b, "auto", NULL, &out_pkg);

	CU_ASSERT_TRUE(status_pkg == 0 || status_pkg == 1 || status_pkg == 6);
	CU_ASSERT_TRUE_FATAL(isfinite(n_out[0]));
	CU_ASSERT_TRUE_FATAL(isfinite(n_out[1]));
	CU_ASSERT_TRUE_FATAL(isfinite(n_out[2]));
	CU_ASSERT_TRUE(n_out[0] > 0.7);
	CU_ASSERT_TRUE(n_out[0] < 0.8);

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
	static const char *species[] = {"hydrogen", "oxygen", "water", "carbonmonoxide", "carbondioxide"};
	size_t i;
	for(i = 0; i < ARRAYLEN(species); ++i){
		double mu_auto = 0.0;
		double mu_shomate = 0.0;
		CU_ASSERT_TRUE(eqm_mu0_source(species[i], "reaktoro_clone_supcrt98", g_eqm.T, g_eqm.P0, &mu_auto) != 0);
		CU_ASSERT_TRUE(eqm_mu0_source(species[i], "shomate:reaktoro_clone_supcrt98", g_eqm.T, g_eqm.P0, &mu_shomate) != 0);
		CU_ASSERT_TRUE(fabs(mu_auto - mu_shomate) <= 1e-9);
	}
	{
		double mu_clone = 0.0;
		double mu_rpp = 0.0;
		CU_ASSERT_TRUE(eqm_mu0_source("carbonmonoxide", "reaktoro_clone_supcrt98", g_eqm.T, g_eqm.P0, &mu_clone) != 0);
		CU_ASSERT_TRUE(eqm_mu0_source("carbonmonoxide", "ideal+ref0:RPP", g_eqm.T, g_eqm.P0, &mu_rpp) != 0);
		CU_ASSERT_TRUE(fabs(mu_clone - mu_rpp) > 1000.0);
		CU_ASSERT_TRUE(eqm_mu0_source("carbondioxide", "reaktoro_clone_supcrt98", g_eqm.T, g_eqm.P0, &mu_clone) != 0);
		CU_ASSERT_TRUE(eqm_mu0_source("carbondioxide", "ideal+ref0:RPP", g_eqm.T, g_eqm.P0, &mu_rpp) != 0);
		CU_ASSERT_TRUE(fabs(mu_clone - mu_rpp) > 1000.0);
	}
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
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
	CU_ASSERT_TRUE(n[0] + n[1] > 0.0);
	CU_ASSERT_TRUE(fabs(2.0 * (n[0] + n[1]) - n[2] - n[3] - n[4]) <= 1e-6);
	CU_ASSERT_TRUE(fabs(6.0 * n[0] + 5.0 * n[1] - 2.0 * n[2] - 3.0 * n[3]) <= 1e-6);
}

static void test_eqm_bg_tuned_spinel_source_smoke(void){
	const FeSpinelPhaseDef *Pcur = NULL;
	const FeSpinelPhaseDef *Ptuned = NULL;
	unsigned i_cur = 0, i_tuned = 0;
	double n_members[5] = {0.1, 0.9, 0.2, 0.7, 0.1};
	double g_cur = NAN, g_tuned = NAN;
	CU_ASSERT_TRUE(spinel_phase_lookup_member("Sp_Fe2_tet", "degterov_2001", &Pcur, &i_cur) != 0);
	CU_ASSERT_TRUE(spinel_phase_lookup_member("Sp_Fe2_tet", "fe_spinel_bg_tuned_2026", &Ptuned, &i_tuned) != 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Pcur);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Ptuned);
	CU_ASSERT_TRUE(i_cur == 0);
	CU_ASSERT_TRUE(i_tuned == 0);
	CU_ASSERT_TRUE(spinel_phase_eval(Pcur, n_members, 623.15, g_eqm.P, &g_cur, NULL) != 0);
	CU_ASSERT_TRUE(spinel_phase_eval(Ptuned, n_members, 623.15, g_eqm.P, &g_tuned, NULL) != 0);
	CU_ASSERT_TRUE(isfinite(g_cur));
	CU_ASSERT_TRUE(isfinite(g_tuned));
	CU_ASSERT_TRUE(fabs(g_cur - g_tuned) > 1000.0);
}

static void test_eqm_mmc1_guess_spinel_source_smoke(void){
	const FeSpinelPhaseDef *Pcur = NULL;
	const FeSpinelPhaseDef *Pguess = NULL;
	unsigned i_cur = 0, i_guess = 0;
	double n_members[5] = {0.2, 0.8, 0.15, 0.75, 0.1};
	double g_cur = NAN, g_guess = NAN;
	CU_ASSERT_TRUE(spinel_phase_lookup_member("Sp_Fe2_tet", "degterov_2001", &Pcur, &i_cur) != 0);
	CU_ASSERT_TRUE(spinel_phase_lookup_member("Sp_Fe2_tet", "fe_spinel_mmc1_guess_2026", &Pguess, &i_guess) != 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Pcur);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Pguess);
	CU_ASSERT_TRUE(i_cur == 0);
	CU_ASSERT_TRUE(i_guess == 0);
	CU_ASSERT_TRUE(spinel_phase_eval(Pcur, n_members, 623.15, g_eqm.P, &g_cur, NULL) != 0);
	CU_ASSERT_TRUE(spinel_phase_eval(Pguess, n_members, 623.15, g_eqm.P, &g_guess, NULL) != 0);
	CU_ASSERT_TRUE(isfinite(g_cur));
	CU_ASSERT_TRUE(isfinite(g_guess));
	CU_ASSERT_TRUE(fabs(g_cur - g_guess) > 1.0);
}

static void test_eqm_hidayat_adj1_spinel_source_smoke(void){
	const FeSpinelPhaseDef *Pguess = NULL;
	const FeSpinelPhaseDef *Padj1 = NULL;
	unsigned i_guess = 0, i_adj1 = 0;
	double n_members[5] = {0.2, 0.8, 0.15, 0.75, 0.1};
	double g_guess = NAN, g_adj1 = NAN;
	CU_ASSERT_TRUE(spinel_phase_lookup_member("Sp_Fe2_tet", "fe_spinel_mmc1_guess_2026", &Pguess, &i_guess) != 0);
	CU_ASSERT_TRUE(spinel_phase_lookup_member("Sp_Fe2_tet", "hidayat_adj1", &Padj1, &i_adj1) != 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Pguess);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Padj1);
	CU_ASSERT_TRUE(i_guess == 0);
	CU_ASSERT_TRUE(i_adj1 == 0);
	CU_ASSERT_TRUE(spinel_phase_eval(Pguess, n_members, 623.15, g_eqm.P, &g_guess, NULL) != 0);
	CU_ASSERT_TRUE(spinel_phase_eval(Padj1, n_members, 623.15, g_eqm.P, &g_adj1, NULL) != 0);
	CU_ASSERT_TRUE(isfinite(g_guess));
	CU_ASSERT_TRUE(isfinite(g_adj1));
	CU_ASSERT_TRUE(fabs(g_guess - g_adj1) > 1.0);
}

static void test_eqm_feoxide_recon_source_smoke(void){
	const FeSpinelPhaseDef *Pspin = NULL;
	const BinarySolutionPhaseDef *Pwus = NULL;
	unsigned i_spin = 0, i_wus = 0;
	double mu = 0.0;
	CU_ASSERT_TRUE(eqm_mu0_source("Fe_bcc", "feoxide_recon_baseline_2026", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
	CU_ASSERT_TRUE(eqm_mu0_source("Fe_fcc", "feoxide_recon_baseline_2026", 1400.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
	CU_ASSERT_TRUE(eqm_mu0_source("Fe3O4", "feoxide_recon_baseline_2026", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
	CU_ASSERT_TRUE(eqm_mu0_source("Fe2O3", "feoxide_recon_baseline_2026", 1000.0, g_eqm.P0, &mu) != 0);
	CU_ASSERT_TRUE(isfinite(mu));
	CU_ASSERT_TRUE(solution_phase_lookup_member("Wus_FeO", "feoxide_recon_baseline_2026", &Pwus, &i_wus) != 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Pwus);
	CU_ASSERT_TRUE(i_wus == 0);
	CU_ASSERT_TRUE(spinel_phase_lookup_member("Sp_Fe2_tet", "feoxide_recon_baseline_2026", &Pspin, &i_spin) != 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Pspin);
	CU_ASSERT_TRUE(i_spin == 0);
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
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
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
	CU_ASSERT_EQM_STATUS_OK_FATAL(status);
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

#define EQM_CORE_TESTS(T) \
	T(mu0_core_species) \
	T(status_text_public_api) \
	T(h2o_dissociation_reduced) \
	T(wgs_reduced) \
	T(co2_dissociation_clone_reduced) \
	T(wgs_clone_reduced) \
	T(slag_species_cp_reference_points) \
	T(slag_species_reference_state_anchors) \
	T(slag_species_mu0) \
	T(alumina_serena_species_cp_reference_points) \
	T(alumina_serena_species_reference_state_anchors) \
	T(alumina_serena_species_mu0) \
	T(gamma_alumina_usgs_species) \
	T(feohsial_pure_capture_1000k) \
	T(qfm_buffer_log10fo2_1000k) \
	T(qfi_buffer_log10fo2_1000k) \
	T(feoc_clone_redox_1000k) \
	T(ammonia_synthesis_helmholtz_ref0_matches_hr_grid) \
	T(wgs_permutation_invariance) \
	T(multi_reaction_mixed_system) \
	T(humid_air_nox_auto_reduced_lowt) \
	T(fe_oxide_mu0_data) \
	T(mu0_source_map_resolution) \
	T(nio_h2_mixed_source_map_reduced) \
	T(mu0_model_selectors) \
	T(helmholtz_ref0_matches_ms_reaction_delta_g) \
	T(nio_h2_shomate_selector_reduced) \
	T(explicit_unknown_source_falls_back_to_ideal) \
	T(reaktoro_clone_auto_routes_to_clone) \
	T(hidayat_pragmatic_species_mu0) \
	T(hidayat_magnetic_mu0_matches_formula) \
	T(degterov_spinel_fe3o4_smoke) \
	T(bg_tuned_spinel_source_smoke) \
	T(mmc1_guess_spinel_source_smoke) \
	T(hidayat_adj1_spinel_source_smoke) \
	T(feoxide_recon_source_smoke) \
	T(wustite_solution_fullspace_unique_balance) \
	T(bcc_iron_solution_fullspace_unique_balance) \
	T(wustite_solution_rejects_nullspace_only) \
	T(feoh_reaktoro_clone_boundary_912C)

#define PHASE_TESTS(T) \
	T(registry_inspection_feoh) \
	T(package_resolver_feoh) \
	T(gibbs_and_elements_feoh) \
	T(entry_fe_wustite_900c_anchor) \
	T(entry_wustite_spinel_900c_anchor) \
	T(fixed_linear_fe_gas_reducing_case) \
	T(fixed_expanded_fe_gas_reducing_case) \
	T(fixed_expanded_wustite_gas) \
	T(fixed_expanded_spinel_gas) \
	T(auto_fe_gas_reducing_case) \
	T(active_set_fe_gas_reducing_case) \
	T(active_set_result_status_wrapper) \
	T(problem_api_fe_gas_reducing_case) \
	T(entry_fe_spinel_bg_400c) \
	T(entry_fe_spinel_bg_500c) \
	T(entry_fe_spinel_bg_540c) \
	T(entry_fe_spinel_bg_560c) \
	T(entry_fe_spinel_bg_564c) \
	T(active_set_fe_spinel_bg_400c) \
	T(active_set_fe_spinel_bg_500c) \
	T(active_set_fe_spinel_bg_540c) \
	T(active_set_fe_spinel_bg_560c) \
	T(active_set_fe_spinel_bg_564c) \
	T(active_set_fe_spinel_co_bg_400c) \
	T(active_set_fe_spinel_co_bg_500c) \
	T(active_set_fe_spinel_co_bg_540c) \
	T(active_set_fe_spinel_co_bg_560c) \
	T(active_set_fe_spinel_co_bg_565c) \
	T(validate_fe_gas_reducing_case) \
	T(auto_fe_co_co2_gas_smoke) \
	T(auto_wustite_co_bg_584c_classification) \
	T(auto_wustite_co_bg_585c_classification) \
	T(auto_wustite_co_bg_586c_classification) \
	T(active_set_wustite_co_bg_584c) \
	T(active_set_wustite_co_bg_585c) \
	T(active_set_wustite_co_bg_586c) \
	T(auto_wustite_co_bg_700c_classification) \
	T(active_set_wustite_co_bg_700c) \
	T(auto_wustite_co_bg_900c_classification) \
	T(active_set_wustite_co_bg_900c) \
	T(auto_wustite_bg_584c_classification) \
	T(auto_wustite_bg_585c_classification) \
	T(auto_wustite_bg_586c_classification) \
	T(active_set_wustite_bg_584c) \
	T(active_set_wustite_bg_585c) \
	T(active_set_wustite_bg_586c) \
	T(auto_wustite_bg_600c_classification) \
	T(active_set_wustite_bg_600c) \
	T(auto_wustite_bg_700c_classification) \
	T(validate_wustite_bg_700c) \
	T(active_set_wustite_bg_700c) \
	T(active_set_wustite_bg_700c_initial_masks) \
	T(auto_wustite_bg_900c_classification) \
	T(active_set_wustite_bg_900c)

#define FPROPS_EQM_TESTS(T) \
	T(tpb_wgs_ms_table) \
	T(tpy_wgs_normalization_and_match_tpb)

#define MIX_H_TESTS(T) \
	T(wgs_matches_tpb_and_scales) \
	T(solution_phase_unsupported) \
	T(fe2o3_h2_reduction_matches_standard_enthalpy)

#define RXN_PACKAGE_TESTS(T) \
	T(mix_h_supports_wustite_phase) \
	T(mix_h_matches_legacy) \
	T(ammonia_helmholtz_ref0_builds_and_solves) \
	T(eqm_matches_legacy) \
	T(eqm_tpy_matches_legacy) \
	T(eqm_sensitivities_wgs)

#define NAME_RESOLVE_TESTS(T) \
	T(reactive_and_unifac_domains)

#define UNIFAC_TESTS(T) \
	T(native_source_data_lookup) \
	T(runtime_prepare_and_gamma) \
	T(liq_fugacity_matches_vlecalc_ethanol_water_bubble_points)

#define FLASH_TESTS(T) \
	T(prepare_unifac_and_tpz)

#define ADD_EQM_CORE_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_eqm_##NAME)){ \
		return CUE_NOTEST; \
	}

#define ADD_PHASE_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_eqm_phase_##NAME)){ \
		return CUE_NOTEST; \
	}

#define ADD_FPROPS_EQM_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_fprops_eqm_##NAME)){ \
		return CUE_NOTEST; \
	}

#define ADD_MIX_H_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_fprops_mix_h_tpn_##NAME)){ \
		return CUE_NOTEST; \
	}

#define ADD_RXN_PACKAGE_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_fprops_rxn_package_##NAME)){ \
		return CUE_NOTEST; \
	}

#define ADD_NAME_RESOLVE_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_name_resolve_##NAME)){ \
		return CUE_NOTEST; \
	}

#define ADD_UNIFAC_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_unifac_##NAME)){ \
		return CUE_NOTEST; \
	}

#define ADD_FLASH_TEST(NAME) \
	if(NULL == CU_add_test(s, #NAME, test_flash_##NAME)){ \
		return CUE_NOTEST; \
	}

static CU_ErrorCode test_register_eqm_core_suite(void){
	CU_pSuite s = CU_add_suite("eqm", eqm_suite_init, eqm_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	EQM_CORE_TESTS(ADD_EQM_CORE_TEST)
#if defined(HAVE_IPOPT) && defined(HAVE_NLOPT)
	if(NULL == CU_add_test(s, "wgs_ipopt_scaled_n_matches_slsqp",
			test_eqm_wgs_ipopt_scaled_n_matches_slsqp)){
		return CUE_NOTEST;
	}
#endif
#if defined(HAVE_IPOPT)
	if(NULL == CU_add_test(s, "wgs_ipopt_selector_matches_scaled_n",
			test_eqm_wgs_ipopt_selector_matches_scaled_n)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "wgs_ipopt_nullspace_matches_slsqp",
			test_eqm_wgs_ipopt_nullspace_matches_slsqp)){
		return CUE_NOTEST;
	}
#endif
	/* Temporarily skipped: unstable under current data/solver settings. */
	(void)test_eqm_feo_pragmatic_low_oxygen_smoke_1400K;
	return CUE_SUCCESS;
}

static CU_ErrorCode test_register_phase_suite(void){
	CU_pSuite s = CU_add_suite("phase", eqm_suite_init, eqm_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	PHASE_TESTS(ADD_PHASE_TEST)
#if defined(HAVE_IPOPT)
	if(NULL == CU_add_test(s, "problem_api_ipopt_smoke",
			test_eqm_phase_problem_api_ipopt_smoke)){
		return CUE_NOTEST;
	}
#endif
	return CUE_SUCCESS;
}

static CU_ErrorCode test_register_fprops_eqm_suite(void){
	CU_pSuite s = CU_add_suite("fprops_eqm", eqm_suite_init, eqm_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	FPROPS_EQM_TESTS(ADD_FPROPS_EQM_TEST)
	return CUE_SUCCESS;
}

static CU_ErrorCode test_register_mix_h_suite(void){
	CU_pSuite s = CU_add_suite("mix_h", eqm_suite_init, eqm_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	MIX_H_TESTS(ADD_MIX_H_TEST)
	return CUE_SUCCESS;
}

static CU_ErrorCode test_register_rxn_package_suite(void){
	CU_pSuite s = CU_add_suite("rxn_package", eqm_suite_init, eqm_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	RXN_PACKAGE_TESTS(ADD_RXN_PACKAGE_TEST)
	return CUE_SUCCESS;
}

static CU_ErrorCode test_register_unifac_flash_suite(void){
	CU_pSuite s = CU_add_suite("unifac_flash", eqm_suite_init, eqm_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	NAME_RESOLVE_TESTS(ADD_NAME_RESOLVE_TEST)
	UNIFAC_TESTS(ADD_UNIFAC_TEST)
	FLASH_TESTS(ADD_FLASH_TEST)
	return CUE_SUCCESS;
}

CU_ErrorCode test_register_eqm(void){
	CU_ErrorCode result;
	result = test_register_eqm_core_suite();
	if(result != CUE_SUCCESS){
		return result;
	}
	result = test_register_phase_suite();
	if(result != CUE_SUCCESS){
		return result;
	}
	result = test_register_fprops_eqm_suite();
	if(result != CUE_SUCCESS){
		return result;
	}
	result = test_register_mix_h_suite();
	if(result != CUE_SUCCESS){
		return result;
	}
	result = test_register_rxn_package_suite();
	if(result != CUE_SUCCESS){
		return result;
	}
	return test_register_unifac_flash_suite();
}
