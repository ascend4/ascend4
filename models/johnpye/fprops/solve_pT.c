/*
ASCEND modelling environment
Copyright (C) 2022 John Pye

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
#include "sat.h"
#include "fprops.h"
#include "pengrob.h"

//#include "sat.h"
//#include "derivs.h"
#include "rundata.h"

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

#define SOLVE_PT_DEBUG
#ifdef SOLVE_PT_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

int fprops_region_pT(double p, double T, const PureFluid *fluid, FpropsError *err){

	double Tsat, rhof, rhog;
	double p_c = fluid->data->p_c;

	if(p >= p_c)return FPROPS_NON;

	fprops_sat_p(p, &Tsat, &rhof, &rhog, fluid, err);
	if(*err){
		*err = FPROPS_SAT_CVGC_ERROR;
		return FPROPS_ERROR;
	}
	
	if(T == Tsat) return FPROPS_SAT; // FIXME is any tolerance needed here?
	return FPROPS_NON;
}

void fprops_solve_pT(double p, double T, double *rho
		, const PureFluid *fluid, FpropsError *err
){
	if(fluid->type == FPROPS_PENGROB){
		return pengrob_solve_pT(p,T,rho,fluid->data,err);
	}else{
		*err = FPROPS_NOT_IMPLEMENTED;
		return;
	}
}

