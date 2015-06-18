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
	int* bnd_not_set = ASC_NEW_ARRAY(int, enginedata->nbnds);
	int eflag = 0;			/*to prevent multiple assignment of all_bnds_set*/
	int all_bnds_set = 1;
	int qrslv_ind, lrslv_ind;
	int *rootsfound; 
	qrslv_ind = slv_lookup_client("QRSlv");
	lrslv_ind = slv_lookup_client("LRSlv");
	


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
				"unreliable with SUNDIALS pre version 2.4.0. Please update if you"		/**/ 
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
#ifdef IDA_BND_DEBUG
				CONSOLE_DEBUG("Boundary '%s': not set",relname);
#endif
				bnd_not_set[i] = 1;
				if(eflag == 0){
					all_bnds_set = 0;
					eflag = 1;
				}

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

	
	

	int system_discrete_change = 0;
	struct dis_discrete **dvlist, *cur_dis;									/*Checks if any of the system discrete variabes have changed*/
	int numDVs, i;
#ifdef IDA_BND_DEBUG
	char *dis_name;
#endif

	dvlist = slv_get_solvers_dvar_list(sys);
	numDVs = slv_get_num_solvers_dvars(sys);

	for (i = 0; i < numDVs; i++) {
		cur_dis = dvlist[i];
		if((dis_kind(cur_dis) == e_dis_boolean_t)
			&& (dis_inwhen(cur_dis) || dis_inevent(cur_dis))
		){
			if(dis_value(cur_dis) != dis_previous_value(cur_dis)){
#ifdef IDA_BND_DEBUG
				dis_name = dis_make_name(sys, cur_dis);
				CONSOLE_DEBUG("Boolean %s (i=%d) has changed (current=%d, prev=%d)", dis_name,
						i, dis_value(cur_dis), dis_previous_value(cur_dis));
				ASC_FREE(dis_name);
#endif
				system_discrete_change = 1;
			}
		}
	}
	if(!system_discrete_change){
		CONSOLE_DEBUG("No boundary vars have changed");
	}



														/*Uses LRSlv to update bools. Additional work required from here on: Work on 'return' statements. 
														Also check again. SYstem must loop until no further changes are seen. Check with Ksenija*/




	if(system_discrete_change){
		slv_status_t status;
		struct bnd_boundary *bnd = NULL;
		int i, num_bnds, num_dvars;
		struct dis_discrete **dvl;
		int32 c, *prevals = NULL;
		double *bnd_prev_eval;
		CONSOLE_DEBUG("LRSlv Updating Bools");
		integrator_output_write(integ);
		integrator_output_write_obs(integ);
		integrator_output_write_event(integ);
		/*^^^ FIXME should we write the above event? It may not have crossed in correct direction...? 
		Harry: In this case, we are only analyzing initial conditions. Therefore it has to be written?*/
		/* Flag the crossed boundary and update bnd_cond_states */
		enginedata = integ->enginedata;
		num_bnds = enginedata->nbnds;
		bnd_prev_eval = ASC_NEW_ARRAY(double,num_bnds);
		for(i = 0; i < num_bnds; i++) {
			/* get the value of the boundary condition before crossing */
			bnd_prev_eval[i] = bndman_real_eval(enginedata->bndlist[i]);
			if(rootsfound[i]){										/*rootsfound not assigned so far. Must be resolved*/
				bnd = enginedata->bndlist[i];
#ifdef IDA_BND_DEBUG
				char *name = bnd_make_name(integ->system,enginedata->bndlist[i]);
				CONSOLE_DEBUG("Boundary '%s': ida_incr=%d, dirn=%d,ida_value=%d,ida_first_cross=%d,bnd_cond_states[i]=%d"
				,name,bnd_ida_incr(bnd),rootsfound[i],bnd_ida_value(bnd)
				,bnd_ida_first_cross(bnd)!=0,bnd_cond_states[i]
				);
#endif
				bnd_set_ida_crossed(bnd, 1);
				if(bnd_ida_first_cross(bnd) /* not crossed before */
				|| !((rootsfound[i] == 1 && bnd_ida_incr(bnd)) /* not crossing upwards twice in a row */
				|| (rootsfound[i] == -1 && !bnd_ida_incr(bnd))) /* not crossing downwards twice in a row */
				){
					if(bnd_cond_states[i] == 0){
						bnd_set_ida_value(bnd, 1);
						bnd_cond_states[i] = 1;
					}else{
						bnd_set_ida_value(bnd, 0);
						bnd_cond_states[i] = 0;
					}
#ifdef IDA_BND_DEBUG
					CONSOLE_DEBUG("Set boundary '%s' to %d (single cross)",name,bnd_ida_value(bnd)!=0);
#endif
				}else{
					/* Boundary crossed twice in one direction. Very unlikey! 
					The aim of the following two lines is to set the value of 
					the boolean variable to such a value as if the boundary 
					was crossed in the opposite direction before */
					CONSOLE_DEBUG("DOUBLE CROSS");
					if(!prevals){
						num_dvars = slv_get_num_solvers_dvars(integ->system);
						dvl = slv_get_solvers_dvar_list(integ->system);
						prevals = ASC_NEW_ARRAY(int32,num_dvars);
						for(c = 0; dvl[c] != NULL; c++)
							prevals[c] = dis_value(dvl[c]);
					}
					bnd_set_ida_value(bnd, !bnd_cond_states[i]);
						if(!ida_log_solve(integ,lrslv_ind)){
						CONSOLE_DEBUG("Error in logic solve in double-cross");
						return -1;
						}
					bnd_set_ida_value(bnd,bnd_cond_states[i]);
#ifdef IDA_BND_DEBUG
					CONSOLE_DEBUG("After double-cross, set boundary '%s' to %d",name,bnd_ida_value(bnd)?1:0);
#endif
				}
				/* flag this boundary as having previously been crossed */
				bnd_set_ida_first_cross(bnd,0);
				/* store this recent crossing-direction for future reference */
				bnd_set_ida_incr(bnd,(rootsfound[i] > 0));
#ifdef IDA_BND_DEBUG
			ASC_FREE(name);
#endif
			}
		}
		if(!ida_log_solve(integ,lrslv_ind)) return -1;

		/* If there was a double crossing, because of ida_log_solve the previous values
		of discrete variables may be equal to their current values, which would mean that
		the discrete vars haven't changed their values, but actually they did. So here we
		restore the previous values of those variables, which are not connected with the
		boundary which was double-crossed. */
		if(prevals){
			for(c = 0; dvl[c] != NULL; c++)
				if(!(dis_value(dvl[c]) == prevals[c] && dis_value(dvl[c]) != dis_previous_value(dvl[c])))
					dis_set_previous_value(dvl[c],prevals[c]);
			ASC_FREE(prevals);
		}

		integrator_output_write(integ);
		integrator_output_write_obs(integ);
		integrator_output_write_event(integ);
		if(!some_dis_vars_changed(integ->system)){
			CONSOLE_DEBUG("Crossed boundary but no effect on system");
			/* Boundary crossing that has no effect on system */
			ASC_FREE(bnd_prev_eval);
			/* Reset the boundary flag */
			for(i = 0; i < num_bnds; i++)
				bnd_set_ida_crossed(enginedata->bndlist[i], 0);
			return 0;
		}
		/* update the main system if required */
		int events_triggered = 1;
		if(slv_get_num_solvers_events(integ->system)!=0) {
			while(some_dis_vars_changed(integ->system) && events_triggered)  {
				if(ida_bnd_reanalyse(integ)) {
					/* select QRSlv solver, and solve the system */
					if(slv_select_solver(integ->system, qrslv_ind) == -1) {
						ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error attempting to load QRSlv");
					}
#ifdef IDA_BND_DEBUG
				{
				struct var_variable **vl = slv_get_solvers_var_list(integ->system);
				int i, n=slv_get_num_solvers_vars(integ->system), first=1;
				CONSOLE_DEBUG("In boundary problem: variables (active, incident, free):");
					for(i=0;i<n;++i){
						if(var_incident(vl[i])&&!var_fixed(vl[i])&&var_active(vl[i])){
							char *name = var_make_name(integ->system,vl[i]);
							fprintf(stderr,"%s%s",(first?"\t":", "),name);
							first=0; ASC_FREE(name);
						}
					}
					fprintf(stderr,"\n");
				}
				{
					struct rel_relation **rl = slv_get_solvers_rel_list(integ->system);
					int i, n=slv_get_num_solvers_rels(integ->system), first=1;
					CONSOLE_DEBUG("...relations (equality, included, active):");
					for(i=0;i<n;++i){
						if(rel_equality(rl[i])&&rel_active(rl[i])&&rel_included(rl[i])){
							char *name = rel_make_name(integ->system,rl[i]);
							fprintf(stderr,"%s%s",(first?"\t":", "),name);
							first=0; ASC_FREE(name);
						}
					}
					fprintf(stderr,"\n");
				}
#endif
				slv_presolve(integ->system);
				slv_solve(integ->system);

				slv_get_status(integ->system, &status);
				if (!status.converged) {
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in "
						"non-linear solver at boundary");
#ifdef IDA_BND_DEBUG
				}else{
					CONSOLE_DEBUG("Converged");
#endif
				}

				for (i = 0; i < num_bnds; i++) {
					if (!rootsfound[i]) {
						if ((bndman_real_eval(enginedata->bndlist[i]) > 0 && bnd_prev_eval[i] < 0) ||
						(bndman_real_eval(enginedata->bndlist[i]) < 0 && bnd_prev_eval[i] > 0)) {
							bnd_cond_states[i] = !bnd_cond_states[i];
							if (bnd_prev_eval[i] > 0) {
								bnd_set_ida_incr(enginedata->bndlist[i],0);
								rootsfound[i] = -1;
							}
							else {
								bnd_set_ida_incr(enginedata->bndlist[i],1);
								rootsfound[i] = 1;
							}
							bnd_set_ida_value(enginedata->bndlist[i],bnd_cond_states[i]);
						}
					}
				}
				if (!ida_log_solve(integ,lrslv_ind)) return -1;

			}else events_triggered = 0;
			if (ida_bnd_reanalyse_cont(integ)) return 2;
			if (events_triggered) {
				integrator_output_write(integ);
				integrator_output_write_obs(integ);
				integrator_output_write_event(integ);
			}
		}
	}

	integrator_output_write(integ);
	integrator_output_write_obs(integ);
	integrator_output_write_event(integ);

	ASC_FREE(bnd_prev_eval);

	/* Reset the boundary flag */
	for (i = 0; i < num_bnds; i++)
		bnd_set_ida_crossed(enginedata->bndlist[i], 0);
	return 1;




















			/*TO BE CONTINUED*/










return 0;		/*solve_initial_conditions_and_whens*/					
}	