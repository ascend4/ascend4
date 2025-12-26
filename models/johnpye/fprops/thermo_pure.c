#include "thermo_pure.h"
#include "fprops.h"
#include "solve_pT.h"
#include "sat.h"

static double pure_h(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const PureFluid *P = (const PureFluid *)ctx;
	(void)x;
	if(!P){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	{
		FluidState2 S = fprops_solve_pT(p, T, P, err);
		if(*err)return 0;
		return fprops_h(S, err);
	}
}

static double pure_s(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const PureFluid *P = (const PureFluid *)ctx;
	(void)x;
	if(!P){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	{
		FluidState2 S = fprops_solve_pT(p, T, P, err);
		if(*err)return 0;
		return fprops_s(S, err);
	}
}

static double pure_g(const void *ctx, double T, double p, const double *x, FpropsError *err){
	const PureFluid *P = (const PureFluid *)ctx;
	(void)x;
	if(!P){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	{
		FluidState2 S = fprops_solve_pT(p, T, P, err);
		if(*err)return 0;
		return fprops_g(S, err);
	}
}

static double pure_mu(const void *ctx, unsigned i, double T, double p, const double *x, FpropsError *err){
	(void)i;
	return pure_g(ctx, T, p, x, err);
}

static double pure_sat_T(const void *ctx, double T, double *rhof, double *rhog, FpropsError *err){
	const PureFluid *P = (const PureFluid *)ctx;
	double psat;
	if(!P){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	fprops_sat_T(T, &psat, rhof, rhog, P, err);
	return psat;
}

static const ThermoModel pure_model = {
	.h = pure_h,
	.s = pure_s,
	.g = pure_g,
	.mu = pure_mu,
	.sat_T = pure_sat_T
};

const ThermoModel *thermo_model_pure(void){
	return &pure_model;
}
