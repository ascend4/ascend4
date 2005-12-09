
/*
 *  anontypes
 *  Anonymous ASCEND IV type classification functions.
 *  By Benjamin Andrew Allan
 *  Created August 30, 1997. 
 *  Copyright 1997, Carnegie Mellon University.
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: anontype.c,v $
 *  Date last modified: $Date: 2000/01/25 02:25:55 $
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
 */

#include <limits.h> /* for LONG_MAX */
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "utilities/ascPrint.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#if TIMECOMPILER
#include <time.h>
#include "general/tm_time.h"
#endif
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/instance_enum.h"
#include "compiler/types.h"
#include "compiler/instance_types.h"
#include "compiler/tmpnum.h"
#include "compiler/atomvalue.h"
#include "compiler/mathinst.h"
#include "compiler/parentchild.h"
#include "compiler/instquery.h"
#include "compiler/visitinst.h"
#include "compiler/instance_io.h"
#include "compiler/instance_name.h"
#include "compiler/anonmerg.h"
#include "compiler/anontype.h"

#ifndef lint
static CONST char AnonTypeModuleID[] = "$Id: anontype.c,v 1.9 2000/01/25 02:25:55 ballan Exp $";
#endif

/*
 * Yo! Pinhead! Don't optimize anything until it has proved slow!
 */

/* These two macros are defined again in the portion of this file
 * dealing with MODEL/array classification. They should not be used
 * elsewhere.
 */
#define GAIN(inst) GetInstanceAnonIndex(inst)
#define GAP(atp) Asc_GetAnonPrototype(atp)

/* hash function for type name pointers.
 * assumes 1024 table size.
 */
#define TYPEHASHINDEX(p) (((((long) (p))*1103515245) >> 20) & 1023)

/*
 * write merge list before returning.
 */
#define AWAL 1

/*
 * enum for use in sorting compound types
 */
enum search_status {
  at_notdone,	/* weirdness */
  at_exact,	/* duplicate instance */
  at_previous, /* insert */
  at_append	/* new entry at list tail */
};

/*
 * Each bucket in the hash table will correspond to one formal type.
 * A doubly linked list of the formal type's anonymous refinements
 * is kept in the bucket. It's a linked list because we may have
 * lots of AT for constant or ATOM types, though usually there are
 * not many for MODEL types.
 *
 * We may need to add another field to accomodate anonymous, but
 * formal in the sense that the system maintains an internal type
 * description of some kind, array types.
 */
struct AnonBucket {
  struct AnonBucket *next;	/* next hash element */
  struct TypeDescription *d;	/* type for this bucket. */
  struct AnonType *anonlist;	/* ptr to an AnonType */
  unsigned long indirected;	/* subscript number for arrays */
  int size;			/* length of anonlist, the number of
                                 * anonymous types based on formal type d.
                                 */
};

/*
 * A bundle of stuff to pass around with visit functions.
 */
struct AnonVisitInfo {
  struct AnonBucket **t;
  struct gl_list_t *atl;
  struct Instance *root;
  int errors;
  int nextfamily;	/* counter for getting unique exactfamily numbers */
  /* there is no direct sortability to nextfamily values */
};

static
struct AnonBucket **CreateAnonTable(size_t size)
{
  struct AnonBucket **t;
  size_t i;
  t = (struct AnonBucket **)
        ascmalloc(sizeof(struct AnonBucket *)*size);
  if (t==NULL) {
    return t;
  }
  for (i=0; i < size; i++) {
    t[i] = NULL;
  }
  return t;
}

/*
 * Destroys the table array and its buckets, but the AT's
 * in the buckets are not destroyed as they are assumed to be
 * kept in the gl_list, atl.
 */
static
void DestroyAnonTable(struct AnonBucket **t)
{
  int i;
  struct AnonBucket *b;
  for (i=0; i < ANONTABLESIZE; i++) {
    while (t[i] !=NULL) {
      b = t[i];
      t[i] = t[i]->next;
      b->size = -1;
      /* AT data in b->anonlist is assumed to be kept elsewhere */
      ascfree(b);
    }
  }
  ascfree(t);
}

/*
 * This is noisy and fatal if d or t is NULL and NDEBUG is not
 * present. Otherwise it returns 0 if d & t != NULL or 1 if OTHERWISE.
 */
static
int AssertBucketInfo(struct TypeDescription *d, struct AnonBucket **t)
{
  if (d==NULL) {
#ifndef NDEBUG
    Asc_Panic(2,"AssertBucketInfo","Called with NULL TypeDescription");
#else
    return 1;
#endif
  }
  if (t==NULL) {
#ifndef NDEBUG
    Asc_Panic(2,"AssertBucketInfo","Called with NULL Bucket table");
#else
    return 1;
#endif
  }
  return 0;
}

/*
 * Returns the bucket for the type given from the
 * table, if there is such a bucket. The tabled is
 * keyed by string, and then element is matched by type pointer.
 * In the case of arrays, it is also necessary to key by level
 * of indirection or we might try to match enums to ints which
 * is really a bad thing.
 */
static
struct AnonBucket *FindAnonBucket(struct TypeDescription *d,
                                  unsigned long indirected,
                                  struct AnonBucket **t)
{
  struct AnonBucket *result;
  int index;
  if (AssertBucketInfo(d,t)) {
     return NULL;
  }
  index = TYPEHASHINDEX(SCP(GetName(d)));
  result = t[index];
  while (result != NULL && 
          (d != result->d ||  /* not type wanted */
            (indirected != LONG_MAX  && /* is array */
             indirected != result->indirected /*wrong level*/
            )
          ) 
        ) {
    result = result->next;
  }
  return result;
}

/*
 * Creates a table entry for the formal type d. Returns
 * NULL if d was already in the table or out of memory.
 * Returns the entry in normal circumstances.
 * Initially there are no AT's associated with the table.
 */
static
struct AnonBucket *AddAnonBucket(struct TypeDescription *d,
                                 unsigned long indirected,
                                 struct AnonBucket **t)
{
  struct AnonBucket *b;
  int index;
  if (AssertBucketInfo(d,t)!=0 || FindAnonBucket(d,indirected,t) != NULL) {
    return NULL;
  }
  b = (struct AnonBucket *)ascmalloc(sizeof(struct AnonBucket));
  if (b == NULL) {
    return NULL;
  }
  index = TYPEHASHINDEX(SCP(GetName(d)));
  b->next = t[index];
  t[index] = b;
  b->d = d;
  b->indirected = indirected;
  b->anonlist = NULL;
  b->size = 0;
  return b;
}

/* 
 * Insert 'at' in anonlist of b after the AT 'after'.
 * after == NULL --> insert at head of anonlist of b.
 */
static
void InsertAnonType(struct AnonBucket *b, struct AnonType *at,
                    struct AnonType *after)
{
  (b->size)++;
  at->prev = after;
  if (after == NULL) {
    /* insert at head of anonlist */
    at->next = b->anonlist;
    if (b->anonlist != NULL) {
      /* This check shouldn't be necessary, as b was created with 1st
       * element of anonlist, yes? hmm.
       */
      b->anonlist->prev = at;
    }
    b->anonlist = at;
  } else {
    at->next = after->next;
    if (at->next != NULL) {
      /* are we appending the bucket list? */
      at->next->prev = at;
    }
    after->next = at;
  }
}

/* Create an AT and append to the user's ultimate result.
 * also return the at for further configuration.
 */
static
struct AnonType *ExpandAnonResult(struct gl_list_t *atl)
{
  struct AnonType *at;
  at = (struct AnonType *)ascmalloc(sizeof(struct AnonType));
  if (at==NULL) {
    Asc_Panic(2,"ExpandAnonResult","Insufficient memory");
    return NULL; /* NOTREACHED */
  }
  gl_append_ptr(atl,(void *)at);
  at->index = gl_length(atl);
  at->next = at->prev = NULL;
  at->visited = 0;
  at->exactfamily = 0;
  at->instances = gl_create(INSTANCES_PER_AT);
  return at;
}

/*
 * Returns the first element of the instance list associated with the
 * AT given. Does not normally return unless there is such a creature.
 */
struct Instance *Asc_GetAnonPrototype(struct AnonType *at)
{
  struct Instance *i;
  assert(at != NULL);
  assert(at->instances != NULL);
  assert(gl_length(at->instances));
  i = (struct Instance *)gl_fetch(at->instances,1);
  return i;
}

/* wrapper to simplify the logic for dealing with
 * unassigned ATOM values and avoid RAV whine..
 * cheating - we're just cheating. The biggest
 * value specifiable in the parser by symbolic means
 * is DBL_MAX/(1+1e-15), which varies from DBL_MAX
 * in digits 13-16. Don't tell me there are any
 * physically meaningful numbers near DBL_MAX - the
 * floating point discretization error alone is
 * 1e+291 at best.
 */
static
double AnonRealAtomValue(struct Instance *i)
{
  if (!AtomAssigned(i)) {
    return DBL_MAX;
  } else {
    return RealAtomValue(i);
  }
}

/* In the following atom/constant cases it is simply cheaper to 
 * check the value directly instead of checking first to see that
 * the FT defines value or dimensionality.
 * In all atom-like cases, we are ignoring subatomic structure,
 * since all subatomic values are variable and their existence
 * determined by the FT of the atom.
 * The lists are ordered so insertion/search are faster.
 *
 * Anon type -> FT, dimens, value, unassigned comes before values
 * in each dimensionally compatible subset of the list.
 * Can we just redo this entirely? That whole wild/unassigned
 * business make this thing just way too combinatorial.
 */
static
struct AnonType *NearestAnonTypeRC(struct Instance *i, struct AnonType *after,
                                   int *exact)
{
  struct AnonType *testat;
  CONST dim_type *dim;
  struct Instance *testi = NULL; /* unnec init to avoid warning */
  double val;

  if (after==NULL) {
    *exact = 0;
    return after;
  }
  testat = after;
  dim = RealAtomDims(i);
  /* find same dimens. sorted by increasing dim address. */
  while (testat != NULL &&
         ((testi = GAP(testat)), dim > RealAtomDims(testi))) {
    /* C comma syntax: testi = in the logic above does nothing to
     * the logic, except make the second test possible.
     */
    after = testat;
    testat = testat->next;
  }
  if (testat == after && dim != RealAtomDims(testi)) {
    /* dimen to insert at head */
    *exact = 0;
    return NULL;
  }
  /* Now either testat == NULL, in which case done, or need to check
   * value because dim(testi(testat)) == dim(i).
   * We get rid of the NULL case so we can cope more easily with
   * unassigned.
   */
  if (testat == NULL) { 
    /* off end */
    *exact = 0;
    return after;
  } 
    
  val = AnonRealAtomValue(i);
  /* Find same value. Sorted by decreasing value. */
  while (testat != NULL && 
         ((testi = GAP(testat)), dim == RealAtomDims(testi)) &&
         val < AnonRealAtomValue(testi)) {
    /* C comma syntax: testi = in the logic above does nothing to
     * the logic, except make the second test possible.
     */
    after = testat;
    testat = testat->next;
  }
  if (testat == NULL) { 
    /* off end */
    *exact = 0;
    return after;
  } 
  if (val > AnonRealAtomValue(testi) ||
      dim != RealAtomDims(testi)) {
    /* insert at head or after */
    *exact = 0;
    if (testat != after) {
      return after;
    } else {
      return NULL; /* insert @ head */
    }
  }
  *exact = 1;
  return testat;
}

/* Anon type -> FT, dimens, not subatomic dimens since they may be
 * reassignable eventually, as in lagrange multipliers.
 * In any case subatomic dimen cannot affect compiled structure.
 * So, the only sort criteria is really just DIMENSION ptr,
 * because we're all in the same FT type.
 */
static
struct AnonType *NearestAnonTypeRA(struct Instance *i,
                                   struct AnonType *after,
                                   int *exact)
{
  struct AnonType *testat;
  CONST dim_type *dim;
  struct Instance *testi = NULL;

  if (after==NULL) {
    *exact = 0;
    return after;
  }
  dim = RealAtomDims(i);
  /* check for add at front */
  if (dim < RealAtomDims(GAP(after))) {
    *exact = 0;
    return NULL;
  }
  testat = after;
  /* find same dimens. sorted by decreasing dim address. */
  while (testat != NULL &&
         ((testi = GAP(testat)), dim > RealAtomDims(testi))) {
    /* C comma syntax: testi = in the logic above does nothing to
     * the logic, except make the second test possible.
     */
    after = testat;
    testat = testat->next;
  }
  /* ran off end of anonlist? */
  if (testat == NULL) {
    *exact = 0;
    return after; /* Works if after NULL or not. */
  } 
  if (dim == RealAtomDims(testi)) {
    *exact = 1;
    return testat;
  } else {
    *exact = 0;
    return after; /* insert at head */
  }
}

/*
 * cheating, we're just cheating.
 * the parser has been jiggered so that the max int
 * symbolically defined is machine LONG_MAX-1.
 */
static
long AnonIntegerAtomValue(CONST struct Instance *i)
{
  if (AtomAssigned(i)) {
    return GetIntegerAtomValue(i);
  } else {
    return LONG_MAX;
  }
}

/* Anon type -> FT, value.
 * buggy: need to handle unassigned as a special case at
 * beginning of list.
 * as it is, unassigned and long_max are lumped together
 * in the hope that no one ever uses long_max.
 * The parser has been adjusted in evil ways to make this
 * less likely.
 */
static
struct AnonType *NearestAnonTypeIC(struct Instance *i,
                                   struct AnonType *after,
                                   int *exact)
{
  struct AnonType *testat;
  CONST struct Instance *testi = NULL;
  long val;

  if (after==NULL) {
    *exact = 0;
    return after;
  }
  val = AnonIntegerAtomValue(i);
  /* check for add at front */
  if (val >AnonIntegerAtomValue(GAP(after))) {
    *exact = 0;
    return NULL;
  }
  /* Find same value. Sorted by decreasing integer value.  */
  testat = after;
  while (testat != NULL &&
         ((testi = GAP(testat)), val < AnonIntegerAtomValue(testi))) {
    /* C comma syntax: testi = in the logic above does nothing to
     * the logic, except make the second test possible.
     */
    after = testat;
    testat = testat->next;
  }
  /* ran off end of anonlist? */
  if (testat == NULL) {
    *exact = 0;
    return after; /* Works if after NULL or not. */
  } 
  if ( val == AnonIntegerAtomValue(testi) ) {
    /* stopped within list on a match */
    *exact = 1;
    return testat;
  } else {
    /* stopped on insertion point */
    *exact = 0;
    return after;
  }
}

/*
 * returns the symbol_atom value of i, or NULL if
 * i is not assigned. This wrapper keeps us from
 * hitting whine or assertions in atomvalue.c.
 */
static
symchar *GetAnonInstSymbol(CONST struct Instance *i)
{
  if (AtomAssigned(i)) {
    return GetSymbolAtomValue(i);
  } else {
    return NULL;
  }
}

/* Anon type -> FT, value.
 * Unassigned constant detected merely by NULL symchar value,
 * so just sorting by ptr will handle unassigned.
 */
static
struct AnonType *NearestAnonTypeSC(struct Instance *i,
                                   struct AnonType *after,
                                   int *exact)
{
  struct AnonType *testat;
  CONST struct Instance *testi = NULL;
  symchar *val;

  if (after==NULL) {
    *exact = 0;
    return after;
  }
  val = GetAnonInstSymbol(i);
  /* check for add at front */
  if (val < GetAnonInstSymbol(GAP(after))) {
    *exact = 0;
    return NULL;
  }
  /* Find same value. Sorted by increasing ptr value.
   * alpha no matter, and both must be from symbol table.
   */
  testat = after;
  while (testat != NULL &&
         ((testi = GAP(testat)), val > GetAnonInstSymbol(testi))) {
    /* C comma syntax: testi = in the logic above does nothing to
     * the logic, except make the second test possible.
     */
    after = testat;
    testat = testat->next;
  }
  /* ran off end of anonlist? */
  if (testat == NULL) {
    *exact = 0;
    return after; /* Works if after NULL or not. */
  } 
  if ( val == GetAnonInstSymbol(testi) ) {
    /* stopped within list on a match */
    *exact = 1;
    return testat;
  } else {
    /* stopped on insertion point */
    *exact = 0;
    return after;
  }
}

/* Anon type -> FT, value.
 */
static
struct AnonType *NearestAnonTypeBC(struct Instance *i,
                                   struct AnonType *after,
                                   int *exact)
{
  struct AnonType *testat;
  CONST struct Instance *testi = NULL;
  int val;

  if (after == NULL) {
    *exact = 0;
    return after;
  }
  testat = after;
  testi = GAP(testat);
  /* sort order is UNDEFINED,T,F or UNDEFINED,F,T */
  if (!AtomAssigned(i)) {
    if (!AtomAssigned(testi)) {
      *exact = 1; /* there already */
      return after;
    } else {
      *exact = 0; /* add undefined at head */
      return NULL;
    }
  } else { 
    /* i is not unassigned */
    val = GetBooleanAtomValue(i);
    if (!AtomAssigned(testi)) {
      /* unassigned first list element */
      if (testat->next == NULL) {
        *exact = 0;     /* unassigned only */
        return after;
      } else {
        testat = testat->next;  /* move to first assigned AT and value */
        testi = GAP(testat);
      } /* else T/F first */
    }
  }
  /* T/F first (possibly after undefined) */
  if (val != GetBooleanAtomValue(testi)) {
    if (testat->next != NULL) { /* return 2nd boolean */
      *exact = 1;
      return testat->next;
    } else {
      *exact = 0;       /* add boolean */
      return testat;
    }
  } else {
    *exact = 1; /* return 1st boolean */
    return testat;
  }
}

/*
 * compare two sets. want ordering undefint-undefsym-int-sym,
 * but will settle for undefint-int-undefsym-sym.
 * Wouldn't it be lovely if all the CmpAtomValues functions
 * had semantics which gave us the undefined values first
 * so that we could write one function for all the ATOM
 * types?
 */
static
struct AnonType *NearestAnonTypeSA(struct Instance *i,
                                   struct AnonType *after,
                                   int *exact)
{
  struct AnonType *testat;
  struct Instance *testi = NULL;  /* initialize to satisfy dumb compilers */
  int cmp;
  if (after == NULL) {
    *exact = 0;
    return after;
  }
  
  /* check for add at front */
  cmp = CmpAtomValues(i,GAP(after));
  if (cmp < 0) {
    *exact = 0;
    return NULL;
  }
  /* Find same value. Sorted by decreasing integer value.  */
  testat = after;
  while (testat != NULL &&
         ((testi = GAP(testat)), CmpAtomValues(i,testi) > 0)) {
    /* C comma syntax: testi = in the logic above does nothing to
     * the logic, except make the second test possible.
     */
    after = testat;
    testat = testat->next;
  }
  /* ran off end of anonlist? */
  if (testat == NULL) {
    *exact = 0;
    return after; /* Works if after NULL or not. */
  } 
  if ( CmpAtomValues(i,testi)==0 ) {
    /* stopped within list on a match */
    *exact = 1;
    return testat;
  } else {
    /* stopped on insertion point */
    *exact = 0;
    return after;
  }
}

/*
 * anon type -> FT, parent FT, hollowness.
 * We aren't doing a full treatment because parent ft is
 * available only after a full pass with this partial treatment.
 * Finished comes first, then hollow, in the very limited
 * sort order. NULL is not to be here. 
 */
static
struct AnonType *NearestAnonTypeRelation(struct Instance *i,
                                         struct AnonType *after,
                                         int *exact)
{
  CONST struct Instance *testi = NULL;

  if (after != NULL) {
    testi = GAP(after);
  } else {
    *exact = 0;
    return after;
  }
  /* now testi != NULL */
  if (GetInstanceRelationOnly(i) != NULL) {
    /* i not hollow */
    if (GetInstanceRelationOnly(testi) != NULL) {
      /* filled already seen */
      *exact = 1;
      return after;
    } else {
      /* i is the first filled relation */
      *exact = 0;
      return NULL; /* add first non-hollow relation at head */
    }
  } else {
    /* i hollow */
    if (GetInstanceRelationOnly(testi) != NULL) {
      /* filled already seen */
      if (after->next != NULL) {
        *exact = 1;
        return after->next;
      } else {
        *exact = 0;
        return after; /* add first hollow after head */
      }
    } else {
      /* filled not seen, means only hollow left, since trapped NULL above. */
      *exact = 1;
      return after;
    }
  }
}

/*
 * anon type -> FT, parent FT, hollowness.
 * We aren't doing a full treatment because parent ft is
 * available only after a full pass with this partial treatment.
 * Finished comes first, then hollow, in the very limited
 * sort order. NULL is not to be here. 
 */
static
struct AnonType *NearestAnonTypeLogRel(struct Instance *i,
                                         struct AnonType *after,
                                         int *exact)
{
  CONST struct Instance *testi = NULL;

  if (after != NULL) {
    testi = GAP(after);
  } else {
    *exact = 0;
    return after;
  }
  /* now testi != NULL */
  if (GetInstanceLogRel(i) != NULL) {
    /* i not hollow */
    if (GetInstanceLogRel(testi) != NULL) {
      /* filled already seen */
      *exact = 1;
      return after;
    } else {
      /* i is the first filled relation */
      *exact = 0;
      return NULL; /* add first non-hollow relation at head */
    }
  } else {
    /* i hollow */
    if (GetInstanceLogRel(testi) != NULL) {
      /* filled already seen */
      if (after->next != NULL) {
        *exact = 1;
        return after->next;
      } else {
        *exact = 0;
        return after; /* add first hollow after head */
      }
    } else {
      /* filled not seen, means only hollow left, since trapped NULL above. */
      *exact = 1;
      return after;
    }
  }
}

/*
 * This function handles the special case where we want TmpNum
 * of a NULL instance to be 0 instead of LONG_MAX as it is defined
 * in the tmpnum header.
 * Unclassified instances will cause an abort or be treated as
 * if they were NULL instances. Either treatment is an error,
 * but this should never be called on an unclassified instance.
 */
static
unsigned long GetInstanceAnonIndex(struct Instance *i)
{
  if (i != NULL) {
#if ATDEBUG
    unsigned long n;
    n = GetTmpNum(i);
    assert(n); /* child must have been already classified! */
    return n;
#else
    return GetTmpNum(i);
#endif
  } else {
    return 0;
  }
}

/*
 * On entry, testat matches i except possibly in merges and after is either
 * testat (and first of the FT list) or the AT before testat
 * in the FT list.
 */
static
enum search_status MatchATMerges(struct Instance *i,
                                 struct AnonType **after_p,
                                 struct AnonType **testat_p,
                                 int *exactfamily)
{
  struct AnonType *after;
  struct AnonType *testat;
  struct Instance *testi;
  int cmp;
  enum search_status s;

  after = *after_p;
  testat = *testat_p;
  testi = GAP(testat);

  *exactfamily = testat->exactfamily;
  assert(*exactfamily != 0);
  cmp = Asc_AnonMergeCmpInstances(i,testi);
  assert(cmp != 2);
  if (cmp == 0) {
    s = at_exact;
  } else {
    if (cmp < 0) {
      /* insert at beginning of family section of ATs */
      s = at_previous;
    } else {
      /* move right until out of family, or NULL AT or found location. */
      while (cmp > 0) {
        if (testat->next == NULL) {
          s = at_append;  /* testat is end of at list, exact is 0 */
          break;          /* exit while early */
        } 
        if (testat->next->exactfamily != *exactfamily) {
          /* ok, so testat is the right edge of our exactfamily 
           * and we want to append the family.
           */
          s = at_append;
          break;
        } else {
          /* move right */
          after = testat;
          testat = testat->next;
          testi = GAP(testat);
          cmp = Asc_AnonMergeCmpInstances(i,testi);
          assert(cmp != 2);
        }
      }
      /* cmp <= 0 but there is at least one family member i is > than. */
      if (cmp == 0) {
        /* found it */
        s = at_exact;
      } else {
        /* passed it. insert after 'after'. */
        s = at_previous;
      }
    }
  }
  *after_p = after;
  *testat_p = testat;
  return s;
}

/* i1, i2 assumed != and not NULL. 
 * The ArrayAnonCmp function assume that the arrays to be compared have the
 * same array type description and level of indirection so that 
 * the basetype and set description
 * of the subscripts are identical -- eliminates int vs enum problems.
 */
static
int ArrayAnonCmpInt(CONST struct Instance *i1, CONST struct Instance *i2)
{
  unsigned long len;
  long n1,n2;
  assert(i1!=NULL);
  assert(i2!=NULL);
  assert(i1!=i2);
  len = NumberChildren(i1);
  if (NumberChildren(i2)!=len) {
    /* sort more children to later */
    return ((len > NumberChildren(i2)) ? -1 : 1);
  }
  while (len > 0) {
    n1 = InstanceIntIndex(ChildName(i1,len));
    n2 = InstanceIntIndex(ChildName(i2,len));
    if (n1 != n2) {
      return ((n1 > n2) ? -1 : 1);
    }
    n1 = (long)GAIN(InstanceChild(i1,len));
    n2 = (long)GAIN(InstanceChild(i2,len));
    if (n1 != n2) {
      return ((n1 > n2) ? -1 : 1);
    }
    len--;
  }
  return 0;
}

/*
 * returns TRUE if # of children, their names, and their anon type
 * indices are all the same.
 */
static
int ArrayAnonCmpEnum(CONST struct Instance *i1, CONST struct Instance *i2)
{
  unsigned long len;
  long n1,n2;
  symchar *s1, *s2;
  assert(i1!=NULL);
  assert(i2!=NULL);
  assert(i1!=i2);
  len = NumberChildren(i1);
  if (NumberChildren(i2)!=len) {
    /* sort more children to later */
    return ((len > NumberChildren(i2)) ? -1 : 1);
  }
  while (len > 0) {
    /* since it's internal, we sort on pointer rather than symbol content */
    s1 = InstanceStrIndex(ChildName(i1,len));
    s2 = InstanceStrIndex(ChildName(i2,len));
    if (s1 != s2) {
      return ((s1 > s2) ? -1 : 1);
    }
    n1 = (long)GAIN(InstanceChild(i1,len));
    n2 = (long)GAIN(InstanceChild(i2,len));
    if (n1 != n2) {
      return ((n1 > n2) ? -1 : 1);
    }
    len--;
  }
  return 0;
}

static
struct AnonType *NearestAnonTypeArrayEnum(struct Instance *i,
                                          struct AnonType *after,
                                          int *exact,
                                          int *exactfamily)
{
  struct AnonType *testat;
  struct Instance *testi;
  enum search_status s;

  if (after == NULL) {
    *exact = 0;
    return NULL;
  }
  if (ArrayAnonCmpEnum(i,GAP(after)) < 0) {
    *exact = 0;
    return NULL;
  }
  testat = after;
  while (testat != NULL &&
         ((testi = GAP(testat)), ArrayAnonCmpEnum(i,testi) > 0)) {
    after = testat;
    testat = testat->next;
  }
  if (testat == NULL) {
    /* append */
    *exact = 0;
    return after;
  }
  if (testat == after) {
    /* didn't enter while, and < case ruled out before entry */
    /* check for mergedness same here. if not identical,
     * return 0 for *exact and cause append or insert(ret NULL) of new AT.
     * i and testi are in the same family.
     */
    s = MatchATMerges(i,&after,&testat,exactfamily);
    switch (s) {
    case at_exact:
      *exact = 1;
      return testat;
    case at_previous:
      *exact = 0;
      return NULL; /* new AT at head of list */
    case at_append:
      *exact = 0;
      return after;
    case at_notdone:
      /* we screwed up */
    default:
      Asc_Panic(2,"NearestAnonTypeArrayInt","Returning while not (1) done");
      exit(2); /* NOTREACHED, but quiets gcc */
    }
  } 
  if (ArrayAnonCmpEnum(i,GAP(after)) < 0) {
    *exact = 0;
    return after;
  } else {
    s = MatchATMerges(i,&after,&testat,exactfamily);
    switch (s) {
    case at_exact:
      *exact = 1;
      return testat;
    case at_previous:
      *exact = 0;
      return after; /* new AT at head of family */
    case at_append:
      *exact = 0;
      return after;
    case at_notdone:
      /* we screwed up */
    default:
      Asc_Panic(2,"NearestAnonTypeArrayInt","Returning while not (2) done");
      exit(2); /* NOTREACHED, but quiets gcc */
    }
  }
}

/*
 * The anonymous type of an array is really determined by its
 * formal type (statement+basetype) and it parent MODEL's anonymous
 * type. But since we work bottom up, we instead classify by
 * the system generated array typedescription, the # of children,
 * and the subscript name/AT of each child.
 * This classification function looks rather simple, mainly
 * because the real dirt goes on in the ArrayAnonCmpInt call.
 * 
 * We also have to compare mergedness of arrays of models/vars.
 */
static
struct AnonType *NearestAnonTypeArrayInt(struct Instance *i,
                                         struct AnonType *after,
                                         int *exact,
                                         int *exactfamily)
{

  struct AnonType *testat;
  struct Instance *testi;
  enum search_status s;

  if (after == NULL) {
    *exact = 0;
    return NULL;
  }
  if (ArrayAnonCmpInt(i,GAP(after)) < 0) {
    *exact = 0;
    return NULL;
  }
  testat = after;
  while (testat != NULL &&
         ((testi = GAP(testat)), ArrayAnonCmpInt(i,testi) > 0)) {
    after = testat;
    testat = testat->next;
  }
  /* at this point, testi is from after */
  if (testat == NULL) {
    /* append */
    *exact = 0;
    return after;
  }
  if (testat == after) {
    /* didn't enter while, and < case ruled out before entry */
    /* check for mergedness same here. if not identical,
     * return 0 for *exact and cause append or insert(ret NULL) of new AT.
     * i and testi are in the same family.
     */
    s = MatchATMerges(i,&after,&testat,exactfamily);
    switch (s) {
    case at_exact:
      *exact = 1;
      return testat;
    case at_previous:
      *exact = 0;
      return NULL; /* new AT at head of list */
    case at_append:
      *exact = 0;
      return after;
    case at_notdone:
      /* we screwed up */
    default:
      Asc_Panic(2,"NearestAnonTypeArrayInt","Returning while not (1) done");
      exit(2); /* NOTREACHED, but quiets gcc */
    }
  } 
  if (ArrayAnonCmpInt(i,GAP(after)) < 0) {
    *exact = 0;
    return after;
  } else {
    s = MatchATMerges(i,&after,&testat,exactfamily);
    switch (s) {
    case at_exact:
      *exact = 1;
      return testat;
    case at_previous:
      *exact = 0;
      return after; /* new AT at head of family */
    case at_append:
      *exact = 0;
      return after;
    case at_notdone:
      /* we screwed up */
    default:
      Asc_Panic(2,"NearestAnonTypeArrayInt","Returning while not (2) done");
      exit(2); /* NOTREACHED, but quiets gcc */
    }
  }
}

/*
 * anon type -> FT, child ATs
 * Now, this is an interesting function. The idea is that we want
 * as simple as data structure as can be reasonably had and still
 * find exact matches (or the need for a new AT) in linear time.
 * Presumably a sorted strucure is faster as it permits us to do
 * fewer than nAT comparisons of the instance being classified
 * against the anonlist. 
 * The minimum time to diagnose an exact match of two instances,
 * assuming all children already classified or NULL, is linear
 * in the number of children.
 * So the sort order/search can be explained with the following
 * example assume for a formal type there are 5 existing ATs.
 * Represent each AT as a column of numbers where the numbers
 * are the AT->index of each child. Here nAT == 5.
 * example anonlist linked in order abcde       instance to be classified:
 *  AT: a       b       c       d       e       CH       f
 * CH:------------------------------------              -
 * 1'|  1*      1       1       1       1       1^      1*
 * 2'|  2*      2*      3*      3       3       2^      3*
 * 3'|  3       4       3*      4*      4       3^      4*
 * 4'|  5       5       5       5*<     5       4^      4*
 * 5'|  6       6       6       6       7       5^      6
 *
 * The maximum cost is k*(nAT+nCH) integer comparisons.
 * We can search by doing integer comparisons with the *'d numbers.
 * When we reach the final point where 4' > 4^ (index 5 > 4)
 * we no we have a new at and it should go in before AT d.
 *
 * So, the logic required for this picture is given in the function body.
 * The obscuring boundary case (there's always one of those :-() is
 * what to do with NULL instances. They can be most conveniently dealt
 * with by assigning them all (regardless of eventual kind) the 
 * AT index of 0 (convenient since real indices start at 1, as they
 * are stored in a gl_list). Unfortunately, this makes a special case
 * out of them for TmpNum purposes because GetTmpNum returns
 * LONG_MAX when it is called on a NULL instance. It would be nice
 * if TmpNum had originally been designed to return 0 on NULL,
 * but we can't mess with the TmpNum convention now.
 * We get rid of the special case with a TmpNum wrapper.
 * This function is really the pivot for the design of the
 * rest of the file.
 *
 * The above tells most of the story. In addition to comparing
 * anonymous types of children, we must then also compare the
 * merges listed on the instance being classified. Things are
 * going to be down-right twisty, but the basic idea of the
 * picture above still holds. The comparison of merges in
 * effect adds an extra row (think of the merges, in toto, as
 * a really weird child). Very seldom in practical models will
 * a merge comparison turn up anything other than equal, but
 * we must do it to have a correct compiler.
 *
 * Note that we could do the rightward search in the grid above
 * on the children by looking at the child's at index. Merge
 * information isn't typed, however, so we can't use it as a
 * criteria for moving (and stopping) rightward. So we invent
 * the exact family identifier in an AT. A set of AT's which
 * are identical up to (but not including) the merge information
 * will all have the same family identifier. Within a family
 * identifier group, a comparison search (initially linear until
 * we prove it needs speedup) for the proper place to add an
 * instance is necessary.
 *
 * exactfamily is assumed to hold 0 on entry. If an instance matches
 * an existing AT up to but not including the merged descendant
 * information, then exactfamily will be set to the family id of
 * the group the instance belongs to.
 */
static
struct AnonType *NearestAnonTypeModel(struct Instance *i,
                                      struct AnonBucket *b,
                                      int *exact,
                                      int *exactfamily)
{
  /* make the code more compact but not too obscure with GAIN,GAP */

  struct AnonType *testat, *after;
  CONST struct Instance *testi = NULL;
  unsigned long index, testindex; /* 0 = NULL child */
  unsigned long c,len;
  enum search_status s;

  assert(b!=NULL);
  after = b->anonlist;
#if ATDEBUG
  FPRINTF(ASCERR,"NearestAnonTypeModel: checking children\n",c);
#endif
  if (after == NULL) {
    *exact = 0;
    return after;
  }
  len = ChildListLen(GetChildList(b->d));
  if (!len) {  
    /* childless --> AT == FT */
    *exact = 1;
    return after;
  }
  s = at_notdone;
#if ATDEBUG
  FPRINTF(ASCERR,"NearestAnonTypeModel: at_notdone set starting len = %lu\n",len);
#endif
  testat = after;
  /* for loop will be entered */
  for (c = 1; c <= len && s == at_notdone; c++) {
    index = GAIN(InstanceChild(i,c));
    testi = GAP(testat); /* if at never changes, this is redundant,
                          * but redundant is cheaper than the logic
                          * to avoid redundancy.
                          */
    testindex = GAIN(InstanceChild(testi,c));
    while (testindex < index) {
      if (testat->next == NULL) {
        s = at_append;  /* testat is end of at list, exact is 0 */
#if ATDEBUG
        FPRINTF(ASCERR,"NearestAnonTypeModel: at_append set (index=%lu, testindex=%lu)\n",index, testindex);
#endif
        break;          /* exit while early */
      } else {
        /* move right */
#if ATDEBUG
        FPRINTF(ASCERR,"NearestAnonTypeModel: moving right(index=%lu, testindex=%lu)\n",index, testindex);
#endif
        after = testat;
        testat = testat->next;
        testi = GAP(testat);
        testindex = GAIN(InstanceChild(testi,c));
      }
    }
    /* append, or testindex >= index. = -> on to next child, > -> insert. */
    if (s == at_notdone && testindex > index) {
      s = at_previous;  /* insert new at between after, testat */
#if ATDEBUG
      FPRINTF(ASCERR,"NearestAnonTypeModel: at_previous set at c = %lu (index=%lu, testindex=%lu)\n",c, index, testindex);
#endif
      break;            /* exit for early */
    }
    /* index == test index */
  }

#if ATDEBUG
  FPRINTF(ASCERR,"NearestAnonTypeModel: after loop c = %lu, s = %d\n",c,s);
#endif
  if (c > len && s == at_notdone) {
    /* now we have to compare the merge lists, sigh.  The for loop
     * leaves us with the first AT in our exact family.
     */
    s = MatchATMerges(i,&after,&testat,exactfamily); 
  }
#if ATDEBUG
  FPRINTF(ASCERR,"NearestAnonTypeModel: after merge check c = %lu, s = %d\n",c,s);
#endif

  switch (s) {
  case at_exact:
    *exact = 1;
    return testat;
  case at_previous:
    *exact = 0;
    if (after == testat) {
      /* very first at in formal type list */
      return NULL;
    } else {
      /* we moved 1 past the place after which i belongs */
      return after;
    }
  case at_append:
    /* we are at the place after which i belongs */
    *exact = 0;
    return testat;
  case at_notdone:
    /* we screwed up */
  default:
    Asc_Panic(2,"NearestAnonTypeModel","Returning while not done");
    exit(2); /* NOTREACHED, but quiets gcc */
  }
}

/*
 * This is where the evil comparisons and linked list traversal are
 * all distributed. Particularly complex cases are arrays/models.
 * We're searching for the AT nearest to the yet undetermined
 * AT of i. All the children of compound i are assumed already
 * classified or NULL. b is the bucket for the formal type of
 * i. *exact is returned 1 if match is found, or 0 if not.
 * If not, then the AT returned is the AT in b after which the
 * new AT that i implies should be inserted.
 * In the comments of this function, CT -> context, FT -> formal type.
 * By the time we reach this function, there isn't a question of
 * competing types with the same name, so anything determined entirely
 * by the formal type of the bucket, b, will have a bucket with length 
 * 0 or 1. Every existing AT encountered is assumed to have at least
 * 1 instance associated with it.
 *
 * exactfamily will be 0 and meaningless if the match is exact.
 * it will be 0 if the match is inexact and a new family is
 * needed (only arrays and models need families at all, but it
 * doesn't hurt to give all ATs a family so we do. If exactfamily
 * is nonzero, then the AT to be created (because exact will be 0)
 * should have as its family id the value of *exactfamily.
 */
static
struct AnonType *NearestAnonType(struct Instance *i,
                                 struct AnonBucket *b,
                                 int *exact,
                                 int *exactfamily)
{

  *exactfamily = 0;
  switch(InstanceKind(i)) {
  case REAL_CONSTANT_INST:
    return NearestAnonTypeRC(i,b->anonlist,exact); /* done? */
  case REAL_ATOM_INST:
    return NearestAnonTypeRA(i,b->anonlist,exact); /* done */
  case INTEGER_CONSTANT_INST:
    return NearestAnonTypeIC(i,b->anonlist,exact); /* done */
  case SYMBOL_CONSTANT_INST:
    return NearestAnonTypeSC(i,b->anonlist,exact); /* done */
  case BOOLEAN_CONSTANT_INST:
    return NearestAnonTypeBC(i,b->anonlist,exact); /* done */
  case SET_ATOM_INST:
    return NearestAnonTypeSA(i,b->anonlist,exact); /* done */

  case ARRAY_ENUM_INST:
    return NearestAnonTypeArrayEnum(i,b->anonlist,exact,exactfamily);
  case ARRAY_INT_INST:
    return NearestAnonTypeArrayInt(i,b->anonlist,exact,exactfamily);

  case MODEL_INST:
    return NearestAnonTypeModel(i,b, /* need the whole bucket */
                                exact, exactfamily); 

  /* For all the following anon type -> FT.
   * either there's already an AT or there isn't. 
   * Strictly speaking, the wheninst is determined by parent AT,
   * but this is not available.
   */
  case INTEGER_ATOM_INST:       /* FALL THROUGH */
  case SYMBOL_ATOM_INST:        /* FALL THROUGH */
  case BOOLEAN_ATOM_INST:       /* FALL THROUGH */
  case WHEN_INST:               /* FALL THROUGH */
  case DUMMY_INST:
    *exact = (b->anonlist != NULL);
    return b->anonlist;		/* done */

  /* log/rels can be one of: (NULL not seen), created hollow, or done. */
  case REL_INST:                /* done? */
    return NearestAnonTypeRelation(i,b->anonlist,exact);
  case LREL_INST:             /* done? */
    return NearestAnonTypeLogRel(i,b->anonlist,exact);

  /* For these anon type -> FT, but who cares? we don't classify these. */
  case REAL_INST:               /* FALL THROUGH */
  case INTEGER_INST:            /* FALL THROUGH */
  case SYMBOL_INST:             /* FALL THROUGH */
  case BOOLEAN_INST:            /* FALL THROUGH */
  case SET_INST:
    Asc_Panic(2,"NearestAnonType","Called with subatomic instance");
    return NULL; /* NOT REACHED, but shuts up gcc */

  case SIM_INST:
    Asc_Panic(2,"NearestAnonType","Called with SIM_INST kind");
    return NULL; /* NOT REACHED, but shuts up gcc */

  default:
    Asc_Panic(2,"NearestAnonType","Called with unknown instance kind");
    return NULL; /* NOT REACHED, but shuts up gcc */
  }
}

/*
 * Classifies each instance it visits, based on data in info
 * and instance tmpnums. Do not visit children of atoms with this
 * function because it will crash as intended.
 * This assumes all the children of the visited models/arrays have
 * already been classified, so it will need a bottom-up visit.
 */
static
void DeriveAnonType(struct Instance *i, struct AnonVisitInfo *info)
{
  struct AnonType  *at, *after;
  struct AnonBucket *b;
  int exact, exactfamily;

  if (i==NULL) {
    info->errors++;
    Asc_Panic(2,"DeriveAnonType","Function called with NULL instance");
    return;
  }
  if (GetTmpNum(i) != 0L) {
    FPRINTF(ASCERR,"Deriving already visited instance!\n");
    WriteInstanceName(ASCERR,i,info->root);
    FPRINTF(ASCERR,"\n");
    return;
  }
  b = FindAnonBucket(InstanceTypeDesc(i),InstanceIndirected(i),info->t);
  if (b == NULL) {
    b = AddAnonBucket(InstanceTypeDesc(i),InstanceIndirected(i),info->t);
    if (b == NULL) {
      Asc_Panic(2,"DeriveAnonType","AddAnonBucket returned NULL");
    }
  }
  exactfamily = 0;
  after = NearestAnonType(i,b,&exact,&exactfamily);
#if ATDEBUG
  WriteInstanceName(ASCERR,i,info->root);
  FPRINTF(ASCERR,"\nexact = %d. after = 0x%p\n",exact,(void *)after);
#endif
  if (!exact) {
    at = ExpandAnonResult(info->atl); /* create, add to atl , set index */
    if (exactfamily != 0) {
      at->exactfamily = exactfamily;
    } else {
      at->exactfamily = ++(info->nextfamily);
    }
    InsertAnonType(b,at,after);
#if ATDEBUG
    FPRINTF(ASCERR,"\tnew-at = 0x%p\n",(void *)at);
#endif
  } else {
    at = after;
  }
  gl_append_ptr(at->instances,(void *)i);
  /* make damn sure this doesn't give us a big list of universal instances
   * all identical. If it does, visit needs fixing.
   */
  SetTmpNum(i,at->index);
}

static
void DestroyAnonType(struct AnonType *at)
{ 
  if (at == NULL) {
    return;
  }
  gl_destroy(at->instances);
  ascfree(at);
}

void Asc_DestroyAnonList(struct gl_list_t *l)
{
  unsigned long c,len;
  if (l==NULL) {
    return;
  }
  len = gl_length(l);
  for (c = 1 ; c <= len ; c++) {
    DestroyAnonType((struct AnonType *)gl_fetch(l,c));
  }
  gl_destroy(l);
}

/*
 * This function classifies an instance tree from the
 * bottom up and returns the list described above.
 * The list should be destroyed with Asc_DestroyAnonList.
 */
struct gl_list_t *Asc_DeriveAnonList(struct Instance *i)
{
  struct AnonBucket **t;
  struct gl_list_t *atl;
  struct AnonVisitInfo info;
  VOIDPTR vp;
#if (TIMECOMPILER && AMSTAT)
  clock_t start,classt;
#endif
#if (AWAL && defined(__WIN32__))
  char WAL_filename[] = "atmlist.txt";
  char WAL_file[PATH_MAX + 12];
  char *temp_path;
#endif

  ZeroTmpNums(i,0);
  t = CreateAnonTable(ANONTABLESIZE);
  if (t == NULL) {
    return NULL;
  }
  atl = gl_create(ANONEXPECTED);
  if (atl == NULL) {
    DestroyAnonTable(t);
    return NULL;
  }
  info.t = t;
  info.root = i;
  info.atl = atl;
  info.nextfamily = 1;
  info.errors = 0;
  /*
   * Apply function in a bottom up fashion, so that children
   * will all be classified before the parent is classified.
   */
#if (TIMECOMPILER && AMSTAT)
    start = clock();
#endif
  vp = Asc_AnonMergeMarkIPs(i);
#if (TIMECOMPILER && AMSTAT)
    classt = clock();
    FPRINTF(ASCERR,
            "Mergedetect\t\t%lu\n",(unsigned long)(classt-start));
#endif
  SilentVisitInstanceTreeTwo(i,(VisitTwoProc)DeriveAnonType,1,0,(void *)&info);
#if AWAL
  {
    FILE *fp;
#if TIMECOMPILER
    FPRINTF(ASCERR, "start atmlist: %lu\n",(unsigned long)clock());
#endif
#ifdef __WIN32__
    temp_path = getenv("TEMP");   /* put file in TEMP, if defined */
    if (temp_path && (PATH_MAX > strlen(temp_path))) {
      strcpy(WAL_file, temp_path);
      strcat(WAL_file, "\\");
    }
    strcat(WAL_file, WAL_filename);
    fp = fopen(WAL_file,"w+");
#else   /* !__WIN32__ */
    fp = fopen("/tmp/atmlist","w+");
#endif  /* __WIN32__ */
    if (fp == NULL) {
      FPRINTF(ASCERR, "Error opening output file in Asc_DeriveAnonList().\n");
    }
    else {
      Asc_WriteAnonList(fp, atl, i, 1);
      fclose(fp);
    }
#if TIMECOMPILER
    FPRINTF(ASCERR, "done atmlist: %lu\n",(unsigned long)clock());
#endif
  }
#endif /* awal */
  Asc_AnonMergeUnmarkIPs(vp);
  DestroyAnonTable(t);
  /* ZeroTmpNums(i,0);  */
  /* not necessary, really, as any tn user should assume they are dirty */
  return atl;
}

static
void WriteAnonType(FILE *fp, struct AnonType *at,
                   struct Instance *root, char *simple, int mlists)
{
  unsigned long c,len;
  struct Instance *i;
  len = gl_length(at->instances);
  i = GAP(at);
  FPRINTF(fp,"\nAT: k=%d index=%lu, count=%lu %s\n",
          InstanceKind(i), at->index, len,simple);
  if (mlists && IsCompoundInstance(i)!=0) {
    FPRINTF(fp,"  MERGES:\n");
    Asc_AnonMergeWriteList(fp,i);
  }
  FPRINTF(fp,"  ALIKE NAMES:\n");
  for (c=1; c <= len; c++) {
    i = (struct Instance *)gl_fetch(at->instances,c);
    FPRINTF(fp,"      ");
    WriteInstanceName(fp,i,root);
    FPRINTF(fp,"\n");
  }
  i = GAP(at);
#if ATDEBUG
  WriteInstance(fp,i);
#endif
}

static
void WriteAnonEpilog(FILE *fp)
{
  FPRINTF(fp,"AT: name %d %s\n",MODEL_INST,"MODEL_INST");
  FPRINTF(fp,"AT: name %d %s\n",REL_INST,"REL_INST");
  FPRINTF(fp,"AT: name %d %s\n",LREL_INST,"LREL_INST");
  FPRINTF(fp,"AT: name %d %s\n",WHEN_INST,"WHEN_INST");
  FPRINTF(fp,"AT: name %d %s\n",ARRAY_INT_INST,"ARRAY_INT_INST");
  FPRINTF(fp,"AT: name %d %s\n",ARRAY_ENUM_INST,"ARRAY_ENUM_INST");
  FPRINTF(fp,"AT: name %d %s\n",REAL_ATOM_INST,"REAL_ATOM_INST");
  FPRINTF(fp,"AT: name %d %s\n",INTEGER_ATOM_INST,"INTEGER_ATOM_INST");
  FPRINTF(fp,"AT: name %d %s\n",BOOLEAN_ATOM_INST,"BOOLEAN_ATOM_INST");
  FPRINTF(fp,"AT: name %d %s\n",SYMBOL_ATOM_INST,"SYMBOL_ATOM_INST");
  FPRINTF(fp,"AT: name %d %s\n",SET_ATOM_INST,"SET_ATOM_INST");
  FPRINTF(fp,"AT: name %d %s\n",REAL_CONSTANT_INST,"REAL_CONSTANT_INST");
  FPRINTF(fp,"AT: name %d %s\n",BOOLEAN_CONSTANT_INST,"BOOLEAN_CONSTANT_INST");
  FPRINTF(fp,"AT: name %d %s\n",INTEGER_CONSTANT_INST,"INTEGER_CONSTANT_INST");
  FPRINTF(fp,"AT: name %d %s\n",SYMBOL_CONSTANT_INST,"SYMBOL_CONSTANT_INST");
  FPRINTF(fp,"AT: name %d %s\n",DUMMY_INST,"DUMMY_INST");
}

void Asc_WriteAnonList(FILE *fp, struct gl_list_t *atl,
                           struct Instance *root, int mlists)
{
  unsigned long c,len;
  long sum;
  struct AnonType *at,*tat;
  struct Instance *i;
  CONST struct TypeDescription *d, *base;
  int tot;
  char *reln, *simple;
  
  if (atl==NULL) {
    return;
  }
  for (c = 1, len = gl_length(atl); c <= len; c++) {
    at = (struct AnonType *)gl_fetch(atl,c);
    at->visited = 0;
  }
  for (c = 1, len = gl_length(atl); c <= len; c++) {
    at = (struct AnonType *)gl_fetch(atl,c);
    if (!at->visited) {
      /* find head of formal type group */
      while (at->prev != NULL) {
        at = at->prev;
      }
      tat = at;
      tot = 0;
      sum = 0;
      while (tat!=NULL) {
        tot++;
        sum +=  (long)gl_length(tat->instances);
        tat = tat->next;
      }
      i = GAP(at);
      d = InstanceTypeDesc(i);
      simple = reln = "";
      base = NULL;
      if (InstanceIndirected(i) != LONG_MAX) {
        base =  GetArrayBaseType(d);
        if (GetArrayBaseIsRelation(d) || GetArrayBaseIsLogRel(d)) {
          reln="relarray";
        }
      }
      if ((ICOMP & InstanceKind(i)) == 0 ||
            (base != NULL &&
             GetBaseType(base) != model_type &&
             GetBaseType(base) != array_type )) {
        simple = "SiMpLe";
      }
      if (sum==tot) {
        sum = -1;
      }
#define ABP 1 /* ATOMic bypass in writing anontypes */
#if ABP
      if (simple[0]!='S' /*  && InstanceKind(i) != MODEL_INST */) { 
        /* bypass details of ATOmlike things and models. */
#endif
        FPRINTF(fp,
            "\n\nFT: %d %s (indirected = %lu) (tot=%d, sum=%ld) %s %s %s\n",
            InstanceKind(i), SCP(GetName(d)),InstanceIndirected(i),
            tot,sum,reln,((base != NULL)?SCP(GetName(base)):""),simple);
        while (at!=NULL) {
          WriteAnonType(fp,at,root,simple,mlists);
          at->visited = 1;
          at = at->next;
        }
#if ABP
      }
#endif
    }
  }
  WriteAnonEpilog(fp);
}
