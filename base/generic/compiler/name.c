/*
 *  Name procedures
 *  by Tom Epperly
 *  Part of Ascend
 *  Version: $Revision: 1.14 $
 *  Version control file: $RCSfile: name.c,v $
 *  Date last modified: $Date: 1998/02/05 16:37:12 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

#include<stdio.h>
#include<assert.h>
#include <utilities/ascConfig.h>
#include "compiler.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/pool.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "func.h"
#include "expr_types.h"
#include "sets.h"
#include "name.h"


#ifndef lint
static CONST char NameProcID[] = "$Id: name.c,v 1.14 1998/02/05 16:37:12 ballan Exp $";
#endif

#ifndef NULL
#define NULL 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef ASC_NO_POOL
#define NAMEUSESPOOL FALSE
#else
#define NAMEUSESPOOL TRUE 
#endif
#define NAMEDEBUG 0

/* do not make these static or conditional or interface.c will need fixing */
unsigned long g_num_names_cur=0;
unsigned long g_num_names_max=0;

/* the logic here is:
 * if namedebug {
 *   AllocName and name counting. malloc/free.
 * } else {
 *   if usepool {
 *     recycle names elements.
 *   } else {
 *     straight malloc/free.
 *   }
 * }
 */
#if NAMEDEBUG

#define IDNMALLOC AllocName()
#define IDNFREE(a) ascfree(a)
static struct Name *AllocName()
{
  g_num_names_cur++;
  if (g_num_names_cur>g_num_names_max) g_num_names_max=g_num_names_cur;
  return IDNMALLOC;
}

#else /*_NAMEDEBUG*/

#if NAMEUSESPOOL
static pool_store_t g_name_pool = NULL;
/* global for our memory manager */
/* aim for 4096 chunks including malloc overhead */
#define NP_LEN 10
#if (SIZEOF_VOID_P == 8)
#define NP_WID 168
#else
#define NP_WID 336
#endif
/* retune rpwid if the size of struct name changes */
#define NP_ELT_SIZE (sizeof(struct Name))
#define NP_MORE_ELTS 10
/*
 *  Number of slots filled if more elements needed.
 *  So if the pool grows, it grows by NP_MORE_ELTS*NP_WID elements at a time.
 */
#define NP_MORE_BARS 500
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown. */
void name_init_pool(void) {
  if (g_name_pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: name_init_pool called twice.\n");
  }
  g_name_pool = pool_create_store(NP_LEN, NP_WID, NP_ELT_SIZE,
    NP_MORE_ELTS, NP_MORE_BARS);
  if (g_name_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: name_init_pool unable to allocate pool.\n");
  }
}

void name_destroy_pool(void) {
  if (g_name_pool==NULL) return;
  pool_clear_store(g_name_pool);
  pool_destroy_store(g_name_pool);
  g_name_pool = NULL;
}

void name_report_pool(void)
{
  if (g_name_pool==NULL)
    FPRINTF(ASCERR,"ListHeadPool is empty\n");
  FPRINTF(ASCERR,"ListHeadPool ");
  pool_print_store(ASCERR,g_name_pool,0);
}

#define IDNMALLOC ((struct Name *)(pool_get_element(g_name_pool)))
/* get a token. Token is the size of the struct struct Name */
#define IDNFREE(p) (pool_free_element(g_name_pool,((void *)p)))
/* return a struct Name */

#else /* NAMEUSESPOOL */

#define IDNFREE(p) ascfree(p)
#define IDNMALLOC ASC_NEW(struct Name)

void name_init_pool(void) {}
void name_destroy_pool(void) {}
void name_report_pool(void) {
    FPRINTF(ASCERR,"ListHeadPool is not used at all.\n");
}


#endif /* NAMEUSESPOOL */
#endif /* NAMEDEBUG */



struct Name *CreateIdNameF(symchar *s,int bits)
{
  register struct Name *result;
  assert(s!=NULL);
  result = IDNMALLOC;
  assert(result!=NULL);
  bits &= (NAMEBIT_IDTY | NAMEBIT_ATTR | NAMEBIT_AUTO | NAMEBIT_CHAT);
  result->bits = bits;
  result->val.id = s;
  result->next = NULL;
  return result;
}

symchar *SimpleNameIdPtr(CONST struct Name *nptr)
{
  if (nptr==NULL) return NULL;
  if (NextName(nptr)!=NULL) return NULL;
  return NameId(nptr) ? NameIdPtr(nptr) : NULL;
}

unsigned int NameLength(CONST struct Name *n)
{
  unsigned int length=0;
  while(n!=NULL){
    length++;
    n = NextName(n);
  }
  return length;
}

struct Name *CreateSetName(struct Set *s)
{
  register struct Name *result;
  assert(s!=NULL);
  result = IDNMALLOC;
  assert(result!=NULL);
  result->bits = 0;
  result->val.s = s;
  result->next = NULL;
  return result;
}

void LinkNames(struct Name *cur, struct Name *next)
{
  assert(cur!=NULL);
  cur->next = next;
}

struct Name *NextNameF(CONST struct Name *n)
{
  assert(n!=NULL);
  return n->next;
}

int NameIdF(CONST struct Name *n)
{
  assert(n!=NULL);
  return ((n->bits & NAMEBIT_IDTY) != 0);
}

int NameAutoF(CONST struct Name *n)
{
  assert(n!=NULL);
  return ((n->bits &( NAMEBIT_IDTY | NAMEBIT_AUTO)) != 0);
}

symchar *NameIdPtrF(CONST struct Name *n)
{
  assert(n!=NULL);
  return n->val.id;
}

CONST struct Set *NameSetPtrF(CONST struct Name *n)
{
  assert(n!=NULL);
  return n->val.s;
}

struct Name *CopyName(CONST struct Name *n)
{
  register struct Name *result,*p;
  register CONST struct Name *np;
  if (n==NULL) return NULL;
  np = n;
  result = IDNMALLOC;
  *result = *np;
  if (!(np->bits & NAMEBIT_IDTY)) {
    result->val.s = CopySetList(np->val.s);
  }
  p = result;
   while (np->next!=NULL) {
     p->next = IDNMALLOC;
     p = p->next;
     np = np->next;
     *p = *np;
     if (!(np->bits & NAMEBIT_IDTY)) {
       p->val.s = CopySetList(np->val.s);
     }
   }
  return result;
}

void DestroyName(register struct Name *n)
{
  register struct Name *next;
  while(n!=NULL) {
    next = n->next;
    if (!(n->bits & NAMEBIT_IDTY)) {
      DestroySetList(n->val.s);
    }
    IDNFREE((char *)n);
#if NAMEDEBUG
    g_num_names_cur--;
#endif
    n = next;
  }
}

void DestroyNamePtr(struct Name *n)
{
  if (n!=NULL) {
    if (!(n->bits & NAMEBIT_IDTY)) {
      DestroySetList(n->val.s);
    }
#if NAMEDEBUG
    g_num_names_cur--;
#endif
    IDNFREE((char *)n);
  }
}

struct Name *JoinNames(struct Name *n1, struct Name *n2)
{
  register struct Name *p;
  if (n1==NULL) return n2;
  /* find end of name list */
  p = n1;
  while (p->next) p = p->next;
  /* link to n2 */
  p->next = n2;
  return n1;
}

CONST struct Name *NextIdName(register CONST struct Name *n)
{
  if (n==NULL) return NULL;
  assert(NameId(n)!=0);
  n = n->next;
  while (n!=NULL && !NameId(n)) {
    n = n->next;
  }
  return n;
}

struct Name *ReverseName(register struct Name *n)
{
  register struct Name *next,*previous=NULL;
  if (n==NULL) return n;
  while (TRUE) {		/* loop until it returns */
    next = n->next;
    n->next = previous;
    if (next==NULL) return n;
    previous = n;
    n = next;
  }
}

int NameCompound(CONST struct Name *n)
{
  int dotseen = 0, idseen = 0, count=0;
  while (n != NULL) {
    if (NameId(n)!=0) {
      idseen++;
      if (count) {
        /* the id follows an array subscript */
        dotseen=1;
      }
    }
    /* array subscripts are really irrelevant, except they bump up count */
    count++;
    n = NextName(n);
  }
  return (dotseen || idseen>1);
}

int NamesEqual(CONST struct Name *n1, CONST struct Name *n2)
{
  if (n1==n2) return 1;
  while ((n1!=NULL)&&(n2!=NULL)){
    if (NameId(n1)!=NameId(n2)) return 0;
    if (NameId(n1)){
      if (NameIdPtr(n1) != NameIdPtr(n2)) return 0;
    } else {
      if (!SetStructuresEqual(NameSetPtr(n1),NameSetPtr(n2))) return 0;
    }
    n1 = NextName(n1);
    n2 = NextName(n2);
  }
  return ((n1==NULL)&&(n2==NULL));
}

/*
 * nameids  > subscripts.
 * this needs to be revisited when supported attributes are done.
 * longer names are > shorter names.
 */
int CompareNames(CONST struct Name *n1, CONST struct Name *n2)
{
  int ctmp;
  if (n1==n2) return 0;
  while ((n1!=NULL)&&(n2!=NULL)){
    if (NameId(n1)!=NameId(n2)) {
      /* if id status !=, then one must be id and other set */
      if (NameId(n1)) {
        return 1;
      } else {
        return -1;
      }
    }
    /* of same type: set or id */
    if (NameId(n1)){ /* id type */
      ctmp = 0;
      if (NameIdPtr(n1) != NameIdPtr(n2)) {
        ctmp = CmpSymchar(NameIdPtr(n1),NameIdPtr(n2));
      }
      if (ctmp!=0) return ctmp;
    } else { /* set type */
      ctmp = CompareSetStructures(NameSetPtr(n1),NameSetPtr(n2));
      if (ctmp!=0) return ctmp;
    }
    n1 = NextName(n1);
    n2 = NextName(n2);
  }
  if (n1!= NULL) {
    return 1;
  }
  if (n2!= NULL) {
    return -1;
  }
  return 0;
}

