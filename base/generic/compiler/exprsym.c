/*
 *  Symbolic Expression Manipulation
 *  by Kirk Abbott
 *  Created: Novermber 21, 1994
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: exprsym.c,v $
 *  Date last modified: $Date: 1997/09/08 18:07:36 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND compiler.
 *
 *  Copyright (C) 1994,1995 Kirk Andre Abbott.
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
 *  COPYING.  COPYING is found in ../compiler.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>		/* !!! dont ever forget this and -lm */
#include <utilities/ascConfig.h>
#include "compiler.h"
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "func.h"
#include "expr_types.h"
#include "instance_enum.h"
#include "freestore.h"
#include "relation_type.h"
#include "find.h"
#include "relation.h"
#include "relation_util.h"
#include "exprsym.h"

#ifndef NO_FREE_STORE
/* free store alloc */
#define TMALLOC (Term *)GetMem()
/* independent allocation */
#define UTMALLOC TERM_ALLOC
#else
/* no free store --> both are independent allocation */
#define TMALLOC TERM_ALLOC
#define UTMALLOC TERM_ALLOC
#endif
#define RMALLOC (RelationINF *)ascmalloc((unsigned)sizeof(RelationINF))

/*
 * Toggle the next 2 for some memory usage statistics.
 */
#define FREESTORE_STATS  1
#undef  FREESTORE_STATS
static int CmpP(CONST char *c1, CONST char *c2)
{
  if (c1 > c2) return 1;
  if (c1 < c2) return -1;
  return 0;
}

/*
 * Very popular constants. We have not put them in the free store
 * as it takes too much fancy footwork not to overwrite them each
 * time that we (re)init the free store.
 */
/* note that Relation real is the size dominant relation_term
   subtype, so these are ok.
*/
static struct RelationReal MinusOne;
static struct RelationReal Zero;
static struct RelationReal One;
static struct RelationReal Two;
static struct RelationReal Three;
static int constants_inited = 0;

/*********************************************************************\
  Terms, and routines to manipulate them.

  At the moment the orginal relations use malloc for getting
  memory, rather than making use of a freestore. Consequently
  we have *very* similar code for making the Terms. One day the
  the original relations will use a freestore as well in which
  case, the term creation routines may be unified. In the mean time
  all the Create routines have a "D" (for derivative) inserted in their
  names and have been made static.

  Actually, however, the free stores, or pool stores, must be separate
  to maintain memory integrity, so the create functions in
  relation.c are private. These are free to become global if that
  is desired. Probably not desired however...
\*********************************************************************/

static Term *CreateDTermBinary(Term *left, enum Expr_enum kind, Term *right)
{
  Term *result;
  result = TMALLOC;
  result->t = kind; /* set the kind of node */
  B_TERM(result)->left = left;
  B_TERM(result)->right = right;
  return result;
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static Term *CreateDTermUnary(Term *left, enum Expr_enum kind)
{
  Term *result;
  result = TMALLOC;
  result->t = kind; /* set the kind of node */
  U_TERM(result)->left = left;
  return result;
}
#endif   /* THIS_IS_AN_UNUSED_FUNCTION */


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static Term *CreateDTermVar(void)
{
  Term *result;
  result = TMALLOC;
  result->t = e_var;
  V_TERM(result)->varnum = 0;	/* not given a number yet */
  return result;
}
#endif   /* THIS_IS_AN_UNUSED_FUNCTION */


static Term *CreateDTermInteger(long int i)
{
  Term *result;
  result = TMALLOC;
  result->t = e_int;
  I_TERM(result)->ivalue = i;
  return result;
}

static Term *CreateDTermReal(double d, dim_type *dim)
{
  Term *result;
  result = TMALLOC;
  result->t = e_real;
  R_TERM(result)->value = d;
  R_TERM(result)->dimensions = dim;
  return result;
}

static Term *CreateDTermFunc(Term *left,CONST Func *fptr)
{
  Term *result;
  result = TMALLOC;
  result->t = e_func;
  F_TERM(result)->left = left;
  F_TERM(result)->fptr = (Func *)fptr;
  return result;
}


/*
 * Clone and Copy degenerate into the same
 * function whenever, the freestore is not in
 * use. Copy must be used whenever a caller
 * requires his own *separate* copy of a relation
 * and its associated terms.
 */
/* pull a copy from the free store */
static Term *CloneDTerm(Term *term)
{
  Term *result;
  result = TMALLOC;
  memcpy((VOIDPTR)result,(VOIDPTR)term,sizeof(union RelationTermUnion));
  return result;
}

/* create an independent copy */
static Term *CopyDTerm(Term *term)
{
  Term *result;

  assert(term!=NULL);
  result = UTMALLOC; /* independent copy */
  memcpy((VOIDPTR)result,(VOIDPTR)term,sizeof(union RelationTermUnion));
  return result;
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static void DestroyDTerm(Term *term)
{
  FreeMem(UNION_TERM(term));
}
#endif   /* THIS_IS_AN_UNUSED_FUNCTION */

/* init static constant real terms we use a lot. must remember to cast
 * them appropriately elsewhere
 */
static void InitConstantTerms(void)
{
  if (!constants_inited) {
    MinusOne.t = e_real;
    MinusOne.value = -1.0;
    MinusOne.dimensions = NULL; /* should probably make wild */

    Zero.t = e_real;
    Zero.value = 0.0;
    Zero.dimensions = NULL;

    One.t = e_real;
    One.value = 1.0;
    One.dimensions = NULL;

    Two.t = e_real;
    Two.value = 2.0;
    Two.dimensions = NULL;

    Three.t = e_real;
    Three.value = 3.0;
    Three.dimensions = NULL;
    constants_inited = 1;
  }
}

/* what the heck is this here for? */
static
RelationINF *CreateRelInfix(Term *lhs, enum Expr_enum op, Term *rhs)
{
  RelationINF *result;
  result = (RelationINF *)CreateRelationStructure(op,crs_NEWUNION);
  result->share->s.relop = op;
  RelationRefCount(result) = 1;
  RTOKEN(result).lhs = NULL;	/* this is for postfix scanning */
  RTOKEN(result).rhs = NULL;
  RTOKEN(result).lhs_term = lhs;
  RTOKEN(result).rhs_term = rhs;
  result->vars = NULL;
  result->d = NULL;
  result->residual = 0.0;
  result->multiplier = 0.0;
  return result;
}

/*
 * This function will run through a tree of terms
 * and fix the varlist associated with the tree.
 * It *assumes* that the current tree has a lower
 * # of inident variables that the old list.
 */
static struct gl_list_t *g_new_var_list = NULL;

static
void CollectVars(Term *term, RelationINF *rel)
{
  unsigned long oldvarnum, pos;
  CONST struct Instance *inst;

  assert(term!=NULL);
  switch(RelationTermType(term)) {
  case e_var:
    oldvarnum = TermVarNumber(term);
    inst = RelationVariable(rel,oldvarnum);
    pos = gl_search(g_new_var_list,inst,(CmpFunc)CmpP);
    if (pos)
      V_TERM(term)->varnum = pos;
    else{
      gl_append_ptr(g_new_var_list,(VOIDPTR)inst);
      V_TERM(term)->varnum = gl_length(g_new_var_list);
    }
    break;
  default:
    break;
  }
}

static
struct gl_list_t *RedoVarList(RelationINF *rel,
			      struct gl_list_t *oldvars)
{
  unsigned long oldlen;
  struct gl_list_t *tmp;
  oldlen = (oldvars!=NULL) ? gl_length(oldvars) : 0;
  if (oldlen) {
    g_new_var_list = gl_create(oldlen); /* be conservative ! */
    DoInOrderVisit(Infix_LhsSide(rel),rel,CollectVars);
    DoInOrderVisit(Infix_RhsSide(rel),rel,CollectVars);
    tmp = g_new_var_list;
    g_new_var_list = NULL;
    return tmp;
  }
  return NULL;
}

/*
 * Return a separate verbatim copy of a term tree.
 * i.e, no simplification, etc..
 */

static
Term *CopyDTermTree(Term *term)
{
  Term *left,*right,*result;
  if (!term) return NULL;
  switch(term->t) {
  case e_var: case e_int: case e_real: case e_zero:
    return CopyDTerm(term);
  case e_plus: case e_minus: case e_times:
  case e_divide: case e_power: case e_ipower:
    left = CopyDTermTree(TermBinLeft(term));
    right = CopyDTermTree(TermBinRight(term));
    result = CopyDTerm(term);
    B_TERM(result)->left = left; B_TERM(result)->right = right;
    return result;
  case e_uminus:
    left = CopyDTermTree(TermUniLeft(term));
    result = CopyDTerm(term);
    U_TERM(result)->left = left;
    return result;
  case e_func:
    left = CopyDTermTree(TermFuncLeft(term));
    result = CopyDTerm(term);
    F_TERM(result)->left = left;
    return result;
  default:
    FPRINTF(ASCERR,"Unknown term type in expression\n");
    return NULL;
  }
}


/*********************************************************************\
  Support routines for symbolic manipulation
\*********************************************************************/

/*
 * The basic principle here is that all the Make* functions
 * must *never* modify what has been given to them. They
 * must create memory and return it, if necessary.
 */
static Term *MakeAdd(Term *left, Term *right)
{
  Term *result = NULL;

  if (left == K_TERM(&Zero)) return right;	/* 0 + u = u */
  if (right == K_TERM(&Zero)) return left;	/* u + 0 = u */

  if (left->t == e_real && right->t == e_real)
    return CreateDTermReal(R_TERM(left)->value + R_TERM(right)->value, NULL);

  if (left->t == e_real && right->t == e_int)
    return CreateDTermReal(R_TERM(left)->value
				 + (double)I_TERM(right)->ivalue, NULL);

  if (left->t == e_int && right->t == e_real)
    return CreateDTermReal((double)I_TERM(left)->ivalue
				+ R_TERM(right)->value, NULL);

  if (left->t == e_int && right->t == e_int)
    return CreateDTermInteger(I_TERM(left)->ivalue + I_TERM(right)->ivalue);

  result = TMALLOC;
  if (right->t == e_uminus) { /* a + (-b) --> a - b */
    result->t = e_minus;
    B_TERM(result)->left = left;
    B_TERM(result)->right = TermUniLeft(right);
    return result;
  }
  if (left->t == e_uminus) { /* (-a) + b --> b - a */
    result->t = e_minus;
    B_TERM(result)->left = right;
    B_TERM(result)->right = TermUniLeft(left);
  }

  result->t = e_plus;
  B_TERM(result)->left = left;
  B_TERM(result)->right = right;
  return result;
}

static Term *MakeSubtract(Term *left, Term *right)
{
  Term *result=NULL;

  if (right==K_TERM(&Zero)) return left;	/* u - 0 = u */

  if (left->t == e_real && right->t == e_real)
    return CreateDTermReal(R_TERM(left)->value - R_TERM(right)->value, NULL);

  if (left->t == e_real && right->t == e_int)
    return CreateDTermReal(R_TERM(left)->value
				- (double)I_TERM(right)->ivalue, NULL);

  if (left->t == e_int && right->t == e_real)
    return CreateDTermReal((double)I_TERM(left)->ivalue
				- R_TERM(right)->value, NULL);

  if (left->t == e_int && right->t == e_int)
    return CreateDTermInteger(I_TERM(left)->ivalue - I_TERM(right)->ivalue);


  result = TMALLOC;
  if (right->t == e_uminus) {
    result->t = e_plus; /* a - (-b) --> a + b */
    B_TERM(result)->left = left;
    B_TERM(result)->right = TermUniLeft(right);
    return result;
  }

  result->t = e_minus;
  B_TERM(result)->left = left;
  B_TERM(result)->right = right;
  return result;
}

static Term *MakeMultiply(Term *left, Term *right)
{
  Term *result;

  if (left==K_TERM(&Zero) || right == K_TERM(&Zero))
    return K_TERM(&Zero);
  if (left==K_TERM(&One) && right==K_TERM(&One)) return K_TERM(&One);
  if (left==K_TERM(&One) && right!=K_TERM(&One)) return right;
  if (left!=K_TERM(&One) && right==K_TERM(&One))  return left;

  /* the following assumes no &One in the expression */

  if (left->t == e_real && right->t == e_real)
    return CreateDTermReal(R_TERM(left)->value * R_TERM(right)->value, NULL);

  if (left->t == e_real && right->t == e_int)
    return CreateDTermReal(R_TERM(left)->value * (double)I_TERM(right)->ivalue,
                           NULL);

  if (left->t == e_int && right->t == e_real)
    return CreateDTermReal((double)I_TERM(left)->ivalue * R_TERM(right)->value,
                           NULL);

  if (left->t == e_int && right->t == e_int)
    return CreateDTermInteger(I_TERM(left)->ivalue * I_TERM(right)->ivalue);

  /* not a simple constant */

  result = TMALLOC;
  result->t = e_times;
  B_TERM(result)->left = left;
  B_TERM(result)->right = right;
  return result;
}


static Term *MakeDivide(Term *left, Term *right)
{
  Term *result;

  if (left==K_TERM(&Zero)) return K_TERM(&Zero);	/* 0 / u = 0 */
  if (right==K_TERM(&Zero)) {
    FPRINTF(ASCERR,"Potential for floating point dvision in MakeDivide\n");
    /* continue processing. */
  }
  if (left==K_TERM(&One) && right==K_TERM(&One))
    return K_TERM(&One);
  if (left!=K_TERM(&One) && right==K_TERM(&One))
    return left;

  if (left->t == e_real && right->t == e_real)
    return CreateDTermReal(R_TERM(left)->value / R_TERM(right)->value, NULL);

  if (left->t == e_real && right->t == e_int)
    return CreateDTermReal(R_TERM(left)->value / (double)I_TERM(right)->ivalue,
                           NULL);

  if (left->t == e_int && right->t == e_real)
    return CreateDTermReal((double)I_TERM(left)->ivalue / R_TERM(right)->value,
                           NULL);

  result = TMALLOC;
  result->t = e_divide;
  B_TERM(result)->left = left;
  B_TERM(result)->right = right;
  return result;
}

static Term *MakePower(Term *left, Term *right)
{
  Term *result;
  enum Expr_enum ltype, rtype;
  double lvalue,rvalue;
  ltype = left->t; rtype = right->t;

  if (left==K_TERM(&One))	/* 1 ^ ? = 1 */
    return K_TERM(&One);
  if (right==K_TERM(&One))	/* ? ^ 1 = ? */
    return left;
  if (rtype == e_int && I_TERM(right)->ivalue == 0)	/* ? ^ 0 = 1 */
    return K_TERM(&One);

  if (ltype == e_real && rtype== e_real)
    return CreateDTermReal(pow(R_TERM(left)->value,R_TERM(right)->value),NULL);

  if (ltype == e_real && rtype == e_int) {
    rvalue = (double)I_TERM(right)->ivalue;
    return CreateDTermReal(pow(R_TERM(left)->value,rvalue), NULL);
  }
  if (ltype == e_int && rtype == e_real) {
    lvalue = (double)R_TERM(left)->value;
    return CreateDTermReal(pow(lvalue,R_TERM(right)->value), NULL);
  }
  if (ltype == e_int && rtype == e_int) {
    lvalue = (double)I_TERM(left)->ivalue;
    rvalue = (double)I_TERM(right)->ivalue;
    return CreateDTermReal(pow(lvalue,rvalue),NULL);
  }
  result = TMALLOC;
  result->t = e_power;
  B_TERM(result)->left = left;
  B_TERM(result)->right = right;
  return result;
}

static Term *MakeUMinus(Term *left)
{
  Term *result;
  result = TMALLOC;
  result->t = e_uminus;
  U_TERM(result)->left = left;
  return result;
}

static Term *MakeNegation(Term *term)
{
  Term *result;
  if (term==K_TERM(&Zero))
    return term;
  if (term==K_TERM(&One))
    return K_TERM(&MinusOne);
  if(term==K_TERM(&MinusOne))
    return K_TERM(&One);
  /*
   * We cannot just compute the (-1.0 * value) as we might
   * be passed in one of the special constants, and we *must*
   * not corrupt those constants.
   * We dont want to search over all possible special
   * constants, so it is easier to create a new node.
   */
  switch(term->t) {
  case e_real:
    result = TMALLOC;
    result->t = e_real;
    R_TERM(result)->value = -1.0*R_TERM(term)->value;
    return result;
  case e_int:
    result = TMALLOC;
    result->t = e_int;
    I_TERM(result)->ivalue = -1*I_TERM(term)->ivalue;
    return result;
  case e_uminus:
    result = TermUniLeft(term);
    return result;
  default:
    result = TMALLOC;
    result->t = e_uminus;
    U_TERM(result)->left = term;
    return result;
  }
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static void DoBreakPoint(void)
{
  return;
}
#endif   /* THIS_IS_AN_UNUSED_FUNCTION */

static Term *MakeFunc(enum Func_enum id, Term *term)
{
  Term *result;
  CONST struct Func *fptr;
  double value;

  fptr = LookupFuncById(id);
  assert(fptr!=NULL);
  if (term->t == e_real) {/* evaluate the term */
    value = FuncEval(fptr,R_TERM(term)->value);
    result = CreateDTermReal(value,NULL);
    return result;
  }

  if (term->t == e_int) {/* evaluate the term */
    value = FuncEval(fptr,(double)I_TERM(term)->ivalue);
    result = CreateDTermReal(value,NULL);
    return result;
  }

  if (term->t == e_uminus) {
    switch(id) {
    case F_SIN: case F_SINH:  /* these are the odd functions */
    case F_TAN: case F_TANH:  /* sin (-x) = - sin (x) */
    case F_ARCSIN: case F_ARCSINH:
    case F_ARCTAN: case F_ARCTANH:
#ifdef HAVE_ERF
    case F_ERF:
#endif /* HAVE_ERF */
    case F_CUBE:
    case F_CBRT:
      result = CreateDTermFunc(TermUniLeft(term),fptr);
      result = MakeUMinus(result);
      return result;
    case F_COS: case F_COSH:   /* these are the even functions */
    case F_ARCCOS: case F_SQR: /* cos (-x) = cos(x) */
      result = CreateDTermFunc(TermUniLeft(term),fptr);
      return result;
    default:
      break;
    }
  }
  result = CreateDTermFunc(term,fptr);
  return result;
}

Term *TermSimplify(Term *term)
{
  Term *left,*right;
  if (term) {
    switch(term->t) {
    case e_var:
    case e_int:
    case e_real:
      return term;
    case e_plus:
      left = TermSimplify(TermBinRight(term));
      right = TermSimplify(TermBinRight(term));
      return MakeAdd(left,right);
    case e_minus:
      left = TermSimplify(TermBinRight(term));
      right = TermSimplify(TermBinRight(term));
      return MakeSubtract(left,right);
    case e_times:
      left = TermSimplify(TermBinRight(term));
      right = TermSimplify(TermBinRight(term));
      return MakeMultiply(left,right);
    case e_divide:
      left = TermSimplify(TermBinRight(term));
      right = TermSimplify(TermBinRight(term));
      return MakeDivide(left,right);
    case e_power:
    case e_ipower:
      left = TermSimplify(TermBinRight(term));
      right = TermSimplify(TermBinRight(term));
      return MakePower(left,right);
    case e_uminus:
      left = TermSimplify(TermUniLeft(term));
      return MakeUMinus(left);
    case e_func:
      left = TermSimplify(TermFuncLeft(term));
      return MakeFunc(FuncId(TermFunc(term)),left);
    default:
      FPRINTF(ASCERR,"Unknown term type in expression\n");
      return NULL;
    }
  }
  return NULL;
}



/*********************************************************************\
  Symbolic derivatives.
\*********************************************************************/

/*
 * Any potentially destructive operations should be preceeded
 * by a copying of the terms. This is necessary so that we don't
 * trash the original expression tree that is handed to us.
 * Still not sure whether I should have the filter operate on Terms
 * or on instances. If it operated on terms then there would be the
 * oppurtunity to handle reductions of e_int to e_real etc. For the
 * moment it does nothing !!
 */

Term *Derivative(Term *term, unsigned long wrt,
		 int (*filter)(struct Instance *))
{
  Term *du_dx, *dv_dx;

  switch(term->t) {
  case e_var:	/* du/dx = du_dx */
    if (wrt==TermVarNumber(term))
      return K_TERM(&One);
    return K_TERM(&Zero);

  case e_int:
  case e_real:
    return K_TERM(&Zero);

  case e_plus: 	/* u + v */
    du_dx = Derivative(TermBinRight(term),wrt,filter);
    dv_dx = Derivative(TermBinRight(term),wrt,filter);

    if (du_dx==K_TERM(&Zero) && dv_dx!=K_TERM(&Zero))
      return dv_dx;
    if (dv_dx==K_TERM(&Zero) && du_dx!=K_TERM(&Zero))
      return du_dx;
    if (du_dx==K_TERM(&Zero) && dv_dx==K_TERM(&Zero))
      return K_TERM(&Zero);
    return MakeAdd(du_dx,dv_dx);

  case e_minus: /* u - v */
    du_dx = Derivative(TermBinRight(term),wrt,filter);
    dv_dx = Derivative(TermBinRight(term),wrt,filter);

    if (du_dx==K_TERM(&Zero) && dv_dx!=K_TERM(&Zero))
      return MakeNegation(dv_dx);
    if (dv_dx==K_TERM(&Zero) && du_dx!=K_TERM(&Zero))
      return du_dx;
    if (du_dx==K_TERM(&Zero) && dv_dx==K_TERM(&Zero))
      return K_TERM(&Zero);

    return MakeSubtract(du_dx,dv_dx);

  case e_times: {  /* d(u * v)/dx  --> (v * du_dx)  +  (u * dv_dx) */
    Term *left, *right;

    left = CloneDTerm(TermBinRight(term)); /* copy first !! */
    right = CloneDTerm(TermBinRight(term));
    du_dx = Derivative(left,wrt,filter);
    dv_dx = Derivative(right,wrt,filter);

    if (du_dx==K_TERM(&Zero) && dv_dx==K_TERM(&Zero)) /* --> 0 */
      return K_TERM(&Zero);

    if (du_dx==K_TERM(&Zero) && dv_dx!=K_TERM(&Zero)) /* --> u * dv_dx */
      return MakeMultiply(left,dv_dx);

    if (dv_dx==K_TERM(&Zero) && du_dx!=K_TERM(&Zero)) /* --> v * du_dx */
      return MakeMultiply(right,du_dx);

    return MakeAdd(MakeMultiply(left,dv_dx),
		   MakeMultiply(right,du_dx));
  }
  case e_divide: {/* d(u/v)/dx = (v*du_dx  - u*dv_dx)/ v^2 */
                  /*           = (du_dx/v  -  u/v^2*dv_dx) */
    Term *left, *right;

    left = CloneDTerm(TermBinRight(term));
    right = CloneDTerm(TermBinRight(term));
    du_dx = Derivative(left,wrt,filter);
    dv_dx = Derivative(right,wrt,filter);

    if (du_dx==K_TERM(&Zero) && dv_dx==K_TERM(&Zero)) /* --> 0 */
      return K_TERM(&Zero);

    if (du_dx==K_TERM(&Zero) && dv_dx!=K_TERM(&Zero)) {
      /* --> -  (u*dv_dx)/(v^2) */
      return MakeNegation(MakeDivide(MakeMultiply(left,dv_dx),
				     MakeFunc(F_SQR,right)));
    }

    if (dv_dx==K_TERM(&Zero) && du_dx!=K_TERM(&Zero)) /* -->  du_dx / v */
      return MakeDivide(du_dx,right);

    return MakeSubtract(MakeDivide(du_dx,right),
			MakeDivide(MakeMultiply(left,dv_dx),
				   MakeFunc(F_SQR,right)));
  }

  case e_power:
  case e_ipower: {/* d ( u ^ v)/dx = u^v * (du_dx * v / u  + dv_dx*ln(u)) */
    Term *left, *right;
    Term *e0, *e1;
    long ivalue;
    int sival;
    double rvalue;

    left = CloneDTerm(TermBinRight(term));
    right = CloneDTerm(TermBinRight(term));

    /* 0 ^0 is undefined, so return Zero */
    if (left==K_TERM(&Zero) && right==K_TERM(&Zero))	/* d (0^0)/dx = 0 */
      return K_TERM(&Zero);
    if (right==K_TERM(&Zero)) 		/* d (u^0)/dx = d(1)/dx = 0 */
      return K_TERM(&Zero);
    if (right==K_TERM(&One)) 		/* d (u^1)/dx = du_dx */
      return Derivative(left,wrt,filter);
    if (left==K_TERM(&One))		/* d(1^u)/dx = d(1)/dx = 0 */
      return K_TERM(&Zero);

    if (right->t==e_int) {
      ivalue = I_TERM(right)->ivalue;
      sival = (int)ivalue;
      switch(sival) {
      case 0:		/* d (u^0)/dx  : as above but comparison by value */
	return K_TERM(&Zero);
      case 1:		/*  d (u^1)/dx : as above but comparison by value */
	return Derivative(left,wrt,filter);
      case 2:		/* d/dx (u ^ 2) = 2 * u * du_dx */
	du_dx = Derivative(left,wrt,filter);
	e0 = MakeMultiply(K_TERM(&Two),MakeMultiply(left,du_dx));
	return e0;
      default:		/* d/dx (u ^ n) = n * (u ^ (n-1))* du_dx */
	du_dx = Derivative(left,wrt,filter);
	e1 = MakePower(left,CreateDTermInteger(ivalue-1));
	e1 = MakeMultiply(CreateDTermInteger(ivalue),MakeMultiply(e1,du_dx));
	return e1;
      }
    }
    /* d/dx (u ^ n) = n * (u ^ (n-1))* du_dx */
    if (right->t==e_real) {
      rvalue = R_TERM(right)->value;
      du_dx = Derivative(left,wrt,filter);
      e1 = MakePower(left,CreateDTermReal(rvalue-1.0,NULL));
      e1 = MakeMultiply(CreateDTermReal(rvalue,NULL),MakeMultiply(e1,du_dx));
      return e1;
    }

    du_dx = Derivative(left,wrt,filter);
    dv_dx = Derivative(right,wrt,filter);
    e0 = MakeFunc(F_LN,TermBinRight(term));
    e0 = MakeMultiply(dv_dx,e0);
    e1 = MakeDivide(TermBinRight(term),TermBinRight(term));
    e1 = MakeMultiply(du_dx,e1);
    e0 = MakeAdd(e0,e1);
    e0 = MakeMultiply(e0,MakePower(TermBinRight(term),TermBinRight(term)));
    return e0;
  }

  case e_func: { /* deal with all the recognized functions */
    enum Func_enum id;
    Term *left;
    Term *e;

    left = CloneDTerm(TermFuncLeft(term));
    du_dx = Derivative(left,wrt,filter);
    switch(id=FuncId(TermFunc(term))){ /* d (sin(u))/dx = cos(u).du_dx */
    case F_SIN:
      e = MakeFunc(F_COS,left);
      e = MakeMultiply(du_dx,e);
      return e;
    case F_SINH:
      e = MakeFunc(F_SINH,left);
      e = MakeMultiply(du_dx,e);
      return e;
    case F_COS:
      e = MakeFunc(F_SIN,left);
      e = MakeMultiply(du_dx,e);
      e = MakeNegation(e);
      return e;
    case F_COSH:
      e = MakeFunc(F_SINH,left);
      e = MakeMultiply(du_dx,e);
      return e;
    case F_TAN:
      e = MakeFunc(F_COS,left);
      e = MakeFunc(F_SQR,left);
      e = MakeDivide(du_dx,e);
      return e;
    case F_TANH:
      e = MakeFunc(F_COSH,left);
      e = MakeFunc(F_SQR,e);
      e = MakeDivide(du_dx,e);
      return e;
    case F_ARCSIN:
      e = MakeFunc(F_SQR,left);
      e = MakeSubtract(K_TERM(&One),e);
      e = MakeFunc(F_SQRT,e);
      e = MakeDivide(du_dx,e);
      return e;
    case F_ARCSINH:
      e = MakeFunc(F_SQR,left);
      e = MakeAdd(e,K_TERM(&One));
      e = MakeFunc(F_SQRT,e);
      e = MakeDivide(du_dx,e);
      return e;
    case F_ARCCOS:
      e = MakeFunc(F_SQR,left);
      e = MakeSubtract(K_TERM(&One),e);
      e = MakeFunc(F_SQRT,e);
      e = MakeDivide(du_dx,e);
      e = MakeNegation(e);
      return e;
    case F_ARCCOSH:
      e = MakeFunc(F_SQR,left);
      e = MakeSubtract(e,K_TERM(&One));
      e = MakeFunc(F_SQRT,e);
      e = MakeDivide(du_dx,e);
      return e;
    case F_ARCTAN:
      e = MakeFunc(F_SQR,left);
      e = MakeAdd(K_TERM(&One),e);
      e = MakeDivide(du_dx,e);
      return e;
    case F_ARCTANH:
      e = MakeFunc(F_SQR,left);
      e = MakeSubtract(K_TERM(&One),e);
      e = MakeDivide(du_dx,e);
      return e;
#ifdef HAVE_ERF
    case F_ERF:
      e = MakeFunc(F_SQR,left);
      e = MakeNegation(e);
      e = MakeFunc(F_EXP,e);
      e = MakeMultiply(e,CreateDTermReal(F_ERF_COEF,NULL));
      e = MakeMultiply(du_dx,e);
      return e;
#endif /* HAVE_ERF */
    case F_EXP:
      e = MakeMultiply(du_dx,term);
      return e;
    case F_LN:
      e = MakeDivide(du_dx,left);
      return e;
    case F_LOG10:
      e = MakeDivide(du_dx,left);
      e = MakeMultiply(e,CreateDTermReal(F_LOG10_COEF,NULL));
      return e;
    case F_SQR:
      e = MakeMultiply(K_TERM(&Two),left);
      e = MakeMultiply(du_dx,e);	/* for prettyness should swap */
      return e;
    case F_SQRT:		/* d(u ^ (1/2))/dx = du_dx / ( 2 * u^(1/2)) */
      e = MakeMultiply(K_TERM(&Two),term);
      e = MakeDivide(du_dx,e);		/* note that term is used */
      return e;
    case F_CUBE:		/* d(u^3)/dx = 3 . u^2. du_dx */
      e = MakeFunc(F_SQR,left);
      e = MakeMultiply(K_TERM(&Three),e);
      return e;
      /*
       * more efficient that using sqr(u)^1/3, as other
       * simplifications become evident later.
       */
    case F_CBRT:      	/* d(u ^ (1/3))/dx = du_dx / (3 * u^(2/3)) */
      e = CreateDTermReal(2.0/3.0,NULL);
      e = MakePower(left,e);
      e = MakeMultiply(K_TERM(&Three),e);
      e = MakeDivide(du_dx,e);
      return e;
    default:
      FPRINTF(ASCERR,"Unknown function type in case e_func\n");
      return NULL;
    }
  } /* end of e_func case */

  case e_uminus: { /* d (-u)/dx = - du_dx */
    Term *left;
    left = CloneDTerm(TermUniLeft(term));
    du_dx = Derivative(left,wrt,filter);

    if (du_dx==K_TERM(&Zero))
      return K_TERM(&Zero);
    return MakeNegation(du_dx);
  }

  default:
    FPRINTF(ASCERR,"Unknown term type in Derivatives\n");
    return NULL;	/* only time that NULL should appear */
  }
}

/*
 * This function assumes that the derivative free store exists.
 * It will only ReInit the store before starting. For the time
 * In the final implementation this will be a pointer to an Instance,
 * or a index into a table of instances. We allow the reset flag
 * as we might choose not to reset after performing the derivatives on
 * say the lhs of a relation, but wait until the rhs has been
 * completed as well.
 */

Term *TermDerivative(Term *term, unsigned long wrt,
		     int (*filter)(struct Instance *))
{
  Term *result = NULL;
  assert(term!=NULL);
  result = Derivative(CloneDTerm(term),wrt,filter);
  return result;
}

RelationINF *RelDerivative(RelationINF *rel,unsigned long wrt,
			   int (*filter)(struct Instance *))
{
  Term *side;	/* taken from the freestore */
  RelationINF *result;
  unsigned long len;

  result = CreateRelInfix(NULL,rel->share->s.relop,NULL);

  len = NumberVariables(rel);
  if (!len) {			/* no variables present */
    RTOKEN(result).lhs_term = CopyDTerm(K_TERM(&Zero));
    RTOKEN(result).rhs_term = CopyDTerm(K_TERM(&Zero));
    return result;
  }
  /*
   * 1) Initialize.
   * 2) From a = b , create a - b = 0; node allocated from freestore.
   * 3) Then differentiate this single function.
   * 4) Make a fresh copy for the user.
   * 5) Fix the varlist, and return the relation -- The user owns all.
   */
  FreeStore_ReInit(FreeStore_GetFreeStore());
  side = CreateDTermBinary(Infix_LhsSide(rel),
			   e_minus,
			   Infix_RhsSide(rel));
  RTOKEN(result).lhs_term = TermDerivative(side,wrt,filter);
  RTOKEN(result).lhs_term = CopyDTermTree(Infix_LhsSide(rel));
  RTOKEN(result).rhs_term = CopyDTerm(K_TERM(&Zero));
  result->vars = RedoVarList(result,rel->vars);

#ifdef FREESTORE_STATS
  FreeStore__Statistics(ASCERR,FreeStore_GetFreeStore());
#endif
  return result;
}

RelationINF *RelDeriveSloppy(RelationINF *rel,unsigned long wrt,
			     int (*filter)(struct Instance *))
{
  Term *side;
  RelationINF *result;
  unsigned long len;

  result = CreateRelInfix(NULL,rel->share->s.relop,NULL);
  len = NumberVariables(rel);
  if (!len) {			/* no variables present */
    RTOKEN(result).lhs_term = K_TERM(&Zero);
    RTOKEN(result).rhs_term = K_TERM(&Zero);
    return result;
  }
  /*
   * Initialize freestore
   * From a = b , create a - b = 0; node allocated from freestore
   * Then differentiate this single function.
   */
  FreeStore_ReInit(FreeStore_GetFreeStore());
  side = CreateDTermBinary(Infix_LhsSide(rel),
			   e_minus,
			   Infix_RhsSide(rel));
  RTOKEN(result).lhs_term = TermDerivative(side,wrt,filter);
  RTOKEN(result).rhs_term = K_TERM(&Zero);
  result->vars = gl_copy(rel->vars);	/* not unique */

#ifdef FREESTORE_STATS
  FreeStore__Statistics(ASCERR,FreeStore_GetFreeStore());
#endif
  return result;
}

void RelDestroySloppy(RelationINF *rel)
{
  Term *side;
  if (rel->vars) gl_destroy(rel->vars);
  side = A_TERM(FreeStoreCheckMem(UNION_TERM(Infix_LhsSide(rel))));
  side = A_TERM(FreeStoreCheckMem(UNION_TERM(Infix_RhsSide(rel))));
}

void PrepareDerivatives(int setup,int n_buffers,int buffer_length)
{
  static struct FreeStore *deriv_store = NULL;
  static struct FreeStore *previous_store = NULL;

  if (setup) {
    previous_store = FreeStore_GetFreeStore();
    deriv_store = FreeStore_Create(n_buffers,buffer_length);
    FreeStore_SetFreeStore(deriv_store);
    InitConstantTerms();
  }
  else{	/* we are to shut down */
    if (deriv_store)
      FreeStore__BlastMem(deriv_store);
    FreeStore_SetFreeStore(previous_store);
    deriv_store = NULL;
    previous_store = NULL;
  }
}



