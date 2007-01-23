/*
 *  When Output Routines
 *  by Vicente Rico-Ramirez
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: when_io.c,v $
 *  Date last modified: $Date: 1997/07/29 15:52:57 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997  Carnegie Mellon University
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
#include <general/list.h>
#include <general/dstring.h>

#include "symtab.h"


#include "functype.h"
#include "expr_types.h"
#include "instance_enum.h"
#include "instance_types.h"
#include "mathinst.h"
#include "case.h"
#include "when_util.h"
#include "instance_io.h"
#include "setio.h"
#include "exprio.h"
#include "when_io.h"


static
void WriteCase(FILE *f, struct Case *cur_case, CONST struct Instance *ref)
{
  struct Set *values;
  struct gl_list_t *reflist;
  struct Instance *inst;
  unsigned long len,c;

  values = GetCaseValues(cur_case);
  FPRINTF(f,"  ");
  if (values!=NULL){
    FPRINTF(f,"CASE ");
    WriteSet(f,values);
  }
  else FPRINTF(f,"OTHERWISE");
  FPRINTF(f," :\n");

  reflist = GetCaseReferences(cur_case);
  len = gl_length(reflist);
  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(reflist,c);
    FPRINTF(f,"      ");
    WriteInstanceName(f,inst,ref);
    FPRINTF(f,"; \n");
  }
}


void WriteWhen(FILE *f, CONST struct Instance *wheninst,
	       CONST struct Instance *ref)
{
  struct gl_list_t *whens;
  struct gl_list_t *vars;
  struct gl_list_t *cases;
  struct Case *cur_case;
  struct Instance *inst;
  unsigned long len,c;

  whens = GetInstanceWhens(wheninst);
  vars = GetInstanceWhenVars(wheninst);
  cases = GetInstanceWhenCases(wheninst);

  FPRINTF(f,"NAME :");
  WriteInstanceName(f,wheninst,ref);
  FPRINTF(f,"\n");
  FPRINTF(f,"\n");
  FPRINTF(f,"WHEN  ");
  if (vars!=NULL) {
    len = gl_length(vars);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(vars,c);
      WriteInstanceName(f,inst,ref);
      if (c<len) {
        FPRINTF(f,",  ");
      }
      if (c==len) {
        FPRINTF(f," \n");
      }
    }
  }
  if (cases!=NULL) {
    len = gl_length(cases);
    for (c=1;c<=len;c++) {
      cur_case = (struct Case *)gl_fetch(cases,c);
      WriteCase(f,cur_case,ref);
    }
    FPRINTF(f,"END ");
    FPRINTF(f,"\n");
  }
  FPRINTF(f,"\n");
  if (whens!=NULL) {
    len = gl_length(whens);
    FPRINTF(f,"USED IN CASES OF: ");
    FPRINTF(f," \n");
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(whens,c);
      FPRINTF(f,"      ");
      WriteInstanceName(f,inst,ref);
      FPRINTF(f," \n");
    }
  }
}


static
void WriteCaseDS(Asc_DString *dsPtr, struct Case *cur_case,
		 CONST struct Instance *ref)
{
  struct Set *values;
  struct gl_list_t *reflist;
  struct Instance *inst;
  unsigned long len,c;

  values = GetCaseValues(cur_case);
  Asc_DStringAppend(dsPtr,"  ",2);
  if (values!=NULL){
    Asc_DStringAppend(dsPtr,"CASE ",5);
    WriteSet2Str(dsPtr,values);
  }
  else Asc_DStringAppend(dsPtr,"OTHERWISE",9);
  Asc_DStringAppend(dsPtr," : ",2);
  Asc_DStringAppend(dsPtr,"\n",-1);

  reflist = GetCaseReferences(cur_case);
  len = gl_length(reflist);
  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(reflist,c);
    Asc_DStringAppend(dsPtr,"      ",6);
    WriteInstanceNameDS(dsPtr,inst,ref);
    Asc_DStringAppend(dsPtr,";",1);
    Asc_DStringAppend(dsPtr,"\n",-1);
  }
}

static
void WriteWhenDS(Asc_DString *dsPtr, CONST struct Instance *wheninst,
	         CONST struct Instance *ref)
{
  struct gl_list_t *whens;
  struct gl_list_t *vars;
  struct gl_list_t *cases;
  struct Case *cur_case;
  struct Instance *inst;
  unsigned long len,c;

  whens = GetInstanceWhens(wheninst);
  vars = GetInstanceWhenVars(wheninst);
  cases = GetInstanceWhenCases(wheninst);

  Asc_DStringAppend(dsPtr,"NAME :",6);
  WriteInstanceNameDS(dsPtr,wheninst,ref);
  Asc_DStringAppend(dsPtr,"\n",-1);
  Asc_DStringAppend(dsPtr,"\n",-1);

  Asc_DStringAppend(dsPtr,"WHEN ",5);
  if (vars!=NULL) {
    len = gl_length(vars);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(vars,c);
      WriteInstanceNameDS(dsPtr,inst,ref);
      if (c<len) {
	Asc_DStringAppend(dsPtr,",  ",3);
      }
      if (c==len) {
	Asc_DStringAppend(dsPtr," ",1);
        Asc_DStringAppend(dsPtr,"\n",-1);
      }
    }
  }
  if (cases!=NULL) {
    len = gl_length(cases);
    for (c=1;c<=len;c++) {
      cur_case = (struct Case *)gl_fetch(cases,c);
      WriteCaseDS(dsPtr,cur_case,ref);
    }
    Asc_DStringAppend(dsPtr,"END ",4);
    Asc_DStringAppend(dsPtr,"\n",-1);
  }
  Asc_DStringAppend(dsPtr,"\n",-1);
  if (whens!=NULL) {
    len = gl_length(whens);
    Asc_DStringAppend(dsPtr,"USED IN CASES OF: ",18);
    Asc_DStringAppend(dsPtr,"\n",-1);
    for (c=1;c<=len;c++) {
      inst = (struct Instance *)gl_fetch(whens,c);
      Asc_DStringAppend(dsPtr,"      ",6);
      WriteInstanceNameDS(dsPtr,inst,ref);
       Asc_DStringAppend(dsPtr," ",1);
       Asc_DStringAppend(dsPtr,"\n",-1);
    }
  }
}


char *WriteWhenString(CONST struct Instance *wheninst,
                      CONST struct Instance *ref)
{
  static Asc_DString ds;
  Asc_DString *dsPtr;
  char *result;

  result = ASC_NEW_ARRAY(char,15);
  if (result == NULL) {
    FPRINTF(stderr,"Memory error in WriteWhenString\n");
    return result;
  }

  dsPtr = &ds;
  Asc_DStringInit(dsPtr);
  WriteWhenDS(dsPtr,wheninst,ref);
  result = Asc_DStringResult(dsPtr);
  Asc_DStringFree(dsPtr);
  return result;

}






