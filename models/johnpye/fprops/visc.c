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
	if(E->visc){
		MSG("Preparing viscosity: currently we are just reusing the FileData pointer; no changes");
		return E->visc;
	}else{
		ERRMSG("Viscosity data is missing for fluid '%s'",P->name);
		return NULL;
	}
}

/*----------------------FIRST CORRELATION---------------------------*/
/*
	perhaps v1 data should include 0.0266958/SQ(sigma)?
	(would make implementing eg Wei et al paper for Hydrogen viscosity a bit easier)
*/

double visc1_ci1(const ViscCI1Data *ci1, double Tstar){
	double res = 0;
	double lnTstar = log(Tstar);
	int i;
	for(i=0; i<ci1->nt; ++i){
		MSG("b[%d] = %e, i = %d",i,ci1->t[i].b, ci1->t[i].i);
		res += ci1->t[i].b * pow(lnTstar, ci1->t[i].i);
	}
	MSG("ln(psi) = %e",res);
	return exp(res);
}

// TODO implement this for thcond calc...
double visc1_mu0(FluidState2 state, FpropsError *err){
	if(state.fluid->visc->type != FPROPS_VISC_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}
	double T = fprops_T(state,err);
	if(*err)return NAN;
	const ViscosityData1 *v1 = &(state.fluid->visc->data.v1);
	double Omega;
	switch(v1->ci.type){
		case FPROPS_CI_1:
			Omega = visc1_ci1(&(v1->ci.data.ci1),T / v1->eps_over_k);
			break;
		default:
			*err = FPROPS_INVALID_REQUEST;
			return NAN;
	}
	MSG("M = %f, sigma = %f, Omega = %f, eps/k = %f",v1->M, v1->sigma, Omega,v1->eps_over_k);
	double mu0 = v1->mu_star * 0.0266958 * sqrt(v1->M * T) / SQ(v1->sigma) / Omega;
	MSG("const = %e", v1->mu_star * 0.0266958 * sqrt(v1->M) / SQ(v1->sigma));
	MSG("T = %e",T);
	MSG("mu0 = %g uPas",mu0 * 1e6);
	return mu0;
}
		
double visc1_mu(FluidState2 state, FpropsError *err){
	if(state.fluid->visc->type != FPROPS_VISC_1){
		*err = FPROPS_INVALID_REQUEST;
		return NAN;
	}

	double T = fprops_T(state, err);
	if(*err)return NAN;
	double rho = fprops_rho(state, err);
	if(*err)return NAN;

	MSG("T = %e, rho = %e", T, rho);
	MSG("  (--> p = %e MPa)",fprops_p(state,err)/1e6);
	const ViscosityData1 *v1 = &(state.fluid->visc->data.v1);
	double mu0 = visc1_mu0(state,err);
	double mur_red = 0;
	MSG("T_star = %f",v1->T_star);
	MSG("rho_star = %f",v1->rho_star);
	double tau = v1->T_star / T;
	double del = rho / v1->rho_star;
	MSG("tau = %e, del = %e", tau, del);
	int i;
	for(i=0; i<v1->nt; ++i){
		double mu1i = v1->t[i].N * pow(tau, v1->t[i].t) * pow(del, v1->t[i].d);
		if(0 == v1->t[i].l){
			MSG("%d: N = %e, t = %f, d = %d, l = %d --> %e", i, v1->t[i].N, v1->t[i].t, v1->t[i].d, v1->t[i].l, mu1i);
			mur_red += mu1i;
		}else{
			MSG("%d: N = %e, t = %f, d = %d, l = %d ** --> %e", i, v1->t[i].N, v1->t[i].t, v1->t[i].d, v1->t[i].l, mu1i * exp(-pow(del, v1->t[i].l)));
			mur_red += mu1i * exp(-pow(del, v1->t[i].l));
		}
	}
	/* TODO something about critical point terms? */
	/* FIXME if adding critical enhancement, note that the function thcond_lamc would need to be modified to allow the enhancement to be excluded from calculation */
	
	double mur = mur_red * v1->mu_star;
	MSG("mur/mu* = %e",mur_red);
	MSG("mu* = %g uPas",v1->mu_star);
	MSG("mur = %g uPas",mur);
	return mu0 + mur;
}

/*----------------------SECOND CORRELATION---------------------------*/

double visc2_mu(double T, const ViscDataEpt *data, FpropsError *err){
	const ViscPowTerm *pt = &(data->pt[0]);
	double term, sum = 0;
	unsigned i;
	for(i = 0; i < data->np; ++i, ++pt){
		term = pt->c * pow(T, pt->t);
		sum += term;
		MSG("i=%u: c = %f, t = %f, term = %f -> sum = %f",i,pt->c, pt->t, term, sum);
	}
	sum += data->b * log(T);
	MSG("b = %f, sum = %f", data->b, sum);
	if(data->is_ln){
		MSG("returning mu* exp(sum)");
		return data->mu_star * exp(sum);
	}else{
		return data->mu_star * sum;
	}
}
