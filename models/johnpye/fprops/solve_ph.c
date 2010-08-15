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
#include "sat.h"
#include "derivs.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define FPE_DEBUG
#define ASSERT_DEBUG

#ifdef ASSERT_DEBUG
# include <assert.h>
#else
# define assert(ARGS...)
#endif

#include <setjmp.h>
#include <signal.h>

#ifdef FPE_DEBUG
#define _GNU_SOURCE
#include <fenv.h>
#endif

#define SQ(X) ((X)*(X))

//#define SOLVE_PH_DEBUG
#ifdef SOLVE_PH_DEBUG
# define MSG(STR,...) fprintf(stderr,"%s:%d: " STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define MSG(ARGS...)
#endif

#define ERRMSG(STR,...) fprintf(stderr,"%s:%d: ERROR: " STR "\n", __func__, __LINE__ ,##__VA_ARGS__)

int fprops_region_ph(double p, double h, const HelmholtzData *D){

	double Tsat, rhof, rhog;
	double p_c = fprops_pc(D);

	if(p >= p_c)return FPROPS_NON;

	int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);

	double hf = helmholtz_h(Tsat, rhof, D);
	if(h <= hf)return FPROPS_NON;

	double hg = helmholtz_h(Tsat, rhog, D);
	if(h >= hg)return FPROPS_NON;

	return FPROPS_SAT;
}

typedef void SignalHandler(int);

#ifdef FPE_DEBUG
jmp_buf mark;
void fprops_fpe(int sig){
	MSG("Catching signal %d!",sig);
	feclearexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW);
	longjmp(mark, -1);
}
#endif

int fprops_solve_ph(double p, double h, double *T, double *rho, int use_guess, const HelmholtzData *D){
	double Tsat, rhof, rhog, hf, hg;
	double T1, rho1;
	int subcrit_pressure = 0;
	int liquid_iteration = 0;
	double rhof_t;
	double p_c = fprops_pc(D);

#ifdef FPE_DEBUG
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
	SignalHandler *old = signal(SIGFPE,&fprops_fpe);
	int jmpret = setjmp(mark);
	if(jmpret==0){
#endif
		
		if(p < p_c){
			MSG("Calculate saturation Tsat(p < p_c)");
			int res = fprops_sat_p(p, &Tsat, &rhof, &rhog, D);
			if(res){
				ERRMSG("Unable to solve saturation state");
				return res;
			}
			hf = helmholtz_h(Tsat, rhof, D);
			hg = helmholtz_h(Tsat, rhog, D);
			MSG("hf = %f kJ/kg, hg = %f",hf/1e3,hg/1e3);

			if(h > hf && h < hg){
				MSG("SATURATION REGION");
				/* saturation region... easy */
				double x = (h - hf)/(hg - hf);
				*rho = 1./(x/rhog + (1.-x)/rhof);
				*T = Tsat;
				return 0;
			}

			subcrit_pressure = 1;
			if(h < hf){
				liquid_iteration = 1;
				if(!use_guess){
#if 1
					double Tsat1, psat1, rhof1, rhog1;
					MSG("SOLVING TSAT(HF)");
					res = fprops_sat_hf(h, &Tsat1, &psat1, &rhof1, &rhog1, D);
					if(res){
						ERRMSG("Unable to solve Tsat(hf), returning T = %f, rho = %f.",Tsat,rhof);
						*T = Tsat;
						*rho = rhof;
						return res;
					}
					*T = Tsat1;
					*rho = rhof1;
#else
					*T = Tsat;
					*rho = rhof;
#endif
					MSG("LIQUID GUESS: T = %f, rho = %f",*T, *rho);
				}
			}else if(!use_guess){
				*T = 1.1 * Tsat;
				*rho = rhog * 0.5;
				MSG("GAS GUESS: T = %f, rho = %f",*T, *rho);
			}
		}else{
			/* FIXME still some problems here at very high pressures */
			if(!use_guess){
				double hc = helmholtz_h_raw(D->T_c, D->rho_c, D);
				assert(!__isnan(hc));
				MSG("hc = %f",hc);
				if(h < 0.9 * hc){
					MSG("h < hc... using saturation Tsat(hf) for starting guess");
					double Tsat1, psat1, rhof1, rhog1;
					int res = fprops_sat_hf(h, &Tsat1, &psat1, &rhof1, &rhog1, D);
					if(res){
						MSG("Unable to solve Tsat(hf)");
						/* accuracy of the estimate of Tsat(hf) doesn't matter 
						very much so we can ignore this error. */
					}
					*T = Tsat1;
					*rho = rhof1;
				}else{
					*T = D->T_c * 1.01;
					*rho = D->rho_c * 1.05;
				}
				MSG("SUPERCRITICAL GUESS: T = %f, rho = %f", *T, *rho);
			}
		}

		MSG("STARTING NON-SAT ITERATION");
		//*rho = 976.82687191126922;
		//*T = 344.80371310850518;

		T1 = *T;
		rho1 = *rho;
		assert(!__isnan(T1));
		assert(!__isnan(rho1));

		if(liquid_iteration){
			double pt,rhogt;
			int res = fprops_sat_T(D->T_t,&pt,&rhof_t, &rhogt, D);
			if(res){
				ERRMSG("Unable to solve triple point liquid density.");
				return 1;
			}
		}

		/* try our own home-baked newton iteration */
		int i = 0;
		double delta_T = 0;
		double delta_rho = 0;
		MSG("STARTING ITERATION");
		while(i++ < 200){
			double p1 = helmholtz_p_raw(T1,rho1,D);
			assert(!__isnan(p1));
			double h1 = helmholtz_h_raw(T1,rho1,D);
			assert(!__isnan(h1));

			MSG("  T = %f, rho = %f\tp = %f bar, h = %f kJ/kg", T1, rho1, p1/1e5, h1/1e3);
			MSG("      p error = %f bar",(p1 - p)/1e5);
			MSG("      h error = %f kJ/kg",(h1 - h)/1e3);

			if(p1 < 0){
				T1 -= (delta_T *= 0.5); 
				rho1 -= (delta_rho *= 0.5); 
				continue;
			}

			if(fabs(p1 - p) < 1e-4 && fabs(h1 - h) < 1e-8){
				MSG("Converged to T = %f, rho = %f, in homebaked Newton solver", T1, rho1);
				*T = T1;
				*rho = rho1;
				return 0;
			}
			/* calculate step, we're solving log(p1) in this code... */
			double f = log(p1) - log(p);
			double g = h1 - h;
			assert(!__isnan(f));
			assert(!__isnan(g));
			assert(rho1 != 0);
			assert(p1 != 0);
			assert(!__isnan(fprops_non_dZdT_v('p', T1,rho1, D)));
			double f_T = 1./p1 * fprops_non_dZdT_v('p', T1,rho1, D);
			double f_rho = -1./p1/SQ(rho1) * fprops_non_dZdv_T('p', T1, rho1, D);
			double g_T = fprops_non_dZdT_v('h', T1,rho1, D);
			double g_rho = -1./SQ(rho1) * fprops_non_dZdv_T('h', T1, rho1, D);
			assert(!__isnan(f_T));

			if(__isnan(f_rho)){
				ERRMSG("     rho1 = %f, T1 = %f",rho1, T1);
			}
			assert(!__isnan(f_rho));

			assert(!__isnan(g_T));
			assert(!__isnan(g_rho));

			double det = g_rho * f_T - f_rho * g_T;
			assert(det!=0);
			assert(!__isnan(det));
			MSG("      ∂f/∂T = %e\t\t∂f/∂rho = %e",f_T, f_rho);
			MSG("      ∂g/∂T = %e\t\t∂g/∂rho = %e",g_T, g_rho);
	
			delta_T = -1./det * (g_rho * f - f_rho * g);
			delta_rho = -1./det * (f_T * g - g_T * f);
			assert(!__isnan(delta_T));
			assert(!__isnan(delta_rho));
			MSG("          ΔT   = %f", delta_T);
			MSG("          Δrho = %f", delta_rho);

			if(subcrit_pressure){
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
				}
			}else{
#if 0
				/* supercritical... stay above critical temperature of density > rho_crit */
				if(rho1 + delta_rho > D->rho_c && T1 + delta_T < D->T_c){
					delta_T = 0.5 *(T1 + D->T_c);
				}
#endif
			}
			/* don't go too dense */
			if(liquid_iteration){
				if(rho1 + delta_rho > rhof_t) delta_rho = rhof_t;
			}
		
			/* don't go too hot */
			if(T1 + delta_T > 5000) delta_T = 5000 - T1;

			/* avoid huge step */
			while(fabs(delta_T / T1) > 0.6){
				delta_T *= 0.5;
			}
			while(fabs(delta_rho / rho1) > 0.2){
				delta_rho *= 0.5;
			}

			T1 = T1 + delta_T;
			if(T1 < D->T_t)T1 = D->T_t;
			rho1 = rho1 + delta_rho;
		}
#ifdef FPE_DEBUG
	}else{
		/* an FPE occurred */
		MSG("An FPE occurred");
		abort();
	}
#endif

	*T = T1;
	*rho = rho1;
	ERRMSG("Iteration failed");
	return 999;

	int res = fprops_nonsolver('p','h',p,h,T,rho,D);
	ERRMSG("Iteration failed in nonsolver");
	return res;
}

