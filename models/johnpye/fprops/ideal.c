/*	ASCEND modelling environment
	Copyright (C) 2008-2011 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Ideal-gas components of helmholtz fundamental functions, calculated using
	terms in cp0 in a standard power series form. For details see the
	publications cited in the various fluid *.c files.

	John Pye, Jul 2008.
*/


#include <math.h>

#include "ideal.h"
#include "cp0.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define SQ(X) ((X)*(X))

#ifndef FPROPS_NEW
# error "Where is FPROPS_NEW??"
#endif

#ifndef FPROPS_ARRAY_COPY
# error "where is FPROPS_ARRAY_COPY??"
#endif

//static void ideal_set_reference_std(FluidData *D, const ReferenceStateStd *R);

#define IDEAL_DEBUG
#ifdef IDEAL_DEBUG
# include "color.h"
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

/*--------------------------------------------
  PREPARATION OF IDEAL RUNDATA from FILEDATA
*/

PureFluid *ideal_prepare(const EosData *E, const ReferenceState *ref){
	PureFluid *P = FPROPS_NEW(PureFluid);
	P->data = FPROPS_NEW(FluidData);
#define D P->data

	MSG("...");

	/* metadata */
	P->name = E->name;
	P->type = FPROPS_IDEAL;

	switch(E->type){
	case FPROPS_CUBIC:
		D->M = E->data.cubic->M;
		D->R = R_UNIVERSAL / D->M;
		D->T_t = 0; /* TODO how will we flag this object so that sat.c doesn't try to solve? */
		D->T_c = 0;
		D->p_c = 0;
		D->rho_c = 0;
		D->omega = 0;
		D->cp0 = cp0_prepare(E->data.cubic->ideal, D->R, D->T_c);
		D->corr.helm = NULL;
		if(ref == NULL){
			ref = &(E->data.cubic->ref);
		}
		break;
	case FPROPS_HELMHOLTZ:
		D->M = E->data.helm->M;
		if(E->data.helm->R == 0){
			P->data->R = R_UNIVERSAL / D->M;
		}else{
			P->data->R = E->data.helm->R;
		}
		D->T_t = 0;
		D->T_c = E->data.helm->T_c;
		D->p_c = 0;
		D->rho_c = E->data.helm->rho_c;
		D->omega = 0;
		D->cp0 = cp0_prepare(E->data.helm->ideal, D->R, D->T_c);
		D->corr.helm = NULL;

		if(ref == NULL){
			ref = &(E->data.helm->ref);
		}
		break;
	default:
		fprintf(stderr,"Unsupported type in ideal_prepare\n");
		FPROPS_FREE(P->data);
		FPROPS_FREE(P);
		return NULL;
	}

	// set the reference point
	switch(ref->type){
	case FPROPS_REF_PHI0:
		MSG("Applying PHI0 reference data");
		P->data->cp0->c = ref->data.phi0.c;
		P->data->cp0->m = ref->data.phi0.m;
		break;
	default:
		fprintf(stderr,"Unsupported reference point type in Helmholtz data with ideal_prepare.\n");
		FPROPS_FREE(P->data);
		FPROPS_FREE(P);
		return NULL;
	}

	/* function pointers... more to come still? */
#define FN(VAR) P->VAR##_fn = &ideal_##VAR
	FN(p); FN(u); FN(h); FN(s); FN(a); FN(g); FN(cp); FN(cv); FN(w);
	FN(dpdrho_T);
#undef FN
#undef D
	return P;
}

#if 0
void ideal_set_reference_std(FluidData *D, const ReferenceStateStd *R){
	double tau0 = D->cp0->Tstar / R->T0;
	double delta0 = R->rho0 / D->cp0->rhostar;
	D->cp0->m = R->h0 / D->R / D->cp0->Tstar;
	D->cp0->c = - R->s0 / D->R - 1 + log(tau0 / delta0);
}
#endif

double ideal_p(double T, double rho, const FluidData *data, FpropsError *err){
	return data->R * T * rho;
}

double ideal_h(double T, double rho, const FluidData *data, FpropsError *err){
	double tau = data->T_c / T;
	double delta = rho / data->rho_c;
	return data->R * T * (1 + tau * ideal_phi_tau(tau,delta,data->cp0));
}

double ideal_s(double T, double rho, const FluidData *data, FpropsError *err){
	double tau = data->T_c / T;
	double delta = rho / data->rho_c;
	return data->R * (tau * ideal_phi_tau(tau,delta,data->cp0) - ideal_phi(tau,delta,data->cp0));
}

double ideal_u(double T, double rho, const FluidData *data, FpropsError *err){
	return ideal_h(T,rho,data,err) - data->R * T;
}

double ideal_a(double T, double rho, const FluidData *data, FpropsError *err){
	return ideal_h(T,rho,data,err) - T * (data->R + ideal_s(T,rho,data,err));
}

double ideal_g(double T, double rho, const FluidData *data, FpropsError *err){
	return ideal_h(T,rho,data,err) - T * ideal_s(T,rho,data,err);
}

double ideal_cp(double T, double rho, const FluidData *data, FpropsError *err){
	double tau = data->T_c / T;
	//MSG("T = %f, T_c = %f, tau = %f",T,data->T_c,tau);
	double res = data->R * (1. - SQ(tau) * ideal_phi_tautau(tau,data->cp0));
	//MSG("R = %f, cp = %f",data->R, res);
	return res;
}

double ideal_cv(double T, double rho, const FluidData *data, FpropsError *err){
	double tau = data->T_c / T;
	return - data->R * SQ(tau) * ideal_phi_tautau(tau,data->cp0);
}

double ideal_w(double T, double rho, const FluidData *data, FpropsError *err){
	double tau = data->T_c / T;
	double w2onRT = 1. - 1. / (SQ(tau) * ideal_phi_tautau(tau,data->cp0));
	return sqrt(data->R * T * w2onRT);
}

double ideal_dpdrho_T(double T, double rho, const FluidData *data, FpropsError *err){
	return data->R * T;
}

