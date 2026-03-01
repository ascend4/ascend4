#include "shomate_data.h"

#include <string.h>

/* Piecewise Cp(T) correlations represented as generalized terms
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

static const char *elements_h2[] = {"H"};
static const double stoich_h2[] = {2.0};

static const char *elements_o2[] = {"O"};
static const double stoich_o2[] = {2.0};

static const char *elements_h2o[] = {"H", "O"};
static const double stoich_h2o[] = {2.0, 1.0};

/* ------------------------------------------------------------------------- */
/* OECD/NEA Ni/NiO source (from shomate.py)                                 */
/* ------------------------------------------------------------------------- */

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

static ShomateRange ranges_ni_oecd[] = {
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

static ShomateRange ranges_nio_oecd[] = {
	{298.15, 519.0, 5, terms_nio_298_519, 0},
	{519.0, 1800.0, 5, terms_nio_519_1800, 0}
};

static const ShomateSpecies species_ni_oecd = {
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
	(unsigned)(sizeof(ranges_ni_oecd) / sizeof(ranges_ni_oecd[0])),
	ranges_ni_oecd
};

static const ShomateSpecies species_nio_oecd = {
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
	(unsigned)(sizeof(ranges_nio_oecd) / sizeof(ranges_nio_oecd[0])),
	ranges_nio_oecd
};

/* ------------------------------------------------------------------------- */
/* Reaktoro/SUPCRT98 clone source                                            */
/* ------------------------------------------------------------------------- */

/* Clone fits target Reaktoro G0(T,1 bar) directly over 298.15..1473.15 K.
   Basis exponents: 0, 1, 2, 3, -0.5, -1, -2. */

static ShomateTerm terms_ni_clone_298_1800[] = {
	TERM(-3.619194026497e+01, 0.0),
	TERM( 1.241293180093e-01, 1.0),
	TERM(-1.216044158751e-04, 2.0),
	TERM( 4.441382623836e-08, 3.0),
	TERM( 6.711517327106e+02,-0.5),
	TERM( 9.852689883015e+01,-1.0),
	TERM( 8.206710463358e-01,-2.0)
};

static ShomateTerm terms_nio_clone_298_1800[] = {
	TERM( 1.360284121488e+03, 0.0),
	TERM(-1.795161933881e+00, 1.0),
	TERM( 1.379159651796e-03, 2.0),
	TERM(-3.951238497373e-07, 3.0),
	TERM(-1.554398257140e+04,-0.5),
	TERM(-2.285198010674e+03,-1.0),
	TERM(-1.904793345042e+01,-2.0)
};

static ShomateTerm terms_h2_clone_298_1800[] = {
	TERM(-2.215833589272e+02, 0.0),
	TERM( 3.391787296453e-01, 1.0),
	TERM(-2.574450536187e-04, 2.0),
	TERM( 7.455618368642e-08, 3.0),
	TERM( 3.013873636648e+03,-0.5),
	TERM( 4.428180598697e+02,-1.0),
	TERM( 3.689949935253e+00,-2.0)
};

static ShomateTerm terms_o2_clone_298_1800[] = {
	TERM(-3.656827987000e+02, 0.0),
	TERM( 5.452130392397e-01, 1.0),
	TERM(-4.172656002975e-04, 2.0),
	TERM( 1.212840625870e-07, 3.0),
	TERM( 4.727688796114e+03,-0.5),
	TERM( 6.947373770964e+02,-1.0),
	TERM( 5.789633845367e+00,-2.0)
};

static ShomateTerm terms_h2o_clone_298_1800[] = {
	TERM( 1.156956899016e+03, 0.0),
	TERM(-1.540126605509e+00, 1.0),
	TERM( 1.149410513394e-03, 2.0),
	TERM(-3.332796318440e-07, 3.0),
	TERM(-1.331076498921e+04,-0.5),
	TERM(-1.956831721504e+03,-1.0),
	TERM(-1.631067917681e+01,-2.0)
};

static ShomateRange ranges_ni_clone[] = {
	{298.15, 1800.0, 7, terms_ni_clone_298_1800, 0}
};

static ShomateRange ranges_nio_clone[] = {
	{298.15, 1800.0, 7, terms_nio_clone_298_1800, 0}
};

static ShomateRange ranges_h2_clone[] = {
	{298.15, 1800.0, 7, terms_h2_clone_298_1800, 0}
};

static ShomateRange ranges_o2_clone[] = {
	{298.15, 1800.0, 7, terms_o2_clone_298_1800, 0}
};

static ShomateRange ranges_h2o_clone[] = {
	{298.15, 1800.0, 7, terms_h2o_clone_298_1800, 0}
};

static const ShomateSpecies species_ni_clone = {
	"Ni",
	"reaktoro_clone_supcrt98",
	58.6934,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	8610.20295247878,
	28.958110111384237,
	8909.13782635,
	1,
	elements_ni,
	stoich_ni,
	(unsigned)(sizeof(ranges_ni_clone) / sizeof(ranges_ni_clone[0])),
	ranges_ni_clone
};

static const ShomateSpecies species_nio_clone = {
	"NiO",
	"reaktoro_clone_supcrt98",
	74.6928,
	FPROPS_PHASE_SOLID,
	298.15,
	100000.0,
	-199963.32247670897,
	38.981778595801316,
	6808.82406563,
	2,
	elements_nio,
	stoich_nio,
	(unsigned)(sizeof(ranges_nio_clone) / sizeof(ranges_nio_clone[0])),
	ranges_nio_clone
};

static const ShomateSpecies species_o2_clone = {
	"oxygen",
	"reaktoro_clone_supcrt98",
	31.9988,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	60767.61453857804,
	203.89671925355952,
	0.0,
	1,
	elements_o2,
	stoich_o2,
	(unsigned)(sizeof(ranges_o2_clone) / sizeof(ranges_o2_clone[0])),
	ranges_o2_clone
};

static const ShomateSpecies species_h2_clone = {
	"hydrogen",
	"reaktoro_clone_supcrt98",
	2.01588,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	38726.897934772845,
	129.93944076364238,
	0.0,
	1,
	elements_h2,
	stoich_h2,
	(unsigned)(sizeof(ranges_h2_clone) / sizeof(ranges_h2_clone[0])),
	ranges_h2_clone
};

static const ShomateSpecies species_h2o_clone = {
	"water",
	"reaktoro_clone_supcrt98",
	18.01528,
	FPROPS_PHASE_GAS,
	298.15,
	100000.0,
	-171226.63495885613,
	190.64104147064737,
	0.0,
	2,
	elements_h2o,
	stoich_h2o,
	(unsigned)(sizeof(ranges_h2o_clone) / sizeof(ranges_h2o_clone[0])),
	ranges_h2o_clone
};

static const ShomateEntry entries[] = {
	/* OECD source */
	{"Ni", &species_ni_oecd},
	{"Ni(cr)", &species_ni_oecd},
	{"NiO", &species_nio_oecd},
	{"NiO(cr)", &species_nio_oecd},

	/* Reaktoro clone source */
	{"Ni", &species_ni_clone},
	{"Ni(cr)", &species_ni_clone},
	{"NiO", &species_nio_clone},
	{"NiO(cr)", &species_nio_clone},
	{"oxygen", &species_o2_clone},
	{"hydrogen", &species_h2_clone},
	{"water", &species_h2o_clone}
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
