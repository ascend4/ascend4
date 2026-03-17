#ifndef FPROPS_SHOMATE_SPECIES_H
#define FPROPS_SHOMATE_SPECIES_H

#include "shomate.h"
#include "thermo.h"

typedef struct {
	const char *name;
	const char *source;
	double M; /* kg/kmol */
	FpropsPhase phase;
	double T_ref; /* K */
	double p_ref; /* Pa */
	double h_ref_molar; /* J/mol at T_ref */
	double s_ref_molar; /* J/mol/K at T_ref */
	double rho_ref; /* kg/m3, optional pressure correction for condensed phases */
	unsigned nelem;
	const char **elements;
	const double *stoich;
	unsigned nranges;
	const ShomateRange *ranges;
} ShomateSpecies;

const ShomateSpecies *shomate_species_lookup(const char *name, const char *source);

double shomate_species_h_molar(const ShomateSpecies *S, double T, FpropsError *err);
double shomate_species_s_molar(const ShomateSpecies *S, double T, FpropsError *err);
double shomate_species_g_molar(const ShomateSpecies *S, double T, double p, FpropsError *err);

#endif /* FPROPS_SHOMATE_SPECIES_H */
