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
	Last in CVS: $Revision: 1.13 $ $Date: 1998/02/05 16:35:58 $ $Author: ballan $
*/

#include <stdio.h>
#include <assert.h>

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include "compiler.h"
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/pool.h>
#include "symtab.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "func.h"
#include "name.h"
#include "sets.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "exprs.h"


#ifndef lint
static CONST char ExpressionID[] = "$Id: exprs.c,v 1.13 1998/02/05 16:35:58 ballan Exp $";
#endif

/*------------------------------------------------------------------------------
  MEMORY USAGE

  Using 'pool' or else regular malloc...
*/

#ifdef ASC_NO_POOL
#define EXPRSUSESPOOL FALSE
#else
#define EXPRSUSESPOOL TRUE
#endif

#if EXPRSUSESPOOL /* using 'g_exprs_pool' for memory management */
/** global for our memory manager */
static pool_store_t g_exprs_pool = NULL;
	/* aim for 4096 chunks including malloc overhead */
# define EMP_LEN 10
# if (SIZEOF_VOID_P == 8)
#  define EMP_WID 63
# else
#  define EMP_WID 127
# endif
	/* retune rpwid if the size of struct name changes */
# define EMP_ELT_SIZE (sizeof(struct Expr))
# define EMP_MORE_ELTS 10
/**< Number of slots filled if more elements needed.
	So if the pool grows, it grows by EMP_MORE_ELTS*EMP_WID elements at a time. */
# define EMP_MORE_BARS 500
/**< This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/**
	This function is called at compiler startup time and destroy at shutdown.
*/
void exprs_init_pool(void) {
  if (g_exprs_pool != NULL ) {
    ASC_PANIC("ERROR: exprs_init_pool called twice.\n");
  }
  g_exprs_pool = pool_create_store(EMP_LEN, EMP_WID, EMP_ELT_SIZE,
    EMP_MORE_ELTS, EMP_MORE_BARS);
  if (g_exprs_pool == NULL) {
    ASC_PANIC("ERROR: exprs_init_pool unable to allocate pool.\n");
  }
}

void exprs_destroy_pool(void) {
  if (g_exprs_pool==NULL) return;
  pool_clear_store(g_exprs_pool);
  pool_destroy_store(g_exprs_pool);
  g_exprs_pool = NULL;
}

void exprs_report_pool()
{
  if (g_exprs_pool==NULL) {
    FPRINTF(ASCERR,"ExprsPool is empty\n");
  }
  FPRINTF(ASCERR,"ExprsPool ");
  pool_print_store(ASCERR,g_exprs_pool,0);
}

# define EPMALLOC \
	((struct Expr *)(pool_get_element(g_exprs_pool)))
/**< get a token. Token is the size of the struct struct Expr */

# define EPFREE(p) \
	(pool_free_element(g_exprs_pool,((void *)p)))
/**< return a struct Expr */

# define EXPR_CHECK_MEMORY(VAR) \
	AssertMemory(VAR)

#else /* not using 'g_exprs_pool'... */

void exprs_init_pool(void) {}
void exprs_destroy_pool(void) {}
void exprs_report_pool(void){
	FPRINTF(ASCERR,"ExprsPool not used at all\n");
}

# define EPFREE(p) \
	ASC_FREE(p)

# define EPMALLOC \
	ASC_NEW(struct Expr)

# define EXPR_CHECK_MEMORY(VAR) \
	AssertAllocatedMemory(result,sizeof(struct Expr))

#endif


# define EXPR_NEW(VAR,TYPE) \
	VAR = EPMALLOC; \
	VAR->t = TYPE; \
	VAR->next = NULL

/*------------------------------------------------------------------------------
  CREATION ROUTINES
*/

struct Expr *CreateVarExpr(struct Name *n)
{
  register struct Expr *result;
  assert(n!=NULL);
  AssertMemory(n);
  EXPR_NEW(result,e_var);
  result->v.nptr = n;
  EXPR_CHECK_MEMORY(result);

  return result;
}

void InitVarExpr(struct Expr *result,CONST struct Name *n)
{
  assert(n!=NULL);
  AssertMemory(n);
  result->t = e_var;
  result->v.nptr = (struct Name *)n;
  result->next = NULL;
}

struct Expr *CreateOpExpr(enum Expr_enum t)
{
  register struct Expr *result;
  assert((t!=e_var)&&(t!=e_func)&&(t!=e_int)&&(t!=e_real)&&(t!=e_zero));
  EXPR_NEW(result,t);
#if EXPRSUSESPOOL
  AssertMemory(result);
#else
  AssertAllocatedMemory(result,sizeof(enum Expr_enum)+sizeof(struct Expr *));
#endif
  return result;
}

struct Expr *CreateSatisfiedExpr(struct Name *n, double tol,
                                 CONST dim_type *dims)
{
  register struct Expr *result;
  EXPR_NEW(result,e_satisfied);
  result->v.se.sen =  n;
  result->v.se.ser.rvalue = tol;
  result->v.se.ser.dimensions = dims;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateFuncExpr(CONST struct Func *f)
{
  register struct Expr *result;
  EXPR_NEW(result,e_func);
  result->v.fptr = f;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateIntExpr(long int i)
{
  register struct Expr *result;
  EXPR_NEW(result,e_int);
  result->v.ivalue = i;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateRealExpr(double r, CONST dim_type *dims)
{
  register struct Expr *result;
  EXPR_NEW(result,e_real);
  result->v.r.rvalue = r;
  result->v.r.dimensions = dims;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateTrueExpr(void)
{
  register struct Expr *result;
  EXPR_NEW(result,e_boolean);
  result->v.bvalue = 1;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateFalseExpr(void)
{
  register struct Expr *result;
  EXPR_NEW(result,e_boolean);
  result->v.bvalue = 0;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateAnyExpr(void)
{
  register struct Expr *result;
  EXPR_NEW(result,e_boolean);
  result->v.bvalue = 2;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateSetExpr(struct Set *set)
{
  register struct Expr *result;
  EXPR_NEW(result,e_set);
  result->v.s = set;
  EXPR_CHECK_MEMORY(result);
  return result;
}


struct Expr *CreateSymbolExpr(symchar *sym)
{
  register struct Expr *result;
  assert(AscFindSymbol(sym)!=NULL);
  EXPR_NEW(result,e_symbol);
  result->v.sym_ptr = sym;
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateQStringExpr(CONST char *qstr)
{
  register struct Expr *result;
  EXPR_NEW(result,e_qstring);
  result->v.sym_ptr = (symchar *)qstr; /* qstr really not symbol */
  EXPR_CHECK_MEMORY(result);
  return result;
}

struct Expr *CreateBuiltin(enum Expr_enum t, struct Set *set)
{
  register struct Expr *result;
  EXPR_NEW(result,t);
  result->v.s = set;
  EXPR_CHECK_MEMORY(result);
  return result;
}

/*------------------------------------------------------------------------------
  MANIPULATION ROUTINES
*/

void LinkExprs(struct Expr *cur, struct Expr *next)
{
  assert(cur!=NULL);
  AssertMemory(cur);
  cur->next = next;
}

struct Expr *NextExprF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->next;
}

enum Expr_enum ExprTypeF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->t;
}

CONST struct Name *ExprNameF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.nptr;
}

CONST struct Func *ExprFuncF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.fptr;
}

long ExprIValueF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.ivalue;
}

double ExprRValueF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.r.rvalue;
}

CONST dim_type *ExprRDimensionsF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.r.dimensions;
}

CONST struct Name *SatisfiedExprNameF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.se.sen ;
}

double SatisfiedExprRValueF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.se.ser.rvalue;
}

CONST dim_type *SatisfiedExprRDimensionsF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.se.ser.dimensions;
}

/* This function can be made HALF as long by being a little
 * smarter about how we return the head of the list.
 */
struct Expr *CopyExprList(CONST struct Expr *e)
{
  register struct Expr *result, *p;
  register CONST struct Expr *ep;
  if (e==NULL) return NULL;
  AssertMemory(e);
  ep = e;
  switch(ep->t){
  case e_func:
    result = EPMALLOC;
    result->v.fptr = ep->v.fptr;
    break;
  case e_var:
    result = EPMALLOC;
    result->v.nptr = CopyName(ep->v.nptr);
    break;
  case e_int:
    result = EPMALLOC;
    result->v.ivalue = ep->v.ivalue;
    break;
  case e_zero:
  case e_real:
    result = EPMALLOC;
    result->v.r.rvalue = ep->v.r.rvalue;
    result->v.r.dimensions = ep->v.r.dimensions;
    break;
  case e_boolean:
    result = EPMALLOC;
    result->v.bvalue = ep->v.bvalue;
    break;
  case e_set:
    result = EPMALLOC;
    result->v.s = CopySetList(ep->v.s);
    break;
  case e_symbol:
    result = EPMALLOC;
    result->v.sym_ptr = ep->v.sym_ptr;
    break;
  case e_card:
  case e_choice:
  case e_sum:
  case e_prod:
  case e_union:
  case e_inter:
    result = EPMALLOC;
    result->v.s = CopySetList(ep->v.s);
    break;
  default:
    result = EPMALLOC;
    break;
  }
  result->t = ep->t;
  AssertMemory(result);
  p = result;
  while (ep->next != NULL) {
    ep = ep->next;
    AssertMemory(ep);
    switch(ep->t) {
    case e_func:
      p->next = EPMALLOC;
      p = p->next;
      p->v.fptr = ep->v.fptr;
      break;
    case e_var:
      p->next = EPMALLOC;
      p = p->next;
      p->v.nptr = CopyName(ep->v.nptr);
      break;
    case e_int:
      p->next = EPMALLOC;
      p = p->next;
      p->v.ivalue = ep->v.ivalue;
      break;
    case e_real:
    case e_zero:
      p->next = EPMALLOC;
      p = p->next;
      p->v.r.rvalue = ep->v.r.rvalue;
      p->v.r.dimensions = ep->v.r.dimensions;
      break;
    case e_boolean:
      p->next = EPMALLOC;
      p = p->next;
      p->v.bvalue = ep->v.bvalue;
      break;
    case e_set:
      p->next = EPMALLOC;
      p = p->next;
      p->v.s = CopySetList(ep->v.s);
      break;
    case e_symbol:
      p->next = EPMALLOC;
      p = p->next;
      p->v.sym_ptr = ep->v.sym_ptr;
      break;
    case e_card:
    case e_choice:
    case e_sum:
    case e_prod:
    case e_union:
    case e_inter:
      p->next = EPMALLOC;
      p = p->next;
      p->v.s = CopySetList(ep->v.s);
      break;
    default:
      p->next = EPMALLOC;
      p = p->next;
      break;
    }
    p->t = ep->t;
    AssertMemory(p);
  }
  p->next = NULL;
  AssertMemory(result);
  return result;
}

void DestroyExprList(struct Expr *e)
{
  register struct Expr *next,*ep;
  ep = e;
  while(ep!=NULL) {
    AssertMemory(ep);
    next = ep->next;
    switch(ep->t) {
		case e_var: 
			DestroyName(ep->v.nptr);
			break;
		case e_set:
		case e_card:
		case e_choice:
		case e_sum:
		case e_prod:
		case e_union:
		case e_inter:
			DestroySetList(ep->v.s);
			break;
		default:
			break;
    }
    EPFREE((char *)ep);
    ep = next;
  }
}

struct Expr *JoinExprLists(struct Expr *e1, struct Expr *e2)
{
  register struct Expr *e;
  if (e1 == NULL) return e2;
  AssertMemory(e1);
  e = e1;
  /* find end of expr list */
  while(e->next) e = e->next;
  /* link to e2 */
  e->next = e2;
  AssertMemory(e1);
  return e1;
}

unsigned long ExprListLength(CONST struct Expr *e)
{
  register CONST struct Expr *ptr;
  unsigned long len = 0L;

  AssertMemory(e);
  ptr = e;
  while (ptr) {
    ptr = ptr->next;
    len++;
  }
  return len;
}


int ExprBValueF(CONST struct Expr *e)
{
  assert(e&&(e->t==e_boolean));
  AssertMemory(e);
  return e->v.bvalue;
}

struct Set *ExprSValueF(CONST struct Expr *e)
{
  assert(e&&(e->t==e_set));
  AssertMemory(e);
  return e->v.s;
}

symchar *ExprSymValueF(CONST struct Expr *e)
{
  assert(e&&(e->t==e_symbol));
  AssertMemory(e);
  return e->v.sym_ptr;
}

CONST char *ExprQStrValueF(CONST struct Expr *e)
{
  assert(e&&(e->t==e_qstring));
  AssertMemory(e);
  return (CONST char *)(e->v.sym_ptr); /* remember not a symbol */
}

CONST struct Set *ExprBuiltinSetF(CONST struct Expr *e)
{
  assert(e!=NULL);
  AssertMemory(e);
  return e->v.s;
}

int ExprsEqual(CONST struct Expr *e1, CONST struct Expr *e2)
{
  if (e1==e2) return 1;
  while ((e1!=NULL)&&(e2!=NULL)){
    AssertMemory(e1);
    AssertMemory(e2);
    if (ExprType(e1)!=ExprType(e2)) return 0;
    switch(ExprType(e1)){
    case e_var:
      if (!NamesEqual(ExprName(e1),ExprName(e2))) return 0;
      break;
    case e_func:
      if (ExprFunc(e1)!=ExprFunc(e2)) return 0;
      break;
    case e_int:
      if (ExprIValue(e1)!=ExprIValue(e2)) return 0;
      break;
    case e_real:
      if ((ExprRValue(e1)!=ExprRValue(e2))||
	  (!SameDimen(ExprRDimensions(e1),ExprRDimensions(e2)))) return 0;
      break;
    case e_boolean:
      if (ExprBValue(e1)!=ExprBValue(e2)) return 0;
      break;
    case e_set:
      if (!SetStructuresEqual(ExprSValue(e1),ExprSValue(e2))) return 0;
      break;
    case e_symbol:
      if (CmpSymchar(ExprSymValue(e1),ExprSymValue(e2))!=0) return 0;
      break;
    default: break;
    }
    e1 = NextExpr(e1);
    e2 = NextExpr(e2);
  }
  return ((e1==NULL)&&(e2==NULL));
}

int CompareExprs(CONST struct Expr *e1, CONST struct Expr *e2)
{
  int ctmp;
  long int ltmp;
  double rtmp;
  if (e1==e2) return 0;
  if (e1==NULL) return 1;
  if (e2==NULL) return -1;
  while ((e1!=NULL)&&(e2!=NULL)){
    AssertMemory(e1);
    AssertMemory(e2);
    ctmp = ExprType(e1) - ExprType(e2);
    if (ctmp != 0) {
      if (ctmp >0) {
        return 1;
      } else {
        return -1;
      }
    }
    switch(ExprType(e1)){
    case e_var:
      ctmp = CompareNames(ExprName(e1),ExprName(e2));
      if (ctmp != 0) return ctmp;
      break;
    case e_func:
      ctmp = strcmp(FuncName(ExprFunc(e1)),FuncName(ExprFunc(e2)));
      if (ctmp != 0) return ctmp;
      break;
    case e_int:
      ltmp = ExprIValue(e1) - ExprIValue(e2);
      if (ltmp != 0L) {
        if (ltmp > 0L) {
          return 1;
        } else {
          return -1;
        }
      }
      break;
    case e_real:
      rtmp = ExprRValue(e1) - ExprRValue(e2);
      if (rtmp != 0.0) {
        if (rtmp > 0.0) {
          return 1;
        } else {
          return -1;
        }
      }
      ctmp = CmpDimen(ExprRDimensions(e1),ExprRDimensions(e2));
      if (ctmp != 0) return ctmp;
      break;
    case e_boolean:
      if (ExprBValue(e1)!=ExprBValue(e2)) {
        if (ExprBValue(e1) >0) {
          return 1;
        } else {
          return -1;
        }
      }
      break;
    case e_set:
      ctmp = CompareSetStructures(ExprSValue(e1),ExprSValue(e2));
      if (ctmp != 0) return ctmp;
      break;
    case e_symbol:
      ctmp = CmpSymchar(ExprSymValue(e1),ExprSymValue(e2));
      if (ctmp != 0) return ctmp;
      break;

    case e_nop:		/* fallthru */
    case e_undefined:		/* fallthru */
    case e_glassbox:		/* fallthru */
    case e_blackbox:		/* fallthru */
    case e_opcode:		/* fallthru */
    case e_token:		/* fallthru */
    case e_zero:		/* fallthru */
    case e_uminus:		/* fallthru */
    case e_plus:		/* fallthru */
    case e_minus:		/* fallthru */
    case e_times:		/* fallthru */
    case e_divide:		/* fallthru */
    case e_power:		/* fallthru */
    case e_ipower:		/* fallthru */
    case e_bol_token:		/* fallthru */
    case e_notequal:		/* fallthru */
    case e_equal:		/* fallthru */
    case e_less:		/* fallthru */
    case e_greater:		/* fallthru */
    case e_lesseq:		/* fallthru */
    case e_greatereq:		/* fallthru */
    case e_maximize:		/* fallthru */
    case e_minimize:		/* fallthru */
    case e_boolean_eq:		/* fallthru */
    case e_boolean_neq:		/* fallthru */
    case e_or:		/* fallthru */
    case e_and:		/* fallthru */
    case e_not:
      break;
    case e_satisfied:
      ctmp = CompareNames(SatisfiedExprName(e1),SatisfiedExprName(e2));
      if (ctmp != 0) return ctmp;
      rtmp = SatisfiedExprRValue(e1) - SatisfiedExprRValue(e2);
      if (rtmp != 0.0) {
        if (rtmp > 0.0) {
          return 1;
        } else {
          return -1;
        }
      }
      ctmp = CmpDimen(SatisfiedExprRDimensions(e1),
                      SatisfiedExprRDimensions(e2));
      if (ctmp != 0) return ctmp;
      break;
    case e_subexpr:
    case e_const:
    case e_par:
    case e_in:
    case e_st:
      break;
    case e_qstring:
      ctmp = strcmp(ExprQStrValue(e1),ExprQStrValue(e2));
      if (ctmp != 0) return ctmp;
      break;
    case e_sum: /* fall thru */
    case e_prod: /* fall thru */
    case e_card: /* fall thru */
    case e_choice: /* fall thru */
    case e_union: /* fall thru */
    case e_inter:
      ctmp = CompareSetStructures(ExprBuiltinSet(e1),ExprBuiltinSet(e2));
      if (ctmp != 0) return ctmp;
      break;
    }
    e1 = NextExpr(e1);
    e2 = NextExpr(e2);
  }
  /* shorter is < longer */
  if (e2!=NULL) return 1;
  if (e1!=NULL) return -1;
  return 0;
}
