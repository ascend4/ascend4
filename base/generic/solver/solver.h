/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*//** @file
	Encapsulation of the NLA solver interface, separate from the
	system definition and access routines.
*//*
	created from bits of slv.c, slv_stdcalls.c, etc.
	John Pye, 2007.
*/

#ifndef ASC_SOLVER_H
#define ASC_SOLVER_H

#include <utilities/ascConfig.h>
#include <system/slv_client.h>

/*-----------------------------------------------------------------------
	SOLVER HOOKS AND REGISTRATION DATA STRUCTURE
*/

/** @todo We will explain all these later in this file someday soon. */
typedef SlvClientToken (SlvClientCreateF) (slv_system_t,int *);
typedef int (SlvClientDestroyF) (slv_system_t,SlvClientToken);
typedef int (SlvClientEligibleF) (slv_system_t);
typedef int32 (SlvGetDefParamsF) (slv_system_t,SlvClientToken,slv_parameters_t *);
typedef void (SlvGetParamsF) (slv_system_t, SlvClientToken, slv_parameters_t *);
typedef void (SlvSetParamsF) (slv_system_t, SlvClientToken, slv_parameters_t *);
typedef int (SlvGetStatusF) (slv_system_t, SlvClientToken, slv_status_t *);
typedef linsolqr_system_t (SlvGetLinSysF)(slv_system_t, SlvClientToken);
typedef mtx_matrix_t (SlvGetSysMtxF)(slv_system_t, SlvClientToken);
typedef void (SlvDumpInfoF)(slv_system_t, SlvClientToken,int);
typedef int (SlvSolveF)(slv_system_t, SlvClientToken);

/** Registration information for a solver.
	@TODO Complete documentation of slv_registration_data members.
*/
typedef struct slv_registration_data {
  int number;
	 /**< we set number AFTER the client registration returns 0.
		client sets all the rest, starting with a symbolic name */

  const char *name;                   /**< symbolic name for solver (required). */
  /*
	Required functions
  */
  SlvClientCreateF    *ccreate;       /**<  (required) */
  SlvClientDestroyF   *cdestroy;      /**<  (required) */
  SlvClientEligibleF  *celigible;     /**<  (required) */
  SlvGetDefParamsF    *getdefparam;   /**< Function that will create default solver-parameter structure (required) */
  SlvGetParamsF       *get_parameters;/**<  (required) */
  SlvSetParamsF       *setparam;      /**<  (required) */
  SlvGetStatusF       *getstatus;     /**<  (required) */
  SlvSolveF           *solve;         /**<  (required) */
  /*
	Functions we really want, but can live without if your solver is old
	and klunky. Your solver may not 'look good' in an interactive environment,
	but then those nasty batch codes seldom do anyway.
	Redesign your bloody batch code.
  */
  SlvSolveF           *presolve;      /**<  (desired) */
  SlvSolveF           *iterate;       /**<  (desired) */
  SlvSolveF           *resolve;       /**<  (desired) */
  /**
	Strictly Optional Functions
  */
  SlvGetLinSysF       *getlinsys;     /**<  (optional) */
  SlvGetSysMtxF       *get_sys_mtx;   /**<  (optional) */
  SlvDumpInfoF        *dumpinternals; /**<  (optional) */
} SlvFunctionsT;


typedef int SolverRegisterFn(void);
/**<
	Registration function for solvers. Should add solver to the list of
	available solvers for NLA/LP/NLP/MINLP problems.
	@return 0 on success
*/

/*------------------------------------------------------------------------------
  REGISTRATION SOLVERS AND QUERYING OF SOLVER LIST
*/

ASC_DLLSPEC int solver_register(const SlvFunctionsT *solver);

ASC_DLLSPEC int SlvRegisterStandardClients(void);
/**<
	Attempts to register solvers slv0 through (as of 6/96) slv7.

	The solvers registered here are those linked at build time of the
	ascend binary. See slv_client.h for registering dynamically loaded
	solvers.

	@return number of solvers registered successfully
*/

ASC_DLLSPEC const struct gl_list_t *solver_get_engines();
/**<
	Get the list of engines. For the purpose of output to the GUI.
*/

ASC_DLLSPEC const SlvFunctionsT *solver_engine(const int number);
/**<
	@param number solver ID number to check for
	@return SlvFunctions for this solver, if the number is valid, else NULL
*/

ASC_DLLSPEC const SlvFunctionsT *solver_engine_named(const char *name);
/**<
	@param name name of solver to check for
	@return SlvFunctions for this solver, if the name is found, else NULL
*/

/*------------------------------------------------------------------------------
  HOOKS INTO SOLVER ENGINE, CALLED BY ASCEND
*/

ASC_DLLSPEC int slv_get_status(slv_system_t sys, slv_status_t *status);
/**<
	Copies the current system status into the given structure.
	@return 0 on success, or nonzero if there is a problem determining status.
*/

ASC_DLLSPEC linsolqr_system_t slv_get_linsolqr_sys(slv_system_t sys);
/**<
	Returns the linsolqr system used, or NULL if none.
	@deprecated { THIS CALL SHOULD GO AWAY }
*/

ASC_DLLSPEC mtx_matrix_t slv_get_sys_mtx(slv_system_t sys);
/**<
	Returns the mtx used, or NULL if none. The user should check.

	@deprecated {THIS CALL SHOULD GO AWAY}

	@NOTE this function is referenced by the C++ api (Simulation::getMatrix).
	So don't don't vanish it! -- JP 20070113 @ENDNOTE
 **/

ASC_DLLSPEC void slv_dump_internals(slv_system_t sys, int level);
/**<
	Will spew whatever the solver interface developer feels like to
	stderr.  Larger values of level will give more detailed information,
	we hope.  No specification is made with regard to what the
	information will be.  returns -1 if solver gutless.  This is provided
	principally to facilitate debugging a little.

	@TODO fix dubious documentation (return type is void...)
*/

ASC_DLLSPEC int slv_presolve(slv_system_t sys);
/**<
	Prepares the system for solving.  This must be called before the
	system is solved, but after everything about the system is set up
	(i.e. variables and relations cannot be changed IN ANY WAY, objective
	function cannot be modified, boundaries cannot be modified, or even
	repermuted, and a new solver cannot be selected: some parameters may
	be modified, they will be marked as such).  The system essentially
	becomes "read-only".  If anything is modified after slv_presolve was
	called, slv_presolve must be called again before solving (EXCEPTIONS:
	slv_resolve may do for a certain restricted class of changes). @par

	It is at this point that the variable list is created if it does not
	already exist and the newly created variables are indexed in the
	order they end up.  The relation list is indexed as well in the order
	it is received. @par

	Among other things, this function will perform structural analysis
	so that structural analysis flags in the status will be accurate.

	@return 0 on success, 1 if errors occurred (they will be output via ERROR_REPORTER)
*/

ASC_DLLSPEC int slv_resolve(slv_system_t sys);
/**<
	This function re-prepares the system for solving.  This function may
	be used instead of slv_presolve, provided the system was partially
	or completely solved before, and then the only changes to the system
	since are as follows:

	@li	 any parameter except "partition".
	@li  variable values.
	@li  variable nominal values.
	@li  variable bounds.

	In particular, the following changes are NOT allowed:

	@li  variable fixed flag.
	@li  relation included flag.
	@li  variable/relation list contents, including order.  Also, the
		variable/relation indices must continue to be consistent with
		the list.
	@li  definition of relations, objective function, and boundaries:
		including structural rearrangement on relations, although any
		expression may be simplified.

	This function is considerably more efficient when it is usable.

	@return 0 on success
*/

ASC_DLLSPEC int slv_iterate(slv_system_t sys);
/**<
	Performs one iteration toward the ultimate solution (or
	failure thereof) of the system.  The user can obtain information
	from the status and from the variables and relations themselves
	(some care should be taken in examining the residuals of relations;
	they may not be up to date).  The user may not modify the system in
	any way between iterations (i.e. you may look, but don't touch: see
	slv_presolve()).  See also slv_solve().

	@return 0 on success
*/

ASC_DLLSPEC int slv_solve(slv_system_t sys);
/**<
	Attempts to solve the entire system in one shot (i.e.
	performs as many iterations as needed or allowed).  For some solvers,
	slv_iterate() and slv_solve() may mean the same thing.

	@return 0 on success
*/


ASC_DLLSPEC int slv_eligible_solver(slv_system_t sys);
/**<
	Determines whether or not the current solver.
	is capable of solving the given system as it is currently set up
	(e.g. some solvers cannot do optimization, or inequalities, etc.).
	The system must be set up first before calling this function, or the
	return value may be misleading. @par

	The solver in question will be asked to pass judgement on the
	data in the slv_system_t wrt the solver being useful.
	If no solver is registered, this returns FALSE.
*/

ASC_DLLSPEC int slv_select_solver(slv_system_t sys, int solver);
/**<
	Sets the given solver to be the current solver
	for the system.  The intelligence or stupidity of this move is not
	investigated. If the system has already has a solver selected and
	it is not the same solver, the data structures of the old selection
	will be blown away.

	@return number of solver actually selected or -1 on failure
*/

ASC_DLLSPEC int slv_get_selected_solver(slv_system_t sys);
/**<
	Returns the current solver number for a system.
*/

ASC_DLLSPEC int slv_switch_solver(slv_system_t sys, int solver);
/**<
	Sets the given solver to be the current solver for the system.
	Return value is number of solver actually selected.
	If failure, return is -1;
*/

ASC_DLLSPEC int32 slv_get_default_parameters(int32 sindex, slv_parameters_t *parameters);
/**< @TODO needs commenting, KHACK */

ASC_DLLSPEC void slv_get_parameters(slv_system_t sys, slv_parameters_t *parameters);
/**<
	Copies the current system parameters to the given structure.

	Do not confuse these parameters [algorithm control variables]
	with the parameter list which is a list of pointers to var_variable.
*/
ASC_DLLSPEC void slv_set_parameters(slv_system_t sys, slv_parameters_t *parameters);
/**<
	Sets the current system parameters to the values contained
	in the given structure.  It is recommended that one
	gets the parameters first, before one modifies them and sets them,
	especially if not all of the parameters are to be modified (and you
	never know when that might suddenly become true because a new
	parameter was added to the structure).  Parameters will only be
	accepted by an engine if they came from that engine, so fetching
	before setting is not only a good idea, it's the law (gas engines
	don't run on diesel very well...). @par

	Do not confuse these parameters [algorithm control variables]
	with the parameter list which is a list of pointers to var_variable.
*/

//------------------------------------------------------------------------------

ASC_DLLSPEC SlvClientToken slv_get_client_token(slv_system_t sys);
/**<
	Returns  the client token of the system_t.
 */

ASC_DLLSPEC void slv_set_client_token(slv_system_t sys, SlvClientToken ct);
/**<
	Sets  the client token of the system_t.
	@deprecated
*/

ASC_DLLSPEC void slv_set_solver_index(slv_system_t sys, int sindex);
/**<
	Sets the solver index of the slv_system_t.
	@deprecated
*/

ASC_DLLSPEC void slv_destroy_client(slv_system_t sys);
/**<
	Destroy the client token of slv_system_t. It does not deallocate
	the allocated data space of sys
*/

ASC_DLLSPEC boolean slv_change_basis(slv_system_t,int32,mtx_range_t *);
/**<
	Move var (given by index #) to the unassigned region (far right)
	of the solver matrix if possible. returns FALSE if impossible
	because structural infeasibility would occur or because solver selected
	won't do it.

	@deprecated THIS CALL SHOULD GO AWAY
*/

extern void slv_print_output(FILE *fp, slv_system_t sys);
/**<
	Start of some report generation routines. For now just prints out
	the variable values and residuals at the moment.

	@TODO make more general in the future.
*/

ASC_DLLSPEC const char*slv_solver_name(int sindex);
/**<
	@param index index of the solver in question (the index depends on the order in which the solvers have been registered)
	@return name of the solver

	There may in general be more than one solver.  The solvers will be
	numbered [0..slv_number_of_solvers).  Not all the solvers may
	be present in a given installation of ASCEND as some are proprietary
	(MINOS, for example). @par

	Solvers not yet registered will not have names. Each registered
	client must have a unique name if user interfaces are to be happy,
	though we suppose an interface could make a unique identifier out
	of name-number pair.
*/

#endif



