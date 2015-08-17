/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*//*
	by Jacob Shealy, July 30-, 2015

	Functions to calculate mixture properties under the ideal-solution model.
 */

#include "mixture_generics.h"
#include "mixture_prepare.h"
#include "mixture_struct.h"
#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <math.h>

/*
	Structure to hold auxiliary data for function to find the error in pressure 
	at a given density
 */
typedef struct PressureRhoData_Struct{
	double T;         /* mixture temperature */
	double p;         /* mixture pressure */
	PureFluid *pfl;   /* pure fluid from the mixture */
	FpropsError *err; /* necessary error variable */
} PRData;

/*
	Structure to hold auxiliary data for function that will return the error in 
	temperature when trying to find the temperature corresponding to a given 
	enthalpy.
 */
typedef struct EnthalpyTData_Struct{
	double h;         /* the whole-mixture enthalpy being sought */
	double p;         /* the pressure of the mixture */
	MixtureSpeca *MS; /* specification of mixture composition */
    double tol;       /* error to be used in solving flash condition for mixture */
	FpropsError *err; /* necessary error variable */
} HTData;

/*
    Find the difference between a given pressure and a pressure calculated from 
    the density, for a single component in a mixture.  Passed to root-finding 
    functions when searching for the density at a given pressure.
 */
double pressure_rho_error(double rho, void *user_data){
	PRData *prd = (PRData *)user_data;
	FluidState fst = {prd->T, rho, prd->pfl};
	
	return fabs(prd->p - fprops_p(fst, prd->err)) / fabs(prd->p);
}

/*
    Find the difference between a given mixture enthalpy and that calculated 
    from the temperature.  Passed to root-finding functions when searching for 
    the density at a given pressure.
 */
double enthalpy_T_error(double T, void *user_data){
	HTData *htd = (HTData *)user_data;

    PhaseSpec *PS = ASC_NEW(PhaseSpec);
    mixture_flash(PS, htd->MS, T, htd->p, htd->tol, htd->err);

    double *h_phases = ASC_NEW_ARRAY(double,PS->phases);

    PhaseMixState *PM = fill_PhaseMixState(T, htd->p, PS, htd->MS);
	
	return (htd->h - mixture_h(PM, h_phases, htd->err)) / fabs(htd->h);
}

/*
	Find the vapor and liquid densities at saturation conditions for each phase 
    in the mixture.  This function MUST receive a PhaseSpec '*PS' which has been 
    successfully filled with phase/mass fraction data using the function 
    'mixture_flash' or some other function.

	Return values indicate:
		0 - success
        1 - the root-finding algorithm that finds component mass fractions 
            converged on a single non-solution point
		2 - the root-finding algorithm converged on infinity or NaN
		3 - the root-finding algorithm reached the maximum number of iterations 
			without converging.
 */
int mixture_rhos_sat(PhaseSpec *PS, double T, double P, double tol, FpropsError *err){
#define PPH PS->PH
#define NPURE PPH[i]->pures
#define RHOS(IX) PPH[ IX ]->rhos
	unsigned i, j;
	int i_v = -1
		, i_l = -1
		, sec = 0  /* whether the secant root-finding method succeeded */
		;
	double p_sat
		, rho_d
		, rhos[2]        /* densities used in searching for supercritical densities */
		/* , tol = MIX_XTOL tolerance used in root-finding function */
		;

	for(i=0;i<PS->phases;i++){
		if(PS->ph_type[i]==SUPERCRIT){
			for(j=0;j<NPURE;j++){
				PRData prd = {T, P, PPH[i]->PF[j], err};
				rhos[0] = P / PPH[i]->PF[j]->data->R / T; /* start at ideal-gas density */
                rhos[0] = 1.01 * rhos[0];

				if(*err!=FPROPS_NO_ERROR){
					*err = FPROPS_NO_ERROR;
				}
				sec = secant_solve(&pressure_rho_error, &prd, rhos, tol);
				if(sec){
					return sec;
				}

				PPH[i]->rhos[j] = rhos[0];
			}
		}else if(PS->ph_type[i]==VAPOR){
			i_v = i;
		}else if(PS->ph_type[i]==LIQUID){
			i_l = i;
		}
	}
	if(i_v>-1 && i_l>-1){
		for(i=0;i<PPH[i_v]->pures;i++){
			fprops_sat_T(T, &p_sat, RHOS(i_l)+i, RHOS(i_v)+i, PPH[i_v]->PF[i], err);
		}
	}else if(i_v>-1){
		for(i=0;i<PPH[i_v]->pures;i++){
			fprops_sat_T(T, &p_sat, &rho_d, RHOS(i_v)+i, PPH[i_v]->PF[i], err);
		}
	}else if(i_l>-1){
		for(i=0;i<PPH[i_l]->pures;i++){
			fprops_sat_T(T, &p_sat, RHOS(i_l)+i, &rho_d, PPH[i_l]->PF[i], err);
		}
	}

	return 0;
#undef RHOS
#undef NPURE
#undef PPH
}

int mixture_T_ph(double *T, MixtureSpec *MS, double p, double h, double tol, FpropsError *err){
    HTData htd = {h, p, MS, tol, err};
    int sec = 0;
    double T_ph[2] = {288, 298}; /* temperatures used in searching for enthalpy */

    sec = secant_solve(&enthalpy_T_error, &htd, T_ph, tol);
    if(sec==2){
        return sec;
    }
    *T = T_d[0];
    return sec;
}

/*
	Find overall mass density of each phase of a mixture of components, and the 
	mass density of the mixture as well.
 */
double mixture_rho(PhaseMixState *PM, double *rhos){
#define PPS PM->PS
#define PPH PPS->PH
#define PXS PPH[i]->Xs
#define RHO PPH[i]->rhos
#define PPF PPH[i]->PF
#define NPURE PM->MS->pures
#define NPHASE PPS->phases
#define PPURE PPH[i]->pures
#define PFRAC PPS->ph_frac[i]

	MSG("Entered the function...");
	unsigned i, j;
	double x_mix = 0.0   /* sum over all mass fractions of phases within the mixture */
		, vol_mix = 0.0 /* volume per unit mass of the whole mixture */
		, x_ph[NPHASE]   /* sum over all mass fractions, within a single phase */
    	, vol_ph[NPHASE] /* volume per unit mass of the mixture, in a single phase */
		;
	for(i=0;i<NPHASE;i++){
		x_ph[i] = 0.0;
		vol_ph[i] = 0.0;

		for(j=0;j<PPURE;j++){
			vol_ph[i] += PPH[i]->Xs[j] / PPH[i]->rhos[j]; /* add weighted mass volume */
			x_ph[i]   += PPH[i]->Xs[j];
		}
		if(fabs(x_ph[i] - 1) > MIX_XTOL){
			ERRMSG(MIX_XSUM_ERROR, x_ph[i]);
		}
		rhos[i] = 1 / vol_ph[i];

		vol_mix += PPS->ph_frac[i] * vol_ph[i]; /* weighted mass volume for a phase */
		x_mix   += PPS->ph_frac[i];

		/* MSG("The density of the current phase is %g kg/m^3", rhos[i]); */
		/* MSG("The current total mass volume is %g m^3/kg", vol_mix); */
	}
	if(fabs(x_mix - 1) > MIX_XTOL){
		ERRMSG(MIX_PSUM_ERROR, x_mix);
	}
	return 1 / vol_mix;
}

#define MIX_FUNC_FIRST(PROP) \
	double mixture_##PROP(PhaseMixState *PM, double *p_phases, FpropsError *err){ \
		/* MSG("Entered the function..."); */ \
		unsigned i, j; \
		double x_mix = 0.0 \
			, p_mix = 0.0 /* the property being calculated -- for the entire mixture */ \
			, x_ph[NPHASE]; \
		for(i=0;i<NPHASE;i++){ \
			x_ph[i] = 0.0; \
			p_phases[i] = 0.0; \
			for(j=0;j<PPURE;j++){ \
				p_phases[i] += PXS[j] * fprops_##PROP((FluidState){PM->T,RHO[j],PPF[j]}, err); \
				x_ph[i] += PXS[j]; \
			} \
			if(fabs(x_ph[i] - 1)>MIX_XTOL){ \
				ERRMSG(MIX_XSUM_ERROR, x_ph[i]); \
			} \
			p_mix += PFRAC * p_phases[i]; \
			x_mix += PFRAC; \
		} \
		if(fabs(x_mix - 1) > MIX_XTOL){ \
			ERRMSG(MIX_PSUM_ERROR, x_mix); \
		} \
		return p_mix; \
	}

#define MIX_FUNC_SECOND(PROP,RFACTOR) \
	double mixture_##PROP(PhaseMixState *PM, double *p_phases, FpropsError *err){ \
		/* MSG("Entered the function..."); */ \
		unsigned i, j; \
		double x_mix = 0.0 \
			, p_mix = 0.0 /* the property being calculated -- for the entire mixture */ \
			, x_ph[NPHASE]; \
		for(i=0;i<NPHASE;i++){ \
			x_ph[i] = 0.0; \
			p_phases[i] = 0.0; \
			for(j=0;j<PPURE;j++){ \
				p_phases[i] += PXS[j] * fprops_##PROP((FluidState){PM->T,RHO[j],PPF[j]}, err); \
				x_ph[i] += PXS[j]; \
			} \
			if(fabs(x_ph[i] - 1)>MIX_XTOL){ \
				ERRMSG(MIX_XSUM_ERROR, x_ph[i]); \
			} \
			p_phases[i] += (RFACTOR) * mixture_x_ln_x(PPURE, PPH[i]->Xs, PPH[i]->PF); \
			p_mix += PFRAC * p_phases[i]; \
			x_mix += PFRAC; \
		} \
		if(fabs(x_mix - 1) > MIX_XTOL){ \
			ERRMSG(MIX_PSUM_ERROR, x_mix); \
		} \
		return p_mix; \
	}

MIX_FUNC_FIRST(u); MIX_FUNC_FIRST(h); MIX_FUNC_FIRST(cp); MIX_FUNC_FIRST(cv);
#define D PPF[0]->data
#define RR D->R * D->M
MIX_FUNC_SECOND(s,-RR); MIX_FUNC_SECOND(g, RR*PM->T); MIX_FUNC_SECOND(a, RR*PM->T);
#undef RR
#undef D
#undef MIX_FUNC_SECOND
#undef MIX_FUNC_FIRST

#undef PFRAC
#undef PPURE
#undef NPHASE
#undef NPURE
#undef PPF
#undef RHO
#undef PXS
#undef PPH
#undef PPS
