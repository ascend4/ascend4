#include "ida.h"
#include "idalinear.h"
#include "idaanalyse.h"
#include "idatypes.h"
#include "idaprec.h"
#include "idacalc.h"
#include "idaio.h"
#include "idaboundary.h"
#include <stdio.h>

#include <ascend/general/platform.h>
#include <ascend/general/list.h>
#include <ascend/general/panic.h>

#include <ascend/solver/solver.h>

#include <ascend/system/slv_client.h>
#include <ascend/system/cond_config.h>
#include <ascend/system/discrete.h>
#include <ascend/system/slv_types.h>
#include <ascend/system/slv_common.h>
#include <ascend/system/logrel.h>
#include <ascend/system/rel.h>
#include <ascend/system/discrete.h>
#include <ascend/system/var.h>
#include <ascend/system/block.h>
#include <ascend/system/bndman.h>

#define IDA_BND_DEBUG

/**
 * Check to see if any of the system discrete variables have changed.
 *
 * @return 1 if any of the values have changed (does not necessarily mean
 * 			 the system needs to be reconfigured)
 */
int some_dis_vars_changed(slv_system_t sys) {
	struct dis_discrete **dvlist, *cur_dis;
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
				return 1;
			}
		}
	}
	CONSOLE_DEBUG("No boundary vars have changed");
	return 0;

}

static void ida_write_values(IntegratorSystem *integ) {
	struct var_variable **vl;
	struct dis_discrete **dvl;
	int32 c;
	vl = slv_get_solvers_var_list(integ->system);
	dvl = slv_get_solvers_dvar_list(integ->system);
	for (c = 0; vl[c] != NULL; c++)
		CONSOLE_DEBUG("Value of %s is %f", var_make_name(integ->system,vl[c]),var_value(vl[c]));
	for (c = 0; dvl[c] != NULL; c++)
		CONSOLE_DEBUG("Value of %s is %d", dis_make_name(integ->system,dvl[c]),dis_value(dvl[c]));
}

int ida_setup_lrslv(IntegratorSystem *integ) {
	CONSOLE_DEBUG("Running logical solver...");
	ida_log_solve(integ);
	if(some_dis_vars_changed(integ->system)){
		CONSOLE_DEBUG("Some discrete vars changed; reanalysing");
		return ida_bnd_reanalyse_cont(integ);
	}
	return 0;
}

int ida_bnd_reanalyse(IntegratorSystem *integ){

	if (integ->y_id != NULL) {
		ASC_FREE(integ->y_id);
		integ->y_id = NULL;
	}

	if (integ->obs_id != NULL){
		ASC_FREE(integ->obs_id);
		integ->obs_id = NULL;
	}
	if (integ->y != NULL) {
		ASC_FREE(integ->y);
		integ->y = NULL;
	}
	if (integ->ydot != NULL) {
		ASC_FREE(integ->ydot);
		integ->ydot = NULL;
	}
	if (integ->obs != NULL) {
		ASC_FREE(integ->obs);
		integ->obs = NULL;
	}

	integ->n_y = 0;


	return reanalyze_solver_lists(integ->system);
}

int ida_bnd_reanalyse_cont(IntegratorSystem *integ){
	CONSOLE_DEBUG("Clearing memory from previous calls...");
	if (integ->y_id != NULL) {
		ASC_FREE(integ->y_id);
		integ->y_id = NULL;
	}

	if (integ->obs_id != NULL){
		ASC_FREE(integ->obs_id);
		integ->obs_id = NULL;
	}
	if (integ->y != NULL) {
		ASC_FREE(integ->y);
		integ->y = NULL;
	}
	if (integ->ydot != NULL) {
		ASC_FREE(integ->ydot);
		integ->ydot = NULL;
	}
	if (integ->obs != NULL) {
		ASC_FREE(integ->obs);
		integ->obs = NULL;
	}

	integ->n_y = 0;

	return integrator_ida_analyse(integ);
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

N_Vector ida_bnd_new_zero_NV(long int vec_length){
	int i;
	N_Vector nv = N_VNew_Serial(vec_length);
	for(i= 0; i< vec_length; i++) {
		NV_Ith_S(nv,i) = 0.0;
	}

	return nv;
}


void ida_bnd_update_IC(IntegratorSystem *integ, realtype t0, N_Vector y0, N_Vector yp0) {
	/* First destroy since n_y may have changed */
	N_VDestroy_Serial(y0);
	N_VDestroy_Serial(yp0);
	/* retrieve new initial values from the system */
	t0 = integrator_get_t(integ);
	y0 = ida_bnd_new_zero_NV(integ->n_y);
	integrator_get_y(integ, NV_DATA_S(y0));

	yp0 = ida_bnd_new_zero_NV(integ->n_y);
	integrator_get_ydot(integ, NV_DATA_S(yp0));

#ifdef IDA_BND_DEBUG
	CONSOLE_DEBUG("BEFORE IC SOLVING:");
	CONSOLE_DEBUG("TIME: %f", t0);
	CONSOLE_DEBUG("Y");
	N_VPrint_Serial(y0);
	CONSOLE_DEBUG("Yp");
	N_VPrint_Serial(yp0);
	CONSOLE_DEBUG("Press any to continue...");
	getchar();
#endif

}

int ida_log_solve(IntegratorSystem *integ) {
	slv_parameters_t parameters;
	int num_params;
	char *pname;
	slv_status_t status;
	int i;
	if (slv_select_solver(integ->system, integrator_ida_enginedata(integ)->lrslv_ind) == -1) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error attempting to load LRSlv");
	}
#ifdef IDA_BND_DEBUG
	CONSOLE_DEBUG("Solver selected is '%s'"
		,slv_solver_name(slv_get_selected_solver(integ->system))
	);
#endif

	/* Flip LRSlv into ida mode */
	slv_get_parameters(integ->system, &parameters);
	num_params = parameters.num_parms;
	for (i = 0; i<num_params; i++) {
		pname = parameters.parms[i].name;
		if(strcmp(pname, "withida") == 0) {
			parameters.parms[i].info.b.value = 1;
		}
	}
	/* solve the logical relations in the model, if possible */
	slv_presolve(integ->system);
	slv_solve(integ->system);

	/* Check for convergence */
	slv_get_status(integ->system, &status);
	if (!status.converged) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in logical solver.");
		return 0;
#ifdef IDA_BND_DEBUG
	}else{
		CONSOLE_DEBUG("Converged");
#endif
	}
	return 1;
}

/*
 * Uses LRSlv to check if any of the logical conditions in the model need
 * updating after a boundary crossing. If so, the model is reconfigured and
 * new initial conditions are determined at the point of the crossing.
 *
 * @return 1 if the crossing causes a change in the system
 */

int ida_cross_boundary(IntegratorSystem *integ, int *rootsfound, int *bnd_cond_states) {

	IntegratorIdaData *enginedata;
	slv_status_t status;

	struct bnd_boundary *bnd = NULL;
	int i, num_bnds, num_dvars;

	struct dis_discrete **dvl;
	int32 c, *prevals = NULL;

	double *bnd_prev_eval;

	CONSOLE_DEBUG("Crossing boundary...");

	integrator_output_write(integ);
	integrator_output_write_obs(integ);
	integrator_output_write_event(integ);
	/*^^^ FIXME should we write the above event? It may not have crossed in correct direction...? */

	/* Flag the crossed boundary and update bnd_cond_states */
	enginedata = integ->enginedata;
	num_bnds = enginedata->nbnds;
	bnd_prev_eval = ASC_NEW_ARRAY(double,num_bnds);
	for(i = 0; i < num_bnds; i++) {
		/* get the value of the boundary condition before crossing */
		bnd_prev_eval[i] = bndman_real_eval(enginedata->bndlist[i]);
		if(rootsfound[i]){
					/* reminder: 'rootsfound[i] == 1 for UP or -1 for DOWN. */
			/* this boundary was one of the boundaries triggered */
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
				/* Boundary crossed twice in one direction. Very unlikey! */

				/* The aim of the following two lines is to set the value of 
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
				if(!ida_log_solve(integ)){
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
	if(!ida_log_solve(integ)) return -1;

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
	if(slv_get_num_solvers_bnds(integ->system)!=0) {
		while(some_dis_vars_changed(integ->system) && events_triggered)  {
			if(ida_bnd_reanalyse(integ)) {
				/* select QRSlv solver, and solve the system */
				if(slv_select_solver(integ->system, enginedata->qrslv_ind) == -1) {
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
				if (!ida_log_solve(integ)) return -1;

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
}

