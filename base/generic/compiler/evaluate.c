/*
 *  Evaluation Routines
 *  by Tom Epperly
 *  Created: 1/16/90
 *  Version: $Revision: 1.23 $
 *  Version control file: $RCSfile: evaluate.c,v $
 *  Date last modified: $Date: 1998/03/17 22:08:30 $
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
 *
 */

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/dstring.h"
#include "general/list.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/setinstval.h"
#include "compiler/value_type.h"
#include "compiler/name.h"
#include "compiler/temp.h"
#include "compiler/sets.h"
#include "compiler/exprs.h"
#include "compiler/find.h"
#include "compiler/exprio.h"
#include "compiler/evaluate.h"

#ifndef lint
static CONST char EvaluationRoutineRCSid[] = "$Id: evaluate.c,v 1.23 1998/03/17 22:08:30 ballan Exp $";
#endif

static struct gl_list_t *g_names_needed = NULL;
/* global var so we are not passing nlist everywhere
 * that we used to pass *EvaluateName.
 */
#define GNN g_names_needed

/*********************************************************************\
  Support Evaluation Routines -- stacks.
  small, fast, and local. Do not export. BAA. 2/96
  changed from longs to ints for size and capacity,
  particularly because expression stacks just don't get that deep.
\*********************************************************************/
struct stack_t {
  struct value_t *ptr; 	/* data pointer */
  unsigned capacity;	/* length of data */
  unsigned long size;	/* pointer while recycled and data while in use */
};

#define SMALLOC(x) x=(struct stack_t *)ascmalloc(sizeof(struct stack_t));
#define DMALLOC(x,n) x\
       =(struct value_t *)ascmalloc((unsigned)n*\
				    (unsigned)sizeof(struct value_t))

static struct stack_t *AllocStack(unsigned int capacity)
{
  struct stack_t *result;
  SMALLOC(result);
  result->size =0;
  DMALLOC(result->ptr,capacity);
  result->capacity = capacity;
  return result;
}

static void DeAllocStack(struct stack_t *stack)
{
  if (stack!=NULL){
    ascfree((char *)stack->ptr);
    stack->ptr = NULL;
    ascfree((char *)stack);
  }
}

#define RECYCLESTACKSIZE 20 /* largest stack we will attempt to recycle */
static struct stack_t * g_recycle_expreval_stacks[RECYCLESTACKSIZE];
/* ANSI ASSUMPTION: this array is initialize to NULL values */
/* Assumption about client:
 * It will never try to free any element of the stack, nor
 * will it ever put the stack inside a struct value_t managed
 * (created and therefore destroyed) by value_type.c.
 * Nor will the client ever give the stack to someone else to
 * deallocate later.
 */
static struct stack_t *CreateStack(unsigned int cap)
{
  struct stack_t *result;
  if (cap < RECYCLESTACKSIZE && g_recycle_expreval_stacks[cap] !=NULL) {
    result = g_recycle_expreval_stacks[cap];
    /* move the NEXT recycled pointer into recycle array */
    g_recycle_expreval_stacks[cap] = (struct stack_t *)result->size;
    result->size =0; /* ptr and capacity valid from initial allocation */
    return result;
  } else {
    return AllocStack(cap);
  }
}

static void DestroyStack(struct stack_t *stack)
{
  if (stack==NULL) return;
  if (stack->capacity < RECYCLESTACKSIZE) {
    /* the recycle list NEXT pointer has to be cast into the size slot */
    stack->size =(unsigned long)(g_recycle_expreval_stacks[stack->capacity]);
    /* push stack on LIFO recycle list */
    g_recycle_expreval_stacks[stack->capacity] = stack;
    return;
  } else {
    DeAllocStack(stack);
  }
}

/* Clear and reinit expression stack recycler */
#define PRINTSTACKSTATS 0
void ClearRecycleStack(void) {
  unsigned int i,cnt;
  struct stack_t *stp; /* pointer to a recycled stack */
#if PRINTSTACKSTATS
  PRINTF("Stack cap.\tRecycle\n");
#endif
  for (i=0; i < RECYCLESTACKSIZE; i++) {
    cnt=0;
    while( (stp = g_recycle_expreval_stacks[i]) != NULL) {
      g_recycle_expreval_stacks[i] = (struct stack_t *)stp->size;
      DeAllocStack(stp);
      cnt++;
    }
#if PRINTSTACKSTATS
    PRINTF("%d\t\t%d\n",i,cnt);
#endif
  }
}

#if 0
static
unsigned long StackSize(struct stack_t *stack)
{
  assert(stack&&stack->ptr);
  return stack->size;
}
#endif

static
struct value_t StackPopTop(struct stack_t *stack)
{
  assert(stack&&stack->ptr&&stack->size);
  return stack->ptr[--(stack->size)];
}

static
void StackPush(struct stack_t *stack, struct value_t value)
{
  assert(stack&&stack->ptr&&(stack->size<stack->capacity));
  stack->ptr[(stack->size)++] = value;
}

/*********************************************************************\
  Expression Evaluation Routines.
\*********************************************************************/

static
unsigned int ExprStackDepth(CONST struct Expr *ex,
			    CONST struct Expr *stop)
{
  register unsigned int maxdepth=0,depth=0;
  while (ex!=stop){
    AssertMemory(ex);
    switch(ExprType(ex)){
    case e_var:
    case e_zero:
    case e_int:
    case e_satisfied:
    case e_real:
    case e_boolean:
    case e_set:
    case e_symbol:
    case e_card:
    case e_choice:
    case e_sum:
    case e_prod:
    case e_union:
    case e_inter:
      if ((++depth)>maxdepth) maxdepth=depth;
      break;
      /* binary operators */
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
    case e_or:
    case e_and:
    case e_in:
    case e_equal:
    case e_notequal:
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
    case e_boolean_eq:
    case e_boolean_neq:
      depth--;
      break;
    case e_st:
      Asc_Panic(2, NULL,
                "Such that in atoms is unsupported for the time being.\n");
      break;
      /* no change*/
    case e_func:
    case e_uminus:
    case e_not:
      break;
    case e_minimize:
    case e_maximize:
      Asc_Panic(2, NULL,
                "Maximize and minimize aren't allowed in expressions.\n"
                "They are only allowed in relations.\n");
      break;
    default:
      Asc_Panic(2, NULL, "Unknown expression node type.\n");
      break;
    }
    ex = NextExpr(ex);
  }
  return maxdepth;
}

static
CONST struct Expr *ContainsSuchThat(CONST struct Expr *expr,
				    CONST struct Expr *stop)
{
  while (expr!=stop){
    AssertMemory(expr);
    if (ExprType(expr)==e_st) return expr;
    expr = NextExpr(expr);
  }
  return 0;
}

static
unsigned SuchThatForm(CONST struct Expr *expr,
		      CONST struct Expr *stop,
		      CONST struct Expr **depth_one)
{
  unsigned depth=0;
  CONST struct Expr *previous=NULL;
  *depth_one = NULL;
  while (expr!=stop){
    AssertMemory(expr);
    switch(ExprType(expr)){
    case e_var:
    case e_zero:
    case e_int:
    case e_satisfied:
    case e_real:
    case e_boolean:
    case e_set:
    case e_symbol:
    case e_card:
    case e_choice:
    case e_sum:
    case e_prod:
    case e_union:
    case e_inter:
      if ((++depth)==1) *depth_one = expr;
      break;
      /* binary operators */
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
    case e_or:
    case e_and:
    case e_in:
    case e_equal:
    case e_notequal:
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
    case e_boolean_eq:
    case e_boolean_neq:
      if ((--depth)==1) *depth_one = expr;
      break;
    case e_func:
    case e_uminus:
    case e_not:
      if (depth==1) *depth_one = expr;
      break;
    case e_st:
      if (previous==NULL) return 2; /* error */
      if (NextExpr(expr)!=stop) return 2; /* error */
      if (ExprType(*depth_one)==e_in) return 0;	/* set definition */
      if (ExprType(previous)==e_in) return 1; /* list of values */
      return 2; /* error */
    case e_minimize:
    case e_maximize:
      Asc_Panic(2, NULL,
                "Maximize and minimize are not allowed in expression.\n"
                "They are only allowed in relations.\n");
      break;
    default:
      Asc_Panic(2, NULL, "Unknown expression node type.\n");
      break;
    }
    previous = expr;
    expr = NextExpr(expr);
  }
  return 2;
}

static
int GetNameAndSet(CONST struct Expr *ex, CONST struct Expr *stop,
		  symchar **name, struct value_t *value,
		  struct value_t (*EvaluateName) (/* ??? */))
{
  /* NAME SET IN */
  if (ExprType(ex)==e_var){
    if ((*name = SimpleNameIdPtr(ExprName(ex)))!=NULL){
      *value = EvaluateExpr(NextExpr(ex),stop,EvaluateName);
      if (ValueKind(*value)==set_value) {
        return 0;
      }
      if (ValueKind(*value)==error_value) {
        return 1;
      } else {
        if (ValueKind(*value)==integer_value) {
          FPRINTF(ASCERR,
                 "Asc-Error: Found integer constant where set expected: ");
          WriteExprNode(ASCERR,NextExpr(ex));
          FPRINTF(ASCERR,"\n");
        }
        if (ValueKind(*value)==symbol_value) {
          FPRINTF(ASCERR,
                 "Asc-Error: Found symbol constant where set expected: ");
          WriteExprNode(ASCERR,NextExpr(ex));
          FPRINTF(ASCERR,"\n");
        }
	DestroyValue(value);
	*value = CreateErrorValue(type_conflict);
	return 1;
      }
    }
  }
  *value = CreateErrorValue(incorrect_such_that);
  return 1;
}

static
int GetNameAndSetNamesNeeded(CONST struct Expr *ex,
                             CONST struct Expr *stop,
		             symchar **name)
{
  /* NAME SET IN */
  if (ExprType(ex)==e_var){
    *name = SimpleNameIdPtr(ExprName(ex));
    if (*name != NULL){
      EvaluateNamesNeeded(NextExpr(ex),stop,GNN);
    }
    return 0;
  }
  return 1;
}

static
struct value_t EvaluateLeftIteration(CONST struct Expr *expr,
				     CONST struct Expr *stop,
				     CONST struct Expr *depth_one,
				     struct value_t (*EvaluateName)(/* ??? */))
{
  CONST struct Expr *st_node,*rhs;
  struct set_t *sptr;
  symchar *tmp_name;		/* name of temporary variable */
  struct value_t iteration_set,tmp_value,l_value,rhs_value;
  unsigned long c,len;
  IVAL(iteration_set);
  IVAL(tmp_value);
  IVAL(l_value);
  IVAL(rhs_value);
  if (GetNameAndSet(expr,depth_one,&tmp_name,&iteration_set,EvaluateName)) {
    return iteration_set;
  }
  sptr = SetValue(iteration_set);
  rhs = NextExpr(depth_one);
  st_node = ContainsSuchThat(rhs,stop);
  switch(SetKind(sptr)){
  case empty_set:
    DestroyValue(&iteration_set);
    return CreateVacantListValue();
  case integer_set:
  case string_set:
    if (TempExists(tmp_name)){
      FPRINTF(ASCERR,"Reused temporary variable %s.\n",SCP(tmp_name));
      DestroyValue(&iteration_set);
      return CreateErrorValue(temporary_variable_reused);
    }
    l_value = CreateEmptyListValue();
    AddTemp(tmp_name);
    len = Cardinality(sptr);
    for(c=1;c<=len;c++){
      if (SetKind(sptr)==string_set) {
	tmp_value = CreateSymbolValue(FetchStrMember(sptr,c),1);
      } else {
	tmp_value = CreateIntegerValue(FetchIntMember(sptr,c),1);
      }
      SetTemp(tmp_name,tmp_value);
      rhs_value =EvaluateExpr(rhs,st_node,EvaluateName);
      if (ValueKind(rhs_value)!=boolean_value){
	DestroyValue(&tmp_value);
	RemoveTemp(tmp_name);
	DestroyValue(&iteration_set);
	DestroyValue(&l_value);
	if (ValueKind(rhs_value)==error_value) {
          return rhs_value;
	} else {
	  DestroyValue(&rhs_value);
	  return CreateErrorValue(incorrect_such_that);
	}
      }
      if (BooleanValue(rhs_value)) {
	AppendToListValue(l_value,tmp_value);
      }
      DestroyValue(&tmp_value);
      DestroyValue(&rhs_value);
    }
    RemoveTemp(tmp_name);
    DestroyValue(&iteration_set);
    return l_value;
  }
  /*NOTREACHED*/
  FPRINTF(ASCERR,"EvaluateLeftIteration returning erroneous value\n");
  return tmp_value;
}

static
void EvaluateLeftIterationNamesNeeded(CONST struct Expr *expr,
                                      CONST struct Expr *stop,
                                      CONST struct Expr *depth_one)
{
  CONST struct Expr *st_node,*rhs;
  symchar *tmp_name;		/* name of temporary variable */
  if (GetNameAndSetNamesNeeded(expr,depth_one,&tmp_name)) {
    return;
  }
  rhs = NextExpr(depth_one);
  st_node = ContainsSuchThat(rhs,stop);
  if (tmp_name !=NULL && TempExists(tmp_name)){
    FPRINTF(ASCERR,"Reused temporary variable %s.\n",SCP(tmp_name));
  }
  AddTemp(tmp_name);
  GNN = EvaluateNamesNeeded(rhs,st_node,GNN);
  RemoveTemp(tmp_name);
  return;
}

static
CONST struct Expr *NodeBeforeSuchThat(CONST struct Expr *ex)
{
  while(ExprType(NextExpr(ex))!=e_st) {
    ex = NextExpr(ex);
  }
  return ex;
}

static
struct value_t EvaluateRightIteration(CONST struct Expr *expr,
				      CONST struct Expr *stop,
				      CONST struct Expr *depth_one,
				      struct value_t (*EvaluateName)(/*???*/))
{
  symchar *tmp_name;
  CONST struct Expr *node;
  struct value_t iteration_set,l_value,tmp_value,lhs_value;
  struct set_t *sptr;
  unsigned long c,len;

  (void)stop;  /*  stop gcc whine about unused parameter  */

  IVAL(iteration_set);
  IVAL(tmp_value);
  IVAL(l_value);
  IVAL(lhs_value);
  node = NodeBeforeSuchThat(depth_one);
  if (GetNameAndSet(NextExpr(depth_one),node,
		    &tmp_name,&iteration_set,EvaluateName))
    return iteration_set;
  node = NextExpr(depth_one);
  sptr = SetValue(iteration_set);
  switch(SetKind(sptr)){
  case empty_set:
    DestroyValue(&iteration_set);
    return CreateVacantListValue();
  case integer_set:
  case string_set:
    if (TempExists(tmp_name)){
      FPRINTF(ASCERR,"Reused temporary variable %s.\n",SCP(tmp_name));
      DestroyValue(&iteration_set);
      return CreateErrorValue(temporary_variable_reused);
    }
    l_value = CreateEmptyListValue();
    AddTemp(tmp_name);
    len = Cardinality(sptr);
    for(c=1;c<=len;c++){
      if (SetKind(sptr)==string_set) {
	tmp_value = CreateSymbolValue(FetchStrMember(sptr,c),1);
      } else {
	tmp_value = CreateIntegerValue(FetchIntMember(sptr,c),1);
      }
      SetTemp(tmp_name,tmp_value);
      lhs_value=EvaluateExpr(expr,node,EvaluateName);
      if (ValueKind(lhs_value)==error_value){
	DestroyValue(&tmp_value);
	RemoveTemp(tmp_name);
	DestroyValue(&iteration_set);
	DestroyValue(&l_value);
	return lhs_value;
      }
      AppendToListValue(l_value,lhs_value);
      DestroyValue(&tmp_value);
    }
    RemoveTemp(tmp_name);
    DestroyValue(&iteration_set);
    return l_value;
  }
  /*NOTREACHED*/
  FPRINTF(ASCERR,"EvaluateRightIteration returning erroneous value\n");
  return tmp_value;
}

static
void EvaluateRightIterationNamesNeeded(CONST struct Expr *expr,
				       CONST struct Expr *stop,
				       CONST struct Expr *depth_one)
{
  symchar *tmp_name;
  CONST struct Expr *node;

  (void)stop;  /*  stop gcc whine about unused parameter  */

  node = NodeBeforeSuchThat(depth_one);
  if (GetNameAndSetNamesNeeded(NextExpr(depth_one),node,&tmp_name)) {
    return;
  }
  node = NextExpr(depth_one);
  if (tmp_name !=NULL && TempExists(tmp_name)){
    FPRINTF(ASCERR,"Reused temporary variable %s.\n",SCP(tmp_name));
    return;
  }
  AddTemp(tmp_name);
  GNN = EvaluateNamesNeeded(expr,node,GNN);
  RemoveTemp(tmp_name);
  return;
}

static
struct value_t EvaluateSuchThat(CONST struct Expr *expr,
				CONST struct Expr *stop,
				struct value_t (*EvaluateName) (/* ??? */))
{
  CONST struct Expr *depth_one;
  switch(SuchThatForm(expr,stop,&depth_one)){
  case 0:
    return EvaluateLeftIteration(expr,stop,depth_one,EvaluateName);
  case 1:
    return EvaluateRightIteration(expr,stop,depth_one,EvaluateName);
  default:
    return CreateErrorValue(incorrect_such_that);
  }
}

static
void EvaluateSuchThatNamesNeeded(CONST struct Expr *expr,
                                 CONST struct Expr *stop)

{
  CONST struct Expr *depth_one;
  switch(SuchThatForm(expr,stop,&depth_one)){
  case 0:
    EvaluateLeftIterationNamesNeeded(expr,stop,depth_one);
    return;
  case 1:
    EvaluateRightIterationNamesNeeded(expr,stop,depth_one);
    return;
  default:
    break;
  }
}

struct value_t EvaluateExpr(CONST struct Expr *expr, CONST struct Expr *stop,
			    struct value_t (*EvaluateName) (/* ? */))
{
  struct value_t top,next;
  symchar *cptr;
  register struct stack_t *stack;
  IVAL(top);
  IVAL(next);
  if (ContainsSuchThat(expr,stop)!=NULL) {
    return EvaluateSuchThat(expr,stop,EvaluateName);
  }
  stack = CreateStack(ExprStackDepth(expr,stop));
  while(expr!=stop){
    AssertMemory(expr);
    switch(ExprType(expr)){
    case e_var:			/* variable */
      cptr = SimpleNameIdPtr(ExprName(expr));
      if ((cptr != NULL)&&TempExists(cptr)) {
	top = TempValue(cptr);
      } else {
	top = (*EvaluateName)(ExprName(expr));
      }
      StackPush(stack,top);
      break;
    case e_func:		/* function evaluation */
      top = ApplyFunction(StackPopTop(stack),ExprFunc(expr));
      StackPush(stack,top);
      break;
    case e_satisfied:		/* satisfied evaluation */
      top = InstanceEvaluateSatisfiedName(SatisfiedExprName(expr),
                                          SatisfiedExprRValue(expr));
      StackPush(stack,top);
      break;
    case e_int:			/* integer constant */
      top = CreateIntegerValue(ExprIValue(expr),1);
      StackPush(stack,top);
      break;
    case e_zero:		/* ambiguous 0 */
      top = CreateRealValue(0.0,WildDimension(),1);
      StackPush(stack,top);
      break;
    case e_real:		/* real constant */
      top = CreateRealValue(ExprRValue(expr),ExprRDimensions(expr),1);
      StackPush(stack,top);
      break;
    case e_boolean:		/* boolean constant */
      top = CreateBooleanValue(ExprBValue(expr),1);
      StackPush(stack,top);
      break;
    case e_set:			/* set */
      top = EvaluateSet(ExprSValue(expr),EvaluateName);
      StackPush(stack,CreateSetFromList(top));
      DestroyValue(&top);
      break;
    case e_symbol:		/* symbol constant */
      top = CreateSymbolValue(ExprSymValue(expr),1);
      StackPush(stack,top);
      break;
    case e_plus:		/* binary plus operator */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,AddValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_minus:		/* binary minus operator */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,SubtractValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_times:		/* binary multiplication operator */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,MultiplyValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_divide:		/* binary division operator */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,DivideValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_power:		/* binary exponentiation operator */
    case e_ipower:		/* binary exponentiation operator */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,PowerValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_card:		/* cardinality operator */
      top = EvaluateSet(ExprBuiltinSet(expr),EvaluateName);
      next = CreateSetFromList(top);
      StackPush(stack,CardValues(next));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_choice:		/* choice operator */
      top = EvaluateSet(ExprBuiltinSet(expr),EvaluateName);
      next = CreateSetFromList(top);
      StackPush(stack,ChoiceValues(next));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_sum:			/* summation operator */
      top = EvaluateSet(ExprBuiltinSet(expr),EvaluateName);
      StackPush(stack,SumValues(top));
      DestroyValue(&top);
      break;
    case e_prod:		/* product operator */
      top = EvaluateSet(ExprBuiltinSet(expr),EvaluateName);
      StackPush(stack,ProdValues(top));
      DestroyValue(&top);
      break;
    case e_union:		/* union operator */
      top = EvaluateSet(ExprBuiltinSet(expr),EvaluateName);
      StackPush(stack,UnionValues(top));
      DestroyValue(&top);
      break;
    case e_inter:		/* intersection operator */
      top = EvaluateSet(ExprBuiltinSet(expr),EvaluateName);
      StackPush(stack,IntersectionValues(top));
      DestroyValue(&top);
      break;
    case e_or:			/* binary logical OR operator */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,OrValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_and:			/* binary logical AND operator */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,AndValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_in:			/* set membership test */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,InValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_equal:		/* equality test */
    case e_boolean_eq:		/* these two ought to be separated */
				/* = should bind more tightly than == */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,EqualValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_notequal:		/* non-equality test */
    case e_boolean_neq:		/* these two ought to be separated */
				/* <> should bind more tightly than != */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,NotEqualValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_less:		/* less than test */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,LessValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_greater:		/* greater than test */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,GreaterValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_lesseq:		/* less then or equal test */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,LessEqValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_greatereq:		/* greater than or equal test */
      top = StackPopTop(stack);
      next = StackPopTop(stack);
      StackPush(stack,GreaterEqValues(next,top));
      DestroyValue(&top);
      DestroyValue(&next);
      break;
    case e_uminus:		/* unary minus operator */
      top = StackPopTop(stack);
      StackPush(stack,NegateValue(top));
      DestroyValue(&top);
      break;
    case e_not:			/* unary logical NOT operator */
      top = StackPopTop(stack);
      StackPush(stack,NotValue(top));
      DestroyValue(&top);
      break;
    case e_st:			/* such that  */
      Asc_Panic(2, NULL, "Something is royally wrong is EvaluateExpr.\n");
      break;
    case e_minimize:
    case e_maximize:
      Asc_Panic(2, NULL,
                "Maximize and minimize are not allowed in expression.\n"
                "They are only allowed in relations.\n");
      break;
    default:
      Asc_Panic(2, NULL, "Unknown expression node in EvaluateExpr.\n");
      break;
    }
    expr = NextExpr(expr);
  }
  assert(StackSize(stack)==1);
  top = StackPopTop(stack);
  DestroyStack(stack);
  return top;
}

struct gl_list_t *EvaluateNamesNeeded(CONST struct Expr *expr,
                                      CONST struct Expr *stop,
			              struct gl_list_t *nlist)
{
  symchar *cptr;
  CONST struct Name *n;

  if (nlist ==NULL) {
    nlist= gl_create(3L);
  }
  assert(nlist!=NULL);
  GNN = nlist; /* always done, so we don't need to set it back NULL after */

  if (ContainsSuchThat(expr,stop)!=NULL) {
    EvaluateSuchThatNamesNeeded(expr,stop);
    return GNN;
  }
  while(expr!=stop){
    AssertMemory(expr);
    switch(ExprType(expr)){
    case e_var:			/* variable */
      cptr = SimpleNameIdPtr(ExprName(expr));
      if ( cptr == NULL || TempExists(cptr)==0 ) {
        /* append if name not already seen in list */
        if (gl_search(nlist,(VOIDPTR)ExprName(expr),
                      (CmpFunc)CompareNames) == 0L)
        {
	  gl_append_ptr(nlist,(VOIDPTR)ExprName(expr));
        }
      }
      n = ExprName(expr);
      n = NextName(n);
      while (n != NULL) {
        if (NameId(n) == 0) {
          GNN = EvaluateSetNamesNeeded(NameSetPtr(n),GNN);
        }
        n = NextName(n);
      }
      break;
    case e_func:		/* function evaluation */
    case e_int:			/* integer constant */
    case e_zero:		/* ambiguous 0 */
    case e_real:		/* real constant */
    case e_boolean:		/* boolean constant */
    case e_satisfied:           /* satisified expression */
      break;
    case e_set:			/* set */
      GNN = EvaluateSetNamesNeeded(ExprSValue(expr),GNN);
      break;
    case e_symbol:		/* symbol constant */
    case e_plus:		/* binary plus operator */
    case e_minus:		/* binary minus operator */
    case e_times:		/* binary multiplication operator */
    case e_divide:		/* binary division operator */
    case e_power:		/* binary exponentiation operator */
    case e_ipower:		/* binary exponentiation operator */
      break;
    case e_card:		/* cardinality operator */
    case e_choice:		/* choice operator */
    case e_sum:			/* summation operator */
    case e_prod:		/* product operator */
    case e_union:		/* union operator */
    case e_inter:		/* intersection operator */
      GNN = EvaluateSetNamesNeeded(ExprBuiltinSet(expr),GNN);
      break;
    case e_or:			/* binary logical OR operator */
    case e_and:			/* binary logical AND operator */
    case e_in:			/* set membership test */
    case e_equal:		/* equality test */
    case e_bol_token:
    case e_boolean_eq:
    case e_boolean_neq:
    case e_notequal:		/* non-equality test */
    case e_less:		/* less than test */
    case e_greater:		/* greater than test */
    case e_lesseq:		/* less then or equal test */
    case e_greatereq:		/* greater than or equal test */
    case e_uminus:		/* unary minus operator */
    case e_not:			/* unary logical NOT operator */
      break;
    case e_st:			/* such that  */
      Asc_Panic(2, "EvaluateNamesNeeded",
                "Something is royally wrong is EvaluateNamesNeeded.\n");
      break;
    case e_minimize:
    case e_maximize:
      break;
    default:
      Asc_Panic(2, NULL, "Unknown expression node in EvaluateNamesNeeded.\n");
      break;
    }
    expr = NextExpr(expr);
  }
  return GNN;
}

struct gl_list_t *EvaluateNamesNeededShallow(CONST struct Expr *expr,
                                             CONST struct Expr *stop,
			                     struct gl_list_t *nlist)
{
  symchar *cptr;

  if (nlist ==NULL) {
    nlist= gl_create(3L);
  }
  assert(nlist!=NULL);
  GNN = nlist; /* always done, so we don't need to set it back NULL after */

  if (ContainsSuchThat(expr,stop)!=NULL) {
    EvaluateSuchThatNamesNeeded(expr,stop);
    return GNN;
  }
  while(expr!=stop){
    AssertMemory(expr);
    switch(ExprType(expr)){
    case e_var:			/* variable */
      cptr = SimpleNameIdPtr(ExprName(expr));
      if ( cptr == NULL || TempExists(cptr)==0 ) {
        /* append if name not already seen in list */
        if (gl_search(nlist,(VOIDPTR)ExprName(expr),
                      (CmpFunc)CompareNames) == 0L)
        {
	  gl_append_ptr(nlist,(VOIDPTR)ExprName(expr));
        }
      }
      break;
    case e_func:		/* function evaluation */
    case e_int:			/* integer constant */
    case e_zero:		/* ambiguous 0 */
    case e_real:		/* real constant */
    case e_boolean:		/* boolean constant */
    case e_satisfied:           /* satisified expression */
      break;
    case e_set:			/* set */
      GNN = EvaluateSetNamesNeededShallow(ExprSValue(expr),GNN);
      break;
    case e_symbol:		/* symbol constant */
    case e_plus:		/* binary plus operator */
    case e_minus:		/* binary minus operator */
    case e_times:		/* binary multiplication operator */
    case e_divide:		/* binary division operator */
    case e_power:		/* binary exponentiation operator */
    case e_ipower:		/* binary exponentiation operator */
      break;
    case e_card:		/* cardinality operator */
    case e_choice:		/* choice operator */
    case e_sum:			/* summation operator */
    case e_prod:		/* product operator */
    case e_union:		/* union operator */
    case e_inter:		/* intersection operator */
      GNN = EvaluateSetNamesNeededShallow(ExprBuiltinSet(expr),GNN);
      break;
    case e_or:			/* binary logical OR operator */
    case e_and:			/* binary logical AND operator */
    case e_in:			/* set membership test */
    case e_equal:		/* equality test */
    case e_bol_token:
    case e_boolean_eq:
    case e_boolean_neq:
    case e_notequal:		/* non-equality test */
    case e_less:		/* less than test */
    case e_greater:		/* greater than test */
    case e_lesseq:		/* less then or equal test */
    case e_greatereq:		/* greater than or equal test */
    case e_uminus:		/* unary minus operator */
    case e_not:			/* unary logical NOT operator */
      break;
    case e_st:			/* such that  */
      Asc_Panic(2, "EvaluateNamesNeededShallow",
                "Something is wrong in EvaluateNamesNeededShallow.\n");
      break;
    case e_minimize:
    case e_maximize:
      break;
    default:
      Asc_Panic(2, "EvaluateNamesNeededShallow",
                "Unknown expression in EvaluateNamesNeededShallow.\n");
      break;
    }
    expr = NextExpr(expr);
  }
  return GNN;
}

/* in this function we should do a bunch of logic checking before
 * calling the CreateEmptyList; if we can he accounts for most
 * of our memory activity
 */
struct value_t EvaluateSet(CONST struct Set *sptr,
			   struct value_t (*EvaluateName) (/* ??? */))
{
  struct value_t result,lower,upper;
  long l,u,c;
  int previous_state;
  previous_state = EvaluatingSets;  /* save the state as called recursively */
  EvaluatingSets = 1;
  result = CreateEmptyListValue();
  while (sptr!=NULL){
    AssertMemory(sptr);
    if (SetType(sptr)){ /* range */
      lower = EvaluateExpr(GetLowerExpr(sptr),NULL,EvaluateName);
      if (ValueKind(lower)==error_value){
	DestroyValue(&result);
	EvaluatingSets = previous_state;
	return lower;
      }
      upper = EvaluateExpr(GetUpperExpr(sptr),NULL,EvaluateName);
      if (ValueKind(upper)==error_value){
	DestroyValue(&lower);
	DestroyValue(&result);
	EvaluatingSets = previous_state;
	return upper;
      }
      if((ValueKind(lower)!=integer_value)||(ValueKind(upper)!=integer_value)){
	DestroyValue(&lower);
	DestroyValue(&upper);
	DestroyValue(&result);
	EvaluatingSets = previous_state;
	return CreateErrorValue(type_conflict);
      }
      l = IntegerValue(lower);
      u = IntegerValue(upper);
      DestroyValue(&lower);
      DestroyValue(&upper);
      for(c=l;c<=u;c++) {
	AppendToListValue(result,CreateIntegerValue(c,1));
      }
    } else {			/* singleton */
      lower = EvaluateExpr(GetSingleExpr(sptr),NULL,EvaluateName);
      if (ValueKind(lower)==error_value){
	DestroyValue(&result);
	EvaluatingSets = previous_state;
	return lower;
      }
      AppendToListValue(result,lower);
    }
    sptr = NextSet(sptr);
  }
  EvaluatingSets = previous_state;
  return result;
}

/* in this function we should do a bunch of logic checking before
 * calling the CreateEmptyList; if we can he accounts for most
 * of our memory activity.
 */
struct gl_list_t *EvaluateSetNamesNeeded(CONST struct Set *sptr,
		                         struct gl_list_t *nlist)
{
  if (nlist ==NULL) {
    nlist= gl_create(3L);
  }
  assert(nlist!=NULL);
  GNN = nlist; /* always done, so we don't need to set it back NULL after */

  while (sptr!=NULL){
    AssertMemory(sptr);
    if (SetType(sptr)){ /* range */
      GNN = EvaluateNamesNeeded(GetLowerExpr(sptr),NULL,GNN);
      GNN = EvaluateNamesNeeded(GetUpperExpr(sptr),NULL,GNN);
    } else {			/* singleton */
      GNN = EvaluateNamesNeeded(GetSingleExpr(sptr),NULL,GNN);
    }
    sptr = NextSet(sptr);
  }
  return GNN;
}

/* in this function we should do a bunch of logic checking before
 * calling the CreateEmptyList; if we can he accounts for most
 * of our memory activity.
 */
struct gl_list_t *EvaluateSetNamesNeededShallow(CONST struct Set *sptr,
                                                struct gl_list_t *nlist)
{
  if (nlist ==NULL) {
    nlist= gl_create(3L);
  }
  assert(nlist!=NULL);
  GNN = nlist; /* always done, so we don't need to set it back NULL after */

  while (sptr!=NULL){
    AssertMemory(sptr);
    if (SetType(sptr)){ /* range */
      GNN = EvaluateNamesNeededShallow(GetLowerExpr(sptr),NULL,GNN);
      GNN = EvaluateNamesNeededShallow(GetUpperExpr(sptr),NULL,GNN);
    } else {			/* singleton */
      GNN = EvaluateNamesNeededShallow(GetSingleExpr(sptr),NULL,GNN);
    }
    sptr = NextSet(sptr);
  }
  return GNN;
}
