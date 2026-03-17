#ifndef FPROPS_CONSTCP_H
#define FPROPS_CONSTCP_H

#include "rundata.h"

typedef enum {
	FPROPS_CONSTCP_SOLID,
	FPROPS_CONSTCP_LIQUID
} ConstCpPhase;

typedef struct {
	ConstCpPhase phase;
	const char *phase_id; /* optional label, eg "alpha", "beta" */
	double M;     /* kg/kmol */
	double cp;    /* J/kg/K */
	double h_ref; /* J/kg at T_ref */
	double s_ref; /* J/kg/K at T_ref */
	double T_ref; /* K */
	double p_ref; /* Pa */
	double rho;   /* kg/m3 (optional, may be 0 if unused) */
} ConstCpData;

ConstCpData *constcp_create(ConstCpPhase phase, const char *phase_id, double M, double cp
	, double T_ref, double p_ref, double h_ref, double s_ref, double rho, FpropsError *err
);
void constcp_destroy(ConstCpData *D);

double constcp_h(double T, const ConstCpData *D, FpropsError *err);
double constcp_s(double T, const ConstCpData *D, FpropsError *err);
double constcp_g(double T, double p, const ConstCpData *D, FpropsError *err);

#endif /* FPROPS_CONSTCP_H */
