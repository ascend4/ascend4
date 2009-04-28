/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

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
	Variable module of the SLV solver.

	This module provides a SLV solver (the "client") with access to
	variables present in the ASCEND compiler (the "server").

	Access to ASCEND variables is abstracted so that SLV solvers
	can theoretically be used to solve other kinds of systems as
	well. @see analyze.h

	The idea of a var makes no sense outside the context of
	a slv_system_t, so some of the functions here may require
	the passing of a slv_system_t with the var.

	06/90 - original version
	01/94 - added var_make_name() to create a copy of the
			instance name
	04/94 - added var_apply_filter() which uses the new
			var_filter_t data structure for perfoming all
			variable filtering needs (eliminating the
			filter module)
	08/94 - added var_BackendTokens_to_vars. BAA.

	@TODO
	At present there is some insanity about asking the instance tree
	vs asking our data structure about the flag values - This needs to
	be fixed (see longer BAA comment in var.h).
*//*
	Authors: Karl Westerberg and Joseph Zaher
	Version: $Revision: 1.26 $
	Date last modified: $Date: 1998/03/30 22:07:10 $
	Last modified by: $Author: rv2a $
*/

#ifndef ASC_VAR_H
#define ASC_VAR_H

/**	@addtogroup system System
	@{
*/

#include <stdio.h>
#include <utilities/ascConfig.h>
#include "slv_types.h"

/*
	The first section of flagbit definitions are those flags to be
	set by the server.  Clients should not mess with them unless
	they are very, very smart.
 */
#define VAR_REAL      0x1
/**<  don't know what this is good for. */
#define VAR_INCIDENT  0x2
/**< is this variable incident on some equation in the slv_system? */
#define VAR_PVAR      0x4
/**< do we think this var a 'solver_par'? */
#define VAR_SVAR      0x8
/**< do we think this var a 'solver_var'? */
#define VAR_INTEGER   0x10
/**< do we think this var a solver_integer? */
#define VAR_BINARY    0x20
/**< do we think this var a solver_binary?  */
#define VAR_SEMICONT  0x40
/**< do we think this var a solver_semi(continuous) ?  */

/*
	Added this one. Not sure if it belongs 'above the line' here or
	not. -- JP 20061222
*/
#define VAR_DERIV     0x80
/**< var is a derivative of some other var present in the system */
#define VAR_DIFF     0x100
/**<
	var is differential if is posesses a derivative explicitly stated in the
	system of equations. Can't have both alg&deriv flags set (unless higher
	order derivatives have been implemented?)
*/

/*
	The remaining flagbit definitions are those flags to be
	manipulated by registered clients. Unregistered clients
	should not change these bits unless the manner in which
	they affect the bits is explained in the header for the
	unregistered client.
*/

#define VAR_PARAM         0x1000
/**< is this variable considered parametric currently? */
#define VAR_FIXED         0x2000
/**< is this variable considered fixed currently? */
#define VAR_INBLOCK       0x4000
/**< is this var in the columns of the block we are currently solving?  */
#define VAR_INTERFACE     0x8000
/**< is this var an interface var currently? */
#define VAR_RELAXED       0x10000
/**< is this var currently a relaxed int or binary or semi var? */
#define VAR_ACTIVE        0x20000
/**< is this var currently a part of my problem? */
#define VAR_NONBASIC      0x40000
/**< is this var currently an independent variable (optimization)? */
#define VAR_ACTIVE_AT_BND 0x80000
/**< is this var active in some of the subregions  neighboring the
	 boundary on which the problem currently lies ? */
#define VAR_ELIGIBLE      0x100000
/**< is this variable eligible to be fixed (globally)? */
#define VAR_ELIGIBLE_IN_SUBREGION 0x200000
/**< is this variable eligible to be fixed (in current subregion) ? */
#define VAR_POTENTIALLY_FIXED 0x400000
/**< this bit is auxiliar while iteratively and recurively we are
	performing a combinatorial search, looking for a consitent
	partition for all the alternatives in an conditional model */
#define VAR_INCIDENT_IN_CASE    0x800000
/**< is this variable incident in some relation of a CASE in a
	WHEN statement ?. This is for purposes of simplfying the
	analysis of conditional models */

/**
	Variable structure.
	Finally, we have a real structure so that we aren't hanging stuff
	of the global instance tree. Do not dereference this structure except
	via macros/functions, because we make no commitments about being
	backward compatible with such code.

	struct Instance * and struct var_variable * are no longer synonymous!
*/
struct var_variable {
  SlvBackendToken ratom;  /**< the associated ascend ATOM */
  struct rel_relation **incidence;  /**< array rels which contain var */
  int32 n_incidences;     /**< length of incidence. */
  int32 sindex;           /**< index in the solver clients list (often column index) */
  int32 mindex;           /**< index in the slv_system_t master list */
  uint32 flags;           /**< batch of binary flags. The bit positions are as above */
};

/** Variable filter structure */
typedef struct var_filter_structure {
  uint32 matchbits;   /**< Bits to match. */
  uint32 matchvalue;  /**< Value to match. */
} var_filter_t;
/**< Variable filter type */

#define var_NO_UPPER_BOUND MAXDOUBLE
#define var_NO_LOWER_BOUND (-MAXDOUBLE/2)

#ifdef NDEBUG
# define var_instance(var) ((var)->ratom)
#else
# define var_instance(var) var_instanceF(var)
#endif
/**<
	Returns the ATOM instance associated with the variable.
	@param var const struct var_variable *, the variable to query.
	@return Returns the instance as a SlvBackendToken.
	@see var_instanceF()
*/

#ifdef NDEBUG
# define var_set_instance(var,inst) ((var)->ratom = (inst))
#else
# define var_set_instance(var,inst) var_set_instanceF((var),(inst))
#endif
/**<
	Sets the ATOM instance associated with the variable.
	@param var  struct var_variable *, the variable to modify.
	@param inst SlvBackendToken, the new instance value.
	@return No return value.
	@see var_instanceF()
*/

ASC_DLLSPEC SlvBackendToken var_instanceF(const struct var_variable *var);
/**<
	Implementation function for var_instance().  Do not call this
	function directly - use var_instance() instead.
*/
extern void var_set_instanceF(struct var_variable *var, SlvBackendToken i);
/**<
	Implementation function for var_set_instance().  Do not call this
	function directly - use var_set_instance() instead.
*/

extern char *var_make_xname(const struct var_variable *var);
/**<
	Returns the index name, eg x23 rather than full name.
	See var_make_name() for more information.
*/
ASC_DLLSPEC char*var_make_name(
	const slv_system_t sys,
	const struct var_variable *var
);
/**<
	Creates and returns a sufficiently large string storing the
	qualified name of var as known by the solver instance tree.
	If the instance is not found, "?" is returned.  The string
	should be destroyed when no longer in use.<br><br>

	The name of a var is context dependent, so you have to provide the
	slv_system_t from which you got the var.
*/

ASC_DLLSPEC void var_write_name(const slv_system_t sys,
                           const struct var_variable *var,
                           FILE *file);
/**<
	Writes a name to the file given. Handles NULL inputs gracefully.
	Does not print any whitespace, including carriage returns.
	Is faster than slv_print_var_name().
	If sys is NULL, writes full ascend name. If file or var is NULL
	does not write.
*/

void var_destroy(struct var_variable *var);
/**<
	Since someone else allocates vars en masse, this just sets
	our integrity check to ERROR.
*/

#ifdef NDEBUG
# define var_mindex(var) (var)->mindex
#else
# define var_mindex(var) var_mindexF(var)
#endif
/**<
	Returns the index of the variable as it appears in a variable list.
	The index is also used to assign the variable to a specific
	original column of a matrix.
	@param var  const struct var_variable *, the variable to query.
	@return Returns the index as an int32.
	@see var_mindexF().
*/


/**
	Sets the index of the variable as it appears in a variable list.
	@param var   struct var_variable *, the variable to modify.
	@param index int32, the new value for the index.
	@return No return value.
	@see var_set_mindexF().
*/
#ifdef NDEBUG
# define var_set_mindex(var,index) (var)->mindex = (index)
#else
# define var_set_mindex(var,index) var_set_mindexF((var),(index))
ASC_DLLSPEC int32 var_mindexF(const struct var_variable *var);
/**<
	Implementation function for var_mindex().  Do not call this
	function directly - use var_mindex() instead.
*/
#endif


extern void var_set_mindexF(struct var_variable *var, int32 mindex);
/**<
	Implementation function for var_set_mindex().  Do not call this
	function directly - use var_set_mindex() instead.
*/

#ifdef NDEBUG
# define var_sindex(var) (var)->sindex
#else
# define var_sindex(var) var_sindexF(var)
#endif
/**<
	Returns the index of the variable as it appears in a solvers
	variable list.  The index is also used to assign the variable
	to a specific original column of a matrix.
	@param var  const struct var_variable *, the variable to query.
	@return Returns the index as an int32.
	@see var_sindexF().
*/

#ifdef NDEBUG
# define var_set_sindex(var,index) (var)->sindex = (index)
#else
# define var_set_sindex(var,index) var_set_sindexF((var),(index))
#endif
/**<
	Sets the index of the variable as it appears in a solver's
	variable list.
	@param var   struct var_variable *, the variable to modify.
	@param index int32, the new value for the index.
	@return No return value.
	@see var_set_sindexF().
*/

ASC_DLLSPEC int32 var_sindexF(const struct var_variable *var);
/**<
	Implementation function for var_sindex().  Do not call this
	function directly - use var_sindex() instead.
*/
ASC_DLLSPEC void var_set_sindexF(struct var_variable *var, int32 sindex);
/**<
	Implementation function for var_set_sindex().  Do not call this
	function directly - use var_set_sindex() instead.
*/

ASC_DLLSPEC real64 var_value(const struct var_variable *var);
/**<  Returns the value of the variable. */
ASC_DLLSPEC void var_set_value(struct var_variable *var, real64 value);
/**<
	Sets the value of the variable.
*/

ASC_DLLSPEC real64 var_nominal(struct var_variable *var);
/**<  Returns the nominal value of the variable, or 1.0 if it has none. */
ASC_DLLSPEC void var_set_nominal(struct var_variable *var, real64 nominal);
/**<
	Sets the nominal value of the variable.
	If no nominal field in var, returns 1.0.
*/

ASC_DLLSPEC real64 var_lower_bound(struct var_variable *var);
/**<  Returns the lower bound value of the variable. */
extern void var_set_lower_bound(struct var_variable *var, real64 lower_bound);
/**<
	Sets the lower bound value of the variable.
*/

ASC_DLLSPEC real64 var_upper_bound(struct var_variable *var);
/**<  Returns the upper bound value of the variable. */
extern void var_set_upper_bound(struct var_variable *var, real64 upper_bound);
/**<
	Gets/sets the upper bound value of the variable.
*/

double var_odeatol(struct var_variable *var);
/**< Returns the ODE absolute error tolerance for the variable, or -1 if not available */

#ifdef NDEBUG
# define var_n_incidences(var) ((var)->n_incidences)
#else
ASC_DLLSPEC int32 var_n_incidencesF(struct var_variable *var);
# define var_n_incidences(var) var_n_incidencesF(var)
#endif
/**<
	Returns the length of the incidence list.
	Not everything in the incidence list is necessarily a
	variable for your particular solver -- check the flags.
	@param var  const struct var_variable *, the variable to query.
	@return Returns the index as an int32.
	@see var_n_incidencesF().
*/

#ifdef NDEBUG
# define var_set_incidences(var,n,ilist) \
  (var)->n_incidences=(n); (var)->incidence = (ilist)
#else
# define var_set_incidences(var,n,ilist) var_set_incidencesF((var),(n),(ilist))
#endif
/**<
	Sets the incidence list for a variable.
	Solver clients should not call var_set_incidences().  It is
	only for use by constructors of bridges to varation back ends.
	@param var   struct var_variable *, the variable to query.
	@param n     int32, length of the new incidence list.
	@param ilist struct rel_relation **, the new incidence list.
	@return No return value.
	@see var_set_incidencesF().
*/

extern void var_set_incidencesF(struct var_variable *var,
                                int32 n,
                                struct rel_relation **ilist);
/**<
	Implementation function for var_set_incidences().  Do not call this
	function directly - use var_set_incidences() instead.
*/

ASC_DLLSPEC struct rel_relation **var_incidence_list_to_modify(
		struct var_variable *var);
/**<  Same as var_incidence_list(), except the returned list is modifiable. */

ASC_DLLSPEC const struct rel_relation **var_incidence_list(struct var_variable *var);
/**<
	Returns a pointer to an array var_n_incidences(var) long of vars.
	Each element of the array is a struct rel_relation *.
	Check the var sindex to see where each might go in a jacobian.
	If there is no incidence, NULL is returned.
	Pointers in this array will be unique.

	The list belongs to the varation. Do not destroy it. Do not change it.

	RA IS NOT a NULL-TERMINATED LIST.
*/

#ifdef NDEBUG
# define var_flags(var) ((var)->flags)
#else
# define var_flags(var) var_flagsF(var)
#endif
/**<
	Returns the flags field of the var.  This flags value should
	be composed of the var_xxx values defined above.
	@param var  const struct var_variable *, the variable to query.
	@return Returns the index as an int32.
	@see var_flagsF().
*/

#ifdef NDEBUG
# define var_set_flags(var,flags) ((var)->flags = (flags))
#else
# define var_set_flags(var,flags) var_set_flagsF((var),(flags))
#endif
/**<
	Sets the entire flag field to the value of flags given.
	This flags value should be composed of the var_xxx values
	defined above.
	@param var   struct var_variable *, the variable to query.
	@param flags uint32, the new flags value.
	@return No return value.
	@see var_set_flagsF().
*/

ASC_DLLSPEC uint32 var_flagsF(const struct var_variable *var);
/**<
	Implementation function for var_flags().  Do not call this
	function directly - use var_flags() instead.
 */
extern void var_set_flagsF(struct var_variable *var, uint32 flags);
/**<
	Implementation function for var_set_flags().  Do not call this
	function directly - use var_set_flags() instead.
*/

ASC_DLLSPEC uint32 var_flagbit(const struct var_variable *rel, const uint32 name);
/**<
	Returns the value of the bit specified from the variable flags.
	name should be a VAR_xx flag defined above.
*/

ASC_DLLSPEC void var_set_flagbit(struct var_variable *var,
                            uint32 NAME,
                            uint32 oneorzero);
/**<
	Sets the bit, which should be referred to by its macro name,
	on if oneorzero is >0 and off is oneorzero is 0.
	The macro names are the defined up at the top of this file.<br><br>

	Example: var_set_flags(var,VAR_PARTITION,1) turns on the
	VAR_PARTITION bit.
	<pre>
	What it really does is:
	    if (oneorzero) {
	      var->flags |= field;
	    } else {
	      var->flags &= ~field;
	    }
	</pre>
*/

/*
 * At present there is some insanity about asking the instance tree
 * vs asking our data structure about the flag values. This needs to
 * be fixed. In the process of that fix, we should establish a
 * protocol that means vars/rels never need to look back at ATOM
 * instances to be queried during the solution process. Rather, we
 * need to establish a process in which the var list tells all its
 * atoms about the solver derived values or the var list updates
 * its values and flags from the instance tree.
 * There is a counterpart exercise that needs to go on in the
 * rel module so that (for remote processing) it is possible to
 * do everything without looking back at the instance tree.
 * At present, we will always keep VAR_FIXED in sync with the
 * instance tree, but the instance tree is the leader.
 * 6/96. baa
*/

ASC_DLLSPEC int32 var_apply_filter(const struct var_variable *var,
                              const var_filter_t *filter);
/**<
	Returns 1 if filter and var flags are compatible, 0 elsewise.
	See the filter description in rel.h. This is exactly the same.
*/

ASC_DLLSPEC uint32 var_fixed(struct var_variable *var);
/**< Returns the fixed flag of var.  Has side effects in the ascend instance. */
ASC_DLLSPEC void var_set_fixed(struct var_variable *var, uint32 fixed);
/**< Sets the fixed flag of var.  Has side effects in the ascend instance. */
extern uint32 var_relaxed(struct var_variable *var);
/**< Returns the relaxed flag of var.  Has side effects in the ascend instance. */
extern void var_set_relaxed(struct var_variable *var, uint32 fixed);
/**< Sets the relaxed flag of var.  Has side effects in the ascend instance. */
extern uint32 var_interface(struct var_variable *var);
/**< Returns the interface flag of var.  Has side effects in the ascend instance. */
extern void var_set_interface(struct var_variable *var, uint32 fixed);
/**< Sets the interface flag of var.  Has side effects in the ascend instance.
	@todo solver/var.h: var_set_interface() not implemented.  Implement or remove.
*/

#ifdef NDEBUG
# define var_in_block(var) ((var)->flags & VAR_INBLOCK)
  /**<  Returns the in_block flag of var as a uint32. */
# define var_incident(var) ((var)->flags & VAR_INCIDENT)
  /**<  Returns the incident flag of var as a uint32. */
# define var_deriv(var) ((var)->flags & VAR_DERIV)
  /**<  Returns the deriv flag of var as a uint32. */
# define var_diff(var) ((var)->flags & VAR_DIFF)
  /**<  Returns the deriv flag of var as a uint32. */
# define var_active(var)   ((var)->flags & VAR_ACTIVE)
  /**<  Returns the active flag of var as a uint32. */
# define var_nonbasic(var) ((var)->flags & VAR_NONBASIC)
  /**<  Returns the nonbasic flag of var as a uint32. */
# define var_active_at_bnd(var) ((var)->flags & VAR_ACTIVE_AT_BND)
  /**<  Returns the active_at_bnd flag of var as a uint32. */
# define var_eligible(var)      ((var)->flags & VAR_ELIGIBLE)
  /**<  Returns the eligible flag of var as a uint32. */
# define var_eligible_in_subregion(var)  ((var)->flags & VAR_ELIGIBLE_IN_SUBREGION)
  /**<  Returns the eligible_in_subregion flag of var as a uint32. */
# define var_incident_in_case(var)   ((var)->flags & VAR_INCIDENT_IN_CASE)
  /**<  Returns the incident_in_case flag of var as a uint32. */
# define var_potentially_fixed(var)  ((var)->flags & VAR_POTENTIALLY_FIXED)
  /**<  Returns the potentially_fixed flag of var as a uint32. */
#else
# define var_in_block(var) var_flagbit((var),VAR_INBLOCK)
  /**<  Returns the in_block flag of var as a uint32. */

# define var_incident(var)      var_flagbit((var),VAR_INCIDENT)
  /**<  Returns the incident flag of var as a uint32. */
# define var_deriv(var)         var_flagbit((var),VAR_DERIV)
  /**<  Returns the deriv flag of var as a uint32. */
# define var_diff(var)         var_flagbit((var),VAR_DIFF)
  /**<  Returns the deriv flag of var as a uint32. */
# define var_active(var)        var_flagbit((var),VAR_ACTIVE)
  /**<  Returns the active flag of var as a uint32. */
# define var_nonbasic(var)      var_flagbit((var),VAR_NONBASIC)
  /**<  Returns the nonbasic flag of var as a uint32. */
# define var_active_at_bnd(var) var_flagbit((var),VAR_ACTIVE_AT_BND)
  /**<  Returns the active_at_bnd flag of var as a uint32. */
# define var_eligible(var)      var_flagbit((var),VAR_ELIGIBLE)
  /**<  Returns the eligible flag of var as a uint32. */
# define var_eligible_in_subregion(var) var_flagbit((var),VAR_ELIGIBLE_IN_SUBREGION)
  /**<  Returns the eligible_in_subregion flag of var as a uint32. */
# define var_incident_in_case(var)      var_flagbit((var),VAR_INCIDENT_IN_CASE)
  /**<  Returns the incident_in_case flag of var as a uint32. */
# define var_potentially_fixed(var)     var_flagbit((var),VAR_POTENTIALLY_FIXED)
  /**<  Returns the potentially_fixed flag of var as a uint32. */
#endif /* NDEBUG */

#define var_set_in_block(var,oneorzero)      \
        var_set_flagbit((var),VAR_INBLOCK,(oneorzero))
/**<  Sets the in_block flag of var on (1) or off (0). */
#define var_set_incident(var,oneorzero)      \
        var_set_flagbit((var),VAR_INCIDENT,(oneorzero))
/**<  Sets the incident flag of var on (1) or off (0). */
#define var_set_deriv(var,oneorzero)      \
        var_set_flagbit((var),VAR_DERIV,(oneorzero))
/**<  Sets the deriv flag of var on (1) or off (0). */
#define var_set_diff(var,oneorzero)      \
        var_set_flagbit((var),VAR_DIFF,(oneorzero))
/**<  Sets the algeb flag of var on (1) or off (0). */
#define var_set_interface(var,oneorzero)     \
        var_set_flagbit((var),VAR_INTERFACE,(oneorzero))
/**<  Sets the interface flag of var on (1) or off (0). */
#define var_set_active(var,oneorzero)        \
        var_set_flagbit((var),VAR_ACTIVE,(oneorzero))
/**<  Sets the active flag of var on (1) or off (0). */
#define var_set_nonbasic(var,oneorzero)      \
        var_set_flagbit((var),VAR_NONBASIC,(oneorzero))
/**<  Sets the nonbasic flag of var on (1) or off (0). */
#define var_set_active_at_bnd(var,oneorzero) \
        var_set_flagbit((var),VAR_ACTIVE_AT_BND,(oneorzero))
/**<  Sets the active_at_bnd flag of var on (1) or off (0). */
#define var_set_eligible(var,oneorzero)      \
        var_set_flagbit((var),VAR_ELIGIBLE,(oneorzero))
/**<  Sets the eligible flag of var on (1) or off (0). */
#define var_set_eligible_in_subregion(var,oneorzero) \
        var_set_flagbit((var),VAR_ELIGIBLE_IN_SUBREGION,(oneorzero))
/**<  Sets the eligible_in_subregion flag of var on (1) or off (0). */
#define var_set_incident_in_case(var,oneorzero)      \
        var_set_flagbit((var),VAR_INCIDENT_IN_CASE,(oneorzero))
/**<  Sets the incident_in_case flag of var on (1) or off (0). */
#define var_set_potentially_fixed(var,oneorzero)     \
        var_set_flagbit((var),VAR_POTENTIALLY_FIXED,(oneorzero))
/**<  Sets the potentially_fixed flag of var on (1) or off (0). */

extern struct var_variable **
var_BackendTokens_to_vars(slv_system_t sys, SlvBackendToken *tokenlist, int32 len);
/**<
	varp is NULL iff something is amiss, OTHERWISE it
	contains len struct var_variable * s.
	The user should free the array varp when done with it.
	Some entries in the array varp may be NULL if the tokenlist
	contains a token which is not from the sys given.
	tokenlist[i] <--> varp[i];<br><br>

	The whole point of a slv_system_t is to isolate the client from
	the compiler backend. Clients who find themselves in need of
	this function are very much in need of rethinking as well.
	For that reason, among others, this function is not heavily
	optimized, it is however reasonable for 1-off jobs.<br><br>

	CPU Cost = k1*complexity of the instance under sys +
	       k2*number of vars in sys + k3*len.

	@todo Hasn't been through verification and testing - SQA asleep.
*/

/*
 * What constitutes a solver_var (and some other interesting types)
 * is controlled by the ascend server via the following functions.
 * Clients shouldn't use these.
 */

#define SOLVER_VAR_STR    "solver_var"
#define SOLVER_INT_STR    "solver_int"
#define SOLVER_BINARY_STR "solver_binary"
#define SOLVER_SEMI_STR   "solver_semi"

#define solver_par(i) (0)

extern boolean solver_var(SlvBackendToken inst);
/**<
	Returns true if the instance in question matches the currently
	known definition of solver_var.  Since these definitions
	involve refinement, solver_var() will report TRUE on variables
	matching any of these types:
	  - solver_var
	  - solver_int
	  - solver_binary
	  - solver_semi

	SERVER ONLY. clients should just look at the flagbits.
*/
extern boolean solver_int(SlvBackendToken inst);
/**<
	Returns true if the instance in question matches the currently
	known definition of solver_int.
	SERVER ONLY. clients should just look at the flagbits.
*/
extern boolean solver_binary(SlvBackendToken inst);
/**<
	Returns true if the instance in question matches the currently
	known definition of solver_binary.
	SERVER ONLY. clients should just look at the flagbits.
*/
extern boolean solver_semi(SlvBackendToken inst);
/**<
	Returns true if the instance in question matches the currently
	known definition of solver_semi.
	SERVER ONLY. clients should just look at the flagbits.
*/
/* For the above...
	Note that these definitions involve refinement, so
	solver_var will report TRUE on variables matching any of
	these types, while solver_binary will only be TRUE on binaries.

	hierarchy is solvervar-\----solver_int-----solver_binary
	                        \-solver_semi

 */

extern boolean set_solver_types(void);
/**<
	Sets (changes) the current definition of solver_var to match
	the current library. Returns 1 if unsuccessful, 0 if ok.
	If 1, it is useless to invoke the solvers.
	Also sets all the related types listed above.
*/

/* @} */

#endif  /* ASC_VAR_H */

