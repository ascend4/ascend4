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
	for(i=0; i < K->nc; ++i){
		MSG("b[%d] = %e, i = %d",i,K->ct[i].b, K->ct[i].i);
		res += K->ct[i].b * pow(Tstar, K->ct[i].i);
	}
	return exp(res);
}

double thcond1_k(FluidState state, FpropsError *err){
	// if we are here, we should be able to assume that state, should be able to remove following test (convert to assert)
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	// value for the conductivity at the zero-density limit
	double lam0 = 0;

	if(0==strcmp(state.fluid->name,"carbondioxide")){
		MSG("lam0 for carbondioxide");
		/* TODO NAUGHTY! this is code specifically for CO2... */
		int i;
		double sum1 = 0;
		double c[] = {2.387869e-2, 4.350794, -10.33404, 7.981590, -1.940558};
		for(i=1; i<=5; ++i){
			sum1 += c[i] * pow(state.T/100, 2-i);
		}
		double cint_over_k = 1.0 + exp(-183.5/state.T)*sum1;
		MSG("cint/k = %f",cint_over_k);
		double r = sqrt(0.4*cint_over_k);
		MSG("r = %f",r);
		double CS_star = thcond1_cs(k1, k1->T_star/state.T);
		lam0 = 0.475598 * sqrt(state.T) * (1 + SQ(r)) / CS_star;
		/* END of code specifically for CO2 */
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
	MSG("lam0 = %e",lam0);

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

	return k1->k_star * (lam0 + lamr + lamc);
}










