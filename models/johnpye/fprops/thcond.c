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

void thcond_prepare(PureFluid *P, ThermalConductivityData *K, FpropsError *err){
	MSG("Preparing thermal conductivity: currently we are just reusing the FileData pointer; no changes");
	P->thcond = K;
	
}

double thcond1_k(FluidState state, FpropsError *err){
	// if we are here, we should be able to assume that state, should be able to remove following test (convert to assert)
	if(state.fluid->thcond->type != FPROPS_THCOND_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	const ThermalConductivityData1 *k1 = &(state.fluid->thcond->data.k1);

	// value for the conductivity at the zero-density limit
	double r;
	double CS_star;
	double lam0 = 0.475598 * sqrt(state.T) * (1 + SQ(r)) / CS_star;

	// value for the residual thermal conductivity
	double lamr = 0;
	double tau = k1->T_star / state.T;
	double del = state.rho / k1->rho_star;
	int i = 0;
	for(i=0; i < k1->nr; ++i){
		double lamri = k1->rt[i].N * pow(tau, k1->rt[i].t) * pow(del, k1->rt[i].d);
		if(0 == k1->rt[i].l){
			lamr += lamri;
		}else{
			lamr += lamri * exp(-pow(del, k1->rt[i].l));
		}
	}

	double lamc = 0;
	if(!(k1->crit)){
		MSG("No critical enhancement function provided");
	}else{
		MSG("Critical enhancement function not yet implemented");
	}

	return k1->k_star * (lam0 + lamr + lamc);
}














