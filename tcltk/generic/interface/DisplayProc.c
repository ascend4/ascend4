/*
 *  DisplayProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.19 $
 *  Version control file: $RCSfile: DisplayProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:05 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>

#include <compiler/symtab.h>
#include <compiler/module.h>
#include <compiler/library.h>


#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/type_descio.h>
#include <compiler/expr_types.h>
#include <compiler/stattypes.h>
#include <compiler/statio.h>
#include <solver/slv_types.h>
#include "HelpProc.h"
#include "DisplayProc.h"
#include "Commands.h"

#ifndef lint
static CONST char DisplayProcID[] = "$Id: DisplayProc.c,v 1.19 2003/08/23 18:43:05 ballan Exp $";
#endif


#define MAXIMUM_ID_LENGTH 80
#define MAXIMUM_STR_LENGTH 256
#define DISPTAB 4

int Asc_DispDefineCmd(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  /* The format of this command is : ddefine ?arg?, where arg may
   * be none or one. We might add a module arg.
   */
  struct TypeDescription *desc;
  unsigned long length,c;
  struct gl_list_t *list;
  FILE *ddef_outfile=NULL;
  int closefile=0;

  UNUSED_PARAMETER(cdata);

  if ( argc > 3 ) {
    Tcl_SetResult(interp, "ddefine [type [filename]]", TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc > 1 ) {/* we will print the types code - Toms argc + 1*/
    desc = FindType(AddSymbol(argv[1]));
    if (desc==NULL) {
      FPRINTF(stderr,"Internal Error : the type %s does not exist\n",
              argv[1]);
      Tcl_SetResult(interp, "Type doesn't exist", TCL_STATIC);
      return TCL_ERROR;
    } else {
      if ( argc == 3 ) {
        ddef_outfile=fopen(argv[2],"w");
        if (!ddef_outfile) {
          Tcl_SetResult(interp, "ddefine: unable to open data file.",
                        TCL_STATIC);
          return TCL_ERROR;
        }
        closefile=1;
      } else {
        ddef_outfile=stderr;
      }
      WriteDefinition(ddef_outfile,desc); /* later store this in a list */
      if (closefile) {
        fclose(ddef_outfile);
      }
      return TCL_OK;
    }
  } else {
    list = DefinitionList();
    if (list) {
      length = gl_length(list);
      for(c=1;c<=length;c++) {
        desc = (struct TypeDescription *)gl_fetch(list,c);
        PRINTF("\t%s\n",SCP(GetName(desc)));
      }
      return TCL_OK;
    } else {
      Tcl_SetResult(interp, "Strange Display Error", TCL_STATIC);
      return TCL_ERROR;
    }
  }
  /* not reached */
}
int Asc_DispDiffDefineCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
/* The format of this command is : ddiffdefine arg ?file? where arg
   is a type and file is output destination.
*/
  struct TypeDescription *desc;
  FILE *ddef_outfile=NULL;
  int closefile=0;

  UNUSED_PARAMETER(cdata);

  if (argc > 3 || argc <2) {
    Tcl_SetResult(interp, "ddiffdefine type [filename]", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = FindType(AddSymbol(argv[1]));
  if (desc==NULL) {
    FPRINTF(stderr,"ddiffdefine: the type %s does not exist\n", argv[1]);
    Tcl_SetResult(interp, "Type doesn't exist", TCL_STATIC);
    return TCL_ERROR;
  } else {
    if ( argc == 3 ) {
        ddef_outfile=fopen(argv[2],"w");
        if (!ddef_outfile) {
          Tcl_SetResult(interp, "ddiffdefine: unable to open data file.",
                        TCL_STATIC);
          return TCL_ERROR;
        }
        closefile=1;

    } else {
      ddef_outfile=stderr;
    }
    WriteDiffDefinition(ddef_outfile,desc);
    if (closefile) {
      fclose(ddef_outfile);
    }
    return TCL_OK;
  }
}

int Asc_DispTypePartsCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct gl_list_t *names;
  unsigned long len,c;
  int atoms=FALSE,models=FALSE;
  symchar *name,*oldname;
  struct TypeDescription *t;

  UNUSED_PARAMETER(cdata);

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "wrong args: dgetparts <ATOM,MODEL,BOTH> <type>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  switch (argv[1][0]) {
  case 'A':
    atoms=TRUE;
    break;
  case 'M':
    models=TRUE;
    break;
  case 'B':
    atoms=models=TRUE;
    break;
  default:
    Tcl_SetResult(interp, "bad filter: dgetparts <ATOM,MODEL,BOTH> <type>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  t = FindType(AddSymbol(argv[2]));
  if (t==NULL) {
    Tcl_SetResult(interp, "dgetparts called with nonexistent type",TCL_STATIC);
    return TCL_ERROR;
  }
  names = GetTypeNamesFromStatList(GetStatementList(t));
  len = gl_length(names);
  oldname = NULL;
  for (c=1;c<=len;c++) {
    name=(symchar *)gl_fetch(names,c);
    if (name == NULL) {
      continue; /* ignore null */
    }
    if (name != oldname) { /*do this if not same symbol as last*/
      t = FindType(name);
      if (t) { /* check ATOM/MODEL and append accordingly */
        switch (GetBaseType(t)) {
        case model_type:
          if (models) {
            Tcl_AppendElement(interp,(char *)SCP(name));
          }
          break;
        case real_type:
        case boolean_type:
        case integer_type:
        case symbol_type:
        case real_constant_type:
        case boolean_constant_type:
        case integer_constant_type:
        case symbol_constant_type:
          if (atoms) {
            Tcl_AppendElement(interp,(char *)SCP(name));
          }
          break;
        case set_type:
        case relation_type:
        case array_type:
          break;
        default:
          break;
        }
      } else {
        FPRINTF(stderr,"Type %s refers to missing type %s!\n",
            argv[2],SCP(name));
      }
    }
    oldname = name;
  }
  gl_destroy(names);
  return TCL_OK;
}

int Asc_DispQueryCmd(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
/* The format of this command is : disp arg ?arg?.
*/
  struct TypeDescription *desc;
  unsigned long len,c;
  struct gl_list_t *list;

  UNUSED_PARAMETER(cdata);

  if ( argc > 3 ) {
    Tcl_SetResult(interp, "wrong # args to \"disp\" : try define", TCL_STATIC);
    return TCL_ERROR;
  }
  if (( argc == 2 ) && (strncmp(argv[1],"define",3)==0)) {
    list = DefinitionList();
    if(list==NULL) {
      Tcl_ResetResult(interp);
      return TCL_OK;
    }
    len = gl_length(list);
    if (len==0) {
      Tcl_ResetResult(interp);
      return TCL_OK;
    }
    for(c=1;c<=len;c++) {
      desc = (struct TypeDescription *)gl_fetch(list,c);
      if (desc!=NULL) {
        Tcl_AppendElement(interp,(char *)SCP(GetName(desc)));
      } else {
        Tcl_ResetResult(interp);
        return TCL_OK;
      }
    }
    return TCL_OK;
  } else {
    FPRINTF(stderr,"Not yet supported\n");
    return TCL_OK;
  }
}


int Asc_DispHierarchyCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc, *refines=NULL;
  unsigned long c=0;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"hierarchy type\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  desc = FindType(AddSymbol(argv[1]));
  if (desc==NULL) {
    return TCL_OK;
  }
  do {
    refines = GetRefinement(desc);
    if (refines!=NULL) {
      Tcl_AppendElement(interp,(char *)SCP(GetName(refines)));
      desc = refines;
      c++;
    }
  } while (refines!=NULL);
  if(c==0) {
    Tcl_ResetResult(interp);
  }
  return TCL_OK;
}


/*  This function accepts the name of a type and returns the filename that
 *  it was found in.
 */
int Asc_DispFileByTypeCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc;
  char *filename;
  symchar *tablename;
  struct module_t *mod;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"file_by_type type\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  tablename = AddSymbol(argv[1]);
  /* Fundamental types are not defined externally -- hence no file */
  if (CheckFundamental(tablename)) {
    return TCL_OK;
  }
  desc = FindType(tablename);
  if (desc==NULL) {
    return TCL_OK;
  }
  mod = GetModule(desc);
  filename = (char *)Asc_ModuleFileName(mod);          /* cast for the CONST */
  Tcl_AppendResult(interp, filename, (char *)NULL);
  return TCL_OK;
}

int Asc_DispChildOneCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  CONST struct TypeDescription *desc;
  ChildListPtr children;
  char buf[MAXIMUM_NUMERIC_LENGTH];            /* string to hold long */
  unsigned long nch, c=0;

  UNUSED_PARAMETER(cdata);

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "wrong # args to \"dchild name num\" ", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = FindType(AddSymbol(argv[1]));
  if (desc==NULL) {
    Tcl_ResetResult(interp);
    return TCL_OK;
  }
  children = GetChildList(desc);
  if (!children) {
    Tcl_ResetResult(interp);
    return TCL_OK;
  }
  nch = ChildListLen(children);
  if(!nch) {
    Tcl_ResetResult(interp);
    return TCL_OK;
  }
  c = atol(argv[2]); /* bug. fixme use strtod */
  if((strcmp(argv[2],"0")==0) || !(c)) {
    sprintf(buf,"%lu",nch);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return TCL_OK;
  }
  if(c>nch) {
    Tcl_AppendElement(interp,(char *)SCP(ChildStrPtr(children,nch)));
  } else {
    Tcl_AppendElement(interp,(char *)SCP(ChildStrPtr(children,c)));
  }
  return TCL_OK;
}

int Asc_DispRefinesMeCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
/* This function will search the entire library hash table for all types
   that refine it. Hence if a refines b, and c refines b, calling this
   function with b should return a and c. This should be expensive.
   registered as \"drefines_me type\".
*/
  struct gl_list_t *refine_me=NULL;
  symchar *refname=NULL;
  unsigned long len,c;

  if (argc!=2 && cdata) {
    Tcl_SetResult(interp, "wrong # args to \"drefines_meall type\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args to \"drefines_me type\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (cdata) {
    refine_me = AllTypesThatRefineMe_Flat(AddSymbol(argv[1]));
  } else {
    refine_me =TypesThatRefineMe(AddSymbol(argv[1]));
  }
  if (!refine_me) {
    Tcl_ResetResult(interp);
    return TCL_OK;
  }
  len = gl_length(refine_me);
  if (!len) {
    Tcl_ResetResult(interp);
    gl_destroy(refine_me);
    return TCL_OK;
  }
  for (c=1;c<=len;c++) {
    refname = (symchar *)gl_fetch(refine_me,c);
    if (refname) {
      Tcl_AppendElement(interp,(char *)SCP(refname));
    }
  }
  gl_destroy(refine_me);
  return TCL_OK;
}

static Tcl_Interp *writehierinterp;

static void DispWriteHierTreeChildless(struct HierarchyNode *h) {
  if (!h) {
    return;
  }
  if (!(h->descendents)) {
    return;
  }
  if (gl_length(h->descendents)!=0L) {
    return;
  }
  Tcl_AppendResult(writehierinterp,"{",(char *)SCP(GetName(h->desc)),
                   " {",(char *)NULL);
  Tcl_AppendResult(writehierinterp,"}} ",(char *)NULL);
}

static void DispWriteHierTreeParents(struct HierarchyNode *h) {
  if (!h) {
    return;
  }
  if (!(h->descendents)) {
    return;
  }
  if (gl_length(h->descendents)==0L) {
    return;
  }
  Tcl_AppendResult(writehierinterp,"{",(char *)SCP(GetName(h->desc)),
                   " {",(char *)NULL);
  gl_iterate(h->descendents,(void (*)(VOIDPTR))DispWriteHierTreeParents);
  gl_iterate(h->descendents,(void (*)(VOIDPTR))DispWriteHierTreeChildless);
  Tcl_AppendResult(writehierinterp,"}} ",(char *)NULL);
}

static void DispWriteHierTree(struct HierarchyNode *h) {
  if (!h) {
    return;
  }
  if (!(h->descendents) || gl_length(h->descendents)==0L) {
    DispWriteHierTreeChildless(h);
  } else {
    DispWriteHierTreeParents(h);
  }
}


int Asc_DispRefinesMeTreeCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct HierarchyNode *h=NULL;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "call is: drefinement_tree <type>", TCL_STATIC);
    return TCL_ERROR;
  }
  h = AllTypesThatRefineMe_Tree(AddSymbol(argv[1]));
  if (!h) {
    Tcl_ResetResult(interp);
    return TCL_OK;
  }
  writehierinterp=interp;
  DispWriteHierTree(h);
  DestroyHierarchyNode(h);
  return TCL_OK;
}

/* still has a slight bug -- */

int Asc_DispIsRootTypeCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
/* Returns true if is type is a base type, or if the type is of type
   model with no refinements.
*/
  unsigned int fundamental;
  CONST struct TypeDescription *desc;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args to \"disroot_type\"", TCL_STATIC);
    return TCL_ERROR;
  }
  fundamental = CheckFundamental(AddSymbol(argv[1]));
  if (fundamental) {
    Tcl_SetResult(interp, "1", TCL_STATIC);
    return TCL_OK;
  } else {
    desc = FindType(AddSymbol(argv[1]));
    if (desc) {
      desc=GetRefinement(desc);
      if (!desc) {
        Tcl_SetResult(interp, "1", TCL_STATIC);
        return TCL_OK;
      }
    } else { /*  cant find it, so it cannot be fundamental */
      Tcl_SetResult(interp, "0", TCL_STATIC);
      return TCL_ERROR;
    }
  }
  Tcl_SetResult(interp, "0", TCL_STATIC);
  return TCL_OK;
}

