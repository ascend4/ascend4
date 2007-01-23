/*
 *  Procedure Data Structure Implementation
 *  by Tom Epperly
 *  Created: 1/10/90
 *  Version: $Revision: 1.18 $
 *  Version control file: $RCSfile: proc.c,v $
 *  Date last modified: $Date: 1998/04/11 01:31:21 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1998 Carnegie Mellon University
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
#include <utilities/ascMalloc.h>

#include "symtab.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include <general/list.h>


#include "functype.h"
#include "expr_types.h"
#include "stattypes.h"
#include "statement.h"
#include "slist.h"
#include "proc.h"

#define PMALLOC(x) x = ASC_NEW(struct InitProcedure)
#ifndef lint
static CONST char ProcedureRCSid[] = "$Id: proc.c,v 1.18 1998/04/11 01:31:21 ballan Exp $";
#endif

/*
 * UNIVERSAL method list for all models.
 * If a method lookup fails, this list is consulted.
 * more or less this exists because there isn't an
 * explicit root MODEL in the implementation.
 */
static struct gl_list_t *g_model_definition_methods = NULL;
#define GMDM g_model_definition_methods

struct InitProcedure *CreateProcedure(symchar *name,
				      struct StatementList *stats)
{
  register struct InitProcedure *result;
  assert(AscFindSymbol(name)!=NULL);
  PMALLOC(result);
  assert(result!=NULL);
  result->name = name;
  result->slist = stats;
  result->parse_id = 0;
  if (stats!=NULL) AddContext(stats,context_METH);
  return result;
}

struct InitProcedure *CopyProcedure(struct InitProcedure *p)
{
  register struct InitProcedure *result;
  assert(p!=NULL);
  PMALLOC(result);
  assert(result!=NULL);
  result->name = p->name;
  result->parse_id = p->parse_id;
  result->slist = CopyStatementList(p->slist);
  return result;
}

struct InitProcedure *CopyProcToModify(struct InitProcedure *p)
{
  register struct InitProcedure *result;
  assert(p!=NULL);
  PMALLOC(result);
  assert(result!=NULL);
  result->name = p->name;
  result->parse_id = 0;
  result->slist = CopyListToModify(p->slist);
  return result;
}

struct gl_list_t *MergeProcedureLists(struct gl_list_t *old,
                                      struct gl_list_t *new)
{
  register struct gl_list_t *result;
  register struct InitProcedure *proc;
  register unsigned long c,len;
  register unsigned long refproc;
  if (old==NULL) {
    return new;
  }
  result = gl_create(gl_length(old)+gl_safe_length(new));
  if (new!=NULL){
    len = gl_length(new);
    for(c=1;c<=len;c++){
      proc = (struct InitProcedure *)gl_fetch(new,c);
      gl_append_ptr(result,(VOIDPTR)proc);
    }                                 /* dont destroy the list yet */
  }
  len = gl_length(old);
  /* this is where method inheritance rules occur. We keep all the
   * methods from the new in result, then we search for any method in
   * the old list which does not have an identically named
   * counterpart in the new list and add it to the result.
   * Methods in the old result which have be redefined in the new
   * list given are ignored.
   *
   * Once a result list is achieved the new list (but not the methods
   * in it) may be destroyed since the methods are all stored in result.
   */
  if (new!=NULL) {
    for(c=1;c<=len;c++) {
      proc = (struct InitProcedure *)gl_fetch(old,c);
      refproc = gl_search(new,proc,(CmpFunc)CmpProcs);
      /* refproc will be 0 if no match */
      if (refproc==0) {
        /* hence append to result */
        gl_append_ptr(result,(VOIDPTR)CopyProcedure(proc));
      }
    }
    gl_destroy(new);
  } else {
    for(c=1;c<=len;c++){
      proc = (struct InitProcedure *)gl_fetch(old,c);
      gl_append_ptr(result,(VOIDPTR)CopyProcedure(proc));
    }
  }
  gl_sort(result,(CmpFunc)CmpProcs);
  return result;
}

void DestroyProcedure(struct InitProcedure *p)
{
  if (p!=NULL){
    /* the following only destroys a reference to p->slist
     * unless of course it is the last reference to p->slist.
     * The name of the method belongs to the symbol table so must
     * not be destroyed here.
     */
    DestroyStatementList(p->slist);
    p->parse_id = -1;
    ascfree((char *)p);
  }
}

void DestroyProcedureList(struct gl_list_t *pl)
{
  if (pl!=NULL){
    gl_iterate(pl,(void (*)(VOIDPTR))DestroyProcedure);
    gl_destroy(pl);
  }
}

/*
 * Returns the list of methods defined for all MODELs
 * unless they redefine the methods themselves.
 */
struct gl_list_t *GetUniversalProcedureList(void)
{
  return GMDM;
}

/*
 * Sets the list of procedures defined for all MODELs.
 * If a UPL already exists, it will be destroyed unless it
 * is the same. If the same, it is resorted.
 */
void SetUniversalProcedureList(struct gl_list_t *upl)
{
  if (GMDM != upl) {
    DestroyProcedureList(GMDM);
    GMDM = upl;
  }
  if (GMDM != NULL) {
    gl_sort(GMDM,(CmpFunc)CmpProcs);
  }
  return;
}

symchar *ProcNameF(CONST struct InitProcedure *p)
{
  assert(p!=NULL);
  return p->name;
}

struct StatementList *ProcStatementListF(CONST struct InitProcedure *p)
{
  assert(p!=NULL);
  return p->slist;
}

long GetProcParseIdF(CONST struct InitProcedure *p)
{
  assert(p!=NULL);
  return p->parse_id;
}

void SetProcParseIdF(struct InitProcedure *p, long id)
{
  assert(p!=NULL);
  p->parse_id = id;
}

int CmpProcs(CONST struct InitProcedure *p1, CONST struct InitProcedure *p2)
{
  assert(p1 && p2);
  return CmpSymchar(p1->name,p2->name);
}


int CompareProcedureLists(struct gl_list_t *pl1, struct gl_list_t *pl2,
                          unsigned long *n) 
{
  CONST struct InitProcedure *proc1, *proc2;
  CONST struct StatementList *sl1, *sl2;  
  unsigned long c,len, len1, len2, diff;

  assert (n != NULL);

  /*  compare ptrs */
  if (pl1==pl2) {
    return 0;
  }

  /* check for nulls */
  if ( (pl1==NULL) || (pl2==NULL) ) {
    *n = 1;
    return 1;
  }

  /* length  of each procedure list */
  len1 = gl_length(pl1);
  len2 = gl_length(pl2);

  /* 
   * if len1 and len2 are different from each other , we will return 1, 
   * but we still need to find the index of the first different procedure. 
   * So, get the number of elements of the shorthest list and compare 
   * that number of procedures in the lists
   */
  if (len1 == len2) {
    len = len1;
  } else {
    if (len1 < len2) {
      len = len1;
    } else {
      len = len2;
    }
  }

  /* analyze the list of procedures */
  for(c=1;c<=len;c++){
    proc1 = (struct InitProcedure *)gl_fetch(pl1,c);
    proc2 = (struct InitProcedure *)gl_fetch(pl2,c);  

    /* 
     * compare names 
     */ 
    if (CmpProcs(proc1,proc2)) {
      *n = c;
      return 1;
    }
    /* 
     * compare statement lists
     * if lists are different, diff will contain the index of the first
     * different statement. Use it if necessary
     */
    sl1 = ProcStatementList(proc1);
    sl2 = ProcStatementList(proc2); 
    if (CompareStatementLists(sl1,sl2,&diff)) {
      *n = c;
      return 1;
    }
  }

  /*
   * If we get here and the number of elements in the list are equal,
   * then the procedures are equal,
   * else the index of the first different procedure is len + 1
   */
  if (len1 == len2) {
    return 0;
  } else {
    *n = len+1;
    return 1;
  }

  /* parse_id  are not for comparison */
}
