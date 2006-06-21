/*
 *  Logical Relation Construction Routines
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: logrelation.h,v $
 *  Date last modified: $Date: 1997/07/18 12:31:43 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Vicente Rico-Ramirez
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

/** @file
 *  Logical Relation Construction Routines.
 *  <pre>
 *  When #including logrelation.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "expr_types.h"
 *         #include "logical_relation.h"
 *         #include "find.h"
 *  </pre>
 */

#ifndef __LOGRELATION_H_SEEN__
#define __LOGRELATION_H_SEEN__

/** Logical relation errors. */
enum logrelation_errors {
  find_logerror,                /**< error finding an instance */
  incorrect_logstructure,
  incorrect_linst_type,         /**< contains a nonboolean instance type */
  incorrect_real_linst_type,    /**< contains a real_inst type */
  incorrect_boolean_linst_type, /**< contains a boolean instance type */
  incorrect_integer_linst_type, /**< contains an integer variable instance type*/
  incorrect_symbol_linst_type,  /**< contains a symbol instance type */
  boolean_value_undefined,      /**< boolean constant doesn't have a value yet */
  incorrect_num_largs,          /**< wrong number of arguments */
  lokay                         /**< no error */
};

extern int g_simplify_logrelations;
/**<
 *  Logical relation simplification mode variable.
 *  The default value of this variable is 0.
 *  Values imply
 *  - <0  As 0 below.
 *  - 0   Compile logical relations as written by user.
 *  - 1   Simplify logical relations with easy reductions.
 *  - >1  As one, but left open for future work if someone wants to
 *        play nifty games like applying distributive and associative rules.<br><br>
 *
 *  The simplification of logical relation IS NOT implemented yet.
 *  @todo Implement simplification of logical relations.
 */

extern void InitLogRelInstantiator(void);
/**<
 *  <!--  InitLogRelInstantiator();                                    -->
 *  Sets up logical relation instantiation gizmos. This must be called once
 *  before any logical relations can be built, ideally at startup time.
 *  Do not call it again unless DestroyLogRelInstantiator is called first.
 *  If insufficient memory to compile anything at all, does exit(2).
 */

extern void DestroyLogRelInstantiator(void);
/**<
 *  <!--  DestroyLogRelInstantiator();                                 -->
 *  Destroy logical relation instantiation gizmos. This must be called to
 *  clean up before shutting down ASCEND.
 *  Do attempt to instantiate anything after you call this unless you
 *  have recalled InitLogRelInstantiator.
 */

extern void ReportLogRelInstantiator(FILE*);
/**<
 *  <!--  ReportLogRelInstantiator(f);                                 -->
 *  <!--  FILE *f;                                                     -->
 *  Reports on the logical relation instantiator to f.
 */

/*
 * Some defines for Logical Relations. struct logrelation *lr;
 */
#define Infix_Log_LhsSide(lr)   ((lr)->token.lhs_term)
#define Infix_Log_RhsSide(lr)   ((lr)->token.rhs_term)
#define Postfix_Log_LhsSide(lr) ((lr)->token.lhs)
#define Postfix_Log_RhsSide(lr) ((lr)->token.rhs)


extern int CheckLogRel(CONST struct Instance *reference,CONST struct Expr *ex);
/**<
 *  <!--  int CheckLogRel(reference,ex)                                -->
 *  <!--  const struct Instance *reference;                            -->
 *  <!--  const struct Expr *ex;                                       -->
 *  Return TRUE iff the expression can be created.
 */

extern struct logrelation *CreateLogRelStructure(enum Expr_enum t);
/**<
 *  <!--  struct logrelation *CreateLogRelStructure(t);                -->
 *  <!--  enum Epxr_enum t;                                            -->
 *  Create and return a new logrelation node of type t.
 *  The node will be initialized to its defaults. It will have a reference
 *  count of 0.
 */

extern struct logrelation *CreateLogicalRelation(struct Instance *reference,
                                                 struct Instance *lrelinst,
                                                 CONST struct Expr *ex,
                                                 enum logrelation_errors *err,
                                                 enum find_errors *ferr);
/**<
 *  <!--  struct logrelation *CreateLogicalRelation(reference,lrelinst,ex,err,ferr) -->
 *  <!--  struct Instance *reference,*relinst;                         -->
 *  <!--  const struct Expr *ex;                                       -->
 *  <!--  enum logrelation_errors *err;                                -->
 *  <!--  enum find_errors *ferr;                                      -->
 *
 *  Create a logical relation from an expression, a reference instance and a
 *  relation instance.  This modifies the instance tree. The type of relinst
 *  will be set to e_bol_token.<br><br>
 *
 *  The relation will not be successfully created until all variables
 *  exist and sets and constants in it are well defined.
 *
 *  @param reference  The instance which names are searched for in.
 *  @param lrelinst   The logical relation instance where this
 *                    relation will be placed.
 *  @param ex	        The expression to be converted.
 *  @param err	      Location to store logrel error code.
 *  @param ferr       Location to store find error code.
 */

extern void DestroyLogRelation(struct logrelation *lrel, struct Instance *lrelinst);
/**<
 *  <!--  void DestroyLogRelation(lrel,lrelinst)                       -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  <!--  struct Instance *lrelinst;                                   -->
 *  Deallocate a logical relation.  This will notify all the boolean instances
 *  in the logical relation that it will no longer be pointing to them.
 *  Also, it will notify all the relations included in satisfied terms
 *  that the logical relation will no longer be pointing to them.
 *  Therefore, this routine will have an effect on the instance tree.
 */

extern void ModifyLogRelPointers(struct gl_list_t *relorvar,
                                 struct logrelation *lrel,
                                 CONST struct Instance *old,
                                 CONST struct Instance *new);

/**<
 *  <!--  void ModifyLogRelPointers(relorvar,lrel,old,new);            -->
 *  <!--  struct gl_list_t *relorvar;                                  -->
 *  <!--  struct logrelation *lrel;                                    -->
 *  <!--  const struct Instance *old, *new;                            -->
 *  Change all references from old to new.  This doesn't do anything
 *  to the instances old and new. The gl_list can be the list of variables
 *  or the list of relations included in satisfied terms.
 */

extern void DoInOrderLogRelVisit(struct logrel_term *term,
                                 struct logrelation *r,
                                 void (*func)(struct logrel_term *,
                                              struct logrelation *));
/**<
 *  <!--  void DoInOrderLogRelVisit(term,r,func);                      -->
 *  <!--  struct logrel_term *term;                                    -->
 *  <!--  void (*func)(struct logrel_term *,struct logrelation *r);    -->
 *  Will visit an infix tree and apply function func to each term.
 *  It takes the logical relation corresponding to the tree as an argument,
 *  as some of the operations on terms are dependent upon the logical
 *  relation that the term belongs to.<br><br>
 *
 *  If the tree you are visiting is part of a logical relation side,
 *  your visiting function should not free any nodes it visits
 *  as these are parts of an array.
 */

extern struct logrelation *CopyLogRelByReference(CONST struct Instance *,
                                                 struct Instance *,
                                                 struct gl_list_t *,
                                                 struct gl_list_t *);
/**<
 *  Not yet implemented.
 *  @todo Implement compiler/logrelation.c/CopyLogRelByReference().
 */

extern struct logrelation
*CopyLogRelToModify(CONST struct Instance *src_inst,
                    struct Instance *dest_inst,
                    struct gl_list_t *varlist,
                    struct gl_list_t *rellist);
/**<
 *  <!--  struct logrelation *CopyLogRelation(src_inst,dest_inst,varlist,rellist) --> 
 *  <!--  const struct Instance *src_inst;                             -->
 *  <!--  struct Instance *dest_inst;                                  -->
 *  <!--  struct gl_list_t *varlist;                                   -->
 *  <!--  struct gl_list_t *rellist;                                   -->
 *
 *  Given a source and a destination logical relation instance, will copy the
 *  logrelation structure from the source instance and return it. A unique
 *  variable list is required. This variable list represents the new
 *  variables that will be marked icident upon the logical relation. Also,
 *  A unique relations list is required.<br><br>
 *
 *  The tokens in the struct logrelation * returned by this function are
 *  NOT independently allocated, they are in the postfix array.
 */

#endif /* __LOGRELATION_H_SEEN__ */

