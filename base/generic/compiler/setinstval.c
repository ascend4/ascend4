/*
 *  Routines for Set Instance Values
 *  by Tom Epperly
 *  12/4/89
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: setinstval.c,v $
 *  Date last modified: $Date: 1998/02/05 16:37:50 $
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
 */
#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascPanic.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "general/pool.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/instance_enum.h"
#include "compiler/cmpfunc.h"
#include "compiler/setinstval.h"

#ifndef lint
static CONST char SetInstValID[] = "$Id: setinstval.c,v 1.9 1998/02/05 16:37:50 ballan Exp $";
#endif

#define MYMIN(x,y) (((x)<(y)) ? (x) : (y))

#ifdef ASC_NO_POOL
#define SETINST_USES_POOL FALSE
#else
#define SETINST_USES_POOL TRUE
#endif

static pool_store_t g_set_pool=NULL;
/*
 * A pool_store for all the sets ever simultaneously in use.
 */
#define SM_LEN 2
#if (SIZEOF_VOID_P == 8)
#define SM_WID 63
#else
#define SM_WID 126
#endif
/* retune if the size of set stub changes dramatically */
#define SM_ELT_SIZE (sizeof(struct set_t))
#define SM_MORE_ELTS 1
/* Number of slots filled if more elements needed.
 * So if the pool grows, it grows by SM_MORE_ELTS*SM_WID elements at a time.
 */
#define SM_MORE_BARS 10
/*  This is the number of pool bar slots to add during expansion.
 *  not all the slots will be filled immediately.
 */

void InitSetManager(void)
{
#if SETINST_USES_POOL 
  if (g_set_pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: InitSetManager called twice.\n");
  }
  g_set_pool =
    pool_create_store(SM_LEN, SM_WID, SM_ELT_SIZE, SM_MORE_ELTS, SM_MORE_BARS);
  if (g_set_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: InitSetManager unable to allocate pool.\n");
  }
#endif
}

void DestroySetManager(void)
{
#if SETINST_USES_POOL 
  assert(g_set_pool!=NULL);
  pool_destroy_store(g_set_pool);
  g_set_pool = NULL;
#endif
}

void ReportSetManager(FILE *f)
{
#if SETINST_USES_POOL 
  assert(g_set_pool!=NULL);
  FPRINTF(f,"SetManager ");
  pool_print_store(f,g_set_pool,0);
#else
  FPRINTF(f,"SetManager pool not used.");
#endif
}

#if SETINST_USES_POOL 
#define MALLOCSET(x) (x = (struct set_t *)pool_get_element(g_set_pool))
#define FREESET(set) pool_free_element(g_set_pool,(set))
#else
#define MALLOCSET(x) x = (struct set_t *)ascmalloc(sizeof(struct set_t ))
#define FREESET(set) ascfree(set)
#endif /* SETINST_USES_POOL */

static
int SetIntCmp(long int i1, long int i2)
{
  if (i1==i2) return 0;
  if (i1<i2) return -1;
  else return 1;
}

static
int SetStrCmp(CONST char *s1, CONST char *s2)
{
  return strcmp(s1,s2);
}

struct set_t *CreateEmptySet(void)
{
  register struct set_t *result;
  assert(sizeof(long)==sizeof(char *));
  MALLOCSET(result);
  result->kind = empty_set;
  result->list = NULL;
  return result;
}

static
void StringViolation(CONST char *name)
{
  Asc_Panic(2, NULL,
            "Integer set routine %s called with string set argument.\n",
            name);
}

static
void IntegerViolation(CONST char *name)
{
  Asc_Panic(2, NULL,
            "String set routine %s called with integer set argument.\n",
            name);
}

void InsertInteger(struct set_t *set, long int i)
{
  assert(set&&((set->kind==integer_set)||(set->kind==empty_set)));
  if (set->kind==empty_set)
    set->kind = integer_set;
  else if (set->kind==string_set)
    StringViolation("InsertInteger");
  if (set->list==NULL) {
    set->list = gl_create(5L);
    gl_insert_sorted(set->list,(VOIDPTR)i,(CmpFunc)SetIntCmp);
  }
  else {
    if (gl_search(set->list,(VOIDPTR)i,(CmpFunc)SetIntCmp)==0)
      gl_insert_sorted(set->list,(VOIDPTR)i,(CmpFunc)SetIntCmp);
  }
}


void InsertString(struct set_t *set, symchar *str)
{
  assert(AscFindSymbol(str)!=NULL);
  assert(set&&((set->kind==string_set)||(set->kind==empty_set)));
  if(set->kind==empty_set)
    set->kind = string_set;
  else if(set->kind==integer_set)
    IntegerViolation("InsertString");
  if (set->list==NULL){
    set->list = gl_create(5L);
    gl_insert_sorted(set->list,(VOIDPTR)str,(CmpFunc)SetStrCmp);
  }
  else {
    if (gl_search(set->list,str,(CmpFunc)SetStrCmp)==0)
      gl_insert_sorted(set->list,(VOIDPTR)str,(CmpFunc)SetStrCmp);
  }
}


void InsertIntegerRange(struct set_t *set, long int lower, long int upper)
{
  register long i;
  assert(set&&((set->kind==integer_set)||(set->kind==empty_set)));
  for(i=lower;i<=upper;InsertInteger(set,i++));
}

struct set_t *SetUnion(CONST struct set_t *s1, CONST struct set_t *s2)
{
  struct set_t *result;
  register unsigned long c1,l1,c2,l2;
  register int (*func)(),cmp;
  assert(s1&&s2);
  if (s1->kind==empty_set) return CopySet(s2);
  if (s2->kind==empty_set) return CopySet(s1);
  assert(s1->kind==s2->kind);
  if (s1->list==NULL) {
    if (s2->list==NULL) {
      MALLOCSET(result);
      result->kind = s1->kind;
      result->list = NULL;
      return result;
    }
    return CopySet(s2);
  }
  if (s2->list==NULL) return CopySet(s1);
  /* prepare for union */
  c1=c2=1;
  l1=gl_length(s1->list);
  l2=gl_length(s2->list);
  MALLOCSET(result);
  result->kind = s1->kind;
  if ((l1==0)&&(l2==0)) {
    result->list = NULL;
    return result;
  }
  result->list = gl_create(l1+l2);
  if (s1->kind == integer_set) func = SetIntCmp;
  else func = SetStrCmp;
  while((c1<=l1)||(c2<=l2)) {
    if (c1>l1)
      gl_append_ptr(result->list,gl_fetch(s2->list,c2++));
    else if (c2>l2)
      gl_append_ptr(result->list,gl_fetch(s1->list,c1++));
    else {
      cmp = (*func)(gl_fetch(s1->list,c1),gl_fetch(s2->list,c2));
      if (cmp<0) gl_append_ptr(result->list,gl_fetch(s1->list,c1++));
      else if (cmp>0) gl_append_ptr(result->list,gl_fetch(s2->list,c2++));
      else { /* equal */
	gl_append_ptr(result->list,gl_fetch(s1->list,c1++));
	c2++;
      }
    }
  }
  gl_sort(result->list,func);
  return result;
}

struct set_t *SetIntersection(CONST struct set_t *s1, CONST struct set_t *s2)
{
  struct set_t *result;
  register unsigned long c1,l1,c2,l2;
  register int (*func)(),cmp;
  assert(s1&&s2);
  if (s1->kind==empty_set) return CreateEmptySet();
  if (s2->kind==empty_set) return CreateEmptySet();
  assert(s1->kind==s2->kind);
  if ((s1->list==NULL)||(s2->list==NULL)){
    MALLOCSET(result);
    result->kind = s1->kind;
    result->list = NULL;
    return result;
  }
  /* prepare for intersection */
  c1=c2=1;
  l1=gl_length(s1->list);
  l2=gl_length(s2->list);
  MALLOCSET(result);
  result->kind = s1->kind;
  if ((l1==0)&&(l2==0)) {
    result->list = NULL;
    return result;
  }
  result->list = gl_create(MYMIN(l1,l2));
  if (s1->kind == integer_set) func = SetIntCmp;
  else func = SetStrCmp;
  while((c1<=l1)&&(c2<=l2)) {
    cmp = (*func)(gl_fetch(s1->list,c1),gl_fetch(s2->list,c2));
    if (cmp<0) c1++;
    else if (cmp>0) c2++;
    else { /* equal */
      gl_append_ptr(result->list,gl_fetch(s1->list,c1++));
      c2++;
    }
  }
  gl_sort(result->list,func);
  return result;
}

struct set_t *SetDifference(CONST struct set_t *s1, CONST struct set_t *s2)
{
  struct set_t *result;
  register unsigned long c1,l1,c2,l2;
  register int (*func)(),cmp;
  assert(s1&&s2);
  if (s1->kind==empty_set) return CreateEmptySet();
  if (s2->kind==empty_set) return CopySet(s1);
  assert(s1->kind==s2->kind);
  if ((s1->list==NULL)||(s2->list==NULL)||(gl_length(s2->list)==0)||
      (gl_length(s1->list)==0))
    return CopySet(s1);
  /* prepare for difference */
  c1=c2=1;
  l1=gl_length(s1->list);
  l2=gl_length(s2->list);
  MALLOCSET(result);
  result->kind = s1->kind;
  result->list = gl_create(l1);
  if (s1->kind == integer_set) func = SetIntCmp;
  else func = SetStrCmp;
  while(c1<=l1) {
    if (c2>l2)
      gl_append_ptr(result->list,gl_fetch(s1->list,c1++));
    else {
      cmp = (*func)(gl_fetch(s1->list,c1),gl_fetch(s2->list,c2));
      if (cmp<0) gl_append_ptr(result->list,gl_fetch(s1->list,c1++));
      else if (cmp>0) c2++;
      else { /* equal */
	c1++;
	c2++;
      }
    }
  }
  gl_sort(result->list,func);
  return result;
}

struct set_t *CopySet(CONST struct set_t *set)
{
  register struct set_t *result;
  assert(set!=NULL);
  MALLOCSET(result);
  result->kind = set->kind;
  if (set->list!=NULL)
    result->list = gl_copy(set->list);
  else
    result->list = NULL;
  return result;
}

int IntMember(long int i, CONST struct set_t *set)
{
  assert(set&&((set->kind==integer_set)||(set->kind==empty_set)));
  if (set->list==NULL) return 0;
  return (gl_search(set->list,(char *)i,(CmpFunc)SetIntCmp)!=0);
}

int StrMember(symchar *str, CONST struct set_t *set)
{
  assert(set&&((set->kind==string_set)||(set->kind==empty_set)));
  if (set->list==NULL) return 0;
  assert(AscFindSymbol(str)!=NULL);
  return (gl_search(set->list,str,(CmpFunc)SetStrCmp)!=0);
}

void DestroySet(struct set_t *set)
{
  if (set!=NULL) {
    if (set->list!=NULL) gl_destroy(set->list);
    set->list=NULL;
    FREESET(set);
  }
}


int NullSet(CONST struct set_t *s)
{
  assert(s!=NULL);
  return (s->list==NULL)||(gl_length(s->list)==0);
}

unsigned long Cardinality(CONST struct set_t *s)
{
  assert(s!=NULL);
  if (s->list==NULL) return 0;
  else return gl_length(s->list);
}

symchar *FetchStrMember(CONST struct set_t *s, unsigned long int i)
{
  assert(s&&s->list&&(s->kind==string_set));
  return gl_fetch(s->list,i);
}

long FetchIntMember(CONST struct set_t *s, unsigned long int i)
{
  assert(s&&s->list&&(s->kind==integer_set));
  return (long)gl_fetch(s->list,i);
}

void SetIterate(struct set_t *s, void (*func) (/* ??? */))
{
  assert(s!=NULL);
  if (s->list!=NULL)
    gl_iterate(s->list,func);
}

enum set_kind SetKind(CONST struct set_t *s)
{
  assert(s!=NULL);
  return s->kind;
}

int SetsEqual(CONST struct set_t *s1, CONST struct set_t *s2)
{
  register unsigned long c,length;
  assert(s1&&s2);
  if (s1->kind!=s2->kind) return 0;
  if (s1->list==NULL) {
    if ((s2->list==NULL)||(gl_length(s2->list)==0)) return 1;
    else return 0;
  }
  if (s2->list==NULL){
    if (gl_length(s1->list)==0) return 1;
    else return 0;
  }
  length = gl_length(s1->list);
  if (length != gl_length(s2->list)) return 0;
  if (s1->kind == integer_set) {
    for(c=1;c<=length;c++) {
      if ((long)gl_fetch(s1->list,c) != (long)gl_fetch(s2->list,c)) return 0;
    }
  } else {
    /* symbol set */
    for(c=1;c<=length;c++) {
      if (gl_fetch(s1->list,c) != gl_fetch(s2->list,c)) return 0;
    }
  }
  return 1;
}

int Subset(CONST struct set_t *s1, CONST struct set_t *s2)
{
  register unsigned long c1,c2,length1,length2;
  assert(s1&&s2);
  if (s1->kind==empty_set) return 1;
  if (s2->kind==empty_set) return 0;
  if (s1->kind!=s2->kind) return 0;
  if ((s1->list==NULL)||(gl_length(s1->list)==0)) return 1;
  if (s2->list==NULL) return 0;
  length1=gl_length(s1->list);
  length2=gl_length(s2->list);
  if (length1>length2) return 0;
  c1=c2=1;
  if (s1->kind == integer_set) {
    register long i1,i2;
    while((c1<=length1)&&(c2<=length2)) {
      i1 = (long)gl_fetch(s1->list,c1);
      i2 = (long)gl_fetch(s2->list,c2);
      if (i1 == i2) {
	c1++;
	c2++;
      }
      else if (i1 < i2) return 0;
      else c2++;
    }
  }
  else {
    register char *str1,*str2;
    register int cmp;
    while((c1<=length1)&&(c2<=length2)) {
      str1 = gl_fetch(s1->list,c1);
      str2 = gl_fetch(s2->list,c2);
      cmp = strcmp(str1,str2);
      if (cmp==0) {
	c1++;
	c2++;
      }
      else if (cmp<0) return 0;
      else c2++;
    }
  }
  if (c1>length1) return 1;
  else return 0;
}

int CmpSetInstVal(CONST struct set_t *s1, CONST struct set_t *s2)
{
  unsigned long c,len;
  symchar *str1, *str2;
  int cmp;
  if (s1==s2) {
    return 0;
  }
  if (s1==NULL) {
    return -1;
  }
  if (s2==NULL) {
    return 1;
  }
  if (s1->kind == empty_set) {
    return -1;
  }
  if (s2->kind == empty_set) {
    return 1;
  }
  if (SetKind(s1)!=SetKind(s2)) {
    return (SetKind(s1)==integer_set) ? 1 : -1;
  }
  if (gl_length(s1->list) != gl_length(s2->list)) {
    return  (gl_length(s1->list) < gl_length(s2->list)) ? 1 : -1;
  }
  len = gl_length(s1->list);
  if (s1->kind == integer_set) {
    for (c = 1; c <= len; c++) {
      if (gl_fetch(s1->list,c) != gl_fetch(s2->list,c)) {
        return  (gl_fetch(s1->list,c) < gl_fetch(s2->list,c)) ? 1 : -1;
      }
    }
  } else {
    for (c = 1; c <= len; c++) {
      str1 = gl_fetch(s1->list,c);
      str2 = gl_fetch(s2->list,c);
      cmp = CmpSymchar(str1,str2);
      if (cmp!=0) {
        return cmp;
      }
    }
  }
  return 0;
}

/*********************************************************************\
  Some ordered set processing. The elements of the below sets are
  not necessarily unique and not necessarily ordered. In this way they
  behave more like lists.
\*********************************************************************/

void AppendIntegerElement(struct set_t *set, long int i)
{
  assert(set&&((set->kind==integer_set)||(set->kind==empty_set)));
  if (set->kind==empty_set)
    set->kind = integer_set;
  else if (set->kind==string_set)
    StringViolation("InsertInteger");
  if (set->list==NULL) {
    set->list = gl_create(5L);
    gl_append_ptr(set->list,(VOIDPTR)i);
  }
  else {
    gl_append_ptr(set->list,(VOIDPTR)i);
  }
}

void AppendStringElement(struct set_t *set, symchar *str)
{
  assert(set&&((set->kind==string_set)||(set->kind==empty_set)));
  assert(AscFindSymbol(str)!=NULL);
  if(set->kind==empty_set)
    set->kind = string_set;
  else if(set->kind==integer_set)
    IntegerViolation("InsertString");
  if (set->list==NULL){
    set->list = gl_create(5L);
    gl_append_ptr(set->list,(VOIDPTR)str);
  }
  else {
    gl_append_ptr(set->list,(VOIDPTR)str);
  }
}

