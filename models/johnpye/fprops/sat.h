#ifndef FPROPS_SAT_H
#define FPROPS_SAT_H

#include "helmholtz.h"

double fprops_psat_T_xiang(double T, const HelmholtzData *d);
double fprops_psat_T_acentric(double T, const HelmholtzData *d);
double fprops_rhof_T_rackett(double T, const HelmholtzData *D);
double fprops_rhog_T_chouaieb(double T, const HelmholtzData *D);

int fprops_sat_T(double T, double *p_sat, double *rho_f, double *rho_g, const HelmholtzData *d);
int fprops_sat_p(double p, double *T_sat, double *rho_f, double *rho_g, const HelmholtzData *d);

double fprops_T_rhof_rackett(double rhof, const HelmholtzData *D);

int fprops_sat_hf(double hf, double *T_sat, double *p_sat, double *rho_f, double *rho_g, const HelmholtzData *d);

#endif

