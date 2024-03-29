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
#include "zeroin.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

//#define SOLVE_PH_DEBUG
#define SOLVE_PH_ERRORS

//#define FPE_DEBUG
//#define ASSERT_DEBUG

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

#define FSU_TRHO(T,RHO) (FluidStateUnion){.Trho={T, RHO}}
int fprops_region_ph(double p, double h, const PureFluid *fluid, FpropsError *err){
    double Tsat, rhof, rhog;
	double p_c = fluid->data->p_c;

	if(p >= p_c)return FPROPS_NON;
	switch(fluid->type){
	case FPROPS_HELMHOLTZ:
	case FPROPS_PENGROB:
		break; // all good, proceed
	case FPROPS_INCOMP:
		// incompressible fluids are always non-saturated.
		return FPROPS_NON;
	default:
		ERRMSG("Unsupported fluid (with p < p_c)");
		*err = FPROPS_NOT_IMPLEMENTED;
		return FPROPS_ERROR;
	}

	fprops_sat_p(p, &Tsat, &rhof, &rhog, fluid, err);
	if(*err){
		*err = FPROPS_SAT_CVGC_ERROR;
		return FPROPS_ERROR;
	}

	double hf = fluid->h_fn(FSU_TRHO(Tsat,rhof), fluid->data, err);
	if(h <= hf)return FPROPS_NON;

	double hg = fluid->h_fn(FSU_TRHO(Tsat,rhog), fluid->data, err);
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

#define STATE_NAN(FLUID) (FluidState2){.vals={.Trho={NAN,NAN}},.fluid=FLUID}
#define STATE_TRHO(FLUID,T,RHO) (FluidState2){.vals={.Trho={T,RHO}},.fluid=FLUID}
#define STATE_TP(FLUID,T,P) (FluidState2){.vals={.Tp={T,P}},.fluid=FLUID}
static FluidState2 fprops_solve_ph_Trho(double p, double h, const PureFluid *fluid, FpropsError *err);
static FluidState2 fprops_solve_ph_incomp(double p, double h, const PureFluid *fluid, FpropsError *err);

FluidState2 fprops_solve_ph(double p, double h, const PureFluid *fluid, FpropsError *err){
	if(!fluid){
		ERRMSG("'fluid' is NULL!");
		*err = FPROPS_INVALID_REQUEST;
		return STATE_NAN(fluid);
	}
	switch(fluid->type){
	case FPROPS_HELMHOLTZ:
	case FPROPS_PENGROB:
		return fprops_solve_ph_Trho(p,h,fluid,err);
	case FPROPS_INCOMP:
		return fprops_solve_ph_incomp(p,h,fluid,err);
	default:
		ERRMSG("Unsupported fluid type");
		*err = FPROPS_NOT_IMPLEMENTED;
		return STATE_NAN(fluid);
	}
}

static FluidState2 fprops_solve_ph_Trho(double p, double h, const PureFluid *fluid, FpropsError *err){
	double T = 0, rho = 0;
	double Tsat, rhof, rhog, hf, hg;
	double T1, rho1;
	int subcrit_pressure = 0;
	int liquid_iteration = 0;
	double rhof_t;
	double p_c = fluid->data->p_c;

	MSG("Solving for p=%f bar, h=%f kJ/kgK (EOS type %d, '%s')",p/1e5,h/1e3,fluid->type,fluid->name);

#ifdef FPE_DEBUG
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
	SignalHandler *old = signal(SIGFPE,&fprops_fpe);
	(void)old;/* not used for anything at this stage */
	int jmpret = setjmp(mark);
	if(jmpret==0){
#endif
		MSG("p_c = %f bar",p_c/1e5);
		if(p < p_c){
			/* TODO what about testing for p >= p_t? */
			MSG("Calculate saturation Tsat(p < p_c) with p = %f",p);
			fprops_sat_p(p, &Tsat, &rhof, &rhog, fluid, err);
			if(*err){
				ERRMSG("Unable to solve saturation state (fluid '%s')",fluid->name);
				*err = FPROPS_SAT_CVGC_ERROR;
				return STATE_NAN(fluid);
			}
			hf = fluid->h_fn((FluidStateUnion){.Trho={Tsat, rhof}}, fluid->data, err);
			hg = fluid->h_fn((FluidStateUnion){.Trho={Tsat, rhog}}, fluid->data, err);
			MSG("at p = %f bar, T_sat = %f, rhof = %f, hf = %f kJ/kg, hg = %f",p/1e5,Tsat,rhof, hf/1e3,hg/1e3);

			if(hf <= h && h <= hg){
				MSG("SATURATION REGION");
				/* saturation region... easy */
				double x = (h - hf)/(hg - hf);
				rho = 1./(x/rhog + (1.-x)/rhof);
				T = Tsat;
				return STATE_TRHO(fluid,T,rho);
			}

			subcrit_pressure = 1;
			if(h < hf){
				liquid_iteration = 1;
				T = Tsat;
				rho = rhof;
				MSG("h < hf; LIQUID GUESS: T = %f, rho = %f",T, rho);
			}else{
				T = 1.1 * Tsat;
				rho = rhog * 0.5;
				MSG("GAS GUESS: T = %f, rho = %f",T, rho);
			}
		}else{ /* p >= p_c */
			/* FIXME: still some problems here at very high pressures */
				/* FIXME we should cache/precalculate hc, store in rundata. */
			double hc = fluid->h_fn((FluidStateUnion){.Trho={fluid->data->T_c, fluid->data->rho_c}}, fluid->data, err);
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
				MSG("h < 0.8 hc... using saturation Tsat(hf) for starting guess");
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
				T = Tsat1;
				rho = rhof1;
#endif
				}else{
				T = fluid->data->T_c * 1.01;
				rho = fluid->data->rho_c * 1.05;
				}
			MSG("SUPERCRITICAL GUESS: T = %f, rho = %f", T, rho);
		}

		MSG("STARTING NON-SAT ITERATION");
		//*rho = 976.82687191126922;
		//*T = 344.80371310850518;

		T1 = T;
		rho1 = rho;
		assert(!isnan(T1));
		assert(!isnan(rho1));

		if(liquid_iteration){
			double pt,rhogt;
			fprops_triple_point(&pt, &rhof_t, &rhogt, fluid, err);
			if(*err){
				ERRMSG("Unable to solve triple point liquid density.");
				*err = FPROPS_SAT_CVGC_ERROR;
				return STATE_TRHO(fluid,T,rho);
			}
		}

		/* try our own home-baked newton iteration */
		int i = 0;
		*err = FPROPS_NO_ERROR;
		double delta_T = 0;
		double delta_rho = 0;
		MSG("STARTING ITERATION");
		MSG("rhof_t = %f",rhof_t);
		while(i++ < 200){
			//FluidState2 S1 = fprops_set_Trho(T1,rho1,fluid,err);
			//double p1 = fprops_p(S1,err);
			double p1 = fluid->p_fn(FSU_TRHO(T1,rho1), fluid->data, err);
			if(*err){
				MSG("Got an error ('%s') in p calculation",fprops_error(*err));
			} 
			double h1 = fluid->h_fn(FSU_TRHO(T1,rho1), fluid->data, err);
			if(*err){
				MSG("Got an error ('%s') in fprops_h calculation",fprops_error(*err));
			}
			assert(!isnan(h1));

			if(i >= 2){
				int nred = 10;
				while(p1 <= 0 && nred){
					rho1 = rho1 - delta_rho;
					T1 = T1 - delta_T;
					delta_rho *= 0.5;
					delta_T *= 0.5;
					rho1 = rho1 + delta_rho;
					T1 = T1 + delta_T;
					//S1 = fprops_set_Trho(T1,rho1,fluid,err);
					p1 = fluid->p_fn(FSU_TRHO(T1,rho1), fluid->data, err);
					MSG("Set smaller step as p < 0. T1 = %f, rho1 = %f --> p1 = %f",T1, rho1, p1);
					nred--;
				}
			}

			MSG("  %d: T = %f, rho = %f\tp = %f bar, h = %f kJ/kg", i, T1, rho1, p1/1e5, h1/1e3);
			//MSG("      p error = %f bar",(p1 - p)/1e5);
			//MSG("      h error = %f kJ/kg",(h1 - h)/1e3);

			if(p1 < 0){
				MSG("p1 < 0, reducing dT, drho");
				T1 -= (delta_T *= 0.5);
				rho1 -= (delta_rho *= 0.5);
				continue;
			}

			if(fabs(p1 - p) < 1e-4 && fabs(h1 - h) < 1e-8){
				MSG("Converged to T = %f, rho = %f, in homebaked Newton solver", T1, rho1);
				return STATE_TRHO(fluid,T1,rho1);
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
#if 1
			if(liquid_iteration){
				MSG("rho1 = %f, delta_rho = %f, rho1+delta_rho = %f", rho1, delta_rho, rho1+delta_rho);
				if(rho1 + delta_rho > rhof_t){
					MSG("Limit rho to be less than rhof_t");
					delta_rho = rhof_t - rho1;
				}
			}
#endif

			/* don't go too hot */
			if(T1 + delta_T > 5000) delta_T = 5000 - T1;

			/* avoid huge step */
			while(fabs(delta_T / T1) > 0.7){
				MSG("Reduce delta_T");
				delta_T *= 0.5;
			}

			/* NOTE if step limit is less than 0.5, we're getting errors */
			while(fabs(delta_rho / rho1) > 0.7){
				MSG("Reduce delta_rho");
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

	ERRMSG("Iteration failed for '%s' with p = %.12e, h = %.12e",fluid->name, p,h);
	*err = FPROPS_NUMERIC_ERROR;
	return STATE_TRHO(fluid,T1,rho1);

#if 0
	int res = fprops_nonsolver('p','h',p,h,T,rho,fluid);
	ERRMSG("Iteration failed in nonsolver");
	return res;
#endif
}

typedef struct IterationData_struct{
	double h;
	double p;
	const PureFluid *fluid;
	FpropsError *err;
} IterationData;
static double herr_T(double T, void *user_data){
	IterationData *data = (IterationData *)user_data;
	FluidState2 S = fprops_set_Tp(T, data->p, data->fluid,data->err);
	MSG("state S: p = %f, T = %f",fprops_p(S,data->err),fprops_T(S,data->err));
	double h = data->fluid->h_fn(S.vals,data->fluid->data,data->err);
	MSG("At T = %f, got h = %f (target %f, h_const = %f)", T, h, data->h, data->fluid->data->corr.incomp->const_h);
	return h - data->h;
}

static FluidState2 fprops_solve_ph_incomp(double p, double h, const PureFluid *fluid, FpropsError *err){

	IterationData data = {h, p, fluid, err};
	double T;
	double herr;
	MSG("Solving for (h = %f, p %f) by varying T",h,p);
	if(zeroin_solve(&herr_T, &data, 200, 2000., 1e-12, &T, &herr)){
		ERRMSG("Failed to convert (p=%f bar,h=%f kJ/kg) for incompressible fluid '%s'",p/1e5,h/1e3,fluid->name);
		*err = FPROPS_NUMERIC_ERROR;
		return STATE_NAN(fluid);
	}
	MSG("Solved: T = %f", T);
	return STATE_TP(fluid,T,p);

}
