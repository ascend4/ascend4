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
#include "fprops.h"
#include "helmholtz.h"
#include "ideal.h"
#include "sat.h"
//#include "redkw.h"
#include "pengrob.h"
//#include "mbwr.h"

# include "color.h"
//# define FPR_DEBUG
#ifdef FPR_DEBUG
# define MSG(FMT, ...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"%s: ",__func__);\
	color_off(stderr);\
	fprintf(stderr,FMT "\n",##__VA_ARGS__)
#else
# define MSG(ARGS...) ((void)0)
#endif

#ifdef FPR_ERRORS
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
#endif

// FIXME convert to use of ideal + departure function here in this file (not at the correlation level)
// ... or doesn't that work? get further with PR EOS first?
double tnbpref,vref;

int fprops_source_match(const EosData *E, const char *source){
	// FIXME do this case-insensitively */
	/* if source is specified, ensure that it's a substring */
	if(!source || strstr(source,E->source))return 1;
	return 0;
}


int fprops_corr_avail(const EosData *E, const char *corrtype){
	// TODO feels like there shoud be a better way of doing this?

	if(corrtype==NULL){
		/* if correlation is not specified, return the 'best' we can manage (not yet very sophisticated) */
		switch(E->type){
		case FPROPS_HELMHOLTZ:
            return E->type;
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
            return FPROPS_PENGROB;
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


PureFluid *fprops_prepare(const EosData *E, const char *corrtype){
	switch(fprops_corr_avail(E,corrtype)){
	case FPROPS_HELMHOLTZ:
		return helmholtz_prepare(E,NULL);
	case FPROPS_PENGROB:
		return pengrob_prepare(E,NULL);
	case FPROPS_IDEAL:
		return ideal_prepare(E,NULL);
	default:
		MSG("Invalid EOS data, unimplemented correlation type requested");
		return NULL;
	}
}


// FIXME XXX not all properties are mass-weighted in the saturation region...

/*
	TODO need some kind of support for freezing line, especially for water
	since for temperatures 0.degC up to 0.01 degC liquid phase is possible
	for for ambient pressure, but this is BELOW triple line. Hmmm.

	Also sublimation curve needs to be added.
*/

#define EVALFN(VAR) \
	double fprops_##VAR(double T, double rho, const PureFluid *fluid, FpropsError *err){\
		double p, rho_f, rho_g;\
		if(T >= fluid->data->T_t && T < fluid->data->T_c){\
            switch(fluid->type){\
                case (FPROPS_PENGROB):{\
                    MSG("starting the satp");\
                    int raz = fprops_sat_p(101.325e3, &tnbpref,&vref, &rho_g, fluid);\
                    MSG("done with satp");\
                    raz = fprops_sat_T_cubic(T, &p, &rho_f, &rho_g, fluid);\
                    if(raz){\
                    *err = raz;\
                    return fluid->data->rho_c;\
                    }\
                    if(rho_g < rho && rho < rho_f){\
                        double x = rho_g*(rho_f/rho - 1)/(rho_f - rho_g);\
                        double Qf = fluid->VAR##_fn(T,rho_f,fluid->data,err);\
                        double Qg = fluid->VAR##_fn(T,rho_g,fluid->data,err);\
                        return x*Qg + (1-x)*Qf;\
                    }\
                    break;\
                }\
                default:{\
                    int res = fprops_sat_T(T, &p, &rho_f, &rho_g, fluid);\
                    if(res){\
                        *err = FPROPS_SAT_CVGC_ERROR;\
                        return fluid->data->rho_c;\
                    }\
                    if(rho_g < rho && rho < rho_f){\
                        double x = rho_g*(rho_f/rho - 1)/(rho_f - rho_g);\
                        double Qf = fluid->VAR##_fn(T,rho_f,fluid->data,err);\
                        double Qg = fluid->VAR##_fn(T,rho_g,fluid->data,err);\
                        return x*Qg + (1-x)*Qf;\
                    }\
                }\
            }\
		}\
		return fluid->VAR##_fn(T,rho,fluid->data,err);\
	}\

EVALFN(p); EVALFN(u); EVALFN(h); EVALFN(s); EVALFN(a); EVALFN(g);
EVALFN(cp); EVALFN(cv);
EVALFN(w);
EVALFN(dpdrho_T);

//EVALFN(alphap); EVALFN(betap); EVALFN(dpdT_rho);
//EVALFN(dpdrho_T); EVALFN(d2pdrho2_T); EVALFN(dhdT_rho); EVALFN(dhdrho_T);
//EVALFN(dudT_rho); EVALFN(dudrho)T);

double fprops_alphap(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}
double fprops_betap(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}

double fprops_cp0(double T, const PureFluid *fluid, FpropsError *err){
	return ideal_cp(T,0,fluid->data,err);
}

double fprops_dpdT_rho(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}

double fprops_d2pdrho2_T(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}

double fprops_dhdT_rho(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}
double fprops_dhdrho_T(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}

double fprops_dudT_rho(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}
double fprops_dudrho_T(double T, double rho, const PureFluid *data, FpropsError *err){
	return 0;
}

char *fprops_error(FpropsError err){
	switch (err) {
	case FPROPS_NO_ERROR:return NULL;
	case FPROPS_DATA_ERROR:return "FPROPS encountered a data error.";
	case FPROPS_NUMERIC_ERROR:return "FPROPS encountered a numerical error.";
	case FPROPS_SAT_CVGC_ERROR:return "FPROPS unable to converge solution in saturation region.";
	case FPROPS_RANGE_ERROR:return "FPROPS had a range error on one of its inputs.";
	case FPROPS_NOT_IMPLEMENTED:return "FPROPS feature not yet implemented.";
	default:return "Unrecognised error";
	}
}

