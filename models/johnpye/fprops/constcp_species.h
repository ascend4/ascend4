#ifndef FPROPS_CONSTCP_SPECIES_H
#define FPROPS_CONSTCP_SPECIES_H

#include "constcp.h"
#include "thermo.h"

typedef struct {
	const char *name;
	const char *source;
	double M; /* kg/kmol */
	unsigned nelem;
	const char **elements;
	const double *stoich;
	unsigned nphases;
	const ConstCpData *phases;
} ConstCpSpecies;

const ConstCpData *constcp_species_select_phase(const ConstCpSpecies *S, double T, double p,
		FpropsError *err);

double constcp_species_phase_g_molar(const ConstCpSpecies *S, const ConstCpData *phase,
		double T, double p, FpropsError *err);

double constcp_species_g_molar(const ConstCpSpecies *S, double T, double p,
		const ConstCpData **phase_out, FpropsError *err);

const ConstCpSpecies *constcp_species_lookup(const char *name, const char *source);

const ThermoModel *thermo_model_constcp_species(void);

#endif /* FPROPS_CONSTCP_SPECIES_H */
