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
	Conditional Module

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

#include <utilities/ascConfig.h>
#include <general/list.h>

#include "slv_types.h"

/**	@addtogroup system System
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

extern struct w_when *when_create(SlvBackendToken instance,
                                  struct w_when *newwhen);
/**<
 *  <!--  when_create(instance,newwhen)                                -->
 *  <!--  struct w_when *newwhen;                                      -->
 *  <!--  SlvBackendToken instance;                                    -->
 *
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
 *  <!--  when_instance(when)                                          -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Returns the instance pointer from a when.
 */

extern void when_write_name(slv_system_t sys,
                            struct w_when *when,
                            FILE *file);
/**<
 *  <!--  when_write_name(sys,when,file);                              -->
 *
 *  Writes a name to the file given. Handles
 *  If sys is NULL, writes full ascend name. If file or when is NULL
 *  does not write.
 */

extern void when_destroy_cases(struct w_when *when);
/**<
 *  <!--  when_destroy_cases(when)                                     -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Destroys a the list of cases of a when.
 */

extern void when_destroy(struct w_when *when);
/**<
 *  <!--  when_destroy(when)                                           -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Destroys a when.
 */

extern char *when_make_name(slv_system_t sys, struct w_when *when);
/**<
 *  <!--  name = when_make_name(sys,when)                              -->
 *  <!--  slv_system_t sys;                                            -->
 *  <!--  struct w_when *when;                                         -->
 *  <!--  char *name;                                                  -->
 *
 *  Copies of the when instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 */

extern struct gl_list_t *when_dvars_list( struct w_when *when);
/**< Retrieves the list of dis variables of the given when. */
extern void when_set_dvars_list( struct w_when *when,
                                 struct gl_list_t *dvlist);
/**<
 *  <!--  dvars = when_dvars_list(when)                                -->
 *  <!--  when_set_dvars_list(when,dvlist)                             -->
 *  <!--  struct gl_list_t *dvlist;                                    -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Sets the list of dis variables of the given when.
 */

extern struct gl_list_t *when_cases_list( struct w_when *when);
/**< Retrieves the list of cases of the given when. */
extern void when_set_cases_list( struct w_when *when,
                                 struct gl_list_t *clist);
/**<
 *  <!--  cases = when_cases_list(when)                                -->
 *  <!--  when_set_cases_list(when,clist)                              -->
 *  <!--  struct gl_list_t *clist;                                     -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Sets the list of cases of the given when.
 */

extern int32 when_num_cases(struct w_when *when);
/**< Retrieves the number of cases of the given when as it
 *  appears in a slv_system_t master when list. */
extern void when_set_num_cases(struct w_when *when, int32 num_cases);
/**<
 *  <!--  num_cases = when_num_cases(when)                             -->
 *  <!--  when_set_num_cases(when,num_cases)                           -->
 *  <!--  int32 num_cases;                                             -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Sets the number of cases of the given when as it
 *  appears in a slv_system_t master when list.
 */

extern int32 when_mindex(struct w_when *when);
/**< Retrieves the index number of the given when as it
 *  appears in a slv_system_t master when list. */
extern void when_set_mindex(struct w_when *when, int32 mindex);
/**<
 *  <!--  index = when_mindex(when)                                    -->
 *  <!--  when_set_mindex(when,index)                                  -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Sets the index number of the given when as it
 *  appears in a slv_system_t master when list.
 */

extern int32 when_sindex(struct w_when *when);
/**< Retrieves the index number of the given relation as it
 *  appears in a solvers when list. */
extern void when_set_sindex(struct w_when *when, int32 sindex);
/**<
 *  <!--  index = when_sindex(when)                                    -->
 *  <!--  when_set_sindex(when,index)                                  -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct w_when *when;                                         -->
 *
 *  Sets the index number of the given relation as it
 *  appears in a solvers when list.
 */

extern int32 when_model(const struct w_when *when);
/**< Retrieves the model number of the given when.
 *  In a hierarchy, whens come associated with
 *  models. Models are numbered from 1 to some upper limit. */
extern void when_set_model(struct w_when *when, int32 mindex);
/**<
 *  <!--  index = when_model(when)                                     -->
 *  <!--  when_set_model(when,index)                                   -->
 *  <!--  int32 index;                                                 -->
 *  <!--  struct w_when *when;                                         -->
 *
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

extern int32 when_apply_filter(struct w_when *when, when_filter_t *filter);
/**<
 *  <!--  value = when_apply_filter(when,filter)                       -->
 *  <!--  int32 value;                                                 -->
 *  <!--  struct w_when *when;                                         -->
 *  <!--  when_filter_t *filter;                                       -->
 *
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the when's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */

extern uint32 when_flags(struct w_when *when);
/**< Returns the flags field of the when. */
extern void when_set_flags(struct w_when *when, uint32 flags);
/**<
 *  <!--  struct w_when *when;                                         -->
 *  <!--  uint32 flags;                                                -->
 *
 * Sets the entire flag field to the value of flags given.
 */

extern uint32 when_flagbit(struct w_when *when, uint32 name);
/**<
 *  <!--  when_flagbit(when,name);                                     -->
 *  <!--  struct w_when *when;                                         -->
 *  <!--  uint32 name;                                                 -->
 *  Returns the value of the bit specified from the when flags.
 *  name should be a WHEN_xx flag defined above)
 */

extern void when_set_flagbit(struct w_when *when,
                             uint32 NAME, uint32 oneorzero);
/**<
 *  <!--  struct w_when *when;                                         -->
 *  <!--  uint32 NAME,oneorzero;                                       -->
 *  <!--  when_set_flagbit(when,NAME,oneorzero)                        -->
 *
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
  struct gl_list_t *rels;         /**< pointer to relations */
  struct gl_list_t *logrels;      /**< pointer to logrelations */
  struct gl_list_t *whens;        /**< pointer to whens */
  int32 case_number;              /**< number of case */
  int32 num_rels;                 /**< number of relations */
  int32 num_inc_var;              /**< number of incident variables */
  int32 *ind_inc;                 /**< master indeces of incidences */
  uint32 flags;                   /**< flags ?? */
};

extern struct when_case *when_case_create(struct when_case *newcase);
/**<
 *  <!--  when_case_create(newcase)                                    -->
 *  <!--  struct when_case *newcase;                                   -->
 *
 *  Creates a when case.
 *  If the case supplied is NULL, we allocate the memory for the
 *  case we return, else we just init the memory you hand us and
 *  return it to you.
 */

extern void when_case_destroy(struct when_case *wc);
/**<
 *  <!--  when_case_destroy(wc)                                        -->
 *  <!--  struct when_case *wc;                                        -->
 *
 *  Destroys a when case.
 */

extern int32 *when_case_values_list( struct when_case *wc);
/**< Retrieves the list of values of the given case. */
extern void when_case_set_values_list( struct when_case *wc, int32 *vallist);
/**<
 *  <!--  values = when_case_values_list(wc)                           -->
 *  <!--  when_case_set_values_list(wc,vallist)                        -->
 *  <!--  int32 *vallist;                                              -->
 *  <!--  struct when_case *wc;                                        -->
 *
 *  Sets the list of values of the given case.
 */

extern struct gl_list_t *when_case_rels_list( struct when_case *wc);
/**< Retrieves the list of rels of the given case. */
extern void when_case_set_rels_list(struct when_case *wc,
                                    struct gl_list_t *rlist);
/**<
 *  <!--  rels = when_case_rels_list(wc)                               -->
 *  <!--  when_case_set_rels_list(wc,rlist)                            -->
 *  <!--  struct gl_list_t *rlist;                                     -->
 *  <!--  struct when_case *wc;                                        -->
 *
 *  Sets the list of rels of the given case.
 */

extern struct gl_list_t *when_case_logrels_list( struct when_case *wc);
/**< Retrieves the list of logrels of the given case. */
extern void when_case_set_logrels_list(struct when_case *wc,
                                       struct gl_list_t *lrlist);
/**<
 *  <!--  logrels = when_case_logrels_list(wc)                         -->
 *  <!--  when_case_set_logrels_list(wc,lrlist)                        -->
 *  <!--  struct gl_list_t *lrlist;                                    -->
 *  <!--  struct when_case *wc;                                        -->
 *
 *  Sets the list of logrels of the given case.
 */

extern struct gl_list_t *when_case_whens_list( struct when_case *wc);
/**<  Retrieves the list of whens nested in the given case. */
extern void when_case_set_whens_list( struct when_case *wc,
                                      struct gl_list_t *wlist);
/**<
 *  <!--  whens = when_case_whens_list(wc)                             -->
 *  <!--  when_case_set_whens_list(wc,wlist)                           -->
 *  <!--  struct gl_list_t *wlist;                                     -->
 *  <!--  struct when_case *wc;                                        -->
 *
 *  Sets the list of whens nested in the given case.
 */

extern int32 when_case_case_number(struct when_case *wc);
/**< Returns the number of the case wc. */
extern void when_case_set_case_number(struct when_case *wc, int32 case_number);
/**<
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  int32 case_number;                                           -->
 *
 * Sets the case_number field of a case to the value of the case_number given.
 */

extern int32 when_case_num_rels( struct when_case *wc);
/**< Returns the number of the relations in wc. */
extern void when_case_set_num_rels(struct when_case *wc, int32 num_rels);
/**<
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  int32 num_rels;                                              -->
 *
 * Sets the num_rels field of a case to the value of the num_rels given.
 */

extern int32 when_case_num_inc_var( struct when_case *wc);
/**< Returns the number of the variables incident
 * in the relations contained in wc. */
extern void when_case_set_num_inc_var(struct when_case *wc, int32 num_inc_var);
/**<
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  int32 num_inc_var;                                           -->
 *
 * Sets the num_inc_var field of a case to the value of the num_inc_var given.
 */

extern int32 *when_case_ind_inc( struct when_case *wc);
/**< Returns the array of the master indices of the
 *  variables incident in the relations contained in wc. */
extern void when_case_set_ind_inc(struct when_case *wc, int32 *ind_inc);
/**<
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  int32 *ind_inc;                                              -->
 *
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
                                    when_case_filter_t *filter);
/**<
 *  <!--  value = when_case_apply_filter(wc,filter)                    -->
 *  <!--  int32 value;                                                 -->
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  when_case_filter_t *filter;                                  -->
 *
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the when's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */

extern uint32 when_case_flags(struct when_case *wc);
/**< Returns the flags field of the case wc. */
extern void when_case_set_flags(struct when_case *wc, uint32 flags);
/**<
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  uint32 flags;                                                -->
 *
 * Sets the entire flag field to the value of flags given.
 */

extern uint32 when_case_flagbit(struct when_case *wc, uint32 name);
/**<
 *  <!--  when_case_flagbit(wc,name);                                  -->
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  uint32 name;                                                 -->
 *  Returns the value of the bit specified from the case flags.
 *  name should be a WHEN_CASE_xx flag
 */

extern void when_case_set_flagbit(struct when_case *wc,
                                  uint32 NAME, uint32 oneorzero);
/**<
 *  <!--  struct when_case *wc;                                        -->
 *  <!--  uint32 NAME,oneorzero;                                       -->
 *  <!--  when_case_set_flagbit(wc,NAME,oneorzero)                     -->
 *
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

