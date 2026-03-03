#include "constcp_species.h"
#include "constcp_data.h"

static double constcp_species_phase_g_mass(const ConstCpSpecies *S, const ConstCpData *phase,
		double T, double p, FpropsError *err){
	(void)S;
	return constcp_g(T, p, phase, err);
}

double constcp_species_phase_g_molar(const ConstCpSpecies *S, const ConstCpData *phase,
		double T, double p, FpropsError *err){
	double g_mass;
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!S || !phase || !(T > 0) || !(p > 0) || !(S->M > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	g_mass = constcp_species_phase_g_mass(S, phase, T, p, err);
	if(*err)return 0;
	return g_mass * (S->M * 1e-3);
}

const ConstCpData *constcp_species_select_phase(const ConstCpSpecies *S, double T, double p,
		FpropsError *err){
	const ConstCpData *best = NULL;
	double best_g = 0.0;
	unsigned i;

	if(!err)return NULL;
	*err = FPROPS_NO_ERROR;
	if(!S || !S->phases || S->nphases == 0 || !(T > 0) || !(p > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return NULL;
	}
	for(i = 0; i < S->nphases; ++i){
		double g = constcp_species_phase_g_molar(S, &S->phases[i], T, p, err);
		if(*err)return NULL;
		if(!best || g < best_g){
			best = &S->phases[i];
			best_g = g;
		}
	}
	return best;
}

double constcp_species_g_molar(const ConstCpSpecies *S, double T, double p,
		const ConstCpData **phase_out, FpropsError *err){
	const ConstCpData *phase = constcp_species_select_phase(S, T, p, err);
	if(!phase || (err && *err)){
		return 0;
	}
	if(phase_out){
		*phase_out = phase;
	}
	return constcp_species_phase_g_molar(S, phase, T, p, err);
}

const ConstCpSpecies *constcp_species_lookup(const char *name, const char *source){
	return constcp_data_lookup(name, source);
}

static double constcp_species_h_cb(const void *ctx, double T, double p, const double *x,
		FpropsError *err){
	const ConstCpSpecies *S = (const ConstCpSpecies *)ctx;
	const ConstCpData *phase;
	(void)x;
	phase = constcp_species_select_phase(S, T, p, err);
	if(!phase || (err && *err)){
		return 0;
	}
	return constcp_h(T, phase, err);
}

static double constcp_species_s_cb(const void *ctx, double T, double p, const double *x,
		FpropsError *err){
	const ConstCpSpecies *S = (const ConstCpSpecies *)ctx;
	const ConstCpData *phase;
	(void)x;
	phase = constcp_species_select_phase(S, T, p, err);
	if(!phase || (err && *err)){
		return 0;
	}
	return constcp_s(T, phase, err);
}

static double constcp_species_g_cb(const void *ctx, double T, double p, const double *x,
		FpropsError *err){
	const ConstCpSpecies *S = (const ConstCpSpecies *)ctx;
	const ConstCpData *phase;
	(void)x;
	phase = constcp_species_select_phase(S, T, p, err);
	if(!phase || (err && *err)){
		return 0;
	}
	return constcp_g(T, p, phase, err);
}

static double constcp_species_mu_cb(const void *ctx, unsigned i, double T, double p,
		const double *x, FpropsError *err){
	(void)i;
	return constcp_species_g_cb(ctx, T, p, x, err);
}

static const ThermoModel constcp_species_model = {
	.h = constcp_species_h_cb,
	.s = constcp_species_s_cb,
	.g = constcp_species_g_cb,
	.mu = constcp_species_mu_cb,
	.sat_T = NULL
};

const ThermoModel *thermo_model_constcp_species(void){
	return &constcp_species_model;
}
