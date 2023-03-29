/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         zz_parse
#define yylex           zz_lex
#define yyerror         zz_error
#define yydebug         zz_debug
#define yynerrs         zz_nerrs
#define yylval          zz_lval
#define yychar          zz_char

/* First part of user prologue.  */
#line 29 "ascend/compiler/ascParse.y"

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>		/* need DBL_MAX and LONG_MAX */
#include <float.h>		/* on a NeXT they are in here */

#include <ascend/compiler/parser.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/dstring.h>
#include <ascend/compiler/compiler.h>
#include <ascend/compiler/scanner.h>
#include <ascend/compiler/symtab.h>		/* the global string/symbol table */
#include <ascend/compiler/notate.h>		/* notes database wrapper */
#include <ascend/compiler/braced.h>
#include <ascend/compiler/fractions.h>
#include <ascend/compiler/dimen.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/func.h>
#include <ascend/compiler/expr_types.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/nameio.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/extfunc.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/sets.h>
#include <ascend/compiler/exprs.h>
#include <ascend/compiler/exprio.h>
#include <ascend/compiler/vlist.h>
#include <ascend/compiler/vlistio.h>		/* for debugging only */
#include <ascend/compiler/stattypes.h>
#include <ascend/compiler/slist.h>
#include <ascend/compiler/statement.h>
#include <ascend/compiler/statio.h>
#include <ascend/compiler/units.h>
#include <ascend/compiler/when.h>
#include <ascend/compiler/select.h>
#include <ascend/compiler/switch.h>
#include <ascend/compiler/proc.h>
#include <ascend/compiler/watchpt.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/child.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/type_descio.h>
#include <ascend/compiler/typedef.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/syntax.h>
#include <ascend/compiler/lexer.h>

/* 1 ==> expr can find missing , w/o  shift/reduce conflicts */
#define COMMAEXPR_NOTBUGGY 0 
#if COMMAEXPR_NOTBUGGY
#include <ascend/compiler/exprio.h>
#endif /* for CommaExpr if working. */

int g_compiler_warnings = 1;		/* level of whine to allow */

#include <ascend/compiler/redirectFile.h>
#ifndef ASCERR
# error "ASCERR not defined"
#endif

extern int zz_error(char *);
/*  provided at the end of this file.
 *  it increments g_untrapped_error, the global defined below
 */

static unsigned long g_header_linenum = 0;
static unsigned int g_untrapped_error = 0;
/* if g_untrapped_error is set to 1, the current definition
 * should be abandoned even if nothing else detectable by
 * typedef.c exists. basically any syntax error should cause a type
 * to be abandoned, but not all cause the parser to barf.
 */

/* the last seen method name while processing a method */
static symchar *g_proc_name=NULL;

/* the last seen ATOM/MODEL/constant type and refinement base */
static symchar *g_type_name=NULL;
static symchar *g_refines_name=NULL;
/*
 * g_type_name is used by the scanner when closing a module to check if
 * the parser is in the middle of parsing a MODEL or ATOM type
 * definition.
 *      g_type_name == NULL implies that the parser is in between
 *                          definitions. This depends on proper
 *                          resets to NULL at END of type productions.
 *      g_type_name != NULL implies that the parser is in the middle
 *                          of a type definition and gives the name
 *                          of that type.
 */

static symchar *g_end_identifier = NULL;
/*  This variable gets reset every time we see an ``END_TOK''.  If the
 *  token after END_TOK is missing (i.e., ``END;'') or if it is recognized
 *  (e.g., FOR_TOK), set this variable to NULL.  When we see an
 *  ``END_TOK IDENTIFIER_TOK'', set this variable to the pointer into the
 *  symbol table returned by the scanner.
 */

static struct StatementList *g_model_parameters=NULL;
/* this is the statementlist of the parameterized type
 */

static struct StatementList *g_parameter_wheres=NULL;
/* this is the where list of a parameterized type. restrictions on args
 */

static struct StatementList *g_parameter_reduction=NULL;
/* this is the statementlist of the REFINES clause
 * at present both might be null, which is bad.
 */

static struct Set *g_typeargs=NULL;
/* optional args to a typename in part declaration.
 * it is set in the production type_identifier. in
 * contexts where args are not allowed, use IDENTIFIER_TOK instead of the
 * type_identifier production.
 */

static struct Set *g_callargs=NULL;
/* optional args to a user defined method.
 * it is set in the production call_identifier. in
 * contexts where args are not allowed, use IDENTIFIER_TOK instead of the
 * type_identifier production.
 */

static struct gl_list_t *g_notelist = NULL;
/*
 * Notes accumulator until a type commits or destroys it.
 */

static int g_defaulted;			/* used for atoms,constants */

static CONST dim_type *g_dim_ptr;	  /* dim of last units parsed, or so */
static CONST dim_type *g_atom_dim_ptr;	  /* dim of DIMENSION decl */
static CONST dim_type *g_default_dim_ptr; /* dim of default value parsed */

static double g_default_double;
static long g_default_long;
symchar *g_default_symbol;
#define DOUBLECONSTANT 0
#define LONGCONSTANT 1
#define BOOLEANCONSTANT 2
#define SYMBOLCONSTANT 3
static int g_constant_type = DOUBLECONSTANT;
static CONST struct Units *g_units_ptr;

int g_parse_relns = 1;
/*  Should we parse relations?
 *      0 indicates don't parse relations
 *      1 indicates process them
 */

/*  Forward declaration of error message reporting
 *  functions provided at the end of this file.
 */
static void ErrMsg_Generic(CONST char *);
static void ErrMsg_CommaName(CONST char *, struct Name *);
#if COMMAEXPR_NOTBUGGY
static void ErrMsg_CommaExpr(CONST char *, struct Expr *);
#endif /* COMMAEXPR_NOTBUGGY */
static void ErrMsg_NullDefPointer(CONST char *);
static void ErrMsg_ParensBrackets(CONST char *);
static void WarnMsg_MismatchEnd(CONST char *, CONST char *,
                                unsigned long, CONST char *);
static CONST char *TokenAsString(unsigned long);

static void error_reporter_current_line(const error_severity_t sev, const char *fmt,...);

/** @page ascend-notes About 'NOTES' in ASCEND
 *
 *  The parser will now parse a NOTES block as well as in-lined NOTES.  As
 *  a matter of fact, the parser would probably parse FORTRAN now since it
 *  is very lenient.  For the in-lined NOTES, I accept any "doubly-quoted"
 *  string after an `fname'.  I am currently dropping the NOTE on the
 *  floor.  Ideally, we should only accept an inline NOTE when we are
 *  creating a new thing, like in an IS_A statement or a labeled relation.
 *  That means either adding the optional_note to fname and whining at the
 *  user when he/she uses the in-lined notes incorrectly, or coming up
 *  with a fvarnotelist and fnamenote that accepts fnames and
 *  optional_notes in a list or a stand-alone form.
 *
 *  For the block NOTES statement, the symtax is
 *
 *      NOTES ( SYMBOL_TOK ( fvarlist BRACEDTEXT_TOK )+ )+ END NOTES ';'
 *
 *  Here I am using () as part of the meta-language to describe the syntax
 *  to you, they are not part of the formal grammer.  An example is
 *
 *      NOTES
 *          'text'  a,b,c  { Here is some text }
 *                  d      { Here is more text }
 *          'html'  SELF   { <bold>html sucks</bold> }
 *      END NOTES;
 *
 *  Note that the only punctuation is the `,' between the members of the
 *  fvarlist and the closing `;'.  Right now, the term `SELF' would be
 *  eaten in the fvarlist production.  I'm not sure if this is what we
 *  should do (which requires having the notes processing do something
 *  special when it sees SELF in the fvarlist), or if we should create
 *  a SELF_TOK token.  The latter is certainly easier to implement from
 *  the parser's perspective, which is why I did it that way.
 *
 *  The block NOTES statement doesn't do anything with its stuff either,
 *  the symbols and {bracedText} get dropped on the floor and I destroy
 *  the fvarlist, but that all that happens.
 *
 *  The `notes_body' and `noteslist' productions return `notes_ptr', which
 *  right now is a `void*' until we can decide what type of data structure
 *  we want to handle NOTES.
 *
 *  As an amusing side note, the parser will currently eat the following:
 *
 *      NOTES
 *          'fun' name  "put some text here"  {put more text here}
 *      END NOTES;
 *
 *  Like I said, this is so the parser will eat them; it's not being real
 *  smart about what it does with them.
 *
 *  For debugging the NOTES stuff, set the DEBUG_NOTES macro to the
 *  following:
 *
 *  #define DEBUG_NOTES(s) FPRINTF(stderr,"****DISCARDED NOTES:\n%s****\n",(s))
 *
 *  To completely ignore the NOTES, set DEBUG_NOTES to the following:
 *
 *  #define DEBUG_NOTES(s) 0
 *
 *  Note that if you do the latter, you'll get `statement with no effect'
 *  warnings from gcc -Wall.
 *
 * -- Mark Thomas  Thursday, 13 March 1997
 */
/* #define DEBUG_NOTES(s) 0 */
#define DEBUG_NOTES(s) ERROR_REPORTER_NOLINE(ASC_USER_WARNING,"Discarded note: %s", (s))

/*
 * Because the ascend types and notes are not tightly bound to each other,
 * what with notes being stored in a database,
 * We collect notes locally until the type associated with a batch of notes
 * is approved by typedef and other error checks. Then we process the
 * collected notes, commiting them to the database.
 *
 * NOTES made via ADD NOTES do not require a type check first and the
 * type may not even exist yet in the library.
 *
 * ProcessNotes(keep); Clear up collected list, commit them if keep != 0.
 * CollectNote(note); Add a note to the collected list.
 */
static void ProcessNotes(int);
static void CollectNote(struct Note *);

/* For 'inline' notes, note on DQUOTE_TOK from scanner.l:
 * Remember that DQUOTE_TOK is a string value which is local to the
 * production that finds it. It must be copied if you want to
 * keep it.
 */

/* MS VC++ won't compiler Bison output unless we switch this */
#ifdef _MSC_VER
# define __STDC__
#endif


#line 347 "ascend/compiler/ascParse.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "ascParse.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ADD_TOK = 3,                    /* ADD_TOK  */
  YYSYMBOL_ALIASES_TOK = 4,                /* ALIASES_TOK  */
  YYSYMBOL_AND_TOK = 5,                    /* AND_TOK  */
  YYSYMBOL_ANY_TOK = 6,                    /* ANY_TOK  */
  YYSYMBOL_AREALIKE_TOK = 7,               /* AREALIKE_TOK  */
  YYSYMBOL_ARETHESAME_TOK = 8,             /* ARETHESAME_TOK  */
  YYSYMBOL_ARRAY_TOK = 9,                  /* ARRAY_TOK  */
  YYSYMBOL_ASSERT_TOK = 10,                /* ASSERT_TOK  */
  YYSYMBOL_ATOM_TOK = 11,                  /* ATOM_TOK  */
  YYSYMBOL_BEQ_TOK = 12,                   /* BEQ_TOK  */
  YYSYMBOL_BNE_TOK = 13,                   /* BNE_TOK  */
  YYSYMBOL_BREAK_TOK = 14,                 /* BREAK_TOK  */
  YYSYMBOL_CALL_TOK = 15,                  /* CALL_TOK  */
  YYSYMBOL_CARD_TOK = 16,                  /* CARD_TOK  */
  YYSYMBOL_CASE_TOK = 17,                  /* CASE_TOK  */
  YYSYMBOL_CHOICE_TOK = 18,                /* CHOICE_TOK  */
  YYSYMBOL_CHECK_TOK = 19,                 /* CHECK_TOK  */
  YYSYMBOL_CONDITIONAL_TOK = 20,           /* CONDITIONAL_TOK  */
  YYSYMBOL_CONSTANT_TOK = 21,              /* CONSTANT_TOK  */
  YYSYMBOL_CONTINUE_TOK = 22,              /* CONTINUE_TOK  */
  YYSYMBOL_CREATE_TOK = 23,                /* CREATE_TOK  */
  YYSYMBOL_DATA_TOK = 24,                  /* DATA_TOK  */
  YYSYMBOL_DECREASING_TOK = 25,            /* DECREASING_TOK  */
  YYSYMBOL_DEFAULT_TOK = 26,               /* DEFAULT_TOK  */
  YYSYMBOL_DEFINITION_TOK = 27,            /* DEFINITION_TOK  */
  YYSYMBOL_DER_TOK = 28,                   /* DER_TOK  */
  YYSYMBOL_DIMENSION_TOK = 29,             /* DIMENSION_TOK  */
  YYSYMBOL_DIMENSIONLESS_TOK = 30,         /* DIMENSIONLESS_TOK  */
  YYSYMBOL_DO_TOK = 31,                    /* DO_TOK  */
  YYSYMBOL_ELSE_TOK = 32,                  /* ELSE_TOK  */
  YYSYMBOL_END_TOK = 33,                   /* END_TOK  */
  YYSYMBOL_EXPECT_TOK = 34,                /* EXPECT_TOK  */
  YYSYMBOL_EXTERNAL_TOK = 35,              /* EXTERNAL_TOK  */
  YYSYMBOL_FALSE_TOK = 36,                 /* FALSE_TOK  */
  YYSYMBOL_FALLTHRU_TOK = 37,              /* FALLTHRU_TOK  */
  YYSYMBOL_FIX_TOK = 38,                   /* FIX_TOK  */
  YYSYMBOL_FOR_TOK = 39,                   /* FOR_TOK  */
  YYSYMBOL_FREE_TOK = 40,                  /* FREE_TOK  */
  YYSYMBOL_FROM_TOK = 41,                  /* FROM_TOK  */
  YYSYMBOL_GLOBAL_TOK = 42,                /* GLOBAL_TOK  */
  YYSYMBOL_IF_TOK = 43,                    /* IF_TOK  */
  YYSYMBOL_IGNORE_TOK = 44,                /* IGNORE_TOK  */
  YYSYMBOL_IMPORT_TOK = 45,                /* IMPORT_TOK  */
  YYSYMBOL_IN_TOK = 46,                    /* IN_TOK  */
  YYSYMBOL_INPUT_TOK = 47,                 /* INPUT_TOK  */
  YYSYMBOL_INCREASING_TOK = 48,            /* INCREASING_TOK  */
  YYSYMBOL_INTERACTIVE_TOK = 49,           /* INTERACTIVE_TOK  */
  YYSYMBOL_INDEPENDENT_TOK = 50,           /* INDEPENDENT_TOK  */
  YYSYMBOL_INTERSECTION_TOK = 51,          /* INTERSECTION_TOK  */
  YYSYMBOL_ISA_TOK = 52,                   /* ISA_TOK  */
  YYSYMBOL__IS_T = 53,                     /* _IS_T  */
  YYSYMBOL_ISREFINEDTO_TOK = 54,           /* ISREFINEDTO_TOK  */
  YYSYMBOL_LINK_TOK = 55,                  /* LINK_TOK  */
  YYSYMBOL_MAXIMIZE_TOK = 56,              /* MAXIMIZE_TOK  */
  YYSYMBOL_MAXINTEGER_TOK = 57,            /* MAXINTEGER_TOK  */
  YYSYMBOL_MAXREAL_TOK = 58,               /* MAXREAL_TOK  */
  YYSYMBOL_METHODS_TOK = 59,               /* METHODS_TOK  */
  YYSYMBOL_METHOD_TOK = 60,                /* METHOD_TOK  */
  YYSYMBOL_MINIMIZE_TOK = 61,              /* MINIMIZE_TOK  */
  YYSYMBOL_MODEL_TOK = 62,                 /* MODEL_TOK  */
  YYSYMBOL_NOT_TOK = 63,                   /* NOT_TOK  */
  YYSYMBOL_NOTES_TOK = 64,                 /* NOTES_TOK  */
  YYSYMBOL_OF_TOK = 65,                    /* OF_TOK  */
  YYSYMBOL_OPTION_TOK = 66,                /* OPTION_TOK  */
  YYSYMBOL_OR_TOK = 67,                    /* OR_TOK  */
  YYSYMBOL_OTHERWISE_TOK = 68,             /* OTHERWISE_TOK  */
  YYSYMBOL_OUTPUT_TOK = 69,                /* OUTPUT_TOK  */
  YYSYMBOL_PATCH_TOK = 70,                 /* PATCH_TOK  */
  YYSYMBOL_PROD_TOK = 71,                  /* PROD_TOK  */
  YYSYMBOL_PROVIDE_TOK = 72,               /* PROVIDE_TOK  */
  YYSYMBOL_REFINES_TOK = 73,               /* REFINES_TOK  */
  YYSYMBOL_REPLACE_TOK = 74,               /* REPLACE_TOK  */
  YYSYMBOL_REQUIRE_TOK = 75,               /* REQUIRE_TOK  */
  YYSYMBOL_RETURN_TOK = 76,                /* RETURN_TOK  */
  YYSYMBOL_RUN_TOK = 77,                   /* RUN_TOK  */
  YYSYMBOL_SATISFIED_TOK = 78,             /* SATISFIED_TOK  */
  YYSYMBOL_SELECT_TOK = 79,                /* SELECT_TOK  */
  YYSYMBOL_SIZE_TOK = 80,                  /* SIZE_TOK  */
  YYSYMBOL_SOLVE_TOK = 81,                 /* SOLVE_TOK  */
  YYSYMBOL_SOLVER_TOK = 82,                /* SOLVER_TOK  */
  YYSYMBOL_STOP_TOK = 83,                  /* STOP_TOK  */
  YYSYMBOL_SUCHTHAT_TOK = 84,              /* SUCHTHAT_TOK  */
  YYSYMBOL_SUM_TOK = 85,                   /* SUM_TOK  */
  YYSYMBOL_SWITCH_TOK = 86,                /* SWITCH_TOK  */
  YYSYMBOL_THEN_TOK = 87,                  /* THEN_TOK  */
  YYSYMBOL_TRUE_TOK = 88,                  /* TRUE_TOK  */
  YYSYMBOL_UNION_TOK = 89,                 /* UNION_TOK  */
  YYSYMBOL_UNITS_TOK = 90,                 /* UNITS_TOK  */
  YYSYMBOL_UNIVERSAL_TOK = 91,             /* UNIVERSAL_TOK  */
  YYSYMBOL_UNLINK_TOK = 92,                /* UNLINK_TOK  */
  YYSYMBOL_WHEN_TOK = 93,                  /* WHEN_TOK  */
  YYSYMBOL_WHERE_TOK = 94,                 /* WHERE_TOK  */
  YYSYMBOL_WHILE_TOK = 95,                 /* WHILE_TOK  */
  YYSYMBOL_WILLBE_TOK = 96,                /* WILLBE_TOK  */
  YYSYMBOL_WILLBETHESAME_TOK = 97,         /* WILLBETHESAME_TOK  */
  YYSYMBOL_WILLNOTBETHESAME_TOK = 98,      /* WILLNOTBETHESAME_TOK  */
  YYSYMBOL_ASSIGN_TOK = 99,                /* ASSIGN_TOK  */
  YYSYMBOL_CASSIGN_TOK = 100,              /* CASSIGN_TOK  */
  YYSYMBOL_DBLCOLON_TOK = 101,             /* DBLCOLON_TOK  */
  YYSYMBOL_USE_TOK = 102,                  /* USE_TOK  */
  YYSYMBOL_LEQ_TOK = 103,                  /* LEQ_TOK  */
  YYSYMBOL_GEQ_TOK = 104,                  /* GEQ_TOK  */
  YYSYMBOL_NEQ_TOK = 105,                  /* NEQ_TOK  */
  YYSYMBOL_DOTDOT_TOK = 106,               /* DOTDOT_TOK  */
  YYSYMBOL_WITH_TOK = 107,                 /* WITH_TOK  */
  YYSYMBOL_VALUE_TOK = 108,                /* VALUE_TOK  */
  YYSYMBOL_WITH_VALUE_T = 109,             /* WITH_VALUE_T  */
  YYSYMBOL_REAL_TOK = 110,                 /* REAL_TOK  */
  YYSYMBOL_INTEGER_TOK = 111,              /* INTEGER_TOK  */
  YYSYMBOL_IDENTIFIER_TOK = 112,           /* IDENTIFIER_TOK  */
  YYSYMBOL_BRACEDTEXT_TOK = 113,           /* BRACEDTEXT_TOK  */
  YYSYMBOL_SYMBOL_TOK = 114,               /* SYMBOL_TOK  */
  YYSYMBOL_DQUOTE_TOK = 115,               /* DQUOTE_TOK  */
  YYSYMBOL_116_ = 116,                     /* ','  */
  YYSYMBOL_117_ = 117,                     /* '|'  */
  YYSYMBOL_118_ = 118,                     /* '<'  */
  YYSYMBOL_119_ = 119,                     /* '='  */
  YYSYMBOL_120_ = 120,                     /* '>'  */
  YYSYMBOL_121_ = 121,                     /* '+'  */
  YYSYMBOL_122_ = 122,                     /* '-'  */
  YYSYMBOL_123_ = 123,                     /* '/'  */
  YYSYMBOL_124_ = 124,                     /* '*'  */
  YYSYMBOL_UMINUS_TOK = 125,               /* UMINUS_TOK  */
  YYSYMBOL_UPLUS_TOK = 126,                /* UPLUS_TOK  */
  YYSYMBOL_127_ = 127,                     /* '^'  */
  YYSYMBOL_128_ = 128,                     /* ';'  */
  YYSYMBOL_129_ = 129,                     /* '('  */
  YYSYMBOL_130_ = 130,                     /* ')'  */
  YYSYMBOL_131_ = 131,                     /* ':'  */
  YYSYMBOL_132_ = 132,                     /* '.'  */
  YYSYMBOL_133_ = 133,                     /* '['  */
  YYSYMBOL_134_ = 134,                     /* ']'  */
  YYSYMBOL_YYACCEPT = 135,                 /* $accept  */
  YYSYMBOL_definitions = 136,              /* definitions  */
  YYSYMBOL_definition = 137,               /* definition  */
  YYSYMBOL_global_def = 138,               /* global_def  */
  YYSYMBOL_require_file = 139,             /* require_file  */
  YYSYMBOL_provide_module = 140,           /* provide_module  */
  YYSYMBOL_import = 141,                   /* import  */
  YYSYMBOL_add_notes_def = 142,            /* add_notes_def  */
  YYSYMBOL_add_notes_head = 143,           /* add_notes_head  */
  YYSYMBOL_add_method_def = 144,           /* add_method_def  */
  YYSYMBOL_add_method_head = 145,          /* add_method_head  */
  YYSYMBOL_replace_method_def = 146,       /* replace_method_def  */
  YYSYMBOL_replace_method_head = 147,      /* replace_method_head  */
  YYSYMBOL_atom_def = 148,                 /* atom_def  */
  YYSYMBOL_atom_head = 149,                /* atom_head  */
  YYSYMBOL_atom_id = 150,                  /* atom_id  */
  YYSYMBOL_default_val = 151,              /* default_val  */
  YYSYMBOL_constant_def = 152,             /* constant_def  */
  YYSYMBOL_constant_head = 153,            /* constant_head  */
  YYSYMBOL_constant_val = 154,             /* constant_val  */
  YYSYMBOL_model_def = 155,                /* model_def  */
  YYSYMBOL_model_head = 156,               /* model_head  */
  YYSYMBOL_model_id = 157,                 /* model_id  */
  YYSYMBOL_optional_model_parameters = 158, /* optional_model_parameters  */
  YYSYMBOL_optional_parameter_wheres = 159, /* optional_parameter_wheres  */
  YYSYMBOL_optional_parameter_reduction = 160, /* optional_parameter_reduction  */
  YYSYMBOL_patch_def = 161,                /* patch_def  */
  YYSYMBOL_patch_head = 162,               /* patch_head  */
  YYSYMBOL_universal = 163,                /* universal  */
  YYSYMBOL_definition_def = 164,           /* definition_def  */
  YYSYMBOL_definition_id = 165,            /* definition_id  */
  YYSYMBOL_units_def = 166,                /* units_def  */
  YYSYMBOL_units_statement = 167,          /* units_statement  */
  YYSYMBOL_unitdeflist = 168,              /* unitdeflist  */
  YYSYMBOL_unitdef = 169,                  /* unitdef  */
  YYSYMBOL_methods = 170,                  /* methods  */
  YYSYMBOL_171_1 = 171,                    /* $@1  */
  YYSYMBOL_proclist = 172,                 /* proclist  */
  YYSYMBOL_proclistf = 173,                /* proclistf  */
  YYSYMBOL_procedure = 174,                /* procedure  */
  YYSYMBOL_procedure_id = 175,             /* procedure_id  */
  YYSYMBOL_fstatements = 176,              /* fstatements  */
  YYSYMBOL_statements = 177,               /* statements  */
  YYSYMBOL_statement = 178,                /* statement  */
  YYSYMBOL_complex_statement = 179,        /* complex_statement  */
  YYSYMBOL_isa_statement = 180,            /* isa_statement  */
  YYSYMBOL_willbe_statement = 181,         /* willbe_statement  */
  YYSYMBOL_aliases_statement = 182,        /* aliases_statement  */
  YYSYMBOL_optional_set_values = 183,      /* optional_set_values  */
  YYSYMBOL_is_statement = 184,             /* is_statement  */
  YYSYMBOL_isrefinedto_statement = 185,    /* isrefinedto_statement  */
  YYSYMBOL_call_identifier = 186,          /* call_identifier  */
  YYSYMBOL_type_identifier = 187,          /* type_identifier  */
  YYSYMBOL_optional_method = 188,          /* optional_method  */
  YYSYMBOL_optional_of = 189,              /* optional_of  */
  YYSYMBOL_optional_with_value = 190,      /* optional_with_value  */
  YYSYMBOL_arealike_statement = 191,       /* arealike_statement  */
  YYSYMBOL_link_statement = 192,           /* link_statement  */
  YYSYMBOL_unlink_statement = 193,         /* unlink_statement  */
  YYSYMBOL_der_statement = 194,            /* der_statement  */
  YYSYMBOL_independent_statement = 195,    /* independent_statement  */
  YYSYMBOL_arethesame_statement = 196,     /* arethesame_statement  */
  YYSYMBOL_willbethesame_statement = 197,  /* willbethesame_statement  */
  YYSYMBOL_willnotbethesame_statement = 198, /* willnotbethesame_statement  */
  YYSYMBOL_assignment_statement = 199,     /* assignment_statement  */
  YYSYMBOL_relation_statement = 200,       /* relation_statement  */
  YYSYMBOL_relation = 201,                 /* relation  */
  YYSYMBOL_blackbox_statement = 202,       /* blackbox_statement  */
  YYSYMBOL_input_args = 203,               /* input_args  */
  YYSYMBOL_output_args = 204,              /* output_args  */
  YYSYMBOL_data_args = 205,                /* data_args  */
  YYSYMBOL_glassbox_statement = 206,       /* glassbox_statement  */
  YYSYMBOL_optional_scope = 207,           /* optional_scope  */
  YYSYMBOL_for_statement = 208,            /* for_statement  */
  YYSYMBOL_optional_direction = 209,       /* optional_direction  */
  YYSYMBOL_forexprend = 210,               /* forexprend  */
  YYSYMBOL_run_statement = 211,            /* run_statement  */
  YYSYMBOL_fix_statement = 212,            /* fix_statement  */
  YYSYMBOL_fix_and_assign_statement = 213, /* fix_and_assign_statement  */
  YYSYMBOL_free_statement = 214,           /* free_statement  */
  YYSYMBOL_solver_statement = 215,         /* solver_statement  */
  YYSYMBOL_option_statement = 216,         /* option_statement  */
  YYSYMBOL_solve_statement = 217,          /* solve_statement  */
  YYSYMBOL_external_statement = 218,       /* external_statement  */
  YYSYMBOL_call_statement = 219,           /* call_statement  */
  YYSYMBOL_assert_statement = 220,         /* assert_statement  */
  YYSYMBOL_if_statement = 221,             /* if_statement  */
  YYSYMBOL_while_statement = 222,          /* while_statement  */
  YYSYMBOL_optional_else = 223,            /* optional_else  */
  YYSYMBOL_when_statement = 224,           /* when_statement  */
  YYSYMBOL_whenlist = 225,                 /* whenlist  */
  YYSYMBOL_whenlistf = 226,                /* whenlistf  */
  YYSYMBOL_flow_statement = 227,           /* flow_statement  */
  YYSYMBOL_use_statement = 228,            /* use_statement  */
  YYSYMBOL_select_statement = 229,         /* select_statement  */
  YYSYMBOL_selectlist = 230,               /* selectlist  */
  YYSYMBOL_selectlistf = 231,              /* selectlistf  */
  YYSYMBOL_switch_statement = 232,         /* switch_statement  */
  YYSYMBOL_switchlist = 233,               /* switchlist  */
  YYSYMBOL_switchlistf = 234,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 235,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 236,          /* notes_statement  */
  YYSYMBOL_notes_body = 237,               /* notes_body  */
  YYSYMBOL_noteslist = 238,                /* noteslist  */
  YYSYMBOL_fvarlist = 239,                 /* fvarlist  */
  YYSYMBOL_varlist = 240,                  /* varlist  */
  YYSYMBOL_fname = 241,                    /* fname  */
  YYSYMBOL_name = 242,                     /* name  */
  YYSYMBOL_end = 243,                      /* end  */
  YYSYMBOL_optional_bracedtext = 244,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 245,           /* optional_notes  */
  YYSYMBOL_set = 246,                      /* set  */
  YYSYMBOL_setexprlist = 247,              /* setexprlist  */
  YYSYMBOL_number = 248,                   /* number  */
  YYSYMBOL_realnumber = 249,               /* realnumber  */
  YYSYMBOL_opunits = 250,                  /* opunits  */
  YYSYMBOL_dims = 251,                     /* dims  */
  YYSYMBOL_dimensions = 252,               /* dimensions  */
  YYSYMBOL_dimexpr = 253,                  /* dimexpr  */
  YYSYMBOL_fraction = 254,                 /* fraction  */
  YYSYMBOL_fractail = 255,                 /* fractail  */
  YYSYMBOL_optional_sign = 256,            /* optional_sign  */
  YYSYMBOL_expr = 257,                     /* expr  */
  YYSYMBOL_relop = 258,                    /* relop  */
  YYSYMBOL_logrelop = 259                  /* logrelop  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1200

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  135
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  125
/* YYNRULES -- Number of rules.  */
#define YYNRULES  327
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  666

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   372


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     129,   130,   124,   121,   116,   122,   132,   123,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   131,   128,
     118,   119,   120,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   133,     2,   134,   127,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   117,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   125,   126
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   424,   424,   426,   430,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   450,   495,
     499,   504,   512,   516,   521,   529,   538,   550,   581,   589,
     612,   625,   633,   654,   666,   674,   726,   737,   746,   751,
     756,   763,   770,   780,   819,   854,   859,   864,   871,   878,
     888,   929,   938,   951,   960,   963,   971,   974,   982,   985,
     992,  1024,  1038,  1041,  1048,  1078,  1087,  1092,  1112,  1115,
    1123,  1133,  1137,  1136,  1150,  1158,  1161,  1186,  1200,  1209,
    1217,  1220,  1228,  1235,  1243,  1244,  1245,  1246,  1247,  1248,
    1249,  1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,  1258,
    1259,  1260,  1261,  1262,  1263,  1264,  1265,  1266,  1267,  1268,
    1269,  1270,  1271,  1272,  1273,  1274,  1275,  1276,  1277,  1278,
    1279,  1283,  1288,  1329,  1358,  1362,  1413,  1416,  1423,  1435,
    1462,  1467,  1475,  1480,  1489,  1492,  1500,  1503,  1511,  1514,
    1521,  1528,  1532,  1536,  1543,  1547,  1554,  1563,  1572,  1579,
    1586,  1593,  1597,  1604,  1617,  1634,  1644,  1652,  1663,  1684,
    1691,  1699,  1702,  1709,  1730,  1733,  1740,  1762,  1765,  1769,
    1776,  1780,  1784,  1788,  1795,  1799,  1806,  1814,  1828,  1835,
    1843,  1851,  1859,  1870,  1881,  1887,  1897,  1907,  1910,  1917,
    1928,  1940,  1947,  1957,  1964,  1968,  1972,  1976,  1983,  1987,
    1991,  1995,  1999,  2006,  2013,  2024,  2034,  2041,  2045,  2049,
    2053,  2060,  2071,  2081,  2088,  2092,  2096,  2100,  2107,  2117,
    2154,  2163,  2178,  2183,  2192,  2203,  2207,  2212,  2225,  2245,
    2249,  2254,  2269,  2274,  2279,  2284,  2289,  2294,  2299,  2304,
    2309,  2314,  2319,  2324,  2329,  2334,  2343,  2346,  2354,  2357,
    2364,  2369,  2375,  2379,  2383,  2388,  2396,  2402,  2411,  2415,
    2437,  2441,  2462,  2466,  2471,  2477,  2481,  2488,  2492,  2496,
    2500,  2504,  2508,  2515,  2522,  2526,  2534,  2537,  2541,  2548,
    2552,  2556,  2560,  2564,  2568,  2572,  2576,  2580,  2584,  2588,
    2593,  2598,  2603,  2608,  2613,  2618,  2623,  2627,  2632,  2637,
    2642,  2647,  2652,  2656,  2660,  2664,  2668,  2675,  2679,  2686,
    2690,  2697,  2701,  2708,  2712,  2719,  2723,  2730,  2734,  2745,
    2752,  2756,  2760,  2764,  2768,  2772,  2779,  2783
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ADD_TOK",
  "ALIASES_TOK", "AND_TOK", "ANY_TOK", "AREALIKE_TOK", "ARETHESAME_TOK",
  "ARRAY_TOK", "ASSERT_TOK", "ATOM_TOK", "BEQ_TOK", "BNE_TOK", "BREAK_TOK",
  "CALL_TOK", "CARD_TOK", "CASE_TOK", "CHOICE_TOK", "CHECK_TOK",
  "CONDITIONAL_TOK", "CONSTANT_TOK", "CONTINUE_TOK", "CREATE_TOK",
  "DATA_TOK", "DECREASING_TOK", "DEFAULT_TOK", "DEFINITION_TOK", "DER_TOK",
  "DIMENSION_TOK", "DIMENSIONLESS_TOK", "DO_TOK", "ELSE_TOK", "END_TOK",
  "EXPECT_TOK", "EXTERNAL_TOK", "FALSE_TOK", "FALLTHRU_TOK", "FIX_TOK",
  "FOR_TOK", "FREE_TOK", "FROM_TOK", "GLOBAL_TOK", "IF_TOK", "IGNORE_TOK",
  "IMPORT_TOK", "IN_TOK", "INPUT_TOK", "INCREASING_TOK", "INTERACTIVE_TOK",
  "INDEPENDENT_TOK", "INTERSECTION_TOK", "ISA_TOK", "_IS_T",
  "ISREFINEDTO_TOK", "LINK_TOK", "MAXIMIZE_TOK", "MAXINTEGER_TOK",
  "MAXREAL_TOK", "METHODS_TOK", "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK",
  "NOT_TOK", "NOTES_TOK", "OF_TOK", "OPTION_TOK", "OR_TOK",
  "OTHERWISE_TOK", "OUTPUT_TOK", "PATCH_TOK", "PROD_TOK", "PROVIDE_TOK",
  "REFINES_TOK", "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK",
  "SATISFIED_TOK", "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK", "SOLVER_TOK",
  "STOP_TOK", "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK", "THEN_TOK",
  "TRUE_TOK", "UNION_TOK", "UNITS_TOK", "UNIVERSAL_TOK", "UNLINK_TOK",
  "WHEN_TOK", "WHERE_TOK", "WHILE_TOK", "WILLBE_TOK", "WILLBETHESAME_TOK",
  "WILLNOTBETHESAME_TOK", "ASSIGN_TOK", "CASSIGN_TOK", "DBLCOLON_TOK",
  "USE_TOK", "LEQ_TOK", "GEQ_TOK", "NEQ_TOK", "DOTDOT_TOK", "WITH_TOK",
  "VALUE_TOK", "WITH_VALUE_T", "REAL_TOK", "INTEGER_TOK", "IDENTIFIER_TOK",
  "BRACEDTEXT_TOK", "SYMBOL_TOK", "DQUOTE_TOK", "','", "'|'", "'<'", "'='",
  "'>'", "'+'", "'-'", "'/'", "'*'", "UMINUS_TOK", "UPLUS_TOK", "'^'",
  "';'", "'('", "')'", "':'", "'.'", "'['", "']'", "$accept",
  "definitions", "definition", "global_def", "require_file",
  "provide_module", "import", "add_notes_def", "add_notes_head",
  "add_method_def", "add_method_head", "replace_method_def",
  "replace_method_head", "atom_def", "atom_head", "atom_id", "default_val",
  "constant_def", "constant_head", "constant_val", "model_def",
  "model_head", "model_id", "optional_model_parameters",
  "optional_parameter_wheres", "optional_parameter_reduction", "patch_def",
  "patch_head", "universal", "definition_def", "definition_id",
  "units_def", "units_statement", "unitdeflist", "unitdef", "methods",
  "$@1", "proclist", "proclistf", "procedure", "procedure_id",
  "fstatements", "statements", "statement", "complex_statement",
  "isa_statement", "willbe_statement", "aliases_statement",
  "optional_set_values", "is_statement", "isrefinedto_statement",
  "call_identifier", "type_identifier", "optional_method", "optional_of",
  "optional_with_value", "arealike_statement", "link_statement",
  "unlink_statement", "der_statement", "independent_statement",
  "arethesame_statement", "willbethesame_statement",
  "willnotbethesame_statement", "assignment_statement",
  "relation_statement", "relation", "blackbox_statement", "input_args",
  "output_args", "data_args", "glassbox_statement", "optional_scope",
  "for_statement", "optional_direction", "forexprend", "run_statement",
  "fix_statement", "fix_and_assign_statement", "free_statement",
  "solver_statement", "option_statement", "solve_statement",
  "external_statement", "call_statement", "assert_statement",
  "if_statement", "while_statement", "optional_else", "when_statement",
  "whenlist", "whenlistf", "flow_statement", "use_statement",
  "select_statement", "selectlist", "selectlistf", "switch_statement",
  "switchlist", "switchlistf", "conditional_statement", "notes_statement",
  "notes_body", "noteslist", "fvarlist", "varlist", "fname", "name", "end",
  "optional_bracedtext", "optional_notes", "set", "setexprlist", "number",
  "realnumber", "opunits", "dims", "dimensions", "dimexpr", "fraction",
  "fractail", "optional_sign", "expr", "relop", "logrelop", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-478)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-226)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -478,   329,  -478,  -478,   -40,   -91,   -64,   -60,   -41,    11,
      42,   107,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
     -34,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
       5,  -478,  -478,  -478,   -17,    78,    86,  -478,  -478,   102,
      48,   128,  -478,    56,   -12,   144,    69,     6,   -13,   127,
     -19,   210,   197,   210,   207,   673,   157,   160,   163,  -478,
     205,  -478,  -478,   150,   207,  -478,   -15,   169,   210,   167,
    -478,   171,  -478,  -478,   172,   822,   -14,  -478,  -478,   231,
     166,  -478,  -478,   127,   174,    29,  -478,   -73,   127,   165,
     170,   179,  -478,   173,   178,  -478,   210,   181,  -478,   822,
    -478,   182,    40,    62,  -478,  -478,   168,   188,  -478,  -478,
     127,   190,   127,   822,   127,    84,   183,   822,  -478,  -478,
     822,   822,   -34,   191,    87,  -478,   127,   184,   -97,  -478,
     195,   198,   101,   -89,  -478,   104,   185,   -81,   822,   127,
     203,   206,   189,  -478,   822,   822,   822,   822,  -478,   192,
     194,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,    39,
      65,  -478,  1012,  -478,   250,  -478,   207,   213,   207,  -478,
     234,   210,   269,   208,   273,   211,   216,   217,  -478,  -478,
     200,   222,   141,   279,   218,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,   235,   236,
    -478,   127,  -478,  -478,  -478,   127,  -478,  -478,  -478,  -478,
    -478,  -478,   219,  -478,  1012,   223,  -478,   822,   822,   822,
     822,   210,   127,   224,  -478,  -478,   142,   309,  -478,   516,
    -478,   822,   822,   -27,  1012,  1012,  -478,   -19,   822,   822,
     822,   256,   127,   127,    10,  -478,  -478,  -478,   822,   822,
     127,    18,   822,   822,    54,   127,    21,   918,  -478,  -478,
    -478,  -478,   822,   232,   232,   467,   226,  -478,  -478,   -61,
    -478,  -478,   246,   249,   246,   246,  -478,  -478,   822,   822,
     754,   822,  -478,  -478,   822,   822,   822,  -478,  -478,  -478,
     822,  -478,  -478,  -478,   822,   822,   822,   822,   822,   822,
     822,   252,   210,   121,   210,   237,   233,   -62,   238,   240,
    -478,   253,   241,  -478,  -478,  -478,  -478,   822,   822,   244,
    -478,   245,  -478,  -478,   210,  -478,  -478,   822,   247,   242,
     248,   251,  -478,   254,   127,   822,  -478,   258,   255,   259,
     264,   265,  -478,  1012,   260,   261,   127,    -8,   262,   822,
     263,   210,    32,   266,   272,   268,   822,   276,   210,    37,
     270,   274,   295,   296,   283,   822,   284,   210,    41,  -478,
     857,  -478,  -478,   127,  -478,   257,   317,   317,  -478,   317,
    1012,  1012,    -2,   285,  -478,   505,   505,   505,  1044,  1044,
      51,    51,   232,   232,   232,   105,  1073,   121,   288,     1,
    -478,   391,   290,  -478,  -478,   311,  -478,  -478,  -478,  -478,
    -478,   983,  1012,  -478,  -478,   293,   294,  -478,  -478,  -478,
    -478,  -478,   297,   889,   393,  -478,  -478,   312,   127,   127,
    -478,  -478,  -478,   138,  -478,    10,   298,  -478,  -478,   822,
     299,  -478,  -478,    18,   303,  -478,  -478,   822,   304,  -478,
    -478,   127,   127,    21,   305,  -478,  -478,   822,   308,   210,
    -478,   310,   822,   316,   332,  -478,   332,   127,    21,   822,
     342,  -478,  -478,  -478,  -478,   -67,  -478,    88,   -18,   319,
    -478,   314,   320,   822,  -478,  -478,  -478,  -478,  -478,   175,
    -478,   210,   336,   315,   323,   206,   324,   210,  -478,  -478,
     326,  -478,   210,  -478,  -478,   330,  -478,   333,   334,   210,
    -478,  -478,   331,  -478,  -478,   366,   335,  -478,   822,  -478,
    -478,   337,   210,   340,    71,   -75,    -7,   351,    77,   -67,
     -67,   131,  -478,  -478,  -478,  -478,  -478,   145,  -478,  -478,
    -478,   341,  1012,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
     127,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,   127,  -478,
    1012,    21,  -478,   127,   359,   424,  -478,  -478,  -478,   145,
     345,  -478,   347,   347,  -478,   -35,   206,  -478,  -478,   355,
    -478,   210,   357,  -478,  -478,  -478,   436,   210,   361,   363,
     360,  -478,  -478,  -478,  -478,   381,  -478,  -478,  -478,  -478,
     383,  -478,   127,   367,   427,   452,   380,   440,   377,  -478,
    -478,   127,  -478,   398,   400,   486,  -478,   385,   408,  -478,
    -478,   390,  -478,   822,   392,  -478
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    17,     0,     0,     0,     0,     0,     0,
       0,     0,    68,    63,     3,    16,     4,     5,     6,     9,
       0,     7,    75,     8,    75,    11,    10,    12,    14,    80,
       0,    13,    80,    15,     0,     0,     0,    65,    80,     0,
       0,     0,   229,     0,    24,     0,     0,    21,     0,     0,
       0,     0,    74,     0,    71,     0,     0,     0,     0,    80,
       0,    43,    80,    54,    71,    66,     0,     0,     0,     0,
      26,     0,    22,    23,     0,   251,     0,    19,    20,   245,
       0,    69,    67,   220,     0,   224,   225,   248,     0,     0,
       0,     0,    76,     0,     0,    72,     0,     0,   285,     0,
     198,     0,     0,     0,    80,   199,     0,     0,   284,   200,
       0,     0,     0,     0,     0,     0,     0,     0,   280,   282,
       0,     0,     0,     0,     0,   201,     0,     0,     0,   181,
       0,   246,     0,     0,   283,     0,     0,     0,     0,     0,
     260,   279,   229,   286,     0,     0,     0,   251,   120,     0,
       0,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,   153,   100,    99,   103,
     104,   105,   121,   106,   107,   109,   108,   102,   101,   110,
     111,   112,   113,   115,   114,   116,   117,   118,   119,     0,
     287,   281,   155,    37,     0,    53,    71,     0,    71,    80,
      56,     0,     0,     0,   134,     0,     0,     0,   230,   287,
       0,   250,   252,     0,     0,   232,   233,   241,   234,   235,
     236,   237,   238,   239,   240,   242,   243,   244,     0,     0,
     222,     0,   227,   249,   228,   221,    27,    29,    78,    80,
      32,    75,     0,    83,   184,   130,   183,   251,   251,   251,
     251,     0,     0,     0,   177,   176,   225,     0,   178,     0,
     147,   251,   251,     0,   157,   156,   296,     0,     0,   251,
     251,   174,     0,     0,     0,   179,   247,   202,   251,   251,
       0,     0,   251,   251,     0,     0,     0,     0,   203,   261,
     258,   259,     0,   302,   303,     0,     0,    81,    82,     0,
     140,   148,     0,     0,     0,     0,   149,   150,     0,     0,
       0,     0,   326,   327,     0,     0,     0,   323,   324,   325,
       0,   321,   320,   322,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   264,     0,     0,     0,     0,     0,     0,
      30,     0,     0,    18,    25,    61,   231,     0,     0,     0,
      33,     0,   223,   226,     0,    73,    60,   251,     0,     0,
       0,     0,   218,     0,     0,     0,    80,     0,     0,     0,
       0,     0,   219,   180,     0,     0,     0,     0,     0,   251,
       0,     0,   206,     0,     0,     0,   251,     0,     0,   213,
       0,     0,     0,     0,     0,   251,     0,     0,   193,    80,
       0,   319,   288,     0,   124,   132,   136,   136,   129,   136,
     151,   152,     0,   229,   154,   294,   299,   295,   301,   300,
     289,   290,   292,   291,   293,   297,   298,   264,     0,     0,
     263,    38,     0,    55,    80,     0,    51,    64,    31,   135,
      28,   254,   253,    34,    70,     0,     0,   314,   315,   316,
     317,   146,     0,   167,   187,   312,   313,     0,     0,     0,
     308,   309,   175,     0,   305,     0,     0,    80,   204,   251,
       0,   306,   307,     0,     0,    80,   211,   251,     0,   310,
     311,     0,     0,     0,     0,    80,   189,   251,     0,     0,
     318,     0,   251,     0,   138,   128,   138,     0,     0,     0,
      45,    35,   268,   267,   265,     0,   262,   266,   276,     0,
      50,     0,    58,     0,    77,   131,   182,   169,   168,     0,
      80,     0,     0,     0,     0,     0,     0,     0,    80,   208,
       0,    80,     0,    80,   215,     0,    80,     0,     0,     0,
      80,   195,     0,    80,   186,     0,     0,   137,     0,   122,
     123,     0,     0,     0,     0,   287,   276,   248,     0,     0,
       0,   276,    40,    41,    42,   277,   278,     0,    36,    57,
      80,     0,   255,   172,   170,   173,   171,    80,   188,   185,
       0,   142,   143,   304,   205,   207,    80,   210,   212,   214,
      80,   217,   144,   145,   191,   194,    80,   197,     0,   133,
     139,     0,   190,     0,     0,     0,    48,    47,    49,     0,
       0,   272,   269,   270,   271,     0,   256,    39,   257,     0,
      52,     0,     0,   209,   216,   196,     0,     0,   161,     0,
       0,   159,    46,    44,   274,     0,   273,    59,   166,   141,
       0,   192,     0,     0,     0,   164,     0,     0,     0,   158,
     160,     0,   163,     0,     0,     0,   165,     0,   126,   162,
     275,     0,   125,   251,     0,   127
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,   465,  -478,  -478,   -56,  -478,   -23,  -478,  -478,
    -478,   -29,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,   -28,  -478,  -224,    27,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,   414,  -478,   215,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -473,  -478,  -478,  -478,  -478,    61,  -478,  -478,    57,  -478,
    -478,  -478,   405,   444,    19,  -478,   -49,   229,   -46,  -478,
     -24,   -90,  -478,   -74,  -461,  -478,   109,  -478,  -477,  -478,
    -478,  -412,   338,  -478,  -478
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    59,    60,   509,    26,    61,   557,
      27,    62,    63,   200,   337,   571,    28,    29,    30,    31,
      32,    33,    34,    48,    81,    96,   241,    51,    52,    92,
      93,    54,    55,   149,   150,   151,   152,   153,   662,   154,
     155,   246,   406,   342,   494,   549,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   553,   628,
     643,   168,   652,   169,   519,   577,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   521,   182,
     397,   398,   183,   184,   185,   381,   382,   186,   388,   389,
     187,   188,    50,    83,    84,    85,   209,    87,    82,   277,
     234,   210,   211,   617,   191,   290,   431,   506,   507,   614,
     636,   567,   212,   329,   330
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      86,    53,   526,    64,    89,    90,   190,    94,   201,    68,
     539,   435,   202,   213,    79,    42,    56,   369,   562,    35,
      79,    37,   205,    42,    36,   552,    57,   379,   558,   606,
     196,    42,   273,   198,    86,   386,   232,  -225,   395,    86,
     280,  -225,   233,   299,   502,   503,   300,   301,   285,   469,
     242,    42,    39,  -225,   477,    40,  -225,   296,   487,    74,
      75,   256,   505,    86,    38,    86,   436,    58,   403,  -225,
     563,    41,  -225,  -225,   189,   251,   634,   271,   380,    86,
      49,   607,   612,   613,    86,    42,   387,   370,    86,   396,
     288,   302,   303,   304,   635,    88,   564,   203,   214,    80,
     470,    45,   229,   565,   566,   478,   618,   608,   463,   488,
      42,    65,   502,   503,   565,   566,    73,  -225,  -225,  -225,
      74,    75,   464,    42,    66,   504,    43,   497,   627,   255,
     505,   258,    67,   260,    78,   305,   306,   307,    74,    75,
     332,    42,   334,    69,   609,   231,   311,   274,   618,   615,
     429,   430,   281,   312,   313,   338,   286,   358,   359,   360,
     361,  -225,  -225,  -225,   308,   309,    42,    71,   392,   247,
     335,   367,   368,   248,   326,   327,    70,  -225,   328,   374,
     375,  -225,   353,   495,    72,   496,    86,   314,   383,   384,
      76,   249,   390,   391,   573,   250,   310,    77,   574,   604,
     559,   560,   605,    86,   561,   362,   575,   611,   315,   576,
     354,   559,   560,   261,   371,   561,   269,   262,   355,    42,
     270,   372,    46,   377,    86,   316,   324,   325,   326,   327,
     278,    86,   328,   282,   279,   393,    86,   283,    44,    42,
      47,   308,   309,    79,   317,   318,   319,   348,   140,   525,
     404,   215,   565,   566,   229,   140,   616,    91,   320,   321,
     322,   323,   324,   325,   326,   327,    95,   446,   328,   193,
     216,   363,   194,   217,   218,   195,   408,   409,   197,   199,
     219,   204,   206,   207,   208,   228,   428,   230,   432,   466,
     220,   238,   378,   236,   245,   221,   474,   252,   237,   385,
     253,   239,   257,   268,   394,   484,   240,   275,   445,   243,
     222,   276,   263,   272,   284,    86,   289,   223,   292,   291,
     297,   224,   298,   331,   225,   333,   226,   462,   336,     2,
       3,   339,     4,   341,   346,   468,   340,   454,   347,   343,
     -62,   349,   476,   227,   344,   345,   350,   356,   351,   352,
     -62,   486,   357,   364,    86,   365,     5,   376,   405,   328,
     402,   407,   434,    86,   427,   439,   437,   433,   438,   440,
     489,     6,   443,   444,     7,   457,   448,   447,   449,   530,
     458,   459,   493,   452,   451,   450,   492,   535,   455,   456,
     460,   -62,   465,   192,   467,   461,   471,   542,   473,     8,
     479,     9,   546,    10,    11,   511,   472,   475,   480,    86,
      86,   481,   482,   483,   499,   485,   501,   508,   510,    12,
      13,   514,   491,   512,   515,   520,   522,   516,   547,   528,
     531,   498,    86,    86,   533,   536,   540,   244,   529,   543,
     545,   548,   556,   544,   569,   581,   534,   568,    86,   570,
     555,   259,   580,   582,   583,   264,   541,   586,   265,   266,
     598,   590,   596,   592,   593,   599,   233,   601,   603,   620,
     630,   631,   311,   633,   561,   579,   287,   523,   524,   312,
     313,   584,   293,   294,   295,   637,   588,   639,   640,   642,
     645,   578,   646,   594,   644,   647,   650,   649,   651,   585,
     537,   538,   587,   653,   589,   654,   602,   591,   655,   657,
     659,   595,   658,   314,   597,   660,   551,   661,   554,   663,
     148,   311,   665,   550,   254,   414,   527,   267,   312,   313,
     532,    86,   235,   610,   315,   632,   500,     0,     0,     0,
       0,   619,     0,     0,     0,     0,     0,     0,   621,    86,
       0,   316,     0,     0,    86,     0,     0,   623,     0,     0,
       0,   624,   314,     0,     0,     0,     0,   625,     0,     0,
     317,   318,   319,   664,     0,   638,     0,     0,     0,     0,
       0,   641,     0,   315,   320,   321,   322,   323,   324,   325,
     326,   327,     0,   648,   328,     0,     0,   401,     0,   622,
     316,     0,   656,   366,     0,     0,   373,     0,   317,   318,
     319,     0,     0,     0,     0,     0,     0,   626,     0,   317,
     318,   319,   629,   321,   322,   323,   324,   325,   326,   327,
     400,     0,   328,   320,   321,   322,   323,   324,   325,   326,
     327,     0,     0,   328,     0,     0,   410,   411,   192,   415,
       0,     0,   416,   417,   418,     0,     0,     0,   419,     0,
       0,     0,   420,   421,   422,   423,   424,   425,   426,     0,
       0,     0,     0,     0,    97,     0,     0,     0,     0,    98,
       0,     0,     0,    99,     0,   441,   442,   100,   101,   102,
     -79,   103,     0,   104,     0,   105,     0,     0,     0,     0,
       0,   106,     0,   453,     0,   -79,   -79,     0,   107,   108,
     109,   110,   111,   112,     0,     0,   113,     0,     0,     0,
       0,     0,     0,   114,   115,     0,     0,     0,   116,   117,
     118,   119,   -79,     0,   120,     0,   121,   122,     0,   123,
       0,   -79,     0,     0,   124,     0,     0,     0,     0,   125,
     126,   127,   128,     0,   129,   130,   131,     0,   132,   133,
      98,   134,   135,    12,     0,   136,   137,     0,   138,     0,
     102,     0,   103,     0,     0,   139,     0,     0,     0,     0,
       0,     0,     0,   140,   141,   142,     0,   143,     0,     0,
     108,     0,     0,     0,   144,   145,     0,     0,     0,     0,
       0,     0,   146,   -79,     0,   115,   147,     0,     0,     0,
     117,   118,   119,     0,     0,   120,     0,   121,     0,     0,
       0,     0,     0,     0,     0,   124,     0,     0,    98,     0,
       0,     0,   127,     0,     0,     0,     0,   400,   102,   132,
     103,     0,   134,   135,     0,     0,     0,   412,     0,     0,
       0,   572,     0,     0,     0,     0,     0,     0,   108,     0,
       0,     0,   311,     0,   140,   141,   413,     0,   143,   312,
     313,     0,     0,   115,     0,   144,   145,     0,     0,   118,
     119,     0,     0,   146,     0,   121,   600,   147,     0,     0,
       0,     0,     0,   124,   311,     0,     0,     0,     0,     0,
     127,   312,   313,   314,     0,     0,     0,   132,     0,     0,
     134,   135,     0,     0,   517,     0,     0,     0,     0,     0,
       0,     0,     0,   311,   315,     0,     0,     0,     0,     0,
     312,   313,   140,   141,   142,   314,   143,   518,     0,     0,
       0,   316,     0,   144,   145,     0,     0,     0,     0,   399,
       0,   146,     0,     0,     0,   147,   315,     0,     0,     0,
     317,   318,   319,     0,   314,     0,     0,     0,     0,     0,
       0,     0,     0,   316,   320,   321,   322,   323,   324,   325,
     326,   327,     0,     0,   328,   315,     0,   490,   311,     0,
       0,     0,   317,   318,   319,   312,   313,     0,     0,     0,
       0,     0,   316,     0,     0,     0,   320,   321,   322,   323,
     324,   325,   326,   327,     0,     0,   328,   311,     0,     0,
       0,   317,   318,   319,   312,   313,     0,     0,     0,   314,
       0,     0,     0,     0,     0,   320,   321,   322,   323,   324,
     325,   326,   327,     0,     0,   328,     0,     0,     0,   311,
     315,     0,     0,     0,     0,     0,   312,   313,   314,     0,
       0,     0,     0,     0,     0,     0,     0,   316,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   311,   315,
       0,     0,     0,     0,     0,     0,   317,   318,   319,   513,
     314,     0,     0,     0,     0,     0,   316,     0,     0,     0,
     320,   321,   322,   323,   324,   325,   326,   327,     0,     0,
     328,   315,     0,     0,     0,   317,   318,   319,     0,   314,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   320,
     321,   322,   323,   324,   325,   326,   327,     0,     0,   328,
     315,     0,     0,     0,     0,     0,     0,   317,   318,   319,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   321,   322,   323,   324,   325,   326,   327,     0,
       0,   328,     0,     0,     0,     0,   317,   318,   319,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   321,   322,   323,   324,   325,   326,   327,     0,     0,
     328
};

static const yytype_int16 yycheck[] =
{
      49,    24,   463,    32,    50,    51,    55,    53,    64,    38,
     483,    73,    27,    27,    33,   112,    11,    44,    36,    59,
      33,   112,    68,   112,    64,   498,    21,    17,   505,    36,
      59,   112,   129,    62,    83,    17,    85,   112,    17,    88,
     129,   116,   115,     4,   111,   112,     7,     8,   129,    17,
      96,   112,   112,   128,    17,   115,   131,   147,    17,   132,
     133,   110,   129,   112,   128,   114,   128,    62,   129,     4,
      88,   112,     7,     8,    55,   104,   111,   126,    68,   128,
     114,    88,   559,   560,   133,   112,    68,   114,   137,    68,
     139,    52,    53,    54,   129,   114,   114,   112,   112,   112,
      68,    59,    83,   121,   122,    68,   567,   114,   116,    68,
     112,   128,   111,   112,   121,   122,   128,    52,    53,    54,
     132,   133,   130,   112,    46,   124,   115,   129,   601,   110,
     129,   112,    46,   114,   128,    96,    97,    98,   132,   133,
     196,   112,   198,    41,   556,   116,     5,   128,   609,   561,
      29,    30,   133,    12,    13,   201,   137,   247,   248,   249,
     250,    96,    97,    98,    99,   100,   112,    39,   114,   129,
     199,   261,   262,   133,   123,   124,   128,   112,   127,   269,
     270,   116,   231,   407,   128,   409,   235,    46,   278,   279,
      46,   129,   282,   283,    19,   133,   131,   128,    23,   128,
     123,   124,   131,   252,   127,   251,    31,   130,    67,    34,
     239,   123,   124,   129,   263,   127,   129,   133,   241,   112,
     133,   267,   115,   272,   273,    84,   121,   122,   123,   124,
     129,   280,   127,   129,   133,   284,   285,   133,     9,   112,
      11,    99,   100,    33,   103,   104,   105,   106,   110,   111,
     299,    20,   121,   122,   235,   110,   111,    60,   117,   118,
     119,   120,   121,   122,   123,   124,    59,   357,   127,   112,
      39,   252,   112,    42,    43,   112,   304,   305,    73,   129,
      49,   112,   115,   112,   112,   119,   332,   113,   334,   379,
      59,   112,   273,   128,   112,    64,   386,   129,   128,   280,
     112,   128,   112,   112,   285,   395,   128,   112,   354,   128,
      79,   113,   129,   129,   129,   364,   113,    86,   129,   113,
     128,    90,   128,    73,    93,   112,    95,   376,    94,     0,
       1,    62,     3,    60,   134,   381,   128,   366,   116,   128,
      11,    62,   388,   112,   128,   128,   128,   128,   113,   113,
      21,   397,   129,   129,   403,    46,    27,   101,   112,   127,
     134,   112,   129,   412,   112,   112,   128,   130,   128,   128,
     399,    42,   128,   128,    45,   116,   134,   130,   130,   469,
     116,   116,    65,   364,   130,   134,   129,   477,   130,   134,
     130,    62,   130,    55,   131,   134,   130,   487,   130,    70,
     130,    72,   492,    74,    75,   434,   134,   131,   134,   458,
     459,   116,   116,   130,   129,   131,   128,    26,   128,    90,
      91,   128,   403,   112,   130,    32,   114,   130,   112,   131,
     131,   412,   481,   482,   131,   131,   131,    99,   467,   131,
     130,   109,   100,   489,   130,   130,   475,   128,   497,   129,
     499,   113,   116,   130,   130,   117,   485,   131,   120,   121,
      94,   131,   131,   130,   130,   130,   115,   130,   128,   128,
     111,    47,     5,   128,   127,   521,   138,   458,   459,    12,
      13,   527,   144,   145,   146,   130,   532,   130,    52,   128,
     130,   520,   111,   539,   131,   112,    69,   130,    46,   528,
     481,   482,   531,   123,   533,    65,   552,   536,   131,   111,
      24,   540,   112,    46,   543,   130,   497,   109,   499,   129,
      55,     5,   130,   496,   110,   310,   465,   122,    12,    13,
     473,   580,    88,   557,    67,   609,   427,    -1,    -1,    -1,
      -1,   570,    -1,    -1,    -1,    -1,    -1,    -1,   577,   598,
      -1,    84,    -1,    -1,   603,    -1,    -1,   586,    -1,    -1,
      -1,   590,    46,    -1,    -1,    -1,    -1,   596,    -1,    -1,
     103,   104,   105,   663,    -1,   621,    -1,    -1,    -1,    -1,
      -1,   627,    -1,    67,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,   642,   127,    -1,    -1,   130,    -1,   580,
      84,    -1,   651,    87,    -1,    -1,   268,    -1,   103,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,   598,    -1,   103,
     104,   105,   603,   118,   119,   120,   121,   122,   123,   124,
     292,    -1,   127,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,    -1,   127,    -1,    -1,   308,   309,   310,   311,
      -1,    -1,   314,   315,   316,    -1,    -1,    -1,   320,    -1,
      -1,    -1,   324,   325,   326,   327,   328,   329,   330,    -1,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,     6,
      -1,    -1,    -1,    10,    -1,   347,   348,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,
      -1,    28,    -1,   365,    -1,    32,    33,    -1,    35,    36,
      37,    38,    39,    40,    -1,    -1,    43,    -1,    -1,    -1,
      -1,    -1,    -1,    50,    51,    -1,    -1,    -1,    55,    56,
      57,    58,    59,    -1,    61,    -1,    63,    64,    -1,    66,
      -1,    68,    -1,    -1,    71,    -1,    -1,    -1,    -1,    76,
      77,    78,    79,    -1,    81,    82,    83,    -1,    85,    86,
       6,    88,    89,    90,    -1,    92,    93,    -1,    95,    -1,
      16,    -1,    18,    -1,    -1,   102,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   110,   111,   112,    -1,   114,    -1,    -1,
      36,    -1,    -1,    -1,   121,   122,    -1,    -1,    -1,    -1,
      -1,    -1,   129,   130,    -1,    51,   133,    -1,    -1,    -1,
      56,    57,    58,    -1,    -1,    61,    -1,    63,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,     6,    -1,
      -1,    -1,    78,    -1,    -1,    -1,    -1,   499,    16,    85,
      18,    -1,    88,    89,    -1,    -1,    -1,    93,    -1,    -1,
      -1,   513,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,
      -1,    -1,     5,    -1,   110,   111,   112,    -1,   114,    12,
      13,    -1,    -1,    51,    -1,   121,   122,    -1,    -1,    57,
      58,    -1,    -1,   129,    -1,    63,   548,   133,    -1,    -1,
      -1,    -1,    -1,    71,     5,    -1,    -1,    -1,    -1,    -1,
      78,    12,    13,    46,    -1,    -1,    -1,    85,    -1,    -1,
      88,    89,    -1,    -1,    25,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     5,    67,    -1,    -1,    -1,    -1,    -1,
      12,    13,   110,   111,   112,    46,   114,    48,    -1,    -1,
      -1,    84,    -1,   121,   122,    -1,    -1,    -1,    -1,    31,
      -1,   129,    -1,    -1,    -1,   133,    67,    -1,    -1,    -1,
     103,   104,   105,    -1,    46,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,   127,    67,    -1,   130,     5,    -1,
      -1,    -1,   103,   104,   105,    12,    13,    -1,    -1,    -1,
      -1,    -1,    84,    -1,    -1,    -1,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,   127,     5,    -1,    -1,
      -1,   103,   104,   105,    12,    13,    -1,    -1,    -1,    46,
      -1,    -1,    -1,    -1,    -1,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    -1,   127,    -1,    -1,    -1,     5,
      67,    -1,    -1,    -1,    -1,    -1,    12,    13,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,    67,
      -1,    -1,    -1,    -1,    -1,    -1,   103,   104,   105,   106,
      46,    -1,    -1,    -1,    -1,    -1,    84,    -1,    -1,    -1,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
     127,    67,    -1,    -1,    -1,   103,   104,   105,    -1,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,   127,
      67,    -1,    -1,    -1,    -1,    -1,    -1,   103,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,   127,    -1,    -1,    -1,    -1,   103,   104,   105,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
     127
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   136,     0,     1,     3,    27,    42,    45,    70,    72,
      74,    75,    90,    91,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   152,   155,   161,   162,
     163,   164,   165,   166,   167,    59,    64,   112,   128,   112,
     115,   112,   112,   115,   242,    59,   115,   242,   168,   114,
     237,   172,   173,   172,   176,   177,    11,    21,    62,   149,
     150,   153,   156,   157,   176,   128,    46,    46,   176,    41,
     128,    39,   128,   128,   132,   133,    46,   128,   128,    33,
     112,   169,   243,   238,   239,   240,   241,   242,   114,   243,
     243,    60,   174,   175,   243,    59,   170,     1,     6,    10,
      14,    15,    16,    18,    20,    22,    28,    35,    36,    37,
      38,    39,    40,    43,    50,    51,    55,    56,    57,    58,
      61,    63,    64,    66,    71,    76,    77,    78,    79,    81,
      82,    83,    85,    86,    88,    89,    92,    93,    95,   102,
     110,   111,   112,   114,   121,   122,   129,   133,   167,   178,
     179,   180,   181,   182,   184,   185,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   206,   208,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   224,   227,   228,   229,   232,   235,   236,   239,
     241,   249,   257,   112,   112,   112,   176,    73,   176,   129,
     158,   170,    27,   112,   112,   243,   115,   112,   112,   241,
     246,   247,   257,    27,   112,    20,    39,    42,    43,    49,
      59,    64,    79,    86,    90,    93,    95,   112,   119,   239,
     113,   116,   241,   115,   245,   238,   128,   128,   112,   128,
     128,   171,   243,   128,   257,   112,   186,   129,   133,   129,
     133,   176,   129,   112,   199,   239,   241,   112,   239,   257,
     239,   129,   133,   129,   257,   257,   257,   237,   112,   129,
     133,   241,   129,   129,   239,   112,   113,   244,   129,   133,
     129,   239,   129,   133,   129,   129,   239,   257,   241,   113,
     250,   113,   129,   257,   257,   257,   246,   128,   128,     4,
       7,     8,    52,    53,    54,    96,    97,    98,    99,   100,
     131,     5,    12,    13,    46,    67,    84,   103,   104,   105,
     117,   118,   119,   120,   121,   122,   123,   124,   127,   258,
     259,    73,   170,   112,   170,   176,    94,   159,   243,    62,
     128,    60,   188,   128,   128,   128,   134,   116,   106,    62,
     128,   113,   113,   241,   176,   172,   128,   129,   246,   246,
     246,   246,   243,   239,   129,    46,    87,   246,   246,    44,
     114,   241,   243,   257,   246,   246,   101,   241,   239,    17,
      68,   230,   231,   246,   246,   239,    17,    68,   233,   234,
     246,   246,   114,   241,   239,    17,    68,   225,   226,    31,
     257,   130,   134,   129,   241,   112,   187,   112,   187,   187,
     257,   257,    93,   112,   201,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   112,   243,    29,
      30,   251,   243,   130,   129,    73,   128,   128,   128,   112,
     128,   257,   257,   128,   128,   243,   246,   130,   134,   130,
     134,   130,   239,   257,   176,   130,   134,   116,   116,   116,
     130,   134,   241,   116,   130,   130,   246,   131,   243,    17,
      68,   130,   134,   130,   246,   131,   243,    17,    68,   130,
     134,   116,   116,   130,   246,   131,   243,    17,    68,   176,
     130,   239,   129,    65,   189,   189,   189,   129,   239,   129,
     251,   128,   111,   112,   124,   129,   252,   253,    26,   151,
     128,   176,   112,   106,   128,   130,   130,    25,    48,   209,
      32,   223,   114,   239,   239,   111,   249,   230,   131,   176,
     246,   131,   233,   131,   176,   246,   131,   239,   239,   225,
     131,   176,   246,   131,   243,   130,   246,   112,   109,   190,
     190,   239,   225,   203,   239,   241,   100,   154,   253,   123,
     124,   127,    36,    88,   114,   121,   122,   256,   128,   130,
     129,   160,   257,    19,    23,    31,    34,   210,   176,   243,
     116,   130,   130,   130,   243,   176,   131,   176,   243,   176,
     131,   176,   130,   130,   243,   176,   131,   176,    94,   130,
     257,   130,   243,   128,   128,   131,    36,    88,   114,   256,
     245,   130,   253,   253,   254,   256,   111,   248,   249,   176,
     128,   176,   239,   176,   176,   176,   239,   225,   204,   239,
     111,    47,   248,   128,   111,   129,   255,   130,   243,   130,
      52,   243,   128,   205,   131,   130,   111,   112,   241,   130,
      69,    46,   207,   123,    65,   131,   241,   111,   112,    24,
     130,   109,   183,   129,   246,   130
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   135,   136,   136,   137,   137,   137,   137,   137,   137,
     137,   137,   137,   137,   137,   137,   137,   137,   138,   139,
     139,   139,   140,   140,   140,   141,   141,   142,   143,   144,
     145,   145,   146,   147,   147,   148,   149,   150,   151,   151,
     151,   151,   151,   152,   153,   154,   154,   154,   154,   154,
     155,   156,   156,   157,   158,   158,   159,   159,   160,   160,
     161,   162,   163,   163,   164,   165,   166,   167,   168,   168,
     169,   170,   171,   170,   172,   173,   173,   174,   175,   176,
     177,   177,   177,   177,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   179,   180,   181,   182,   182,   183,   183,   184,   185,
     186,   186,   187,   187,   188,   188,   189,   189,   190,   190,
     191,   192,   192,   192,   193,   193,   194,   195,   196,   197,
     198,   199,   199,   200,   200,   201,   201,   201,   202,   203,
     204,   205,   205,   206,   207,   207,   208,   209,   209,   209,
     210,   210,   210,   210,   211,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   223,   224,
     224,   224,   224,   225,   226,   226,   226,   226,   227,   227,
     227,   227,   227,   228,   229,   229,   230,   231,   231,   231,
     231,   232,   232,   233,   234,   234,   234,   234,   235,   236,
     237,   237,   238,   238,   239,   240,   240,   240,   241,   242,
     242,   242,   243,   243,   243,   243,   243,   243,   243,   243,
     243,   243,   243,   243,   243,   243,   244,   244,   245,   245,
     246,   246,   247,   247,   247,   247,   248,   248,   249,   249,
     250,   250,   251,   251,   251,   252,   252,   253,   253,   253,
     253,   253,   253,   254,   255,   255,   256,   256,   256,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   257,   257,   257,
     258,   258,   258,   258,   258,   258,   259,   259
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     5,     3,
       3,     2,     3,     3,     2,     5,     3,     4,     6,     4,
       5,     6,     4,     5,     6,     6,     6,     2,     0,     3,
       2,     2,     2,     2,     8,     0,     3,     2,     2,     2,
       6,     4,     7,     2,     0,     3,     0,     4,     0,     3,
       5,     5,     0,     1,     5,     2,     2,     3,     0,     2,
       4,     0,     0,     3,     1,     0,     2,     5,     2,     1,
       0,     3,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     5,     5,     3,    12,     0,     4,     4,     3,
       1,     4,     1,     4,     0,     2,     0,     2,     0,     2,
       2,     8,     6,     6,     6,     6,     4,     2,     2,     2,
       2,     3,     3,     1,     3,     1,     2,     2,     9,     3,
       3,     0,     4,     9,     0,     2,     8,     0,     1,     1,
       1,     1,     1,     1,     2,     4,     2,     2,     2,     2,
       3,     1,     5,     2,     2,     6,     5,     0,     2,     4,
       6,     6,     8,     1,     4,     3,     5,     4,     1,     1,
       1,     1,     2,     2,     4,     6,     1,     4,     3,     5,
       4,     4,     6,     1,     4,     3,     5,     4,     3,     3,
       2,     3,     2,     3,     1,     1,     3,     2,     2,     1,
       3,     4,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     1,     0,     1,     0,     1,
       1,     0,     1,     3,     3,     5,     1,     1,     2,     2,
       0,     1,     2,     1,     0,     1,     1,     1,     1,     3,
       3,     3,     3,     2,     1,     5,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     3,     3,     3,
       3,     3,     2,     2,     6,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     3,
       1,     1,     1,     1,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 17: /* definition: error  */
#line 444 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in definition.");
	}
#line 2207 "ascend/compiler/ascParse.c"
    break;

  case 18: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 451 "ascend/compiler/ascParse.y"
        {
      /* the following steps apply to string buffers only, not files */
      struct gl_list_t *stats;
      int dispose;
      if ((yyvsp[-2].slptr) != NULL) {
        stats = gl_create(1L);
        gl_append_ptr(stats,(void *)(yyvsp[-2].slptr));
        if (g_untrapped_error) {
          ErrMsg_Generic("Because of a syntax error, the following statements are being ignored:");
          WriteStatementList(ASCERR,(yyvsp[-2].slptr),4);
          DestroyStatementList((yyvsp[-2].slptr));
        }else{
          dispose = Asc_ModuleAddStatements(Asc_CurrentModule(),stats);
          switch (dispose) {
            case 1: /* appended */
              if (stats != NULL) {
                  gl_destroy(stats);
              }
              break;
            case 0: /* kept */
              break;
            case -1: /* illegal in file */
              ErrMsg_Generic("GLOBAL statements can only be made interactively. Ignoring:");
              if (stats != NULL) {
                WriteStatementList(ASCERR,(yyvsp[-2].slptr),4);
                gl_iterate(stats,(DestroyFunc)DestroyStatementList);
                gl_destroy(stats);
              }
              break;
            default:
              break;
          }
        }
      }
      /* don't let any bizarreness in string parsing hang around */
      g_type_name = g_refines_name = g_proc_name = NULL;
      g_model_parameters =
      g_parameter_reduction =
      g_parameter_wheres = NULL;
      g_untrapped_error = 0;
    }
#line 2253 "ascend/compiler/ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 496 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 2261 "ascend/compiler/ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name ';'  */
#line 500 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2270 "ascend/compiler/ascParse.c"
    break;

  case 21: /* require_file: REQUIRE_TOK name  */
#line 505 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2279 "ascend/compiler/ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 513 "ascend/compiler/ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 2287 "ascend/compiler/ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name ';'  */
#line 517 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2296 "ascend/compiler/ascParse.c"
    break;

  case 24: /* provide_module: PROVIDE_TOK name  */
#line 522 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2305 "ascend/compiler/ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 530 "ascend/compiler/ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
	    );
      }
	}
#line 2318 "ascend/compiler/ascParse.c"
    break;

  case 26: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 539 "ascend/compiler/ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 2331 "ascend/compiler/ascParse.c"
    break;

  case 27: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 551 "ascend/compiler/ascParse.y"
        {
	  /*  see comments for notes statement.  */
	  if( (yyvsp[-1].int_value) != NOTES_TOK ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, (yyvsp[-1].int_value), NULL);
	  }
	  if ((yyvsp[-2].notesptr) != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = (yyvsp[-2].notesptr);
	    while (nt != NULL) {
	      if (nt->lang != NULL) {
	        lang = nt->lang;
	      }
	      /* save exploding vardata to simple entries until we keep */
	      CollectNote(CreateNote(g_type_name, lang, NULL, g_proc_name,
	                             Asc_ModuleBestName(Asc_CurrentModule()),
	                             nt->bt,
	                             nt->line, nt->vardata, nd_vlist));
	      nt = nt->next;
	    }
	    /* now keep them */
	    ProcessNotes(1);
	    DestroyNoteTmpList((yyvsp[-2].notesptr));
      }
      g_type_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
#line 2363 "ascend/compiler/ascParse.c"
    break;

  case 28: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 582 "ascend/compiler/ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 2372 "ascend/compiler/ascParse.c"
    break;

  case 29: /* add_method_def: add_method_head proclist end ';'  */
#line 590 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-3].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[-2].listp));
	  } else {
	    if( (yyvsp[-1].int_value) != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("ADD METHODS", NULL, (yyvsp[-1].int_value), "METHODS");
	    }
	    if (AddMethods((yyvsp[-3].tptr),(yyvsp[-2].listp),g_untrapped_error) != 0) {
	      if ((yyvsp[-3].tptr) != ILLEGAL_DEFINITION) {
	            error_reporter_current_line(ASC_USER_ERROR
	        ,"ADD METHODS failed for type %s"
	        ,SCP(GetName((yyvsp[-3].tptr)))
	      );
	      DestroyProcedureList((yyvsp[-2].listp));
	      } /* else adding in DEFINITION MODEL may have misgone */
	    }
	  }
	  g_untrapped_error = 0;
	}
#line 2396 "ascend/compiler/ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 613 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-1].id_ptr));
	  if(tmptype == NULL){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"ADD METHODS called with undefined type (%s)"
	      ,SCP((yyvsp[-1].id_ptr))
	    );
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	  g_type_name = (yyvsp[-1].id_ptr); /* scope for notes */
	}
#line 2413 "ascend/compiler/ascParse.c"
    break;

  case 31: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 626 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 2422 "ascend/compiler/ascParse.c"
    break;

  case 32: /* replace_method_def: replace_method_head proclist end ';'  */
#line 634 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-3].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[-2].listp));
	  } else {
	    if( (yyvsp[-1].int_value) != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("REPLACE METHODS", NULL, (yyvsp[-1].int_value), "METHODS");
	    }
	    if (ReplaceMethods((yyvsp[-3].tptr),(yyvsp[-2].listp),g_untrapped_error) != 0) {
	      error_reporter_current_line(ASC_USER_ERROR
	        ,"REPLACE METHODS failed for type %s"
	        ,SCP(GetName((yyvsp[-3].tptr)))
	      );
	      DestroyProcedureList((yyvsp[-2].listp));
	    }
	  }
	  g_untrapped_error = 0;
	}
#line 2444 "ascend/compiler/ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 655 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-1].id_ptr));
	  if (tmptype == NULL) {
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"REPLACE METHODS called with undefined type (%s)"
	      ,SCP((yyvsp[-1].id_ptr))
	    );
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	}
#line 2460 "ascend/compiler/ascParse.c"
    break;

  case 34: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 667 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 2469 "ascend/compiler/ascParse.c"
    break;

  case 35: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 675 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (yyvsp[-1].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("ATOM", SCP(g_type_name),
	                        (yyvsp[-1].int_value), SCP(g_type_name));
	  }
	  g_atom_dim_ptr = CheckDimensionsMatch(g_default_dim_ptr,
	                                        g_atom_dim_ptr);
	  if (g_atom_dim_ptr != NULL) {
	    def_ptr = CreateAtomTypeDef(g_type_name,
	                                g_refines_name,
	                                real_type, /* ignored..really */
	                                Asc_CurrentModule(),
	                                (yyvsp[-5].int_value),
	                                (yyvsp[-3].slptr),
	                                (yyvsp[-2].listp),
	                                g_defaulted,
	                                g_default_double,
	                                g_atom_dim_ptr,
	                                g_default_long,
	                                g_default_symbol,
	                                g_untrapped_error);
	    if (def_ptr != NULL) {
	      keepnotes = AddType(def_ptr);
	    } else {
	      /* CreateAtomTypeDef is responsible for freeing (if needed)
	       * all args sent to it event of failure so we don't have to.
	       * In particular $3 $4 should be killed before returning NULL.
	       */
	      ErrMsg_NullDefPointer(SCP(g_type_name));
	    }
	  } else {
	    error_reporter(ASC_USER_ERROR,Asc_ModuleBestName(Asc_CurrentModule()),g_header_linenum,NULL
	      ,"Atom dimensions don't match in ATOM %s"
	      ,SCP(g_type_name)
	    );
	    DestroyStatementList((yyvsp[-3].slptr));
	    DestroyProcedureList((yyvsp[-2].listp));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
#line 2522 "ascend/compiler/ascParse.c"
    break;

  case 36: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 727 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 2534 "ascend/compiler/ascParse.c"
    break;

  case 37: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 738 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 2543 "ascend/compiler/ascParse.c"
    break;

  case 38: /* default_val: %empty  */
#line 746 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
#line 2553 "ascend/compiler/ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK optional_sign number  */
#line 752 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_defaulted = 1;
	}
#line 2562 "ascend/compiler/ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 757 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 2573 "ascend/compiler/ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 764 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 2584 "ascend/compiler/ascParse.c"
    break;

  case 42: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 771 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 2595 "ascend/compiler/ascParse.c"
    break;

  case 43: /* constant_def: universal constant_head  */
#line 781 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;
	  if (g_defaulted) {
	    g_atom_dim_ptr = CheckDimensionsMatch(g_default_dim_ptr,
	                                          g_atom_dim_ptr);
	  }
	  if (g_atom_dim_ptr != NULL) {
	    def_ptr = CreateConstantTypeDef(g_type_name,
	                                    g_refines_name,
	                                    Asc_CurrentModule(),
	                                    (yyvsp[-1].int_value),
	                                    g_defaulted,
	                                    g_default_double,
	                                    g_default_long,
	                                    g_default_symbol,
	                                    g_atom_dim_ptr,
	                                    g_untrapped_error);
	    if (def_ptr != NULL) {
	      keepnotes = AddType(def_ptr);
	    } else {
	      ErrMsg_NullDefPointer(SCP(g_type_name));
	    }
	  } else {
	    error_reporter(ASC_USER_ERROR,Asc_ModuleBestName(Asc_CurrentModule()),g_header_linenum,NULL,
	            "Constant dimensions don't match in CONSTANT %s"
	            " on line %s:%lu.\n",
	            SCP(g_type_name),
	            Asc_ModuleBestName(Asc_CurrentModule()),
	            g_header_linenum);
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = NULL;
	  g_untrapped_error = 0;
	}
#line 2635 "ascend/compiler/ascParse.c"
    break;

  case 44: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK dims constant_val optional_notes ';'  */
#line 821 "ascend/compiler/ascParse.y"
        {
	  g_type_name = (yyvsp[-6].id_ptr);
	  g_refines_name = (yyvsp[-4].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-3].dimp);
	  switch (g_constant_type) {
	  case DOUBLECONSTANT:
	    g_default_double = (yyvsp[-2].real_value);
	    break;
	  case LONGCONSTANT:
	    g_default_long = (yyvsp[-2].real_value);
	    break;
	  case BOOLEANCONSTANT:
	    g_default_long = (yyvsp[-2].int_value);
	    break;
	  case SYMBOLCONSTANT:
	    g_default_symbol = (yyvsp[-2].sym_ptr);
	    break;
	  default:
	    ErrMsg_Generic("Wierd constant type assign encountered.");
	    break; /* better not be reached. */
	  }
	  g_header_linenum = LineNum();
	  if ((yyvsp[-1].dquote_ptr) != NULL) {
	    CollectNote(CreateNote(g_type_name,InlineNote(),SelfNote(),NULL,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar((yyvsp[-1].dquote_ptr),InlineNote()),
	                           g_header_linenum,NULL,nd_empty));
	  }
	}
#line 2669 "ascend/compiler/ascParse.c"
    break;

  case 45: /* constant_val: %empty  */
#line 854 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
#line 2679 "ascend/compiler/ascParse.c"
    break;

  case 46: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 860 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_defaulted = 1;
	}
#line 2688 "ascend/compiler/ascParse.c"
    break;

  case 47: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 865 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 2699 "ascend/compiler/ascParse.c"
    break;

  case 48: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 872 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 2710 "ascend/compiler/ascParse.c"
    break;

  case 49: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 879 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 2721 "ascend/compiler/ascParse.c"
    break;

  case 50: /* model_def: universal model_head fstatements methods end ';'  */
#line 889 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;
	  if(( (yyvsp[-1].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("MODEL", SCP(g_type_name),
	                        (yyvsp[-1].int_value), SCP(g_type_name));
	  }
	  def_ptr = CreateModelTypeDef(g_type_name,
	                               g_refines_name,
	                               Asc_CurrentModule(),
	                               (yyvsp[-5].int_value),
	                               (yyvsp[-3].slptr),
	                               (yyvsp[-2].listp),
	                               g_model_parameters,
	                               g_parameter_reduction,
	                               g_parameter_wheres,
	                               g_untrapped_error);
	  if (def_ptr != NULL) {
	    keepnotes = AddType(def_ptr);
	  } else {
	    /* CreateModelTypeDef is responsible for freeing (if needed)
	     * all args sent to it so we don't have to here.
	     * in particular $3 $4 g_model_parameters, g_parameter_reduction,
	     * and g_parameter_wheres.
	     */
	    ErrMsg_NullDefPointer(SCP(g_type_name));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = NULL;
	  g_model_parameters =
	    g_parameter_reduction =
	    g_parameter_wheres = NULL;
	  g_untrapped_error = 0;
	}
#line 2763 "ascend/compiler/ascParse.c"
    break;

  case 51: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 931 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 2775 "ascend/compiler/ascParse.c"
    break;

  case 52: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 940 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 2788 "ascend/compiler/ascParse.c"
    break;

  case 53: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 952 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 2797 "ascend/compiler/ascParse.c"
    break;

  case 54: /* optional_model_parameters: %empty  */
#line 960 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 2805 "ascend/compiler/ascParse.c"
    break;

  case 55: /* optional_model_parameters: '(' fstatements ')'  */
#line 964 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 2813 "ascend/compiler/ascParse.c"
    break;

  case 56: /* optional_parameter_wheres: %empty  */
#line 971 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 2821 "ascend/compiler/ascParse.c"
    break;

  case 57: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 975 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 2829 "ascend/compiler/ascParse.c"
    break;

  case 58: /* optional_parameter_reduction: %empty  */
#line 982 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 2837 "ascend/compiler/ascParse.c"
    break;

  case 59: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 986 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 2845 "ascend/compiler/ascParse.c"
    break;

  case 60: /* patch_def: patch_head fstatements methods end ';'  */
#line 993 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  if (((yyvsp[-1].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("PATCH", SCP(g_type_name),
	                        (yyvsp[-1].int_value), SCP(g_type_name));
	  }
	  def_ptr = CreatePatchTypeDef(g_type_name,
	                               g_refines_name,
	                               NULL,
	                               Asc_CurrentModule(),
	                               (yyvsp[-3].slptr),
	                               (yyvsp[-2].listp),
	                               g_untrapped_error);
	  g_untrapped_error = 0;
	  if (def_ptr != NULL) {
	    AddType(def_ptr);
	  } else {
	    /* CreatePatchTypeDef is responsible for freeing (if needed)
	     * all args sent to it so we don't have to here.
	     * in particular $2 $3
	     */
	    ErrMsg_NullDefPointer(SCP(g_type_name));
	  }
	  g_type_name = g_refines_name = g_proc_name = NULL;
	}
#line 2878 "ascend/compiler/ascParse.c"
    break;

  case 61: /* patch_head: PATCH_TOK IDENTIFIER_TOK FOR_TOK IDENTIFIER_TOK ';'  */
#line 1025 "ascend/compiler/ascParse.y"
        {
	  /*
	   * A patch definition looks just like a model def.
	   * with the original name <=> refine name.
	   */
	  g_type_name = (yyvsp[-3].id_ptr);
	  g_refines_name = (yyvsp[-1].id_ptr);
	  g_header_linenum = LineNum();
	}
#line 2892 "ascend/compiler/ascParse.c"
    break;

  case 62: /* universal: %empty  */
#line 1038 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 2900 "ascend/compiler/ascParse.c"
    break;

  case 63: /* universal: UNIVERSAL_TOK  */
#line 1042 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 2908 "ascend/compiler/ascParse.c"
    break;

  case 64: /* definition_def: definition_id fstatements methods end ';'  */
#line 1049 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (yyvsp[-1].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != (yyvsp[-4].id_ptr) )) {
	    WarnMsg_MismatchEnd("DEFINITION", SCP((yyvsp[-4].id_ptr)), (yyvsp[-1].int_value), SCP((yyvsp[-4].id_ptr)));
	  }
	  if( (yyvsp[-4].id_ptr) == GetBaseTypeName(relation_type)) {
	    def_ptr = CreateRelationTypeDef(Asc_CurrentModule(),(yyvsp[-4].id_ptr),(yyvsp[-3].slptr),(yyvsp[-2].listp));
	  }
	  else if( (yyvsp[-4].id_ptr) == GetBaseTypeName(logrel_type) ) {
	    def_ptr = CreateLogRelTypeDef(Asc_CurrentModule(),(yyvsp[-4].id_ptr),(yyvsp[-3].slptr),(yyvsp[-2].listp));
	  }
	  else {
	    ErrMsg_Generic("Bad type passed to DEFINITION statement.");
	    def_ptr = NULL;
	  }
	  if ( def_ptr != NULL ) {
	    keepnotes = AddType(def_ptr);
	  } else {
	    ErrMsg_NullDefPointer(SCP((yyvsp[-4].id_ptr)));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = NULL;
	  g_untrapped_error = 0;
	}
#line 2939 "ascend/compiler/ascParse.c"
    break;

  case 65: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1079 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 2948 "ascend/compiler/ascParse.c"
    break;

  case 66: /* units_def: units_statement ';'  */
#line 1088 "ascend/compiler/ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 2954 "ascend/compiler/ascParse.c"
    break;

  case 67: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1093 "ascend/compiler/ascParse.y"
        {
	  struct UnitDefinition *ud;
	  unsigned long c,len;

	  if( (yyvsp[0].int_value) != UNITS_TOK ) {
	    WarnMsg_MismatchEnd("UNITS", NULL, (yyvsp[0].int_value), NULL);
	  }
	  len = gl_length((yyvsp[-1].listp));
	  for (c=1; c <= len; c++) {
	    ud = (struct UnitDefinition *)gl_fetch((yyvsp[-1].listp),c);
	    ProcessUnitDef(ud);
	    DestroyUnitDef(ud);
	  }
	  gl_destroy((yyvsp[-1].listp));
	  (yyval.statptr) = NULL;
	}
#line 2975 "ascend/compiler/ascParse.c"
    break;

  case 68: /* unitdeflist: %empty  */
#line 1112 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(100L);
	}
#line 2983 "ascend/compiler/ascParse.c"
    break;

  case 69: /* unitdeflist: unitdeflist unitdef  */
#line 1116 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 2992 "ascend/compiler/ascParse.c"
    break;

  case 70: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1124 "ascend/compiler/ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 3001 "ascend/compiler/ascParse.c"
    break;

  case 71: /* methods: %empty  */
#line 1133 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 3009 "ascend/compiler/ascParse.c"
    break;

  case 72: /* $@1: %empty  */
#line 1137 "ascend/compiler/ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 3020 "ascend/compiler/ascParse.c"
    break;

  case 73: /* methods: METHODS_TOK $@1 proclist  */
#line 1144 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 3028 "ascend/compiler/ascParse.c"
    break;

  case 74: /* proclist: proclistf  */
#line 1151 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 3037 "ascend/compiler/ascParse.c"
    break;

  case 75: /* proclistf: %empty  */
#line 1158 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 3045 "ascend/compiler/ascParse.c"
    break;

  case 76: /* proclistf: proclistf procedure  */
#line 1162 "ascend/compiler/ascParse.y"
        {
	  unsigned long c;
	  struct InitProcedure *oldproc;
	  c = gl_length((yyvsp[-1].listp));
	  while (c > 0) {
	    oldproc = (struct InitProcedure *)gl_fetch((yyvsp[-1].listp),c);
	    if (ProcName((yyvsp[0].procptr)) == ProcName(oldproc)) {
	      error_reporter_current_line(ASC_USER_WARNING
	        ,"Duplicate METHOD %s rejected", SCP(ProcName((yyvsp[0].procptr)))
	      );
	      break;
	    }
	    c--;
	  }
	  if (c) { /* broke early */
	    DestroyProcedure((yyvsp[0].procptr));
	  } else {
	    gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].procptr));
	  }
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 3071 "ascend/compiler/ascParse.c"
    break;

  case 77: /* procedure: procedure_id ';' fstatements end ';'  */
#line 1187 "ascend/compiler/ascParse.y"
        {
	  if (((yyvsp[-1].int_value) != IDENTIFIER_TOK) || ((yyvsp[-4].id_ptr) != g_end_identifier)) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("METHOD", SCP((yyvsp[-4].id_ptr)), (yyvsp[-1].int_value), SCP((yyvsp[-4].id_ptr)));
	  }
	  (yyval.procptr) = CreateProcedure((yyvsp[-4].id_ptr),(yyvsp[-2].slptr));
	  g_proc_name = NULL;
	}
#line 3086 "ascend/compiler/ascParse.c"
    break;

  case 78: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 1201 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 3095 "ascend/compiler/ascParse.c"
    break;

  case 79: /* fstatements: statements  */
#line 1210 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 3103 "ascend/compiler/ascParse.c"
    break;

  case 80: /* statements: %empty  */
#line 1217 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 3111 "ascend/compiler/ascParse.c"
    break;

  case 81: /* statements: statements statement ';'  */
#line 1221 "ascend/compiler/ascParse.y"
        {
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ((yyvsp[-1].statptr) != NULL) {
	    gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].statptr));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 3123 "ascend/compiler/ascParse.c"
    break;

  case 82: /* statements: statements complex_statement ';'  */
#line 1229 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 3134 "ascend/compiler/ascParse.c"
    break;

  case 83: /* statements: statements error ';'  */
#line 1236 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 3143 "ascend/compiler/ascParse.c"
    break;

  case 122: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 1289 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-2].id_ptr));
	  if ((yyvsp[0].eptr) != NULL) {
	    ErrMsg_Generic("WITH VALUE clause not allowed in IS_A.");
	    g_untrapped_error++;
	    DestroyVariableList((yyvsp[-4].lptr));
	    DestroySetList(g_typeargs);
	    DestroyExprList((yyvsp[0].eptr));
	    (yyval.statptr) = NULL;
	  } else {
	    if (tmptype != NULL) {
	      if ((GetBaseType(tmptype) != model_type) &&
	          (g_typeargs != NULL)) {
	        error_reporter_current_line(ASC_USER_ERROR,
	                "IS_A has arguments to the nonmodel type %s.\n",
	                SCP((yyvsp[-2].id_ptr)));
	        DestroyVariableList((yyvsp[-4].lptr));
	        DestroySetList(g_typeargs);
	        DestroyExprList((yyvsp[0].eptr));
	        g_untrapped_error++;
	        (yyval.statptr) = NULL;
	      } else {
	        (yyval.statptr) = CreateISA((yyvsp[-4].lptr),(yyvsp[-2].id_ptr),g_typeargs,(yyvsp[-1].id_ptr));
	      }
	    } else {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_A uses the undefined type %s.", SCP((yyvsp[-2].id_ptr)));
	      DestroyVariableList((yyvsp[-4].lptr));
	      DestroySetList(g_typeargs);
	      DestroyExprList((yyvsp[0].eptr));
	      g_untrapped_error++;
	      (yyval.statptr) = NULL;
	    }
	  }
	  g_typeargs = NULL;

	}
#line 3185 "ascend/compiler/ascParse.c"
    break;

  case 123: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 1330 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-2].id_ptr));
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) &&
	        (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"WILL_BE has arguments to the nonmodel type '%s'",SCP((yyvsp[-2].id_ptr)));
	      DestroyVariableList((yyvsp[-4].lptr));
	      DestroySetList(g_typeargs);
	      DestroyExprList((yyvsp[0].eptr));
	      g_untrapped_error++;
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateWILLBE((yyvsp[-4].lptr),(yyvsp[-2].id_ptr),g_typeargs,(yyvsp[-1].id_ptr),(yyvsp[0].eptr));
	    }
	  } else {
	    DestroyVariableList((yyvsp[-4].lptr));
	    DestroySetList(g_typeargs);
	    DestroyExprList((yyvsp[0].eptr));
	    g_untrapped_error++;
	    (yyval.statptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"WILL_BE uses the undefined type %s.",SCP((yyvsp[-2].id_ptr)));
	  }
	  g_typeargs = NULL;
	}
#line 3215 "ascend/compiler/ascParse.c"
    break;

  case 124: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 1359 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 3223 "ascend/compiler/ascParse.c"
    break;

  case 125: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 1364 "ascend/compiler/ascParse.y"
        {
	  int carray_err;
	  carray_err = 0;
	  if (VariableListLength((yyvsp[-11].lptr)) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES allows only 1 LHS name. Found:");
	    WriteVariableList(ASCERR,(yyvsp[-11].lptr));
	  }
	  if (VariableListLength((yyvsp[-5].lptr)) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES/IS_A allows only one LHS name. Found:");
	    WriteVariableList(ASCERR,(yyvsp[-5].lptr));
	  }
	  /* verify $9 == "set" */
	  if (!carray_err && (yyvsp[-3].id_ptr) != GetBaseTypeName(set_type)) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,"Compound ALIASES statement requires IS_A %s. ",SCP(GetBaseTypeName(set_type)));
	    FPRINTF(ASCERR,"    Found %s.\n",SCP((yyvsp[-3].id_ptr)));
	  }
	  /* verify set type */
	  if ((!carray_err) &&
	      ((yyvsp[-1].id_ptr) != GetBaseTypeName(symbol_constant_type)) &&
	      ((yyvsp[-1].id_ptr) != GetBaseTypeName(integer_constant_type))) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES IS_A statement requires %s or %s.\n",
	            SCP(GetBaseTypeName(integer_constant_type)),
	            SCP(GetBaseTypeName(symbol_constant_type)));
	    FPRINTF(ASCERR,"	Found %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	  if (carray_err) {
	    DestroyVariableList((yyvsp[-11].lptr));
	    DestroyVariableList((yyvsp[-8].lptr));
	    DestroyVariableList((yyvsp[-5].lptr));
	    DestroySetList((yyvsp[0].sptr));
	    g_untrapped_error++;
	    (yyval.statptr) = NULL;
	  } else {
	    int intset;
	    intset = ((yyvsp[-1].id_ptr) == GetBaseTypeName(integer_constant_type));
	    (yyval.statptr) = CreateARR((yyvsp[-11].lptr),(yyvsp[-8].lptr),(yyvsp[-5].lptr),intset,(yyvsp[0].sptr));
	  }
	}
#line 3273 "ascend/compiler/ascParse.c"
    break;

  case 126: /* optional_set_values: %empty  */
#line 1413 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 3281 "ascend/compiler/ascParse.c"
    break;

  case 127: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 1417 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 3289 "ascend/compiler/ascParse.c"
    break;

  case 128: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 1424 "ascend/compiler/ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 3302 "ascend/compiler/ascParse.c"
    break;

  case 129: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 1436 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[0].id_ptr));
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) && 
	        (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_REFINED_TO has arguments to the nonmodel type %s.",SCP((yyvsp[0].id_ptr)));
	      DestroyVariableList((yyvsp[-2].lptr));
	      DestroySetList(g_typeargs);
	      g_untrapped_error++;
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateIRT((yyvsp[-2].lptr),(yyvsp[0].id_ptr),g_typeargs);
	    }
	  } else {
	    error_reporter_current_line(ASC_USER_ERROR,"The IS_REFINED_TO uses the undefined type %s.\n",SCP((yyvsp[0].id_ptr)));
	    DestroyVariableList((yyvsp[-2].lptr));
	    DestroySetList(g_typeargs);
	    g_untrapped_error++;
	    (yyval.statptr) = NULL;
	  }
	  g_typeargs = NULL;
	}
#line 3330 "ascend/compiler/ascParse.c"
    break;

  case 130: /* call_identifier: IDENTIFIER_TOK  */
#line 1463 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 3339 "ascend/compiler/ascParse.c"
    break;

  case 131: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 1468 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 3348 "ascend/compiler/ascParse.c"
    break;

  case 132: /* type_identifier: IDENTIFIER_TOK  */
#line 1476 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 3357 "ascend/compiler/ascParse.c"
    break;

  case 133: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 1481 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 3366 "ascend/compiler/ascParse.c"
    break;

  case 134: /* optional_method: %empty  */
#line 1489 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 3374 "ascend/compiler/ascParse.c"
    break;

  case 135: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 1493 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3382 "ascend/compiler/ascParse.c"
    break;

  case 136: /* optional_of: %empty  */
#line 1500 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 3390 "ascend/compiler/ascParse.c"
    break;

  case 137: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 1504 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3398 "ascend/compiler/ascParse.c"
    break;

  case 138: /* optional_with_value: %empty  */
#line 1511 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 3406 "ascend/compiler/ascParse.c"
    break;

  case 139: /* optional_with_value: WITH_VALUE_T expr  */
#line 1515 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 3414 "ascend/compiler/ascParse.c"
    break;

  case 140: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 1522 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 3422 "ascend/compiler/ascParse.c"
    break;

  case 141: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 1529 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 3430 "ascend/compiler/ascParse.c"
    break;

  case 142: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 1533 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 3438 "ascend/compiler/ascParse.c"
    break;

  case 143: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 1537 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 3446 "ascend/compiler/ascParse.c"
    break;

  case 144: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 1544 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 3454 "ascend/compiler/ascParse.c"
    break;

  case 145: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 1548 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 3462 "ascend/compiler/ascParse.c"
    break;

  case 146: /* der_statement: DER_TOK '(' fvarlist ')'  */
#line 1555 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 3472 "ascend/compiler/ascParse.c"
    break;

  case 147: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 1564 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 3482 "ascend/compiler/ascParse.c"
    break;

  case 148: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 1573 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 3490 "ascend/compiler/ascParse.c"
    break;

  case 149: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 1580 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 3498 "ascend/compiler/ascParse.c"
    break;

  case 150: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 1587 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 3506 "ascend/compiler/ascParse.c"
    break;

  case 151: /* assignment_statement: fname ASSIGN_TOK expr  */
#line 1594 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 3514 "ascend/compiler/ascParse.c"
    break;

  case 152: /* assignment_statement: fname CASSIGN_TOK expr  */
#line 1598 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 3522 "ascend/compiler/ascParse.c"
    break;

  case 153: /* relation_statement: relation  */
#line 1605 "ascend/compiler/ascParse.y"
        {
	  if (IsRelation((yyvsp[0].eptr))) {
	    if (g_parse_relns == 0) {
	      DestroyExprList((yyvsp[0].eptr));
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateREL(NULL,(yyvsp[0].eptr));
	    }
	  } else {
	    (yyval.statptr) = CreateLOGREL(NULL,(yyvsp[0].eptr));
	  }
	}
#line 3539 "ascend/compiler/ascParse.c"
    break;

  case 154: /* relation_statement: fname ':' relation  */
#line 1618 "ascend/compiler/ascParse.y"
        {
	  if (IsRelation((yyvsp[0].eptr))) {
	    if (g_parse_relns == 0) {
	      DestroyExprList((yyvsp[0].eptr));
	      DestroyName((yyvsp[-2].nptr));
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateREL((yyvsp[-2].nptr),(yyvsp[0].eptr));
	    }
	  } else {
	    (yyval.statptr) = CreateLOGREL((yyvsp[-2].nptr),(yyvsp[0].eptr));
	  }
	}
#line 3557 "ascend/compiler/ascParse.c"
    break;

  case 155: /* relation: expr  */
#line 1635 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 3571 "ascend/compiler/ascParse.c"
    break;

  case 156: /* relation: MINIMIZE_TOK expr  */
#line 1645 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 3583 "ascend/compiler/ascParse.c"
    break;

  case 157: /* relation: MAXIMIZE_TOK expr  */
#line 1653 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 3595 "ascend/compiler/ascParse.c"
    break;

  case 158: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 1664 "ascend/compiler/ascParse.y"
    {
      /*
       * This is the blackbox declarative external relation.
       */
      	struct VariableList *vl;
	/*determine the number of variables declared in input_args and output_args*/
      	unsigned long n_inputs, n_outputs;
	n_inputs = VariableListLength((yyvsp[-4].lptr));
	n_outputs = VariableListLength((yyvsp[-2].lptr));
	/*continue with normal parsing process */
	vl = JoinVariableLists((yyvsp[-4].lptr),(yyvsp[-2].lptr)); 
	/* $$ = CreateEXTERN(2,$1,SCP($3),vl,$8,NULL); */

      /*$$ = CreateEXTERNBlackBox($1,SCP($3),vl,$8); //original */
      //statement now also knows how many of the variables in vl are inputs/outputs
      (yyval.statptr) = CreateEXTERNBlackBox((yyvsp[-8].nptr),SCP((yyvsp[-6].id_ptr)),vl,(yyvsp[-1].nptr),n_inputs,n_outputs); 
    }
#line 3617 "ascend/compiler/ascParse.c"
    break;

  case 159: /* input_args: fvarlist ':' INPUT_TOK  */
#line 1685 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 3625 "ascend/compiler/ascParse.c"
    break;

  case 160: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 1692 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 3633 "ascend/compiler/ascParse.c"
    break;

  case 161: /* data_args: %empty  */
#line 1699 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 3641 "ascend/compiler/ascParse.c"
    break;

  case 162: /* data_args: ';' fname ':' DATA_TOK  */
#line 1703 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 3649 "ascend/compiler/ascParse.c"
    break;

  case 163: /* glassbox_statement: fname ':' IDENTIFIER_TOK '(' fvarlist ';' INTEGER_TOK ')' optional_scope  */
#line 1710 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is the glassbox declarative external relation.
	   * This now allows a scope for placement of the relations
	   */
	  struct VariableList *vl = (yyvsp[-4].lptr);
	  struct Name *nptr;
	  char tmp[32]; 
	  symchar *str;

	  sprintf(tmp,"%ld",(yyvsp[-2].int_value));
	  str = AddSymbol(tmp);
	  nptr = CreateIdName(str);
	/* $$ = CreateEXTERN(1,$1,SCP($3),vl,nptr,$9); */
	  (yyval.statptr) = CreateEXTERNGlassBox((yyvsp[-8].nptr),SCP((yyvsp[-6].id_ptr)),vl,nptr,(yyvsp[0].nptr));
	}
#line 3670 "ascend/compiler/ascParse.c"
    break;

  case 164: /* optional_scope: %empty  */
#line 1730 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 3678 "ascend/compiler/ascParse.c"
    break;

  case 165: /* optional_scope: IN_TOK fname  */
#line 1734 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[0].nptr);
	}
#line 3686 "ascend/compiler/ascParse.c"
    break;

  case 166: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 1742 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != FOR_TOK ) {
	    WarnMsg_MismatchEnd("FOR", SCP((yyvsp[-6].id_ptr)), (yyvsp[0].int_value), NULL);
	  }
	  if ((yyvsp[-2].fkind) == fk_create && (yyvsp[-3].order) != f_random) {
	    /* create cannot have an order in declarative FOR */
	    ErrMsg_Generic("FOR loops only accept DECREASING or INCREASING in the method section.");
	    g_untrapped_error++;
	  }
	  if ((yyvsp[-2].fkind) == fk_do && (yyvsp[-3].order) == f_random) {
	    /* all FOR/DO default to increasing */
	    (yyval.statptr) = CreateFOR((yyvsp[-6].id_ptr),(yyvsp[-4].eptr),(yyvsp[-1].slptr),f_increasing,(yyvsp[-2].fkind));
	  } else {
	    (yyval.statptr) = CreateFOR((yyvsp[-6].id_ptr),(yyvsp[-4].eptr),(yyvsp[-1].slptr),(yyvsp[-3].order),(yyvsp[-2].fkind));
	  }
	}
#line 3707 "ascend/compiler/ascParse.c"
    break;

  case 167: /* optional_direction: %empty  */
#line 1762 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 3715 "ascend/compiler/ascParse.c"
    break;

  case 168: /* optional_direction: INCREASING_TOK  */
#line 1766 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 3723 "ascend/compiler/ascParse.c"
    break;

  case 169: /* optional_direction: DECREASING_TOK  */
#line 1770 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 3731 "ascend/compiler/ascParse.c"
    break;

  case 170: /* forexprend: CREATE_TOK  */
#line 1777 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 3739 "ascend/compiler/ascParse.c"
    break;

  case 171: /* forexprend: EXPECT_TOK  */
#line 1781 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 3747 "ascend/compiler/ascParse.c"
    break;

  case 172: /* forexprend: CHECK_TOK  */
#line 1785 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 3755 "ascend/compiler/ascParse.c"
    break;

  case 173: /* forexprend: DO_TOK  */
#line 1789 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 3763 "ascend/compiler/ascParse.c"
    break;

  case 174: /* run_statement: RUN_TOK fname  */
#line 1796 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 3771 "ascend/compiler/ascParse.c"
    break;

  case 175: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 1800 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 3779 "ascend/compiler/ascParse.c"
    break;

  case 176: /* fix_statement: FIX_TOK fvarlist  */
#line 1807 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 3788 "ascend/compiler/ascParse.c"
    break;

  case 177: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 1815 "ascend/compiler/ascParse.y"
        {
		struct Statement *assign = (yyvsp[0].statptr);
		struct Name *n = CopyName((yyvsp[0].statptr) -> v.asgn.nptr);
		struct VariableList *vars = CreateVariableNode(n);
		struct Statement *fix = CreateFIX(ReverseVariableList(vars));
		struct gl_list_t *fix_and_assign = gl_create(7L);
		gl_append_ptr(fix_and_assign,(char*)fix);
		gl_append_ptr(fix_and_assign,(char*)assign);
		(yyval.listp) = fix_and_assign;
	}
#line 3803 "ascend/compiler/ascParse.c"
    break;

  case 178: /* free_statement: FREE_TOK fvarlist  */
#line 1829 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 3811 "ascend/compiler/ascParse.c"
    break;

  case 179: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 1836 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 3820 "ascend/compiler/ascParse.c"
    break;

  case 180: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 1844 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 3829 "ascend/compiler/ascParse.c"
    break;

  case 181: /* solve_statement: SOLVE_TOK  */
#line 1852 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE();
	}
#line 3838 "ascend/compiler/ascParse.c"
    break;

  case 182: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 1860 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 3850 "ascend/compiler/ascParse.c"
    break;

  case 183: /* call_statement: CALL_TOK call_identifier  */
#line 1871 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 3862 "ascend/compiler/ascParse.c"
    break;

  case 184: /* assert_statement: ASSERT_TOK expr  */
#line 1882 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 3870 "ascend/compiler/ascParse.c"
    break;

  case 185: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 1888 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 3881 "ascend/compiler/ascParse.c"
    break;

  case 186: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 1898 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 3892 "ascend/compiler/ascParse.c"
    break;

  case 187: /* optional_else: %empty  */
#line 1907 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3900 "ascend/compiler/ascParse.c"
    break;

  case 188: /* optional_else: ELSE_TOK fstatements  */
#line 1911 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 3908 "ascend/compiler/ascParse.c"
    break;

  case 189: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 1918 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[-1].wptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 3923 "ascend/compiler/ascParse.c"
    break;

  case 190: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 1929 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[-1].wptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  DestroyName((yyvsp[-5].nptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 3939 "ascend/compiler/ascParse.c"
    break;

  case 191: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 1941 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 3950 "ascend/compiler/ascParse.c"
    break;

  case 192: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 1948 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 3961 "ascend/compiler/ascParse.c"
    break;

  case 193: /* whenlist: whenlistf  */
#line 1958 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 3969 "ascend/compiler/ascParse.c"
    break;

  case 194: /* whenlistf: CASE_TOK set ':' fstatements  */
#line 1965 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 3977 "ascend/compiler/ascParse.c"
    break;

  case 195: /* whenlistf: OTHERWISE_TOK ':' fstatements  */
#line 1969 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	}
#line 3985 "ascend/compiler/ascParse.c"
    break;

  case 196: /* whenlistf: whenlistf CASE_TOK set ':' fstatements  */
#line 1973 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	}
#line 3993 "ascend/compiler/ascParse.c"
    break;

  case 197: /* whenlistf: whenlistf OTHERWISE_TOK ':' fstatements  */
#line 1977 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	}
#line 4001 "ascend/compiler/ascParse.c"
    break;

  case 198: /* flow_statement: BREAK_TOK  */
#line 1984 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 4009 "ascend/compiler/ascParse.c"
    break;

  case 199: /* flow_statement: CONTINUE_TOK  */
#line 1988 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 4017 "ascend/compiler/ascParse.c"
    break;

  case 200: /* flow_statement: FALLTHRU_TOK  */
#line 1992 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 4025 "ascend/compiler/ascParse.c"
    break;

  case 201: /* flow_statement: RETURN_TOK  */
#line 1996 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 4033 "ascend/compiler/ascParse.c"
    break;

  case 202: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 2000 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 4041 "ascend/compiler/ascParse.c"
    break;

  case 203: /* use_statement: USE_TOK fname  */
#line 2007 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 4049 "ascend/compiler/ascParse.c"
    break;

  case 204: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 2014 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SELECT statement.");
	  DestroySelectList((yyvsp[-1].septr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 4064 "ascend/compiler/ascParse.c"
    break;

  case 205: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 2025 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 4075 "ascend/compiler/ascParse.c"
    break;

  case 206: /* selectlist: selectlistf  */
#line 2035 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 4083 "ascend/compiler/ascParse.c"
    break;

  case 207: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 2042 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 4091 "ascend/compiler/ascParse.c"
    break;

  case 208: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 2046 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 4099 "ascend/compiler/ascParse.c"
    break;

  case 209: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 2050 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 4107 "ascend/compiler/ascParse.c"
    break;

  case 210: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 2054 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 4115 "ascend/compiler/ascParse.c"
    break;

  case 211: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 2061 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SWITCH statement.");
	  DestroySwitchList((yyvsp[-1].swptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 4130 "ascend/compiler/ascParse.c"
    break;

  case 212: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 2072 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 4141 "ascend/compiler/ascParse.c"
    break;

  case 213: /* switchlist: switchlistf  */
#line 2082 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 4149 "ascend/compiler/ascParse.c"
    break;

  case 214: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 2089 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 4157 "ascend/compiler/ascParse.c"
    break;

  case 215: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 2093 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 4165 "ascend/compiler/ascParse.c"
    break;

  case 216: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 2097 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 4173 "ascend/compiler/ascParse.c"
    break;

  case 217: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 2101 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 4181 "ascend/compiler/ascParse.c"
    break;

  case 218: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 2108 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 4192 "ascend/compiler/ascParse.c"
    break;

  case 219: /* notes_statement: NOTES_TOK notes_body end  */
#line 2118 "ascend/compiler/ascParse.y"
        {
	  /*  All processing of notes takes place on the notes_body here.
	   *  Notes should NOT be added to the statement list.
	   *  Here we know the current type and method names.
	   */
	  if( (yyvsp[0].int_value) != NOTES_TOK ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, (yyvsp[0].int_value), NULL);
	  }
	  if ((yyvsp[-1].notesptr) != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = (yyvsp[-1].notesptr);
	    while (nt != NULL) {
	      if (nt->lang != NULL) {
		/* this logic works because of the reverse sort that
	         * yacc does via noteslist and the forward sort that
	         * we do via notesbody. lang recorded last appears
	         * before other entries that need it.
	         */
	        lang = nt->lang;
	      }

	      /* save exploding vardata to simple entries until we keep */
	      CollectNote(CreateNote(g_type_name, lang, NULL, g_proc_name,
	                             Asc_ModuleBestName(Asc_CurrentModule()),
	                             nt->bt,
	                             nt->line, nt->vardata, nd_vlist));
	      nt = nt->next;
	    }
	    DestroyNoteTmpList((yyvsp[-1].notesptr));
	  }
	  (yyval.statptr) = NULL;
	}
#line 4230 "ascend/compiler/ascParse.c"
    break;

  case 220: /* notes_body: SYMBOL_TOK noteslist  */
#line 2155 "ascend/compiler/ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 4243 "ascend/compiler/ascParse.c"
    break;

  case 221: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 2164 "ascend/compiler/ascParse.y"
        {
	  struct NoteTmp *nt;
	  (yyval.notesptr) = (yyvsp[-2].notesptr);
	  assert((yyvsp[0].notesptr)->lang == NULL);
	  (yyvsp[0].notesptr)->lang = (yyvsp[-1].sym_ptr);
	  nt = (yyval.notesptr);
	  while (nt->next != NULL) {
	    nt = nt->next;
	  }
	  LinkNoteTmp(nt,(yyvsp[0].notesptr));
	}
#line 4259 "ascend/compiler/ascParse.c"
    break;

  case 222: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 2179 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 4268 "ascend/compiler/ascParse.c"
    break;

  case 223: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 2184 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 4278 "ascend/compiler/ascParse.c"
    break;

  case 224: /* fvarlist: varlist  */
#line 2193 "ascend/compiler/ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 4290 "ascend/compiler/ascParse.c"
    break;

  case 225: /* varlist: fname  */
#line 2204 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 4298 "ascend/compiler/ascParse.c"
    break;

  case 226: /* varlist: varlist ',' fname  */
#line 2208 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 4307 "ascend/compiler/ascParse.c"
    break;

  case 227: /* varlist: varlist fname  */
#line 2213 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 4321 "ascend/compiler/ascParse.c"
    break;

  case 228: /* fname: name optional_notes  */
#line 2226 "ascend/compiler/ascParse.y"
        {
	  symchar *simple;
	  void *data;
	  enum NoteData nd;
	  (yyval.nptr) = ReverseName((yyvsp[-1].nptr));
	  if ((yyvsp[0].dquote_ptr) != NULL && (yyvsp[-1].nptr) != NULL) {
	    simple = SimpleNameIdPtr((yyval.nptr));
	    data = (simple == NULL ? (void *)(yyval.nptr) : NULL);
	    nd = (data == NULL ? nd_empty : nd_name);
	    CollectNote(CreateNote(g_type_name, InlineNote(), simple,
	                           g_proc_name,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar((yyvsp[0].dquote_ptr),InlineNote()),
	                           LineNum(), data, nd));
	  }
	}
#line 4342 "ascend/compiler/ascParse.c"
    break;

  case 229: /* name: IDENTIFIER_TOK  */
#line 2246 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 4350 "ascend/compiler/ascParse.c"
    break;

  case 230: /* name: name '.' IDENTIFIER_TOK  */
#line 2250 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 4359 "ascend/compiler/ascParse.c"
    break;

  case 231: /* name: name '[' set ']'  */
#line 2255 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-1].sptr) == NULL) {
	    error_reporter_current_line(ASC_USER_ERROR,"syntax error: Empty set in name definition, name:");
	    WriteName(ASCERR,(yyvsp[-3].nptr));
	    FPRINTF(ASCERR,"[]\n");
	    g_untrapped_error++;
	  } else {
	    (yyval.nptr) = CreateSetName((yyvsp[-1].sptr));
	    LinkNames((yyval.nptr),(yyvsp[-3].nptr));
	  }
	}
#line 4375 "ascend/compiler/ascParse.c"
    break;

  case 232: /* end: END_TOK CONDITIONAL_TOK  */
#line 2270 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 4384 "ascend/compiler/ascParse.c"
    break;

  case 233: /* end: END_TOK FOR_TOK  */
#line 2275 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 4393 "ascend/compiler/ascParse.c"
    break;

  case 234: /* end: END_TOK IF_TOK  */
#line 2280 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 4402 "ascend/compiler/ascParse.c"
    break;

  case 235: /* end: END_TOK INTERACTIVE_TOK  */
#line 2285 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 4411 "ascend/compiler/ascParse.c"
    break;

  case 236: /* end: END_TOK METHODS_TOK  */
#line 2290 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 4420 "ascend/compiler/ascParse.c"
    break;

  case 237: /* end: END_TOK NOTES_TOK  */
#line 2295 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 4429 "ascend/compiler/ascParse.c"
    break;

  case 238: /* end: END_TOK SELECT_TOK  */
#line 2300 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 4438 "ascend/compiler/ascParse.c"
    break;

  case 239: /* end: END_TOK SWITCH_TOK  */
#line 2305 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 4447 "ascend/compiler/ascParse.c"
    break;

  case 240: /* end: END_TOK UNITS_TOK  */
#line 2310 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 4456 "ascend/compiler/ascParse.c"
    break;

  case 241: /* end: END_TOK GLOBAL_TOK  */
#line 2315 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 4465 "ascend/compiler/ascParse.c"
    break;

  case 242: /* end: END_TOK WHEN_TOK  */
#line 2320 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 4474 "ascend/compiler/ascParse.c"
    break;

  case 243: /* end: END_TOK WHILE_TOK  */
#line 2325 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 4483 "ascend/compiler/ascParse.c"
    break;

  case 244: /* end: END_TOK IDENTIFIER_TOK  */
#line 2330 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 4492 "ascend/compiler/ascParse.c"
    break;

  case 245: /* end: END_TOK  */
#line 2335 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 4501 "ascend/compiler/ascParse.c"
    break;

  case 246: /* optional_bracedtext: %empty  */
#line 2343 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 4509 "ascend/compiler/ascParse.c"
    break;

  case 247: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 2347 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 4517 "ascend/compiler/ascParse.c"
    break;

  case 248: /* optional_notes: %empty  */
#line 2354 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 4525 "ascend/compiler/ascParse.c"
    break;

  case 249: /* optional_notes: DQUOTE_TOK  */
#line 2358 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 4533 "ascend/compiler/ascParse.c"
    break;

  case 250: /* set: setexprlist  */
#line 2365 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 4541 "ascend/compiler/ascParse.c"
    break;

  case 251: /* set: %empty  */
#line 2369 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4549 "ascend/compiler/ascParse.c"
    break;

  case 252: /* setexprlist: expr  */
#line 2376 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 4557 "ascend/compiler/ascParse.c"
    break;

  case 253: /* setexprlist: expr DOTDOT_TOK expr  */
#line 2380 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4565 "ascend/compiler/ascParse.c"
    break;

  case 254: /* setexprlist: setexprlist ',' expr  */
#line 2384 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 4574 "ascend/compiler/ascParse.c"
    break;

  case 255: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 2389 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 4583 "ascend/compiler/ascParse.c"
    break;

  case 256: /* number: INTEGER_TOK  */
#line 2397 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
#line 4593 "ascend/compiler/ascParse.c"
    break;

  case 257: /* number: realnumber  */
#line 2403 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 4603 "ascend/compiler/ascParse.c"
    break;

  case 258: /* realnumber: REAL_TOK opunits  */
#line 2412 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	}
#line 4611 "ascend/compiler/ascParse.c"
    break;

  case 259: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 2416 "ascend/compiler/ascParse.y"
        {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((yyvsp[0].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (yyval.real_value) = (double)(yyvsp[-1].int_value)*UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
	    char **errv;
	    (yyval.real_value) = (double)(yyvsp[-1].int_value);
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'", (yyvsp[0].braced_ptr));
	    errv = UnitsExplainError((yyvsp[0].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
#line 4633 "ascend/compiler/ascParse.c"
    break;

  case 260: /* opunits: %empty  */
#line 2437 "ascend/compiler/ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  (yyval.real_value) = 1.0;
	}
#line 4642 "ascend/compiler/ascParse.c"
    break;

  case 261: /* opunits: BRACEDTEXT_TOK  */
#line 2442 "ascend/compiler/ascParse.y"
        {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((yyvsp[0].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (yyval.real_value) = UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
	    char **errv;
	    (yyval.real_value) = 1.0;
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'",(yyvsp[0].braced_ptr));
	    errv = UnitsExplainError((yyvsp[0].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
#line 4664 "ascend/compiler/ascParse.c"
    break;

  case 262: /* dims: DIMENSION_TOK dimensions  */
#line 2463 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 4672 "ascend/compiler/ascParse.c"
    break;

  case 263: /* dims: DIMENSIONLESS_TOK  */
#line 2467 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 4680 "ascend/compiler/ascParse.c"
    break;

  case 264: /* dims: %empty  */
#line 2471 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 4688 "ascend/compiler/ascParse.c"
    break;

  case 265: /* dimensions: '*'  */
#line 2478 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 4696 "ascend/compiler/ascParse.c"
    break;

  case 266: /* dimensions: dimexpr  */
#line 2482 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 4704 "ascend/compiler/ascParse.c"
    break;

  case 267: /* dimexpr: IDENTIFIER_TOK  */
#line 2489 "ascend/compiler/ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 4712 "ascend/compiler/ascParse.c"
    break;

  case 268: /* dimexpr: INTEGER_TOK  */
#line 2493 "ascend/compiler/ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 4720 "ascend/compiler/ascParse.c"
    break;

  case 269: /* dimexpr: dimexpr '/' dimexpr  */
#line 2497 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 4728 "ascend/compiler/ascParse.c"
    break;

  case 270: /* dimexpr: dimexpr '*' dimexpr  */
#line 2501 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 4736 "ascend/compiler/ascParse.c"
    break;

  case 271: /* dimexpr: dimexpr '^' fraction  */
#line 2505 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 4744 "ascend/compiler/ascParse.c"
    break;

  case 272: /* dimexpr: '(' dimexpr ')'  */
#line 2509 "ascend/compiler/ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 4752 "ascend/compiler/ascParse.c"
    break;

  case 273: /* fraction: optional_sign fractail  */
#line 2516 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 4760 "ascend/compiler/ascParse.c"
    break;

  case 274: /* fractail: INTEGER_TOK  */
#line 2523 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 4768 "ascend/compiler/ascParse.c"
    break;

  case 275: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 2527 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 4776 "ascend/compiler/ascParse.c"
    break;

  case 276: /* optional_sign: %empty  */
#line 2534 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 4784 "ascend/compiler/ascParse.c"
    break;

  case 277: /* optional_sign: '+'  */
#line 2538 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 4792 "ascend/compiler/ascParse.c"
    break;

  case 278: /* optional_sign: '-'  */
#line 2542 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 4800 "ascend/compiler/ascParse.c"
    break;

  case 279: /* expr: INTEGER_TOK  */
#line 2549 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 4808 "ascend/compiler/ascParse.c"
    break;

  case 280: /* expr: MAXINTEGER_TOK  */
#line 2553 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 4816 "ascend/compiler/ascParse.c"
    break;

  case 281: /* expr: realnumber  */
#line 2557 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 4824 "ascend/compiler/ascParse.c"
    break;

  case 282: /* expr: MAXREAL_TOK  */
#line 2561 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 4832 "ascend/compiler/ascParse.c"
    break;

  case 283: /* expr: TRUE_TOK  */
#line 2565 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 4840 "ascend/compiler/ascParse.c"
    break;

  case 284: /* expr: FALSE_TOK  */
#line 2569 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 4848 "ascend/compiler/ascParse.c"
    break;

  case 285: /* expr: ANY_TOK  */
#line 2573 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 4856 "ascend/compiler/ascParse.c"
    break;

  case 286: /* expr: SYMBOL_TOK  */
#line 2577 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 4864 "ascend/compiler/ascParse.c"
    break;

  case 287: /* expr: fname  */
#line 2581 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 4872 "ascend/compiler/ascParse.c"
    break;

  case 288: /* expr: '[' set ']'  */
#line 2585 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 4880 "ascend/compiler/ascParse.c"
    break;

  case 289: /* expr: expr '+' expr  */
#line 2589 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4889 "ascend/compiler/ascParse.c"
    break;

  case 290: /* expr: expr '-' expr  */
#line 2594 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4898 "ascend/compiler/ascParse.c"
    break;

  case 291: /* expr: expr '*' expr  */
#line 2599 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4907 "ascend/compiler/ascParse.c"
    break;

  case 292: /* expr: expr '/' expr  */
#line 2604 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4916 "ascend/compiler/ascParse.c"
    break;

  case 293: /* expr: expr '^' expr  */
#line 2609 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4925 "ascend/compiler/ascParse.c"
    break;

  case 294: /* expr: expr AND_TOK expr  */
#line 2614 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4934 "ascend/compiler/ascParse.c"
    break;

  case 295: /* expr: expr OR_TOK expr  */
#line 2619 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4943 "ascend/compiler/ascParse.c"
    break;

  case 296: /* expr: NOT_TOK expr  */
#line 2624 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 4951 "ascend/compiler/ascParse.c"
    break;

  case 297: /* expr: expr relop expr  */
#line 2628 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4960 "ascend/compiler/ascParse.c"
    break;

  case 298: /* expr: expr logrelop expr  */
#line 2633 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4969 "ascend/compiler/ascParse.c"
    break;

  case 299: /* expr: expr IN_TOK expr  */
#line 2638 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4978 "ascend/compiler/ascParse.c"
    break;

  case 300: /* expr: expr '|' expr  */
#line 2643 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4987 "ascend/compiler/ascParse.c"
    break;

  case 301: /* expr: expr SUCHTHAT_TOK expr  */
#line 2648 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4996 "ascend/compiler/ascParse.c"
    break;

  case 302: /* expr: '+' expr  */
#line 2653 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 5004 "ascend/compiler/ascParse.c"
    break;

  case 303: /* expr: '-' expr  */
#line 2657 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 5012 "ascend/compiler/ascParse.c"
    break;

  case 304: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 2661 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 5020 "ascend/compiler/ascParse.c"
    break;

  case 305: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 2665 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 5028 "ascend/compiler/ascParse.c"
    break;

  case 306: /* expr: SUM_TOK '(' set ')'  */
#line 2669 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 5039 "ascend/compiler/ascParse.c"
    break;

  case 307: /* expr: SUM_TOK '[' set ']'  */
#line 2676 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 5047 "ascend/compiler/ascParse.c"
    break;

  case 308: /* expr: PROD_TOK '(' set ')'  */
#line 2680 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 5058 "ascend/compiler/ascParse.c"
    break;

  case 309: /* expr: PROD_TOK '[' set ']'  */
#line 2687 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 5066 "ascend/compiler/ascParse.c"
    break;

  case 310: /* expr: UNION_TOK '(' set ')'  */
#line 2691 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 5077 "ascend/compiler/ascParse.c"
    break;

  case 311: /* expr: UNION_TOK '[' set ']'  */
#line 2698 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 5085 "ascend/compiler/ascParse.c"
    break;

  case 312: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 2702 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 5096 "ascend/compiler/ascParse.c"
    break;

  case 313: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 2709 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 5104 "ascend/compiler/ascParse.c"
    break;

  case 314: /* expr: CARD_TOK '(' set ')'  */
#line 2713 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 5115 "ascend/compiler/ascParse.c"
    break;

  case 315: /* expr: CARD_TOK '[' set ']'  */
#line 2720 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 5123 "ascend/compiler/ascParse.c"
    break;

  case 316: /* expr: CHOICE_TOK '(' set ')'  */
#line 2724 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 5134 "ascend/compiler/ascParse.c"
    break;

  case 317: /* expr: CHOICE_TOK '[' set ']'  */
#line 2731 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 5142 "ascend/compiler/ascParse.c"
    break;

  case 318: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 2735 "ascend/compiler/ascParse.y"
        {
	  CONST struct Func *fptr;
	  if ((fptr = LookupFunc(SCP((yyvsp[-3].id_ptr))))!=NULL) {
	    (yyval.eptr) = JoinExprLists((yyvsp[-1].eptr),CreateFuncExpr(fptr));
	  } else {
	    (yyval.eptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Function '%s' is not defined.",SCP((yyvsp[-3].id_ptr)));
	    g_untrapped_error++;
	  }
	}
#line 5157 "ascend/compiler/ascParse.c"
    break;

  case 319: /* expr: '(' expr ')'  */
#line 2746 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 5165 "ascend/compiler/ascParse.c"
    break;

  case 320: /* relop: '='  */
#line 2753 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 5173 "ascend/compiler/ascParse.c"
    break;

  case 321: /* relop: '<'  */
#line 2757 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 5181 "ascend/compiler/ascParse.c"
    break;

  case 322: /* relop: '>'  */
#line 2761 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 5189 "ascend/compiler/ascParse.c"
    break;

  case 323: /* relop: LEQ_TOK  */
#line 2765 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 5197 "ascend/compiler/ascParse.c"
    break;

  case 324: /* relop: GEQ_TOK  */
#line 2769 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 5205 "ascend/compiler/ascParse.c"
    break;

  case 325: /* relop: NEQ_TOK  */
#line 2773 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 5213 "ascend/compiler/ascParse.c"
    break;

  case 326: /* logrelop: BEQ_TOK  */
#line 2780 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 5221 "ascend/compiler/ascParse.c"
    break;

  case 327: /* logrelop: BNE_TOK  */
#line 2784 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 5229 "ascend/compiler/ascParse.c"
    break;


#line 5233 "ascend/compiler/ascParse.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 2788 "ascend/compiler/ascParse.y"

/* END OF GRAMMAR RULES
   -----------------------------------------------------------------------------
   START OF EPILOGUE
*/

/*
 * We really need to do something about freeing up the productions
 * that invoke this so we don't leak memory like a seive.
 * for example  z[i IN [1..2]][j IN [process[i]] IS_A mass; eats a ton.
 */
int
zz_error(char *s){
  g_untrapped_error++;
  if (Asc_CurrentModule() != NULL) {
    error_reporter_current_line(ASC_USER_ERROR,"%s",s);
  } else {
    error_reporter(ASC_USER_ERROR,NULL,0,NULL,"%s at end of input.",s);
  }
  return 0;
}

/*
 *  See the header file scanner.h for a description of this function.
 */
void
Asc_ErrMsgTypeDefnEOF(void)
{
  /*  Check g_type_name to see if we're in the middle of a type
   *  definition.  If NULL no, otherwise yes.
   */
  if ( g_type_name ) {
    error_reporter_current_line(ASC_USER_ERROR,
            "End of file reached in a type definition. Incomplete definition for '%s'.",
            SCP(g_type_name));
  }
}

#define ERRCOUNT_PARSERGENERIC 30
/*
 *  void ErrMsg_*(void)
 *
 *  The following print error and warning messages to the filehandles
 *  ASCERR and ASCWARN, respectively.
 *  The type of error/warning that will be printed is indicated by the
 *  functions name and the arguments to fprintf.
 */
static void ErrMsg_Generic(CONST char *string){
	static int errcount=0;
	if(errcount<30){ 
		/* the module may have be already closed, Asc_CurrentModule will be null */
		error_reporter_current_line(ASC_USER_ERROR,"%s",string);

		if (g_type_name != NULL) {
		error_reporter_current_line(ASC_USER_ERROR,"    type %s\n",SCP(g_type_name));
		}
		if (g_proc_name != NULL) {
			error_reporter_current_line(ASC_USER_ERROR,"    METHOD %s\n",SCP(g_proc_name));
		}

		errcount++;
		if(errcount==30){
			ERROR_REPORTER_HERE(ASC_PROG_NOTE
				,"Further reports of this error will be suppressed.\n"
			);
		}
	}
}

static void ErrMsg_CommaName(CONST char *what, struct Name *name)
{
  struct module_t *mod;

  /* the module may have be already closed */
  mod = Asc_CurrentModule();

  ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
  FPRINTF(ASCERR, "Missing comma or operator before %s '",what);
  WriteName(ASCERR,name);
  FPRINTF(ASCERR, "'");
  error_reporter_end_flush();
}

#if COMMAEXPR_NOTBUGGY
static void ErrMsg_CommaExpr(CONST char *what, struct Expr *eptr)
{
  struct module_t *mod;

  /* the module may have be already closed */
  error_reporter_current_line(ASC_USER_ERROR, "ASC-Error: Missing comma before %s ",what);
  WriteExpr(ASCERR,eptr);
}
#endif /* COMMAEXPR_NOTBUGGY. delete if can't fix */

static void
ErrMsg_NullDefPointer(CONST char *object)
{
  error_reporter_current_line(ASC_USER_ERROR,"Rejected '%s'", object);
}

static void
ErrMsg_ParensBrackets(CONST char *operation)
{
  error_reporter_current_line(ASC_USER_ERROR,
          "  You should be using %s[] not %s()",
          operation,
          operation);
}


/**
	Print a warning message that the token after the END keyword did not
	match what we were expecting for the current statement.

	@param statement --the current statement, e.g. ATOM, METHOD, FOR, IF, CASE
	@param opt_name  --the name of the thing we were defining for ATOMs, METHODs,
		etc, or NULL anonymous statements (FOR, IF, CASE, etc)
	@param end_token --the TOKEN_TOK that we were received instead.  We use the
		TokenAsString to produce a string given a TOKEN_TOK
	@param expecting --the keyword we were expecting to see after the END; if
	NULL, we were expecting the string given in statement
*/
static void
WarnMsg_MismatchEnd(CONST char *statement, CONST char *opt_name,
		    unsigned long end_token, CONST char *expecting)
{
  error_reporter_current_line(ASC_USER_WARNING,
          "%s %s terminated with 'END %s;', expecting 'END %s;'"
          ,statement
          ,((opt_name != NULL) ? opt_name : "statement")
          ,TokenAsString(end_token)
          ,((expecting != NULL) ? expecting : statement));
}


/*
	Take a TOKEN_TOK (e.g., FOR_TOK, MODEL_TOK, END_TOK, IDENTIFIER_TOK) and returns
	a string representation of it:
	    e.g.:  TokenAsString(FOR_TOK) ==> "FOR"
	
	Since this function is only used inside WarnMsg_MismatchEnd, we do a
	couple of things specific to that function:  If token is END_TOK, we
	return an empty string, and if it is IDENTIFIER_TOK, we return the
	current value of g_end_identifier, or UNKNOWN if g_end_identifier is
	NULL.
*/
static CONST char *
TokenAsString(unsigned long token)
{
  switch( token ) {
  case ATOM_TOK:
    return "ATOM";
  case CONDITIONAL_TOK:
    return "CONDITIONAL";
  case FOR_TOK:
    return "FOR";
  case ASSERT_TOK:
	return "ASSERT";
  case IF_TOK:
    return "IF";
  case INTERACTIVE_TOK:
    return "INTERACTIVE";
  case METHOD_TOK:
    return "METHOD";
  case METHODS_TOK:
    return "METHODS";
  case MODEL_TOK:
    return "MODEL";
  case NOTES_TOK:
    return "NOTES";
  case PATCH_TOK:
    return "PATCH";
  case SELECT_TOK:
    return "SELECT";
  case SWITCH_TOK:
    return "SWITCH";
  case UNITS_TOK:
    return "UNITS";
  case WHEN_TOK:
    return "WHEN";
  case END_TOK:
    return "";
  case IDENTIFIER_TOK:
  default:
    if( g_end_identifier != NULL ) {
      return SCP(g_end_identifier);
    } else {
      return "UNKNOWN";
    }
  }
}

/* need a refcount game on the text field of the note. must keep
 * original note to avoid losing the varlist.
 */
static void ProcessNotes(int keep)
{
  int c,len;
  if (g_notelist == NULL) {
    return;
  }
  if (keep) {
    len = gl_length(g_notelist);
    for (c=1;c <= len;c++) {
      CommitNote(LibraryNote(),gl_fetch(g_notelist,c));
    }
  } else {
    gl_iterate(g_notelist,(void (*) (VOIDPTR))DestroyNote);
  }
  gl_destroy(g_notelist);
  g_notelist = NULL;
}

static void CollectNote(struct Note *n)
{
  if (g_notelist == NULL) {
    g_notelist = gl_create(50L);
  }
  if (g_notelist == NULL) {
    DestroyNote(n);
    return;
  }
  gl_append_ptr(g_notelist,(VOIDPTR)n);
}

/*
	This can be called as error_reporter_current_line(ASC_USER_ERROR,...);
	or error_reporter_current_line(ASC_USER_WARNING,...), or with any of the other 
	severity flags.
*/
static void error_reporter_current_line(const error_severity_t sev, const char *fmt,...){
	va_list args;
	va_start(args,fmt);
	va_error_reporter(sev,Asc_ModuleBestName(Asc_CurrentModule()),(int)LineNum(),NULL,fmt,args);
	va_end(args);
}

/* vim: set ts=8: */

