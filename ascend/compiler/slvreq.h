/*	ASCEND modelling environment
	Copyright (C) 2009 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Routines in this file provide a way for libascend to request its 
	controlling interface (by it a GUI, a scripting environment, etc) to
	assign a solver to the current simulation, and to set corresponding
	solver parameters.
*//*
	by John Pye
	2009
*/

#ifndef ASC_SLVREQ_H
#define ASC_SLVREQ_H

#include <ascend/general/platform.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/value_type.h>

/**	@addtogroup compiler_proc Compiler Methods
	@{
*/

/*
	type definitions for the solver request functions. functions of these
	types would be provided by the user interface.
*/

typedef int SlvReqSetSolverFn(const char *solvername, void *user_data);
typedef int SlvReqSetIntegratorFn(const char *integratorname, void *user_data);
typedef int SlvReqSetOptionFn(const char *optionname, struct value_t *val, void *user_data);
typedef int SlvReqDoSolveFn(struct Instance *instance, void *user_data);
typedef int SlvReqDeleteSystemFn(void *user_data);

typedef struct SlvReqObserveRequest_struct{
	unsigned long n_observed;
	struct Instance **observed;
	const char *name;
} SlvReqObserveRequest;

typedef int SlvReqDoObserveFn(const SlvReqObserveRequest *request, void *user_data);

enum SlvReqStudyMode{
	SLVREQ_STUDY_NONE = 0,
	SLVREQ_STUDY_STEPS,
	SLVREQ_STUDY_STEP,
	SLVREQ_STUDY_RATIO
};

enum SlvReqStudyDistribution{
	SLVREQ_STUDY_DIST_DEFAULT = 0,
	SLVREQ_STUDY_DIST_LINEAR,
	SLVREQ_STUDY_DIST_LOG
};

typedef struct SlvReqStudyRequest_struct{
	unsigned long n_observed;
	struct Instance **observed;
	struct Instance *vary;
	struct value_t lower;
	struct value_t upper;
	struct value_t value;
	long steps;
	enum SlvReqStudyMode mode;
	enum SlvReqStudyDistribution distribution;
	const char *run_method;
	unsigned int now;
	const char *filename;
} SlvReqStudyRequest;

typedef int SlvReqDoStudyFn(const SlvReqStudyRequest *request, void *user_data);

typedef struct SlvReqIntegrateRequest_struct{
	struct value_t start;
	struct value_t stop;
	long steps;
} SlvReqIntegrateRequest;

typedef int SlvReqDoIntegrateFn(const SlvReqIntegrateRequest *request, void *user_data);

typedef struct SlvReqHooks_struct{
	SlvReqSetSolverFn *set_solver_fn;
	SlvReqSetIntegratorFn *set_integrator_fn;
	SlvReqSetOptionFn *set_option_fn;
	SlvReqDoSolveFn *do_solve_fn;
	SlvReqDoObserveFn *do_observe_fn;
	SlvReqDoStudyFn *do_study_fn;
	SlvReqDoIntegrateFn *do_integrate_fn;
	SlvReqDeleteSystemFn *delete_system_fn;
	void *user_data;
} SlvReqHooks;

#define SLVREQ_HOOKS_EMPTY {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}

/**
	Store hook functions in the simulation instance.
*/
ASC_DLLSPEC int slvreq_assign_hooks(struct Instance *siminst, const SlvReqHooks *hooks);

/**
	Free the little bit of memory where slvreq hooks are stored, if
	necessary. The function will find its way up to the SimulationInstance
	by navigating the instance tree, and will free the slvreq_hooks once there.
	@param inst any instance within the instance tree.
*/
ASC_DLLSPEC void slvreq_destroy_hooks(struct Instance *inst);

/**
	Free the little bit of memory where slvreq hooks are stored, if
	necessary. This function expects to be pass the siminst, rather
	than just any instance within the tree.
	@param siminst pointer to the SimulationInstance object.
*/
ASC_DLLSPEC void slvreq_sim_destroy_hooks(struct Instance *siminst);

#define SLVREQ_NOT_IMPLEMENTED 8

#define SLVREQ_SOLVER_HOOK_NOT_SET -1
#define SLVREQ_UNKNOWN_SOLVER 1

/**
	Request a particular solver be assigned to the problem. It is left
	entirely up to the controlling interface to determine whether the request
	can be met (whether the solver really exists, whether it is eligible, etc.

	@param hooks structure containing the hook functions, as provided by the
		user interface, or NULL 
	@param solvername the solver name, as a string
	@param user_data a pointer to a data structure that the user interface
		may have registered earlier. Not yet implemented.

	@return 0 on success: -1 if no hooks set, else retval of hook fn, which 
	should be 1 if the solver name is not known.
*/
int slvreq_set_solver(struct Instance *simroot, const char *solvername);

#define SLVREQ_INTEGRATOR_HOOK_NOT_SET -1
#define SLVREQ_UNKNOWN_INTEGRATOR 1
int slvreq_set_integrator(struct Instance *inst, const char *integratorname);

#define SLVREQ_OPTION_HOOK_NOT_SET -1
#define SLVREQ_OPTIONS_UNAVAILABLE 1
#define SLVREQ_INVALID_OPTION_NAME 2
#define SLVREQ_WRONG_OPTION_VALUE_TYPE 3

/**
	Request to set an string-valued (solver or other) option. We propose to let the
	user interface query the solver to work out what the actual type of the
	parameter should be, and cast accordingly.

	@param optionname the name of the option, should be somehow understandable
	to the GUI
	@param val value to be assigned to the option; the GUI will have to know
	how to parse it.
	@return 0 on success: -1 if no hooks set, else retval of hook fn, which should
	be as follows: 1 if options are not yet assignable (no solver assigned),
	2 if the optionname is invalid, 3 if val is not of the value type expected 
	for the named option (or can not be or has not been converted).
*/
int slvreq_set_option(struct Instance *inst, const char *optionname, struct value_t *val);

#define SLVREQ_SOLVE_HOOK_NOT_SET -1
#define SLVREQ_NO_SOLVER_SELECTED 1
#define SLVREQ_PRESOLVE_FAIL 2
#define SLVREQ_SOLVE_FAIL 3
/**
	Request the user interface to intitiate the solver on the provided instance.
	How the user interface chooses to do that is left entirely up to the
	designed of the user interface; this is just a hook to allow a MODEL to
	partially automate itself.
	
	@param inst the instance to be solved
	@param user_data a pointer to a data structure that the user interface may
	have registered earlier, in a call to 'slvreq_assign_hooks'.
	
	@return 0 on success: -1 if no hooks set, else retval of hook fn, which
	must be 1 if the no solver has yet been assigned, 2 if the presolve fails, 3 if
	the normal solver iteration fails (in which case the user interface should
	have provided feedback in other ways).
*/
int slvreq_do_solve(struct Instance *inst);

#define SLVREQ_OBSERVE_HOOK_NOT_SET -1
#define SLVREQ_OBSERVE_INVALID_REQUEST 1
int slvreq_do_observe(struct Instance *inst, const SlvReqObserveRequest *request);

#define SLVREQ_STUDY_HOOK_NOT_SET -1
#define SLVREQ_STUDY_INVALID_REQUEST 1
#define SLVREQ_STUDY_IO_ERROR 2
/**
	Request the user interface to execute or surface a STUDY request.
	The interface may choose to run immediately, open a dialog, or write
	results to a file/console according to its own capabilities.
*/
int slvreq_do_study(struct Instance *inst, const SlvReqStudyRequest *request);

#define SLVREQ_INTEGRATE_HOOK_NOT_SET -1
#define SLVREQ_INTEGRATE_INVALID_REQUEST 1
#define SLVREQ_NO_INTEGRATOR_SELECTED 2
#define SLVREQ_INTEGRATE_FAIL 3
int slvreq_do_integrate(struct Instance *inst, const SlvReqIntegrateRequest *request);

#define SLVREQ_DELETE_HOOK_NOT_SET -1
int slvreq_delete_system(struct Instance *inst);

#endif /* ASC_SLVREQ_H */
