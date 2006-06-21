/*
 *  numlist.h
 *  by Ben Allan
 *  December 20, 1997
 *  Part of ASCEND
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: numlist.c,v $
 *  Date last modified: $Date: 1998/06/16 16:38:43 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/ascPrint.h>
#include <general/list.h>
#include "numlist.h" /* should be in general/ */
#if NUMLISTUSESPOOL
#include <general/pool.h>
#endif

/*
 * This is an element in a list of
 * numbers and ranges.
 * Valid numbers are 1..INT_MAX.
 * Numbers are stored in increasing order.
 * Valid numbers are 1..INT_MAX.
 * numpair is a range which include all numbers from .lo to .hi
 * including the limits of the range. A range may go from i to i.
 * but not from j to i where j >i.
 */
struct numpair {
  int lo, hi;
};

/*
 * These are the objects we hand out to the user.
 * but they don't need to know that.
 */
struct numpair_list {
  struct numpair_list *head;
  struct numpair *list; /* array of pairs */
  /* if not NULL, list is piece of core rooted in list of head.
   * if NULL this list's data is referenced by refcount pointers.
   * When refcount is 0, destroying the numpair_list
   * should entail destroying its data and head list as well.
   * The g_recycled_npl may reference a list, as will all the
   * lists created using the data of this list.
   */
  int len; /* size of list, meaning data in use */
  int cap; /* capacity of allocated list, if allocated, else -1. */
  int refcount; /* number of sharers in data space if head is NULL. */
#if 0 /* may need this on long pointer hardware */
#if (SIZEOF_VOID_P == 8)
  int pad;
#endif
#endif
};

struct shared_data {
  struct numpair_list *headlist;/* list allocated bigger than needed. */
  int headcap;                  /* size of allocation in headlist */
  int headfree;                 /* elements in headlist data at end
                                 * not in use. */
};

/*
 * List of head lists with space available.
 * Cap will grow to the size needed.
 * Size will shrink to zero cap/NULL when len -> 0.
 */
static struct {
  struct shared_data *shared; /* array of shared list records */
  int shared_cap ;     /* array size. */
  int shared_len ;     /* array size in use */
} g_recycled_npl = {NULL,0,0};
#define GRN g_recycled_npl /* cheesy alias */
#define GROW_GRN 0.5 /* grow by this fraction of previous size */
#define INITSIZE_GRN 100

/*
 * set the largest number of elements we are willing to
 * sacrifice when declaring a shared data entirely used up.
 * must be at least 1. If having short headfree values
 * causes a long search for free space, then increase minfree.
 */
#define MINFREE 1

/*
 * set the number of elements to put in a shared head list.
 * should be reasonably large. Memory will be allocated in
 * blocks of SHRSIZE*sizeof(struct numpair) when creating
 * shareable data spaces.
 * All lists larger than this will be individually created.
 */
#define SHRSIZE 1024

/*
 * in shared lists, we will stick a checknum in between
 * shared data chunks. If this is overwritten, someone
 * has overrun a list.
 */
#define NHCHECKNUM INT_MAX

/*
 * we should pool the heads.
 */
#if NUMLISTUSESPOOL

static pool_store_t g_numlist_head_pool = NULL;
/* global for our memory manager */
/* aim for 4096 chunks including malloc overhead */
#define PNL_LEN 10
#ifdef __alpha
#define PNL_WID 100
#else
#define PNL_WID 201
#endif
/* retune rpwid if the size of struct numlist changes */
#define PNL_ELT_SIZE (sizeof(struct numpair_list))
#define PNL_MORE_ELTS 10
/*
 *  Number of slots filled if more elements needed.
 *  So if the pool grows, it grows by PNL_MORE_ELTS*PNL_WID elements at a time.
 */
#define PNL_MORE_BARS 500
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown. */
static
void numlist_init_pool(void) {
  if (g_numlist_head_pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: numlist_init_pool called twice.\n");
  }
  g_numlist_head_pool = pool_create_store(PNL_LEN, PNL_WID, PNL_ELT_SIZE,
    PNL_MORE_ELTS, PNL_MORE_BARS);
  if (g_numlist_head_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: numlist_init_pool unable to allocate pool.\n");
  }
}

static
void numlist_destroy_pool(void) {
  if (g_numlist_head_pool==NULL) return;
  pool_clear_store(g_numlist_head_pool);
  pool_destroy_store(g_numlist_head_pool);
  g_numlist_head_pool = NULL;
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
/*@unused@*/
static
void numlist_report_pool(FILE *f)
{
  if (g_numlist_head_pool==NULL) {
    FPRINTF(f,"NumlistHeadPool is empty\n");
  }
  FPRINTF(f,"NumlistHeadPool ");
  pool_print_store(f,g_numlist_head_pool,0);
}
#endif

static
struct numpair_list *GetPoolHead()
{
  if (g_numlist_head_pool == NULL) {
    numlist_init_pool();
  }
  return (struct numpair_list *)(pool_get_element(g_numlist_head_pool));
}

#define NHMALLOC GetPoolHead()
/* get a token. Token is the size of the struct struct numpair_list */
#define NHFREE(p) (pool_free_element(g_numlist_head_pool,((void *)p)))
/* return a struct numpair_list */
#else
#define NHFREE(p) ascfree(p)
#define NHMALLOC \
  (struct numpair_list *)ascmalloc(sizeof(struct numpair_list))
#endif


/* write np up to len to stderr */
static
void NPWrite(FILE *fp, struct numpair *p, int len)
{
  int i;
  if (fp == NULL) {
    fp = stderr;
  }
  if (p == NULL) {
    return;
  }
  FPRINTF(fp,"0x%p: ",p);
  for (i = 0; i < len; i++) {
    if (p[i].lo != p[i].hi) {
      FPRINTF(fp,"%d..%d, ", p[i].lo, p[i].hi);
    } else {
      FPRINTF(fp,"%d, ",p[i].hi);
    }
  }
  FPRINTF(fp,"\n");
  return;
}

/* conditionally exported */
void NLPWrite(FILE *fp, Numlist_p nlp)
{
  if (nlp == NULL) {
    return;
  }
  NPWrite(fp,nlp->list,nlp->len);
  if (fp == NULL) {
    fp = stderr;
  }
  FPRINTF(fp,"len = %d, cap = %d\n",nlp->len,nlp->cap);
}

/*
 * nlp = NumpairExpandableList(nlp,size);
 * Expands the size of nlp, which must be created
 * with NumpairExpandableList. If nlp is NULL, creates
 * the list with the size specified.
 * If insufficient memory to create or expand to size
 * required, returns NULL.
 */
Numlist_p NumpairExpandableList(Numlist_p nlp, int newsize)
{
  struct numpair *newdata;
  if (nlp == NULL) {
    if (newsize == 0) {
      return NULL;
    } else {
      nlp = NHMALLOC;
      if (nlp == NULL) {
        return NULL; /* not reached */
      }
    }
    nlp->list = ASC_NEW_ARRAY(struct numpair,newsize);
    if (nlp->list == NULL) {
      NHFREE(nlp);
      return NULL;
    }
    nlp->refcount = 1;
    nlp->len = 0;
    nlp->cap = newsize;
    nlp->head = NULL;
  } else {
    if (nlp->head != NULL || nlp->refcount != 1 || nlp->cap == -1) {
      Asc_Panic(2,"Numlist_p to be expanded is not expandable",
                "NumpairExpandableList");
      return NULL; /*not reached */
    }
    if (nlp->cap >= newsize) {
      return nlp;
    }
  }
  newdata = (struct numpair *)ascrealloc(nlp->list,
                                         sizeof(struct numpair)*newsize);
  if (newdata == NULL) {
    return NULL;
  } else {
    nlp->list = newdata;
    nlp->cap = newsize;
    return nlp;
  }
}

/*
 * Destroy a list. list may have come from
 * NumpairCopyList or NumpairExpandableList.
 */
void NumpairDestroyList(Numlist_p nlp)
{
  if (nlp == NULL) {
    return;
  }
  /* nlp sharing data space it doesn't own */
  if (nlp->head != NULL) {
#ifndef NDEBUG
    /* check for overrun */
    if (nlp->list !=NULL) {
      if (nlp->list[-1].lo != NHCHECKNUM || nlp->list[-1].hi != NHCHECKNUM) {
        Asc_Panic(2,"Write past end of numlist detected",
                  "NumpairDestroyList");
        /* not reached */
      }
    }
#endif
    nlp->head->refcount--;
    if (nlp->head->refcount == 0 ) {
      /* this reference may have kept its nlp around
       * after the head was destroyed officially.
       */
      ascfree(nlp->head->list);
      nlp->head->refcount = -3;
      NHFREE(nlp->head);
    }
    assert(nlp->refcount == 1);
    nlp->refcount = -2;
    NHFREE(nlp);
    return;
  }
  /* this owns a data space that may be shared */
  nlp->refcount--;
  if (nlp->refcount < 1) {
    /* nobody else cares about data, so destroy all */
    ascfree(nlp->list);
    nlp->refcount = -2;
    NHFREE(nlp);
  }
}

/* called only by numpaircopylist.
 * copy a list. data capacity will be dlen, but list
 * result will only know its size of nlp->len.
 * Result is potentially to become shared data space of size dlen,
 * so someone other than
 * the list is responsible for tracking capacity.
 * At any rate, the result of this function is not expandable, appendable.
 */
static
Numlist_p NumpairDuplicate(Numlist_p nlp,int dlen)
{
  Numlist_p result;
  struct numpair *data, *src;
  int len,i;

  assert(nlp != NULL);

  result = NHMALLOC;
  if (result == NULL) {
    return NULL;
  }
  len = nlp->len;
  result->list = ASC_NEW_ARRAY(struct numpair,dlen);
  result->head = NULL;
  if (result->list == NULL) {
    NHFREE(result);
    return NULL;
  }
  result->refcount = 1;
  result->len = len;
  /* result->cap = dlen; somebody else's problem */
  result->cap = -1; /* how odd! but this means list is not expandable */
  data = result->list;
  src = nlp->list;
  for (i=0;i < len; i++) {
    data[i].lo = src[i].lo;
    data[i].hi = src[i].hi;
  }
  return result;
}

/*
 * find a list in GRN.shared which is big enough to hold need
 * data. return -1 if none found.
 */
static
int GetHead(int need)
{
  int len,i;
  len = GRN.shared_len;
  i = 0;
  while (i<len && GRN.shared[i].headfree < need) {
    i++;
  }
  if (i == len) {
    return -1;
  }
  return i;
}
/*
 * if insufficient memory, this simply won't grow the GRN and
 * the list won't get pooled for future use or referenced.
 */
static
void AddToGRN(Numlist_p nlp, int cap)
{
  struct shared_data *resize;
  int newsize;
  if (nlp==NULL) {
    return;
  }
  if (GRN.shared_cap == GRN.shared_len) {
    if (GRN.shared == NULL) { /*create*/
      GRN.shared = (struct shared_data *)ascmalloc(INITSIZE_GRN*
                    sizeof(struct shared_data));
      if (GRN.shared == NULL) {
        return;
      }
      GRN.shared_cap = INITSIZE_GRN;
    } else { /* grow */
      newsize = (int)(GROW_GRN*(double)GRN.shared_cap) + GRN.shared_cap;
      resize = (struct shared_data *)ascrealloc(GRN.shared,
                   newsize*sizeof(struct shared_data));
      if (resize==NULL) {
        return;
      }
      GRN.shared = resize;
    }
  }
  GRN.shared[GRN.shared_len].headlist = nlp;
  GRN.shared[GRN.shared_len].headcap = cap;
  GRN.shared[GRN.shared_len++].headfree = cap - nlp->len;
  nlp->refcount++;
}

/* return a singleton list */
Numlist_p NumpairElementary(int num)
{
  struct numpair_list nl;
  struct numpair np;

  /* fake list not allocated, the use copy operator */
  nl.len = 1;
  nl.list = &np;
  nl.head = NULL;
  nl.refcount=0;
  np.lo = np.hi = num;

  return NumpairCopyList(&nl);
}

/*
 * nlp2 = NumpairCopyList(nlp);
 * Numlist_p nlp2, nlp;
 * Returns an efficiently allocated numpair_list containing the
 * data of the list given. The data in this list may or may not
 * be in a shared allocation, depending on the list size.
 */
Numlist_p NumpairCopyList(Numlist_p nlp)
{
  Numlist_p head, result;
  struct numpair *data, *src;
  int len,i, cell,nextelt;
  assert(nlp != NULL);

  /* if oversized list. create, copy, return */
  if (nlp->len >= SHRSIZE - MINFREE) {
    return NumpairDuplicate(nlp,nlp->len);
  }

  /* list we'd like to copy by using data space at end of some other list.
   * Get index of head with shareable data. if not available, make one and
   * add it to puddle of shared lists.
   */
  cell = GetHead(nlp->len+1); /* we're putting in the checknum so 1 more */
  if (cell != -1) {
    len = nlp->len;
    /* get elements needed from just after end of data in head */
    head = GRN.shared[cell].headlist;
    nextelt = GRN.shared[cell].headcap - GRN.shared[cell].headfree;
    data = &(head->list[nextelt]);
    /* deduct from remaining in cell */
    GRN.shared[cell].headfree -= (len+1);
    if (GRN.shared[cell].headfree < MINFREE) {
      /* eliminate reference at shared[cell] and decrease list len.  */
      head->refcount--;
      if (cell < GRN.shared_len-1) {
        /* copy tail to replace cell, if cell isn't tail */
        GRN.shared[cell] = GRN.shared[GRN.shared_len-1];
      }
      GRN.shared_len--;
      if ( GRN.shared_len ==0) {
        ascfree(GRN.shared);
        GRN.shared = NULL;
        GRN.shared_cap = 0;
      }
    }
    result = NHMALLOC;
    if (result == NULL) {
      return NULL;
    }
    data[0].lo = NHCHECKNUM;
    data[0].hi = NHCHECKNUM;
    data++; /* skip past pad */
    result->list = data;
    result->head = head;
    result->refcount = 1;
    head->refcount++;
    src = nlp->list;
    for (i=0;i < len; i++) {
      data[i].lo = src[i].lo;
      data[i].hi = src[i].hi;
    }
    result->len = len;
    result->cap = -1;
    return result;
  } else {
    /* create list, copy data to it, add list to shared pool,
     * and return resulting list.
     */
    result = NumpairDuplicate(nlp,SHRSIZE);
    if (result != NULL) {
      AddToGRN(result,SHRSIZE);
    }
    return result;
  }
  /* not reached */
}

/* third arg is a pair of long, not a pointer.
 * assumes p.lo >= r[rlen-1].lo
 */
static
int ExtendResult(struct numpair *r, CONST int rlen, CONST struct numpair p)
{
  int idx;
  if (rlen) {
    idx = rlen - 1;
    if (p.lo > r[idx].hi+1) {
      /* add new disjoint element to r. len++ */
      r[rlen].lo = p.lo;
      r[rlen].hi = p.hi;
      return rlen + 1;
    }
    /* extend range to p.hi if needed. no len change. */
    if (p.hi > r[idx].hi) {
      r[idx].hi = p.hi;
    }
    return rlen;
  } else {
    /* first ever element */
    r[0].lo = p.lo;
    r[0].hi = p.hi;
    return 1;
  }
}

/*
 * err = NumpairMergeLists(list1,list2,result);
 * struct numpair_list *list1, *list2, *result;
 * The maximum needed size of result is list1->len+list2->len.
 * The size of result is not checked and this will
 * corrupt memory if result needed is larger than result given.
 * Result must be expandable. list1,list2 need not be.
 * Result is overwritten even if it already has nonzero length.
 */
static
void NumpairMergeLists(Numlist_p nlp1, Numlist_p nlp2, Numlist_p nlpr)
{
  int n1,len1, n2,len2,lenr; /* lengths of lists */
  struct numpair *p1, *p2, *r; /* data from lists */

  len1 = nlp1->len;
  len2 = nlp2->len;
  p1 = nlp1->list;
  p2 = nlp2->list;
  r = nlpr->list;
  n1 = n2 = lenr = 0;

  while (n1 < len1 && n2 < len2) {
    /* copy all p1 entries starting before p2 entry */
    while (n1 < len1 && p1[n1].lo <= p2[n2].lo) {
      lenr = ExtendResult(r,lenr,p1[n1]);
      n1++;
    }
    if (n1 == len1) {
      break;
    }
    /* copy all p2 entries starting before p1 entry */
    while (n2 < len2 && p2[n2].lo < p1[n1].lo) {
      lenr = ExtendResult(r,lenr,p2[n2]);
      n2++;
    }
  }
  /* clean up leftovers. only one of the following while loops will go */
  while (n1 < len1) {
    lenr = ExtendResult(r,lenr,p1[n1]);
    n1++;
  }
  while (n2 < len2) {
    lenr = ExtendResult(r,lenr,p2[n2]);
    n2++;
  }
  nlpr->len = lenr;
}


/*
 * NumpairCalcUnion(enlp1,nlp2,enlp3);
 * Numlist_p enlp1,nlp2,enlp3;
 * Calculates the union of enlp1, nlp2 and leaves the result in
 * enlp1. enlp3 is used if needed and is left in an indeterminate
 * state.
 */
void NumpairCalcUnion(Numlist_p enlp1, Numlist_p nlp2, Numlist_p enlp3)
{
  Numlist_p t;
  struct numpair *src, *data; /* data from lists */
  int len, i;

  assert(enlp1 != NULL);
  assert(enlp3 != NULL);
  assert(nlp2  != NULL);
  assert(enlp1 != enlp3);

  /* handle case of no previous data in enlp1 */
  if (enlp1->len == 0) {
    t = NumpairExpandableList(enlp1, nlp2->len);
    if (t == NULL) { /* ack! */
      return;
    }
    data = enlp1->list;
    src = nlp2->list;
    len = nlp2->len;
    enlp1->len = len;
    for (i=0;i < len; i++) {
      data[i].lo = src[i].lo;
      data[i].hi = src[i].hi;
    }
    return; /* simple case done */
  }
  /* make sure we have memory for worst case */
  t = NumpairExpandableList(enlp3, enlp1->len + nlp2->len);
  if (t==NULL)  { /* ack! */
    return;
  }
  NumpairMergeLists(enlp1, nlp2, enlp3);
  t = NumpairExpandableList(enlp1, enlp3->len);
  if (t==NULL)  { /* ack! */
    return;
  }
  /* copy back data to enlp1 */
  data = enlp1->list;
  src = enlp3->list;
  len = enlp3->len;
  enlp1->len = len;
  for (i=0;i < len; i++) {
    data[i].lo = src[i].lo;
    data[i].hi = src[i].hi;
  }
}

void NumpairCalcIntersection(Numlist_p nlp1, Numlist_p nlp2, Numlist_p enlp3)
{
  struct numpair *s1, *s2, *r; /* data from lists */
  struct numpair overlap;
  int  i, j, lenr;
  int l1,l2;

  assert(nlp1 != NULL);
  assert(nlp2 != NULL);
  assert(enlp3 != NULL);
  assert(nlp1 != enlp3);
  assert(nlp2 != enlp3);

  l1 = nlp1->len;
  l2 = nlp2->len;
  s1 = nlp1->list;
  s2 = nlp2->list;

  NumpairClearList(enlp3);
  /* skip the nobrainers */
  if (!l1 ||
      !l2 ||
      s2[0].lo > s1[l1-1].hi ||
      s1[0].lo > s2[l2-1].hi) {
    return;
  }
  NumpairExpandableList(enlp3, l1 + l2);
  if (s1==NULL) {
    Asc_Panic(2,"Numlist_p enlp3 to be expanded. Insufficient memory.",
              "NumpairCalcIntersection");
  }
  lenr = i = j = 0;
  while (j < l2 && i < l1) {
    /* could we test elsewhere for i,j and break? */
    while (i < l1 && s1[i].hi < s2[j].lo) {
      /* move i up to next j it might overlap */
      i++;
    }
    if (i >= l1) {
      /* ran out of data */
      break;
    }
    while (j < l2 && s2[j].hi < s1[i].lo) {
      /* move j up to next i it might overlap */
      j++;
    }
    if (j >= l2) {
      /* ran out of data */
      break;
    }
    /* max lo1,lo2 */
    overlap.lo = (s1[i].lo > s2[j].lo) ? s1[i].lo : s2[j].lo;
    /* min hi1, hi2, and note which to move next, or both */
    if (s1[i].hi < s2[j].hi) {
      overlap.hi = s1[i].hi;
      i++;
    } else {
      overlap.hi = s2[j].hi;
      if (s1[i].hi == s2[j++].hi) {
        /* hi1 == hi2, move both i j up */
        i++;
      }
    }
    if (overlap.hi >= overlap.lo) {
      NumpairExpandableList(enlp3, enlp3->len + 1);
      r = enlp3->list;
      lenr = ExtendResult(r,lenr,overlap);
    } /* else the search ahead switched tracks */
  }
  enlp3->len = lenr;
}

/*
 * merges multiple lists efficiently and returns an
 * efficient copy of the result.
 */
Numlist_p NumpairCombineLists(struct gl_list_t *gl,
                              Numlist_p elp1, Numlist_p elp2)
{
  unsigned long glen, next;
  Numlist_p nlp1,nlp2, tmp, s1,t2;

  assert(gl!=NULL);
  assert(elp1!=NULL);
  assert(elp2!=NULL);
  assert(elp1!=elp2);
  assert(elp1->cap >= 0);
  assert(elp2->cap >= 0);

  glen = gl_length(gl);
  switch (glen) {
  case 0:
    return NULL;
  case 1:
    return NumpairCopyList((Numlist_p)gl_fetch(gl,1));
  default:
    break;
  }
  /* at least 2 lists */
  nlp1 = (Numlist_p)gl_fetch(gl,1);
  nlp2 = (Numlist_p)gl_fetch(gl,2);
  s1 = elp1 = NumpairExpandableList(elp1, nlp1->len + nlp2->len);
  if (s1==NULL) {
    Asc_Panic(2,"Numlist_p s1 to be expanded. Insufficient memory.",
              "NumpairCombineLists");
  }
  NumpairClearList(s1);
  NumpairMergeLists(nlp1,nlp2,s1);
  t2 = elp2;
  next = 3;
  /* on the first trip through the loop, elp2 is the target and elp1 source.
   * then they are swapped.
   * on the second trip through elp1 is the target, elp2 source.
   * each trip through the loop also includes the next nlp.
   * on loop exit, s1 is always points to the most recent target.
   */
  while (next <= glen) {
    nlp1 = (Numlist_p)gl_fetch(gl,next);
    t2 = NumpairExpandableList(t2, nlp1->len + s1->len);
    if (t2==NULL) {
      Asc_Panic(2,"Numlist_p t2 to be expanded. Insufficient memory.",
                "NumpairCombineLists");
    }
    NumpairClearList(t2);
    NumpairMergeLists(nlp1,s1,t2);
    tmp = s1;
    s1 = t2;
    t2 = tmp;
    next++;
  }
  return NumpairCopyList(s1);
}

/*
 * We trust that no number difference is actually ever > INT_MAX.
 * This is a loose comparator. One argument must be a singleton
 * with lo =0 and hi the value. This is the key value we want
 * to know is in the list or not.
 * The other must be a normal element i..i or i..j,j>i.
 * This is for use with bsearch.
 */
static
int CmpNumpairs(CONST void *c1, CONST void *c2)
{
  CONST struct numpair *n1, *n2;
  assert(c1!=NULL);
  assert(c2!=NULL);
  n1 = (struct numpair *)c1;
  n2 = (struct numpair *)c2;
  if (n1->lo == 0) {
    assert(n2->lo != 0);
    /* n1 single, n2 range */
    if (n1->hi < n2->lo) {
      return -1;
    }
    if (n1->hi > n2->hi) {
      return 1;
    }
    return 0; /* within range */
  } else {
    /* n1 range, n2 single */
    assert(n2->lo == 0);
    if (n2->hi < n1->lo) {
      return 1;
    }
    if (n2->hi > n1->hi) {
      return -1;
    }
    return 0; /* within range */
  }
}

/* want to return -1 if num > n2, 1 if num <n2, 0 if in n2. */
static
int CmpIntToPair(int num, struct numpair *n2)
{
  assert(n2->lo != 0);
  /* n2 normal range */
  if (num < n2->lo) {
    return 1;
  }
  if (num > n2->hi) {
    return -1;
  }
  return 0; /* within range */
}

/* add a number somewhere into the list (sorted order)
 * list must be expandable.
 */
void NumpairAppendList(Numlist_p enlp, int num)
{
  Numlist_p nnlp;
  struct numpair *data;
  int comparison;
  unsigned int lower,upper,search=0;

  assert(enlp!=NULL);
  assert(enlp->cap >=0);

  /* expand list if potentially needed */
  nnlp = NumpairExpandableList(enlp, enlp->len+1);
  if (nnlp == NULL) {
    Asc_Panic(2,"Numlist_p to be appended. Insufficient memory.",
              "NumpairAppendList");
    return; /* not reached */
  }

  data = enlp->list;

  /* no data case */
  if (enlp->len==0) {
    data[0].lo = data[0].hi = num;
    enlp->len = 1;
    return;
  }

  /* boundary cases */
  /* low */
  if (num < data[0].lo) {
    if (num+1 == data[0].lo) {
      /* extend lower end */
      data[0].lo = num;
      return;
    } else {
      /* insert at 0 */
      for (upper = enlp->len; upper > 0; upper--) {
        data[upper].lo = data[upper-1].lo;
        data[upper].hi = data[upper-1].hi;
      }
      data[0].lo = data[0].hi = num;
      enlp->len++;
      return;
    }
  }

  upper = enlp->len - 1;

  /* high */
  if (num > data[upper].hi) {
    if (num-1 == data[upper].hi) {
      /* extend upper end */
      data[upper].hi = num;
      return;
    } else {
      /* append  */
      data[enlp->len].lo = data[enlp->len].hi = num;
      enlp->len++;
      return;
    }
  }
  /* prepend and append eliminated */
  /* num belongs on the interior of the list */
  lower = 0;
  while (lower <= upper) {
    search = (lower + upper) >> 1; /* integer divide by two */
    comparison = CmpIntToPair(num,&data[search]);
    if (comparison == 0) {
      /* nothing to do -- got it in the list already */
      return;
    }
    if (comparison < 0) {
      lower = search + 1;
    } else {
      upper = search - 1;
    }
  }
  /* that we got here means num is not in list and that
   * lower > upper. num goes between the two remaining
   * elements somehow or on an end.
   * So we have several cases to handle:
   * We are inserting on or between L:num:H
   * a) L.hi << num << H.lo --> insert element.
   * b) L.hi +1 == num << H.lo --> update L.hi
   * c) L.hi << num == H.lo -1 --> update H.lo
   * d) L.hi +1 == num == H.lo -1 --> assign H.hi to L.hi, delete H, shift.
   */
  /* swap lower and upper so we can read the code. */
  /* --> lower == upper-- */
  search = upper;
  upper = lower;
  lower = search;
  if (data[lower].hi+1 == num) {
    /* b,d */
    if (data[upper].lo - 1 != num) {
      data[lower].hi = num; /*b*/
    } else {
      data[lower].hi = data[upper].hi; /* merge adjacent cells, d */
      /* shift cells on right 1 to left */
      search = (--(enlp->len));
      for ( ; upper < search; upper++) {
        data[upper].lo = data[upper+1].lo;
        data[upper].hi = data[upper+1].hi;
      }
    }
    return;
  } else {
    /* a,c */
    if (data[upper].lo - 1 != num) {
      /*a, insert element */
      for (search = enlp->len; search > upper; search--) {
        data[search].lo = data[search-1].lo;
        data[search].hi = data[search-1].hi;
      }
      data[upper].lo = data[upper].hi = num;
      enlp->len++;
    } else {
      data[upper].lo = num; /*c*/
    }
    return;
  }
}

int NumpairListLen(Numlist_p nlp)
{
  assert(nlp!=NULL);
  return nlp->len;
}

void NumpairClearList(Numlist_p enlp)
{
  assert(enlp != NULL);
  assert(enlp->cap >= 0);
  enlp->len = 0;
}

/*
 * NumberInList(nlp,number);
 * Returns 1 if number is in list and 0 if it is not.
 */
int NumpairNumberInList(Numlist_p nlp, int num)
{
  switch (nlp->len) {
  case 0:
    return 0;
  case 1:
    if (num < nlp->list[0].lo || num > nlp->list[0].hi) {
      return 0;
    } else {
      return 1;
    }
  case 2:
    if (num < nlp->list[0].lo || num > nlp->list[1].hi) {
      return 0;
    }
    if (num <= nlp->list[0].hi || num >= nlp->list[1].lo) {
      return 1;
    } else {
      return 0;
    }
  case 3:
    if (num < nlp->list[0].lo || num > nlp->list[2].hi) {
      return 0;
    }
    /* num is now within combined range of the 3 elements */
    if (num <= nlp->list[0].hi || num >= nlp->list[2].lo) {
      /* num is in element 0 or 2 */
      return 1;
    }
    /* num is in element 1 or not in list. */
    if (num < nlp->list[1].lo || num > nlp->list[1].hi) {
      return 0;
    } else {
      return 1;
    }
  default: {
      struct numpair test;
      char *location; /* location of answer, but we only care if NULL or not */
      test.lo = 0;
      test.hi = num;
      location = bsearch(&test,nlp->list,nlp->len,sizeof(struct numpair),
                         CmpNumpairs);
      return (location != NULL);
    }
  }
}

/*
 * NumberInListHinted(nlp,number,hint);
 * Returns 1 if number is in list and 0 if it is not.
 */
static
int InListDecreasingHint(Numlist_p nlp, int num, int *hint)
{
  int np;
  struct numpair *list;

  np = *hint;
  assert(np < nlp->len);
  list = nlp->list;
  if (np < 0) {
    np = nlp->len - 1;
  }
  /* linear search left from np to 0 for a range with lo <= num */
  while (np > 0 && list[np].lo > num) {
    np--;
  }
  /* now we may have fallen off low-end of list. so do
   * complete check whether num is in the range list[np].
   */
  *hint = np;
  if (num < list[np].lo || num > list[np].hi) {
    return 0;
  } else {
    return 1;
  }
}

/*
 * NumberInListHintedDecreasing(nlp,number,hint);
 * Returns 1 if number is in list at or to the left of
 * hint for lists bigger than 3. hint is ignored for
 * small lists. To initiate a series of searches, call
 * with *hint == -1.
 * Cost O(len) per call worst case, but O(1) if used
 * properly.
 * Note that if hint value is incorrect, this may lie.
 */
int NumpairNumberInListHintedDecreasing(Numlist_p nlp, int num, int *hint)
{
  assert(hint != NULL);
  switch (nlp->len) {
  case 0:
    return 0;
  case 1:
    if (num < nlp->list[0].lo || num > nlp->list[0].hi) {
      return 0;
    } else {
      return 1;
    }
  case 2:
    if (num < nlp->list[0].lo || num > nlp->list[1].hi) {
      return 0;
    }
    if (num <= nlp->list[0].hi || num >= nlp->list[1].lo) {
      return 1;
    } else {
      return 0;
    }
  case 3:
    if (num < nlp->list[0].lo || num > nlp->list[2].hi) {
      return 0;
    }
    /* num is now within combined range of the 3 elements */
    if (num <= nlp->list[0].hi || num >= nlp->list[2].lo) {
      /* num is in element 0 or 2 */
      return 1;
    }
    /* num is in element 1 or not in list. */
    if (num < nlp->list[1].lo || num > nlp->list[1].hi) {
      return 0;
    } else {
      return 1;
    }
  default:
    return InListDecreasingHint(nlp,num,hint);
  }
}

/*
 * prev = NumpairPrevNumber(nlp,last,hint);
 * int *hint;
 * int last;
 * Returns the next lower number in the list preceding
 * last. If last is 0, returns highest
 * number in the list. *hint should be the output from the
 * last call to this function on nlp, or -1.  This function lets
 * you write a list iteration. If last given is 0, hint ignored.
 * Remember that 0 is never really a valid list element.
 */
int NumpairPrevNumber(Numlist_p nlp, int last, int *hint)
{
  struct numpair test;
  char *location;
  if (last < 1) {
    *hint = nlp->len - 1;
    return nlp->list[*hint].hi;
  }

  if (*hint < 0 || *hint >= nlp->len) {
    /* given hint is junk. */
    *hint = nlp->len - 1;
  }
  if (nlp->list[*hint].hi < last) {
    /* so-so hint type 1*/
    (*hint)++;
    if (*hint < nlp->len) {
      test.lo = 0;
      test.hi = last;
      location = bsearch(&test,&(nlp->list[*hint]),nlp->len - (*hint),
                         sizeof(struct numpair), CmpNumpairs);
      if (location == NULL) {
        *hint = -1;
        return 0;
      }
      *hint = (((struct numpair *)location) - nlp->list);
      assert(*hint < nlp->len);
    } else {
      /* last wasn't in this list. user is an idiot */
      *hint = -1;
      return 0;
    }
  } else {
    if (nlp->list[*hint].lo > last) {
      /* so-so hint type 2*/
      (*hint)--;
      if (*hint > -1) {
        test.lo = 0;
        test.hi = last;
        location = bsearch(&test, nlp->list, (*hint)+1,
                           sizeof(struct numpair), CmpNumpairs);
        if (location == NULL) {
          *hint = -1;
          return 0;
        }
        *hint = (((struct numpair *)location) - nlp->list);
        assert(*hint < nlp->len);
      } else {
        /* last wasn't in this list. user is an idiot */
        *hint = -1;
        return 0;
      }
    }
  }
  /* so at last * hint is on the element of list where last was found */
  if (nlp->list[*hint].lo < last) {
    return last - 1; /* hint stays same. in middle of range */
  }
  if (*hint > 0) {
    /* move to previous range */
    (*hint)--;
    return nlp->list[*hint].hi;
  } else {
    *hint = -1;
    return 0;
  }
}

/*
 * prev = NumpairNextNumber(nlp,last,hint);
 * int *hint;
 * int last;
 * Returns the next higher number in the list following
 * last. If last is >= end of list, returns 0.
 * *hint should be the output from the
 * last call to this function on nlp, or 0.  This function lets
 * you write a list iteration. If last 0, hint ignored.
 * Remember that 0 is never really a valid list element.
 */
int NumpairNextNumber(Numlist_p nlp,int last,int *hint)
{
  struct numpair test;
  char *location;

  if (last < 1) {
    *hint = 0;
    return nlp->list[*hint].lo;
  }

  if (*hint < 0 || *hint >= nlp->len) {
    /* given hint is junk. */
    *hint = 0;
  }
  if (nlp->list[*hint].lo > last) {
    /* so-so hint type 1*/
    (*hint)--;
    if (*hint > -1) {
      test.lo = 0;
      test.hi = last;
      location = bsearch(&test,nlp->list, *hint,
                         sizeof(struct numpair), CmpNumpairs);
      if (location == NULL) {
        *hint = -1;
        return 0;
      }
      *hint = (((struct numpair *)location) - nlp->list);
      assert(*hint < nlp->len);
    } else {
      /* last wasn't in this list. user is an idiot */
      *hint = -1;
      return 0;
    }
  } else {
    if (nlp->list[*hint].hi < last) {
      /* so-so hint type 2*/
      (*hint)++;
      if (*hint >= nlp->len) {
        test.lo = 0;
        test.hi = last;
        location = bsearch(&test, &(nlp->list[*hint]), nlp->len - (*hint),
                           sizeof(struct numpair), CmpNumpairs);
        if (location == NULL) {
          *hint = -1;
          return 0;
        }
        *hint = (((struct numpair *)location) - nlp->list);
        assert(*hint < nlp->len);
      } else {
        /* last wasn't in this list. user is an idiot */
        *hint = -1;
        return 0;
      }
    }
  }
  /* so at last * hint is on the element of list where last was found */
  if (nlp->list[*hint].hi > last) {
    return last + 1; /* hint stays same. in middle of range */
  }
  if (*hint < (nlp->len - 1)) {
    /* move to next range */
    (*hint)++;
    return nlp->list[*hint].lo;
  } else {
    *hint = -1;
    return 0;
  }
}

void NumpairListIterate(Numlist_p nlp,NPLFunc func,void *userdata)
{
  int r,i, rlen,ihi;
  if (nlp == NULL || func == NULL || nlp->len == 0) {
    return;
  }
  for (r = 0, rlen = nlp->len; r < rlen; r++) {
    for (i = nlp->list[r].lo, ihi =  nlp->list[r].hi; i <= ihi; i++) {
      (*func)(i,userdata);
    }
  }
}

/*
 * common = NumpairGTIntersection(nlp1,nlp2,lowlimit);
 * int lowlimit;
 * Returns the first number that is both common to nlp1, nlp2
 * and > lowlimit.
 * If no number > lowlimit is common, returns 0.
 * normally lowlimit should be common to both lists, but this
 * is not required.
 */
int NumpairGTIntersection(Numlist_p nlp1, Numlist_p nlp2, int low)
{
  int i,j;	/* indices into the lists of nlp1, nlp2 */
  int l1,l2;
  struct numpair *s1,*s2;
  int change, ilo, ihi;

  l1 = nlp1->len;
  l2 = nlp2->len;
  s1 = nlp1->list;
  s2 = nlp2->list;
  /* skip the nobrainers */
  if (!l1 || !l2 || low > s1[l1-1].hi || low > s2[l2-1].hi) {
    return 0;
  }
  /* find i nearest to low limit in list 1. this could be pivoted. */
  for ( i = 0; i < l1; i++) {
    if ( s1[i].hi > low) {
      /* found first entry containing a number > low */
      break;
    }
  }

  /* find j nearest to low limit in list 2 */
  for ( j = 0; j < l2; j++) {
    if ( s2[j].hi > low) {
      /* found first entry containing a number > low */
      break;
    }
  }

  change = 1;
  /* now find first overlapping ij pair of ranges */
  while (change) {
    change = 0;
    while ( j < l2 && i < l1 && s1[i].lo > s2[j].hi) {
      j++;
      change = 1;
    }
    while ( i < l1 && j < l2 && s2[j].lo > s1[i].hi) {
      i++;
      change = 1;
    }
  }
  if (i == l1 || j == l2) {
    /* no overlap */
    return 0;
  }
  ilo = (s1[i].lo > s2[j].lo) ? s1[i].lo : s2[j].lo;
  ihi = (s1[i].hi < s2[j].hi) ? s1[i].hi : s2[j].hi;
  assert(ihi >= ilo);
  if (ilo <= low) {
    ilo = low + 1;
  }
  if (ilo > ihi) {
    return 0;
  }
  return ilo;
}

int NumpairIntersectionLTHinted(Numlist_p nlp1, int *hint1,
                                Numlist_p nlp2, int *hint2,
                                int high)
{
  int i,j;	/* indices into the lists of nlp1, nlp2 */
  int l1,l2;
  struct numpair *s1,*s2;
  int change, ilo, ihi;

  l1 = nlp1->len;
  l2 = nlp2->len;
  s1 = nlp1->list;
  s2 = nlp2->list;
  if (high <= 0) {
    high = INT_MAX;
  }
  /* skip the nobrainers */
  if (!l1 || !l2 || high < s1[0].lo || high < s2[0].lo) {
    return 0;
  }
  /* find i nearest to high limit in list 1. this could be pivoted. */
  if (*hint1 >= 0) {
    i = *hint1;
  } else {
    i = l1 - 1;
  }
  for ( ; i >= 0; i--) {
    if ( s1[i].lo < high) {
      /* found first entry containing a number < high */
      break;
    }
  }

  /* find j nearest to high limit in list 2 */
  if (*hint2 >= 0) {
    j = *hint2;
  } else {
    j = l2 - 1;
  }
  for ( ; j >= 0; j--) {
    if ( s2[j].lo < high) {
      /* found first entry containing a number < high */
      break;
    }
  }

  change = 1;
  /* now find first overlapping ij pair of ranges */
  while (change) {
    change = 0;
    while ( j >= 0 && i >= 0 && s1[i].hi < s2[j].lo) {
      j--;
      change = 1;
    }
    while ( i >= 0 && j >= 0 && s2[j].hi < s1[i].lo) {
      i--;
      change = 1;
    }
  }
  if (i < 0 || j < 0) {
    /* no overlap */
    return 0;
  }
  /* compute overlap */
  ilo = (s1[i].lo > s2[j].lo) ? s1[i].lo : s2[j].lo; /* max .lo */
  ihi = (s1[i].hi < s2[j].hi) ? s1[i].hi : s2[j].hi; /* min .hi */
  assert(ihi >= ilo);
  if (ihi >= high) {
    ihi = high - 1;
  }
  if (ihi < ilo) {
    return 0;
  }
  *hint1 = i;
  *hint2 = j;
  return ihi;
}

int NumpairCardinality(Numlist_p nlp)
{
  int i,len,count;
  struct numpair *s;
  if (nlp == NULL || nlp->len == 0 || nlp->list == NULL) {
    return 0;
  }
  count = 0;
  len = nlp->len;
  s = nlp->list;
  for (i = 0 ; i < len; i++) {
    count += (s[i].hi - s[i].lo); /* to be complete, add 1 at each iteration */
  }
  /* since we didn't add 1 at each iteration, we can add len at the END
   * to save len-1 additions.
   */
  count += len;
  return count;
}

void NumpairClearPuddle(void)
{
  int len,i;
  len = GRN.shared_len;
  i = 0;
  for (i = 0; i < len ;i++) {
    NumpairDestroyList(GRN.shared[i].headlist);
    GRN.shared[i].headlist = NULL;
  }
  GRN.shared_len = 0;
  GRN.shared_cap = 0;
  if (GRN.shared != NULL) {
    ascfree(GRN.shared);
    GRN.shared = NULL;
  }
#if NUMLISTUSESPOOL
  numlist_destroy_pool();
#endif
}
/*
 * test the basic operations.
 */
/*
#ifdef NUMPAIRSELFTEST
#define TESTITER 0
#define TESTLT 0
#define TESTGT 0
#define TESTINT 1
FILE *g_ascend_errors = stderr;
int main()
{
  Numlist_p p1,p2,p3,ep4,ep5,ep6,ep7,ep8,ep9, es1,es2, p10;
  struct gl_list_t *nlpgl;
  int last, hint, gt, h1, h2;

  gl_init();
  gl_init_pool();

  p1 = NumpairElementary(7);
  FPRINTF(stderr,"p1\n");
  NLPWrite(NULL,p1);

  p2 = NumpairDuplicate(p1,10);
  FPRINTF(stderr,"p2\n");
  NLPWrite(NULL,p2);

  p3 = NumpairCopyList(p2);
  FPRINTF(stderr,"p3\n");
  NLPWrite(NULL,p3);

  ep4 = NumpairExpandableList(NULL,3);
  NumpairAppendList(ep4,5);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);
  NumpairAppendList(ep4,9);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);
  NumpairAppendList(ep4,6);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);


  NumpairAppendList(ep4,4);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);
  NumpairAppendList(ep4,10);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);
  NumpairAppendList(ep4,14);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);
  NumpairAppendList(ep4,17);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);
  NumpairAppendList(ep4,23);
  FPRINTF(stderr,"ep4\n");
  NLPWrite(NULL,ep4);

  ep5 = NumpairExpandableList(NULL,8);
  FPRINTF(stderr,"ep5\n");
  NLPWrite(NULL,ep5);
  NumpairMergeLists(p1,ep4,ep5);
  FPRINTF(stderr,"ep5\n");
  NLPWrite(NULL,ep5);

#if TESTITER
  /* test iteration *
  FPRINTF(stderr,"\nIteration tests\n");
  FPRINTF(stderr,"Prev\n");
  hint = 0;
  last = 0;
  while (hint != -1) {
    last = NumpairPrevNumber(ep5,last,&hint);
    FPRINTF(stderr,"prev - %d hint %d\n", last , hint );
  }

  FPRINTF(stderr,"Next\n");
  hint = 0;
  last = 0;
  while (hint != -1) {
    last = NumpairNextNumber(ep5,last,&hint);
    FPRINTF(stderr,"next - %d hint %d\n", last , hint );
  }
#endif
  ep6 = NumpairExpandableList(NULL,1);
  NumpairAppendList(ep6,18);
  NumpairAppendList(ep6,29);
  NumpairAppendList(ep6,44);
  ep7 = NumpairExpandableList(NULL,1);
  NumpairAppendList(ep7,28);
  NumpairAppendList(ep7,15);
  NumpairAppendList(ep7,3);
  ep8 = NumpairExpandableList(NULL,1);
  NumpairAppendList(ep8,27);
  NumpairAppendList(ep8,13);
  NumpairAppendList(ep8,1);
  ep9 = NumpairExpandableList(NULL,1);
  NumpairAppendList(ep9,72);
  NumpairAppendList(ep9,31);
  NumpairAppendList(ep9,24);
  nlpgl = gl_create(10);
  gl_append_ptr(nlpgl,ep4);
  gl_append_ptr(nlpgl,ep5);
  gl_append_ptr(nlpgl,ep6);
  gl_append_ptr(nlpgl,ep7);
  gl_append_ptr(nlpgl,ep8);
  gl_append_ptr(nlpgl,ep9);

  es1 = NumpairExpandableList(NULL,5);
  es2 = NumpairExpandableList(NULL,5);

  FPRINTF(stderr,"\n MERGE TESTS:\n");
  FPRINTF(stderr,"\nep4\n");
  NLPWrite(NULL,ep4);

  FPRINTF(stderr,"\nep5\n");
  NLPWrite(NULL,ep5);

  FPRINTF(stderr,"\nep6\n");
  NLPWrite(NULL,ep6);

  FPRINTF(stderr,"\nep7\n");
  NLPWrite(NULL,ep7);

  FPRINTF(stderr,"\nep8\n");
  NLPWrite(NULL,ep8);

  FPRINTF(stderr,"\nep9\n");
  NLPWrite(NULL,ep9);

  p10 = NumpairCombineLists(nlpgl,es1,es2);

  FPRINTF(stderr,"\nes1\n");
  NLPWrite(NULL,es1);

  FPRINTF(stderr,"\nes2\n");
  NLPWrite(NULL,es2);

  FPRINTF(stderr,"\np10\n");
  NLPWrite(NULL,p10);

#if TESTLT
  FPRINTF(stderr,"LTIntersection Test ep5, es2\n");
  for (last = 46; last > 0; last--) {
    h1 = h2 = -1;
    gt = NumpairIntersectionLTHinted(ep5,&h1,es2,&h2,last);
    FPRINTF(stderr,"highlimit = %d, result = %d\n",last,gt);
  }

  FPRINTF(stderr,"LTIntersection Test ep5, es2\n");
  h1 = h2 = -1;
  for (last = 0; last > 0 || h1 < 0; ) {
    gt = NumpairIntersectionLTHinted(ep5,&h1,es2,&h2,last);
    FPRINTF(stderr,"highlimit = %d, result = %d\n",last,gt);
    FPRINTF(stderr,"	h1 = %d, h2 = %d\n",h1,h2);
    last = gt;
  }
#endif
#if TESTGT
  FPRINTF(stderr,"GTIntersection Test ep5, es2\n");
  for (last = 0; last < 46; last++) {
    gt = NumpairGTIntersection(ep5,es2,last);
    FPRINTF(stderr,"lowlimit = %d, result = %d\n",last,gt);
  }

  FPRINTF(stderr,"GTIntersection Test es2, ep5\n");
  for (last = 0; last < 46; last++) {
    gt = NumpairGTIntersection(es2,ep5,last);
    FPRINTF(stderr,"lowlimit = %d, result = %d\n",last,gt);
  }

  FPRINTF(stderr,"GTIntersection Test ep5, ep5\n");
  for (last = 0; last < 26; last++) {
    gt = NumpairGTIntersection(ep5,ep5,last);
    FPRINTF(stderr,"lowlimit = %d, result = %d\n",last,gt);
  }
#endif
  FPRINTF(stderr,"\nCalcIntersection Test ep8+29, es1\n");
  NumpairAppendList(ep8,29);
  FPRINTF(stderr,"ep8\n");
  NLPWrite(NULL,ep8);
  FPRINTF(stderr,"\nes1\n");
  NLPWrite(NULL,es1);
  NumpairClearList(ep5);
  NumpairCalcIntersection(ep8,es1,ep5);
  FPRINTF(stderr,"\nresult ep5:\n");
  NLPWrite(NULL,ep5);
  FPRINTF(stderr,"\nCalcIntersection Test es1, ep8+29\n");
  NumpairClearList(ep5);
  NumpairCalcIntersection(es1,ep8,ep5);
  FPRINTF(stderr,"\nresult ep5:\n");
  NLPWrite(NULL,ep5);
  NumpairClearList(ep4);
  NumpairClearList(ep5);
  NumpairClearList(ep6);
  for (h1 = 6; h1 <37; h1++) {
    NumpairAppendList(ep4,h1);
  }
  NumpairAppendList(ep4,45);
  NumpairAppendList(ep4,81);
  NumpairAppendList(ep4,82);
  for (h1 = 84; h1 <104; h1++) {
    NumpairAppendList(ep4,h1);
  }
  NumpairAppendList(ep5,34);
  NumpairAppendList(ep5,35);
  NumpairAppendList(ep5,52);
  NumpairAppendList(ep5,71);
  NumpairAppendList(ep5,97);
  NumpairAppendList(ep5,115);
  FPRINTF(stderr,"\nep4:\n");
  NLPWrite(NULL,ep4);
  FPRINTF(stderr,"\nep5:\n");
  NLPWrite(NULL,ep5);
  NumpairCalcIntersection(ep4,ep5,ep6);
  FPRINTF(stderr,"\nresult ep4 ^ ep5:\n");
  NLPWrite(NULL,ep6);
  NumpairCalcIntersection(ep5,ep4,ep6);
  FPRINTF(stderr,"\nresult ep5 ^ ep4:\n");
  NLPWrite(NULL,ep6);


  NumpairDestroyList(p1);
  NumpairDestroyList(p2);
  NumpairDestroyList(p3);
  NumpairDestroyList(ep4);
  NumpairDestroyList(ep5);
  NumpairDestroyList(ep6);
  NumpairDestroyList(ep7);
  NumpairDestroyList(ep8);
  NumpairDestroyList(ep9);
  NumpairDestroyList(p10);
  NumpairDestroyList(es1);
  NumpairDestroyList(es2);

  NumpairClearPuddle();
  gl_destroy(nlpgl);

  gl_emptyrecycler();                  /* empty the reused list pool *
  gl_destroy_pool();                   /* empty the reused list head pool *

  exit(0);
}
*//*#endif*/
