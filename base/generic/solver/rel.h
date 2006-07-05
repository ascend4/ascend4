/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*//**
	@file
	Relation module for the SLV solver.

	This module provides a SLV solver (the "client") with access to
	relations present in the ASCEND compiler (the "server").

	Access to ASCEND relations is abstracted so that SLV solvers
	can theoretically be used to solve other kinds of systems as
	well. @see analyze.h

	@TODO Question: shouldn't the 'ExtRel' stuff be in the compiler side of
	things? Even if it is being cached, it seems that the request for 
	evaluation is coming from the RelationCalcResidualPostfix call in
	relation_util.c, so we're going	back and forth...

	04/94 - added rel_apply_filter() which uses the new
			rel_filter_t data structure for perfoming all
			relation filtering needs (eliminating the
			filter module)

	Client (ie solver engine) requires:
	#include "var.h"

	Server-side use (ie the ASCEND compiler, analyser, etc) *further* requires:
	#include <expr.h> ??
	#include <compiler/expr_types.h> ??
	#include <compiler/extfunc.h>
	#include <compiler/relation.h>
	#include <compiler/packages.h>
	#include <compiler/extcall.h>
	#include "mtx.h"
*//*
	by Karl Michael Westerberg and Joseph Zaher
	Created: 2/6/90
	Last CVS version: $Revision: 1.41 $ $Date: 1998/02/05 15:59:24 $ $Author: ballan $
*/

#ifndef ASC_REL_H
#define ASC_REL_H

#include <utilities/ascConfig.h>

/*------------------------------------------------------------------------------
  forward decls and typedefs
*/

/*
 * rel_relation data type and basic type operators.
 */

#define rel_TOK_less    0x1
#define rel_TOK_equal   0x2
#define rel_TOK_greater 0x4

struct rel_extnode {
  int32 whichvar;
  struct ExtRelCache *cache;  /**< pointer to private structure */
};

/** The exact operator of a relation. */
enum rel_enum {
  e_rel_minimize,
  e_rel_maximize,
  e_rel_equal,
  e_rel_not_equal,
  e_rel_less,
  e_rel_greater,
  e_rel_lesseq,
  e_rel_greatereq
};

/** Backend relation types. */
enum backend_enum {
  e_rel_token,
  e_rel_glassbox,
  e_rel_blackbox
};

/**
	Relation data structure (solver-side).

	@NOTE
		If you mess with this struct, change the defaults for it in .c file.
*/
struct rel_relation {
	SlvBackendToken instance;
		/**< Normally, this is a pointer to an Instance struct */
	struct rel_extnode *nodeinfo;  
		/**< For use by blackbox relations, NULL in other cases */
	struct var_variable **incidence;
	                    /**< array of atomic vars and nonvars in rel */
	enum backend_enum type;  /**< tokens, glassbox or blackbox */
	int32 n_incidences;      /**< length of incidence. */
	int32 mindex;            /**< index in the slv_system_t master list */
	int32 sindex;            /**< index in the slv_system_t solver list */
	int32 model;             /**< index of a hypothetical MODEL rel is from */
	uint32 flags;            /**< flags */
/*
 * For future use:
 *
 * struct var_variable **incidentals; array of nonvars,
 * once solveratoms arrive.
 *
 * int32 n_incidentals;
 */
};

/**
	Cache for external relations. This is a place to stash away information
	relating to the evaluation of external relations, with the intention of
	making them work efficiently. 

	It is apparently intended to prevent repeated calls to external relations
	when different outputs values are required for the same set of input
	values. Not sure how complete etc it is -- JP.

	Originally present in a separate file 'extrel.h', but now merged into
	'rel.h' in the solver directory.
*/
struct ExtRelCache{
  int32 nodestamp;
  struct ExternalFunc *efunc;       /**< pre_slv, eval and deriv funcs */
  SlvBackendToken data;             /**< only passed on pre_slv */
  struct gl_list_t *arglist;        /**< only passed on pre_slv */
  struct gl_list_t *inputlist;
  struct var_variable **invars; 	/**< pointers to var_variables, in input argument order */
  struct var_variable **outvars;    /**< pointers to var_variables, in output argument order */
  void *user_data;                  /**< user data */
  int32 ninputs, noutputs;
  double *inputs;
  double *outputs;
  double *jacobian;
  unsigned evaluation_required     :1;     /**< bits needed to control */
  unsigned first_func_eval  :1;     /**< recalculation. until we can */
  unsigned first_deriv_eval :1;     /**< do proper block coding */
};

/*------------------------------------------------------------------------------
  CREATION / MANIPULATION OF REL_RELATION OBJECTS
*/

extern struct rel_relation *rel_create(SlvBackendToken instance,
                                       struct rel_relation *rel);
/**<
	Creates a relation given the relation instance.
	If the rel supplied is NULL, we allocate the memory for the
	rel we return, else we just init the memory you hand us and
	return it to you.

	We set the fields instance, nodeinfo, type following
	the instance. Setting the rest of the information is the job
	of the bridge building function between the ascend instance
	tree (or other relation back end) and the slv_system_t.
	In particular, the incidence list and indexing info is not
	handled here.
*/

extern void rel_destroy(struct rel_relation *rel);
/**<
	Destroys a relation.
*/

ASC_DLLSPEC(void ) rel_write_name(slv_system_t sys, struct rel_relation *rel, FILE *file);
/**<
	Writes a name to the file given. Handles NULL inputs gracefully.
	Does not print any whitespace, including carriage returns.
	Is faster than slv_print_var_name.
	If sys is NULL, writes full ascend name. If file or rel is NULL
	does not write.
*/

/** Relation filter structure */
typedef struct rel_filter_structure {
  uint32 matchbits;   /**< Bits to match. */
  uint32 matchvalue;  /**< Value to match. */
} rel_filter_t;
/**< Relation filter type */

ASC_DLLSPEC(SlvBackendToken) rel_instance(struct rel_relation *rel);
/**<
	Returns the instance pointer from a rel.
*/

extern void rel_set_extnodeinfo(struct rel_relation *rel,
                                struct rel_extnode *nodeinfo);
/**< Sets the external node information structure for a relation. */
extern struct rel_extnode *rel_extnodeinfo(struct rel_relation *rel);
/**<
	Fetches the pointer to the external node information structure for
	a relation. If this is NULL, which will be the case for most
	relations, then there are no external call nodes present.
*/

extern void rel_set_extwhichvar(struct rel_relation *rel, int whichvar);
/**< Sets the argument list index for a relation having external nodes. */

unsigned long rel_extwhichvar(struct rel_relation *rel);
/**<
	Returns the index into the argument list from which rel was
	constructed.  This applies ONLY to rels that have external nodes!
	Relations that have external nodes have associated with them an
	index into the argument list from which the relation was constructed.
	rel_whichvar returns that index. e.g. if when this relation was
	constructed from an external procedure call, the number of output
	variables was 4, and the number of inputs was 6, valid results from
	this rel_whichvar would be 7, 8, 9, 10.

	@return index into argument list, or a value <= 0 on error.
*/

extern boolean rel_less(struct rel_relation *rel);
/**<
	Returns true if the given relation is satisfied if "less than"
	is among those that make up the comparator of the relation
	(i.e. <>, <, or <=).<br><br>
	le==TRUE implies rel would be satisfied if lhs < rhs
*/

extern boolean rel_equal(struct rel_relation *rel);
/**<
	Returns true if the given relation is satisfied if "equals"
	is among those that make up the comparator of the relation
	(i.e. =, >=, or <=).<br><br>
	eq==TRUE implies rel would be satisfied if lhs ~ rhs
*/
extern boolean rel_greater(struct rel_relation *rel);
/**<
	Returns true if the given relation is satisfied if "greater than"
	is among those that make up the comparator of the relation
	(i.e. <>, >, or >=).<br><br>
	gr==TRUE implies rel would be satisfied if lhs > rhs
*/

extern enum rel_enum rel_relop(struct rel_relation *rel);
/**<
	Returns the type of the relational operator of a given relation.
*/

ASC_DLLSPEC(char *) rel_make_name(slv_system_t sys, struct rel_relation *rel);
/**<
	Copies of the relation instance name can be made and returned.
	The string returned should be freed when no longer in use.
*/

extern int32 rel_mindex(struct rel_relation *rel);
/**<
	Retrieves the index number of the given relation as it
	appears in a slv_system_t master relation list.
*/
extern void rel_set_mindex(struct rel_relation *rel, int32 index);
/**<
	Sets the index number of the given relation as it
	appears in a slv_system_t master relation list.
*/

ASC_DLLSPEC(int32 ) rel_sindex(const struct rel_relation *rel);
/**<
	Retrieves the index number of the given relation as it
	appears in a solvers relation list. The index is most often used
	to assign the relation to a specific original row of a matrix.
*/

extern void rel_set_sindex(struct rel_relation *rel, int32 index);
/**<
	Sets the index number of the given relation as it
	appears in a solvers relation list. The index is most often used
	to assign the relation to a specific original row of a matrix.
*/

extern int32 rel_model(const struct rel_relation *rel);
/**<
	Retrieves the model number of the given relation.
	In a hierarchy, relations come in groups associated with
	models. Models are numbered from 1 to some upper limit.
*/
extern void rel_set_model(struct rel_relation *rel, int32 index);
/**<
	Sets the model number of the given relation.
	In a hierarchy, relations come in groups associated with
	models. Models are numbered from 1 to some upper limit.
*/

ASC_DLLSPEC(real64) rel_residual(struct rel_relation *rel);
/**<
	Retrieves the residual field of the given relation.
	Note that the residual is not actually computed by rel_residual:
	there is no guarantee (from this function) that the residual is
	actually correct.
*/

ASC_DLLSPEC(void) rel_set_residual(struct rel_relation *rel, real64 residual);
/**<
	Sets the residual field of the given relation.
*/

ASC_DLLSPEC(real64 ) rel_nominal(struct rel_relation *rel);
/**<
	Retrieves the nominal field of the given relation.
	No slv client has any business being able to set the nominal,
	so no such operator is provided.
*/	

extern void rel_set_nominal(struct rel_relation *rel, real64 nominal);
/**<
	Breaking the 'rule' of rel_nominal() for the time being.
	@todo Remove rel_set_nominal() or change "rule" stated by rel_nominal().
*/


#ifdef NDEBUG
# define rel_n_incidences(rel) ((rel)->n_incidences)
#else
ASC_DLLSPEC(int32) rel_n_incidencesF(struct rel_relation *rel);
# define rel_n_incidences(rel) rel_n_incidencesF(rel)
#endif
/**<
 *  Returns the length of the incidence_list for a relation.
 *  Not everything in the incidence list is necessarily a
 *  variable for your particular solver -- check the flags.
 *  @param rel struct rel_relation *, the relation to query.
 *  @return Returns the length as an int32.
 *  @see rel_n_incidencesF()
 */

#ifdef NDEBUG
# define rel_set_incidences(rel,n,ilist) \
  (rel)->n_incidences=(n); (rel)->incidence = (ilist)
#else
# define rel_set_incidences(rel,n,ilist) rel_set_incidencesF((rel),(n),(ilist))
#endif
/**<
 *  Sets the length and incidence_list for a relation.
 *  Not everything in the incidence list is necessarily a
 *  variable for your particular solver -- check the flags.
 *  Solver clients should not call rel_set_incidences,
 *  it is only for use by constructors of bridges to relation
 *  back ends.
 *  @param rel   struct rel_relation *, the relation to modify.
 *  @param n     int32
 *  @param ilist struct var_variable **, the incidence list
 *  @return No return value.
 *  @see rel_set_incidencesF()
 */
extern void rel_set_incidencesF(struct rel_relation *rel,
		int32 n, struct var_variable **ilist);
/**<
 *  Implementation function for rel_set_incidences().  Do not call
 *  this function directly - use rel_set_incidences() instead.
 */


ASC_DLLSPEC(struct var_variable **)
rel_incidence_list_to_modify(struct rel_relation *rel);
/**<
	Returns a non-const pointer to an array rel_n_incidences(rel)
	long of vars.
	@see rel_incidence_list().
*/
ASC_DLLSPEC(const struct var_variable**) rel_incidence_list(struct rel_relation *rel);
/**<
	Returns a pointer to an array rel_n_incidences(rel) long of vars.
	Each element of the array is a struct var_variable *.
	Check the var sindex to see where each might go in a jacobian.
	If there is no incidence, NULL is returned.

	Pointers in this array will be unique.
	The list belongs to the relation. Do not destroy it. Do not change it.
	
	The returned array is *not* null-terminated.
*/

/*-----------------------------------------------------------------------------
  RELATION FILTERING FUNCTIONS AND FLAG GET/SET FUNCTIONS

	We have a lot (32) of binary (one bit) flags a client may want to query
	in arbitrary combinations and paying attention to only certain of
	the bits. We will provide a set of macros and functions for each of
	these bits and for operations on the whole set.
*/

ASC_DLLSPEC(int32 ) rel_apply_filter(const struct rel_relation *rel,
                              rel_filter_t *filter);
/**<
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the relation's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.<br><br>
 *
 *  EXAMPLE:
 *  To find out if a relation is an unsatisfied equality:
 *    We don't care about anything except satisfaction and equality,
 *    so set filter->matchbits = (REL_SATISFIED | REL_EQUALITY);
 *    We want relation NOT satisfied, so leave REL_SATISFIED = 0
 *    in filter->matchvalue, i.e. filter->matchvalue = REL_EQUALITY;
 *  Usually it is most convenient to just declare a rel_filter_t filt;
 *  locally in a function and then take the address of it (&filt) after
 *  setting filt.matchbits = (| of all your interesting bits) and
 *  setting filt.matchvalue = (| of the bits you want to be TRUE).
 */

extern uint32 rel_flags(struct rel_relation *rel);
/**<
 * Returns the flags field of the relation.
 */
extern void rel_set_flags(struct rel_relation *rel, uint32 flags);
/**<
 *  Sets the entire flag field to the value of flags given.
 */

ASC_DLLSPEC(uint32) rel_flagbit(struct rel_relation *rel, uint32 name);
/**<
 *  Returns the value of the bit specified from the relation flags.
 *  name should be a REL_xx flag defined above)
 */

ASC_DLLSPEC(void ) rel_set_flagbit(struct rel_relation *rel,
                            uint32 NAME, uint32 oneorzero);
/**<
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 *  <pre>
 *  Example:
 *    rel_set_flags(rel,REL_PARTITION,1) turns on the REL_PARTITION bit.
 *
 *    What it really does is:
 *      if (oneorzero) {
 *        rel->flags |= field;
 *      } else {
 *        rel->flags &= ~field;
 *      }
 *  </pre>
 *  In unix, see also man 3f bit or man not.
 */

/*
 *  the bit flags. explained afterward. several are for use of
 *  transient clients and should be ignored by solver engines
 */
#define REL_PARTITION 0x1
/**< Is it in the interesting region? Reordering clients. */
#define REL_TORN      0x2
/**<  Is it a tear? Reordering clients output. */
#define REL_INTERFACE 0x4
/**< User suggests it's a tear eqn. Solvers, ui clients. */
#define REL_INCLUDED  0x8
/**<
 *  User wants eqn in problem.  Solvers, ui clients.
 *  Use rel_set_* to change.
 *  INCLUDED is as yet a funny one. Treat it as readonly because
 *  you can only change it using a real function and not the
 *  bit manipulation functions. It is here in the bits because
 *  it is useful in filters sometimes.
 */
#define REL_OBJNEGATE 0x10
/**<  Read_only for clients.  Rel module. */
#define REL_BLACKBOX  0x20
/**<  Read_only for clients.  Rel module. */
#define REL_SATISFIED 0x40
/**<
 *  Has rel been pronounced satisfied by someone?
 *  Bit should be treated as readonly. use rel_set_* to change.
 */
#define REL_EQUALITY  0x80
/**< Is relation an equality?  Readonly for clients. */
#define REL_INBLOCK   0x100
/**< Is the relation in the current block of registered client?  For clients. */
/* Conditional Modeling */
#define REL_INWHEN      0x200
/**< Is relation in a when?  Readonly for clients. */
#define REL_ACTIVE      0x400
/**< Is this relation currently a part of my problem? */
#define REL_INVARIANT   0x800
/**< Is this relation an invariant in the conditional modeling analysis? */
/* Conditional Relations (Boundaries) */
#define REL_CONDITIONAL       0x1000
/**< Is relation conditional?  Readonly for clients. */
#define REL_IN_CUR_SUBREGION  0x2000
/**< Is the relation in the subregion currently analyzed?  */
#define REL_GENERATED 0x10000
/**<
 *  Temporary relation that doesn't exist independently in the backend,
 *  but is made by some process of the backend or the solver client.
 *  Is rel fake and cooked up for this system only? */

/*
 * the bit flag lookups
 */
#ifdef NDEBUG
#define rel_partition(rel)        ((rel)->flags & REL_PARTITION)
#define rel_torn(rel)             ((rel)->flags & REL_TORN)
#define rel_interface(rel)        ((rel)->flags & REL_INTERFACE)
#define rel_obj_negate(rel)       ((rel)->flags & REL_OBJNEGATE)
/**<
 *  Returns TRUE if relation is of type e_maximize.
 *  Returns FALSE if relation is of type e_minimize.
 *  Note: should only be used on objectives. other relations
 *  will give a meaningless result (probably FALSE).
 */
#define rel_blackbox(rel)         ((rel)->flags & REL_BLACKBOX)
#define rel_satisfied(rel)        ((rel)->flags & REL_SATISFIED)
/**< Retrieves the satisfied field of the given relation.
 * See rel_residual() for disclaimer. */
#define rel_equality(rel)         ((rel)->flags & REL_EQUALITY)
/**<  TRUE if relation is of form lhs = rhs. */
#define rel_in_block(rel)         ((rel)->flags & REL_INBLOCK)
/**< Retrieves the INBLOCK member of the given relation flags
 *  which determines if the relation is within the current block. */
#define rel_in_when(rel)          ((rel)->flags & REL_INWHEN)
#define rel_active(rel)           ((rel)->flags & REL_ACTIVE)
#define rel_invariant(rel)        ((rel)->flags & REL_INVARIANT)
#define rel_conditional(rel)      ((rel)->flags & REL_CONDITIONAL)
#define rel_in_cur_subregion(rel) ((rel)->flags & REL_IN_CUR_SUBREGION)
#define rel_generated(rel)        ((rel)->flags & REL_GENERATED)
#else
#define rel_partition(rel)        rel_flagbit((rel),REL_PARTITION)
#define rel_torn(rel)             rel_flagbit((rel),REL_TORN)
#define rel_interface(rel)        rel_flagbit((rel),REL_INTERFACE)
#define rel_obj_negate(rel)       rel_flagbit((rel),REL_OBJNEGATE)
/**<
 *  Returns TRUE if relation is of type e_maximize.
 *  Returns FALSE if relation is of type e_minimize.
 *  Note: should only be used on objectives. other relations
 *  will give a meaningless result (probably FALSE).
 */
#define rel_blackbox(rel)         rel_flagbit((rel),REL_BLACKBOX)
#define rel_satisfied(rel)        rel_flagbit((rel),REL_SATISFIED)
/**< Retrieves the satisfied field of the given relation.
 * See rel_residual() for disclaimer. */
#define rel_equality(rel)         rel_flagbit((rel),REL_EQUALITY)
/**<  TRUE if relation is of form lhs = rhs. */
#define rel_in_block(rel)         rel_flagbit((rel),REL_INBLOCK)
/**< Retrieves the INBLOCK member of the given relation flags
 *  which determines if the relation is within the current block. */
#define rel_in_when(rel)          rel_flagbit((rel),REL_INWHEN)
#define rel_active(rel)           rel_flagbit((rel),REL_ACTIVE)
#define rel_invariant(rel)        rel_flagbit((rel),REL_INVARIANT)
#define rel_conditional(rel)      rel_flagbit((rel),REL_CONDITIONAL)
#define rel_in_cur_subregion(rel) rel_flagbit((rel),REL_IN_CUR_SUBREGION)
#define rel_generated(rel)        rel_flagbit((rel),REL_GENERATED)
#endif /* NDEBUG */

/*
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
 * Many of these bits have read-only semantics for clients.
 * Take care when setting them.
 */
#define rel_set_partition(rel,bitval)   rel_set_flagbit((rel),REL_PARTITION,(bitval))
#define rel_set_torn(rel,bitval)        rel_set_flagbit((rel),REL_TORN,(bitval))
#define rel_set_interface(rel,bitval)   rel_set_flagbit((rel),REL_INTERFACE,(bitval))
#define rel_set_obj_negate(rel,bitval)  rel_set_flagbit((rel),REL_OBJNEGATE,(bitval))
#define rel_set_blackbox(rel,bitval)    rel_set_flagbit((rel),REL_BLACKBOX,(bitval))
#define rel_set_satisfied(rel,bitval)   rel_set_flagbit((rel),REL_SATISFIED,(bitval))
#define rel_set_equality(rel,bitval)    rel_set_flagbit((rel),REL_EQUALITY,(bitval))
#define rel_set_in_block(rel,bitval)    rel_set_flagbit((rel),REL_INBLOCK,(bitval))
#define rel_set_in_when(rel,bitval)     rel_set_flagbit((rel),REL_INWHEN,(bitval))
#define rel_set_active(rel,bitval)      rel_set_flagbit((rel),REL_ACTIVE,(bitval))
#define rel_set_invariant(rel,bitval)   rel_set_flagbit((rel),REL_INVARIANT,(bitval))
#define rel_set_conditional(rel,bitval)  \
        rel_set_flagbit((rel),REL_CONDITIONAL,(bitval))
#define rel_set_in_cur_subregion(rel,bitval)  \
        rel_set_flagbit((rel),REL_IN_CUR_SUBREGION,(bitval))
#define rel_set_generated(rel,bitval)		rel_set_flagbit((rel),REL_GENERATED,(bitval))

ASC_DLLSPEC(uint32) rel_included(struct rel_relation *rel);
/**<
 *  Retrieves the included field of the given relation.
 *  @todo This has side effects on the ascend instance, so it isn't
 *        implemented with the rest of the macros above. This needs to
 *        change.
 */
extern void rel_set_included(struct rel_relation *rel, uint32 included);
/**<
 *  Sets the included field of the given relation.
 *  <!--  This has side effect on the ascend instance, so it isn't     -->
 *  <!--  implemented with the rest of the macros above. This needs to -->
 *  <!--  change.                                                      -->
 */

extern real64 rel_multiplier(struct rel_relation *rel);
/**<
 *  Retrieves the multiplier field of the given relation.
 *  This is expected to be computed as a lagrange multiplier which will
 *  relate the gradient of the relation rel with that of some objective
 *  function.
 */
extern void rel_set_multiplier(struct rel_relation *rel, real64 multiplier);
/**<
 *  (<!--  won a temporary reprieve. this should be a system property, not rel.) -->
 *  Sets the multiplier field of the given relation.
 *  This is expected to be computed as a lagrange multiplier which will
 *  relate the gradient of the relation rel with that of some objective
 *  function.
 */

/*------------------------------------------------------------------------------
  SERVER-SIDE UTILITY FUNCTIONS

	Things for the server side only. Not visible to clients.

	Ok, really nosy clients courting death can cheat. Don't cry when
	they break.
	We make absolutely no commitment to being compatible with this portion
	of the header at any time in the future.
*/
#ifdef _SLV_SERVER_C_SEEN_

extern double g_external_tolerance; /**< DEFAULT 1e-12 */

/* - - - - - - - - - - - - -
	EXTERNAL RELATION CACHE STUFF
	this wasn't originally documented, so I've added my own understanding
	of all this stuff here... feel free to revise... -- JP
*/

void extrel_store_input_vars(struct rel_relation *rel);
/**<
	Convert the ExtRelCache 'inputlist' into a list of var_variable pointers
	inside the ExtRElCAche object.
*/

extern struct ExtRelCache *rel_extcache(struct rel_relation *rel);
/**<
	Retrieve external relation information.

	This is the gateway to the external relation information
	stashed away to make processing of external relations efficient.
	See the file extrel.[ch] for functions to deal with the external
	relations cache, unless of course you are a client in which CASE
	don't.

	@NOTE This applies ONLY to rels that have external nodes! Ensure this by
	calling rel_extnodeinfo FIRST !
*/

extern void rel_set_extcache(struct rel_relation *rel, struct ExtRelCache *cache);
/**<
	Set external relation information.

	This is the gateway to the external relation information
	stashed away to make processing of external relations efficient.
	See the file extrel.[ch] for functions to deal with the external
	relations cache, unless of course you are a client in which CASE
	don't.

	@NOTE This applies ONLY to rels that have external nodes! Ensure this by 
	calling rel_extnodeinfo FIRST!
*/

extern struct ExtRelCache *CreateExtRelCache(struct ExtCallNode *ext);
/**<
	Create an external relation cache from a given ExtCallNode. Copies over
	all the pointers inside the ExtCallNode object, but also allocates its own
	memory for the input and output arrays and jacobian.

	A pointer to the 'arglist' used by the ExtCallNode is put in the ExtRelCache
	and also a linearised list of the inputs is put in the
	'inputlist' of the ExtRelCache (and this list is owned by the ExtRelCache).
*/

struct ExtRelCache *CreateCacheFromInstance(SlvBackendToken relinst);

extern void ExtRel_DestroyCache(struct ExtRelCache *cache);
/**<
	Destory an ExtRelCache object. Mostly we just throw away the pointers that
	we have. The exception is the 'inputlist', which we own and must destroy
	here.
*/

extern int32 ExtRel_PreSolve(struct ExtRelCache *cache, int32 setup);
/**<
	Presolve an external relation. This allows the external relation to
	initialise its internal data if required, before the main solver
	iterations begin. -- JP.

	To deal with the first time we also want to do argument
	checking, and then turn off the first_func_eval flag.
	Turn on the evaluation_required flag. The rationale behind this is
	as follows:
	
	The solver at the moment does not treat an external relation
	specially, i.e., as a block. It also calls for its functions
	a relation at a time. However the external relations compute
	all their outputs at once. So as not to do unnecessary
	recalculations we use these flag bits. We set evaluation_required
	initially to true, so as to force *at least* one calculation
	of the external relations. By similar reasoning first_func_eval (done)
	is set to false.

	@TODO Question: isn't the init function going to get called repeatedly here
	for blackboxes with multiple outputs? -- JP
*/

extern real64 ExtRel_Evaluate_Residual(struct rel_relation *rel);
/**<
	Evaluate the external relation and return the required residual, but only
	if evaluation is required. This means that current inputs are compared with
	previous values to ensure that at least one has really changed.

	Renamed from ExtRel_Evaluate_RHS (and *_LHS removed, because it was just
	a placeholder).
*/

extern real64 ExtRel_Evaluate_RHS(struct rel_relation *rel);
/**<
	Evaluate the relation RHS, which is the externally-computed value of the
	output for the current value of the inputs.
*/

extern real64 ExtRel_Evaluate_LHS(struct rel_relation *rel);
/**<
	Evaluate the relation LHS, which is always current value of the output
	as retreived from the model.
*/

/**
	Evaluate the residual as well as Jacobian elements for a given
	external relation object
*/
double extrel_resid_and_jacobian(struct rel_relation *rel
	, var_filter_t *filter, int32 row, mtx_matrix_t mtx
);

#endif /* _SLV_SERVER_C_SEEN_ */

/* removed some dead stuff here -- JP  */

#endif /* ASC_REL_H  */
