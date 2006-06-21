/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
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
*//**
	@file
	Expression Module

	Requires:
	#include "utilities/ascConfig.h"
	#include "fractions.h"
	#include "compiler.h"
	#include "dimen.h"
	#include "expr_types.h"
*//*
	by Tom Epperly
	Last in CVS: $Revision: 1.11 $ $Date: 1998/02/05 16:36:00 $ $Author: ballan $
*/

#ifndef ASC_EXPRS_H
#define ASC_EXPRS_H

extern struct Expr *CreateVarExpr(struct Name *n);
/**<
 *  Create a name type expr node.
 */

extern struct Expr *CreateDiffExpr(struct Name *n);
/**<
	Create a time-derivative-of-a-single-variable node. Currently just returns
	the same as 0.0 (a real-valued expression with wildcard dimensions)
*/

extern void InitVarExpr(struct Expr *e, CONST struct Name *n);
/**< 
 *  Given an empty expr node, initialize it to contain the name.
 *  Generally this is only used to init a temporary expr node
 *  that you want to be able to destroy later (or forget later)
 *  without destroying the contents -- in this case name.
 *  How you create and destroy e is your business. using stack space
 *  is suggested.<br><br>
 *
 *  The problem with creating a varexpr with a name you want to keep
 *  after the node dies is that the name is destroyed when the node is.
 */

extern struct Expr *CreateOpExpr(enum Expr_enum t);
/**< 
 *  Create an operator node.
 */

extern struct Expr *CreateSatisfiedExpr(struct Name *n, 
                                        double tol, 
                                        CONST dim_type *dims);
/**<
 *  Create an satisfied operator node.
 */

extern struct Expr *CreateFuncExpr(CONST struct Func *f);
/**< 
 *  Create a function node.
 */

extern struct Expr *CreateIntExpr(long i);
/**< 
 *  Create an integer node.
 */

extern struct Expr *CreateRealExpr(double r, CONST dim_type *dims);
/**< 
 *  Create a real node with value r and dimensions "dims".
 */

extern struct Expr *CreateTrueExpr(void);
/**< 
 *  Create a boolean node with value TRUE.
 */

extern struct Expr *CreateFalseExpr(void);
/**< 
 *  Create a boolean node with value FALSE.
 */

extern struct Expr *CreateAnyExpr(void);
/**< 
 *  Create a boolean node with undefined value. b_value will be 2.
 */

extern struct Expr *CreateSetExpr(struct Set *set);
/**< 
 *  Create a set node.
 */

extern struct Expr *CreateSymbolExpr(symchar *sym);
/**< 
 *  Create a symbol node.
 */

extern struct Expr *CreateQStringExpr(CONST char *qstring);
/**< 
 *  Create a string node. The difference is that string may contain
 *  anything and are quoted as "qstring is string", whereas symbols
 *  are of the form 'symbol' and may have content restrictions.
 */

extern struct Expr *CreateBuiltin(enum Expr_enum t, struct Set *set);
/**< 
 *  Create a node for SUM, PROD, UNION, etc....
 */

extern void LinkExprs(struct Expr *cur, struct Expr *next);
/**< 
 *  Link cur to next.
 */

extern unsigned long ExprListLength(CONST struct Expr *e);
/**< 
 *  Traverse list to the end to find the length.
 *  Sometimes one would like to know the length a priori.
 */

#ifdef NDEBUG
#define NextExpr(e) ((e)->next)
#else
#define NextExpr(e) NextExprF(e)
#endif
/**<
 *  Return the expr node linked to e.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the linked node as type <code>struct Expr*</code>.
 *  @see NextExprF()
 */
extern struct Expr *NextExprF(CONST struct Expr *e);
/**<
 *  Return the expr node linked to e.
 *  Implementation function for NextExpr().  Do not use this function
 *  directly - use NextExpr() instead.
 */

#ifdef NDEBUG
#define ExprType(e) ((e)->t)
#else
#define ExprType(e) ExprTypeF(e)
#endif
/**<
 *  Return the type of e.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the type as a <code>enum Expr_enum</code>.
 *  @see ExprTypeF()
 */
extern enum Expr_enum ExprTypeF(CONST struct Expr *e);
/**<
 *  Return the type of e.
 *  Implementation function for ExprType().  Do not use this function
 *  directly - use ExprType() instead.
 */

#ifdef NDEBUG
#define ExprName(e) ((e)->v.nptr)
#else
#define ExprName(e) ExprNameF(e)
#endif
/**<
 *  Return the name field of a var type expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the name as a <code>CONST struct Name*</code>.
 *  @see ExprNameF()
 */
extern CONST struct Name *ExprNameF(CONST struct Expr *e);
/**<
 *  Return the name field of a var type expr node.
 *  Implementation function for ExprName().  Do not use this function
 *  directly - use ExprName() instead.
 */

#ifdef NDEBUG
#define ExprFunc(e) ((e)->v.fptr)
#else
#define ExprFunc(e) ExprFuncF(e)
#endif
/**<
 *  Return the func field of a function type expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the func as a <code>CONST struct Func*</code>.
 *  @see ExprFuncF()
 */
extern CONST struct Func *ExprFuncF(CONST struct Expr *e);
/**<
 *  Return the func field of a function type expr node.
 *  Implementation function for ExprFunc().  Do not use this function
 *  directly - use ExprFunc() instead.
 */

#ifdef NDEBUG
#define ExprIValue(e) ((e)->v.ivalue)
#else
#define ExprIValue(e) ExprIValueF(e)
#endif
/**<
 *  Return the integer value of a integer type expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as a <code>long</code>.
 *  @see ExprIValueF()
 */
extern long ExprIValueF(CONST struct Expr *e);
/**<
 *  Return the integer value of a integer type expr node.
 *  Implementation function for ExprIValue().  Do not use this function
 *  directly - use ExprIValue() instead.
 */

#ifdef NDEBUG
#define ExprRValue(e) ((e)->v.r.rvalue)
#else
#define ExprRValue(e) ExprRValueF(e)
#endif
/**<
 *  Return the real value of a real type expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as a <code>double</code>.
 *  @see ExprRValueF()
 */
extern double ExprRValueF(CONST struct Expr *e);
/**<
 *  Return the real value of a real type expr node.
 *  Implementation function for ExprRValue().  Do not use this function
 *  directly - use ExprRValue() instead.
 */

#ifdef NDEBUG
#define ExprRDimensions(e) ((e)->v.r.dimensions)
#else
#define ExprRDimensions(e) ExprRDimensionsF(e)
#endif
/**<
 *  Return the dimensions of a real type expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as a <code>CONST dim_type*</code>.
 *  @see ExprRDimensionsF()
 */
extern CONST dim_type *ExprRDimensionsF(CONST struct Expr *e);
/**<
 *  Return the dimensions of a real type expr node.
 *  Implementation function for ExprRDimensions().  Do not use this function
 *  directly - use ExprRDimensions() instead.
 */

#ifdef NDEBUG
#define SatisfiedExprName(e) ((e)->v.se.sen)
#else
#define SatisfiedExprName(e) SatisfiedExprNameF(e)
#endif
/**<
 *  Return the name field of a var type satisfied expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the name as a <code>CONST struct Name*</code>.
 *  @see SatisfiedExprNameF()
 */
extern CONST struct Name *SatisfiedExprNameF(CONST struct Expr *e);
/**<
 *  Return the name field of a var type satisfied expr node.
 *  Implementation function for SatisfiedExprName().  Do not use this function
 *  directly - use SatisfiedExprName() instead.
 */

#ifdef NDEBUG
#define SatisfiedExprRValue(e) ((e)->v.se.ser.rvalue)
#else
#define SatisfiedExprRValue(e) SatisfiedExprRValueF(e)
#endif
/**<
 *  Return the real value of a real type satisfied expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as a <code>double</code>.
 *  @see SatisfiedExprRValueF()
 */
extern double SatisfiedExprRValueF(CONST struct Expr *e);
/**<
 *  Return the real value of a real type satisfied expr node.
 *  Implementation function for SatisfiedExprRValue().  Do not use this function
 *  directly - use SatisfiedExprRValue() instead.
 */

#ifdef NDEBUG
#define SatisfiedExprRDimensions(e) ((e)->v.se.ser.dimensions)
#else
#define SatisfiedExprRDimensions(e) SatisfiedExprRDimensionsF(e)
#endif
/**<
 *  Return the dimensions of a real type satisfied expr node.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the dimension as a <code>CONST dim_type*</code>.
 *  @see SatisfiedExprRDimensionsF()
 */
extern CONST dim_type *SatisfiedExprRDimensionsF(CONST struct Expr *e);
/**<
 *  Return the dimensions of a real type satisfied expr node.
 *  Implementation function for SatisfiedExprRDimensions().  Do not use this function
 *  directly - use SatisfiedExprRDimensions() instead.
 */

#ifdef NDEBUG
#define ExprBValue(e) ((e)->v.bvalue)
#else
#define ExprBValue(e) ExprBValueF(e)
#endif
/**<
 *  Return the boolean value of a boolean type satisfied expr node.
 *  Returns 1 if e is TRUE, 0 if e is FALSE, 2 if e is ANY.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as an <code>int</code>.
 *  @see ExprBValueF()
 */
extern int ExprBValueF(CONST struct Expr *e);
/**<
 *  Return 1 if e is TRUE, 0 if e is FALSE, 2 if e is ANY.
 *  Implementation function for ExprBValue().  Do not use this function
 *  directly - use ExprBValue() instead.
 */

#ifdef NDEBUG
#define ExprSValue(e) ((e)->v.s)
#else
#define ExprSValue(e) ExprSValueF(e)
#endif
/**<
 *  Return the set value of a set node type.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as an <code>struct Set*</code>.
 *  @see ExprSValueF()
 */
extern struct Set *ExprSValueF(CONST struct Expr *e);
/**<
 *  Return the set value of a set node type.
 *  Implementation function for ExprSValue().  Do not use this function
 *  directly - use ExprSValue() instead.
 */

#ifdef NDEBUG
#define ExprSymValue(e) ((e)->v.sym_ptr)
#else
#define ExprSymValue(e) ExprSymValueF(e)
#endif
/**<
 *  Return the symbol pointer value from a symbol node type.
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as an <code>symchar*</code>.
 *  @see ExprSymValueF()
 */
extern symchar *ExprSymValueF(CONST struct Expr *e);
/**<
 *  Return the symbol pointer value from a symbol node type.
 *  Implementation function for ExprSymValue().  Do not use this function
 *  directly - use ExprSymValue() instead.
 */

#ifdef NDEBUG
#define ExprQStrValue(e) ((e)->v.sym_ptr)
#else
#define ExprQStrValue(e) ExprQStrValueF(e)
#endif
/**<
 *  Return the string pointer value from a string node type.
 *  The difference between a string and a symbol, is that the former
 *  may contain whitespace. The type is called e_qstring
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the value as an <code>CONST char*</code>.
 *  @see ExprQStrValueF()
 */
extern CONST char *ExprQStrValueF(CONST struct Expr *e);
/**<
 *  Return the string pointer value from a string node type.
 *  The difference between a string and a symbol, is that the former
 *  may contain whitespace. The type is called e_qstring
 *  Implementation function for ExprQStrValue().  Do not use this function
 *  directly - use ExprQStrValue() instead.
 */

#ifdef NDEBUG
#define ExprBuiltinSet(e) ((e)->v.s)
#else
#define ExprBuiltinSet(e) ExprBuiltinSetF(e)
#endif
/**<
 *  Return the set argument for one of the builtin operations.  
 *  SUM, PROD, CARD, etc..
 *  @param e <code>CONST struct Expr*</code>, the expr to query.
 *  @return Returns the set as an <code>CONST struct Set*</code>.
 *  @see ExprBuiltinSetF()
 */
extern CONST struct Set *ExprBuiltinSetF(CONST struct Expr *e);
/**<
 *  Return the set argument for one of the builtin operations.
 *  SUM, PROD, CARD, etc. 
 *  Implementation function for ExprBuiltinSet().  Do not use this function
 *  directly - use ExprBuiltinSet() instead.
 */

extern struct Expr *CopyExprList(CONST struct Expr *e);
/**< 
 *  Make and return a copy of e.
 */

extern void DestroyExprList(struct Expr *e);
/**< 
 *  Deallocate all the memory associated with e.
 *  Handles NULL input gracefully.
 */

extern struct Expr *JoinExprLists(struct Expr *e1, struct Expr *e2);
/**< 
 *  Append list e2 to the end of e1.  This returns e1, unless e1
 *  is NULL in which case it returns e2.
 */

extern int ExprsEqual(CONST struct Expr *e1, CONST struct Expr *e2);
/**< 
 *  Return TRUE if and only if e1 and e2 are structurally equivalent.
 */

extern int CompareExprs(CONST struct Expr *e1, CONST struct Expr *e2);
/**< 
 *  Compares2 expressions.
 *  Return -1, 0, 1 as e1 is < == > e2.
 *  Expressions being complicated things, this is not easily
 *  explained. The expressions are being compared in the
 *  absence of an instance context, so we're looking for
 *  structural differences.
 *  The NULL Expr > all Expr.
 */

extern void exprs_init_pool(void);
/**< 
 * Starts memory recycle. do not call twice before stopping recycle.
 */

extern void exprs_destroy_pool(void);
/**< 
 * Stops memory recycle. do not call while ANY Expr are outstanding.
 */

extern void exprs_report_pool(void);
/**< 
 * Write the pool report to ASCERR for the exprs pool.
 */

#endif /* ASC_EXPRS_H */

