/*
 *  ProbeProc.c
 *  by Ben Allan
 *  Created: 6/97
 *  Version: $Revision: 1.38 $
 *  Version control file: $RCSfile: ProbeProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
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

#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include <compiler/symtab.h>
#include <compiler/instance_enum.h>


#include <compiler/instquery.h>
#include <compiler/visitinst.h>
#include <compiler/instance_name.h>
#include <compiler/instance_io.h>
#include "HelpProc.h"
#include "Qlfdid.h"
#include "BrowserQuery.h"
#include "ProbeProc.h"
#include "UnitsProc.h"
#include "ProbeProc.h"

#ifndef lint
static CONST char ProbeProcID[] = "$Id: ProbeProc.c,v 1.38 2003/08/23 18:43:07 ballan Exp $";
#endif


/*
 * the number of arguments required to specify a complete filter.
 */
#define NUMFILTERS 16

#define ENTRYMALLOC \
(struct ProbeEntry *)ascmalloc((unsigned)sizeof(struct ProbeEntry))

/*
 * In the 0->len-1 indexing, returns the list in position n.
 * returns NULL for n out of range.
 */
#define ProbeArray(n) GetProbeList(n)
/*
 * return ProbeArray size, or 0 if not initialized.
 */
#define ProbeArraySize ((g_probe_array==NULL) ? 0 : g_probe_array_size)
/*
 * return the attributes of a probe entry pointer
 */
#define ProbeEntryName(e) (e)->name
#define ProbeEntryInst(e) (e)->i
/*
 * Fetch an entry pointer, assumes valid num and ind.
 */
#define ProbeGetEntry(num,ind) \
  (struct ProbeEntry *)gl_fetch(ProbeArray(num),(unsigned long)((ind)+1))


struct ProbeEntry {
  char *name;
  /* This is the authoritative data. */
  struct Instance *i;
  /* This may be NULL, requiring lookup or deletion per user */
};

struct ProbeFilterFlags {
  unsigned VisitRelations;
  unsigned VisitLogRelations;
  unsigned VisitBooleans;
  unsigned VisitIntegers;
  unsigned VisitReals;
  unsigned VisitSymbols;
  unsigned VisitSets;
  unsigned VisitSABooleans;
  unsigned VisitSAIntegers;
  unsigned VisitSAReals;
  unsigned VisitSASymbols;
  unsigned VisitSASets;
  unsigned VisitBooleanConstants;
  unsigned VisitIntegerConstants;
  unsigned VisitRealConstants;
  unsigned VisitSymbolConstants;
};

/*
 * An array of lists we use to track collections. Grown as needed.
 * not a list because it is too hard to replace list elements.
 * All elements of this array should always contain
 * a list, though the list may be empty or change identity.
 * Eventually, should become an array of userdata, perhaps.
 */
static struct gl_list_t **g_probe_array = NULL;
/*
 * The size of this array. valid entries 0..size-1.
 */
static unsigned g_probe_array_size = 0;

/*
 * The instance root while creating names within
 * a Visit collection of instances.
 */
static struct Instance *g_visit_root = NULL;

/* The current context for visit consumption.
 */
static struct gl_list_t *g_cur_context = NULL;

/* The root name for visit consumption.
 */
static char *g_visit_root_name = NULL;

/* the length of the root name, without ending NULL.
 */
static int g_visit_root_name_len = 0;

/*
 * A struct for visit functions to use in selecting object to
 * add to the collection.
 */
static struct ProbeFilterFlags g_probe_filter;

/*
 * Returns, with sanity check, the nth element of the collection array.
 */
static
struct gl_list_t *GetProbeList(unsigned int n)
{
  if (g_probe_array == NULL || n >= g_probe_array_size) {
    return NULL;
  }
  return g_probe_array[n];
}

static
void InitProbeFilter(struct ProbeFilterFlags *f)
{
  f->VisitRelations =
  f->VisitLogRelations =
  f->VisitBooleans =
  f->VisitIntegers =
  f->VisitReals =
  f->VisitSymbols =
  f->VisitSets =
  f->VisitSABooleans =
  f->VisitSAIntegers =
  f->VisitSAReals =
  f->VisitSASymbols =
  f->VisitSASets =
  f->VisitBooleanConstants =
  f->VisitIntegerConstants =
  f->VisitRealConstants =
  f->VisitSymbolConstants = 0;
}

/*
 * This function is the source of the positional dependence of
 * the filter arguments.
 * low is the index in argv of first flag. argc is the
 */
static
void SetupProbeFilter(struct ProbeFilterFlags *f,
                      char **argv,int low,int argc)
{
  int c;

  InitProbeFilter(f);
  c = low;

#define SPFGetArg(flag) \
  if (c<argc && argv[c][0]=='1') { f->flag = 1; } c++

  SPFGetArg(VisitRelations);
  SPFGetArg(VisitLogRelations);
  SPFGetArg(VisitBooleans);
  SPFGetArg(VisitIntegers);
  SPFGetArg(VisitReals);
  SPFGetArg(VisitSymbols);
  SPFGetArg(VisitSets);
  SPFGetArg(VisitSABooleans);
  SPFGetArg(VisitSAIntegers);
  SPFGetArg(VisitSAReals);
  SPFGetArg(VisitSASymbols);
  SPFGetArg(VisitSASets);
  SPFGetArg(VisitBooleanConstants);
  SPFGetArg(VisitIntegerConstants);
  SPFGetArg(VisitRealConstants);
  SPFGetArg(VisitSymbolConstants);
#undef SPFGetArg
}


/*
 * mallocs an entry, assigning name and inst. i may be NULL,
 * but if name is NULL, returns NULL.
 * The entry owns the name string given.
 * i may change over the course of the entry's existence.
 * We may want to make a pool for these if malloc is too slow.
 */
static
struct ProbeEntry *ProbeEntryCreate(char *name, struct Instance *i)
{
  struct ProbeEntry *result;

  result = ENTRYMALLOC;
  if (result==NULL || name == NULL) {
    /* should ascpanic here */
    return NULL;
  }
  result->name = name;
  result->i = i;
  return result;
}

/*
 * Frees the memory associated with the entry.
 */
static
void ProbeEntryDestroy(struct ProbeEntry *e)
{
  if(e!=NULL) {
    if (e->name != NULL)  {
      ascfree(e->name);
    } /* very odd if this if fails! */
    e->name = NULL;
    e->i = NULL;
    ascfree(e);
  }
}

/*
 * Sets up the global array, and/or expands it by 1 empty list.
 * returns 0 if ok or 1 if insufficient memory.
 * Grows only by one.
 */
static
int Asc_ProbeArrayGrow()
{
  struct gl_list_t **tmp;
  if (g_probe_array==NULL) {
    g_probe_array =
      (struct gl_list_t **)ascmalloc(sizeof(struct gl_list_t *));
    if (g_probe_array==NULL) {
      g_probe_array_size = 0;
      return 1;
    }
    g_probe_array[0] = gl_create(100);
    if (g_probe_array[0]==NULL) {
      g_probe_array_size = 0;
      ascfree(g_probe_array);
      g_probe_array = NULL;
      return 1;
    }
    g_probe_array_size = 1;
  } else {
    tmp = (struct gl_list_t **)ascrealloc(g_probe_array,
      sizeof(struct gl_list_t *)*(1+g_probe_array_size));
    if (tmp == NULL) {
      return 1;
    }
    g_probe_array = tmp;
    g_probe_array[g_probe_array_size] = gl_create(100);
    if (g_probe_array[g_probe_array_size]==NULL) {
      return 1;
    }
    g_probe_array_size++;
  }
  return 0;
}

static
void ProbeDeleteAll(struct gl_list_t *p)
{
  struct ProbeEntry *e;
  unsigned long len,c;

  if (p==NULL) {
    return;
  }
  len = gl_length(p);
  for (c=1;c<=len;c++) {
    e = (struct ProbeEntry *)gl_fetch(p,c);
    ProbeEntryDestroy(e); /* should free e->name, NULL i+name, and free e */
  }
  gl_destroy(p);
}

/*
 * Frees all memory associated with the probe. Should
 * be called before shutting down the system.
 */
static
void Asc_ProbeArrayDestroy(void)
{
  unsigned int c;
  for (c = 0; c < ProbeArraySize; c++) {
    ProbeDeleteAll(ProbeArray(c));
    g_probe_array[c] = NULL;
  }
  ascfree(g_probe_array);
  g_probe_array = NULL;
}

static
void ProbeUpdateEntries(struct gl_list_t *p)
{
  unsigned long c,len;
  int err;
  struct ProbeEntry *e;
  if (p != NULL) {
    len = gl_length(p);
    for (c=1;c <= len; c++) {
      e = (struct ProbeEntry *)gl_fetch(p,c);
      if (ProbeEntryInst(e)==NULL) {
        err = Asc_QlfdidSearch3(ProbeEntryName(e),0);
        if (err == 0) {
          ProbeEntryInst(e) = g_search_inst;
        }
      }
    }
  }
}

static
void ProbeNullInstances(struct gl_list_t *p)
{
  unsigned long c,len;
  if (p != NULL) {
    len = gl_length(p);
    for (c=1;c <= len; c++) {
      ((struct ProbeEntry *)gl_fetch(p,c))->i = NULL;
    }
  }
}

static
void NullAllInstancePointers(void)
{
  unsigned c,len;
  if (g_probe_array != NULL) {
    len = g_probe_array_size;
    for (c=0;c < len; c++) {
      ProbeNullInstances(ProbeArray(c));
    }
  }
}

/*
 * returns -1 if context is invalid.
 */
static
int ProbeNumEntries(unsigned int context)
{
  struct gl_list_t *p;
  if ( context >= g_probe_array_size) {
    return -1;
  }
  p = ProbeArray(context);

  return ((p!=NULL)? (int) gl_length(p) : -1);
}

static
void ProbeVisitAll_Filtered(struct Instance *i)
{
  struct ProbeEntry *e1;
  char *wholename, *nametail;
  int add=0;

#define PVAFAdd(flag) add = (g_probe_filter.flag) ? 1 : 0
  if (i!=NULL) {
    switch(InstanceKind(i)) {
    case BOOLEAN_INST:
      PVAFAdd(VisitSABooleans);
      break;
    case BOOLEAN_ATOM_INST:
      PVAFAdd(VisitBooleans);
      break;
    case BOOLEAN_CONSTANT_INST:
      PVAFAdd(VisitBooleanConstants);
      break;
    case INTEGER_INST:
      PVAFAdd(VisitSAIntegers);
      break;
    case INTEGER_ATOM_INST:
      PVAFAdd(VisitIntegers);
      break;
    case INTEGER_CONSTANT_INST:
      PVAFAdd(VisitIntegerConstants);
      break;
    case REAL_INST:
      PVAFAdd(VisitSAReals);
      break;
    case REAL_ATOM_INST:
      PVAFAdd(VisitReals);
      break;
    case REAL_CONSTANT_INST:
      PVAFAdd(VisitRealConstants);
      break;
    case REL_INST:
      PVAFAdd(VisitRelations);
      break;
    case LREL_INST:
      PVAFAdd(VisitLogRelations);
      break;
    case SYMBOL_INST:
      PVAFAdd(VisitSASymbols);
      break;
    case SYMBOL_ATOM_INST:
      PVAFAdd(VisitSymbols);
      break;
    case SYMBOL_CONSTANT_INST:
      PVAFAdd(VisitSymbolConstants);
      break;
    case SET_INST:
      PVAFAdd(VisitSASets);
      break;
    case SET_ATOM_INST:
      PVAFAdd(VisitSets);
      break;
    default:
      add = 0;
      break;
    }
    if (add) {
      nametail = WriteInstanceNameString(i,g_visit_root);
      if (nametail==NULL) { /* out of memory */
        return;
      }
      wholename = (char *)ascmalloc(g_visit_root_name_len +
                                    strlen(nametail) + 3);
      if (wholename ==NULL) {
        ascfree(nametail);
        return;
      }
      if (IsArrayInstance(g_visit_root)) {
        sprintf(wholename,"%s%s",g_visit_root_name,nametail);
      } else {
        sprintf(wholename,"%s.%s",g_visit_root_name,nametail);
      }
      ascfree(nametail);
      e1 = ProbeEntryCreate(wholename,i);
      if (e1==NULL)  {
        ascfree(wholename);
      }
      gl_append_ptr(g_cur_context,e1);
    }
  }
}

/*
 * Adds the value of the entry passed to it to the interp as an
 * element, or at least the name if the instance pointer is NULL.
 * At preseent always returns 0 because it ignores the returns of
 * functions it calls.
 */
static
int AppendEntryItem(Tcl_Interp *interp,struct ProbeEntry *e)
{
  char *name;
  char *ustr, tmp[1024]; /* not safe for long symbol values at all */
  struct Instance *i;

  if (e==NULL) {
    Tcl_AppendElement(interp,"UNDEFINED probe entry");
    return 0;
  }

  i = ProbeEntryInst(e);
  name = ProbeEntryName(e);

  Tcl_AppendResult(interp,"{",(char *)NULL);	/* start element */
  Tcl_AppendResult(interp,name,(char *)NULL);	/* append name */
  if (i==NULL) {
    Tcl_AppendResult(interp," = UNCERTAIN} ",(char *)NULL);
    return 0;
  }
  switch(InstanceKind(i)) {
  case REL_INST:
  case REAL_INST:
  case REAL_ATOM_INST:
  case REAL_CONSTANT_INST:
    ustr = Asc_UnitValue(i);
    if (ustr!=NULL) {
      Tcl_AppendResult(interp," = ",ustr,(char *)NULL);
    } else {
      Tcl_AppendResult(interp," = ","????",(char *)NULL);
    }
    Tcl_AppendResult(interp,"} ",(char *)NULL);	/* end REAL */
    break;
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_INST:
  case BOOLEAN_CONSTANT_INST:
  case BOOLEAN_ATOM_INST:
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
  case LREL_INST:
    tmp[0] = '\0';
    Tcl_AppendResult(interp,tmp," = ",(char *)NULL);
    (void)Asc_BrowWriteAtomValue(tmp,i);
    Tcl_AppendResult(interp,tmp,"} ",(char *)NULL); /* end OTHERS */
    break;
  default:
    Tcl_AppendResult(interp,"} ",(char *)NULL);	/* end default */
    break;
  }
  return 0;
}

static
void ProbeAppendAll(Tcl_Interp *interp, struct gl_list_t *p)
{
  unsigned long c,len;
  struct ProbeEntry *e;
  if (p != NULL) {
    len = gl_length(p);
    for (c=1;c <= len; c++) {
      e = (struct ProbeEntry *)gl_fetch(p,c);
      AppendEntryItem(interp,e);
    }
  }
}

/*
 * This function is alleged to explain the filter flags. it needs
 * improvement.
 */
static
void DefineProbeFilters(Tcl_Interp *interp)
{
  Tcl_AppendElement(interp,"VisitRelations/Collect relations");
  Tcl_AppendElement(interp,"VisitLogRelations/Collect logical relations");
  Tcl_AppendElement(interp,"VisitBooleans/Collect booleans");
  Tcl_AppendElement(interp,"VisitIntegers/Collect integers");
  Tcl_AppendElement(interp,"VisitReals/Collect reals");
  Tcl_AppendElement(interp,"VisitSymbols/Collect symbols");
  Tcl_AppendElement(interp,"VisitSets/Collect sets");
  Tcl_AppendElement(interp,"VisitSABooleans/Collect subatomic booleans");
  Tcl_AppendElement(interp,"VisitSAIntegers/Collect subatomic integers");
  Tcl_AppendElement(interp,"VisitSAReals/Collect subatomic reals");
  Tcl_AppendElement(interp,"VisitSASymbols/Collect subatomic symbols");
  Tcl_AppendElement(interp,"VisitSASets/Collect subatomic sets");
  Tcl_AppendElement(interp,"VisitBooleanConstants/Collect boolean constants");
  Tcl_AppendElement(interp,"VisitIntegerConstants/Collect integer constants");
  Tcl_AppendElement(interp,"VisitRealConstants/Collect real constants");
  Tcl_AppendElement(interp,"VisitSymbolConstants/Collect symbol constants");
}

static
void ProbeGarbageCollect(int number)
{
  struct gl_list_t *new,  *old;
  struct ProbeEntry *e;
  unsigned long c, len;
  if (number <0 || number > (int)g_probe_array_size) {
    return;
  }
  old = ProbeArray(number);
  if (old==NULL) {
    g_probe_array[number] = gl_create(100L);
    return;
  }
  len = gl_length(old);
  if (len == 0L) {
    return;
  }
  new = gl_create(len);
  if (new==NULL) {
    return;
  }
  for (c = 1; c <= len; c++) {
    e = (struct ProbeEntry *)gl_fetch(old,c);
    if (ProbeEntryInst(e)!=NULL) {
      gl_append_ptr(new,e);
    } else {
      ProbeEntryDestroy(e);
    }
  }
  g_probe_array[number] = new;
  gl_destroy(old);
}
/*
 * macro ParseCollectionNumber assumes int status, char *argv[],
 * Tcl_Interp *interp , int number, int argc.Forces a return with
 * an appropriately filled interp if argv[2] is not an int or out of range.
 * This ugly macro is only for use in Asc_ProbeCmd.
 * The arg dummy is ignored.
 * This validates the number.
 */
#define ParseCollectionNumber(dummy) \
  if (argc <3) { \
    Tcl_AppendResult(interp, "\nProbe collection number missing ", \
      argv[0]," ", argv[1],(char *)NULL); \
    return TCL_ERROR; \
  } \
  status = Tcl_GetInt(interp,argv[2],&number); \
  if (status != TCL_OK) { \
    Tcl_AppendResult(interp, "\nError in probe collection number ", \
      argv[0]," ", argv[1]," ", argv[2],(char *)NULL); \
    return TCL_ERROR; \
  } \
  if (number < 0 || number >= (int)ProbeArraySize) { \
    Tcl_AppendResult(interp, "\nProbe collection number out of range ", \
      argv[0], " ",argv[1]," ", argv[2],(char *)NULL); \
    return TCL_ERROR; \
  }

/*
 * macro ParseCollectionIndex assumes int status, char *argv[],
 * Tcl_Interp *interp , int number, int index, int argc.Forces a return with
 * an appropriately filled interp if argv[2] is not an int or out of range.
 * This ugly macro is only for use in Asc_ProbeCmd.
 * Number is assumed to be a valid collection number.
 * This validates the index. ndx is the element of argv to be digested.
 */
#define ParseCollectionIndex(number,ndx) \
  if (argc <4) { \
    Tcl_AppendResult(interp, "\nProbe collection index missing: ", \
      argv[0]," ", argv[1]," ", argv[2], (char *)NULL); \
    return TCL_ERROR; \
  } \
  status = Tcl_GetInt(interp,argv[(ndx)],&indexe); \
  if (status != TCL_OK) { \
    Tcl_AppendResult(interp, "\nError in collection index: ", \
      argv[0]," ", argv[1]," ", argv[2]," ",argv[(ndx)],(char *)NULL); \
    return TCL_ERROR; \
  } \
  if ( indexe < 0 || \
       (unsigned long)indexe >= gl_length(g_probe_array[(number)])) { \
    Tcl_ResetResult(interp); \
    Tcl_AppendResult(interp,Asc_ProbeCmdHN," ",argv[1]," ",argv[2]," ", \
      argv[(ndx)], " : index out of list range.", (char *)NULL); \
    return TCL_ERROR; \
  }

STDHLF(Asc_ProbeCmd,(Asc_ProbeCmdHL1, Asc_ProbeCmdHL2, Asc_ProbeCmdHL3,
  Asc_ProbeCmdHL4, Asc_ProbeCmdHL5, Asc_ProbeCmdHL6, Asc_ProbeCmdHL7,
  Asc_ProbeCmdHL8, Asc_ProbeCmdHL9, Asc_ProbeCmdHL10, Asc_ProbeCmdHL11,
  HLFSTOP));

int Asc_ProbeCmd(ClientData cdata, Tcl_Interp *interp,
                 int argc, CONST84 char *argv[])
{
  int number, indexe, status, oldindexe, pos;
  unsigned int size;
  unsigned long c,len;
  struct ProbeEntry *e;
  char buf[MAXIMUM_NUMERIC_LENGTH];
  char *name;
  /* newlist may be leftover from a prior early exit. clear it, but not the
   * data if it is not NULL.
   * We should never see this leaked at shutdown if the probe has
   * been destroyed before exiting.
   */
  static struct gl_list_t *newlist = NULL, *oldlist;

  if (newlist!=NULL) {
    gl_destroy(newlist);
    newlist = NULL;
  }
  ASCUSE;
  if (argc <2) {
    Asc_HelpGetUsage(interp,Asc_ProbeCmdHN);
    return TCL_ERROR;
  };
  /*
   * On breaking out of the switch, the function returns tcl_ok.
   */
  switch (argv[1][0]) {
  case 'a': /* add */
    if (argc<4) {
      Tcl_AppendResult(interp,"Not enough arguments to ",
        Asc_ProbeCmdHN," ",argv[1],": need also <number> <instance>",
        " [filter-list]", (char *)NULL);
      return TCL_ERROR;
    }
    ParseCollectionNumber(0);
    g_cur_context = ProbeArray(number);
    status = Asc_QlfdidSearch3(argv[3],0);
    if (argc ==4) {
      name = ASC_STRDUP(argv[3]);
      e = ProbeEntryCreate(name,((status==0) ? g_search_inst : NULL));
      gl_append_ptr(g_cur_context,e);
      break;
    } else {
      if (status!=0) {
        Tcl_AppendResult(interp, Asc_ProbeCmdHN," ",argv[1]," ",argv[2],
        " ",argv[3],": unable to locate ",argv[3]," for search",
        (char *)NULL);
        return TCL_ERROR;
      }
    }
    g_visit_root = g_search_inst;
    if (argc == (4+NUMFILTERS)) {
      g_visit_root_name = QUIET(argv[3]);
      g_visit_root_name_len = strlen(argv[3]);
      SetupProbeFilter(&g_probe_filter,QUIET2(argv),4,argc);
      VisitInstanceTree(g_visit_root,ProbeVisitAll_Filtered,0,1);
    } else {
      sprintf(buf,"%d",NUMFILTERS);
      Tcl_AppendResult(interp,"Not enough arguments to ",
        Asc_ProbeCmdHN," ",argv[1]," ",argv[2]," ",argv[3]," [filter-list]: ",
        "filter-list needs ",buf," boolean values.",(char *)NULL);
      return TCL_ERROR;
    }
    break;
  case 'c': /* clear */
    ParseCollectionNumber(0);
    if (argc==3) { /* clear whole list */
      ProbeDeleteAll(ProbeArray(number));
      g_probe_array[number] = gl_create(100L);
      /* create cannot fail because we just recycled one of those */
    } else {
      newlist = gl_create(gl_length(g_probe_array[number]));
      if (newlist==NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp,Asc_ProbeCmdHN," ",argv[1]," ",argv[2],
          " : not enough memory",(char *)NULL);
        return TCL_ERROR;
      }
      pos = 3;
      c = 1;
      oldindexe = -1;
      /* for memory integrity we must copy and delete in separate passes */
      /* this also traps erroneous user indices automagically + safely */
      while(pos < argc) { /* copy list, except for the given indices. */
        ParseCollectionIndex(number,pos);
        if (indexe <= oldindexe) { /* no duplicates or reversals allowed */
          gl_destroy(newlist);
          newlist = NULL;
          Tcl_ResetResult(interp);
          Tcl_AppendResult(interp,Asc_ProbeCmdHN," ",argv[1]," ",argv[2],
            " : index out of order ",argv[pos],(char *)NULL);
          return TCL_ERROR;
        }
        oldindexe = indexe;
        while(c < (unsigned long)(indexe+1)) { /* copy up to the next index */
          gl_append_ptr(newlist,gl_fetch(g_probe_array[number],c));
          c++;
        }
        c++; /* skip the indicated element */
        pos++; /* next index */
      }
      /* copy the list tail after the last deleted entry */
      oldlist = g_probe_array[number];
      len = gl_length(oldlist);
      /* ? c++; */
      while (c <= len) {
        gl_append_ptr(newlist,gl_fetch(oldlist,c));
        c++;
      }
      /* destroy the indicated entries of the old list and swap in new one */
      pos = 3;
      c = 1;
      while(pos < argc) { /* copy list, except for the given indices. */
        ParseCollectionIndex(number,pos);
        ProbeEntryDestroy(ProbeGetEntry(number,indexe));
        pos++;
      }
      g_probe_array[number] = newlist;
      newlist = NULL;
      gl_destroy(oldlist);
    }
    break;
  case 'd': /* destroy */
    Asc_ProbeArrayDestroy();
    break;
  case 'e': /* expand */
    if (argc!=2) {
      Tcl_AppendResult(interp,"Too many arguments to ",
        Asc_ProbeCmdHN," ",argv[1],(char *)NULL);
      return TCL_ERROR;
    }
    if (Asc_ProbeArrayGrow()) {
      Tcl_AppendResult(interp,"Insufficient memory to ",
        argv[0]," ",argv[1],(char *)NULL);
      return TCL_ERROR;
    }
    sprintf(buf,"%u",ProbeArraySize-1);
    Tcl_AppendResult(interp,buf,(char *)NULL);
    break;
  case 'f': /* filter */
    DefineProbeFilters(interp);
    break;
  case 'g': /* get */
    ParseCollectionNumber(0);
    if (argc==3) {
      /* get whole list */
      ProbeAppendAll(interp,ProbeArray(number));
    } else {
      /* get the given indices. */
      pos = 3;
      while(pos < argc) {
        ParseCollectionIndex(number,pos);
        AppendEntryItem(interp,ProbeGetEntry(number,indexe));
        pos++; /* next index */
      }
    }
    break;
  case 'i': /* invalidate */
    NullAllInstancePointers();
    break;
  case 'n': /* name */
    if (argc!=4) {
      Tcl_AppendResult(interp,Asc_ProbeCmdHN," ",argv[1]," <number> <index>",
        (char *)NULL);
      return TCL_ERROR;
    }
    ParseCollectionNumber(0);
    ParseCollectionIndex(number,3);
    Tcl_AppendResult(interp,ProbeEntryName(ProbeGetEntry(number,indexe)),
      (char *)NULL);
    break;
  case 'q': /* qlfdid */
    if (argc!=4) {
      Tcl_AppendResult(interp,Asc_ProbeCmdHN," ",argv[1]," <number> <index>",
        (char *)NULL);
      return TCL_ERROR;
    }
    ParseCollectionNumber(0);
    ParseCollectionIndex(number,3);
    g_relative_inst = g_search_inst =
      ProbeEntryInst(ProbeGetEntry(number,indexe));
    if (g_search_inst==NULL) {
      Tcl_AppendResult(interp,"0",(char *)NULL);
    } else {
      Tcl_AppendResult(interp,"1",(char *)NULL);
    }
    break;
  case 's': /* size */
    /*
     * size of probe array or of one list in it.
     */
    if (argc==2) {
      sprintf(buf,"%u",ProbeArraySize);
      Tcl_AppendResult(interp,buf,(char *)NULL);
    } else {
      ParseCollectionNumber(0);
      size = ProbeNumEntries(number);
      sprintf(buf,"%d",size);
      Tcl_AppendResult(interp,buf,(char *)NULL);
    }
    break;
  case 't': /* trash */
    if (argc>3) {
      Tcl_AppendResult(interp,"Too many arguments to ",
        Asc_ProbeCmdHN," ",argv[1],(char *)NULL);
      return TCL_ERROR;
    }
    if (argc==2) { /* garbage collect all */
      for (number = 0; number < (int)g_probe_array_size; number++) {
        ProbeGarbageCollect(number);
      }
    } else {
      ParseCollectionNumber(0);
      ProbeGarbageCollect(number);
    }
    break;
  case 'u': /* update */
    if (argc>3) {
      Tcl_AppendResult(interp,"Too many arguments to ",
        Asc_ProbeCmdHN," ",argv[1],(char *)NULL);
      return TCL_ERROR;
    }
    if (argc==2) { /* garbage collect all */
      for (number = 0; number < (int)g_probe_array_size; number++) {
        ProbeUpdateEntries(ProbeArray(number));
      }
    } else {
      ParseCollectionNumber(0);
      ProbeUpdateEntries(ProbeArray(number));
    }
    break;
  default:
    Asc_HelpGetUsage(interp,Asc_ProbeCmdHN);
    return TCL_ERROR;
  }
#undef ParseCollectionIndex
#undef ParseCollectionNumber
  return TCL_OK;
}

