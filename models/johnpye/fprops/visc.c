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


#include "visc.h"
#include <math.h>

//#define VISC_DEBUG
#ifdef VISC_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

const ViscosityData *visc_prepare(const EosData *E, const PureFluid *P, FpropsError *err){
	MSG("Preparing viscosity");
	return E->visc;
}

/*----------------------FIRST CORRELATION---------------------------*/
/*
	perhaps v1 data should include 0.0266958/SQ(sigma)?
*/

double visc1_ci1(const ViscCI1Data *ci1, double Tstar){
	double res = 0;
	double lnTstar = log(Tstar);
	int i;
	for(i=0; i<ci1->nt; ++i){
		MSG("b[%d] = %e, i = %d",i,ci1->t[i].b, ci1->t[i].i);
		res += ci1->t[i].b * pow(lnTstar, ci1->t[i].i);
	}
	return exp(res);
}

double visc1_mu(FluidState state, FpropsError *err){
	if(state.fluid->visc->type != FPROPS_VISC_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	double Omega;
	const ViscosityData1 *v1 = &(state.fluid->visc->data.v1);
	switch(v1->ci.type){
		case FPROPS_CI_1:
			Omega = visc1_ci1(&(v1->ci.data.ci1),state.T / v1->eps_over_k);
			break;
		default:
			*err = FPROPS_INVALID_REQUEST;
			return NAN;
	}
	MSG("M = %f, sigma = %f, Omega = %f, eps/k = %f",v1->M, v1->sigma, Omega,v1->eps_over_k);
	double mu0 = 0.0266958 * sqrt(v1->M * state.T) / SQ(v1->sigma) / Omega;
	MSG("mu0 = %e",mu0);
	double mur = 0;
	double tau = state.fluid->data->T_c / state.T;
	double del = state.rho / state.fluid->data->rho_c;
	int i;
	for(i=0; i<v1->nt; ++i){
		double mu1i = v1->t[i].N * pow(tau, v1->t[i].t) * pow(del, v1->t[i].d);
		if(0 == v1->t[i].l){
			mur += mu1i;
		}else{
			mur += mu1i * exp(-pow(del, v1->t[i].l));
		}
	}
	/* TODO something about critical point terms? */
	MSG("mur = %e",mur);
	MSG("mustar = %e",v1->mu_star);
	return v1->mu_star * (mu0 + mur);
}


