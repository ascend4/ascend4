/*	ASCEND modelling environment
	Copyright (C) 2013 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Event list and utility and output routines.

*//*
	Created: 19/06/2013
*/

#include <stdio.h>
#include <assert.h>

#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>

#include "functype.h"
#include "expr_types.h"
#include "stattypes.h"
#include "sets.h"
#include "exprs.h"
#include "slist.h"
#include "vlist.h"
#include "instance_types.h"
#include "rel_common.h"
#include "mathinst.h"
#include "instance_io.h"
#include "setio.h"
#include "exprio.h"
#include "event.h"

#define EMALLOC(x) x = ASC_NEW(struct EventList)

/* Event list functions */

struct EventList *CreateEvent(struct Set *set, struct StatementList *sl)
{
  register struct EventList *result;
  EMALLOC(result);
  result->slist = sl;
  result->values =set;
  result->next = NULL;
  return result;
}

struct EventList *ReverseEventCases(struct EventList *e)
{
  register struct EventList *next,*previous=NULL;
  if (e==NULL) return e;
  while (1) {			/* loop until broken */
    next = e->next;
    e->next = previous;
    if (next==NULL) return e;
    previous = e;
    e = next;
  }
}

struct EventList *LinkEventCases(struct EventList *e1, struct EventList *e2)
{
  register struct EventList *p;
  p = e1;
  while (p->next!=NULL) p = p->next;
  p->next = e2;
  return e1;
}

struct EventList *NextEventCaseF(struct EventList *e)
{
  assert(e!=NULL);
  return e->next;
}

struct Set *EventSetListF(struct EventList *e)
{
  assert(e!=NULL);
  return e->values;
}


struct StatementList *EventStatementListF(struct EventList *e)
{
 assert(e!=NULL);
 return e->slist;
}

void DestroyEventNode(struct EventList *e)
{
  register struct Set *set;
  if (e!=NULL){
    set = e->values;
    if (e->values) {
      if (set->next == NULL) {
        DestroySetNodeByReference(e->values);
      }
      else {
        DestroySetListByReference(e->values);
      }
    }
    DestroyStatementList(e->slist);
    e->next = NULL;
    ascfree((char *)e);
  }
}

void DestroyEventList(struct EventList *e)
{
  register struct EventList *next;
  while (e!=NULL){
    next = NextEventCase(e);
    DestroyEventNode(e);
    e = next;
  }
}

struct EventList *CopyEventNode(struct EventList *e)
{
  register struct EventList *result;
  assert(e!=NULL);
  EMALLOC(result);
  if (e->values) result->values = CopySetByReference(e->values);
  else result->values = e->values;
  result->slist = CopyListToModify(e->slist);
  result->next = NULL;
  return result;
}

struct EventList *CopyEventList(struct EventList *e)
{
  register struct EventList *head=NULL,*p;
  if (e!=NULL) {
    p = head = CopyEventNode(e);
    e = e->next;
    while(e!=NULL){
      p->next = CopyEventNode(e);
      p = p->next;
      e = e->next;
    }
  }
  return head;
}

/* Event utility functions */

void ModifyEventPointers(struct gl_list_t *reforvar,
		 	 CONST struct Instance *old,
			 CONST struct Instance *new)
{
  unsigned long pos,other;
  assert(reforvar!=NULL);

  if (old==new) return;
  if (new){
    if (0 != (pos = gl_search(reforvar,old,(CmpFunc)CmpP))) {
      if (0 != (other = gl_search(reforvar,new,(CmpFunc)CmpP))){
	gl_store(reforvar,pos,(VOIDPTR)new);	       /* case 3 */
        if (pos < other) Swap(&pos,&other);
        /* pos > other now */
        gl_delete(reforvar,pos,0);
      }
      else
	gl_store(reforvar,pos,(char *)new);            /* case 2 */
    }
    else{					       /* case 1 */
      FPRINTF(ASCERR,"Warning ModifiyEventPointers not found.\n");
      FPRINTF(ASCERR,"This shouldn't effect your usage at all.\n");
    }
  }
  else						       /* case 4 */
    if (0 != (pos = gl_search(reforvar,old,(CmpFunc)CmpP)))
      gl_store(reforvar,pos,(VOIDPTR)new);
}

struct gl_list_t *CopyEventBVarList(struct Instance *dest_inst,
			  	    struct gl_list_t *copylist)
{
  struct Instance *bvar;
  struct gl_list_t *newbvarlist = NULL;
  unsigned long len,c,pos;

  if (copylist) {
    len = gl_length(copylist);
    newbvarlist = gl_create(len);
    for (c=1;c<=len;c++) {
      bvar = (struct Instance *)gl_fetch(copylist,c);
      pos = gl_search(newbvarlist,bvar,(CmpFunc)CmpP);
      if (pos) {
	ASC_PANIC("Corrupted variable list in CopyEventBVarList\n");
      }
      gl_append_ptr(newbvarlist,(VOIDPTR)bvar);
      switch(bvar->t){
        case BOOLEAN_ATOM_INST:
        case INTEGER_ATOM_INST:
        case SYMBOL_ATOM_INST:
        case BOOLEAN_CONSTANT_INST:
        case INTEGER_CONSTANT_INST:
        case SYMBOL_CONSTANT_INST:
          AddEvent(bvar,dest_inst);
          break;
        default:
          FPRINTF(ASCERR,"Incorrect instance in Event Variable list\n");
      }
    }
  }
  else{	/* we will always return a varlist, even if empty */
    newbvarlist = gl_create(1L);
  }
  return newbvarlist;
}

void DestroyEventVarList(struct gl_list_t *l, struct Instance *inst)
{
  register struct Instance *ptr;
  register unsigned long c;
  for(c=gl_length(l);c>=1;c--){
    if (NULL != (ptr = (struct Instance *)gl_fetch(l,c))){
      switch(ptr->t) {
      case BOOLEAN_ATOM_INST:
      case INTEGER_ATOM_INST:
      case SYMBOL_ATOM_INST:
      case BOOLEAN_CONSTANT_INST:
      case INTEGER_CONSTANT_INST:
      case SYMBOL_CONSTANT_INST:
        RemoveEvent(ptr,inst);
        break;
      default:
        FPRINTF(ASCERR,"Incorrect instance in EventVarList\n");
      }
    }
  }
  gl_destroy(l);
}


static
void DestroyEventCases(struct Case *cur_case, struct Instance *inst)
{
  register struct Instance *ptr;
  register unsigned long c,len;
  struct gl_list_t *reflist;

  reflist = GetCaseReferences(cur_case);
  assert(reflist!=NULL);
  len = gl_length(reflist);

  for(c=len;c>=1;c--){
     if (NULL != (ptr = (struct Instance *)gl_fetch(reflist,c))){
      switch(ptr->t) {
      case MODEL_INST:
      case REL_INST:
      case LREL_INST:
      case EVENT_INST:
        RemoveEvent(ptr,inst);
        break;
      default:
        FPRINTF(ASCERR,"Incorrect instance in DestroyEventCases\n");
      }
    }
  }
  DestroyCase(cur_case);
}


void DestroyEventCaseList(struct gl_list_t *l, struct Instance *inst)
{
  struct Case *cur_case;
  register unsigned long c,len;
  len = gl_length(l);

  for(c=len;c>=1;c--){
    cur_case = (struct Case *)gl_fetch(l,c);
    DestroyEventCases(cur_case,inst);
  }
  gl_destroy(l);
}

/* Event output routines */

static
void WriteCase(FILE *f, struct Case *cur_case, CONST struct Instance *ref)
{
  struct Set *values;
  struct gl_list_t *reflist;
  struct Instance *inst;
  unsigned long len,c;

  values = GetCaseValues(cur_case);
  FPRINTF(f,"  ");
  if (values!=NULL){
    FPRINTF(f,"CASE ");
    WriteSet(f,values);
  }
  else FPRINTF(f,"OTHERWISE");
  FPRINTF(f," :\n");

  reflist = GetCaseReferences(cur_case);
  len = gl_length(reflist);
  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(reflist,c);
    FPRINTF(f,"      ");
    WriteInstanceName(f,inst,ref);
    FPRINTF(f,"; \n");
  }
}


void WriteEvent(FILE *f, CONST struct Instance *eventinst,
	        CONST struct Instance *ref)
{
  struct gl_list_t *whens;
  struct gl_list_t *events;
  struct gl_list_t *vars;
  struct gl_list_t *cases;
  struct Case *cur_case;
  struct Instance *inst;
  unsigned long len,c;

  whens = GetInstanceWhens(eventinst);
  events = GetInstanceEvents(eventinst);
  vars = GetInstanceEventVars(eventinst);
  cases = GetInstanceEventCases(eventinst);

  FPRINTF(f,"NAME :");
  WriteInstanceName(f,eventinst,ref);
  FPRINTF(f,"\n");
  FPRINTF(f,"\n");
  FPRINTF(f,"EVENT  ");
  if (vars!=NULL) {
    len = gl_length(vars);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(vars,c);
      WriteInstanceName(f,inst,ref);
      if (c<len) {
        FPRINTF(f,",  ");
      }
      if (c==len) {
        FPRINTF(f," \n");
      }
    }
  }
  if (cases!=NULL) {
    len = gl_length(cases);
    for (c=1;c<=len;c++) {
      cur_case = (struct Case *)gl_fetch(cases,c);
      WriteCase(f,cur_case,ref);
    }
    FPRINTF(f,"END ");
    FPRINTF(f,"\n");
  }
  FPRINTF(f,"\n");
  if (whens!=NULL) {
    len = gl_length(whens);
    FPRINTF(f,"USED IN CASES OF WHENS: ");
    FPRINTF(f," \n");
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(whens,c);
      FPRINTF(f,"      ");
      WriteInstanceName(f,inst,ref);
      FPRINTF(f," \n");
    }
  }
  if (events!=NULL) {
    len = gl_length(events);
    FPRINTF(f,"USED IN CASES OF EVENTS: ");
    FPRINTF(f," \n");
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(events,c);
      FPRINTF(f,"      ");
      WriteInstanceName(f,inst,ref);
      FPRINTF(f," \n");
    }
  }
}


static
void WriteCaseDS(Asc_DString *dsPtr, struct Case *cur_case,
		 CONST struct Instance *ref)
{
  struct Set *values;
  struct gl_list_t *reflist;
  struct Instance *inst;
  unsigned long len,c;

  values = GetCaseValues(cur_case);
  Asc_DStringAppend(dsPtr,"  ",2);
  if (values!=NULL){
    Asc_DStringAppend(dsPtr,"CASE ",5);
    WriteSet2Str(dsPtr,values);
  }
  else Asc_DStringAppend(dsPtr,"OTHERWISE",9);
  Asc_DStringAppend(dsPtr," : ",2);
  Asc_DStringAppend(dsPtr,"\n",-1);

  reflist = GetCaseReferences(cur_case);
  len = gl_length(reflist);
  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(reflist,c);
    Asc_DStringAppend(dsPtr,"      ",6);
    WriteInstanceNameDS(dsPtr,inst,ref);
    Asc_DStringAppend(dsPtr,";",1);
    Asc_DStringAppend(dsPtr,"\n",-1);
  }
}

static
void WriteEventDS(Asc_DString *dsPtr, CONST struct Instance *eventinst,
	          CONST struct Instance *ref)
{
  struct gl_list_t *whens;
  struct gl_list_t *events;
  struct gl_list_t *vars;
  struct gl_list_t *cases;
  struct Case *cur_case;
  struct Instance *inst;
  unsigned long len,c;

  whens = GetInstanceWhens(eventinst);
  events = GetInstanceEvents(eventinst);
  vars = GetInstanceEventVars(eventinst);
  cases = GetInstanceEventCases(eventinst);

  Asc_DStringAppend(dsPtr,"NAME :",6);
  WriteInstanceNameDS(dsPtr,eventinst,ref);
  Asc_DStringAppend(dsPtr,"\n",-1);
  Asc_DStringAppend(dsPtr,"\n",-1);

  Asc_DStringAppend(dsPtr,"EVENT ",5);
  if (vars!=NULL) {
    len = gl_length(vars);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(vars,c);
      WriteInstanceNameDS(dsPtr,inst,ref);
      if (c<len) {
	Asc_DStringAppend(dsPtr,",  ",3);
      }
      if (c==len) {
	Asc_DStringAppend(dsPtr," ",1);
        Asc_DStringAppend(dsPtr,"\n",-1);
      }
    }
  }
  if (cases!=NULL) {
    len = gl_length(cases);
    for (c=1;c<=len;c++) {
      cur_case = (struct Case *)gl_fetch(cases,c);
      WriteCaseDS(dsPtr,cur_case,ref);
    }
    Asc_DStringAppend(dsPtr,"END ",4);
    Asc_DStringAppend(dsPtr,"\n",-1);
  }
  Asc_DStringAppend(dsPtr,"\n",-1);
  if (whens!=NULL) {
    len = gl_length(whens);
    Asc_DStringAppend(dsPtr,"USED IN CASES OF WHENS: ",18);
    Asc_DStringAppend(dsPtr,"\n",-1);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(whens,c);
      Asc_DStringAppend(dsPtr,"      ",6);
      WriteInstanceNameDS(dsPtr,inst,ref);
       Asc_DStringAppend(dsPtr," ",1);
       Asc_DStringAppend(dsPtr,"\n",-1);
    }
  }
  if (events!=NULL) {
    len = gl_length(events);
    Asc_DStringAppend(dsPtr,"USED IN CASES OF EVENTS: ",18);
    Asc_DStringAppend(dsPtr,"\n",-1);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(events,c);
      Asc_DStringAppend(dsPtr,"      ",6);
      WriteInstanceNameDS(dsPtr,inst,ref);
       Asc_DStringAppend(dsPtr," ",1);
       Asc_DStringAppend(dsPtr,"\n",-1);
    }
  }
}


char *WriteEventString(CONST struct Instance *eventinst,
                       CONST struct Instance *ref)
{
  static Asc_DString ds;
  Asc_DString *dsPtr;
  char *result;
  result = ASC_NEW_ARRAY(char,15);
  if (result == NULL) {
    FPRINTF(stderr,"Memory error in WriteEventString\n");
    return result;
  }
  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  WriteEventDS(dsPtr,eventinst,ref);
  result = Asc_DStringResult(dsPtr);
  Asc_DStringFree(dsPtr);
  return result;

}


