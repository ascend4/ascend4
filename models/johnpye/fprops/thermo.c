#include "thermo.h"

double thermo_h(const ThermoState *S, FpropsError *err){
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!S || !S->model || !S->model->h){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	return S->model->h(S->model_data, S->T, S->p, S->x, err);
}

double thermo_s(const ThermoState *S, FpropsError *err){
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!S || !S->model || !S->model->s){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	return S->model->s(S->model_data, S->T, S->p, S->x, err);
}

double thermo_g(const ThermoState *S, FpropsError *err){
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!S || !S->model || !S->model->g){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	return S->model->g(S->model_data, S->T, S->p, S->x, err);
}

double thermo_mu(const ThermoState *S, unsigned i, FpropsError *err){
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!S || !S->model || !S->model->mu){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	return S->model->mu(S->model_data, i, S->T, S->p, S->x, err);
}

double thermo_sat_T(const ThermoState *S, double T, double *rhof, double *rhog, FpropsError *err){
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!S || !S->model || !S->model->sat_T){
		*err = FPROPS_NOT_IMPLEMENTED;
		return 0;
	}
	return S->model->sat_T(S->model_data, T, rhof, rhog, err);
}
