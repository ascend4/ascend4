/*
 *  Ascend Atom Child Definition Module
 *  by Benjamin A Allan
 *  Created: 11/20/96
 *  Version: $Revision: 1.8 $
 *  Version control file: $RCSfile: childdef.c,v $
 *  Date last modified: $Date: 1998/03/26 20:39:36 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *
 */

#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include <utilities/ascPanic.h>
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "stattypes.h"
#include "statement.h"
#include "slist.h"
#include "statio.h"
#include "symtab.h"
#include "module.h"
#include "library.h"
#include "child.h"
#include "vlist.h"
#include "name.h"
#include "when.h"
#include "select.h"
#include "sets.h"
#include "exprs.h"
#include "forvars.h"
#include "bit.h"
#include "setinstval.h"
#include "childinfo.h"
#include "instance_enum.h"
#include "type_desc.h"
#include "type_descio.h"
#include "atomsize.h"
#include "value_type.h"
#include "evaluate.h"
#include "proc.h"
#include "childdef.h"


/* These are the allowable ATOM children types, at present.
 * the functions BaseType and GetTypeInfoFromISA rely on this table
 * and the ordering within it. This is a hack to restrict the types
 * allowed as ATOM children.
 * function BaseType uses and initializes the table.
 */
static symchar *FundamentalTypeList[NUM_FUNDTYPES];

/*
 * The code here is *VERY* dependent upon the number and position
 * of the FundamentalTypeList !
 */
int BaseType(symchar *name)
{
  static int init = 0;
  int c;
  if (!name) return -1;         /* missing OF in set declaration */
  if (!init) {
    init = 1;
    FundamentalTypeList[0] = GetBaseTypeName(boolean_type);
    FundamentalTypeList[1] = GetBaseTypeName(integer_type);
    FundamentalTypeList[2] = GetBaseTypeName(real_type);
    FundamentalTypeList[3] = GetBaseTypeName(set_type);
    FundamentalTypeList[4] = GetBaseTypeName(symbol_type);
  }
  for(c=0;c<NUM_FUNDTYPES;c++){
    if (name == FundamentalTypeList[c]) return c;
  }
  return c;
}

static
void TypeBooleanVarList(CONST struct VariableList *vlist,
			ChildListPtr clist,
			struct ChildDesc *childd,
			struct BitList *inited)
{
  symchar *name;
  unsigned long place;
  while (vlist!=NULL){
    name = SimpleNameIdPtr(NamePointer(vlist));
    assert(name!=NULL);
    place = ChildPos(clist,name);
    assert(place);
    SetBit(inited,place-1);
    AssignChildArrayElement(childd,place,MakeBooleanDesc(0,0));
    vlist = NextVariableNode(vlist);
  }
}

static
void TypeRealVarList(CONST struct VariableList *vlist,
		     ChildListPtr clist,
		     struct ChildDesc *childd,
		     struct BitList *inited)
{
  symchar *name;
  unsigned long place;
  while (vlist!=NULL){
    name = SimpleNameIdPtr(NamePointer(vlist));
    assert(name!=NULL);
    place = ChildPos(clist,name);
    assert(place);
    SetBit(inited,place-1);
    AssignChildArrayElement(childd,place,MakeRealDesc(0,0.0,NULL));
    vlist = NextVariableNode(vlist);
  }
}

static
void TypeSymbolVarList(CONST struct VariableList *vlist,
		       ChildListPtr clist,
		       struct ChildDesc *childd,
		       struct BitList *inited)
{
  symchar *name;
  unsigned long place;
  while (vlist!=NULL){
    name = SimpleNameIdPtr(NamePointer(vlist));
    assert(name!=NULL);
    place = ChildPos(clist,name);
    assert(place);
    SetBit(inited,place-1);
    AssignChildArrayElement(childd,place,MakeSymbolDesc(0,NULL));
    vlist = NextVariableNode(vlist);
  }
}

static
void TypeIntegerVarList(CONST struct VariableList *vlist,
			ChildListPtr clist,
			struct ChildDesc *childd,
			struct BitList *inited)
{
  symchar *name;
  unsigned long place;
  while (vlist!=NULL){
    name = SimpleNameIdPtr(NamePointer(vlist));
    assert(name!=NULL);
    place = ChildPos(clist,name);
    assert(place);
    SetBit(inited,place-1);
    AssignChildArrayElement(childd,place,MakeIntegerDesc(0,0L));
    vlist = NextVariableNode(vlist);
  }
}

static
void TypeSetVarList(CONST struct VariableList *vlist,
		    ChildListPtr clist,
		    struct ChildDesc *childd,
		    struct BitList *inited,
		    int basetype)
{
  symchar *name;
  unsigned long place;
  assert((basetype==1)||(basetype==4)); /* Put the debugger here */
  while (vlist!=NULL){
    name = SimpleNameIdPtr(NamePointer(vlist));
    assert(name!=NULL);
    place = ChildPos(clist,name);
    assert(place);
    SetBit(inited,place-1);
    if(basetype==1) {
      AssignChildArrayElement(childd,place,MakeSetDesc(0,1,0));
    } else {
      AssignChildArrayElement(childd,place,MakeSetDesc(0,0,0));
    }
    vlist = NextVariableNode(vlist);
  }
}

/*
 * Do special processing for basetypes in the language.
 */
static
void GetTypeInfoFromISA(struct Statement *stat,
			ChildListPtr clist,
			struct ChildDesc *childd,
			struct BitList *inited)
{
  int bt;
  switch(BaseType(GetStatType(stat))){
  case 0: /* boolean */
    TypeBooleanVarList(GetStatVarList(stat),clist,childd,inited);
    break;
  case 1: /* integer*/
    TypeIntegerVarList(GetStatVarList(stat),clist,childd,inited);
    break;
  case 2: /* real */
    TypeRealVarList(GetStatVarList(stat),clist,childd,inited);
    break;
  case 3: /* set */
    bt = BaseType(GetStatSetType(stat));
    if ((bt == 1) || (bt == 4))
      TypeSetVarList(GetStatVarList(stat),clist,childd,inited,bt);
    else { /* error */
      STATEMENT_ERROR(stat, (bt < 0) ?
                      "Missing OF clause in set IS_A declaration":
                      "Incorrect OF type in set IS_A declaration");
      FPRINTF(ASCERR,
              "Treating as set OF symbol to allow continued processing...\n");
      TypeSetVarList(GetStatVarList(stat),clist,childd,inited,4);
    }
    break;
  case 4: /* symbol */
    TypeSymbolVarList(GetStatVarList(stat),clist,childd,inited);
    break;
  default: /* error */
    STATEMENT_ERROR(stat, "Non-fundamental type used in atom definition");
    break;
  }
}

static
ChildListPtr g_def_child_list_ptr=NULL;
static
struct ChildDesc *g_def_child_desc_ptr=NULL;
static
struct BitList *g_def_child_bit_list = NULL;

static
struct value_t DefEvaluateName(struct Name *nptr)
{
  symchar *name;
  struct ChildDesc rec;
  unsigned long pos;
  if ((g_def_child_list_ptr==NULL)||(g_def_child_desc_ptr==NULL)||
      (g_def_child_bit_list==NULL))
    return CreateErrorValue(name_unfound);
  name = SimpleNameIdPtr(nptr);
  if (name == NULL) return CreateErrorValue(name_unfound);
  if ((pos = ChildPos(g_def_child_list_ptr,name))==0){
    FPRINTF(ASCERR,"Undefine child used in atom definition.\n");
    return CreateErrorValue(name_unfound);
  }
  if (!ReadBit(g_def_child_bit_list,pos-1)){
    FPRINTF(ASCERR,
      "Identifier used before being defined in atom definition.\n");
    return CreateErrorValue(name_unfound);
  }
  rec = GetChildArrayElement(g_def_child_desc_ptr,pos);
  if (!ValueAssigned(rec)) return CreateErrorValue(undefined_value);
  switch(ChildDescType(rec)){
  case real_child:
    return CreateRealValue(RealDefaultValue(rec),RealDimensions(rec),0);
  case integer_child:
    return CreateIntegerValue(IntegerDefault(rec),0);
  case boolean_child:
    return CreateBooleanValue(BooleanDefault(rec),0);
  case set_child:
    return CreateSetValue(CopySet(SetDefault(rec)));
  case symbol_child:
    return CreateSymbolValue(SymbolDefault(rec),0);
  case bad_child:
    return CreateErrorValue(type_conflict); /* should never ever be here */
  }
  return CreateErrorValue(type_conflict);
}

static
struct value_t DefEvaluateExpr(struct Expr *expr,
			       ChildListPtr clist,
			       struct ChildDesc *childd,
			       struct BitList *inited)
{
  struct value_t result;
  IVAL(result);
  g_def_child_list_ptr = clist;
  g_def_child_desc_ptr = childd;
  g_def_child_bit_list = inited;
  result = EvaluateExpr(expr,NULL,DefEvaluateName);
  g_def_child_list_ptr = NULL;
  g_def_child_desc_ptr = NULL;
  g_def_child_bit_list = NULL;
  return result;
}

/* evaluate the default assignments of atom/relation children */
static void EvaluateAssignment(struct Statement *stat,
                        ChildListPtr clist,
                        struct ChildDesc *childd,
                        struct BitList *inited)
{
  symchar *lhs;
  CONST dim_type *dim;
  struct ChildDesc rec;
  unsigned long place;
  struct value_t result;
  struct Name *var;
  struct Expr *expr;
  IVAL(result);
  var = DefaultStatVar(stat);
  expr = DefaultStatRHS(stat);
  lhs = SimpleNameIdPtr(var);
  assert(lhs!=NULL);
  if ((place=ChildPos(clist,lhs))>0){
    rec = GetChildArrayElement(childd,place);
    result = DefEvaluateExpr(expr,clist,childd,inited);
    switch(ChildDescType(rec)){
    case real_child:
      if (ValueKind(result)==real_value){
        if (ValueAssigned(rec)){
          if (0 != (dim = CheckDimensionsMatch(RealValueDimensions(result),
                                               RealDimensions(rec)))){
            AssignChildArrayElement(childd,place,
                                    MakeRealDesc(1,RealValue(result),
                                                 dim));
          }
          else{
            STATEMENT_ERROR(stat, "Dimensionally inconsistent assignment");
            PrintDimenMessage("Mismatched dimensions"
            	,"LHS",RealValueDimensions(result)
            	,"RHS",RealDimensions(rec)
            );
          }
        }
        else{
          AssignChildArrayElement(childd,place,
                                  MakeRealDesc(1,RealValue(result),
                                               RealValueDimensions(result)));
        }
      }
      else if (ValueKind(result)==integer_value){
        if (ValueAssigned(rec)){
          if (0 != (dim = CheckDimensionsMatch(Dimensionless(),
                                               RealDimensions(rec)))){
            AssignChildArrayElement(childd,place,
                                    MakeRealDesc(1,
                                                 (double)IntegerValue(result),
                                                 dim));
          }
          else{
            STATEMENT_ERROR(stat, "Dimensionally inconsistent assignment");
            PrintDimenMessage("Mismatched dimensions"
            	,"LHS",Dimensionless()
            	,"RHS",RealDimensions(rec)
            );
          }
        }
        else{
          AssignChildArrayElement(childd,place,
                                  MakeRealDesc(1,(double)IntegerValue(result),
                                               Dimensionless()));
        }
      }
      else /*error*/
        STATEMENT_ERROR(stat,"Assignment type conflict");
      break;
    case integer_child:
        if (ValueKind(result)==integer_value){
          AssignChildArrayElement(childd,place,
                                  MakeIntegerDesc(1,IntegerValue(result)));
        }
        else { /*error*/
          STATEMENT_ERROR(stat,"Assignment type conflict");
        }
      break;
    case boolean_child:
      if (ValueKind(result)==boolean_value){
        AssignChildArrayElement(childd,place,
                                MakeBooleanDesc(1,BooleanValue(result)));
      }
      break;
    case set_child:
      if (!ValueAssigned(rec)){
        if (ValueKind(result)==set_value){
          AssignChildArrayElement(childd,place,
                                  MakeSetDesc(1,SetIsIntegerSet(rec)
                                              ,SetValue(result)));
        }
        else/*error*/
          STATEMENT_ERROR(stat,"Assignment type conflict");

      }
      else /*error*/
        STATEMENT_ERROR(stat, "Attempt to reassign to a set atom child");
      break;
    case symbol_child:
      if (!ValueAssigned(rec)){
        if (ValueKind(result)==symbol_value){
          AssignChildArrayElement(childd,place,
                                  MakeSymbolDesc(1,SymbolValue(result)));
        }
        else /*error*/
          STATEMENT_ERROR(stat,"Assignment type conflict");
      }
      else /*error*/
        STATEMENT_ERROR(stat, "Attempt to reassign to a symbol atom child");
      break;
    case bad_child:
        STATEMENT_ERROR(stat, "Attempt to assign to a childless atom child?");
      break;
    }
  }
  else
    STATEMENT_ERROR(stat, "Assignment to undefined atom child");
}

static
void MakeStatementPass(struct gl_list_t *l,
		       ChildListPtr clist,
		       struct ChildDesc *childd,
		       struct BitList *blist,
		       struct BitList *inited)
{
  register unsigned long c,len;
  register struct Statement *stat;
  len = gl_length(l);
  c = FirstNonZeroBit(blist);
  while(c<len){
    if (ReadBit(blist,c)){
      stat = (struct Statement *)gl_fetch(l,c+1);
      switch(StatementType(stat)){
      case ISA:
        if ( BaseType(GetStatType(stat)) < NUM_FUNDTYPES ) {
	  GetTypeInfoFromISA(stat,clist,childd,inited);
	  ClearBit(blist,c);
	} /* skip non-fund types */
	break;
      case ASGN:
        EvaluateAssignment(stat,clist,childd,inited);
	ClearBit(blist,c);
	break;
      default: /* IRT, ATS, AA, FOR, LREL, REL, RUN, IF, WHEN, EXT,
                 REF, CASGN, COND, FNAME */
        break;
      }
    }
    c++;
  }
}

static
unsigned long BitCount(struct BitList *blist)
{
  register unsigned long result=0,c,len;
  len = BLength(blist);
  for(c=FirstNonZeroBit(blist); c < len;c++)
    if (ReadBit(blist,c)) result++;
  return result;
}

static void FillInValues(symchar *name,
			 struct StatementList *sl,
			 ChildListPtr clist,
			 struct ChildDesc *childd)
{
  struct gl_list_t *list;
  struct BitList *blist,*inited;
  unsigned long c,len,oldcount;
  list = GetList(sl);
  inited = CreateBList(ChildListLen(clist));
  len  = gl_length(list);
  blist = CreateBList(len);
  for(c=0;c<len;c++) SetBit(blist,c);
  oldcount = len+1;
  while((!BitListEmpty(blist))&&(BitCount(blist)<oldcount)){
    oldcount = BitCount(blist);
    MakeStatementPass(list,clist,childd,blist,inited);
  }
  if (!BitListEmpty(blist)){
    FPRINTF(ASCERR,"Bad atom definition: %s. Statements left unexecuted.\n",
	SCP(name));
  }
  DestroyBList(blist);
  DestroyBList(inited);
}

struct ChildDesc *MakeChildDesc(symchar *name,
				struct StatementList *sl,
				ChildListPtr clist)
{
  register unsigned num_children;
  register struct ChildDesc *result;
  num_children = ChildListLen(clist);
  if (num_children) {
    result = CreateChildDescArray(num_children);
    FillInValues(name,sl,clist,result);
  } else {
    result = CreateEmptyChildDescArray();
  }
  return result;
}

unsigned long CalcByteSize(enum type_kind t,
			   ChildListPtr clist,
			   struct ChildDesc *childd)
{
  switch(t){
  case real_type:
    return RealAtomByteLength(ChildListLen(clist),childd);
  case boolean_type:
    return BooleanAtomByteLength(ChildListLen(clist),childd);
  case integer_type:
    return IntegerAtomByteLength(ChildListLen(clist),childd);
  case real_constant_type:
    return ConstantByteLength(REAL_CONSTANT_INST);
  case integer_constant_type:
    return ConstantByteLength(INTEGER_CONSTANT_INST);
  case boolean_constant_type:
    return ConstantByteLength(BOOLEAN_CONSTANT_INST);
  case symbol_constant_type:
    return ConstantByteLength(SYMBOL_CONSTANT_INST);
  case set_type:
    return SetAtomByteLength(ChildListLen(clist),childd);
  case symbol_type:
    return SymbolAtomByteLength(ChildListLen(clist),childd);
  case relation_type:
    return RelationAtomByteLength(ChildListLen(clist),childd);
  case logrel_type:
    return LogRelAtomByteLength(ChildListLen(clist),childd);
  default:
    Asc_Panic(2, NULL, "Unknown type.\n");
    exit(2);/* Needed to keep gcc from whining */
  }
  /*NOTREACHED*/
}

