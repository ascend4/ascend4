/*
 *  Relation Output Routines
 *  by Tom Epperly
 *  Created: 2/8/90
 *  Version: $Revision: 1.16 $
 *  Version control file: $RCSfile: relation_io.c,v $
 *  Date last modified: $Date: 1998/04/10 23:25:47 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1998 Benjamin Allan and Vicente Rico-Ramirez
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
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/pool.h>
#include <general/dstring.h>
#include <general/pretty.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "func.h"
#include "expr_types.h"
#include "extcall.h"
#include "instance_enum.h"
#include "extfunc.h"
#include "mathinst.h"
#include "visitinst.h"
#include "parentchild.h"
#include "instquery.h"
#include "tmpnum.h"
#include "find.h"
#include "relation_type.h"
#include "extfunc.h"
#include "rel_blackbox.h"
#include "extfunc.h"
#include "rel_blackbox.h"
#include "vlist.h"
#include "relation.h"
#include "relation_util.h"
#include "relation_io.h"
#include "instance_io.h"
#include "nameio.h"

#ifndef lint
static CONST char RelationOutputRoutinesRCS[]="$Id: relation_io.c,v 1.16 1998/04/10 23:25:47 ballan Exp $";
#endif


static char g_shortbuf[256];
#define SB255 g_shortbuf

#ifdef ASC_NO_POOL
#define RELIO_USES_POOL FALSE
#else
#define RELIO_USES_POOL TRUE
#endif

/*
 * stack entry for relation io conversion to infix.
 */
struct rel_stack {
  struct rel_stack *next;
  unsigned long pos;
  int first;
  unsigned long lhs; /* used only for binary tokens with first = 2 */
};

/* default for other tokens. this will cause an array bounds read
 * error if misused, which is nice if you assume purify.
 */
/* 
 *  THIS LOOKS LIKE A BUG.
 *  rel_stack.lhs is unsigned long, so assigning -1 appears wrong
 *  If the intent is to use the result of (unsigned long)-1, then
 *  that should at least be commented.  (JDS 12/11/2005)
 */
#define NOLHS -1

/* worklist */
static struct rel_stack *g_rel_stack=NULL;

/* recycle spaces */
static pool_store_t g_rel_stack_pool = NULL;
/* global for our memory manager */
/* aim for 4096 chunks including malloc overhead */
#define RSP_LEN 2
#if (SIZEOF_VOID_P == 8)
#define RSP_WID 10
#else
#define RSP_WID 20
#endif
/* retune rpwid if the size of struct name changes */
#define RSP_ELT_SIZE (sizeof(struct rel_stack))
#define RSP_MORE_ELTS 1
/*
 *  Number of slots filled if more elements needed.
 *  So if the pool grows, it grows by RSP_MORE_ELTS*RSP_WID elements at a time.
 */
#define RSP_MORE_BARS 5
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown. */
void RelationIO_init_pool(void) {
  if (g_rel_stack_pool != NULL ) {
    ASC_PANIC("ERROR: RelationIO_init_pool called twice.\n");
  }
  g_rel_stack_pool = pool_create_store(RSP_LEN, RSP_WID, RSP_ELT_SIZE,
    RSP_MORE_ELTS, RSP_MORE_BARS);
  if (g_rel_stack_pool == NULL) {
    Asc_Panic(2, NULL,
      "ERROR: RelationIO_init_pool unable to allocate pool.\n");
  }
}

void RelationIO_destroy_pool(void) {
  if (g_rel_stack_pool==NULL) return;
  pool_clear_store(g_rel_stack_pool);
  pool_destroy_store(g_rel_stack_pool);
  g_rel_stack_pool = NULL;
}

void RelationIO_report_pool(void)
{
  if (g_rel_stack_pool==NULL) {
    FPRINTF(ASCERR,"rel_stacksPool is empty\n");
  }
  FPRINTF(ASCERR,"rel_stacksPool ");
  pool_print_store(ASCERR,g_rel_stack_pool,0);
}

#if RELIO_USES_POOL
#define RSPMALLOC ((struct rel_stack *)(pool_get_element(g_rel_stack_pool)))
/* get a token. Token is the size of the struct struct rel_stack */
#define RSPFREE(p) (pool_free_element(g_rel_stack_pool,((void *)p)))
/* return a struct rel_stack */

#else

#define RSPMALLOC ascmalloc(sizeof(struct rel_stack ))
#define RSPFREE(p) ascfree(p)

#endif /* RELIO_USES_POOL */

static
void WriteOp(FILE *f, enum Expr_enum t)
{
  switch(t){
  case e_plus: PUTC('+',f); break;
  case e_uminus:
  case e_minus: PUTC('-',f); break;
  case e_times: PUTC('*',f); break;
  case e_divide: PUTC('/',f); break;
  case e_power: PUTC('^',f); break;
  case e_ipower: PUTC('^',f); break;
  case e_equal: PUTC('=',f); break;
  case e_notequal: FPRINTF(f,"<>"); break;
  case e_less: PUTC('<',f); break;
  case e_greater: PUTC('>',f); break;
  case e_lesseq: FPRINTF(f,"<="); break;
  case e_greatereq: FPRINTF(f,">="); break;
  case e_maximize: FPRINTF(f,"MAXIMIZE"); break;
  case e_minimize: FPRINTF(f,"MINIMIZE"); break;
  default:
    ASC_PANIC("Unknown term in WriteOp.\n");
    break;
  }
}

/* appends operator to dynamic symbol */
static void WriteOpDS(Asc_DString *dsPtr, enum Expr_enum t,
                      enum rel_lang_format lang)
{
  (void)lang; /* future use for relational operators in other langs. */
  assert(dsPtr!=NULL);
  switch(t){
  case e_plus:
    Asc_DStringAppend(dsPtr,"+",1);
    break;
  case e_uminus:
  case e_minus:
    Asc_DStringAppend(dsPtr,"-",1);
    break;
  case e_times:
    Asc_DStringAppend(dsPtr,"*",1);
    break;
  case e_divide:
    Asc_DStringAppend(dsPtr,"/",1);
    break;
  case e_power:
    Asc_DStringAppend(dsPtr,"^",1);
    break;
  case e_ipower:
    Asc_DStringAppend(dsPtr,"^",1);
    break;
  case e_equal:
    Asc_DStringAppend(dsPtr,"=",1);
    break;
  case e_notequal:
    Asc_DStringAppend(dsPtr,"<>",2);
    break;
  case e_less:
    Asc_DStringAppend(dsPtr,"<",1);
    break;
  case e_greater:
    Asc_DStringAppend(dsPtr,">",1);
    break;
  case e_lesseq:
    Asc_DStringAppend(dsPtr,"<=",2);
    break;
  case e_greatereq:
    Asc_DStringAppend(dsPtr,">=",2);
    break;
  case e_maximize:
    Asc_DStringAppend(dsPtr,"MAXIMIZE",8);
    break;
  case e_minimize:
    Asc_DStringAppend(dsPtr,"MINIMIZE",8);
    break;
  default:
    FPRINTF(ASCERR,"Unknown term in WriteOpDS.\n");
    Asc_DStringAppend(dsPtr,"ERROR",5);
    break;
  }
}

char *RelationVarXName(CONST struct relation *r,
                       unsigned long varnum, 
                       struct RXNameData *rd)
{
  unsigned long i;
  struct RXNameData myrd = {"x",NULL,""};
  (void)r;

  if (varnum < 1) {
    return NULL;
  }
  if (rd == NULL) {
    rd = &myrd;
  } else {
    assert(rd->prefix!=NULL);
    assert(rd->suffix!=NULL);
  }
  if (rd->indices == NULL) {
    i = varnum;
  } else {
    i = (unsigned long)(rd->indices[varnum]);
  }
  sprintf(SB255,"%.110s%lu%.110s",rd->prefix,i,rd->suffix);
  return SB255;
}

static
void WriteTerm(FILE *f,
	       CONST struct relation *rel,
	       CONST struct relation_term *term,
	       CONST struct Instance *inst)
{
  struct Instance *cur_var;
  switch(RelationTermType(term)){
  case e_var:
    cur_var = RelationVariable(rel,TermVarNumber(term));
    WriteInstanceName(f,cur_var,inst);
    break;
  case e_func:
    FPRINTF(f,FuncName(TermFunc(term)));
    break;
  case e_int:
    FPRINTF(f,"%ld",TermInteger(term));
    break;
  case e_zero:
    FPRINTF(f,"%g",0.0);
    break;
  case e_real:
    FPRINTF(f,"%g",TermReal(term));
    break;
  case e_plus:
  case e_minus:
  case e_times:
  case e_divide:
  case e_power:
  case e_ipower:
    WriteOp(f,RelationTermType(term));
    break;
  case e_uminus:
    FPRINTF(f,"NEG");
    break;
  default:
    ASC_PANIC("Unknown term type in WriteTerm.\n");
    break;
  }
}

/* write out a postfix term */
static
void WriteTermDS(Asc_DString *dsPtr,
                 CONST struct relation *rel,
                 CONST struct relation_term *term,
                 CONST struct Instance *inst)
{
  struct Instance *cur_var;
  switch(RelationTermType(term)){
  case e_var:
    cur_var = RelationVariable(rel,TermVarNumber(term));
    WriteInstanceNameDS(dsPtr,cur_var,inst);
    break;
  case e_func:
    Asc_DStringAppend(dsPtr,FuncName(TermFunc(term)),-1);
    break;
  case e_int:
    sprintf(SB255,"%ld",TermInteger(term));
    Asc_DStringAppend(dsPtr,SB255,-1);
    break;
  case e_zero:
    Asc_DStringAppend(dsPtr,"0.0",3);
    break;
  case e_real:
    sprintf(SB255,"(double) %.18g",TermReal(term));
    Asc_DStringAppend(dsPtr,SB255,-1);
    break;
  case e_plus:
  case e_minus:
  case e_times:
  case e_divide:
  case e_power:
  case e_ipower:
    WriteOpDS(dsPtr,RelationTermType(term),relio_ascend);
    break;
  case e_uminus:
    Asc_DStringAppend(dsPtr,"NEG",3);
    break; /* what is this anyway? */
  default:
    FPRINTF(ASCERR,"Unknown term in WriteTermDS.\n");
    Asc_DStringAppend(dsPtr,"ERROR",5);
    break;
  }
}

static
void WriteSidePF(FILE *f,
		 CONST struct relation *r,
		 int side,
		 CONST struct Instance *ref)
{
  unsigned c,len;
  CONST struct relation_term *term;
  len = RelationLength(r,side);
  for(c=1;c<=len;c++){
    term = RelationTerm(r,c,side);
    WriteTerm(f,r,term,ref);
    if(c<len) PUTC(' ',f);
  }
}

static
void WriteSidePFDS(Asc_DString *dsPtr,
		 CONST struct relation *r,
		 int side,
		 CONST struct Instance *ref)
{
  unsigned c,len;
  CONST struct relation_term *term;
  len = RelationLength(r,side);
  for(c=1;c<=len;c++){
    term = RelationTerm(r,c,side);
    WriteTermDS(dsPtr,r,term,ref);
    if(c<len) Asc_DStringAppend(dsPtr," ",1);
  }
}

void WriteRelationPostfix(FILE *f,
			  CONST struct Instance *relinst,
			  CONST struct Instance *ref)
{
  CONST struct relation *r;
  enum Expr_enum type;

  r = GetInstanceRelation(relinst,&type);
  if (type!=e_token) {
    FPRINTF(ASCERR,"Invalid relation type in WriteRelationPostfix\n");
    return;
  }

  switch(RelationRelop(r)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    WriteSidePF(f,r,1,ref);
    PUTC(' ',f);
    WriteSidePF(f,r,0,ref);
    PUTC(' ',f);
    WriteOp(f,RelationRelop(r));
    break;
  case e_maximize:
  case e_minimize:
    WriteOp(f,RelationRelop(r));
    PUTC(' ',f);
    WriteSidePF(f,r,1,ref);
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in WriteRelationPostfix\n");
    break;
  }
}

char *WriteRelationPostfixString( CONST struct Instance *relinst,
			         CONST struct Instance *ref)
{
  CONST struct relation *r;
  enum Expr_enum type;
  char *result;
  static Asc_DString ds;
  Asc_DString *dsPtr;

  r = GetInstanceRelation(relinst,&type);
  if (type!=e_token) {
    FPRINTF(ASCERR,"Invalid relation type in WriteRelationPostfixString\n");
    result=ASC_NEW_ARRAY(char,60);
    if (result==NULL) return result;
    sprintf(result,"WriteRelationPostfixString called on non-token relation.");
    return result;
  }
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);

  switch(RelationRelop(r)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    WriteSidePFDS(dsPtr,r,1,ref);
    Asc_DStringAppend(dsPtr," ",1);
    WriteSidePFDS(dsPtr,r,0,ref);
    Asc_DStringAppend(dsPtr," ",1);
    WriteOpDS(dsPtr,RelationRelop(r),relio_ascend);
    break;
  case e_maximize:
  case e_minimize:
    WriteOpDS(dsPtr,RelationRelop(r),relio_ascend);
    Asc_DStringAppend(dsPtr," ",1);
    WriteSidePFDS(dsPtr,r,1,ref);
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in WriteRelationPostfix\n");
    break;
  }
  result = Asc_DStringResult(dsPtr);
  Asc_DStringFree(dsPtr);
  return result;
}

static
void ClearRelationStack(void)
{
  struct rel_stack *next;
  while(g_rel_stack!=NULL){
    next = g_rel_stack->next;
    RSPFREE(g_rel_stack);
    g_rel_stack = next;
  }
}

static
void PushRelation(unsigned long int pos, int first, unsigned long int lhs)
{
  struct rel_stack *next;
  next = g_rel_stack;
  g_rel_stack = RSPMALLOC; 
  g_rel_stack->next = next;
  g_rel_stack->pos = pos;
  g_rel_stack->first = first;
  g_rel_stack->lhs = lhs;
}

static
int NotEmptyStack(void)
{
  return g_rel_stack!=NULL;
}

static
int FirstTop(void)
{
  assert(g_rel_stack!=NULL);
  return g_rel_stack->first;
}

static
int LHSTop(void)
{
  assert(g_rel_stack!=NULL);
  return g_rel_stack->lhs;
}

/*
 * destroy top of stack and returns its pos value.
 */
static
unsigned long PopRelation(void)
{
  struct rel_stack *next;
  unsigned long result;
  assert(g_rel_stack!=NULL);
  next = g_rel_stack->next;
  result = g_rel_stack->pos;
  RSPFREE((char *)g_rel_stack);
  g_rel_stack = next;
  return result;
}

/* Assumes relation array of the form [lhs tokens, rhstokens,postoken].
 * Searches left starting from postoken given until it finds the pos
 * preceding the ",". This pos is the top of the lhs tokens tree,
 * if you think about things in tree terms. postoken is the top of
 * the tree and is binary. lhs and rhs refer to left and right
 * side of any binary operator.
 * Calling this with an ill formed tree is likely to cause weird
 * errors and overrun the array or underrun the result.
 */
static
unsigned long LeftHandSide(CONST struct relation *r,
			   unsigned long int pos,
			   int side)
{
  unsigned long depth=1;
  CONST struct relation_term *term;
  pos--;
  while(depth){
    term = RelationTerm(r,pos,side);
    switch(RelationTermType(term)){
    case e_int:
    case e_zero:
    case e_real:
    case e_var:
      depth--;
      break;
    case e_func:
    case e_uminus:
      break;
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      depth++;
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type.\n"
                "(%d) in function LeftHandSide\n",
                RelationTermType(term));
      break;
    }
    pos--;
  }
  return pos;
}

/* a little function to tell us about precedence */
static
unsigned Priority(enum Expr_enum t)
{
  switch(t){
  case e_zero:
  case e_var:
  case e_int:
  case e_real:
  case e_func:
    return 0;

  case e_plus:
  case e_minus:
    return 1;

  case e_times:
  case e_divide:
    return 2;

  case e_uminus:
    return 3;

  case e_power:
  case e_ipower:
    return 4;

  default:
    return 0; /* 0=good idea? */
  }
  /*NOTREACHED*/
}

int NeedParen(enum Expr_enum parent_op, enum Expr_enum child_op, int rhs)
{
  unsigned parent_p,child_p;
  switch(child_op){
  case e_zero:
  case e_var:
  case e_int:
  case e_real:
  case e_func:
    return 0;
  default:
    parent_p = Priority(parent_op);
    child_p = Priority(child_op);
    if (parent_p > child_p) return 1;
    if (parent_p < child_p) return 0;
    if ((parent_op == e_minus)&&rhs) return 1;
    if ((parent_op == e_divide)&&rhs) return 1;
    return 0;
  }
}

static
void WriteSide(FILE *f,
	       CONST struct relation *r,
	       int side,
	       CONST struct Instance *ref)
{
  unsigned long pos,lhs,oldlhs;
  int first;
  enum Expr_enum t;
  CONST struct relation_term *term;
  struct Instance *cur_var;
  ClearRelationStack();
  PushRelation(RelationLength(r,side),1,NOLHS);
  while(NotEmptyStack()){
    first = FirstTop();		/* check if this is the first visit */
    oldlhs = LHSTop();		/* check for cached LHS position on binaries */
    pos = PopRelation();	/* check the top */
    term = RelationTerm(r,pos,side);
    switch(t = RelationTermType(term)){
    case e_var:
      cur_var = RelationVariable(r,TermVarNumber(term));
      WriteInstanceName(f,cur_var,ref);
      break;
    case e_int:
      FPRINTF(f,"%ld",TermInteger(term));
      break;
    case e_zero:
      FPRINTF(f,"0");
      break;
    case e_real:
      FPRINTF(f,"%g",TermReal(term));
      break;
    case e_func:
      if(first) {
	FPRINTF(f,"%s(",FuncName(TermFunc(term)));
	PushRelation(pos,0,NOLHS);
	PushRelation(pos-1,1,NOLHS);
      }
      else{
	PUTC(')',f);
      }
      break;
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      switch(first){
      case 1:
	lhs = LeftHandSide(r,pos,side);
	term = RelationTerm(r,lhs,side);
	if (NeedParen(t,RelationTermType(term),0)) {
	  PUTC('(',f);
        }
	PushRelation(pos,2,lhs);
	PushRelation(lhs,1,NOLHS);
	break;
      case 2:
	term = RelationTerm(r,oldlhs,side);
	if (NeedParen(t,RelationTermType(term),0)) {
	  PUTC(')',f);
        }
	PUTC(' ',f);
	WriteOp(f,t);
	PUTC(' ',f);
	PushRelation(pos,0,NOLHS);
	term = RelationTerm(r,pos-1,side);
	if (NeedParen(t,RelationTermType(term),1))
	  PUTC('(',f);
	PushRelation(pos-1,1,oldlhs);
	break;
      case 0:
	term = RelationTerm(r,pos-1,side);
	if (NeedParen(t,RelationTermType(term),1))
	  PUTC(')',f);
	break;
      }
      break;
    case e_uminus:
      term = RelationTerm(r,pos-1,side);
      if (first){
	PUTC('-',f);
	PushRelation(pos,0,NOLHS);
	switch(RelationTermType(term)){
	case e_power:
	case e_ipower:
	case e_zero:
	case e_int:
	case e_real:
	case e_var:
	case e_func:
	case e_uminus:
	  break;
	default:
	  PUTC('(',f);
	  break;
	}
	PushRelation(pos-1,1,NOLHS);
      }
      else{
	switch(RelationTermType(term)){
	case e_power:
	case e_ipower:
	case e_int:
	case e_zero:
	case e_real:
	case e_var:
	case e_func:
	case e_uminus:
	  break;
	default:
	  PUTC(')',f);
	  break;
	}
      }
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type: function WriteSide\n");
      break;
    }
  }
}

/**
	write a side in infix notation
*/
static
void WriteSideDS(Asc_DString *dsPtr, CONST struct relation *r, int side,
                 CONST struct Instance *ref, WRSNameFunc func, void *userdata,
                 enum rel_lang_format lang)
{
  unsigned long pos,lhs, oldlhs, rlen;
  int first;
  enum Expr_enum t;
  CONST struct relation_term *term;
  struct Instance *cur_var;
  char *username;
  

  ClearRelationStack();
  rlen = RelationLength(r,side);
  PushRelation(rlen,1,NOLHS);
  while(NotEmptyStack()){
    first = FirstTop();		/* check if this is the first visit */
    oldlhs = LHSTop();		/* check if this is the first visit */
    pos = PopRelation();	/* check the top */
    term = RelationTerm(r,pos,side);
    t = RelationTermType(term);
    switch (t) {
    case e_var:
      if (func == NULL) {
        cur_var = RelationVariable(r,TermVarNumber(term));
        WriteInstanceNameDS(dsPtr,cur_var,ref);
      } else {
        username = (*func)(r,TermVarNumber(term),userdata);
        Asc_DStringAppend(dsPtr,username,-1);
      }
      break;
    case e_int:
      if (lang == relio_C) {
        if (pos == rlen /* don't check next term if at END */ ||
            RelationTermType(RelationTerm(r,pos+1,side)) != e_ipower) { 
          /* don't want to provoke integer division, normally */
          sprintf(SB255,"%ld.0",TermInteger(term));
        } else {
          /* ipow(expr,i) is the only legal way of seeing ipow )
           * which in stack terms is [expr... , int , e_ipower]
           */
          sprintf(SB255,"%ld",TermInteger(term));
        }
      } else {
        sprintf(SB255,"%ld",TermInteger(term));
      }
      Asc_DStringAppend(dsPtr,SB255,-1);
      break;
    case e_zero:
      if (lang == relio_C) {
        Asc_DStringAppend(dsPtr,"0.0",3);
        /* don't want to provoke integer division */
      } else {
        Asc_DStringAppend(dsPtr,"0",1);
      }
      break;
    case e_real:
      if (lang == relio_C) {
        sprintf(SB255,"(double) %.18g",TermReal(term));
      } else {
        sprintf(SB255,"%g",TermReal(term));
      }
      Asc_DStringAppend(dsPtr,SB255,-1);
      break;
    case e_func:
      if(first) {
        if (lang == relio_C) {
          sprintf(SB255,"%s(",FuncCName(TermFunc(term)));
        } else {
          sprintf(SB255,"%s(",FuncName(TermFunc(term)));
        }
        Asc_DStringAppend(dsPtr,SB255,-1);
        PushRelation(pos,0,NOLHS);
        PushRelation(pos-1,1,NOLHS);
      }
      else{
        Asc_DStringAppend(dsPtr,")",1);
      }
      break;
    case e_power:
    case e_ipower:
      if (lang == relio_C) {
        /* we assume the args to pow Always need () around them
         * to keep , from confusing anything, so lhs not used.
         */
        switch(first) {
        case 1:
          /* seeing this binary token the first time */
          if (t==e_power) {
            Asc_DStringAppend(dsPtr,"pow((",5);
          } else {
            Asc_DStringAppend(dsPtr,"asc_ipow((",10);
          }
          PushRelation(pos,2,NOLHS);
          lhs = LeftHandSide(r,pos,side);
          PushRelation(lhs,1,NOLHS);
	  break;
        case 2:
          /* seeing this binary token the second time */
          if (t==e_power) {
            Asc_DStringAppend(dsPtr,") , (",5);
          } else {
            Asc_DStringAppend(dsPtr,") , ",4);
          }
          PushRelation(pos,0,NOLHS);
          PushRelation(pos-1,1,NOLHS);
          break;
        case 0: 
          /* seeing binary token the third (last) time */
          if (t==e_power) {
            Asc_DStringAppend(dsPtr," ))",3);
          } else {
            Asc_DStringAppend(dsPtr,")",1);
          }
          break;
        default: /* first */
          ASC_PANIC("Don't know this type of stack first");
          break;
        }
        break;
      }
      /* else FALL THROUGH to usual binary operator treatment */
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
      switch(first){
      case 1:
        /* seeing this binary token the first time */
        lhs = LeftHandSide(r,pos,side);
        term = RelationTerm(r,lhs,side);
        if (NeedParen(t,RelationTermType(term),0)) {
          Asc_DStringAppend(dsPtr,"(",1);
        }
        PushRelation(pos,2,lhs);
        PushRelation(lhs,1,NOLHS);
        break;
      case 2:
        /* seeing this binary token the second time */
        term = RelationTerm(r,oldlhs,side);
        if (NeedParen(t,RelationTermType(term),0)) {
          Asc_DStringAppend(dsPtr,")",1);
        }
        Asc_DStringAppend(dsPtr," ",1);
        WriteOpDS(dsPtr,t,lang);
        Asc_DStringAppend(dsPtr," ",1);
        PushRelation(pos,0,NOLHS);
        term = RelationTerm(r,pos-1,side);
        if (NeedParen(t,RelationTermType(term),1)) {
          Asc_DStringAppend(dsPtr,"(",1);
        }
        PushRelation(pos-1,1,NOLHS);
        break;
      case 0: 
        /* seeing binary token the third (last) time */
        term = RelationTerm(r,pos-1,side);
        if (NeedParen(t,RelationTermType(term),1))
          Asc_DStringAppend(dsPtr,")",1);
        break;
      }
      break;
    case e_uminus:
      term = RelationTerm(r,pos-1,side);
      if (first){
        Asc_DStringAppend(dsPtr,"-",1);
        PushRelation(pos,0,NOLHS);
        switch(RelationTermType(term)){
        case e_power:
        case e_ipower:
        case e_zero:
        case e_int:
        case e_real:
        case e_var:
        case e_func:
        case e_uminus:
          break;
        default:
          Asc_DStringAppend(dsPtr,"(",1);
          break;
        }
        PushRelation(pos-1,1,NOLHS);
      }
      else{
        switch(RelationTermType(term)){
        case e_power:
        case e_ipower:
        case e_int:
        case e_zero:
        case e_real:
        case e_var:
        case e_func:
        case e_uminus:
          break;
        default:
          Asc_DStringAppend(dsPtr,")",1);
          break;
        }
      }
      break;
    default:
      ASC_PANIC("Don't know this type of relation token");
      break;
    }
  }
}

static
void WriteTokenRelation(FILE *f,
			CONST struct relation *r,
			CONST struct Instance *ref)
{
  switch(RelationRelop(r)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    WriteSide(f,r,1,ref);
    PUTC(' ',f);
    WriteOp(f,RelationRelop(r));
    PUTC(' ',f);
    WriteSide(f,r,0,ref);
    break;
  case e_maximize:
  case e_minimize:
    WriteOp(f,RelationRelop(r));
    PUTC(' ',f);
     WriteSide(f,r,1,ref);
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in WriteTokenRelation\n");
    break;
  }
}

static
void WriteTokenRelationDS(Asc_DString *dsPtr,
			  CONST struct relation *r,
			  CONST struct Instance *ref,
                          WRSNameFunc func, void *userdata,
                          enum rel_lang_format lang)
{
  switch(RelationRelop(r)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    if (lang == relio_C) {
      Asc_DStringAppend(dsPtr,"( ",2);
    }
    WriteSideDS(dsPtr,r,1,ref,func,userdata,lang);
    switch (lang) {
    case relio_ascend:
      Asc_DStringAppend(dsPtr," ",1);
      WriteOpDS(dsPtr,RelationRelop(r),lang);
      Asc_DStringAppend(dsPtr," ",1);
      break;
    case relio_C:
      Asc_DStringAppend(dsPtr," ) - ( ",7);
      break;
    default:
      ASC_PANIC("Unknown lang in WriteRelationString.\n");
      exit(2); /* NOTREACHED ,  shuts up gcc */
    }
    WriteSideDS(dsPtr,r,0,ref,func,userdata,lang);
    if (lang == relio_C) {
      Asc_DStringAppend(dsPtr," )",2);
    }
    break;
  case e_maximize:
  case e_minimize:
    /* max/min are lhs-only expression, so no operator in C case. */
    switch (lang) {
    case relio_ascend:
      WriteOpDS(dsPtr,RelationRelop(r),lang);
      Asc_DStringAppend(dsPtr," ",1);
      WriteSideDS(dsPtr,r,1,ref,func,userdata,lang);
      break;
    case relio_C:
      Asc_DStringAppend(dsPtr,"( ",2);
      WriteSideDS(dsPtr,r,1,ref,func,userdata,lang);
      Asc_DStringAppend(dsPtr," )",2);
      break;
    default:
      ASC_PANIC("Unknown lang in WriteRelationString.\n");
      exit(2); /* NOTREACHED ,  shuts up gcc */
    }
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in WriteTokenRelationDS\n");
    Asc_DStringAppend(dsPtr,"BADRELOPR ",9);
    break;
  }
}

static
void Infix_WriteSide(FILE *f,
		     CONST struct relation *r,
		     struct relation_term *term,
		     CONST struct Instance *ref)
{
  enum Expr_enum t;
  struct Instance *cur_var;
  int parens;

  switch(t = RelationTermType(term)) {
  case e_var:
    cur_var = RelationVariable(r,TermVarNumber(term));
    WriteInstanceName(f,cur_var,ref);
    break;
  case e_int:
    FPRINTF(f,"%ld",TermInteger(term));
    break;
  case e_zero:
    FPRINTF(f,"0");
    break;
  case e_real:
    FPRINTF(f,"%g",TermReal(term));
    break;
  case e_func:
    FPRINTF(f,"%s(",FuncName(TermFunc(term)));
    Infix_WriteSide(f,r,TermFuncLeft(term),ref);
    FPRINTF(f,")");
    break;
  case e_uminus:
    FPRINTF(f,"-(");
    Infix_WriteSide(f,r,TermUniLeft(term),ref);
    FPRINTF(f,")");
    break;
  case e_plus:
  case e_minus:
  case e_times:
  case e_divide:
  case e_power:
  case e_ipower:
    parens = NeedParen(RelationTermType(term),
		       RelationTermType(TermBinLeft(term)),0);
    if (parens) PUTC('(',f);
    Infix_WriteSide(f,r,TermBinLeft(term),ref);
    if (parens) PUTC(')',f);
    PUTC(' ',f);
    WriteOp(f,t);
    PUTC(' ',f);
    parens = NeedParen(RelationTermType(term),
		       RelationTermType(TermBinRight(term)),1);
    if (parens) PUTC('(',f);
    Infix_WriteSide(f,r,TermBinRight(term),ref);
    if (parens) PUTC('(',f);
    break;
  default:
    FPRINTF(f,"***");
    break;
  }
}

void Infix_WriteRelation(FILE *f,
			 CONST struct Instance *relinst,
			 CONST struct Instance *ref)
{
  CONST struct relation *r;
  enum Expr_enum type;

  r = GetInstanceRelation(relinst,&type);
  if (type!=e_token) {
    FPRINTF(ASCERR,"Invalid relation type in Infix_WriteRelation\n");
    return;
  }
  switch(RelationRelop(r)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    Infix_WriteSide(f,r,Infix_LhsSide(r),ref);
    PUTC(' ',f);
    WriteOp(f,RelationRelop(r));
    PUTC(' ',f);
    Infix_WriteSide(f,r,Infix_RhsSide(r),ref);
    break;
  case e_maximize:
  case e_minimize:
    WriteOp(f,RelationRelop(r));
    PUTC(' ',f);
    Infix_WriteSide(f,r,Infix_LhsSide(r),ref);
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in Infix_WriteRelation\n");
  }
}


/*
 * KAA.
 * Just a dumb little note about writing out variable lists --
 * We need to check for the penultimate variable so that we
 * can know if we must write : "var, " or simply "var)".
 * One of doing this without having an if within the main loop
 * is to run from 1 to len-1, and then writing the last variable.
 * By running from 1 to len, if the length of the list is 0, we
 * automatically skip the loop. But by running from 1 to len-1,
 * we skip the loop, but have to then check that len was in fact
 * greater than 0, as gl_fetch(list,0) is not a pretty sight. Overall
 * we save len-1 ifs!! yaaaa!
 *
 * BAA
 * yah, that's it, optimize code way way way off the critical path
 * instead of doing things right. idiot.
 */
static
void WriteGlassBoxRelation(FILE *f,
			   CONST struct relation *r,
			   CONST struct Instance *ref)
{
  struct ExternalFunc *efunc;
  CONST char *name;
  struct Instance *var;
  CONST struct gl_list_t *list;
  unsigned long len,c;

  efunc = RelationGlassBoxExtFunc(r);
  name = ExternalFuncName(efunc);
  FPRINTF(f,"%s(",name);
  if (NULL != (list = RelationVarList(r))) {
    len = gl_length(list);
    for (c=1;c<len;c++) {			/* the < is intentional */
      var = (struct Instance *)gl_fetch(list,c);
      WriteInstanceName(f,var,ref);
      FPRINTF(f,", ");
    }
    if (len) {					/* check length */
      var = (struct Instance *)gl_fetch(list,len);
      WriteInstanceName(f,var,ref);
      FPRINTF(f," ; %d)",GlassBoxRelIndex(r));
    }
  }
  else{
    FPRINTF(f," )");
  }
  WriteOp(f,RelationRelop(r));
}

static void WriteGlassBoxRelationDS(Asc_DString *dsPtr,
			   CONST struct relation *r,
			   CONST struct Instance *ref)
{
  struct ExternalFunc *efunc;
  CONST char *name;
  struct Instance *var;
  CONST struct gl_list_t *list;
  unsigned long len,c;

  efunc = RelationGlassBoxExtFunc(r);
  name = ExternalFuncName(efunc);
  Asc_DStringAppend(dsPtr,name,-1);
  Asc_DStringAppend(dsPtr,"(",1);
  if (NULL != (list = RelationVarList(r))) {
    len = gl_length(list);
    for (c=1;c<len;c++) {			/* the < is intentional */
      var = (struct Instance *)gl_fetch(list,c);
      WriteInstanceNameDS(dsPtr,var,ref);
      Asc_DStringAppend(dsPtr,", ",2);
    }
    if (len) {					/* check length */
      var = (struct Instance *)gl_fetch(list,len);
      WriteInstanceNameDS(dsPtr,var,ref);
      sprintf(SB255," ; %d)",GlassBoxRelIndex(r));
      Asc_DStringAppend(dsPtr,SB255,-1);
    }
  }
  else{
    Asc_DStringAppend(dsPtr," )",2);
  }
  WriteOpDS(dsPtr,RelationRelop(r),relio_ascend);
}

/**
	Output a blackbox relation to the specified file pointer.
*/
static
void WriteBlackBoxRelation(FILE *f,
			   CONST struct relation *r,
			   CONST struct Instance *inst)
{
  struct ExternalFunc *efunc;
  struct gl_list_t *arglist;
  struct gl_list_t *branch;
  CONST struct Name *arg;
  unsigned long len1,c1,len2,c2, ninput;

  (void) inst;
  efunc = RelationBlackBoxExtFunc(r);
  ninput = NumberInputArgs(efunc);
  arglist = RelationBlackBoxArgNames(r);
  len1 = gl_length(arglist);
  FPRINTF(f,"%s(",ExternalFuncName(efunc)); /* function name */

  if (len1) {
    FPRINTF(f,"\n\t");
    for (c1=1;c1<=len1;c1++) {
      branch = (struct gl_list_t *)gl_fetch(arglist,c1);
      if (branch) {
        len2 = gl_length(branch);
		for (c2=1;c2<=len2;c2++) {
		  arg = (struct Name *)gl_fetch(branch,c2);
		  WriteName(f,arg);
		  if(c2<len2)FPRINTF(f,", ");
		}
      }
      if (c1<len1) {
		FPRINTF(f,"\n); %s\n",(c1 <= ninput ?"INPUT":"OUTPUT"));
      }
    }
  }

  arg = RelationBlackBoxDataName(r);
  if (arg != NULL) {
    WriteName(f,arg);
    FPRINTF(f,"); DATA\n"); /* sequencing correct? */
  }
  FPRINTF(f,"); OUTPUT\n");
}

static
void WriteBlackBoxRelationDS(Asc_DString *dsPtr,
			   CONST struct relation *r,
			   CONST struct Instance *inst)
{
  struct ExternalFunc *efunc;
  struct gl_list_t *arglist;
  struct gl_list_t *branch;
  CONST struct Name *arg;
  unsigned long len1,c1,len2,c2,ninput;

  (void)inst;
  efunc = RelationBlackBoxExtFunc(r);
  ninput = NumberInputArgs(efunc);
  arglist = RelationBlackBoxArgNames(r);
  len1 = gl_length(arglist);
  sprintf(SB255," %s(",ExternalFuncName(efunc)); /* function name */
  Asc_DStringAppend(dsPtr,SB255,-1);

  if (len1) {
    Asc_DStringAppend(dsPtr,"\n\t",2);
    for (c1=1;c1<=len1;c1++) {
      branch = (struct gl_list_t *)gl_fetch(arglist,c1);
      if (branch) {
		len2 = gl_length(branch);
		for (c2=1;c2<=len2;c2++) {
		  arg = (struct Name *)gl_fetch(branch,c2);
		  WriteName2Str(dsPtr,arg);
		  if(c2<len2)Asc_DStringAppend(dsPtr,", ",2);
		}
      }
      if(c1<len1) {
        if (c1 <= ninput) {
          Asc_DStringAppend(dsPtr," INPUT \n\t, ",11);
        } else {
          Asc_DStringAppend(dsPtr," OUTPUT \n\t, ",12);
        }
      }
    }
    Asc_DStringAppend(dsPtr," OUTPUT \n",9);
  }

  arg = RelationBlackBoxDataName(r);
  if (arg != NULL) {
    Asc_DStringAppend(dsPtr,"\t",1);
    WriteName2Str(dsPtr,arg);
    Asc_DStringAppend(dsPtr," DATA\n",6);
  }
  Asc_DStringAppend(dsPtr,");\n",3);
}


void WriteRelation(FILE *f, CONST struct Instance *relinst,
		   CONST struct Instance *ref)
{
  CONST struct relation *reln;
  enum Expr_enum reltype;

  reln = GetInstanceRelation(relinst,&reltype);
  if (!reln) {
    FPRINTF(f,"NULL relation\n");
    return;
  }
  switch (reltype) {
  case e_token:
    WriteTokenRelation(f,reln,ref);
    return;
  case e_blackbox:
    WriteBlackBoxRelation(f,reln,ref);
    return;
  case e_glassbox:
    WriteGlassBoxRelation(f,reln,ref);
    return;
  default:
    FPRINTF(ASCERR,"Unknown relation type in WriteRelation\n");
    return;
  }
}

char *WriteRelationString(CONST struct Instance *relinst,
                          CONST struct Instance *ref,
                          WRSNameFunc func, void *userdata,
                          enum rel_lang_format lang,
                          int *lenptr)
{
  CONST struct relation *reln;
  enum Expr_enum reltype;
  static Asc_DString ds;
  Asc_DString *dsPtr;
  char *result;

  reln = GetInstanceRelation(relinst,&reltype);
  if (!reln) {
    result = ASC_NEW_ARRAY(char,15);
    if (result == NULL) return result;
    sprintf(result,"NULL relation\n");
    if (lenptr != NULL) {
      *lenptr = 14;
    }
    return result;
  }

  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  switch (reltype) {
  case e_token:
    WriteTokenRelationDS(dsPtr,reln,ref,func,userdata,lang);
    break;
  case e_blackbox:
    WriteBlackBoxRelationDS(dsPtr,reln,ref);
    break;
  case e_glassbox:
    WriteGlassBoxRelationDS(dsPtr,reln,ref);
    break;
  default:
    FPRINTF(ASCERR,"Unknown relation type in WriteRelationString\n");
    if (lenptr != NULL) {
      *lenptr = -1;
    }
    return NULL;
  }
  if (lenptr != NULL) {
    *lenptr = Asc_DStringLength(dsPtr);
  }
  result = Asc_DStringResult(dsPtr);
  Asc_DStringFree(dsPtr);
  return result;
}


/*
 * some io to help in debugging relation manipulators.
 */
static FILE *g_writfp = NULL;
static void WriteIfRel(struct Instance *i)
{
  struct Instance *p;
  char *string;
  struct RXNameData crd = {"x[",NULL,"]"};
  if (!i) {
    FPRINTF(ASCERR,"null child pointer in WriteIfRel\n");
    return;
  }
  if (InstanceKind(i)==REL_INST) {
    FPRINTF(g_writfp,"\n");
    WriteInstanceName(g_writfp,i,NULL);
    FPRINTF(g_writfp,"\n");
    p = InstanceParent(i,1);
    while (InstanceKind(p)!= MODEL_INST && InstanceKind(p) != SIM_INST) {
      p = InstanceParent(p,1);
    }
    FPRINTF(g_writfp,"INFIX\n");
    WriteRelation(g_writfp,i,p);
    FPRINTF(g_writfp,"\n");
    FFLUSH(g_writfp);

    FPRINTF(g_writfp,"STRING-INFIX\n");
    string = WriteRelationString(i,p,NULL,NULL,relio_ascend,NULL);
    FPRINTF(g_writfp,"%s\n",(string==NULL)?"nullstring":string);
    if (string != NULL) {
      ascfree(string);
    }
    FFLUSH(g_writfp);

    FPRINTF(g_writfp,"STRING-X-ASCEND\n");
    string = WriteRelationString(i,p,(WRSNameFunc)RelationVarXName,
                                 NULL,relio_ascend,NULL);
    FPRINTF(g_writfp,"%s\n",(string==NULL)?"nullstring":string);
    if (string != NULL) {
      ascfree(string);
    }
    FFLUSH(g_writfp);

    FPRINTF(g_writfp,"STRING-C-pretty\n");
    string = WriteRelationString(i,p,(WRSNameFunc)RelationVarXName,
                                 (VOIDPTR)&crd,relio_C,NULL);
    print_long_string(g_writfp,string,60,8);
    FPRINTF(g_writfp,"STRING-C\n");
    FPRINTF(g_writfp,"%s\n",(string==NULL)?"nullstring":string);
    if (string != NULL) {
      ascfree(string);
    }
    FFLUSH(g_writfp);

    FPRINTF(g_writfp,"POSTFIX\n");
    WriteRelationPostfix(g_writfp,i,p);
    FPRINTF(g_writfp,"\n");
    FFLUSH(g_writfp);
  }
  return;
}

void WriteRelationsInTree(FILE *fp,struct Instance *i)
{
  if (i==NULL || fp==NULL) return;
  g_writfp = fp;
  SlowVisitInstanceTree(i,WriteIfRel,0,0);
}

void WriteRelationsInList(FILE *fp,struct gl_list_t *list)
{
  if (list==NULL || fp==NULL) {
    return;
  }
  g_writfp = fp;
  gl_iterate(list,(void (*)(VOIDPTR))WriteIfRel);
}


/*
 *********************************************************************
 * Save Relation Code
 *
 * The below code is concerned with saving relations in a persistent
 * format. It writes out the code in a condensed format to allow
 * restoration to the original representation. A number of conversion
 * formats are provided. In particular, a token or opcode relation
 * may be saved as a glassbox relation, with  the attendent loss of
 * information high level information.
 *
 * The grammar will follow later once it has stabilized.
 * In the mean time, the following keywords are used.
 * $KIND INTEGER - the reltype - e_token, e_opcode etc.
 * $RELOP INTEGER - the relational operator - e_maximize, e_equal etc.
 * $COUNT INTEGER - the number of variables.
 * $OPCODES INTEGER ':' (INTEGER)*
 * $VARIABLES ':' (INTEGER)*	- global varindex
 * $CONSTANTS ':' (REAL)*
 *********************************************************************
 */

#undef LHS
#undef RHS
#define LHS 0
#define RHS 1
#define BREAKLINES 65
static
void SaveTokenRelnSide(FILE *fp,CONST struct relation *r,
		       int side, struct gl_list_t *constants)
{
  CONST struct relation_term *term;
  enum Expr_enum t;
  unsigned c,len;
  int count;

  len = RelationLength(r,side);
  if (!len) return;

  count = 16;
  FPRINTF(fp,"\t$OPCODES %d : ",side);
  for (c=1;c<=len;c++) {
    term = RelationTerm(r,c,side);
    t = RelationTermType(term);
    count += FPRINTF(fp," %d ",(int)t);	/* the opcode is the enum */
    switch (t) {
    case e_var:
      count += FPRINTF(fp,"%lu",TermVarNumber(term));
      break;
    case e_func:
      count += FPRINTF(fp,"%d",(int)FuncId(TermFunc(term)));
      break;
    case e_int:
    case e_zero:
    case e_real:
      gl_append_ptr(constants,(VOIDPTR)term);
      count += FPRINTF(fp,"%lu",gl_length(constants));
      break;
    case e_uminus:	/* DOUBLE CHECK -- KAA_DEBUG */
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      break;
    default:
      count += FPRINTF(fp,"%d",(int)e_nop);
      break;
    }
    if (count >= BREAKLINES) {
      PUTC('\n',fp); PUTC('\t',fp);
      count = 8;
    }
  }
  FPRINTF(fp,";\n");		/* terminate */
}


/*
 * We write out everybody as a floating point number. The
 * opcode will tell whether to coerce the double to a long
 * or vice verca.
 */
static
void SaveTokenConstants(FILE *fp, struct gl_list_t *constants)
{
  CONST struct relation_term *term;
  unsigned long len,c;

  len = gl_length(constants);
  if (!len) return;

  FPRINTF(fp,"\t$CONSTANTS : ");
  for (c=1;c<=len;c++) {
    term = (CONST struct relation_term *)gl_fetch(constants,c);
    switch (RelationTermType(term)) {
    case e_zero:
      FPRINTF(fp,"%12.8e",0.0);
      break;
    case e_real:
      FPRINTF(fp,"%12.8e",TermReal(term));
      break;
    case e_int:
      FPRINTF(fp,"%12.8e",(double)TermInteger(term));
      break;
    default:
      ASC_PANIC("Illegal term in SaveTokenConstants\n");
      break;
    }
    PUTC(' ',fp);
  }
  FPRINTF(fp,";\n");		/* terminate */
}

/*
 *********************************************************************
 * SaveRelationVariables
 *
 * This should be good for all types of relations, and the
 * conversions between those relations.
 *********************************************************************
 */
void SaveRelationVariables(FILE *fp, CONST struct relation *r)
{
  struct Instance *var;
  unsigned long len,c;
  int count;
  len = NumberVariables(r);
  if (!len) return;

  count = 16;
  FPRINTF(fp,"\t$VARIABLES : ");
  for (c=1;c<=len;c++) {
    var = RelationVariable(r,c);
    count += FPRINTF(fp," %lu ",GetTmpNum(var));
    if (count >= BREAKLINES) {
      PUTC('\n',fp); PUTC('\t',fp);
      count = 8;
    }
  }
  FPRINTF(fp,";\n");		/* terminate */
}

/*
 *********************************************************************
 * SaveTokenRelation
 *
 * Save a token relation in condensed opcode format.
 * Saving of dimensionality information is not done at the moment.
 *********************************************************************
 */
void SaveTokenRelation(FILE *fp, CONST struct Instance *relinst)
{
  CONST struct relation *reln;
  enum Expr_enum type;
  struct gl_list_t *constants = NULL;

  reln = GetInstanceRelation(relinst,&type);
  if (type!=e_token) {
    FPRINTF(ASCERR,"Invalid relation type in SaveTokenRelation\n");
    return;
  }
  constants = gl_create(50L);		/* overkill, but ... */

  FPRINTF(fp,"$RELATION %lu {\n",GetTmpNum(relinst)); 	/* header */
  FPRINTF(fp,"\t$KIND %d, $RELOP %d, $COUNT %lu;\n",
	  (int)type, (int)RelationRelop(reln), NumberVariables(reln));
  /*
   * Now write out the optional stuff.
   */
  SaveTokenRelnSide(fp,reln,LHS,constants);
  SaveTokenRelnSide(fp,reln,RHS,constants);
  SaveTokenConstants(fp,constants);
  SaveRelationVariables(fp,reln);
  FPRINTF(fp,"}\n\n");					/* the trailer */

  gl_destroy(constants);
}

/*
 *********************************************************************
 * SaveGlassBoxRelation
 *
 *********************************************************************
 */
void SaveGlassBoxRelation(FILE *fp, CONST struct Instance *relinst)
{
  CONST struct relation *reln;
  enum Expr_enum type;
  CONST char *tmp;
  int len;

  reln = GetInstanceRelation(relinst,&type);
  len = GlassBoxNumberArgs(reln);
  if (len==0) return;
  if (NumberVariables(reln) != (unsigned long)len) {
    FPRINTF(ASCERR,"Corrupted arguements in glassbox relation\n");
    return;
  }

  FPRINTF(fp,"$RELATION %lu {\n",GetTmpNum(relinst)); 	/* header */
  FPRINTF(fp,"\t$KIND %d, $RELOP %d, $COUNT %d;\n",
	  (int)type, (int)RelationRelop(reln), len);

  tmp = ExternalFuncName(RelationGlassBoxExtFunc(reln));
  FPRINTF(fp,"\t$BASETYPE %s;\n",tmp);			/* the funcname */
  FPRINTF(fp,"\t$INDEX %d\n",GlassBoxRelIndex(reln));

  SaveRelationVariables(fp,reln);
  FPRINTF(fp,"}\n\n");					/* the trailer */
}


#ifdef  THIS_IS_AN_UNUSED_FUNCTION
/*
 * This function should be good enough to save token relations
 * and opcode relations to glassbox format. Blackbox relations
 * are not in order here.
 */
static
void Save__Reln2GlassBox(FILE *fp, CONST struct Instance *relinst,
		       char *prefix, unsigned long index)
{
  CONST struct relation *reln;
  enum Expr_enum type;
  unsigned long len;

  reln = GetInstanceRelation(relinst,&type);
  assert(type!=e_blackbox);
  len = NumberVariables(reln);
  if (!len) return;

  FPRINTF(fp,"$RELATION %lu {\n",GetTmpNum(relinst)); 	/* header */
  FPRINTF(fp,"\t$KIND %d, $RELOP %d, $COUNT %lu;\n",
	  (int)type, (int)RelationRelop(reln), len);
  FPRINTF(fp,"\t$BASETYPE %s;\n",prefix);		/* the funcname */
  FPRINTF(fp,"\t$INDEX %lu;\n",index);

  SaveRelationVariables(fp,reln);
  FPRINTF(fp,"}\n\n");					/* the trailer */
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */


/*
 *********************************************************************
 * SaveReln2GlassBox
 *
 * Save a relation and perform conversion to be able to restore it
 * as a glassbox.
 *********************************************************************
 */
void SaveReln2GlassBox(FILE *fp, CONST struct Instance *relinst,
		       char *prefix, unsigned long index_)
{
  enum Expr_enum type;

  type = GetInstanceRelationType(relinst);
  switch (type) {
  case e_token:		/* token -> glassbox */
  case e_opcode:	/* opcode -> glassbox */
    SaveReln2GlassBox(fp,relinst,prefix,index_);
    break;
  case e_glassbox:
    SaveGlassBoxRelation(fp,relinst);	/* we will use the existing prefix */
    break;
  default:
    FPRINTF(ASCERR,"Relation type not supported in SaveGlassBox\n");
    break;
  }
}

int ConversionIsValid(enum Expr_enum old, enum Expr_enum new)
{
  if (new==old)
    return 1;

  switch (old) {
  case e_token:
    if (new!=e_blackbox)
      return 1;		/* we can handle all but blackboxes */
    return 0;
  case e_opcode:	/* not fully supported yet */
    return 0;
  case e_glassbox:
  case e_blackbox:
    return 0;
  default:
    FPRINTF(ASCERR,"ConversionIsValid called with nonrelation type.\n");
    return 0;
  }
}

void WriteNamesInList(FILE *f, struct gl_list_t *l, CONST char *sep)
{
	unsigned long n,i;
	struct Name *name;

	n = gl_length(l);
	if (f==NULL || l == NULL) { return; }
	for (i = 1; i <= n; i++) {
		name = (struct Name *)gl_fetch(l,i);
		WriteName(f,name);
		fprintf(f,"%s",sep);
	}
}

void WriteNamesInList2D(FILE *f, struct gl_list_t *l, CONST char *sep, CONST char *sep2)
{
	unsigned long n,i;
	struct gl_list_t *gl;

	n = gl_length(l);
	if (f==NULL || l == NULL) { return; }
	for (i = 1; i <= n; i++) {
		gl = (struct gl_list_t *)gl_fetch(l,i);
		WriteNamesInList(f,gl,sep);
		fprintf(f,"%s --list %d--",sep2,(int)i);
	}
	fprintf(f,"%s",sep2);
}
