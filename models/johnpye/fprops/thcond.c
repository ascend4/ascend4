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

#define THCOND_DEBUG
#ifdef THCOND_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

void thcond_prepare(PureFluid *P, const ThermalConductivityData *K, FpropsError *err){
	MSG("Preparing thermal conductivity: currently we are just reusing the FileData pointer; no changes");
	P->thcond = K;
	
}

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

double thcond1_k0(FluidState state, FpropsError *err){
	if(state.fluid->thcond->type != FPROPS_THCOND_1){*err = FPROPS_INVALID_REQUEST; return NAN;}
	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);
	double lam0 = 0;

	// TODO FIXME need to re-factor this to be standardised and only use data from filedata.h structures.

	if(0==strcmp(state.fluid->name,"carbondioxide")){
		//MSG("lam0 for carbondioxide");
		int i;
		double sum1 = 0;
		double c[] = {2.387869e-2, 4.350794, -10.33404, 7.981590, -1.940558};
		for(i=0; i<5; ++i){
			sum1 += c[i] * pow(state.T/100, 2-(i+1));
		}
		double cint_over_k = 1.0 + exp(-183.5/state.T)*sum1;

		//MSG("cint/k = %f",cint_over_k);
		//MSG("1 + r^2 = %f (by cint/k)",1+0.4*cint_over_k);

		//double cp0 = fprops_cp0(state,err);
		//double R = state.fluid->data->R;
		//MSG("cp0 = %f, R = %f", cp0, R);
		//double M = state.fluid->data->M;
		//double sigma = 0.3751; // nm!
		//double opr2_2 = 0.177568/0.475598 * cp0/R * 1 / SQ(sigma) / sqrt(M);
		//MSG("1 + r^2 = %f (by cp0)", opr2_2);
		//MSG("5/2 *(cp0(T)/R - 1) = %f\n", 5./2*(fprops_cp0(state,err)/state.fluid->data->R - 1));

		double r = sqrt(0.4*cint_over_k);
		//MSG("r = %f",r);
		double CS_star = thcond1_cs(k1, k1->eps_over_k/state.T);
		//MSG("CS_star = %f", CS_star);
		//double sigma = 0.3751e-9;
		lam0 = 0.475598 * sqrt(state.T) * (1 + 0.4*cint_over_k) / CS_star;

		// 0.177568 (mW/m/K)*(nm^2)
		//lam0 = 0.177568 * sqrt(state.T) / sqrt(state.fluid->data->M) / SQ(sigma) * cp0 / R / CS_star;

	}else if(0==strcmp(state.fluid->name,"nitrogen")){
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
	return lam0 * k1->k_star;
}


double thcond1_k(FluidState state, FpropsError *err){
	// if we are here, we should be able to assume that state, should be able to remove following test (convert to assert)
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	// value for the conductivity at the zero-density limit
	double lam0 = thcond1_k0(state,err);
	MSG("lam0(%f) = %e",state.T, lam0);

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
	MSG("lamr = %e",lamr);

	double lamc = 0;
	if(!(k1->crit)){
		MSG("No critical enhancement function provided");
	}else{
		MSG("Critical enhancement function not yet implemented");
	}
	MSG("lamc = %e",lamc);

	return lam0 + k1->k_star * (lamr + lamc);
}










