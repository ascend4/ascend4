/*
 *  Type Description Output
 *  by Tom Epperly
 *  Created: 1/15/89
 *  Version: $Revision: 1.24 $
 *  Version control file: $RCSfile: type_descio.c,v $
 *  Date last modified: $Date: 1998/04/10 23:25:52 $
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
 */

#include <utilities/ascConfig.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "symtab.h"
#include "setinstval.h"
#include "stattypes.h"
#include "statio.h"
#include "proc.h"
#include "dimen_io.h"
#include "child.h"
#include "childinfo.h"
#include "instance_enum.h"
#include "watchptio.h"
#include "setinst_io.h"
#include "setio.h"
#include "type_desc.h"
#include "module.h"
#include "type_descio.h"

#ifndef lint
static CONST char TypeDescIORCSid[] = "$Id: type_descio.c,v 1.24 1998/04/10 23:25:52 ballan Exp $";
#endif

static
void WriteProcedureList(FILE *f, struct gl_list_t *pl)
{
  unsigned long c,len;
  if (pl!=NULL){
    len = gl_length(pl);
    for(c=1;c<=len;c++)
      WriteProcedure(f,(struct InitProcedure *)gl_fetch(pl,c));
  }
}

static
void WriteChildren(FILE *f,
		   CONST ChildListPtr clist,
		   CONST struct ChildDesc *cdesc)
{
  unsigned long c,len;
  struct ChildDesc rec;
  len = ChildListLen(clist);
  if (len > 0){
    FPRINTF(f,"%20s VALUE\n","CHILD");
    for(c=1;c<=len;c++){
      FPRINTF(f,"%20s ",SCP(ChildStrPtr(clist,c)));
      rec = GetChildArrayElement(cdesc,c);
      if (ValueAssigned(rec)){
	switch(ChildDescType(rec)){
	case real_child:
	  FPRINTF(f,"%g\n",RealDefaultValue(rec));
	  break;
	case integer_child:
	  FPRINTF(f,"%ld\n",IntegerDefault(rec));
	  break;
	case boolean_child:
	  FPRINTF(f,BooleanDefault(rec)?"TRUE\n":"FALSE\n");
	  break;
	case set_child:
	  WriteInstSet(f,SetDefault(rec));
	  PUTC('\n',f);
	  break;
	case symbol_child:
	  FPRINTF(f,"'%s'\n",SCP(SymbolDefault(rec)));
	  break;
	case bad_child:
	  FPRINTF(ASCERR,"WriteChildren called with bad_child of atom\n");
	  FPRINTF(ASCERR," memory has been corrupted !\n");
	  break;
	}
      }
      else FPRINTF(f,"UNDEFINED\n");
    }
  }
}


/* write the name of type on file */
static void WriteTypeName(FILE *f,CONST struct TypeDescription *t)
{
  if (f==NULL) return;
  if (t!=NULL) {
    FPRINTF(f,"%s",SCP(GetName(t)));
  } else {
    FPRINTF(f,"NULL_TYPE");
  }
}
static
void WriteRefiners(FILE *f, CONST struct gl_list_t *rlist)
{
  unsigned long int c, len;
  if (rlist==NULL || gl_length(rlist)==0) return;
  len = gl_length(rlist);
  FPRINTF(f,"\n(* DIRECT REFINEMENTS:");
  for (c=1;c<=len;c++) {
    FPRINTF(f,"\n");
    WriteTypeName(f,(struct TypeDescription *)gl_fetch(rlist,c));
  }
  FPRINTF(f,"*)");
}

static
void WriteDefault(FILE *f, struct TypeDescription *desc)
{
  switch (GetBaseType(desc)) {
  case real_type:
    FPRINTF(f,"%g",GetRealDefault(desc));
    return;
  case boolean_type:
    FPRINTF(f,"%s", (( 0.0==GetBoolDefault(desc) ) ? "FALSE" : "TRUE"));
    return;
  case integer_type:
    FPRINTF(f,"%ld",(long)GetIntDefault(desc));
    return;
  case real_constant_type:
    FPRINTF(f,"%g",GetConstantDefReal(desc));
    return;
  case boolean_constant_type:
    FPRINTF(f,"%s",( (GetConstantDefBoolean(desc)) ? "TRUE" : "FALSE"));
    return;
  case integer_constant_type:
    FPRINTF(f,"%ld",GetConstantDefInteger(desc));
    return;
  case symbol_constant_type:
    FPRINTF(f,"%s",SCP(GetConstantDefSymbol(desc)));
    return;
  case set_type:
  case symbol_type:
  default:
    FPRINTF(f,"*");
    return;
  }
}

static
void WriteIndexType(FILE *f,struct IndexType *it)
{
  if (it->sptr!=NULL) {
    FPRINTF(f,"sptr = \"%s\"\n",SCP(it->sptr));
  } else {
    FPRINTF(f,"sptr = NULL\n");
  }
  FPRINTF(f,"int_index = %u\n",it->int_index);
  FPRINTF(f,"set = ");
  WriteSet(f,it->set);
  FPRINTF(f,"\n");
}

void WriteDefinition(FILE *f, struct TypeDescription *desc)
{
  struct StatementList *tmpsl=NULL;
  if (GetUniversalFlag(desc)) FPRINTF(f,"UNIVERSAL ");
  switch(GetBaseType(desc)){
  case model_type:
    FPRINTF(f,"MODEL %s",SCP(GetName(desc)));
    tmpsl = GetModelParameterList(desc);
    if (StatementListLength(tmpsl) > 0 ) {
      FPRINTF(f,"(\n");
      WriteStatementList(f,tmpsl,4);
      FPRINTF(f,")");
    }
    tmpsl = GetModelParameterWheres(desc);
    if (StatementListLength(tmpsl) > 0 ) {
      FPRINTF(f," WHERE (\n");
      WriteStatementList(f,tmpsl,4);
      FPRINTF(f,")");
    }
    if (GetRefinement(desc)) {
      tmpsl = GetModelParameterReductions(desc);
      if (StatementListLength(tmpsl) > 0 ) {
        FPRINTF(f," REFINES %s(\n",SCP(GetName(GetRefinement(desc))));
        WriteStatementList(f,tmpsl,4);
        FPRINTF(f,");\n");
      } else {
        FPRINTF(f," REFINES %s;\n",SCP(GetName(GetRefinement(desc))));
      }
    } else {
      FPRINTF(f,";\n");
    }
    tmpsl = GetModelAbsorbedParameters(desc);
    if (StatementListLength(tmpsl) > 0 ) {
      FPRINTF(f,"(* passed by value parameters fixed by definition:\n");
      WriteStatementList(f,tmpsl,4);
      FPRINTF(f,"*)\n");
    }
    WriteStatementList(f,GetStatementList(desc),4);
    FPRINTF(f,"METHODS\n");
    WriteProcedureList(f,GetInitializationList(desc));
#ifndef NDEBUG
    FPRINTF(f,"(* Parse info:\n");
    WriteChildList(f,GetChildList(desc));
    FPRINTF(f,"flags = %u\n",(unsigned int)(desc->flags));
    FPRINTF(f,"*)\n");
#endif
    FPRINTF(f,"END %s;\n\n",SCP(GetName(desc)));
    break;
  case dummy_type:
    FPRINTF(f," %s (* no properties *)\n",BASE_UNSELECTED);
    break;
  case patch_type:
    FPRINTF(f,"PATCH %s FOR %s;\n",
	    SCP(GetName(desc)),SCP(GetName(GetPatchOriginal(desc))));
    WriteStatementList(f,GetStatementList(desc),4);
    FPRINTF(f,"METHODS\n");
    WriteProcedureList(f,GetInitializationList(desc));
    FPRINTF(f,"END %s;\n\n",SCP(GetName(desc)));
    break;
  case real_type:
  case boolean_type:
  case integer_type:
  case set_type:
  case symbol_type:
    FPRINTF(f,"ATOM %s",SCP(GetName(desc)));
    if (GetRefinement(desc))
      FPRINTF(f," REFINES %s",SCP(GetName(GetRefinement(desc))));
    if (GetBaseType(desc)==real_type){
      FPRINTF(f," DIMENSION ");
      WriteDimensions(f,GetRealDimens(desc));
    }
    if (AtomDefaulted(desc)) {
      FPRINTF(f," DEFAULT ");
      WriteDefault(f,desc);
    }
    FPRINTF(f,";\n");
    WriteStatementList(f,GetStatementList(desc),4);
    FPRINTF(f,"METHODS\n");
    WriteProcedureList(f,GetInitializationList(desc));
    FPRINTF(f,"END %s;\n",SCP(GetName(desc)));
    WriteChildren(f,GetChildList(desc),GetChildDesc(desc));
    WriteRefiners(f,GetRefiners(desc));
#ifndef NDEBUG
    if (GetModule(desc)!=NULL) {
      FPRINTF(f,"(*\n");
      Asc_ModuleWrite(f,GetModule(desc));
      FPRINTF(f,"*)\n");
    }
#endif
    PUTC('\n',f);
    break;
  case real_constant_type:
  case boolean_constant_type:
  case integer_constant_type:
  case symbol_constant_type:
    FPRINTF(f,"CONSTANT %s",SCP(GetName(desc)));
    if (GetRefinement(desc))
      FPRINTF(f," REFINES %s",SCP(GetName(GetRefinement(desc))));
    if (GetBaseType(desc)==real_constant_type){
      FPRINTF(f," DIMENSION ");
      WriteDimensions(f,GetConstantDimens(desc));
    }
    if (ConstantDefaulted(desc)) {
      FPRINTF(f," DEFAULT ");
      WriteDefault(f,desc);
    }
    FPRINTF(f,";\n");
    WriteRefiners(f,GetRefiners(desc));
    PUTC('\n',f);
    break;
  case relation_type:
    FPRINTF(f,"RELATION %s",SCP(GetName(desc)));
    if (GetRefinement(desc)) {
      FPRINTF(f," REFINES %s;\n",SCP(GetName(GetRefinement(desc))));
    } else {
      FPRINTF(f,";\n");
    }
    WriteStatementList(f,GetStatementList(desc),4);
    FPRINTF(f,"METHODS\n");
    WriteProcedureList(f,GetInitializationList(desc));
    FPRINTF(f,"END %s;\n",SCP(GetName(desc)));
    WriteChildren(f,GetChildList(desc),GetChildDesc(desc));
    WriteRefiners(f,GetRefiners(desc));
    PUTC('\n',f);
    break;
  case logrel_type:
    FPRINTF(f,"LOGRELATION %s",SCP(GetName(desc)));
    if (GetRefinement(desc))
      FPRINTF(f," REFINES %s;\n",SCP(GetName(GetRefinement(desc))));
    else
      FPRINTF(f,";\n");
    WriteStatementList(f,GetStatementList(desc),4);
    FPRINTF(f,"METHODS\n");
    WriteProcedureList(f,GetInitializationList(desc));
    FPRINTF(f,"END %s;\n",SCP(GetName(desc)));
    WriteChildren(f,GetChildList(desc),GetChildDesc(desc));
    WriteRefiners(f,GetRefiners(desc));
    PUTC('\n',f);
    break;
  case when_type:
    FPRINTF(f,"WHEN %s",SCP(GetName(desc)));
    if (GetRefinement(desc)!=NULL) {
      FPRINTF(f," REFINES %s;\n",SCP(GetName(GetRefinement(desc))));
    } else {
      FPRINTF(f,";\n");
    }
    PUTC('\n',f);
    break;
  case array_type:
    {
      struct gl_list_t *ilist;

      FPRINTF(f,"%s\n",SCP(GetName(desc)));
      if (GetBaseType(desc) != array_type ) {
        FPRINTF(f,"Incorrect type = %lu\n",(unsigned long)GetBaseType(desc));
      }
      if (GetUniversalFlag(desc)) {
        FPRINTF(f,"Unexpectedly UNIVERSAL\n");
      }
      if (GetTypeFlags(desc)!=0) {
        FPRINTF(f,"Flags = %d\n",(unsigned int)GetTypeFlags(desc));
      }
      if (GetRefinement(desc)!=NULL) {
        FPRINTF(f,"REFINES %s\n",SCP(GetName(GetRefinement(desc))));
      }
      if (GetModule(desc)!=NULL) {
        FPRINTF(f,"From %s\n",Asc_ModuleName(GetModule(desc)));
      } else {
        FPRINTF(f,"Not defined in a module!\n");
      }
      if (GetChildList(desc) != NULL) {
        FPRINTF(f,"Has unexpected child list.\n");
      }
      if (GetChildList(desc) != NULL) {
        FPRINTF(f,"Has unexpected child list.\n");
      }
      if (GetInitializationList(desc) != NULL) {
        FPRINTF(f,"Has unexpected methods list.\n");
      }
      if (GetStatementList(desc) != NULL) {
        FPRINTF(f,"Has unexpected statement list.\n");
      }
      FPRINTF(f,"Reference count %lu\n",desc->ref_count);
      if (GetArrayBaseType(desc)!=NULL) {
        FPRINTF(f,"Base type:  %s\n",SCP(GetName(GetArrayBaseType(desc))));
      }
      FPRINTF(f,"IsInt,IsRel,IsLog,IsWhen = %d %d %d %d\n",
        GetArrayBaseIsInt(desc),GetArrayBaseIsRelation(desc),
        GetArrayBaseIsLogRel(desc),GetArrayBaseIsWhen(desc));
      ilist = GetArrayIndexList(desc);
      if (ilist!=NULL && gl_length(ilist) !=0) {
        unsigned long c,len;
        len = gl_length(ilist);
        for (c=1;c <= len; c++) {
          WriteIndexType(f,(struct IndexType *)gl_fetch(ilist,c));
        }
      } else {
        FPRINTF(f,"Array with no indices!\n");
      }
      break;
    }
  }
}

void WriteDiffDefinition(FILE *f, struct TypeDescription *desc)
{
  struct TypeDescription *refines=NULL;
  if (GetUniversalFlag(desc)) FPRINTF(f,"UNIVERSAL ");
  switch(GetBaseType(desc)){
  case model_type:
    FPRINTF(f,"MODEL %s",SCP(GetName(desc)));
    if ( NULL != ( refines=GetRefinement(desc) ) ) {
      FPRINTF(f," REFINES %s;\n",SCP(GetName(refines)));
      WriteDiffStatementList(f,GetStatementList(refines),
        GetStatementList(desc),4);
    } else {
      FPRINTF(f,";\n");
      WriteStatementList(f,GetStatementList(desc),4);
    }
    FPRINTF(f,"END %s;\n\n",SCP(GetName(desc)));
    break;
  case real_type:
  case boolean_type:
  case integer_type:
  case set_type:
  case symbol_type:
    FPRINTF(f,"ATOM %s",SCP(GetName(desc)));
    if ( NULL != (refines=GetRefinement(desc)) )
      FPRINTF(f," REFINES %s",SCP(GetName(refines)));
    if (GetBaseType(desc)==real_type){
      FPRINTF(f," DIMENSION ");
      WriteDimensions(f,GetRealDimens(desc));
    }
    FPRINTF(f,";\n");
    WriteDiffStatementList(f,GetStatementList(refines),
      GetStatementList(desc),4);
    FPRINTF(f,"END %s;\n",SCP(GetName(desc)));
    break;
  case relation_type: /* nobody refines relations, really */
  case logrel_type:
  case when_type:
  case array_type:
  case dummy_type:
    break;
  case patch_type:
    break;
  case real_constant_type:
  case boolean_constant_type:
  case integer_constant_type:
  case symbol_constant_type:
    FPRINTF(f,"CONSTANT %s",SCP(GetName(desc)));
    if ( NULL != (refines=GetRefinement(desc)) )
      FPRINTF(f," REFINES %s",SCP(GetName(refines)));
    if (GetBaseType(desc)==real_type){
      FPRINTF(f," DIMENSION ");
      WriteDimensions(f,GetRealDimens(desc));
    }
    /* this needs to be expanded */
    FPRINTF(f,";\n");
    break;
  }
}

/*
 * array of symbol table entries we need.
 */
static symchar *g_symbols[17];
#define G_BASE_SYMBOL_NAME 	g_symbols[0]
#define G_BASE_REAL_NAME	g_symbols[1]
#define G_BASE_INTEGER_NAME	g_symbols[2]
#define G_BASE_BOOLEAN_NAME 	g_symbols[3]
#define G_BASE_CON_SYMBOL_NAME 	g_symbols[4]
#define G_BASE_CON_REAL_NAME 	g_symbols[5]
#define G_BASE_CON_INTEGER_NAME g_symbols[6]
#define G_BASE_CON_BOOLEAN_NAME g_symbols[7]
#define G_BASE_SET_NAME 	g_symbols[8]
#define G_BASE_WHEN_NAME 	g_symbols[9]
#define G_BASE_REL_NAME 	g_symbols[10]
#define G_BASE_LOGREL_NAME 	g_symbols[11]
#define G_BASE_UNSELECTED 	g_symbols[12]
#define G_BASE_EXT_NAME 	g_symbols[13]
#define G_BASE_MODEL_NAME 	g_symbols[14]
#define G_BASE_ARRAY_NAME 	g_symbols[15]
#define G_BASE_PATCH_NAME 	g_symbols[16]

symchar *GetBaseTypeName(enum type_kind bt)
{
  switch (bt) {
  case symbol_type:
    return G_BASE_SYMBOL_NAME;
  case real_type:
    return G_BASE_REAL_NAME;
  case integer_type:
    return G_BASE_INTEGER_NAME;
  case boolean_type:
    return G_BASE_BOOLEAN_NAME;
  case symbol_constant_type:
    return G_BASE_CON_SYMBOL_NAME;
  case real_constant_type:
    return G_BASE_CON_REAL_NAME;
  case integer_constant_type:
    return G_BASE_CON_INTEGER_NAME;
  case boolean_constant_type:
    return G_BASE_CON_BOOLEAN_NAME;
  case set_type:
    return G_BASE_SET_NAME;
  case when_type:
    return G_BASE_WHEN_NAME;
  case relation_type:
    return G_BASE_REL_NAME;
  case logrel_type:
    return G_BASE_LOGREL_NAME;
  case dummy_type:
    return G_BASE_UNSELECTED;
  case model_type:
    return G_BASE_MODEL_NAME;
  case array_type:
    return G_BASE_ARRAY_NAME;
  case patch_type:
    return G_BASE_PATCH_NAME;
  default:
    return G_BASE_EXT_NAME; /* not a type we recognize */
  }
}

void InitBaseTypeNames(void)
{
  FPRINTF(ASCERR,"INIT BASE TYPE NAMES...\n");
  G_BASE_SYMBOL_NAME 	= AddSymbol(BASE_SYMBOL_NAME);
  G_BASE_REAL_NAME	= AddSymbol(BASE_REAL_NAME);
  G_BASE_INTEGER_NAME	= AddSymbol(BASE_INTEGER_NAME);
  G_BASE_BOOLEAN_NAME 	= AddSymbol(BASE_BOOLEAN_NAME);
  G_BASE_CON_SYMBOL_NAME = AddSymbol(BASE_CON_SYMBOL_NAME);
  G_BASE_CON_REAL_NAME 	= AddSymbol(BASE_CON_REAL_NAME);
  G_BASE_CON_INTEGER_NAME = AddSymbol(BASE_CON_INTEGER_NAME);
  G_BASE_CON_BOOLEAN_NAME = AddSymbol(BASE_CON_BOOLEAN_NAME);
  G_BASE_SET_NAME 	= AddSymbol(BASE_SET_NAME);
  G_BASE_WHEN_NAME 	= AddSymbol(BASE_WHEN_NAME);
  G_BASE_REL_NAME 	= AddSymbol(BASE_REL_NAME);
  G_BASE_LOGREL_NAME 	= AddSymbol(BASE_LOGREL_NAME);
  G_BASE_UNSELECTED	= AddSymbol(BASE_UNSELECTED);
  G_BASE_EXT_NAME 	= AddSymbol(BASE_EXT_NAME);
  G_BASE_MODEL_NAME	= AddSymbol("MODEL");
  G_BASE_ARRAY_NAME	= AddSymbol("ARRAY");
  G_BASE_PATCH_NAME	= AddSymbol("PATCH");
  FPRINTF(ASCERR,"...INIT BASE TYPE NAMES\n");
}
