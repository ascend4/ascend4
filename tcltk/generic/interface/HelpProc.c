/*
 *  HelpProc.c
 *  ASCEND IV interface help functions.
 *  by Benjamin Andrew Allan
 *  Created April 29, 1997
 *  Version: $Revision: 1.12 $
 *  Version control file: $RCSfile: HelpProc.c,v $
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

#define ASC_BUILDING_INTERFACE

#include <stdarg.h>
#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/pool.h>
#include "HelpProc.h"

#ifndef lint
static CONST char HelpProcRCS[] = "$Id: HelpProc.c,v 1.12 2003/08/23 18:43:06 ballan Exp $";
#endif

struct HelpData {
  struct HelpData *next; /* if we decide we need a hash table use this */
  CONST char *name;	/* just as defined in the Asc_HelpDefine header */
  CONST char *group;			/* " */
  CONST char *usage;			/* " */
  CONST char *shorth;			/* " */
  CONST char *longh;			/* " */
};

struct HelpGroup {
  CONST char *gname;		/* the name of the group */
  CONST char *explanation;	/* an explanation for the group */
  struct gl_list_t *data;	/* the list of HelpData in this group */
};

static struct gl_list_t *g_helplist = NULL;
/* this list is responsible for all allocated HelpData. */

static struct gl_list_t *g_helpgroups = NULL;
/* this list is responsible for all allocated HelpGroup. */


char *HelpBuildString(char *first,...)
{
  char *argv[MAXHELPARGS];
  int argl[MAXHELPARGS];
  char *str;
  char *result;
  int c,argc,len;
  char *tmp;
  va_list ap;

  va_start(ap,first);
  argv[0] = first;
  argl[0] = strlen(first);
  argc = 1;
  len = argl[0];
  while ( (str = va_arg(ap,char *)) != NULL && argc < MAXHELPARGS) {
    argv[argc] = str;
    argl[argc] = strlen(str);
    if (argl[argc]==0) {
      FPRINTF(ASCERR,"Empty string passed to HelpBuildString.\n%s%s%s",
        "\nPrevious string was:\n",argv[argc-1],"\nExpect core.\n");
      va_end(ap);
      return NULL;
    } else {
      if (argl[argc]>=509) {
        FPRINTF(ASCERR,"String %d long passed to HelpBuildString.\n>>%s\n%s",
        argl[argc], str, "<<This is non-portable\n");
      }
    }
    len += argl[argc];
    argc++;
  }
  va_end(ap);
  len++;
  result = (char *)malloc(len);
  if (result == NULL) {
    return NULL;
  }
  tmp = result;
  for (c = 0; c <argc; c++) {
    if (argv[c]!=NULL) {
      strcpy(tmp,argv[c]);
      tmp += argl[c];
    }
  }
  tmp[0] = '\0';
  return result;
}

int Asc_HelpCheck(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  (void)cdata; /* shut up gcc */
  if (argc >=2 && argv[1][0]=='-') {
    if (argv[1][1]=='h') {
      Tcl_AppendResult(interp,argv[0],":\n",(char *)NULL);
      (void)Asc_HelpGetShort(interp,argv[0]);
      Tcl_AppendResult(interp,"\n",(char *)NULL);
      (void)Asc_HelpGetUsage(interp,argv[0]);
      return TCL_BREAK;
    }
    if (argv[1][1]=='H') {
      Tcl_AppendResult(interp,argv[0],":\n",(char *)NULL);
      (void)Asc_HelpGetLong(interp,argv[0]);
      return TCL_BREAK;
    }
  }
  return TCL_OK;
}


int Asc_HelpInit(void)
{
  assert(g_helplist==NULL);
  assert(g_helpgroups==NULL);
  g_helplist = gl_create(200L);
  g_helpgroups = gl_create(20L);
  return TCL_OK;
}

static
void DestroyHelpGroup(void *g)
{
  gl_destroy(((struct HelpGroup *)g)->data);
}

static
void DestroyHelpData(void *g)
{
  if (((struct HelpData *)g)->longh != NULL) {
    ascfree((void *)(((struct HelpData *)g)->longh));
  }
}

int Asc_HelpDestroy(void)
{
  gl_iterate(g_helpgroups,DestroyHelpGroup);
  gl_free_and_destroy(g_helpgroups);
  gl_iterate(g_helplist,DestroyHelpData);
  gl_free_and_destroy(g_helplist);
  g_helplist = g_helpgroups = NULL;
  return TCL_OK;
}

/* returns the strcmp of the names of two helpdata */
static
int CompareHelpData(struct HelpData *h1,struct HelpData *h2)
{
  if (h1==h2) {
    return 0;
  }
  if (h1==NULL || h1->name == NULL) {
    return 1;
  }
  if (h2==NULL || h2->name == NULL) {
    return -1;
  }
  return strcmp(h1->name,h2->name);
}

/* returns the strcmp of the names of two helpgroup */
static
int CompareHelpGroup(struct HelpGroup *h1,struct HelpGroup *h2)
{
  if (h1==h2) {
    return 0;
  }
  if (h1==NULL || h1->gname == NULL) {
    return 1;
  }
  if (h2==NULL || h2->gname == NULL) {
    return -1;
  }
  return strcmp(h1->gname,h2->gname);
}

/*
 * Return the data corresponding to the command name given, or NULL if no
 * such command is known.
 */
static
struct HelpData *FindHelpData(CONST char *name)
{
  struct HelpData srch;
  unsigned long pos;

  srch.name = name;
  pos = gl_search(g_helplist,&srch,(CmpFunc)CompareHelpData);
  if (pos==0) {
    return NULL;
  } else {
    return((struct HelpData *)gl_fetch(g_helplist,pos));
  }
}

/*
 * Return the data corresponding to the command name given, or NULL if no
 * such command is known.
 */
static
struct HelpGroup *FindHelpGroup(CONST char *group)
{
  struct HelpGroup srch;
  unsigned long pos;

  srch.gname = group;
  pos = gl_search(g_helpgroups,&srch,(CmpFunc)CompareHelpGroup);
  if (pos==0) {
    return NULL;
  } else {
    return((struct HelpGroup *)gl_fetch(g_helpgroups,pos));
  }
}

/* creates a group with the explanation given and
 * inserts it in the global list.
 * Returns NULL if error occurs.
 */
static
struct HelpGroup *CreateHelpGroup(CONST char *group, CONST char *explanation)
{
  struct HelpGroup *g;
  g = (struct HelpGroup *)ascmalloc(sizeof(struct HelpGroup));
  if (g == NULL) {
    return NULL;
  }
  g->gname = group;
  g->explanation = explanation;
  g->data = gl_create(30L);
  if (g->data == NULL) {
    ascfree(g);
    return NULL;
  }
  gl_insert_sorted(g_helpgroups,g,(CmpFunc)CompareHelpGroup);
  return g;
}

/*
 * This sets or resets the group field of the help structure
 * and the group structures that point to the command.
 * Returns tcl style int values.
 */
static
int AssignHelpGroup(struct HelpData *d, CONST char *group)
{
  struct HelpGroup *g;
  unsigned long pos;

  if (d->group != NULL) {
    /* reassigning, which means we may need to get it out of the
     * prior group.
     */
    if (strcmp(group,d->group) != 0) {
      g = FindHelpGroup(d->group);
      assert(g!=NULL);
      pos = gl_search(g->data,d,(CmpFunc)CompareHelpData);
      assert(pos!=0);
      gl_delete(g->data,pos,0);
    }
  }
  d->group = group;
  g = FindHelpGroup(group);
  if (g == NULL) {
    g = CreateHelpGroup(group,"Explanation: none given yet.");
    if (g==NULL) {
      return TCL_ERROR;
    }
  }
  gl_insert_sorted(g->data,d,(CmpFunc)CompareHelpData);
  return TCL_OK;
}

/* Inserts help information into the list, or if the same
 * command already exists, replaces that information with
 * the new information.
 * Returns tcl return codes.
 * Currently only possible error is insufficient memory.
 */
static
int AddHelpData(CONST char *name,
                CONST char *group,
                CONST char *usage,
                CONST char *shorth,
                CONST char *longh)
{
  struct HelpData *d;

  d = FindHelpData(name);
  if (d == NULL) {
    d = (struct HelpData *)ascmalloc(sizeof(struct HelpData));
    if (d==NULL) {
      return TCL_ERROR;
    }
    d->group = NULL;
  }
  d->next = NULL;
  d->name = name;
  d->usage = usage;
  d->shorth = shorth;
  d->longh = longh;
  AssignHelpGroup(d,group);
  gl_insert_sorted(g_helplist,d,(CmpFunc)CompareHelpData);
  return TCL_OK;
}

int Asc_HelpDefineGroup(CONST char *group, CONST char *explanation)
{
  struct HelpGroup *g;
  if (explanation==NULL ||
      strlen(explanation)<11 ||
      strncmp(explanation,"Explanation",11)!=0) {
    return TCL_ERROR;
  }
  g = FindHelpGroup(group);
  if (g == NULL) {
    g = CreateHelpGroup(group,explanation);
    if (g==NULL) {
      return TCL_ERROR;
    }
    return TCL_OK;
  } else {
    g->explanation = explanation;
  }
  return TCL_OK;
}


int Asc_HelpDefine(CONST char *name,
                   CONST char *group,
                   CONST char *usage,
                   CONST char *shorth,
                   HLFunc bigstring)
{
  assert(g_helplist!=NULL);
  assert(g_helpgroups!=NULL);
  if (bigstring != NULL) {
    return AddHelpData(name,group,usage,shorth,bigstring());
  } else {
    return AddHelpData(name,group,usage,shorth,NULL);
  }
}

CONST char *Asc_HelpGetShort(Tcl_Interp *interp, CONST84 char *name)
{
  struct HelpData *d;
  d = FindHelpData(name);
  if (d==NULL) {
    return NULL;
  } else {
    Tcl_AppendResult(interp,d->shorth,(char *)NULL);
    return d->shorth;
  }
}

CONST char *Asc_HelpGetLong(Tcl_Interp *interp, CONST84 char *name)
{
  struct HelpData *d;
  d = FindHelpData(name);
  if (d==NULL) {
    return NULL;
  } else {
    Tcl_AppendResult(interp,d->longh,(char *)NULL);
    return d->longh;
  }
}

CONST char *Asc_HelpGetUsage(Tcl_Interp *interp, CONST84 char *name)
{
  struct HelpData *d;
  d = FindHelpData(name);
  if (d==NULL) {
    return NULL;
  } else {
    Tcl_AppendResult(interp,d->usage,(char *)NULL);
    return d->usage;
  }
}

static
void AppendHelpElements(Tcl_Interp *interp,struct gl_list_t *dlist)
{
  unsigned long c,len;
  struct HelpData *d;

  if (interp==NULL || dlist==NULL){
    return;
  }
  len = gl_length(dlist);
  for (c=1;c <=len; c++) {
    d = (struct HelpData *)gl_fetch(dlist,c);
    Tcl_AppendElement(interp,(char *)d->name);
  }
  return;
}

static
void AppendGroupElements(Tcl_Interp *interp, struct HelpGroup *g)
{
  assert(g!=NULL);
  AppendHelpElements(interp,g->data);
}

int Asc_HelpGetGroup(Tcl_Interp *interp, CONST84 char *gname)
{
  struct HelpGroup *g;

  g = FindHelpGroup(gname);
  if (g==NULL) {
    Tcl_SetResult(interp, "{Help group undefined}", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g->explanation !=NULL) {
    Tcl_AppendElement(interp,(char *)g->explanation);
  }
  AppendGroupElements(interp,g);
  return TCL_OK;
}


int Asc_HelpCommandGroups(Tcl_Interp *interp)
{
  unsigned long c,len;
  struct HelpGroup *g;

  if (interp==NULL || g_helpgroups==NULL){
    return TCL_ERROR;
  }
  len = gl_length(g_helpgroups);
  for (c=1;c <=len; c++) {
    g = (struct HelpGroup *)gl_fetch(g_helpgroups,c);
    Tcl_AppendElement(interp,(char *)g->gname);
  }
  return TCL_OK;
}

int Asc_HelpCommandList(Tcl_Interp *interp)
{
  if (interp==NULL || g_helplist==NULL){
    return TCL_ERROR;
  }
  AppendHelpElements(interp,g_helplist);
  return TCL_OK;
}

int Asc_HelpCommandsByGroups(Tcl_Interp *interp)
{
  unsigned long c,len;
  struct HelpGroup *g;

  if (g_helpgroups==NULL) {
    return TCL_ERROR;
  }
  len = gl_length(g_helpgroups);
  for (c=1;c <=len; c++) {
    g = (struct HelpGroup *)gl_fetch(g_helpgroups,c);
    Tcl_AppendResult(interp," {GROUP ",g->gname,":} ",(char *)NULL);
    AppendGroupElements(interp,g);
  }
  return TCL_OK;
}

STDHLF(Asc_HelpCmd,(Asc_HelpCmdHL1,Asc_HelpCmdHL2,Asc_HelpCmdHL3,
 Asc_HelpCmdHL4,HLFSTOP));

#define ADDHELPSTR(s) Tcl_AppendResult(interp,s,(char *)NULL)
/* appends s to the tcl result */

int Asc_HelpCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
  CONST char *um, *sm, *lm;
  UNUSED_PARAMETER(cdata);

  ASCUSE;

  if (argc==1) {
    Asc_HelpGetLong(interp,argv[0]);
    return TCL_OK;
  }
  /* isn't -H*  or -h*  */
  if (argc==2) {
    if (Asc_HelpGetGroup(interp,argv[1]) == TCL_OK) {
      return TCL_OK;
    }
    /* isn't a group */
    Tcl_ResetResult(interp);
    ADDHELPSTR("Usage: ");
    um =  Asc_HelpGetUsage(interp,argv[1]);
    ADDHELPSTR("\nSummary: ");
    sm = Asc_HelpGetShort(interp,argv[1]);
    ADDHELPSTR("\nDetails:\n");
    lm = Asc_HelpGetLong(interp,argv[1]);
    if (um == NULL && sm == NULL && lm == NULL) {
      Tcl_ResetResult(interp);
      /* isn't a regular command, could be a help subcommand. */
      if (strcmp(argv[1],"all") == 0) {
        Asc_HelpCommandList(interp);
        return TCL_OK;
      }
      if (strcmp(argv[1],"commands") == 0) {
        Asc_HelpCommandsByGroups(interp);
        return TCL_OK;
      }
      if (strcmp(argv[1],"groups") == 0) {
        Asc_HelpCommandGroups(interp);
        return TCL_OK;
      }
      ADDHELPSTR("Unknown or undocumented command: ");
      ADDHELPSTR(argv[1]);
      return TCL_ERROR;
    } else {
      return TCL_OK;
    }
  }
  /* may want more here later. */
  Tcl_SetResult(interp, "Too many arguments to help. Try help -h", TCL_STATIC);
  return TCL_ERROR;
}

