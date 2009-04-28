/*
 *  base/generic/compiler/qlfdid.c created from tcltk/generic/interface/Qlfdid.c
 *  Created: 1/94
 *  Version: $Revision: 1.22 $
 *  Version control file: $RCSfile: Qlfdid.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of ASCEND.
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  ASCEND is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  ASCEND is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#include "qlfdid.h"

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include "instance_enum.h"
#include "symtab.h"
#include "simlist.h"
#include "instance_io.h"
#include "instance_name.h"
#include "instquery.h"
#include "parentchild.h"


#include "expr_types.h"
#include "stattypes.h"
#include "instantiate.h"

#ifndef MAXIMUM_STRING_LENGTH
#define MAXIMUM_STRING_LENGTH 1024
#endif
#define QLFDIDMALLOC ASC_NEW(struct SearchEntry);


/* used for searching */

struct Instance *g_search_inst = NULL;
struct Instance *g_relative_inst = NULL;

char *Asc_MakeInitString(int len)
{
  char *result;
  int defaultlen = 40;
  if (len<=0) {
    FPRINTF(stderr,
            "Setting length to %d due to invalid length given\n",defaultlen);
    len = defaultlen;
  }
  result = ASC_NEW_ARRAY(char,(len + 1)*sizeof(char));
  assert(result!=NULL);
  sprintf(result,"%s","\0");
  return result;
}

void Asc_ReInitString(char *str)
{
  if ((str)&&(strlen(str))) {
    strcpy(str,"");
  }
}

/*
 * Create a search entry node with a simple name
 * such as : a.
 */
struct SearchEntry *Asc_SearchEntryCreate(char *name,struct Instance *i)
{
  struct SearchEntry *result;
  result = QLFDIDMALLOC;
  assert(result!=NULL);
  result->name = ASC_NEW_ARRAY(char,strlen(name)+1);
  strcpy(result->name,name);
  result->i = i;
  return result;
}

/*
 * Create a search entry node with a name that
 * is formatted like a integer array. such as:
 * [14].
 */
static
struct SearchEntry *SearchEntryCreateIntArray(char *name,struct Instance *i)
{
  struct SearchEntry *result;
  result = QLFDIDMALLOC;
  assert(result!=NULL);
  result->name = ASC_NEW_ARRAY(char,strlen(name)+8);
  sprintf(result->name,"[%s]",name);
  result->i = i;
  return result;
}

/*
 * Create a search entry node with a name that
 * is formatted like a string array. such as:
 * ['benzene.flow'].
 */
static
struct SearchEntry *SearchEntryCreateStrArray(char *name,struct Instance *i)
{
  struct SearchEntry *result;
  result = QLFDIDMALLOC;
  assert(result!=NULL);
  result->name = ASC_NEW_ARRAY(char,strlen(name)+8);
  sprintf(result->name,"[\'%s\']",name);
  result->i = i;
  return result;
}

struct Instance *Asc_SearchEntryInstance(struct SearchEntry *se)
{
  assert(se!=NULL);
  return (se->i);
}

char *Asc_SearchEntryName(struct SearchEntry *se)
{
  assert(se!=NULL);
  return (se->name);
}

void Asc_SearchEntryDestroy(struct SearchEntry *se)
{
  if (!se) {
    return;
  }
  ascfree(se->name);
  se->name = NULL;
  se->i = NULL;
  ascfree(se);
}

void Asc_SearchListDestroy(struct gl_list_t *search_list)
{
  struct SearchEntry *se;
  unsigned long len,c;
  if (!search_list) {
    return;
  }
  len = gl_length(search_list);
  for(c=1;c<=len;c++) {
    se = (struct SearchEntry *)gl_fetch(search_list,c);
    Asc_SearchEntryDestroy(se);
  }
  gl_destroy(search_list);
}

static
int CheckChildExist(struct InstanceName name)
{
  unsigned long ndx,nch;
  symchar  *tablename; /* hacky, but centralized slop avoidance */
  /* remember that a struct passed by value can be overwritten safely. */
  nch = NumberChildren(g_search_inst);
  if (!nch) {
    g_search_inst = NULL;
    return 0;
  }
  switch (InstanceNameType(name)) {
  case IntArrayIndex:
    break;
  case StrName:
    tablename = InstanceNameStr(name);
    SetInstanceNameStrPtr(name,tablename);
    break;
  case StrArrayIndex:
    tablename = InstanceStrIndex(name);
    SetInstanceNameStrIndex(name,tablename);
    break;
  default:
    Asc_Panic(2,"%s: CheckChildExist called with bad arguments.",__FILE__);
    break;
  }
  ndx = ChildSearch(g_search_inst,&name); /* symchar safe */
  if (ndx) {
    g_search_inst = InstanceChild(g_search_inst,ndx);
    return ndx;
  } else {
    g_search_inst = NULL;
    return 0;
  }
}

static void HandleLastPart(char *temp)
{
  struct InstanceName name;
  if (g_search_inst) {
    SetInstanceNameType(name,StrName);
    SetInstanceNameStrPtr(name,AddSymbol(temp));
    CheckChildExist(name);            /* sets g_search_inst regardless */
    return;
  } else {
    g_search_inst = Asc_FindSimulationRoot(AddSymbol(temp));
  }
}

struct gl_list_t *Asc_BrowQlfdidSearch(char *str, char *temp)
{
  register char *ptr, *org;
  struct InstanceName name;
  struct gl_list_t *search_list = NULL;
  struct SearchEntry *se;
  int ndx = 0;
  int open_bracket = 0;
  int open_quote = 0;

  g_search_inst = NULL;      /* always start searches from the top */
  if (!str) {
    return NULL;
  }
  search_list = gl_create(8L);
  ptr = temp;
  org = str;
  while(*str != '\0') {
    switch(*str) {
    case '.':
      if (*(str-1) != ']') {
        if (open_quote) {	/* to deal b['funny.name']  */
          *(ptr++) = *(str++);	/*              ---^---	    */
          break;
        }
        *ptr = '\0';
        if (g_search_inst) {
          SetInstanceNameType(name,StrName);
          SetInstanceNameStrPtr(name,AddSymbol(temp));
          if(0 == (ndx=CheckChildExist(name))) {
            Asc_SearchListDestroy(search_list);
            return NULL;
          }
        } else {
          g_search_inst = Asc_FindSimulationRoot(AddSymbol(temp));
          if (!g_search_inst) {
            Asc_SearchListDestroy(search_list);
            return NULL;
          }
        }
        se = Asc_SearchEntryCreate(temp,g_search_inst);
        gl_append_ptr(search_list,se);
      }
      str++;
      ptr = temp;
      break;
    case '\'':
      str++;
      if (open_quote) {
        open_quote--;
      } else {
        open_quote++;
      }
      break;
    case '[':
      if (*(str-1) != ']') {
        *ptr = '\0';
        if (g_search_inst) {
          SetInstanceNameType(name,StrName);
          SetInstanceNameStrPtr(name,AddSymbol(temp));
          if(0 == (ndx=CheckChildExist(name))) {
            Asc_SearchListDestroy(search_list);
            return NULL;
          }
        } else {
          g_search_inst = Asc_FindSimulationRoot(AddSymbol(temp));
          if (!g_search_inst) {
            Asc_SearchListDestroy(search_list);
            return NULL;
          }
        }
        se = Asc_SearchEntryCreate(temp,g_search_inst);
        gl_append_ptr(search_list,se);
      }
      ptr = temp;
      open_bracket++;
      str++;
      break;
    case ']':
      open_bracket--;
      *ptr = '\0';
      str++;
      switch(InstanceKind(g_search_inst)) {
      case ARRAY_INT_INST:
        SetInstanceNameType(name,IntArrayIndex);
        SetInstanceNameIntIndex(name,atol(temp));
        if(0 == (ndx=CheckChildExist(name))) {
          Asc_SearchListDestroy(search_list);
          return NULL;
        }
        ptr = temp;
        se = SearchEntryCreateIntArray(temp,g_search_inst);
        gl_append_ptr(search_list,se);
        break;
      case ARRAY_ENUM_INST:
        SetInstanceNameType(name,StrArrayIndex);
        SetInstanceNameStrIndex(name,AddSymbol(temp));
        if(0 == (ndx=CheckChildExist(name))) {  /* sets g_search_inst */
          Asc_SearchListDestroy(search_list);
          return NULL;
        }
        ptr = temp;
        se = SearchEntryCreateStrArray(temp,g_search_inst);
        gl_append_ptr(search_list,se);
        break;
      default:
        FPRINTF(stderr,"Mismatch in qlfdid (%s) and simulation.\n",org);
        break;
      }
      break;
    default:
      *(ptr++) = *(str++);
      break;
    }
  }
  *ptr = '\0';
  if (*temp == '\0') {
    return search_list;
  }
  HandleLastPart(temp);
  if (g_search_inst) {
    se = Asc_SearchEntryCreate(temp,g_search_inst);
    gl_append_ptr(search_list,se);
    return search_list;
  } else {
    Asc_SearchListDestroy(search_list);
    return NULL;
  }
}

int Asc_QlfdidSearch2(char *str)
{
  char temp[MAXIMUM_ID_LENGTH];
  struct gl_list_t *search_list;

  search_list = Asc_BrowQlfdidSearch(str,temp);
  if ((g_search_inst==NULL) || (search_list==NULL)) {
    return 1;
  } else {
    Asc_SearchListDestroy(search_list);
    return 0;
  }
}

/*
 *********************************************************************
 * This is the version of the function that should be used purely
 * for the effect of leaving the g_search_inst looking at the result
 * of a qualified id search.
 *********************************************************************
 */

static
void HandleLastPart3(char *temp)
{
  struct InstanceName name;
  if (g_search_inst) {
    SetInstanceNameType(name,StrName);
    SetInstanceNameStrPtr(name,AddSymbol(temp));
    CheckChildExist(name);            /* sets g_search_inst regardless */
    return;
  } else {
    g_search_inst = Asc_FindSimulationRoot(AddSymbol(temp));
  }
}

static
struct Instance *BrowQlfdidSearch3(CONST char *str, char *temp,int relative)
{
  register char *ptr;
  struct InstanceName name;
  int ndx = 0;
  int open_bracket = 0;
  int open_quote = 0;
  CONST char *org;

  if (relative == 1) {
    g_search_inst = g_relative_inst; /* could be NULL */
  } else {
    g_search_inst = NULL;      /* start search from the top */
  }
  if (str == NULL) {
    return NULL;
  }
  org = str;
  ptr = temp;
  while(*str != '\0') {
    switch(*str) {
    case '.':
      if (*(str-1) != ']') {
        if (open_quote) {	/* to deal b['funny.name']  */
          *(ptr++) = *(str++);	/*              ---^---	    */
          break;
        }
        *ptr = '\0';
        if (g_search_inst) {
          SetInstanceNameType(name,StrName);
          SetInstanceNameStrPtr(name,AddSymbol(temp));
          ndx=CheckChildExist(name);
          if(ndx==0) {
            return NULL;
          }
        } else {
          g_search_inst = Asc_FindSimulationRoot(AddSymbol(temp));
          if (!g_search_inst) {
            return NULL;
          }
        }
      }
      str++;
      ptr = temp;
      break;
    case '\'':
      str++;
      if (open_quote) {
        open_quote--;
      } else {
        open_quote++;
      }
      break;
    case '[':
      if (*(str-1) != ']') {
        *ptr = '\0';
        if (g_search_inst) {
          SetInstanceNameType(name,StrName);
          SetInstanceNameStrPtr(name,AddSymbol(temp));
          ndx=CheckChildExist(name);
          if(ndx==0) {
            return NULL;
          }
        } else {
          g_search_inst = Asc_FindSimulationRoot(AddSymbol(temp));
          if (!g_search_inst) {
            return NULL;
          }
        }
      }
      ptr = temp;
      open_bracket++;
      str++;
      break;
    case ']':
      open_bracket--;
      *ptr = '\0';
      str++;
      switch(InstanceKind(g_search_inst)) {
      case ARRAY_INT_INST:
        SetInstanceNameType(name,IntArrayIndex);
        SetInstanceNameIntIndex(name,atol(temp));
        ndx=CheckChildExist(name);
        if(ndx==0) {
          return NULL;
        }
        ptr = temp;
        break;
      case ARRAY_ENUM_INST:
        SetInstanceNameType(name,StrArrayIndex);
        SetInstanceNameStrIndex(name,AddSymbol(temp));
        ndx=CheckChildExist(name);
        if(ndx==0) {  /* sets g_search_inst */
          return NULL;
        }
        ptr = temp;
        break;
      default:
        FPRINTF(stderr,"Mismatch in values file (%s) and simulation.\n",org);
        break;
      }
      break;
    default:
      *(ptr++) = *(str++);
      break;
    }
  }
  *ptr = '\0';
  if (*temp == '\0') {
    return g_search_inst;
  }
  HandleLastPart3(temp);
  return g_search_inst; /* which may be NULL */
}

int Asc_QlfdidSearch3(CONST char *str, int relative)
{
  char *temp;
  struct Instance *found;

  if (str==NULL) {
    return 1;
  }
  temp = ASC_STRDUP((char *)str);
  if (temp==NULL) {
    return 1;
  }
  found = BrowQlfdidSearch3(str,temp,relative);
  ascfree(temp);
  if (found != NULL) {
    return 0;
  } else {
    return 1;
  }
}








