/*
 *  Case Processing Functions
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: case.c,v $
 *  Date last modified: $Date: 1997/07/18 12:28:11 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include<stdio.h>
#include<assert.h>
#include <ascend/general/platform.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>


#include "functype.h"
#include "expr_types.h"
#include "exprs.h"
#include "sets.h"
#include "case.h"
#include "statement.h"

/********************************************************************\
                        Case processing
\********************************************************************/



struct Case *CreateCase(struct Set *vl, struct gl_list_t *refinst){
  struct Case *result = ASC_NEW(struct Case);
  assert(result!=NULL);
  result->ValueList = vl;
  result->otherwise_label = NULL;
  result->condition = NULL;
  result->applies = NULL;
  result->ref = refinst;
  result->reinit = NULL;
  result->mod = NULL;
  result->linenum = 0;
  result->active = 0;
  return result;
}


struct Set *GetCaseValuesF(struct Case *c){
  assert(c);
  return c->ValueList;
}

symchar *GetCaseOtherwiseLabelF(struct Case *c){
  assert(c);
  return c->otherwise_label;
}

struct Expr *GetCaseConditionF(struct Case *c){
  assert(c);
  return c->condition;
}

struct Expr *GetCaseAppliesF(struct Case *c){
  assert(c);
  return c->applies;
}

struct gl_list_t *GetCaseReferencesF(struct Case *c){
  assert(c);
  return c->ref;
}

struct gl_list_t *GetCaseReinitStatementsF(struct Case *c){
  assert(c);
  return c->reinit;
}

struct module_t *GetCaseModuleF(struct Case *c){
  assert(c);
  return c->mod;
}

unsigned long GetCaseLineNumF(struct Case *c){
  assert(c);
  return c->linenum;
}


int GetCaseStatusF(struct Case *c){
  assert(c);
  return c->active;
}

struct Case *SetCaseValues(struct Case *c, struct Set *vl){
  assert(c);
  c->ValueList = vl;
  return c;
}

struct Case *SetCaseOtherwiseLabel(struct Case *c, symchar *label){
  assert(c);
  c->otherwise_label = label;
  return c;
}

struct Case *SetCaseCondition(struct Case *c, struct Expr *condition){
  assert(c);
  c->condition = condition;
  return c;
}

struct Case *SetCaseApplies(struct Case *c, struct Expr *applies){
  assert(c);
  c->applies = applies;
  return c;
}

struct Case *SetCaseReferences(struct Case *c, struct gl_list_t *refinst){
  assert(c);
  c->ref = refinst;
  return c;
}

struct Case *SetCaseReinitStatements(struct Case *c, struct gl_list_t *reinit){
  assert(c);
  c->reinit = reinit;
  return c;
}

struct Case *SetCaseSource(struct Case *c,
                           struct module_t *mod,
                           unsigned long linenum){
  assert(c);
  c->mod = mod;
  c->linenum = linenum;
  return c;
}

struct Case *SetCaseStatus(struct Case *c, int setact){
  assert(c);
  c->active = setact;
  return c;
}


unsigned long NumberCaseRefs(struct Case *c){
  struct gl_list_t *refs;
  unsigned long n;
  refs = GetCaseReferences(c);
  n = (refs!=NULL) ? gl_length(refs) : 0;
  return n;
}


struct Instance *CaseRef(struct Case *c, unsigned long int refnum){
  struct gl_list_t *refs;
  refs = GetCaseReferences(c);
  return (struct Instance *)gl_fetch(refs,refnum);
}


void DestroyCase(struct Case *c){
  if(c!=NULL){
    struct Set *set = c->ValueList;
    if(set) {
      if(set->next== NULL) {
        DestroySetNodeByReference(set);
      }else{
        DestroySetListByReference(set);
      }
    }
    if(c->condition != NULL){
      DestroyExprList(c->condition);
      c->condition = NULL;
    }
    if(c->applies != NULL){
      DestroyExprList(c->applies);
      c->applies = NULL;
    }
    gl_destroy(c->ref);
    if(c->reinit != NULL){
      unsigned long i, len = gl_length(c->reinit);
      for(i = 1; i <= len; ++i){
        struct Statement *stat = (struct Statement *)gl_fetch(c->reinit, i);
        if(stat != NULL){
          DestroyStatement(stat);
        }
      }
      gl_destroy(c->reinit);
      c->reinit = NULL;
    }
    c->active = 0;
    ASC_FREE(c);
  }
}


struct Case *CopyCase(struct Case *c){
  assert(c);
  struct Case *result = ASC_NEW(struct Case);
  if (c->ValueList) result->ValueList = CopySetByReference(c->ValueList);
  else result->ValueList = c->ValueList;
  result->otherwise_label = c->otherwise_label;
  result->condition = CopyExprList(c->condition);
  result->applies = CopyExprList(c->applies);
  result->ref = gl_copy(c->ref);
  result->mod = c->mod;
  result->linenum = c->linenum;
  if(c->reinit != NULL){
    unsigned long i, len = gl_length(c->reinit);
    result->reinit = gl_create(len);
    for(i = 1; i <= len; ++i){
      struct Statement *stat = (struct Statement *)gl_fetch(c->reinit, i);
      gl_append_ptr(result->reinit, CopyStatement(stat));
    }
  }else{
    result->reinit = NULL;
  }
  result->active = c->active;
  return result;
}

/* vim: set noai ts=8 sw=2 et: */
