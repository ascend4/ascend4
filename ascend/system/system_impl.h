/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2005-2007 Carnegie-Mellon University

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*//** @defgroup system_impl System Internal implementation
	Implementation a the 'slv_system_t' class. This is a private header file
	to be used only by SLV *server* code. If you are writing a new solver, you
	SHOULD NOT use this header file.

	This structure is subject to change. Access it through the methods and
	macros defined in slv_client.h in order to avoid problems.

	Note that this file includes diffvars.h but NOT diffvars_impl.h. So,
	you don't get access to the derivative chains structs unless you explicitly
	ask for it by including diffvars_impl.h.
*//**
	Split from slv.c by John Pye -- Jan 2007
*/

#include "system.h"
#include "diffvars.h"
#include <ascend/solver/solver.h>

/**	@addtogroup system_impl
	@{
*/

/**
	System structure, referred to by pointer type slv_system_t. This
	structure is passed around by the Solver API as the basic representation of
	a 'flat' problem in ASCEND.

	@warning DO NOT access this structure directly; use the functions in solver.h instead. The details of this structure are intended to be concealed your solver's implementation code.

	@warning This structure is subject to change. Access it through the methods and macros defined in slv_client.h in order to avoid problems.
*/
struct system_structure {
	int solver; /**< ID number of the currently-connected solver -- JP */
	const SlvFunctionsT *internals; /**< Pointers to the current solver's implementation functions */

	int serial_id;
	/**< Through time, two systems may have the same pointer but never
		simultaneously. The serial_id provides a unique tag that will
		never repeat. Clients concerned with identity but not capable
		of tracking time must use the serial_id for checks. */

	SlvBackendToken instance;	/* should be void * in the most generic case */

	/* All solver handles.  sysI can't be dereferenced outside slvI.c
	* should be an array of pointers to arrays of the functions provided
	* by dynamically loaded clients, or at least by the client which this
	* system is currently supporting.
	*/

	SlvClientToken ct;
	/**< This is a pointer that the client returns on registration.
	* If it is not null, the registration was successful.
	* This token will be handed back to the client code on all calls
	* originating from here.
	*/

	dof_t dof;                    /* non linear blocks */
	dof_t logdof;                 /* logical blocks */

	/* In the following NULL terminated lists, note that snum and mnum
	* are the lengths of the arrays WITHOUT the NULL pointer at the end.
	* Note objs is a list of relations that are objectives
	* (e_maximize,e_minimize). this list will include the first included obj.
	*/

	/** real-valued variables */
	struct {
		int snum;			/* length of the solver list */
		int mnum;			/* length of the master list */
		struct var_variable **solver;
		struct var_variable **master;
		struct var_variable *buf;
	} vars;

	/** discrete-valued variabled (integers, booleans, enumerations) */
	struct {
		int snum;		        	/* length of the solver list */
		int mnum;			       /* length of the master list */
		struct dis_discrete **solver;
		struct dis_discrete **master;
		struct dis_discrete *buf;
		int bufnum;
	} dvars;

	/** real-valued relations */
	struct {
		int snum;			/* length of the solver list */
		int mnum;			/* length of the master list */
		struct rel_relation **solver;
		struct rel_relation **master;
		struct rel_relation *buf;
	} rels;

	/** objective relations (MAXIMIZE and MINIMIZE) */
	struct {
		int snum;
		int mnum;
		struct rel_relation **solver;
		struct rel_relation **master;
		struct rel_relation *buf;
	} objs;

	/** real-valued conditional relations @todo check this ?? */
	struct {
		int snum;			/* length of the solver list */
		int mnum;			/* length of the master list */
		struct rel_relation **solver;
		struct rel_relation **master;
		struct rel_relation *buf;
	} condrels;

	/** logical relations */
	struct {
		int snum;			/* length of the solver list */
		int mnum;			/* length of the master list */
		struct logrel_relation **solver;
		struct logrel_relation **master;
		struct logrel_relation *buf;
	} logrels;

	/** logical-valued conditional relations @todo check this*/
	struct {
	int snum;			/* length of the solver list */
	int mnum;			/* length of the master list */
	struct logrel_relation **solver;
	struct logrel_relation **master;
	struct logrel_relation *buf;
	} condlogrels;

	/** WHEN..CASE structures for turning on and off relations according to dvar values */
	struct {
		int snum;			/* length of the solver list */
		int mnum;			/* length of the master list */
		struct w_when **solver;
		struct w_when **master;
		struct w_when *buf;
		int bufnum;
	} whens;

	/** real-valued and logical-valued boundaries */
	struct {
		int snum;			/* length of the solver list */
		int mnum;			/* length of the master list */
		struct bnd_boundary **solver;
		struct bnd_boundary **master;
		struct bnd_boundary *buf;
		int bufnum;
	} bnds;

	/** real ATOM instance parameters */
	struct {
		int snum;
		int mnum;
		struct var_variable **solver;
		struct var_variable **master;
		struct var_variable *buf;
	} pars;

	/** Unattached real-valued variables */ 
	struct {
		int snum;
		int mnum;
		struct var_variable **solver;
		struct var_variable **master;
		struct var_variable *buf;
	} unattached;

	/** Unattached discrete-valued variables */
	struct {
		int snum;
		int mnum;
		struct dis_discrete **solver;
		struct dis_discrete **master;
		struct dis_discrete *buf;
	} disunatt;

	/**< derivative chains, if present (NULL if not present) */
	struct SolverDiffVarCollectionStruct *diffvars; 

	/* ----- the data that follows is for internal consumption only.--------- */

	/** external relations */
	struct {
		int num_extrels;
		struct ExtRelCache **erlist;
	} extrels;

	struct rel_relation *obj; /**< selected for optimization from list */
	struct var_variable *objvar; /**< selected for optimization from list */
	struct gl_list_t *symbollist; /**< list of symbol values struct used to assign an integer value to a symbol value */
	struct {
		struct var_variable **incidence; /**< all relation incidence list memory */
		struct rel_relation **varincidence; /**< all variable incidence list memory */
		struct dis_discrete **logincidence; /**< all logrel incidence list memory */
		long incsize;     /**< size of incidence array */
		long varincsize;  /**< size of varincidence array */
		long logincsize;  /**< size of discrete incidence array */
#if NEEDSTOBEDONE
		/* we should be group allocating this data, but aren't */
		struct ExtRelCache *ebuf; /**< data space for all extrel caches */
#endif
	} data;

	int32 nmodels;
	int32 need_consistency; /**< consistency analysis required for conditional model ? */
	real64 objvargrad; /**< maximize -1 minimize 1 noobjvar 0 */
};

/* @} */

