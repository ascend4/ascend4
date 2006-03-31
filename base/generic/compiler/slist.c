/*
 *  Statement list procedures
 *  by Tom Epperly
 *  Part of Ascend
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: slist.c,v $
 *  Date last modified: $Date: 1997/07/18 12:34:56 $
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
 */

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "stattypes.h"
#include "statement.h"
#include "statio.h"
#include "slist.h"
#include <general/mathmacros.h>

#define SLMALLOC \
(struct StatementList *)ascmalloc((unsigned)sizeof(struct StatementList))

#ifndef lint
static CONST char StatementListID[] = "$Id: slist.c,v 1.10 1997/07/18 12:34:56 mthomas Exp $";
#endif

struct StatementList *CreateStatementList(struct gl_list_t *l)
{
  struct StatementList *result;
  result = SLMALLOC;
  assert(l!=NULL);
  assert(result!=NULL);
  result->ref_count = 1;
  result->l = l;
  return result;
}

struct StatementList *EmptyStatementList(void)
{
  struct gl_list_t *list;
  list = gl_create(0L);
  return CreateStatementList(list);
}

struct gl_list_t *GetListF(CONST struct StatementList *sl,
                           CONST char *fname, int li)
{
  if ( sl == NULL ) {
    FPRINTF(ASCERR,"GetListF called with NULL StatementList (%s:%d)\n",
    fname,li);
    return NULL;
  }
  if ( sl->ref_count == 0 ) {
    FPRINTF(ASCERR,"GetListF called with bad StatementList (%s:%d)\n",
    fname,li);
  }
  return sl->l;
}

unsigned long StatementInList(CONST struct StatementList *sl,
                              CONST struct Statement *s)
{
  if (sl==NULL || s==NULL || sl->l ==NULL) return 0L;
  return gl_ptr_search(sl->l,s,0);
}

struct Statement *GetStatement(CONST struct StatementList *sl,
                               unsigned long int j)
{
  if (sl==NULL || sl->l == NULL || gl_length(sl->l) < j || j == 0L) {
    return NULL;
  }
  return (struct Statement *)gl_fetch(sl->l,j);
}

struct StatementList *CopyStatementList(struct StatementList *sl)
{
  assert(sl!=NULL);
  assert(sl->ref_count);
  if (sl->ref_count < MAXREFCOUNT) sl->ref_count++;
  return sl;
}

struct StatementList *CopyListToModify(struct StatementList *sl)
{
  register struct StatementList *result;
  register struct Statement *s;
  register unsigned long c,len;
  assert(sl!=NULL);
  assert(sl->ref_count);
  result=SLMALLOC;
  assert(result!=NULL);
  result->ref_count = 1;
  result->l = gl_create(len=gl_length(sl->l));
  for(c=1;c<=len;c++) {
    s = CopyToModify((struct Statement *)gl_fetch(sl->l,c));
    gl_append_ptr(result->l,(VOIDPTR)s);
  }
  return result;
}

int CompareStatementLists(CONST struct StatementList *sl1,
                          CONST struct StatementList *sl2,
                          unsigned long int *diff)
{
  unsigned long int c,len1, len2, len;
  int ctmp = 0;
  CONST struct Statement *s1;
  CONST struct Statement *s2;

  *diff = 0L;
  if (sl1 == sl2) return 0;
  len1=StatementListLength(sl1);
  len2=StatementListLength(sl2);
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
    s1 = (CONST struct Statement *)gl_fetch(GetList(sl1),c);
    s2 = (CONST struct Statement *)gl_fetch(GetList(sl2),c);
    ctmp = CompareStatements(s1,s2);
    if (ctmp != 0)  {
      break;
    }
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

int CompareISLists(CONST struct StatementList *sl1,
                   CONST struct StatementList *sl2,
                   unsigned long int *diff)
{
  unsigned long int c,len1, len2, len;
  int ctmp = 0;
  CONST struct Statement *s1;
  CONST struct Statement *s2;

  *diff = 0L;
  if (sl1 == sl2) return 0;
  len1=StatementListLength(sl1);
  len2=StatementListLength(sl2);
  /* do not return early just because len1 != len2. want to check
   * equivalency up to the length of the shorter.
   */
  len = MIN(len1,len2);
  if (len==0) {
    /* curiously  empty lists > lists with something */
    *diff = 1L; /* if 1 not empty, they differ in 1st entry */
    if (len1) return -1;
    if (len2) return 1;
    *diff = 0L; /* both empty. don't differ. */
    return 0;
  }
  for (c=1; c <= len; c++) { /* we always enter this loop */
    s1 = (CONST struct Statement *)gl_fetch(GetList(sl1),c);
    s2 = (CONST struct Statement *)gl_fetch(GetList(sl2),c);
    ctmp = CompareISStatements(s1,s2);
    if (ctmp != 0)  {
      break;
    }
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

void AppendStatement(struct StatementList *sl1, struct Statement *s)
{
  register struct gl_list_t *l1;

  assert(s!=NULL);
  if (sl1 ==NULL) {
    sl1 = EmptyStatementList();
  }
  l1 = GetList(sl1);
  CopyStatement(s);
  gl_append_ptr(l1,(VOIDPTR)s);
}

struct StatementList *AppendStatementLists(CONST struct StatementList *sl1,
                                           struct StatementList *sl2)
{
  register CONST struct gl_list_t *l1;
  register struct gl_list_t *list,*l2;
  register struct Statement *stat;
  register unsigned long c,len;
  assert(sl1 && sl2);
  l1 = GetList(sl1);
  l2 = GetList(sl2);
  len = gl_length(l1);
  list = gl_create(len+gl_length(l2));
  /* add elements from sl1 */
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(l1,c);
    CopyStatement(stat);
    gl_append_ptr(list,(VOIDPTR)stat);
  }
  /* add elements from sl2 */
  len = gl_length(l2);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(l2,c);
    CopyStatement(stat);
    gl_append_ptr(list,(VOIDPTR)stat);
  }
  DestroyStatementList(sl2);
  return CreateStatementList(list);
}


void DestroyStatementList(struct StatementList *sl)
{
  register unsigned long c,len;
  if (sl == NULL) return;
  assert(sl->ref_count!=0);
  if (sl->ref_count < MAXREFCOUNT) sl->ref_count--;
  if (sl->ref_count==0) {
    len = gl_length(sl->l);
    for(c=1;c<=len;c++)
      DestroyStatement((struct Statement *)gl_fetch(sl->l,c));
    gl_destroy(sl->l);
    ascfree((char *)sl);
  }
}
