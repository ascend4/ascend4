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
	This file contains the process_events_and_whens function.
	The current outline of this function is as below:
	process_events_and_whens would then
		solve the logrels
		if no values changed, 
			return
		if event cases were triggered (ie a positive edge):
			write_timestep_data
			write_event
			if the event has 'USE' statement(s), 
				run the METHOD, 
				run QRSlv, 
				run the _end method, return
			write time_step data
		if when cases were triggered,
			update the solver rel lists
		if structure has changed (how to tell?), 
			reanalyse lists and reconfigure IDA
	
	

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

 
int process_events_and_whens(IntegratorSystem *integ, void *ida_mem, unsigned long start_index, unsigned long finish_index, int* rootdir, int first_run){
	int t_index;
	realtype t0, tout, tret, tol;
	realtype tmin;				/** < The length of a small additional step made after an event is triggered */
	N_Vector ypret, yret;
	IntegratorIdaData *enginedata;
	int i, flag;
	int *rootsfound;			/** < IDA rootfinder reports root index in here */
	int *bnd_cond_states;		/** < Record of boundary states so that IDA can tell LRSlv how to evaluate a boundary crossing */
	int *bnd_not_set;
	int all_bnds_set = 1;
	int need_to_reconfigure = 0;	/** < Flag to indicate system rebuild after crossing */
	int qrslv_ind, lrslv_ind;
	int after_root = 0;
	int system_discrete_change = 0;
	int numDVs, i;
	slv_status_t status;
	struct bnd_boundary *bnd = NULL;
	int i, num_bnds, num_dvars;
	struct dis_discrete **dvl;
	int32 c, *prevals = NULL;
	double *bnd_prev_eval;
	int updateflag = 400;
	int reanalyseflag = 400;
	int first_run = 1;

	struct dis_discrete **dvlist, *cur_dis;	
#if defined(SOLVE_DEBUG) || defined(IDA_BND_DEBUG)
	char *relname;
	//CONSOLE_DEBUG("STARTING IDA...");
#endif


	if(first_run){
	/***********HAVE TO SOLVE INITIAL CONDITIONS**************/
	}


	/* Setup boundary list */
	enginedata = integrator_ida_enginedata(integ);
	enginedata->bndlist = slv_get_solvers_bnd_list(integ->system);
	enginedata->nbnds = slv_get_num_solvers_bnds(integ->system);
	enginedata->safeeval = SLV_PARAM_BOOL(&(integ->params),IDA_PARAM_SAFEEVAL);
	//CONSOLE_DEBUG("safeeval = %d",enginedata->safeeval);    
	qrslv_ind = slv_lookup_client("QRSlv");
	lrslv_ind = slv_lookup_client("LRSlv");
	bnd_not_set = ASC_NEW_ARRAY(int,enginedata->nbnds);
	if(enginedata->nbnds) IDASetRootDirection(ida_mem, rootdir);
#ifdef SOLVE_DEBUG
	integrator_ida_debug(integ, stderr);
#endif
	/* store reference to list of relations (in enginedata) */
	ida_load_rellist(integ);
	/* Setup parameter inputs and initial conditions for IDA. */
	tout = samplelist_get(integ->samples, start_index + 1);


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
		}
#ifdef IDA_BND_DEBUG
		for(i = 0; i < enginedata->nbnds; i++) {
			CONSOLE_DEBUG("Boundary '%s' is %d",relname,bnd_cond_states[i]);
			ASC_FREE(relname);
		}
#endif
		CONSOLE_DEBUG("Setting up LRSlv...");
		if(ida_setup_lrslv(integ,qrslv_ind,lrslv_ind)){
			ERROR_REPORTER_HERE(ASC_USER_ERROR, "Idaanalyse failed.");
			return 1;
		}
	}
#ifdef ASC_SIGNAL_TRAPS
	Asc_SignalHandlerPushDefault(SIGINT);
	if(setjmp(g_int_env) == 0) {
#endif
#ifdef ASC_SIGNAL_TRAPS
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Caught interrupt");
		flag = -555;
	}
	Asc_SignalHandlerPopDefault(SIGINT);
#endif

	if(!IDA_ROOT_RETURN){
		need_to_reconfigure = ida_cross_boundary(integ, rootsfound, bnd_cond_states);   /*************EVEN IF NO ROOT IS RETURNED LRSLV MUST BE CALLED TO UPDATE BOOLS**********************/										
		if (need_to_reconfigure == 0){
			return 657;
		}
		if (need_to_reconfigure == 1){							/********HIGHLY UNLIKELY*******/					
			return 143; 
		}
	}

	if(IDA_ROOT_RETURN){
#ifdef IDA_BND_DEBUG
		CONSOLE_DEBUG("IDA reports root found!");
#endif
			/* Store the root index */
		rootsfound = ASC_NEW_ARRAY_CLEAR(int,enginedata->nbnds);
		if(IDA_SUCCESS != IDAGetRootInfo(ida_mem, rootsfound)) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to fetch boundary-crossing info");
			return 14;
		}

#ifdef IDA_BND_DEBUG
			/* write out the boundaries that were crossed */
		for(i = 0; i < enginedata->nbnds; i++) {
			if(rootsfound[i]) {
				relname = bnd_make_name(integ->system, enginedata->bndlist[i]);
				CONSOLE_DEBUG("Boundary '%s' crossed at time x = %f, direction %s"
					,relname,tret,(rootsfound>0?"UP":"DOWN")
				);
				ASC_FREE(relname);
			}
		}
#endif
		if(all_bnds_set == 0){
#ifdef IDA_BND_DEBUG
			CONSOLE_DEBUG("Unset bounds exist; evaluate them explicitly...");
#endif
			all_bnds_set = 1;
			for(i = 0; i < enginedata->nbnds; i++){
				if(bnd_not_set[i]){
					if(!rootsfound[i]){
						bnd_cond_states[i] = bndman_calc_satisfied(enginedata->bndlist[i]);
#ifdef IDA_BND_DEBUG
						relname = bnd_make_name(integ->system, enginedata->bndlist[i]);
						CONSOLE_DEBUG("Boundary '%s': bnd_cond_states[%d] = %d"
						,relname,i,bnd_cond_states[i]
						);
						ASC_FREE(relname);
#endif
					}else{
						all_bnds_set = 0;
					}
				}
			}
		}
#ifdef IDA_BND_DEBUG
		CONSOLE_DEBUG("Just 'after_root'...");
		for(i=0;i<enginedata->nbnds;++i){
			relname = bnd_make_name(integ->system, enginedata->bndlist[i]);
			CONSOLE_DEBUG("Boundary '%s': bnd_cond_states[%d] = %d",relname,i,bnd_cond_states[i]);
			ASC_FREE(relname);
		}
#endif
		need_to_reconfigure = ida_cross_boundary(integ, rootsfound, bnd_cond_states);
		if(need_to_reconfigure == 1){
			updateflag = ida_bnd_update_relist(integ);
		}
		if(need_to_reconfigure == 1 && updateflag == 0){
			if(!first_run){		
			

				/************************EVENT PROCESSING MUST HAPPEN HERE**********************/

			}


			return 143;
		}	

		if(need_to_reconfigure == -1) {
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Analysis after the boundary failed.");
			return 1;
		}
		if(need_to_reconfigure == 0){
			if (ida_bnd_update_relist(integ) != 0) {
				/* system not square, failure */
				return 1;
			}else{
				return 657;				/**********CASE WHERE ROOTS FOUND BUT NO NEED TO UPDATE**************/
			}
		}
	}
}

	











int ida_cross_boundary(IntegratorSystem *integ, int *rootsfound,
	int *bnd_cond_states) {
	IntegratorIdaData *enginedata;
	slv_status_t status;
	struct bnd_boundary *bnd;
	int i, num_bnds;
	/* Flag the crossed boundary and update bnd_cond_states */
	enginedata = integ->enginedata;
	num_bnds = enginedata->nbnds;
	for (i = 0; i < num_bnds; i++) {
		if (rootsfound[i]) {
			integrator_output_write(integ);
			bnd = enginedata->bndlist[i];
  			bnd_set_ida_crossed(bnd, 1);
	/* Flag boundary for change, update bnd_cond_state */
			if (bnd_cond_states[i] == 0) {
				bnd_set_ida_value(bnd, 1);
				bnd_cond_states[i] = 1;
			}else{
				bnd_set_ida_value(bnd, 0);
				bnd_cond_states[i] = 0;
			}
		break;
		}
	}
 	/* solve the logical relations in the model, if possible */
	slv_presolve(integ->system);
	slv_solve(integ->system);
	/* Check for convergence */
	slv_get_status(integ->system, &status);
	if (!status.converged) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in logical solver.");
		return -1;
	}
 
	/* Reset the boundary flag */
	bnd_set_ida_crossed(bnd, 0);
 	/* update the main system if required */
	if (some_dis_vars_changed(integ->system)) {
		ida_bnd_reanalyse(integ);
		return 1;
	}else{
        /* Boundary crossing that has no effect on system */
	return 0;
     }
}












int ida_bnd_update_relist(IntegratorSystem *integ){
	IntegratorIdaData *enginedata;
	struct rel_relation **rels;
	int i,j,n_solverrels,n_active_rels;

	enginedata = integrator_ida_enginedata(integ);

	n_solverrels = slv_get_num_solvers_rels(integ->system);
	n_active_rels = slv_count_solvers_rels(integ->system, &integrator_ida_rel);
	rels = slv_get_solvers_rel_list(integ->system);

	if(enginedata->rellist != NULL){
		ASC_FREE(enginedata->rellist);
		enginedata->rellist = NULL;
		enginedata->rellist = ASC_NEW_ARRAY(struct rel_relation *, n_active_rels);
	}

	j = 0;
	for (i = 0; i < n_solverrels; ++i) {
		if (rel_apply_filter(rels[i], &integrator_ida_rel)) {
#ifdef IDA_BND_DEBUG
			char *relname;
			relname = rel_make_name(integ->system, rels[i]);
			CONSOLE_DEBUG("rel '%s': 0x%x", relname, rel_flags(rels[i]));
			ASC_FREE(relname);
#endif
			enginedata->rellist[j++] = rels[i];
		}
	}
	asc_assert(j == n_active_rels);
	enginedata->nrels = n_active_rels;

	if (enginedata->nrels != integ->n_y) {
		ERROR_REPORTER_HERE(ASC_USER_ERROR
				,"Integration problem is not square (%d active rels, %d vars)"
				,n_active_rels, integ->n_y
		);
		return 1; /* failure */
	}

	return 0;
}