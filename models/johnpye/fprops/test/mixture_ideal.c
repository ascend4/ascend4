#include "../test.h"
#include "../mixture.h"
#include "../fluids.h"
#include "../ideal.h"
#include "../thermo.h"
#include "../thermo_pure.h"
#include "../constcp.h"
#include "../thermo_constcp.h"
#include "../thermo_mix_ideal.h"

#include <math.h>

static void test_mix_ideal_basic(void){
	FpropsError err = FPROPS_NO_ERROR;
	const PureFluid *n2 = fprops_fluid("nitrogen",NULL,NULL);
	const PureFluid *o2 = fprops_fluid("oxygen",NULL,NULL);
	const PureFluid *species[2];
	FpropsMix *mix;
	double T = 300.0;
	double p = 1e5;
	double x[2] = {0.79, 0.21};

	ASSERT(n2 != NULL);
	ASSERT(o2 != NULL);

	species[0] = n2;
	species[1] = o2;
	mix = fprops_mix_create(species, 2, &err);
	ASSERT(err == FPROPS_NO_ERROR);
	ASSERT(mix != NULL);
	ASSERT(fprops_mix_set_x(mix, x, &err) == 0);
	ASSERT(err == FPROPS_NO_ERROR);

	{
		double h = fprops_mix_h_ig(mix, T, &err);
		double cp = fprops_mix_cp_ig(mix, T, &err);
		double s = fprops_mix_s_ig(mix, T, p, &err);
		double g = fprops_mix_g_ig(mix, T, p, &err);
		ASSERT(err == FPROPS_NO_ERROR);
		ASSERT(isfinite(h));
		ASSERT(isfinite(cp));
		ASSERT(isfinite(s));
		ASSERT(isfinite(g));
	}

	{
		double Mmix = mix->Mmix;
		double hmix = fprops_mix_h_ig(mix, T, &err);
		double hsum = 0.0;
		unsigned i;
		ASSERT(err == FPROPS_NO_ERROR);
		for(i = 0; i < 2; ++i){
			const PureFluid *P = species[i];
			double Mi = P->data->M;
			double wi = x[i] * Mi / Mmix;
			double hi = ideal_h((FluidStateUnion){.Trho={T,1.0}}, P->data, &err);
			ASSERT(err == FPROPS_NO_ERROR);
			hsum += wi * hi;
		}
		ASSERT(fabs(hmix - hsum) < 1e-6 * (fabs(hsum) + 1.0));
	}

	fprops_mix_destroy(mix);
	fprops_fluid_destroy((PureFluid *)n2);
	fprops_fluid_destroy((PureFluid *)o2);
}

static void test_pure_model_sat(void){
	FpropsError err = FPROPS_NO_ERROR;
	const PureFluid *co2 = fprops_fluid("carbondioxide","pengrob",NULL);
	ASSERT(co2 != NULL);

	{
		ThermoState S = thermo_state_pure(co2, 280.0, 1e5);
		double rhof = 0.0, rhog = 0.0;
		double psat = thermo_sat_T(&S, 280.0, &rhof, &rhog, &err);
		ASSERT(err == FPROPS_NO_ERROR);
		ASSERT(psat > 0);
		ASSERT(rhof > rhog);
		ASSERT(rhof > 0 && rhog > 0);
	}

	fprops_fluid_destroy((PureFluid *)co2);
}

static void test_constcp_model(void){
	FpropsError err = FPROPS_NO_ERROR;
	ConstCpData *D = constcp_create(FPROPS_CONSTCP_SOLID, 55.845, 750.0, 298.15
		, 0.0, 0.0, 7800.0, &err
	);
	ASSERT(err == FPROPS_NO_ERROR);
	ASSERT(D != NULL);
	{
		ThermoState S = thermo_state_constcp(D, 1000.0, 1e5);
		double h = thermo_h(&S, &err);
		double s = thermo_s(&S, &err);
		double g = thermo_g(&S, &err);
		ASSERT(err == FPROPS_NO_ERROR);
		ASSERT(isfinite(h));
		ASSERT(isfinite(s));
		ASSERT(isfinite(g));
	}
	constcp_destroy(D);
}

static void test_mix_model_ideal(void){
	FpropsError err = FPROPS_NO_ERROR;
	const PureFluid *n2 = fprops_fluid("nitrogen",NULL,NULL);
	const PureFluid *o2 = fprops_fluid("oxygen",NULL,NULL);
	const PureFluid *species[2];
	FpropsMix *mix;
	double T = 350.0;
	double p = 2e5;
	double x[2] = {0.7, 0.3};

	ASSERT(n2 != NULL);
	ASSERT(o2 != NULL);

	species[0] = n2;
	species[1] = o2;
	mix = fprops_mix_create(species, 2, &err);
	ASSERT(err == FPROPS_NO_ERROR);
	ASSERT(mix != NULL);
	ASSERT(fprops_mix_set_x(mix, x, &err) == 0);
	ASSERT(err == FPROPS_NO_ERROR);

	{
		ThermoState S = thermo_state_mix_ideal(mix, T, p);
		double h = thermo_h(&S, &err);
		double s = thermo_s(&S, &err);
		double g = thermo_g(&S, &err);
		double mu0 = thermo_mu(&S, 0, &err);
		ASSERT(err == FPROPS_NO_ERROR);
		ASSERT(isfinite(h));
		ASSERT(isfinite(s));
		ASSERT(isfinite(g));
		ASSERT(isfinite(mu0));
	}

	fprops_mix_destroy(mix);
	fprops_fluid_destroy((PureFluid *)n2);
	fprops_fluid_destroy((PureFluid *)o2);
}

static void test_pure_as_mix(void){
	FpropsError err = FPROPS_NO_ERROR;
	const PureFluid *n2 = fprops_fluid("nitrogen",NULL,NULL);
	ASSERT(n2 != NULL);
	{
		ThermoState S = thermo_state_pure_as_mix(n2, 300.0, 1e5, &err);
		ASSERT(err == FPROPS_NO_ERROR);
		{
			double h = thermo_h(&S, &err);
			ASSERT(err == FPROPS_NO_ERROR);
			ASSERT(isfinite(h));
		}
		fprops_mix_destroy((FpropsMix *)S.model_data);
	}
	fprops_fluid_destroy((PureFluid *)n2);
}

CU_ErrorCode test_register_mix_ideal(void){
	CU_pSuite s = CU_add_suite("mix_ideal",NULL,NULL);
	if(NULL == s){
		return CUE_NOSUITE;
	}
	if(NULL == CU_add_test(s, "mix_ideal_basic", test_mix_ideal_basic)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "pure_model_sat", test_pure_model_sat)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "constcp_model", test_constcp_model)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "mix_model_ideal", test_mix_model_ideal)){
		return CUE_NOTEST;
	}
	if(NULL == CU_add_test(s, "pure_as_mix", test_pure_as_mix)){
		return CUE_NOTEST;
	}
	return CUE_SUCCESS;
}
