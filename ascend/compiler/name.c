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
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include<stdio.h>
#include<assert.h>
#include <ascend/general/platform.h>

#include "instance_enum.h"
#include "cmpfunc.h"
#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/general/pool.h>


#include "functype.h"
#include "func.h"
#include "expr_types.h"
#include "exprs.h"
#include "sets.h"
#include "name.h"
#include "vlist.h"
#include "nameio.h"

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
static struct Name *AllocName(){
		
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
    ASC_PANIC("ERROR: name_init_pool called twice.\n");
  }
  g_name_pool = pool_create_store(NP_LEN, NP_WID, NP_ELT_SIZE,
    NP_MORE_ELTS, NP_MORE_BARS);
  if (g_name_pool == NULL) {
    ASC_PANIC("ERROR: name_init_pool unable to allocate pool.\n");
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
  bits &= (NAMEBIT_IDTY | NAMEBIT_ATTR | NAMEBIT_AUTO | NAMEBIT_CHAT | NAMEBIT_DERIV | NAMEBIT_PRE);
  result->bits = bits;
  result->val.id = s;
  result->next = NULL;
  return result;
}

symchar *SimpleNameIdPtr(CONST struct Name *nptr)
{
  if (nptr==NULL) return NULL;
  if (NextName(nptr)!=NULL) return NULL;
  return (NameId(nptr) || NameDeriv(nptr) || NamePre(nptr)) ? NameIdPtr(nptr) : NULL;
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

struct DerName *CreateDeriv(struct VariableList *vlist)
{
  register struct DerName *result;
  assert(vlist!=NULL);
  result = ASC_NEW(struct DerName);
  assert(result!=NULL);
  result->vlist = vlist;
  result->strname = NULL;
  return result;
}

struct Name *CreateDerivName(struct DerName *der)
{
  register struct Name *result;
  assert(der!=NULL);
  result = IDNMALLOC;
  assert(result!=NULL);
  result->bits = NAMEBIT_DERIV;
  result->val.der = der;
  result->next = NULL;
  return result;
}

struct PreName *CreatePre(struct Name *n)
{
  register struct PreName *result;
  assert(n!=NULL);
  result = ASC_NEW(struct PreName);
  assert(result!=NULL);
  result->n = n;
  result->strname = NULL;
  return result;
}

struct Name *CreatePreName(struct PreName *pre)
{
  register struct Name *result;
  assert(pre!=NULL);
  result = IDNMALLOC;
  assert(result!=NULL);
  result->bits = NAMEBIT_PRE;
  result->val.pre = pre;
  result->next = NULL;
  return result;
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

struct Name *CreateReservedIndexName(symchar *reserved)
{
  struct Name *n;
  struct Expr *ex;
  struct Set *s;
  struct Name *result;
  n = CreateIdName(reserved);
  ex = CreateVarExpr(n);
  s = CreateSingleSet(ex);
  result = CreateSetName(s);
  return result;
}

struct Name *CreateEnumElementName(symchar *senum)
{
  struct Expr *ex;
  struct Set *s;
  struct Name *result;

  ex = CreateSymbolExpr(senum);
  s = CreateSingleSet(ex);
  result = CreateSetName(s);

  return result;
}

struct Name *CreateIntegerElementName(long i)
{
  struct Expr *ex;
  struct Set *s;
  struct Name *result;

  ex = CreateIntExpr(i);
  s = CreateSingleSet(ex);
  result = CreateSetName(s);

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

int NameDerivF(CONST struct Name *n)
{
  if(n==NULL) return 0;
  return ((n->bits & NAMEBIT_DERIV) != 0);
}

int NamePreF(CONST struct Name *n)
{
  if(n==NULL) return 0;
  return ((n->bits & NAMEBIT_PRE) != 0);
}

int NameAutoF(CONST struct Name *n)
{
  assert(n!=NULL);
  return ((n->bits &( NAMEBIT_IDTY | NAMEBIT_AUTO)) != 0);
}

symchar *NameIdPtrF(CONST struct Name *n)
{
  assert(n!=NULL);
  if (NameId(n)) return n->val.id;
  else {
    if (NameDeriv(n)) return n->val.der->strname;
    else return n->val.pre->strname;
  }
}

CONST struct Set *NameSetPtrF(CONST struct Name *n)
{
  assert(n!=NULL);
  return n->val.s;
}

CONST struct DerName *NameDerPtrF(CONST struct Name *n)
{
  assert(n!=NULL);
  return n->val.der;
}

symchar *DerStrPtrF(CONST struct DerName *n)
{
  assert(n!=NULL);
  return n->strname;
}

struct VariableList *DerVlistF(CONST struct DerName *n)
{
  assert(n!=NULL);
  return n->vlist;
}

struct DerName *CopyDerName(CONST struct DerName *n)
{
  register struct DerName *result;
  if (n==NULL) return NULL;
  result = ASC_NEW(struct DerName);
  result->vlist = CopyVariableList(n->vlist);
  result->strname = n->strname;
  return result;
}

CONST struct PreName *NamePrePtrF(CONST struct Name *n)
{
  assert(n!=NULL);
  return n->val.pre;
}

symchar *PreStrPtrF(CONST struct PreName *n)
{
  assert(n!=NULL);
  return n->strname;
}

struct Name *PreNameF(CONST struct PreName *n)
{
  assert(n!=NULL);
  return n->n;
}

struct PreName *CopyPreName(CONST struct PreName *n)
{
  register struct PreName *result;
  if (n==NULL) return NULL;
  result = ASC_NEW(struct PreName);
  result->n = CopyName(n->n);
  result->strname = n->strname;
  return result;
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
    if (np->bits & NAMEBIT_DERIV) {
      result->val.der = CopyDerName(np->val.der);
    }else{
      if (np->bits & NAMEBIT_PRE) {
        result->val.pre = CopyPreName(np->val.pre);
      }else{
        result->val.s = CopySetList(np->val.s);
      }
    }
  }
  p = result;
   while (np->next!=NULL) {
     p->next = IDNMALLOC;
     p = p->next;
     np = np->next;
     *p = *np;
     if (!(np->bits & NAMEBIT_IDTY)) {
       if (np->bits & NAMEBIT_DERIV) {
         p->val.der = CopyDerName(np->val.der);
       }else{
         if (np->bits & NAMEBIT_PRE) {
           p->val.pre = CopyPreName(np->val.pre);
         }else{
           p->val.s = CopySetList(np->val.s);
         }
       }
     }
   }
  return result;
}

struct Name *CopyAppendNameNode(CONST struct Name *n, CONST struct Name *node)
{
  struct Name *result = NULL, *tmp = NULL;
  if (n != NULL) {
    result = CopyName(n);
  }
  if (node != NULL) {
    tmp = IDNMALLOC;
    *tmp = *node;
    if (!(tmp->bits & NAMEBIT_IDTY)) {
      if (tmp->bits & NAMEBIT_DERIV) {
        tmp->val.der = CopyDerName(node->val.der);
      }else{
        if (tmp->bits & NAMEBIT_PRE) {
          tmp->val.pre = CopyPreName(node->val.pre);
        }else{
          tmp->val.s = CopySetList(node->val.s);
        }
      }
    }
    tmp->next = NULL;
  }
  result = JoinNames(result,tmp);
  return result;
}

struct Name *AppendNameNode(struct Name *n1, CONST struct Name *n2)
{
  register struct Name *p, *tmp;
  tmp = NULL;
  if (n2 != NULL) {
    tmp = IDNMALLOC;
    *tmp = *n2;
    if (!(tmp->bits & NAMEBIT_IDTY)) {
      if (tmp->bits & NAMEBIT_DERIV) {
        tmp->val.der = CopyDerName(n2->val.der);
      }else{
        if (tmp->bits & NAMEBIT_PRE) {
          tmp->val.pre = CopyPreName(n2->val.pre);
        }else{
          tmp->val.s = CopySetList(n2->val.s);
        }
      }
    }
    tmp->next = NULL;
  }
  if (!n1) return tmp;
  /* find end of name list */
  p = n1;
  while (p->next) {
    p = p->next;
  }
  /* link to n2 */
  p->next = tmp;
  return n1;
}

void DestroyName(register struct Name *n)
{
  register struct Name *next;
  while(n!=NULL) {
    next = n->next;
    if (!(n->bits & NAMEBIT_IDTY)) {
      if (n->bits & NAMEBIT_DERIV) {
        DestroyDerName(n->val.der);
      }else{
        if (n->bits & NAMEBIT_PRE) {
          DestroyPreName(n->val.pre);
        }else{
          DestroySetList(n->val.s);
        }
      }
    }
    IDNFREE((char *)n);
#if NAMEDEBUG
    g_num_names_cur--;
#endif
    n = next;
  }
}

void DestroyDerName(register struct DerName *der)
{
  if (der!=NULL) {
    DestroyVariableList(der->vlist);
    ascfree((char *)der);
  }
}

void DestroyPreName(register struct PreName *pre)
{
  if (pre!=NULL) {
    DestroyName(pre->n);
    ascfree((char *)pre);
  }
}

void DestroyNamePtr(struct Name *n)
{
  if (n!=NULL) {
    if (!(n->bits & NAMEBIT_IDTY)) {
      if (n->bits & NAMEBIT_DERIV) {
        DestroyDerName(n->val.der);
      }else{
        if (n->bits & NAMEBIT_PRE) {
          DestroyPreName(n->val.pre);
        }else{
          DestroySetList(n->val.s);
        }
      }
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
  while (p->next) {
    p = p->next;
  }
  /* link to n2 */
  p->next = n2;
  return n1;
}

CONST struct Name *NextIdName(register CONST struct Name *n)
{
  if (n==NULL) return NULL;
  assert(NameId(n)!=0 || NameDeriv(n)!=0 || NamePre(n)!=0);
  n = n->next;
  while (n!=NULL && !NameId(n) && !NameDeriv(n) && !NamePre(n)) {
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
    if (NameId(n)!=0 || NameDeriv(n)!=0 || NamePre(n)!=0) {
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
    if (NameDeriv(n1)!=NameDeriv(n2)) return 0;
    if (NamePre(n1)!=NamePre(n2)) return 0;
    if (NameId(n1)){
      if (NameIdPtr(n1) != NameIdPtr(n2)) return 0;
    } else {
      if (NameDeriv(n1)){
        if (CompareDers(NameDerPtr(n1),NameDerPtr(n2))) return 0;
      }else{
        if (NamePre(n1)) {
          if (ComparePres(NamePrePtr(n1),NamePrePtr(n2))) return 0;
        }else{
          if (!SetStructuresEqual(NameSetPtr(n1),NameSetPtr(n2))) return 0;
        }
      }
    }
    n1 = NextName(n1);
    n2 = NextName(n2);
  }
  return ((n1==NULL)&&(n2==NULL));
}

/*
 * pres > derivatives > nameids  > subscripts.
 * this needs to be revisited when supported attributes are done.
 * longer names are > shorter names.
 */
int CompareNames(CONST struct Name *n1, CONST struct Name *n2)
{
  int ctmp;
  if (n1==n2) return 0;
  while ((n1!=NULL)&&(n2!=NULL)){
    if (NameId(n1)!=NameId(n2) || NameDeriv(n1)!=NameDeriv(n2) || NamePre(n1)!=NamePre(n2)) {
      if (NameId(n1)) { /* If n1 is an id, n2 can be either a derivative or a pre or a subscript */
	if (NameDeriv(n2) || NamePre(n2)) return -1;
        else return 1;
      } else if (NameDeriv(n1)) {
        if (NamePre(n2)) return -1;
        else return 1;
      } else if (NamePre(n1)) {
        return 1;
      } else { /* n1 is a subscript */
        return -1;
      }
    }
    /* of same type: set or id or derivative */
    if (NameId(n1)){ /* id type */
      ctmp = 0;
      if (NameIdPtr(n1) != NameIdPtr(n2)) {
        ctmp = CmpSymchar(NameIdPtr(n1),NameIdPtr(n2));
      }
      if (ctmp!=0) return ctmp;
    } else if (NameDeriv(n1)) { /* derivative type */
      ctmp = CompareDers(NameDerPtr(n1),NameDerPtr(n2));
      if (ctmp!=0) return ctmp;
    } else if (NamePre(n1)) {
      ctmp = ComparePres(NamePrePtr(n1),NamePrePtr(n2));
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

int CompareDers(CONST struct DerName *n1, CONST struct DerName *n2)
{
 if (n1==n2) return 0;
 return CompareVariableLists(n1->vlist,n2->vlist);
}

int ComparePres(CONST struct PreName *n1, CONST struct PreName *n2)
{
 if (n1==n2) return 0;
 return CompareNames(n1->n,n2->n);
}

