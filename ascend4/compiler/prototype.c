/*
 *  Prototype Library
 *  by Tom Epperly
 *  11/15/89
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: prototype.c,v $
 *  Date last modified: $Date: 1998/02/05 22:23:32 $
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
 *
 *
 *  This is a standard hash table for storing prototypes.
 */

#include<string.h>
#include<stdio.h>
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "compiler/symtab.h"
#include "compiler/instance_enum.h"
#include "compiler/prototype.h"
#include "compiler/instquery.h"
#include "compiler/destroyinst.h"

#ifndef lint
static CONST char PrototypeID[] = "$Id: prototype.c,v 1.9 1998/02/05 22:23:32 ballan Exp $";
#endif


#define PROTOTYPEHASHSIZE 1024
#define PROTOHASH(p) (((((long) (p))*1103515245) >> 20) & 1023)

struct ProtoRec {
  struct ProtoRec *next;
  symchar *t;
  struct Instance *i;
};

struct ProtoRec *g_proto_ht[PROTOTYPEHASHSIZE];
unsigned long g_proto_count=0;

void InitializePrototype(void)
{
  register unsigned c;
  for(c=0;c<PROTOTYPEHASHSIZE;g_proto_ht[c++]=NULL);
  g_proto_count = 0;
}

struct Instance *LookupPrototype(symchar *t)
{
  register struct ProtoRec *p;
  assert(AscFindSymbol(t)!=NULL);

  p = g_proto_ht[PROTOHASH(t)];
  while (p!=NULL) {
    if (t == p->t) {
      return p->i;
    }
    p = p->next;
  }
  return NULL;
}

void AddPrototype(struct Instance *i)
{
  register unsigned long bucket;
  register struct ProtoRec *p;
  register struct ProtoRec *prev;
  register symchar *t;

  t = InstanceType(i);
  p = g_proto_ht[bucket=PROTOHASH(t)];
  if (p==NULL) {
    /* initialize bucket */
    p=g_proto_ht[bucket] =
      (struct ProtoRec *)ascmalloc(sizeof(struct ProtoRec));
    g_proto_count++;
    p->next = NULL;
    p->t = t;
    p->i = i;
    return;
  }

  /* search down the list */
  prev = p;
  p = p->next;
  while (p!=NULL) {
    if (t == p->t) /* found a match */
      break;
    prev = p;
    p = p->next;
  }
  if (!p) { /* reached the end of the chain */
    p = g_proto_ht[bucket];
    g_proto_ht[bucket] =
      (struct ProtoRec *)ascmalloc(sizeof(struct ProtoRec));
    g_proto_count++;
    g_proto_ht[bucket]->next = p;
    p = g_proto_ht[bucket];
    p->t = t;
    p->i = i;
  }
  else{ /* matches ProtoRec p */
    /* update the instance information */
    FPRINTF(ASCERR,"Warning replacing prototype.\n");
    DestroyInstance(p->i,NULL);
    p->i = i;
  }
}


void DeletePrototype(symchar *t)
{
  register struct ProtoRec *p,**prev;
  register unsigned long bucket;
  assert(AscFindSymbol(t)!=NULL);

  p = g_proto_ht[bucket = PROTOHASH(t)];
  prev = &g_proto_ht[bucket];
  while (p!=NULL) {
    if (t == p->t) { /* found the type */
      *prev = p->next;
      p->next = NULL;
      DestroyInstance(p->i,NULL);
      p->i = NULL;
      p->t = NULL;
      ascfree(p);
      g_proto_count--;
      return;
    }
    else{
      prev = &p->next;
      p = p->next;
    }
  }
}

void DestroyPrototype(void)
{
  register unsigned c;
  register struct ProtoRec *p,*next;
  for(c=0;c<PROTOTYPEHASHSIZE;c++){
    if ((p=g_proto_ht[c])!=NULL) {
      g_proto_ht[c]=NULL;
      while (p!=NULL) {
	next = p->next;
	p->next = NULL;
	p->t = NULL;
	DestroyInstance(p->i,NULL);
	p->i = NULL;
	ascfree(p);
	p = next;
      }
    }
  }
  g_proto_count =0;
}
