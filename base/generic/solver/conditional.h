/*
 *  Conditional Module
 *  by Vicente Rico-Ramirez
 *  Created: 09/96
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: conditional.h,v $
 *  Date last modified: $Date: 1998/03/30 22:06:53 $
 *  Last modified by: $Author: rv2a $
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
 *
 */

/*
 *  Contents:     Conditional  module (ascend)
 *
 *  Dates:        09/96 - original version
 *
 *  Description:  This is the ascend version of the conditional module
 *                This version should be used by any user who receives
 *                his/her equations directly from an instance tree created
 *                by the ASCEND compiler.
 */

#ifndef conditional__already_included
#define conditional__already_included
/* 
 * requires #include "list.h" 
 * requires #include "base.h" 
 */
/*
 * I have to get rid of this
 */
#ifndef MAX_VAR_IN_LIST
#define MAX_VAR_IN_LIST 20
#endif  /* MAX_VAR_IN_LIST */ 


struct w_when {
  SlvBackendToken instance;    /* the associated ascend ATOM  */
  struct gl_list_t *dvars;     /* index of dis vars  */
  struct gl_list_t *cases;     /* when_case's */
  int32 num_cases;             /* number of cases in the WHEN */ 
  int32 sindex;
  int32 mindex;
  int32 model;		      /* index of a hypothetical MODEL when is from */
  uint32 flags;
};

/* 
 * if you mess with the above struct, change the defaults for it in .c file 
 */


/*
 *                        When functions
 */


extern struct w_when *when_create(SlvBackendToken, struct w_when *);
/*
 *  when_create(instance,newwhen)
 *  struct w_when *newwhen;
 *  SlvBackendToken instance;
 *
 *  Creates a when given the when instance.
 *  If the when supplied is NULL, we allocate the memory for the
 *  when we return, else we just init the memory you hand us and
 *  return it to you.
 *  We set the fields instance. Setting the rest of the information
 *  is the job of the bridge building function between the ascend
 *  instance tree (or other when back end) and the slv_system_t.
 */


extern SlvBackendToken when_instance(struct w_when *);
/*
 *  when_instance(when)
 *  struct w_when *when;
 *
 *  Returns the instance pointer from a when.
 */

extern void when_write_name(slv_system_t,struct w_when *,FILE *);
/*
 *  when_write_name(sys,when,file);
 *
 *  Writes a name to the file given. Handles 
 *  If sys is NULL, writes full ascend name. If file or when is NULL
 *  does not write.
 */

extern void when_destroy_cases(struct w_when *);
/*
 *  when_destroy_cases(when)
 *  struct w_when *when;
 *
 *  Destroys a the list of cases of a when.
 */

extern void when_destroy(struct w_when *);
/*
 *  when_destroy(when)
 *  struct w_when *when;
 *
 *  Destroys a when.
 */

extern char *when_make_name(slv_system_t,struct w_when *);
/*
 *  name = when_make_name(sys,when)
 *  slv_system_t sys;
 *  struct w_when *when;
 *  char *name;
 *
 *  Copies of the when instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 */


extern struct gl_list_t *when_dvars_list( struct w_when *);
extern void when_set_dvars_list( struct w_when *, struct gl_list_t *);
/*
 *  dvars = when_dvars_list(when)
 *  when_set_dvars_list(when,dvlist)
 *  struct gl_list_t *dvlist;
 *  struct w_when *when;
 *
 *  Sets or retrieves the list of dis variables of the given when
 */


extern struct gl_list_t *when_cases_list( struct w_when *);
extern void when_set_cases_list( struct w_when *, struct gl_list_t *);
/*
 *  cases = when_cases_list(when)
 *  when_set_cases_list(when,clist)
 *  struct gl_list_t *clist;
 *  struct w_when *when;
 *
 *  Sets or retrieves the list of cases of the given when
 */

extern int32 when_num_cases(struct w_when *);
extern void when_set_num_cases(struct w_when *,int32);
/*
 *  num_cases = when_num_cases(when)
 *  when_set_num_cases(when,num_cases)
 *  int32 num_cases;
 *  struct w_when *when;
 *
 *  Sets or retrieves the number of cases of the given when as it
 *  appears in a slv_system_t master when list.  
 */

extern int32 when_mindex(struct w_when *);
extern void when_set_mindex(struct w_when *,int32);
/*
 *  index = when_mindex(when)
 *  when_set_mindex(when,index)
 *  int32 index;
 *  struct w_when *when;
 *
 *  Sets or retrieves the index number of the given when as it
 *  appears in a slv_system_t master when list.  
 */

extern int32 when_sindex(struct w_when *);
extern void when_set_sindex(struct w_when *,int32);
/*
 *  index = when_sindex(when)
 *  when_set_sindex(when,index)
 *  int32 index;
 *  struct w_when *when;
 *
 *  Sets or retrieves the index number of the given relation as it
 *  appears in a solvers when list. 
 */

extern int32 when_model(const struct w_when *);
extern void when_set_model(struct w_when *,int32);
/*
 *  index = when_model(when)
 *  when_set_model(when,index) 
 *  int32 index;
 *  struct w_when *when;
 *
 *  Sets or retrieves the model number of the given when.
 *  In a hierarchy, whens come associated with
 *  models. Models are numbered from 1 to some upper limit.
 */

typedef struct when_filter_structure {
  uint32 matchbits;
  uint32 matchvalue;
} when_filter_t;


/*
 * when filtration functions.
 * We could have 32 binary (one bit) flags a client may want to query
 * in arbitrary combinations and paying attention to only certain of
 * the bits. We will provide a set of macros and functions for each of
 * these bits and for operations on the whole set.
 */

extern int32 when_apply_filter(struct w_when *,when_filter_t *);
/*
 *  value = when_apply_filter(when,filter)
 *  int32 value;
 *  struct w_when *when;
 *  when_filter_t *filter;
 *
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the when's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */
  
extern uint32 when_flags(struct w_when *);
extern void when_set_flags(struct w_when *,uint32);
/*
 *  struct w_when *when;
 *  uint32 flags;
 *
 * when_flags(when) returns the flags field of the when.
 * when_set_flags(when,flags) sets the entire flag field to the
 * value of flags given.
 */

extern uint32 when_flagbit(struct w_when *,uint32);
/* 
 *  when_flagbit(when,name);
 *  struct w_when *when;
 *  uint32 name;		
 *  name should be a WHEN_xx flag defined above)
 *  Returns the value of the bit specified from the when flags.
 */

extern void when_set_flagbit(struct w_when *,uint32, uint32);
/*
 *  struct w_when *when;
 *  uint32 NAME,oneorzero;
 *  when_set_flagbit(when,NAME,oneorzero)
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 *
 *  Example: when_set_flags(when,WHEN_INCLUDED,1) turns on the
 *  WHEN_INCLUDED bit.
 *  What it really does is:
 *  if (oneorzero) {
 *    when->flags |= field;
 *  } else {
 *    when->flags &= ~field;
 *  }
 *  In unix, see also man 3f bit or man not.
 */

/* 
 * the bit flags. 
 */

#define WHEN_INWHEN	0x1
#define WHEN_INCLUDED	0x2
#define WHEN_VISITED	0x4
#define WHEN_CHANGES_STRUCTURE	0x8
     
/*
 * WHEN_INWHEN	  Is this when nested in another when ?
 * WHEN_INCLUDED  solvers, ui clients. user wants when in problem.
 * 		  bit should be treated as readonly. use when_set_*
 *		  to change.
 * WHEN_VISITED	  Required for conditional analysis, for avoiding
 *                to reanalyze a WHEN
 * WHEN_CHANGES_STRUCTURE  Required for conditional analysis. Tells if
 *                the sutructure of the different CASES is all equal or
 *                otherwise
 */

/* 
 * the bit flag lookups 
 */
#ifdef NDEBUG
#define when_inwhen(w) 	         ((w)->flags & WHEN_INWHEN)
#define when_included(w) 	 ((w)->flags & WHEN_INCLUDED)
#define when_visited(w) 	 ((w)->flags & WHEN_VISITED)
#define when_changes_structure(w) ((w)->flags & WHEN_CHANGES_STRUCTURE)
#else
#define when_inwhen(w) 	         when_flagbit((w),WHEN_INWHEN)
#define when_included(w) 	 when_flagbit((w),WHEN_INCLUDED)
#define when_visited(w) 	 when_flagbit((w),WHEN_VISITED)
#define when_changes_structure(w) when_flagbit((w),WHEN_CHANGES_STRUCTURE)
#endif /* NDEBUG */

/* 
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
 */
#define when_set_inwhen(w,bv)   \
               when_set_flagbit((w),WHEN_INWHEN,(bv))
#define when_set_included(w,bv)   \
               when_set_flagbit((w),WHEN_INCLUDED,(bv))
#define when_set_visited(w,bv)   \
               when_set_flagbit((w),WHEN_VISITED,(bv))
#define when_set_changes_structure(w,bv)   \
               when_set_flagbit((w),WHEN_CHANGES_STRUCTURE,(bv))



/*
 *                  When Case  utility functions
 */


struct when_case {
  int32 values[MAX_VAR_IN_LIST];  /* values of conditional variables */
  struct gl_list_t *rels;         /* pointer to relations */
  struct gl_list_t *logrels;      /* pointer to logrelations */
  struct gl_list_t *whens;        /* pointer to whens */
  int32 case_number;              /* number of case */
  int32 num_rels;                 /* number of relations */
  int32 num_inc_var;              /* number of incident variables */
  int32 *ind_inc;                 /* master indeces of incidences */
  uint32 flags;                   /* flags ?? */
};


extern struct when_case *when_case_create(struct when_case *);
/*
 *  when_case_create(newcase)
 *  struct when_case *newcase;
 *
 *  Creates a when case.
 *  If the case supplied is NULL, we allocate the memory for the
 *  case we return, else we just init the memory you hand us and
 *  return it to you.
 */

extern void when_case_destroy(struct when_case *);
/*
 *  when_case_destroy(wc)
 *  struct when_case *wc;
 *
 *  Destroys a when case.
 */

extern int32 *when_case_values_list( struct when_case *); 
extern void when_case_set_values_list( struct when_case *, int32 *values);
/*
 *  values = when_case_values_list(wc)
 *  when_case_set_values_list(wc,vallist)
 *  int32 *vallist;
 *  struct when_case *wc;
 *
 *  Sets or retrieves the list of values of the given case
 */

extern struct gl_list_t *when_case_rels_list( struct when_case *); 
extern void when_case_set_rels_list( struct when_case *, struct gl_list_t *);
/*
 *  rels = when_case_rels_list(wc)
 *  when_case_set_rels_list(wc,rlist)
 *  struct gl_list_t *rlist;
 *  struct when_case *wc;
 *
 *  Sets or retrieves the list of rels of the given case
 */

extern struct gl_list_t *when_case_logrels_list( struct when_case *); 
extern void when_case_set_logrels_list(struct when_case *,struct gl_list_t *);
/*
 *  logrels = when_case_logrels_list(wc)
 *  when_case_set_logrels_list(wc,lrlist)
 *  struct gl_list_t *lrlist;
 *  struct when_case *wc;
 *
 *  Sets or retrieves the list of logrels of the given case
 */

extern struct gl_list_t *when_case_whens_list( struct when_case *); 
extern void when_case_set_whens_list( struct when_case *, struct gl_list_t *);
/*
 *  whens = when_case_whens_list(wc)
 *  when_case_set_whens_list(wc,wlist)
 *  struct gl_list_t *wlist;
 *  struct when_case *wc;
 *
 *  Sets or retrieves the list of whens nested in  the given case
 */

extern int32 when_case_case_number(struct when_case *);
extern void when_case_set_case_number(struct when_case *,int32);
/*
 *  struct when_case *wc;
 *  int32 case_number;
 *
 * when_case_case_number(wc) returns the number of the case wc.
 * when_case_set_case_number(wc,case_number) sets the case_number field of
 * a case to the value of the case_number given.
 */

extern int32 when_case_num_rels( struct when_case *);
extern void when_case_set_num_rels(struct when_case *, int32);
/*
 *  struct when_case *wc;
 *  int32 num_rels;
 *
 * when_case_num_rels(wc) returns the number of the relations in wc.
 * when_case_set_num_rels(wc,num_rels) sets the num_rels field of
 * a case to the value of the num_rels given.
 */

extern int32 when_case_num_inc_var( struct when_case *);
extern void when_case_set_num_inc_var(struct when_case *, int32);
/*
 *  struct when_case *wc;
 *  int32 num_inc_var;
 *
 * when_case_num_inc_var(wc) returns the number of the variables incident
 * in the relations contained in wc.
 * when_case_set_num_inc_var(wc,num_inc_var) sets the num_inc_var field of
 * a case to the value of the num_inc_var given.
 */

extern int32 *when_case_ind_inc( struct when_case *);
extern void when_case_set_ind_inc(struct when_case *, int32*);
/*
 *  struct when_case *wc;
 *  int32 *ind_inc;
 *
 * when_case_ind_inc(wc) returns the array of the master indices of
 * the variables incident in the relations contained in wc.
 * when_case_set_ind_inc(wc,ind_inc) sets the ind_inc pointer of
 * a case to the value of the array ind_inc given.
 */

typedef struct when_case_filter_structure {
  uint32 matchbits;
  uint32 matchvalue;
} when_case_filter_t;


/*  
 * case filtration functions 
 */

extern int32 when_case_apply_filter(struct when_case *,when_case_filter_t *);
/*
 *  value = when_case_apply_filter(wc,filter)
 *  int32 value;
 *  struct when_case *wc;
 *  when_case_filter_t *filter;
 *
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the when's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */
  
extern uint32 when_case_flags(struct when_case *);
extern void when_case_set_flags(struct when_case *,uint32);
/*
 *  struct when_case *wc;
 *  uint32 flags;
 *
 * when_case_flags(wc) returns the flags field of the case wc.
 * when_case_set_flags(wc,flags) sets the entire flag field to the
 * value of flags given.
 */

extern uint32 when_case_flagbit(struct when_case *,uint32);
/*
 *  when_case_flagbit(wc,name);
 *  struct when_case *wc;
 *  uint32 name;		
 *  name should be a WHEN_CASE_xx flag
 *  Returns the value of the bit specified from the case flags.
 */

extern void when_case_set_flagbit(struct when_case *,uint32, uint32);
/*
 *  struct when_case *wc;
 *  uint32 NAME,oneorzero;
 *  when_case_set_flagbit(wc,NAME,oneorzero)
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 */

/* 
 * the bit flags. 
 */

#define WHEN_CASE_ACTIVE        0x1
     
/*
 * WHEN_CASE_ACTIVE	Is this case active ?
 */

/* 
 * the bit flag lookups 
 */
#ifdef NDEBUG
#define when_case_active(c) 	         ((c)->flags & WHEN_CASE_ACTIVE)
#else
#define when_case_active(c) 	    when_case_flagbit((c),WHEN_CASE_ACTIVE)
#endif /* NDEBUG */

/* 
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
 */
#define when_case_set_active(c,bv)   \
               when_case_set_flagbit((c),WHEN_CASE_ACTIVE,(bv))

#endif  /* conditional__already_included */
