/*	ASCEND modelling environment
	Copyright (C) 2006, 2007 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

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
*//*
	@file
	Name, set and expr type definitions

	@todo Complete documentation of types.h.

	Renamed from 'types.h' -- JP, 21 Jun 2006.
*//*
	by Tom Epperly
	July 31, 1989
	Last in CVS $Revision: 1.17 $ $Date: 1998/03/25 00:32:31 $ $Author: ballan $
*/

#ifndef ASC_TYPES_H
#define ASC_TYPES_H

/**	@addtogroup compiler Compiler
	@{
*/

#include <limits.h>
#include "compiler.h"
#include "dimen.h"
#include <general/list.h>
#include "functype.h"

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * Please see usage notes at top of Expr_enum before modifying it.
 * In particular, if you mess with it, make sure the defines still work.
 */
#define NUM_EXPR_ENUMS 44
/**
 * Expression token types. Note that several have
 * multiple uses. Where more than 1 use is listed, only the first
 * can be encountered in real valued equations.
 * If you must add a type, do so after e_ipower.
 * If you explicitly use the values of the enum instead of the
 * symbolic name, put the string TOKEN_ENUM in a comment at
 * the top of the function so that dependent code is easy to find
 * in the event of drastic changes. If appropriate, also put in header.
 * If you assign explicit values, do so without leaving a gap anywhere
 * as the statistics code in check.c depends on values being 0-NUM_EXPR_ENUMS-1
 */
enum Expr_enum {
  /* shouldn't be seen, but just in case */
  e_nop		= 0,     /**< null instruction */

  /**< relation implementation kinds */
  e_undefined	= 1, /**< relation type undefined */
  e_glassbox	= 2, /**< glassbox relation type */
  e_blackbox	= 3, /**< blackbox relation type */
  e_opcode	= 4,   /**< unimplemented opcode relation type. not seen */
  e_token	= 5,     /**< token array infix/postfix relation type */

  e_zero	= 6,     /**< symbolic processing <==> quiet wild real 0.0 */
  e_real	= 7,     /**< real constant invariant across for loops */
  e_int		= 8,     /**< integer constant invariant across for loops */
/* two types only to be seen in fully compiled token relations, and not
  yet seen at all. These are the types for RelationRealConst terms and
  RelationIntegerConst terms.
  e_realconst  = 7a,
  e_intconst  = 8a,
*/
  e_var		= 9,     /**< real variable */

  e_uminus	= 10,  /**< unary minus */
  e_func	= 11,    /**< unary real function */

  e_plus	= 12,    /**< addition, set union */
  e_minus	= 13,    /**< subtraction, set difference */
  e_times	= 14,    /**< multiplication, set intersection */
  e_divide	= 15,  /**< division */
  e_power	= 16,    /**< a^x */
  e_ipower	= 17,  /**< a^n */
  /* Don't mess with the ordering of the above items */

  /* Boolean Relations  */
  e_bol_token,

  /* relational operators, at present somewhat overloaded. may be in any kind*/
  e_notequal,     /**< <> strict real inequality */
  e_equal,        /**< =, rather overloaded */
  e_less,         /**< < strict real inequality */
  e_greater,      /**< > strict real inequality */
  e_lesseq,       /**< <=, =< tolerant real inequality */
  e_greatereq,    /**< >=, => tolerant real inequality */
  e_maximize,     /**< MAXIMIZE by convention implies a unary minus, we minimize */
  e_minimize,     /**< MINIMIZE objective function. max and min both become lhs */
  e_boolean_eq,   /**< boolean equality */
  e_boolean_neq,  /**< boolean inequality */

  /* ALL following are never encountered in compiled real relations */
  e_boolean,      /**< boolean constant */
/*e_var_bool,	   unimplemented boolean variable */
  e_or,           /**< OR */
  e_and,          /**< AND, ",", + */
  e_not,          /**< NOT */
  e_satisfied,    /**< Operator for boolean relations */

  /* ALL following are never encountered in compiled bool relations */
  e_subexpr,      /**< intermediate symbolic processing */
  e_const,        /**< intermediate symbolic processing */
  e_par,          /**< intermediate symbolic processing */
  e_sum,          /**< unexpanded summation */
  e_prod,         /**< unexpanded product */

  e_symbol,       /**< symbol constant, symbol variable. needs to be fixed*/
/*e_var_sym	unimplemented symbol variable */
  e_set,          /**< [set definition] */
  e_in,           /**< IN */
  e_st,           /**< | (such that) */
  e_card,         /**< CARD (set cardinality) */
  e_choice,       /**< CHOICE (set pick one arbitrary but consistent) */
  e_union,        /**< UNION (set union aka ",") */
  e_inter,        /**< INTERSECTION (set intersection, aka "*") */

  /* miscellany */
  e_qstring       /**< a quoted string */
};

#define TOK_SCALAR_LOW    e_zero
#define TOK_SCALAR_HIGH   e_var
#define TOK_CONSTANT_LOW  e_zero
#define TOK_CONSTANT_HIGH e_int
#define TOK_XARY_LOW      e_uminus
#define TOK_XARY_HIGH     e_ipower
#define TOK_BINARY_LOW    e_plus
#define TOK_BINARY_HIGH   e_ipower
#define TOK_UNARY_LOW     e_uminus
#define TOK_UNARY_HIGH    e_func
#define TOK_REAL_REL_LOW  e_zero
#define TOK_REAL_REL_HIGH e_ipower
#define TOK_REL_TYPE_LOW  e_glassbox
#define TOK_REL_TYPE_HIGH e_token

struct ExprReal {
  double rvalue;
  CONST dim_type *dimensions;
};

struct SatisfiedExpr {
  struct ExprReal ser;
  struct Name *sen;
};

union ExprUnion {
  struct Name *nptr;
  long ivalue;
  symchar *sym_ptr;   /**< symbol pointer */
  struct Set *s;
  struct ExprReal r;
  CONST struct Func *fptr;
  unsigned bvalue;    /**< boolean value true or false */
  struct SatisfiedExpr se;
};

struct Expr {
  struct Expr *next;
  enum Expr_enum t;
  union ExprUnion v;
};

struct Range {
  struct Expr *lower,*upper;
};

union SetUnion {
  struct Expr *e;   /**< single value */
  struct Range r;   /**< range of values lower..upper */
};

struct Set {
  struct Set *next;
  unsigned range;   /**< 0 single expression, 1 range */
  union SetUnion val;
  unsigned long ref_count;
};

union NameUnion {
  symchar *id;      /**< symbol table member */
  struct Set *s;
  int attribute;    /**< supported attribute name index */
};

struct Name {
  /* this element info */
  unsigned int bits;
  /**<
   * if NAMEBIT_IDTY 1,
   * this name is an identifier (id) name. if 0, it's a set.
   * if NAMEBIT_ATTR 1,
   * this name is a supported attribute id that should be
   * printed with a $ in front of it.
   * if NAMEBIT_CHAT 1,
   * chain this is part of contains a supported attribute
   * element. This is determinable at parse time and it costs no
   * more to store that fact here for all elements in the chain.
   * This makes it possible to test conditionals much earlier,
   * perhaps at parse stage.
   * if NAMEBIT_AUTO 1,
   * this name element is system generated.
   */
#define NAMEBIT_IDTY	0x1
#define NAMEBIT_ATTR	0x2
#define NAMEBIT_CHAT	0x4
#define NAMEBIT_AUTO	0x8
  struct Name *next;
  union NameUnion val;
};

/**
 * MAXINTREAL
 * This is the largest e_real number we will convert to an e_int
 * in certain relation simplifications.
 * Note that there is an upper limit beyond which double precision
 * floating point numbers cannot map uniquely to ints and back again.
 * On DEC alphas (ieee 64 bit), this was found to be near 130000.
 * The IEEE gurus will tell you OTHERWISE, of course.
 */
#define MAXINTREAL 100000

/* @} */

#endif /* ASC_TYPES_H */

