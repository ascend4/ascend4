/*	ASCEND modelling environment
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
	Logical Relation Module.

	This is the ASCEND version of the logrel module.  This
	version should be used by any user who receives his/her
	equations directly from an instance tree created by the
	ASCEND compiler.
*//*
	by Vicente Rico-Ramirez, 09/96
	Last in CVS: $Revision: 1.6 $ $Date: 1997/07/29 15:47:43 $ $Author: rv2a $
*/

#ifndef ASC_LOGREL_H
#define ASC_LOGREL_H

#include <stdio.h>
#include <utilities/ascConfig.h>
#include "discrete.h"
#include "slv_types.h"

/**	@addtogroup system System
	@{
*/

/* ********************************************************************\
  logrel_relation basic type operators.
\*********************************************************************/

/**
 * The operator of a logical relation.
 */
enum logrel_enum {
  e_logrel_equal,         /**< equality */
  e_logrel_not_equal      /**< not equality */
};

/** Logical relation data structure */
struct logrel_relation {
   SlvBackendToken instance;
   struct dis_discrete **incidence;
   int32 n_incidences;    /**< length of incidence */
   int32 mindex;          /**< index in the slv_system_t master list */
   int32 sindex;          /**< index in the slv_system_t solver list */
   int32 model;           /**< index of a hypothetical MODEL rel is from */
   uint32 flags;          /**< flags */
};
/*
 * if you mess with the above struct, change the defaults for it in .c file
 */

extern struct logrel_relation *logrel_create(SlvBackendToken instance,
                                             struct logrel_relation *newlogrel);
/**<
 *  Creates a logrelation given the logrelation instance.
 *  If the logrel supplied is NULL, we allocate the memory for the
 *  logrel we return, else we just init the memory you hand us and
 *  return it to you.
 *  We set the fields instance, nodeinfo.
 *  Setting the rest of the information is the job
 *  of the bridge building function between the ascend instance
 *  tree (or other logrelation back end) and the slv_system_t.
 *  In particular, the incidence list and indexing info is not
 *  handled here.
 */

extern SlvBackendToken logrel_instance(struct logrel_relation *logrel);
/**<
 *  Returns the instance pointer from a logrel.
 */

extern void logrel_write_name(slv_system_t sys,
                              struct logrel_relation *logrel,
                              FILE *file);
/**<
 *  Writes a name to the file given.
 *  If sys is NULL, writes full ascend name. If file or logrel is NULL
 *  does not write.
 */

extern void logrel_destroy(struct logrel_relation *logrel);
/**<
 *  Destroys a logrelation.
 */

extern boolean logrel_equal(struct logrel_relation *logrel);
/**<
 *  Returns true if the given logrelation would be
 *  satisfied when lhs == rhs.
 *  @see logrel_not_equal()
 */
extern boolean logrel_not_equal(struct logrel_relation *logrel);
/**<
 *  Returns true if the given logrelation would be
 *  satisfied when lhs != rhs.
 *  @see logrel_equal()
 */

extern enum logrel_enum logrel_relop(struct logrel_relation *logrel);
/**<
 * Returns the type of the operator of a given logrelation.
 */

extern char *logrel_make_name(slv_system_t sys,
                              struct logrel_relation *logrel);
/**<
 *  Copies of the logrelation instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 */

extern int32 logrel_mindex(struct logrel_relation *logrel);
/**<
 *  Retrieves the index number of the given logrelation as it
 *  appears in a slv_system_t master logrelation list.
 */
extern void logrel_set_mindex(struct logrel_relation *logrel,
                              int32 mindex);
/**<
 *  Sets the index number of the given logrelation as it
 *  appears in a slv_system_t master logrelation list.
 */

extern int32 logrel_sindex(struct logrel_relation *logrel);
/**<
 *  Retrieves the index number of the given logrelation as it
 *  appears in a solvers logrelation list.
 */
extern void logrel_set_sindex(struct logrel_relation *logrel,
                              int32 sindex);
/**<
 *  Sets he index number of the given logrelation as it
 *  appears in a solvers logrelation list.
 */

extern int32 logrel_model(const struct logrel_relation *logrel);
/**<
 *  Retrieves the model number of the given logrelation.
 *  Models are numbered from 1 to some upper limit.
 */
extern void logrel_set_model(struct logrel_relation *logrel,
                             int32 mindex);
/**<
 *  Sets the model number of the given logrelation.
 *  Models are numbered from 1 to some upper limit.
 */

extern int32 logrel_residual(struct logrel_relation *logrel);
/**<
 *  Retrieves the logical residual field of the given logrelation.
 *  Note that the residual is not actually computed by logrel_residual:
 *  there is no guarantee (from this function) that the residual is
 *  actually correct.
 */
ASC_DLLSPEC void logrel_set_residual(struct logrel_relation *logrel,
                                int32 residual);
/**<
 *  Sets the logical residual field of the given logrelation.
 *  <!--  Note that the residual is not actually computed by logrel_residual: -->
 *  <!--  there is no guarantee (from this function) that the residual is     -->
 *  <!--  actually correct.                                            -->
 */

extern int32 logrel_nominal(struct logrel_relation *logrel);
/**<
 *  Retrieves the nominal field of the given logrelation.
 *  No slv client has any business being able to set the nominal,
 *  so no such operator is provided.
 */

#ifdef NDEBUG
#define logrel_n_incidences(lr) ((lr)->n_incidences)
#else
#define logrel_n_incidences(lr) logrel_n_incidencesF(lr)
#endif
/**<
 *  Returns the length of the incidence_list.
 *  @param  lr  struct logrel_relation*, the incidence list to query.
 *  @return  The length as an int32.
 *  @see logrel_n_incidencesF()
 */

#ifdef NDEBUG
#define logrel_set_incidences(lr,n,ilist) \
   (lr)->n_incidences=(n); (lr)->incidence = (ilist)
#else
#define logrel_set_incidences(lr,n,ilist) \
   logrel_set_incidencesF((lr),(n),(ilist))
#endif
/**<
 *  Sets the length of the incidence_list.
 *  Solver clients should not call logrel_set_incidences(),
 *  it is only for use by constructors of bridges to logrelation
 *  back ends.
 *  @param  lr     struct logrel_relation*, the incidence list to modify.
 *  @param  n      int32, the new length.
 *  @param  ilist  struct dis_discrete**.
 *  @return  No return value.
 *  @see logrel_set_incidencesF()
 */

extern int32 logrel_n_incidencesF(struct logrel_relation *logrel);
/**<
 *  Implementation function for logrel_n_incidences() (debug mode).
 *  Do not call this function directly - use logrel_n_incidences() instead.
 */
extern void logrel_set_incidencesF(struct logrel_relation *logrel,
                                   int32 n,
                                   struct dis_discrete **ilist);
/**<
 *  Implementation function for logrel_set_incidences() (debug mode).
 *  Do not call this function directly - use logrel_set_incidences() instead.
 */

ASC_DLLSPEC const struct dis_discrete**logrel_incidence_list(
	struct logrel_relation *logrel
);
/**<
 *  Returns a pointer to an array logrel_n_incidences(logrel) long of bvars.
 *  Each element of the array is a struct dis_discrete *.
 *  If there is no incidence, NULL is returned.
 *  Pointers in this array will be unique.
 *  The list belongs to the logrelation. Do not destroy it.
 *  Do not change it.<br><br>
 *
 *  RETURNED LIST IS NOT NULL-TERMINATED.
 */
extern struct dis_discrete
**logrel_incidence_list_to_modify(struct logrel_relation *logrel);
/**<
 *  Returns a pointer to an array logrel_n_incidences(logrel) long of bvars.
 *  Each element of the array is a struct dis_discrete *.
 *  If there is no incidence, NULL is returned.
 *  Pointers in this array will be unique.
 *  The list belongs to the logrelation. Do not destroy it.
 *  Do not change it.
 *
 *  RETURNED LIST IS NOT NULL-TERMINATED.
 */

/*
 * logrelation filtration functions.
 * We have 32 binary (one bit) flags a client may want to query
 * in arbitrary combinations and paying attention to only certain of
 * the bits. We will provide a set of macros and functions for each of
 * these bits and for operations on the whole set.
 */

/** Logical relation filter structure */
typedef struct logrel_filter_structure {
  uint32 matchbits;   /**< Bits to match. */
  uint32 matchvalue;  /**< Value to match. */
} logrel_filter_t;
/**< logrel filter type */

extern int logrel_apply_filter(struct logrel_relation *logrel,
                               logrel_filter_t *filter);
/**<
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the logrelation's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */

extern uint32 logrel_flags(struct logrel_relation *logrel);
/**<
 * Returns the flags field of the logrelation.
 */
extern void logrel_set_flags(struct logrel_relation *logrel, uint32 flags);
/**<
 * Sets the entire flag field to the value of flags given.
 */

extern uint32 logrel_flagbit(struct logrel_relation *logrel, uint32 name);
/**<
 *  Returns the value of the bit specified from the logrelation flags.
 *  name should be a LOGREL_xx flag defined above)
 */

extern void logrel_set_flagbit(struct logrel_relation *logrel,
                               uint32 NAME, uint32 oneorzero);
/**<
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 *
 *  Example:                                                 <pre>
 *     logrel_set_flags(logrel,LOGREL_INCLUDED,1) turns on the
 *         LOGREL_INCLUDED bit.
 *     What it really does is:
 *          if (oneorzero) {
 *            logrel->flags |= field;
 *          } else {
 *            logrel->flags &= ~field;
 *          }                                                </pre>
 *
 *  In unix, see also man 3f bit or man not.
 */

/*-------------------------
  THE BIT FLAGS.

  Several are for use of transient clients
  and should be ignored by solver engines
*/

#define LOGREL_INCLUDED      0x1
/**<
 * User wants eqn in problem (solvers, ui clients).
 * Bit should be treated as readonly. use logrel_set_* to change.<br><br>
 *
 * INCLUDED is as yet a funny one. treat it as readonly because
 * you can only change it using a int function and not the
 * bit manipulation functions. It is here in the bits because
 * it is useful in filters sometimes.
 */

#define LOGREL_SATISFIED     0x2
/**<
 * Has logrel been pronounced satisfied by someone?
 * Bit should be treated as readonly. use logrel_set_* to change.
 */

#define LOGREL_EQUALITY      0x4
/**<* Is logrelation an equality? Readonly for clients. */

/* Conditional Modeling */

#define LOGREL_INWHEN        0x8
/**< Is logrelation in a when? Readonly for clients. */

#define LOGREL_ACTIVE       0x10
/**< Is this logrelation currently a part of my problem? */

/* Conditional LogRelations (Boundaries) */

#define LOGREL_CONDITIONAL  0x20
/**< Is logrelation conditional? Readonly for clients. */

#define LOGREL_IN_BLOCK     0x40
/**<
 * Is the logrelation in the current block of registered client?
 * for clients.
 */

/*-------------------------
  BIT FLAG LOOKUPS
*/

#ifdef NDEBUG
#define logrel_satisfied(lr)    ((lr)->flags & LOGREL_SATISFIED)
#define logrel_equality(lr)     ((lr)->flags & LOGREL_EQUALITY)
#define logrel_in_when(lr)      ((lr)->flags & LOGREL_INWHEN)
#define logrel_active(lr)       ((lr)->flags & LOGREL_ACTIVE)
#define logrel_conditional(lr)  ((lr)->flags & LOGREL_CONDITIONAL)
#define logrel_in_block(lr)     ((lr)->flags & LOGREL_IN_BLOCK)
#else
#define logrel_satisfied(lr)    logrel_flagbit((lr),LOGREL_SATISFIED)
#define logrel_equality(lr)     logrel_flagbit((lr),LOGREL_EQUALITY)
#define logrel_in_when(lr)      logrel_flagbit((lr),LOGREL_INWHEN)
#define logrel_active(lr)       logrel_flagbit((lr),LOGREL_ACTIVE)
#define logrel_conditional(lr)  logrel_flagbit((lr),LOGREL_CONDITIONAL)
#define logrel_in_bolck(lr)     logrel_flagbit((lr),LOGREL_IN_BLOCK)
#endif /* NDEBUG */

/*-------------------------
  BIT FLAG ASSIGNMENTS.
  Any value other than 0 for bv turns the
  named flag to 1. 0 sets it to 0.
*/

#define logrel_set_satisfied(lr,bv)   \
        logrel_set_flagbit((lr),LOGREL_SATISFIED,(bv))
#define logrel_set_equality(lr,bv)    \
        logrel_set_flagbit((lr),LOGREL_EQUALITY,(bv))
#define logrel_set_in_when(lr,bv)   \
        logrel_set_flagbit((lr),LOGREL_INWHEN,(bv))
#define logrel_set_active(lr,bv)   \
        logrel_set_flagbit((lr),LOGREL_ACTIVE,(bv))
#define logrel_set_conditional(lr,bv)    \
        logrel_set_flagbit((lr),LOGREL_CONDITIONAL,(bv))
#define logrel_set_in_block(lr,bv)    \
        logrel_set_flagbit((lr),LOGREL_IN_BLOCK,(bv))

ASC_DLLSPEC uint32 logrel_included(struct logrel_relation *logrel);
/**<
 *  Retrieves the included field of the given logrelation.
 *  This has side effect on the ascend instance, so it isn't
 *  implemented with the rest of the macros above. This needs to
 *  change.
 *  @todo  Modify logrel_included() per comment in solver/logrel.h?
 */

extern void logrel_set_included(struct logrel_relation *logrel,
                                uint32 included);
/**<
 *  Sets the included field of the given logrelation.
 *  This has side effect on the ascend instance, so it isn't
 *  implemented with the rest of the macros above. This needs to
 *  change.
 *  @todo  Modify logrel_set_included() per comment in solver/logrel.h?
 */

/* @} */

#endif  /* ASC_LOGREL_H  */

