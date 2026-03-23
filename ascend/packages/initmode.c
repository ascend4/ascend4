/*	ASCEND modelling environment
	Copyright (C) 2026

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.
*/

#include <ascend/packages/initmode.h>

#include <ascend/compiler/instquery.h>
#include <ascend/compiler/simstatus.h>
#include <ascend/compiler/slvreq.h>
#include <ascend/system/system.h>
#include <ascend/utilities/error.h>

static struct Instance *
initmode_get_target(struct Instance *root, struct gl_list_t *arglist){
	if(arglist == NULL
		|| gl_length(arglist) == 0L
		|| gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 1
		|| gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL
	){
		return root;
	}
	return (struct Instance *)gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1);
}

static int
initmode_set(struct Instance *root, struct gl_list_t *arglist, SystemBuildMode mode){
	struct Instance *target = initmode_get_target(root, arglist);
	struct Instance *siminst;
	struct Instance *simroot;
	int res;

	if(target == NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Initialization mode change requires a valid instance context");
		return 1;
	}

	siminst = FindSimulationInstance(target);
	if(siminst == NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Initialization mode change requires a simulation context");
		return 1;
	}

	simroot = GetSimulationRoot(siminst);
	if(simroot == NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unable to locate simulation root for initialization mode change");
		return 1;
	}

	system_set_build_mode(simroot, mode);
	asc_simstatus_mark_dirty(simroot);

	res = slvreq_delete_system(simroot);
	if(res == 0 || res == SLVREQ_DELETE_HOOK_NOT_SET || res == SLVREQ_NOT_IMPLEMENTED){
		return 0;
	}

	ERROR_REPORTER_HERE(ASC_USER_ERROR,"Failed to invalidate the current solver system after changing initialization mode");
	return 1;
}

int
Asc_EnterInitialMode(struct Instance *root, struct gl_list_t *arglist, void *userdata){
	(void)userdata;
	return initmode_set(root, arglist, SYSTEM_BUILD_INITIAL);
}

int
Asc_LeaveInitialMode(struct Instance *root, struct gl_list_t *arglist, void *userdata){
	(void)userdata;
	return initmode_set(root, arglist, SYSTEM_BUILD_NORMAL);
}
