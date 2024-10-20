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
#include "sets.h"
#include "case.h"

/********************************************************************\
                        Case processing
\********************************************************************/



struct Case *CreateCase(struct Set *vl, struct gl_list_t *refinst){
  struct Case *result = ASC_NEW(struct Case);
  assert(result!=NULL);
  result->ValueList = vl;
  result->ref = refinst;
  result->active = 0;
  return result;
}


struct Set *GetCaseValuesF(struct Case *c){
  assert(c);
  return c->ValueList;
}


struct gl_list_t *GetCaseReferencesF(struct Case *c){
  assert(c);
  return c->ref;
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

struct Case *SetCaseReferences(struct Case *c, struct gl_list_t *refinst){
  assert(c);
  c->ref = refinst;
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
    gl_destroy(c->ref);
    c->active = 0;
    ASC_FREE(c);
  }
}


struct Case *CopyCase(struct Case *c){
  assert(c);
  struct Case *result = ASC_NEW(struct Case);
  if (c->ValueList) result->ValueList = CopySetByReference(c->ValueList);
  else result->ValueList = c->ValueList;
  result->ref = gl_copy(c->ref);
  result->active = c->active;
  return result;
}

/* vim: set noai ts=8 sw=2 et: */

