/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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
	Implementation of the Peng-Robinson equation of state.

	For nomenclature see Tillner-Roth, Harms-Watzenberg and Baehr, Eine neue
	Fundamentalgleichung für Ammoniak.

	Richard Towers, 2nd June 2011
*/

#include "rundata.h"
//#include "numer.h"
#include "pengrob.h"
#include "fprops.h"
#include "sat.h"
#include "ideal_impl.h"
#include "cp0.h"
#include "helmholtz.h"
#include <math.h>
#include <stdio.h>

//#define PR_DEBUG
#define PR_ERRORS

#ifdef PR_DEBUG
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

/* TODO centralise declaration of our error-reporting function somehow...? */
#ifdef PR_ERRORS
# include "color.h"
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
#endif


PureFluid *pengrob_prepare(const EosData *E, const ReferenceState *ref){
	MSG("Preparing PR fluid...");
	PureFluid *P = FPROPS_NEW(PureFluid);
	P->data = FPROPS_NEW(FluidData);

	/* metadata */
	P->name = E->name;
	P->type = FPROPS_PENGROB;

#define D P->data
	/* common data across all correlation types */
	switch(E->type){
	case FPROPS_HELMHOLTZ:
#define I E->data.helm
		D->M = I->M;
		D->R = I->R;
		MSG("case FPROPS_HELMHOLTZ R:%f...",D->R);
		D->T_t = I->T_t;
		D->T_c = I->T_c;
		D->rho_c = I->rho_c;
		{
			FpropsError herr = FPROPS_NO_ERROR;
			MSG("Preparing helmholtz data '%s'...",E->name);
			PureFluid *PH = helmholtz_prepare(E,ref);
			if(!PH){
				ERRMSG("Failed to create Helmholtz runtime data");
			}
			D->p_c = PH->p_fn(D->T_c, D->rho_c, PH->data, &herr);
			MSG("Calculated p_c = %f from Helmholtz data",D->p_c);
			if(herr){
				ERRMSG("Failed to calculate critical pressure (%s)",fprops_error(herr));
				return NULL;
			}
		}
		D->omega = I->omega;
		D->cp0 = cp0_prepare(I->ideal, D->R, D->T_c);
		break;
#undef I
	case FPROPS_CUBIC:
#define I E->data.cubic
		D->M = I->M;
		D->R = R_UNIVERSAL/(I->M);
		D->T_t = I->T_t;
		D->T_c = I->T_c;
		D->p_c = I->p_c;
		D->rho_c = I->rho_c;
		D->omega = I->omega;
		D->cp0 = cp0_prepare_rpp(I->ideal, D->R, D->T_c); //new function to format rpp cp0 values
		break;
	default:
		fprintf(stderr,"Invalid EOS data\n");
		return NULL;
	}

	if(D->p_c == 0){
		MSG("WARNING p_c is zero in this data, need to calculate it here somehow");
	}

#define C P->data->corr.pengrob
	C = FPROPS_NEW(PengrobRunData);
	C->aTc = 0.45724 * SQ(D->R * D->T_c) / D->p_c;
	C->b = 0.07780 * D->R * D->T_c / D->p_c;
	C->kappa = 0.37464 + (1.54226 - 0.26992 * D->omega) * D->omega;

	/* function pointers... more to come still? */
#define FN(VAR) P->VAR##_fn = &pengrob_##VAR
	FN(p); FN(u); FN(h); FN(s); FN(a); FN(g); FN(cp); FN(cv); FN(w);
	FN(dpdrho_T); FN(alphap); FN(betap);
#undef FN
#undef I
#undef D
#undef C
	return P;
}


/* shortcuts take us straight into the correct data structure */
#define PD data->corr.pengrob
#define PD_TCRIT data->T_c
#define PD_RHOCRIT data->rho_c
#define PD_M data->M
#define SQRT2 1.4142135623730951

#define DEFINE_SQRTALPHA \
	double sqrtalpha = 1 + PD->kappa * (1 - sqrt(T / PD_TCRIT));
#define DEFINE_A \
	double a = PD->aTc * SQ(sqrtalpha);

#define DEFINE_V double v = 1./rho;


double pengrob_p(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
	double b = PD->b;
    if(rho > 1./b){
        *err =  FPROPS_RANGE_ERROR;
    }
	double p = (data->R * T)/(v - b) - a/(v*(v + b) + b*(v - b));
	return p;
}


double pengrob_h(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
	//must have residual at T,P;ideal cp at T and Tref, residual at ref state, and the enthalpy at ref state
	//default is NBP; rpp file suggests IIR, but may not be possible for many fluids in rpp (i.e. methane T_c >0 C).
	//double h0 = ideal_h(T,rho,data,err);
	//MSG("h0: %e,", h0);
	double p = pengrob_p(T, rho, data, err);
	double Z = p * v / (data->R * T);
	double B = p * PD->b / (data->R * T);
	double dadt = -PD->aTc * PD->kappa * sqrtalpha / sqrt(T * PD_TCRIT);
	double hr = data->R * T * (Z - 1) + (T*dadt - a)/(2*SQRT2 * PD->b) * log((Z + (1+SQRT2)*B) / (Z + (1-SQRT2)*B));
	MSG("hr: %e,", hr);
	//have residual at t,p; now must have ideal enthalpy
	double tref = tnbpref;
	MSG("tref: %e,", tref);
	//can integrate cp fairly easily analytically
	double cptref = ideal_cp0_rpp_integrate(tref, data);
	double cpt = ideal_cp0_rpp_integrate(T, data);
	MSG("cptref: %e, cpt: %e", cptref,cpt);
	double idealenthalpy = cpt - cptref;
	MSG("ideal enthalpy: %e,", idealenthalpy);
	//now subtract departure at ref state
	T=tref;
	p = 101.325e3;
	v = 1./vref;
	Z = p * v / (data->R * T);
	B = p * PD->b / (data->R * T);
	dadt = -PD->aTc * PD->kappa * sqrtalpha / sqrt(T * PD_TCRIT);
	double hrref = data->R * T * (Z - 1) + (T*dadt - a)/(2*SQRT2 * PD->b) * log((Z + (1+SQRT2)*B) / (Z + (1-SQRT2)*B));
	MSG("hrref: %e,", hrref);
	//now add enthalpy at ref state
	int href = 0;// J/Kg
	return (hr + idealenthalpy - hrref + href);
}


double pengrob_s(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_V;
    //must have residual at T,P;ideal cp at T and Tref, residual at ref state, and the entropy at ref state
	//default is NBP; rpp file suggests IIR, but may not be possible for many fluids in rpp (i.e. methane T_c >0 C).
	double p = pengrob_p(T, rho, data, err);
    double Z = p * v / (data->R * T);
    double B = p * PD->b / (data->R * T);
	double dadt = -PD->aTc * PD->kappa * sqrtalpha / sqrt(T * PD_TCRIT);
	double sr = data->R * log(Z-B) + dadt/(2*SQRT2*PD->b) * log((Z+(1+SQRT2)*B)/(Z+(1-SQRT2)*B));
    MSG("sr: %e,", sr);
	//have residual at t,p; now must have ideal entropy
	double tref = tnbpref;
	MSG("tref: %e,", tref);
	double cptref = ideal_cp0_rpp(tref, data);
	double cpt = ideal_cp0_rpp(T, data);
	double cpaverage = cpt +cptref/2;
	double idealentropy = cpaverage*log(T/tref)-data->R*log(p/101325);//maybe change to full numerical integration later
	MSG("ideal entropy: %e,", idealentropy);
	//now subtract departure at ref condition
    T=tref;
	p = 101.325e3;
	v = 1./vref;
	Z = p * v / (data->R * T);
	B = p * PD->b / (data->R * T);
	dadt = -PD->aTc * PD->kappa * sqrtalpha / sqrt(T * PD_TCRIT);
	double srref = data->R * log(Z-B) + dadt/(2*SQRT2*PD->b) * log((Z+(1+SQRT2)*B)/(Z+(1-SQRT2)*B));
	//now add entropy at reference state
	int sref = 0;// J/Kg*K
	return (sr + idealentropy - srref + sref);
}


double pengrob_a(double T, double rho, const FluidData *data, FpropsError *err){
	// FIXME maybe we can improve this with more direct maths
	double h = pengrob_h(T,rho,data,err);
	MSG("h: %e,", h);
	double s = pengrob_s(T,rho,data,err); // duplicated calculation of p!
	MSG("s: %e,", s);
	double p = pengrob_p(T,rho,data,err); // duplicated calculation of p!
	MSG("p: %e,", p);
	MSG("returning helholtz: %e,", (h - p/rho) - T * s);
	return (h - p/rho) - T * s;

//	previous code from Richard, probably fine but need to check
//	DEFINE_A;
//	double vm = PD_M / rho;
//    double p = pengrob_p(T, rho, data, err);
//    return -log(p*(v - PD->b)/(data->R * T))+a/(2*SQRT2 * PD->b * data->R *T)*log((v + (1-SQRT2)*PD->b)/(v + (1+SQRT2)*PD->b));
}


double pengrob_u(double T, double rho, const FluidData *data, FpropsError *err){
	// FIXME work out a cleaner approach to this...
	double p = pengrob_p(T, rho, data, err);
	return pengrob_h(T,rho,data,err) - p/rho; // duplicated calculation of p!

}

double pengrob_g(double T, double rho, const FluidData *data, FpropsError *err){
	double h = pengrob_h(T,rho,data,err);
	double s = pengrob_s(T,rho,data,err); // duplicated calculation of p!
	return h - T*s;
//	previous code from Richard, probably fine but need to check
//	DEFINE_A;
//	DEFINE_V;
//	double p = pengrob_p(T, rho, data, err);
//	double Z = p*v/(data->R * T);
//	double B = p*PD->b/(data->R * T);
//	double A = p * a / SQ(data->R * T);
//	return -log(fabs(Z-B))-(A/(sqrt(8)*B))*log(fabs((Z+(1+sqrt(2))*B)/(Z+(1-sqrt(2))*B)))+Z-1;
}


//Haven't worked these out yet:
double pengrob_cv(double T, double rho, const FluidData *data, FpropsError *err){
    DEFINE_SQRTALPHA;
	DEFINE_V;
	double idealheatcap = ideal_cp0_rpp(T,data)-data->R;
	double p = pengrob_p(T, rho, data, err);
    double Z = p * v / (data->R * T);
	double B = p * PD->b / (data->R * T);
    double d2adt2 = (PD->aTc*PD->kappa/2*T*sqrt(PD_TCRIT))*(sqrtalpha/sqrt(T)+PD->kappa/sqrt(PD_TCRIT));
    double heatcapr1 = T*d2adt2/(1./PD->b)*sqrt(8);
    double heatcapr = log((Z+B*(1+sqrt(2)))/(Z+B*(1-sqrt(2))));
	return idealheatcap+heatcapr*heatcapr1;// J/K*Kg
}

double pengrob_cp(double T, double rho, const FluidData *data, FpropsError *err){
    //these calculations are broken apart intentionally to help provide clarity as the calculations are tedious
    DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
    double idealheatcap = ideal_cp0_rpp(T, data);
	double p = pengrob_p(T, rho, data, err);
    double Z = p * v / (data->R * T);
    double A = p * a / SQ(data->R * T);
	double B = p * PD->b / (data->R * T);
	double dadt = -PD->aTc * PD->kappa * sqrtalpha / sqrt(T * PD_TCRIT);
    double d2adt2 = (PD->aTc*PD->kappa/2*T*sqrt(PD_TCRIT))*(sqrtalpha/sqrt(T)+PD->kappa/sqrt(PD_TCRIT));
    double M = (SQ(Z)+2*B*Z-SQ(B))/(Z-B);
    double N = (B/(PD->b*(data->R)))*dadt;
    double heatcapr = (T/2*sqrt(2)*PD->b)*d2adt2*log((Z+2.414*B)/(Z-0.414*B))+(data->R*SQ(M-N)/(SQ(M)-2*A*(Z+B)))-data->R;
    return idealheatcap + heatcapr;// J/K*Kg
}

double pengrob_w(double T, double rho, const FluidData *data, FpropsError *err){
    DEFINE_V;
	double c = sqrt(-v*v*(pengrob_cp(T, rho, data, err)/pengrob_cv(T, rho, data, err))*pengrob_dpdrho_T(T, rho, data, err));// m/s
	//double cId = sqrt(ideal_cp0_rpp(T, data)/(ideal_cp0_rpp(T,data)-data->R)*(data->R*T)); //m/s
	//MSG("Max=%e",cId);
	return c;
}

double pengrob_dpdrho_T(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
#define b PD->b
	return 2 * (v+b) * a / SQ(v*(v+b) + b*(v-b))  -  data->R * T / SQ(v-b);
#undef b
}

double pengrob_alphap(double T, double rho, const FluidData *data, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

double pengrob_betap(double T, double rho, const FluidData *data, FpropsError *err){
	*err = FPROPS_NOT_IMPLEMENTED;
	return 0;
}

#if 0
double pengrob_helm_resid(double tau, double delta, const EosData *data, FpropsError *err){
    const CubicData *d=data->cubicData;
	const PengRobCoeffs *c=d->coeffs->pengRobCoeffs;
    const CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau, rho=delta*rho_c;
    double vm=0.001*d->M/rho;
    double P=pengrob_p(tau, delta, data, err);
#define SQRT2 1.4142135623730951
    double a_depart=-log(P*(vm-PD->b)/(R_UNIVERSAL*T))+a/(2*SQRT2*PD->b*R_UNIVERSAL*T)*log((vm+(1-SQRT2)*PD->b)/(vm+(1+SQRT2)*PD->b));
#undef SQRT2
    return a_depart;
}
double pengrob_helm_resid_delta(double tau, double delta, const EosData *data, FpropsError *err){
#define DELTA .001
#define PLUS (1+DELTA/2)
#define MINUS (1-DELTA/2)
    const CubicData *d=data->cubicData;
	const PengRobCoeffs *c=d->coeffs->pengRobCoeffs;
    const CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau;
    double rho_minus=MINUS*delta*rho_c, rho_plus=PLUS*delta*rho_c;
    double V_minus=0.001*d->M/rho_minus, V_plus=0.001*d->M/rho_plus;
    double P_minus=pengrob_p(tau, delta, data, err), P_plus=pengrob_p(tau, delta, data, err);
#define SQRT2 1.4142135623730951
    double a_plus=-log(P_plus*(V_plus-PD->b)/(R_UNIVERSAL*T))+a/(2*SQRT2*PD->b*R_UNIVERSAL*T)*log((V_plus+(1-SQRT2)*PD->b)/(V_plus+(1+SQRT2)*PD->b));
    double a_minus=-log(P_minus*(V_minus-PD->b)/(R_UNIVERSAL*T))+a/(2*SQRT2*PD->b*R_UNIVERSAL*T)*log((V_minus+(1-SQRT2)*PD->b)/(V_minus+(1+SQRT2)*PD->b));
#undef SQRT2
    return (a_plus-a_minus)/(DELTA*tau);
#undef DELTA
#undef PLUS
#undef MINUS
}
double pengrob_helm_resid_tau(double tau, double delta, const EosData *data, FpropsError *err){
#define DELTA .001
#define PLUS (1+DELTA/2)
#define MINUS (1-DELTA/2)
    const CubicData *d=data->cubicData;
	const PengRobCoeffs *c=d->coeffs->pengRobCoeffs;
    const CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T, rho_c=crit->rho;
    double T=T_c/tau;
    double rho_minus=MINUS*delta*rho_c, rho_plus=PLUS*delta*rho_c;
    double V_minus=0.001*d->M/rho_minus, V_plus=0.001*d->M/rho_plus;
    double P_minus=pengrob_p(tau, delta, data, err), P_plus=pengrob_p(tau, delta, data, err);
#define SQRT2 1.4142135623730951
    double a_plus=-log(P_plus*(V_plus-PD->b)/(R_UNIVERSAL*T))+a/(2*SQRT2*PD->b*R_UNIVERSAL*T)*log((V_plus+(1-SQRT2)*PD->b)/(V_plus+(1+SQRT2)*PD->b));
    double a_minus=-log(P_minus*(V_minus-PD->b)/(R_UNIVERSAL*T))+a/(2*SQRT2*PD->b*R_UNIVERSAL*T)*log((V_minus+(1-SQRT2)*PD->b)/(V_minus+(1+SQRT2)*PD->b));
#undef SQRT2
    return (a_plus-a_minus)/(DELTA*tau);
#undef DELTA
#undef PLUS
#undef MINUS
}
#endif


/**
 TODO: Add saturation region calculations...
 */
#if 0
double pengrob_Z  (double tau, double   p, const EosData *data, FpropsError *err){
    CubicData *d=data->cubicData; PengRobCoeffs *c=d->coeffs->pengRobCoeffs;
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T;
    double T=T_c/tau;
    //Check that coefficients were calculated at the correct temperature:
    if(c->T_last != T) pengrob_refresh_coeffs(T, c, d, err);

#define R R_UNIVERSAL
    double B=p*PD->b/(R*T), A=a*p/(R*T*R*T);
#undef R
    //Form the polynomial:
    CubicCoeffs poly={1,B-1,A-3*B*B-2*B,-A*B+B*B+B*B*B};
    //Solve and return (possibly complex) roots:
    ComplexRoots roots=solve_cubic(poly);

    //If we have three real roots then we're in the saturation region:
    if(cimag(roots.r1)==0 && cimag(roots.r2)==0 && cimag(roots.r3)==0){
        //TODO: Saturation stuff here...
        return 0;
    }
    //Otherwise simply return the one real root:
    else{
        if(cimag(roots.r1)==0) return creal(roots.r1);
        else if (cimag(roots.r2)==0) return creal(roots.r2);
        else return creal(roots.r3);
    }
}

double pengrob_Vm (double tau, double   p, const EosData *data, FpropsError *err){
    CriticalData *crit=data->cubicData->critical;
    double T_c=crit->T;
    double T=T_c/tau;
    return pengrob_Z(tau, p, data, err)*R_UNIVERSAL*T/p;
}

double pengrob_rho(double tau, double   p, const EosData *data, FpropsError *err){
    CubicData *d=data->cubicData;
    return d->M/pengrob_Vm(tau, p, data, err);
}
#endif
