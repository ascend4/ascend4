#include "../test.h"
#include "../fprops.h"
#include "../fluids.h"
#include "../refstate.h"
#include "../pengrob.h"
#include "../ideal.h"
#include "../helmholtz.h"
#include "../cp0.h"
#include "../zeroin.h"

#include <math.h>

typedef struct RefstateFixture{
	double T;
	double rho_seed;
	double rho_helm_seed;
	double h_shift;
	double s_shift;
	double g_shift;
	double h_tol;
	double s_tol;
	double g_tol;
} RefstateFixture;

static RefstateFixture g_ref = {0};

typedef struct RefstateTPData{
	PureFluid *P;
	double T;
	double p;
} RefstateTPData;

static int refstate_suite_init(void){
	g_ref.T = 350.0;
	g_ref.rho_seed = 6.0;
	g_ref.rho_helm_seed = 9.0;
	g_ref.h_shift = 12345.0;
	g_ref.s_shift = 37.5;
	g_ref.g_shift = 1500.0;
	g_ref.h_tol = 1e-3;
	g_ref.s_tol = 1e-6;
	g_ref.g_tol = 1e-3;
	return 0;
}

static int refstate_suite_cleanup(void){
	return 0;
}

extern const EosData eos_rpp_nitrogen;

static double to_molar(double specific, const PureFluid *P){
	return specific * P->data->M / 1000.0;
}

static PureFluid *prepare_pengrob_n2(void){
	const EosData *E = fprops_eos("nitrogen", "pengrob", "RPP");
	ReferenceState ref = {FPROPS_REF_PHI0,{.phi0={0,0}}};
	const PureFluid *P;
	CU_ASSERT_PTR_NOT_NULL_FATAL(E);
	P = pengrob_prepare(E, &ref);
	CU_ASSERT_PTR_NOT_NULL_FATAL(P);
	return (PureFluid *)P;
}

static PureFluid *prepare_helmholtz_n2(void){
	const PureFluid *P = fprops_fluid("nitrogen", "helmholtz", NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(P);
	return (PureFluid *)P;
}

static double refstate_rho_resid(double rho, void *user_data){
	RefstateTPData *D = (RefstateTPData *)user_data;
	FpropsError err = FPROPS_NO_ERROR;
	double p = fprops_p(fprops_set_Trho(D->T, rho, D->P, &err), &err);
	if(err){
		return -1.0;
	}
	return p - D->p;
}

static int solve_rho_from_Tp(PureFluid *P, double T, double p, double *rho_out){
	RefstateTPData D;
	double resid = 0.0;
	int zerr;
	if(!P || !rho_out){
		return 0;
	}
	D.P = P;
	D.T = T;
	D.p = p;
	zerr = zeroin_solve(&refstate_rho_resid, &D, 1e-10, 5.0 * P->data->rho_c, 1e-5, rho_out, &resid);
	return zerr == 0;
}

static void test_refstate_tphs_applies_target_pengrob(void){
	PureFluid *P = prepare_pengrob_n2();
	FpropsError err = FPROPS_NO_ERROR;
	FluidState2 S0 = fprops_set_Trho(g_ref.T, g_ref.rho_seed, P, &err);
	double p0;
	double h0;
	double s0;
	double h_target;
	double s_target;
	double h1 = NAN;
	double s1 = NAN;
	double rho1 = NAN;
	ReferenceState R;
	int res;
	FluidState2 S1;

	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	p0 = fprops_p(S0, &err);
	h0 = fprops_h(S0, &err);
	s0 = fprops_s(S0, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);

	h_target = h0 + g_ref.h_shift;
	s_target = s0 + g_ref.s_shift;
	R = (ReferenceState){FPROPS_REF_TPHS,{.tphs={g_ref.T, p0, h_target, s_target}}};
	res = fprops_set_reference_state(P, &R);
	CU_ASSERT_EQUAL_FATAL(res, 0);

	CU_ASSERT_TRUE_FATAL(solve_rho_from_Tp(P, g_ref.T, p0, &rho1));
	S1 = fprops_set_Trho(g_ref.T, rho1, P, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h1 = fprops_h(S1, &err);
	s1 = fprops_s(S1, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(fabs(h1 - h_target) <= g_ref.h_tol);
	CU_ASSERT_TRUE(fabs(s1 - s_target) <= g_ref.s_tol);

	fprops_fluid_destroy(P);
}

static void test_refstate_tphg_applies_target_pengrob(void){
	PureFluid *P = prepare_pengrob_n2();
	FpropsError err = FPROPS_NO_ERROR;
	FluidState2 S0 = fprops_set_Trho(g_ref.T, g_ref.rho_seed, P, &err);
	double p0;
	double h0;
	double g0;
	double h_target;
	double g_target;
	double s_target;
	double h1 = NAN;
	double s1 = NAN;
	double rho1 = NAN;
	ReferenceState R;
	int res;
	FluidState2 S1;

	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	p0 = fprops_p(S0, &err);
	h0 = fprops_h(S0, &err);
	g0 = fprops_g(S0, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);

	h_target = h0 + g_ref.h_shift;
	g_target = g0 + g_ref.g_shift;
	s_target = (h_target - g_target) / g_ref.T;
	R = (ReferenceState){FPROPS_REF_TPHG,{.tphg={g_ref.T, p0, h_target, g_target}}};
	res = fprops_set_reference_state(P, &R);
	CU_ASSERT_EQUAL_FATAL(res, 0);

	CU_ASSERT_TRUE_FATAL(solve_rho_from_Tp(P, g_ref.T, p0, &rho1));
	S1 = fprops_set_Trho(g_ref.T, rho1, P, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h1 = fprops_h(S1, &err);
	s1 = fprops_s(S1, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(fabs(h1 - h_target) <= g_ref.h_tol);
	CU_ASSERT_TRUE(fabs(s1 - s_target) <= g_ref.s_tol);

	fprops_fluid_destroy(P);
}

static void test_pengrob_prepare_applies_requested_reference(void){
	const EosData *E = fprops_eos("nitrogen", NULL, "RPP");
	PureFluid *Pbase;
	PureFluid *P;
	ReferenceState ref_base = {FPROPS_REF_PHI0,{.phi0={0,0}}};
	FpropsError err = FPROPS_NO_ERROR;
	double p0;
	double h0;
	double s0;
	double h_target;
	double s_target;
	double rho1;
	FluidState2 S0;
	FluidState2 S1;
	ReferenceState R;

	CU_ASSERT_PTR_NOT_NULL_FATAL(E);
	Pbase = pengrob_prepare(E, &ref_base);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Pbase);

	S0 = fprops_set_Trho(g_ref.T, g_ref.rho_seed, Pbase, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	p0 = fprops_p(S0, &err);
	h0 = fprops_h(S0, &err);
	s0 = fprops_s(S0, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	fprops_fluid_destroy(Pbase);

	h_target = h0 + 0.5 * g_ref.h_shift;
	s_target = s0 + 0.5 * g_ref.s_shift;
	R = (ReferenceState){FPROPS_REF_TPHS,{.tphs={g_ref.T, p0, h_target, s_target}}};
	P = pengrob_prepare(E, &R);
	CU_ASSERT_PTR_NOT_NULL_FATAL(P);
	CU_ASSERT_TRUE_FATAL(solve_rho_from_Tp(P, g_ref.T, p0, &rho1));
	S1 = fprops_set_Trho(g_ref.T, rho1, P, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h0 = fprops_h(S1, &err);
	s0 = fprops_s(S1, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(fabs(h0 - h_target) <= g_ref.h_tol);
	CU_ASSERT_TRUE(fabs(s0 - s_target) <= g_ref.s_tol);

	fprops_fluid_destroy(P);
}

static void test_refstate_tphs_applies_target_helmholtz(void){
	PureFluid *P = prepare_helmholtz_n2();
	FpropsError err = FPROPS_NO_ERROR;
	FluidState2 S0 = fprops_set_Trho(g_ref.T, g_ref.rho_helm_seed, P, &err);
	double p0;
	double h0;
	double s0;
	double h_target;
	double s_target;
	double h1;
	double s1;
	double rho1;
	ReferenceState R;
	int res;
	FluidState2 S1;

	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	p0 = fprops_p(S0, &err);
	h0 = fprops_h(S0, &err);
	s0 = fprops_s(S0, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);

	h_target = h0 + g_ref.h_shift;
	s_target = s0 + g_ref.s_shift;
	R = (ReferenceState){FPROPS_REF_TPHS,{.tphs={g_ref.T, p0, h_target, s_target}}};
	res = fprops_set_reference_state(P, &R);
	CU_ASSERT_EQUAL_FATAL(res, 0);
	CU_ASSERT_TRUE_FATAL(solve_rho_from_Tp(P, g_ref.T, p0, &rho1));

	S1 = fprops_set_Trho(g_ref.T, rho1, P, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h1 = fprops_h(S1, &err);
	s1 = fprops_s(S1, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(fabs(h1 - h_target) <= g_ref.h_tol);
	CU_ASSERT_TRUE(fabs(s1 - s_target) <= g_ref.s_tol);

	fprops_fluid_destroy(P);
}

static void test_refstate_trhs_applies_target_helmholtz(void){
	PureFluid *P = prepare_helmholtz_n2();
	FpropsError err = FPROPS_NO_ERROR;
	FluidState2 S0 = fprops_set_Trho(g_ref.T, g_ref.rho_helm_seed, P, &err);
	double h0;
	double s0;
	double h_target;
	double s_target;
	double h1;
	double s1;
	ReferenceState R;
	int res;
	FluidState2 S1;

	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h0 = fprops_h(S0, &err);
	s0 = fprops_s(S0, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);

	h_target = h0 + 0.5 * g_ref.h_shift;
	s_target = s0 + 0.5 * g_ref.s_shift;
	R = (ReferenceState){FPROPS_REF_TRHS,{.trhs={g_ref.T, g_ref.rho_helm_seed, h_target, s_target}}};
	res = fprops_set_reference_state(P, &R);
	CU_ASSERT_EQUAL_FATAL(res, 0);

	S1 = fprops_set_Trho(g_ref.T, g_ref.rho_helm_seed, P, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h1 = fprops_h(S1, &err);
	s1 = fprops_s(S1, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(fabs(h1 - h_target) <= g_ref.h_tol);
	CU_ASSERT_TRUE(fabs(s1 - s_target) <= g_ref.s_tol);

	fprops_fluid_destroy(P);
}

static void test_pengrob_prepare_default_ref_fallback_survives(void){
	const EosData *E = fprops_eos("nitrogen", NULL, "RPP");
	PureFluid *P;
	FpropsError err = FPROPS_NO_ERROR;
	FluidState2 S;
	double h;
	double s;

	CU_ASSERT_PTR_NOT_NULL_FATAL(E);
	P = pengrob_prepare(E, NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(P);
	S = fprops_set_Trho(g_ref.T, g_ref.rho_seed, P, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h = fprops_h(S, &err);
	s = fprops_s(S, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(isfinite(h));
	CU_ASSERT_TRUE(isfinite(s));
	fprops_fluid_destroy(P);
}

static void test_refstate_matrix_expected_success_failure(void){
	PureFluid *Phelm = prepare_helmholtz_n2();
	PureFluid *Ppr = prepare_pengrob_n2();
	ReferenceState ref_phi0 = {FPROPS_REF_PHI0,{.phi0={0.0,0.0}}};
	ReferenceState ref_tphs = {FPROPS_REF_TPHS,{.tphs={g_ref.T, 1e5, 1e5, 1e3}}};
	ReferenceState ref_undef = {FPROPS_REF_UNDEFINED};
	PureFluid *Pideal = ideal_prepare(&eos_rpp_nitrogen, &ref_phi0);

	CU_ASSERT_PTR_NOT_NULL_FATAL(Pideal);

	/* Helmholtz and Peng-Robinson should accept TPHS and reject UNDEFINED. */
	CU_ASSERT_EQUAL(fprops_set_reference_state(Phelm, &ref_tphs), 0);
	CU_ASSERT_NOT_EQUAL(fprops_set_reference_state(Phelm, &ref_undef), 0);
	CU_ASSERT_EQUAL(fprops_set_reference_state(Ppr, &ref_tphs), 0);
	CU_ASSERT_NOT_EQUAL(fprops_set_reference_state(Ppr, &ref_undef), 0);

	/* Ideal currently supports only PHI0/REF0 paths in ideal_prepare. */
	CU_ASSERT_NOT_EQUAL(fprops_set_reference_state(Pideal, &ref_tphs), 0);

	fprops_fluid_destroy(Phelm);
	fprops_fluid_destroy(Ppr);
	/* no ideal_destroy yet; keep alive for process lifetime */
}

static void test_refstate_null_uses_ref0_path(void){
	PureFluid *P = prepare_pengrob_n2();
	FpropsError err = FPROPS_NO_ERROR;
	FluidState2 S;
	double h;
	double s;

	CU_ASSERT_EQUAL(fprops_set_reference_state(P, NULL), 0);
	S = fprops_set_Trho(g_ref.T, g_ref.rho_seed, P, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	h = fprops_h(S, &err);
	s = fprops_s(S, &err);
	CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
	CU_ASSERT_TRUE(isfinite(h));
	CU_ASSERT_TRUE(isfinite(s));

	fprops_fluid_destroy(P);
}

static void test_refstate_invalid_type_returns_error(void){
	PureFluid *P = prepare_pengrob_n2();
	ReferenceState ref_bad = {(ReferenceStateType)999,{.phi0={0.0,0.0}}};
	int res = fprops_set_reference_state(P, &ref_bad);
	CU_ASSERT_NOT_EQUAL(res, 0);
	fprops_fluid_destroy(P);
}

static void test_pengrob_prepare_explicit_bad_reference_fails(void){
	const EosData *E = fprops_eos("nitrogen", NULL, "RPP");
	ReferenceState ref_bad = {FPROPS_REF_UNDEFINED};
	PureFluid *P;
	CU_ASSERT_PTR_NOT_NULL_FATAL(E);
	P = pengrob_prepare(E, &ref_bad);
	CU_ASSERT_PTR_NULL(P);
}

static void test_ideal_prepare_default_ref_from_cubic_fails_cleanly(void){
	PureFluid *P = ideal_prepare(&eos_rpp_nitrogen, NULL);
	CU_ASSERT_PTR_NULL(P);
}

static void test_ideal_prepare_rejects_explicit_tphs(void){
	ReferenceState ref_tphs = {FPROPS_REF_TPHS,{.tphs={298.15,1e5,0.0,0.0}}};
	PureFluid *P = ideal_prepare(&eos_rpp_nitrogen, &ref_tphs);
	CU_ASSERT_PTR_NULL(P);
}

static void test_ideal_prepare_accepts_helmholtz_ref0(void){
	const EosData *Ehelm = fprops_eos("nitrogen", "helmholtz", NULL);
	ReferenceState ref_ref0 = {FPROPS_REF_REF0};
	PureFluid *P;
	CU_ASSERT_PTR_NOT_NULL_FATAL(Ehelm);
	P = ideal_prepare(Ehelm, &ref_ref0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(P);
	fprops_fluid_destroy(P);
}

static void test_rpp_water_cp0_filedata_matches_reference(void){
	const EosData *Erpp = fprops_eos("water", "ideal", "RPP");
	const PureFluid *Phelm = fprops_fluid("water", "helmholtz", NULL);
	const double temps[] = {298.15, 773.15};
	unsigned i;

	CU_ASSERT_PTR_NOT_NULL_FATAL(Erpp);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Phelm);
	CU_ASSERT_EQUAL_FATAL(Erpp->type, FPROPS_CUBIC);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Erpp->data.cubic->ideal);

	for(i = 0; i < sizeof(temps) / sizeof(temps[0]); ++i){
		double T = temps[i];
		double cp0_mass = cp0_cp(T, &Erpp->data.cubic->ideal->data.cp0);
		double cp0_molar = cp0_mass * Erpp->data.cubic->M / 1000.0;
		double rho_helm = 101325.0 / (Phelm->data->R * T);
		FpropsError err = FPROPS_NO_ERROR;
		double cp0_helm = to_molar(
			fprops_cp0(fprops_set_Trho(T, rho_helm, Phelm, &err), &err), Phelm
		);
		CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);
		if(i == 0){
			CU_ASSERT_TRUE(fabs(cp0_molar - 33.58) <= 0.2);
		}
		CU_ASSERT_TRUE(fabs(cp0_molar - cp0_helm) <= 1.0);
	}
}

static void test_rpp_hydrogen_cp0_filedata_matches_helmholtz(void){
	const EosData *Erpp = fprops_eos("hydrogen", "ideal", "RPP");
	const PureFluid *Phelm = fprops_fluid("hydrogen", "helmholtz", NULL);
	const double temps[] = {298.15, 773.15};
	unsigned i;

	CU_ASSERT_PTR_NOT_NULL_FATAL(Erpp);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Phelm);
	CU_ASSERT_EQUAL_FATAL(Erpp->type, FPROPS_CUBIC);
	CU_ASSERT_PTR_NOT_NULL_FATAL(Erpp->data.cubic->ideal);

	for(i = 0; i < sizeof(temps) / sizeof(temps[0]); ++i){
		double T = temps[i];
		double cp0_mass = cp0_cp(T, &Erpp->data.cubic->ideal->data.cp0);
		double cp0_molar = cp0_mass * Erpp->data.cubic->M / 1000.0;
		double rho_helm = 101325.0 / (Phelm->data->R * T);
		FpropsError err = FPROPS_NO_ERROR;
		double cp0_helm = to_molar(
			fprops_cp0(fprops_set_Trho(T, rho_helm, Phelm, &err), &err), Phelm
		);
		CU_ASSERT_EQUAL_FATAL(err, FPROPS_NO_ERROR);

		CU_ASSERT_TRUE(cp0_molar > 10.0 && cp0_molar < 60.0);
		CU_ASSERT_TRUE(fabs(cp0_molar - cp0_helm) <= 2.0);
	}
}

CU_ErrorCode test_register_refstate(void){
	CU_pSuite s = CU_add_suite("refstate", refstate_suite_init, refstate_suite_cleanup);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	if(NULL == CU_add_test(s, "tphs_applies_target_pengrob", test_refstate_tphs_applies_target_pengrob)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "tphg_applies_target_pengrob", test_refstate_tphg_applies_target_pengrob)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "pengrob_prepare_applies_reference", test_pengrob_prepare_applies_requested_reference)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "tphs_applies_target_helmholtz", test_refstate_tphs_applies_target_helmholtz)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "trhs_applies_target_helmholtz", test_refstate_trhs_applies_target_helmholtz)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "pengrob_prepare_default_ref_fallback_survives", test_pengrob_prepare_default_ref_fallback_survives)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "matrix_expected_success_failure", test_refstate_matrix_expected_success_failure)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "null_uses_ref0_path", test_refstate_null_uses_ref0_path)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "invalid_type_returns_error", test_refstate_invalid_type_returns_error)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "pengrob_prepare_explicit_bad_reference_fails", test_pengrob_prepare_explicit_bad_reference_fails)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "ideal_prepare_default_ref_from_cubic_fails_cleanly", test_ideal_prepare_default_ref_from_cubic_fails_cleanly)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "ideal_prepare_rejects_explicit_tphs", test_ideal_prepare_rejects_explicit_tphs)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "ideal_prepare_accepts_helmholtz_ref0", test_ideal_prepare_accepts_helmholtz_ref0)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "rpp_water_cp0_filedata_matches_reference", test_rpp_water_cp0_filedata_matches_reference)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "rpp_hydrogen_cp0_filedata_matches_helmholtz", test_rpp_hydrogen_cp0_filedata_matches_helmholtz)){
		return CUE_NOTEST;
	}
	return CUE_SUCCESS;
}
