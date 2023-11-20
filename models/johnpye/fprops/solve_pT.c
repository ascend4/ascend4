/*
ASCEND modelling environment
Copyright (C) 2020-2022 John Pye

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
*//** @file
	Solve state of a fluid for fixed (p,T).
	
	We'll start by implementing this just for pengrob EOS, and see where that
	takes us.
*/

#include "solve_pT.h"
#include "fprops.h"
#include "sat.h"
#include "derivs.h"
#include "rundata.h"
#include "zeroin.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

//#define SOLVE_PT_DEBUG
#define SOLVE_PT_ERRORS

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

#ifdef SOLVE_PT_DEBUG
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

#ifdef SOLVE_PT_ERRORS
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
int fprops_region_pT(double p, double T, const PureFluid *fluid, FpropsError *err){

	switch(fluid->type){
	case FPROPS_HELMHOLTZ:
	case FPROPS_PENGROB:
		{
			double p_c = fluid->data->p_c;
			double T_c = fluid->data->T_c;
			if(p > p_c || T > T_c) return FPROPS_NON;

			// Note: to return 'saturated', we need the EXACT (p,T) pair:
			double Tsat, rhof, rhog;
			fprops_sat_p(p, &Tsat, &rhof, &rhog, fluid, err);
			if(*err){
				*err = FPROPS_SAT_CVGC_ERROR;
				return FPROPS_ERROR;
			}
	
			if(T == Tsat)return FPROPS_SAT;
			return FPROPS_NON;
		}
	case FPROPS_INCOMP:
	case FPROPS_IDEAL:
		// ideal and incompressible fluids are always non-saturated.
		return FPROPS_NON;
	default:
		ERRMSG("Unsupported fluid (with p < p_c)");
		*err = FPROPS_NOT_IMPLEMENTED;
		return FPROPS_ERROR;
	}
}

#define STATE_NAN(FLUID) (FluidState2){.vals={.Trho={NAN,NAN}},.fluid=FLUID}
#define STATE_TRHO(FLUID,T,RHO) (FluidState2){.vals={.Trho={T,RHO}},.fluid=FLUID}
#define STATE_TP(FLUID,T,P) (FluidState2){.vals={.Tp={T,P}},.fluid=FLUID}
static FluidState2 fprops_solve_pT_Trho(double p, double T, const PureFluid *fluid, FpropsError *err);
static FluidState2 fprops_solve_pT_incomp(double p, double T, const PureFluid *fluid, FpropsError *err);

FluidState2 fprops_solve_pT(double p, double h, const PureFluid *fluid, FpropsError *err){
	if(!fluid){
		ERRMSG("'fluid' is NULL!");
		*err = FPROPS_INVALID_REQUEST;
		return STATE_NAN(fluid);
	}
	switch(fluid->type){
	case FPROPS_HELMHOLTZ:
	case FPROPS_PENGROB:
		return fprops_solve_pT_Trho(p,h,fluid,err);
	case FPROPS_INCOMP:
		return fprops_solve_pT_incomp(p,h,fluid,err);
	default:
		ERRMSG("Unsupported fluid type");
		*err = FPROPS_NOT_IMPLEMENTED;
		return STATE_NAN(fluid);
	}
}
static FluidState2 fprops_solve_pT_Trho(double p, double T, const PureFluid *fluid, FpropsError *err){

	ERRMSG("Unsupported fluid type");
	*err = FPROPS_NOT_IMPLEMENTED;
	return STATE_NAN(fluid);

	// not the that pT are not independent in the saturation region, so saturated states cannot be recovered.
}

static FluidState2 fprops_solve_pT_incomp(double p, double T, const PureFluid *fluid, FpropsError *err){
	// trivial!
	MSG("Creating fluid state at (p = %f bar, T = %f K)",p/1e5,T);
	return STATE_TP(fluid,T,p);
}

