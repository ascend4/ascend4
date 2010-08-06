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

#include "sat.h"
#include "helmholtz_impl.h"

# include <assert.h>
#include <math.h>
#include <stdio.h>

#define SQ(X) ((X)*(X))

#if 0
#define _GNU_SOURCE
#include <fenv.h>
#endif

/**
	Estimate of saturation pressure using H W Xiang ''The new simple extended
	corresponding-states principle: vapor pressure and second virial
	coefficient'', Chemical Engineering Science,
	57 (2002) pp 1439-1449.
*/
double fprops_psat_T_xiang(double T, const HelmholtzData *d){

	double Zc = d->p_c / (8314. * d->rho_c * d->T_c);

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

	double a0 = aa[0] + aa[1]*d->omega + aa[2]*theta;
	double a1 = aa[3] + aa[4]*d->omega + aa[5]*theta;
	double a2 = aa[6] + aa[7]*d->omega + aa[8]*theta;

	double Tr = T / d->T_c;
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

	return p_r * d->p_c;
}

/**
	Estimate saturation pressure using acentric factor. This algorithm
	is used for first estimates for later refinement in the program REFPROP.
*/
double fprops_psat_T_acentric(double T, const HelmholtzData *d){
	/* first guess using acentric factor */
	double p = d->p_c * pow(10, -7./3 * (1.+d->omega) * (d->T_c / T - 1.));
	return p;
}


/**
	Saturated liquid density correlation of Rackett, Spencer & Danner (1972)
	see http://dx.doi.org/10.1002/aic.690250412
*/
double fprops_rhof_T_rackett(double T, const HelmholtzData *D){

	double Zc = D->rho_c * D->R * D->T_c / D->p_c;
	double Tau = 1. - T/D->T_c;
	double vf = (D->R * D->T_c / D->p_c) * pow(Zc, -1 - pow(Tau, 2./7));

	return 1./vf;
}

/**
	Inverse of fprops_rhof_T_rackett. FIXME this need checking.
*/
double fprops_T_rhof_rackett(double rhof, const HelmholtzData *D){
	double Zc = D->rho_c * D->R * D->T_c / D->p_c;
	double f1 = D->p_c / D->R / D->T_c / rhof;
	double f2 = -log(f1)/log(Zc);
	return pow(f2 -1, 3./2);
}

/**
	Saturated vapour density correlation of Chouaieb, Ghazouani, Bellagi
	see http://dx.doi.org/10.1016/j.tca.2004.05.017
*/
double fprops_rhog_T_chouaieb(double T, const HelmholtzData *D){
	double Zc = D->rho_c * D->R * D->T_c / D->p_c;
	double Tau = 1. - T/D->T_c;
#if 0
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
	double NNN = PPP + 1./(N1*D->omega + N2);	
#else
/* exact values from Chouaieb for CO2 */
# define MMM 2.4686277
# define NNN 1.1345838
# define PPP -0.6240188
#endif

	double alpha = exp(pow(Tau,1./3) + sqrt(Tau) + Tau + pow(Tau, MMM));
	return D->rho_c * exp(PPP * (pow(alpha,NNN) - exp(1-alpha)));
}

int fprops_sat_T(double T, double *psat_out, double *rhof_out, double * rhog_out, const HelmholtzData *d){
	double tau = d->T_c / T;
	double delf = 1.1 * fprops_rhof_T_rackett(T,d) / d->rho_c;
	double delg = 0.9 * fprops_rhog_T_chouaieb(T,d) / d->rho_c;
	//feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);

	int i = 0;
	while(i++ < 50){
		//fprintf(stderr,"%s: iter %d: rhof = %f, rhog = %f\n",__func__,i,delf*d->rho_c, delg*d->rho_c);
		double phirf = helm_resid(tau,delf,d);
		double phirf_d = helm_resid_del(tau,delf,d);
		double phirf_dd = helm_resid_deldel(tau,delf,d);
		double phirg = helm_resid(tau,delg,d);
		double phirg_d = helm_resid_del(tau,delg,d);
		double phirg_dd = helm_resid_deldel(tau,delg,d);

#define J(FG) (del##FG * (1. + del##FG * phir##FG##_d))
#define K(FG) (del##FG * phir##FG##_d + phir##FG + log(del##FG))
#define J_del(FG) (1 + 2 * del##FG * phir##FG##_d + SQ(del##FG) * phir##FG##_dd)
#define K_del(FG) (2 * phir##FG##_d + del##FG * phir##FG##_dd + 1./del##FG)
		double Jf = J(f);
		double Jg = J(g);
		double Kf = K(f);
		double Kg = K(g);
		double Jf_del = J_del(f);
		double Jg_del = J_del(g);
		double Kf_del = K_del(f);
		double Kg_del = K_del(g);

		double DELTA = Jg_del * Kf_del - Jf_del * Kg_del;

#define gamma 1
		delf += gamma/DELTA * ((Kg - Kf) * Jg_del - (Jg - Jf) * Kg_del);
		delg += gamma/DELTA * ((Kg - Kf) * Jf_del - (Jg - Jf) * Kf_del);

		if(fabs(Kg - Kf) + fabs(Jg - Jf) < 1e-8){
			//fprintf(stderr,"%s: CONVERGED\n",__func__);
			*rhof_out = delf * d->rho_c;
			*rhog_out = delg * d->rho_c;
			*psat_out = helmholtz_p_raw(T, *rhog_out, d);
			return 0;
		}
	}
	*rhof_out = delf * d->rho_c;
	*rhog_out = delg * d->rho_c;
	*psat_out = helmholtz_p_raw(T, *rhog_out, d);
	fprintf(stderr,"%s: NOT CONVERGED for '%s' with T = %e (rhof=%f, rhog=%f)\n",__func__,d->name,T,*rhof_out,*rhog_out);
	return 1;

}

/**
	Solve saturation properties in terms of pressure.
	This function makes calls to fprops_sat_T, and solves for temperature using
	a Newton solver algorith. Derivatives dp/dT are calculated using the
	Clapeyron equation.
	@return 0 on success.
*/
int fprops_sat_p(double p, double *Tsat_out, double *rhof_out, double * rhog_out, const HelmholtzData *d){
	/*
	Estimate of saturation temperature using definition	of acentric factor and
	the assumed p(T) relationship:
		log10(p)=A + B/T
	See Reid, Prausnitz and Poling, 4th Ed., section 2.3. 
	*/
	double T1 = d->T_c / (1. - 3./7. / (1.+d->omega) * log10(p / d->p_c));
	double p1, rhof, rhog;
	int i = 0;
	while(i++ < 20){
		int res = fprops_sat_T(T1, &p1, &rhof, &rhog, d);
		if(res)return 1;
		//fprintf(stderr,"%s: T1 = %f ——> p = %f bar\trhof = %f\trhog = %f\n",__func__, T1, p1/1e5, rhof, rhog);
		if(fabs(p1 - p) < 1e-5){
			*Tsat_out = T1;
			*rhof_out = rhof;
			*rhog_out = rhog;
			return 0;
		}
		double hf = helmholtz_h_raw(T1, rhof, d);
		double hg = helmholtz_h_raw(T1, rhog, d);
		double dpdT_sat = (hg - hf) / T1 / (1./rhog - 1./rhof);
		//fprintf(stderr,"\t\tdpdT_sat = %f bar/K\n",dpdT_sat/1e5);
		double delta_T = -(p1 - p)/dpdT_sat;
		if(T1 + delta_T < d->T_t)T1 = 0.5 * (d->T_t + T1);
		else T1 += delta_T;
	}
	*Tsat_out = T1;
	*rhof_out = rhof;
	*rhog_out = rhog;
	return 1;
}



/**
	Calculate Tsat based on a value of hf. This value is useful in setting
	first guess Temperatures when solving for the coordinates (p,h).
	Secant method.
*/
int fprops_sat_hf(double hf, double *Tsat_out, double *psat_out, double *rhof_out, double *rhog_out, const HelmholtzData *d){
	double T1 = 0.8 * d->T_t + 0.2 * d->T_c;
	double T2 = d->T_t;
	double h1, h2, p, rhof, rhog;
	int res = fprops_sat_T(T2, &p, &rhof, &rhog, d);
	if(res)return 1;
	h2 = helmholtz_h(T2,rhof,d);
	int i = 0;
	while(i++ < 20){
		res = fprops_sat_T(T1, &p, &rhof, &rhog, d);
		if(res)return 1;
		h1 = helmholtz_h(T1,rhof, d);
		if(fabs(h1 - hf) < 1e-5){
			*Tsat_out = T1;
			*psat_out = p;
			*rhof_out = rhof;
			*rhog_out = rhog;
			return 0;
		}

		double delta_T = -(h1 - hf) * (T1 - T2) / (h1 - h2);
		T2 = T1;
		h2 = h1;
		T1 += delta_T;
	}
	fprintf(stderr,"Failed to solve Tsat for hf = %f\n",hf);
	*Tsat_out = T1;
	*psat_out = p;
	*rhof_out = rhof;
	*rhog_out = rhog;
	return 1;
}


