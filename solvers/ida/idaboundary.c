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

#define IDA_BND_DEBUG
/*
 *
 *
 * Check to see if and of the system discrete variables have changed.
 *
 * @return 1 if any of the values have changed (does not necessarily mean
 * 			 the system needs to be reconfigured)
 */
int some_dis_vars_changed(slv_system_t sys) {
	struct dis_discrete **dvlist, *cur_dis;
	int numDVs, i, ret;
	char *dis_name;

	dvlist = slv_get_solvers_dvar_list(sys);
	numDVs = slv_get_num_solvers_dvars(sys);

	ret = 0;
	for (i = 0; i < numDVs; i++) {
		cur_dis = dvlist[i];


#ifdef IDA_BND_DEBUG
		dis_name = dis_make_name(sys, cur_dis);
		CONSOLE_DEBUG("Boundary %s index, current, prev = %d, %d, %d ", dis_name,
				i, dis_value(cur_dis), dis_previous_value(cur_dis));
#endif

		if ((dis_kind(cur_dis) == e_dis_boolean_t) && dis_inwhen(cur_dis)) {
			if (dis_value(cur_dis) != dis_previous_value(cur_dis)) {
				ret = 1;
			}
		}
	}

	return ret;

}

void ida_setup_lrslv(IntegratorSystem *integ) {
	slv_parameters_t parameters;
	slv_status_t status;
	int i, num_params, slv_index;
	char *pname;

	/* Setup the logical solver */
	slv_index = slv_lookup_client("LRSlv");
	if (slv_select_solver(integ->system, slv_index) == -1) {
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error attempting to load LRSlv");
	}

#ifdef IDA_BND_DEBUG
	CONSOLE_DEBUG("Solver selected is '%s'",slv_solver_name
			(slv_get_selected_solver(integ->system)));
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

	/* solve the initial logical states */
		slv_presolve(integ->system);
		slv_solve(integ->system);

		/* Check for convergence */
		slv_get_status(integ->system, &status);
		if (!status.converged) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in logical solver at"
					"intialisation");
		}

		if(some_dis_vars_changed(integ->system)) {
			ida_bnd_reanalyse(integ);
		}
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


	integrator_ida_analyse(integ);

	return 0;
}

int ida_bnd_update_relist(IntegratorSystem *integ){
	IntegratorIdaData *enginedata;
	struct rel_relation **rels;
	char *relname;
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

/*
 * Uses LRSlv to check if any of the logical conditions in the model need
 * updating after a boundary crossing. If so, the model is reconfigured and
 * new initial conditions are determined at the point of the crossing.
 *
 * @return 1 if the crossing causes a change in the system
 */

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
			} else {
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
	} else {
		/* Boundary crossing that has no effect on system */
		return 0;
	}

}
