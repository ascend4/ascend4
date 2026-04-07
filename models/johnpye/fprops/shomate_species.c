#include "shomate_species.h"
#include "shomate_data.h"

#include <math.h>

static int shomate_find_range_idx(const ShomateSpecies *S, double T){
	unsigned i;
	if(!S || !S->ranges || S->nranges == 0){
		return -1;
	}
	for(i = 0; i < S->nranges; ++i){
		if(shomate_range_contains(&S->ranges[i], T)){
			return (int)i;
		}
	}
	/* Outside tabulated bounds: clamp to nearest range. */
	if(T < S->ranges[0].T_min){
		return 0;
	}
	return (int)(S->nranges - 1);
}

static int shomate_integrate_piecewise(const ShomateSpecies *S, double T0, double T1,
		double *dh, double *ds, FpropsError *err){
	double current = T0;
	double dh_acc = 0.0;
	double ds_acc = 0.0;
	int guard = 0;
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!S || !(T0 > 0.0) || !(T1 > 0.0) || !dh || !ds){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	if(T0 == T1){
		*dh = 0.0;
		*ds = 0.0;
		return 1;
	}
	while(1){
		int idx;
		const ShomateRange *R;
		double next;
		double dh_seg = 0.0;
		double ds_seg = 0.0;
		idx = shomate_find_range_idx(S, current);
		if(idx < 0){
			*err = FPROPS_INVALID_REQUEST;
			return 0;
		}
		R = &S->ranges[idx];
		if(T1 > T0){
			next = (T1 < R->T_max) ? T1 : R->T_max;
		}else{
			next = (T1 > R->T_min) ? T1 : R->T_min;
		}
		dh_seg = shomate_delta_h_molar(R, current, next, err);
		if(*err)return 0;
		ds_seg = shomate_delta_s_molar(R, current, next, err);
		if(*err)return 0;
		dh_acc += dh_seg;
		ds_acc += ds_seg;
		if(next == T1){
			break;
		}
		current = (T1 > T0) ? (next + 1e-9) : (next - 1e-9);
		++guard;
		if(guard > 128){
			*err = FPROPS_NUMERIC_ERROR;
			return 0;
		}
	}
	*dh = dh_acc;
	*ds = ds_acc;
	return 1;
}

const ShomateSpecies *shomate_species_lookup(const char *name, const char *source){
	return shomate_data_lookup(name, source);
}

double shomate_species_h_molar(const ShomateSpecies *S, double T, FpropsError *err){
	double dh = 0.0;
	double ds_dummy = 0.0;
	if(!err)return 0.0;
	*err = FPROPS_NO_ERROR;
	if(!S || !(T > 0.0) || !(S->T_ref > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	if(!shomate_integrate_piecewise(S, S->T_ref, T, &dh, &ds_dummy, err)){
		return 0.0;
	}
	return S->h_ref_molar + dh;
}

double shomate_species_s_molar(const ShomateSpecies *S, double T, FpropsError *err){
	double ds = 0.0;
	double dh_dummy = 0.0;
	if(!err)return 0.0;
	*err = FPROPS_NO_ERROR;
	if(!S || !(T > 0.0) || !(S->T_ref > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	if(!shomate_integrate_piecewise(S, S->T_ref, T, &dh_dummy, &ds, err)){
		return 0.0;
	}
	return S->s_ref_molar + ds;
}

double shomate_species_g_molar(const ShomateSpecies *S, double T, double p, FpropsError *err){
	double h;
	double s;
	double g;
	if(!err)return 0.0;
	*err = FPROPS_NO_ERROR;
	if(!S || !(T > 0.0) || !(p > 0.0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0.0;
	}
	h = shomate_species_h_molar(S, T, err);
	if(*err)return 0.0;
	s = shomate_species_s_molar(S, T, err);
	if(*err)return 0.0;
	g = h - T * s;
	if(S->phase == FPROPS_PHASE_GAS && S->p_ref > 0.0){
		g += 8.31446261815324 * T * log(p / S->p_ref);
	}else if(S->phase != FPROPS_PHASE_GAS && S->rho_ref > 0.0){
		double molar_volume = (S->M * 1e-3) / S->rho_ref;
		g += (p - S->p_ref) * molar_volume;
	}
	return g;
}
