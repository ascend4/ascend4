/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 1997 Benjamin Allan, Vicente Rico-Ramirez

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//*
	@file
	Ascend Instantiator Implementation

	@TODO this file is enormous and should be broken into pieces! -- JP
*//*
	by Tom Epperly
	Created: 1/24/90
	Last in CVS: $Revision: 1.84 $ $Date: 2003/02/06 04:08:30 $ $Author: ballan $
*/

#include "instantiate.h"

#include <ctype.h>
#include <strings.h>

#include <ascend/utilities/bit.h>

#include "vlist.h"
#include "initialize.h"
#include "watchpt.h"

#include "stattypes.h"
#include "statement.h"
#include "type_desc.h"
#include "type_descio.h"
#include <ascend/general/ascMalloc.h>
#include <ascend/general/dstring.h>
#include <ascend/general/ospath.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/config.h>
#include "module.h"
#include "library.h"
#include "sets.h"
#include "setio.h"
#include "units.h"
#include "extfunc.h"
#include "extcall.h"
#include "forvars.h"
#include "exprs.h"
#include "exprio.h"
#include "nameio.h"
#include "evaluate.h"
#include "value_type.h"
#include "statio.h"
#include "pending.h"
#include "find.h"
#include "relation.h"
#include "logical_relation.h"
#include "logrelation.h"
#include "relation_util.h"
#include "rel_blackbox.h"
#include "logrel_util.h"
#include "instance_types.h"
#include "cmpfunc.h"
#include "instance_io.h"
#include "when.h"
#include "case.h"
#include "when_util.h"
#include "select.h"
#include "link.h"
/* new headers */
#include "atomvalue.h"
#include "arrayinst.h"
#include "copyinst.h"
#include "createinst.h"
#include "destroyinst.h"
#include "extinst.h"
#include "visitinst.h"
#include "instquery.h"
#include "mathinst.h"
#include "mergeinst.h"
#include "parentchild.h"
#include "refineinst.h"
#include "check.h"
#include "instance_name.h"
#include "setinstval.h"
#include "anontype.h"
#include "anoncopy.h"
#include "parpend.h"
#include "parpend.h"
#include "bintoken.h"
#include "relerr.h"

#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#ifdef ASC_WITH_ZLIB
# include <zlib.h>
#endif
#ifdef ASC_WITH_LZMA
# include <lzma.h>
#endif

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>

#ifdef ASC_SIGNAL_TRAPS
# include <ascend/utilities/ascSignal.h>
#endif

/*#include <stdlib.h>*/
#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>
#include <ascend/general/pool.h>
#include <ascend/general/list.h>
#include <ascend/general/dstring.h>

#if TIMECOMPILER
#include <ascend/general/tm_time.h>
#endif

/* don't even THINK ABOUT adding instmacro.h to this list -- ...what the? */

#define MAXNUMBER 4		/* maximum number of iterations allowed
                                 * without change */
#define PASS2MAXNUMBER 1	/* maximum number of iterations allowed
                                 * without change doing relations. In
                                 * system where rels reference rels, > 1 */

#define PASS3MAXNUMBER 4	/* maximum number of iterations allowed
                                 * without change doing logical relations.
                                 * In system where logrels reference logrels,
                                 * > 1 */

#define PASS4MAXNUMBER 1	/* maximum number of iterations allowed
                                 * without change executing WHEN. In
                                 * system where WHEN reference WHEN, > 1 */

#define PASS5MAXNUMBER 1	/* maximum number of iterations allowed
                                 * without change executing LINK. In
                                 * system where LINK reference LINK, > 1 */

#define AVG_CASES 2L		/* size to which all cases lists are */
                                /* initialized (WHEN instance) */
#define AVG_REF 2L		/* size to which all list of references */
                                /* in a case are initialized (WHEN) */

#define NO_INCIDENCES 7         /* avg number of vars in a external reln */

static int g_iteration = 0;	/* the current iteration. */
/**
 * Tracks whether any DATASET statements were executed in the current
 * pass-1 instantiation cycle. Large DATASET-backed constant arrays can
 * make full pass-1 structural diagnostics very expensive.
 */
static int g_pass1_saw_dataset = 0;

/* moved from tcltk/generic/interface/SimsProc.c */
struct Instance *g_cursim;

#define NEW_ext 1
#define OLD_ext 0
/**<
	variable to check agreement in the number of boolean, integer or symbol
	variables in the WHEN/SELECT statement with the number of boolean, integer
	or symbol values in each of the CASEs
*/

#define MAX_VAR_IN_LIST  20
/**<
	Variables to switch old and new pass 2 instantiation.
	The condition for using new pass 2 (anonymous type-based
	relation copying) is g_use_copyanon != 0 || FORCE applied.
*/

int g_use_copyanon = 1;
/**
	the user switch for anonymous type based relation
	copying. if 0, no copying by that method is done.
*/

#if TIMECOMPILER
static
int g_ExecuteREL_CreateTokenRelation_calls = 0;
static
int g_ExecuteEXT_CreateBlackboxRelation_calls = 0;
/* count the number of calls to CreateTokenRelation from ExecuteREL */
int g_CopyAnonRelation = 0;
#endif

long int g_compiler_counter = 1;
/**<
	What: counter incremented every time a compiler action capable of
	     changing the instance tree is executed.
	     At present the compiler cares nothing about this counter,
	     but it is provided as a service to clients.

	Real applications:
	1) This variable is used for keeping track of calls to
	the compiler which will create the need for a total solver system
	rebuild.  This variable should be incremented anytime a function
	which changes the instance tree is called.
*/

#define DEBUG_RELS
/* undef DEBUG_RELS if you want less spew in pass 2 */
#undef DEBUG_RELS

#ifdef DEBUG_RELS
/* root of tree being visited in pass 2. */
struct Instance *debug_rels_work;
#endif /* dbgrels */

static unsigned
int g_instantiate_relns = ALLRELS;	/* default is to do all rels */

/* pointer to possible error message for child expansion.
 * messy way of error handling; do not imitate.
 */
static char *g_trychildexpansion_errmessage = NULL;
#define TCEM g_trychildexpansion_errmessage

/* error messages */
#define REDEFINE_CHILD_MESG "IS_A statement attempting to redefine child "
#define REDEFINE_CHILD_MESG2 "ALIASES statement attempting to redefine child "
#define UNDEFINED_TYPE_MESG "IS_A statement refers to undefined type "
#define IRT_UNDEFINED_TYPE "IS_REFINED_TO statement refers to undefined type "
#define REASSIGN_MESG1 "Attempt to reassign constant "
#define REASSIGN_MESG2 " value."

/*------------------------------------------------------------------------------
	forward declarations
*/

static void WriteForValueError(struct Statement *, struct value_t);
static void MakeInstance(CONST struct Name *, struct TypeDescription *, int,
                  struct Instance *, struct Statement *, struct Instance *);
static int CheckVarList(struct Instance *, struct Statement *);
static int CheckWhereStatements(struct Instance *,struct StatementList *);
static int ExecuteISA(struct Instance *, struct Statement *);
static int ExecuteCASGN(struct Instance *, struct Statement *);
static int ExecuteTABLE(struct Instance *, struct Statement *);
static int DigestArguments(struct Instance *,
                    struct gl_list_t *, struct StatementList *,
                    struct StatementList *, struct Statement *);
static int DeriveSetType(CONST struct Set *, struct Instance *,CONST unsigned int);

static void MissingInsts(struct Instance *, CONST struct VariableList *,int);
static struct gl_list_t *FindArgInsts(struct Instance *, struct Set *,
                               rel_errorlist *);
static void AddIncompleteInst(struct Instance *);
static int CheckALIASES(struct Instance *, struct Statement *);
static int CheckARR(struct Instance *, struct Statement *);
static int CheckISA(struct Instance *, struct Statement *);
static int IsSelectorTypeDesc(CONST struct TypeDescription *);
static int AssignISAInitialValue(struct Instance *, struct Statement *, struct Instance *);
static int AssignStructuralValue(struct Instance *,struct value_t,struct Statement *);
static int  CheckSELECT(struct Instance *, struct Statement *);
static int  CheckWHEN(struct Instance *, struct Statement *);
static void MakeRealWhenCaseReferencesFOR(struct Instance *,
                                          struct Instance *,
                                          struct Statement *,
                                          struct gl_list_t *);
static void MakeWhenCaseReferencesFOR(struct Instance *,
                                      struct Instance *,
                                      struct Statement *,
                                      struct gl_list_t *);
static int  Pass1CheckFOR(struct Instance *, struct Statement *);
static int  Pass1ExecuteFOR(struct Instance *, struct Statement *);
#ifdef THIS_IS_AN_UNUSED_FUNCTION
static int  Pass1RealCheckFOR(struct Instance *, struct Statement *);
#endif /* THIS_IS_AN_UNUSED_FUNCTION */
static void Pass1RealExecuteFOR(struct Instance *, struct Statement *);
static int  Pass2CheckFOR(struct Instance *, struct Statement *);
static int  Pass2ExecuteFOR(struct Instance *, struct Statement *);
static void Pass2FORMarkCond(struct Instance *, struct Statement *);
static void Pass2FORMarkCondRelations(struct Instance *, struct Statement *);
static int  Pass2RealCheckFOR(struct Instance *, struct Statement *);
static int  Pass2RealExecuteFOR(struct Instance *, struct Statement *);
static int  Pass3CheckFOR(struct Instance *, struct Statement *);
static int  Pass3ExecuteFOR(struct Instance *, struct Statement *);
static int  Pass3RealCheckFOR (struct Instance *, struct Statement *);
static int  Pass3RealExecuteFOR(struct Instance *, struct Statement *);
static void Pass3FORMarkCond(struct Instance *, struct Statement *);
static void Pass3FORMarkCondLogRels(struct Instance *, struct Statement *);
static int  Pass4CheckFOR(struct Instance *, struct Statement *);
static int  Pass4ExecuteFOR(struct Instance *, struct Statement *);
static int  Pass4RealCheckFOR(struct Instance *, struct Statement *);
static int  Pass5ExecuteFOR(struct Instance *, struct Statement *);
static int  ExecuteUnSelectedForStatements(struct Instance *,
                                           struct StatementList *);
static void ExecuteDefault(struct Instance *, struct Statement *,
                           unsigned long int *);
static void RealDefaultFor(struct Instance *, struct Statement *,
                           unsigned long int *);
static void DefaultStatementList(struct Instance *, struct gl_list_t *,
                                 unsigned long int *);
static void ExecuteDefaultStatements(struct Instance *, struct gl_list_t *,
                                     unsigned long int *);
static int ExecuteSELECT(struct Instance *, unsigned long *,
                         struct Statement *);
static void ExecuteDefaultsInSELECT(struct Instance *, unsigned long *,
                                    struct Statement *, unsigned long int *);
static void RealExecuteWHEN(struct Instance *, struct Statement *);
static int  ExecuteUnSelectedSELECT(struct Instance *, unsigned long *,
                                    struct Statement *);
static void ExecuteUnSelectedStatements(struct Instance *i,unsigned long *,
                                        struct StatementList *);
static void ExecuteUnSelectedWhenStatements(struct Instance *,
                                            struct StatementList *);
static int ExecuteUnSelectedWHEN(struct Instance *, struct Statement *);

static struct gl_list_t *CollectWhenReinitStatements(struct StatementList *sl){
  struct gl_list_t *result = NULL;
  struct gl_list_t *list;
  unsigned long c, len;

  if(sl == NULL){
    return NULL;
  }
  list = GetList(sl);
  len = gl_length(list);
  for(c = 1; c <= len; ++c){
    struct Statement *statement = (struct Statement *)gl_fetch(list, c);
    if(statement == NULL){
      continue;
    }
    switch(StatementType(statement)){
    case REINIT:
    case SWITCHTO:
      if(result == NULL){
        result = gl_create(2L);
      }
      gl_append_ptr(result, CopyStatement(statement));
      break;
    case FOR:
    {
      struct gl_list_t *nested = CollectWhenReinitStatements(ForStatStmts(statement));
      if(nested != NULL){
        unsigned long i, nlen = gl_length(nested);
        if(result == NULL){
          result = gl_create(nlen);
        }
        for(i = 1; i <= nlen; ++i){
          gl_append_ptr(result, gl_fetch(nested, i));
        }
        gl_destroy(nested);
      }
      break;
    }
    default:
      break;
    }
  }
  return result;
}
static void ReEvaluateSELECT(struct Instance *, unsigned long *,
                             struct Statement *, int, int *);
static int ExecuteLNK(struct Instance *inst, struct Statement *statement);

static CONST struct Expr *ConstraintStatementExpr(CONST struct Statement *statement){
  switch (StatementType(statement)) {
  case REL:
    return RelationStatExpr(statement);
  case LOGREL:
    return LogicalRelStatExpr(statement);
  default:
    return NULL;
  }
}

/*-----------------------------------------------------------------------------
	...
*/


static
void ClearIteration(void)
{
  g_iteration = 0;
}

static
void instantiation_error(error_severity_t sev
		, const struct Statement *stat, const char *fmt
		, ...
){
	va_list args,args2;
	va_start(args,fmt);
	va_copy(args2,args);
	if(stat!= NULL){
		va_error_reporter(sev
			, Asc_ModuleBestName(StatementModule(stat))
			, StatementLineNum(stat), NULL
			, fmt, args2
		);
	}else{
		va_error_reporter(sev
			, NULL, 0, NULL
			, fmt, args2
		);
	}
	va_end(args);
}

static void instantiation_name_error(error_severity_t sev
		, const struct Name *name,const char *msg
){
	ERROR_REPORTER_START_NOLINE(sev);
	FPRINTF(ASCERR,"%s: name '",msg);
	WriteName(ASCERR,name);
	FPRINTF(ASCERR,"'");
	error_reporter_end_flush();
}

static
void WriteSetError(struct Statement *statement, struct TypeDescription *def)
{
  STATEMENT_ERROR(statement, (GetBaseType(def) == set_type) ?
                             "No set type specified in IS_A statement"
                             : "Set type specified for a non-set type");
}

/**
	This code will emit error messages only on the last
	iteration when trying to clear pending statements.
	g_iteration is the global iteration counter, and MAXNUMBER
	is the number of times that the instantiator will try
	to clear the list, without change.
*/
static
void WriteUnexecutedMessage(FILE *f, struct Statement *stat, CONST char *msg)
{
  if (g_iteration>=(MAXNUMBER)) WSSM(f,stat,msg,0);
}


/**
	Write Unexecuted Error Message in Pass 3 WUEMPASS3

	This code will emit error messages only on the last
	iteration of pass3 when trying to clear pending statements.
	g_iteration is the global iteration counter, and PASS3MAXNUMBER
	is the number of times that the instantiator will try
	to clear the list, without change.
*/
static
void WUEMPASS3(FILE *f, struct Statement *stat, CONST char *msg)
{
  if (g_iteration>=(PASS3MAXNUMBER)) WSSM(f,stat,msg,0);
}

/*------------------------------------------------------------------------------
  DENSE ARRAY PROCESSING

	...mostly
*/

/**
	returns 0 if c is NULL, probably should be -1.
	-2 if c is illegal set type
	1 if c IS_A integer_constant set type
	0 if c IS_A symbol_constant set type
	@param statement is used only to issue error messages.
*/
static
int CalcSetType(symchar *c, struct Statement *statement)
{
  struct TypeDescription *desc;
  if (c==NULL) return 0;
  if ((desc = FindType(c)) != NULL){
    switch(GetBaseType(desc)){
    case integer_constant_type: return 1;
    case symbol_constant_type: return 0;
    default:
      STATEMENT_ERROR(statement, "Incorrect set type in IS_A");
      /* lint should keep us from ever getting here */
      return -2;
    }
  }else{
    STATEMENT_ERROR(statement, "Unable to determine type of set.");
    return -2;
  }
}

/**
	last minute check for set values that subscript arrays.
	probably should check constantness too but does not.
	return 0 if ok, 1 if not.
*/
static
int CheckSetVal(struct value_t setval)
{
  if (ValueKind(setval) != set_value) {
    switch (ValueKind(setval)) {
    case integer_value:
      TCEM = "Incorrectly integer-valued array range.";
      break;
    case symbol_value:
      TCEM = "Incorrect symbol-valued array range.";
      break;
    case real_value:
      TCEM = "Incorrect real-valued array subscript.";
      break;
    case boolean_value:
      TCEM = "Incorrect boolean-valued array subscript.";
      break;
    case list_value:
      TCEM = "Incorrect list-valued array subscript.";
      break;
    case error_value:
      switch (ErrorValue(setval)) {
      case type_conflict:
        TCEM = "Set expression type conflict in array subscript.";
        break;
      default:
        TCEM = "Generic error 1 in array subscript.";
        break;
      }
      break;
    case set_value: /* really weird if this happens, since if eliminated it */
      break;
    default:
      TCEM = "Generic error 2 in array subscript.";
      break;
    }
    return 1;
  }
  return 0;
}

/**
	This attempts to evaluate a the next undone subscript of the
	array and call ExpandArray with that set value.
	In the case of ALIAS arrays this must always succeed, because
	we have checked first that it will. If it did not we would
	be stuck because later calls to ExpandArray will not know
	the difference between the unexpanded alias array and the
	unexpanded IS_A array.

	Similarly, in the case of parameterized arrays this must
	always succeed, OTHERWISE ExpandArray will not know the
	arguments of the IS_A type, arginst next time around.

	In the event that the set given or set value expanded is bogus,
	returns 1 and statement from which this call was derived is
	semantically garbage.
*/
static
int ValueExpand(struct Instance *i, unsigned long int pos,
                 struct value_t value, int *changed,
                 struct Instance *rhsinst, struct Instance *arginst,
                 struct gl_list_t *rhslist)
{
  struct value_t setval;
  switch(ValueKind(value)){
  case list_value:
    setval = CreateSetFromList(value);
    if (CheckSetVal(setval)) {
      return 1;
    }
    ExpandArray(i,pos,SetValue(setval),rhsinst,arginst,rhslist);
    /* this may modify the pending instance list if
     * rhslist and rhsinst both == NULL.
     */
    *changed = 1;
    DestroyValue(&setval);
    break;
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      break;
    default:
      TCEM = "Array instance has incorrect index type.";
      return 1;
    }
    break;
  default:
    TCEM = "Array instance has incorrect index value type.";
    return 1;
  }
  return 0;
}

/**
	When an incorrect combination of sparse and dense indices is found,
	marks the statement wrong and whines. If the statement has already
	been marked wrong, does not whine.

	In FOR loops,
	this function warns  about a problem that the implementation really
	should allow. Alas, the fix is pending a complete rework of arrays.

	In user is idiot case,
	this really should have been ruled out by checkisa, which lets a little
	too much trash through. Our whole array implementation sucks.
*/
static
void SignalChildExpansionFailure(struct Instance *work,unsigned long cnum)
{
  struct TypeDescription *desc;
  ChildListPtr clp;
  struct Statement *statement;

  asc_assert(work!= NULL);
  asc_assert(cnum!= 0);
  asc_assert(InstanceKind(work)==MODEL_INST);
  desc = InstanceTypeDesc(work);
  clp = GetChildList(desc);
  statement = (struct Statement *)ChildStatement(clp,cnum);
  if(statement == NULL){
    ERROR_REPORTER_HERE(ASC_USER_ERROR,
      "Unable to determine declaration statement for child '%s' while reporting array expansion failure",
      SCP(ChildStrPtr(clp,cnum))
    );
    return;
  }
  if ( StatWrong(statement) != 0) {
    return;
  }
  if (TCEM != NULL) {
    CONSOLE_DEBUG("TCEM = %s",TCEM);
    TCEM = NULL;
  }
  if (StatInFOR(statement)) {
    MarkStatContext(statement,context_WRONG);
    STATEMENT_ERROR(statement, "Add another FOR index. In FOR loops,"
         " all array subscripts must be scalar values, not sets.");
    WSS(ASCERR,statement);
  }else{
    MarkStatContext(statement,context_WRONG);
    STATEMENT_ERROR(statement, "Subscripts of conflicting or incorrect types"
         " in rectangular array.");
    WSS(ASCERR,statement);
  }
  return;
}

/**
	Should never be called with BOTH rhs(inst/list) and arginst != NULL,
	but one or both may be NULL depending on other circumstances.
	Should never be called on ALIASES/IS_A inside a for loop.
	Returns an error number other than 0 if called inside a for loop.
	If error, outer scope should mark statement incorrect.
*/
static
int TryChildExpansion(struct Instance *child,
                       struct Instance *parent,
                       int *changed,
                       struct Instance *rhsinst,
                       struct Instance *arginst,
                       struct gl_list_t *rhslist)
{
  unsigned long pos,oldpos=0;
  struct value_t value;
  CONST struct Set *setp;
  int error=0;
  asc_assert(arginst==NULL || (rhsinst==NULL && rhslist==NULL));
  /* one must be NULL as alii do not have args */
  while((pos=NextToExpand(child))>oldpos){
    oldpos=pos;
    setp = IndexSet(child,pos);
    if (GetEvaluationContext() != NULL) {
      error++;
      FPRINTF(ASCERR,"TryChildExpansion with mixed instance\n");
    }else{
      SetEvaluationContext(parent); /* could be wrong for mixed style arrays */
      value = EvaluateSet(setp,InstanceEvaluateName);
      SetEvaluationContext(NULL);
      if (ValueExpand(child,pos,value,changed,rhsinst,arginst,rhslist) != 0) {
        error++;
      }
      DestroyValue(&value);
    }
  }
  return error;
}

/**
	expands, if possible, children of nonrelation,
	nonalias, nonparameterized arrays.
*/
static
void TryArrayExpansion(struct Instance *work, int *changed)
{
  unsigned long c,len;
  struct Instance *child;
  struct TypeDescription *desc;
  len = NumberChildren(work);
  for(c=1;c<=len;c++){
    child = InstanceChild(work,c);
    if (child!=NULL){
      switch(InstanceKind(child)){
      case ARRAY_INT_INST:
      case ARRAY_ENUM_INST:
        desc = InstanceTypeDesc(child);
        /* no alii, no parameterized types, no for loops allowed. */
        if ((!GetArrayBaseIsRelation(desc))&&(!RectangleArrayExpanded(child)) &&
             (!GetArrayBaseIsLogRel(desc)) ) {
          if (TryChildExpansion(child,work,changed,NULL,NULL,NULL)!= 0) {
            SignalChildExpansionFailure(work,c);
          }
        }
        break;
      default:
#if 0 /* example of what not to do here */
        FPRINTF(ASCERR,"TryArrayExpansion called with non-array instance\n");
        /* calling with non array child is fairly common and unavoidable */
#endif
        break;
      }
    }
  }
}

static
void DestroyIndexList(struct gl_list_t *gl)
{
  struct IndexType *ptr;
  int c,len;
  if (gl!=NULL) {
    for (c=1,len = gl_length(gl);c <= len;c++) {
      ptr = (struct IndexType *)gl_fetch(gl,c);
      if (ptr) DestroyIndexType(ptr);
    }
    gl_destroy(gl);
  }
}

/**
	returns 1 if ex believed to be integer, 0 if symbol, and -1 if
	confused. if searchfor TRUE, includes fortable in search
*/
static
int FindExprType(CONST struct Expr *ex, struct Instance *parent,
                 CONST unsigned int searchfor
){
  struct Instance *i;
  struct gl_list_t *ilist;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  switch(ExprType(ex)){
  case e_var:
    ilist = FindInstances(parent,ExprName(ex),&err);
    if ((ilist!=NULL)&&(gl_length(ilist)>0)){
      i = (struct Instance *)gl_fetch(ilist,1);
      gl_destroy(ilist);
      switch(InstanceKind(i)){
      case INTEGER_ATOM_INST:
      case INTEGER_INST:
      case INTEGER_CONSTANT_INST:
        return 1;
      case SYMBOL_ATOM_INST:
      case SYMBOL_INST:
      case SYMBOL_CONSTANT_INST:
        return 0;
      case SET_ATOM_INST:
      case SET_INST:
        return IntegerSetInstance(i);
      default:
        FPRINTF(ASCERR,"Incorrect index type; guessing integer index.\n");
        return 1;
      }
    }else{
      if (ilist!=NULL) gl_destroy(ilist);
      if (GetEvaluationForTable()!=NULL) {
        symchar *name;
        struct for_var_t *ptr;
        AssertMemory(GetEvaluationForTable());
        name = SimpleNameIdPtr(ExprName(ex));
        if (name!=NULL) {
          ptr = FindForVar(GetEvaluationForTable(),name);
          if (ptr!=NULL) {
            switch(GetForKind(ptr)) {
            case f_integer:
              return 1;
            case f_symbol:
              return 0;
            default:
              FPRINTF(ASCERR,"Undefined FOR or indigestible variable.\n");
            }
          }
        }
      }
      return -1;
    }
  case e_int:
    return 1;
  case e_symbol:
    return 0;
  case e_set:
    return DeriveSetType(ExprSValue(ex),parent,searchfor);
  default:
    if (g_iteration>=(MAXNUMBER)) {
      /* referencing g_iteration sucks, but seeing spew sucks more.*/
      /* WUM, which we want, needs a statement ptr we can't supply. */
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Heuristic FindExprType failed. Check your indices. Assuming integer array index.");
    }
    return -1;
  }
}

/**
	returns -1 if has no clue,
	returns 1 if set appears to be int set
	returns 0 if apparently symbol_constant set.
*/
static
int DeriveSetType(CONST struct Set *sptr, struct Instance *parent,
                  CONST unsigned int searchfor
){
  CONST struct Set *ptr;
  int result=-1;		/* -1 indicates a failure */
  ptr = sptr;
  /* if it contains a range it must be an integer set */
  while(ptr!=NULL){
    if (SetType(ptr)) return 1;
    ptr = NextSet(ptr);
  }
  ptr = sptr;
  /* try to find the type from the expressions */
  while(ptr!=NULL){
    if ((result = FindExprType(GetSingleExpr(ptr),parent,searchfor)) >= 0) {
      return result;
    }
    ptr = NextSet(ptr);
  }
  return -1;			/* undefined type */
}

/**
	Returns a gllist contain the string form (or forms) of array
	subscripts(s)
	e.g. Name a[1..2]['foo']
	will return a gllist containing something like:
	"1..2"
	"foo"
*/
static
struct gl_list_t *ArrayIndices(CONST struct Name *name,
                               struct Instance *parent)
{
  struct gl_list_t *result;
  int settype;
  CONST struct Set *sptr;

  if (!NameId(name)) return NULL;
  name = NextName(name);
  if (name == NULL) return NULL;
  result = gl_create(2L);
  while (name!=NULL){
    if (NameId(name)){
      DestroyIndexList(result);
      return NULL;
    }
    sptr = NameSetPtr(name);
    if ((settype = DeriveSetType(sptr,parent,0)) >= 0){
      gl_append_ptr(result,
                    (VOIDPTR)CreateIndexType(CopySetList(sptr),settype));
    }else{
      DestroyIndexList(result);
      return NULL;
    }
    name = NextName(name);
  }
  return result;
}

/*-----------------------------------------------------------------------------
  SPARSE AND DENSE ARRAY PROCESSING
*/

/* this function has been modified to handle list results when called
 * from check aliases and dense executearr.
 * The indices made here in the aliases case where the alias is NOT
 * inside a FOR loop are NOT for consumption by anyone because they
 * contain a dummy index type. They merely indicate that
 * indices can be made. They should be immediately destroyed.
 * DestroyIndexType is the only thing that groks the Dummy.
 * This should not be called on the final subscript of an ALIASES/IS_A
 * inside a FOR loop unless you can grok a dummy in last place.
 *
 * External relations (bbox) contain an innermost implicit for loop
 * in their arrayness over the outputs of the bbox, and this causes
 * us to have a bit of trickiness.
 */
static
struct IndexType *MakeIndex(struct Instance *inst,
                            CONST struct Set *sptr,
                            struct Statement *stat, int last)
{
  struct value_t value;
  struct value_t setval;
  int intset;
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  if (StatInFOR(stat) || StatementType(stat) == EXT) {
    if (sptr == NULL ||
        NextSet(sptr) != NULL ||
        SetType(sptr) != 0 ) {
      /* must be simple index */
      WriteUnexecutedMessage(ASCERR,stat,
        "Next subscript in FOR loop IS_A must be a scalar value,"
        " not a set value.");
      SetEvaluationContext(NULL);
      return NULL;
    }
    value = EvaluateExpr(GetSingleExpr(sptr),NULL,InstanceEvaluateName);
    SetEvaluationContext(NULL);
    switch(ValueKind(value)){
    case real_value:
    case boolean_value:
    case set_value:
    case list_value:
      if (last==0) {
        STATEMENT_ERROR(stat, "Index to sparse array is of an incorrect type");
        DestroyValue(&value);
        return NULL;
      }else{
        setval = CreateSetFromList(value);
        intset = (SetKind(SetValue(setval)) == integer_set);
        DestroyValue(&value);
        DestroyValue(&setval);
        return CreateDummyIndexType(intset);
        /* damn thing ends up in typedesc of arrays. */
      }
    case integer_value:
      DestroyValue(&value);
      return CreateIndexType(CopySetList(sptr),1);
    case symbol_value:
      DestroyValue(&value);
      return CreateIndexType(CopySetList(sptr),0);
    case error_value:
      switch(ErrorValue(value)){
      case undefined_value:
        if (StatementType(stat)==REL||StatementType(stat)==LOGREL) {
          WSSM(ASCERR,stat,"Undefined relation array indirect indices",3);
          /* don't want to warn about sparse IS_A/aliases here */
        }
        break;
      case name_unfound:
        break;
      default:
        WSSM(ASCERR,stat, "Error in sparse array indices",3);
        break;
      }
      DestroyValue(&value);
      return NULL;
    default:
      STATEMENT_ERROR(stat, "Unknown result value type in MakeIndex.\n");
      ASC_PANIC("Unknown result value type in MakeIndex.\n");
    }
  }else{ /* checking subscripts on dense ALIASES/param'd IS_A statement */
    if (sptr==NULL) {
      SetEvaluationContext(NULL);
      return NULL;
    }
    value = EvaluateSet(sptr,InstanceEvaluateName);
    SetEvaluationContext(NULL);
    switch(ValueKind(value)){
    case list_value:
      DestroyValue(&value);
      return CreateDummyIndexType(0 /* doesn't matter -- dense alias check */);
    case error_value:
      switch(ErrorValue(value)){
        case undefined_value:
        case name_unfound:
          DestroyValue(&value);
          return NULL;
      default:
        DestroyValue(&value);
        WSSM(ASCERR,stat, "Error evaluating index to dense array",3);
        return NULL;
      }
    default:
      DestroyValue(&value);
      STATEMENT_ERROR(stat, "Bad index to dense alias array");
      ASC_PANIC("Bad index to dense alias array");
    }
    /* return NULL; */  /* unreachable */
    /* FIXME add GCC flags to tell compiler that we never get here? */
  }
}

/**
	This function is used for making the indices of individual
	elements of sparse arrays (and for checking that it is possible)
	and for checking that the indices of dense alias arrays (a
	very wierd thing to have) and dense parameterized IS_A
	are fully defined so that aliases
	and parameterized/sparse IS_A can be fully constructed in 1 pass.
	paves over the last subscript on sparse ALIASES-IS_A.
*/
static
struct gl_list_t *MakeIndices(struct Instance *inst,
                              CONST struct Name *name,
                              struct Statement *stat)
{
  struct gl_list_t *result;
  CONST struct Set *sptr;
  struct IndexType *ptr;
  int last;


  result = gl_create((unsigned long)NameLength(name));
  while(name != NULL){
    if (NameId(name)){
      DestroyIndexList(result);
      return NULL;
    }
    sptr = NameSetPtr(name);
    last = (NextName(name)==NULL && StatementType(stat)==ARR);
    ptr = MakeIndex(inst,sptr,stat,last);
    if (ptr !=  NULL) {
      gl_append_ptr(result,(VOIDPTR)ptr);
    }else{
      DestroyIndexList(result);
      return NULL;
    }
    name = NextName(name);
  }
  return result;
}

static
void LinkToParentByName(struct Instance *inst,
                        struct Instance *child,
                        symchar *name)
{
  struct InstanceName rec;
  unsigned long pos;
  SetInstanceNameType(rec,StrName);
  SetInstanceNameStrPtr(rec,name);
  pos = ChildSearch(inst,&rec);
  LinkToParentByPos(inst,child,pos);
}

void LinkToParentByPos(struct Instance *inst,
                       struct Instance *child,
                       unsigned long pos)
{
  asc_assert(pos);
  asc_assert(child != NULL);
  asc_assert(inst != NULL);

  StoreChildPtr(inst,pos,child);
  AddParent(child,inst);
}

static
struct Instance *GetArrayHead(struct Instance *inst, CONST struct Name *name)
{
  struct InstanceName rec;
  unsigned long pos;
  if (NameId(name)){
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,NameIdPtr(name));
    pos=ChildSearch(inst,&rec);
    if (pos>0) {
      return InstanceChild(inst,pos);
    }else{
      return NULL;
    }
  }
  return NULL;
}

/**
	We are inside a FOR loop.
	If rhsinst is not null, we are in an alias statement and
	will use rhsinst as the child added instead of
	creating a new child.
	If arginst is not null, we will use it to aid in
	creating IS_A elements.
	at least one of arginst, rhsinst must be NULL.
	If last !=0, returns NULL naturally and ok.
*/
static
struct Instance *DoNextArray(struct Instance *parentofary, /* MODEL */
                             struct Instance *ptr, /* array layer */
                             CONST struct Name *name, /* subscript */
                             struct Statement *stat,
                             struct Instance *rhsinst, /*ALIASES*/
                             struct Instance *arginst, /* IS_A */
                             struct gl_list_t *rhslist, /*ARR*/
                             int last /* ARR */)
{
  CONST struct Set *sptr;
  struct value_t value;
  struct value_t setval;
  long i;
  symchar *sym;

  if (NameId(name) != 0) return NULL; /* must be subscript, i.e. set */
  sptr = NameSetPtr(name);
  if ((sptr==NULL)||(NextSet(sptr)!=NULL)||(SetType(sptr))) {
    return NULL;
  }
  asc_assert(GetEvaluationContext()==NULL);
  asc_assert(rhsinst==NULL || arginst==NULL);
  SetEvaluationContext(parentofary);
  value = EvaluateExpr(GetSingleExpr(sptr),NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case real_value:
  case set_value:
  case boolean_value:
  case list_value:
    if (last==0) {
      STATEMENT_ERROR(stat, "Index to array is of an incorrect type");
      DestroyValue(&value);
      return NULL;
    }else{
      /* we are at last subscript of ALIASES/IS_A in for loop. */
      /* expand using rhslist pretending dense array. */
      setval = CreateSetFromList(value);
      ExpandArray(ptr,1L,SetValue(setval),NULL,NULL,rhslist);
      DestroyValue(&setval);
      DestroyValue(&value);
      return NULL;
    }
  case integer_value:
    i = IntegerValue(value);
    DestroyValue(&value);
    return FindOrAddIntChild(ptr,i,rhsinst,arginst);
  case symbol_value:
    sym = SymbolValue(value);
    DestroyValue(&value);
    return FindOrAddStrChild(ptr,sym,rhsinst,arginst);
  case error_value:
    switch(ErrorValue(value)){
    case undefined_value:
      if (StatementType(stat)==REL||StatementType(stat)==LOGREL) {
        WSSM(ASCERR,stat, "Undefined relation array indirect indices",3);
      }
      break;
    case name_unfound:
      break;
    default:
      STATEMENT_ERROR(stat, "Error in array indices");
      break;
    }
    DestroyValue(&value);
    return NULL;
  default:
    ASC_PANIC("Unknown result value type.\n");

  }
}

/**
	We are inside a FOR loop.
	If rhsinst is not null, we are in an alias statement and
	will eventually use rhsinst as the child added instead of
	creating a new child.
	we expand each subscript individually here rahter than recursively.
	If we are on last subscript of an ALIASES/IS_A, we copy the
	layer in rhslist rather than expanding individually.
	rhslist and intset only make sense simultaneously.
*/
static
struct Instance *AddArrayChild(struct Instance *parentofary,
                               CONST struct Name *name,
                               struct Statement *stat,
                               struct Instance *rhsinst,
                               struct Instance *arginst,
                               struct gl_list_t *rhslist)
{
  struct Instance *ptr;
  int last;

  ptr = GetArrayHead(parentofary,name);
  if(ptr != NULL) {
    name = NextName(name);
    while(name!=NULL){
      last = (rhslist != NULL && NextName(name)==NULL);
      ptr = DoNextArray(parentofary,ptr,name,stat,
                        rhsinst,arginst,rhslist,last);
      if (ptr==NULL){
        return NULL;
      }
      name = NextName(name);
    }
    return ptr;
  }else{
    return NULL;
  }
}

/**
	Create the sparse array typedesc based on the statement kind
	and also add first child named. intset and def used for nonrelation types
	only.

	This function returns the child pointer because relation functions
	need it, not because the child is unconnected.

	If rhsinst is not NULL, uses rhsinst instead of creating new one.
	If rhslist is not NULL, uses rhslist instead of rhsinst or creating.

	It is expected that all subscripts will be evaluatable and that
	in the case of the ALIASES-IS_A statement, the IS_A part is done
	just before the ALIASES part.
*/
static
struct Instance *MakeSparseArray(struct Instance *parent,
                                 CONST struct Name *name,
                                 struct Statement *stat,
                                 struct TypeDescription *def,
                                 int intset,
                                 struct Instance *rhsinst,
                                 struct Instance *arginst,
                                 struct gl_list_t *rhslist)
{
  struct TypeDescription *desc = NULL;
  struct Instance *aryinst;
  struct gl_list_t *indices;
  indices = MakeIndices(parent,NextName(name),stat);
  if (indices != NULL) {
    switch (StatementType(stat)) {
    case REL:
      asc_assert(def==NULL && rhsinst==NULL && rhslist == NULL && arginst == NULL);
      desc = CreateArrayTypeDesc(StatementModule(stat),FindRelationType(),
                                 0,1,0,0,indices);
      break;
    case EXT:
      asc_assert(def==NULL && rhsinst==NULL && rhslist == NULL && arginst == NULL);
      desc = CreateArrayTypeDesc(StatementModule(stat),FindRelationType(),
                                 0,1,0,0,indices);
      break;
    case LOGREL:
      asc_assert(def==NULL && rhsinst==NULL && rhslist == NULL && arginst == NULL);
      desc = CreateArrayTypeDesc(StatementModule(stat),FindLogRelType(),
                                 0,0,1,0,indices);
      break;
    case WHEN:
      asc_assert(def==NULL && rhsinst==NULL && rhslist == NULL && arginst == NULL);
      desc = CreateArrayTypeDesc(StatementModule(stat),
                                 FindWhenType(),0,0,0,1,indices);
      break;
    case ISA:
    case ALIASES:
    case ARR:
      asc_assert(def!=NULL);
      desc = CreateArrayTypeDesc(StatementModule(stat),def,
                                 intset,0,0,0,indices);
      break;
    default:
      STATEMENT_ERROR(stat, "Utter screw-up in MakeSparseArray");
      ASC_PANIC("Utter screw-up in MakeSparseArray");
    }
    aryinst = CreateArrayInstance(desc,1);
    LinkToParentByName(parent,aryinst,NameIdPtr(name));
    return AddArrayChild(parent,name,stat,rhsinst,arginst,rhslist);
  }else{
    return NULL;
  }
}

/*------------------------------------------------------------------------------
	...
*/

/**
	handles construction of alias statements, allegedly, per lhs.
	parent function should find rhs and send it in as rhsinst.
	rhsinst == null should never be used with this function.
	currently, arrays ignored, fatally.
*/
static
void MakeAliasInstance(CONST struct Name *name,
                       CONST struct TypeDescription *basedef,
                       struct Instance *rhsinst,
                       struct gl_list_t *rhslist,
                       int intset,
                       struct Instance *parent,
                       struct Statement *statement)
{
  symchar *childname;
  int changed;
  unsigned long pos;
  struct Instance *inst;
  struct InstanceName rec;
  struct TypeDescription *arydef, *def;
  struct gl_list_t *indices;
  int tce;
  asc_assert(rhsinst != NULL || rhslist !=NULL); /* one required */
  asc_assert(rhsinst == NULL || rhslist ==NULL); /* only one allowed */
  childname = SimpleNameIdPtr(name);
  if (childname !=NULL){
    /* case of simple part name */
    if (StatInFOR(statement) && StatWrong(statement)==0) {
      MarkStatContext(statement,context_WRONG);
      STATEMENT_ERROR(statement,"Unindexed statement in FOR loop not allowed.");
      WSS(ASCERR,statement);
      return;
    }
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,childname);
    pos = ChildSearch(parent,&rec);
    if (pos>0){
      /* case of part expected */
      if (InstanceChild(parent,pos)==NULL){
        /* case of part not there yet */
        inst = rhsinst;
        StoreChildPtr(parent,pos,inst);
        if (SearchForParent(inst,parent)==0) {
          /* case where we don't already have it at this scope */
          AddParent(inst,parent);
        }
      }else{			/* redefining instance */
        /* case of part already there and we barf */
        char *msg = ASC_NEW_ARRAY(char,SCLEN(childname)+strlen(REDEFINE_CHILD_MESG2)+1);
        strcpy(msg,REDEFINE_CHILD_MESG2);
        strcat(msg,SCP(childname));
        STATEMENT_ERROR(statement,msg);
        ascfree(msg);
      }
    }else{			/* unknown child name */
      /* case of part not expected */
      STATEMENT_ERROR(statement, "Unknown child name.  Never should happen");
      ASC_PANIC("Unknown child name.  Never should happen");
    }
  }else{
    /* if reach the else, means compound identifier or garbage */
    indices = ArrayIndices(name,parent);
    if (rhsinst != NULL) {
      def = InstanceTypeDesc(rhsinst);
    }else{
      def = (struct TypeDescription *)basedef;
    }
    if (indices!=NULL){ /* array of some sort */
      childname = NameIdPtr(name);
      SetInstanceNameType(rec,StrName);
      SetInstanceNameStrPtr(rec,childname);
      pos = ChildSearch(parent,&rec);
      if (!StatInFOR(statement)) {
        /* rectangle arrays */
        arydef = CreateArrayTypeDesc(StatementModule(statement),
                                     def,intset,0,0,0,indices);
        if (pos>0) {
          inst = CreateArrayInstance(arydef,1);
          if (inst!=NULL){
            changed = 0;
            tce = TryChildExpansion(inst,parent,&changed,rhsinst,NULL,rhslist);
            /* we're not in a for loop, so can't fail unless user is idiot. */
            LinkToParentByPos(parent,inst,pos); /* don't want to lose memory */
            /* if user is idiot, whine. */
            if (tce != 0) {
              SignalChildExpansionFailure(parent,pos);
            }
          }else{
            STATEMENT_ERROR(statement, "Unable to create alias array instance");
            ASC_PANIC("Unable to create alias array instance");
          }
        }else{
          DeleteTypeDesc(arydef);
          STATEMENT_ERROR(statement,
               "Unknown array child name. Never should happen");
          ASC_PANIC("Unknown array child name. Never should happen");
        }
      }else{
        /* sparse array */
        DestroyIndexList(indices);
        if (pos>0) {
          if (InstanceChild(parent,pos)==NULL) {
            /* need to make alias array */
            /* should check for NULL return here */
            (void)
            MakeSparseArray(parent,name,statement,def,
                            intset,rhsinst,NULL,rhslist);
          }else{
            /* need to add alias array element */
            /* should check for NULL return here */
            (void) AddArrayChild(parent,name,statement,
                                 rhsinst,NULL,rhslist);
          }
        }else{
          STATEMENT_ERROR(statement,
            "Unknown array child name. Never should happen");
          ASC_PANIC("Unknown array child name. Never should happen");
        }
      }
    }else{
      /* bad child name. cannot create parts of parts. should never
       * happen, being trapped out in typelint.
       */
      STATEMENT_ERROR(statement,"Bad ALIASES child name.");
    }
  }
}

/**
	@return 1 if concluded with statement, 0 if might try later.
*/
static
int ExecuteALIASES(struct Instance *inst, struct Statement *statement)
{
  CONST struct VariableList *vlist;
  struct gl_list_t *rhslist;
  struct Instance *rhsinst;
  CONST struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  int intset;

  asc_assert(StatementType(statement)==ALIASES);
  if (StatWrong(statement)) {
    /* incorrect statements should be warned about when they are
     * marked wrong, so we just ignore them here.
     */
    return 1;
  }
  if (!CheckALIASES(inst,statement)) {
    WriteUnexecutedMessage(ASCERR,statement,
      "Possibly undefined sets/ranges in ALIASES statement.");
    return 0;
  }
  name = AliasStatName(statement);
  rhslist = FindInstances(inst,name,&err);
  if (rhslist == NULL) {
    WriteUnexecutedMessage(ASCERR,statement,
      "Possibly undefined right hand side in ALIASES statement.");
    return 0; /* rhs not compiled yet */
  }
  if (gl_length(rhslist)>1) {
    STATEMENT_ERROR(statement,"ALIASES needs exactly 1 RHS");
    gl_destroy(rhslist);
    return 1; /* rhs not unique for current values of sets */
  }
  rhsinst = (struct Instance *)gl_fetch(rhslist,1);
  gl_destroy(rhslist);
  if (InstanceKind(rhsinst)==REL_INST || LREL_INST ==InstanceKind(rhsinst)) {
    STATEMENT_ERROR(statement,"Direct ALIASES of relations are not permitted");
    MarkStatContext(statement,context_WRONG);
    WSS(ASCERR,statement);
    return 1; /* relations only aliased through models */
  }
  intset = ( (InstanceKind(rhsinst)==SET_ATOM_INST) &&
             (IntegerSetInstance(rhsinst)) );
  vlist = GetStatVarList(statement);
  while (vlist!=NULL){
    MakeAliasInstance(NamePointer(vlist),NULL,rhsinst,
                      NULL,intset,inst,statement);
    vlist = NextVariableNode(vlist);
  }
  return 1;
}


/*------------------------------------------------------------------------------
  SUPPORT FOR ALIASES-IS_A STATEMENTS
*/

/**
	enforce max len and no-apostrope (') rules for subscripts. string returned
	may not be string sent.
*/
static
char *DeSingleQuote(char *s)
{
  char *old;
  int len;
  if (s==NULL) {
    return s;
  }
  len = strlen(s);
  if (len > 40) {
    old = s;
    s = ASC_NEW_ARRAY(char,41);
    strncpy(s,old,17);
    s[17] = '.';
    s[18] = '.';
    s[19] = '.';
    s[20] = '\0';
    strcat(s,(old+len-20));
    ascfree(old);
  }
  old = s;
  while (*s != '\0') {
    if (*s =='\'') {
      *s = '_';
    }
    s++;
  }

  return old;
}

/**
	returns a symchar based on but not in strset,
	and adds original and results to sym table.
	destroys the s given.
*/
static
symchar *UniquifyString(char *s, struct set_t *strset)
{
  int oldlen, maxlen, c;
  char *new;
  symchar *tmp;

  tmp = AddSymbol(s);
  if (StrMember(tmp,strset)!=0) {
    oldlen = strlen(s);
    maxlen = oldlen+12;
    new = ascrealloc(s,oldlen+14);
    asc_assert(new!=NULL);
    while ( (oldlen+1) < maxlen) {
      new[oldlen+1] = '\0';
      for(c = 'a'; c <= 'z'; c++){
        new[oldlen] = (char)c;
        tmp = AddSymbol(new);
        if (StrMember(tmp,strset)==0) {
          ascfree(new);
          return tmp;
        }
      }
      oldlen++;
    }
    Asc_Panic(2, NULL,
              "Unable to generate unique compound alias subscript.\n");

  }else{
    ascfree(s);
    return tmp;
  }
}

static
struct value_t GenerateSubscripts(struct Instance *iref,
                                  struct gl_list_t *rhslist,
                                  int intset)
{
  struct set_t *setinstval;
  unsigned long c,len;
  char *str;
  symchar *sym;

  setinstval = CreateEmptySet();
  len = gl_length(rhslist);
  if (intset!=0) {
    /* create subscripts 1..rhslistlen */
    for (c=1;c<=len; c++) {
      AppendIntegerElement(setinstval,c);
    }
    return CreateSetValue(setinstval);
  }
  /* create string subscripts */
  for (c=1; c<= len; c++) {
    str = WriteInstanceNameString((struct Instance *)gl_fetch(rhslist,c),iref);
    str = DeSingleQuote(str); /* transmogrify for length and ' marks */
    sym = UniquifyString(str,setinstval); /* convert to symbol and free str */
    AppendStringElement(setinstval,sym);
  }
  return CreateSetValue(setinstval);
}

static
void DestroyArrayElements(struct gl_list_t *rhslist)
{
  unsigned long c,len;
  if (rhslist==NULL){
    return;
  }
  for (c=1, len = gl_length(rhslist); c <= len; c++) {
    FREEPOOLAC(gl_fetch(rhslist,c));
  }
  gl_destroy(rhslist);
}

/**
	this function computes the subscript set (or generates it if
	needed) and checks it for matching against the instance list
	and whines when things aren't kosher.
	When things are kosher, creates a gl_list of array children.
	This list is returned through rhslist.
*/
static
struct value_t ComputeArrayElements(struct Instance *inst,
                                    struct Statement *statement,
                                    struct gl_list_t *rhsinstlist,
                                    struct gl_list_t **rhslist)
{
  struct value_t subslist;
  struct value_t subscripts;
  struct value_t result; /* return value is the expanded subscript set */
  CONST struct Set *setp;
  struct set_t *sip;
  int intset;
  unsigned long c, len;
  struct ArrayChild *ptr;

  asc_assert((*rhslist)==NULL && rhsinstlist != NULL && rhslist != NULL);

  intset = ArrayStatIntSet(statement);
  len = gl_length(rhsinstlist);
  setp = ArrayStatSetValues(statement);
  if (setp==NULL) {
    /* value generated is a set and automatically is of correct CARD() */
    result = GenerateSubscripts(inst,rhsinstlist,intset);
    /* fill up rhslist and return */
    *rhslist = gl_create(len);
    sip = SetValue(result);
    if (intset != 0) {
      for (c = 1; c <= len; c++) {
        ptr = MALLOCPOOLAC;
        ptr->inst = gl_fetch(rhsinstlist,c);
        ptr->name.index = FetchIntMember(sip,c);
        gl_append_ptr(*rhslist,(VOIDPTR)ptr);
      }
    }else{
      for (c = 1; c <= len; c++) {
        ptr = MALLOCPOOLAC;
        ptr->inst = gl_fetch(rhsinstlist,c);
        ptr->name.str = FetchStrMember(sip,c);
        gl_append_ptr(*rhslist,(VOIDPTR)ptr);
      }
    }
    return result;
  }else{
    /* cook up the users list */
    asc_assert(GetEvaluationContext()==NULL);
    SetEvaluationContext(inst);
    subslist = EvaluateSet(setp,InstanceEvaluateName);
    SetEvaluationContext(NULL);
    /* check that it evaluates */
    if (ValueKind(subslist)==error_value) {
      switch(ErrorValue(subslist)) {
      case name_unfound:
      case undefined_value:
        DestroyValue(&subslist);
        WriteUnexecutedMessage(ASCERR,statement,
          "Undefined values in WITH_VALUE () list");
        return CreateErrorValue(undefined_value);
      default:
        STATEMENT_ERROR(statement,"Bad result in evaluating WITH_VALUE list\n");
        MarkStatContext(statement,context_WRONG);
        WSS(ASCERR,statement);
        DestroyValue(&subslist);
      }
    }
    /* collect sets to assign later */
    result = CreateSetFromList(subslist); /* unique list */
    ListMode=1;
    subscripts = CreateOrderedSetFromList(subslist); /* as ordered to insts */
    ListMode=0;
    DestroyValue(&subslist); /* done with it */
    /* check everything dumb that can happen */
    if ( ValueKind(result) != set_value ||
         Cardinality(SetValue(subscripts)) != Cardinality(SetValue(result))
       ) {
      DestroyValue(&result);
      DestroyValue(&subscripts);
      STATEMENT_ERROR(statement,
        "WITH_VALUE list does not form a proper subscript set.\n");
      MarkStatContext(statement,context_WRONG);
      WSS(ASCERR,statement);
      return CreateErrorValue(type_conflict);
    }
    /* check sanity of values. may need fixing around empty set. */
    if ( (SetKind(SetValue(subscripts))==integer_set) != (intset!=0)) {
      STATEMENT_ERROR(statement,
        "Unable to construct set. Values and set type mismatched\n");
      DestroyValue(&result);
      DestroyValue(&subscripts);
      MarkStatContext(statement,context_WRONG);
      WSS(ASCERR,statement);
      return CreateErrorValue(type_conflict);
    }
    /* check set size == instances to alias */
    if (Cardinality(SetValue(subscripts)) != len) {
      STATEMENT_ERROR(statement,"In: ");
      FPRINTF(ASCERR,
        "WITH_VALUE list length (%lu) != number of instances given (%lu)\n",
        Cardinality(SetValue(subscripts)),len);
      DestroyValue(&result);
      DestroyValue(&subscripts);
      MarkStatContext(statement,context_WRONG);
      WSS(ASCERR,statement);
      return CreateErrorValue(type_conflict);
    }
    /* fill up rhslist and return */
    *rhslist = gl_create(len);
    sip = SetValue(subscripts);
    if (intset != 0) {
      for (c = 1; c <= len; c++) {
        ptr = MALLOCPOOLAC;
        ptr->inst = gl_fetch(rhsinstlist,c);
        ptr->name.index = FetchIntMember(sip,c);
        gl_append_ptr(*rhslist,(VOIDPTR)ptr);
      }
    }else{
      for (c = 1; c <= len; c++) {
        ptr = MALLOCPOOLAC;
        ptr->inst = gl_fetch(rhsinstlist,c);
        ptr->name.str = FetchStrMember(sip,c);
        gl_append_ptr(*rhslist,(VOIDPTR)ptr);
      }
    }
    DestroyValue(&subscripts);
    return result;
  }
}

/**
	@return 1 if concluded with statement, 0 if might try later.
*/
static
int ExecuteARR(struct Instance *inst, struct Statement *statement)
{
  CONST struct VariableList *vlist;
  struct gl_list_t *rhsinstlist; /* list of instances found to alias */
  struct gl_list_t *setinstl; /* instance found searching for IS_A'd set */
  struct gl_list_t *rhslist=NULL; /* list of arraychild structures */
  struct value_t subsset;
#ifndef NDEBUG
  struct Instance *rhsinst;
#endif
  struct Instance *setinst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  CONST struct TypeDescription *basedef;
  ChildListPtr icl;
  int intset;

  asc_assert(StatementType(statement)==ARR);
  if (StatWrong(statement)) {
    /* incorrect statements should be warned about when they are
     * marked wrong, so we just ignore them here.
     */
    return 1;
  }
  if (!CheckARR(inst,statement)) {
    WriteUnexecutedMessage(ASCERR,statement,
      "Possibly undefined instances/sets/ranges in ALIASES-IS_A statement.");
    return 0;
  }
  rhsinstlist = FindInsts(inst,GetStatVarList(statement),&err);
  if (rhsinstlist == NULL) {
    MissingInsts(inst,GetStatVarList(statement),0);
    WriteUnexecutedMessage(ASCERR,statement,
      "Incompletely defined source instance list in ALIASES-IS_A statement.");
    return 0; /* rhs's not compiled yet */
  }
  /* check for illegal rhs types. parser normally bars this. */
#ifndef NDEBUG
  if (gl_length(rhsinstlist) >0) {
    rhsinst = (struct Instance *)gl_fetch(rhsinstlist,1);
    if (BaseTypeIsEquation(InstanceTypeDesc(rhsinst))) {
      STATEMENT_ERROR(statement,
        "Direct ALIASES of rels/lrels/whens are not permitted");
      MarkStatContext(statement,context_WRONG);
      WSS(ASCERR,statement);
      gl_destroy(rhsinstlist);
      return 1; /* (log)relations/whens only aliased through models */
    }
  }
#endif
  /* evaluate name list, if given, OTHERWISE generate it, and check CARD.
   * issues warnings as needed
   */
  subsset = ComputeArrayElements(inst,statement,rhsinstlist,&rhslist);
  gl_destroy(rhsinstlist);
  /* check return values of subsset and rhslist here */
  if (ValueKind(subsset)== error_value) {
    if (ErrorValue(subsset) == undefined_value) {
      DestroyValue(&subsset);
      return 0;
    }else{
      DestroyValue(&subsset);
      return 1;
    }
  }
  asc_assert(rhslist!=NULL); /* might be empty, but not NULL */
  /* make set ATOM */
  vlist = ArrayStatSetName(statement);
  intset = ArrayStatIntSet(statement);
  MakeInstance(NamePointer(vlist),FindSetType(),intset,inst,statement,NULL);
  /* get instance  and assign. */
  setinstl = FindInstances(inst,NamePointer(vlist),&err);
  if (setinstl == NULL || gl_length(setinstl) != 1L) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to construct set. Bizarre error in ALIASES-IS_A.");
    if (setinstl!=NULL) {
      gl_destroy(setinstl);
    }
    DestroyArrayElements(rhslist);
    DestroyValue(&subsset);
    MarkStatContext(statement,context_WRONG);
    WSS(ASCERR,statement);
    /* should nuke entire compound ALIASES/IS_A array pair already built */
    return 1;
  }else{
    setinst = (struct Instance *)gl_fetch(setinstl,1);
    gl_destroy(setinstl);
    AssignSetAtomList(setinst,CopySet(SetValue(subsset)));
    DestroyValue(&subsset);
  }

  /* create  ALIASES-IS_A array */
  /* recycle the local pointer to our set ATOM to check base type of rhslist */
  setinst = CAC(gl_fetch(rhslist,1))->inst;
  intset = ( InstanceKind(setinst)==SET_ATOM_INST &&
             IntegerSetInstance(setinst)!=0 );
  /* the real question is does anyone downstream care if intset correct?
   * probably not since its an alias anyway.
   */
  vlist = ArrayStatAvlNames(statement);
  icl = GetChildList(InstanceTypeDesc(inst));
  basedef = ChildBaseTypePtr(icl,ChildPos(icl,NameIdPtr(NamePointer(vlist))));
  while (vlist!=NULL){
    /* fix me for sparse case. dense ok. */
    MakeAliasInstance(NamePointer(vlist), basedef,NULL,
                      rhslist, intset, inst, statement);
    vlist = NextVariableNode(vlist);
  }
  /* clean up memory */
  DestroyArrayElements(rhslist);

  return 1;
}

/*------------------------------------------------------------------------------
	...
*/
/**
	Makes a single instance of the type given,which must not be array
	or relation of any kind or when.

	If type is a MODEL, adds the MODEL to pending list.

	The argument intset is only used if type is set, then
	if intset==1, set ATOM made will be integer set.

	Attempts to find a UNIVERSAL before making the instance.

	@param statement only used for error messages.
*/
static
struct Instance *MakeSimpleInstance(struct TypeDescription *def,
                                    int intset,
                                    struct Statement *statement,
                                    struct Instance *arginst)
{
  struct Instance *inst;

  inst = ShortCutMakeUniversalInstance(def);
  if (inst==NULL) {
    switch(GetBaseType(def)){
    case model_type:
      inst = CreateModelInstance(def);  /* if we are here - build one */
      if (!GetUniversalFlag(def)||!InstanceInList(inst)) {
        /* add PENDING model if not UNIVERSAL, or UNIVERSAL and
         * this is the very first time seen - don't ever want an instance
         * in the pending list twice.
         */
        /*
         * here we need to shuffle in info from arginst.
         * note that because this is inside the UNIVERSAL check,
         * only the first set of arguments to a UNIVERSAL type will
         * ever apply.
         */
        ConfigureInstFromArgs(inst,arginst);
        AddBelow(NULL,inst);
      }
      break;
    case real_type:
    case real_constant_type:
      inst = CreateRealInstance(def);
      break;
    case boolean_type:
    case boolean_constant_type:
      inst = CreateBooleanInstance(def);
      break;
    case integer_type:
    case integer_constant_type:
      inst = CreateIntegerInstance(def);
      break;
    case set_type:
      inst = CreateSetInstance(def,intset);
      break;
    case symbol_type:
    case symbol_constant_type:
      inst = CreateSymbolInstance(def);
      break;
    case relation_type:
      inst = NULL;
      FPRINTF(ASCERR,"Type '%s' is not allowed in IS_A.\n",
              SCP(GetBaseTypeName(relation_type)));
    case logrel_type:
      inst = NULL;
      FPRINTF(ASCERR,"Type '%s' is not allowed in IS_A.\n",
              SCP(GetBaseTypeName(logrel_type)));
      break;
    case when_type:
      inst = NULL;
      FPRINTF(ASCERR,"Type '%s' is not allowed in IS_A.\n",
              SCP(GetBaseTypeName(when_type)));
      break;
    case array_type:
    default: /* picks up patch_type */
      STATEMENT_ERROR(statement, "MakeSimpleInstance error. PATCH/ARRAY found.\n");
      ASC_PANIC("MakeSimpleInstance error. PATCH/ARRAY found.\n");
    }
  }
  return inst;
}

static unsigned long g_unasscon_count = 0L;
/* counter for the following functions */
static
void CountUnassignedConst(struct Instance *i)
{
  if (i!=NULL && (IsConstantInstance(i) || InstanceKind(i)==SET_ATOM_INST) ) {
    if (AtomAssigned(i)==0) {
      g_unasscon_count++;
    }
  }
}
/**
	Returns 0 if all constant scalars in ipass are assigned,
	for ipass that are of set/scalar array/scalar type.
	Handles null input gracefully, as if there is something
	unassigned in it.
	Variable types are considered permanently assigned, since
	we are checking for constants being unassigned.
	Assumes arrays, if passed in, are fully expanded.
*/
static
int ArgValuesUnassigned(struct Instance *ipass)
{
  struct TypeDescription *abd;
  if (ipass==NULL) return 1;
  switch (InstanceKind(ipass)) {
  case ERROR_INST:
   return 1;
  case SIM_INST:
  case MODEL_INST:
  case REL_INST:
  case LREL_INST:
  case WHEN_INST:
    return 0;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    abd = GetArrayBaseType(InstanceTypeDesc(ipass));
    if (BaseTypeIsConstant(abd)==0 && BaseTypeIsSet(abd)==0) {
      return 0;
    }
    g_unasscon_count = 0;
    SilentVisitInstanceTree(ipass,CountUnassignedConst,0,0);
    if (g_unasscon_count != 0) {
      return 1;
    }else{
      return 0;
    }
  case REAL_INST:
  case INTEGER_INST:
  case BOOLEAN_INST:
  case SYMBOL_INST:
  case SET_INST:
  case REAL_ATOM_INST:
  case INTEGER_ATOM_INST:
  case BOOLEAN_ATOM_INST:
  case SYMBOL_ATOM_INST:
    return 0;
  case SET_ATOM_INST:
  case REAL_CONSTANT_INST:
  case BOOLEAN_CONSTANT_INST:
  case INTEGER_CONSTANT_INST:
  case SYMBOL_CONSTANT_INST:
    return (AtomAssigned(ipass)==0); /* return 0 if assigned, 1 not */
  default:
    return 1; /* NOTREACHED */
  }
}
/**
	Appends the pointers in the set chain s
	into the list given args. args must not be NULL unless s is.

	If needed, args will be expanded, but if you know the length
	to expect, make args of that size before calling and this
	will be faster.

	This does not go into the expressions (which may contain other
	sets themselves) of the set nodes and disassemble them.
	The list may be safely destroyed, but its contents should not
	be destroyed with it as they belong to something else in all
	likelihood.

	@TODO This function should be moved into a set header someplace.
*/
static
void SplitArgumentSet(CONST struct Set *s, struct gl_list_t *args)
{
  struct Set *sp;
  if (s==NULL) return;
  asc_assert(args !=NULL); /* debug WriteSet(ASCERR,s); FPRINTF(ASCERR,"\n"); */
  while (s!=NULL) {
    sp = CopySetNode(s);
    gl_append_ptr(args,(VOIDPTR)sp);
    s = NextSet(s);
  }
}

#define GETARG(l,n) ((struct Set *)gl_fetch((l),(n)))

/**
	Check compatibility of array elements? -- JP

	@return 1 if all ok, 0 if any array child is < type required,
		-1 if some array child is type incompatible with ptype/stype.

	Does some optimization around arrays of sets and array basetypes.
	Doesn't check names.
*/
static
int ArrayElementsTypeCompatible(CONST struct Instance *ipass,
                                CONST struct TypeDescription *ptype,
                                symchar *stype)
{
  struct gl_list_t *achildren=NULL;
  CONST struct TypeDescription *atype;
  CONST struct TypeDescription *mrtype;
  unsigned long c,len,lessrefined=0L;
  struct Instance *i;

  if (ipass==NULL || ptype == NULL) {
    return -1; /* hosed input */
  }
  asc_assert(IsArrayInstance(ipass) != 0);
  atype = GetArrayBaseType(InstanceTypeDesc(ipass));
  if (BaseTypeIsSet(atype)==0 && MoreRefined(atype,ptype)==atype) {
      /* if not set and if array base is good enough */
    return 1;
  }
  achildren = CollectArrayInstances(ipass,NULL);
  len = gl_length(achildren);
  for (c = 1; c <= len; c++) {
    i = (struct Instance *)gl_fetch(achildren,c);
    atype = InstanceTypeDesc(i);
    if (InstanceKind(i) == SET_ATOM_INST) {
      /* both should be of same type "set" */
      if (atype!=ptype ||
          (IntegerSetInstance(i)==0 &&
           stype == GetBaseTypeName(integer_constant_type))
          || (IntegerSetInstance(i)==1 &&
              stype == GetBaseTypeName(symbol_constant_type))
         ) {
        /* set type mismatch */
        gl_destroy(achildren);
        return -1;
      }else{
        /* assumption about arrays of sets being sane, if 1 element is. */
        gl_destroy(achildren);
        return 1;
      }
    }
    if (ptype==atype) {
      continue;
    }
    mrtype = MoreRefined(ptype,atype);
    if (mrtype == NULL) {
      gl_destroy(achildren);
      return -1;
    }
    if (mrtype == ptype) {
      lessrefined++;
    }
  }
  gl_destroy(achildren);
  return (lessrefined==0L); /* if any elements are inadequately refined, 0 */
}

/**
	returns a value_t, but the real result is learned by consulting err.
	err == 0 means some interesting value found.
	err == 1 means try again later
	err == -1 means things are hopeless.
*/
static
struct value_t FindArgValue(struct Instance *parent,
                            struct Set *argset,
                            int *err)
{
  int previous_context;
  struct value_t value;

  asc_assert(err!=NULL);
  *err=0;
  previous_context = GetDeclarativeContext();
  SetDeclarativeContext(0);
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(parent);
  value = EvaluateExpr(GetSingleExpr(argset),
                       NULL,
                       InstanceEvaluateName);
  SetEvaluationContext(NULL);
  SetDeclarativeContext(previous_context);
  if (ValueKind(value)==error_value) {
    switch(ErrorValue(value)){
    case name_unfound:
      *err = 1;
      DestroyValue(&value);
      return CreateErrorValue(undefined_value);
    case undefined_value:
      *err = 1;
      return value;
    default:
      *err = -1;
    }
  }
  if (IsConstantValue(value)==0){
    *err = -1;
    DestroyValue(&value);
    return CreateErrorValue(type_conflict);
  }
  return value;
}

/* return codes and message handling for MakeParameterInst */
#define MPIOK 1
#define MPIWAIT 0
#define MPIINPUT -1
#define MPIARGTYPE -2
#define MPIARRINC -3
#define MPIBADASS -4
#define MPIARRRNG -5
#define MPIINSMEM -6
#define MPIBADARG -7
#define MPIMULTI -8
#define MPIBADVAL -9
#define MPIWEIRD -10
#define MPIUNMADE -11
#define MPIWEAKTYPE -12
#define MPIUNASSD -13
#define MPIARGVAL -14
#define MPIARGSIZ -15
#define MPIBADWBTS -16
#define MPIBADWNBTS -17
#define MPIBADMERGE -18
#define MPIREASGN -19
#define MPIREDEF -20
#define MPIFOR -21
#define MPIBADREL -22
#define MPIEXCEP -23
#define MPIVARREL -24
#define MPINOTBOOL -25
static
char *g_mpi_message[] = {
/* 0 */  "Nothing wrong with parameter",
/* -1 */ "Bad input statement or parent or arginstptr.",
/* -2 */ "Incompatible argument type.",
/* -3 */ "Incomplete assignment of absorbed pass-by-value array.",
/* -4 */ "Error in absorbed assignment RHS.",
/* -5 */ "Mismatch in range of array subscripts.",
/* -6 */ "Insufficient memory - crashing soon",
/* -7 */ "Nonexistent argument. (bad set in array expression, probably)",
/* -8 */ "Too many instances named for 1 parameter slot",
/* -9 */ "Bad expression passed to IS_A",
/* -10 */ "Something rotten in lint",
/* -11 */ "Instance doesn't yet exist",
/* -12 */ "Instance not sufficiently refined",
/* -13 */ "Argument value not assigned",
/* -14 */ "Argument value != required value",
/* -15 */ "Array object given has with too many/too few subscripts.",
/* -16 */ "Incorrect instance named in WILL_BE_THE_SAME.",
/* -17 */ "Nonexistent instance named in WILL_NOT_BE_THE_SAME.",
/* -18 */ "Merged instances found in WILL_NOT_BE_THE_SAME.",
/* -19 */ "Refinement cannot reassign constant value.",
/* -20 */ "Refinement must pass in same objects used in IS_A.",
/* -21 */ "Improper FOR loop in WHERE statements",
/* -22 */ "WHERE condition unsatisfied",
/* -23 */ "WHERE condition incorrect (system exception occurred)",
/* -24 */ "WHERE condition incorrect (nonconstant value)",
/* -25 */ "WHERE condition incorrect (nonboolean value)"
};

/**
	Returns MPIOK if value in ipass matches WITH_VALUE field of
	statement, or if the test is silly beacause ipass isn't
	a set/constant or if statement does not constrain value.
	Returns MPIWAIT if statement truth cannot be tested because
	WITH_VALUE clause is not yet evaluatable.
	Returns MPIARGVAL if WITH_VALUE is provably unsatisfied.
	On truly garbage input, unlikely to return.
*/
static
int ArgValueCorrect(struct Instance *inst,
                    struct Instance *tmpinst,
                    CONST struct Statement *statement)
{
  CONST struct Expr *check;
  int previous_context;
  struct value_t value;

  asc_assert(inst!=NULL);
  asc_assert(tmpinst!=NULL);
  asc_assert(statement!=NULL);

  if ( StatementType(statement)!= WILLBE ||
       (check = GetStatCheckValue(statement)) == NULL ||
       ( IsConstantInstance(inst) ==0 &&
         InstanceKind(inst) != SET_ATOM_INST)
     ) {
    return MPIOK;
  }
  if (!AtomAssigned(inst)) {
    return MPIWAIT;
  }
  previous_context = GetDeclarativeContext();
  SetDeclarativeContext(0);
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(tmpinst);
  value = EvaluateExpr(check, NULL, InstanceEvaluateName);
  SetEvaluationContext(NULL);
  SetDeclarativeContext(previous_context);
  if (ValueKind(value)==error_value) {
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      return MPIWAIT;
    default:
      DestroyValue(&value);
      return MPIARGVAL;
    }
  }
  if (IsConstantValue(value)==0){
    DestroyValue(&value);
    FPRINTF(ASCERR,"Variable value found where constant required");
    return MPIARGVAL;
  }
  /* ok, so we have a reasonable inst type and a constant value */
  switch(InstanceKind(inst)){
  case REAL_CONSTANT_INST:
    switch(ValueKind(value)){
    case real_value:
      if ( ( RealValue(value) != RealAtomValue(inst) ||
             !SameDimen(RealValueDimensions(value),RealAtomDims(inst)) )
         ) {
        DestroyValue(&value);
        return MPIARGVAL;
      }
      break;
    case integer_value:
      if ( ( (double)IntegerValue(value) != RealAtomValue(inst) ||
             !SameDimen(Dimensionless(),RealAtomDims(inst)) )
         ) {
        DestroyValue(&value);
        return MPIARGVAL;
      }
      break;
    default:
      DestroyValue(&value);
      return MPIARGVAL;
    }
    break;
  case BOOLEAN_CONSTANT_INST:
    if (ValueKind(value)!=boolean_value ||
        BooleanValue(value) != GetBooleanAtomValue(inst) ) {
      DestroyValue(&value);
      return MPIARGVAL;
    }
    break;
  case INTEGER_CONSTANT_INST:
    switch(ValueKind(value)){
    case integer_value:
      if (GetIntegerAtomValue(inst)!=IntegerValue(value)) {
        DestroyValue(&value);
        return MPIARGVAL;
      }
      break;
    case real_value: /* case which is parser artifact: real, wild 0 */
      if ( RealValue(value)==0.0 &&
           IsWild(RealValueDimensions(value)) &&
           GetIntegerAtomValue(inst) != 0) {
        DestroyValue(&value);
        return MPIARGVAL;
      }
      break;
    default:
      DestroyValue(&value);
      return MPIARGVAL;
    }
    break;
  case SET_ATOM_INST:
    if (ValueKind(value)!=set_value ||
        !SetsEqual(SetValue(value),SetAtomList(inst))) {
      DestroyValue(&value);
      return MPIARGVAL;
    }
    break;
  case SYMBOL_CONSTANT_INST:
    if (ValueKind(value) != symbol_value ||
         SymbolValue(value) != GetSymbolAtomValue(inst)) {
      asc_assert(AscFindSymbol(SymbolValue(value))!=NULL);
      DestroyValue(&value);
      return MPIARGVAL;
    }
    break;
  default:
    DestroyValue(&value);
    return MPIARGVAL;
  }
  DestroyValue(&value);
  return MPIOK;
}

/**
	evaluate a logical or real relation and see that it is satisfied.

	@BUG baa. needs to be exception safe and is not.

	returns MPIOK (satisfied)
	returns MPIBADREL (dissatisified)
	returns MPIVARREL (dissatisified - variable result)
	returns MPIWAIT (not yet determinable)
	returns MPIEXCEP (evaluation is impossible due to float/other error)
	returns MPINOTBOOL (dissatisfied- nonboolean result)

	@param statement should be a rel or logrel.
*/
static
int MPICheckConstraint(struct Instance *tmpinst, struct Statement *statement)
{
  struct value_t value;
  CONST struct Expr *constraint_expr;

  IVAL(value);

  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(tmpinst);
  switch (StatementType(statement)){
  case REL: /* note EXT not allowed in constraint list */
    value = EvaluateExpr(RelationStatExpr(statement),NULL,
                         InstanceEvaluateName);
    break;
  case LOGREL:
    value = EvaluateExpr(LogicalRelStatExpr(statement),NULL,
                         InstanceEvaluateName);
    break;
  default:
    SetEvaluationContext(NULL);
    return MPIWEIRD;
  }
  SetEvaluationContext(NULL);
  switch (ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case undefined_value:
      DestroyValue(&value);
      WriteUnexecutedMessage(ASCERR,statement,
        "Incomplete expression (value undefined) in argument condition.");
      return MPIWAIT;
    case name_unfound:
      DestroyValue(&value);
      WriteUnexecutedMessage(ASCERR,statement,
        "Incomplete expression (name unfound) in argument condition.");
      return MPIWAIT;
    default:
      /* it questionable whether this is a correct action in all cases*/
      /* we could probably turn out more useful error messages here */
      STATEMENT_ERROR(statement, "Condition doesn't make sense.");
      DestroyValue(&value);
      return MPIBADREL;
    }
  case boolean_value:
    if (IsConstantValue(value)!=0) {
      if (BooleanValue(value) != FALSE) {
        DestroyValue(&value);
        return MPIOK;
      }else{
        char *infix = NULL;
        Asc_DString ds;
        DestroyValue(&value);
        constraint_expr = ConstraintStatementExpr(statement);
        Asc_DStringInit(&ds);
        if (constraint_expr != NULL) {
          WriteExprInfix2Str(&ds,constraint_expr);
          infix = Asc_DStringResult(&ds);
        } else {
          Asc_DStringFree(&ds);
        }
        if (infix != NULL) {
          WriteStatementError(ASC_PROG_ERR,statement,0,
            "Parameter requirement evaluated FALSE for the supplied arguments. "
            "Failed requirement: %s. "
            "If this model is instantiated via REFINES, check the values assigned in the REFINES clause.",
            infix
          );
          ASC_FREE(infix);
        } else {
          WriteStatementError(ASC_PROG_ERR,statement,0,
            "Parameter requirement evaluated FALSE for the supplied arguments. "
            "If this model is instantiated via REFINES, check the values assigned in the REFINES clause."
          );
        }
        return MPIBADREL;
      }
    }else{
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Requirements cannot be satisfied by variables");
      return MPIVARREL;
    }
  default:
    DestroyValue(&value);
    STATEMENT_ERROR(statement, "Constraint does not evaluate to boolean result.");
    return MPINOTBOOL;
  }
}

/**
	returns MPIOK if subscripts match declarations,
	MPIWAIT if declarations cannot yet be interpretted,
	or some other error if there is a mismatch.

	So far only the square version. Should have a forvar
	capable recursive version sometime when we allow
	passage of sparse arrays.

	Assumes the array given has proper number of
	subscripts to match name and is fully expanded.
*/
static
int MPICheckSubscripts(struct Instance *tmpinst,
                       struct Instance *aryinst,
                       struct Statement *s)
{
  CONST struct Name *nptr;

  nptr = NextName(NamePointer(GetStatVarList(s)));
  switch (RectangleSubscriptsMatch(tmpinst,aryinst,nptr)) {
  case -2:
    return MPIWAIT;
  case 1:
    return MPIOK;
  case 0:
  default:
    return MPIARRRNG;
  }
}

#define NOIPICHECK 0
#define IPICHECK 1
/**
	links parent and child. if checkdup != 0,
	it will check child to see if it already has this parent.
*/
static
int InsertParameterInst(struct Instance *parent,
                        struct Instance *child,
                        CONST struct Name *name,
                        CONST struct Statement *statement,
                        int checkdup)
{
  symchar *childname;
  struct InstanceName rec;
  unsigned long pos;

  childname = NameIdPtr(name);
  SetInstanceNameType(rec,StrName);
  SetInstanceNameStrPtr(rec,childname);
  pos = ChildSearch(parent,&rec);
  if (pos>0) {
    if (InstanceChild(parent,pos)==NULL) {
      StoreChildPtr(parent,pos,child);
      if (checkdup == 0 || SearchForParent(child,parent)==0) {
          /* case where we don't already have it at this scope */
        AddParent(child,parent);
      }
      return 1;
    }else{			/* redefining instance */
      char *msg = ASC_NEW_ARRAY(char,SCLEN(childname)+strlen(REDEFINE_CHILD_MESG)+1);
      strcpy(msg,REDEFINE_CHILD_MESG);
      strcat(msg,SCP(childname));
      STATEMENT_ERROR(statement,msg);
      ascfree(msg);
      return 0;
    }
  }else{			/* unknown name */
    STATEMENT_ERROR(statement, "Unknown parameter name.  Never should happen");
    ASC_PANIC("Unknown parameter name.  Never should happen");

  }
}

/**
	The instance this is called with should not have
	any parents whatsoever. The instance this is called
	with will be completely destroyed including any parts
	of the instance that do not have other parents.
*/
static
void DestroyParameterInst(struct Instance *i)
{
  DestroyInstance(i,NULL);
}

/**
	destroys everything you send it. If you send some arguments in
	as null, we don't mind.
*/
static
void ClearMPImem(
  struct gl_list_t *args,
  struct gl_list_t *il,
  struct Instance *tmpinst,
  struct Instance *ipass,
  struct value_t *valp
)
{
  if (args!=NULL) {
    gl_iterate(args,(void (*)(VOIDPTR))DestroySetNode);
    gl_destroy(args);
  }
  if (il!=NULL) {
    gl_destroy(il);
  }
  if (tmpinst!=NULL) {
    DestroyParameterInst(tmpinst);
  }
  if (ipass!=NULL) {
    DestroyParameterInst(ipass);
  }
  if (valp!=NULL) {
    DestroyValue(valp);
  }
}

/**
	 Error reporting for 'MakeParameterInstance'
*/
static
void mpierror(struct Set *argset,
              unsigned long argn,
              struct Statement *statement,
              int errcode)
{
  int arrloc;
  if (errcode<0) {
    arrloc = (-errcode);
  }else{
    return;
    /* why are we here? */
  }
  FPRINTF(ASCERR,"Parameter passing error: %s\n",g_mpi_message[arrloc]);
  if (argset !=NULL && argn >0) {
    FPRINTF(ASCERR,"  Argument %lu:",argn);
    WriteSet(ASCERR,argset);
  }
  STATEMENT_ERROR(statement,"Error in executing statement:");
  MarkStatContext(statement,context_WRONG);
  WSS(ASCERR,statement);
}

static
void MPIwum(struct Set *argset,
            unsigned long argn,
            struct Statement *statement,
            int msgcode)
{
  int arrloc;
  if (g_iteration < MAXNUMBER) {
    return;
  }
  if (msgcode<0) {
    arrloc = (-msgcode);
  }else{
    return;
    /* why are we here? */
  }
  ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
  FPRINTF(ASCERR,"Parameter list waiting on sufficient type or value of:\n");
  if (argset !=NULL && argn >0) {
    FPRINTF(ASCERR,"  Argument %lu:",argn);
    WriteSetNode(ASCERR,argset);
  }
  error_reporter_end_flush();
  WriteUnexecutedMessage(ASCERR,statement,g_mpi_message[arrloc]);
}

/**
	process pass by value scalar: evaluate and make it, or return
	appropriate whine if not possible.
	If this returns anything other than mpiok, the user may
	wish to dispose of tmpinst, args as we do not here.
	We do issue whines here, however.
*/
static
int MPIMakeSimple(struct Instance *parent,
                  struct Instance *tmpinst,
                  struct Set *argset,
                  unsigned long argn,
                  CONST struct Name *nptr,
                  struct TypeDescription *ptype,
                  int intset,
                  struct Statement *ps,
                  struct Statement *statement
)
{
  int tverr;	/* error return from checking array elt type, or value */
  struct Instance *ipass;
  struct value_t vpass;

  vpass = FindArgValue(parent,argset,&tverr);
  if (tverr != 0) {
    if (tverr == 1) { /* try later */
      MPIwum(argset,argn,statement,MPIUNASSD);
      return MPIWAIT;
    }else{ /* hopeless */
      mpierror(argset,argn,statement,MPIBADVAL);
      return MPIBADVAL;
    }
  }
  /* don't forget to dispose of vpass if exiting err after here */
  ipass = MakeSimpleInstance(ptype,intset,ps,NULL);
  if (ipass==NULL) {
    DestroyValue(&vpass);
    return MPIINSMEM;
  }
  /* don't forget to dispose of vpass if exiting err after here */
  if (AssignStructuralValue(ipass,vpass,statement)!=1) {
    mpierror(argset,argn,statement,MPIARGTYPE);
    DestroyParameterInst(ipass);
    DestroyValue(&vpass);
    return MPIARGTYPE;
  }
  DestroyValue(&vpass);
  /* install ipass in tmpinst */
  if ( InsertParameterInst(tmpinst,ipass,nptr,ps,IPICHECK) != 1) {
    /* noipicheck because var just created has no parents at all,
     * unless of course var is UNIVERSAL... so ipicheck */
    mpierror(argset,argn,statement,MPIMULTI);
    DestroyParameterInst(ipass);
    return MPIMULTI;
  }
  return MPIOK;
}
#define NOKEEPARGINST 0
#define KEEPARGINST 1
/**
	Check and assemble the arguments of the parameterized type referenced in
	statement, using information derived from the parent instance.

	If the type found in the statement given is not a MODEL type,
	we will immediately return 1 and *arginstptr will be set NULL.

	In general, we are trying to check and assemble enough information
	to prove that a parameterized IS_A can be executed or proved wrong
	once ExecuteISA sees it.

	If keepargs ==KEEPARGINST, then on a successful return,
	*arginstptr will be to a MODEL instance (with no parents)
	with its children derived via parameter list filled in and
	all other children NULL.
	If there are NO children derived via parameter list or
	the reductions list, then *arginstptr will be NULL.
	If keepargs != KEEPARGINST, then arginstptr will not be
	used/set in any way, OTHERWISE it should be NULL on entry.
	If keepargs != KEEPARGINST, then we will do only the minimal
	necessary work to check that the arginst could be created.
	At present, we can't tell what this last ambition amounts to -
	we do the same amount of work regardless, though we try to put
	the more likely to fail steps first.

	A successful return value is 1.

	A failure possibly to succeed later is 0.
	Possible causes will be detailed via the WriteUnexecutedMessage
	facility.

	A permanent failure is any value < 0.
	Causes will be detailed via the WSEM facility, in addition return
	values < 0 have the interpretations given in g_mpi_message[-value]
	above.

	@NOTE assumes statement is well formed, in terms of
	arglist of IS_A/IS_REFINED_TO (if there is one) being of correct length.
	returns fairly quickly for nonmodel and nonparametric
	MODEL types.
*/
static
int MakeParameterInst(struct Instance *parent,
                      struct Statement *statement,
                      struct Instance **arginstptr,
                      int keepargs)
{
  struct TypeDescription *d; /* the type we are constructing or checking */
  struct TypeDescription *atype; /* the type we are being passed */
  struct TypeDescription *ptype; /* the type we are expecting */
  struct TypeDescription *mrtype; /* the more refined of two types */
  symchar *stype;		/* the set type we are expecting */
  struct gl_list_t *args; /* parameter Set given split for easy access */
  struct gl_list_t *il;   /* instance(s) required to digest a parameter */
  struct Instance *ipass; /* instance being passed into type */
  struct Instance *tmpinst; /* holding instance for derivation work. */
  struct StatementList *psl; /* list of parameters the type requires */
  struct StatementList *absorbed; /* list of absorbed isas and casgns */
  struct Statement *ps;      /* a statement from psl */
  struct Set *argset;		/* set element extracted from arglist */
  CONST struct VariableList *vl;
  struct for_table_t *SavedForTable;
  unsigned long slen,c,argn;
  int tverr;	/* error return from checking array elt type, or value */
  int suberr;	/* error return from other routine */
  int intset;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  unsigned int pc;	     /* number of parameters the type requires */

  if (StatWrong(statement)) {
    /* incorrect statements should be warned about when they are
     * marked wrong, so we just ignore them here.
     */
    return MPIOK;
  }
  d = FindType(GetStatType(statement));
  if (d==NULL) {
    /* lint should make this impossible */
    mpierror(NULL,0L,statement,MPIINPUT);
    return MPIINPUT;
  }
  if (keepargs == KEEPARGINST && arginstptr == NULL) {
    /* someone screwed up the call, but maybe they get it right later. */
    FPRINTF(ASCERR," *** MakeParameterInst miscalled *** \n");
    return MPIWAIT;
  }
  if (keepargs == KEEPARGINST) {
    /* init arginstptr */
    *arginstptr = NULL;
  }
  if ( GetBaseType(d)!=model_type) {
    return MPIOK;
  }
  pc = GetModelParameterCount(d);
  absorbed = GetModelAbsorbedParameters(d);
  if (pc==0 && StatementListLength(absorbed)==0L) {
    /* no parameters in this type or its ancestors */
    return MPIOK;
  }
  /* init tmpinst, which we must remember to punt before
   * error returns or nokeep returns.
   */
  /* may want an SCMUI here, not sure. */
  tmpinst = CreateModelInstance(d);
  if (tmpinst==NULL) {
    mpierror(NULL,0L,statement,MPIINPUT);
    return MPIINSMEM;
  }
  args = gl_create((unsigned long)pc);
  if (args == NULL) {
    mpierror(NULL,0L,statement,MPIINPUT);
    ClearMPImem(NULL,NULL,tmpinst,NULL,NULL);
    return MPIINSMEM;
  }
  SplitArgumentSet(GetStatTypeArgs(statement),args);
  /* due to typelint, the following assertion should pass. fix lint if not. */
  asc_assert(gl_length(args)==(unsigned long)pc);
  psl = GetModelParameterList(d);
  slen = StatementListLength(psl);
  argn = 1L;
  for (c = 1; c <= slen; c++) {
    ps = GetStatement(psl,c);
    vl = GetStatVarList(ps); /* move inside switch if allow FOR later */
    ptype = FindType(GetStatType(ps));
    stype = GetStatSetType(ps);
    intset = CalcSetType(stype,ps);
    if (intset <0 || intset >1) {
      /* shouldn't be possible -- typelint trapped */
      mpierror(NULL,0L,statement,MPIARGTYPE);
      ClearMPImem(args,NULL,tmpinst,NULL,NULL);
      return MPIARGTYPE;
    }
    switch (StatementType(ps)) {
    case WILLBE:
      while (vl != NULL) {
        argset = GETARG(args,argn);
        il = FindArgInsts(parent,argset,&err);
        if (il == NULL) {
          switch(rel_errorlist_get_find_error(&err)) {
          case unmade_instance:
          case undefined_instance: /* this case ought to be separable */
            MPIwum(argset,argn,statement,MPIUNMADE);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWAIT;
          case impossible_instance:
            mpierror(argset,argn,statement,MPIBADARG);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIBADARG;
          case correct_instance:
            mpierror(argset,argn,statement,MPIWEIRD);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWEIRD;
          }
        }
        if (gl_length(il)!=1L) {
          mpierror(argset,argn,statement,MPIMULTI);
          ClearMPImem(args,il,tmpinst,NULL,NULL);
          return MPIMULTI;
        }
        ipass = (struct Instance *)gl_fetch(il,1L);
        gl_destroy(il);
        il = NULL;
        if (SimpleNameIdPtr(NamePointer(vl))==NULL) {
          /* arg required is an array, check this.
           * check complete expansion of arg, constant type or not.
           * check compatible base type of all elements with spec-
           * note we haven't checked subscript ranges at this point.
           */
          if (IsArrayInstance(ipass)==0) {
            mpierror(argset,argn,statement,MPIARGTYPE);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIARGTYPE;
          }
          if (RectangleArrayExpanded(ipass)==0) {
            /* this works for sparse or dense because sparse won't
             * exist except in the fully expanded state due to
             * the construction all at once.
             */
            MPIwum(argset,argn,statement,MPIUNMADE);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWAIT;
          }
          if (NumberofDereferences(ipass) !=
              (unsigned long)(NameLength(NamePointer(vl)) - 1)) {
            /* I may need an offset other than -1 here */
            mpierror(argset,argn,statement,MPIARGSIZ);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIARGTYPE;
          }
          tverr = ArrayElementsTypeCompatible(ipass,ptype,stype);
          switch (tverr) {
          case 1:
           /* happy happy joy joy */
            break;
          case 0:
            MPIwum(argset,argn,statement,MPIWEAKTYPE);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWAIT;
          default:
            mpierror(argset,argn,statement,MPIARGTYPE);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIARGTYPE;
          }
          if (ArgValuesUnassigned(ipass)!=0) {
            MPIwum(argset,argn,statement,MPIUNASSD);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWAIT;
          }
        }else{
          /* arg must be scalar/set/MODEL */
          atype = InstanceTypeDesc(ipass);
          if (atype==ptype) {
            /* we're happy unless sets of mismatched base */
            if (stype!=NULL) {
              if ((IntegerSetInstance(ipass)!=0 && intset==0) ||
                  (IntegerSetInstance(ipass)==0 && intset==1)) {
                mpierror(argset,argn,statement,MPIARGTYPE);
                ClearMPImem(args,NULL,tmpinst,NULL,NULL);
                return MPIARGTYPE;
              }
            }
          }else{
            mrtype = MoreRefined(atype,ptype);
            if (mrtype==NULL) {
              mpierror(argset,argn,statement,MPIARGTYPE);
              ClearMPImem(args,NULL,tmpinst,NULL,NULL);
              return MPIARGTYPE;
            }
            if (mrtype==ptype) {
              /* arg is less refined than param spec. maybe better later */
              MPIwum(argset,argn,statement,MPIWEAKTYPE);
              ClearMPImem(args,NULL,tmpinst,NULL,NULL);
              return MPIWAIT;
            }
          }
          if (ArgValuesUnassigned(ipass)!=0) {
            MPIwum(argset,argn,statement,MPIUNASSD);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWAIT;
          }
          /* here we check against WITH_VALUE clause, if one in ps */
          suberr = ArgValueCorrect(ipass,tmpinst,ps);
          switch(suberr) {
          case MPIOK:
            break;
          case MPIWAIT:
            /* can only occur if other portions of tmpinst needed to compute
             * check value are not in place yet. no wum here because
             * Digest below will catch it if it's broken.
             */
            break;
          /* may need additional cases depending on argval implementation */
          default:
            mpierror(argset,argn,statement,MPIARGVAL);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          }
        }
        /* install ipass in tmpinst */
        if ( InsertParameterInst(tmpinst,ipass,NamePointer(vl),ps,IPICHECK)
            !=1) {
          /* ipicheck because we might be passed same instance in 2 slots */
          mpierror(argset,argn,statement,MPIMULTI);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIMULTI;
        }
        argn++;
        vl = NextVariableNode(vl);
      }
      break;
    case ISA:
      argset = GETARG(args,argn);
      if (SimpleNameIdPtr(NamePointer(vl))!=NULL) {
        /* scalar: evaluate and make it */
        suberr = MPIMakeSimple(parent,tmpinst,argset,argn,
                               NamePointer(vl),ptype,intset,ps,statement);
        if (suberr!=MPIOK) {
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return suberr;
        }
      }else{
        /* check completedness, assignedness, base type of array-by-value
         * and copy. Note that what we copy may prove to be incompatible
         * later when we check the names of subscripts.
         */
        il = FindArgInsts(parent,argset,&err);
        if (il == NULL) {
          switch(rel_errorlist_get_find_error(&err)) {
          case unmade_instance:
          case undefined_instance: /* this case ought to be separable */
            MPIwum(argset,argn,statement,MPIUNMADE);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWAIT;
          case impossible_instance:
            mpierror(argset,argn,statement,MPIBADARG);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIBADARG;
          case correct_instance:
            mpierror(argset,argn,statement,MPIWEIRD);
            ClearMPImem(args,NULL,tmpinst,NULL,NULL);
            return MPIWEIRD;
          }
        }
        if (gl_length(il)!=1L) {
          mpierror(argset,argn,statement,MPIMULTI);
          ClearMPImem(args,il,tmpinst,NULL,NULL);
          return MPIMULTI;
        }
        ipass = (struct Instance *)gl_fetch(il,1L);
        gl_destroy(il);
        il = NULL;
        /* arg required is an array, check this.
         * check complete expansion of arg, constant type or not.
         * check compatible base type of all elements with spec-
         * note we haven't checked subscript ranges at this point.
         */
        if (IsArrayInstance(ipass)==0) {
          mpierror(argset,argn,statement,MPIARGTYPE);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIARGTYPE;
        }
        if (RectangleArrayExpanded(ipass)==0) {
          /* this works for spare or dense because sparse won't
           * exist except in the fully expanded state due to
           * the construction all at once.
           */
          MPIwum(argset,argn,statement,MPIUNMADE);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIWAIT;
        }
        if (NumberofDereferences(ipass) !=
            (unsigned long)(NameLength(NamePointer(vl)) - 1)) {
          /* I may need an offset other than -1 here */
          mpierror(argset,argn,statement,MPIARGSIZ);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIARGTYPE;
        }
        tverr = ArrayElementsTypeCompatible(ipass,ptype,stype);
        switch (tverr) {
        case 1:
         /* happy happy joy joy */
          break;
        case 0:
          /* wum here */
          MPIwum(argset,argn,statement,MPIWEAKTYPE);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIWAIT;
        default:
          mpierror(argset,argn,statement,MPIARGTYPE);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIARGTYPE;
        }
        if (ArgValuesUnassigned(ipass)!=0) {
          MPIwum(argset,argn,statement,MPIUNASSD);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIWAIT;
        }
        /* this copy will mess up tmpnums in old ipass. */
        ipass = CopyInstance(ipass);
        /* note the copy has only been verified to work for completed
         * arrays of constants, not models.
         */
        /* we don't care about the old ipass any more. check new one. */
        if (ipass==NULL) {
          mpierror(argset,argn,statement,MPIINSMEM);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIMULTI;
        }
        /* install ipass in tmpinst */
        if ( InsertParameterInst(tmpinst,ipass,NamePointer(vl),ps,NOIPICHECK)
            !=1 /* arrays cannot be UNIVERSAL */ ) {
          mpierror(argset,argn,statement,MPIMULTI);
          ClearMPImem(args,NULL,tmpinst,NULL,NULL);
          return MPIMULTI;
        }
        /* we still need to check the subscripts for compatibility with
         * arg description. can't do yet.
         */
      }
      argn++;
      break;
    default:
      ASC_PANIC("how the hell did typelint let that through?");
      /* how the hell did typelint let that through? */
      break;
    }
  }
  /* ok, so now we have everything passed (which might be nothing)
   * in place. We need to check WITH_VALUE's, subscript ranges,
   * and insist all scalars end up assigned while processing
   * the absorbed statements. Possibly may still find undefined
   * values in rhs of assignments or in subscript ranges, drat.
   * May take several passes.
   */

  suberr = DigestArguments(tmpinst,args,psl,absorbed,statement); /*1*/
  switch(suberr) {
  case MPIOK:
    break;
  case MPIWAIT:
    ClearMPImem(args,NULL,tmpinst,NULL,NULL);
    return MPIWAIT;
  default:
    /* anything else is an error. mpierror will have been called. */
    ClearMPImem(args,NULL,tmpinst,NULL,NULL);
    return MPIINPUT;
  }

  /* ok, now we need to check where statement list. */
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  suberr = CheckWhereStatements(tmpinst,GetModelParameterWheres(d));
  DestroyForTable(GetEvaluationForTable());
  SetEvaluationForTable(SavedForTable);
  switch(suberr) {
  case MPIOK:
    break;
  case MPIWAIT:
    ClearMPImem(args,NULL,tmpinst,NULL,NULL);
    return MPIWAIT;
  default:
    /* anything else is an error */
    ClearMPImem(args,NULL,tmpinst,NULL,NULL);
    mpierror(NULL,0,statement,suberr);
    return suberr;
  }

  ClearMPImem(args,NULL,NULL,NULL,NULL);
  if (keepargs == KEEPARGINST) {
    *arginstptr = tmpinst;
  }else{
    DestroyParameterInst(tmpinst);
  }
  return MPIOK;
}

static
int BuildInstanceFromAbsorbedParameters(struct TypeDescription *d,
                                        struct Instance **arginstptr)
{
  struct Instance *tmpinst;
  struct StatementList *absorbed;
  struct gl_list_t *args;
  struct for_table_t *SavedForTable;
  struct Statement *trigger;
  int suberr;

  asc_assert(d != NULL);
  asc_assert(arginstptr != NULL);
  *arginstptr = NULL;

  absorbed = GetModelAbsorbedParameters(d);
  if (StatementListLength(absorbed) == 0L) {
    return MPIOK;
  }

  tmpinst = CreateModelInstance(d);
  if (tmpinst == NULL) {
    return MPIINSMEM;
  }
  args = gl_create(0L);
  if (args == NULL) {
    DestroyParameterInst(tmpinst);
    return MPIINSMEM;
  }

  trigger = GetStatement(absorbed,1);
  suberr = DigestArguments(
    tmpinst,
    args,
    GetModelParameterList(d),
    absorbed,
    trigger
  );
  switch (suberr) {
  case MPIOK:
    break;
  default:
    ClearMPImem(args,NULL,tmpinst,NULL,NULL);
    return suberr;
  }

  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  suberr = CheckWhereStatements(tmpinst,GetModelParameterWheres(d));
  DestroyForTable(GetEvaluationForTable());
  SetEvaluationForTable(SavedForTable);
  switch (suberr) {
  case MPIOK:
    break;
  default:
    ClearMPImem(args,NULL,tmpinst,NULL,NULL);
    return suberr;
  }

  ClearMPImem(args,NULL,NULL,NULL,NULL);
  *arginstptr = tmpinst;
  return MPIOK;
}

static int MPICheckWBTS(struct Instance *tmpinst, struct Statement *statement){
  struct gl_list_t *instances;
  unsigned long c,len;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *head = NULL;

  instances = FindInsts(tmpinst,GetStatVarList(statement),&err);
  if (instances==NULL) {
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance:
      MissingInsts(tmpinst,GetStatVarList(statement),1);
      STATEMENT_ERROR(statement,
        "WILL_BE_THE_SAME statement contains an impossible instance name");
      return MPIBADWBTS;
    default:
      MissingInsts(tmpinst,GetStatVarList(statement),0);
      WriteUnexecutedMessage(ASCERR,statement,
        "Incomplete instances in WILL_BE_THE_SAME");
      return MPIWAIT; /* statement is not ready to be executed */
    }
  }
  len = gl_length(instances);
  if (len >0 ) {
    head = gl_fetch(instances,1);
  }
  for (c=2; c<=len; c++) {
    if (((struct Instance *)gl_fetch(instances,c)) != head) {
      if (IsArrayInstance(head)==0 &&
          MoreRefined(InstanceTypeDesc(gl_fetch(instances,c)),
                      InstanceTypeDesc(head))==NULL) {
        /* can't be merged later */
        STATEMENT_ERROR(statement,
          "WILL_BE_THE_SAME statement contains incompatible instances");
        gl_destroy(instances);
        return MPIBADWBTS;
      }else{
        /* maybe merge later */
        WriteUnexecutedMessage(ASCERR,statement,
          "Unmerged instances in WILL_BE_THE_SAME");
        gl_destroy(instances);
        return MPIWAIT;
      }
    }
  }
  gl_destroy(instances);
  return MPIOK;
}

#define MPICheckWB(a,b) MPIWEIRD
/* WILL_BE not yet legal in where section. implement later if req'd */

/**
	verifies that all the instances found, if any, are different.
	uses an nlogn (n = # of instance) algorithm, which
	could be made order n using the interface pointer protocol,
	but the additional overhead makes the multiplier for
	o(n) probably not worth the trouble.
*/
static
int MPICheckWNBTS(struct Instance *tmpinst, struct Statement *statement)
{
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  instances = FindInsts(tmpinst,GetStatVarList(statement),&err);
  if (instances==NULL) {
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance:
      MissingInsts(tmpinst,GetStatVarList(statement),1);
      STATEMENT_ERROR(statement,
        "WILL_NOT_BE_THE_SAME statement contains an impossible instance name");
      return MPIBADWNBTS;
    default:
      MissingInsts(tmpinst,GetStatVarList(statement),0);
      WriteUnexecutedMessage(ASCERR,statement,
        "Incomplete instances in WILL_NOT_BE_THE_SAME");
      return MPIWAIT; /* statement is not ready to be executed */
    }
  }
  if (gl_unique_list(instances)==0) {
    STATEMENT_ERROR(statement,
          "WILL_NOT_BE_THE_SAME statement contains"
         " identical/merged instances");
    gl_destroy(instances);
    return MPIBADMERGE;
  }
  gl_destroy(instances);
  return MPIOK;
}

/**
	Checks the for statements, along with all the horrid machinery needed
	to make a for loop go.
*/
static
int CheckWhereFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  int code=MPIOK;

  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return MPIFOR;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "FOR has undefined values");
      return MPIFOR; /* this maybe should be mpiwait? */
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      return MPIFOR;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    instantiation_error(ASC_USER_ERROR,statement
  		,"FOR expression returns the wrong type."
  	);
    DestroyValue(&value);
    return MPIFOR;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        code = CheckWhereStatements(inst,sl);
        if (code != MPIOK) {
          break;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        code = CheckWhereStatements(inst,sl);
        if (code != MPIOK) {
          break;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
  return code;
}

/**
	checks that all conditions are satisfied, else returns a whine.
	does not call mpierror, so caller ought to if needed.
	returns one of the defined MPI codes.
*/
static
int CheckWhereStatements(struct Instance *tmpinst, struct StatementList *sl)
{
  unsigned long c,len;
  struct Statement *s;
  int code=MPIOK;

  if (tmpinst ==NULL) {
    return MPIWEIRD;
  }
  len = StatementListLength(sl);
  for (c=1;c <= len && code == MPIOK; c++) {
    s = GetStatement(sl,c);
    switch (StatementType(s)) {
    case WBTS:
      code = MPICheckWBTS(tmpinst,s);
      break;
    case WNBTS:
      code = MPICheckWNBTS(tmpinst,s);
      break;
    case WILLBE:
      code = MPICheckWB(tmpinst,s);
      break;
    case LOGREL:
    case REL: /* note EXT not allowed in constraint list */
        /* baa. fix me. bug. need to evaluate rules in a way which is
         * exception-safe. EvaluateExpr currently isn't
         */
      code = MPICheckConstraint(tmpinst,s);
      break;
    case FOR:
      code = CheckWhereFOR(tmpinst,s);
      break;
    default:
      code = MPIWEIRD;
      break;
    }
  }
  return code;
}

#if 0 /* migrating, or migraining, depending on your viewpoint, to parpend.h */
enum ppstatus {
  pp_ERR =0,
  pp_ISA,	/* IS_A of simple to be done, from absorbed. */
  pp_ISAARR,	/* IS_A of array to do, from absorbed and
                 * gets converted to asar during processing.
                 */
  pp_ARR,	/* array that's constructed but requires range checking */
  pp_ASGN,	/* assignment to do in absorbed objects */
  pp_ASSC,	/* scalar assignment to check in absorbed objects */
  pp_ASAR,	/* Array to be checked for being completely assigned,
                 * but its subscript range is presumed right.
                 */
  pp_WV,	/* WITH_VALUE to be checked */
  pp_DONE	/* finished statement */
};

struct parpendingentry {
  struct Set *arg;	/* parameter given in user's IS_A statement */
  struct Statement *s;
  struct Instance *inst;
  struct parpendingentry *next;
  enum ppstatus status;
  int argn; /* the psl position if >0,  or -(the absorbed position) if <0 */
  /* argn==0 is an error */
};

#endif /* 0 migraining */

/**
	returns a single instance, if such can be properly derived
	from the name given.

	Returns NULL if too many or no instances are found.
	Probably ought to have a return code, but doesn't.
*/
static
struct Instance *GetNamedInstance(CONST struct Name *nptr,
                                  CONST struct Instance *tmpinst)
{
  struct Instance *i=NULL;
  struct gl_list_t *insts;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  asc_assert(nptr!=NULL);
  asc_assert(tmpinst!=NULL);
  insts = FindInstances(tmpinst,nptr,&err);
  if (insts==NULL) {
    return NULL;
  }
  if (gl_length(insts) == 1L) {
    i = (struct Instance *)gl_fetch(insts,1);
  }
  gl_destroy(insts);
  return i;
}

/*
	put the parameters open (if any) and absorbed statements into the
	pending list we're creating.
*/
static
struct parpendingentry *
CreateParameterPendings(struct Instance *tmpinst,
                        struct gl_list_t *args,
                        struct StatementList *psl,
                        struct StatementList *absorbed)
{
  unsigned long c,len;
  struct parpendingentry *new, *list=NULL;
  CONST struct Expr *ex;
  struct gl_list_t *nlist=NULL;

  asc_assert(args!=NULL);

  len = gl_length(args);
  for (c=len; c >= 1; c--) {
    new = CreatePPE();
    /* Create must not return NULL */
    new->arg = gl_fetch(args,c);
    new->s = GetStatement(psl,c);
    new->inst = NULL;
    new->argn = c;
    switch (StatementType(new->s)) {
    case WILLBE:
      /* assumes lint did it's job */
      if (NameLength(NamePointer(GetStatVarList(new->s))) > 1) {
        /* arrays were connected already, but no subscript check */
        new->inst = GetArrayHead(tmpinst,NamePointer(GetStatVarList(new->s)));
        new->status = pp_ARR;
      }else{
        /* scalar */
        ex = GetStatCheckValue(new->s);
        if (ex != NULL) {
          nlist = EvaluateNamesNeededShallow(ex,NULL,NULL);
          asc_assert(nlist!=NULL);
          if (gl_length(nlist) != 0L) {
            new->status = pp_WV;
            new->inst =
              GetNamedInstance(NamePointer(GetStatVarList(new->s)),tmpinst);
          }else{
            /* nothing further to check. done already */
            DestroyPPE(new);
            new = NULL;
          }
          gl_destroy(nlist);
        }else{
          DestroyPPE(new);
          new = NULL;
        }
      }
      break;
    case ISA:
      if (NameLength(NamePointer(GetStatVarList(new->s))) > 1) {
        /* subscript check */
        new->inst = GetArrayHead(tmpinst,NamePointer(GetStatVarList(new->s)));
        new->status = pp_ARR;
      }else{
        /* nothing further to check. assumed done already */
        DestroyPPE(new);
        new = NULL;
      }
      break;
    default:
      Asc_Panic(2, "CreateParameterPendings",
                "Unknown statement type in CreateParameterPendings!\n");
      break;
    }
    if (new != NULL) {
      /* insert at head, but completed statements don't get added */
      new->next = list;
      list = new;
    }
  }
  len = StatementListLength(absorbed);
  for (c=len; c >= 1; c--) {
    new = CreatePPE();
    /* Create must not return NULL */
    new->arg = NULL;
    new->s = GetStatement(absorbed,c);
    new->inst = NULL;
    new->argn =0; new->argn -= c;
    switch (StatementType(new->s)) {
    case ISA:
      if (NameLength(NamePointer(GetStatVarList(new->s))) > 1) {
        /* array needed and subscript check */
        new->status = pp_ISAARR;
        /* after construction, no check until fully assigned at end */
      }else{
        /* simplename */
        new->status = pp_ISA;
      }
      break;
    case CASGN:
      new->status = pp_ASGN;
      break;
    default:
      Asc_Panic(2, "CreateParameterPendings",
                "Unknown statement type in CreateParameterPendings!\n");
      break;
    }
    new->next = list;
    list = new;
  }
  return list;
}
/* destroy a list of pending parameter items.
 */
static
void DestroyParameterPendings( struct parpendingentry *pp)
{
  struct parpendingentry *old;
  while (pp!=NULL) {
    old = pp;
    pp = pp->next;
    DestroyPPE(old);
  }
}

/*
	this function should not be entered until all WB arguments have
	been installed in tmpinst.
*/
static
int DigestArguments(
                    struct Instance *tmpinst,
                    struct gl_list_t *args,
                    struct StatementList *psl,
                    struct StatementList *absorbed,
                    struct Statement *statement)
{
  struct parpendingentry *pp,	/* current work */
                         *pphead, /* first in work list */
                         *pplast; /* just prior work, so can delete current */
  int change = 1;
  int suberr = MPIOK; /* maybe mpi enum */

  pphead = pp = CreateParameterPendings(tmpinst,args,psl,absorbed);
  while (change && pphead!=NULL && suberr ==MPIOK) {
    pplast = NULL;
    pp = pphead;
    change = 0;
    while (pp != NULL && suberr ==MPIOK) {
      switch (pp->status) {
      case pp_ISA:
        /* building a scalar! OTHERWISE recursion could bite us.
         * We don't use mpimakesimpleinstance because no argval.
         */
        suberr = ExecuteISA(tmpinst,pp->s);
        if (suberr!=1) {
          suberr = MPIWEIRD;
          pp->status = pp_ERR;
          FPRINTF(ASCERR,"While executing (1) absorbed statement in %s:\n",
            SCP(GetName(InstanceTypeDesc(tmpinst))));
          WriteStatement(ASCERR,pp->s,2);
          mpierror(NULL,0,statement,suberr);
        }else{
          pp->inst =
            GetNamedInstance(NamePointer(GetStatVarList(pp->s)),tmpinst);
          if (pp->inst != NULL) {
            suberr = MPIOK;
            pp->status = pp_ASSC;
          }else{
            suberr = MPIWEIRD;
            pp->status = pp_ERR;
            FPRINTF(ASCERR,"While executing (2) absorbed statement in %s:\n",
              SCP(GetName(InstanceTypeDesc(tmpinst))));
            WriteStatement(ASCERR,pp->s,2);
            mpierror(NULL,0,statement,suberr);
          }
        }
        change++;
        break;
        /* done case */
      case pp_ISAARR:
        /* IS_A of array that needs doing, range, args assignment */
        if (CheckISA(tmpinst,pp->s) == 1) {
          /* Must have subscripts defined first, because we do not
           * want the array to be put on the global pending list as
           * that would be algorithmic suicide. The whole point of
           * parameters is reducing a set of operations to a point
           * in the ProcessPending execution cycle.
           */
          suberr = ExecuteISA(tmpinst,pp->s);
          /* so the array should be completely expanded now. */
          /* we won't check unless problems start to show up,
           * since we believe the array code to be correct.
           */
          if (suberr!=1) {
            suberr = MPIWEIRD;
            pp->status = pp_ERR;
            FPRINTF(ASCERR,"While executing (3) absorbed statement in %s:\n",
              SCP(GetName(InstanceTypeDesc(tmpinst))));
            WriteStatement(ASCERR,pp->s,2);
            mpierror(NULL,0,statement,suberr);
          }else{
            pp->inst =GetArrayHead(tmpinst,NamePointer(GetStatVarList(pp->s)));
            if (pp->inst == NULL) {
              suberr = MPIWEIRD;
              pp->status = pp_ERR;
              FPRINTF(ASCERR,"While executing (4) absorbed statement in %s:\n",
                SCP(GetName(InstanceTypeDesc(tmpinst))));
              WriteStatement(ASCERR,pp->s,2);
              mpierror(NULL,0,statement,suberr);
            }else{
              suberr = MPIOK;
              pp->status = pp_ASAR; /* needs assigning */
            }
          }
          change++;
        }
        /* done case */
        break;
      case pp_ARR:
        /* someone will have init'd pp->inst */
        /* checking whether sets in pp->s expand to match sets
         * in pp->inst, the array head and child of tmpinst.
         * Must accomodate FOR loops in future.
         */
        suberr = MPICheckSubscripts(tmpinst,pp->inst,pp->s);
        switch(suberr) {
        case MPIOK:
          pp->status = pp_DONE;
          change++;
          break;
        case MPIWAIT:
          suberr = MPIOK;
          break;
        default:
          pp->status = pp_ERR;
          WriteInstance(ASCERR,tmpinst);
          WriteInstance(ASCERR,pp->inst);
          mpierror(pp->arg,pp->argn,statement,suberr);
          change++;
          break;
        }
        break;
        /* done case */
      case pp_ASGN:
        if (ExecuteCASGN(tmpinst,pp->s) == 1) {
          pp->status = pp_DONE;
          change++;
        }
        /* done case */
        break;
      case pp_WV:	/* WITH_VALUE that needs checking */
        if (ArgValueCorrect(pp->inst,tmpinst,pp->s)==MPIOK) {
          pp->status = pp_DONE;
          change++;
        }
        /* done case */
        break;
      case pp_ASAR:
      case pp_ASSC:
        if (ArgValuesUnassigned(pp->inst)==0) {
          pp->status = pp_DONE;
          change++;
        }
        /* done case */
        break;
      case pp_DONE:
        FPRINTF(ASCERR,"Unexpected pp_DONE in DigestParameters!\n");
        break;
        /* say what? should have been deleted already. */
        /* done case */
      case pp_ERR:
        /* shouldn't have gone through the loop to reach an err marked pp */
      default:
        ASC_PANIC("Unexpected status in DigestParameters!\n");
        break;
      }
      /* delete if we finished it, then advance counter. */
      if (pp->status == pp_DONE) {
        /* delete pp, but pplast cannot change */
        if (pplast != NULL) { /* we're somewhere in the middle */
          pplast->next = pp->next;
          DestroyPPE(pp);
          pp = pplast->next; /* could be null */
        }else{
          /* we're at the top */
          pphead = pp->next;
          DestroyPPE(pp);
          pp = pphead; /* could be null */
        }
      }else{
        /* just advance the list, even if pperr. */
        pplast = pp;
        pp = pplast->next;
        /* if pp --> NULL, inner while will fail, outer may */
      }
    }
  }
  /* either fell out on error, in which case it is in pplast and the
   * error whine already was done,
   * or pphead !=NULL, but changed didn't move, in which case we
   * need to look for unexecuted assignments, unchecked WITH_VALUE's,
   * and unverified array subscripts and wum about them,
   * or pphead == NULL and we're done and can get out.
   */
  if (suberr!= MPIOK) {
    DestroyParameterPendings(pphead);
    return suberr;
  }
  if (pphead == NULL) {
    return suberr; /* the normal exit */
  }
  pp = pphead;
  while (pp!=NULL) {
    char *msg;
    //CONST struct Statement *stat;
    switch (pp->status) {
    case pp_ISA:
      msg = "Oddly unable to construct parameter scalar";
      //stat = pp->s;
      break;
    case pp_ISAARR:
      msg = "Unable to construct array parameter. Probably missing subscripts";
      //stat = pp->s;
      break;
    case pp_ARR:
      msg = "Unable to check parameter array subscripts.";
      //stat = pp->s;
      break;
    case pp_ASGN:
      msg = "Unable to execute assigment: LHS unmade or RHS not evaluatable";
      //stat = pp->s;
      break;
    case pp_ASSC:
      msg ="Unable to set scalar param: RHS not evaluatable or incorrect type";
      //stat = pp->s;
      break;
    case pp_ASAR:
      msg = "Parameters: Not all array elements assigned during refinement";
      //stat = pp->s;
      break;
    case pp_WV:
      msg = "Unable to verify parameter value: probably bad WITH_VALUE RHS";
      //stat = pp->s;
      break;
    case pp_ERR:
      //stat = statement;
      msg = "Unexpected pp_ERR pending in parameters";
      break;
    case pp_DONE:
      msg = NULL;
      break;
    default:
	  msg = NULL;
    }
    if (msg != NULL) {
      WriteUnexecutedMessage(ASCERR,statement,msg);
    }
    pp = pp->next;
  }
  DestroyParameterPendings(pphead);
  return MPIWAIT;
}

static
void ConfigureCopy(struct Instance *inst,
                   CONST struct Instance *arginst,
                   unsigned long cnum)
{
  struct Instance *src,*copy;

  src = InstanceChild(arginst,cnum);
  asc_assert(src!=NULL);
  copy = CopyInstance(src);
  asc_assert(copy!=NULL);
  StoreChildPtr(inst,cnum,copy);
  /* hunting out UNIVERSAL/arrays we could make this check much
   * less needed.
   */
  if (SearchForParent(copy,inst)==0) {
    AddParent(copy,inst);
  }
}

/* assumes inst, arginst of same type. copies reference
 * children of arginst to same slots in inst.
 */
static
void ConfigureReference(struct Instance *inst,
                        CONST struct Instance *arginst,
                        unsigned long cnum)
{
  struct Instance *src;

  src = InstanceChild(arginst,cnum);
  asc_assert(src!=NULL);
  StoreChildPtr(inst,cnum,src);
  /* hunting out UNIVERSAL/arrays we could make this check much
   * less needed.
   */
  if (SearchForParent(src,inst)==0) {
    AddParent(src,inst);
  }
}

/**
	Connect WILL_BE'd children from arginst to inst.
	Copy IS_A'd children from arginst to inst.
	At this point there can be no alias children -- all
	are either WILL_BE or IS_A of constants/arrays.
	This must only be called with models when arginst !=NULL.
	arginst == NULL --> immediate, no action return.
	inst and arginst are assumed to be the same type.
*/
void ConfigureInstFromArgs(struct Instance *inst,
                           CONST struct Instance *arginst)
{
  ChildListPtr clist;
  unsigned long c,len;

  if (arginst == NULL) {
    return;
  }
  asc_assert(InstanceKind(inst)==MODEL_INST);
  asc_assert(InstanceTypeDesc(inst)==InstanceTypeDesc(arginst));
  clist = GetChildList(InstanceTypeDesc(inst));
  len = ChildListLen(clist);
  for (c=1; c <= len; c++) {
    switch(ChildOrigin(clist,c)) {
    case origin_ALI:
    case origin_ARR:
    case origin_ISA:
    case origin_WB:
    case origin_PALI:
    case origin_PARR:
      if (InstanceChild(arginst,c)!=NULL) {
        ASC_PANIC("arginst caught with illegitimate child. Bye!");
      }
      break;
    case origin_PISA:
      ConfigureCopy(inst,arginst,c);
      break;
    case origin_PWB:
      ConfigureReference(inst,arginst,c);
      break;
    case origin_ERR:
    default:
      ASC_PANIC("arginst caught with alien child. Bye!");
    }
  }
}

/**
	For Those children not already present in inst,
	which must be of the same type as arginst.
	Connect WILL_BE'd children from arginst to inst.
	Copy IS_A'd children from arginst to inst.
	At this point there can be no alias children -- all
	are either WILL_BE or IS_A of constants/arrays, so far as
	arginst is concerned.

	This must only be called with models when arginst !=NULL.
	arginst == NULL --> immediate, no action return.
	inst is expected to be of same type as arginst.
*/
void ReConfigureInstFromArgs(struct Instance *inst,
                             CONST struct Instance *arginst)
{
  ChildListPtr clist;
  unsigned long c,len;

  if (arginst == NULL) {
    return;
  }
  asc_assert(InstanceKind(inst)==MODEL_INST);
  asc_assert(InstanceTypeDesc(inst)==InstanceTypeDesc(arginst));
  clist = GetChildList(InstanceTypeDesc(arginst));
  len = ChildListLen(clist);
  for (c=1; c <= len; c++) {
    switch(ChildOrigin(clist,c)) {
    case origin_ALI:
    case origin_ARR:
    case origin_ISA:
    case origin_WB:
    case origin_PALI:
    case origin_PARR:
      if (InstanceChild(arginst,c)!=NULL) {
        ASC_PANIC("arginst caught with illegitimate child. Bye!");
      }
      break;
    case origin_PISA:
      if (InstanceChild(inst,c)==NULL) {
        /* child that didn't exist in the less refined type. */
        ConfigureCopy(inst,arginst,c);
      }
      break;
    case origin_PWB:
      if (InstanceChild(inst,c)==NULL) {
        /* child that didn't exist in the less refined type. */
        ConfigureReference(inst,arginst,c);
      }
      break;
    case origin_ERR:
    default:
      ASC_PANIC("arginst caught with alien child. Bye!");
    }
  }
}

static
int EqualChildInsts(struct Instance *i1, struct Instance *i2,
                    unsigned long c1, unsigned long c2)
{
  if (c1==0 || c2==0 || i1 == NULL || i2 == NULL ||
      InstanceChild(i1,c1) != InstanceChild(i2,c2)) {
    return 1;
  }
  return 0;
}

/**
	On proper types returns 0 if the inst values are ==
	for the c1th child of i1 and c2th child of i2. OTHERWISE nonzero.

	@BUG
	do not call this with instances other than variables/constants
	or arrays of same. relations, models, etc make it barf or lie.
*/
static
int CompareChildInsts(struct Instance *i1, struct Instance *i2,
                      unsigned long c1, unsigned long c2)
{
  struct Instance *ch1,* ch2;
  asc_assert(i1!=NULL);
  asc_assert(i2!=NULL);
  ch1 = InstanceChild(i1,c1);
  ch2 = InstanceChild(i2,c2);
  asc_assert(ch1!=NULL);
  asc_assert(ch2!=NULL);
  if (InstanceKind(ch1) != InstanceKind(ch2)) {
    return 1;
  }
  if (IsArrayInstance(ch1)) {
    return CmpArrayInsts(ch1,ch2);
  }else{
    return CmpAtomValues(ch1,ch2);
  }
}

/**
	Needs to see that all nonnull children in inst are compatible
	with corresponding children in mpi if such exist.
	arginst must be as or morerefined than inst.

	In particular, needs to be damned picky about where's being met
	and types matching exactly because we won't refine up stuff
	by passing it through a parameter list.
	WILL_BE child pointers of the arginst must = those in inst
	when the inst has a child of that name.
	IS_A child pointers of the arginst must have same value as
	those in inst when the inst has a child of that name.
	When inst has no child of that name, must eventually copy it
	to the expanded instance.

	This has to check that absolutely everything is correct
	because RefineClique/RefineInstance asks no questions.
	This itself assume arginst has been correctly constructed.
*/
static
int CheckParamRefinement(struct Instance *parent,
                         struct Instance *inst,
                         struct Instance *arginst,
                         struct Statement *statement)
{
  ChildListPtr icl, aicl;
  unsigned long oldlen, newlen, c,pos;
  symchar *childname;

  asc_assert(MoreRefined(InstanceTypeDesc(inst),InstanceTypeDesc(arginst))==
         InstanceTypeDesc(arginst));
  icl = GetChildList(InstanceTypeDesc(inst));
  aicl = GetChildList(InstanceTypeDesc(arginst));
  oldlen = ChildListLen(icl);
  newlen = ChildListLen(aicl);
  if (newlen == oldlen) {
    /* very common case, just upgrading types by assigning constants
     * in REFINES clause, though things may have been constructed
     * with those constants earlier.
     */
    for (c=1; c <= newlen; c++) {
      switch(ChildOrigin(aicl,c)) {
      case origin_ALI:
      case origin_ARR:
      case origin_ISA:
      case origin_WB:
      case origin_PALI:
      case origin_PARR:
        if (InstanceChild(arginst,c)!=NULL) {
          ASC_PANIC("arginst caught with illegitimate child. Bye!");
        }
        break;
      case origin_PISA:
        /* both must be assigned, and to the same values */
        if (CompareChildInsts(inst,arginst,c,c)!=0) {
          FPRINTF(ASCERR,"Incompatible constants: ");
          WriteInstanceName(ASCERR,InstanceChild(inst,c),parent);
          mpierror(NULL,0,statement,MPIREASGN);
          return MPIREASGN;
        }
        break;
      case origin_PWB:
        if (EqualChildInsts(inst,arginst,c,c)!=0) {
          FPRINTF(ASCERR,"Different object passed for: ");
          WriteInstanceName(ASCERR,InstanceChild(inst,c),parent);
          mpierror(NULL,0,statement,MPIREDEF);
          return MPIREDEF;
        }
        break;
      case origin_ERR:
      default:
        ASC_PANIC("arginst caught with alien child. Bye!");
      }
    }
  }else{
    /* increased child list */
    for (c=1; c <= newlen; c++) {
      switch(ChildOrigin(aicl,c)) {
      case origin_ALI:
      case origin_ARR:
      case origin_ISA:
      case origin_WB:
      case origin_PALI:
      case origin_PARR:
        if (InstanceChild(arginst,c)!=NULL) {
          ASC_PANIC("arginst caught with illegitimate child. Bye!");
        }
        break;
      case origin_PISA:
        /* both must be assigned, and to the same values, if inst has it */
        childname = ChildStrPtr(aicl,c);
        pos = ChildPos(icl,childname);
        if (pos > 0 && CompareChildInsts(inst,arginst,pos,c)!=0) {
          FPRINTF(ASCERR,"Incompatible constants: ");
          WriteInstanceName(ASCERR,InstanceChild(inst,pos),parent);
          mpierror(NULL,0,statement,MPIREASGN);
          return MPIREASGN;
        }
        break;
      case origin_PWB:
        childname = ChildStrPtr(aicl,c);
        pos = ChildPos(icl,childname);
        if (pos > 0 && EqualChildInsts(inst,arginst,pos,c)!=0) {
          FPRINTF(ASCERR,"Different object passed for: ");
          WriteInstanceName(ASCERR,InstanceChild(inst,pos),parent);
          mpierror(NULL,0,statement,MPIREDEF);
          return MPIREDEF;
        }
        break;
      case origin_ERR:
      default:
        ASC_PANIC("arginst caught with alien child. Bye!");
      }
    }
  }
  return MPIOK;
}

/*------------------------------------------------------------------------------
	...
*/

/**
	handles construction of IS_A statements.
	MakeInstance and its subsidiaries must not cannibalize
	parts from arginst, because it may be used again on
	subsequent calls when the IS_A has several lhs.
*/
static
void MakeInstance(CONST struct Name *name,
                  struct TypeDescription *def,
                  int intset,
                  struct Instance *parent,
                  struct Statement *statement,
                  struct Instance *arginst)
{
  symchar *childname;
  int changed;
  unsigned long pos;
  struct Instance *inst;
  struct InstanceName rec;
  struct TypeDescription *arydef;
  struct gl_list_t *indices;
  int tce;
  /*char *nstr;
  nstr = WriteNameString(name);
  CONSOLE_DEBUG(nstr);
  ascfree(nstr); */
  if ((childname = SimpleNameIdPtr(name))!=NULL){ /* simple 1 element name */
    if (StatInFOR(statement) && StatWrong(statement)==0) {
      MarkStatContext(statement,context_WRONG);
      STATEMENT_ERROR(statement,"Unindexed statement in FOR loop not allowed.");
      WSS(ASCERR,statement);
      return;
    }
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,childname);
    pos = ChildSearch(parent,&rec);
    if (pos>0) {
      if (InstanceChild(parent,pos)==NULL){
        inst = MakeSimpleInstance(def,intset,statement,arginst);
        LinkToParentByPos(parent,inst,pos);
      }else{			/* redefining instance */
        char *msg = ASC_NEW_ARRAY(char,SCLEN(childname)+strlen(REDEFINE_CHILD_MESG)+1);
        strcpy(msg,REDEFINE_CHILD_MESG);
        strcat(msg,SCP(childname));
        STATEMENT_ERROR(statement,msg);
        ascfree(msg);
      }
    }else{			/* unknown child name */
      STATEMENT_ERROR(statement, "Unknown child name.  Never should happen");
      ASC_PANIC("Unknown child name.  Never should happen");
    }
  }else{
    /* if reach the else, means compound identifier or garbage */
    indices = ArrayIndices(name,parent);
    if (indices!=NULL){ /* array of some sort */
      childname = NameIdPtr(name);
      SetInstanceNameType(rec,StrName);
      SetInstanceNameStrPtr(rec,childname);
      pos = ChildSearch(parent,&rec);
      if (!StatInFOR(statement)) { /* rectangle arrays */
        arydef = CreateArrayTypeDesc(StatementModule(statement),
                                     def,intset,0,0,0,indices);
        if (pos>0) {
          inst = CreateArrayInstance(arydef,1);
          if (inst!=NULL){
            changed = 0;
            tce = TryChildExpansion(inst,parent,&changed,NULL,arginst,NULL);
            /* we're not in a for loop, so can't fail unless user is idiot. */
            LinkToParentByPos(parent,inst,pos);
            /* if user is idiot, whine. */
            if (tce != 0) {
              SignalChildExpansionFailure(parent,pos);
            }
          }else{
            STATEMENT_ERROR(statement, "Unable to create array instance");
            ASC_PANIC("Unable to create array instance");
          }
        }else{
          DeleteTypeDesc(arydef);
          STATEMENT_ERROR(statement,
               "Unknown array child name. Never should happen");
          ASC_PANIC("Unknown array child name. Never should happen");
        }
      }else{
        DestroyIndexList(indices);
        if (pos>0) {
          if (InstanceChild(parent,pos)==NULL) {
            /* must make IS_A array */
            (void) /* should check for NULL return here */
            MakeSparseArray(parent,name,statement,
                            def,intset,NULL,arginst,NULL);
          }else{
            /* must add array element */ /* should check for NULL return here */
            (void)AddArrayChild(parent,name,statement,NULL,arginst,NULL);
          }
        }else{
          STATEMENT_ERROR(statement,
            "Unknown array child name. Never should happen");
          ASC_PANIC("Unknown array child name. Never should happen");
        }
      }
    }else{
      /* bad child name. cannot create parts of parts.  should never
       * happen, being trapped out in typelint.
       */
      STATEMENT_ERROR(statement,"Bad IS_A child name.");
    }
  }
}

static
int ExecuteISA(struct Instance *inst, struct Statement *statement)
{
  struct TypeDescription *def;
  CONST struct VariableList *vlist;
  struct Instance *arginst = NULL;
  int mpi;
  int intset;
  int selector_type;

  asc_assert(StatementType(statement)==ISA);
  if (StatWrong(statement)) {
    /* incorrect statements should be warned about when they were
     * marked wrong, so we just ignore them here.
     */
    return 1;
  }
  if ((def = FindType(GetStatType(statement)))!=NULL){
    selector_type = IsSelectorTypeDesc(def);
    if (GetStatCheckKind(statement) == ISCV_DEFAULT
        && (StatementType(statement) != ISA || !selector_type)) {
      STATEMENT_ERROR(statement,
        "Declaration DEFAULT is only supported for selector IS_A declarations");
      return 1;
    }
    if ((GetStatSetType(statement)!=NULL)
        && (GetBaseType(def)!=set_type)
        && !selector_type) {
      WriteSetError(statement,def);
      return 1;
    }
    if ((GetStatSetType(statement)==NULL) && (GetBaseType(def)==set_type)){
      WriteSetError(statement,def);
      return 1;
    }
    if (!CheckISA(inst,statement)) {
      /* last pass whine */
      WriteUnexecutedMessage(ASCERR,statement,
        "Possibly undefined indices in IS_A statement.");
      return 0;
    }
    mpi = MakeParameterInst(inst,statement,&arginst,KEEPARGINST);/*3*/
    if (mpi != MPIOK) {
      if (mpi == MPIWAIT) {
        WriteUnexecutedMessage(ASCERR,statement,
          "Possibly undefined arguments in IS_A statement.");
        return 0;
      }else{
        /* bogus args or definition. punt IS_A permanently. */
        MarkStatContext(statement,context_WRONG);
        WSS(ASCERR,statement);
        return 1;
      }
    }
    if (GetBaseType(def)==set_type) {
      intset = CalcSetType(GetStatSetType(statement),statement);
      if (intset < 0) { /* incorrect set type */
        STATEMENT_ERROR(statement,"Illegal set type encountered.");
        /* should never happen due to lint */
        return 0;
      }
    }else{
      intset = -1;
    }
    vlist = GetStatVarList(statement);
    while (vlist!=NULL){
      REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
      struct gl_list_t *instances;
      MakeInstance(NamePointer(vlist),def,intset,inst,statement,arginst);
      if (GetStatCheckValue(statement) != NULL) {
        instances = FindInstances(inst,NamePointer(vlist),&err);
        if (instances == NULL || gl_length(instances) != 1) {
          if (instances != NULL) {
            gl_destroy(instances);
          }
          if (arginst != NULL) {
            DestroyParameterInst(arginst);
          }
          STATEMENT_ERROR(statement,"DEFAULT declaration target must resolve to exactly one scalar instance");
          return 0;
        }
        if (!AssignISAInitialValue(inst,statement,(struct Instance *)gl_fetch(instances,1))) {
          gl_destroy(instances);
          if (arginst != NULL) {
            DestroyParameterInst(arginst);
          }
          return 0;
        }
        gl_destroy(instances);
      }
      vlist = NextVariableNode(vlist);
    }
    if (arginst != NULL) {
      DestroyParameterInst(arginst);
    }
    return 1;
  }else{
    /*
     * Should never happen, due to lint.
     */
    char *msg = ASC_NEW_ARRAY(char,strlen(UNDEFINED_TYPE_MESG)+SCLEN(GetStatType(statement))+1);
    strcpy(msg,UNDEFINED_TYPE_MESG);
    strcat(msg,SCP(GetStatType(statement)));
    STATEMENT_ERROR(statement,msg); /* added print. baa. string was here already*/
    ascfree(msg);
    return 1;
  }
}

/**
	handles construction of Dummy Instance
	A dummy instance is universal.
*/
static
void MakeDummyInstance(CONST struct Name *name,
                       struct TypeDescription *def,
                       struct Instance *parent,
                       struct Statement *statement)
{
  symchar *childname;
  unsigned long pos;
  struct Instance *inst;
  struct InstanceName rec;

  childname = SimpleNameIdPtr(name);
  if (childname==NULL) {
    childname = NameIdPtr(name);
  }
  SetInstanceNameType(rec,StrName);
  SetInstanceNameStrPtr(rec,childname);
  pos = ChildSearch(parent,&rec);
  if (pos>0) {
    if (InstanceChild(parent,pos)==NULL){
      inst = ShortCutMakeUniversalInstance(def);
      if (inst==NULL) {
        inst = CreateDummyInstance(def);
      }
      LinkToParentByPos(parent,inst,pos);
    }else{			/* redefining instance */
      char *msg = ASC_NEW_ARRAY(char,SCLEN(childname) + strlen(REDEFINE_CHILD_MESG)+1);
      strcpy(msg,REDEFINE_CHILD_MESG);
      strcat(msg,SCP(childname));
      STATEMENT_ERROR(statement,msg);
      ascfree(msg);
    }
  }else{			/* unknown child name */
      STATEMENT_ERROR(statement, "Unknown child name.  Never should happen");
      ASC_PANIC("Unknown child name.  Never should happen");
  }
}


/**
	Used for IS_A statement inside a non-matching CASE of a
	SELECT statement.

	Make a dummy instance for each name in vlisti, but arrays are not expanded
	over subscripts. The dummy instance is UNIVERSAL.
*/
static
int ExecuteUnSelectedISA( struct Instance *inst, struct Statement *statement)
{
  struct TypeDescription *def;
  CONST struct VariableList *vlist;
  asc_assert(StatementType(statement)==ISA);
  if ((def = FindDummyType())!=NULL){
    vlist = GetStatVarList(statement);
    while (vlist!=NULL){
      MakeDummyInstance(NamePointer(vlist),def,inst,statement);
      vlist = NextVariableNode(vlist);
    }
    return 1;
  }else{
    /*
     * Should never happen, due to lint.
     */
    char *msg = ASC_NEW_ARRAY(char,strlen(UNDEFINED_TYPE_MESG)+11);
    strcpy(msg,UNDEFINED_TYPE_MESG);
    strcat(msg,"dummy_type");
    STATEMENT_ERROR(statement,msg);
    ascfree(msg);
    return 1;
  }
}


/**
	For ALIASES inside a non matching CASEs of a SELECT statement, we
	do not even have to care about the rhs. Similar to ISAs, we only
	take the list of variables and create the dummy instance
*/
static
int ExecuteUnSelectedALIASES(struct Instance *inst,
                             struct Statement *statement)
{
  CONST struct VariableList *vlist;

  asc_assert(StatementType(statement)==ALIASES);
  vlist = GetStatVarList(statement);
  while (vlist!=NULL){
    MakeDummyInstance(NamePointer(vlist),FindDummyType(),inst,statement);
    vlist = NextVariableNode(vlist);
  }
  return 1;
}

/*------------------------------------------------------------------------------
	REFERENCE STATEMENT PROCESSING

	"Highly incomplete		KAA_DEBUG"
*/

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
struct Instance *RealExecuteRef(struct Name *name,
                                struct TypeDescription *def,
                                int intset,
                                struct Instance *parent,
                                struct Statement *statement)
{
  struct Instance *result = NULL;

  return result;
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */

static
int ExecuteREF(struct Instance *inst, struct Statement *statement)
{
  (void)inst;       /*  stop gcc whine about unused parameter */
  (void)statement;  /*  stop gcc whine about unused parameter */
  return 1;
}

/**
	Finds all the instances required to evaluate set element given.
	If problem, returns NULL and err should be consulted.
	Note this may have some angst around FOR vars, as it
	should since forvars are not instances.

	Lint is precluding passing a forvar where an instance is required.
	err should only be consulted if result comes back NULL.

	@NOTE that we will ignore any sets chained on to the end of s.
*/
static struct gl_list_t *FindArgInsts(struct Instance *parent,
	struct Set *s, rel_errorlist *err
){
  struct gl_list_t *result,*temp; /* instance lists */
  struct gl_list_t *nl=NULL; /* name list */
  unsigned nc,nlen;

  result = gl_create(2L);
  nl = EvaluateSetNamesNeededShallow(s,nl);
  nlen = gl_length(nl);
  for (nc=1; nc <= nlen; nc++) {
    temp = FindInstances(parent,(struct Name *)gl_fetch(nl,nc),err);
    if (temp==NULL){
      gl_destroy(nl);
      gl_destroy(result);
      return NULL;
    }
    gl_append_list(result,temp);
    gl_destroy(temp);
  }
  gl_destroy(nl);
  return result;
}


/**
   Missing instances: makes sure at least one thing is found for
   each name item on list (else prints the name with a little message)
   if noisy != 0 || on last iteration, does the printing, OTHERWISE
   returns immediately.
*/
static
void MissingInsts(struct Instance *inst,
                  CONST struct VariableList *list,
                  int noisy)
{
  struct gl_list_t *temp;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  if (g_iteration >= (MAXNUMBER-1) || noisy != 0) {
    while(list!=NULL){
      temp = FindInstances(inst,NamePointer(list),&err);
      if (temp==NULL){
        instantiation_name_error(ASC_USER_ERROR,NamePointer(list),
			"Instance not found"
		);
		error_reporter_end_flush();
      }else{
        gl_destroy(temp);
      }
      list = NextVariableNode(list);
    }
  }
}

/**
   Verify instances: Makes sure at least one thing is found for
   each name item on list. Returns 1 if so, or 0 if not.
   Does not return the collected instances.
*/
static
int VerifyInsts(struct Instance *inst,
	CONST struct VariableList *list, rel_errorlist *err
){
  struct gl_list_t *temp;
  while(list!=NULL){
    temp = FindInstances(inst,NamePointer(list),err);
    if (temp==NULL){
      gl_destroy(temp);
      return 0;
    }
    gl_destroy(temp);
    list = NextVariableNode(list);
  }
  return 1;
}

static
int SameClique(struct Instance *i1, struct Instance *i2)
{
  struct Instance *i=i1;
  do {
    if (i==i2) return 1;
    i = NextCliqueMember(i);
  } while(i!=i1);
  return 0;
}

static
int InPrecedingClique(struct gl_list_t *list, unsigned long int pos,
                      struct Instance *inst)
{
  unsigned long c;
  struct Instance *i;
  asc_assert(pos<=gl_length(list));
  for(c=1;c<pos;c++){
    i = (struct Instance *)gl_fetch(list,c);
    if (SameClique(i,inst)) return 1;
  }
  return 0;
}

/**
	This procedure takes time proportion to n^2.
*/
static
void RemoveExtras(struct gl_list_t *list){
  unsigned long c=1;
  struct Instance *inst;
  while(c<=gl_length(list)){
    inst = (struct Instance *)gl_fetch(list,c);
    if (InPrecedingClique(list,c,inst)) gl_delete(list,c,0);
    else c++;
  }
}

static
int ListContainsFundamental(struct gl_list_t *list)
{
  unsigned long c=1;
  CONST struct Instance *inst;
  while(c <= gl_length(list)){
    inst = (CONST struct Instance *)gl_fetch(list,c);
    if ( IsFundamentalInstance(inst) ){
      return 1;
    }
    c++;
  }
  return 0;
}

static
int ListContainsParameterized(struct gl_list_t *list)
{
  unsigned long c,len;
  CONST struct Instance *inst;
  CONST struct TypeDescription *d;

  len = gl_length(list);
  for (c=1; c <= len; c++) {
    inst = (CONST struct Instance *)gl_fetch(list,c);
    if (inst != NULL) {
      d = InstanceTypeDesc(inst);
      if (d != NULL) {
        if (TypeHasParameterizedInsts(d)!=0) {
          return 1;
        }
      }else{
        FPRINTF(ASCERR,"NULL TypeDescription in ExecuteAA\n");
        return 1;
      }
    }else{
      FPRINTF(ASCERR,"NULL instance in ExecuteAA\n");
      return 1;
    }
  }
  return 0;
}

static
int ExecuteIRT(struct Instance *work, struct Statement *statement)
{
  struct TypeDescription *def, *more_refined;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct gl_list_t *instances; /* presently leaking ? */
  struct Instance *inst, *arginst;
  unsigned long c,len;
  int suberr;

  asc_assert(StatementType(statement)==IRT);

  def = FindType(GetStatType(statement)); /* sort of redundant, but safe */
  if(def!=NULL) {
    instances = FindInsts(work,GetStatVarList(statement),&err);
    if(instances != NULL){
      if (ListContainsFundamental(instances)){
        STATEMENT_ERROR(statement,
              "IS_REFINED_TO statement affects a part of an atom");
        gl_destroy(instances);
        MarkStatContext(statement,context_WRONG);
        WSS(ASCERR,statement);
        return 1;
      }
      RemoveExtras(instances);	/* slow process to make sure each clique is */
                                /* only represented once in the list */
      suberr = MakeParameterInst(work,statement,&arginst,KEEPARGINST);/*2*/
      if (suberr != MPIOK) {
        gl_destroy(instances);
        if (suberr == MPIWAIT) {
          WriteUnexecutedMessage(ASCERR,statement,
            "Possibly undefined arguments in IS_REFINED_TO statement.");
          return 0;
        }else{
          /* bogus args or definition. punt IRT permanently. */
          MarkStatContext(statement,context_WRONG);
          WSS(ASCERR,statement);
          return 1;
        }
      }
      len = gl_length(instances);
      /* first we check compatibility -
       * no half executed statements and no parameterized cliques.
       */
      for(c=1;c<=len;c++){
        inst = (struct Instance *)gl_fetch(instances,c);
        more_refined = MoreRefined(def,InstanceTypeDesc(inst));
        if ( more_refined == NULL){
          FPRINTF(ASCERR,"Incompatible instance: ");
          WriteInstanceName(ASCERR,inst,work);
          STATEMENT_ERROR(statement,
               "Unconformable refinement in IS_REFINED_TO statement");
          gl_destroy(instances);
          MarkStatContext(statement,context_WRONG);
          WSS(ASCERR,statement);
          if (arginst!=NULL) {
            DestroyParameterInst(arginst);
          }
          return 1;
        }
        if (arginst!=NULL) {
          if (inst != NextCliqueMember(inst)) {
            FPRINTF(ASCERR,"ARE_ALIKE'd instance: ");
            WriteInstanceName(ASCERR,inst,work);
            STATEMENT_ERROR(statement,
              "Refinement of clique to parameterized type family disallowed");
            gl_destroy(instances);
            MarkStatContext(statement,context_WRONG);
            WSS(ASCERR,statement);
            DestroyParameterInst(arginst);
            return 1;
          }
          suberr = CheckParamRefinement(work,inst,arginst,statement);
          /* CheckParamRefinement is responsible for mpierrors wums */
          switch (suberr) {
          case MPIOK:
            break;
          case MPIWAIT:
            gl_destroy(instances);
            DestroyParameterInst(arginst);
            return 0;
          default:
            MarkStatContext(statement,context_WRONG);
            WSS(ASCERR,statement);
            DestroyParameterInst(arginst);
            return 1;
          }
        }
      }
      /* ok, so we're going to repeat a little list/type lookups */
      for(c=1;c<=len;c++){
        inst = (struct Instance *)gl_fetch(instances,c);
        more_refined = MoreRefined(def,InstanceTypeDesc(inst));
        if (more_refined == def) {
          /* whole set will need refining. */
          inst = RefineClique(inst,def,arginst);
        }
      }
      DestroyParameterInst(arginst);
      gl_destroy(instances);
      return 1;
    }else{
      switch(rel_errorlist_get_find_error(&err)){
      case impossible_instance:
        STATEMENT_ERROR(statement,
          "IS_REFINED_TO statement contains an impossible instance name");
        MissingInsts(work,GetStatVarList(statement),1);
        return 1;
      default:
        MissingInsts(work,GetStatVarList(statement),0);
        WriteUnexecutedMessage(ASCERR,statement,
                               "Could not execute IS_REFINED_TO");
        return 0; /* statement is not ready to be executed */
      }
    }
  }else{
    char *msg = ASC_NEW_ARRAY(char,strlen(IRT_UNDEFINED_TYPE)+SCLEN(GetStatType(statement))+1);
    strcpy(msg,IRT_UNDEFINED_TYPE);
    strcat(msg,SCP(GetStatType(statement)));
    STATEMENT_ERROR(statement,msg);
    ascfree(msg);
    return 1;
  }
}

/**
	This assumes that NULL is not in the list.
*/
static
void RemoveDuplicates(struct gl_list_t *list)
{
  VOIDPTR ptr=NULL;
  unsigned c=1;
  gl_sort(list,(CmpFunc)CmpPtrs);
  while(c<=gl_length(list)){
    if (ptr == gl_fetch(list,c)) {
      gl_delete(list,c,0);
    }else{
      ptr = gl_fetch(list,c);
      c++;
    }
  }
}

/**
	Return NULL if the list is not conformable or empty.  Otherwise,
	return the type description of the most refined instance.
*/
static
struct TypeDescription *MostRefined(struct gl_list_t *list)
{
  struct TypeDescription *mostrefined;
  struct Instance *inst;
  unsigned long c,len;
  asc_assert(list!=NULL);
  len = gl_length(list);
  if (len==0) return NULL;
  inst = (struct Instance  *)gl_fetch(list,1);
  mostrefined = InstanceTypeDesc(inst);
  for(c=2;c<=len;c++){
    inst = (struct Instance *)gl_fetch(list,c);
    mostrefined = MoreRefined(mostrefined,InstanceTypeDesc(inst));
    if (mostrefined==NULL) return NULL;
  }
  return mostrefined;
}


static int ExecuteATS(struct Instance *inst, struct Statement *statement){
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  unsigned long c,len;
  struct Instance *inst1,*inst2;

  instances = FindInsts(inst,GetStatVarList(statement),&err);
  if (instances != NULL){
    if (ListContainsFundamental(instances)){
      STATEMENT_ERROR(statement,
        "ARE_THE_SAME statement affects a part of an atom");
      gl_destroy(instances);
      return 1;
    }
    RemoveDuplicates(instances); /* make sure that no instances occurs */
                                 /* multiple times */
    if ((gl_length(instances)==0)||(MostRefined(instances)!=NULL)){
      len = gl_length(instances);
      if (len>1){
        int merge_failed = 0;
        inst1 = (struct Instance *)gl_fetch(instances,1);
        for(c=2;c<=len;c++){
          inst2 = (struct Instance *)gl_fetch(instances,c);
          inst1 = MergeInstances(inst1,inst2);
          if (inst1==NULL){
            STATEMENT_ERROR(statement, "ARE_THE_SAME merge failed");
            merge_failed = 1;
            break;
          }
        }
        if(!merge_failed){
          PostMergeCheck(inst1);
        }
      }
    }else{
      STATEMENT_ERROR(statement,
           "ARE_THE_SAME statement contains unconformable instances");
    }
    gl_destroy(instances);
    return 1;
  }else{
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance:
      MissingInsts(inst,GetStatVarList(statement),1);
      WriteStatementError(ASC_USER_ERROR,statement,1,"ARE_THE_SAME contains impossible instance");
      return 1;
    default:
      MissingInsts(inst,GetStatVarList(statement),0);
      WriteUnexecutedMessage(ASCERR,statement,
                             "Could not execute ARE_THE_SAME");
      return 0; /* statement is not ready to be executed */
    }
  }
}


/**
	disallows parameterized objects from being added to cliques.
*/
static int ExecuteAA(struct Instance *inst, struct Statement *statement){
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct TypeDescription *mostrefined = NULL;
  unsigned long c,len;
  struct Instance *inst1,*inst2;
  instances = FindInsts(inst,GetStatVarList(statement),&err);
  if (instances != NULL){
    if (ListContainsFundamental(instances)){
      STATEMENT_ERROR(statement, "ARE_ALIKE statement affects a part of an atom");
      gl_destroy(instances);
      return 1;
    }
    if (ListContainsParameterized(instances)){
      STATEMENT_ERROR(statement, "ARE_ALIKE statement affects parameterized type");
      gl_destroy(instances);
      return 1;
    }
    if ((gl_length(instances)==0) ||
        ((mostrefined = MostRefined(instances))!=NULL)){
      RemoveExtras(instances);	/* slow process to make sure each clique is */
                                /* only represented once in the list */
      len = gl_length(instances);
      /* refine instances */
      for(c=1;c<=len;c++){
        inst1 = (struct Instance *)gl_fetch(instances,c);
        inst2 = RefineClique(inst1,mostrefined,NULL);
        if (inst2!=inst1) {
          gl_store(instances,c,(char *)inst2);
        }
      }
      /* merge cliques */
      if (len>1){
        inst1 = (struct Instance *)gl_fetch(instances,1);
        for(c=2;c<=len;c++){
          inst2 = (struct Instance *)gl_fetch(instances,c);
          MergeCliques(inst1,inst2);
        }
      }
    }else{
      STATEMENT_ERROR(statement,
                    "ARE_ALIKE statement contains unconformable instances");
    }
    gl_destroy(instances);
    return 1;
  }else{
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance:
      MissingInsts(inst,GetStatVarList(statement),1);
      STATEMENT_ERROR(statement, "ARE_ALIKE contains impossible instance");
      return 1;
    default:
      MissingInsts(inst,GetStatVarList(statement),0);
      WriteUnexecutedMessage(ASCERR,statement,
                             "Could not execute ARE_ALIKE");
      return 0;
    }
  }
}


static int ExecuteLNK(struct Instance *inst, struct Statement *statement){
	struct TypeDescription *def_inst;
	REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
	struct gl_list_t *instances;
	symchar *key;

	asc_assert(StatementType(statement)==LNK);

	def_inst = InstanceTypeDesc(inst);
	instances = FindInsts(inst,LINKStatVlist(statement),&err);
	key = LINKStatKey(statement);

	if((instances != NULL) && (key != NULL)){
		switch (def_inst->t) {
		case model_type:
			if(statement->v.lnk.key_type == 2) {/* in case the LINK entry has the 'ignore' key */
				CONSOLE_DEBUG("Ignore declarative link");
				ignoreDeclLinkEntry(inst,key,LINKStatVlist(statement));
				gl_destroy(instances);
			}else{
				CONSOLE_DEBUG("Adding declarative link");
				addLinkEntry(inst,key,instances,statement,1);
			}
			return 1;
		default:
			gl_destroy(instances);
			STATEMENT_ERROR(statement, "LINK is not called by a model");
			return 1;
		}
	}else if(key == NULL){
		STATEMENT_ERROR(statement, "declarative LINK contains impossible key");
		return 1;
	}else{
		switch(rel_errorlist_get_find_error(&err)){
		case impossible_instance:
			MissingInsts(inst,LINKStatVlist(statement),1);
			STATEMENT_ERROR(statement, "LINK contains impossible instance");
			return 1;
		default:
			MissingInsts(inst,LINKStatVlist(statement),0);
			WriteUnexecutedMessage(ASCERR,statement, "Could not execute LINK");
			return 0;
		}
	}
}
/*------------------------------------------------------------------------------
	RELATION PROCESSING
*/

static
struct Instance *MakeRelationInstance(struct Name *name,
                                      struct TypeDescription *def,
                                      struct Instance *parent,
                                      struct Statement *stat,
                                      enum Expr_enum type)
{
  /* CONSOLE_DEBUG("..."); */
  symchar *childname;
  struct Instance *child;
  struct InstanceName rec;
  unsigned long pos;
  childname = SimpleNameIdPtr(name);
  if (childname!=NULL){
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,childname);
    pos = ChildSearch(parent,&rec);
    if(pos>0){
      /* following assertion should be true */
      asc_assert(InstanceChild(parent,pos)==NULL);
      child = CreateRelationInstance(def,type);	/* token, bbox relation so far */
      LinkToParentByPos(parent,child,pos);
      return child;
    }else{
      return NULL;
    }
  }else{				/* sparse array of relations */
    childname = NameIdPtr(name);
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,childname);
    pos = ChildSearch(parent,&rec);
    if (pos>0) {
      if (InstanceChild(parent,pos)==NULL){
        /* must make array */
        child = MakeSparseArray(parent,name,stat,NULL,0,NULL,NULL,NULL);
      }else{
      	/* must add array element */
        child = AddArrayChild(parent,name,stat,NULL,NULL,NULL);
      }
      return child;
    }else{
      return NULL;
    }
  }
}

static void ApplyInitialStatementFlags(struct Instance *eqninst,
                                       struct Statement *statement)
{
  struct Instance *flaginst;
  if ((GetStatContext(statement) & context_INITIAL) == 0) {
    return;
  }
  flaginst = ChildByChar(eqninst, AddSymbol("initial"));
  if (flaginst != NULL && InstanceKind(flaginst) == BOOLEAN_INST) {
    SetBooleanAtomValue(flaginst, TRUE, 0U);
  }
  flaginst = ChildByChar(eqninst, AddSymbol("included"));
  if (flaginst != NULL && InstanceKind(flaginst) == BOOLEAN_INST) {
    SetBooleanAtomValue(flaginst, FALSE, 0U);
  }
}


/**
	ok, now we can whine real loud about what's missing.
	even in relations referencing relations, because they
	should have been added to pendings in dependency order. (hah!)
*/
static int ExecuteREL(struct Instance *inst, struct Statement *statement){
	struct Name *name;
	REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
	struct relation *reln;
	struct Instance *child;
	struct gl_list_t *instances;
	enum Expr_enum reltype;
	//char *iname;

#ifdef DEBUG_RELS
	CONSOLE_DEBUG("ENTERED ExecuteREL");
#endif

	name = RelationStatName(statement);
	instances = FindInstances(inst,name,&err);
	/* see if the relation is there already */
	if(instances==NULL){
		if(rel_errorlist_get_find_error(&err) == unmade_instance){		/* make a reln head */
			child = MakeRelationInstance(name,FindRelationType(),
                                   inst,statement,e_token);
			if(child==NULL){
				STATEMENT_ERROR(statement, "Unable to create expression structure");
				/* print a better message here if needed. maybe an if!makeindices moan*/
				return 1;
			}
		}else{
			/* undefined instances in the relation name, or out of memory */
			WSSM(ASCERR,statement, "Unable to execute relation label",3);
			return 1;
		}
	}else{
		if(gl_length(instances)==1){
			child = (struct Instance *)gl_fetch(instances,1);
			asc_assert((InstanceKind(child)==REL_INST)||(InstanceKind(child)==DUMMY_INST));
			gl_destroy(instances);
			if (InstanceKind(child)==DUMMY_INST) {
#ifdef DEBUG_RELS
				STATEMENT_ERROR(statement, "DUMMY_INST foundin compiling relation.");
#endif
				return 1;
			}
#ifdef DEBUG_RELS
			STATEMENT_ERROR(statement, "REL_INST found in compiling relation.");
#endif
		}else{
			STATEMENT_ERROR(statement, "Expression name refers to more than one object");
			gl_destroy(instances);	/* bizarre! */
			return 1;
		}
	}

	/* child now contains the pointer to the relation instance.
	We should perhaps double check that the reltype
	has not been set or has been set to e_undefined. */
	if(GetInstanceRelation(child,&reltype)==NULL){
		if((g_instantiate_relns & TOKRELS) ==0){
#ifdef DEBUG_RELS
			STATEMENT_NOTE(statement, "TOKRELS 0 found in compiling relation.");
#endif
			return 1;
		}
#if TIMECOMPILER
		g_ExecuteREL_CreateTokenRelation_calls++;
#endif
		reln = CreateTokenRelation(inst,child,RelationStatExpr(statement),&err);
		if(reln != NULL){
			SetInstanceRelation(child,reln,e_token);
      ApplyInitialStatementFlags(child,statement);
#ifdef DEBUG_RELS
			STATEMENT_NOTE(statement, "Created relation");
#endif
			return 1;
		}else{
			SetInstanceRelation(child,NULL,e_token);
            rel_errorlist_report_error(&err,statement);
		}
#ifdef DEBUG_RELS
		STATEMENT_NOTE(statement, "   Failed relation -- unexpected scenario.");
#endif
	}else{
		/*  Do nothing, somebody already completed the relation.  */
#ifdef DEBUG_RELS
		STATEMENT_NOTE(statement, "Already compiled in compiling relation?!.");
#endif
		return 1;
	}
#ifdef DEBUG_RELS
	STATEMENT_NOTE(statement, "End of ExecuteREL. huh?");
#endif
	return 0;
}

/**
	set a relation instance as Conditional. This is done by activating
	a bit ( relinst_set_conditional(rel,TRUE) ) and by using a flag
	SetRelationIsCond(reln). Only one of these two would be strictly
	required
*/
static
void MarkREL(struct Instance *inst, struct Statement *statement)
{
  struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct relation *reln;
  struct Instance *rel;
  struct gl_list_t *instances;
  enum Expr_enum reltype;

  name = RelationStatName(statement);
  instances = FindInstances(inst,name,&err);
  if (instances==NULL){
    gl_destroy(instances);
    return;
  }
  else{
    if(gl_length(instances)==1){
      rel = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      asc_assert(InstanceKind(rel)==REL_INST);
      relinst_set_conditional(rel,TRUE);
      reln = GetInstanceRelToModify(rel,&reltype);
      if (reln == NULL) {
        return ;
      }
      SetRelationIsCond(reln);
    }else{         /* expression name refers to more than one object */
      gl_destroy(instances);
      return;
    }
  }
}

/**
	set a relation instance as Conditional. This is done by activating
	a bit ( relinst_set_conditional(rel,TRUE) ) and by using a flag
	SetRelationIsCond(reln). Only one of these two would be strictly
	required

	@TODO FIXME. probably should look similar to MarkREL.
	Remember that the compiled instance of an EXT statement
	is still a Relation instance.
*/
static void MarkEXT(struct Instance *inst, struct Statement *statement){
	(void)inst; (void)statement;

        FPRINTF(stderr,"MarkEXT: external rels in conditional statements not fully supported yet.\n");
}

/**
	set a logical relation instance as Conditional. This is done by activating
	a bit ( logrelinst_set_conditional(lrel,TRUE) ) and by using a flag
	SetLogRelIsCond(reln). Only one of these two would be strictly
	required
*/
static
void MarkLOGREL(struct Instance *inst, struct Statement *statement)
{
  struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct logrelation *lreln;
  struct Instance *lrel;
  struct gl_list_t *instances;

  name = LogicalRelStatName(statement);
  instances = FindInstances(inst,name,&err);
  if (instances==NULL){
    gl_destroy(instances);
    return;
  }
  else{
    if(gl_length(instances)==1){
      lrel = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      asc_assert(InstanceKind(lrel)==LREL_INST);
      logrelinst_set_conditional(lrel,TRUE);
      lreln = GetInstanceLogRelToModify(lrel);
      if (lreln == NULL) {
        return;
      }
      SetLogRelIsCond(lreln);
    }else{          /* expression name refers to more than one object */
      gl_destroy(instances);
      return;
    }
  }
}


/**
	For its use in ExecuteUnSelectedStatements.
	Execute the REL or LOGREL statements inside those cases of a SELECT
	which do not match the selection variables
*/
static
int ExecuteUnSelectedEQN(struct Instance *inst, struct Statement *statement)
{
  struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *child;
  struct gl_list_t *instances;

  switch(StatementType(statement)) {
  case REL:
    name = RelationStatName(statement);
    break;
  case EXT:
    name = ExternalStatNameRelation(statement);
    break;
  case LOGREL:
    name = LogicalRelStatName(statement);
    break;
  default:
    ASC_PANIC("Incorrect argument passed to ExecuteUnSelectedEQN\n");
	name = NULL;
  }
  instances = FindInstances(inst,name,&err);
  /* see if the relation is there already */
  if (instances==NULL) {
    MakeDummyInstance(name,FindDummyType(),inst,statement);
  }else{
    if(gl_length(instances)==1){
      child = (struct Instance *)gl_fetch(instances,1);
      asc_assert(InstanceKind(child)==DUMMY_INST);
      gl_destroy(instances);
    }else{
      STATEMENT_ERROR(statement, "Expression name refers to more than one object");
      gl_destroy(instances);
      ASC_PANIC("Expression name refers to more than one object");
    }
  }
  return 1;
}

/*------------------------------------------------------------------------------
  LOGICAL RELATIONS PROCESSING

  Making instances of logical relations or arrays of instances of
  logical relations.
*/

static
struct Instance *MakeLogRelInstance(struct Name *name,
                                    struct TypeDescription *def,
                                    struct Instance *parent,
                                    struct Statement *stat)
{
  symchar *childname;
  struct Instance *child;
  struct InstanceName rec;
  unsigned long pos;
  if ((childname=SimpleNameIdPtr(name))!=NULL){ /* simple name */
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,childname);
    if(0 != (pos = ChildSearch(parent,&rec))){
      /* following assertion should be true */
      asc_assert(InstanceChild(parent,pos)==NULL);
      child = CreateLogRelInstance(def);
      LinkToParentByPos(parent,child,pos);
      return child;
    }else{
      return NULL;
    }
  }else{				/* sparse array of logical relations */
    childname = NameIdPtr(name);
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,childname);
    if(0 != (pos = ChildSearch(parent,&rec))){
      if (InstanceChild(parent,pos)==NULL){ /* need to make array */
        child = MakeSparseArray(parent,name,stat,NULL,0,NULL,NULL,NULL);
      }else{			/* need to add array element */
        child = AddArrayChild(parent,name,stat,NULL,NULL,NULL);
      }
      return child;
    }else{
      return NULL;
    }
  }
}

static
int ExecuteLOGREL(struct Instance *inst, struct Statement *statement)
{
  struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct logrelation *lreln;
  struct Instance *child;
  struct gl_list_t *instances;

  name = LogicalRelStatName(statement);
  instances = FindInstances(inst,name,&err);
  /* see if the logical relation is there already */
  if (instances==NULL){
    gl_destroy(instances);
    if(rel_errorlist_get_find_error(&err) == unmade_instance){
      child = MakeLogRelInstance(name,FindLogRelType(),inst,statement);
      if (child==NULL){
        WUEMPASS3(ASCERR,statement, "Unable to create logical expression structure");
        /* print a better message here if needed */
        return 1;
      }
    }
    else {
      WUEMPASS3(ASCERR,statement, "Unable to execute expression");
      return 1;
    }
  }
  else{
    if(gl_length(instances)==1){
      child = (struct Instance *)gl_fetch(instances,1);
      asc_assert((InstanceKind(child)==LREL_INST)||(InstanceKind(child)==DUMMY_INST));
      gl_destroy(instances);
      if (InstanceKind(child)==DUMMY_INST) {
        return 1;
      }
    }else{
      WUEMPASS3(ASCERR,statement,
                           "Expression name refers to more than one object");
      gl_destroy(instances);
      return 1;
    }
  }

  /*
   * child now contains the pointer to the logical relation.
   */
  if (GetInstanceLogRel(child)==NULL){
    /*    if ( (g_instantiate_relns & TOKRELS) ==0) {
      return 1;
    }  */
    if(NULL!=(lreln = CreateLogicalRelation(
		inst,child,LogicalRelStatExpr(statement),&err)
	)){
      SetInstanceLogRel(child,lreln);
      ApplyInitialStatementFlags(child,statement);
      return 1;
    }else{
      SetInstanceLogRel(child,NULL);
      switch(rel_errorlist_get_lrcode(&err)){
      case incorrect_logstructure:
        WUEMPASS3(ASCERR,statement,
                       "Bad logical relation expression in ExecuteLOGREL\n");
        return 0;
      case incorrect_linst_type:
        WUEMPASS3(ASCERR,statement,
                             "Incorrect instance types in logical relation");
        return 0;
      case incorrect_boolean_linst_type:
        WUEMPASS3(ASCERR,statement,
             "Incorrect boolean child of atom instance in logical relation");
        return 0;
      case incorrect_integer_linst_type:
        WUEMPASS3(ASCERR,statement,
                           "Incorrect integer instance in logical relation");
        return 0;
      case incorrect_symbol_linst_type:
        WUEMPASS3(ASCERR,statement,
                            "Incorrect symbol instance in logical relation");
        return 0;
      case incorrect_real_linst_type:
        WUEMPASS3(ASCERR,statement,
                              "Incorrect real instance in logical relation");
        return 0;
      case find_logerror:
        switch(rel_errorlist_get_find_error(&err)){
        case unmade_instance:
        case undefined_instance:
          WUEMPASS3(ASCERR,statement,
                       "Unmade or Undefined instances in logical relation");
          return 0;
        case impossible_instance:
          WUEMPASS3(ASCERR,statement,
                        "Logical Relation contains an impossible instance");
          return 0;
        case correct_instance:
          ASC_PANIC("Incorrect error response.\n");/*NOTREACHED*/
        default:
          ASC_PANIC("Unknown error response.\n");/*NOTREACHED*/
        }
      case boolean_value_undefined:
        WUEMPASS3(ASCERR,statement,
                                "Unassigned constants in logical relation");
          return 0;
      case lokay:
        ASC_PANIC("Incorrect error response.\n");/*NOTREACHED*/

      default:
        ASC_PANIC("Unknown error response.\n");/*NOTREACHED*/

      }
    }
  }else{
    /* do nothing. someone already completed the logrelation */
    return 1;
  }
}

/*==============================================================================
  EXTERNAL CALL PROCESSING
*/

/*------------------------------------------------------------------------------
  BLACK BOX RELATIONS PROCESSING
*/

/**
	Verify that all instances named in the arglist of lists are real_atoms.
*/
static
int CheckExtCallArgTypes(struct gl_list_t *arglist)
{
  unsigned long len1,c1;
  unsigned long len2,c2;
  struct gl_list_t *branch;
  struct Instance *arg;

  len1 = gl_length(arglist);
  for (c1=1;c1<=len1;c1++){
    branch = (struct gl_list_t *)gl_fetch(arglist,c1);
    if (!branch) return 1;
    len2 = gl_length(branch);
    for(c2=1;c2<=len2;c2++){
      arg = (struct Instance *)gl_fetch(branch,c2);
      if ((InstanceKind(arg)) != REAL_ATOM_INST) {
        return 1;
      }
    }
  }
  return 0;
}

/*
 @param names a pointer address that will be set to a list or null
  to provide the names result.
 */
static /* blackbox only */
struct gl_list_t *GetExtCallArgs(struct Instance *inst, struct Statement *stat
	,rel_errorlist *err, struct gl_list_t **names
){
  CONST struct VariableList *vl;
  struct gl_list_t *result;
  REL_ERRORLIST err2 = REL_ERRORLIST_EMPTY;

  /* the two process calls could be merged if we change findpaths
     to return instance/name pairs rather than just names. it does
     know the instances.
   */

  vl = ExternalStatVlistRelation(stat);
  result = ProcessExtRelArgs(inst,vl,err);

  /* FIXME is it OK to set *names=NULL? memory leak? */
  *names = NULL;
  if (result != NULL) {
    *names = ProcessExtRelArgNames(inst,vl,&err2);
    if (*names == NULL) {
      DestroySpecialList(result);
      rel_errorlist_set_find_error(err, rel_errorlist_get_find_error(&err2));
      return NULL;
    }
    asc_assert(rel_errorlist_get_find_error(err) == rel_errorlist_get_find_error(&err2));
  }
  return result;
}

static /* blackbox only */
struct Instance *CheckExtCallData(struct Instance *inst, struct Statement *stat,
	rel_errorlist *err, struct Name **name
){
  struct Name *n;
  struct Instance *result;
  /* we need a second error list here, since we need a separate check */
  REL_ERRORLIST err2 = REL_ERRORLIST_EMPTY;

  n = ExternalStatDataBlackBox(stat);
  result = ProcessExtRelData(inst, n, err);

  *name = NULL;
  if (result != NULL) {
    *name = ProcessExtRelDataName(inst, n, &err2);
    asc_assert(rel_errorlist_get_find_error(err)==rel_errorlist_get_find_error(&err2));
  }
  return result;
}


static int Pass2ExecuteBlackBoxEXTLoop(struct Instance *inst, struct Statement *statement);

int ExecuteBBOXElement(struct Instance *inst, struct Statement *statement, struct Instance *subject, struct gl_list_t *inputs,  struct BlackBoxCache * common, long c, CONST char *context);

/**
	This function does the job of creating an instance of a 'black box' external
	relation or set of relations. It does it by working as if the user wrote a 
	for loop over the outputs. While managing this for loop, we have to make
	sure that if we are already inside a for loop we don't mess that one up.
	The role of savedfortable is to manage the change of model context when one
	model instantiates another inside a for loop. For relations in particular, 
	the saved for table will always be null unless we drop the multipass
	instantiation scheme.
*/
static int ExecuteBlackBoxEXT(struct Instance *inst
	,struct Statement *statement
){
  int alreadyInFor = 0;
  int executeStatus = 0;
  struct for_table_t *SavedForTable;

  if(GetEvaluationForTable() != NULL){
	/* is this the right test? FIXME if not multipass instantiation. */
	/* as written, this might see a fortable from elsewhere */
    alreadyInFor = 1;
  }

  if(!alreadyInFor){ /* pushing/popping null table...*/
    SavedForTable = GetEvaluationForTable();
    SetEvaluationForTable(CreateForTable());
  }
  executeStatus = Pass2ExecuteBlackBoxEXTLoop(inst,statement);
  if(!alreadyInFor){
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
  }
  if(executeStatus){
	/* don't report an error here, because return 1 is possible even with
	eventual success at later compiler passes. */
	//ERROR_REPORTER_HERE(ASC_USER_ERROR,"Failed to execute blackbox statement");
    return 1;
  }else{
    return 0;
  }
}

/* This looks a lot like a FOR loop, since we're building an array,
but is very specialized to eliminate the user trying to declare
the for loop of an external relation correctly. Basically they
would have to write the output index expression identically twice, which
apparently is too hard for some.
@param instance: the enclosing model.
@param statement: the EXT bbox statement.
*/
int Pass2ExecuteBlackBoxEXTLoop(struct Instance *inst, struct Statement *statement){
  int rval = 1;
  symchar *name;
  struct Expr *ex, *one, *en;
  unsigned long c,len;
  long aindex;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;

  struct BlackBoxCache * common = NULL;
  char *context = NULL;
  struct Instance *data=NULL, *subject = NULL;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct gl_list_t *arglist=NULL;
  struct ExternalFunc *efunc = NULL;
  CONST char *funcname = NULL;
  unsigned long n_input_args=0L, n_output_args=0L; /* formal arg counts */
  unsigned long n_inputs_actual=0L, n_outputs_actual=0L; /* atomic arg counts */
  struct gl_list_t *inputs = NULL, *outputs = NULL, *argListNames = NULL;
  struct Name *dataName = NULL;
  unsigned long start,end;
  struct Set *extrange= NULL;
  int value_ready = 0;

  IVAL(value);

  /* common stuff do once ------------ */

  /*
	note ignoring return codes as all is guaranteed to work by passing
	the checks done before this statement was attempted.
  */
  if (ExternalStatDataBlackBox(statement) != NULL) {
    data = CheckExtCallData(inst, statement, &err, &dataName);
    /* should never be here, if checkext were being used properly somewhere. it isn't */
    if (data==NULL && rel_errorlist_get_find_error(&err) != correct_instance){
      /* should never be here, if checkext were being used properly somewhere. it isn't */
      switch(rel_errorlist_get_find_error(&err)){
      case unmade_instance:
		STATEMENT_ERROR(statement,"Statement contains unmade data instance");
        goto cleanup;
      case undefined_instance:
        STATEMENT_ERROR(statement,"Statement contains undefined data instance\n");
        goto cleanup; /* for the time being give another crack */
      case impossible_instance:
        STATEMENT_ERROR(statement,"Statement contains impossible data instance\n");
        goto cleanup;
      default:
        STATEMENT_ERROR(statement,"Unhandled case!");
        goto cleanup;
      }
    }
  }

  /* expand the formal args into a list of lists of realatom args. */
  arglist = GetExtCallArgs(inst, statement, &err, &argListNames);
  if (arglist==NULL){
    /* should never be here, if checkext were being used properly somewhere. it isn't */
    switch(rel_errorlist_get_find_error(&err)){
    case unmade_instance:
      STATEMENT_ERROR(statement,"Statement contains unmade argument instance\n");
      goto cleanup;
    case undefined_instance:
      STATEMENT_ERROR(statement,"Statement contains undefined argument instance\n");
      goto cleanup;
    case impossible_instance:
      instantiation_error(ASC_USER_ERROR,statement,"Statement contains impossible instance\n");
      goto cleanup;
    default:
      instantiation_error(ASC_PROG_ERR,statement,"Unhandled case!");
      goto cleanup;
    }
  }
  funcname = ExternalStatFuncName(statement);
  efunc = LookupExtFunc(funcname);
  if (efunc == NULL) {
    goto cleanup;
  }
/*
  n_input_args = NumberInputArgs(efunc);
  n_output_args = NumberOutputArgs(efunc);
 */
  //so lets see if getting i and o works better than commented code above
  n_input_args = statement->v.ext.u.black.n_inputs;
  n_output_args = statement->v.ext.u.black.n_outputs;

  if ((len =gl_length(arglist)) != (n_input_args + n_output_args)) {
    instantiation_error(ASC_PROG_ERR,statement
		,"Unable to create external expression structure."
	);
    goto cleanup;
  }

  /* we should have a valid arglist at this stage */
  if (CheckExtCallArgTypes(arglist)) {
    instantiation_error(ASC_USER_ERROR,statement,"Wrong type of args to external statement");
    goto cleanup;
  }
  start = 1L;
  end = n_input_args;
  inputs = LinearizeArgList(arglist,start,end);
  if (inputs == NULL) {
    instantiation_error(ASC_PROG_ERR,statement,"Unable to linearize external input arguments.");
    goto cleanup;
  }
  n_inputs_actual = gl_length(inputs);

  /* Now process the outputs */
  start = n_input_args+1;
  end = n_input_args + n_output_args;
  outputs = LinearizeArgList(arglist,start,end);
  if (outputs == NULL) {
    instantiation_error(ASC_PROG_ERR,statement,"Unable to linearize external output arguments.");
    goto cleanup;
  }
  n_outputs_actual = gl_length(outputs);

/*
  char *tempnamestr = WriteNameString(dataName);
  CONSOLE_DEBUG("dataName = %s", tempnamestr);
  ASC_FREE(tempnamestr);
*/

  /* Now create the relations, all with the same common. */
  common = CreateBlackBoxCache(n_inputs_actual,n_outputs_actual, argListNames, dataName, efunc);
  InitBBox(inst, common);
  context = WriteInstanceNameString(inst, NULL);

  /* now set up the for loop index --------------------------------*/
  name = AddSymbol(BBOX_RESERVED_INDEX);
  /* using a reserved character not legal in user level modeling. */
  asc_assert(FindForVar(GetEvaluationForTable(),name) == NULL);
  /* cannot happen as bbox definitions don't nest as statements and	user identifiers cannot contain ?. */

  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  /* construct a set value of 1..bbox_arraysize */
  one = CreateIntExpr(1L); /* destroyed with set. */
  en = CreateIntExpr(n_outputs_actual); /* destroyed with set. */
  extrange = CreateRangeSet(one,en);
  ex = CreateSetExpr(extrange);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  value_ready = 1;
  SetEvaluationContext(NULL);

  ASC_ASSERT_EQ(ValueKind(value),set_value);
  sptr = SetValue(value);
  ASC_ASSERT_EQ(SetKind(sptr),integer_set);
  fv = CreateForVar(name);
  SetForVarType(fv,f_integer);
  AddLoopVariable(GetEvaluationForTable(),fv);
  len = Cardinality(sptr);
  #ifdef DEBUG_RELS
  ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"Pass2RealExecuteFOR integer_set %lu.\n",len);
  #endif
  for(c=1;c<=len;c++){
    aindex = FetchIntMember(sptr,c);
    SetForInteger(fv,aindex);
    subject = (struct Instance *)gl_fetch(outputs,aindex);
    ExecuteBBOXElement(inst, statement, subject, inputs, common, aindex, context);
    /*  currently designed to always succeed or fail permanently */
  }
  RemoveForVariable(GetEvaluationForTable());
  rval = 1;
cleanup:
  if (value_ready) {
    DestroyValue(&value);
  }
  if (extrange != NULL) {
    DestroySetNode(extrange);
  }

/* ------------ */ /* ------------ */
  /* and now for cleaning up shared data. */
  if (common != NULL) {
    common->interp.task = bb_none;
    DeleteRefBlackBoxCache(NULL, &common);
  }
  if (context != NULL) {
    ascfree(context);
  }
  if (inputs != NULL) {
    gl_destroy(inputs);
  }
  if (outputs != NULL) {
    gl_destroy(outputs);
  }
  if (arglist != NULL) {
    DestroySpecialList(arglist);
  }
  if (argListNames != NULL) {
    DeepDestroySpecialList(argListNames,(DestroyFunc)DestroyName);
  }
  if (dataName != NULL) {
    DestroyName(dataName);
  }
/* ------------ */ /* ------------ */

  /*  currently designed to always succeed or fail permanently.
   *  We reached this point meaning we've processed everything.
   *  Therefore the statment returns 1 and becomes no longer pending.
   */
  return rval;
}

int ExecuteBBOXElement(struct Instance *inst
	, struct Statement *statement, struct Instance *subject
	, struct gl_list_t *inputs,  struct BlackBoxCache * common, long c
	, CONST char *context
){
  struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct gl_list_t *instances = NULL;
  struct Instance *child = NULL;
  struct relation *reln  = NULL;
  enum Expr_enum reltype;

#ifdef DEBUG_RELS
  CONSOLE_DEBUG("ENTERED ExecuteBBOXElement\n");
#endif

  /* make or find the instance */
  name = ExternalStatNameRelation(statement);
  instances = FindInstances(inst, name, &err);
  /* see if the relation is there already. If it is, we're really hosed. */
  if(instances==NULL){
    if(rel_errorlist_get_find_error(&err) == unmade_instance){		/* make a reln head */
      child = MakeRelationInstance(name,FindRelationType(),
                                   inst,statement,e_blackbox);
      if (child==NULL){
        STATEMENT_ERROR(statement, "Unable to create external black-box expression structure (unmade instance)");
       /* print a better message here if needed. maybe an if!makeindices moan*/
        return 1;
      }
    }else{
      /* undefined instances in the relation name, or out of memory */
      WSSM(ASCERR,statement, "Unable to execute blackbox label",3);
      return 1;
    }
  }else{
    if(gl_length(instances)==1){
      child = (struct Instance *)gl_fetch(instances,1);
      asc_assert((InstanceKind(child)==REL_INST) || (InstanceKind(child)==DUMMY_INST));
      gl_destroy(instances);
      if (InstanceKind(child)==DUMMY_INST) {
#ifdef DEBUG_RELS
        STATEMENT_ERROR(statement, "DUMMY_INST found in compiling blackbox.");
#endif
        return 1;
      }
#ifdef DEBUG_RELS
      STATEMENT_ERROR(statement, "REL_INST found in compiling blackbox.");
#endif
    }else{
      STATEMENT_ERROR(statement, "Expression name refers to more than one object");
      gl_destroy(instances);	/* bizarre! */
      return 1;
    }
  }

  /*
   * child now contains the pointer to the relation instance.
   * We should perhaps double check that the reltype
   * has not been set or has been set to e_undefined.
   */
  if (GetInstanceRelation(child,&reltype)==NULL) {
    if ( (g_instantiate_relns & BBOXRELS) ==0) {
#ifdef DEBUG_RELS
      STATEMENT_NOTE(statement, "BBOXRELS 0 found in compiling bbox.");
#endif
      return 1;
    }
#if TIMECOMPILER
    g_ExecuteEXT_CreateBlackboxRelation_calls++;
#endif
    reln = CreateBlackBoxRelation(child,
				subject, inputs,
				common, c-1, context);
    asc_assert(reln != NULL); /* cbbr does not return null */
    SetInstanceRelation(child,reln,e_blackbox);
#ifdef DEBUG_RELS
    STATEMENT_NOTE(statement, "Created bbox relation.");
#endif
    return 1;
  }else{
    /*  Do nothing, somebody already completed the relation.  */
#ifdef DEBUG_RELS
        STATEMENT_NOTE(statement, "Already compiled element in blackbox?!.");
#endif
    return 1;
  }
#ifdef DEBUG_RELS
  STATEMENT_NOTE(statement, "End of ExecuteBBOX. huh?");
  /* not reached */
#endif
}

/*------------------------------------------------------------------------------
  GLASS BOX RELATIONS PROCESSING

	GlassBox relations processing. As is to be expected, this code
	is a hybrid between TRUE ascend relations and blackbox relations.
*/

#if 0

static struct gl_list_t *CheckGlassBoxArgs(struct Instance *inst
	,struct Statement *stat,rel_errorlist *err
){
  struct Instance *var;
  CONST struct VariableList *vl;
  struct gl_list_t *varlist = NULL, *tmp = NULL;
  unsigned long len,c;
  int error = 0;

  vl = ExternalStatVlistRelation(stat);
  if (!vl) {
    rel_errorlist_set_find_error(err,impossible_instance); /* a relation with no incidence ! */
    return NULL;
  }

  ListMode = 1; /* order is very important */
  varlist = gl_create(NO_INCIDENCES); /* could be fine tuned */
  while (vl!=NULL) {
    tmp = FindInstances(inst,NamePointer(vl),err);
    if (tmp) {
      len = gl_length(tmp);
      for (c=1;c<=len;c++) {
        var = (struct Instance *)gl_fetch(tmp,c);
        if (InstanceKind(var) != REAL_ATOM_INST) {
          error++;
          rel_errorlist_set_code(err,incorrect_inst_type);
          //rel_errorlist_add(err,incorrect_inst_type,var,stat);
          rel_errorlist_set_find_error(err,correct_instance);
          gl_destroy(tmp);
          goto cleanup;
        }
        gl_append_ptr(varlist,(VOIDPTR)var);
      }
      gl_destroy(tmp);
    }else{ /* ferr will be already be set */
      error++;
      goto cleanup;
    }
    vl = NextVariableNode(vl);
  }

cleanup:
  ListMode = 0;
  if (error) {
    gl_destroy(varlist);
    return NULL;
  }else{
    return varlist;
  }
}


static int CheckGlassBoxIndex(struct Instance *inst
	, struct Statement *stat, rel_errorlist *err
){
  int result;
  long int iresult;
  char *tail;
  CONST struct Name *n;
  symchar *str;		   /* a string representation of the index */

  (void)inst;  /*  stop gcc whine about unused parameter  */

  n = ExternalStatDataGlassBox(stat);
  if(!n){
    rel_errorlist_set_code(err,incorrect_num_args);
	//rel_errorlist_add(err,incorrect_num_args,inst,stat);/* we must have an index */
    return -1;
  }

  str = SimpleNameIdPtr(n);
  if(str){
#if 0
    result = atoi(SCP(str));	/* convert to integer. use strtol */
#endif
    errno = 0;
    iresult = strtol(SCP(str),&tail,0);
    if (errno != 0 || (iresult == 0 && tail == SCP(str))) {
      rel_errorlist_set_code(err,incorrect_structure);
      //rel_errorlist_add(err,incorrect_structure,inst,stat);
      return -1;
    }
    result = iresult; /* range errror possible. */
    rel_errorlist_set_code(err,okay);
    return result;
  }else{
	rel_errorlist_set_code(err,incorrect_structure);
    //rel_errorlist_add(err,incorrect_structure,inst,stat);		/* we really need to expand */
    return -1;				/* the relation_error types. !! */
  }
}

static int ExecuteGlassBoxEXT(struct Instance *inst
	, struct Statement *statement
){
  struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *child;
  struct gl_list_t *instances;
  struct gl_list_t *varlist;
  struct relation *reln;
  struct ExternalFunc *efunc;
  CONST char *funcname;
  enum Expr_enum type;
  int gbindex;

  /*
   * Get function call details. The external function had better
   * loaded at this stage or report an error. No point in wasting
   * time.
   */
  funcname = ExternalStatFuncName(statement);
  efunc = LookupExtFunc(funcname);
  if (!efunc) {
    FPRINTF(ASCERR,"External function %s was not loaded\n",funcname);
    return 1;
  }

  name = ExternalStatNameRelation(statement);
  instances = FindInstances(inst,name,&err);
  if(instances==NULL){
    if(rel_errorlist_get_find_error(&err) == unmade_instance){			/* glassbox reln */
      child = MakeRelationInstance(name,FindRelationType(),inst,statement,e_glassbox);
      if (child==NULL){
        STATEMENT_ERROR(statement, "Unable to create glass box expression structure");
        return 1;
      }
    }else{
      STATEMENT_ERROR(statement, "Unable to execute expression");
      return 1;
    }
  }else{
    if(gl_length(instances)==1){
      child = (struct Instance *)gl_fetch(instances,1);
      ASC_ASSERT_EQ(InstanceKind(child), REL_INST);
      gl_destroy(instances);
    }else{
      STATEMENT_ERROR(statement, "Expression name refers to more than one object");
      gl_destroy(instances);
      return 1;
    }
  }

  /*
   * child now contains the pointer to the relation instance;
   * Ensure that the variable list is ready.
   */
  /* FIX FIX FIX -- give some more error diagnostics for err and ferr */
  varlist = CheckGlassBoxArgs(inst,statement,&err);
  if (varlist==NULL){
    switch(rel_errorlist_get_find_error(&err)){
    case unmade_instance:
      return 0;
    case undefined_instance:
      return 0; /* for the time being give another crack */
    case impossible_instance:
      instantiation_error(ASC_PROG_ERROR,statement
			,"Statement contains impossible instance\n");
      return 1;
    default:
      instantiation_error(ASC_PROG_ERROR,statement
			,"Something really wrong in ExecuteGlassEXT routine\n");
      return 1;
    }
  }

  /*
   * Get the gbindex of the relation for mapping into the external
   * call. An gbindex < 0 is invalid.
   */
  gbindex = CheckGlassBoxIndex(inst,statement,&err);
  if (gbindex < 0) {
    instantiation_error(ASC_USER_ERROR,statement
	    ,"Invalid index in external relation statement");
    return 1;
  }

  /*
   * All should be ok at this stage. Create the relation
   * structure and attach it to the relation instance.
   * CreateGlassBoxRelation makes a copy of the varlist.
   * But before we go through the trouble of making the
   * relation, we will check that none exists already. If
   * one has been created we cleanup and return 1.
   */
  if (GetInstanceRelation(child,&type)!=NULL) {
    goto error;
  }
  reln = CreateGlassBoxRelation(child,efunc,varlist,gbindex,e_equal);
  if (!reln) {
    Asc_Panic(2, __FUNCTION__,
      "Major error: Unable to create external relation structure."
    );
  }
  SetInstanceRelation(child,reln,e_glassbox);

 error:
  if (varlist) gl_destroy(varlist);
  return 1;
}
#endif


static int ExecuteEXT(struct Instance *inst, struct Statement *statement){
  int mode;

  /* CONSOLE_DEBUG("..."); */
  mode = ExternalStatMode(statement);
  switch(mode) {
  case ek_method:
    instantiation_error(ASC_USER_ERROR,statement
			,"Invalid external statement in declarative section. \n");
    return 1;
#if 0
  case ek_glass:
    return ExecuteGlassBoxEXT(inst,statement);
#endif
  case ek_black:
    return ExecuteBlackBoxEXT(inst,statement);
  default:
    instantiation_error(ASC_USER_ERROR,statement
			,"Invalid external statement in declarative section. \n");
    return 1;
  }
}

/*------------------------------------------------------------------------------
  ASSIGNMENT PROCESSING
*/

static void StructuralAsgnErrorReport(struct Statement *statement
	, struct value_t *value
){
  STATEMENT_ERROR(statement,
    "Structural assignment right hand side is not constant");
  DestroyValue(value);
}


/*
 * returns 1 if error will be persistent, or 0 if error may
 * go away later when more compiling is done.
 * Issues some sort of message in the case of persistent errors.
 */
static int AsgnErrorReport(struct Statement *statement, struct value_t *value){
  switch(ErrorValue(*value)){
  case undefined_value:
  case name_unfound: DestroyValue(value); return 0;
  case incorrect_name:
    STATEMENT_ERROR(statement,
         "Assignment right hand side contains non-existent instance");
    DestroyValue(value);
    return 1;
  case temporary_variable_reused:
    STATEMENT_ERROR(statement, "Assignment re-used temporary variable");
    DestroyValue(value);
    return 1;
  case dimension_conflict:
    STATEMENT_ERROR(statement,
          "Assignment right hand side is dimensionally inconsistent");
    DestroyValue(value);
    return 1;
  case incorrect_such_that:
    STATEMENT_ERROR(statement, "Assignment uses incorrect such that expression");
    DestroyValue(value);
    return 1;
  case empty_choice:
    STATEMENT_ERROR(statement, "Assignment has CHOICE of an empty set");
    DestroyValue(value);
    return 1;
  case empty_intersection:
    STATEMENT_ERROR(statement,
      "Assignment has an empty INTERSECTION() construct which is undefined");
    DestroyValue(value);
    return 1;
  case type_conflict:
    STATEMENT_ERROR(statement,
         "Assignment right hand side contains a type conflict");
    DestroyValue(value);
    return 1;
  default:
    STATEMENT_ERROR(statement, "Assignment contains strange error");
    DestroyValue(value);
    return 1;
  }
}


static void ReAssignmentError(CONST char *str, struct Statement *statement){
  char *msg = ASC_NEW_ARRAY(char,strlen(REASSIGN_MESG1)+strlen(REASSIGN_MESG2)+strlen(str)+1);
  strcpy(msg,REASSIGN_MESG1);
  strcat(msg,str);
  strcat(msg,REASSIGN_MESG2);
  STATEMENT_ERROR(statement,msg);
  ascfree(msg);
}

static char *AssignmentDimenLabel(struct Statement *statement, struct Instance *inst){
  char *name, *label;
  const char *typename;

  name = NULL;
  if(statement != NULL && (StatementType(statement) == ASGN || StatementType(statement) == CASGN)){
    name = WriteNameString(AssignStatVar(statement));
  }
  if(name == NULL && inst != NULL){
    name = WriteInstanceNameString(inst,NULL);
  }
  if(name == NULL){
    return ASC_STRDUP("LHS");
  }

  typename = NULL;
  if(inst != NULL && InstanceTypeDesc(inst) != NULL && GetName(InstanceTypeDesc(inst)) != NULL){
    typename = SCP(GetName(InstanceTypeDesc(inst)));
  }
  if(typename == NULL){
    return name;
  }

  label = ASC_NEW_ARRAY(char,strlen(name) + strlen(typename) + 11);
  sprintf(label,"%s (IS_A %s)",name,typename);
  ascfree(name);
  return label;
}


/**
	returns 1 if ok, 0 if unhappy.
	for any given statement, once unhappy = always unhappy.
*/
static int AssignStructuralValue(struct Instance *inst
	,struct value_t value,struct Statement *statement
){
  switch(InstanceKind(inst)){
  case MODEL_INST:
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
  case REL_INST:
  case LREL_INST:
    STATEMENT_ERROR(statement, "Arg!  Attempt to assign to a non-scalar");
    return 0;
  case REAL_ATOM_INST:
  case REAL_INST:
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_INST:
  case INTEGER_ATOM_INST:
  case INTEGER_INST:
  case SYMBOL_ATOM_INST:
  case SYMBOL_INST:
    STATEMENT_ERROR(statement, "Assignment to non-constant LHS ignored");
    return 0;
  case REAL_CONSTANT_INST:
    switch(ValueKind(value)){
    case real_value:
      if ( AtomAssigned(inst) &&
           ( RealValue(value) != RealAtomValue(inst) ||
             !SameDimen(RealValueDimensions(value),RealAtomDims(inst)) )
         ) {
        ReAssignmentError(SCP(GetBaseTypeName(real_constant_type)),statement);
        return 0;
      }else{
        if (!AtomAssigned(inst)) {
          if ( !IsWild(RealAtomDims(inst)) &&
               !SameDimen(RealValueDimensions(value),RealAtomDims(inst)) ) {
            char *lhslabel;
            STATEMENT_ERROR(statement, "Dimensionally inconsistent assignment");
            lhslabel = AssignmentDimenLabel(statement,inst);
            PrintDimenMessage("Mismatched dimensions"
              ,lhslabel,RealAtomDims(inst)
              ,"RHS term",RealValueDimensions(value)
            );
            ascfree(lhslabel);
            return 0;
          }else{
      	    if (IsWild(RealAtomDims(inst))) {
              SetRealAtomDims(inst,RealValueDimensions(value));
            }
            SetRealAtomValue(inst,RealValue(value),0);
          }
        }
      }
      /* case of same value,dimen reassigned is silently ignored */
      return 1;
    case integer_value:
      if ( AtomAssigned(inst) &&
           ( (double)IntegerValue(value) != RealAtomValue(inst) ||
             !SameDimen(Dimensionless(),RealAtomDims(inst)) )
         ) {
        ReAssignmentError(SCP(GetBaseTypeName(real_constant_type)),
                          statement);
        return 0;
      }else{
        if (!AtomAssigned(inst)) {
          if ( !IsWild(RealAtomDims(inst)) &&
               !SameDimen(Dimensionless(),RealAtomDims(inst)) ) {
            char *lhslabel;
            STATEMENT_ERROR(statement, "Dimensionally inconsistent assignment");
            lhslabel = AssignmentDimenLabel(statement,inst);
            PrintDimenMessage("Mismatched dimensions"
              ,lhslabel,RealAtomDims(inst)
              ,"RHS term",Dimensionless()
            );
            ascfree(lhslabel);
            return 0;
          }else{
      	    if (IsWild(RealAtomDims(inst))) {
              SetRealAtomDims(inst,Dimensionless());
            }
            SetRealAtomValue(inst,(double)IntegerValue(value),0);
          }
        }
      }
      /* case of same value,dimen reassigned is silently ignored */
      return 1;
    default:
      STATEMENT_ERROR(statement,
           "Attempt to assign non-real value to a real instance");
    }
    return 0;
  case BOOLEAN_CONSTANT_INST:
    if (ValueKind(value)!=boolean_value){
      STATEMENT_ERROR(statement,
      "Attempt to assign a non-boolean value to a boolean instance");
      return 0;
    }else{
      if ( AtomAssigned(inst) &&
           BooleanValue(value) != GetBooleanAtomValue(inst) ) {
        ReAssignmentError(SCP(GetBaseTypeName(boolean_constant_type)),
                          statement);
        return 0;
      }else{
        if (!AtomAssigned(inst)) {
          SetBooleanAtomValue(inst,BooleanValue(value),0);
        }
      }
    }
    return 1;
  case INTEGER_CONSTANT_INST:
    switch(ValueKind(value)){
    case integer_value:
      if (AtomAssigned(inst)
          && (GetIntegerAtomValue(inst)!=IntegerValue(value))) {
        ReAssignmentError(SCP(GetBaseTypeName(integer_constant_type)),
                          statement);
        return 0;
      }else{
        if (!AtomAssigned(inst)) {
          SetIntegerAtomValue(inst,IntegerValue(value),0);
        }
      }
      return 1;
    case real_value: /* case which is parser artifact: real, wild 0 */
      if ( RealValue(value)==0.0 && IsWild(RealValueDimensions(value)) ) {
        if (!AtomAssigned(inst)) {
          SetIntegerAtomValue(inst,(long)0,0);
        }else{
          if (AtomAssigned(inst) && (GetIntegerAtomValue(inst)!=0)) {
            ReAssignmentError(SCP(GetBaseTypeName(integer_constant_type)),
                              statement);
            return 0;
          }
        }
        return 1;
      }
      /* intended to fall through to default if not wild real or not 0 */
    default:
      STATEMENT_ERROR(statement,
           "Attempt to assign a non-integer value to an integer instance");
    }
    return 0;
  case SET_ATOM_INST:
  case SET_INST:
    if (ValueKind(value)==set_value){
      if (AtomAssigned(inst)&&
          !SetsEqual(SetValue(value),SetAtomList(inst))) {
        ReAssignmentError(SCP(GetBaseTypeName(set_type)),
                          statement);
        return 0;
      }else{
        if(!AtomAssigned(inst)) {
          struct set_t *cslist;
          cslist = CopySet(SetValue(value));
          if (!AssignSetAtomList(inst,cslist)) {
            DestroySet(cslist);
            return 0;
          }
        }
        /* quietly ignore benign reassignment */
      }
      return 1;
    }else{
      STATEMENT_ERROR(statement,
           "Attempt to assign a non-set value to a set instance");
      return 0;
    }
  case SYMBOL_CONSTANT_INST:
    if (ValueKind(value)==symbol_value){
      asc_assert(AscFindSymbol(SymbolValue(value))!=NULL);
      if (AtomAssigned(inst) &&
          (SymbolValue(value) != GetSymbolAtomValue(inst))) {
        ReAssignmentError(SCP(GetBaseTypeName(symbol_constant_type)),
                          statement);
        return 0;
      }else{
        if (!AtomAssigned(inst)) {
          SetSymbolAtomValue(inst,SymbolValue(value));
        }
      }
      return 1;
    }else{
      STATEMENT_ERROR(statement,
              "Attempt to assign a non-symbol value to a symbol instance");
    }
    return 0;
  default:
    STATEMENT_ERROR(statement, "Error: Unknown value type");
    return 0;
  }
}

struct table_domain_t {
  struct set_t *set;
  enum set_kind kind;
  unsigned long len;
};

struct dataset_column_t {
  symchar *name;
  char *units; /* optional units string */
};

struct dataset_row_t {
  char **cells;
  char *storage;
};

struct dataset_table_t {
  unsigned ncols;
  struct dataset_column_t *cols;
  unsigned nrows;
  struct dataset_row_t *rows;
};

struct dataset_cache_entry_t {
  char *path;
  struct dataset_table_t *table;
  struct dataset_cache_entry_t *next;
};

static struct dataset_cache_entry_t *g_dataset_cache = NULL;

static void TableDomainDestroy(struct table_domain_t *domain){
  if (domain->set != NULL) {
    DestroySet(domain->set);
    domain->set = NULL;
  }
  domain->kind = empty_set;
  domain->len = 0;
}

static void DatasetTableDestroy(struct dataset_table_t *table)
{
  unsigned r;
  unsigned c;
  if (table == NULL) {
    return;
  }
  if (table->cols != NULL) {
    for (c = 0; c < table->ncols; ++c) {
      if (table->cols[c].units != NULL) {
        ascfree(table->cols[c].units);
      }
    }
    ascfree(table->cols);
  }
  if (table->rows != NULL) {
    for (r = 0; r < table->nrows; ++r) {
      if (table->rows[r].storage != NULL) {
        ascfree(table->rows[r].storage);
      }
      if (table->rows[r].cells != NULL) {
        ascfree(table->rows[r].cells);
      }
    }
    ascfree(table->rows);
  }
  ascfree(table);
}

static void DatasetCacheClear(void)
{
  struct dataset_cache_entry_t *entry = g_dataset_cache;
  while (entry != NULL) {
    struct dataset_cache_entry_t *next = entry->next;
    if (entry->path != NULL) {
      ascfree(entry->path);
    }
    DatasetTableDestroy(entry->table);
    ascfree(entry);
    entry = next;
  }
  g_dataset_cache = NULL;
}

static struct dataset_table_t *DatasetCacheGet(const char *path)
{
  struct dataset_cache_entry_t *entry;
  if (path == NULL) {
    return NULL;
  }
  for (entry = g_dataset_cache; entry != NULL; entry = entry->next) {
    if (entry->path != NULL && strcmp(entry->path,path) == 0) {
      return entry->table;
    }
  }
  return NULL;
}

static void DatasetCacheInsert(const char *path, struct dataset_table_t *table)
{
  struct dataset_cache_entry_t *entry;
  if (path == NULL || table == NULL) {
    return;
  }
  entry = ASC_NEW(struct dataset_cache_entry_t);
  entry->path = ASC_STRDUP(path);
  entry->table = table;
  entry->next = g_dataset_cache;
  g_dataset_cache = entry;
}

static int TableInitDomainFromSet(CONST struct set_t *set,
                                  struct Statement *statement,
                                  struct table_domain_t *domain)
{
  domain->set = CopySet(set);
  if (domain->set == NULL) {
    STATEMENT_ERROR(statement,"TABLE index set is unavailable");
    return 0;
  }
  domain->kind = SetKind(domain->set);
  if (domain->kind != integer_set && domain->kind != string_set && domain->kind != empty_set) {
    STATEMENT_ERROR(statement,"TABLE index set type is unsupported");
    TableDomainDestroy(domain);
    return 0;
  }
  domain->len = Cardinality(domain->set);
  return 1;
}

static int TableEvaluateDomains(struct Instance *inst,
                                CONST struct Set *set_expr,
                                struct Statement *statement,
                                struct table_domain_t *domains,
                                unsigned max_domains,
                                unsigned *num_domains)
{
  struct value_t value, ordered;
  struct gl_list_t *list;
  struct value_t *vptr;
  int saved_list_mode;
  unsigned long c,len;
  int all_sets = 0;
  IVAL(value);
  IVAL(ordered);
  *num_domains = 0;

  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateSet(set_expr,InstanceEvaluateName);
  SetEvaluationContext(NULL);

  switch (ValueKind(value)) {
  case set_value:
    if (max_domains < 1) {
      STATEMENT_ERROR(statement,"Too many TABLE indices");
      DestroyValue(&value);
      return 0;
    }
    if (!TableInitDomainFromSet(SetValue(value),statement,&domains[0])) {
      DestroyValue(&value);
      return 0;
    }
    *num_domains = 1;
    break;
  case list_value:
    list = value.u.lvalues;
    len = (list != NULL) ? gl_length(list) : 0;
    all_sets = (len > 0);
    for (c = 1; c <= len; ++c) {
      vptr = (struct value_t *)gl_fetch(list,c);
      if (vptr == NULL || ValueKind(*vptr) != set_value) {
        all_sets = 0;
        break;
      }
    }
    if (all_sets) {
      if (len > max_domains) {
        STATEMENT_ERROR(statement,"Too many TABLE indices");
        DestroyValue(&value);
        return 0;
      }
      for (c = 1; c <= len; ++c) {
        vptr = (struct value_t *)gl_fetch(list,c);
        if (!TableInitDomainFromSet(SetValue(*vptr),statement,&domains[c-1])) {
          unsigned long d;
          for (d = 0; d < c - 1; ++d) {
            TableDomainDestroy(&domains[d]);
          }
          DestroyValue(&value);
          return 0;
        }
      }
      *num_domains = (unsigned)len;
      break;
    }
    if (max_domains < 1) {
      STATEMENT_ERROR(statement,"Too many TABLE indices");
      DestroyValue(&value);
      return 0;
    }
    saved_list_mode = ListMode;
    ListMode = 1;
    ordered = CreateOrderedSetFromList(value);
    ListMode = saved_list_mode;
    if (ValueKind(ordered) != set_value) {
      STATEMENT_ERROR(statement,"TABLE index set cannot be converted from list");
      DestroyValue(&ordered);
      DestroyValue(&value);
      return 0;
    }
    if (!TableInitDomainFromSet(SetValue(ordered),statement,&domains[0])) {
      DestroyValue(&ordered);
      DestroyValue(&value);
      return 0;
    }
    *num_domains = 1;
    DestroyValue(&ordered);
    break;
  case error_value:
    STATEMENT_ERROR(statement,"TABLE index set could not be evaluated");
    DestroyValue(&value);
    return 0;
  default:
    STATEMENT_ERROR(statement,"TABLE index expression must evaluate to a set");
    DestroyValue(&value);
    return 0;
  }
  DestroyValue(&value);
  return 1;
}

static int TableParseNumberToken(CONST char *tok,
                                 int sign,
                                 int *is_int,
                                 long *ival,
                                 double *rval)
{
  char *end;
  long ltmp;
  double dtmp;
  errno = 0;
  ltmp = strtol(tok,&end,10);
  if (end != NULL && *end == '\0' && errno != ERANGE) {
    *is_int = 1;
    *ival = sign * ltmp;
    *rval = (double)(*ival);
    return 1;
  }
  errno = 0;
  dtmp = strtod(tok,&end);
  if (end != NULL && *end == '\0' && errno != ERANGE) {
    *is_int = 0;
    *rval = (sign < 0) ? -dtmp : dtmp;
    *ival = (long)(*rval);
    return 1;
  }
  return 0;
}

struct table_cell_value_t {
  int is_int;
  long ival;
  double rval;
};

struct scalar_units_runtime_t {
  int has_units;
  double conv;
  CONST dim_type *dims;
};

struct table_domain_ref_t {
  CONST struct Expr *expr;
  CONST struct Name *set_name;
};

static int TableAssignCell(struct Instance *work,
                           struct Statement *statement,
                           CONST struct table_domain_t *domains,
                           unsigned ndims,
                           CONST unsigned long *positions,
                           CONST struct scalar_units_runtime_t *units_runtime,
                           int is_int,
                           long ival,
                           double rval);
static int TableAssignCellMaybeWait(struct Instance *work,
                                    struct Statement *statement,
                                    CONST struct table_domain_t *domains,
                                    unsigned ndims,
                                    CONST unsigned long *positions,
                                    CONST struct scalar_units_runtime_t *units_runtime,
                                    int is_int,
                                    long ival,
                                    double rval);

static int TableTokenIsDelimiter(CONST char *tok)
{
  return (strcmp(tok,",") == 0 || strcmp(tok,";") == 0);
}

static int TableTokenIsSign(CONST char *tok)
{
  return (strcmp(tok,"+") == 0 || strcmp(tok,"-") == 0);
}

static int TableTokenIsPunctuation(CONST char *tok)
{
  return TableTokenIsDelimiter(tok)
      || TableTokenIsSign(tok)
      || strcmp(tok,":") == 0
      || strcmp(tok,"=") == 0;
}

static int TableTokenIsQuotedSymbol(CONST char *tok)
{
  size_t n;
  if (tok == NULL) {
    return 0;
  }
  n = strlen(tok);
  return (n >= 2 && tok[0] == '\'' && tok[n - 1] == '\'');
}

static int TableParseIntegerToken(CONST char *tok, long *ival)
{
  char *end = NULL;
  long v;
  if (tok == NULL) {
    return 0;
  }
  errno = 0;
  v = strtol(tok,&end,10);
  if (end == NULL || *end != '\0' || errno == ERANGE) {
    return 0;
  }
  if (ival != NULL) {
    *ival = v;
  }
  return 1;
}

static int TableParseSymbolToken(CONST char *tok, symchar **sym)
{
  size_t n;
  if (tok == NULL || sym == NULL) {
    return 0;
  }
  n = strlen(tok);
  if (n == 0) {
    return 0;
  }
  if (TableTokenIsQuotedSymbol(tok)) {
    /* AddSymbolL requires a terminated string, not a substring span. */
    char *label = ASC_NEW_ARRAY(char,n - 1);
    memcpy(label,tok + 1,n - 2);
    label[n - 2] = '\0';
    *sym = AddSymbolL(label,n - 2);
    ascfree(label);
  } else {
    *sym = AddSymbol(tok);
  }
  return (*sym != NULL);
}

static int DatasetParseRealToken(CONST char *tok, double *rval)
{
  char *end = NULL;
  double v;
  if (tok == NULL) {
    return 0;
  }
  errno = 0;
  v = strtod(tok,&end);
  if (end == NULL || *end != '\0' || errno == ERANGE) {
    return 0;
  }
  if (rval != NULL) {
    *rval = v;
  }
  return 1;
}

static int DatasetParseBooleanToken(CONST char *tok, int *bval)
{
  if (tok == NULL || bval == NULL) {
    return 0;
  }
  if (strcasecmp(tok,"TRUE") == 0) {
    *bval = 1;
    return 1;
  }
  if (strcasecmp(tok,"FALSE") == 0) {
    *bval = 0;
    return 1;
  }
  return 0;
}

static int ResolveScalarUnits(CONST char *units,
                              struct Statement *statement,
                              CONST char *kind,
                              struct scalar_units_runtime_t *out)
{
  unsigned long pos;
  int error_code;
  CONST struct Units *u;

  if (out == NULL) {
    return 0;
  }
  out->has_units = 0;
  out->conv = 1.0;
  out->dims = Dimensionless();

  if (units == NULL) {
    return 1;
  }

  u = FindOrDefineUnits(units,&pos,&error_code);
  if (u == NULL) {
    if (strcmp(kind,"TABLE") == 0) {
      STATEMENT_ERROR(statement,"TABLE units are invalid");
    } else {
      STATEMENT_ERROR(statement,"DATASET units are invalid");
    }
    return 0;
  }

  out->has_units = 1;
  out->conv = UnitsConvFactor(u);
  out->dims = UnitsDimensions(u);
  return 1;
}

static int AssignNumericToConstantInstance(struct Instance *inst,
                                           struct Statement *statement,
                                           int is_int,
                                           long ival,
                                           double rval,
                                           CONST struct scalar_units_runtime_t *units_runtime,
                                           CONST dim_type *default_real_dims,
                                           CONST char *kind)
{
  struct value_t value;
  int ok;

  if (inst == NULL) {
    if (strcmp(kind,"TABLE") == 0) {
      STATEMENT_ERROR(statement,"TABLE assignment target instance is NULL");
    } else {
      STATEMENT_ERROR(statement,"DATASET assignment target instance is NULL");
    }
    return 0;
  }

  switch (InstanceKind(inst)) {
  case REAL_CONSTANT_INST:
    if (units_runtime != NULL && units_runtime->has_units) {
      value = CreateRealValue((is_int ? (double)ival : rval) * units_runtime->conv,
                              units_runtime->dims,1);
    } else if (is_int) {
      value = CreateIntegerValue(ival,1);
    } else {
      value = CreateRealValue(rval,
                              default_real_dims != NULL ? default_real_dims : Dimensionless(),
                              1);
    }
    ok = AssignStructuralValue(inst,value,statement);
    DestroyValue(&value);
    return ok;
  case INTEGER_CONSTANT_INST:
    if (units_runtime != NULL && units_runtime->has_units) {
      if (strcmp(kind,"TABLE") == 0) {
        STATEMENT_ERROR(statement,"TABLE units are not allowed for integer values");
      } else {
        STATEMENT_ERROR(statement,"DATASET units are not allowed for integer values");
      }
      return 0;
    }
    if (!is_int) {
      if (strcmp(kind,"TABLE") == 0) {
        STATEMENT_ERROR(statement,"TABLE value is not a valid integer");
      } else {
        STATEMENT_ERROR(statement,"DATASET value is not a valid integer");
      }
      return 0;
    }
    value = CreateIntegerValue(ival,1);
    ok = AssignStructuralValue(inst,value,statement);
    DestroyValue(&value);
    return ok;
  default:
    if (strcmp(kind,"TABLE") == 0) {
      STATEMENT_ERROR(statement,"TABLE assignment target is not a constant");
    } else {
      STATEMENT_ERROR(statement,"DATASET assignment target is not a constant");
    }
    return 0;
  }
}

static double DatasetNowSeconds(void)
{
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC,&ts) != 0) {
    return 0.0;
  }
  return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
}

enum dataset_stream_kind_t {
  DATASET_STREAM_PLAIN = 0,
  DATASET_STREAM_GZ,
  DATASET_STREAM_XZ
};

struct dataset_stream_t {
  enum dataset_stream_kind_t kind;
  FILE *file;
#ifdef ASC_WITH_ZLIB
  gzFile gz;
#endif
#ifdef ASC_WITH_LZMA
  lzma_stream xz;
  int xz_initialised;
  int xz_input_eof;
  int xz_stream_end;
  unsigned char xz_inbuf[8192];
  unsigned char xz_outbuf[8192];
  size_t xz_outpos;
  size_t xz_outlen;
#endif
};

static int DatasetPathHasSuffix(CONST char *path, CONST char *suffix)
{
  size_t plen;
  size_t slen;
  if (path == NULL || suffix == NULL) {
    return 0;
  }
  plen = strlen(path);
  slen = strlen(suffix);
  if (plen < slen) {
    return 0;
  }
  return strcasecmp(path + plen - slen,suffix) == 0;
}

static void DatasetStreamInit(struct dataset_stream_t *stream)
{
  if (stream == NULL) {
    return;
  }
  memset(stream,0,sizeof(*stream));
  stream->kind = DATASET_STREAM_PLAIN;
}

static int DatasetResolvePath(CONST char *filename,
                              char **resolved_path,
                              struct Statement *statement)
{
  struct FilePath *fp;
  struct FilePath **sp;
  struct FilePath **sp_root;
  struct FilePath *found = NULL;
  ospath_stat_t buf;
  char *path;

  if (resolved_path != NULL) {
    *resolved_path = NULL;
  }
  if (filename == NULL || filename[0] == '\0') {
    STATEMENT_ERROR(statement,"DATASET filename is empty");
    return 0;
  }

  fp = ospath_new(filename);
  if (fp == NULL) {
    STATEMENT_ERROR(statement,"DATASET filename is invalid");
    return 0;
  }

  if (ospath_stat(fp,&buf) == 0) {
    if (resolved_path != NULL) {
      *resolved_path = ospath_str(fp);
    }
    ospath_free(fp);
    return 1;
  }

  path = Asc_GetEnv(ASC_ENV_LIBRARY);
  if (path == NULL) {
    STATEMENT_ERROR(statement,"No search path available for DATASET file");
    ospath_free(fp);
    return 0;
  }

  sp = ospath_searchpath_new(path);
  ascfree(path);
  if (sp == NULL) {
    STATEMENT_ERROR(statement,"Unable to parse DATASET search path");
    ospath_free(fp);
    return 0;
  }
  sp_root = sp;

  for (; *sp != NULL; ++sp) {
    struct FilePath *candidate = ospath_concat(*sp,fp);
    if (candidate == NULL) {
      continue;
    }
    if (ospath_stat(candidate,&buf) == 0) {
      found = candidate;
      break;
    }
    ospath_free(candidate);
  }

  ospath_free(fp);
  ospath_searchpath_free(sp_root);
  if (found != NULL && resolved_path != NULL) {
    *resolved_path = ospath_str(found);
  }
  if (found != NULL) {
    ospath_free(found);
  }
  if (found == NULL) {
    STATEMENT_ERROR(statement,"DATASET file not found in search path");
    return 0;
  }
  return 1;
}

static int DatasetStreamOpen(CONST char *filename,
                             char **resolved_path,
                             struct Statement *statement,
                             struct dataset_stream_t *stream)
{
  char *path = NULL;

  if (stream == NULL) {
    STATEMENT_ERROR(statement,"Internal DATASET stream error");
    return 0;
  }
  DatasetStreamInit(stream);

  if (!DatasetResolvePath(filename,&path,statement)) {
    return 0;
  }

  if (DatasetPathHasSuffix(path,".gz")) {
#ifdef ASC_WITH_ZLIB
    stream->gz = gzopen(path,"rb");
    if (stream->gz == NULL) {
      STATEMENT_ERROR(statement,"Unable to open gzip-compressed DATASET file");
      ascfree(path);
      return 0;
    }
    stream->kind = DATASET_STREAM_GZ;
#else
    STATEMENT_ERROR(statement,"DATASET .gz input requires zlib support at build time");
    ascfree(path);
    return 0;
#endif
  } else if (DatasetPathHasSuffix(path,".xz")) {
#ifdef ASC_WITH_LZMA
    stream->file = fopen(path,"rb");
    if (stream->file == NULL) {
      STATEMENT_ERROR(statement,"Unable to open xz-compressed DATASET file");
      ascfree(path);
      return 0;
    }
    stream->xz = (lzma_stream)LZMA_STREAM_INIT;
    if (lzma_stream_decoder(&stream->xz,UINT64_MAX,LZMA_CONCATENATED) != LZMA_OK) {
      STATEMENT_ERROR(statement,"Unable to initialise xz decompressor for DATASET file");
      fclose(stream->file);
      stream->file = NULL;
      ascfree(path);
      return 0;
    }
    stream->xz_initialised = 1;
    stream->kind = DATASET_STREAM_XZ;
#else
    STATEMENT_ERROR(statement,"DATASET .xz input requires liblzma support at build time");
    ascfree(path);
    return 0;
#endif
  } else {
    stream->file = fopen(path,"rb");
    if (stream->file == NULL) {
      STATEMENT_ERROR(statement,"Unable to open DATASET file");
      ascfree(path);
      return 0;
    }
    stream->kind = DATASET_STREAM_PLAIN;
  }

  if (resolved_path != NULL) {
    *resolved_path = path;
  } else {
    ascfree(path);
  }
  return 1;
}

static void DatasetStreamClose(struct dataset_stream_t *stream)
{
  if (stream == NULL) {
    return;
  }
#ifdef ASC_WITH_ZLIB
  if (stream->kind == DATASET_STREAM_GZ && stream->gz != NULL) {
    gzclose(stream->gz);
    stream->gz = NULL;
  }
#endif
#ifdef ASC_WITH_LZMA
  if (stream->kind == DATASET_STREAM_XZ && stream->xz_initialised) {
    lzma_end(&stream->xz);
    stream->xz_initialised = 0;
  }
#endif
  if (stream->file != NULL) {
    fclose(stream->file);
    stream->file = NULL;
  }
  stream->kind = DATASET_STREAM_PLAIN;
}

#ifdef ASC_WITH_LZMA
static int DatasetStreamRefillXZ(struct dataset_stream_t *stream,
                                 struct Statement *statement)
{
  while (1) {
    lzma_ret ret;
    lzma_action action = stream->xz_input_eof ? LZMA_FINISH : LZMA_RUN;

    if (stream->xz_outpos < stream->xz_outlen) {
      return 1;
    }
    if (stream->xz_stream_end) {
      return 0;
    }

    if (stream->xz.avail_in == 0 && !stream->xz_input_eof) {
      size_t nread = fread(stream->xz_inbuf,1,sizeof(stream->xz_inbuf),stream->file);
      if (nread == 0) {
        if (ferror(stream->file)) {
          STATEMENT_ERROR(statement,"Error reading compressed DATASET stream");
          return -1;
        }
        stream->xz_input_eof = 1;
        action = LZMA_FINISH;
      } else {
        stream->xz.next_in = stream->xz_inbuf;
        stream->xz.avail_in = nread;
      }
    }

    stream->xz.next_out = stream->xz_outbuf;
    stream->xz.avail_out = sizeof(stream->xz_outbuf);
    ret = lzma_code(&stream->xz,action);
    stream->xz_outpos = 0;
    stream->xz_outlen = sizeof(stream->xz_outbuf) - stream->xz.avail_out;

    if (ret == LZMA_STREAM_END) {
      stream->xz_stream_end = 1;
      if (stream->xz_outlen > 0) {
        return 1;
      }
      return 0;
    }
    if (ret != LZMA_OK) {
      STATEMENT_ERROR(statement,"DATASET xz decompression failed");
      return -1;
    }
    if (stream->xz_outlen > 0) {
      return 1;
    }
    if (stream->xz_input_eof && stream->xz.avail_in == 0) {
      return 0;
    }
  }
}
#endif

static int DatasetStreamGetChar(struct dataset_stream_t *stream,
                                int *outch,
                                struct Statement *statement)
{
  if (stream == NULL || outch == NULL) {
    return -1;
  }

  switch (stream->kind) {
  case DATASET_STREAM_PLAIN:
    {
      int ch = fgetc(stream->file);
      if (ch == EOF) {
        if (ferror(stream->file)) {
          STATEMENT_ERROR(statement,"Error reading DATASET file");
          return -1;
        }
        return 0;
      }
      *outch = ch;
      return 1;
    }
#ifdef ASC_WITH_ZLIB
  case DATASET_STREAM_GZ:
    {
      int ch = gzgetc(stream->gz);
      if (ch == -1) {
        if (gzeof(stream->gz)) {
          return 0;
        } else {
          STATEMENT_ERROR(statement,"Error reading gzip DATASET stream");
          return -1;
        }
      }
      *outch = ch;
      return 1;
    }
#endif
#ifdef ASC_WITH_LZMA
  case DATASET_STREAM_XZ:
    {
      int refill = DatasetStreamRefillXZ(stream,statement);
      if (refill <= 0) {
        return refill;
      }
      *outch = (int)stream->xz_outbuf[stream->xz_outpos++];
      return 1;
    }
#endif
  default:
    STATEMENT_ERROR(statement,"Unsupported DATASET stream type");
    return -1;
  }
}

static int DatasetReadLine(struct dataset_stream_t *stream,
                           Asc_DString *line,
                           unsigned long *line_no,
                           struct Statement *statement)
{
  int got = 0;
  int ch = 0;
  if (line == NULL || stream == NULL) {
    return 0;
  }
  Asc_DStringTrunc(line,0);
  while (1) {
    int rc = DatasetStreamGetChar(stream,&ch,statement);
    if (rc < 0) {
      return -1;
    }
    if (rc == 0) {
      break;
    }
    {
      char cbuf[2];
      got = 1;
      cbuf[0] = (char)ch;
      cbuf[1] = '\0';
      Asc_DStringAppend(line,cbuf,1);
      if (cbuf[0] == '\n') {
        break;
      }
    }
  }
  if (got && line_no != NULL) {
    (*line_no)++;
  }
  return got;
}

static char *DatasetTrimCopy(CONST char *txt)
{
  CONST char *start;
  CONST char *end;
  size_t len;
  char *out;

  if (txt == NULL) {
    return NULL;
  }
  start = txt;
  while (*start != '\0' && isspace((unsigned char)*start)) {
    ++start;
  }
  end = start + strlen(start);
  while (end > start && isspace((unsigned char)end[-1])) {
    --end;
  }
  len = (size_t)(end - start);
  out = ASC_NEW_ARRAY(char,len + 1);
  if (len > 0) {
    memcpy(out,start,len);
  }
  out[len] = '\0';
  return out;
}

static int DatasetIsDimensionlessToken(CONST char *tok)
{
  return tok == NULL
      || tok[0] == '\0'
      || strcmp(tok,"-") == 0
      || strcmp(tok,"1") == 0
      || strcasecmp(tok,"dimensionless") == 0;
}

static int DatasetTokenizeLine(CONST char *line,
                               char ***tokens_out,
                               unsigned *count_out,
                               struct Statement *statement,
                               unsigned long line_no)
{
  Asc_DString token;
  char **tokens = NULL;
  unsigned count = 0;
  unsigned ti;
  int in_quote = 0;
  char quote_char = '\0';
  int saw_token = 0;
  size_t i;

  if (tokens_out == NULL || count_out == NULL || line == NULL) {
    return 0;
  }
  *tokens_out = NULL;
  *count_out = 0;
  (void)line_no;

  for (i = 0; line[i] != '\0'; ++i) {
    if (!isspace((unsigned char)line[i])) {
      if (line[i] == '#') {
        return 0;
      }
      break;
    }
  }
  if (line[i] == '\0') {
    return 0;
  }

  Asc_DStringInit(&token);

  for (i = 0; ; ++i) {
    int end = (line[i] == '\0');
    int delim = (!in_quote && !end && (line[i] == ',' || line[i] == ';'));
    int comment = 0;

    if (!in_quote && !end && line[i] == '#') {
      char *trimmed = DatasetTrimCopy(Asc_DStringValue(&token));
      if (trimmed == NULL || trimmed[0] == '\0') {
        if (trimmed != NULL) {
          ascfree(trimmed);
        }
        break;
      }
      ascfree(trimmed);
      comment = 1;
      end = 1;
    }

    if (!end && !delim && !comment) {
      char ch = line[i];
      if (!in_quote && (ch == '\'' || ch == '"')) {
        in_quote = 1;
        quote_char = ch;
        continue;
      }
      if (in_quote && ch == quote_char) {
        in_quote = 0;
        quote_char = '\0';
        continue;
      }
    }

    if (end || delim || comment) {
      char *trimmed = DatasetTrimCopy(Asc_DStringValue(&token));
      Asc_DStringTrunc(&token,0);
      if (trimmed == NULL) {
        STATEMENT_ERROR(statement,"Unable to allocate DATASET token buffer");
        for (ti = 0; ti < count; ++ti) {
          ascfree(tokens[ti]);
        }
        ascfree(tokens);
        Asc_DStringFree(&token);
        return -1;
      }
      if (trimmed[0] != '\0') {
        tokens = (char **)ascrealloc(tokens,sizeof(char *) * (count + 1));
        tokens[count++] = trimmed;
        saw_token = 1;
      } else {
        ascfree(trimmed);
        if (delim || (comment && saw_token)) {
          STATEMENT_ERROR(statement,"Empty DATASET field");
          for (ti = 0; ti < count; ++ti) {
            ascfree(tokens[ti]);
          }
          ascfree(tokens);
          Asc_DStringFree(&token);
          return -1;
        }
      }
      if (end || comment) {
        break;
      }
      continue;
    }

    {
      char chbuf[2] = {line[i],'\0'};
      Asc_DStringAppend(&token,chbuf,1);
    }
  }

  Asc_DStringFree(&token);
  if (in_quote) {
    STATEMENT_ERROR(statement,"Unterminated quote in DATASET row");
    for (i = 0; i < count; ++i) {
      ascfree(tokens[i]);
    }
    ascfree(tokens);
    return -1;
  }

  if (!saw_token) {
    return 0;
  }

  *tokens_out = tokens;
  *count_out = count;
  return 1;
}

static int DatasetExtractColumnUnitsSuffix(CONST char *tok, char **name, char **units)
{
  char closech;
  char opench;
  CONST char *openptr = NULL;
  CONST char *start;
  CONST char *end;
  CONST char *p;
  CONST char *ustart;
  CONST char *uend;
  CONST char *nstart;
  CONST char *nend;
  size_t nlen;
  size_t ulen;
  char *ncopy;
  char *ucopy;

  *name = NULL;
  *units = NULL;
  if (tok == NULL) {
    return 0;
  }

  start = tok;
  while (*start != '\0' && isspace((unsigned char)*start)) {
    ++start;
  }
  end = start + strlen(start);
  while (end > start && isspace((unsigned char)end[-1])) {
    --end;
  }
  if (end <= start) {
    return 0;
  }

  closech = end[-1];
  if (closech == '}') {
    opench = '{';
  } else if (closech == ']') {
    opench = '[';
  } else {
    return 0;
  }

  for (p = end - 1; p > start; --p) {
    if (p[-1] == opench) {
      openptr = p - 1;
      break;
    }
  }
  if (openptr == NULL) {
    return 0;
  }

  ustart = openptr + 1;
  uend = end - 1;
  while (ustart < uend && isspace((unsigned char)*ustart)) {
    ++ustart;
  }
  while (uend > ustart && isspace((unsigned char)uend[-1])) {
    --uend;
  }
  if (uend <= ustart) {
    return 0;
  }

  nstart = start;
  nend = openptr;
  while (nend > nstart && isspace((unsigned char)nend[-1])) {
    --nend;
  }
  if (nend > nstart && nend[-1] == '/') {
    --nend;
    while (nend > nstart && isspace((unsigned char)nend[-1])) {
      --nend;
    }
  }
  while (nstart < nend && isspace((unsigned char)*nstart)) {
    ++nstart;
  }
  if (nend <= nstart) {
    return 0;
  }

  nlen = (size_t)(nend - nstart);
  ulen = (size_t)(uend - ustart);
  ncopy = ASC_NEW_ARRAY(char,nlen + 1);
  ucopy = ASC_NEW_ARRAY(char,ulen + 1);
  memcpy(ncopy,nstart,nlen);
  ncopy[nlen] = '\0';
  memcpy(ucopy,ustart,ulen);
  ucopy[ulen] = '\0';

  *name = ncopy;
  *units = ucopy;
  return 1;
}

static int DatasetSplitColumnToken(CONST char *tok, symchar **name, char **units)
{
  char *name_copy = NULL;
  char *units_copy = NULL;
  char *trimmed = NULL;
  int ok = 0;

  if (tok == NULL || name == NULL || units == NULL) {
    return 0;
  }
  *name = NULL;
  *units = NULL;

  if (!DatasetExtractColumnUnitsSuffix(tok,&name_copy,&units_copy)) {
    trimmed = DatasetTrimCopy(tok);
    if (trimmed == NULL || trimmed[0] == '\0') {
      if (trimmed != NULL) {
        ascfree(trimmed);
      }
      return 0;
    }
    name_copy = trimmed;
    trimmed = NULL;
  }

  ok = TableParseSymbolToken(name_copy,name);
  ascfree(name_copy);
  if (!ok) {
    if (units_copy != NULL) {
      ascfree(units_copy);
    }
    return 0;
  }

  if (units_copy != NULL) {
    char *u = DatasetTrimCopy(units_copy);
    ascfree(units_copy);
    if (u != NULL && u[0] != '\0') {
      *units = u;
    } else if (u != NULL) {
      ascfree(u);
    }
  }
  return 1;
}

static int DatasetParseUnitsOnlyToken(CONST char *tok, char **units_out, int *has_units)
{
  char *trimmed;
  CONST char *start;
  CONST char *end;
  char *units = NULL;

  if (units_out == NULL || has_units == NULL || tok == NULL) {
    return 0;
  }
  *units_out = NULL;
  *has_units = 0;

  trimmed = DatasetTrimCopy(tok);
  if (trimmed == NULL) {
    return 0;
  }
  if (DatasetIsDimensionlessToken(trimmed)) {
    ascfree(trimmed);
    return 1;
  }

  start = trimmed;
  if (*start == '/') {
    ++start;
    while (*start != '\0' && isspace((unsigned char)*start)) {
      ++start;
    }
  }
  end = start + strlen(start);
  while (end > start && isspace((unsigned char)end[-1])) {
    --end;
  }
  if (end - start >= 2
      && ((start[0] == '{' && end[-1] == '}')
          || (start[0] == '[' && end[-1] == ']'))) {
    size_t len;
    ++start;
    --end;
    while (end > start && isspace((unsigned char)end[-1])) {
      --end;
    }
    while (start < end && isspace((unsigned char)*start)) {
      ++start;
    }
    len = (size_t)(end - start);
    units = ASC_NEW_ARRAY(char,len + 1);
    if (len > 0) {
      memcpy(units,start,len);
    }
    units[len] = '\0';
    if (!DatasetIsDimensionlessToken(units)) {
      *units_out = units;
      *has_units = 1;
      ascfree(trimmed);
      return 1;
    }
    ascfree(units);
    ascfree(trimmed);
    return 1;
  }

  ascfree(trimmed);
  return 0;
}

static int DatasetMaybeApplyUnitsRow(struct dataset_table_t *table,
                                     char **tokens,
                                     unsigned ntokens,
                                     struct Statement *statement,
                                     int *consumed)
{
  char **parsed = NULL;
  unsigned i;
  int saw_units = 0;

  if (consumed != NULL) {
    *consumed = 0;
  }
  if (table == NULL || tokens == NULL || ntokens != table->ncols || table->nrows != 0) {
    return 1;
  }

  parsed = ASC_NEW_ARRAY_CLEAR(char *,ntokens);
  if (parsed == NULL) {
    STATEMENT_ERROR(statement,"Unable to allocate DATASET units-row buffer");
    return 0;
  }
  for (i = 0; i < ntokens; ++i) {
    int has_units = 0;
    if (!DatasetParseUnitsOnlyToken(tokens[i],&parsed[i],&has_units)) {
      goto not_units_row;
    }
    if (has_units) {
      saw_units = 1;
    }
  }

  if (!saw_units) {
    goto not_units_row;
  }

  for (i = 0; i < ntokens; ++i) {
    if (parsed[i] == NULL) {
      continue;
    }
    if (table->cols[i].units != NULL && strcmp(table->cols[i].units,parsed[i]) != 0) {
      STATEMENT_ERROR(statement,"DATASET units row conflicts with column header units");
      goto fail;
    }
    if (table->cols[i].units == NULL) {
      table->cols[i].units = parsed[i];
      parsed[i] = NULL;
    }
  }

  for (i = 0; i < ntokens; ++i) {
    if (parsed[i] != NULL) {
      ascfree(parsed[i]);
    }
  }
  ascfree(parsed);
  if (consumed != NULL) {
    *consumed = 1;
  }
  return 1;

not_units_row:
  for (i = 0; i < ntokens; ++i) {
    if (parsed[i] != NULL) {
      ascfree(parsed[i]);
    }
  }
  ascfree(parsed);
  return 1;

fail:
  for (i = 0; i < ntokens; ++i) {
    if (parsed[i] != NULL) {
      ascfree(parsed[i]);
    }
  }
  ascfree(parsed);
  return 0;
}

static struct dataset_table_t *DatasetReadFile(CONST char *path,
                                               struct dataset_stream_t *stream,
                                               struct Statement *statement)
{
  Asc_DString line;
  unsigned long line_no = 0;
  int have_header = 0;
  struct dataset_table_t *table = NULL;
  (void)path;

  if (stream == NULL) {
    return NULL;
  }

  Asc_DStringInit(&line);
  while (1) {
    int line_result = DatasetReadLine(stream,&line,&line_no,statement);
    if (line_result < 0) {
      DatasetTableDestroy(table);
      Asc_DStringFree(&line);
      return NULL;
    }
    if (line_result == 0) {
      break;
    }
    char **tokens = NULL;
    unsigned ntokens = 0;
    int tok_result = DatasetTokenizeLine(Asc_DStringValue(&line),&tokens,&ntokens,statement,line_no);
    if (tok_result < 0) {
      DatasetTableDestroy(table);
      Asc_DStringFree(&line);
      return NULL;
    }
    if (tok_result == 0) {
      continue;
    }

    if (!have_header) {
      unsigned i;
      if (ntokens == 0) {
        continue;
      }
      table = ASC_NEW(struct dataset_table_t);
      table->ncols = ntokens;
      table->nrows = 0;
      table->rows = NULL;
      table->cols = ASC_NEW_ARRAY(struct dataset_column_t,ntokens);
      for (i = 0; i < ntokens; ++i) {
        symchar *colname = NULL;
        char *units = NULL;
        if (!DatasetSplitColumnToken(tokens[i],&colname,&units)) {
          STATEMENT_ERROR(statement,"Invalid DATASET column name");
          DatasetTableDestroy(table);
          table = NULL;
          break;
        }
        if (table != NULL) {
          unsigned j;
          for (j = 0; j < i; ++j) {
            if (table->cols[j].name == colname) {
              STATEMENT_ERROR(statement,"DATASET contains duplicate column names");
              DatasetTableDestroy(table);
              table = NULL;
              if (units != NULL) {
                ascfree(units);
              }
              break;
            }
          }
          if (table == NULL) {
            break;
          }
        }
        table->cols[i].name = colname;
        table->cols[i].units = units;
      }
      for (i = 0; i < ntokens; ++i) {
        ascfree(tokens[i]);
      }
      ascfree(tokens);
      if (table == NULL) {
        Asc_DStringFree(&line);
        return NULL;
      }
      have_header = 1;
      continue;
    }

    if (table == NULL) {
      unsigned i;
      for (i = 0; i < ntokens; ++i) {
        ascfree(tokens[i]);
      }
      ascfree(tokens);
      continue;
    }

    if (table->nrows == 0) {
      int consumed_units_row = 0;
      if (!DatasetMaybeApplyUnitsRow(table,tokens,ntokens,statement,&consumed_units_row)) {
        unsigned i;
        for (i = 0; i < ntokens; ++i) {
          ascfree(tokens[i]);
        }
        ascfree(tokens);
        DatasetTableDestroy(table);
        Asc_DStringFree(&line);
        return NULL;
      }
      if (consumed_units_row) {
        unsigned i;
        for (i = 0; i < ntokens; ++i) {
          ascfree(tokens[i]);
        }
        ascfree(tokens);
        continue;
      }
    }

    if (ntokens != table->ncols) {
      unsigned i;
      STATEMENT_ERROR(statement,"DATASET row has incorrect number of columns");
      for (i = 0; i < ntokens; ++i) {
        ascfree(tokens[i]);
      }
      ascfree(tokens);
      DatasetTableDestroy(table);
      Asc_DStringFree(&line);
      return NULL;
    }

    {
      struct dataset_row_t *row;
      char *dst;
      size_t storage_len = 0;
      unsigned i;

      table->rows = (struct dataset_row_t *)ascrealloc(table->rows,sizeof(struct dataset_row_t) * (table->nrows + 1));
      row = &table->rows[table->nrows];
      row->cells = ASC_NEW_ARRAY(char *,table->ncols);
      row->storage = NULL;
      for (i = 0; i < table->ncols; ++i) {
        storage_len += strlen(tokens[i]) + 1;
      }
      row->storage = ASC_NEW_ARRAY(char,storage_len > 0 ? storage_len : 1);
      dst = row->storage;
      for (i = 0; i < table->ncols; ++i) {
        size_t len = strlen(tokens[i]) + 1;
        memcpy(dst,tokens[i],len);
        row->cells[i] = dst;
        dst += len;
        ascfree(tokens[i]);
      }
    }
    ascfree(tokens);
    table->nrows++;
  }

  Asc_DStringFree(&line);

  if (!have_header) {
    STATEMENT_ERROR(statement,"DATASET file is missing header row");
    DatasetTableDestroy(table);
    return NULL;
  }
  if (table != NULL && table->nrows == 0) {
    STATEMENT_ERROR(statement,"DATASET file contains no data rows");
    DatasetTableDestroy(table);
    return NULL;
  }
  return table;
}

static int TableCollectDomainRefs(CONST struct Name *target,
                                  struct Statement *statement,
                                  struct table_domain_ref_t refs[2],
                                  unsigned *ndims)
{
  CONST struct Name *node;
  unsigned d = 0;
  for (node = target; node != NULL; node = NextName(node)) {
    CONST struct Set *setnode;
    if (NameId(node)) {
      continue;
    }
    for (setnode = NameSetPtr(node); setnode != NULL; setnode = NextSet(setnode)) {
      CONST struct Expr *expr;
      if (d >= 2) {
        STATEMENT_ERROR(statement,"TABLE currently supports at most 2 indices");
        return 0;
      }
      if (SetType(setnode)) {
        STATEMENT_ERROR(statement,"TABLE index ranges are not supported");
        return 0;
      }
      expr = GetSingleExpr(setnode);
      if (expr == NULL) {
        STATEMENT_ERROR(statement,"TABLE index expression is invalid");
        return 0;
      }
      refs[d].expr = expr;
      refs[d].set_name = NULL;
      if (ExprType(expr) == e_var
          && ExprListLength(expr) == 1
          && SimpleNameIdPtr(ExprName(expr)) != NULL) {
        refs[d].set_name = ExprName(expr);
      }
      d++;
    }
  }
  *ndims = d;
  return 1;
}

static int TableEvaluateDomainExpr(struct Instance *work,
                                   CONST struct Expr *expr,
                                   struct Statement *statement,
                                   struct table_domain_t *domain,
                                   int *undefined)
{
  struct Set tmpset;
  struct value_t value, ordered;
  int saved_list_mode;
  IVAL(value);
  IVAL(ordered);
  *undefined = 0;

  tmpset.next = NULL;
  tmpset.range = 0;
  tmpset.val.e = (struct Expr *)expr;
  tmpset.ref_count = 1;

  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(work);
  value = EvaluateSet(&tmpset,InstanceEvaluateName);
  SetEvaluationContext(NULL);

  switch (ValueKind(value)) {
  case set_value:
    if (!TableInitDomainFromSet(SetValue(value),statement,domain)) {
      DestroyValue(&value);
      return 0;
    }
    DestroyValue(&value);
    return 1;
  case list_value:
    saved_list_mode = ListMode;
    ListMode = 1;
    ordered = CreateOrderedSetFromList(value);
    ListMode = saved_list_mode;
    DestroyValue(&value);
    if (ValueKind(ordered) != set_value) {
      STATEMENT_ERROR(statement,"TABLE index set cannot be converted from list");
      DestroyValue(&ordered);
      return 0;
    }
    if (!TableInitDomainFromSet(SetValue(ordered),statement,domain)) {
      DestroyValue(&ordered);
      return 0;
    }
    DestroyValue(&ordered);
    return 1;
  case error_value:
    if (ErrorValue(value) == name_unfound || ErrorValue(value) == undefined_value) {
      *undefined = 1;
      DestroyValue(&value);
      return 0;
    }
    STATEMENT_ERROR(statement,"TABLE index set could not be evaluated");
    DestroyValue(&value);
    return 0;
  default:
    STATEMENT_ERROR(statement,"TABLE index expression must evaluate to a set");
    DestroyValue(&value);
    return 0;
  }
}

static int TableAssignDomainSet(struct Instance *work,
                                CONST struct Name *set_name,
                                CONST struct set_t *setvalue,
                                struct Statement *statement)
{
  struct gl_list_t *instances;
  struct Instance *setinst;
  struct value_t value;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  int ok;

  instances = FindInstances(work,(struct Name *)set_name,&err);
  if (instances == NULL || gl_length(instances) != 1) {
    if (instances != NULL) {
      gl_destroy(instances);
    }
    STATEMENT_ERROR(statement,"TABLE inferred index set name could not be resolved uniquely");
    return 0;
  }
  setinst = (struct Instance *)gl_fetch(instances,1);
  gl_destroy(instances);

  value = CreateSetValue(CopySet(setvalue));
  ok = AssignStructuralValue(setinst,value,statement);
  DestroyValue(&value);
  return ok;
}

static int TableInferDomainFromLabels(struct Instance *work,
                                      CONST struct Name *set_name,
                                      char **labels,
                                      unsigned long nlabels,
                                      struct Statement *statement,
                                      struct table_domain_t *domain)
{
  struct set_t *setvalue = NULL;
  unsigned long i;
  int all_int = 1;
  int ok = 0;

  if (set_name == NULL) {
    STATEMENT_ERROR(statement,"TABLE index set is undefined and cannot be inferred");
    return 0;
  }

  for (i = 0; i < nlabels; ++i) {
    long ival;
    if (labels[i] == NULL || TableTokenIsQuotedSymbol(labels[i]) || !TableParseIntegerToken(labels[i],&ival)) {
      all_int = 0;
      break;
    }
  }

  setvalue = CreateEmptySet();
  if (all_int) {
    for (i = 0; i < nlabels; ++i) {
      long ival;
      if (!TableParseIntegerToken(labels[i],&ival)) {
        STATEMENT_ERROR(statement,"TABLE inferred integer index contains invalid label");
        goto cleanup;
      }
      if (IntMember((asc_intptr_t)ival,setvalue)) {
        STATEMENT_ERROR(statement,"TABLE inferred integer index contains duplicate labels");
        goto cleanup;
      }
      InsertInteger(setvalue,(asc_intptr_t)ival);
    }
  } else {
    for (i = 0; i < nlabels; ++i) {
      symchar *sym;
      if (!TableParseSymbolToken(labels[i],&sym)) {
        STATEMENT_ERROR(statement,"TABLE inferred string index contains invalid label");
        goto cleanup;
      }
      if (StrMember(sym,setvalue)) {
        STATEMENT_ERROR(statement,"TABLE inferred string index contains duplicate labels");
        goto cleanup;
      }
      InsertString(setvalue,sym);
    }
  }

  if (!TableAssignDomainSet(work,set_name,setvalue,statement)) {
    goto cleanup;
  }
  if (!TableInitDomainFromSet(setvalue,statement,domain)) {
    goto cleanup;
  }
  ok = 1;

cleanup:
  if (setvalue != NULL) {
    DestroySet(setvalue);
  }
  return ok;
}

static int TableLabelToPosition(CONST struct table_domain_t *domain,
                                CONST char *tok,
                                unsigned long *pos,
                                struct Statement *statement,
                                int is_row_label)
{
  unsigned long i;
  const char *axis = is_row_label ? "row" : "column";
  if (domain->kind == integer_set) {
    long ival;
    if (!TableParseIntegerToken(tok,&ival)) {
      STATEMENT_ERROR(statement,
        is_row_label
          ? "TABLE row label is not a valid integer for first index set"
          : "TABLE column label is not a valid integer for second index set");
      return 0;
    }
    if (!IntMember((asc_intptr_t)ival,domain->set)) {
      STATEMENT_ERROR(statement,
        is_row_label
          ? "TABLE row label is not a member of first index set"
          : "TABLE column label is not a member of second index set");
      return 0;
    }
    for (i = 1; i <= domain->len; ++i) {
      if ((long)FetchIntMember(domain->set,i) == ival) {
        *pos = i;
        return 1;
      }
    }
  } else if (domain->kind == string_set) {
    symchar *sym;
    if (!TableParseSymbolToken(tok,&sym)) {
      STATEMENT_ERROR(statement,
        is_row_label
          ? "TABLE row label is not a valid symbol for first index set"
          : "TABLE column label is not a valid symbol for second index set");
      return 0;
    }
    if (!StrMember(sym,domain->set)) {
      STATEMENT_ERROR(statement,
        is_row_label
          ? "TABLE row label is not a member of first index set"
          : "TABLE column label is not a member of second index set");
      return 0;
    }
    for (i = 1; i <= domain->len; ++i) {
      if (FetchStrMember(domain->set,i) == sym) {
        *pos = i;
        return 1;
      }
    }
  } else {
    STATEMENT_ERROR(statement,"TABLE index set type is unsupported");
    return 0;
  }
  FPRINTF(ASCERR,"Internal TABLE %s-label mapping failure.\n",axis);
  return 0;
}

/* Tokens point into a private copy of the parser's normalised TABLE body. */
struct table_vector_row_t {
  char **tokens;
  unsigned long len;
};

struct table_vector_t {
  char **labels;
  struct table_cell_value_t *values;
  unsigned long len;
};

static void TableVectorDestroy(struct table_vector_t *vector)
{
  unsigned long i;
  for (i = 0; i < vector->len; ++i) {
    ascfree(vector->labels[i]);
  }
  if (vector->labels != NULL) ascfree(vector->labels);
  if (vector->values != NULL) ascfree(vector->values);
}

/* Unlike strtok, keep whitespace inside quoted symbol labels. */
static char *TableVectorNextToken(char **cursor)
{
  char *start = *cursor;
  char *end;
  while (*start && isspace((unsigned char)*start)) ++start;
  if (!*start) return NULL;
  end = start;
  if (*start == '\'') {
    ++end;
    while (*end && *end != '\'') ++end;
    if (*end) ++end;
  }
  while (*end && !isspace((unsigned char)*end)) ++end;
  *cursor = *end ? end + 1 : end;
  if (*end) *end = '\0';
  return start;
}

static char *TableVectorReadLabel(CONST struct table_vector_row_t *row,
                                  unsigned long *offset)
{
  char *tok;
  if (*offset >= row->len) return NULL;
  tok = row->tokens[(*offset)++];
  if (TableTokenIsSign(tok)) {
    char *label;
    long unused;
    size_t len;
    if (*offset >= row->len
        || !TableParseIntegerToken(row->tokens[*offset],&unused)) return NULL;
    len = strlen(row->tokens[*offset]);
    label = ASC_NEW_ARRAY(char,len + 2);
    label[0] = tok[0];
    memcpy(label + 1,row->tokens[(*offset)++],len + 1);
    return label;
  }
  if (TableTokenIsPunctuation(tok) || tok[0] == '{') return NULL;
  return ASC_STRDUP(tok);
}

static int TableVectorReadValue(CONST struct table_vector_row_t *row,
                                unsigned long *offset,
                                struct table_cell_value_t *value)
{
  int sign = 1;
  char *tok;
  if (*offset >= row->len) return 0;
  tok = row->tokens[(*offset)++];
  if (TableTokenIsSign(tok)) {
    sign = tok[0] == '-' ? -1 : 1;
    if (*offset >= row->len) return 0;
    tok = row->tokens[(*offset)++];
  }
  return TableParseNumberToken(tok,sign,&value->is_int,&value->ival,&value->rval);
}

/* Probe syntax only: no assignments, domain inference, or diagnostics here. */
static int TableVectorParse(CONST struct table_vector_row_t *rows,
                             unsigned long nrows, int horizontal,
                             struct table_vector_t *vector)
{
  unsigned long r, p, i;
  if (nrows == 0 || (horizontal && nrows != 2)) return 0;
  if (horizontal) {
    int empty_corner = strcmp(rows[0].tokens[0],",") == 0;
    p = empty_corner || strcmp(rows[0].tokens[0],":") == 0 ? 1 : 0;
    while (p < rows[0].len) {
      char *label = TableVectorReadLabel(&rows[0],&p);
      if (label == NULL) return 0;
      vector->labels = (char **)ascrealloc(vector->labels,
          sizeof(char *) * (vector->len + 1));
      vector->labels[vector->len++] = label;
      if (p < rows[0].len && strcmp(rows[0].tokens[p],",") == 0) {
        if (++p == rows[0].len) return 0;
      }
    }
    if (vector->len == 0) return 0;
    vector->values = ASC_NEW_ARRAY(struct table_cell_value_t,vector->len);
    p = 0;
    /* A CSV row vector may retain the empty label column on both lines. */
    if (empty_corner && strcmp(rows[1].tokens[0],",") == 0) ++p;
    for (i = 0; i < vector->len; ++i) {
      if (!TableVectorReadValue(&rows[1],&p,&vector->values[i])) return 0;
      if (i + 1 < vector->len && p < rows[1].len
          && strcmp(rows[1].tokens[p],",") == 0) ++p;
    }
    return p == rows[1].len;
  }
  vector->labels = ASC_NEW_ARRAY(char *,nrows);
  vector->values = ASC_NEW_ARRAY(struct table_cell_value_t,nrows);
  for (r = 0; r < nrows; ++r) {
    char *label;
    p = 0;
    label = TableVectorReadLabel(&rows[r],&p);
    if (label == NULL) return 0;
    vector->labels[vector->len++] = label;
    if (p < rows[r].len && strcmp(rows[r].tokens[p],":") == 0) ++p;
    if (p < rows[r].len && strcmp(rows[r].tokens[p],",") == 0) ++p;
    if (!TableVectorReadValue(&rows[r],&p,&vector->values[r])
        || p != rows[r].len) return 0;
  }
  return 1;
}

static int ExecuteTABLEVector(struct Instance *work, struct Statement *statement,
                              CONST struct table_domain_ref_t *ref)
{
  struct table_domain_t domain = {NULL,empty_set,0};
  struct scalar_units_runtime_t units_runtime;
  struct table_vector_row_t *rows = NULL;
  struct table_vector_t horizontal = {NULL,NULL,0};
  struct table_vector_t vertical = {NULL,NULL,0};
  struct table_vector_t *vector;
  char *body = ASC_STRDUP(statement->v.table.body != NULL ? statement->v.table.body : "");
  char *line, *line_ctx = NULL;
  unsigned long nrows = 0, i;
  unsigned long *positions = NULL;
  char *seen = NULL;
  int h, v, undefined = 0, result = 0;

  for (line = strtok_r(body,"\n",&line_ctx); line != NULL;
       line = strtok_r(NULL,"\n",&line_ctx)) {
    char *cursor = line, *tok;
    struct table_vector_row_t row = {NULL,0};
    while ((tok = TableVectorNextToken(&cursor)) != NULL) {
      row.tokens = (char **)ascrealloc(row.tokens,sizeof(char *) * (row.len + 1));
      row.tokens[row.len++] = tok;
    }
    if (row.len) {
      rows = (struct table_vector_row_t *)ascrealloc(rows,sizeof(*rows) * (nrows + 1));
      rows[nrows++] = row;
    }
  }
  h = TableVectorParse(rows,nrows,1,&horizontal);
  v = TableVectorParse(rows,nrows,0,&vertical);
  if (h && v) {
    STATEMENT_ERROR(statement,"Ambiguous 1-D TABLE: use ': labels...' for horizontal layout or 'label: value' for vertical layout");
    goto invalid;
  }
  if (!h && !v) {
    STATEMENT_ERROR(statement,"Malformed labelled 1-D TABLE: expected a label header and numeric value row, or label/value pairs; empty data fields are not allowed");
    goto invalid;
  }
  vector = h ? &horizontal : &vertical;
  if (!ResolveScalarUnits(statement->v.table.units,statement,"TABLE",&units_runtime)) goto invalid;
  if (!TableEvaluateDomainExpr(work,ref->expr,statement,&domain,&undefined)) {
    if (!undefined || !TableInferDomainFromLabels(work,ref->set_name,
        vector->labels,vector->len,statement,&domain)) goto invalid;
  }
  if (domain.len != vector->len) {
    STATEMENT_ERROR(statement,"1-D TABLE label/value count does not match index cardinality");
    goto invalid;
  }
  positions = ASC_NEW_ARRAY(unsigned long,vector->len);
  seen = ASC_NEW_ARRAY_CLEAR(char,domain.len);
  for (i = 0; i < vector->len; ++i) {
    if (!TableLabelToPosition(&domain,vector->labels[i],&positions[i],statement,1)) goto invalid;
    if (seen[positions[i] - 1]) {
      STATEMENT_ERROR(statement,"1-D TABLE contains duplicate labels");
      goto invalid;
    }
    seen[positions[i] - 1] = 1;
  }
  for (i = 0; i < vector->len; ++i) {
    struct table_cell_value_t *cell = &vector->values[i];
    int assigned = TableAssignCellMaybeWait(work,statement,&domain,1,&positions[i],
        &units_runtime,cell->is_int,cell->ival,cell->rval);
    if (assigned < 0) goto cleanup;
    if (!assigned) goto invalid;
  }
  result = 1;
  goto cleanup;

invalid:
  MarkStatContext(statement,context_WRONG);
  result = 1;
cleanup:
  if (seen != NULL) ascfree(seen);
  if (positions != NULL) ascfree(positions);
  TableDomainDestroy(&domain);
  TableVectorDestroy(&horizontal);
  TableVectorDestroy(&vertical);
  for (i = 0; i < nrows; ++i) ascfree(rows[i].tokens);
  if (rows != NULL) ascfree(rows);
  ascfree(body);
  return result;
}

static int ExecuteTABLEDense(struct Instance *work, struct Statement *statement)
{
  struct table_domain_t domains[2];
  struct table_domain_ref_t refs[2];
  struct scalar_units_runtime_t units_runtime;
  unsigned ndims = 0;
  char *bodycopy = NULL;
  char *line_ctx = NULL;
  char *line;
  char **col_labels = NULL;
  char **row_labels = NULL;
  struct table_cell_value_t *values = NULL;
  struct table_cell_value_t *row_values = NULL;
  unsigned long *row_pos = NULL;
  unsigned long *col_pos = NULL;
  char *row_seen = NULL;
  char *col_seen = NULL;
  unsigned long ncols = 0;
  unsigned long nrows = 0;
  int header_seen = 0;
  unsigned d;
  int rval = 0;

  if (!TableCollectDomainRefs(statement->v.table.name,statement,refs,&ndims)) {
    MarkStatContext(statement,context_WRONG);
    return 1;
  }
  if (ndims == 1) {
    return ExecuteTABLEVector(work,statement,&refs[0]);
  }
  if (ndims != 2) {
    STATEMENT_ERROR(statement,"Dense non-POSITIONAL TABLE requires 1 or 2 indices");
    MarkStatContext(statement,context_WRONG);
    return 1;
  }

  if (!ResolveScalarUnits(statement->v.table.units,statement,"TABLE",&units_runtime)) {
    MarkStatContext(statement,context_WRONG);
    return 1;
  }

  for (d = 0; d < 2; ++d) {
    domains[d].set = NULL;
    domains[d].kind = empty_set;
    domains[d].len = 0;
  }

  if (statement->v.table.body != NULL) {
    size_t n = strlen(statement->v.table.body);
    bodycopy = ASC_NEW_ARRAY(char,n + 1);
    memcpy(bodycopy,statement->v.table.body,n + 1);
  } else {
    bodycopy = ASC_NEW_ARRAY(char,1);
    bodycopy[0] = '\0';
  }

  for (line = strtok_r(bodycopy,"\n",&line_ctx); line != NULL; line = strtok_r(NULL,"\n",&line_ctx)) {
    char *tok_ctx = NULL;
    char *tok = strtok_r(line," \t\r",&tok_ctx);

    if (tok == NULL) {
      continue;
    }

    if (!header_seen) {
      int expect_label = 1;
      int seen_any = 0;
      int last_delim = 0;
      if (strcmp(tok,":") == 0) {
        tok = strtok_r(NULL," \t\r",&tok_ctx);
      }
      for (; tok != NULL; tok = strtok_r(NULL," \t\r",&tok_ctx)) {
        if (TableTokenIsDelimiter(tok)) {
          if (expect_label || !seen_any) {
            STATEMENT_ERROR(statement,"TABLE header has misplaced delimiter");
            MarkStatContext(statement,context_WRONG);
            goto cleanup;
          }
          expect_label = 1;
          last_delim = tok[0];
          continue;
        }
        if (TableTokenIsPunctuation(tok)) {
          STATEMENT_ERROR(statement,"TABLE header contains invalid punctuation");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        if (!expect_label) {
          /* whitespace-separated labels are accepted */
        }
        col_labels = (char **)ascrealloc(col_labels,sizeof(char *) * (ncols + 1));
        col_labels[ncols++] = tok;
        seen_any = 1;
        expect_label = 0;
        last_delim = 0;
      }
      if (!seen_any) {
        STATEMENT_ERROR(statement,"TABLE header row is empty");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
      if (expect_label) {
        if (last_delim != ';') {
          STATEMENT_ERROR(statement,"TABLE header cannot end with delimiter");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
      }
      header_seen = 1;
      continue;
    }

    {
      unsigned long row_values_count = 0;
      int sign = 1;
      int sign_pending = 0;
      int delim_pending = 0;
      int last_delim = 0;

      if (TableTokenIsPunctuation(tok)) {
        STATEMENT_ERROR(statement,"TABLE data row is missing row label");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }

      row_labels = (char **)ascrealloc(row_labels,sizeof(char *) * (nrows + 1));
      row_labels[nrows] = tok;

      tok = strtok_r(NULL," \t\r",&tok_ctx);
      if (tok != NULL && strcmp(tok,":") == 0) {
        tok = strtok_r(NULL," \t\r",&tok_ctx);
      }
      if (tok != NULL && TableTokenIsDelimiter(tok)) {
        tok = strtok_r(NULL," \t\r",&tok_ctx);
      }

      row_values = ASC_NEW_ARRAY(struct table_cell_value_t,ncols > 0 ? ncols : 1);
      for (; tok != NULL; tok = strtok_r(NULL," \t\r",&tok_ctx)) {
        if (TableTokenIsDelimiter(tok)) {
          if (sign_pending) {
            STATEMENT_ERROR(statement,"TABLE delimiter cannot follow a sign without a value");
            MarkStatContext(statement,context_WRONG);
            goto cleanup;
          }
          if (row_values_count == 0) {
            STATEMENT_ERROR(statement,"TABLE data row cannot begin values with delimiter");
            MarkStatContext(statement,context_WRONG);
            goto cleanup;
          }
          if (delim_pending) {
            STATEMENT_ERROR(statement,"TABLE data row has repeated delimiters without a value");
            MarkStatContext(statement,context_WRONG);
            goto cleanup;
          }
          delim_pending = 1;
          last_delim = tok[0];
          continue;
        }
        if (TableTokenIsSign(tok)) {
          if (sign_pending) {
            STATEMENT_ERROR(statement,"Malformed TABLE value sign sequence");
            MarkStatContext(statement,context_WRONG);
            goto cleanup;
          }
          sign = (tok[0] == '-') ? -1 : 1;
          sign_pending = 1;
          delim_pending = 0;
          last_delim = 0;
          continue;
        }
        if (strcmp(tok,":") == 0 || strcmp(tok,"=") == 0) {
          STATEMENT_ERROR(statement,"Unexpected sparse punctuation in dense TABLE row");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        if (row_values_count >= ncols) {
          STATEMENT_ERROR(statement,"TABLE row has too many values");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        if (!TableParseNumberToken(tok,sign_pending ? sign : 1,
                                   &row_values[row_values_count].is_int,
                                   &row_values[row_values_count].ival,
                                   &row_values[row_values_count].rval)) {
          STATEMENT_ERROR(statement,"Dense TABLE contains non-numeric value token");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        row_values_count++;
        sign = 1;
        sign_pending = 0;
        delim_pending = 0;
        last_delim = 0;
      }

      if (sign_pending) {
        STATEMENT_ERROR(statement,"Dangling sign token at end of TABLE row");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
      if (delim_pending) {
        if (last_delim != ';') {
          STATEMENT_ERROR(statement,"TABLE data row cannot end with delimiter");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
      }
      if (row_values_count != ncols) {
        STATEMENT_ERROR(statement,"TABLE row has incorrect number of values");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }

      values = (struct table_cell_value_t *)ascrealloc(values,sizeof(struct table_cell_value_t) * (nrows + 1) * ncols);
      memcpy(values + nrows * ncols,row_values,sizeof(struct table_cell_value_t) * ncols);
      ascfree(row_values);
      row_values = NULL;
      nrows++;
    }
  }

  if (!header_seen) {
    STATEMENT_ERROR(statement,"TABLE body is missing header row");
    MarkStatContext(statement,context_WRONG);
    goto cleanup;
  }
  if (nrows == 0) {
    STATEMENT_ERROR(statement,"TABLE body is missing data rows");
    MarkStatContext(statement,context_WRONG);
    goto cleanup;
  }

  for (d = 0; d < 2; ++d) {
    int undefined = 0;
    if (!TableEvaluateDomainExpr(work,refs[d].expr,statement,&domains[d],&undefined)) {
      if (!undefined) {
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
      if (!TableInferDomainFromLabels(work
            ,refs[d].set_name
            ,(d == 0) ? row_labels : col_labels
            ,(d == 0) ? nrows : ncols
            ,statement
            ,&domains[d])) {
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
    }
  }

  if (domains[0].len != nrows) {
    STATEMENT_ERROR(statement,"TABLE row count does not match first index cardinality");
    MarkStatContext(statement,context_WRONG);
    goto cleanup;
  }
  if (domains[1].len != ncols) {
    STATEMENT_ERROR(statement,"TABLE column count does not match second index cardinality");
    MarkStatContext(statement,context_WRONG);
    goto cleanup;
  }

  row_pos = ASC_NEW_ARRAY(unsigned long,nrows);
  col_pos = ASC_NEW_ARRAY(unsigned long,ncols);
  row_seen = ASC_NEW_ARRAY(char,domains[0].len > 0 ? domains[0].len : 1);
  col_seen = ASC_NEW_ARRAY(char,domains[1].len > 0 ? domains[1].len : 1);
  memset(row_seen,0,domains[0].len > 0 ? domains[0].len : 1);
  memset(col_seen,0,domains[1].len > 0 ? domains[1].len : 1);

  for (d = 0; d < nrows; ++d) {
    if (!TableLabelToPosition(&domains[0],row_labels[d],&row_pos[d],statement,1)) {
      MarkStatContext(statement,context_WRONG);
      goto cleanup;
    }
    if (row_seen[row_pos[d] - 1]) {
      STATEMENT_ERROR(statement,"TABLE contains duplicate row labels");
      MarkStatContext(statement,context_WRONG);
      goto cleanup;
    }
    row_seen[row_pos[d] - 1] = 1;
  }
  for (d = 0; d < ncols; ++d) {
    if (!TableLabelToPosition(&domains[1],col_labels[d],&col_pos[d],statement,0)) {
      MarkStatContext(statement,context_WRONG);
      goto cleanup;
    }
    if (col_seen[col_pos[d] - 1]) {
      STATEMENT_ERROR(statement,"TABLE contains duplicate column labels");
      MarkStatContext(statement,context_WRONG);
      goto cleanup;
    }
    col_seen[col_pos[d] - 1] = 1;
  }

  for (d = 0; d < nrows; ++d) {
    unsigned long c;
    for (c = 0; c < ncols; ++c) {
      unsigned long pos[2];
      CONST struct table_cell_value_t *cell = &values[d * ncols + c];
      int assign_result;
      pos[0] = row_pos[d];
      pos[1] = col_pos[c];
      assign_result = TableAssignCellMaybeWait(work,statement,domains,2,pos,
                                               &units_runtime,
                                               cell->is_int,cell->ival,cell->rval);
      if (assign_result < 0) {
        rval = 0;
        goto cleanup;
      }
      if (!assign_result) {
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
    }
  }

  rval = 1;

cleanup:
  if (row_values != NULL) {
    ascfree(row_values);
  }
  if (row_seen != NULL) {
    ascfree(row_seen);
  }
  if (col_seen != NULL) {
    ascfree(col_seen);
  }
  if (row_pos != NULL) {
    ascfree(row_pos);
  }
  if (col_pos != NULL) {
    ascfree(col_pos);
  }
  if (values != NULL) {
    ascfree(values);
  }
  if (row_labels != NULL) {
    ascfree(row_labels);
  }
  if (col_labels != NULL) {
    ascfree(col_labels);
  }
  if (bodycopy != NULL) {
    ascfree(bodycopy);
  }
  for (d = 0; d < 2; ++d) {
    TableDomainDestroy(&domains[d]);
  }
  return rval;
}

static struct Name *TableBuildCellName(CONST struct Name *templ,
                                       CONST struct table_domain_t *domains,
                                       unsigned ndims,
                                       CONST unsigned long *positions)
{
  CONST struct Name *scan;
  CONST struct Name *node;
  struct Name *result = NULL;
  unsigned d = 0;

  for (node = templ; node != NULL; node = NextName(node)) {
    if (NameId(node)) {
      result = CopyAppendNameNode(result,node);
    } else {
      unsigned rem_nodes = 0;
      unsigned rem_dims;
      unsigned take;
      unsigned k;

      for (scan = node; scan != NULL; scan = NextName(scan)) {
        if (!NameId(scan)) {
          rem_nodes++;
        }
      }
      rem_dims = ndims - d;
      if (rem_nodes == 0 || rem_dims == 0 || rem_dims < rem_nodes) {
        DestroyName(result);
        return NULL;
      }
      take = rem_dims - (rem_nodes - 1);
      for (k = 0; k < take; ++k) {
        struct Name *indexnode = NULL;
        switch (domains[d].kind) {
        case integer_set:
          indexnode = CreateIntegerElementName((long)FetchIntMember(domains[d].set,positions[d]));
          break;
        case string_set:
          indexnode = CreateEnumElementName(FetchStrMember(domains[d].set,positions[d]));
          break;
        default:
          DestroyName(result);
          return NULL;
        }
        result = JoinNames(result,indexnode);
        d++;
      }
    }
  }
  if (d != ndims) {
    DestroyName(result);
    return NULL;
  }
  return result;
}

static int TableAssignCell(struct Instance *work,
                           struct Statement *statement,
                           CONST struct table_domain_t *domains,
                           unsigned ndims,
                           CONST unsigned long *positions,
                           CONST struct scalar_units_runtime_t *units_runtime,
                           int is_int,
                           long ival,
                           double rval)
{
  struct Name *lhs;
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *inst;
  int ok;

  lhs = TableBuildCellName(statement->v.table.name,domains,ndims,positions);
  if (lhs == NULL) {
    STATEMENT_ERROR(statement,"Unable to construct TABLE assignment target name");
    return 0;
  }

  instances = FindInstances(work,lhs,&err);
  DestroyName(lhs);
  if (instances == NULL || gl_length(instances) != 1) {
    if (instances != NULL) {
      gl_destroy(instances);
    }
    STATEMENT_ERROR(statement,"TABLE assignment target could not be resolved uniquely");
    return 0;
  }
  inst = (struct Instance *)gl_fetch(instances,1);
  gl_destroy(instances);

  ok = AssignNumericToConstantInstance(inst,statement,is_int,ival,rval,
                                       units_runtime,Dimensionless(),"TABLE");
  return ok;
}

static int TableAssignCellMaybeWait(struct Instance *work,
                                    struct Statement *statement,
                                    CONST struct table_domain_t *domains,
                                    unsigned ndims,
                                    CONST unsigned long *positions,
                                    CONST struct scalar_units_runtime_t *units_runtime,
                                    int is_int,
                                    long ival,
                                    double rval)
{
  struct Name *lhs;
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *inst;
  int ok;

  lhs = TableBuildCellName(statement->v.table.name,domains,ndims,positions);
  if (lhs == NULL) {
    STATEMENT_ERROR(statement,"Unable to construct TABLE assignment target name");
    return 0;
  }

  instances = FindInstances(work,lhs,&err);
  DestroyName(lhs);
  if (instances == NULL) {
    switch (rel_errorlist_get_find_error(&err)) {
    case unmade_instance:
    case undefined_instance:
      return -1;
    default:
      STATEMENT_ERROR(statement,"TABLE assignment target could not be resolved uniquely");
      return 0;
    }
  }
  if (gl_length(instances) != 1) {
    gl_destroy(instances);
    STATEMENT_ERROR(statement,"TABLE assignment target could not be resolved uniquely");
    return 0;
  }
  inst = (struct Instance *)gl_fetch(instances,1);
  gl_destroy(instances);

  ok = AssignNumericToConstantInstance(inst,statement,is_int,ival,rval,
                                       units_runtime,Dimensionless(),"TABLE");
  return ok;
}

struct dataset_domain_ref_t {
  CONST struct Expr *expr;
  CONST struct Name *set_name;
};

struct dataset_index_runtime_t {
  CONST struct DatasetIndexItem *item;
  symchar *set_name;
  unsigned col;
  enum set_kind kind;
  int implicit_row;
  struct set_t *set;
  struct table_domain_t domain;
  int int_fastpath;
  long int_first;
  long int_last;
};

struct dataset_map_runtime_t {
  CONST struct DatasetMapItem *item;
  unsigned col;
  unsigned ndims;
  struct dataset_index_runtime_t **indices;
  struct table_domain_t *domains;
  unsigned long *posbuf;
  int fast_array_1d;
  struct Instance *fast_array_inst;
};

static int DatasetFindColumn(CONST struct dataset_table_t *table, symchar *name)
{
  unsigned i;
  if (table == NULL || name == NULL) {
    return -1;
  }
  for (i = 0; i < table->ncols; ++i) {
    if (table->cols[i].name == name) {
      return (int)i;
    }
  }
  return -1;
}

static struct dataset_index_runtime_t *DatasetFindIndexBySetName(struct dataset_index_runtime_t *indices,
                                                                 unsigned nindices,
                                                                 symchar *set_name)
{
  unsigned i;
  if (indices == NULL || set_name == NULL) {
    return NULL;
  }
  for (i = 0; i < nindices; ++i) {
    if (indices[i].set_name == set_name) {
      return &indices[i];
    }
  }
  return NULL;
}

static int DatasetIndexTypeKind(symchar *type_name, enum set_kind *kind)
{
  if (type_name == NULL || kind == NULL) {
    return 0;
  }
  if (type_name == GetBaseTypeName(integer_constant_type)
      || type_name == GetBaseTypeName(integer_type)) {
    *kind = integer_set;
    return 1;
  }
  if (type_name == GetBaseTypeName(symbol_constant_type)
      || type_name == GetBaseTypeName(symbol_type)) {
    *kind = string_set;
    return 1;
  }
  return 0;
}

static int DatasetIntValueCmp(VOIDPTR a, VOIDPTR b)
{
  asc_intptr_t ia = (asc_intptr_t)a;
  asc_intptr_t ib = (asc_intptr_t)b;
  return (ia > ib) ? 1 : ((ia < ib) ? -1 : 0);
}

static void DatasetInitIndexFastPath(struct dataset_index_runtime_t *index)
{
  unsigned long i;
  CONST struct table_domain_t *domain;

  if (index == NULL) {
    return;
  }
  index->int_fastpath = 0;
  index->int_first = 0;
  index->int_last = 0;

  domain = &index->domain;
  if (domain->kind != integer_set || domain->set == NULL || domain->len == 0) {
    return;
  }

  index->int_first = (long)FetchIntMember(domain->set,1);
  index->int_last = index->int_first;
  for (i = 2; i <= domain->len; ++i) {
    long current = (long)FetchIntMember(domain->set,i);
    if (index->int_last == LONG_MAX || current != index->int_last + 1) {
      return;
    }
    index->int_last = current;
  }
  index->int_fastpath = 1;
}

static int DatasetLabelToPosition(CONST struct dataset_index_runtime_t *index,
                                  CONST char *tok,
                                  unsigned long *pos,
                                  struct Statement *statement)
{
  CONST struct table_domain_t *domain;
  if (index == NULL || tok == NULL || pos == NULL) {
    return 0;
  }
  domain = &index->domain;
  if (domain->kind == integer_set) {
    long ival;
    unsigned long p;
    if (!TableParseIntegerToken(tok,&ival)) {
      STATEMENT_ERROR(statement,"DATASET index label is not a valid integer");
      return 0;
    }
    if (index->int_fastpath) {
      if (ival < index->int_first || ival > index->int_last) {
        STATEMENT_ERROR(statement,"DATASET index label is not a member of index set");
        return 0;
      }
      *pos = (unsigned long)(ival - index->int_first) + 1;
      return 1;
    }
    p = gl_search(domain->set->list,(VOIDPTR)(asc_intptr_t)ival,(CmpFunc)DatasetIntValueCmp);
    if (p == 0) {
      STATEMENT_ERROR(statement,"DATASET index label is not a member of index set");
      return 0;
    }
    *pos = p;
    return 1;
  } else if (domain->kind == string_set) {
    symchar *sym;
    unsigned long p;
    if (!TableParseSymbolToken(tok,&sym)) {
      STATEMENT_ERROR(statement,"DATASET index label is not a valid symbol");
      return 0;
    }
    p = gl_search(domain->set->list,sym,(CmpFunc)CmpSymchar);
    if (p == 0) {
      STATEMENT_ERROR(statement,"DATASET index label is not a member of index set");
      return 0;
    }
    *pos = p;
    return 1;
  } else {
    STATEMENT_ERROR(statement,"DATASET index set type is unsupported");
    return 0;
  }
  STATEMENT_ERROR(statement,"DATASET index label mapping failure");
  return 0;
}

static int DatasetCollectDomainRefs(CONST struct Name *target,
                                    struct Statement *statement,
                                    struct dataset_domain_ref_t **out_refs,
                                    unsigned *out_ndims)
{
  CONST struct Name *node;
  struct dataset_domain_ref_t *refs = NULL;
  unsigned cap = 0;
  unsigned count = 0;

  if (out_refs == NULL || out_ndims == NULL) {
    return 0;
  }
  *out_refs = NULL;
  *out_ndims = 0;

  for (node = target; node != NULL; node = NextName(node)) {
    CONST struct Set *setnode;
    if (NameId(node)) {
      continue;
    }
    for (setnode = NameSetPtr(node); setnode != NULL; setnode = NextSet(setnode)) {
      CONST struct Expr *expr;
      if (SetType(setnode)) {
        STATEMENT_ERROR(statement,"DATASET index ranges are not supported");
        ascfree(refs);
        return 0;
      }
      expr = GetSingleExpr(setnode);
      if (expr == NULL) {
        STATEMENT_ERROR(statement,"DATASET index expression is invalid");
        ascfree(refs);
        return 0;
      }
      if (count >= cap) {
        cap = (cap == 0) ? 4 : cap * 2;
        refs = (struct dataset_domain_ref_t *)ascrealloc(refs,sizeof(struct dataset_domain_ref_t) * cap);
      }
      refs[count].expr = expr;
      refs[count].set_name = NULL;
      if (ExprType(expr) == e_var
          && ExprListLength(expr) == 1
          && SimpleNameIdPtr(ExprName(expr)) != NULL) {
        refs[count].set_name = ExprName(expr);
      }
      count++;
    }
  }
  if (count == 0) {
    STATEMENT_ERROR(statement,"DATASET target must include at least one index");
    ascfree(refs);
    return 0;
  }
  *out_refs = refs;
  *out_ndims = count;
  return 1;
}

static int DatasetAssignTokenToInstance(struct Instance *inst,
                                        struct Statement *statement,
                                        CONST char *token,
                                        CONST char *units)
{
  CONST char *valtok = token;
  CONST char *cell_units = NULL;
  char *valbuf = NULL;
  char *unitbuf = NULL;
  struct scalar_units_runtime_t units_runtime;
  struct value_t value;
  int ok;

  if (valtok != NULL) {
    const char *brace = strchr(valtok,'{');
    size_t len = strlen(valtok);
    if (brace != NULL && len > 0 && valtok[len - 1] == '}') {
      size_t nlen = (size_t)(brace - valtok);
      size_t ulen = len - nlen - 2;
      valbuf = ASC_NEW_ARRAY(char,nlen + 1);
      unitbuf = ASC_NEW_ARRAY(char,ulen + 1);
      memcpy(valbuf,valtok,nlen);
      valbuf[nlen] = '\0';
      memcpy(unitbuf,brace + 1,ulen);
      unitbuf[ulen] = '\0';
      valtok = valbuf;
      cell_units = unitbuf;
    }
  }

  if (units != NULL && cell_units != NULL && strcmp(units,cell_units) != 0) {
    STATEMENT_ERROR(statement,"DATASET value units conflict with declared units");
    ok = 0;
    goto cleanup_units;
  }
  if (units == NULL && cell_units != NULL) {
    units = cell_units;
  }

  if (!ResolveScalarUnits(units,statement,"DATASET",&units_runtime)) {
    ok = 0;
    goto cleanup_units;
  }

  switch (InstanceKind(inst)) {
  case REAL_CONSTANT_INST:
    {
      double rval;
      if (!DatasetParseRealToken(valtok,&rval)) {
        STATEMENT_ERROR(statement,"DATASET value is not a valid real");
        ok = 0;
        break;
      }
      ok = AssignNumericToConstantInstance(inst,statement,0,0,rval,
                                           &units_runtime,Dimensionless(),"DATASET");
    }
    break;
  case INTEGER_CONSTANT_INST:
    {
      long ival;
      if (!TableParseIntegerToken(valtok,&ival)) {
        STATEMENT_ERROR(statement,"DATASET value is not a valid integer");
        ok = 0;
        break;
      }
      ok = AssignNumericToConstantInstance(inst,statement,1,ival,(double)ival,
                                           &units_runtime,Dimensionless(),"DATASET");
    }
    break;
  case SYMBOL_CONSTANT_INST:
    {
      symchar *sym;
      if (units != NULL) {
        STATEMENT_ERROR(statement,"DATASET units are not allowed for symbol values");
        ok = 0;
        break;
      }
      if (!TableParseSymbolToken(valtok,&sym)) {
        STATEMENT_ERROR(statement,"DATASET value is not a valid symbol");
        ok = 0;
        break;
      }
      value = CreateSymbolValue(sym,1);
      ok = AssignStructuralValue(inst,value,statement);
      DestroyValue(&value);
    }
    break;
  case BOOLEAN_CONSTANT_INST:
    {
      int bval;
      if (units != NULL) {
        STATEMENT_ERROR(statement,"DATASET units are not allowed for boolean values");
        ok = 0;
        break;
      }
      if (!DatasetParseBooleanToken(valtok,&bval)) {
        STATEMENT_ERROR(statement,"DATASET value is not a valid boolean");
        ok = 0;
        break;
      }
      value = CreateBooleanValue(bval,1);
      ok = AssignStructuralValue(inst,value,statement);
      DestroyValue(&value);
    }
    break;
  default:
    STATEMENT_ERROR(statement,"DATASET assignment target is not a constant");
    ok = 0;
    break;
  }

cleanup_units:
  if (valbuf != NULL) {
    ascfree(valbuf);
  }
  if (unitbuf != NULL) {
    ascfree(unitbuf);
  }
  return ok;
}

static int DatasetAssignValueMaybeWait(struct Instance *work,
                                       struct Statement *statement,
                                       CONST struct DatasetMapItem *map,
                                       CONST struct table_domain_t *domains,
                                       unsigned ndims,
                                       CONST unsigned long *positions,
                                       CONST char *token,
                                       CONST char *units)
{
  struct Name *lhs;
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *inst;

  lhs = TableBuildCellName(map->target,domains,ndims,positions);
  if (lhs == NULL) {
    STATEMENT_ERROR(statement,"Unable to construct DATASET assignment target name");
    return 0;
  }

  instances = FindInstances(work,lhs,&err);
  DestroyName(lhs);
  if (instances == NULL) {
    switch (rel_errorlist_get_find_error(&err)) {
    case unmade_instance:
    case undefined_instance:
      return -1;
    default:
      STATEMENT_ERROR(statement,"DATASET assignment target could not be resolved uniquely");
      return 0;
    }
  }
  if (gl_length(instances) != 1) {
    gl_destroy(instances);
    STATEMENT_ERROR(statement,"DATASET assignment target could not be resolved uniquely");
    return 0;
  }
  inst = (struct Instance *)gl_fetch(instances,1);
  gl_destroy(instances);
  return DatasetAssignTokenToInstance(inst,statement,token,units);
}

static struct Name *DatasetBuildSimpleArrayName(CONST struct Name *target, unsigned ndims)
{
  CONST struct Name *node;
  CONST struct Name *last = NULL;
  struct Name *result = NULL;
  unsigned seen_sets = 0;
  int seen_set_node = 0;

  if (target == NULL || ndims != 1) {
    return NULL;
  }

  for (node = target; node != NULL; node = NextName(node)) {
    last = node;
    if (NameId(node)) {
      if (seen_set_node) {
        DestroyName(result);
        return NULL;
      }
      result = CopyAppendNameNode(result,node);
    } else {
      CONST struct Set *setnode = NameSetPtr(node);
      seen_set_node = 1;
      for (; setnode != NULL; setnode = NextSet(setnode)) {
        if (SetType(setnode) || GetSingleExpr(setnode) == NULL) {
          DestroyName(result);
          return NULL;
        }
        seen_sets++;
      }
    }
  }

  if (result == NULL || last == NULL || NameId(last) || seen_sets != ndims) {
    DestroyName(result);
    return NULL;
  }
  return result;
}

static int DatasetTryBindFastArray1D(struct Instance *work,
                                     struct Statement *statement,
                                     struct dataset_map_runtime_t *map_runtime)
{
  struct Name *array_name;
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *inst;

  if (work == NULL || statement == NULL || map_runtime == NULL) {
    return 0;
  }
  if (map_runtime->ndims != 1 || map_runtime->item == NULL) {
    return 1;
  }

  array_name = DatasetBuildSimpleArrayName(map_runtime->item->target,map_runtime->ndims);
  if (array_name == NULL) {
    return 1;
  }

  instances = FindInstances(work,array_name,&err);
  DestroyName(array_name);
  if (instances == NULL) {
    switch (rel_errorlist_get_find_error(&err)) {
    case unmade_instance:
    case undefined_instance:
      return -1;
    default:
      STATEMENT_ERROR(statement,"DATASET fast-path array target could not be resolved uniquely");
      return 0;
    }
  }
  if (gl_length(instances) != 1) {
    gl_destroy(instances);
    STATEMENT_ERROR(statement,"DATASET fast-path array target could not be resolved uniquely");
    return 0;
  }
  inst = (struct Instance *)gl_fetch(instances,1);
  gl_destroy(instances);

  if (!IsArrayInstance(inst)) {
    return 1;
  }

  map_runtime->fast_array_1d = 1;
  map_runtime->fast_array_inst = inst;
  return 1;
}

static int ExecuteDATASET(struct Instance *work, struct Statement *statement)
{
  struct dataset_table_t *table;
  struct DatasetIndexItem *idx;
  struct DatasetMapItem *map;
  struct dataset_index_runtime_t *indices = NULL;
  struct dataset_map_runtime_t *maps = NULL;
  struct dataset_map_prepare_t {
    CONST struct DatasetMapItem *item;
    struct dataset_domain_ref_t *refs;
    unsigned ndims;
    unsigned col;
  } *map_prep = NULL;
  unsigned nindices = 0;
  unsigned nmaps = 0;
  unsigned nexplicit_indices = 0;
  symchar **implicit_set_names = NULL;
  unsigned nimplicit_sets = 0;
  unsigned implicit_set_cap = 0;
  unsigned i;
  int pending = 0;
  int ok = 1;
  int dataset_profile = 0;
  int dataset_cache_hit = 1;
  double t_total = 0.0;
  double t_load = 0.0;
  double t_index = 0.0;
  double t_expand = 0.0;
  double t_map_prepare = 0.0;
  double t_assign = 0.0;
  double tmark = 0.0;
  const char *profile_env = getenv("ASC_DATASET_PROFILE");

  if (profile_env != NULL && profile_env[0] != '\0' && profile_env[0] != '0') {
    dataset_profile = 1;
    tmark = DatasetNowSeconds();
  }

  if (StatWrong(statement)) {
    return 1;
  }
  g_pass1_saw_dataset = 1;

  if (statement->v.dataset.filename == NULL) {
    STATEMENT_ERROR(statement,"DATASET filename is missing");
    MarkStatContext(statement,context_WRONG);
    return 1;
  }

  table = DatasetCacheGet(statement->v.dataset.filename);
  if (table == NULL) {
    double t0 = 0.0;
    struct dataset_stream_t stream;
    dataset_cache_hit = 0;
    if (dataset_profile) {
      t0 = DatasetNowSeconds();
    }
    if (!DatasetStreamOpen(statement->v.dataset.filename,NULL,statement,&stream)) {
      MarkStatContext(statement,context_WRONG);
      return 0;
    }
    table = DatasetReadFile(statement->v.dataset.filename,&stream,statement);
    DatasetStreamClose(&stream);
    if (table == NULL) {
      MarkStatContext(statement,context_WRONG);
      return 0;
    }
    DatasetCacheInsert(statement->v.dataset.filename,table);
    if (dataset_profile) {
      t_load += DatasetNowSeconds() - t0;
    }
  }

  if (dataset_profile) {
    tmark = DatasetNowSeconds();
  }

  for (idx = statement->v.dataset.indices; idx != NULL; idx = idx->next) {
    nindices++;
  }
  nexplicit_indices = nindices;

  indices = (nindices > 0) ? ASC_NEW_ARRAY(struct dataset_index_runtime_t,nindices) : NULL;
  for (i = 0; i < nindices; ++i) {
    indices[i].item = NULL;
    indices[i].set_name = NULL;
    indices[i].col = 0;
    indices[i].kind = empty_set;
    indices[i].implicit_row = 0;
    indices[i].set = NULL;
    indices[i].domain.set = NULL;
    indices[i].domain.kind = empty_set;
    indices[i].domain.len = 0;
    indices[i].int_fastpath = 0;
    indices[i].int_first = 0;
    indices[i].int_last = 0;
  }
  i = 0;
  for (idx = statement->v.dataset.indices; idx != NULL; idx = idx->next) {
    enum set_kind kind;
    int col = DatasetFindColumn(table,idx->column_name);
    if (col < 0) {
      STATEMENT_ERROR(statement,"DATASET INDEX column not found");
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    if (!DatasetIndexTypeKind(idx->type_name,&kind)) {
      STATEMENT_ERROR(statement,"DATASET INDEX type is unsupported");
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    indices[i].item = idx;
    indices[i].set_name = idx->set_name;
    indices[i].col = (unsigned)col;
    indices[i].kind = kind;
    indices[i].implicit_row = 0;
    indices[i].set = CreateEmptySet();
    indices[i].domain.set = NULL;
    indices[i].domain.kind = empty_set;
    indices[i].domain.len = 0;
    i++;
  }

  for (i = 0; i < table->nrows; ++i) {
    unsigned j;
    for (j = 0; j < nindices; ++j) {
      CONST char *tok = table->rows[i].cells[indices[j].col];
      if (indices[j].kind == integer_set) {
        long ival;
        if (!TableParseIntegerToken(tok,&ival)) {
          STATEMENT_ERROR(statement,"DATASET INDEX value is not a valid integer");
          MarkStatContext(statement,context_WRONG);
          ok = 0;
          goto cleanup;
        }
        InsertInteger(indices[j].set,(asc_intptr_t)ival);
      } else if (indices[j].kind == string_set) {
        symchar *sym;
        if (!TableParseSymbolToken(tok,&sym)) {
          STATEMENT_ERROR(statement,"DATASET INDEX value is not a valid symbol");
          MarkStatContext(statement,context_WRONG);
          ok = 0;
          goto cleanup;
        }
        InsertString(indices[j].set,sym);
      }
    }
  }

  for (i = 0; i < nindices; ++i) {
    struct Name *setname;
    if (indices[i].set_name == NULL) {
      STATEMENT_ERROR(statement,"DATASET INDEX set name is invalid");
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    setname = CreateIdName(indices[i].set_name);
    if (!TableAssignDomainSet(work,setname,indices[i].set,statement)) {
      DestroyName(setname);
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    DestroyName(setname);
    if (!TableInitDomainFromSet(indices[i].set,statement,&indices[i].domain)) {
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    DatasetInitIndexFastPath(&indices[i]);
    DestroySet(indices[i].set);
    indices[i].set = NULL;
  }

  if (dataset_profile) {
    double now = DatasetNowSeconds();
    t_index += now - tmark;
    tmark = now;
  }

  for (map = statement->v.dataset.maps; map != NULL; map = map->next) {
    nmaps++;
  }
  if (nmaps == 0) {
    STATEMENT_ERROR(statement,"DATASET has no map items");
    MarkStatContext(statement,context_WRONG);
    ok = 0;
    goto cleanup;
  }

  maps = ASC_NEW_ARRAY(struct dataset_map_runtime_t,nmaps);
  map_prep = ASC_NEW_ARRAY(struct dataset_map_prepare_t,nmaps);
  for (i = 0; i < nmaps; ++i) {
    maps[i].item = NULL;
    maps[i].col = 0;
    maps[i].ndims = 0;
    maps[i].indices = NULL;
    maps[i].domains = NULL;
    maps[i].posbuf = NULL;
    maps[i].fast_array_1d = 0;
    maps[i].fast_array_inst = NULL;
    map_prep[i].item = NULL;
    map_prep[i].refs = NULL;
    map_prep[i].ndims = 0;
    map_prep[i].col = 0;
  }
  i = 0;
  for (map = statement->v.dataset.maps; map != NULL; map = map->next) {
    struct dataset_domain_ref_t *refs = NULL;
    unsigned ndims = 0;
    unsigned d;
    int col = DatasetFindColumn(table,map->column_name);
    const char *col_units = NULL;
    if (col < 0) {
      STATEMENT_ERROR(statement,"DATASET column not found for map item");
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    col_units = table->cols[col].units;
    if (map->units != NULL && col_units != NULL && strcmp(map->units,col_units) != 0) {
      STATEMENT_ERROR(statement,"DATASET map units conflict with column units");
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    if (!DatasetCollectDomainRefs(map->target,statement,&refs,&ndims)) {
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
    map_prep[i].item = map;
    map_prep[i].col = (unsigned)col;
    map_prep[i].refs = refs;
    map_prep[i].ndims = ndims;
    for (d = 0; d < ndims; ++d) {
      struct dataset_index_runtime_t *idxmatch = NULL;
      symchar *setid = NULL;
      if (refs[d].set_name != NULL) {
        setid = SimpleNameIdPtr(refs[d].set_name);
      }
      if (setid == NULL) {
        STATEMENT_ERROR(statement,"DATASET target index must be a simple set name");
        MarkStatContext(statement,context_WRONG);
        ok = 0;
        goto cleanup;
      }
      idxmatch = DatasetFindIndexBySetName(indices,nindices,setid);
      if (idxmatch == NULL) {
        if (nexplicit_indices > 0) {
          STATEMENT_ERROR(statement,"DATASET target index not declared in INDEX items");
          MarkStatContext(statement,context_WRONG);
          ok = 0;
          goto cleanup;
        }
        if (nimplicit_sets == 0) {
          implicit_set_cap = 2;
          implicit_set_names = ASC_NEW_ARRAY(symchar *,implicit_set_cap);
        } else if (nimplicit_sets >= implicit_set_cap) {
          implicit_set_cap *= 2;
          implicit_set_names = (symchar **)ascrealloc(implicit_set_names,sizeof(symchar *) * implicit_set_cap);
        }
        if (nimplicit_sets > 0 && implicit_set_names[0] != setid) {
          STATEMENT_ERROR(statement,"DATASET implicit row-index mode supports only one undeclared index set");
          MarkStatContext(statement,context_WRONG);
          ok = 0;
          goto cleanup;
        }
        if (nimplicit_sets == 0) {
          implicit_set_names[nimplicit_sets++] = setid;
        }
      }
    }
    i++;
  }

  if (nexplicit_indices == 0 && nimplicit_sets > 0) {
    unsigned base = nindices;
    indices = (struct dataset_index_runtime_t *)ascrealloc(indices,sizeof(struct dataset_index_runtime_t) * (nindices + nimplicit_sets));
    for (i = 0; i < nimplicit_sets; ++i) {
      struct Name *setname;
      unsigned long row;
      unsigned pos = base + i;
      indices[pos].item = NULL;
      indices[pos].set_name = implicit_set_names[i];
      indices[pos].col = 0;
      indices[pos].kind = integer_set;
      indices[pos].implicit_row = 1;
      indices[pos].set = CreateEmptySet();
      indices[pos].domain.set = NULL;
      indices[pos].domain.kind = empty_set;
      indices[pos].domain.len = 0;
      indices[pos].int_fastpath = 0;
      indices[pos].int_first = 0;
      indices[pos].int_last = 0;

      for (row = 1; row <= table->nrows; ++row) {
        InsertInteger(indices[pos].set,(asc_intptr_t)row);
      }

      setname = CreateIdName(indices[pos].set_name);
      if (!TableAssignDomainSet(work,setname,indices[pos].set,statement)) {
        DestroyName(setname);
        MarkStatContext(statement,context_WRONG);
        ok = 0;
        goto cleanup;
      }
      DestroyName(setname);
      if (!TableInitDomainFromSet(indices[pos].set,statement,&indices[pos].domain)) {
        MarkStatContext(statement,context_WRONG);
        ok = 0;
        goto cleanup;
      }
      DatasetInitIndexFastPath(&indices[pos]);
      DestroySet(indices[pos].set);
      indices[pos].set = NULL;
    }
    nindices += nimplicit_sets;
  }

  /*
   * DATASET index assignment can define array index sets that were previously
   * unknown. Try an immediate expansion so map assignments can resolve
   * target instances in this same pass.
   */
  {
    int changed = 0;
    TryArrayExpansion(work,&changed);
  }

  if (dataset_profile) {
    double now = DatasetNowSeconds();
    t_expand += now - tmark;
    tmark = now;
  }

  for (i = 0; i < nmaps; ++i) {
    unsigned d;
    int fast_bind;
    maps[i].item = map_prep[i].item;
    maps[i].col = map_prep[i].col;
    maps[i].ndims = map_prep[i].ndims;
    maps[i].indices = ASC_NEW_ARRAY(struct dataset_index_runtime_t *,maps[i].ndims);
    maps[i].domains = ASC_NEW_ARRAY(struct table_domain_t,maps[i].ndims);
    maps[i].posbuf = ASC_NEW_ARRAY(unsigned long,maps[i].ndims);
    for (d = 0; d < maps[i].ndims; ++d) {
      symchar *setid = NULL;
      struct dataset_index_runtime_t *idxmatch;
      if (map_prep[i].refs[d].set_name != NULL) {
        setid = SimpleNameIdPtr(map_prep[i].refs[d].set_name);
      }
      if (setid == NULL) {
        STATEMENT_ERROR(statement,"DATASET target index must be a simple set name");
        MarkStatContext(statement,context_WRONG);
        ok = 0;
        goto cleanup;
      }
      idxmatch = DatasetFindIndexBySetName(indices,nindices,setid);
      if (idxmatch == NULL) {
        STATEMENT_ERROR(statement,"DATASET target index not declared in INDEX items");
        MarkStatContext(statement,context_WRONG);
        ok = 0;
        goto cleanup;
      }
      maps[i].indices[d] = idxmatch;
      maps[i].domains[d] = idxmatch->domain;
    }
    fast_bind = DatasetTryBindFastArray1D(work,statement,&maps[i]);
    if (fast_bind < 0) {
      pending = 1;
      maps[i].fast_array_1d = 0;
      maps[i].fast_array_inst = NULL;
    } else if (!fast_bind) {
      MarkStatContext(statement,context_WRONG);
      ok = 0;
      goto cleanup;
    }
  }

  if (dataset_profile) {
    double now = DatasetNowSeconds();
    t_map_prepare += now - tmark;
    tmark = now;
  }

  for (i = 0; i < table->nrows; ++i) {
    unsigned m;
    for (m = 0; m < nmaps; ++m) {
      unsigned d;
      unsigned long *pos = maps[m].posbuf;
      int assign_result;
      for (d = 0; d < maps[m].ndims; ++d) {
        if (maps[m].indices[d]->implicit_row) {
          pos[d] = i + 1;
        } else {
          CONST char *tok = table->rows[i].cells[maps[m].indices[d]->col];
          if (!DatasetLabelToPosition(maps[m].indices[d],tok,&pos[d],statement)) {
            MarkStatContext(statement,context_WRONG);
            ok = 0;
            goto cleanup;
          }
        }
      }
      {
        CONST char *units = maps[m].item->units;
        if (units == NULL) {
          units = table->cols[maps[m].col].units;
        }
        if (maps[m].fast_array_1d && maps[m].fast_array_inst != NULL && maps[m].ndims == 1) {
          unsigned long p = pos[0];
          struct Instance *cellinst = NULL;
          if (p == 0 || p > NumberChildren(maps[m].fast_array_inst)) {
            STATEMENT_ERROR(statement,"DATASET fast-path index position is out of array bounds");
            assign_result = 0;
          } else {
            cellinst = InstanceChild(maps[m].fast_array_inst,p);
            if (cellinst == NULL) {
              assign_result = -1;
            } else {
              assign_result = DatasetAssignTokenToInstance(cellinst,statement,table->rows[i].cells[maps[m].col],units);
            }
          }
        } else {
          assign_result = DatasetAssignValueMaybeWait(work,statement,maps[m].item,maps[m].domains,maps[m].ndims,pos,table->rows[i].cells[maps[m].col],units);
        }
      }
      if (assign_result < 0) {
        pending = 1;
      } else if (!assign_result) {
        MarkStatContext(statement,context_WRONG);
        ok = 0;
        goto cleanup;
      }
    }
  }

  if (dataset_profile) {
    double now = DatasetNowSeconds();
    t_assign += now - tmark;
    tmark = now;
    t_total = t_load + t_index + t_expand + t_map_prepare + t_assign;
    FPRINTF(ASCERR,
      "DATASET_PROFILE file='%s' cache_hit=%d rows=%u nindices=%u nmaps=%u load=%.6fs index=%.6fs expand=%.6fs map_prepare=%.6fs assign=%.6fs total=%.6fs\n",
      statement->v.dataset.filename ? statement->v.dataset.filename : "(null)",
      dataset_cache_hit,
      table->nrows,
      nindices,
      nmaps,
      t_load,
      t_index,
      t_expand,
      t_map_prepare,
      t_assign,
      t_total
    );
  }

cleanup:
  if (map_prep != NULL) {
    for (i = 0; i < nmaps; ++i) {
      if (map_prep[i].refs != NULL) {
        ascfree(map_prep[i].refs);
      }
    }
    ascfree(map_prep);
  }
  if (implicit_set_names != NULL) {
    ascfree(implicit_set_names);
  }
  if (maps != NULL) {
    for (i = 0; i < nmaps; ++i) {
      if (maps[i].indices != NULL) {
        ascfree(maps[i].indices);
      }
      if (maps[i].domains != NULL) {
        ascfree(maps[i].domains);
      }
      if (maps[i].posbuf != NULL) {
        ascfree(maps[i].posbuf);
      }
    }
    ascfree(maps);
  }
  if (indices != NULL) {
    for (i = 0; i < nindices; ++i) {
      TableDomainDestroy(&indices[i].domain);
      if (indices[i].set != NULL) {
        DestroySet(indices[i].set);
      }
    }
    ascfree(indices);
  }
  if (!ok) {
    return 0;
  }
  if (pending) {
    return 0;
  }
  return 1;
}

static int ExecuteTABLE(struct Instance *work, struct Statement *statement){
  struct table_domain_t domains[2];
  struct scalar_units_runtime_t units_runtime;
  CONST struct Name *node;
  unsigned ndims = 0;
  int rval = 1;
  char *bodycopy = NULL;
  char *line_ctx = NULL;
  char *line;
  unsigned long row_index = 1;
  unsigned long flat_index = 0;
  unsigned di;

  if (StatWrong(statement)) {
    return 1;
  }

  if (statement->v.table.name == NULL || !NameId(statement->v.table.name)) {
    STATEMENT_ERROR(statement,"TABLE target name is invalid");
    MarkStatContext(statement,context_WRONG);
    return 1;
  }
  if (!statement->v.table.positional) {
    return ExecuteTABLEDense(work,statement);
  }

  if (!ResolveScalarUnits(statement->v.table.units,statement,"TABLE",&units_runtime)) {
    MarkStatContext(statement,context_WRONG);
    return 1;
  }

  for (di = 0; di < 2; ++di) {
    domains[di].set = NULL;
    domains[di].kind = empty_set;
    domains[di].len = 0;
  }

  for (node = statement->v.table.name; node != NULL; node = NextName(node)) {
    if (!NameId(node)) {
      unsigned added = 0;
      if (ndims >= 2) {
        STATEMENT_ERROR(statement,"POSITIONAL TABLE currently supports at most 2 indices");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
      if (!TableEvaluateDomains(work,NameSetPtr(node),statement,&domains[ndims],2 - ndims,&added)) {
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
      ndims += added;
    }
  }
  if (ndims == 0) {
    STATEMENT_ERROR(statement,"TABLE target must include at least one index");
    MarkStatContext(statement,context_WRONG);
    goto cleanup;
  }

  if (statement->v.table.body != NULL) {
    size_t n = strlen(statement->v.table.body);
    bodycopy = ASC_NEW_ARRAY(char,n + 1);
    memcpy(bodycopy,statement->v.table.body,n + 1);
  } else {
    bodycopy = ASC_NEW_ARRAY(char,1);
    bodycopy[0] = '\0';
  }

  for (line = strtok_r(bodycopy,"\n",&line_ctx); line != NULL; line = strtok_r(NULL,"\n",&line_ctx)) {
    char *tok_ctx = NULL;
    char *tok;
    unsigned long col_index = 1;
    int row_has_values = 0;
    int delim_pending = 0;
    int last_delim = 0;
    int sign = 1;
    int sign_pending = 0;

    for (tok = strtok_r(line," \t\r",&tok_ctx); tok != NULL; tok = strtok_r(NULL," \t\r",&tok_ctx)) {
      int is_int;
      long ival;
      double rvalnum;
      unsigned long pos[2];

      if (strcmp(tok,";") == 0 || strcmp(tok,",") == 0) {
        if (sign_pending) {
          STATEMENT_ERROR(statement,"TABLE delimiter cannot follow a sign without a value");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        if (!row_has_values) {
          STATEMENT_ERROR(statement,"TABLE row cannot begin with a delimiter");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        if (delim_pending) {
          STATEMENT_ERROR(statement,"TABLE row has repeated delimiters without a value");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        delim_pending = 1;
        last_delim = tok[0];
        continue;
      }
      if (strcmp(tok,"+") == 0 || strcmp(tok,"-") == 0) {
        if (sign_pending) {
          STATEMENT_ERROR(statement,"Malformed TABLE value sign sequence");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        delim_pending = 0;
        last_delim = 0;
        sign = (tok[0] == '-') ? -1 : 1;
        sign_pending = 1;
        continue;
      }
      if (strcmp(tok,":") == 0 || strcmp(tok,"=") == 0) {
        STATEMENT_ERROR(statement,"Unexpected sparse-token punctuation in POSITIONAL TABLE");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }

      if (!TableParseNumberToken(tok,sign_pending ? sign : 1,&is_int,&ival,&rvalnum)) {
        STATEMENT_ERROR(statement,"POSITIONAL TABLE contains non-numeric token");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
      delim_pending = 0;
      last_delim = 0;
      sign = 1;
      sign_pending = 0;
      row_has_values = 1;

      if (ndims == 1) {
        flat_index++;
        if (flat_index > domains[0].len) {
          STATEMENT_ERROR(statement,"Too many TABLE values for 1-D target");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        pos[0] = flat_index;
        if (!TableAssignCell(work,statement,domains,1,pos,&units_runtime,is_int,ival,rvalnum)) {
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
      } else {
        if (row_index > domains[0].len) {
          STATEMENT_ERROR(statement,"Too many TABLE rows for first index set");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        if (col_index > domains[1].len) {
          STATEMENT_ERROR(statement,"Too many TABLE columns for second index set");
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        pos[0] = row_index;
        pos[1] = col_index;
        if (!TableAssignCell(work,statement,domains,2,pos,&units_runtime,is_int,ival,rvalnum)) {
          MarkStatContext(statement,context_WRONG);
          goto cleanup;
        }
        col_index++;
      }
    }

    if (sign_pending) {
      STATEMENT_ERROR(statement,"Dangling sign token at end of TABLE row");
      MarkStatContext(statement,context_WRONG);
      goto cleanup;
    }
    if (delim_pending) {
      if (last_delim != ';') {
        STATEMENT_ERROR(statement,"TABLE row cannot end with a delimiter");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
    }
    if (!row_has_values) {
      continue;
    }
    if (ndims == 2) {
      if (col_index - 1 != domains[1].len) {
        STATEMENT_ERROR(statement,"TABLE row has incorrect number of columns");
        MarkStatContext(statement,context_WRONG);
        goto cleanup;
      }
      row_index++;
    }
  }

  if (ndims == 1) {
    if (flat_index != domains[0].len) {
      STATEMENT_ERROR(statement,"TABLE value count does not match index cardinality");
      MarkStatContext(statement,context_WRONG);
      goto cleanup;
    }
  } else {
    if (row_index - 1 != domains[0].len) {
      STATEMENT_ERROR(statement,"TABLE row count does not match first index cardinality");
      MarkStatContext(statement,context_WRONG);
      goto cleanup;
    }
  }
  rval = 1;
  goto cleanup;

cleanup:
  if (bodycopy != NULL) {
    ascfree(bodycopy);
  }
  for (di = 0; di < 2; ++di) {
    TableDomainDestroy(&domains[di]);
  }
  return rval;
}


/**
	Execute structural and dimensional assignments.
	This is called by execute statements and exec for statements.
	Assignments to variable types are ignored.
	Variable defaults expressions are done in executedefaults.
	rhs expressions must yield constant value_t.
	Incorrect statements will be marked context_WRONG where possible.
*/
static int ExecuteCASGN(struct Instance *work, struct Statement *statement){
  struct gl_list_t *instances;
  struct Instance *inst;
  unsigned long c,len;
  struct value_t value;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  int previous_context;
  int rval;

  if (StatWrong(statement)) return 1; /* if we'll never execute it, it's ok */

  previous_context = GetDeclarativeContext();
  SetDeclarativeContext(0);
  instances = FindInstances(work,AssignStatVar(statement),&err);
  if (instances != NULL){
    asc_assert(GetEvaluationContext()==NULL);
    SetEvaluationContext(work);

#ifdef ASC_SIGNAL_TRAPS
	Asc_SignalHandlerPushDefault(SIGFPE);
	if(SETJMP(g_fpe_env)==0){
#endif

	value = EvaluateExpr(AssignStatRHS(statement),NULL,InstanceEvaluateName);

#ifdef ASC_SIGNAL_TRAPS
	}else{
		STATEMENT_ERROR(statement, "Floating-point error while evaluating assignment statement");
        MarkStatContext(statement,context_WRONG);
		gl_destroy(instances);
		SetEvaluationContext(NULL);
		Asc_SignalHandlerPopDefault(SIGFPE);
		SetDeclarativeContext(previous_context);
		return 1;
	}
	Asc_SignalHandlerPopDefault(SIGFPE);
#endif

    SetEvaluationContext(NULL);


    if (ValueKind(value)==error_value || !IsConstantValue(value) ){
      if (ValueKind(value)==error_value) {
        gl_destroy(instances);
        SetDeclarativeContext(previous_context);
        rval = AsgnErrorReport(statement,&value);
        if (rval) {
          MarkStatContext(statement,context_WRONG);
          WSS(ASCERR,statement);
        }
        return rval;
      }else{
        gl_destroy(instances);
        SetDeclarativeContext(previous_context);
        StructuralAsgnErrorReport(statement,&value);
        STATEMENT_ERROR(statement, "Assignment is impossible");
        MarkStatContext(statement,context_WRONG);
        WSS(ASCERR,statement);
        return 1;
      }
    }else{
      /* good rhs value, but may be mismatched to set ATOM */
      len = gl_length(instances);
      for(c=1;c<=len;c++){
        inst = (struct Instance *)gl_fetch(instances,c);
        if (!AssignStructuralValue(inst,value,statement)) {
          MarkStatContext(statement,context_WRONG);
          STATEMENT_ERROR(statement, "Assignment is impossible (wrong set type)");
          WSS(ASCERR,statement);
        }
      }
      DestroyValue(&value);
      gl_destroy(instances);
      SetDeclarativeContext(previous_context);
      return 1;
    }
  }else{
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance:
      STATEMENT_ERROR(statement, "Left hand side of assignment statement"
            " contains an impossible instance");
      SetDeclarativeContext(previous_context);
      return 1;
    default:			/* unmade instances or something */
      SetDeclarativeContext(previous_context);
      return 0;
    }
  }
}

/*------------------------------------------------------------------------------
  CHECK ROUTINES
*/

/**
	Returns 1 if name can be found in name, or 0 OTHERWISE.
	only deals well with n and sub being Id names.
*/
static int NameContainsName(CONST struct Name *n,CONST struct Name *sub){
  struct gl_list_t *nl;
  unsigned long c,len;
  struct Expr *en;

  asc_assert(n!=NULL);
  asc_assert(sub!=NULL);
  en = ASC_NEW(struct Expr);
  InitVarExpr(en,n);
  nl = EvaluateNamesNeededShallow(en,NULL,NULL);
  /* should this function be checking deep instead? can't tell yet. */
  if (nl==NULL || gl_length(nl)==0) {
    ascfree(en);
    return 0; /* should never happen */
  }
  for (c=1, len = gl_length(nl); c <= len; c++) {
    if (CompareNames((struct Name *)gl_fetch(nl,c),sub)==0) {
      gl_destroy(nl);
      ascfree(en);
      return 1;
    }
  }
  gl_destroy(nl);
  ascfree(en);
  return 0;
}

/**
	Checks that the namelist, less any components that contain arrsetname,
	can be evaluated to constant values.
	Returns 1 if it can be evaluated.

	This is heuristic. It can fail in very very twisty circumstances.
	What saves the heuristic is that usually all the other conditions
	on the compound ALIASES (that rhs's must exist and so forth) will
	be satisfied before this check is performed and that that will mean
	enough structure to do the job at Execute time will be in place even
	if this returns a FALSE positive.

	Basically to trick this thing you have to do indirect addressing with
	the set elements of the IS_A set in declaring the lhs of the ALIASES
	part. Of course if you really do that sort of thing, you should be
	coding in C++ or F90 anyway.

	What it comes down to is that this array constructor from diverse
	elements really sucks -- but so does varargs and that's what we're
	using the compound alias array constructor to implement.

	There is an extremely expensive alternative that is not heuristic --
	create the IS_A set (which might be a sparse array) during the
	check process and blow it away when the check fails. This is an
	utter nuisance and a cost absurdity.
	--baa 1/97.
*/
static int ArrayCheckNameList(struct Instance *inst
	, struct Statement *statement, struct gl_list_t *nl
	, CONST struct Name *arrsetname
){
  unsigned long c,len,i,ilen;
  struct Instance *fi;
  CONST struct Name *n;
  struct gl_list_t *il;
  symchar *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  len = gl_length(nl);
  if (len==0) {
    return 1;
  }
  for (c=1; c <= len; c++) {
    n = (struct Name *)gl_fetch(nl,c);
    if (NameContainsName(n,arrsetname) == 0 ) {
      name = SimpleNameIdPtr(n);
      if (name !=NULL && StatInFOR(statement) &&
          FindForVar(GetEvaluationForTable(),name)!=NULL) {
        continue;
      }
      /* else hunt up the instances */
      il = FindInstances(inst,n,&err);
      if (il == NULL) {
        return 0;
      }
      for (i=1, ilen=gl_length(il); i <=ilen; i++) {
        fi = (struct Instance *)gl_fetch(il,i);
        switch(InstanceKind(fi)) {
        case SET_ATOM_INST:
        case INTEGER_CONSTANT_INST:
        case SYMBOL_CONSTANT_INST:
          if (AtomAssigned(fi)==0) {
            gl_destroy(il);
            return 0;
          }
          break;
        case MODEL_INST:
        case ARRAY_INT_INST:
        case ARRAY_ENUM_INST:
          /* ok, it was found. odd, that, but it might be ok */
          break;
        /* fundamental, variable, relation, when, logrel, realcon, boolcon
         * can none of them figure in the definition of valid set.
         * so we exit early and execution will fail as required.
         */
        default:
          gl_destroy(il);
          return 1;
        }
      }
    }
  }
  return 1;
}
/*
	check the subscripts for definedness, including FOR table checks and
	checks for the special name in the compound ALIASES-IS_A statement.
	Assumes it is going to be handed a name consisting entirely of
	subscripts.
*/
static int FailsCompoundArrayCheck(struct Instance *inst
	,CONST struct Name *name, struct Statement *statement
	,CONST struct Name *arrsetname
){
  struct gl_list_t *nl;
  CONST struct Set *sptr;
  int ok;

  while(name != NULL){
    /* foreach subscript */
    if (NameId(name)!=0){ /* what's a . doing in the name? */
      return 1;
    }
    sptr = NameSetPtr(name);
    nl = EvaluateSetNamesNeeded(sptr,NULL);
    if (nl !=  NULL) {
      ok = ArrayCheckNameList(inst,statement,nl,arrsetname);
      gl_destroy(nl);
      if (ok == 0 ) {
        return 1;
      }
    }else{
      return 1;
    }
    name = NextName(name);
  }
  return 0;
}

/**
	The name pointer is known to be an array, so now it is checked to make
	sure that each index type can be determined.
	It is not a . qualified name.

	With searchfor == 0:
	This routine deliberately lets some errors through because the will
	be trapped elsewhere.  Its *only* job is to detect undefined index
	types. (defined indices simply missing values will merely be done
	in a later array expansion.
	Returns 1 if set type indeterminate.

	With searchfor != 0:
	Tries to expand the indices completely and returns 1 if fails.
	arrset name is a special name that may be used in indices when
	creating compound ALIASES-IS_A -- it is the name the IS_A will create.
	It is only considered if searchfor != 0.
*/
static
int FailsIndexCheck(CONST struct Name *name, struct Statement *statement,
                    struct Instance *inst, CONST unsigned int searchfor,
                    CONST struct Name *arrsetname)
{
  CONST struct Set *sptr;
  struct gl_list_t *indices;
  if (!NameId(name)) {
     return 0;	/* this is a different type of error */
  }
  /* hunt the subscripts */
  name = NextName(name);
  if (name == NULL) {
    return 0;	/* this is a different type of error */
  }
  if (searchfor == 0) { /* not in FOR loop and not ALIASES of either sort */
    while (name != NULL){
      if (NameId(name) !=0 ) {
        /* what's a . doing here? */
        return 0;
      }
      sptr = NameSetPtr(name);
      if (DeriveSetType(sptr,inst,0) < 0) {
        return 1; /* confusion reigns */
      }
      name = NextName(name);
    }
  }else{
    asc_assert(statement!=NULL);
    if (arrsetname == NULL) {
      /* sparse IS_A or ALIASES but not ALIASES/IS_A */
      indices = MakeIndices(inst,name,statement);
      if (indices != NULL) {
        DestroyIndexList(indices);
        return 0;
      }else{
        return 1;
      }
    }else{
      /* sparse or dense ALIASES-IS_A where we have to handle a
       * special name we
       * can't tell the value of yet because the IS_A hasn't been
       * compiled.
       */
      return FailsCompoundArrayCheck(inst,name,statement,arrsetname);
    }
  }
  return 0;
}

/**
	This has to check this member of the variable list for unknown
	array indices.  It returns TRUE iff it contains an unknown index;
	otherwise, it returns FALSE.
	If searchfor !=0, include for indices in list of valid things,
	and insist that values actually have been assigned as well.
*/
static
int ContainsUnknownArrayIndex(struct Instance *inst,
                              struct Statement *stat,
                              CONST struct Name *name,
                              CONST unsigned int searchfor,
                              CONST struct Name *arrsetname)
{
  if (!SimpleNameIdPtr(name)){ /* simple names never miss indices */
    if (FailsIndexCheck(name,stat,inst,searchfor,arrsetname)) return 1;
  }
  return 0;
}

/**
	If there are no array instances, this should always return TRUE.  When
	there are array instances to be created, it has to check to make sure
	that all of the index types can be determined and their values are
	defined!

	aliases always appears to be in for loop because we must always have
	a definition of all the sets because an alias array can't be finished
	up later.
*/
static int CheckALIASES(struct Instance *inst, struct Statement *stat){
  CONST struct VariableList *vlist;
  int cu;
  struct gl_list_t *rhslist;
  CONST struct Name *name;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  vlist = GetStatVarList(stat);
  while (vlist != NULL){
    cu = ContainsUnknownArrayIndex(inst,stat,NamePointer(vlist),1,NULL);
    if (cu) {
      return 0;
    }
    vlist = NextVariableNode(vlist);
  }

  /*
   * Checking the existence of the rhs in the aliases statement
   */
  name = AliasStatName(stat);
  rhslist = FindInstances(inst,name,&err);
  if (rhslist == NULL) {
    WriteUnexecutedMessage(ASCERR,stat,
      "Possibly undefined right hand side in ALIASES statement.");
    return 0; /* rhs not compiled yet */
  }
  if (gl_length(rhslist)>1) {
    STATEMENT_ERROR(stat,"ALIASES needs exactly 1 RHS");
  }
  gl_destroy(rhslist);

  return 1;
}

/**
	This has to make sure the RHS list of the ALIASES and the WITH_VALUE
	of the IS_A are both satisfied.

	When the statement is in a FOR loop, this has to check to make sure
	that all of the LHS index types can be determined and their values are
	defined!
	ALIASES always appears to be in for loop because we must always have
	a definition of all the sets because an alias array can't be finished
	up later.
*/
static int CheckARR(struct Instance *inst, struct Statement *stat){
  CONST struct VariableList *vlist;
  struct value_t value;
  int cu;

  asc_assert(StatementType(stat)==ARR);

  /* check subscripts on IS_A portion lhs. all mess should be in fortable */
  cu = ContainsUnknownArrayIndex(inst,
                                 stat,
                                 NamePointer( ArrayStatSetName(stat)),
                                 1,
                                 NULL);
  if(cu != 0){
    return 0;
  }
  /* check ALIASES portion lhs list */
  vlist = ArrayStatAvlNames(stat);
  while(vlist != NULL){
    cu = ContainsUnknownArrayIndex(inst,
                                   stat,
                                   NamePointer(vlist),
                                   1,
                                   NamePointer(ArrayStatSetName(stat)));
    if(cu != 0){
      return 0;
    }
    vlist = NextVariableNode(vlist);
  }
  /* check ALIASES portion rhs (list of instances collecting to an array) */
  if(CheckVarList(inst,stat)==0){
    return 0;
  }
  /* check IS_A WITH_VALUE list */
  if(ArrayStatSetValues(stat)!=NULL){
    asc_assert(GetEvaluationContext()==NULL);
    SetEvaluationContext(inst);
    value = EvaluateSet(ArrayStatSetValues(stat),InstanceEvaluateName);
    SetEvaluationContext(NULL);
    switch(ValueKind(value)){
    case list_value:
      /* set may be garbage, in which case execute will whine */
      break;
    case error_value:
      switch(ErrorValue(value)){
      case name_unfound:
      case undefined_value:
        DestroyValue(&value);
        return 0;
      default:
        instantiation_error(ASC_USER_ERROR,stat
			,"Compound alias instance has incorrect index type.\n"
		);
        break;
      }
      break;
    default:
      instantiation_error(ASC_USER_ERROR,stat
			,"Compound alias instance has incorrect index value type."
	  );
      break;
    }
    DestroyValue(&value);
  }
  return 1;
}

/**
	If there are no array instances, this should always return TRUE.  When
	there are array instances to be created, it has to check to make sure
	that all of the index types can be determined.
	If statement requires type args, also checks that all array indices
	can be evaluated.

	Currently, this can handle checking for completable sets in any
	statement's var list, not just ISAs.

	It does not at present check arguments of IS_A's.
*/
static int CheckISA(struct Instance *inst, struct Statement *stat){
  CONST struct VariableList *vlist;
  int cu;
  unsigned int searchfor;
  if (StatWrong(stat)) return 1; /* if we'll never execute it, it's ok */
  searchfor = ( StatInFOR(stat)!=0 ||
                GetStatNeedsArgs(stat) > 0 ||
                StatModelParameter(stat)!=0 );
  vlist = GetStatVarList(stat);
  while(vlist != NULL){
    cu =
      ContainsUnknownArrayIndex(inst,stat,NamePointer(vlist),searchfor,NULL);
    if(cu){
      return 0;
    }
    vlist = NextVariableNode(vlist);
  }
  return 1;
}

static int IsSelectorTypeDesc(CONST struct TypeDescription *desc)
{
  symchar *selector_name = AddSymbol("selector");
  while (desc != NULL) {
    if (GetName(desc) == selector_name) {
      return 1;
    }
    desc = GetRefinement(desc);
  }
  return 0;
}

static CONST struct set_t *ResolveSelectorDomain(struct Instance *scope,
                                                 struct Statement *statement)
{
  struct Name *domain_name;
  struct gl_list_t *instances;
  struct Instance *domain;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  symchar *domain_id;

  domain_id = GetStatSetType(statement);
  if (domain_id == NULL) {
    STATEMENT_ERROR(statement,"Selector declaration is missing a domain set");
    return NULL;
  }

  domain_name = CreateIdName(domain_id);
  instances = FindInstances(scope,domain_name,&err);
  DestroyName(domain_name);
  if (instances == NULL || gl_length(instances) != 1) {
    if (instances != NULL) gl_destroy(instances);
    STATEMENT_ERROR(statement,"Unable to resolve selector domain set");
    return NULL;
  }

  domain = (struct Instance *)gl_fetch(instances,1);
  gl_destroy(instances);
  if (domain == NULL || InstanceKind(domain) != SET_ATOM_INST) {
    STATEMENT_ERROR(statement,"Selector domain must be a set instance");
    return NULL;
  }

  return SetAtomList(domain);
}

static int AssignISAInitialValue(struct Instance *scope,
                                 struct Statement *statement,
                                 struct Instance *target)
{
  struct value_t value;
  CONST struct set_t *domain = NULL;
  int previous_context;

  if (GetStatCheckValue(statement) == NULL) {
    return 1;
  }

  previous_context = GetDeclarativeContext();
  SetDeclarativeContext(0);
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(scope);
  value = EvaluateExpr(GetStatCheckValue(statement),NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  SetDeclarativeContext(previous_context);

  if (ValueKind(value)==error_value) {
    switch(ErrorValue(value)){
    case undefined_value:
    case name_unfound:
      DestroyValue(&value);
      return 0;
    default:
      DestroyValue(&value);
      STATEMENT_ERROR(statement,"Unable to evaluate declaration DEFAULT expression");
      return 0;
    }
  }

  if (IsSelectorTypeDesc(InstanceTypeDesc(target))) {
    domain = ResolveSelectorDomain(scope,statement);
    if (domain == NULL) {
      DestroyValue(&value);
      return 0;
    }
    switch (ValueKind(value)) {
    case symbol_value:
      if (!StrMember(SymbolValue(value),domain)) {
        DestroyValue(&value);
        STATEMENT_ERROR(statement,"DEFAULT selector value is not a member of the selector domain");
        return 0;
      }
      break;
    case integer_value:
      if (!IntMember((asc_intptr_t)IntegerValue(value),domain)) {
        DestroyValue(&value);
        STATEMENT_ERROR(statement,"DEFAULT selector value is not a member of the selector domain");
        return 0;
      }
      break;
    default:
      DestroyValue(&value);
      STATEMENT_ERROR(statement,"DEFAULT selector value must match the selector domain type");
      return 0;
    }
  }

  switch(InstanceKind(target)) {
  case SYMBOL_ATOM_INST:
    if (ValueKind(value) != symbol_value) {
      DestroyValue(&value);
      STATEMENT_ERROR(statement,"DEFAULT value for symbol declaration must be a symbol");
      return 0;
    }
    SetSymbolAtomValue(target, SymbolValue(value));
    break;
  case INTEGER_ATOM_INST:
    if (ValueKind(value) != integer_value) {
      DestroyValue(&value);
      STATEMENT_ERROR(statement,"DEFAULT value for integer declaration must be an integer");
      return 0;
    }
    SetIntegerAtomValue(target, IntegerValue(value), 0);
    break;
  case BOOLEAN_ATOM_INST:
    if (ValueKind(value) != boolean_value) {
      DestroyValue(&value);
      STATEMENT_ERROR(statement,"DEFAULT value for boolean declaration must be boolean");
      return 0;
    }
    SetBooleanAtomValue(target, BooleanValue(value), 0);
    break;
  default:
    DestroyValue(&value);
    STATEMENT_ERROR(statement,"DEFAULT is only supported on scalar atomic declarations");
    return 0;
  }

  DestroyValue(&value);
  return 1;
}

/**
	checks that all the names in a varlist exist as instances.
	returns 1 if TRUE, 0 if not.
*/
static int CheckVarList(struct Instance *inst, struct Statement *statement){
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  int instances;
  instances = VerifyInsts(inst,GetStatVarList(statement),&err);
  if(instances){
    return 1;
  }else{
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance: return 1;
    default: return 0;
    }
  }
}

static int CheckIRT(struct Instance *inst, struct Statement *statement){
  if (FindType(GetStatType(statement))==NULL) return 1;
  return CheckVarList(inst,statement);
}

static int CheckATS(struct Instance *inst, struct Statement *statement){
  return CheckVarList(inst,statement);
}

static int CheckAA(struct Instance *inst, struct Statement *statement){
  return CheckVarList(inst,statement);
}

static int CheckLNK(struct Instance *inst, struct Statement *statement){
  return CheckVarList(inst,statement);
}

/**
	Checks that the lhs of an assignment statement expands into
	a complete set of instances.
	Not check that the first of those instances is type compatible with
	the value being assigned.
*/
static int CheckCASGN(struct Instance *inst, struct Statement *statement){
  struct gl_list_t *instances;
  struct value_t value;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  instances = FindInstances(inst,AssignStatVar(statement),&err);
  if (instances != NULL){
    gl_destroy(instances);
    asc_assert(GetEvaluationContext()==NULL);
    SetEvaluationContext(inst);
    value = EvaluateExpr(AssignStatRHS(statement),NULL,
                         InstanceEvaluateName);
    SetEvaluationContext(NULL);
    if (ValueKind(value)==error_value){
      switch(ErrorValue(value)){
      case undefined_value:
      case name_unfound:
        DestroyValue(&value);
        return 0;
      default: /* it is a question whether this is a correct action */
        break; /* should we handle other error classes? */
      }
    }
    DestroyValue(&value);
    return 1; /* everything is okay */
  }else{
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance: return 1;
    default:
      return 0;
    }
  }
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
int CheckASGN(struct Instance *inst, struct Statement *statement)
{
  struct gl_list_t *instances;
  struct value_t value;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  instances = FindInstances(inst,DefaultStatVar(statement),&err);
  if (instances != NULL){
    gl_destroy(instances);
    asc_assert(GetEvaluationContext()==NULL);
    SetEvaluationContext(inst);
    value = EvaluateExpr(DefaultStatRHS(statement),NULL,
                         InstanceEvaluateName);
    SetEvaluationContext(NULL);
    if (ValueKind(value)==error_value){
      switch(ErrorValue(value)){
      case undefined_value:
      case name_unfound:
        DestroyValue(&value);
        return 0;
      default: /* it is a question whether this is a correct action */
        break; /* should we handle other error classes? */
      }
    }
    DestroyValue(&value);
    return 1;			/* everything is okay */
  }
  else{
    switch(rel_errorlist_get_find_error(&err)){
    case impossible_instance: return 1;
    default:
      return 0;
    }
  }
}
#endif   /* THIS_IS_AN_UNUSED_FUNCTION */


/**
	Check if the relation exists, also, if it exists as relation or as a
	dummy instance. return -1 for DUMMY. 1 for relation. 0 if the checking
	fails.
*/
static int CheckRelName(struct Instance *work, struct Name *name){
  struct gl_list_t *instances;
  struct Instance *inst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  instances = FindInstances(work,name,&err);
  if(instances==NULL){
    return 1;
  }else{
    if (gl_length(instances)==1){
      inst = (struct Instance *)gl_fetch(instances,1);
      asc_assert((InstanceKind(inst)==REL_INST) || (InstanceKind(inst)==DUMMY_INST));
      gl_destroy(instances);
      if (InstanceKind(inst)==DUMMY_INST) {
        return -1;
      }
      return 1;
    }else{
      gl_destroy(instances);
      return 0;
    }
  }
}

/*
 * If the relation is already there, it may be a dummy instance. In
 * such a case, do not check the expression.
 */
static int CheckREL(struct Instance *inst, struct Statement *statement){
  int status;
  status = CheckRelName(inst,RelationStatName(statement));
  if (status == 0) {
    return 0;
  }
  if ( status == -1) {
    return 1;
  }
  return CheckRelation(inst,RelationStatExpr(statement));
}
/*
 * If the external relation is already there,
 * it may be a dummy instance. In
 * such a case, do not check the args.
 */
static int CheckEXT(struct Instance *inst, struct Statement *statement){
  int status;
  status = CheckRelName(inst,ExternalStatNameRelation(statement));
  if (status == 0) {
    return 0;
  }
  if ( status == -1) {
    return 1;
  }
  return CheckExternal(inst,
                       ExternalStatVlistRelation(statement),
                       ExternalStatDataBlackBox(statement));
}

/***********************************************************************/

/* Check that the logical relation instance of some name has not been
 * previously created, or if it has, the instance is unique and
 * corresponds to a logical relation or to a dummy.
 * return -1 for DUMMY. 1 for log relation. 0 if the checking fails.
 */
static int CheckLogRelName(struct Instance *work, struct Name *name){
  struct gl_list_t *instances;
  struct Instance *inst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  instances = FindInstances(work,name,&err);
  if (instances==NULL){
    return 1;
  }else{
    if (gl_length(instances)==1){
      inst = (struct Instance *)gl_fetch(instances,1);
      asc_assert((InstanceKind(inst)==LREL_INST) || (InstanceKind(inst)==DUMMY_INST));
      gl_destroy(instances);
      if (InstanceKind(inst)==DUMMY_INST) {
        return -1;
      }
      return 1;
    }else {
      gl_destroy(instances);
      return 0;
    }
  }
}

/**
	Checking of Logical relation. First the name, then the expression.
	If the logrel exists as a dummy, then do not check the expression.
	Currently not in use.
*/
static int CheckLOGREL(struct Instance *inst, struct Statement *statement){
  if (!CheckLogRelName(inst,LogicalRelStatName(statement)))
    return 0;
  if ( CheckLogRelName(inst,LogicalRelStatName(statement)) == -1)
    return 1;
  return CheckLogRel(inst,LogicalRelStatExpr(statement));
}


/**
	Checking FNAME statement (1)

	["The following two functions..." -- ed] Check that the FNAME inside a WHEN
	make reference to instance of models, relations, or arrays of
	models or relations previously created.
*/
static int CheckArrayRelMod(struct Instance *child){
  struct Instance *arraychild;
  unsigned long len,c;
  switch (InstanceKind(child)) {
    case REL_INST:
    case LREL_INST:
    case MODEL_INST:
      return 1;
    case ARRAY_INT_INST:
    case ARRAY_ENUM_INST:
      len = NumberChildren(child);
      for(c=1;c<=len;c++){
        arraychild = InstanceChild(child,c);
        if (!CheckArrayRelMod(arraychild)){
          return 0;
        }
      }
      return 1;
    default:
      FPRINTF(ASCERR,
      "Incorrect array instance name inside a WHEN statement\n");
      return 0;
  }
}

/**
	Checking FNAME statement (2)

	Check that the FNAME inside a WHEN
	make reference to instance of models, relations, or arrays of
	models or relations previously created.
*/
static int CheckRelModName(struct Instance *work, struct Name *name){
  struct gl_list_t *instances;
  struct Instance *inst, *child;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  unsigned long len,c;
  instances = FindInstances(work,name,&err);
  if (instances==NULL){
    instantiation_name_error(ASC_USER_ERROR,name,
		"Un-made Relation/Model instance inside a 'WHEN':"
	);
    gl_destroy(instances);
    return 0;
  }else{
    if (gl_length(instances)==1){
     inst = (struct Instance *)gl_fetch(instances,1);
     switch (InstanceKind(inst)) {
     case REL_INST:
     case LREL_INST:
     case MODEL_INST:
       gl_destroy(instances);
       return 1;
     case ARRAY_INT_INST:
     case ARRAY_ENUM_INST:
      len = NumberChildren(inst);
      for(c=1;c<=len;c++){
        child = InstanceChild(inst,c);
        if (!CheckArrayRelMod(child)){
          gl_destroy(instances);
          return 0;
        }
      }
      gl_destroy(instances);
      return 1;
     default:
       instantiation_name_error(ASC_USER_ERROR,name
			,"Incorrect instance name (no Model/Relation) inside 'WHEN'"
	   );
       gl_destroy(instances);
       return 0;
     }
    }else{
    instantiation_name_error(ASC_USER_ERROR,name
		,"Error in 'WHEN'. Name assigned to more than one instance type"
	);
    gl_destroy(instances);
    return 0;
    }
  }
}

/**
	A FNAME statement stands for a relation, model, or an array of models
	or relations. This checking is to make sure that those instance
	were already created
*/
static int CheckFNAME(struct Instance *inst, struct Statement *statement){
  if(!CheckRelModName(inst,FnameStat(statement))){
    return 0;
  }else{
    return 1;
  }
}

/**
	Only logrelations and FOR loops of logrelations are allowed inside a
	conditional statement in Pass3. This function ask for recursively
	checking these statements
*/
static int Pass3CheckCondStatements(struct Instance *inst
	, struct Statement *statement
){
  asc_assert(inst&&statement);
  switch(StatementType(statement)){
    case LOGREL:
      return CheckLOGREL(inst,statement);
    case FOR:
      return Pass3RealCheckFOR(inst,statement);
    case REL:
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case LNK:
    case UNLNK:
    case CALL:
    case EXT:
    case ASGN:
    case CASGN:
    case COND:
    case WHEN:
    case FNAME:
    case SELECT:
    case TABLESTAT:
    case DATASETSTAT:
      STATEMENT_ERROR(statement,
        "Statement not allowed inside a CONDITIONAL statement\n");
      return 0;
    default:
      STATEMENT_ERROR(statement,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
      return 1;
    }
}

/**
	Checking the statement list inside a CONDITIONAL statement in Pass3
*/
static int Pass3CheckCOND(struct Instance *inst, struct Statement *statement){
  struct StatementList *sl;
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  sl = CondStatList(statement);
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
     stat = (struct Statement *)gl_fetch(list,c);
     if (!Pass3CheckCondStatements(inst,stat)) return 0;
  }
  return 1;
}


/**
	Only relations and FOR loops of relations are allowed inside a
	conditional statement in Pass2. This function ask for recursively
	checking these statements
*/
static int Pass2CheckCondStatements(struct Instance *inst
	,struct Statement *statement
){
  asc_assert(inst&&statement);
  switch(StatementType(statement)){
    case REL:
      return CheckREL(inst,statement);
    case EXT:
      return CheckEXT(inst,statement);
    case FOR:
      return Pass2RealCheckFOR(inst,statement);
    case LOGREL:
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case LNK:
    case UNLNK:
    case CALL:
    case ASGN:
    case CASGN:
    case COND:
    case WHEN:
    case FNAME:
    case SELECT:
    case TABLESTAT:
    case DATASETSTAT:
         STATEMENT_ERROR(statement,
               "Statement not allowed inside a CONDITIONAL statement\n");
         return 0;
    default:
      STATEMENT_ERROR(statement,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
      return 1;
    }
}

/**
	Checking the statement list inside a CONDITIONAL statement in Pass2
*/
static int Pass2CheckCOND(struct Instance *inst, struct Statement *statement){
  struct StatementList *sl;
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  sl = CondStatList(statement);
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
     stat = (struct Statement *)gl_fetch(list,c);
     if (!Pass2CheckCondStatements(inst,stat)) return 0;
  }
  return 1;
}

/**
	Checking that not other instance has been created with the same
	name of the current WHEN. If it has, it has to be a WHEN or a
	DUMMY. return -1 for DUMMY. 1 for WHEN. 0 if the checking fails.
*/
static int CheckWhenName(struct Instance *work, struct Name *name){
  struct gl_list_t *instances;
  struct Instance *inst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  instances = FindInstances(work,name,&err);
  if (instances==NULL){
    return 1;
  }else{
    if (gl_length(instances)==1){
      inst = (struct Instance *)gl_fetch(instances,1);
      asc_assert((InstanceKind(inst)==WHEN_INST)||(InstanceKind(inst)==DUMMY_INST) );
      gl_destroy(instances);
      if (InstanceKind(inst)==DUMMY_INST) {
        return -1;
      }
      return 1;
    }else {
      gl_destroy(instances);
      return 0;
    }
  }
}

/**
	p1 and p2 are pointers to arrays of integers. Here we are checking
	that the type (integer, boolean, symbol) of each variable in the
	variable list of a WHEN (or a SELECT) is the same as the type of
	each value in the list of values a CASE
*/
static int CompListInArray(unsigned long numvar, int *p1, int *p2){
  unsigned long c;
  for (c=1;c<=numvar;c++) {
    if (*p2 != 3) { /* To account for ANY */
      if (*p1 != *p2) return 0;
    }
    if (c < numvar) {
      p1++;
      p2++;
    }
  }
  return 1;
}


/**
	Checking that the values of the set of values of each CASE of a
	WHEN statement are appropriate. This is, they
	are symbol, integer or boolean. The first part of the
	function was written for the case of WHEN statement
	inside a FOR loop. This function also sorts
	the kinds of values in the set by assigning a value
	to the integer *p2
*/
static int CheckWhenSetNode(struct Instance *ref
	,CONST struct Expr *expr, int *p2
){
  symchar *str;
  struct for_var_t *fvp;
  struct Set *set;
  CONST struct Expr *es;
  switch (ExprType(expr)) {
  case e_boolean:
    if (ExprBValue(expr)==2) {
      *p2 = 3;  /*  ANY */
    }else{
      *p2=1;
    }
    return 1;
  case e_int:
    *p2=0;
    return 1;
  case e_symbol:
    *p2=2;
    return 1;
  case e_var:
    if ((GetEvaluationForTable() != NULL) &&
        (NULL != (str = SimpleNameIdPtr(ExprName(expr)))) &&
        (NULL != (fvp=FindForVar(GetEvaluationForTable(),str)))) {
      if (GetForKind(fvp)==f_integer){
        *p2=0;
        return 1;
      }else{
        if (GetForKind(fvp)==f_symbol){
          *p2=2;
          return 1;
        }else{
	  	  instantiation_name_error(ASC_USER_ERROR,ExprName(expr)
			,"Inappropriate index in the list of values of a CASE in a 'WHEN'\n"
			"(only symbols or integers are allowed)"
		  );
	  return 0;
	}
      }
    }else{
		instantiation_name_error(ASC_USER_ERROR,ExprName(expr),
			"Inappropriate value type in the list of values of a CASE of a 'WHEN'\n"
			"(index has not been created)"
		);
        return 0;
    }
  case e_set:
    set = expr->v.s;
    if (set->range) {
      return 0;
    }
    es = GetSingleExpr(set);
    return CheckWhenSetNode(ref,es,p2);
  default:
    FPRINTF(ASCERR,"Innapropriate value type in the list of %s\n",
	    "values of a CASE of a WHEN statement");
    FPRINTF(ASCERR,"Only symbols or integers and booleans are allowed\n");
    return 0;
  }
}


/**
	Checking that the variables of the list of variables of a
	WHEN statement are appropriate. This is, they
	are boolean, integer or symbol instances. The first part of the
	function was written for the case of WHEN statement
	inside a FOR loop. This function also sorts
	the kinds of variables in the list by assigning a value
	to the integer *p1
*/
static int CheckWhenVariableNode(struct Instance *ref
	, CONST struct Name *name, int *p1
){
  struct gl_list_t *instances;
  struct Instance *inst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  symchar *str;
  struct for_var_t *fvp;
  str = SimpleNameIdPtr(name);
  if(str!=NULL &&
      GetEvaluationForTable()!=NULL &&
      (fvp=FindForVar(GetEvaluationForTable(),str))!=NULL) {
    switch (GetForKind(fvp)) {
    case f_integer:
      *p1=0;
      return 1;
    case f_symbol:
      *p1=2;
      return 1;
    default:
      FPRINTF(ASCERR,"Innapropriate index in the list of %s\n",
	      "variables of a WHEN statement");
      FPRINTF(ASCERR,"only symbol or integer allowed\n");
      return 0;
    }
  }
  instances = FindInstances(ref,name,&err);
  if (instances == NULL){
    switch(rel_errorlist_get_find_error(&err)){
    case unmade_instance:
    case undefined_instance:
      FPRINTF(ASCERR,"Unmade instance in the list of %s\n",
	      "variables of a WHEN statement");
      WriteName(ASCERR,name);
      return 0;
    default:
      FPRINTF(ASCERR,"Unmade instance in the list of %s\n",
	      "variables of a WHEN statement");
      WriteName(ASCERR,name);
      return 0;
    }
  }else{
    if (gl_length(instances)==1) {
      inst = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      switch(InstanceKind(inst)){
      case BOOLEAN_ATOM_INST:
        *p1=1;
        return 1;
      case BOOLEAN_CONSTANT_INST:
        if (AtomAssigned(inst)) {
          *p1=1;
          return 1;
        }else{
          FPRINTF(ASCERR,"Undefined constant in the list of %s\n",
	          "variables of a WHEN statement");
          WriteName(ASCERR,name);
          return 0;
        }
      case INTEGER_ATOM_INST:
        *p1=0;
        return 1;
      case INTEGER_CONSTANT_INST:
        if (AtomAssigned(inst)) {
           *p1=0;
           return 1;
        }else{
          FPRINTF(ASCERR,"Undefined constant in the list of %s\n",
	          "variables of a WHEN statement");
          WriteName(ASCERR,name);
          return 0;
        }
      case SYMBOL_ATOM_INST:
        *p1=2;
        return 1;
      case SYMBOL_CONSTANT_INST:
        if (AtomAssigned(inst)) {
          *p1=2;
          return 1;
        }else{
          FPRINTF(ASCERR,"Undefined constant in the list of %s\n",
	          "variables of a WHEN statement");
          WriteName(ASCERR,name);
          return 0;
        }
      default:
        FPRINTF(ASCERR,"Inappropriate instance in the list of %s\n",
		"variables of a WHEN statement");
        FPRINTF(ASCERR,"Only boolean, integer and symbols are allowed\n");
        WriteName(ASCERR,name);
        return 0;
      }
    }else{
      gl_destroy(instances);
      FPRINTF(ASCERR,"Inappropriate instance in the list of %s\n",
	      "variables of a WHEN statement");
      FPRINTF(ASCERR,"Multiple instances of\n");
      WriteName(ASCERR,name);
      return 0;
    }
  }
}


/**
	Inside a WHEN, only FNAMEs (name of models, relations or array of)
	and nested WHENs ( and FOR loops of them) are allowed. This function
	asks for the checking of these statements.
*/
static
int CheckWhenStatements(struct Instance *inst, struct Statement *statement){
  asc_assert(inst&&statement);
  switch(StatementType(statement)){
    case WHEN:
      return CheckWHEN(inst,statement);
    case FNAME:
      return CheckFNAME(inst,statement);
    case REL:
      return CheckREL(inst,statement);
    case LOGREL:
      return CheckLOGREL(inst,statement);
    case EXT:
      return CheckEXT(inst,statement);
    case REINIT:
    case SWITCHTO:
      return 1;
    case FOR:
      return Pass4RealCheckFOR(inst,statement);
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case LNK:
    case UNLNK:
    case CALL:
    case ASGN:
    case SELECT:
    case TABLESTAT:
    case DATASETSTAT:
         STATEMENT_ERROR(statement,
              "Statement not allowed inside a WHEN statement\n");
         return 0;
    default:
      STATEMENT_ERROR(statement,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
      return 1;
    }
}

/**
	Call CheckWhenSetNode for each value in the set of values included
	in the CASE of a WHEN statement
*/
static
int CheckWhenSetList(struct Instance *inst, struct Set *s, int *p2)
{
  struct Set *set;
  CONST struct  Expr *expr;
  set = s;
  while (set!=NULL) {
    expr = GetSingleExpr(set);
    if (!CheckWhenSetNode(inst,expr,p2)) return 0;
    set = NextSet(set);
    p2++;
  }
  return 1;
}

/**
	Call CheckWhenVariableNode for each variable vl in the variable
	list of a WHEN statement
*/
static
int CheckWhenVariableList(struct Instance *inst, struct VariableList *vlist,
                          int *p1)
{
  CONST struct Name *name;
  CONST struct VariableList *vl;
  vl = vlist;
  while (vl!=NULL) {
    name = NamePointer(vl);
    if (!CheckWhenVariableNode(inst,name,p1)) return 0;
    vl = NextVariableNode(vl);
    p1++;
  }
  return 1;
}

/**
	Checking the list statements of statements inside each CASE of the
	WHEN statement by calling CheckWhenStatements
*/
static
int CheckWhenStatementList(struct Instance *inst, struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  struct gl_list_t *list;
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
     statement = (struct Statement *)gl_fetch(list,c);
     if (!CheckWhenStatements(inst,statement)) return 0;
  }
  return 1;
}


/**
	Checking of the Select statements. It checks that:
	1) The name of the WHEN. If it was already created. It has to be
	   a WHEN or a DUMMY. If a Dummy (case -1 of CheckWhenName),
	   do not check the structure of the WHEN statement, return 1.
	2) The number of conditional variables is equal to the number
	   of values in each of the CASEs.
	3) That the conditional variables exist, and are boolean
	   integer or symbol.
	4) The number and the type  of conditional variables is the same
	   as the number of values in each of the CASEs.
	5) Only one OTHERWISE case is present.
	6) The statements inside a WHEN are only a FNAME or a nested WHEN,
	   and ask for the chcking of these interior statements.
*/
static
int CheckWHEN(struct Instance *inst, struct Statement *statement)
{
  struct Name *wname;
  struct VariableList *vlist;
  struct WhenList *w1;
  struct Set *s;
  struct StatementList *sl;
  unsigned long numother;
  unsigned long numvar;
  unsigned long numset;
  int vl[MAX_VAR_IN_LIST],*p1;
  int casel[MAX_VAR_IN_LIST],*p2;
  wname = WhenStatName(statement);
  if (wname!=NULL) {
    if (!CheckWhenName(inst,wname)) {
    FPRINTF(ASCERR,"Name of a WHEN already exits in\n");
    WriteInstanceName(ASCERR,inst,NULL);
    STATEMENT_ERROR(statement,"The following statement will not be executed: \n");
      return 0;
    }
    if ( CheckWhenName(inst,wname) == -1) return 1;
  }
  vlist = WhenStatVL(statement);
  numvar = VariableListLength(vlist);
  asc_assert(numvar<=MAX_VAR_IN_LIST);
  p1 = &vl[0];
  p2 = &casel[0];
  numother=0;
  if (!CheckWhenVariableList(inst,vlist,p1)) {
    FPRINTF(ASCERR,"In ");
    WriteInstanceName(ASCERR,inst,NULL);
    STATEMENT_ERROR(statement," the following statement will not be executed:\n");
    return 0;
  }
  w1 = WhenStatCases(statement);
  while (w1!=NULL){
      s = WhenSetList(w1);
      if (s!=NULL) {
          numset = SetLength(s);
          if (numvar != numset) {
            FPRINTF(ASCERR,"Number of variables different from %s\n",
		    "number of values in a CASE");
            FPRINTF(ASCERR,"In ");
            WriteInstanceName(ASCERR,inst,NULL);
            STATEMENT_ERROR(statement,
		 " the following statement will not be executed: \n");
	    return 0;
	  }
          if (!CheckWhenSetList(inst,s,p2)) {
            FPRINTF(ASCERR,"In ");
            WriteInstanceName(ASCERR,inst,NULL);
            STATEMENT_ERROR(statement,
		 " the following statement will not be executed: \n");
	    return 0;
	  }
          p1 = &vl[0];
          p2 = &casel[0];
          if (!CompListInArray(numvar,p1,p2)) {
            FPRINTF(ASCERR,"Type of variables different from type %s\n",
		    "of values in a CASE");
            FPRINTF(ASCERR,"In ");
            WriteInstanceName(ASCERR,inst,NULL);
            STATEMENT_ERROR(statement,
		 " the following statement will not be executed: \n");
	    return 0;
	  }
      }else{
          numother++;
          if (numother>1) {
            FPRINTF(ASCERR,"More than one default case in a WHEN\n");
            FPRINTF(ASCERR,"In ");
            WriteInstanceName(ASCERR,inst,NULL);
            STATEMENT_ERROR(statement,
		 " the following statement will not be executed: \n");
	    return 0;
	  }
      }
      sl = WhenStatementList(w1);
      if (!CheckWhenStatementList(inst,sl)) {
        FPRINTF(ASCERR,"In ");
        WriteInstanceName(ASCERR,inst,NULL);
        STATEMENT_ERROR(statement,
	     " the following statement will not be executed: \n");
	return 0;
      }
      w1 = NextWhenCase(w1); }
  return 1;
}


/* - - - - - - - - - - - - -
	Check SELECT Functions
*/

#ifdef THIS_IS_AN_UNUSED_FUNCTION
/**
	Code curently not in use. It would be used in case that we want to do
	the checking of all of the statement list in all of the cases of a
	SELECT simultaneously, previous to execution.
	Actually, the code is in disrepair, particularly around what is
	allowed in SELECT. We surely need to create a CheckSelectStatement
	function specific for each pass of instantiation.
*/
static
int CheckSelectStatements(struct Instance *inst, struct Statement *statement)
{
  asc_assert(inst&&statement);
  switch(StatementType(statement)){
  case ALIASES:
  case ISA:
  case IRT:
  case ATS:
  case AA:
  case LNK:
  case UNLNK:
  case ARR:
    return 1;
  case FOR:
    return Pass1RealCheckFOR(inst,statement);
  case ASGN:
    return CheckASGN(inst,statement);
  case CASGN:
    return CheckCASGN(inst,statement);
  case SELECT:
    return CheckSELECT(inst,statement);
  case REL: /* not broken. equations disallowed. */
  case LOGREL:
  case EXT:
  case CALL:
  case WHEN:
  case FNAME:
    if (g_iteration>=MAXNUMBER) { /* see WriteUnexecutedMessage */
       STATEMENT_ERROR(statement,
              "Statement not allowed inside a SELECT statement\n"); }
    /** AND WHY NOT? fix me. **/
    return 0;
  default:
    STATEMENT_ERROR(statement,"Inappropriate statement type");
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
    return 1;
  }
}
#endif  /*  THIS_IS_AN_UNUSED_FUNCTION  */


#ifdef THIS_IS_AN_UNUSED_FUNCTION
/**
	Currently not in use
*/
static
int CheckSelectStatementList(struct Instance *inst, struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  struct gl_list_t *list;
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    if (!CheckSelectStatements(inst,statement)) return 0;
  }
  return 1;
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */


/**
	Current checking of the Select statement starts here.

	Checking that the values of the set of values of each CASE of a
	SELECT statement are appropriate. This is, they
	are symbol, integer or boolean. The first part of the
	function was written for the case of SELECT statement
	inside a FOR loop. Therefore, it is going to be there,
	but not used at the moment.This function also sorts
	the kinds of values in the set by assigning a value
	to the integer *p2
*/
static
int CheckSelectSetNode(struct Instance *ref, CONST struct Expr *expr,
                       int *p2 )
{
  symchar *str;
  struct for_var_t *fvp;
  struct Set *set;
  CONST struct Expr *es;
  switch (ExprType(expr)) {
    case e_boolean:
      if (ExprBValue(expr)==2) {
       *p2 = 3;  /*  ANY */
      }else{
       *p2=1;
      }
       return 1;
    case e_int:
       *p2=0;
       return 1;
    case e_symbol:
       *p2=2;
       return 1;
    case e_var:
      if ((NULL != GetEvaluationForTable()) &&
          (NULL != (str = SimpleNameIdPtr(ExprName(expr)))) &&
          (NULL != (fvp=FindForVar(GetEvaluationForTable(),str)))) {
        if (GetForKind(fvp)==f_integer){
          *p2=0;
          return 1;
        }else{
          if(GetForKind(fvp)==f_symbol){
            *p2=2;
            return 1;
          }
          return 0;
        }
      }else{
        return 0;
      }
    case e_set:
      set = expr->v.s;
      if (set->range) {
        return 0;
      }
      es = GetSingleExpr(set);
      return CheckSelectSetNode(ref,es,p2);
    default:
       return 0;
  }
}

/**
	Checking that the variables of the list of variables of a
	SELECT statement are appropriate. This is, they
	are constant and are assigned. The first part of the
	function was written for the case of SELECT statement
	inside a FOR loop. Therefore, it is going to be there,
	but not used at the moment.This function also sorts
	the kinds of variables in the list by assigning a value
	to the integer *p1
*/
static
int CheckSelectVariableNode(struct Instance *ref,
                            CONST struct Name *name,
                            int *p1)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  symchar *str;
  struct for_var_t *fvp;

  str = SimpleNameIdPtr(name);
  if( str!=NULL &&
      GetEvaluationForTable() != NULL &&
      (fvp=FindForVar(GetEvaluationForTable(),str))!=NULL) {

    switch (GetForKind(fvp)) {
    case f_integer:
      *p1=0;
      return 1;
    case f_symbol:
      *p1=2;
      return 1;
    default:
      return 0;
    }
  }

  instances = FindInstances(ref,name,&err);
  if (instances == NULL){
    switch(rel_errorlist_get_find_error(&err)){
    case unmade_instance:
    case undefined_instance: return 0;
    default:
      return 0;
    }
  }else{
    if (gl_length(instances)==1) {
      inst = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      switch(InstanceKind(inst)){
        case BOOLEAN_CONSTANT_INST:
          if (AtomAssigned(inst)) {
            *p1 = 1;
            return 1;
          }else{
            return 0;
          }
        case INTEGER_CONSTANT_INST:
          if (AtomAssigned(inst)) {
            *p1 = 0;
            return 1;
          }else{
            return 0;
          }
        case SYMBOL_CONSTANT_INST:
          if (AtomAssigned(inst)) {
            *p1 = 2;
            return 1;
          }else{
            return 0;
          }
        default:
          return 0;
      }
    }else{
      gl_destroy(instances);
      return 0;
    }
  }
}

/**
	Call CheckSelectSetNode for each set member of the set of
	values of each CASE of a SELECT statement
*/
static
int CheckSelectSetList(struct Instance *inst, struct Set *s, int *p2 )
{
  struct Set *set;
  CONST struct  Expr *expr;
  set = s;
  while (set!=NULL) {
    expr = GetSingleExpr(set);
    if (!CheckSelectSetNode(inst,expr,p2)) return 0;
    set = NextSet(set);
    p2++;
  }
  return 1;
}

/**
	Call CheckSelectVariableNode for each variable vl in the variable
	list of a SELECT statement
*/
static
int CheckSelectVariableList(struct Instance *inst, struct VariableList *vlist,
                            int *p1)
{
  CONST struct Name *name;
  CONST struct VariableList *vl;
  vl = vlist;
  while (vl!=NULL) {
    name = NamePointer(vl);
    if (!CheckSelectVariableNode(inst,name,p1)) return 0;
    vl = NextVariableNode(vl);
    p1++;
  }
  return 1;
}


/**
	The conditions for checkselect is that
	1) The number of selection variables is equal to the number
	   of values in each of the CASEs.
	2) That the selection variables exist, are constant and
	   are assigned.
	3) Only one OTHERWISE case is present.
*/
static
int CheckSELECT(struct Instance *inst, struct Statement *statement)
{
  struct VariableList *vlist;
  struct SelectList *sel1;
  struct Set *set;
  unsigned long numother;
  unsigned long numsvar;
  unsigned long numsset;
  int vl[MAX_VAR_IN_LIST], *p1;
  int casel[MAX_VAR_IN_LIST], *p2;

  vlist = SelectStatVL(statement);
  numsvar = VariableListLength(vlist);
  asc_assert(numsvar<=MAX_VAR_IN_LIST);
  p1 = &vl[0];
  p2 = &casel[0];
  numother = 0;

  if (!CheckSelectVariableList(inst,vlist,p1)) return 0;
  sel1 = SelectStatCases(statement);
  while (sel1!=NULL){
    set = SelectSetList(sel1);
    if (set!=NULL) {
      numsset = SetLength(set);
      if (numsvar != numsset) return 0;
      if (!CheckSelectSetList(inst,set,p2)) return 0;
      p1 = &vl[0];
      p2 = &casel[0];
      if (!CompListInArray(numsvar,p1,p2)) return 0;
    }else{
      numother++;
      if (numother>1) return 0;
    }
    sel1 = NextSelectCase(sel1);
  }
  return 1;
}


/*==============================================================================
	PASS-BY-PASS CHECKING
*/


static
int Pass4CheckStatement(struct Instance *inst, struct Statement *stat)
{
  asc_assert(stat&&inst);
  switch(StatementType(stat)){
  case WHEN:
    return CheckWHEN(inst,stat);
  case FNAME:
    return CheckFNAME(inst,stat);
  case FOR:
    return Pass4CheckFOR(inst,stat);
  case UNLNK:
    STATEMENT_ERROR(stat,"Inappropriate UNLINK statement in the declarative part");
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
    return 0;
  case COND:
  case SELECT:
  case REL:
  case EXT:
  case LOGREL:
  case ISA:
  case ARR:
  case ALIASES:
  case IRT:
  case ATS:
  case AA:
  case LNK:
  case CASGN:
  case ASGN:
  case TABLESTAT:
  case DATASETSTAT:
  default:
    return 1; /* ignore all in phase 4.*/
  }
}


static
int Pass3CheckStatement(struct Instance *inst, struct Statement *stat)
{
  asc_assert(stat&&inst);
  switch(StatementType(stat)){
  case FOR:
    return Pass3RealCheckFOR(inst,stat);
  case LOGREL:
    return CheckLOGREL(inst,stat);
  case COND:
    return Pass3CheckCOND(inst,stat);
  case REL:
  case EXT:
  case ALIASES:
  case ARR:
  case ISA:
  case IRT:
  case ATS:
  case AA:
  case LNK:
  case UNLNK:
  case CASGN:
  case ASGN:
  case WHEN:
  case SELECT:
  case FNAME:
  case TABLESTAT:
  case DATASETSTAT:
  default:
    return 1; /* ignore all in phase 3. nondeclarative flagged in pass1 */
  }
}


static
int Pass2CheckStatement(struct Instance *inst, struct Statement *stat)
{
  asc_assert(stat&&inst);
  switch(StatementType(stat)){
  case FOR:
    return Pass2RealCheckFOR(inst,stat);
  case EXT:
    return CheckEXT(inst,stat);
  case REL:
    return CheckREL(inst,stat);
  case COND:
    return Pass2CheckCOND(inst,stat);
  case LOGREL:
  case ALIASES:
  case ARR:
  case ISA:
  case IRT:
  case ATS:
  case AA:
  case LNK:
  case UNLNK:
  case CASGN:
  case ASGN:
  case WHEN:
  case SELECT:
  case FNAME:
  case TABLESTAT:
  case DATASETSTAT:
  default:
    return 1; /* ignore all in phase 2. nondeclarative flagged in pass1 */
  }
}

/**
	checking statementlist, as in a FOR loop check.
	relations are not handled in pass 1
*/
static
int Pass1CheckStatement(struct Instance *inst, struct Statement *stat)
{
  asc_assert(stat&&inst);
  switch(StatementType(stat)){
  case ALIASES:
    return CheckALIASES(inst,stat);
  case ARR:
    return CheckARR(inst,stat);
  case ISA:
    if ( CheckISA(inst,stat) == 0 ) {
      return 0;
    }
    return MakeParameterInst(inst,stat,NULL,NOKEEPARGINST); /*1*/
  case IRT:
    if ( CheckIRT(inst,stat) == 0 ) {
      return 0;
    }
    return MakeParameterInst(inst,stat,NULL,NOKEEPARGINST); /*1b*/
  case ATS:
    return CheckATS(inst,stat);
  case AA:
    return CheckAA(inst,stat);
  case LNK:
    return CheckLNK(inst,stat);
  case UNLNK:
   	FPRINTF(ASCERR,"UNLINK are only allowed inside a non-declarative part.\n");
   	return 0;
  case FOR:
    return Pass1CheckFOR(inst,stat);
  case REL:
    return 1; /* ignore'm in phase 1 */
  case EXT:
    return 1; /* ignore'm in phase 1 */
  case COND:
    return 1; /* ignore'm in phase 1 */
  case LOGREL:
    return 1; /* ignore'm in phase 1 */
  case CASGN:
    return CheckCASGN(inst,stat);
  case TABLESTAT:
    return 1;
  case DATASETSTAT:
    return 1;
  case ASGN:
    return 1; /* ignore'm in phase 1 */
  case WHEN:
    return 1; /* ignore'm in phase 1 */
  case SELECT:
    return CheckSELECT(inst,stat);
  case FNAME:
    FPRINTF(ASCERR,"FNAME are only allowed inside a WHEN Statement\n");
    return 0;
  default:
    STATEMENT_ERROR(stat,"Inappropriate statement type");
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
    return 1;
  }
}

static
int Pass4CheckStatementList(struct Instance *inst, struct StatementList *sl)
{
  unsigned long c,len;
  struct gl_list_t *list;
  struct Statement *stat;
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    if (!Pass4CheckStatement(inst,stat)) return 0;
  }
  return 1;
}

static
int Pass3CheckStatementList(struct Instance *inst, struct StatementList *sl)
{
  unsigned long c,len;
  struct gl_list_t *list;
  struct Statement *stat;
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    if (!Pass3CheckStatement(inst,stat)) return 0;
  }
  return 1;
}

static
int Pass2CheckStatementList(struct Instance *inst, struct StatementList *sl)
{
  unsigned long c,len;
  struct gl_list_t *list;
  struct Statement *stat;
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    if (Pass2CheckStatement(inst,stat)==0) return 0;
  }
  return 1;
}

static
int Pass1CheckStatementList(struct Instance *inst, struct StatementList *sl)
{
  unsigned long c,len;
  struct gl_list_t *list;
  struct Statement *stat;
  asc_assert(inst&&sl);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    if (Pass1CheckStatement(inst,stat)==0) return 0;
  }
  return 1;
}


/*------------------------------------------------------------------------------
  'FNAME' STATEMENT PROCESSING
*/

/**
	The FNAME statement is just used to stand for the model relations or
	arrays inside the CASES of a WHEN statement. Actually, this
	statement does not need to be executed. It is required only
	for checking and for avoiding conflicts in the semantics.
*/
static
int ExecuteFNAME(struct Instance *inst, struct Statement *statement)
{
  (void)inst;       /*  stop gcc whine about unused parameter */
  (void)statement;  /*  stop gcc whine about unused parameter */
  return 1;
}

/*------------------------------------------------------------------------------
  CONDITIONAL Statement Processing
*/

/**
	The logical relations inside a conditional statement do not have
	to be satisified. They are going to be used to check conditions in
	the solution of other logical relations. So, we need something to
	distinguish a conditional logrelation from a non-conditional
	logrelation. The next three functions "Mark" those log relations
	inside a CONDITIONAL statement as Conditional logrelations.
	Right now we not only set a bit indicating
	that the logrelation is conditional, but also set a flag equal to 1.
	This is done in MarkLOGREL above. The flag could be eliminated, but
	we need to fix some places in which it is used, and to use the
	bit instead.
*/
static
void Pass3MarkCondLogRels(struct Instance *inst, struct Statement *statement)
{
  switch(StatementType(statement)){
    case LOGREL:
      MarkLOGREL(inst,statement);
      break;
    case FOR:
      if ( ForContainsLogRelations(statement) ) {
        Pass3FORMarkCond(inst,statement);
      }
      break;
    case REL:
      break;
    case EXT:
      break;
    default:
      STATEMENT_ERROR(statement,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
  }
}

static
void Pass3MarkCondLogRelStatList(struct Instance *inst,
                                 struct StatementList *sl)
{
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(stat)){
      case LOGREL:
        MarkLOGREL(inst,stat);
        break;
      case FOR:
        if ( ForContainsLogRelations(stat) ) {
        Pass3FORMarkCondLogRels(inst,stat);
        }
        break;
      case EXT:
        break;
      case REL:
        break;
      default:
        STATEMENT_ERROR(stat,"Inappropriate statement type");
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
    }
  }
}

static
void Pass3MarkCondLogRelStat(struct Instance *inst,
                             struct Statement *statement)
{
  struct StatementList *sl;
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  sl = CondStatList(statement);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    Pass3MarkCondLogRels(inst,stat);
  }
}


/**
	Execution of the statements allowed inside a CONDITIONAL
	statement. Only log/relations and FOR loops containing only
	log/relations are allowed.
*/
static
int Pass3ExecuteCondStatements(struct Instance *inst,
                               struct Statement *statement)
{
  switch(StatementType(statement)){
    case LOGREL:
      return ExecuteLOGREL(inst,statement);
    case FOR:
      if ( ForContainsLogRelations(statement) ) {
        return Pass3ExecuteFOR(inst,statement);
      }else{
        return 1;
      }
    case EXT:
      return 1; /* assume done */
    case REL:
      return 1; /* assume done */
    default:
      STATEMENT_ERROR(statement,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
      return 0;
  }
}

static
int Pass3RealExecuteCOND(struct Instance *inst, struct Statement *statement)
{
  struct StatementList *sl;
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  sl = CondStatList(statement);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    if (!Pass3ExecuteCondStatements(inst,stat)) return 0;
  }
  return 1;
}

/**
	Execution of the Conditional statements. In pass3 we consider only
	logrelations (or FOR loops of logrelations),so  the checking is not
	done at all. After execution, the logrelations are set as conditional
	by means of a bit and a flag
*/
static
int Pass3ExecuteCOND(struct Instance *inst, struct Statement *statement)
{
  int return_value;

  if (Pass3RealExecuteCOND(inst,statement)) {
    return_value = 1;
  }else{
    return_value = 0;
  }
  Pass3MarkCondLogRelStat(inst,statement);
  return return_value;
}

/**
	The relations inside a conditional statement do not have to be
	solved. They are going to be used as boundaries in conditional
	programming. So, we need something to distinguish a conditional
	relation from a non-conditional relation. The next three functions
	"Mark" those relations inside a CONDITIONAL statement as
	Conditional relations. Right now we not only set a bit indicating
	that the relation is conditional, but also set a flag equal to 1.
	This is done in MarkREL above. The flag could be eliminated, but
	we need to fix some places in which it is used, and to use the
	bit instead.
*/
static
void Pass2MarkCondRelations(struct Instance *inst, struct Statement *statement)
{
  switch(StatementType(statement)){
    case REL:
      MarkREL(inst,statement);
      break;
    case EXT:
      MarkEXT(inst,statement);
      break;
    case FOR:
      if ( ForContainsRelations(statement) ||
           ForContainsExternal(statement)
         ) {
        Pass2FORMarkCond(inst,statement);
      }
      break;
    case LOGREL:
      break;
    default:
      STATEMENT_ERROR(statement,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
  }
}

static
void Pass2MarkCondRelStatList(struct Instance *inst, struct StatementList *sl)
{
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(stat)){
      case EXT:
        MarkEXT(inst,stat);
        break;
      case REL:
        MarkREL(inst,stat); /* MarkEXT is MarkREL exactly */
        break;
      case FOR:
        if ( ForContainsRelations(stat) ||
             ForContainsExternal(stat)
           ) {
          Pass2FORMarkCondRelations(inst,stat);
        }
        break;
      case LOGREL:
        break;
      default:
        STATEMENT_ERROR(stat,"Inappropriate statement type");
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
    }
  }
}

static
void Pass2MarkCondRelStat(struct Instance *inst, struct Statement *statement)
{
  struct StatementList *sl;
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  sl = CondStatList(statement);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    Pass2MarkCondRelations(inst,stat);
  }
}

/**
	Execution of the statements allowed inside a CONDITIONAL
	statement. Only relations and FOR loops containing only
	relations are considered in Pass2.
*/
static
int Pass2ExecuteCondStatements(struct Instance *inst,
                               struct Statement *statement)
{
  switch(StatementType(statement)){
    case REL:
#ifdef DEBUG_RELS
    ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
    FPRINTF(stderr,"Pass2ExecuteCondStatements: case REL");
    WriteStatement(stderr, statement, 3);
    error_reporter_end_flush();
#endif
      return ExecuteREL(inst,statement);
    case EXT:
#ifdef DEBUG_RELS
    ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
    FPRINTF(stderr,"Pass2ExecuteCondStatements: case EXT");
    WriteStatement(stderr, statement, 3);
    error_reporter_end_flush();
#endif
      return ExecuteEXT(inst,statement);
    case FOR:
      if ( ForContainsRelations(statement) ) {
#ifdef DEBUG_RELS
        ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
        FPRINTF(stderr,"Pass2ExecuteCondStatements: case FOR");
        WriteStatement(stderr, statement, 3);
        error_reporter_end_flush();
#endif
        return Pass2ExecuteFOR(inst,statement);
      }
      return 1;
    case LOGREL:
      return 1; /* Ignore */
    default:
      STATEMENT_ERROR(statement,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
      return 0;
  }
}

static
int Pass2RealExecuteCOND(struct Instance *inst, struct Statement *statement)
{
  struct StatementList *sl;
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  sl = CondStatList(statement);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    if (!Pass2ExecuteCondStatements(inst,stat)) return 0;
  }
  return 1;
}

/**
	Execution of the Conditional statements. In pass2 we consider only
	relations (or FOR loops of relations),so  the checking is not
	done at all. After execution, the relations are set as conditional
	by means of a bit and a flag
*/
static
int Pass2ExecuteCOND(struct Instance *inst, struct Statement *statement)
{
  int return_value;

  if (Pass2RealExecuteCOND(inst,statement)) {
    return_value = 1;
  }else{
    return_value = 0;
  }
  Pass2MarkCondRelStat(inst,statement);
  return return_value;
}


/**
	For its use in ExecuteUnSelectedStatements.
	Execute the  statements of a CONDITIONAL statement which is inside
	a CASE not matching the selection variables.

	Only FOR loops containing log/relations or log/relations are allowed
	inside CONDITIONAL statements. This function ultimately call
	the function ExecuteUnSelectedEQN, to create Dummy instances
	for the relations inside CONDITIONAL
*/
static
int ExecuteUnSelectedCOND(struct Instance *inst, struct Statement *statement)
{
  struct StatementList *sl;
  struct Statement *stat;
  unsigned long c,len;
  struct gl_list_t *list;
  int return_value = 0;

  sl = CondStatList(statement);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(stat)){
    case FOR:
      return_value = ExecuteUnSelectedForStatements(inst,ForStatStmts(stat));
      break;
    case REL:
    case EXT:
    case LOGREL:
      return_value = ExecuteUnSelectedEQN(inst,stat);
      break;
    default:
      STATEMENT_ERROR(stat,"Inappropriate statement type");
	  ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
      Asc_Panic(2, NULL,
                "Inappropriate statement type in CONDITIONAL Statement");
    }
    asc_assert(return_value);
  }
  return 1;
}

/*------------------------------------------------------------------------------
  'WHEN' STATEMENT PROCESSING
*/

/**
	Find the instances corresponding to the list of conditional
	variables of a WHEN, and append ther pointers in a gl_list.
	This gl_list becomes part of the WHEN instance.
	Also, this function notify those instances that the WHEN is
	pointing to them, so that their list of whens is updated.
*/
static
struct gl_list_t *MakeWhenVarList(struct Instance *inst,
                                  struct Instance *child,
                                  CONST struct VariableList *vlist)
{
  CONST struct Name *name;
  struct Instance *var;
  struct gl_list_t *instances;
  struct gl_list_t *whenvars;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  unsigned long numvar;

  numvar = VariableListLength(vlist);
  whenvars = gl_create(numvar);

  while(vlist != NULL){
    name = NamePointer(vlist);
    instances = FindInstances(inst,name,&err);
    if (instances == NULL){
      ASC_PANIC("Instance not found in MakeWhenVarList \n");
    }else{
      if (gl_length(instances)==1) {
        var = (struct Instance *)gl_fetch(instances,1);
        gl_destroy(instances);
        switch(InstanceKind(var)){
          case BOOLEAN_ATOM_INST:
          case INTEGER_ATOM_INST:
          case SYMBOL_ATOM_INST:
          case BOOLEAN_CONSTANT_INST:
          case INTEGER_CONSTANT_INST:
          case SYMBOL_CONSTANT_INST:
            gl_append_ptr(whenvars,(VOIDPTR)var);
            AddWhen(var,child);
            break;
          default:
            Asc_Panic(2, NULL,
                      "Incorrect instance type in MakeWhenVarList \n");
        }
      }else{
        gl_destroy(instances);
        Asc_Panic(2, NULL,
                  "Variable name assigned to more than one instance \n");
      }
    }
    vlist = NextVariableNode(vlist);
  }
  return whenvars;
}

/*- - - - - - - - - -  -  - - - - - - - - -
	The following four functions create the gl_list of references of
	each CASE of a WHEN instance. By list of references I mean the
	list of pointers to relations, models or arrays which will become
	active if such a CASE applies.
*/

/**
	creating list of reference for each CASE in a WHEN: (1) dealing with arrays
*/
static
void MakeWhenArrayReference(struct Instance *when,
                            struct Instance *child,
                            struct gl_list_t *listref)
{
  struct Instance *arraychild;
  unsigned long len,c;
  switch (InstanceKind(child)) {
  case REL_INST:
    gl_append_ptr(listref,(VOIDPTR)child);
    AddWhen(child,when);
    relinst_set_in_when(child,TRUE);
    return;
  case LREL_INST:
    gl_append_ptr(listref,(VOIDPTR)child);
    AddWhen(child,when);
    logrelinst_set_in_when(child,TRUE);
    return;
  case MODEL_INST:
    gl_append_ptr(listref,(VOIDPTR)child);
    AddWhen(child,when);
    model_set_in_when(child,TRUE);
    return;
  case WHEN_INST:
    gl_append_ptr(listref,(VOIDPTR)child);
    AddWhen(child,when);
    return;
  case ARRAY_INT_INST:
  case ARRAY_ENUM_INST:
    len = NumberChildren(child);
    for(c=1;c<=len;c++){
      arraychild = InstanceChild(child,c);
      MakeWhenArrayReference(when,arraychild,listref);
    }
    return;
  default:
    Asc_Panic(2, NULL,
              "Incorrect array instance name inside a WHEN statement\n");
  }
}

/**
	creating list of reference for each CASE in a WHEN: (2)
*/
static
void MakeWhenReference(struct Instance *ref,
                       struct Instance *child,
                       struct Name *name,
                       struct gl_list_t *listref)
{
  struct Instance *inst,*arraychild;
  struct gl_list_t *instances;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  unsigned long len,c;

  instances = FindInstances(ref,name,&err);
  if (instances==NULL){
    gl_destroy(instances);
    WriteName(ASCERR,name);
    Asc_Panic(2, NULL,
              "Name of an unmade instance (Relation-Model)"
              " inside a WHEN statement \n");
  }else{
    if (gl_length(instances)==1){
      inst = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      switch (InstanceKind(inst)) {
        case REL_INST:
          gl_append_ptr(listref,(VOIDPTR)inst);
          AddWhen(inst,child);
          relinst_set_in_when(inst,TRUE);
          return;
        case LREL_INST:
          gl_append_ptr(listref,(VOIDPTR)inst);
          AddWhen(inst,child);
          logrelinst_set_in_when(inst,TRUE);
          return;
        case MODEL_INST:
          gl_append_ptr(listref,(VOIDPTR)inst);
          AddWhen(inst,child);
          model_set_in_when(inst,TRUE);
          return;
        case WHEN_INST:
          gl_append_ptr(listref,(VOIDPTR)inst);
          AddWhen(inst,child);
          return;
        case ARRAY_INT_INST:
        case ARRAY_ENUM_INST:
          len = NumberChildren(inst);
          for(c=1;c<=len;c++){
            arraychild = InstanceChild(inst,c);
            MakeWhenArrayReference(child,arraychild,listref);
          }
          return;
        default:
          gl_destroy(instances);
          WriteName(ASCERR,name);
          Asc_Panic(2, NULL,
                    "Incorrect instance name inside a WHEN statement\n");
          break;
      }
    }else{
      gl_destroy(instances);
      WriteName(ASCERR,name);
      Asc_Panic(2, NULL,
                "Error in WHEN statement. Name assigned"
                " to more than one instance type\n");
    }
  }
}

static void MakeWhenStatementReference(struct Instance *ref,
                                       struct Instance *child,
                                       struct Statement *statement,
                                       struct gl_list_t *listref)
{
  struct Name *name = NULL;

  switch(StatementType(statement)){
  case WHEN:
    name = WhenStatName(statement);
    break;
  case FNAME:
    name = FnameStat(statement);
    break;
  case REL:
    name = RelationStatName(statement);
    break;
  case LOGREL:
    name = LogicalRelStatName(statement);
    break;
  case EXT:
    name = ExternalStatNameRelation(statement);
    break;
  default:
    return;
  }

  if(name != NULL){
    MakeWhenReference(ref,child,name,listref);
  }
}

/**
	creating list of reference for each CASE in a WHEN: (3) nested WHENs,
	nested FOR loops etc.
*/
static
void MakeWhenCaseReferences(struct Instance *inst,
                            struct Instance *child,
                            struct StatementList *sl,
                            struct gl_list_t *listref)
{
  struct Statement *statement;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case WHEN:
    case FNAME:
    case REL:
    case LOGREL:
    case EXT:
      MakeWhenStatementReference(inst,child,statement,listref);
      break;
    case FOR:
      MakeWhenCaseReferencesFOR(inst,child,statement,listref);
      break;
    case REINIT:
    case SWITCHTO:
      break;
    default:
      WSEM(stderr,statement,
                      "Inappropriate statement type in WHEN Statement");
      ASC_PANIC("Inappropriate statement type in WHEN Statement");
    }
  }
}

/**
	creating list of reference for each CASE in a WHEN: (4) almost identical
	to the previous one.
	They differ only in the case of a FOR loop. This function is
	required to appropriately deal with nested FOR loops which
	contain FNAMEs or WHENs
*/
static
void MakeRealWhenCaseReferencesList(struct Instance *inst,
                                    struct Instance *child,
                                    struct StatementList *sl,
                                    struct gl_list_t *listref)
{
  struct Statement *statement;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case WHEN:
    case FNAME:
    case REL:
    case LOGREL:
    case EXT:
      MakeWhenStatementReference(inst,child,statement,listref);
      break;
    case FOR:
      MakeRealWhenCaseReferencesFOR(inst,child,statement,listref);
      break;
    case REINIT:
    case SWITCHTO:
      break;
    default:
      STATEMENT_ERROR(statement,
                      "Inappropriate statement type in declarative section");
      ASC_PANIC("Inappropriate statement type in declarative section");
      break;
    }
  }
  return ;
}

/*-  - - - - - - - - - -- - - - - -  - - */

/**
	Make a WHEN instance or an array of WHEN instances by calling
	CreateWhenInstance. It does not create the lists of pointers
	to the conditional variables or the models or relations.
*/

static
struct Instance *MakeWhenInstance(struct Instance *parent,
                                  struct Name *name,
                                  struct Statement *stat)
{
  symchar *when_name;
  struct TypeDescription *desc;
  struct Instance *child;
  struct InstanceName rec;
  unsigned long pos;
  if ((when_name=SimpleNameIdPtr(name))!=NULL){
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,when_name);
    if(0 != (pos = ChildSearch(parent,&rec))){
      asc_assert(InstanceChild(parent,pos)==NULL);
      desc = FindWhenType();
      child = CreateWhenInstance(desc);
      LinkToParentByPos(parent,child,pos);
      return child;
    }else{
      return NULL;
    }
  }else{				/* sparse array of when */
    when_name = NameIdPtr(name);
    SetInstanceNameType(rec,StrName);
    SetInstanceNameStrPtr(rec,when_name);
    if(0 != (pos = ChildSearch(parent,&rec))){
      if (InstanceChild(parent,pos)==NULL){ /* need to make array */
        child = MakeSparseArray(parent,name,stat,NULL,0,NULL,NULL,NULL);
      }else{			/* need to add array element */
        child = AddArrayChild(parent,name,stat,NULL,NULL,NULL);
      }
      return child;
    }else{
      return NULL;
    }
  }
}

/**
	Executing the possible kind of statements inside a WHEN. It
	consider the existence of FOR loops and nested WHENs
*/
static
void ExecuteWhenStatements(struct Instance *inst,
                           struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  int return_value = 0;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case WHEN:
      return_value = 1;
      RealExecuteWHEN(inst,statement);
      break;
    case FNAME:
      return_value = ExecuteFNAME(inst,statement);
      break;
    case REL:
      return_value = ExecuteREL(inst,statement);
      break;
    case LOGREL:
      return_value = ExecuteLOGREL(inst,statement);
      break;
    case EXT:
      return_value = ExecuteEXT(inst,statement);
      break;
    case REINIT:
    case SWITCHTO:
      return_value = 1;
      break;
    case FOR:
      return_value = 1;
      Pass4ExecuteFOR(inst,statement);
      break;
    default:
      WSEM(stderr,statement,
                      "Inappropriate statement type in WHEN Statement");
      ASC_PANIC("Inappropriate statement type in WHEN Statement");
    }
    if(!return_value){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,
        "while running %s on statement inside FOR", __FUNCTION__);
      return;
    }
  }
}


/**
	Creates a CASE included in a WHEN statement. It involves the
	allocation of memory of the CASE and the creation of the
	gl_list of references (pointer to models, arrays, relations)
	which will be contained in such a case.
*/
static
struct Case *RealExecuteWhenStatements(struct Instance *inst,
                                       struct Instance *child,
                                       struct WhenList *w1)
{
  struct StatementList *sl;
  struct Case *cur_case;
  struct gl_list_t *listref;
  struct gl_list_t *reinit;
  struct Set *set;

  listref = gl_create(AVG_REF);

  set = WhenSetList(w1);
  cur_case = CreateCase(CopySetByReference(set),NULL);
  sl = WhenStatementList(w1);
  ExecuteWhenStatements(inst,sl);
  MakeWhenCaseReferences(inst,child,sl,listref);
  reinit = CollectWhenReinitStatements(sl);
  SetCaseReferences(cur_case,listref);
  SetCaseReinitStatements(cur_case,reinit);
  return cur_case;
}


/**
	Call the Creation of a WHEN instance. This function is also in charge
	of filling the gl_list of conditional variables and the gl_list of
	CASEs contained in the WHEN instance
*/
static void RealExecuteWHEN(struct Instance *inst, struct Statement *statement){
  struct VariableList *vlist;
  struct WhenList *w1;
  struct Instance *child;
  struct Name *wname;
  struct Case *cur_case;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct gl_list_t *instances;
  struct gl_list_t *whenvars;
  struct gl_list_t *whencases;

  wname = WhenStatName(statement);
  instances = FindInstances(inst,wname,&err);
  if (instances==NULL) {
    /* if (ferr == unmade_instance) { */
      child = MakeWhenInstance(inst,wname,statement);
      if (child == NULL) {
        STATEMENT_ERROR(statement,"Unable to create when instance");
        ASC_PANIC("Unable to create when instance");
      }
      /*    }
    else {
      STATEMENT_ERROR(statement,"Unable to execute statement");
      ASC_PANIC("Unable to execute statement");
    }  */
  }else{
    if(gl_length(instances)==1){
      child = (struct Instance *)gl_fetch(instances,1);
      asc_assert((InstanceKind(child)==WHEN_INST) || (InstanceKind(child)==DUMMY_INST));
      gl_destroy(instances);
      if (InstanceKind(child)==DUMMY_INST) {
        return;
      }
    }else{
      STATEMENT_ERROR(statement, "Expression name refers to more than one object");
      gl_destroy(instances);
      ASC_PANIC("Expression name refers to more than one object");
	  child = NULL;
    }
  }
  vlist = WhenStatVL(statement);
  whenvars = MakeWhenVarList(inst,child,vlist);
  SetWhenVarList(child,whenvars);
  whencases = gl_create(AVG_CASES);
  w1 = WhenStatCases(statement);
  while (w1!=NULL){
    cur_case = RealExecuteWhenStatements(inst,child,w1);
    gl_append_ptr(whencases,(VOIDPTR)cur_case);
    w1 = NextWhenCase(w1);
  }
  SetWhenCases(child,whencases);
}


/**
	After Checking the WHEN statement, it calls for its  execution
*/
static
int ExecuteWHEN(struct Instance *inst, struct Statement *statement)
{
  if (CheckWHEN(inst,statement)){
    RealExecuteWHEN(inst,statement);
    return 1;
  }else{
    return 0;
  }
}


/**
	Written because of the possiblity of nested WHEN and
	Nested WHEN inside a FOR loop in an unselected case of
	SELECT statement
*/
static
void ExecuteUnSelectedWhenStatements(struct Instance *inst,
                                     struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  int return_value = 0;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case WHEN:
      return_value = ExecuteUnSelectedWHEN(inst,statement);
      break;
    case FNAME:
      return_value = 1;
      break;
    case REL:
    case LOGREL:
    case EXT:
      return_value = ExecuteUnSelectedEQN(inst,statement);
      break;
    case REINIT:
    case SWITCHTO:
      return_value = 1;
      break;
    case FOR:
      return_value = ExecuteUnSelectedForStatements(inst,
                                                    ForStatStmts(statement));
      break;
    default:
      WSEM(stderr,statement,
                      "Inappropriate statement type in WHEN Statement");
      ASC_PANIC("Inappropriate statement type in WHEN Statement");
    }
    if(!return_value){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,
        "while running %s on statement inside FOR", __FUNCTION__);
      return;
    }
  }
}

/*
	For its use in ExecuteUnSelectedStatements.
	Execute the WHEN statements inside those cases of a SELECT
	which do not match the selection variables
*/
static
int ExecuteUnSelectedWHEN(struct Instance *inst, struct Statement *statement)
{
  struct WhenList *w1;
  struct Instance *child;
  struct Name *wname;
  struct StatementList *sl;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct gl_list_t *instances;
  struct TypeDescription *def;

  wname = WhenStatName(statement);
  instances = FindInstances(inst,wname,&err);
  if (instances==NULL) {
    def = FindDummyType();
    MakeDummyInstance(wname,def,inst,statement);
  }else{
    if(gl_length(instances)==1){
      child = (struct Instance *)gl_fetch(instances,1);
      asc_assert(InstanceKind(child)==DUMMY_INST);
      gl_destroy(instances);
    }else{
      STATEMENT_ERROR(statement, "Expression name refers to more than one object");
      gl_destroy(instances);
      ASC_PANIC("Expression name refers to more than one object");
    }
  }

  w1 = WhenStatCases(statement);
  while (w1!=NULL){
    sl = WhenStatementList(w1);
    ExecuteUnSelectedWhenStatements(inst,sl);
    w1 = NextWhenCase(w1);
  }
  return 1;
}

/*------------------------------------------------------------------------------
  'SELECT' STATEMENT PROCESSING
*/

/**
	Execution of the Statements inside the case that
	matches the selection variables
*/
static
void ExecuteSelectStatements(struct Instance *inst, unsigned long *count,
                             struct StatementList *sl)
{
  struct BitList *blist;
  struct Statement *statement;
  unsigned long c,len;
  int return_value;
  struct gl_list_t *list;

  blist = InstanceBitList(inst);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    (*count)++;
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
      case ALIASES:
        return_value = ExecuteALIASES(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case CASGN:
        return_value =  ExecuteCASGN(inst,statement);
        if (return_value) {
          ClearBit(blist,*count);
        }
        break;
      case ARR:
        return_value = ExecuteISA(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case ISA:
        return_value = ExecuteISA(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case IRT:
        return_value = ExecuteIRT(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case ATS:
        return_value = ExecuteATS(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case AA:
        return_value = ExecuteAA(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case LNK:
        return_value = ExecuteLNK(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case UNLNK:
        FPRINTF(ASCERR,"UNLINK are only allowed inside a non-declarative part of the \n");
        break;
      case FOR:
        return_value = Pass1ExecuteFOR(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case TABLESTAT:
        return_value = ExecuteTABLE(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case DATASETSTAT:
        return_value = ExecuteDATASET(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case ASGN:
      case REL:
      case EXT:
      case LOGREL:
      case COND:
      case CALL:
      case WHEN:
        return_value = 1;  /* ignore'm */
        ClearBit(blist,*count);
        break;
      case FNAME:
        if (g_iteration>=MAXNUMBER) {
          STATEMENT_ERROR(statement,
              "FNAME not allowed inside a SELECT Statement");
        }
        return_value = 1; /* Ignore it */
        ClearBit(blist,*count);
        break;
      case SELECT:
        return_value = ExecuteSELECT(inst,count,statement);
        break;
      default:
        WSEM(stderr,statement,
           "Inappropriate statement type in declarative section SELECT\n");
        Asc_Panic(2, NULL,
                  "Inappropriate statement type"
                  " in declarative section SELECT");
    }
  }
}


/**
	Execution of the UnSelected Statements inside those cases of the
	SELECT that do not match match the selection variables
*/
static
void ExecuteUnSelectedStatements(struct Instance *inst,unsigned long *count,
                                 struct StatementList *sl)
{
  struct BitList *blist;
  struct Statement *statement;
  unsigned long c,len;
  int return_value;
  struct gl_list_t *list;

  blist = InstanceBitList(inst);
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    (*count)++;
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
      case ARR:
      case IRT:
      case ATS:
      case AA:
      case LNK:
      case UNLNK:
      case CALL:
      case CASGN:
      case ASGN:
      case TABLESTAT:
      case DATASETSTAT:
        ClearBit(blist,*count);
        break;
      case FNAME:
        if (g_iteration>=MAXNUMBER) {
          STATEMENT_ERROR(statement,"FNAME not allowed inside a SELECT Statement");
        }
        return_value = 1; /*ignore it */
        ClearBit(blist,*count);
        break;
      case ALIASES:
        return_value = ExecuteUnSelectedALIASES(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case ISA:
        return_value = ExecuteUnSelectedISA(inst,statement);
        if (return_value) ClearBit(blist,*count);
        break;
      case FOR:
        return_value = ExecuteUnSelectedForStatements(inst,
                       ForStatStmts(statement));
        if (return_value) ClearBit(blist,*count);
        break;
      case REL:
      case EXT:
      case LOGREL:
        return_value = ExecuteUnSelectedEQN(inst,statement);
        ClearBit(blist,*count);
        break;
      case COND:
        return_value = ExecuteUnSelectedCOND(inst,statement);
        ClearBit(blist,*count);
        break;
      case WHEN:
        return_value = ExecuteUnSelectedWHEN(inst,statement);
        ClearBit(blist,*count);
        break;
      case SELECT:
        return_value = ExecuteUnSelectedSELECT(inst,count,statement);
        break;
      default:
        WSEM(stderr,statement,
          "Inappropriate statement type in declarative section unSELECTed\n");
        ASC_PANIC("Inappropriate statement type"
                  " in declarative section unSELECTed\n");
    }
  }
}

/**
	Execution of the SELECT statement inside a case that does not
	match the selection variables
*/
static
int ExecuteUnSelectedSELECT(struct Instance *inst, unsigned long *c,
                            struct Statement *statement)
{
  struct BitList *blist;
  struct SelectList *sel1;
  struct StatementList *sl;

  blist = InstanceBitList(inst);
  ClearBit(blist,*c);
  sel1 = SelectStatCases(statement);
  while (sel1!=NULL){
    sl = SelectStatementList(sel1);
    ExecuteUnSelectedStatements(inst,c,sl);
    sel1 = NextSelectCase(sel1);
  }
  return 1;
}


/**
	Compare current values of the selection variables with
	the set of values in a CASE of a SELECT statement, and try to find
	is such values are the same. If they are, the function will return 1,
	else, it will return 0.
*/
static
int AnalyzeSelectCase(struct Instance *ref, struct VariableList *vlist,
                      struct Set *s)
{
  CONST struct Expr *expr;
  CONST struct Name *name;
  symchar *value;
  symchar *symvar;
  CONST struct VariableList *vl;
  CONST struct Set *values;
  int val;
  int valvar;
  struct gl_list_t *instances;
  struct Instance *inst;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;

  asc_assert(s!= NULL);
  asc_assert(vlist != NULL);
  values = s;
  vl = vlist;

  while (vl!=NULL) {
    name = NamePointer(vl);
    expr = GetSingleExpr(values);
    instances = FindInstances(ref,name,&err);
    asc_assert(gl_length(instances)==1);
    inst = (struct Instance *)gl_fetch(instances,1);
    gl_destroy(instances);
    switch(ExprType(expr)) {
      case e_boolean:
        val =  ExprBValue(expr);
        if (val == 2) { /* ANY */
          break;
        }
        valvar = GetBooleanAtomValue(inst);
        if (val != valvar) return 0;
        break;
      case e_int:
        asc_assert(InstanceKind(inst)==INTEGER_CONSTANT_INST);
        val =  ExprIValue(expr);
        valvar = GetIntegerAtomValue(inst);
        if (val != valvar) return 0;
        break;
      case e_symbol:
        asc_assert(InstanceKind(inst)==SYMBOL_CONSTANT_INST);
        symvar = ExprSymValue(expr);
        value = GetSymbolAtomValue(inst);
        if (symvar != value) {
          asc_assert(AscFindSymbol(symvar)!=NULL);
          return 0;
        }
        break;
      default:
        FPRINTF(stderr,"Something wrong happens in AnalyzeSelectCase \n");
        return 0;
    }
    vl = NextVariableNode(vl);
    values = NextSet(values);
  }

  return 1;
}


/**
	This function will determine which case of a SELECT statement
	applies for the current values of the selection variables.
	this function  will call for the execution of the case which
	matches. It handles OTHERWISE properly (case when set == NULL).
	At most one case is going to be executed.
*/
static
void RealExecuteSELECT(struct Instance *inst, unsigned long *c,
                       struct Statement *statement)
{
  struct VariableList *vlist;
  struct SelectList *sel1;
  struct Set *set;
  struct StatementList *sl;
  int case_match;

  vlist = SelectStatVL(statement);
  sel1 = SelectStatCases(statement);
  case_match =0;

  while (sel1!=NULL){
    set = SelectSetList(sel1);
    sl = SelectStatementList(sel1);
    if (case_match==0) {
      if (set != NULL) {
        case_match = AnalyzeSelectCase(inst,vlist,set);
        if (case_match==1) {
          ExecuteSelectStatements(inst,c,sl);
        }else{
          ExecuteUnSelectedStatements(inst,c,sl);
        }
      }else{
        ExecuteSelectStatements(inst,c,sl);
        case_match = 1;
      }
    }else{
      ExecuteUnSelectedStatements(inst,c,sl);
    }
    sel1 = NextSelectCase(sel1);
  }

  if (case_match == 0) {
    FPRINTF(ASCERR,"No case matched in SELECT statement\n");
  }
}


/**
	If A SELECT statement passess its checking, this function
	will ask for its execution, otherwise the SELECT and all
	the other statements inside of it, will not be touched.
	The counter in the bitlist is increased properly.
	NOTE for efficiency: Maybe we should integrate the
	Check function of the SELECT together with the analysis
	of the CASEs to see which of them matches.We are doing
	twice the execution of some C functions.
*/
static
int ExecuteSELECT(struct Instance *inst, unsigned long *c,
                  struct Statement *statement)
{
  unsigned long tmp;
  struct BitList *blist;

  blist = InstanceBitList(inst);
  if (CheckSELECT(inst,statement)){
    ClearBit(blist,*c);
    RealExecuteSELECT(inst,c,statement);
    return 1;
  }else{
    tmp = SelectStatNumberStats(statement);
    *c = (*c) + tmp;
    return 0;
  }
}


/**
	This function jumps the statements inside non-matching
	cases of a SELECT statement, so that they are not analyzed
	in compilation passes > 1.
	If there is a SELECT inside a SELECT,
	the function uses the number of statements in the nested
	SELECTs
*/
static
void JumpSELECTStats(unsigned long *count,struct StatementList *sl)
{
  unsigned long c,length;
  int tmp;
  struct Statement *s;

  length = StatementListLength(sl);
  *count = (*count) + length;
  for(c=1;c<=length;c++){
    tmp = 0;
    s = GetStatement(sl,c);
    asc_assert(s!=NULL);
    switch(StatementType(s)) {
      case SELECT:
        tmp = SelectStatNumberStats(s);
        break;
      default:
        break;
    }
    *count = (*count) + tmp;
  }
  return;
}

/**
	This function is used only for setting  the
	bits ON for some statements in the matching case of a
	SELECT statement. Only these statements will be
	analyzed in Pass > 1. The conditions to set a bit ON
	depend on the number of pass.
*/
static
void SetBitsOnOfSELECTStats(struct Instance *inst, unsigned long *count,
                            struct StatementList *sl, int pass, int *changed)
{
  unsigned long c,length;
  struct Statement *s;
  struct BitList *blist;

  blist = InstanceBitList(inst);
  length = StatementListLength(sl);
  for(c=1;c<=length;c++){
    s = GetStatement(sl,c);
    asc_assert(s!=NULL);
    (*count)++;
    switch (pass) {
      case 2:
        switch(StatementType(s)) {
          case REL:
            SetBit(blist,*count);
            (*changed)++;
            break;
          case EXT:
            SetBit(blist,*count);
            (*changed)++;
            break;
          case COND:
            if (CondContainsRelations(s)) {
              SetBit(blist,*count);
              (*changed)++;
            }
            break;
          case FOR:
            if ( ForContainsRelations(s) ) {
              SetBit(blist,*count);
              (*changed)++;
            }
            break;
          case SELECT:
            if (SelectContainsRelations(s)) {
              ReEvaluateSELECT(inst,count,s,pass,changed);
            }else{
              *count = *count + SelectStatNumberStats(s);
            }
            break;
          default:
            break;
        }
        break;
      case 3:
        switch(StatementType(s)) {
          case LOGREL:
            SetBit(blist,*count);
            (*changed)++;
            break;
          case COND:
            if (CondContainsLogRelations(s)) {
              SetBit(blist,*count);
              (*changed)++;
            }
            break;
          case FOR:
            if ( ForContainsLogRelations(s) ) {
              SetBit(blist,*count);
              (*changed)++;
            }
            break;
          case SELECT:
            if (SelectContainsLogRelations(s)) {
              ReEvaluateSELECT(inst,count,s,pass,changed);
            }else{
              *count = *count + SelectStatNumberStats(s);
            }
            break;
          default:
            break;
        }
        break;
      case 4:
        switch(StatementType(s)) {
          case WHEN:
            SetBit(blist,*count);
            (*changed)++;
            break;
          case FOR:
            if ( ForContainsWhen(s) ) {
              SetBit(blist,*count);
              (*changed)++;
            }
            break;
          case SELECT:
            if (SelectContainsWhen(s)) {
              ReEvaluateSELECT(inst,count,s,pass,changed);
            }else{
              *count = *count + SelectStatNumberStats(s);
            }
            break;
          default:
            break;
        }
				case 5:			/**> DS: Added support for LINK statements inside SELECTs */
        switch(StatementType(s)) {
          case WHEN:
            SetBit(blist,*count);
            (*changed)++;
            break;
          case FOR:
            if ( ForContainsLink(s) ) {
              SetBit(blist,*count);
              (*changed)++;
            }
            break;
          case SELECT:
            if (SelectContainsLink(s)) {
              ReEvaluateSELECT(inst,count,s,pass,changed);
            }else{
              *count = *count + SelectStatNumberStats(s);
            }
            break;
          default:
            break;
        }
        break;
      default:
        FPRINTF(ASCERR,"Wrong pass Number in SetBitsOnOfSELECTStats \n");
        break;
    }
  }
  return;
}


/**
	This function will determine which case of a SELECT statement
	applies for the current values of the selection variables.
	Similar performance from RealExecuteSELECT, but this function
	does not call for execution, it is used only for "jumping"
	the statements inside a non-matching case, or seting the
	bits on for some statements in the matching case.
	It handles OTHERWISE properly (case when set == NULL).
*/
static
void SetBitOfSELECTStat(struct Instance *inst, unsigned long *c,
                        struct Statement *statement, int pass, int *changed)
{
  struct VariableList *vlist;
  struct SelectList *sel1;
  struct Set *set;
  struct StatementList *sl;
  int case_match;

  vlist = SelectStatVL(statement);
  sel1 = SelectStatCases(statement);
  case_match =0;

  while (sel1!=NULL){
    set = SelectSetList(sel1);
    sl = SelectStatementList(sel1);
    if (case_match==0) {
      if (set != NULL) {
        case_match = AnalyzeSelectCase(inst,vlist,set);
        if (case_match==1) {
          SetBitsOnOfSELECTStats(inst,c,sl,pass,changed);
        }else{
          JumpSELECTStats(c,sl);
        }
      }else{
        SetBitsOnOfSELECTStats(inst,c,sl,pass,changed);
        case_match = 1;
      }
    }else{
      JumpSELECTStats(c,sl);
    }
    sel1 = NextSelectCase(sel1);
  }
}

/**
	For compilation passes > 1, this function will tell me if I
	should Set the Bits on for statements inside the CASEs of
	a SELECT statement. This evaluation is needed because there may be
	relations, whens or log rels that should not be executed
	at all (when the selection variables do not exist, for example)
	or should  not be reanlyzed in pass2 3 and 4 (when they are
	already dummys, for example). This re-evaluation will not be done
	if the SELECT does not contain rels, logrels or when.
	NOTE for efficiency: Maybe we should integrate the
	Check function of the SELECT together with the analysis
	of the CASEs to see which of them matches.We are doing
	twice the execution of some C functions.
*/
static
void ReEvaluateSELECT(struct Instance *inst, unsigned long *c,
                      struct Statement *statement, int pass, int *changed)
{
  unsigned long tmp;
  //struct BitList *blist;

  //blist = InstanceBitList(inst);
  if (CheckSELECT(inst,statement)){
    SetBitOfSELECTStat(inst,c,statement,pass,changed);
  }else{
    tmp = SelectStatNumberStats(statement);
    *c = (*c) + tmp;
  }
  return;
}


/**
	This function is used only for setting  the
	bits ON for some statements in the matching case of a
	SELECT statement. Only these statements will be
	analyzed in Pass > 1. The conditions to set a bit ON
	depend on the number of pass.
*/
static
void ExecuteDefaultsInSELECTCase(struct Instance *inst, unsigned long *count,
                                 struct StatementList *sl,
                                 unsigned long int *depth)
{
  unsigned long c,length;
  struct Statement *s;
  struct for_table_t *SavedForTable;

  length = StatementListLength(sl);
  for(c=1;c<=length;c++){
    s = GetStatement(sl,c);
    asc_assert(s!=NULL);
    (*count)++;
    switch(StatementType(s)) {
      case ASGN:
        ExecuteDefault(inst,s,depth);
        break;
      case FOR:
        if ( ForContainsDefaults(s) ){
          SavedForTable = GetEvaluationForTable();
          SetEvaluationForTable(CreateForTable());
          RealDefaultFor(inst,s,depth);
          DestroyForTable(GetEvaluationForTable());
          SetEvaluationForTable(SavedForTable);
        }
        break;
      case SELECT:
        ExecuteDefaultsInSELECT(inst,count,s,depth);
        break;
      default:
        break;
    }
  }
  return;
}


/**
	This function will determine which case of a SELECT statement
	applies for the current values of the selection variables.
	Similar performance from RealExecuteSELECT. This function
	is used only for "jumping"  the statements inside a non-matching
	case, or Executing Defaults in the matching case.
	It handles OTHERWISE properly (case when set == NULL).
*/
static
void ExecuteDefaultsInSELECTStat(struct Instance *inst, unsigned long *c,
                                 struct Statement *statement,
                                 unsigned long int *depth)
{
  struct VariableList *vlist;
  struct SelectList *sel1;
  struct Set *set;
  struct StatementList *sl;
  int case_match;

  vlist = SelectStatVL(statement);
  sel1 = SelectStatCases(statement);
  case_match =0;

  while (sel1!=NULL){
    set = SelectSetList(sel1);
    sl = SelectStatementList(sel1);
    if (case_match==0) {
      if (set != NULL) {
        case_match = AnalyzeSelectCase(inst,vlist,set);
        if (case_match==1) {
          ExecuteDefaultsInSELECTCase(inst,c,sl,depth);
        }else{
          JumpSELECTStats(c,sl);
        }
      }else{
        ExecuteDefaultsInSELECTCase(inst,c,sl,depth);
        case_match = 1;
      }
    }else{
      JumpSELECTStats(c,sl);
    }
    sel1 = NextSelectCase(sel1);
  }
}

/**
	For Execution of Defaults, which uses a Visit Instance Tree instead of
	a BitList. this function will tell me if I
	should Set the Bits on for statements inside the CASEs of
	a SELECT statement. This evaluation is needed because there is
	the possibility of different assignments to the same variable in
	different cases of the select. I need to execute only those in
	cases mathing the selection variables.

	@TODO It is becoming annoying to have so similar functions, I need
	to create a robust and general function which considers all the
	possible applications.
*/
static
void ExecuteDefaultsInSELECT(struct Instance *inst, unsigned long *c,
                             struct Statement *statement,
                             unsigned long int *depth)
{
  unsigned long tmp;

  if (CheckSELECT(inst,statement)){
    ExecuteDefaultsInSELECTStat(inst,c,statement,depth);
  }else{
    tmp = SelectStatNumberStats(statement);
    *c = (*c) + tmp;
  }
  return;
}

/*------------------------------------------------------------------------------
  'FOR' STATEMENT PROCESSING
*/
static
void WriteForValueError(struct Statement *statement, struct value_t value)
{
  switch(ErrorValue(value)){
  case type_conflict:
    STATEMENT_ERROR(statement, "Type conflict in FOR expression");
    break;
  case incorrect_name:
    STATEMENT_ERROR(statement, "Impossible instance in FOR expression");
    break;
  case temporary_variable_reused:
    STATEMENT_ERROR(statement, "Temporary variable reused in FOR expression");
    break;
  case dimension_conflict:
    STATEMENT_ERROR(statement, "Dimension conflict in FOR expression");
    break;
  case incorrect_such_that:
    STATEMENT_ERROR(statement, "Incorrect such that expression in FOR expression");
    break;
  case empty_choice:
    STATEMENT_ERROR(statement,
                         "CHOICE is called on an empty set in FOR expression");
    break;
  case empty_intersection:
    STATEMENT_ERROR(statement, "Empty INTERSECTION() in FOR expression");
    break;
  default:
    STATEMENT_ERROR(statement, "Unexpected error in FOR expression");
    break;
  }
}

static
int Pass5ExecuteForStatements(struct Instance *inst,
                              struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case LNK:
      if (!ExecuteLNK(inst,statement))return 0;
      break;
    case UNLNK:
      STATEMENT_ERROR(statement,"Inappropriate statement type in declarative "
        "section (UNLINK only possible in the METHODS section)"
      );
      Asc_Panic(2, NULL,"Inappropriate statement type in declarative section "
      	"(UNLINK only possible in the METHODS section)"
      );
      break;
    case FNAME:
      if (!ExecuteFNAME(inst,statement)) return 0;
      break;
    case FOR:
      if (!Pass5ExecuteFOR(inst,statement)) return 0;
      break;
    case SELECT:
      STATEMENT_ERROR(statement,
           "SELECT statements are not allowed inside a FOR Statement");
      return 0;
    case WHEN:
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case REF:
    case ASGN:
    case CASGN:
    case REL:
    case LOGREL:
    case COND:
    case CALL:
    case TABLESTAT:
    case DATASETSTAT:
    case EXT:  /* ignore'm */
    break;
    default:
      STATEMENT_ERROR(statement,
           "Inappropriate statement type in declarative section LINK");
      Asc_Panic(2, NULL,
                "Inappropriate statement type in declarative section LINK");
    }
  }
  return 1;
}

static
int Pass4ExecuteForStatements(struct Instance *inst,
                              struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case LNK:
      return 1; /* ignore'm until pass 5 */
      break;
    case UNLNK:
      return 0;
      break;
    case WHEN:
      if (!ExecuteWHEN(inst,statement)) return 0;
      break;
    case FNAME:
      if (!ExecuteFNAME(inst,statement)) return 0;
      break;
    case FOR:
      if (!Pass4ExecuteFOR(inst,statement)) return 0;
      break;
    case SELECT:
      STATEMENT_ERROR(statement,
           "SELECT statements are not allowed inside a FOR Statement");
      return 0;
      /* I probably need to change NP4REF to integer */
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case REF:
    case ASGN:
    case CASGN:
    case REL:
    case LOGREL:
    case COND:
    case CALL:
    case TABLESTAT:
    case DATASETSTAT:
    case EXT:  /* ignore'm */
    break;
    default:
      STATEMENT_ERROR(statement,
           "Inappropriate statement type in declarative section WHEN");
      Asc_Panic(2, NULL,
                "Inappropriate statement type in declarative section WHEN");
    }
  }
  return 1;
}


/**
	@NOTE this function must not be called until all the rel,ext
	statements in sl pass their checks.
*/
static
int Pass3ExecuteForStatements(struct Instance *inst,
                              struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  int return_value;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);

  return_value =1;
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case LNK:
    case UNLNK:
    case REF:
    case ASGN:
    case REL:
    case CALL:
    case TABLESTAT:
    case DATASETSTAT:
    case EXT: /* ignore'm */
    case CASGN:
    case WHEN:
      return_value = 1; /* ignore'm until pass 4 */
      break;
    case FNAME:
      STATEMENT_ERROR(statement,
                 "FNAME statements are only allowed inside a WHEN Statement");
      return_value = 0;
      break;
    case SELECT:
      STATEMENT_ERROR(statement,
           "SELECT statements are not allowed inside a FOR Statement");
      return_value = 0;
      break;
    case FOR:
      if ( ForContainsLogRelations(statement) ) {
        return_value = Pass3RealExecuteFOR(inst,statement);
      }
      break;
    case COND:
      STATEMENT_ERROR(statement,
                 "COND not allowed inside a FOR. Try FOR inside COND");
      return_value = 0;
      break;
    case LOGREL:
      if (ExecuteLOGREL(inst,statement)) {
        return_value = 1;
      }else{
        return_value = 0;
      }
      break;
    default:
      STATEMENT_ERROR(statement,
           "Inappropriate statement type in declarative section log rel\n");
      ASC_PANIC("Inappropriate statement type"
                " in declarative section log rel\n");
    }
    if (!return_value) {
      return 0;
    }
  }
  return 1;
}


/* Note: this function must not be called until all the rel,ext
 * statements in sl pass their checks.
 * This is because if any of the Executes fail
 * (returning 0) we abort (at least when assert is active).
 */
static
void Pass2ExecuteForStatements(struct Instance *inst,
                               struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  int return_value = 0;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case LNK:
    case UNLNK:
    case CALL:
    case REF:
    case ASGN: /* ignore'm */
    case CASGN:
    case LOGREL:
    case TABLESTAT:
    case DATASETSTAT:
      return_value = 1; /* ignore'm until pass 3 */
      break;
    case WHEN:
      return_value = 1; /* ignore'm until pass 4 */
      break;
    case SELECT:
      STATEMENT_ERROR(statement,
           "SELECT statements are not allowed inside a FOR Statement");
      return_value = 0;
      break;
    case FNAME:
      STATEMENT_ERROR(statement,
                 "FNAME statements are only allowed inside a WHEN Statement");
      return_value = 0;
      break;
    case FOR:
      return_value = 1;
      if ( ForContainsRelations(statement) ) {
#ifdef DEBUG_RELS
        ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
        WriteStatement(stderr, statement, 6);
        error_reporter_end_flush();
#endif
        Pass2RealExecuteFOR(inst,statement);
        /* p2ref expected to succeed or fail permanently.
         * if it doesn't, this needs fixing.
         */
      }
      break;
    case COND:
      STATEMENT_ERROR(statement,
                 "COND not allowed inside a FOR. Try FOR inside COND");
      return_value = 0;
      break;
    case REL:
#ifdef DEBUG_RELS
       ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
       WriteStatement(stderr, statement, 6);
       error_reporter_end_flush();
#endif
      return_value = ExecuteREL(inst,statement);
      /* ER expected to succeed or fail permanently,returning 1.
       * if it doesn't, this needs fixing.
       */
      break;
    case EXT:
      return_value = ExecuteEXT(inst,statement);
      /* ER expected to succeed or fail permanently,returning 1.
       * if it doesn't, this needs fixing.
       */
      if (return_value == 0) {
        STATEMENT_ERROR(statement,"Impossible external relation encountered unexpectedly.");
      }
      break;
    default:
      STATEMENT_ERROR(statement,
      "Inappropriate statement type in declarative section relations");
      ASC_PANIC("Inappropriate statement type"
                " in declarative section relations");
    }
    if(!return_value){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,
        "while running %s on statement inside FOR", __FUNCTION__);
      return;
    }
  }
}


/**
	@NOTE this function must not be called until all the statements in sl
	(except rel, ext)pass their checks.
	This is because if any of the Executes fail
	(returning 0) we abort (at least when assert is active)
*/
static
void Pass1ExecuteForStatements(struct Instance *inst,
                               struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  int return_value = 0;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case ALIASES:
      return_value = ExecuteALIASES(inst,statement);
      break;
    case ARR:
      return_value = ExecuteARR(inst,statement);
      break;
    case ISA:
      return_value = ExecuteISA(inst,statement);
      break;
    case IRT:
      return_value = ExecuteIRT(inst,statement);
      break;
    case ATS:
      return_value = ExecuteATS(inst,statement);
      break;
    case AA:
      return_value = ExecuteAA(inst,statement);
      break;
    case LNK:
      return_value = 1; /*DS: ignore'm until pass 5*/
      break;
    case UNLNK:
      STATEMENT_ERROR(statement,
        "UNLINK statements are not allowed in the declarative section");
      return_value = 0;
      break;
    case FOR:
      return_value = 1;
      Pass1RealExecuteFOR(inst,statement);
      break;
    case REL:
    case CALL:
    case EXT:
    case ASGN: /* ignore'm */
    case LOGREL:
    case COND:
    case WHEN:
      return_value = 1; /* ignore'm until pass 2, 3 or 4 */
      break;
    case REF:
      return_value = ExecuteREF(inst,statement);
      break;
    case CASGN:
      return_value = ExecuteCASGN(inst,statement);
      break;
    case TABLESTAT:
      return_value = ExecuteTABLE(inst,statement);
      break;
    case DATASETSTAT:
      return_value = ExecuteDATASET(inst,statement);
      break;
    case FNAME:
      STATEMENT_ERROR(statement,
                "FNAME statements are only allowed inside a WHEN Statement");
      return_value = 0;
      break;
    case SELECT:
      STATEMENT_ERROR(statement,
                "SELECT statements are not allowed inside a FOR Statement");
      return_value = 0;
      break;
    default:
      STATEMENT_ERROR(statement,
           "Inappropriate statement type in declarative section");
      Asc_Panic(2, NULL,
                "Inappropriate statement type in declarative section");
    }
    asc_assert(return_value);
  }
}


/**
	Execute UnSelected statements inside a FOR loop
	Note that we are not expanding arrays. This actually
	may be impossible even if we want to do it.
*/
static
int ExecuteUnSelectedForStatements(struct Instance *inst,
                                   struct StatementList *sl)
{
  struct Statement *statement;
  unsigned long c,len;
  //int return_value;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
      case ARR:
      case IRT:
      case ATS:
      case AA:
      case LNK:
      case UNLNK:
      case CALL:
      case CASGN:
      case ASGN:
      case TABLESTAT:
      case DATASETSTAT:
        break;
      case FNAME:
        if (g_iteration>=MAXNUMBER) {
          STATEMENT_ERROR(statement,
              "FNAME not allowed inside a SELECT Statement");
        }
        //return_value = 1; /*ignore it */
        break;
      case ALIASES:
        ExecuteUnSelectedALIASES(inst,statement);
        break;
      case ISA:
        ExecuteUnSelectedISA(inst,statement);
        break;
      case FOR:
        ExecuteUnSelectedForStatements(inst,
                                 ForStatStmts(statement));
        break;
      case REL:
      case EXT:
      case LOGREL:
        ExecuteUnSelectedEQN(inst,statement);
        break;
      case WHEN:
        ExecuteUnSelectedWHEN(inst,statement);
        break;
      case COND:
        STATEMENT_ERROR(statement,
        "CONDITIONAL not allowed inside a FOR loop. Try FOR inside COND");
        ASC_PANIC("CONDITIONAL not allowed inside a FOR loop."
                  " Try FOR inside COND");
      case SELECT:
        STATEMENT_ERROR(statement, "SELECT not allowed inside a FOR Statement");
        ASC_PANIC("SELECT not allowed inside a FOR Statement");
        break;
      default:
        WSEM(stderr,statement,
             "Inappropriate statement type in declarative section unSEL FOR");
        ASC_PANIC("Inappropriate statement type in"
                  " declarative section unSEL FOR");
    }
  }
  return 1;
}

static
int Pass5RealExecuteFOR(struct Instance *inst, struct Statement *statement)
{
	printf("\n Pass5RealExecuteFOR called \n");
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return 0;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Phase 5 FOR has undefined values");
      return 0;
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      return 0;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    DestroyValue(&value);
    return 0;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        if (!Pass5ExecuteForStatements(inst,sl)) return 0;
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        if (!Pass5ExecuteForStatements(inst,sl)) return 0;
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
  return 1;
}


static
int Pass4RealExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return 0;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Phase 4 FOR has undefined values");
      return 0;
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      return 0;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    DestroyValue(&value);
    return 0;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        if (!Pass4ExecuteForStatements(inst,sl)) {
           RemoveForVariable(GetEvaluationForTable());
           DestroyValue(&value);
           return 0 ;
        /*  currently designed to always succeed or fail permanently */
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        if (!Pass4ExecuteForStatements(inst,sl)) {
           RemoveForVariable(GetEvaluationForTable());
           DestroyValue(&value);
           return 0 ;
        /*  currently designed to always succeed or fail permanently */
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
  /*  currently designed to always succeed or fail permanently.
   *  We reached this point meaning we've processed everything.
   *  Therefore the statment returns 1 and becomes no longer pending.
   */
  return 1;
}

static
void MakeRealWhenCaseReferencesFOR(struct Instance *inst,
                                   struct Instance *child,
                                   struct Statement *statement,
                                   struct gl_list_t *listref)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return ;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Phase 2 FOR has undefined values");
      break;
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      break;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    DestroyValue(&value);
    break;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        MakeRealWhenCaseReferencesList(inst,child,sl,listref);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        MakeRealWhenCaseReferencesList(inst,child,sl,listref);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
}

/**
	@TODO this function needs to be made much less aggressive about exiting
	and more verbose about error messages  so we can skip the np3checkfor
	probably also means it needs the 0/1 fail/succeed return code.
*/
static
int Pass3RealExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return 0;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Phase 3 FOR has undefined values");
      return 0;
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      return 0;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    DestroyValue(&value);
    return 0;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        if (!Pass3ExecuteForStatements(inst,sl)) return 0;
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        if (!Pass3ExecuteForStatements(inst,sl)) return 0;
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
  return 1;
}


static
void Pass3FORMarkCondLogRels(struct Instance *inst,
                             struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return ;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Phase 3 FOR has undefined values");
      break;
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      break;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    DestroyValue(&value);
    break;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        Pass3MarkCondLogRelStatList(inst,sl);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        Pass3MarkCondLogRelStatList(inst,sl);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
}

static
void Pass3FORMarkCond(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;

  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  Pass3FORMarkCondLogRels(inst,statement);
  DestroyForTable(GetEvaluationForTable());
  SetEvaluationForTable(SavedForTable);
}


/* This function requires all the preconditions for success have
 * been checked. Verifying correctness is separate from and prior to building
 * instance data. This pattern is followed throughout instantiation.
 */
static
int Pass2RealExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return 0;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Phase 2 FOR has undefined values");
      return 0;
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      return 0;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    error_reporter_end_flush();
    DestroyValue(&value);
    return 0;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set:
#ifdef DEBUG_RELS
      ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"Pass2RealExecuteFOR empty_set.\n");
#endif
      break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
#ifdef DEBUG_RELS
      ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"Pass2RealExecuteFOR integer_set %lu.\n",len);
#endif
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        Pass2ExecuteForStatements(inst,sl);
        /*  currently designed to always succeed or fail permanently */
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
#ifdef DEBUG_RELS
      ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"Pass2RealExecuteFOR string_set %lu.\n",len);
#endif
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        Pass2ExecuteForStatements(inst,sl);
        /*  currently designed to always succeed or fail permanently */
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
  /*  currently designed to always succeed or fail permanently.
   *  We reached this point meaning we've processed everything.
   *  Therefore the statment returns 1 and becomes no longer pending.
   */
  return 1;
}

static
void Pass2FORMarkCondRelations(struct Instance *inst,
                               struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return ;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "Phase 2 FOR has undefined values");
      break;
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      break;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    DestroyValue(&value);
    break;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        Pass2MarkCondRelStatList(inst,sl);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        Pass2MarkCondRelStatList(inst,sl);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
}

static
void Pass2FORMarkCond(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;

  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  Pass2FORMarkCondRelations(inst,statement);
  DestroyForTable(GetEvaluationForTable());
  SetEvaluationForTable(SavedForTable);
}

static
void Pass1RealExecuteFOR(struct Instance *inst, struct Statement *statement)
{
	/*printf("\n Pass1RealExecuteFOR called \n");*/
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable */
    STATEMENT_ERROR(statement, "FOR construct uses duplicate index variable");
    return;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      STATEMENT_ERROR(statement, "FOR has undefined values");
      ASC_PANIC("FOR has undefined values");
    default:
      WriteForValueError(statement,value);
      DestroyValue(&value);
      return;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    WriteStatement(ASCERR,statement,0);
    FPRINTF(ASCERR,"FOR expression returns the wrong type.\n");
    DestroyValue(&value);
    return;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        Pass1ExecuteForStatements(inst,sl);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        Pass1ExecuteForStatements(inst,sl);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
}

static
int Pass4CheckFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)) return 1; /* will give error */
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      return 0;
    default:
      DestroyValue(&value);
      return 1;			/* will give an error */
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    DestroyValue(&value);
    return 1;			/* will give error */
  case set_value:		/* okay thus far */
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;	/* always okay */
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        if (!Pass4CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        if (!Pass4CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
    return 1;			/* everything checks out */
  }
  /*NOTREACHED*/
  return 0; /* we here? */
}

static
int Pass4RealCheckFOR (struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if (Pass4CheckFOR(inst,statement)) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}

static
int Pass3CheckFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)) {
    return 1; /* will give error */
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      return 0;
    default:
      DestroyValue(&value);
      return 1;			/* will give an error */
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    DestroyValue(&value);
    return 1;			/* will give error */
  case set_value:		/* okay thus far */
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;	/* always okay */
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        if (!Pass3CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        if (!Pass3CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
    return 1;			/* everything checks out */
  }
  /*NOTREACHED*/
  return 0; /* we here? */
}

static
int Pass3RealCheckFOR (struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if (Pass3CheckFOR(inst,statement)) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}


/**
	a currently unused function, with therefore unused subsidiary functions
*/
static
int Pass2CheckFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)) return 1; /* will give error */
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      return 0;
    default:
      DestroyValue(&value);
      return 1;			/* will give an error */
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    DestroyValue(&value);
    return 1;			/* will give error */
  case set_value:		/* okay thus far */
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;	/* always okay */
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        if (!Pass2CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        if (!Pass2CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
    return 1;			/* everything checks out */
  }
  /*NOTREACHED*/
  return 0; /* we here? */
}

static
int Pass2RealCheckFOR (struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if (Pass2CheckFOR(inst,statement)) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}

/* checks every statement against every value of the loop index */
static
int Pass1CheckFOR(struct Instance *inst, struct Statement *statement)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  name = ForStatIndex(statement);
  ex = ForStatExpr(statement);
  sl = ForStatStmts(statement);
  if (FindForVar(GetEvaluationForTable(),name)) return 1; /* will give error */
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(inst);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      return 0;
    default:
      DestroyValue(&value);
      return 1;			/* will give an error */
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    DestroyValue(&value);
    return 1;			/* will give error */
  case set_value:		/* okay thus far */
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;	/* always okay */
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        if (!Pass1CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        if (!Pass1CheckStatementList(inst,sl)){
          RemoveForVariable(GetEvaluationForTable());
          DestroyValue(&value);
          return 0;
        }
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
    return 1;			/* everything checks out */
  }
  /*NOTREACHED*/
  return 0; /* we here? */
}


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
int Pass1RealCheckFOR(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if (Pass1CheckFOR(inst,statement)){
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}
#endif  /*  THIS_IS_AN_UNUSED_FUNCTION  */



static
int Pass5ExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if ( Pass5RealExecuteFOR(inst,statement) ) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}



static
int Pass4ExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if ( Pass4RealExecuteFOR(inst,statement) ) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}

static
void MakeWhenCaseReferencesFOR(struct Instance *inst,
                               struct Instance *child,
                               struct Statement *statement,
                               struct gl_list_t *listref)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
    MakeRealWhenCaseReferencesFOR(inst,child,statement,listref);
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return;
}

static
int Pass3ExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if ( Pass3RealExecuteFOR(inst,statement) ) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}

/* This function is seen exactly once when handling a set of
nested for loops in the same model.
In pass2 models are handled from a list rather than recursively,
so the SavedForTable should always be null (as best I can tell).
BAA; 8/2006.
*/
static
int Pass2ExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if ( Pass2RealExecuteFOR(inst,statement) ) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}

static
int Pass1ExecuteFOR(struct Instance *inst, struct Statement *statement)
{
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  if (Pass1CheckFOR(inst,statement)){
    Pass1RealExecuteFOR(inst,statement);
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 1;
  }else{
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
    return 0;
  }
}

/*------------------------------------------------------------------------------
  GENERAL STATEMENT PROCESSING
*/

static
int Pass5ExecuteStatement(struct Instance *inst,struct Statement *statement)
{
  switch(StatementType(statement)){ /* should be a LNK statement */
  case LNK:
    return ExecuteLNK(inst,statement);
  case UNLNK:
    STATEMENT_ERROR(statement," UNLINK statement not allowed in the declarative section");
    return 0;
  case FOR:
    return Pass5ExecuteFOR(inst,statement);
  case TABLESTAT:
    return 1;
  case DATASETSTAT:
    return 1;
  default:
    return 1;
    /* For anything else but a LINK and FOR statement */
  }
}

static
int Pass4ExecuteStatement(struct Instance *inst,struct Statement *statement)
{
  switch(StatementType(statement)){ /* should be a WHEN statement */
  case WHEN:
    return ExecuteWHEN(inst,statement);
  case FOR:
    return Pass4ExecuteFOR(inst,statement);
  case TABLESTAT:
    return 1;
  case DATASETSTAT:
    return 1;
  case LNK:
    return 1; /* automatically assume done */
  case UNLNK:
    return 1;
  default:
    return 1;
    /* For anything else but a WHEN and FOR statement */
  }
}

static
int Pass3ExecuteStatement(struct Instance *inst,struct Statement *statement)
{
  switch(StatementType(statement)){ /* should be an if relinstance */
  case FOR:
    return Pass3ExecuteFOR(inst,statement);
  case LOGREL:
    return ExecuteLOGREL(inst,statement);
  case COND:
    return Pass3ExecuteCOND(inst,statement);
  case TABLESTAT:
    return 1;
  case DATASETSTAT:
    return 1;
  case WHEN:
    return 1; /* assumed done  */
  case LNK:
    return 1; /* assumed done */
  case UNLNK:
    return 1;
  case FNAME:
    STATEMENT_ERROR(statement,"FNAME are allowed only inside a WHEN statement");
    return 0;
  default:
    return 0;
    /* Nondeclarative statements flagged in pass3 */
  }
}

static
int Pass2ExecuteStatement(struct Instance *inst,struct Statement *statement)
{
  switch(StatementType(statement)){ /* should be an if relinstance */
  case FOR:
#ifdef DEBUG_RELS
    ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
    WriteStatement(stderr, statement, 3);
    error_reporter_end_flush();
#endif
    return Pass2ExecuteFOR(inst,statement);
  case REL:
#ifdef DEBUG_RELS
    ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
    WriteStatement(stderr, statement, 3);
    error_reporter_end_flush();
#endif
    /* ER expected to succeed or fail permanently. this may change. */
    return ExecuteREL(inst,statement);
  case EXT:
	/* CONSOLE_DEBUG("ABOUT TO EXECUTEEXT"); */
    return ExecuteEXT(inst,statement);
  case COND:
    return Pass2ExecuteCOND(inst,statement);
  case TABLESTAT:
    return 1;
  case DATASETSTAT:
    return 1;
  case LOGREL:
		return 1; /* assumed done */
  case WHEN:
    return 1; /* assumed done  */
  case LNK:
    return 1; /* assumed done */
  case UNLNK:
    return 0; /* meaning what??? FIXME */
  case FNAME:
    STATEMENT_ERROR(statement,"FNAME are allowed only inside a WHEN statement");
    return 0;
  default:
    return 0;
    /* Nondeclarative statements flagged in pass2 */
  }
}

static
int Pass1ExecuteStatement(struct Instance *inst, unsigned long *c,
                          struct Statement *statement)
{
  switch(StatementType(statement)){
  case ALIASES:
    return ExecuteALIASES(inst,statement);
  case ARR:
    return ExecuteARR(inst,statement);
  case ISA:
    return ExecuteISA(inst,statement);
  case IRT:
    return ExecuteIRT(inst,statement);
  case ATS:
    return ExecuteATS(inst,statement);
  case AA:
    return ExecuteAA(inst,statement);
  case FOR:
    return Pass1ExecuteFOR(inst,statement);
  case TABLESTAT:
    return ExecuteTABLE(inst,statement);
  case DATASETSTAT:
    return ExecuteDATASET(inst,statement);
  case REL:
    return 1; /* automatically assume done */
  case CALL:
  case EXT:
    return 1; /* automatically assume done */
  case REF:
    return ExecuteREF(inst,statement);
  case CASGN:
    return ExecuteCASGN(inst,statement);
  case ASGN: /* don't do these in instantiation phase. just mark off */
    return 1;
  case LOGREL:
    return 1; /* automatically assume done */
  case COND:
    return 1; /* automatically assume done */
  case WHEN:
    return 1; /* automatically assume done */
  case LNK:
    return 1; /* automatically assume done */
  case UNLNK:
    STATEMENT_ERROR(statement,"UNLINK is allowed only inside the declarative part of the model; statement not executed");
    return 0;
  case FNAME:
    STATEMENT_ERROR(statement,"FNAME are allowed only inside a WHEN statement");
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"while running %s",__FUNCTION__);
    return 0;
  case SELECT:
    return ExecuteSELECT(inst,c,statement);
  default:
    STATEMENT_ERROR(statement,
                       "Inappropriate statement type in declarative section");
    ASC_PANIC("Inappropriate statement type in declarative section");
  }
  return 0;
}


static
int ArraysExpanded(struct Instance *work)
{
  unsigned long c,len;
  struct Instance *child;
  len = NumberChildren(work);
  for(c=1;c<=len;c++){
    child = InstanceChild(work,c);
    if (child!=NULL)
      if ((InstanceKind(child)==ARRAY_INT_INST)||
          (InstanceKind(child)==ARRAY_ENUM_INST))
        if (!RectangleArrayExpanded(child)) return 0;
  }
  return 1;
}


/**
	Try to execute all the LINK statements in instance work.
	It assumes that work is the top of the pending instance list.
	Will skip all non-LINK statements.
*/
static
void Pass5ExecuteLinkStatements(struct BitList *blist,
                                struct Instance *work,
                                int *changed
){
  unsigned long c;
  struct TypeDescription *def;
  struct Statement *stat;
  def = InstanceTypeDesc(work);
  for(c=FirstNonZeroBit(blist);c<BLength(blist);c++){
    if (ReadBit(blist,c)){
      stat = GetExecutableStatement(def,c+1);
      if ( Pass5ExecuteStatement(work,stat) ) {
        ClearBit(blist,c);
        *changed = 1;
      }
    }
  }
}


/**
	Try to execute all the when statements in instance work.
	It assumes that work is the top of the pending instance list.
	Will skip all non-when statements.
*/
static
void Pass4ExecuteWhenStatements(struct BitList *blist,
                                struct Instance *work,
                                int *changed
){
  unsigned long c;
  struct TypeDescription *def;
  struct Statement *stat;
  def = InstanceTypeDesc(work);
  for(c=FirstNonZeroBit(blist);c<BLength(blist);c++){
    if (ReadBit(blist,c)){
      stat = GetExecutableStatement(def,c+1);
      if ( Pass4ExecuteStatement(work,stat) ) {
        ClearBit(blist,c);
        *changed = 1;
      }
    }
  }
}

/**
	Try to execute all the unexecuted logical relations in instance work.
	It assumes that work is the top of the pending instance list.
	Will skip all non-logical relations.
*/
static
void Pass3ExecuteLogRelStatements(struct BitList *blist,
                                     struct Instance *work,
                                     int *changed
){
  unsigned long c;
  struct TypeDescription *def;
  struct Statement *stat;
  def = InstanceTypeDesc(work);
  for(c=FirstNonZeroBit(blist);c<BLength(blist);c++){
    if (ReadBit(blist,c)){
      stat = GetExecutableStatement(def,c+1);
      if ( Pass3ExecuteStatement(work,stat) ) {
        ClearBit(blist,c);
        *changed = 1;
      }
    }
  }
}

/**
	Try to execute all the unexecuted relations in instance work.
	Does not assume that work is the top of the pending instance list.
	Will skip all non-relations in instance work.
*/
static
void Pass2ExecuteRelationStatements(struct BitList *blist,
                                    struct Instance *work,
                                    int *changed
){
  unsigned long c;
  struct TypeDescription *def;
  struct Statement *stat;
  def = InstanceTypeDesc(work);
  for(c=FirstNonZeroBit(blist);c<BLength(blist);c++){
    if (ReadBit(blist,c)){
      stat = GetExecutableStatement(def,c+1);
      if ( Pass2ExecuteStatement(work,stat) ) {
        //CONSOLE_DEBUG("Got error code here, clearing bit in blist, setting '*changed' to 1");
        ClearBit(blist,c);
        *changed = 1;
      }
    }
  }
}

/**
	Try to execute all the unexecuted statements in instance work.
	It assumes that work is the top of the pending instance list.
	Will skip relations in a new way. Relations instances and arrays of
	relations will be left as NULL instances (not merely hollow relations)
*/
static
void Pass1ExecuteInstanceStatements(struct BitList *blist,
                                    struct Instance *work,
                                    int *changed
){
  unsigned long c;
  struct TypeDescription *def;
  struct Statement *stat;

  def = InstanceTypeDesc(work);
  c=FirstNonZeroBit(blist);
  while(c<BLength(blist)) {
    if (ReadBit(blist,c)){
      stat = GetExecutableStatement(def,c+1);
      if ( Pass1ExecuteStatement(work,&c,stat) ) {
        if (StatementType(stat) != SELECT ) {
          ClearBit(blist,c);
        }
        *changed = 1;
      }
    }
    c++;
  }
}


static
void Pass5ProcessPendingInstances(void)
{
  struct pending_t *work;
  struct Instance *inst;
  struct BitList *blist;
  int changed = 0,count=0;
  unsigned long c;
  /*
   * pending will have at least one instance, or while will fail
   */
  while((count < PASS5MAXNUMBER) && NumberPending()>0){
    changed = 0;
    c = 0;
    while(c < NumberPending()){
      work = TopEntry();
      if (work!=NULL) {
        inst = PendingInstance(work);
        blist = InstanceBitList(inst);
      }else{
        blist = NULL;   /* this shouldn't be necessary, but is */
		inst = NULL;
      }
      if ((blist!=NULL)&&!BitListEmpty(blist)){
        /* only models get here */
        Pass5ExecuteLinkStatements(blist,inst,&changed);
        /* we do away with TryArrayExpansion because it doesn't do links either */
        if (BitListEmpty(blist)) {
          /*
		   * delete PENDING model.
		   */
          RemoveInstance(PendingInstance(work));
        }else{
		  /*
		   * bitlist is still unhappy, but there's nothing to do about it.
           * Move the instance to the bottom and increase the counter
		   * so that we do not visit it again.
		   */
          if (work == TopEntry()) {
            MoveToBottom(work);
          }
          c++;
        }
      }else{
        /* We do not attempt to expand non-link arrays in pass5. */
      }
    }
#if (PASS5MAXNUMBER > 1)
    if (!changed) {
#endif
      count++;
      g_iteration++;   /* The global iteration counter */
#if (PASS5MAXNUMBER > 1)
    }
#endif
  }
  /* done, or there were no pendings at all and while failed */
}

static
void Pass4ProcessPendingInstances(void)
{
  struct pending_t *work;
  struct Instance *inst;
  struct BitList *blist;
  int changed = 0,count=0;
  unsigned long c;
  /*
   * pending will have at least one instance, or while will fail
   */
  while((count < PASS4MAXNUMBER) && NumberPending()>0){
    changed = 0;
    c = 0;
    while(c < NumberPending()){
      work = TopEntry();
      if (work!=NULL) {
        inst = PendingInstance(work);
        blist = InstanceBitList(inst);
      }else{
        blist = NULL;   /* this shouldn't be necessary, but is */
		inst = NULL;
      }
      if ((blist!=NULL)&&!BitListEmpty(blist)){
        /* only models get here */
        Pass4ExecuteWhenStatements(blist,inst,&changed);
        /* we do away with TryArrayExpansion because it doesn't do whens */
        if (BitListEmpty(blist)) {
          /*
		   * delete PENDING model.
		   */
          RemoveInstance(PendingInstance(work));
        }else{
		  /*
		   * bitlist is still unhappy, but there's nothing to do about it.
           * Move the instance to the bottom and increase the counter
		   * so that we do not visit it again.
		   */
          if (work == TopEntry()) {
            MoveToBottom(work);
          }
          c++;
        }
      }else{
        /* We do not attempt to expand non-when arrays in pass4. */
      }
    }
#if (PASS4MAXNUMBER > 1)
    if (!changed) {
#endif
      count++;
      g_iteration++;   /* The global iteration counter */
#if (PASS4MAXNUMBER > 1)
    }
#endif
  }
  /* done, or there were no pendings at all and while failed */
}

static
void Pass3ProcessPendingInstances(void)
{
  struct pending_t *work;
  struct Instance *inst;
  struct BitList *blist;
  int changed = 0,count=0;
  unsigned long c;
  /* Reinitialize the number of iterations */
  ClearIteration();
  g_iteration++;

  /* pending will have at least one instance, or while will fail */
  while((count < PASS3MAXNUMBER) && NumberPending()>0){
    changed = 0;
    c = 0;
    while(c < NumberPending()){
      work = TopEntry();
      if (work!=NULL) {
        inst = PendingInstance(work);
        /* WriteInstanceName(stderr,inst,NULL); FPRINTF(stderr,"\n"); */
        blist = InstanceBitList(inst);
      }else{
        blist = NULL; /* this shouldn't be necessary, but is */
        inst = NULL;
      }
      if ((blist!=NULL)&&!BitListEmpty(blist)){
        /* only models get here */
        Pass3ExecuteLogRelStatements(blist,inst,&changed);
        /* we do away with TryArrayExpansion because it doesn't do rels */

#if (PASS3MAXNUMBER > 1)
        if (BitListEmpty(blist) && ArraysExpanded(inst)) {
        /* removal is now unconditional because even if there are
                pendings, theres nothing we can do. If we
                go back to some uglier scheme, we would still need to test,
                but only against bitlist, not ArraysExpanded. */
#endif
          RemoveInstance(PendingInstance(work));
                /* delete PENDING model. bitlist could still be unhappy,
                        but there's nothing to do about it. */
          /* instance could move while being worked. reget the pointer.
             work itself cannot move, in memory that is. its list position
             can change. Actually in relation phase, this may not be
                true. */
#if (PASS3MAXNUMBER > 1)
        /* we aren't touching any model twice, so this isn't needed
                unless back to uglier scheme */
        }else{
          if (work == TopEntry())
            MoveToBottom(work);
          c++;
        }
#endif
      }else{
        /* We do not attempt to expand non-logical relation arrays in pass3.*/
      }
    }
    if (!changed) {
      count++;
      g_iteration++;		/* The global iteration counter */
    }
  }
  /* done, or there were no pendings at all and while failed */
}

/**
	This is the singlepass phase2 with anontype sharing of
	relations implemented. If relations can depend on other
	relations (as in future differential work) then this function
	needs to be slightly more sophisticated.
*/
static
void Pass2ProcessPendingInstancesAnon(struct Instance *result)
{
  struct BitList *blist;
  struct Instance *proto;	/* first of an anon clique */
  struct gl_list_t *atl;	/* anonymous types in result */
  struct gl_list_t *protovarindices; /* all vars in all rels in local MODEL */
  struct AnonType *at;
  int changed = 0;		/* will become 1 if any local relation made */
  int anychange = 0;		/* will become 1 if any change anywhere */
  unsigned long c,n,alen,clen;
#if TIMECOMPILER
  double start,classt;
#endif
  /*CONSOLE_DEBUG("...");*/

  /* pending will have at least one instance, or quick return. */
  asc_assert(PASS2MAXNUMBER==1);

  if (NumberPending() > 0) {
#if TIMECOMPILER
    start = tm_cpu_time();
#endif
    atl = Asc_DeriveAnonList(result);
#if TIMECOMPILER
    classt = tm_cpu_time();
    CONSOLE_DEBUG("Classification: %0.6f s (for relation sharing)",(classt-start));
    start = tm_cpu_time();
#endif
    alen = gl_length(atl);
    /* iterate over all anontypes, working on only models. */
    for (n=1; n <= alen; n++) {
      changed = 0;
      at = Asc_GetAnonType(atl,n);
      proto = Asc_GetAnonPrototype(at);
      if (InstanceKind(proto) == MODEL_INST && InstanceInList(proto)) {
#ifdef DEBUG_RELS
        ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
        FPRINTF(stderr,"Rels in model: ");
        WriteInstanceName(stderr,proto,NULL); FPRINTF(stderr,"\n");
        error_reporter_end_flush();
#endif
        blist = InstanceBitList(proto);
        if ((blist!=NULL) && !BitListEmpty(blist)) {
          Pass2ExecuteRelationStatements(blist,proto,&changed);
          RemoveInstance(proto);
          anychange += changed;
        }
        /* finish rest of AT clique, if there are any, if we made something */
        clen = Asc_GetAnonCount(atl,n);
        if (clen==1 || changed == 0) {
          continue;
        }
        protovarindices = Pass2CollectAnonProtoVars(proto);
        for (c=2; c <= clen; c++) {
          Pass2CopyAnonProto(proto,blist,protovarindices,
                             Asc_GetAnonTypeInstance(at,c));
        }
        Pass2DestroyAnonProtoVars(protovarindices);
      }
    }
    Asc_DestroyAnonList(atl);
    if (!anychange) {
      g_iteration++;		/* The global iteration counter */
    }else{
      /* we did something, so try the binary compile */
#if TIMECOMPILER
      classt = tm_cpu_time();
      CONSOLE_DEBUG("Making tokens: %0.6f s (for relations)",(classt-start));
      start = tm_cpu_time();
#endif
      BinTokensCreate(result,BT_C);
#if TIMECOMPILER
      classt = tm_cpu_time();
      CONSOLE_DEBUG("build/link: %0.6f s (for bintokens)",(classt-start));
#endif
    }
  }
  /* done, or there were no pendings at all and while failed */
}

/**
	This is the old pass1-like flavor of pass2process.
	Do not delete it yet, as it is the way we'll have to
	start thinking if relations reference relations, i.e.
	in the use of derivatives in the ASCEND language.
*/
static
void Pass2ProcessPendingInstances(void)
{
  struct pending_t *work;
  struct Instance *inst;
  struct BitList *blist;
  int changed = 0,count=0;
  unsigned long c;
  /* pending will have at least one instance, or while will fail */
  while((count < PASS2MAXNUMBER) && NumberPending()>0){
    changed = 0;
    c = 0;
    while(c < NumberPending()){
      work = TopEntry();
      if (work!=NULL) {
        inst = PendingInstance(work);
        /* WriteInstanceName(stderr,inst,NULL); FPRINTF(stderr,"\n"); */
        blist = InstanceBitList(inst);
      }else{
        blist = NULL; /* this shouldn't be necessary, but is */
        inst = NULL;
      }
      if ((blist!=NULL)&&!BitListEmpty(blist)){
        /* only models get here */
        Pass2ExecuteRelationStatements(blist,inst,&changed);
        /* we do away with TryArrayExpansion because it doesn't do rels */

#if (PASS2MAXNUMBER > 1)
        if (BitListEmpty(blist) && ArraysExpanded(inst)) {
        /* removal is now unconditional because even if there are
                pendings, theres nothing we can do. If we
                go back to some uglier scheme, we would still need to test,
                but only against bitlist, not ArraysExpanded. */
#endif
          RemoveInstance(PendingInstance(work));
                /* delete PENDING model. bitlist could still be unhappy,
                        but there's nothing to do about it. */
          /* instance could move while being worked. reget the pointer.
             work itself cannot move, in memory that is. its list position
             can change. Actually in relation phase, this may not be
                true. */
#if (PASS2MAXNUMBER > 1)
        /* we aren't touching any model twice, so this isn't needed
                unless back to uglier scheme */
        }else{
          if (work == TopEntry())
            MoveToBottom(work);
          c++;
        }
#endif
      }else{
        /* We do not attempt to expand non-relation arrays in pass2. */
      }
    }
    if (!changed) {
      count++;
      g_iteration++;		/* The global iteration counter */
    }
  }
  /* done, or there were no pendings at all and while failed */
}


/*
 * in a bizarre way, this will generally lead to a bottom up
 * instantiation finishing process, though it is started in a
 * top down fashion. While not quite a recursion, work proceeds
 * in a nested fashion and so the FOR tables must be pushed
 * and popped to get the right interpretation.
 */
static
void Pass1ProcessPendingInstances(void)
{
  struct pending_t *work;
  struct Instance *inst;
  struct BitList *blist;
  int changed = 0,count=0;
  unsigned long c;
  while((count <= MAXNUMBER)&&NumberPending()>0){
    changed = 0;
    c = 0;
    while(c < NumberPending()){
      work = TopEntry();
      inst = PendingInstance(work);
      blist = InstanceBitList(inst);
      if ((blist!=NULL)&&!BitListEmpty(blist)){
        /* only models get here */
        Pass1ExecuteInstanceStatements(blist,inst,&changed);
        TryArrayExpansion(inst,&changed);
        /* try to expand any nonalias,nonparameterized arrays */
        if (BitListEmpty(blist)&&ArraysExpanded(inst)) {
          RemoveInstance(PendingInstance(work));
                /* delete PENDING model */
          /* instance could move while being worked. reget the pointer.
             work itself cannot move, in memory that is. its list position
             can change */
        }else{
          if (work == TopEntry()) {
            MoveToBottom(work);
          }
          c++;
        }
      }else{
        TryArrayExpansion(inst,&changed);
        /* try to expand any nonalias,nonparameterized arrays */
        if (ArraysExpanded(inst)) {
          RemoveInstance(PendingInstance(work));
                /* delete PENDING array */
          /* instance could move while being worked. reget the pointer.
             work itself cannot move, in memory that is. its list position
             can change */
        }else{
          if (work == TopEntry())
            MoveToBottom(work);
          c++;
        }
      }
    }
    if (!changed) {
      count++;
      g_iteration++;		/* The global iteration counter */
    }
  }
}


static struct gl_list_t *GetInstanceStatementList(struct Instance *i){
  struct TypeDescription *def;
  CONST struct StatementList *slist;
  def = InstanceTypeDesc(i);
  if (def==NULL) return NULL;
  slist = GetStatementList(def);
  if (slist==NULL) return NULL;
  return GetList(slist);
}


/* run the given default statements of i */
static void ExecuteDefault(struct Instance *i
	, struct Statement *stat,unsigned long int *depth
){
  struct gl_list_t *lvals;
  unsigned long c,length;
  struct Instance *ptr;
  struct value_t value;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  if(NULL != (lvals = FindInstances(i,DefaultStatVar(stat),&err))){
    for(c=1,length=gl_length(lvals);c<=length;c++){
      ptr = (struct Instance *)gl_fetch(lvals,c);
      switch(InstanceKind(ptr)){
      case REAL_ATOM_INST:
      case REAL_INST:
        if (*depth == 0) *depth = InstanceDepth(i);
        if (DepthAssigned(ptr) >= *depth){
          asc_assert(GetEvaluationContext()==NULL);
          SetEvaluationContext(i);
          value = EvaluateExpr(DefaultStatRHS(stat),NULL,
                               InstanceEvaluateName);
          SetEvaluationContext(NULL);
          if ( IsWild(RealAtomDims(ptr)) ) {
            switch(ValueKind(value)) {
            case real_value:
              SetRealAtomValue(ptr,RealValue(value),*depth);
              if ( !IsWild(RealValueDimensions(value)) ) {
                SetRealAtomDims(ptr,RealValueDimensions(value));
              }
              break;
            case integer_value:
              SetRealAtomValue(ptr,(double)IntegerValue(value),*depth);
              SetRealAtomDims(ptr,Dimensionless());
              break;
            default:
              STATEMENT_ERROR(stat,"Bad real default value");
              break;
            }
          }else{
            switch(ValueKind(value)) {
            case real_value:
              if ( !SameDimen(RealValueDimensions(value),RealAtomDims(ptr)) ){
                STATEMENT_ERROR(stat,
                "Default right hand side is dimensionally inconsistent");
              }else{
                SetRealAtomValue(ptr,RealValue(value),*depth);
              }
              break;
            case integer_value:
              if ( !SameDimen(Dimensionless(),RealAtomDims(ptr)) ){
                STATEMENT_ERROR(stat,
                "Default right hand side is dimensionally inconsistent");
              }else{
                SetRealAtomValue(ptr,(double)IntegerValue(value),*depth);
              }
              break;
            default:
              STATEMENT_ERROR(stat,"Bad real default value");
              break;
            }
          }
          DestroyValue(&value);
        }
        break;
      case BOOLEAN_ATOM_INST:
      case BOOLEAN_INST:
        if (*depth == 0) *depth = InstanceDepth(i);
        if (DepthAssigned(ptr) > *depth){
          asc_assert(GetEvaluationContext()==NULL);
          SetEvaluationContext(i);
          value = EvaluateExpr(DefaultStatRHS(stat),NULL,
                               InstanceEvaluateName);
          SetEvaluationContext(NULL);
          if (ValueKind(value) == boolean_value){
            SetBooleanAtomValue(ptr,BooleanValue(value),*depth);
          }else{
            STATEMENT_ERROR(stat, "Bad boolean default value");
          }
          DestroyValue(&value);
        }
        break;
      case INTEGER_ATOM_INST:
      case INTEGER_INST:
        asc_assert(GetEvaluationContext()==NULL);
        SetEvaluationContext(i);

        value = EvaluateExpr(DefaultStatRHS(stat),NULL,
                               InstanceEvaluateName);
        SetEvaluationContext(NULL);
        if (ValueKind(value) == integer_value){
          SetIntegerAtomValue(ptr,IntegerValue(value),0);
        }else{
          STATEMENT_ERROR(stat, "Bad integer default value");
        }
        DestroyValue(&value);
        break;
      case SYMBOL_ATOM_INST:
      case SYMBOL_INST:
        asc_assert(GetEvaluationContext()==NULL);
        SetEvaluationContext(i);
        value = EvaluateExpr(DefaultStatRHS(stat),NULL,
                               InstanceEvaluateName);
        SetEvaluationContext(NULL);
        if (ValueKind(value) == symbol_value){
          SetSymbolAtomValue(ptr,SymbolValue(value));
        }else{
          STATEMENT_ERROR(stat, "Bad symbol default value");
        }
        DestroyValue(&value);
        break;
      default: /* NEED stuff here */
 	break;
      }
    }
    gl_destroy(lvals);
  }else{
    STATEMENT_ERROR(stat, "Nonexistent LHS variable in default statement.");
  }
}

/**
	run the default statements of i, including nested fors, but
	not recursive to i children.
*/
static
void ExecuteDefaultStatements(struct Instance *i,
                              struct gl_list_t *slist,
                              unsigned long int *depth)
{
  unsigned long c,length;
  struct Statement *stat;

  if (slist){
    length = gl_length(slist);
    for(c=1;c<=length;c++){
      stat = (struct Statement *)gl_fetch(slist,c);
      switch(StatementType(stat)){
      case ASGN:
        ExecuteDefault(i,stat,depth);
        break;
      case FOR:
        if ( ForContainsDefaults(stat) ){
          RealDefaultFor(i,stat,depth);
        }
        break;
      default: /* nobody else is a default */
        break;
      }
    }
  }
}

static
void RealDefaultFor(struct Instance *i,
                    struct Statement *stat,
                    unsigned long int *depth)
{
  symchar *name;
  struct Expr *ex;
  struct StatementList *sl;
  unsigned long c,len;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;
  sl = ForStatStmts(stat);
  name = ForStatIndex(stat);
  ex = ForStatExpr(stat);
  if (FindForVar(GetEvaluationForTable(),name)){ /* duplicated for variable*/
    FPRINTF(ASCERR,"Error during default stage.\n");
    STATEMENT_ERROR(stat, "FOR construct uses duplicate index variable");
    return;
  }
  asc_assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(i);
  value = EvaluateExpr(ex,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  switch(ValueKind(value)){
  case error_value:
    switch(ErrorValue(value)){
    case name_unfound:
    case undefined_value:
      DestroyValue(&value);
      FPRINTF(ASCERR,"Error in default stage.\n");
      STATEMENT_ERROR(stat, "FOR has undefined values");
      return;
    default:
      WriteForValueError(stat,value);
      DestroyValue(&value);
      return;
    }
  case real_value:
  case integer_value:
  case symbol_value:
  case boolean_value:
  case list_value:
    FPRINTF(ASCERR,"Error during default stage.\n");
    STATEMENT_ERROR(stat, "FOR expression returns the wrong type");
    DestroyValue(&value);
    return;
  case set_value:
    sptr = SetValue(value);
    switch(SetKind(sptr)){
    case empty_set: break;
    case integer_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_integer);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForInteger(fv,FetchIntMember(sptr,c));
        ExecuteDefaultStatements(i,GetList(sl),depth);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    case string_set:
      fv = CreateForVar(name);
      SetForVarType(fv,f_symbol);
      AddLoopVariable(GetEvaluationForTable(),fv);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++){
        SetForSymbol(fv,FetchStrMember(sptr,c));
        ExecuteDefaultStatements(i,GetList(sl),depth);
      }
      RemoveForVariable(GetEvaluationForTable());
      break;
    }
    DestroyValue(&value);
  }
}

static
void DefaultStatementList(struct Instance *i,
                          struct gl_list_t *slist,
                          unsigned long int *depth)
{
  unsigned long c,length;
  struct Statement *stat;
  struct for_table_t *SavedForTable;
  if (slist){
    length = gl_length(slist);
    for(c=1;c<=length;c++){
      stat = (struct Statement *)gl_fetch(slist,c);
      switch(StatementType(stat)){
      case ASGN:
        ExecuteDefault(i,stat,depth);
        break;
      case FOR:
        if ( ForContainsDefaults(stat) ){
          SavedForTable = GetEvaluationForTable();
          SetEvaluationForTable(CreateForTable());
          RealDefaultFor(i,stat,depth);
          DestroyForTable(GetEvaluationForTable());
          SetEvaluationForTable(SavedForTable);
        }
        break;
      case SELECT:
        if (SelectContainsDefaults(stat)) {
          ExecuteDefaultsInSELECT(i,&c,stat,depth);
        }else{
          c = c + SelectStatNumberStats(stat) ;
        }
        break;
      default:
        break;
      }
    }
  }
}

static
void DefaultInstance(struct Instance *i)
{
  if (i && (InstanceKind(i) == MODEL_INST)){
    unsigned long depth=0;
    if (TypeHasDefaultStatements(InstanceTypeDesc(i)))
      DefaultStatementList(i,GetInstanceStatementList(i),&depth);
  }
}

static
void DefaultInstanceTree(struct Instance *i)
{
  VisitInstanceTree(i,DefaultInstance,0,0);
}



/**
	This just handles instantiating LINKs,
	ignoring anything else.
	This works with Pass5ProcessPendingInstances.
*/
static
struct Instance *Pass5InstantiateModel(struct Instance *result,
                                       unsigned long *pcount)
{
  /* do we need a ForTable on the stack here? don't think so. np4ppi does it */
  if (result!=NULL) {
    /* pass5 pendings already set by visit */
    Pass5ProcessPendingInstances();
    if (NumberPending()!=0) {
      FPRINTF(ASCERR,
        "There are unexecuted Phase 5 (LINKs) in the instance.\n");
      *pcount = NumberPending();
    }
    ClearList();
  }
  return result;
}

static
void Pass5SetLinkBits(struct Instance *inst)
{
  struct Statement *stat;

  if (inst != NULL && InstanceKind(inst)==MODEL_INST) {
    struct BitList *blist;

    blist = InstanceBitList(inst);
    if (blist!=NULL){
      unsigned long c;
      enum stat_t st;
      int changed;

      changed=0;
      for(c=0;c<BLength(blist);c++){
        stat = GetExecutableStatement(InstanceTypeDesc(inst),c+1);
        st= StatementType(stat);
        if (st == SELECT) {
          if (SelectContainsLink(stat)) {
            ReEvaluateSELECT(inst,&c,stat,5,&changed);
          }else{
            c = c + SelectStatNumberStats(stat);
          }
        }else{
          if ( st == LNK || (st == FOR && ForContainsLink(stat)) ) {
            SetBit(blist,c);
            changed++;
          }
        }
      }
      /* if changed = 0 but bitlist not empty, we don't want to retry
        thoroughly done insts. if whens, then we can't avoid.
        if we did add any bits, then changed!= 0 is sufficient test. */
      if ( changed ) {
        AddBelow(NULL,inst);
        /* add PENDING model */
      }
    }
  }
}


/**
	This just handles instantiating whens,
	ignoring anything else.
	This works with Pass4ProcessPendingInstances.
*/
static
struct Instance *Pass4InstantiateModel(struct Instance *result,
                                       unsigned long *pcount)
{
  /* do we need a ForTable on the stack here? don't think so. np4ppi does it */
  if (result!=NULL) {
    /* pass4 pendings already set by visit */
    Pass4ProcessPendingInstances();
    if (NumberPending()!=0) {
      FPRINTF(ASCERR,
        "There are unexecuted Phase 4 (whens) in the instance.\n");
      *pcount = NumberPending();
    }
    ClearList();
  }
  return result;
}

static
void Pass4SetWhenBits(struct Instance *inst)
{
  struct Statement *stat;

  if (inst != NULL && InstanceKind(inst)==MODEL_INST) {
    struct BitList *blist;

    blist = InstanceBitList(inst);
    if (blist!=NULL){
      unsigned long c;
      enum stat_t st;
      int changed;

      changed=0;
      for(c=0;c<BLength(blist);c++){
        stat = GetExecutableStatement(InstanceTypeDesc(inst),c+1);
        st= StatementType(stat);
        if (st == SELECT) {
          if (SelectContainsWhen(stat)) {
            ReEvaluateSELECT(inst,&c,stat,4,&changed);
          }else{
            c = c + SelectStatNumberStats(stat);
          }
        }else{
          if ( st == WHEN || (st == FOR && ForContainsWhen(stat)) ) {
            SetBit(blist,c);
            changed++;
          }
        }
      }
      /* if changed = 0 but bitlist not empty, we don't want to retry
        thoroughly done insts. if whens, then we can't avoid.
        if we did add any bits, then changed!= 0 is sufficient test. */
      if ( changed ) {
        AddBelow(NULL,inst);
        /* add PENDING model */
      }
    }
  }
}



/**
	This just handles instantiating logical relations,
	ignoring anything else.
	This works with Pass3ProcessPendingInstances.
	No recursion. No reallocation of result.
*/
static
struct Instance *Pass3InstantiateModel(struct Instance *result,
                                       unsigned long *pcount)
{
  if (result!=NULL) {
    /* pass3 pendings already set by visit */
    Pass3ProcessPendingInstances();
    if (NumberPending()!=0) {
      FPRINTF(ASCERR,
      "There are unexecuted Phase 3 (logical relations) in the instance.\n");
      *pcount = NumberPending();
    }
    ClearList();
  }
  return result;
}

static
void Pass3SetLogRelBits(struct Instance *inst)
{
  struct Statement *stat;
  if (inst != NULL && InstanceKind(inst)==MODEL_INST) {
    struct BitList *blist;

    blist = InstanceBitList(inst);
    if (blist!=NULL){
      unsigned long c;
      enum stat_t st;
      int changed;

      changed=0;
      for(c=0;c<BLength(blist);c++){
        stat = GetExecutableStatement(InstanceTypeDesc(inst),c+1);
        st= StatementType(stat);
        if (st == SELECT) {
          if (SelectContainsLogRelations(stat)) {
            ReEvaluateSELECT(inst,&c,stat,3,&changed);
          }else{
            c = c + SelectStatNumberStats(stat);
          }
        }else{
          if ((st == LOGREL)
              || (st == COND && CondContainsLogRelations(stat))
              || (st == FOR && ForContainsLogRelations(stat)) ) {
            SetBit(blist,c);
            changed++;
          }
        }
      }
      /* if changed = 0 but bitlist not empty, we don't want to retry
        thoroughly done insts. if relations, then we can't avoid.
        if we did add any bits, then changed!= 0 is sufficient test. */
      if ( changed ) {
        AddBelow(NULL,inst);
        /* add PENDING model */
      }
    }
  }
}

/**
	This just handles instantiating relations, ignoring anything else.
	This works with Pass2ProcessPendingInstances AND
	Pass2ProcessPendingInstancesAnon, both of which are required to
	maintain a correct compilation.
	No recursion. No reallocation of result.
*/
#define ANONFORCE 0 /* require anonymous type use, even if whining OTHERWISE */
static struct Instance *Pass2InstantiateModel(struct Instance *result,
		unsigned long *pcount
){
#ifdef INST_DEBUG
  CONSOLE_DEBUG("starting...");
#endif

  /* do we need a ForTable on the stack here? don't think so. np2ppi does it */
  if (result!=NULL) {
    /* CONSOLE_DEBUG("result!=NULL..."); */
    /* pass2 pendings already set by visit */
    if (ANONFORCE || (g_use_copyanon != 0 && !g_pass1_saw_dataset)) {
#if TIMECOMPILER
      g_ExecuteREL_CreateTokenRelation_calls = 0;
      g_CopyAnonRelation = 0;
#endif
      Pass2ProcessPendingInstancesAnon(result);
#if TIMECOMPILER
      CONSOLE_DEBUG("Relations in the instance U %d + C %d = T %d." ,
         g_ExecuteREL_CreateTokenRelation_calls,g_CopyAnonRelation,
         g_CopyAnonRelation+g_ExecuteREL_CreateTokenRelation_calls);
#endif
    }else{
      Pass2ProcessPendingInstances();
    }
    if (NumberPending()!=0) {
      FPRINTF(ASCERR,
        "There are unexecuted Phase 2 (relations) in the instance.\n");
        /* dump them here, nitwit. BAA. */
      *pcount = NumberPending();
    }
    ClearList();
  }
#ifdef INST_DEBUG
  CONSOLE_DEBUG("...done");
#endif
  return result;
}

static
void Pass2SetRelationBits(struct Instance *inst)
{
  struct Statement *stat;
  if (inst != NULL && InstanceKind(inst)==MODEL_INST) {
    struct BitList *blist;
#ifdef DEBUG_RELS
    ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
    FPRINTF(ASCERR,"P2SRB: ");
    WriteInstanceName(ASCERR,inst,debug_rels_work);
    error_reporter_end_flush();
#endif

    blist = InstanceBitList(inst);
    if (blist!=NULL){
      unsigned long c;
      enum stat_t st;
      int changed;

      changed=0;
      for(c=0;c<BLength(blist);c++){
        stat = GetExecutableStatement(InstanceTypeDesc(inst),c+1);
        st= StatementType(stat);
        if (st == SELECT) {
          if (SelectContainsRelations(stat) ||
              SelectContainsExternal(stat))
          {
            ReEvaluateSELECT(inst,&c,stat,2,&changed);
          }else{
            c = c + SelectStatNumberStats(stat);
          }
        }else{
          if ( st == REL ||
               st == EXT ||
	       (st == COND &&
                 (CondContainsRelations(stat) ||
                  CondContainsExternal(stat) )
               ) ||
               (st == FOR &&
                 (ForContainsRelations(stat) ||
                  ForContainsExternal(stat) )
               )
             ){
            SetBit(blist,c);
            changed++;
          }
        }
      }
      /* if changed = 0 but bitlist not empty, we don't want to retry
        thoroughly done insts. if relations, then we can't avoid.
        if we did add any bits, then changed!= 0 is sufficient test. */
      if ( changed ) {
        AddBelow(NULL,inst);
        /* add PENDING model */
#ifdef DEBUG_RELS
        ERROR_REPORTER_START_NOLINE(ASC_PROG_NOTE);
        FPRINTF(stderr,"Changed: ");
        WriteInstanceName(ASCERR,inst,debug_rels_work);
        error_reporter_end_flush();
#endif
      }
    }
  }
}


/**
	This just handles instantiating models and reinstantiating models/arrays,
	ignoring defaults and relations.
	This works with Pass1ProcessPendingInstances.
	This is not a recursive function.
	Either def should be null or oldresult should null.
	If def is null, it is a reinstantiation, else result will be created.
*/
static
struct Instance *Pass1InstantiateModel(struct TypeDescription *def,
                                       unsigned long *pcount,
                                       struct Instance *oldresult)
{
  struct Instance *result;
  struct Instance *arginst = NULL;
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());
  g_pass1_saw_dataset = 0;

  if (def != NULL && oldresult != NULL) {
    Asc_Panic(2, "Pass1InstantiateModel",
              "Pass1InstantiateModel called with both type and instance.");
  }
  if (def!=NULL) { /* usual case */
    result = ShortCutMakeUniversalInstance(def);
    if (result==NULL) {
      result = CreateModelInstance(def);
      if (result != NULL
          && GetModelParameterCount(def) == 0
          && StatementListLength(GetModelAbsorbedParameters(def)) != 0L) {
        if (BuildInstanceFromAbsorbedParameters(def,&arginst) != MPIOK) {
          DestroyParameterInst(result);
          result = NULL;
        } else {
          ConfigureInstFromArgs(result,arginst);
        }
      }
      /* at present, creating parameterized sims illegal */
    }
  }else{
    result = oldresult;
  }
  if (result!=NULL) {
    ClearList();
    if (oldresult !=NULL) {
      SilentVisitInstanceTree(result,AddIncompleteInst,1,0);
    }else{
      AddBelow(NULL,result);
    }

    /* add PENDING model */
    Pass1ProcessPendingInstances();
    if(NumberPending()!=0) {
      *pcount = NumberPending();
      FPRINTF(ASCERR,
          "There are %lu unexecuted Phase 1 statements in the instance.\n"
          ,*pcount
      );
        if(g_compiler_warnings < 2 && *pcount >10L) {
          FPRINTF(ASCWAR,"More than 10 pending statements and warning %s"
              ,"level too low to allow printing.\n"
          );
        }else{
          FPRINTF(ASCWAR,"---- Pass 1 pending: -------------\n");
          if (g_compiler_warnings > 1
              && (!g_pass1_saw_dataset || g_compiler_warnings > 2)) {
            CheckInstanceLevel(ASCWAR,result,1);
          }else if (g_compiler_warnings > 1 && g_pass1_saw_dataset) {
            FPRINTF(ASCWAR,
              "Skipping full pass-1 structural check because DATASET statements were executed.\n"
              "Use compiler verbosity > 2 to force full diagnostics for DATASET-backed models.\n");
          }else{
            FPRINTF(ASCWAR,"(Total object check suppressed: g_compiler_warnings = %d)\n",g_compiler_warnings);
          }
          FPRINTF(ASCWAR,"---- End pass 1 pending-----------\n");
        }
        /* could instead start an error pool data structure with
        a review protocol in place post instantiation. */
    }
    ClearList();
  }
  if (arginst != NULL) {
    DestroyParameterInst(arginst);
  }
  DatasetCacheClear();
  DestroyForTable(GetEvaluationForTable());
  SetEvaluationForTable(SavedForTable);
  return result;
}

/**
	we have to introduce a new head to instantiatemodel to manage
	the phases.
	6 phases: model creation, relation creation,
	logical relation creation, when creation, LINK creation
	defaulting.
	BAA
	each pass is responsible for clearing the pending list it leaves.
*/
static
struct Instance *NewInstantiateModel(struct TypeDescription *def)
{
  struct Instance *result;
  unsigned long pass1pendings,pass2pendings,pass3pendings,pass4pendings,pass5pendings;
  int inst_profile = 0;
  double inst_mark = 0.0;
  double inst_p1 = 0.0;
  double inst_p2 = 0.0;
  double inst_p3 = 0.0;
  double inst_p4 = 0.0;
  double inst_p5 = 0.0;
  double inst_p6 = 0.0;
  double inst_total = 0.0;
  const char *inst_profile_env = getenv("ASC_INSTANTIATE_PROFILE");
#if TIMECOMPILER
  double start, phase1t,phase2t,phase3t,phase4t,phase5t,phase6t;
#endif

#ifdef INST_DEBUG
  CONSOLE_DEBUG("starting...");
#endif

  pass1pendings = 0L;
  pass2pendings = 0L;
  pass3pendings = 0L;
  pass4pendings = 0L;
	pass5pendings = 0L;
  if (inst_profile_env != NULL && inst_profile_env[0] != '\0' && inst_profile_env[0] != '0') {
    inst_profile = 1;
    inst_mark = DatasetNowSeconds();
  }
#if TIMECOMPILER
  start = tm_cpu_time();
#endif
  result = Pass1InstantiateModel(def,&pass1pendings,NULL);
  if (inst_profile) {
    double now = DatasetNowSeconds();
    inst_p1 = now - inst_mark;
    inst_mark = now;
  }

#if TIMECOMPILER
  phase1t = tm_cpu_time();
  CONSOLE_DEBUG("Phase 1 models = %0.6f s",phase1t-start);
#endif

  /* At this point, there may be unexecuted non-relation
   * statements, but they can never be executed. The
   * pending list is therefore empty. We know how many.
   * The bitlists know which ones.
   */
  if (result!=NULL) {
#ifdef DEBUG_RELS
    debug_rels_work = result;
#endif
    /* now set the bits for relation statements and add pending models */
    SilentVisitInstanceTree(result,Pass2SetRelationBits,0,0);
    /* note, the order of the visit might be better 1 than 0. don't know
     * at present order 0, so we do lower models before those near root
     */
    if (g_use_copyanon) {
    }
    result = Pass2InstantiateModel(result,&pass2pendings);
    if (inst_profile) {
      double now = DatasetNowSeconds();
      inst_p2 = now - inst_mark;
      inst_mark = now;
    }
    /* result will not move as currently implemented */
#ifdef DEBUG_RELS
    debug_rels_work = NULL;
#endif
  }else{
    return result;
  }

#if TIMECOMPILER
  phase2t = tm_cpu_time();
  CONSOLE_DEBUG("Phase 2 relations = %0.6f s",(phase2t-phase1t));
#endif

  /* CONSOLE_DEBUG("Starting phase 3..."); */
  /* at this point, there may be unexecuted non-logical relation
   * statements, but they can never be executed. The
   * pending list is therefore empty. We know how many.
   * The bitlists know which ones.
   */
  if (result!=NULL) {
    /* now set the bits for relation statements and add pending models */
    SilentVisitInstanceTree(result,Pass3SetLogRelBits,0,0);
    /* note, the order of the visit might be better 1 than 0. don't know
     * at present order 0, so we do lower models before those near root
     */
    result = Pass3InstantiateModel(result,&pass3pendings);
    if (inst_profile) {
      double now = DatasetNowSeconds();
      inst_p3 = now - inst_mark;
      inst_mark = now;
    }
   /* result will not move as currently implemented */
  }else{
    return result;
  }

#if TIMECOMPILER
  phase3t = tm_cpu_time();
  CONSOLE_DEBUG("Phase 3 logicals = %0.6f s",(phase3t-phase2t));
#endif

  if (result!=NULL) {
    /* now set the bits for when statements and add pending models */
    SilentVisitInstanceTree(result,Pass4SetWhenBits,0,0);
    /* note, the order of the visit might be better 1 than 0. don't know */
    /* at present order 0, so we do lower models before those near root */
    result = Pass4InstantiateModel(result,&pass4pendings);
    if (inst_profile) {
      double now = DatasetNowSeconds();
      inst_p4 = now - inst_mark;
      inst_mark = now;
    }
   /* result will not move as currently implemented */
  }else{
    return result;
  }

#if TIMECOMPILER
  phase4t = tm_cpu_time();
  CONSOLE_DEBUG("Phase 4 when-case = %0.6f s",(phase4t-phase3t));
#endif

  if (result!=NULL) {
		/* now set the bits for LINK statements and add pending models */
	  SilentVisitInstanceTree(result,Pass5SetLinkBits,0,0);
	  /* note, the order of the visit might be better 1 than 0. don't know */
	  /* at present order 0, so we do lower models before those near root */
		result = Pass5InstantiateModel(result,&pass5pendings);
    if (inst_profile) {
      double now = DatasetNowSeconds();
      inst_p5 = now - inst_mark;
      inst_mark = now;
    }
		/* result will not move as currently implemented <-DS: what do you really mean by this? */
  }else{
    return result;
  }
#if TIMECOMPILER
  phase5t = tm_cpu_time();
  CONSOLE_DEBUG("Phase 5 LINK-case = %0.6f s",(phase5t-phase4t));
#endif
  if (result!=NULL) {
    if (!pass1pendings && !pass2pendings && !pass3pendings && !pass4pendings && !pass5pendings){
      DefaultInstanceTree(result);
      if (inst_profile) {
        double now = DatasetNowSeconds();
        inst_p6 = now - inst_mark;
        inst_mark = now;
      }
    }else{
      ERROR_REPORTER_NOLINE(ASC_USER_WARNING,"There are unexecuted statements "
		"in the instance.\nDefault assignments not executed.");
    }
  }

#if TIMECOMPILER
  phase6t = tm_cpu_time();
  CONSOLE_DEBUG("Phase 6 defaults = %0.6f s",(phase6t-phase5t));
  if(pass1pendings || pass2pendings || pass3pendings || pass4pendings || pass5pendings) {
    CONSOLE_DEBUG("Phase 1 models \t\t%0.6f s\n", (phase1t-start));
    CONSOLE_DEBUG("Phase 2 relations \t\t%0.6f s\n", (phase2t-phase1t));
    CONSOLE_DEBUG("Phase 3 logical \t\t%0.6f s\n", (phase3t-phase2t));
    CONSOLE_DEBUG("Phase 4 when-case \t\t%0.6f s\n", (phase4t-phase3t));
    CONSOLE_DEBUG("Phase 5 LINK-case \t\t%0.6f s\n", (phase5t-phase4t));
    CONSOLE_DEBUG("Phase 6 defaults\t\t%0.6f s\n", (phase6t-phase5t));
  }
  CONSOLE_DEBUG("Total = %0.6f s",(phase6t-start));
# if 0 /* deep performance tuning */
  gl_reportrecycler(ASCERR);
# endif
#endif
  if (inst_profile) {
    inst_total = inst_p1 + inst_p2 + inst_p3 + inst_p4 + inst_p5 + inst_p6;
    FPRINTF(ASCERR,
      "INSTANTIATE_PROFILE p1_models=%.6fs p2_rels=%.6fs p3_logrels=%.6fs p4_whens=%.6fs p5_links=%.6fs p6_defaults=%.6fs total=%.6fs pendings=[%lu,%lu,%lu,%lu,%lu]\n",
      inst_p1,
      inst_p2,
      inst_p3,
      inst_p4,
      inst_p5,
      inst_p6,
      inst_total,
      pass1pendings,
      pass2pendings,
      pass3pendings,
      pass4pendings,
      pass5pendings
    );
  }

  return result;
}



/**
	@return 1 if the type is uninstantiable as a sim or 0 otherwise
*/
static
int ValidRealInstantiateType(struct TypeDescription *def)
{
  if (def==NULL) return 1;
  switch(GetBaseType(def)){
  case real_constant_type:
  case boolean_constant_type:
  case integer_constant_type:
  case symbol_constant_type:
  case real_type:
  case boolean_type:
  case integer_type:
  case symbol_type:
  case set_type:
  case dummy_type:
    return 0;
  case model_type:
    if (GetModelParameterCount(def) !=0) {
      FPRINTF(ASCERR,
            "You cannot instance parameterized types by themselves yet.\n");
      FPRINTF(ASCERR,"They can only be contained in models or arrays.\n");
      return 1;
    }
    return 0;
  case array_type:
  case relation_type:
  case logrel_type:
  case when_type:
    FPRINTF(ASCERR,
            "You cannot instance arrays and relations by themselves.\n");
    FPRINTF(ASCERR,"They can only be contained in models or arrays.\n");
    return 1;
  default:
    ASC_PANIC("Unknown definition type.\n");			/*NOTREACHED*/

  }
}

/**
	this function not recursive
*/
static
struct Instance *NewRealInstantiate(struct TypeDescription *def,
                                 int intset)
{
  struct Instance *result;
  /* CONSOLE_DEBUG("..."); */

  result = ShortCutMakeUniversalInstance(def); /*does quick Universal check */
  if (result) return result;

  switch(GetBaseType(def)){
  case real_type:
  case real_constant_type:
    return CreateRealInstance(def);
  case boolean_type:
  case boolean_constant_type:
    return CreateBooleanInstance(def);
  case integer_type:
  case integer_constant_type:
    return CreateIntegerInstance(def);
  case symbol_type:
  case symbol_constant_type:
    return CreateSymbolInstance(def);
  case set_type:
    return CreateSetInstance(def,intset);
  case dummy_type:
    return CreateDummyInstance(def);
  case model_type:
    return NewInstantiateModel(def); /*this is now a nonrecursive controller */
  case array_type:
  case relation_type:
  case logrel_type:
  case when_type:
    FPRINTF(ASCERR,
            "You cannot instance arrays and relations by themselves.\n");
    FPRINTF(ASCERR,
            "They can only be contained in models or arrays.\n");
    return NULL; /* how did we get here? */
  default:
    ASC_PANIC("Unknown definition type.\n");	/*NOTREACHED*/

  }
}

static
void ExecDefMethod(struct Instance *root,symchar *simname, symchar *defmethod)
{
  //enum Proc_enum runstat;
  struct Name *name;
  if (InstanceKind(root) == MODEL_INST && defmethod != NULL) {
    name = CreateIdName(defmethod);
    Initialize(root,name,(char *)SCP(simname),ASCERR,(WP_BTUIFSTOP|WP_STOPONERR),NULL,NULL);
    DestroyName(name);
  }
}

struct Instance *NewInstantiate(symchar *type, symchar *name, int intset,
                                symchar *defmethod)
{
  struct Instance *result;	/* the SIM_INSTANCE */
  struct Instance *root;	/* the thing created by instantiate */
  struct TypeDescription *def;

  ++g_compiler_counter;/*instance tree may change:increment compiler counter*/
  def = FindType(type);
  if (def==NULL) {
    FPRINTF(ASCERR,"Cannot find the type for %s in the library\n",SCP(type));
    return NULL;
  }
  if (ValidRealInstantiateType(def)) return NULL;
  /* don't want to set up all the sim crap and then destroy it.
   * this stuff below core dumps if root comes back NULL, so we
   * check here first.
   */

  ClearIteration();
  result = CreateSimulationInstance(def,name);
  root = NewRealInstantiate(def,intset);
  if (root == NULL) {
    DestroyInstance(result,NULL);
    return NULL;
  }
  LinkToParentByPos(result,root,1);
  if (g_ExtVariablesTable!=NULL) {
    SetSimulationExtVars(result,g_ExtVariablesTable);
    g_ExtVariablesTable = NULL;
  }
  ClearIteration();
  ExecDefMethod(root,name,defmethod);
  return result;
}


#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
int IsInstanceComplete(struct Instance *i)
{
  struct BitList *blist;
  if (i==NULL) {
    return 0;
  }
  blist = InstanceBitList(i);
  if (blist) { /* only MODEL_INST have bitlists */
    if (BitListEmpty(blist))
      return 1;
  }
  return 1; /* atoms are assumed to be complete */
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */


int IncompleteArray(CONST struct Instance *i)
{
  unsigned long c,len;
  struct Instance *child;
  struct TypeDescription *desc;
  len = NumberChildren(i);
  for(c=1;c<=len;c++){
    child = InstanceChild(i,c);
    if (child != NULL){
      switch(InstanceKind(child)){
      case ARRAY_INT_INST:
      case ARRAY_ENUM_INST:
        desc = InstanceTypeDesc(child);
        if ((!GetArrayBaseIsRelation(desc))&&
            (!RectangleArrayExpanded(child))&&
            (!GetArrayBaseIsLogRel(desc))) {
          return 1;
        }
      default:
        break; /* out of switch, not out of for */
      }
    }
  }
  return 0;
}

static
void AddIncompleteInst(struct Instance *i)
{
  struct BitList *blist;
  asc_assert(i!=NULL);
  if ( ( (blist = InstanceBitList(i)) != NULL &&
        !BitListEmpty(blist)  ) ||
      IncompleteArray(i)) {
    /* model and atom/model array inst pending even if they aren't */
    AddBelow(NULL,i);
    /* add PENDING model or non-relation array */
  }
}

/**
	On entry it is assumed that the instance i has already been
	refined and so will not MOVE during subsequent work.
	The process here must be kept in sync with NewRealInstantiateModel,
	but must, additionally, deal ok with array instances as input.
*/
void NewReInstantiate(struct Instance *i)
{
  struct Instance *result;
  unsigned long pass1pendings,pass2pendings,pass3pendings,pass4pendings,pass5pendings;
#if TIMECOMPILER
  double start, phase1t,phase2t,phase3t,phase4t,phase5t,phase6t;
#endif
  ++g_compiler_counter;/*instance tree will change:increment compiler counter*/
  asc_assert(i!=NULL);
  if (i==NULL || !IsCompoundInstance(i)) return;
  /* can't reinstantiate simple objects, missing objects */

  pass1pendings = 0L;
  pass2pendings = 0L;
  pass3pendings = 0L;
  pass4pendings = 0L;
  pass5pendings = 0L;
#if TIMECOMPILER
  start = tm_cpu_time();
#endif
  result = Pass1InstantiateModel(NULL,&pass1pendings,i);
#if TIMECOMPILER
  phase1t = tm_cpu_time();
#endif
  if (result!=NULL) {
    SilentVisitInstanceTree(result,Pass2SetRelationBits,0,0);
    result = Pass2InstantiateModel(result,&pass2pendings);
  }else{
    ASC_PANIC("Reinstantiation phase 2 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase2t = tm_cpu_time();
#endif
  if (result!=NULL) {
    SilentVisitInstanceTree(result,Pass3SetLogRelBits,0,0);
    result = Pass3InstantiateModel(result,&pass3pendings);
  }else{
    ASC_PANIC("Reinstantiation phase 3 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase3t = tm_cpu_time();
#endif
  if (result!=NULL) {
    SilentVisitInstanceTree(result,Pass4SetWhenBits,0,0);
    result = Pass4InstantiateModel(result,&pass4pendings);
  }else{
    ASC_PANIC("Reinstantiation phase 4 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase4t = tm_cpu_time();
#endif
	if (result!=NULL) {
    SilentVisitInstanceTree(result,Pass5SetLinkBits,0,0);
    result = Pass5InstantiateModel(result,&pass5pendings);
  }else{
    ASC_PANIC("Reinstantiation phase 5 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase5t = tm_cpu_time();
#endif
  if (result!=NULL) {
    if (!pass1pendings && !pass2pendings && !pass3pendings && !pass4pendings && !pass5pendings){
      DefaultInstanceTree(result);
    }else{
      FPRINTF(ASCERR,"There are unexecuted statements in the instance.\n");
      FPRINTF(ASCERR,"Default assignments not executed.\n");
    }
  }else{
    ASC_PANIC("Reinstantiation phase 6 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase6t = tm_cpu_time();
  CONSOLE_DEBUG("Reinstantiation times:\n");
  CONSOLE_DEBUG("Phase 1 models \t\t%0.6f s\n",(phase1t-start));
  CONSOLE_DEBUG("Phase 2 relations \t\t%0.6f s\n",(phase2t-phase1t));
  CONSOLE_DEBUG("Phase 3 logicals \t\t%0.6f s\n",(phase3t-phase2t));
  CONSOLE_DEBUG("Phase 4 when-case \t\t%0.6f s\n",(phase4t-phase3t));
  CONSOLE_DEBUG("Phase 5 LINKs \t\t%0.6f s\n",(phase5t-phase4t));
  CONSOLE_DEBUG("Phase 6 defaults \t\t%0.6f s\n",(phase6t-phase5t));
  CONSOLE_DEBUG("Total\t\t%0.6f s\n",(phase6t-start));
#endif
  return;
}

/*------------------------------------------------------------------------------
	Some supporting code for the new (how new?) partial instantiation,
	and encapsulation schemes.
*/

void SetInstantiationRelnFlags(unsigned int flag)
{
  g_instantiate_relns = flag;
}

unsigned int GetInstantiationRelnFlags(void)
{
  return g_instantiate_relns;
}

#if 0 && defined(DISUSED)
/**
	This is the version of instantiate to deal with with 'patched'
	types. Here name is the name of the patch that is to be
	instantiated. We first find the 'original' type, instantiate it
	and then apply the patch. The things that are properly and fully
	supported is external relations, which is the real reason that
	the patch was designed.
*/
void UpdateInstance(struct Instance *root, /* the simulation root */
                    struct Instance *target,
                    CONST struct StatementList *slist)
{
  struct gl_list_t *list, *instances = NULL;
  unsigned long len, c;
  struct Statement *stat;
  REL_ERRORLIST err = REL_ERRORLIST_EMPTY;
  struct Instance *scope;
  struct Name *name;

  (void)root;  /*  stop gcc whine about unused parameter */

  list = GetList(slist);
  if (!list) return;
  len = gl_length(list);
  for (c=1;c<=len;c++) {
    stat = (struct Statement *)gl_fetch(list,c);
    switch (StatementType(stat)) {
    case EXT:
      name = ExternalStatScope(stat);
      if (name==NULL) {
        scope = target;
      }else{
        instances = FindInstances(target,name,&err);
        if (instances) {
          if (gl_length(instances)!=1) {
            FPRINTF(ASCERR,"More than 1 scope instance found !!\n");
			scope = NULL;
          }else{
            scope = (struct Instance *)gl_fetch(instances,1L);
          }
          gl_destroy(instances);
        }else{
          FPRINTF(ASCERR,"Unable to find scope instance !!\n");
          scope = target;
        }
      }
      ExecuteEXT(scope,stat);
      break;
    default:
      break;
    }
  }
}

/**
	this function instantiates a thing of type name
	without doing relations.

	Relations are then hacked in from external places
	but OTHERWISE the object appears as a regular
	ASCEND object. (note HACKED is the right word.)

	@DEPRECATED This function is obsolete; bintoken.c and multiphase
	instantiation make it irrelevant.
*/
struct Instance *InstantiatePatch(symchar *patch,
                                  symchar *name, int intset)
{
  struct Instance *result;	/* the SIM_INSTANCE */
  struct Instance *root;	/* the thing created by instantiate */
  struct TypeDescription *patchdef;
  symchar *original;
  unsigned int oldflags;

  ++g_compiler_counter;/*instance tree will change:increment compiler counter*/
  patchdef = FindType(patch);
  if (patchdef==NULL) {
    FPRINTF(ASCERR,"Cannot find the patch %s in the libary\n",SCP(patch));
    return NULL;
  }
  if (GetBaseType(patchdef)!=patch_type) {
    FPRINTF(ASCERR,"Given type \"%s\" is not a patch\n",SCP(patch));
    return NULL;
  }
  /*
   * Do the partial instantiation with the original.
   * This requires setting up the instantiate relations flags.
   * Any failures after this require going to cleanup.
   */

  original = GetName(GetPatchOriginal(patchdef));
  asc_assert(original!=NULL);
  oldflags = GetInstantiationRelnFlags();
  SetInstantiationRelnFlags(EXTRELS);
  result = Instantiate(original,name,intset,NULL);
  if (result) {
    root = GetSimulationRoot(result);
    if (!root) {
      FPRINTF(ASCERR,"NULL root instance\n");
      goto cleanup;
    }
    UpdateInstance(root,root,GetStatementList(patchdef)); /* cast statement?*/
  }else{
    FPRINTF(ASCERR,"Instantiation failure: NULL simulation\n");
  }

 cleanup:
  SetInstantiationRelnFlags(oldflags);
  return result;
}
#endif
