#include <ascend/utilities/config.h>
#include "slvreq.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include "instance_types.h"
#include "instquery.h"

int slvreq_assign_hooks(struct Instance *siminst, const SlvReqHooks *hooks){
	/* check that it's the right kind */
	assert(InstanceKind(siminst)==SIM_INST);
	assert(hooks != NULL);

	((struct SimulationInstance *)siminst)->slvreq_hooks = *hooks;

	return 0;
}


ASC_DLLSPEC void slvreq_destroy_hooks(struct Instance *inst){
	/* FIXME check its the right kind */
	struct Instance *sim = FindSimulationInstance(inst);
	assert(sim!=NULL);
	((struct SimulationInstance *)sim)->slvreq_hooks = (SlvReqHooks)SLVREQ_HOOKS_EMPTY;
}

ASC_DLLSPEC void slvreq_sim_destroy_hooks(struct Instance *sim){
	assert(InstanceKind(sim)==SIM_INST);
	((struct SimulationInstance *)sim)->slvreq_hooks = (SlvReqHooks)SLVREQ_HOOKS_EMPTY;
}

int slvreq_set_solver(struct Instance *inst, const char *solvername){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = &((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->set_solver_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No SOLVER hook set");
		return -1;
	}
	/*CONSOLE_DEBUG("Setting solver to '%s'",solvername);*/
	return (*(hooks->set_solver_fn))(solvername, hooks->user_data);
}

int slvreq_set_option(struct Instance *inst, const char *optionname, struct value_t *val){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = &((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->set_option_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No OPTION hook set");
		return -1;
	}

	/*CONSOLE_DEBUG("Setting option '%s' to value of type %d",optionname,ValueKind(val));*/
	return (*(hooks->set_option_fn))(optionname, val, hooks->user_data);
}

int slvreq_do_solve(struct Instance *inst){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = &((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->do_solve_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No SOLVE hook set");
		return -1;
	}

	return (*(hooks->do_solve_fn))(inst, hooks->user_data);
}

int slvreq_do_study(struct Instance *inst, const SlvReqStudyRequest *request){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = &((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->do_study_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No STUDY hook set");
		return -1;
	}

	return (*(hooks->do_study_fn))(request, hooks->user_data);
}

int slvreq_delete_system(struct Instance *inst){
	struct Instance *sim = FindSimulationInstance(inst);
	SlvReqHooks *hooks = &((struct SimulationInstance *)sim)->slvreq_hooks;
	if(hooks==NULL || hooks->delete_system_fn==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"No DELETE SYSTEM hook set");
		return -1;
	}

	return (*(hooks->delete_system_fn))(hooks->user_data);
}
