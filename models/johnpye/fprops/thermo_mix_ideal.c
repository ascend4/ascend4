#include "thermo_mix_ideal.h"

static double mix_h_cb(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const FpropsMix *mix = (const FpropsMix *)ctx;
	(void)p; (void)x;
	return fprops_mix_h_ig(mix, T, err);
}

static double mix_s_cb(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const FpropsMix *mix = (const FpropsMix *)ctx;
	(void)x;
	return fprops_mix_s_ig(mix, T, p, err);
}

static double mix_g_cb(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const FpropsMix *mix = (const FpropsMix *)ctx;
	(void)x;
	return fprops_mix_g_ig(mix, T, p, err);
}

static double mix_mu_cb(const void *ctx, unsigned i, double T, double p, const double *x, FpropsError *err){
	const FpropsMix *mix = (const FpropsMix *)ctx;
	(void)x;
	return fprops_mix_mu_ig(mix, i, T, p, err);
}

static const ThermoModel mix_ideal_model = {
	.h = mix_h_cb,
	.s = mix_s_cb,
	.g = mix_g_cb,
	.mu = mix_mu_cb,
	.sat_T = NULL
};

const ThermoModel *thermo_model_mix_ideal(void){
	return &mix_ideal_model;
}
