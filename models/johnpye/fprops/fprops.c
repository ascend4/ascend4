/*
 * FPROPS fluid property calculation library
 * Copyright (C) 2011 - John Pye
 *
 * ASCEND is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ASCEND is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASCEND; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/*
 * This file should contain general functions which are common to all equations of state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rundata.h"
#include "filedata.h"
#include "fprops.h"
#include "helmholtz.h"
#include "ideal.h"
#include "sat.h"
//#include "redkw.h"
#include "pengrob.h"
#include "visc.h"
#include "thcond.h"
//#include "mbwr.h"

//#define FPR_DEBUG
#ifdef FPR_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

#include <stdio.h>

// FIXME convert to use of ideal + departure function here in this file (not at the correlation level)
// ... or doesn't that work? get further with PR EOS first?


int fprops_corr_avail(const EosData *E, const char *corrtype){
	if(corrtype==NULL){
		/* if none specify, return the 'best' we can manage (not yet very civilised) */
		switch(E->type){
		case FPROPS_HELMHOLTZ:
		case FPROPS_IDEAL:
			return E->type;
		case FPROPS_CUBIC:
			return FPROPS_PENGROB;
		default:
			return 0;
		}
	}else if(strcmp(corrtype,"helmholtz")==0){
		switch(E->type){
		case FPROPS_HELMHOLTZ:
			return FPROPS_HELMHOLTZ;
		default:
			return 0;
		}
	}else if(strcmp(corrtype,"pengrob")==0){
		switch(E->type){
		case FPROPS_HELMHOLTZ:
		case FPROPS_CUBIC:
			return FPROPS_PENGROB;
		default:
			return 0;
		}
	}else if(strcmp(corrtype,"ideal")==0){
		switch(E->type){
		case FPROPS_IDEAL:
		case FPROPS_HELMHOLTZ:
		case FPROPS_CUBIC:
			return FPROPS_IDEAL;
		default:
			return 0;
		}
	}
	return 0;
}


PureFluid *fprops_prepare(const EosData *E,const char *corrtype){
	PureFluid *P = NULL;
	FpropsError err = FPROPS_NO_ERROR;
	MSG("Working with EosData name '%s', source '%s", E->name, E->source);
	MSG("Chosen correlation: %d (requested %s)", fprops_corr_avail(E,corrtype),corrtype);
	switch(fprops_corr_avail(E,corrtype)){
	case FPROPS_HELMHOLTZ:
		P = helmholtz_prepare(E,NULL);
		break;
	case FPROPS_PENGROB:
		P = pengrob_prepare(E,NULL);
		break;
	case FPROPS_IDEAL:
		P = ideal_prepare(E,NULL);
		break;
	default:
		ERRMSG("Invalid EOS data, unimplemented correlation type requested");
		return NULL;
	}
	/* next: add preparation of viscosity, thermal conductivity, surface tension, ... */

	MSG("Preparing viscosity data...");
	P->visc = visc_prepare(E,P,&err);
	if(err){
		ERRMSG("Invalid viscosity data for '%s",P->name);
		/* visc_prepare should return NULL if there was an error, so result is
		same as when there is no viscosity data at all */
	}

	MSG("Preparing thermal conductivity data...");
	err = FPROPS_NO_ERROR;
	if(E->thcond){
		thcond_prepare(P,E->thcond,&err);
		if(err){
			ERRMSG("Invalid viscosity data for '%s",P->name);
			/* visc_prepare should return NULL if there was an error, so result is
			same as when there is no viscosity data at all */
		}
	}
	return P;
}

FluidState fprops_set_Trho(double T, double rho, const PureFluid *fluid, FpropsError *err){
	FluidState state = {T,rho,fluid};
	return state;
}

// FIXME XXX not all properties are mass-weighted in the saturation region...

/*
	TODO need some kind of support for freezing line, especially for water
	since for temperatures 0.degC up to 0.01 degC liquid phase is possible
	for ambient pressure, but this is BELOW triple line. Hmmm.

	Also sublimation curve needs to be added.
*/

#define EVALFN(VAR) \
	double fprops_##VAR(FluidState state, FpropsError *err){\
		double p, rho_f, rho_g;\
		if(state.T >= state.fluid->data->T_t && state.T < state.fluid->data->T_c){\
			fprops_sat_T(state.T, &p, &rho_f, &rho_g, state.fluid, err);\
			if(*err){\
				MSG("Got error %d from saturation calc in %s\n",*err,__func__);\
				/*return state.fluid->VAR##_fn(state.fluid->data->T_c,state.fluid->data->rho_c,state.fluid->data,err);*/\
				return 0;\
			}\
			if(rho_g < state.rho && state.rho < rho_f){\
				double x = rho_g*(rho_f/state.rho - 1)/(rho_f - rho_g);\
				double Qf = state.fluid->VAR##_fn(state.T,rho_f,state.fluid->data,err);\
				double Qg = state.fluid->VAR##_fn(state.T,rho_g,state.fluid->data,err);\
				return x*Qg + (1-x)*Qf;\
			}\
		}\
		return state.fluid->VAR##_fn(state.T,state.rho,state.fluid->data,err);\
	}

#define EVALFN_SATUNDEFINED(VAR) \
	double fprops_##VAR(FluidState state, FpropsError *err){\
		double p, rho_f, rho_g;\
		if(state.T >= state.fluid->data->T_t && state.T < state.fluid->data->T_c){\
			fprops_sat_T(state.T, &p, &rho_f, &rho_g, state.fluid, err);\
			if(*err){\
				MSG("Got error %d from sat calc in %s\n",*err,__func__);\
				return state.fluid->data->rho_c;\
			}\
			if(rho_g < state.rho && state.rho < rho_f){\
				*err = FPROPS_VALUE_UNDEFINED;\
			}\
		}\
		return state.fluid->VAR##_fn(state.T,state.rho,state.fluid->data,err);\
	}

EVALFN(p); EVALFN(u); EVALFN(h); EVALFN(s); EVALFN(a); EVALFN(g);
EVALFN_SATUNDEFINED(cp); EVALFN_SATUNDEFINED(cv);
EVALFN_SATUNDEFINED(w);
EVALFN(dpdrho_T);

EVALFN(alphap); EVALFN(betap);
// EVALFN(dpdT_rho);
//EVALFN(dpdrho_T); EVALFN(d2pdrho2_T); EVALFN(dhdT_rho); EVALFN(dhdrho_T);
//EVALFN(dudT_rho); EVALFN(dudrho)T);

#if 0
double fprops_alphap(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}
double fprops_betap(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}
#endif

/// TODO reimplement with function pointer?
double fprops_mu(FluidState state, FpropsError *err){
	if(NULL!=state.fluid->visc){
		switch(state.fluid->visc->type){
		case FPROPS_VISC_1:
			return visc1_mu(state,err);
		default:
			break;
		}	
	}
	*err = FPROPS_NOT_IMPLEMENTED;
	return NAN;
}

/// TODO reimplement with function pointer?
double fprops_lam(FluidState state, FpropsError *err){
	if(NULL!=state.fluid->thcond){
		switch(state.fluid->thcond->type){
		case FPROPS_THCOND_1:
			return thcond1_lam(state,err);
		default:
			break;
		}	
	}
	*err = FPROPS_NOT_IMPLEMENTED;
	return NAN;
}


double fprops_cp0(FluidState state, FpropsError *err){
	return ideal_cp(state.T,0,state.fluid->data,err);
}

double fprops_x(FluidState state, FpropsError *err){
	double p, rho_f, rho_g;
	if(state.T >= state.fluid->data->T_t && state.T < state.fluid->data->T_c){
		fprops_sat_T(state.T, &p, &rho_f, &rho_g, state.fluid, err);
		if(*err)return 0;
		if(state.rho > rho_f)return 0;
		if(state.rho < rho_g)return 1;
		return rho_g*(rho_f/state.rho - 1)/(rho_f - rho_g);
	}
	fprintf(stderr,"Temperature is <T_t or >T_c\n");
	*err = FPROPS_VALUE_UNDEFINED;
	return 0;
}

double fprops_dpdT_rho(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_d2pdrho2_T(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_dhdT_rho(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_dhdrho_T(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double fprops_dudT_rho(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}
double fprops_dudrho_T(FluidState state, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

char *fprops_error(FpropsError err){
	switch (err) {
	case FPROPS_NO_ERROR:return NULL;
	case FPROPS_NUMERIC_ERROR:return "FPROPS encountered a numerical error.";
	case FPROPS_SAT_CVGC_ERROR:return "FPROPS unable to converge solution in saturation region.";
	case FPROPS_RANGE_ERROR:return "FPROPS had a range error on one of its inputs.";
	case FPROPS_DATA_ERROR:return "FPROPS encountered a data error.";
	case FPROPS_NOT_IMPLEMENTED:return "FPROPS feature not yet implemented.";
	case FPROPS_INVALID_REQUEST:return "FPROPS encountered an invalid request.";
	case FPROPS_VALUE_UNDEFINED:return "FPROPS reports the request value is locally undefined.";
	default:return "Unrecognised error";
	}
}

char *fprops_corr_type(EosType type){
	switch(type){
	case FPROPS_IDEAL:
		return "ideal";
	case FPROPS_CUBIC:
		return "cubic";
	case FPROPS_PENGROB:
		return "pengrob";
	case FPROPS_REDKW:
		return "redkw";
	case FPROPS_SOAVE:
		return "soave";
	case FPROPS_HELMHOLTZ:
		return "helmholtz";
	case FPROPS_MBWR:
		return "mbwr";
	}
	return NULL;
}

