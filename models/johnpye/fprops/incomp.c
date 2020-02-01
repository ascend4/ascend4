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
	Incompressible fluid  components of helmholtz fundamental functions, calculated using
	terms in cp0 in a standard power series form. For details see the
	publications cited in the various fluid *.c files.

	John Pye, Jul 2008.
*/

#include <math.h>

#include "incomp.h"
#include "cp0.h"
#include "refstate.h"
#include "rundata.h"

/* these are the 'raw' functions, they don't do phase equilibrium. */
PropEvalFn2 incomp_rho;
PropEvalFn2 incomp_T;
PropEvalFn2 incomp_p;
PropEvalFn2 incomp_u;
PropEvalFn2 incomp_h;
PropEvalFn2 incomp_s;
PropEvalFn2 incomp_a;
PropEvalFn2 incomp_g;
PropEvalFn2 incomp_w;
PropEvalFn2 incomp_cp;
PropEvalFn2 incomp_cv;
PropEvalFn2 incomp_dpdrho_T;
SatEvalFn incomp_sat;

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

//static void incomp_set_reference_std(FluidData *D, const ReferenceStateStd *R);

//#define INCOMP_DEBUG
#ifdef INCOMP_DEBUG
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

PureFluid *incomp_prepare(const EosData *E, const ReferenceState *ref){
	PureFluid *P = FPROPS_NEW(PureFluid);
	P->data = FPROPS_NEW(FluidData);
#define D P->data
#define I E->data.incomp

	MSG("E->data.incomp = %p",I);

	/* metadata */
	MSG("E->name = %s",E->name);
	P->name = E->name;
	P->source = E->source;
	P->type = FPROPS_INCOMP;

	switch(E->type){
	case FPROPS_INCOMP:
		MSG("Incomp");
		break;
	default:
		ERRMSG("Unsupported source data type in incomp_prepare");
		FPROPS_FREE(P->data);
		FPROPS_FREE(P);
		return NULL;
	}

	D->M = I->M;
	D->R = I->cp0.cp0star;
	D->Tstar = I->cp0.Tstar;

	D->rhostar = NAN;
	D->T_t = NAN;
	D->rho_c = NAN;
	D->omega = NAN;

	if(NULL == &(I->rho)){
		ERRMSG("Density null in the provided filedata");
		cp0_destroy(D->cp0);
		FPROPS_FREE(P->data); FPROPS_FREE(P);
		return NULL;
	}

	IncompRunData *R = FPROPS_NEW(IncompRunData);
	D->corr.incomp = R;

	/* FIXME use a different approach for cp0 */
#if 0
	IdealData *J = FPROPS_NEW(IdealData);
	J->data.cp0 = I->cp0;
	J->type = IDEAL_CP0;
	D->cp0 = cp0_prepare(J, D->R, I->cp0.Tstar);
	FPROPS_FREE(J);
#else
	MSG("filedata for cp0 = %p (np = %u)",&(I->cp0),I->cp0.np);
	D->corr.incomp->cp0 = &(I->cp0);
	MSG("rundata np = %u",D->corr.incomp->cp0->np);
#endif

	//MSG("P->data->corr.incomp = %p",P->data->corr.incomp);
	//MSG("I->rho = %p",&(I->rho));
	//MSG("I->rho.Tstar = %f",I->rho.Tstar);
	//MSG("I->rho.np = %u",I->rho.np);

	R->rho = I->rho;

	//MSG("D->corr.incomp->rho.np = %u",D->corr.incomp->rho.np);

	/* function pointers... more to come still? */
#define FN(VAR) P->VAR##_fn = &incomp_##VAR
	FN(p); FN(u); FN(h); FN(s); FN(a); FN(g); FN(cp); FN(cv); FN(w);
	FN(rho);
	FN(dpdrho_T);
	FN(sat);
#undef FN
	P->setref_fn = &refstate_set_for_incomp;

	MSG("Setting reference state...");
	// fix up the reference point now...
	if(ref == NULL){
		MSG("Using default if available");
		// use the provided ReferenceState, or the default one otherwise.
		ref = &(I->ref);
		if(ref){
			MSG("Default reference type %d found",I->ref.type);
		}
	}
	int res = fprops_set_reference_state(P,ref);
	if(res){
		ERRMSG("Error applying reference state (type %d, err %d)",ref->type,res);
		return NULL;
	}

#undef D
	MSG("Returning P, with P->name = %s",P->name);
	//MSG("P->data->corr.incomp = %p",P->data->corr.incomp);

	return P;
}


#define DEFINE_T double T = vals.Tp.T
#define DEFINE_P double p = vals.Tp.p
#define DEFINE_TAU DEFINE_T; double tau = data->Tstar / T
//#define DEFINE_TAUDELTA DEFINE_TAU; DEFINE_RHO; double delta = rho / data->rhostar


double incomp_p(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	//assert(data->type == FPROPS_INCOMP);
	return vals.Tp.p;
}

double incomp_T(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	//assert(data->type == FPROPS_INCOMP);
	return vals.Tp.T;
}

double incomp_rho(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	//assert(data->type == FPROPS_INCOMP);
	//MSG("...");
#define D data->corr.incomp
	//MSG("D = %p",D);
	const unsigned np = D->rho.np;
	double sum;
	double Tred;
	//MSG("type = %d", D->rho.type);
	//MSG("T* = %f",D->rho.Tstar);

	switch(D->rho.type){
	case FPROPS_DENS_T: // series in terms of T/Tstar
		Tred = vals.Tp.T / D->rho.Tstar;
		//MSG("Tred = T/T* = %f",Tred);
		break;
	case FPROPS_DENS_1MT: // series in terms of [1 - T/Tstar]
		Tred = 1 - vals.Tp.T / D->rho.Tstar;
		//MSG("Tred = 1 - T/T* = %f",Tred);
		break;
	default:
		*err = FPROPS_NOT_IMPLEMENTED;
		return -1;
	}

	sum=0;
	for(int i=0; i<np; ++i){
		sum += D->rho.pt[i].c * pow(Tred, D->rho.pt[i].n);
		//MSG("i = %u, c = %f, n = %f --> sum = %f", i, D->rho.pt[i].c, D->rho.pt[i].n,sum);
	}
	return D->rho.rhostar * sum;
#undef D
}

double incomp_h(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	//MSG("Calculating h(T = %f, p = %f), T* = %f",vals.Tp.T, vals.Tp.p, data->Tstar);
	return cp0_h(vals.Tp.T, data->corr.incomp->cp0, data->corr.incomp->const_h);
	// TODO ReferenceState and cp0 implementation need more work for the incompressible case.
}

double incomp_s(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	MSG("Calculating s(T = %f, p = %f), T* = %f",vals.Tp.T, vals.Tp.p, data->Tstar);
	return cp0_s(vals.Tp.T, data->corr.incomp->cp0, data->corr.incomp->const_s);
}

double incomp_u(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	return incomp_h(vals, data,err) - vals.Tp.p / incomp_rho(vals,data,err);
}

double incomp_a(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	ERRMSG("Not implemented");
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double incomp_g(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	ERRMSG("Not implemented");
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double incomp_cp(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	MSG("cp0_eval at T = %f, cp0 = %p",vals.Trho.T, data->corr.incomp->cp0);
	return cp0_cp(vals.Trho.T, data->corr.incomp->cp0);
}

double incomp_cv(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	ERRMSG("Not implemented");
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double incomp_w(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	ERRMSG("Not implemented");
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double incomp_dpdrho_T(FluidStateUnion vals, const FluidData *data, FpropsError *err){
	ERRMSG("Not implemented");
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double incomp_sat(double T,double *rhof_ret, double *rhog_ret, const FluidData *data, FpropsError *err){
	ERRMSG("Not implemented");
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}
