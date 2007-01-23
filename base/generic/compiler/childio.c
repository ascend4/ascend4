/*
 *
 *  Child List output
 *  by Benjamin Allan
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: childio.c,v $
 *  Date last modified: $Date: 1998/06/11 17:36:22 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
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
 *  Implementation of Child list output
 */

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/dstring.h>

#include "symtab.h"
#include "instance_enum.h"
#include "fractions.h"
#include "dimen.h"
#include "expr_types.h"
#include "stattypes.h"
#include "statement.h"
#include "statio.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "cmpfunc.h"
#include "name.h"
#include "nameio.h"
#include "vlist.h"
#include "module.h"
#define __CHILD_ILLEGAL_ACCESS__
#include "childpriv.h"
#include "childio.h"

#ifndef lint
static CONST char ChildIOID[] = "$Id: childio.c,v 1.6 1998/06/11 17:36:22 ballan Exp $";
#endif

/*
 * list of children reported missing.
 */
static struct gl_list_t *g_missing = NULL;


#if 0 /* reference */
extern int CompareChildLists(ChildListPtr cl1,
                             ChildListPtr cl2,
                             unsigned long *diff)
{
  struct ChildListEntry *cle1, *cle2;
  unsigned long int len1, len2, c, len;
  int ctmp;
  if (cl1==cl2) {
    return 0;
  }
  ctmp = 0;
  assert(cl1!= NULL);
  assert(cl2!= NULL);
  assert(diff != NULL);
  *diff = 0;
  len1=ChildListLen(cl1);
  len2=ChildListLen(cl2);
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
    cle1 = CGET(cl1,c);
    cle2 = CGET(cl2,c);
    if (cle1->strptr != cle2->strptr) {
      ctmp = CmpSymchar(cle1->strptr , cle2->strptr);
      assert(ctmp);
      break;
    }
    if (cle1->typeptr != cle2->typeptr) {
      ctmp = (GetParseId(cle1->typeptr) > GetParseId(cle2->typeptr) ? 1 : -1);
      break;
    }
    if (cle1->bflags != cle2->bflags) {
      ctmp = ((cle1->bflags > cle2->bflags) ? 1 : -1);
      break;
    }
    if (cle1->origin != cle2->origin) {
      ctmp = ((cle1->origin > cle2->origin) ? 1 : -1);
      break;
    }
    if (cle1->isarray != cle2->isarray) {
      ctmp = ((cle1->isarray > cle2->isarray) ? 1 : -1);
      break;
    }
    /* passing bflags, origin, and isarray tests pretty much assures
     * passing statement comparison test, too.
     */
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
#endif

void WriteChildList(FILE *fp,ChildListPtr cl)
{
  unsigned long c,len;
  struct ChildListEntry *cle;
  CONST struct gl_list_t *l;
  if (cl!=NULL) {
    l = GL(cl);
    len = gl_length(l);
    if (!len) {
      FPRINTF(fp,"Child list is empty\n");
      return;
    }
    FPRINTF(fp,"Child list is %lu long.\n",len);
    for (c=1;c<=len;c++) {
      cle = GGET(l,c);
      if (cle!=NULL) {
        FPRINTF(fp,"%lu name=\"%s\" type=\"%s\" ARRAY=%d origin=%d bits=%u\n",
          c,
          SCP(cle->strptr),
          ((cle->typeptr==NULL)?"UNKNOWN":SCP(GetName(cle->typeptr))),
          cle->isarray,
          (int)cle->origin,cle->bflags);
        WSEM(fp,cle->statement,"  Declared at ");
      } else {
        FPRINTF(fp,"Child list item %lu is empty!\n",c);
      }
    }
  }
}

CONST char *WriteChildMetaDetails(void)
{
  static char *metadata[] = {
    "localname-string-{link name without subscripts}",
    "alias-boolean-{is child simply aliasing}",
    "aliasarray-boolean-{is child array defined by ALIASES/IS_A}",
    "isa-boolean-{is child defined by IS_A}",
    "willbe-boolean-{is child defined by WILL_BE}",
    "received-boolean-{is the child originating somehow via parameter list}",
    "visible-boolean-{is child being unfiltered currently}",
    "supported-boolean-{is child a supported attribute}",
    "passed-boolean-{approximately is child passed}",
    "guesstype-string-{approximate type of child}",
    "basetype-string-{base class e.g. MODEL real integer_constant}",
    "arraydepth-integer-{number of subscript to reach an object of guesstype}",
    "fullname-string-{name as defined with subscripts possibly}",
    "module-string-{name of defining module}",
    "line-integer-{line in defining module}",
    "statement-string-{not implemented}"
  };
  static char result[(sizeof(metadata)+1)*80];
  static int done=0;
  
  if (!done) {
    done = 1;
    sprintf(result,
      "{%s} {%s} {%s} {%s} {%s} {%s} {%s} {%s}" /* cpp join */
      " {%s} {%s} {%s} {%s} {%s} {%s} {%s} {%s}",
            metadata[0],
            metadata[1],
            metadata[2],
            metadata[3],
            metadata[4],
            metadata[5],
            metadata[6],
            metadata[7],
            metadata[8],
            metadata[9],
            metadata[10],
            metadata[11],
            metadata[12],
            metadata[13],
            metadata[14],
            metadata[15]);
  }
  return result;
}


static
CONST struct Name *ExtractChildName(CONST struct Statement *stat,
                                    symchar *sym,
                                    CONST struct TypeDescription *desc)
{
  CONST struct Name *name;
  CONST struct VariableList *vl;
  switch (GetBaseType(desc)) {
  case relation_type:
    return RelationStatName(stat);
  case logrel_type:
    return LogicalRelStatName(stat);
  case when_type:
    return WhenStatName(stat);
  default:
    break;
  }
  vl = GetStatVarList(stat);
  if (vl == NULL) {
    return NULL;
  }
  while (vl != NULL) {
    name = NamePointer(vl);
    assert(NameId(name));
    if (sym == NameIdPtr(name)) {
      return name;
    }
    vl = NextVariableNode(vl);
  }
  assert(StatementType(stat) == ARR);
  vl = ArrayStatAvlNames(stat);
  while (vl != NULL) {
    name = NamePointer(vl);
    assert(NameId(name));
    if (sym == NameIdPtr(name)) {
      return name;
    }
    vl = NextVariableNode(vl);
  }
  vl = ArrayStatSetName(stat);
  while (vl != NULL) {
    name = NamePointer(vl);
    assert(NameId(name));
    if (sym == NameIdPtr(name)) {
      return name;
    }
    vl = NextVariableNode(vl);
  }
  return NULL;
}

char *WriteChildDetails(ChildListPtr cl,unsigned long n)
{
  unsigned long len;
  struct ChildListEntry *cle;
  CONST struct gl_list_t *l;
  CONST struct TypeDescription *desc;
  CONST struct Name *name;
  CONST struct Statement *stat;
  char *result = NULL;
  CONST char *tmp;
  Asc_DString ds,*dsPtr;
  int origin;
  symchar *sym;
  char longspace[40]; /* room for a %lu*/
  dsPtr = &ds;

  if (cl==NULL) {
    return NULL;
  }
  l = GL(cl);
  len = gl_length(l);
  if (!len || n > len || !n) {
    return NULL;
  }
  cle = GGET(l,n);
  if (cle!=NULL) {
    Asc_DStringInit(dsPtr);
    /* localname */
    Asc_DStringAppend(dsPtr,"{",1);
    sym = ChildStrPtr(cl,n);
    Asc_DStringAppend(dsPtr,SCP(sym),SCLEN(sym));
    Asc_DStringAppend(dsPtr,"} {",3);
    /* alias */
    if (ChildAliasing(cl,n)) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* aliasarray */
    origin = ChildOrigin(cl,n);
    if (origin == origin_ARR || origin == origin_PARR) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* isa */
    if (origin == origin_ISA || origin == origin_PISA) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* willbe */
    if (origin == origin_WB || origin == origin_PWB) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* received */
    if (ChildParametric(cl,n)) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* visible */
    if (ChildVisible(cl,n)) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* supported */ 
    if (ChildSupported(cl,n)) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* passed */ 
    if (ChildPassed(cl,n)) {
      Asc_DStringAppend(dsPtr,"1} {",4);
    } else {
      Asc_DStringAppend(dsPtr,"0} {",4);
    }
    /* guesstype */ 
    desc = ChildBaseTypePtr(cl,n);
    if (desc == NULL) {
      Asc_DStringAppend(dsPtr,"UNKNOWN",7);
    } else {
      sym = GetName(desc);
      Asc_DStringAppend(dsPtr,SCP(sym),SCLEN(sym));
    }
    Asc_DStringAppend(dsPtr,"} {",3);
    /* basetype */
    sym = GetBaseTypeName(GetBaseType(desc));
    Asc_DStringAppend(dsPtr,SCP(sym),SCLEN(sym));
    Asc_DStringAppend(dsPtr,"} {",3);
    /* arraydepth */
    sprintf(longspace,"%d",ChildIsArray(cl,n));
    Asc_DStringAppend(dsPtr,longspace,-1);
    Asc_DStringAppend(dsPtr,"} {",3);
    stat = ChildStatement(cl,n);
    /* fullname */
    sym = ChildStrPtr(cl,n);
    name = ExtractChildName(stat,sym,desc);
    if (name == NULL) {
      ASC_PANIC("Name %s not found in defining statement",
            SCP(sym));
    }
    WriteName2Str(dsPtr,name);
    Asc_DStringAppend(dsPtr,"} {",3);
    /* module */ 
    tmp = Asc_ModuleBestName(StatementModule(stat));
    Asc_DStringAppend(dsPtr,tmp,-1);
    Asc_DStringAppend(dsPtr,"} {",3);
    /* line */ 
    sprintf(longspace,"%lu",StatementLineNum(stat));
    Asc_DStringAppend(dsPtr,longspace,-1);
    /* statement */
    Asc_DStringAppend(dsPtr,"} {}",4); /* statement to dsPtr not done */
    result = Asc_DStringResult(dsPtr);
  }
  return result;
}

void WriteChildMissing(FILE *fp, char *fcn, symchar *childname)
{
  if (fp==NULL || fcn == NULL || childname == NULL) {
    if (g_missing != NULL) {
      gl_destroy(g_missing);
      g_missing = NULL;
    }
    return;
  }
  if (g_missing == NULL) {
    if (childname != NULL) {
      g_missing = gl_create(5);
    } else {
      return;
    }
  }
  if (g_missing == NULL) {
    return;
  }
  if (gl_ptr_search(g_missing,childname,0)==0) {
    gl_append_ptr(g_missing,(VOIDPTR)childname);
    CONSOLE_DEBUG("Child '%s' not found (requested by %s).",SCP(childname),fcn);
  }
}
