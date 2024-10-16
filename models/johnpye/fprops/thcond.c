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
#include <assert.h>

#ifndef PI
# ifndef M_PI
#  define M_PI (3.14159265358979323846264338327950288)
# endif
# define PI M_PI
#endif

#define K_BOLTZMANN 1.3806488e-23

//#define THCOND_DEBUG
#ifdef THCOND_DEBUG
# include "color.h"
# include "test.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define ASSERT(ARGS...) ((void)0)
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

void thcond_prepare(PureFluid *P, const ThermalConductivityData *K, FpropsError *err){
	MSG("Preparing thermal conductivity: currently we are just reusing the FileData pointer; no changes");
	ASSERT(P!=NULL);
	ASSERT(K!=NULL);
	MSG("K.type: %d",K->type);
	MSG("K.source: %s",K->source);
	switch(K->type){
	case FPROPS_THCOND_1:
		MSG("K.data.k1.nc: %d",K->data.k1.nc);
		P->thcond = K;
		MSG("P.thcond.type = %d",P->thcond->type);
		return;
	case FPROPS_THCOND_POLY:
		MSG("Series with %u polynomial terms",K->data.poly.np);
		P->thcond = K;
		MSG("P.thcond.type = %d",P->thcond->type);
		return;
	case FPROPS_THCOND_NONE:
		*err = FPROPS_NOT_IMPLEMENTED;
		return;
	}
}

/*-----------------------------IDEAL PART ------------------------------------*/

static double thcond1_cs(const ThermalConductivityData1 *K, double Tstar){
	double res = 0;
	int i;
	MSG("K: %p",K);
	MSG("Tstar = %f (1/Tstar = %f)",Tstar,1/Tstar);
	MSG("nc = %d",K->nc);
	for(i=0; i < K->nc; ++i){
		//MSG("b[%d] = %e, i = %d, term = %f",i,K->ct[i].b, K->ct[i].i, K->ct[i].b * pow(Tstar, K->ct[i].i));
		res += K->ct[i].b * pow(Tstar, K->ct[i].i);
	}
	MSG("res = %f",res);
	return res;
}

double thcond1_lam0(FluidState2 state, FpropsError *err){
	double T = fprops_T(state,err);
	if(*err){
		ERRMSG("Failed to evaluate temperature");
		return NAN;
	}
	if(state.fluid->thcond->type != FPROPS_THCOND_1){*err = FPROPS_INVALID_REQUEST; return NAN;}
	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);
	double lam0 = 0;

	MSG("k1: %p",k1);

	// TODO FIXME need to re-factor this to be standardised and only use data from filedata.h structures!!

	if(0==strcmp(state.fluid->name,"carbondioxide")){
		MSG("lam0 for carbondioxide");

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
			sum1 += c[i] * pow(T/100, 2-(i+1));
		}
		double cint_over_k = 1.0 + exp(-183.5/T)*sum1;

		//MSG("cint/k = %f",cint_over_k);
		//MSG("1 + r^2 = %f (by cint/k)",1+0.4*cint_over_k);
		double opr2 = 1+0.4*cint_over_k;
		//double r = sqrt(0.4*cint_over_k);
		//MSG("r = %f",r);
#endif
		// FIXME convert the other way, convert the cint/k stuff to simply cp0/R, should be more generalised that way.

		double CS_star = thcond1_cs(k1, k1->eps_over_k/T);
		//MSG("CS_star = %f", CS_star);

		lam0 = 0.475598 * sqrt(T) * opr2 / CS_star;

		// 0.177568 (mW/m/K)*(nm^2)
		//lam0 = 0.177568 * sqrt(T) / sqrt(state.fluid->data->M) / SQ(sigma) * cp0 / R / CS_star;

	}else if(0==strcmp(state.fluid->name,"nitrogen")){
		// this uses a rather different approach/formulation; see http://ascend4.org/FPROPS/Thermal_conductivity
		MSG("lam0 for nitrogen");
		double N1 = 1.511;
		double N2 = 2.117, t2 = -1.;
		double N3 = -3.332, t3 = -0.7;
		double tau = k1->T_star / T;
		lam0 = N1 * (visc1_mu0(state,err)/1e-6) + N2 * pow(tau,t2) + N3 * pow(tau,t3);
	}else{
		ERRMSG("lam0 not implemented");
		*err = FPROPS_NOT_IMPLEMENTED;
		return 0;
	}
	MSG("lam0(T=%f) = %e",T, lam0);
	return lam0 * k1->k_star;
}


/*---------------------------RESIDUAL PART -----------------------------------*/

double thcond1_lamr(FluidState2 state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);
	double T = fprops_T(state,err);
	if(*err){
		ERRMSG("Failed to evaluate temperature");
		return NAN;
	}
	double rho = fprops_rho(state,err);
	if(*err){
		ERRMSG("Failed to evaluate temperature");
		return NAN;
	}

	// value for the residual thermal conductivity
	double lamr = 0;
	double tau = k1->T_star / T;
	double del = rho / k1->rho_star;
	int i;
	for(i=0; i < k1->nr; ++i){
		double lamri = k1->rt[i].N * pow(tau, k1->rt[i].t) * pow(del, k1->rt[i].d);
		if(0 == k1->rt[i].l){
			lamr += lamri;
		}else{
			lamr += lamri * exp(-pow(del, k1->rt[i].l));
		}
	}
	MSG("lamr(rho=%f) = %e",rho, lamr);
	return lamr * k1->k_star;
}

/*---------------------------CRITICAL ENHANCEMENT-----------------------------*/
/**
	Reduced symmetrised compressibility, as described/defined in Vesovic et al.,
	1990, J Phys Chem Ref Data 19(3). This function is used in the calculation
	of the critical enhancement of thermal conductivity, in particular for 
	Carbon Dioxide.
*/
double thcond1_chitilde(FluidState2 state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	double rho = fprops_rho(state,err);
	if(*err){
		ERRMSG("Failed to evaluate density");
		return NAN;
	}
	if(*err){
		ERRMSG("Failed to evaluate temperature");
		return NAN;
	}
	double p_c = state.fluid->data->p_c;
	double rho_c = state.fluid->data->rho_c;
	//double T_c = state.fluid->data->T_c;
	MSG("p_c = %e, rho_c = %f",p_c, rho_c);
	/* FIXME we use dpdrho_T directly; assume that we have checked if we're in two-phase region or not */
	double dpdrho_T = (*(state.fluid->dpdrho_T_fn))(state.vals, state.fluid->data, err);
	MSG("dpdrho_T = %f",dpdrho_T);

	double chitilde = p_c * rho / SQ(rho_c) / dpdrho_T;
#ifdef THCOND_DEBUG
	double T = fprops_T(state,err);
	MSG("chitilde(T=%f,rho=%f) = %f",T,rho,chitilde);
#endif
	return chitilde;
}


double thcond1_lamc(FluidState2 state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	double T = fprops_T(state,err);
	if(*err){
		ERRMSG("Failed to evaluate temperature");
		return NAN;
	}
	double rho = fprops_rho(state,err);
	if(*err){
		ERRMSG("Failed to evaluate density");
		return NAN;
	}
	//const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	/* parameters specific to CO2 */ // FIXME
	double qt_D = 4.0e-10; // [m]
	double xi0 = 1.5e-10 /* m */;
	double Gamma = 0.052;
	double T_ref = 450; /* K */

	// 'universal' parameters, 'theoretically based parameters'
	double R_0 = 1.01; /* see eq 35 of Vesovic et al, 1990 or eq 7 of Lemmon & Jacobsen 2004. */
	double nu = 0.630;
	double gamma = 1.2415;

	MSG("state: T=%f, rho=%f",T,rho);
	//const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	/* use the cp/cv functions directly, to avoid bothering with saturation boundary checks (ie assume we're outside saturation region?) */
	double cp = (*(state.fluid->cp_fn))(state.vals, state.fluid->data, err);
	double cv = (*(state.fluid->cv_fn))(state.vals, state.fluid->data, err);
	
	MSG("cp = %f",cp);
	MSG("cv = %f",cv);

#if 0
	double T_orig = state.T;
	if(T_orig >= 445.){
		state.T = 445.;
	}
#endif
	FluidState2 state_r = state;
	assert(state.fluid->type == FPROPS_HELMHOLTZ);
	state_r.vals.Trho.T = T_ref;
	MSG("state_r: T=%f, rho=%f",state_r.vals.Trho.T, state_r.vals.Trho.rho);
	//MSG("chitilde(state) = %e", thcond1_chitilde(state,err));
	//MSG("chitilde(state_r) = %e", thcond1_chitilde(state_r,err));
	//MSG("chitilde(state_r)*T_ref/T = %e", thcond1_chitilde(state_r,err)*T_ref/state.T);
	
	double brackterm = (thcond1_chitilde(state,err) - thcond1_chitilde(state_r,err) * T_ref / T) / Gamma;

	double lamc = 0;
	if(brackterm<=0){
		/* according to Lemmon & Jacobsen, we should use lamc=0 whenever brackterm <= 0 */
		MSG("brackterm<=0 -> lamc = 0");
	}else{
		double xi = xi0 * pow(brackterm, nu/gamma); /* m */
		MSG("xi = %e",xi);
		//ASSERT(!isnan(xi));
#if 0
		if(T_orig >= 445.){
			xi *= exp(-(T_orig - 445)/10.);
		}
		MSG("xi = %f",xi);
#endif

		double xioq = xi / qt_D;
		MSG("xioq = %f",xioq);

		//double xi = thcond1_xi(state, err);
		double rho_c = state.fluid->data->rho_c;
		double Omegatilde = 2/PI *( ((cp-cv)/cp) * atan(xioq) + (cv/cp)*xioq);
		double Omegatilde_0 = 2/PI * (1 - exp(-1/(1./xioq + 1./3*SQ(xioq)*SQ(rho_c/rho))));
		MSG("Omegatilde = %e",Omegatilde);
		MSG("Omegatilde_0 = %e",Omegatilde_0);

		double mu = visc1_mu(state, err); /* TODO ensure visc1_mu excludes crit enhancement! */

		lamc = rho * cp * R_0 * K_BOLTZMANN * T/(6.*PI*xi*mu)* (Omegatilde - Omegatilde_0);
	}
	return lamc;
}

double thcond1_lam(FluidState2 state, FpropsError *err){
	return thcond1_lam0(state,err) + thcond1_lamr(state,err) + thcond1_lamc(state,err);
	}

/*------------------- POLYNOMIAL THERMAL CONDUCTIVITY WRT TEMPERATURE-------------------*/

double thcond1_lam_poly(double T, const ThCondPoly *poly, FpropsError *err){
	if(poly == NULL){*err = FPROPS_INVALID_REQUEST; return NAN;}
	unsigned i;
	double sum = 0;
	double Tred = T / poly->Tstar;
	MSG("T = %f, Tstar = %f, Tred = %f",T, poly->Tstar, Tred);
	for(i=0; i<poly->np; ++i){
		sum += poly->pt[i].c * pow(Tred, poly->pt[i].n);
		MSG("c = %f, n = %d, sum = %f", poly->pt[i].c, poly->pt[i].n, sum);
	}
	return poly->kstar * sum;
}










