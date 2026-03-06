#ifndef FPROPS_GIBBS_SPECIES_H
#define FPROPS_GIBBS_SPECIES_H

#include "rundata.h"

typedef struct {
	const char *name;
	const char *source;
	unsigned nelem;
	const char **elements;
	const double *stoich;
	FpropsError (*g_molar_fn)(double T, double p, double *g_out);
} GibbsSpecies;

const GibbsSpecies *gibbs_species_lookup(const char *name, const char *source);
int gibbs_species_g_molar(const GibbsSpecies *S, double T, double p, double *g_out);
int gibbs_species_h_molar(const GibbsSpecies *S, double T, double p, double *h_out);

#endif /* FPROPS_GIBBS_SPECIES_H */
