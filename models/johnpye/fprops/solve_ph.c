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
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "solve_ph.h"
#include "fprops.h"
#include "sat.h"
#include "derivs.h"
#include "rundata.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define SOLVE_PH_DEBUG
#define SOLVE_PH_ERRORS

//#define FPE_DEBUG
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
int feenableexcept(int excepts);
int fedisableexcept(int excepts);
int fegetexcept(void);
#endif

#ifdef SOLVE_PH_DEBUG
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

#ifdef SOLVE_PH_ERRORS
# include "color.h"
# define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define ERRMSG(ARGS...) ((void)0)
#endif

int fprops_region_ph(double p, double h, const PureFluid *fluid, FpropsError *err){
    double Tsat, rhof, rhog;
	double p_c = fluid->data->p_c;

	if(p >= p_c)return FPROPS_NON;

	fprops_sat_p(p, &Tsat, &rhof, &rhog, fluid, err);
	if(*err){
		*err = FPROPS_SAT_CVGC_ERROR;
		return FPROPS_ERROR;
	}

	double hf = fluid->h_fn(Tsat, rhof, fluid->data, err);
	if(h <= hf)return FPROPS_NON;

	double hg = fluid->h_fn(Tsat,rhog, fluid->data, err);
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

void fprops_solve_ph(double p, double h, double *T, double *rho, int use_guess
		, const PureFluid *fluid, FpropsError *err
){
	double Tsat, rhof, rhog, hf, hg;
	double T1, rho1;
	int subcrit_pressure = 0;
	int liquid_iteration = 0;
	double rhof_t;
	double p_c = fluid->data->p_c;

	MSG("Solving for p=%f bar, h=%f kJ/kgK (EOS type %d)",p/1e5,h/1e3,fluid->type);

#ifdef FPE_DEBUG
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
	SignalHandler *old = signal(SIGFPE,&fprops_fpe);
	(void)old;/* not used for anything at this stage */
	int jmpret = setjmp(mark);
	if(jmpret==0){
#endif
		MSG("p_c = %f bar",p_c/1e5);
		if(p < p_c){
			MSG("Calculate saturation Tsat(p < p_c)");
			fprops_sat_p(p, &Tsat, &rhof, &rhog, fluid, err);
			if(*err){
				ERRMSG("Unable to solve saturation state");
				*err = FPROPS_SAT_CVGC_ERROR;
				return;
			}
			hf = fluid->h_fn(Tsat, rhof, fluid->data, err);
			hg = fluid->h_fn(Tsat, rhog, fluid->data, err);
			MSG("at p = %f bar, T_sat = %f, hf = %f kJ/kg, hg = %f",p/1e5,Tsat,hf/1e3,hg/1e3);

			if(hf < h && h < hg){
				MSG("SATURATION REGION");
				/* saturation region... easy */
				double x = (h - hf)/(hg - hf);
				*rho = 1./(x/rhog + (1.-x)/rhof);
				*T = Tsat;
				return;
			}

			subcrit_pressure = 1;
			if(h < hf){
				liquid_iteration = 1;
				if(!use_guess){
					*T = Tsat;
					*rho = rhof;
					MSG("LIQUID GUESS: T = %f, rho = %f",*T, *rho);
				}
			}else if(!use_guess){
				*T = 1.1 * Tsat;
				*rho = rhog * 0.5;
				MSG("GAS GUESS: T = %f, rho = %f",*T, *rho);
			}
		}else{ /* p >= p_c */
			/* FIXME: still some problems here at very high pressures */
			if(!use_guess){
				/* FIXME we should cache/precalculate hc, store in rundata. */
				double hc = fluid->h_fn(fluid->data->T_c, fluid->data->rho_c, fluid->data, err);
				assert(!isnan(hc));
				MSG("hc = %f kJ/kgK",hc/1e3);
				if(h < 0.8*hc){ /* FIXME should make use of h_ref here in some way? Otherwise arbitrary... */
#if 0
					double Tsat1, psat1, rhof1, rhog1;
					int res = fprops_sat_p(p, &Tsat1, &rhof1, &rhog1, fluid);
					MSG("Unable to solve saturation at p = %f",p);
					*T = Tsat1;
					*rho = rhof1;
#else					
					MSG("h < 0.9 hc... using saturation Tsat(hf) for starting guess");
					double Tsat1, psat1, rhof1, rhog1;
					fprops_sat_hf(h, &Tsat1, &psat1, &rhof1, &rhog1, fluid, err);
					if(*err){
						MSG("Unable to solve Tsat(hf)");
						/* accuracy of the estimate of Tsat(hf) doesn't matter
						very much so we can ignore this error. */

						/* try initialising to T_t, rho_f(T_t) */
						Tsat1 = fluid->data->T_t;
						MSG("T_t = %f",Tsat1);
						fprops_sat_T(Tsat1, &psat1, &rhof1, &rhog1, fluid, err);
						if(*err)MSG("Unable to solve rhof(Tt)");
					}
					*T = Tsat1;
					*rho = rhof1;
#endif
				}else{
					*T = fluid->data->T_c * 1.01;
					*rho = fluid->data->rho_c * 1.05;
				}
				MSG("SUPERCRITICAL GUESS: T = %f, rho = %f", *T, *rho);
			}
		}

		MSG("STARTING NON-SAT ITERATION");
		//*rho = 976.82687191126922;
		//*T = 344.80371310850518;

		T1 = *T;
		rho1 = *rho;
		assert(!isnan(T1));
		assert(!isnan(rho1));

		if(liquid_iteration){
			double pt,rhogt;
			fprops_triple_point(&pt, &rhof_t, &rhogt, fluid, err);
			if(*err){
				ERRMSG("Unable to solve triple point liquid density.");
				*err = FPROPS_SAT_CVGC_ERROR;
				return;
			}
		}

		/* try our own home-baked newton iteration */
		int i = 0;
		*err = FPROPS_NO_ERROR;
		double delta_T = 0;
		double delta_rho = 0;
		MSG("STARTING ITERATION");
		while(i++ < 200){
			double p1 = fluid->p_fn(T1,rho1, fluid->data, err);
			if(*err){
				MSG("Got an error ('%s') in p_fn calculation",fprops_error(*err));
			} 
			double h1 = fluid->h_fn(T1,rho1, fluid->data, err);
			assert(!isnan(h1));

			MSG("  %d: T = %f, rho = %f\tp = %f bar, h = %f kJ/kg", i, T1, rho1, p1/1e5, h1/1e3);
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
				return;
			}
			/* calculate step, we're solving log(p1) in this code... */
			double f = log(p1) - log(p);
			double g = h1 - h;
			assert(!isnan(f));
			assert(!isnan(g));
			assert(rho1 != 0);
			assert(p1 != 0);
			assert(!isnan(fprops_non_dZdT_v('p', T1,rho1, fluid,err)));
			double f_T = 1./p1 * fprops_non_dZdT_v('p', T1,rho1, fluid,err);
			double f_rho = -1./p1/SQ(rho1) * fprops_non_dZdv_T('p', T1, rho1, fluid,err);
			double g_T = fprops_non_dZdT_v('h', T1,rho1, fluid,err);
			double g_rho = -1./SQ(rho1) * fprops_non_dZdv_T('h', T1, rho1, fluid,err);
			assert(!isnan(f_T));

			if(isnan(f_rho)){
				ERRMSG("     rho1 = %f, T1 = %f",rho1, T1);
			}
			assert(!isnan(f_rho));

			assert(!isnan(g_T));
			assert(!isnan(g_rho));

			double det = g_rho * f_T - f_rho * g_T;
			assert(det!=0);
			assert(!isnan(det));
			MSG("      df/dT = %e\t\tdf/drho = %e",f_T, f_rho);
			MSG("      dg/dT = %e\t\tdg/drho = %e",g_T, g_rho);

			delta_T = -1./det * (g_rho * f - f_rho * g);
			delta_rho = -1./det * (f_T * g - g_T * f);
			assert(!isnan(delta_T));
			assert(!isnan(delta_rho));
			MSG("          dT   = %f", delta_T);
			MSG("          drho = %f", delta_rho);

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
					if(T1 + delta_T < fluid->data->T_t) delta_T = 0.5 * (T1 + fluid->data->T_t);
				}
			}else{
#if 0
				/* supercritical... stay above critical temperature of density > rho_crit */
				if(rho1 + delta_rho > fluid->data->rho_c && T1 + delta_T < fluid->data->T_c){
					delta_T = 0.5 *(T1 + fluid->data->T_c);
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
			if(T1 < fluid->data->T_t)T1 = fluid->data->T_t;
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
	ERRMSG("Iteration failed for '%s' with p = %.12e, h = %.12e",fluid->name, p,h);
	*err = FPROPS_NUMERIC_ERROR;
	return;

#if 0
	int res = fprops_nonsolver('p','h',p,h,T,rho,fluid);
	ERRMSG("Iteration failed in nonsolver");
	return res;
#endif
}

