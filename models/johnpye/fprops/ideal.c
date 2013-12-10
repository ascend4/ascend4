/*	ASCEND modelling environment
	Copyright (C) 2008-2013 John Pye

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
*//** @file
	Ideal-gas components of helmholtz fundamental functions, calculated using
	terms in cp0 in a standard power series form. For details see the
	publications cited in the various fluid *.c files.

	John Pye, Jul 2008.
*/

#include <math.h>

#include "ideal.h"
#include "cp0.h"
#include "refstate.h"

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
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
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
	P->source = E->source;
	P->type = FPROPS_IDEAL;

	switch(E->type){
	case FPROPS_CUBIC:
		MSG("Cubic");
		D->M = E->data.cubic->M;
		D->R = R_UNIVERSAL / D->M;
		D->T_t = 0; /* TODO how will we flag this object so that sat.c doesn't try to solve? */
		D->T_c = 0; /* TODO we need a temperature for scaling against, what should it be if critical point is not specific, and how will it be provided? */
		D->p_c = 0;
		D->rho_c = 0;
		D->omega = 0;
		D->Tstar = 1;
		D->rhostar = 1;
		D->cp0 = cp0_prepare(E->data.cubic->ideal, D->R, D->Tstar);
		D->corr.helm = NULL;

		MSG("ref0 type = %d", E->data.cubic->ref0.type);
		D->ref0 = E->data.cubic->ref0;
		if(ref == NULL){
			ref = &(E->data.cubic->ref);
		}
		break;
	case FPROPS_HELMHOLTZ:
		MSG("Helmholtz");
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
		D->Tstar = 1;
		D->rhostar = 1;
		D->cp0 = cp0_prepare(E->data.helm->ideal, D->R, D->Tstar);
		D->corr.helm = NULL;

		if(ref == NULL){
			ref = &(E->data.helm->ref);
		}
		break;
	default:
		ERRMSG("Unsupported source data type in ideal_prepare");
		FPROPS_FREE(P->data);
		FPROPS_FREE(P);
		return NULL;
	}

	/* function pointers... more to come still? */
#define FN(VAR) P->VAR##_fn = &ideal_##VAR
	FN(p); FN(u); FN(h); FN(s); FN(a); FN(g); FN(cp); FN(cv); FN(w);
	FN(dpdrho_T);
	FN(sat);
#undef FN
#undef D

	MSG("Setting reference state...");
	// set the reference point
	switch(ref->type){
	case FPROPS_REF_PHI0:
		MSG("Applying PHI0 reference data");
		P->data->cp0->c = ref->data.phi0.c;
		P->data->cp0->m = ref->data.phi0.m;
		break;
	case FPROPS_REF_REF0:
		MSG("Applying ref0 reference state");
		switch(P->data->ref0.type){
		case FPROPS_REF_TPHG:
			{
				//MSG("TPHG");
				ReferenceState *ref0 = &(P->data->ref0);
				MSG("T0 = %f, p0 = %f, h0 = %f, g0 = %f",ref0->data.tphg.T0,ref0->data.tphg.p0,ref0->data.tphg.h0,ref0->data.tphg.g0);
				FpropsError res = FPROPS_NO_ERROR;
				double rho0 = ref0->data.tphg.p0 / P->data->R / ref0->data.tphg.T0;
				double T0 = ref0->data.tphg.T0;
				double s0 = (ref0->data.tphg.h0 - ref0->data.tphg.g0) / T0;
				double h0 = ref0->data.tphg.h0;

				P->data->cp0->c = 0;
				P->data->cp0->m = 0;
				//MSG("T0 = %f, rho0 = %f",T0,rho0);
				//MSG("btw, p = %f", P->data->R * T0 *rho0); // is OK
				res = FPROPS_NO_ERROR;
				double h1 = ideal_h(T0, rho0, P->data, &res);
				double s1 = ideal_s(T0, rho0, P->data, &res);
				if(res)ERRMSG("error %d",res);
				//MSG("h1 = %f",h1);
				P->data->cp0->c = -(s0 - s1)/P->data->R;
				P->data->cp0->m = (h0 - h1)/P->data->R/P->data->Tstar;

				h0 = ideal_h(T0,rho0, P->data, &res);
				if(res)ERRMSG("error %d",res);
				MSG("new h0(T0,rho0) = %f", h0);
				double g0 = ideal_g(T0,rho0, P->data, &res);
				if(res)ERRMSG("error %d",res);
				MSG("new g0(T0,rho0) = %f", g0);
				//MSG("DONE");
			}
			break;
		default:
			ERRMSG("Unsupported type of reference state (ref0) in ideal_prepare");
			FPROPS_FREE(P->data); FPROPS_FREE(P);
			return NULL;
		}
		break;
	default:
		ERRMSG("Unsupported type of reference state requested in ideal_prepare.\n");
		FPROPS_FREE(P->data);
		FPROPS_FREE(P);
		return NULL;
	}

	return P;
}

#define DEFINE_TAU double tau = data->Tstar / T
#define DEFINE_TAUDELTA DEFINE_TAU; double delta = rho / data->rhostar

double ideal_p(double T, double rho, const FluidData *data, FpropsError *err){
	return data->R * T * rho;
}

double ideal_h(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TAUDELTA;
	return data->R * T * (1 + tau * ideal_phi_tau(tau,delta,data->cp0));
}

double ideal_s(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TAUDELTA;
	double pht = ideal_phi_tau(tau,delta,data->cp0);
	double ph = ideal_phi(tau,delta,data->cp0);
	return data->R * (tau * ideal_phi_tau(tau,delta,data->cp0) - ideal_phi(tau,delta,data->cp0));
}

double ideal_u(double T, double rho, const FluidData *data, FpropsError *err){
	return ideal_h(T,rho,data,err) - data->R * T;
}

double ideal_a(double T, double rho, const FluidData *data, FpropsError *err){
	return ideal_h(T,rho,data,err) - T * (data->R + ideal_s(T,rho,data,err));
}

double ideal_g(double T, double rho, const FluidData *data, FpropsError *err){
	//MSG("g(T=%f,rho=%f)...",T,rho);
	double h = ideal_h(T,rho,data,err);
	double s = ideal_s(T,rho,data,err);
	//MSG("h = %f, T = %f, s = %f, h-T*s = %f",h,T,s,h-T*s);
	return h - T * s;
}

/**
	Note that this function is called by ALL fluid types via 'fprops_cp0' which
	means that it needs to include the scaling temperature within the structure;
	we can't just define Tstar as a constant for ideal fluids.
*/
double ideal_cp(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TAU;
	double res = data->R * (1. - SQ(tau) * ideal_phi_tautau(tau,data->cp0));
	return res;
}

double ideal_cv(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TAU;
	return - data->R * SQ(tau) * ideal_phi_tautau(tau,data->cp0);
}

double ideal_w(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_TAU;
	double w2onRT = 1. - 1. / (SQ(tau) * ideal_phi_tautau(tau,data->cp0));
	return sqrt(data->R * T * w2onRT);
}

double ideal_dpdrho_T(double T, double rho, const FluidData *data, FpropsError *err){
	return data->R * T;
}

double ideal_sat(double T,double *rhof_ret, double *rhog_ret, const FluidData *data, FpropsError *err){
	MSG("Ideal gas: saturation calculation is not possible");
	*err = FPROPS_RANGE_ERROR;
	return 0;
}




