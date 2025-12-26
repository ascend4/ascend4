#include "constcp.h"

#include <math.h>
#include <stdlib.h>

ConstCpData *constcp_create(ConstCpPhase phase, double M, double cp, double T_ref
	, double h_ref, double s_ref, double rho, FpropsError *err
){
	ConstCpData *D;
	if(!err)return NULL;
	*err = FPROPS_NO_ERROR;
	if(M <= 0 || cp <= 0 || T_ref <= 0){
		*err = FPROPS_INVALID_REQUEST;
		return NULL;
	}
	D = (ConstCpData *)calloc(1,sizeof(ConstCpData));
	if(!D){
		*err = FPROPS_NUMERIC_ERROR;
		return NULL;
	}
	D->phase = phase;
	D->M = M;
	D->cp = cp;
	D->T_ref = T_ref;
	D->h_ref = h_ref;
	D->s_ref = s_ref;
	D->rho = rho;
	return D;
}

void constcp_destroy(ConstCpData *D){
	free(D);
}

double constcp_h(double T, const ConstCpData *D, FpropsError *err){
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!D || !(T > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	return D->h_ref + D->cp * (T - D->T_ref);
}

double constcp_s(double T, const ConstCpData *D, FpropsError *err){
	if(!err)return 0;
	*err = FPROPS_NO_ERROR;
	if(!D || !(T > 0)){
		*err = FPROPS_INVALID_REQUEST;
		return 0;
	}
	return D->s_ref + D->cp * log(T / D->T_ref);
}

double constcp_g(double T, const ConstCpData *D, FpropsError *err){
	double h = constcp_h(T, D, err);
	if(*err)return 0;
	{
		double s = constcp_s(T, D, err);
		if(*err)return 0;
		return h - T * s;
	}
}
