/*
 *  External Call Module
 *  by Kirk Andre Abbott
 *  Created: Jun 1, 1995.
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: extcall.c,v $
 *  Date last modified: $Date: 1998/02/24 21:44:42 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott
 *  Copyright (C) 1995  Kirk Andre' Abbott
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

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "compiler/compiler.h"
#include "general/list.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/extinst.h"
#include "compiler/mathinst.h"
#include "compiler/extcall.h"


struct Instance *GetSubjectInstance(struct gl_list_t *arglist,
				     unsigned long varndx)
{
  struct Instance *arg;
  struct gl_list_t *branch;
  unsigned long len1,c=1L,len2,count=0L;
  long safetycheck;

  if (arglist&&varndx) {
    len1 = gl_length(arglist);
    while(c<=len1){
      branch = (struct gl_list_t *)gl_fetch(arglist,c);
      if (!branch) return NULL;
      len2 = gl_length(branch);
      count += len2;
      if (count>=varndx){
	safetycheck = len2-count+varndx;
	if (safetycheck<=0){
	  FPRINTF(ASCERR,"Something really wrong in GetSubjectInstance\n");
	  FPRINTF(ASCERR,"Please report to%s\n",ASC_BIG_BUGMAIL);
	  return NULL;
	}
	arg = (struct Instance *)gl_fetch(branch,(unsigned long)safetycheck);
	return arg;
      }
      c++;
    }
  }
  return NULL;
}

unsigned long GetSubjectIndex(struct gl_list_t *arglist,
			       struct Instance *subject)
{
  unsigned long len1,c1,len2,c2;
  struct gl_list_t *branch;
  struct Instance *arg;
  unsigned long count=0L;

  if (arglist&&subject){
    len1 = gl_length(arglist);
    for(c1=1;c1<=len1;c1++){
      branch = (struct gl_list_t *)gl_fetch(arglist,c1);
      if (!branch) return 0L;	/* error */
      len2 = gl_length(branch);
      for(c2=1;c2<=len2;c2++){
	count++;
	arg = (struct Instance *)gl_fetch(branch,c2);
	if (arg==subject)
	  return count;
      }
    }
    return 0L;			/*NOTREACHED*/
  }
  return 0L;
}

unsigned long CountNumberOfArgs(struct gl_list_t *arglist,
				unsigned long start, unsigned long end)
{
  unsigned long c,count=0L;
  struct gl_list_t *branch;

  if (arglist) {
    assert(start<=end);
    for (c=start;c<=end;c++){
      branch = (struct gl_list_t *)gl_fetch(arglist,c);
      if (!branch) return 0L;	/*error*/
      count += gl_length(branch);
    }
    return count;
  }
  else
    return 0L;
}

struct gl_list_t *LinearizeArgList(struct gl_list_t *arglist,
				   unsigned long start, unsigned long end)
{
  struct gl_list_t *result,*branch;
  struct Instance *arg;
  unsigned long c1,len2,c2;

  if (arglist){
    assert(start<=end);
    result = gl_create(20L);
    for(c1=start;c1<=end;c1++){
      branch = (struct gl_list_t *)gl_fetch(arglist,c1);
      if (!branch){
	gl_destroy(result);
	return NULL;
      }
      len2 = gl_length(branch);
      for(c2=1;c2<=len2;c2++){
	arg = (struct Instance *)gl_fetch(branch,c2);
	gl_append_ptr(result,(VOIDPTR)arg);
      }
    }
    return result;
  }
  return NULL;
}

void DestroySpecialList(struct gl_list_t *list)
{
  unsigned long len,c;
  struct gl_list_t *branch,*tmp;
  if (list) {
    tmp = list;
    len = gl_length(tmp);
    for (c=1;c<=len;c++) {
      branch = (struct gl_list_t *)gl_fetch(tmp,c);
      if (branch != NULL) {
        gl_destroy(branch);
      }
    }
    gl_destroy(tmp);
    list = NULL;
  }
}

struct gl_list_t *CopySpecialList(struct gl_list_t *list)
{
  unsigned long len1,c1,len2,c2;
  struct gl_list_t *result,*branch,*tmp;
  struct Instance *arg;
  if (list) {
    len1 = gl_length(list);
    result = gl_create(len1);
    for(c1=1;c1<=len1;c1++) {
      tmp = (struct gl_list_t *)gl_fetch(list,c1);
      if (tmp) {
	len2 = gl_length(tmp);
	branch = gl_create(len2);
	for (c2=1;c2<=len2;c2++) {
	  arg = (struct Instance *)gl_fetch(tmp,c2);
	  gl_append_ptr(branch,(VOIDPTR)arg);
	}
      }
      else{
	DestroySpecialList(result);
	return NULL;
      }
      gl_append_ptr(result,(VOIDPTR)branch);
    }
    return result;
  }
  return NULL;
}


struct ExtCallNode *CreateExtCall(struct ExternalFunc *efunc,
				      struct gl_list_t *args,
				      struct Instance *subject,
				      struct Instance *data)
{
  struct ExtCallNode *ext;
  struct Instance **hndl=NULL;
  unsigned long pos;
  int added=0;
  ext = (struct ExtCallNode *)ascmalloc(sizeof(struct ExtCallNode));
  ext->efunc = efunc;
  ext->arglist = args;
  if (data) {
    hndl = AddVarToTable(data,&added);	/** FIX FIX FIX **/
  }
  ext->data = hndl;
  if (subject) {
    pos = GetSubjectIndex(args,subject);
    ext->subject = pos;
  } else {
    ext->subject = 0L;
  }
  ext->nodestamp = -1;
  return ext;
}

void DestroyExtCall(struct ExtCallNode *ext, struct Instance *relinst)
{
  struct Instance *ptr;
  unsigned long len1, c1;
  unsigned long len2, c2;
  struct gl_list_t *arglist, *branch;

  if (!ext) return;
  arglist = ext->arglist;
  if (arglist) {
    len1 = gl_length(arglist);
    for (c1=1;c1<=len1;c1++) {
      branch = (struct gl_list_t *)gl_fetch(arglist,c1);
      len2 = gl_length(branch);
      for (c2=len2;c2>=1;c2--) {
	if ( (ptr = (struct Instance *)gl_fetch(branch,c2)) !=NULL)
	  RemoveRelation(ptr,relinst);
      }
      gl_destroy(branch);
    }
    gl_destroy(arglist);
    arglist = NULL;
  }
}

struct ExternalFunc *ExternalCallExtFuncF(struct ExtCallNode *ext)
{
  return ext->efunc;
}

struct gl_list_t *ExternalCallArgListF(struct ExtCallNode *ext)
{
  return ext->arglist;
}

struct Instance *ExternalCallDataInstance(struct ExtCallNode *ext)
{
  struct Instance **hndl;
  hndl = ext->data;
  if (hndl)
    return *hndl;
  else
    return NULL;
}

int ExternalCallNodeStampF(struct ExtCallNode *ext)
{
  return ext->nodestamp;
}

void SetExternalCallNodeStamp(struct ExtCallNode *ext,
			      int nodestamp)
{
  if (ext->nodestamp==-1) {
    ext->nodestamp = nodestamp;
  }
}

unsigned long ExternalCallVarIndexF(struct ExtCallNode *ext)
{
  return ext->subject;
}

struct Instance *ExternalCallVarInstance(struct ExtCallNode *ext)
{
  struct Instance *i;
  assert(ext->subject);
  i = GetSubjectInstance(ext->arglist,ext->subject);
  assert(i!=NULL);
  return i;
}









