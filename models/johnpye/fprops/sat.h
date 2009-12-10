#ifndef FPROPS_SAT_H
#define FPROPS_SAT_H

#include "helmholtz.h"

double fprops_psat_T_xiang(double T, const HelmholtzData *d);

void phase_criterion(
	double T, double rho_f, double rho_g, double p_sat, double *eq1
	, double *eq2, double *eq3, const HelmholtzData *d
);

double fprops_vf_T_rackett(double T, const HelmholtzData *D);
double fprops_vg_T_chouaieb(double T, const HelmholtzData *D);

void solve_saturation(double T, HelmholtzData *D);

#endif

