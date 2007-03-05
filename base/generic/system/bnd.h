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
*//* @file
	Boundary Module

	This is the ascend version of the boundary module.
	This version should be used by any user who receives
	his/her formulation directly from an instance tree
	created by the ASCEND compiler.
*//*
	Created: 04/97
	Last in CVS: $Revision: 1.10 $ $Date: 1997/07/18 12:13:55 $ $Author: mthomas $
*/

#ifndef ASC_BND_H
#define ASC_BND_H

#include <utilities/ascConfig.h>

#include "slv_types.h"
#include "rel.h"
#include "logrel.h"

/**	@addtogroup system System
	@{
*/

/** Type of condition held by a boundary. */
enum bnd_enum {
  e_bnd_rel,        /**< real condition */
  e_bnd_logrel,     /**< logical condition */
  e_bnd_undefined   /**< undefined condition type */
};

/** Union of boundary condition structures. */
union bnd_union {
  struct rel_relation *relbnd;        /**< real condition */
  struct logrel_relation *logrelbnd;  /**< logical condition */
};

/** Boundary data structure. */
struct bnd_boundary {
   enum bnd_enum kind;         /**< type of condition - real or logical */
   union bnd_union cond;       /**< condition */
   struct gl_list_t *logrels;  /**< logrel using the truth value of condition */
   real64 tolerance;           /**< assume a unique and consistent value for the
                                    tolerance. Used only when the condition is a
                                    rel_relation. Needs FIX for arbitrary values */
   int32 mindex;               /**< index in the slv_system_t master list */
   int32 sindex;               /**< index in the slv_system_t solver list */
   int32 model;	               /**< index of a hypothetical MODEL bnd is from */
   uint32 flags;               /**< flags */
};

extern struct bnd_boundary *bnd_create(struct bnd_boundary *bnd);
/**<
 *  Creates a new boundary.
 *  If the bnd supplied is NULL, new memory will be allocated for 
 *  the returned bnd.  If non-NULL, the memory will be initialized
 *  and returned.   Setting the the information is the job of the
 *  bridge building function between the ascend instance tree and
 *  the slv_system_t.  When finished with it, the caller should 
 *  bnd_destroy() and ascfree() the new boundary.
 */

extern void bnd_destroy(struct bnd_boundary *bnd);
/**<  
 *  Deallocates any memory held internally by the specified boundary.
 *  Nothing is done if bnd is NULL.
 */

extern void bnd_set_kind(struct bnd_boundary *bnd, enum bnd_enum kind);
/**<  
 *  Sets the type of the specified boundary.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern enum bnd_enum bnd_kind(struct bnd_boundary *bnd);
/**<
 *  Retrieves the type of the specified boundary.
 *  bnd may not be NULL (checked by asc_assertion).
 */

#define bnd_log_cond(b)  ((b)->cond.logrelbnd)
/**<
 *  Retrieves the condition of a logrel boundary.
 *  @param b The boundary to query (struct bnd_boundary *)
 *  @return The boundary's condition as a struct logrel_relation *.
 */
#define bnd_real_cond(b) ((b)->cond.relbnd)
/**<
 *  Retrieves the condition of a rel boundary.
 *  @param b The boundary to query (struct bnd_boundary *)
 *  @return The boundary's condition as a struct rel_relation *.
 */
#define bnd_logrel(b)    ((struct logrel_relation *)(b))
/**<
 *  Casts a pointer to logrel condition pointer.
 *  @param b The pointer to cast to (struct logrel_relation *).
 *  @return No return value.
 */
#define bnd_rel(b)       ((struct rel_relation *)(b))
/**<
 *  Casts a pointer to rel condition pointer.
 *  @param b The pointer to cast to (struct rel_relation *).
 *  @return No return value.
 */

extern void bnd_set_logrels(struct bnd_boundary *bnd,
                            struct gl_list_t *logrels);
/**< 
 *  Sets the list of pointers to the logrels using the condition.
 *  logrels is a gl_list of (struct logrel_relation *). 
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern struct gl_list_t *bnd_logrels(struct bnd_boundary *bnd);
/**<
 *  Retrieves the list of pointers to the logrels using the condition.
 *  logrels is a gl_list of (struct logrel_relation *).
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern void bnd_set_tolerance(struct bnd_boundary *bnd, real64 tolerance);
/**<
 *  Set the tolerance used to determine the truth value
 *  of the condition.  The current implimentation allows a single 
 *  tolerance.  If different values are required, this needs a FIX.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern real64 bnd_tolerance(struct bnd_boundary *bnd);
/**<
 *  Retrieves the tolerance used to determine the truth value
 *  of the condition. The current implimentation allows a single 
 *  tolerance.  If different values are required, this needs a FIX.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern char *bnd_make_name(slv_system_t sys, struct bnd_boundary *bnd);
/**<
 *  Copies of the condition instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 *  Returns NULL if either sys or bnd are NULL.
 */

extern int32 bnd_mindex(struct bnd_boundary *bnd);
/**<
 *  Retrieves the index of the given boundary as it
 *  appears in a slv_system_t master boundary list.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern void bnd_set_mindex(struct bnd_boundary *bnd, int32 mindex);
/**<
 *  Sets the index of the given boundary as it
 *  appears in a slv_system_t master boundary list.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern int32 bnd_sindex(const struct bnd_boundary *bnd);
/**<
 *  Retrieves the index of the given boundary as it
 *  appears in a solvers boundary list.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern void bnd_set_sindex(struct bnd_boundary *bnd, int32 sindex);
/**<
 *  Sets the index of the given boundary as it
 *  appears in a solvers boundary list.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern int32 bnd_model(const struct bnd_boundary *bnd);
/**<
 *  Retrieves the model number of the given boundary.
 *  In a hierarchy, boundaries come in groups associated with
 *  models. Models are numbered from 1 to some upper limit.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern void bnd_set_model(struct bnd_boundary *bnd, int32 mindex);
/**<
 *  Sets the model number of the given boundary.
 *  In a hierarchy, boundaries come in groups associated with
 *  models. Models are numbered from 1 to some upper limit.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern struct var_variable **bnd_real_incidence(struct bnd_boundary *bnd);
/**<
 *  Returns a pointer to an array of variables incident in the 
 *  relation constituting the boundary.  The returned list is kept 
 *  by the relation - do not destroy or change it in any way.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern int32 bnd_n_real_incidences(struct bnd_boundary *bnd);
/**<
 *  Gets the number of variables incident in the relation
 *  constituting the boundary.  This will be the length of the
 *  array pointed to by the return value of bnd_real_incidence().
 *  bnd may not be NULL (checked by asc_assertion).
 */

/*
 * Boundary filtration functions.
 * We have a lot (32) of binary (one bit) flags a client may want to query
 * in arbitrary combinations and paying attention to only certain of
 * the bits. We will provide a set of macros and functions for each of
 * these bits and for operations on the whole set.
 */

/** Boundary filter structure. */
typedef struct bnd_filter_structure {
  uint32 matchbits;   /**< Bits to match. */
  uint32 matchvalue;  /**< Value to match. */
} bnd_filter_t;
/**< Boundary filter type. */

extern int bnd_apply_filter(const struct bnd_boundary *bnd,
                            bnd_filter_t *filter);
/**<
 *  Applies a filter to a boundary.
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the boundary's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 *  Returns 0 if either bnd or filter is NULL.
 */

extern unsigned int bnd_flags(struct bnd_boundary *bnd);
/**<
 *  Returns the flags field of the boundary.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern void bnd_set_flags(struct bnd_boundary *bnd, uint32 flags);
/**<
 *  Sets the entire flag field to the value of flags given.
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern uint32 bnd_flagbit(struct bnd_boundary *bnd, uint32 name);
/**<
 *  Returns the value of the bit specified from the boundary flags.
 *  name should be a BND_xx flag defined below).
 *  bnd may not be NULL (checked by asc_assertion).
 */

extern void bnd_set_flagbit(struct bnd_boundary *bnd,
                            uint32 NAME, uint32 oneorzero);
/**<
 *  Sets a flag bit for a boundary.
 *  Sets the bit, which should be referred to by its macro 
 *  name, on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 *  bnd may not be NULL (checked by asc_assertion).
 */

/* the bit flags.*/
#define BND_REAL         0x1    /**< is the boundary a real relation ? */
#define BND_IN_LOGREL    0x2    /**< is the boundary used in some logical relation ? */
#define BND_EQUALITY     0x4    /**< is the boundary an equality? readonly for clients. */
#define BND_AT_ZERO      0x8    /**< Am I at the "zero" of a conditional boundary ? */
#define BND_CUR_STATUS  0x10    /**< Is the Boundary boundary currently satisfied ? */
#define BND_PRE_STATUS  0x20    /**< Was the Boundary boundary satisfied in the previous
                                     iteration ? (The last two help to answer the query
                                     "Was the boundary crossed ?") */
#define BND_CROSSED     0x40    /**< Was the boundary crossed ? */
#define BND_PERTURB     0x80    /**< Should I perturb this boundary in the calculation
                                     of logical variables ? */
/*
 * the bit flag lookups
 */
#ifdef NDEBUG
#define bnd_real(b)        ((b)->flags & BND_REAL)
#define bnd_in_logrel(b)   ((b)->flags & BND_IN_LOGREL)
#define bnd_equality(b)    ((b)->flags & BND_EQUALITY)
#define bnd_at_zero(b)     ((b)->flags & BND_AT_ZERO)
#define bnd_cur_status(b)  ((b)->flags & BND_CUR_STATUS)
#define bnd_pre_status(b)  ((b)->flags & BND_PRE_STATUS)
#define bnd_crossed(b)     ((b)->flags & BND_CROSSED)
#define bnd_perturb(b)     ((b)->flags & BND_PERTURB)
#else
#define bnd_real(b)        bnd_flagbit((b),BND_REAL)
#define bnd_in_logrel(b)   bnd_flagbit((b),BND_IN_LOGREL)
#define bnd_equality(b)    bnd_flagbit((b),BND_EQUALITY)
#define bnd_at_zero(b)     bnd_flagbit((b),BND_AT_ZERO)
#define bnd_cur_status(b)  bnd_flagbit((b),BND_CUR_STATUS)
#define bnd_pre_status(b)  bnd_flagbit((b),BND_PRE_STATUS)
#define bnd_crossed(b)     bnd_flagbit((b),BND_CROSSED)
#define bnd_perturb(b)     bnd_flagbit((b),BND_PERTURB)
#endif /* NDEBUG */

/*
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
 */
#define bnd_set_real(b,bv)           bnd_set_flagbit((b),BND_REAL,(bv))
#define bnd_set_in_logrel(b,bv)      bnd_set_flagbit((b),BND_IN_LOGREL,(bv))
#define bnd_set_equality(b,bv)       bnd_set_flagbit((b),BND_EQUALITY,(bv))
#define bnd_set_at_zero(b,bv)        bnd_set_flagbit((b),BND_AT_ZERO,(bv))
#define bnd_set_cur_status(b,bv)     bnd_set_flagbit((b),BND_CUR_STATUS,(bv))
#define bnd_set_pre_status(b,bv)     bnd_set_flagbit((b),BND_PRE_STATUS,(bv))
#define bnd_set_crossed(b,bv)        bnd_set_flagbit((b),BND_CROSSED,(bv))
#define bnd_set_perturb(b,bv)        bnd_set_flagbit((b),BND_PERTURB,(bv))

extern int32 bnd_status_cur(struct bnd_boundary *bnd);  /**< See bnd_status_pre(). */
extern int32 bnd_status_pre(struct bnd_boundary *bnd);
/**<
 *  Returns an int32 (0 or 1) instead of an arbitrary nonzero or
 *  zero value. It is silly, but it makes easier to deal with comparisons
 *  between the current and the previous status of a boundary.
 *  bnd may not be NULL (checked by asc_assertion).
 */

/* @} */

#endif /* ASC_BND_H */

