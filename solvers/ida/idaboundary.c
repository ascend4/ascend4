#include "idaboundary.h"

int ida_cross_boundary(IntegratorSystem *integ, int *rootsfound){

	/* solve the logical relations in the model, if possible */

	slv_set_client_token(server,token[LOGICAL_SOLVER]);
	slv_set_solver_index(server,solver_index[LOGICAL_SOLVER]);
	slv_presolve(server);
	slv_solve(server);

	slv_get_status(server,&status);
	sys->s.converged  = status.converged;
	if(!sys->s.converged ) {
	  unsuccessful = update_unsuccessful(sys,&status);
	  if(unsuccessful) {
		sys->s.ready_to_solve = !unsuccessful;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in logical solver.");
		slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
		slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
		gl_destroy(disvars);
		disvars = NULL;
		iteration_ends(sys);
		return 4;
	  }
	}

	/* update the main system as required */
	if(some_dis_vars_changed(server,asys) ) {
		reanalyze_solver_lists(server);
		update_relations_residuals(server);
		system_was_reanalyzed = 1;

		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Iterating with Optimizer...");
		slv_presolve(server);
		slv_get_status(server,&status);
		update_real_status(&(sys->s),&status,0);
		if(sys->s.cost) {
			destroy_array(sys->s.cost);
		}
		sys->s.cost =
			create_zero_array(sys->s.costsize,struct slv_block_cost);
	
		reset_cost(sys->s.cost,sys->s.costsize);
	}

	/* the above code was basically lifted out of the CMSlv solver code. It
	is COMPLETE RUBBISH at this stage but hopefully will give us some clues
	about what is needed here. */

}
