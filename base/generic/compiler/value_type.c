/*
 *  Value Type Implementation
 *  by Tom Epperly
 *  Created: 1/16/90
 *  Version: $Revision: 1.20 $
 *  Version control file: $RCSfile: value_type.c,v $
 *  Date last modified: $Date: 1998/02/05 22:23:36 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
#include <math.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/pool.h>
#ifndef FIRSTCHOICE 
#include <general/hashpjw.h>
#endif
#include "compiler.h"
#include "symtab.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "func.h"
#include "setinstval.h"
/* stuf for ListMode */
#include "types.h"
#include "find.h"
#include "value_type.h"

#ifndef lint
static CONST char ValueTypeRCSid[] = "$Id: value_type.c,v 1.20 1998/02/05 22:23:36 ballan Exp $";
#endif

static pool_store_t g_value_pool = NULL;
/* A pool_store for value_t elements.
 * If you can figure out when to reset it, do so.
 */

void ValInit(struct value_t *v)
{
  memset((char *)v,0,sizeof(struct value_t));
}
/*
 * IVAL(stackvar) should be called on locally allocated
 * value_t before any other action using them is taken.
 * When NDEBUG is not defined, it causes the stack memory to be
 * initialized to 0. Normally it is a do nothing macro.
 * Proper initialization helps us separate signal from noise in
 * gdb and purify.
 */

#define POOL_ALLOCVALUE ((struct value_t *)(pool_get_element(g_value_pool)))
/* get a value */
#define POOL_FREEVALUE(p) (pool_free_element(g_value_pool,(void *)(p)))
/* return a value_t for reuse */
#define POOL_RESET pool_clear_store(g_value_pool)
/* reset the pool for next time. only safe after all objects,types killed. */

#define VP_LEN 2
#if (SIZEOF_VOID_P == 8)
#define VP_WID 41
#else
#define VP_WID 42
#endif
/* retune vpwid if the size of value_t changes dramatically */
#define VP_ELT_SIZE (sizeof(struct value_t))
#define VP_MORE_ELTS 1
/* Number of slots filled if more elements needed.
   So if the pool grows, it grows by VP_MORE_ELTS*VP_WID elements at a time. */
#define VP_MORE_BARS 50
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown.
   One could also recall these every time there is a delete all types. */
void InitValueManager(void) {
  if (g_value_pool != NULL ) {
    Asc_Panic(2, NULL, "ERROR: InitValueManager called twice.\n");
  }
  g_value_pool =
    pool_create_store(VP_LEN, VP_WID, VP_ELT_SIZE, VP_MORE_ELTS, VP_MORE_BARS);
  if (g_value_pool == NULL) {
    Asc_Panic(2, NULL, "ERROR: InitValueManager unable to allocate pool.\n");
  }
}

void DestroyValueManager(void) {
  if (g_value_pool==NULL) return;
  pool_destroy_store(g_value_pool);
  g_value_pool = NULL;
}

void ReportValueManager(FILE *f)
{
  if (g_value_pool==NULL)
    FPRINTF(f,"ValueManager is empty\n");
  FPRINTF(f,"ValueManager ");
  pool_print_store(f,g_value_pool,0);
}

struct value_t CreateRealValue(double value, CONST dim_type *dim,unsigned cv)
{
  struct value_t result;
  IVAL(result);
  result.t = real_value;
  result.constant = cv ? 1:0;
  result.u.r.value = value;
  result.u.r.dimp = dim;
  return result;
}

struct value_t CreateIntegerValue(long int value, unsigned cv)
{
  struct value_t result;
  IVAL(result);
  result.t = integer_value;
  result.constant = cv ? 1:0;
  result.u.i = value;
  return result;
}

struct value_t CreateSymbolValue(symchar *sym_ptr, unsigned cv)
{
  struct value_t result;
  IVAL(result);
  assert(sym_ptr == NULL || AscFindSymbol(sym_ptr)!=NULL);
  result.t = symbol_value;
  result.constant = cv ? 1:0;
  result.u.sym_ptr = sym_ptr;
  return result;
}

struct value_t CreateBooleanValue(int truth, unsigned cv)
{
  struct value_t result;
  IVAL(result);
  result.t = boolean_value;
  result.constant = cv;
  result.u.b = truth ? 1 : 0;
  return result;
}

struct value_t CreateSetValue(struct set_t *sptr)
{
  struct value_t result;
  IVAL(result);
  result.t = set_value;
  result.constant = 1;
  result.u.sptr = sptr;
  return result;
}

struct value_t CreateSetFromList(struct value_t value)
{
  struct value_t result;
  register struct value_t *vptr;
  register struct set_t *sptr,*tmp;
  register struct gl_list_t *list;
  register unsigned long c,len;
  IVAL(result);
  if (value.t == error_value) return value;
  result.t = set_value;
  result.constant = 1;
  if (value.t == list_value){
    sptr = CreateEmptySet();
    result.u.sptr = sptr;
    list = value.u.lvalues;
    if (gl_length(list)>0){
      len = gl_length(list);
      for(c=1;c<=len;c++){
	vptr = (struct value_t *)gl_fetch(list,c);
	switch(vptr->t){
	case integer_value:
	  if (SetKind(sptr)==string_set){
	    result.t = error_value;
	    result.u.t = type_conflict;
	    DestroySet(sptr);
	    return result;
	  }
	  InsertInteger(sptr,vptr->u.i);
	  break;
	case symbol_value:
	  if (SetKind(sptr)==integer_set){
	    result.t = error_value;
	    result.u.t = type_conflict;
	    DestroySet(sptr);
	    return result;
	  }
	  InsertString(sptr,vptr->u.sym_ptr);
	  break;
	case set_value:
	  if ((SetKind(sptr)==empty_set)
	      ||(SetKind(sptr)==SetKind(vptr->u.sptr))){
	    tmp = SetUnion(sptr,vptr->u.sptr);
	    DestroySet(sptr);
	    sptr = tmp;
	    result.u.sptr = sptr;
	  }
	  else{
	    DestroySet(sptr);
	    result.t = error_value;
	    result.u.t = type_conflict;
	    return result;
	  }
	  break;
	default:
	  result.t = error_value;
	  result.u.t = type_conflict;
	  DestroySet(sptr);
	  return result;
	}
      }
    }
  }
  else if (value.t == integer_value){
    sptr = CreateEmptySet();
    InsertInteger(sptr,value.u.i);
    result.u.sptr = sptr;
  }
  else if (value.t == symbol_value){
    sptr = CreateEmptySet();
    InsertString(sptr,value.u.sym_ptr);
    result.u.sptr = sptr;
  }
  else if (value.t == set_value){
    result.u.sptr = CopySet(value.u.sptr);
  }
  else{ /* error */
    result.t = error_value;
    result.u.t = type_conflict;
  }
  return result;
}

struct value_t CreateOrderedSetFromList(struct value_t value)
{
  struct value_t result;
  register struct value_t *vptr;
  register struct set_t *sptr;
  struct set_t *sval;
  register struct gl_list_t *list;
  register unsigned long c,len, sc,slen;
  IVAL(result);

  assert(ListMode!=0);
  if (value.t == error_value) {
    return value;
  }
  result.t = set_value;
  result.constant = 0;
  if (value.t == list_value){
    sptr = CreateEmptySet();
    result.u.sptr = sptr;
    list = value.u.lvalues;
    if (list) {
      len = gl_length(list);
      for(c=1;c<=len;c++){
	vptr = (struct value_t *)gl_fetch(list,c); /* a list of value_t's */
	switch(vptr->t){
	case integer_value:
	  if (SetKind(sptr)==string_set){
	    result.t = error_value;
	    result.u.t = type_conflict;
	    DestroySet(sptr);
	    return result;
	  }
	  AppendIntegerElement(sptr,vptr->u.i);
	  break;
	case symbol_value:
	  if (SetKind(sptr)==integer_set){
	    result.t = error_value;
	    result.u.t = type_conflict;
	    DestroySet(sptr);
	    return result;
	  }
	  AppendStringElement(sptr,vptr->u.sym_ptr);
	  break;
	case set_value:
          sval = SetValue(*vptr);
	  if (SetKind(sval) == SetKind(sptr) ||  SetKind(sptr) == empty_set) {
            if (SetKind(sval) == integer_set) {
              for (sc = 1, slen = Cardinality(sval); sc <= slen; sc++ ) {
	        AppendIntegerElement(sptr,FetchIntMember(sval,sc));
	      }
	    } else {
              for (sc = 1, slen = Cardinality(sval); sc <= slen; sc++ ) {
	        AppendStringElement(sptr,FetchStrMember(sval,sc));
	      }
            }
	    break;
          }
          /* fall through: set type mismatch */
	default:
	  result.t = error_value;
	  result.u.t = type_conflict;
	  DestroySet(sptr);
	  return result;
	}
      }
    }
  }
  else if (value.t == integer_value){
    sptr = CreateEmptySet();
    AppendIntegerElement(sptr,value.u.i);
    result.u.sptr = sptr;
  }
  else if (value.t == symbol_value){
    sptr = CreateEmptySet();
    AppendStringElement(sptr,value.u.sym_ptr);
    result.u.sptr = sptr;
  }
  else if (value.t == set_value){/* error */
    FPRINTF(ASCERR,"Set values are not allowed (2) in LIST processing\n");
    FPRINTF(ASCERR,"Please report error in CreateOrderedSetFromList\n");
    result.t = error_value;
    result.u.t = type_conflict;
  }
  else{ /* error */
    result.t = error_value;
    result.u.t = type_conflict;
  }
  return result;
}


static
struct value_t CreateListCopy(struct gl_list_t *l)
{
  struct value_t result,*ptr,*cpy;
  unsigned long c,len;
  IVAL(result);
  result.t = list_value;
  result.constant = 0;
  len = gl_length(l);
  result.u.lvalues = gl_create(len);
  for(c=1;c<=len;c++){
    ptr = (struct value_t *)gl_fetch(l,c);
    cpy = POOL_ALLOCVALUE;
    IVALPTR(cpy);
    *cpy = CopyValue(*ptr);
    gl_append_ptr(result.u.lvalues,(VOIDPTR)cpy);
  }
  return result;
}

struct value_t CopyValue(struct value_t value)
{
  switch(value.t){
  case set_value: return CreateSetValue(CopySet(value.u.sptr));
  case list_value: return CreateListCopy(value.u.lvalues);
  default: return value;
  }
}

struct value_t CreateErrorValue(enum evaluation_error t)
{
  struct value_t result;
  IVAL(result);
  result.t = error_value;
  result.constant = 0;
  result.u.t = t;
  return result;
}

struct value_t CreateVacantListValue(void)
{
  struct value_t result;
  IVAL(result);
  result.t = list_value;
  result.constant = 0;
  result.u.lvalues = gl_create(2L);
  return result;
}

struct value_t CreateEmptyListValue(void)
{
  struct value_t result;
  IVAL(result);
  result.t = list_value;
  result.constant = 0;
  result.u.lvalues = gl_create(7L);
  return result;
}

void AppendToListValue(struct value_t list, struct value_t value)
{
  assert(list.t == list_value);
  if (value.t == list_value){
    gl_append_list(list.u.lvalues,value.u.lvalues);
    gl_destroy(value.u.lvalues);
    return;
  }
  else {
    struct value_t *ptr;
    ptr = POOL_ALLOCVALUE;
    IVALPTR(ptr);
    *ptr = value;
    gl_append_ptr(list.u.lvalues,(VOIDPTR)ptr);
  }
}

/* Do not export this function. Applications needn't be creating
 * independently allocated values. 3/96 baa. Some idiot will call
 * it with a ptr to C stack space.
 * Recursive.
 */
static void DestroyAndFreeValue(struct value_t *value)
{
  switch (value->t){
  case set_value:
    if (value->u.sptr!=NULL) DestroySet(value->u.sptr);
    break;
  case list_value:
    if (value->u.lvalues!=NULL){
      gl_iterate(value->u.lvalues,(void (*)(VOIDPTR))DestroyAndFreeValue);
      gl_destroy(value->u.lvalues);
    }
    break;
  default:
    break; /* no action required */
  }
  /* value->t = error_value;  no point, going in recycle who stomps it. */
  POOL_FREEVALUE(value);
}

/* see header for this function ! */
void DestroyValue(struct value_t *value)
{
  switch (value->t){
  case set_value:
    if (value->u.sptr!=NULL) DestroySet(value->u.sptr);
    value->u.sptr = NULL;
    break;
  case list_value:
    if (value->u.lvalues!=NULL){
      gl_iterate(value->u.lvalues,(void (*)(VOIDPTR))DestroyAndFreeValue);
      gl_destroy(value->u.lvalues);
    }
    value->u.lvalues=NULL;
    break;
  default: break; /* no action required */
  }
  value->t = error_value;
  /* Free not required */
}

static
struct value_t DimensionConflict(void)
{
  struct value_t result;
  IVAL(result);
  result.t = error_value;
  result.constant = 0;
  result.u.t = dimension_conflict;
  return result;
}

static
struct value_t EmptyIntersection(void)
{
  struct value_t result;
  IVAL(result);
  result.t = error_value;
  result.constant = 0;
  result.u.t = empty_intersection;
  return result;
}

static
struct value_t TypeConflict(void)
{
  struct value_t result;
  IVAL(result);
  result.t = error_value;
  result.constant = 0;
  result.u.t = type_conflict;
  return result;
}

static
int SetsOkay(struct set_t *set1, struct set_t *set2)
{
  if (SetKind(set1)==empty_set) return 1;
  if (SetKind(set2)==empty_set) return 1;
  return SetKind(set1)==SetKind(set2);
}

struct value_t AddValues(struct value_t value1, struct value_t value2)
{
  CONST dim_type *dim;
  if (value1.t == error_value) return value1;
  if (value2.t == error_value) return value2;
  if ( (value1.t == boolean_value) ||
       (value1.t == list_value) ||
       (value2.t == boolean_value) ||
       (value2.t == list_value) ) {
    return TypeConflict();
  }
  if ((value1.t == symbol_value) || (value2.t == symbol_value) ) {
#ifdef CATTEST
    /* do nothing */
#else
    return TypeConflict();
#endif
  }
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      dim = CheckDimensionsMatch(value1.u.r.dimp,value2.u.r.dimp);
      if (dim!=NULL) {
        return CreateRealValue(value1.u.r.value+value2.u.r.value,
				dim,BothConstantValue(value1,value2));
      }
      else return DimensionConflict();
    case integer_value:
      return CreateIntegerValue(value1.u.i+value2.u.i,
				BothConstantValue(value1,value2));
#ifdef CATTEST
    case symbol_value:
    {
      int slen;
      char *str;
      symchar *sym;
      if (value1.u.sym_ptr==NULL) {
        return CreateSymbolValue(value2.u.sym_ptr,
                                 BothConstantValue(value1,value2));
      }
      if (value2.u.sym_ptr==NULL) {
        return CreateSymbolValue(value1.u.sym_ptr,
                                 BothConstantValue(value1,value2));
      }
      slen = strlen(value1.u.sym_ptr) + strlen(value2.u.sym_ptr);
      str = (char *)ascmalloc(slen+1);
      if (str==NULL) {
       return TypeConflict(); /* out of memory error */
      }
      sprintf(str,"%s%s",value1.u.sym_ptr,value2.u.sym_ptr);
      sym = AddSymbolL(str,slen);
      ascfree(str);
      return CreateSymbolValue(sym, BothConstantValue(value1,value2));
    }
#endif
    case set_value:
      if (SetsOkay(value1.u.sptr,value2.u.sptr))
	return CreateSetValue(SetUnion(value1.u.sptr,value2.u.sptr));
      else return TypeConflict();
    default: /*NOTREACHED*/
      break;
    }
    /*NOTREACHED*/
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      dim = CheckDimensionsMatch(Dimensionless(),value2.u.r.dimp);
      if (dim!=NULL)
        return CreateRealValue((double)value1.u.i+value2.u.r.value,dim,
				BothConstantValue(value1,value2));
      else return DimensionConflict();
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      dim = CheckDimensionsMatch(Dimensionless(),value1.u.r.dimp);
      if (dim!=NULL)
        return CreateRealValue(value1.u.r.value+(double)value2.u.i,dim,
				BothConstantValue(value1,value2));
      else return DimensionConflict();
    }
    return TypeConflict();
  }
  /*NOTREACHED*/
  return TypeConflict();
}

struct value_t SubtractValues(struct value_t value1, struct value_t value2)
{
  CONST dim_type *dim;
  if (value1.t == error_value) return value1;
  if (value2.t == error_value) return value2;
  if ((value1.t == symbol_value)
      ||(value1.t == boolean_value)
      ||(value1.t == list_value)||
        (value2.t == symbol_value)
      ||(value2.t == boolean_value)
      ||(value2.t == list_value))
    return TypeConflict();
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      dim = CheckDimensionsMatch(value1.u.r.dimp,value2.u.r.dimp);
      if (dim!=NULL)
        return CreateRealValue(value1.u.r.value-value2.u.r.value,dim,
				BothConstantValue(value1,value2));
      else return DimensionConflict();
    case integer_value:
      return CreateIntegerValue(value1.u.i-value2.u.i,
				BothConstantValue(value1,value2));
    case set_value:
      if (SetsOkay(value1.u.sptr,value2.u.sptr))
	return CreateSetValue(SetDifference(value1.u.sptr,value2.u.sptr));
      else return TypeConflict();
    default: /*NOTREACHED*/
      break;
    }
    /*NOTREACHED*/
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      dim = CheckDimensionsMatch(Dimensionless(),value2.u.r.dimp);
      if (dim!=NULL)
        return CreateRealValue((double)value1.u.i-value2.u.r.value,dim,
				BothConstantValue(value1,value2));
      else return DimensionConflict();
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      dim = CheckDimensionsMatch(Dimensionless(),value1.u.r.dimp);
      if (dim!=NULL)
        return CreateRealValue(value1.u.r.value-(double)value2.u.i,dim,
				BothConstantValue(value1,value2));
      else return DimensionConflict();
    }
    return TypeConflict();
  }
  /*NOTREACHED*/
  return TypeConflict();
}

struct value_t MultiplyValues(struct value_t value1, struct value_t value2)
{
  dim_type dim;
  if (value1.t == error_value) return value1;
  if (value2.t == error_value) return value2;
  if ((value1.t == symbol_value)||
      (value1.t == boolean_value)||
      (value1.t == list_value)||
      (value2.t == symbol_value)||
      (value2.t == boolean_value)||
      (value2.t == list_value))
    return TypeConflict();
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      dim = AddDimensions(value1.u.r.dimp,value2.u.r.dimp);
      return CreateRealValue(value1.u.r.value*value2.u.r.value,
		FindOrAddDimen(&dim),BothConstantValue(value1,value2));
    case integer_value:
      return CreateIntegerValue(value1.u.i*value2.u.i,
				BothConstantValue(value1,value2));
    case set_value:
      if (SetsOkay(value1.u.sptr,value2.u.sptr))
	return CreateSetValue(SetIntersection(value1.u.sptr,value2.u.sptr));
      else return TypeConflict();
    default: /*NOTREACHED*/
      break;
    }
    /*NOTREACHED*/
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value))
      return CreateRealValue((double)value1.u.i*value2.u.r.value,
                             value2.u.r.dimp,
                             BothConstantValue(value1,value2));
    if((value1.t==real_value)&&(value2.t==integer_value))
      return CreateRealValue(value1.u.r.value*(double)value2.u.i,
			value1.u.r.dimp,BothConstantValue(value1,value2));
    return TypeConflict();
  }
  /*NOTREACHED*/
  return TypeConflict();
}

struct value_t DivideValues(struct value_t value1, struct value_t value2)
{
  dim_type dim;
  if (value1.t == error_value) return value1;
  if (value2.t == error_value) return value2;
  if ((value1.t == symbol_value)||
      (value1.t == boolean_value)||
      (value1.t == list_value)||
      (value2.t == symbol_value)||
      (value2.t == boolean_value)||
      (value2.t == list_value)||
      (value1.t == set_value)||
      (value2.t == set_value))
    return TypeConflict();
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      dim = SubDimensions(value1.u.r.dimp,value2.u.r.dimp);
      return CreateRealValue(value1.u.r.value/value2.u.r.value,
        FindOrAddDimen(&dim),BothConstantValue(value1,value2));
    case integer_value:
      return CreateIntegerValue(value1.u.i/value2.u.i,
				BothConstantValue(value1,value2));
    default: /*NOTREACHED*/
      break;
    }
    /*NOTREACHED*/
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      dim = SubDimensions(Dimensionless(),value2.u.r.dimp);
      return CreateRealValue((double)value1.u.i/value2.u.r.value,
        FindOrAddDimen(&dim),BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value))
      return CreateRealValue(value1.u.r.value/(double)value2.u.i,
        value1.u.r.dimp,BothConstantValue(value1,value2));
    return TypeConflict();
  }
  /*NOTREACHED*/
  return TypeConflict();
}

/* integer x^y function */
static
long ipower(long int x, long int y)
{
  register long result=1;
  if (y==0) return result;
  if (y>0) {
    while(y-->0)
      result *= x;
  } else {
    switch (x) {
    case 1:
      result=1;
      break;
    case -1:
      result=y%2;
      if (result) result=-1;
      else result=1;
      break;
    default:
      result=0;
      break;
    }
  }
  return result;
}

struct value_t PowerValues(struct value_t value1, struct value_t value2)
{
  dim_type dim;
  if (value1.t == error_value) return value1;
  if (value2.t == error_value) return value2;
  if ((value1.t == symbol_value)||
      (value1.t == boolean_value)||
      (value1.t == list_value)||
      (value2.t == symbol_value)||
      (value2.t == boolean_value)||
      (value2.t == list_value)||
      (value1.t == set_value)||
      (value2.t == set_value))
    return TypeConflict();
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      if ((CheckDimensionsMatch(Dimensionless(),value1.u.r.dimp)==NULL)||
	  (CheckDimensionsMatch(Dimensionless(),value2.u.r.dimp)==NULL))
	return DimensionConflict();
      return CreateRealValue(pow(value1.u.r.value,value2.u.r.value),
        Dimensionless(),BothConstantValue(value1,value2));
    case integer_value:
      return CreateIntegerValue(ipower(value1.u.i,value2.u.i),
				BothConstantValue(value1,value2));
    default: /*NOTREACHED*/
      break;
    }
    /*NOTREACHED*/
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      if (CheckDimensionsMatch(Dimensionless(),value2.u.r.dimp)==NULL)
	return DimensionConflict();
      return CreateRealValue(pow((double)value1.u.i,value2.u.r.value),
        Dimensionless(),BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      dim = ScaleDimensions(value1.u.r.dimp,
                            CreateFraction((short)value2.u.i,1));
      return CreateRealValue(pow(value1.u.r.value,(double)value2.u.i),
		FindOrAddDimen(&dim),BothConstantValue(value1,value2));
    }
    return TypeConflict();
  }
  return TypeConflict();
}

struct value_t CardValues(struct value_t value)
{
  if (value.t==error_value) return value;
  if (value.t==set_value){
    return CreateIntegerValue((long)Cardinality(value.u.sptr),1);
  }
  else return TypeConflict();
}

#ifndef FIRSTCHOICE 
/*
 * next two functions used only if firstchoice not defined
 */
static
int IntegerSetHash(CONST struct set_t *sptr)
/* Styled after hashpjw */
/* this should probably be revisited */
{
  unsigned long c;
  int sum=0,tsum;
  CONST unsigned long card = (unsigned long)Cardinality(sptr);
  assert(SetKind(sptr) == integer_set);
  for(c = 1L;c <= card;c++){
    sum = (sum << 4) + FetchIntMember(sptr,c);
    if ( (tsum = sum & 0xf0000000) ){
      sum = sum ^ (tsum >> 24);
      sum = sum ^ tsum;
    }
  }
  return sum;
}

static
int StringSetHash(CONST struct set_t *sptr)
{
  int c,sum=0;
  CONST int card = Cardinality(sptr);
  assert(SetKind(sptr) == string_set);
  for(c=1;c <= card;c++){
    sum += hashpjw(FetchStrMember(sptr,c),ULONG_MAX);
  }
  return sum;
}
#endif /* firstchoice */


struct value_t ChoiceValues(struct value_t value)
/* This is where the method of evaluating CHOICE is defined.  It is */
/* designed to return an arbitrary yet consistent value */
{
  int choice, card;
  if (value.t==error_value) return value;
  if (value.t==set_value){
    card = Cardinality(value.u.sptr);
    if (card == 0)
      return CreateErrorValue(empty_choice);
    switch(SetKind(value.u.sptr)){
    case integer_set:
      if (card == 1) {
        choice = 1;
      } else {
#ifdef FIRSTCHOICE
        choice = 1;
#else
	choice = (IntegerSetHash(value.u.sptr) % card) + 1;
#endif
      }
      return CreateIntegerValue(FetchIntMember(value.u.sptr,choice),1);
    case string_set:
      if (card == 1) {
        choice = 1;
      } else {
#ifdef FIRSTCHOICE
        choice = 1;
#else
	choice = (StringSetHash(value.u.sptr) % card) + 1;
#endif
      }
      return CreateSymbolValue(FetchStrMember(value.u.sptr,choice),1);
    default:
      return CreateErrorValue(empty_choice);
    }
  }
  else return TypeConflict();
}

static
struct value_t SumList(struct gl_list_t *l)
{
  struct value_t result,*ptr;
  unsigned long c,len;
  IVAL(result);
  len = gl_length(l);
  if (len>0){
    ptr = (struct value_t *)gl_fetch(l,1);
    result = *ptr;
    if ((result.t!=integer_value)&&(result.t!=real_value))
       return TypeConflict();
    for(c=2;(c<=len)&&(result.t!=error_value);c++){
      ptr = (struct value_t *)gl_fetch(l,c);
      if ((ptr->t!=integer_value)&&(ptr->t!=real_value)) {
        return TypeConflict();
      }
      result = AddValues(result,*ptr);
    }
    return result;
  }
  else return CreateIntegerValue(0L,1); /* sum empty = constant 0 */
}

struct value_t SumValues(struct value_t value)
{
  switch(value.t){
  case real_value: return value;
  case integer_value: return value;
  case list_value:
    return SumList(value.u.lvalues);
  case error_value:
    return value;
  default:
    return TypeConflict();
  }
}

static
struct value_t MultiplyList(struct gl_list_t *l)
{
  struct value_t result,*ptr;
  unsigned long c,len;
  IVAL(result);
  len = gl_length(l);
  if (len>0){
    ptr = (struct value_t *)gl_fetch(l,1);
    result = *ptr;
    if ((result.t!=integer_value)&&(result.t!=real_value))
      return TypeConflict();
    for(c=2;(c<=len)&&(result.t!=error_value);c++){
      ptr = (struct value_t *)gl_fetch(l,c);
      if ((ptr->t!=integer_value)&&(ptr->t!=real_value)) {
        return TypeConflict();
      }
      result = MultiplyValues(result,*ptr);
    }
    return result;
  }
  else return CreateIntegerValue(1L,1);
}

struct value_t ProdValues(struct value_t value)
{
  switch(value.t){
  case real_value: return value;
  case integer_value: return value;
  case list_value:
    return MultiplyList(value.u.lvalues);
  case error_value:
    return value;
  default:
    return TypeConflict();
  }
}

static
struct value_t UnionList(struct gl_list_t *l)
{
  struct value_t result,*ptr,old;
  unsigned long c,len;
  IVAL(result);
  len = gl_length(l);
  if(len>0){
    ptr = (struct value_t *)gl_fetch(l,1);
    if (ptr->t!=set_value) return TypeConflict();
    result = CreateSetValue(CopySet(ptr->u.sptr));
    for(c=2;(c<=len)&&(result.t!=error_value);c++){
      old = result;
      ptr = (struct value_t *)gl_fetch(l,c);
      if (ptr->t!=set_value){
	DestroyValue(&result);
	return TypeConflict();
      }
      result = AddValues(result,*ptr);
      DestroyValue(&old);
    }
    return result;
  }
  else return CreateSetValue(CreateEmptySet());
}

struct value_t UnionValues(struct value_t value)
{
  switch(value.t){
  case error_value: return value;
  case set_value:
    return CreateSetValue(CopySet(value.u.sptr));
  case list_value:
    return UnionList(value.u.lvalues);
  default:
    return TypeConflict();
  }
}

static
struct value_t IntersectList(struct gl_list_t *l)
{
  struct value_t result,*ptr,old;
  unsigned long c,len;
  IVAL(result);
  len = gl_length(l);
  if(len>0){
    ptr = (struct value_t *)gl_fetch(l,1);
    if (ptr->t!=set_value) return TypeConflict();
    result = CreateSetValue(CopySet(ptr->u.sptr));
    for(c=2;(c<=len)&&(result.t!=error_value);c++){
      old = result;
      ptr = (struct value_t *)gl_fetch(l,c);
      if (ptr->t!=set_value){
	DestroyValue(&result);
	return TypeConflict();
      }
      result = MultiplyValues(result,*ptr);
      DestroyValue(&old);
    }
    return result;
  }
  else return EmptyIntersection();
}

struct value_t IntersectionValues(struct value_t value)
{
  switch(value.t){
  case error_value: return value;
  case set_value:
    return CreateSetValue(CopySet(value.u.sptr));
  case list_value:
    return IntersectList(value.u.lvalues);
  default:
    return TypeConflict();
  }
}

struct value_t OrValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if ((value1.t!=boolean_value)||(value2.t!=boolean_value))
    return TypeConflict();
  return CreateBooleanValue(value1.u.b||value2.u.b,
				BothConstantValue(value1,value2));
}

struct value_t AndValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if ((value1.t!=boolean_value)||(value2.t!=boolean_value))
    return TypeConflict();
  return CreateBooleanValue(value1.u.b&&value2.u.b,
				BothConstantValue(value1,value2));
}

struct value_t InValues(struct value_t value1, struct value_t value2)
{
  if (value2.t == error_value) return value2;
  if (value2.t!= set_value) return TypeConflict();
  switch(value1.t){
  case error_value: return value1;
  case integer_value:
    if (SetKind(value2.u.sptr)==string_set) return TypeConflict();
    return CreateBooleanValue(IntMember(value1.u.i,value2.u.sptr),
                              BothConstantValue(value1,value2));
  case symbol_value:
    if (SetKind(value2.u.sptr)==integer_set) return TypeConflict();
    return CreateBooleanValue(StrMember(value1.u.sym_ptr,value2.u.sptr),
                              BothConstantValue(value1,value2));
  default:
    return TypeConflict();
  }
}

/* checks for identical values and compatible dimens. wild is compatible
 * with anything.
 */
static
int EqualReals(struct value_t *value1, struct value_t *value2)
{
 return (value1->u.r.value == value2->u.r.value &&
         CheckDimensionsMatch(value1->u.r.dimp,value2->u.r.dimp)!=NULL);
}

struct value_t EqualValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      return CreateBooleanValue(EqualReals(&value1,&value2),
				BothConstantValue(value1,value2));
    case set_value:
      return CreateBooleanValue(SetsEqual(value1.u.sptr,value2.u.sptr),
				BothConstantValue(value1,value2));
    case integer_value:
      return CreateBooleanValue(value1.u.i==value2.u.i,
				BothConstantValue(value1,value2));
    case symbol_value:
      return CreateBooleanValue((value1.u.sym_ptr == value2.u.sym_ptr),
				BothConstantValue(value1,value2));
    case boolean_value:
      return CreateBooleanValue(value1.u.b==value2.u.b,
				BothConstantValue(value1,value2));
    default:
      return TypeConflict();
    }
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      if (CheckDimensionsMatch(value2.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue((double)value1.u.i==value2.u.r.value,
					BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      if (CheckDimensionsMatch(value1.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue(value1.u.r.value==(double)value2.u.i,
					BothConstantValue(value1,value2));
    }
    return TypeConflict();
  }
}

struct value_t NotEqualValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      return CreateBooleanValue(value1.u.r.value!=value2.u.r.value,
				BothConstantValue(value1,value2));
    case integer_value:
      return CreateBooleanValue(value1.u.i!=value2.u.i,
				BothConstantValue(value1,value2));
    case symbol_value:
      return CreateBooleanValue((value1.u.sym_ptr != value2.u.sym_ptr),
				BothConstantValue(value1,value2));
    default:
      return TypeConflict();
    }
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      if (CheckDimensionsMatch(value2.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue((double)value1.u.i!=value2.u.r.value,
					BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      if (CheckDimensionsMatch(value1.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue(value1.u.r.value!=(double)value2.u.i,
					BothConstantValue(value1,value2));
    }
    return TypeConflict();
  }
}

struct value_t LessValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      return CreateBooleanValue(value1.u.r.value<value2.u.r.value,
				BothConstantValue(value1,value2));
    case integer_value:
      return CreateBooleanValue(value1.u.i<value2.u.i,
				BothConstantValue(value1,value2));
    case symbol_value:
      return CreateBooleanValue(CmpSymchar(value1.u.sym_ptr,value2.u.sym_ptr)<0,
				BothConstantValue(value1,value2));
    default:
      return TypeConflict();
    }
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      if (CheckDimensionsMatch(value2.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue((double)value1.u.i<value2.u.r.value,
					BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      if (CheckDimensionsMatch(value1.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue(value1.u.r.value<(double)value2.u.i,
					BothConstantValue(value1,value2));
    }
    return TypeConflict();
  }
}

struct value_t LessEqValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      return CreateBooleanValue(value1.u.r.value<=value2.u.r.value,
				BothConstantValue(value1,value2));
    case integer_value:
      return CreateBooleanValue(value1.u.i<=value2.u.i,
				BothConstantValue(value1,value2));
    case symbol_value:
      return CreateBooleanValue(CmpSymchar(value1.u.sym_ptr,
                                           value2.u.sym_ptr) <= 0,
				BothConstantValue(value1,value2));
    default:
      return TypeConflict();
    }
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      if (CheckDimensionsMatch(value2.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue((double)value1.u.i<=value2.u.r.value,
				BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      if (CheckDimensionsMatch(value1.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue(value1.u.r.value<=(double)value2.u.i,
				BothConstantValue(value1,value2));
    }
    return TypeConflict();
  }
}

struct value_t GreaterValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      return CreateBooleanValue(value1.u.r.value>value2.u.r.value,
				BothConstantValue(value1,value2));
    case integer_value:
      return CreateBooleanValue(value1.u.i>value2.u.i,
				BothConstantValue(value1,value2));
    case symbol_value:
      return CreateBooleanValue(CmpSymchar(value1.u.sym_ptr,value2.u.sym_ptr)>0,
				BothConstantValue(value1,value2));
    default:
      return TypeConflict();
    }
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      if (CheckDimensionsMatch(value2.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue((double)value1.u.i>value2.u.r.value,
				BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      if (CheckDimensionsMatch(value1.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue(value1.u.r.value>(double)value2.u.i,
				BothConstantValue(value1,value2));
    }
    return TypeConflict();
  }
}


struct value_t GreaterEqValues(struct value_t value1, struct value_t value2)
{
  if (value1.t==error_value) return value1;
  if (value2.t==error_value) return value2;
  if (value1.t==value2.t){
    switch(value1.t){
    case real_value:
      return CreateBooleanValue(value1.u.r.value>=value2.u.r.value,
				BothConstantValue(value1,value2));
    case integer_value:
      return CreateBooleanValue(value1.u.i>=value2.u.i,
				BothConstantValue(value1,value2));
    case symbol_value:
      return CreateBooleanValue(CmpSymchar(value1.u.sym_ptr,
                                           value2.u.sym_ptr) >= 0,
				BothConstantValue(value1,value2));
    default:
      return TypeConflict();
    }
  }
  else{
    if((value1.t==integer_value)&&(value2.t==real_value)){
      if (CheckDimensionsMatch(value2.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue((double)value1.u.i>=value2.u.r.value,
				BothConstantValue(value1,value2));
    }
    if((value1.t==real_value)&&(value2.t==integer_value)){
      if (CheckDimensionsMatch(value1.u.r.dimp,Dimensionless())==NULL)
	return DimensionConflict();
      else
	return CreateBooleanValue(value1.u.r.value>=(double)value2.u.i,
				BothConstantValue(value1,value2));
    }
    return TypeConflict();
  }
}

struct value_t NegateValue(struct value_t value)
{
  switch(value.t){
  case error_value: return value;
  case integer_value: return CreateIntegerValue(-value.u.i,
						IsConstantValue(value));
  case real_value: return CreateRealValue(-value.u.r.value,value.u.r.dimp,
						IsConstantValue(value));
  default:
    return TypeConflict();
  }
}

struct value_t NotValue(struct value_t value)
{
  switch(value.t){
  case error_value: return value;
  case boolean_value: return CreateBooleanValue(!value.u.b,
						IsConstantValue(value));
  default:
    return TypeConflict();
  }
}

/* original. delete when smoke clears.
struct value_t ApplyFunction(struct value_t value, CONST struct Func *f)
{
  CONST dim_type *dim;
  switch(value.t){
  case error_value: return value;
  case integer_value:
    return CreateRealValue(FuncEval(f,(double)IntegerValue(value))
			   ,Dimensionless(),IsConstantValue(value));
  case real_value:
    dim = CheckDimensionsMatch(Dimensionless(),value.u.r.dimp);
    if (dim==NULL)
      return DimensionConflict();
    else
      return CreateRealValue(FuncEval(f,RealValue(value)),Dimensionless(),
				IsConstantValue(value));
  default:
    return TypeConflict();
  }
}
*/
struct value_t ApplyFunction(struct value_t value, CONST struct Func *f)
{
  CONST dim_type *dim;
  dim_type sdim;
  switch(value.t){
  case error_value: return value;
  case integer_value:
    return CreateRealValue(FuncEval(f,(double)IntegerValue(value)),
                           Dimensionless(),IsConstantValue(value));
  case real_value:
    dim = CheckDimensionsMatch(FuncDimens(f),value.u.r.dimp);
    if (dim==NULL)
      return DimensionConflict();
    else
      switch (FuncId(f)) {
        case F_LOG10:
        case F_LN:
        case F_EXP:
#ifdef HAVE_ERF
		case F_ERF:
#endif
        case F_LNM:
        case F_SIN:
        case F_COS:
        case F_TAN:
        case F_SINH:
        case F_COSH:
        case F_TANH:
        case F_ARCSINH:
        case F_ARCCOSH:
        case F_ARCTANH:
          return CreateRealValue(FuncEval(f,RealValue(value)),
                                 Dimensionless(),IsConstantValue(value));
        case F_SQRT:
          if (OddDimension(dim))
            return DimensionConflict();
          else {
            CONST dim_type *dp=HalfDimension(dim,FALSE);
            return CreateRealValue(FuncEval(f,RealValue(value)), dp,
					IsConstantValue(value));
          }
        case F_CBRT:
          if (NonCubicDimension(dim))
            return DimensionConflict();
          else {
            CONST dim_type *dp=ThirdDimension(dim,FALSE);
            return CreateRealValue(FuncEval(f,RealValue(value)), dp,
					IsConstantValue(value));
          }
        case F_ARCSIN:
        case F_ARCCOS:
        case F_ARCTAN:
          return CreateRealValue(FuncEval(f,RealValue(value)),
                                 TrigDimension(),IsConstantValue(value));
        case F_SQR:
          sdim=AddDimensions(dim,dim);
          return CreateRealValue(FuncEval(f,RealValue(value)),
                                 FindOrAddDimen(&sdim),IsConstantValue(value));
        case F_ABS:
        case F_HOLD:
          return CreateRealValue(FuncEval(f,RealValue(value)),
                                 dim,IsConstantValue(value));
      default:
        return TypeConflict();
      } /* end func type */
  default:
    return TypeConflict();
  } /* end value type */
}

