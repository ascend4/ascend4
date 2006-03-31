/*
 *  anonmerge.c
 *  by Benjamin Andrew Allan
 *  Created September 21, 1997
 *  Copyright 1997 Carnegie Mellon University.
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: anonmerg.c,v $
 *  Date last modified: $Date: 2000/01/25 02:25:52 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
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
 *  The idea is to find and mark all the merges that need to be
 *  marked to disambiguate the anontype classification scheme
 *  which can be mislead into believing identical anontypes when
 *  one instance has some children merged but the other doesn't.
 *  Put another way, we want to record the minimum set of merges
 *  that can account for the identity of all variables/constants
 *  in any relation,logical relation, or when statement.
 *
 *  Assumptions:
 *  - We will perform this step at a point in the compilation
 *  sequence where the instance structures are set (any time after
 *  phase 1). Instances cannot move while detecting merges.
 *  - The tmpnums and interface pointers are not in use. (To be
 *  safe we will PushInterfacePointers. Tmpnums left 0.)
 *  - Universal instances need not be investigated for merge
 *  properties. That is, when enumerating a namespace and we
 *  encounter a universal instance, we will skip it and all
 *  its children. We should probably introduce a child-of-universal
 *  bit in the ChildList info and disallow merges/aliases of parts
 *  of universals with the outside in order to prevent graphs of
 *  universal instances from having connections anywhere except at
 *  the top. There are unofficial universals, and we will detect them.
 *
 *
 *  Desirables:
 *  - We want to mark instances with a minimal set of merges that accounts
 *  for the deviations in their name space, i.e. we don't want to collect
 *  the merges for all a.b.x, c.b.x if a.b, c.b are merged.
 *  - We want to mark the instance which is the smallest scope containing
 *  both merges. That is, a.b.c, a.b.d are merged, we want to mark a.b,
 *  not a.
 *
 *  Problems:
 *  Because there is no simple indexing of parents of an instance
 *  (i.e. I can't travel from a parent p to a child c and easily
 *  know which of the back (parent) links of c I should follow
 *  to return to the parent) we must store paths for navigation
 *  in terms of child number/instance context pairs. Following
 *  back a parent trail requires a search for the child being
 *  left which is unacceptable. This is the same as an io NameNode,
 *  unless we decide to play a reference count game to avoid memory
 *  consumption.
 *
 *  Much of this process is readily explained in terms of a global
 *  list of childnumber/context pairs which maps the entire namespace.
 *  Fortunately we don't need to build this list, we can build parts
 *  of it only, but the overall algorithm becomes obscured.
 *  Because we are mapping the LINK space, rather than the node space,
 *  we have to write our own bizarre set of VisitName functions.
 *  We will have to visit each link between any two objects once.
 *
 *  These functions maintain all their own data and do not use any
 *  global information, EXCEPT that they use instance InterfacePtrs.
 *  For this reason they are not thread-safe. They also build data
 *  structures with references to instances, so no instance moving
 *  actions should be performed while there are structures from this
 *  module in use.
 */


#include <limits.h> /* for INT_MAX */
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/ascPrint.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/listio.h>
#include <general/dstring.h>
#include "compiler.h"
#if TIMECOMPILER
#include <time.h>
#include <general/tm_time.h>
#endif
#include "fractions.h"
#include "dimen.h"
#include "child.h"
#include "type_desc.h"
#include "instance_enum.h"
#include "types.h"
#include "instance_types.h"
#include "instance_name.h"
#include "tmpnum.h"
#include "parentchild.h"
#include "instquery.h"
#include "visitinst.h"
#include "instance_io.h"
#include "visitinst.h"
#include "visitlink.h"
#include "arrayinst.h" /* for arrayvisitlocalleaves only */
#include "anonmerg.h"
#if AMSTAT
/* need numlistio to write debugging info */
#define NUMLISTEXPORTIO
#endif
#include "numlist.h"
#if AMSTAT
#undef NUMLISTEXPORTIO
#endif

#ifndef lint
static CONST char AnonMergeModuleID[] = "$Id: anonmerg.c,v 1.9 2000/01/25 02:25:52 ballan Exp $";
#endif

/* if want to compile unused functions, set to 1.
 */
#define AMUNUSED 0

/* print detail timing breakdown of merge detection */
#define AMBKDN 0
#define CFI 0

/* print node list info */
#define CHECKN (AMSTAT || 0)

/* if want to write reachability info as derived, set to 1. */
#define WRITEREACHABLE 0

/*
 * Number of buckets in the hash table to use for detecting universals.
 * Should bear some relation to the number of formal types in question.
 * Must match the mask in AMUHASH (buckets = 2^N, and mask = 2^N -1.)
 */
#define AMUBUCKETS 1024
/*
 * hash function to put a heap pointer in a bucket.
 */
#define AMUHASH(p) (((((long) (p))*1103515245) >> 20) & 1023)

/*
 * this is the flag that indicates an instance is one of a kind
 * within the scope of merge detection, therefore is effectively
 * UNIVERSAL.
 */
#define AMUFLAG 0x10

/* This is the flag in anonflags indicates that an amip is on the instance.
 */
#define AMIPFLAG 0x1


/* so on each node which has merged descendants that need to be recorded,
 * we need a list of lists of paths.
 * i->amip->amlist-|-------------|->path1 to d1 |||||
 *                 |             |->path2 to d1 |||||
 *                 |
 *                 |-------------|->path1 to d2 |||||
 *                 etc           |->path2 to d2 |||||
 *                               etc
 *
 *                 gllist        gllists        gllist of childnums
 * where each path is the gllist of childnumbers that is followed
 * to reach the shared instance di.
 */
/*
 */
struct AnonMergeIP {    /* Hangs off all instances with amipflag=1 */
  struct gl_list_t *amlist;
  /* The list of merges contained in this inst. each element is a
   * gl_list of paths. each path is a gl_list of child numbers.
   */
  int ip_number;	/* number of ip node (subgraph) */
  int node_number;      /* number of merged node, or 0 if this
                         * node is itself uninteresting.
                         * (shared)
                         */
  Numlist_p subgraph;   /* list of all ip'd nodes in subgraph(i) */
  Numlist_p shared;     /* list of merged nodes in subgraph(i) */
  Numlist_p parentslist;/* list of parent nodes of i */
  struct gl_list_t *glp;/* gl list of parent nodes of i. */
  /* number of merges recorded is gllen amlist. */
};

/* The existence of a bucket implies at least one instance of the
 * formal type exists. If no more than one exists, then its pointer
 * is stored in the bucket. If a second is found, the bucket becomes empty.
 * (b->i==NULL).
 * When the table is all built, mark anything that has a non-NULL
 * instance as locally-universal.
 */
struct AnonMergeUniBucket {
  CONST struct TypeDescription *d;   /* type = hash key */
  struct Instance *i;                /* could be a list later */
  struct AnonMergeUniBucket *next;   /* chain */
  unsigned long indirected;          /* needed for arrays */
};

struct AnonMergeVisitInfo {
  struct Instance *root;
  unsigned long nip;		/* instances to get IPs */
  unsigned long nim;		/* instances with multiple nonaliased parents */
  struct AnonMergeUniBucket *t[AMUBUCKETS];     /* hash table */
  unsigned long maxchildren;    /* most children of any instance */
};

struct sgelt {
  int sg;
  struct sgelt *next;
};

struct mdata {
  struct sgelt **header; /* [maxip] */
  int *colcount; /* [maxip] */
  int *sg2blob; /* [maxchildren] */
  int *blobcollected; /* [maxchildren] */
  struct gl_list_t **blob; /* [maxchildren] */
  int nextnode_ip;
  int sg;
  int maxchildren;
  pool_store_t pool;
};


/*
 * packet that persists while derived merge data is in use.
 */
struct AnonMergeIPData {
  struct gl_list_t *oldips;	/* result of puship */
  struct Instance *root;
  struct AnonMergeIP *ipbuf;    /* a mass allocated set of AMIPs */
  unsigned long iplen;          /* number of ips */
  unsigned int ipused;          /* number of slots from buf handed out */
  unsigned int ipback;          /* number of slots from buf returned */
  FILE *fp;

  int *listhints;               /* listhints[i] --> hint for graph i */
  int *node2ip;                 /* node2ip[node_number] --> ip_number */

  Numlist_p enlp0, enlp1,enlp2; /* scratch expandable Numlists */
  struct gl_list_t *nlpgl;      /* gllist of collected numlists */

  Numlist_p senlp0, senlp1,senlp2; /* scratch expandable Numlists */
  struct gl_list_t *snlpgl;      /* gllist of collected numlists */

  Numlist_p *mergegraphs;       /* array of shareds size totgraphs */
  Numlist_p *graphs;            /* array of subgraphs size totgraphs */
  unsigned long *graphchildnum; /* child of i that graph came from */
  int totgraphs; 	        /* number of children of i with subgraphs */

  Numlist_p *routes;            /* array of subgraphs size totroutes */
  unsigned long *routechildnum; /* child of i that route graph came from */
  int totroutes;      		/* number of children containing target */

  unsigned long *finalchildnum; /* children of i that yield merge path */
  int totfinal; 	        /* number of paths to target we record */

  struct gl_list_t *scratchpath;        /* scratch childnumber list */
  struct gl_list_t *scratchpathlist;    /* scratch list of paths */
  struct gl_list_t *scratchamlist;      /* scratch list of paths */

  int num_mergelists;		/* total number of N-path merges recorded */
  int num_iwithmerge;		/* total number of instances logging merges. */
  int compcount;

  struct mdata md;
};

/* given a pointer to an AM gl_list, destroys the list and sublists.
 * ignores NULL input.
 */
static
void DestroyMList(struct gl_list_t *aml)
{
  struct gl_list_t *pathlist, *path;
  unsigned long c,len,d,dlen;
  if (aml != NULL) {
    len = gl_length(aml);
    /* for each descendant */
    for ( c = 1; c <= len; c++) {
      pathlist = (struct gl_list_t *)gl_fetch(aml,c);
      dlen = gl_length(pathlist);
      /* for each path in list of paths to descendant */
      for ( d = 1; d <= dlen; d++) {
        path = (struct gl_list_t *)gl_fetch(pathlist,d);
        /* path is just a list of ints, so nothing to do to them */
        gl_destroy(path);
      }
      gl_destroy(pathlist);
    }
    gl_destroy(aml);
  }
}

/*
 * return an ip from the array of them we are managing
 */
static
struct AnonMergeIP *AMGetIP(struct AnonMergeIPData *amipd)
{
  struct AnonMergeIP *amip;
  assert(amipd!=NULL);
  if (amipd->iplen <= amipd->ipused) {
    Asc_Panic(2,"AMGetIP","Too many ips requested");
    gl_write_list(NULL,NULL); /* delete when done. forcing linker */
    return NULL; /* not reached */
  }
  amip = &(amipd->ipbuf[amipd->ipused]);
  (amipd->ipused)++;
  return amip;
}

/* Returns 1 if instance ch is of set, constant, array of set, or
 * array of constant type. The mergedness of these cannot affect
 * the meaning of models containing them, since the models' structures
 * are dependent on values of them, not identity.
 * Likewise returns 1 for relations, logical relations, whens,
 * and arrays thereof since these always have 1 parent and couldn't
 * be merged in any case. Likewise for dummy instance, which are UNIVERSAL.
 */
static
int IsIgnorableInstance(struct Instance *ch)
{
  assert(ch!=NULL);
  if (InstanceKind(ch)&IERRINST) {
    Asc_Panic(2,"IsIgnorableInstance","Called with bad instance");
    return 0; /* not reached */
  }
  if (InstanceKind(ch) & (IFUND | ICONS | IDUMB | IWHEN | IRELN | ILRELN)) {
    return 1;
  }
  if (IsArrayInstance(ch) ) {
    struct TypeDescription *d;
    d = GetArrayBaseType(InstanceTypeDesc(ch));
    if (GetBaseType(d) & ERROR_KIND) {
      Asc_Panic(2,"IsIgnorableInstance","Called with bad array instance");
      return 0; /* not reached */
    }
    if (GetBaseType(d) & (EQN_KIND | CONSTANT_KIND | DUMB_KIND | SET_KIND)) {
      return 1;
    }
  }
  return 0;
}

/*
 * Collects and merges into a cheaply allocated numlist (not expandable)
 * all the interesting descendant information from the children of i.
 * i itself is in the subgraph of i if i is interesting.
 * If nothing interesting is reachable, may return NULL.
 *
 * This function does not collect as 'reachable' constant and set
 * instance types since the 'identity' or lack thereof does not
 * change the semantics of these structural parameter instances.
 * We don't need to know about their mergedness.
 * Because we are numbering only the mergednodes we care about,
 * ignoring constants does not fill the maps with holes.
 *
 * shared is the list described above. This function also collects
 * subgraph, the list of both merged and compound nodes reachable
 * from i.
 *
 * We handle each child of i once and double up the other operations.
 */
static
void CollectSubgraphNumlist(struct Instance *i,
                            struct AnonMergeIPData *amipd,
                            struct AnonMergeIP *amip)
{
  unsigned long c,len, tn;
  struct Instance *ch;
  struct AnonMergeIP *chamip;
  Numlist_p shared;	/* list of shared nodes in the dag */
  Numlist_p subgraph;	/* list of shared and compound nodes in dag */
  ChildListPtr cl;

  NumpairClearList(amipd->enlp0);
  NumpairClearList(amipd->senlp0);
  gl_reset(amipd->nlpgl);
  gl_reset(amipd->snlpgl);
  len = NumberChildren(i);
  cl = (InstanceKind(i) == MODEL_INST)?GetChildList(InstanceTypeDesc(i)):NULL;
  for (c = 1; c <= len; c++) {
    ch = InstanceChild(i,c);
    if ( ch != NULL && (GetAnonFlags(ch) & AMIPFLAG) != 0 ) {
      chamip = (struct AnonMergeIP *)GetInterfacePtr(ch);
      /* collect child reachables and also their nums */
      if (chamip != NULL) {
        if (chamip->glp != NULL &&
            (cl == NULL || ChildAliasing(cl,c)==0)) {
          gl_append_ptr(chamip->glp,(void *)amip->ip_number);
        }
        if (chamip->shared != NULL) {
          gl_append_ptr(amipd->nlpgl, chamip->shared);
        }
        tn = chamip->node_number;
        if (tn != 0L) {
          NumpairAppendList(amipd->enlp0, tn);
        }

        if (chamip->subgraph != NULL) {
          gl_append_ptr(amipd->snlpgl, chamip->subgraph);
        }
        tn = chamip->ip_number;
        if (tn != 0L) {
          NumpairAppendList(amipd->senlp0, tn);
        }
      }
    }
  }
  if (amip->node_number != 0) {
    NumpairAppendList(amipd->enlp0, amip->node_number);
  }
  if (amip->ip_number != 0) {
    NumpairAppendList(amipd->senlp0, amip->ip_number);
  }
  gl_append_ptr(amipd->nlpgl, amipd->enlp0);
  gl_append_ptr(amipd->snlpgl, amipd->senlp0);
  shared = NumpairCombineLists(amipd->nlpgl, amipd->enlp1, amipd->enlp2);
  subgraph = NumpairCombineLists(amipd->snlpgl, amipd->senlp1, amipd->senlp2);
  NumpairClearList(amipd->enlp0);
  NumpairClearList(amipd->senlp0);
  amip->shared = shared;
  amip->subgraph = subgraph;
}

/* Grabs an ip from the buffer and inits/returns it.
 * When used with pushIP, decorates all instances that are marked with
 * AMIPFLAG which should only be models/arrays and shared variables.
 * As an interesting side effect, we collect the list of mergednodes in the
 * subgraph of i (in numlist_p form).  Should be applied bottom-up.
 * We also add i's number to the lists of parents in its children.
 * By dfbu, parent numbers will be added in increasing order, possibly
 * with the same parent appearing twice consecutively in the child's
 * parent list. Logging numbers here allows us to avoid sorting the
 * numbers later before creating a persistent numlistp.
 * Actually, to reduce work, we subcontract numbering the children
 * to CollectSubgraphNumlist.
 */
static
struct AnonMergeIP *AnonMergeCreateIP(struct Instance *i,
                                      struct AnonMergeIPData *amipd)
{
  struct AnonMergeIP *amip;

  assert(i!=NULL);

  amip = NULL;

  if (GetAnonFlags(i) & AMIPFLAG) {

    amip = AMGetIP(amipd);
    assert(amip!=NULL);
    amip->amlist = NULL;
    amip->subgraph = NULL;
    amip->glp = gl_create(NumberParents(i)+2);
    amip->parentslist = NULL;

    amip->node_number = (int)GetTmpNum(i);
    amip->ip_number = amipd->ipused; /* array loc in ipbuf +1 */
    gl_append_ptr(amip->glp,(void *)amip->ip_number);
    assert(amip->node_number >= 0);
    /* Node number is the dfbu numbering of shared instances,
     * the dfbu mergenode number in tmpnum of i.
     */
    SetTmpNum(i,0L);

    CollectSubgraphNumlist(i, amipd, amip);
  }
  return amip;
}

/* Hunts up node 'target' in the descendants of child 'start' of i.
 * Resets content of scratch on entry.
 * Fills path 'scratch' with childnumbers that can be followed to
 * get to target. Scratch should not be NULL.
 * Could return target instance for free if needed.
 * This function cannot fail if the shared lists in the amip are
 * correct.
 * Scratch result should be copied.
 */
static
void AnonMergeFindPath(struct Instance *i, int target,
              unsigned long start, struct gl_list_t *scratch)
{
#define AMFPDEBUG 0
  unsigned long c;
  struct Instance *ch = NULL; /* unnec init to avoid warning */
  struct AnonMergeIP *amip;
  int notfound, notchild;
  ChildListPtr cl;
#if AMFPDEBUG
  int first = 1;
#endif

  assert(scratch != NULL);
  assert(InstanceChild(i,start) != NULL);

  /* start out knowing the start child, so we don't search much on 1st one */
  gl_reset(scratch);
  c = start;
  notfound = 1;
  /* ch will always hit InstanceChild assignment before being used
   * when this function is called with a correct start value.
   */
  while (notfound) { /* entered at least once */
    notchild = 1;
    if (InstanceKind(i) == MODEL_INST) {
      cl = GetChildList(InstanceTypeDesc(i));
    } else {
      cl = NULL;
    }
    while (notchild) { /* entered at least once */
      /* must succeed on first trip if start is correct */
      /* hunt for next child of i which has target as a merged descendant */
      ch = InstanceChild(i,c);
      if (ch != NULL &&
          (GetAnonFlags(ch) & AMIPFLAG) != 0 &&
          (cl == NULL || ChildAliasing(cl,c) == 0)) {
        amip = (struct AnonMergeIP *)GetInterfacePtr(ch);
        assert(amip != NULL);
        if (NumpairNumberInList(amip->shared,(int)target)) { /*nnl*/
          notchild = 0;
          if (amip->node_number == target) {
            notfound = 0;
          }
          gl_append_ptr(scratch,(void *)c);
        }
      } /* else child can't be it. go to next. */
#if AMFPDEBUG
      if (first) {
        first = 0;
        if (notchild) {
          FPRINTF(ASCERR,"AMFindPath missed on given child %lu\n",start);
        }
      }
#endif
      c++;
    }
    /* descend to child found. hunt for next left to right. */
    c = 1;
    i = ch; /* so when at last located, i becomes target instance. */
  }
  return;
}

static
void AnonWritePath(FILE *fp, struct Instance *i, struct gl_list_t *path)
{
#define AWPDB 0 /* debugging for this function */
  struct Instance *root;
  unsigned long c,len, cn;
#if AWPDB
  int ip,im;
  struct AnonMergeIP *amip;
#endif
  struct InstanceName name;

  assert(i != NULL);
  assert(path != NULL);

  len = gl_length(path);
  for (c = 1; c <= len; c++) {
    root = i;
    cn = (unsigned long)gl_fetch(path,c);
    i = InstanceChild(root,cn);
#if AWPDB
    if (GetAnonFlags(i)&AMIPFLAG) {
      amip = (struct AnonMergeIP *)GetInterfacePtr(i);
      assert(amip != NULL);
      ip = amip->ip_number;
      im = amip->node_number;
    } else {
      im = ip = -1;
    }
    FPRINTF(fp,"(%d){%d}",ip,im);
#endif
    name = ChildName(root,cn);
    switch (InstanceNameType(name)){
    case StrName:
      if (c>1) {
        PUTC('.',fp);
      }
      FPRINTF(fp,SCP(InstanceNameStr(name)));
      break;
    case IntArrayIndex:
      FPRINTF(fp,"[%ld]",InstanceIntIndex(name));
      break;
    case StrArrayIndex:
      FPRINTF(fp,"['%s']",SCP(InstanceStrIndex(name)));
      break;
    }
  }
  FPRINTF(fp,"\n");
}

static
void WriteMList(FILE *fp, struct gl_list_t *aml, struct Instance *i)
{
  struct gl_list_t *pathlist, *path;
  unsigned long c,len;
  unsigned long d,dlen;
  unsigned long elen;

  if (aml !=NULL) {
    len = gl_length(aml);
    for (c = 1; c <= len; c++) { /* over each merge bush */
      pathlist = (struct gl_list_t *)gl_fetch(aml,c);
      assert(pathlist != NULL);
      dlen = gl_length(pathlist);
      assert(dlen != 0);
      FPRINTF(fp,"%lu ALTERNATE PATHS to ",dlen);
      path = (struct gl_list_t *)gl_fetch(pathlist,1);
      assert(pathlist != NULL);
      AnonWritePath(fp,i,path);
      /* write the list of paths */
      FPRINTF(fp," //\n");
      for (d = 1; d <= dlen; d++) {
        path = (struct gl_list_t *)gl_fetch(pathlist,d);
        elen = gl_length(path);
        AnonWritePath(fp,i,path);
      }
    }
  }
}


#if AMSTAT
static
void AnonMergeLogIP(struct Instance *i, struct AnonMergeIPData *amipd)
{
  struct AnonMergeIP *amip;
  if (GetAnonFlags(i)&AMIPFLAG) {
    amip = (struct AnonMergeIP *)GetInterfacePtr(i);
    if (amip->amlist != NULL) {
      FPRINTF(amipd->fp,"%lu, ", gl_length(amip->amlist));
      WriteInstanceName(amipd->fp,i,NULL);
      FPRINTF(amipd->fp,"\n");
      WriteMList(amipd->fp,amip->amlist,i);
      FPRINTF(amipd->fp,"\n");
    }
  }
}
#endif
/*
 * clears lists in amip, if any.
 */
static
void AnonMergeDestroyIP(struct Instance *i,
                        struct AnonMergeIP *amip,
                        struct AnonMergeIPData *amipd)
{
  (void)i;
  DestroyMList(amip->amlist);
#if AMSTAT
  if (amip->ip_number !=0 || amip->node_number != 0) {
    FPRINTF(amipd->fp,"ip# %d   mn# %d  ",amip->ip_number,amip->node_number);
    WriteInstanceName(amipd->fp,i,NULL);
    FPRINTF(amipd->fp,"\n");
  }
#endif
  amip->amlist = NULL;
  if (amip->subgraph != NULL) {
    NumpairDestroyList(amip->subgraph);
    amip->subgraph = NULL;
  }
  if (amip->glp != NULL) {
    gl_destroy(amip->glp);
    amip->glp = NULL;
  }
  if (amip->parentslist != NULL) {
    NumpairDestroyList(amip->parentslist);
    amip->parentslist = NULL;
  }
  if (amip->shared != NULL) {
    NumpairDestroyList(amip->shared);
    amip->shared = NULL;
  }
  amipd->ipback++;
}


#define THASH 0
static
struct AnonMergeUniBucket *FindUniBucket(struct TypeDescription *d,
                                         unsigned long indirected,
                                         struct AnonMergeUniBucket **t)
{
  struct AnonMergeUniBucket *result;
  int index;
  index = AMUHASH(SCP(GetName(d)));
  result = t[index];
  while (result != NULL &&
          ( d != result->d ||
            (indirected != LONG_MAX && indirected != result->indirected)
          )
        ) {
    result = result->next;
  }
#if THASH
  FPRINTF(ASCERR,"FUB:\t%d\tind %lu\t%s\t%lu\n",
          index,indirected,SCP(GetName(d)),(unsigned long)result);
#endif
  return result;
}

/*
 * caller is responsible for setting b->i.
 */
static
struct AnonMergeUniBucket *AddUniBucket(struct TypeDescription *d,
                                        unsigned long indirected,
                                        struct AnonMergeUniBucket **t)
{
  struct AnonMergeUniBucket *b;
  int index;
  b = (struct AnonMergeUniBucket *)ascmalloc(sizeof(struct AnonMergeUniBucket));
  if (b==NULL) {
    return NULL;
  }
  index = AMUHASH(SCP(GetName(d)));
  b->next = t[index];
  t[index] = b;
  b->d = d;
  b->indirected = indirected;
  return b;
}
/*
 * Clears the table (free the buckets) and any bucket
 * for implying only 1 instance, mark the instance as
 * being universal and mark it as having 0 parent references
 * because we don't care about its merges.
 */
static
void AnonMergeMarkUniversals(struct AnonMergeVisitInfo *amvi)
{
  int i;
  struct AnonMergeUniBucket **t;
  struct AnonMergeUniBucket *b;
  t = amvi->t;
  for (i = 0; i < AMUBUCKETS; i++) {
    while (t[i] != NULL) {
      b = t[i];
      t[i] = b->next;
      if (b->i != NULL) {
        SetAnonFlags(b->i,AMUFLAG); /* this must also catch explicit univs */
        SetTmpNum(b->i,0L);
#if (AMSTAT && 1)
        WriteInstanceName(ASCERR,b->i,NULL);
        FPRINTF(ASCERR," UNIVERSAL\n");
#endif
      }
      ascfree(b);
    }
  }
}

/*
 * AnonMergeCountTree(i,amvi);
 * This must be applied bottom up.
 * Sets the anonflags to 0 for all i.
 * Sets the tmpnum of i to zero.
 * Increment tmpnum in each child of compound i which is not an alias child of
 * i or a constant type. Constants are fully accounted for in their individual
 * anontype, so sharedness does not further affect instance semantics -- at
 * least so far as equations are concerned.  We deduct the array alii
 * contributions from the MODEL level where we find the alias statement
 * because it is hard to do it in the array scope.
 *
 * When visit done with this function, every nonconstant instance ends with
 * tmpnum set to the number of parents it is connected to in a non-alias way.
 *
 * Sometimes and incorrectly:
 * Counts total number of children, parents, extra parents, universals,
 * parents of universals;
 * These will be overestimates because the visitinstance
 * functions do not ignore universals, even though we will.
 */
void AnonMergeCountTree(struct Instance *i, struct AnonMergeVisitInfo *amvi)
{
  unsigned long c,len;
  struct AnonMergeUniBucket *b;
  struct Instance *ch;
  ChildListPtr cl;
#if AMSTAT
  unsigned long tmp;
  (void) tmp;
#endif

  SetAnonFlags(i,0);
  SetTmpNum(i,0);

  /* we need to find if all instances are universal or at least of
   * locally unique formal type. Inventory them here.
   */
  b = FindUniBucket(InstanceTypeDesc(i),InstanceIndirected(i),amvi->t);
  if (b == NULL) {
    b = AddUniBucket(InstanceTypeDesc(i),InstanceIndirected(i),amvi->t);
    b->i = i;         /* remember first, hopefully only, instance */
  } else {
    b->i = NULL;      /* more than 1 instance. not universal */
  }

  if (IsCompoundInstance(i)) {
    /* Mark increment parent link count on interesting children.  Elsewhere
     * all compound instances and all the ones that happen to have multiple
     * non-alias parents will get amips. All those with with multiple
     * non-alias parents will eventually get tmpnums reassigned dfbu.
     */
    len = NumberChildren(i);
    if (len > amvi->maxchildren) {
      amvi->maxchildren = len;
    }
    if (InstanceKind(i) == MODEL_INST) {
      cl = GetChildList(InstanceTypeDesc(i));
      assert(cl!=NULL);
      for (c = 1; c <= len; c++) {
        if ((ch = InstanceChild(i,c)) != NULL && !IsIgnorableInstance(ch)) {
          /*ignore const/set/reln/dummy/aryofsame ch*/
          if ( !ChildAliasing(cl,c) ) {
            /* record existence of a real link to ch */
            IncrementTmpNum(ch);
          } else {
            /* ALIASES. array possibly. */
            if (IsArrayInstance(ch)) {
              /* Now, in the context of the MODEL, we can spot if a child is
               * an alias array. If it is, then we want to decrement the
               * reference counts on the leaves of the array, but not on the
               * array_insts, unless those leaves happen to be other arrays.
               * It may happen that there are no leaves if the array is
               * defined over a NULL set in some portion.  So for example if
               * c[8][9] IS_A real; b[1..2][4..5] ALIASES c;
               * then the instance b has 4 subscripts to the end user,
               * but the 'leaves' of b which we don't want to
               * count as parents of elements c[i] are b[i][j].
               * Go NumberofDereferences(ch) and delete that contribution to
               * the number of parents counts.  Add this to arrayinst.c.
               */
              ArrayVisitLocalLeaves(ch,(AVProc)DecrementTmpNum);
            }
          }
        }
      }
    } else {
      assert(IsArrayInstance(i)); /* must be array since not MODEL */
      for (c = 1; c <= len; c++) {
        if ( (ch = InstanceChild(i,c)) != NULL) {
          if (!IsIgnorableInstance(ch)) {
            /* do not care about merges on arrays of constants/sets/relns
             * arrays of same since the anontype includes value and
             * structure doesn't matter if value is the same.
             */
            IncrementTmpNum(ch);
          }
        }
      }
    }
  }
}

/* Sets the tmpnum to mergenode dfbu if tmpnum currently > 1.
 * Sets the tmpnum to 0 if tmpnum currently 1.
 * Does nothing to tmpnum if tmpnum 0.
 * Counts the instance toward the number of ips needed if it is
 * compound or its final tmpnum is not 0.
 * This must be called after universals are marked, else the universal
 * marking may stop on the anonflags.
 */
static
void AnonMergeLabelNodes(struct Instance *i, struct AnonMergeVisitInfo *amvi)
{
  assert(GetTmpNum(i) <10000000);
  switch (GetTmpNum(i)) {
  case 0:
    /* all ignorable stuff should fall in here */
    break;
  case 1:
    assert(IsIgnorableInstance(i)==0);
    /* we're not supposed to be marking ignorable instances tmpnums at all
     * in the net.
     */
    SetTmpNum(i,0L);
    break;
  default:
    /* more than 1 real parent */
    (amvi->nim)++;
    assert(IsIgnorableInstance(i)==0);
    SetTmpNum(i,(unsigned long)amvi->nim);
    assert(GetTmpNum(i) <100000000);
    /* compound or not it gets an IP */
    (amvi->nip)++;
    SetAnonFlags(i,(GetAnonFlags(i) | AMIPFLAG));
    return;
  }
  if (IsCompoundInstance(i)) {
    /* nonmerged instance or uninteresting instance. check compoundness
     * as even uninteresting, unmerged compound instances need a space
     * to hang their subgraph numlist. universals included.
     */
    SetAnonFlags(i,(GetAnonFlags(i) | AMIPFLAG));
    (amvi->nip)++;
  }
}

/*
 * prints out the statistics of academic interest.
 * does not compile (cannot) unless amstat == TRUE.
 */
#if (AMSTAT)
static
void AMCWriteCounts(struct AnonMergeVisitInfo *amvi)
{
  FPRINTF(ASCERR,"AnonMerge statistics:\n");
  FPRINTF(ASCERR,"\tMerged Instances:\t%lu\n",amvi->nim);
  FPRINTF(ASCERR,"\tIP Instances:\t%lu\n",amvi->nip);
}
#endif /* amstat */

static
void AMVIInit(struct AnonMergeVisitInfo *amvi,struct Instance *root)
{
  int i;
  assert(amvi!=NULL);
  assert(root!=NULL);
  amvi->root = root;
  amvi->nip = 0L;
  amvi->nim = 0L;
  amvi->maxchildren = 0L;
  for (i = 0; i < AMUBUCKETS; i++) {
    amvi->t[i] = NULL;
  }
}

#define SP_LEN 2
#if (SIZEOF_VOID_P == 8)
#define SP_WID 252
#else
#define SP_WID 504
#endif
/* retune spwid if the size of sgelt changes dramatically */
#define SP_ELT_SIZE (sizeof(struct sgelt))
#define SP_MORE_ELTS 1
/* Number of slots filled if more elements needed.
   So if the pool grows, it grows by SP_MORE_ELTS*SP_WID elements at a time. */
#define SP_MORE_BARS 50
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

static
void InitSgeltPool(struct mdata *md) {
  if (md->pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: InitSgeltPool called twice.\n");
  }
  md->pool =
    pool_create_store(SP_LEN, SP_WID, SP_ELT_SIZE, SP_MORE_ELTS, SP_MORE_BARS);
  if (md->pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: InitSgeltPool unable to allocate pool.\n");
  }
}

static
void DestroySgeltPool(struct mdata *md) {
  if (md->pool==NULL) return;
  pool_destroy_store(md->pool);
  md->pool = NULL;
}

#if 0
/* UNUSED */
static
void ReportSgeltPool(FILE *f,struct mdata *md)
{
  if (md->pool==NULL) {
    FPRINTF(f,"SgeltPool is empty\n");
  }
  FPRINTF(f,"SgeltPool ");
  pool_print_store(f,md->pool,0);
}
#endif

/* create amipd and init all its bits */
static
struct AnonMergeIPData *AMIPDInit(struct AnonMergeVisitInfo *amvi)
{
  size_t asize;
  struct AnonMergeIPData *amipd;
  int i,len;
  unsigned long j,dlen;

  amipd = (struct AnonMergeIPData *)ascmalloc(sizeof(struct AnonMergeIPData));
  if (amipd==NULL) {
    ascfree(amipd);
    Asc_Panic(2,"AMIPDInit","Insufficent memory for amipd.");
    return NULL;
  }
  amipd->root = amvi->root;
  amipd->iplen = amvi->nip;
  amipd->num_mergelists = 0;
  amipd->num_iwithmerge = 0;
  amipd->node2ip = (int *)ascmalloc(sizeof(int)*(amvi->nim+1));
  if (amipd->node2ip == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for node2ip.");
    return NULL;
  }
  amipd->ipbuf = (struct AnonMergeIP *)
                   ascmalloc(sizeof(struct AnonMergeIP)*amvi->nip);
  if (amipd->ipbuf==NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for ipbuf.");
    return NULL;
  }
  amipd->ipused = 0;
  amipd->ipback = 0;
  amipd->compcount = 1;
  amipd->enlp0 = NumpairExpandableList(NULL,100);
  amipd->enlp1 = NumpairExpandableList(NULL,100);
  amipd->enlp2 = NumpairExpandableList(NULL,100);
  amipd->nlpgl = gl_create(100);
  if (amipd->enlp0 == NULL ||
      amipd->enlp1 == NULL ||
      amipd->enlp2 == NULL ||
      amipd->nlpgl == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for scratch nlps.");
    return NULL;
  }
  amipd->senlp0 = NumpairExpandableList(NULL,100);
  amipd->senlp1 = NumpairExpandableList(NULL,100);
  amipd->senlp2 = NumpairExpandableList(NULL,100);
  amipd->snlpgl = gl_create(100);
  if (amipd->senlp0 == NULL ||
      amipd->senlp1 == NULL ||
      amipd->senlp2 == NULL ||
      amipd->snlpgl == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for scratch snlps.");
    return NULL;
  }
  amipd->oldips = PushInterfacePtrs(amvi->root,(IPFunc)AnonMergeCreateIP,
                                    amvi->nip,1, (VOIDPTR)amipd);

  asize = amvi->maxchildren; /* now some buffers of this size */

  amipd->listhints = (int *)ascmalloc(asize*sizeof(int));
  amipd->graphs = (Numlist_p *)ascmalloc(asize*sizeof(Numlist_p));
  amipd->mergegraphs = (Numlist_p *)ascmalloc(asize*sizeof(Numlist_p));
  amipd->graphchildnum =
      (unsigned long *)ascmalloc(asize*sizeof(unsigned long));
  amipd->routes = (Numlist_p *)ascmalloc(asize*sizeof(Numlist_p));
  amipd->routechildnum =
      (unsigned long *)ascmalloc(asize*sizeof(unsigned long));
  amipd->finalchildnum =
      (unsigned long *)ascmalloc(asize*sizeof(unsigned long));
  /* totfinal, totroute, totgraph are all 'in use' counters, not
   * the size of allocation. amvi->maxchildren is the capacity.
   */
  if (amipd->graphs == NULL ||
      amipd->mergegraphs == NULL ||
      amipd->graphchildnum == NULL ||
      amipd->routes == NULL ||
      amipd->routechildnum == NULL ||
      amipd->finalchildnum == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for subgraph lists.");
    return NULL;
  }
  amipd->scratchpath = gl_create(100);
  amipd->scratchpathlist = gl_create(100);
  amipd->scratchamlist = gl_create(100);
  if (amipd->scratchpath == NULL ||
      amipd->scratchpathlist == NULL ||
      amipd->scratchamlist == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for scratch lists.");
    return NULL;
  }
  /* setup md */
  amipd->md.header = (struct sgelt **)
      ascmalloc(sizeof(struct sgelt *)*(amvi->nip+1));
  if (amipd->md.header == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for md.header.");
    return NULL;
  }
  amipd->md.colcount = (int *)ascmalloc(sizeof(int)*(amvi->nip+1));
  if (amipd->md.colcount == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for md.colcount.");
    return NULL;
  }
  amipd->md.sg2blob = (int *)ascmalloc(sizeof(int)*(asize+1));
  if (amipd->md.sg2blob == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for md.sg2blob.");
    return NULL;
  }
  amipd->md.blobcollected = (int *)ascmalloc(sizeof(int)*(asize+1));
  if (amipd->md.blobcollected == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for md.blobcollected.");
    return NULL;
  }
  amipd->md.blob = (struct gl_list_t **)
      ascmalloc(sizeof(struct gl_list_t *)*(asize+1));
  if (amipd->md.blob == NULL) {
    Asc_Panic(2,"AMIPDInit","Insufficent memory for md.blob.");
    return NULL;
  }
  for (i=0;i <= (int)asize;i++) {
    /* as we'll never use most of these, create them small and
     * let them grow.
     */
    amipd->md.blob[i] = gl_create(2);
    if (amipd->md.blob[i] == NULL) {
      Asc_Panic(2,"AMIPDInit","Insufficent memory for md.blob data.");
      return NULL;
    }
  }
  amipd->md.maxchildren = asize;
  amipd->md.nextnode_ip = 0;
  amipd->md.sg = 0;
  amipd->md.pool = NULL;
  InitSgeltPool(&(amipd->md));
  /* */
  len = amipd->ipused;
  for (i=0; i < len; i++) {
    struct gl_list_t *gl;
    gl = amipd->ipbuf[i].glp;
    if (gl != NULL) {
      if ((dlen=gl_length(gl)) > 0) {
        NumpairClearList(amipd->enlp0);
        for (j=1; j <= dlen; j++) {
          NumpairAppendList(amipd->enlp0,(int)gl_fetch(gl,j));
        }
        amipd->ipbuf[i].parentslist = NumpairCopyList(amipd->enlp0);
      }
      amipd->ipbuf[i].glp = NULL;
      gl_destroy(gl);
    }
  }
  return amipd;
}
/* deallocate amipd */
static
void DestroyAMIPD(struct AnonMergeIPData *amipd) {
  int i;
  ascfree(amipd->ipbuf);
  ascfree(amipd->node2ip);
  ascfree(amipd->listhints);
  DestroySgeltPool(&(amipd->md));
  ascfree(amipd->md.header);
  ascfree(amipd->md.colcount);
  ascfree(amipd->md.sg2blob);
  ascfree(amipd->md.blobcollected);
  for (i=0; i <= amipd->md.maxchildren; i++) {
    gl_destroy(amipd->md.blob[i]);
  }
  ascfree(amipd->md.blob);
  gl_destroy(amipd->nlpgl);
  NumpairDestroyList(amipd->enlp0);
  NumpairDestroyList(amipd->enlp1);
  NumpairDestroyList(amipd->enlp2);
  gl_destroy(amipd->snlpgl);
  NumpairDestroyList(amipd->senlp0);
  NumpairDestroyList(amipd->senlp1);
  NumpairDestroyList(amipd->senlp2);
  ascfree(amipd->mergegraphs);
  ascfree(amipd->graphs);
  ascfree(amipd->routes);
  ascfree(amipd->graphchildnum);
  ascfree(amipd->routechildnum);
  ascfree(amipd->finalchildnum);
  gl_destroy(amipd->scratchpath);
  gl_destroy(amipd->scratchpathlist);
  gl_destroy(amipd->scratchamlist);
  ascfree(amipd);
}

#if CHECKN
static
void AMCheckN(struct Instance *i, struct AnonMergeIPData *amipd)
{
  struct AnonMergeIP *amip;
  if ((GetAnonFlags(i) & AMIPFLAG) == 0) {
    return;
  } else {
    /* get amip for use in the rest of this function. */
    amip = (struct AnonMergeIP *)GetInterfacePtr(i);
  }
  if (amip->node_number > 0) {
    if (amip->shared != NULL && NumpairListLen(amip->shared)>0) {
      FPRINTF(amipd->fp,"snlist %d len=%d\n", amip->node_number,
        NumpairListLen(amip->shared));
    }
  }
  if (amip->ip_number > 0) {
    if (amip->subgraph != NULL && NumpairListLen(amip->subgraph)>0) {
      FPRINTF(amipd->fp,"iplist %d len=%d\n", amip->ip_number,
        NumpairListLen(amip->subgraph));
    }
  }
}
#endif

static
Numlist_p GetParentsList(int p, struct AnonMergeIPData *amipd)
{
  assert(amipd != NULL);
  assert(p>0);
  assert (amipd->ipbuf[p-1].ip_number == p);
  return amipd->ipbuf[p-1].parentslist;
}

static
void ZeroArrayEntry(int i, struct mdata *md)
{
  md->header[i] = NULL;
  md->colcount[i] = 0;
}

/*
 * Move content of all blobs indicated in bl into the biggest.
 * renumbers sg2blob of smaller blobs to match bigblob.
 */
static
void MergeBlobs(struct mdata *md, int bigblob, struct gl_list_t *bl)
{
  struct gl_list_t *keep, *old;
  unsigned long c,len, blc,bllen;
  int oldblob, oldsg;
  keep = md->blob[bigblob];
  for (blc=1,bllen = gl_length(bl); blc <= bllen; blc++) {
    oldblob = (int)gl_fetch(bl,blc);
    if (oldblob != bigblob) {
      old = md->blob[oldblob];
      for (c=1, len = gl_length(old); c <= len; c++) {
        oldsg = (int)gl_fetch(old,c);
#if 1
        /* check the supposition that we are keeping lists nonoverlapping */
        assert(gl_ptr_search(keep,(VOIDPTR)oldsg,0)==0);
#endif
        gl_append_ptr(keep,(VOIDPTR)oldsg); /* add to new blob */
        md->sg2blob[oldsg] = bigblob; /* reassign sg2blob lookup */
      }
    }
  }
}

#define ELTMALLOC (struct sgelt *)pool_get_element(md->pool)
/* ELTFREE is not defined, as we use pool reset instead */
#define ELTRESET(md) pool_clear_store((md)->pool)
/*
 * Build a sparse matrix of sorts col indexed by parent ip
 * with 'rows' indexing by sg, the reduced child number of i.
 * there is no rowwise access to this because we don't need it.
 * elts must be pooled or we spend eternity in malloc.
 */
static
void add_matrix_incidence(int parent, struct mdata *md)
{
  struct sgelt *elt;
  if (parent > md->nextnode_ip) {
#if CFI
    FPRINTF(ASCERR,"mat row %d col %d\n",md->sg,parent);
#endif
    elt = ELTMALLOC;
    elt->sg = md->sg;
    elt->next = md->header[parent];
    md->header[parent] = elt;
    md->colcount[parent]++;
  } else {
#if (CFI && 0)
    FPRINTF(ASCERR,"ign row %d col %d\n",md->sg,parent);
#endif
  }
}

/* can we move some of this into md? */
static
void CalcFinalIndependentRoutes(struct mdata *md,
                                Numlist_p parentsini,
                                Numlist_p CONST *routes,
                                Numlist_p parentsinchild,
                                struct gl_list_t *blobsincol,
                                struct gl_list_t *newsgincol,
                                unsigned long * CONST finalchildnum,
                                unsigned long * CONST routechildnum,
                                int * CONST totfinal,
                                CONST int totroutes)
{
  unsigned long newsg,blc,bllen;
  struct sgelt *elt;
  int blob;
  int bigblob;
  unsigned long bigsize;
  int p,sg,hint;

  /* init work space */
  NumpairListIterate(parentsini,(NPLFunc)ZeroArrayEntry,(VOIDPTR)md);
  /* collect connection matrix */
  for (sg = 0; sg < totroutes; sg++) {
    md->sg = sg;
    md->sg2blob[sg] = -1;
    md->blobcollected[sg] = 0;
    gl_reset(md->blob[sg]);
    NumpairCalcIntersection(parentsini,routes[sg],parentsinchild);
    NumpairListIterate(parentsinchild,(NPLFunc)add_matrix_incidence,
                       (VOIDPTR)md);
  }
  /* process columns of matrix to group rows into overlapping blobs */
  /* while processing a column, blobcollected indicates if we've
   * already seen that blob in this column.
   */
  p = 0;
  hint = -1;
  while ( p = NumpairPrevNumber(parentsini,p, &hint),
          p > md->nextnode_ip) {
#if CFI
    FPRINTF(ASCERR,"parent %d count %d\n",p,md->colcount[p]);
#endif
    switch(md->colcount[p]) {
    case 0: /* target appears directly in child list of source */
      /* At least one element of sg2blob will stay -1.
       * We'll need that sg at the end as it is an independent path.
       */
      break;
    case 1:
      sg = md->header[p]->sg;
      if (md->sg2blob[sg] == -1) {
        /* new blob for sg, and new sg for new blob */
        md->sg2blob[sg] = sg;
        gl_append_ptr(md->blob[sg], (VOIDPTR)sg);
      }
      /* else this parent is already seen by some other
       * child of the source.
       */
      break;
    default:
      elt = md->header[p];
      if (elt != NULL) {
        /* must be at least a blob or an unblobbed sg in the column */
        bigblob = -1; /* sg of the biggest seen so far */
        bigsize = 0L;
        gl_reset(blobsincol); /* nonredundant list of blobs seen in col */
        /* each blob is a nonredundant list of sg in the blob */
        gl_reset(newsgincol); /* list of sg new in column not in any blob */
        /* Compute list of existing blobs in col. These must be
         * merged if there are more than 1.
         * Compute list of unblobbed (as yet) parentsini. If no
         * existing blobs, these form a new blob, else
         * we add these to the existing blobs.
         * With this collection method, we never need merge overlapping
         * blobs because we never create a blob that contains any member
         * of an existing blob. At the end of the surrounding loop, we
         * will have only nonoverlapping blobs and direct children (-1)
         * left in matrix.sg2blob. We want to keep the lowest sg of each
         * blob in sg2blob.
         */
        while (elt != NULL) {
          blob = md->sg2blob[elt->sg];
          if (blob == -1) {
            /* never seen this child of source */
            gl_append_ptr(newsgincol,(VOIDPTR)elt->sg);
          } else {
            if (md->blobcollected[blob] == 0) {
              /* never seen this blob, blob must be nonempty */
              assert(gl_length(md->blob[blob]) != 0);
              if (gl_length(md->blob[blob]) > bigsize) {
                /* track which of collected blobs is biggest */
                bigsize = gl_length(md->blob[blob]);
                bigblob = blob;
              }
              gl_append_ptr(blobsincol,(VOIDPTR)blob);
              md->blobcollected[blob] = 1;
            }
            /* else saw blob before. */
          }
          elt = elt->next;
        } /* endwhile */
        /* By construction, last element of newsgincol
         * is also the lowest sg in col because sg are inserted in
         * increasing order at header in the matrix assembly.
         */
        if (gl_length(blobsincol) == 0L) {
          assert(bigblob == -1);
          /* get list index of least sg to use as new blob */
          bllen = (int)gl_length(newsgincol);
#if CFI
          FPRINTF(ASCERR,"No blobs. %lu newsg\n",bllen);
#endif
          assert(bllen > 0);
            /* fails mysteriously when all sg in first blob.
             * Why is blobsincol empty? because someone left blobcollected 1.
             */
            /* new blob */
          blob = (int)gl_fetch(newsgincol,bllen); /* new blob number:last */
          assert(md->blob[blob] != NULL);
          assert(gl_length(md->blob[blob]) == 0);
          /* add sg to new blob and assign new blob to those sg */
          for (blc = 1; blc <= bllen; blc++) {
            newsg = (int)gl_fetch(newsgincol,blc);
            gl_append_ptr(md->blob[blob],(VOIDPTR)newsg);
            md->sg2blob[newsg] = blob;
          }
        } else {
#if CFI
          FPRINTF(ASCERR,"blobs: %lu.\n",gl_length(blobsincol));
#endif
          assert(bigblob >= 0);
          /* renumber smaller blobs in blobsincol to be members of biggest */
          MergeBlobs(md, bigblob, blobsincol);
          md->blobcollected[bigblob] = 0;
          /* blobcollected for smaller blobs will not be seen again.
           * reinit it for next column for the big blob
           */
          /* add new sg's to bigblob */
          for (blc = 1, bllen = gl_length(newsgincol); blc <= bllen; blc++) {
            newsg = (int)gl_fetch(newsgincol,blc);
            gl_append_ptr(md->blob[bigblob],(VOIDPTR)newsg);
            md->sg2blob[newsg] = bigblob;
          }
        }
      } /* end if elt ! NULL */
      break;
    } /* end switch */
  } /* end while */
  for (sg = 0; sg < totroutes; sg++) {
    md->blobcollected[sg] = 0;
  }
  /* note this process and the mergeblobs process both leave lots
   * of old data scattered in their wakes.
   */
  /* now blobcollected indicates we're keeping a route */
  for (sg = 0; sg < totroutes; sg++) {
    if (md->sg2blob[sg] == -1) {
      /* direct route. sg not in blob. collect it. */
      md->blobcollected[sg] = 1;
      finalchildnum[*totfinal] = routechildnum[sg];
      (*totfinal)++;
    } else {
      if (md->blobcollected[md->sg2blob[sg]] == 0) {
        /* sg is leftmost route through blob it is in.
         * collect sg and mark blob collected.
         */
        md->blobcollected[md->sg2blob[sg]] = 1;
        finalchildnum[*totfinal] = routechildnum[sg];
        (*totfinal)++;
      }
    }
  }
  ELTRESET(md);
  /* ok, totfinal and finalchildnum are what they need to be. */
}

/* This now called with visittree, not visitnametree, so no
 * path to i is known or needed.
 * All the amipd scratch spaces are assumed clean on entry,
 * so leave them clean on exit and clean them before the
 * first call to this occurs in a related series.
 */
static
void AnonMergeDetect(struct Instance *i,
                     struct AnonMergeIPData *amipd)
{
  struct Instance *ch;                  /* immediate children of i */
  struct AnonMergeIP *amip, *chamip;
  unsigned long nch,c;
  int len;
  int nextnode, nextnode_ip;		/* index for descendant iteration */
  int nphint;		 		/* clues for iterations */
  int thisnode;                         /* nodenumber for i */
  int sg;                               /* iteration */
  unsigned int aflags;
  Numlist_p mergednodes;                /* interesting descendants of i */
  Numlist_p parents;                	/* interesting descendants of i */
  struct gl_list_t *scratchpath, *scratchpathlist, *scratchamlist;
  struct gl_list_t *path, *pathlist;
  ChildListPtr cl;
  /* probably want to make totroutes, totgraphs, totfinal local ints */

  /* Here we take the subgraph map of merged nodes in i
   * and try to find nonintersecting paths to each of the
   * merged nodes starting from each of the immediate
   * children of i. All compound or merged instances have a subgraph
   * map (or NULL if they have no merged descendants of
   * interest) of mergednode numbers.
   */

  aflags = GetAnonFlags(i);
  if ((aflags & AMIPFLAG) == 0 || /* ignore junk */
      (aflags & AMUFLAG) != 0 /* ignore UNIVERSALs */) {
    /* skip the maximally uninteresting nodes */
    return;
  } else {
    /* get amip for use in the rest of this function. */
    amip = (struct AnonMergeIP *)GetInterfacePtr(i);
    assert(amip!=NULL);
    /* for all nodes set up the translation to ip_number */
    amipd->node2ip[amip->node_number] = amip->ip_number;
    /* this number is not used in this function call, but in
     * later calls where this node is the merged descendant
     * being accounted for. node2ip[0] is obviously not meaningful.
     */
  }

  if (IsCompoundInstance(i) && (nch = NumberChildren(i)) > 1) {
    mergednodes = amip->shared;
    if (mergednodes == NULL || NumpairListLen(mergednodes) == 0) {
      /* no merged descendants to account for */
      return;
    }
    /* get this nodes number so we can ignore i while processing i */
    thisnode = amip->node_number;
    amipd->totgraphs = 0;
#if (AMSTAT && 0)
    FPRINTF(ASCERR,"\nroot ip %d ",amip->ip_number);
    WriteInstanceName(ASCERR,i,NULL);
    FPRINTF(ASCERR,"\n");
#if (AMSTAT && 0)
    FPRINTF(ASCERR,"descendants:\n");
    NLPWrite(NULL,amip->subgraph);
#endif
#endif
    if (InstanceKind(i)==MODEL_INST) {
      cl = GetChildList(InstanceTypeDesc(i));
    } else {
      cl = NULL;
    }

    for (c = 1; c <= nch; c++) {
      /* lots of children have no merged subgraphs. collect total set from
       * those that do and then iterate only over those children.
       * Also exclude alias children as they will be accounted in
       * the type definition.
       */
      ch = InstanceChild(i,c);
      if (ch != NULL &&
          (GetAnonFlags(ch)&AMIPFLAG) != 0 &&
          (cl == NULL || ChildAliasing(cl,c)==0)) {
        chamip = (struct AnonMergeIP *)GetInterfacePtr(ch);
        assert(chamip != NULL);
        if (chamip->shared != NULL) {
          amipd->graphs[amipd->totgraphs] = chamip->subgraph;
          amipd->mergegraphs[amipd->totgraphs] = chamip->shared;
          amipd->graphchildnum[amipd->totgraphs] = c; /* track origin */
          amipd->totgraphs++;
#if (AMSTAT && 0)
          if (chamip->subgraph != NULL &&
              NumpairListLen(chamip->subgraph) > 0) {
            FPRINTF(ASCERR,"root ip %d, child number %lu, subgraph:\n",
                    amip->ip_number,c);
#if NLPWRITE
            NLPWrite(ASCERR, chamip->subgraph);
            FPRINTF(ASCERR,"shared\n");
            NLPWrite(ASCERR, chamip->shared);
            FPRINTF(ASCERR,"\n");
#endif
          } /* endif */
#endif /*amstat*/
        } /* endif */
      } /* endif */
    } /* endfor children i */

    if (amipd->totgraphs < 2) {
      /* one child has all the merged nodes, so we will never be able
       * to find two independent paths to any merged descendant. quit now.
       */
      amipd->totgraphs = 0;
      return;
    }

    /* Foreach merged descendant of i, find merges recordable on i.
     * There can never be more than n=totgraphs independent paths.
     * Process in descending node order,
     * though this is not required as what is recorded about one descendant
     * does not affect what is recorded about another.
     * Reverse order may make some set computations cheaper if we were
     * to highly optimize the processing.
     */
    scratchpath = amipd->scratchpath;
    scratchpathlist = amipd->scratchpathlist;
    scratchamlist = amipd->scratchamlist;
    gl_reset(scratchamlist);
    nphint = -1;
    len = amipd->totgraphs;
    for (sg = 0; sg < len; sg++) {
      /* With a little care, we could construct totgraphs full of -1
       * and always leave it in that state. The amount of assignments
       * would be the same, but the iteration would be skipped if we
       * could piggyback the iteration on some other iteration.
       */
      amipd->listhints[sg] = -1;
    }
    for (nextnode = NumpairPrevNumber(mergednodes,0,&nphint);
         nextnode > 0;
         nextnode = NumpairPrevNumber(mergednodes,nextnode,&nphint)) {
      amipd->totroutes = 0;
      /* find merges of i's descendants nextnode but not of i itself. */
      if (nextnode != thisnode) {
        len = amipd->totgraphs;
        /* filter out shared that don't have a route to nextnode in them */
        nextnode_ip = amipd->node2ip[nextnode];
        for (sg = 0; sg < len; sg++) {
          if (NumpairNumberInListHintedDecreasing(amipd->mergegraphs[sg],
                                                  nextnode,
                                                  &(amipd->listhints[sg]))
             ) {
            /* collect shared that contain a route to nextnode.
             * Because we can do this in a hinted fashion and skip
             * out on extra computing for many easy cases, we
             * do not combine it with the redundancy detection step which
             * costs a bit more.
             */
            amipd->routes[amipd->totroutes] = amipd->graphs[sg];
            amipd->routechildnum[amipd->totroutes] = amipd->graphchildnum[sg];
            amipd->totroutes++;
          }
        }
        if (amipd->totroutes < 2) {
          /* one child has all the routes, there are not
           * two independent paths to this merged descendant.
           * quit this descendant now.
           */
          continue; /* go on to nextnode */
        }
#if (AMSTAT && 0)
        FPRINTF(ASCERR,"Routes to merge %d (ip %d): %d\n", nextnode,
                nextnode_ip, amipd->totroutes);
#endif
        /* filter out redundant routes and redundant merges */
        /* amipd->enlp0 is a scratch enlp */
        NumpairClearList(amipd->enlp0);

	/* collect parents of nextnode that are in graph of i.
         * Do this by calculating the intersection of subgraph(i)
         * with parents(nextnode). Calculate the parents(nextnode)
         * list once in preprocessing.
         */
        parents = GetParentsList(nextnode_ip,amipd);
        assert(parents != NULL);
#if (AMSTAT && 0)
        FPRINTF(ASCERR,"Parents of ip#%d\n",nextnode_ip);
        NLPWrite(NULL,parents);
#endif
        NumpairCalcIntersection(amip->subgraph, parents, amipd->enlp0);
        amipd->totfinal = 0;
#if CFI
        FPRINTF(ASCERR,"Checking merged ip# %d\n",nextnode_ip);
#endif
        amipd->md.nextnode_ip = nextnode_ip;
        CalcFinalIndependentRoutes(
            &(amipd->md),
            amipd->enlp0,
            amipd->routes,
            amipd->enlp1,
            scratchpathlist,
            scratchpath,
            amipd->finalchildnum,
            amipd->routechildnum,
            &(amipd->totfinal),
            amipd->totroutes
        );

        if (amipd->totfinal < 2) {
          /* merges for nextnode are redundant in i. go to next */
          continue;
        }

        /*
         * Ok, so now we have a list of nonintersecting starting points
         * (direct children of i). Follow alphabetic path from each
         * to nextnode and record it. (Intersecting at i and nextnode,
         * but nowhere else in the intervening dag, is ok.)
         */
        gl_reset(scratchpathlist);
#if (AMSTAT && 0)
        FPRINTF(ASCERR,"ip_node %d: merge %d (ip %d) in subgraph %d"
                " independent ways.\n",
                amip->ip_number,nextnode,nextnode_ip,amipd->totfinal);
#endif /* amstat */
        for (sg = 0; sg < amipd->totfinal; sg++) {
          gl_reset(scratchpath);
#if (AMSTAT && 0)
          FPRINTF(ASCERR,"Searching in child %lu.\n",amipd->finalchildnum[sg]);
#endif /* amstat */
          AnonMergeFindPath(i,nextnode,amipd->finalchildnum[sg],scratchpath);
          /* save path found to pathlist */
          path = gl_copy(scratchpath);
          gl_append_ptr(scratchpathlist,path);
        }
        /* save pathlist to mergelist of i */
        pathlist = gl_copy(scratchpathlist);
        gl_append_ptr(scratchamlist,pathlist);
      }
    } /* endfor nextnode */
    /* save merges for this node i to IP */
    assert(amip->amlist == NULL);
    if (gl_length(scratchamlist) > 0) {
      amip->amlist = gl_copy(scratchamlist);
      amipd->num_iwithmerge++;
      amipd->num_mergelists += (int)gl_length(amip->amlist);
    } else {
      amip->amlist = NULL;
    }
  } /* done with this compound instance */
}

/* public stuff */
/*
 * sanity checker variable.
 */
static int g_ammarking = 0;
/*
 * vp = Asc_AnonMergeMarkIP(root);
 * VOIDPTR vp;
 * This function finds merged instances, anon or otherwise, and
 * records the merges at the scope most appropriate.
 * On return, all InterfacePointers in the tree of root are either
 * NULL or point to some data we understand.
 * When done using these IPs, the caller should call
 * AnonMergeDestroyIPs(vp);
 * This function uses the push/pop protocol for ips, so ip data
 * of other clients may be lost if Unmark is not called properly.
 * Generally, both functions should be called from the same scope.
 *
 * ! ! Assumes that tmpnums are all 0 on entry, and leaves any
 * it has touched 0 on exit.
 *
 * Does not record recursive merges, i.e. if a,b ARE_THE_SAME,
 * don't record a.i,b.i ARE_THE_SAME. This is simple to detect
 * as a.i,b.i have the same childnumber/instanceparent pair.
 */
VOIDPTR Asc_AnonMergeMarkIPs(struct Instance *root)
{
  struct AnonMergeVisitInfo amvi; /* no pointers inside, so no malloc/free*/
  struct AnonMergeIPData *amipd;

  g_ammarking = 1;
  /* init before estimate of ips needed */
  AMVIInit(&amvi,root);

  /* zero anon flags (universal), mark number of
   * real parents to children in tmpnums, and census the formal types
   * for detecting anonymously universal instances.
   */
#if (AMSTAT || AMBKDN)
  FPRINTF(ASCERR,"%ld init amvi done\n",clock());
#endif
  SilentVisitInstanceTreeTwo(root,(VisitTwoProc)AnonMergeCountTree,1,0,&amvi);

#if (AMSTAT || AMBKDN)
  FPRINTF(ASCERR,"%ld counttree done\n",clock());
#endif
  /* mark the instances which have only one instance as 'locally' universal
   * in anonflags and set tmpnum to 0 for same.
   */
  AnonMergeMarkUniversals(&amvi);
#if (AMSTAT || AMBKDN)
  FPRINTF(ASCERR,"%ld mark universals done\n",clock());
#endif

  /* Set the tmpnum of anything with a 1 tmpnum(1 parent) to 0 tmpnum.
   * Set the tmpnums of anything with > 1 parent to the merged node
   * number. note that this forces us to use tmpnum instead of amip->nodenum
   * since there is amip are on any compound instance and any shared instance.
   * amvi.nip is number of ips we'll need and amvi.nim is the number of
   * shared instances for which we need to account the merges.
   */
  SilentVisitInstanceTreeTwo(root,(VisitTwoProc)AnonMergeLabelNodes,
                             1,0,&amvi);
  /* Nodes are tmpnumbered 1..mergednodes for nodes of interest, not all
   * instances.  If we were cpu fascists, we would merge this step with the
   * next one quite carefully.
   */
#if (AMSTAT || AMBKDN)
  FPRINTF(ASCERR,"%ld label nodes done\n",clock());
#endif

#if (AMSTAT)
  AMCWriteCounts(&amvi);
#endif /*amstat*/

  /* create AMIPs and other temporary memory */
  amipd = AMIPDInit(&amvi);
  if (amipd == NULL) {
    return NULL;
  }

#if (AMSTAT || AMBKDN)
  FPRINTF(ASCERR,"%ld amipdinit done\n",clock());
#endif

#if (CHECKN)
  /* check the numbering for continuity */
  amipd->fp = fopen("/tmp/nodes","w+");
  SilentVisitInstanceTreeTwo(root,(VisitTwoProc)AMCheckN,1,0,amipd);
  fclose(amipd->fp);
#endif
  /* do the magic bottom up. */
  SilentVisitInstanceTreeTwo(root,(VisitTwoProc)AnonMergeDetect,1,0,amipd);

#if (AMSTAT || AMBKDN)
  FPRINTF(ASCERR,"%ld mergedetect done\n\n",clock());
#endif
#if AMBKDN
  FPRINTF(ASCERR,"Instances with merges  %d\n",amipd->num_iwithmerge);
  FPRINTF(ASCERR,"N-way merges recorded  %d\n",amipd->num_mergelists);
  FPRINTF(ASCERR,"Merged instances       %ld\n",amvi.nim);
  FPRINTF(ASCERR,"Instances expecting IP %ld\n",amvi.nip);
  FPRINTF(ASCERR,"IPs used               %d\n",amipd->ipused);
#endif
  return (VOIDPTR)amipd;
}

/*
 * return the comparison of merge lists from two distinct instances
 * first check for same instance (aml's equal), then
 * we must compare until a difference found.
 * This function must yield a partial order, ie a<b && b<c ==> a < c.
 * We compare by at 4 levels, searching for the first difference:
 * 1) length of aml (number of merged instances in i)
 * 2) lengths of lists in aml (number of names for each merged descendant)
 * 3) lengths of paths in each pathlist.
 * 4) content of each pair of paths.
 */
static
int AnonMergeCmpMLists(CONST struct gl_list_t *aml1,
                       CONST struct gl_list_t *aml2)
{
  unsigned long len1, len2, len3, len4, pl, p;
  struct gl_list_t *pathlist1, *pathlist2, *path1, *path2;
  int cmp = 0;

  if (aml1==aml2) {
    /* includes NULL == NULL case. */
    return 0;
  }
  if (aml1 == NULL) {
    return -1;
  }
  if (aml2 == NULL) {
    return 1;
  }
  assert(aml1!=NULL);
  assert(aml2!=NULL);
  /* check for same number of merged descendants */
  len1 = gl_length(aml1);
  len2 = gl_length(aml2);
  if (len2 != len1) {
    return (len2 > len1) ? -1 : 1; /* longer is >, right? */
  }
  /* foreach descendant, check same number of paths found */
  for (pl = 1; pl <= len1; pl++) {
    pathlist1 = (struct gl_list_t *)gl_fetch(aml1,pl);
    pathlist2 = (struct gl_list_t *)gl_fetch(aml2,pl);
    assert(pathlist1!=NULL);
    assert(pathlist2!=NULL);
    len2 = gl_length(pathlist1);
    len3 = gl_length(pathlist2);
    if (len3 != len2) {
      return (len3 > len2) ? -1 : 1; /* longer is >, right? */
    }
  }
  /* for each descendant, check that paths of same length were found */
  for (pl = 1; pl <= len1; pl++) {
    pathlist1 = (struct gl_list_t *)gl_fetch(aml1,pl);
    pathlist2 = (struct gl_list_t *)gl_fetch(aml2,pl);
    len2 = gl_length(pathlist1);
    for (p = 1; p <= len2; p++) {
      path1 = (struct gl_list_t *)gl_fetch(pathlist1,p);
      path2 = (struct gl_list_t *)gl_fetch(pathlist2,p);
      assert(path1!=NULL);
      assert(path2!=NULL);
      len3 = gl_length(path1);
      len4 = gl_length(path2);
      if (len4 != len3) {
        return (len4 > len3) ? -1 : 1; /* longer is >, right? */
      }
    }
  }
  /* for each descendant, check that paths of same content were found */
  for (pl = 1; pl <= len1; pl++) {
    pathlist1 = (struct gl_list_t *)gl_fetch(aml1,pl);
    pathlist2 = (struct gl_list_t *)gl_fetch(aml2,pl);
    len2 = gl_length(pathlist1);
    for (p = 1; p <= len2; p++) {
      path1 = (struct gl_list_t *)gl_fetch(pathlist1,p);
      path2 = (struct gl_list_t *)gl_fetch(pathlist2,p);
      cmp = gl_compare_ptrs(path1,path2);
      if (cmp != 0) {
        return cmp;
      }
    }
  }

  return 0;
}

/*
 * Returns the comparison of the merge information stored in two
 * instances, which must of the same formal type and have
 * children of the same anonymous types. It doesn't make
 * sense to call the function on ATOM-like instances, since they
 * can have no deeper merged structures.
 * UNIVERSAL instances will always return 0. (Think about it.)
 * Objects which are part of UNIVERSAL instances or somehow
 * not supposed to have a list of merges will always return 2,
 * even if compared to themselves!
 * If the comparison is valid, it will return 0,1,-1.
 * instances without lists shold be < instances with lists.
 */
int Asc_AnonMergeCmpInstances(CONST struct Instance *i1,
                              CONST struct Instance *i2)
{
  struct AnonMergeIP *amip1, *amip2;
  int cmp, flags1, flags2;
  assert(i1!=NULL);
  assert(i2!=NULL);
  assert(InstanceTypeDesc(i1) == InstanceTypeDesc(i2));
  assert(i1 != i2);
  flags1 = GetAnonFlags(i1);
  if (flags1 == (AMUFLAG | AMIPFLAG)) {
    return 0;
  }
  flags2 = GetAnonFlags(i2);
  if ((flags1 & AMIPFLAG) == 0 || (flags2 & AMIPFLAG) == 0 ) {
    return 2;
  }
  amip1 = (struct AnonMergeIP *)GetInterfacePtr(i1);
  amip2 = (struct AnonMergeIP *)GetInterfacePtr(i2);
  if (amip1 == NULL || amip2 == NULL) {
    return 2;
  }
  cmp = AnonMergeCmpMLists(amip1->amlist, amip2->amlist);
  return cmp;
}

/*
 * frees data structures returned by AnonMergeMarkIPs.
 */
void Asc_AnonMergeUnmarkIPs(VOIDPTR vp)
{
  struct AnonMergeIPData *amipd;
  if (g_ammarking == 0) {
    Asc_Panic(2,"Asc_AnonMergeUnmarkIP","Called without marks current");
  }
  amipd = (struct AnonMergeIPData *)vp;
#if AMSTAT
  amipd->fp = fopen("/tmp/amipd","w+");
  SilentVisitInstanceTreeTwo(amipd->root,(VisitTwoProc)AnonMergeLogIP,
                             1,0,amipd);
  FPRINTF(amipd->fp,"\n######\n");
#endif
  PopInterfacePtrs(amipd->oldips,
                   (IPDeleteFunc)AnonMergeDestroyIP,
                   (VOIDPTR)amipd);
  assert(amipd->ipused==amipd->ipback);
#if AMSTAT
  fclose(amipd->fp);
#endif
  DestroyAMIPD(amipd);
  g_ammarking = 0;
}

void Asc_AnonMergeWriteList(FILE *fp, struct Instance *i)
{
  struct AnonMergeIP *amip;
  if (g_ammarking == 0) {
    Asc_Panic(2,"Asc_AnonMergeWriteList","Called without marks current");
  }
  assert(i!= NULL);
  if (IsCompoundInstance(i)==0) {
    FPRINTF(fp,"NONE\n");
    return;
  }
  if (GetAnonFlags(i)&AMIPFLAG) {
    amip = (struct AnonMergeIP *)GetInterfacePtr(i);
    if (amip->amlist != NULL) {
      WriteMList(fp, amip->amlist, i);
    }
  }
}


