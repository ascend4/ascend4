/*
 *  Logical Relation Module
 *  by Vicente Rico-Ramirez
 *  Created: 09/96
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: logrel.h,v $
 *  Date last modified: $Date: 1997/07/29 15:47:43 $
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
 *  Contents:     Logical Relation module (ascend)
 *
 *  Dates:        09/96 - original version
 *
 *  Description:  This is the ascend version of the logrel module.  This
 *                version should be used by any user who receives his/her
 *                equations directly from an instance tree created by the
 *                ASCEND compiler.
 */
#ifndef logrel__already_included
#define logrel__already_included

/* 
 * requires #include "/base.h"
 * requires #include "/discrete.h" 
 */

/*********************************************************************\
  logrel_relation basic type operators.
\*********************************************************************/
  
enum logrel_enum {
  e_logrel_equal,
  e_logrel_not_equal
};

/*
 * enum logrel_enum:
 * The operator of a logical relation.
 */
  
struct logrel_relation {
   SlvBackendToken instance;
   struct dis_discrete **incidence;
   int32 n_incidences;		/* length of incidence */
   int32 mindex;		/* index in the slv_system_t master list */
   int32 sindex;		/* index in the slv_system_t solver list */
   int32 model;		       /* index of a hypothetical MODEL rel is from */
   uint32 flags;		/* flags */
};
/* 
 * if you mess with the above struct, change the defaults for it in .c file 
 */


extern struct logrel_relation *logrel_create(SlvBackendToken ,
					     struct logrel_relation *);
/*
 *  logrel_create(instance,newlogrel)
 *  struct logrel_relation *newlogrel;
 *  SlvBackendToken instance;
 *
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


extern SlvBackendToken logrel_instance(struct logrel_relation *);
/*
 *  logrel_instance(logrel)
 *  struct logrel_relation *logrel;
 *
 *  Returns the instance pointer from a logrel.
 */

extern void logrel_write_name(slv_system_t,struct logrel_relation *,FILE *);
/*
 *  logrel_write_name(sys,logrel,file);
 *
 *  Writes a name to the file given. 
 *  If sys is NULL, writes full ascend name. If file or logrel is NULL
 *  does not write.
 */

extern void logrel_destroy(struct logrel_relation *);
/*
 *  logrel_destroy(logrel)
 *  struct logrel_relation *logrel;
 *
 *  Destroys a logrelation.
 */

extern boolean logrel_equal(struct logrel_relation *);
extern boolean logrel_not_equal(struct logrel_relation *);
/*
 *  eq = logrel_equal(logrel)
 *  neq = logrel_not_equal(logrel)
 *  boolean eq,neq;
 *  struct logrel_relation *logrel;
 *
 *  Returns true if the given logrelation is satisfied if the
 *  operator in question is among those that make up the
 *  comparator of the logrelation.
 *  neq==TRUE implies logrel would be satisfied if lhs != rhs
 *  eq==TRUE implies logrel would be satisfied if lhs == rhs
 */

extern enum logrel_enum logrel_relop(struct logrel_relation *);
/*
 * relop = logrel_relop(logrel);
 * logrel_enum relop;
 * struct logrel_relation *logrel;
 * Returns the type of the  operator of a given logrelation.
 */

extern char *logrel_make_name(slv_system_t,struct logrel_relation *);
/*
 *  name = logrel_make_name(sys,logrel)
 *  slv_system_t sys;
 *  struct logrel_relation *logrel;
 *  char *name;
 *
 *  Copies of the logrelation instance name can be made and returned.
 *  The string returned should be freed when no longer in use.
 */
 
extern int32 logrel_mindex(struct logrel_relation *);
extern void logrel_set_mindex(struct logrel_relation *,int32);
/*
 *  index = logrel_mindex(logrel)
 *  logrel_set_mindex(logrel,index)
 *  int32 index;
 *  struct logrel_relation *logrel;
 *
 *  Sets or retrieves the index number of the given logrelation as it
 *  appears in a slv_system_t master logrelation list.  
 */

extern int32 logrel_sindex(struct logrel_relation *);
extern void logrel_set_sindex(struct logrel_relation *,int32);
/*
 *  index = logrel_sindex(logrel)
 *  logrel_set_sindex(logrel,index)
 *  int32 index;
 *  struct logrel_relation *logrel;
 *
 *  Sets or retrieves the index number of the given logrelation as it
 *  appears in a solvers logrelation list. 
 */

extern int32 logrel_model(const struct logrel_relation *);
extern void logrel_set_model(struct logrel_relation *,int32);
/*
 *  index = logrel_model(logrel)
 *  logrel_set_model(logrel,index)
 *  int32 index;
 *  struct logrel_relation *logrel;
 *
 *  Sets or retrieves the model number of the given logrelation.
 *  Models are numbered from 1 to some upper limit.
 */

extern int32 logrel_residual(struct logrel_relation *);
extern void logrel_set_residual(struct logrel_relation *,int32);
/*
 *  residual = logrel_residual(logrel)
 *  logrel_set_residual(logrel,residual)
 *  int32 residual;
 *  struct logrel_relation *logrel;
 *
 *  Sets or retrieves the logical residual field of the given logrelation.
 *  Note that the residual is not actually computed by logrel_residual:
 *  there is no guarantee (from this function) that the residual is
 *  actually correct.
 */

extern int32 logrel_nominal(struct logrel_relation *);
/*
 *  nominal = logrel_nominal(logrel)
 *  int32 nominal;
 *  struct logrel_relation *logrel;
 *
 *  Retrieves the nominal field of the given logrelation.
 *  No slv client has any business being able to set the nominal,
 *  so no such operator is provided.
 */


#ifdef NDEBUG
#define logrel_n_incidences(lr) ((lr)->n_incidences)
#define logrel_set_incidences(lr,n,ilist) \
   (lr)->n_incidences=(n); (lr)->incidence = (ilist)
#else
#define logrel_n_incidences(lr) logrel_n_incidencesF(lr)
#define logrel_set_incidences(lr,n,ilist) \
   logrel_set_incidencesF((lr),(n),(ilist))
#endif /* NDEBUG */


extern int32 logrel_n_incidencesF(struct logrel_relation *);
extern void logrel_set_incidencesF(struct logrel_relation *,
                                   int32,struct dis_discrete **);
/*
 *  logrel_n_incidences(logrel)
 *  logrel_set_incidences(logrel,n,ilist)
 *  struct logrel_relation *logrel;
 *  struct dis_discrete **ilist;
 *  int32 n;
 *
 *  logrel_n_incidences returns the length of the incidence_list.
 *  Solver clients should not call logrel_set_incidences,
 *  it is only for use by constructors of bridges to logrelation
 *  back ends.
 */

extern const struct dis_discrete
**logrel_incidence_list(struct logrel_relation *);
extern struct dis_discrete
**logrel_incidence_list_to_modify(struct logrel_relation *);
/*
 *  bv = logrel_incidence_list(logrel)
 *  struct logrel_relation *logrel;
 *  struct dis_discrete **bv;
 *
 *  Returns a pointer to an array logrel_n_incidences(logrel) long of bvars.
 *  Each element of the array is a struct dis_discrete *.
 *  If there is no incidence, NULL is returned.
 *  Pointers in this array will be unique. 
 *  The list belongs to the logrelation. Do not destroy it. 
 *  Do not change it.
 *
 *  BV IS NOT a NULL-TERMINATED LIST.
 */

/*
 * logrelation filtration functions.
 * We coulhave 32 binary (one bit) flags a client may want to query
 * in arbitrary combinations and paying attention to only certain of
 * the bits. We will provide a set of macros and functions for each of
 * these bits and for operations on the whole set.
 */

typedef struct logrel_filter_structure {
  uint32 matchbits;
  uint32 matchvalue;
} logrel_filter_t;

extern int logrel_apply_filter(struct logrel_relation *,logrel_filter_t *);
/*
 *  value = logrel_apply_filter(logrel,filter)
 *  int value;
 *  struct logrel_relation *logrel;
 *  logrel_filter_t *filter;
 *
 *  Returns 1 only if all of the positions specified in
 *  filter->matchbits have the same values in
 *  filter->matchvalue and the logrelation's flags value.
 *  Bits set to 0 in filter->matchbits are ignored for the test.
 */
  
extern uint32 logrel_flags(struct logrel_relation *);
extern void logrel_set_flags(struct logrel_relation *,uint32);
/*
 *  struct logrel_relation *logrel;
 *  uint32 flags;
 *
 * logrel_flags(logrel) returns the flags field of the logrelation.
 * logrel_set_flags(logrel,flags) sets the entire flag field to the
 * value of flags given.
 */

extern uint32 logrel_flagbit(struct logrel_relation *,uint32);
/* 
 *  logrel_flagbit(logrel,name);
 *  struct logrel_relation *logrel;
 *  uint32 name;		
 *  name should be a LOGREL_xx flag defined above)
 *  Returns the value of the bit specified from the logrelation flags.
 */

extern void logrel_set_flagbit(struct logrel_relation *,uint32, uint32);
/*
 *  struct logrel_relation *logrel;
 *  unsigned int NAME,oneorzero;
 *  logrel_set_flagbit(logrel,NAME,oneorzero)
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *  The macro names are the defined up at the top of this file.
 *
 *  Example: logrel_set_flags(logrel,LOGREL_INCLUDED,1) turns on the
 *  LOGREL_INCLUDED bit.
 *  What it really does is:
 *  if (oneorzero) {
 *    logrel->flags |= field;
 *  } else {
 *    logrel->flags &= ~field;
 *  }
 *  In unix, see also man 3f bit or man not.
 */

/* 
 * the bit flags. explained afterward. several are for use of
 * transient clients and should be ignored by solver engines 
 */
#define LOGREL_INCLUDED	0x1
/* 
 * INCLUDED is as yet a funny one. treat it as readonly because
 * you can only change it using a int function and not the
 * bit manipulation functions. It is here in the bits because
 * it is useful in filters sometimes.
 */
#define LOGREL_SATISFIED 0x2
#define LOGREL_EQUALITY	0x4
/* Conditional Modeling */
#define LOGREL_INWHEN      0x8
#define LOGREL_ACTIVE      0x10 
/* Conditional LogRelations (Boundaries) */
#define LOGREL_CONDITIONAL 0x20 
#define LOGREL_IN_BLOCK 0x40 
/*
 * LOGREL_INCLUDED	solvers, ui clients. user wants eqn in problem.
 *		        bit should be treated as readonly. use logrel_set_*
 *		        to change.
 * LOGREL_SATISFIED  has logrel been pronounced satisfied by someone?
 *		     bit should be treated as readonly. use logrel_set_*
 *		     to change.
 * LOGREL_EQUALITY	is logrelation an equality? readonly for clients.
 * LOGREL_INWHEN	is logrelation in a when? readonly for clients.
 * LOGREL_ACTIVE        is this logrelation currently a part of my problem?
 * LOGREL_CONDITIONAL   is logrelation conditional? readonly for clients.
 * LOGREL_IN_BLOCK      is the logrelation in the current block of registered
 *		        client? for clients.
 */

/* 
 * the bit flag lookups 
 */
#ifdef NDEBUG
#define logrel_satisfied(lr) 	 ((lr)->flags & LOGREL_SATISFIED)
#define logrel_equality(lr)      ((lr)->flags & LOGREL_EQUALITY)
#define logrel_in_when(lr) 	 ((lr)->flags & LOGREL_INWHEN)
#define logrel_active(lr) 	 ((lr)->flags & LOGREL_ACTIVE)
#define logrel_conditional(lr)   ((lr)->flags & LOGREL_CONDITIONAL)
#define logrel_in_block(lr)      ((lr)->flags & LOGREL_IN_BLOCK)
#else
#define logrel_satisfied(lr) 	 logrel_flagbit((lr),LOGREL_SATISFIED)
#define logrel_equality(lr)      logrel_flagbit((lr),LOGREL_EQUALITY)
#define logrel_in_when(lr) 	 logrel_flagbit((lr),LOGREL_INWHEN)
#define logrel_active(lr) 	 logrel_flagbit((lr),LOGREL_ACTIVE)
#define logrel_conditional(lr)   logrel_flagbit((lr),LOGREL_CONDITIONAL)
#define logrel_in_bolck(lr)      logrel_flagbit((lr),LOGREL_IN_BLOCK)
#endif /* NDEBUG */

/* 
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
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

extern uint32 logrel_included(struct logrel_relation *);
extern void logrel_set_included(struct logrel_relation *,uint32);
/*
 *  included = logrel_included(logrel)
 *  logrel_set_included(logrel,included)
 *  uint32 included;
 *  struct logrel_relation *logrel;
 *
 *  Sets or retrieves the included field of the given logrelation.
 *  This has side effect on the ascend instance, so it isn't
 *  implemented with the rest of the macros above. This needs to
 *  change.
 */

#endif  /* logrel_already_included  */
