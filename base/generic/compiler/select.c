/*
 *  SELECT List Implementation
 *  by Vicente Rico-Ramirez
 *  1/97
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: select.c,v $
 *  Date last modified: $Date: 1997/07/29 15:52:54 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Vicente Rico-Ramirez
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#include<stdio.h>
#include<assert.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/func.h"
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/select.h"
#include "compiler/sets.h"
#include "compiler/exprs.h"
#include "compiler/slist.h"

#define SELMALLOC(x) \
        (x) = (struct SelectList *)ascmalloc(sizeof(struct SelectList))

struct SelectList *CreateSelect(struct Set *set, struct StatementList *sl)
{
  register struct SelectList *result;
  SELMALLOC(result);
  result->slist = sl;
  result->values =set;
  result->next = NULL;
  return result;
}

struct SelectList *ReverseSelectCases(struct SelectList *sel)
{
  register struct SelectList *next,*previous=NULL;
  if (sel==NULL) return sel;
  while (1) {			/* loop until broken */
    next = sel->next;
    sel->next = previous;
    if (next==NULL) return sel;
    previous = sel;
    sel = next;
  }
}

struct SelectList *LinkSelectCases(struct SelectList *sel1,
				   struct SelectList *sel2)
{
  register struct SelectList *p;
  p = sel1;
  while (p->next!=NULL) p = p->next;
  p->next = sel2;
  return sel1;
}

struct SelectList *NextSelectCaseF(struct SelectList *sel)
{
  assert(sel);
  return sel->next;
}


struct Set *SelectSetListF(struct SelectList *sel)
{
  assert(sel);
  return sel->values;
}


struct StatementList *SelectStatementListF(struct SelectList *sel)
{
 assert(sel);
 return sel->slist;
}

void DestroySelectNode(struct SelectList *sel)
{
  register struct Set *set;
  if (sel!=NULL){
    set = sel->values;
    if (sel->values) {
      if (set->next == NULL) {
        DestroySetNodeByReference(sel->values);
      }
      else {
        DestroySetListByReference(sel->values);
      }
    }
    DestroyStatementList(sel->slist);
    sel->next = NULL;
    ascfree((char *)sel);
  }
}

void DestroySelectList(struct SelectList *sel)
{
  register struct SelectList *next;
  while (sel!=NULL){
    next = NextSelectCase(sel);
    DestroySelectNode(sel);
    sel = next;
  }
}

struct SelectList *CopySelectNode(struct SelectList *sel)
{
  register struct SelectList *result;
  assert(sel);
  SELMALLOC(result);
  if (sel->values) result->values = CopySetByReference(sel->values);
  else result->values = sel->values;
  result->slist = CopyListToModify(sel->slist);
  result->next = NULL;
  return result;
}

struct SelectList *CopySelectList(struct SelectList *sel)
{
  register struct SelectList *head=NULL,*p;
  if (sel!=NULL) {
    p = head = CopySelectNode(sel);
    sel = sel->next;
    while(sel!=NULL){
      p->next = CopySelectNode(sel);
      p = p->next;
      sel = sel->next;
    }
  }
  return head;
}






