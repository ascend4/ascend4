/*
 *  WHEN List Implementation
 *  by Vicente Rico-Ramirez
 *  7/96
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: when.c,v $
 *  Date last modified: $Date: 1997/07/29 15:52:56 $
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
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/sets.h"
#include "compiler/exprs.h"
#include "compiler/slist.h"
#include "compiler/vlist.h"
#include "compiler/when.h"

#define WMALLOC(x) x = (struct WhenList *)ascmalloc(sizeof(struct WhenList))

#ifndef lint
static CONST char WhenRCSid[] = "$Id: when.c,v 1.11 1997/07/29 15:52:56 rv2a Exp $";
#endif


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
int SetNodeIsTrue(struct Set *s)
{
  CONST struct Expr *e;
  assert(s && (SetType(s)==0));
  e = GetSingleExpr(s);
  assert(e && (ExprType(e)==e_boolean) && (NextExpr(e)==NULL));
  return ExprBValue(e);
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


struct WhenList *CreateWhen(struct Set *set, struct StatementList *sl)
{
  register struct WhenList *result;
  WMALLOC(result);
  result->slist = sl;
  result->values =set;
  result->next = NULL;
  return result;
}

struct WhenList *ReverseWhenCases(struct WhenList *w)
{
  register struct WhenList *next,*previous=NULL;
  if (w==NULL) return w;
  while (1) {			/* loop until broken */
    next = w->next;
    w->next = previous;
    if (next==NULL) return w;
    previous = w;
    w = next;
  }
}

struct WhenList *LinkWhenCases(struct WhenList *w1, struct WhenList *w2)
{
  register struct WhenList *p;
  p = w1;
  while (p->next!=NULL) p = p->next;
  p->next = w2;
  return w1;
}

struct WhenList *NextWhenCaseF(struct WhenList *w)
{
  assert(w!=NULL);
  return w->next;
}

struct Set *WhenSetListF(struct WhenList *w)
{
  assert(w!=NULL);
  return w->values;
}


struct StatementList *WhenStatementListF(struct WhenList *w)
{
 assert(w!=NULL);
 return w->slist;
}

void DestroyWhenNode(struct WhenList *w)
{
  register struct Set *set;
  if (w!=NULL){
    set = w->values;
    if (w->values) {
      if (set->next == NULL) {
        DestroySetNodeByReference(w->values);
      }
      else {
        DestroySetListByReference(w->values);
      }
    }
    DestroyStatementList(w->slist);
    w->next = NULL;
    ascfree((char *)w);
  }
}

void DestroyWhenList(struct WhenList *w)
{
  register struct WhenList *next;
  while (w!=NULL){
    next = NextWhenCase(w);
    DestroyWhenNode(w);
    w = next;
  }
}

struct WhenList *CopyWhenNode(struct WhenList *w)
{
  register struct WhenList *result;
  assert(w!=NULL);
  WMALLOC(result);
  if (w->values) result->values = CopySetByReference(w->values);
  else result->values = w->values;
  result->slist = CopyListToModify(w->slist);
  result->next = NULL;
  return result;
}

struct WhenList *CopyWhenList(struct WhenList *w)
{
  register struct WhenList *head=NULL,*p;
  if (w!=NULL) {
    p = head = CopyWhenNode(w);
    w = w->next;
    while(w!=NULL){
      p->next = CopyWhenNode(w);
      p = p->next;
      w = w->next;
    }
  }
  return head;
}









