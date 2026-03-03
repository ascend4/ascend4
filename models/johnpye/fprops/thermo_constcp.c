#include "thermo_constcp.h"

static double constcp_h_cb(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const ConstCpData *D = (const ConstCpData *)ctx;
	(void)p; (void)x;
	return constcp_h(T, D, err);
}

static double constcp_s_cb(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const ConstCpData *D = (const ConstCpData *)ctx;
	(void)p; (void)x;
	return constcp_s(T, D, err);
}

static double constcp_g_cb(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const ConstCpData *D = (const ConstCpData *)ctx;
	(void)x;
	return constcp_g(T, p, D, err);
}

static double constcp_mu_cb(const void *ctx, unsigned i, double T, double p, const double *x, FpropsError *err){
	(void)i;
	return constcp_g_cb(ctx, T, p, x, err);
}

static const ThermoModel constcp_model = {
	.h = constcp_h_cb,
	.s = constcp_s_cb,
	.g = constcp_g_cb,
	.mu = constcp_mu_cb,
	.sat_T = NULL
};

const ThermoModel *thermo_model_constcp(void){
	return &constcp_model;
}
