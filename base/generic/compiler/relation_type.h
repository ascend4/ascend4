/*
 *  Relation Data Type
 *  by Tom Epperly
 *  Created: 1/18/90
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: relation_type.h,v $
 *  Date last modified: $Date: 1998/01/06 12:05:31 $
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
 *  Relation Data Type.
 *  <pre>
 *  When #including relation_type.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "list.h"
 *         #include "func.h"
 *         #include "extfunc.h"
 *         #include "extcall.h"
 *  </pre>
 */

#ifndef __RELATION_TYPE_H_SEEN__
#define __RELATION_TYPE_H_SEEN__

/** If TOKENDOMINANT is 1, then we assume union RelationUnion fields
 * other than relop and ref_count are all going to be initialized
 * to 0/NULL if we do the initializations for the token unions.
 * If it is 0, we do a memset (bytewise initialization) which
 * is slower.
 */
#define TOKENDOMINANT 1

/*
 *  The following relation term types should only be accessed through the
 *  operators in relation.h.
 */

/** constants invariant with loop index */
struct RelationReal{
  /* this is the dominant size term */
  enum Expr_enum t;   /**< type of term */
  CONST dim_type *dimensions;
  double value;
};

struct RelationInteger {
  enum Expr_enum t;   /**< type of term */
  long ivalue;
};

/* constants varying with loop index, coming soon to a compiler near you. */
/* The purpose of these is to allow isomorphic relations, up to the values
 * of the constant coefficients, to share a single token string. This can
 * only be accomplished in a second visitation of relation arrays after the
 * relations of the array in question have been created and simplified.
 */
struct RelationRealConst{
  enum Expr_enum t;   /**< type of term */
  CONST dim_type *dimensions;
  int cnum;           /**< index in constant array, from 0 */
};

struct RelationIntegerConst {
  enum Expr_enum t; /**< type of term */
  int cnum;         /**< index in constant array, from 0 */
};

/** real variables */
struct RelationVar {
  enum Expr_enum t;     /**< type of term */
  unsigned int flags;   /**< flags for future use */
  unsigned long varnum; /**< index in variable gl_list, from 1 */
};

/** operators */
struct RelationFunc {
  enum Expr_enum t;     /**< type of term */
  unsigned int flags;   /**< flags for future use */
  CONST struct Func *fptr;
  struct relation_term *left;
};

/** unary minus */
struct RelationUnary {
  enum Expr_enum t;     /**< type of term */
  unsigned int flags;   /**< flags for future use */
  struct relation_term *left;
};

/** binary, plus,minus,divide,times,power */
struct RelationBinary {
  enum Expr_enum t;     /**< type of term */
  unsigned int flags;   /**< flags for future use */
  struct relation_term *left;
  struct relation_term *right;
};

/** future existence */
struct RelationNary {
  enum Expr_enum t;     /**< type of term */
  unsigned int flags;   /**< flags for future use */
  CONST struct Func *fptr;
  struct gl_list_t *args;
};

/** New token type designed just as struct Instance */
struct relation_term {
  enum Expr_enum t;   /**< type of term */
  unsigned int flags; /**< flags for future use. only valid for operator and var terms */
};

/**
 * A union type for sizeof folks and other nosy sorts.
 * The size should be 16 bytes on 4byte ptr machines and 24 on
 * fat pointer machines.
 * Not really intended for actual use except in sizeof and array declarations.
 * All members of this union are able to align on N byte boundaries
 * where N is sizeof(union RelationTermUnion) for the machine in question.
 * Do not try to pack the union subtypes into an array like atom children.
 */
union RelationTermUnion {
  struct relation_term anon;      /**< anonymous relation term type */
  struct RelationInteger i;       /**< integer value */
  struct RelationReal r;          /**< real value */
  struct RelationIntegerConst ic; /**< integer value */
  struct RelationRealConst rc;		/**< real value */
  struct RelationVar var;         /**< vars */
  struct RelationFunc func;       /**< funcs */
  struct RelationUnary uni;       /**< standard unary function */
  struct RelationBinary bin;      /**< standard binary functions */
  struct RelationNary n;          /**< numeric functions with a list argument */
};

/*
 * The different types of relation structures that need to
 * be supported by the relation module.
 */

/** Each RelationUnion element should start with these two entries
 * so we can ask a the RelationUnion its relop, rather than storing
 * the information redundantly in every struct relation refering
 * to the shared info.
 * Keep TokenRelation, OpCodeRelation, GlassBoxRelation, BlackBoxRelation
 * matched to this. And see the definition of TOKENDOMINANT above
 * if these structs are changed.
 */
struct SharedRelation {
  enum Expr_enum relop; /**< relation kind etc */
  REFCOUNT_T ref_count; /**< number of instances looking here */
};

/**  TokenRelations:
 *  Under NO CIRCUMSTANCES should you attempt to free any element
 *  of the infix trees. They share memory with the postfix arrays.
 *  Also you should not dereference the pointers in a relation
 *  except by appropriate operators from the header.
 */
struct TokenRelation {
  enum Expr_enum relop;     /**< type of relation */
  REFCOUNT_T ref_count;     /**< number of instances looking here */
  unsigned long lhs_len, rhs_len;
  union RelationTermUnion *lhs, *rhs;   /**< postfix arrays */
  struct relation_term *lhs_term, *rhs_term;    /**< infix trees */
  unsigned btable, bindex;  /**< indices to table and entry of machine code */
};

/** Unimplemented OpCodes */
struct OpCodeRelation {
  enum Expr_enum relop;   /**< type of constraint */
  REFCOUNT_T ref_count;   /**< number of instances looking here */
  int *lhs, *rhs;         /**< array of opcodes */
  int *args;
  int nargs;
  double *constants;      /**< array of reals */
};

struct GlassBoxRelation {
  enum Expr_enum relop;   /**< type of constraint */
  REFCOUNT_T ref_count;   /**< number of instances looking here */
  struct ExternalFunc *efunc;
  int *args;              /**< an array of indexes into the varlist */
  int nargs;
  int index;              /**< the *external* index of this relation */
};

struct BlackBoxRelation {
  enum Expr_enum relop;     /**< type of constraint */
  REFCOUNT_T ref_count;     /**< number of instances looking here */
  struct ExtCallNode *ext;  /**< external call info */
  int *args;                /**< an array of indexes into the varlist */
  int nargs;
};

union RelationUnion {
  struct SharedRelation s;
  struct TokenRelation token;
  struct OpCodeRelation opcode;
  struct GlassBoxRelation gbox;
  struct BlackBoxRelation bbox;
};

/** a union type for double and long constants. */
union doublong {
  double dval;
  long lval;
};

/**
 * Most of the attributes in this structure are instance
 * attributes that cannot be shared among relation instances,
 * not attributes of the relation recipe for calculating them.
 * The calculation recipe, 'share' is sharable among all relations with
 * identical symbolic form; roughly 80+% of relations
 * in physical models are duplicates in differing contexts.
 * That part (calculation recipe) which is instance independent
 * is stored in the union 'share'. <br><br>
 *
 * Share contains information mapping from the index in *vars
 * to value slots in the calculation. Each RelationInstance
 * has its own struct relation and vars gl_list-- if the
 * vars list is changed for one of the instances sharing
 * a the RelationUnion, then a new share structure must
 * be created for that instance. If this is not done,
 * the other instances will miscalculate or core dump.
 * Usually just miscalculates in silence.<br><br>
 *
 * To compile really large models, these must be memory
 * pooled and properly aligned.
 */
struct relation {
  union RelationUnion *share; /**< should never be NULL but at creation */
  double residual;
  double multiplier;
  double nominal;
  int iscond;
  struct gl_list_t *vars;     /**< list starting from 1 of RealAtomInst ptrs */
/* coming soon.
 * union doublong *constants;	 loop variant constants. maybe NULL.
 */
  dim_type *d;
};

/* casts to fix things up, should they really be needed. */
#define A_TERM(i) ((struct relation_term *)(i))
/* anonymous term */
#define R_TERM(i) ((struct RelationReal *)(i))
#define I_TERM(i) ((struct RelationInteger *)(i))
#define RC_TERM(i) ((struct RelationRealConst *)(i))
#define IC_TERM(i) ((struct RelationIntegerConst *)(i))
#define V_TERM(i) ((struct RelationVar *)(i))
#define L_TERM(i) ((struct RelationLogical *)(i))
#define F_TERM(i) ((struct RelationFunc *)(i))
#define B_TERM(i) ((struct RelationBinary *)(i))
#define U_TERM(i) ((struct RelationUnary *)(i))
#define N_TERM(i) ((struct RelationNary *)(i))
#define UNION_TERM(i) ((union RelationTermUnion *)(i))

/**
 * The following define is for people who expect each term
 * allocated to be individually deallocated and interchangable
 * to all types of term. It returns a struct relation_term *.
 * Cast as needed if you must.
 * Use of individually allocated terms is a really bad idea!
 */
#define TERM_ALLOC A_TERM(ascmalloc(sizeof(union RelationTermUnion)))

#endif /* __RELATION_TYPE_H_SEEN__ */

