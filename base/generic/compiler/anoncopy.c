/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1998 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Anonymous copy implementation
*//*
	by Benjamin Allan
	September 08, 1997
	Last in CVS $Revision: 1.3 $ $Date: 1998/06/16 16:38:34 $ $Author: mthomas $
*/

/*
 * we really need to ditch some of this crap.
 */
#include <time.h>
#include <stdarg.h>


#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/pairlist.h>
#include <general/dstring.h>
#include "bit.h"
#include "symtab.h"


#include "functype.h"
#include "expr_types.h"
#include "instance_enum.h"
#include "stattypes.h"
#include "statement.h"
#include "child.h"
#include "type_desc.h"
#include "module.h"
#include "library.h"
#include "sets.h"
#include "setio.h"
#include "extfunc.h"
#include "extcall.h"

#include "forvars.h"
#include "exprs.h"
#include "name.h"
#include "nameio.h"
#include "vlist.h"
#include "slist.h"
#include "evaluate.h"
#include "value_type.h"
#include "statio.h"
#include "pending.h"
#include "find.h"
#include "relation_type.h"
#include "extfunc.h"
#include "rel_blackbox.h"
#include "relation.h"
#include "logical_relation.h"
#include "logrelation.h"
#include "relation_util.h"
#include "logrel_util.h"
#include "instance_types.h"
#include "cmpfunc.h"
#include "instance_io.h"
#include "when.h"
#include "case.h"
#include "when_util.h"
#include "select.h"
#include "atomvalue.h"
#include "arrayinst.h"
#include "copyinst.h"
#include "createinst.h"
#include "destroyinst.h"
#include "extinst.h"
#include "visitinst.h"
#include "instquery.h"
#include "mathinst.h"
#include "mergeinst.h"
#include "parentchild.h"
#include "refineinst.h"
#include "check.h"
#include "instance_name.h"
#include "setinstval.h"
#include "tmpnum.h"
#include "anontype.h"
#include "instmacro.h" /* some of this should move to relation.c */
#include "instantiate.h"

#if ATDEBUG
#include "relation_io.h"
static struct Instance *g_Pass2AddAnonProtoVar_visit_root = 0;
#endif


/* forward declaration */
static struct Instance *
CopyAnonRelationArrayInstance(struct Instance *, struct Instance *,
                              unsigned long, struct gl_list_t *, struct pairlist_t *);
/**
	Create a new relation instance based on protorel,
	and attach it to newparent, copying details by reference.
	If newparent is NULL, no attachment done. This should only
	be the case when the newparent is an array and will be
	OTHERWISE established.

	When we copy an anonymous relation instance, we don't copy the 'ptr'
	element, since that contains the specific stuff about which *variable*
	are being indicated. This needs to be torn out and replaced by the
	correct stuff from the original (duplicand) relation. -- JP
*/

static
struct Instance *CopyAnonRelationInstance(struct Instance *newparent,
			                  struct Instance *protorel,
                                          unsigned long pos,
			                  struct gl_list_t *copyvars,
					struct pairlist_t *bboxtable)
{
  register struct RelationInstance *src,*result;
  struct relation *rel;
  enum Expr_enum type;
  register unsigned long size;

#if TIMECOMPILER
  /* hack to get some statistics. BAA */
  extern int g_CopyAnonRelation;
  g_CopyAnonRelation++;
#endif

  /*CONSOLE_DEBUG("COPYING PROTOTYPE %p TO POS %lu UNDER PARENT %p",protorel,pos,newparent);*/

  assert(InstanceKind(protorel) == REL_INST);
  src = RELN_INST(protorel);
  size = GetByteSize(src->desc);
  result = RELN_INST(ascmalloc((unsigned)size));
  AssertMemory(result);

  ascbcopy((char *)src,(char *)result,(int)size);
  result->parent[0] = NULL;
  result->parent[1] = NULL;
  result->whens = NULL;
  result->logrels = NULL;
  result->anon_flags = 0x0;
  CopyTypeDesc(result->desc);
  /*if(result->ptr!=NULL){
    CONSOLE_DEBUG("Clearing ptr %p in new rel %p, copy of %p",result->ptr,result,src);
  }*/
  result->ptr = NULL;

  /* fixup subatomic relation child pointers. */
  RedoChildPointers(ChildListLen(GetChildList(result->desc)),
		    INST(result),REL_CHILD(result,0),
		    protorel,REL_CHILD(protorel,0));
  /* check fixup subatomic relation child pointers to set values. */
  CheckChildCopies(ChildListLen(GetChildList(result->desc)),
		   REL_CHILD(result,0));

  rel = CopyAnonRelationByReference(protorel,INST(result),copyvars, bboxtable);
  type = GetInstanceRelationType(protorel);
  SetInstanceRelation(INST(result),rel,type);

  if (newparent !=NULL) {
    LinkToParentByPos(newparent,INST(result),pos);
  }
  return INST(result);
}

static
struct gl_list_t *CopyAnonArrayChildPtrs(struct Instance *newparent,
                                         struct gl_list_t *list,
                                         struct gl_list_t *copyvars,
					struct pairlist_t *bboxtable)
{
  register struct gl_list_t *result;
  register unsigned long length,c;
  register struct ArrayChild *new,*src;
  if (list!=NULL){
    length = gl_length(list);
    if (length) {
      result = gl_create(length);
      for (c=1;c<=length;c++) {
	src = (struct ArrayChild *)gl_fetch(list,c);
	new = MALLOCPOOLAC;
	*new = *src;
        /* new->inst points to the proto of this array entry. need to fix */
        if (new->inst !=NULL) {
          switch (InstanceKind(new->inst)) {
          /* should add an lrel case here if lrels ever get to sharing. */
          case REL_INST:
            new->inst = CopyAnonRelationInstance(NULL,new->inst,c,
                                                 copyvars, bboxtable);
            AddParent(new->inst,newparent);
            break;
          case ARRAY_ENUM_INST:
          case ARRAY_INT_INST:
			CONSOLE_DEBUG("Copying ARRAY_INT_INST");
            new->inst = CopyAnonRelationArrayInstance(NULL, new->inst,
                                                      0, copyvars, bboxtable);
            AddParent(new->inst,newparent);
            break;
          default:
            Asc_Panic(2,__FUNCTION__,"Non-relation array child!");
          }
        }
	gl_append_ptr(result,(VOIDPTR)new);
      }
      gl_set_sorted(result, TRUE); /* because the originals were, by name */
      return result;
    } else {
      return gl_create(0);
    }
  } else {
    return NULL;
  }
}

/*
 * this function recurses, and probably belongs in arrayinst.c
 * If newparent is not NULL, links result to it automatically.
 */
static
struct Instance *
CopyAnonRelationArrayInstance(struct Instance *newparent,
                              struct Instance *proto,
                              unsigned long pos,
                              struct gl_list_t *copyvars,
				struct pairlist_t *bboxtable)
{
  register struct ArrayInstance *ary,*result;
  AssertMemory(proto);
  ary = ARY_INST(proto);
  result = ARY_INST(ASC_NEW(struct ArrayInstance));
  AssertMemory(result);
  result->t = ary->t;
  result->pending_entry = NULL;
  result->desc = ary->desc;
  CopyTypeDesc(result->desc);
  result->indirected = ary->indirected;
  result->visited = 0;
  result->tmp_num = 0;
  result->parents = gl_create(AVG_PARENTS);
  result->children = CopyAnonArrayChildPtrs(INST(result),
                                            ary->children,
                                            copyvars,bboxtable);
  if (newparent != NULL) {
    LinkToParentByPos(newparent,INST(result),pos);
  }
  return INST(result);
}

/* Collects the index path to any instance w/tmpnum != 0
 * and resets the tmpnum of the index of the collected path in
 * the index path list. Each collected path ends with a 0.
 * Since we visit an instance exactly once,
 * The memory needed here is freed all at once, so we can
 * play a pooling game as needed. Then we will need to make
 * a little struct that holds the buffer and indexpaths.
 */
static
void Pass2AddAnonProtoVar(struct Instance *i,
                          unsigned long *llist,
                          int activelen,
                          struct gl_list_t *indexpaths)
{
  unsigned long int *indices;
  int j;
  if (GetTmpNum(i)) {
    assert(InstanceKind(i) == REAL_ATOM_INST);
    indices = ASC_NEW_ARRAY(unsigned long int,activelen+1);
    if (indices==NULL) {
      FPRINTF(ASCERR,"Pass2AddAnonProtoVar: Insufficient memory.");
      return;
    }
    for (j = 0; j < activelen ; j++) {
      indices[j] = llist[j];
    }
    indices[activelen] = 0L;
    gl_append_ptr(indexpaths,(void *)indices);
    SetTmpNum(i,gl_length(indexpaths));
#if ATDEBUG
    FPRINTF(ASCERR,"Pass2AddAnonProtoVar: found index path %lu for ", gl_length(indexpaths));
	WriteInstanceName(ASCERR, i,g_Pass2AddAnonProtoVar_visit_root);
    FPRINTF(ASCERR,"\n");
    FPRINTF(ASCERR,"indices(0-terminated) =");
    for (j = 0 ; j <= activelen; j++) {
      FPRINTF(ASCERR,"%lu ", indices[j]);
    }
    FPRINTF(ASCERR,"\n");
#endif

  }
}

/*
 * sets to 1 the tmpnums of all vars occuring in any relation visited.
 */
static
void Pass2LabelAnonProtoVars(struct Instance *i)
{
  if (InstanceKind(i)==REL_INST) {
    CONST struct relation *r;
    r = GetInstanceRelationOnly(i);
    if (r != NULL) {
      unsigned long c,len;
      len = NumberVariables(r);
      for (c = 1; c <= len; c++) {
        /* just as fast to set as it is to check whether already set */
        SetTmpNum(RelationVariable(r,c),1);
      }
    }
  }
}
/*
 * i must be a MODEL.
 * This function returns a gl_list of index paths through i
 * to reach the vars occuring in relations (or relation arrays) of i.
 * Each var will only occur once in the path list returned.
 * An index path can be followed through any instance isomorphic to i
 * and will end at a variable semantically equivalent to the one
 * in i that generated the path.
 * At the expense of a visit tree call (which we need anyway)
 * this returns the list unsorted and never searched.
 *
 * At present the list returned should be destroyed with
 * gl_free_and_destroy. This may change if we find ourselves
 * in malloc/free hell from constructing index paths.
 */
struct gl_list_t *Pass2CollectAnonProtoVars(struct Instance *i)
{
  unsigned long c,len;
  struct Instance *ch;
  unsigned int indexlen = 40;
  unsigned long *indices;
  struct gl_list_t *result;

  assert(InstanceKind(i)==MODEL_INST);
#if ATDEBUG
      FPRINTF(ASCERR,"Pass2AddAnonProtoVar: Labeling vars for model:\n   ");
	WriteInstanceName(ASCERR,i,NULL);
      FPRINTF(ASCERR,"\n");
#endif
  len = NumberChildren(i);
  result = gl_create(100);
  indices = ASC_NEW_ARRAY(unsigned long,indexlen);
  if (indices == NULL || result == NULL) {
    FPRINTF(ASCERR,"Pass2CollectAnonProtoVars: Insufficient memory\n");
    return result;
  }
  if (len==0) {
    return result;
  }
  /* Don't visit whole tree of i to label relation vars, just local rels.
   * Set tmpnums of vars in local relations to LONG_MAX-1 and of all other
   * instances to 0.
   */
  ZeroTmpNums(i,0);
  for (c=1; c<= len; c++) {
    ch = InstanceChild(i,c);
    switch (InstanceKind(ch)) {
    case REL_INST:
#if ATDEBUG
      FPRINTF(ASCERR,"Pass2AddAnonProtoVar: Labeling vars for equation:\n   ");
	WriteRelation(ASCERR,ch,i);
      FPRINTF(ASCERR,"\n");
#endif
      Pass2LabelAnonProtoVars(ch);
      break;
    case ARRAY_INT_INST:
    case ARRAY_ENUM_INST:
      if (GetBaseType(GetArrayBaseType(InstanceTypeDesc(ch)))
          == relation_type) {
#if ATDEBUG
      FPRINTF(ASCERR,"Pass2AddAnonProtoVar: Labeling vars for an eqn array:  \n   ");
	WriteInstanceName(ASCERR,ch,i);
      FPRINTF(ASCERR,"\n");
#endif
        SilentVisitInstanceTree(ch,(VisitProc)Pass2LabelAnonProtoVars,0,0);
      }
      break;
    default:
      break;
    }
  }
  /*
   * Now do an indexed visit to collect the index paths to var with
   * tmpnum != 0. Set the tmpnum of each collected var to the index
   * in the path.
   */
#if ATDEBUG
	g_Pass2AddAnonProtoVar_visit_root = i;
#endif
  IndexedVisitInstanceTree(i,(IndexedVisitProc)Pass2AddAnonProtoVar,
                           0,0,&indices,&indexlen,result);
  ascfree(indices);
  return result;
}

/*
 * deallocate the indexpathlist collected by Pass2CollectAnonProtoVars.
 */
void Pass2DestroyAnonProtoVars(struct gl_list_t *indexpathlist)
{
  gl_free_and_destroy(indexpathlist);
}

/*
 * This function takes the index paths from Pass2CollectAnonProtoVars(proto)
 * and follows the indices through i which must be of the same anonymous
 * type to build a gl_list of ATOM pointers indexed in the same way
 * as the atoms in proto are tmpnum'd. The tmpnums of i are irrelevant.
 */
static
struct gl_list_t *Pass2CollectAnonCopyVars(struct gl_list_t *protovars,
                                           struct Instance *i)
{
  struct gl_list_t *result;
  struct Instance *ch;
  unsigned long c,len;
  unsigned long *indexpath;
  int j;

#if ATDEBUG
      FPRINTF(ASCERR,"Pass2CollectAnonCopyVars: collecting vars for model:\n   ");
	WriteInstanceName(ASCERR,i,NULL);
      FPRINTF(ASCERR,"\n");
#endif
  assert(InstanceKind(i)==MODEL_INST);
  len = gl_length(protovars);
  result = gl_create(len);
  for (c=1; c <= len; c++) {
    /* find var c in similar instance */
    indexpath = ( unsigned long *)gl_fetch(protovars,c);
    ch = i;
    for (j = 0; indexpath[j] != 0; j++) {
      ch = InstanceChild(ch,indexpath[j]);
      assert(ch!=NULL);
#if ATDEBUG
      FPRINTF(ASCERR,"Pass2CollectAnonCopyVars: child link %d = %lu to  ",j,indexpath[j]);
	WriteInstanceName(ASCERR,ch,i);
      FPRINTF(ASCERR,"\n");
#endif
    }
    gl_append_ptr(result,ch);
    assert(InstanceKind(ch) == REAL_ATOM_INST);
#if ATDEBUG
	WriteInstanceName(ASCERR,ch,i);
    FPRINTF(ASCERR,"\n");
#endif
  }
  return result;
}

static
void CopyAnonDummyInstance(struct Instance *parent,
                           unsigned long pos,
                           struct TypeDescription *def)
{
  struct Instance *inst;
  inst = ShortCutMakeUniversalInstance(def);
  if (inst==NULL) {
    inst = CreateDummyInstance(def);
  }
  LinkToParentByPos(parent,inst,pos);
}


/*
 * Copies all the local relations (including those in arrays)
 * of the MODEL instance proto to the instance i using only
 * local information. No global information is needed, but
 * we need to arrange that the tmpnums all start and end 0
 * so we can avoid extra 0ing of them.
 * Strictly not recursive at this level.
 */
void Pass2CopyAnonProto(struct Instance *proto,
		struct BitList *protoblist,
		struct gl_list_t *protovarindices,
		struct Instance *i
){
  struct BitList *blist;
  struct gl_list_t *copyvars;
  struct Instance *ch;
  unsigned long nch,c;
  struct BlackBoxCache *dupCache;
  struct BlackBoxData *bbd;
/*
 * This pairlist matches proto and new bbox common objects.
 */
  struct pairlist_t *bboxtable;

  blist = InstanceBitList(i);
  /* copy bitlist from proto to i since they must be identical
   * when we are done making them identical by copying newly
   * made relations. Likewise, remove from pending list if needed.
   */
  OverwriteBList(protoblist,blist);
  if (!InstanceInList(proto) && InstanceInList(i)) {
    /* for now, this if will always pass */
    RemoveInstance(i);
  }
#if ATDEBUG
      FPRINTF(ASCERR,"Pass2CollectAnonCopyVars: Collecting vars for  ");
	WriteInstanceName(ASCERR,i,NULL);
    FPRINTF(ASCERR,"\n");
#endif
  copyvars = Pass2CollectAnonCopyVars(protovarindices,i);
  bboxtable = pairlist_create(10);
  /* Now copy local relations, and wherever a var is needed
   * in the copy, use the tmpnum of the original var in proto to
   * look up the equivalent var in the copyvars found from i.
   */
  nch = NumberChildren(proto);
  for (c = 1; c <= nch; c++) {
    ch = InstanceChild(proto,c);
    if (ch != NULL && InstanceChild(i,c) == NULL) {
      switch (InstanceKind(ch)) {
      case REL_INST:
        CopyAnonRelationInstance(i,ch,c,copyvars,bboxtable);
        break;
      case ARRAY_ENUM_INST:
      case ARRAY_INT_INST:
        CopyAnonRelationArrayInstance(i,ch,c,copyvars,bboxtable);
        break;
      case DUMMY_INST:
        CopyAnonDummyInstance(i,c,InstanceTypeDesc(ch));
        break;
      default:
        break;
      }
    }
  }
  gl_destroy(copyvars);

  /* fixup the init call */
  nch = pairlist_length(bboxtable);
#if ATDEBUG
      FPRINTF(ASCERR,"P2CAP: bbox fixups for %d in ",(int)nch);
	WriteInstanceName(ASCERR,i,NULL);
    FPRINTF(ASCERR,"\n");
#endif
  for (c = 1; c <= nch; c++)
  {
    bbd = (struct BlackBoxData *)pairlist_valueAt(bboxtable,c);
    dupCache = bbd->common;
    InitBBox(i,dupCache);
  }
  pairlist_destroy(bboxtable);
}

