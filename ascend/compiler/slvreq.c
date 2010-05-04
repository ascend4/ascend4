#include <ascend/utilities/config.h>
#include "slvreq.h"

#include <ascend/utilities/ascMalloc.h>
#include <ascend/utilities/ascPanic.h>
#include "instance_types.h"
#include "instquery.h"

int slvreq_assign_hooks(struct Instance *siminst
		, SlvReqSetSolverFn *set_solver_fn
		, SlvReqSetOptionFn *set_option_fn
		, SlvReqDoSolveFn *do_solve_fn
		, void *user_data
){
	/* check that it's the right kind */
	assert(InstanceKind(siminst)==SIM_INST);

	SlvReqHooks *h = ASC_NEW(SlvReqHooks);
	h->set_solver_fn = set_solver_fn;
	h->set_option_fn = set_option_fn;
	h->do_solve_fn = do_solve_fn;
	h->user_data = user_data;

	if(((struct SimulationInstance *)siminst)->slvreq_hooks){
		ASC_FREE(((struct SimulationInstance *)siminst)->slvreq_hooks);
	}
	((struct SimulationInstance *)siminst)->slvreq_hooks = h;

	return 0;
}


ASC_DLLSPEC void slvreq_destroy_hooks(struct Instance *inst){
	/* FIXME check its the right kind */
	struct Instance *sim = FindSimulationInstance(inst);
	assert(sim!=NULL);
	if(((struct SimulationInstance *)sim)->slvreq_hooks != NULL){
		ASC_FREE(((struct SimulationInstance *)sim)->slvreq_hooks);
		((struct SimulationInstance *)sim)->slvreq_hooks = NULL;
	}
}

ASC_DLLSPEC void slvreq_sim_destroy_hooks(struct Instance *sim){
	assert(InstanceKind(sim)==SIM_INST);
	if(((struct SimulationInstance *)sim)->slvreq_hooks != NULL){
		ASC_FREE(((struct SimulationInstance *)sim)->slvreq_hooks);
		((struct SimulationInstance *)sim)->slvreq_hooks = NULL;
	}
}

int slvreq_set_solver(struct Instance *inst, const char *solvername){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = ((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->set_solver_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No SOLVER hook set");
		return -1;
	}
	/*CONSOLE_DEBUG("Setting solver to '%s'",solvername);*/
	return (*(hooks->set_solver_fn))(solvername, hooks->user_data);
}

int slvreq_set_option(struct Instance *inst, const char *optionname, struct value_t *val){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = ((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->set_option_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No OPTION hook set");
		return -1;
	}

	/*CONSOLE_DEBUG("Setting option '%s' to value of type %d",optionname,ValueKind(val));*/
	return (*(hooks->set_option_fn))(optionname, val, hooks->user_data);
}

int slvreq_do_solve(struct Instance *inst){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = ((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->do_solve_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No SOLVE hook set");
		return -1;
	}

	return (*(hooks->do_solve_fn))(inst, hooks->user_data);
}
