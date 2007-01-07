/*
 *  Type definitions for statements and statement lists.
 *  Tom Epperly
 *  August 10, 1989
 *  Version: $Revision: 1.28 $
 *  Version control file: $RCSfile: stattypes.h,v $
 *  Date last modified: $Date: 1998/04/21 23:49:57 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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

/** @file
 *  Type definitions for statements and statement lists.
 *  <pre>
 *  When #including stattypes.h, make sure these files are #included first:
 *         #include <limits.h>
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "expr_types.h"
 *         #include "list.h"
 *         #include "bit.h"
 *         #include "module.h"
 *         #include "vlist.h"
 *  </pre>
 *  @todo Complete documentation of stattypes.h.
 */

#ifndef ASC_STATTYPES_H
#define ASC_STATTYPES_H

/**	addtogroup compiler Compiler
	@{
*/

/** FOR statement ordering types. */
enum ForOrder {
  f_random,     /**< FOR/CREATE only */
  f_increasing, /**< FOR/DO INCREASING and FOR/DO default */
  f_decreasing  /**< FOR/DO DECREASING */
};

/** FOR statement kinds. */
enum ForKind {
  fk_create,  /**< for create */
  fk_do,      /**< for do */
  fk_check,   /**< for check */
  fk_expect   /**< for expect */
};

/** Codes for the varieties of flow control statement. */
enum FlowControl {
  fc_break,
  fc_continue,
  fc_fallthru,
  fc_return,
  fc_stop     /**< this one must be last or fix statio.c */
};

enum ExternalKind {
  ek_method = 0, /**< method call */
  ek_glass = 1, /**< glass box relations */
  ek_black = 2 /**< black box relations */
};

/** Statement types. */
enum stat_t {
  ALIASES = 0,  /**< ALIASES */
  ISA,          /**< IS_A */
  ARR,          /**< CREATE ARRAY i AND SIZE j FROM(list) */
  IRT,          /**< IS_REFINED_TO */
  ATS,          /**< ARE_THE_SAME */
  AA,           /**< ARE_ALIKE */
  FLOW,         /**< BREAK, CONTINUE, FALL_THROUGH, RETURN, STOP */
  FOR,          /**< FOR CREATE LOOP */
  REL,          /**< RELATION */
  LOGREL,       /**< LOGICAL RELATION */
  ASGN,         /**< ASSIGNMENT */
  CASGN,        /**< Structural ASSIGNMENT */
  RUN,          /**< RUN statement */
  FIX,          /**< FIX statement */
  FREE,			/**< FREE statement */
  ASSERT,       /**< TEST statement */
  IF,           /**< IF-ELSE statement */
  WHEN,         /**< WHEN statement */
  FNAME,        /**< Name of model or relation */
  SELECT,       /**< SELECT statement */
  SWITCH,       /**< SWITCH statement */
  WHILE,        /**< WHILE statement */
  EXT,          /**< EXTERNAL fcn/relation statement of abbott */
  CALL,         /**< user defined function statement */
  REF,          /**< REFERENCE statement */
  COND,         /**< CONDITIONAL statement */
  WBTS,         /**< WILL_BE_THE_SAME */
  WNBTS,        /**< WILL_NOT_BE_THE_SAME */
  WILLBE        /**< WILL_BE */
  /* if you add anything after WILLBE, change statio.c/suppression
   * accordingly.
   */
  /* Inserting before WILLBE is fine, though, don't forget to check
   * that the io routines handle the new statements.
   */
};

#define context_MODEL    0     /**< main declarative line */
#define context_FOR      0x1   /**< in a loop, declarative or method */
#define context_COND     0x2   /**< inside a CONDITIONAL statement*/
#define context_METH     0x4   /**< statement is in a method */
#define context_WRONG    0x8   /**< statement is uninstantiably bad */
#define context_MODPARAM 0x10  /**< statement is in model parameter list */
#define context_SELECT   0x20  /**< statement is in SELECT's statement list */
#define context_WHEN     0x40  /**< statement is in WHEN's statement list
                                   The context in WHENs is useful only in
                                   the case of a nested WHEN. It does not
                                   hurt to have this information though. */
#define context_IF       0x80  /**< statement is in IF's statement list */
#define context_SWITCH   0x100 /**< statement is in SWITCH's statement list */
#define context_WHILE    0x200 /**< statement is in WHILE's statement list */
#define context_MODWHERE 0x400 /**< statement is in model where list */

/*
 * Certain statement types are more easily interpreted if we cache
 * some information about where they appear within the MODEL.
 * These are the bit values we have detected a need for so far.
 * Since statements are defined with a union, we aren't paying extra
 * memory for the bits due to bizarre alignment restrictions.
 *
 * The StateIS keeps a string name of the type required for IS_A/
 * IS_REFINED_TO statements. This name will be of the type expected
 * in the type library by FindType when the statement is executed
 * during instantiation. ChildList information derived from this
 * name at parse time must be kept up to date with the type library.
 * This is not the responsibility of the statement module, but is
 * useful to know.
 */

struct WhenList {
  struct WhenList *next;        /**< next in list */
  struct Set *values;           /**< matching values of the case */
  struct StatementList *slist;  /**< statements to be executed */
};

struct SelectList {
  struct SelectList *next;      /**< next in list */
  struct Set *values;           /**< matching values of the case */
  struct StatementList *slist;  /**< statements to be executed */
};

struct SwitchList {
  struct SwitchList *next;      /**< next in list */
  struct Set *values;           /**< matching values of the case */
  struct StatementList *slist;  /**< statements to be executed */
};

/** Lets see if we can get by with no more than 6 members in any of
 * the individual statement types. 5-6 seems to be a functional
 * minimum.
 */
struct StateRUN {
  struct Name *proc_name; /**< procedure name */
  struct Name *type_name; /**< whether 'class access' eg a::b */
};

struct StateFIX {
  struct VariableList *vars; /**< Variable(s) to be fixed */
};

struct StateASSERT {
  struct Expr *test;
};

struct StateIF {
  struct Expr *test;      /**< expression to be tested */
  struct StatementList *thenblock;
  struct StatementList *elseblock;
};

struct StateCOND {
  struct StatementList *stmts;  /**< list of conditional log/relations */
  unsigned int contains;        /**< flags about what in COND stmts lists */
};

struct StateWHEN {
  struct Name *nptr;        /**< Name of the WHEN */
  struct VariableList *vl;  /**< list of variables */
  struct WhenList *cases;   /**< list of cases  */
};

struct StateFNAME {
  struct Name *wname;       /**< Name of model or a relation (WHEN)  */
};

struct StateSELECT {
  struct VariableList *vl;  /**<  list of variables */
  struct SelectList *cases; /**<  list of cases  */
  int n_statements;         /**< number of stmts in SELECT stmts lists */
  unsigned int contains;    /**< flags about what in SELECT stmts lists */
};


struct StateSWITCH {
  struct VariableList *vl;  /**< list of variables */
  struct SwitchList *cases; /**< list of cases  */
};

/** used for IS_A, IS_REFINED_TO, WILL_BE */
struct StateIS {
  struct VariableList *vl;  /**< all, but WILL_BE may want len=1. */
  symchar *type;            /**< all */
  struct Set *typeargs;     /**< all, parameter list. may be NULL */
  symchar *settype;         /**< IS_A only */
  struct Expr *checkvalue;  /**< WILL_BE only */
  /* note that checkvalue!=NULL and typeargs!=NULL are mutually exclusive
   * because checkvalues go with constants which are never parameterized.
   */
};

/** used for ARR, the compound ALIASES/set IS_A. */
struct CompoundAlias {
  struct VariableList *setname; /**< name of new set being defined, as varlist */
  int intset;                   /**< 0 -> symbol set, 1-> integer set */
  struct Set *setvals;          /**< set expression for values, if given */
};

/** 
 *  Used for ALIASES and IS (undefined). Eventually IS
 *  may be universe scope alii
 *  @todo fix this comment.
 */
struct StateAlias {
  struct VariableList *vl;
    /**< ALIASES:		LHS, new alii being defined  a,b,c ALIASES y
     * ARR:  RHS list of instances to point at a[s] ALIASES y[s2]
     * Ken has, quite reasonably, requested that vl be a set expression
     * so that set arithmetic can control which subset y[i]  | i IN foo
     * is usable. This may play hell on type checking -- need to see
     * what the rules are for determining the base type of instances
     * named with a set expression.
     */
  union {
    struct Name *nptr;            /**< ALIASES: RHS, name of object being pointed at */
    struct VariableList *avlname; /**< ARR: LHS, new alias being defined */
  } u;                            /**< union of name and alias name */
  struct CompoundAlias c;         /**< ARR use only */
};

/** used for ARE_THE_SAM,E ARE_ALIKE and */
struct StateARE {
  struct VariableList *vl;  /**< WILL_BE_THE_SAME, WILL_NOT_BE_THE_SAME. */
};

/** used for assignment :==,:= statements */
struct StateAssign {
  struct Name *nptr;
  struct Expr *rhs;
};

/** used for general external methods */
struct StateCall {
  symchar *id;      /**< global function called */
  struct Set *args; /**< list of arguments to function */
};

/** used for relations */
struct StateRelation {
  struct Name *nptr;
  struct Expr *relation;
};

/** used for logical relations */
struct StateLogicalRel {
  struct Name *nptr;
  struct Expr *logrel;
};

/** legacy external methods in METHODS section */
struct StateExternalMethod {
	struct VariableList *vl; /**< list of arguments */
};

/** Black or glass box equation model. */ 
struct StateExternalRelation {
  struct Name *nptr;        /**< name of the statement */
  struct VariableList *vl;  /**< list of arguments */
};

/** Black box equation model. Top must match StateExternalRelation. */
struct StateExternalBlackBox {
  struct Name *nptr;        /**< name of the statement */
  struct VariableList *vl;  /**< list of arguments */
  struct Name *data;        /**< additional user data */
};

/* blackboxes contain an implicit for loop. For
internal compiler processing, we make this explicit
with a loop index name BBOX_RESERVED_INDEX
*/
#define BBOX_RESERVED_INDEX "?BBOX_OUTPUT"
#define BBOX_RESERVED_INDEX_LEN 12
  
/** Glassbox equation model. Top must match StateExternalRelation. */ 
struct StateExternalGlassBox {
  struct Name *nptr;        /**< name of the statement */
  struct VariableList *vl;  /**< list of arguments */
  struct Name *data;        /**< additional user data */
  struct Name *scope;       /**< scope to add the external relations for glassboxes */
};
  

/** used for external statements */
struct StateExternal {
  enum ExternalKind mode;     /**< 0=procedural, 1=glassbox, 2=blackbox */
  CONST char *extcall;      /**< name of the function */
  union {
    struct StateExternalRelation relation; /* which is either glass or black */
    struct StateExternalGlassBox glass;
    struct StateExternalBlackBox black;
    struct StateExternalMethod method;
  } u;
};

struct StateReference {
  int mode;                 /**< clone or reference */
  symchar *ref_name;        /**< thing being referenced */
  symchar *settype;
  struct VariableList *vl;
};

struct StateFlow {
  enum FlowControl fc;        /**< what control flow change */
  struct bracechar *message;  /**< message on change, if any */
};

/**< used for FOR loops */
struct StateFOR {
  symchar *index;
  struct Expr *e;
  struct StatementList *stmts;
  enum ForOrder order;
  enum ForKind kind;
  /* NOTE: the following bits are recursive. If stmts contains a for
   * loop which contains relations, then thereby this for loop does too.
   */
  unsigned int contains;    /**< bit flags about what in for statement list */
};

/**< used for While loops */
struct StateWhile {
  struct Expr *test;
  struct StatementList *block;
};

/**
 * The following bits are used for BOTH FOR statements and SELECT
 * statements. VRR
 */
#define contains_REL 0x1        /**< TRUE if relations in stmts list */
#define contains_DEF 0x2        /**< true if default statements in stmts list */
#define contains_CAS 0x4        /**< true if structural var assignments in stmts list*/
#define contains_ALI 0x8        /**< true if aliases in stmts list */
#define contains_ISA 0x10       /**< true if IS_A in stmts list */
#define contains_WILLBE 0x20    /**< true if IS_A in stmts list */
#define contains_ATS 0x40       /**< true if ARE_THE_SAME in stmts list */
#define contains_IRT 0x80       /**< true if IS_REFINED_TO in stmts list */
#define contains_LREL 0x100     /**< TRUE if logical relations in stmts list */
#define contains_WHEN 0x200     /**< TRUE if WHENs in stmts list */
#define contains_AA 0x400       /**< true if aliases in stmts list */
#define contains_WBTS 0x800     /**< true if WILL_BE_THE_SAME in stmts list */
#define contains_ARR 0x1000     /**< true if CREATE ARRAY in stmts list */
#define contains_WNBTS 0x2000   /**< true if WILL_NOT_BE_THE_SAME in stmts list */
#define contains_SELECT 0x4000  /**< true if SELECT in stmts list */
#define contains_COND 0x8000    /**< true if CONDITIONAL in stmts list
                                     I am doing this seeking for a *
                                     generalization. IMHO, a CONDITIONAL
                                     should always be only in the context of a
                                     MODEL, and any loop should be inside the
                                     CONDITIONAL statement. VRR */
#define contains_EXT 0x10000	/*< contains a External statement. */
#define contains_ILL 0x80000    /**< true if illegal statement in loop */
/* unsupported values, meaning we should be using them but don't yet */

union StateUnion {
  struct StateAlias      ali;
  struct StateIS         i;
  struct StateARE        a;
  struct StateAssign     asgn;
  struct StateRelation   rel;
  struct StateLogicalRel lrel;
  struct StateFOR        f;
  struct StateRUN        r;
  struct StateFIX		 fx;
  struct StateCall       call;
  struct StateIF         ifs;
  struct StateASSERT     asserts;
  struct StateWHEN       w;
  struct StateFNAME      n;
  struct StateSELECT     se;
  struct StateSWITCH     sw;
  struct StateExternal   ext;
  struct StateReference  ref;
  struct StateCOND       cond;
  struct StateWhile      loop;
  struct StateFlow       flow;
};

struct Statement {
  enum stat_t t;
  unsigned int context;   /**< tells about context and correctness */
  union StateUnion v;
  struct module_t *mod;   /**< module where statement occurs */
  unsigned long linenum;  /**< line number where statement occurs */
  char *stringform;       /**< character string in {} list format */
  REFCOUNT_T ref_count;   /**< number of references to this structure */
};

struct StatementList {
  REFCOUNT_T ref_count;
  struct gl_list_t *l;
};

/* @} */

#endif  /* ASC_STATTYPES_H */

