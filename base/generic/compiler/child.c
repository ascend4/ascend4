/*
 *
 *  Child List Implementation
 *  by Tom Epperly
 *  Version: $Revision: 1.25 $
 *  Version control file: $RCSfile: child.c,v $
 *  Date last modified: $Date: 1998/03/26 20:39:34 $
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
 *  Implementation of Child list stuff.
 */

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/statio.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/cmpfunc.h"
#define __CHILD_ILLEGAL_ACCESS__
#include "compiler/childpriv.h"
#include "compiler/childio.h"

#ifndef lint
static CONST char ChildListID[] = "$Id: child.c,v 1.25 1998/03/26 20:39:34 ballan Exp $";
#endif

#define NEWCL 1
/* origin of sanity (or sanity of origin) check. dependent on header */
#define LegalOrigin(n) ( ((n)>0 && (n) <=8) ? (n) : 0 )

/* replace the following with a pool maybe */
#define CLPOOLALLOC ((struct ChildListEntry *) \
  ascmalloc(sizeof(struct ChildListEntry)))
#define CLPOOLFREE(p) ascfree(p)

/* copy the contents of a entry pointer to another ptr */
#define CLCOPY(src) copylistentry(src)

/* wrappers for mem management if we want to pool heads later. unlikely */
#define CLSMALLOC \
  (struct ChildListStructure *)ascmalloc(sizeof(struct ChildListStructure))
#define CLSFREE(clp) ascfree(clp)

/* returns an array of size n of childhash to be tabled singly. */
#define CHMALLOC(n) (struct ChildHash *)ascmalloc(sizeof(struct ChildHash)*n)
/* destroys all the hash entries of the type at one shot */
#define CHFREE(chp) ascfree(chp)

/* string comparison function of names */
int CmpChildListEntries(CONST struct ChildListEntry *e1,
                        CONST struct ChildListEntry *e2)
{
  assert(e1!=NULL);
  assert(e2!=NULL);
  assert(e1->strptr!=NULL);
  assert(e2->strptr!=NULL);
  if (e1->strptr==e2->strptr) return 0;
  return CmpSymchar(e1->strptr,e2->strptr);
}

static
struct ChildListEntry *copylistentry(struct ChildListEntry *old)
{
  struct ChildListEntry *new;
  new = CLPOOLALLOC;
  new->strptr = old->strptr;

  new->typeptr = old->typeptr;
  CopyTypeDesc((struct TypeDescription *)old->typeptr);

  new->statement = old->statement;
  new->bflags = old->bflags;
  new->isarray = (old->isarray > 0) ? old->isarray : 0;
  new->origin = LegalOrigin(old->origin);
  return new;
}

static
int cl_inputIsOK(struct gl_list_t *l)
{
  unsigned long c,length;
  struct ChildListEntry *old;
  length = gl_length(l);
  for (c=1;c<=length;c++) {
    old = GGET(l,c);
    if (old->typeptr == NULL) {
      return 0;
    }
    assert(AscFindSymbol(old->strptr) != NULL);
  }
  return 1;
}

/*
 * assumes a fully allocated ChildListPtr and that it is ok to
 * overwrite any previous hash data.
 */
static void BuildChildHashTable(struct ChildListStructure *result)
{
  unsigned long c,len;
  struct ChildListEntry *old;
  long htindex;
  struct ChildHash *chp;

  len = gl_length(result->list);
  for (htindex = CHILDHASHTABLESIZE-1; htindex >= 0; htindex--) {
    result->table[htindex] = NULL;
  }
  for (c = 1; c <= len; c++) {
    old = CGET(result,c);
    chp = &(result->data[c-1]);
    chp->clindex = c;			/* data we want fast in childpos */
    chp->sym = old->strptr;		/* hash key on pointer, not content. */
    htindex = CHILDHASHINDEX(chp->sym);	/* could save if symtab calc index */
    chp->next = result->table[htindex];	/* stick in bucket list. */
    result->table[htindex] = chp;	
  }
}

ChildListPtr CreateChildList(struct gl_list_t *l)
{
  struct ChildListEntry *old;
  struct ChildListStructure *result;
  register unsigned long c,length;
  assert(l!=NULL);

  length = gl_length(l);
  if (!cl_inputIsOK(l)) {
    return NULL;
  }
  result = CLSMALLOC;
  assert(result != NULL); /* should be panic */
  result->list = gl_create(length);
  assert(result->list != NULL); /* should be panic */
  result->symbols = gl_create(length);
  assert(result->symbols != NULL); /* should be panic */
  result->data = CHMALLOC(length);
  assert(result->data != NULL); /* should be panic */
  if (!gl_sorted(l)) gl_sort(l,(CmpFunc)CmpChildListEntries);
  for (c=1;c<=length;c++) {
    old = GGET(l,c);
    assert(old!=NULL);
    CPUT(result,CLCOPY(old)); /* uses copylistentry(old) */
    gl_append_ptr(result->symbols,(VOIDPTR)old->strptr);
  }
  BuildChildHashTable(result);
  return result;
}

#define TESTCL 0
#if TESTCL
/* shift is number of bits to rotate right: 32 - n - 2
 * on 4 byte ptr hardware. mask is 2^n - 1,
 * the lower order bits to use for the result.
 * The result is an index into an array size 2^n.
 */
static unsigned long heaphashptr(void *p,int shift,unsigned long mask)
{
  return (((((long) (p))*1103515245) >> shift) & mask);
}

/* rewrite this to check collisions */
static void
analyzesyms(struct gl_list_t *sl)
{
  unsigned long int c,len;
  char *p, *minp, *maxp;
  int maxlen,slen;
  if (sl==NULL) return;
  len = gl_length(sl);
  if (len<10) return;
  FPRINTF(ASCERR,"\nLIST\n");
  minp = (char *)ULONG_MAX;
  maxp = NULL;
  maxlen = 0;
  for (c = 1; c <= len; c++) {
    p =(char *)gl_fetch(sl,c);
    slen = strlen(p);
    if (slen > maxlen) maxlen = slen;
    if (p > maxp) maxp = p;
    if (p < minp) minp = p;
  }
  for (c = 1; c <= len; c++) {
    p =(char *)gl_fetch(sl,c);
    FPRINTF(ASCERR,"%lu\t%lu\t(%lu\t%lu\t%lu\t%lu)\t%s\n",
        c,(unsigned long)p-(unsigned long)minp,
        heaphashptr(p,28,3),
        heaphashptr(p,26,15),
        heaphashptr(p,24,63),
        heaphashptr(p,22,255),
        p);
  }
  FPRINTF(ASCERR,"LONGEST: %d\nLEAST: %lu\n",maxlen,(unsigned long)minp);
}
#endif /* testcl */

void DestroyChildList(ChildListPtr clp)
{
  unsigned long int c,len;
  struct ChildListEntry *old;
  struct ChildListStructure *cl;

  cl = (struct ChildListStructure *)clp;
  assert(cl!=NULL);
  assert(cl->list!=NULL);

  len = gl_length(GL(cl));
  for (c=1;c<=len;c++) {
    old = CGET(cl,c);
    DeleteTypeDesc((struct TypeDescription *)old->typeptr);
    old->typeptr = NULL;
    CLPOOLFREE(old);
  }
  gl_destroy(cl->list);
#if TESTCL
  analyzesyms(cl->symbols);
#endif /*testing */
  gl_destroy(cl->symbols);
  cl->list = NULL;
  cl->symbols = NULL;
  CHFREE(cl->data);	/* destroys contents of table. */
  CLSFREE(cl);		/* destroys table and head. */
}

ChildListPtr AppendChildList(ChildListPtr cl,
                             struct gl_list_t *l)
{
  struct ChildListStructure *result;
  struct ChildListEntry *cle, *gle;
  register unsigned long length,cln,gln;
  register unsigned long clength;
#ifndef NDEBUG
  register unsigned long totlen;
#endif
  assert(l!=NULL);
  assert(cl!=NULL);
  if (!cl_inputIsOK(l)) {
    return NULL;
  }
  length = gl_length(l);
  if (!gl_sorted(l)) gl_sort(l,(CmpFunc)CmpChildListEntries);

  clength = gl_length(GL(cl));
  if (clength == 0) {
    return CreateChildList(l);
  }
  if (length == 0) {
    return CreateChildList(GL(cl));
  }
  result = CLSMALLOC;
  result->list = gl_create(clength+length);
  result->symbols = gl_create(clength+length);
  result->data = CHMALLOC(clength+length);
#ifndef NDEBUG
  totlen = (clength+length);
#endif

  /* merge two sorted lists */
  /*  cle = clnth element of cl.  */
  /*  gle = glnth element of gl.  */
  gln=1;
  cln=1;
  gle = gl_fetch(l,gln);
  cle = CGET(cl,cln);
  while (1) {
    /* once here always returns to caller for finite lists */
    if (CmpChildListEntries(gle,cle)>0) {
      CPUT(result,CLCOPY(cle));
      gl_append_ptr(result->symbols,(VOIDPTR)cle->strptr);
      cle = CGET(cl,++cln);
      if (--clength==0) {
        /* copy tail of l */
        while (gln++ <= length) {
          cle = GGET(l,gln);
          CPUT(result,CLCOPY(cle));
          gl_append_ptr(result->symbols,(VOIDPTR)cle->strptr);
        }
        assert(totlen == gl_length(result->list));
        BuildChildHashTable(result);
        return result;
      }
    } else {
      CPUT(result,CLCOPY(gle));
      gl_append_ptr(result->symbols,
                    (VOIDPTR)((struct ChildListEntry *)gle)->strptr);
      if (++gln <= length) {
        gle = gl_fetch(l,gln);
      } else {
        /* copy tail of cl */
        while(clength-->0) {
          cle = CGET(cl,cln);
          CPUT(result,CLCOPY(cle));
          gl_append_ptr(result->symbols, (VOIDPTR)cle->strptr);
          cln++;
        }
        assert(totlen== gl_length(result->list));
        BuildChildHashTable(result);
        return result;
      }
    }
  }
  /* not reached */
}

unsigned long ChildListLen(ChildListPtr cl)
{
  assert(cl!=NULL); /* even childless defs are supposed to have empty list */
  return gl_length(GL(cl));
}

symchar *ChildStrPtr(ChildListPtr cl, unsigned long int n)
{
  assert(cl!=NULL && cl->symbols!=NULL && CNAME(cl,n) != NULL);
  return CNAME(cl,n);
}

unsigned int ChildIsArray(ChildListPtr cl, unsigned long int n)
{
  return (unsigned int)CGET(cl,n)->isarray;
}

unsigned int ChildOrigin(ChildListPtr cl, unsigned long int n)
{
  return CGET(cl,n)->origin;
}

unsigned int ChildAliasing(ChildListPtr cl, unsigned long int n)
{
  unsigned result, origin;
  origin = CGET(cl,n)->origin;
  result = AliasingOrigin(origin); 
  return result;
}

unsigned int ChildParametric(ChildListPtr cl, unsigned long int n)
{
  return ParametricOrigin(CGET(cl,n)->origin);
}

CONST struct Statement *ChildStatement(ChildListPtr cl, unsigned long int n)
{
  assert(cl!=NULL && CGET(cl,n)->statement!=NULL);
  return CGET(cl,n)->statement;
}

unsigned ChildGetBooleans(ChildListPtr cl, unsigned long int n)
{
  assert(cl!=NULL && n && n <= gl_length(GL(cl)));
  return CGET(cl,n)->bflags;
}

void ChildSetBoolean(ChildListPtr cl, unsigned long int n,
                     unsigned flag, unsigned bool)
{
  assert(cl!=NULL && n && n <= gl_length(GL(cl)));
  switch (flag) {
  case CBF_VISIBLE:
  case CBF_SUPPORTED:
  case CBF_PASSED:
    break;
  default:
    return;
  }
  if (bool==1) {
    CGET(cl,n)->bflags |= flag;
  } else {
    CGET(cl,n)->bflags &= ~flag;
  }
}

CONST struct TypeDescription *ChildBaseTypePtr(ChildListPtr cl,
                                               unsigned long int n)
{
  assert(cl!=NULL);
  return CGET(cl,n)->typeptr;
}

/*
Use pointer hash table to locate the string.
(removed binary search stuff here -- johnpye 20051208 )
*/
unsigned long
ChildPos(ChildListPtr cl, symchar *s)
{
  struct ChildHash *chp;
  assert(AscFindSymbol(s) != NULL); /* baa 1/98 */
  chp = cl->table[CHILDHASHINDEX(s)];
  while (chp != NULL) {
    if (chp->sym == s) { /* usually right first time */
      return chp->clindex;
    }
    chp = chp->next;
  }
  return 0;
}

extern int CompareChildLists(ChildListPtr cl1,
                             ChildListPtr cl2,
                             unsigned long *diff)
{
  struct ChildListEntry *cle1, *cle2;
  unsigned long int len1, len2, c, len;
  int ctmp;
  if (cl1==cl2) {
    return 0;
  }
  ctmp = 0;
  assert(cl1!= NULL);
  assert(cl2!= NULL);
  assert(diff != NULL);
  *diff = 0;
  len1=ChildListLen(cl1);
  len2=ChildListLen(cl2);
  /* do not return early just because len1 != len2. want to check
   * equivalency up to the length of the shorter.
   */
  len = MIN(len1,len2);
  if (len==0) {
    /* curiously  empty lists > lists with something */
    if (len1) return -1;
    if (len2) return 1;
    return 0;
  }
  for (c=1; c <= len; c++) { /* we always enter this loop */
    cle1 = CGET(cl1,c);
    cle2 = CGET(cl2,c);
    if (cle1->strptr != cle2->strptr) {
      ctmp = CmpSymchar(cle1->strptr , cle2->strptr);
      assert(ctmp);
      break;
    }
    if (cle1->typeptr != cle2->typeptr) {
      ctmp = (GetParseId(cle1->typeptr) > GetParseId(cle2->typeptr) ? 1 : -1);
      break;
    }
    if (cle1->bflags != cle2->bflags) {
      ctmp = ((cle1->bflags > cle2->bflags) ? 1 : -1);
      break;
    }
    if (cle1->origin != cle2->origin) {
      ctmp = ((cle1->origin > cle2->origin) ? 1 : -1);
      break;
    }
    if (cle1->isarray != cle2->isarray) {
      ctmp = ((cle1->isarray > cle2->isarray) ? 1 : -1);
      break;
    }
    /* passing bflags, origin, and isarray tests pretty much assures
     * passing statement comparison test, too.
     */
  }
  *diff = c;
  if (c <= len) {
    /* finished before end of short list */
    return ctmp;
  }
  if (len1 == len2) {
    /* same len. finished both lists */
    *diff = 0;
    return 0;
  }
  if (len > len2) {
    /* identical up to len. list length decides */
    return 1;
  } else {
    return -1;
  }
}

#if 0 /* to childio.c */
void WriteChildList(FILE *fp,ChildListPtr cl)
{
  unsigned long c,len;
  struct ChildListEntry *cle;
  CONST struct gl_list_t *l;
  if (cl!=NULL) {
    l = GL(cl);
    len = gl_length(l);
    if (!len) {
      FPRINTF(fp,"Child list is empty\n");
      return;
    }
    FPRINTF(fp,"Child list is %lu long.\n",len);
    for (c=1;c<=len;c++) {
      cle = GGET(l,c);
      if (cle!=NULL) {
        FPRINTF(fp,"%lu name=\"%s\" type=\"%s\" ARRAY=%d origin=%d bits=%u\n",
          c,
          SCP(cle->strptr),
          ((cle->typeptr==NULL)?"UNKNOWN":SCP(GetName(cle->typeptr))),
          cle->isarray,
          (int)cle->origin,cle->bflags);
        WSEM(fp,cle->statement,"  Declared at ");
      } else {
        FPRINTF(fp,"Child list item %lu is empty!\n",c);
      }
    }
  }
}
#endif /* migrated */
