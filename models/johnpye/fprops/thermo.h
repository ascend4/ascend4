#ifndef FPROPS_THERMO_H
#define FPROPS_THERMO_H

#include "rundata.h"

typedef enum {
	FPROPS_PHASE_GAS,
	FPROPS_PHASE_LIQUID,
	FPROPS_PHASE_SOLID
} FpropsPhase;

typedef struct ThermoModel_struct ThermoModel;

typedef struct {
	const ThermoModel *model;
	const void *model_data;
	FpropsPhase phase;
	double T;
	double p;
	const double *x; /* mole fractions, or NULL for pure */
	unsigned n;
} ThermoState;

struct ThermoModel_struct{
	double (*h)(const void *ctx, double T, double p, const double *x, FpropsError *err);
	double (*s)(const void *ctx, double T, double p, const double *x, FpropsError *err);
	double (*g)(const void *ctx, double T, double p, const double *x, FpropsError *err);
	double (*mu)(const void *ctx, unsigned i, double T, double p, const double *x, FpropsError *err);

	/* optional: saturation for pure species */
	double (*sat_T)(const void *ctx, double T, double *rhof, double *rhog, FpropsError *err);
};

double thermo_h(const ThermoState *S, FpropsError *err);
double thermo_s(const ThermoState *S, FpropsError *err);
double thermo_g(const ThermoState *S, FpropsError *err);
double thermo_mu(const ThermoState *S, unsigned i, FpropsError *err);
double thermo_sat_T(const ThermoState *S, double T, double *rhof, double *rhog, FpropsError *err);

#endif /* FPROPS_THERMO_H */
