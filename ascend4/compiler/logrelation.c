/*
 *  Logical Relation  construction  routines
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.18 $
 *  Version control file: $RCSfile: logrelation.c,v $
 *  Date last modified: $Date: 1998/02/20 02:10:24 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright 1997, Carnegie Mellon University
 *
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

#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/pool.h"
#include "general/list.h"
#include "general/dstring.h"
#include "general/stack.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/name.h"
#include "compiler/exprs.h"
#include "compiler/sets.h"
#include "compiler/value_type.h"
#include "compiler/evaluate.h"
#include "compiler/forvars.h"
#include "compiler/find.h"
#include "compiler/relation_type.h"
#include "compiler/logical_relation.h"
#include "compiler/relation_util.h"
#include "compiler/logrelation.h"
#include "compiler/logrel_util.h"
#include "compiler/rel_common.h"
#include "compiler/temp.h"
#include "compiler/instance_enum.h"
#include "compiler/instquery.h"
#include "compiler/mathinst.h"
#include "compiler/atomvalue.h"
#include "compiler/instance_io.h"


/*
 * Some global and exported variables.  */

/* boolean variables in the lofical relation */
struct gl_list_t *g_logrelation_bvar_list = NULL;
/* relations or logrelations in satisfied's exprssions */
struct gl_list_t *g_logrelation_satrel_list = NULL;


/*
 * Simplification of Logical Equations not implemented yet
 * We have to create
 * the function SimplifyLogTermBuf (and subsidiary functions)
 */
int g_simplify_logrelations = 0;

/*********************************************************************\
  Section for creation and management of logical relation terms.
  It is cheaper to create logical relation terms in arrays the size of
  the union than individually because of operating system overhead.
  Lookout, the tokens have unionized: next they'll want a raise.
\*********************************************************************/
/*
 * The define POOL_ALLOCLOGTERM is for people who are pulling terms out
 * of a pool and promise to return them immediately.
 */

static pool_store_t g_logterm_pool = NULL;
/* A pool_store for 1 expression.
 * Each time an expression is completed, it will be copied
 * into an array which can be created already knowing
 * its proper size. The array will be naturally in postfix.
 */

#define POOL_ALLOCLOGTERM LOGA_TERM(pool_get_element(g_logterm_pool))
/* get a token. Token is the size of the LogRelTermUnion. */
#define POOL_LOGRESET pool_clear_store(g_logterm_pool)
/* reset the pool for next expression */

static struct {
  long startcheck;
  size_t len;
  size_t cap;
  struct logrel_term **buf;
  unsigned long *termstack;
  unsigned long termstackcap;
  long endcheck;
} g_logterm_ptrs = {1234567890,0,0,NULL,NULL,0,987654321};

#define TPBUF_LOGRESET (g_logterm_ptrs.len=0)
/* forget about all the logical terms in the buffer */


/*
 * The pool has a good growth mechanism and can handle tokens.
 * Tradeoff: it is slower to copy the final token data into a
 * fixed array from pool pointers than from a buffer monolith.
 */
#define TPBUF_LOGINITSIZE 1000
/* initial token buffer capacity */
#define TPBUF_LOGGROW 1000
/* token buffer growth rate */

#define LOGRP_LEN 5
#if (SIZEOF_VOID_P == 8)
#define LOGRP_WID 41
#else
#define LOGRP_WID 63
#endif
/* retune rpwid if the size of tokens changes dramatically */
#define LOGRP_ELT_SIZE (sizeof(union LogRelTermUnion))
#define LOGRP_MORE_ELTS 5
/* Number of slots filled if more elements needed.
   So if the pool grows, it grows by LOGRP_MORE_ELTS*LOGRP_WID elements
   at a time. */
#define LOGRP_MORE_BARS 508
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown.
   One could also recall these every time there is a delete all types. */
void InitLogRelInstantiator(void) {
  if (g_logterm_pool != NULL || g_logterm_ptrs.buf != NULL) {
    Asc_Panic(2, NULL, "ERROR: InitLogRelInstantiator called twice.\n");
  }
  g_logterm_pool =
    pool_create_store(LOGRP_LEN, LOGRP_WID, LOGRP_ELT_SIZE, LOGRP_MORE_ELTS,
                      LOGRP_MORE_BARS);
  if (g_logterm_pool == NULL) {
    Asc_Panic(2, "InitLogRelInstantiator",
              "ERROR: InitLogRelInstantiator unable to allocate pool.\n");
  }
  g_logterm_ptrs.buf = (struct logrel_term **)
	asccalloc(TPBUF_LOGINITSIZE,sizeof(union LogRelTermUnion *));
  if (g_logterm_ptrs.buf == NULL) {
    Asc_Panic(2, "InitLogRelInstantiator",
              "ERROR: InitLogRelInstantiator unable to allocate memory.\n");
  }
  g_logterm_ptrs.len = 0;
  g_logterm_ptrs.cap = TPBUF_LOGINITSIZE;
  g_logterm_ptrs.termstackcap = 200;
  g_logterm_ptrs.termstack =
    (unsigned long *)ascmalloc((sizeof(unsigned long)*200));
  if (g_logterm_ptrs.termstack == NULL) {
    Asc_Panic(2, "InitLogRelInstantiator",
              "ERROR: InitLogRelInstantiator unable to allocate memory.\n");
  }
}

/* this function returns NULL when newcap is 0 or when
 * it is unable to allocate the space requested.
 */
static unsigned long *realloc_term_stack(unsigned long newcap){
  if (!newcap) {
    if (g_logterm_ptrs.termstackcap !=0) {
      ascfree(g_logterm_ptrs.termstack);
      g_logterm_ptrs.termstack = NULL;
      g_logterm_ptrs.termstackcap = 0;
    }
  } else { /* less than means currently ok */
    if (newcap >= g_logterm_ptrs.termstackcap) {
      unsigned long *newbuf;
      newbuf = (unsigned long *)
        ascrealloc(g_logterm_ptrs.termstack,(sizeof(unsigned long)*newcap));
      if (newbuf!=NULL) {
        g_logterm_ptrs.termstack = newbuf;
        g_logterm_ptrs.termstackcap = newcap;
      } else {
 	FPRINTF(ASCERR,"Insufficient memory in logical relation processor\n");
        return NULL;
      }
    }
  }
  return g_logterm_ptrs.termstack;
}

void DestroyLogRelInstantiator(void) {
  assert(g_logterm_ptrs.buf!=NULL);
  assert(g_logterm_pool!=NULL);
  ascfree(g_logterm_ptrs.buf);
  g_logterm_ptrs.buf = NULL;
  g_logterm_ptrs.cap = g_logterm_ptrs.len = (size_t)0;
  if (g_logterm_ptrs.termstackcap != 0) {
    ascfree(g_logterm_ptrs.termstack);
    g_logterm_ptrs.termstack = NULL;
    g_logterm_ptrs.termstackcap = 0;
  }
  pool_destroy_store(g_logterm_pool);
  g_logterm_pool = NULL;
}

void ReportLogRelInstantiator(FILE *f)
{
  assert(g_logterm_pool!=NULL);
  FPRINTF(f,"LogRelInstantiator ");
  pool_print_store(f,g_logterm_pool,0);
  FPRINTF(f,"LogRelInstantiator buffer capacity: %lu\n",
    (unsigned long)g_logterm_ptrs.cap);
}

/* The slower expansion process. */
static void ExpandLogTermBuf(struct logrel_term *t) {
  struct logrel_term **newbuf;
  newbuf = (struct logrel_term **)ascrealloc(g_logterm_ptrs.buf,
      (sizeof(struct logrel_term *)*(g_logterm_ptrs.cap+TPBUF_LOGGROW)));
  if (newbuf!=NULL) {
    g_logterm_ptrs.buf = newbuf;
    g_logterm_ptrs.cap += TPBUF_LOGGROW;
    g_logterm_ptrs.buf[g_logterm_ptrs.len] = t;
    g_logterm_ptrs.len++;
  } else {
    FPRINTF(ASCERR,
          "ERROR: LogicalRelation Instantiator unable to allocate memory.\n");
   /* we have ignored the term pointer, but somebody else still has it: pool */
  }
  return;
}

/* Appends term to buffer. if buffer full and can't expand, forgets term.*/
static void AppendLogTermBuf(struct logrel_term *t) {
  if (g_logterm_ptrs.len < g_logterm_ptrs.cap) {
    g_logterm_ptrs.buf[g_logterm_ptrs.len++] = t;
  } else {
    ExpandLogTermBuf(t);
  }
  return;
}


struct logrel_side_temp {
  unsigned long length;
  union LogRelTermUnion *side;
};


/* forward declaration */
static struct logrel_term
*InfixArr_MakeLogSide(CONST struct logrel_side_temp *, int *);


/*
 * returns 1 if converting buf is successful
 * returns 0 if buf empty or insufficient memory.
 * The structure tmp given is filled with an array of terms
 * and its length. You must free the array if you decide you
 * don't want it. We don't care how the structure is initialized.
 */
static int ConvertLogTermBuf(struct logrel_side_temp *tmp)
{
  union LogRelTermUnion *arr = NULL;
  unsigned long len,c;

  realloc_term_stack(0);
  len = g_logterm_ptrs.len;
  if (len < 1) return 0;
  arr = (union LogRelTermUnion *)
	ascmalloc(len*sizeof(union LogRelTermUnion));
  if (arr==NULL) {
    FPRINTF(ASCERR,"Create Logical Relation: Insufficient memory :-(.\n");
    return 0;
  }
  for (c=0; c<len; c++) {
    arr[c] = *(LOGUNION_TERM(g_logterm_ptrs.buf[c]));
  }
  tmp->side = arr;
  tmp->length = len;
  return 1;
}

/* usually we want to reset both simultaneously. reset our
   pooling and buffering data. */
static
void DestroyLogTermList(void) {
  POOL_LOGRESET;
  TPBUF_LOGRESET;
}


/* create a term from the pool */
static struct logrel_term *CreateLogOpTerm(enum Expr_enum t)
{
  struct logrel_term *term;
  term = POOL_ALLOCLOGTERM;
  assert(term!=NULL);
  term->t = t;
  if (t==e_not) {
    LOGU_TERM(term)->left = NULL;
  } else {
    LOGB_TERM(term)->left = NULL;
    LOGB_TERM(term)->right = NULL;
  }
  return term;
}

/* create a term from the pool */
static struct logrel_term *CreateBoolVarTerm(CONST struct Instance *i)
{
  struct logrel_term *term;
  unsigned long pos;
  if ((pos = gl_search(g_logrelation_bvar_list,i,(CmpFunc)CmpP))){
    /* find boolean var if already on logical relations var list */
    term = POOL_ALLOCLOGTERM;
    assert(term!=NULL);
    term->t = e_var;
    LOGBV_TERM(term) -> varnum = pos;
  }
  else{
    /* or add it to the var list */
    gl_append_ptr(g_logrelation_bvar_list,(VOIDPTR)i);
    term = POOL_ALLOCLOGTERM;
    term->t = e_var;
    LOGBV_TERM(term) -> varnum = gl_length(g_logrelation_bvar_list);
  }
  return term;
}


static struct logrel_term *CreateBooleanTerm( int val )
{
  struct logrel_term *term;
  term = POOL_ALLOCLOGTERM;
  assert(term!=NULL);
  term->t = e_boolean;
  LOGBC_TERM(term) -> bvalue = val;
  return term;
}

static struct logrel_term *CreateLogIntegerTerm( int val )
{
  struct logrel_term *term;
  term = POOL_ALLOCLOGTERM;
  assert(term!=NULL);
  term->t = e_int;
  LOGI_TERM(term) -> ivalue = val;
  return term;
}

/* create a term from the pool */
static struct logrel_term *CreateSatisfiedTerm(CONST struct Name *n,
                                               struct Instance *inst,
                                               double value,
                                               CONST dim_type *dimensions)
{
  struct logrel_term *term;
  unsigned long pos;
  if ((pos = gl_search(g_logrelation_satrel_list,inst,(CmpFunc)CmpP))){
    /* find log/relation if already on logical relations satrel list */
    term = POOL_ALLOCLOGTERM;
    assert(term!=NULL);
    term->t = e_satisfied;
    LOGS_TERM(term) ->ncond = n;
    LOGS_TERM(term) ->relnum = pos;
    LOGS_TERM(term) ->rtol = value;
    LOGS_TERM(term) ->dim = dimensions;
  }
  else{
    /* or add it to the satrel list */
    gl_append_ptr(g_logrelation_satrel_list,(VOIDPTR)inst);
    term = POOL_ALLOCLOGTERM;
    term->t = e_satisfied;
    LOGS_TERM(term) ->ncond = n;
    LOGS_TERM(term) ->relnum = gl_length(g_logrelation_satrel_list);
    LOGS_TERM(term) ->rtol = value;
    LOGS_TERM(term) ->dim = dimensions;
  }
  return term;
}


/*
 * This function create and *must* create the memory
 * for the structure and for the union that the stucuture
 * points to. Too much code depends on the pre-existent
 * of a properly initialized union.
 */
struct logrelation *CreateLogRelStructure(enum Expr_enum t)
{
  struct logrelation *result;
  result = (struct logrelation *)ascmalloc(sizeof(struct logrelation));
  assert(result!=NULL);
  memset((char *)&(result->token),0,sizeof(struct TokenLogRel));
  result->logresidual = 0;
  result->logiscond = 0;
  result->lognominal = 1;
  result->bvars = NULL;
  result->satrels = NULL;
  result->ref_count = 0;
  result->relop = t;
  return result;
}


/**************************************************************************\
  Logical Relation processing and general logical relation check routines.
\**************************************************************************/

/*
 *  Here we give up if boolean vars are not well defined.
 *  At present e_var acceptable ARE:
 *  BOOLEAN_ATOM_INSTANCE
 *  Well defined boolean constants.
 *  Everything else is trash.
 *  CreateLogTermFromInst() and CheckLogExpr() must have matching semantics.
 */
static
struct logrel_term *CreateLogTermFromInst(struct Instance *inst,
					  struct Instance *lrel,
					  enum logrelation_errors *err)
{
  struct logrel_term *term;
  switch(InstanceKind(inst)){
  case BOOLEAN_ATOM_INST:
    term = CreateBoolVarTerm(inst);
    AddLogRel(inst,lrel);
    return term;
  case BOOLEAN_CONSTANT_INST:
    if ( AtomAssigned(inst) ){
      term = CreateBooleanTerm(GetBooleanAtomValue(inst));
      return term;
    }
    else{
      *err = boolean_value_undefined;
      return NULL;
    }
  case BOOLEAN_INST:
    *err = incorrect_boolean_linst_type;
    return NULL;
  case INTEGER_CONSTANT_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_INST:
    *err = incorrect_integer_linst_type;
    return NULL;
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
  case SYMBOL_INST:
    *err = incorrect_symbol_linst_type;
    return NULL;
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case REAL_INST:
    *err = incorrect_real_linst_type;
    return NULL;
  default:
    *err = incorrect_linst_type;
    return NULL;
  }
}




/* nonrecursive, but may call recursive things. returns 1 if ok. 0 if not
 * On a return of 1, newside->arr will be filled and should be deallocated
 * if the user does not want it. a return of 0 means that newside data is
 * invalid.
 * This is the ONLY function that should call DestroyLogTermList.
 */
static int ConvertLogExpr(CONST struct Expr *start,
			      CONST struct Expr *stop,
			      struct Instance *ref,
			      struct Instance *lrel,
			      enum logrelation_errors *err,
			      enum find_errors *ferr,
			      struct logrel_side_temp *newside)
{
  struct gl_list_t *instances;
  struct logrel_term *term;
  struct Instance *inst;
  CONST struct relation *rel;
  CONST struct logrelation *logrel;
  enum Expr_enum type;
  int result;
  symchar *str;
  struct for_var_t *fvp;
  if (newside==NULL) {
    Asc_Panic(2, NULL, "newside == NULL");
  }
  while(start!=stop){
    switch(ExprType(start)){
    case e_and:
    case e_or:
    case e_not:
      term = CreateLogOpTerm(ExprType(start));
      AppendLogTermBuf(term);
      break;
    case e_var:
      if (GetEvaluationForTable()!= NULL &&
	  (str = SimpleNameIdPtr(ExprName(start)))&&
	  (fvp=FindForVar(GetEvaluationForTable(),str))){
	if (GetForKind(fvp)==f_integer){
	  term = CreateLogIntegerTerm(GetForInteger(fvp));
	  AppendLogTermBuf(term);
	}
	else{
	  *err = incorrect_linst_type;
	  DestroyLogTermList();
	  return 0;
	}
      }
      else{
	instances = FindInstances(ref,ExprName(start),ferr);
	if (instances!=NULL){
	  if (gl_length(instances)==1){
	    inst = (struct Instance *)gl_fetch(instances,1);
	    gl_destroy(instances);
	    if ((term = CreateLogTermFromInst(inst,lrel,err))!=NULL){
	      AppendLogTermBuf(term);
	    }
	    else{
	      DestroyLogTermList();
	      return 0;
	    }
	  }
	  else{
	    *err=incorrect_logstructure;
	    gl_destroy(instances);
	    DestroyLogTermList();
	    return 0;
	  }
	}
	else{
	  *err = find_logerror;
	  DestroyLogTermList();
	  return 0;
	}
      }
      break;
    case e_boolean:
      term = CreateBooleanTerm(ExprBValue(start));
      AppendLogTermBuf(term);
      break;
    case e_satisfied:
      instances = FindInstances(ref,SatisfiedExprName(start),ferr);
      if (instances == NULL){
	*err = find_logerror;
        gl_destroy(instances);
	DestroyLogTermList();
        return 0;
      }
      else{
        if (gl_length(instances)==1) {
          inst = (struct Instance *)gl_fetch(instances,1);
          gl_destroy(instances);
          switch(InstanceKind(inst)){
            case REL_INST:
              rel = GetInstanceRelation(inst,&type);
              if (!RelationIsCond(rel)){
  	        *err = incorrect_linst_type;
	        DestroyLogTermList();
                return 0;
              }
    	      break;
            case LREL_INST:
              logrel = GetInstanceLogRel(inst);
              if (!LogRelIsCond(logrel)){
  	        *err = incorrect_linst_type;
	        DestroyLogTermList();
                return 0;
              }
    	      break;
            default:
	     *err = incorrect_linst_type;
	     DestroyLogTermList();
             return 0;
          }
        }
        else {
	  gl_destroy(instances);
  	  *err=incorrect_logstructure;
          return 0;
        }
      }
      term = CreateSatisfiedTerm(SatisfiedExprName(start),
                                 inst,
                                 SatisfiedExprRValue(start),
                                 SatisfiedExprRDimensions(start));
      AddLogRel(inst,lrel);
      AppendLogTermBuf(term);
      break;
    default:
      *err = incorrect_logstructure;
      DestroyLogTermList();
      return 0;
    }
    start = NextExpr(start);
  }
  result = ConvertLogTermBuf(newside);
  DestroyLogTermList();
  return result;
  /* we do not check result here. that is the callers job */
}

static
CONST struct Expr *FindLogRHS(CONST struct Expr *ex)
{
  CONST struct Expr *rhs,*previous=NULL;
  unsigned depth=0;
  while(ex!=NULL){
    switch(ExprType(ex)){
    case e_zero:
    case e_var:
    case e_int:
    case e_real:
    case e_boolean:
    case e_set:
    case e_symbol:
    case e_card:
    case e_choice:
    case e_sum:
    case e_prod:
    case e_union:
    case e_satisfied:
      if ((++depth)==1) rhs = ex;
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
      if ((--depth)==1) rhs = ex;
      break;
    case e_equal:
    case e_notequal:
    case e_boolean_eq:
    case e_boolean_neq:
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
      if (NextExpr(ex)==NULL) {
	return NextExpr(rhs);
      } else {
	return NULL;
      }
    case e_func:
    case e_uminus:
    case e_not:
      if (depth==1) {
        rhs = ex;
      }
      break;
    case e_st:
      Asc_Panic(2, NULL, "Such that expressions are not allowed.\n");
      break;
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
    previous = ex;
    ex = NextExpr(ex);
  }
  return NULL;
}

/*********************************************************************\
  This code is to support the conversion from postfix to infix.
\*********************************************************************/

static void DoBreakPoint(void)
{
  FPRINTF(ASCERR,"Something screwy with Infix_MakeLogSide\n");
  return;
}

#define PopLogTermStack(stack) \
   ((struct logrel_term *)gs_stack_pop((stack)))
#define PushLogTermStack(stack,term) \
   (gs_stack_push((stack),(char*)(term)))

#if 0 /* defunct code. should be deleted? */
struct logrel_term *Infix_MakeLogSide(struct gl_list_t *term_list)
{
  struct logrel_term *term = NULL;
  struct logrel_term *left;
  unsigned long len,count=1;
  struct gs_stack_t *stack;
  enum Expr_enum t;

  len = gl_length(term_list);
  stack = gs_stack_create(len);
  while(count <= len) {
    term = (struct logrel_term *)gl_fetch(term_list,count);
    switch(t = LogRelTermType(term)) {
    case e_boolean:
    case e_var:
      gs_stack_push(stack,(char *)term);
      break;
    case e_satisfied:
      gs_stack_push(stack,(char *)term);
      break;
    case e_not:
      left = LOGA_TERM(gs_stack_pop(stack));
      LOGU_TERM(term)-> left = left;
      gs_stack_push(stack,(char *)term);
      break;
    case e_and:
    case e_or:
      LOGB_TERM(term) -> right = LOGA_TERM(gs_stack_pop(stack));
      LOGB_TERM(term) -> left = LOGA_TERM(gs_stack_pop(stack));
      gs_stack_push(stack,(char *)term);
      break;
    default:
      Asc_Panic(2, NULL,
                "Dont know this type of logical relation term in MakeInfix\n");
      break;
    }
    count++;
  }
  term = LOGA_TERM(gs_stack_pop(stack));
  if (!gs_stack_empty(stack)) /* ensure that the stack is empty */
    DoBreakPoint();
  gs_stack_destroy(stack,0);
  return term;
}
#endif

/*
 * *err = 0 if ok, 1 otherwise. Sets up infix pointers.
 */
static struct logrel_term
*InfixArr_MakeLogSide(CONST struct logrel_side_temp *tmp, int *err)
{
  struct logrel_term *term = NULL;
  struct logrel_term *left;
  long len,count=0;
  struct gs_stack_t *stack;
  enum Expr_enum t;

  *err = 0;
  len = tmp->length;
  stack = gs_stack_create(len);
  while(count < len) {
    term = LOGA_TERM(&(tmp->side[count])); /* aka tmp->side+count */
    switch(t = LogRelTermType(term)) {
    case e_boolean:
    case e_var:
      gs_stack_push(stack,(char *)term);
      break;
    case e_satisfied:
      gs_stack_push(stack,(char *)term);
      break;
    case e_not:
      left = LOGA_TERM(gs_stack_pop(stack));
      LOGU_TERM(term)->left = left;
      gs_stack_push(stack,(char *)term);
      break;
    case e_and:
    case e_or:
      LOGB_TERM(term)->right = LOGA_TERM(gs_stack_pop(stack));
      LOGB_TERM(term)->left = LOGA_TERM(gs_stack_pop(stack));
      gs_stack_push(stack,(char *)term);
      break;
    default:
      Asc_Panic(2, NULL,
                "Dont know this type of logical relation term in MakeInfix\n");
      break;
    }
    count++;
  }
  term = LOGA_TERM(gs_stack_pop(stack));
  if (!gs_stack_empty(stack)) {
    /* ensure that the stack is empty */
    FPRINTF(ASCERR,"stacksize %ld\n",stack->size);
    DoBreakPoint();
    *err = 1;
  }
  gs_stack_destroy(stack,0);
  return term;
}

void DoInOrderLogRelVisit(struct logrel_term *term,
		    struct logrelation *r,
		    void (*func)(struct logrel_term *,
				 struct logrelation *))
{
  if (term) {
    switch(LogRelTermType(term)) {
    case e_boolean:
    case e_var:
      (*func)(term,r);
      break;
    case e_satisfied:
      (*func)(term,r);
      break;
    case e_not:
      DoInOrderLogRelVisit(LOGU_TERM(term)->left,r,func);
      (*func)(term,r);
      break;
    case e_and:
    case e_or:
      DoInOrderLogRelVisit(LOGB_TERM(term)->left,r,func);
      (*func)(term,r);
      DoInOrderLogRelVisit(LOGB_TERM(term)->right,r,func);
      break;
    default:
      return;
    }
  }
}

#if 0
/* This is a recursive deallocation of a term tree.
   It presupposes all terms are independently allocated,
   which at present is true nowhere in the compiler.
   It's a nice little function, though so we'll keep it in case,
   but not compile it in the meantime.
   Token of logrelations term lists are not independently allocated.
*/
void DestroyLogTermTree(struct logrel_term *term)
{
  if (term) {
    switch(term->t) {
    case e_and:
    case e_or:
      DestroyLogTermTree(LOGB_TERM(term)->left);
      DestroyLogTermTree(LOGB_TERM(term)->right);
      ascfree((char *)term);
      term = NULL;
      break;
    case e_satisfied:
      ascfree((char *)term);
      term = NULL;
      break;
    case e_not:
      DestroyLogTermTree(LOGU_TERM(term)->left);
      break;
    case e_boolean:
    case e_var:
      ascfree((char *)term);
      term = NULL;
      break;
    default:
      FPRINTF(ASCERR,"DestroyLogTermTree called with unexpected term type\n");
      break;
    }
  }
}
#endif


/*********************************************************************\
  Relation Processing for Instantiation.
\*********************************************************************/
static void DestroyLogTermSide(struct logrel_side_temp *);
void DestroyBVarList(struct gl_list_t *, struct Instance *);
void DestroySatRelList(struct gl_list_t *, struct Instance *);

struct logrelation *CreateLogicalRelation(struct Instance *reference,
				          struct Instance *lrelinst,
				          CONST struct Expr *ex,
				          enum logrelation_errors *err,
				          enum find_errors *ferr)
{
  struct logrelation *result;
  CONST struct Expr *rhs_ex,*last_ex;
  int lhs,rhs;
  enum Expr_enum type;
  struct logrel_side_temp leftside,rightside;
  assert(reference&&lrelinst&&ex&&err&&ferr);
  g_logrelation_bvar_list = gl_create(20l);
  g_logrelation_satrel_list = gl_create(2l);
  *err = lokay;
  *ferr = correct_instance;
  last_ex = FindLastExpr(ex);
  switch(ExprType(last_ex)){
  case e_boolean_eq:
  case e_boolean_neq:
    type = ExprType(last_ex);
    rhs_ex = FindLogRHS(ex);
    if (rhs_ex!=NULL){
      lhs = ConvertLogExpr(ex,rhs_ex,reference,lrelinst,err,ferr,&leftside);
      if(!lhs) {
        if (g_logrelation_bvar_list!=NULL) {
          DestroyBVarList(g_logrelation_bvar_list,lrelinst);
	}
	g_logrelation_bvar_list = NULL;
        if (g_logrelation_satrel_list!=NULL) {
          DestroySatRelList(g_logrelation_satrel_list,lrelinst);
	}
	g_logrelation_satrel_list = NULL;
	return NULL;
      }
      rhs = ConvertLogExpr(rhs_ex,last_ex,reference,lrelinst,err,
                           ferr,&rightside);
      if(!rhs) {
	DestroyLogTermSide(&leftside);
        if (g_logrelation_bvar_list!=NULL) {
          DestroyBVarList(g_logrelation_bvar_list,lrelinst);
	}
	g_logrelation_bvar_list = NULL;
        if (g_logrelation_satrel_list!=NULL) {
          DestroySatRelList(g_logrelation_satrel_list,lrelinst);
	}
	g_logrelation_satrel_list = NULL;
	return NULL;
      }
    }
    else{
      *err = incorrect_logstructure;
      FPRINTF(ASCERR,"Error finding logical relation operator.\n");
      if (g_logrelation_bvar_list!=NULL) {
         DestroyBVarList(g_logrelation_bvar_list,lrelinst);
      }
      g_logrelation_bvar_list = NULL;
      if (g_logrelation_satrel_list!=NULL) {
        DestroySatRelList(g_logrelation_satrel_list,lrelinst);
      }
      g_logrelation_satrel_list = NULL;
      return NULL;
    }
    break;
  default:
    *err = incorrect_logstructure;
    FPRINTF(ASCERR,"Error expression missing logical relation operator.\n");
    if (g_logrelation_bvar_list!=NULL) {
      DestroyBVarList(g_logrelation_bvar_list,lrelinst);
    }
    g_logrelation_bvar_list = NULL;
    if (g_logrelation_satrel_list!=NULL) {
      DestroySatRelList(g_logrelation_satrel_list,lrelinst);
    }
    g_logrelation_satrel_list = NULL;
    return NULL;
  }
  result = CreateLogRelStructure(type);
  result->ref_count = 1;
  if (lhs) {
    int status;
    result->token.lhs_len = leftside.length;
    result->token.lhs = leftside.side;
    result->token.lhs_term = InfixArr_MakeLogSide(&leftside,&status);
#ifndef NDEBUG
    if (status) {
      FPRINTF(ASCERR,"Anomaly in ");
      WriteInstanceName(ASCERR,lrelinst,NULL);
      FPRINTF(ASCERR," LHS.\n");
    }
#endif
  }
  if (rhs) { /* sometimes true */
    int status;
    result->token.rhs_len = rightside.length;
    result->token.rhs = rightside.side;
    result->token.rhs_term = InfixArr_MakeLogSide(&rightside,&status);
#ifndef NDEBUG
    if (status) {
      FPRINTF(ASCERR,"Anomaly in ");
      WriteInstanceName(ASCERR,lrelinst,NULL);
      FPRINTF(ASCERR," RHS.\n");
    }
#endif
  }
  result->bvars = g_logrelation_bvar_list;
  result->satrels = g_logrelation_satrel_list;
  g_logrelation_bvar_list = NULL;
  g_logrelation_satrel_list = NULL;
  return result;
}


/*
 **************************************************************************
 * Destroy Code.
 *
 * This takes care of destroying the parts of logical relations.
 * It ensures that any variables that are
 * incident upon the logical relations have their logrelation references
 * removed. This is done using the RemoveLogRel function.
 * It also ensures that all the relations or logrelations referenced
 * (from SATISFIED operator) have their logrelation references
 * removed, using the RemoveLogRel function too.
 **************************************************************************
 */

static void DestroyLogTermSide(struct logrel_side_temp *temp)
{
  if (temp!=NULL){
    if (temp->side !=NULL) ascfree(temp->side);
  }
  temp->side=NULL;
  temp->length=0L;
}

void DestroyBVarList(struct gl_list_t *l, struct Instance *inst)
{
  register struct Instance *ptr;
  register unsigned long c;
  for(c=gl_length(l);c>=1;c--)
    if ((ptr = (struct Instance *)gl_fetch(l,c)))
      RemoveLogRel(ptr,inst);
  gl_destroy(l);
}

void DestroySatRelList(struct gl_list_t *l, struct Instance *inst)
{
  register struct Instance *ptr;
  register unsigned long c;
  for(c=gl_length(l);c>=1;c--)
    if ((ptr = (struct Instance *)gl_fetch(l,c)))
      RemoveLogRel(ptr,inst);
  gl_destroy(l);
}

void DestroyLogRelation(struct logrelation *lrel, struct Instance *lrelinst)
{
  if (!lrel) return;
  assert(lrel->ref_count);
  if (--(lrel->ref_count)==0) {
    if (lrel->token.lhs!=NULL) ascfree(lrel->token.lhs);
    if (lrel->token.rhs!=NULL) ascfree(lrel->token.rhs);
  }
  if (lrel->bvars) DestroyBVarList(lrel->bvars,lrelinst);
  if (lrel->satrels) DestroySatRelList(lrel->satrels,lrelinst);
  ascfree((char *)lrel);
}


/*
 **************************************************************************
 * Variable Maintenance.
 *
 * Logical Relations need to keep a *unique* list of variables incident
 * upon them. However variables move around and also disappear,
 * in particular when being ARE_THE_SAME'd. This code does that variable
 * maintenance.
 *
 * This requires some explanation. There are a number of cases
 * to consider.
 *
 * 1) the old instance does not exist in the bvar list -- do nothing.
 *
 * 2) the old instance exists, but the new does not -- store the
 *    the new instance in the slot where the old instance was and
 *    return.
 *
 * 3) the old instance exists, *and* the new instance also exists in
 *    the bvarlist. This can happen in the case when 2 variables
 *    incident upon a logical relation are going to be ATS'ed (not wise
 *    but possible.) We need to run down the entire list in the case
 *    of logical  relations. This is expensive and uses the
 *    DeleteAndChange function.
 *
 *  4) the new instance is NULL, which can happen transiently during some
 *     operations. This defaults to case 2).
 **************************************************************************
 */


static
void ChangeLogVarTermSide(union LogRelTermUnion *side,
		          unsigned long int len,
		          unsigned long int old,
		          unsigned long int new)
{
  register long c;
  register struct logrel_term *term;
  for(c=len-1;c>=0;c--){
    term = LOGA_TERM(&(side[c]));
    switch (term->t){
    case e_var:
      if (LOGBV_TERM(term)->varnum == old)
	LOGBV_TERM(term)->varnum = new;
      else
	if (LOGBV_TERM(term)->varnum > old) LOGBV_TERM(term)->varnum--;
      break;
    default:
      break;
    }
  }
}


static
void ChangeLogSatTermSide(union LogRelTermUnion *side,
		          unsigned long int len,
		          unsigned long int old,
		          unsigned long int new)
{
  register long c;
  register struct logrel_term *term;
  for(c=len-1;c>=0;c--){
    term = LOGA_TERM(&(side[c]));
    switch (term->t){
      case e_satisfied:
        if (LOGS_TERM(term)->relnum == old)
	  LOGS_TERM(term)->relnum = new;
        else
	  if (LOGS_TERM(term)->relnum > old) LOGS_TERM(term)->relnum--;
        break;
      default:
        break;
    }
  }
}


static
void LogDeleteAndChange(struct gl_list_t *relorvar, struct logrelation *lrel,
		        unsigned long int pos1, unsigned long int pos2,
                        int varflag)
{
  if (pos1 < pos2) Swap(&pos1,&pos2);
  /* pos1 > pos2 now */
  gl_delete(relorvar,pos1,0);
  if (varflag) {
  if (lrel->token.rhs)
    ChangeLogVarTermSide(lrel->token.rhs,lrel->token.rhs_len,pos1,pos2);
  if (lrel->token.lhs)
    ChangeLogVarTermSide(lrel->token.lhs,lrel->token.lhs_len,pos1,pos2);
  }
  else {
  if (lrel->token.rhs)
    ChangeLogSatTermSide(lrel->token.rhs,lrel->token.rhs_len,pos1,pos2);
  if (lrel->token.lhs)
    ChangeLogSatTermSide(lrel->token.lhs,lrel->token.lhs_len,pos1,pos2);
  }
}

void ModifyLogRelPointers(struct gl_list_t *relorvar,
			  struct logrelation *lrel,
			  CONST struct Instance *old,
			  CONST struct Instance *new)
{
  unsigned long pos,other;
  CONST struct Instance *inst;
  int varflag;
  assert(lrel!=NULL);

  if (old==new) return;

  if (new){
    if ((pos = gl_search(relorvar,old,(CmpFunc)CmpP))) {
      if ((other = gl_search(relorvar,new,(CmpFunc)CmpP))){
	gl_store(relorvar,pos,(VOIDPTR)new);     /* case 3 */
        if (new == NULL) {
          inst = old;
        }
        else {
          inst = new;
        }
        switch (inst->t) {
        case BOOLEAN_ATOM_INST:
          varflag = 1;
  	  LogDeleteAndChange(relorvar,lrel,pos,other,varflag);
          break;
        case REL_INST:
        case LREL_INST:
          varflag = 0;
  	  LogDeleteAndChange(relorvar,lrel,pos,other,varflag);
          break;
        default:
          Asc_Panic(2, NULL,
                    "Wrong instance type passed to ChangeLogRelPointers\n");
          break;
	}
      }
      else
	gl_store(relorvar,pos,(char *)new);	/* case 2 */
    }
    else{					/* case 1 */
      FPRINTF(ASCERR,"Warning ModifiyLogRelPointers not found.\n");
      FPRINTF(ASCERR,"This shouldn't effect your usage at all.\n");
    }
  }
  else						/* case 4 */
    if ((pos = gl_search(relorvar,old,(CmpFunc)CmpP)))
      gl_store(relorvar,pos,(VOIDPTR)new);
}


/* forward declaration for recursing Check functions of logical relation */
static int
CheckLogExpr(CONST struct Instance *ref, CONST struct Expr *start,
  CONST struct Expr *stop, int list);


/**********************************************************************\
  Here we check that vars are well defined, a precondition to FOR
  statements being executed.
  If lists of vars are acceptable (don't know why they would be)
  list should be passed in as 1, otherwise 0.
  At present e_var acceptable ARE:
  BOOLEAN_ATOM_INSTANCE
  Well defined Boolean constants.
  CreateLogTermFromInst() and CheckLogExpr() must have matching semantics.

 Returns:  1 --> OK,
	   0  --> BAD (undefined/unassigned) try again later
	  -1  --> incurably BAD
\**********************************************************************/
static int CheckExprBVar(CONST struct Instance *ref, CONST struct Name *name,
			 int list)
{
  struct gl_list_t *instances;
  symchar *str;
  struct Instance *inst;
  struct for_var_t *fvp;
  enum find_errors err;
  if((str = SimpleNameIdPtr(name))){
    if (TempExists(str))
      if (ValueKind(TempValue(str))==integer_value)
	return 1;
      else
	return -1;
    if (GetEvaluationForTable() != NULL &&
        (fvp=FindForVar(GetEvaluationForTable(),str)))
      if (GetForKind(fvp)==f_integer)
	return 1;
      else
	return -1;
  }
  instances = FindInstances(ref,name,&err); /* need noisy version of Find */
  if (instances == NULL){
    switch(err){
    case unmade_instance:
    case undefined_instance:
      return 0;
    default:
      return -1;
    }
  }
  else{
    if (gl_length(instances)==1) {
      inst = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      switch(InstanceKind(inst)){
      case BOOLEAN_ATOM_INST:
	return 1;
      case DUMMY_INST:
	return 1;
      case BOOLEAN_CONSTANT_INST:
	if (AtomAssigned(inst)) {
	  return 1;
        }
	return 0;
      default: return -1; /* bogus var type found */
      }
    }
    else if (list){
/* this part of the code is most peculiar, and semantics may not
   match it. We need to find out what is the semantics of list. */
      unsigned long c,len;
      len = gl_length(instances);
      for(c=1;c<=len;c++){
	inst = (struct Instance *)gl_fetch(instances,1);
	switch(InstanceKind(inst)){
	case BOOLEAN_ATOM_INST:
	  break;
        case DUMMY_INST:
	  break;
	case BOOLEAN_CONSTANT_INST:
	  if (!AtomAssigned(inst)){
	    gl_destroy(instances);
	    return -1;
	  }
	default:
	  gl_destroy(instances);
	  return 0;
	}
      }
      gl_destroy(instances);
      return 1;
    }
    else {
      gl_destroy(instances);
      return -1;
    }
  }
}


static int CheckExprSatisfied(CONST struct Instance *ref,
                              CONST struct Name *name)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  enum find_errors err;

  instances = FindInstances(ref,name,&err);
  if (instances == NULL){
    gl_destroy(instances);
    FPRINTF(ASCERR,
    "Name of an unmade instance (Relation) in Satisfied Expr \n");
    return 0;
  }
  else{
    if (gl_length(instances)==1) {
      inst = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      switch(InstanceKind(inst)){
        case REL_INST:
	  return 1;
        case LREL_INST:
	  return 1;
        case DUMMY_INST:
	  return 1;
        default:
         FPRINTF(ASCERR,
                 "Incorrect instance name (No Log/Relation)"
                 " inside a Satisfied Expr\n");
          return 0;
      }
    }
    else {
      gl_destroy(instances);
      FPRINTF(ASCERR,
      "Error in Satisfied Expr Name assigned to more than one instance\n");
      return 0;
    }
  }
}


/**********************************************************************\
 CheckLogExpr(ref, start, stop, list)
 struct Instance *ref; context of the logical relation instance, ie
 parent. int list; boolean whether list of instances are acceptable
 struct Expr *start, *stop; pointers to the portion of relation this
 checks.
\**********************************************************************/
static int CheckLogExpr(CONST struct Instance *ref,
			CONST struct Expr *start,
			CONST struct Expr *stop,
			int list)
{
  while (start!=stop){
    switch(ExprType(start)){
    case e_not:
    case e_and:
    case e_or:
    case e_boolean:
      break;			/* automatically okay! */
    case e_satisfied:
      if(!CheckExprSatisfied(ref,SatisfiedExprName(start))) {
         return 0 ;
      }
      break;
    case e_var:
      switch(CheckExprBVar(ref,ExprName(start),list)){
      case 0:
      case -1:
        return 0;
      }
      break;
    default:
      return 0;
    }
    start = NextExpr(start);
  }
  return 1;
}

/* see header. returns 1 if relation expression is fully instantiable
 ie all vars exist, and, if need be, properly initialized. */
int CheckLogRel(CONST struct Instance *reference, CONST struct Expr *ex)
{
  CONST struct Expr *last_ex,*rhs_ex;
  last_ex = FindLastExpr(ex);
  switch(ExprType(last_ex)){
  case e_boolean_eq:
  case e_boolean_neq:
    rhs_ex = FindLogRHS(ex);
    if (!CheckLogExpr(reference,rhs_ex,last_ex,0)) return 0;
    return CheckLogExpr(reference,ex,rhs_ex,0);
  default:
    return 0;
  }
}


/*
 * We can now just do a memcopy and the infix pointers
 * all adjust by the difference between the token
 * arrays that the gl_lists are hiding. Cool, eh?
 * Note, if any turkey ever tries to delete an individual
 * token from these gl_lists AND deallocate it,
 * they will get a severe headache.
 *
 * This is a full blown copy and not copy by reference.
 * You do not need to remake the infix pointers after
 * calling this function.
 */
static union LogRelTermUnion
*CopyLogRelSide(union LogRelTermUnion *old, unsigned long len)
{
  struct logrel_term *term;
  union LogRelTermUnion *arr;
  unsigned long c;
  long int delta;

  if (!old || !len) return NULL;
  arr = (union LogRelTermUnion *)
	ascmalloc(len*sizeof(union LogRelTermUnion));
  if (arr==NULL) {
    FPRINTF(ASCERR,"Copy Logical Relation: Insufficient memory :-(.\n");
    return NULL;
  }
  memcpy( (VOIDPTR)arr, (VOIDPTR)old, len*sizeof(union LogRelTermUnion));
 /*
  *  Difference in chars between old and arr ptrs. It should me a multiple
  *  of sizeof(double) but may not be a multiple of sizeof(union LRTU).
  *  Delta may easily be negative.
  *  Normally, though arr > old.
  */
  delta = (char *)arr - (char *)old;
#ifdef ADJLOGPTR
#undef ADJLOGPTR
#endif
#define ADJLOGPTR(p) ( (p) = LOGA_TERM((char *)(p)+delta) )
  for (c=0;c<len;c++) {
    term = LOGA_TERM(&(arr[c]));
    switch (term->t) {
    /* unary terms */
    case e_not:
      ADJLOGPTR(LOGU_TERM(term)->left);
      break;
    /* binary terms */
    case e_and:
    case e_or:
      ADJLOGPTR(LOGB_TERM(term)->left);
      ADJLOGPTR(LOGB_TERM(term)->right);
      break;
    case e_boolean:
    case e_var:			/* the var number will be correct */
      break;
    case e_satisfied:
      break;
    /* don't know how to deal with the following relation operators.
       they may be binary or unary, but InfixArr_MakeLogSide never set them. */
    case e_boolean_eq: case e_boolean_neq:
    default:
      Asc_Panic(2, NULL, "Unknown term type in CopyLogRelSide\n");
      break;
    }
  }
#undef ADJLOGPTR

  return arr;
}


/*
 * This function will always create a new instance list, from
 * the instance list provided. The instances will be copied
 * and made aware of the destination logrelation instance.
 */
static
struct gl_list_t *CopyLogRelInstList(struct Instance *dest_inst,
				     struct gl_list_t *instlist)
{
  struct Instance *inst;
  struct gl_list_t *newinstlist = NULL;
  unsigned long len,c,pos;

  if (instlist) {
    len = gl_length(instlist);
    newinstlist = gl_create(len);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(instlist,c);
      pos = gl_search(newinstlist,inst,(CmpFunc)CmpP);
      if (pos) {
	Asc_Panic(2, NULL, "Corrupted instance list in CopyLogRelation\n");
      }
      gl_append_ptr(newinstlist,(VOIDPTR)inst);
      AddLogRel(inst,dest_inst);
    }
  }
  else{	/* we will always return a newinstlist, even if empty */
    newinstlist = gl_create(1L);
  }
  return newinstlist;
}


static
struct logrelation *CopyLogRelation(CONST struct Instance *src_inst,
				    struct Instance *dest_inst,
				    struct gl_list_t *varlist,
				    struct gl_list_t *rellist)
{
  CONST struct logrelation *src;
  struct logrelation *result;
  long int delta;

  src = GetInstanceLogRel(src_inst);
  if (!src) return NULL;

  result = CreateLogRelStructure(e_bol_token);

  result->ref_count = 1;

  result->token.lhs =
    CopyLogRelSide(src->token.lhs,src->token.lhs_len);
  if(result->token.lhs != NULL) {
    delta = LOGUNION_TERM(src->token.lhs_term) - src->token.lhs;
    result->token.lhs_term = LOGA_TERM(result->token.lhs+delta);
    result->token.lhs_len = src->token.lhs_len;
  } else {
    result->token.lhs_term = NULL;
    result->token.lhs_len = 0;
  }

  result->token.rhs =
    CopyLogRelSide(src->token.rhs,src->token.rhs_len);
  if(result->token.rhs != NULL) {
    delta = LOGUNION_TERM(src->token.rhs_term) - src->token.rhs;
    result->token.rhs_term = LOGA_TERM(result->token.rhs+delta);
    result->token.rhs_len = src->token.rhs_len;
  } else {
    result->token.rhs_term = NULL;
    result->token.rhs_len = 0;
  }

  result->bvars = CopyLogRelInstList(dest_inst,varlist);
  result->satrels = CopyLogRelInstList(dest_inst,rellist);
  result->logresidual = src->logresidual;
  result->lognominal = src->lognominal;
  return result;
}


struct logrelation *CopyLogRelByReference(CONST struct Instance *src_inst,
					  struct Instance *dest_inst,
					  struct gl_list_t *varlist,
				          struct gl_list_t *rellist)
{
  struct logrelation *src;
  struct logrelation *result;
  unsigned size;

  src = (struct logrelation *)GetInstanceLogRel(src_inst);
  if (!src) return NULL;

  result = CreateLogRelStructure(e_bol_token);
  size = sizeof(struct logrelation);
  ascbcopy(src,result,sizeof(struct logrelation));
    /* copy everything.  */
  /*
   * We now have a verbatim copy. We now need to patch the public
   * stuff.
   */
  result->bvars = CopyLogRelInstList(dest_inst,varlist);
  result->satrels = CopyLogRelInstList(dest_inst,rellist);
  (src->ref_count)++;
  return result;
}

struct logrelation *CopyLogRelToModify(CONST struct Instance *src_inst,
				       struct Instance *dest_inst,
				       struct gl_list_t *varlist,
				       struct gl_list_t *rellist)
{
  CONST struct logrelation *src;
  struct logrelation *result;
  src = GetInstanceLogRel(src_inst);
  if (!src) return NULL;
  result = CopyLogRelation(src_inst,dest_inst,varlist,rellist);
  return result;
}


