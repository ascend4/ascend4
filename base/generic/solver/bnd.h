/* 
 *  Boundary Module
 *  Created: 04/97
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: bnd.h,v $
 *  Date last modified: $Date: 1997/07/18 12:13:55 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
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
 */

/** @file
 *  Boundary Module.
 *  <pre>
 *  Description: This is the ascend version of the boundary module.
 *               This version should be used by any user who receives
 *               his/her formulation directly from an instance tree
 *               created by the ASCEND compiler.
 *
 *  Requires:    #include "utilities/ascConfig.h"
 *               #include "base.h"
 *               #include "rel.h"
 *               #include "logrel.h"
 *  </pre>
 *  @todo Remove reference to base.h?  Appears to have been refactored away.
 */

#ifndef bnd__already_included
#define bnd__already_included

/** Need a better way of doing this, dynamic allocation */
#define MAXNUM_OF_SAT_TERMS 5

/** The condition is a rel or a logrel */
enum bnd_enum {
  e_bnd_rel,
  e_bnd_logrel,
  e_bnd_undefined
};

union bnd_union { 
  struct rel_relation *relbnd;
  struct logrel_relation *logrelbnd;
};

struct bnd_boundary {
   enum bnd_enum kind;         /**< real or logical condition */
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
 *  <!--  bnd_create(bnd)                                              -->
 *  <!--  bnd = bnd_create(NULL)                                       -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *
 *  Creates a boundary.
 *  If the bnd supplied is NULL, we allocate the memory for the
 *  bnd we return, else we just init the memory you hand us and
 *  Setting the the information is the job
 *  of the bridge building function between the ascend instance
 *  tree and the slv_system_t.
 */

extern void bnd_destroy(struct bnd_boundary *bnd);
/**<
 *  <!--  bnd_destroy(bnd)                                             -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *
 *  Destroys a boundary.
 */

extern void bnd_set_kind(struct bnd_boundary *bnd, enum bnd_enum kind);
/**<
 * Sets the type of the condition of the given boundary.
 */
extern enum bnd_enum bnd_kind(struct bnd_boundary *bnd);
/**<
 * <!--  kind = bnd_kind(bnd);                                         -->
 * <!--  bnd_enum kind;                                                -->
 * <!--  struct bnd_boundary *bnd;                                     -->
 * Retrieves the type of the condition of the given boundary.
 */

#define bnd_log_cond(b)  ((b)->cond.logrelbnd)
#define bnd_real_cond(b) ((b)->cond.relbnd)
#define bnd_logrel(b)    ((struct logrel_relation *)(b))
#define bnd_rel(b)       ((struct rel_relation *)(b))

extern void bnd_set_logrels(struct bnd_boundary *bnd,
                            struct gl_list_t *logrels);
/**< Set the list of pointers to the logrels using the condition. */
extern struct gl_list_t *bnd_logrels(struct bnd_boundary *bnd);
/**<
 *  <!--  logrels = bnd_logrels(bnd)                                   -->
 *  <!--  struct bnd_logrels *bnd;                                     -->
 *  <!--  struct gl_list_t *logrels;                                   -->
 *
 *  Retrieves the list of pointers to the logrels using  the condition.
 */

extern void bnd_set_tolerance(struct bnd_boundary *bnd, real64 tolerance);
/**<  
 *  Set the tolerance used to determine the truth value
 *  of the condition. Assume only one sucha a value. If different
 *  values, this needs a FIX.
 */
extern real64 bnd_tolerance(struct bnd_boundary *bnd);
/**<
 *  <!--  tolerance = bnd_tolerance(bnd)                               -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *  <!--  real64 tolerance;                                            -->
 *
 *  Retrieves the tolerance used to determine the truth value
 *  of the condition. Assume only one such a value. If different
 *  values, this needs a FIX.
 */

extern char *bnd_make_name(slv_system_t sys, struct bnd_boundary *bnd);
/**<
 *  <!--  name = bnd_make_name(sys,bnd)                                -->
 *  <!--  slv_system_t sys;                                            -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *  <!--  char *name;                                                  -->
 *
 *  Copies of the condition instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 */

extern int32 bnd_mindex(struct bnd_boundary *bnd);
/**<
 *  Retrieves the index number of the given boundary as it
 *  appears in a slv_system_t master boundary list.
 */
extern void bnd_set_mindex(struct bnd_boundary *bnd, int32 index);
/**<
 *  <!--  index = bnd_mindex(bnd)                                      -->
 *  <!--  bnd_set_mindex(bnd,index)                                    -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *
 *  Sets the index number of the given boundary as it
 *  appears in a slv_system_t master boundary list.  
 */

extern int32 bnd_sindex(const struct bnd_boundary *bnd);
/**<
 *  Retrieves the index number of the given boundary as it
 *  appears in a solvers boundary list.
 */
extern void bnd_set_sindex(struct bnd_boundary *bnd, int32 index);
/**< 
 *  <!--  index = bnd_sindex(bnd)                                      -->
 *  <!--  bnd_set_sindex(bnd,index)                                    -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *
 *  Sets the index number of the given boundary as it
 *  appears in a solvers boundary list.
 */
    
extern int32 bnd_model(const struct bnd_boundary *bnd);
/**<
 *  Retrieves the model number of the given boundary.
 *  In a hierarchy, boundaries come in groups associated with
 *  models. Models are numbered from 1 to some upper limit.
 */
extern void bnd_set_model(struct bnd_boundary *bnd, int32 index);
/**<
 *  <!--  index = bnd_model(bnd)                                       -->
 *  <!--  bnd_set_model(bnd,index)                                     -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *
 *  Sets the model number of the given boundary.
 *  In a hierarchy, boundaries come in groups associated with
 *  models. Models are numbered from 1 to some upper limit.
 */

extern struct var_variable **bnd_real_incidence(struct bnd_boundary *bnd);
/**< 
 * Get the list of variables incident in the relation constituting
 * the boundary. 
 */
 
extern int32 bnd_n_real_incidences(struct bnd_boundary *bnd);
/**< 
 * Get the number of variables incident in the relation constituting
 * the boundary. 
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
 *  <!--  value = bnd_apply_filter(bnd,filter)                         -->
 *  <!--  int value;                                                   -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *  <!--  bnd_filter_t *filter;                                        -->
 *
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the boundary's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */

extern unsigned int bnd_flags(struct bnd_boundary *bnd);
/**<  Returns the flags field of the boundary. */
extern void bnd_set_flags(struct bnd_boundary *bnd, uint32 flags);
/**<
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *  <!--  uint32 flags;                                                -->
 *
 * Sets the entire flag field to the value of flags given.
 */

extern uint32 bnd_flagbit(struct bnd_boundary *bnd, uint32 name);
/**<
 *  <!--  bnd_flagbit(bnd,name);                                       -->
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *  <!--  uint32 name;                                                 -->
 *  Returns the value of the bit specified from the boundary flags.
 *  name should be a BND_xx flag defined above)
 */

extern void bnd_set_flagbit(struct bnd_boundary *bnd,
                            uint32 NAME, uint32 oneorzero);
/**<
 *  <!--  struct bnd_boundary *bnd;                                    -->
 *  <!--  unsigned int NAME,oneorzero;                                 -->
 *  <!--  bnd_set_flagbit(bnd,NAME,oneorzero)                          -->
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 */

/** the bit flags.*/
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
/**
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

/**
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
 * Returns an int32 (0 or 1) instead of an arbitrary nonzero
 * or zero value. It is silly, but it makes easier to deal with comparisons
 * between the current and the previous status of a boundary.
 */

#endif /* bnd__already_included */

