/*
 *  Relation Construction Routines
 *  by Tom Epperly
 *  Created: 1/30/90
 *  Version: $Revision: 1.14 $
 *  Date last modified: $Date: 1998/01/11 17:03:59 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1994, 1995 Kirk Andre' Abbott
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
/*
 *  Change log:
 *  1995 Abbott
 *   - Added glassbox and blackbox external relations.
 *   - Implemented relation sharing.
 *  1996 Allan
 *   - Reduced size, but not content, of relation terms.
 *   - Tightened relations semantics for constants.
 *   - Improved relation error reporting.
 *   - Added compile time relation simplification.
 *   - Removed relation query/scaling routines to relation_util.h.
 */

/** @file
 *  Relation Construction Routines.
 *  <pre>
 *  When #including relation.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "extfunc.h"
 *         #include "dimen.h"
 *         #include "expr_types.h"
 *         #include "relation_type.h"
 *         #include "find.h"
 *         #include "instance_enum.h"
 *         #include "exprs.h"
 *         #include "extfunc.h"
 *         #include "rel_blackbox.h"
 *	   #include "vlist.h"
 *  </pre>
 */

#ifndef ASC_RELATION_H
#define ASC_RELATION_H

/**	addtogroup compiler Compiler
	@{
*/

enum relation_errors {
  find_error,                   /**< indicates an error finding an instance */
  incorrect_structure,
  incorrect_inst_type,          /**< contains a nonscalar instance type */
  incorrect_real_inst_type,     /**< contains a real_inst type */
  incorrect_boolean_inst_type,  /**< contains a boolean instance type */
  incorrect_integer_inst_type,  /**< contains an integer variable instance type*/
  incorrect_symbol_inst_type,   /**< contains a symbol instance type */
  integer_value_undefined,      /**< integer constant doesn't have a value yet */
  real_value_undefined,         /**< real constant doesn't have a value yet */
  real_value_wild,              /**< real constant doesn't have a dim yet */
  incorrect_num_args,           /**< wrong number of arguements */
  okay
};

extern int g_ExternalNodeStamps;
/**< 
 * Counter of the nodestamps ever used.
 * If you aren't with relation.c or instantiate.c, don't
 * change this value.
 */

#define RANT 1
/* The following comment is an editorial by Kirk Abbott.
   Your mileage may vary. Widely. Very Widely. */
/*
 *  This module attempts to encapsulate the code needed for relations
 *  processing in ASCEND. At this time we find it necessary to support
 *  four (4) !! types of relations.
 *  1) TokenRelations : relations which are a stream of tokens in postfix,
 *      and also wired up infix -- elegant but very expensive. Very easy
 *      to do symbolic processing though.
 *  2) OpCodeRelations : relations which may be represented as any array
 *      of integer opcodes in postfix , with an auxiliary array of doubles.
 *      Very compact representation; can be very fast, but not very useful
 *      for other operations than evaluation.
 *  3) GlassBoxRelations : relations which provide enough information
 *      to access external pieces of code. This is expected to the workhorse
 *      relation type especially for well understood problems.
 *  4) BlackBoxRelations : relations which fully represent external pieces
 *      of code, and which at solution time solve themselves to completion.
 *      Good for interfacing to someones else package. Computationally
 *      though, is not very attractive, especially if imbedded within a
 *      tightly coupled system.
 *
 *  For small to medium size problems (< 10000 vars), it is expected that
 *  relations types 1 or 2 will be used most. For problems of this size
 *  that dont require special meta-symbolic manipulation, it is expected
 *  that type 2) will dominate and eventually type 1) will become obsolete.
 *
 *  For medium to large scale problems (5000 - 30000 vars), it is expected
 *  that there will be repeated substructures that are well understood that
 *  can be captured in the GlassBoxRelations.
 *
 *  For very large scale problems, the distribution is expected to be:
 *  15 - 20 % - TokenRelations or OpCodeRelations.
 *  75 - 80 % - GlassBoxRelations
 *  5 %       - BlackBoxRelations -- encapsulating an unknown number of
 *               variables/relations.
 *
 *  Each relation maintains a list of incident vars. The definition of
 *  incident var at this time is as follows:
 *  Any REAL_ATOM_INST which appears in the relation, is considered
 *  incident. It is proposed to extent this definition to include
 *  BOOLEAN_ATOM_INST. It is the responsibility of a client solver to
 *  narrow this definition further, for example by considering some
 *  variables as being constant or parameters.
 *
 *  At the time of construction of the relation, the variables that are
 *  incident upon a relation also are told that which relations they are
 *  a part of. This is necessary to support other operations in the language
 *  such as ARE_THE_SAME etc.
 */

/* The following comment is a rebuttal, by Ben Allan, of the above.
 * Your mileage may vary. Very Widely.
 */
/*
 *  The above goop is mostly conjecture unsupported by fact.
 *  It was written describing a very naive implementation of token
 *  relations. It was also written ignoring the fact that todays
 *  processors can benefit greatly from having all information
 *  localized to a single array of tokens rather than spread over
 *  a bunch of arrays of opcodes and their auxillaries.
 *
 *  The role of glass and black boxes is to support legacy code and
 *  models whose very special structures gives big speed improvements,
 *  not to gain speed on generic ascend models. The main reason for
 *  this is that the glass and black formats strip away all the
 *  information (token info) needed to make a solver behave well,
 *  i.e. functional form, scaling information, and higher derivative
 *  information.
 *
 *  In a Real Time Optimization production environment one would
 *  want glassbox relations exclusively, and possibly evaluation
 *  of same in parallel. We have achieved big memory reduction
 *  by having glassbox and token relation code/token data
 *  shared by large isomorphic groups of models/relations, but
 *  this would most likely cause access conflicts during
 *  parallel gradient and residual evaluation. Bad News.
 *
 *  In math programming one must take care not to be penny wise
 *  and pound foolish.
 */
#undef RANT

/*  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv <--headered in compiler.h now.
 *  extern int g_simplify_relations;
 *
 *  The default value of this variable is 1.
 *  Values imply (as of 3/96, Benjamin Allan)
 *  <0: As 0 below.
 *  0:  Compile token relations as written by user.
 *  1:  Simplify relations with easy reductions.
 *  >1: As one, but left open for future work if someone wants to
 *      play nifty games like applying distributive and associative rules.
 *
 *  Level 1 reductions:
 *  Reduce all functions of constants to resulting values, i.e.
 *      C(+,-,*,/,^)C' => C"
 *      f(C) => C' where f is any unary function or unary minus
 *      A/C => A*C' where A is anything
 *  Convert real powers to integer powers where correct to do so, ie
 *      A^2 (normally pow(A,2.0)) => A^(int)2 (asc_powi(A,2)).
 *  Elimination of invariant subexpressions caused by 0, i.e.
 *      (s)^0 => 1 for all s, but 0^0 => error. 1 is dimensionless.
 *  Convert all integer constants to dimensionless real where
 *  correct to do so.
 *  Level 1 reductions also involve a fair amount of sanity checking
 *  in relations and may spot user errors.
 *
 *  Level 2 reductions:
 *  Elimination of invariant subexpressions caused by 0, i.e.
 *      (s)*0 => 0 for all s (not yet implemented)
 *      0/s => 0 for all s (not yet implemented)
 *
 *  These	 simplifications have implications:
 *     1) Users should group their constants for reductions.
 *     2) Expressions such as v^SUM(j | j IN [empty set]]) legal and => 1.
 *        This didn't work before.
 *     3) Sqr and cube become obsolete since we convert small integer
 *        powers to ipowers. It is nice to keep these for symbolic inversion,
 *        though.
 *     4) The compiler picks up the ability to do dimensionality analysis.
 */

extern void InitRelInstantiator(void);
/**<
 *  Sets up relation instantiation gizmos. This must be called once
 *  before any relations can be built, ideally at startup time.
 *  Do not call it again unless DestroyRelInstantiator is called first.
 *  If insufficient memory to compile anything at all, does exit(2).
 */

extern void DestroyRelInstantiator(void);
/**<
 *  Destroy relation instantiation gizmos. This must be called to
 *  clean up before shutting down ASCEND.
 *  Do attempt to instantiate anything after you call this unless you
 *  have recalled InitRelInstantiator.
 */

extern void ReportRelInstantiator(FILE *f);
/**<
 *  Reports on the relation instantiator to f.
 */

/**
 * Some defines for SharedRelations. struct relation *r;
 * In principle, all relations are SharedRelations.
 * These macros can be used as C lval or rval.
 * struct relation *r;
 */
#define RelationRefCount(r) ((r)->share->s.ref_count)

/** General key-stroke saving macros that will also let us
 * avoid rewriting most of relation.c every time the
 * structures get changed.
 * struct relation *r;
 */
#define RTOKEN(r)  (r)->share->token
#define RBBOX(r)   (r)->share->bbox
#define RGBOX(r)   (r)->share->gbox
#define ROPCODE(r) (r)->share->opcode

/**
 * Some defines for TokenRelations. struct relation *r;
 */
#define Infix_LhsSide(r)   ((r)->share->token.lhs_term)
#define Infix_RhsSide(r)   ((r)->share->token.rhs_term)
#define PostFix_LhsSide(r) ((r)->share->token.lhs)
#define PostFix_RhsSide(r) ((r)->share->token.rhs)

extern int CheckExternal(CONST struct Instance *reference,
                         CONST struct VariableList *vlist,
                         CONST struct Name *n);
/**<
 *  Return TRUE iff the vlist can be resolved.
 */

extern struct gl_list_t *ProcessExtRelArgs(CONST struct Instance *inst,
                              CONST struct VariableList *vl,
                              enum find_errors *ferr);
/**<
return the list of lists arguments, if they all exist, or NULL
if something is missing.
Result is the callers to destroy with DestroySpecialList.
@param inst parent model of the external relation.
@param vl argument list of names.
@param ferr find result code, if anyone cares.
*/

struct gl_list_t *ProcessExtRelArgNames(CONST struct Instance *inst, CONST struct VariableList *vl, enum find_errors *ferr);
/**<
return the list of lists argument names, if they all exist, or NULL
if something is missing.
Result is the callers to destroy with DeepDestroySpecialList(l,DestroyName).
@param inst parent model of the external relation.
@param vl argument list of names.
@param ferr find result code, if anyone cares.
*/

extern struct Instance *ProcessExtRelData(CONST struct Instance *inst, CONST struct Name *n, enum find_errors *ferr);
/**<
Check the value of ferr for correct_instance or otherwise.
@return the DATA instance if it exists (may be NULL if not specified).
@param inst parent model of the external relation.
@param n data instance name.
@param ferr find result code, if anyone cares.

*/

extern struct Name *ProcessExtRelDataName(CONST struct Instance *inst, CONST struct Name *n, enum find_errors *ferr);
/**<
Check the value of ferr for correct_instance or otherwise.
@return the name for DATA instance if it data exists (may be NULL if not specified).
@param inst parent model of the external relation.
@param n data instance name, possibly in indexed rather than exact form.
@param ferr find result code, if anyone cares.

*/

extern int CheckRelation(CONST struct Instance *reference,
                         CONST struct Expr *ex);
/**<
 *  Return TRUE iff the expression can be created.
 */

#define crs_NOUNION 0
#define crs_NEWUNION 1
extern struct relation *CreateRelationStructure(enum Expr_enum relop,
                                                int copyunion);
/**<
 *  Create and return a new relation node of relational operator relop.
 *  relop = e_equal, etc etc, not e_token, e_glassbox, etc.
 *  e_token, etc are stored on the instance, not the relation core.
 *  The node will be initialized to its defaults. It will have a reference
 *  count of 0. If copyunion is crs_NEWUNION, the RelationUnion
 *  pointer will be allocated, else if crs_NOUNION the pointer will be NULL.
 */

extern struct relation
*CreateTokenRelation(struct Instance *reference,
                     struct Instance *relinst,
                     CONST struct Expr *ex,
                     enum relation_errors *err,
                     enum find_errors *ferr);
/**<
 *  Create a relation from an expression, a reference instance and a relation
 *  instance.  This modifies the instance tree. The type of relinst will
 *  be set to e_token.<br><br>
 *
 *  The relation will not be successfully created until all variables
 *  exist and sets and constants in it are well defined.<br><br>
 *
 *  @param reference The instance which names are searched for in.
 *  @param relinst   The relation instance where this relation will be placed.
 *  @param ex        The expression to be converted..
 *  @param err       Location to store relation error code.
 *  @param ferr      Location to store find error code.
 */

extern struct relation
*CreateOpCodeRelation(struct Instance *reference,
                      struct Instance *relinst,
                      CONST struct Expr *ex,
                      enum relation_errors *err,
                      enum find_errors *ferr);
/**<
 *  NOT IMPLEMENTED.
 *
 *  Create a relation from an expression, a reference instance and a relation
 *  instance.  This modifies the instance tree. The type of relinst will
 *  be set to e_opcode.<br><br>
 *
 *  reference	the instance which names are searched for in
 *  relinst		the relation instance where this relation will be placed
 *  ex		the expression to be converted.
 *  err		output any errors
 */

extern struct relation
*CreateGlassBoxRelation(struct Instance *relinst,
                        struct ExternalFunc *efunc,
                        struct gl_list_t *varlist,
                        int rIndex,
                        enum Expr_enum relop);
/**<
 *  Create a relation from an expression, a reference instance and a relation
 *  instance.  This modifies the instance tree.
 */

extern struct relation
*CreateBlackBoxRelation(struct Instance *relinst,
                        struct Instance *whichvar,
                        struct gl_list_t *inputs,
			struct BlackBoxCache * common,
			unsigned long lhsIndex,
			CONST char * context

);
/**<
 *  Construct an external relation from an external statement, a reference
 *  relation instance, a list of lists -- the arglist, a list of inputs,
 *  and a data instance which may be NULL. A copy is made of the arglist.
 *  @param relinst the RelationInstance this relation is owned by.
 *  @param efunc the external function table.
 *  @param arglist the formal parameters list of lists.
 *  @param whichvar the lhs (output) variable instance.
 *  @param inputs the flat list of input variables.
 *  @param data the arbitrary instance argument.
 *  @param common the data common to all relations in this blackbox set;
 *         this set is always made as an array, so common is passed in from
 *         the array instantiation loop.
 *  @param lhsIndex the index of whichvar in the blackbox efunc double vector.
 *  @param context the string name of the array instance parent.
 */

extern void DestroyRelation(struct relation *rel, struct Instance *relinst);
/**<
 *  Deallocate a relation.  This will notify all the real instances in
 *  the relation that it will no longer be pointing to them.  Therefore,
 *  this routine will have an effect on the instance tree.
 *  This routine is smart enough to deal with all the different relation types.
 */

extern void ModifyTokenRelationPointers(struct Instance *relinst,
                                        struct relation *rel,
                                        CONST struct Instance *old,
                                        CONST struct Instance *new_inst);
/**<
 *  Change all references in a token relation from old to new.  
 *  This doesn't do anything to the instances old and new.
 *  Note that this function will be slower when called on a relation
 *  with vars that were allocated during an instance copy function
 *  since the relation var list is not sorted.
 *  Given this and the criticality of copying in the compiler, the
 *  varlist should probably not be sorted in the first place.
 *
 *  @bug This applies to ModifyTokenRelationPointers(),
 *       ModifyGlassBoxRelPointers(), and ModifyBlackBoxRelPointers() in
 *       compiler/relation.[ch].  A token relation which ends up merging
 *       two variables in its varlist will need to split off a copy of the
 *       token list if the token list is shared among relations. There are
 *       penalties to merging vars now, too.
 */

extern void ModifyGlassBoxRelPointers(struct Instance *relinst,
                                      struct relation *rel,
                                      CONST struct Instance *old,
                                      CONST struct Instance *new_inst);
/**<
 *  Change all references in a glass box relation from old to new.
 *  This doesn't do anything to the instances old and new.
 *  @see ModifyTokenRelationPointers() for more information which
 *       applies to this function also.
 */

extern void ModifyBlackBoxRelPointers(struct Instance *relinst,
                                      struct relation *rel,
                                      CONST struct Instance *old,
                                      CONST struct Instance *new_inst);
/**<
 *  Change all references in a black box relation from old to new.
 *  This doesn't do anything to the instances old and new.
 *  @see ModifyTokenRelationPointers() for more information which
 *       applies to this function also.
 */

extern void DoInOrderVisit(struct relation_term *term,
                           struct relation *r,
                           void (*func)(struct relation_term *,
                                        struct relation *));
/**<
 *  Will visit an infix tree and apply function func to each term.
 *  It takes the relation corresponding to the tree as an argument, as some
 *  of the operations on terms are dependent upon the relation that the term
 *  belongs to.<br><br>
 *
 *  The func you supply is responsible for switching over the type of term
 *  it is called on.<br><br>
 *
 *  If the tree you are visiting is part of a token relation side,
 *  your visiting function should not free any nodes it visits
 *  as these are parts of an array.
 */

extern struct relation
*CopyRelationByReference(CONST struct Instance *src_instance,
                         struct Instance *target_instance,
                         struct gl_list_t *targvarlist);
/**<
 * Abbott version of relation reference copying.
 * src_instance is a completed rel instance.
 * target is a new rel instance with no struct relation or varlist at
 * home.
 * targetvarlist is the list of vars needed for targetinstance.
 * The struct relation share field is copied by reference from src_inst.
 */

extern struct relation
*CopyAnonRelationByReference(CONST struct Instance *anonproto,
                             struct Instance *target,
                             struct gl_list_t *copyvars,
                             void *bboxtable);
/**<
 * Version of relation copy-by-reference for use with the AnonType
 * approach to copying relations.
 * The tmpnums of the vars in the relation list of the anonproto instance
 * should give the index of the corresponding vars in the list
 * copyvars.
 * The target relation instance should be as yet unconfigured;
 * this function will copy the internals (share) of the struct relation
 * by upping the reference count and notify the interested copyvars
 * of the new relation they have. The needed new varlist for target
 * is also created and set.
 * @param bboxtable must be a struct pairlist_t *.
 * If the anonproto struct relation is NULL, this will quietly return NULL.
 */

extern struct relation
*CopyRelationToModify(CONST struct Instance *src_inst,
                      struct Instance *dest_inst,
                      struct gl_list_t *copylist);
/**<
 *  Given a source and a destination relation instance, will copy the
 *  relation structure from the source instance and return it. A unique
 *  variable list is required. This variable list represents the new
 *  variables that will be marked icident upon the relation. This function
 *  will handle all relations type (eventually). At the moment it only
 *  supports token relations and glassbox relations.<br><br>
 *
 *  The tokens in the struct relation * returned by this function are
 *  NOT independently allocated, they are in the postfix array.
 */

extern void RelationSetBinTokens(struct Instance *c,
                                 int btable, int bindex);
/**<
 * Resets the btable and bindex for the shared in a token
 * relation. If btable and bindex are not 0,0, then
 * this will report an error if the existing share does
 * not INT_MAX for btable and bindex.
 */

/* @} */

#endif /* ASC_RELATION_H */
