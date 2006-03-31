/*
 *  Temporary Statement Output routines
 *  by Tom Epperly
 *  Version: $Revision: 1.41 $
 *  Version control file: $RCSfile: statio.c,v $
 *  Date last modified: $Date: 1998/04/21 23:49:55 $
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */
#define INDENTATION 4

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "stattypes.h"
#include "statement.h"
#include "statio.h"
#include "exprio.h"
#include "nameio.h"
#include "when.h"
#include "select.h"
#include "switch.h"
#include "bit.h"
#include "vlistio.h"
#include "slist.h"
#include "setio.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "module.h"
#include "forvars.h"
#include "find.h"
#include "symtab.h"
#include "braced.h"
#include "instance_enum.h"
#include "cmpfunc.h"

#ifndef lint
static CONST char StatementIOID[] = "$Id: statio.c,v 1.41 1998/04/21 23:49:55 ballan Exp $";
#endif

static int g_show_statement_detail = 1;
/* global to control display detail. It's default value 1 means
 * print the expression with relations. On occasion this is burdensome
 * and the expression can be suppressed by setting this variable to
 * any value other than 1.
 * It should generally be restored to the default state after temporary
 * modification.
 */

static int *g_statio_suppressions = NULL;
/* SIS returns TRUE if we do care about suppressions */
#define GSS (g_statio_suppressions!=NULL)
/* SUP(s) returns TRUE if statement s is to be suppressed */
#define SUP(s) g_statio_suppressions[StatementType(s)]

static const char *g_statio_label[5] = {
  "Asc-WSEM-bug:",
  "Asc-Style:   ",
  "Asc-Warning: ",
  "Asc-Error:   ",
  "Asc-Fatal:   "
};

/*
 * g_statio_stattypenames maps to the enum stat_t to strings from the
 * symbol table.
 */
/*
 * ANSI ASSUMPTION: the contents of g_statio_stattypenames will be
 * NULL on startup.
 */
static
symchar *g_statio_stattypenames[WILLBE+1];
/* typenames for the subclasses of FLOW statement */
static
symchar *g_statio_flowtypenames[fc_stop+1];

static
void Indent(FILE *f, register int i)
{
  register int tabs;
  if (i<=0) return;
  tabs = i / 8;
  while (tabs--) PUTC('\t',f);
  i %= 8;
  while (i--) PUTC(' ',f);
}

/* return a string of the kind */
static
char *ForKindString(enum ForKind fk)
{
  switch (fk) {
  case fk_create:
    return "CREATE";
  case fk_do:
    return "DO";
  case fk_check:
    return "CHECK";
  case fk_expect:
    return "EXPECT";
  default:
    return "ERRFOR";
  }
}

static
void WriteOrder(FILE *f, enum ForOrder o)
{
  switch(o){
  case f_increasing: FPRINTF(f," INCREASING"); break;
  case f_decreasing: FPRINTF(f," DECREASING"); break;
  default: break;
  }
}

static
void WriteWhenNode(FILE *f,struct WhenList *w, int i)
{
  register struct Set *set;
  Indent(f,i);
  set = WhenSetList(w);
  if (set!=NULL){
    FPRINTF(f,"CASE ");
    WriteSet(f,set);
  }
  else FPRINTF(f,"OTHERWISE");
  FPRINTF(f," :\n");
  WriteStatementList(f,WhenStatementList(w),i+INDENTATION);
}

static
void WriteWhenList(FILE *f, struct WhenList *w, int i)
{
  while (w!=NULL) {
    WriteWhenNode(f,w,i);
    w = NextWhenCase(w);
  }
}

static
void WriteSelectNode(FILE *f, struct SelectList *sel, int i)
{
  register struct Set *set;
  Indent(f,i);
  set = SelectSetList(sel);
  if (set!=NULL){
    FPRINTF(f,"CASE ");
    WriteSet(f,set);
  } else {
    FPRINTF(f,"OTHERWISE");
  }
  FPRINTF(f," :\n");
  WriteStatementList(f,SelectStatementList(sel),i+INDENTATION);
}

static
void WriteSelectList(FILE *f, struct SelectList *sel, int i)
{
  while (sel!=NULL) {
    WriteSelectNode(f,sel,i);
    sel = NextSelectCase(sel);
  }
}

static
void WriteSwitchNode(FILE *f, struct SwitchList *sw, int i)
{
  register struct Set *set;
  Indent(f,i);
  set = SwitchSetList(sw);
  if (set!=NULL){
    FPRINTF(f,"CASE ");
    WriteSet(f,set);
  } else {
    FPRINTF(f,"OTHERWISE");
  }
  FPRINTF(f," :\n");
  WriteStatementList(f,SwitchStatementList(sw),i+INDENTATION);
}

static
void WriteSwitchList(FILE *f, struct SwitchList *sw, int i)
{
  while (sw!=NULL) {
    WriteSwitchNode(f,sw,i);
    sw = NextSwitchCase(sw);
  }
}

struct gl_list_t *GetTypeNamesFromStatList(CONST struct StatementList *sl)
{
  register unsigned long len,c;
  register CONST struct gl_list_t *l=NULL;
  struct gl_list_t *found=NULL;
  struct Statement *s=NULL;
  symchar *td;

  found=gl_create(20L);
  if (!sl) return found;
  l = GetList(sl);
  len = gl_length(l);
  for(c=1;c<=len;) {
    s=(struct Statement *)gl_fetch(l,c++);
    td=NULL;
    switch(StatementType(s)) {
    case ISA:
    case WILLBE:
      if (GetStatSetType(s)==NULL)
        td = GetStatType(s);
      else
        td = GetStatSetType(s);
      break;
    case IRT:
      td=    GetStatType(s);
      break;
    case ALIASES:
    case ARR: /* possible bug. ben */
    case ATS:
    case WBTS:
    case WNBTS:
    case REL:
    case LOGREL:
    case AA:
    case FOR: 	/* that this isn't handled further may be a bug */
    case ASGN:
    case CASGN:
    case RUN:
    case IF:
    case WHEN:
    case FNAME:
    case SELECT:
    case SWITCH:
    case COND:
    case CALL:
    case FLOW:
    case WHILE:
    case EXT:
    case REF:	/* that this isn't handled may be a bug */
      break;
    default:
      break;
    }
    if (td) gl_insert_sorted(found,(VOIDPTR)td,(CmpFunc)CmpSymchar);
  }
  return found;
}

unsigned long StatementListLength(CONST struct StatementList *sl)
{
  if (sl==NULL) return 0L;
  if (GetList(sl) == NULL) return 0L;
  return gl_length(GetList(sl));
}

void WriteStatement(FILE *f, CONST struct Statement *s, int i)
{
  struct Name *n;
  assert(s!=NULL);
  if (GSS && SUP(s)) return;
  Indent(f,i);
  if (StatWrong(s)) {
    FPRINTF(f,"(* ERROR *) ");
  }
  switch(StatementType(s)) {
  case ALIASES:
    WriteVariableList(f,GetStatVarList(s));
    FPRINTF(f," ALIASES ");
    WriteName(f,AliasStatName(s));
    FPRINTF(f,";\n");
    break;
  case ARR:
    WriteVariableList(f,ArrayStatAvlNames(s));
    FPRINTF(f," ALIASES (");
    WriteVariableList(f,GetStatVarList(s));

    FPRINTF(f,")\n");
    Indent(f,i+2);
    FPRINTF(f,"WHERE ");
    WriteVariableList(f,ArrayStatSetName(s));
    FPRINTF(f," IS_A set OF %s",
	SCP(ArrayStatIntSet(s)?
		GetBaseTypeName(integer_constant_type):
		GetBaseTypeName(symbol_constant_type)));
    if (ArrayStatSetValues(s) != NULL) {
      FPRINTF(f," WITH_VALUE (");
      WriteSet(f,ArrayStatSetValues(s));
      FPRINTF(f,");\n");
    } else {
      FPRINTF(f,";\n");
    }
    break;
  case ISA:
    WriteVariableList(f,GetStatVarList(s));
    if (GetStatSetType(s)==NULL) {
      FPRINTF(f," IS_A %s",SCP(GetStatType(s)));
      if (GetStatTypeArgs(s) != NULL) {
        FPRINTF(f,"(");
        WriteSet(f,GetStatTypeArgs(s));
        FPRINTF(f,");\n");
      } else {
        FPRINTF(f,";\n");
      }
    } else {
      /* no parameters to sets */
      FPRINTF(f," IS_A %s OF %s;\n",
              SCP(GetStatType(s)),SCP(GetStatSetType(s)));
    }
    break;
  case WILLBE:
    WriteVariableList(f,GetStatVarList(s));
    if (GetStatSetType(s)==NULL) {
      FPRINTF(f," WILL_BE %s",SCP(GetStatType(s)));
      if (GetStatTypeArgs(s) != NULL) {
        FPRINTF(f,"(");
        WriteSet(f,GetStatTypeArgs(s));
        FPRINTF(f,")");
      }
    } else {
      FPRINTF(f," WILL_BE %s OF %s",
              SCP(GetStatType(s)),SCP(GetStatSetType(s)));
    }
    if (GetStatCheckValue(s)!=NULL ) {
      FPRINTF(f," WITH_VALUE ");
      WriteExpr(f,GetStatCheckValue(s));
    }
    FPRINTF(f,";\n");
    break;
  case IRT:
    WriteVariableList(f,GetStatVarList(s));
    FPRINTF(f," IS_REFINED_TO %s",SCP(GetStatType(s)));
    if (GetStatTypeArgs(s) != NULL) {
      FPRINTF(f,"(");
      WriteSet(f,GetStatTypeArgs(s));
      FPRINTF(f,");\n");
    } else {
      FPRINTF(f,";\n");
    }
    break;
  case ATS:
    WriteVariableList(f,GetStatVarList(s));
    FPRINTF(f," ARE_THE_SAME;\n");
    break;
  case WBTS:
    WriteVariableList(f,GetStatVarList(s));
    FPRINTF(f," WILL_BE_THE_SAME;\n");
    break;
  case WNBTS:
    WriteVariableList(f,GetStatVarList(s));
    FPRINTF(f," WILL_NOT_BE_THE_SAME;\n");
    break;
  case AA:
    WriteVariableList(f,GetStatVarList(s));
    FPRINTF(f," ARE_ALIKE;\n");
    break;
  case FOR:
  {
    FPRINTF(f,"FOR %s IN ",SCP(ForStatIndex(s)));
    WriteExpr(f,ForStatExpr(s));
    WriteOrder(f,ForLoopOrder(s));
#ifndef NDEBUG
    if (g_show_statement_detail==1) {
      FPRINTF(f," %s (*<contains%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s>*)\n",
		ForKindString(ForLoopKind(s)),
		(ForContainsRelations(s)) ? " rel" : "",
		(ForContainsLogRelations(s)) ? " lrel" : "",
		(ForContainsDefaults(s)) ? " def" : "",
		(ForContainsCAssigns(s)) ? " con" : "",
		(ForContainsWhen(s)) ? " when" : "",
		(ForContainsIsa(s)) ? " isa" : "",
		(ForContainsSelect(s)) ? " select" : "",
		(ForContainsConditional(s)) ? " conditional" : "",
		(ForContainsWillbe(s)) ? " wb" : "",
		(ForContainsAlike(s)) ? " aa" : "",
		(ForContainsAlias(s)) ? " ali" : "",
		(ForContainsArray(s)) ? " arr" : "",
		(ForContainsWbts(s)) ? " wbts" : "",
		(ForContainsAts(s)) ? " ats" : "",
		(ForContainsIrt(s)) ? " irt" : ""
             );
    } else {
      FPRINTF(f," %s\n", ForKindString(ForLoopKind(s)));
    }
#else
    FPRINTF(f," %s\n", ForKindString(ForLoopKind(s)));
#endif
    WriteStatementList(f,ForStatStmts(s),i+INDENTATION);
    Indent(f,i);
    FPRINTF(f,"END;\n");
    break;
  }
  case REL:
    if (RelationStatName(s)!=NULL) {
      WriteName(f,RelationStatName(s));
      FPRINTF(f," : ");
    }
    if (g_show_statement_detail==1) {
      WriteExpr(f,RelationStatExpr(s));
      FPRINTF(f,";\n");
    } else {
      FPRINTF(f,"(details suppressed);\n");
    }
    break;
  case LOGREL:
    if (LogicalRelStatName(s)!=NULL) {
      WriteName(f,LogicalRelStatName(s));
      FPRINTF(f," : ");
    }
    if (g_show_statement_detail==1) {
      WriteExpr(f,LogicalRelStatExpr(s));
      FPRINTF(f,";\n");
    } else {
      FPRINTF(f,"(details suppressed);\n");
    }
    break;
  case EXT:
    switch (ExternalStatMode(s)) {
      case 0:				/* Procedural */
        FPRINTF(f," EXTERNAL %s(",ExternalStatFuncName(s));
        WriteVariableList(f,ExternalStatVlist(s));
        FPRINTF(f,")\n");
        break;
      case 1:				/* Glassbox Declarative */
      case 2:				/* Blackbox Declarative */
        if (ExternalStatName(s)!=NULL) {
          WriteName(f,ExternalStatName(s));
          FPRINTF(f," : ");
        }
        FPRINTF(f," %s(",ExternalStatFuncName(s));
        WriteVariableList(f,ExternalStatVlist(s));
        FPRINTF(f," : INPUT/OUTPUT");
        if (ExternalStatData(s)!=NULL) {
          FPRINTF(f,", ");
          WriteName(f,ExternalStatData(s));
          FPRINTF(f," : DATA");
        }
        FPRINTF(f,")\n");
        break;
    }
    break;
  case REF:
    WriteVariableList(f,ReferenceStatVlist(s));
    switch (ReferenceStatMode(s)) {
    case 0:
      FPRINTF(f," _IS_ ");
      break;
    case 1:
      FPRINTF(f," _REFERS_ ");
      break;
    }
    FPRINTF(f,"%s\n;",SCP(ReferenceStatName(s)));
    break;
  case CASGN:
    WriteName(f,AssignStatVar(s));
    FPRINTF(f," :== ");
    WriteExpr(f,AssignStatRHS(s));
    FPRINTF(f,";\n");
    break;
  case ASGN:
    WriteName(f,DefaultStatVar(s));
    FPRINTF(f," := ");
    WriteExpr(f,DefaultStatRHS(s));
    FPRINTF(f,";\n");
    break;
  case RUN:
    FPRINTF(f,"RUN ");
    if ( (n=RunStatAccess(s))!=NULL ) {	/* class/model/atom access */
      WriteName(f,n);
      FPRINTF(f,"::");
      WriteName(f,RunStatName(s));
      FPRINTF(f,";\n");
    } else {
      WriteName(f,RunStatName(s));
      FPRINTF(f,";\n");
    }
    break;
  case FIX:
  	FPRINTF(f,"FIX ");
	WriteVariableList(f,FixFreeStatVars(s));
	FPRINTF(f,";\n");
	break;
  case FREE:
  	FPRINTF(f,"FREE ");
	WriteVariableList(f,FixFreeStatVars(s));
	FPRINTF(f,";\n");
	break;
  case CALL:
    FPRINTF(f,"CALL %s(",SCP(CallStatId(s)));
    WriteSet(f,CallStatArgs(s));
    FPRINTF(f,");\n");
    break;
  case WHILE:
    FPRINTF(f,"WHILE (");
    WriteExpr(f,WhileStatExpr(s));
    FPRINTF(f,") DO\n");
    WriteStatementList(f,WhileStatBlock(s),i+INDENTATION);
    Indent(f,i);
    FPRINTF(f,"END WHILE;\n");
    break;
  case FLOW:
    FPRINTF(f,"%s",SCP(StatementTypeString(s)));
    if (FlowStatMessage(s) != NULL) {
      FPRINTF(f," {%s}",BCS(FlowStatMessage(s)));
    }
    FPRINTF(f,";\n");
    break;
  case IF:
    FPRINTF(f,"IF (");
    WriteExpr(f,IfStatExpr(s));
    FPRINTF(f,") THEN\n");
    WriteStatementList(f,IfStatThen(s),i+INDENTATION);
    if (IfStatElse(s)!=NULL) {
      Indent(f,i);
      FPRINTF(f,"ELSE\n");
      WriteStatementList(f,IfStatElse(s),i+INDENTATION);
    }
    Indent(f,i);
    FPRINTF(f,"END IF;\n");
    break;
  case WHEN:
    if (WhenStatName(s)!=NULL) {
      WriteName(f,WhenStatName(s));
      FPRINTF(f," : ");
    }
    FPRINTF(f,"WHEN ");
    WriteVariableList(f,WhenStatVL(s));
    FPRINTF(f,"\n");
    WriteWhenList(f,WhenStatCases(s),i);
    Indent(f,i);
    FPRINTF(f,"END;\n");
    break;
  case FNAME:
    FPRINTF(f,"USE ");
    WriteName(f,FnameStat(s));
    FPRINTF(f,";\n");
    break;
  case SELECT:
    FPRINTF(f,"SELECT ");
    WriteVariableList(f,SelectStatVL(s));
    FPRINTF(f,"\n");
    WriteSelectList(f,SelectStatCases(s),i);
    Indent(f,i);
    FPRINTF(f,"END;\n");
    break;
  case SWITCH:
    FPRINTF(f,"SWITCH ");
    WriteVariableList(f,SwitchStatVL(s));
    FPRINTF(f,"\n");
    WriteSwitchList(f,SwitchStatCases(s),i);
    Indent(f,i);
    FPRINTF(f,"END;\n");
    break;
  case COND:
    FPRINTF(f,"CONDITIONAL \n ");
    WriteStatementList(f,CondStatList(s),i+INDENTATION);
    Indent(f,i);
    FPRINTF(f,"END;\n");
    break;
  default:
    FPRINTF(f,"Unknown\n");
    break;
  }
}

/*
 * Modified to avoid printing twice the statements inside a SELECT
 * VRR. But the "jumping" of the statements is only for the outermost
 * SELECT, the nested SELECTs do not require it.
 */
void WriteStatementList(FILE *f, CONST struct StatementList *sl, int i)
{
  register unsigned long len,c;
  register CONST struct gl_list_t *l;
  struct Statement *stat;
  l = GetList(sl);
  len = gl_length(l);
  for(c=1; c<=len; c++) {
    stat = (struct Statement *)gl_fetch(l,c);
    WriteStatement(f,stat,i);
    if (StatementType(stat)== SELECT && !StatInSELECT(stat)) {
      c = c + SelectStatNumberStats(stat);
    }
  }
}

/*
 * Modified to avoid printing twice the statements inside a SELECT
 * VRR.
 */
void WriteDiffStatementList(FILE *f, CONST struct StatementList *sl1,
                            CONST struct StatementList *sl2, int i)
{
  register unsigned long len1,len2,c;
  register CONST struct gl_list_t *l;
  struct Statement *stat;
  l = GetList(sl1);
  len1=gl_length(l);
  l = GetList(sl2);
  len2 = gl_length(l);
  for(c=(1+len1);c<=len2;c++) {
    stat = (struct Statement *)gl_fetch(l,c);
    WriteStatement(f,stat,i);
    if (StatementType(stat)== SELECT) {
      c = c + SelectStatNumberStats(stat);
    }
  }
}

void WriteStatementSuppressed(FILE *f, CONST struct Statement *stat)
{
  if (stat!=NULL) {
    FPRINTF(f,"  Incorrect statement (final warning) at %s:%lu\n",
            Asc_ModuleBestName(StatementModule(stat)),
            StatementLineNum(stat));
    FPRINTF(f,
            "  All future occurences of this statement will be ignored.\n\n");
  } else {
    FPRINTF(f,"  Suppressing NULL STATEMENT!!! How odd! Expect crash.\n");
  }
}

void WriteStatementErrorMessage(FILE *f, CONST struct Statement *stat,
                                CONST char *message, int noisy,int level)
{
  /* old behavior */
  const char *filename=NULL;
  int line=0;
  if(stat!=NULL){
    filename=Asc_ModuleBestName(StatementModule(stat));
    line=StatementLineNum(stat);
  }

  if (level == 0 || level ==3 ){
	error_reporter_start(ASC_USER_ERROR,filename,line,NULL);
	FPRINTF(f,"%s\n",message);
  }else if(level < 0){
    error_reporter_start(ASC_PROG_NOTE,filename,line,NULL);
	FPRINTF(f,"%s\n",message);
  }else{
	error_reporter_start(ASC_USER_ERROR, filename, line,NULL);
	FPRINTF(f,"%s%s\n",StatioLabel(level), message);
  }

  if(stat!=NULL){

    /* write some more detail */
    g_show_statement_detail = ((noisy!=0) ? 1 : 0);
    WriteStatement(ASCERR,stat,2);
    g_show_statement_detail = 1;

    if (GetEvaluationForTable()!=NULL) {
      WriteForTable(ASCERR,GetEvaluationForTable());
    }
  }else{
    FPRINTF(f,"NULL STATEMENT!");
  }

  error_reporter_end_flush();
}

CONST char *StatioLabel(int level)
{
  if (level >4 || level <1) {
    level = 0;
  }
  return g_statio_label[level];
}

int *GetStatioSuppressions(void) {
  /* WILLBE is the last element of the stattype enum. */
  int *table;
  table = (int *)asccalloc(1,sizeof(int)*(WILLBE+1));
  assert(table!=NULL);
  return table;
}

void DestroySuppressions(int *table)
{
  assert(table!=NULL);
  ascfree(table);
}


void WriteStatementErrorSparse(FILE *f,
                                CONST struct Statement *stat,
                                CONST char *message, int *ignore)
{
  assert(ignore!=NULL);
  g_statio_suppressions = ignore;
  if (!SUP(stat)) {
    if (message) FPRINTF(f,message);
    if (stat ){
      FPRINTF(f," %s:%lu\n",
              Asc_ModuleBestName(StatementModule(stat)),
              StatementLineNum(stat));
      WriteStatement(f,stat,0);
      if (GetEvaluationForTable()!=NULL) {
        WriteForTable(f,GetEvaluationForTable());
        FPRINTF(f,"\n");
      }
    } else {
      FPRINTF(f,"NULL STATEMENT!!!\n");
    }
  }
  g_statio_suppressions = NULL;
}

symchar *StatementTypeString(CONST struct Statement *s)
{
  static symchar *error_statement_sym;
  assert(s!=NULL);
  if (g_statio_stattypenames[0]==NULL) {
    error_statement_sym = AddSymbol("Unknown-statement-type");
    g_statio_stattypenames[ALIASES] = AddSymbol("ALIASES");
    g_statio_stattypenames[ISA] = AddSymbol("IS_A");
    g_statio_stattypenames[ARR] = AddSymbol("ALIASES/IS_A");
    g_statio_stattypenames[IRT] = AddSymbol("IS_REFINED_TO");
    g_statio_stattypenames[ATS] = AddSymbol("ARE_THE_SAME");
    g_statio_stattypenames[AA] = AddSymbol("ARE_ALIKE");
    g_statio_stattypenames[FOR] = AddSymbol("FOR");
    g_statio_stattypenames[REL] = GetBaseTypeName(relation_type);
    g_statio_stattypenames[LOGREL] = GetBaseTypeName(logrel_type);
    g_statio_stattypenames[ASGN] = AddSymbol("Assignment");
    g_statio_stattypenames[CASGN] = AddSymbol("Constant assignment");
    g_statio_stattypenames[RUN] = AddSymbol("RUN");
    g_statio_stattypenames[IF] = AddSymbol("IF");
    g_statio_stattypenames[WHEN] = GetBaseTypeName(when_type);
    g_statio_stattypenames[FNAME] = AddSymbol("FNAME");
    g_statio_stattypenames[SELECT] = AddSymbol("SELECT");
    g_statio_stattypenames[SWITCH] = AddSymbol("SWITCH");
    g_statio_stattypenames[EXT] = AddSymbol("EXTERNAL");
    g_statio_stattypenames[CALL] = AddSymbol("CALL");
    g_statio_stattypenames[FLOW] = AddSymbol("<flow-control>");
    g_statio_stattypenames[WHILE] = AddSymbol("WHILE");
    g_statio_stattypenames[REF] = AddSymbol("_IS_");
    g_statio_stattypenames[COND] = AddSymbol("CONDITIONAL");
    g_statio_stattypenames[WBTS] = AddSymbol("WILL_BE_THE_SAME");
    g_statio_stattypenames[WNBTS] = AddSymbol("WILL_NOT_BE_THE_SAME");
    g_statio_stattypenames[WILLBE] = AddSymbol("WILL_BE");
    g_statio_flowtypenames[fc_return] = AddSymbol("RETURN");
    g_statio_flowtypenames[fc_continue] = AddSymbol("CONTINUE");
    g_statio_flowtypenames[fc_stop] = AddSymbol("STOP");
    g_statio_flowtypenames[fc_break] = AddSymbol("BREAK");
    g_statio_flowtypenames[fc_fallthru] = AddSymbol("FALL_THROUGH");
  }
  switch(StatementType(s)) {
  case ALIASES:
  case ISA:
  case ARR:
  case IRT:
  case ATS:
  case AA:
  case FOR:
  case REL:
  case LOGREL:
  case ASGN:
  case CASGN:
  case RUN:
  case IF:
  case WHEN:
  case FNAME:
  case SELECT:
  case SWITCH:
  case EXT:
  case CALL:
  case REF:
  case COND:
  case WBTS:
  case WNBTS:
  case WILLBE:
  case WHILE:
    /* It's a massive fall through to check that we know the statement */
    return g_statio_stattypenames[StatementType(s)];
  case FLOW:
    return g_statio_flowtypenames[FlowStatControl(s)];
  default:
    return error_statement_sym;
  }
}

void Asc_StatErrMsg_NotAllowedMethod(FILE *f,
                                     CONST struct Statement *stat)
{
  FPRINTF(f,
          "%sIn the method at %s:%lu.\n"
          "  %s statements are not allowed.\n",
          StatioLabel(3),
          Asc_ModuleBestName(StatementModule(stat)),
          StatementLineNum(stat),
          SCP(StatementTypeString(stat)));
}

void Asc_StatErrMsg_NotAllowedDeclarative(FILE *f,
                                          CONST struct Statement *stat)
{
  FPRINTF(f,
          "%sIn the declarative section %s:%lu.\n"
          "  %s statements are not allowed.\n",
          StatioLabel(3),
          Asc_ModuleBestName(StatementModule(stat)),
          StatementLineNum(stat),
          SCP(StatementTypeString(stat)));
}
