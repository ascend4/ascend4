/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.26 $
 *  Version control file: $RCSfile: var.h,v $
 *  Date last modified: $Date: 1998/03/30 22:07:10 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */
/**
 *  Contents:     Variable module (ascend)
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                01/94 - added var_make_name() to create a copy of the
 *                        instance name
 *                04/94 - added var_apply_filter() which uses the new
 *                        var_filter_t data structure for perfoming all
 *                        variable filtering needs (eliminating the
 *                        filter module)
 *		    08/94 - added var_BackendTokens_to_vars. BAA.
 *
 *  Description:  This is the ascend version of the var module.  This
 *                version should be used by any user who receives his/her
 *                equations indirectly from an instance tree created by the
 *                ASCEND compiler.
 *
 *                The idea of a var makes no sense outside the context of
 *                a slv_system_t, so some of the functions here may require
 *                the passing of a slv_system_t with the var.
 */

#ifndef var__already_included
#define var__already_included

/* 
 * requires #include <stdio.h>
 * requires #include "base.h"
 * requires #include "slv_types.h"
 */

/*
 * The first section of flagbit definitions are those flags to be
 * set by the server.  Clients should not mess with them unless
 * they are very, very smart.
 */
#define VAR_REAL	0x1
/**<  don't know what this is good for. */
#define VAR_INCIDENT	0x2
/**< is this variable incident on some equation in the slv_system? */
#define VAR_PVAR 	0x4
/**< do we think this var a 'solver_par'? */
#define VAR_SVAR	0x8
/**< do we think this var a 'solver_var'? */
#define VAR_INTEGER	0x10
/**< do we think this var a solver_integer? */
#define VAR_BINARY	0x20
/**< do we think this var a solver_binary?  */
#define VAR_SEMICONT	0x40
/**< do we think this var a solver_semi(continuous) ?  */

/*
 * The remaining flagbit definitions are those flags to be
 * manipulated by registered clients. Unregistered clients
 * should not change these bits unless the manner in which
 * they affect the bits is explained in the header for the
 * unregistered client.
*/

#define VAR_PARAM	0x1000
/**< VAR_PARAM      is this variable considered parametric currently? */

#define VAR_FIXED	0x2000
/**< VAR_FIXED      is this variable considered fixed currently? */

#define VAR_INBLOCK	0x4000
/**< VAR_INBLOCK    is this var in the columns of the block we are 
                currently solving?  */

#define VAR_INTERFACE	0x8000
/**< VAR_INTERFACE  is this var an interface var currently? */

#define VAR_RELAXED	0x10000
/**< VAR_RELAXED    is this var currently a relaxed int or binary or semi var? */

#define VAR_ACTIVE      0x20000
/**< VAR_ACTIVE     is this var currently a part of my problem */

#define VAR_NONBASIC    0x40000
/**< VAR_NONBASIC   is this var currently an independent variable 
 *                (optimization)? */

#define VAR_ACTIVE_AT_BND  0x80000
/**< VAR_ACTIVE_AT_BND is this var active in some of the subregions 
 *                neighboring the boundary on which the problem 
 *                currently lies ? */

#define VAR_ELIGIBLE    0x100000
/**< VAR_ELIGIBLE   is this variable eligible to be fixed (globally)? */

#define VAR_ELIGIBLE_IN_SUBREGION 0x200000
/**< VAR_ELIGIBLE_IN_SUBREGION is this variable eligible to be fixed (in current
 *                           subregion) ? */

#define VAR_POTENTIALLY_FIXED 0x400000
/**< VAR_POTENTIALLY_FIXED this bit is auxiliar while iteratively and recurively
 *                       we are performing a combinatorial search, looking
 *                       for a consitent partition for all the alternatives
 *                       in an conditional model */

#define VAR_INCIDENT_IN_CASE    0x800000
/**< VAR_INCIDENT_IN_CASE is this variable incident in some relation of a 
 *                CASE in a WHEN statement ?. This is for purposes of
 *                simplfying the analysis of conditional models
 */

struct var_variable {
  SlvBackendToken ratom;	/**< the associated ascend ATOM */
  struct rel_relation **incidence;
  /**< array rels which contain var */
  int32 n_incidences;		/**< length of incidence. */
  int32 sindex;	/**< index in the solver clients list (often column index) */
  int32 mindex;	/**< index in the slv_system_t master list */
  uint32 flags; /**< batch of binary flags. The bit positions are as above */
};

/**<
 *  Finally, we have a real structure so that we aren't hanging stuff
 * of the global instance tree. Do not dereference this structure except
 * via macros/functions, because we make no commitments about being
 * backward compatible with such code.
 *
 * struct Instance * and struct var_variable *are no longer synonymous!
 */

typedef struct var_filter_structure {
  uint32 matchbits;
  uint32 matchvalue;
} var_filter_t;


#define var_NO_UPPER_BOUND MAXDOUBLE
#define var_NO_LOWER_BOUND (-MAXDOUBLE/2)

#ifdef NDEBUG
#define var_instance(v) ((v)->ratom)
#define var_set_instance(v,i) ((v)->ratom = (i))
#else 
#define var_instance(v) var_instanceF(v)
#define var_set_instance(v,i) var_set_instanceF((v),(i))
#endif /* NDEBUG */

extern SlvBackendToken var_instanceF(const struct var_variable *);
extern void var_set_instanceF(struct var_variable *,SlvBackendToken );
/**<
 * i = var_instance(var); 
 * var_set_instance(var,i);
 * struct var_variable *var;
 * SlvBackendToken i;
 * Returns/sets the ATOM instance associated with the variable.
 */

extern char *var_make_name(const slv_system_t,const struct var_variable *);
extern char *var_make_xname(const struct var_variable *);
/**<
 *  name = var_make_name(sys,var)
 *  name = var_make_xname(var)
 *  struct var_variable *var;
 *
 *  Creates and returns a sufficiently large string storing the 
 *  qualified name of var as known by the solver instance tree.
 *  If the instance is not found, "?" is returned.  The string
 *  should be destroyed when no longer in use.
 *  var_make_xname returns the index name, eg x23 rather than full name.
 *  
 *  The name of a var is context dependent, so you have to provide the
 *  slv_system_t from which you got the var.
 */

extern void var_write_name(const slv_system_t,
                           const struct var_variable *,FILE *);
/**<
 *  var_write_name(sys,var,file);
 *  Writes a name to the file given. Handles NULL inputs gracefully.
 *  Does not print any whitespace, including carriage returns.
 *  Is faster than slv_print_var_name.
 *  If sys is NULL, writes full ascend name. If file or var is NULL 
 *  does not write.
 */

void var_destroy(struct var_variable *);
/**<
 *  var_destroy(var);
 *  struct var_variable *var;
 *
 *  Since someone else allocates vars en masse, this just sets
 *  our integrity check to ERROR.
 */

#ifdef NDEBUG
#define var_mindex(v) (v)->mindex
#define var_set_mindex(v,n) (v)->mindex = (n)
#else 
#define var_mindex(v) var_mindexF(v)
#define var_set_mindex(v,n) var_set_mindexF((v),(n))
#endif /* NDEBUG */

extern int32 var_mindexF(const struct var_variable *);
extern void var_set_mindexF(struct var_variable *,int32);
/**<
 *  index = var_mindex(var)
 *  var_set_mindex(var,index)
 *  int32 index;
 *  struct var_variable *var;
 *
 *  Gets/sets the index of the variable as it appears
 *  in a variable list.  The index is also used to assign
 *  the variable to a specific original column of a matrix.
 */

#ifdef NDEBUG
#define var_sindex(v) (v)->sindex
#define var_set_sindex(v,n) (v)->sindex = (n)
#else 
#define var_sindex(v) var_sindexF(v)
#define var_set_sindex(v,n) var_set_sindexF((v),(n))
#endif /* NDEBUG */

extern int32 var_sindexF(const struct var_variable *);
extern void var_set_sindexF(struct var_variable *,int32);
/**<
 *  index = var_sindex(var)
 *  var_set_sindex(var,index)
 *  int32 index;
 *  struct var_variable *var;
 *
 *  Gets/sets the index of the variable as it appears
 *  in a solvers variable list.  The index is also used to assign
 *  the variable to a specific original column of a matrix.
 */

extern real64 var_value(const struct var_variable *);
extern void var_set_value(struct var_variable *,real64);
/**<
 *  value = var_value(var)
 *  var_set_value(var,value)
 *  real64 value;
 *  struct var_variable *var;
 *
 *  Gets/sets the value of the variable.
 */

extern real64 var_nominal(struct var_variable *);
extern void var_set_nominal(struct var_variable *,real64);
/**<
 *  nominal = var_nominal(var)
 *  var_set_nominal(var,nominal)
 *  real64 nominal;
 *  struct var_variable *var;
 *
 *  Gets/sets the nominal value of the variable.
 *  If no nominal field in var, returns 1.0.
 */

extern real64 var_lower_bound(struct var_variable *);
extern void var_set_lower_bound(struct var_variable *,real64);
/**<
 *  lower_bound = var_lower_bound(var)
 *  var_set_lower_bound(var,lower_bound)
 *  real64 lower_bound;
 *  struct var_variable *var;
 *
 *  Gets/sets the lower bound value of the variable.
 */

extern real64 var_upper_bound(struct var_variable *);
extern void var_set_upper_bound(struct var_variable *,real64);
/**<
 *  upper_bound = var_upper_bound(var)
 *  var_set_upper_bound(var,upper_bound)
 *  real64 upper_bound;
 *  struct var_variable *var;
 *
 *  Gets/sets the upper bound value of the variable.
 */

#ifdef NDEBUG
#define var_n_incidences(r) ((r)->n_incidences)
#define var_set_incidences(r,n,ilist) \
  (r)->n_incidences=(n); (r)->incidence = (ilist)
#else
#define var_n_incidences(r) var_n_incidencesF(r)
#define var_set_incidences(r,n,ilist) var_set_incidencesF((r),(n),(ilist))
#endif  /* NDEBUG  */

extern int32 var_n_incidencesF(struct var_variable *);
extern void var_set_incidencesF(struct var_variable *,
                                int32,struct rel_relation **);
/**<
 *  var_n_incidences(var)
 *  var_set_incidences(var,n,ilist) //SERVER ONLY
 *  struct var_variable *var;
 *  struct rel_relation **ilist;
 *  int32 n;
 *
 *  var_n_incidences returns the length of the incidence_list.
 *  Not everything in the incidence list is necessarily a
 *  variable for your particular solver -- check the flags.
 *  Solver clients should not call var_set_incidences,
 *  it is only for use by constructors of bridges to varation
 *  back ends.
 */

extern const struct rel_relation **var_incidence_list(struct var_variable *);
extern struct rel_relation
**var_incidence_list_to_modify(struct var_variable *);
/**<
 *  va = var_incidence_list(var)
 *  struct var_variable *var;
 *  struct rel_relation **va;
 *
 *  Returns a pointer to an array var_n_incidences(var) long of vars.
 *  Each element of the array is a struct rel_relation *.
 *  Check the var sindex to see where each might go in a jacobian.
 *  If there is no incidence, NULL is returned.
 *  Pointers in this array will be unique. 
 *  The list belongs to the varation. Do not destroy it. Do not change it.
 *
 *  RA IS NOT a NULL-TERMINATED LIST.
 */


     
#ifdef NDEBUG
#define var_flags(v) ((v)->flags)
#define var_set_flags(v,f) ((v)->flags = (f))
#else
#define var_flags(v) var_flagsF(v)
#define var_set_flags(v,f) var_set_flagsF((v),(f))
#endif /* NDEBUG */


extern uint32 var_flagsF(const struct var_variable *);
extern void var_set_flagsF(struct var_variable *,uint32);
/**<
 *  var_flags(var);
 *  var_set_flags(var,flags);
 *  struct var_variable *var;
 *  uint32 flags;
 *
 * var_flags(var) returns the flags field of the var.
 * var_set_flags(var,flags) sets the entire flag field to the
 * value of flags given. This flags value should be composed
 * of the var_xxx values defined above.
 */

extern uint32 var_flagbit(const struct var_variable *,const uint32);
/**<
 *  var_flagbit(rel,name);
 *  struct var_variable *rel;
 *  uint32 name;
 *  name should be a VAR_xx flag defined above)
 *  Returns the value of the bit specified from the variable flags.
 */

extern void var_set_flagbit(struct var_variable *,uint32, uint32);
/**<
 *  struct var_variable *var;
 *  unsigned int32 NAME,oneorzero;
 *  var_set_flagbit(var,NAME,oneorzero)
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 *
 *  Example: var_set_flags(var,VAR_PARTITION,1) turns on the
 *  VAR_PARTITION bit.
 *  What it really does is:
 *  if (oneorzero) {
 *    var->flags |= field;
 *  } else {
 *    var->flags &= ~field;
 *  }
 */

/**<
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

extern int32 var_apply_filter(const struct var_variable *,
                               const var_filter_t *);
/**<
 *  value = var_apply_filter(var,filter)
 *  int32 value;
 *  struct var_variable *var;
 *  var_filter_t *filter;
 *
 *  Returns 1 if filter and var flags are compatible, 0 elsewise.
 *  See the filter description in rel.h. This is exactly the same.
 */

extern uint32 var_fixed(struct var_variable *);
extern void var_set_fixed(struct var_variable *, uint32);
extern uint32 var_relaxed(struct var_variable *);
extern void var_set_relaxed(struct var_variable *, uint32);
extern uint32 var_interface(struct var_variable *);
extern void var_set_interface(struct var_variable *, uint32);
/**<
 *  fixed = var_fixed(var)
 *  var_set_fixed(var,fixed)
 *  uint32 fixed;
 *  struct var_variable *var;
 *
 *  Gets/sets the fixed/relaxed/interface flag of the variable. This
 *  has side effects in the ascend instance, with which
 *  we are keeping the bits in sync.
 */

#ifdef NDEBUG
#define var_in_block(v) ((v)->flags & VAR_INBLOCK)
#define var_incident(v) ((v)->flags & VAR_INCIDENT)
#define var_active(v)   ((v)->flags & VAR_ACTIVE)
#define var_nonbasic(v) ((v)->flags & VAR_NONBASIC)
#define var_active_at_bnd(v)   ((v)->flags & VAR_ACTIVE_AT_BND)
#define var_eligible(v)  ((v)->flags & VAR_ELIGIBLE)
#define var_eligible_in_subregion(v)  ((v)->flags & VAR_ELIGIBLE_IN_SUBREGION)
#define var_incident_in_case(v)  ((v)->flags & VAR_INCIDENT_IN_CASE)
#define var_potentially_fixed(v)  ((v)->flags & VAR_POTENTIALLY_FIXED)
#else 
#define var_in_block(v) var_flagbit((v),VAR_INBLOCK)
#define var_incident(v) var_flagbit((v),VAR_INCIDENT)
#define var_active(v)   var_flagbit((v),VAR_ACTIVE)
#define var_nonbasic(v) var_flagbit((v),VAR_NONBASIC)
#define var_active_at_bnd(v)   var_flagbit((v),VAR_ACTIVE_AT_BND)
#define var_eligible(v) var_flagbit((v),VAR_ELIGIBLE)
#define var_eligible_in_subregion(v) var_flagbit((v),VAR_ELIGIBLE_IN_SUBREGION)
#define var_incident_in_case(v) var_flagbit((v),VAR_INCIDENT_IN_CASE)
#define var_potentially_fixed(v) var_flagbit((v),VAR_POTENTIALLY_FIXED)
#endif /* NDEBUG */
#define var_set_in_block(v,b) var_set_flagbit((v),VAR_INBLOCK,(b))
#define var_set_incident(v,b) var_set_flagbit((v),VAR_INCIDENT,(b))
#define var_set_interface(v,b) var_set_flagbit((v),VAR_INTERFACE,(b))
#define var_set_active(v,b)   var_set_flagbit((v),VAR_ACTIVE,(b))
#define var_set_nonbasic(v,b) var_set_flagbit((v),VAR_NONBASIC,(b))
#define var_set_active_at_bnd(v,b) var_set_flagbit((v),VAR_ACTIVE_AT_BND,(b))
#define var_set_eligible(v,b) var_set_flagbit((v),VAR_ELIGIBLE,(b))
#define var_set_eligible_in_subregion(v,b) var_set_flagbit((v),VAR_ELIGIBLE_IN_SUBREGION,(b))
#define var_set_incident_in_case(v,b) var_set_flagbit((v),VAR_INCIDENT_IN_CASE,(b))
#define var_set_potentially_fixed(v,b) var_set_flagbit((v),VAR_POTENTIALLY_FIXED,(b))

/**<
 *  incident = var_incident(var)
 *  var_set_incident(var,incident)
 *  uint32 incident;
 *  struct var_variable *var;
 *
 *  Gets/sets the incident flag of the variable.
 */
/**<
 *  in_block = var_in_block(var)
 *  var_set_in_block(var,in_block)
 *  uint32 in_block;
 *  struct var_variable *var;
 *
 *  Gets/sets the in_block flag of the variable.
 */

extern struct var_variable **var_BackendTokens_to_vars(slv_system_t,
						SlvBackendToken *, int32);
/**<
 *  varp = var_BackendTokens_to_vars(sys,tokenlist,len);
 *  slv_system_t sys;			System to get indexing from.
 *  SlvBackendToken tokenlist[];	Array of backend tokens.
 *  int32 len;			Tokenlist size.
 *  struct var_variable *varp[];	aka **varp;
 *
 *  varp is NULL iff something is amiss, OTHERWISE it
 *  contains len struct var_variable * s.
 *  The user should free the array varp when done with it.
 *  Some entries in the array varp may be NULL if the tokenlist
 *  contains a token which is not from the sys given.
 *  tokenlist[i] <--> varp[i];
 *
 *  The whole point of a slv_system_t is to isolate the client from
 *  the compiler backend. Clients who find themselves in need of
 *  this function are very much in need of rethinking as well.
 *  For that reason, among others, this function is not heavily
 *  optimized, it is however reasonable for 1-off jobs.
 *  
 *  CPU Cost = k1*complexity of the instance under sys +
 *         k2*number of vars in sys + k3*len.
 *
 *  BUGS: hasn't been through verification and testing. SQA asleep.
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

extern boolean solver_var(SlvBackendToken);
extern boolean solver_int(SlvBackendToken);
extern boolean solver_binary(SlvBackendToken);
extern boolean solver_semi(SlvBackendToken);
/**<
 *  e.g. if (solver_var(inst)) {}
 *  SlvBackendToken inst;
 *  Returns true if the instance in question matches the currently
 *  known definition of solver_var.
 *  Ditto for solver_int, solver_binary, and solver_semi.
 *  Note that these definitions involve refinement, so
 *  solver_var will report TRUE on variables matching any of
 *  these types, while solver_binary will only be TRUE on binaries.
 *  hierarchy is solvervar-\----solver_int-----solver_binary
 *                          \-solver_semi
 *
 *  SERVER ONLY. clients should just look at the flagbits.
 */

extern boolean set_solver_types(void);
/**<
 *  Sets (changes) the current definition of solver_var to match
 *  the current library. Returns 1 if unsuccessful, 0 if ok.
 *  If 1, it is useless to invoke the solvers.
 *  Also sets all the related types listed above.
 */
#endif /* __VAR_H__SEEN__ */
