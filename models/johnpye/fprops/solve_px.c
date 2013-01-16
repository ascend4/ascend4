/*
ASCEND modelling environment
Copyright (C) 2004-2012 John Pye

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

#include "solve_px.h"
#include "sat.h"
#include "derivs.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

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

#define SQ(X) ((X)*(X))

#define SOLVE_PX_DEBUG
#ifdef SOLVE_PX_DEBUG
# define MSG(STR,...) fprintf(stderr,"%s:%d: " STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define MSG(ARGS...)
#endif

#define ERRMSG(STR,...) fprintf(stderr,"%s:%d: ERROR: " STR "\n", __func__, __LINE__ ,##__VA_ARGS__)

/**
	Return region specification for given (p,x) property pair. Basically just
	checks 0<x<1 and p_t < p < p_c, since we must inside the saturation region.
*/
int fprops_region_px(double p, double x, const PureFluid *fluid, FpropsError *err){
	double p_t, rhof_t, rhog_t;
	fprops_triple_point(&p_t, &rhof_t, &rhog_t, fluid, err);
	if(*err){
		ERRMSG("Unable to solve triple point");
		return FPROPS_ERROR;
	}
	if(x < 0 || x > 1)return FPROPS_ERROR;
	if(p > fluid->data->p_c)return FPROPS_NON;
	if(p < p_t)return FPROPS_ERROR;
	return FPROPS_SAT;
}

/**
	Determine (T,rho) for specified (p,x). Requires use of phase equilbrium
	solver fprops_sat_p which can be very costly. We would like to make it
	cheaper by implementing alternative saturation calculations, since
	calculations in terms of p, eg (p,h) are desirable for energy system sims.
*/
void fprops_solve_px(double p, double x, double *T, double *rho, const PureFluid *fluid, FpropsError *err){
	double T_sat, rho_f, rho_g;
	double p_t, rhof_t, rhog_t;
	if(*err){
		ERRMSG("ERROR FLAG ALREADY SET");
	}
	fprops_triple_point(&p_t, &rhof_t, &rhog_t, fluid, err);
	if(*err){
		ERRMSG("Unable to solve triple point");
		return;
	}

	assert(rho != NULL);
	assert(fluid != NULL);
	assert(err != NULL);
	
	if(p > fluid->data->p_c){
		ERRMSG("Pressure (%f) exceeds critical pressure (%f)",p, fluid->data->p_c);
		*err = FPROPS_RANGE_ERROR;
		return;
	}
	if(x < 0 || x > 1){
		ERRMSG("Quality x should be in range [0,1]");
		*err = FPROPS_RANGE_ERROR;
		return;
	}
	if(p < p_t){
		ERRMSG("Pressure is below triple point");
		*err = FPROPS_RANGE_ERROR;
		return;
	}

	fprops_sat_p(p, &T_sat, &rho_f, &rho_g, fluid, err);
	if(*err){
		ERRMSG("Unable to solve saturation state at p = %f (p_c = %f)", p, fluid->data->p_c);
		*err = FPROPS_SAT_CVGC_ERROR;
		return;
	}

	double v = (1./rho_f) * (1 - x) + (1./rho_g) * x;
	*T = T_sat;
	*rho = 1./ v;
}

