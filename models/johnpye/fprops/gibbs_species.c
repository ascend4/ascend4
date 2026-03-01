#include "gibbs_species.h"

#include <math.h>
#include <string.h>

typedef struct {
	const char *name;
	const GibbsSpecies *species;
} GibbsSpeciesEntry;

static const char *elements_fe[] = {"Fe"};
static const double stoich_fe[] = {1.0};

static const char *elements_fe3o4[] = {"Fe", "O"};
static const double stoich_fe3o4[] = {3.0, 4.0};

static const char *elements_fe2o3[] = {"Fe", "O"};
static const double stoich_fe2o3[] = {2.0, 3.0};

static double gibbs_species_R(void){
	return 8.31446261815324;
}

static double hillert_jarl_A(double p){
	return 518.0 / 1125.0 + (11692.0 / 15975.0) * (1.0 / p - 1.0);
}

static FpropsError hillert_jarl_gmag(double T, double Tord, double beta, double p, double *g_out){
	double tau, A, f;
	if(!g_out || !(T > 0.0) || !(Tord > 0.0) || !(beta > 0.0) || !(p > 0.0)){
		return FPROPS_RANGE_ERROR;
	}
	tau = T / Tord;
	A = hillert_jarl_A(p);
	if(!(A > 0.0)){
		return FPROPS_RANGE_ERROR;
	}
	if(tau <= 1.0){
		double poly = tau * tau * tau / 6.0
			+ pow(tau, 9.0) / 135.0
			+ pow(tau, 15.0) / 600.0;
		f = 1.0 - (
			(79.0 / (140.0 * p)) / tau
			+ (474.0 / 497.0) * (1.0 / p - 1.0) * poly
		) / A;
	}else{
		f = -(
			pow(tau, -5.0) / 10.0
			+ pow(tau, -15.0) / 315.0
			+ pow(tau, -25.0) / 1500.0
		) / A;
	}
	*g_out = f * gibbs_species_R() * T * log(beta + 1.0);
	return isfinite(*g_out) ? FPROPS_NO_ERROR : FPROPS_RANGE_ERROR;
}

static FpropsError gibbs_fe_bcc(double T, double p, double *g_out){
	double gmag = 0.0;
	(void)p;
	if(!g_out || !(T >= 298.0) || !(T <= 1811.0)){
		return FPROPS_RANGE_ERROR;
	}
	*g_out = 10375.2
		+ 114.5502 * T
		- 23.5143 * T * log(T)
		- 0.004398 * T * T
		+ 77359.0 / T
		- 5.8927e-8 * T * T * T;
	if(hillert_jarl_gmag(T, 1043.0, 2.22, 0.40, &gmag) != FPROPS_NO_ERROR){
		return FPROPS_RANGE_ERROR;
	}
	*g_out += gmag;
	return FPROPS_NO_ERROR;
}

static FpropsError gibbs_fe_fcc(double T, double p, double *g_out){
	double gmag = 0.0;
	(void)p;
	if(!g_out || !(T >= 298.0) || !(T <= 6000.0)){
		return FPROPS_RANGE_ERROR;
	}
	if(T <= 1811.0){
		*g_out = -236.5
			+ 132.4156 * T
			- 24.6643 * T * log(T)
			- 0.003758 * T * T
			+ 77359.0 / T
			- 5.8927e-8 * T * T * T;
	}else{
		*g_out = -27097.2
			+ 300.2521 * T
			- 46.0 * T * log(T)
			+ 2.78854e31 * pow(T, -9.0);
	}
	if(hillert_jarl_gmag(T, 67.0, 0.70, 0.28, &gmag) != FPROPS_NO_ERROR){
		return FPROPS_RANGE_ERROR;
	}
	*g_out += gmag;
	return FPROPS_NO_ERROR;
}

static FpropsError gibbs_fe3o4(double T, double p, double *g_out){
	(void)p;
	if(!g_out || !(T >= 298.0) || !(T <= 2500.0)){
		return FPROPS_RANGE_ERROR;
	}
	*g_out = -1140237.0
		+ 1015.067 * T
		- 174.832 * T * log(T)
		- 0.008149196 * T * T
		+ 1445276.0 / T;
	return FPROPS_NO_ERROR;
}

static FpropsError gibbs_fe2o3(double T, double p, double *g_out){
	double gmag = 0.0;
	(void)p;
	if(!g_out || !(T >= 298.0) || !(T <= 4000.0)){
		return FPROPS_RANGE_ERROR;
	}
	if(T <= 2500.0){
		*g_out = -859683.1
			+ 828.0501 * T
			- 137.0089 * T * log(T)
			+ 1453820.0 / T;
	}else{
		*g_out = -857356.9
			+ 823.7122 * T
			- 136.5437 * T * log(T);
	}
	if(hillert_jarl_gmag(T, 955.667, 8.36667, 0.28, &gmag) != FPROPS_NO_ERROR){
		return FPROPS_RANGE_ERROR;
	}
	*g_out += gmag;
	return FPROPS_NO_ERROR;
}

/*
 * Pragmatic Tier 2 subset from Hidayat et al. (2015):
 * direct Gibbs-energy fits plus unary Hillert-Jarl magnetic terms for
 * Fe_bcc, Fe_fcc, and Fe2O3, without the full spinel CEF/magnetic model.
 */
static const GibbsSpecies species_fe_bcc = {
	"Fe_bcc", "hidayat_2015",
	1, elements_fe, stoich_fe,
	&gibbs_fe_bcc
};

static const GibbsSpecies species_fe_fcc = {
	"Fe_fcc", "hidayat_2015",
	1, elements_fe, stoich_fe,
	&gibbs_fe_fcc
};

static const GibbsSpecies species_fe3o4 = {
	"Fe3O4", "hidayat_2015",
	2, elements_fe3o4, stoich_fe3o4,
	&gibbs_fe3o4
};

static const GibbsSpecies species_fe2o3 = {
	"Fe2O3", "hidayat_2015",
	2, elements_fe2o3, stoich_fe2o3,
	&gibbs_fe2o3
};

static const GibbsSpeciesEntry entries[] = {
	{"Fe_bcc", &species_fe_bcc},
	{"Fe(alpha)", &species_fe_bcc},
	{"Fe(delta)", &species_fe_bcc},
	{"Fe_fcc", &species_fe_fcc},
	{"Fe(gamma)", &species_fe_fcc},
	{"Fe3O4", &species_fe3o4},
	{"Fe2O3", &species_fe2o3}
};

const GibbsSpecies *gibbs_species_lookup(const char *name, const char *source){
	size_t i;
	if(!name){
		return NULL;
	}
	for(i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i){
		const GibbsSpecies *S = entries[i].species;
		if(0 != strcmp(entries[i].name, name)){
			continue;
		}
		if(source){
			if(S->source && NULL != strstr(S->source, source)){
				return S;
			}
			continue;
		}
		return S;
	}
	return NULL;
}

int gibbs_species_g_molar(const GibbsSpecies *S, double T, double p, double *g_out){
	FpropsError err;
	if(!S || !S->g_molar_fn || !g_out){
		return 0;
	}
	err = S->g_molar_fn(T, p, g_out);
	if(err != FPROPS_NO_ERROR || !isfinite(*g_out)){
		return 0;
	}
	return 1;
}
