/* ex:set ts=8: */
/*
 *  Initialization Routines
 *  by Tom Epperly
 *  Created: 3/24/1990
 *  Version: $Revision: 1.36 $
 *  Version control file: $RCSfile: initialize.c,v $
 *  Date last modified: $Date: 1998/06/11 15:28:30 $
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

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "symtab.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "forvars.h"
#include "name.h"
#include "find.h"
#include "vlist.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "stattypes.h"
#include "statement.h"
#include "statio.h"
#include "switch.h"
#include "module.h"
#include "evaluate.h"
#include "value_type.h"
#include "setinstval.h"
#include "extfunc.h"
#include "packages.h"
#include "instance_io.h"
#include "nameio.h"
#include "atomvalue.h"
#include "instquery.h"
#include "slist.h"
#include "child.h"
#include "type_desc.h"
#include "library.h"
#include "extcall.h"
#include "proc.h"
#include "watchpt.h"
#include "procframe.h"
#include "procio.h"
#include "initialize.h"
#include "switch.h"
#include "exprs.h"
#include "sets.h"
#include "parentchild.h"


#ifndef lint
static CONST char InitializeRCSid[]="$Id: initialize.c,v 1.36 1998/06/11 15:28:30 ballan Exp $";
#endif /* lint */

/* set to 1 for tracing execution the hard way. */
#define IDB 0

/*********************************************************************\
  There is a stack of procedure calls kept for tracing and breaking
  recursion errors.
  INITSTACKLIMIT is the minimum we will allow internally.
  This is independent of the procframes until we get those
  solidly cleaned up.
\*********************************************************************/

static
struct {
  unsigned long limit;
  unsigned long depth;
} g_proc = {INITSTACKLIMIT,0L};

unsigned long GetProcStackLimit(void)
{
  return g_proc.limit;
}

void SetProcStackLimit(unsigned long lim)
{
  if (lim < 3) {
    FPRINTF(ASCERR,
      "SetProcStackLimit called with limit too small (%lu). Ignored.\n",lim);
    return;
  }
  if (g_proc.depth) {
    FPRINTF(ASCERR, "SetProcStackLimit called during evaluation. Ignored.\n");
    return;
  }
  g_proc.limit = lim;
  return;
}

/* The following 2 forward declarations have been moved out of the
 * header, where they had no business being, so we can adequately
 * guard against recursive functions.
 * static void ExecuteInitRun(struct procFrame *, struct Statement *);
 * static void ExecuteInitProcedure(struct procFrame *,
 *                                  struct InitProcedure *);
 */


static void ExecuteInitStatements(struct procFrame *,struct StatementList *);
static void RealInitialize(struct procFrame *, struct Name *);
static void ClassAccessRealInitialize(struct procFrame *, struct Name *, struct Name *);

/* just forward declarations cause we need it */

/*
 * modifies the name given to it, if needed shortening it.
 * If shortening, destroys the cut off part.
 */
static
void InstanceNamePart(struct Name *n, struct Name **copy,
                      symchar **procname)
{
  register struct Name *ptr,*tmp;

  /*FPRINTF(ASCERR,"INSTANCE NAME PART, input is n=");
  WriteName(ASCERR,n);
  FPRINTF(ASCERR,"\n");
   */

  if (n==NULL){
	  FPRINTF(ASCERR,"n IS NULL");
    *copy = NULL;
    *procname = NULL;
    return;
  }
  if (NextName(n)==NULL) {	/* RUN a; a is the procname */
    *copy = NULL;
    if (NameId(n) != 0) {
      *procname = NameIdPtr(n);
    } else {
      *procname = NULL;
    }
  } else {
    /* RUN a.b.c.clear; clear is the procname */
    ptr = *copy = CopyName(n);
    while (NextName(NextName(ptr))!=NULL) {
      ptr = NextName(ptr);
    }
    tmp = NextName(ptr);
    LinkNames(ptr,NULL);	/* disconnect last part of name */
    if (NameId(tmp) != 0) {
      *procname = NameIdPtr(tmp);
    } else {
      *procname = NULL;
    }
    DestroyName(tmp);
  }
}

struct InitProcedure *SearchProcList(CONST struct gl_list_t *l,
                                     symchar *name)
{
  register unsigned up,c,low;
  register struct InitProcedure *ptr;
  register int cmp;
  assert(AscFindSymbol(name)!=NULL);
  if (l == NULL) {
    return NULL;
  }
  up = gl_length(l);
  low = 1;
  while(low<=up){
    c = (low+up)/2;
    ptr = (struct InitProcedure *)gl_fetch(l,c);
    cmp = CmpSymchar(ProcName(ptr),name);
    if (cmp == 0) {
      return ptr;
    }
    if (cmp<0) {
      low = c+1;
    } else {
      up = c-1;
    }
  }
  return NULL;
}

struct InitProcedure *FindProcedure(CONST struct Instance *i,
        			    symchar *procname)
{
  struct TypeDescription *desc;
  struct gl_list_t *plist;
  struct InitProcedure *result = NULL;

  desc = InstanceTypeDesc(i);
  plist = GetInitializationList(desc);
  if (plist != NULL){
    result = SearchProcList(plist,procname);
  }
  plist = GetUniversalProcedureList();
  if (result == NULL && plist != NULL) {
    /* try for a UNIVERSAL method seen since parsing MODEL of i */
    result = SearchProcList(plist,procname);
  }
  return result;
}


/*********************************************************************\
 * void ExecuteInitRun(fm,stat);
 * struct procFrame *fm;
 * struct InitProcedure *proc;
 * This will execute a run statement, using the given instance as the
 * context. stat must be a RUN statement. In the event of error will
 * print appropriate messages to stderr.
\*********************************************************************/
/*
 * This returns proc_all_ok in all circumstances except stack overflow.
 * If within it any other error occurs, it prints the message and
 * then pretends everything is ok.
 * This behavior should perhaps be better.
 */
static
void ExecuteInitRun(struct procFrame *fm, struct Statement *stat)
{
  struct Name *typename;

  typename = RunStatAccess(stat);
  if (typename != NULL) {
    ClassAccessRealInitialize(fm,typename,RunStatName(stat));
  } else {
    RealInitialize(fm,RunStatName(stat));
  }
  /* an error was encountered */
  if (fm->flow == FrameError) {
    ProcWriteRunError(fm);
  }
}

/**
	Shared function for FIX and FREE execution
	@param val is TRUE for 'FIX', or FALSE for 'FREE'.
*/
static void
execute_init_fix_or_free(int val, struct procFrame *fm, struct Statement *stat){
  CONST struct VariableList *vars;
  enum find_errors e;
  struct gl_list_t *temp;
  unsigned i, len;
  struct Instance *i1, *i2;
  char *instname;
  struct TypeDescription *t, *st;
  CONST struct Name *name;
  symchar *fixed;
  /* setup */
  fixed = AddSymbol("fixed");
  st = FindType(AddSymbol("solver_var"));
  if(st==NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"'solver_var' type is not yet in library");
	fm->ErrNo = Proc_type_not_found;
    return;
  }

  /* iterate through the variable list */
  /*CONSOLE_DEBUG("STARTING 'FIX'/'FREE' STATEMENT EXECUTION");*/
  vars = stat->v.fx.vars;
  while(vars!=NULL){
    name = NamePointer(vars);
    temp = FindInstances(fm->i, name, &e);
    if(temp==NULL){
	  fm->ErrNo = Proc_bad_name;
      return;
    }
    len = gl_length(temp);
    for(i=1; i<=len; i++){
    	i1 = (struct Instance *)gl_fetch(temp,i);
	instname = WriteInstanceNameString(i1,NULL);
	/*if(val){
		CONSOLE_DEBUG("ABOUT TO FIX %s",instname);
	}else{
		CONSOLE_DEBUG("ABOUT TO FREE %s",instname);
	}*/		
	ascfree(instname);
	if(InstanceKind(i1)!=REAL_ATOM_INST){
	  fm->ErrNo = Proc_illegal_type_use;
	  ProcWriteFixError(fm,name);
	  return;
	}
	t = InstanceTypeDesc(i1);
	if(!MoreRefined(t,st)){
	  CONSOLE_DEBUG("Attempted to FIX or FREE variable that is not a refined solver_var.");
	  fm->ErrNo = Proc_illegal_type_use;
	  ProcWriteFixError(fm,name);
	  return;
	}
	i2 = ChildByChar(i1,fixed);
	if(i2==NULL){
	  CONSOLE_DEBUG("Attempted to FIX or FREE a solver_var that doesn't have a 'fixed' child!");
	  fm->ErrNo = Proc_illegal_type_use;
	  ProcWriteFixError(fm,name);
	  return;
	}
	if(InstanceKind(i2)!=BOOLEAN_INST){
	  CONSOLE_DEBUG("Attempted to FIX or FREE a solver_var whose 'fixed' child is not boolean!");
	  fm->ErrNo = Proc_illegal_type_use;
	  ProcWriteFixError(fm,name);
	  return;
	}
	SetBooleanAtomValue(i2,val,0);
    }
    vars = NextVariableNode(vars);
  }
  /* CONSOLE_DEBUG("DONE WITH VARLIST"); */

  /* return 'ok' */
  fm->ErrNo = Proc_all_ok;
}

static void
ExecuteInitFix(struct procFrame *fm, struct Statement *stat){
	execute_init_fix_or_free(TRUE,fm,stat);
}

static void
ExecuteInitFree(struct procFrame *fm, struct Statement *stat){
	execute_init_fix_or_free(FALSE,fm,stat);
}


static
void ExecuteInitFlow(struct procFrame *fm)
{
  assert(fm!=NULL);
  assert(fm->stat!=NULL);
  assert(StatementType(fm->stat)==FLOW);
  switch (FlowStatControl(fm->stat)) {
  case fc_break:
    fm->ErrNo = Proc_break;
    fm->flow = FrameBreak;
    break;
  case fc_continue:
    fm->ErrNo = Proc_continue;
    fm->flow = FrameContinue;
    break;
  case fc_fallthru:
    fm->ErrNo = Proc_fallthru;
    fm->flow = FrameFallthru;
    break;
  case fc_return:
    fm->ErrNo = Proc_return;
    fm->flow = FrameReturn; /* needs to be caught automagically to frameok
                             * if errno is proc_return.
                             */
    break;
  case fc_stop:
    fm->ErrNo = Proc_stop;
    fm->flow = FrameError;
    ProcWriteIfError(fm,"STOP");
    break;
  default:
    break;
  }
}

/**
	The following functions have been made static as they are very similar to those used in instantiate.c. They really should be rationalized and exported by instantiate.c. As usual, any function with Special in the name is written by KAA.
 */
#define SELF_NAME "SELF"

static
int SpecialSelfName(CONST struct Name *n)
{
  symchar *id;
  if (n == NULL) {
    return 0;
  }
  id = SimpleNameIdPtr(n);
  if (id == NULL) {
    return 0;
  }
  if (strcmp(SCP(id),SELF_NAME)==0) {
    return 1;
  } else {
    return 0;
  }
}

/**
	Produces a list of lists of argument instances. a the list returned is never NULL except when out of memory. Entries in this list may be NULL if some argument search fails. Argument search is successful IFF errlist returned is empty (length 0).
 */
static
struct gl_list_t *ProcessArgs(struct Instance *inst,
                              CONST struct VariableList *vl,
                              struct gl_list_t *errlist)
{
  struct gl_list_t *arglist;
  struct gl_list_t *branch;
  CONST struct Name *n;
  enum find_errors ferr;
  unsigned long pos;

  ListMode=1;
  arglist = gl_create(10L);
  pos = 1;
  while(vl!=NULL){
    n = NamePointer(vl);
    ferr = correct_instance;
    branch = FindInstances(inst,n,&ferr);
    if (branch == NULL || ferr != correct_instance) {
      /* check for SELF only if find fails, so SELF IS_A foo
       * overrides the normal self.
       */
      if (SpecialSelfName(n)) {
        if (branch == NULL) {
          branch = gl_create(1L);
        } else {
          gl_reset(branch);
        }
        /* Self referential instance */
        gl_append_ptr(branch,(VOIDPTR)inst);
      } else {
        gl_append_ptr(errlist,(VOIDPTR)pos); /* error position */
        gl_append_ptr(errlist,(VOIDPTR)ferr); /* error code */
        if (branch == NULL) {
          branch = gl_create(1L); /* create empty branch */
        }
      }
    }
    assert(branch != NULL);
    gl_append_ptr(arglist,(VOIDPTR)branch);
    vl = NextVariableNode(vl);
    pos++;
  }
  ListMode=0;
  return arglist;
}

static
struct gl_list_t *InitCheckExtCallArgs(struct Instance *inst,
                                       struct Statement *stat,
                                       struct gl_list_t *errs)
{
  struct VariableList *vl;
  struct gl_list_t *result;

  vl = ExternalStatVlist(stat);
  result = ProcessArgs(inst,vl,errs);
  return result;
}

static
void ExecuteInitCall(struct procFrame *fm, struct Statement *stat)
{
  (void)fm;     /*  stop gcc whine about unused parameter  */
  (void)stat;  /*  stop gcc whine about unused parameter  */
#if 0 /* guts of CALL statement execution need coding. */
/* something like ExecuteInitExt only string driven gllist argument
 * translation +/- varargs BS, etc, etc
 * Get rid of that awfully misnamed SlvInterp at any rate.
 */
#endif
}

/*
 * This always returns ok. at least as of 5/96.
 */
static
void ExecuteInitExt(struct procFrame *fm, struct Statement *stat)
{
  struct ExternalFunc *efunc;
  CONST char *funcname;
  struct gl_list_t *arglist=NULL, *errlist;
  enum find_errors ferr;
  unsigned long c,len,pos;

  struct Slv_Interp slv_interp;
  int (*eval_func) ();
  int nok;

  funcname = ExternalStatFuncName(stat);
  efunc = LookupExtFunc(funcname);

  /*CONSOLE_DEBUG("EXECUTEINITEXT func name:'%s'",funcname);*/

  if (efunc == NULL) {
    CONSOLE_DEBUG("Failed to look up external function");
    fm->ErrNo = Proc_CallError;
    fm->flow = FrameError;
    ProcWriteExtError(fm,funcname,PE_unloaded,0);
    return;
  }

  CONSOLE_DEBUG("%s: in:%ld, out:%ld", efunc->name, efunc->n_inputs, efunc->n_outputs);

  eval_func = GetValueFunc(efunc);
  if (eval_func == NULL) {
    CONSOLE_DEBUG("GetValueFunc(efunc) returned NULL");
    fm->ErrNo = Proc_CallError;
    fm->flow = FrameError;
    ProcWriteExtError(fm,funcname,PE_nulleval,0);
    return;
  }

  errlist = gl_create(1);
  arglist = InitCheckExtCallArgs(fm->i,stat,errlist);
  len = gl_length(errlist);
  if (len != 0) {
    CONSOLE_DEBUG("InitCheckExtCallArgs returned items in errlist...");
    fm->flow = FrameError;
    ProcWriteExtError(fm,funcname,PE_argswrong,0);
    c = 1;
    assert((len & 0x1) == 0); /* must be even */
    while (c < len) {
      /* works because error position/code pairs */
      pos = (unsigned long)gl_fetch(errlist,c);
      c++;	/* Wait, who let that dirty word in here!? */
      ferr = (enum find_errors)gl_fetch(errlist,c);
      c++;
      switch (ferr) {
      case unmade_instance:
        fm->ErrNo = Proc_instance_not_found;
        ProcWriteExtError(fm,funcname,PE_badarg,(int)pos);
        break;
      case undefined_instance:
        fm->ErrNo = Proc_name_not_found;
        ProcWriteExtError(fm,funcname,PE_badarg,(int)pos);
        break;
      case impossible_instance:
        fm->ErrNo = Proc_illegal_name_use;
        ProcWriteExtError(fm,funcname,PE_badarg,(int)pos);
        break; /* move write to procio */
      case correct_instance:
        fm->ErrNo = Proc_CallError;
        ProcWriteExtError(fm,funcname,PE_badarg,(int)pos);
        break;
      default:
        fm->ErrNo = Proc_bad_name;
        ProcWriteExtError(fm,funcname,PE_badarg,(int)pos);
        break;
      }
    }
    fm->ErrNo = Proc_CallError;
    if (arglist != NULL) {
      DestroySpecialList(arglist);
    }
    if (errlist != NULL) {
      gl_destroy(errlist);
    }
    return;
  }

  /*CONSOLE_DEBUG("CHECKED EXTERNAL ARGS, OK");*/

  Init_Slv_Interp(&slv_interp);
  nok = (*eval_func)(&slv_interp,fm->i,arglist);
  /* this should switch on Proc_CallXXXXX */
    /* should switch on proc_enum call bits to translate Proc_Call
     * flow of control to our fm->flow.
     */
  if (nok) {
    fm->flow = FrameError; /* move write to procio */
	ERROR_REPORTER_HERE(ASC_USER_NOTE,"NOK");
    ProcWriteExtError(fm,funcname,PE_evalerr,0);
  } else {
    fm->flow = FrameOK;
  }
  if (arglist != NULL) {
    DestroySpecialList(arglist);
  }
  if (errlist != NULL) {
    gl_destroy(errlist);
  }

  return;
}

/*
 * executes a for loop
 */
static
void ExecuteInitFor(struct procFrame *fm, struct Statement *stat)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  int direction;        /* was declared unsigned long, but used as int (JDS 12/11/2005) */
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  enum FrameControl oldflow;

  c = direction = 1; /* shut up gcc */

  name = ForStatIndex(stat);
  ex = ForStatExpr(stat);
  sl = ForStatStmts(stat);
  fv = FindForVar(GetEvaluationForTable(),name);
  if (fv != NULL) { /* duplicated for variable */
    fm->flow = FrameError;
    fm->ErrNo = Proc_for_duplicate_index;
    ProcWriteForError(fm);
    return;
  }
  assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(fm->i);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    fm->flow = FrameError;
    fm->ErrNo = Proc_for_set_err;
    ProcWriteForError(fm);
    break;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set:
      break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      switch(ForLoopOrder(stat)){
      case f_random:
        /* fall through, that should never occur due to parser. */
      case f_increasing:
        direction = 1;
        c = 1;
        break;
      case f_decreasing:
        direction = -1;
        c = len;
        break;
      }
      /* we handle all logic with one for loop to avoid
       * duplicate code insanity.
       */
      oldflow = fm->flow;
      fm->flow = FrameLoop;
      for(/* init c in switch above */;
          c >= 1 && c <= len &&
          fm->flow != FrameBreak && fm->flow != FrameReturn;
          c += direction) {
        SetForInteger(fv,FetchIntMember(sptr,c));
        ExecuteInitStatements(fm,sl);
        switch (fm->flow) {
        case FrameOK:
        case FrameContinue:
          fm->flow = FrameLoop;
          break;
        case FrameLoop:
        case FrameBreak:
        case FrameFallthru:
        case FrameReturn:
          break;
        case FrameError: /*EISS not to return this!*/
        default: /* should never happen */
#if IDB
FPRINTF(fm->err,"ERR-NEVER1: "); WriteStatement(fm->err,stat,0);
FPRINTF(fm->err,"\n");
#endif
          fm->flow = FrameReturn;
          break;
        }
      }
      /* post loop flow processing */
      switch (fm->flow) {
      case FrameLoop:
      case FrameBreak:
        fm->flow = oldflow;
        break;
      default:
        break; /* let return, fallthru out to next level */
      }
      RemoveForVariable(GetEvaluationForTable());
      break; /* integer_set */
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      switch(ForLoopOrder(stat)){
      case f_random:
        /* fall through, that should never occur due to parser. */
      case f_increasing:
        direction = 1;
        c = 1;
        break;
      case f_decreasing:
        direction = -1;
        c = len;
        break;
      }
      oldflow = fm->flow;
      fm->flow = FrameLoop;
      for(/* init c in switch above */;
          c >= 1 && c <= len &&
          fm->flow != FrameBreak && fm->flow != FrameReturn;
          c += direction) {
        SetForSymbol(fv,FetchStrMember(sptr,c));
        ExecuteInitStatements(fm,sl);
        switch (fm->flow) {
        case FrameOK:
        case FrameContinue:
          fm->flow = FrameLoop;
          break;
        case FrameLoop:
        case FrameBreak:
        case FrameReturn:
        case FrameFallthru:
          break;
        case FrameError: /*EISS not to return this!*/
        default: /* should never happen */
#if IDB
FPRINTF(fm->err,"ERR-NEVER2: "); WriteStatement(fm->err,stat,0);
FPRINTF(fm->err,"\n");
#endif
          fm->flow = FrameReturn;
          break;
        }
      }
      /* post loop flow processing */
      switch (fm->flow) {
      case FrameLoop:
      case FrameBreak:
        fm->flow = oldflow;
        break;
      default:
        break;
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    break;
  default:
    fm->flow = FrameError;
    fm->ErrNo = Proc_for_not_set;
    ProcWriteForError(fm);
    break;
  }
  DestroyValue(&value);
  return;
}

static void
ExecuteInitAssert(struct procFrame *fm, struct Statement *stat){
	struct value_t value;
	int testerr;
	assert(GetEvaluationContext()==NULL);
	SetEvaluationContext(fm->i);
	value = EvaluateExpr(AssertStatExpr(stat),NULL,InstanceEvaluateName);
	SetEvaluationContext(NULL);
	testerr = 1; /* set 0 on success */
	switch(ValueKind(value)){
		case boolean_value:
			testerr = 0;
			if(BooleanValue(value)){
				ERROR_REPORTER_STAT(ASC_USER_SUCCESS,stat,"Assertion OK");
			}else{
				ERROR_REPORTER_STAT(ASC_USER_ERROR,stat,"Assertion failed");
			}
			break;
		case real_value:
			fm->flow = FrameError;
			fm->ErrNo = Proc_if_real_expr;
			break;
		case integer_value:
			fm->flow = FrameError;
			fm->ErrNo = Proc_if_integer_expr;
			break;
		case symbol_value:
			fm->flow = FrameError;
			fm->ErrNo = Proc_if_symbol_expr;
			break;
		case set_value: /* FALLTHROUGH */
			case list_value:
			fm->flow = FrameError;
			fm->ErrNo = Proc_if_set_expr;
			break;
		case error_value:
			fm->flow = FrameError;
			fm->ErrNo = Proc_if_expr_error_confused;
			switch (ErrorValue(value)) {
				case type_conflict:
					fm->ErrNo = Proc_if_expr_error_typeconflict;
					break;
				case name_unfound:
					fm->ErrNo = Proc_if_expr_error_nameunfound;
					break;
				case incorrect_name:
					fm->ErrNo = Proc_if_expr_error_incorrectname;
					break;
				case undefined_value:
					fm->ErrNo = Proc_if_expr_error_undefinedvalue;
					break;
				case dimension_conflict:
					fm->ErrNo = Proc_if_expr_error_dimensionconflict;
					break;
				case empty_choice:
					fm->ErrNo = Proc_if_expr_error_emptychoice;
					break;
				case empty_intersection:
					fm->ErrNo = Proc_if_expr_error_emptyintersection;
					break;
				default:
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unhandled case");
			}
			break;
		default:
			fm->flow = FrameError;
			fm->ErrNo = Proc_if_not_logical;
			break;
	}
	if (fm->flow == FrameError && testerr) {
		ProcWriteIfError(fm,"TEST");
	}
	DestroyValue(&value);
	return;
}

static
void ExecuteInitIf(struct procFrame *fm, struct Statement *stat)
{
  struct value_t value;
  int iferr;

  assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(fm->i);
  value = EvaluateExpr(IfStatExpr(stat),NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  iferr = 1;		/* set 0 on success */
  switch(ValueKind(value)){
  case boolean_value:
    iferr = 0;
    if (BooleanValue(value)) {
      ExecuteInitStatements(fm,IfStatThen(stat));
    } else {
      if (IfStatElse(stat) != NULL) {
        ExecuteInitStatements(fm,IfStatElse(stat));
      }
    }
    break;
  case real_value:
    fm->flow = FrameError;
    fm->ErrNo = Proc_if_real_expr;
    break;
  case integer_value:
    fm->flow = FrameError;
    fm->ErrNo = Proc_if_integer_expr;
    break;
  case symbol_value:
    fm->flow = FrameError;
    fm->ErrNo = Proc_if_symbol_expr;
    break;
  case set_value: /* FALLTHROUGH */
  case list_value:
    fm->flow = FrameError;
    fm->ErrNo = Proc_if_set_expr;
    break;
  case error_value:
    fm->flow = FrameError;
    fm->ErrNo = Proc_if_expr_error_confused;
    switch (ErrorValue(value)) {
    case type_conflict:
      fm->ErrNo = Proc_if_expr_error_typeconflict;
      break;
    case name_unfound:
      fm->ErrNo = Proc_if_expr_error_nameunfound;
      break;
    case incorrect_name:
      fm->ErrNo = Proc_if_expr_error_incorrectname;
      break;
    case undefined_value:
      fm->ErrNo = Proc_if_expr_error_undefinedvalue;
      break;
    case dimension_conflict:
      fm->ErrNo = Proc_if_expr_error_dimensionconflict;
      break;
    case empty_choice:
      fm->ErrNo = Proc_if_expr_error_emptychoice;
      break;
    case empty_intersection:
      fm->ErrNo = Proc_if_expr_error_emptyintersection;
      break;
    default:
      break;
    }
    break;
  default:
    fm->flow = FrameError;
    fm->ErrNo = Proc_if_not_logical;
    break;
  }
  if (fm->flow == FrameError && iferr) {
    ProcWriteIfError(fm,"IF");
  }
  DestroyValue(&value);
  return;
}

/*
 */
static
void ExecuteInitWhile(struct procFrame *fm, struct Statement *stat)
{
  struct value_t value;
  int iferr;
  int stop;
  long limit = WP_MAXTRIPS;
  enum FrameControl oldflow;

  assert(GetEvaluationContext()==NULL);
  stop = 0;
  oldflow = fm->flow;
  fm->flow = FrameLoop;
  while (!stop) {
    assert(fm->flow == FrameLoop);
    SetEvaluationContext(fm->i);
    value = EvaluateExpr(WhileStatExpr(stat),NULL,InstanceEvaluateName);
    SetEvaluationContext(NULL);
    iferr = 1;		/* set 0 on success */
    limit--;
    switch(ValueKind(value)){
    case boolean_value:
      iferr = 0;
      if (BooleanValue(value)) {
        ExecuteInitStatements(fm,WhileStatBlock(stat));
        switch (fm->flow) {
        case FrameOK:
        case FrameContinue:
          fm->flow = FrameLoop;
          break;
        case FrameLoop:
          break;
        case FrameBreak: /* break while loop only */
        case FrameFallthru: /* while must be inside switch...*/
        case FrameReturn:
          stop = 1;
          break;
        case FrameError: /* EISS is not supposed to let this happen */
        default: /* should never happen */
#if IDB
FPRINTF(fm->err,"ERR-NEVER3: "); WriteStatement(fm->err,stat,0);
FPRINTF(fm->err,"\n");
#endif
          fm->flow = FrameReturn;
          break;
        }
      } else {
        stop = 1;
      }
      break;
    case real_value:
      fm->flow = FrameError;
      fm->ErrNo = Proc_if_real_expr;
      break;
    case integer_value:
      fm->flow = FrameError;
      fm->ErrNo = Proc_if_integer_expr;
      break;
    case symbol_value:
      fm->flow = FrameError;
      fm->ErrNo = Proc_if_symbol_expr;
      break;
    case set_value: /* FALLTHROUGH */
    case list_value:
      fm->flow = FrameError;
      fm->ErrNo = Proc_if_set_expr;
      break;
    case error_value:
      fm->flow = FrameError;
      fm->ErrNo = Proc_if_expr_error_confused;
      switch (ErrorValue(value)) {
      case type_conflict:
        fm->ErrNo = Proc_if_expr_error_typeconflict;
        break;
      case name_unfound:
        fm->ErrNo = Proc_if_expr_error_nameunfound;
        break;
      case incorrect_name:
        fm->ErrNo = Proc_if_expr_error_incorrectname;
        break;
      case undefined_value:
        fm->ErrNo = Proc_if_expr_error_undefinedvalue;
        break;
      case dimension_conflict:
        fm->ErrNo = Proc_if_expr_error_dimensionconflict;
        break;
      case empty_choice:
        fm->ErrNo = Proc_if_expr_error_emptychoice;
        break;
      case empty_intersection:
        fm->ErrNo = Proc_if_expr_error_emptyintersection;
        break;
      default:
        break;
      }
      break;
    default:
      fm->flow = FrameError;
      fm->ErrNo = Proc_if_not_logical;
      break;
    }
    if (fm->flow == FrameError && iferr) {
      ProcWriteIfError(fm,"WHILE");
    }
    DestroyValue(&value);
    if (limit < 0) {
      stop = 1;
      fm->flow = FrameError;
      fm->ErrNo = Proc_infinite_loop;
      ProcWriteIfError(fm,"WHILE");
    }
  } /* endwhile */
  /* post loop processing */
  switch (fm->flow) {
  case FrameLoop:
  case FrameBreak:
    fm->flow = oldflow;
    break;
  default: /* let return, fallthru, out to next scope */
    break;
  }
  return;
}


/*
 * Compare current values of the switching variables with
 * the set of values in a CASE of a SWITCH statement, and try to find
 * is such values are the same.
 * If they are, the function will return Proc_case_matched,
 * else, it will return Proc_case_unmatched unless there is an error.
 * The possible error returns are legion, and this function
 * handles issuing error messages for them.
 *
 * If s given is NULL AND arm is -1, simply verifies that vlist elements
 * exist/are assigned. Normally this is only of use in checking
 * the OTHERWISE branch of the switch.
 * s must NOT be NULL unless arm is -1.
 */
static
void AnalyzeSwitchCase(struct procFrame *fm, struct VariableList *vlist,
                       struct Set *s, int arm)
{
  CONST struct Expr *expr;
  CONST struct Name *name;
  symchar *value;
  symchar *symvar;
  CONST struct VariableList *vl;
  CONST struct Set *values;
  int val;
  int pos;
  int valvar;
  struct gl_list_t *instances;
  struct Instance *inst;
  enum find_errors err;
  symchar *str;
  struct for_var_t *fvp;

  assert(vlist != NULL);
  vl = vlist;
  fm->ErrNo = Proc_case_matched;
  pos = 0;
  if (s==NULL && arm == -1) {
    /* check vlist only */
    while (vl!=NULL) {
      pos++;
      name = NamePointer(vl);
      instances = FindInstances(fm->i,name,&err);
      if (instances == NULL){
        switch (err) {
        case unmade_instance:
          fm->ErrNo = Proc_instance_not_found;
          break;
        case undefined_instance:
          fm->ErrNo = Proc_name_not_found;
          break;
        case impossible_instance:
          fm->ErrNo = Proc_illegal_name_use;
          break;
        case correct_instance:
          fm->ErrNo = Proc_CallError;
          break;
        }
      }
      if (gl_length(instances)==1) {
        inst = (struct Instance *)gl_fetch(instances,1);
        gl_destroy(instances);
        if (!AtomAssigned(inst)) {
          fm->ErrNo = Proc_case_undefined_value;
          break; /* while */
        }
      } else {
        fm->ErrNo = Proc_case_extra_values;
        gl_destroy(instances);
        break; /* while */
      }
      vl = NextVariableNode(vl);
    }
    if (fm->ErrNo != Proc_case_matched) {
      ProcWriteCaseError(fm,arm,pos);
    }
    fm->flow = FrameError;
    return;
  }

  assert(s!= NULL);
  values = s;

  while (vl!=NULL) {
    pos++;
    name = NamePointer(vl);
    expr = GetSingleExpr(values);
    instances = FindInstances(fm->i,name,&err);
    if (instances == NULL){
      switch (err) {
      case unmade_instance:
        fm->ErrNo = Proc_instance_not_found;
        break;
      case undefined_instance:
        fm->ErrNo = Proc_name_not_found;
        break;
      case impossible_instance:
        fm->ErrNo = Proc_illegal_name_use;
        break;
      case correct_instance:
        fm->ErrNo = Proc_CallError; /* move write to procio */
        break;
      }
    } else {
      if (gl_length(instances)==1) {
        inst = (struct Instance *)gl_fetch(instances,1);
        gl_destroy(instances);
        if (!AtomAssigned(inst)) {
          fm->ErrNo = Proc_case_undefined_value;
          break;
        }
        switch(ExprType(expr)) {
        case e_boolean:
          if ((InstanceKind(inst) & IBOOL) == 0) {
            fm->ErrNo = Proc_case_boolean_mismatch;
            break;
          }
          val =  ExprBValue(expr);
          if (val == 2) { /* ANY */
            break;
          }
          valvar = GetBooleanAtomValue(inst);
          if (val != valvar) {
            fm->ErrNo = Proc_case_unmatched;
          }
          break;
        case e_int:
          if ((InstanceKind(inst) & IINT) == 0) {
            fm->ErrNo = Proc_case_integer_mismatch;
            break;
          }
          val =  ExprIValue(expr);
          valvar = GetIntegerAtomValue(inst);
          if (val != valvar) {
            fm->ErrNo = Proc_case_unmatched;
          }
          break;
        case e_symbol:
          if ((InstanceKind(inst) & ISYM) == 0) {
            fm->ErrNo = Proc_case_symbol_mismatch;
            break;
          }
          symvar = ExprSymValue(expr);
          value = GetSymbolAtomValue(inst);
          assert(AscFindSymbol(symvar)!=NULL);
          assert(AscFindSymbol(value)!=NULL);
          if (symvar != value) {
            fm->ErrNo = Proc_case_unmatched;
          }
          break;
        case e_var:
          /* evar ok only if a loop index? */
          if ((GetEvaluationForTable() != NULL) &&
              (NULL != (str = SimpleNameIdPtr(ExprName(expr)))) &&
              (NULL != (fvp=FindForVar(GetEvaluationForTable(),str)))) {
            switch (GetForKind(fvp)) {
            case f_integer:
              if ((InstanceKind(inst) & IINT) == 0) {
                fm->ErrNo = Proc_case_integer_mismatch;
                break;
              }
              val = GetForInteger(fvp);
              valvar = GetIntegerAtomValue(inst);
              if (val != valvar) {
                fm->ErrNo = Proc_case_unmatched;
              }
              break;
            case f_symbol:
              if ((InstanceKind(inst) & ISYM) == 0) {
                fm->ErrNo = Proc_case_symbol_mismatch;
                break;
              }
              symvar = GetForSymbol(fvp);
              value = GetSymbolAtomValue(inst);
              if (symvar != value) {
                fm->ErrNo = Proc_case_unmatched;
              }
              break;
            default:
              fm->ErrNo = Proc_case_wrong_index;
              break;
            }
          } else {
            fm->ErrNo = Proc_case_wrong_index;
          }
          break;
        default:
          fm->ErrNo = Proc_case_wrong_value;
        }
      } else {
        gl_destroy(instances);
        fm->ErrNo = Proc_case_extra_values;
      }
    }
    if (fm->ErrNo != Proc_case_matched) {
      break;
    }
    vl = NextVariableNode(vl);
    values = NextSet(values);
  }
  if (fm->ErrNo != Proc_case_matched && fm->ErrNo != Proc_case_unmatched) {
    ProcWriteCaseError(fm,arm,pos);
    fm->flow = FrameError;
  }
  return;
}

/* This function will determine which case of a SWITCH statement
 * applies for the current values of the switching variables.
 * this function  will call for the execution of the cases which
 * match. It handles OTHERWISE properly (case when set == NULL).
 */

static
void ExecuteInitSwitch(struct procFrame *fm, struct Statement *stat)
{
  struct VariableList *vlist;
  struct SwitchList *sw;
  struct Set *set;
  struct StatementList *sl;
  int arm;
  int case_match;
  int fallthru;
  enum FrameControl oldflow;

  vlist = SwitchStatVL(stat);
  sw = SwitchStatCases(stat);
  case_match = 0;

  arm = 0;
  oldflow = fm->flow;
  while (sw!=NULL) { /* && notbreak. fixme */
    arm++;
    set = SwitchSetList(sw);
    sl = SwitchStatementList(sw);
    if (set != NULL) {
      AnalyzeSwitchCase(fm,vlist,set,arm); /*add fallthru arg */
      switch (fm->ErrNo) {
      case Proc_case_matched:
        case_match++;
        /* could put fallthru handling here if in grammar */
        fm->ErrNo = Proc_all_ok;
        fm->flow = FrameLoop;
        ExecuteInitStatements(fm,sl);
        switch (fm->flow) {
        case FrameLoop:
        case FrameOK:
          fm->flow = oldflow;
          fallthru = 0;
          break;
        case FrameReturn:
          return;
        case FrameBreak: /* not properly implemented. fixme */
          fallthru = 0;
          break;
        case FrameContinue:
          if (oldflow == FrameLoop) {
            return;
          }
          break;
        case FrameFallthru: /* not implemented */
          fallthru = 1;
        case FrameError: /* EISS not supposed to return this */
        default:
          break;
        }
        break;
      case Proc_case_unmatched:
        break;
      default:
        /* fixme policy might suppress error return */
        fm->flow = FrameError;
        return;
      }
    } else {
      /* OTHERWISE arm, which we seem to be assuming comes last */
      if (!case_match) {
        AnalyzeSwitchCase(fm,vlist,NULL,-1);
        if (fm->ErrNo == Proc_case_matched) {
          fm->ErrNo = Proc_all_ok;
          ExecuteInitStatements(fm,sl);
          case_match = 1;
          if (fm->ErrNo != Proc_all_ok) {
            /* fixme logic */
            WriteInitErr(fm,"Error in execution of SWITCH statements\n");
            break;
          }
        }
      }
    }
    sw = NextSwitchCase(sw);
  }
  if (case_match == 0) {
    WriteInitWarn(fm,"No case matched in SWITCH statement\n");
  }
  return;
}

/* i is generally NOT fm->i, but in the scope of fm->i */
static
void AssignInitValue(struct Instance *i, struct value_t v, struct procFrame *fm)
{
  CONST dim_type *dim;
  int assignerr = 1; /* set 0 on success */
  switch(InstanceKind(i)) {
  case MODEL_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
  case REL_INST:
    fm->ErrNo = Proc_nonatom_assignment;
    fm->flow = FrameError;
    break;
  case DUMMY_INST:
    /* cpp string concatenation */
    assignerr = 0;
    WriteInitWarn(fm,"Assignment to an unSELECTed_part ignored."
                      "SELECT should be shadowed by SWITCH in METHODS");
    break;
  case INTEGER_INST:
  case INTEGER_ATOM_INST:
    if (ValueKind(v)!=integer_value){
      fm->ErrNo = Proc_noninteger_assignment;
      fm->flow = FrameError;
    } else {
      assignerr = 0;
      SetIntegerAtomValue(i,IntegerValue(v),0);
    }
    break;
  case SET_INST:
  case SET_ATOM_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
    fm->ErrNo = Proc_declarative_constant_assignment;
    fm->flow = FrameError;
    break;
  case REAL_INST:
  case REAL_ATOM_INST:
    switch(ValueKind(v)){
    case real_value:
      dim = CheckDimensionsMatch(RealValueDimensions(v),RealAtomDims(i));
      if (dim==NULL){
        fm->ErrNo = Proc_nonconsistent_assignment;
        fm->flow = FrameError;
      } else {
        assignerr = 0;
        if (dim!=RealAtomDims(i)) {
          SetRealAtomDims(i,dim);
        }
        SetRealAtomValue(i,RealValue(v),0);
      }
      break;
    case integer_value:
      dim = CheckDimensionsMatch(Dimensionless(),RealAtomDims(i));
      if (dim==NULL){
        fm->ErrNo = Proc_nonconsistent_assignment;
        fm->flow = FrameError;
      } else {
        assignerr = 0;
        if (dim != RealAtomDims(i)) {
          SetRealAtomDims(i,dim);
        }
        SetRealAtomValue(i,(double)IntegerValue(v),0);
      }
      break;
    default:
      fm->ErrNo = Proc_nonreal_assignment;
      fm->flow = FrameError;
      break;
    }
    break;
  case BOOLEAN_INST:
  case BOOLEAN_ATOM_INST:
    if (ValueKind(v)!=boolean_value){
      fm->ErrNo = Proc_nonboolean_assignment;
      fm->flow = FrameError;
    } else {
      assignerr = 0;
      SetBooleanAtomValue(i,BooleanValue(v),0);
    }
    break;
  case SYMBOL_INST:
  case SYMBOL_ATOM_INST:
    if (ValueKind(v)!=symbol_value){
      fm->ErrNo = Proc_nonsymbol_assignment;
      fm->flow = FrameError;
    } else {
      assignerr = 0;
      SetSymbolAtomValue(i,SymbolValue(v));
    }
    break;
  default:
    fm->ErrNo = Proc_nonsense_assignment;
    fm->flow = FrameError;
    break;
  }
  if (assignerr) {
    ProcWriteAssignmentError(fm);
  }
}

/* this function always returns ok. 5/96 */
static
void ExecuteInitAsgn(struct procFrame *fm, struct Statement *stat)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  unsigned c,len;
  enum FrameControl oldflow;
  struct value_t value;
  enum find_errors err;

  instances = FindInstances(fm->i,DefaultStatVar(stat),&err);
  if (instances != NULL){
    assert(GetEvaluationContext()==NULL);
    SetEvaluationContext(fm->i);
    value = EvaluateExpr(DefaultStatRHS(stat),NULL,InstanceEvaluateName);
    SetEvaluationContext(NULL);
    if (ValueKind(value)==error_value) {
      fm->ErrNo = Proc_rhs_error;
      fm->flow = FrameError;
      ProcWriteAssignmentError(fm);
    } else {
      len = gl_length(instances);
      oldflow = fm->flow;
      for(c=1;c<=len;c++){
        inst = (struct Instance *)gl_fetch(instances,c);
        AssignInitValue(inst,value,fm); /* does its own errors */
        if (fm->flow == FrameError) {
          if (/* fm->policy-check */0) {
            fm->flow = oldflow; /* suppress error flow */
          } else {
            break; /* skip rest of loop */
          }
        }
      }
    }
    DestroyValue(&value);
    gl_destroy(instances);
  } else {
    /* error finding left hand side */
    fm->ErrNo = Proc_lhs_error;
    fm->flow = FrameError;
    ProcWriteAssignmentError(fm);
  }
  return /* Proc_all_ok */;
}

static
void ExecuteInitStatement(struct procFrame *fm, struct Statement *stat)
{
#if IDB
FPRINTF(fm->err,"\n");
FPRINTF(fm->err,"EIS-IN: %s\n",FrameControlToString(fm->flow));
FPRINTF(fm->err,"EIS: "); WriteStatement(fm->err,stat,2);
#endif
  switch(StatementType(stat)){
  case FOR:
    ExecuteInitFor(fm,stat);
    break;
  case ASGN:
    ExecuteInitAsgn(fm,stat);
    break;
  case RUN:
    ExecuteInitRun(fm,stat);
    break;
  case FIX:
    ExecuteInitFix(fm,stat);
    break;
  case FREE:
	ExecuteInitFree(fm,stat);
	break;
  case FLOW:
    ExecuteInitFlow(fm);
    break;
  case EXT:
    CONSOLE_DEBUG("ABOUT TO ExecuteInitExt");
    ExecuteInitExt(fm,stat);
    break;
  case CALL:
    ExecuteInitCall(fm,stat);
    break;
  case WHILE:
    ExecuteInitWhile(fm,stat);
    break;
  case ASSERT:
	ExecuteInitAssert(fm,stat);
	break;
  case IF:
    ExecuteInitIf(fm,stat);
    break;
  case SWITCH:
    ExecuteInitSwitch(fm,stat);
    break;
  case CASGN:
    fm->flow = FrameError;
    fm->ErrNo = Proc_declarative_constant_assignment;
    WriteInitErr(fm,
                 "Incorrect statement type (constant assigned)"
                 " in initialization section");
    break;
  default:
    fm->flow = FrameError;
    fm->ErrNo = Proc_bad_statement;
    WriteInitErr(fm,"Unexpected statement type in initialization section");
    break;
  }
#if IDB
FPRINTF(fm->err,"EIS-OUT: %s\n\n",FrameControlToString(fm->flow));
#endif
  return;
}

/* This is our central error handling logic control point.
 * This function should not itself return fm->flow == FrameError.
 * To the maximum extent possible, do not process errors separately
 * elsewhere but defer them to here. That makes maintenance of code
 * which handles debugging output and execution logic much simpler.
 */
static
void ExecuteInitStatements(struct procFrame *fm, struct StatementList *sl)
{
  unsigned c,length;
  struct gl_list_t *statements;
  struct Statement *stat;
  enum FrameControl oldflow;
  int stop;

  statements = GetList(sl);
  length = gl_length(statements);
  stop = 0;
  oldflow = fm->flow;
  for (c = 1; c <= length && !stop; c++){
    stat = (struct Statement *)gl_fetch(statements,c);
    UpdateProcFrame(fm,stat,fm->i);
    /* statements should issue their own complaints */
    ExecuteInitStatement(fm,stat);
    switch (fm->flow) {
    case FrameLoop:
    case FrameOK:
      fm->flow = oldflow;
      break;
    case FrameError:
#if IDB
FPRINTF(fm->err,"ERR: "); WriteStatement(fm->err,fm->stat,0);
FPRINTF(fm->err,"\n");
#endif
      if ((fm->gen & WP_STOPONERR)!= 0) {
        fm->flow = FrameReturn;
        stop = 1;
      } else {
        fm->flow = oldflow;
      }
      break;
    case FrameFallthru: /* say what? */
    case FrameContinue:
    case FrameBreak:
      if (oldflow == FrameLoop) {
        stop = 1;
      } else {
        /* whine about missing loop/switch context.
         * should be parser enforced.
         */
#if IDB
FPRINTF(fm->err,"LOOP-ERR: "); WriteStatement(fm->err,fm->stat,0);
FPRINTF(fm->err,"\n");
#endif
        if ((fm->gen & WP_STOPONERR)!= 0) {
          fm->flow = FrameReturn;
          stop = 1;
        } else {
          fm->flow = oldflow;
        }
      }
      break;
    case FrameReturn:
#if IDB
FPRINTF(fm->err,"ERR-UNWIND: "); WriteStatement(fm->err,fm->stat,0);
FPRINTF(fm->err,"\n");
#endif
      if (/* i/o policy check */1) {
        /* whine backtrace*/
      }
      stop = 1;
      break;
    /* all cases must be handled here. */
    }
    if (g_procframe_stop) {
      g_procframe_stop = 0;
      fm->ErrNo = Proc_user_interrupt;
      WriteInitErr(fm,"USER interrupted METHOD execution");
      fm->flow = FrameReturn;
      stop = 1;
    }
  }
  /* UpdateProcFrame(fm,NULL, fm->i); */ /* leave a mess for messages */
  assert(fm->flow != FrameError);
}

/*********************************************************************\
 * void ExecuteInitProcedure(i,proc)
 * struct Instance *i;
 * struct InitProcedure *proc;
 * This will execute proc on the instance i.
\*********************************************************************/
/*
 * Here's where we enforce stack limits (approximately).
 * Here's where we unwind the stack in the event of an
 * early return.
 */
static
void ExecuteInitProcedure(struct procFrame *fm, struct InitProcedure *proc)
{
  struct for_table_t *OldForTable;

  g_proc.depth++;
  assert(fm != NULL && fm->i != NULL && proc != NULL);
  if (g_proc.depth > g_proc.limit) {
    g_proc.depth--;
    fm->ErrNo = Proc_stack_exceeded_this_frame;
    fm->flow = FrameError;
    return;
  }

  OldForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  ExecuteInitStatements(fm,ProcStatementList(proc));
  DestroyForTable(GetEvaluationForTable());
  SetEvaluationForTable(OldForTable);
  g_proc.depth--;
}

/* returns overflow or ok. possibly either form of overflow. */
static
void RealInitialize(struct procFrame *fm, struct Name *name)
{
  struct Name *instname = NULL;
  struct Instance *ptr;
  enum find_errors err;
  struct InitProcedure *proc;
  struct gl_list_t *instances;
  unsigned long c,length;
  char *morename;
  struct procFrame *newfm;
  symchar *procname=NULL;
  int stop;
  int previous_context = GetDeclarativeContext();

  SetDeclarativeContext(1); /* set up for procedural processing */
  InstanceNamePart(name,&instname,&procname);

  if (procname != NULL) {
    instances = FindInstances(fm->i, instname, &err);
    if (instances != NULL) {
      length = gl_length(instances);
      stop = 0;
      for(c=1; c<=length && !stop; c++){
        ptr = (struct Instance *)gl_fetch(instances,c);
        proc = FindProcedure(ptr,procname);
        if (proc != NULL) {
          morename = WriteInstanceNameString(ptr,fm->i);
          newfm = AddProcFrame(fm,ptr,
                               (morename!=NULL)?morename:"",
                               proc,FrameInherit);
          /* this usage probably force memory recycle in proctype.c */
          if (morename != NULL) {
            ascfree(morename);
          }
          ExecuteInitProcedure(newfm,proc);
          switch (newfm->flow) {
          case FrameOK:
          case FrameLoop:
            /* do nothing */
            break;
          case FrameBreak:
          case FrameContinue:
          case FrameFallthru:
            /* aren't supposed to work across frames, or are they? */
            /* do nothing */
            break;
          case FrameError:
            /* having to check this here sucks, but the stack
             * limit is not optional.
             */
            if ((fm->gen & WP_STOPONERR) != 0 || /* ||, not && */
                 newfm->ErrNo == Proc_stack_exceeded_this_frame) {
              fm->flow = newfm->flow;
              fm->ErrNo = newfm->ErrNo;
              if (fm->ErrNo == Proc_stack_exceeded_this_frame) {
                fm->ErrNo = Proc_stack_exceeded;
              }
              stop = 1;
            }
            ProcWriteStackCheck(newfm,NULL,name);
            break;
          case FrameReturn:
            if (newfm->ErrNo != Proc_return) {
              fm->flow = newfm->flow;
              fm->ErrNo = newfm->ErrNo;
              ProcWriteStackCheck(newfm,NULL,name);
            } /* else was a c-like RETURN;. don't pass upward */
            break;
          }
          DestroyProcFrame(newfm);
        } else {
          fm->flow = FrameError;
	  ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"PROCEDURE NOT FOUND (FindProcedure failed).");
          fm->ErrNo = Proc_proc_not_found;
        }
      }
      gl_destroy(instances);
    } else {			/* unable to find instances */
      fm->flow = FrameError;
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"PROCEDURE NOT FOUND (FindInstances failed).");
      fm->ErrNo = Proc_instance_not_found;
    }
  } else {
    fm->flow = FrameError;
    fm->ErrNo = Proc_bad_name;
  }
  SetDeclarativeContext(previous_context);
  DestroyName(instname);
  return;
}

/* Convert all those messy result to a proc enum for UI consumption. */
static
enum Proc_enum InitCalcReturn(struct procFrame *fm)
{
  switch(fm->flow) {
  case FrameOK:
    return Proc_all_ok;
  case FrameReturn: /* FALLTHROUGH */
  case FrameError:
    /* whine */
    return fm->ErrNo;
  case FrameLoop:
    /* whine a lot */
  case FrameContinue:
    return Proc_continue;
  case FrameBreak:
    return Proc_break;
  case FrameFallthru:
    return Proc_fallthru;
    /* all must be handled in this switch */
  }
  return -1;
}

/* internal debug head */
static
enum Proc_enum DebugInitialize(struct Instance *context,
                               struct Name *name,
                               char *cname,
                               FILE *err,
                               wpflags options,
                               struct gl_list_t *watchpoints,
                               FILE *log,
                               struct procFrame *fm)
{
  struct procDebug dbi; /* this struct is huge */

  InitDebugTopProcFrame(fm,context,cname,err,options,&dbi,watchpoints,log);
  RealInitialize(fm,name);
  return InitCalcReturn(fm);
}

/* internal normal head */
static
enum Proc_enum NormalInitialize(struct procFrame *fm, struct Name *name)
{
  RealInitialize(fm,name);
  return InitCalcReturn(fm);
}

enum Proc_enum Initialize(struct Instance *context,
                          struct Name *name,
                          char *cname,
                          FILE *err,
                          wpflags options,
                          struct gl_list_t *watchpoints,
                          FILE *log)
{
  enum Proc_enum rval;
  struct procFrame fm;

  assert(err != NULL);
  g_proc.depth = 0;
  Asc_SetMethodUserInterrupt(0);
  if (watchpoints == NULL) {
    InitNormalTopProcFrame(&fm,context,cname,err,options);
    rval = NormalInitialize(&fm,name);
  } else {
    rval = DebugInitialize(context,name,cname,err,options,watchpoints,log,&fm);
  }
  return rval;
}

/*
 * This deals with initializations of the form:
 * RUN Type::procname; where Type is model or atom type,
 * and procname is a procedure defined within that type.
 * If the Type happened to have redefined a procedure from its
 * parent class, that procedure would be the one on its
 * procedure list and hence the one that would be invoked.
 *
 */
static
void ClassAccessRealInitialize(struct procFrame *fm,
                               struct Name *class,
                               struct Name *name)
{
  struct InitProcedure *proc;
  struct procFrame *newfm;
  struct gl_list_t *plist;
  symchar *procname;
  symchar *typename;
  struct TypeDescription *desc,*conformable;
  int previous_context = GetDeclarativeContext();

  SetDeclarativeContext(1); /* set up for procedural processing */

  typename = SimpleNameIdPtr(class);
  if (typename != NULL) {
    desc = FindType(typename);
    if (desc != NULL) {
      conformable = InstanceTypeDesc(fm->i);
      if (MoreRefined(conformable,desc)) {
        plist = GetInitializationList(desc);
        if (plist != NULL) {
          procname = SimpleNameIdPtr(name);
          if (procname != NULL) {
            proc = SearchProcList(plist,procname);
            if (proc == NULL) {
              proc = SearchProcList(GetUniversalProcedureList(),procname);
            }
            if (proc != NULL) {
              newfm = AddProcFrame(fm,fm->i,"",proc,FrameInherit);
              /* apf starts newfm with frameok */
              ExecuteInitProcedure(newfm,proc);
              switch (newfm->flow) {
              case FrameOK:
              case FrameLoop:
                /* do nothing */
                break;
              case FrameBreak:
              case FrameContinue:
              case FrameFallthru:
                /* aren't supposed to work across frames are they? */
                /* do nothing */
                break;
              case FrameError:
                fm->flow = newfm->flow;
                fm->ErrNo = newfm->ErrNo;
                ProcWriteStackCheck(newfm,class,name);
                /* having to check this here sucks, but the stack
                 * limit is not optional.
                 */
                if (fm->ErrNo == Proc_stack_exceeded_this_frame) {
                  fm->ErrNo = Proc_stack_exceeded;
                }
                break;
              case FrameReturn:
                if (newfm->ErrNo != Proc_return) {
                  fm->flow = newfm->flow;
                  fm->ErrNo = newfm->ErrNo;
                  ProcWriteStackCheck(newfm,class,name); /* fixme?*/
                } /* else was a c-like RETURN;. don't pass upward */
                break;
              }
              DestroyProcFrame(newfm);
            } else {
              fm->flow = FrameError;
	      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"PROCEDURE NOT FOUND (SearchProcList).");
              fm->ErrNo = Proc_proc_not_found;
            }
          } else {
            fm->flow = FrameError;
            fm->ErrNo = Proc_illegal_name_use;
          }
        } else {
          fm->flow = FrameError;
	  ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"PROCEDURE NOT FOUND (GetInitializationList is null).");
          fm->ErrNo = Proc_proc_not_found;
        }
      } else {
        fm->flow = FrameError;
        fm->ErrNo = Proc_illegal_type_use;
      }
    } else {
      fm->flow = FrameError;
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"PROCEDURE NOT FOUND (FindType failed)\n");
      fm->ErrNo = Proc_type_not_found;
    }
  } else {
    fm->flow = FrameError;
    fm->ErrNo = Proc_illegal_name_use;
  }

  SetDeclarativeContext(previous_context);
  return;
}

/* internal debug head */
static
enum Proc_enum DebugClassAccessInitialize(struct Instance *context,
        			          struct Name *class,
        			          struct Name *name,
                                          char *cname,
                                          FILE *err,
                                          wpflags options,
                                          struct gl_list_t *watchpoints,
                                          FILE *log,
                                          struct procFrame *fm)
{
  struct procDebug dbi; /* this struct is huge */

  InitDebugTopProcFrame(fm,context,cname,err,options,&dbi,watchpoints,log);
  ClassAccessRealInitialize(fm,class,name);
  return InitCalcReturn(fm);
}

/* internal normal head */
static
enum Proc_enum NormalClassAccessInitialize(struct procFrame *fm,
                                           struct Name *class,
                                           struct Name *name)
{
  ClassAccessRealInitialize(fm,class,name);
  return InitCalcReturn(fm);
}

enum Proc_enum ClassAccessInitialize(struct Instance *context,
        			     struct Name *class,
        			     struct Name *name,
                                     char *cname,
                                     FILE *err,
                                     wpflags options,
                                     struct gl_list_t *watchpoints,
                                     FILE *log)
{
  struct procFrame fm;

  assert(err != NULL);
  g_proc.depth = 0;
  Asc_SetMethodUserInterrupt(0);
  if (watchpoints == NULL) {
    InitNormalTopProcFrame(&fm,context,cname,err,options);
    return NormalClassAccessInitialize(&fm,class,name);
  } else {
    return DebugClassAccessInitialize(context,class,name,cname,
                                      err,options,watchpoints,log,&fm);
  }
}
