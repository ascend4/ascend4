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
	This file contains the ida_main_solve function.
	This is the main solver function that calls other 
	sub-functions to complete integration.
	The current outline of this function is as below:
	Boundary flags initialisation: all the condition equations are evaluated 
		and the values of corresponding boundary flags are set.
	Boolean variables initialisation: the system is solved using the logical 
		equations solver in order to get the correct values of Boolean variables.
	Using the logical values found at step 2 the configuration of the system is chosen.
	Solver initialisation: setting solver parameters, allocating memory, et cetera.
	Loop through all time steps:
		If a root has not been found at the previous iteration then then reset all 
			the root direction settings. It means that IDA will be searching for 
			roots of condition equations in both directions.
		If a root has been found and we are far from desired output then make a very 
			small time step and solve the system with IDA. Else make a usual time 
			step which is set by the user.
		If we have made a small time step then call a logical solver in order to update 
			the values of logical variables.
		Check if a root has been found at this step. If yes:
			Call a function for processing this boundary.
			If there have been any changes in the system, then reinitialise the solver.
			Set the direction in which the roots of condition equations will be detected: 
				if a boundary has been crossed, then at the next step (which is an 
				auxiliary small step) it will not be crossed in the same direction.
			If we are still far from desired output then repeat steps.

	So basically we have two modes here: the first is when we make an ordinary time step, check all 
	roots and if a boundary is found then call the function for processing it. The second mode is entered.
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
#include <signal.h>										/*Check if all these includes are necessary*/
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

 
int ida_main_solve(IntegratorSystem *integ, unsigned long start_index, unsigned long finish_index){
	void *ida_mem;
	int t_index;
	realtype t0, tout, tret, tol;
	realtype tmin;				/** < The length of a small additional step made after an event is triggered */
	N_Vector ypret, yret;
	IntegratorIdaData *enginedata;
	int i, flag;
	int *rootsfound;			/** < IDA rootfinder reports root index in here */
	int *rootdir = NULL;				/** < Used to tell IDA to ignore doulve crossings */
	int *bnd_cond_states;		/** < Record of boundary states so that IDA can tell LRSlv how to evaluate a boundary crossing */
	int *bnd_not_set;
	int all_bnds_set = 1;
	int need_to_reconfigure;	/** < Flag to indicate system rebuild after crossing */
	int need_to_reinteg = 0;	/** < Flag for when crossings happen on or very close to timesteps */
	int skipping_output;		/** < Flag to skip output to reporter */
	int qrslv_ind, lrslv_ind;
	int after_root = 0;
	int peaw = 0;                  /*Flag returned by process_events_and_whens*/
	int subpeaw = 0; 		/*Sub-flag used to control auxiliay integration steps*/
#if defined(SOLVE_DEBUG) || defined(IDA_BND_DEBUG)
	char *relname;
	//CONSOLE_DEBUG("STARTING IDA...");
#endif
#ifdef SOLVE_DEBUG
	integrator_ida_debug(integ, stderr);
#endif
	/* store reference to list of relations (in enginedata) */
	ida_load_rellist(integ);
	/* create IDA object */
	ida_mem = IDACreate();
	/* Setup parameter inputs and initial conditions for IDA. */
	call to solve_initial_conditions_and_whens;
	/* solve the initial conditions, allocate memory, other stuff... */
	ida_prepare_integrator(integ, ida_mem, tout);						/*Change call to new function*/
	tol = 0.0001*(samplelist_get(integ->samples, finish_index) 
					- samplelist_get(integ->samples, start_index))
				/samplelist_length(integ->samples);
	tmin = tol;
	/* -- set up the IntegratorReporter */
	integrator_output_init(integ);
	/* -- store the initial values of all the stuff */
	integrator_output_write(integ);
	integrator_output_write_obs(integ);
	/* specify where the returned values should be stored */
	yret 	= ida_bnd_new_zero_NV(integ->n_y);
	ypret 	= ida_bnd_new_zero_NV(integ->n_y);
	/* advance solution in time, return values as yret and derivatives as ypret */
	integ->currentstep = 1;
	rootdir = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds); 
	

	for(t_index = start_index + 1; t_index <= finish_index; ++t_index, ++integ->currentstep){
		tout = samplelist_get(integ->samples, t_index);
		t0 = integrator_get_t(integ);
		asc_assert(tout > t0);
#ifdef SOLVE_DEBUG
		CONSOLE_DEBUG("Integrating from t0 = %f to t = %f", t0, tout);
#endif			
		if(integ->nbnds){
			peaw = process_events_and_whens(IntegratorSystem *integ, void *ida_mem, 
			unsigned long t_index, unsigned long finish_index);}
	

	
	 	if(peaw==143){								/*Flag for rootsfound - system is reconfigured already!*/
			
			subpeaw = 0;

			flag = IDASolve(ida_mem, t0 + tmin, &t0, yret, ypret, IDA_NORMAL);
			

		  	2. Call process_events_and_whens again and see if any new roots have been detected. Take rootdir into consideration.
		  	3. After reconfiguring, continue with integration
			4. Increment timestep. 		
		}




		
	 	if(peaw==657){
			flag = IDASolve(ida_mem, tout, &t0, yret, ypret, IDA_NORMAL);	
		}




	

	}/*End of For Loop*/

	
}	