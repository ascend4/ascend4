/*
 *  Implementation of set routines
 *  by Tom Epperly
 *  August 1, 1989
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: sets.c,v $
 *  Date last modified: $Date: 1998/01/27 11:00:21 $
 *  Last modified by: $Author: ballan $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 *  This is the implementation of the set routines
 */

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/list.h"
#include "general/pool.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/sets.h"
#include "compiler/setio.h"
#include "compiler/evaluate.h"
#include "compiler/exprs.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef lint
static CONST char SetFuncsID[] = "$Id: sets.c,v 1.13 1998/01/27 11:00:21 ballan Exp $";
#endif

#define SETUSESPOOL TRUE

#if (!SETUSESPOOL)

#define SMALLOC (struct Set *)ascmalloc((unsigned)sizeof(struct Set))
#define SFREE(s) ascfree((char *)(s))
void sets_init_pool(void) {}
void sets_destroy_pool(void) {}
void sets_report_pool(void)
{
    FPRINTF(ASCERR,"SetsPool not used at all\n");
}


#else

static pool_store_t g_sets_pool = NULL;
/* global for our memory manager */
/* aim for 4096 chunks including malloc overhead */
#define SETS_LEN 10
#if (SIZEOF_VOID_P == 8)
#define SETS_WID 100
#else
#define SETS_WID 200
#endif
/* retune rpwid if the size of struct name changes */
#define SETS_ELT_SIZE (sizeof(struct Set))
#define SETS_MORE_ELTS 3
/*
 *  Number of slots filled if more elements needed.
 *  So if the pool grows, it grows by SETS_MORE_ELTS*SETS_WID elements at a time.
 */
#define SETS_MORE_BARS 50
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown. */
void sets_init_pool(void) {
  if (g_sets_pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: sets_init_pool called twice.\n");
  }
  g_sets_pool = pool_create_store(SETS_LEN, SETS_WID, SETS_ELT_SIZE,
    SETS_MORE_ELTS, SETS_MORE_BARS);
  if (g_sets_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: sets_init_pool unable to allocate pool.\n");
  }
}

void sets_destroy_pool(void) {
  if (g_sets_pool==NULL) return;
  pool_clear_store(g_sets_pool);
  pool_destroy_store(g_sets_pool);
  g_sets_pool = NULL;
}

void sets_report_pool(void)
{
  if (g_sets_pool==NULL) {
    FPRINTF(ASCERR,"SetsPool is empty\n");
  }
  FPRINTF(ASCERR,"SetsPool ");
  pool_print_store(ASCERR,g_sets_pool,0);
}

#define SMALLOC ((struct Set *)(pool_get_element(g_sets_pool)))
/* get a token. Token is the size of the struct struct Set */
#define SFREE(p) (pool_free_element(g_sets_pool,((void *)p)))
/* return a struct Set */

#endif

struct Set *CreateSingleSet(struct Expr *ex)
{
  struct Set *result;
  result = SMALLOC;
  result->range = 0;
  result->next = NULL;
  result->val.e = ex;
  result->ref_count = 1;
  return result;
}

struct Set *CreateRangeSet(struct Expr *lower, struct Expr *upper)
{
  struct Set *result;
  result = SMALLOC;
  result->range = 1;
  result->next =NULL;
  result->val.r.lower = lower;
  result->val.r.upper = upper;
  result->ref_count = 1;
  return result;
}

void LinkSets(struct Set *cur, struct Set *next)
{
  assert(cur!=NULL);
  cur->next = next;
}

int SetTypeF(CONST struct Set *s)
{
  assert(s!=NULL);
  return s->range;
}

struct Set *NextSetF(CONST struct Set *s)
{
  assert(s!=NULL);
  return s->next;
}

CONST struct Expr *GetSingleExprF(CONST struct Set *s)
{
  assert(s!=NULL);
  assert(!(s->range));
  return s->val.e;
}

CONST struct Expr *GetLowerExprF(CONST struct Set *s)
{
  assert(s!=NULL);
  assert(s->range);
  return s->val.r.lower;
}

CONST struct Expr *GetUpperExprF(CONST struct Set *s)
{
  assert(s!=NULL);
  assert(s->range);
  return s->val.r.upper;
}

struct Set *CopySetNode(CONST struct Set *s)
{
  register struct Set *result;
  if (s==NULL) {
    return NULL;
  }
  result = SMALLOC;
  result->ref_count = 1;
  if (s->range) {
    result->val.r.lower = CopyExprList(s->val.r.lower);
    result->val.r.upper = CopyExprList(s->val.r.upper);
  } else {
    result->val.e = CopyExprList(s->val.e);
  }
  result->range = s->range;
  result->next = NULL;
  return result;
}

struct Set *CopySetList(CONST struct Set *s)
{
  register struct Set *result,*p;
  if (s==NULL) {
    return NULL;
  }
  result = CopySetNode(s); /* must keep the head node to return it */
  p = result;
  while (s->next!=NULL) {
    s = s->next;
    p->next = CopySetNode(s);
    p = p->next;
  }
  p->next = NULL;
  return result;
}


struct Set *CopySetByReference(struct Set *s)
{ if (s==NULL) return s;
  assert(s->ref_count);
  s->ref_count++;
  return s;
}

void DestroySetNode(struct Set *s)
{
  assert(s!=NULL);
  AssertAllocatedMemory(s,sizeof(struct Set));
  if (s->range) {
    DestroyExprList(s->val.r.lower);
    DestroyExprList(s->val.r.upper);
  } else {
    DestroyExprList(s->val.e);
  }
  SFREE(s);
}


void DestroySetList(struct Set *s)
{
  register struct Set *next;
  while (s!=NULL) {
    AssertAllocatedMemory(s,sizeof(struct Set));
    next = s->next;
    DestroySetNode(s);
    s = next;
  }
}

void DestroySetHead(struct Set *s)
{
  assert(s!=NULL);
  AssertAllocatedMemory(s,sizeof(struct Set));
  SFREE(s);
}

void DestroySetListByReference(struct Set *s)
{
  register struct Set *next;
  if (--s->ref_count == 0){
    while (s!=NULL) {
      AssertAllocatedMemory(s,sizeof(struct Set));
      next = s->next;
      if (s->range) {
        DestroyExprList(s->val.r.lower);
        DestroyExprList(s->val.r.upper);
      }
      else DestroyExprList(s->val.e);
      SFREE(s);
      s = next;
    }
  }
}

void DestroySetNodeByReference(struct Set *s)
{
  assert(s!=NULL);
  AssertAllocatedMemory(s,sizeof(struct Set));
  if (--s->ref_count == 0){
    if (s->range) {
      DestroyExprList(s->val.r.lower);
      DestroyExprList(s->val.r.upper);
    }
    else DestroyExprList(s->val.e);
    SFREE(s);
  }
}

struct Set *JoinSetLists(struct Set *s1, struct Set *s2)
{
  register struct Set *s;
  if (s1==NULL) return s2;
  s = s1;
  /* find end of set list */
  while(s->next) s = s->next;
  /* link to s2 */
  s->next = s2;
  return s1;
}

struct Set *ReverseSetList(register struct Set *s)
{
  register struct Set *next,*previous=NULL;
  if (s==NULL) return s;
  while (TRUE) {		/* loop until broken */
    next = s->next;
    s->next = previous;
    if (next==NULL) return s;
    previous = s;
    s = next;
  }
}

int SetStructuresEqual(CONST struct Set *s1, CONST struct Set *s2)
{
  if (s1==s2) return 1;
  while ((s1!=NULL)&&(s2!=NULL)){
    if (SetType(s1)!=SetType(s2)) return 0;
    if (SetType(s1)){		/* range */
      if ((!ExprsEqual(GetLowerExpr(s1),GetLowerExpr(s2)))||
	  (!ExprsEqual(GetUpperExpr(s1),GetUpperExpr(s2))))
	return 0;
    }
    else
      if (!ExprsEqual(GetSingleExpr(s1),GetSingleExpr(s2))) return 0;
    s1 = NextSet(s1);
    s2 = NextSet(s2);
  }
  return ((s1==NULL)&&(s2==NULL));
}

int CompareSetStructures(CONST struct Set *s1, CONST struct Set *s2)
{
  int ctmp;
  if (s1==s2) return 0;
  if (s1 == NULL) {
    return 1;
  }
  if (s2 == NULL) {
    return -1;
  }
  while ((s1!=NULL)&&(s2!=NULL)){
    if (SetType(s1)!=SetType(s2)) {
      /* 1 is a range and 1 an expr */
      if (SetType(s1)) {
        return 1;
      } else {
        return -1;
      }
    }
    /* both of same type */
    if (SetType(s1)){		/* range */
      ctmp = CompareExprs(GetLowerExpr(s1),GetLowerExpr(s2));
      if (ctmp != 0) {
        return ctmp;
      }
      ctmp = CompareExprs(GetUpperExpr(s1),GetUpperExpr(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    } else { /* exprs */
      ctmp = CompareExprs(GetSingleExpr(s1),GetSingleExpr(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    s1 = NextSet(s1);
    s2 = NextSet(s2);
  }
  if (s1 != NULL) {
    return 1;
  }
  if (s2 != NULL) {
    return -1;
  }
  return 0;
}

unsigned long SetLength(CONST struct Set *set)
{
  register unsigned long l=0;
  while (set!=NULL){
    l++;
    set = NextSet(set);
  }
  return l;
}

struct gl_list_t *SetNameList(CONST struct Set *set)
{
  struct gl_list_t *list;
  list = gl_create(3L);
  assert(list!=NULL);
  while (set!=NULL){
    if (SetType(set)) { /*range*/
      list = EvaluateNamesNeeded(GetLowerExpr(set),NULL,list);
      list = EvaluateNamesNeeded(GetUpperExpr(set),NULL,list);
    } else {
      list = EvaluateNamesNeeded(GetSingleExpr(set),NULL,list);
    }
    set = NextSet(set);
  }
  return list;
}

char *CreateStrFromSet(CONST struct Set *set)
{
  char *result;
  Asc_DString dstring;
  Asc_DStringInit(&dstring);
  WriteSet2Str(&dstring,set);
  result = Asc_DStringResult(&dstring);
  return result;
}

