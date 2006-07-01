/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly, Kirk Andre Abbott

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//*
	by Kirk Andre Abbott
	Created: Jun 1, 1995.
	Last in CVS: $Revision: 1.9 $ $Date: 1998/02/24 21:44:42 $ $Author: ballan $
*/

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/error.h>
#include "compiler.h"
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "extinst.h"
#include "mathinst.h"
#include "extcall.h"

struct ExtCallNode *CreateExtCall(struct ExternalFunc *efunc,
		struct gl_list_t *args,
		struct Instance *subject,
		struct Instance *data
){
  struct ExtCallNode *ext;
  struct Instance **hndl=NULL;
  unsigned long pos;
  int added=0;

  CONSOLE_DEBUG("...");
  int i,n;
  struct Instance *inst;
  /* char *tmp; */
  n = gl_length(args);
  for(i = 1; i < n; ++i){
    inst = (struct Instance *)gl_fetch(args,i);
	/* tmp = WriteInstanceNameString(inst, NULL); */
	CONSOLE_DEBUG("argument[%d] at %p", i, inst);
	/* ASC_FREE(tmp) */

  }

  ext = ASC_NEW(struct ExtCallNode);
  CONSOLE_DEBUG("ASSIGNING efunc = %p TO ExtCallNode %p",efunc,ext);
  ext->efunc = efunc;
  ext->arglist = args;
  if(data){
    hndl = AddVarToTable(data,&added);	/** FIX FIX FIX **/
  }
  ext->data = hndl;

  if(subject){
	CONSOLE_DEBUG("subject at %p",subject);
    pos = GetSubjectIndex(args,subject);
    ext->subject = pos;
	CONSOLE_DEBUG("subject index is %d",pos);
  }else{
    ext->subject = 0L;
  }

  ext->nodestamp = -1;
  return ext;
}

void DestroyExtCall(struct ExtCallNode *ext, struct Instance *relinst){
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

struct Instance *GetSubjectInstance(struct gl_list_t *arglist,
		unsigned long varndx
){
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
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"Something really wrong (%s)",__FUNCTION__);
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

unsigned long GetSubjectIndex(struct gl_list_t *arglist, struct Instance *subject){
  unsigned long len1,c1,len2,c2;
  struct gl_list_t *branch;
  struct Instance *arg;
  unsigned long count=0L;

  if(arglist&&subject){
    len1 = gl_length(arglist);
    for(c1=1;c1<=len1;c1++){
      branch = (struct gl_list_t *)gl_fetch(arglist,c1);
      if(branch==NULL){
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Found null branch");
        return 0L; /* error */
      }
      len2 = gl_length(branch);
      for(c2=1;c2<=len2;c2++){
        count++;
        arg = (struct Instance *)gl_fetch(branch,c2);
        if (arg==subject){
          CONSOLE_DEBUG("Subject index %lu",count);
          return count;
        }
      }
    }
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Reached impossible place");
    return 0L;			/*NOTREACHED*/
  }
  CONSOLE_DEBUG("No subject index found");
  return 0L;
}

unsigned long CountNumberOfArgs(struct gl_list_t *arglist,
		unsigned long start, unsigned long end
){
  unsigned long c,count=0L;
  struct gl_list_t *branch;

  if(arglist){
    assert(start<=end);
    for (c=start;c<=end;c++){
      branch = (struct gl_list_t *)gl_fetch(arglist,c);
      if(branch==NULL){
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Found null branch");
        return 0L; /*error*/
      }
      count += gl_length(branch);
    }
    return count;
  }
  return 0L;
}

struct gl_list_t *LinearizeArgList(struct gl_list_t *arglist,
		unsigned long start, unsigned long end
){
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

struct gl_list_t *CopySpecialList(struct gl_list_t *list){
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

struct ExternalFunc *ExternalCallExtFuncF(struct ExtCallNode *ext){
  return ext->efunc;
}

struct gl_list_t *ExternalCallArgListF(struct ExtCallNode *ext){
  return ext->arglist;
}

struct Instance *ExternalCallDataInstance(struct ExtCallNode *ext){
  struct Instance **hndl;
  hndl = ext->data;
  if(hndl!=NULL){
	/* 
		ExtCallNode::data is an array of pointers to Instance
		structures. We are here returning the first pointer from that array.
	*/
    return *hndl;
  }else{
    return NULL;
  }
}

int ExternalCallNodeStampF(struct ExtCallNode *ext){
  return ext->nodestamp;
}

void SetExternalCallNodeStamp(struct ExtCallNode *ext, int nodestamp){
  if (ext->nodestamp==-1) {
    ext->nodestamp = nodestamp;
  }
}

unsigned long ExternalCallVarIndexF(struct ExtCallNode *ext){
  return ext->subject;
}

struct Instance *ExternalCallVarInstance(struct ExtCallNode *ext){
  struct Instance *i;
  assert(ext->subject);
  i = GetSubjectInstance(ext->arglist,ext->subject);
  assert(i!=NULL);
  return i;
}

