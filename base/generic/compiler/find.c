/*
 *  Ascend Instance Tree Search Implementation
 *  by Tom Epperly
 *  Created: 1/24/90
 *  Version: $Revision: 1.24 $
 *  Version control file: $RCSfile: find.c,v $
 *  Date last modified: $Date: 1998/03/26 20:39:44 $
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

CONST struct Instance *g_EvaluationContext=NULL;
struct for_table_t *g_EvaluationForTable=NULL;
int ListMode=0;           /* 0 = set or normal mode; 1 = list mode */
int EvaluatingSets=0;     /* 1 = in process of set evaluation */
int g_DeclarativeContext=0; /* 0 = declarative processing
			     * !0 = procedural processing.
                             */

int GetDeclarativeContextF()
{
  return g_DeclarativeContext;
}

void SetDeclarativeContextF(int n,char *file,int line)
{
  g_DeclarativeContext = n;
  FPRINTF(ASCERR,"SDC=%d at %s:%d\n",n,file,line);
}


struct Instance *GetEvaluationContextF(void)
{
  return (struct Instance *)g_EvaluationContext;
}
void SetEvaluationContextF(CONST struct Instance *i
#if (EVALDEBUG == 1 || EVALDEBUG == 3)
  , char *file, int line
#endif
)
{
#if (EVALDEBUG == 1 || EVALDEBUG == 3)
  FPRINTF(ASCERR,"setEvalContext:0x%p %s:%d\n",i,file,line);
#endif /*evaldebug*/
  g_EvaluationContext = i;
}
struct for_table_t *GetEvaluationForTableF(void)
{
  return g_EvaluationForTable;
}
void SetEvaluationForTableF(struct for_table_t *ft
#if (EVALDEBUG == 2 || EVALDEBUG == 3)
  , char *file, int line
#endif
)
{
#if (EVALDEBUG == 2 || EVALDEBUG == 3)
  FPRINTF(ASCERR,"setEvalForTable:0x%p %s:%d\n",ft,file,line);
#endif /* evaldebug */
  g_EvaluationForTable = ft;
}

/*
 * checks to see whether the integer atom or integer inst
 * is a legal set index and bitches if not.
 * as currently used, will always return errors when
 * we are in the declarative section because ATOM i
 * is _always_ mutable now that we have integer constants.
 */
static int ProcessIntegersInSets(CONST struct Instance *i)
{
  if (i!=NULL) {
    if (AtomMutable(i)&&(GetDeclarativeContext()==0)){
      FPRINTF(ASCERR, "Error: variable integer used as set index\n");
      WriteInstanceName(ASCERR,i,NULL);
      FPRINTF(ASCERR,"\n");
      return 1;
    }
    else return 0;
  }
  return 2;
}

static struct gl_list_t *RealFindInstances(CONST struct Instance *i,
				    CONST struct Name *n,
				    enum find_errors *errval);

struct value_t InstanceEvaluateName(CONST struct Name *nptr)
{
  struct gl_list_t *list;
  struct Instance *inst;
  struct value_t result;
  unsigned long c,len;
  enum find_errors errval;
  struct for_var_t *ptr;
  symchar *name;

  if (GetEvaluationForTable()!=NULL){
    /* check FOR vars list before user vars */
    AssertMemory(GetEvaluationForTable());
    if ((name = SimpleNameIdPtr(nptr))!=NULL){
      if ((ptr = FindForVar(GetEvaluationForTable(),name))!=NULL){
	switch(GetForKind(ptr)){
	case f_integer:
	  return CreateIntegerValue(GetForInteger(ptr),1);
	case f_symbol:
	  return CreateSymbolValue(GetForSymbol(ptr),1);
	case f_set:
	  return CreateSetValue(CopySet(GetForSet(ptr)));
	default:
	  FPRINTF(ASCERR,"Untyped for variable.\n");
	  return CreateErrorValue(undefined_value);
	}
      }
    }
  }
  /* didn't find or bomb, so must be in user vars */
  if (GetEvaluationContext()==NULL) return CreateErrorValue(incorrect_name);
  AssertMemory(GetEvaluationContext());
  list = RealFindInstances(GetEvaluationContext(),nptr,&errval);
  if (list==NULL){
    assert(errval!=correct_instance);
    switch(errval){
    case unmade_instance:
      return CreateErrorValue(name_unfound);
    case undefined_instance:
      return CreateErrorValue(undefined_value);
    case impossible_instance:
      return CreateErrorValue(incorrect_name);
    default: /*NOTREACHED*/
      break;
    }
    /*NOTREACHED*/
  } else{
    /* found user's variable */
    AssertMemory(list);
    if (gl_length(list)==1){
      inst = (struct Instance *)gl_fetch(list,1);
      AssertMemory(inst);
      gl_destroy(list);
      switch(InstanceKind(inst)){
      case REAL_INST:
      case REAL_ATOM_INST:
      case REAL_CONSTANT_INST:
	if (AtomAssigned(inst)) {
	  return CreateRealValue(RealAtomValue(inst),RealAtomDims(inst),
				IsConstantInstance(inst));
	} else {
	  return CreateErrorValue(undefined_value);
        }
      case BOOLEAN_INST:
      case BOOLEAN_ATOM_INST:
      case BOOLEAN_CONSTANT_INST:
	if (AtomAssigned(inst)) {
	  return CreateBooleanValue(GetBooleanAtomValue(inst),
					IsConstantInstance(inst));
	} else {
	  return CreateErrorValue(undefined_value);
        }
      case INTEGER_CONSTANT_INST:
	if (inst !=NULL && AtomAssigned(inst)) {
	  return CreateIntegerValue(GetIntegerAtomValue(inst),1);
	} else {
	  return CreateErrorValue(undefined_value);
        }
      case INTEGER_ATOM_INST:
      case INTEGER_INST:
	if (EvaluatingSets){
	  int piis;
          piis = ProcessIntegersInSets(inst);
	  if(piis==1) {
	    return CreateErrorValue(type_conflict);
          }
	  if(piis==2) {
	    return CreateErrorValue(incorrect_name);
          }
	}
	if (AtomAssigned(inst)) {
	  return CreateIntegerValue(GetIntegerAtomValue(inst),0);
	} else {
	  return CreateErrorValue(undefined_value);
        }
      case SET_ATOM_INST:
	if (ListMode) {
          /* more kaa-ism. client should be checking this. not us. */
	  FPRINTF(ASCERR,"Sets are not allowed in lists !!\n");
	  return CreateErrorValue(illegal_set_use);
	}
	if (AtomAssigned(inst)) {
	  return CreateSetValue(CopySet(SetAtomList(inst)));
	} else {
	  return CreateErrorValue(undefined_value);
        }
      case SYMBOL_INST:
      case SYMBOL_ATOM_INST:
      case SYMBOL_CONSTANT_INST:
	if (AtomAssigned(inst)) {
	  return CreateSymbolValue(GetSymbolAtomValue(inst),
					IsConstantInstance(inst));
	} else {
	  return CreateErrorValue(undefined_value);
        }
      case SET_INST:
	if (ListMode) {
          /* more kaa-ism. client should be checking this. not us. */
	  FPRINTF(ASCERR,"Sets are not allowed in lists !!\n");
	  return CreateErrorValue(illegal_set_use);
	}
	if (AtomAssigned(inst)) {
	  return CreateSetValue(CopySet(SetAtomList(inst)));
	} else {
	  return CreateErrorValue(undefined_value);
        }
      default:
	return CreateErrorValue(incorrect_name);
      }
    } else {
    /* BUG BAA this block may be incorrect. what is it? Looks like kaaism*/
      if (GetDeclarativeContext()==0) {
	/* find out if this is ever called in instantiation */
FPRINTF(ASCERR,"BAA debug: please tell ballan@cs.cmu.edu you saw:\n");
WriteName(ASCERR,nptr);
FPRINTF(ASCERR,"\nin compiling what MODEL.\n");
      }
      result = CreateEmptyListValue();
      len = gl_length(list);
      for(c=1; c<=len; c++){
	inst = (struct Instance *)gl_fetch(list,c);
	AssertMemory(inst);
      /* Don't know why only integers should be valid in lists.
       * He who introduces crap should at least be complete about it.
       * Somewhere there's probably a client not doing proper checking
       * trying to pawn it off on us to do here. That client is now broken.
       */
#define OLDCRAP 0
#if OLDCRAP /* integer only */
	switch(InstanceKind(inst)) {
	case INTEGER_ATOM_INST:
	  if (AtomAssigned(inst)) {
	    AppendToListValue(result,
			      CreateIntegerValue(GetIntegerAtomValue(inst),
				IsConstantInstance(inst)));
	  } else {
	    return CreateErrorValue(undefined_value);
          }
	  break;
	case SET_ATOM_INST:
	case SYMBOL_ATOM_INST:
	case INTEGER_INST:
	case SET_INST:
	case SYMBOL_INST:
	default:
	  gl_destroy(list);
	  DestroyValue(&result);
	  return CreateErrorValue(incorrect_name);
	}
#else
  /* extended oldcrap to include this. baa. 3/98 */
	switch(InstanceKind(inst)) {
	case REAL_ATOM_INST:
	case REAL_CONSTANT_INST:
	  if (AtomAssigned(inst)) {
	    AppendToListValue(result,
			      CreateRealValue(RealAtomValue(inst),
                                              RealAtomDims(inst),
				IsConstantInstance(inst)));
	  } else {
	    return CreateErrorValue(undefined_value);
          }
	  break;
	case INTEGER_ATOM_INST:
	case INTEGER_CONSTANT_INST:
	  if (AtomAssigned(inst)) {
	    AppendToListValue(result,
			      CreateIntegerValue(GetIntegerAtomValue(inst),
				IsConstantInstance(inst)));
	  } else {
	    return CreateErrorValue(undefined_value);
          }
	  break;
	case SET_ATOM_INST:
	  if (AtomAssigned(inst)) {
	    AppendToListValue(result,
	        CreateSetValue(CopySet(SetAtomList(inst))));
	  } else {
	    return CreateErrorValue(undefined_value);
          }
	  break;
	case SYMBOL_ATOM_INST:
	case SYMBOL_CONSTANT_INST:
	  if (AtomAssigned(inst)) {
	    AppendToListValue(result,
			      CreateSymbolValue(GetSymbolAtomValue(inst),
				IsConstantInstance(inst)));
	  } else {
	    return CreateErrorValue(undefined_value);
          }
	  break;
        /* ok, we will uniformly reject lists of subatomics, though
	 * it is arbitrary and inconsistent to do so.
         */
	case INTEGER_INST:
	case SET_INST:
	case SYMBOL_INST:
	default:
	  gl_destroy(list);
	  DestroyValue(&result);
	  return CreateErrorValue(incorrect_name);
	}
#endif
      }
      gl_destroy(list);
      return result;
    }
  }
  /* we need to verify that this is not reached */
  FPRINTF(ASCERR,"InstanceEvaluateName returning unexpectedly\n");
  return CreateErrorValue(incorrect_name);
}



/* Specially to evaluate name of relations, logrelations in SATISFIED
 * expressions
 */
struct value_t InstanceEvaluateSatisfiedName(CONST struct Name *nptr,
                                             double tol)
{
  struct gl_list_t *list;
  struct Instance *inst;
  enum find_errors errval;
  struct for_var_t *ptr;
  symchar *name;
  CONST struct relation *rel;
  enum Expr_enum relop;
  enum safe_err status = safe_ok;
  double res;
  int logres;

  if (GetEvaluationForTable()!=NULL){
    AssertMemory(GetEvaluationForTable());
    if ((name = SimpleNameIdPtr(nptr))!=NULL){
      if ((ptr = FindForVar(GetEvaluationForTable(),name))!=NULL){
	switch(GetForKind(ptr)){
	case f_integer:
	  return CreateIntegerValue(GetForInteger(ptr),1);
	case f_symbol:
	  return CreateSymbolValue(GetForSymbol(ptr),1);
	case f_set:
	  return CreateSetValue(CopySet(GetForSet(ptr)));
	default:
	  FPRINTF(ASCERR,"Untyped for variable.\n");
	  return CreateErrorValue(undefined_value);
	}
      }
    }
  }
  if (GetEvaluationContext()==NULL) return CreateErrorValue(incorrect_name);
  AssertMemory(GetEvaluationContext());
  list = RealFindInstances(GetEvaluationContext(),nptr,&errval);
  if (list==NULL){
    assert(errval!=correct_instance);
    switch(errval){
    case unmade_instance:
      return CreateErrorValue(name_unfound);
    case undefined_instance:
      return CreateErrorValue(undefined_value);
    case impossible_instance:
      return CreateErrorValue(incorrect_name);
    default: /*NOTREACHED*/
      break;
    }
    /*NOTREACHED*/
  }
  else{
    AssertMemory(list);
    if (gl_length(list)==1){
      inst = (struct Instance *)gl_fetch(list,1);
      AssertMemory(inst);
      gl_destroy(list);
      switch(InstanceKind(inst)){
      case REL_INST:
        status = RelationCalcResidualPostfixSafe(inst,&res);
        if (status != safe_ok) {
        FPRINTF(ASCERR,
            "Something wrong while calculating a residual in Sat Expr\n");
	return CreateErrorValue(undefined_value);
        }
        rel = GetInstanceRelationOnly(inst);
        relop = RelationRelop(rel);
        switch(relop) {
        case e_equal:
          if (tol != DBL_MAX) {
            if(fabs(tol)>fabs(res)) {
	      return CreateBooleanValue(1,0);
            }
            else {
	      return CreateBooleanValue(0,0);
            }
          }
          else {
            if((DEFTOLERANCE)>fabs(res)) {
	      return CreateBooleanValue(1,0);
            }
            else {
	      return CreateBooleanValue(0,0);
            }
          }
        case e_notequal:
          if (tol != DBL_MAX) {
            if(fabs(tol)>fabs(res)) {
              return CreateBooleanValue(0,0);
            }
            else {
              return CreateBooleanValue(1,0);
            }
          }
          else {
            if((DEFTOLERANCE)>fabs(res)) {
              return CreateBooleanValue(0,0);
            }
            else {
              return CreateBooleanValue(1,0);
            }
          }
        case e_greater:
          if (tol != DBL_MAX) {
            if(fabs(tol)<res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
          else {
            if((DEFTOLERANCE)<res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
        case e_greatereq:
          if (tol != DBL_MAX) {
            if(-fabs(tol)<res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
          else {
            if(-(DEFTOLERANCE)<res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
        case e_less:
          if (tol != DBL_MAX) {
            if(-fabs(tol)>res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
          else {
            if(-(DEFTOLERANCE)>res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
        case e_lesseq:
          if (tol != DBL_MAX) {
            if(fabs(tol)>res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
          else {
            if((DEFTOLERANCE)>res) {
              return CreateBooleanValue(1,0);
            }
            else {
              return CreateBooleanValue(0,0);
            }
          }
        default:
          FPRINTF(ASCERR,
               "Something wrong while calculating a residual in Sat Expr\n");
          return CreateErrorValue(incorrect_name);
	}
      case LREL_INST:
        if (LogRelCalcResidual(inst,&logres)) {
          FPRINTF(ASCERR,
            "Something wrong while calculating a log residual in Sat Expr\n");
 	  return CreateErrorValue(undefined_value);
        }
	return CreateBooleanValue(logres,0);
      default:
	return CreateErrorValue(incorrect_name);
      }
    }
    else {
      FPRINTF(ASCERR,"InstanceEvaluateSatisfiedName returning unexpectedly\n");
      gl_destroy(list);
      return CreateErrorValue(undefined_value);
    }
  }
  /* we need to verify that this is not reached */
  FPRINTF(ASCERR,"InstanceEvaluateSatisfiedName returning unexpectedly\n");
  return CreateErrorValue(incorrect_name);
}


static struct gl_list_t *FindArrayChildren(struct gl_list_t *list,
				    CONST struct set_t *sptr,
				    enum find_errors *errval)
{
  struct gl_list_t *result;
  struct Instance *i,*child;
  struct InstanceName rec;
  struct TypeDescription *desc;
  unsigned long c1,len1,c2,len2,pos;
  switch(SetKind(sptr)){
  case empty_set: return gl_create(0);
  case string_set:
    SetInstanceNameType(rec,StrArrayIndex);
    len2 = Cardinality(sptr);
    len1 = gl_length(list);
    result = gl_create(len1*len2);
    for(c1=1; c1<=len1; c1++){
      i = (struct Instance *)gl_fetch(list,c1);
      if (InstanceKind(i)==ARRAY_ENUM_INST){
	if (NextToExpand(i)!=1){
	  for(c2=1; c2<=len2; c2++){
	    SetInstanceNameStrIndex(rec,FetchStrMember(sptr,c2));
	    if ((pos = ChildSearch(i,&rec))==0){
	      gl_destroy(result);
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
		gl_append_ptr(result,(VOIDPTR)child);
	      } else {
		gl_destroy(result);
		*errval = unmade_instance;
		return NULL;
	      }
	    }
	  }
	} else {
	  gl_destroy(result);
	  *errval = unmade_instance;
	  return NULL;
	}
      } else {
	gl_destroy(result);
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
    for(c1=1; c1<=len1; c1++){
      i = (struct Instance *)gl_fetch(list,c1);
      if (InstanceKind(i)==ARRAY_INT_INST){
	if (NextToExpand(i)!=1){
	  for (c2=1; c2<=len2; c2++){
	    SetInstanceNameIntIndex(rec,FetchIntMember(sptr,c2));
	    if ((pos = ChildSearch(i,&rec))==0){
	      gl_destroy(result);
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
		gl_append_ptr(result,(VOIDPTR)child);
	      } else{
		gl_destroy(result);
		*errval = unmade_instance;
		return NULL;
	      }
	    }
	  }
	} else {
	  gl_destroy(result);
	  *errval = unmade_instance;
	  return NULL;
	}
      } else {
	gl_destroy(result);
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
struct gl_list_t *FindNextNameElement(CONST struct Name *n,
				      struct gl_list_t *list,
				      enum find_errors *errval)
{
  unsigned long pos,c,len;
  struct InstanceName rec;
  struct Instance *current,*child;
  struct value_t setvalue,oldvalue;
  CONST struct Set *sptr;
  struct gl_list_t *result;

  *errval = correct_instance;
  if (NameId(n)){
    result = gl_create(NAMELISTSIZE);
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,NameIdPtr(n));
    len = gl_length(list);
    for(c=1; c<=len; c++){
      current = (struct Instance *)gl_fetch(list,c);
      pos = ChildSearch(current,&rec);
      if (pos!=0){
	child = InstanceChild(current,pos);
	if (child!=NULL){
	  gl_append_ptr(result,(VOIDPTR)child);
	} else{
	  *errval = unmade_instance;
	  gl_destroy(result);
	  return NULL;
	}
      } else{
	*errval = unmade_instance;
        /* it would seem this ought to be undefined_instance,
         * but maybe refinement causes insanity. -- in which case
         * it should be a caller policy to wait, rather than our
         * job to anticipate policy and short circuit things here.
         */
	gl_destroy(result);
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
      result = FindArrayChildren(list,SetValue(setvalue),errval);
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
      Asc_Panic(2, NULL, "Need to add to FindNextNameElement.\n");
      exit(2);/* Needed to keep gcc from whining */
    }
  }
}

static
struct gl_list_t *RealFindInstances(CONST struct Instance *i,
				    CONST struct Name *n,
				    enum find_errors *errval)
{
  struct gl_list_t *result,*next;
  result = gl_create(NAMELISTSIZE);
  gl_append_ptr(result,(VOIDPTR)i);
  while(n!=NULL){
    next = FindNextNameElement(n,result,errval);
    gl_destroy(result);
    if (next!=NULL){
      result = next;
      n = NextName(n);
    } else {
      return NULL;
    }
  }
  return result;
}

struct gl_list_t *FindInstances(CONST struct Instance *i,
				CONST struct Name *n,
				enum find_errors *errval)
{
  struct gl_list_t *result;
  *errval = impossible_instance;
  if (i == NULL) return NULL;
  AssertMemory(i);
  assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(i);
  *errval = correct_instance;
  result = RealFindInstances(i,n,errval);
  SetEvaluationContext(NULL);
  return result;
}
