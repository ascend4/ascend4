/*
 *  SWITCH List Implementation
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: switch.c,v $
 *  Date last modified: $Date: 1997/07/18 12:35:17 $
 *  Last modified by: $Author: mthomas $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "func.h"
#include "expr_types.h"
#include "stattypes.h"
#include "switch.h"
#include "sets.h"
#include "exprs.h"
#include "slist.h"
#define SWMALLOC(x) (x) = ASC_NEW(struct SwitchList)

struct SwitchList *CreateSwitch(struct Set *set, struct StatementList *sl)
{
  register struct SwitchList *result;
  SWMALLOC(result);
  result->slist = sl;
  result->values =set;
  result->next = NULL;
  return result;
}

struct SwitchList *ReverseSwitchCases(struct SwitchList *sw)
{
  register struct SwitchList *next,*previous=NULL;
  if (sw==NULL) return sw;
  while (1) {			/* loop until broken */
    next = sw->next;
    sw->next = previous;
    if (next==NULL) return sw;
    previous = sw;
    sw = next;
  }
}

struct SwitchList *LinkSwitchCases(struct SwitchList *sw1,
				   struct SwitchList *sw2)
{
  register struct SwitchList *p;
  p = sw1;
  while (p->next!=NULL) p = p->next;
  p->next = sw2;
  return sw1;
}

struct SwitchList *NextSwitchCaseF(struct SwitchList *sw)
{
  assert(sw);
  return sw->next;
}


struct Set *SwitchSetListF(struct SwitchList *sw)
{
  assert(sw);
  return sw->values;
}


struct StatementList *SwitchStatementListF(struct SwitchList *sw)
{
 assert(sw);
 return sw->slist;
}

void DestroySwitchNode(struct SwitchList *sw)
{
  register struct Set *set;
  if (sw!=NULL){
    set = sw->values;
    if (sw->values) {
      if (set->next == NULL) {
        DestroySetNodeByReference(sw->values);
      }
      else {
        DestroySetListByReference(sw->values);
      }
    }
    DestroyStatementList(sw->slist);
    sw->next = NULL;
    ascfree((char *)sw);
  }
}

void DestroySwitchList(struct SwitchList *sw)
{
  register struct SwitchList *next;
  while (sw!=NULL){
    next = NextSwitchCase(sw);
    DestroySwitchNode(sw);
    sw = next;
  }
}

struct SwitchList *CopySwitchNode(struct SwitchList *sw)
{
  register struct SwitchList *result;
  assert(sw);
  SWMALLOC(result);
  if (sw->values) result->values = CopySetByReference(sw->values);
  else result->values = sw->values;
  result->slist = CopyListToModify(sw->slist);
  result->next = NULL;
  return result;
}

struct SwitchList *CopySwitchList(struct SwitchList *sw)
{
  register struct SwitchList *head=NULL,*p;
  if (sw!=NULL) {
    p = head = CopySwitchNode(sw);
    sw = sw->next;
    while(sw!=NULL){
      p->next = CopySwitchNode(sw);
      p = p->next;
      sw = sw->next;
    }
  }
  return head;
}






