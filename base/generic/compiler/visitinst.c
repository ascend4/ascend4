/*
 *  Ascend Instance Tree Type Visit Implementation
 *  by Tom Epperly
 *  9/3/89
 *  Version: $Revision: 1.21 $
 *  Version control file: $RCSfile: visitinst.c,v $
 *  Date last modified: $Date: 1998/02/26 15:59:37 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Benjamin Allan
 *  based on instance.c
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
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include "symtab.h"
#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "type_desc.h"
#include "instance_name.h"
#include "instquery.h"
#include "instance_io.h"
#include "instmacro.h"
#include "parentchild.h"
#include "when_util.h"
#include "instance_types.h"
#include "tmpnum.h"
#include "visitinst.h"

#ifndef lint
static CONST char InstanceVisitModuleID[] = "$Id: visitinst.c,v 1.21 1998/02/26 15:59:37 mthomas Exp $";
#endif

unsigned long global_visit_num = 0;
int g_iscomplete = 1;
/************* VisitInstance stuff **************************************/

#define OLDCHECKVISIT 0
#if OLDCHECKVISIT
/* returns 1 if not visited yet, 0 if been here already. */
static int CheckVisitNumber(struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
    if (global_visit_num > MOD_INST(i)->visited) {
      MOD_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case REAL_CONSTANT_INST:
    if (global_visit_num > RC_INST(i)->visited) {
      RC_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case BOOLEAN_CONSTANT_INST:
    if (global_visit_num > BC_INST(i)->visited) {
      BC_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case INTEGER_CONSTANT_INST:
    if (global_visit_num > IC_INST(i)->visited) {
      IC_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case SYMBOL_CONSTANT_INST:
    if (global_visit_num > SYMC_INST(i)->visited) {
      SYMC_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case REAL_ATOM_INST:
    if (global_visit_num > RA_INST(i)->visited) {
      RA_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case BOOLEAN_ATOM_INST:
    if (global_visit_num > BA_INST(i)->visited) {
      BA_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case INTEGER_ATOM_INST:
    if (global_visit_num > IA_INST(i)->visited) {
      IA_INST(i)->visited = global_visit_num;
      return 1;
    }
    else return 0;
  case SET_ATOM_INST:
    if (global_visit_num > SA_INST(i)->visited) {
      SA_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  case SYMBOL_ATOM_INST:
    if (global_visit_num > SYMA_INST(i)->visited) {
      SYMA_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  case REL_INST:
    if (global_visit_num > RELN_INST(i)->visited) {
      RELN_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  case LREL_INST:
    if (global_visit_num > LRELN_INST(i)->visited) {
      LRELN_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  case WHEN_INST:
    if (global_visit_num > W_INST(i)->visited) {
      W_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    if (global_visit_num > ARY_INST(i)->visited) {
      ARY_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  case DUMMY_INST:
    if (global_visit_num > D_INST(i)->visited) {
      D_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
    return 1;			/* assumed always unvisited */
        			/* since they have exactly one ATOM parent */
  default:
    Asc_Panic(2, "VisitInstanceTree",
              "Instance tree contains illegal instance.");
    break;
  }
}
#else  /* oldcheckvisit alternative, faster? needs testing */

/* returns 1 if not visited yet, 0 if been here already or i bogus. */
static int CheckVisitNumber(struct Instance *i)
{
  AssertMemory(i);
  if (i->t & IERRINST) {
    FPRINTF(ASCERR,"CheckVisitNumber called with bad instance\n");
    return 0;
  }
  /* models arrays atoms relations constants */
  if (IsCompoundInstance(i)) {
    /* models arrays, maybe lists in future */
    if (IsArrayInstance(i)) {
      /* arrays */
      if (global_visit_num > ARY_INST(i)->visited) {
        ARY_INST(i)->visited = global_visit_num;
        return 1;
      } else {
        return 0;
      }
    } else {
      /* models */
      if (i->t == MODEL_INST && global_visit_num > MOD_INST(i)->visited) {
        MOD_INST(i)->visited = global_visit_num;
        return 1;
      } else {
        return 0;
        /* sim instances are assumed visited, especially since they
         * should never be seen by VisitTree.
         */
      }
    }
  } else { /* atoms and relations constants */
    if (IsAtomicInstance(i)) {
      if (global_visit_num > CA_INST(i)->visited) {
        CA_INST(i)->visited = global_visit_num;
        return 1;
      } else {
        return 0;
      }
    }
    if (IsConstantInstance(i)) {
      /* constants */
      if (global_visit_num > CI_INST(i)->visited) {
        CI_INST(i)->visited = global_visit_num;
        return 1;
      } else {
        return 0;
      }
    }
    /* would be nice to have a CommonEqnInstance
     * here for whenrellogrel CE_INST.
     */
    if (i->t == REL_INST) {
      if (global_visit_num > RELN_INST(i)->visited) {
        RELN_INST(i)->visited = global_visit_num;
        return 1;
      } else {
        return 0;
      }
    }
    if (i->t == LREL_INST) {
      if (global_visit_num > LRELN_INST(i)->visited) {
        LRELN_INST(i)->visited = global_visit_num;
        return 1;
      } else {
        return 0;
      }
    }
    if (i->t == WHEN_INST) {
      if (global_visit_num > W_INST(i)->visited) {
        W_INST(i)->visited = global_visit_num;
        return 1;
      } else {
        return 0;
      }
    }
  }
  /* fundamentals and rogues */
  if (IsFundamentalInstance(i) ) {
    /* fundamentals assumed always unvisited */
    return 1;
  }
  if ( InstanceKind(i)==DUMMY_INST) {
    if (global_visit_num > D_INST(i)->visited) {
      D_INST(i)->visited = global_visit_num;
      return 1;
    } else {
      return 0;
    }
  } else {
    /* rogues */
    Asc_Panic(2, "VisitInstanceTree",
              "VisitInstanceTree: Instance tree contains illegal instance.");
    
  }
}
#endif  /*  OLDCHECKVISIT  */

#if 0 /* old unused code. might be handy for debugging someday */
/* if visit number of i is nonzero sets it 0 and returns 1, else 0 */
static int ZeroVisitNumber(struct Instance *i)
{
  AssertMemory(i);
  switch(i->t) {
  case MODEL_INST:
    if (MOD_INST(i)->visited) {
      MOD_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case REAL_CONSTANT_INST:
    if (RC_INST(i)->visited) {
      RC_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case BOOLEAN_CONSTANT_INST:
    if (BC_INST(i)->visited) {
      BC_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case INTEGER_CONSTANT_INST:
    if (IC_INST(i)->visited) {
      IC_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case SYMBOL_CONSTANT_INST:
    if (SYMC_INST(i)->visited) {
      SYMC_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case REAL_ATOM_INST:
    if (RA_INST(i)->visited) {
      RA_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case BOOLEAN_ATOM_INST:
    if (BA_INST(i)->visited) {
      BA_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case INTEGER_ATOM_INST:
    if (IA_INST(i)->visited) {
      IA_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case SET_ATOM_INST:
    if (SA_INST(i)->visited) {
      SA_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case SYMBOL_ATOM_INST:
    if (SYMA_INST(i)->visited) {
      SYMA_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case REL_INST:
    if (RELN_INST(i)->visited) {
      RELN_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case LREL_INST:
    if (LRELN_INST(i)->visited) {
      LRELN_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case WHEN_INST:
    if (W_INST(i)->visited) {
      W_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    if (ARY_INST(i)->visited) {
      ARY_INST(i)->visited=0;
      return 1;
    }
    else return 0;
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SET_INST:
  case SYMBOL_INST:
  case DUMMY_INST:
    return 0;			/* assumed always unvisited */
        			/* since they have exactly one ATOM parent */
  default:
    ASC_PANIC("ZeroVisitInstanceTree:"
              "  Instance tree contains illegal instance.\n");
    /*NOTREACHED*/
  }
  
}

#endif /* old unused code. might be handy for debugging someday */

static
void WriteWhereNull(FILE *f, struct Instance *i)
{
  (void) i;
  FPRINTF(f,"Null instance in tree at ???? (null child of instance of type '%s')\n", SCP(InstanceType(i)) );
  g_iscomplete=0;
}

/* set global_visit_num = 0 after calling this for all instances
 * in simulation universe and prototype libraries. 
 */
void ResetVisitCounts(struct Instance *inst)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (InstanceKind(inst) == SIM_INST) return; /* ack!! */
  if (CheckVisitNumber(inst)){
    /* subtree has been visited in the past, not 0d */
    if (NotAtom(inst)) {
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++) {
        if ((child = InstanceChild(inst,c))!=NULL) {
          ResetVisitCounts(child);
          /* don't really care about NULL */
        }
      }
    }
  }
}

static void SlowVisitTree(struct Instance *inst,
        		VisitProc proc,
        		int depth, int leaf)
{
  unsigned long nc,c;
  struct Instance *child;
  unsigned nullchildren=0;

  AssertMemory(inst);			/* If null got here, die */
  if (CheckVisitNumber(inst)){		/* Not here another way already.*/
    if (!depth) (*proc)(inst);	       /* Apply on the way down, if top down.*/
    if (leaf||NotAtom(inst)) {		/* Go down on all children not null.*/
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++) {
        if ((child = InstanceChild(inst,c))!=NULL) { /* don't go down NULL */
          SlowVisitTree(child,proc,depth,leaf);
        } else {
          nullchildren++;
          /* no point in whining about someplace we can't give a name */
        }
      }
      if (nullchildren) {
	    ERROR_REPORTER_START_HERE(ASC_PROG_WARNING);
        FPRINTF(ASCERR,"Found %u NULL children of '",nullchildren);
        WriteInstanceName(ASCERR,inst,NULL);
        FPRINTF(ASCERR,"'.\n");
		error_reporter_end_flush();
      }
    }
    if (depth) (*proc)(inst);		/* Apply on way up, if bottom up.*/
  }
}

void SlowVisitInstanceTree(struct Instance *inst,
        			VisitProc proc,
        			int depth, int leaf)
{
  global_visit_num++;
  AssertMemory(inst);
  if (inst!=NULL) {
    SlowVisitTree(inst,proc,depth,leaf);
  } else {
    FPRINTF(ASCERR,"SlowVisitInstanceTree called with NULL.");
  }
}

static
void FastVisitTree(struct Instance *inst, VisitProc proc,int depth, int leaf)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (CheckVisitNumber(inst)){
    if (!depth) (*proc)(inst);
    if (leaf||NotAtom(inst)) {
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++)
        if ((child = InstanceChild(inst,c))!=NULL) {
          FastVisitTree(child,proc,depth,leaf);
        } else {
          WriteWhereNull(ASCERR,inst);
        }
    }
    if (depth) (*proc)(inst);
  }
}

static
void SilentVisitTree(struct Instance *inst,VisitProc proc, int depth, int leaf)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (CheckVisitNumber(inst)){
    if (!depth) (*proc)(inst);
    if (leaf||NotAtom(inst)) {
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++) {
        if ((child = InstanceChild(inst,c))!=NULL) {
          SilentVisitTree(child,proc,depth,leaf);
        }
      }
    }
    if (depth) (*proc)(inst);
  }
}

void FastVisitInstanceTree(struct Instance *inst, VisitProc proc,
        	       int depth, int leaf)
{
  global_visit_num++;
  AssertMemory(inst);
  FastVisitTree(inst,proc,depth,leaf);
}

void SilentVisitInstanceTree(struct Instance *inst, VisitProc proc,
        	       int depth, int leaf)
{
  global_visit_num++;
  AssertMemory(inst);
  SilentVisitTree(inst,proc,depth,leaf);
}

#define IVIT_MIN_LEN 20
static
void IndexedVisitCheckSize(unsigned long **llist, unsigned int *llen, int len)
{
  unsigned long *old;
  if ( *llen < (unsigned int)len || !(*llen) || llist == NULL) {
    old = *llist;
    if (*llist != NULL) {
      *llist = (unsigned long *)ascrealloc(*llist,
                         sizeof(unsigned long)*(*llen+IVIT_MIN_LEN));
    } else {
      *llist = ASC_NEW_ARRAY(unsigned long,IVIT_MIN_LEN);
    }
    if (*llist == NULL) {
      *llist = old;
      ASC_PANIC("insufficient memory");
    }
  }
}

static
void IndexedVisitTree(struct Instance *inst, IndexedVisitProc proc,
                      int depth, int leaf,
                      unsigned long **llist, unsigned int *llen, int len,
                      VOIDPTR userdata)
{
  unsigned long nc,c;
  struct Instance *child;
  int oldlen;

  AssertMemory(inst);
  if (CheckVisitNumber(inst)){
    if (!depth) (*proc)(inst,*llist,len,userdata);
    if (leaf||NotAtom(inst)) {
      oldlen = len;
      len++;
      nc = NumberChildren(inst);
      IndexedVisitCheckSize(llist,llen,len);
      for (c = 1; c <= nc; c++) {
        child = InstanceChild(inst,c);
        if (child != NULL) {
          (*llist)[oldlen] = c;
          IndexedVisitTree(child,proc,depth,leaf,llist,llen,len,userdata);
        }
      }
      len--;
    }
    if (depth) (*proc)(inst,*llist,len,userdata);
  }
}

void IndexedVisitInstanceTree(struct Instance *inst, IndexedVisitProc proc,
        	              int depth, int leaf, unsigned long **llist,
                              unsigned int *llen, VOIDPTR userdata)
{
  global_visit_num++;
  AssertMemory(inst);

  assert(llist != NULL);
  assert(llen != NULL);
  IndexedVisitCheckSize(llist,llen,IVIT_MIN_LEN);

  IndexedVisitTree(inst,proc,depth,leaf,llist,llen,0,userdata);
}
#undef IVIT_MIN_LEN

static
void SilentVisitTreeTwo(struct Instance *inst,
        	        VisitTwoProc proc,
        	        int depth, int leaf,VOIDPTR userdata)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (CheckVisitNumber(inst)){
    if (!depth) {
      (*proc)(inst,userdata);
    }
    if (leaf||NotAtom(inst)) {
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++) {
        if ( (child = InstanceChild(inst,c)) !=NULL) {
          SilentVisitTreeTwo(child,proc,depth,leaf,userdata);
        }
      }
    }
    if (depth) {
      (*proc)(inst,userdata);
    }
  }
}

static
void VisitTreeTwo(struct Instance *inst,
        	  VisitTwoProc proc,
        	  int depth, int leaf,VOIDPTR userdata)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (CheckVisitNumber(inst)){
    if (!depth) {
      (*proc)(inst,userdata);
    }
    if (leaf||NotAtom(inst)) {
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++) {
        if ( (child = InstanceChild(inst,c)) !=NULL) {
          VisitTreeTwo(child,proc,depth,leaf,userdata);
        } else  {
          WriteWhereNull(ASCERR,inst);
        }
      }
    }
    if (depth) {
      (*proc)(inst,userdata);
    }
  }
}


void SilentVisitInstanceTreeTwo(struct Instance *inst,
        		        VisitTwoProc proc,
        		        int depth, int leaf,
        		        VOIDPTR userdata)
{
  global_visit_num++;
  AssertMemory(inst);
  SilentVisitTreeTwo(inst,proc,depth,leaf,userdata);
}

void VisitInstanceTreeTwo(struct Instance *inst,
        		  VisitTwoProc proc,
        		  int depth, int leaf,
        		  VOIDPTR userdata)
{
  global_visit_num++;
  AssertMemory(inst);
  VisitTreeTwo(inst,proc,depth,leaf,userdata);
}

static
void SilentVisitFringeTwo(struct Instance *inst,
        	          VisitTwoProc proc, VisitTwoProc proc2,
        	          int depth, int leaf,VOIDPTR userdata)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (CheckVisitNumber(inst)){
    if (!depth) {
      (*proc)(inst,userdata);
    }
    if (leaf||NotAtom(inst)) {
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++) {
        if ( (child = InstanceChild(inst,c)) !=NULL) {
          SilentVisitFringeTwo(child,proc,proc2,depth,leaf,userdata);
        }
      }
    }
    if (depth) {
      (*proc)(inst,userdata);
    }
  } else {
    /* play it again sam */
    (*proc2)(inst,userdata);
  }
}

static
void VisitFringeTwo(struct Instance *inst,
        	    VisitTwoProc proc, VisitTwoProc proc2,
        	    int depth, int leaf,VOIDPTR userdata)
{
  unsigned long nc,c;
  struct Instance *child;
  AssertMemory(inst);
  if (CheckVisitNumber(inst)){
    if (!depth) {
      (*proc)(inst,userdata);
    }
    if (leaf||NotAtom(inst)) {
      nc = NumberChildren(inst);
      for(c=1;c<=nc;c++) {
        if ( (child = InstanceChild(inst,c)) !=NULL) {
          VisitFringeTwo(child,proc,proc2,depth,leaf,userdata);
        } else  {
          WriteWhereNull(ASCERR,inst);
        }
      }
    }
    if (depth) {
      (*proc)(inst,userdata);
    }
  } else {
    /* play it again sam */
    (*proc2)(inst,userdata);
  }
}


void SilentVisitInstanceFringeTwo(struct Instance *inst,
        		          VisitTwoProc proc,
        		          VisitTwoProc proc2,
        		          int depth, int leaf,
        		          VOIDPTR userdata)
{
  global_visit_num++;
  AssertMemory(inst);
  SilentVisitFringeTwo(inst,proc,proc2,depth,leaf,userdata);
}

void VisitInstanceFringeTwo(struct Instance *inst,
        		    VisitTwoProc proc,
        		    VisitTwoProc proc2,
        		    int depth, int leaf,
                            VOIDPTR userdata)
{
  global_visit_num++;
  AssertMemory(inst);
  VisitFringeTwo(inst,proc,proc2,depth,leaf,userdata);
}

static
void SilentVisitRootsTwo(struct Instance *inst, VisitTwoProc proc,
        	         int depth,VOIDPTR userdata)
{
  unsigned long nc,c;
  struct Instance *parent;
  AssertMemory(inst);
  if (CheckVisitNumber(inst)) {
    if (!depth) {
      (*proc)(inst,userdata);
    }
    nc = NumberParents(inst);
    for (c=1; c <= nc; c++) {
      parent = InstanceParent(inst,c);
      assert(parent != NULL);
      SilentVisitRootsTwo(parent,proc,depth,userdata);
    }
    if (depth) {
      (*proc)(inst,userdata);
    }
  }
}

void SilentVisitInstanceRootsTwo(struct Instance *inst, VisitTwoProc proc,
        	                int depth, VOIDPTR userdata)
{
  global_visit_num++;
  AssertMemory(inst);
  SilentVisitRootsTwo(inst,proc,depth,userdata);
}

/* struct visitmapinfo *MakeVisitMap(i,&len);
 * Returns a vector len long of visitmapinfos collected showing the instances
 * and how they are encountered in a visitation.
 * example: (i, icld, ignd are instance ptrs)
 *[parent,context,dir,child]
 * [NULL,root,DOWN,1],
 * [root,icld,DOWN,1],
 * [icld,ignd,-UP-,1],
 * [root,icld,-UP-,1],
 * [NULL,root,DOWN,2],
 * [root,icld,-UP-,1],
 * [NULL,root,-UP-,0].
 *
 * The visitation indicated by this mapping (both bottom-up and top-down)
 * is:
 * v<i>
 * v<i>.b
 * ^<i>.b.f
 * ^<i>.b
 * v<i>
 * ^<i>.c
 * ^<i>
 */
static
void MakeMap(struct Instance *inst,
             struct Instance *parent,
             unsigned long parents_child_num_of_i,
             unsigned long *lensofar,
             struct visitmapinfo *map)
{
  unsigned long c,nc;
  struct Instance *ch;
  /* account for the children if we haven't been here before */
  if (CheckVisitNumber(inst) && NotAtom(inst)) {
    nc = NumberChildren(inst);
    for (c=1; c <= nc; c++) {
      ch=InstanceChild(inst,c);
      if (ch != NULL) {
        map[*lensofar].parent = parent;
        map[*lensofar].context = inst;
        map[*lensofar].dir = vimap_DOWN;
        map[*lensofar].last = -1;
        map[*lensofar].child = c;
        (*lensofar)++;
        MakeMap(ch,inst,c,lensofar,map);
      }
    }
  }
  /* if been here before or not, must record seeing here. */
  map[*lensofar].parent = parent;
  map[*lensofar].context = inst;
  map[*lensofar].dir = vimap_UP;
  map[*lensofar].last = (int)GetTmpNum(inst);
  map[*lensofar].child = parents_child_num_of_i;
  SetTmpNum(inst,*lensofar);
  (*lensofar)++; 
}

/*
 * CalcMapSize(i,&len);
 * calculate the number of instances seen and reseen in a visit
 * and return it in len. Init len to 0 before calling.
 */
static
void CalcMapSize(struct Instance *inst,unsigned long *lensofar)
{
  unsigned long c,nc;
  struct Instance *ch;
  /* account for the children if we haven't been here before */
  if (CheckVisitNumber(inst) && NotAtom(inst)) {
    nc = NumberChildren(inst);
    for (c=1; c <= nc; c++) {
      ch=InstanceChild(inst,c);
      if (ch != NULL) {
        (*lensofar)++;
        CalcMapSize(ch,lensofar);
      }
    }
  }
  /* if been here before or not, must record seeing here. */
  SetTmpNum(inst,INT_MAX);
  (*lensofar)++; 
}

struct visitmapinfo *MakeVisitMap(struct Instance *root,unsigned long *len)
{
  struct visitmapinfo *map;
  unsigned long maplen;

  AssertMemory(root);
  global_visit_num++;
  *len = 0; /* we don't count root entry, since we don't know context. */
  CalcMapSize(root,len);
  map = ASC_NEW_ARRAY(struct visitmapinfo,(*len)+1);
  if (map==NULL) {
    return NULL;
  }
  /* put some crap at the END that should signal any reasonable program
   * should also check this for being same before freeing. 
   */
  map[*len].parent = NULL;
  map[*len].context = NULL;
  map[*len].child = 0UL;
  map[*len].last = INT_MAX;
  map[*len].dir = vimap_ERROR;
  /* record info */
  global_visit_num++;
  maplen = 0;
  MakeMap(root,NULL,0,&maplen,map);
  assert(map[*len].parent==NULL);
  assert(map[*len].context==NULL);
  assert(map[*len].child==0UL);
  assert(maplen==(*len));
  return map;
}
