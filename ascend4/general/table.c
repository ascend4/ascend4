/*
 *  Table Module
 *  by Kirk A. Abbott
 *  Created December 29, 1994.
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: table.c,v $
 *  Date last modified: $Date: 1998/06/16 15:47:46 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1994 Kirk Andre Abbott
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
k
 */
#include "utilities/ascConfig.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "general/hashpjw.h"
#include "general/table.h"

struct TableEntry {
  char *id;
  void *data;
  struct TableEntry *next;
};

struct Table {
  unsigned long hashsize; 	/* number of buckets */
  unsigned long size;		/* the current number of entries */
  struct TableEntry **buckets;
  struct TableEntry *lastfind;	/* a pointer into the last thing found */
};

struct Table *CreateTable(unsigned long hashsize)
{
  struct Table *result;
  struct TableEntry **buckets;
  result = (struct Table *)ascmalloc(sizeof(struct Table));
  buckets = (struct TableEntry **)
    asccalloc(hashsize,sizeof(struct TableEntry *));
  result->buckets = buckets;
  result->size = 0;
  result->hashsize = hashsize;
  result->lastfind = NULL;
  return result;
}

static void DestroyTableData(void *data, int info)
{
  if (info)
    ascfree((char *)data);
}

void DestroyTable(struct Table *table,int info)
{
  struct TableEntry *ptr, *next;
  unsigned long hashsize,c;
  if (table==NULL) return;
  hashsize = table->hashsize;
  for (c=0;c<hashsize;c++) {
    if (table->buckets[c]!=NULL) {
      ptr = table->buckets[c];
      while(ptr!=NULL) {
	DestroyTableData(ptr->data,info);	/* deallocate the data */
	next = ptr->next;
	ascfree((char *)ptr->id);		/* deallocate the string */
	ascfree((char *)ptr);			/* deallocate the node */
	table->size--;
	ptr = next;
      }
      table->buckets[c] = NULL;
    }
  }
  ascfree((char *)table->buckets);		/* deallocate the table */
  ascfree((char *)table);			/* deallocate the head */
}


void AddTableData(struct Table *table,void *data,CONST char *id)
{
  unsigned long c;
  struct TableEntry *ptr;

  assert(table&&data);
  c = hashpjw(id,table->hashsize);
  ptr = table->buckets[c];
  /* search for name collisions */
  while (ptr) {
    if (strcmp(id,ptr->id)==0)
      return;
    ptr = ptr->next;
  }
  /* add new information node to the head of the list. */
  ptr = (struct TableEntry *)
           ascmalloc(sizeof(struct TableEntry));
  ptr->id = (char *)ascmalloc((strlen(id)+1)*sizeof(char));
  strcpy(ptr->id,id);		/* we will copy the string */
  ptr->next = table->buckets[c];
  ptr->data = data;
  table->buckets[c] = ptr;
  table->size++;
  table->lastfind = ptr;
}

void *LookupTableData(struct Table *table,CONST char *id)
{
  unsigned long c;
  struct TableEntry *ptr;

  assert(table&&id);
  c = hashpjw(id,table->hashsize);
  ptr = table->buckets[c];
  while (ptr) {
    if (strcmp(id,ptr->id)==0) {
      table->lastfind = ptr;
      return ptr->data;
    }
    ptr = ptr->next;
  }
  table->lastfind = NULL;
  return NULL; /* name not found */
}

void *RemoveTableData(struct Table *table,char *id)
{
  unsigned long c;
  struct TableEntry *ptr, **tmp;
  void *result;

  assert(table&&id);
  c = hashpjw(id,table->hashsize);
  tmp = &table->buckets[c];
  ptr = table->buckets[c];
  while (ptr) {
    if (strcmp(id,ptr->id)==0) {
      *tmp = ptr->next;
      result = ptr->data;
      if (table->lastfind==ptr)	table->lastfind = NULL;
      ascfree((char *)ptr->id);	/* deallocate the string */
      ascfree((char *)ptr); /* deallocate the node */
      table->size--;
      return result;
    }
    tmp = &ptr->next;
    ptr = ptr->next;
  }
  return NULL; /* node info not found */
}

/*
 * Check our cached pointer first. If we dont find a match,
 * then do a Lookup. The lookup will reset the cached pointer.
 */
void TableApplyOne(struct Table *table,TableIteratorOne applyfunc,
		   char *id)
{
  void *data;

  assert(table&&id);
  if (table->lastfind) {
    if (strcmp(table->lastfind->id,id)==0) {
      (*applyfunc)(table->lastfind->data);
      return;
    }
  }
  data = LookupTableData(table,id);
  if (data)
    (*applyfunc)(data);
  return;
}

void TableApplyAll(struct Table *table,TableIteratorOne applyfunc)
{
  unsigned long c,hashsize;
  struct TableEntry *ptr;
  assert(table!=NULL);

  hashsize = table->hashsize;
  for (c=0;c<hashsize;c++) {
    ptr = table->buckets[c];
    while (ptr) {
      (*applyfunc)(ptr->data);
      ptr = ptr->next;
    }
  }
}

void TableApplyAllTwo(struct Table *table,TableIteratorTwo applyfunc,
		      void *arg2)
{
  unsigned long c,hashsize;
  struct TableEntry *ptr;
  assert(table!=NULL);

  hashsize = table->hashsize;
  for (c=0;c<hashsize;c++) {
    ptr = table->buckets[c];
    while (ptr) {
      (*applyfunc)(ptr->data,arg2);
      ptr = ptr->next;
    }
  }
}

void PrintTable(FILE *f,struct Table *table)
{
  unsigned long c,hashsize;
  struct TableEntry *ptr;
  unsigned long entrynum = 1;
  assert(table!=NULL);

  hashsize = table->hashsize;
  for (c=0;c<hashsize;c++) {
    ptr = table->buckets[c];
    while (ptr) {
      FPRINTF(f,"Entry %lu\tBucket %lu\tId %s\n",
	      entrynum++,c,ptr->id);
      ptr = ptr->next;
    }
  }
}


unsigned long TableSize(struct Table *table)
{
  assert(table!=NULL);
  return table->size;
}

unsigned long TableHashSize(struct Table *table)
{
  assert(table!=NULL);
  return table->hashsize;
}

void *TableLastFind(struct Table *table)
{
  assert(table!=NULL);
  return table->lastfind;
}

