#include "simstatus.h"

#include <ascend/general/ascMalloc.h>
#include "instquery.h"
#include "instance_types.h"

struct asc_simstatus{
	int dirty;
	unsigned method_depth;
	struct Instance *last_solve_target;
};

#define SIMSTATUS_SIM(inst) ((struct SimulationInstance *)(inst))

static struct Instance *
simulation_status_get_siminst(struct Instance *inst){
	if(inst == NULL){
		return NULL;
	}
	if(InstanceKind(inst) == SIM_INST){
		return inst;
	}
	return FindSimulationInstance(inst);
}

static struct asc_simstatus *
simulation_status_get_entry(struct Instance *inst, int create){
	struct Instance *siminst = simulation_status_get_siminst(inst);
	struct asc_simstatus *entry;

	if(siminst == NULL){
		return NULL;
	}

	entry = SIMSTATUS_SIM(siminst)->simstatus;
	if(entry != NULL){
		return entry;
	}
	if(!create){
		return NULL;
	}
	entry = ASC_NEW(struct asc_simstatus);
	entry->dirty = 1;
	entry->method_depth = 0;
	entry->last_solve_target = NULL;
	SIMSTATUS_SIM(siminst)->simstatus = entry;
	return entry;
}

void
asc_simstatus_method_enter(struct Instance *inst){
	struct asc_simstatus *entry = simulation_status_get_entry(inst, 1);
	if(entry != NULL){
		++entry->method_depth;
	}
}

void
asc_simstatus_method_leave(struct Instance *inst){
	struct asc_simstatus *entry = simulation_status_get_entry(inst, 1);
	if(entry != NULL && entry->method_depth > 0){
		--entry->method_depth;
	}
}

void
asc_simstatus_mark_dirty(struct Instance *inst){
	struct asc_simstatus *entry = simulation_status_get_entry(inst, 1);
	if(entry != NULL){
		entry->dirty = 1;
	}
}

void
asc_simstatus_mark_clean(struct Instance *inst, struct Instance *target){
	struct asc_simstatus *entry = simulation_status_get_entry(inst, 1);
	if(entry != NULL){
		entry->dirty = 0;
		entry->last_solve_target = target;
	}
}

int
asc_simstatus_is_dirty(struct Instance *inst){
	struct asc_simstatus *entry = simulation_status_get_entry(inst, 0);
	return entry == NULL ? 1 : entry->dirty;
}

unsigned
asc_simstatus_method_depth(struct Instance *inst){
	struct asc_simstatus *entry = simulation_status_get_entry(inst, 0);
	return entry == NULL ? 0U : entry->method_depth;
}

struct Instance *
asc_simstatus_get_last_solve_target(struct Instance *inst){
	struct asc_simstatus *entry = simulation_status_get_entry(inst, 0);
	return entry == NULL ? NULL : entry->last_solve_target;
}

void
asc_simstatus_destroy(struct Instance *siminst){
	struct Instance *real_siminst = simulation_status_get_siminst(siminst);
	if(real_siminst == NULL){
		return;
	}
	if(SIMSTATUS_SIM(real_siminst)->simstatus != NULL){
		ASC_FREE(SIMSTATUS_SIM(real_siminst)->simstatus);
		SIMSTATUS_SIM(real_siminst)->simstatus = NULL;
	}
}
