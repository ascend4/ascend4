/**< 
 *  Statement list module
 *  by Tom Epperly
 *  August 8, 1989
 *  Version: $Revision: 1.26 $
 *  Version control file: $RCSfile: statement.h,v $
 *  Date last modified: $Date: 1998/04/21 23:49:51 $
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

#ifndef __STATEMENT_H_SEEN__
#define __STATEMENT_H_SEEN__


/**< 
 *  When #including statement.h, make sure these files are #included first:
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "stattypes.h"
 */


extern void AddContext(struct StatementList *,unsigned int);
/**< 
 *  AddContext(slist,c)
 *
 *  Adds the given context flags, c, to the statements in the list
 *  using the | operator, so that existing context
 *  bits are preserved (other than context_MODEL).
 *  Handles NULL slist gracefully.
 *
 *  This function is recursive. Those statements which contain another
 *  statement list will have those lists visited with this function,
 *  adding the flags we have here. Context is a cumulative notion,
 *  after all. Careless use will make this a tad expensive for
 *  deeply nested code.
 *
 *  All the create functions below return a statement (if successful)
 *  which will have a context value of context_MODEL (0).
 *
 *  VRR. Added the context flags for statements inside WHEN, SELECT and
 *  CONDITIONAL. 2/3/97 and SWITCH 29/3/97
 */


extern unsigned int SlistHasWhat(struct StatementList *);
/**< 
 *  unsigned int SlistHasWhat(slist);
 *  Returns an unsigned with the low bits set according to whether or
 *  not there is a statement of the indicated class in the slist given.
 *  The value is determined by examining the list, not by looking up
 *  some attribute.
 *  The return type is valid as a contains field.
 */

extern struct Statement *CreateALIASES(struct VariableList *,
                                       struct Name *);
/**< 
 *  struct Statement *CreateALIASES(vl,n,nsize)
 *  struct VariableList *vl;
 *  struct Name *n;
 *  Parameters
 *  vl	variable list
 *  n	instance name
 *  Create an ALIASES statement.
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateARR(struct VariableList *,
                                   struct VariableList *,
                                   struct VariableList *,
                                   int,
                                   struct Set *);
/**< 
 *  struct Statement *CreateARR(aname,il,setname,intset,setvals)
 *  struct VariableList *avlname;
 *  struct VariableList *il;
 *  struct VariableList *setvlname;
 *  int intset;
 *  struct Set *setvals;
 *  Parameters
 *  avlname	lhs name (in varlist form) of array being made
 *  il	rhs varlist of things to alias
 *  setname name (in varlist form) of set instance to create which indexes
 *         avlname.
 *  intset  symbol (0) or integer (1) set instance
 *  setvals set expression for subscripts. must evaluate to match
 *  count of items il evaluates to if setvals != NULL.
 *  Create a compound ALIASES-IS_A statement.
 *  VariableListLength(avlname) and VariableListLength(setname) should
 *  be 1.
 *  setvals may be NULL, rest may must be supplied.
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateISA(struct VariableList *,symchar *,
       struct Set *,symchar *);
/**< 
 *  struct Statement *CreateISA(vl,t,ta,st)
 *  struct VariableList *vl;
 *  struct Set *ta;
 *  const char *t,*st;
 *  Parameters
 *  vl	variable list
 *  t	instance type
 *  ta	arguments for type t
 *  st	set type
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateWILLBE(struct VariableList *, symchar *,
                                      struct Set *, symchar *,
                                      struct Expr *);
/**< 
 *  struct Statement *CreateWILLBE(vl,t,ta,st,cv)
 *  struct VariableList *vl;
 *  struct Set *ta;
 *  const char *t,*st;
 *  struct Expr *cv;
 *  Parameters
 *  vl	variable list
 *  t	instance type
 *  ta	arguments for type t
 *  st	set type (possibly)
 *  cv	WILL_BE value specification.
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateIRT(struct VariableList *,symchar *,
                                   struct Set *);
/**< 
 *  struct Statement *CreateIRT(vl,t)
 *  struct VariableList *vl;
 *  const char *t;
 *  struct Set *ta;
 *  Parameters
 *  vl	variable list
 *  t	instance type
 *  ta	arguments for type t
 *  Comments:
 *  Create an IS_REFINED_TO statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateAA(struct VariableList *);
/**< 
 *  struct Statement *CreateAA(vl)
 *  struct VariableList *vl;
 *  Parameters
 *  vl	variable list
 *  Comments:
 *  Create an ARE_ALIKE statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateATS(struct VariableList *);
/**< 
 *  struct Statement *CreateATS(vl)
 *  struct VariableList *vl;
 *  Parameters
 *  vl	variable list
 *  Comments:
 *  Create an ARE_THE_SAME statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateWBTS(struct VariableList *);
/**< 
 *  struct Statement *CreateWBTS(vl)
 *  struct VariableList *vl;
 *  Parameters
 *  vl	variable list
 *  Comments:
 *  Create an WILL_BE_THE_SAME statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateWNBTS(struct VariableList *);
/**< 
 *  struct Statement *CreateWNBTS(vl)
 *  struct VariableList *vl;
 *  Parameters
 *  vl	variable list
 *  Comments:
 *  Create an WILL_NOT_BE_THE_SAME statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateFOR(symchar *,struct Expr *,
       struct StatementList *,
       enum ForOrder,enum ForKind);
/**< 
 *  struct Statement *CreateFOR(index,expr,stmts,order,kind)
 *  struct Expr *expr;
 *  const char *index;
 *  struct StatementList *stmts;
 *  enum ForOrder order;
 *  enum ForKind kind;
 *
 *  Comments:
 *  Create a FOR statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *  The statement list is checked for presence of relations,
 *  default assignments, and structural assignments and marked
 *  as needed.
 *  The context bit context_FOR will be set on the statements
 *  in the list stmts that have a context field.
 */

extern struct Statement *CreateREL(struct Name *,struct Expr *);
/**< 
 *  struct Statement *CreateREL(n,relation);
 *  struct Name *n;
 *  struct Expr *relation;
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */


extern struct Statement *CreateLOGREL(struct Name *,struct Expr *);
/**< 
 *  struct Statement *CreateLOGREL(n,logrel);
 *  struct Name *n;
 *  struct Expr *logrel;
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */


extern struct Statement *CreateEXTERN(int mode, struct Name *, CONST char *,
          struct VariableList *,struct Name *,
          struct Name *);
/**< 
 *  struct Statement *CreateEXTERN(mode,n,funcname,vl);
 *  int mode; -- 0 = procedural;
 *           -- 1 = glassbox declarative;
 *  -- 2 = blackbox declarative
 *  struct Name *n;
 *  char *funcname;
 *  struct VariableList *vl;
 *  struct Name *data; -- additional user data or NULL;
 *  struct Name *scope; -- scope at which external relations are to
 *                        be embedded. Applicable only to glassboxes
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateFlow(enum FlowControl, CONST char *);
/**< 
 *  struct Statement *CreateFlow(fc,msgtext);
 *  Create a BREAK, CONTINUE, FALL_THROUGH, RETURN, STOP as
 *  indicated by fc. If msgtext is not NULL, it is
 *  converted to a bracechar and stored as the message.
 */

extern
struct Statement *CreateREF(struct VariableList *,symchar *,
                            symchar *, int);
/**< 
 *  struct Statement *CreateREF(vl,ref_name,st,mode)
 *  struct VariableList *vl;
 *  const char *ref_name;
 *  const char *settype;
 *  int mode; -- 0 = copy semantics; (_IS_)
 *           -- 1 = pure reference semantics; (_REFERS_)
 *  Creates a new REFERENCE type statement. ref_name is the name of
 *  the type or prototype being referred to.
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *  NOT IMPLEMENTED.
 */

extern void SetRelationName(struct Statement *, struct Name *);
/**< 
 *  void SetRelationName(stat,n)
 *  struct Statement *stat;
 *  struct Name *n;
 *  This procedure provides a mechanism for unnamed relations to be named.
 *  It should only be called on relation statements that are unnamed.  It
 *  will name the relation with the name provided.
 */


extern void SetLogicalRelName(struct Statement *, struct Name *);
/**< 
 *  void SetLogicalRelName(stat,n)
 *  struct Statement *stat;
 *  struct Name *n;
 *  This procedure provides a mechanism for unnamed relations to be named.
 *  It should only be called on logcial relation statements that are unnamed.
 *  It will name the logical relation with the name provided.
 */


extern struct Statement *CreateRUN(struct Name *,struct Name *);
/**< 
 *  struct Statement *CreateRUN(n,type)
 *  struct Name *n;
 *  struct Name *type;
 *  Create a run statement with procedure name n, and/or class type.
 *  If type is non NULL, then so called 'class access is to be used to
 *  access the procedure. This is equivalent to the "::" in some object
 *  oriented languages.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateCALL(symchar *,struct Set *);
/**< 
 *  struct Statement *CreateCALL(n,argList)
 *  symchar *n;
 *  struct Set * argList;
 *  Create a CALL statement with name n and arguments argList.
 *  argList may be NULL.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateIF(struct Expr *,struct StatementList *,
      struct StatementList *);
/**< 
 *  struct Statement *CreateIF(ex,ifblock,elseblock)
 *  struct Expr *ex;
 *  struct StatementList *ifblock,*elseblock;
 *  Create an IF-THEN-ELSE structure.  If the else block is omitted,
 *  pass in NULL for elseblock.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateWhile(struct Expr *, struct StatementList *);
/**< 
 *  struct Statement *CreateWhile(test,body)
 *  struct Expr *test;
 *  struct StatementList *body;
 *  Create a WHILE-DO structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateWHEN(struct Name *, struct VariableList *,
                                    struct WhenList *);
/**< 
 *  struct Statement *CreateWHEN(wname,vlist,wl)
 *  struct Name *wname;
 *  struct VariableList *vlist;
 *  struct WhenList *wl;
 *  Crease a WHEN statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateFNAME(struct Name *);
/**< 
 *  struct Statement *CreateFNAME(name)
 *  Crease a FNAME statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *  Name is the name of a model or a relation inside a WHEN statement
 */

extern struct Statement *CreateSWITCH(struct VariableList *,
                                      struct SwitchList *);
/**< 
 *  struct Statement *CreateSWITCH(v,sw)
 *  struct VariableList *v;
 *  struct SelectList *sw;
 *  Crease a SWITCH statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */


extern struct Statement *CreateCOND(struct StatementList *);
/**< 
 *  struct Statement *CreateCOND(stat)
 *  struct StatementList *stmts;
 *  Create a CONDITIONAL statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */


extern struct Statement *CreateSELECT(struct VariableList *,
                                      struct SelectList *);
/**< 
 *  struct Statement *CreateSELECT(v,sel)
 *  struct VariableList *v;
 *  struct SelectList *sel;
 *  Crease a SELECT statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateASSIGN(struct Name *, struct Expr *);
/**< 
 *  struct Statement *CreateASSIGN(n,rhs);
 *  struct Name *n;
 *  struct Expr *rhs;
 *  Comments:
 *  Create a default assignment statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateCASSIGN(struct Name *, struct Expr *);
/**< 
 *  struct Statement *CreateCASSIGN(n,rhs);
 *  struct Name *n;
 *  struct Expr *rhs;
 *  Comments:
 *  Create a structural assignment statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

#ifdef NDEBUG
#define StatementType(s) ((s)->t)
#else
#define StatementType(s) StatementTypeF(s)
#endif
extern enum stat_t StatementTypeF(CONST struct Statement *);
/**< 
 *  macro StatementType(s)
 *  enum stat_t StatementTypeF(s)
 *  const struct Statement *s;
 *  Return the statement type of s.
 */

#ifdef NDEBUG
#define StatementModule(s) ((s)->mod)
#else
#define StatementModule(s) StatementModuleF(s)
#endif
extern struct module_t *StatementModuleF(CONST struct Statement *);
/**< 
 *  macro StatementModule(s)
 *  struct module_t *StatementModuleF(s)
 *  const struct Statement *s;
 *  Return the module that statement s was defined in.
 */

#ifdef NDEBUG
#define StatementLineNum(s) ((s)->linenum)
#else
#define StatementLineNum(s) StatementLineNumF(s)
#endif
extern unsigned long StatementLineNumF(CONST struct Statement *);
/**< 
 *  macro StatementLineNum(s)
 *  unsigned long StatementLineNumF(s)
 *  const struct Statement *s;
 *  Return the line number where statement s was defined.
 */

#ifdef NDEBUG
#define CopyStatement(s) if ((s)->ref_count<MAXREFCOUNT) (((s)->ref_count)++)
#else
#define CopyStatement(s) CopyStatementF(s)
#endif
extern struct Statement *CopyStatementF(struct Statement *);
/**< 
 *  struct Statement *CopyStatementF(s)
 *  struct Statement *s;
 *  Make a new reference to statement s.  This does not actually make a new
 *  copy in memory of the data in s: it only increases the reference count.
 *  If you need to modify the statement, you should use CopyToModify which
 *  is defined below.
 *  You don't need to worry about the count overflowing unless the user is
 *  utterly insane.
 */

extern void DestroyStatement(struct Statement *);
/**< 
 *  void DeleteStatement(s)
 *  struct Statement *s;
 *  Delete a reference to statement s.  This make not actually deallocate
 *  the memory associated with s if there are still other references to
 *  the statement.  If there aren't any, remaining references it deallocates
 *  the memory.
 */

extern struct Statement *CopyToModify(struct Statement *);
/**< 
 *  struct Statement *CopyToModify(s)
 *  struct Statement *s;
 *  This will always make a new separate copy of the statement, so that
 *  it can be modified.  The statement reference count is initialize to
 *  one.
 */

#ifndef NDEBUG
#define GetStatContext(s) ((s)->context)
#else
#define GetStatContext(s) GetStatContextF(s)
#endif
extern unsigned int GetStatContextF(CONST struct Statement *);
/**< 
 *  bits = GetStatContext(s);
 *  const struct Statement *s;
 *  Returns the context bits of the statement.
 *  If statement is of a type that doesn't have context bits,
 *  returns context_MODEL and issues warning message.
 */

#define StatInFOR(stat) (GetStatContext(stat)&context_FOR)
/**< 
 *  Returns a nonzero value (context_FOR) if the statement is inside
 *  the statement list of a FOR statement.
 */

#define StatInMETH(stat) (GetStatContext(stat)&context_METH)
/**< 
 *  Returns a nonzero value (context_METH) if the statement is inside
 *  the statement list of a method.
 */

#define StatWrong(stat) (GetStatContext(stat)&context_WRONG)
/**< 
 *  Returns a nonzero value (context_WRONG) if the statement is probably
 *  incorrect in all situations.
 */

#define StatInCOND(stat) (GetStatContext(stat)&context_COND)
/**< 
 *  Returns a nonzero value (context_COND) if the statement is inside
 *  the statement list of a CONDITIONAL statement.
 */

#define StatInWHEN(stat) (GetStatContext(stat)&context_WHEN)
/**< 
 *  Returns a nonzero value (context_WHEN) if the statement is inside
 *  the statement list of a WHEN statement.
 */

#define StatInSELECT(stat) (GetStatContext(stat)&context_SELECT)
/**< 
 *  Returns a nonzero value (context_SELECT) if the statement is inside
 *  the statement list of a SELECT statement.
 */

#define StatInIF(stat) (GetStatContext(stat)&context_IF)
/**< 
 *  Returns a nonzero value (context_IF) if the statement is inside
 *  the thenblock or elseblock of an IF statement.
 */

#define StatInSWITCH(stat) (GetStatContext(stat)&context_SWITCH)
/**< 
 *  Returns a nonzero value (context_SWITCH) if the statement is inside
 *  the statement list of a SWITCH statement.
 */

#define StatModelParameter(stat) (GetStatContext(stat)&context_MODPARAM)
/**< 
 *  Returns a nonzero value (context_MODPARAM) if the statement is
 *  in a parameterized type list. (or at least came from there).
 */

extern void SetStatContext(struct Statement *,unsigned int);
/**< 
 *  SetStatContext(s,bits)
 *  Sets the context field of a statement to the value, bits, given.
 *  E.g. to turn on the FOR context bit,
 *  This function should only be used at parse time, and probably not
 *  then -- see MarkStatContext for the proper interface.
 *  Calling it on a statement type without a context field causes
 *  an exit(2) and an error message.
 */

extern void MarkStatContext(struct Statement *,unsigned int);
/**< 
 *  MarkStatContext(s,bits)
 *  Adds the context bits given in bits to the context field of the
 *  statement given.
 *  E.g. to turn on the FOR context bit,
 *  MarkStatContext(s,context_FOR);
 *  This function should only be used at parse time.
 *  Unfortunately, until we have proof that the parser is err tight,
 *  we might also use this function at points in instantiation where
 *  we have proved that a statement is always going to be impossible.
 */

extern struct VariableList *GetStatVarList(CONST struct Statement *);
/**< 
 *  struct VariableList *GetStatVarListF(s)
 *  const struct Statement *s;
 *  Returns the variable list of a
 *  IS_A, IS_REFINED_TO, WILL_BE, WILL_BE_THE_SAME,
 *  ARE_ALIKE, ARE_THE_SAME, ALIASES or ALIASES-ISA(ARR) statement.
 *  It must be passed one of these types of statement.
 *  Other statements will return NULL or crash.
 */

/*** StateIs functions ***/

#ifdef NDEBUG
#define GetStatType(s) ((s)->v.i.type)
#else
#define GetStatType(s) GetStatTypeF(s)
#endif
extern symchar *GetStatTypeF(CONST struct Statement *);
/**< 
 *  macro GetStatType(s)
 *  symchar *GetStatTypeF(s)
 *  const struct Statement *s;
 *  Return the type field of an IS_A, WILL_BE, or IS_REFINED_TO statement.
 *  It must be passed one of these types of statement.
 *  Other statements will return NULL or crash.
 *  See also GetStatTypeDesc, type_desc.h.
 */

#ifdef NDEBUG
#define GetStatTypeArgs(s) ((s)->v.i.typeargs)
#else
#define GetStatTypeArgs(s) GetStatTypeArgsF(s)
#endif
extern CONST struct Set *GetStatTypeArgsF(CONST struct Statement *);
/**< 
 *  macro GetStatTypeArgs(s)
 *  const struct Set *GetStatTypeArgsF(s)
 *  const struct Statement *s;
 *  Return the typeargs field of an IS_A, WILL_BE or IS_REFINED_TO statement.
 *  It must be passed one of these types of statement.
 *  Proper WILLBE statements should always return NULL.
 *  Other statements will return NULL or crash.
 */

extern unsigned int GetStatNeedsArgs(CONST struct Statement *);
/**< 
 *  len = GetStatNeedsArgs(s);
 *  const struct Statement *s;
 *  unsigned int len;
 *  Returns the number of arguments required to satisfy the type of the
 *  ISA or IRT statement given. Note that this number is not necessarily
 *  the number of arguments actually in the statement as the user wrote it.
 *  The number comes from the typedescription in the statement.
 */

#ifdef NDEBUG
#define GetStatSetType(s) ((s)->v.i.settype)
#else
#define GetStatSetType(s) GetStatSetTypeF(s)
#endif
extern symchar *GetStatSetTypeF(CONST struct Statement *);
/**< 
 *  macro GetStatSetType(s)
 *  symchar *GetStatSetTypeF(s)
 *  const struct Statement *s;
 *  Return the type of set for an IS_A, WILL_BE.
 *  Often this will be NULL, which
 *  means that there is no "OF" part to the IS_A/WILL_BE.
 */

#ifdef NDEBUG
#define GetStatCheckValue(s) ((s)->v.i.checkvalue)
#else
#define GetStatCheckValue(s) GetStatCheckValueF(s)
#endif
extern CONST struct Expr *GetStatCheckValueF(CONST struct Statement *);
/**< 
 *  macro GetStatCheckValue(s)
 *  const struct Expr *GetStatCheckValueF(s)
 *  const struct Statement *s;
 *  Return the value expression for a WILLBE.  Often this will be NULL,
 *  which means that there is no WITH_VALUE part to the WILL_BE.
 */

/*** StateAlias functions ***/

#ifdef NDEBUG
#define AliasStatName(s) ((s)->v.ali.u.nptr)
#else
#define AliasStatName(s) AliasStatNameF(s)
#endif
extern CONST struct Name *AliasStatNameF(CONST struct Statement *);
/**< 
 *  macro AliasStatName(s)
 *  struct Name *AliasStatNameF(s)
 *  Return the Name RHS from a simple ALIASES statement.
 *  See stattypes.h for meaning of Name.
 */


#ifdef NDEBUG
#define ArrayStatAvlNames(s) ((s)->v.ali.u.avlname)
#else
#define ArrayStatAvlNames(s) ArrayStatAvlNamesF(s)
#endif
extern CONST struct VariableList *ArrayStatAvlNamesF(CONST struct Statement *);
/**< 
 *  macro ArrayStatAvlNames(s)
 *  CONST struct VariableList *ArrayStatAvlNamesF(s)
 *  Return the LHS (new array(s)) being defined from an ARR statement.
 *  See stattypes.h for meaning of vlist.
 *  syntax is currently:
 *
 *  AvlNames ALIASES (VarList)
 *  WHERE SetName IS_A set OF IntSet WITH_VALUE (SetValues);
 *
 *  while the standard ALIASES statement is just:
 *
 *  VarList ALIASES Name;
 */

#ifdef NDEBUG
#define ArrayStatSetName(s) ((s)->v.ali.c.setname)
#else
#define ArrayStatSetName(s) ArrayStatSetNameF(s)
#endif
extern CONST struct VariableList *ArrayStatSetNameF(CONST struct Statement *);
/**< 
 *  macro ArrayStatSetName(s)
 *  struct VariableList *ArrayStatSetNameF(s)
 *  const struct Statement *s;
 *  Return the name of set for an ARR statement.
 */

#ifdef NDEBUG
#define ArrayStatIntSet(s) ((s)->v.ali.c.intset)
#else
#define ArrayStatIntSet(s) ArrayStatIntSetF(s)
#endif
extern int ArrayStatIntSetF(CONST struct Statement *);
/**< 
 *  macro ArrayStatIntSet(s)
 *  int ArrayStatIntSetF(s)
 *  const struct Statement *s;
 *
 *  Return 0 or 1 corresponding to set being defined as symbol or integer.
 *  It must be passed an ARR statement.
 *  Other statements will return goop or crash.
 */

#ifdef NDEBUG
#define ArrayStatSetValues(s) ((s)->v.ali.c.setvals)
#else
#define ArrayStatSetValues(s) ArrayStatSetValuesF(s)
#endif
extern CONST struct Set *ArrayStatSetValuesF(CONST struct Statement *);
/**< 
 *  macro ArrayStatSetValues(s)
 *  struct Set *ArrayStatSetValuesF(s)
 *  const struct Statement *s;
 *
 *  Return the user proposed set of values to use for array names.
 *  It must be passed an ARR statement. May be empty/NULL set.
 *  (this is the list after WITH_VALUE)
 *  Other statements will return goop or crash.
 */

/*** StateFOR functions ***/

#ifdef NDEBUG
#define ForStatIndex(s) ((s)->v.f.index)
#else
#define ForStatIndex(s) ForStatIndexF(s)
#endif
extern symchar *ForStatIndexF(CONST struct Statement *);
/**< 
 *  macro ForStatIndex(s)
 *  symchar *ForStatIndexF(s)
 *  const struct Statement *s;
 *  Return the FOR index variable identifier.
 */

#ifdef NDEBUG
#define ForStatExpr(s) ((s)->v.f.e)
#else
#define ForStatExpr(s) ForStatExprF(s)
#endif
extern struct Expr *ForStatExprF(CONST struct Statement *);
/**< 
 *  macro ForStatExpr(s)
 *  struct Expr *ForStatExprF(s)
 *  const struct Statement *s;
 *  Return the expression of values that the index variable takes.
 */

#ifdef NDEBUG
#define ForStatStmts(s) ((s)->v.f.stmts)
#else
#define ForStatStmts(s) ForStatStmtsF(s)
#endif
extern struct StatementList *ForStatStmtsF(CONST struct Statement *);
/**< 
 *  macro ForStatStmts(s)
 *  struct StatementList *ForStatStmtsF(s)
 *  const struct Statement *s;
 *  Return the list of statements that are in the body of the FOR construct.
 */

#ifdef NDEBUG
#define ForLoopKind(s) ((s)->v.f.kind)
#else
#define ForLoopKind(s) ForLoopKindF(s)
#endif
extern enum ForKind ForLoopKindF(CONST struct Statement *);
/**< 
 *  macro ForLoopKind(s)
 *  enum ForOrder ForLoopKindF(s)
 *  const struct Statement *s;
 *  Return the kind of the FOR construct. 
 */

#ifdef NDEBUG
#define ForLoopOrder(s) ((s)->v.f.order)
#else
#define ForLoopOrder(s) ForLoopOrderF(s)
#endif
extern enum ForOrder ForLoopOrderF(CONST struct Statement *);
/**< 
 *  macro ForLoopOrder(s)
 *  enum ForOrder ForLoopOrderF(s)
 *  const struct Statement *s;
 *  Return the order of the FOR construct.  For declarative this should
 *  always be random.
 */

#ifdef NDEBUG
#define ForContains(s) ((s)->v.f.contains)
#define ForContainsRelations(s) ((s)->v.f.contains & contains_REL)
#define ForContainsLogRelations(s) ((s)->v.f.contains & contains_LREL)
#define ForContainsDefaults(s) ((s)->v.f.contains & contains_DEF)
#define ForContainsCAssigns(s) ((s)->v.f.contains & contains_CAS)
#define ForContainsWhen(s) ((s)->v.f.contains & contains_WHEN)
#define ForContainsAlike(s) ((s)->v.f.contains & contains_AA)
#define ForContainsAlias(s) ((s)->v.f.contains & contains_ALI)
#define ForContainsArray(s) ((s)->v.f.contains & contains_ARR)
#define ForContainsIsa(s) ((s)->v.f.contains & contains_ISA)
#define ForContainsIrt(s) ((s)->v.f.contains & contains_IRT)
#define ForContainsAts(s) ((s)->v.f.contains & contains_ATS)
#define ForContainsWbts(s) ((s)->v.f.contains & contains_WBTS)
#define ForContainsWnbts(s) ((s)->v.f.contains & contains_WNBTS)
#define ForContainsWillbe(s) ((s)->v.f.contains & contains_WILLBE)
#define ForContainsSelect(s) ((s)->v.f.contains & contains_SELECT)
#define ForContainsConditional(s) ((s)->v.f.contains & contains_COND)
#define ForContainsIllegal(s) ((s)->v.f.contains & contains_ILL)
#else
#define ForContains(s) ForContainsF(s)
#define ForContainsRelations(s) ForContainsRelationsF(s)
#define ForContainsLogRelations(s) ForContainsLogRelationsF(s)
#define ForContainsDefaults(s) ForContainsDefaultsF(s)
#define ForContainsCAssigns(s) ForContainsCAssignsF(s)
#define ForContainsWhen(s) ForContainsWhenF(s)
#define ForContainsAlike(s) ForContainsAlikeF(s)
#define ForContainsAlias(s) ForContainsAliasF(s)
#define ForContainsArray(s) ForContainsArrayF(s)
#define ForContainsIsa(s) ForContainsIsaF(s)
#define ForContainsIrt(s) ForContainsIrtF(s)
#define ForContainsAts(s) ForContainsAtsF(s)
#define ForContainsWbts(s) ForContainsWbtsF(s)
#define ForContainsWnbts(s) ForContainsWnbtsF(s)
#define ForContainsWillbe(s) ForContainsWillbeF(s)
#define ForContainsSelect(s) ForContainsSelectF(s)
#define ForContainsConditional(s) ForContainsConditionalF(s)
#define ForContainsIllegal(s) ForContainsIllegalF(s)
#endif
extern unsigned ForContainsF(CONST struct Statement *);
extern unsigned ForContainsRelationsF(CONST struct Statement *);
extern unsigned ForContainsLogRelationsF(CONST struct Statement *);
extern unsigned ForContainsDefaultsF(CONST struct Statement *);
extern unsigned ForContainsCAssignsF(CONST struct Statement *);
extern unsigned ForContainsWhenF(CONST struct Statement *);
extern unsigned ForContainsAlikeF(CONST struct Statement *);
extern unsigned ForContainsAliasF(CONST struct Statement *);
extern unsigned ForContainsArrayF(CONST struct Statement *);
extern unsigned ForContainsIsaF(CONST struct Statement *);
extern unsigned ForContainsIrtF(CONST struct Statement *);
extern unsigned ForContainsAtsF(CONST struct Statement *);
extern unsigned ForContainsWbtsF(CONST struct Statement *);
extern unsigned ForContainsWnbtsF(CONST struct Statement *);
extern unsigned ForContainsWillbeF(CONST struct Statement *);
extern unsigned ForContainsSelectF(CONST struct Statement *);
extern unsigned ForContainsConditionalF(CONST struct Statement *);
extern unsigned ForContainsIllegalF(CONST struct Statement *);
/**< 
 *  macro ForContains(s)
 *  unsigned ForContainsF(s)
 *  macro ForContainsRelations(s)
 *  unsigned ForContainsRelationsF(s)
 *  macro ForContainsLogRelations(s)
 *  unsigned ForContainsLogRelationsF(s)
 *  macro ForContainsDefaults(s)
 *  unsigned ForContainsDefaultsF(s)
 *  macro ForContainsCAssigns(s)
 *  unsigned ForContainsCAssignsF(s)
 *  macro ForContainsWhen(s)
 *  unsigned ForContainsWhenF(s)
 *  macro ForContainsIllegal(s)
 *  unsigned ForContainsIllegalF(s)
 *  etc....
 *  const struct Statement *s;
 *
 *  Respectively:
 *  Returns the entire contains field.
 *  Return true if the FOR statement given contains itself or in any of
 *  its nested statements a relation.
 *  Return true if the FOR statement given contains itself or in any of
 *  its nested statements a default statement.
 *  Return true if the FOR statement given contains itself or in any of
 *  its nested statements a constant/structural variable assignment.
 *  Return true if the FOR statement given contains itself or in any of
 *  its nested statements a WHEN statement.
 *  Likewise for:
 *  ARE_ALIKE
 *  ALIASES
 *  CREATE ARRAY
 *  IS_A
 *  IS_REFINED_TO
 *  ARE_THE_SAME
 *  WILL_BE_THE_SAME
 *  WILL_BE
 *  SELECT
 *  CONDITIONAL.
 *  Return true if the FOR statement given contains itself or in any of
 *  its nested statements an illegal instruction.
 */

/*** StateAssign functions ***/

/**< 
 *  Assign has been split into Assign (structural) and Default
 *  (variable) assignment types.
 */
#ifdef NDEBUG
#define DefaultStatVar(sptr) ((sptr)->v.asgn.nptr)
#else
#define DefaultStatVar(sptr) DefaultStatVarF(sptr)
#endif
extern struct Name *DefaultStatVarF(CONST struct Statement *);
/**< 
 *  macro DefaultStatVar(s)
 *  struct Name *DefaultStatVarF(s)
 *  const struct Statement *s;
 *  Return the name structure which is the left hand side of the
 *  default assignment.
 */

#ifdef NDEBUG
#define DefaultStatRHS(s) ((s)->v.asgn.rhs)
#else
#define DefaultStatRHS(s) DefaultStatRHSF(s)
#endif
extern struct Expr *DefaultStatRHSF(CONST struct Statement *);
/**< 
 *  macro DefaultStatRHS(s)
 *  struct Expr *DefaultStatRHSF(s)
 *  const struct Statement *s;
 *  Return the right hand side expression of the default assignment.
 */

#ifdef NDEBUG
#define AssignStatVar(sptr) ((sptr)->v.asgn.nptr)
#else
#define AssignStatVar(sptr) AssignStatVarF(sptr)
#endif
extern struct Name *AssignStatVarF(CONST struct Statement *);
/**< 
 *  macro AssignStatVar(s)
 *  struct Name *AssignStatVarF(s)
 *  const struct Statement *s;
 *  Return the name structure which is the left hand side of the
 *  structural assignment.
 */

#ifdef NDEBUG
#define AssignStatRHS(s) ((s)->v.asgn.rhs)
#else
#define AssignStatRHS(s) AssignStatRHSF(s)
#endif
extern struct Expr *AssignStatRHSF(CONST struct Statement *);
/**< 
 *  macro AssignStatRHS(s)
 *  struct Expr *AssignStatRHSF(s)
 *  const struct Statement *s;
 *  Return the right hand side expression of the structural assignment.
 */

/*** StateRelation functions ***/

#ifdef NDEBUG
#define RelationStatName(s) ((s)->v.rel.nptr)
#else
#define RelationStatName(s) RelationStatNameF(s)
#endif
extern struct Name *RelationStatNameF(CONST struct Statement *);
/**< 
 *  macro RelationStatName(s)
 *  struct Name *RelationStatNameF(s)
 *  const struct Statement *s;
 *  Return the name of the relation.
 */

#ifdef NDEBUG
#define RelationStatExpr(s) ((s)->v.rel.relation)
#else
#define RelationStatExpr(s) RelationStatExprF(s)
#endif
extern struct Expr *RelationStatExprF(CONST struct Statement *);
/**< 
 *  macro RelationStatExpr(s)
 *  struct Expr *RelationStatExprF(s)
 *  const struct Statement *s;
 *  Return the relation expression.
 */


#ifdef NDEBUG
#define LogicalRelStatName(s) ((s)->v.lrel.nptr)
#else
#define LogicalRelStatName(s) LogicalRelStatNameF(s)
#endif
extern struct Name *LogicalRelStatNameF(CONST struct Statement *);
/**< 
 *  macro LogicalRelStatName(s)
 *  struct Name *LogicalRelStatNameF(s)
 *  const struct Statement *s;
 *  Return the name of the logical relation.
 */

#ifdef NDEBUG
#define LogicalRelStatExpr(s) ((s)->v.lrel.logrel)
#else
#define LogicalRelStatExpr(s) LogicalRelStatExprF(s)
#endif
extern struct Expr *LogicalRelStatExprF(CONST struct Statement *);
/**< 
 *  macro LogicalRelStatExpr(s)
 *  struct Expr *LogicalRelStatExprF(s)
 *  const struct Statement *s;
 *  Return the logical relation expression.
 */

/*** StateExternal functions ***/

#ifdef NDEBUG
#define ExternalStatMode(s) ((s)->v.ext.mode)
#else
#define ExternalStatMode(s) ExternalStatModeF(s)
#endif
extern int ExternalStatModeF(CONST struct Statement *);
/**< 
 *  macro ExternalStatMode(s)
 *  int ExternalStatModeF(s)
 *  const struct Statement *s;
 *  Return the external statement's mode.
 *  0 = Procedural, 1 = Glassbox Declarative, 2 = Blackbox Declarative.
 */

#ifdef NDEBUG
#define ExternalStatName(s) ((s)->v.ext.nptr)
#else
#define ExternalStatName(s) ExternalStatNameF(s)
#endif
extern struct Name *ExternalStatNameF(CONST struct Statement *);
/**< 
 *  macro ExternalStatName(s)
 *  struct Name *ExternalStatNameF(s)
 *  const struct Statement *s;
 *  Return the external statement's name.
 */

#ifdef NDEBUG
#define ExternalStatData(s) ((s)->v.ext.data)
#else
#define ExternalStatData(s) ExternalStatDataF(s)
#endif
extern struct Name *ExternalStatDataF(CONST struct Statement *);
/**< 
 *  macro ExternalStatData(s)
 *  struct Name *ExternalStatDataF(s)
 *  const struct Statement *s;
 *  Return the external statement's name of its additional data.
 */

#ifdef NDEBUG
#define ExternalStatScope(s) ((s)->v.ext.scope)
#else
#define ExternalStatScope(s) ExternalStatScopeF(s)
#endif
extern struct Name *ExternalStatScopeF(CONST struct Statement *);
/**< 
 *  macro ExternalStatScope(s)
 *  struct Name *ExternalStatScopeF(s)
 *  const struct Statement *s;
 *  Return the external statement's name of its scope. This may be NULL.
 *  If it is NULL, then the external relation (at this time assumed to be
 *  a glassbox, will added at the scope that it was found; otherwise it
 *  will be added at the given scope.
 */

#ifdef NDEBUG
#define ExternalStatVlist(s) ((s)->v.ext.vl)
#else
#define ExternalStatVlist(s) ExternalStatVlistF(s)
#endif
extern struct VariableList *ExternalStatVlistF(CONST struct Statement *);
/**< 
 *  macro ExternalStatVlist(s)
 *  struct VariableList *ExternalStatVlist(s)
 *  const struct Statement *s;
 *  Return the external statement's variable list -- the arguement list.
 */


#ifdef NDEBUG
#define ExternalStatFuncName(s) ((s)->v.ext.extcall)
#else
#define ExternalStatFuncName(s) ExternalStatFuncNameF(s)
#endif
extern CONST char *ExternalStatFuncNameF(CONST struct Statement *);
/**< 
 *  macro ExternalStatFuncName(s)
 *  struct Name *ExternalStatFuncNameF(s)
 *  const struct Statement *s;
 *  Return the external statement's function call
 *  name. -- the name invoked with.
 */

/*** StateReference functions ***/

#ifdef NDEBUG
#define ReferenceStatMode(s) ((s)->v.ref.mode)
#else
#define ReferenceStatMode(s) ReferenceStatModeF(s)
#endif
extern int ReferenceStatModeF(CONST struct Statement *);
/**< 
 *  macro ReferenceStatMode(s)
 *  int ReferenceStatModeF(s)
 *  const struct Statement *s;
 *  Return the mode of the refernce statement. Mode at the moment:
 *  0 => copy/clone semantics; 1 => pure reference semantics, such as in
 *  languages with pointers. Value semantics is handled by ISA statements
 */

#ifdef NDEBUG
#define ReferenceStatName(s) ((s)->v.ref.ref_name)
#else
#define ReferenceStatName(s) ReferenceStatNameF(s)
#endif
extern symchar *ReferenceStatNameF(CONST struct Statement *);
/**< 
 *  macro ReferenceStatName(s)
 *  const char *ReferenceStatNameF(s)
 *  const struct Statement *s;
 *  Return the name of the thing being referred to. This name is a simple
 *  such as a type description. Later this may become accept a fully qulafied
 *  id., and will depend upon the semantics of how it was invoked.
 *  This name may alternatively be called the *referee*
 */

#ifdef NDEBUG
#define ReferenceStatSetType(s) ((s)->v.ref.settype)
#else
#define ReferenceStatSetType(s) ReferenceStatSetTypeF(s)
symchar *ReferenceStatSetTypeF(CONST struct Statement *s);
#endif
/**< 
 *  macro ReferenceStatSetType(s)
 *  symchar *ReferenceStatSetType(s)
 *  const struct Statement *s;
 *  Return the type of set for a REF statement. This may be NULL, which
 *  means that there is no "OF" part to the _IS_ statement.
 *  Eg: 	foo[thing] _IS_ set OF barobject;	-- not NULL
 *              foo_other  _IS_ bar_other;		-- NULL.
 */

#ifdef NDEBUG
#define ReferenceStatVlist(s) ((s)->v.ref.vl)
#else
#define ReferenceStatVlist(s) ReferenceStatVlistF(s)
#endif
extern struct VariableList *ReferenceStatVlistF(CONST struct Statement *);
/**< 
 *  macro ReferenceStatVlist(s)
 *  struct VariableList *ReferenceStatVlistF(s)
 *  const struct Statement *s;
 *  Return the list of variables/models that refer to the given referee.
 */

/*** StateRun functions ***/

#ifdef NDEBUG
#define RunStatName(s) ((s)->v.r.proc_name)
#else
#define RunStatName(s) RunStatNameF(s)
#endif
extern struct Name *RunStatNameF(CONST struct Statement *);
/**< 
 *  macro RunStatName(s)
 *  struct Name *RunStatNameF(s)
 *  const struct Statement *s;
 *  Returns the name pointer of a run statement.
 */

#ifdef NDEBUG
#define RunStatAccess(s) ((s)->v.r.type_name)
#else
#define RunStatAccess(s) RunStatAccessF(s)
#endif
extern struct Name *RunStatAccessF(CONST struct Statement *);
/**< 
 *  macro RunStatAccess(s)
 *  char *RunStatAccessF(s)
 *  const struct Statement *s;
 *  Returns the type access of a RUN statement. This is to allow the
 *  use of syntax such as: RUN A::b where 'B' is a class/model/atom, and b
 *  is a name of procedure within that class/atom/model.
 */

/*** StateCall functions ***/
#ifdef NDEBUG
#define CallStatArgs(s) ((s)->v.call.args)
#else
#define CallStatArgs(s) CallStatArgsF(s)
#endif
extern struct Set *CallStatArgsF(CONST struct Statement *);
/**< 
 *  macro CallStatArgs(s)
 *  struct Set *CallStatArgs(s)
 *  const struct Statement *s;
 *  Return the call statement's argument list.
 */

#ifdef NDEBUG
#define CallStatId(s) ((s)->v.call.id)
#else
#define CallStatId(s) CallStatIdF(s)
#endif
extern symchar *CallStatIdF(CONST struct Statement *);
/**< 
 *  macro CallStatId(s)
 *  symchar *CallStatId(s)
 *  const struct Statement *s;
 *  Return the call statement's function id.
 */

/*** StateWhile functions ***/

#ifdef NDEBUG
#define WhileStatExpr(s) ((s)->v.loop.test)
#else
#define WhileStatExpr(s) WhileStatExprF(s)
#endif
extern struct Expr *WhileStatExprF(CONST struct Statement *);
/**< 
 *  macro WhileStatExpr(s)
 *  struct Expr *WhileStatExprF(s)
 *  const struct Statement *s;
 *  Return the WHILE expression.
 */

#ifdef NDEBUG
#define WhileStatBlock(s) ((s)->v.loop.block)
#else
#define WhileStatBlock(s) WhileStatBlockF(s)
#endif
extern struct StatementList *WhileStatBlockF(CONST struct Statement *);
/**< 
 *  macro WhileStatBlock(s)
 *  struct StatementList *WhileStatBlockF(s)
 *  const struct Statement *s;
 *  Return the block of a while statement.
 */

/**< stateflow */

#define FlowStatControl(s) ((s)->v.flow.fc)
/**< return the type of flow control. needs a function wrapper. */

#define FlowStatMessage(s) ((s)->v.flow.message)
/**< return the message of the flow control. needs a function wrapper. */

/*** StateIf functions ***/

#ifdef NDEBUG
#define IfStatExpr(s) ((s)->v.ifs.test)
#else
#define IfStatExpr(s) IfStatExprF(s)
#endif
extern struct Expr *IfStatExprF(CONST struct Statement *);
/**< 
 *  macro IfStatExpr(s)
 *  struct Expr *IfStatExprF(s)
 *  const struct Statement *s;
 *  Return the IF expression.
 */

#ifdef NDEBUG
#define IfStatThen(s) ((s)->v.ifs.thenblock)
#else
#define IfStatThen(s) IfStatThenF(s)
#endif
extern struct StatementList *IfStatThenF(CONST struct Statement *);
/**< 
 *  macro IfStatThen(s)
 *  struct StatementList *IfStatThenF(s)
 *  const struct Statement *s;
 *  Return the then block of an if statement.
 */

#ifdef NDEBUG
#define IfStatElse(s) ((s)->v.ifs.elseblock)
#else
#define IfStatElse(s) IfStatElseF(s)
#endif
extern struct StatementList *IfStatElseF(CONST struct Statement *);
/**< 
 *  macro IfStatElse(s)
 *  struct StatementList *IfStatElseF(s)
 *  const struct Statement *s;
 *  Return the else block of an if statement.  This is NULL if the else
 *  was omitted.
 */

/*** StateWhen functions ***/

#ifdef NDEBUG
#define WhenStatName(s) ((s)->v.w.nptr)
#else
#define WhenStatName(s) WhenStatNameF(s)
#endif
extern struct Name *WhenStatNameF(CONST struct Statement *);
/**< 
 *  macro WhenStatName(s)
 *  struct Name *WhenStatName(s)
 *  const struct Statement *s;
 *  Return the name of a WHEN node.
 */


extern void SetWhenName(struct Statement *, struct Name *);
/**< 
 *  void SetWhenName(s,n)
 *  struct Statement *s;
 *  struct Name *n;
 *  This procedure provides a mechanism for unnamed whens to be named.
 */


#ifdef NDEBUG
#define WhenStatVL(s) ((s)->v.w.vl)
#else
#define WhenStatVL(s) WhenStatVLF(s)
#endif
extern struct VariableList *WhenStatVLF(CONST struct Statement *);
/**< 
 *  macro WhenStatVL(s)
 *  struct VariableList *WhenStatVLF(s)
 *  const struct Statement *s;
 *  Return the variable list part of a WHEN statement.
 */

#ifdef NDEBUG
#define WhenStatCases(s) ((s)->v.w.cases)
#else
#define WhenStatCases(s) WhenStatCasesF(s)
#endif
extern struct WhenList *WhenStatCasesF(CONST struct Statement *);
/**< 
 *  macro WhenStatCases(s)
 *  struct WhenList *WhenStatCasesF(s)
 *  const struct Statement *s;
 *  Return the list of cases of a WHEN statement. To obtain the
 *  statement list in each of the CASEs, see when.h
 */

extern int CompareWhenStatements(CONST struct Statement *,
            CONST struct Statement *);
/**< 
 *  Compare functions for WHEN statements. It includes the decomposition
 *  of the WHEN in the list of variables and the list of CASEs. Also,
 *  each case is decomposed in the set of values and the list of statements.
 *  It is done here since we are comparing the statement rather than only a
 *  WhenList structure (when.[ch]).
 *  It is called for the function CompareStatements and it follows the
 *  same pattern for the comparation.
 *
 */


/*** StateFname functions ***/

#ifdef NDEBUG
#define FnameStat(s) ((s)->v.n.wname)
#else
#define FnameStat(s) FnameStatF(s)
#endif
extern struct Name *FnameStatF(CONST struct Statement *);
/**< 
 *  macro FnameStatcases(s)
 *  struct Name *FnameStatF(s)
 *  const struct Statement *s;
 *  Return the name of a model or relation (inside WHEN statement)
 */

/*** StateCOND functions ***/

#ifdef NDEBUG
#define CondStatList(s) ((s)->v.cond.stmts)
#else
#define CondStatList(s) CondStatListF(s)
#endif
extern struct StatementList *CondStatListF(CONST struct Statement *);
/**< 
 *  macro CondStatList(s)
 *  struct StatementList *CondStatListF(s)
 *  const struct Statement *s;
 *  Return the list of statements in a Conditional statement.
 */

#ifdef NDEBUG
#define CondContains(s) ((s)->v.cond.contains)
#define CondContainsRelations(s) ((s)->v.cond.contains & contains_REL)
#define CondContainsLogRelations(s) ((s)->v.cond.contains & contains_LREL)
#else
#define CondContains(s) CondContainsF(s)
#define CondContainsRelations(s) CondContainsRelationsF(s)
#define CondContainsLogRelations(s) CondContainsLogRelationsF(s)
#endif
extern unsigned CondContainsF(CONST struct Statement *);
extern unsigned CondContainsRelationsF(CONST struct Statement *);
extern unsigned CondContainsLogRelationsF(CONST struct Statement *);
/**< 
 *  macro CondContains(s)
 *  unsigned CondContainsF(s)
 *  macro CondContainsRelations(s)
 *  unsigned CondContainsRelationsF(s)
 *  macro CondContainsLogRelations(s)
 *  unsigned CondContainsLogRelationsF(s)
 *  const struct Statement *s;
 *
 *  Respectively:
 *  Returns the entire contains field.
 *  Return true if the Cond statement given contains a relation.
 *  Return true if the Cond statement given contains a logical relation.
 */


/*** StateSelect functions ***/

#ifdef NDEBUG
#define SelectStatVL(s) ((s)->v.se.vl)
#else
#define SelectStatVL(s) SelectStatVLF(s)
#endif
extern struct VariableList *SelectStatVLF(CONST struct Statement *);
/**< 
 *  macro SelectStatVL(s)
 *  struct VariableList *SelectStatVLF(s)
 *  const struct Statement *s;
 *  Return the set part of a SELECT statement.
 */

#ifdef NDEBUG
#define SelectStatNumberStats(s) ((s)->v.se.n_statements)
#else
#define SelectStatNumberStats(s) SelectStatNumberStatsF(s)
#endif
extern int SelectStatNumberStatsF(CONST struct Statement *);
/**< 
 *  macro SelectStatNumberStats(s)
 *  int SelectStatNumberStatsF(s)
 *  const struct Statement *s;
 *  Return the number of statements inside a SELECT statement.
 */

#ifdef NDEBUG
#define SelectStatCases(s) ((s)->v.se.cases)
#else
#define SelectStatCases(s) SelectStatCasesF(s)
#endif
extern struct SelectList *SelectStatCasesF(CONST struct Statement *);
/**< 
 *  macro SelectStatCases(s)
 *  struct SelectList *SelectStatCasesF(s)
 *  const struct Statement *s;
 *  Return the list of cases of a SELECT statement. To obtain the
 *  statement list in each of the CASEs, see select.h
 */

extern int CompareSelectStatements(CONST struct Statement *,
              CONST struct Statement *);

/**< 
 *  Compare functions for SELECT statements. It includes the decomposition
 *  of the SELECT in the list of variables and the list of CASEs. Also,
 *  each case is decomposed in the set of values and the list of statements.
 *  It is done here since we are comparing the statement rather than only a
 *  SelectList structure (select.[ch]).
 *
 *  It is called for the function CompareStatements and it follows the
 *  same pattern for the comparation.
 *
 */

#ifdef NDEBUG
#define SelectContains(s) ((s)->v.se.contains)
#define SelectContainsRelations(s) ((s)->v.se.contains & contains_REL)
#define SelectContainsLogRelations(s) ((s)->v.se.contains & contains_LREL)
#define SelectContainsDefaults(s) ((s)->v.se.contains & contains_DEF)
#define SelectContainsCAssigns(s) ((s)->v.se.contains & contains_CAS)
#define SelectContainsWhen(s) ((s)->v.se.contains & contains_WHEN)
#define SelectContainsAlike(s) ((s)->v.se.contains & contains_AA)
#define SelectContainsAlias(s) ((s)->v.se.contains & contains_ALI)
#define SelectContainsArray(s) ((s)->v.se.contains & contains_ARR)
#define SelectContainsIsa(s) ((s)->v.se.contains & contains_ISA)
#define SelectContainsIrt(s) ((s)->v.se.contains & contains_IRT)
#define SelectContainsAts(s) ((s)->v.se.contains & contains_ATS)
#define SelectContainsWbts(s) ((s)->v.se.contains & contains_WBTS)
#define SelectContainsWnbts(s) ((s)->v.se.contains & contains_WNBTS)
#define SelectContainsWillbe(s) ((s)->v.se.contains & contains_WILLBE)
#define SelectContainsSelect(s) ((s)->v.se.contains & contains_SELECT)
#define SelectContainsConditional(s) ((s)->v.se.contains & contains_COND)
#define SelectContainsIllegal(s) ((s)->v.se.contains & contains_ILL)
#else
#define SelectContains(s) SelectContainsF(s)
#define SelectContainsRelations(s) SelectContainsRelationsF(s)
#define SelectContainsLogRelations(s) SelectContainsLogRelationsF(s)
#define SelectContainsDefaults(s) SelectContainsDefaultsF(s)
#define SelectContainsCAssigns(s) SelectContainsCAssignsF(s)
#define SelectContainsWhen(s) SelectContainsWhenF(s)
#define SelectContainsAlike(s) SelectContainsAlikeF(s)
#define SelectContainsAlias(s) SelectContainsAliasF(s)
#define SelectContainsArray(s) SelectContainsArrayF(s)
#define SelectContainsIsa(s) SelectContainsIsaF(s)
#define SelectContainsIrt(s) SelectContainsIrtF(s)
#define SelectContainsAts(s) SelectContainsAtsF(s)
#define SelectContainsWbts(s) SelectContainsWbtsF(s)
#define SelectContainsWnbts(s) SelectContainsWnbtsF(s)
#define SelectContainsWillbe(s) SelectContainsWillbeF(s)
#define SelectContainsSelect(s) SelectContainsSelectF(s)
#define SelectContainsConditional(s) SelectContainsConditionalF(s)
#define SelectContainsIllegal(s) SelectContainsIllegalF(s)
#endif
extern unsigned SelectContainsF(CONST struct Statement *);
extern unsigned SelectContainsRelationsF(CONST struct Statement *);
extern unsigned SelectContainsLogRelationsF(CONST struct Statement *);
extern unsigned SelectContainsDefaultsF(CONST struct Statement *);
extern unsigned SelectContainsCAssignsF(CONST struct Statement *);
extern unsigned SelectContainsWhenF(CONST struct Statement *);
extern unsigned SelectContainsAlikeF(CONST struct Statement *);
extern unsigned SelectContainsAliasF(CONST struct Statement *);
extern unsigned SelectContainsArrayF(CONST struct Statement *);
extern unsigned SelectContainsIsaF(CONST struct Statement *);
extern unsigned SelectContainsIrtF(CONST struct Statement *);
extern unsigned SelectContainsAtsF(CONST struct Statement *);
extern unsigned SelectContainsWbtsF(CONST struct Statement *);
extern unsigned SelectContainsWnbtsF(CONST struct Statement *);
extern unsigned SelectContainsWillbeF(CONST struct Statement *);
extern unsigned SelectContainsSelectF(CONST struct Statement *);
extern unsigned SelectContainsConditionalF(CONST struct Statement *);
extern unsigned SelectContainsIllegalF(CONST struct Statement *);
/**< 
 *  macro SelectContains(s)
 *  unsigned SelectContainsF(s)
 *  macro SelectContainsRelations(s)
 *  unsigned SelectContainsRelationsF(s)
 *  macro SelectContainsLogRelations(s)
 *  unsigned SelectContainsLogRelationsF(s)
 *  macro SelectContainsDefaults(s)
 *  unsigned SelectContainsDefaultsF(s)
 *  macro SelectContainsCAssigns(s)
 *  unsigned SelectContainsCAssignsF(s)
 *  macro SelectContainsWhen(s)
 *  unsigned SelectContainsWhenF(s)
 *  macro SelectContainsIllegal(s)
 *  unsigned SelectContainsIllegalF(s)
 *  etc....
 *  const struct Statement *s;
 *
 *  Respectively:
 *  Returns the entire contains field.
 *  Return true if the SELECT statement given contains itself or in any of
 *  its nested statements a relation.
 *  Return true if the SELECT statement given contains itself or in any of
 *  its nested statements a default statement.
 *  Return true if the SELECT statement given contains itself or in any of
 *  its nested statements a constant/structural variable assignment.
 *  Return true if the SELECT statement given contains itself or in any of
 *  its nested statements a WHEN statement.
 *  Likewise for:
 *  ARE_ALIKE
 *  ALIASES
 *  CREATE ARRAY
 *  IS_A
 *  IS_REFINED_TO
 *  ARE_THE_SAME
 *  WILL_BE_THE_SAME
 *  WILL_BE
 *  SELECT
 *  CONDITIONAL.
 *  Return true if the Select statement given contains itself or in any of
 *  its nested statements an illegal instruction.
 */

/********************************/


/*** StateSwitch functions ***/

#ifdef NDEBUG
#define SwitchStatVL(s) ((s)->v.sw.vl)
#else
#define SwitchStatVL(s) SwitchStatVLF(s)
#endif
extern struct VariableList *SwitchStatVLF(CONST struct Statement *);
/**< 
 *  macro SwitchStatVL(s)
 *  struct VariableList *SwitchStatVLF(s)
 *  const struct Statement *s;
 *  Return the set part of a SWITCH statement.
 */

#ifdef NDEBUG
#define SwitchStatCases(s) ((s)->v.sw.cases)
#else
#define SwitchStatCases(s) SwitchStatCasesF(s)
#endif
extern struct SwitchList *SwitchStatCasesF(CONST struct Statement *);
/**< 
 *  macro SwitchStatCases(s)
 *  struct SwitchList *SwitchStatCasesF(s)
 *  const struct Statement *s;
 *  Return the list of cases of a SWITCH statement. To obtain the
 *  statement list in each of the CASEs, see switch.h
 */

extern int CompareSwitchStatements(CONST struct Statement *,
              CONST struct Statement *);

/**< 
 *  Compare functions for SWITCH statements. It includes the decomposition
 *  of the Switch in the list of variables and the list of CASEs. Also,
 *  each case is decomposed in the set of values and the list of statements.
 *  It is done here since we are comparing the statement rather than only a
 *  SwitchList structure (switch.[ch]).
 *
 *  It is called for the function CompareStatements and it follows the
 *  same pattern for the comparation.
 *
 */

/**************************************/


extern int CompareStatements(CONST struct Statement *,
                             CONST struct Statement *);
/**< 
 *  CompareStatements(s1,s2);
 *  Returns -1,0,1 as s1 is <, ==, > s2.
 *  s1 < s2 if type(s1) < type(s2), exception: NULL > all statements.
 *  For statements of the same type, compared according to
 *  the number of arguments or alphabetically, as appropriate.
 *  We are comparing statement contents, not statement memory location
 *  or origin.
 *  Special notes:
 *  For a less picky version (when it comes to type compatibility)
 *  see CompareISStatements below.
 */

extern int CompareISStatements(CONST struct Statement *,
                               CONST struct Statement *);
/**< 
 *  CompareISStatements(s1,s2);
 *  s1 and s2 must be StateIS or some of the StateARE based statements, i.e.
 *  IS_A WILL_BE IS_REFINED_TO WILL_BE_THE_SAME statements, or
 *  s1 and s2 must be relations.
 *  FOR loops containing only these kinds are also allowed.
 *  Returns -1,0,1 as s1 is <, ==, > s2.
 *  s1 < s2 if StatementType(s1) < StatementType(s2),
 *  exception: NULL > all statements.
 *
 *  For statements of the same StatementType, compared according to
 *  the number of arguments or alphabetically, as appropriate
 *  after type compatibility is checked. In particular,
 *  if MoreRefined(GetStateType(s1),GetStatType(s2)) returns
 *  the type from s2, the two potentially type differing statements
 *  which OTHERWISE are identical are considered identical.
 *  If s1 is more refined than s2, the statements are considered
 *  incompatible. If s1 has a WITH_VALUE clause that does not match
 *  s2, the statements are incompatible.
 *
 *  We are comparing statement contents, not statement memory location
 *  or origin.
 *
 *  Special notes:
 *  For a more picky version (when it comes to type compatibility)
 *  see CompareStatements above.
 */

#endif /**< __STATEMENT_H_SEEN__ */
