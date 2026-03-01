#include "constcp_data.h"

#include <string.h>

/*
 * Data provenance note (Ni-O-H extension):
 * Ni/NiO constant-cp entries added in this file are derived from
 * OECD/NEA Thermochemical Database, Nickel volume:
 * https://www.oecd-nea.org/dbtdb/pubs/vol6-nickel.pdf
 */

typedef struct {
	const char *name;
	const ConstCpSpecies *species;
} ConstCpEntry;

static const char *elements_fe[] = {"Fe"};
static const double stoich_fe[] = {1.0};

static const char *elements_ni[] = {"Ni"};
static const double stoich_ni[] = {1.0};

static const char *elements_nio[] = {"Ni", "O"};
static const double stoich_nio[] = {1.0, 1.0};

static const char *elements_feo[] = {"Fe", "O"};
static const double stoich_feo[] = {1.0, 1.0};

static const char *elements_fe2o3[] = {"Fe", "O"};
static const double stoich_fe2o3[] = {2.0, 3.0};

static const char *elements_fe3o4[] = {"Fe", "O"};
static const double stoich_fe3o4[] = {3.0, 4.0};

static const char *elements_h2o[] = {"H", "O"};
static const double stoich_h2o[] = {2.0, 1.0};

static const ConstCpData phases_fe[] = {
	{FPROPS_CONSTCP_SOLID, "solid", 55.845, 450.0, 0.0, 0.0, 298.15, 101325.0, 7874.0},
	{FPROPS_CONSTCP_LIQUID, "liquid", 55.845, 800.0, 0.0, 0.0, 298.15, 101325.0, 7000.0}
};

/* OECD/NEA TDB Nickel volume (vol6-nickel.pdf), converted via shomate.py */
static const ConstCpData phases_ni[] = {
	{FPROPS_CONSTCP_SOLID, "cr", 58.6934, 720.1416192194781, 0.0, 508.9158235849346,
		298.15, 101325.0, 8908.0}
};

static const ConstCpData phases_nio[] = {
	{FPROPS_CONSTCP_SOLID, "cr", 74.6924, 732.3013022737122, -3209144.656513077, 514.1057772636719,
		298.15, 101325.0, 6670.0}
};

static const ConstCpData phases_feo[] = {
	{FPROPS_CONSTCP_SOLID, "solid", 71.844, 650.0, 0.0, 0.0, 298.15, 101325.0, 5700.0}
};

static const ConstCpData phases_fe2o3[] = {
	{FPROPS_CONSTCP_SOLID, "solid", 159.687, 750.0, 0.0, 0.0, 298.15, 101325.0, 5250.0}
};

static const ConstCpData phases_fe3o4[] = {
	{FPROPS_CONSTCP_SOLID, "solid", 231.533, 800.0, 0.0, 0.0, 298.15, 101325.0, 5200.0}
};

static const ConstCpData phases_h2o_liq[] = {
	{FPROPS_CONSTCP_LIQUID, "liquid", 18.015, 4180.0, 0.0, 0.0, 298.15, 101325.0, 997.0}
};

static const ConstCpSpecies species_fe = {
	"Fe", "ellingham_placeholder", 55.845,
	1, elements_fe, stoich_fe,
	2, phases_fe
};

static const ConstCpSpecies species_ni = {
	"Ni", "oecd_nea_tdb_vol6_nickel", 58.6934,
	1, elements_ni, stoich_ni,
	1, phases_ni
};

static const ConstCpSpecies species_nio = {
	"NiO", "oecd_nea_tdb_vol6_nickel", 74.6924,
	2, elements_nio, stoich_nio,
	1, phases_nio
};

static const ConstCpSpecies species_feo = {
	"FeO", "ellingham_placeholder", 71.844,
	2, elements_feo, stoich_feo,
	1, phases_feo
};

static const ConstCpSpecies species_fe2o3 = {
	"Fe2O3", "ellingham_placeholder", 159.687,
	2, elements_fe2o3, stoich_fe2o3,
	1, phases_fe2o3
};

static const ConstCpSpecies species_fe3o4 = {
	"Fe3O4", "ellingham_placeholder", 231.533,
	2, elements_fe3o4, stoich_fe3o4,
	1, phases_fe3o4
};

static const ConstCpSpecies species_h2o_l = {
	"H2O(l)", "ellingham_placeholder", 18.015,
	2, elements_h2o, stoich_h2o,
	1, phases_h2o_liq
};

static const ConstCpEntry entries[] = {
	{"Fe", &species_fe},
	{"Ni", &species_ni},
	{"Ni(cr)", &species_ni},
	{"NiO", &species_nio},
	{"NiO(cr)", &species_nio},
	{"FeO", &species_feo},
	{"Fe2O3", &species_fe2o3},
	{"Fe3O4", &species_fe3o4},
	{"H2O(l)", &species_h2o_l}
};

static int entry_count(void){
	return (int)(sizeof(entries) / sizeof(entries[0]));
}

const ConstCpSpecies *constcp_data_lookup(const char *name, const char *source){
	int i;
	if(!name){
		return NULL;
	}
	for(i = 0; i < entry_count(); ++i){
		const ConstCpSpecies *S = entries[i].species;
		if(0 == strcmp(entries[i].name, name)){
			if(source){
				if(S->source && NULL != strstr(S->source, source)){
					return S;
				}
				continue;
			}
			return S;
		}
	}
	return NULL;
}

int constcp_data_build_element_matrix(const char **names, int ns, const char **elements, int ne,
		const char *source, double *A_out){
	int i;
	int e;
	int k;

	if(!names || !elements || !A_out || ns <= 0 || ne <= 0){
		return 0;
	}

	for(e = 0; e < ne; ++e){
		for(i = 0; i < ns; ++i){
			A_out[e * ns + i] = 0.0;
		}
	}

	for(i = 0; i < ns; ++i){
		const ConstCpSpecies *S = constcp_data_lookup(names[i], source);
		if(!S){
			return 0;
		}
		if(!S->elements || !S->stoich || S->nelem == 0){
			return 0;
		}
		for(k = 0; k < (int)S->nelem; ++k){
			for(e = 0; e < ne; ++e){
				if(0 == strcmp(S->elements[k], elements[e])){
					A_out[e * ns + i] += S->stoich[k];
				}
			}
		}
	}

	return 1;
}
