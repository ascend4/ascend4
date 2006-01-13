/*
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

/** @file
 *  Statement list module.
 *  <pre>
 *  When #including statement.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "stattypes.h"
 *  </pre>
 */

#ifndef __STATEMENT_H_SEEN__
#define __STATEMENT_H_SEEN__

extern void AddContext(struct StatementList *slist ,unsigned int c);
/**<
 *  <!--  AddContext(slist,c)                                          -->
 *
 *  Adds the given context flags, c, to the statements in the list
 *  using the | operator, so that existing context
 *  bits are preserved (other than context_MODEL).
 *  Handles NULL slist gracefully.<br><br>
 *
 *  This function is recursive. Those statements which contain another
 *  statement list will have those lists visited with this function,
 *  adding the flags we have here. Context is a cumulative notion,
 *  after all. Careless use will make this a tad expensive for
 *  deeply nested code.<br><br>
 *
 *  All the create functions below return a statement (if successful)
 *  which will have a context value of context_MODEL (0).<br><br>
 *
 *  VRR. Added the context flags for statements inside WHEN, SELECT and
 *  CONDITIONAL. 2/3/97 and SWITCH 29/3/97
 */


extern unsigned int SlistHasWhat(struct StatementList *slist);
/**<
 *  <!--  unsigned int SlistHasWhat(slist);                            -->
 *  Returns an unsigned with the low bits set according to whether or
 *  not there is a statement of the indicated class in the slist given.
 *  The value is determined by examining the list, not by looking up
 *  some attribute.<br><br>
 *  The return type is valid as a contains field.
 */

extern struct Statement *CreateALIASES(struct VariableList *vl,
                                       struct Name *n);
/**<
 *  <!--  struct Statement *CreateALIASES(vl,n)                        -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  struct Name *n;                                              -->
 *  Create an ALIASES statement.
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 *  @param n  instance name
 */

extern struct Statement *CreateARR(struct VariableList *aname,
                                   struct VariableList *il,
                                   struct VariableList *setname,
                                   int intset,
                                   struct Set *setvals);
/**<
 *  <!--  struct Statement *CreateARR(aname,il,setname,intset,setvals) -->
 *  <!--  struct VariableList *avlname;                                -->
 *  <!--  struct VariableList *il;                                     -->
 *  <!--  struct VariableList *setvlname;                              -->
 *  <!--  int intset;                                                  -->
 *  <!--  struct Set *setvals;                                         -->
 *  Create a compound ALIASES-IS_A statement.
 *  VariableListLength(avlname) and VariableListLength(setname) should
 *  be 1.
 *  setvals may be NULL, rest may must be supplied.
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param aname   avlname lhs name (in varlist form) of array being made.
 *  @param il      rhs varlist of things to alias.
 *  @param setname name (in varlist form) of set instance to create which 
 *                 indexes avlname.
 *  @param intset  symbol (0) or integer (1) set instance.
 *  @param setvals set expression for subscripts. Must evaluate to match
 *                 count of items il evaluates to if setvals != NULL.
 */

extern struct Statement *CreateISA(struct VariableList *vl,
                                   symchar *t,
                                   struct Set *ta,
                                   symchar *st);
/**<
 *  <!--  struct Statement *CreateISA(vl,t,ta,st)                      -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  struct Set *ta;                                              -->
 *  <!--  const char *t,*st;                                           -->
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 *  @param t  instance type
 *  @param ta arguments for type t
 *  @param st set type
 */

extern struct Statement *CreateWILLBE(struct VariableList *vl,
                                      symchar *t,
                                      struct Set *ta,
                                      symchar *st,
                                      struct Expr *cv);
/**<
 *  <!--  struct Statement *CreateWILLBE(vl,t,ta,st,cv)                -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  struct Set *ta;                                              -->
 *  <!--  const char *t,*st;                                           -->
 *  <!--  struct Expr *cv;                                             -->
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 *  @param t  instance type
 *  @param ta arguments for type t
 *  @param st set type (possibly)
 *  @param cv WILL_BE value specification.
 */

extern struct Statement *CreateIRT(struct VariableList *vl,
                                   symchar *t,
                                   struct Set *ta);
/**<
 *  <!--  struct Statement *CreateIRT(vl,t,ta)                         -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  const char *t;                                               -->
 *  <!--  struct Set *ta;                                              -->
 *  Create an IS_REFINED_TO statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 *  @param t  instance type
 *  @param ta arguments for type t
 */

extern struct Statement *CreateAA(struct VariableList *vl);
/**<
 *  <!--  struct Statement *CreateAA(vl)                               -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Create an ARE_ALIKE statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 */

extern struct Statement *CreateATS(struct VariableList *vl);
/**<
 *  <!--  struct Statement *CreateATS(vl)                              -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Create an ARE_THE_SAME statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 */

extern struct Statement *CreateFIX(struct VariableList *vl);
/**<
	Create a 'FIX' statement node. The statements module/line number
	are stored, and the variable list is also stored.
*/

extern struct Statement *CreateWBTS(struct VariableList *vl);
/**<
 *  <!--  struct Statement *CreateWBTS(vl)                             -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Create an WILL_BE_THE_SAME statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 */

extern struct Statement *CreateWNBTS(struct VariableList *vl);
/**<
 *  <!--  struct Statement *CreateWNBTS(vl)                            -->
 *  <!--  struct VariableList *vl;                                     -->
 *  Create an WILL_NOT_BE_THE_SAME statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *
 *  @param vl variable list
 */

extern struct Statement *CreateFOR(symchar *index,
                                   struct Expr *expr,
                                   struct StatementList *stmts,
                                   enum ForOrder order,
                                   enum ForKind kind);
/**<
 *  <!--  struct Statement *CreateFOR(index,expr,stmts,order,kind)     -->
 *  <!--  struct Expr *expr;                                           -->
 *  <!--  const char *index;                                           -->
 *  <!--  struct StatementList *stmts;                                 -->
 *  <!--  enum ForOrder order;                                         -->
 *  <!--  enum ForKind kind;                                           -->
 *
 *  Create a FOR statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *  The statement list is checked for presence of relations,
 *  default assignments, and structural assignments and marked
 *  as needed.
 *  The context bit context_FOR will be set on the statements
 *  in the list stmts that have a context field.
 */

extern struct Statement *CreateREL(struct Name *n,
                                   struct Expr *relation);
/**<
 *  <!--  struct Statement *CreateREL(n,relation);                     -->
 *  <!--  struct Name *n;                                              -->
 *  <!--  struct Expr *relation;                                       -->
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateLOGREL(struct Name *n,
                                      struct Expr *logrel);
/**<
 *  <!--  struct Statement *CreateLOGREL(n,logrel);                    -->
 *  <!--  struct Name *n;                                              -->
 *  <!--  struct Expr *logrel;                                         -->
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */


extern struct Statement *CreateEXTERN(int mode,
                                      struct Name *n,
                                      CONST char *funcname,
                                      struct VariableList *vl,
                                      struct Name *data,
                                      struct Name *scope);
/**<
 *  <!--  struct Statement *CreateEXTERN(mode,n,funcname,vl,data,scope); -->
 *  <!--  int mode; -- 0 = procedural;                                 -->
 *  <!--            --  1 = glassbox declarative;                      -->
 *  <!--            --  2 = blackbox declarative                       -->
 *  <!--  struct Name *n;                                              -->
 *  <!--  char *funcname;                                              -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  struct Name *data; -- additional user data or NULL;          -->
 *  <!--  struct Name *scope; -- scope at which external relations are to -->
 *  <!--                        be embedded. Applicable only to glassboxes -->
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *  @param mode  0 = procedural; 1 = glassbox declarative; 2 = blackbox declarative
 *  @param data  additional user data or NULL
 *  @param scope scope at which external relations are to be embedded.
 *               Applicable only to glassboxes.
 */

extern struct Statement *CreateFlow(enum FlowControl fc,
                                    CONST char *msgtext);
/**<
 *  <!--  struct Statement *CreateFlow(fc,msgtext);                    -->
 *  Create a BREAK, CONTINUE, FALL_THROUGH, RETURN, STOP as
 *  indicated by fc. If msgtext is not NULL, it is
 *  converted to a bracechar and stored as the message.
 */

extern struct Statement *CreateREF(struct VariableList *vl,
                                   symchar *ref_name,
                                   symchar *st,
                                   int mode);
/**<
 *  <!--  struct Statement *CreateREF(vl,ref_name,st,mode)             -->
 *  <!--  struct VariableList *vl;                                     -->
 *  <!--  const char *ref_name;                                        -->
 *  <!--  const char *settype;                                         -->
 *  <!--  int mode; -- 0 = copy semantics; (_IS_)                      -->
 *  <!--          --  1 = pure reference semantics; (_REFERS_)         -->
 *  Creates a new REFERENCE type statement. ref_name is the name of
 *  the type or prototype being referred to.
 *  Initializes the reference count to one.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.<br><br>
 *
 *  NOT IMPLEMENTED.
 *
 *  @param mode 0 = copy semantics (_IS_);
 *              1 = pure reference semantics; (_REFERS_)
 */

extern void SetRelationName(struct Statement *stat, struct Name *n);
/**<
 *  <!--  void SetRelationName(stat,n)                                 -->
 *  <!--  struct Statement *stat;                                      -->
 *  <!--  struct Name *n;                                              -->
 *  This procedure provides a mechanism for unnamed relations to be named.
 *  It should only be called on relation statements that are unnamed.  It
 *  will name the relation with the name provided.
 */

extern void SetLogicalRelName(struct Statement *stat, struct Name *n);
/**<
 *  <!--  void SetLogicalRelName(stat,n)                               -->
 *  <!--  struct Statement *stat;                                      -->
 *  <!--  struct Name *n;                                              -->
 *  This procedure provides a mechanism for unnamed relations to be named.
 *  It should only be called on logcial relation statements that are unnamed.
 *  It will name the logical relation with the name provided.
 */


extern struct Statement *CreateRUN(struct Name *n, struct Name *type);
/**<
 *  <!--  struct Statement *CreateRUN(n,type)                          -->
 *  <!--  struct Name *n;                                              -->
 *  <!--  struct Name *type;                                           -->
 *  Create a run statement with procedure name n, and/or class type.
 *  If type is non NULL, then so called 'class access is to be used to
 *  access the procedure. This is equivalent to the "::" in some object
 *  oriented languages.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateCALL(symchar *n, struct Set *arglist);
/**<
 *  <!--  struct Statement *CreateCALL(n,argList)                      -->
 *  <!--  symchar *n;                                                  -->
 *  <!--  struct Set * argList;                                        -->
 *  Create a CALL statement with name n and arguments argList.
 *  argList may be NULL.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateASSERT(struct Expr *ex);
/**
	@param ex test expression.

	Create a ASSERT statement with test expression 'ex'. If the expression
	evaluates to false, an error message will be shown. Implemented for use
	in 'self_test' functionality.
*/

extern struct Statement *CreateIF(struct Expr *ex,
                                  struct StatementList *ifblock,
                                  struct StatementList *elseblock);
/**<
 *  <!--  struct Statement *CreateIF(ex,ifblock,elseblock)             -->
 *  <!--  struct Expr *ex;                                             -->
 *  <!--  struct StatementList *ifblock,*elseblock;                    -->
 *  Create an IF-THEN-ELSE structure.  If the else block is omitted,
 *  pass in NULL for elseblock.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateWhile(struct Expr *test,
                                     struct StatementList *body);
/**<
 *  <!--  struct Statement *CreateWhile(test,body)                     -->
 *  <!--  struct Expr *test;                                           -->
 *  <!--  struct StatementList *body;                                  -->
 *  Create a WHILE-DO structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateWHEN(struct Name *wname,
                                    struct VariableList *vlist,
                                    struct WhenList *wl);
/**<
 *  <!--  struct Statement *CreateWHEN(wname,vlist,wl)                 -->
 *  <!--  struct Name *wname;                                          -->
 *  <!--  struct VariableList *vlist;                                  -->
 *  <!--  struct WhenList *wl;                                         -->
 *  Crease a WHEN statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateFNAME(struct Name *name);
/**<
 *  <!--  struct Statement *CreateFNAME(name)                          -->
 *  Crease a FNAME statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 *  Name is the name of a model or a relation inside a WHEN statement
 */

extern struct Statement *CreateSWITCH(struct VariableList *v,
                                      struct SwitchList *sw);
/**<
 *  <!--  struct Statement *CreateSWITCH(v,sw)                         -->
 *  <!--  struct VariableList *v;                                      -->
 *  <!--  struct SelectList *sw;                                       -->
 *  Crease a SWITCH statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateCOND(struct StatementList *stat);
/**<
 *  <!--  struct Statement *CreateCOND(stat)                           -->
 *  <!--  struct StatementList *stmts;                                 -->
 *  Create a CONDITIONAL statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateSELECT(struct VariableList *v,
                                      struct SelectList *sel);
/**<
 *  <!--  struct Statement *CreateSELECT(v,sel)                        -->
 *  <!--  struct VariableList *v;                                      -->
 *  <!--  struct SelectList *sel;                                      -->
 *  Crease a SELECT statement structure.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateASSIGN(struct Name *n,
                                      struct Expr *rhs);
/**<
 *  <!--  struct Statement *CreateASSIGN(n,rhs);                       -->
 *  <!--  struct Name *n;                                              -->
 *  <!--  struct Expr *rhs;                                            -->
 *  Create a default assignment statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

extern struct Statement *CreateCASSIGN(struct Name *n, struct Expr *rhs);
/**<
 *  <!--  struct Statement *CreateCASSIGN(n,rhs);                      -->
 *  <!--  struct Name *n;                                              -->
 *  <!--  struct Expr *rhs;                                            -->
 *  Create a structural assignment statement node.
 *  The statement's module is set to the current open module.
 *  The statement's line number is set to the current line number.
 */

#ifdef NDEBUG
#define StatementType(s) ((s)->t)
#else
#define StatementType(s) StatementTypeF(s)
#endif
/**<
 *  Return the statement type of s.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The type as an enum stat_t.
 *  @see StatementTypeF()
 */
extern enum stat_t StatementTypeF(CONST struct Statement *s);
/**<
 *  <!--  macro StatementType(s)                                       -->
 *  <!--  enum stat_t StatementTypeF(s)                                -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the statement type of s.                              -->
 *  Implementation function for StatementType().  Do not call this
 *  function directly - use StatementType() instead.
 */

#ifdef NDEBUG
#define StatementModule(s) ((s)->mod)
#else
#define StatementModule(s) StatementModuleF(s)
#endif
/**<
 *  Return the module that statement s was defined in.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The defining module as a struct module_t*.
 *  @see StatementModuleF()
 */
extern struct module_t *StatementModuleF(CONST struct Statement *s);
/**<
 *  <!--  macro StatementModule(s)                                     -->
 *  <!--  struct module_t *StatementModuleF(s)                         -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the module that statement s was defined in.           -->
 *  Implementation function for StatementModule().  Do not call this
 *  function directly - use StatementModule() instead.
 */

#ifdef NDEBUG
#define StatementLineNum(s) ((s)->linenum)
#else
#define StatementLineNum(s) StatementLineNumF(s)
#endif
/**<
 *  Return the line number where statement s was defined.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The line number as an unsigned long.
 *  @see StatementLineNumF()
 */
extern unsigned long StatementLineNumF(CONST struct Statement *s);
/**<
 *  <!--  macro StatementLineNum(s)                                    -->
 *  <!--  unsigned long StatementLineNumF(s)                           -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the line number where statement s was defined.        -->
 *  Implementation function for StatementLineNum().  Do not call this
 *  function directly - use StatementLineNum() instead.
 */

#ifdef NDEBUG
#define CopyStatement(s) if ((s)->ref_count<MAXREFCOUNT) (((s)->ref_count)++)
#else
#define CopyStatement(s) CopyStatementF(s)
#endif
/**<
 *  Make a new reference to statement s.  This does not actually make a new
 *  copy in memory of the data in s: it only increases the reference count.
 *  If you need to modify the statement, you should use CopyToModify which
 *  is defined below.
 *  You don't need to worry about the count overflowing unless the user is
 *  utterly insane.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The new reference as a struct Statement*.
 *  @see CopyStatementF()
 */
extern struct Statement *CopyStatementF(struct Statement *s);
/**<
 *  <!--  struct Statement *CopyStatementF(s)                          -->
 *  <!--  struct Statement *s;                                         -->
 *  <!--  Make a new reference to statement s.  This does not actually make a new -->
 *  <!--  copy in memory of the data in s: it only increases the reference count. -->
 *  <!--  If you need to modify the statement, you should use CopyToModify which  -->
 *  <!--  is defined below.                                            -->
 *  <!--  You don't need to worry about the count overflowing unless the user is  -->
 *  <!--  utterly insane.                                              -->
 *  Implementation function for CopyStatement().  Do not call this
 *  function directly - use CopyStatement() instead.
 */

extern void DestroyStatement(struct Statement *s);
/**< 
 *  <!--  void DeleteStatement(s)                                      -->
 *  <!--  struct Statement *s;                                         -->
 *  Delete a reference to statement s.  This make not actually deallocate
 *  the memory associated with s if there are still other references to
 *  the statement.  If there aren't any, remaining references it deallocates
 *  the memory.
 */

extern struct Statement *CopyToModify(struct Statement *s);
/**< 
 *  <!--  struct Statement *CopyToModify(s)                            -->
 *  <!--  struct Statement *s;                                         -->
 *  This will always make a new separate copy of the statement, so that
 *  it can be modified.  The statement reference count is initialize to
 *  one.
 */

#ifndef NDEBUG
#define GetStatContext(s) ((s)->context)
#else
#define GetStatContext(s) GetStatContextF(s)
#endif
/**<
 *  Returns the context bits of the statement.
 *  If statement is of a type that doesn't have context bits,
 *  returns context_MODEL and issues warning message.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The context as an unsigned int.
 *  @see GetStatContextF()
 */
extern unsigned int GetStatContextF(CONST struct Statement *s);
/**<
 *  <!--  bits = GetStatContext(s);                                    -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Returns the context bits of the statement.                   -->
 *  <!--  If statement is of a type that doesn't have context bits,    -->
 *  <!--  returns context_MODEL and issues warning message.            -->
 *  Implementation function for GetStatContext().  Do not call this
 *  function directly - use GetStatContext() instead.
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

extern void SetStatContext(struct Statement *s, unsigned int bits);
/**< 
 *  <!--  SetStatContext(s,bits)                                       -->
 *  Sets the context field of a statement to the value, bits, given.
 *  E.g. to turn on the FOR context bit,
 *  This function should only be used at parse time, and probably not
 *  then -- see MarkStatContext for the proper interface.
 *  Calling it on a statement type without a context field causes
 *  an exit(2) and an error message.
 */

extern void MarkStatContext(struct Statement *s, unsigned int bits);
/**< 
 *  <!--  MarkStatContext(s,bits)                                      -->
 *  Adds the context bits given in bits to the context field of the
 *  statement given.
 *  E.g. to turn on the FOR context bit,
 *  MarkStatContext(s,context_FOR);
 *  This function should only be used at parse time.
 *  Unfortunately, until we have proof that the parser is err tight,
 *  we might also use this function at points in instantiation where
 *  we have proved that a statement is always going to be impossible.
 */

extern struct VariableList *GetStatVarList(CONST struct Statement *s);
/**< 
 *  <!--  struct VariableList *GetStatVarListF(s)                      -->
 *  <!--  const struct Statement *s;                                   -->
 *  Returns the variable list of a
 *  IS_A, IS_REFINED_TO, WILL_BE, WILL_BE_THE_SAME,
 *  ARE_ALIKE, ARE_THE_SAME, ALIASES or ALIASES-ISA(ARR) statement.
 *  It must be passed one of these types of statement.
 *  Other statements will return NULL or crash.
 */

/*== StateIs functions ==*/

#ifdef NDEBUG
#define GetStatType(s) ((s)->v.i.type)
#else
#define GetStatType(s) GetStatTypeF(s)
#endif
/**<
 *  Return the type field of an IS_A, WILL_BE, or IS_REFINED_TO statement.
 *  It must be passed one of these types of statement.
 *  Other statements will return NULL or crash.
 *  See also GetStatTypeDesc(), type_desc.h.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The type field as a symchar*.
 *  @see GetStatTypeF()
 */
extern symchar *GetStatTypeF(CONST struct Statement *s);
/**<
 *  <!--  macro GetStatType(s)                                         -->
 *  <!--  symchar *GetStatTypeF(s)                                     -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the type field of an IS_A, WILL_BE, or IS_REFINED_TO statement. -->
 *  <!--  It must be passed one of these types of statement.           -->
 *  <!--  Other statements will return NULL or crash.                  -->
 *  <!--  See also GetStatTypeDesc, type_desc.h.                       -->
 *  Implementation function for GetStatType().  Do not call this
 *  function directly - use GetStatType() instead.
 */

#ifdef NDEBUG
#define GetStatTypeArgs(s) ((s)->v.i.typeargs)
#else
#define GetStatTypeArgs(s) GetStatTypeArgsF(s)
#endif
/**<
 *  Return the typeargs field of an IS_A, WILL_BE or IS_REFINED_TO statement.
 *  It must be passed one of these types of statement.
 *  Proper WILL_BE statements should always return NULL.
 *  Other statements will return NULL or crash.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The typergs field as a CONST struct Set*.
 *  @see GetStatTypeArgsF()
 */
extern CONST struct Set *GetStatTypeArgsF(CONST struct Statement *s);
/**<
 *  <!--  macro GetStatTypeArgs(s)                                     -->
 *  <!--  const struct Set *GetStatTypeArgsF(s)                        -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the typeargs field of an IS_A, WILL_BE or IS_REFINED_TO statement. -->
 *  <!--  It must be passed one of these types of statement.           -->
 *  <!--  Proper WILLBE statements should always return NULL.          -->
 *  <!--  Other statements will return NULL or crash.                  -->
 *  Implementation function for GetStatTypeArgs().  Do not call this
 *  function directly - use GetStatTypeArgs() instead.
 */

extern unsigned int GetStatNeedsArgs(CONST struct Statement *s);
/**< 
 *  <!--  len = GetStatNeedsArgs(s);                                   -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  unsigned int len;                                            -->
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
/**<
 *  Return the type of set for an IS_A, WILL_BE.
 *  Often this will be NULL, which
 *  means that there is no "OF" part to the IS_A/WILL_BE.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The type as a symchar*.
 *  @see GetStatSetTypeF()
 */
extern symchar *GetStatSetTypeF(CONST struct Statement *s);
/**<
 *  <!--  macro GetStatSetType(s)                                      -->
 *  <!--  symchar *GetStatSetTypeF(s)                                  -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the type of set for an IS_A, WILL_BE.                 -->
 *  <!--  Often this will be NULL, which                               -->
 *  <!--  means that there is no "OF" part to the IS_A/WILL_BE.        -->
 *  Implementation function for GetStatSetType().  Do not call this
 *  function directly - use GetStatSetType() instead.
 */

#ifdef NDEBUG
#define GetStatCheckValue(s) ((s)->v.i.checkvalue)
#else
#define GetStatCheckValue(s) GetStatCheckValueF(s)
#endif
/**<
 *  Return the value expression for a WILLBE.  Often this will be NULL,
 *  which means that there is no WITH_VALUE part to the WILL_BE.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The expression as a CONST struct Expr*.
 *  @see GetStatCheckValueF()
 */
extern CONST struct Expr *GetStatCheckValueF(CONST struct Statement *s);
/**<
 *  <!--  macro GetStatCheckValue(s)                                   -->
 *  <!--  const struct Expr *GetStatCheckValueF(s)                     -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the value expression for a WILLBE.  Often this will be NULL, -->
 *  <!--  which means that there is no WITH_VALUE part to the WILL_BE. -->
 *  Implementation function for GetStatCheckValue().  Do not call this
 *  function directly - use GetStatCheckValue() instead.
 */

/* * * StateAlias functions * * */

#ifdef NDEBUG
#define AliasStatName(s) ((s)->v.ali.u.nptr)
#else
#define AliasStatName(s) AliasStatNameF(s)
#endif
/**<
 *  Return the Name RHS from a simple ALIASES statement.
 *  See stattypes.h for meaning of Name.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a CONST struct Name*.
 *  @see AliasStatNameF()
 */
extern CONST struct Name *AliasStatNameF(CONST struct Statement *s);
/**<
 *  <!--  macro AliasStatName(s)                                       -->
 *  <!--  struct Name *AliasStatNameF(s)                               -->
 *  <!--  Return the Name RHS from a simple ALIASES statement.         -->
 *  <!--  See stattypes.h for meaning of Name.                         -->
 *  Implementation function for AliasStatName().  Do not call this
 *  function directly - use AliasStatName() instead.
 */

#ifdef NDEBUG
#define ArrayStatAvlNames(s) ((s)->v.ali.u.avlname)
#else
#define ArrayStatAvlNames(s) ArrayStatAvlNamesF(s)
#endif
/**<
 *  Return the LHS (new array(s)) being defined from an ARR statement.
 *  See stattypes.h for meaning of vlist.
 *  syntax is currently:
 *  <pre>
 *  AvlNames ALIASES (VarList)
 *  WHERE SetName IS_A set OF IntSet WITH_VALUE (SetValues);
 *  </pre>
 *  while the standard ALIASES statement is just:
 *  <pre>
 *  VarList ALIASES Name;
 *  </pre>
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The LHS as a CONST struct VariableList*.
 *  @see ArrayStatAvlNamesF()
 */
extern CONST struct VariableList *ArrayStatAvlNamesF(CONST struct Statement *s);
/**<
 *  <!--  macro ArrayStatAvlNames(s)                                   -->
 *  <!--  CONST struct VariableList *ArrayStatAvlNamesF(s)             -->
 *  <!--  Return the LHS (new array(s)) being defined from an ARR statement. -->
 *  <!--  See stattypes.h for meaning of vlist.                        -->
 *  <!--  syntax is currently:                                         -->
 *
 *  <!--  AvlNames ALIASES (VarList)                                   -->
 *  <!--  WHERE SetName IS_A set OF IntSet WITH_VALUE (SetValues);     -->
 *
 *  <!--  while the standard ALIASES statement is just:                -->
 *
 *  <!--  VarList ALIASES Name;                                        -->
 *  Implementation function for ArrayStatAvlNames().  Do not call this
 *  function directly - use ArrayStatAvlNames() instead.
 */

#ifdef NDEBUG
#define ArrayStatSetName(s) ((s)->v.ali.c.setname)
#else
#define ArrayStatSetName(s) ArrayStatSetNameF(s)
#endif
/**<
 *  Return the name of set for an ARR statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a CONST struct VariableList*.
 *  @see ArrayStatSetNameF()
 */
extern CONST struct VariableList *ArrayStatSetNameF(CONST struct Statement *s);
/**<
 *  <!--  macro ArrayStatSetName(s)                                    -->
 *  <!--  struct VariableList *ArrayStatSetNameF(s)                    -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name of set for an ARR statement.                 -->
 *  Implementation function for ArrayStatSetName().  Do not call this
 *  function directly - use ArrayStatSetName() instead.
 */

#ifdef NDEBUG
#define ArrayStatIntSet(s) ((s)->v.ali.c.intset)
#else
#define ArrayStatIntSet(s) ArrayStatIntSetF(s)
#endif
/**<
 *  Return 0 or 1 corresponding to set being defined as symbol or integer.
 *  It must be passed an ARR statement.
 *  Other statements will return goop or crash.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The type code as an int.
 *  @see ArrayStatIntSetF()
 */
extern int ArrayStatIntSetF(CONST struct Statement *s);
/**<
 *  <!--  macro ArrayStatIntSet(s)                                     -->
 *  <!--  int ArrayStatIntSetF(s)                                      -->
 *  <!--  const struct Statement *s;                                   -->
 *
 *  <!--  Return 0 or 1 corresponding to set being defined as symbol or integer. -->
 *  <!--  It must be passed an ARR statement.                          -->
 *  <!--  Other statements will return goop or crash.                  -->
 *  Implementation function for ArrayStatIntSet().  Do not call this
 *  function directly - use ArrayStatIntSet() instead.
 */

#ifdef NDEBUG
#define ArrayStatSetValues(s) ((s)->v.ali.c.setvals)
#else
#define ArrayStatSetValues(s) ArrayStatSetValuesF(s)
#endif
/**<
 *  Return the user proposed set of values to use for array names.
 *  It must be passed an ARR statement. May be empty/NULL set.
 *  (this is the list after WITH_VALUE)
 *  Other statements will return goop or crash.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The set as a CONST struct Set*.
 *  @see ArrayStatSetValuesF()
 */
extern CONST struct Set *ArrayStatSetValuesF(CONST struct Statement *s);
/**<
 *  <!--  macro ArrayStatSetValues(s)                                  -->
 *  <!--  struct Set *ArrayStatSetValuesF(s)                           -->
 *  <!--  const struct Statement *s;                                   -->
 *
 *  <!--  Return the user proposed set of values to use for array names. -->
 *  <!--  It must be passed an ARR statement. May be empty/NULL set.   -->
 *  <!--  (this is the list after WITH_VALUE)                           -->
 *  <!--  Other statements will return goop or crash.                  -->
 *  Implementation function for ArrayStatSetValues().  Do not call this
 *  function directly - use ArrayStatSetValues() instead.
 */

/* * * StateFOR functions * * */

#ifdef NDEBUG
#define ForStatIndex(s) ((s)->v.f.index)
#else
#define ForStatIndex(s) ForStatIndexF(s)
#endif
/**<
 *  Return the FOR index variable identifier.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The identifier as a symchar*.
 *  @see ForStatIndexF()
 */
extern symchar *ForStatIndexF(CONST struct Statement *s);
/**<
 *  <!--  macro ForStatIndex(s)                                        -->
 *  <!--  symchar *ForStatIndexF(s)                                    -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the FOR index variable identifier.                    -->
 *  Implementation function for ForStatIndex().  Do not call this
 *  function directly - use ForStatIndex() instead.
 */

#ifdef NDEBUG
#define ForStatExpr(s) ((s)->v.f.e)
#else
#define ForStatExpr(s) ForStatExprF(s)
#endif
/**<
 *  Return the expression of values that the index variable takes.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The expression as a struct Expr*.
 *  @see ForStatExprF()
 */
extern struct Expr *ForStatExprF(CONST struct Statement *s);
/**<
 *  <!--  macro ForStatExpr(s)                                         -->
 *  <!--  struct Expr *ForStatExprF(s)                                 -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the expression of values that the index variable takes. -->
 *  Implementation function for ForStatExpr().  Do not call this
 *  function directly - use ForStatExpr() instead.
 */

#ifdef NDEBUG
#define ForStatStmts(s) ((s)->v.f.stmts)
#else
#define ForStatStmts(s) ForStatStmtsF(s)
#endif
/**<
 *  Return the list of statements that are in the body of the FOR construct.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct StatementList*.
 *  @see ForStatStmtsF()
 */
extern struct StatementList *ForStatStmtsF(CONST struct Statement *s);
/**<
 *  <!--  macro ForStatStmts(s)                                        -->
 *  <!--  struct StatementList *ForStatStmtsF(s)                       -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the list of statements that are in the body of the FOR construct. -->
 *  Implementation function for ForStatStmts().  Do not call this
 *  function directly - use ForStatStmts() instead.
 */

#ifdef NDEBUG
#define ForLoopKind(s) ((s)->v.f.kind)
#else
#define ForLoopKind(s) ForLoopKindF(s)
#endif
/**<
 *  Return the kind of the FOR construct.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The kind as an enum ForKind.
 *  @see ForLoopKindF()
 */
extern enum ForKind ForLoopKindF(CONST struct Statement *s);
/**<
 *  <!--  macro ForLoopKind(s)                                         -->
 *  <!--  enum ForOrder ForLoopKindF(s)                                -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the kind of the FOR construct.                        -->
 *  Implementation function for ForLoopKind().  Do not call this
 *  function directly - use ForLoopKind() instead.
 */

#ifdef NDEBUG
#define ForLoopOrder(s) ((s)->v.f.order)
#else
#define ForLoopOrder(s) ForLoopOrderF(s)
#endif
/**<
 *  Return the order of the FOR construct.  For declarative this should
 *  always be random.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The order as an enum ForOrder.
 *  @see ForLoopOrderF()
 */
extern enum ForOrder ForLoopOrderF(CONST struct Statement *s);
/**<
 *  <!--  macro ForLoopOrder(s)                                        -->
 *  <!--  enum ForOrder ForLoopOrderF(s)                               -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the order of the FOR construct.  For declarative this -->
 *  <!--  should always be random.                                     -->
 *  Implementation function for ForLoopOrder().  Do not call this
 *  function directly - use ForLoopOrder() instead.
 */

#ifdef NDEBUG
#define ForContains(s)             ((s)->v.f.contains)
#define ForContainsRelations(s)    ((s)->v.f.contains & contains_REL)
#define ForContainsLogRelations(s) ((s)->v.f.contains & contains_LREL)
#define ForContainsDefaults(s)     ((s)->v.f.contains & contains_DEF)
#define ForContainsCAssigns(s)     ((s)->v.f.contains & contains_CAS)
#define ForContainsWhen(s)         ((s)->v.f.contains & contains_WHEN)
#define ForContainsAlike(s)        ((s)->v.f.contains & contains_AA)
#define ForContainsAlias(s)        ((s)->v.f.contains & contains_ALI)
#define ForContainsArray(s)        ((s)->v.f.contains & contains_ARR)
#define ForContainsIsa(s)          ((s)->v.f.contains & contains_ISA)
#define ForContainsIrt(s)          ((s)->v.f.contains & contains_IRT)
#define ForContainsAts(s)          ((s)->v.f.contains & contains_ATS)
#define ForContainsWbts(s)         ((s)->v.f.contains & contains_WBTS)
#define ForContainsWnbts(s)        ((s)->v.f.contains & contains_WNBTS)
#define ForContainsWillbe(s)       ((s)->v.f.contains & contains_WILLBE)
#define ForContainsSelect(s)       ((s)->v.f.contains & contains_SELECT)
#define ForContainsConditional(s)  ((s)->v.f.contains & contains_COND)
#define ForContainsIllegal(s)      ((s)->v.f.contains & contains_ILL)
#else
#define ForContains(s)             ForContainsF(s)
#define ForContainsRelations(s)    ForContainsRelationsF(s)
#define ForContainsLogRelations(s) ForContainsLogRelationsF(s)
#define ForContainsDefaults(s)     ForContainsDefaultsF(s)
#define ForContainsCAssigns(s)     ForContainsCAssignsF(s)
#define ForContainsWhen(s)         ForContainsWhenF(s)
#define ForContainsAlike(s)        ForContainsAlikeF(s)
#define ForContainsAlias(s)        ForContainsAliasF(s)
#define ForContainsArray(s)        ForContainsArrayF(s)
#define ForContainsIsa(s)          ForContainsIsaF(s)
#define ForContainsIrt(s)          ForContainsIrtF(s)
#define ForContainsAts(s)          ForContainsAtsF(s)
#define ForContainsWbts(s)         ForContainsWbtsF(s)
#define ForContainsWnbts(s)        ForContainsWnbtsF(s)
#define ForContainsWillbe(s)       ForContainsWillbeF(s)
#define ForContainsSelect(s)       ForContainsSelectF(s)
#define ForContainsConditional(s)  ForContainsConditionalF(s)
#define ForContainsIllegal(s)      ForContainsIllegalF(s)
#endif
extern unsigned ForContainsF(CONST struct Statement *s);
extern unsigned ForContainsRelationsF(CONST struct Statement *s);
extern unsigned ForContainsLogRelationsF(CONST struct Statement *s);
extern unsigned ForContainsDefaultsF(CONST struct Statement *s);
extern unsigned ForContainsCAssignsF(CONST struct Statement *s);
extern unsigned ForContainsWhenF(CONST struct Statement *s);
extern unsigned ForContainsAlikeF(CONST struct Statement *s);
extern unsigned ForContainsAliasF(CONST struct Statement *s);
extern unsigned ForContainsArrayF(CONST struct Statement *s);
extern unsigned ForContainsIsaF(CONST struct Statement *s);
extern unsigned ForContainsIrtF(CONST struct Statement *s);
extern unsigned ForContainsAtsF(CONST struct Statement *s);
extern unsigned ForContainsWbtsF(CONST struct Statement *s);
extern unsigned ForContainsWnbtsF(CONST struct Statement *s);
extern unsigned ForContainsWillbeF(CONST struct Statement *s);
extern unsigned ForContainsSelectF(CONST struct Statement *s);
extern unsigned ForContainsConditionalF(CONST struct Statement *s);
extern unsigned ForContainsIllegalF(CONST struct Statement *s);
/**< 
 *  <pre>
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
 *  </pre>
 *  @todo Rework documentation for individual functions.
 */

/* * * StateAssign functions * * */

/*
 *  Assign has been split into Assign (structural) and Default
 *  (variable) assignment types.
 */
#ifdef NDEBUG
#define DefaultStatVar(sptr) ((sptr)->v.asgn.nptr)
#else
#define DefaultStatVar(sptr) DefaultStatVarF(sptr)
#endif
/**<
 *  Return the name structure which is the left hand side of the
 *  default assignment.
 *  @param sptr CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see DefaultStatVarF()
 */
extern struct Name *DefaultStatVarF(CONST struct Statement *s);
/**<
 *  <!--  macro DefaultStatVar(s)                                      -->
 *  <!--  struct Name *DefaultStatVarF(s)                              -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name structure which is the left hand side of the -->
 *  <!--  default assignment.                                          -->
 *  Implementation function for DefaultStatVar().  Do not call this
 *  function directly - use DefaultStatVar() instead.
 */

#ifdef NDEBUG
#define DefaultStatRHS(s) ((s)->v.asgn.rhs)
#else
#define DefaultStatRHS(s) DefaultStatRHSF(s)
#endif
/**<
 *  Return the right hand side expression of the default assignment.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The expression as a struct Expr*.
 *  @see DefaultStatRHSF()
 */
extern struct Expr *DefaultStatRHSF(CONST struct Statement *s);
/**<
 *  <!--  macro DefaultStatRHS(s)                                      -->
 *  <!--  struct Expr *DefaultStatRHSF(s)                              -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the right hand side expression of the default assignment. -->
 *  Implementation function for DefaultStatRHS().  Do not call this
 *  function directly - use DefaultStatRHS() instead.
 */

#ifdef NDEBUG
#define AssignStatVar(sptr) ((sptr)->v.asgn.nptr)
#else
#define AssignStatVar(sptr) AssignStatVarF(sptr)
#endif
/**<
 *  Return the name structure which is the left hand side of the
 *  structural assignment.
 *  @param sptr CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see AssignStatVarF()
 */
extern struct Name *AssignStatVarF(CONST struct Statement *s);
/**<
 *  <!--  macro AssignStatVar(s)                                       -->
 *  <!--  struct Name *AssignStatVarF(s)                               -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name structure which is the left hand side of the -->
 *  <!--  structural assignment.                                       -->
 *  Implementation function for AssignStatVar().  Do not call this
 *  function directly - use AssignStatVar() instead.
 */

#ifdef NDEBUG
#define AssignStatRHS(s) ((s)->v.asgn.rhs)
#else
#define AssignStatRHS(s) AssignStatRHSF(s)
#endif
/**<
 *  Return the right hand side expression of the structural assignment.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The expression as a struct Expr*.
 *  @see AssignStatRHSF()
 */
extern struct Expr *AssignStatRHSF(CONST struct Statement *s);
/**<
 *  <!--  macro AssignStatRHS(s)                                       -->
 *  <!--  struct Expr *AssignStatRHSF(s)                               -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the right hand side expression of the structural assignment. -->
 *  Implementation function for AssignStatRHS().  Do not call this
 *  function directly - use AssignStatRHS() instead.
 */

/* * * StateRelation functions * * */

#ifdef NDEBUG
#define RelationStatName(s) ((s)->v.rel.nptr)
#else
#define RelationStatName(s) RelationStatNameF(s)
#endif
/**<
 *  Return the name of the relation.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see RelationStatNameF()
 */
extern struct Name *RelationStatNameF(CONST struct Statement *s);
/**<
 *  <!--  macro RelationStatName(s)                                    -->
 *  <!--  struct Name *RelationStatNameF(s)                            -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name of the relation.                             -->
 *  Implementation function for RelationStatName().  Do not call this
 *  function directly - use RelationStatName() instead.
 */

#ifdef NDEBUG
#define RelationStatExpr(s) ((s)->v.rel.relation)
#else
#define RelationStatExpr(s) RelationStatExprF(s)
#endif
/**<
 *  Return the relation expression.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The expression as a struct Expr*.
 *  @see RelationStatExprF()
 */
extern struct Expr *RelationStatExprF(CONST struct Statement *s);
/**<
 *  <!--  macro RelationStatExpr(s)                                    -->
 *  <!--  struct Expr *RelationStatExprF(s)                            -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the relation expression.                              -->
 *  Implementation function for RelationStatExpr().  Do not call this
 *  function directly - use RelationStatExpr() instead.
 */

#ifdef NDEBUG
#define LogicalRelStatName(s) ((s)->v.lrel.nptr)
#else
#define LogicalRelStatName(s) LogicalRelStatNameF(s)
#endif
/**<
 *  Return the name of the logical relation.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see LogicalRelStatNameF()
 */
extern struct Name *LogicalRelStatNameF(CONST struct Statement *s);
/**<
 *  <!--  macro LogicalRelStatName(s)                                  -->
 *  <!--  struct Name *LogicalRelStatNameF(s)                          -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name of the logical relation.                     -->
 *  Implementation function for LogicalRelStatName().  Do not call this
 *  function directly - use LogicalRelStatName() instead.
 */

#ifdef NDEBUG
#define LogicalRelStatExpr(s) ((s)->v.lrel.logrel)
#else
#define LogicalRelStatExpr(s) LogicalRelStatExprF(s)
#endif
/**<
 *  Return the logical relation expression.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The expression as a struct Expr*.
 *  @see RelationStatExprF()
 */
extern struct Expr *LogicalRelStatExprF(CONST struct Statement *s);
/**<
 *  <!--  macro LogicalRelStatExpr(s)                                  -->
 *  <!--  struct Expr *LogicalRelStatExprF(s)                          -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the logical relation expression.                      -->
 *  Implementation function for LogicalRelStatExpr().  Do not call this
 *  function directly - use LogicalRelStatExpr() instead.
 */

/* * * StateExternal functions * * */

#ifdef NDEBUG
#define ExternalStatMode(s) ((s)->v.ext.mode)
#else
#define ExternalStatMode(s) ExternalStatModeF(s)
#endif
/**<
 *  Return the external statement's mode.
 *  0 = Procedural, 1 = Glassbox Declarative, 2 = Blackbox Declarative.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The mode as an int.
 *  @see ExternalStatModeF()
 */
extern int ExternalStatModeF(CONST struct Statement *s);
/**<
 *  <!--  macro ExternalStatMode(s)                                    -->
 *  <!--  int ExternalStatModeF(s)                                     -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the external statement's mode.                        -->
 *  <!--  0 = Procedural, 1 = Glassbox Declarative, 2 = Blackbox Declarative. -->
 *  Implementation function for ExternalStatMode().  Do not call this
 *  function directly - use ExternalStatMode() instead.
 */

#ifdef NDEBUG
#define ExternalStatName(s) ((s)->v.ext.nptr)
#else
#define ExternalStatName(s) ExternalStatNameF(s)
#endif
/**<
 *  Return the external statement's name.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see ExternalStatNameF()
 */
extern struct Name *ExternalStatNameF(CONST struct Statement *s);
/**<
 *  <!--  macro ExternalStatName(s)                                    -->
 *  <!--  struct Name *ExternalStatNameF(s)                            -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the external statement's name.                        -->
 *  Implementation function for ExternalStatName().  Do not call this
 *  function directly - use ExternalStatName() instead.
 */

#ifdef NDEBUG
#define ExternalStatData(s) ((s)->v.ext.data)
#else
#define ExternalStatData(s) ExternalStatDataF(s)
#endif
/**<
 *  Return the external statement's name of its additional data.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see ExternalStatDataF()
 */
extern struct Name *ExternalStatDataF(CONST struct Statement *s);
/**<
 *  <!--  macro ExternalStatData(s)                                    -->
 *  <!--  struct Name *ExternalStatDataF(s)                            -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the external statement's name of its additional data. -->
 *  Implementation function for ExternalStatData().  Do not call this
 *  function directly - use ExternalStatData() instead.
 */

#ifdef NDEBUG
#define ExternalStatScope(s) ((s)->v.ext.scope)
#else
#define ExternalStatScope(s) ExternalStatScopeF(s)
#endif
/**<
 *  Return the external statement's name of its scope. This may be NULL.
 *  If it is NULL, then the external relation (at this time assumed to be
 *  a glassbox), will added at the scope that it was found; otherwise it
 *  will be added at the given scope.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see ExternalStatScopeF()
 */
extern struct Name *ExternalStatScopeF(CONST struct Statement *s);
/**<
 *  <!--  macro ExternalStatScope(s)                                   -->
 *  <!--  struct Name *ExternalStatScopeF(s)                           -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the external statement's name of its scope. This may be NULL.  -->
 *  <!--  If it is NULL, then the external relation (at this time assumed to be -->
 *  <!--  a glassbox, will added at the scope that it was found; otherwise it   -->
 *  <!--  will be added at the given scope.                            -->
 *  Implementation function for ExternalStatScope().  Do not call this
 *  function directly - use ExternalStatScope() instead.
 */

#ifdef NDEBUG
#define ExternalStatVlist(s) ((s)->v.ext.vl)
#else
#define ExternalStatVlist(s) ExternalStatVlistF(s)
#endif
/**<
 *  Return the external statement's variable list -- the argument list.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct VariableList*.
 *  @see ExternalStatVlistF()
 */
extern struct VariableList *ExternalStatVlistF(CONST struct Statement *s);
/**<
 *  <!--  macro ExternalStatVlist(s)                                   -->
 *  <!--  struct VariableList *ExternalStatVlist(s)                    -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the external statement's variable list -- the argument list. -->
 *  Implementation function for ExternalStatVlist().  Do not call this
 *  function directly - use ExternalStatVlist() instead.
 */

#ifdef NDEBUG
#define ExternalStatFuncName(s) ((s)->v.ext.extcall)
#else
#define ExternalStatFuncName(s) ExternalStatFuncNameF(s)
#endif
/**<
 *  Return the external statement's function call
 *  name. -- the name invoked with.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a CONST char*.
 *  @see ExternalStatFuncNameF()
 */
extern CONST char *ExternalStatFuncNameF(CONST struct Statement *s);
/**<
 *  <!--  macro ExternalStatFuncName(s)                                -->
 *  <!--  struct Name *ExternalStatFuncNameF(s)                        -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the external statement's function call                -->
 *  <!--  name. -- the name invoked with.                              -->
 *  Implementation function for ExternalStatFuncName().  Do not call this
 *  function directly - use ExternalStatFuncName() instead.
 */

/* * * StateReference functions * * */

#ifdef NDEBUG
#define ReferenceStatMode(s) ((s)->v.ref.mode)
#else
#define ReferenceStatMode(s) ReferenceStatModeF(s)
#endif
/**<
 *  Return the mode of the refernce statement. Mode at the moment:
 *  0 => copy/clone semantics; 1 => pure reference semantics, such as in
 *  languages with pointers. Value semantics is handled by ISA statements
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The mode as an int.
 *  @see ReferenceStatModeF()
 */
extern int ReferenceStatModeF(CONST struct Statement *s);
/**<
 *  <!--  macro ReferenceStatMode(s)                                   -->
 *  <!--  int ReferenceStatModeF(s)                                    -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the mode of the refernce statement. Mode at the moment:        -->
 *  <!--  0 => copy/clone semantics; 1 => pure reference semantics, such as in  -->
 *  <!--  languages with pointers. Value semantics is handled by ISA statements -->
 *  Implementation function for ReferenceStatMode().  Do not call this
 *  function directly - use ReferenceStatMode() instead.
 */

#ifdef NDEBUG
#define ReferenceStatName(s) ((s)->v.ref.ref_name)
#else
#define ReferenceStatName(s) ReferenceStatNameF(s)
#endif
/**<
 *  Return the name of the thing being referred to. This name is a simple
 *  such as a type description. Later this may become accept a fully qulafied
 *  id., and will depend upon the semantics of how it was invoked.
 *  This name may alternatively be called the *referee*
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a symchar*.
 *  @see ReferenceStatNameF()
 */
extern symchar *ReferenceStatNameF(CONST struct Statement *s);
/**<
 *  <!--  macro ReferenceStatName(s)                                   -->
 *  <!--  const char *ReferenceStatNameF(s)                            -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name of the thing being referred to. This name is a simple     -->
 *  <!--  such as a type description. Later this may become accept a fully qulafied -->
 *  <!--  id., and will depend upon the semantics of how it was invoked.            -->
 *  <!--  This name may alternatively be called the *referee*          -->
 *  Implementation function for ReferenceStatName().  Do not call this
 *  function directly - use ReferenceStatName() instead.
 */

#ifdef NDEBUG
#define ReferenceStatSetType(s) ((s)->v.ref.settype)
#else
#define ReferenceStatSetType(s) ReferenceStatSetTypeF(s)
/**<
 *  Return the type of set for a REF statement. This may be NULL, which
 *  means that there is no "OF" part to the _IS_ statement.<br>
 *  Eg:   foo[thing] _IS_ set OF barobject;  -- not NULL    <br>
 *        foo_other  _IS_ bar_other;         -- NULL.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The type as a symchar*.
 *  @see ReferenceStatSetTypeF()
 */
extern symchar *ReferenceStatSetTypeF(CONST struct Statement *s);
#endif
/**<
 *  <!--  macro ReferenceStatSetType(s)                                -->
 *  <!--  symchar *ReferenceStatSetType(s)                             -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the type of set for a REF statement. This may be NULL, which -->
 *  <!--  means that there is no "OF" part to the _IS_ statement.      -->
 *  <!--  Eg: 	foo[thing] _IS_ set OF barobject;	-- not NULL          -->
 *  <!--            foo_other  _IS_ bar_other;		-- NULL.             -->
 *  Implementation function for ReferenceStatSetType().  Do not call this
 *  function directly - use ReferenceStatSetType() instead.
 */

#ifdef NDEBUG
#define ReferenceStatVlist(s) ((s)->v.ref.vl)
#else
#define ReferenceStatVlist(s) ReferenceStatVlistF(s)
#endif
/**<
 *  Return the list of variables/models that refer to the given reference.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct VariableList*.
 *  @see ReferenceStatVlistF()
 */
extern struct VariableList *ReferenceStatVlistF(CONST struct Statement *s);
/**<
 *  <!--  macro ReferenceStatVlist(s)                                  -->
 *  <!--  struct VariableList *ReferenceStatVlistF(s)                  -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the list of variables/models that refer to the given reference. -->
 *  Implementation function for ReferenceStatVlist().  Do not call this
 *  function directly - use ReferenceStatVlist() instead.
 */

/* * * StateRun functions * * */

#ifdef NDEBUG
#define RunStatName(s) ((s)->v.r.proc_name)
#else
#define RunStatName(s) RunStatNameF(s)
#endif
/**<
 *  Returns the name pointer of a run statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see RunStatNameF()
 */
extern struct Name *RunStatNameF(CONST struct Statement *s);
/**<
 *  <!--  macro RunStatName(s)                                         -->
 *  <!--  struct Name *RunStatNameF(s)                                 -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Returns the name pointer of a run statement.                 -->
 *  Implementation function for RunStatName().  Do not call this
 *  function directly - use RunStatName() instead.
 */

#ifdef NDEBUG
#define RunStatAccess(s) ((s)->v.r.type_name)
#else
#define RunStatAccess(s) RunStatAccessF(s)
#endif
/**<
 *  Returns the type access of a RUN statement. This is to allow the
 *  use of syntax such as: RUN A::b where 'B' is a class/model/atom, and b
 *  is a name of procedure within that class/atom/model.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The type access as a struct Name*.
 *  @see RunStatAccessF()
 */
extern struct Name *RunStatAccessF(CONST struct Statement *s);
/**<
 *  <!--  macro RunStatAccess(s)                                       -->
 *  <!--  char *RunStatAccessF(s)                                      -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Returns the type access of a RUN statement. This is to allow the       -->
 *  <!--  use of syntax such as: RUN A::b where 'B' is a class/model/atom, and b -->
 *  <!--  is a name of procedure within that class/atom/model.         -->
 *  Implementation function for RunStatAccess().  Do not call this
 *  function directly - use RunStatAccess() instead.
 */

#ifdef NDEBUG
# define FixStatVars(s) ((s)->v.fx.vars)
#else
# define FixStatVars(s) FixStatVarsF(s)
#endif
/**<
	Returns the variable list for a FIX statement
	@param s CONST struct Statement*, the statement to query.
	@return struct VariableList*, the list of variables
	@see FixStatAccessF
*/

extern struct VariableList *FixStatVarsF(CONST struct Statement *s);
/**<
	Implementation function for FixStatVars(). Do not call this directory, use FixStatVars instead.
*/

/* * * StateCall functions * * */

#ifdef NDEBUG
#define CallStatArgs(s) ((s)->v.call.args)
#else
#define CallStatArgs(s) CallStatArgsF(s)
#endif
/**<
 *  Return the call statement's argument list.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct Set*.
 *  @see CallStatArgsF()
 */
extern struct Set *CallStatArgsF(CONST struct Statement *s);
/**<
 *  <!--  macro CallStatArgs(s)                                        -->
 *  <!--  struct Set *CallStatArgs(s)                                  -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the call statement's argument list.                   -->
 *  Implementation function for CallStatArgs().  Do not call this
 *  function directly - use CallStatArgs() instead.
 */

#ifdef NDEBUG
#define CallStatId(s) ((s)->v.call.id)
#else
#define CallStatId(s) CallStatIdF(s)
#endif
/**<
 *  Return the call statement's function id.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The id as a symchar*.
 *  @see CallStatIdF()
 */
extern symchar *CallStatIdF(CONST struct Statement *s);
/**<
 *  <!--  macro CallStatId(s)                                          -->
 *  <!--  symchar *CallStatId(s)                                       -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the call statement's function id.                     -->
 *  Implementation function for CallStatId().  Do not call this
 *  function directly - use CallStatId() instead.
 */

/* * * StateWhile functions * * */

#ifdef NDEBUG
#define WhileStatExpr(s) ((s)->v.loop.test)
#else
#define WhileStatExpr(s) WhileStatExprF(s)
#endif
/**<
 *  Return the WHILE expression.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The expression as a struct Expr*.
 *  @see WhileStatExprF()
 */
extern struct Expr *WhileStatExprF(CONST struct Statement *s);
/**<
 *  <!--  macro WhileStatExpr(s)                                       -->
 *  <!--  struct Expr *WhileStatExprF(s)                               -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the WHILE expression.                                 -->
 *  Implementation function for WhileStatExpr().  Do not call this
 *  function directly - use WhileStatExpr() instead.
 */

#ifdef NDEBUG
#define WhileStatBlock(s) ((s)->v.loop.block)
#else
#define WhileStatBlock(s) WhileStatBlockF(s)
#endif
/**<
 *  Return the block of a while statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The block as a struct StatementList*.
 *  @see WhileStatBlockF()
 */
extern struct StatementList *WhileStatBlockF(CONST struct Statement *s);
/**<
 *  <!--  macro WhileStatBlock(s)                                      -->
 *  <!--  struct StatementList *WhileStatBlockF(s)                     -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the block of a while statement.                       -->
 *  Implementation function for WhileStatBlock().  Do not call this
 *  function directly - use WhileStatBlock() instead.
 */

/* * * stateflow * * */

#define FlowStatControl(s) ((s)->v.flow.fc)
/**< 
 * Return the type of flow control. 
 * @todo Needs a function wrapper. 
 */

#define FlowStatMessage(s) ((s)->v.flow.message)
/**< 
 * Return the message of the flow control.
 * @todo Needs a function wrapper.
 */

/* * * StateASSERT functions * * */

/** Return the ASSERT expression 
	@param s the expression to test
	@see AssertStatExprF()
*/
#ifdef NDEBUG
# define AssertStatExpr(s) ((s)->v.asserts.test)
#else
# define AssertStatExpr(s) AssertStatExprF(s)
#endif

/** Internal implemention of ASSERT expression
	@see IfStatExprF()
*/
extern struct Expr *AssertStatExprF(CONST struct Statement *s);

/* * * StateIf functions * * */

#ifdef NDEBUG
#define IfStatExpr(s) ((s)->v.ifs.test)
#else
#define IfStatExpr(s) IfStatExprF(s)
#endif
/**<
 *  Return the IF expression.
 *  @param s the statement to query.
 *  @return The expression as a struct Expr*.
 *  @see IfStatExprF()
 */
extern struct Expr *IfStatExprF(CONST struct Statement *s);
/**<
 *  <!--  macro IfStatExpr(s)                                          -->
 *  <!--  struct Expr *IfStatExprF(s)                                  -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the IF expression.                                    -->
 *  Implementation function for IfStatExpr().  Do not call this
 *  function directly - use IfStatExpr() instead.
 */

#ifdef NDEBUG
#define IfStatThen(s) ((s)->v.ifs.thenblock)
#else
#define IfStatThen(s) IfStatThenF(s)
#endif
/**<
 *  Return the then block of an if statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The block as a struct StatementList*.
 *  @see IfStatThenF()
 */
extern struct StatementList *IfStatThenF(CONST struct Statement *s);
/**<
 *  <!--  macro IfStatThen(s)                                          -->
 *  <!--  struct StatementList *IfStatThenF(s)                         -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the then block of an if statement.                    -->
 *  Implementation function for IfStatThen().  Do not call this
 *  function directly - use IfStatThen() instead.
 */

#ifdef NDEBUG
#define IfStatElse(s) ((s)->v.ifs.elseblock)
#else
#define IfStatElse(s) IfStatElseF(s)
#endif
/**<
 *  Return the else block of an if statement.  This is NULL if the else
 *  was omitted.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The block as a struct StatementList*.
 *  @see IfStatElseF()
 */
extern struct StatementList *IfStatElseF(CONST struct Statement *s);
/**<
 *  <!--  macro IfStatElse(s)                                          -->
 *  <!--  struct StatementList *IfStatElseF(s)                         -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the else block of an if statement.                    -->
 *  <!--  This is NULL if the else was omitted.                        -->
 *  Implementation function for IfStatElse().  Do not call this
 *  function directly - use IfStatElse() instead.
 */

/* * * StateWhen functions * * */

#ifdef NDEBUG
#define WhenStatName(s) ((s)->v.w.nptr)
#else
#define WhenStatName(s) WhenStatNameF(s)
#endif
/**<
 *  Return the name of a WHEN node.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see WhenStatNameF()
 */
extern struct Name *WhenStatNameF(CONST struct Statement *s);
/**<
 *  <!--  macro WhenStatName(s)                                        -->
 *  <!--  struct Name *WhenStatName(s)                                 -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name of a WHEN node.                              -->
 *  Implementation function for WhenStatName().  Do not call this
 *  function directly - use WhenStatName() instead.
 */

extern void SetWhenName(struct Statement *s, struct Name *n);
/**<
 *  <!--  void SetWhenName(s,n)                                        -->
 *  <!--  struct Statement *s;                                         -->
 *  <!--  struct Name *n;                                              -->
 *  This procedure provides a mechanism for unnamed whens to be named.
 */

#ifdef NDEBUG
#define WhenStatVL(s) ((s)->v.w.vl)
#else
#define WhenStatVL(s) WhenStatVLF(s)
#endif
/**<
 *  Return the variable list part of a WHEN statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct VariableList*.
 *  @see WhenStatVLF()
 */
extern struct VariableList *WhenStatVLF(CONST struct Statement *s);
/**<
 *  <!--  macro WhenStatVL(s)                                          -->
 *  <!--  struct VariableList *WhenStatVLF(s)                          -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the variable list part of a WHEN statement.           -->
 *  Implementation function for WhenStatVL().  Do not call this
 *  function directly - use WhenStatVL() instead.
 */

#ifdef NDEBUG
#define WhenStatCases(s) ((s)->v.w.cases)
#else
#define WhenStatCases(s) WhenStatCasesF(s)
#endif
/**<
 *  Return the list of cases of a WHEN statement. To obtain the
 *  statement list in each of the CASEs, see when.h
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct WhenList*.
 *  @see WhenStatCasesF()
 */
extern struct WhenList *WhenStatCasesF(CONST struct Statement *s);
/**<
 *  <!--  macro WhenStatCases(s)                                       -->
 *  <!--  struct WhenList *WhenStatCasesF(s)                           -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the list of cases of a WHEN statement. To obtain the  -->
 *  <!--  statement list in each of the CASEs, see when.h              -->
 *  Implementation function for WhenStatCases().  Do not call this
 *  function directly - use WhenStatCases() instead.
 */

extern int CompareWhenStatements(CONST struct Statement *s1,
                                 CONST struct Statement *s2);
/**<
 *  Compare functions for WHEN statements. It includes the decomposition
 *  of the WHEN in the list of variables and the list of CASEs. Also,
 *  each case is decomposed in the set of values and the list of statements.
 *  It is done here since we are comparing the statement rather than only a
 *  WhenList structure (when.[ch]).
 *  It is called for the function CompareStatements and it follows the
 *  same pattern for the comparation.
 */

/* * * StateFname functions * * */

#ifdef NDEBUG
#define FnameStat(s) ((s)->v.n.wname)
#else
#define FnameStat(s) FnameStatF(s)
#endif
/**<
 *  Return the name of a model or relation (inside WHEN statement)
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The name as a struct Name*.
 *  @see FnameStatF()
 */
extern struct Name *FnameStatF(CONST struct Statement *s);
/**<
 *  <!--  macro FnameStatcases(s)                                      -->
 *  <!--  struct Name *FnameStatF(s)                                   -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the name of a model or relation (inside WHEN statement) -->
 *  Implementation function for FnameStat().  Do not call this
 *  function directly - use FnameStat() instead.
 */

/* * * StateCOND functions * * */

#ifdef NDEBUG
#define CondStatList(s) ((s)->v.cond.stmts)
#else
#define CondStatList(s) CondStatListF(s)
#endif
/**<
 *  Return the list of statements in a Conditional statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct StatementList*.
 *  @see CondStatListF()
 */
extern struct StatementList *CondStatListF(CONST struct Statement *s);
/**<
 *  <!--  macro CondStatList(s)                                        -->
 *  <!--  struct StatementList *CondStatListF(s)                       -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the list of statements in a Conditional statement.    -->
 *  Implementation function for CondStatList().  Do not call this
 *  function directly - use CondStatList() instead.
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
extern unsigned CondContainsF(CONST struct Statement *s);
extern unsigned CondContainsRelationsF(CONST struct Statement *s);
extern unsigned CondContainsLogRelationsF(CONST struct Statement *s);
/**<
 *  <pre>
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
 *  </pre>
 *  @todo Rework documentation for individual functions.
 */

/* * * StateSelect functions * * */

#ifdef NDEBUG
#define SelectStatVL(s) ((s)->v.se.vl)
#else
#define SelectStatVL(s) SelectStatVLF(s)
#endif
/**<
 *  Return the set part of a SELECT statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The set part as a struct VariableList*.
 *  @see SelectStatVLF()
 */
extern struct VariableList *SelectStatVLF(CONST struct Statement *s);
/**<
 *  <!--  macro SelectStatVL(s)                                        -->
 *  <!--  struct VariableList *SelectStatVLF(s)                        -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the set part of a SELECT statement.                   -->
 *  Implementation function for SelectStatVL().  Do not call this
 *  function directly - use SelectStatVL() instead.
 */

#ifdef NDEBUG
#define SelectStatNumberStats(s) ((s)->v.se.n_statements)
#else
#define SelectStatNumberStats(s) SelectStatNumberStatsF(s)
#endif
/**<
 *  Return the number of statements inside a SELECT statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The statement count as an int.
 *  @see SelectStatNumberStatsF()
 */
extern int SelectStatNumberStatsF(CONST struct Statement *s);
/**<
 *  <!--  macro SelectStatNumberStats(s)                               -->
 *  <!--  int SelectStatNumberStatsF(s)                                -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the number of statements inside a SELECT statement.   -->
 *  Implementation function for SelectStatNumberStats().  Do not call this
 *  function directly - use SelectStatNumberStats() instead.
 */

#ifdef NDEBUG
#define SelectStatCases(s) ((s)->v.se.cases)
#else
#define SelectStatCases(s) SelectStatCasesF(s)
#endif
/**<
 *  Return the list of cases of a SELECT statement. To obtain the
 *  statement list in each of the CASEs, see select.h
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct SelectList*.
 *  @see SelectStatCasesF()
 */
extern struct SelectList *SelectStatCasesF(CONST struct Statement *s);
/**<
 *  <!--  macro SelectStatCases(s)                                     -->
 *  <!--  struct SelectList *SelectStatCasesF(s)                       -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the list of cases of a SELECT statement. To obtain the -->
 *  <!--  statement list in each of the CASEs, see select.h            -->
 *  Implementation function for SelectStatCases().  Do not call this
 *  function directly - use SelectStatCases() instead.
 */

extern int CompareSelectStatements(CONST struct Statement *s1,
                                   CONST struct Statement *s2);

/**<
 *  Compare functions for SELECT statements. It includes the decomposition
 *  of the SELECT in the list of variables and the list of CASEs. Also,
 *  each case is decomposed in the set of values and the list of statements.
 *  It is done here since we are comparing the statement rather than only a
 *  SelectList structure (select.[ch]).<br><br>
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
extern unsigned SelectContainsF(CONST struct Statement *s);
extern unsigned SelectContainsRelationsF(CONST struct Statement *s);
extern unsigned SelectContainsLogRelationsF(CONST struct Statement *s);
extern unsigned SelectContainsDefaultsF(CONST struct Statement *s);
extern unsigned SelectContainsCAssignsF(CONST struct Statement *s);
extern unsigned SelectContainsWhenF(CONST struct Statement *s);
extern unsigned SelectContainsAlikeF(CONST struct Statement *s);
extern unsigned SelectContainsAliasF(CONST struct Statement *s);
extern unsigned SelectContainsArrayF(CONST struct Statement *s);
extern unsigned SelectContainsIsaF(CONST struct Statement *s);
extern unsigned SelectContainsIrtF(CONST struct Statement *s);
extern unsigned SelectContainsAtsF(CONST struct Statement *s);
extern unsigned SelectContainsWbtsF(CONST struct Statement *s);
extern unsigned SelectContainsWnbtsF(CONST struct Statement *s);
extern unsigned SelectContainsWillbeF(CONST struct Statement *s);
extern unsigned SelectContainsSelectF(CONST struct Statement *s);
extern unsigned SelectContainsConditionalF(CONST struct Statement *s);
extern unsigned SelectContainsIllegalF(CONST struct Statement *s);
/**< 
 *  <pre>
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
 *  </pre>
 *  @todo Rework documentation for individual functions.
 */

/*==============================*/

/* * * StateSwitch functions * * */

#ifdef NDEBUG
#define SwitchStatVL(s) ((s)->v.sw.vl)
#else
#define SwitchStatVL(s) SwitchStatVLF(s)
#endif
/**<
 *  Return the set part of a SWITCH statement.
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The set part as a struct VariableList*.
 *  @see SwitchStatVLF()
 */
extern struct VariableList *SwitchStatVLF(CONST struct Statement *s);
/**<
 *  <!--  macro SwitchStatVL(s)                                        -->
 *  <!--  struct VariableList *SwitchStatVLF(s)                        -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the set part of a SWITCH statement.                   -->
 *  Implementation function for SwitchStatVL().  Do not call this
 *  function directly - use SwitchStatVL() instead.
 */

#ifdef NDEBUG
#define SwitchStatCases(s) ((s)->v.sw.cases)
#else
#define SwitchStatCases(s) SwitchStatCasesF(s)
#endif
/**<
 *  Return the list of cases of a SWITCH statement. To obtain the
 *  statement list in each of the CASEs, see switch.h
 *  @param s CONST struct Statement*, the statement to query.
 *  @return The list as a struct SwitchList*.
 *  @see SwitchStatCasesF()
 */
extern struct SwitchList *SwitchStatCasesF(CONST struct Statement *s);
/**<
 *  <!--  macro SwitchStatCases(s)                                     -->
 *  <!--  struct SwitchList *SwitchStatCasesF(s)                       -->
 *  <!--  const struct Statement *s;                                   -->
 *  <!--  Return the list of cases of a SWITCH statement. To obtain the -->
 *  <!--  statement list in each of the CASEs, see switch.h            -->
 *  Implementation function for SwitchStatCases().  Do not call this
 *  function directly - use SwitchStatCases() instead.
 */

extern int CompareSwitchStatements(CONST struct Statement *s1,
                                   CONST struct Statement *s2);
/**<
 *  Compare functions for SWITCH statements. It includes the decomposition
 *  of the Switch in the list of variables and the list of CASEs. Also,
 *  each case is decomposed in the set of values and the list of statements.
 *  It is done here since we are comparing the statement rather than only a
 *  SwitchList structure (switch.[ch]).<br><br>
 *
 *  It is called for the function CompareStatements and it follows the
 *  same pattern for the comparation.
 */

/*====================================*/

extern int CompareStatements(CONST struct Statement *s1,
                             CONST struct Statement *s2);
/**<
 *  <!--  CompareStatements(s1,s2);                                    -->
 *  Returns -1,0,1 as s1 is <, ==, > s2.
 *  s1 < s2 if type(s1) < type(s2), exception: NULL > all statements.
 *  For statements of the same type, compared according to
 *  the number of arguments or alphabetically, as appropriate.
 *  We are comparing statement contents, not statement memory location
 *  or origin.<br><br>
 *  Special notes:
 *  For a less picky version (when it comes to type compatibility)
 *  see CompareISStatements below.
 */

extern int CompareISStatements(CONST struct Statement *s1,
                               CONST struct Statement *s2);
/**<
 *  <!--  CompareISStatements(s1,s2);                                  -->
 *  s1 and s2 must be StateIS or some of the StateARE based statements, i.e.
 *  IS_A WILL_BE IS_REFINED_TO WILL_BE_THE_SAME statements, or
 *  s1 and s2 must be relations.
 *  FOR loops containing only these kinds are also allowed.
 *  Returns -1,0,1 as s1 is <, ==, > s2.
 *  s1 < s2 if StatementType(s1) < StatementType(s2),
 *  exception: NULL > all statements.<br><br>
 *
 *  For statements of the same StatementType, compared according to
 *  the number of arguments or alphabetically, as appropriate
 *  after type compatibility is checked. In particular,
 *  if MoreRefined(GetStateType(s1),GetStatType(s2)) returns
 *  the type from s2, the two potentially type differing statements
 *  which OTHERWISE are identical are considered identical.
 *  If s1 is more refined than s2, the statements are considered
 *  incompatible. If s1 has a WITH_VALUE clause that does not match
 *  s2, the statements are incompatible.<br><br>
 *
 *  We are comparing statement contents, not statement memory location
 *  or origin.<br><br>
 *
 *  Special notes:
 *  For a more picky version (when it comes to type compatibility)
 *  see CompareStatements above.
 */

#endif /* __STATEMENT_H_SEEN__ */

