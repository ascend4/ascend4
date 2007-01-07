/*
 *
 *  This file is part of the Ascend Language Interpreter.
 *  See top of findpath.h
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
#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "symtab.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "child.h"
#include "type_desc.h"
#include "instance_enum.h"
#include "instance_name.h"
#include "instance_io.h"
#include "mathinst.h"
#include "name.h"
#include "nameio.h"
#include "evaluate.h"
#include "atomvalue.h"
#include "instquery.h"
#include "arrayinst.h"
#include "parentchild.h"
#include "value_type.h"
#include "forvars.h"
#include "setinstval.h"
#include "find.h"
#include "findpath.h"
#include "safe.h"
#include "relation_type.h"
#include "relation_util.h"
#include "logical_relation.h"
#include "logrelation.h"
#include "logrel_util.h"


#define NAMELISTSIZE 20L
#define DEFTOLERANCE 1e-08

#ifndef lint
static CONST char FindRCSid[]="$Id: find.c,v 1.24 1998/03/26 20:39:44 ballan Exp $";
#endif /* lint */

typedef struct IAndName 
{
	CONST struct Instance *i;
	struct Name *n;
} PAN;

/** store a name and the instance that carries it, with
    reference to a context which is not defined here. Thus
    n may be null if i == context.
*/
static
PAN *CreatePAN(CONST struct Instance *i, struct Name *n)
{
	PAN *result = (PAN *)ascmalloc(sizeof(PAN));
	result->i = i;
	result->n = n;
	return result;
}

static
void DestroyPAN(PAN *p)
{
	p->i = NULL;
	p->n = NULL;
	ascfree(p);
}

/* by usage in this file, all n in PAN are allocated for exclusive use. */
static
void DestroyPANAndName(PAN *p)
{
	p->i = NULL;
	DestroyName(p->n);
	p->n = NULL;
	ascfree(p);
}

static
void DestroyPANList(struct gl_list_t **gl)
{
	gl_iterate(*gl,(IterateFunc)DestroyPANAndName);
	gl_destroy(*gl);
	*gl = NULL;
}

static struct gl_list_t *FindArrayChildrenPath(struct gl_list_t *list,
				    CONST struct set_t *sptr,
				    enum find_errors *errval)
{
  struct gl_list_t *result;
  CONST struct Instance *i,*child;
  struct InstanceName rec;
  struct TypeDescription *desc;
  unsigned long c1,len1,c2,len2,pos;
  PAN *p, *p2;
  struct Name *n, *n2;
  symchar *senum;
  long sint;

  switch(SetKind(sptr)){
  case empty_set: return gl_create(0);
  case string_set:
    SetInstanceNameType(rec,StrArrayIndex);
    len2 = Cardinality(sptr);
    len1 = gl_length(list);
    result = gl_create(len1*len2);
    for (c1=1; c1<=len1; c1++){
      p = (PAN *)gl_fetch(list,c1);
      i = p->i;
      if (InstanceKind(i)==ARRAY_ENUM_INST){
	if (NextToExpand(i)!=1){
	  for (c2=1; c2<=len2; c2++){
            senum = FetchStrMember(sptr,c2);
	    SetInstanceNameStrIndex(rec,senum);
	    if ((pos = ChildSearch(i,&rec))==0){
	      DestroyPANList(&result);
	      desc = InstanceTypeDesc(i);
	      if ( GetArrayBaseIsRelation(desc) || GetArrayBaseIsLogRel(desc)){
		*errval = unmade_instance;
	      } else {
		*errval = impossible_instance;
              }
	      return NULL;
	    } else {
              child = InstanceChild(i,pos);
	      if (child!=NULL){
                n = CreateEnumElementName(senum);
                n2 = CopyAppendNameNode(p->n, n);
                DestroyName(n);
                p2 = CreatePAN(child, n2);
		gl_append_ptr(result,(VOIDPTR)p2);
	      } else {
		DestroyPANList(&result);
		*errval = unmade_instance;
		return NULL;
	      }
	    }
	  }
	} else {
	  DestroyPANList(&result);
	  *errval = unmade_instance;
	  return NULL;
	}
      } else {
	DestroyPANList(&result);
	*errval = impossible_instance;
	return NULL;
      }
    }
    return result;
  case integer_set:
    SetInstanceNameType(rec,IntArrayIndex);
    len2 = Cardinality(sptr);
    len1 = gl_length(list);
    result = gl_create(len1*len2);
    for (c1=1; c1<=len1; c1++){
      p = (PAN *)gl_fetch(list,c1);
      i = p->i;
      if (InstanceKind(i)==ARRAY_INT_INST){
	if (NextToExpand(i)!=1){
	  for (c2=1; c2<=len2; c2++){
            sint = FetchIntMember(sptr,c2);
	    SetInstanceNameIntIndex(rec,sint);
	    if ((pos = ChildSearch(i,&rec))==0){
	      DestroyPANList(&result);
	      desc = InstanceTypeDesc(i);
	      if (GetArrayBaseIsRelation(desc) || GetArrayBaseIsLogRel(desc)) {
		*errval = unmade_instance;
	      } else {
		*errval = impossible_instance;
              }
	      return NULL;
	    } else {
	      child = InstanceChild(i,pos);
	      if (child!=NULL){
                n = CreateIntegerElementName(sint);
                n2 = CopyAppendNameNode(p->n, n);
                DestroyName(n);
                p2 = CreatePAN(child, n2);
		gl_append_ptr(result,(VOIDPTR)p2);
	      } else{
		DestroyPANList(&result);
		*errval = unmade_instance;
		return NULL;
	      }
	    }
	  }
	} else {
	  DestroyPANList(&result);
	  *errval = unmade_instance;
	  return NULL;
	}
      } else {
	DestroyPANList(&result);
	*errval = impossible_instance;
	return NULL;
      }
    }
    return result;
  }
  /*NOTREACHED*/
  return NULL;
}

static
struct gl_list_t *FindNextNameElementPath(CONST struct Name *n,
				      struct gl_list_t *list,
				      enum find_errors *errval)
{
  unsigned long pos,c,len;
  struct InstanceName rec;
  CONST struct Instance *current,*child;
  struct value_t setvalue,oldvalue;
  CONST struct Set *sptr;
  struct gl_list_t *result;
  PAN *p, *p2;
  struct Name *n2;

  *errval = correct_instance;
  if (NameId(n)){
    result = gl_create(NAMELISTSIZE);
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,NameIdPtr(n));
    len = gl_length(list);
    for (c=1; c<=len; c++){
      p = (PAN *)gl_fetch(list,c);
      current = p->i;
      pos = ChildSearch(current,&rec);
      if (pos!=0){
	child = InstanceChild(current,pos);
	if (child!=NULL){
          n2 = CopyAppendNameNode(p->n, n);
          p2 = CreatePAN(child, n2);
	  gl_append_ptr(result,(VOIDPTR)p2);
	} else{
	  *errval = unmade_instance;
	  DestroyPANList(&result);
	  return NULL;
	}
      } else{
	*errval = unmade_instance;
        /* it would seem this ought to be undefined_instance,
         * but maybe refinement causes insanity. -- in which case
         * it should be a caller policy to wait, rather than our
         * job to anticipate policy and short circuit things here.
         */
	DestroyPANList(&result);
	return NULL;
      }
    }
    return result;
  } else {
    sptr = NameSetPtr(n);
    setvalue = EvaluateSet(sptr,InstanceEvaluateName);
    switch(ValueKind(setvalue)){
    case integer_value:
    case symbol_value:
    case list_value:
      oldvalue = setvalue;
      if (ListMode) {
	setvalue = CreateOrderedSetFromList(oldvalue);
      } else {
	setvalue = CreateSetFromList(oldvalue);
      }
      DestroyValue(&oldvalue);
      /* intended to fall through to next case */
    case set_value:
      result = FindArrayChildrenPath(list,SetValue(setvalue),errval);
      DestroyValue(&setvalue);
      return result;
    case error_value:
      switch(ErrorValue(setvalue)){
      case illegal_set_use:
	*errval = impossible_instance;
	break;
      default:
	*errval = undefined_instance;
	break;
	/* more needs to be added here */
      }
      DestroyValue(&setvalue);
      return NULL;
    default:
      ASC_PANIC("Need to add to FindNextNameElementPath.\n");
      exit(2);/* Needed to keep gcc from whining */
    }
  }
}


static
struct gl_list_t *RealFindInstancesPath(CONST struct Instance *i,
				    CONST struct Name *n,
				    enum find_errors *errval)
{
  struct gl_list_t *result,*next;
  PAN *p;

  result = gl_create(NAMELISTSIZE);
  p = CreatePAN(i,NULL); /* in context of i, i has no name. */
  gl_append_ptr(result,(VOIDPTR)p);
  while(n!=NULL){
    next = FindNextNameElementPath(n,result,errval);
    DestroyPANList(&result);
    if (next!=NULL){
      result = next;
      n = NextName(n);
    } else {
      return NULL;
    }
  }
  return result;
}

struct gl_list_t *FindInstancesPaths(CONST struct Instance *i,
				CONST struct Name *n,
				enum find_errors *errval)
{
  struct gl_list_t *result;
  unsigned long k, len;
  PAN *p;
  struct Name *n2;
  *errval = impossible_instance;
  if (i == NULL) return NULL;
  AssertMemory(i);
  assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(i);
  *errval = correct_instance;
  result = RealFindInstancesPath(i,n,errval);
  SetEvaluationContext(NULL);
  if (result != NULL)
  {
    /* convert list of pairs to list of names. */
    len = gl_length(result);
    for (k= 1 ; k <= len; k++) {
      p = (PAN*)gl_fetch(result,k);
      n2 = p->n;
      gl_store(result,k,n2);
      DestroyPAN(p);
    }
  }
  return result;
}
