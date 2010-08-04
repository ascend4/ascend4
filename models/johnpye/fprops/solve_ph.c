/*
ASCEND modelling environment
Copyright (C) 2004-2010 John Pye

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "solve_ph.h"
#include "sat2.h"
#include "derivs.h"

#include <stdio.h>
#include <math.h>
#include <assert.h>

#define SQ(X) ((X)*(X))

int fprops_region_ph(double p, double h, const HelmholtzData *D){

	double Tsat, rhof, rhog;

	if(p >= D->p_c)return FPROPS_NON;

	int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);

	double hf = helmholtz_h(Tsat, rhof, D);
	if(h <= hf)return FPROPS_NON;

	double hg = helmholtz_h(Tsat, rhog, D);
	if(h >= hg)return FPROPS_NON;

	return FPROPS_SAT;
}

int fprops_solve_ph(double p, double h, double *T, double *rho, int use_guess, const HelmholtzData *D){
	double Tsat, rhof, rhog, hf, hg;
	double T1, rho1;
	int subcrit = 0;
	if(p < D->p_c){
		int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);
		if(res){
			fprintf(stderr,"Unable to solve saturation state\n");
			return res;
		}
		hf = helmholtz_h(Tsat, rhof, D);
		hg = helmholtz_h(Tsat, rhog, D);
		fprintf(stderr,"hf = %f kJ/kg, hg = %f\n",hf/1e3,hg/1e3);

		if(h > hf && h < hg){
			fprintf(stderr,"SATURATION REGION\n");
			/* saturation region... easy */
			double x = (h - hf)/(hg - hf);
			*rho = 1./(x/rhog + (1.-x)/rhof);
			*T = Tsat;
			return 0;
		}

		subcrit = 1;
		if(!use_guess){
			*T = Tsat;
			if(h <= hf)*rho = 1.1 * rhof;
			else *rho = rhog * 0.5;
		}
	}else{
		if(!use_guess){
			*T = D->T_c;
			*rho = D->rho_c;
		}
	}

	fprintf(stderr,"STARTING NON-SAT ITERATION\n");
	//*rho = 0.21584907523805483;
	//*T = 1004.0675923338069;

	T1 = *T;
	rho1 = *rho;
	/* try our own home-baked newton iteration */
	int i = 0;
	while(i++ < 40){
		double p1 = helmholtz_p(T1,rho1,D);
		assert(!__isnan(p1));
		double h1 = helmholtz_h(T1,rho1,D);
		assert(!__isnan(h1));

		fprintf(stderr,"  T = %f, rho = %f\tp = %f bar, h = %f kJ/kg\n", T1, rho1, p1/1e5, h1/1e3);
		fprintf(stderr,"      p error = %f bar\n",(p1 - p)/1e5);
		fprintf(stderr,"      h error = %f kJ/kg\n",(h1 - h)/1e3);
		if(fabs(p1 - p) < 1e-7 && fabs(h1 - h) < 1e-7){
			fprintf(stderr,"Converged to T = %f, rho = %f, in homebaked Newton solver", T1, rho1);
			*T = T1;
			*rho = rho1;
			return 0;
		}
		/* calculate step */
		double f = log(p1) - log(p);
		double g = h1 - h;
		double f_T = 1./p1 * fprops_non_dZdT_v('p', T1,rho1, D);
		double f_rho = -1./p1/SQ(rho1) * fprops_non_dZdv_T('p', T1, rho1, D);
		double g_T = fprops_non_dZdT_v('h', T1,rho1, D);
		double g_rho = -1./SQ(rho1) * fprops_non_dZdv_T('h', T1, rho1, D);
		double det = g_rho * f_T - f_rho * g_T;
		fprintf(stderr,"      ∂f/∂T = %e\t\t∂f/∂rho = %e\n",f_T, f_rho);
		fprintf(stderr,"      ∂g/∂T = %e\t\t∂g/∂rho = %e\n",g_T, g_rho);
	
		double delta_T = -1./det * (g_rho * f - f_rho * g);
		double delta_rho = -1./det * (f_T * g - g_T * f);
		fprintf(stderr,"          ΔT   = %f\n", delta_T);
		fprintf(stderr,"          Δrho = %f\n", delta_rho);

		if(subcrit){
			if(h > hg){
				/* vapour */
				int i = 0;
				while(rho1 + delta_rho > rhog && i++ < 20){
					delta_rho *= 0.5;
				}
			}else{
				/* liquid */
				while(rho1 + delta_rho < rhof && i++ < 20){
					delta_rho *= 0.5;
				}
				if(T1 + delta_T < D->T_t) delta_T = 0.5 * (T1 + D->T_t);
				if(T1 < D->T_t) delta_T = +1;
			}
		}
		/* don't go too dense */
		if(rho1 + delta_rho > 2000) delta_rho = 2000;

		/* avoid huge step */
		while(fabs(delta_T / T1) > 0.6){
			delta_T *= 0.5;
		}
		while(fabs(delta_rho / rho1) > 0.2){
			delta_rho *= 0.5;
		}

		T1 = T1 + delta_T;
		rho1 = rho1 + delta_rho;
	}

	return 999;

	int res = fprops_nonsolver('p','h',p,h,T,rho,D);
	fprintf(stderr,"%s: Iteration failed in nonsolver\n",__func__);
	return res;
}

