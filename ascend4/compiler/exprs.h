/*
 *  Expression Module
 *  by Tom Epperly
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: exprs.h,v $
 *  Date last modified: $Date: 1998/02/05 16:36:00 $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

#ifndef __EXPRS_H_SEEN__
#define __EXPRS_H_SEEN__

/*
 *  When #including exprs.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 */


extern struct Expr *CreateVarExpr(struct Name *);
/*
 *  struct Expr *CreateVarExpr(n)
 *  struct Name *n;
 *  Create a name type expr node.
 */

extern void InitVarExpr(struct Expr *,CONST struct Name *);
/*
 *  struct Expr *InitVarExpr(e,n)
 *  struct Expr *e;
 *  CONST struct Name *n;
 *  Given an empty expr node, init it to contain the name.
 *  Generally this is only used to init a temporary expr node
 *  that you want to be able to destroy later (or forget later)
 *  without destroying the contents -- in this case name.
 *  How you create and destroy e is your business. using stack space
 *  is suggested.
 *
 *  The problem with creating a varexpr with a name you want to keep
 *  after the node dies is that the name is destroyed when the node is.
 */

extern struct Expr *CreateOpExpr(enum Expr_enum);
/*
 *  struct Expr *CreateOpExpr(t)
 *  enum Expr_enum t;
 *  Create an operator node.
 */

extern struct Expr *CreateSatisfiedExpr(struct Name *,double,CONST dim_type *);
/*
 *  struct Expr *CreateSatisfiedExpr(n,tol,dims)
 *  struct Name *n;
 *  double tol;
 *  const dim_type *dims;
 *  Create an satisfied operator node.
 */

extern struct Expr *CreateFuncExpr(CONST struct Func *);
/*
 *  struct Expr *CreateFuncExpr(f)
 *  const struct Func *f;
 *  Create a function node.
 */

extern struct Expr *CreateIntExpr(long);
/*
 *  struct Expr *CreateIntExpr(i)
 *  long i;
 *  Create an integer node.
 */

extern struct Expr *CreateRealExpr(double,CONST dim_type *);
/*
 *  struct Expr *CreateRealExpr(r,dims)
 *  double r;
 *  const dim_type *dims;
 *  Create a real node with value r and dimensions "dims".
 */

extern struct Expr *CreateTrueExpr(void);
/*
 *  struct Expr *CreateTrueExpr();
 *  Create a boolean node with value TRUE.
 */

extern struct Expr *CreateFalseExpr(void);
/*
 *  struct Expr *CreateFalseExpr();
 *  Create a boolean node with value FALSE.
 */

extern struct Expr *CreateAnyExpr(void);
/*
 *  struct Expr *CreateAnyExpr();
 *  Create a boolean node with undefined value. b_value will be 2.
 */

extern struct Expr *CreateSetExpr(struct Set *);
/*
 *  struct Expr *CreateSetExpr(set)
 *  struct Set *set.
 *  Create a set node.
 */

extern struct Expr *CreateSymbolExpr(symchar *);
/*
 *  struct Expr *CreateSymbolExpr(sym)
 *  const char *sym;
 *  Create a symbol node.
 */

extern struct Expr *CreateQStringExpr(CONST char *);
/*
 *  struct Expr *CreateQStringExpr(sym)
 *  const char *qstring;
 *  Create a string node. The difference is that string may contain 
 *  anything and are quoted as "qstring is string", whereas symbols
 *  are of the form 'symbol' and may have content restrictions.
 */

extern struct Expr *CreateBuiltin(enum Expr_enum,struct Set *);
/*
 *  struct Expr *CreateBuiltin(t,set)
 *  enum Expr_enum t;
 *  struct Set *set;
 *  Create a node for SUM, PROD, UNION, etc....
 */

extern void LinkExprs(struct Expr *, struct Expr *);
/*
 *  void LinkExprs(cur,next)
 *  struct Expr *cur, *next;
 *  Link cur to next.
 */

unsigned long ExprListLength(CONST struct Expr *);
/*
 *  unsigned long ExprListLength(e);
 *  CONST struct Expr *e;
 *  Does as you expect. Traverse to list to the end to find the length.
 *  Sometimes one would like to know the length a priori.
 */

#ifdef NDEBUG
#define NextExpr(e) ((e)->next)
#else
#define NextExpr(e) NextExprF(e)
#endif
extern struct Expr *NextExprF(CONST struct Expr *);
/*
 *  macro NextExpr(e)
 *  struct Expr *NextExprF(e)
 *  const struct Expr *e;
 *  Return the expr node linked to e.
 */

#ifdef NDEBUG
#define ExprType(e) ((e)->t)
#else
#define ExprType(e) ExprTypeF(e)
#endif
extern enum Expr_enum ExprTypeF(CONST struct Expr *);
/*
 *  macro ExprType(e)
 *  enum Expr_enum ExprTypeF(e)
 *  const struct Expr *e;
 *  Return the type of e.
 */

#ifdef NDEBUG
#define ExprName(e) ((e)->v.nptr)
#else
#define ExprName(e) ExprNameF(e)
#endif
extern CONST struct Name *ExprNameF(CONST struct Expr *);
/*
 *  macro ExprName(e)
 *  const struct Name *ExprNameF(e)
 *  const struct Expr *e;
 *  Return the name field of a var type expr node.
 */

#ifdef NDEBUG
#define ExprFunc(e) ((e)->v.fptr)
#else
#define ExprFunc(e) ExprFuncF(e)
#endif
extern CONST struct Func *ExprFuncF(CONST struct Expr *);
/*
 *  macro ExprFunc(e)
 *  const struct Func *ExprFuncF(e)
 *  const struct Expr *e;
 *  Return the func field of a function type expr node.
 */

#ifdef NDEBUG
#define ExprIValue(e) ((e)->v.ivalue)
#else
#define ExprIValue(e) ExprIValueF(e)
#endif
extern long ExprIValueF(CONST struct Expr *);
/*
 *  macro ExprIValue(e)
 *  long ExprIValueF(e)
 *  const struct Expr *e;
 *  Return the integer value of a integer type expr node.
 */

#ifdef NDEBUG
#define ExprRValue(e) ((e)->v.r.rvalue)
#else
#define ExprRValue(e) ExprRValueF(e)
#endif
extern double ExprRValueF(CONST struct Expr *);
/*
 *  macro ExprRValue(e)
 *  double ExprRValueF(e)
 *  const struct Expr *e;
 *  Return the real value of a real type expr node.
 */

#ifdef NDEBUG
#define ExprRDimensions(e) ((e)->v.r.dimensions)
#else
#define ExprRDimensions(e) ExprRDimensionsF(e)
#endif
extern CONST dim_type *ExprRDimensionsF(CONST struct Expr *);
/*
 *  macro ExprRDimensions(e)
 *  const dim_type *ExprRDimensionsF(e)
 *  const struct Expr *e;
 *  Return the dimensions of a real type expr node.
 */

#ifdef NDEBUG
#define SatisfiedExprName(e) ((e)->v.se.sen)
#else
#define SatisfiedExprName(e) SatisfiedExprNameF(e)
#endif
extern CONST struct Name *SatisfiedExprNameF(CONST struct Expr *);
/*
 *  macro SatisfiedExprName(e)
 *  const struct Name *SatisfiedExprNameF(e)
 *  const struct Expr *e;
 *  Return the name field of a var type satisfied expr node.
 */

#ifdef NDEBUG
#define SatisfiedExprRValue(e) ((e)->v.se.ser.rvalue)
#else
#define SatisfiedExprRValue(e) SatisfiedExprRValueF(e)
#endif
extern double SatisfiedExprRValueF(CONST struct Expr *);
/*
 *  macro SatisfiedExprRValue(e)
 *  double SatisfiedExprRValueF(e)
 *  const struct Expr *e;
 *  Return the real value of a real type satisfied expr node.
 */

#ifdef NDEBUG
#define SatisfiedExprRDimensions(e) ((e)->v.se.ser.dimensions)
#else
#define SatisfiedExprRDimensions(e) SatisfiedExprRDimensionsF(e)
#endif
extern CONST dim_type *SatisfiedExprRDimensionsF(CONST struct Expr *);
/*
 *  macro SatisfiedExprRDimensions(e)
 *  const dim_type *SatisfiedExprRDimensionsF(e)
 *  const struct Expr *e;
 *  Return the dimensions of a real type satisfied expr node.
 */

#ifdef NDEBUG
#define ExprBValue(e) ((e)->v.bvalue)
#else
#define ExprBValue(e) ExprBValueF(e)
#endif
extern int ExprBValueF(CONST struct Expr *);
/*
 *  macro ExprBValue(e)
 *  int ExprBValue(e)
 *  const struct Expr *e;
 *  Return 1 if e is TRUE, 0 if e is FALSE, 2 if e is ANY.
 */

#ifdef NDEBUG
#define ExprSValue(e) ((e)->v.s)
#else
#define ExprSValue(e) ExprSValueF(e)
#endif
extern struct Set *ExprSValueF(CONST struct Expr *);
/*
 *  macro ExprSValue(e)
 *  struct Set *ExprSValueF(e)
 *  const struct Expr *e;
 *  Return the set value of a set node type.
 */

#ifdef NDEBUG
#define ExprSymValue(e) ((e)->v.sym_ptr)
#else
#define ExprSymValue(e) ExprSymValueF(e)
#endif
extern symchar *ExprSymValueF(CONST struct Expr *);
/*
 *  macro ExprSymValue(e)
 *  symchar *ExprSymValueF(e)
 *  const struct Expr *e;
 *  Return the symbol pointer value from a symbol node type.
 */

#ifdef NDEBUG
#define ExprQStrValue(e) ((e)->v.sym_ptr)
#else
#define ExprQStrValue(e) ExprQStrValueF(e)
#endif
extern CONST char *ExprQStrValueF(CONST struct Expr *);
/*
 *  macro ExprQStrValue(e)
 *  const char *ExprQStrValueF(e)
 *  const struct Expr *e;
 *  Return the string pointer value from a string node type.
 *  The difference between a string and a symbol, is that the former
 *  may contain whitespace. The type is called e_qstring
 */

#ifdef NDEBUG
#define ExprBuiltinSet(e) ((e)->v.s)
#else
#define ExprBuiltinSet(e) ExprBuiltinSetF(e)
#endif
extern CONST struct Set *ExprBuiltinSetF(CONST struct Expr *);
/*
 *  macro ExprBuiltinSet(e)
 *  const struct Set *ExprBuiltinSetF(e)
 *  const struct Expr *e;
 *  Return the set argument for one of the builtin operations.  SUM, PROD,
 *  CARD, etc..
 */

extern struct Expr *CopyExprList(CONST struct Expr *);
/*
 *  struct Expr *CopyExprList(e)
 *  struct Expr *e;
 *  Make and return a copy of e.
 */

extern void DestroyExprList(struct Expr *);
/*
 *  void DestroyExprList(e)
 *  struct Expr *e;
 *  Deallocate all the memory associated with e.
 *  Handles NULL input gracefully.
 */

extern struct Expr *JoinExprLists(struct Expr *,struct Expr *);
/*
 *  struct Expr *JoinExprLists(e1,e2)
 *  struct Expr *e1, *e2;
 *  Append list e2 to the end of e1.  This returns e1, unless e1
 *  is NULL in which case it returns e2.
 */

extern int ExprsEqual(CONST struct Expr *,CONST struct Expr *);
/*
 *  int ExprsEqual(e1,e2)
 *  const struct Expr *e1,*e2;
 *  Return TRUE if and only if e1 and e2 are structurally equivalent.
 */

extern int CompareExprs(CONST struct Expr *,CONST struct Expr *);
/*
 *  int CompareExprs(e1,e2)
 *  const struct Expr *e1,*e2;
 *  Return -1, 0, 1 as e1 is < == > e2.
 *  Expressions being complicated things, this is not easily
 *  explained. The expressions are being compared in the
 *  absence of an instance context, so we're looking for
 *  structural differences.
 *  The NULL Expr > all Expr.
 */

extern void exprs_init_pool(void);
/*
 * starts memory recycle. do not call twice before stopping recycle.
 */

extern void exprs_destroy_pool(void);
/*
 * stops memory recycle. do not call while ANY Expr are outstanding.
 */

extern void exprs_report_pool(void);
/*
 * write the pool report to ASCERR for the exprs pool.
 */

#endif /* __EXPRS_H_SEEN__ */
