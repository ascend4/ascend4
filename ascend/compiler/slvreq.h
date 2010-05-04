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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
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

#include <ascend/utilities/ascConfig.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/value_type.h>

/*
	type definitions for the solver request functions. functions of these
	types would be provided by the user interface.
*/

typedef int SlvReqSetSolverFn(const char *solvername, void *user_data);
typedef int SlvReqSetOptionFn(const char *optionname, struct value_t *val, void *user_data);
typedef int SlvReqDoSolveFn(struct Instance *instance, void *user_data);

typedef struct SlvReqHooks_struct{
	SlvReqSetSolverFn *set_solver_fn;
	SlvReqSetOptionFn *set_option_fn;
	SlvReqDoSolveFn *do_solve_fn;
	void *user_data;
} SlvReqHooks;

/**
	Store hook functions in the simulation instance.
*/
ASC_DLLSPEC int slvreq_assign_hooks(struct Instance *siminst
		, SlvReqSetSolverFn *set_solver_fn
		, SlvReqSetOptionFn *set_option_fn
		, SlvReqDoSolveFn *do_solve_fn
		, void *user_data
);

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

#endif /* ASC_SLVREQ_H */

