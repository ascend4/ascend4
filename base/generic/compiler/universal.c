/*
 *  Universal Routines
 *  by Tom Epperly
 *  Created: 3/27/1990
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: universal.c,v $
 *  Date last modified: $Date: 1997/07/18 12:36:19 $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

#include <utilities/ascConfig.h>
#include "compiler.h"
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "types.h"
#include "stattypes.h"
#include "child.h"
#include "type_desc.h"
#include "universal.h"

#ifndef lint
static CONST char UniversalModuleRCSid[] = "$Id: universal.c,v 1.7 1997/07/18 12:36:19 mthomas Exp $";
#endif

struct universal_rec {
  struct TypeDescription *desc;
  struct Instance *inst;
};

#define UNIVERSAL_TABLE_SIZE 15
struct UniversalTable *GlobalUniversalTable=NULL;

struct UniversalTable *CreateUniversalTable(void)
{
  return gl_create(UNIVERSAL_TABLE_SIZE);
}


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
struct UniversalTable *MergeUniversalTable(struct gl_list_t *table1,
					   struct gl_list_t *table2)
{
  (void)table1;  /*  stop gcc whine about unused varaible  */
  (void)table2;  /*  stop gcc whine about unused varaible  */
  return NULL;
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


void DestroyUniversalTable(struct gl_list_t *table)
{
  if (table) {
    gl_free_and_destroy(table);
  }
}

void SetUniversalTable(struct gl_list_t *table)
{
  GlobalUniversalTable = table;
}

struct UniversalTable *GetUniversalTable(void)
{
  return GlobalUniversalTable;
}

struct Instance *LookupInstance(struct gl_list_t *table,
				struct TypeDescription *desc)
{
  register unsigned long c;
  register struct universal_rec *ptr;
  if (table){
    for(c=gl_length(table);c>=1;--c){
      ptr = (struct universal_rec *)gl_fetch(table,c);
      AssertAllocatedMemory(ptr,sizeof(struct universal_rec));
      if (ptr->desc == desc) return ptr->inst;
    }
  }
  return NULL;
}

void AddUniversalInstance(struct gl_list_t *table,
			  struct TypeDescription *desc,
			  struct Instance *inst)
{
  struct universal_rec *ptr;
  if (table){
    ptr = (struct universal_rec *)ascmalloc(sizeof(struct universal_rec));
    AssertAllocatedMemory(ptr,sizeof(struct universal_rec));
    ptr->desc = desc;
    ptr->inst = inst;
    gl_append_ptr(table,(char *)ptr);
  }
}

void RemoveUniversalInstance(struct gl_list_t *table, struct Instance *inst)
{
  register unsigned long c;
  register struct universal_rec *ptr;
  if (table){
    for(c=gl_length(table);c>=1;--c){
      ptr = (struct universal_rec *)gl_fetch(table,c);
      AssertAllocatedMemory(ptr,sizeof(struct universal_rec));
      if (ptr->inst == inst){
	gl_delete(table,c,1);
	return;
      }
    }
  }
}

void ChangeUniversalInstance(struct gl_list_t *table,
			     struct Instance *oldinst,
			     struct Instance *newinst)
{
  register unsigned long c;
  register struct universal_rec *ptr;
  if (table){
    for(c=gl_length(table);c>=1;--c){
      ptr = (struct universal_rec *)gl_fetch(table,c);
      AssertAllocatedMemory(ptr,sizeof(struct universal_rec));
      if (ptr->inst == oldinst) {
	ptr->inst = newinst;
      }
    }
  }
}

unsigned long NumberTypes(struct gl_list_t *table)
{
  if (table) {
    return gl_length(table);
  } else {
    return 0;
  }
}

struct Instance *GetInstance(struct gl_list_t *table, unsigned long int pos)
{
  register struct universal_rec *ptr;
  if (table){
    ptr = (struct universal_rec *)gl_fetch(table,pos);
    AssertAllocatedMemory(ptr,sizeof(struct universal_rec));
    return ptr->inst;
  } else {
    return NULL;
  }
}

struct TypeDescription *GetTypeDescription(struct gl_list_t *table,
					   unsigned long int pos)
{
  register struct universal_rec *ptr;
  if (table){
    ptr = (struct universal_rec *)gl_fetch(table,pos);
    AssertAllocatedMemory(ptr,sizeof(struct universal_rec));
    return ptr->desc;
  } else {
    return NULL;
  }
}
