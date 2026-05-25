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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @defgroup system_cond System Conditional
	This is the ascend version of the conditional module
	This version should be used by any user who receives
	his/her equations directly from an instance tree created
	by the ASCEND compiler.
*//*
	by Vicente Rico-Ramirez, 09/96
	Last in CVS: $Revision: 1.7 $ $Date: 1998/03/30 22:06:53 $ $Author: rv2a $
*/

#ifndef ASC_CONDITIONAL_H
#define ASC_CONDITIONAL_H

#include <ascend/general/platform.h>
#include <ascend/general/list.h>
#include <ascend/compiler/compiler.h>

#include "slv_types.h"

struct Expr;
struct module_t;

/**	@addtogroup system_cond
	@{
*/

/**
 * @todo I have to get rid of this
 */
#ifndef MAX_VAR_IN_LIST
#define MAX_VAR_IN_LIST 20
#endif  /* MAX_VAR_IN_LIST */

/** When data structure. */
struct w_when {
  SlvBackendToken instance;    /**< the associated ascend ATOM  */
  struct gl_list_t *dvars;     /**< index of dis vars  */
  struct gl_list_t *cases;     /**< when_case's */
  int32 num_cases;             /**< number of cases in the WHEN */
  int32 sindex;
  int32 mindex;
  int32 model;		             /**< index of a hypothetical MODEL when is from */
  uint32 flags;
};
/*
 * if you mess with the above struct, change the defaults for it in .c file
 */

/*
 *                        When functions
 */

ASC_DLLSPEC struct w_when *when_create(SlvBackendToken instance,
                                  struct w_when *newwhen);
/**<
 *  Creates a when given the when instance.
 *  If the when supplied is NULL, we allocate the memory for the
 *  when we return, else we just init the memory you hand us and
 *  return it to you.<br><br>
 *  We set the fields instance. Setting the rest of the information
 *  is the job of the bridge building function between the ascend
 *  instance tree (or other when back end) and the slv_system_t.
 */

extern SlvBackendToken when_instance(struct w_when *when);
/**<
 *  Returns the instance pointer from a when.
 */

extern void when_write_name(slv_system_t sys,
                            struct w_when *when,
                            FILE *file);
/**<
 *  Writes a name to the file given. Handles
 *  If sys is NULL, writes full ascend name. If file or when is NULL
 *  does not write.
 */

extern void when_destroy_cases(struct w_when *when);
/**<
 *  Destroys a the list of cases of a when.
 */

ASC_DLLSPEC void when_destroy(struct w_when *when);
/**<
 *  Destroys a when.
 */

ASC_DLLSPEC char *when_make_name(slv_system_t sys, struct w_when *when);
/**<
 *  Copies of the when instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 */

extern struct gl_list_t *when_dvars_list( struct w_when *when);
/**< Retrieves the list of dis variables of the given when. */
extern void when_set_dvars_list( struct w_when *when,
                                 struct gl_list_t *dvlist);
/**<
 *  Sets the list of dis variables of the given when.
 */

ASC_DLLSPEC struct gl_list_t *when_cases_list( struct w_when *when);
/**< Retrieves the list of cases of the given when. */
extern void when_set_cases_list( struct w_when *when,
                                 struct gl_list_t *clist);
/**<
 *  Sets the list of cases of the given when.
 */

extern int32 when_num_cases(struct w_when *when);
/**< Retrieves the number of cases of the given when as it
 *  appears in a slv_system_t master when list. */
extern void when_set_num_cases(struct w_when *when, int32 num_cases);
/**<
 *  Sets the number of cases of the given when as it
 *  appears in a slv_system_t master when list.
 */

extern int32 when_mindex(struct w_when *when);
/**< Retrieves the index number of the given when as it
 *  appears in a slv_system_t master when list. */
extern void when_set_mindex(struct w_when *when, int32 mindex);
/**<
 *  Sets the index number of the given when as it
 *  appears in a slv_system_t master when list.
 */

extern int32 when_sindex(struct w_when *when);
/**< Retrieves the index number of the given relation as it
 *  appears in a solvers when list. */
extern void when_set_sindex(struct w_when *when, int32 sindex);
/**<
 *  Sets the index number of the given relation as it
 *  appears in a solvers when list.
 */

extern int32 when_model(const struct w_when *when);
/**< Retrieves the model number of the given when.
 *  In a hierarchy, whens come associated with
 *  models. Models are numbered from 1 to some upper limit. */
extern void when_set_model(struct w_when *when, int32 mindex);
/**<
 *  Sets the model number of the given when.
 *  In a hierarchy, whens come associated with
 *  models. Models are numbered from 1 to some upper limit.
 */

/** when filter structure. */
typedef struct when_filter_structure {
  uint32 matchbits;   /**< Bits to match. */
  uint32 matchvalue;  /**< Value to match. */
} when_filter_t;
/**< when filter type */

/*
 * when filtration functions.
 * We could have 32 binary (one bit) flags a client may want to query
 * in arbitrary combinations and paying attention to only certain of
 * the bits. We will provide a set of macros and functions for each of
 * these bits and for operations on the whole set.
 */

extern int32 when_apply_filter(struct w_when *when, const when_filter_t *filter);
/**<
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the when's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */

extern uint32 when_flags(struct w_when *when);
/**< Returns the flags field of the when. */
extern void when_set_flags(struct w_when *when, uint32 flags);
/**<
 * Sets the entire flag field to the value of flags given.
 */

extern uint32 when_flagbit(struct w_when *when, uint32 name);
/**<
 *  Returns the value of the bit specified from the when flags.
 *  name should be a WHEN_xx flag defined above)
 */

extern void when_set_flagbit(struct w_when *when,
                             uint32 NAME, uint32 oneorzero);
/**<
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.<br><br>
 *
 *  Example:                                                       <pre>
 *    when_set_flags(when,WHEN_INCLUDED,1) turns on the WHEN_INCLUDED bit.
 *    What it really does is:
 *      if (oneorzero) {
 *        when->flags |= field;
 *      } else {
 *        when->flags &= ~field;
 *      }
 *                                                                 </pre>
 *  In unix, see also man 3f bit or man not.
 */

/*
 * the bit flags.
 */
#define WHEN_INWHEN             0x1
/**<  Is this when nested in another when? */
#define WHEN_INCLUDED           0x2
/**<
 *  Wser wants when in problem.  Bit should be treated as
 *  readonly. use when_set_* to change.   solvers, ui clients.
 */
#define WHEN_VISITED            0x4
/**< Required for conditional analysis, for avoiding to reanalyze a WHEN */
#define WHEN_CLASSIFIER_GUARD_DVARS 0x10
/**< Classifier WHEN has generated Boolean guard dvars for solver analysis. */
#define WHEN_CHANGES_STRUCTURE  0x8
/**<
 *  Required for conditional analysis. Tells if the sutructure of
 *  the different CASES is all equal or otherwise.
 */

/*
 * the bit flag lookups
 */
#ifdef NDEBUG
#define when_inwhen(when)    ((when)->flags & WHEN_INWHEN)
#define when_included(when)  ((when)->flags & WHEN_INCLUDED)
#define when_visited(when)   ((when)->flags & WHEN_VISITED)
#define when_changes_structure(when) ((when)->flags & WHEN_CHANGES_STRUCTURE)
#else
#define when_inwhen(when)    when_flagbit((when),WHEN_INWHEN)
#define when_included(when)  when_flagbit((when),WHEN_INCLUDED)
#define when_visited(when)   when_flagbit((when),WHEN_VISITED)
#define when_changes_structure(when) when_flagbit((when),WHEN_CHANGES_STRUCTURE)
#endif /* NDEBUG */

/*
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
 */
#define when_set_inwhen(when,bv)   \
               when_set_flagbit((when),WHEN_INWHEN,(bv))
#define when_set_included(when,bv)   \
               when_set_flagbit((when),WHEN_INCLUDED,(bv))
#define when_set_visited(when,bv)   \
               when_set_flagbit((when),WHEN_VISITED,(bv))
#define when_set_changes_structure(when,bv)   \
               when_set_flagbit((when),WHEN_CHANGES_STRUCTURE,(bv))

/*
 *                  When Case  utility functions
 */

/** when case data structure */
struct when_case {
  int32 values[MAX_VAR_IN_LIST];  /**< values of conditional variables */
  const struct Expr *condition;    /**< optional CASE ... IF classifier guard */
  const struct Expr *applies;      /**< optional APPLIES IF region predicate */
  symchar *otherwise_label;        /**< optional label for OTHERWISE cases */
  struct Expr *region;             /**< solver-side lowered region predicate */
  int32 region_source;             /**< enum when_region_source */
  struct module_t *source_module;   /**< source module for user diagnostics */
  unsigned long source_line;        /**< source line for user diagnostics */
  struct gl_list_t *rels;         /**< pointer to relations */
  struct gl_list_t *logrels;      /**< pointer to logrelations */
  struct gl_list_t *whens;        /**< pointer to whens */
  struct gl_list_t *reinits;      /**< pointer to REINIT actions */
  int32 case_number;              /**< number of case */
  int32 num_rels;                 /**< number of relations */
  int32 num_inc_var;              /**< number of incident variables */
  int32 *ind_inc;                 /**< master indeces of incidences */
  uint32 flags;                   /**< flags ?? */
};

struct when_reinit {
  SlvBackendToken target;         /**< resolved target instance */
  const struct Expr *rhs;         /**< RHS expression evaluated at event time */
  const struct Expr *guard;       /**< optional guard expression evaluated at event time */
};

enum when_region_request {
  WHEN_REGION_STEADY = 1,         /**< region predicates for steady conditional solving */
  WHEN_REGION_DYNAMIC_CLASSIFIER  /**< region predicates for inferred dynamic tracking */
};

enum when_region_source {
  WHEN_REGION_NONE = 0,
  WHEN_REGION_APPLIES,
  WHEN_REGION_CASE_IF,
  WHEN_REGION_CASE_IF_OTHERWISE
};

struct when_guard_materialization {
  int32 guard_booleans;           /**< logical guard slots in compact CASE IF encoding */
  int32 reusable_named_guards;     /**< CASE IF guards that can reuse a simple named Boolean */
  int32 real_boundaries;          /**< natural real relation boundaries in guards */
  int32 logical_boundaries;       /**< SATISFIED/logical boundary terms in guards */
  int32 boolean_ops;              /**< Boolean/logical operators in guards */
  int32 unsupported_dynamic_terms; /**< pre/der terms not suitable for steady CMSlv lowering */
  int32 hidden_boolean_instances; /**< generated Boolean atom instances needed by instance-backed lowering */
  int32 hidden_relation_instances; /**< generated relation instances needed for real boundaries */
  int32 hidden_logrel_instances;   /**< generated logrelation instances needed for guard definitions */
  int32 requires_generated_artifacts; /**< nonzero when generated solver-side artifacts are needed */
};

struct when_guard_artifact {
  int32 guard_index;               /**< zero-based CASE IF guard slot */
  const struct when_case *source_case; /**< source case owning this guard */
  const struct Expr *guard;        /**< source guard expression, not owned */
  int32 reuse_existing_boolean;    /**< nonzero if guard itself is a named Boolean */
  int32 generated_boolean;         /**< nonzero if a helper guard Boolean is needed */
  int32 real_boundaries;           /**< natural real relation boundaries in this guard */
  int32 logical_boundaries;        /**< SATISFIED/logical boundary terms in this guard */
  int32 boolean_ops;               /**< Boolean operators in this guard */
  int32 reusable_named_terms;      /**< named Boolean terms reusable inside this guard */
  int32 unsupported_dynamic_terms; /**< pre/der terms in this guard */
  struct module_t *source_module;  /**< source module for diagnostics */
  unsigned long source_line;       /**< source line for diagnostics */
};

extern struct when_reinit *when_reinit_create(struct when_reinit *newreinit);
extern void when_reinit_destroy(struct when_reinit *wr);
extern SlvBackendToken when_reinit_target(const struct when_reinit *wr);
extern void when_reinit_set_target(struct when_reinit *wr, SlvBackendToken target);
extern const struct Expr *when_reinit_rhs(const struct when_reinit *wr);
extern void when_reinit_set_rhs(struct when_reinit *wr, const struct Expr *rhs);
extern const struct Expr *when_reinit_guard(const struct when_reinit *wr);
extern void when_reinit_set_guard(struct when_reinit *wr, const struct Expr *guard);

ASC_DLLSPEC struct when_case *when_case_create(struct when_case *newcase);
/**<
 *  Creates a when case.
 *  If the case supplied is NULL, we allocate the memory for the
 *  case we return, else we just init the memory you hand us and
 *  return it to you.
 */

extern void when_case_destroy(struct when_case *wc);
/**<
 *  Destroys a when case.
 */

ASC_DLLSPEC int32 *when_case_values_list( struct when_case *wc);
/**< Retrieves the list of values of the given case. */
extern void when_case_set_values_list( struct when_case *wc, int32 *vallist);
/**<
 *  Sets the list of values of the given case.
 */

ASC_DLLSPEC const struct Expr *when_case_condition(const struct when_case *wc);
/**< Retrieves the optional CASE ... IF classifier guard of the given case. */
ASC_DLLSPEC void when_case_set_condition(struct when_case *wc, const struct Expr *condition);
/**<
 *  Sets the optional CASE ... IF classifier guard of the given case.
 */

ASC_DLLSPEC const struct Expr *when_case_applies(const struct when_case *wc);
/**< Retrieves the optional APPLIES IF region predicate of the given case. */
ASC_DLLSPEC void when_case_set_applies(struct when_case *wc, const struct Expr *applies);
/**<
 *  Sets the optional APPLIES IF region predicate of the given case.
 */

ASC_DLLSPEC symchar *when_case_otherwise_label(const struct when_case *wc);
/**< Retrieves the optional label attached to an OTHERWISE case. */
ASC_DLLSPEC void when_case_set_otherwise_label(struct when_case *wc, symchar *label);
/**<
 *  Sets the optional label attached to an OTHERWISE case.
 */

ASC_DLLSPEC int when_case_has_classifier_predicate(const struct when_case *wc);
/**< Returns nonzero if the case has CASE IF or APPLIES IF metadata. */

ASC_DLLSPEC int when_has_classifier_predicates(const struct w_when *when);
/**< Returns nonzero if this WHEN or any nested active/inactive case has classifier predicates. */

ASC_DLLSPEC const struct Expr *when_case_region_predicate(const struct when_case *wc);
/**< Retrieves the solver-side lowered region predicate, if any. */

ASC_DLLSPEC int32 when_case_region_source(const struct when_case *wc);
/**< Retrieves the enum when_region_source value for the lowered predicate. */

ASC_DLLSPEC void when_case_clear_region_predicate(struct when_case *wc);
/**< Clears and destroys any solver-side lowered region predicate. */

ASC_DLLSPEC void when_case_set_region_predicate(struct when_case *wc,
                                           struct Expr *region,
                                           int32 source);
/**<
 *  Sets the solver-side lowered region predicate.
 *  Ownership of region is transferred to wc.
 */

ASC_DLLSPEC struct module_t *when_case_source_module(const struct when_case *wc);
/**< Retrieves the source module for diagnostics, if available. */

ASC_DLLSPEC unsigned long when_case_source_line(const struct when_case *wc);
/**< Retrieves the source line for diagnostics, or zero if unavailable. */

ASC_DLLSPEC void when_case_set_source(struct when_case *wc,
                                      struct module_t *module,
                                      unsigned long line);
/**< Sets source provenance for diagnostics. */

ASC_DLLSPEC int when_lower_classifier_regions(struct w_when *when,
                                         enum when_region_request request);
/**<
 *  Derives per-case region predicates from CASE IF or APPLIES IF metadata.
 *
 *  This mutates only the solver-side w_when/when_case presentation created in
 *  slv_system_t. It does not modify the compiler instance tree or its Case
 *  objects. Solvers should call this explicitly when they are prepared to
 *  consume lowered classifier-region semantics.
 *
 *  Returns 0 on success, nonzero on unsupported or inconsistent metadata.
 */

ASC_DLLSPEC int when_case_if_guard_count(const struct w_when *when,
                                         int32 *nguards);
/**<
 *  Counts the ordered guard expressions in a CASE IF cascade. This is the
 *  compact Boolean encoding of the cascade: each guard is a natural generated
 *  Boolean, and cases are represented as wildcard tuple patterns over those
 *  guards.
 *
 *  Returns 0 on success and nonzero if the WHEN is not a pure CASE IF cascade.
 */

ASC_DLLSPEC const struct Expr *when_case_if_guard(const struct w_when *when,
                                                 int32 guard_index);
/**<
 *  Retrieves guard_index from the compact CASE IF guard list. Indexing is
 *  zero-based. The returned expression is owned by the when_case and must not be
 *  destroyed by the caller.
 */

ASC_DLLSPEC int when_case_if_pattern(const struct w_when *when,
                                     const struct when_case *wc,
                                     int32 *values,
                                     int32 *nvalues);
/**<
 *  Computes the old-style WHEN(bool,...) wildcard tuple for a case in a CASE IF
 *  cascade. TRUE is 1, FALSE is 0, and ANY/don't-care is -2.
 *
 *  For example:
 *    CASE A IF g1      -> TRUE, *
 *    CASE B IF g2      -> FALSE, TRUE
 *    OTHERWISE         -> FALSE, FALSE
 *
 *  Returns 0 on success and nonzero if the WHEN is not a pure CASE IF cascade
 *  or the case is not a member of the WHEN.
 */

ASC_DLLSPEC int when_case_if_materialization_plan(
                                     const struct w_when *when,
                                     struct when_guard_materialization *plan);
/**<
 *  Computes the generated-object shape needed to materialize a CASE IF cascade
 *  as old CMSlv-style Boolean/logrel/boundary machinery while preserving
 *  natural guard boundaries.
 *
 *  Each CASE IF guard becomes one generated Boolean. Real relational operators
 *  found inside those guards are counted as natural real boundaries; SATISFIED
 *  and Boolean equality/inequality terms are counted as logical boundaries.
 *
 *  Returns 0 on success and nonzero if the WHEN is not a pure CASE IF cascade.
 */

ASC_DLLSPEC struct when_guard_artifact *when_guard_artifact_create(
                                     const struct w_when *when,
                                     int32 guard_index);
/**<
 *  Creates a descriptor for one CASE IF guard slot.
 *
 *  The returned artifact owns no compiler expressions or cases; it only points
 *  back to the source guard and source case. Destroy it with
 *  when_guard_artifact_destroy().
 */

ASC_DLLSPEC void when_guard_artifact_destroy(
                                     struct when_guard_artifact *artifact);
/**< Destroys a guard artifact descriptor. */

ASC_DLLSPEC struct gl_list_t *when_case_if_artifacts_create(
                                     const struct w_when *when);
/**<
 *  Creates a gl_list of struct when_guard_artifact*, one per CASE IF guard.
 *  Returns NULL if the WHEN is not a pure CASE IF cascade.
 */

ASC_DLLSPEC void when_case_if_artifacts_destroy(struct gl_list_t *artifacts);
/**< Destroys a list returned by when_case_if_artifacts_create(). */

extern struct gl_list_t *when_case_rels_list( struct when_case *wc);
/**< Retrieves the list of rels of the given case. */
extern void when_case_set_rels_list(struct when_case *wc,
                                    struct gl_list_t *rlist);
/**<
 *  Sets the list of rels of the given case.
 */

extern struct gl_list_t *when_case_logrels_list( struct when_case *wc);
/**< Retrieves the list of logrels of the given case. */
extern void when_case_set_logrels_list(struct when_case *wc,
                                       struct gl_list_t *lrlist);
/**<
 *  Sets the list of logrels of the given case.
 */

extern struct gl_list_t *when_case_whens_list( struct when_case *wc);
/**<  Retrieves the list of whens nested in the given case. */
extern void when_case_set_whens_list( struct when_case *wc,
                                      struct gl_list_t *wlist);
/**<
 *  Sets the list of whens nested in the given case.
 */

extern struct gl_list_t *when_case_reinits_list(struct when_case *wc);
/**< Retrieves the list of REINIT actions of the given case. */
extern void when_case_set_reinits_list(struct when_case *wc,
                                       struct gl_list_t *rlist);
/**<
 *  Sets the list of REINIT actions of the given case.
 */

extern int32 when_case_case_number(struct when_case *wc);
/**< Returns the number of the case wc. */
extern void when_case_set_case_number(struct when_case *wc, int32 case_number);
/**<
 * Sets the case_number field of a case to the value of the case_number given.
 */

extern int32 when_case_num_rels( struct when_case *wc);
/**< Returns the number of the relations in wc. */
extern void when_case_set_num_rels(struct when_case *wc, int32 num_rels);
/**<
 * Sets the num_rels field of a case to the value of the num_rels given.
 */

extern int32 when_case_num_inc_var( struct when_case *wc);
/**< Returns the number of the variables incident
 * in the relations contained in wc. */
extern void when_case_set_num_inc_var(struct when_case *wc, int32 num_inc_var);
/**<
 * Sets the num_inc_var field of a case to the value of the num_inc_var given.
 */

extern int32 *when_case_ind_inc( struct when_case *wc);
/**< Returns the array of the master indices of the
 *  variables incident in the relations contained in wc. */
extern void when_case_set_ind_inc(struct when_case *wc, int32 *ind_inc);
/**<
 * Sets the ind_inc pointer of a case to the value of the array ind_inc given.
 */

/** when case filter structure */
typedef struct when_case_filter_structure {
  uint32 matchbits;   /**< Bits to match. */
  uint32 matchvalue;  /**< Value to match. */
} when_case_filter_t;
/**< when case filter type */

/*
 * case filtration functions
 */

extern int32 when_case_apply_filter(struct when_case *wc,
                                    const when_case_filter_t *filter);
/**<
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the when's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */

extern uint32 when_case_flags(struct when_case *wc);
/**< Returns the flags field of the case wc. */
extern void when_case_set_flags(struct when_case *wc, uint32 flags);
/**<
 * Sets the entire flag field to the value of flags given.
 */

extern uint32 when_case_flagbit(struct when_case *wc, uint32 name);
/**<
 *  Returns the value of the bit specified from the case flags.
 *  name should be a WHEN_CASE_xx flag
 */

extern void when_case_set_flagbit(struct when_case *wc,
                                  uint32 NAME, uint32 oneorzero);
/**<
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 */

/*
 * the bit flags.
 */

#define WHEN_CASE_ACTIVE        0x1
/**<  Is this case active?  */

#ifdef NDEBUG
#define when_case_active(case) ((case)->flags & WHEN_CASE_ACTIVE)
#else
#define when_case_active(case) when_case_flagbit((case),WHEN_CASE_ACTIVE)
#endif
/**<
 *  Returns TRUE if case has it WHEN_CASE_ACTIVE bit flag set.
 *  @param case struct when_case *, the case to query.
 *  @return Returns TRUE if the bit is set, FALSE otherwise.
 */

#define when_case_set_active(case,bv)   \
               when_case_set_flagbit((case),WHEN_CASE_ACTIVE,(bv))
/**<
 *  Sets the WHEN_CASE_ACTIVE bit flag of case to bv.
 *  Any value other than 0 for bv turns the bit flag to 1.
 *  0 sets it to 0.
 *  @param case struct when_case *, the case to modify.
 *  @param bv   uint32, the new value for the flag (1 or 0).
 *  @return No return value.
 */

/* @} */

#endif  /* ASC_CONDITIONAL_H */
