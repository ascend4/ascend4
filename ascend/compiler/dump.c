/*
 *  Instance Garbage Dump
 *  by Tom Epperly
 *  10/24/89
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: dump.c,v $
 *  Date last modified: $Date: 1998/02/05 22:23:24 $
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
 *  Implementation of instance garbage dump using hash tables.
 */

#include <utilities/ascConfig.h>

#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include "instance_enum.h"
#include "cmpfunc.h"
#include "symtab.h"
#include "destroyinst.h"
#include "instquery.h"
#include "dump.h"

#ifndef lint
static CONST char GarbageDumpID[] = "$Id: dump.c,v 1.10 1998/02/05 22:23:24 ballan Exp $";
#endif

/*
 * hash table keyed by type name pointer. chain is for some
 * bizarre reason alphabetized within each bucket.
 */
#define DUMPHASHSIZE 1024
/*
 * macro for hashing pointers. shift, mask must match dump SIZE.
 */
#define DUMPHASHINDEX(p) (((((long) (p))*1103515245) >> 20) & 1023) 


#define DUMPLISTSIZE 10L

struct DumpRec {
  struct DumpRec *next;
  symchar *type;
  struct gl_list_t *instances;
};

struct DumpRec *g_dump_ht[DUMPHASHSIZE];
unsigned long g_dump_type_count,g_dump_inst_count;

void InitDump(void)
{
  register unsigned c;
  for(c=0;c<DUMPHASHSIZE;g_dump_ht[c++]=NULL);
  g_dump_type_count = 0;
  g_dump_inst_count = 0;
}

void EmptyTrash(void)
{
  register unsigned c;
  register unsigned long i,len;
  register struct DumpRec *p,*next;
  register struct gl_list_t *l;
  if (g_dump_inst_count==0) return;
  for(c=0;c<DUMPHASHSIZE;c++) {
    p = g_dump_ht[c];
    g_dump_ht[c]=NULL;
    while (p!=NULL) {
      AssertAllocatedMemory(p,sizeof(struct DumpRec));
      next = p->next;
      if ((l=p->instances)!=NULL) {
	len = gl_length(l);
	for(i=1;i<=len;i++)
	  DestroyInstance((struct Instance *)gl_fetch(l,i),NULL);
	gl_destroy(l);
      }
      ascfree((char *)p);
      p = next;
    }
  }
  g_dump_type_count = 0;
  g_dump_inst_count = 0;
}

void TendTrash(void)
{
  register unsigned c;
  register unsigned long i;
  register struct DumpRec *p;
  register struct gl_list_t *l;
  if (g_dump_inst_count <= MESSYTHRESH) return;
  for(c=0;c<DUMPHASHSIZE;c++) {
    p = g_dump_ht[c];
    while(p!=NULL) {
      if ((l=p->instances)!=NULL) {
	for (i=gl_length(l);i>MESSYTHRESH;i--) {
	  DestroyInstance((struct Instance *)gl_fetch(l,i),NULL);
	  g_dump_inst_count--;
	  gl_delete(l,i,0);
	}
      }
      p = p->next;
    }
  }
}

void TrashType(symchar *str)
{
  register unsigned long c,len,bucket;
  register struct DumpRec *p,*prev;
  register struct gl_list_t *l;
  register int cmp;
  assert(AscFindSymbol(str)!=NULL);
  if (*(SCP(str)) == '\0') return;
  bucket = DUMPHASHINDEX(SCP(str));
  if ((p = g_dump_ht[bucket])==NULL) return;
  cmp = CmpSymchar(p->type,str);
  if (cmp == 0)
    g_dump_ht[bucket] = p->next;
  else
    if (cmp > 0) return;
    else {
      prev = p;
      if ((p = p->next)==NULL) return;
      while((cmp=CmpSymchar(p->type,str))<0) {
	prev = p;
	if ((p=p->next)==NULL) return;
      }
      if (cmp!=0) return;
      /* remove from linked list */
      prev->next = p->next;
    }
  if ((l=p->instances)!=NULL) {
    len = gl_length(l);
    for(c=1;c<=len;c++)
      DestroyInstance((struct Instance *)gl_fetch(l,c),NULL);
    gl_destroy(l);
    g_dump_inst_count -= len;
  }
  g_dump_type_count--;
  ascfree((char *)p);
}

struct Instance *FindInstance(symchar *str)
{
  register struct DumpRec *p,*prev;
  register int cmp;
  register struct gl_list_t *l;
  register struct Instance *result;
  register unsigned long bucket;
  if (*(SCP(str)) == '\0') return NULL;
  bucket = DUMPHASHINDEX(SCP(str));
  if ((p = g_dump_ht[bucket])==NULL) return NULL;
  if ((cmp=CmpSymchar(p->type,str)) > 0) return NULL;
  else {
    if (cmp==0) {
      l = p->instances;
      result = (struct Instance *)gl_fetch(l,gl_length(l));
      gl_delete(l,gl_length(l),0);
      g_dump_inst_count--;
      if (gl_length(l)==0) {
	gl_destroy(l);
	g_dump_ht[bucket] = p->next;
	g_dump_type_count--;
	ascfree((char *)p);
      }
    }
    else {
      prev = p;
      if ((p = p->next)==NULL) return NULL;
      while((cmp=CmpSymchar(p->type,str))<0) {
	prev = p;
	if ((p=p->next)==NULL) return NULL;
      }
      if (cmp!=0) return NULL;
      /* pick out instance */
      l = p->instances;
      result = (struct Instance *)gl_fetch(l,gl_length(l));
      gl_delete(l,gl_length(l),0);
      g_dump_inst_count--;
      if (gl_length(l)==0) {
	gl_destroy(l);
	prev->next = p->next;
	g_dump_type_count--;
	ascfree((char *)p);
      }
    }
    return result;
  }
}

void AddInstance(struct Instance *i)

     /* add instance i to the trash dump */
{
  register struct DumpRec *p,*prev;
  register symchar *type;
  register int cmp;
  register unsigned long bucket;
  type = InstanceType(i);
  if ((i==NULL)||(type==NULL)) return;
  bucket = DUMPHASHINDEX(SCP(type));
  if ((p=g_dump_ht[bucket])==NULL) {
    g_dump_type_count++;
    g_dump_ht[bucket] = ASC_NEW(struct DumpRec);
    p = g_dump_ht[bucket];
    p->next = NULL;
  }
  else {
    cmp = CmpSymchar(p->type,type);
    if (cmp > 0) {
      g_dump_type_count++;
      p = ASC_NEW(struct DumpRec);
      p->next = g_dump_ht[bucket];
      g_dump_ht[bucket] = p;
    }
    else {
      if (cmp == 0) {
	gl_append_ptr(p->instances,(VOIDPTR)i);
	g_dump_inst_count++;
	return;
      }
      else {
	prev = p;
	if ((p = p->next)!=NULL)
	  while ((p!=NULL)&&(cmp=CmpSymchar(p->type,type)) <0) {
	    prev = p;
	    p = p->next;
	  }
	if (cmp == 0) { /* found a match */
	  gl_append_ptr(p->instances,(VOIDPTR)i);
	  g_dump_inst_count++;
	  return;
	}
	/* insert between prev and p */
	g_dump_type_count++;
	prev->next = ASC_NEW(struct DumpRec);
	prev->next->next = p;
	p = prev->next;
      }
    }
  }
  p->type = type;
  p->instances = gl_create(DUMPLISTSIZE);
  g_dump_inst_count++;
  gl_append_ptr(p->instances,(VOIDPTR)i);
}


