/*	ASCEND modelling environment
	Copyright (C) 2014 John Pye

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "thcond.h"
#include "visc.h"
#include <math.h>

#ifndef PI
# define PI M_PI
#endif

#define K_BOLTZMANN 1.3806488e-23

#define THCOND_DEBUG
#ifdef THCOND_DEBUG
# include "color.h"
# include "test.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define ASSERT(ARGS...) (void)0)
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

void thcond_prepare(PureFluid *P, const ThermalConductivityData *K, FpropsError *err){
	MSG("Preparing thermal conductivity: currently we are just reusing the FileData pointer; no changes");
	P->thcond = K;
	
}

/*-----------------------------IDEAL PART ------------------------------------*/

static double thcond1_cs(const ThermalConductivityData1 *K, double Tstar){
	double res = 0;
	int i;
	//MSG("Tstar = %f (1/Tstar = %f)",Tstar,1/Tstar);
	for(i=0; i < K->nc; ++i){
		//MSG("b[%d] = %e, i = %d, term = %f",i,K->ct[i].b, K->ct[i].i, K->ct[i].b * pow(Tstar, K->ct[i].i));
		res += K->ct[i].b * pow(Tstar, K->ct[i].i);
	}
	//MSG("res = %f",res);
	return res;
}

double thcond1_lam0(FluidState state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){*err = FPROPS_INVALID_REQUEST; return NAN;}
	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);
	double lam0 = 0;

	// TODO FIXME need to re-factor this to be standardised and only use data from filedata.h structures.

	if(0==strcmp(state.fluid->name,"carbondioxide")){
		//MSG("lam0 for carbondioxide");

//#define USE_CP0_FOR_LAM0
#ifdef USE_CP0_FOR_LAM0
		double cp0 = fprops_cp0(state,err);
		double R = state.fluid->data->R;
		//MSG("cp0 = %f, R = %f", cp0, R);
		double M = state.fluid->data->M;
		double sigma = 0.3751; // nm!
		double opr2 = 0.177568/0.475598 * cp0/R * 1 / SQ(sigma) / sqrt(M);
		//MSG("1 + r^2 = %f (by cp0)", opr2_2);
		//MSG("5/2 *(cp0(T)/R - 1) = %f\n", 5./2*(fprops_cp0(state,err)/state.fluid->data->R - 1));
#else
		int i;
		double sum1 = 0;
		double c[] = {2.387869e-2, 4.350794, -10.33404, 7.981590, -1.940558};
		for(i=0; i<5; ++i){
			sum1 += c[i] * pow(state.T/100, 2-(i+1));
		}
		double cint_over_k = 1.0 + exp(-183.5/state.T)*sum1;

		//MSG("cint/k = %f",cint_over_k);
		//MSG("1 + r^2 = %f (by cint/k)",1+0.4*cint_over_k);
		double opr2 = 1+0.4*cint_over_k;
		//double r = sqrt(0.4*cint_over_k);
		//MSG("r = %f",r);
#endif
		// FIXME convert the other way, convert the cint/k stuff to simply cp0/R, should be more generalised that way.

		double CS_star = thcond1_cs(k1, k1->eps_over_k/state.T);
		//MSG("CS_star = %f", CS_star);

		lam0 = 0.475598 * sqrt(state.T) * opr2 / CS_star;

		// 0.177568 (mW/m/K)*(nm^2)
		//lam0 = 0.177568 * sqrt(state.T) / sqrt(state.fluid->data->M) / SQ(sigma) * cp0 / R / CS_star;

	}else if(0==strcmp(state.fluid->name,"nitrogen")){
		// this uses a rather different approach/formulation; see http://ascend4.org/FPROPS/Thermal_conductivity
		MSG("lam0 for nitrogen");
		double N1 = 1.511;
		double N2 = 2.117, t2 = -1.;
		double N3 = -3.332, t3 = -0.7;
		double tau = k1->T_star / state.T;
		lam0 = N1 * (visc1_mu0(state,err)/1e-6) + N2 * pow(tau,t2) + N3 * pow(tau,t3);
	}else{
		ERRMSG("lam0 not implemented");
		*err = FPROPS_NOT_IMPLEMENTED;
		return 0;
	}
	MSG("lam0(T=%f) = %e",state.T, lam0);
	return lam0 * k1->k_star;
}


/*---------------------------RESIDUAL PART -----------------------------------*/

double thcond1_lamr(FluidState state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	// value for the residual thermal conductivity
	double lamr = 0;
	double tau = k1->T_star / state.T;
	double del = state.rho / k1->rho_star;
	int i;
	for(i=0; i < k1->nr; ++i){
		double lamri = k1->rt[i].N * pow(tau, k1->rt[i].t) * pow(del, k1->rt[i].d);
		if(0 == k1->rt[i].l){
			lamr += lamri;
		}else{
			lamr += lamri * exp(-pow(del, k1->rt[i].l));
		}
	}
	MSG("lamr(rho=%f) = %e",state.rho, lamr);		
	return lamr * k1->k_star;
}

/*---------------------------CRITICAL ENHANCEMENT-----------------------------*/
/**
	Reduced symmetrised compressibility, as described/defined in Vesovic et al.,
	1990, J Phys Chem Ref Data 19(3). This function is used in the calculation
	of the critical enhancement of thermal conductivity, in particular for 
	Carbon Dioxide.
*/
double thcond1_chitilde(FluidState state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	double p_c = state.fluid->data->p_c;
	double rho_c = state.fluid->data->rho_c;
	double T_c = state.fluid->data->T_c;
	/* FIXME we use dpdrho_T directly; assume that we have checked if we're in two-phase region or not */
	double dpdrho_T = (*(state.fluid->dpdrho_T_fn))(state.T, state.rho, state.fluid->data, err);
	//MSG("drhodp_T = %e",1/dpdrho_T);

	double chitilde = (p_c / SQ(rho_c) / T_c) * state.rho * state.T * (1. / dpdrho_T);
	//MSG("chitilde = %f",chitilde);
	return chitilde;
}

double thcond1_xi(FluidState state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	// parameters specific to CO2...
	double xi0 = 1.5e-10 /* m */;
	double Gamma = 0.052;
	double T_r = 450; /* K */

	// 'universal' parameter'...
	double nu = 0.630;
	double gamma = 1.2415;

	MSG("state: T=%f, rho=%f",state.T, state.rho);
	
	double T_orig = state.T;
	if(T_orig >= 445.){
		state.T = 445.;
	}
	FluidState state_r = state;
	state_r.T = 445.;
	MSG("state_r: T=%f, rho=%f",state_r.T, state_r.rho);
	MSG("chitilde(state) = %e", thcond1_chitilde(state,err));
	MSG("chitilde(state_r) = %e", thcond1_chitilde(state_r,err));
	MSG("chitilde(state_r)*T_r/T = %e", thcond1_chitilde(state_r,err)*T_r/state.T);

	double Delta_chitilde = thcond1_chitilde(state,err) - thcond1_chitilde(state_r,err) * T_r / state.T;
	MSG("xi0 = %e, Delta_chitilde = %f", xi0, Delta_chitilde);
	MSG("Delta_chitilde/Gamma = %e", Delta_chitilde / Gamma);
	double xi = xi0 * pow(Delta_chitilde / Gamma, nu/gamma); /* m */
	ASSERT(!isnan(xi));
	if(T_orig >= 445.){
		xi *= exp(-(T_orig - 445)/10.);
	}
	MSG("xi = %f",xi);
	return xi;
	
}

double thcond1_lamc(FluidState state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	//const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	/* use the cp/cv functions directly, to avoid bothering with saturation boundary checks (ie assume we're outside saturation region?) */
	double cp = (*(state.fluid->cp_fn))(state.T, state.rho, state.fluid->data, err);
	double cv = (*(state.fluid->cp_fn))(state.T, state.rho, state.fluid->data, err);
	double xi = thcond1_xi(state, err);
	double rho_c = state.fluid->data->rho_c;

	/* this value should be stored in k1 */
	double qtilde_D = 1./(4.0e-10); // [m^-1]

	double Omegatilde = 2/PI * ((cp-cv)/cp * atan(qtilde_D * xi) + cv/cp*qtilde_D*xi);
	double Omegatilde_0 = 2/PI * (1 - exp(-1/(1./(qtilde_D*xi) + 1./3*SQ(qtilde_D*xi*rho_c/state.rho))));

	double R = 1.01; /* 'universal' amplitude parameter, see eq 35 of Vesovic et al, 1990 */
	double mubar = visc1_mu(state, err); /* TODO make sure visc1_mu excludes critical enhancement */

	double lamc = state.rho * cp * R * K_BOLTZMANN *state.T/(6*PI*mubar*xi)* (Omegatilde - Omegatilde_0);
	return lamc;
}

/*----------------------------------OVERALL RESULT----------------------------*/

double thcond1_k(FluidState state, FpropsError *err){
	// if we are here, we should be able to assume that state, should be able to remove following test (convert to assert)
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	// value for the conductivity at the zero-density limit
	double lam0 = thcond1_lam0(state,err);

	double lamr = thcond1_lamr(state,err);

	double lamc = 0;
	if(!(k1->crit)){
		MSG("No critical enhancement function provided");
	}else{
		MSG("Critical enhancement function not yet implemented");
	}
	MSG("lamc = %e",lamc);

	return lam0 + lamr + k1->k_star * (lamc);
}










