/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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

*//*
	by Harry, 4th June 2015
	This file contains the solve_initial_conditions_and_whens function.
	The function acts to compute initial conditions and also handle 
	semi-implict initial conditions. The initial outline for the function is as 
	follows:

	solve_initial_conditions_and_whens would be
		once the continuous part of the problem is solved, evaluate the bnds (conditionals)/check for roots, 
		then run LRSlv (to update event booleans).
		If that results in triggering any changes to WHEN cases, then 
			reanalyse the equations
			reconfigure IDA
			go back and solve the continuous problem again (loop)
			If there are too many iterations, 
				then we should return an error to the user.
		we don't want to trigger any EVENTs during this process; only WHENs.
	

*/





#define _GNU_SOURCE								

#include "ida.h"
#include "idalinear.h"
#include "idaanalyse.h"
#include "idatypes.h"										/* The list of includes needs cleaning up! Once basic files are written,  
#include "idaprec.h"										all commonly called functions can be added in just one header file.
#include "idacalc.h"										Once that's done, these includes must be revisited*/
#include "idaio.h"
#include "idaboundary.h"

#include <signal.h>										/*Check if these includes are necessary*/
#include <setjmp.h>
#include <fenv.h>
#include <math.h>

#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/panic.h>
#include <ascend/compiler/instance_enum.h>


#include <ascend/system/slv_client.h>
#include <ascend/system/relman.h>
#include <ascend/system/block.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/jacobian.h>
#include <ascend/system/bndman.h>

#include <ascend/utilities/config.h>
#include <ascend/integrator/integrator.h>

 
int solve_initial_conditions_and_whens(IntegratorSystem *integ, void *ida_mem){
	


	/*-------------------                              CHECK FOR ROOTS                      --------------------*
			     In this block, we use IDARootInit to check for some of the roots in our model. 
 	*-----------------------------------------------------------------------------------------------------------*/

	IntegratorIdaData *enginedata = integ->enginedata;

	if(enginedata->nbnds){
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR>=4
		IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn);				/*This checks for roots. The next step is to run LRSLV to get the present values of boolean and other variables*/ 
#else
		IDARootInit(ida_mem, enginedata->nbnds, &integrator_ida_rootfn,
				(void *) integ);
#endif
	}



	/* Initialise boundary condition states if appropriate. Reconfigure if necessary */
	if(enginedata->nbnds){
		CONSOLE_DEBUG("Initialising boundary states");
#if SUNDIALS_VERSION_MAJOR==2 && SUNDIALS_VERSION_MINOR<4
		ERROR_REPORTER_HERE(ASC_PROG_WARNING, "Warning: boundary detection is"
				"unreliable with SUNDIALS pre version 2.4.0. Please update if you"
				"wish to use IDA for conditional integration");
#endif
		bnd_cond_states = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);

		/* identify if we're exactly *on* any boundaries currently */
		for(i = 0; i < enginedata->nbnds; i++) {
#ifdef IDA_BND_DEBUG
			relname = bnd_make_name(integ->system,enginedata->bndlist[i]);
#endif
			bnd_cond_states[i] = bndman_calc_satisfied(enginedata->bndlist[i]);
			bnd_set_ida_first_cross(enginedata->bndlist[i],1);
			if(bndman_real_eval(enginedata->bndlist[i]) == 0) {
				/* if the residual for the boundary is zero (ie looks like we are *on* the boundary?) JP */
#ifdef IDA_BND_DEBUG
				CONSOLE_DEBUG("Boundary '%s': not set",relname);
#endif
				bnd_not_set[i] = 1;
				all_bnds_set = 0;
			}else{
				bnd_not_set[i] = 0;
			}
#ifdef IDA_BND_DEBUG
			CONSOLE_DEBUG("Boundary '%s' is %d",relname,bnd_cond_states[i]);
			ASC_FREE(relname);
#endif
		}
		CONSOLE_DEBUG("Setting up LRSlv...");
		if(ida_setup_lrslv(integ,qrslv_ind,lrslv_ind)){
			ERROR_REPORTER_HERE(ASC_USER_ERROR, "Idaanalyse failed.");
			return 1;
		}

	}











			/*TO BE CONTINUED*/










return 0;		/*solve_initial_conditions_and_whens*/					
}	