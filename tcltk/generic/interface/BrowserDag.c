/*
 *  BrowserDag.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.15 $
 *  Version control file: $RCSfile: BrowserDag.c,v $
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


#include <tcl.h>
#include <tk.h>
#include "config.h"
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <compiler/instance_enum.h>
#include <system/slv_client.h>
#include "BrowserQuery.h"
#include "BrowserDag.h"
#include <general/mathmacros.h>

#ifndef lint
static CONST char BrowserDagID[] = "$Id: BrowserDag.c,v 1.15 2003/08/23 18:43:04 ballan Exp $";
#endif


/* OLD */
/*
 *
 * This is the original version of the code.
 * It may be going away soon.
 *
 */
static Tcl_Interp *local_interp = NULL;
static struct InstanceName global_name;

int BrowWriteName_ToInterp(Tcl_Interp *interp,
                           CONST struct InstanceName *rec)
{
  char tmp[256];
  switch(InstanceNameType(*rec)) {
  case IntArrayIndex:
    sprintf(tmp,"[%ld]",InstanceIntIndex(*rec));
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    break;
  case StrArrayIndex:
    sprintf(tmp,"'%s'",InstanceStrIndex(*rec));
    Tcl_AppendResult(interp,tmp,(char *)NULL);
    break;
  case StrName:
    Tcl_AppendResult(interp,InstanceNameStr(*rec),(char *)NULL);
    break;
  }
  return TCL_OK;
}


void BrowSpecialVisitTree(struct Instance *inst)
{
  unsigned long nch,c;
  struct InstanceName childname;
  struct Instance *child;
  if (inst) {
    nch = NumberChildren(inst);
    switch(InstanceKind(inst)) {
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      if (BrowIsAtomicArray(inst)||(BrowIsRelation(inst))) {
        break;
      }
      /* fall through */
    case MODEL_INST:
      Tcl_AppendResult(local_interp,"{",(char *)NULL); /* write name */
      (void)BrowWriteName_ToInterp(local_interp,&global_name);
      if (nch) {
        Tcl_AppendResult(local_interp," {",(char *)NULL);
        for (c=1;c<=nch;c++) {
          child = InstanceChild(inst,c);
          if (child) {
            if (!BrowIsAtomicArray(child) && !BrowIsRelation(child)) {
              childname = ChildName(inst,c);
              (void)BrowWriteName_ToInterp(local_interp,&childname);
            }
            Tcl_AppendResult(local_interp," ",(char *)NULL);
          }
        }
        Tcl_AppendResult(local_interp,"}} ",(char *)NULL);
      }
      break;
    default:
      break;
    }
    for(c=1;c<=nch;c++) {
      if (child = InstanceChild(inst,c)) {
        global_name = ChildName(inst,c);
        BrowSpecialVisitTree(child);
      } else {
        FPRINTF(stderr,"Null instance in tree ???\n");
      }
    }
  }
}


int Asc_BrowTreeListCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  struct Instance *i;
  struct gl_list_t *list;
  if ( argc != 3 ) {
    Tcl_SetResult(interp,
                  "wrong # args: Usage __brow_tree_list ?current?search  name",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    i = g_search_inst;
  }
  if (!i) {
    Tcl_SetResult(interp, "requested instance is NULL", TCL_STATIC);
    return TCL_ERROR;
  }
  local_interp = interp;
  SetInstanceNameStrPtr(global_name,argv[2]);
  SetInstanceNameType(global_name,StrName);
  BrowSpecialVisitTree(i);
  local_interp = NULL;
  return TCL_OK;
}


/* NEW */
/*
 * BrowTree Commands.
 * This is the new version of the code. It kicks out
 * copy numbers. It may replace the original verion
 * of the code.
 */

#define AVG_PARENTS 2
#define AVG_CHILDREN 4
#define AVG_RELATIONS 15
#define AVG_GROWTH 2
#define PART_THRESHOLD 1000

/*
 * Global Data.
 * g_dagdata is the main structure. It is static permanent
 * structure to facilitate, multiple queries of the data.
 * It must firt be prepared, and then later shutdown to
 * free whatever memory is associated with it.
 */

enum DagVisitSequence {
  v_df_bu = 0,	/* depth first bottom up */
  v_df_td,	/* depth first top down */
  v_bf_bu,	/* breadth first bottom up */
  v_bf_td	/* breadth first top down */
};

enum PartitionMethod {
  p_clusterup = 0,	/* bottom up clustering */
  p_clusterdown, 	/* top down clustering */
  p_bisection,		/* as the name says */
  p_kwaypart		/* k-way partitioning */
};


struct DagData {
  struct gl_list_t *instances;
  struct gl_list_t *models;
  struct gl_list_t *relations;
  struct gl_list_t *counts;
  struct gl_list_t *partitions;
  struct gl_list_t *tears;
  unsigned long threshold;
  unsigned long last_used_index; /* index of last partition seen */
  enum DagVisitSequence visit;
  enum PartitionMethod p_method;
  int initialized;
};

static struct DagData g_dagdata_reset = {
  NULL,		/* instances */
  NULL,		/* models */
  NULL,		/* relations */
  NULL,		/* counts */
  NULL,		/* partitions */
  NULL,		/* tears */
  0,		/* threshold */
  1,		/* index */
  v_df_bu,	/* this is the default visit sequence */
  p_clusterup,	/* default partitioning */
  0		/* intialized */
};

static struct DagData g_dagdata;

struct SlvModel {
  struct Instance *instance;
  unsigned long index;
  unsigned long level;
  unsigned long local;
  unsigned long subtree;
  unsigned long partition;
  struct gl_list_t *parents;	/* a list of parents */
  struct gl_list_t *children;
  struct gl_list_t *relations;	/* a list of lists of relations */
};

static struct SlvModel *CreateSlvModel(void)
{
  struct SlvModel *result;

  result = (struct SlvModel *)ascmalloc(sizeof(struct SlvModel));
  result->instance = NULL;
  result->index = 0;
  result->level = 0;
  result->local = 0;
  result->subtree = 0;
  result->partition = 1;	/* the default partition */
  result->parents = gl_create(AVG_PARENTS);
  result->children = gl_create(AVG_CHILDREN);
  result->relations = gl_create(AVG_GROWTH);
  return result;
}

static
void DestroySlvModel(struct SlvModel *model)
{
  unsigned long len,c;
  struct gl_list_t *tmp;

  if (!model) {
    return;
  }
  if (model->parents) {
    gl_destroy(model->parents);
  }
  if (model->children) {
    if (gl_length(model->children)==0) {
      DoSlvModel_BreakPoint();
    }
    gl_destroy(model->children);
  }
  if (model->relations) {
    len = gl_length(model->relations);
    for (c=1;c<=len;c++) {
      tmp = (struct gl_list_t *)gl_fetch(model->relations,c);
      gl_destroy(tmp);
    }
    gl_destroy(model->relations);
  }
  ascfree((char *)model);
  return;
}

static
void DestroyModelList(struct gl_list_t *models)
{
  unsigned long len,c;
  struct SlvModel *model;

  if (!models) {
    return;
  }
  len = gl_length(models);
  for (c=1;c<=len;c++) {
    model = (struct SlvModel *)gl_fetch(models,c);
    DestroySlvModel(model);
  }
  gl_destroy(models);
}

static
void ResetDagData(void)
{
  g_dagdata = g_dagdata_reset;	/* structure assignment */
}

static
void DestroyDagData(void)
{
  unsigned long len,c;
  struct SlvModel *model;

  if (g_dagdata.instances) {
    gl_destroy(g_dagdata.instances);
    g_dagdata.instances = NULL;
  }

  if (g_dagdata.models) {
    DestroyModelList(g_dagdata.models);
    g_dagdata.models = NULL;
  }

  if (g_dagdata.relations) {
    gl_destroy(g_dagdata.relations);
    g_dagdata.relations = NULL;
  }

  if (g_dagdata.counts) {
    gl_destroy(g_dagdata.counts);
  }
  ResetDagData();
}


/*
 * This code is here for debugging purposes only.
 */
int DoSlvModel_BreakPoint(void)
{
  return 1;
}

/*
 *********************************************************************
 * CountRelations
 *
 * The next few functions are concerned with the counting of relations.
 * Some of these functions run of the original instance structure,
 * whilst others work off the SolverModel.
 *********************************************************************
 */

/*
 * Given an instance, which is assumed to be able to respond to
 * a NumberChildren query, this function will count the number of
 * relations associated with instance. It is recursive as, a given
 * model instance has ownership of relations in a number of ways.
 * 1) a REL_INST may be a direct child of the model.
 * 2) the REL_INST may be a child of an array, (or multiply dimensioned
 *    array, which is the child of the model.
 *
 * CountLocalRelations__ is used by CountLocalRelations, which returns
 * a list of counts foreach model in an instance list.
 */
static
void CountLocalRelations__(struct Instance *inst, unsigned long *count)
{
  struct Instance *child;
  unsigned long nch,c;

  if (!inst) {
    return;
  }
  nch = NumberChildren(inst);
  for (c=1;c<=nch;c++) {
    child = InstanceChild(inst,c);
    if (!child) {
      continue;
    }
    switch (InstanceKind(child)) {
    case REL_INST:
      (*count)++;
      break;
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      if (BrowIsRelation(child)) {	/* an array of relations */
        CountLocalRelations__(child,count);
      }
      break;
    default:
      break;
    }
  }
}


/*
 * Creates and returns a list of relation counts for each
 * instance on the given list. Uses the primitive CountLocalRelations__
 */
static
struct gl_list_t *CountLocalRelations(struct gl_list_t *inst_list)
{
  struct gl_list_t *list;
  struct Instance *inst;
  unsigned long len,c;
  unsigned long count;
  unsigned long total = 0;	/* KAA_DEBUG */

  len = gl_length(inst_list);
  list = gl_create(len);
  for (c=1;c<=len;c++) {
    count = 0;
    inst = (struct Instance *)gl_fetch(inst_list,c);
    CountLocalRelations__(inst,&count);
    gl_append_ptr(list,(char *)count);	/* assumes pointer == unsigned long */
    total += count;
  }
  FPRINTF(stderr,"Found %lu relations\n",total);
  return list;
}

/*
 * This function is very similar to CountLocalRelations, but
 * appends each relation found to the given rellist. It has
 * been made a separate function for efficiency, HOWEVER
 * they perhaps be rationalized; any one who simply wants a
 * count simply builds the list and then queries the length.
 * I WILL CLEAN THIS UP LATER.
 */
static
void CollectLocalRelations(struct Instance *inst,
                           struct gl_list_t *rellist)
{
  struct Instance *child;
  unsigned long nch,c;

  if (!inst) {
    return;
  }
  nch = NumberChildren(inst);
  for (c=1;c<=nch;c++) {
    child = InstanceChild(inst,c);
    if (!child) {
      continue;
    }
    switch (InstanceKind(child)) {
    case REL_INST:
      gl_append_ptr(rellist,(char *)child);
      break;
    case ARRAY_ENUM_INST:
    case ARRAY_INT_INST:
      if (BrowIsRelation(child)) {	/* an array of relations */
        CollectLocalRelations(child,rellist);
      }
      break;
    default:
      break;
    }
  }
}


/*
 * These functions are tot be used when the model tree has already been
 * built, and the local relations have been filled in.
 */

static
unsigned long CountRelnsInList_Fast(struct gl_list_t *models,
                                    unsigned long start,
                                    unsigned long end)
{
  struct SlvModel *model;
  unsigned long c;
  unsigned long total = 0L;

  for (c=start;c<=end;c++) {
    model = (struct SlvModel *)gl_fetch(models,c);
    total += model->local;
  }
  return total;
}



/*
 *********************************************************************
 * Auxiliary Functions
 *
 * The next few functions are auxiliary functions that may be applied
 * to a tree/list of solver models. These include
 * 1) Labelling levels in the tree.
 * 2) Counting aliased variables.
 * 3) RemovingAliasing using different heuristics.
 * 4) Tree Reorienting routines.
 *********************************************************************
 */
static
void LabelLevels(struct SlvModel *root, int *level)
{
  struct SlvModel *modchild;
  unsigned long nch,c;

  if (!root) {
    return;
  }
  nch = gl_length(root->children);
  if (nch) {
    (*level)++;
    for (c=1;c<=nch;c++) {
      modchild = (struct SlvModel *)gl_fetch(root->children,c);
      LabelLevels(modchild,level);
    }
    (*level)--;
  } else {
    root->level = MAX(root->level,*level);
  }
}

/*
 * This function returns :
 * 1  if p1 has *less* children than p2
 * -1 if p2 has *more* children than p1
 * 0  if equal.
 */
int ReorientCriteria_1(void *p1, void *p2)
{
  struct SlvModel *child1 = (struct SlvModel *)p1;
  struct SlvModel *child2 = (struct SlvModel *)p2;

  if (gl_length(child1->children) < gl_length(child2->children)) {
    return 1;
  } else if (gl_length(child1->children) > gl_length(child2->children)) {
    return -1;
  } else {
    return 0;
  }
}

/*
 * This function attempts to reorient a SlvModel tree, such
 * that nodes with more children appear before nodes with less
 * children.
 *
 * NOTE: After calling this function, anyone with references to
 * models in the tree *will* still have valid references.
 * However the indexing, whether bottom up or top down or breadth first
 * rather than depth first will be messed up. The tree then will need
 * to be reindexed.
 */
static
void ReorientTree(struct SlvModel *root)
{
  struct SlvModel *child;
  unsigned long len,c;

  if (!root) {
    return;
  }
  gl_sort(root->children,(CmpFunc)ReorientCriteria_1);
  len = gl_length(root->children);
  for (c=1;c<=len;c++) {
    child = (struct SlvModel *)gl_fetch(root->children,c);
    ReorientTree(child);
  }
}

/*
 * This fuction should be called before any of the Indexing functions are
 * called. Otherwise, later code will die. The reason is simple:
 * the indexing code depends upon unique labeling; we use the label of 0,
 * to find out if a node has been visited or not. If the labeling gets
 * messed up (in particular if we end up with a label greater than
 * the length of the original list, then the PostFixup code will choke.
 */
static
void ResetModelTree(struct gl_list_t *models)
{
  struct SlvModel *model;
  unsigned long len,c;

  len = gl_length(models);
  for (c=1;c<=len;c++) {
    model = (struct SlvModel *)gl_fetch(models,c);
    model->index = 0;
  }
}

/*
 * This function does a Depth First Bottom up indexing of the model tree.
 * Because we may have aliasing in the tree, we need to see if a node has
 * been visited before we index it. This function thus assumes that all
 * the indexes in the tree have been reset to 0. A nonzero index means
 * that the node has already been visited, so that we dont reindex it.
 */
static
void IndexTreeDF_BottomUp(struct SlvModel *model, unsigned long *index)
{
  struct SlvModel *child;
  unsigned long nch,c;

  if (!model) {
    return;
  }
  nch = gl_length(model->children);
  for (c=1;c<=nch;c++) {
    child = (struct SlvModel *)gl_fetch(model->children,c);
    IndexTreeDF_BottomUp(child,index);
  }
  if (!model->index) {		/* do the visitation check */
    model->index = (*index)++;	/* index the node */
  }
}

/*
 * This function does a Depth First Top Down indexing
 * of the model tree. See the comments regarding aliasing
 * for IndexTreeDF_BottomUp.
 */
static
void IndexTreeDF_TopDown(struct SlvModel *model, unsigned long *index)
{
  struct SlvModel *child;
  unsigned long nch,c;

  if (!model) {
    return;
  }
  nch = gl_length(model->children);
  if (!model->index) {		/* do the visitation check */
    model->index = (*index)++;
  }
  for (c=1;c<=nch;c++) {
    child = (struct SlvModel *)gl_fetch(model->children,c);
    IndexTreeDF_TopDown(child,index);
  }
}

static
void IndexTree(struct SlvModel *model, enum DagVisitSequence visit)
{
  unsigned long index = 1;
  switch (visit) {
  case v_df_bu:
  case v_bf_bu:
    IndexTreeDF_BottomUp(model,&index);
    break;
  case v_df_td:
  case v_bf_td:
    IndexTreeDF_TopDown(model,&index);
    break;
  default:
    FPRINTF(stderr,"Unknown visit sequence\n");
    return;
  }
}

static
void PostIndexingFixup(struct SlvModel *root,
                       struct gl_list_t *models)
{
  struct SlvModel *child;
  unsigned long nch,c;

  if (!root) {
    return;
  }
  nch = gl_length(root->children);
  for (c=1;c<=nch;c++) {
    child = (struct SlvModel *)gl_fetch(root->children,c);
    /*
    assert((child->index >=1) && (child->index <= gl_length(models)));
    */
    if ((child->index < 1) || (child->index > gl_length(models))) {
      FPRINTF(stderr,"Corrupted data in PostIndexingFixup at %lu\n",c);
      continue;
    }
    gl_store(models,child->index,(char *)child);
    PostIndexingFixup(child,models);
  }
}

/*
 *********************************************************************
 * Partitioning Routines
 *
 * Partitions are simply a list of relations. By storing the start
 * and end models, which maintain the relation information we have
 * enough information to define a partition.

 * This code uses a list of Solver Models and attempts to create
 * partitions of relations. A number of different partitioning
 * algorithms may be used.
 *
 * 1)	PartitionBU_1 -- by doing a special ordering of the model nodes
 *    	we simply run accumulator over the number of local relations.
 *	We mark of partitions as we pass some threshold value. This
 * 	is essentially a stack of models. We continue pushing models
 * 	onto the stack on the threshold has been reached, then we
 * 	pop the entire thing off the stack and label it as being in
 *	the same partition.
 *
 * 2)	PartitionTD_1 -- the same algorithm as PartitionBU_1 but we
 * 	collect the partitions by working from the end of the list
 *	provided.
 *
 *********************************************************************
 */

struct Partition {
  unsigned long start;
  unsigned long end;
  unsigned long count;
  unsigned long index;
  struct Partition *left;	/* Used in the bisect partition */
  struct Partition *right;
};

struct Partition *CreatePartition(unsigned long start,
                                  unsigned long end)
{
  struct Partition *result;

  result = (struct Partition *)ascmalloc(sizeof(struct Partition));
  assert(result);
  result->start = start;
  result->end = end;
  result->count = 0L;
  result->index = 0L;
  result->left = NULL;
  result->right = NULL;
  return result;
}

/*
 * start and end refer to the start and end indexes of the sublist
 * of models on the main model list. All models on the list
 * between and including start and end will be processed.
 */
struct gl_list_t *PartitionBU_1(struct gl_list_t *models,
                                unsigned long threshold,
                                unsigned long start,
                                unsigned long end)

{
  struct SlvModel *model;
  struct gl_list_t *partitionlist;
  struct Partition *p;
  long len;
  unsigned long c;
  unsigned long accum, partition;

  accum = 0;
  partition = 1;
  len = end - start + 1;	/* just to get an estimate */

  /*
   * Create the partiton list; then create and add the first
   * partition to the this list. The initial partition is the
   * size of the entire problem. Successive partitions shrink.
   */
  partitionlist = gl_create((unsigned long)(len*0.1 + 1));
  p = CreatePartition(start,end);
  gl_append_ptr(partitionlist,(char *)p);

  for (c=start;c<=end;c++) {
    model = (struct SlvModel *)gl_fetch(models,c);
    accum += model->local;
    model->partition = partition;
    if (accum >= threshold) {
      p->count = accum;
      p->end = c;	/* finish up the previous partition */
      /*
       * Set up for next partition. Make sure that we are not
       * at the end of the list of models.
       */
      if (c!=end) {
        p = CreatePartition(c+1,end);	/* start the next partition */
        gl_append_ptr(partitionlist,(char *)p);
        partition++;
        accum = 0;
      }
    }
  }
  FPRINTF(stderr,"%lu partitions were found\n",
          gl_length(partitionlist));
  return partitionlist;
}

struct gl_list_t *PartitionTD_1(struct gl_list_t *models,
                                unsigned long threshold,
                                unsigned long start,
                                unsigned long end)
{
  struct SlvModel *model;
  struct gl_list_t *partitionlist;
  struct Partition *p;
  long len;
  unsigned long c;
  unsigned long accum, partition;

  accum = 0;
  partition = 1;
  len = end - start + 1;	/* just to get an estimate */

  /*
   * Create the partiton list; then create and add the first
   * partition to the this list. The initial partition is the
   * size of the entire problem. Successive partitions shrink.
   */
  partitionlist = gl_create((unsigned long)(len*0.1 + 1));
  p = CreatePartition(start,end);
  gl_append_ptr(partitionlist,(char *)p);

  for (c=end;c>=start;c--) {
    model = (struct SlvModel *)gl_fetch(models,c);
    accum += model->local;
    model->partition = partition;
    if (accum >= threshold) {
      p->count = accum;
      p->start = c;	/* finish up the previous partition */
      /*
       * Set up for next partition. Make sure that we are not
       * at the top of the list of models.
       */
      if (c!=start) {
        p = CreatePartition(start,c-1);	/* start the next partition */
        gl_append_ptr(partitionlist,(char *)p);
        partition++;
        accum = 0;
      }
    }
  }
  FPRINTF(stderr,"%lu partitions were found\n",
          gl_length(partitionlist));
  return partitionlist;
}

struct gl_list_t *Partition_1(struct gl_list_t *models,
                              unsigned long threshold,
                              enum PartitionMethod p_method,
                              unsigned long start, unsigned long end)
{
  struct gl_list_t *partitionlist = NULL;

  switch (p_method) {
  case p_clusterdown:
    partitionlist = PartitionTD_1(models,threshold,start,end);
    break;
  case p_clusterup:
  default:
    partitionlist = PartitionBU_1(models,threshold,start,end);
    break;
  }
  return partitionlist;
}


/*
 *********************************************************************
 * BisectPartition
 *
 * This code performs a bisection of the model list using the local
 * local relation counts as the criteria. It essentially
 * accumulates the count until a cutoff is reached. This forms the
 * 'left' partition. The rest of the models and their respective
 * relations form the 'right' partition.
 *
 * We then do a bottom up visitation of the partition tree (LRV)
 * to label it and simultaneously collect the partitions into a
 * partitionlist. Once this is done all the other partition code is
 * then applicable.
 *********************************************************************
 */
static
void BisectPartition(struct Partition *root,
                     struct gl_list_t *models,
                     unsigned long cutoff)
{
  struct SlvModel *model;
  struct Partition *left, *right;
  unsigned long c,accum = 0;
  unsigned long threshold;

  if (!root) {
    return;
  }
  if (root->start==root->end) {
    return;
  }
  threshold = (root->count / 2);
  if (threshold < cutoff) {
    return;
  }

  for (c=root->start;c<=root->end;c++) {
    model = (struct SlvModel *)gl_fetch(models,c);
    accum += model->local;
    if (accum >= threshold) {
      root->left = CreatePartition(root->start,c);
      root->left->count = accum;
      root->right = CreatePartition(c+1,root->end);
      root->right->count = MAX(0,(root->count - accum));
      BisectPartition(root->left,models,cutoff);	/* visit left */
      BisectPartition(root->right,models,cutoff);	/* visit right */
      break;
    }
  }
}

static
void LabelPartitions_2(struct Partition *root,
                       struct gl_list_t *partitionlist)
{
  if (root) {				/* LRV -- i.e. postorder */
    LabelPartitions_2(root->left,partitionlist);
    LabelPartitions_2(root->right,partitionlist);
    gl_append_ptr(partitionlist,(char *)root);
    root->index = gl_length(partitionlist);
  }
}

/*
 * The partition list for a bisection partition contains
 * partition of different granularity. It is the leaf partitions
 * that matter, and these leaves contain the information necessary
 * to label their models. We do that here.
 * NOTE: We could possibly be smarter about labelling of models.
 * so that we dont have to scan the entire model list. I need to
 * think about this some more. This could almost surely be done
 * while we are doing the BisectPartition.
 */
static
int IsLeafPartition(struct Partition *part)
{
  if ((part->left==NULL) && (part->right==NULL)) {
    return 1;
  } else {
    return 0;
  }
}

static
void LabelLeafPartitions(struct gl_list_t *partitionlist,
                         struct gl_list_t *models)
{
  struct Partition *part;
  struct SlvModel *model;
  unsigned long len,c,cc;
  unsigned long partition,start,end;

  len = gl_length(partitionlist);
  for (c=1;c<=len;c++) {
    part = (struct Partition *)gl_fetch(partitionlist,c);
    if (IsLeafPartition(part)) {/* label the models in the leaves */
      partition = part->index;
      start = part->start;
      end = part->end;
      for (cc=start;cc<=end;cc++) {
        model = (struct SlvModel *)gl_fetch(models,cc);
        model->partition = partition;
      }
    }
  }
}

static
struct gl_list_t *Partition_2(struct gl_list_t *models,
                              unsigned long cutoff)
{
  struct Partition *root;
  struct gl_list_t *partitionlist = NULL;
  unsigned long len,c;

  len = gl_length(models);
  if (!len) {
    partitionlist = gl_create(1L);
    return partitionlist;
  }
  /*
   * the length of the partitionlist should be a log function
   */
  partitionlist = gl_create((0.05*len) + 1);
  root = CreatePartition(1,len);
  root->count = CountRelnsInList_Fast(models,1,len);
  BisectPartition(root,models,cutoff);
  LabelPartitions_2(root,partitionlist);
  LabelLeafPartitions(partitionlist,models);
  FPRINTF(stderr,"\t%lu partitions were found by bisection\n",
          gl_length(partitionlist));
  return partitionlist;
}

/*
 * These functions will mark the relations in a list as
 * being in the given partition. This could be made smarter,
 * possibly by the use of a master relations list. We also
 * mark the relation as not being torn.
 */
static
void MarkRelationList(struct gl_list_t *relations,
                      unsigned long partition)
{
  struct Instance *relinst;
  struct rel_relation *rel;
  unsigned long len,c;

  len = gl_length(relations);
  if (!len) {
    return;
  }

  for (c=1;c<=len;c++) {	/* Put some sanity checking here FIX */
    relinst = (struct Instance *)gl_fetch(relations,c);
    rel = (struct rel_relation *)GetInterfacePtr(relinst);
    rel_set_flagbit(rel,REL_PARTITION,(int)partition);
    rel_set_flagbit(rel,REL_COUPLING,rel_interfacial(rel));
/*    rel->partition = (int)partition;*/	/* FIX FIX FIX dont deference */
/*    rel->coupling = rel_interfacial(rel);*/	/* FIX FIX */
  }
}


static
void MarkRelnPartitions(struct gl_list_t *models)
{
  struct SlvModel *model;
  struct gl_list_t *tmp;
  unsigned long len,c,cc;
  unsigned long nrelsets, partition;

  len = gl_length(models);
  if (!len) {
    return;
  }
  for (c=1;c<=len;c++) {
    model = (struct SlvModel *)gl_fetch(models,c);
    nrelsets = gl_length(model->relations);
    for (cc=1;cc<=nrelsets;cc++) {
      tmp = (struct gl_list_t *)gl_fetch(model->relations,cc);
      MarkRelationList(tmp,model->partition);
    }
  }
}


/*
 *********************************************************************
 * Coupling Relations
 *
 * rel_interfacial is an expensive query, so we cache the
 * info here. We accept a relation if its signature is less
 * than the filter. e.g, if the filter is 1, and the relation
 * has a signature of 100, it would be ignored.
 *********************************************************************
 */
static
void MarkCouplingRelations(struct rel_relation **rp, int nrels,
                           int filter)
{
  struct rel_relation *rel;
  int signature;
  int i;

  for (i=0;i<nrels;i++) {
    rel = rp[i];
    signature = rel_interfacial(rel);
    if (signature <= filter) {
        rel_set_flagbit(rel,REL_COUPLING,1);
    } else {
        rel_set_flagbit(rel,REL_COUPLING,0);
    }
  }
}

int WriteCouplingRelnsToInterp(Tcl_Interp *interp,
                               struct rel_relation **rp,int nrels,
                               int filter)
{
  struct rel_relation *rel;
  int i;
  char tmp[64];

  if (!rp) {
    Tcl_SetResult(interp, "No relation list given", TCL_STATIC);
    return TCL_ERROR;
  }
  MarkCouplingRelations(rp,nrels,filter);	/* mark them first */
  for (i=0;i<nrels;i++) {
    rel = rp[i];
    if (rel_coupling(rel)) {
      sprintf(tmp,"%d",rel_mindex(rel));
      Tcl_AppendResult(interp," ",tmp," ",(char *)NULL);
    }
  }
  return 0;
}


static
void WritePartitions(struct gl_list_t *partitionlist)
{
  struct Partition *part;
  unsigned long len,c;

  len = gl_length(partitionlist);
  for (c=1;c<=len;c++) {
    part = (struct Partition *)gl_fetch(partitionlist,c);
    FPRINTF(stderr,"Partition %3lu: %4lu -> %4lu: size = %lu",
            part->index, part->start, part->end, part->count);
    if (IsLeafPartition(part)) {
      FPRINTF(stderr," *** \n");
    } else {
      FPRINTF(stderr,"\n");
    }
  }
}


/*
 *********************************************************************
 * Tearing Routines
 *
 * This code implements the labelling of the variables that need to be
 * torn. The algorithm is simple: for any given variable, find out how
 * many partitions that it exists in. If this count is more than 1,
 * it is a candidate variable to be torn.
 *********************************************************************
 */

/*
 * KAA_DEBUG. fetching the struct rel_relation *from a rel_instance NEEDS
 * a function and/or macro.
 */

/*
 * Determine the socalled home partition of the variable. This will
 * be the partition of the first relation that it is incident upon.
 * The first relation that is not in the home partition means that
 * the variable is a tear. Return TRUE if the variable is a tear;
 * false otherwise. The possibility of using some heuristics, such
 * as working from the end of the relations list back towards the
 * front exists, and may be investigated in a late iteration.
 *
 */

/*
 * The var_index is for debugging purposes only.
 */
static
int IsTearVariable_1(struct Instance *var, int var_sindex)
{
  struct Instance *compiler_reln;
  struct rel_relation *rel;
  unsigned long nrels, i;
  int homepartition;
  int tear = 0;

  nrels = RelationsCount(var);
  if (nrels <= 1) {		/* a singleton var -- never a tear */
    return 0;
  }

  compiler_reln = RelationsForAtom(var,1);
  rel = (struct rel_relation *)GetInterfacePtr(compiler_reln); /* FIX */
  assert(rel);
  homepartition = rel_partition(rel);

  for (i=2;i<=nrels;i++) {
    compiler_reln = RelationsForAtom(var,i);
    rel = (struct rel_relation *)GetInterfacePtr(compiler_reln); /* FIX */
    if (rel_partition(rel)!=homepartition) { /* we have a tear */
      tear = 1;
      return 1;
    }
  }

  return 0;	/* if we are here then it is not a tear */
}

static
struct gl_list_t *MarkTearVariables(struct var_variable **vp, int nvars)
{
  struct Instance *var;
  struct gl_list_t *tears;
  int j;

  if (nvars==0) {
    tears = gl_create(1L);
    return tears;
  } else {
    tears = gl_create((unsigned long)(0.05*nvars + 1)); /* FINE TUNE */
  }

  for (j=0;j<nvars;j++) {
    if (var_incident(vp[j])) {
      var = var_instance(vp[j]);
      if (IsTearVariable_1(var,var_sindex(vp[j]))) {
        gl_append_ptr(tears,var);
      }
    }
  }
  return tears;
}


/*
 * This function assumes that a REAL_ATOM_INST is synonymous with
 * a var !! FIX . KIRKBROKEN
 */
static
void WriteTearVarsToInterp(Tcl_Interp *interp,struct gl_list_t *tears)
{
  struct Instance *var;
  char tmp[64];
  unsigned long len,c;
  int index;

  len = gl_length(tears);
  for (c=1;c<=len;c++) {
    var = (struct Instance *)gl_fetch(tears,c);
    index = var_sindex(var);				/* FIX */
    sprintf(tmp,"%d ",index);
    Tcl_AppendResult(interp,tmp,(char *)NULL);
  }
}

/*
 * This is some debugging code. It may cleaned up or better
 * yet removed.
 */
static
void WriteModelData(struct gl_list_t *models)
{
  FILE *fp;
  struct SlvModel *model;
  unsigned long len,c;

  fp = fopen("modeldata.tmp","w");
  if (!fp) {
    return;
  }
  len = gl_length(models);
  for (c=1;c<=len;c++) {
    model = (struct SlvModel *)gl_fetch(models,c);
    if (!model) {
      FPRINTF(stderr,"Corrupted model data found !!\n");
    }
    FPRINTF(fp,"%4lu. index %4lu: nrels %4lu: partition %4lu\n",
            c, model->index, model->local, model->partition);
  }
  fclose(fp);
}


/*
 * Write the model-relation file to a file. This should be
 * really be written to the interpreter, and then handled
 * from the command line level but I am feeling lazy.
 * This code is as ugly as it is, because I am keeping a list
 * of list of relation instances associated which each slvmodel,
 * in the event that I *really* start to play clustering games.
 * In this case collapsing nodes will be moving around lists
 * rather than the individual relations which could be a big
 * win. In the mean time.....
 *
 * The format of the file is:
 *   n_models n_relations
 *   model_ndx rel_ndx
 *   model_ndx rel_ndx
 *   [ .... ]
 */
static
int WriteModelRelnsToFile(FILE *fp,
                          struct gl_list_t *models,
                          int n_relations)
{
  unsigned long len1,len2,len3;
  unsigned long i,j,k;
  struct SlvModel *model;
  struct Instance *relinst;
  struct gl_list_t *list;
  struct rel_relation *rel;

  len1 = gl_length(models);
  FPRINTF(fp,"%d %d\n",(int)len1,n_relations);

  for (i=1;i<=len1;i++) {
    model = (struct SlvModel *)gl_fetch(models,i);
    len2 = gl_length(model->relations);
    for (j=1;j<=len2;j++) {
      list = (struct gl_list_t *)gl_fetch(model->relations,j);
      len3 = gl_length(list);
      for (k=1;k<=len3;k++) {
        relinst = (struct Instance *)gl_fetch(list,k);
        rel = (struct rel_relation *)GetInterfacePtr(relinst);
        FPRINTF(fp,"%d %d\n",(int)model->index,rel_mindex(rel));
      }
    }
  }
}


/*
 *********************************************************************
 * Dag Creation Routines
 *
 * This function builds a dag of solver models based on the list
 * of model and array instances given. It also builds a list of relations
 * for each solver model and appends it. The linking of the models
 * is done elsewhere. This routine assumes that the instance list is
 * unique i.e, a dag or a tree with no aliases. If this is not the
 * case then the relations count will not be correct, but otherwise
 * the code will work fine.
 *
 * NOTE: We could be more efficient here by using arrays of SlvModelsf
 * rather than mallocing a node for each. That will be left for another
 * iteration of the code.
 *********************************************************************
 */
struct gl_list_t *Build_SlvModels(struct gl_list_t *inst_list)
{
  struct Instance *inst;
  struct gl_list_t *newlist, *tmp;
  struct SlvModel *model;
  unsigned long len,c;
  unsigned long total = 0;

  assert(inst_list);
  len = gl_length(inst_list);
  newlist = gl_create(len);

  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(inst_list,c);
    model = CreateSlvModel();			/* create a node */
    model->index = c;
    model->instance = inst;
    tmp = gl_create(AVG_RELATIONS);
    CollectLocalRelations(inst,tmp);
    model->local = gl_length(tmp);		/* set up local count */
    total += model->local;			/* debugging totalizer */
    gl_append_ptr(model->relations,(char *)tmp);
    gl_append_ptr(newlist,(char *)model);	/* add node to list */
  }
  FPRINTF(stderr,"Found %lu relations in Build_SlvModels\n",total);
  return newlist;
}


/*
 * This function attempts to wire up the list of solver models
 * based on the original list of models. The code here is based
 * on the wire up code that may be found in instance.c. The only
 * trick here is that we need to remove aliasing. A model may have
 * more than 1 parent, (i.e. it has an alias).
 * We build a replica of the instance tree. Removal of aliasing
 * is done somewhere else.
 */

static
void LinkSlvModels(struct gl_list_t *inst_list,
                   struct gl_list_t *newlist)
{
  struct Instance *inst, *child;
  struct SlvModel *model, *modchild;
  unsigned long len,c,cc;
  unsigned long nch, copynum, index;

  len = gl_length(inst_list);
  for (c=1;c<=len;c++) {
    inst = (struct Instance *)gl_fetch(inst_list,c);
    model = (struct SlvModel *)gl_fetch(newlist,c);
    nch = NumberChildren(inst);
    for (cc=1;cc<=nch;cc++) {
      child = InstanceChild(inst,cc);
      index = GetTmpNum(child);
      if (index==0) {	/* we did not label these */
        continue;
      }
      /*
       * Link parents and children.
       */
      modchild = (struct SlvModel *)gl_fetch(newlist,index);
      gl_append_ptr(modchild->parents,(char *)model);
      gl_append_ptr(model->children,(char *)modchild); 	/* add child */
    }
  }
}

static
struct gl_list_t *BuildDag(struct gl_list_t *inst_list)
{
  struct gl_list_t *models;

  models = Build_SlvModels(inst_list);
  LinkSlvModels(inst_list,models);
  return models;
}

/*
 *********************************************************************
 * BuildInstanceList
 *
 * These functions visit the instance tree from the given instance
 * and collect all instances that are models or arrays of models.
 * We first visit the tree to count the data and to index the nodes,
 * by setting their copynums. We then make a second appending the data
 * to a list which is then returned.
 *********************************************************************
 */
extern void ResetNodes(struct Instance *);	/* see compiler/instance.c */

struct CountNodeData {
  unsigned long count;
};

static
void CountModelNodes(struct Instance *inst,void *data)
{
  struct CountNodeData *tmp = (struct CountNodeData *)data;
  unsigned long count;

  if (!inst) {
    return;
  }
  count = tmp->count;
  switch (InstanceKind(inst)) {
  case MODEL_INST:
    count++;
    SetTmpNum(inst,count);
    break;
  case ARRAY_ENUM_INST:
  case ARRAY_INT_INST:
    if (!BrowIsAtomicArray(inst) && !BrowIsRelation(inst)) {
      count++;
      SetTmpNum(inst,count);
    }
    break;
  default:
    break;
  }
  tmp->count = count;	/* update the count */
}

static
void CollectModelNodes(struct Instance *inst,void *data)
{
  struct gl_list_t *list = (struct gl_list_t *)data;

  if (!inst) {
    return;
  }
  if (GetTmpNum(inst)) {
    gl_append_ptr(list,inst);
  }
}

static
struct gl_list_t *BuildInstanceList(struct Instance *inst,
                                    enum DagVisitSequence visit)
{
  struct gl_list_t *list;
  struct CountNodeData data;

  if (!inst) {
    list = gl_create(1);
    return list;
  }

  switch (visit) {
  case v_df_td:
    ZeroTmpNums(inst,0);
    data.count = 0;
    VisitInstanceTreeTwo(inst,CountModelNodes,
                         0,0,(void *)&data);
    list = gl_create(data.count);
    VisitInstanceTreeTwo(inst,CollectModelNodes,
                         0,0,(void *)list);
    break;
  default:
    FPRINTF(stderr,"This visitation sequence is not yet supported\n");
    /* fall through */
  case v_df_bu:
    ZeroTmpNums(inst,1);
    data.count = 0;
    VisitInstanceTreeTwo(inst,CountModelNodes,
                         1,0,(void *)&data);
    list = gl_create(data.count);
    VisitInstanceTreeTwo(inst,CollectModelNodes,
                         1,0,(void *)list);
    break;
  }

  g_dagdata.visit = visit;
  return list;
}


/*
 *********************************************************************
 * Exported Functions
 *
 * These functions are the entry points to the code in this file.
 * The supported functions at this moment are:
 * 0) 	PrepareDag
 * 1)	WriteDagToInterp
 * 2)	WriteRelnCountToInterp
 * 3)	WriteTearsToInterp
 * 4)	ShutDownDag
 *********************************************************************
 */



/*
 * We dont want to write out information for leaf model nodes;
 * i.e, a model that has only atoms, or relations or arrays of
 * atoms, or arrays of relations. So we start running down the
 * child list until we find something useful, then stop.
 * We prepare the formatting information, and then do the
 * processing from start to nch.
 */
static
void WriteInstNodeToInterp(Tcl_Interp *interp,struct Instance *inst)
{
  struct Instance *child;
  char tmp[64];
  unsigned long index,nch,c;
  unsigned long start = 0;

  nch = NumberChildren(inst);
  for (c=1;c<=nch;c++) {
    child = InstanceChild(inst,c);
    index = GetTmpNum(child);
    if (index) {
      start = c;
      break;
    }
  }
  if (!start) {
    return;		/* => this was a leaf node */
  }

  sprintf(tmp,"%lu",GetTmpNum(inst));
  Tcl_AppendResult(interp," {",tmp," ",(char *)NULL);
  for (c=start;c<=nch;c++) {
    child = InstanceChild(inst,c);
    index = GetTmpNum(child);
    if (index) {
      sprintf(tmp,"%lu",GetTmpNum(child));
      Tcl_AppendResult(interp,tmp," ",(char *)NULL);
    }
  }
  Tcl_AppendResult(interp,"} ",(char *)NULL);
}

static
void WriteInstListToInterp(Tcl_Interp *interp, struct gl_list_t *list)
{
  struct Instance *tmp_inst;
  unsigned long len,c;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    tmp_inst = (struct Instance *)gl_fetch(list,c);
    WriteInstNodeToInterp(interp,tmp_inst);
  }
}

int Asc_DagWriteInstDagCmd(ClientData cdata, Tcl_Interp *interp,
                       int argc, CONST84 char *argv[])
{
  struct Instance *i;
  struct gl_list_t *list;
  if ( argc < 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args: Usage __dag_write_instdag ?current?search",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    i = g_search_inst;
  }
  if (!i) {
    Tcl_SetResult(interp, "requested instance is NULL", TCL_STATIC);
    return TCL_ERROR;
  }
  list = g_dagdata.instances;
  if (!list) {
    Tcl_SetResult(interp, "dag data has not been prepared", TCL_STATIC);
    return TCL_ERROR;
  }
  WriteInstListToInterp(interp,list);
  return TCL_OK;
}


/*
 *********************************************************************
 * WriteModel Dag
 *
 * These functions accept a list of models, and write out the
 * connectivity information requied to reconstruct the dag associated
 * with the list of models. It is very similar to the above functions
 * but runs off a model list rather than an instance list. Once the
 * model list has been built it should be much faster than the instance
 * version as there is much less stuff to skip over.
 *********************************************************************
 */
static
void WriteModelNodeToInterp(Tcl_Interp *interp,struct SlvModel *model)
{
  struct SlvModel *child;
  char tmp[64];
  unsigned long nch,c;

  nch = gl_length(model->children);
  if (!nch) {
    return;
  }

  sprintf(tmp,"%lu",model->index);
  Tcl_AppendResult(interp," {",tmp," ",(char *)NULL);
  for (c=1;c<=nch;c++) {
    child = (struct SlvModel *)gl_fetch(model->children,c);
    sprintf(tmp,"%lu",child->index);
    Tcl_AppendResult(interp,tmp," ",(char *)NULL);
  }
  Tcl_AppendResult(interp,"} ",(char *)NULL);
}

static
void WriteModelDagToInterp(Tcl_Interp *interp, struct gl_list_t *list)
{
  struct SlvModel *model;
  unsigned long len,c;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    model = (struct SlvModel *)gl_fetch(list,c);
    WriteModelNodeToInterp(interp,model);
  }
}

int Asc_DagWriteModelDagCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  struct Instance *i;
  struct gl_list_t *list;
  struct gl_list_t *models;

  if ( argc < 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __dag_write_modeldag ?current?search",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    i = g_search_inst;
  }
  if (!i) {
    Tcl_SetResult(interp, "requested instance is NULL", TCL_STATIC);
    return TCL_ERROR;
  }
  list = g_dagdata.instances;
  if (!list) {
    Tcl_SetResult(interp, "dag data has not been prepared", TCL_STATIC);
    return TCL_ERROR;
  }
  models = g_dagdata.models;
  if (!models) {
    Tcl_SetResult(interp, "model data has not been prepared", TCL_STATIC);
    return TCL_ERROR;
  }

  WriteModelDagToInterp(interp,models);
  return TCL_OK;
}


/*
 * In v_*_td visitation of the tree, the root model will the
 * first model on the list. In a v_*_bu visitation, it will be
 * the last.
 */
static
struct SlvModel *GetDagRoot(struct gl_list_t *models,
                            enum DagVisitSequence visit)
{
  struct SlvModel *root;
  unsigned long len;

  len = gl_length(models);
  assert(len);
  switch (visit) {	/* root is first on the list */
  case v_df_td:
  case v_bf_td:
    root = (struct SlvModel *)gl_fetch(models,1);
    break;
  case v_df_bu:
  case v_bf_bu:
    root = (struct SlvModel *)gl_fetch(models,len);
    break;
  default:
    FPRINTF(stderr,"Unknown visitation sequence\n");
    root = NULL;
    break;
  }
  return root;
}


int DagPartition(Tcl_Interp *interp,unsigned long threshold,
                 int method)
{
  struct SlvModel *root;
  struct gl_list_t *list;
  struct gl_list_t *models;
  struct gl_list_t *partitionlist;
  struct gl_list_t *tears;
  slv_system_t sys;
  struct var_variable **vp;
  unsigned long len;

  /*
   * Check all of our data. If stuff checks out ok, update
   * the g_dagdata structure.
   */
  list = g_dagdata.instances;
  if (!list) {
    Tcl_SetResult(interp, "dag data not prepared", TCL_STATIC);
    return TCL_ERROR;
  }

  models = g_dagdata.models;
  if (!models) {
    Tcl_SetResult(interp, "dag model not prepared", TCL_STATIC);
    return TCL_ERROR;
  }

  sys = g_solvsys_cur;	/* this might be made more general */
  if (!sys) {
    Tcl_SetResult(interp, "solve system does not exist", TCL_STATIC);
    return TCL_ERROR;
  }

  if (threshold < 100) {
    g_dagdata.threshold = 100;
  } else {
    g_dagdata.threshold = threshold;
  }

  switch (method) {
  case 1:
    /*
     * Pre process
     */
    len = gl_length(list);
    root = GetDagRoot(models,g_dagdata.visit);
    assert(root);
    ReorientTree(root);
    ResetModelTree(models);
    IndexTree(root,g_dagdata.visit);
    PostIndexingFixup(root,models);
    /*
     * Do the partitioning. This also labels each model as to which
     * partition that it sits in.
     */
    partitionlist = Partition_1(models, g_dagdata.threshold,
                                p_clusterup, 1,len);
    /*
     * Post process.
     */
    MarkRelnPartitions(models);
    tears = MarkTearVariables(slv_get_master_var_list(sys),
                              slv_get_num_master_vars(sys));
    break;
  case 2:
    /*
     * Pre process
     */
    len = gl_length(list);
    root = GetDagRoot(models,g_dagdata.visit);
    assert(root);
    ReorientTree(root);
    ResetModelTree(models);
    IndexTree(root,g_dagdata.visit);
    PostIndexingFixup(root,models);
    /*
     * Do the partitioning. This also labels each model as to which
     * partition that it sits in.
     */
    partitionlist = Partition_2(models,threshold);
    /*
     * Post process.
     */
    MarkRelnPartitions(models);
    tears = MarkTearVariables(slv_get_master_var_list(sys),
                              slv_get_num_master_vars(sys));
    break;
  default:
    Tcl_SetResult(interp, "this method not yet supported", TCL_STATIC);
    return TCL_ERROR;
  }

  /*
   * Do some data reporting, and set up for the next call.
   */
  WritePartitions(partitionlist);
  WriteTearVarsToInterp(interp,tears);
  WriteModelData(models);
  if (g_dagdata.tears) {
    gl_destroy(g_dagdata.tears);
    g_dagdata.tears = tears;
  }
  if (g_dagdata.partitions) {
    gl_free_and_destroy(g_dagdata.partitions);	/* destroy the data */
    g_dagdata.partitions = partitionlist;
  }

  return 0;
}

/*
 * This is the sole entry point for all the partitioning algorithms.
 * method controls which partitioning algorithm will be used.
 * threshold means different things for different algorithms, but
 * in general means the cutoff size for clusters.
 */
int Asc_DagPartitionCmd(ClientData cdata, Tcl_Interp *interp,
                    int argc, CONST84 char *argv[])
{
  unsigned long threshold;
  int method;
  int result;

  if ( argc != 3 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __dag_partition method threshold",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  method = atoi(argv[1]);
  threshold = atol(argv[2]);
  result = DagPartition(interp,threshold,method);
  if (result) {
    return TCL_ERROR;
  }
}


/*
 * THIS IS INCOMPLETE
 * I need to fiqure out a way to avoid calling ParentsName
 * for every model in the graph, as ParentsName requires a
 * linear search. In fact it is perhaps much easier to do
 * this off the instance list, rather than off the model list.
 * THIS IS INCOMPLETE.
 */

static
void DagWriteNamesToInterp(Tcl_Interp *interp,struct gl_list_t *models)
{
  struct SlvModel *model;
  unsigned long len,c;

  return;
}

/* THIS IS INCOMPLETE */
int DagWriteNamesCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  if ( argc != 1 ) {
    Tcl_SetResult(interp, "wrong # args : Usage __dag_writenames", TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_dagdata.models==NULL) {
    Tcl_SetResult(interp, "model dag data not prepared", TCL_STATIC);
    return TCL_ERROR;
  }
  DagWriteNamesToInterp(interp,g_dagdata.models);
  return TCL_OK;
}


int Asc_DagCouplingRelnsCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  slv_system_t sys;
  int result;
  int filter;

  if ( argc != 3 ) {
    Tcl_SetResult(interp,
                  "wrong # args:"
                  " Usage __dag_coupling_relns ?current?search filter",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  sys = g_solvsys_cur;	/* this might be made more general */
  if (!sys) {
    Tcl_SetResult(interp, "solve system does not exist", TCL_STATIC);
    return TCL_ERROR;
  }
  filter = atoi(argv[2]);
  result = WriteCouplingRelnsToInterp(interp,slv_get_master_rel_list(sys),
                                      slv_get_num_master_rels(sys),
                                      filter);
  if (result) {
    return TCL_ERROR;
  }
  return TCL_OK;
}


int Asc_DagModelRelnsCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  FILE *fp = NULL;
  slv_system_t sys;
  char *file;

  if ( argc != 3 ) {
    Tcl_SetResult(interp,
                  "wrong # args: Usage __dag_model_relns ?current?search file",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!g_dagdata.models) {
    Tcl_SetResult(interp, "model data has not been prepared", TCL_STATIC);
    return TCL_ERROR;
  }
  sys = g_solvsys_cur;	/* this might be made more general */
  if (!sys) {
    Tcl_SetResult(interp, "solve system does not exist", TCL_STATIC);
    return TCL_ERROR;
  }

  fp = fopen(argv[2],"w");
  if (!fp) {
    Tcl_SetResult(interp, "unable to open file", TCL_STATIC);
    return TCL_ERROR;
  }
  WriteModelRelnsToFile(fp,g_dagdata.models,
                        slv_get_num_master_rels(sys));
  if (fp) {
    fclose(fp);
  }
  return TCL_OK;
}


void DagWriteLocalRelns(Tcl_Interp *interp, struct gl_list_t *list)
{
  char tmp[64];
  unsigned long len,c;
  unsigned long local;

  len = gl_length(list);
  for (c=1;c<=len;c++) {
    local = (unsigned long)gl_fetch(list,c);
    sprintf(tmp,"%lu",local);
    Tcl_AppendResult(interp,tmp," ",(char *)NULL);
  }
}

int Asc_DagCountRelnsCmd(ClientData cdata, Tcl_Interp *interp,
                     int argc, CONST84 char *argv[])
{
  struct gl_list_t *list;

  if ( argc < 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __dag_countrelns ?current?search",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (g_dagdata.instances==NULL) {
    Tcl_SetResult(interp, "dag data not prepared", TCL_STATIC);
    return TCL_ERROR;
  }
  list = CountLocalRelations(g_dagdata.instances);
  DagWriteLocalRelns(interp,list);
  gl_destroy(list);	/* we could possibly save it; but... */
  return TCL_OK;
}


/*
 * This function assumes that the instance list has been built
 * already and safely cached away in the g_dagdata structure.
 * It will build the dag model list and also stash it away.
 * It will not destroy an existing model  list. It will also
 * use whatever vistitation sequence that the instance list was
 * built from.
 */
int Asc_DagBuildDagCmd(ClientData cdata, Tcl_Interp *interp,
                   int argc, CONST84 char *argv[])
{
  struct Instance *i;
  struct gl_list_t *list;
  struct gl_list_t *models;

  if ( argc < 2 ) {
    Tcl_SetResult(interp,
                  "wrong # args : Usage __dag_build_modeldag ?current?search",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    i = g_search_inst;
  }
  if (!i) {
    Tcl_SetResult(interp, "requested instance is NULL", TCL_STATIC);
    return TCL_ERROR;
  }
  list = g_dagdata.instances;
  if (!list) {
    Tcl_SetResult(interp, "dag data has not been prepared", TCL_STATIC);
    return TCL_ERROR;
  }

  models = g_dagdata.models;
  if (!models) {		/* Need to build the model list */
    models = BuildDag(list);
    g_dagdata.models = models;
  }
  return TCL_OK;
}

int Asc_DagPrepareCmd(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  struct Instance *i;
  enum DagVisitSequence visitseq;
  if ( argc != 3 ) {
    Tcl_AppendResult(interp,"wrong # args : ",
                     "Usage __dag_prepare ?current?search? ",
                     "visit_sequence",(char *)NULL);
    return TCL_ERROR;
  }

  if (strncmp(argv[1],"current",3)==0) {
    i = g_curinst;
  } else {
    i = g_search_inst;
  }
  if (!i) {
    Tcl_SetResult(interp, "requested instance is NULL", TCL_STATIC);
    return TCL_ERROR;
  }
  visitseq = (enum DagVisitSequence)atoi(argv[2]);
  ResetDagData();
  g_dagdata.instances = BuildInstanceList(i,visitseq);
  g_dagdata.visit = visitseq;
  return TCL_OK;
}

/*
 * Registered as __dag_shutdown
 */
int Asc_DagShutdownCmd(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  DestroyDagData();		/* Write Function */
  ResetDagData();
  return TCL_OK;
}

