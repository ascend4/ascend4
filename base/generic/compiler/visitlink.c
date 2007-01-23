/*
 *  Ascend Name Tree Visit Implementation
 *  by Benjamin Andrew Allan
 *  09/19/97
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: visitlink.c,v $
 *  Date last modified: $Date: 1997/12/20 17:51:57 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
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

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include "symtab.h"


#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "type_desc.h"
#include "instance_enum.h"
#include "instance_name.h"
#include "instquery.h"
#include "instance_io.h"
#include "instmacro.h"
#include "parentchild.h"
#include "when_util.h"
#include "instance_types.h"
#include "visitlink.h"

#ifndef lint
static CONST char NameVisitModuleID[] = "$Id: visitlink.c,v 1.3 1997/12/20 17:51:57 ballan Exp $";
#endif

static
void SlowVisitTreeTwo(struct Instance *inst,
                      VisitNameTwoProc proc,
                      int depth, int leaf, int anon_flags, 
                      struct gl_list_t *path,
                      VOIDPTR userdata)
{
  unsigned long nc,c;
  unsigned nullchildren=0;
  struct Instance *child;
  AssertMemory(inst);
  if (!depth) {
    (*proc)(inst,path,userdata);
  }
  if (leaf || NotAtom(inst) ) {
    nc = NumberChildren(inst);
    for(c = 1; c <= nc; c++) {
      child = InstanceChild(inst,c);
      if (child != NULL) {
        if ( anon_flags || !InstanceUniversal(child) ) {
          gl_append_ptr(path,(VOIDPTR)c);
          gl_append_ptr(path,(VOIDPTR)inst);
          SlowVisitTreeTwo(child,proc,depth,leaf,anon_flags,path,userdata);
          gl_delete(path,gl_length(path),0);
          gl_delete(path,gl_length(path),0);
        }
      } else {
        nullchildren++;
      }
    }
    if (nullchildren) {
      FPRINTF(ASCERR,"Found %u NULL children of '",nullchildren);
      WriteInstanceName(ASCERR,inst,NULL);
      FPRINTF(ASCERR,"'.\n");
    }
  }
  if (depth) {
    (*proc)(inst,path,userdata);
  }
}

static
void SilentVisitTreeTwo(struct Instance *inst,
                        VisitNameTwoProc proc,
                        int depth, int leaf, int anon_flags, 
                        struct gl_list_t *path,
                        VOIDPTR userdata)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (!depth) {
    (*proc)(inst,path,userdata);
  }
  if (leaf || NotAtom(inst) ) {
    nc = NumberChildren(inst);
    for(c = 1; c <= nc; c++) {
      child = InstanceChild(inst,c);
      if (child != NULL && (GetAnonFlags(child) & anon_flags) == 0) {
        gl_append_ptr(path,(VOIDPTR)c);
        gl_append_ptr(path,(VOIDPTR)inst);
        SilentVisitTreeTwo(child,proc,depth,leaf,anon_flags,path,userdata);
        gl_delete(path,gl_length(path),0);
        gl_delete(path,gl_length(path),0);
      }
    }
  }
  if (depth) {
    (*proc)(inst,path,userdata);
  }
}


#define IVIT_MIN_LEN 20
void SlowVisitNameTreeTwo(struct Instance *inst,
                          VisitNameTwoProc proc,
                          int depth, int leaf, int anon_flags,
                          VOIDPTR userdata)
{
  struct gl_list_t *path;
  AssertMemory(inst);
  if (inst!=NULL) {
    path = gl_create(IVIT_MIN_LEN*2);
    if (path==NULL) {
      ASC_PANIC("insufficient memory");
    }
    SlowVisitTreeTwo(inst,proc,depth,leaf,anon_flags,path,userdata);
    gl_destroy(path);
  } else {
    FPRINTF(ASCERR,"SlowVisitNameTreeTwo called with NULL.");
  }
}
void SilentVisitNameTreeTwo(struct Instance *inst,
                            VisitNameTwoProc proc,
                            int depth, int leaf, int anon_flags,
                            VOIDPTR userdata)
{
  struct gl_list_t *path;
  AssertMemory(inst);
  path = gl_create(IVIT_MIN_LEN*2);
  if (path==NULL) {
    ASC_PANIC("insufficient memory");
  }
  SilentVisitTreeTwo(inst,proc,depth,leaf,anon_flags,path,userdata);
  gl_destroy(path);
}
#undef IVIT_MIN_LEN
