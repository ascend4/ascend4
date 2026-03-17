#ifndef FPROPS_MIXTURE_H
#define FPROPS_MIXTURE_H

#include "rundata.h"

typedef struct FpropsMix_struct{
	const PureFluid **species; /* array of pointers, not owned */
	unsigned nspecies;
	double *x;                 /* mole fractions */
	double Mmix;               /* kg/kmol */
	double Rmix;               /* J/kg/K */
} FpropsMix;

/**
	Create an ideal-gas mixture object from a list of species.
	All properties are mass-basis (J/kg, J/kg/K) for the mixture.
*/
FpropsMix *fprops_mix_create(const PureFluid **species, unsigned nspecies, FpropsError *err);
void fprops_mix_destroy(FpropsMix *mix);
FpropsMix *fprops_mix_from_pure(const PureFluid *species, FpropsError *err);

/**
	Set mole fractions (will be normalised).
*/
int fprops_mix_set_x(FpropsMix *mix, const double *x, FpropsError *err);

double fprops_mix_h_ig(const FpropsMix *mix, double T, FpropsError *err);
double fprops_mix_cp_ig(const FpropsMix *mix, double T, FpropsError *err);
double fprops_mix_s_ig(const FpropsMix *mix, double T, double p, FpropsError *err);
double fprops_mix_g_ig(const FpropsMix *mix, double T, double p, FpropsError *err);

/**
	Ideal-gas chemical potential for species i, returned on a mass basis.
*/
double fprops_mix_mu_ig(const FpropsMix *mix, unsigned i, double T, double p, FpropsError *err);

#endif /* FPROPS_MIXTURE_H */
