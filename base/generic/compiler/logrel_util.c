/*
 *  Logical Relation utility functions for Ascend
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.16 $
 *  Version control file: $RCSfile: logrel_util.c,v $
 *  Date last modified: $Date: 1998/04/10 23:28:35 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Interpreter.
 *
 *  The Ascend Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  ASCEND is distributed in hope that it will be
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
#include <math.h>
#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/instance_name.h"
#include "compiler/find.h"
#include "compiler/atomvalue.h"
#include "compiler/instance_io.h"
#include "compiler/logical_relation.h"
#include "compiler/logrelation.h"
#include "compiler/logrel_util.h"
#include "compiler/extfunc.h"
#include "compiler/relation_type.h"
#include "compiler/relation.h"
#include "compiler/relation_util.h"
#include "compiler/instance_io.h"
#include "compiler/instquery.h"
#include "compiler/visitinst.h"
#include "compiler/mathinst.h"

#define DEFTOLERANCE 1e-08

/*********************************************************************\
  logical relation term queries section.
\*********************************************************************/

enum Expr_enum LogRelRelop(CONST struct logrelation *lrel)
{
  AssertAllocatedMemory(lrel,sizeof(struct logrelation));
  return lrel->relop;
}


unsigned long LogRelLength(CONST struct logrelation *lrel, int lhs)
{
  assert(lrel!=NULL);
  AssertAllocatedMemory(lrel,sizeof(struct logrelation));
  if (lhs){
    if (lrel->token.lhs) return (lrel->token.lhs_len);
    else return 0;
  }
  if (lrel->token.rhs) return (lrel->token.rhs_len);
  else return 0;
}

/*
 * This query assumes the
 * user still thinks tokens number from [1..len].
 */
CONST struct logrel_term *LogRelTerm(CONST struct logrelation *lrel,
				     unsigned long int pos, int lhs)
{
  assert(lrel!=NULL);
  AssertAllocatedMemory(lrel,sizeof(struct logrelation));
  if (lhs){
    if (lrel->token.lhs)
      return LOGA_TERM(&(lrel->token.lhs[pos-1]));
    else return NULL;
  }
  else{
    if (lrel->token.rhs)
      return LOGA_TERM(&(lrel->token.rhs[pos-1]));
    else return NULL;
  }
}


CONST struct logrel_term
*NewLogRelTermF(CONST struct logrelation *lrel, unsigned long pos, int lhs)
{
  assert(lrel!=NULL);
  AssertAllocatedMemory(lrel,sizeof(struct logrelation));
  if (lhs){
    if (lrel->token.lhs != NULL)
      return LOGA_TERM(&(lrel->token.lhs[pos]));
    else return NULL;
  } else {
    if (lrel->token.rhs != NULL)
      return LOGA_TERM(&(lrel->token.rhs[pos]));
    else return NULL;
  }
}


CONST struct logrel_term
*LogRelSideTermF(CONST union LogRelTermUnion *side, unsigned long pos)
{
  assert(side!=NULL);
  return LOGA_TERM(&(side[pos]));
}


enum Expr_enum LogRelTermTypeF(CONST struct logrel_term *term)
{
  AssertMemory(term);
  return term->t;
}

unsigned long LogTermBoolVarNumber(CONST struct logrel_term *term)
{
  assert(term&&term->t == e_var);
  AssertMemory(term);
  return LOGBV_TERM(term)->varnum;
}

int LogTermBoolean(CONST struct logrel_term *term)
{
  assert(term&&(term->t==e_boolean));
  AssertMemory(term);
  return LOGBC_TERM(term)->bvalue;
}

int LogTermInteger(CONST struct logrel_term *term)
{
  assert(term&&(term->t==e_int));
  AssertMemory(term);
  return (LOGI_TERM(term)->ivalue);
}

int LogTermIntegerBoolValue(CONST struct logrel_term *term)
{
  assert(term&&(term->t==e_int));
  AssertMemory(term);
  if (LOGI_TERM(term)->ivalue){
    return 1;
  }
  else {
    return 0;
  }
}

int
LogTermBoolVar(CONST struct logrelation *lrel, CONST struct logrel_term *term)
{
  return
    GetBooleanAtomValue((struct Instance *)
      LogRelBoolVar(lrel,LogTermBoolVarNumber(term)));
}

int
LogTermSatisfied(CONST struct logrelation *lrel,
                 CONST struct logrel_term *term,
                 int perturb,
                 struct gl_list_t *instances)
{
  struct Instance *inst;
  struct Instance *relname;
  CONST struct relation *rel;
  enum Expr_enum relop;
  enum safe_err status = safe_ok;
  double res;
  double tol;
  unsigned long len,n;
  int satisfied,not_satisfied;
  int logres;

  inst = LogRelRelation(lrel,LogTermSatRelNumber(term));
  tol = LogTermSatTolerance(term);

  satisfied = 1;
  not_satisfied = 0;

  if (perturb && (instances!=NULL)) {
    len = gl_length(instances);
    for (n=1;n<=len;n++) {
      relname = (struct Instance *)(gl_fetch(instances,n));
      if (inst == relname) {
        satisfied = 0;
        not_satisfied = 1;
        break;
      }
    }
  }
  switch (InstanceKind(inst)) {
  case REL_INST:
    status = RelationCalcResidualPostfixSafe(inst,&res);
    if (status != safe_ok) {
      FPRINTF(ASCERR,
        "Something wrong while calculating a residual in Sat Expr\n");
      return 0;
    }
    rel = GetInstanceRelationOnly(inst);
    relop = RelationRelop(rel);
    switch(relop) {
    case e_equal:
      if (tol != DBL_MAX) {
        if(fabs(tol)>fabs(res)) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
      else {
        if((DEFTOLERANCE)>fabs(res)) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
    case e_notequal:
      if (tol != DBL_MAX) {
        if(fabs(tol)>fabs(res)) {
          return not_satisfied;
        }
        else {
          return satisfied;
        }
      }
      else {
        if((DEFTOLERANCE)>fabs(res)) {
          return not_satisfied;
        }
        else {
          return satisfied;
        }
      }
    case e_greater:
      if (tol != DBL_MAX) {
        if(fabs(tol)<res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
      else {
        if((DEFTOLERANCE)<res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
    case e_greatereq:
      if (tol != DBL_MAX) {
        if(-fabs(tol)<res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
      else {
        if(-(DEFTOLERANCE)<res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
    case e_less:
      if (tol != DBL_MAX) {
        if(-fabs(tol)>res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
      else {
        if(-(DEFTOLERANCE)>res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
    case e_lesseq:
      if (tol != DBL_MAX) {
        if(fabs(tol)>res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
      else {
        if((DEFTOLERANCE)>res) {
          return satisfied;
        }
        else {
          return not_satisfied;
        }
      }
    default:
      FPRINTF(ASCERR,
        "Something wrong while calculating a residual in Sat Expr\n");
      return 0;
    }
  case LREL_INST:
    if (LogRelCalcResidualPostfix(inst,&logres,perturb,instances)) {
      FPRINTF(ASCERR,
      "Something wrong while calculating a logical residual in Sat Expr\n");
      return 0;
    }
    if(satisfied) {
      return logres;
    } else {
      return (!logres);
    }
  default:
    FPRINTF(ASCERR,
    "Incorrect instance name (No Log/Relation) in Satisfied Expr\n");
    return 0;
  }
}


CONST struct Name *LogTermSatName(CONST struct logrel_term *term)
{
  assert( term && (term->t==e_satisfied) );
  AssertMemory(term);
  return LOGS_TERM(term)->ncond;
}


unsigned long LogTermSatRelNumber(CONST struct logrel_term *term)
{
  assert(term&&term->t == e_satisfied);
  AssertMemory(term);
  return LOGS_TERM(term)->relnum;
}


double LogTermSatTolerance(CONST struct logrel_term *term)
{
  assert( term && (term->t==e_satisfied) );
  AssertMemory(term);
  return LOGS_TERM(term)->rtol;
}

CONST dim_type *LogTermSatDimensions(CONST struct logrel_term *term)
{
  assert( term && (term->t==e_satisfied) );
  AssertMemory(term);
  return LOGS_TERM(term)->dim;
}


struct logrel_term *LogRelINF_Lhs(CONST struct logrelation *lrel)
{
  return lrel->token.lhs_term;
}

struct logrel_term *LogRelINF_Rhs(CONST struct logrelation *lrel)
{
  return lrel->token.rhs_term;
}



CONST struct gl_list_t *LogRelBoolVarList(CONST struct logrelation *lrel)
{
  return (CONST struct gl_list_t *)lrel->bvars;
}


CONST struct gl_list_t *LogRelSatRelList(CONST struct logrelation *lrel)
{
  return (CONST struct gl_list_t *)lrel->satrels;
}


int LogRelResidual(CONST struct logrelation *lrel)
{
  assert(lrel!=NULL);
  return lrel->logresidual;
}

void SetLogRelResidual(struct logrelation *lrel, int value)
{
  assert(lrel!=NULL);
  lrel->logresidual = value;
}

int LogRelNominal(CONST struct logrelation *lrel)
{
  assert(lrel!=NULL);
  return lrel->lognominal;
}

void SetLogRelNominal(struct logrelation *lrel, int value)
{
  assert(lrel!=NULL);
  lrel->lognominal = value;
}


int LogRelIsCond(CONST struct logrelation *lrel)
{
  assert(lrel!=NULL);
  return lrel->logiscond;
}

void SetLogRelIsCond(struct logrelation *lrel)
{
  assert(lrel!=NULL);
  lrel->logiscond = 1;
}


unsigned long NumberBoolVars(CONST struct logrelation *lrel)
{
  unsigned long n;
  assert(lrel!=NULL);
  n = (lrel->bvars!=NULL) ? gl_length(lrel->bvars) : 0;
  return n;
}

struct Instance *LogRelBoolVar(CONST struct logrelation *lrel,
			       unsigned long int varnum)
{
  assert(lrel!=NULL);
  return (struct Instance *)gl_fetch(lrel->bvars,varnum);
}

unsigned long NumberRelations(CONST struct logrelation *lrel)
{
  unsigned long n;
  assert(lrel!=NULL);
  n = (lrel->satrels!=NULL) ? gl_length(lrel->satrels) : 0;
  return n;
}

struct Instance *LogRelRelation(CONST struct logrelation *lrel,
			        unsigned long int relnum)
{
  assert(lrel!=NULL);
  return (struct Instance *)gl_fetch(lrel->satrels,relnum);
}


static void LogCalcDepth(CONST struct logrelation *lrel,
	                 int lhs,
	                 unsigned long int *depth,
	                 unsigned long int *maxdepth)
{
  unsigned long c,length;
  CONST struct logrel_term *term;
  length = LogRelLength(lrel,lhs);
  for(c=0;c<length;c++){
    term = NewLogRelTerm(lrel,c,lhs);
    switch(LogRelTermType(term)){
    case e_boolean:
    case e_var:
      if (++(*depth) > *maxdepth) *maxdepth = *depth;
      break;
    case e_satisfied:
    case e_not:
      break;
    case e_and:
    case e_or:
      (*depth)--;
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of logical relation type.\n"
                "in function LogCalcDepth\n");
      break;
    }
  }
}

unsigned long LogRelDepth(CONST struct logrelation *lrel)
{
  unsigned long depth=0,maxdepth=0;
  switch(LogRelRelop(lrel)){
  case e_boolean_eq:
  case e_boolean_neq:
    LogCalcDepth(lrel,1,&depth,&maxdepth);
    LogCalcDepth(lrel,0,&depth,&maxdepth);
    assert(depth == 2);
    break;
  default:
    Asc_Panic(2, NULL, "Unknown logical relation type.\n");
    break;
  }
  return maxdepth;
}


/*********************************************************************\
  calculation functions
\*********************************************************************/

/* global logical relation pointer to avoid passing a logical relation
 * recursively
 */
static struct logrelation *glob_lrel;

/* The next three functions are used for finding the value defined for
   the tolerance in a SATISFIED term. The relation instance in the
   satisfied term must be that given by the argument condinst provided
   by the caller */

static int FindTolInSatisfiedTerm(struct logrelation *lrel,
                                struct logrel_term *term,
                                struct Instance *condinst,
                                double *tolerance)
{
  struct Instance *inst;
  double tol;

  inst = LogRelRelation(lrel,LogTermSatRelNumber(term));
  tol = LogTermSatTolerance(term);

  if (inst == condinst) {
    *tolerance = tol;
    return 1;
  } else {
   return 0;
  }
}


/* Note that ANY function calling FinTolInLogRelBranch should set
 * glob_lrel to point at the logical relation being evaluated.
 * The calling function should also set glob_lrel = NULL when it
 * is done.
 */

static int FindTolInLogRelBranch(struct logrel_term *term,
                                 struct Instance *condinst,
                                 double *tolerance)
{
  assert(term != NULL);
  switch(LogRelTermType(term)) {
  case e_var:
  case e_int:
  case e_boolean:
    return 0;
  case e_satisfied:
    if (FindTolInSatisfiedTerm(glob_lrel,term,condinst,tolerance)){
      return 1;
    } else {
      return 0;
    }
  case e_and:
  case e_or:
    if (FindTolInLogRelBranch(LogTermBinLeft(term),condinst,tolerance)){
      return 1;
    } else {
      if (FindTolInLogRelBranch(LogTermBinRight(term),condinst,tolerance)) {
        return 1;
      } else {
        return 0;
      }
    }
  case e_not:
   if (FindTolInLogRelBranch(LogTermBinLeft(term),condinst,tolerance)){
      return 1;
    } else {
      return 0;
    }
  default:
    FPRINTF(ASCERR, "error in FindTolInLogRelBranch routine\n");
    FPRINTF(ASCERR, "logical relation term type not recognized\n");
    return 0;
  }
}


/* For finding the value defined for the tolerance in a SATISFIED term.
 * The relation instance in the satisfied term must be that given by
 * the argument condinst
 */

int FindTolInSatTermOfLogRel(struct Instance *lrelinst,
                             struct Instance *condinst, double *tolerance)
{
  int found;
  glob_lrel = NULL;

  if( lrelinst == NULL ) {
    FPRINTF(ASCERR, "error in FindTolInSatTermOfLogRel: null instance\n");
    return 1;
  }

  if( InstanceKind(lrelinst) != LREL_INST ) {
    FPRINTF(ASCERR, "error in FindTolInSatTermOfLogRel: not logrelation\n");
    return 1;
  }

  glob_lrel = (struct logrelation *)GetInstanceLogRel(lrelinst);

  if( glob_lrel == NULL ) {
    FPRINTF(ASCERR, "error in FindTolInSatTermOfLogRel: null logrelation\n");
    return 1;
  }

  if(Infix_Log_LhsSide(glob_lrel) != NULL) {
    found = FindTolInLogRelBranch(Infix_Log_LhsSide(glob_lrel),condinst,
                                  tolerance);
    if (found) {
      glob_lrel = NULL;
      return 0;
    }
  } else {
    glob_lrel = NULL;
    return 1;
  }

  if(Infix_Log_RhsSide(glob_lrel) != NULL) {
    found= FindTolInLogRelBranch(Infix_Log_RhsSide(glob_lrel),condinst,
                                 tolerance);
    if (found) {
      glob_lrel = NULL;
      return 0;
    } else {
      glob_lrel = NULL;
      return 1;
    }
  } else {
    glob_lrel = NULL;
    return 1;
  }
}


/* Note that ANY function calling LogRelBranchEvaluator should set
 * glob_lrel to point at the logical relation being evaluated.
 * The calling function should also set glob_lrel = NULL when it
 * is done.
 */

static int LogRelBranchEvaluator(struct logrel_term *term, int perturb,
                                 struct gl_list_t *instances)
{
  assert(term != NULL);
  switch(LogRelTermType(term)) {
  case e_var:
    return LogTermBoolVar(glob_lrel,term);
  case e_int:
    return LogTermIntegerBoolValue(term);
  case e_boolean:
    return LogTermBoolean(term);
  case e_satisfied:
    return LogTermSatisfied(glob_lrel,term,perturb,instances);
  case e_and:
    return (LogRelBranchEvaluator(LogTermBinLeft(term),perturb,instances) &&
      LogRelBranchEvaluator(LogTermBinRight(term),perturb,instances));
  case e_or:
    return (LogRelBranchEvaluator(LogTermBinLeft(term),perturb,instances) ||
      LogRelBranchEvaluator(LogTermBinRight(term),perturb,instances));
  case e_not:
    return (!LogRelBranchEvaluator(LogTermBinLeft(term),perturb,instances));
 /* return (LogRelBranchEvaluator(LogTermBinLeft(term),perturb,instances)
                                 ? 0 : 1); */
  default:
    FPRINTF(ASCERR, "error in LogRelBranchEvaluator routine\n");
    FPRINTF(ASCERR, "logical relation term type not recognized\n");
    return 0;
  }
}

/* LogRelEvaluatePostfixBranch
 * This function is passed a logical relation pointer, lrel, a pointer,
 * pos, to a position in the postfix version of the logical relation
 * (0<=pos<length), and a flag, lhs, telling whether we are interested
 * in the left(=1) or right(=0) side of the logical relation.
 * This function will tranverse and evaluate the subtree rooted at pos
 * and will return the value as an integer (0,1).
 * To do its evaluation, this function goes backwards through
 * the postfix representation of logical relation and calls itself at each
 * node--creating a stack of function calls.
 * NOTE: This function changes the value of pos--- to the position of
 * the deepest leaf visited
 */
static int
LogRelEvaluatePostfixBranch(CONST struct logrelation *lrel,
                            unsigned long *pos,
                            int lhs, int perturb,
                            struct gl_list_t *instances)
{
  CONST struct logrel_term *term; /* the current term */
  int b;                          /* temporary value */

  term = NewLogRelTerm(lrel,*pos,lhs);
  assert(term != NULL);
  switch( LogRelTermType(term) ) {
  case e_int:
    return LogTermIntegerBoolValue(term);
  case e_var:
    return LogTermBoolVar(lrel, term);
  case e_boolean:
    return LogTermBoolean(term);
  case e_satisfied:
    return LogTermSatisfied(lrel,term,perturb,instances);
  case e_and:
    (*pos)--;
    b = LogRelEvaluatePostfixBranch(lrel,pos,lhs,perturb,instances);
                                        /* b==right-side of 'AND' */
    (*pos)--;
    return (LogRelEvaluatePostfixBranch(lrel,pos,lhs,perturb,instances) && b);
  case e_or:
    (*pos)--;
    b = LogRelEvaluatePostfixBranch(lrel,pos,lhs,perturb,instances);
                                        /* b==right-side of 'OR' */
    (*pos)--;
    return (LogRelEvaluatePostfixBranch(lrel,pos,lhs,perturb,instances) || b);
  case e_not:
    (*pos)--;
    return (!LogRelEvaluatePostfixBranch(lrel,pos,lhs,perturb,instances));
    /*    return (LogRelEvaluatePostfixBranch(lrel,pos,lhs,perturb,instances))
                                              ? 0 : 1);  */
  default:
    Asc_Panic(2, NULL,
              "Don't know this type of logical relation term\n"
              "in function LogRelEvaluatePostfixBranch\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}


/* LogRelEvaluateResidualPostfix
 * Yet another function for calculating the residual of a logical relation.
 * This function also uses the postfix version of the logical relations,
 * but it manages a stack(array) of ints and calculates the residual in this
 * stack; therefore the function is not recursive.  If the funtion
 * cannot allocate memory for its stack, it returns 0, so there is
 * currently no way of knowing if this function failed.
 */
static int
LogRelEvaluateResidualPostfix(CONST struct logrelation *lrel, int perturb,
                              struct gl_list_t *instances)
{
  unsigned long t;       /* the current term in the logical relation lrel */
  int lhs;               /* looking at left(=1) or right(=0) hand side */
  int *res_stack;        /* the stack we use for evaluating the residual */
  long s = -1;           /* the top position in the stacks */
  unsigned long length_lhs, length_rhs;
  CONST struct logrel_term *term;
  enum Expr_enum lrelop;

  length_lhs = LogRelLength(lrel,1);
  length_rhs = LogRelLength(lrel,0);
  if( (length_lhs+length_rhs) == 0 ) return 0;
  lrelop = LogRelRelop(lrel);
  /* create the stacks */
  res_stack = tmpalloc_array((1+MAX(length_lhs,length_rhs)),int);
  if( res_stack == NULL ) return 0;

  lhs = 1;
  t = 0;
  while (1) {
    if( lhs && (t >= length_lhs) ) {
      /* finished processing left hand side, switch to right if it exists */
      if( length_rhs ) {
        lhs = t = 0;
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_rhs==0), the
         * length_lhs must be > 0, thus s>=0
         */
        return 0;
      }
    }
    else if( (!lhs) && (t >= length_rhs) ) {
      /* finished processing right hand side */
      if( length_lhs ) {
        /* we know length_lhs and length_rhs are both > 0, since if
         * length_rhs == 0, we would have exited above.
         */
        if (lrelop == e_boolean_eq) {
          if (res_stack[s-1] == res_stack[s]) {
            return 1;
	  }
          else {
            return 0;
	  }
	}
        else if (lrelop == e_boolean_neq) {
          if (res_stack[s-1] != res_stack[s]) {
            return 1;
	  }
          else {
            return 0;
	  }
	} else {
           FPRINTF(ASCERR, "error in LogRelEvaluateResidualPostfix:\n");
           FPRINTF(ASCERR, "wrong logical relation operator \n");
           return 0;
	}
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_lhs==0), the
         * length_rhs must be > 0, thus s>=0
         */
        return 0;
      }
    }

    term = NewLogRelTerm(lrel,t++,lhs);
    switch( LogRelTermType(term) ) {
    case e_boolean:
      s++;
      res_stack[s] = LogTermBoolean(term);
      break;
    case e_satisfied:
      s++;
      res_stack[s] = LogTermSatisfied(lrel,term,perturb,instances);
      break;
    case e_int:
      s++;
      res_stack[s] = LogTermIntegerBoolValue(term);
      break;
    case e_var:
      s++;
      res_stack[s] = LogTermBoolVar(lrel,term);
      break;
    case e_and:
      res_stack[s-1] = (res_stack[s-1] && res_stack[s]);
      s--;
      break;
    case e_or:
      res_stack[s-1] = ( res_stack[s-1] || res_stack[s]);
      s--;
      break;
    case e_uminus:
      res_stack[s] = ( !res_stack[s] );
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of logical relation term\n"
                "in function LogRelEvaluateResidualPostfix\n");
      break;
    }
  }
}

/*
 * CALCULATION ROUTINES
 */


int
LogRelCalcResidualPostfix(struct Instance *i, int *res, int perturb,
                          struct gl_list_t *instances)
{
  struct logrelation *lrel;
  enum Expr_enum lrelop;
  unsigned long length_lhs, length_rhs;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualPostfix: null instance\n");
    return 1;
  }
  if (res == NULL){
    FPRINTF(ASCERR,"error in LogRelCalcResidualPostfix: null ptr\n");
    return 1;
  }
  if( InstanceKind(i) != LREL_INST ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualPostfix: not logrelation\n");
    return 1;
  }
#endif
  lrel = (struct logrelation *)GetInstanceLogRel(i);
  if( lrel == NULL ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualPostfix: null logrelation\n");
    return 1;
  }

  lrelop = LogRelRelop(lrel);
  length_lhs = LogRelLength(lrel,1);
  length_rhs = LogRelLength(lrel,0);

  if( length_lhs > 0 ) {
    length_lhs--;
    *res = LogRelEvaluatePostfixBranch(lrel,&length_lhs,1,perturb,instances);
  }
  else {
    *res = 0;
  }

  if( lrelop == e_boolean_eq ) {
    if( length_rhs > 0 ) {
      length_rhs--;
      if (*res == LogRelEvaluatePostfixBranch(lrel, &length_rhs,
                                              0,perturb,instances)) {
        *res = 1;
      }
      else {
        *res = 0;
      }
    }
    return 0;
  }
  else {
    if(lrelop == e_boolean_neq) {
      if( length_rhs > 0 ) {
        length_rhs--;
        if (*res != LogRelEvaluatePostfixBranch(lrel,&length_rhs,
                                                0,perturb,instances)) {
          *res = 1;
        }
        else {
          *res = 0;
        }
      }
      return 0;
    }
    else {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix:\n");
    FPRINTF(ASCERR, "wrong logical relation operator \n");
    return 1;
    }
  }
}

int
LogRelCalcResidualInfix(struct Instance *i, int *res, int perturb,
                        struct gl_list_t *instances)
{
  enum Expr_enum lrelop;
  glob_lrel = NULL;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualInfix: null instance\n");
    return 1;
  }
  if (res == NULL){
    FPRINTF(ASCERR,"error in LogRelCalcResidualInfix: null ptr\n");
    return 1;
  }
  if( InstanceKind(i) != LREL_INST ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualInfix: not logrelation\n");
    return 1;
  }
#endif
  glob_lrel = (struct logrelation *)GetInstanceLogRel(i);
  if( glob_lrel == NULL ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualInfix: null logrelation\n");
    return 1;
  }
  lrelop = LogRelRelop(glob_lrel);

  if(Infix_Log_LhsSide(glob_lrel) != NULL) {
    *res = LogRelBranchEvaluator(Infix_Log_LhsSide(glob_lrel),
                                 perturb,instances);
  } else {
     *res = 0;
  }

  if( lrelop == e_boolean_eq ) {
    if(Infix_Log_RhsSide(glob_lrel) != NULL) {
      if (*res == LogRelBranchEvaluator(Infix_Log_RhsSide(glob_lrel),
                                        perturb,instances)){
        *res = 1;
      }
      else {
        *res = 0;
      }
    }
    glob_lrel = NULL;
    return 0;
  }
  else if (lrelop == e_boolean_neq) {
    if(Infix_Log_RhsSide(glob_lrel) != NULL) {
      if (*res != LogRelBranchEvaluator(Infix_Log_RhsSide(glob_lrel),
                                        perturb,instances)){
        *res = 1;
      }
      else {
        *res = 0;
      }
    }
    glob_lrel = NULL;
    return 0;
  } else {
    FPRINTF(ASCERR, "error in LogRelCalcResidualInfix:\n");
    FPRINTF(ASCERR, "wrong logical relation operator \n");
    glob_lrel = NULL;
    return 1;
  }
}


/* LogRelCalcResidualPostfix2
 * Yes, yet another function to calculate the logical residual
 * It is not used anywhere. It does take in account the perturb
 * flag of the other routine calculations.
 */
int
LogRelCalcResidualPostfix2(struct Instance *i,int *res, int perturb,
                           struct gl_list_t *instances)
{
  struct logrelation *lrel;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualPostfix2: null instance\n");
    return 1;
  }
  if( res == NULL ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualPostfix2: null ptr\n");
    return 1;
  }
  if( InstanceKind(i) != LREL_INST ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualPostfix2: not logrelation\n");
    return 1;
  }
#endif
  lrel = (struct logrelation *)GetInstanceLogRel(i);
  if( lrel == NULL ) {
    FPRINTF(ASCERR, "error in LogRelCalcResidualPostfix2: null logrelation\n");
    return 1;
  }

  *res = LogRelEvaluateResidualPostfix(lrel,perturb,instances);
  return 0;
}


/**
 *** The following functions support LogRelFindBoolValues which
 *** is the compiler implementation of DirectSolve for Logical
 *** relations.
 *** These functions can be catagorized as follows:
 *** Memory Management and Copying functions:
 ***      LogRelCreateTmp, LogRelDestroyTmp, LogRelTmpCopySide,
 ***      LogRelTmpTokenCopy, append_bool_soln
 *** Eternal Function:
 ***      LogRelFindBoolValues
 **/

/*************************************************************************\
           Memory Management and Copying Functions
\*************************************************************************/

/*
 * structure dynamically allocated/reallocated to store the number of
 * boolean solutions
 */

struct ds_boolsol_list {
   int length,capacity;
   int *bool_soln;
};


#define alloc_bool_array(nbools,type)   \
   ((nbools) > 0 ? (type *)ascmalloc((nbools)*sizeof(type)) : NULL)
#define copy_bool_value(from,too,nvalues)  \
   asc_memcpy((from),(too),(nvalues)*sizeof(int))

/*
 * Appends a case_number onto the list
 */
static void append_bool_soln( struct ds_boolsol_list *cl, int bool_soln)
{
   if( cl->length == cl->capacity ) {
      int newcap;
      int *newlist;
      newcap = cl->capacity + 10;
      newlist = alloc_bool_array(newcap,int);
      copy_bool_value((char *)cl->bool_soln,(char *)newlist,cl->length);
      if( cl->bool_soln != NULL )
	 ascfree(cl->bool_soln);
      cl->bool_soln = newlist;
      cl->capacity = newcap;
   }
   cl->bool_soln[cl->length++] = bool_soln;
}


/**
 *** LogRelCreateTmp creates a struct logrelation
 *** and passes back a pointer to the logrelation. The lengths of
 *** the right and left sides (lhslen and rhslen) of the logrelation
 *** are supplied by the calling function.
 **/
static struct logrelation *LogRelCreateTmp(int lhslen, int rhslen)
{
  struct logrelation *lrel;
  lrel = CreateLogRelStructure(e_bol_token);
  lrel->token.lhs = (union LogRelTermUnion *)
	ascmalloc(lhslen*sizeof(union LogRelTermUnion));
  lrel->token.rhs = (union LogRelTermUnion *)
	ascmalloc(rhslen*sizeof(union LogRelTermUnion));
  return lrel;
}

/**
 *** LogRelDestroyTmp deallocates a struct logrelation.
 *** The calling function should provide a pointer
 *** to the logrelation to be destroyed.
 **/
static void LogRelDestroyTmp(struct logrelation *lrel)
{
  if (!lrel) return;
  if (lrel->token.lhs!=NULL) ascfree(lrel->token.lhs);
  if (lrel->token.rhs!=NULL) ascfree(lrel->token.rhs);
  ascfree((char *)lrel);
}


/*
 * We can now just do a memcopy and the infix pointers
 * all adjust by the difference between the token
 * arrays that the gl_lists are hiding.
 * Note, if any turkey ever tries to delete an individual
 * token from these gl_lists AND deallocate it,
 * they will get a severe headache.
 *
 * This is a full blown copy and not copy by reference.
 * You do not need to remake the infix pointers after
 * calling this function.
 */
static int LogRelTmpCopySide(union LogRelTermUnion *old, unsigned long len,
			     union LogRelTermUnion *arr)
{
  struct logrel_term *term;
  unsigned long c;
  long int delta;

  if (old==NULL || !len) return 1;
  if (arr==NULL) {
    FPRINTF(ASCERR,"LogRelTmpCopySide: null LogRelTermUnion :-(.\n");
    return 1;
  }
  memcpy( (VOIDPTR)arr, (VOIDPTR)old, len*sizeof(union LogRelTermUnion));
 /*
  *  Difference in chars between old and arr ptrs. It should be a multiple
  *  of sizeof(double) but may not be a multiple of sizeof(union RTU).
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
    case e_satisfied:
      break;
    case e_boolean_eq: case e_boolean_neq:
    default:
      Asc_Panic(2, "LogRelTmpCopySide",
                "Unknown term type in LogRelTmpCopySide\n");
    }
  }
#undef ADJLOGPTR

  return 0;
}

static
struct logrelation *LogRelTmpTokenCopy(CONST struct logrelation *src)
{
  struct logrelation *result;
  long int delta;
  assert(src!=NULL);

  result = LogRelCreateTmp(src->token.lhs_len,src->token.rhs_len);

  if(LogRelTmpCopySide(src->token.lhs,src->token.lhs_len,
			result->token.lhs) == 0) {
    delta = LOGUNION_TERM(src->token.lhs_term) - src->token.lhs;
    result->token.lhs_term = LOGA_TERM(result->token.lhs+delta);
    result->token.lhs_len = src->token.lhs_len;
  } else {
    result->token.lhs_term = NULL;
    result->token.lhs_len = 0;
  }

  if(LogRelTmpCopySide(src->token.rhs,src->token.rhs_len,
			  result->token.rhs) == 0) {
    delta = LOGUNION_TERM(src->token.rhs_term) - src->token.rhs;
    result->token.rhs_term = LOGA_TERM(result->token.rhs+delta);
    result->token.rhs_len = src->token.rhs_len;
  } else {
    result->token.rhs_term = NULL;
    result->token.rhs_len = 0;
  }

  result->bvars = src->bvars;
  result->satrels = src->satrels;
  result->logresidual = src->logresidual;
  result->lognominal = src->lognominal;
  return result;
}


/*************************************************************************\
                               External Function
\*************************************************************************/

/**
 *** LogRelFindBoolValues WILL find a boolean value if there is one.
 *** It returns 1 for success and 0 for failure. In general compiler
 *** functions return 0 for success but this function returns 1 for
 *** success because success = 1 is the convention on the solver side.
 *** A return of -1 indicates a problem such as var not found.
 *** If nsolns > 1 then a list of solutions will be returned.
 *** For logical equations, at most two values are expected
 **/

/* Note we should recycle the memory used for glob_lrel */

int *LogRelFindBoolValues(struct Instance *i, unsigned long *dvarnum,
                          int *able,int *nsolns, int perturb,
                          struct gl_list_t *instances)
{
  struct logrelation *lrel;
  struct ds_boolsol_list soln_list;
  CONST struct gl_list_t *list;
  int  res,status,bvalue;

  soln_list.length = soln_list.capacity = 0;
  soln_list.bool_soln = NULL;
  append_bool_soln(&soln_list,0);

  *able = FALSE;
  *nsolns = -1;     /* nsolns will be -1 for a very unhappy rootfinder */
  glob_lrel = NULL;


#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in LogRelFindBoolValues: null instance\n");
    glob_lrel = NULL; return NULL;
  }
  if (able == NULL){
    FPRINTF(ASCERR,"error in LogRelFindBoolValues: null int ptr\n");
    glob_lrel = NULL; return NULL;
  }
  if (dvarnum == NULL){
    FPRINTF(ASCERR,"error in LogRelFindBoolValues: null dvarnum\n");
    glob_lrel = NULL; return NULL;
  }
  if( InstanceKind(i) != LREL_INST ) {
    FPRINTF(ASCERR, "error in LogRelFindBoolValues: not logrelation\n");
    glob_lrel = NULL; return NULL;
  }
#endif
  lrel = (struct logrelation *)GetInstanceLogRel(i);
  if( lrel == NULL ) {
    FPRINTF(ASCERR, "error in LogRelFindBoolValues: null logrelation\n");
    glob_lrel = NULL; return NULL;
  }

  glob_lrel = LogRelTmpTokenCopy(lrel);
  assert(glob_lrel!=NULL);
  list = LogRelBoolVarList(glob_lrel);

  if(!(*dvarnum >= 1 && *dvarnum <= gl_length(list))){
    FPRINTF(ASCERR, "Error in LogRelFindBoolValues: dvar not found\n");
    glob_lrel = NULL;
    return NULL;
  }

  /* Current value  */
  bvalue = GetBooleanAtomValue((struct Instance *)
                               gl_fetch(LogRelBoolVarList(glob_lrel),*dvarnum));
  status = 1;

  /* Assign the value TRUE to the boolean */
  SetBooleanAtomValue(
     (struct Instance *)gl_fetch(LogRelBoolVarList(glob_lrel),*dvarnum),TRUE,0);

  /* Check Residual  */
  status = LogRelCalcResidualPostfix(i,&res,perturb,instances);
  if (!status) {
    if (res) {
      *nsolns = 1;
      append_bool_soln(&soln_list,1);
    }

    /* Assign the value FALSE to the boolean */
    SetBooleanAtomValue(
     (struct Instance *)gl_fetch(LogRelBoolVarList(glob_lrel),*dvarnum),FALSE,0);

    /* Check Residual  */
    status = LogRelCalcResidualPostfix(i,&res,perturb,instances);

    if (!status) {
      if (res) {
        if ((*nsolns)== 1) {
          *nsolns = 2;
        } else {
          *nsolns = 1;
        }
        append_bool_soln(&soln_list,0);
      }
    }
    /* Assign original boolean value */
    SetBooleanAtomValue(
    (struct Instance *)gl_fetch(LogRelBoolVarList(glob_lrel),*dvarnum),bvalue,0);

  }else {
    *able = FALSE;
    LogRelDestroyTmp(glob_lrel);
    return soln_list.bool_soln;
  }
  *able = TRUE;
  LogRelDestroyTmp(glob_lrel);
  return soln_list.bool_soln;
}


/*
 *  Temporary Functions for testing direct solve.
 *  Remove calls from interface.c when this is removed.
 */
void PrintDirectBooleanResult(struct Instance *i)
{
  struct logrelation *lrel;
  int num,status,n,nsoln;
  int *soln_list;
  unsigned long dvarnum;
  CONST struct gl_list_t *list;

  if (InstanceKind(i) == LREL_INST) {
    lrel = (struct logrelation *)GetInstanceLogRel(i);
    list = LogRelBoolVarList(lrel);
    for(num = 1; num <= (int)gl_length(list);++num) {
      status = -1;
      dvarnum = num;
      soln_list = LogRelFindBoolValues(i,&(dvarnum),&(status),&(nsoln),0,NULL);
      if (nsoln > 0) {
        FPRINTF(stderr,"VAR NUMBER %d\n",num);
        for(n =1; n<=nsoln;n++) {
          FPRINTF(stderr,"SOLUTION = %d\n",soln_list[n]);
        }
      }
    }
  }
}

void PrintDirectSolveBooleanSolutions(struct Instance *i)
{
  VisitInstanceTree(i,PrintDirectBooleanResult, 0, 0);
}

