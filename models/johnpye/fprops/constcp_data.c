#include "constcp_data.h"

#include <string.h>

typedef struct {
	const char *name;
	const ConstCpSpecies *species;
} ConstCpEntry;

static const char *elements_fe[] = {"Fe"};
static const double stoich_fe[] = {1.0};

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
