/*	ASCEND modelling environment
	Copyright (C) 2011-2012 Carnegie Mellon University

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
	Implementation of the Peng-Robinson equation of state.

	For nomenclature see Sandler, 5e and IAPWS-95 and IAPWS Derivatives.

	Authors: John Pye, Richard Towers, Sean Muratet
*/

#include "pengrob.h"
#include "rundata.h"
#include "cubicroots.h"
#include "fprops.h"
#include "sat.h"
#include "ideal_impl.h"
#include "cp0.h"
#include "zeroin.h"
#include <math.h>
#include <stdio.h>

#include "helmholtz.h" // for helmholtz_prepare

/* these are the 'raw' functions, they don't do phase equilibrium. */
PropEvalFn pengrob_p;
PropEvalFn pengrob_u;
PropEvalFn pengrob_h;
PropEvalFn pengrob_s;
PropEvalFn pengrob_a;
PropEvalFn pengrob_g;
PropEvalFn pengrob_cp;
PropEvalFn pengrob_cv;
PropEvalFn pengrob_w;
PropEvalFn pengrob_dpdrho_T;
PropEvalFn pengrob_alphap;
PropEvalFn pengrob_betap;
SatEvalFn pengrob_sat;

static double MidpointPressureCubic(double T, const FluidData *data, FpropsError *err);

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
	P->source = E->source;
	P->type = FPROPS_PENGROB;

#define D P->data
	/* common data across all correlation types */
	switch(E->type){
	case FPROPS_HELMHOLTZ:
#define I E->data.helm
		D->M = I->M;
		D->R = I->R;
		D->T_t = I->T_t;
		D->T_c = I->T_c;
		D->rho_c = I->rho_c;
		D->omega = I->omega;
		D->cp0 = cp0_prepare(I->ideal, D->R, D->T_c);

		// helmholtz data doesn't include p_c so we need to calculate it somehow...

#ifdef USE_HELMHOLTZ_RHO_C
		// use Zc = 0.307 to calculate p_c from the T_c and rho_c in the
		// helmholtz data. This seems to be a bad choice
		// since eg for CO2 it gives a significantly overestimated p_c.
		{
			double Zc = 0.307;
			D->p_c = Zc * D->R * D->T_c * D->rho_c;
			MSG("p_c = %f", D->p_c);
		}
#else // USE HELMHOLTZ P_C
		// Probably the preferred alternative in this case is to
		// use rho_c and T_c to calculated helmholtz_p(T_c, rho_c), then
		// use that pressure as the PR p_c, together with updating the
		// value of rho_c for consistency with PR EOS (from the known
		// Z_c = 0.307.
		{
			FpropsError herr = FPROPS_NO_ERROR;
			MSG("Preparing helmholtz data '%s'...",E->name);
			PureFluid *PH = helmholtz_prepare(E,ref);
			if(!PH){
				ERRMSG("Failed to create Helmholtz runtime data");
				return NULL;
			}
			D->p_c = PH->p_fn(D->T_c, D->rho_c, PH->data, &herr);
			MSG("Calculated p_c = %f from Helmholtz data",D->p_c);
			if(herr){
				ERRMSG("Failed to calculate critical pressure (%s)",fprops_error(herr));
				return NULL;
			}
			double Zc = 0.307;
			D->rho_c = D->p_c / (Zc * D->R * D->T_c); 
		}			
#endif
		break;
#undef I
	case FPROPS_CUBIC:
#define I E->data.cubic
		D->M = I->M;
		D->R = R_UNIVERSAL / I->M;
		D->T_t = I->T_t;
		D->T_c = I->T_c;
		D->p_c = I->p_c;
		D->rho_c = I->rho_c;
		D->omega = I->omega;
		D->cp0 = cp0_prepare(I->ideal, D->R, D->T_c);
		break;
	default:
		fprintf(stderr,"Invalid EOS data\n");
		return NULL;
	}

	if(D->p_c == 0){
		ERRMSG("ERROR p_c is zero in this data, need to calculate it here somehow");
		return NULL;
	}

	/* FIXME note that in the following paper, the constants in the PR EOS
	are given with lots more decimal places. Need to figure out if it's
	preferable to use those extra DPs, or not...?
	http://www.che.uah.edu/courseware/che641/peng-robinson-derivatives-joule-thompson.pdf
	*/

	/* NOTE: we're using a mass basis for all our property calculations. That
	means that our 'b' is the usual 'b/M' since our 'R' is 'Rm/M'. Because of 
	this, our p is still OK, since Ru/M / (Vm/M - b/M) is still the same value.*/
#define C P->data->corr.pengrob
	C = FPROPS_NEW(PengrobRunData);
	C->aTc = 0.45724 * SQ(D->R * D->T_c) / D->p_c;
	C->b = 0.07780 * D->R * D->T_c / D->p_c;
	C->kappa = 0.37464 + (1.54226 - 0.26992 * D->omega) * D->omega;

	/* function pointers... more to come still? */
#define FN(VAR) P->VAR##_fn = &pengrob_##VAR
	FN(p); FN(u); FN(h); FN(s); FN(a); FN(g); FN(cp); FN(cv); FN(w);
	FN(dpdrho_T); FN(alphap); FN(betap);
	FN(sat);
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

#define DEFINE_V double v = 1. / rho;

/**
	Maxima code:
	a(T) := aTc * (1 + kappa * (1 - sqrt(T/Tc)));
	diff(a(T),T,1);
	XXX
*/
#define DEFINE_DADT \
	double dadT = -PD->kappa * PD->aTc * sqrtalpha / sqrt(T * PD_TCRIT)

/**
	Maxima code:
	a(T) := aTc * (1 + kappa * (1 - sqrt(T/Tc)));
	diff(a(T),T,2);
	XXX
*/
#define DEFINE_D2ADT2 \
	double d2adt2 = PD->aTc * PD->kappa * sqrt(PD_TCRIT/T) * (1 + PD->kappa) / (2 * T * PD_TCRIT);

#define DEFINE_DPDT_RHO \
	double dpdT_rho = data->R/(v - PD->b) - dadT/(v*(v + PD->b) + PD->b*(v - PD->b))

double pengrob_p(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
	double b = PD->b;
	if(rho > 1./b){
		/* TODO check this: is it because it gives p < 0? */
		MSG("Density exceeds limit value 1/b = %f",1./b);
		*err = FPROPS_RANGE_ERROR;
	}
	//MSG("v = %f, b = %f",v,b);
	double p = (data->R * T)/(v - b) - a/(v*(v + b) + b*(v - b));
	//MSG("p(T = %f, rho = %f) = %f",T,rho,p);
	return p;
}


double pengrob_h(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
	double h0 = ideal_h(T,rho,data,err);
	double p = pengrob_p(T, rho, data, err);
	double Z = p * v / (data->R * T);
	double B = p * PD->b / (data->R * T);
	DEFINE_DADT;
	double hr = data->R * T * (Z - 1) + (T*dadT - a)/(2*SQRT2 * PD->b) * log((Z + (1+SQRT2)*B) / (Z + (1-SQRT2)*B));
	return h0 + hr;
}


double pengrob_s(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_V;
	double b = PD->b;
	if(rho > 1./b){
		MSG("Density exceeds limit value 1/b = %f",1./b);
		*err = FPROPS_RANGE_ERROR;
		return 0;
	}
	double s0 = ideal_s(T,rho,data,err);
	double p = pengrob_p(T, rho, data, err);
    double Z = p * v / (data->R * T);
    double B = p * b / (data->R * T);
	DEFINE_DADT;
	//MSG("s0 = %f, p = %f, Z = %f, B = %f, dadt = %f", s0, p, Z, B, dadt);
	//MSG("log(Z-B) = %f, log((Z+(1+SQRT2)*B)/(Z+(1-SQRT2)*B = %f", log(Z-B), log((Z+(1+SQRT2)*B)/(Z+(1-SQRT2)*B)));
	double sr = data->R * log(Z-B) + dadT/(2*SQRT2*b) * log((Z+(1+SQRT2)*B)/(Z+(1-SQRT2)*B));
	return s0 + sr;
}


double pengrob_a(double T, double rho, const FluidData *data, FpropsError *err){
	// FIXME maybe we can improve this with more direct maths
	double b = PD->b;
	if(rho > 1./b){
		MSG("Density exceeds limit value 1/b = %f",1./b);
		*err = FPROPS_RANGE_ERROR;
	}
	double h = pengrob_h(T,rho,data,err);
	double s = pengrob_s(T,rho,data,err); // duplicated calculation of p!
	double p = pengrob_p(T,rho,data,err); // duplicated calculation of p!
	MSG("h = %f, p = %f, s = %f, rho = %f, T = %f",h,p,s,rho,T);
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

/**
	\f[ c_p = \left(\frac{\partial u}{\partial T} \right)_v \f]

	See Pratt, 2001. "Thermodynamic Properties Involving Derivatives",
	ASEE Chemical Engineering Division Newsletter, Malaysia
	http://www.che.uah.edu/courseware/che641/peng-robinson-derivatives-joule-thompson.pdf

	Maxima code:
	u(T,x):= (T*diff(a(T),T) - a(T))/b/sqrt(8) * log((Z(T) + B(T)*(1+sqrt(2)))/(Z(T)+B(T)*(1-sqrt(2))));
	diff(u(T,v),T);
	subst(B(T)*(alpha_p - 1/T), diff(B(T),T), %);
	subst(Z(T)*(alpha_p - 1/T), diff(Z(T),T), %);
	subst(B(T)*v/Z(T),b,%);
	subst(b*Z(T)/B(T),v,%);
	ratsimp(%);
*/
double pengrob_cv(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_V;
	DEFINE_D2ADT2;
	double cv0 = ideal_cv(T, rho, data, err);
	MSG("cv0 = %f",cv0);
#define DEFINE_CVR \
	double p = pengrob_p(T, rho, data, err); \
    double Z = p * v / (data->R * T); \
	double B = p * PD->b / (data->R * T); \
    double cvr1 = T * d2adt2/ (PD->b * 2*SQRT2); \
    double cvr2 = (Z+B*(1+SQRT2))/(Z+B*(1-SQRT2)); \
	double cvr = cvr1*log(cvr2);
	DEFINE_CVR;
	MSG("d2adT2 = %f", d2adt2);
	MSG("b = %f",PD->b);
	MSG("cvr1 = %f, cvr2 = %f, log(cvr2) = %f", cvr1, cvr2, log(cvr2));
	MSG("cvr = %f",cvr);
	return cv0 + cvr;// J/K*Kg
}

/**
	Isobaric specific heat capacity
	\f[ c_p = \left(\frac{\partial h}{\partial T} \right)_p \f]

	See Pratt, 2001. "Thermodynamic Properties Involving Derivatives",
	ASEE Chemical Engineering Division Newsletter, Malaysia
	http://www.che.uah.edu/courseware/che641/peng-robinson-derivatives-joule-thompson.pdf

	TODO this function needs to be checked.
*/
double pengrob_cp(double T, double rho, const FluidData *data, FpropsError *err){
    //these calculations are broken apart intentionally to help provide clarity as the calculations are tedious
    DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
	DEFINE_DADT;
	DEFINE_D2ADT2;
	DEFINE_CVR;
    double cp0 = ideal_cp(T, rho, data, err);
	DEFINE_DPDT_RHO;

#define DEFINE_CPR \
    double A = p * a / SQ(data->R * T); \
	double dAdT_p = p / SQ(data->R*T) * (dadT - 2*a/ T); \
	double dBdT_p = - PD->b * p / (data->R * SQ(T)); \
 	double num = dAdT_p * (B-Z) + dBdT_p * (6*B*Z + 2*Z - 3*SQ(B) - 2*B + A - SQ(Z)); \
	double den = 3*SQ(Z) + 2*(B-1)*Z + (A - 2*B - 3*SQ(B)); \
	double dZdT_p = num / den; \
	double dvdT_p = data->R / p * (T * dZdT_p + Z); \
	double cpr = cvr + T * dpdT_rho * dvdT_p - data->R
	DEFINE_CPR;
	return cp0 + cpr;
}

/**
	Speed of sound (see Çengel and Boles, 2012. "Thermodynamics, An Engineering Approach", McGraw-Hill, 7SIe):
	\f[ w = \sqrt{ \left(\frac{\partial p}{\partial \rho} \right)_s }
	= v \sqrt{ -\frac{c_p}{c_v} \left(\frac{\partial p}{\partial v}\right)_T}
	\f]

	See Pratt, 2001. "Thermodynamic Properties Involving Derivatives",
	ASEE Chemical Engineering Division Newsletter, Malaysia
	http://www.che.uah.edu/courseware/che641/peng-robinson-derivatives-joule-thompson.pdf

	TODO this function needs to be checked.
*/
double pengrob_w(double T, double rho, const FluidData *data, FpropsError *err){
    DEFINE_SQRTALPHA;
	DEFINE_V;
	DEFINE_DADT;
	DEFINE_D2ADT2;
	DEFINE_A;
	DEFINE_DPDT_RHO;
	double cv0 = ideal_cv(T, rho, data, err);
	double cp0 = cv0 + data->R;
	DEFINE_CVR;
	DEFINE_CPR;
	double k = (cp0 + cpr) / (cv0 + cvr);
	double dpdv_T = - SQ(rho) * pengrob_dpdrho_T(T,rho,data,err);
	return v * sqrt(-k * dpdv_T);
}

double pengrob_dpdrho_T(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_A;
	DEFINE_V;
#define b PD->b
	// CHECK: I check this...I think the previous expression was for dpdv_T
	return -SQ(v)*( data->R*T / SQ(v-b) - 2 * a * (v + b) / SQ(v*(v+b) + b*(v-b)));
#undef b
}

/**
	Relative pressure coefficient, as defined in http://www.iapws.org/relguide/Advise3.pdf
	\f[ \alpha_p = \frac{1}{p} \left( \frac{\partial p}{\partial T} \right)_v \f]

	Maxima code:
	p(T,v) := R*T/(v-b) - a(T)/(v*(v+b)+b*(v-b))
	alpha_p = 1/p * diff(p(T,v),T)

	TODO the function is not yet checked/tested.
*/
double pengrob_alphap(double T, double rho, const FluidData *data, FpropsError *err){
	DEFINE_SQRTALPHA;
	DEFINE_V;
	DEFINE_DADT;
	double p = pengrob_p(T, rho, data, err);
	DEFINE_DPDT_RHO;
	return 1/p * dpdT_rho;
}


/**
	Isothermal stress coefficient, as defined in http://www.iapws.org/relguide/Advise3.pdf
	\f[ \beta_p = - \frac{1}{p} \left( \frac{\partial p}{\partial v} \right)_T \f]

	Maxima code:
	p(T,v) := R*T/(v-b) - a(T)/(v*(v+b)+b*(v-b))	 
*/
double pengrob_betap(double T, double rho, const FluidData *data, FpropsError *err){
	double p = pengrob_p(T, rho, data, err);
	return -1/p * SQ(rho) * pengrob_dpdrho_T(T,rho,data,err);
}

/**
	Saturation calculation for a pure Peng-Robinson fluid. Algorithm as
	outlined in Sandler 5e, sect 7.5. Another source of information is at 
	https://www.e-education.psu.edu/png520/m17.html

	@return psat(T)
	@param T temperature [K]
	@param rhof_ret (output) saturated liquid density
	@param rhog_ret (output) saturated vapour density
	@param err (output) error flag, if any. should be reset first by caller if needed.

	TODO implement accelerated successive substitution method (ASSM) or the
	'Minimum Variable Newton Raphson (MVNR) Method' as detailed at the above
	link.

	Possible reference: Ghanbari & Davoodi Majd, 2004, "Liquid-Vapor Equilibrium
	Curves for Methane System by Using Peng-Robinson Equation of State",
	Petroleum & Coal 46(1) 23-27.
	http://www.vurup.sk/sites/vurup.sk/archivedsite/www.vurup.sk/pc/vol46_2004/issue1/pdf/pc_ghanbari.pdf
	http://www.doaj.org/doaj?func=abstract&id=251368
*/
double pengrob_sat(double T,double *rhof_ret, double *rhog_ret, const FluidData *data, FpropsError *err){
	/* rewritten, using GSOC code from Sean Muratet as a starting point -- thanks Sean! */

	if(fabs(T - data->T_c) < 1e-3){
		MSG("Saturation conditions requested at critical temperature");
		*rhof_ret = data->rho_c;
		*rhog_ret = data->rho_c;
		return data->p_c;
	}

	//double rhog;
	double A, B;
	double sqrt2 = sqrt(2);

	double p = fprops_psat_T_acentric(T, data);

	int i = 0;
	double Zg, Z1, Zf, vg, vf;
	// FIXME test upper iteration limit required
	while(++i < 100){
		MSG("iter %d: p = %f, rhof = %f, rhog = %f", i, p, 1/vf, 1/vg);
		// Peng & Robinson eq 17
		double sqrtalpha = 1 + PD->kappa * (1 - sqrt(T / PD_TCRIT));
		// Peng & Robinson eq 12
		double a = PD->aTc * SQ(sqrtalpha);
		// Peng & Robinson eq 6
        A = a * p / SQ(data->R*T);
		B = PD->b * p / (data->R*T);
		
		// use GSL function to return real roots of polynomial: Peng & Robinson eq 5
		if(3 == cubicroots(-(1-B), A-3*SQ(B)-2*B, -(A*B-SQ(B)*(1+B)), &Zf,&Z1,&Zg)){
			//MSG("    roots: Z = %f, %f, %f", Zf, Z1, Zg);
			//MSG("    Zf = %f, Zg = %f", Zf, Zg);
			// three real roots in this case
			vg = Zg*data->R*T / p;
			vf = Zf*data->R*T / p;
			if(vf < 0 || vg < 0){
				// FIXME find out what does this mean; how does it happen?
				MSG("Got a density root less than 0");
				*err = FPROPS_SAT_CVGC_ERROR;
				return 0;
			}
			//MSG("    vf = %f, vg = %f",vf,vg);
			//MSG("    VMf = %f, VMg = %f",vf*data->M,vg*data->M);

			// TODO can we use a function pointer for the fugacity expression, so that we can reuse this function for other cubic EOS?
#define FUG(Z,A,B) \
	exp( (Z-1) - log(Z - B) - A/(2*sqrt2*B)*log( (Z + (1+sqrt2) * B) / (Z + (1-sqrt2) * B)) )

			double ff = FUG(Zf,A,B);
            double fg = FUG(Zg,A,B);
			double fratio = ff/fg;
			//MSG("    ff = %f, fg = %f, fratio = %f", ff, fg, fratio);

			//double hf = pengrob_h(T, 1/vf, data, err);
			//double hg = pengrob_h(T, 1/vg, data, err);
			//MSG("    HMf = %f, HMg = %f", hf*data->M/1000, hg*data->M/1000);
		
			if(fabs(fratio - 1) < 1e-7){
				*rhof_ret = 1 / vf;
				*rhog_ret = 1 / vg;
				p = pengrob_p(T, *rhog_ret, data, err);
				MSG("Solved for T = %f: p = %f, rhof = %f, rhog = %f", T, p, *rhof_ret, *rhog_ret);
				return p;
			}
			p *= fratio;
		}else{
			/* In this case we need to adjust our guess p(T) such that we get 
			into the narrow range of values that gives multiple solutions. */
			p = MidpointPressureCubic(T, data, err);
			if(*err){
				ERRMSG("Failed to solve for a midpoint pressure");
				return p;
			}
			MSG("    single root: Z = %f. new pressure guess: %f", Zf, p);
		}
	}
	MSG("Did not converge");
	*err = FPROPS_SAT_CVGC_ERROR;
	return 0;
}

/*
	FIXME can we generalise this to work with other cubic EOS as well?
	Currently not easily done since pointers are kept at a higher level in our
	data structures than FluidData.
*/

typedef struct{
	const FluidData *data;
	FpropsError *err;
	double T;
} MidpointSolveData;

static ZeroInSubjectFunction resid_dpdrho_T;

/**
	This function is trying to find the locations of the stationary points
	of the p(rho,T) function at a specified temperature, assumed to be
	close to the critical temperature.

	We return the midpoint of the pressure between these two.
	
	It is assumed that if using this function we are close enough to the 
	critical point that vf < vc < vg, and that the stationary points
	will be within those interval, ie vf < v1 < vc < v2 < vg.
*/
double MidpointPressureCubic(double T, const FluidData *data, FpropsError *err){
	MidpointSolveData msd = {data, err, T};
	double rhomin = 0.5 * data->rho_c;
	double rhomax = data->rho_c;
	double rho, resid;

	if(T > data->T_c){
		ERRMSG("Invalid temperature T > T_c");
		*err = FPROPS_RANGE_ERROR;
		return data->p_c;
	}

	// look for a stationary point in density range less than rho_c
	int res = zeroin_solve(&resid_dpdrho_T, &msd, rhomin, rhomax, 1e-9, &rho, &resid);
	if(res){
		ERRMSG("Failed to solve density for first stationary point");
		*err = FPROPS_NUMERIC_ERROR;
		return data->p_c;
	}
	double p1 = pengrob_p(T,rho, data, err);

	// look for the other stationary point in density range less than rho_c
	rhomin = data->rho_c;
	rhomax = 2 * data->rho_c;
	if(rhomax + 1e-2 > 1./(data->corr.pengrob->b)) rhomax = 1./(data->corr.pengrob->b) - 1e-3;

	res = zeroin_solve(&resid_dpdrho_T, &msd, rhomin, rhomax, 1e-9, &rho, &resid);
	if(res){
		ERRMSG("Failed to solve density for second stationary point");
		*err = FPROPS_NUMERIC_ERROR;
		return data->p_c;
	}

	double p2 = pengrob_p(T,rho, data, err);
	return 0.5*(p1 + p2);
}


static double resid_dpdrho_T(double rho, void *user_data){
#define D ((MidpointSolveData *)user_data)
    return pengrob_dpdrho_T(D->T,rho,D->data,D->err);
#undef D
}

