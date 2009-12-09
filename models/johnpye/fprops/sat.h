#ifndef FPROPS_SAT_H
#define FPROPS_SAT_H

#include "helmholtz.h"

double fprops_psat_T_xiang(double T, const HelmholtzData *d);

void phase_criterion(double T, double rho_f, double rho_g, double p_sat, const HelmholtzData *d);

#endif

