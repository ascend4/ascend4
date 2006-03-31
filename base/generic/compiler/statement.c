/*
 *  Implementation of the statement routines
 *  by Tom Epperly
 *  August 10, 1989
 *  Version: $Revision: 1.32 $
 *  Version control file: $RCSfile: statement.c,v $
 *  Date last modified: $Date: 1998/04/21 23:49:48 $
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
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include "symtab.h"
#include "braced.h"
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
#include "exprs.h"
#include "name.h"
#include "vlist.h"
#include "sets.h"
#include "select.h"
#include "switch.h"
#include "when.h"
#include "module.h"
#include "scanner.h"
/* additional for statement comparisons */
#include "instance_enum.h"
#include "cmpfunc.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "library.h"

#define STMALLOC stmallocF()
#define STFREE(p) ascfree((char *)(p))

#ifndef TRUE
#define TRUE 1
#endif

#ifndef lint
static CONST char StatementID[] = "$Id: statement.c,v 1.32 1998/04/21 23:49:48 ballan Exp $";
#endif

static
struct Statement *stmallocF()
{
  struct Statement *result;
  result = (struct Statement *)ascmalloc((unsigned)sizeof(struct Statement));
  result->stringform = NULL;
  return result;
}

/**
	Create a statement of the specified type and assign the local context to it
*/
static struct Statement *
create_statement_here(enum stat_t t){
	struct Statement *result;
	result=STMALLOC;
	assert(result!=NULL);
	result->t=t;
	result->linenum = LineNum();
	result->mod = Asc_CurrentModule();
	result->context = context_MODEL;
	result->ref_count=1;
	return result;
}

void AddContext(struct StatementList *slist, unsigned int con)
{
  unsigned long c,length;
  struct Statement *s;
  struct gl_list_t *flist;
  struct StatementList *sublist;
  struct WhenList *w;
  struct SelectList *sel;
  struct SwitchList *sw;

  if (StatementListLength(slist)==0) {
    return;
  }
  flist = GetList(slist);
  length=gl_length(flist);
  for(c=1;c<=length;c++){
    s = (struct Statement *)gl_fetch(flist,c);
    assert(s!=NULL);
    s->context |= con;
    switch (s->t){
    case ALIASES:
    case ISA:
    case ARR:
    case WILLBE:
    case IRT:
    case AA:
    case ATS:
    case WBTS:
    case WNBTS:
    case REL:
    case LOGREL:
    case ASGN:
    case CASGN:
    case CALL:
    case EXT:
    case REF:
	case FIX:
	case FREE:
    case RUN:
    case FNAME:
    case FLOW:
      break;
    case FOR:
      sublist = ForStatStmts(s);
      if (sublist!=NULL) {
        AddContext(sublist,con);
      }
      break;
    case WHILE:
      sublist = WhileStatBlock(s);
      if (sublist!=NULL) {
        AddContext(sublist,con);
      }
      break;
	case ASSERT:
      /* no sublists under a TEST statement */
	  break;
    case IF:
      sublist = IfStatThen(s);
      AddContext(sublist,con);
      sublist = IfStatElse(s);
      if (sublist!=NULL) {
        AddContext(sublist,con);
      }
      break;
    case SWITCH:
      sw = SwitchStatCases(s);
      while (sw!=NULL) {
        sublist = SwitchStatementList(sw);
        if (sublist!=NULL) {
          AddContext(sublist,con);
        }
        sw = NextSwitchCase(sw);
      }
      break;
    case WHEN:
      w = WhenStatCases(s);
      while (w!=NULL) {
        sublist = WhenStatementList(w);
        if (sublist!=NULL) {
          AddContext(sublist,con);
        }
        w = NextWhenCase(w);
      }
      break;
    case SELECT:
      sel = SelectStatCases(s);
      while (sel!=NULL) {
        sublist = SelectStatementList(sel);
        if (sublist!=NULL) {
          AddContext(sublist,con);
        }
        sel = NextSelectCase(sel);
      }
      break;
    case COND:
      sublist = CondStatList(s);
      if (sublist!=NULL) {
        AddContext(sublist,con);
      }
      break;
    default:
      Asc_Panic(2, NULL, "AddContext called with bad statement list.");
      break;
    }
  }
}

struct Statement *CreateALIASES(struct VariableList *vl, struct Name *n)
{
  struct Statement *result;
  result=create_statement_here(ALIASES);
  result->v.ali.vl = vl;
  result->v.ali.u.nptr = n;
  result->v.ali.c.setname = NULL;
  result->v.ali.c.intset = -1;
  result->v.ali.c.setvals = NULL;
  /* should check nptr and all vl names here for !contains_at */
  return result;
}

struct Statement *CreateARR(struct VariableList *avlname,
                            struct VariableList *vl,
                            struct VariableList *sn,
                            int intset,
                            struct Set *sv) {
  struct Statement *result;
  result=create_statement_here(ARR);
  result->v.ali.u.avlname = avlname;
  result->v.ali.vl = vl;
  result->v.ali.c.setname = sn;
  result->v.ali.c.intset = intset;
  result->v.ali.c.setvals = sv;
  return result;
}

struct Statement *CreateISA(struct VariableList *vl,
			    symchar *t,
                            struct Set *ta,
			    symchar *st)
{
  struct Statement *result;
  result=create_statement_here(ISA);
  result->v.i.vl = vl;
  result->v.i.type = t;
  result->v.i.typeargs = ta;
  result->v.i.settype = st;
  result->v.i.checkvalue = NULL;
  return result;
}

struct Statement *CreateWILLBE(struct VariableList *vl, symchar *t,
                               struct Set *ta,
			       symchar *st, struct Expr *cv)
{
  struct Statement *result;
  result=create_statement_here(WILLBE);
  result->v.i.vl = vl;
  result->v.i.type = t;
  result->v.i.typeargs = ta;
  result->v.i.settype = st;
  result->v.i.checkvalue = cv;
  return result;
}

struct Statement *CreateIRT(struct VariableList *vl, symchar *t,
                            struct Set *ta)
{
  register struct Statement *result;
  result=create_statement_here(IRT);
  result->v.i.type = t;
  result->v.i.typeargs = ta;
  result->v.i.settype = NULL;
  result->v.i.checkvalue = NULL;
  result->v.i.vl = vl;
  return result;
}

struct Statement *CreateAA(struct VariableList *vl)
{
  register struct Statement *result;
  result=create_statement_here(AA);
  result->v.a.vl = vl;
  return result;
}

struct Statement *CreateATS(struct VariableList *vl)
{
  register struct Statement *result;
  result=create_statement_here(ATS);
  result->v.a.vl = vl;
  return result;
}

struct Statement *CreateFIX(struct VariableList *vars){
	register struct Statement *result;
	/* CONSOLE_DEBUG("CREATING FIX STMT"); */
	result=create_statement_here(FIX);
	result->v.fx.vars = vars;
	return result;
}

struct Statement *CreateFREE(struct VariableList *vars){
  register struct Statement *result;
  CONSOLE_DEBUG("CREATING FREE STMT");
  result=create_statement_here(FREE);
  result->v.fx.vars = vars;
  return result;
}

struct Statement *CreateWBTS(struct VariableList *vl)
{
  register struct Statement *result;
  result=create_statement_here(WBTS);
  result->v.a.vl = vl;
  return result;
}

struct Statement *CreateWNBTS(struct VariableList *vl)
{
  register struct Statement *result;
  result=create_statement_here(WNBTS);
  result->v.a.vl = vl;
  return result;
}


/********************************************************************\
 Returns an unsigned with the low bits set according to whether or
 not there is a statement of the indicated class in the slist given.
 The value is determined by examining the list, not by looking up
 some attribute.
 The return type is valid as a contains field.
\********************************************************************/
unsigned int SlistHasWhat(struct StatementList *slist)
{
  unsigned long c,length;
  struct Statement *stat;
  struct gl_list_t *flist;
  unsigned int what=0;

  flist = GetList(slist);
  for(c=1,length=gl_length(flist);c<=length;c++){
    stat = (struct Statement *)gl_fetch(flist,c);
    switch (StatementType(stat)){
    case ALIASES:
      what |= contains_ALI;
      break;
    case ARR:
      what |= contains_ARR;
      break;
    case ISA:
      what |= contains_ISA;
      break;
    case WILLBE:
      what |= contains_WILLBE;
      break;
    case IRT:
      what |= contains_IRT;
      break;
    case ATS:
      what |= contains_ATS;
      break;
    case WBTS:
      what |= contains_WBTS;
      break;
    case WNBTS:
      what |= contains_WNBTS;
      break;
    case AA:
      what |= contains_AA;
      break;
    case CASGN:
      what |= contains_CAS;
      break;
    case ASGN:
      what |= contains_DEF;
      break;
    case REL:
      what |= contains_REL;
      break;
    case LOGREL:
      what |= contains_LREL;
      break;
    case WHEN:
      what |= contains_WHEN;
      break;
    case SELECT:
      what |= contains_SELECT;
      what |= SelectContains(stat);
      break;
    case COND:
      what |= contains_COND;
      what |= CondContains(stat);
      break;
    case FOR:
      what |= ForContains(stat);
      break;
    default:
      break;
    }
  }
  return what;
}

struct Statement *CreateFOR(symchar *index,
			    struct Expr *expr,
			    struct StatementList *stmts,
			    enum ForOrder order, enum ForKind kind)
{
  register struct Statement *result;
  result=create_statement_here(FOR);
  result->v.f.index = index;
  result->v.f.e = expr;
  result->v.f.stmts = stmts;
  result->v.f.order = order;
  result->v.f.kind = kind;
  result->v.f.contains = SlistHasWhat(stmts);
  AddContext(stmts,context_FOR);
  return result;
}

struct Statement *CreateFlow(enum FlowControl fc, CONST char *mt)
{
  register struct Statement *result;

  switch (fc) {
  case fc_break:
  case fc_continue:
  case fc_fallthru:
  case fc_return:
  case fc_stop:
    break;
  default:
    Asc_Panic(2, "CreateFlow",
              "CreateFlow called with unknown flow of control enum");
    break; /*NOTREACHED*/
  }
  result=create_statement_here(FLOW);
  result->v.flow.fc = fc;
  if (mt != NULL) {
    result->v.flow.message = AddBraceChar(mt,AddSymbolL("stop",4));
  } else {
    result->v.flow.message = NULL;
  }
  return result;
}

void SetRelationName(struct Statement *stat, struct Name *n)
{
  assert(stat && (stat->t==REL) && (stat->v.rel.nptr==NULL));
  /* make assignment */
  stat->v.rel.nptr = n;
}

struct Statement *CreateREL(struct Name *n, struct Expr *relation)
{
  register struct Statement *result;
  result=create_statement_here(REL);
  result->v.rel.nptr = n;
  result->v.rel.relation = relation;
  return result;
}


void SetLogicalRelName(struct Statement *stat, struct Name *n)
{
  assert(stat && (stat->t==LOGREL) && (stat->v.lrel.nptr==NULL));
  /* make assignment */
  stat->v.lrel.nptr = n;
}

struct Statement *CreateLOGREL(struct Name *n, struct Expr *logrel)
{
  register struct Statement *result;
  result=create_statement_here(LOGREL);
  result->v.lrel.nptr = n;
  result->v.lrel.logrel = logrel;
  return result;
}

struct Statement *CreateEXTERN(int mode,
			       struct Name *n, CONST char *funcname,
			       struct VariableList *vl,
			       struct Name *data,
			       struct Name *scope)
{
  register struct Statement *result;
  if(mode==2){
    ERROR_REPORTER_DEBUG("Found blackbox function statement '%s'\n",funcname);
  }

  result=create_statement_here(EXT);
  result->v.ext.mode = mode;
  result->v.ext.nptr = n;
  result->v.ext.extcall = funcname;
  result->v.ext.vl = vl;
  result->v.ext.data = data; 	/* NULL is valid */
  result->v.ext.scope = scope;	/* NULL is valid */
  return result;
}


struct Statement *CreateREF(struct VariableList *vl,
			    symchar *ref_name,
			    symchar *st,
			    int mode)
{
  register struct Statement *result;
  result=create_statement_here(REF);
  result->v.ref.mode = mode;
  result->v.ref.ref_name = ref_name;
  result->v.ref.settype = st;
  result->v.ref.vl = vl;
  return result;
}

struct Statement *CreateRUN(struct Name *n,struct Name *type_access)
{
  register struct Statement *result;
  result=create_statement_here(RUN);
  result->v.r.proc_name = n;
  result->v.r.type_name = type_access;	/* NULL is valid */
  return result;
}

struct Statement *CreateCALL(symchar *sym,struct Set *args)
{
  register struct Statement *result;
  result=create_statement_here(CALL);
  result->v.call.id = sym;
  result->v.call.args = args;	/* NULL is valid */
  return result;
}

struct Statement *CreateASSERT(struct Expr *ex){
  register struct Statement *result;
  result=create_statement_here(ASSERT);
  result->v.asserts.test = ex;
  return result;
}

struct Statement *CreateIF(struct Expr *ex,
			   struct StatementList *thenblock,
			   struct StatementList *elseblock)
{
  register struct Statement *result;
  result=create_statement_here(IF);
  result->v.ifs.test = ex;
  result->v.ifs.thenblock = thenblock;
  result->v.ifs.elseblock = elseblock; /* this may be NULL */
  if (thenblock != NULL) {
    AddContext(thenblock,context_IF);
  }
  if (elseblock != NULL) {
    AddContext(elseblock,context_IF);
  }
  return result;
}

struct Statement *CreateWhile(struct Expr *ex,
			   struct StatementList *thenblock)
{
  register struct Statement *result;
  result=create_statement_here(WHILE);
  result->v.loop.test = ex;
  result->v.loop.block = thenblock;
  if (thenblock != NULL) {
    AddContext(thenblock,context_WHILE);
  }
  return result;
}

struct Statement *CreateWHEN(struct Name *wname, struct VariableList *vlist,
                             struct WhenList *wl)
{
  struct StatementList *sl;
  register struct Statement *result;
  result=create_statement_here(WHEN);
  result->v.w.nptr = wname;
  result->v.w.vl = vlist;
  result->v.w.cases = wl;
  while (wl!= NULL) {
    sl = WhenStatementList(wl);
    AddContext(sl,context_WHEN);
    wl = NextWhenCase(wl);
  }
  return result;
}

struct Statement *CreateFNAME(struct Name *name)
{
  register struct Statement *result;
  result=create_statement_here(FNAME);
  result->v.n.wname = name;
  return result;
}


struct Statement *CreateSWITCH(struct VariableList *v, struct SwitchList *sw)
{
  struct StatementList *sl;
  register struct Statement *result;
  result=create_statement_here(SWITCH);
  result->v.sw.vl = v;
  result->v.sw.cases = sw;
  while (sw!= NULL) {
    sl = SwitchStatementList(sw);
    AddContext(sl,context_SWITCH);
    sw = NextSwitchCase(sw);
  }
  return result;
}


/*
 * This function gives us the number of statements inside a
 * SELECT statement. If there is a SELECT inside a SELECT,
 * the function works recursively
 */
static
int CountStatementsInSelect(struct SelectList *sel)
{
  struct StatementList *sl;
  unsigned long c,length;
  int tmp, count;
  struct Statement *s;

  count = 0;
  while ( sel!=NULL) {
    sl = SelectStatementList(sel);
    length = StatementListLength(sl);
    count = count + length;
    for(c=1;c<=length;c++){
      tmp = 0;
      s = GetStatement(sl,c);
      assert(s!=NULL);
      switch(StatementType(s)) {
        case SELECT:
	  tmp = CountStatementsInSelect(SelectStatCases(s));
	  break;
        default:
	  break;
      }
      count = count + tmp;
    }
    sel = NextSelectCase(sel);
  }
  return count;
}

struct Statement *CreateSELECT(struct VariableList *v, struct SelectList *sel)
{
  unsigned int tmp=0;
  struct StatementList *sl;
  register struct Statement *result;
  result=create_statement_here(SELECT);
  result->v.se.vl = v;
  result->v.se.cases = sel;
  result->v.se.n_statements = CountStatementsInSelect(sel);
  while (sel!= NULL) {
    sl = SelectStatementList(sel);
    tmp |= SlistHasWhat(sl);
    AddContext(sl,context_SELECT);
    sel = NextSelectCase(sel);
  }
  result->v.se.contains = tmp;
  return result;
}


struct Statement *CreateCOND(struct StatementList *stmts)
{
  register struct Statement *result;
  result=create_statement_here(COND);
  result->v.cond.stmts = stmts;
  AddContext(stmts,context_COND);
  result->v.cond.contains =  SlistHasWhat(stmts);
  return result;
}


struct Statement *CreateASSIGN(struct Name *n, struct Expr *rhs)
{
  register struct Statement *result;
  result=create_statement_here(ASGN);
  result->v.asgn.nptr = n;
  result->v.asgn.rhs = rhs;
  return result;
}

struct Statement *CreateCASSIGN(struct Name *n, struct Expr *rhs)
{
  register struct Statement *result;
  result=create_statement_here(CASGN);
  result->v.asgn.nptr = n;
  result->v.asgn.rhs = rhs;
  return result;
}

enum stat_t StatementTypeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  return s->t;
}

struct module_t *StatementModuleF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  return s->mod;
}

unsigned long StatementLineNumF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  return s->linenum;
}

struct Statement *CopyStatementF(struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  if (s->ref_count<MAXREFCOUNT) s->ref_count++;
  return s;
}

void DestroyStatement(struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  if (s->ref_count<MAXREFCOUNT) {
    --(s->ref_count);
    if (s->ref_count ==0) {
      switch(s->t) {
      case ALIASES:
        DestroyVariableList(s->v.ali.vl);
        DestroyName(s->v.ali.u.nptr);
        s->v.ali.vl = NULL;
        s->v.ali.u.nptr = NULL;
        break;
      case ARR:
        DestroyVariableList(s->v.ali.vl);
        DestroyVariableList(s->v.ali.u.avlname);
        DestroyVariableList(s->v.ali.c.setname);
        s->v.ali.vl = NULL;
        s->v.ali.u.avlname = NULL;
        s->v.ali.c.setname = NULL;
        s->v.ali.c.intset = -2;
        if (s->v.ali.c.setvals != NULL) {
          DestroySetList(s->v.ali.c.setvals);
          s->v.ali.c.setvals = NULL;
        }
        break;
      case ISA:
      case IRT:
      case WILLBE:
        DestroyVariableList(s->v.i.vl);
        s->v.i.vl = NULL;
        if (s->v.i.checkvalue != NULL) {
          DestroyExprList(s->v.i.checkvalue);
          s->v.i.checkvalue = NULL;
        }
        if (s->v.i.typeargs != NULL) {
          DestroySetList(s->v.i.typeargs);
          s->v.i.typeargs = NULL;
        }
        break;
      case ATS:
      case AA:
      case WBTS:
      case WNBTS:
        DestroyVariableList(s->v.a.vl);
        s->v.a.vl = NULL;
        break;
      case FOR:
        DestroyExprList(s->v.f.e);
        s->v.f.e = NULL;
        DestroyStatementList(s->v.f.stmts);
        s->v.f.stmts = NULL;
        break;
      case REL:
        DestroyName(s->v.rel.nptr);
        s->v.rel.nptr = NULL;
        DestroyExprList(s->v.rel.relation);
        s->v.rel.relation = NULL;
        break;
      case LOGREL:
        DestroyName(s->v.lrel.nptr);
        s->v.lrel.nptr = NULL;
        DestroyExprList(s->v.lrel.logrel);
        s->v.lrel.logrel = NULL;
        break;
      case CALL:
        s->v.call.id = NULL;
        DestroySetList(s->v.call.args);
        s->v.call.args = NULL;
        break;
      case EXT:
        DestroyName(s->v.ext.nptr);
        s->v.ext.nptr = NULL;
        DestroyVariableList(s->v.ext.vl);
        s->v.ext.vl = NULL;
        if (s->v.ext.data) DestroyName(s->v.ext.data);
        s->v.ext.data = NULL;
	    if (s->v.ext.scope) DestroyName(s->v.ext.scope);
	    s->v.ext.scope = NULL;
	    break;
      case REF:
        DestroyVariableList(s->v.ref.vl);
        s->v.ref.vl = NULL;
        break;
      case ASGN:
      case CASGN:
        DestroyName(s->v.asgn.nptr);
        s->v.asgn.nptr = NULL;
        DestroyExprList(s->v.asgn.rhs);
        s->v.asgn.rhs = NULL;
        break;
      case RUN:
        DestroyName(s->v.r.proc_name);
        s->v.r.proc_name = NULL;
        DestroyName(s->v.r.type_name);
        s->v.r.type_name = NULL;
        break;

      case FIX:
	  case FREE:
        DestroyVariableList(s->v.fx.vars);
		break;

      case ASSERT:
        DestroyExprList(s->v.asserts.test);
        s->v.asserts.test = NULL;
        break;

      case IF:
        DestroyStatementList(s->v.ifs.thenblock);
        s->v.ifs.thenblock = NULL;
        if (s->v.ifs.elseblock!=NULL){
          DestroyStatementList(s->v.ifs.elseblock);
          s->v.ifs.elseblock = NULL;
        }
        DestroyExprList(s->v.ifs.test);
        s->v.ifs.test = NULL;
        break;
      case WHEN:
        DestroyName(s->v.w.nptr);
        s->v.w.nptr = NULL;
        DestroyWhenList(s->v.w.cases);
        s->v.w.cases = NULL;
        DestroyVariableList(s->v.w.vl);
        s->v.w.vl = NULL;
        break;
      case FNAME:
        DestroyName(s->v.n.wname);
        s->v.n.wname = NULL;
        break;
      case SELECT:
        DestroySelectList(s->v.se.cases);
        s->v.se.cases = NULL;
        DestroyVariableList(s->v.se.vl);
        s->v.se.vl = NULL;
        break;
      case SWITCH:
        DestroySwitchList(s->v.sw.cases);
        s->v.sw.cases = NULL;
        DestroyVariableList(s->v.sw.vl);
        s->v.sw.vl = NULL;
        break;
      case COND:
        DestroyStatementList(s->v.cond.stmts);
        s->v.cond.stmts = NULL;
        break;
      case FLOW:
        DestroyBraceChar(s->v.flow.message); /* tolerates NULL */
        break;
      case WHILE:
        if (s->v.loop.block!=NULL){
          DestroyStatementList(s->v.loop.block);
          s->v.loop.block = NULL;
        }
        DestroyExprList(s->v.loop.test);
        s->v.loop.test = NULL;
        break;
      }
      s->mod = NULL;
      s->linenum = 0;
      s->context |= context_WRONG;
      if (s->stringform != NULL) {
        ascfree(s->stringform);
        s->stringform = NULL;
      }
      STFREE(s);
    }
  }
}

struct Statement *CopyToModify(struct Statement *s)
{
  register struct Statement *result;
  size_t size;
  assert(s!=NULL);
  assert(s->ref_count);
  result = STMALLOC;
  if (s->stringform != NULL) {
    size = strlen(s->stringform);
    result->stringform = (char *)ascmalloc(size+1);
    memcpy(result->stringform,s->stringform,size+1);
  }
  result->ref_count = 1;
  result->t = s->t;
  result->mod = s->mod;
  result->linenum = s->linenum;
  result->context = s->context;
  switch(result->t) {
  case ARR:
    result->v.ali.c.intset = s->v.ali.c.intset;
    result->v.ali.c.setname = CopyVariableList(s->v.ali.c.setname);
    result->v.ali.c.setvals = CopySetList(s->v.ali.c.setvals);
    result->v.ali.vl = CopyVariableList(s->v.ali.vl);
    result->v.ali.u.avlname = CopyVariableList(s->v.ali.u.avlname);
    break;
  case ALIASES:
    result->v.ali.vl = CopyVariableList(s->v.ali.vl);
    result->v.ali.u.nptr = CopyName(s->v.ali.u.nptr);
    result->v.ali.c.setname = NULL;
    result->v.ali.c.intset = -1;
    result->v.ali.c.setvals = NULL;
    break;
  case ISA:
  case IRT:
  case WILLBE:
    result->v.i.type = s->v.i.type;
    result->v.i.settype = s->v.i.settype;
    result->v.i.vl = CopyVariableList(s->v.i.vl);
    result->v.i.checkvalue =  CopyExprList(s->v.i.checkvalue);
    /* is this complete for IS_A with args to type? */
    break;
  case AA:
  case ATS:
  case WBTS:
  case WNBTS:
    result->v.a.vl = CopyVariableList(s->v.a.vl);
    break;
  case FOR:
    result->v.f.index = s->v.f.index;
    result->v.f.e = CopyExprList(s->v.f.e);
    result->v.f.stmts = CopyListToModify(s->v.f.stmts);
    result->v.f.order = s->v.f.order;
    break;
  case REL:
    result->v.rel.nptr = CopyName(s->v.rel.nptr);
    result->v.rel.relation = CopyExprList(s->v.rel.relation);
    break;
  case LOGREL:
    result->v.lrel.nptr = CopyName(s->v.lrel.nptr);
    result->v.lrel.logrel = CopyExprList(s->v.lrel.logrel);
    break;
  case CALL:
    result->v.call.args = CopySetList(s->v.call.args);
    result->v.call.id = s->v.call.id;
    break;
  case EXT:
    result->v.ext.mode = s->v.ext.mode;
    result->v.ext.nptr = CopyName(s->v.ext.nptr);
    result->v.ext.extcall = s->v.ext.extcall;
    result->v.ext.vl = CopyVariableList(s->v.ext.vl);
    result->v.ext.data = CopyName(s->v.ext.data);
    result->v.ext.scope = CopyName(s->v.ext.scope);
    break;
  case REF:
    result->v.ref.mode = s->v.ref.mode;
    result->v.ref.ref_name = s->v.ref.ref_name;
    result->v.ref.vl = CopyVariableList(s->v.ref.vl);
    break;
  case ASGN:
  case CASGN:
    result->v.asgn.nptr = CopyName(s->v.asgn.nptr);
    result->v.asgn.rhs = CopyExprList(s->v.asgn.rhs);
    break;
  case RUN:
    result->v.r.proc_name = CopyName(s->v.r.proc_name);
    result->v.r.type_name = CopyName(s->v.r.type_name);
    break;
  case ASSERT:
    result->v.asserts.test = CopyExprList(s->v.asserts.test);
    break;
  case IF:
    result->v.ifs.test = CopyExprList(s->v.ifs.test);
    result->v.ifs.thenblock = CopyListToModify(s->v.ifs.thenblock);
    if (s->v.ifs.elseblock!=NULL) {
      result->v.ifs.elseblock = CopyListToModify(s->v.ifs.elseblock);
    } else {
      result->v.ifs.elseblock = NULL;
    }
    break;
  case WHEN:
    result->v.w.nptr = CopyName(s->v.w.nptr);
    result->v.w.vl = CopyVariableList(s->v.w.vl);
    result->v.w.cases = CopyWhenList(s->v.w.cases);
    break;
  case FNAME:
    result->v.n.wname = CopyName(s->v.n.wname);
    break;
  case SELECT:
    result->v.se.vl = CopyVariableList(s->v.se.vl);
    result->v.se.cases = CopySelectList(s->v.se.cases);
    result->v.se.n_statements = s->v.se.n_statements;
    result->v.se.contains = s->v.se.contains;
    break;
  case SWITCH:
    result->v.sw.vl = CopyVariableList(s->v.sw.vl);
    result->v.sw.cases = CopySwitchList(s->v.sw.cases);
    break;
  case COND:
    result->v.cond.stmts = CopyListToModify(s->v.cond.stmts);
    break;
  case FLOW:
    result->v.flow.fc = s->v.flow.fc;
    break;
  case WHILE:
    result->v.loop.test = CopyExprList(s->v.loop.test);
    if (s->v.loop.block!=NULL) {
      result->v.loop.block = CopyListToModify(s->v.loop.block);
    } else {
      result->v.loop.block = NULL;
    }
    break;
  }
  return result;
}

unsigned int GetStatContextF(CONST struct Statement *s)
{
  assert(s!=NULL);
  switch (s->t) {
  case ALIASES:
  case ARR:
  case ISA:
  case WILLBE:
  case IRT:
  case AA:
  case ATS:
  case WBTS:
  case WNBTS:
  case REL:
  case LOGREL:
  case ASGN:
  case CASGN:
  case FOR:
  case CALL:
  case EXT:
  case REF:
  case RUN:
  case FIX:
  case FREE:
  case ASSERT:
  case IF:
  case WHEN:
  case FNAME:
  case SELECT:
  case SWITCH:
  case COND:
  case WHILE:
  case FLOW:
    return s->context;
  default:
	ERROR_REPORTER_STAT(ASC_PROG_ERR,s,"GetStatContext called on incorrect statement type.");
    return context_MODEL;
  }
}

void SetStatContext(struct Statement *s, unsigned int c)
{
  assert(s!=NULL);
  switch (s->t) {
  case ALIASES:
  case ARR:
  case ISA:
  case WILLBE:
  case IRT:
  case AA:
  case ATS:
  case WBTS:
  case WNBTS:
  case REL:
  case LOGREL:
  case ASGN:
  case CASGN:
  case FOR:
  case CALL:
  case EXT:
  case REF:
  case RUN:
  case FIX:
  case FREE:
  case ASSERT:
  case IF:
  case WHEN:
  case FNAME:
  case SELECT:
  case SWITCH:
  case COND:
  case WHILE:
  case FLOW:
    s->context = c;
    break;
  default:
    Asc_Panic(2, "SetStatContext",
              "SetStatContext called on incorrect statement type.\n");
    break;
  }
}

void MarkStatContext(struct Statement *s, unsigned int c)
{
  assert(s!=NULL);
  switch (s->t) {
  case ALIASES:
  case ARR:
  case ISA:
  case WILLBE:
  case IRT:
  case AA:
  case ATS:
  case WBTS:
  case WNBTS:
  case REL:
  case LOGREL:
  case ASGN:
  case CASGN:
  case FOR:
  case CALL:
  case EXT:
  case REF:
  case RUN:
  case FIX:
  case FREE:
  case ASSERT:
  case IF:
  case WHEN:
  case FNAME:
  case SELECT:
  case SWITCH:
  case COND:
  case WHILE:
  case FLOW:
    s->context |= c;
    break;
  default:
    Asc_Panic(2, "MarkStatContext",
              "MarkStatContext called on incorrect statement type.\n");
    break;
  }
}

struct VariableList *GetStatVarList(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(	(s->t==ISA) ||
		(s->t==WILLBE) ||
		(s->t==IRT) ||
		(s->t==AA)  ||
		(s->t==ATS) ||
		(s->t==WBTS) ||
		(s->t==WNBTS) ||
		(s->t==ARR) ||
		(s->t==ALIASES)
	);
  switch (s->t) {
  case ISA:
  case WILLBE:
  case IRT:
    return (s)->v.i.vl;
  case AA:
  case ATS:
  case WBTS:
  case WNBTS:
    return (s)->v.a.vl;
  case ALIASES:
  case ARR:
    return (s)->v.ali.vl;
  default:
    return NULL;
  }
}

symchar *GetStatTypeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert((s->t==ISA) || (s->t==IRT) || (s->t==WILLBE));
  return s->v.i.type;
}

CONST struct Set *GetStatTypeArgsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert((s->t==ISA) || (s->t==IRT) || (s->t==WILLBE));
  return s->v.i.typeargs;
}

unsigned int GetStatNeedsArgs(CONST struct Statement *s)
{
  CONST struct TypeDescription *d;
  assert(s!=NULL);
  assert(s->ref_count);
  assert((s->t==ISA) || (s->t==IRT));
  d = FindType(s->v.i.type);
  if (d==NULL || GetBaseType(d) != model_type) {
    return 0;
  }
  return GetModelParameterCount(d);
}

symchar *GetStatSetTypeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ISA || s->t==WILLBE);
  return s->v.i.settype;
}

CONST struct Expr *GetStatCheckValueF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==WILLBE);
  return s->v.i.checkvalue;
}

CONST struct Name *AliasStatNameF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ALIASES);
  return s->v.ali.u.nptr;
}

CONST struct VariableList *ArrayStatAvlNamesF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ARR);
  return s->v.ali.u.avlname;
}

CONST struct VariableList *ArrayStatSetNameF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ARR);
  return s->v.ali.c.setname;
}

int ArrayStatIntSetF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ARR);
  return s->v.ali.c.intset;
}

CONST struct Set *ArrayStatSetValuesF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ARR);
  return s->v.ali.c.setvals;
}

symchar *ForStatIndexF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==FOR);
  return s->v.f.index;
}

struct Expr *ForStatExprF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==FOR);
  return s->v.f.e;
}

struct StatementList *ForStatStmtsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t == FOR);
  return s->v.f.stmts;
}

enum ForOrder ForLoopOrderF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return s->v.f.order;
}

enum ForKind ForLoopKindF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return s->v.f.kind;
}


unsigned ForContainsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains);
}

unsigned ForContainsRelationsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_REL);
}


unsigned ForContainsLogRelationsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_LREL);
}

unsigned ForContainsDefaultsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_DEF);
}

unsigned ForContainsCAssignsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_CAS);
}


unsigned ForContainsWhenF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_WHEN);
}

unsigned ForContainsIsaF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_ISA);
}

unsigned ForContainsIrtF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_IRT);
}

unsigned ForContainsAlikeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_AA);
}

unsigned ForContainsAliasF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_ALI);
}

unsigned ForContainsArrayF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_ARR);
}

unsigned ForContainsAtsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_ATS);
}

unsigned ForContainsWbtsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_WBTS);
}

unsigned ForContainsWnbtsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_WNBTS);
}

unsigned ForContainsWillbeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_WILLBE);
}

unsigned ForContainsSelectF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_SELECT);
}

unsigned ForContainsConditionalF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_COND);
}

unsigned ForContainsIllegalF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_ILL);
}

struct Name *DefaultStatVarF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ASGN);
  return s->v.asgn.nptr;
}

struct Name *AssignStatVarF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==CASGN);
  return s->v.asgn.nptr;
}

struct Expr *DefaultStatRHSF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==ASGN);
  return s->v.asgn.rhs;
}


struct Expr *AssignStatRHSF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==CASGN);
  return s->v.asgn.rhs;
}

struct Name *RelationStatNameF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==REL);
  return s->v.rel.nptr;
}

struct Expr *RelationStatExprF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==REL);
  return s->v.rel.relation;
}

struct Name *LogicalRelStatNameF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==LOGREL);
  return s->v.lrel.nptr;
}

struct Expr *LogicalRelStatExprF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==LOGREL);
  return s->v.lrel.logrel;
}

int ExternalStatModeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  return(s->v.ext.mode);
}

struct Name *ExternalStatNameF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  return(s->v.ext.nptr);
}

struct Name *ExternalStatDataF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  return(s->v.ext.data);
}

struct Name *ExternalStatScopeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  return(s->v.ext.scope);
}

struct VariableList *ExternalStatVlistF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  return(s->v.ext.vl);
}

CONST char *ExternalStatFuncNameF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  return(s->v.ext.extcall);
}

int ReferenceStatModeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==REF);
  return(s->v.ref.mode);
}

symchar *ReferenceStatNameF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==REF);
  return(s->v.ref.ref_name);
}

symchar *ReferenceStatSetTypeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t==REF);
  return (s->v.ref.settype);
}

struct VariableList *ReferenceStatVlistF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==REF);
  return(s->v.ref.vl);
}

struct Name *RunStatNameF(CONST struct Statement *s)
{
  assert(s && (s->ref_count) && (s->t == RUN));
  return s->v.r.proc_name;
}

struct Name *RunStatAccessF(CONST struct Statement *s)
{
  assert(s && (s->ref_count) && (s->t == RUN));
  return s->v.r.type_name;
}

struct VariableList *FixFreeStatVarsF(CONST struct Statement *s){
	assert(s!=NULL);
	assert(s->t==FIX || s->t==FREE);
	return(s->v.fx.vars);
}

struct Set *CallStatArgsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==CALL);
  return(s->v.call.args);
}

symchar *CallStatIdF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==CALL);
  return(s->v.call.id);
}

struct Expr *WhileStatExprF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == WHILE));
  return s->v.loop.test;
}

struct Expr *AssertStatExprF(CONST struct Statement *s){
	assert(s && s->ref_count && (s->t == ASSERT));
	return s->v.asserts.test;
}

struct Expr *IfStatExprF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == IF));
  return s->v.ifs.test;
}

struct StatementList *WhileStatBlockF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t == WHILE);
  return s->v.loop.block;
}

struct StatementList *IfStatThenF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->ref_count);
  assert(s->t == IF);
  return s->v.ifs.thenblock;
}

struct StatementList *IfStatElseF(CONST struct Statement *s)
{
  assert(s && (s->ref_count) && (s->t == IF));
  return s->v.ifs.elseblock;
}


struct Name *WhenStatNameF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == WHEN));
  return s->v.w.nptr;
}

void SetWhenName(struct Statement *s, struct Name *n)
{
  assert(s && (s->t==WHEN) && (s->v.w.nptr==NULL));
  s->v.w.nptr = n;
}

struct VariableList *WhenStatVLF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == WHEN));
  return s->v.w.vl;
}

struct WhenList *WhenStatCasesF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == WHEN));
  return s->v.w.cases;
}


/*
 * Compare functions for WHEN statements. It includes the
 * decomposition of the WHEN in the list of variables and
 * the list of CASEs. Also, each case is decomposed in
 * the set of values and the list of statements. It is
 * done here since we are comparing the statement rather
 * than only a WhenList structure (when.[ch]).
 */

int CompareWhenStatements(CONST struct Statement *s1,
			  CONST struct Statement *s2)
{
  int ctmp; /* temporary comparison result */
  unsigned long int ltmp;
  struct VariableList *vl1, *vl2;
  struct WhenList *cases1,*cases2;
  struct Set *val1, *val2;
  struct StatementList *sl1, *sl2;

  vl1 = WhenStatVL(s1);
  vl2 = WhenStatVL(s2);

  ctmp = CompareVariableLists(vl1,vl2);
  if (ctmp != 0) {
    return ctmp;
  }

  cases1 = WhenStatCases(s1);
  cases2 = WhenStatCases(s2);

  while ( (cases1!=NULL) && (cases2!=NULL) ) {
    val1 = WhenSetList(cases1);
    val2 = WhenSetList(cases2);

    ctmp = CompareSetStructures(val1,val2);
    if (ctmp != 0) {
      return ctmp;
    }

    sl1 = WhenStatementList(cases1);
    sl2 = WhenStatementList(cases2);
    ctmp = CompareStatementLists(sl1,sl2,&ltmp);
    if (ctmp != 0) {
      return ctmp;
    }
    cases1 = NextWhenCase(cases1);
    cases2 = NextWhenCase(cases2);
  }
  return 0;
}

struct StatementList *CondStatListF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == COND));
  return s->v.cond.stmts;
}

unsigned CondContainsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==COND);
  return (s->v.cond.contains);
}

unsigned CondContainsRelationsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==COND);
  return (s->v.cond.contains & contains_REL);
}


unsigned CondContainsLogRelationsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==COND);
  return (s->v.cond.contains & contains_LREL);
}

struct Name *FnameStatF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == FNAME));
  return s->v.n.wname;
}

int SelectStatNumberStatsF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == SELECT));
  return s->v.se.n_statements;
}

struct VariableList *SelectStatVLF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == SELECT));
  return s->v.se.vl;
}

struct SelectList *SelectStatCasesF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == SELECT));
  return s->v.se.cases;
}

/*
 * Compare functions for SELECT statements. It includes the
 * decomposition of the SELECT in the list of variables and
 * the list of CASEs. Also, each case is decomposed in
 * the set of values and the list of statements. It is
 * done here since we are comparing the statement rather
 * than only a SelectList structure (select.[ch]).
 */

int CompareSelectStatements(CONST struct Statement *s1,
			    CONST struct Statement *s2)
{
  int ctmp; /* temporary comparison result */
  unsigned long int ltmp;
  struct VariableList *vl1, *vl2;
  struct SelectList *cases1,*cases2;
  struct Set *val1, *val2;
  struct StatementList *sl1, *sl2;

  vl1 = SelectStatVL(s1);
  vl2 = SelectStatVL(s2);

  ctmp = CompareVariableLists(vl1,vl2);
  if (ctmp != 0) {
    return ctmp;
  }

  cases1 = SelectStatCases(s1);
  cases2 = SelectStatCases(s2);

  while ( (cases1!=NULL) && (cases2!=NULL) ) {
    val1 = SelectSetList(cases1);
    val2 = SelectSetList(cases2);

    ctmp = CompareSetStructures(val1,val2);
    if (ctmp != 0) {
      return ctmp;
    }

    sl1 = SelectStatementList(cases1);
    sl2 = SelectStatementList(cases2);
    ctmp = CompareStatementLists(sl1,sl2,&ltmp);
    if (ctmp != 0) {
      return ctmp;
    }
    cases1 = NextSelectCase(cases1);
    cases2 = NextSelectCase(cases2);
  }
  return 0;
}


unsigned SelectContainsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains);
}

unsigned SelectContainsRelationsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_REL);
}


unsigned SelectContainsLogRelationsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_LREL);
}

unsigned SelectContainsDefaultsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_DEF);
}

unsigned SelectContainsCAssignsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_CAS);
}


unsigned SelectContainsWhenF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_WHEN);
}

unsigned SelectContainsIsaF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_ISA);
}

unsigned SelectContainsIrtF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_IRT);
}

unsigned SelectContainsAlikeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_AA);
}

unsigned SelectContainsAliasF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_ALI);
}

unsigned SelectContainsArrayF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_ARR);
}

unsigned SelectContainsAtsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_ATS);
}

unsigned SelectContainsWbtsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_WBTS);
}

unsigned SelectContainsWnbtsF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_WNBTS);
}

unsigned SelectContainsWillbeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_WILLBE);
}

unsigned SelectContainsSelectF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_SELECT);
}

unsigned SelectContainsConditionalF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_COND);
}

unsigned SelectContainsIllegalF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_ILL);
}


struct VariableList *SwitchStatVLF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == SWITCH));
  return s->v.sw.vl;
}

struct SwitchList *SwitchStatCasesF(CONST struct Statement *s)
{
  assert(s && s->ref_count && (s->t == SWITCH));
  return s->v.sw.cases;
}

/*
 * Compare functions for SWITCH statements. It includes the
 * decomposition of the SWITCH in the list of variables and
 * the list of CASEs. Also, each case is decomposed in
 * the set of values and the list of statements. It is
 * done here since we are comparing the statement rather
 * than only a SwitchList structure (switch.[ch]).
 */

int CompareSwitchStatements(CONST struct Statement *s1,
			    CONST struct Statement *s2)
{
  int ctmp; /* temporary comparison result */
  unsigned long int ltmp;
  struct VariableList *vl1, *vl2;
  struct SwitchList *cases1,*cases2;
  struct Set *val1, *val2;
  struct StatementList *sl1, *sl2;

  vl1 = SwitchStatVL(s1);
  vl2 = SwitchStatVL(s2);

  ctmp = CompareVariableLists(vl1,vl2);
  if (ctmp != 0) {
    return ctmp;
  }

  cases1 = SwitchStatCases(s1);
  cases2 = SwitchStatCases(s2);

  while ( (cases1!=NULL) && (cases2!=NULL) ) {
    val1 = SwitchSetList(cases1);
    val2 = SwitchSetList(cases2);

    ctmp = CompareSetStructures(val1,val2);
    if (ctmp != 0) {
      return ctmp;
    }

    sl1 = SwitchStatementList(cases1);
    sl2 = SwitchStatementList(cases2);
    ctmp = CompareStatementLists(sl1,sl2,&ltmp);
    if (ctmp != 0) {
      return ctmp;
    }
    cases1 = NextSwitchCase(cases1);
    cases2 = NextSwitchCase(cases2);
  }
  return 0;
}


/*********************************************************************\
CompareStatements(s1,s2);
Returns -1,0,1 as s1 is <, ==, > s2.
s1 < s2 if type(s1) < type(s2), exception: NULL > all statements.
For statements of the same type, compared according to
the number of arguments or alphabetically, as appropriate.
We are comparing statement contents, not statement
memory location or origin.
FOR/DO vs FOR/CREATE is considered a different type, so
the 'in a method' location matters. file, line, etc don't, though.
\*********************************************************************/
int CompareStatements(CONST struct Statement *s1, CONST struct Statement *s2)
{
  int ctmp; /* temporary comparison result */
  unsigned long int ltmp;
  if (s1 == s2) return 0;
  if (s1==NULL) return 1;
  if (s2==NULL) return -1;
  if (s1->t > s2->t) return 1;
  if (s1->t < s2->t) return -1;
  switch (s1->t) {
  case ALIASES:
    ctmp = CompareNames(AliasStatName(s1),AliasStatName(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
  case ARR:
    ctmp = CompareVariableLists(ArrayStatAvlNames(s1),ArrayStatAvlNames(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareVariableLists(ArrayStatSetName(s1),ArrayStatSetName(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = ArrayStatIntSet(s1) - ArrayStatIntSet(s2);
    if (ctmp != 0) {
      if (ctmp ==1) {
        return CmpSymchar(GetBaseTypeName(integer_constant_type),
                          GetBaseTypeName(symbol_constant_type));
      } else {
	/* ctmp == -1 */
        return CmpSymchar(GetBaseTypeName(symbol_constant_type) ,
                          GetBaseTypeName(integer_constant_type));
      }
    }
    return CompareSetStructures(ArrayStatSetValues(s1),ArrayStatSetValues(s2));
  case ISA:
    if (GetStatSetType(s1)!=NULL || GetStatSetType(s2) !=NULL) {
      if (GetStatSetType(s1) == NULL) { return -1; }
      if (GetStatSetType(s2) == NULL) { return 1; }
      ctmp = CmpSymchar(GetStatSetType(s1),GetStatSetType(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    /* fallthru */
  case IRT:
    ctmp = CmpSymchar(GetStatType(s1),GetStatType(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareSetStructures(GetStatTypeArgs(s1),GetStatTypeArgs(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
  case WILLBE:
    if (GetStatSetType(s1) != NULL || GetStatSetType(s2) != NULL) {
      if (GetStatSetType(s1) == NULL) { return -1; }
      if (GetStatSetType(s2) == NULL) { return 1; }
      ctmp = CmpSymchar(GetStatSetType(s1),GetStatSetType(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    ctmp = CmpSymchar(GetStatType(s1),GetStatType(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareSetStructures(GetStatTypeArgs(s1),GetStatTypeArgs(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareExprs(GetStatCheckValue(s1),GetStatCheckValue(s2));
  case ATS: /* fallthru */
  case WBTS: /* fallthru */
  case WNBTS: /* fallthru */
  case AA:
    return CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
  case FOR:
    if (ForStatIndex(s1)!=ForStatIndex(s2)) {
      return CmpSymchar(ForStatIndex(s1),ForStatIndex(s2));
    }
    if (ForContains(s1)!=ForContains(s2)) {
      /* arbitrary but consistent */
      return ( ForContains(s1) > ForContains(s2)) ? 1 : -1;
    }
    if (ForLoopOrder(s1)!=ForLoopOrder(s2)) {
      return ( ForLoopOrder(s1) > ForLoopOrder(s2)) ? 1 : -1;
    }
    if (ForLoopKind(s1)!=ForLoopKind(s2)) {
      return ( ForLoopKind(s1) > ForLoopKind(s2)) ? 1 : -1;
    }
    ctmp = CompareExprs(ForStatExpr(s1),ForStatExpr(s2));
    if (ctmp!=0) {
      return ctmp;
    }
    return CompareStatementLists(ForStatStmts(s1),ForStatStmts(s2),&ltmp);
  case REL:
    ctmp = CompareNames(RelationStatName(s1),RelationStatName(s2));
    if (ctmp!=0 &&
        /* we need to skip this for system generated names */
        !NameAuto(RelationStatName(s1)) &&
        !NameAuto(RelationStatName(s2))) {
      return ctmp;
    }
    return CompareExprs(RelationStatExpr(s1),RelationStatExpr(s2));
  case LOGREL:
    ctmp = CompareNames(LogicalRelStatName(s1),LogicalRelStatName(s2));
    if (ctmp!=0 &&
        /* we need to skip this for system generated names */
        !NameAuto(LogicalRelStatName(s1)) &&
        !NameAuto(LogicalRelStatName(s2))) {
      return ctmp;
    }
    return CompareExprs(LogicalRelStatExpr(s1),LogicalRelStatExpr(s2));
  case ASGN:
    ctmp = CompareNames(DefaultStatVar(s1),DefaultStatVar(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareExprs(DefaultStatRHS(s1),DefaultStatRHS(s2));
  case CASGN:
    ctmp = CompareNames(AssignStatVar(s1),AssignStatVar(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareExprs(AssignStatRHS(s1),AssignStatRHS(s2));
  case RUN:
    ctmp = CompareNames(RunStatName(s1),RunStatName(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareNames(RunStatAccess(s1),RunStatAccess(s2));
  case WHILE:
    ctmp = CompareExprs(WhileStatExpr(s1), WhileStatExpr(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareStatementLists(WhileStatBlock(s1), WhileStatBlock(s2),&ltmp);

  case ASSERT:
	ctmp = CompareExprs(AssertStatExpr(s1), AssertStatExpr(s2));
	return ctmp;

  case IF:
    ctmp = CompareExprs(IfStatExpr(s1), IfStatExpr(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareStatementLists(IfStatThen(s1), IfStatThen(s2),&ltmp);
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareStatementLists(IfStatElse(s1), IfStatElse(s2),&ltmp);
  case WHEN:
    ctmp = CompareNames(WhenStatName(s1),WhenStatName(s2));
    if (ctmp!=0 &&
        /* we need to skip this for system generated names */
        !NameAuto(WhenStatName(s1)) &&
        !NameAuto(WhenStatName(s2))) {
      return ctmp;
    }
    return CompareWhenStatements(s1,s2);
  case FNAME:
    return CompareNames(FnameStat(s1),FnameStat(s2));
  case SELECT:
    if (SelectContains(s1)!=SelectContains(s2)) {
      /* arbitrary but consistent */
      return ( SelectContains(s1) > SelectContains(s2)) ? 1 : -1;
    }
    if (SelectStatNumberStats(s1)!=SelectStatNumberStats(s2)) {
      /* arbitrary but consistent */
      return (SelectStatNumberStats(s1) > SelectStatNumberStats(s2)) ? 1 : -1;
    }
    return CompareSelectStatements(s1,s2);
  case SWITCH:
    return CompareSwitchStatements(s1,s2);
  case CALL:
    if (CallStatId(s1)!=CallStatId(s2)) {
      return CmpSymchar(CallStatId(s1),CallStatId(s2));
    }
    return CompareSetStructures(CallStatArgs(s1),CallStatArgs(s2));
  case EXT:
    ctmp = strcmp(ExternalStatFuncName(s1),ExternalStatFuncName(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    if (ExternalStatMode(s1) != ExternalStatMode(s2)) {
      return (ExternalStatMode(s1) > ExternalStatMode(s2)) ? 1 : -1;
    }
    ctmp = CompareNames(ExternalStatName(s1),ExternalStatName(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareNames(ExternalStatScope(s1),ExternalStatScope(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareNames(ExternalStatData(s1),ExternalStatData(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareVariableLists(ExternalStatVlist(s1),ExternalStatVlist(s2));
  case REF:
    /* need fixing. since both are ill defined, they
     * will not be fixed until the definitions are really known.
     */
    Asc_Panic(2, NULL, "Don't know how to compare REF statements\n");
    exit(2);/* Needed to keep gcc from whining */
  case COND:
    return CompareStatementLists(CondStatList(s1),CondStatList(s2),&ltmp);
  case FLOW:
    if (FlowStatControl(s1) != FlowStatControl(s2)) {
      return (FlowStatControl(s1) > FlowStatControl(s2)) ? 1 : -1;
    }
    /* FlowStatMessage is considered comment info and so not compared */
    return 0;
  default:
	ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"CompareStatements called with unknown statement");
    return 0;
  }
}

/* could we merge some of this with plain compare statements via some
 *static functions? baa. fix me.
 */
int CompareISStatements(CONST struct Statement *s1, CONST struct Statement *s2)
{
  unsigned long int ltmp;
  struct TypeDescription *d1, *d2;
  int ctmp; /* temporary comparison result */

  if (s1 == s2) return 0;
  if (s1==NULL) return 1;
  if (s2==NULL) return -1;
  if (s1->t > s2->t) return 1;
  if (s1->t < s2->t) return -1;
  switch (s1->t) {
  case ISA:
    /* compare set OF parts */
    if (GetStatSetType(s1)!=NULL || GetStatSetType(s2) !=NULL) {
      if (GetStatSetType(s1) == NULL) { return -1; }
      if (GetStatSetType(s2) == NULL) { return 1; }
      ctmp = CmpSymchar(GetStatSetType(s1),GetStatSetType(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    /********** FALLTHRU  *  FALL THROUGH  *  FALL TO IRT *************/
  case IRT:
    /* compare basic types */
    d1 = FindType(GetStatType(s1));
    d2 = FindType(GetStatType(s2));
    assert(d1 !=NULL);
    assert(d2 !=NULL);
    if (d1!=d2 && MoreRefined(d1,d2) != d2) {
      if (MoreRefined(d1,d2) == NULL) {
        return 1;
      } else {
        return -1;
      }
    }
    ctmp = CompareSetStructures(GetStatTypeArgs(s1),GetStatTypeArgs(s2));
    /* this may be a little too picky comparison of arglists */
    if (ctmp != 0) {
      return ctmp;
    }
    return CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
    /* IS_A IS_REFINED_TO have not WITH_VALUE part */
  case WILLBE:
    /* compare set OF parts */
    if (GetStatSetType(s1) != NULL || GetStatSetType(s2) != NULL) {
      if (GetStatSetType(s1) == NULL) { return -1; }
      if (GetStatSetType(s2) == NULL) { return 1; }
      ctmp = CmpSymchar(GetStatSetType(s1),GetStatSetType(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    /* compare basic types */
    d1 = FindType(GetStatType(s1));
    d2 = FindType(GetStatType(s2));
    assert(d1 !=NULL);
    assert(d2 !=NULL);
    if (d1!=d2 && MoreRefined(d1,d2) != d2) {
      if (MoreRefined(d1,d2) == NULL) {
        return 1;
      } else {
        return -1;
      }
    }
    ctmp = CompareSetStructures(GetStatTypeArgs(s1),GetStatTypeArgs(s2));
    /* this may be a little too picky comparison of arglists */
    if (ctmp != 0) {
      return ctmp;
    }
    ctmp = CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
    if (ctmp != 0) {
      return ctmp;
    }
    if (GetStatCheckValue(s1) != NULL) {
      return CompareExprs(GetStatCheckValue(s1),GetStatCheckValue(s2));
    } else {
      return 0;
    }
  case WBTS: /* fallthru */
  case WNBTS:
    return CompareVariableLists(GetStatVarList(s1),GetStatVarList(s2));
  case FOR:
    if (ForStatIndex(s1)!=ForStatIndex(s2)) {
      return CmpSymchar(ForStatIndex(s1),ForStatIndex(s2));
    }
    if (ForContains(s1)!=ForContains(s2)) {
      /* arbitrary but consistent */
      return ( ForContains(s1) > ForContains(s2)) ? 1 : -1;
    }
    if (ForLoopOrder(s1)!=ForLoopOrder(s2)) {
      return ( ForLoopOrder(s1) > ForLoopOrder(s2)) ? 1 : -1;
    }
    if (ForLoopKind(s1)!=ForLoopKind(s2)) {
      return ( ForLoopKind(s1) > ForLoopKind(s2)) ? 1 : -1;
    }
    ctmp = CompareExprs(ForStatExpr(s1),ForStatExpr(s2));
    if (ctmp!=0) {
      return ctmp;
    }
    return CompareISLists(ForStatStmts(s1),ForStatStmts(s2),&ltmp);
  case LOGREL:
  case REL:
    return CompareStatements(s1,s2);
  case ALIASES:
  case ARR:
  case ATS: /* fallthru */
  case AA:
  case ASGN:
  case CASGN:
  case RUN:
  case CALL:
  case ASSERT:
  case IF:
  case WHEN:
  case FNAME:
  case SELECT:
  case SWITCH:
  case EXT: /* fallthru */
  case REF:
  case COND:
  case FLOW:
  case WHILE:
	ERROR_REPORTER_STAT(ASC_PROG_ERR,s1,"CompareISStatements called with incorrect statement");
    return -1;
  default:
	ERROR_REPORTER_STAT(ASC_PROG_ERR,s1,"CompareISStatements called with unknown statement");
    return 1;
  }
}

