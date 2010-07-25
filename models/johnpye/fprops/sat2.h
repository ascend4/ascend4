#ifndef FPROPS_SAT2_H
#define FPROPS_SAT2_H

#include "helmholtz.h"

double fprops_sat_succsubs(double T, double *rhof_out, double *rhog_out, const HelmholtzData *d, int *err);


#define FPROPS_PHASE_LIQUID 'L'
#define FPROPS_PHASE_VAPOUR 'V'

int fprops_rho_pT(double p, double T, char phase, char use_guess, const HelmholtzData *d, double *rho);


#endif

