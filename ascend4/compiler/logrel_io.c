/*
 *  Logical Relation Output Routines
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: logrel_io.c,v $
 *  Date last modified: $Date: 1997/10/28 19:20:39 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
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
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/find.h"
#include "compiler/instance_enum.h"
#include "compiler/instance_io.h"
#include "compiler/logical_relation.h"
#include "compiler/logrelation.h"
#include "compiler/logrel_util.h"
#include "compiler/logrel_io.h"
#include "compiler/instquery.h"
#include "compiler/parentchild.h"
#include "compiler/mathinst.h"
#include "compiler/visitinst.h"
#include "compiler/tmpnum.h"


static char g_log_shortbuf[256];
#define SBL255 g_log_shortbuf

/* Logical Operators */
static
void WriteLogOp(FILE *f, enum Expr_enum t)
{
  switch(t){
  case e_and: FPRINTF(f,"AND"); break;
  case e_not: FPRINTF(f,"NOT"); break;
  case e_or: FPRINTF(f,"OR"); break;
  case e_boolean_eq: FPRINTF(f,"=="); break;
  case e_boolean_neq: FPRINTF(f,"!="); break;
  default:
    Asc_Panic(2, NULL, "Unknown term in WriteLogOp.\n");/*NOTREACHED*/
  }
}

/* appends operators to dynamic symbol */
static void WriteLogOpDS(Asc_DString *dsPtr, enum Expr_enum t)
{
  assert(dsPtr!=NULL);
  switch(t){
  case e_and:
    Asc_DStringAppend(dsPtr,"AND",3);
    break;
  case e_not:
    Asc_DStringAppend(dsPtr,"NOT",3);
    break;
  case e_or:
    Asc_DStringAppend(dsPtr,"OR",2);
    break;
  case e_boolean_eq:
    Asc_DStringAppend(dsPtr,"==",2);
    break;
  case e_boolean_neq:
    Asc_DStringAppend(dsPtr,"!=",2);
    break;
  default:
    FPRINTF(ASCERR,"Unknown term in WriteLogOpDS.\n");
    Asc_DStringAppend(dsPtr,"ERROR",5);
    break;
  }
}


static
void WriteLogTerm(FILE *f,
	          CONST struct logrelation *lrel,
	          CONST struct logrel_term *term,
	          CONST struct Instance *inst)
{
  struct Instance *cur_var;
  struct Instance *rel;
  int bvalue;

  switch(LogRelTermType(term)){
  case e_var:
    cur_var = LogRelBoolVar(lrel,LogTermBoolVarNumber(term));
    WriteInstanceName(f,cur_var,inst);
    break;
  case e_satisfied:
    FPRINTF(f,"SATISFIED(");
    rel = LogRelRelation(lrel,LogTermSatRelNumber(term));
    WriteInstanceName(f,rel,inst);
    if (LogTermSatTolerance(term) != DBL_MAX) {
      FPRINTF(f,",");
      FPRINTF(f,"%g",LogTermSatTolerance(term));
    }
    FPRINTF(f,")");
    break;
  case e_int:
    FPRINTF(f,"%d",LogTermInteger(term));
    break;
  case e_boolean:
    bvalue = LogTermBoolean(term);
    if (bvalue){
      FPRINTF(f,"TRUE");
    }
    else {
      FPRINTF(f,"FALSE");
    }
    break;
  case e_and:
  case e_or:
    WriteLogOp(f,LogRelTermType(term));
    break;
  case e_not: FPRINTF(f,"NOT"); break;
  default:
    Asc_Panic(2, NULL, "Unknown term type in WriteLogTerm.\n");/*NOTREACHED*/
  }
}

static
void WriteLogTermDS(Asc_DString *dsPtr,
	            CONST struct logrelation *lrel,
	            CONST struct logrel_term *term,
	            CONST struct Instance *inst)
{
  struct Instance *cur_var;
  struct Instance *rel;
  int bvalue;
  switch(LogRelTermType(term)){
  case e_var:
    cur_var = LogRelBoolVar(lrel,LogTermBoolVarNumber(term));
    WriteInstanceNameDS(dsPtr,cur_var,inst);
    break;
  case e_satisfied:
    Asc_DStringAppend(dsPtr,"SATISFIED(",10);
    rel = LogRelRelation(lrel,LogTermSatRelNumber(term));
    WriteInstanceNameDS(dsPtr,rel,inst);
    if (LogTermSatTolerance(term) != DBL_MAX) {
      Asc_DStringAppend(dsPtr,",",1);
      sprintf(SBL255,"%g",LogTermSatTolerance(term));
      Asc_DStringAppend(dsPtr,SBL255,-1);
    }
    Asc_DStringAppend(dsPtr,")",1);
    break;
  case e_int:
    sprintf(SBL255,"%d",LogTermInteger(term));
    Asc_DStringAppend(dsPtr,SBL255,-1);
    break;
  case e_boolean:
    bvalue = LogTermBoolean(term);
    if (bvalue){
      Asc_DStringAppend(dsPtr,"TRUE",4);
    }
    else {
      Asc_DStringAppend(dsPtr,"FALSE",5);
    }
    break;
  case e_and:
  case e_or:
    WriteLogOpDS(dsPtr,LogRelTermType(term));
    break;
  case e_not:
    Asc_DStringAppend(dsPtr,"NOT",3);
    break;
  default:
    FPRINTF(ASCERR,"Unknown term in WriteLogTermDS.\n");
    Asc_DStringAppend(dsPtr,"ERROR",5);
    break;
  }
}

static
void WriteLogSidePostfix(FILE *f,
		         CONST struct logrelation *lr,
		         int side,
		         CONST struct Instance *ref)
{
  unsigned c,len;
  CONST struct logrel_term *term;
  len = LogRelLength(lr,side);
  for(c=1;c<=len;c++){
    term = LogRelTerm(lr,c,side);
    WriteLogTerm(f,lr,term,ref);
    if(c<len) PUTC(' ',f);
  }
}

static
void WriteLogSidePostfixDS(Asc_DString *dsPtr,
		           CONST struct logrelation *lr,
	 	           int side,
		           CONST struct Instance *ref)
{
  unsigned c,len;
  CONST struct logrel_term *term;
  len = LogRelLength(lr,side);
  for(c=1;c<=len;c++){
    term = LogRelTerm(lr,c,side);
    WriteLogTermDS(dsPtr,lr,term,ref);
    if(c<len) Asc_DStringAppend(dsPtr," ",1);
  }
}

void WriteLogRelPostfix(FILE *f,
			CONST struct Instance *lrelinst,
			CONST struct Instance *ref)
{
  CONST struct logrelation *lr;

  lr = GetInstanceLogRel(lrelinst);
  switch(LogRelRelop(lr)){
  case e_boolean_eq:
  case e_boolean_neq:
    WriteLogSidePostfix(f,lr,1,ref);
    PUTC(' ',f);
    WriteLogSidePostfix(f,lr,0,ref);
    PUTC(' ',f);
    WriteLogOp(f,LogRelRelop(lr));
    break;
  default:
    FPRINTF(ASCERR,"Unexpected boolean Relop in WriteLogRelPostfix\n");
    break;
  }
}

char *WriteLogRelPostfixToString( CONST struct Instance *lrelinst,
			        CONST struct Instance *ref)
{
  CONST struct logrelation *lr;
  char *result;
  static Asc_DString ds;
  Asc_DString *dsPtr;

  lr = GetInstanceLogRel(lrelinst);
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);

  switch(LogRelRelop(lr)){
  case e_boolean_eq:
  case e_boolean_neq:
    WriteLogSidePostfixDS(dsPtr,lr,1,ref);
    Asc_DStringAppend(dsPtr," ",1);
    WriteLogSidePostfixDS(dsPtr,lr,0,ref);
    Asc_DStringAppend(dsPtr," ",1);
    WriteLogOpDS(dsPtr,LogRelRelop(lr));
    break;
  default:
    FPRINTF(ASCERR,"Unexpected boolean Relop in WriteLogRelPostfixToString\n");
    break;
  }
  result = Asc_DStringResult(dsPtr);
  Asc_DStringFree(dsPtr);
  return result;
}

struct logrel_stack {
  struct logrel_stack *next;
  unsigned long pos;
  int first;
};

static struct logrel_stack *g_logrel_stack=NULL;

static
void ClearLogRelStack(void)
{
  struct logrel_stack *next;
  while(g_logrel_stack!=NULL){
    next =g_logrel_stack->next;
    ascfree((char *)g_logrel_stack);
    g_logrel_stack = next;
  }
}

static
void PushLogRel(unsigned long int pos, int first)
{
  struct logrel_stack *next;
  next = g_logrel_stack;
  g_logrel_stack = (struct logrel_stack *)
                                      ascmalloc(sizeof(struct logrel_stack));
  g_logrel_stack->next = next;
  g_logrel_stack->pos = pos;
  g_logrel_stack->first = first;
}

static
int LogRelNotEmptyStack(void)
{
  return g_logrel_stack!=NULL;
}

static
int LogrelFirstTop(void)
{
  assert(g_logrel_stack!=NULL);
  return g_logrel_stack->first;
}

static
unsigned long PopLogRel(void)
{
  struct logrel_stack *next;
  unsigned long result;
  assert(g_logrel_stack!=NULL);
  next = g_logrel_stack->next;
  result = g_logrel_stack->pos;
  ascfree((char *)g_logrel_stack);
  g_logrel_stack = next;
  return result;
}

static
unsigned long LogRelLeftHandSide(CONST struct logrelation *lr,
			         unsigned long int pos,
			         int side)
{
  unsigned long depth=1;
  CONST struct logrel_term *term;
  pos--;
  while(depth){
    term = LogRelTerm(lr,pos,side);
    switch(LogRelTermType(term)){
    case e_int:
    case e_boolean:
    case e_satisfied:
    case e_var:
      depth--;
      break;
    case e_not:
      break;
    case e_and:
    case e_or:
      depth++;
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of logical relation type.\n"
                "(%d) in function LogRelLeftHandSide\n",
                LogRelTermType(term));
    }
    pos--;
  }
  return pos;
}

/* a little function to tell us about precedence */
static
unsigned LogPriority(enum Expr_enum t)
{
  switch(t){
  case e_var:
  case e_int:
  case e_satisfied:
    return 0;

  case e_and:
    return 1;

  case e_or:
    return 2;

  case e_not:
    return 3;

  default:
    return 0; /* 0=good idea? */
  }
  /*NOTREACHED*/
}

int LogExprNeedParentheses(enum Expr_enum parent_op, enum Expr_enum child_op, int rhs)
{
  unsigned parent_p,child_p;
  switch(child_op){
  case e_var:
  case e_int:
  case e_boolean:
  case e_satisfied:
    return 0;
  default:
    parent_p = LogPriority(parent_op);
    child_p = LogPriority(child_op);
    if (parent_p > child_p) return 1;
    if (parent_p < child_p) return 0;
    if ((parent_op == e_not)&&rhs) return 1;
    return 0;
  }
}

static
void WriteLogSide(FILE *f,
	          CONST struct logrelation *lr,
	          int side,
	          CONST struct Instance *ref)
{
  unsigned long pos,lhs;
  int first;
  enum Expr_enum t;
  CONST struct logrel_term *term;
  struct Instance *cur_var;
  struct Instance *rel;
  int bvalue;
  ClearLogRelStack();
  PushLogRel(LogRelLength(lr,side),1);
  while(LogRelNotEmptyStack()){
    first = LogrelFirstTop();	 /* check if this is the first visit */
    pos = PopLogRel();           /* check the top */
    term = LogRelTerm(lr,pos,side);
    switch(t = LogRelTermType(term)){
    case e_var:
      cur_var = LogRelBoolVar(lr,LogTermBoolVarNumber(term));
      WriteInstanceName(f,cur_var,ref);
      break;
    case e_int:
      FPRINTF(f,"%d",LogTermInteger(term));
      break;
    case e_boolean:
      bvalue = LogTermBoolean(term);
      if (bvalue){
        FPRINTF(f,"TRUE");
      }
      else {
        FPRINTF(f,"FALSE");
      }
      break;
    case e_satisfied:
      if(first) {
        FPRINTF(f,"SATISFIED(");
        rel = LogRelRelation(lr,LogTermSatRelNumber(term));
        WriteInstanceName(f,rel,ref);
        if (LogTermSatTolerance(term) != DBL_MAX) {
          FPRINTF(f,",");
          FPRINTF(f,"%g",LogTermSatTolerance(term));
	}
          FPRINTF(f,")");
      }
      else{
	PUTC(')',f);
      }
      break;
    case e_and:
    case e_or:
      switch(first){
      case 1:
	PushLogRel(pos,2);
	lhs = LogRelLeftHandSide(lr,pos,side);
	term = LogRelTerm(lr,lhs,side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),0))
	  PUTC('(',f);
	PushLogRel(lhs,1);
	break;
      case 2:
	term = LogRelTerm(lr,LogRelLeftHandSide(lr,pos,side),side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),0))
	  PUTC(')',f);
	PUTC(' ',f);
	WriteLogOp(f,t);
	PUTC(' ',f);
	PushLogRel(pos,0);
	term = LogRelTerm(lr,pos-1,side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),1))
	  PUTC('(',f);
	PushLogRel(pos-1,1);
	break;
      case 0:
	term = LogRelTerm(lr,pos-1,side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),1))
	  PUTC(')',f);
	break;
      }
      break;
    case e_not:
      if (first){
	FPRINTF(f,"NOT");
	PushLogRel(pos,0);
	  PUTC('(',f);
	PushLogRel(pos-1,1);
      }
      else{
	  PUTC(')',f);
      }
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of logical relation type.\n"
                "function WriteLogSide\n");;
    }
  }
}

static
void WriteLogSideDS(Asc_DString *dsPtr, CONST struct logrelation *lr, int side,
	            CONST struct Instance *ref)
{
  unsigned long pos,lhs;
  int first;
  enum Expr_enum t;
  CONST struct logrel_term *term;
  struct Instance *cur_var;
  struct Instance *rel;
  int bvalue;
  ClearLogRelStack();
  PushLogRel(LogRelLength(lr,side),1);
  while(LogRelNotEmptyStack()){
    first = LogrelFirstTop();	      /* check if this is the first visit */
    pos = PopLogRel();	              /* check the top */
    term = LogRelTerm(lr,pos,side);
    switch(t = LogRelTermType(term)){
    case e_var:
      cur_var = LogRelBoolVar(lr,LogTermBoolVarNumber(term));
      WriteInstanceNameDS(dsPtr,cur_var,ref);
      break;
    case e_int:
      sprintf(SBL255,"%d",LogTermInteger(term));
      Asc_DStringAppend(dsPtr,SBL255,-1);
      break;
    case e_boolean:
      bvalue = LogTermBoolean(term);
      if (bvalue){
        Asc_DStringAppend(dsPtr,"TRUE",4);
      }
      else {
        Asc_DStringAppend(dsPtr,"FALSE",5);
      }
      break;
    case e_satisfied:
      if(first) {
        Asc_DStringAppend(dsPtr,"SATISFIED(",10);
        rel = LogRelRelation(lr,LogTermSatRelNumber(term));
        WriteInstanceNameDS(dsPtr,rel,ref);
        if (LogTermSatTolerance(term) != DBL_MAX) {
          Asc_DStringAppend(dsPtr,",",1);
          sprintf(SBL255,"%g",LogTermSatTolerance(term));
          Asc_DStringAppend(dsPtr,SBL255,-1);
	}
        Asc_DStringAppend(dsPtr,")",1);
      }
      else{
        Asc_DStringAppend(dsPtr,")",1);
      }
      break;
    case e_and:
    case e_or:
      switch(first){
      case 1:
	PushLogRel(pos,2);
	lhs = LogRelLeftHandSide(lr,pos,side);
	term = LogRelTerm(lr,lhs,side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),0))
          Asc_DStringAppend(dsPtr,"(",1);
	PushLogRel(lhs,1);
	break;
      case 2:
	term = LogRelTerm(lr,LogRelLeftHandSide(lr,pos,side),side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),0)) {
          Asc_DStringAppend(dsPtr,")",1);
        }
        Asc_DStringAppend(dsPtr," ",1);
	WriteLogOpDS(dsPtr,t);
        Asc_DStringAppend(dsPtr," ",1);
	PushLogRel(pos,0);
	term = LogRelTerm(lr,pos-1,side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),1)) {
          Asc_DStringAppend(dsPtr,"(",1);
        }
	PushLogRel(pos-1,1);
	break;
      case 0:
	term = LogRelTerm(lr,pos-1,side);
	if (LogExprNeedParentheses(t,LogRelTermType(term),1))
          Asc_DStringAppend(dsPtr,")",1);
	break;
      }
      break;
    case e_not:
      if (first){
        Asc_DStringAppend(dsPtr,"NOT",3);
	PushLogRel(pos,0);
          Asc_DStringAppend(dsPtr,"(",1);
	PushLogRel(pos-1,1);
      }
      else{
        Asc_DStringAppend(dsPtr,")",1);
      }
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of logical relation type.\n"
                "function WriteLogSide\n");
    }
  }
}

static
void WriteLogicalRelation(FILE *f,
		          CONST struct logrelation *lr,
			  CONST struct Instance *ref)
{
  switch(LogRelRelop(lr)){
  case e_boolean_eq:
  case e_boolean_neq:
    WriteLogSide(f,lr,1,ref);
    PUTC(' ',f);
    WriteLogOp(f,LogRelRelop(lr));
    PUTC(' ',f);
    WriteLogSide(f,lr,0,ref);
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in WriteLogicalRelation\n");
    break;
  }
}

static
void WriteLogRelDS(Asc_DString *dsPtr,
			    CONST struct logrelation *lr,
			    CONST struct Instance *ref)
{
  switch(LogRelRelop(lr)){
  case e_boolean_eq:
  case e_boolean_neq:
    WriteLogSideDS(dsPtr,lr,1,ref);
    Asc_DStringAppend(dsPtr," ",1);
    WriteLogOpDS(dsPtr,LogRelRelop(lr));
    Asc_DStringAppend(dsPtr," ",1);
    WriteLogSideDS(dsPtr,lr,0,ref);
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in WriteLogRelDS\n");
    Asc_DStringAppend(dsPtr,"BADRELOPR ",9);
    break;
  }
}

static
void WriteLogSideInfix(FILE *f,
		        CONST struct logrelation *lr,
		        struct logrel_term *term,
		        CONST struct Instance *ref)
{
  enum Expr_enum t;
  struct Instance *cur_var;
  struct Instance *rel;
  int bvalue;
  int parens;

  switch(t = LogRelTermType(term)) {
  case e_var:
    cur_var = LogRelBoolVar(lr,LogTermBoolVarNumber(term));
    WriteInstanceName(f,cur_var,ref);
    break;
  case e_int:
    FPRINTF(f,"%d",LogTermInteger(term));
    break;
  case e_boolean:
    bvalue = LogTermBoolean(term);
    if (bvalue){
      FPRINTF(f,"TRUE");
    }
    else {
      FPRINTF(f,"FALSE");
    }
    break;
  case e_satisfied:
    FPRINTF(f,"SATISFIED(");
    rel = LogRelRelation(lr,LogTermSatRelNumber(term));
    WriteInstanceName(f,rel,ref);
    if (LogTermSatTolerance(term) != DBL_MAX) {
      FPRINTF(f,",");
      FPRINTF(f,"%g",LogTermSatTolerance(term));
    }
    FPRINTF(f,")");
    break;
  case e_not:
    FPRINTF(f,"NOT(");
    WriteLogSideInfix(f,lr,LogTermUniLeft(term),ref);
    FPRINTF(f,")");
    break;
  case e_and:
  case e_or:
    parens = LogExprNeedParentheses(LogRelTermType(term),
		        LogRelTermType(LogTermBinLeft(term)),0);
    if (parens) PUTC('(',f);
    WriteLogSideInfix(f,lr,LogTermBinLeft(term),ref);
    if (parens) PUTC(')',f);
    PUTC(' ',f);
    WriteLogOp(f,t);
    PUTC(' ',f);
    parens = LogExprNeedParentheses(LogRelTermType(term),
		        LogRelTermType(LogTermBinRight(term)),1);
    if (parens) PUTC('(',f);
    WriteLogSideInfix(f,lr,LogTermBinRight(term),ref);
    if (parens) PUTC('(',f);
    break;
  default:
    FPRINTF(f,"***");
    break;
  }
}

void WriteLogRelInfix(FILE *f,
		       CONST struct Instance *lrelinst,
		       CONST struct Instance *ref)
{
  CONST struct logrelation *lr;

  lr = GetInstanceLogRel(lrelinst);
  switch(LogRelRelop(lr)){
  case e_boolean_eq:
  case e_boolean_neq:
    WriteLogSideInfix(f,lr,Infix_Log_LhsSide(lr),ref);
    PUTC(' ',f);
    WriteLogOp(f,LogRelRelop(lr));
    PUTC(' ',f);
    WriteLogSideInfix(f,lr,Infix_Log_RhsSide(lr),ref);
    break;
  default:
    FPRINTF(ASCERR,"Unexpected Relop in WriteLogRelInfix\n");
  }
}



void WriteLogRel(FILE *f, CONST struct Instance *lrelinst,
		 CONST struct Instance *ref)
{
  CONST struct logrelation *lreln;

  lreln = GetInstanceLogRel(lrelinst);
  if (!lreln) {
    FPRINTF(f,"NULL logical relation\n");
    return;
  }
  WriteLogicalRelation(f,lreln,ref);
  return;
}

char *WriteLogRelToString(CONST struct Instance *lrelinst,
                        CONST struct Instance *ref)
{
  CONST struct logrelation *lreln;
  static Asc_DString ds;
  Asc_DString *dsPtr;
  char *result;

  lreln = GetInstanceLogRel(lrelinst);
  if (!lreln) {
    result = (char *) ascmalloc(15);
    if (result == NULL) return result;
    sprintf(result,"NULL logical relation\n");
    return result;
  }

  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  WriteLogRelDS(dsPtr,lreln,ref);
  result = Asc_DStringResult(dsPtr);
  Asc_DStringFree(dsPtr);
  return result;
}


/*
 * some io to help in debugging logical relation manipulators.
 */
static FILE *g_logwritfp = NULL;
static void WriteIfLogRel(struct Instance *i)
{
  struct Instance *p;
  if (!i) {
    FPRINTF(ASCERR,"null child pointer in WriteIfLogRel\n");
    return;
  }
  if (InstanceKind(i)==LREL_INST) {
    FPRINTF(g_logwritfp,"\n");
    WriteInstanceName(g_logwritfp,i,NULL);
    FPRINTF(g_logwritfp,"\n");
    p = InstanceParent(i,1);
    while (InstanceKind(p)!= MODEL_INST && InstanceKind(p) != SIM_INST) {
      p = InstanceParent(p,1);
    }
    WriteLogRel(g_logwritfp,i,p);
    FPRINTF(g_logwritfp,"\n");
    FPRINTF(g_logwritfp,"\n");
    WriteLogRelPostfix(g_logwritfp,i,p);
    FPRINTF(g_logwritfp,"\n");
  }
  return;
}

void WriteLogRelationsInTree(FILE *fp,struct Instance *i)
{
  if (i==NULL || fp==NULL) return;
  g_logwritfp = fp;
  SlowVisitInstanceTree(i,WriteIfLogRel,0,0);
}


/*
 *********************************************************************
 * Save Logical Relation Code
 *
 * The below code is concerned with saving relations in a persistent
 * format. It writes out the code in a condensed format to allow
 * restoration to the original representation.
 *
 * The grammar will follow later once it has stabilized.
 * In the mean time, the following keywords are used.
 * $LRELOP INTEGER - the relational operator - e_boolean_eq etc.
 * $LOPCODES INTEGER ':' (INTEGER *)
 * $LCOUNT INTEGER - the number of variables.
 * $BVARIABLES ':' (INTEGER)*	- global varindex
 * $BCONSTANTS ':' (BOOLEAN)*
 *********************************************************************
 */

#undef LogLHS
#undef LogRHS
#define LogLHS 0
#define LogRHS 1
#define LBREAKLINES 65
#define SATID 23
static
void SaveLogRelSide(FILE *fp,CONST struct logrelation *lr,
		         int side, struct gl_list_t *constants)
{
  CONST struct logrel_term *term;
  struct Instance *rel;
  enum Expr_enum t;
  unsigned c,len;
  int count;

  len = LogRelLength(lr,side);
  if (!len) return;

  count = 16;
  FPRINTF(fp,"\t$LOPCODES %d : ",side);
  for (c=1;c<=len;c++) {
    term = LogRelTerm(lr,c,side);
    t = LogRelTermType(term);
    count += FPRINTF(fp," %d ",(int)t);
    switch (t) {
    case e_var:
      count += FPRINTF(fp,"%lu",LogTermBoolVarNumber(term));
      break;
    case e_satisfied:
      count += FPRINTF(fp,"%d",SATID);
      rel = LogRelRelation(lr,LogTermSatRelNumber(term));
      count += FPRINTF(fp," %lu ",GetTmpNum(rel));
      if (LogTermSatTolerance(term) != DBL_MAX) {
        count += FPRINTF(fp,"%12.8e",LogTermSatTolerance(term));
      }
      break;
    case e_int:
    case e_boolean:
      gl_append_ptr(constants,(VOIDPTR)term);
      count += FPRINTF(fp,"%lu",gl_length(constants));
      break;
    case e_not:
    case e_and:
    case e_or:
      break;
    default:
      count += FPRINTF(fp,"%d",(int)e_nop);
      break;
    }
    if (count >= LBREAKLINES) {
      PUTC('\n',fp); PUTC('\t',fp);
      count = 8;
    }
  }
  FPRINTF(fp,";\n");
}


static
void SaveTokenLogConstants(FILE *fp, struct gl_list_t *constants)
{
  CONST struct logrel_term *term;
  unsigned long len,c;

  len = gl_length(constants);
  if (!len) return;

  FPRINTF(fp,"\t$BCONSTANTS : ");
  for (c=1;c<=len;c++) {
    term = (CONST struct logrel_term *)gl_fetch(constants,c);
    switch (LogRelTermType(term)) {
    case e_boolean:
      FPRINTF(fp,"%d",LogTermBoolean(term));
      break;
    case e_int:
      FPRINTF(fp,"%d",LogTermInteger(term));
      break;
    default:
      Asc_Panic(2, NULL, "Illegal term in SaveTokenLogConstants\n");
    }
    PUTC(' ',fp);
  }
  FPRINTF(fp,";\n");
}

/*
 *********************************************************************
 * Save Logical Relation Boolean Variables
 *********************************************************************
 */
void SaveLogRelBoolVars(FILE *fp, CONST struct logrelation *lr)
{
  struct Instance *bvar;
  unsigned long len,c;
  int count;
  len = NumberBoolVars(lr);
  if (!len) return;

  count = 16;
  FPRINTF(fp,"\t$BVARIABLES : ");
  for (c=1;c<=len;c++) {
    bvar = LogRelBoolVar(lr,c);
    count += FPRINTF(fp," %lu ",GetTmpNum(bvar));
    if (count >= LBREAKLINES) {
      PUTC('\n',fp); PUTC('\t',fp);
      count = 8;
    }
  }
  FPRINTF(fp,";\n");
}

/*
 *********************************************************************
 * SaveLogRel
 * Save a logical relation in condensed opcode format.
 *********************************************************************
 */
void SaveLogRel(FILE *fp, CONST struct Instance *lrelinst)
{
  CONST struct logrelation *lreln;
  struct gl_list_t *constants = NULL;

  lreln = GetInstanceLogRel(lrelinst);
  constants = gl_create(50L);

  FPRINTF(fp,"$LOGRELATION %lu {\n",GetTmpNum(lrelinst));
  FPRINTF(fp,"\t$COUNT %lu;\n",NumberBoolVars(lreln));

  SaveLogRelSide(fp,lreln,LogLHS,constants);
  SaveLogRelSide(fp,lreln,LogRHS,constants);
  SaveTokenLogConstants(fp,constants);
  SaveLogRelBoolVars(fp,lreln);
  FPRINTF(fp,"}\n\n");

  gl_destroy(constants);
}


