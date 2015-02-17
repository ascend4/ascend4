/*	ASCEND modelling environment
	Copyright (C) 2008-2009 Carnegie Mellon University

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
	Routines to calculate saturation properties using Helmholtz correlation
	data. We first include some 'generic' saturation equations that make use
	of the acentric factor and critical point properties to predict saturation
	properties (pressure, vapour density, liquid density). These correlations
	seem to be only very rough in some cases, but it is hoped that they will
	be suitable as first-guess values that can then be fed into an iterative
	solver to converge to an accurate result.
*/

#include "rundata.h"
#include "sat.h"
#include "fprops.h"
#include "zeroin.h"
#include "pengrob.h"

//#define SAT_DEBUG
#ifdef SAT_DEBUG
# include <assert.h>
#else
# define assert(ARGS...)
#endif

#include <math.h>
#include <stdio.h>

#define SQ(X) ((X)*(X))

#define TCRIT(DATA) (DATA->data->T_c)
#define PCRIT(DATA) (DATA->data->p_c)
#define RHOCRIT(DATA) (DATA->data->rho_c)
#define OMEGA(DATA) (DATA->data->omega)
#define RGAS(DATA) (DATA->data->R)
#define TTRIP(DATA) (DATA->data->T_t)

//#define THROW_FPE

#ifdef THROW_FPE
#define _GNU_SOURCE
#include <fenv.h>
int feenableexcept (int excepts);
int fedisableexcept (int excepts);
int fegetexcept (void);
#endif

# include "color.h"

#ifdef SAT_DEBUG
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

#ifdef SAT_ERRORS
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
#endif

/**
	Estimate of saturation pressure using H W Xiang ''The new simple extended
	corresponding-states principle: vapor pressure and second virial
	coefficient'', Chemical Engineering Science,
	57 (2002) pp 1439-1449.
*/
double fprops_psat_T_xiang(double T, const PureFluid *d){
	double Zc = PCRIT(d) / (8314. * RHOCRIT(d) * TCRIT(d));

#ifdef TEST
	fprintf(stderr,"Zc = %f\n",Zc);
#endif

	double theta = SQ(Zc - 0.29);

#ifdef TEST
	fprintf(stderr,"theta = %f\n",theta);
#endif

	double aa[] = {5.790206, 4.888195, 33.91196
	            , 6.251894, 15.08591, -315.0248
	            , 11.65859, 46.78273, -1672.179
	};

	double a0 = aa[0] + aa[1]*OMEGA(d) + aa[2]*theta;
	double a1 = aa[3] + aa[4]*OMEGA(d) + aa[5]*theta;
	double a2 = aa[6] + aa[7]*OMEGA(d) + aa[8]*theta;

	double Tr = T / TCRIT(d);
	double tau = 1 - Tr;

	double taupow = pow(tau, 1.89);
	double temp = a0 + a1 * taupow + a2 * taupow * taupow * taupow;

	double logpr = temp * log(Tr);
	double p_r = exp(logpr);

#ifdef TEST
	fprintf(stderr,"a0 = %f\n", a0);
	fprintf(stderr,"a1 = %f\n", a1);
	fprintf(stderr,"a2 = %f\n", a2);
	fprintf(stderr,"temp = %f\n", temp);
	fprintf(stderr,"T_r = %f\n", Tr);
	fprintf(stderr,"p_r = %f\n", p_r);
#endif

	return p_r * PCRIT(d);
}

/**
	Estimate saturation pressure using acentric factor. This algorithm
	is used for first estimates for later refinement in the program REFPROP.
*/
double fprops_psat_T_acentric(double T, const PureFluid *d){
	/* first guess using acentric factor */
	double p = PCRIT(d) * pow(10, -7./3 * (1.+OMEGA(d)) * (TCRIT(d) / T - 1.));
	return p;
}


/**
	Saturated liquid density correlation of Rackett, Spencer & Danner (1972)
	see http://dx.doi.org/10.1002/aic.690250412
*/
double fprops_rhof_T_rackett(double T, const PureFluid *d){
	MSG("RHOCRIT=%f, RGAS=%f, TCRIT=%f, PCRIT=%f",RHOCRIT(d),RGAS(d),TCRIT(d),PCRIT(d));
	double Zc = RHOCRIT(d) * RGAS(d) * TCRIT(d) / PCRIT(d);
	double Tau = 1. - T/TCRIT(d);
	MSG("Zc = %f, Tau = %f",Zc,Tau);
	double vf = (RGAS(d) * TCRIT(d) / PCRIT(d)) * pow(Zc, -1 - pow(Tau, 2./7));
	MSG("got vf(T=%f) = %f",T,vf);
	return 1./vf;
}

/*
	TODO add Yamada & Gunn sat rhof equation eg from RPP5 eq 4-11.4a, should
	be more accurate?
*/

/**
	Inverse of fprops_rhof_T_rackett. TODO: this need checking.
*/
double fprops_T_rhof_rackett(double rhof, const PureFluid *d){
	double Zc = RHOCRIT(d) * RGAS(d) * TCRIT(d) / PCRIT(d);
	double f1 = PCRIT(d) / RGAS(d) / TCRIT(d) / rhof;
	double f2 = -log(f1)/log(Zc);
	return pow(f2 -1, 3./2);
}

/**
	Saturated vapour density correlation of Chouaieb, Ghazouani, Bellagi
	see http://dx.doi.org/10.1016/j.tca.2004.05.017
*/
double fprops_rhog_T_chouaieb(double T, const PureFluid *d){
	double Tau = 1. - T/TCRIT(d);
#if 0
	double Zc = RHOCRIT(d) * RGAS(d) * TCRIT(d) / PCRIT(d);
# define N1 -0.1497547
# define N2 0.6006565
# define P1 -19.348354
# define P2 -41.060325
# define P3 1.1878726
	double MMM = 2.6; /* guess, reading from Chouaieb, Fig 8 */
	//MMM = 2.4686277;
	double PPP = Zc / (P1 + P2*Zc*log(Zc) + P3/Zc);
	fprintf(stderr,"PPP = %f\n",PPP);
	//PPP = -0.6240188;
	double NNN = PPP + 1./(N1*OMEGA(d) + N2);
#else
/* exact values from Chouaieb for CO2 */
# define MMM 2.4686277
# define NNN 1.1345838
# define PPP -0.6240188
#endif

	double alpha = exp(pow(Tau,1./3) + sqrt(Tau) + Tau + pow(Tau, MMM));
	return RHOCRIT(d) * exp(PPP * (pow(alpha,NNN) - exp(1-alpha)));
}


/**
	Solve saturation condition for a specified temperature using approach of
	Akasaka, but adapted for general use to non-helmholtz property correlations.
	@param T temperature [K]
	@param psat_out output, saturation pressure [Pa]
	@param rhof_out output, saturated liquid density [kg/m^3]
	@param rhog_out output, saturated vapour density [kg/m^3]
	@param d helmholtz data object for the fluid in question.
	@return 0 on success, non-zero on error (eg algorithm failed to converge, T out of range, etc.)
*/
int fprops_sat_T(double T, double *psat_out, double *rhof_out, double * rhog_out, const PureFluid *d){
	// FIXME sat.c functions should take err pointer too...
	FpropsError err = FPROPS_NO_ERROR;
	//double tau = TCRIT(d) / T;

	if(T < TTRIP(d) - 1e-8){
		ERRMSG("Input temperature %f K is below triple-point temperature %f K",T,TTRIP(d));
		return FPROPS_RANGE_ERROR;
	}

	if(T > TCRIT(d)){
		ERRMSG("Input temperature is above critical point temperature");
		return FPROPS_RANGE_ERROR;
	}

	// we're at the critical point
	if(fabs(T - TCRIT(d)) < 1e-9){
		*psat_out = PCRIT(d);
		*rhof_out = RHOCRIT(d);
		*rhog_out = RHOCRIT(d);
		return 0;
	}

	// initial guesses for liquid and vapour density
	double rhof = 1.1 * fprops_rhof_T_rackett(T,d);
	double rhog= 0.9 * fprops_rhog_T_chouaieb(T,d);
	double R = d->data->R;
	double pc = d->data->p_c;

#ifdef SAT_DEBUG
	MSG("initial guess rho_f = %f, rho_g = %f\n",rhof,rhog);
	MSG("calculating for %s, T = %.12e",d->name,T);
#endif

	int i = 0;
	while(i++ < 30){
		assert(!isnan(rhog));
		assert(!isnan(rhof));
#ifdef SAT_DEBUG
		MSG("iter %d: T = %f, rhof = %f, rhog = %f",i,T, rhof, rhog);
#endif

		double pf = d->p_fn(T,rhof,d->data,&err);
		MSG("pf= %e",pf);
		double pg = d->p_fn(T,rhog,d->data,&err);
		MSG("pg= %e",pg);
		double gf = d->a_fn(T,rhof,d->data,&err) + pf/rhof;
		MSG("gf= %e",gf);
		double gg = d->a_fn(T,rhog,d->data,&err) + pg/rhog;
		MSG("gg= %e",gg);
		double dpdrf = d->dpdrho_T_fn(T,rhof,d->data,&err);
		MSG("dpdrf= %e",dpdrf);
		double dpdrg = d->dpdrho_T_fn(T,rhog,d->data,&err);
		MSG("dpdrg= %e",dpdrg);

		// jacobian for [F;G](rhof, rhog) --- derivatives wrt rhof and rhog
		double F = (pf - pg)/pc;
		double G = (gf - gg)/R/T;

		if(fabs(F) + fabs(G) < 1e-12){
			//fprintf(stderr,"%s: CONVERGED\n",__func__);
			*rhof_out = rhof;
			*rhog_out = rhog;
			*psat_out = d->p_fn(T, *rhog_out,d->data,&err);
			return err; /* success */
		}

		double Ff = dpdrf/pc;
		double Fg = -dpdrg/pc;
		//MSG("Ff = %e, Fg = %e",Ff,Fg);

		double Gf = dpdrf/rhof/R/T;
		double Gg = -dpdrg/rhog/R/T;
		//MSG("Gf = %e, Gg = %e",Gf,Gg);

		double DET = Ff*Gg - Fg*Gf;
		//MSG("DET = %f",DET);
#define gamma 1.
		rhof += gamma/DET * (Fg*G - Gg*F);
		rhog += gamma/DET * ( Gf*F - Ff*G);
#undef gamma

		assert(!isnan(rhof));
		assert(!isnan(rhog));

		if(rhog < 0)rhog = -0.5*rhog;
		if(rhof < 0)rhof = -0.5*rhof;
	}
	*rhof_out = rhof;
	*rhog_out = rhog;
	*psat_out = d->p_fn(T, rhog,d->data, &err);
	ERRMSG("Not converged: '%s' with T = %e (rhof=%f, rhog=%f).",d->name,T,*rhof_out,*rhog_out);
	return FPROPS_SAT_CVGC_ERROR;

}

/**
	Calculate the triple point pressure and densities using T_t from the FluidData.
*/
int fprops_triple_point(double *p_t_out, double *rhof_t_out, double *rhog_t_out, const PureFluid *d){
	static const PureFluid *d_last = NULL;
	static double p_t, rhof_t, rhog_t;
	if(d == d_last){
		*p_t_out = p_t;
		*rhof_t_out = rhof_t;
		*rhog_t_out = rhog_t;
		return 0;
	}
	MSG("Calculating saturation for '%s' (T_c = %f, p_c = %f) at T = %f",d->name, d->data->T_c, d->data->p_c, d->data->T_t);
	int res = fprops_sat_T(d->data->T_t, &p_t, &rhof_t, &rhog_t,d);
	if(res)return res;
	else{
		d_last = d;
		*p_t_out = p_t;
		*rhof_t_out = rhof_t;
		*rhog_t_out = rhog_t;
		return 0;
	}
}


typedef struct{
	const PureFluid *P;
	double p;
} SatPResidData;
static ZeroInSubjectFunction sat_p_resid;
static double sat_p_resid(double T, void *user_data){
#define D ((SatPResidData *)user_data)
	double p, rhof, rhog;
	if(fprops_sat_T(T, &p, &rhof, &rhog, D->P))return -1;
	return p - D->p;
#undef D
}
static double sat_p_resid_cubic(double T, void *user_data){
#define D ((SatPResidData *)user_data)
	double p, rhof, rhog;
	if(fprops_sat_T_cubic(T, &p, &rhof, &rhog, D->P))return -1;
	return p - D->p;
#undef D
}

int fprops_sat_p(double p, double *T_sat, double *rho_f, double *rho_g, const PureFluid *P){
	SatPResidData D = {P, p};
	double p1, T, resid;
	int err;
	switch(P->type){
    case 2:
       MSG("in sat p case 2");
	   err = zeroin_solve(&sat_p_resid_cubic, &D, P->data->T_t, P->data->T_c, 1e-5, &T, &resid);
       err = fprops_sat_T_cubic(T, &p1, rho_f, rho_g, P);
       MSG("Reading type peng robinson");
       break;

    default:
	   err = zeroin_solve(&sat_p_resid, &D, P->data->T_t, P->data->T_c, 1e-5, &T, &resid);
	   if(err){
		   fprintf(stderr,"%s: Failed to solve (p = %f)\n",__func__,p);
           return 100 + err;
	   }
	   err = fprops_sat_T(T, &p1, rho_f, rho_g, P);
	}
	if(!err)*T_sat = T;
	return err;
}


/**
	Calculate Tsat based on a value of hf. This value is useful in setting
	first guess Temperatures when solving for the coordinates (p,h).
	This function uses the secant method for the iterative solution.

	FIXME convert this to using FpropsError struct.
*/
int fprops_sat_hf(double hf, double *Tsat_out, double *psat_out, double *rhof_out, double *rhog_out, const PureFluid *P){
	FpropsError err;
	double T1 = 0.4 * P->data->T_t + 0.6 * P->data->T_c;
	double T2 = P->data->T_t;
	double h1, h2, p, rhof, rhog;
	int res = fprops_sat_T(T2, &p, &rhof, &rhog, P);
	if(res){
		ERRMSG("Failed to solve psat(T_t = %.12e) for %s",T2,P->name);
		return 1;
	}
	double tol = 1e-6;
	h2 = P->h_fn(T2,rhof,P->data, &err);
	if(err){
		ERRMSG("Unable to calculate h(T=%f K,rhof=%f kg/m3",T2,rhof);
		return 3;
	}
	if(hf < h2){
		ERRMSG("Value given for hf = %.12e is below that calculated for triple point liquid hf_t = %.12e",hf,h2);
		return 2;
	}

	int i = 0;
	while(i++ < 60){
		assert(T1 >= P->data->T_t - 1e-4);
		assert(T1 <= P->data->T_c);
		MSG("T1 = %f\n",T1);
		res = fprops_sat_T(T1, &p, &rhof, &rhog, P);
		if(res){
			ERRMSG("Failed to solve psat(T = %.12e) for %s",T1,P->name);
			return 1;
		}
		h1 = P->h_fn(T1,rhof, P->data, &err);
		if(err){
			ERRMSG("Unable to calculate h");
			return 3;
		}
		if(fabs(h1 - hf) < tol){
			*Tsat_out = T1;
			*psat_out = p;
			*rhof_out = rhof;
			*rhog_out = rhog;
			return 0;
		}
		if(h1 == h2){
			MSG("With %s, got h1 = h2 = %.12e, but hf = %.12e!",P->name,h1,hf);
			return 2;
		}

		double delta_T = -(h1 - hf) * (T1 - T2) / (h1 - h2);
		T2 = T1;
		h2 = h1;
		while(T1 + delta_T > P->data->T_c)delta_T *= 0.5;
		T1 += delta_T;
		if(T1 < P->data->T_t)T1 = P->data->T_t;
		if(i==20 || i==30)tol*=100;
	}
	fprintf(stderr,"Failed to solve Tsat for hf = %f (got to T = %f)\n",hf,T1);
	*Tsat_out = T1;
	*psat_out = p;
	*rhof_out = rhof;
	*rhog_out = rhog;
	return 1;
}

int fprops_sat_T_cubic(double T, double *psat_out, double *rhof_out, double *rhog_out, const PureFluid *d){
	// FIXME sat.c functions should take err pointer too...
	FpropsError err = FPROPS_NO_ERROR;
    double rhof = 1.1 * fprops_rhof_T_rackett(T,d);
	double rhog= fprops_rhog_T_chouaieb(T,d);
    double A,B,C,D;
	//double tau = TCRIT(d) / T;

	if(T < TTRIP(d) - 1e-8){
		MSG("Input temperature %f K is below triple-point temperature %f K",T,TTRIP(d));
		return FPROPS_RANGE_ERROR;
	}

	if(T > TCRIT(d)){
		MSG("Input temperature is above critical point temperature");
		return FPROPS_RANGE_ERROR;
	}

	// we're at the critical point
	if(fabs(T - TCRIT(d)) < 1e-9){
		*psat_out = PCRIT(d);
		*rhof_out = RHOCRIT(d);
		*rhog_out = RHOCRIT(d);
		return 0;
	}
	// take the given rho and start
	switch(d->type){
        case FPROPS_PENGROB:{
            if(rhof > 1./(d->data->corr.pengrob->b)){
                MSG("First guess density is above range of PR EOS");
                rhof = 1./(d->data->corr.pengrob->b)-(.1*1./(d->data->corr.pengrob->b));
            }
            double p = d->p_fn(T, rhog,d->data,&err);
            //at this point density could still be middle root and therefore bad
            MSG("Going with density=%e",rhog);
            int k = 0;
            while(++k <50){
                //might be able to remove the root finding and just use rhof and rhog, but this way better ensures that the rho value
                //from the correlations does not give a value that could be the middle root.As long as rhof is reasonable, we should get
                //convergence
                #define vals d->data->corr.pengrob
                double b = vals->b;
                double sqrtalpha = 1 + vals->kappa * (1 - sqrt(T / d->data->T_c));
                double a = vals->aTc * SQ(sqrtalpha);
                AZ = (a*p)/((d->data->R)*(d->data->R)*T*T);
                BZ = (b*p)/((d->data->R)*T);
                A = 1;
                B = BZ-1;
                C = AZ-2*BZ-3*pow(BZ,2);
                D = pow(BZ,3)+pow(BZ,2)-AZ*BZ;
                CubicRoots(A,B,C,D);
                //MSG("Number of roots=%i",PRroots);
                #undef vals
                int i = 0;
                int j = 0;
                for (; i<3; ++i){
                    returnedroots[i] = 1./((returnedroots[i]*(d->data->R)*T)/p);
                    if (cimag(returnedroots[i]) == 0){
                    ++j;
                    }
                }
                if(j == 3){  //all roots are real
                    double min = fmin(returnedroots[0],fmin(returnedroots[1],returnedroots[2]));
                    MSG("Min=%e",min);
                    double max = fmax(returnedroots[0],fmax(returnedroots[1],returnedroots[2]));
                    MSG("Max=%e",max);
                    if(min < 0 ){
                        MSG("Density in single phase region only");
                        return FPROPS_SAT_CVGC_ERROR;
                    }
                    // This will be a quick check that the sat conditions are satisfied by making sure that liquid and vapor phase fugacities are equal
                    double Zv = p*(1./min)/(T*(d->data->R));
                    //MSG("Zv:, %e", Zv);
                    double Zl = p*(1./max)/(T*(d->data->R));
                    //MSG("Zl:, %e", Zl);
                    double Fv = exp((Zv-1-log(Zv-BZ)-(AZ/(2*sqrt(2)*BZ))*log((Zv+BZ*(1+sqrt(2)))/(Zv+BZ*(1-sqrt(2))))));
                    //MSG("Fv:, %e", Fv);
                    double Fl = exp((Zl-1-log(Zl-BZ)-(AZ/(2*sqrt(2)*BZ))*log((Zl+BZ*(1+sqrt(2)))/(Zl+BZ*(1-sqrt(2))))));
                    //MSG("Fl:, %e", Fl);
                    double fug = Fl/Fv;
                    MSG("fug:, %e", fug);
                    if(fabsf(fug-1)<1e-5){
                        MSG("fugacity around 1, iterations:%i,",k);
                        *rhof_out = max;
                        *rhog_out = min;
                        *psat_out = d->p_fn(T, *rhog_out,d->data,&err);
                        return err; /* success */
                    }
                    else{
                        p=p*fug;
                    }
                }
                else{
                    p = MidpointPressureCubic(T, d);
                    MSG("new p, %f",p);
                }

            }

            break;
        }
        default:{
            MSG("In default case");
            break;
        }


	}

return -1;
}

/*
This function was originally taken from the Ankit branch.  I have modified it so that the equation is not "deflated" by dividing by the pressure.
Because we are using specific volume, and in some cases large pressure, we may need a more numerically sound method that won't return nan's
Should probably be put somewhere else if used.
In this case, coefficients are AZ^3+BZ^2+CZ+D.  SPM
*/
int CubicRoots(double A, double B, double C, double D){ // This function will give value of liquid and vapour volumes at P using PR, and store it in reference variables
    #define PI 3.14159265
    #define J _Complex_I
    double discriminant = 18*A*B*C*D - 4*B*B*B*D + C*C*B*B - 4*A*C*C*C - 27*A*A*D*D;
	double term1 = (2*B*B*B)-(9*A*B*C)+(27*A*A*D);
	double complex term2;
	MSG("term1:, %e", term1);
	if(discriminant>0){
        term2 = J*sqrt(27*A*A*discriminant);
        MSG(" term2 real: %e, term2 imag, %e", creal(term2),cimag(term2));
	}
	else{
        term2 = sqrt(-27*A*A*discriminant);
        MSG(" term2 real: %e, term2 imag, %e", creal(term2),cimag(term2));
	}
	double complex cuberootpos = cpow(.5*(term1+term2),1./3);
	MSG(" cuberootpos real: %e, cuberootpos imag, %e", creal(cuberootpos),cimag(cuberootpos));
	double complex cuberootneg = cpow(.5*(term1-term2),1./3);
	MSG(" cuberootneg real: %e, cuberootneg imag, %e", creal(cuberootneg),cimag(cuberootneg));
	double complex complextermpos = (1+J*sqrt(3))/(6*A);
	MSG(" complexterm real: %e, complexterm imag, %e", creal(complextermpos),cimag(complextermpos));
    double complex complextermneg = (1-J*sqrt(3))/(6*A);
    MSG(" complextermneg real: %e, complextermneg imag, %e", creal(complextermneg),cimag(complextermneg));

	returnedroots[0] = -B/(3*A) -((1/(3*A))*cuberootpos)-((1/(3*A))*cuberootneg);
	MSG(" root 1 real: %e, root 1 imag, %e", creal(returnedroots[0]),cimag(returnedroots[0]));
    returnedroots[1] = -B/(3*A) + complextermpos*cuberootpos + complextermneg*cuberootneg;
    MSG(" root 2 real: %e, 2 imag, %e", creal(returnedroots[1]),cimag(returnedroots[1]));
    returnedroots[2] = -B/(3*A) + complextermneg*cuberootpos + complextermpos*cuberootneg;
    MSG(" root 3 real: %e, 3 imag, %e", creal(returnedroots[2]),cimag(returnedroots[2]));



        if (discriminant>0)
        return 3;
        else
        return 1;
}

typedef struct{
	const PureFluid *P;
	double T;
} CubicDerivData;
static ZeroInSubjectFunction Roots_dpdrho_T;
static double Roots_dpdrho_T(double V, void *user_data){
#define D ((CubicDerivData *)user_data)
    FpropsError *cubicdataerror = FPROPS_NO_ERROR;
    double a = D->P->dpdrho_T_fn(D->T,V,D->P->data,cubicdataerror);
	return a;
#undef D
}

double MidpointPressureCubic(double T, const PureFluid *P){
	CubicDerivData D = {P, T};
	int i = 1;
	FpropsError *cubicdataerror = FPROPS_NO_ERROR;
	double max = 1./(P->data->corr.pengrob->b);
	MSG("in zero in, max %f",max);
	double V1,V2,p,resid1,resid2;
	zeroin_solve(&Roots_dpdrho_T, &D, i, max-1, 1e-9, &V1, &resid1);
	if(fabsf(V1 -1)<1e-9){
		while(fabsf(V1 -1)<1e-9){
            zeroin_solve(&Roots_dpdrho_T, &D, i+1, max-1, 1e-9, &V1, &resid1);
            ++i;
        }
	}
	MSG("in zero in, V1 %f",V1);
	MSG("in zero in, resid1 %f",resid1);
	//near critical point may not solve for 0, so we get the best we can
	#define root P->dpdrho_T_fn
    // try one interval
    zeroin_solve(&Roots_dpdrho_T, &D, i , V1-1e-9, 1e-9, &V2, &resid2);
    if(fabsf(V1-V2)<1e-9){
         zeroin_solve(&Roots_dpdrho_T, &D, V1+1e-9 , max-1, 1e-9, &V2, &resid2);
    }
    MSG("in zero in, V2 %f",V2);
    p = (P->p_fn(T, V1,P->data,cubicdataerror)+P->p_fn(T, V2,P->data,cubicdataerror))/2;
    #undef root
    return p;
}










