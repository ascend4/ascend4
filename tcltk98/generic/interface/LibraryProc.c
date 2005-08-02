/*
 *  LibraryProc.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.44 $
 *  Version control file: $RCSfile: LibraryProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:06 $
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

#include <time.h>
#include "tcl.h"
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/dstring.h"
#include "general/list.h"
#include "compiler/compiler.h"
#include "compiler/symtab.h"
#include "compiler/braced.h"
#include "compiler/notate.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/types.h"
#include "compiler/syntax.h"
#include "compiler/module.h"
#include "compiler/instance_enum.h"
#include "compiler/dump.h"
#include "compiler/stattypes.h"
#include "compiler/slist.h"
#include "compiler/child.h"
#include "compiler/childio.h"
#include "compiler/type_desc.h"
#include "compiler/typedef.h"
#include "compiler/extfunc.h"
#include "compiler/library.h"
#include "compiler/prototype.h"
#include "compiler/proc.h"
#include "compiler/nameio.h"
#include "solver/slv_types.h"
#include "interface/HelpProc.h"
#include "interface/LibraryProc.h"
#include "interface/Commands.h"
#include "interface/SimsProc.h"

#ifndef lint
static CONST char LibraryProcID[] = "$Id: LibraryProc.c,v 1.44 2003/08/23 18:43:06 ballan Exp $";
#endif


extern
int Asc_FileIDCopy(FILE *filein, FILE *fileout)
{
  int c;
  while ((c = fgetc(filein)) != EOF) {
    FPUTC(c,fileout);
  }
  return 0;
}


struct int_option {
  int *option_ptr;
  char *option_name;
  int low;
  int high;
};

/* keep the names here < 60 chars. Data for Options command */
#define OPTIONCOUNT 4
static
struct int_option g_option_list[OPTIONCOUNT] = {
  {&g_compiler_warnings,"-compilerWarnings",0,INT_MAX},
  {&g_parser_warnings,"-parserWarnings",0,5}, 
  {&g_simplify_relations,"-simplifyRelations",0,1},
  {&g_use_copyanon,"-useCopyAnon",0,1}
};
#define GOL g_option_list
STDHLF(Asc_LibrOptionsCmd,(Asc_LibrOptionsCmdHL,HLFSTOP));
int Asc_LibrOptionsCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char **argv)
{
  int i, opt, status;
  char buf[80];
  ASCUSE;  /* see if first arg is -help */
  if (argc == 1) {
    for (i = 0; i < OPTIONCOUNT; i++) {
      sprintf(buf,"%s %d",GOL[i].option_name, *(GOL[i].option_ptr));
      Tcl_AppendElement(interp,buf);
    }
    return TCL_OK;
  }
  if (argc == 2) {
    for (i = 0; i < OPTIONCOUNT; i++) {
      if (strcmp(argv[1],GOL[i].option_name)==0) {
        sprintf(buf,"%d", *(GOL[i].option_ptr));
        Tcl_AppendResult(interp,buf,(char *)NULL);
        return TCL_OK;
      }
    }
    Tcl_AppendResult(interp,"Unknown option '",argv[1],"' to ",
                     Asc_LibrOptionsCmdHN,(char *)NULL);
    return TCL_ERROR;
  }
  if (argc == 3) {
    for (i = 0; i < OPTIONCOUNT; i++) {
      if (strcmp(argv[1],GOL[i].option_name)==0) {
        status = Tcl_GetInt(interp,argv[2],&opt);
        if (status != TCL_OK) {
          Tcl_AppendResult(interp,"Non-integer value (",argv[2],") given for ",
                           argv[0]," ",argv[1],(char *)NULL);
          return TCL_ERROR;
        }
        if (opt < GOL[i].low || opt > GOL[i].high) {
          sprintf(buf,"Value %d out of range [%d - %d]",opt,GOL[i].low,
                  GOL[i].high);
          Tcl_AppendResult(interp,argv[0],": ",buf," for ",argv[1],
                           (char *)NULL);
          return TCL_ERROR;
        }
        *(GOL[i].option_ptr) = opt;
        return TCL_OK;
      }
    }
    Tcl_AppendResult(interp,"Unknown option '",argv[1],"' to ",
                     argv[0],(char *)NULL);
    return TCL_ERROR;
  }
  sprintf(buf,"%d",argc);
  Tcl_AppendResult(interp,"Too many arguments (",buf,") to ",
                   Asc_LibrOptionsCmdHN, (char *)NULL);
  return TCL_ERROR;
}

STDHLF(Asc_LibrReadCmd,(Asc_LibrReadCmdHL,HLFSTOP));
int Asc_LibrReadCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char **argv)
{
  struct module_t *mod;
  int relns_flag = 1;
  int result;
  int zz_parse();

  ASCUSE; /* see if first arg is -help */

  if ( argc < 2 || argc > 3 ) {
    Tcl_SetResult(interp,
                  "wrong # args: Usage: " Asc_LibrReadCmdHU, TCL_STATIC);
    return TCL_ERROR;
  }

  /* set up the parse relns flag */
  if ( argc == 3 ) {
    relns_flag = atoi(argv[2]);
  }

  SetParseRelnsFlag(relns_flag);
  if((mod = Asc_OpenModule(argv[1],NULL)) == NULL) {
    Tcl_AppendResult(interp,
                     Asc_LibrReadCmdHN ": Error in opening file ",
                     argv[1], (char*)NULL);
    result = TCL_ERROR;
  } else {
    /*
     * the open was successful.  parse the file.
     */
    Tcl_SetResult(interp, (char*)SCP(Asc_ModuleName(mod)), TCL_VOLATILE);
    zz_parse();
    result = TCL_OK;
  }
  SetParseRelnsFlag(1);	  /* always reset */
  return result;
}

STDHLF(Asc_LibrParseCmd,(Asc_LibrParseCmdHL,HLFSTOP));
int Asc_LibrParseCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char **argv)
{
  struct module_t *mod;
  int osmerr;
  int result;
  int zz_parse();

  ASCUSE; /* see if first arg is -help */

  if ( argc != 2) {
    Tcl_SetResult(interp,
                  "wrong # args: Usage: " Asc_LibrParseCmdHU, TCL_STATIC);
    return TCL_ERROR;
  }

  mod = Asc_OpenStringModule(argv[1],&osmerr,NULL);
  if (mod == NULL) {
    Tcl_AppendResult(interp,
                     Asc_LibrParseCmdHN ": Insufficient memory to open "
                     "string buffer ",
                     argv[1], (char*)NULL);
    result = TCL_ERROR;
  } else {
    /*
     * the open was successful.  parse the string.
     */
    Tcl_SetResult(interp, (char*)SCP(Asc_ModuleName(mod)), TCL_VOLATILE);
    zz_parse();
    Asc_CloseCurrentModule();
    result = TCL_OK;
  }
  return result;
}


static
int LibrModuleList(Tcl_Interp *interp, int module_type)
{
  unsigned long c;
  struct gl_list_t *ml;

  if ( module_type < 0 || module_type > 2) {
    Tcl_SetResult(interp, "module_type given not in [0 .. 2]", TCL_STATIC);
    return TCL_ERROR;
  }
  ml = Asc_ModuleList(module_type);
  if( ml == NULL ) {
    /* module list is empty, return empty string */
    Tcl_ResetResult(interp);
    return TCL_OK;
  }

  for( c = gl_length(ml); c > 0; c-- ) {
    Tcl_AppendElement(interp, (char *)gl_fetch( ml, c ));
  }
  gl_destroy(ml);
  return TCL_OK;
}


static
int LibrModelDefinitionMethods(Tcl_Interp *interp)
{
  struct gl_list_t *pl;
  unsigned long c,len;
  
  pl = GetUniversalProcedureList();
  if (pl == NULL) {
    return TCL_OK;
  }
  len = gl_length(pl);
  for (c = 1; c <= len; c++) {
    Tcl_AppendElement(interp,
        (char *)SCP(ProcName((struct InitProcedure *)gl_fetch(pl,c))));
  }
  return TCL_OK;
}

STDHLF(Asc_LibrTypeListCmd,(Asc_LibrTypeListCmdHL,HLFSTOP));
int Asc_LibrTypeListCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char **argv)
{
  struct gl_list_t *dl;
  unsigned long len;
  unsigned long c;
  CONST struct module_t *module;

  ASCUSE;  /* see if first arg is -help */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args: Usage: " Asc_LibrTypeListCmdHU,
                  TCL_STATIC);
    return TCL_ERROR;
  }

  module = Asc_GetModuleByName(argv[1]);
  if( module == NULL ) {
    Tcl_AppendResult(interp, Asc_LibrTypeListCmdHN
                     ": Cannot find a module having the name ", argv[1],
                     NULL);
    return TCL_ERROR;
  }

  dl = Asc_TypeByModule(module);
  if ( dl == NULL ) {
    Tcl_AppendResult(interp, Asc_LibrTypeListCmdHN
                     ": The type definition list for", argv[1], "is NULL",
                     NULL);
    return TCL_ERROR;
  }

  len = gl_length(dl);
  for( c = 1; c <= len; c++ ) {
    Tcl_AppendElement(interp, (char*)gl_fetch(dl,c));
  }
  gl_destroy(dl);
  return TCL_OK;
}


STDHLF(Asc_LibrDestroyTypesCmd, (Asc_LibrDestroyTypesCmdHL,HLFSTOP));
int Asc_LibrDestroyTypesCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char **argv)
{
  ASCUSE;  /* see if first arg is -help */

  FFLUSH(stderr);
  DestroyNotesDatabase(LibraryNote());
  SetUniversalProcedureList(NULL);
  DestroyLibrary();
  DestroyPrototype();
  EmptyTrash();
  Asc_DestroyModules((DestroyFunc)DestroyStatementList);
  WriteChildMissing(NULL,NULL,NULL);
  DefineFundamentalTypes();
  InitNotesDatabase(LibraryNote());
  return TCL_OK;
}



/*
 *  void AddRootName(t);
 *      const struct TypeDescription *t;
 *
 *  AddRootName is called via gl_interate to find all root types, i.e,
 *  types that don't refine other types, and append the names of those
 *  types to the Tcl result.  We need to make a locally global pointer
 *  to the Tcl interpreter (called lroottypesinterp) so that AddRootName
 *  can access it.
 */
static Tcl_Interp *lroottypesinterp;
static void AddRootName(CONST struct TypeDescription *t)
{
  if(( t != NULL ) && ( GetRefinement(t) == NULL )) {
    Tcl_AppendElement(lroottypesinterp, (char *)SCP(GetName(t)));
  }
}

static
int LibrRootTypes(Tcl_Interp *interp)
{
  struct gl_list_t *deflist;

  deflist = DefinitionList();
  lroottypesinterp = interp;
  gl_iterate(deflist,(void (*)(VOIDPTR))AddRootName);
  gl_destroy(deflist);
  return TCL_OK;
}

static
int LibrCatalog(Tcl_Interp *interp)
{
  struct gl_list_t *deflist;
  unsigned long len;
  unsigned long c;

  deflist = DefinitionList();
  len = gl_length(deflist);
  for (c = 1; c <= len; c++) {
    Tcl_AppendElement(interp,
     (char*)SCP(GetName((CONST struct TypeDescription *)gl_fetch(deflist,c))));
  }
  gl_destroy(deflist);
  return TCL_OK;
}



int Asc_GNUTextCmd(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char **argv)
{
  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  Tcl_AppendResult(interp,
                   "See the \"License\" buffer in the Script\n",
                   "for information on the GNU License and Warranty\n",
                   (char*)NULL);
  return TCL_OK;
}


static
int LibrFindType(Tcl_Interp *interp, struct TypeDescription *desc)
{
  struct module_t *mod;
  assert(desc!=NULL);

  mod = GetModule(desc);
  if( mod == NULL ) {
    Tcl_AppendResult(interp, Asc_LibrQueryTypeCmdHN
                     ": Type ", (char *)SCP(GetName(desc)),
                     " is a fundamental type", NULL);
    return TCL_ERROR;
  }

  Tcl_SetResult(interp, (char *)SCP(Asc_ModuleName(mod)), TCL_VOLATILE);
  return TCL_OK;
}

static
int LibrAncestorType(Tcl_Interp *interp, struct TypeDescription *desc)
{
  struct gl_list_t *names;
  unsigned long c,len;

  assert(desc!=NULL);
  names = GetAncestorNames(desc);
  if( names == NULL ) {
    Tcl_AppendResult(interp, "-ancestors: out of memory", NULL);
    return TCL_ERROR;
  }
  for (c = 1, len = gl_length(names);  c <= len; c++) {
    Tcl_AppendElement(interp,(char *)gl_fetch(names,c));
  }
  gl_destroy(names);
  return TCL_OK;
}


STDHLF(Asc_LibrModuleInfoCmd,(Asc_LibrModuleInfoCmdHL,HLFSTOP));
int Asc_LibrModuleInfoCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char **argv)
{
  CONST struct module_t *mod;
  CONST char *string;
  char index[36];
  int i;

  ASCUSE;  /* see if first arg is -help */

  if( argc < 2 ) {
    Tcl_SetResult(interp, "wrong # args: Usage: " Asc_LibrModuleInfoCmdHU,
                  TCL_STATIC);
    return TCL_ERROR;
  }

  for( i = 1; i < argc; i++ ) {
    if((mod = Asc_GetModuleByName(argv[i])) != NULL ) {
      Tcl_AppendElement(interp, (char *)SCP(Asc_ModuleName(mod)));
      Tcl_AppendElement(interp, (char *)SCP(Asc_ModuleBestName(mod)));
      string = Asc_ModuleString(mod);
      if (string == NULL) {
        Tcl_AppendElement(interp, asctime(Asc_ModuleTimeModified(mod)));
        Tcl_AppendElement(interp, NULL);
      } else {
        sprintf(index,"%d",(int)Asc_ModuleStringIndex(mod));
        Tcl_AppendElement(interp, index);
        Tcl_AppendElement(interp, (char *)string);
      }
    }
  }
  return TCL_OK;
}


static
int LibrExternalFuncs(Tcl_Interp *interp)
{
  char *stringresult;
  stringresult = WriteExtFuncLibraryString();
  if (stringresult!=NULL) {
    Tcl_AppendResult(interp,stringresult,(char *)NULL);
    ascfree(stringresult);
  }
  return TCL_OK;
}



STDHLF(Asc_LibrHideTypeCmd, (Asc_LibrHideTypeCmdHL,HLFSTOP));
int Asc_LibrHideTypeCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char **argv)
{
  struct TypeDescription *type;
  ChildListPtr clist;
  unsigned long c;

  ASCUSE;  /* see if first arg is -help */

  if( argc < 2 || argc > 3 ) {
    Tcl_SetResult(interp, "wrong # args: Usage: " Asc_LibrHideTypeCmdHU,
                  TCL_STATIC);
    return TCL_ERROR;
  }

  type = FindType(AddSymbol(argv[1]));
  if (type==NULL) {
    Tcl_AppendResult(interp,
                     Asc_LibrHideTypeCmdHN " called with unknown type: ",
                     argv[1], (char *)NULL);
    return TCL_ERROR;
  }

  if ( argc == 2 ) {
    SetTypeShowBit(type,FALSE);
    return TCL_OK;
  }

  clist = GetChildList(type);
  if (clist==NULL) {
    Tcl_AppendResult(interp,
                     Asc_LibrHideTypeCmdHN " called with unknown type part",
                     (char *)NULL);
    return TCL_ERROR;
  }
  c = ChildPos(clist,AddSymbol(argv[2]));
  if( c == 0UL ) {
    Tcl_AppendResult(interp,
                     Asc_LibrHideTypeCmdHN " called with unknown type part",
                     (char *)NULL);
    return TCL_ERROR;
  }
  ChildHide(clist,c);
  return TCL_OK;
}


STDHLF(Asc_LibrUnHideTypeCmd, (Asc_LibrUnHideTypeCmdHL,HLFSTOP));
int Asc_LibrUnHideTypeCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char **argv)
{
  struct TypeDescription *type;
  ChildListPtr clist;
  unsigned long c;

  ASCUSE;  /* see if first arg is -help */

  if (argc < 2 || argc > 3) {
    Tcl_SetResult(interp, "wrong # args: Usage: " Asc_LibrUnHideTypeCmdHU,
                  TCL_STATIC);
    return TCL_ERROR;
  }

  type = FindType(AddSymbol(argv[1]));
  if (type==NULL) {
    Tcl_AppendResult(interp,
                     Asc_LibrUnHideTypeCmdHN " called with unknown type: ",
                     argv[1], (char*)NULL);
    return TCL_ERROR;
  }

  if ( argc == 2 ) {
    SetTypeShowBit(type,TRUE);
    return TCL_OK;
  }

  clist = GetChildList(type);
  if (clist==NULL) {
    Tcl_AppendResult(interp,
                     Asc_LibrUnHideTypeCmdHN " called with unknown type part",
                     (char*)NULL);
    return TCL_ERROR;
  }
  c = ChildPos(clist,AddSymbol(argv[2]));
  if (c == 0UL) {
    Tcl_AppendResult(interp,
                     Asc_LibrUnHideTypeCmdHN " called with unknown type part",
                     (char*)NULL);
    return TCL_ERROR;
  }
  ChildShow(clist,c);
  return TCL_OK;
}


static
int LibrGetFundamentals(Tcl_Interp *interp)
{
  struct gl_list_t *fundies;
  struct TypeDescription *type;
  unsigned long len,c;
  symchar *name;

  fundies = FindFundamentalTypes();
  len = gl_length(fundies);

  for (c=1;c<=len;c++) {
    type = (struct TypeDescription *)gl_fetch(fundies,c);
    name = GetName(type);
    Tcl_AppendElement(interp,(char *)SCP(name));
  }
  gl_destroy(fundies);
  return TCL_OK;
}


STDHLF(Asc_LibrTypeIsShownCmd, (Asc_LibrTypeIsShownCmdHL, HLFSTOP));
int Asc_LibrTypeIsShownCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char **argv)
{
  struct TypeDescription *type;
  char buf[MAXIMUM_NUMERIC_LENGTH];   /* string to hold integer */

  ASCUSE;  /* see if first arg is -help */

  if ( argc != 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args: Usage: " Asc_LibrTypeIsShownCmdHU,
                  TCL_STATIC);
    return TCL_ERROR;
  }
  type = FindType(AddSymbol(argv[1]) );
  if( type == NULL ) {
    Tcl_AppendResult(interp,
                     Asc_LibrTypeIsShownCmdHN " called with unknown type",
                     argv[1], (char *)NULL);
    return TCL_ERROR;
  }
  sprintf(buf,"%d",TypeShow(type));
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

static
int LibrFileExtsCmd(Tcl_Interp *interp)
{
  int i;

  for (i = 0; i <  MOD_FILE_EXTS; i++) {
    Tcl_AppendElement(interp,(char *)g_alt_ending[i]);
  }
  return TCL_OK;
}

static
int LibrTypeChildren(Tcl_Interp *interp, struct TypeDescription *desc)
{
  ChildListPtr children;
  unsigned long nch;
  unsigned long c = 0;
  assert(desc!=NULL);

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
  for(c=1;c<=nch;c++) {
    Tcl_AppendElement(interp,(char *)SCP(ChildStrPtr(children,c)));
  }
  return TCL_OK;
}


static
int LibrChildInfo(Tcl_Interp *interp, struct TypeDescription *desc,
                  symchar *child)
{
  char *s;
  ChildListPtr cl;
  unsigned long nch;
  unsigned long c = 0;

  if (desc == NULL) {
    Tcl_AppendResult(interp,WriteChildMetaDetails(),(char *)NULL);
    return TCL_OK;
  }
  cl = GetChildList(desc);
  assert(cl != NULL);
  if (child!=NULL) {
    c = ChildPos(cl,child);
    if (!c) {
      Tcl_AppendResult(interp,"child ",(char *)SCP(child)," not found",
                       (char *)NULL);
      return TCL_ERROR;
    }
    s = WriteChildDetails(cl,c);
    Tcl_AppendResult(interp,s,(char *)NULL);
    ascfree(s);
    return TCL_OK;
  } else {
    nch = ChildListLen(cl);
    for (c = 1; c <= nch; c++) {
      s = WriteChildDetails(cl,c);
      Tcl_AppendResult(interp,s,(char *)NULL);
      ascfree(s);
    }
    return TCL_OK;
  }
}

static
int LibrMethods(Tcl_Interp *interp, struct TypeDescription *desc)
{
  struct InitProcedure *ip;
  struct gl_list_t *pl;
  unsigned long len,c;

  assert(desc!=NULL);
  pl = GetInitializationList(desc);
  if (pl!=NULL) {
    len = gl_length(pl);
    for(c=1;c<=len;c++) {
      ip = (struct InitProcedure *)gl_fetch(pl,c);
      Tcl_AppendElement(interp,(char *)SCP(ProcName(ip)));
    }
  }
  return TCL_OK;
}

static
int LibrNoteDBList(Tcl_Interp *interp)
{
  struct gl_list_t *dbl;
  unsigned long len;
  dbl = ListNotesDatabases();
  if (dbl != NULL) {
    len = gl_length(dbl);
    while (len>0) {
      Tcl_AppendElement(interp,(char *)SCP(gl_fetch(dbl,len)));
      len--;
    }
  }
  return TCL_OK;
}

static 
int LibrNoteLangs(Tcl_Interp *interp, symchar *dbid)
{
  struct gl_list_t *langs;
  unsigned long len;
  langs = GetNotesAllLanguages(dbid);
  if (langs==NULL) {
    Tcl_AppendResult(interp,"dbid invalid: ",(char *)SCP(dbid),(char *)NULL);
    return TCL_ERROR;
  }
  len = gl_length(langs);
  while (len > 0) {
    Tcl_AppendElement(interp,(char *)SCP(gl_fetch(langs,len)));
    len--;
  }
  gl_destroy(langs);
  return TCL_OK;
}

/* this function does not return the notes on qualified names
 * since we don't have an elegant way of executing the query.
 * We need more switches to the notes query syntax tcl
 * interface to manage those.
 * This returns notes about simple names.
 * This function is conceptually several functions, arg!
 * The empty symchar "" is treated as NULL.
 */
static 
int LibrGetNotes(Tcl_Interp *interp,symchar *type, symchar *lang,
                 symchar *child, symchar *method, long noteptr, long tokenptr,
                 symchar *dbid)
{
  struct gl_list_t *notes;
  struct Note *n;
  struct bracechar *bc;
  char *text;
  char linenum[40];
  unsigned long len;
  symchar *empty;
  struct Name *qlfdid;
  symchar *typename, *language, *childname, *methodname;
  struct gl_list_t *tl, *ll, *cl, *ml, *ndl;
  void *hold;

  if (tokenptr != (long)NULL) {
    /* release previously held result */
    ReleaseNoteData(dbid,(void *)tokenptr);
    return TCL_OK;
  }
  if (noteptr == (long)NULL) {
    /* return pointer (as text) to held list we find */
    typename = (   (type==NULL   || SCLEN(type)<1)   ? NOTESWILD : type);
    language = (   (lang==NULL   || SCLEN(lang)<1)   ? NOTESWILD : lang);
    childname = (  (child==NULL  || SCLEN(child)<1)  ? NOTESWILD : child);
    methodname = ( (method==NULL || SCLEN(method)<1) ? NOTESWILD : method);
    tl = gl_create(2);
    gl_append_ptr(tl,(VOIDPTR)typename);
    ll = gl_create(2);
    gl_append_ptr(ll,(VOIDPTR)language);
    cl = gl_create(2);
    gl_append_ptr(cl,(VOIDPTR)childname);
    ml = gl_create(2);
    gl_append_ptr(ml,(VOIDPTR)methodname);
    ndl = gl_create(2);
    gl_append_ptr(ndl,(VOIDPTR)nd_empty);
    gl_append_ptr(ndl,(VOIDPTR)nd_name);
    notes = GetNotesList(dbid,tl,ll,cl,ml,ndl);
    gl_destroy(tl);
    gl_destroy(ll);
    gl_destroy(cl);
    gl_destroy(ml);
    gl_destroy(ndl);
    hold = HoldNoteData(dbid,notes);
    sprintf(linenum,"%ld",(long)hold);
    Tcl_AppendResult(interp,linenum,(char *)NULL);
    return TCL_OK;
  } else {
    /* return formatted record */
    notes = GetExactNote(dbid,(struct Note *)noteptr);
  }

  /* list notes must not be held before here because we
   * destroy it at the END.
   */
  if (notes==NULL) {
    Tcl_AppendResult(interp,"note not found in ",(char *)SCP(dbid),
                     (char *)NULL);
    return TCL_ERROR;
  }
  len = gl_length(notes);
  empty = AddSymbolL("",0);
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL) {
      continue;
    }
    typename = GetNoteType(n);
    if (typename == NULL) {
      typename = empty;
    }
    childname = GetNoteId(n);
    if (childname == NULL) {
      childname = empty;
    }
    language = GetNoteLanguage(n);
    if (language == NULL) {
      language = empty;
    }
    methodname = GetNoteMethod(n);
    if (methodname == NULL) {
      methodname = empty;
    }
    Tcl_AppendResult(interp,"{{",(char *)SCP(typename),"} {",
                                (char *)SCP(language),"} {",(char *)NULL);
    qlfdid = (struct Name *)GetNoteData(n,nd_name);
    if (childname==empty && qlfdid != NULL) {
      text = WriteNameString(qlfdid);
      Tcl_AppendResult(interp, text,"} {",(char *)NULL);
      ascfree(text);
    } else {
      Tcl_AppendResult(interp, (char *)SCP(childname),"} {",(char *)NULL);
    }
    bc = GetNoteText(n);
    if (bc == NULL) {
      text = (char *)SCP(empty);
    } else {
      text = (char *)BCS(bc);
    }
    Tcl_AppendResult(interp, (char *)SCP(methodname),"} {",text,"}",
                     (char *)NULL);
    if (noteptr == (long)NULL) {
      /* close element */
      Tcl_AppendResult(interp, "} ",(char *)NULL);
    } else {
      /* digging up everything on specific note */
      text = (char *)GetNoteFilename(n);
      if (text == NULL) {
        text = (char *)SCP(empty);
      }
      sprintf(linenum,"%d",GetNoteLineNum(n));
      Tcl_AppendResult(interp, " {",text,"} {",linenum,"}} ", (char *)NULL);
    }
  }
 /* END */
  gl_destroy(notes);
  return TCL_OK;
}

/* Function to set up the tcl regexp engine and call it with the
 * notes in heldlist or database for matches against pattern.
 */
static
int LibrMatchNotes(Tcl_Interp *interp, char *pattern,
                   long heldlist,symchar *dbid)
{
  struct gl_list_t *notes = NULL;
  int status = TCL_OK;
  void *held;
  char idnum[40];
  struct NoteEngine *ne;
  if (pattern==NULL) {
    Tcl_AppendResult(interp,"NOTES match needs pattern string to match",
                     (char *)NULL);
    return TCL_ERROR;
  }
  if (heldlist != (long)NULL) {
    notes = HeldNotes(dbid,(void *)heldlist);
    if (notes == NULL) {
      Tcl_AppendResult(interp,"NOTES database token given not valid",
                       (char *)NULL);
      return TCL_ERROR;
    }
  }
  ne = NotesCreateEngine(interp,
                         (NEInitFunc)Tcl_RegExpCompile,
                         (NECompareFunc)Tcl_RegExpExec);
  if (ne == NULL) {
    Tcl_AppendResult(interp,"NOTES match unable to set up regexp engine",
                     (char *)NULL);
    return TCL_ERROR;
  }
  sprintf(idnum,"xxx");
  notes = GetMatchingNotes(dbid,pattern,notes,ne);
  if (notes != NULL) {
    held = HoldNoteData(dbid,notes);
    sprintf(idnum,"%ld",(long)held);
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp,idnum,(char *)NULL);
  } else {
    if (strlen(Tcl_GetStringResult(interp)) == 0) {
      sprintf(idnum,"%ld",(long)0);
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp,idnum,(char *)NULL);
    } else {
      status = TCL_ERROR;
      /* else leave possible error in interp */
    }
  }
  NotesDestroyEngine(ne);
  return status;
}

/* this function returns the notes on everything in more or less
 * easily sortable columns form.
 */
static 
int LibrDumpNotes(Tcl_Interp *interp, int tmax, long heldlist, symchar *dbid)
{
  struct gl_list_t *notes;
  struct Note *n;
  struct bracechar *bc;
  char *text;
  char *abbr, idnum[40];
  int tlen; /* length of text in a note */
  int row; /* the 'row' in the database, which may change with new reads
            * and should not be shown to the user.
            */
  unsigned long len;
  symchar *empty;
  struct Name *qlfdid;
  symchar *typename, *language, *childname, *methodname;

  if (tmax < 5) {
    tmax = 5;
  }
 
  if (heldlist != (long)NULL) {
    notes = HeldNotes(dbid,(void *)heldlist);
    if (notes == NULL) {
      Tcl_AppendResult(interp,"NOTES database token given not valid",
                       (char *)NULL);
      return TCL_ERROR;
    }
  } else {
    notes = GetNotes(dbid,NOTESWILD,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
    if (notes==NULL) {
      return TCL_OK; /* empty database */
    }
  }
  empty = AddSymbolL("~",1);

  /* process type names */
  row = 0;
  len = gl_length(notes);
  Tcl_AppendResult(interp,"{",(char *)NULL);
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    typename = GetNoteType(n);
    if (typename == NULL) {
      typename = empty;
    }
    sprintf(idnum,"%d",row);
    Tcl_AppendResult(interp,"{{",
                     (char *)SCP(typename),"} ",idnum,
                            "} ", (char *)NULL);
    row++;
  }
  Tcl_AppendResult(interp,"} {",(char *)NULL);
  /* process languages */
  row = 0;
  len = gl_length(notes);
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    language = GetNoteLanguage(n);
    if (language == NULL) {
      language = empty;
    }
    sprintf(idnum,"%d",row);
    Tcl_AppendResult(interp,"{{",
                     (char *)SCP(language),"} ",idnum,
                            "} ", (char *)NULL);
    row++;
  }
  Tcl_AppendResult(interp,"} {",(char *)NULL);
  /* process names. use qlfdid iff id == NULL */
  row = 0;
  len = gl_length(notes);
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    childname = GetNoteId(n);
    qlfdid = (struct Name *)GetNoteData(n,nd_name);
    if (childname == NULL) {
      childname = empty;
    }
    sprintf(idnum,"%d",row);
    if (childname == empty && qlfdid != NULL) {
      text = WriteNameString(qlfdid);
      Tcl_AppendResult(interp,"{{", text, "} ",idnum, "} ", (char *)NULL);
      ascfree(text);
    } else {
      Tcl_AppendResult(interp,"{{", (char *)SCP(childname),"} ",idnum, "} ",
                       (char *)NULL);
    }
    row++;
  }
  Tcl_AppendResult(interp,"} {",(char *)NULL);
  
  /* process method names */
  row = 0;
  len = gl_length(notes);
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    methodname = GetNoteMethod(n);
    if (methodname == NULL) {
      methodname = empty;
    }
    sprintf(idnum,"%d",row);
    Tcl_AppendResult(interp,"{{",
                     (char *)SCP(methodname),"} ",idnum,
                            "} ", (char *)NULL);
    row++;
  }
  Tcl_AppendResult(interp,"} {",(char *)NULL);

  /* process text */
  row = 0;
  len = gl_length(notes);
  abbr = (char *)ascmalloc(tmax+1);
  if (abbr == NULL) {
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp,"NOTES dump: out of memory",(char *)NULL);
    gl_destroy(notes);
    return TCL_ERROR;
  }
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    bc = GetNoteText(n);
    if (bc == NULL) {
      text = (char *)SCP(empty);
      tlen = 0;
    } else {
      text = (char *)BCS(bc);
      tlen = BCL(bc);
    }
    if (tlen < tmax) {
      sprintf(abbr,"%s",text);
    } else {
      sprintf(abbr,"%.*s...",tmax-3,text); /* fixme variable prec %s fmt */
    }
    sprintf(idnum,"%d",row);
    Tcl_AppendResult(interp,"{{", abbr, "} ",idnum, "} ", (char *)NULL);
    row++;
  }
  Tcl_AppendResult(interp,"} {",(char *)NULL);
  ascfree(abbr);

  /* process filename,line */
  row = 0;
  len = gl_length(notes);
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    tlen = GetNoteLineNum(n);
    text = (char *)GetNoteFilename(n);
    if (text == NULL) {
      text = (char *)SCP(empty);
    }
    /* fixme. want leaf name only. use file tail in tcl */
    sprintf(idnum,":%d} %d",tlen,row);
    Tcl_AppendResult(interp,"{{", text,/*d} d*/ idnum, "} ", (char *)NULL);
    row++;
  }
  Tcl_AppendResult(interp,"} {", (char *)NULL);

  /* process record number, which we are cheating an using the pointer for. */
  row = 0;
  len = gl_length(notes);
  while (len > 0) {
    n = (struct Note *)gl_fetch(notes,len);
    len--;
    if (n==NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    sprintf(idnum,"%lu",(unsigned long)n);
    Tcl_AppendResult(interp,"{", idnum,(char *)NULL);
    sprintf(idnum,"%d",row);
    Tcl_AppendResult(interp," ", idnum,"} ",(char *)NULL);
    row++;
  }
  Tcl_AppendResult(interp,"}", (char *)NULL);

  gl_destroy(notes);
  return TCL_OK;
}

static
int LibrUnimplemented(Tcl_Interp *interp, CONST84 char **argv)
{
  Tcl_AppendResult(interp,"Unimplemented option '",argv[1],"' in ",
                   Asc_LibrQueryTypeCmdHN,(char *)NULL);
  return TCL_ERROR;
}

STDHLF(Asc_LibrQueryTypeCmd,(Asc_LibrQueryTypeCmdHL1,
                             Asc_LibrQueryTypeCmdHL10,
                             Asc_LibrQueryTypeCmdHL20,
                             Asc_LibrQueryTypeCmdHL30,
                             Asc_LibrQueryTypeCmdHL40,
                             Asc_LibrQueryTypeCmdHL50,
                             Asc_LibrQueryTypeCmdHL60,
                             Asc_LibrQueryTypeCmdHL70,
                             Asc_LibrQueryTypeCmdHL80,
                             Asc_LibrQueryTypeCmdHL85,
                             Asc_LibrQueryTypeCmdHL90,
                             Asc_LibrQueryTypeCmdHL100,
                             Asc_LibrQueryTypeCmdHL110,
                             Asc_LibrQueryTypeCmdHL115,
                             Asc_LibrQueryTypeCmdHL120,
                             Asc_LibrQueryTypeCmdHL130,
                             Asc_LibrQueryTypeCmdHL131,
                             Asc_LibrQueryTypeCmdHL132,
                             Asc_LibrQueryTypeCmdHL133,
                             Asc_LibrQueryTypeCmdHL135,
                             Asc_LibrQueryTypeCmdHL136,
                             Asc_LibrQueryTypeCmdHL137,
                             Asc_LibrQueryTypeCmdHL140, 
                             HLFSTOP));
int Asc_LibrQueryTypeCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char **argv)
{
  enum qtype {
    q_error,
    q_ancestors,
    q_basemethods,
    q_catalog,
    q_childnames,
    q_childinfo,
    q_definition,
    q_exists,
    q_externalfunctions,
    q_findtype,
    q_filetypes,
    q_fundamentals,
    q_language,
    q_methods,
    q_modulelist,
    q_notes,
    q_notesdump,
    q_notekinds,
    q_notesmatch,
    q_notesdblist,
    q_roottypes
  } q = q_error;
  symchar *type=NULL;
  symchar *method=NULL;
  symchar *child=NULL;
  symchar *language=NULL;
  symchar *dbid=NULL;
  int mtype=0;
  int i;
  int status;
  long noteptr = (long)NULL; /* parsed as long and cast to ptr. */
  long tokenptr = (long)NULL; /* parsed as long and cast to ptr. */
  char *pattern = NULL;
  struct TypeDescription *desc=NULL;

  ASCUSE;  /* see if first arg is -help */
  
  if (argc < 2) {
    Tcl_AppendResult(interp,Asc_LibrQueryTypeCmdHN " called without arguments",
                     (char *)NULL);
    return TCL_ERROR;
  }
  if (strcmp(argv[1],"-ancestors")==0) {
    q = q_ancestors;
  }
  if (strcmp(argv[1],"-basemethods")==0) {
    q = q_basemethods;
  }
  if (strcmp(argv[1],"-catalog")==0) {
    q = q_catalog;
  }
  if (strcmp(argv[1],"-childnames")==0) {
    q = q_childnames;
  }
  if (strcmp(argv[1],"-childinfo")==0) {
    q = q_childinfo;
  }
  if (strcmp(argv[1],"-definition")==0) {
    q = q_definition;
  }
  if (strcmp(argv[1],"-exists")==0) {
    q = q_exists;
  }
  if (strcmp(argv[1],"-externalfunctions")==0) {
    q = q_externalfunctions;
  }
  if (strcmp(argv[1],"-findtype")==0) {
    q = q_findtype;
  }
  if (strcmp(argv[1],"-filetypes")==0) {
    q = q_filetypes;
  }
  if (strcmp(argv[1],"-fundamentals")==0) {
    q = q_fundamentals;
  }
  if (strcmp(argv[1],"-language")==0) {
    q = q_language;
  }
  if (strcmp(argv[1],"-methods")==0) {
    q = q_methods;
  }
  if (strcmp(argv[1],"-modulelist")==0) {
    q = q_modulelist;
  }
  if (strcmp(argv[1],"-notesdblist")==0) {
    q = q_notesdblist;
  }
  if (strcmp(argv[1],"-notes")==0) {
    q = q_notes;
  }
  if (strcmp(argv[1],"-notesdump")==0) {
    q = q_notesdump;
  }
  if (strcmp(argv[1],"-notekinds")==0) {
    q = q_notekinds;
  }
  if (strcmp(argv[1],"-notesmatch")==0) {
    q = q_notesmatch;
  }
  if (strcmp(argv[1],"-roottypes")==0) {
    q = q_roottypes;
  }
  if (q==q_error) {
    Tcl_AppendResult(interp,"Unknown option '",argv[1],"' to ",
                     Asc_LibrQueryTypeCmdHN,(char *)NULL);
    return TCL_ERROR;
  }
  /* pick off the options */
  for (i=2; i < argc; /* ifs do increment */ ) {
    if (strcmp(argv[i],"-type")==0) {
      if (i < (argc-1)) {
        type = AddSymbol(argv[i+1]);
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-dbid")==0) {
      if (i < (argc-1)) {
        dbid = AddSymbol(argv[i+1]);
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-child")==0) {
      if (i < (argc-1)) {
        child = AddSymbol(argv[i+1]);
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-method")==0) {
      if (i < (argc-1)) {
        method = AddSymbol(argv[i+1]);
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-pattern")==0) {
      if (i < (argc-1)) {
        pattern = QUIET(argv[i+1]);
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-language")==0) {
      if (i < (argc-1)) {
        language = AddSymbol(argv[i+1]);
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-destroytoken")==0 ||
        strcmp(argv[i],"-notestoken")==0) {
      if (i < (argc-1)) {
        status = Tcl_ExprLong(interp,argv[i+1],&tokenptr);
        if (status != TCL_OK) {
          return status;
        }
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-record")==0) {
      if (i < (argc-1)) {
        status = Tcl_ExprLong(interp,argv[i+1],&noteptr);
        if (status != TCL_OK) {
          return status;
        }
      }
      i += 2;
      continue;
    }
    if (strcmp(argv[i],"-mtype")==0 ||
        strcmp(argv[i],"-textwidth")==0) {
      if (i < (argc-1)) {
        status = Tcl_GetInt(interp,argv[i+1],&mtype);
        if (status != TCL_OK) {
          return status;
        }
      }
      i += 2;
      continue;
    }
    Tcl_AppendResult(interp,"Unknown option '",argv[i],"' to ",
                     Asc_LibrQueryTypeCmdHN,(char *)NULL);
    return TCL_ERROR;
  }

  if (type != NULL) {
    desc = FindType(type);
    if (q != q_exists && desc == NULL) {
      Tcl_AppendResult(interp,"Unknown type '",SCP(type),"' to ",
                       Asc_LibrQueryTypeCmdHN,(char *)NULL);
      return TCL_ERROR;
    }
  }
  switch (q) {
  case q_ancestors:
    return LibrAncestorType(interp,desc);
  case q_basemethods:
    return LibrModelDefinitionMethods(interp);
  case q_catalog:
    return LibrCatalog(interp);
  case q_childnames:
    return LibrTypeChildren(interp,desc);
  case q_childinfo:
    return LibrChildInfo(interp,desc,child);
  case q_definition:
    return LibrUnimplemented(interp,argv);
  case q_exists:
    if (desc==NULL) {
      Tcl_SetResult(interp,"0",TCL_STATIC);
    } else {
      Tcl_SetResult(interp,"1",TCL_STATIC);
    }
    return TCL_OK;
  case q_externalfunctions:
    return LibrExternalFuncs(interp);
  case q_findtype:
    return LibrFindType(interp,desc);
  case q_filetypes:
    return LibrFileExtsCmd(interp);
  case q_fundamentals:
    return LibrGetFundamentals(interp);
  case q_language:
    return LibrUnimplemented(interp,argv);
  case q_methods:
    return LibrMethods(interp,desc);
  case q_modulelist:
    return LibrModuleList(interp,mtype);
  case q_notekinds:
    return LibrNoteLangs(interp,dbid);
  case q_notesdblist:
    return LibrNoteDBList(interp);
  case q_notes:
    return LibrGetNotes(interp,type,language,child,method,
                        noteptr,tokenptr,dbid);
  case q_notesmatch:
    return LibrMatchNotes(interp,pattern,tokenptr,dbid);
  case q_notesdump:
    if (mtype == 0) {
      mtype = 15;
    }
    return LibrDumpNotes(interp,mtype,tokenptr,dbid);
  case q_roottypes:
    return LibrRootTypes(interp);
  default:
    Tcl_AppendResult(interp,"Unhandled option '",argv[1],"' in ",
                     Asc_LibrQueryTypeCmdHN,(char *)NULL);
    break;
  }
  return TCL_ERROR;
}
