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

#include "solve_Tx.h"
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

//#define SOLVE_TX_DEBUG
#ifdef SOLVE_TX_DEBUG
# define MSG(STR,...) fprintf(stderr,"%s:%d: " STR "\n", __func__, __LINE__ ,##__VA_ARGS__)
#else
# define MSG(ARGS...)
#endif

#define ERRMSG(STR,...) fprintf(stderr,"%s:%d: ERROR: " STR "\n", __func__, __LINE__ ,##__VA_ARGS__)

/**
	Return region specification for given (T,x) property pair. This is a silly
	function because if x is specified then you MUST be in the saturation region
	(unless you do something whacky with the definition of x, which we don't do)
	So this function just checks that the parameters are all within allowable
	limits for the fluid in question.
*/
int fprops_region_Tx(double T, double x, const PureFluid *fluid, FpropsError *err){

	if(x < 0 || x > 1)return FPROPS_ERROR;
	if(T > fluid->data->T_c)return FPROPS_NON;
	if(T < fluid->data->T_t)return FPROPS_ERROR;

	return FPROPS_SAT;
}

/**
	Determine (T,rho) for specified (T,x). This is a very simple property pair
	because one of the inputs is already one of the outputs. But we write this
	function to provide a uniform API for users.
*/
void fprops_solve_Tx(double T, double x, double *rho, const PureFluid *fluid, FpropsError *err){
	double p_sat, rho_f, rho_g;

	assert(rho != NULL);
	assert(fluid != NULL);
	assert(err != NULL);
	
	if(T > fluid->data->T_c){
		ERRMSG("Temperature (%f) exceeds critical temperature (%f)",T, fluid->data->T_c);
		*err = FPROPS_RANGE_ERROR;
		return;
	}
	if(x < 0 || x > 1){
		ERRMSG("Quality x should be in range [0,1]");
		*err = FPROPS_RANGE_ERROR;
		return;
	}
	if(T < fluid->data->T_t){
		ERRMSG("Temperature is below triple point");
		*err = FPROPS_RANGE_ERROR;
		return;
	}

	fprops_sat_T(T, &p_sat, &rho_f, &rho_g, fluid, err);
	if(*err){
		ERRMSG("Unable to solve saturation state at T = %f (T_c = %f) for '%s'", T,fluid->data->T_c,fluid->name);
		*err = FPROPS_SAT_CVGC_ERROR;
		return;
	}

	double v = (1./rho_f) * (1 - x) + (1./rho_g) * x;
	*rho = 1./ v;
}

