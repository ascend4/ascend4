#ifndef FPROPS_SPECIES_H
#define FPROPS_SPECIES_H

#include "rundata.h"
#include "thermo.h"
#include "constcp_species.h"

typedef enum {
	FPROPS_SPECIES_EOS = 0,
	FPROPS_SPECIES_CONSTCP,
	FPROPS_SPECIES_DERIVED_CONSTCP
} FpropsSpeciesKind;

typedef struct {
	const char *name;
	const char *source;
	const char *modeltype;
	const char *corrtype;
	FpropsSpeciesKind kind;
	const ThermoModel *model;
	void *model_data;
	const EosData *eos;
	const ConstCpSpecies *constcp;
} FpropsSpecies;

FpropsSpecies *fprops_species_lookup(const char *name, const char *source,
		const char *modeltype, const char *corrtype, FpropsError *err);

void fprops_species_destroy(FpropsSpecies *S);

const char *fprops_species_kind_name(FpropsSpeciesKind kind);

#endif /* FPROPS_SPECIES_H */
