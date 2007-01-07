/*
 *  Ascend Type Definition Lint Module
 *  by Benjamin Andrew Allan
 *  Created: 9/16/96
 *  Version: $Revision: 1.27 $
 *  Version control file: $RCSfile: typelint.h,v $
 *  Date last modified: $Date: 1998/04/21 23:50:10 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Benjamin Andrew Allan
 *
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
 */

/** @file
 *  Ascend Type Definition Lint Module.
 *
 *  This module provides some lint-like functionality for ascend type
 *  definitions.  That is, it checks for the most obvious sorts of blunders
 *  but due to the nature of the ascend language, not all can be
 *  conveniently checked for.
 *
 *  This implementation is merely a start. It needs to be much better.
 *  <pre>
 *  When #including typelint.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "list.h"
 *         #include "stattypes.h"
 *  </pre>
 *  @todo Upgrade type definition lint module.
 */

#ifndef ASC_TYPELINT_H
#define ASC_TYPELINT_H

/**	@addtogroup compiler Compiler
	@{
*/

/**
 * if TLINT_STYLE == TRUE, then style messages should be issued.
 * Likewise for the other levels of messaging.
 */
#define TLINT_STYLE   (1 >= g_parser_warnings)
#define TLINT_WARNING (2 >= g_parser_warnings)
#define TLINT_ERROR   (3 >= g_parser_warnings)
#define TLINT_FATAL   (4 >= g_parser_warnings)

enum typelinterr {
  DEF_OKAY             =  0,
  DEF_NAME_DUPLICATE   =  1,
  DEF_NAME_INCORRECT   =  2,
  DEF_NAME_MISSING     =  3,
  DEF_FUNC_UNDEFINED   =  4,
  DEF_ILLEGAL_RHS      =  5,
  DEF_ILLEGAL          =  6,
  DEF_ILLEGAL_PARAM    =  7,
  DEF_STAT_MISLOCATED  =  8,
  DEF_MULTI_PARAM      =  9,
  DEF_VALUEPARAM_BAD   =  10,
  DEF_REASSIGNED_PARAM =  11,
  DEF_ILLEGAL_VALPAR   =  12,
  DEF_ILLEGAL_INREL    =  13,
  DEF_ILLEGAL_CASGN    =  14,
  DEF_ARGNUM_INCORRECT =  15,
  DEF_ARGS_INCORRECT   =  16,
  DEF_PARAM_MODIFIED   =  17,
  DEF_CASGN_INCORRECT  =  18,
  DEF_ASGN_INCORRECT   =  19,
  DEF_ILLEGAL_ASGN     =  20,
  DEF_ILLEGAL_REFINE   =  21,
  DEF_FOR_SHADOW       =  22,
  DEF_ILLEGAL_AA       =  23,
  DEF_ILLPARAM_AA      =  24,
  DEF_ILLARRAY_AA      =  25,
  DEF_ILLEGAL_WBTS     =  26,
  DEF_UNIMPLEMENTED_WB =  27,
  DEF_ARR_INCORRECT    =  28,
  DEF_ILLEGAL_FORSET   =  29,
  DEF_ILLEGAL_SELECT   =  30,
  DEF_TOOMANY_RELOP    =  31,
  DEF_TOOMANY_LOGOP    =  32,
  DEF_USE_NOTWHEN      =  33,
  DEF_FOR_NOTMETH      =  34,
  DEF_FOR_NOTBODY      =  35,
  DEF_ILLEGAL_ATS      =  36,
  DEF_STAT_BODYASGN    =  37,
  DEF_ILLEGAL_BREAK    =  38,
  DEF_ILLEGAL_CONTINUE =  39,
  DEF_ILLEGAL_FALLTHRU =  40,
  DEF_ARGDEF_INCORRECT =  41,
  DEF_RELARRAY_SUBS    =  42,
  DEF_FOR_NOTCHECK     =  43,
  DEF_FOR_NOTEXPECT    =  44,
  DEF_MISC_STYLE       =  45,
  DEF_MISC_WARNING     =  46,
  DEF_MISC_ERROR       =  47,
  DEF_UNKNOWN_ERR
};

extern void TypeLintError(FILE *file, 
                          CONST struct Statement *statement,
                          enum typelinterr error_code);
/**<
 *  <!--  TypeLintError(file,statement,error_code)                     -->
 *  Writes a message describing the error and giving the statement to
 *  the file given.
 *  file and statement should not be NULL.
 *  Message will be suppressed if g_parse_warnings is high enough.
 */

extern void TypeLintErrorAuxillary(FILE *file,
                                   char *string,
                                   enum typelinterr error_code,
                                   int uselabel);
/**<
 *  <!--  TypeLintErrorAuxillary(file,string,error_code,uselabel)      -->
 *  Writes a string describing the error in some context to the
 *  the file given.
 *  file and string should not be NULL.
 *  If uselabel is TRUE, writes the label appropriate to the level of
 *  error_code.
 *  Message will be suppressed if g_parse_warnings is high enough.
 */

extern void TypeLintName(FILE *file,
                         CONST struct Name *name,
                         char *mesg);
/**<
 *  <!--  TypeLintName(file,name,mesg)                                 -->
 *  Writes a message followed by the name and a return to the file given.
 *  No NULL arguments allowed.
 */

#define TLNM(f,n,m,l) TypeLintNameMsg((f),(n),(m),(l))
extern void TypeLintNameMsg(FILE *file,
                            CONST struct Name *name,
                            char *mesg,
                            int level);
/**<
 *  <!--  TypeLintNameMsg(file,name,mesg,level)                        -->
 *  Writes a message followed by the name and a return to the file given.
 *  Uses the StatioLabel matching level given here.
 *  No NULL arguments allowed.
 *  Message will be suppressed if level < g_parse_warnings.
 */

extern void TypeLintNameNode(FILE *file,
                             CONST struct Name *name,
                             char *mesg);
/**<
 *  <!--  TypeLintNameNode(file,name,mesg)                             -->
 *  Writes a message followed by the first element of name and a return
 *  to the file given.
 *  No NULL arguments allowed.
 */

#define TLNNM(f,n,m,l) TypeLintNameNodeMsg((f),(n),(m),(l))
extern void TypeLintNameNodeMsg(FILE *file,
                                CONST struct Name *name,
                                char *mesg,
                                int level);
/**<
 *  <!--  TypeLintNameNodeMsg(file,name,mesg,level)                    -->
 *  Writes a message followed by the first element of name and a return
 *  to the file given.
 *  Uses the StatioLabel matching level given here.
 *  Message will be suppressed if level < g_parse_warnings.
 *  No NULL arguments allowed.
 */

/*
 * Below, less ambitious lint functions.
 * These 'Illegal' functions check statements for semantic
 * errors that can be detected WITHOUT knowing the overall
 * namespace implied by the statement list.
 */

extern enum typelinterr
TypeLintIllegalBodyStats(FILE *fp,
                         symchar *typename,
                         CONST struct StatementList *statements, 
                         unsigned int context);
/**< 
 *  <!--  error_code = TypeLintIllegalBodyStats(fp,typename,statements,context); -->
 *  Checks all statements for type legality, i.e.
 *  is the statement of a type allowed in body and are any
 *  rhs types referred to at least minimally legal in the body?
 *  Currently, IF, WILL_BE, RUN disallowed.
 *  Also, many places where compound names are illegal but yacc-able
 *  this traps. Spews per error to fp.
 *  Returns DEF_OKAY under normal circumstances,
 *  DEF_ILLEGAL or more specific error when unhappy.
 *  context should normally be context_MODEL |'d with other context_#
 *  flags as needed.
 */

extern enum typelinterr
TypeLintIllegalParamStats(FILE *fp, 
                          symchar *typename,
                          CONST struct StatementList *statements);
/**<
 *  <!--  error_code = TypeLintIllegalParamStats(fp,typename,statements); -->
 *  Checks all statements for type legality, i.e.
 *  is the statement of a type allowed in parameter list and are any
 *  rhs types referred to at least minimally legal in the parameter list?
 *  Currently only IS_A WILL_BE are allowed.
 *  Also, where compound names are illegal but yacc-able
 *  this traps. Spews per error to fp.
 *  Returns DEF_OKAY under normal circumstances,
 *  DEF_ILLEGAL or other when unhappy.
 */

extern enum typelinterr
TypeLintIllegalWhereStats(FILE *fp, 
                          symchar *typename,
                          CONST struct StatementList *statements);
/**< 
 *  <!--  error_code = TypeLintIllegalWhereStats(fp,typename,statements);  -->
 *  Checks all statements for type legality, i.e.
 *  is the statement of a type allowed in WHERE list.
 *  Currently only WILL_BE_THE_SAME allowed.
 *  Spews per error to fp.
 *  Returns DEF_OKAY under normal circumstances,
 *  DEF_ILLEGAL or other when unhappy.
 */

extern enum typelinterr
TypeLintIllegalReductionStats(FILE *fp,
                              symchar *typename,
                              CONST struct StatementList *statements);
/**< 
 *  <!--  error_code = TypeLintIllegalReductionStats(fp,typename,statements);  -->
 *  Checks all statements for type legality, i.e.
 *  is the statement of a type allowed in reduction list?
 *  Currently only CASGN allowed.
 *  Returns DEF_OKAY under normal circumstances,
 *  DEF_ILLEGAL or other when unhappy.
 */


extern enum typelinterr
TypeLintIllegalMethodStats(FILE *fp,
                           symchar *typename,
                           struct gl_list_t *pl,
                           unsigned int context);
/**<
 *  <!--  error_code = TypeLintIllegalMethodStats(fp,typename,pl,context); -->
 *  Checks all init procedures for type legality, i.e.
 *  is the statement of a type allowed in a method.
 *  Returns DEF_OKAY under normal circumstances,
 *  DEF_ILLEGAL or other when unhappy.
 */

/* @} */

#endif  /* ASC_TYPELINT_H */

