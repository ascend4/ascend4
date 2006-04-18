/* ex: set ts=2 : */
/*
 *  Interface Implementation
 *  by Tom Epperly
 *  Created: 1/17/90
 *  Version: $Revision: 1.53 $
 *  Version control file: $RCSfile: interface.c,v $
 *  Date last modified: $Date: 1998/06/17 15:33:21 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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

#include <signal.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#if (defined(__alpha) || defined(sun))
#include <malloc.h>
#endif
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/ascSignal.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "symtab.h"
#include "notate.h"
#include "braced.h"
#include "ascCompiler.h"
#include "commands.h"
#include "termsetup.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "visitinst.h"
#include "name.h"
#include "bit.h"
#include "instance_io.h"
#include "module.h"
#include "library.h"
#include "prototype.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "name.h"		/* these three included to do */
#include "stattypes.h"
#include "statement.h"		/* Interactive processing */
#include "vlist.h"
#include "vlistio.h"
#include "slist.h"
#include "statio.h"
#include "setinstval.h"
#include "extinst.h"
#include "arrayinst.h"
#include "instquery.h"
#include "copyinst.h"
#include "parentchild.h"
#include "mergeinst.h"
#include "refineinst.h"
#include "mathinst.h"
#include "atomvalue.h"
#include "destroyinst.h"
#include "instance_name.h"
#include "instantiate.h"
#include "value_type.h"
#include "watchpt.h"
#include "proc.h"
#include "initialize.h"
#include "check.h"
#include "pending.h"
#include "license.h"
#include "extfunc.h"
#include "packages.h"
#include "find.h"
#include "relation_type.h"
#include "relation.h"
#include "relation_util.h"
#include "relation_io.h"
#include "logical_relation.h"
#include "logrelation.h"
#include "logrel_util.h"
#include "logrel_io.h"
#include "setinstval.h"
#include "syntax.h"
#include "anontype.h"
#include "actype.h"           /*  required for isidchar()  */
#include "simlist.h"
#include "tmpnum.h"
#include "bintoken.h"
#include "interface.h"

#ifndef lint
static CONST char InterfaceRCSid[]="$Id: interface.c,v 1.53 1998/06/17 15:33:21 mthomas Exp $";
#endif /* lint */

extern int zz_parse();

/* character definitions */
#define ASC_FLUSH	'\025'		/* control-U */
#define ASC_BS	        '\010'		/* backspace */
#define ASC_DELETE	'\177'		/* delete */
#define ASC_NEWLINE	'\n'		/* newline */
#define ASC_CLEAR	'\f'		/* formfeed clear screen */

#define DEFMETHOD "default_self"
#define MAXID 256
#define MAXCOMMANDLINE 2048
#define PROMPT "Ascend>"
#define STARTUP "Welcome to ASCEND!\nPress '?' for a command summary.\n"
#define SHUTDOWN "So long!\n"
#define LEAVEHELP "exit the ASCEND system"

/* type definitions */

union argument {
  CONST char *id;
  struct Instance *i;
  struct value_t value;
  struct TypeDescription *desc;
};

/* global variables */
struct gl_list_t *g_def_list = NULL; /* list of definitions sorted */
				     /* alphabetically */
struct Instance *g_root = NULL; /* root instance */
struct Instance *g_search_inst = NULL; /* used for searching */

int open_bracket,open_quote;

/* fwd decls */
static void SetupBinTokens(void);

/*
 * We don't have a use for this yet. We are being protractive
 * in that any objects that we are referencing directly get
 * cleaned up *before* we call any routines that could
 * potentially move an instance.
 */
static
void InterfacePtrDeleteProc(struct Instance *i, char *ptr)
{
  (void)i;    /*  stop gcc whine about unused parameter  */
  (void)ptr;  /*  stop gcc whine about unused parameter  */
}

/*
 * No longer necessary as being handled properly in instance.c. This
 * function was necessary before for the following reasons:
 * Let us assume that there as a simulation sim, and it had a root instance,
 * root. If refine was called on sim->root, and the instance was moved in
 * memory, we would be notified by this function, and so that if:
 * sim->root == old, then sim->root = new. This was only necessary for
 * sims, as they used to be treated *NOT* as instances and so sim->root had
 * 0 parents, with all the associated implications.
 * Now that simulations are proper SIM_INSTances, their root instance has
 * at *least* 1 parent, (possibly more if they are universal !*), and the
 * necessary fixing up is done within the code in instance.c.
 *
 * For the time being it is being left to see if it gets invoked.
 */
static
void InterfaceNotifyProc(char *ptr, struct Instance *old, struct Instance *new)
{
  register unsigned long c,len;
  register struct Instance *sptr;
  (void)ptr;
  (void)new;
  len = gl_length(g_simulation_list);
  for(c=len;c>=1;c--){
    sptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (GetSimulationRoot(sptr) == old) {
      FPRINTF(stderr,
	      "Simulation %s has been relocated for your information.\n",
	      GetSimulationName(sptr));
      FPRINTF(stderr,"If you see this message please report to\n");
      FPRINTF(stderr,"\tascend+bugs@cs.cmu.edu\n");
    }
  }
}

static
void Trap(int sig)
{
  RestoreTerminal();
  putchar('\n');
  exit(sig);
}

static
void ResetTerminal(union argument *args,int argc)
{
  (void) argc;
  (void) args;
  TermSetup_ResetTerminal();
}

static
void ProtoTypeInstanceCmd(union argument *args, int argc)
{
  struct Instance *target, *result;
  CONST struct TypeDescription *desc;
  int start_time;

  if (argc!=1 || args[0].i==NULL) {
    FPRINTF(ASCERR,"Call is: proto instance\n");
    return;
  }

  start_time = clock();
  target = args[0].i;
  switch (InstanceKind(target)) {
  case ARRAY_ENUM_INST:
  case ARRAY_INT_INST:
  case SIM_INST:
    FPRINTF(ASCERR,"Cannot prototype this type of instance\n");
    return;
  default:
    break;
  }

  desc = InstanceTypeDesc(target);
  if (LookupPrototype(GetName(desc))) {
    FPRINTF(ASCERR,"A prototype already exists");
    return;
  }
  result = CopyInstance(target);        /* using copy by reference */
  start_time = clock() - start_time;
  if (result) {
    AddPrototype(result);
    FPRINTF(stderr,"Time to prototype instance = %d\n",start_time);
    return;
  }
  else{
    FPRINTF(ASCERR,"Error in prototyping instance");
    return;
  }
}


static
void PrintArrayTypes(union argument *args, int argc)
{
  (void) argc;
  (void) args;
  WriteArrayTypeList(stderr);
}

static
void DumpAT(FILE *fp,struct Instance *root)
{
  int start;
  struct gl_list_t *atl;
  start = clock();
  atl = Asc_DeriveAnonList(root);
  PRINTF("time to classify = %d\n",clock()-start);
  Asc_WriteAnonList(fp,atl,root,0);
  Asc_DestroyAnonList(atl);
}

static
void PrintAnonTypes(union argument *args, int argc)
{
  if (argc==0) {
    if (g_root!=NULL) {
      DumpAT(stdout,g_root);
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      FILE *fp;
      fp = fopen("/tmp/reldump.txt","w+");
      DumpAT(fp,args[0].i);
      fclose(fp);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

struct twolists {
  struct gl_list_t *rl, *il;
};
static
void GetRels(struct Instance *i,struct twolists *t)
{
  CONST struct relation *rel;
  enum Expr_enum reltype;
  if (i != NULL && t != NULL && InstanceKind(i) == REL_INST) {
    rel = GetInstanceRelation(i, &reltype);
    if (rel==NULL || reltype != e_token) {
      return;
    }
    gl_append_ptr(t->rl,(VOIDPTR)rel);
    gl_append_ptr(t->il,(VOIDPTR)i);
  }
}

extern void TimeCalcResidual(struct gl_list_t *, int);
static
void TimeResiduals(union argument *args, int argc)
{
  struct twolists t;
  struct Instance *i = NULL;
  clock_t start,stop;
  if (argc==0) {
    if (g_root!=NULL) {
      i = g_root;
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      i = args[0].i;
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
  if (i==NULL) return;
  t.il = gl_create(10000L);
  t.rl = gl_create(10000L);
  SilentVisitInstanceTreeTwo(i,(VisitTwoProc)GetRels,0,0,(VOIDPTR)&t);
  FPRINTF(ASCERR,"Relation count: %lu\n",gl_length(t.il));

  start = clock();
  TimeCalcResidual(t.il,1);
  stop = clock();
  stop -= start;
  FPRINTF(ASCERR,"PostfixSafe Time: %lu\n",(unsigned long)stop);

#if 0
  start = clock();
  TimeCalcResidual(t.rl,0);
  stop = clock();
  stop -= start;
  FPRINTF(ASCERR,"Binary Time: %lu\n",(unsigned long)stop);
#endif

  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
  if (setjmp(g_fpe_env)==0) {
    start = clock();
    TimeCalcResidual(t.rl,0);
    stop = clock();
    stop -= start;
    FPRINTF(ASCERR,"Binary-IGN Time: %lu\n",(unsigned long)stop);
  } else {
    FPRINTF(ASCERR,"Binary-IGN Time: SIGFPE -- REALLY_WIERD\n");
  }
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);

  Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
  if (setjmp(g_fpe_env)==0) {
    start = clock();
    TimeCalcResidual(t.il,2);
    stop = clock();
    stop -= start;
    FPRINTF(ASCERR,"Postfix Time: %lu\n",(unsigned long)stop);
  } else {
    FPRINTF(ASCERR,"Postfix Time: SIGFPE\n");
  }
  Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);

  Asc_SignalHandlerPush(SIGFPE,Asc_SignalTrap);
  if (setjmp(g_fpe_env)==0) {
    start = clock();
    TimeCalcResidual(t.il,3);
    stop = clock();
    stop -= start;
    FPRINTF(ASCERR,"Infix Time: %lu\n",(unsigned long)stop);
  } else {
    FPRINTF(ASCERR,"Infix Time: SIGFPE\n");
  }
  Asc_SignalHandlerPop(SIGFPE,Asc_SignalTrap);

  gl_destroy(t.rl);
  gl_destroy(t.il);
}

static
void PrintInstanceRelations(union argument *args, int argc)
{
  if (argc==0){
    if (g_root) {
      FILE *fp;
      fp = fopen("/tmp/reldump.txt","w+");
      WriteRelationsInTree(fp,g_root);
      fclose(fp);
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  }
  else{
    if (args[0].i) {
      FILE *fp;
      fp = fopen("/tmp/reldump.txt","w+");
      WriteRelationsInTree(fp,args[0].i);
      fclose(fp);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void PrintUniqueRelations(union argument *args, int argc)
{
  FILE *fp;
  struct gl_list_t *list;
  if (argc==0){
    if (g_root) {
      fp = fopen("/tmp/reldump.txt","w+");
      list = CollectTokenRelationsWithUniqueBINlessShares(g_root,400);
      WriteRelationsInList(fp,list);
      if (list != NULL)  {
        FPRINTF(ASCERR,"%lu relations written.\n",gl_length(list));
        gl_destroy(list);
      }
      fclose(fp);
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      fp = fopen("/tmp/reldump.txt","w+");
      list = CollectTokenRelationsWithUniqueBINlessShares(g_root,400);
      WriteRelationsInList(fp,list);
      if (list != NULL)  {
        FPRINTF(ASCERR,"%lu relations written.\n",gl_length(list));
        gl_destroy(list);
      }
      fclose(fp);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}


static
void PrintInstanceLogRelations(union argument *args, int argc)
{
  if (argc==0){
    if (g_root)
      WriteInstance(stdout,g_root);
    else{
      Bell();
      PRINTF("No root instance.\n");
    }
  }
  else{
    if (args[0].i) {
      FILE *fp;
      fp = fopen("/tmp/reldump.txt","w+");
      WriteLogRelationsInTree(fp,args[0].i);
      fclose(fp);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static struct Instance *g_wviinst;
static FILE *g_wvifp;
static
void WriteVisitIndex(struct Instance *i,unsigned long *list,int len,
                     VOIDPTR dummy)
{
  int c;
  WriteInstanceName(g_wvifp,i,g_wviinst);
  FPRINTF(g_wvifp,":\n");
  for ( c = 0; c < len; c++) {
    FPRINTF(g_wvifp,"%lu,",list[c]);
  }
  FPRINTF(g_wvifp,"\n");
}
static
void PrintIndexedVisit(union argument *args, int argc)
{
  if (argc==0){
    if (g_root) {
      WriteInstance(stdout,g_root);
    } else{
      Bell();
      PRINTF("No root instance.\n");
    }
  } else{
    if (args[0].i) {
      unsigned int l = 0;
      unsigned long *llp=NULL;
      g_wvifp = fopen("/tmp/reldump.txt","w+");
      IndexedVisitInstanceTree(args[0].i,WriteVisitIndex,0,0,&llp,&l,NULL);
      FPRINTF(stderr,"llen became %d\n",l);
      if (llp!=NULL) {
        ascfree(llp);
      }
      fclose(g_wvifp);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void NumberTreeDOT(struct Instance *i,unsigned long *num)
{
  struct Instance *ch;
  unsigned long c,len;
  (*num)++;
  SetTmpNum(i,*num);
  if (IsCompoundInstance(i)) {
    for (c=1, len = NumberChildren(i); c <= len; c++) {
      ch = InstanceChild(i,c);
      if (ch != NULL) {
        FPRINTF(g_wvifp,"%lu -> %lu\n",*num,GetTmpNum(ch));
      }
    }
  }
}

static
void PrintVisitMapDOT(union argument *args, int argc)
{
  if (argc==0){
    if (g_root) {
      WriteInstance(stdout,g_root);
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      unsigned int l;
      unsigned long num=0;
      g_wvifp = fopen("/tmp/reldump.txt","w+");
      SilentVisitInstanceTreeTwo(args[0].i,(VisitTwoProc)NumberTreeDOT,
				 1,0,&num);
      fclose(g_wvifp);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void NumberTree(struct Instance *i,unsigned long *num)
{
  (*num)++;
  SetTmpNum(i,*num);
}

static
void PrintVisitMap(union argument *args, int argc)
{
  if (argc==0){
    if (g_root) {
      WriteInstance(stdout,g_root);
    } else{
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      struct visitmapinfo *map;
      unsigned int l;
      char *ds;
      char *realetos[3] = {"ERR!","DOWN","-UP-"};
      char **etos;
      unsigned long maplen=0,num=0;
      etos = realetos+1;
      map = MakeVisitMap(args[0].i,&maplen);
      SilentVisitInstanceTreeTwo(args[0].i,(VisitTwoProc)NumberTree,1,0,&num);
      FPRINTF(stderr,"maplen = %lu\n",maplen);
      g_wvifp = fopen("/tmp/reldump.txt","w+");
      if (map==NULL) {
        FPRINTF(g_wvifp,"insufficient memory for map\n");
        fclose(g_wvifp);
        return;
      }
      FPRINTF(g_wvifp,"index\tcontext  child\tdir'n\tparent\n");
      for (l= 0; l <= maplen; l++) {
        FPRINTF(g_wvifp,"%d\t%lu\t%lu\t",l,
                         (map[l].context != NULL)?GetTmpNum(map[l].context):0L,
                         map[l].child);
        FPRINTF(g_wvifp,"%s\t%lu\t%d\n",
                         etos[map[l].dir],
                         (map[l].parent!=NULL)?GetTmpNum(map[l].parent):0L,
                         map[l].last);
      }
      fclose(g_wvifp);
      ascfree(map);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void SystemCmd(union argument *args, int argc)
{
  (void) argc;
  if (args[0].id==NULL) return;
  system(args[0].id);
  ascfree((char *)args[0].id); /* this id was made my strcpy, not addsymbol */
}

static
void HideChild(union argument *args, int argc)
{
  unsigned long c;
  symchar *name;
  if (argc==1 && g_root!=NULL){
    name = AddSymbol(args[0].id);
    c = ChildPos(GetChildList(InstanceTypeDesc(g_root)),name);
    if (c) {
      /* note this is a really crappy way to do this */
      /* should create InstanceHideChildByName,InstanceShowChildByName */
      /* in parentchild.h */
      ChildHide(GetChildList(InstanceTypeDesc(g_root)),c);
    } else {
      Bell();
      PRINTF("No child by name %s to hide.\n",args[0].id);
    }
  } else {
    Bell();
    PRINTF("No instance name to hide.\n");
  }
}

static
void ShowChildren(union argument *args, int argc)
{
  unsigned long c, len;
  (void) args;
  (void) argc;
  if (g_root!=NULL){
    len = NumberChildren(g_root);
    for (c = 1; c <=len; c++) {
      /* note this is a really crappy way to do this. */
      /* should create InstanceHideChild and InstanceShowChild(i,n) */
      /* in parentchild.h */
      ChildShow(GetChildList(InstanceTypeDesc(g_root)),c);
    }
  } else {
    Bell();
    PRINTF("No instance to unhide children of.\n");
  }
}

static
void PrintInstanceReach(union argument *args, int argc)
{
  struct Instance *i;
  if (argc==0){
    if (g_root) {
      i = g_root;
    } else {
      Bell();
      PRINTF("No root instance.\n");
      return;
    }
  } else {
    if (args[0].i) {
      i = args[0].i;
    } else {
      PRINTF("Incorrect instance.\n");
      return;
    }
  }
/*
 *AnonWriteMergeReachable(i);
 */
}

static
void TestLeaf(struct Instance *i)
{
  fprintf(ASCERR,"\n");
  WriteAliases(ASCERR,i);
  fprintf(ASCERR,"\n");
}

static
void LeavesInstance(union argument *args, int argc)
{
  struct Instance *i;
  if (argc==0){
    if (g_root) {
      i = g_root;
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      i = args[0].i;
    } else {
      Bell();
      PRINTF("Incorrect instance.\n");
    }
  }
  if (i == NULL ||
      (InstanceKind(i)!= ARRAY_INT_INST &&
       InstanceKind(i)!= ARRAY_ENUM_INST)) {
    Bell();
    PRINTF("Incorrect nonarray instance.\n");
  }
  ArrayVisitLocalLeaves(i,TestLeaf);
}

static
void PrintInstance(union argument *args, int argc)
{
  if (argc==0){
    if (g_root) {
      WriteInstance(stdout,g_root);
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      WriteInstance(stdout,args[0].i);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void ListParents(union argument *args, int argc)
{
  unsigned c,length;
  struct Instance *ptr;
  if (argc==0){
    if (g_root){
      ptr = g_root;
    }
    else{
      Bell();
      PRINTF("No root instance.\n");
      return;
    }
  }
  else
    ptr = args[0].i;
  if (ptr){
    length = NumberParents(ptr);
    for(c=1;c<=length;c++){
      WriteInstanceName(stdout,InstanceParent(ptr,c),NULL);
      putchar('\n');
    }
  }
  else{
    PRINTF("Incorrect instance.\n");
  }
}

static
void AliiInstance(union argument *args, int argc)
{
  if (argc==0){
    if (g_root) {
      WriteAliases(stdout,g_root);
    } else{
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      WriteAliases(stdout,args[0].i);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void IsasInstance(union argument *args, int argc)
{
  if (argc==0){
    if (g_root) {
      WriteISAs(stdout,g_root);
    } else{
      Bell();
      PRINTF("No root instance.\n");
    }
  } else {
    if (args[0].i) {
      WriteISAs(stdout,args[0].i);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void CliqueInstance(union argument *args, int argc)
{
  if (argc==0){
    if (g_root)
      WriteClique(stdout,g_root);
    else{
      Bell();
      PRINTF("No root instance.\n");
    }
  }
  else
    if (args[0].i)
      WriteClique(stdout,args[0].i);
    else
      PRINTF("Incorrect instance.\n");
}

static
void InstanceMerge(union argument *args, int argc)
{
  struct Instance *result;
  if (argc==2){
    if (args[0].i&&args[1].i){
      result = MergeInstances(args[0].i,args[1].i);
      PostMergeCheck(result);
    }
    else
      PRINTF("One of the two instances is incorrect.\n");
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}

static
void RunInitialization(union argument *args, int argc)
{
  struct Name *name=NULL;
  enum Proc_enum result;
  if (argc==2){
    name = CreateIdName(AddSymbol(args[1].id));
    if (args[0].i){
      result = Initialize(args[0].i,name,"sacompiler.",ASCERR,
         (WP_BTUIFSTOP|WP_STOPONERR),NULL,NULL);
      if (result!=Proc_all_ok) {
	/* error */
	Bell();
	PRINTF("Error executing initialization.\n");
      }
    } else{
      PRINTF("Incorrect instance.\n");
    }
    DestroyName(name);
  } else{
    Bell();
    PRINTF("Incorrect arguments. Instname, procname\n");
  }
}

static
void MakeAlike(union argument *args, int argc)
{
  struct TypeDescription *desc,*desc1,*desc2;
  if (argc==2){
    if (args[0].i && args[1].i){
      desc1 = InstanceTypeDesc(args[0].i);
      desc2 = InstanceTypeDesc(args[1].i);
      if (desc1==desc2) MergeCliques(args[0].i,args[1].i);
      else{
	if ((desc = MoreRefined(desc1,desc2))){
	  if (desc == desc1) RefineClique(args[1].i,desc,NULL);
	  else RefineClique(args[0].i,desc,NULL);
	  MergeCliques(args[0].i,args[1].i);
	}
	else{
	  Bell();
	  PRINTF("Instances are unconformable.\n");
	}
      }
    }
    else
      PRINTF("Incorrect instance.\n");
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}

static
void CallExternalCmd(union argument *args, int argc)
{
  int result;
  if (argc==0){
    Bell();
    PRINTF("Incorrect number of args to callproc\n");
  }
  else{
    if (args[0].i) {
      result = CallExternalProcs(args[0].i);
      if (result)
	PRINTF("Error in calling external procedure\n");
    }
    else
      PRINTF("Incorrect instance or calculation error.\n");
  }
}


static
void Shutdown(union argument *args, int argc)
{
  (void) args;
  (void) argc;
  if (g_def_list) gl_destroy(g_def_list);
  g_def_list = NULL;
  Asc_DestroySimulations();
  PRINTF("Done.\n");
}

static
void SetRoot(union argument *args, int argc)
{
  if (argc==1){
    g_root = args[0].i;
  }
  else{
    PRINTF("Reseting root to NULL.\n");
    g_root = NULL;
  }
}

static
void ListInstances(union argument *args, int argc)
{
  register unsigned long c,length;
  register struct Instance *ptr;
  struct Instance *i;
  struct InstanceName name;
  if (argc==0){
    if (g_root){
      i = g_root;
    }
    else{
      length = gl_length(g_simulation_list);
      for(c=1;c<=length;c++){
	ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
	PRINTF("%s\n",GetSimulationName(ptr));
      }
      return;
    }
  }
  else
    i = args[0].i;
  length = NumberChildren(i);
  for(c=1;c<=length;c++){
    name = ChildName(i,c);
    switch(InstanceNameType(name)){
    case IntArrayIndex:
      PRINTF("[%ld]\n",InstanceIntIndex(name)); break;
    case StrArrayIndex:
      PRINTF("['%s']\n",InstanceStrIndex(name)); break;
    case StrName:
      PRINTF("%s\n",InstanceNameStr(name)); break;
    }
  }
}

static
void WriteNotes(struct gl_list_t *nl)
{
  unsigned long c,len;
  struct bracechar *bc;
  struct Note *n;
  if (nl==NULL || gl_length(nl)==0) {
    printf("No notes.\n");
    return;
  }
  printf("NOTES\n");
  len = gl_length(nl);
  for (c=1; c <= len; c++) {
    n = (struct Note *) gl_fetch(nl,c);
    printf("'%s' ",SCP(GetNoteLanguage(n)));
    if (GetNoteEnum(n) == nd_vlist) {
      WriteVariableList(stdout,GetNoteData(n,nd_vlist));
      printf(" \"vlist\" {\n");
    } else {
      printf("%s {\n",SCP(GetNoteId(n)));
    }
    bc = GetNoteText(n);
    printf("%s\n} ",BraceCharString(bc));
    if (GetNoteMethod(n)!=NULL) {
      printf("METHOD %s ",SCP(GetNoteMethod(n)));
    }
    if (GetNoteType(n)!=NULL) {
      printf("Type %s ",SCP(GetNoteType(n)));
    }
    printf("\n");
  }
  printf("END NOTES;\n");
}

static
void TypeNotes(union argument *args, int argc)
{
  unsigned c,length;
  struct TypeDescription *desc;
  struct gl_list_t *nlist;
  if (argc==1) {
    nlist =
      GetNotes(LibraryNote(),GetName(args[0].desc),NOTESWILD,NOTESWILD,NOTESWILD,nd_empty);
    WriteNotes(nlist);
    gl_destroy(nlist);
  } else {
    Bell();
    PRINTF("Odd typenotes request.\n");
  }
}
static
void DefineType(union argument *args, int argc)
{
  unsigned c,length;
  struct TypeDescription *desc;
  if (argc==1) {
    WriteDefinition(stdout,args[0].desc);
  } else {
    if (g_def_list){
      length = gl_length(g_def_list);
      PRINTF("ATOM/CONSTANT types:\n");
      for(c=1;c<=length;c++){
	desc = (struct TypeDescription *)gl_fetch(g_def_list,c);
        if (BaseTypeIsAtomic(desc)!=0 || BaseTypeIsConstant(desc)!=0) {
	  PRINTF("\t%s\n",SCP(GetName(desc)));
        }
      }
      PRINTF("Complex types:\n");
      for(c=1;c<=length;c++){
	desc = (struct TypeDescription *)gl_fetch(g_def_list,c);
        if (BaseTypeIsAtomic(desc)==0 && BaseTypeIsConstant(desc)==0) {
	  PRINTF("\t%s\n",SCP(GetName(desc)));
        }
      }
    } else {
      Bell();
      PRINTF("Strange problem.\n");
    }
  }
}

static
void ListUniversals(union argument *args, int argc)
{
  (void) args;
  (void) argc;
  PRINTF("Not implemented yet.\n");
}

static
void ResumeInstantiation(union argument *args, int argc)
{
  if(argc==0){
    if (g_root) {
      SetupBinTokens();
      ReInstantiate(g_root);
    } else {
      Bell();
      PRINTF("No root instance.\n");
    }
  }
  else{
    if (args[0].i) {
      SetupBinTokens();
      ReInstantiate(args[0].i);
    } else {
      PRINTF("Incorrect instance.\n");
    }
  }
}

static
void lowerstring(register char *str)
{
  while (*str != '\0'){
    if ((*str >= 'A')&&(*str <= 'Z'))
      *str = *str + ('a' - 'A');
    str++;
  }
}


static
void Assignment(union argument *args, int argc)
{
  char str[MAXID];
  int length=0;
  if((argc==1)&&(args[0].i)){
    switch(InstanceKind(args[0].i)) {
    case REAL_ATOM_INST:
    case REAL_INST:
    case REAL_CONSTANT_INST:
      PRINTF("Please enter the real value followed by units:");
      ReadString(str,&length);
      SetRealAtomValue(args[0].i,atof(str),0);
      break;
    case BOOLEAN_ATOM_INST:
    case BOOLEAN_INST:
    case BOOLEAN_CONSTANT_INST:
      PRINTF("Please enter boolean value(true or false):");
      ReadString(str,&length);
      lowerstring(str);
      if (strcmp(str,"true")==0)
	SetBooleanAtomValue(args[0].i,1,0);
      else if (strcmp(str,"false")==0)
	SetBooleanAtomValue(args[0].i,0,0);
      else{
	Bell();
	PRINTF("Incorrect boolean value.\n");
      }
      break;
    case INTEGER_ATOM_INST:
    case INTEGER_INST:
    case INTEGER_CONSTANT_INST:
      if (AtomMutable(args[0].i)){
	PRINTF("Please enter the integer value:");
	ReadString(str,&length);
	SetIntegerAtomValue(args[0].i,atol(str),0);
      }
      else{
	Bell();
	PRINTF("Attempting to assign to a fixed integer.\n");
      }
      break;
    case SET_ATOM_INST:
    case SET_INST:
      Bell();
      PRINTF("Not implemented yet.\n");
      break;
    case SYMBOL_ATOM_INST:
    case SYMBOL_INST:
    case SYMBOL_CONSTANT_INST:
      PRINTF("Enter the symbol value enclosed by single quotes('):");
      ReadString(str,&length);

      break;
    default:
      Bell();
      PRINTF("The argument to assign is not a atom.\n");
    }
  }
  else{
    Bell();
    PRINTF("Incorrect arguments to assign.\n");
  }
}

static
int UniqueName(CONST char *str)
{
  unsigned long c;
  struct Instance *ptr;
  for(c=gl_length(g_simulation_list);c>=1;c--){
    ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (strcmp(SCP(GetSimulationName(ptr)),str)==0) return 0;
  }
  return 1;
}

static
int CmpSim(struct Instance *sim1, struct Instance *sim2)
{
  assert(sim1&&sim2);
  return CmpSymchar(GetSimulationName(sim1),GetSimulationName(sim2));
}

static
void GetDefinitionList(void)
{
  if (g_def_list) gl_destroy(g_def_list);
  g_def_list = DefinitionList();
}

static
void HLoadDefinitions(union argument *args, int argc)
{
  CONST char *name;
  int oldflag;
  /* Treat everything after the ``hload'' as the module to load
   * if (argc != 1) {
   *   Bell();
   *   PRINTF("Incorrect arguments\n");
   *   ascfree((char *)args[0].id);
   *   return;
   * }
   */
  (void)argc;
  name = args[0].id;
  if (Asc_OpenModule(name,NULL)){
    oldflag = GetParseRelnsFlag();
    SetParseRelnsFlag(0);
    zz_parse();
    SetParseRelnsFlag(oldflag);
    PRINTF("Module %s loaded.\n",name);
    GetDefinitionList();
  }
  else{
    Bell();
    PRINTF("Unable to load module %s\n",name);
  }
  ascfree((char *)args[0].id); /* this id was made my strcpy, not addsymbol */
}

static
void GlobalDefinitions(union argument *args, int argc)
{
#define SBUFSIZE 1024
  CONST char *name;
  char data[SBUFSIZE];
  struct module_t *m;
  struct gl_list_t *msl;
  unsigned long c,len;
  int status;
  FILE *fp;
  /* Treat everything after the ``gload'' as the module to load */
  (void)argc;
  name = args[0].id;
  fp = fopen( args[0].id, "r+");
  fgets(data,SBUFSIZE,fp);
  fclose(fp);
  if (strlen(data) == (SBUFSIZE-1)) {
    printf("test file may have been longer than %d characters read\n",
           SBUFSIZE-1);
  }
  printf(">>%s<<\n",data);
  m = Asc_OpenStringModule(data,&status,NULL);
  if (m != NULL ) {
    zz_parse();
    Asc_CloseCurrentModule();
    if (m != NULL) {
      PRINTF("Module name %s loaded.\n",Asc_ModuleName(m));
      PRINTF("Module best name %s loaded.\n",Asc_ModuleBestName(m));
      msl = Asc_ModuleStatementLists(m);
      if (msl != NULL) {
        PRINTF("Module statement ptr %lu.\n", (unsigned long)msl);
        len = gl_length(msl);
        PRINTF("Module # slists %lu.\n", len);
        for (c= 1; c<= len; c++) {
          WriteStatementList(stdout,gl_fetch(msl,c),4);
        }
      } else {
        PRINTF("Module no statements\n");
      }
      PRINTF("Module filename %s loaded.\n",Asc_ModuleFileName(m));
    }
    GetDefinitionList();
  }
  else{
    Bell();
    PRINTF("Unable to load module %s\n",name);
  }
  if (args[0].id != NULL) {
    ascfree((char *)args[0].id); /* this id was made my strcpy, not addsymbol */
  }
}

static
void LoadDefinitions(union argument *args, int argc)
{
  CONST char *name;
  /* Treat everything after the ``load'' as the module to load
   * if (argc != 1) {
   *   Bell();
   *   PRINTF("Incorrect arguments\n");
   *   ascfree((char *)args[0].id);
   *   return;
   * }
   */
  (void)argc;
  name = args[0].id;
  if (Asc_OpenModule(name,NULL)){
    zz_parse();
    PRINTF("Module %s loaded.\n",name);
    GetDefinitionList();
  }
  else{
    Bell();
    PRINTF("Unable to load module %s\n",name);
  }
  if (args[0].id != NULL) {
    ascfree((char *)args[0].id); /* this id was made my strcpy, not addsymbol */
  }
}

static
void RequireModule(union argument *args, int argc)
{
  CONST char *name;
  int status;       /* return value from RequireModule() */

  (void)argc;  /* unused parameter */

  name = args[0].id;
  Asc_RequireModule(name, &status );
  if( status == 5 ) {
    FPRINTF(ASCINF, "REQUIRED module %s already PROVIDED\n", name);
  }
  else if( status == 4 ) {
    FPRINTF(ASCERR, "Recursive REQUIRE for module %s.  Ignored\n", name);
  }
  else if( status != 0 ) {
    FPRINTF(ASCERR, "Unable to locate file for module %s\n", name);
  }
  else {
    zz_parse();
    PRINTF("Module %s loaded.\n",name);
    GetDefinitionList();
  }
  ascfree((char *)args[0].id); /* this id was made my strcpy, not addsymbol */
}


#define NEWINSTTEST 1
/* 1 means 3 phase compiler, 0 means 3phase, with no relations */
static
void CreateHollowInstance(union argument *args, int argc)
{
  CONST char *name;
  struct TypeDescription *type;
  struct Instance *new;
  unsigned int prel;
  if (argc == 2){
    name = args[0].id;
    type = args[1].desc;
    if (UniqueName(name)){
      time_t start, delta;

      g_ExtVariablesTable = NULL;			/* defined in extinst.[ch] */
#if (!NEWINSTTEST)
      prel = GetInstantiationRelnFlags();
      SetInstantiationRelnFlags(NORELS);
#endif
      SetupBinTokens();
      start = clock();
      new = Instantiate(GetName(type),AddSymbol(name),0,AddSymbol(DEFMETHOD));
      delta = clock()-start;
      PRINTF("New Instantiation time (u-sec): %d\n",delta);
#if (!NEWINSTTEST)
      SetInstantiationRelnFlags(prel);
#endif
      if (new) {				/* instantiate copies name */
	gl_insert_sorted(g_simulation_list,new,(CmpFunc)CmpSim);
      }
      else{
	PRINTF("Unable to create simulation %s\n",name);
      }
    }
    else{
      Bell();
      PRINTF("A simulation of the name %s already exists.\n",name);
      PRINTF("Please use a different name.\n");
    }
  }
  else{
    Bell();
    PRINTF("Incorrect arguments\n");
  }
}

static
void CreateInstance(union argument *args, int argc)
{
  CONST char *name;
  struct TypeDescription *type;
  struct Instance *new;
  if (argc == 2){
    name = args[0].id;
    type = args[1].desc;
    if (UniqueName(name)){
      time_t start, delta;
      g_ExtVariablesTable = NULL;	/* defined in extinst.[ch] */
      SetupBinTokens();
      start = clock();
      new = Instantiate(GetName(type),AddSymbol(name),0,AddSymbol(DEFMETHOD));
      delta = clock()-start;
      PRINTF("Instantiation time (u-sec): %d\n",delta);
      if (new) {				/* instantiate copies name */
	gl_insert_sorted(g_simulation_list,new,(CmpFunc)CmpSim);
      }
      else{
	PRINTF("Unable to create simulation %s\n",name);
      }
    }
    else{
      Bell();
      PRINTF("A simulation of the name %s already exists.\n",name);
      PRINTF("Please use a different name.\n");
    }
  }
  else{
    Bell();
    PRINTF("Incorrect arguments\n");
  }
}

static
void Help(union argument *args, int argc)
{
  unsigned long int com;
  if (argc == 1){
    com = FindCommand(args[0].id);
    if (com != 0) {
      CommandArgsPrint(stdout,com);
    } else {
      FPRINTF(stdout,"\"%s\" is not a defined command.\n",args[0].id);
    }
  } else {
    PRINTF(
      "Syntax: help <command name> to see the arguments of a command. Or,\n");
    PRINTF("while entering any command hit <space-?>"
           " to see options for next field.\n");
  }
}

static
void IntDestroyInstance(union argument *args, int argc)
{
  if (argc == 1){
    DestroyInstance(args[0].i,NULL);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}

void InterfaceCheckInstance(union argument *args, int argc)
{
  if (argc == 1){
    CheckInstance(stdout,args[0].i);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}

void InterfaceUseAnon(union argument *args, int argc)
{
  g_use_copyanon = !g_use_copyanon;
  PRINTF("g_use_copyanon = %d\n",g_use_copyanon);
}

static
void InterfaceInstanceMemory(union argument *args, int argc)
{
#if (defined(__alpha) || defined(sun))
  struct mallinfo mi;
#endif
  (void) args;
  (void) argc;

#ifdef MALLOC_DEBUG
  ascstatus("ALLOCATION REPORT:");
#else
  PRINTF("Compiled without MALLOC_DEBUG\n");
#endif
  /*
#ifdef sun
  mallocmap();
#endif
*/
#if (defined(__alpha) || defined(sun))
  mi = mallinfo();
  PRINTF("arena:\t%d\n",mi.arena);
  PRINTF("ordblks:\t%d\n",mi.ordblks);
  PRINTF("smblks:\t%d\n",mi.smblks);
  PRINTF("hblks:\t%d\n",mi.hblks);
  PRINTF("hblkhd:\t%d\n",mi.hblkhd);
  PRINTF("usmblks:\t%d\n",mi.usmblks);
  PRINTF("fsmblks:\t%d\n",mi.fsmblks);
  PRINTF("uordblks:\t%d\n",mi.uordblks);
  PRINTF("fordblks:\t%d\n",mi.fordblks);
  PRINTF("keepcost:\t%d\n",mi.keepcost);
#ifdef __SUN_SUNOS__
  PRINTF("mxfast:\t%d\n",mi.mxfast);
  PRINTF("nlblks:\t%d\n",mi.nlblks);
  PRINTF("grain:\t%d\n",mi.grain);
  PRINTF("uordbytes:\t%d\n",mi.uordbytes);
  PRINTF("allocated:\t%d\n",mi.allocated);
  PRINTF("treeoverhead:\t%d\n",mi.treeoverhead);
#endif
#endif

}

static
void InterfaceInstanceStatistics(union argument *args, int argc)
{
  if (argc == 1){
    InstanceStatistics(stdout,args[0].i);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}

/* these should go away */
#define NAME_DEBUG 0
#if NAME_DEBUG
extern unsigned long g_num_names_max;
extern unsigned long g_num_names_cur;
#endif
static
void InterfaceTokenStatistics(union argument *args, int argc)
{
  if (argc == 1){
    InstanceTokenStatistics(stdout,args[0].i);
    ReportPendingPool(ASCERR);
    ReportValueManager(ASCERR);
    ReportSetManager(ASCERR);
    ReportRelInstantiator(ASCERR);
    ReportLogRelInstantiator(ASCERR);
    ReportInstanceNanny(ASCERR);
    gl_report_pool(ASCERR);
    gl_reportrecycler(ASCERR);
#if NAME_DEBUG
    FPRINTF(ASCERR,"name elements %lu\n",g_num_names_cur);
    FPRINTF(ASCERR,"name elements max %lu\n",g_num_names_max);
#endif
    PrintTab(0);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}

static
void ShowNoWarranty(union argument *args, int argc)
{
  (void) args;
  (void) argc;
  PRINTF("%s",NO_WARRANTY);
}

static
void ShowGPL(union argument *args, int argc)
{
  (void) args;
  (void) argc;
  PRINTF("%s%s%s%s%s",GPL1,GPL2,GPL3,GPL4,GPL5);
}


static
void ScaleEqns(union argument *args, int argc)
{
  double j;
  enum Expr_enum reltype;
  struct relation *rel;
  if (argc == 1 && InstanceKind(args[0].i)==REL_INST){
     j = CalcRelationNominal(args[0].i);
     if (j > 0.0) {
       rel = (struct relation *)GetInstanceRelation(args[0].i,&reltype);
       SetRelationNominal(rel,j);
       PRINTF(" scale constant = %g\n", RelationNominal(rel));
     } else {
       PRINTF(" error in scale constant calculation.");
     }
  }
  else{
    Bell();
    PRINTF("Incorrect relation argument.\n");
  }
}

static
void ScaleAllEqns(union argument *args, int argc)
{
  if (argc == 1){
     PrintRelationNominals(args[0].i);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}
static
void CalcResiduals(union argument *args, int argc)
{
  if (argc == 1){
     PrintRelationResiduals(args[0].i);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}
static
void CalcAllVars(union argument *args, int argc)
{
  if (argc == 1){
     PrintDirectSolveSolutions(args[0].i);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}
static
void CalcAllBoolVars(union argument *args, int argc)
{
  if (argc == 1){
     PrintDirectSolveBooleanSolutions(args[0].i);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}
static
void CalcGradients(union argument *args, int argc)
{
  if (argc == 1){
     PrintRelationGradients(args[0].i);
  }
  else{
    Bell();
    PRINTF("Incorrect arguments.\n");
  }
}

static
void InstanceRefine(union argument *args, int argc)
{
  struct TypeDescription *desc, *desc1, *desc2;
  struct Instance *i, *top, *inst;
  if (argc>1) {
    PRINTF("Wrong # args. refine <typename>\n");
    return;
  }
  i = g_root;
  if (!i) {
    PRINTF("Cannot refine a NULL instance\n");
    return;
  }
  switch(InstanceKind(i)) {
  case REAL_INST: case BOOLEAN_INST:
  case INTEGER_INST: case SYMBOL_INST:
  case SET_INST: case REL_INST: case LREL_INST:
    PRINTF("AscendIV does not allow\nrefinement of\nchildren of ATOMs");
    return;
  default:
    break;
  }

  desc1 = InstanceTypeDesc(i);
  desc2 = args[0].desc;
  if (!desc2) {
    PRINTF("Type %s not found",args[0].id);
    return;
  }
  if (desc1==desc2) {
    return;
  }
  desc = MoreRefined(desc1,desc2);
  /* fix me. needs to prohibit refinement to parameterized types
   * or figure out how to get proper arguments
   */
  if(desc !=NULL) {
    if (desc == desc1) {
      /* desc1 more refined than desc2 */
      return;
    } else {
      inst = i;                     /* just in case refine moves i*/
      top = inst = RefineClique(inst,desc,NULL);
      do {                          /* Reinstatiate the entire clique */
        SetupBinTokens();
	ReInstantiate(inst);
	inst = NextCliqueMember(inst);
      } while (inst != top);
      g_root = i;
    }
  } else {
    PRINTF("Types are not conformable or the Library is inconsistent");
  }
}


static
void AddCommands(void)
{
  AddCommand(1,"print",PrintInstance,"print an instance",0,1,instance_arg);
  AddCommand(1,"visitlocal",LeavesInstance,"test arrayVisitLocalLeaves",0,1,
             instance_arg);
  AddCommand(1,"check",InterfaceCheckInstance,"check an instance",0,1,
	     instance_arg);
  AddCommand(1,"memory",InterfaceInstanceMemory,
	     "report memory data",0,0);
  AddCommand(1,"anoncompilation",InterfaceUseAnon,
	     "toggle anonymous relation sharing optimization",0,0);
  AddCommand(1,"statistics",InterfaceInstanceStatistics,
	     "report instance data",0,1,instance_arg);
  AddCommand(1,"tokens",InterfaceTokenStatistics,
	     "report token relation data",0,1,instance_arg);
  AddCommand(1,"aliases",AliiInstance,"list all the aliases of an instance",
	     0,1,instance_arg);
  AddCommand(1,"isas",IsasInstance,"list all the constructions of an instance",
	     0,1,instance_arg);
  AddCommand(1,"cliques",CliqueInstance,
	     "list all members of an instance's clique",0,1,instance_arg);
  AddCommand(1,"merge",InstanceMerge,"ARE_THE_SAME two instances",
	     0,2,instance_arg,instance_arg);
  AddCommand(1,"run",RunInitialization,"execute an initialization procedure",
	     0,2,instance_arg,id_arg);
  AddCommand(1,"make_alike",MakeAlike,"ARE_ALIKE two instances",
	     0,2,instance_arg,instance_arg);
  AddCommand(1,"callproc",CallExternalCmd,
	     "call an external procedure", 0,1,instance_arg);
  AddCommand(1,"quit",Shutdown,LEAVEHELP,1,0);
  AddCommand(1,"exit",Shutdown,LEAVEHELP,1,0);
  AddCommand(1,"bye",Shutdown,LEAVEHELP,1,0);
  AddCommand(1,"stop",Shutdown,LEAVEHELP,1,0);
  AddCommand(1,"reset",ResetTerminal,"reset the terminal",0,0);
/*  AddCommand(1,"minos",MinosInstance,"solve using MINOS 5.1",
	     0,1,instance_arg); */
/*  AddCommand(1,"optimize",OptimizeInstance,
	     "run Tom's algorithm on a NLP",0,1,instance_arg); */
/*  AddCommand(1,"solve",SolveInstance,"solve the system of equations",
	     0,1,instance_arg);*/
  AddCommand(1,"root",SetRoot,"set the context for all other searches",
	     0,1,instance_arg);
  AddCommand(1,"list",ListInstances,"list all the instances",
	     0,1,instance_arg);
  AddCommand(1,"define",DefineType,"define a type or list all the types",
	     0,1,definition_arg);
  AddCommand(1,"notes",TypeNotes,"list notes on a type",
	     0,1,definition_arg);
  AddCommand(1,"refine",InstanceRefine,"refine instance to new type",
	     0,1,definition_arg);
  AddCommand(1,"universals",ListUniversals,"list all the universal instances",
	     0,0);
  AddCommand(1,"resume",ResumeInstantiation,
	     "try to complete the instantiation of a partial instance",
	     0,1,instance_arg);
  AddCommand(1,"assign",Assignment,"assign a value to an instance",
	     0,1,instance_arg);
  AddCommand(1,"create",CreateInstance,"create an instance of a type",
	     0,2,id_arg,definition_arg);
  AddCommand(1,"hide",HideChild,"hide a child in a type",
	     0,1,id_arg);
  AddCommand(1,"showchildren",ShowChildren,"unhide children in a type",
	     0,0);
  AddCommand(1,"ncreate",CreateHollowInstance,
		"create (2 phase) an instance of a type",
	     0,2,id_arg,definition_arg);
  AddCommand(1,"load",LoadDefinitions,"load a module",
	     0,1,shell_arg);
  AddCommand(1,"hload",HLoadDefinitions,"load a module w/no relss",
	     0,1,shell_arg);
  AddCommand(1,"gload",GlobalDefinitions,"load a small module as a string",
	     0,1,shell_arg);
  AddCommand(1,"require",RequireModule,"load module if not previously loaded",
	     0,1,shell_arg);
  AddCommand(1,"help",Help,"give expected arg list for a given command",
	     0,1,id_arg);
  AddCommand(1,"destroy",IntDestroyInstance,
	     "deallocate and deconstruct an instance",0,1,instance_arg);
  AddCommand(1,"parents",ListParents,"list an instances parents",
	     0,1,instance_arg);
  AddCommand(1,"copying",ShowGPL,"show the terms of your license", 0,0);
  AddCommand(1,"arrays",PrintArrayTypes,"list array definitions", 0,0);
  AddCommand(1,"anons",PrintAnonTypes,"list anonymous types", 0,1,instance_arg);
  AddCommand(1,"warranty",ShowNoWarranty,"show no warranty notice", 0,0);
  AddCommand(1,"scale",ScaleEqns,"scales equations by largest additive term",
             0,1,instance_arg);
  AddCommand(1,"allscale",ScaleAllEqns,
             "scales equations by largest additive term",0,1,instance_arg);
  AddCommand(1,"timeresid",TimeResiduals,
             "times residuals using binary, infix and postfix functions",
             0,1,instance_arg);
  AddCommand(1,"calcresid",CalcResiduals,
             "calculates residuals using infix and postfix functions",
             0,1,instance_arg);
  AddCommand(1,"calcvars",CalcAllVars,
             "direct solves for each var in relation (for testing purposes)",
             0,1,instance_arg);
  AddCommand(1,"calcbvars",CalcAllBoolVars,
             "direct solves for each dvar in logrel(for testing purposes)",
	     0,1,instance_arg);
  AddCommand(1,"calcgrads",CalcGradients,
             "calculates gradients using postfix function",0,1,instance_arg);
  AddCommand(1,"relprint",PrintInstanceRelations,
             "prints relations to /tmp/reldump.txt",0,1,instance_arg);
  AddCommand(1,"urelprint",PrintUniqueRelations,
             "prints unique relations to /tmp/reldump.txt",0,1,instance_arg);
  AddCommand(1,"mapprint",PrintVisitMap,
             "prints visit map info to /tmp/reldump.txt",0,1,instance_arg);
  AddCommand(1,"dotprint",PrintVisitMapDOT,
             "prints visit map DOT to /tmp/reldump.txt",0,1,instance_arg);
  AddCommand(1,"lrelprint",PrintInstanceLogRelations,
             "prints log relations to /tmp/reldump.txt",0,1,instance_arg);
  AddCommand(1,"visitindex",PrintIndexedVisit,
             "prints visit indices/names to /tmp/reldump.txt",0,1,instance_arg);
  AddCommand(1,"proto",ProtoTypeInstanceCmd,
             "prototype an instance",0,1,instance_arg);
  AddCommand(1,"shell",SystemCmd, /* don't change 'shell' to anything else */
             "send rest of arguments to system shell",0,1,shell_arg);
}

static
void ClearBuffers(char *line, int *gpos, char *piece, int *pos)
{
  while((*gpos)>0) line[--(*gpos)] = '\0';
  while((*pos)>0) piece[--(*pos)] = '\0';
}

static
void RePrint(char *line)
{
  PRINTF("%s%s",PROMPT,line);
}

static
void WritePossible(unsigned long int lower, unsigned long int upper)
{
  while(lower<=upper){
    PRINTF("%-15s %s\n",CommandName(lower),CommandHelp(lower));
    lower++;
  }
}

static
void ClearPiece(char *str, int *count)
{
  while(*count > 0) str[--(*count)] = '\0';
}

/* c,&lower,&upper,full_line,piece,&gpos,&pos,&state, &arg_count,&done */
static
void AddShellChar(char c,
		  unsigned long int *l,
		  unsigned long int *u,
		  char *line,
		  char *piece,
		  int *gpos, int *pos, int *state,
		  int *argcount, int *done,union argument *args)
{
  int len;
  CONST char *p;
  (void)l;
  (void)u;
  (void)state;
  (void)argcount;
  *done = 0;
  if (isprint(c)){
    putchar(c);
    line[(*gpos)++] = c;
    piece[(*pos)++] = c;
  } else{
    switch(c){
    case ASC_FLUSH:
      putchar('\r');
      ClearLine();
      ClearBuffers(line,gpos,piece,pos);
      RePrint(line);
      break;
    case ASC_CLEAR:
      ResetTerminal(NULL,0);
      RePrint(line);
      break;
    case ASC_BS:
    case ASC_DELETE:
      if (*gpos > 0){
	DeleteBackOne();
	line[--(*gpos)] = '\0';
	if (*pos > 0){
	  piece[--(*pos)] = '\0';
	}
      } else {
	Bell();
      }
      break;
    case ASC_NEWLINE:
      putchar(ASC_NEWLINE);
      *done = 1;
      p = line;
      while( isspace(*p) && *p++ != '\0' ); /* remove leading whitespace */
      while( ! isspace(*p) && *p++ != '\0' ); /* command name */
      while( isspace(*p) && *p++ != '\0' ); /* remove whitespace between
                                             * command name are arguments */
      len = strlen(p);
      if (len > 0) {
        char *shellstring;
        shellstring = (char *)ascmalloc(len+1);
        strcpy(shellstring, p);
	args[0].id = shellstring;
        ClearBuffers(line,gpos,piece,pos);
      }
      break;
    default:
      Bell();
      break;
    }
  }
}


static
void AddCommandChar(char c,
		    unsigned long int *l,
		    unsigned long int *u,
		    char *line,
		    char *piece,
		    int *gpos, int *pos, int *state,
		    int *errloc, unsigned long int *com)
{
  int oldpos;
  *com = 0;
  if ((*l==0)&&(*u==0)){
    *l = 1;
    *u = NumberCommands();
    LimitCommand(l,u,piece,0);
  }
  if (isprint(c)){
    switch(c){
    case ' ':
      if (*pos){
	if (strcmp(piece,CommandName(*l))){
	  oldpos = *pos;
	  CompleteCommand(*l,*u,piece,pos);
	  while(oldpos < *pos){
	    putchar(piece[oldpos]);
	    line[(*gpos)++] = piece[oldpos++];
	  }
	} else {			/* equal */
	  putchar(' ');
	  line[(*gpos)++] = ' ';
	  *com = *l;
	  ClearPiece(piece,pos);
	}
      } else {
	line[(*gpos)++] = c;
	putchar(c);
      }
      break;
    case '?':
      putchar('\n');
      WritePossible(*l,*u);
      RePrint(line);
      break;
    default:
      putchar(c);
      line[(*gpos)++] = c;
      piece[(*pos)++] = c;
      LimitCommand(l,u,piece,*pos - 1);
      if (*l > *u){
	piece[--(*pos)] = '\0';
	*l = *u = 0;
	Bell();
	*errloc = *gpos - 1;
	*state = 5;
      }
      break;
    }
  } else{
    switch(c){
    case ASC_FLUSH:
      putchar('\r');
      ClearLine();
      *l=0;
      *u=0;
      ClearBuffers(line,gpos,piece,pos);
      RePrint(line);
      break;
    case ASC_CLEAR:
      ResetTerminal(NULL,0);
      RePrint(line);
      break;
    case ASC_BS:
    case ASC_DELETE:
      if (*gpos > 0){
	DeleteBackOne();
	line[--(*gpos)] = '\0';
	if (*pos > 0){
	  piece[--(*pos)] = '\0';
	}
	*l = *u = 0;
      }
      else
	Bell();
      break;
    case ASC_NEWLINE:
      putchar(ASC_NEWLINE);
      if ((*l <= *u)&&(strcmp(piece,CommandName(*l))==0)){
	*com = *l;
      }
      ClearBuffers(line,gpos,piece,pos);
      break;
    default:
      Bell();
      break;
    }
  }
}

static
void AddErrorChar(char c, char *line, int *gpos, char *piece,
		  int *pos, int *state, int errloc, int oldstate)
{
  if (isprint(c)){
    if (c=='?'){
      PRINTF("\nNo possible completions.\n");
      RePrint(line);
    } else {
      line[(*gpos)++] = c;
      putchar(c);
    }
  } else {
    switch(c){
    case ASC_FLUSH:
      putchar('\r');
      ClearLine();
      ClearBuffers(line,gpos,piece,pos);
      RePrint(line);
      *state = 0;
      break;
    case ASC_CLEAR:
      ResetTerminal(NULL,0);
      RePrint(line);
      break;
    case ASC_BS:
    case ASC_DELETE:
      if (*gpos > 0){
	DeleteBackOne();
	line[--(*gpos)] = '\0';
	if (*gpos == errloc){
	  *state = oldstate;
	}
      }
      else
	Bell();
      break;
    case ASC_NEWLINE:
      putchar(ASC_NEWLINE);
      ClearBuffers(line,gpos,piece,pos);
      *state = 0;
      break;
    default:
      Bell();
      break;
    }

  }
}

static
void InitializeArgs(unsigned long int com, union argument *args)
{
  int c,num;
  num=CommandNumArgs(com);
  for(c=0;c<num;c++){
    switch(CommandArgument(com,c)){
    case instance_arg:
      args[c].i = NULL;
      break;
    case definition_arg:
      args[c].desc = NULL;
      break;
    case id_arg:
      args[c].id = NULL;
      break;
    case shell_arg:
      args[c].id = NULL;
      break;
    }
  }
}

static
void GetLastPiece(register char *line,
		  register int gpos,
		  register char *piece,
		  register int *pos)
{
  register int c=0;
  assert((*piece=='\0')&&(*pos==0));
  c = gpos--;
  while((gpos >= 0)&&(line[gpos] != ' ')) gpos--;
  if (gpos >= 0) gpos++;
  else gpos = 0;
  while(gpos < c) piece[(*pos)++] = line[gpos++];
  piece[*pos] = '\0';
}

static
int StateFromArg(unsigned long int com, int arg)
{
  switch(CommandArgument(com,arg)){
  case instance_arg:
    g_search_inst = g_root;
    return 1;
  case definition_arg:	return 2;
  case id_arg:		return 3;
  case shell_arg:	return 7;
  }
  Asc_Panic(2, NULL, "Unknown argument type in StateFromArg\n");
  exit(2);/* Needed to keep gcc from whining */
}

static
void ControlCharacter(char c, char *line, int *gpos, char *piece,
		      int *pos, int *state, unsigned long int com, int *argc)
{
  switch(c){
  case ASC_FLUSH:
    putchar('\r');
    ClearLine();
    ClearBuffers(line,gpos,piece,pos);
    RePrint(line);
    *state = 0;
    break;
  case ASC_CLEAR:
    ResetTerminal(NULL,0);
    RePrint(line);
    break;
  case ASC_BS:
  case ASC_DELETE:
    if (*gpos > 0){
      DeleteBackOne();
      line[--(*gpos)] = '\0';
      if (*pos > 0)
	piece[--(*pos)] = '\0';
      else{
	if (line[*gpos - 1] != ' '){ /* backed into previous field */
	  if (*argc)
	    *state = StateFromArg(com,--(*argc));
	  else
	    *state = 0;
	  GetLastPiece(line,*gpos,piece,pos);
	}
      }
    }
    else
      Bell();
    break;
  case ASC_NEWLINE:
    putchar(ASC_NEWLINE);
    ClearBuffers(line,gpos,piece,pos);
    *state = 0;
    break;
  default:
    Bell();
    break;
  }
}

static
void AddIdChar(char c, char *line, int *gpos, char *piece,
	       int *pos, int *state, int *errloc,
	       int *done, union argument *args, int *argc,
	       unsigned long int com)
{
  *done = 0;
  if (isprint(c)){
    if (c == '?'){
      PRINTF("\nPlease enter an identifier.\n");
      RePrint(line);
    }
    else if (c == ' '){
      putchar(c);
      line[(*gpos)++] = c;
      if (*pos > 0){
	args[(*argc)++].id = SCP(AddSymbolL(piece,*pos));
	ClearPiece(piece,pos);
	*done = 1;
      }
    }
    else if (isidchar(c)){
      line[(*gpos)++] = c;
      piece[(*pos)++] = c;
      putchar(c);
    }
    else{
      putchar(c);
      line[(*gpos)++] = c;
      Bell();
      *errloc = *gpos - 1;
      *state = 5;
    }
  }
  else{
    if (c==ASC_NEWLINE){
      if (*pos>0)
	args[(*argc)++].id = SCP(AddSymbolL(piece,*pos));
      else
	args[*argc].id = NULL;
      *done = 1;
    }
    ControlCharacter(c,line,gpos,piece,pos,state,com,argc);
  }
}

static
int Defmycmp(register CONST char *str1, register CONST char *str2)
{
  while(*str1 != '\0'){
    if (*str1 < *str2) return -1;
    if (*str1 > *str2) return 1;
    str1++;
    str2++;
  }
  return 0;
}

static
int DefNumberMatch(register CONST char *str1,
		   register CONST char *str2,
		   int max)
{
  register int c=0;
  while((c<max)&&(*str1 != '\0')&&(*str1 == *str2)){
    str1++;
    str2++;
    c++;
  }
  return c;
}

static
unsigned long FindDefGLB(char *str, int place,
			 register unsigned long int lower,
			 register unsigned long int upper)
/*********************************************************************\
Find the greatist lower bound for a command string that could match
str.
\*********************************************************************/
{
  register struct TypeDescription *ptr;
  unsigned long search;
  str += place;
  while (lower <= upper){
    search = (lower+upper)/2;
    ptr = (struct TypeDescription *)gl_fetch(g_def_list,search);
    switch(Defmycmp(str,SCP(GetName(ptr))+place)){
    case 0:
      do {
	if (--search < lower) return lower;
	ptr = (struct TypeDescription *)gl_fetch(g_def_list,search);
      } while(Defmycmp(str,SCP(GetName(ptr))+place)==0);
      return search+1;
    case -1: upper = search-1; break;
    case 1: lower = search+1; break;
    }
  }
  return lower;
}

static
unsigned long FindDefLUB(char *str, int place,
			 register unsigned long int lower,
			 register unsigned long int upper)
/*********************************************************************\
Find the least upper bound for a command string that could match
str.
\*********************************************************************/
{
  register struct TypeDescription *ptr;
  unsigned long search;
  str += place;
  while (lower <= upper){
    search = (lower+upper)/2;
    ptr = (struct TypeDescription *)gl_fetch(g_def_list,search);
    switch(Defmycmp(str,SCP(GetName(ptr))+place)){
    case 0:
      do {
	if (++search > upper) return upper;
	ptr = (struct TypeDescription *)gl_fetch(g_def_list,search);
      } while(Defmycmp(str,SCP(GetName(ptr))+place)==0);
      return search-1;
    case -1: upper = search-1; break;
    case 1: lower = search+1; break;
    }
  }
  return upper;
}

static
void LimitDefinition(unsigned long int *l,
		     unsigned long int *u,
		     char *str, int pos)
{
  register struct TypeDescription *ptr;
  if (*l > *u) return;
  if (*l == *u){
    ptr = (struct TypeDescription *)gl_fetch(g_def_list,*l);
    if (Defmycmp(str+pos,SCP(GetName(ptr))+pos)) (*u)--;
  }
  else{
    *l = FindDefGLB(str,pos,*l,*u);
    *u = FindDefLUB(str,pos,*l,*u);
  }
}

static
void CompleteDefinition(unsigned long int l,
			unsigned long int u,
			char *str, int *place)
{
  register CONST char *cpy;
  int count;
  unsigned long pos;
  struct TypeDescription *ptr;
  ptr = (struct TypeDescription *)gl_fetch(g_def_list,l);
  cpy = SCP(GetName(ptr));
  count = (int)strlen(cpy);
  for(pos = l+1;pos<=u;pos++){
    ptr = (struct TypeDescription *)gl_fetch(g_def_list,pos);
    count = DefNumberMatch(cpy,SCP(GetName(ptr)),count);
  }
  strncpy(str,cpy,count);
  str[count] = '\0';
  *place = count;
}

static
void WritePossibleDefs(unsigned long int l, unsigned long int u)
{
  struct TypeDescription *desc;
  while(l <= u){
    desc = (struct TypeDescription *)gl_fetch(g_def_list,l++);
    PRINTF("%s\n",SCP(GetName(desc)));
  }
}

static
void AddDefinitionChar(char c,
		       unsigned long int *l,
		       unsigned long int *u,
		       char *line, int *gpos, char *piece, int *pos,
		       int *state, int *errloc, int *done,
		       union argument *args, int *argc,
		       unsigned long int com)
{
  int oldpos;
  *done = 0;
  if ((*l==0)&&(*u==0)){
    *l = 1;
    *u = gl_length(g_def_list);
    LimitDefinition(l,u,piece,0);
  }
  if (isprint(c)){
    switch(c){
    case ' ':
      if (*pos){
	if (!FindType(AddSymbol(piece))){
	  oldpos = *pos;
	  CompleteDefinition(*l,*u,piece,pos);
	  while (oldpos < *pos){
	    putchar(piece[oldpos]);
	    line[(*gpos)++] = piece[oldpos++];
	  }
	}
	else{
	  putchar(' ');
	  line[(*gpos)++] = ' ';
	  args[(*argc)++].desc = FindType(AddSymbol(piece));
	  *done = 1;
	  ClearPiece(piece,pos);
	}
      }
      else{
	line[(*gpos)++] = c;
	putchar(c);
      }
      break;
    case '?':
      putchar('\n');
      WritePossibleDefs(*l,*u);
      RePrint(line);
      break;
    default:
      putchar(c);
      line[(*gpos)++] = c;
      piece[(*pos)++] = c;
      LimitDefinition(l,u,piece,*pos - 1);
      if (*l > *u){
	piece[--(*pos)] = '\0';
	*l = *u = 0;
	Bell();
	*errloc = *gpos - 1;
	*state = 5;
      }
      break;
    }
  }
  else{
    switch(c){
    case ASC_FLUSH:
      putchar('\r');
      ClearLine();
      *l = *u =0;
      *state = 0;
      ClearBuffers(line,gpos,piece,pos);
      RePrint(line);
      break;
    case ASC_CLEAR:
      ResetTerminal(NULL,0);
      RePrint(line);
      break;
    case ASC_BS:
    case ASC_DELETE:
      if (*gpos > 0){
	DeleteBackOne();
	line[--(*gpos)] = '\0';
	if (*pos > 0){
	  piece[--(*pos)] = '\0';
	}
	else{
	  if (line[*gpos - 1] != ' '){ /* backed into previous field */
	    if (*argc)
	      *state = StateFromArg(com,--(*argc));
	    else
	      *state = 0;
	    GetLastPiece(line,*gpos,piece,pos);
	  }
	}
	*l = *u = 0;
      }
      else
	Bell();
      break;
    case ASC_NEWLINE:
      putchar(ASC_NEWLINE);
      *done = 1;
      if ((*l <= *u)&&(FindType(AddSymbol(piece)))){
	args[(*argc)++].desc = FindType(AddSymbol(piece));
      }
      ClearBuffers(line,gpos,piece,pos);
      break;
    default:
      Bell();
      break;
    }
  }
}

static
void AddDoneChar(char c, char *line, int *gpos, char *piece,
		 int *pos, int *state, int *errloc,
		 unsigned long int com, int *argc)
{
  if (isprint(c)){
    if (c == '?'){
      PRINTF("\nThe command and all its arguments"
             " are correct press return.\n");
      RePrint(line);
    }
    else {
      putchar(c);
      line[(*gpos)++] = c;
      if (c != ' '){
	*errloc = *gpos - 1;
	Bell();
	*state = 5;
      }
    }
  }
  else
    ControlCharacter(c,line,gpos,piece,pos,state,com,argc);
}

static
struct Instance *FindSimulationRoot(char *str)
{
  unsigned long c,len;
  struct Instance *ptr;

  len = gl_length(g_simulation_list);
  for (c=len;c>=1;c--){
    ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
    if (strcmp(SCP(GetSimulationName(ptr)),str)==0) {
      return GetSimulationRoot(ptr);
    }
  }
  return NULL;
}

static
void SearchStr(char *str, char *temp)
{
  register char *ptr;
  struct InstanceName name;
  open_bracket = 0;
  open_quote = 0;
  g_search_inst = g_root;
  ptr = temp;
  while(*str != '\0'){
    switch(*str){
    case '.':
      if (*(str-1) != ']'){
	*ptr = '\0';
	if (g_search_inst){
	  SetInstanceNameType(name,StrName);
	  SetInstanceNameStrPtr(name,AddSymbol(temp));
	  g_search_inst = InstanceChild(g_search_inst,
					ChildSearch(g_search_inst,&name));
	}
	else
	  g_search_inst = FindSimulationRoot(temp);
      }
      str++;
      ptr = temp;
      break;
    case '\'':
      str++;
      if (open_quote) open_quote--;
      else open_quote++;
      break;
    case '[':
      if (*(str-1) != ']'){
	*ptr = '\0';
	if (g_search_inst){
	  SetInstanceNameType(name,StrName);
	  SetInstanceNameStrPtr(name,AddSymbol(temp));
	  g_search_inst = InstanceChild(g_search_inst,
					ChildSearch(g_search_inst,&name));
	}
	else
	  g_search_inst = FindSimulationRoot(temp);
      }
      ptr = temp;
      open_bracket++;
      str++;
      break;
    case ']':
      open_bracket--;
      *ptr = '\0';
      str++;
      switch(InstanceKind(g_search_inst)){
      case ARRAY_INT_INST:
	SetInstanceNameType(name,IntArrayIndex);
	SetInstanceNameIntIndex(name,atol(temp));
	g_search_inst = InstanceChild(g_search_inst,
				      ChildSearch(g_search_inst,&name));
	break;
      case ARRAY_ENUM_INST:
	SetInstanceNameType(name,StrArrayIndex);
	SetInstanceNameStrIndex(name,AddSymbol(temp));
	g_search_inst = InstanceChild(g_search_inst,
				      ChildSearch(g_search_inst,&name));
	break;
      default:
	Bell();
	PRINTF("Unexpected error in FindInstanceLimits.\n");
	PRINTF("Report to %s\n",ASC_MILD_BUGMAIL);
	break;
      }
      ptr = temp;
      break;
    default:
      *(ptr++) = *(str++);
      break;
    }
  }
  *ptr = '\0';
}

static
void GetLastPart(char *str, char *part)
{
  int length;
  register char *ptr;
  length = strlen(str);
  while(length && (str[length] != '[') && (str[length] != '.'))
    length--;
  if (str[length] == '.') length++;
  ptr = part;
  while(str[length] != '\0')
    *(ptr++) = str[length++];
  *ptr = '\0';
}

static
int CmpName(struct InstanceName name1, struct InstanceName name2)
{
  assert(InstanceNameType(name1)==InstanceNameType(name2));
  switch(InstanceNameType(name1)){
  case IntArrayIndex:
    if (InstanceIntIndex(name1)<InstanceIntIndex(name2)) {
      return -1;
    } else {
      if (InstanceIntIndex(name1)==InstanceIntIndex(name2)) {
        return 0;
      } else {
        return 1;
      }
    }
  case StrArrayIndex:
    return Defmycmp(SCP(InstanceStrIndex(name1)),SCP(InstanceStrIndex(name2)));
  case StrName:
    return Defmycmp(SCP(InstanceNameStr(name1)),SCP(InstanceNameStr(name2)));
  }
  /*NOTREACHED*/
  return 1;
}

static
unsigned long FindSimLUB(char *str)
{
  unsigned long search,lower,upper;
  struct Instance *ptr;
  lower = 1;
  upper = gl_length(g_simulation_list);
  while(lower<=upper){
    search = (lower+upper)/2;
    ptr = (struct Instance *)gl_fetch(g_simulation_list,search);
    switch(Defmycmp(str,SCP(GetSimulationName(ptr)))){
    case 0:
      do {
	if (++search > upper) return upper;
	ptr = (struct Instance *)gl_fetch(g_simulation_list,search);
      } while (Defmycmp(str,SCP(GetSimulationName(ptr)))==0);
      return search - 1;
    case -1: upper = search - 1; break;
    case 1: lower = search + 1; break;
    }
  }
  return upper;
}

static
unsigned long FindSimGLB(char *str)
{
  unsigned long search,lower,upper;
  struct Instance *ptr;
  lower = 1;
  upper = gl_length(g_simulation_list);
  while(lower<=upper){
    search = (lower+upper)/2;
    ptr = (struct Instance *)gl_fetch(g_simulation_list,search);
    switch(Defmycmp(str,SCP(GetSimulationName(ptr)))){
    case 0:
      do {
	if (--search < lower) return lower;
	ptr = (struct Instance *)gl_fetch(g_simulation_list,search);
      } while (Defmycmp(str,SCP(GetSimulationName(ptr)))==0);
      return search + 1;
    case -1: upper = search - 1; break;
    case 1: lower = search + 1; break;
    }
  }
  return lower;
}

static
unsigned long InstanceGLB(struct Instance *i, struct InstanceName name)
{
  unsigned long search,lower,upper;
  struct InstanceName testname;
  lower = 1;
  upper = NumberChildren(i);
  while (lower <= upper){
    search = (lower+upper)/2;
    testname = ChildName(i,search);
    switch(CmpName(name,testname)){
    case 0:
      do {
	if (--search < lower) return lower;
	testname = ChildName(i,search);
      } while (CmpName(name,testname)==0);
      return search + 1;
    case -1: upper = search - 1; break;
    case 1: lower = search + 1; break;
    }
  }
  return lower;
}

static
unsigned long InstanceLUB(struct Instance *i, struct InstanceName name)
{
  unsigned long search,lower,upper;
  struct InstanceName testname;
  lower = 1;
  upper = NumberChildren(i);
  while (lower <= upper){
    search = (lower+upper)/2;
    testname = ChildName(i,search);
    switch(CmpName(name,testname)){
    case 0:
      do {
	if (++search > upper) return upper;
	testname = ChildName(i,search);
      } while (CmpName(name,testname)==0);
      return search - 1;
    case -1: upper = search - 1; break;
    case 1: lower = search + 1; break;
    }
  }
  return upper;

}

static
void LimitInstance(unsigned long int *l, unsigned long int *u, char *str)
{
  char part[MAXID];
  register char *ptr1,*ptr2;
  struct InstanceName name;
  GetLastPart(str,part);
  if (g_search_inst){
    switch(InstanceKind(g_search_inst)){
    case MODEL_INST:
    case REAL_ATOM_INST:
    case BOOLEAN_ATOM_INST:
    case INTEGER_ATOM_INST:
    case SET_ATOM_INST:
    case SYMBOL_ATOM_INST:
    case REL_INST:
    case LREL_INST:
    case WHEN_INST:
      assert(*part != '\0');
      SetInstanceNameType(name,StrName);
      SetInstanceNameStrPtr(name,AddSymbol(part));
      *l = InstanceGLB(g_search_inst,name);
      *u = InstanceLUB(g_search_inst,name);
      break;
    case ARRAY_INT_INST:
      *l = 1;
      *u = NumberChildren(g_search_inst);
      break;
    case ARRAY_ENUM_INST:
      ptr1 = part;
      while((*ptr1 != '\0')&&(*ptr1 != '\'')) ptr1++;
      if (*ptr1 == '\0'){
	*l = 1;
	*u = NumberChildren(g_search_inst);
      } else{
	ptr2 = ptr1++;
	while ((*ptr2 != '\0')&&(*ptr2 != '\'')) ptr2++;
	if (*ptr2 == '\'') *ptr2 = '\0';
	SetInstanceNameType(name,StrArrayIndex);
	SetInstanceNameStrIndex(name,AddSymbol(ptr1));
	*l = InstanceGLB(g_search_inst,name);
	*u = InstanceLUB(g_search_inst,name);
      }
      break;
    case REAL_CONSTANT_INST:
    case INTEGER_CONSTANT_INST:
    case BOOLEAN_CONSTANT_INST:
    case SYMBOL_CONSTANT_INST:
    case REAL_INST:
    case INTEGER_INST:
    case BOOLEAN_INST:
    case SET_INST:
    case DUMMY_INST:
    case SYMBOL_INST:
      *l = 1;
      *u = 0;
      break;
    default:
      break;
    }
  } else{				/* search simulation list */
    *l = FindSimGLB(part);
    *u = FindSimLUB(part);
  }
}

static
void FindInstanceLimits(unsigned long int *l,
			unsigned long int *u,
			char *str, int length)
{
  char temp[MAXID];
  if (length){
    SearchStr(str,temp);
    if (*temp != '\0')
      LimitInstance(l,u,str);
    else{
      assert(g_search_inst);
      *l = 1;
      *u = NumberChildren(g_search_inst);
    }
  }
  else{
    open_bracket = 0;
    open_quote = 0;
    if ((g_search_inst = g_root)){
      *l = 1;
      *u = NumberChildren(g_search_inst);
    }
    else{
      *l = 1;
      *u = gl_length(g_simulation_list);
    }
  }
}

static
struct Instance *InstanceComplete(char *str)
{
  char temp[MAXID];
  symchar *symbol;
  int length;
  unsigned long pos;
  struct InstanceName name;
  length = strlen(str);
  switch(str[length-1]){
  case '.': return NULL;
  case '[': return NULL;
  case '\'': return NULL;
  case ']':
    return g_search_inst;
  default:
    GetLastPart(str,temp);
    if (*temp == '[') return NULL;
    if (g_search_inst){
      SetInstanceNameType(name,StrName);
      symbol = AddSymbol(temp);
      SetInstanceNameStrPtr(name,symbol);
      if ((pos = ChildSearch(g_search_inst,&name))) {
	return InstanceChild(g_search_inst,pos);
      } else {
	return NULL;
      }
    }
    else return FindSimulationRoot(temp);
  }
}

static
void WritePossibleInstances(unsigned long int l, unsigned long int u)
{
  struct InstanceName name;
  struct Instance *ptr;
  if (g_search_inst){
    while(l <= u){
      name = ChildName(g_search_inst,l++);
      switch(InstanceNameType(name)){
      case IntArrayIndex:
	PRINTF("[%ld]\n",InstanceIntIndex(name));
	break;
      case StrArrayIndex:
	PRINTF("['%s']\n",InstanceStrIndex(name));
	break;
      case StrName:
	PRINTF("%s\n",InstanceNameStr(name));
	break;
      }
    }
  }
  else{
    while(l<=u){
      ptr = (struct Instance *)gl_fetch(g_simulation_list,l++);
      PRINTF("%s\n",GetSimulationName(ptr));
    }
  }
}

static
void CompleteInstance(unsigned long int *l,
		      unsigned long int *u,
		      char *str, int *pos)
{
  register unsigned long c;
  register CONST char *str1;
  register int count,oldlen;
  if (g_search_inst){
    struct InstanceName name;
    char part[MAXID];
    GetLastPart(str,part);
    switch(InstanceKind(g_search_inst)) {
    case MODEL_INST:
    case REAL_ATOM_INST:
    case BOOLEAN_ATOM_INST:
    case INTEGER_ATOM_INST:
    case SET_ATOM_INST:
    case SYMBOL_ATOM_INST:
    case REL_INST:
    case LREL_INST:
      name = ChildName(g_search_inst,*l);
      assert(InstanceNameType(name) == StrName);
      str1 = SCP(InstanceNameStr(name));
      count = strlen(str1);
      for(c = *l + 1; c <= *u;c++){
	name = ChildName(g_search_inst,c);
	assert(InstanceNameType(name) == StrName);
	count = DefNumberMatch(str1,SCP(InstanceNameStr(name)),count);
      }
      oldlen = strlen(part);
      while (oldlen < count)
	str[(*pos)++] = str1[oldlen++];
      str[*pos] = '\0';
      break;
    case ARRAY_INT_INST:
      break;
    case ARRAY_ENUM_INST:
      name = ChildName(g_search_inst,*l);
      assert(InstanceNameType(name) == StrArrayIndex);
      str1 = SCP(InstanceStrIndex(name));
      count = strlen(str1);
      for(c = *l + 1; c <= *u;c++){
	name = ChildName(g_search_inst,c);
	assert(InstanceNameType(name) == StrArrayIndex);
	count = DefNumberMatch(str1,SCP(InstanceStrIndex(name)),count);
      }
      oldlen = strlen(part);
      if (oldlen == 0){
	str[(*pos)++] = '[';
	str[(*pos)++] = '\'';
      } else if (oldlen == 1){
	str[(*pos)++] = '\'';
	oldlen = 0;
      }
      else oldlen -= 2;
      while (oldlen < count)
	str[(*pos)++] = str1[oldlen++];
      str[*pos] = '\0';
      break;
    default:
      Asc_Panic(2, NULL, "Bad instance type.\n");
    }
  }
  else{
    register struct Instance *ptr;
    ptr = (struct Instance *)gl_fetch(g_simulation_list,*l);
    str1 = SCP(GetSimulationName(ptr));
    count = strlen(str1);
    for(c = *l + 1;c <= *u;c++){
      ptr = (struct Instance *)gl_fetch(g_simulation_list,c);
      count = DefNumberMatch(str1,SCP(GetSimulationName(ptr)),count);
    }
    if (count > *pos) {
      strncpy(str,str1,count);
      str[count] = '\0';
      *pos = count;
    }
  }
}

static
int CheckClosingQuote(struct Instance *i, char *str, int pos)
{
  char temp[MAXID];
  register int open_pos=pos,c=0;
  struct InstanceName name;
  /* find the str */
  while(str[--open_pos] != '\'');
  while(open_pos <= pos)
    temp[c++] = str[++open_pos];
  SetInstanceNameType(name,StrArrayIndex);
  SetInstanceNameStrIndex(name,AddSymbol(temp));
  if (ChildSearch(i,&name)) return 1;
  else return 0;
}

static
int CheckOpeningBracket(char *str, int pos)
{
  char temp[MAXID];
  register int start=pos-1,c=0;
  struct Instance *inst;
  register unsigned long childpos;
  struct InstanceName name;
  if (pos==0){
    if (g_search_inst &&
	((InstanceKind(g_search_inst)==ARRAY_INT_INST)||
	 (InstanceKind(g_search_inst)==ARRAY_ENUM_INST)))
      return 1;
    return 0;
  }
  if ((str[start] == ']')&&
      ((InstanceKind(g_search_inst)==ARRAY_INT_INST)||
      (InstanceKind(g_search_inst)==ARRAY_ENUM_INST)))
    return 1;
  while((start>=0)&&(str[start] != '.')&&(str[start] != ']')) start--;
  while(start <= pos)
    temp[c++] = str[++start];
  if (g_search_inst){
    SetInstanceNameType(name,StrName);
    SetInstanceNameStrPtr(name,AddSymbol(temp));
    if ((childpos = ChildSearch(g_search_inst,&name))){
      inst = InstanceChild(g_search_inst,childpos);
      if ((InstanceKind(inst)==ARRAY_INT_INST)||
	  (InstanceKind(inst)==ARRAY_ENUM_INST)){
	g_search_inst = inst;
	return 1;
      }
    }
    return 0;
  }
  else{
    register unsigned long search,lower,upper;
    register struct Instance *ptr;
    register int cmp;
    lower = 1;
    upper = gl_length(g_simulation_list);
    while(lower<=upper){
      search = (lower+upper)/2;
      ptr = (struct Instance *)gl_fetch(g_simulation_list,search);
      if ((cmp = strcmp(temp,SCP(GetSimulationName(ptr))))==0){
	inst = GetSimulationRoot(ptr);
	if ((InstanceKind(inst)==ARRAY_INT_INST)||
	    (InstanceKind(inst)==ARRAY_ENUM_INST)){
	  g_search_inst = inst;
	  return 1;
	}
	return 0;
      }
      else if (cmp > 0) lower = search + 1;
      else upper = search - 1;
    }
    return 0;
  }
}

static
int CheckClosingPeriod(char *str, int pos)
{
  char temp[MAXID];
  register int start=pos-1,c=0;
  struct Instance *inst;
  register unsigned long childpos;
  struct InstanceName name;
  if (str[start] == ']'){
    switch(InstanceKind(g_search_inst)) {
    case MODEL_INST:
    case REAL_ATOM_INST:
    case BOOLEAN_ATOM_INST:
    case INTEGER_ATOM_INST:
    case SET_ATOM_INST:
    case SYMBOL_ATOM_INST:
    case REL_INST:
    case LREL_INST:
/* instances w/out children don't go here */
      return 1;
    default:
      break;
    }
    return 0;
  }
  while((start>=0)&&(str[start] != '.')&&(str[start] != ']')) start--;
  while(start <= pos)
    temp[c++] = str[++start];
  if (g_search_inst){
    SetInstanceNameType(name,StrName);
    SetInstanceNameStrPtr(name,AddSymbol(temp));
    if ((childpos = ChildSearch(g_search_inst,&name))){
      inst = InstanceChild(g_search_inst,childpos);
      switch(InstanceKind(inst)) {
      case MODEL_INST:
      case REAL_ATOM_INST:
      case BOOLEAN_ATOM_INST:
      case INTEGER_ATOM_INST:
      case SET_ATOM_INST:
      case SYMBOL_ATOM_INST:
      case REL_INST:
      case LREL_INST:
/* instances w/out children don't go here */
	g_search_inst = inst;
	return 1;
      default:
        break;
      }
    }
    return 0;
  }
  else{
    register int cmp;
    register unsigned long search,lower,upper;
    register struct Instance *ptr;
    lower = 1;
    upper = gl_length(g_simulation_list);
    while(lower<=upper){
      search = (lower+upper)/2;
      ptr = (struct Instance *)gl_fetch(g_simulation_list,search);
      if ((cmp = strcmp(temp,SCP(GetSimulationName(ptr))))==0){
	g_search_inst = GetSimulationRoot(ptr);
	return 1;
      }
      else if (cmp > 0) lower = search + 1;
      else upper = search - 1;
    }
    return 0;
  }
}

static
int IsIntegerStr(register char *str)
{
  register int started=0;
  while(*str != '\0'){
    if(isdigit(*str)) started = 1;
    else
      if ((!started)&&((*str == '+')||(*str == '-'))) started = 1;
      else return 0;
    str++;
  }
  return started;
}

static
int CheckClosingBracket(char *str, int pos)
{
  char temp[MAXID];
  register int start=pos-1,c=0;
  register unsigned long childpos;
  struct InstanceName name;
  if (str[start] == '\''){
    pos = --start;
    while(str[start] != '\'') start--;
    while(start < pos)
      temp[c++] = str[++start];
    temp[c] = '\0';
    SetInstanceNameType(name,StrArrayIndex);
    SetInstanceNameStrIndex(name,AddSymbol(temp));
    if ((childpos = ChildSearch(g_search_inst,&name))){
      g_search_inst = InstanceChild(g_search_inst,childpos);
      return 1;
    }
  }
  else{				/* integer index */
    while((str[start] != '[')) start--;
    while (start <= pos)
      temp[c++] = str[++start];
    if (IsIntegerStr(temp)){
      SetInstanceNameType(name,IntArrayIndex);
      SetInstanceNameIntIndex(name,atol(temp));
      if ((childpos = ChildSearch(g_search_inst,&name))){
	g_search_inst = InstanceChild(g_search_inst,childpos);
	return 1;
      }
    }
  }
  return 0;
}

static
void AddInstanceChar(char c,
		     unsigned long int *l,
		     unsigned long int *u,
		     char *line, int *gpos, char *piece,
		     int *pos, int *state,
		     int *errloc, int *done,
		     union argument *args, int *argc,
		     unsigned long int com)
{
  int oldpos;
  *done = 0;
  if ((*l == 0)&&(*u==0)){
    FindInstanceLimits(l,u,piece,*pos);
  }
  if (isprint(c)){
    switch(c){
    case ' ':
      if (*pos){
	if (InstanceComplete(piece)){
	  putchar(' ');
	  line[(*gpos)++] = ' ';
	  args[(*argc)++].i = InstanceComplete(piece);
	  *done = 1;
	  ClearPiece(piece,pos);
	}
	else{
	  oldpos = *pos;
	  CompleteInstance(l,u,piece,pos);
	  while(oldpos < *pos){
	    putchar(piece[oldpos]);
	    line[(*gpos)++] = piece[oldpos++];
	  }
	}
      }
      else{
	line[(*gpos)++] = c;
	putchar(c);
      }
      break;
    case '?':
      putchar('\n');
      WritePossibleInstances(*l,*u);
      RePrint(line);
      break;
    case '[':			/* check opening bracket */
      line[(*gpos)++] = c;
      putchar(c);
      if ((!(open_bracket)) &&
	  CheckOpeningBracket(piece,*pos)){
	open_bracket++;
	piece[(*pos)++] = c;
	*l = 1;
	*u = NumberChildren(g_search_inst);
      }
      else{
	*l = *u = 0;
	Bell();
	*errloc = *gpos - 1;
	*state = 5;
      }
      break;
    case '\'':			/* check end or beginning quote */
      line[(*gpos)++] = c;
      putchar(c);
      if (open_quote){
	if (CheckClosingQuote(g_search_inst,piece,*pos)){
	  piece[(*pos)++] = c;
	  open_quote--;
	}
	else{
	  *l = *u = 0;
	  Bell();
	  *errloc = *gpos -1;
	  *state = 5;
	}
      }
      else{
	if (g_search_inst && (InstanceKind(g_search_inst) == ARRAY_ENUM_INST)){
	  open_quote++;
	  piece[(*pos)++] = c;
	}
	else{
	  *l = *u = 0;
	  Bell();
	  *errloc = *gpos - 1;
	  *state = 5;
	}
      }
      break;
    case ']':			/* check closing bracket */
      putchar(c);
      line[(*gpos)++] = c;
      if (open_bracket && (!open_quote)&&CheckClosingBracket(piece,*pos)){
	piece[(*pos)++] = c;
	open_bracket--;
	*l = 1;
	*u = NumberChildren(g_search_inst);
      }
      else{
	*l = *u = 0;
	Bell();
	*errloc = *gpos -1;
	*state = 5;
      }
      break;
    case '.':			/* check period */
      putchar('.');
      line[(*gpos)++] = c;
      if (CheckClosingPeriod(piece,*pos)){ /* will change g_search_inst */
	piece[(*pos)++] = c;
	*l = 1;
	*u = NumberChildren(g_search_inst);
      }
      else{
	*l = *u = 0;
	Bell();
	*errloc = *gpos - 1;
	*state = 5;
      }
      break;
    default:
      putchar(c);
      line[(*gpos)++] = c;
      piece[(*pos)++] = c;
      LimitInstance(l,u,piece);
      if (*l > *u){
	piece[--(*pos)] = '\0';
	*l = *u = 0;
	Bell();
	*errloc = *gpos - 1;
	*state = 5;
      }
      break;
    }
  }
  else{
    switch(c){
    case ASC_FLUSH:
      putchar('\r');
      ClearLine();
      *l = *u =0;
      *state = 0;
      ClearBuffers(line,gpos,piece,pos);
      RePrint(line);
      break;
    case ASC_CLEAR:
      ResetTerminal(NULL,0);
      RePrint(line);
      break;
    case ASC_BS:
    case ASC_DELETE:
      if (*gpos > 0){
	DeleteBackOne();
	line[--(*gpos)] = '\0';
	if (*pos > 0){
	  piece[--(*pos)] = '\0';
	}
	else{
	  if (line[*gpos - 1] != ' '){ /* backed into previous field */
	    if (*argc)
	      *state = StateFromArg(com,--(*argc));
	    else
	      *state = 0;
	    GetLastPiece(line,*gpos,piece,pos);
	  }
	}
	*l = *u = 0;
      }
      else
	Bell();
      break;
    case ASC_NEWLINE:
      putchar(ASC_NEWLINE);
      *done = 1;
      if (InstanceComplete(piece)) /* removed (*l <= *u) */
	args[(*argc)++].i = InstanceComplete(piece);
      else if (*pos)
	args[(*argc)++].i = NULL;
      ClearBuffers(line,gpos,piece,pos);
      break;
    default:
      Bell();
      break;
    }
  }
}

static
void CommandProc(void)
{
  FILE *mystdout = stdout;
  char full_line[MAXCOMMANDLINE+1],piece[MAXID+1];
  int c;
  union argument args[MAX_COMMAND_ARGS];
  int error_location = -1,pos=0,gpos=0,state=0,oldstate=0,arg_count=0,done=0;
  unsigned long lower=0,upper=0,command=0;
  ascbzero(full_line,MAXCOMMANDLINE+1);
  ascbzero(piece,MAXID+1);
  PRINTF(PROMPT);
  while((c=getchar())!=4){
    switch(state){
    case 0:			/* command parsing state */
      oldstate = 0;
      AddCommandChar(c,&lower,&upper,full_line,piece,&gpos,&pos,&state,
		     &error_location,&command);
      if (command){
	arg_count = 0;
	lower = upper = 0;
	InitializeArgs(command,args);
	if (CommandNumArgs(command)==0) {
          state = 6;
	} else {
	  state = StateFromArg(command,arg_count);
        }
      }
      break;
    case 1:			/* instance parsing state */
      oldstate = 1;
      AddInstanceChar(c,&lower,&upper,full_line,&gpos,piece,&pos,&state,
		      &error_location,&done,args,&arg_count,command);
      if (done){
	lower = upper = 0;
	if (arg_count < CommandNumArgs(command)) {
	  state = StateFromArg(command,arg_count);
	} else {
	  state = 6;
        }
      }
      break;
    case 2:			/* definition parsing state */
      oldstate = 2;
      AddDefinitionChar(c,&lower,&upper,full_line,&gpos,piece,&pos,&state,
			&error_location,&done,args,&arg_count,command);
      if (done){
	lower = upper = 0;
	if (arg_count < CommandNumArgs(command)) {
	  state = StateFromArg(command,arg_count);
	} else {o
	  state = 6;
        }
      }
      break;
    case 3:			/* id parsing state */
      oldstate = 3;
      AddIdChar(c,full_line,&gpos,piece,&pos,&state,&error_location,&done,
		args,&arg_count,command);
      if (done){
	if (arg_count < CommandNumArgs(command)) {
	  state = StateFromArg(command,arg_count);
	} else {
	  state = 6;
        }
      }
      break;
    case 5:			/* error state */
      AddErrorChar(c,full_line,&gpos,piece,&pos,&state,
		   error_location,oldstate);
      break;
    case 6:			/* done waiting for newline */
      oldstate = 6;
      AddDoneChar(c,full_line,&gpos,piece,&pos,
		  &state,&error_location,command,&arg_count);
      break;
    case 7:			/* shell state */
      oldstate = 7;
      AddShellChar(c,&lower,&upper,full_line,piece,&gpos,&pos,&state,
		   &arg_count,&done,args);
      if (done){
	if (arg_count < CommandNumArgs(command)) {
	  state = StateFromArg(command,arg_count);
	} else {
	  state = 6;
        }
      }
      break;
    }
    if (c==ASC_NEWLINE){
      state = 0;
      if (command){
	void (*func)();
	CommandFunc(command,&func);
	(*func)(args,arg_count);
	if (CommandTerminate(command))  {
          return;
        }
      } else{
	if (gpos){
	  PRINTF("Unknown command.\n");
	}
      }
      FPRINTF(mystdout,"Ascend>"); /* PRINTF(PROMPT); */
    }
  } /* end while */
  putchar(ASC_NEWLINE);
}

static int g_uid=5;
static
void SetupBinTokens(void)
{
  char *b[5];
  char leaf[25];
  g_uid++;
  b[0]=(char *)ascmalloc(50);
  b[1]=(char *)ascmalloc(50);
  b[2]=(char *)ascmalloc(50);
  b[3]=(char *)ascmalloc(180);
  b[4]=(char *)ascmalloc(50);
  sprintf(b[0],"/tmp/btsrc%d.c",g_uid);
  sprintf(b[1],"/tmp/btsrc%d.o",g_uid);
  sprintf(b[2],"/tmp/btsrc%d.so",g_uid);
  sprintf(b[3],"cd /usr0/ballan/new/ascend4/lib; make -f Makefile.bt RM=/bin/rm BTINCLUDES=-I/usr0/ballan/new/ascend4/lib BTTARGET=/tmp/btsrc%d /tmp/btsrc%d",
    g_uid,g_uid);
  sprintf(b[4],"/bin/rm");
  BinTokenSetOptions(b[0],b[1],b[2],b[3],b[4],/*0*/1000,1,0);
}

extern
void Interface(void)
{
  clock();
  PRINTF(STARTUP);
  InterfaceNotify = InterfaceNotifyProc;
  InterfacePtrDelete = InterfacePtrDeleteProc;
  InitializeCommands();
  GetDefinitionList();
  SetupBinTokens();
  AddCommands();
  SetupTerminal();
  SetupTermcapStuff();
  (void)signal(SIGINT,Trap);
  /* command processor */
  CommandProc();
  RestoreTerminal();
  /* prepare for shutdown */
  DestroyCommands();
  InterfaceNotify = NULL;
  InterfacePtrDelete = NULL;
  PRINTF("%s",SHUTDOWN);
}
