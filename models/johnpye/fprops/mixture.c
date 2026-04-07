#include "mixture.h"
#include "ideal.h"

#include <math.h>
#include <stdlib.h>

static int mix_update_cache(FpropsMix *mix, FpropsError *err){
	double Mmix = 0.0;
	unsigned i;
	for(i = 0; i < mix->nspecies; ++i){
		const PureFluid *P = mix->species[i];
		double Mi = (P && P->data) ? P->data->M : 0.0;
		if(Mi <= 0){
			*err = FPROPS_INVALID_REQUEST;
			return 1;
		}
		Mmix += mix->x[i] * Mi;
	}
	if(Mmix <= 0){
		*err = FPROPS_INVALID_REQUEST;
		return 1;
	}
	mix->Mmix = Mmix;
	mix->Rmix = R_UNIVERSAL / Mmix;
	return 0;
}

FpropsMix *fprops_mix_create(const PureFluid **species, unsigned nspecies, FpropsError *err){
	FpropsMix *mix;
	unsigned i;
	if(!err)return NULL;
	*err = FPROPS_NO_ERROR;
	if(!species || nspecies == 0){
		*err = FPROPS_INVALID_REQUEST;
		return NULL;
	}
	mix = (FpropsMix *)calloc(1,sizeof(FpropsMix));
	if(!mix){
		*err = FPROPS_NUMERIC_ERROR;
		return NULL;
	}
	mix->species = (const PureFluid **)calloc(nspecies,sizeof(PureFluid *));
	mix->x = (double *)calloc(nspecies,sizeof(double));
	if(!mix->species || !mix->x){
		*err = FPROPS_NUMERIC_ERROR;
		fprops_mix_destroy(mix);
		return NULL;
	}
	mix->nspecies = nspecies;
	for(i = 0; i < nspecies; ++i){
		mix->species[i] = species[i];
		mix->x[i] = 1.0 / (double)nspecies;
	}
	if(mix_update_cache(mix, err)){
		fprops_mix_destroy(mix);
		return NULL;
	}
	return mix;
}

FpropsMix *fprops_mix_from_pure(const PureFluid *species, FpropsError *err){
	const PureFluid *list[1];
	if(!err)return NULL;
	*err = FPROPS_NO_ERROR;
	if(!species){
		*err = FPROPS_INVALID_REQUEST;
		return NULL;
	}
	list[0] = species;
	return fprops_mix_create(list, 1, err);
}

void fprops_mix_destroy(FpropsMix *mix){
	if(!mix)return;
	free(mix->species);
	free(mix->x);
	free(mix);
}

int fprops_mix_set_x(FpropsMix *mix, const double *x, FpropsError *err){
	unsigned i;
	double sum = 0.0;
	if(!err)return 1;
	*err = FPROPS_NO_ERROR;
	if(!mix || !x || mix->nspecies == 0){
		*err = FPROPS_INVALID_REQUEST;
		return 1;
	}
	for(i = 0; i < mix->nspecies; ++i){
		if(x[i] < 0){
			*err = FPROPS_RANGE_ERROR;
			return 1;
		}
		sum += x[i];
	}
	if(sum <= 0){
		*err = FPROPS_RANGE_ERROR;
		return 1;
	}
	for(i = 0; i < mix->nspecies; ++i){
		mix->x[i] = x[i] / sum;
	}
	return mix_update_cache(mix, err);
}

static double mix_partial_rho(const PureFluid *P, double x_i, double T, double p){
	double Ri = P->data->R;
	double p_i = x_i * p;
	return p_i / (Ri * T);
}

double fprops_mix_h_ig(const FpropsMix *mix, double T, FpropsError *err){
	unsigned i;
	double hmix = 0.0;
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!mix || !(T > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	for(i = 0; i < mix->nspecies; ++i){
		const PureFluid *P = mix->species[i];
		double Mi = P->data->M;
		double wi = mix->x[i] * Mi / mix->Mmix;
		double h_i = ideal_h((FluidStateUnion){.Trho={T,1.0}}, P->data, err);
		if(*err)return 0;
		hmix += wi * h_i;
	}
	return hmix;
}

double fprops_mix_cp_ig(const FpropsMix *mix, double T, FpropsError *err){
	unsigned i;
	double cpmix = 0.0;
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!mix || !(T > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	for(i = 0; i < mix->nspecies; ++i){
		const PureFluid *P = mix->species[i];
		double Mi = P->data->M;
		double wi = mix->x[i] * Mi / mix->Mmix;
		double cp_i = ideal_cp((FluidStateUnion){.Trho={T,1.0}}, P->data, err);
		if(*err)return 0;
		cpmix += wi * cp_i;
	}
	return cpmix;
}

double fprops_mix_s_ig(const FpropsMix *mix, double T, double p, FpropsError *err){
	unsigned i;
	double s_molar = 0.0;
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!mix || !(T > 0) || !(p > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	for(i = 0; i < mix->nspecies; ++i){
		const PureFluid *P = mix->species[i];
		double x_i = mix->x[i];
		if(x_i <= 0)continue;
		double rho_i = mix_partial_rho(P, x_i, T, p);
		double s_i = ideal_s((FluidStateUnion){.Trho={T,rho_i}}, P->data, err);
		if(*err)return 0;
		s_molar += x_i * s_i * P->data->M;
	}
	return s_molar / mix->Mmix;
}

double fprops_mix_g_ig(const FpropsMix *mix, double T, double p, FpropsError *err){
	unsigned i;
	double g_molar = 0.0;
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!mix || !(T > 0) || !(p > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	for(i = 0; i < mix->nspecies; ++i){
		const PureFluid *P = mix->species[i];
		double x_i = mix->x[i];
		if(x_i <= 0)continue;
		double rho_i = mix_partial_rho(P, x_i, T, p);
		double g_i = ideal_g((FluidStateUnion){.Trho={T,rho_i}}, P->data, err);
		if(*err)return 0;
		g_molar += x_i * g_i * P->data->M;
	}
	return g_molar / mix->Mmix;
}

double fprops_mix_mu_ig(const FpropsMix *mix, unsigned i, double T, double p, FpropsError *err){
	const PureFluid *P;
	double x_i;
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!mix || i >= mix->nspecies || !(T > 0) || !(p > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	P = mix->species[i];
	x_i = mix->x[i];
	if(x_i <= 0){
		*err = FPROPS_RANGE_ERROR;
		return 0;
	}
	{
		double rho_i = mix_partial_rho(P, x_i, T, p);
		return ideal_g((FluidStateUnion){.Trho={T,rho_i}}, P->data, err);
	}
}
