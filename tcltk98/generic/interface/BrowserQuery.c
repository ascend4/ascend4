/*
 *  BrowserQuery.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.51 $
 *  Version control file: $RCSfile: BrowserQuery.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:04 $
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

#include <math.h>
#include <stdarg.h>
#include "tcl.h"
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/instance_name.h"
#include "compiler/dimen.h"
#include "compiler/symtab.h"
#include "compiler/instance_io.h"
#include "compiler/types.h"
#include "compiler/stattypes.h"
#include "compiler/statement.h"
#include "compiler/statio.h"
#include "compiler/extfunc.h"
#include "compiler/find.h"
#include "compiler/relation_type.h"
#include "compiler/relation_io.h"
#include "compiler/functype.h"
#include "compiler/safe.h"
#include "compiler/relation_util.h"
#include "compiler/logical_relation.h"
#include "compiler/logrel_io.h"
#include "compiler/logrel_util.h"
#include "compiler/dimen_io.h"
#include "compiler/instance_name.h"
#include "compiler/instquery.h"
#include "compiler/parentchild.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/mathinst.h"
#include "compiler/visitinst.h"
#include "compiler/atomvalue.h"
#include "compiler/module.h"
#include "compiler/library.h"
#include "compiler/setinstval.h"
#include "compiler/setinst_io.h"
#include "compiler/units.h"
#include "compiler/qlfdid.h"
#include "solver/slv_types.h"
#include "interface/HelpProc.h"
#include "interface/plot.h"
#include "interface/BrowserQuery.h"
#include "interface/Qlfdid.h"
#include "interface/SimsProc.h"
#include "interface/BrowserProc.h"
#include "interface/UnitsProc.h"
#include "packages/ascFreeAllVars.h"

#ifndef lint
static CONST char BrowserQueryID[] = "$Id: BrowserQuery.c,v 1.51 2003/08/23 18:43:04 ballan Exp $";
#endif


#ifndef MAXIMUM_STRING_LENGTH
#define MAXIMUM_STRING_LENGTH 2048
#endif
#define MAXIMUM_SET_LENGTH 256 /* optimistic as all hell */
#define BRSTRINGMALLOC \
(char *)ascmalloc((MAXIMUM_STRING_LENGTH+1)* sizeof(char))

int g_do_values = 0;
unsigned long g_do_onechild = 0;

static
int BrowIsRelation(struct Instance *i)
{
  return ArrayIsRelation(i);
}

static
int BrowIsLogRel(struct Instance *i)
{
  return ArrayIsLogRel(i);
}

static
int BrowIsWhen(struct Instance *i)
{
  return ArrayIsWhen(i);
}


static
int BrowIsInstanceInWhen(struct Instance *i)
{
  switch(InstanceKind(i)) {
    case BOOLEAN_ATOM_INST:
    case BOOLEAN_CONSTANT_INST:
    case INTEGER_ATOM_INST:
    case INTEGER_CONSTANT_INST:
    case SYMBOL_ATOM_INST:
    case SYMBOL_CONSTANT_INST:
    case REL_INST:
      return 1;
    default:
      return 0;
  }
}

static
int BrowIsModel(struct Instance *i)
{
  while((InstanceKind(i)==ARRAY_INT_INST)
        ||(InstanceKind(i)==ARRAY_ENUM_INST)) {
    if (NumberChildren(i)==0) { /* just to make sure children exist */
      break;
    }
    i = InstanceChild(i,1L);
  }
  if (InstanceKind(i)==MODEL_INST) {
    return 1;
  } else {
    return 0;
  }
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
int BrowIsAtomicArray(struct Instance *i)
{
  while((InstanceKind(i)==ARRAY_INT_INST)||(InstanceKind(i)==ARRAY_ENUM_INST)){
    if (NumberChildren(i)==0) { /* just to make sure children exist */
      break;
    }
    i = InstanceChild(i,1L);
  }
  if ((i)&&(Asc_BrowInstIsAtomic(i))) {
    return 1;
  } else {
    return 0;
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


int Asc_BrowIsRelationCmd(ClientData cdata, Tcl_Interp *interp,
                          int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char buf[MAXIMUM_NUMERIC_LENGTH];  /* string to hold integer */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __brow_isrelation ?cuurent?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to \"__brow_isrelation\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  sprintf(buf, "%d", BrowIsRelation(i));
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}


int Asc_BrowIsLogRelCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char buf[MAXIMUM_NUMERIC_LENGTH];   /* string to hold integer */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __brow_islogrel ?current?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to \"__brow_islogrel\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  sprintf(buf, "%d", BrowIsLogRel(i));
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

int Asc_BrowIsWhenCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char buf[MAXIMUM_NUMERIC_LENGTH];   /* string to hold integer */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp,"wrong # args : Usage __brow_iswhen ?current?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to \"__brow_iswhen\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  sprintf(buf, "%d", BrowIsWhen(i));
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}


int Asc_BrowIsInstanceInWhenCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char buf[MAXIMUM_NUMERIC_LENGTH];   /* string to hold integer */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args: "
                  "Usage __brow_isintanceinwhen ?current?search?", TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to \"__brow_isinstanceinwhen\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  sprintf(buf, "%d", BrowIsInstanceInWhen(i));
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}


int Asc_BrowIsModelCmd(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char buf[MAXIMUM_NUMERIC_LENGTH];       /* string to hold integer */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __brow_ismodel ?cuurent?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to \"__brow_ismodel\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  sprintf(buf, "%d", BrowIsModel(i));
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

struct gl_list_t *Asc_BrowShortestPath(CONST struct Instance *i,
                                   CONST struct Instance *ref,
                                   unsigned int height, unsigned int best)
{
  struct gl_list_t *path,*shortest=NULL;
  unsigned long c,len;
  unsigned mybest= UINT_MAX;
  if (height>=best) {
    return NULL;
  }
  if (i==ref) {
    shortest = gl_create(1L);
    gl_append_ptr(shortest,(char *)ref);
    return shortest;
  }
  if ((len=NumberParents(i))) {
    for(c=len;c>=1;c--) {
      path = Asc_BrowShortestPath(InstanceParent(i,c),ref,height+1,mybest);
      if (path!=NULL) {
        if (shortest==NULL) {
          shortest=path;
          mybest = height+gl_length(path);
        } else {
          if (gl_length(path)<gl_length(shortest)) {
            gl_destroy(shortest);
            shortest = path;
            mybest = height+gl_length(path);
          } else {
            gl_destroy(path);
          }
        }
      }
    }
    if (shortest) {
      gl_append_ptr(shortest,NULL);
      for(c=gl_length(shortest);c>1;c--) {
        gl_store(shortest,c,gl_fetch(shortest,c-1));
      }
      gl_store(shortest,1,(char *)i);
      assert((ref!=NULL)||(gl_length(shortest)==InstanceShortDepth(i)));
    }
  } else {
    if(ref==NULL) {
      shortest = gl_create(1L);
      gl_append_ptr(shortest,(char *)i);
      assert(gl_length(shortest)==InstanceShortDepth(i));
    } else {
      return NULL;
    }
  }
  return shortest;
}

/*
 * I am always going to use ref as NULL ; registered as \"__brow_iname\"
 */
int Asc_BrowWriteInstanceNameCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[])
{
  CONST struct Instance *i, *ref;
  char *tmp;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc > 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args: Usage \"__brow_iname\" ?current?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc == 1 ) {
    i = g_curinst;
  } else {
    if (strncmp(argv[1],"currrent",3)==0) {
      i = g_curinst;
    } else if (strncmp(argv[1],"search",3)==0) {
      i = g_search_inst;
    } else {
      Tcl_SetResult(interp, "Invalid args to \"__brow_iname\"", TCL_STATIC);
      return TCL_ERROR;
    }
  }
  if (!i) {
    Tcl_AppendResult(interp,"NULL_INSTANCE",(char *)NULL);
    return TCL_OK;
  }
  ref = (CONST struct Instance *)NULL;	/* at the moment ref always == NULL */
  tmp = WriteInstanceNameString(i,ref);
  Tcl_AppendResult(interp,tmp,(char *)NULL);
  ascfree(tmp);
  return TCL_OK;
  /*NOTREACHED*/
}

int Asc_BrowWriteAliasesCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct Instance *i = NULL;
  struct gl_list_t *strings;
  char *tmp;
  unsigned long c,len;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"aliases\" ?current?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  }
  if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  }
  if (i==NULL) {
    Tcl_SetResult(interp,
                  "No instance found or usage error: aliases <current,search>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  strings = WriteAliasStrings(i);
  len = gl_length(strings);
  if (len) {
    for(c=1;c<=len;c++) {
      tmp = (char *)gl_fetch(strings,c);
      Tcl_AppendResult(interp,"{",(char *)NULL);
      Tcl_AppendResult(interp,tmp,(char *)NULL);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
      if (tmp!=NULL) {
        ascfree(tmp);
      }
    }
  } else {
    Tcl_SetResult(interp, "aliases: Instance with no names??", TCL_STATIC);
    return TCL_ERROR;
  }
  gl_destroy(strings);
  return TCL_OK;
}

static
struct count_numbers {
unsigned long it;	/* total unique instances counted. */

unsigned long at;	/* total ATOM-like instances counted */
unsigned long aa;	/* total names of these atoms. */
unsigned long ai;	/* total creating names of these atoms. */

unsigned long rt;	/* total relation-like instances */
unsigned long ra;	/* total names of these relations. */
unsigned long ri;	/* total creating names of these relations. */

unsigned long mt;	/* total MODEL instances counted. */
unsigned long ma;	/* total names of these models. */
unsigned long mi;	/* total creating names of these models. */

unsigned long At;	/* total array instances counted. */
unsigned long Aa;	/* total names of these arrays. */
unsigned long Ai;	/* total creating names of these arrays. */
unsigned long Nt;	/* total NULL instances */
unsigned long Dt;	/* total Dummy instances */
} g_cn;

static
void Initgcn()
{
  g_cn.it =
  g_cn.at =
  g_cn.aa =
  g_cn.ai =
  g_cn.rt =
  g_cn.ra =
  g_cn.ri =
  g_cn.mt =
  g_cn.ma =
  g_cn.mi =
  g_cn.At =
  g_cn.Aa =
  g_cn.Ai =
  g_cn.Nt =
  g_cn.Dt = 0L;
}

static void CountNames(struct Instance *i)
{
  unsigned long isanames, aliasnames;

  g_cn.it++;
  if (i==NULL) {
    g_cn.Nt++;
    return;
  }
  aliasnames = CountAliases(i);
  isanames = CountISAs(i);
  switch (InstanceKind(i)) {
  case REAL_INST:
  case BOOLEAN_INST:
  case SYMBOL_INST:
  case INTEGER_INST:
  case SET_INST:
  case SIM_INST:
    break; /* shouldn't be here */
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case INTEGER_ATOM_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case SET_ATOM_INST:
    g_cn.at++;
    g_cn.aa += aliasnames;
    g_cn.ai += isanames;
    break;
  case REL_INST:
  case LREL_INST:
  case WHEN_INST:
    g_cn.rt++;
    g_cn.ra += aliasnames;
    g_cn.ri += isanames;
    break;
  case MODEL_INST:
    g_cn.mt++;
    g_cn.ma += aliasnames;
    g_cn.mi += isanames;
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    g_cn.At++;
    g_cn.Aa += aliasnames;
    g_cn.Ai += isanames;
    break;
  case DUMMY_INST:
    g_cn.Dt++;
    break;
  default:
    break;
  }
}

int Asc_BrowCountNamesCmd(ClientData cdata, Tcl_Interp *interp,
                      int argc, CONST84 char *argv[])
{
  struct Instance *i = NULL;
  char tmp[40];

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage \"count_names\" <current,search>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  }
  if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  }
  if (i==NULL) {
    Tcl_SetResult(interp, "No instance found or usage error:"
                  " count_names <current, search>", TCL_STATIC);
    return TCL_ERROR;
  }
  Initgcn();
  SilentVisitInstanceTree(i,CountNames,0,0);
  /* write stuff to interp here */

  sprintf(tmp,"%lu", g_cn.it);
  Tcl_AppendResult(interp,"{INSTANCE-total: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.mt);
  Tcl_AppendResult(interp," {MODEL-total: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.ma);
  Tcl_AppendResult(interp," {MODEL-alii: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.mi);
  Tcl_AppendResult(interp," {MODEL-isas: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.At);
  Tcl_AppendResult(interp," {ARRAY-total: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.Aa);
  Tcl_AppendResult(interp," {ARRAY-alii: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.Ai);
  Tcl_AppendResult(interp," {ARRAY-isas: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.at);
  Tcl_AppendResult(interp," {ATOM-total: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.aa);
  Tcl_AppendResult(interp," {ATOM-alii: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.ai);
  Tcl_AppendResult(interp," {ATOM-isas: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.rt);
  Tcl_AppendResult(interp," {LRWN-total: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.ra);
  Tcl_AppendResult(interp," {LRWN-alii: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.ri);
  Tcl_AppendResult(interp," {LRWN-isas: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.Nt);
  Tcl_AppendResult(interp," {NULL-total: ",tmp,"}",(char *)NULL);
  sprintf(tmp,"%lu", g_cn.Dt);
  Tcl_AppendResult(interp," {DUMMY-total: ",tmp,"}",(char *)NULL);
  return TCL_OK;
}

int Asc_BrowWriteISAsCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct Instance *i = NULL;
  struct gl_list_t *strings;
  char *tmp;
  unsigned long c,len;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"isas\" <current,search>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  }
  if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  }
  if (i==NULL) {
    Tcl_SetResult(interp,
                  "No instance found or usage error: isas <current, search>",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  strings = WriteISAStrings(i);
  len = gl_length(strings);
  if (len) {
    for(c=1;c<=len;c++) {
      tmp = (char *)gl_fetch(strings,c);
      Tcl_AppendResult(interp,"{",(char *)NULL);
      Tcl_AppendResult(interp,tmp,(char *)NULL);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
      if (tmp!=NULL) {
        ascfree(tmp);
      }
    }
  } else {
    Tcl_SetResult(interp, "isas: Instance with no names?", TCL_STATIC);
    return TCL_ERROR;
  }
  gl_destroy(strings);
  return TCL_OK;
}

/*
 * I am not sure of the semantics. But I did not like what was
 * here either. Soooo, I am just returning the name of all the
 * instances in the clique. kaa.
 */
int Asc_BrowWriteCliqueCmd(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  CONST struct Instance *i;
  CONST struct Instance *tmp;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  i = g_curinst;
  if(!i) {
    Tcl_SetResult(interp, "NULL_INSTANCE in \"clique\"", TCL_STATIC);
    return TCL_ERROR;
  }
  tmp = i;
  do {
    char *tmpstr;
    Tcl_AppendResult(interp,"{",(char *)NULL); /* make proper list elems */
    tmpstr = WriteInstanceNameString(tmp,NULL);
    Tcl_AppendResult(interp,tmpstr,(char *)NULL);
    ascfree(tmpstr);
    Tcl_AppendResult(interp,"} ",(char *)NULL);
    tmp = NextCliqueMember(tmp);
  } while(tmp != i);
  return TCL_OK;
}

/*
 *  Children List Commands.
 */
static
int BrowWriteInstSet(char *ftorv, CONST struct set_t *s)
{
  unsigned long c,len;
  int available = 0;
  char *tmpstr, *mark;
  switch(SetKind(s)) {
  case empty_set:
    sprintf(ftorv,"[]");
    return 0;              /* done processing, so return ok */
  case integer_set:
  case string_set:
    mark = tmpstr = Asc_MakeInitString(256);
    len = Cardinality(s);
    for(c=1;c<=len;c++) {
      if (SetKind(s)==integer_set) {
        sprintf(mark, (c<len) ? "%ld," : "%ld",FetchIntMember(s,c));
      } else {
        sprintf(mark, (c<len) ? "'%s'," : "'%s'", FetchStrMember(s,c));
      }
      available = 256 - strlen(tmpstr);
      if (available <= 80) {
        break;
      }
      mark = &tmpstr[strlen(tmpstr)];
    }
    break;
  default:
    FPRINTF(stderr,"Error in BrowWriteSet\n");
    return 1;              /* done processing, so return nok */
  }
  if (c<len) {/* indicating that the loop exited early */
    sprintf(ftorv,"[%s...]",tmpstr); /* truncate if too long */
    ascfree(tmpstr);
    return 0;
  } else {
    sprintf(ftorv,"[%s]",tmpstr);
    ascfree(tmpstr);
    return 0;
  }
}

static
int BrowWriteFrac(char *fdims, struct fraction frac, CONST char *str,
               int *CONST p)
{
  char sval[MAXIMUM_NUMERIC_LENGTH];
  if (Numerator(frac)) {
    if (*p) {
      strcat(fdims,"*");
    }
    (*p) = 1;
    if (Denominator(frac)==1) {
      sprintf(sval,"%s^%d",str,Numerator(frac));
   }  else {
      sprintf(sval,"%s^(%d/%d)",str,Numerator(frac),Denominator(frac));
    }
    strcat(fdims,sval);
  }
  return 0;
}

int Asc_BrowWriteDimensions(char *fdims, CONST dim_type *dimp)
{
  struct fraction frac;
  int printed=0;
  if (IsWild(dimp)) {
    sprintf(fdims,"*");
  } else {
    int i;
    for( i=0; i<NUM_DIMENS; i++ ) {
       frac = GetDimFraction(*dimp,i);
       BrowWriteFrac(fdims,frac,DimName(i),&printed);
    }
    if (!printed) {
      sprintf(fdims,"%s","");
    }
  }
  return 0;
}

int Asc_BrowWriteAtomValue(char *ftorv, CONST struct Instance *i)
{
  CONST struct relation *rel;
  CONST struct logrelation *lrel;
  CONST struct set_t *s;
  enum inst_t kind;
  enum Expr_enum type;

  if (InstanceKind(i)==REL_INST) {
    rel = GetInstanceRelation(i,&type);
    if (!rel) {
      return 1;
    } else {
      sprintf(ftorv,"%.*g",Asc_UnitGetCPrec(),
              RelationResidual(rel));
      return 0;
    }
  }
  if (InstanceKind(i)==LREL_INST) {
    lrel = GetInstanceLogRel(i);
    if (!lrel) {
      return 1;
    } else {
      sprintf(ftorv,LogRelResidual(lrel)?"TRUE":"FALSE");
      return 0;
    }
  }
  if (InstanceKind(i)==WHEN_INST) {
      return 0;
  }
  if (InstanceKind(i)==DUMMY_INST) {
      return 0;
  }
  if (AtomAssigned(i)) {
    switch(kind = InstanceKind(i)) {
    case REAL_INST:
    case REAL_ATOM_INST:
    case REAL_CONSTANT_INST:
      sprintf(ftorv,"%.6g",RealAtomValue(i));
      break;
    case INTEGER_INST:
    case INTEGER_ATOM_INST:
    case INTEGER_CONSTANT_INST:
      sprintf(ftorv,"%ld",GetIntegerAtomValue(i));
      break;
    case SET_INST:
    case SET_ATOM_INST:
      s = SetAtomList(i);
      BrowWriteInstSet(ftorv,s);
      break;
    case BOOLEAN_INST:
    case BOOLEAN_ATOM_INST:
    case BOOLEAN_CONSTANT_INST:
      sprintf(ftorv,GetBooleanAtomValue(i)?"TRUE":"FALSE");
      break;
    case SYMBOL_INST:
    case SYMBOL_ATOM_INST:
    case SYMBOL_CONSTANT_INST:
      sprintf(ftorv,"'%s'",SCP(GetSymbolAtomValue(i)));
      break;
    default:
      Asc_Panic(2, NULL, "Unrecognized atom type in BrowInstAtomValue\n");
    }
  } else {
    sprintf(ftorv,"UNDEFINED");
  }
  return 0;
}

int Asc_BrowWriteAtomChildren(Tcl_Interp *interp, CONST struct Instance *i)
{
  unsigned long c,len;
  unsigned long start,end;
  struct InstanceName rec;
  CONST struct Instance *child;
  enum inst_t kind;
  char *fname, *ftorv, *fdims;
  struct TypeDescription *desc;
  ChildListPtr clist;
  int domany=0; /* if 0, only one child is asked for and so,ignore visibility*/

  if (i==NULL) {
    return TCL_ERROR;
  }
  len = NumberChildren(i);
  if (!len) {
    return TCL_ERROR;
  }
  desc = InstanceTypeDesc(i);
  clist = GetChildList(desc);
  assert(clist!=NULL);
  if ((g_do_onechild>0)&&(g_do_onechild<=len)) {
    start = end = g_do_onechild;
  } else {
    start = 1;
    end = len;
    domany = 1;
  }
  fname = Asc_MakeInitString(256);               /* Make the strings */
  ftorv = Asc_MakeInitString(256);
  fdims = Asc_MakeInitString(80);
  for(c=start;c<=end;c++) {
    if (ChildVisible(clist,c)==0 && domany) {
      continue;
    }
    child = InstanceChild(i,c);
    kind = InstanceKind(child);
    rec = ChildName(i,c);
    assert(InstanceNameType(rec)==StrName);
    sprintf(fname,"%s ",SCP(InstanceNameStr(rec)));

    if (g_do_values) {
      Asc_BrowWriteAtomValue(ftorv,child);
      if ((kind==REAL_INST)||
          (kind==REAL_ATOM_INST)||
          (kind==REAL_CONSTANT_INST)||
          (kind==REL_INST)) {
        char * ustr = Asc_UnitValue(child);
        char op[5] = " = ";
        if (kind==REL_INST) {
          sprintf(&op[0]," : ");
        }
        if (ustr!=NULL) {
          Tcl_AppendResult(interp,"{",fname,&op[0],ustr,"}"," ",(char *)NULL);
        } else {
          Tcl_AppendResult(interp,
                           "{",fname,&op[0],"????","}"," ",(char *)NULL);
        }
      } else {
        if (kind==LREL_INST) {
          char op[5] = "";
          sprintf(&op[0]," : ");
          Tcl_AppendResult(interp,"{",fname,&op[0],ftorv,"}"," ",(char *)NULL);
        } else {
          Tcl_AppendResult(interp,"{",fname," = ",ftorv,"}"," ",(char *)NULL);
        }
      }
    } else {
      sprintf(ftorv,"%s ",SCP(InstanceType(child)));
      Tcl_AppendResult(interp,"{",fname," IS_A ",ftorv,"}"," ",(char *)NULL);
    }
    Asc_ReInitString(fname); Asc_ReInitString(ftorv); Asc_ReInitString(fdims);
  }
  ascfree(fname); ascfree(ftorv); ascfree(fdims);   /* Free the strings */
  return TCL_OK;
}

int Asc_BrowWriteNameRec(char *fname, CONST struct InstanceName *rec)
{
  switch(InstanceNameType(*rec)) {
  case IntArrayIndex:
    sprintf(fname,"[%ld]",InstanceIntIndex(*rec));
    break;
  case StrArrayIndex:
    sprintf(fname,"['%s']",SCP(InstanceStrIndex(*rec)));
    break;
  case StrName:
    strcpy(fname,SCP(InstanceNameStr(*rec)));
    break;
  }
  return TCL_OK;
}

/* ftorv: string buffer, somewhat riskily assumed big enough.
 * parent: parent instance of the child we are writing about.
 * child: the object to write the value or type of.
 * cnum: the position of the child in the parent's child list.
 */
static
int BrowWriteTypeOrValue(char *ftorv,
                         CONST struct Instance *parent,
                         CONST struct Instance *child,
                         unsigned long cnum)
{
  char tmp[1024];
  enum Expr_enum reltype;
  if (child==NULL) {
     if (parent==NULL || cnum==0) {
       sprintf(ftorv," IS_A NULL_INSTANCE");
       return TCL_OK;
     } else {
       sprintf(ftorv," IS_A NULL_INSTANCE %s",
          ( ChildDeclaration(parent,cnum)!=NULL &&
            StatWrong(ChildDeclaration(parent,cnum))
          ) ? "PERMANENTLY" : "TEMPORARILY");
       return TCL_OK;
     }
  }
  switch(InstanceKind(child)) {
  case MODEL_INST:
    sprintf(ftorv, " IS_A %s",SCP(InstanceType(child)));
    break;
  case REL_INST:
    if (GetInstanceRelation(child,&reltype)==NULL) {
      sprintf(ftorv," IS_A NULL_RELATION");
      return TCL_OK;
    }
  case REAL_INST:
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
  case SET_INST:
  case SET_ATOM_INST:
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
    if (g_do_values) {
      sprintf(tmp, " = ");             /* might make this into "add to front */
      Asc_BrowWriteAtomValue(ftorv,child); /* he should copy into the space */
      strcat(tmp,ftorv);
      strcpy(ftorv,tmp);
    } else {
      if (InstanceKind(child) == SET_INST ||
          InstanceKind(child) == SET_ATOM_INST) {
        sprintf(ftorv," IS_A %s OF %s",SCP(InstanceType(child)),
          (IntegerSetInstance(child) ?
             BASE_CON_INTEGER_NAME : BASE_CON_SYMBOL_NAME
          )
        );
      } else {
        sprintf(ftorv," IS_A %s",SCP(InstanceType(child)));
      }
    }
    break;
  case LREL_INST:
    if (GetInstanceLogRel(child)==NULL) {
      sprintf(ftorv," IS_A NULL_LOGIC_RELATION");
      return TCL_OK;
    }
    if (g_do_values) {
      sprintf(tmp, "%s", "");
      Asc_BrowWriteAtomValue(ftorv,child);
      strcat(tmp,ftorv);
      strcpy(ftorv,tmp);
    } else {
      sprintf(ftorv," IS_A logic_relation");
    }
    break;
  case DUMMY_INST:
    if (!g_do_values) {
      sprintf(ftorv, " IS_A UnSelectedPart");
    }
    break;
  case WHEN_INST:
    sprintf(ftorv," IS_A when");
    break;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    sprintf(ftorv," IS_A ARRAY OF %s REFINEMENTS",
      SCP(GetName(GetArrayBaseType(InstanceTypeDesc(child)))));
    break;
  default:
    FPRINTF(stderr,"Unknown instance type in AtomWriteTypeOrValue.\n");
    sprintf(ftorv," IS_A UNKNOWN_INSTANCE_TYPE");
    break;
    /*NOTREACHED*/
  }
  return TCL_OK;
}

/*
 * Modified to consider the types with bit TYPESHOW set to zero. VRR
 */
static
int BrowWriteArrayChildren(Tcl_Interp *interp, CONST struct Instance *i)
{
  unsigned long c,len;
  CONST struct Instance *child;
  enum inst_t childkind;
  CONST struct TypeDescription *d;
  struct InstanceName rec;
  char *fname,*ftorv;
  fname = Asc_MakeInitString(80);
  ftorv = Asc_MakeInitString(1024);
  len = NumberChildren(i);
  for(c=1;c<=len;c++) {      /* For type with TYPESHOW bit set to zero */
    child = InstanceChild(i,c);
    d = InstanceTypeDesc(child);
    if (!TypeShow(d)) {
      continue;
    }
    childkind = InstanceKind(child);
    rec = ChildName(i,c);
    Asc_BrowWriteNameRec(fname,&rec);
    BrowWriteTypeOrValue(ftorv,i,child,c);
    if (g_do_values && ((childkind==REAL_INST)||
                        (childkind==REAL_ATOM_INST)||
                        (childkind==REAL_CONSTANT_INST)||
                        (childkind==REL_INST))) {
      char * ustr = Asc_UnitValue(child);
      char op[5] = " = ";
      if (childkind==REL_INST) {
        sprintf(&op[0]," : ");
      }
      if (ustr!=NULL) {
        Tcl_AppendResult(interp,"{",fname,&op[0],ustr,"}"," ",(char *)NULL);
      } else {
        Tcl_AppendResult(interp,"{",fname,&op[0],"????","}"," ",(char *)NULL);
      }
    } else {
        if (g_do_values && (childkind==LREL_INST)) {
          char op[5] = "";
          sprintf(&op[0]," : ");
          Tcl_AppendResult(interp,"{",fname,&op[0],ftorv,"}"," ",(char *)NULL);
        } else {
          Tcl_AppendResult(interp,"{",fname,ftorv,"}"," ",(char *)NULL);
        }
      }
    Asc_ReInitString(fname);
    Asc_ReInitString(ftorv);
  }
  ascfree(fname); ascfree(ftorv);
  return TCL_OK;
}

/*
 * To find if arrays of types with TYPESHOW bit set to zero. VRR
 */
static
int BrowTypeOfArrayIsShown(struct Instance *child)
{
  enum inst_t childkind;
  struct Instance *arraychild;
  CONST struct TypeDescription *d;
  unsigned int flag;

  if (child==NULL) {
    return 1; /* show NULL children, suppressed or not */
  }
  childkind = InstanceKind(child);
  switch (childkind) {
    case MODEL_INST:
    case DUMMY_INST:
    case REAL_INST:
    case REAL_ATOM_INST:
    case REAL_CONSTANT_INST:
    case BOOLEAN_INST:
    case BOOLEAN_ATOM_INST:
    case BOOLEAN_CONSTANT_INST:
    case INTEGER_INST:
    case INTEGER_ATOM_INST:
    case INTEGER_CONSTANT_INST:
    case SET_INST:
    case SET_ATOM_INST:
    case SYMBOL_INST:
    case SYMBOL_ATOM_INST:
    case SYMBOL_CONSTANT_INST:
    case REL_INST:
    case LREL_INST:
    case WHEN_INST:
      d = InstanceTypeDesc(child);
      flag = TypeShow(d);
      return flag;
    case ARRAY_INT_INST:
    case ARRAY_ENUM_INST:
      if (NumberChildren(child)==0) {
        return 1;
      }
      arraychild = InstanceChild(child,1L);
      flag = BrowTypeOfArrayIsShown(arraychild);
      return flag;
    default:
      FPRINTF(stderr,"Unknown child type found in BrowTypeOfArrayIsShown\n");
      return 1;
  }
}


/*
 * Modified to consider the types with bit TYPESHOW set to zero. VRR
 * and child list visibility bit. BAA.
 * and ATOM invisibility and MODEL part passing (baa) 
 * unless fetching specific child.
 */
static
void BrowListModelChildren(Tcl_Interp *interp, struct Instance *i, int atoms,
                           int show_passed_parts)
{
  unsigned long c,len,start;
  struct InstanceName name;
  struct Instance *child;
  struct Instance *arraychild;
  CONST struct TypeDescription *d;
  enum inst_t childkind;
  char *fname, *ftorv, *fdims;
  ChildListPtr clist;
  unsigned int flag; /* if 1, attempt to show child */


  fname = Asc_MakeInitString(80);
  ftorv = Asc_MakeInitString(1024);
  fdims = Asc_MakeInitString(80);
  len = NumberChildren(i);
  if (len) {
    clist = GetChildList(InstanceTypeDesc(i));
    if (g_do_onechild !=0 && g_do_onechild <= len) {
      start = len = g_do_onechild;
    } else {
      start = 1;
    }
    for(c=start;c<=len;c++) {
      /* check for any part with VISIBILITY bit set to zero,
       * or passed set to 1
       */
      if (( ChildVisible(clist,c)==0 || 
            (ChildPassed(clist,c) == 1 && !show_passed_parts) 
          ) &&
          !g_do_onechild) {
        continue;
      }
      /* check for any type with TYPESHOW bit set to zero 
       * or atoms we don't want to see.
       */
      child = InstanceChild(i,c);
      if (child != NULL) {
        d = InstanceTypeDesc(child);
        if (!TypeShow(d)) {
          continue;
        }
        flag = 1;     /* For arrays of types with TYPESHOW bit set to zero */
        childkind = InstanceKind(child);
        switch (childkind) {
        case ARRAY_INT_INST:
        case ARRAY_ENUM_INST:
          if (NumberChildren(child)==0) {
            flag = 1;
          } else {
            arraychild = InstanceChild(child,1L);
            flag = BrowTypeOfArrayIsShown(arraychild);
          }
          break;
        case REL_INST:
        case SET_ATOM_INST:
        case LREL_INST:
        case REAL_ATOM_INST:
        case REAL_CONSTANT_INST:
        case BOOLEAN_ATOM_INST:
        case BOOLEAN_CONSTANT_INST:
        case INTEGER_ATOM_INST:
        case INTEGER_CONSTANT_INST:
        case SYMBOL_ATOM_INST:
        case SYMBOL_CONSTANT_INST:
          flag = (unsigned int)atoms;
          break;
     /* impossible cases- models can't have subatoms:
      * case SET_INST:
      * case REAL_INST:
      * case BOOLEAN_INST:
      * case INTEGER_INST:
      * case SYMBOL_INST:
      */
        default:
          break;
        }
        if (!flag && !g_do_onechild) {
          continue;
        }
      }

      name = ChildName(i,c);
      Asc_BrowWriteNameRec(fname,&name);
      BrowWriteTypeOrValue(ftorv,i,child,c);
      if (g_do_values && ( (childkind==REAL_INST)||
                           (childkind==REAL_ATOM_INST)||
                           (childkind==REAL_CONSTANT_INST)||
                           (childkind==REL_INST)
                         )
          ) {
        char * ustr = Asc_UnitValue(child);
        char op[5] = " = \0";
        if (childkind==REL_INST) {
          sprintf(&op[0]," : ");
        }
        if (ustr!=NULL) {
          Tcl_AppendResult(interp,"{",fname,&op[0],ustr,"}"," ",(char *)NULL);
        } else {
          Tcl_AppendResult(interp,
                           "{",fname,&op[0],"????","}"," ",(char *)NULL);
        }
      } else {
        if (g_do_values && (childkind==LREL_INST)) {
          char op[5] = "\0";
          sprintf(&op[0]," : ");
          Tcl_AppendResult(interp,"{",fname,&op[0],ftorv,"}"," ",(char *)NULL);
        } else {
            Tcl_AppendResult(interp,"{",fname,ftorv,"}"," ",(char *)NULL);
        }
      }

      Asc_ReInitString(fname);
      Asc_ReInitString(ftorv);
      Asc_ReInitString(fdims);
    }
  }
  ascfree(fname);
  ascfree(ftorv);
  ascfree(fdims);
}


/*
 * Modified to consider the types with bit TYPESHOW set to zero. VRR
 * also, if show_child_atoms !=0, atoms will be included children
 * of models list. BAA: if show_passed_parts != 0, instances with
 * CBF_PASSED in child list will be shown.
 */
static
int BrowWriteInstance(Tcl_Interp *interp, struct Instance *i,
                      int show_child_atoms, int show_passed_parts)
{
  enum inst_t kind;
  struct Instance *child;

  switch(kind=InstanceKind(i)) {
  case MODEL_INST:
    BrowListModelChildren(interp,i,show_child_atoms,show_passed_parts);
    break;
  case DUMMY_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_INST:
  case SYMBOL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
    /* can't have children */
    break;
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SET_ATOM_INST:
  case SYMBOL_ATOM_INST:
    Asc_BrowWriteAtomChildren(interp,i);
    break;
  case REL_INST:
    Asc_BrowWriteAtomChildren(interp,i);
    break;
  case LREL_INST:
    Asc_BrowWriteAtomChildren(interp,i);
    break;
  case WHEN_INST:
    break;
  case ARRAY_INT_INST:
    if (NumberChildren(i)==0) {
      break;
    }
    child = InstanceChild(i,1L); /* For type with TYPESHOW bit set to zero */
    if (!BrowTypeOfArrayIsShown(child)) {
      break;
    }
    BrowWriteArrayChildren(interp,i);
    break;
  case ARRAY_ENUM_INST:
    if (NumberChildren(i)==0) {
      break;
    }
    child = InstanceChild(i,1L); /* For type with TYPESHOW bit set to zero */
    if (!BrowTypeOfArrayIsShown(child)) {
      break;
    }
    BrowWriteArrayChildren(interp,i);
    break;
  default:
    Tcl_SetResult(interp,"Unrecognized type in BrowWriteInstance", TCL_STATIC);
    break;
  }
  return TCL_OK;
}

int Asc_BrowWriteInstanceCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  struct Instance *i;
  unsigned long ndx;
  int c;                  /*  looping variable  */
  int nok=0;
  int show_child_atoms=0;
  int show_passed_parts=0;

  (void)cdata;    /* stop gcc whine about unused parameter */

  ASCUSE;

  if (( argc < 3 ) || ( argc > 6 )) {
    Tcl_AppendResult(interp, "Usage : ",
                     Asc_BrowWriteInstanceCmdHU,(char *)NULL);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    /* search context */
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp,
                  "Invalid args : should be \"current\" or \"search\" ",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (i==NULL) {
    /* a NULL instance MUST NOT return an error !!!*/
    Tcl_ResetResult(interp);
    return TCL_OK;
  }
  if (strncmp(argv[2],"all",3)==0) {        /* child index or all */
    g_do_onechild = 0;
  } else {
    ndx = atol(argv[2]);
    if (ndx) {
      g_do_onechild = ndx;
    } else {
      Tcl_SetResult(interp, "Invalid args : should be \"all\" or an integer",
                    TCL_STATIC);
      return TCL_ERROR;
    }
  }
  for (c=3;c<argc;c++) {                   /* attributes */
    if (strcmp(argv[c],"TYPE")==0) {
      g_do_values = 0;
    }
    if (strcmp(argv[c],"VALUE")==0) {
      g_do_values = 1;
    }
    if (strcmp(argv[c],"ATOMS")==0) {
      show_child_atoms = 1;
    }
    if (strcmp(argv[c],"PASSED")==0) {
      show_passed_parts = 1;
    }
  }
  nok = BrowWriteInstance(interp,i,show_child_atoms,show_passed_parts);
  return nok;
}

/* The code below handle plotting of data. It should be able to write a
 * file that may be accepted by either gnuplot, xgraph and maybe xmgr.
 * It makes use of the plot code in $ASCENDDIST/interface.
 * Takes the names the name of an instance and returns trueTypeShow if
 * the type is plottable, i.e, is more refined than plot_type. Later
 * this will be made a define so that we can change it.
 */
int Asc_BrowIsPlotAllowedCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char buf[MAXIMUM_NUMERIC_LENGTH];       /* string to hold integer */
  int result=0;         /* 0 = FALSE; 1 = TRUE */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong # args : Usage \"b_isplottable ?cur?search?",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to b_isplottable", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "0", TCL_STATIC);
    return TCL_OK;
  }
  result = plot_allowed(i);
  sprintf(buf, "%d", result);
  Tcl_SetResult(interp, buf, TCL_VOLATILE);
  return TCL_OK;
}

int Asc_BrowPreparePlotFileCmd(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char *argv[])
{
  struct Instance *i;
  char *filename;

  (void)cdata;    /* stop gcc whine about unused parameter */

  /* We use the g_plot_type defined in plot.h */
  if (( argc < 3 ) || ( argc > 5 )) {
    Tcl_AppendResult(interp,"wrong # args : ",
                 "Usage \"b_prepplotfile\" inst filename type",(char *)NULL);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {      /* check instance context */
    i = g_curinst;
  } else if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  } else {
    Tcl_SetResult(interp, "invalid args to b_prepplotfile", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!i) {
    Tcl_SetResult(interp, "NULL Instance -- Nothing to plot", TCL_STATIC);
    return TCL_ERROR;
  }
  filename = QUIET(argv[2]);                         /* grab filename */
  if ( argc == 3 ) {                          /* get plot_type */
    g_plot_type = PLAIN_PLOT;
  } else if ( argc == 4 ) {
    if (strncmp(argv[3],"plain_plot",5)==0)  {
      g_plot_type=PLAIN_PLOT;
    } else if (strncmp(argv[3],"gnu_plot",3)==0) {
      g_plot_type=GNU_PLOT;
    } else if (strncmp(argv[3],"xgraph_plot",5)==0) {
      g_plot_type=XGRAPH_PLOT;
    } else {
      g_plot_type=PLAIN_PLOT;
    }
  }
  /* This will be set up in time with other flags to set up for the
   *  different plotting types -- it will be  a third arg for this call.
   *  When you do so remember to fix the slv_interface code
   */
  plot_prepare_file(i,filename);
  return TCL_OK;
}

int Asc_BrowRefinesMeCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct TypeDescription *desc;
  int result=0;

  (void)cdata;    /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  if ( argc != 1 ) {
    Tcl_SetResult(interp, "wrong # args to \"is_type_refined\"", TCL_STATIC);
    return TCL_ERROR;
  }
  if (!g_curinst) {
    Tcl_SetResult(interp, "is_type_refined called on null.", TCL_STATIC);
    return TCL_ERROR;
  }
  desc = InstanceTypeDesc(g_curinst);
  result = IsTypeRefined(desc);
  if (result) {
    Tcl_SetResult(interp, "1", TCL_STATIC);
  } else {
    Tcl_SetResult(interp, "0", TCL_STATIC);
  }
  return TCL_OK;
}

static FILE *b_val_io_file = NULL;
static CONST84 char *b_acmd = NULL;
static struct Instance *g_rbval_ref = NULL;

/*
 * this function also needs to save symbol_atom/inst_values.
 */
static
void BrowWriteRBValues(struct Instance *i)
{
  char *i_name=NULL;
  if (!i) {
    return;
  }
  switch(InstanceKind(i)) {
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case REAL_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_INST:
    FPRINTF(b_val_io_file,"%s",b_acmd);
    i_name = WriteInstanceNameString(i,g_rbval_ref);
/* old code
   WriteInstanceName(b_val_io_file,i,NULL);
*/
    FPRINTF(b_val_io_file,"%s",i_name);
    FPRINTF(b_val_io_file,"} ");
    WriteAtomValue(b_val_io_file,i);
    FPRINTF(b_val_io_file," -relative\n");
    break;
  case INTEGER_CONSTANT_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
    /* don't write constant values */
    return;
  default:
    break;
  }
  if (i_name != NULL) {
    ascfree(i_name);
  }
}




static
void BrowWriteRBValues2(struct Instance *i)
{
  if (!i) {
    return;
  }
  switch(InstanceKind(i)) {
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case REAL_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_INST:
    FPRINTF(b_val_io_file,"%s",b_acmd);
    WriteAnyInstanceName(b_val_io_file,i);
    FPRINTF(b_val_io_file,"} ");
    WriteAtomValue(b_val_io_file,i);
    FPRINTF(b_val_io_file,"\n");
    break;
  case INTEGER_CONSTANT_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
    /* don't write constant values */
    return;
  default:
    break;
  }
}

/*
 *  End of the faster write routines. Added an additional arg,
 *  which says original (default) or fast. Now takes 6 args.
 *  If current,root or search given, then a dummy name (i.e, other than "")
 *  must be provided. The 6th arg says fast, but sloppy (shortest path name
 *  is not used), slow and pretty, and is optional but defaults to slow.
 */

int Asc_BrowWriteValues(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  CONST84 char *fname,*il;
  struct Instance *i = NULL;		/* must init to NULL */
  int fast_but_sloppy = 0;		/* default is original */
  int nok = 0;

  (void)cdata;    /* stop gcc whine about unused parameter */
  
  if (argc<5|| argc>6) {
    Tcl_AppendResult(interp,"wrong # args: Usage : \"bwritevalues\" ",
                     "filename  acmd  current?root?search?qualified ",
                     "dummy_name?qlfdid  <fast_slow>",(char *)NULL);
    return TCL_ERROR;
  }

  fname = argv[1];
  b_acmd = argv[2];
  il = argv[3];
  switch (il[0]) {			/* establish the context */
  case 'c':
    i = g_curinst; break;
  case 'r':
    i = g_root; break;
  case 's':
    i = g_search_inst; break;
  case 'q':				/* argv[4] ignored in other cases */
    nok = Asc_QlfdidSearch2(QUIET(argv[4]));
    if (nok) {
      i = NULL;
    } else {
      i = g_search_inst;
    }
    break;
  default:
    break;
  }
  if (!i) {				/* check instance */
    Tcl_SetResult(interp, "bwritevalues given bad instance.", TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc == 6 ) {			/* establish which function */
    fast_but_sloppy = 1;
  }
  b_val_io_file = fopen(fname,"w");	/* check file access */
  if (!b_val_io_file) {
    Tcl_SetResult(interp,"bwritevalues: unable to open data file.",TCL_STATIC);
    return TCL_ERROR;
  }
  FPRINTF(b_val_io_file,"qlfdid {");
  WriteInstanceName(b_val_io_file,i,NULL);
  FPRINTF(b_val_io_file,"}\n");
  g_rbval_ref = i;

  /* use BrowWriteRBValue which has been fixed up to use relative
     names */
  fast_but_sloppy = 0;
  
  if (fast_but_sloppy) {			/* write the file */
    VisitInstanceTree(i,BrowWriteRBValues2,0,1);
  } else {
    VisitInstanceTree(i,BrowWriteRBValues,0,1);
  }

  fclose(b_val_io_file);
  return TCL_OK;
}



static struct gl_list_t *g_find_type_list = NULL;
static struct TypeDescription *g_type_desc = NULL;

static
struct Instance *Brow_MatchAttr(struct Instance *i, symchar *attr_desc)
{
  unsigned long nch,pos;
  struct InstanceName rec;

  if (i!=NULL && attr_desc!=NULL) {
    nch = NumberChildren(i);
    if (nch) {
      SetInstanceNameType(rec,StrName);
      SetInstanceNameStrPtr(rec,attr_desc);
      pos = ChildSearch(i,&rec); /* symchar safe */
      if (pos) {
        return InstanceChild(i,pos);
      }
    }
  }
  return NULL;
}

static
int Special_AttrMatch(int argc, char **argv)
{
  if ( argc < 3 ) {
    return 1;
  }
  if (argv[3] != NULL && strcmp(argv[3],"VALUE")==0) {
    return 1;
  }
  if (argv[3] != NULL && strcmp(argv[3],"UNDEFINED")==0) {
    return 1;
  }
  return 0;
}

static
struct Instance *FilterModels(struct Instance *i,
                              int argc, char **argv)
{
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  return i;
}

/* The following batch of static functions are specific to
   filter find list and make the same assumptions about argv, argc
   as in Asc_BrowFindTypeCmd
*/
static struct Instance *FilterReals(struct Instance *i,
                                    int argc, char **argv)
{
  double r_value, r_low, r_high;
  char *str;

  if (!i) {
    return NULL;
  }
  str = NULL;
  switch(argc) {
  case 5: /* just matching value of attribute to within tolerance of lowval */
    if (AtomAssigned(i)) {
      r_value = (double)strtod(argv[4],&str);
      if (str == argv[4]) { /* r_value is bogus */
        return NULL;
      }
      if (fabs(r_value - RealAtomValue(i))<1.0e-08) { /* FIXME */
        return i;
      } else {
        return NULL;
      }
    } else {
      if (!strcmp(argv[4],"UNDEFINED")) {
        return i;
      }
    }
    return NULL;
  case 6:
    if (AtomAssigned(i)) {
      r_low = (double)atof(argv[4]);
      r_high = (double)atof(argv[5]);
      r_value = RealAtomValue(i);
      if ((r_value >= r_low)&&(r_high >= r_value)) {
        return i;
      } else {
        return NULL;
      }
    } else {
      if (!strcmp(argv[4],"UNDEFINED")) {
        return i;
      }
    }
    return NULL;
  case 3: /* Should not be here */
  case 4: /* Should not be here */
  default:
    return NULL;
  }
}

static struct Instance *FilterBooleans(struct Instance *i,
                                       int argc, char **argv)
{
  int b_value;
  if (!i) {
    return NULL;
  }
  switch(argc) {
  case 5: /* just matching value of attribute */
  case 6: /* This should really be an error !! but will let it slide */
    if (strcmp(argv[4],"UNDEFINED")==0) {
      if (!AtomAssigned(i)) {
        return i;
      } else {
        return NULL;
      }
    } else {
      if (AtomAssigned(i)) {
        b_value = atoi(argv[4]);
        if (b_value == GetBooleanAtomValue(i)) {
          return i;
        } else {
          return NULL;
        }
      }
    }
    return NULL;
  case 3: /* Should not be here */
  case 4: /* Should not be here */
  default:
    return NULL;
  }
}

static struct Instance *FilterIntegers(struct Instance *i,
                                int argc, char **argv)
{
  long i_value, i_low, i_high;
  char *str;

  str = NULL;
  if (!i) {
    return NULL;
  }
  switch(argc) {
  case 5: /* just matching value of attribute */
    if (AtomAssigned(i)) {
      i_value = strtol(argv[4],&str,10);
      if (str == argv[4]) { /* bogus ivalue */
        return NULL;
      }
      if (i_value == GetIntegerAtomValue(i)) {
        return i;
      } else {
        return NULL;
      }
    } else {
      if (!strcmp(argv[4],"UNDEFINED")) {
        return i;
      }
    }
    return NULL;
  case 6:
    if (AtomAssigned(i)) {
      i_low = atol(argv[4]);
      i_high = atol(argv[5]);
      i_value = GetIntegerAtomValue(i);
      if ((i_value >= i_low)&&(i_high >= i_value)) {
        return i;
      } else {
        return NULL;
      }
    } else {
      if (!strcmp(argv[4],"UNDEFINED")) {
        return i;
      }
    }
    return NULL;
  case 3: /* Should not be here */
  case 4: /* Should not be here */
  default:
    return NULL;
  }
}

/*
 * this ought to check values by ptr, after calling
 * addsymbol at the top.
 */
static
struct Instance *FilterSymbols(struct Instance *i,
                               int argc, char **argv)
{
  char *s_value, *s_low, *s_high;

  if (!i) {
    return NULL;
  }
  switch(argc) {
  case 5: /* just matching value of attribute */
    if (AtomAssigned(i)) {
      s_value = argv[4];
      if (strcmp(s_value,SCP(GetSymbolAtomValue(i)))==0) {
        return i;
      } else {
        return NULL;
      }
    } else {
      if (!strcmp(argv[4],"UNDEFINED")) {
        return i;
      }
    }
    return NULL;
  case 6:
    if (AtomAssigned(i)) {
      s_low = argv[4];
      s_high = argv[5];
      s_value = (char *)SCP(GetSymbolAtomValue(i));
      if ((strcmp(s_low,s_value)<=0)&&(strcmp(s_value,s_high)<=0)) {
        return i;
      } else {
        return NULL;
      }
    } else {
      if (!strcmp(argv[4],"UNDEFINED")) {
        return i;
      }
    }
    return NULL;
  case 4: /* Should not be here */
  case 3: /* Should not be here */
  default:
    return NULL;
  }
}

static struct Instance *FilterLogRelations(struct Instance *i,
                                 int argc, char **argv)
{
  CONST struct logrelation *lrel;
  int b_value;
  char *str;

  str = NULL;
  if (!i) {
    return NULL;
  }
  lrel = GetInstanceLogRel(i);
  if (!lrel) {
    return NULL;
  }
  switch(argc) {
  case 5: /* just matching value of attribute */
    b_value = strtol(argv[4],&str,10);
    if (str == argv[4]) { /* bogus bval*/
      return NULL;
    }
    if ( b_value == LogRelResidual(lrel)) {
      return i;
    } else {
      return NULL;
    }
  case 6:
  case 4: /* Should not be here */
  case 3: /* Should not be here */
  default:
    return NULL;
  }
}

static
struct Instance *FilterWhens(struct Instance *i,
                             int argc, char **argv)
{
  (void)argc;     /* stop gcc whine about unused parameter */
  (void)argv;     /* stop gcc whine about unused parameter */

  return i;
}

static struct Instance *FilterRelations(struct Instance *i,
                                 int argc, char **argv)
{
  double r_value, r_low, r_high;
  CONST struct relation *rel;
  enum Expr_enum reltype;
  char *str;

  if (!i) {
    return NULL;
  }
  rel = GetInstanceRelation(i,&reltype);
  str = NULL;
  switch(argc) {
  case 5: /* just matching value of attribute */
    r_value = (double)strtod(argv[4],&str);
    if (fabs(r_value - RelationResidual(rel))<1.0e-08) {
      return i;
    } else {
      return NULL;
    }
  case 6:
    r_low = (double)atof(argv[4]);
    r_high = (double)atof(argv[5]);
    r_value = RelationResidual(rel);
    if ((r_value >= r_low)&&(r_high >= r_value)) {
      return i;
    } else {
      return NULL;
    }
  case 4: /* Should not be here */
  case 3: /* Should not be here */
  default:
    return NULL;
  }
}

static struct Instance *FilterSets(struct Instance *i, int argc, char **argv)
{

  if (!i) {
    return NULL;
  }
  switch(argc) {
  case 5: /* just matching value attribute if UNDEFINED */ /* fall thru */
  case 6: /* being sloppy. what is the 'range' of a set? */
    if (AtomAssigned(i)) {
      return NULL;
    }
    if (!strcmp(argv[4],"UNDEFINED")) {
      return i;
    }
    return NULL;
  case 4: /* Should not be here */
  case 3: /* Should not be here */
  default:
    return NULL;
  }
}

/* filters the list sent, creating a new one. the old one is destroyed */
/* this code is a piece of garbage that needs to be cleaned up to reduce
 * all the strcmp that goes on, among other things. more kirkisms. 
 */
static struct gl_list_t *Brow_FilterFindList(struct gl_list_t *list,
                                             int argc, char **argv)
{
  unsigned long len,c;
  struct Instance *i;
  symchar *tablename;
  struct Instance *child;
  struct gl_list_t *new;
  int matchvalues;

  if (list==NULL) {
    return NULL;
  }
  len = gl_length(list);
  new = gl_create(len);
  matchvalues = Special_AttrMatch(argc,argv);

  if ( argc == 4 ) {	/* just matching the attribute existing */
    for (c=1;c<=len;c++) {
      i = (struct Instance *)gl_fetch(list,c);
      if (matchvalues) {
        child = i;
      /* matching the instance itself, since they all have values if atoms */
      } else {
        tablename = AddSymbol(argv[3]);
        child = Brow_MatchAttr(i,tablename);
        /* matching the name of an ATOM child */
      }
      if (child!=NULL) {
        continue;   /* failed the filter, so process the next item */
      }
      gl_append_ptr(new,(char *)i);    /* append the parent *NOT* the child */
    }
    gl_destroy(list);
    return new;
  } else {				/* need to match values as well */
    for (c=1;c<=len;c++) {
      i = (struct Instance *)gl_fetch(list,c);
      if (matchvalues) {
        child = i;
      } else {
        tablename = AddSymbol(argv[3]);
        child = Brow_MatchAttr(i,tablename);
      }
      if (child==NULL) {
        continue; /* failed the filter, so process the next item */
      }
      switch(InstanceKind(child)) {
      case DUMMY_INST:
        break;
      case ARRAY_INT_INST:
      case ARRAY_ENUM_INST:
      case MODEL_INST:
        child = FilterModels(child,argc,argv);
        break;
      case REAL_INST:
      case REAL_ATOM_INST:
      case REAL_CONSTANT_INST:
        child = FilterReals(child,argc,argv);
        break;
      case INTEGER_INST:
      case INTEGER_ATOM_INST:
      case INTEGER_CONSTANT_INST:
        child = FilterIntegers(child,argc,argv);
        break;
      case SYMBOL_INST:
      case SYMBOL_ATOM_INST:
      case SYMBOL_CONSTANT_INST:
        child = FilterSymbols(child,argc,argv);
        break;
      case BOOLEAN_INST:
      case BOOLEAN_ATOM_INST:
      case BOOLEAN_CONSTANT_INST:
        child = FilterBooleans(child,argc,argv);
        break;
      case REL_INST:
        child = FilterRelations(child,argc,argv);
        break;
      case LREL_INST:
        child = FilterLogRelations(child,argc,argv);
        break;
      case WHEN_INST:
        child = FilterWhens(child,argc,argv);
        break;
      case SET_INST:
      case SET_ATOM_INST:
        child = FilterSets(child,argc,argv);
        break;
      default:
        child = NULL;
        break;
      }
      if (child!=NULL) {
        gl_append_ptr(new,(char *)i); /* append the parent *NOT* the child */
      }
    }
    gl_destroy(list);
    return new;
  }
}

static
void Brow_MatchType(struct Instance *i)
{
  struct TypeDescription *desc1;
  CONST struct TypeDescription *desc2;
  assert(g_type_desc!=NULL);
  if (i) {
    desc1 = InstanceTypeDesc(i);
    if (desc1==NULL) {/* must be atomchild or something else is wrong */
      return;
    }
    desc2 = MoreRefined(desc1,g_type_desc);
    if (desc2 != NULL && (desc1==desc2)) {
	 /* desc1 is more refined, so keep i */
      gl_append_ptr(g_find_type_list,(VOIDPTR)i);
    }
  }
}

static
struct gl_list_t *BrowFindTypeList(struct Instance *i,
                                   int argc,
                                   char **argv)
{
  struct gl_list_t *list = NULL;

  g_find_type_list = list = gl_create(200L);
  VisitInstanceTree(i,Brow_MatchType,0,0);
  if ( argc > 3 ) {  	/* need to filter the list */
    list = Brow_FilterFindList(list,argc,argv);
  }
  g_find_type_list = NULL;
  g_type_desc = NULL;
  return list;
}


/*
 * This function will attempt to find a given type and
 * to filter it using the value or a range of values of its
 * attributes.
 *\" __brow_find_type\" current/search type attribute value lowvalue highvalue.
 */
int Asc_BrowFindTypeCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  struct Instance *inst;
  struct TypeDescription *desc;
  struct gl_list_t *list = NULL;
  unsigned long len,c;
  struct Instance *i=NULL;
  int j;                           /*  looping variable  */

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc < 3 ) {
    Tcl_AppendResult(interp,"wrong # args: Usage \"__brow_find_type\" ",
      "<current,search> type [attribute [<lowvalue,matchvalue> [highvalue]]]",
      (char *)NULL);
    return TCL_ERROR;
  }
  for (j=0;j<argc;j++) {
    FPRINTF(stderr,"%d %s\n",j,argv[j]);
    if (argv[j]==NULL) {
      Tcl_SetResult(interp,
                    "__brow_find_type called with empty slot", TCL_STATIC);
      return TCL_ERROR;
    }
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  }
  if (strncmp(argv[1],"search",3)==0) {
    i = g_search_inst;
  }
  if (i==NULL) {
    Tcl_SetResult(interp, "__brow_find_type instance is NULL !", TCL_STATIC);
    FPRINTF(stderr,"__brow_find_type called incorrectly.\n");
    return TCL_ERROR;
  }
  desc = FindType(AddSymbol(argv[2]));
  if (desc==NULL) {
    Tcl_AppendResult(interp,"Type given does not exist",(char *)NULL);
    return TCL_ERROR;
  }
  g_type_desc = (struct TypeDescription *)desc;
  list = BrowFindTypeList(i,argc,QUIET2(argv));
  if (list != NULL) {
    len = gl_length(list);
    for (c=1;c<=len;c++) {
      char *tmps;
      inst = (struct Instance *)gl_fetch(list,c);
      Tcl_AppendResult(interp,"{",(char *)NULL); /* make proper list elems */
      tmps = WriteInstanceNameString(inst,i); /* use i as reference or else! */
      Tcl_AppendResult(interp,tmps,(char *)NULL);
      ascfree(tmps);
      Tcl_AppendResult(interp,"} ",(char *)NULL);
    }
    gl_destroy(list);
    list = NULL;
  }
  return TCL_OK;
}

int Asc_BrowRelationRelopCmd(ClientData cdata, Tcl_Interp *interp,
                         int argc, CONST84 char *argv[])
{
  struct Instance *i;
  CONST struct relation *rel;
  enum Expr_enum t, reltype;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong #args : Usage __brow_reln_relop ?cur?seach?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    i = g_search_inst;
  }
  if (i) {
    if (InstanceKind(i)!= REL_INST) {
      Tcl_SetResult(interp, "given instance is not a relation", TCL_STATIC);
      return TCL_ERROR;
    }
    rel = GetInstanceRelation(i,&reltype);
    if (!rel) {
      Tcl_SetResult(interp, "Instance has NULL relation", TCL_STATIC);
      return TCL_ERROR;
    }
    t = RelationRelop(rel);
    switch (t) {
    case e_equal:
      Tcl_SetResult(interp,"equal",TCL_STATIC);
      return TCL_OK;
    case e_notequal:
      Tcl_SetResult(interp,"notequal",TCL_STATIC);
      return TCL_OK;
    case e_less:
    case e_lesseq:
      Tcl_SetResult(interp,"less",TCL_STATIC);
      return TCL_OK;
    case e_greater:
    case e_greatereq:
      Tcl_SetResult(interp,"greater",TCL_STATIC);
      return TCL_OK;
    case e_maximize:
      Tcl_SetResult(interp,"maximize",TCL_STATIC);
      return TCL_OK;
    case e_minimize:
      Tcl_SetResult(interp,"minimize",TCL_STATIC);
      return TCL_OK;
    default:
      Tcl_SetResult(interp, "Unknown relation type ???", TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
    Tcl_SetResult(interp, "Null relation instance", TCL_STATIC);
    return TCL_ERROR;
  }
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
int BrowLogRelRelopCmd(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  struct Instance *i;
  CONST struct logrelation *lrel;
  enum Expr_enum t;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if ( argc != 2 ) {
    Tcl_SetResult(interp, "wrong #args : Usage __brow_lrel_relop ?cur?seach?",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    i = g_search_inst;
  }
  if (i) {
    if (InstanceKind(i)!= LREL_INST) {
      Tcl_SetResult(interp, "given instance is not a relation", TCL_STATIC);
      return TCL_ERROR;
    }
    lrel = GetInstanceLogRel(i);
    if (!lrel) {
      Tcl_SetResult(interp, "Instance has NULL logical relation", TCL_STATIC);
      return TCL_ERROR;
    }
    t = LogRelRelop(lrel);
    switch (t) {
    case e_boolean_eq:
      Tcl_SetResult(interp,"b_equal",TCL_STATIC);
      return TCL_OK;
    case e_boolean_neq:
      Tcl_SetResult(interp,"b_notequal",TCL_STATIC);
      return TCL_OK;
    default:
      Tcl_SetResult(interp, "Unknown logical relation type ???", TCL_STATIC);
      return TCL_ERROR;
    }
  } else {
    Tcl_SetResult(interp, "Null logical relation instance", TCL_STATIC);
    return TCL_ERROR;
  }
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */

int Asc_BrowClearVarsCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  int status;
  struct Instance *i;

  (void)cdata;    /* stop gcc whine about unused parameter */

  if (( argc < 1 ) || ( argc > 2 )) {
    Tcl_SetResult(interp, "wrong # args: Usage free_all_vars [qlfdid]",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if ( argc == 1 ) {		/* e.g., runproc clear */
    i = g_curinst;
  } else {			/* e.g., runproc a.b.c clear */
    status = Asc_QlfdidSearch3(argv[1],0);
    if (status==0) { 		/* catch inst ptr */
      i = g_search_inst;
    } else {			/* failed. bail out. */
      Tcl_AppendResult(interp,"free_all_vars: Asc_BrowClearVarsCmd: ",
                       "Could not find instance.",(char *)NULL);
      return TCL_ERROR;
    }
  }

  if (i==NULL) {
    Tcl_SetResult(interp, "Instance not found", TCL_STATIC);
    return TCL_ERROR;
  }
  /* assume everything will be ok from here on out */
  if (Asc_ClearVarsInTree(i) != 0) {
    FPRINTF(stderr,"ERROR:  (BrowserQuery) \n");
    FPRINTF(stderr,"        Type solver_var not defined.\n");
    FPRINTF(stderr,"        definition needed to clear vars.\n");
    Tcl_SetResult(interp, "ERROR: solver_var undefined. no action taken",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}
