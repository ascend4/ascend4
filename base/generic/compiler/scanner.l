%{
/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 2006 Carnegie Mellon University

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
*//** @file
	ASCEND lexer

	This module is the input file for Flex (Lexical Analyzer
	Generator). Its goal is to convert a stream of characters into a
	stream of tokens.  It has been defined to be consistent with the
	routines required by the common compiler-compiler yacc.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "compiler/compiler.h"
/* everything between here and the next comment is only here */
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/func.h"
#include "compiler/expr_types.h"
#include "compiler/stattypes.h"
#include "compiler/fractions.h"
#include "compiler/proc.h"
/* because ascParse.h has a nasty union we can't digest without them. */
#include "compiler/ascParse.h"
/* these below we actually need */
#include "general/list.h"
#include "compiler/module.h"
#include "compiler/scanner.h"
#include "compiler/symtab.h"
#include "compiler/parser.h"

#define YY_BREAK
/*  Defining yybreak as above means that all of our matches must end
 *  in break or return because the normal flex supplied yybreak will
 *  be preempted by our empty one.
 *  In cases where matches contain a conditional return, make sure a
 *  break follows in the failure case.
 */

#define ENDTOK 0
/*  Return value when we reach the end of the input.
 *  This to must be 0 or negative according to yacc
 */

#define MAX_REQUIRE_DEPTH 10
/*  The maximum number of REQUIREd file nesting we will accept.
 *  See RequireStack below.
 */

#define WORKBUF_INIT_SIZE 4095
/*  We need a temporary buffer to copy yytext into before returning
 *  to the scanner (see g_workbuf below).
 *  WORKBUF_INIT_SIZE is the initial size of g_workbuf
 */

static unsigned long yy_line = 1;
/*  The current line number;
 *  every match of newline (\n) needs to ++ this variable.
 */

static unsigned long start_line = 0;
/*  The ine number where an open-comment, open-brace, or
 *  open-double-quote occurs.  We use this to help in error reporting.
 */

static int CommentNestLevel = 0;
/*  Nesting level of (* comments *)
 */

static int BracesNestLevel = 0;
/*  Nesting level of {braced} expressions
 */

static int MatchedBackslash = 0;
/*  If this variable is positive, we matched a backslash \ in a DoubleQuote
 *  or BracedText state, and we should call ProcessBackslashes() to
 *  process them.
 */

static int RequireIndex = 0;
/*  The current nesting level of REQUIREd files
 */

static YY_BUFFER_STATE RequireStack[MAX_REQUIRE_DEPTH];
/* The Flex buffers used for the REQUIREd files
 */

static char *g_workbuf = NULL;
/*  We need a place to keep doubly-quoted-text and braced-text for passing
 *  it back to the parser.  yytext will not work since the parser may ask
 *  the scanner to read another token, at which point the value in yytext
 *  gets lost.
 *
 *  The initial size of g_workbuf is WORKBUF_INIT_SIZE, and g_workbuf
 *  doubles when it needs more space.
 *
 *  A call to CopyIntoWorkBuffer(yytext,yyleng) will copy yyleng characters
 *  of yytext into the working buffer.
 *
 *  Note that having a single work buffer will not work if we ever create
 *  yacc productions that scan multiple chunks of doubly-quoted-text and/or
 *  braced-text before acting on them.
 */


/*  Forward declaration of functions
 *  provided at the end of this file.
 */
static int Asc_ScannerPopBuffer(void);
static char *CopyIntoWorkBuffer(CONST char *, unsigned long);
static int Process_Backslashes(void);
static void ErrMsg_BracesEOF(void);
static void ErrMsg_CommentEOF(void);
static void ErrMsg_DoubleQuoteEOF(void);
static void ErrMsg_LongID(void);
static void ErrMsg_LongSymbol(void);
static void ErrMsg_SymbolEOF(void);
static void ErrMsg_SymbolEOL(void);
static void ErrMsg_UnexpectedChar(void);

%}

%x Comment
%x Symbol
%x BracedText
%x DoubleQuote

blank        [\f\r\t\v ]
digit        [0-9]
letter       [a-zA-Z]

exp          ([eE][-+]?{digit}+)
real         (((({digit}+"."{digit}*)|("."{digit}+)){exp}?)|({digit}+{exp}))
integer      {digit}+
IDChar       {letter}({integer}|{letter}|_)*

%%

<INITIAL>"<="			{ return LEQ_TOK; }
<INITIAL>">="			{ return GEQ_TOK; }
<INITIAL>"<>"			{ return NEQ_TOK; }
<INITIAL>".."			{ return DOTDOT_TOK; }
<INITIAL>"::"			{ return DBLCOLON_TOK;}
<INITIAL>":="			{ return ASSIGN_TOK; }
<INITIAL>":=="			{ return CASSIGN_TOK; }
<INITIAL>"=="			{ return BEQ_TOK ; }
<INITIAL>"!="			{ return BNE_TOK ; }

<INITIAL>"="			{ return '=' ;}
<INITIAL>">"			{ return '>' ;}
<INITIAL>"<"			{ return '<' ;}
<INITIAL>","			{ return ',' ;}
<INITIAL>"."			{ return '.' ;}
<INITIAL>";"			{ return ';' ;}
<INITIAL>":"			{ return ':' ;}
<INITIAL>"["			{ return '[' ;}
<INITIAL>"]"			{ return ']' ;}
<INITIAL>"("			{ return '(' ;}
<INITIAL>")"			{ return ')' ;}
<INITIAL>"+"			{ return '+' ;}
<INITIAL>"-"			{ return '-' ;}
<INITIAL>"*"			{ return '*' ;}
<INITIAL>"/"			{ return '/' ;}
<INITIAL>"^"			{ return '^' ;}
<INITIAL>"|"			{ return '|' ;}

		/****  Reserved Keywords  ****/

<INITIAL>"ADD"			{ return ADD_TOK			; }
<INITIAL>"ALIASES"		{ return ALIASES_TOK		; }
<INITIAL>"AND"			{ return AND_TOK			; }
<INITIAL>"ANY"			{ return ANY_TOK			; }
<INITIAL>"ARE_ALIKE"    { return AREALIKE_TOK		; }
<INITIAL>"ARE_THE_SAME" { return ARETHESAME_TOK   ; }
<INITIAL>"ARRAY"		{ return ARRAY_TOK		; }
<INITIAL>"ATOM"			{ return ATOM_TOK			; }
<INITIAL>"BREAK"		{ return BREAK_TOK		; }
<INITIAL>"CALL"			{ return CALL_TOK			; }
<INITIAL>"CARD"			{ return CARD_TOK			; }
<INITIAL>"CASE"			{ return CASE_TOK			; }
<INITIAL>"CHECK"		{ return CHECK_TOK		; }
<INITIAL>"CHOICE"		{ return CHOICE_TOK	  	; }
<INITIAL>"CONDITIONAL"		{ return CONDITIONAL_TOK		; }
<INITIAL>"CONSTANT"		{ return CONSTANT_TOK		; }
<INITIAL>"CONTINUE"		{ return CONTINUE_TOK		; }
<INITIAL>"CREATE"		{ return CREATE_TOK		; }
<INITIAL>"DATA"			{ return DATA_TOK			; }
<INITIAL>"DECREASING"		{ return DECREASING_TOK		; }
<INITIAL>"DEFAULT"		{ return DEFAULT_TOK		; }
<INITIAL>"DEFINITION"		{ return DEFINITION_TOK		; }
<INITIAL>"DIMENSION"		{ return DIMENSION_TOK		; }
<INITIAL>"DIMENSIONLESS"	{ return DIMENSIONLESS_TOK	; }
<INITIAL>"DO"			{ return DO_TOK			; }
<INITIAL>"ELSE"			{ return ELSE_TOK			; }
<INITIAL>"END"			{ return END_TOK			; }
<INITIAL>"EXPECT"		{ return EXPECT_TOK		; }
<INITIAL>"EXTERNAL"		{ return EXTERNAL_TOK		; }
<INITIAL>"FALSE"		{ return FALSE_TOK		; }
<INITIAL>"FALL_THROUGH" { return FALLTHRU_TOK		; }
<INITIAL>"FIX"		    { return FIX_TOK	        ; }
<INITIAL>"FREE"		    { return FREE_TOK	        ; }
<INITIAL>"FOR"			{ return FOR_TOK			; }
<INITIAL>"FROM"			{ return FROM_TOK			; }
<INITIAL>"GLOBAL"		{ return GLOBAL_TOK		; }
<INITIAL>"ASSERT"       { return ASSERT_TOK			; }
<INITIAL>"IF"			{ return IF_TOK			; }
<INITIAL>"IMPORT"		{ return IMPORT_TOK		; }
<INITIAL>"IN"			{ return IN_TOK			; }
<INITIAL>"INCREASING"		{ return INCREASING_TOK		; }
<INITIAL>"INPUT"		{ return INPUT_TOK		; }
<INITIAL>"INTERSECTION"		{ return INTERSECTION_TOK		; }
<INITIAL>"IS_A"			{ return ISA_TOK			; }
<INITIAL>"IS_REFINED_TO"	{ return ISREFINEDTO_TOK		; }
<INITIAL>"MAXIMIZE"		{ return MAXIMIZE_TOK		; }
<INITIAL>"MAX_INTEGER"		{ return MAXINTEGER_TOK		; }
<INITIAL>"MAX_REAL"		{ return MAXREAL_TOK		; }
<INITIAL>"METHOD"		{ return METHOD_TOK		; }
<INITIAL>"METHODS"		{ return METHODS_TOK		; }
<INITIAL>"MINIMIZE"		{ return MINIMIZE_TOK		; }
<INITIAL>"MODEL"		{ return MODEL_TOK		; }
<INITIAL>"NOT"			{ return NOT_TOK			; }
<INITIAL>"NOTES"		{ return NOTES_TOK		; }
<INITIAL>"OF"			{ return OF_TOK			; }
<INITIAL>"OR"			{ return OR_TOK			; }
<INITIAL>"OTHERWISE"		{ return OTHERWISE_TOK		; }
<INITIAL>"OUTPUT"		{ return OUTPUT_TOK		; }
<INITIAL>"PROD"			{ return PROD_TOK			; }
<INITIAL>"PROVIDE"		{ return PROVIDE_TOK		; }
<INITIAL>"REFINES"		{ return REFINES_TOK		; }
<INITIAL>"REPLACE"		{ return REPLACE_TOK		; }
<INITIAL>"REQUIRE"		{ return REQUIRE_TOK		; }
<INITIAL>"RETURN"		{ return RETURN_TOK		; }
<INITIAL>"RUN"			{ return RUN_TOK			; }
<INITIAL>"SATISFIED"		{ return SATISFIED_TOK		; }
<INITIAL>"SELECT"		{ return SELECT_TOK		; }
<INITIAL>"SUCH_THAT"		{ return SUCHTHAT_TOK		; }
<INITIAL>"SUM"			{ return SUM_TOK			; }
<INITIAL>"SIZE"			{ return SIZE_TOK			; }
<INITIAL>"SWITCH"		{ return SWITCH_TOK		; }
<INITIAL>"STOP"			{ return STOP_TOK			; }
<INITIAL>"THEN"			{ return THEN_TOK			; }
<INITIAL>"TRUE"			{ return TRUE_TOK			; }
<INITIAL>"UNION"		{ return UNION_TOK		; }
<INITIAL>"UNITS"		{ return UNITS_TOK		; }
<INITIAL>"UNIVERSAL"		{ return UNIVERSAL_TOK		; }
<INITIAL>"USE"			{ return USE_TOK			; }
<INITIAL>"VALUE"		{ return VALUE_TOK		; }
<INITIAL>"WHEN"			{ return WHEN_TOK			; }
<INITIAL>"WHERE"		{ return WHERE_TOK		; }
<INITIAL>"WHILE"		{ return WHILE_TOK		; }
<INITIAL>"WILL_BE"		{ return WILLBE_TOK		; }
<INITIAL>"WILL_BE_THE_SAME"	{ return WILLBETHESAME_TOK	; }
<INITIAL>"WILL_NOT_BE_THE_SAME"	{ return WILLNOTBETHESAME_TOK	; }
<INITIAL>"WITH"			{ return WITH_TOK			; }
<INITIAL>"WITH_VALUE"		{ return WITH_VALUE_T		; }

	/*
	 *  Code to handle (* Comments *)
	 *
	 *  "(*" puts us into the Comment state.  Comments nest, so in the
	 *  Comment state we need to look for "(*" that increases the nesting
	 *  level and "*)" that will lower it.
	 *  Flex is faster if we match as much as possible, so we repeat
	 *  patterns with and without the "\n" (although it is more difficult
	 *  for the maintainer to understand) to avoid the overhead of a
	 *  separate "\n" rule.
	 *  Do NOT try to match \(\*+ since that causes "(****)" to parse
	 *  incorrectly.
	 */

<INITIAL>\(\*			{
				  /*  Match "(" followed by "*" puts us into
				   *  the COMMENT state.  Don't use \*+ since
				   *  that will parse "(***)" incorrectly.
				   *  Initialize the nesting level.
				   *  Store the current line for ErrMsg use.
				   */
				  BEGIN (Comment);
				  CommentNestLevel = 1;
				  start_line = yy_line;
				  break;
				}
<Comment>\(\*[^*(\n]*		{
				  /*  Match "(" followed "*" followed by
				   *  anything that's not "(" nor "*".
				   *  Increase the commment nesting level.
				   */
				  CommentNestLevel++;
				  break;
				}
<Comment>\(\*[^*(\n]*\n		{
				  /*  Match "(" followed by "*" followed by
				   *  anything that's not "(" nor "*".
				   *  Increase the commment nesting level.
				   */
				  yy_line++;
				  CommentNestLevel++;
				  break;
				}
<Comment>[^*(\n]*\*+\)		{
				  /*  Match anything not "*" or "(" followed
				   *  by one or more "*"s followed by ")".
				   *  This decreases the comment nesting level
				   *  and kicks us out if we're back to zero.
				   */
				  CommentNestLevel--;
				  if (CommentNestLevel == 0) {
				    BEGIN (INITIAL);
				  }
				  break;
				}
<Comment>[^*(\n]*		{
				  /*  Eat anything that's not a "*" nor a "("
				   */
				  break;
				}
<Comment>[^*(\n]*\n		{
				  /*  Eat anything that's not a "*" nor a "("
				   *  that is followed by a newline.
				   *  This rule also matches empty line.
				   */
				  yy_line++;
				  break;
				}
<Comment>\(+[^*(\n]*		{
				  /*  Eat "("s not followed by "*"
				   */
				  break;
				}
<Comment>\(+[^*(\n]*\n		{
				  /*  Eat "("s not followed by "*" plus a "\n"
				   */
				  yy_line++;
				  break;
				}
<Comment>\*+[^*()\n]*		{
				  /*  Eat "*"s not followed by ")"
				   */
				  break;
				}
<Comment>\*+[^*()\n]*\n		{
				  /*  Eat "*" not followed by ")" plus a "\n"
				   */
				  yy_line++;
				  break;
				}
<Comment><<EOF>>		{
				  /*  An EOF in a Comment means bad nesting.
				   *  Print an error and pop back a level
				   *  or return ENDTOK if no more input.
				   */
				  ErrMsg_CommentEOF();
				  CommentNestLevel = 0;
				  if ( Asc_ScannerPopBuffer() == 1 ) {
				    return ENDTOK;
				  }
				  break;
				}


	/*
	 *  Code to handle 'Symbols'
	 *
	 *  Symbols are simple: they are 'singely quoted strings' that
	 *  exist on a single line.  Look for anything that is not
	 *  a quote or a newline to get the text of the symbol.
	 */

<INITIAL>\'			{
				  /*  A single quote (') in the INITIAL state
				   *  puts us into the Symbol state.
				   */
				  BEGIN (Symbol);
				  break;
				}
<Symbol>[^'\n]*\'		{
				  /*  Anything that is not a (') nor a newline
				   *  followed by a (') is the symbol's text.
				   *  Return to the INITIAL state, store the
				   *  symbol in the symbol table and return
				   *  SYMBOL_TOK to the parser.
				   */
				  BEGIN (INITIAL);
				  /*  strip off the final (')
				   */
				  yytext[--yyleng] = '\0';
				  if (yyleng > YY_MAXLEN) {
				    ErrMsg_LongSymbol();
				    break;
				  }
				  zz_lval.sym_ptr = AddSymbolL(yytext,yyleng);
				  return SYMBOL_TOK;
				}
<Symbol>[^'\n]*\n		{
				  /*  If we find a newline before a ('), the
				   *  symbol is unterminated.  Print an error
				   *  message and return to the INITIAL state.
				   */
				  ErrMsg_SymbolEOL();
				  yy_line++;
				  BEGIN(INITIAL);
				  break;
				}
<Symbol><<EOF>>			{
				  /*  If we find an EOF before a ('), the
				   *  symbol is unterminated.  Print an error
				   *  message and pop to the previously
				   *  REQUIREd file or return ENDTOK if the
				   *  pop fails due to no more input.
				   */
				  ErrMsg_SymbolEOF();
				  if ( Asc_ScannerPopBuffer() == 1 ) {
				    return ENDTOK;
				  }
				  break;
				}


	/*
	 *  Code to handle "Text in Double Quotes"
	 *
	 *  The DoubleQuote state begins with a double quote and ends
	 *  with a double quote; double quotes can be included by
	 *  escaping them with a backslash (e.g. \").  There is no
	 *  nesting level to worry about.
	 *  Flex is faster if we match as much as possible, so we repeat
	 *  patterns with and without the "\n" (although it is more difficult
	 *  for the maintainer to understand) to avoid the overhead of a
	 *  separate "\n" rule.
	 *  We want to keep the text, so we need to call yymore().
	 */

<INITIAL>\"			{
				  /*  A double quote puts us into the
				   *  DoubleQuote state.  Save the line
				   *  number for error reporting.
				   */
				  BEGIN (DoubleQuote);
				  start_line = yy_line;
				  break;
				}
<DoubleQuote>[^\\"\n]*\\.	{
				  /*  A backslash \ in the DoubleQuote
				   *  state protects any character.
				   */
				  MatchedBackslash++;
				  yymore();
				  break;
				}
<DoubleQuote>[^\\"\n]*\\\n	{
				  /*  A backslash \ in the DoubleQuote
				   *  state protects a newline.
				   */
				  MatchedBackslash++;
				  yy_line++;
				  yymore();
				  break;
				}
<DoubleQuote>[^\\"\n]*\"	{
				  /*  A double quote in the DoubleQuote state
				   *  (that is not protected by backslash)
				   *  will put us back in the INITIAL state.
				   *  Process the string and return DQUOTE_TOK
				   *  to the parser.
				   */
				  BEGIN (INITIAL);
				  /*  Remove the final double quote
				   */
				  yytext[--yyleng] = '\0';
				  /*  Do backslash substitutions on the string
				   *  before returing it to the scanner.
				   */
				  if ( MatchedBackslash != 0 ) {
				    Process_Backslashes();
				    MatchedBackslash = 0;
				  }
				  zz_lval.dquote_ptr =
				      CopyIntoWorkBuffer(yytext,yyleng);
				  return DQUOTE_TOK;
				}
<DoubleQuote>[^\\"\n]*		{
				  /*  Match anything that is not backslash nor
				   *  doublequote and add it to the text.
				   */
				  yymore();
				  break;
				}
<DoubleQuote>[^\\"\n]*\n	{
				  /*  Match anything that is not backslash nor
				   *  doublequote and add it to the text.
				   *  This also matches an empty line.
				   */
				  yy_line++;
				  yymore();
				  break;
				}
<DoubleQuote><<EOF>>		{
				  /*  End of File in a DoubleQuote state
				   *  means no matching double quote.
				   *  Print an error and pop next buffer
				   *  off the RequireStack or return ENDTOK
				   *  if there is no more input.
				   */
				  ErrMsg_DoubleQuoteEOF();
				  MatchedBackslash = 0;
				  if ( Asc_ScannerPopBuffer() == 1 ) {
				    return ENDTOK;
				  }
				  break;
				}


	/*
	 *  Code to handle { Text in Braces }
	 *
	 *  "{" puts us into the BracedText state.	Braces nest, so
	 *  in the BracedText state we need to look for "{" that increases
	 *  the nesting level and "}" that will lower it.
	 *  Flex is faster if we match as much as possible, so we repeat
	 *  patterns with and without the "\n" (although it is more difficult
	 *  for the maintainer to understand) to avoid the overhead of a
	 *  separate "\n" rule.
	 *  We want to keep the text we scan, so we have to call yymore().
	 */

<INITIAL>\{{blank}*\n		{ /*  A "{" puts us into the BracedText state.
				   *  If from the opening "{" to the first
				   *  newline is all whitespace, then ignore
				   *  it.
				   *  Initialize the nesting level.
				   *  Save the current line number for
				   *  error message reporting.
				   */
				  BEGIN (BracedText);
				  BracesNestLevel = 1;
				  start_line = yy_line;
				  yy_line++;
				  break;
				}
<INITIAL>\{			{
				  /*  A "{" puts us into the BracedText state.
				   *  Initialize the nesting level.
				   *  Save the current line number for
				   *  error message reporting.
				   */
				  BEGIN (BracedText);
				  BracesNestLevel = 1;
				  start_line = yy_line;
				  break;
				}
<BracedText>[^\\{}\n]*\\.	{
				  /*  A backslash \ in the BracedText state
				   *  protects any character and does not
				   *  affect the Nesting Level.
				   */
				  MatchedBackslash++;
				  yymore();
				  break;
				}
<BracedText>[^\\{}\n]*\\\n	{
				  /*  A backslash \ in the BracedText state
				   *  protects a newline.
				   */
				  MatchedBackslash++;
				  yy_line++;
				  yymore();
				  break;
				}
<BracedText>\{[^\\{}\n]*	{
				  /*  A "{" in the braces state gets added to
				   *  the text and increase the nesting level.
				   */
				  BracesNestLevel++;
				  yymore();
				  break;
				}
<BracedText>\{[^\\{}\n]*\n	{
				  /*  A "{" in the braces state gets added to
				   *  the text and increase the nesting level.
				   */
				  yy_line++;
				  BracesNestLevel++;
				  yymore();
				  break;
				}
<BracedText>[^\\{}\n]*\}	{
				  /*  A "}" will reduce the nesting level.
				   *  If the nesting level is zero, go back to
				   *  the INITIAL level, save the text as a
				   *  Symbol, do the backslash substitution,
				   *  and return BRACEDTEXT_TOK to the
				   *  parse; otherwise, add the "}" to the
				   *  text and keep scanning.
				   */
				  BracesNestLevel--;
				  if (BracesNestLevel == 0) {
				    BEGIN (INITIAL);
				    /*	Remove the final "}"
				     */
				    yytext[--yyleng] = '\0';
				    /*	Do backslash substitutions on the text
				     *	before returing it to the scanner.
				     */
				    if ( MatchedBackslash != 0 ) {
				      Process_Backslashes();
				      MatchedBackslash = 0;
				    }
				    zz_lval.braced_ptr =
				        CopyIntoWorkBuffer(yytext,yyleng);
				    return BRACEDTEXT_TOK;
				  }
				  yymore();
				  break;
				}
<BracedText>[^\\{}\n]*		{
				  /*  Match anything that is not "{" nor "}"
				   *  nor "\\"(backslash) and add it to text.
				   */
				  yymore();
				  break;
				}
<BracedText>[^\\{}\n]*\n		{
				  /*  Match anything that is not "{" nor "}"
				   *  nor "\\"(backslash) followed by a "\n"
				   *  and add it to text.
				   *  This also matches an empty line.
				   */
				  yy_line++;
				  yymore();
				  break;
				}
<BracedText><<EOF>>		{
				  /*  End of File in braces means bad nesting.
				   *  Print an error message and pop to the
				   *  previously REQUIREd file or return
				   *  ENDTOK if no more input.
				   */
				  ErrMsg_BracesEOF();
				  BracesNestLevel = 0;
				  MatchedBackslash = 0;
				  if ( Asc_ScannerPopBuffer() == 1 ) {
				    return ENDTOK;
				  }
				  break;
				}


	/*
	 *  Code to handle Miscellaneous types.
	 *
	 */

<INITIAL>{integer}		{
				  /*  An integer.  Defn near top of file.
				   */
				  zz_lval.int_value = atol(yytext);
				  return INTEGER_TOK;
				}

<INITIAL>{integer}/".."		{
				  /*  An integer as the first number in a
				   *  range (need to avoid parsing "1..2"
				   *  as the real numbers 1.0 and 0.2).
				   */
				  zz_lval.int_value = atol(yytext);
				  return INTEGER_TOK;
				}

<INITIAL>{real}			{
				  /*  A real number.  Defn near top of file.
				   */
				  zz_lval.real_value = atof(yytext);
				  return REAL_TOK;
				}

<INITIAL>{IDChar}+		{
				  /*  An identifier.  Defn near top of file.
				   */
				  if (yyleng >YY_MAXLEN) {
				    ErrMsg_LongID();
				    break;
				  }
				  zz_lval.id_ptr = AddSymbolL(yytext,yyleng);
				  return IDENTIFIER_TOK;
				}

<INITIAL>{blank}*		{
				  /*  Ignore whitespace.  */
				  break;
				}
<INITIAL>{blank}*\n		{
				  /*  Ignore whitespace.  */
				  yy_line++;
				  break;
				}


<INITIAL>.			{
				  /*  Unknown character.  Print error
				   *  message and keep going.
				   */
				  ErrMsg_UnexpectedChar();
				  break;
				}

<INITIAL><<EOF>>		{
				  /*  Print an error message if we
				   *  reached EOF in the middle of a
				   *  type definition.	Pop to the
				   *  previously REQUIREd file or return
				   *  ENDTOK if no more input.
				   */
				  Asc_ErrMsgTypeDefnEOF();
				  if ( Asc_ScannerPopBuffer() == 1 ) {
				    return ENDTOK;
				  }
				  break;
				}

%%
/*
 *  int yywrap(void);
 *
 *  This returns 1 if the scanner should stop parsing, or
 *  0 if the scanner should continue.  Flex requires this
 *  function unless %option noyywrap is defined.
 */
int
yywrap(void)
{
  return 1;
}


/*
 *  See the header file scanner.h for a description of this function.
 */
unsigned long
LineNum(void)
{
  return yy_line;
}


/*
 *  See the header file scanner.h for a description of this function.
 */
void
Asc_ScannerAssignFile(FILE *f, unsigned long linenum)
{
  yyin = f;
  yy_line = linenum;
  if ( RequireIndex == 0 ) {
    yyrestart(f);
  }
}

/*
 *  See the header file scanner.h for a description of this function.
 */
void
Asc_ScannerAssignString(void *yybs, unsigned long linenum, int first)
{
  /* yyin = f; */
  yy_line = linenum;
  yy_switch_to_buffer((YY_BUFFER_STATE)yybs);
  if (first) {
    BEGIN(INITIAL);
  }
  if ( RequireIndex == 0 ) {
    yyrestart((FILE *)NULL); /* ? ? ? should be reading from a string buffer... */
  }
}


/*
 *  See the header file scanner.h for a description of this function.
 */
int
Asc_ScannerPushBuffer(CONST char *filename)
{
  int status;        /* status returned from Asc_RequireModule */
  
  if ( RequireIndex >= MAX_REQUIRE_DEPTH ) {
    FPRINTF(ASCERR,
	    "Error:\tREQUIRE nested too deeply (%d levels) on line %s:%lu.\n"
	    "\tFile \"%s\" not read.\n",
	    RequireIndex,
	    Asc_ModuleBestName(Asc_CurrentModule()),
	    yy_line,
	    filename);
    return 1;
  }

  /*  The current Flex buffer is not on the RequireStack yet, so add it
   *  before calling Asc_OpenModule.  We need to increment RequireIndex
   *  before calling Asc_OpenModule due to the check in
   *  Asc_ScannerAssignFile.
   */
  RequireStack[RequireIndex++] = YY_CURRENT_BUFFER;

  Asc_RequireModule( filename, &status );
  if( status == 5 ) {
    /* already required */
    RequireIndex--;
    CONSOLE_DEBUG("REQUIREd module \"%s\" already PROVIDEd", filename);
    return 1;
  }
  if( status == 4 ) {
    /* recursive require */
    RequireIndex--;
    ERROR_REPORTER_HERE(ASC_USER_WARNING
		,"Recursive REQUIRE for module \"%s\" (ignored)",filename
	);
    return 1;
  }
  if ( status != 0 ) {
    /*  The open failed.  Decrement RequireIndex and print an error.
     */
    RequireIndex--;
    error_reporter(ASC_USER_ERROR
		,Asc_ModuleBestName(Asc_CurrentModule()),yy_line,NULL
		,"REQUIRE cannot open module \"%s\""
	    ,filename
	);
    return 2;
  }

  /*  Asc_OpenModule was successful, so print a message, switch to the
   *  new buffer in the INITIAL state.
   * SHOULD never reach here with a string buffer as they cannot be
   * REQUIREd.
   */
  Asc_FPrintf(stderr,"REQUIREing file \"%s\"\n", filename);
  yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
  BEGIN (INITIAL);
  return 0;
}


/*
 *  int Asc_ScannerPopBuffer()
 *
 *  When we reach an End Of File (EOF) in the scanner, we call this
 *  function to pop us to the file which REQUIREd the file we just
 *  reached the end of and return 0.  If there are no more files on the
 *  RequireStack, return 1.
 */
static int
Asc_ScannerPopBuffer(void)
{
  Asc_CloseCurrentModule(); /* the current module may be NULL. */
  if ( RequireIndex == 0 ) {
    return 1;
  }

  yy_delete_buffer(YY_CURRENT_BUFFER);
  yy_switch_to_buffer( RequireStack[--RequireIndex] );
  BEGIN(INITIAL);
  return 0;
}

void Asc_ScannerReleaseStringBuffer(void *yybs)
{
  yy_delete_buffer((YY_BUFFER_STATE)yybs);
  yy_switch_to_buffer( RequireStack[--RequireIndex] );
  BEGIN(INITIAL);
}

void *Asc_ScannerCreateStringBuffer(CONST char *string, int len)
{
  /* we hope to god yy_scan_bytes does not change current buffer */
  YY_BUFFER_STATE yybs;
  /* push current, don't care its name or type. */
  RequireStack[RequireIndex++] = YY_CURRENT_BUFFER;
  yybs = yy_scan_bytes(string, len);
  assert(yybs);
  return (void *)yybs;
}

/*
 *  g_workbuf = CopyIntoWorkBuffer(str, len)
 *  char *g_workbuf;
 *  CONST char *str;
 *  unsigned long len;
 *
 *  Copy the string `str' having length `len' into the Scanner's Working
 *  Buffer g_workbuf and return g_workbuf if successful or NULL if
 *  unsuccessful (due to lack of memory).
 *  If g_workbuf is too short to hold `str', it is repeatably doubled until
 *  either it is big enough or memory is exhausted.
 *  We actually copy len+1 characters of `str' into g_workbuf so that the
 *  final \0 gets copied also.
 *  Global Effects: Modifies the contents of g_workbuf
 *                  Possibly changes the address of g_workbuf (on realloc)
 */
static char *
CopyIntoWorkBuffer(CONST char *str, unsigned long len)
{
  static size_t g_workbuf_len = WORKBUF_INIT_SIZE;  /* length of g_workbuf */

  assert(str[len]=='\0'); /* is this true for all yacc? */
  len++;  /* add one to make sure we have a final '\0' */
  if (( len >= g_workbuf_len ) || ( g_workbuf == NULL )) {
    while( len >= g_workbuf_len ) {
      g_workbuf_len *= 2;
    }
    if (g_workbuf == NULL ) {
      g_workbuf = ASC_NEW_ARRAY(char, g_workbuf_len );
    } else {
      g_workbuf = (char*)ascrealloc( (void*)g_workbuf, g_workbuf_len );
    }
    if ( g_workbuf == NULL ) {
      return NULL;
    }
  }
  return strncpy(g_workbuf, str, len); /* does not add a NULL char */
}


extern void
Asc_DestroyScannerWorkBuffer(void)
{
  if (g_workbuf != NULL) {
    ascfree(g_workbuf);
  }
  g_workbuf = NULL;
}

void Asc_DestroyScannerInputBuffer(void)
{
  if (YY_CURRENT_BUFFER != NULL) {
    yy_delete_buffer(YY_CURRENT_BUFFER);
  }
}

/*
 *  int Process_Backslashes(void)
 *
 *  Covert any backslash \ escapes into the correct character.
 *  Operates on and modifies in place the value of yytext; yyleng
 *  is adjusted accordingly.  Returns the number of backslash
 *  substitutions made.  The following are supported:
 *      \a   (alert)
 *      \b   (backspace)
 *      \f   (formfeed)
 *      \n   (newline)
 *      \r   (carriage return)
 *      \t   (horizontal tab)
 *      \v   (vertical tab)
 *      \xhh (hexadecimal hh)              ***** NOT IMPLEMENTED *****
 *      \ooo (octal ooo where o == [0-7])  ***** NOT IMPLEMENTED *****
 *      \\n  (backslash before newline removes the backslash and newline)
 *      \.   (any other char produces that char)
 */
static int
Process_Backslashes(void)
{
  int old_index;
  int new_index;
  int substitution_count = 0;

  for (old_index=0, new_index=0; old_index<yyleng; old_index++) {
    if (yytext[old_index] != '\\') {
      yytext[new_index++] = yytext[old_index];
    } else {
      if (++old_index < yyleng) {
        switch(yytext[old_index]) {
        case 'a':
          yytext[new_index++] = '\a';
          substitution_count++;
          break;
        case 'b':
          yytext[new_index++] = '\b';
          substitution_count++;
          break;
        case 'f':
          yytext[new_index++] = '\f';
          substitution_count++;
          break;
        case 'n':
          yytext[new_index++] = '\n';
          substitution_count++;
          break;
        case 'r':
          yytext[new_index++] = '\r';
          substitution_count++;
          break;
        case 't':
          yytext[new_index++] = '\t';
          substitution_count++;
          break;
        case 'v':
          yytext[new_index++] = '\v';
          substitution_count++;
          break;
        case 'x':
          /* need to add processing for
	   * hexadecimal numbers \xhh here
           */
          yytext[new_index++] = yytext[old_index];
          substitution_count++;
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          /* need to add processing for
	   * octal numbers \ooo here
           */
          yytext[new_index++] = yytext[old_index];
          substitution_count++;
          break;
        case '\n':
	  /*  Backslash at the end of the line removes
	   *  the slash and the newline from the result,
	   *  so no futher processing is needed.
	   */
	  substitution_count++;
	  break;
        default:
          yytext[new_index++] = yytext[old_index];
          substitution_count++;
          break;
        }
      }
    }
  }
  yytext[new_index] = '\0';
  yyleng = new_index;
  return substitution_count;
}


/*
 *  void ErrMsg_*(void)
 *
 *  The following all print error messages to the file handle ASCERR.
 *  The type of error is indicated by the function's name and the
 *  arguments to fprintf.
 */
static void
ErrMsg_BracesEOF(void){
	error_reporter(ASC_USER_ERROR, Asc_ModuleBestName(Asc_CurrentModule()), start_line, NULL
		,"End of file reached within a unit, data table or explanation. No closing brace "
		"found for open brace."
	);
}


static void
ErrMsg_CommentEOF(void)
{
  FPRINTF(ASCERR,
          "Error:\tEnd of file reached within a comment.\n"
	  "\tNo close-comment found for comment starting on line %s:%lu\n",
	  Asc_ModuleBestName(Asc_CurrentModule()), start_line);
}


static void
ErrMsg_LongID(void)
{
  FPRINTF(ASCERR,
	  "Error:\tIdentifier too long on line %s:%lu.\n"
	  "\tIdentifier \"%s\" exceeds the maximum identifier size of %d\n",
	  Asc_ModuleBestName(Asc_CurrentModule()),
	  yy_line,
	  yytext,
	  YY_MAXLEN);
}


static void
ErrMsg_LongSymbol(void)
{
  FPRINTF(ASCERR,
	  "Error:\tSymbol too long on line %s:%lu.\n"
	  "\tSymbol %s exceeds the maximum symbol size of %d\n",
	  Asc_ModuleBestName(Asc_CurrentModule()),
	  yy_line,
	  yytext,
	  YY_MAXLEN);
}


static void
ErrMsg_DoubleQuoteEOF(void)
{
  FPRINTF(ASCERR,
          "Error:\tEnd of file reached with a double quoted string.\n"
	  "\tNo close quote found for the open quote on line %s:%lu\n",
	  Asc_ModuleBestName(Asc_CurrentModule()), start_line);
}


static void
ErrMsg_SymbolEOF(void)
{
  FPRINTF(ASCERR,
          "Error:\tEnd of file reached within a symbol.\n"
	  "\tNo close quote found for symbol on line %s:%lu\n",
	  Asc_ModuleBestName(Asc_CurrentModule()), yy_line);
}


static void
ErrMsg_SymbolEOL(void)
{
  FPRINTF(ASCERR,
          "Error:\tEnd of line reached within a symbol.\n"
	  "\tNo close quote found for symbol on line %s:%lu\n",
	  Asc_ModuleBestName(Asc_CurrentModule()), yy_line);
}

#define ERRCOUNT_UNEXPCHAR 30
static void ErrMsg_UnexpectedChar(){
	static int errcount=0;
	if(errcount<ERRCOUNT_UNEXPCHAR){
		error_reporter(ASC_USER_ERROR
			,Asc_ModuleBestName(Asc_CurrentModule()), yy_line
			,"Unexpected character '%s' in input.", yytext
		);
		errcount++;
		if(errcount==ERRCOUNT_UNEXPCHAR){
			ERROR_REPORTER_HERE(ASC_PROG_NOTE
				,"Further reports of this error will be supressed\n"
			);
		}
	}
}
