#include "shomate_data.h"

#include <string.h>

/* Piecewise Cp(T) correlations from models/johnpye/fprops/shomate.py
   (derived from OECD/NEA vol6-nickel selections), using generalized terms
   Cp,m = sum_k c_k T^(n_k). Term kinds are classified at data-prepare time. */

typedef struct {
	const char *name;
	const ShomateSpecies *species;
} ShomateEntry;

#define TERM(C, N) {(C), (N), SHOMATE_TERM_UNSET}

static const char *elements_ni[] = {"Ni"};
static const double stoich_ni[] = {1.0};

static const char *elements_nio[] = {"Ni", "O"};
static const double stoich_nio[] = {1.0, 1.0};

static ShomateTerm terms_ni_298_450[] = {
	TERM(3.36472e+01, 0.0),
	TERM(-2.47530e-02, 1.0),
	TERM(4.35458e-05, 2.0),
	TERM(-3.61955e+05, -2.0)
};

static ShomateTerm terms_ni_450_600[] = {
	TERM(-4.93235e+01, 0.0),
	TERM(1.676480e-01, 1.0),
	TERM(-7.48518e-05, 2.0),
	TERM(3.76214e+06, -2.0)
};

static ShomateTerm terms_ni_600_631[] = {
	TERM(1.05023e+05, 0.0),
	TERM(-2.29221e+02, 1.0),
	TERM(1.40772e-01, 2.0),
	TERM(-6.52809e+09, -2.0)
};

static ShomateTerm terms_ni_631_640[] = {
	TERM(5.29406e+04, 0.0),
	TERM(-1.65828e+02, 1.0),
	TERM(1.29936e-01, 2.0)
};

static ShomateTerm terms_ni_640_690[] = {
	TERM(1.82638e+02, 0.0),
	TERM(-4.20407e-01, 1.0),
	TERM(2.90521e-04, 2.0)
};

static ShomateTerm terms_ni_690_1728[] = {
	TERM(1.23677e+01, 0.0),
	TERM(2.22800e-02, 1.0),
	TERM(4.45300e-06, 2.0),
	TERM(2.46766e+06, -2.0)
};

static ShomateRange ranges_ni[] = {
	{298.0, 450.0, 4, terms_ni_298_450, 0},
	{450.0, 600.0, 4, terms_ni_450_600, 0},
	{600.0, 631.0, 4, terms_ni_600_631, 0},
	{631.0, 640.0, 3, terms_ni_631_640, 0},
	{640.0, 690.0, 3, terms_ni_640_690, 0},
	{690.0, 1728.0, 4, terms_ni_690_1728, 0}
};

static ShomateTerm terms_nio_298_519[] = {
	TERM(4110.64, 0.0),
	TERM(-5.302412, 1.0),
	TERM(3.52061e-3, 2.0),
	TERM(-53039.297, -0.5),
	TERM(2.43067e+07, -2.0)
};

static ShomateTerm terms_nio_519_1800[] = {
	TERM(-8.776, 0.0),
	TERM(4.2232e-2, 1.0),
	TERM(-7.5267e-6, 2.0),
	TERM(787.25, -0.5),
	TERM(3.6067e+06, -2.0)
};

static ShomateRange ranges_nio[] = {
	{298.15, 519.0, 5, terms_nio_298_519, 0},
	{519.0, 1800.0, 5, terms_nio_519_1800, 0}
};

static const ShomateSpecies species_ni = {
	"Ni",
	"oecd_nea_tdb_vol6_nickel",
	58.6934,
	FPROPS_PHASE_SOLID,
	298.15,
	101325.0,
	0.0,
	29.87,
	8908.0,
	1,
	elements_ni,
	stoich_ni,
	(unsigned)(sizeof(ranges_ni) / sizeof(ranges_ni[0])),
	ranges_ni
};

static const ShomateSpecies species_nio = {
	"NiO",
	"oecd_nea_tdb_vol6_nickel",
	74.6924,
	FPROPS_PHASE_SOLID,
	298.15,
	101325.0,
	-239700.0,
	38.4,
	6670.0,
	2,
	elements_nio,
	stoich_nio,
	(unsigned)(sizeof(ranges_nio) / sizeof(ranges_nio[0])),
	ranges_nio
};

static const ShomateEntry entries[] = {
	{"Ni", &species_ni},
	{"Ni(cr)", &species_ni},
	{"NiO", &species_nio},
	{"NiO(cr)", &species_nio}
};

static int entry_count(void){
	return (int)(sizeof(entries) / sizeof(entries[0]));
}

static void shomate_prepare_species_ranges(const ShomateSpecies *S){
	unsigned i;
	for(i = 0; i < S->nranges; ++i){
		shomate_prepare_range((ShomateRange *)&S->ranges[i]);
	}
}

const ShomateSpecies *shomate_data_lookup(const char *name, const char *source){
	int i;
	if(!name){
		return NULL;
	}
	for(i = 0; i < entry_count(); ++i){
		const ShomateSpecies *S = entries[i].species;
		if(0 != strcmp(entries[i].name, name)){
			continue;
		}
		if(source){
			if(!(S->source && NULL != strstr(S->source, source))){
				continue;
			}
		}
		shomate_prepare_species_ranges(S);
		return S;
	}
	return NULL;
}
