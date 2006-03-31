/*
 *  Variable name list implementation
 *  by Tom Epperly
 *  August 8, 1989
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: vlist.c,v $
 *  Date last modified: $Date: 1997/07/18 12:36:36 $
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

#include<stdio.h>
#include<assert.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include <utilities/ascMalloc.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "vlist.h"
#include "name.h"

#define NLMALLOC \
(struct VariableList *)ascmalloc((unsigned)sizeof(struct VariableList))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef lint
static CONST char VariableListID[] = "$Id: vlist.c,v 1.7 1997/07/18 12:36:36 mthomas Exp $";
#endif

struct VariableList *CreateVariableNode(struct Name *n)
{
  register struct VariableList *result;
  assert(n!=NULL);
  result = NLMALLOC;
  assert(result!=NULL);
  result->nptr = n;
  result->next = NULL;
  return result;
}

void LinkVariableNodes(struct VariableList *cur, struct VariableList *next)
{
  assert(cur!=NULL);
  cur->next = next;
}

CONST struct VariableList *NextVariableNodeF(CONST struct VariableList *n)
{
  assert(n!=NULL);
  return n->next;
}

CONST struct Name *NamePointerF(CONST struct VariableList *n)
{
  assert(n!=NULL);
  return n->nptr;
}

struct VariableList *CopyVariableList(struct VariableList *n)
{
  register struct VariableList *result,*p,*np;
  if (n==NULL) return NULL;
  np = n;
  result = NLMALLOC;
  result->nptr = CopyName(np->nptr);
  p = result;
  while(np->next !=NULL) {
    p->next = NLMALLOC;
    p = p->next;
    np = np->next;
    p->nptr = CopyName(np->nptr);
  }
  p->next = NULL;
  return result;
}

unsigned long VariableListLength(CONST struct VariableList *n)
{
  unsigned long len=0L;
  while(n!=NULL) {
    n = n->next;
    len++;
  }
  return len;
}

void DestroyVariableList(register struct VariableList *n)
{
  register struct VariableList *next;
  while (n!=NULL) {
    next = n->next;
    DestroyName(n->nptr);
    ascfree((char *)n);
    n = next;
  }
}

void DestroyVariableListNode(struct VariableList *n)
{
  if (n!=NULL) {
    DestroyName(n->nptr);
    ascfree((char *)n);
  }
}

struct VariableList *JoinVariableLists(struct VariableList *n1,
				       struct VariableList *n2)
{
  register struct VariableList *p;
  if (n1==NULL) return n2;
  /* find end of name list n1 */
  p = n1;
  while (p->next) p = p->next;
  /* link to n2 */
  p->next = n2;
  return n1;
}

struct VariableList *ReverseVariableList(register struct VariableList *n)
{
  register struct VariableList *next,*previous=NULL;
  if (n==NULL) return n;
  while (TRUE) {		/* loop until broken */
    next = n->next;
    n->next = previous;
    if (next==NULL) return n;
    previous = n;
    n = next;
  }
}

int CompareVariableLists(CONST struct VariableList *vl1,
                         CONST struct VariableList *vl2)
{
  int ctmp;
  if (vl1==vl2) { return 0; }
  if (vl1==NULL) { return 1; }
  if (vl2==NULL) { return -1; }
  while(vl1!=NULL && vl2!=NULL) {
    ctmp = CompareNames(vl1->nptr,vl2->nptr);
    if (ctmp != 0) { return ctmp; }
    vl1 = vl1->next;
    vl2 = vl2->next;
  }
  if (vl1!=NULL) { return 1; }
  if (vl2!=NULL) { return -1; }
  return 0;
}
