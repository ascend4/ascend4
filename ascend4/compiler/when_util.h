/*
 *  When Utility Functions
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: when_util.h,v $
 *  Date last modified: $Date: 1997/07/29 15:53:00 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

#ifndef __WHEN_UTIL_H_SEEN__
#define __WHEN_UTIL_H_SEEN__
/* requires
 *# #include "compiler.h"
 *# #include "base.h"
 *  base, say what?
 *# #include "instance_enum.h"
 *# #include "case.h"
 *# #include "relation.h"
 *# #include "relation_type.h"
 *# #include "instance_types.h"
 */

#ifndef TRUE
#define TRUE       1
#endif
#ifndef FALSE
#define FALSE      0
#endif

/*
 *                       When utility fuctions
 */

extern unsigned long NumberWhenVariables(CONST struct Instance *);
/*
 *  unsigned long NumberWhenVariables(when)
 *  struct Instance *when;
 *  This will indicate the number of distinct variables to which the
 *  varlist of this when instance points.
 */

extern struct Instance *WhenVariable(CONST struct Instance *,
                                     unsigned long);
/*
 *  struct Instance *WhenVariable(when,varnum)
 *  struct Instance *when;
 *  unsigned long varnum;
 *  This will return the varnum'th variable of the when varlist.
 */

extern int VarFoundInWhen(CONST struct Instance *, CONST struct Instance *);
/*
 *  int VarFoundInWhen(var,when)
 *  struct Instance *var;
 *  struct Instance *when;
 *  Return 1 if var is a member of the when var list, else return 0.
 */

extern unsigned long NumberWhenCases(CONST struct Instance *);
/*
 *  unsigned long NumberWhenCases(when)
 *  struct Instance *when;
 *  This will indicate the number of distinct cases to which the
 *  caselist of this when instance points.
 */

extern struct Case *WhenCase(CONST struct Instance *,
                             unsigned long);
/*
 *  struct Instance *WhenCase(when,casenum)
 *  struct Instance *when;
 *  unsigned long casenum;
 *  This will return the casenum'th case of the when caselist.
 */

extern void ModifyWhenPointers(struct gl_list_t *,
                               CONST struct Instance *,
                               CONST struct Instance *);
/*
 *  void ModifyWhenPointers(reforvar,old,new);
 *  struct gl_list_t *reforvar;
 *  CONST struct Instance *old;
 *  CONST struct Instance *new;
 *
 *  Variable List or Case List Maintenance.
 *
 *  This requires some explanation. There are a number of cases
 *  to consider.
 *
 *  1) the old instance does not exist in the reforvar list -- do nothing.
 *
 *  2) the old instance exists, but the new does not -- store the
 *     the new instance in the slot where the old instance was and
 *     return.
 *
 *  3) the old instance exists, *and* the new instance also exists in
 *     the reforvar list.
 *
 *   4) the new instance is NULL, which can happen transiently during
 *      some operations. This defaults to case 2).
 */


extern struct gl_list_t *CopyWhenBVarList(struct Instance *,
              struct gl_list_t *);
/*
 *  struct gl_list_t *CopyWhenBVarList(dest_inst,copylist)
 *  struct Instance *dest_inst;
 *  struct gl_list_t *copylist;
 *
 *  Copy a When list of variables.
 */

extern struct gl_list_t *CopyWhenCaseRefList(struct Instance *,
                 struct gl_list_t *);
/*
 *  struct gl_list_t *CopyWhenCaseRefList(dest_inst,copylist)
 *  struct Instance *dest_inst;
 *  struct gl_list_t *copylist;
 *  Copy the Reference list of a case in a WHEN instance
 */

extern void DestroyWhenVarList(struct gl_list_t *, struct Instance *);
/*
 *  void DestroyWhenVarList(l,inst)
 *  struct gl_list_t *l;
 *  struct Instance *inst;
 *  Destroy a When list of variables.
 */


extern void DestroyWhenCaseList(struct gl_list_t *, struct Instance *);
/*
 *  void DestroyWhenCaseList(l,inst);
 *  struct gl_list_t *l;
 *  struct Instance *inst;
 *  Destroy a When list of cases
 */


/*
 *  Model Filtration functions. This file will provide a mean to "turn
 *  off" all the models included in a WHEN statement. That will be done
 *  by using a set of binary (one bit) flags. We will provide a set of
 *  macros and functions for each of these bits and for operations on the
 *  whole set.
 */

extern unsigned int model_flags(struct Instance *);
extern void model_set_flags(struct Instance *,unsigned int);
/*
 *  struct ModelInstance *mod;
 *  unsigned int anon_ flags;
 *
 *  model_flags(mod) returns the anon_flags field of the model instance.
 *  model_set_flags(mod,flags) sets the entire anon_flags field to the
 *  value of flags given.
 */

extern unsigned int model_flagbit(CONST struct Instance *, unsigned int);
/*
 *  model_flagbit(mod,name);
 *  struct WhenInstance *mod;
 *  unsigned int name;
 *  name should be a MOD_xx flag
 *  Returns the value of the bit specified from the model instance flags.
 */

extern void model_set_flagbit(struct Instance *,unsigned int,
                              unsigned int);
/*
 *  struct ModelInstance *mod;
 *  unsigned int NAME,oneorzero;
 *  model_set_flagbit(mod,NAME,oneorzero)
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *
 *  Example: model_set_flags(mod,REL_ON,1) turns on the
 *  MODEL_ON bit.
 *  What it really does is:
 *  if (oneorzero) {
 *    mod->anon_flags |= field;
 *  } else {
 *    mod->anon_flags &= ~field;
 *  }
 */

/* the bit flags. explained afterward.*/
#define MODEL_ON    	0x1
#define MODEL_IN_WHEN	0x2
#define MODEL_ACTIVE    0x4

/*
 *  MODEL_ON  	     is the model part of my problem ?
 *  MODEL_IN_WHEN	     is the MODEL inside a WHEN statement ?
 *  MODEL_ACTIVE         is the model active in the current iteration ?
 */

#define model_on(m)            	model_flagbit((m),MODEL_ON)
#define model_in_when(m)	model_flagbit((m),MODEL_IN_WHEN)
#define model_active(m)	       	model_flagbit((m),MODEL_ACTIVE)

/*
 * bit flag assignments. any value other than 0 for bv turns the
 * named flag to 1. 0 sets it to 0.
 */
#define model_set_on(m,bv)	  model_set_flagbit((m),MODEL_ON,(bv))
#define model_set_in_when(m,bv)   model_set_flagbit((m),MODEL_IN_WHEN,(bv))
#define model_set_active(m,bv)	  model_set_flagbit((m),MODEL_ACTIVE,(bv))


/*
 *  Log/Relation Filtration functions. This file will provide a mean to "turn
 *  on" all the relations/logrelation  not included in a WHEN statement.
 *  That will be done by using a set of binary (one bit) flags. We will
 *  provide a set of  macros and functions for each of these bits and for
 *  operations on the whole set. A flag to define a relation/logrelation as
 *  Conditional will also be provided.
 */

extern unsigned int relinst_flags(struct Instance *);
extern void relinst_set_flags(struct Instance *,unsigned int);
/*
 *  struct Instance *rel;
 *  unsigned int anon_ flags;
 *
 *  relinst_flags(rel) returns the anon_flags field of the relation instance.
 *  relinst_set_flags(rel,flags) sets the entire anon_flags field to the
 *  value of flags given.
 */

extern unsigned int relinst_flagbit(CONST struct Instance *,unsigned int);
/*
 *  relinst_flagbit(rel,name);
 *  struct Instance *rel;
 *  unsigned int name;
 *  name should be a REL_xx flag
 *  Returns the value of the bit specified from the relation instance flags.
 */

extern void relinst_set_flagbit(struct Instance *,unsigned int,
                                unsigned int);
/*
 *  struct Instance *rel;
 *  unsigned int NAME,oneorzero;
 *  relinst_set_flagbit(rel,NAME,oneorzero)
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *
 *  Example: relinst_set_flags(rel,REL_ON,1) turns on the
 *  REL_ON bit.
 *  What it really does is:
 *  if (oneorzero) {
 *    rel->anon_flags |= field;
 *  } else {
 *    rel->anon_flags &= ~field;
 *  }
 */

extern unsigned int logrelinst_flags(struct Instance *);
extern void logrelinst_set_flags(struct Instance *,unsigned int);
/*
 *  struct Instance *lrel;
 *  unsigned int anon_ flags;
 *
 *  logrelinst_flags(lrel) returns the anon_flags field of the logical
 *  relation instance.
 *  log relinst_set_flags(lrel,flags) sets the entire anon_flags field to the
 *  value of flags given.
 */

extern unsigned int logrelinst_flagbit(CONST struct Instance *,unsigned int);
/*
 *  logrelinst_flagbit(lrel,name);
 *  struct Instance *lrel;
 *  unsigned int name;
 *  name should be a LOGREL_xx flag
 *  Returns the value of the bit specified from the logical relation
 *  instance flags.
 */

extern void logrelinst_set_flagbit(struct Instance *,unsigned int,
                                   unsigned int);
/*
 *  struct Instance *rel;
 *  unsigned int NAME,oneorzero;
 *  logrelinst_set_flagbit(lrel,NAME,oneorzero)
 *
 *  Sets the bit, which should be referred to by its macro name,
 *  on if oneorzero is >0 and off is oneorzero is 0.
 *
 *  Example: logrelinst_set_flags(lrel,LOGREL_ON,1) turns on the
 *  LOGREL_ON bit.
 *  What it really does is:
 *  if (oneorzero) {
 *    lrel->anon_flags |= field;
 *  } else {
 *    lrel->anon_flags &= ~field;
 *  }
 */

/* the bit flags. explained afterward.*/
#define RELINST_ON    	0x1
#define RELINST_CONDITIONAL	0x2
#define RELINST_IN_WHEN     0x4
#define LOGRELINST_ON    	0x8
#define LOGRELINST_CONDITIONAL	0x10
#define LOGRELINST_IN_WHEN     0x20

/*
 *  RELINST_ON  	     is the relation part of my problem ?
 *  RELINST_CONDITIONAL  is the relation a boundary expression ?
 *  RELINST_IN_WHEN	     is the relation inside a WHEN statement?
 *  LOGRELINST_ON  	     is the logical relation part of my problem ?
 *  LOGRELINST_CONDITIONAL  is the logical relation a boundary expression ?
 *  LOGRELINST_IN_WHEN	     is the logical relation inside a WHEN statement?
 */

/* the bit flag lookups */
#if 0
/*
 *  need to fix these. r is now an instance, not relation instance
 *  Also, lr is a not a logrelation instance.
 */
#define relinst_on(r)	             ((r)->anon_flags & RELINST_ON)
#define relinst_conditional(r)	     ((r)->anon_flags & RELINST_CONDITIONAL)
#define relinst_in_when(r)	     ((r)->anon_flags & RELINST_IN_WHEN)
#define logrelinst_on(lr)	     ((lr)->anon_flags & LOGRELINST_ON)
#define logrelinst_conditional(lr)  ((lr)->anon_flags & LOGRELINST_CONDITIONAL)
#define logrelinst_in_when(lr)	     ((lr)->anon_flags & LOGRELINST_IN_WHEN)
#else
#define relinst_on(r)                relinst_flagbit((r),RELINST_ON)
#define relinst_conditional(r)	     relinst_flagbit((r),RELINST_CONDITIONAL)
#define relinst_in_when(r)	     relinst_flagbit((r),RELINST_IN_WHEN)
#define logrelinst_on(lr)            logrelinst_flagbit((lr),LOGRELINST_ON)
#define logrelinst_conditional(lr) \
                                logrelinst_flagbit((lr),LOGRELINST_CONDITIONAL)
#define logrelinst_in_when(lr)	    logrelinst_flagbit((lr),LOGRELINST_IN_WHEN)
#endif

/*
 *  bit flag assignments. any value other than 0 for bv turns the
 *  named flag to 1. 0 sets it to 0.
 */
#define relinst_set_on(r,bv)   relinst_set_flagbit((r),RELINST_ON,(bv))
#define relinst_set_conditional(r,bv)  \
                       relinst_set_flagbit((r),RELINST_CONDITIONAL,(bv))
#define relinst_set_in_when(r,bv) relinst_set_flagbit((r),RELINST_IN_WHEN,(bv))
#define logrelinst_set_on(lr,bv)   \
                       logrelinst_set_flagbit((lr),LOGRELINST_ON,(bv))
#define logrelinst_set_conditional(lr,bv)  \
                       logrelinst_set_flagbit((lr),LOGRELINST_CONDITIONAL,(bv))
#define logrelinst_set_in_when(lr,bv) \
                       logrelinst_set_flagbit((lr),LOGRELINST_IN_WHEN,(bv))

#endif /* __WHEN_UTIL_H_SEEN__ */
