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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//*
	@file
	Ascend Instantiator Implementation

	@TODO this file is enormous and should be broken into pieces! -- JP
*//*
	by Tom Epperly
	Created: 1/24/90
	Last in CVS: $Revision: 1.84 $ $Date: 2003/02/06 04:08:30 $ $Author: ballan $
*/

#include <stdarg.h>
#include <errno.h>

#include <utilities/config.h>
#include <utilities/ascConfig.h>

#ifdef ASC_SIGNAL_TRAPS
# include <utilities/ascSignal.h>
#endif

/*#include <stdlib.h>*/
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/error.h>
#include <general/pool.h>
#include <general/list.h>
#include <general/dstring.h>

#if TIMECOMPILER
#include <time.h>
#include <general/tm_time.h>
#endif
#include "bit.h"
#include "symtab.h"
#include "functype.h"
#include "expr_types.h"
#include "stattypes.h"
#include "statement.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "module.h"
#include "library.h"
#include "sets.h"
#include "setio.h"
#include "extfunc.h"
#include "extcall.h"
#include "forvars.h"
#include "exprs.h"
#include "name.h"
#include "nameio.h"
#include "vlist.h"
#include "slist.h"
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
#include "watchpt.h"
#include "initialize.h"
#include "instantiate.h"

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

#define AVG_CASES 2L		/* size to which all cases lists are */
                                /* initialized (WHEN instance) */
#define AVG_REF 2L		/* size to which all list of references */
                                /* in a case are initialized (WHEN) */

#define NO_INCIDENCES 7         /* avg number of vars in a external reln */

static int g_iteration = 0;	/* the current iteration. */

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

/* #define DEBUG_RELS */
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
static int DigestArguments(struct Instance *,
                    struct gl_list_t *, struct StatementList *,
                    struct StatementList *, struct Statement *);
static int DeriveSetType(CONST struct Set *, struct Instance *,CONST unsigned int);

static struct gl_list_t *FindInsts(struct Instance *, CONST struct VariableList *,
                            enum find_errors *);

static void MissingInsts(struct Instance *, CONST struct VariableList *,int);
static struct gl_list_t *FindArgInsts(struct Instance *, struct Set *,
                               enum find_errors *);
static void AddIncompleteInst(struct Instance *);
static int CheckALIASES(struct Instance *, struct Statement *);
static int CheckARR(struct Instance *, struct Statement *);
static int CheckISA(struct Instance *, struct Statement *);
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
static void ReEvaluateSELECT(struct Instance *, unsigned long *,
                             struct Statement *, int, int *);

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
	va_list args;
	va_start(args,fmt);
	if(stat!= NULL){
		va_error_reporter(sev
			, Asc_ModuleBestName(StatementModule(stat))
			, StatementLineNum(stat), NULL
			, fmt, args
		);
	}else{
		va_error_reporter(sev
			, NULL, 0, NULL
			, fmt, args
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
  enum find_errors err;
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
  register CONST struct Set *ptr;
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
  enum find_errors ferr;
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
  rhslist = FindInstances(inst,name,&ferr);
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
  enum find_errors ferr;
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
  rhsinstlist = FindInsts(inst,GetStatVarList(statement),&ferr);
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
  setinstl = FindInstances(inst,NamePointer(vlist),&ferr);
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
        DestroyValue(&value);
        STATEMENT_ERROR(statement, "Arguments do not conform to requirements");
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
	 What is MPI? parallel computing stuff?? -- JP
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
  FPRINTF(ASCERR,"Parameter list waiting on sufficient type or value of:\n");
  if (argset !=NULL && argn >0) {
    FPRINTF(ASCERR,"  Argument %lu:",argn);
    WriteSetNode(ASCERR,argset);
  }
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
  enum find_errors ferr;
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
        il = FindArgInsts(parent,argset,&ferr);
        if (il == NULL) {
          switch(ferr) {
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
        il = FindArgInsts(parent,argset,&ferr);
        if (il == NULL) {
          switch(ferr) {
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
int MPICheckWBTS(struct Instance *tmpinst, struct Statement *statement)
{
  struct gl_list_t *instances;
  unsigned long c,len;
  enum find_errors err;
  struct Instance *head = NULL;

  instances = FindInsts(tmpinst,GetStatVarList(statement),&err);
  if (instances==NULL) {
    switch(err){
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
  enum find_errors err;

  instances = FindInsts(tmpinst,GetStatVarList(statement),&err);
  if (instances==NULL) {
    switch(err){
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
  enum find_errors ferr;

  asc_assert(nptr!=NULL);
  asc_assert(tmpinst!=NULL);
  insts = FindInstances(tmpinst,nptr,&ferr);
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
    CONST struct Statement *stat;
    switch (pp->status) {
    case pp_ISA:
      msg = "Oddly unable to construct parameter scalar";
      stat = pp->s;
      break;
    case pp_ISAARR:
      msg = "Unable to construct array parameter. Probably missing subscripts";
      stat = pp->s;
      break;
    case pp_ARR:
      msg = "Unable to check parameter array subscripts.";
      stat = pp->s;
      break;
    case pp_ASGN:
      msg = "Unable to execute assigment: LHS unmade or RHS not evaluatable";
      stat = pp->s;
      break;
    case pp_ASSC:
      msg ="Unable to set scalar param: RHS not evaluatable or incorrect type";
      stat = pp->s;
      break;
    case pp_ASAR:
      msg = "Parameters: Not all array elements assigned during refinement";
      stat = pp->s;
      break;
    case pp_WV:
      msg = "Unable to verify parameter value: probably bad WITH_VALUE RHS";
      stat = pp->s;
      break;
    case pp_ERR:
      stat = statement;
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

  asc_assert(StatementType(statement)==ISA);
  if (StatWrong(statement)) {
    /* incorrect statements should be warned about when they were
     * marked wrong, so we just ignore them here.
     */
    return 1;
  }
  if ((def = FindType(GetStatType(statement)))!=NULL){
    if ((GetStatSetType(statement)!=NULL) != (GetBaseType(def)==set_type)){
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
    intset = CalcSetType(GetStatSetType(statement),statement);
    if (intset < 0) { /* incorrect set type */
      STATEMENT_ERROR(statement,"Illegal set type encountered.");
      /* should never happen due to lint */
      return 0;
    }
    vlist = GetStatVarList(statement);
    while (vlist!=NULL){
      MakeInstance(NamePointer(vlist),def,intset,inst,statement,arginst);
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
static
struct gl_list_t *FindArgInsts(struct Instance *parent,
                               struct Set *s,
                               enum find_errors *err)
{
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
	Find instances: Make sure at least one thing is found for each name item
	on list (else returned list will be NULL) and return the collected instances.
*/
static
struct gl_list_t *FindInsts(struct Instance *inst,
                            CONST struct VariableList *list,
                            enum find_errors *err)
{
  struct gl_list_t *result,*temp;
  unsigned c,len;
  result = gl_create(7L);
  while(list!=NULL){
    temp = FindInstances(inst,NamePointer(list),err);
    if (temp==NULL){
      gl_destroy(result);
      return NULL;
    }
    len = gl_length(temp);
    for(c=1;c<=len;c++) {
      gl_append_ptr(result,gl_fetch(temp,c));
    }
    gl_destroy(temp);
    list = NextVariableNode(list);
  }
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
  enum find_errors err;

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
                CONST struct VariableList *list,
                enum find_errors *err)
{
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
  register struct Instance *i=i1;
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
  enum find_errors err;
  struct gl_list_t *instances; /* presently leaking ? */
  struct Instance *inst, *arginst;
  unsigned long c,len;
  int suberr;

  asc_assert(StatementType(statement)==IRT);

  def = FindType(GetStatType(statement)); /* sort of redundant, but safe */
  if (def!=NULL) {
    instances = FindInsts(work,GetStatVarList(statement),&err);
    if (instances != NULL){
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
      switch(err){
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

static
int ExecuteATS(struct Instance *inst, struct Statement *statement)
{
  struct gl_list_t *instances;
  enum find_errors err;
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
        inst1 = (struct Instance *)gl_fetch(instances,1);
        for(c=2;c<=len;c++){
          inst2 = (struct Instance *)gl_fetch(instances,c);
          inst1 = MergeInstances(inst1,inst2);
          if (inst1==NULL){
            STATEMENT_ERROR(statement, "Fatal ARE_THE_SAME error");
            ASC_PANIC("Fatal ARE_THE_SAME error");
            /*NOTREACHED Wanna bet? ! */
          }
        }
        PostMergeCheck(inst1);
      }
    }else{
      STATEMENT_ERROR(statement,
           "ARE_THE_SAME statement contains unconformable instances");
    }
    gl_destroy(instances);
    return 1;
  }else{
    switch(err){
    case impossible_instance:
      MissingInsts(inst,GetStatVarList(statement),1);
      STATEMENT_ERROR(statement, "ARE_THE_SAME contains impossible instance");
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
static
int ExecuteAA(struct Instance *inst, struct Statement *statement)
{
  struct gl_list_t *instances;
  enum find_errors err;
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
    switch(err){
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


/**
	ok, now we can whine real loud about what's missing.
	even in relations referencing relations, because they
	should have been added to pendings in dependency order. (hah!)
*/
static
int ExecuteREL(struct Instance *inst, struct Statement *statement)
{
  struct Name *name;
  enum relation_errors err;
  enum find_errors ferr;
  struct relation *reln;
  struct Instance *child;
  struct gl_list_t *instances;
  enum Expr_enum reltype;

#ifdef DEBUG_RELS
  CONSOLE_DEBUG("ENTERED ExecuteREL\n");
#endif
  name = RelationStatName(statement);
  instances = FindInstances(inst,name,&ferr);
  /* see if the relation is there already */
  if (instances==NULL){
    if (ferr == unmade_instance){		/* make a reln head */
      child = MakeRelationInstance(name,FindRelationType(),
                                   inst,statement,e_token);
      if (child==NULL){
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

  /*
   * child now contains the pointer to the relation instance.
   * We should perhaps double check that the reltype
   * has not been set or has been set to e_undefined.
   */
  if (GetInstanceRelation(child,&reltype)==NULL) {
    if ( (g_instantiate_relns & TOKRELS) ==0) {
#ifdef DEBUG_RELS
      STATEMENT_NOTE(statement, "TOKRELS 0 found in compiling relation.");
#endif
      return 1;
    }
#if TIMECOMPILER
    g_ExecuteREL_CreateTokenRelation_calls++;
#endif
    reln = CreateTokenRelation(inst,child,RelationStatExpr(statement),
                               &err,&ferr);
    if (reln != NULL){
      SetInstanceRelation(child,reln,e_token);
#ifdef DEBUG_RELS
      STATEMENT_NOTE(statement, "Created relation.");
#endif
      return 1;
    }else{
      SetInstanceRelation(child,NULL,e_token);
      switch(err){
      case incorrect_structure:
        WSSM(ASCERR,statement, "Bad relation expression in ExecuteRel",3);
        return 1;
      case incorrect_inst_type:
        WSSM(ASCERR,statement, "Incorrect instance types in relation",3);
        return 1;
      case incorrect_boolean_inst_type:
        WSSM(ASCERR,statement, "Incorrect boolean instance in relation",3);
        return 1;
      case incorrect_integer_inst_type:
        WSSM(ASCERR,statement, "Incorrect integer instance in relation",3);
        return 1;
      case incorrect_symbol_inst_type:
        WSSM(ASCERR,statement, "Incorrect symbol instance in relation",3);
        return 1;
      case incorrect_real_inst_type:
        WSSM(ASCERR,statement,
                "Incorrect real child of atom instance in relation",3);
        return 1;
      case find_error:
        switch(ferr){
        case unmade_instance:
        case undefined_instance:
          WSSM(ASCERR,statement,
                     "Unmade or Undefined instances in relation",3);
          return 1;
        case impossible_instance:
          WSSM(ASCERR,statement,
                     "Relation contains an impossible instance",3);
          return 1;
        case correct_instance:
          ASC_PANIC("Incorrect error response.\n");/*NOTREACHED*/
        default:
          ASC_PANIC("Unknown error response.\n");/*NOTREACHED*/
        }
      case integer_value_undefined:
      case real_value_wild:
      case real_value_undefined:
        WriteUnexecutedMessage(ASCERR,statement,
         "Unassigned constants or wild dimensioned real constant in relation");
          return 1;
      case okay:
        ASC_PANIC("Incorrect error response.\n");/*NOTREACHED*/
      default:
        ASC_PANIC("Unknown error response.\n");/*NOTREACHED*/

      }
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
  enum find_errors ferr;
  struct relation *reln;
  struct Instance *rel;
  struct gl_list_t *instances;
  enum Expr_enum reltype;

  name = RelationStatName(statement);
  instances = FindInstances(inst,name,&ferr);
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
  enum find_errors ferr;
  struct logrelation *lreln;
  struct Instance *lrel;
  struct gl_list_t *instances;

  name = LogicalRelStatName(statement);
  instances = FindInstances(inst,name,&ferr);
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
  enum find_errors ferr;
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
  instances = FindInstances(inst,name,&ferr);
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
  enum logrelation_errors err;
  enum find_errors ferr;
  struct logrelation *lreln;
  struct Instance *child;
  struct gl_list_t *instances;

  name = LogicalRelStatName(statement);
  instances = FindInstances(inst,name,&ferr);
  /* see if the logical relation is there already */
  if (instances==NULL){
    gl_destroy(instances);
    if (ferr == unmade_instance){
      child = MakeLogRelInstance(name,FindLogRelType(),inst,statement);
      if (child==NULL){
        WUEMPASS3(ASCERR,statement, "Unable to create expression structure");
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
    if((lreln = CreateLogicalRelation(inst,child,
                            LogicalRelStatExpr(statement),&err,&ferr))!=NULL
    ){
      SetInstanceLogRel(child,lreln);
      return 1;
    }else{
      SetInstanceLogRel(child,NULL);
      switch(err){
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
        switch(ferr){
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
struct gl_list_t *GetExtCallArgs(struct Instance *inst,
                                   struct Statement *stat,
                                   enum find_errors *ferr,
				struct gl_list_t **names)
{
  CONST struct VariableList *vl;
  struct gl_list_t *result;
  enum find_errors ferr2;

  /* the two process calls could be merged if we change findpaths
     to return instance/name pairs rather than just names. it does
     know the instances.
   */

  vl = ExternalStatVlistRelation(stat);
  result = ProcessExtRelArgs(inst, vl, ferr);

  *names = NULL;
  if (result != NULL) {
    *names = ProcessExtRelArgNames(inst,vl,&ferr2);
    asc_assert(*ferr == ferr2);
  }
  return result;
}

static /* blackbox only */
struct Instance *CheckExtCallData(struct Instance *inst,
                                  struct Statement *stat,
                                  enum find_errors *ferr,
                                  struct Name **name)
{
  struct Name *n;
  struct Instance *result;
  enum find_errors ferr2;

  n = ExternalStatDataBlackBox(stat);
  result = ProcessExtRelData(inst, n, ferr);

  *name = NULL;
  if (result != NULL) {
    *name = ProcessExtRelDataName(inst, n, &ferr2);
    asc_assert(*ferr == ferr2);
  }
  return result;
}



static int Pass2ExecuteBlackBoxEXTLoop(struct Instance *inst, struct Statement *statement);

int ExecuteBBOXElement(struct Instance *inst, struct Statement *statement, struct Instance *subject, struct gl_list_t *inputs,  struct BlackBoxCache * common, long c, CONST char *context);

/**
	This function does the job of creating an instance of a 'black box'
	external relation or set of relations.
	It does it by working as if the user wrote a for loop
	over the outputs.
	While managing this for loop, we have to make sure that if
	we are already inside a for loop we don't mess that one up.
	The role of savedfortable is to manage the change of
	model context when one model instantiates another inside
	a for loop. For relations in particular, the saved for
	table will always be null unless we drop the multipass
	instantiation scheme.
*/
static int ExecuteBlackBoxEXT(struct Instance *inst
		, struct Statement *statement
){
  int alreadyInFor = 0;
  int executeStatus = 0;
  struct for_table_t *SavedForTable;

  if (GetEvaluationForTable() != NULL) {
	/* is this the right test? FIXME if not multipass instantiation. */
	/* as written, this might see a fortable from elsewhere */
    alreadyInFor = 1;
  }

  if (!alreadyInFor) { /* pushing/popping null table...*/
    SavedForTable = GetEvaluationForTable();
    SetEvaluationForTable(CreateForTable());
  }
  executeStatus = Pass2ExecuteBlackBoxEXTLoop(inst,statement);
  if (!alreadyInFor) {
    DestroyForTable(GetEvaluationForTable());
    SetEvaluationForTable(SavedForTable);
  }
  if ( executeStatus ) {
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
  symchar *name;
  struct Expr *ex, *one, *en;
  unsigned long c,len;
  long aindex;
  struct value_t value;
  struct set_t *sptr;
  struct for_var_t *fv;

  struct BlackBoxCache * common;
  ExtBBoxInitFunc * init;
  char *context;
  struct Instance *data=NULL , *subject = NULL;
  enum find_errors ferr;
  struct gl_list_t *arglist=NULL;
  struct ExternalFunc *efunc = NULL;
  CONST char *funcname = NULL;
  unsigned long n_input_args=0L, n_output_args=0L; /* formal arg counts */
  unsigned long n_inputs_actual=0L, n_outputs_actual=0L; /* atomic arg counts */
  struct gl_list_t *inputs, *outputs, *argListNames;
  struct Name *dataName = NULL;
  unsigned long start,end;
  struct Set *extrange= NULL;

  /* common stuff do once ------------ */

  /*
	note ignoring return codes as all is guaranteed to work by passing
	the checks done before this statement was attempted.
  */
  if (ExternalStatDataBlackBox(statement) != NULL) {
    data = CheckExtCallData(inst, statement, &ferr, &dataName);
    /* should never be here, if checkext were being used properly somewhere. it isn't */
    if (data==NULL && ferr != correct_instance){
      /* should never be here, if checkext were being used properly somewhere. it isn't */
      switch(ferr){
      case unmade_instance:
		STATEMENT_ERROR(statement,"Statement contains unmade data instance");
        return 1;
      case undefined_instance:
        STATEMENT_ERROR(statement,"Statement contains undefined data instance\n");
        return 1; /* for the time being give another crack */
      case impossible_instance:
        STATEMENT_ERROR(statement,"Statement contains impossible data instance\n");
        return 1;
      default:
        STATEMENT_ERROR(statement,"Unhandled case!");
        return 1;
      }
    }
  }

  /* expand the formal args into a list of lists of realatom args. */
  arglist = GetExtCallArgs(inst, statement, &ferr, &argListNames);
  if (arglist==NULL){
    /* should never be here, if checkext were being used properly somewhere. it isn't */
    switch(ferr){
    case unmade_instance:
      STATEMENT_ERROR(statement,"Statement contains unmade argument instance\n");
      return 1;
    case undefined_instance:
      STATEMENT_ERROR(statement,"Statement contains undefined argument instance\n");
      return 1;
    case impossible_instance:
      instantiation_error(ASC_USER_ERROR,statement,"Statement contains impossible instance\n");
      return 1;
    default:
      instantiation_error(ASC_PROG_ERR,statement,"Unhandled case!");
      return 1;
    }
  }
  funcname = ExternalStatFuncName(statement);
  efunc = LookupExtFunc(funcname);
  if (efunc == NULL) {
    return 1;
  }

  n_input_args = NumberInputArgs(efunc);
  n_output_args = NumberOutputArgs(efunc);
  if ((len =gl_length(arglist)) != (n_input_args + n_output_args)) {
    instantiation_error(ASC_PROG_ERR,statement
		,"Unable to create external expression structure."
	);
    return 1;
  }

  /* we should have a valid arglist at this stage */
  if (CheckExtCallArgTypes(arglist)) {
    instantiation_error(ASC_USER_ERROR,statement,"Wrong type of args to external statement");
    DestroySpecialList(arglist);
    return 1;
  }
  start = 1L;
  end = n_input_args;
  inputs = LinearizeArgList(arglist,start,end);
  n_inputs_actual = gl_length(inputs);

  /* Now process the outputs */
  start = n_input_args+1;
  end = n_input_args + n_output_args;
  outputs = LinearizeArgList(arglist,start,end);
  n_outputs_actual = gl_length(outputs);

/*
  char *tempnamestr = WriteNameString(dataName);
  CONSOLE_DEBUG("dataName = %s", tempnamestr);
  ASC_FREE(tempnamestr);
*/

  /* Now create the relations, all with the same common. */
  common = CreateBlackBoxCache(n_inputs_actual,n_outputs_actual, argListNames, dataName, efunc);
  common->interp.task = bb_first_call;
  context = WriteInstanceNameString(inst, NULL);

  /* now set up the for loop index --------------------------------*/
  name = AddSymbolL(BBOX_RESERVED_INDEX, BBOX_RESERVED_INDEX_LEN);
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
  DestroyValue(&value);
  DestroySetNode(extrange);

/* ------------ */ /* ------------ */
  /* and now for cleaning up shared data. */
  init = GetInitFunc(efunc);
  if(init){
    if( (*init)( &(common->interp), data, arglist) ){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in blackbox initfn");
    }
  }
  common->interp.task = bb_none;
  ascfree(context);
  DeleteRefBlackBoxCache(NULL, &common);
  gl_destroy(inputs);
  gl_destroy(outputs);
  DestroySpecialList(arglist);
  DeepDestroySpecialList(argListNames,(DestroyFunc)DestroyName);
  DestroyName(dataName);
/* ------------ */ /* ------------ */

  /*  currently designed to always succeed or fail permanently.
   *  We reached this point meaning we've processed everything.
   *  Therefore the statment returns 1 and becomes no longer pending.
   */
  return 1;
}

int ExecuteBBOXElement(struct Instance *inst, struct Statement *statement, struct Instance *subject, struct gl_list_t *inputs,  struct BlackBoxCache * common, long c, CONST char *context)
{
  struct Name *name;
  enum find_errors ferr;
  struct gl_list_t *instances = NULL;
  struct Instance *child = NULL;
  struct relation *reln  = NULL;
  enum Expr_enum reltype;

#ifdef DEBUG_RELS
  CONSOLE_DEBUG("ENTERED ExecuteBBOXElement\n");
#endif

  /* make or find the instance */
  name = ExternalStatNameRelation(statement);
  instances = FindInstances(inst, name, &ferr);
  /* see if the relation is there already. If it is, we're really hosed. */
  if (instances==NULL){
    if (ferr == unmade_instance){		/* make a reln head */
      child = MakeRelationInstance(name,FindRelationType(),
                                   inst,statement,e_blackbox);
      if (child==NULL){
        STATEMENT_ERROR(statement, "Unable to create expression structure");
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

static
struct gl_list_t *CheckGlassBoxArgs(struct Instance *inst,
                                    struct Statement *stat,
                                    enum relation_errors *err,
                                    enum find_errors *ferr)
{
  struct Instance *var;
  CONST struct VariableList *vl;
  struct gl_list_t *varlist = NULL, *tmp = NULL;
  unsigned long len,c;
  int error = 0;

  vl = ExternalStatVlistRelation(stat);
  if (!vl) {
    *ferr = impossible_instance; /* a relation with no incidence ! */
    return NULL;
  }

  ListMode = 1;				/* order is very important */
  varlist = gl_create(NO_INCIDENCES);	/* could be fine tuned */
  while (vl!=NULL) {
    tmp = FindInstances(inst,NamePointer(vl),ferr);
    if (tmp) {
      len = gl_length(tmp);
      for (c=1;c<=len;c++) {
        var = (struct Instance *)gl_fetch(tmp,c);
        if (InstanceKind(var) != REAL_ATOM_INST) {
          error++;
          *err = incorrect_inst_type;
          *ferr = correct_instance;
          gl_destroy(tmp);
          goto cleanup;
        }
        gl_append_ptr(varlist,(VOIDPTR)var);
      }
      gl_destroy(tmp);
    }else{			/* ferr will be already be set */
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

static
int CheckGlassBoxIndex(struct Instance *inst,
                       struct Statement *stat,
                       enum relation_errors *err)
{
  int result;
  long int iresult;
  char *tail;
  CONST struct Name *n;
  symchar *str;		   /* a string representation of the index */

  (void)inst;  /*  stop gcc whine about unused parameter  */

  n = ExternalStatDataGlassBox(stat);
  if (!n) {
    *err = incorrect_num_args;		/* we must have an index */
    return -1;
  }

  str = SimpleNameIdPtr(n);
  if (str) {
#if 0
    result = atoi(SCP(str));	/* convert to integer. use strtol */
#endif
    errno = 0;
    iresult = strtol(SCP(str),&tail,0);
    if (errno != 0 || (iresult == 0 && tail == SCP(str))) {
      *err = incorrect_structure;
      return -1;
    }
    result = iresult; /* range errror possible. */
    *err = okay;
    return result;
  }else{
    *err = incorrect_structure;		/* we really need to expand */
    return -1;				/* the relation_error types. !! */
  }
}

static
int ExecuteGlassBoxEXT(struct Instance *inst, struct Statement *statement)
{
  struct Name *name;
  enum relation_errors err;
  enum find_errors ferr;
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
  instances = FindInstances(inst,name,&ferr);
  if (instances==NULL){
    if (ferr == unmade_instance){			/* glassbox reln */
      child = MakeRelationInstance(name,FindRelationType(),
                                   inst,statement,e_glassbox);
      if (child==NULL){
        STATEMENT_ERROR(statement, "Unable to create expression structure");
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
  varlist = CheckGlassBoxArgs(inst,statement,&err,&ferr);
  if (varlist==NULL){
    switch(ferr){
    case unmade_instance:
      return 0;
    case undefined_instance:
      return 0; 		/* for the time being give another crack */
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

static
int ExecuteEXT(struct Instance *inst, struct Statement *statement)
{
  int mode;

  /* CONSOLE_DEBUG("..."); */

  mode = ExternalStatMode(statement);
  switch(mode) {
  case ek_method:
    instantiation_error(ASC_USER_ERROR,statement
			,"Invalid external statement in declarative section. \n");
    return 1;
  case ek_glass:
    return ExecuteGlassBoxEXT(inst,statement);
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
static
void StructuralAsgnErrorReport(struct Statement *statement,
                               struct value_t *value)
{
  STATEMENT_ERROR(statement,
    "Structural assignment right hand side is not constant");
  DestroyValue(value);
}

/*
 * returns 1 if error will be persistent, or 0 if error may
 * go away later when more compiling is done.
 * Issues some sort of message in the case of persistent errors.
 */
static
int AsgnErrorReport(struct Statement *statement, struct value_t *value)
{
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

static
void ReAssignmentError(CONST char *str, struct Statement *statement)
{
  char *msg = ASC_NEW_ARRAY(char,strlen(REASSIGN_MESG1)+strlen(REASSIGN_MESG2)+strlen(str)+1);
  strcpy(msg,REASSIGN_MESG1);
  strcat(msg,str);
  strcat(msg,REASSIGN_MESG2);
  STATEMENT_ERROR(statement,msg);
  ascfree(msg);
}

/**
	returns 1 if ok, 0 if unhappy.
	for any given statement, once unhappy = always unhappy.
*/
static
int AssignStructuralValue(struct Instance *inst,
                 struct value_t value,
                 struct Statement *statement)
{
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
            STATEMENT_ERROR(statement, "Dimensionally inconsistent assignment");
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
            STATEMENT_ERROR(statement, "Dimensionally inconsistent assignment");
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

/**
	Execute structural and dimensional assignments.
	This is called by execute statements and exec for statements.
	Assignments to variable types are ignored.
	Variable defaults expressions are done in executedefaults.
	rhs expressions must yield constant value_t.
	Incorrect statements will be marked context_WRONG where possible.
*/
static
int ExecuteCASGN(struct Instance *work, struct Statement *statement)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  unsigned long c,len;
  struct value_t value;
  enum find_errors err;
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
    switch(err){
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
static
int NameContainsName(CONST struct Name *n,CONST struct Name *sub)
{
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
    return 0; /* should never happen */
  }
  for (c=1, len = gl_length(nl); c <= len; c++) {
    if (CompareNames((struct Name *)gl_fetch(nl,c),sub)==0) {
      gl_destroy(nl);
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
static
int ArrayCheckNameList(struct Instance *inst,
                       struct Statement *statement,
                       struct gl_list_t *nl,
                       CONST struct Name *arrsetname)
{
  unsigned long c,len,i,ilen;
  struct Instance *fi;
  CONST struct Name *n;
  struct gl_list_t *il;
  symchar *name;
  enum find_errors err;

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
static
int FailsCompoundArrayCheck(struct Instance *inst,
                            CONST struct Name *name,
                            struct Statement *statement,
                            CONST struct Name *arrsetname)
{
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
static
int CheckALIASES(struct Instance *inst, struct Statement *stat)
{
  CONST struct VariableList *vlist;
  int cu;
  struct gl_list_t *rhslist;
  CONST struct Name *name;
  enum find_errors ferr;

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
  rhslist = FindInstances(inst,name,&ferr);
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
static
int CheckARR(struct Instance *inst, struct Statement *stat)
{
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
  if (cu != 0) {
    return 0;
  }
  /* check ALIASES portion lhs list */
  vlist = ArrayStatAvlNames(stat);
  while (vlist != NULL){
    cu = ContainsUnknownArrayIndex(inst,
                                   stat,
                                   NamePointer(vlist),
                                   1,
                                   NamePointer(ArrayStatSetName(stat)));
    if (cu != 0) {
      return 0;
    }
    vlist = NextVariableNode(vlist);
  }
  /* check ALIASES portion rhs (list of instances collecting to an array) */
  if (CheckVarList(inst,stat)==0) {
    return 0;
  }
  /* check IS_A WITH_VALUE list */
  if (ArrayStatSetValues(stat)!=NULL) {
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
static
int CheckISA(struct Instance *inst, struct Statement *stat)
{
  CONST struct VariableList *vlist;
  int cu;
  unsigned int searchfor;
  if (StatWrong(stat)) return 1; /* if we'll never execute it, it's ok */
  searchfor = ( StatInFOR(stat)!=0 ||
                GetStatNeedsArgs(stat) > 0 ||
                StatModelParameter(stat)!=0 );
  vlist = GetStatVarList(stat);
  while (vlist != NULL){
    cu =
      ContainsUnknownArrayIndex(inst,stat,NamePointer(vlist),searchfor,NULL);
    if (cu) {
      return 0;
    }
    vlist = NextVariableNode(vlist);
  }
  return 1;
}

/**
	checks that all the names in a varlist exist as instances.
	returns 1 if TRUE, 0 if not.
*/
static
int CheckVarList(struct Instance *inst, struct Statement *statement)
{
  enum find_errors err;
  int instances;
  instances = VerifyInsts(inst,GetStatVarList(statement),&err);
  if (instances){
    return 1;
  }else{
    switch(err){
    case impossible_instance: return 1;
    default: return 0;
    }
  }
}

static
int CheckIRT(struct Instance *inst, struct Statement *statement)
{
  if (FindType(GetStatType(statement))==NULL) return 1;
  return CheckVarList(inst,statement);
}

static
int CheckATS(struct Instance *inst, struct Statement *statement)
{
  return CheckVarList(inst,statement);
}

static
int CheckAA(struct Instance *inst, struct Statement *statement)
{
  return CheckVarList(inst,statement);
}

/**
	Checks that the lhs of an assignment statement expands into
	a complete set of instances.
	Not check that the first of those instances is type compatible with
	the value being assigned.
*/
static
int CheckCASGN(struct Instance *inst, struct Statement *statement)
{
  struct gl_list_t *instances;
  struct value_t value;
  enum find_errors err;
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
    return 1;			/* everything is okay */
  }else{
    switch(err){
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
  enum find_errors err;
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
    switch(err){
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
static
int CheckRelName(struct Instance *work, struct Name *name)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  enum find_errors ferr;
  instances = FindInstances(work,name,&ferr);
  if (instances==NULL){
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
static
int CheckREL(struct Instance *inst, struct Statement *statement)
{
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
static
int CheckEXT(struct Instance *inst, struct Statement *statement)
{

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
static
int CheckLogRelName(struct Instance *work, struct Name *name)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  enum find_errors ferr;
  instances = FindInstances(work,name,&ferr);
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
static
int CheckLOGREL(struct Instance *inst, struct Statement *statement)
{
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
static
int CheckArrayRelMod(struct Instance *child)
{
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
static
int CheckRelModName(struct Instance *work, struct Name *name)
{
  struct gl_list_t *instances;
  struct Instance *inst, *child;
  enum find_errors ferr;
  unsigned long len,c;
  instances = FindInstances(work,name,&ferr);
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
static
int CheckFNAME(struct Instance *inst, struct Statement *statement){
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
static
int Pass3CheckCondStatements(struct Instance *inst,
                             struct Statement *statement)
{
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
    case CALL:
    case EXT:
    case ASGN:
    case CASGN:
    case COND:
    case WHEN:
    case FNAME:
    case SELECT:
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
static
int Pass3CheckCOND(struct Instance *inst, struct Statement *statement)
{
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
static
int Pass2CheckCondStatements(struct Instance *inst,
                             struct Statement *statement)
{
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
    case CALL:
    case ASGN:
    case CASGN:
    case COND:
    case WHEN:
    case FNAME:
    case SELECT:
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
static
int Pass2CheckCOND(struct Instance *inst, struct Statement *statement)
{
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
static
int CheckWhenName(struct Instance *work, struct Name *name)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  enum find_errors ferr;
  instances = FindInstances(work,name,&ferr);
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
static
int CompListInArray(unsigned long numvar, int *p1, int *p2)
{
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
static
int CheckWhenSetNode(struct Instance *ref, CONST struct Expr *expr,
                     int *p2)
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
static
int CheckWhenVariableNode(struct Instance *ref,
                          CONST struct Name *name,
                          int *p1)
{
  struct gl_list_t *instances;
  struct Instance *inst;
  enum find_errors err;
  symchar *str;
  struct for_var_t *fvp;
  str = SimpleNameIdPtr(name);
  if( str!=NULL &&
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
    switch(err){
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
    case FOR:
      return Pass4RealCheckFOR(inst,statement);
    case ALIASES:
    case ARR:
    case ISA:
    case IRT:
    case ATS:
    case AA:
    case REL:
    case LOGREL:
    case EXT:
    case CALL:
    case ASGN:
    case SELECT:
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
  enum find_errors err;
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
    switch(err){
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
  case CASGN:
  case ASGN:
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
  case CASGN:
  case ASGN:
  case WHEN:
  case SELECT:
  case FNAME:
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
  case CASGN:
  case ASGN:
  case WHEN:
  case SELECT:
  case FNAME:
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
  enum find_errors err;
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
  enum find_errors err;
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
  struct Name *name;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case WHEN:
      name = WhenStatName(statement);
      MakeWhenReference(inst,child,name,listref);
      break;
    case FNAME:
      name = FnameStat(statement);
      MakeWhenReference(inst,child,name,listref);
      break;
    case FOR:
      MakeWhenCaseReferencesFOR(inst,child,statement,listref);
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
  struct Name *name;
  unsigned long c,len;
  struct gl_list_t *list;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    statement = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(statement)){
    case WHEN:
      name = WhenStatName(statement);
      MakeWhenReference(inst,child,name,listref);
      break;
    case FNAME:
      name = FnameStat(statement);
      MakeWhenReference(inst,child,name,listref);
      break;
    case FOR:
      MakeRealWhenCaseReferencesFOR(inst,child,statement,listref);
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
    case FOR:
      return_value = 1;
      Pass4ExecuteFOR(inst,statement);
      break;
    default:
      WSEM(stderr,statement,
                      "Inappropriate statement type in WHEN Statement");
      ASC_PANIC("Inappropriate statement type in WHEN Statement");
    }
    asc_assert(return_value);
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
  struct Set *set;

  listref = gl_create(AVG_REF);

  set = WhenSetList(w1);
  cur_case = CreateCase(CopySetByReference(set),NULL);
  sl = WhenStatementList(w1);
  ExecuteWhenStatements(inst,sl);
  MakeWhenCaseReferences(inst,child,sl,listref);
  SetCaseReferences(cur_case,listref);
  return cur_case;
}

/**
	Call the Creation of a WHEN instance. This function is also in charge
	of filling the gl_list of conditional variables and the gl_list of
	CASEs contained in the WHEN instance
*/
static
void RealExecuteWHEN(struct Instance *inst, struct Statement *statement)
{
  struct VariableList *vlist;
  struct WhenList *w1;
  struct Instance *child;
  struct Name *wname;
  struct Case *cur_case;
  enum find_errors ferr;
  struct gl_list_t *instances;
  struct gl_list_t *whenvars;
  struct gl_list_t *whencases;

  wname = WhenStatName(statement);
  instances = FindInstances(inst,wname,&ferr);
  if (instances==NULL) {
    /*    if (ferr == unmade_instance) { */
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
    case FOR:
      return_value = ExecuteUnSelectedForStatements(inst,
                                                    ForStatStmts(statement));
      break;
    default:
      WSEM(stderr,statement,
                      "Inappropriate statement type in WHEN Statement");
      ASC_PANIC("Inappropriate statement type in WHEN Statement");
    }
    asc_assert(return_value);
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
  enum find_errors ferr;
  struct gl_list_t *instances;
  struct TypeDescription *def;

  wname = WhenStatName(statement);
  instances = FindInstances(inst,wname,&ferr);
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
      case FOR:
        return_value = Pass1ExecuteFOR(inst,statement);
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
      case CALL:
      case CASGN:
      case ASGN:
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
  enum find_errors err;

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
  struct BitList *blist;

  blist = InstanceBitList(inst);
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
    case REF:
    case ASGN:
    case REL:
    case CALL:
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
    case CALL:
    case REF:
    case ASGN: /* ignore'm */
    case CASGN:
    case LOGREL:
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
    asc_assert(return_value);
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
  int return_value;
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
      case CALL:
      case CASGN:
      case ASGN:
        return_value = 1;
        break;
      case FNAME:
        if (g_iteration>=MAXNUMBER) {
          STATEMENT_ERROR(statement,
              "FNAME not allowed inside a SELECT Statement");
        }
        return_value = 1; /*ignore it */
        break;
      case ALIASES:
        return_value = ExecuteUnSelectedALIASES(inst,statement);
        break;
      case ISA:
        return_value = ExecuteUnSelectedISA(inst,statement);
        break;
      case FOR:
        return_value =  ExecuteUnSelectedForStatements(inst,
                                 ForStatStmts(statement));
        break;
      case REL:
      case EXT:
      case LOGREL:
        return_value = ExecuteUnSelectedEQN(inst,statement);
        break;
      case WHEN:
        return_value = ExecuteUnSelectedWHEN(inst,statement);
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
int Pass4ExecuteStatement(struct Instance *inst,struct Statement *statement)
{
  switch(StatementType(statement)){ /* should be a WHEN statement */
  case WHEN:
    return ExecuteWHEN(inst,statement);
  case FOR:
    return Pass4ExecuteFOR(inst,statement);
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
  case WHEN:
    return 1; /* assumed done  */
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
  case LOGREL:
  case WHEN:
    return 1; /* assumed done  */
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
    return 1;/* automatically assume done */
  case WHEN:
    return 1; /* automatically assume done */
  case FNAME:
    STATEMENT_ERROR(statement,"FNAME are allowed only inside a WHEN statement");
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
  struct gl_list_t *statements;
  CONST struct StatementList *stats;
  def = InstanceTypeDesc(work);
  stats = GetStatementList(def);
  statements = GetList(stats);
  for(c=FirstNonZeroBit(blist);c<BLength(blist);c++){
    if (ReadBit(blist,c)){
      if ( Pass4ExecuteStatement(work,
           (struct Statement *)gl_fetch(statements,c+1)) ) {
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
  struct gl_list_t *statements;
  CONST struct StatementList *stats;
  def = InstanceTypeDesc(work);
  stats = GetStatementList(def);
  statements = GetList(stats);
  for(c=FirstNonZeroBit(blist);c<BLength(blist);c++){
    if (ReadBit(blist,c)){
      if ( Pass3ExecuteStatement(work,
           (struct Statement *)gl_fetch(statements,c+1)) ) {
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
  struct gl_list_t *statements;
  CONST struct StatementList *stats;
  def = InstanceTypeDesc(work);
  stats = GetStatementList(def);
  statements = GetList(stats);
  for(c=FirstNonZeroBit(blist);c<BLength(blist);c++){
    if (ReadBit(blist,c)){
      if ( Pass2ExecuteStatement(work,
           (struct Statement *)gl_fetch(statements,c+1)) ) {
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
  struct gl_list_t *statements;
  CONST struct StatementList *stats;
  struct Statement *stat;

  def = InstanceTypeDesc(work);
  stats = GetStatementList(def);
  statements = GetList(stats);
  c=FirstNonZeroBit(blist);
  while(c<BLength(blist)) {
    if (ReadBit(blist,c)){
      stat = (struct Statement *)gl_fetch(statements,c+1);
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
  clock_t start,classt;
#endif
  /* CONSOLE_DEBUG("..."); */

  /* pending will have at least one instance, or quick return. */
  asc_assert(PASS2MAXNUMBER==1);

  if (NumberPending() > 0) {
#if TIMECOMPILER
    start = clock();
#endif
    atl = Asc_DeriveAnonList(result);
#if TIMECOMPILER
    classt = clock();
    CONSOLE_DEBUG("Classification: %lu (for relation sharing)",
            (unsigned long)(classt-start));
    start = clock();
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
      classt = clock();
      CONSOLE_DEBUG("Making tokens: %lu (for relations)",
            (unsigned long)(classt-start));
      start = clock();
#endif
      BinTokensCreate(result,BT_C);
#if TIMECOMPILER
      classt = clock();
      CONSOLE_DEBUG("build/link: %lu (for bintokens)",
              (unsigned long)(classt-start));
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

static
struct gl_list_t *GetInstanceStatementList(struct Instance *i)
{
  struct TypeDescription *def;
  CONST struct StatementList *slist;
  def = InstanceTypeDesc(i);
  if (def==NULL) return NULL;
  slist = GetStatementList(def);
  if (slist==NULL) return NULL;
  return GetList(slist);
}

/* run the given default statements of i */
static void ExecuteDefault(struct Instance *i, struct Statement *stat,
                                unsigned long int *depth)
{
  struct gl_list_t *lvals;
  register unsigned long c,length;
  register struct Instance *ptr;
  struct value_t value;
  enum find_errors err;
  if ( (lvals = FindInstances(i,DefaultStatVar(stat),&err)) != NULL ){
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
  register unsigned long c,length;
  register struct Statement *stat;

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
      struct gl_list_t *statements = NULL;
      enum stat_t st;
      int changed;

      changed=0;
      if (BLength(blist)) {
        statements = GetList(GetStatementList(InstanceTypeDesc(inst)));
      }
      for(c=0;c<BLength(blist);c++){
        stat = (struct Statement *)gl_fetch(statements,c+1);
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
      struct gl_list_t *statements = NULL;
      enum stat_t st;
      int changed;

      changed=0;
      if (BLength(blist)) {
        statements = GetList(GetStatementList(InstanceTypeDesc(inst)));
      }
      for(c=0;c<BLength(blist);c++){
        stat = (struct Statement *)gl_fetch(statements,c+1);
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
  /* CONSOLE_DEBUG("starting..."); */
  /* do we need a ForTable on the stack here? don't think so. np2ppi does it */
  if (result!=NULL) {
    /* CONSOLE_DEBUG("result!=NULL..."); */
    /* pass2 pendings already set by visit */
    if (ANONFORCE || g_use_copyanon != 0) {
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
  /* CONSOLE_DEBUG("...done"); */
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
      struct gl_list_t *statements = NULL;
      enum stat_t st;
      int changed;

      changed=0;
      if (BLength(blist)) {
        statements = GetList(GetStatementList(InstanceTypeDesc(inst)));
      }
      for(c=0;c<BLength(blist);c++){
        stat = (struct Statement *)gl_fetch(statements,c+1);
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
  struct for_table_t *SavedForTable;
  SavedForTable = GetEvaluationForTable();
  SetEvaluationForTable(CreateForTable());

  if (def != NULL && oldresult != NULL) {
    Asc_Panic(2, "Pass1InstantiateModel",
              "Pass1InstantiateModel called with both type and instance.");
  }
  if (def!=NULL) { /* usual case */
    result = ShortCutMakeUniversalInstance(def);
    if (result==NULL) {
      result = CreateModelInstance(def); /*need to account for absorbed here.*/
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
    if (NumberPending()!=0) {
      *pcount = NumberPending();
      FPRINTF(ASCERR,
        "There are %lu unexecuted Phase 1 statements in the instance.\n",
        *pcount);
        if (g_compiler_warnings < 2 && *pcount >10L) {
          FPRINTF(ASCWAR,"More than 10 pending statements and warning %s",
            "level too low to allow printing.\n");
        }else{
          FPRINTF(ASCWAR,"---- Pass 1 pending: -------------\n");
          if (g_compiler_warnings > 1) {
            CheckInstanceLevel(ASCWAR,result,1);
          }else{
            FPRINTF(ASCWAR,"(Total object check suppressed.)\n");
          }
          FPRINTF(ASCWAR,"---- End pass 1 pending-----------\n");
        }
        /* could instead start an error pool data structure with
        a review protocol in place post instantiation. */
    }
    ClearList();
  }
  DestroyForTable(GetEvaluationForTable());
  SetEvaluationForTable(SavedForTable);
  return result;
}

/**
	we have to introduce a new head to instantiatemodel to manage
	the phases.
	5 phases: model creation, relation creation,
	logical relation creation, when creation,
	defaulting.
	BAA
	each pass is responsible for clearing the pending list it leaves.
*/
static
struct Instance *NewInstantiateModel(struct TypeDescription *def)
{
  struct Instance *result;
  unsigned long pass1pendings,pass2pendings,pass3pendings,pass4pendings;
#if TIMECOMPILER
  clock_t start, phase1t,phase2t,phase3t,phase4t,phase5t;
#endif

  pass1pendings = 0L;
  pass2pendings = 0L;
  pass3pendings = 0L;
  pass4pendings = 0L;
#if TIMECOMPILER
  start = clock();
#endif
  result = Pass1InstantiateModel(def,&pass1pendings,NULL);
#if TIMECOMPILER
  phase1t = clock();
  CONSOLE_DEBUG("Phase 1 models = %lu",(unsigned long)phase1t-start);
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
    /* result will not move as currently implemented */
#ifdef DEBUG_RELS
    debug_rels_work = NULL;
#endif
  }else{
    return result;
  }
#if TIMECOMPILER
  phase2t = clock();
  CONSOLE_DEBUG("Phase 2 relations = %lu",(unsigned long)(phase2t-phase1t));
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
   /* result will not move as currently implemented */
  }else{
    return result;
  }
#if TIMECOMPILER
  phase3t = clock();
  CONSOLE_DEBUG("Phase 3 logicals = %lu",(unsigned long)(phase3t-phase2t));
#endif
  if (result!=NULL) {
    /* now set the bits for when statements and add pending models */
    SilentVisitInstanceTree(result,Pass4SetWhenBits,0,0);
    /* note, the order of the visit might be better 1 than 0. don't know */
    /* at present order 0, so we do lower models before those near root */
    result = Pass4InstantiateModel(result,&pass4pendings);
   /* result will not move as currently implemented */
  }else{
    return result;
  }
#if TIMECOMPILER
  phase4t = clock();
  CONSOLE_DEBUG("Phase 4 when-case = %lu",(unsigned long)(phase4t-phase3t));
#endif
  if (result!=NULL) {
    if (!pass1pendings && !pass2pendings && !pass3pendings && !pass4pendings){
      DefaultInstanceTree(result);
    }else{
      ERROR_REPORTER_NOLINE(ASC_USER_WARNING,"There are unexecuted statements "
		"in the instance.\nDefault assignments not executed.");
    }
  }
#if TIMECOMPILER
  phase5t = clock();
  CONSOLE_DEBUG("Phase 5 defaults = %lu",(unsigned long)(phase5t-phase4t));
  if (pass1pendings || pass2pendings || pass3pendings || pass4pendings) {
#ifdef __WIN32__
    char *timeunit = "milliseconds";
#else
    char *timeunit = "microseconds";
#endif
    CONSOLE_DEBUG("Compilation times (%s):\n",timeunit);
    CONSOLE_DEBUG("Phase 1 models \t\t%lu\n",
            (unsigned long)(phase1t-start));
    CONSOLE_DEBUG("Phase 2 relations \t\t%lu\n",
            (unsigned long)(phase2t-phase1t));
    CONSOLE_DEBUG("Phase 3 logical \t\t%lu\n",
            (unsigned long)(phase3t-phase2t));
    CONSOLE_DEBUG("Phase 4 when-case \t\t%lu\n",
            (unsigned long)(phase4t-phase3t));
    CONSOLE_DEBUG("Phase 5 defaults\t\t%lu\n",
            (unsigned long)(phase5t-phase4t));
  }
  CONSOLE_DEBUG("Total = %lu",(unsigned long)(phase5t-start));
#if 0 /* deep performance tuning */
  gl_reportrecycler(ASCERR);
#endif
#endif
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
  enum Proc_enum runstat;
  struct Name *name;
  if (InstanceKind(root) == MODEL_INST && defmethod != NULL) {
    name = CreateIdName(defmethod);
    runstat = Initialize(root,name,(char *)SCP(simname),ASCERR,
                         (WP_BTUIFSTOP|WP_STOPONERR),NULL,NULL);
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
  register struct TypeDescription *desc;
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
  unsigned long pass1pendings,pass2pendings,pass3pendings,pass4pendings;
#if TIMECOMPILER
  time_t start, phase1t,phase2t,phase3t,phase4t,phase5t;
#endif
  ++g_compiler_counter;/*instance tree will change:increment compiler counter*/
  asc_assert(i!=NULL);
  if (i==NULL || !IsCompoundInstance(i)) return;
  /* can't reinstantiate simple objects, missing objects */

  pass1pendings = 0L;
  pass2pendings = 0L;
  pass3pendings = 0L;
  pass4pendings = 0L;
#if TIMECOMPILER
  start = clock();
#endif
  result = Pass1InstantiateModel(NULL,&pass1pendings,i);
#if TIMECOMPILER
  phase1t = clock();
#endif
  if (result!=NULL) {
    SilentVisitInstanceTree(result,Pass2SetRelationBits,0,0);
    result = Pass2InstantiateModel(result,&pass2pendings);
  }else{
    ASC_PANIC("Reinstantiation phase 2 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase2t = clock();
#endif
  if (result!=NULL) {
    SilentVisitInstanceTree(result,Pass3SetLogRelBits,0,0);
    result = Pass3InstantiateModel(result,&pass3pendings);
  }else{
    ASC_PANIC("Reinstantiation phase 3 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase3t = clock();
#endif
  if (result!=NULL) {
    SilentVisitInstanceTree(result,Pass4SetWhenBits,0,0);
    result = Pass4InstantiateModel(result,&pass4pendings);
  }else{
    ASC_PANIC("Reinstantiation phase 4 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase4t = clock();
#endif
  if (result!=NULL) {
    if (!pass1pendings && !pass2pendings && !pass3pendings && !pass4pendings){
      DefaultInstanceTree(result);
    }else{
      FPRINTF(ASCERR,"There are unexecuted statements in the instance.\n");
      FPRINTF(ASCERR,"Default assignments not executed.\n");
    }
  }else{
    ASC_PANIC("Reinstantiation phase 5 went insane. Bye!\n");
  }
#if TIMECOMPILER
  phase5t = clock();
  CONSOLE_DEBUG("Reinstantiation times (microseconds):\n");
  CONSOLE_DEBUG("Phase 1 models \t\t%lu\n",(unsigned long)(phase1t-start));
  CONSOLE_DEBUG("Phase 2 relations \t\t%lu\n",
          (unsigned long)(phase2t-phase1t));
  CONSOLE_DEBUG(
          "Phase 3 logicals \t\t%lu\n",(unsigned long)(phase3t-phase2t));
  CONSOLE_DEBUG("Phase 4 when-case \t\t%lu\n",
          (unsigned long)(phase4t-phase3t));
  CONSOLE_DEBUG(
          "Phase 5 defaults \t\t%lu\n",(unsigned long)(phase5t-phase4t));
  CONSOLE_DEBUG("Total\t\t%lu\n",(unsigned long)(phase5t-start));
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
  enum find_errors ferr;
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
        instances = FindInstances(target,name,&ferr);
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

