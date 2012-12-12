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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>

#include "symtab.h"
#include "braced.h"
#include <ascend/general/panic.h>
#include <ascend/general/list.h>


#include "functype.h"
#include "expr_types.h"
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
  result = ASC_NEW(struct Statement);
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
    case LNK:
    case UNLNK:
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
    case SOLVER:
    case OPTION:
    case SOLVE:
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
      ASC_PANIC("AddContext called with bad statement list.");
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


struct Statement *IgnoreLNK(symchar *key, struct Name *n_key, struct VariableList *vl)
{
  register struct Statement *result;
  result=create_statement_here(LNK);

  if(key != NULL){
    result->v.lnk.key = key;
    result->v.lnk.key_type = link_ignore; /**>> DS: this flag tells the compiler that it is a ignore LINK */
    result->v.lnk.vl = vl;
  }else if(n_key != NULL){
    result->v.lnk.key = n_key->val.id;
    result->v.lnk.key_type = link_ignore;
    result->v.lnk.vl = vl;
  }
  return result;
}

struct Statement *CreateLNK(symchar *key, struct Name *n_key, struct VariableList *vl)
{
  register struct Statement *result;
  result=create_statement_here(LNK);

  if(key != NULL){
    result->v.lnk.key = key;
    result->v.lnk.key_type = link_symchar;
    result->v.lnk.vl = vl;
  }else if(n_key != NULL){
    result->v.lnk.key = n_key->val.id; /**>>DS TODO: Not sure if this is fine, I however need a single datatype to store the keys in the appropriate datastructures */
    result->v.lnk.key_type = link_name;
    result->v.lnk.vl = vl;
  }
  return result;
}


struct Statement *CreateUNLNK(symchar *key, struct Name *n_key, struct VariableList *vl)
{
  register struct Statement *result;
  result=create_statement_here(UNLNK);

  if(key != NULL){
    result->v.lnk.key = key;
    result->v.lnk.key_type = 0;
    result->v.lnk.vl = vl;
  }else if(n_key != NULL){
    result->v.lnk.key = n_key->val.id; /**>>DS: Not sure if this is fine, I however need a single datatype to store the keys in the appropriate datastructures */
    result->v.lnk.key_type = 1;
    result->v.lnk.vl = vl;
  }
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
	/*CONSOLE_DEBUG("CREATING FIX STMT");*/
	result=create_statement_here(FIX);
	result->v.fx.vars = vars;
	/*WriteVariableList(ASCERR,result->v.fx.vars);*/
	return result;
}

struct Statement *CreateFREE(struct VariableList *vars){
  register struct Statement *result;
  /* CONSOLE_DEBUG("CREATING FREE STMT"); */
  result=create_statement_here(FREE);
  result->v.fx.vars = vars;
  return result;
}

struct Statement *CreateSOLVER(CONST char *solvername){
	register struct Statement *result;
	result=create_statement_here(SOLVER);
	result->v.solver.name = solvername;
	/*CONSOLE_DEBUG("CREATED SOLVER STATEMENT");*/
	return result;
}

struct Statement *CreateOPTION(CONST char *optname, struct Expr *rhs){
	register struct Statement *result;
	result=create_statement_here(OPTION);
	result->v.option.name = optname;
	result->v.option.rhs = rhs;
	return result;
}

struct Statement *CreateSOLVE(){
	register struct Statement *result;
	result=create_statement_here(SOLVE);
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
    case LNK:
      what |= contains_LNK;
      break;
    case UNLNK:
      what |= contains_UNLNK;
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
    case EXT:
      what |= contains_EXT;
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

struct Statement *CreateFOR(symchar *sindex,
			    struct Expr *expr,
			    struct StatementList *stmts,
			    enum ForOrder order, enum ForKind kind)
{
  register struct Statement *result;
  result=create_statement_here(FOR);
  result->v.f.index = sindex;
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
    result->v.flow.message = AddBraceChar(mt,AddSymbol("stop"));
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

struct Statement *CreateEXTERNGlassBox(
			       struct Name *n, CONST char *funcname,
			       struct VariableList *vl,
			       struct Name *data,
			       struct Name *scope)
{
  register struct Statement *result;
  ERROR_REPORTER_DEBUG("Found glassbox equation statement '%s'\n",funcname);

  result=create_statement_here(EXT);
  result->v.ext.mode = ek_glass;
  result->v.ext.extcall = funcname;
  result->v.ext.u.glass.nptr = n;
  result->v.ext.u.glass.vl = vl;
  result->v.ext.u.glass.data = data;   /* NULL is valid */
  result->v.ext.u.glass.scope = scope; /* NULL is valid */
  return result;
}

struct Statement *CreateEXTERNBlackBox(
			       struct Name *n, CONST char *funcname,
			       struct VariableList *vl,
			       struct Name *data,
			       unsigned long n_inputs,
			       unsigned long n_outputs)
{
  register struct Statement *result;
  struct Name *bbsuffix;
  symchar *bsuf;
  /* ERROR_REPORTER_DEBUG("Found blackbox equation statement '%s'\n",funcname); */

  result=create_statement_here(EXT);
  result->v.ext.mode = ek_black;
  result->v.ext.extcall = funcname;
  /* bbox names are indexed bbox{[optionalUserFORIndices]}[?BBOX_OUTPUT].
	The last index is always ?BBOX_OUTPUT and there is never a case
	where there can be two such indices, by bbox definition.
	We could call the symbol ?* instead, but in case another similar
	internal compiler usage pops up (likely) we like the informative name.
   */
  /* name of the bbox implicit int set */
  bsuf = AddSymbol(BBOX_RESERVED_INDEX);
  bbsuffix = CreateReservedIndexName(bsuf); /* add a [?BBOX_OUTPUT] index */
  n = JoinNames(n, bbsuffix);
  result->v.ext.u.black.nptr = n;
  result->v.ext.u.black.vl = vl;
  result->v.ext.u.black.data = data;          /* NULL is valid */
  result->v.ext.u.black.n_inputs = n_inputs;  //number of inputs from parsed statement
  result->v.ext.u.black.n_outputs = n_outputs;//number of outputs from parsed statement
  return result;
}

struct Statement *CreateEXTERNMethod(
			       CONST char *funcname,
			       struct VariableList *vl)
{
  register struct Statement *result;
  /* ERROR_REPORTER_DEBUG("Found external method statement '%s'\n",funcname); */
  result=create_statement_here(EXT);
  result->v.ext.mode = ek_method;
  result->v.ext.extcall = funcname;
  result->v.ext.u.method.vl = vl;
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
  result->v.r.type_name = type_access; /* NULL is valid */
  return result;
}

struct Statement *CreateCALL(symchar *sym,struct Set *args)
{
  register struct Statement *result;
  result=create_statement_here(CALL);
  result->v.call.id = sym;
  result->v.call.args = args; /* NULL is valid */
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
      case LNK:
      case UNLNK:
        DestroyVariableList(s->v.lnk.vl);
        s->v.lnk.vl = NULL;
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
        s->v.ext.extcall = NULL;
        switch (s->v.ext.mode) {
        case ek_method:
          DestroyVariableList(s->v.ext.u.method.vl);
          s->v.ext.u.method.vl = NULL;
          break;
        case ek_glass:
          DestroyName(s->v.ext.u.glass.nptr);
          s->v.ext.u.glass.nptr = NULL;
          DestroyVariableList(s->v.ext.u.glass.vl);
          s->v.ext.u.glass.vl = NULL;
          if (s->v.ext.u.glass.data) DestroyName(s->v.ext.u.glass.data);
          s->v.ext.u.glass.data = NULL;
          if (s->v.ext.u.glass.scope) DestroyName(s->v.ext.u.glass.scope);
          s->v.ext.u.glass.scope = NULL;
          break;
        case ek_black:
          DestroyName(s->v.ext.u.black.nptr);
          s->v.ext.u.black.nptr = NULL;
          DestroyVariableList(s->v.ext.u.black.vl);
          s->v.ext.u.black.vl = NULL;
          if (s->v.ext.u.black.data) DestroyName(s->v.ext.u.black.data);
          s->v.ext.u.black.data = NULL;
          break;
        }
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

      case SOLVER:
        s->v.solver.name = NULL;
        break;

      case OPTION:
        s->v.option.name = NULL;
        DestroyExprList(s->v.option.rhs);
        break;

      case SOLVE:
        /* currently there's no data stored in this command */
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
    result->stringform = ASC_NEW_ARRAY(char,size+1);
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
  case UNLNK:
  case LNK:
    result->v.lnk.key = s->v.lnk.key;
    result->v.lnk.vl = CopyVariableList(s->v.lnk.vl);
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
    result->v.ext.extcall = s->v.ext.extcall;
    switch (s->v.ext.mode) {
    case ek_glass:
      result->v.ext.u.glass.nptr = CopyName(s->v.ext.u.glass.nptr);
      result->v.ext.u.glass.vl = CopyVariableList(s->v.ext.u.glass.vl);
      result->v.ext.u.glass.data = CopyName(s->v.ext.u.glass.data);
      result->v.ext.u.glass.scope = CopyName(s->v.ext.u.glass.scope);
      break;
    case ek_black:
      result->v.ext.u.black.nptr = CopyName(s->v.ext.u.black.nptr);
      result->v.ext.u.black.vl = CopyVariableList(s->v.ext.u.black.vl);
      result->v.ext.u.black.data = CopyName(s->v.ext.u.black.data);
      break;
    case ek_method:
      result->v.ext.u.method.vl = CopyVariableList(s->v.ext.u.method.vl);
      break;
    }
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

  case FIX:
  case FREE:
    result->v.fx.vars = CopyVariableList(s->v.fx.vars);
    break;

  case SOLVER:
    result->v.solver.name = s->v.solver.name;
    break;

  case OPTION:
    result->v.option.name = s->v.option.name;
    result->v.option.rhs = CopyExprList(s->v.option.rhs);
    break;

  case SOLVE:
    /* no data to be copied for this command */
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
  case LNK:
  case UNLNK:
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
  case SOLVER:
  case OPTION:
  case SOLVE:
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
  case LNK:
  case UNLNK:
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
  case SOLVER:
  case OPTION:
  case SOLVE:
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
  case LNK:
  case UNLNK:
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
  case SOLVER:
  case OPTION:
  case SOLVE:
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
		(s->t==LNK)  ||
		(s->t==UNLNK) ||
		(s->t==ATS)  ||
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
  case LNK:
  case UNLNK:
    return (s)->v.lnk.vl;
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

symchar *LINKStatKeyF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==LNK || s->t==UNLNK);
  return s->v.lnk.key;
}

struct VariableList *LINKStatVlistF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==LNK || s->t==UNLNK);
  return s->v.lnk.vl;
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

unsigned ForContainsExternalF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_EXT);
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

unsigned ForContainsLinkF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_LNK);
}

unsigned ForContainsUnlinkF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==FOR);
  return (s->v.f.contains & contains_UNLNK);
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

enum ExternalKind ExternalStatModeF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  return(s->v.ext.mode);
}

struct Name *ExternalStatNameRelationF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  assert(s->v.ext.mode == ek_glass || s->v.ext.mode == ek_black);
  return(s->v.ext.u.relation.nptr);
}

struct Name *ExternalStatDataBlackBoxF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  assert(s->v.ext.mode == ek_black);
  return(s->v.ext.u.black.data);
}

struct Name *ExternalStatDataGlassBoxF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  assert(s->v.ext.mode == ek_glass);
  return(s->v.ext.u.glass.data);
}

struct Name *ExternalStatScopeGlassBoxF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  assert(s->v.ext.mode == ek_glass);
  return(s->v.ext.u.glass.scope);
}

CONST struct VariableList *ExternalStatVlistRelationF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  assert(s->v.ext.mode == ek_black || s->v.ext.mode == ek_glass);
  return(s->v.ext.u.relation.vl);
}

CONST struct VariableList *ExternalStatVlistMethodF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==EXT);
  assert(s->v.ext.mode == ek_method);
  return(s->v.ext.u.method.vl);
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

unsigned CondContainsExternalF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==COND);
  return (s->v.cond.contains & contains_EXT);
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

unsigned SelectContainsExternalF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_EXT);
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

unsigned SelectContainsLinkF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_LNK);
}

unsigned SelectContainsUnlinkF(CONST struct Statement *s)
{
  assert(s!=NULL);
  assert(s->t==SELECT);
  return (s->v.se.contains & contains_UNLNK);
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
  case LNK: /* fallthru */ /* FIXME check this? */
  case UNLNK: /* fallthru */
    CONSOLE_DEBUG("CHECK HERE! don't we also need to check the TYPE of link?");
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
    if (ExternalStatMode(s1) == ek_glass || ExternalStatMode(s1) == ek_black) {
      ctmp = CompareNames(ExternalStatNameRelation(s1),ExternalStatNameRelation(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    if (ExternalStatMode(s1) == ek_glass) {
      ctmp = CompareNames(ExternalStatScope(s1),ExternalStatScope(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    if (ExternalStatMode(s1) == ek_glass) {
      ctmp = CompareNames(ExternalStatDataGlassBox(s1),ExternalStatDataGlassBox(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    if (ExternalStatMode(s1) == ek_black) {
      ctmp = CompareNames(ExternalStatDataBlackBox(s1),ExternalStatDataBlackBox(s2));
      if (ctmp != 0) {
        return ctmp;
      }
    }
    switch (ExternalStatMode(s1)) {
    case ek_method:
      return CompareVariableLists(ExternalStatVlistMethod(s1),ExternalStatVlistMethod(s2));
    case ek_glass:
    case ek_black:
      return CompareVariableLists(ExternalStatVlistRelation(s1),ExternalStatVlistRelation(s2));
    default:
      return 1;
    }
  case REF:
    /* need fixing. since both are ill defined, they
     * will not be fixed until the definitions are really known.
     */
    ASC_PANIC("Don't know how to compare REF statements\n");

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
 *static functions? baa. FIXME.
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
  case LNK:
  case UNLNK:
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

/* vim: set ts=8: */

