/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
	Copyright (C) 2007 Carnegie Mellon University

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
*//** @file
	The following functions give an alternative to the recursive clear
	procedure found older ASCEND models.
*//*
	by Ben Allan, February 24, 1998
	Last in CVS: $Revision: 1.4 $ $Date: 1998/06/16 16:42:09 $ $Author: mthomas $
*/

#include <utilities/ascConfig.h>
#include <utilities/ascPrint.h>

#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/symtab.h>
#include <compiler/instquery.h>
#include <compiler/atomvalue.h>
#include <compiler/visitinst.h>
#include <compiler/parentchild.h>
#include <compiler/library.h>
#include <packages/ascFreeAllVars.h>

struct cvpacket {
  struct TypeDescription *g_solver_var_type;
  symchar *fixed;
};

static
void Asc_ClearVars(struct Instance *i, struct cvpacket *cv)
{
  struct Instance *c;
  struct TypeDescription *type;

  type = InstanceTypeDesc(i);
  if ( GetBaseType(type) == real_type &&
       type == MoreRefined(type,cv->g_solver_var_type) ) {
    c = ChildByChar(i,cv->fixed);
    if (c != NULL && InstanceKind(c)==BOOLEAN_INST) {
      SetBooleanAtomValue(c,FALSE,0);
    }
  }
}


int Asc_ClearVarsInTree(struct Instance *i){
  struct cvpacket cv;
  cv.g_solver_var_type = FindType(AddSymbol("solver_var"));
  if (cv.g_solver_var_type  == NULL){
	ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CV.G_SOLVER_VAR_TYPE IS NULL");
	return 1;
  }/*else{
	ERROR_REPORTER_DEBUG("solver_var was found :)\n");
  }*/

  if (i==NULL){
	ERROR_REPORTER_HERE(ASC_PROG_ERROR,"INSTANCE IS NULL");
    return 1;
  }
  cv.fixed = AddSymbol("fixed");
  VisitInstanceTreeTwo(i,(VisitTwoProc)Asc_ClearVars, 0, 0, &cv);
  return 0;
}

int Asc_FreeAllVars( struct Instance *root, struct gl_list_t *arglist, void *userdata){
  /* arglist is a list of gllist of instances */
  if (arglist == NULL ||
      gl_length(arglist) == 0L ||
      gl_length((struct gl_list_t *)gl_fetch(arglist,1)) != 1 ||
      gl_fetch((struct gl_list_t *)gl_fetch(arglist,1),1) == NULL) {
	/*ERROR_REPORTER_HERE(ASC_PROG_NOTE,"About to call ClearVarsInTree(root)");*/
    return Asc_ClearVarsInTree(root);
  } else {
	/*ERROR_REPORTER_HERE(ASC_PROG_NOTE,"About to call ClearVarsInTree(arglist[1][1])\n");*/
    return Asc_ClearVarsInTree((struct Instance *)gl_fetch(
                               (struct gl_list_t *)gl_fetch(arglist,1),1));
  }
}
