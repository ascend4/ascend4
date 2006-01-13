/*
 *  Instance Checking Routines
 *  by Tom Epperly
 *  Created: 5/4/1990
 *  Version: $Revision: 1.25 $
 *  Version control file: $RCSfile: check.c,v $
 *  Date last modified: $Date: 1998/06/23 13:44:24 $
 *  Last modified by: $Author: ballan $
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

#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/pool.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/bit.h"
#include "compiler/symtab.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/types.h"
#include "compiler/instance_enum.h"
#include "compiler/stattypes.h"
#include "compiler/statio.h"
#include "compiler/module.h"
#include "compiler/statement.h"
#include "compiler/slist.h"
#include "compiler/instance_types.h" /* for sizes */
#include "compiler/parentchild.h"
#include "compiler/atomvalue.h"
#include "compiler/instquery.h"
#include "compiler/mathinst.h"
#include "compiler/visitinst.h"
#include "compiler/arrayinst.h"
#include "compiler/instance_io.h"
#include "compiler/instantiate.h"
#include "compiler/find.h"
#include "compiler/extfunc.h"
#include "compiler/relation_type.h"
#include "compiler/logical_relation.h"
#include "compiler/relation.h"
#include "compiler/logrelation.h"
#include "compiler/relation_util.h"
#include "compiler/logrel_util.h"
#include "compiler/exprio.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/library.h"
#include "compiler/instance_types.h"
#include "compiler/instance_name.h"
#include "compiler/cmpfunc.h"
#include "compiler/check.h"

#ifndef lint
static CONST char CheckModuleID[] = "$Id: check.c,v 1.25 1998/06/23 13:44:24 ballan Exp $";
#endif

/*
 * If 1, statistics can take a real long time.
 */
#define EXTRAPATHS 0

#define CLIQUE_WARNINGS 10000
#define NONZERO(x) ((x) ? (x) : 1)
/* array of statements we don't want written in errors */
static int *g_suppressions = NULL;
#define SUP(st) (g_suppressions[st])

/*
 * the following is a little batch of lists for catching unassigned
 * constants by type.
 * They should always be null outside the scope of an executing check
 * function since we have no operator for deallocating them.
 */
static struct {
  struct gl_list_t *rlist;
  struct gl_list_t *blist;
  struct gl_list_t *ilist;
  struct gl_list_t *slist;
} g_cons = {NULL,NULL,NULL,NULL};

static
int CheckInstanceType(FILE *f, CONST struct Instance *i,
		      CONST struct Instance *parent)
{
  switch(InstanceKind(i)) {
  case MODEL_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SET_ATOM_INST:
  case SYMBOL_ATOM_INST:

  case LREL_INST:
  case REL_INST:
  case WHEN_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:

  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:

  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case DUMMY_INST:
    return 0;
  default:
	error_reporter(ASC_PROG_ERROR,NULL,0,"Instance is not of a valid type in CheckInstanceType");
    FPRINTF(f,"A child of ");
    WriteInstanceName(f,parent,NULL);
    FPRINTF(f," is an incorrect data type.\n");
    FPRINTF(f,"This is the most serious type of error.\n");
    FPRINTF(f,"Please report this to %s\n",ASC_BIG_BUGMAIL);
    return 1;
  }
}

/*
 *  at present, the lists are unsorted because we want them in order found
 */
static void AppendUnique(struct gl_list_t *l, CONST struct Instance *i)
{
  if (gl_ptr_search(l,i,0)==0) gl_append_ptr(l,(VOIDPTR)i);
}

/*
 *this function now records rather than reporting constants.
 * It's pretty gross. Blame the whiney users.
 */
static
void CheckUnassignedConstants(FILE *f, CONST struct Instance *i)
{
  unsigned long c,len;
  struct Instance *child;

  (void)f;  /*  stop gcc whine about unused parameter  */

  len = NumberChildren(i);
  for(c=1;c<=len;c++){
    child = InstanceChild(i,c);
    if (child != NULL){
      switch(InstanceKind(child)){
      case REAL_CONSTANT_INST:
	if (IsWild(RealAtomDims(child)) || !AtomAssigned(child)) {
          AppendUnique(g_cons.rlist,child);
        }
	break;
      case INTEGER_CONSTANT_INST:
	if (!AtomAssigned(child)) {
          AppendUnique(g_cons.ilist,child);
        }
        break;
      case BOOLEAN_CONSTANT_INST:
	if (!AtomAssigned(child)) {
          AppendUnique(g_cons.blist,child);
        }
        break;
      case SYMBOL_CONSTANT_INST:
	if (!AtomAssigned(child)) {
          AppendUnique(g_cons.slist,child);
        }
        break;
      default:
	break;
      }
    }
  }
}

static
void CheckIncompleteArray(FILE *f, CONST struct Instance *i)
{
  unsigned long c,len;
  struct Instance *child;
  struct TypeDescription *desc;
  len = NumberChildren(i);
  for(c=1;c<=len;c++){
    child = InstanceChild(i,c);
    if (child != NULL){
      switch(InstanceKind(child)){
      case ARRAY_INT_INST:
      case ARRAY_ENUM_INST:
	desc = InstanceTypeDesc(child);
	if ((!GetArrayBaseIsRelation(desc)) &&
	    (!RectangleArrayExpanded(child)) &&
            (!GetArrayBaseIsLogRel(desc)) ) {
	  WriteInstanceName(f,child,NULL);
          FPRINTF(f,"[UNDEFINED]\n");
        }
      default:
	break;
      }
    }
  }
}

static
void CheckCompleteness(FILE *f, CONST struct Instance *i,int pass)
{
  unsigned long c, length;
  CONST struct BitList *blist;
  CONST struct TypeDescription *desc;
  CONST struct StatementList *slist;
  CONST struct Statement *stat;
  CONST struct gl_list_t *list;
  if (pass) {
    blist = InstanceBitList(i);
    if (blist!=NULL && !BitListEmpty(blist)){
	  error_reporter_start(ASC_PROG_WARNING,NULL,0);
      WriteInstanceName(f,i,NULL);
      FPRINTF(f," has the following unexecuted statements.\n");
      desc = InstanceTypeDesc(i);
      slist = GetStatementList(desc);
      list = GetList(slist);
      length = gl_length(list);
      assert(length == BLength(blist));
      for(c=1;c<=length;c++){
        if (ReadBit(blist,c-1)){
          stat = (struct Statement *)gl_fetch(list,c);
          WSEMSPARSE(f,stat,"Unable to execute",g_suppressions);
        }
      }
	  error_reporter_end_flush();
    }
  }
  if (IncompleteArray(i)) {
	error_reporter_start(ASC_PROG_WARNING,NULL,0);
    WriteInstanceName(f,i,NULL);
    FPRINTF(f," has the following incomplete arrays.\n");
    error_reporter_end_flush();
    CheckIncompleteArray(f,i);
  }
  CheckUnassignedConstants(f,i);
}

static
int SuppressNullInstance(CONST struct TypeDescription *d)
{
  if (d==NULL) return 0;
  switch (GetBaseType(d)) {
  case relation_type:
    return SUP(REL);
  case logrel_type:
    return SUP(LOGREL);
  case when_type:
    return SUP(WHEN);
  case set_type:
  case real_type:
  case integer_type:
  case boolean_type:
  case symbol_type:
  case real_constant_type:
  case integer_constant_type:
  case boolean_constant_type:
  case symbol_constant_type:
  case dummy_type:
  case model_type:
    return (SUP(ISA) || SUP(ALIASES));
  case patch_type:
  case array_type:
    return 0;
  }
  return 0;
}

static
void RecursiveCheckInstance(FILE *f, CONST struct Instance *i,
			    CONST struct Instance *parent,CONST int pass)
{
  CONST struct Instance *ptr;
  struct TypeDescription *desc;
  struct InstanceName name;
  unsigned long c=0;
  /* check for erroneous instance types */
  if (CheckInstanceType(f,i,parent)) return;
  /* check for unexecuted statements */
  CheckCompleteness(f,i,pass);
  /* check if i is connected to its parent */
  if (parent != NULL &&
      InstanceKind(i) != DUMMY_INST &&
      SearchForParent(i,parent) == 0){
    WriteInstanceName(f,parent,NULL);
    FPRINTF(f," thinks that it is a parent of ");
    WriteInstanceName(f,i,NULL);
    FPRINTF(f,"\nbut ");
    WriteInstanceName(f,i,NULL);
    FPRINTF(f," doesn't have it in its parent list.\n");
  }
  desc = InstanceTypeDesc(i);
  /* check the clique links */
  ptr = NextCliqueMember(i);
  while(ptr != i){
    ptr = NextCliqueMember(ptr);
    if (InstanceTypeDesc(ptr) != desc){
      FPRINTF(f,"CLIQUE ERROR\n");
      WriteInstanceName(f,i,NULL);
      FPRINTF(f," is of type %s and ", SCP(InstanceType(i)));
      WriteInstanceName(f,ptr,NULL);
      FPRINTF(f," is of type %s.\n",SCP(InstanceType(ptr)));
    }
    if ((++c % CLIQUE_WARNINGS)==0){
      FPRINTF(f,"POSSIBLE ERROR\n");
      WriteInstanceName(f,i,NULL);
      FPRINTF(f," is in a clique with %lu or more instances.\n",c);
      FPRINTF(f,
      "This is very large, so it may indicate an improper clique link.\n");
    }
  }
  if (c > CLIQUE_WARNINGS){
    FPRINTF(f,"Please ignore the clique warnings.  ");
    FPRINTF(f,"The clique was properly terminated.\n");
  }
  /* check i's parents */
  for(c=NumberParents(i);c>=1;c--){
    ptr = InstanceParent(i,c);
    if (!ChildIndex(ptr,i)){
      WriteInstanceName(f,i,NULL);
      FPRINTF(f," thinks that ");
      WriteInstanceName(f,ptr,NULL);
      FPRINTF(f," is one of its parents.\n");
      FPRINTF(f,"But ");
      WriteInstanceName(f,ptr,NULL);
      FPRINTF(f," doesn't think that ");
      WriteInstanceName(f,i,NULL);
      FPRINTF(f," is one of its children.\n");
    }
  }
  /* check children */
  for(c=NumberChildren(i);c>=1;c--){
    ptr = InstanceChild(i,c);
    if (ptr) {
      RecursiveCheckInstance(f,ptr,i,pass);
    } else {
      if (!SuppressNullInstance(ChildRefines(i,c))) {
        FPRINTF(f,"Instance ");
        WriteInstanceName(f,i,NULL);
        FPRINTF(f," is missing part `");
        name = ChildName(i,c);
        switch(InstanceNameType(name)){
        case IntArrayIndex:
          FPRINTF(f,"[%ld]'\n",InstanceIntIndex(name));
          break;
        case StrArrayIndex:
          FPRINTF(f,"[%s]'\n",SCP(InstanceStrIndex(name)));
          break;
        case StrName:
          FPRINTF(f,"%s'\n",SCP(InstanceNameStr(name)));
          break;
        default:
          FPRINTF(f,"BAD NAME!!!'\n");
          break;
        }
      }
    }
  }
}

static void InitConsLists(void)
{
  g_cons.rlist = gl_create(100);
  g_cons.blist = gl_create(10);
  g_cons.slist = gl_create(20);
  g_cons.ilist = gl_create(20);
}
static void ClearConsLists(void)
{
  gl_destroy(g_cons.rlist);
  g_cons.rlist=NULL;
  gl_destroy(g_cons.blist);
  g_cons.blist=NULL;
  gl_destroy(g_cons.slist);
  g_cons.slist=NULL;
  gl_destroy(g_cons.ilist);
  g_cons.ilist=NULL;
}

static FILE *g_diagf;
/*
 * global pointer so gliterate will have a file to print to in WriteConsLists
 */

static
void Diagnose(struct Instance *i) {
  if (InstanceKind(i)== REAL_CONSTANT_INST) {
    if (IsWild(RealAtomDims(i))) {
      FPRINTF(g_diagf,"Undimensioned ");
    }
    if (!AtomAssigned(i)) {
      FPRINTF(g_diagf,"Unassigned ");
    }
    FPRINTF(g_diagf,"real constant ");
    WriteInstanceName(g_diagf,i,NULL);
    FPRINTF(g_diagf,"\n");
  } else {
    FPRINTF(g_diagf,"Unassigned constant ");
    WriteInstanceName(g_diagf,i,NULL);
    FPRINTF(g_diagf,"\n");
  }
}

/* writes out the unassigned foo based on the flags given (pr,b,s,i) */
static void WriteConsLists(FILE *f, int pr, int pb, int pi, int ps)
{
  g_diagf = f;
  if (pr) gl_iterate(g_cons.rlist,(void (*)(VOIDPTR))Diagnose);
  if (pb) gl_iterate(g_cons.blist,(void (*)(VOIDPTR))Diagnose);
  if (pi) gl_iterate(g_cons.ilist,(void (*)(VOIDPTR))Diagnose);
  if (ps) gl_iterate(g_cons.slist,(void (*)(VOIDPTR))Diagnose);
}

void CheckInstanceLevel(FILE *f, CONST struct Instance *i,int pass)
{
  InitConsLists();
  g_suppressions = GetStatioSuppressions();
  if (pass<5) g_suppressions[ASGN]=1;
  if (pass<4) g_suppressions[WHEN]=1;
  if (pass<3) g_suppressions[LOGREL]=1;
  if (pass<2) g_suppressions[REL]=1;
  RecursiveCheckInstance(f,i,NULL,pass);
  DestroySuppressions(g_suppressions);
  WriteConsLists(f,1,1,1,1);
  ClearConsLists();
}

void CheckInstanceStructure(FILE *f,CONST struct Instance *i)
{
  InitConsLists();
  RecursiveCheckInstance(f,i,NULL,5);
  WriteConsLists(f,0,1,1,1);
  ClearConsLists();
}

/*********************************************************************\
Implementation of the InstanceStatistics procedure.
\*********************************************************************/

struct TypeCount {
  CONST char *name;		/* the name of the type. !symchar */
  enum inst_t basetype;		/* kind of the instance */
  unsigned count;		/* the number present in the instance tree */
};

/*********************************************************************\
Global for counting number of each type present in the instance
tree.
\*********************************************************************/
static struct gl_list_t *g_type_count_list=NULL;
				/* a list of struct TypeCount */

/*********************************************************************\
Globals for counting instances
\*********************************************************************/
static unsigned long g_num_complex_instances=0;
	/* number of models and  atoms(excluding children of atoms) */
static unsigned long g_num_model_instances=0;
	/* number of models */
static unsigned long g_model_bytes=0;
	/* number of model bytes */
static unsigned long g_num_atom_instances=0;
	/* number of atoms (nonfundamental) */
static unsigned long g_atom_bytes=0;
	/* number of atom bytes */
static unsigned long g_tree_bytes=0;
/* number of nonshared instance tree bytes,
   (ignore struct relation*, set_t*, dim_type *, char *)*/
static unsigned long g_num_constant_real=0;
static unsigned long g_num_constant_bool=0;
static unsigned long g_num_constant_int=0;
static unsigned long g_num_constant_sym=0;
static unsigned long g_num_constant_all=0;
				/* number of constants of various types */
static unsigned long g_num_atom_children=0;
				/* number of children of atoms or relations */
static unsigned long g_num_relation_instances=0;
				/* number of relations */
static struct gl_list_t *g_relation_guts = NULL;
				/* list of token relation guts */
static unsigned long g_num_array_instances=0;
				/* number of array instances */
static unsigned long g_num_unsel_instances=0;
				/* number of unselected instances */
/*********************************************************************\
The total number of instances should be equal to:
	g_num_complex_instances + g_num_atom_children +
	g_num_relation_instances + g_num_array_instances +
	g_num_constant_all
\*********************************************************************/

/*********************************************************************\
Globals for counting number of parents
	All of the following are counted for models, atoms(excluding
	children of atoms), and arrays.  Relations and children of atoms
	aren't counted because they can realistically only have one
	parent.
	Parents of constants aren't counted yet. need more vars.
\*********************************************************************/
static unsigned g_minimum_parents=0;
static unsigned g_maximum_parents=0;
static unsigned g_extra_paths=0;
static unsigned g_extra_parents=0;
static unsigned g_extra_parents_sum=0;
static unsigned g_total_parents=0;

/*********************************************************************\
Globals for counting numbers of children
	The following are counted for models, atoms(excluding children
	of atoms), arrays, and relations.  These globals are used to
	report the minimum, maximum, and average number of children.
\*********************************************************************/
static unsigned g_minimum_children=0;
static unsigned g_maximum_children=0;
static unsigned g_total_children=0;

/*********************************************************************\
Globals for counting the number of cliques
This global is used to estimate the number and min, max, and average
size of the cliques in this tree.
\*********************************************************************/
static struct gl_list_t *g_clique_list=NULL;  /* of struct Instance * */

/*********************************************************************\
Globals for estimating the number of relations in which each real appears.
These are used to estimate the min, max, and average size of the relation
list.
\*********************************************************************/
static unsigned g_total_variables=0;
static unsigned g_minimum_relations=0;
static unsigned g_maximum_relations=0;
static unsigned g_total_relations=0;


/*********************************************************************\
Globals for counting the number of terms in a relation.
A count is also made of all the reals in relations. This is
to get an estimate of how expensive the relations are.
\*********************************************************************/
static unsigned long g_total_reals_in_rels=0;
static unsigned long g_relation_terms=0;

/*********************************************************************\
Globals for calculating the minimum, maximum, and average number
of array elements per array.
\*********************************************************************/
static unsigned g_total_array_children=0;

/*********************************************************************\
\*********************************************************************/

long g_token_counts[NUM_EXPR_ENUMS];

/* this sorts by inst_t with a secondary alpha sort */
static
int CompareTypeCounts(CONST struct TypeCount *one,
		      CONST struct TypeCount *two)
{
  if (one == two) return 0;
  if (!one) return 1;		/* NULLs float to top */
  if (!two) return -1;		/* NULLs float to top */

  if (one->basetype==two->basetype) {
    return strcmp(one->name,two->name);
  } else {
    return (one->basetype < two->basetype) ? -1 : 1;
  }
}

static unsigned CliqueSize(CONST struct Instance *i)
{
  unsigned result = 0;
  CONST struct Instance *ptr;
  ptr = i;
  do {
    result++;
    ptr = NextCliqueMember(ptr);
  } while(ptr != i);
  return result;
}

static unsigned MinimumCliqueSize(struct gl_list_t *l)
{
  unsigned result = UINT_MAX,size;
  unsigned long c, length;
  CONST struct Instance *i;
  for(c=1, length = gl_length(l);c <= length;c++){
    i = gl_fetch(l,c);
    size = CliqueSize(i);
    if (size < result) result = size;
  }
  return length ? result : 0;
}

static unsigned MaximumCliqueSize(struct gl_list_t *l)
{
  unsigned result = 0,size;
  unsigned long c, length;
  CONST struct Instance *i;
  for(c=1, length = gl_length(l);c <= length;c++){
    i = gl_fetch(l,c);
    size = CliqueSize(i);
    if (size > result) result = size;
  }
  return length ? result : 0;
}

static double AverageCliqueSize(struct gl_list_t *l)
{
  unsigned long total_clique_members=0;
  unsigned long c, length;
  CONST struct Instance *i;
  for(c=1, length = gl_length(l);c <= length;c++){
    i = gl_fetch(l,c);
    total_clique_members += CliqueSize(i);
  }
  return length ? (double)total_clique_members/(double)length : 1;
}

static void IncrementTypeCount(CONST struct Instance *i)
{
  unsigned long index;
  struct TypeCount rec,*ptr;
  symchar *name;

  name = InstanceType(i);
  if (*SCP(name)) {
    rec.name = SCP(name);
  } else {
    rec.name = "UNNAMED ARRAY NODE";
  }
  rec.basetype = InstanceKind(i);
  index = gl_search(g_type_count_list,&rec, (CmpFunc)CompareTypeCounts);
  if (index != 0){
    /* increment the current count by one */
    ptr = (struct TypeCount *)gl_fetch(g_type_count_list,index);
    ptr->count++;
  } else{				/* add a new type count to the list */
    ptr = (struct TypeCount *)ascmalloc(sizeof(struct TypeCount));
    ptr->name = rec.name;
    ptr->count = 1;
    ptr->basetype = rec.basetype;
    gl_insert_sorted(g_type_count_list,ptr, (CmpFunc)CompareTypeCounts);
  }
}

static int InClique(CONST struct Instance *test,
		    CONST struct Instance *clique)
/* return true iff test is in clique */
{
  CONST struct Instance *ptr;
  ptr = clique;
  do{
    if (ptr == test) return 1;
    ptr = NextCliqueMember(ptr);
  } while(ptr != clique);
  return 0;
}

static void CheckCliqueList(CONST struct Instance *i)
{
  unsigned c, length;
  CONST struct Instance *clique;
  for(c=1, length = gl_length(g_clique_list); c <= length;c++){
    clique = gl_fetch(g_clique_list,c);
    if (InClique(i,clique)) return;
  }
  /* add to clique list */
  gl_append_ptr(g_clique_list,(VOIDPTR)i);
}

static
void AccParents(CONST struct Instance *i)
{
  unsigned long temp;
#if EXTRAPATHS
  unsigned long c;
  struct gl_list_t *plist;
#endif
  temp = NumberParents(i);
  if (temp < g_minimum_parents)  {
    g_minimum_parents = temp;
  }
  if (temp > g_maximum_parents) {
    g_maximum_parents = temp;
  }
  if (temp > 1 && !InstanceUniversal(i)) {
    g_extra_parents++;
    g_extra_parents_sum += (temp-1);
#if EXTRAPATHS
    plist = AllPaths(i);
    g_extra_paths += gl_length(plist);
    for(c=1;c <= gl_length(plist);c++){
       gl_free_and_destroy(gl_fetch(plist,c));
    }
    gl_destroy(plist);
#endif /* extrapaths */
  }
  g_total_parents += temp;
}

static
void AccStatistics(CONST struct Instance *i)
{
  unsigned long temp;
  CONST struct relation *rel;
  enum Expr_enum reltype;

  IncrementTypeCount(i);
  if (NextCliqueMember(i) != i)
    CheckCliqueList(i);
  switch(InstanceKind(i)){
  case MODEL_INST:
    g_num_complex_instances++;
    g_num_model_instances++;
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    g_model_bytes +=
      (sizeof(struct ModelInstance) + temp*sizeof(struct Instance *));
    break;
  case REAL_ATOM_INST:
    g_num_complex_instances++;
    g_num_atom_instances++;
    g_atom_bytes += GetByteSize(InstanceTypeDesc(i));
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    g_total_variables++;
    temp = RelationsCount(i);
    if (temp < g_minimum_relations) g_minimum_relations = temp;
    if (temp > g_maximum_relations) g_maximum_relations = temp;
    g_total_relations += temp;
    break;
  case BOOLEAN_ATOM_INST:
    g_num_complex_instances++;
    g_num_atom_instances++;
    g_atom_bytes += GetByteSize(InstanceTypeDesc(i));
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    break;
  case INTEGER_ATOM_INST:
    g_num_complex_instances++;
    g_num_atom_instances++;
    g_atom_bytes += GetByteSize(InstanceTypeDesc(i));
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    break;
  case SET_ATOM_INST:
    g_num_complex_instances++;
    g_num_atom_instances++;
    g_atom_bytes += GetByteSize(InstanceTypeDesc(i));
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    break;
  case SYMBOL_ATOM_INST:
    g_num_complex_instances++;
    g_num_atom_instances++;
    g_atom_bytes += GetByteSize(InstanceTypeDesc(i));
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    break;
  case REL_INST:
    g_num_relation_instances++;
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    /* count terms */
    rel = GetInstanceRelation(i,&reltype);
    if (rel&&(reltype==e_token)) {
      g_relation_terms += RelationLength(rel,0);
      g_relation_terms += RelationLength(rel,1);
      g_total_reals_in_rels += NumberVariables(rel);
    }
    break;
  case ARRAY_INT_INST:
    g_num_array_instances++;
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    g_total_array_children += temp;
    break;
  case ARRAY_ENUM_INST:
    g_num_array_instances++;
    AccParents(i);
    temp = NumberChildren(i);
    if (temp < g_minimum_children) g_minimum_children = temp;
    if (temp > g_maximum_children) g_maximum_children = temp;
    g_total_children += temp;
    g_total_array_children += temp;
    break;
  case REAL_CONSTANT_INST:
    g_num_constant_real++;
    g_num_constant_all++;
    break;
  case INTEGER_CONSTANT_INST:
    g_num_constant_int++;
    g_num_constant_all++;
    break;
  case BOOLEAN_CONSTANT_INST:
    g_num_constant_bool++;
    g_num_constant_all++;
    break;
  case SYMBOL_CONSTANT_INST:
    g_num_constant_sym++;
    g_num_constant_all++;
    break;
  case REAL_INST:
    g_num_atom_children++;
    break;
  case INTEGER_INST:
    g_num_atom_children++;
    break;
  case BOOLEAN_INST:
    g_num_atom_children++;
    break;
  case SET_INST:
    g_num_atom_children++;
    break;
  case SYMBOL_INST:
    g_num_atom_children++;
    break;
  case DUMMY_INST:
    g_num_unsel_instances++;
    break;
  case WHEN_INST:
  case LREL_INST:
    /* vicente we should be collecting info here */
    break;
  default:
    Asc_Panic(2, NULL, "Invalid arguments to AccStatistics.\n");
    break;
  }
}

static void AccBytes(CONST struct Instance *i)
{
  g_tree_bytes += InstanceSize(i);
}

/*
 * Does not account properly for shared external rels.
 */
static void AccTokenStatistics(CONST struct Instance *i)
{
  unsigned long p,len,llen,rlen;
  CONST struct relation *rel;
  enum Expr_enum t;
  CONST union RelationTermUnion *rside;
  switch(InstanceKind(i)){
  case REL_INST:
    g_num_relation_instances++;
    /* count terms */
    rel = GetInstanceRelation(i,&t);
    if (rel&&(t==e_token)) {
      g_total_reals_in_rels += NumberVariables(rel);

      /* count all the terms that would be evaluated */
      rside = PostFix_RhsSide(rel);
      rlen = RelationLength(rel,0);
      g_relation_terms += rlen;
      rside = PostFix_LhsSide(rel);
      llen = RelationLength(rel,1);
      g_relation_terms += llen;
      /* if not on list, add and count tokens in fine */
      if ( !gl_search(g_relation_guts,rside,(CmpFunc)CmpPtrs) ) {
        gl_append_ptr(g_relation_guts,(VOIDPTR)rside);
        len = llen;
        rside = PostFix_LhsSide(rel);
        for (p=0; p<len; p++) {
          t = RelationTermType(RelationSideTerm(rside,p));
          g_token_counts[t]++;
        }
        len = rlen;
        rside = PostFix_RhsSide(rel);
        for (p=0; p<len; p++) {
          t = RelationTermType(RelationSideTerm(rside,p));
          g_token_counts[t]++;
        }
      }
    }
    break;
  case MODEL_INST:
  case REAL_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case INTEGER_ATOM_INST:
  case SET_ATOM_INST:
  case SYMBOL_ATOM_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
  case REAL_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST:
    break;
  default:
    Asc_Panic(2, NULL, "Invalid arguments to AccTokenStatistics.\n");
    break;
  }
}

void InstanceTokenStatistics(FILE *f, CONST struct Instance *i)
{
  int j;
  if (i==NULL) return;
  g_relation_guts = gl_create(10000);
  for (j=0; j < NUM_EXPR_ENUMS; j++) g_token_counts[j] = 0;
  g_num_relation_instances = g_relation_terms = g_total_reals_in_rels = 0;
  SilentVisitInstanceTree((struct Instance *)i,
		    (void (*)(struct Instance *))AccTokenStatistics,1,0);
  FPRINTF(f,"Token relation statistics\n===================\n");
  FPRINTF(f,"Total number of relation instances:    %lu\n",
    g_num_relation_instances);
  FPRINTF(f,"Total number of 'unique' relations:    %lu\n",
    gl_length(g_relation_guts));
  FPRINTF(f,"Total number of relation terms:    %lu\n", g_relation_terms);
  FPRINTF(f,"Total number of reals in relation: %lu\n",
	  g_total_reals_in_rels);
  FPRINTF(f,"Number\t\tType of term:\n");
  g_relation_terms = 0;
  for (j=0; j < NUM_EXPR_ENUMS; j++) {
    if (g_token_counts[j]) {
      FPRINTF(f,"%lu\t\t%s\n", g_token_counts[j], ExprEnumName(j));
      g_relation_terms += g_token_counts[j];
    }
  }
  FPRINTF(f,"Total compiled tokens:\t%lu\n",g_relation_terms);
  FPRINTF(f,"===================\n");

  gl_destroy(g_relation_guts);
  g_relation_guts = NULL;
}

void InstanceStatistics(FILE *f, CONST struct Instance *i)
{
  struct TypeCount *count;
  unsigned long c, length;
  /*
   * initialize global variables
   */
  g_type_count_list = gl_create(50L);
  g_clique_list = gl_create(300L);
  g_num_complex_instances = g_num_atom_children =
    g_num_relation_instances = g_num_array_instances =
      g_num_unsel_instances = g_maximum_parents = g_total_parents =
	g_maximum_children = g_total_children = g_total_variables =
	  g_maximum_relations = g_total_relations = g_total_array_children =
	    g_relation_terms = g_total_reals_in_rels =
	      g_num_constant_sym = g_num_constant_real =
	        g_num_constant_bool = g_num_constant_int =
		 g_num_constant_all = g_num_model_instances =
		   g_num_atom_instances = g_model_bytes = g_atom_bytes =
		     g_tree_bytes = g_extra_parents = g_extra_parents_sum =
                       g_extra_paths =
	      0;
  g_minimum_parents = g_minimum_children = g_minimum_relations = UINT_MAX;

  VisitInstanceTree((struct Instance *)i,
		    (void (*)(struct Instance *))AccStatistics,1,1);
  SilentVisitInstanceTree((struct Instance *)i,
                          (void (*)(struct Instance *))AccBytes,0,0);

  FPRINTF(f,"General Instance Tree Numbers\n=============================\n");
  FPRINTF(f,"Number of models and complex atoms: %lu\n",
	  g_num_complex_instances);
  FPRINTF(f,"Number of models: %lu (%lu bytes)\n",
	  g_num_model_instances,g_model_bytes);
  FPRINTF(f,"Number of atoms: %lu (%lu bytes)\n",
	  g_num_atom_instances,g_atom_bytes);
  FPRINTF(f,"Number of atom children instances: %lu\n",
	  g_num_atom_children);
  FPRINTF(f,"Number of constant instances: %lu\n",
	  g_num_constant_all);
  if (FindRelationType() !=NULL) {
    length = GetByteSize(FindRelationType());
  } else {
    length = 0;
  }
  FPRINTF(f,"Number of relation heads: %lu (%lu bytes)\n",
    g_num_relation_instances, length*g_num_relation_instances);
  FPRINTF(f,"Number of array instances: %lu\n",
    g_num_array_instances);
  FPRINTF(f,"Number of unselected instances: %lu\n",
    g_num_unsel_instances);
  FPRINTF(f,"TOTAL INSTANCES: %lu\n",
	  g_num_complex_instances + g_num_atom_children +
	  g_num_relation_instances + g_num_relation_instances +
	  g_num_array_instances + g_num_constant_all);
  FPRINTF(f,"TOTAL BYTES (neglecting shared internals): %lu\n", g_tree_bytes);

  FPRINTF(f,"Instance number by type\n=======================\n");
  length = gl_length(g_type_count_list);
  for(c=1; c <= length;c++){
    count = gl_fetch(g_type_count_list,c);
    if (count){
      FPRINTF(f,"%-40s %lu\n",count->name,(unsigned long)count->count);
    } else {
      FPRINTF(f,"NULL pointer in type count list\n");
    }
  }
  FPRINTF(f,"Parental statistics\n===================\n");
  FPRINTF(f,"Minimum number of parents: %u\n",g_minimum_parents);
  FPRINTF(f,"Maximum number of parents: %u\n",g_maximum_parents);
  FPRINTF(f,"Average number of parents: %g\n",
	  (double)g_total_parents/
	  (double)NONZERO(g_num_complex_instances + g_num_array_instances));
  FPRINTF(f,"Instances with nP > 1: %u\n",g_extra_parents);
  FPRINTF(f,"Total extra parents: %u\n",g_extra_parents_sum);
#if EXTRAPATHS
  FPRINTF(f,"Total extra paths: %u\n",g_extra_paths);
#endif /* extrapaths */

  FPRINTF(f,"Children statistics\n===================\n");
  FPRINTF(f,"Minimum number of children: %u\n",g_minimum_children);
  FPRINTF(f,"Maximum number of children: %u\n",g_maximum_children);
  FPRINTF(f,"Average number of children: %g\n",
	  (double)g_total_children/
	  (double)NONZERO(g_num_complex_instances + g_num_array_instances +
			  g_num_relation_instances));

  FPRINTF(f,"Clique statistics\n=================\n");
  FPRINTF(f,"Number of cliques: %lu\n",gl_length(g_clique_list));
  FPRINTF(f,"Minimum clique size: %u\n",MinimumCliqueSize(g_clique_list));
  FPRINTF(f,"Maximum clique size: %u\n",MaximumCliqueSize(g_clique_list));
  FPRINTF(f,"Average clique size: %g\n",AverageCliqueSize(g_clique_list));

  FPRINTF(f,"Variable statistics\n===================\n");
  FPRINTF(f,"Number of complex reals: %u\n",g_total_variables);
  FPRINTF(f,"Minimum number of relations per real: %u\n",
	  g_minimum_relations);
  FPRINTF(f,"Maximum number of relations per real: %u\n",
	  g_maximum_relations);
  FPRINTF(f,"Average number of relations per real: %g\n",
	  (double)g_total_relations/(double)NONZERO(g_total_variables));

  FPRINTF(f,"Constant statistics\n===================\n");
  FPRINTF(f,"Number of constant reals:\t %lu\n",g_num_constant_real);
  FPRINTF(f,"Number of constant booleans:\t %lu\n",g_num_constant_bool);
  FPRINTF(f,"Number of constant integers:\t %lu\n",g_num_constant_int);
  FPRINTF(f,"Number of constant symbols:\t %lu\n",g_num_constant_sym);

  FPRINTF(f,"Relation statistics\n===================\n");
  FPRINTF(f,"Total number of relation terms:    %lu\n", g_relation_terms);
  FPRINTF(f,"Total number of reals in relation: %lu\n",
	  g_total_reals_in_rels);

  FPRINTF(f,"Array statistics\n================\n");
  FPRINTF(f,"Average children per array node: %g\n",
	  (double)g_total_array_children /
	  (double)NONZERO(g_num_array_instances));

  gl_destroy(g_clique_list);
  gl_free_and_destroy(g_type_count_list);
}
