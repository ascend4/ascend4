#ifndef FPROPS_SAT2_H
#define FPROPS_SAT2_H

#include "helmholtz.h"

/**
	solve saturation properties in terms of temperature
*/
int fprops_sat_T(double T, double *p_sat, double *rho_f, double *rho_g, const HelmholtzData *d);

/**
	solve saturation properties in terms of pressure
	FIXME still need to implement this!
*/
int fprops_sat_p(double p, double *T_sat, double *rho_f, double *rho_g, const HelmholtzData *d);


#define FPROPS_PHASE_LIQUID 'L'
#define FPROPS_PHASE_VAPOUR 'V'
int fprops_rho_pT(double p, double T, char phase, char use_guess, const HelmholtzData *d, double *rho);


#endif

