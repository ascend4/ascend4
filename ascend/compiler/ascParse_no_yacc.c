/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7"

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
#define yylval          zz_lval
#define yychar          zz_char
#define yydebug         zz_debug
#define yynerrs         zz_nerrs

/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
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
#include <ascend/compiler/event.h>
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
static struct VariableList *g_like_children; /* children of the last atom parsed
                                                      from the LIKE_CHILDREN list */

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


/* Line 371 of yacc.c  */
#line 347 "ascend/compiler/ascParse.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "ascParse.h".  */
#ifndef YY_ZZ_ASCEND_COMPILER_ASCPARSE_H_INCLUDED
# define YY_ZZ_ASCEND_COMPILER_ASCPARSE_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int zz_debug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ADD_TOK = 258,
     ALIASES_TOK = 259,
     AND_TOK = 260,
     ANY_TOK = 261,
     AREALIKE_TOK = 262,
     ARETHESAME_TOK = 263,
     ARRAY_TOK = 264,
     ASSERT_TOK = 265,
     ATOM_TOK = 266,
     BEQ_TOK = 267,
     BNE_TOK = 268,
     BREAK_TOK = 269,
     CALL_TOK = 270,
     CARD_TOK = 271,
     CASE_TOK = 272,
     CHOICE_TOK = 273,
     CHECK_TOK = 274,
     CHILDREN_TOK = 275,
     CONDITIONAL_TOK = 276,
     CONSTANT_TOK = 277,
     CONTINUE_TOK = 278,
     CREATE_TOK = 279,
     DATA_TOK = 280,
     DECREASING_TOK = 281,
     DEFAULT_TOK = 282,
     DEFINITION_TOK = 283,
     DER_TOK = 284,
     DIMENSION_TOK = 285,
     DERIV_TOK = 286,
     DERIVATIVE_TOK = 287,
     DIMENSIONLESS_TOK = 288,
     DO_TOK = 289,
     ELSE_TOK = 290,
     END_TOK = 291,
     EVENT_TOK = 292,
     EXPECT_TOK = 293,
     EXTERNAL_TOK = 294,
     FALSE_TOK = 295,
     FALLTHRU_TOK = 296,
     FIX_TOK = 297,
     FOR_TOK = 298,
     FREE_TOK = 299,
     FROM_TOK = 300,
     GLOBAL_TOK = 301,
     IF_TOK = 302,
     IGNORE_TOK = 303,
     IMPORT_TOK = 304,
     IN_TOK = 305,
     INPUT_TOK = 306,
     INCREASING_TOK = 307,
     INTERACTIVE_TOK = 308,
     INDEPENDENT_TOK = 309,
     INTERSECTION_TOK = 310,
     ISA_TOK = 311,
     _IS_T = 312,
     ISREFINEDTO_TOK = 313,
     LIKE_TOK = 314,
     LINK_TOK = 315,
     MAXIMIZE_TOK = 316,
     MAXINTEGER_TOK = 317,
     MAXREAL_TOK = 318,
     METHODS_TOK = 319,
     METHOD_TOK = 320,
     MINIMIZE_TOK = 321,
     MODEL_TOK = 322,
     NOT_TOK = 323,
     NOTES_TOK = 324,
     OF_TOK = 325,
     OPTION_TOK = 326,
     OR_TOK = 327,
     OTHERWISE_TOK = 328,
     OUTPUT_TOK = 329,
     PATCH_TOK = 330,
     PRE_TOK = 331,
     PREVIOUS_TOK = 332,
     PROD_TOK = 333,
     PROVIDE_TOK = 334,
     REFINES_TOK = 335,
     REPLACE_TOK = 336,
     REQUIRE_TOK = 337,
     RETURN_TOK = 338,
     RUN_TOK = 339,
     SATISFIED_TOK = 340,
     SELECT_TOK = 341,
     SIZE_TOK = 342,
     SOLVE_TOK = 343,
     SOLVER_TOK = 344,
     STOP_TOK = 345,
     SUCHTHAT_TOK = 346,
     SUM_TOK = 347,
     SWITCH_TOK = 348,
     THEN_TOK = 349,
     TRUE_TOK = 350,
     UNION_TOK = 351,
     UNITS_TOK = 352,
     UNIVERSAL_TOK = 353,
     UNLINK_TOK = 354,
     WHEN_TOK = 355,
     WHERE_TOK = 356,
     WHILE_TOK = 357,
     WILLBE_TOK = 358,
     WILLBETHESAME_TOK = 359,
     WILLNOTBETHESAME_TOK = 360,
     ASSIGN_TOK = 361,
     CASSIGN_TOK = 362,
     DBLCOLON_TOK = 363,
     USE_TOK = 364,
     LEQ_TOK = 365,
     GEQ_TOK = 366,
     NEQ_TOK = 367,
     DOTDOT_TOK = 368,
     WITH_TOK = 369,
     VALUE_TOK = 370,
     WITH_VALUE_T = 371,
     REAL_TOK = 372,
     INTEGER_TOK = 373,
     IDENTIFIER_TOK = 374,
     BRACEDTEXT_TOK = 375,
     SYMBOL_TOK = 376,
     DQUOTE_TOK = 377,
     UPLUS_TOK = 378,
     UMINUS_TOK = 379
   };
#endif
/* Tokens.  */
#define ADD_TOK 258
#define ALIASES_TOK 259
#define AND_TOK 260
#define ANY_TOK 261
#define AREALIKE_TOK 262
#define ARETHESAME_TOK 263
#define ARRAY_TOK 264
#define ASSERT_TOK 265
#define ATOM_TOK 266
#define BEQ_TOK 267
#define BNE_TOK 268
#define BREAK_TOK 269
#define CALL_TOK 270
#define CARD_TOK 271
#define CASE_TOK 272
#define CHOICE_TOK 273
#define CHECK_TOK 274
#define CHILDREN_TOK 275
#define CONDITIONAL_TOK 276
#define CONSTANT_TOK 277
#define CONTINUE_TOK 278
#define CREATE_TOK 279
#define DATA_TOK 280
#define DECREASING_TOK 281
#define DEFAULT_TOK 282
#define DEFINITION_TOK 283
#define DER_TOK 284
#define DIMENSION_TOK 285
#define DERIV_TOK 286
#define DERIVATIVE_TOK 287
#define DIMENSIONLESS_TOK 288
#define DO_TOK 289
#define ELSE_TOK 290
#define END_TOK 291
#define EVENT_TOK 292
#define EXPECT_TOK 293
#define EXTERNAL_TOK 294
#define FALSE_TOK 295
#define FALLTHRU_TOK 296
#define FIX_TOK 297
#define FOR_TOK 298
#define FREE_TOK 299
#define FROM_TOK 300
#define GLOBAL_TOK 301
#define IF_TOK 302
#define IGNORE_TOK 303
#define IMPORT_TOK 304
#define IN_TOK 305
#define INPUT_TOK 306
#define INCREASING_TOK 307
#define INTERACTIVE_TOK 308
#define INDEPENDENT_TOK 309
#define INTERSECTION_TOK 310
#define ISA_TOK 311
#define _IS_T 312
#define ISREFINEDTO_TOK 313
#define LIKE_TOK 314
#define LINK_TOK 315
#define MAXIMIZE_TOK 316
#define MAXINTEGER_TOK 317
#define MAXREAL_TOK 318
#define METHODS_TOK 319
#define METHOD_TOK 320
#define MINIMIZE_TOK 321
#define MODEL_TOK 322
#define NOT_TOK 323
#define NOTES_TOK 324
#define OF_TOK 325
#define OPTION_TOK 326
#define OR_TOK 327
#define OTHERWISE_TOK 328
#define OUTPUT_TOK 329
#define PATCH_TOK 330
#define PRE_TOK 331
#define PREVIOUS_TOK 332
#define PROD_TOK 333
#define PROVIDE_TOK 334
#define REFINES_TOK 335
#define REPLACE_TOK 336
#define REQUIRE_TOK 337
#define RETURN_TOK 338
#define RUN_TOK 339
#define SATISFIED_TOK 340
#define SELECT_TOK 341
#define SIZE_TOK 342
#define SOLVE_TOK 343
#define SOLVER_TOK 344
#define STOP_TOK 345
#define SUCHTHAT_TOK 346
#define SUM_TOK 347
#define SWITCH_TOK 348
#define THEN_TOK 349
#define TRUE_TOK 350
#define UNION_TOK 351
#define UNITS_TOK 352
#define UNIVERSAL_TOK 353
#define UNLINK_TOK 354
#define WHEN_TOK 355
#define WHERE_TOK 356
#define WHILE_TOK 357
#define WILLBE_TOK 358
#define WILLBETHESAME_TOK 359
#define WILLNOTBETHESAME_TOK 360
#define ASSIGN_TOK 361
#define CASSIGN_TOK 362
#define DBLCOLON_TOK 363
#define USE_TOK 364
#define LEQ_TOK 365
#define GEQ_TOK 366
#define NEQ_TOK 367
#define DOTDOT_TOK 368
#define WITH_TOK 369
#define VALUE_TOK 370
#define WITH_VALUE_T 371
#define REAL_TOK 372
#define INTEGER_TOK 373
#define IDENTIFIER_TOK 374
#define BRACEDTEXT_TOK 375
#define SYMBOL_TOK 376
#define DQUOTE_TOK 377
#define UPLUS_TOK 378
#define UMINUS_TOK 379



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 305 "ascend/compiler/ascParse.y"

  double real_value;
  long   int_value;
  struct fraction frac_value;
  symchar *id_ptr;
  CONST char *braced_ptr;	/* pointer for units, explanations, tables */
  symchar *sym_ptr;		/* pointer for symbols */
  CONST char *dquote_ptr;       /* for text in "double quotes" */
  struct Name *nptr;
  struct Expr *eptr;
  struct Set *sptr;
  struct VariableList *lptr;
  struct Statement *statptr;
  struct StatementList *slptr;
  struct SelectList *septr;
  struct SwitchList *swptr;
  struct WhenList *wptr;
  struct EventList *evptr;
  struct NoteTmp *notesptr;	/* change this once struct Notes is defined */
  struct gl_list_t *listp;
  struct InitProcedure *procptr;
  CONST dim_type *dimp;
  struct TypeDescription *tptr;
  struct UnitDefinition *udefptr;
  dim_type dimen;
  enum ForOrder order;
  enum ForKind fkind;


/* Line 387 of yacc.c  */
#line 668 "ascend/compiler/ascParse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE zz_lval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int zz_parse (void *YYPARSE_PARAM);
#else
int zz_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int zz_parse (void);
#else
int zz_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_ZZ_ASCEND_COMPILER_ASCPARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 696 "ascend/compiler/ascParse.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
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
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1356

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  142
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  132
/* YYNRULES -- Number of rules.  */
#define YYNRULES  348
/* YYNRULES -- Number of states.  */
#define YYNSTATES  723

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   379

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     136,   137,   131,   128,   123,   129,   139,   130,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   138,   135,
     125,   126,   127,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   140,     2,   141,   134,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   124,     2,     2,     2,     2,     2,
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
     115,   116,   117,   118,   119,   120,   121,   122,   132,   133
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    41,
      45,    49,    52,    56,    60,    63,    69,    73,    78,    85,
      90,    96,   103,   108,   114,   121,   128,   136,   139,   140,
     144,   147,   150,   153,   154,   158,   161,   170,   171,   175,
     178,   181,   184,   191,   196,   204,   207,   208,   212,   213,
     218,   219,   223,   229,   235,   236,   238,   244,   247,   250,
     254,   255,   258,   263,   264,   265,   269,   271,   272,   275,
     281,   284,   286,   287,   291,   295,   299,   301,   303,   305,
     307,   309,   311,   313,   315,   317,   319,   321,   323,   325,
     327,   329,   331,   333,   335,   337,   339,   341,   343,   345,
     347,   349,   351,   353,   355,   357,   359,   361,   363,   365,
     367,   369,   371,   373,   375,   377,   379,   381,   387,   393,
     397,   410,   411,   416,   421,   424,   425,   428,   433,   437,
     439,   444,   446,   451,   452,   455,   456,   459,   460,   463,
     466,   475,   482,   489,   496,   503,   508,   511,   514,   517,
     520,   524,   528,   530,   534,   536,   539,   542,   552,   556,
     560,   561,   566,   576,   577,   580,   589,   590,   592,   594,
     596,   598,   600,   602,   605,   610,   613,   616,   619,   622,
     626,   628,   634,   637,   640,   647,   653,   654,   657,   662,
     669,   676,   685,   687,   692,   696,   702,   707,   709,   711,
     713,   715,   718,   721,   726,   733,   740,   749,   751,   756,
     760,   766,   771,   776,   783,   785,   790,   794,   800,   805,
     810,   817,   819,   824,   828,   834,   839,   843,   847,   850,
     854,   857,   861,   863,   865,   869,   872,   875,   877,   881,
     886,   891,   896,   899,   902,   905,   908,   911,   914,   917,
     920,   923,   926,   929,   932,   935,   938,   940,   941,   943,
     944,   946,   948,   949,   951,   955,   959,   965,   967,   969,
     972,   975,   976,   978,   981,   983,   984,   986,   988,   990,
     992,   996,  1000,  1004,  1008,  1011,  1013,  1019,  1020,  1022,
    1024,  1026,  1028,  1030,  1032,  1034,  1036,  1038,  1040,  1042,
    1046,  1050,  1054,  1058,  1062,  1066,  1070,  1074,  1077,  1081,
    1085,  1089,  1093,  1097,  1100,  1103,  1110,  1115,  1120,  1125,
    1130,  1135,  1140,  1145,  1150,  1155,  1160,  1165,  1170,  1175,
    1180,  1184,  1186,  1188,  1190,  1192,  1194,  1196,  1198
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     143,     0,    -1,    -1,   143,   144,    -1,   146,    -1,   147,
      -1,   148,    -1,   151,    -1,   153,    -1,   149,    -1,   160,
      -1,   155,    -1,   163,    -1,   172,    -1,   169,    -1,   174,
      -1,   145,    -1,     1,    -1,    46,   135,   184,   257,   135,
      -1,    82,   122,   135,    -1,    82,   256,   135,    -1,    82,
     256,    -1,    79,   122,   135,    -1,    79,   256,   135,    -1,
      79,   256,    -1,    49,   119,    45,   122,   135,    -1,    49,
     122,   135,    -1,   150,   251,   257,   135,    -1,     3,    69,
      50,   119,   199,   135,    -1,   152,   180,   257,   135,    -1,
       3,    64,    50,   119,   135,    -1,     3,    64,    50,    28,
      67,   135,    -1,   154,   180,   257,   135,    -1,    81,    64,
      50,   119,   135,    -1,    81,    64,    50,    28,    67,   135,
      -1,   171,   156,   184,   178,   257,   135,    -1,   157,    80,
     119,   265,   158,   159,   135,    -1,    11,   119,    -1,    -1,
      27,   270,   262,    -1,    27,    40,    -1,    27,    95,    -1,
      27,   121,    -1,    -1,    59,    20,   253,    -1,   171,   161,
      -1,    22,   119,    80,   119,   265,   162,   259,   135,    -1,
      -1,   107,   270,   262,    -1,   107,    95,    -1,   107,    40,
      -1,   107,   121,    -1,   171,   164,   184,   178,   257,   135,
      -1,   165,   166,   167,   135,    -1,   165,   166,   167,    80,
     119,   168,   135,    -1,    67,   119,    -1,    -1,   136,   184,
     137,    -1,    -1,   101,   136,   184,   137,    -1,    -1,   136,
     184,   137,    -1,   170,   184,   178,   257,   135,    -1,    75,
     119,    43,   119,   135,    -1,    -1,    98,    -1,   173,   184,
     178,   257,   135,    -1,    28,   119,    -1,   175,   135,    -1,
      97,   176,   257,    -1,    -1,   176,   177,    -1,   119,   126,
     120,   135,    -1,    -1,    -1,    64,   179,   180,    -1,   181,
      -1,    -1,   181,   182,    -1,   183,   135,   184,   257,   135,
      -1,    65,   119,    -1,   185,    -1,    -1,   185,   186,   135,
      -1,   185,   187,   135,    -1,   185,     1,   135,    -1,   188,
      -1,   189,    -1,   190,    -1,   192,    -1,   193,    -1,   195,
      -1,   196,    -1,   202,    -1,   203,    -1,   204,    -1,   205,
      -1,   206,    -1,   207,    -1,   208,    -1,   209,    -1,   210,
      -1,   211,    -1,   217,    -1,   213,    -1,   230,    -1,   229,
      -1,   219,    -1,   222,    -1,   223,    -1,   225,    -1,   226,
      -1,   228,    -1,   227,    -1,   231,    -1,   232,    -1,   233,
      -1,   235,    -1,   239,    -1,   240,    -1,   238,    -1,   243,
      -1,   246,    -1,   249,    -1,   250,    -1,   175,    -1,   224,
      -1,   253,    56,   198,   200,   201,    -1,   253,   103,   198,
     200,   201,    -1,   253,     4,   255,    -1,   253,     4,   136,
     253,   137,   101,   253,    56,   119,    70,   119,   191,    -1,
      -1,   116,   136,   260,   137,    -1,    32,    70,   253,   194,
      -1,    77,   253,    -1,    -1,   114,   255,    -1,   253,    57,
     119,   200,    -1,   253,    58,   198,    -1,   119,    -1,   119,
     136,   260,   137,    -1,   119,    -1,   119,   136,   260,   137,
      -1,    -1,    65,   119,    -1,    -1,    70,   119,    -1,    -1,
     116,   271,    -1,   253,     7,    -1,    60,   136,    48,   123,
     121,   123,   253,   137,    -1,    60,   136,   121,   123,   253,
     137,    -1,    60,   136,   255,   123,   253,   137,    -1,    99,
     136,   121,   123,   253,   137,    -1,    99,   136,   255,   123,
     253,   137,    -1,    29,   136,   253,   137,    -1,    54,   253,
      -1,   253,     8,    -1,   253,   104,    -1,   253,   105,    -1,
     255,   106,   271,    -1,   255,   107,   271,    -1,   212,    -1,
     255,   138,   212,    -1,   271,    -1,    66,   271,    -1,    61,
     271,    -1,   255,   138,   119,   136,   214,   135,   215,   216,
     137,    -1,   253,   138,    51,    -1,   253,   138,    74,    -1,
      -1,   135,   255,   138,    25,    -1,   255,   138,   119,   136,
     253,   135,   118,   137,   218,    -1,    -1,    50,   255,    -1,
      43,   119,    50,   271,   220,   221,   184,   257,    -1,    -1,
      52,    -1,    26,    -1,    24,    -1,    38,    -1,    19,    -1,
      34,    -1,    84,   255,    -1,    84,   255,   108,   255,    -1,
      42,   253,    -1,    42,   210,    -1,    44,   253,    -1,    89,
     119,    -1,    71,   119,   271,    -1,    88,    -1,    39,   119,
     136,   253,   137,    -1,    15,   197,    -1,    10,   271,    -1,
      47,   271,    94,   184,   234,   257,    -1,   102,   271,    34,
     184,   257,    -1,    -1,    35,   184,    -1,   100,   253,   236,
     257,    -1,   255,   138,   100,   253,   236,   257,    -1,   100,
     136,   253,   137,   236,   257,    -1,   255,   138,   100,   136,
     253,   137,   236,   257,    -1,   237,    -1,    17,   260,   138,
     184,    -1,    73,   138,   184,    -1,   237,    17,   260,   138,
     184,    -1,   237,    73,   138,   184,    -1,    14,    -1,    23,
      -1,    41,    -1,    83,    -1,    90,   258,    -1,   109,   255,
      -1,    37,   253,   241,   257,    -1,   255,   138,    37,   253,
     241,   257,    -1,    37,   136,   253,   137,   241,   257,    -1,
     255,   138,    37,   136,   253,   137,   241,   257,    -1,   242,
      -1,    17,   260,   138,   184,    -1,    73,   138,   184,    -1,
     242,    17,   260,   138,   184,    -1,   242,    73,   138,   184,
      -1,    86,   253,   244,   257,    -1,    86,   136,   253,   137,
     244,   257,    -1,   245,    -1,    17,   260,   138,   184,    -1,
      73,   138,   184,    -1,   245,    17,   260,   138,   184,    -1,
     245,    73,   138,   184,    -1,    93,   253,   247,   257,    -1,
      93,   136,   253,   137,   247,   257,    -1,   248,    -1,    17,
     260,   138,   184,    -1,    73,   138,   184,    -1,   248,    17,
     260,   138,   184,    -1,   248,    73,   138,   184,    -1,    21,
     184,   257,    -1,    69,   251,   257,    -1,   121,   252,    -1,
     251,   121,   252,    -1,   253,   120,    -1,   252,   253,   120,
      -1,   254,    -1,   255,    -1,   254,   123,   255,    -1,   254,
     255,    -1,   256,   259,    -1,   119,    -1,   256,   139,   119,
      -1,   256,   140,   260,   141,    -1,    31,   136,   253,   137,
      -1,    76,   136,   255,   137,    -1,    36,    21,    -1,    36,
      37,    -1,    36,    43,    -1,    36,    47,    -1,    36,    53,
      -1,    36,    64,    -1,    36,    69,    -1,    36,    86,    -1,
      36,    93,    -1,    36,    97,    -1,    36,    46,    -1,    36,
     100,    -1,    36,   102,    -1,    36,   119,    -1,    36,    -1,
      -1,   120,    -1,    -1,   122,    -1,   261,    -1,    -1,   271,
      -1,   271,   113,   271,    -1,   261,   123,   271,    -1,   261,
     123,   271,   113,   271,    -1,   118,    -1,   263,    -1,   117,
     264,    -1,   118,   120,    -1,    -1,   120,    -1,    30,   266,
      -1,    33,    -1,    -1,   131,    -1,   267,    -1,   119,    -1,
     118,    -1,   267,   130,   267,    -1,   267,   131,   267,    -1,
     267,   134,   268,    -1,   136,   267,   137,    -1,   270,   269,
      -1,   118,    -1,   136,   118,   130,   118,   137,    -1,    -1,
     128,    -1,   129,    -1,   118,    -1,    62,    -1,   263,    -1,
      63,    -1,    95,    -1,    40,    -1,     6,    -1,   121,    -1,
     255,    -1,   140,   260,   141,    -1,   271,   128,   271,    -1,
     271,   129,   271,    -1,   271,   131,   271,    -1,   271,   130,
     271,    -1,   271,   134,   271,    -1,   271,     5,   271,    -1,
     271,    72,   271,    -1,    68,   271,    -1,   271,   272,   271,
      -1,   271,   273,   271,    -1,   271,    50,   271,    -1,   271,
     124,   271,    -1,   271,    91,   271,    -1,   128,   271,    -1,
     129,   271,    -1,    85,   136,   255,   123,   263,   137,    -1,
      85,   136,   255,   137,    -1,    92,   136,   260,   137,    -1,
      92,   140,   260,   141,    -1,    78,   136,   260,   137,    -1,
      78,   140,   260,   141,    -1,    96,   136,   260,   137,    -1,
      96,   140,   260,   141,    -1,    55,   136,   260,   137,    -1,
      55,   140,   260,   141,    -1,    16,   136,   260,   137,    -1,
      16,   140,   260,   141,    -1,    18,   136,   260,   137,    -1,
      18,   140,   260,   141,    -1,   119,   136,   271,   137,    -1,
     136,   271,   137,    -1,   126,    -1,   125,    -1,   127,    -1,
     110,    -1,   111,    -1,   112,    -1,    12,    -1,    13,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   429,   429,   431,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   455,   500,
     504,   509,   517,   521,   526,   534,   543,   555,   586,   594,
     617,   630,   638,   659,   671,   679,   732,   744,   753,   758,
     763,   770,   777,   788,   791,   797,   836,   871,   876,   881,
     888,   895,   905,   946,   955,   968,   977,   980,   988,   991,
     999,  1002,  1009,  1041,  1055,  1058,  1065,  1095,  1104,  1109,
    1129,  1132,  1140,  1150,  1154,  1153,  1167,  1175,  1178,  1203,
    1217,  1226,  1234,  1237,  1245,  1252,  1260,  1261,  1262,  1263,
    1264,  1265,  1266,  1267,  1268,  1269,  1270,  1271,  1272,  1273,
    1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,  1282,  1283,
    1284,  1285,  1286,  1287,  1288,  1289,  1290,  1291,  1292,  1293,
    1294,  1295,  1296,  1297,  1298,  1299,  1303,  1308,  1349,  1378,
    1382,  1433,  1436,  1443,  1469,  1493,  1496,  1503,  1515,  1542,
    1547,  1555,  1560,  1569,  1572,  1580,  1583,  1591,  1594,  1601,
    1608,  1612,  1616,  1623,  1627,  1634,  1643,  1652,  1659,  1666,
    1673,  1677,  1684,  1697,  1714,  1724,  1732,  1743,  1764,  1771,
    1779,  1782,  1789,  1810,  1813,  1820,  1842,  1845,  1849,  1856,
    1860,  1864,  1868,  1875,  1879,  1886,  1894,  1908,  1915,  1923,
    1931,  1939,  1950,  1961,  1967,  1977,  1987,  1990,  1997,  2008,
    2020,  2027,  2037,  2044,  2048,  2052,  2056,  2063,  2067,  2071,
    2075,  2079,  2086,  2093,  2104,  2115,  2122,  2132,  2139,  2143,
    2147,  2151,  2158,  2169,  2179,  2186,  2190,  2194,  2198,  2205,
    2216,  2226,  2233,  2237,  2241,  2245,  2252,  2262,  2299,  2308,
    2323,  2328,  2337,  2348,  2352,  2357,  2370,  2390,  2394,  2399,
    2416,  2420,  2427,  2432,  2437,  2442,  2447,  2452,  2457,  2462,
    2467,  2472,  2477,  2482,  2487,  2492,  2497,  2506,  2509,  2517,
    2520,  2527,  2532,  2538,  2542,  2546,  2551,  2559,  2565,  2574,
    2578,  2600,  2604,  2625,  2629,  2634,  2640,  2644,  2651,  2655,
    2659,  2663,  2667,  2671,  2678,  2685,  2689,  2697,  2700,  2704,
    2711,  2715,  2719,  2723,  2727,  2731,  2735,  2739,  2743,  2747,
    2751,  2756,  2761,  2766,  2771,  2776,  2781,  2786,  2790,  2795,
    2800,  2805,  2810,  2815,  2819,  2823,  2827,  2831,  2838,  2842,
    2849,  2853,  2860,  2864,  2871,  2875,  2882,  2886,  2893,  2897,
    2908,  2915,  2919,  2923,  2927,  2931,  2935,  2942,  2946
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ADD_TOK", "ALIASES_TOK", "AND_TOK",
  "ANY_TOK", "AREALIKE_TOK", "ARETHESAME_TOK", "ARRAY_TOK", "ASSERT_TOK",
  "ATOM_TOK", "BEQ_TOK", "BNE_TOK", "BREAK_TOK", "CALL_TOK", "CARD_TOK",
  "CASE_TOK", "CHOICE_TOK", "CHECK_TOK", "CHILDREN_TOK", "CONDITIONAL_TOK",
  "CONSTANT_TOK", "CONTINUE_TOK", "CREATE_TOK", "DATA_TOK",
  "DECREASING_TOK", "DEFAULT_TOK", "DEFINITION_TOK", "DER_TOK",
  "DIMENSION_TOK", "DERIV_TOK", "DERIVATIVE_TOK", "DIMENSIONLESS_TOK",
  "DO_TOK", "ELSE_TOK", "END_TOK", "EVENT_TOK", "EXPECT_TOK",
  "EXTERNAL_TOK", "FALSE_TOK", "FALLTHRU_TOK", "FIX_TOK", "FOR_TOK",
  "FREE_TOK", "FROM_TOK", "GLOBAL_TOK", "IF_TOK", "IGNORE_TOK",
  "IMPORT_TOK", "IN_TOK", "INPUT_TOK", "INCREASING_TOK", "INTERACTIVE_TOK",
  "INDEPENDENT_TOK", "INTERSECTION_TOK", "ISA_TOK", "_IS_T",
  "ISREFINEDTO_TOK", "LIKE_TOK", "LINK_TOK", "MAXIMIZE_TOK",
  "MAXINTEGER_TOK", "MAXREAL_TOK", "METHODS_TOK", "METHOD_TOK",
  "MINIMIZE_TOK", "MODEL_TOK", "NOT_TOK", "NOTES_TOK", "OF_TOK",
  "OPTION_TOK", "OR_TOK", "OTHERWISE_TOK", "OUTPUT_TOK", "PATCH_TOK",
  "PRE_TOK", "PREVIOUS_TOK", "PROD_TOK", "PROVIDE_TOK", "REFINES_TOK",
  "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK", "SATISFIED_TOK",
  "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK", "SOLVER_TOK", "STOP_TOK",
  "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK", "THEN_TOK", "TRUE_TOK",
  "UNION_TOK", "UNITS_TOK", "UNIVERSAL_TOK", "UNLINK_TOK", "WHEN_TOK",
  "WHERE_TOK", "WHILE_TOK", "WILLBE_TOK", "WILLBETHESAME_TOK",
  "WILLNOTBETHESAME_TOK", "ASSIGN_TOK", "CASSIGN_TOK", "DBLCOLON_TOK",
  "USE_TOK", "LEQ_TOK", "GEQ_TOK", "NEQ_TOK", "DOTDOT_TOK", "WITH_TOK",
  "VALUE_TOK", "WITH_VALUE_T", "REAL_TOK", "INTEGER_TOK", "IDENTIFIER_TOK",
  "BRACEDTEXT_TOK", "SYMBOL_TOK", "DQUOTE_TOK", "','", "'|'", "'<'", "'='",
  "'>'", "'+'", "'-'", "'/'", "'*'", "UPLUS_TOK", "UMINUS_TOK", "'^'",
  "';'", "'('", "')'", "':'", "'.'", "'['", "']'", "$accept",
  "definitions", "definition", "global_def", "require_file",
  "provide_module", "import", "add_notes_def", "add_notes_head",
  "add_method_def", "add_method_head", "replace_method_def",
  "replace_method_head", "atom_def", "atom_head", "atom_id", "default_val",
  "like_children", "constant_def", "constant_head", "constant_val",
  "model_def", "model_head", "model_id", "optional_model_parameters",
  "optional_parameter_wheres", "optional_parameter_reduction", "patch_def",
  "patch_head", "universal", "definition_def", "definition_id",
  "units_def", "units_statement", "unitdeflist", "unitdef", "methods",
  "$@1", "proclist", "proclistf", "procedure", "procedure_id",
  "fstatements", "statements", "statement", "complex_statement",
  "isa_statement", "willbe_statement", "aliases_statement",
  "optional_set_values", "derivative_statement", "previous_statement",
  "optional_with", "is_statement", "isrefinedto_statement",
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
  "event_statement", "eventlist", "eventlistf", "select_statement",
  "selectlist", "selectlistf", "switch_statement", "switchlist",
  "switchlistf", "conditional_statement", "notes_statement", "notes_body",
  "noteslist", "fvarlist", "varlist", "fname", "name", "end",
  "optional_bracedtext", "optional_notes", "set", "setexprlist", "number",
  "realnumber", "opunits", "dims", "dimensions", "dimexpr", "fraction",
  "fractail", "optional_sign", "expr", "relop", "logrelop", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,    44,   124,    60,    61,    62,    43,    45,
      47,    42,   378,   379,    94,    59,    40,    41,    58,    46,
      91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   142,   143,   143,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   145,   146,
     146,   146,   147,   147,   147,   148,   148,   149,   150,   151,
     152,   152,   153,   154,   154,   155,   156,   157,   158,   158,
     158,   158,   158,   159,   159,   160,   161,   162,   162,   162,
     162,   162,   163,   164,   164,   165,   166,   166,   167,   167,
     168,   168,   169,   170,   171,   171,   172,   173,   174,   175,
     176,   176,   177,   178,   179,   178,   180,   181,   181,   182,
     183,   184,   185,   185,   185,   185,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   187,   188,   189,   190,
     190,   191,   191,   192,   193,   194,   194,   195,   196,   197,
     197,   198,   198,   199,   199,   200,   200,   201,   201,   202,
     203,   203,   203,   204,   204,   205,   206,   207,   208,   209,
     210,   210,   211,   211,   212,   212,   212,   213,   214,   215,
     216,   216,   217,   218,   218,   219,   220,   220,   220,   221,
     221,   221,   221,   222,   222,   223,   224,   225,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   234,   235,   235,
     235,   235,   236,   237,   237,   237,   237,   238,   238,   238,
     238,   238,   239,   240,   240,   240,   240,   241,   242,   242,
     242,   242,   243,   243,   244,   245,   245,   245,   245,   246,
     246,   247,   248,   248,   248,   248,   249,   250,   251,   251,
     252,   252,   253,   254,   254,   254,   255,   256,   256,   256,
     256,   256,   257,   257,   257,   257,   257,   257,   257,   257,
     257,   257,   257,   257,   257,   257,   257,   258,   258,   259,
     259,   260,   260,   261,   261,   261,   261,   262,   262,   263,
     263,   264,   264,   265,   265,   265,   266,   266,   267,   267,
     267,   267,   267,   267,   268,   269,   269,   270,   270,   270,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   272,   272,   272,   272,   272,   272,   273,   273
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     5,     3,
       3,     2,     3,     3,     2,     5,     3,     4,     6,     4,
       5,     6,     4,     5,     6,     6,     7,     2,     0,     3,
       2,     2,     2,     0,     3,     2,     8,     0,     3,     2,
       2,     2,     6,     4,     7,     2,     0,     3,     0,     4,
       0,     3,     5,     5,     0,     1,     5,     2,     2,     3,
       0,     2,     4,     0,     0,     3,     1,     0,     2,     5,
       2,     1,     0,     3,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     5,     5,     3,
      12,     0,     4,     4,     2,     0,     2,     4,     3,     1,
       4,     1,     4,     0,     2,     0,     2,     0,     2,     2,
       8,     6,     6,     6,     6,     4,     2,     2,     2,     2,
       3,     3,     1,     3,     1,     2,     2,     9,     3,     3,
       0,     4,     9,     0,     2,     8,     0,     1,     1,     1,
       1,     1,     1,     2,     4,     2,     2,     2,     2,     3,
       1,     5,     2,     2,     6,     5,     0,     2,     4,     6,
       6,     8,     1,     4,     3,     5,     4,     1,     1,     1,
       1,     2,     2,     4,     6,     6,     8,     1,     4,     3,
       5,     4,     4,     6,     1,     4,     3,     5,     4,     4,
       6,     1,     4,     3,     5,     4,     3,     3,     2,     3,
       2,     3,     1,     1,     3,     2,     2,     1,     3,     4,
       4,     4,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     0,     1,     0,
       1,     1,     0,     1,     3,     3,     5,     1,     1,     2,
       2,     0,     1,     2,     1,     0,     1,     1,     1,     1,
       3,     3,     3,     3,     2,     1,     5,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     3,     3,
       3,     3,     3,     2,     2,     6,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       3,     1,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     1,    17,     0,     0,     0,     0,     0,     0,
       0,     0,    70,    65,     3,    16,     4,     5,     6,     9,
       0,     7,    77,     8,    77,    11,    10,    12,    14,    82,
       0,    13,    82,    15,     0,     0,     0,    67,    82,     0,
       0,     0,     0,     0,   247,     0,    24,     0,     0,    21,
       0,     0,     0,     0,    76,     0,    73,     0,     0,     0,
       0,    82,     0,    45,    82,    56,    73,    68,     0,     0,
       0,     0,    26,     0,     0,     0,    22,    23,     0,   272,
       0,    19,    20,   266,     0,    71,    69,   238,     0,   242,
     243,   269,     0,     0,     0,     0,    78,     0,     0,    74,
       0,     0,   306,     0,   207,     0,     0,     0,    82,   208,
       0,     0,     0,     0,   305,   209,     0,     0,     0,     0,
       0,     0,     0,     0,   301,   303,     0,     0,     0,     0,
       0,     0,   210,     0,     0,     0,   190,     0,   267,     0,
       0,   304,     0,     0,     0,     0,     0,   281,   300,   247,
     307,     0,     0,     0,   272,   125,     0,     0,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   162,   104,   103,   107,   108,
     109,   126,   110,   111,   113,   112,   106,   105,   114,   115,
     116,   117,   120,   118,   119,   121,   122,   123,   124,     0,
     308,   302,   164,    37,     0,    55,    73,     0,    73,    82,
      58,     0,     0,     0,   143,     0,     0,     0,     0,     0,
     248,   308,     0,   271,   273,     0,     0,   252,   253,   254,
     262,   255,   256,   257,   258,   259,   260,   261,   263,   264,
     265,     0,     0,   240,     0,   245,   270,   246,   239,    27,
      29,    80,    82,    32,    77,     0,    85,   193,   139,   192,
     272,   272,   272,   272,     0,     0,     0,     0,     0,     0,
     186,   185,   243,     0,   187,     0,   156,   272,   272,     0,
     166,   165,   317,     0,     0,   134,   272,   272,   183,     0,
       0,     0,   188,   268,   211,   272,   272,     0,     0,   272,
     272,     0,     0,     0,     0,   212,   282,   279,   280,     0,
     323,   324,     0,     0,    83,    84,     0,   149,   157,     0,
       0,     0,     0,   158,   159,     0,     0,     0,     0,   347,
     348,     0,     0,     0,   344,   345,   346,     0,   342,   341,
     343,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     285,     0,     0,     0,     0,     0,     0,    30,     0,     0,
      18,    25,    63,   250,   251,   249,     0,     0,     0,    33,
       0,   241,   244,     0,    75,    62,   272,     0,     0,     0,
       0,   236,     0,   135,     0,   272,     0,     0,   217,     0,
       0,    82,     0,     0,     0,     0,     0,   237,   189,     0,
       0,     0,     0,     0,   272,     0,     0,   224,     0,     0,
       0,   272,     0,     0,   231,     0,     0,     0,     0,     0,
     272,     0,     0,   202,    82,     0,   340,   309,     0,   129,
     141,   145,   145,   138,   145,   160,   161,     0,     0,   247,
     163,   315,   320,   316,   322,   321,   310,   311,   313,   312,
     314,   318,   319,   285,     0,     0,   284,    38,     0,    57,
      82,     0,    53,    66,    31,   144,    28,   275,   274,    34,
      72,     0,     0,   335,   336,   337,   338,   155,     0,   133,
       0,     0,    82,   213,   272,     0,     0,   176,   196,   333,
     334,     0,     0,     0,   329,   330,   184,     0,   326,     0,
       0,    82,   222,   272,     0,   327,   328,     0,     0,    82,
     229,   272,     0,   331,   332,     0,     0,     0,     0,    82,
     198,   272,     0,     0,   339,     0,   272,     0,   147,   137,
     147,     0,     0,     0,     0,     0,    47,    35,   289,   288,
     286,     0,   283,   287,   297,    43,    52,     0,    60,     0,
      79,   140,   136,     0,    82,   219,     0,    82,   191,   178,
     177,     0,    82,     0,     0,     0,     0,     0,     0,     0,
      82,   226,     0,    82,     0,    82,   233,     0,    82,     0,
       0,     0,    82,   204,     0,    82,   195,     0,     0,   146,
       0,   127,   128,     0,     0,     0,     0,     0,     0,   308,
     297,   269,     0,     0,     0,   297,    40,    41,    42,   298,
     299,     0,     0,     0,    59,    82,     0,   276,   215,   218,
      82,   221,   181,   179,   182,   180,    82,   197,   194,     0,
     151,   152,   325,   223,   225,    82,   228,   230,   232,    82,
     235,   153,   154,   200,   203,    82,   206,     0,   142,   148,
       0,   214,     0,   199,     0,     0,     0,    50,    49,    51,
       0,     0,   293,   290,   291,   292,     0,   277,    39,   278,
       0,    36,     0,    54,   220,     0,     0,   227,   234,   205,
       0,     0,     0,   170,     0,     0,   168,    48,    46,   295,
       0,   294,    44,    61,   175,   150,     0,   216,   201,     0,
       0,     0,   173,     0,     0,     0,   167,   169,     0,   172,
       0,     0,     0,   174,     0,   131,   171,   296,     0,   130,
     272,     0,   132
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    61,    62,   545,   613,    26,    63,
     601,    27,    64,    65,   210,   354,   616,    28,    29,    30,
      31,    32,    33,    34,    50,    85,   100,   254,    53,    54,
      96,    97,    56,    57,   156,   157,   158,   159,   160,   719,
     161,   162,   479,   163,   164,   259,   431,   359,   528,   591,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   597,   683,   700,   177,   709,   178,   561,   626,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   563,   191,   422,   423,   192,   193,   194,   387,
     388,   195,   406,   407,   196,   413,   414,   197,   198,    52,
      87,    88,    89,   221,    91,    86,   294,   247,   222,   223,
     668,   201,   307,   457,   542,   543,   665,   691,   611,   224,
     346,   347
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -554
static const yytype_int16 yypact[] =
{
    -554,   644,  -554,  -554,   -38,   -76,   -62,   -82,   -60,     4,
      30,    14,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
     -43,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
     156,  -554,  -554,  -554,    41,   132,   146,  -554,  -554,   162,
      92,   189,   106,   116,  -554,   134,   131,   211,   159,   158,
     -22,    -4,   -18,   253,   238,   253,   246,   800,   206,   208,
     209,  -554,   252,  -554,  -554,   200,   246,  -554,   -13,   216,
     253,   217,  -554,   221,    -4,    -4,  -554,  -554,   222,   975,
     -12,  -554,  -554,   375,   218,  -554,  -554,    -4,   223,    15,
    -554,   -89,    -4,   212,   213,   227,  -554,   214,   219,  -554,
     253,   220,  -554,   975,  -554,   234,    16,   109,  -554,  -554,
     224,   272,   -27,   237,  -554,  -554,    -4,   239,    -4,   975,
      -4,   180,   225,   975,  -554,  -554,   975,   975,   -43,   243,
      -4,   181,  -554,    -4,   230,   -14,  -554,   244,   247,   183,
      -1,  -554,   190,   233,     1,   975,    -4,   254,   255,   236,
    -554,   975,   975,   975,   975,  -554,   241,   248,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,   197,
      56,  -554,  1222,  -554,   277,  -554,   246,   259,   246,  -554,
     278,   253,   306,   249,   316,   250,   257,   260,   245,   251,
    -554,  -554,   256,   271,  1045,   332,   267,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,   291,   295,  -554,    -4,  -554,  -554,  -554,    -4,  -554,
    -554,  -554,  -554,  -554,  -554,   282,  -554,  1222,   283,  -554,
     975,   975,   975,   975,   253,    -4,    -4,    -4,     3,   284,
    -554,  -554,   153,   373,  -554,  1127,  -554,   975,   975,   137,
    1222,  1222,  -554,   -18,   975,  -554,   975,   975,   317,    -4,
      -4,     8,  -554,  -554,  -554,   975,   975,    -4,    38,   975,
     975,    79,    -4,    51,  1154,  -554,  -554,  -554,  -554,   975,
     290,   290,   279,   285,  -554,  -554,    22,  -554,  -554,   310,
     311,   310,   310,  -554,  -554,   975,   975,   904,   975,  -554,
    -554,   975,   975,   975,  -554,  -554,  -554,   975,  -554,  -554,
    -554,   975,   975,   975,   975,   975,   975,   975,   312,   253,
     207,   253,   296,   298,   -61,   300,   301,  -554,   324,   313,
    -554,  -554,  -554,  -554,  -554,  -554,   975,   975,   314,  -554,
     315,  -554,  -554,   253,  -554,  -554,   975,   308,   321,   318,
     322,  -554,   319,   338,   320,   975,   328,   253,    69,    -4,
     975,  -554,   330,   329,   331,   335,   348,  -554,  1222,   336,
     337,    -4,   -81,   339,   975,   345,   253,   111,   349,   344,
     350,   975,   351,   253,   126,   353,   347,   368,   369,   356,
     975,   358,   253,   130,  -554,   996,  -554,  -554,    -4,  -554,
     361,   383,   383,  -554,   383,  1222,  1222,    54,    70,   367,
    -554,   662,   662,   662,   152,   152,    61,    61,   290,   290,
     290,   184,   500,   207,   371,   154,  -554,   480,   374,  -554,
    -554,   389,  -554,  -554,  -554,  -554,  -554,  1196,  1222,  -554,
    -554,   385,   386,  -554,  -554,  -554,  -554,  -554,    -4,  -554,
       3,   372,  -554,  -554,   975,   390,   392,   388,   491,  -554,
    -554,   409,    -4,    -4,  -554,  -554,  -554,   100,  -554,     8,
     393,  -554,  -554,   975,   395,  -554,  -554,    38,   396,  -554,
    -554,   975,   397,  -554,  -554,    -4,    -4,    51,   398,  -554,
    -554,   975,   400,   253,  -554,   406,   975,   426,   430,  -554,
     430,    -4,     3,    -4,    51,   975,   440,  -554,  -554,  -554,
    -554,   -48,  -554,   174,    59,   489,  -554,   412,   415,   975,
    -554,  -554,  -554,   253,  -554,  -554,   417,  -554,  -554,  -554,
    -554,    10,  -554,   253,   433,   416,   421,   255,   422,   253,
    -554,  -554,   424,  -554,   253,  -554,  -554,   425,  -554,   423,
     429,   253,  -554,  -554,   431,  -554,  -554,   466,   434,  -554,
     975,  -554,  -554,   437,   253,   439,   253,   435,   199,   -19,
      91,   446,    99,   -48,   -48,   139,  -554,  -554,  -554,  -554,
    -554,   170,   555,   442,  -554,  -554,   444,  1222,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,    -4,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,    -4,  -554,  1222,
       3,  -554,    51,  -554,    -4,   462,   530,  -554,  -554,  -554,
     170,   447,  -554,   450,   450,  -554,   -97,   255,  -554,  -554,
      -4,  -554,   448,  -554,  -554,   253,   449,  -554,  -554,  -554,
     531,   253,   253,   453,   454,   456,  -554,  -554,  -554,  -554,
     476,  -554,  -554,  -554,  -554,  -554,   478,  -554,  -554,    -4,
     461,   525,   550,   471,   532,   467,  -554,  -554,    -4,  -554,
     486,   487,   582,  -554,   472,   492,  -554,  -554,   477,  -554,
     975,   479,  -554
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,   558,  -554,  -554,   -58,  -554,   -23,  -554,
    -554,  -554,   113,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,   -26,  -554,  -425,    87,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,   502,  -554,
     293,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,  -554,
    -554,  -554,  -554,  -554,  -512,  -554,  -554,  -554,  -554,  -478,
    -554,  -554,   122,  -554,  -554,   117,  -554,  -554,  -554,   495,
     543,     9,  -554,   -51,   172,   -42,  -554,    35,   -52,  -554,
     -17,  -494,  -554,   188,  -554,  -433,  -554,  -554,  -553,   438,
    -554,  -554
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -244
static const yytype_int16 yytable[] =
{
      90,    55,   553,   568,    42,   581,   200,   529,   211,   530,
      93,    94,  -243,    98,    83,   212,   225,    42,    83,   461,
     385,   689,   596,    90,   219,   404,    35,    42,   215,   622,
      42,    36,    42,   246,   623,    42,    90,    39,   245,   690,
      40,    90,   497,    37,   624,    42,    42,   660,   625,    43,
      78,    79,   666,    42,   594,   411,   498,  -243,   255,    41,
    -243,    90,    43,  -243,  -243,   272,   199,    90,   420,    90,
     538,   539,    43,    38,   462,    43,   386,    43,    51,    90,
      43,   405,   288,   218,    90,    42,   484,  -243,   541,    90,
      43,    43,    44,    90,    47,   305,   242,    84,    43,   606,
    -243,    42,   313,    92,  -243,    44,   213,   226,   602,   267,
      42,   412,  -243,  -243,  -243,    44,  -243,   669,    44,  -243,
      44,   268,   290,    44,   421,   271,    45,   274,   503,   276,
      43,   657,  -243,    44,    44,   297,    48,   302,   244,   285,
     682,    44,   485,   511,   291,    66,    43,   521,   349,   298,
     351,    70,   260,   303,   607,    43,   261,   328,   428,  -243,
    -243,  -243,   325,   326,   329,   330,   669,    58,    42,   355,
     663,   664,   681,    44,   206,  -243,    67,   208,    59,  -243,
     608,    46,    68,    49,   504,   394,   658,   609,   610,    44,
     531,   343,   344,   372,   327,   345,    69,    90,    44,   512,
     417,   316,   331,   522,   317,   318,   533,    71,   377,   378,
     379,   380,   659,    43,    90,    90,    90,   147,   567,   609,
     610,   264,   381,    60,   332,   392,   393,    72,   396,   603,
     604,   374,    73,   605,   399,   400,   662,   455,   402,    90,
     456,   397,    74,   408,   409,   262,    90,   415,   416,   263,
     418,    90,    75,   319,   320,   321,    44,   242,   395,   325,
     326,    80,   334,   335,   336,   429,    77,   609,   610,    76,
      78,    79,   538,   539,   382,   383,   384,   338,   339,   340,
     341,   342,   343,   344,   328,   540,   345,   147,   667,    83,
     541,   329,   330,    82,    81,   433,   434,    78,    79,   403,
     322,   323,   324,    95,   603,   604,   410,   454,   605,   458,
      99,   419,   341,   342,   343,   344,   277,   286,   345,   295,
     278,   287,   352,   296,   472,   203,   299,   204,   205,   331,
     300,   471,   207,   481,   655,   214,   209,   656,    90,   216,
     217,   220,   266,   243,   241,   483,   251,   249,   250,   252,
     496,   332,   500,   258,   253,   256,   269,   348,   273,   508,
     265,   279,   284,   292,   502,   373,   289,   293,   518,   301,
     333,   510,   309,   356,   306,   308,   314,    90,   350,   353,
     520,   358,   363,   315,   357,   360,    90,    90,   364,   334,
     335,   336,   361,   328,   366,   362,   227,   365,   486,   368,
     329,   330,   369,   337,   338,   339,   340,   341,   342,   343,
     344,   370,   228,   345,   559,   371,   426,   375,   229,   376,
     389,   230,   231,   390,   345,   401,   427,   552,   232,   430,
     432,   453,   556,   459,   460,   463,   464,   525,   331,   233,
     560,    90,    90,   465,   234,   473,   532,   534,   466,   469,
     470,   572,   478,   527,   491,   475,   477,   480,   492,   577,
     332,   235,   474,   476,    90,    90,   482,   489,   236,   584,
     490,   493,   237,   494,   588,   238,   499,   239,   495,   333,
      90,   586,    90,   501,   599,   506,   505,   507,   514,   509,
     513,   515,   516,   517,   240,   202,   519,   526,   334,   335,
     336,   565,   566,   535,   488,   328,   537,   544,   548,   546,
     554,   618,   337,   338,   339,   340,   341,   342,   343,   344,
     550,   628,   345,   551,   579,   580,   562,   633,   557,   558,
     564,   570,   637,   573,   575,   578,   582,   523,   585,   643,
     593,   257,   595,   587,   598,   589,   590,   600,   612,   614,
     331,   615,   651,   630,   653,   620,   629,   275,   631,   632,
     641,   280,   635,   639,   281,   282,   642,   647,   246,   645,
     654,   648,   332,   547,   650,   670,   652,   671,    90,   673,
     685,   686,   688,   304,   605,   693,   695,   696,   699,   310,
     311,   312,   701,   702,   703,   555,    90,   704,   706,   707,
     708,   710,   711,    90,   714,   712,   715,   716,   718,   717,
     334,   335,   336,   720,   571,   155,   722,   592,   270,    90,
     440,   569,   576,   283,   574,   338,   339,   340,   341,   342,
     343,   344,   583,   694,   345,   248,   661,     0,   676,   697,
     698,   536,     0,   687,     2,     3,     0,     4,   705,     0,
       0,     0,     0,     0,     0,   -64,   680,   713,     0,     0,
       0,     0,     0,   684,     0,     0,   -64,   619,   721,     0,
     621,     0,     5,     0,     0,   627,     0,     0,     0,   692,
       0,     0,     0,   634,     0,     0,   636,     0,   638,     0,
       6,   640,     0,     7,     0,   644,     0,     0,   646,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   -64,     0,     0,     0,     0,     0,     0,     0,     8,
       0,     0,   398,     9,     0,    10,    11,     0,   672,     0,
       0,     0,     0,   674,     0,     0,     0,     0,     0,   675,
       0,    12,    13,     0,     0,     0,     0,   425,   677,     0,
       0,     0,   678,     0,     0,     0,     0,     0,   679,     0,
       0,     0,     0,   435,   436,   202,   441,     0,     0,   442,
     443,   444,   334,   335,   336,   445,     0,     0,     0,   446,
     447,   448,   449,   450,   451,   452,     0,   338,   339,   340,
     341,   342,   343,   344,     0,     0,   345,     0,     0,     0,
       0,   101,     0,     0,   467,   468,   102,     0,     0,     0,
     103,     0,     0,     0,   104,   105,   106,   -81,   107,     0,
       0,   108,     0,   109,     0,     0,     0,     0,   487,   110,
       0,    42,   111,     0,     0,   -81,   -81,   112,     0,   113,
     114,   115,   116,   117,   118,     0,     0,   119,     0,     0,
       0,     0,     0,     0,   120,   121,     0,     0,     0,     0,
     122,   123,   124,   125,   -81,     0,   126,     0,   127,   128,
       0,   129,     0,   -81,     0,     0,    43,   130,   131,     0,
       0,     0,     0,   132,   133,   134,   135,     0,   136,   137,
     138,     0,   139,   140,     0,   141,   142,    12,     0,   143,
     144,     0,   145,     0,     0,     0,     0,     0,     0,   146,
     102,     0,     0,     0,     0,     0,     0,   147,   148,   149,
     106,   150,   107,     0,     0,     0,     0,     0,   151,   152,
       0,     0,     0,     0,     0,    42,   153,   -81,     0,     0,
     154,   437,     0,     0,   114,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   121,
       0,     0,     0,     0,     0,   123,   124,   125,     0,     0,
     126,     0,   127,   425,     0,     0,     0,     0,     0,     0,
      43,   102,   131,     0,     0,     0,     0,   617,     0,   134,
       0,   106,     0,   107,     0,     0,   139,     0,     0,   141,
     142,   328,     0,     0,   438,     0,    42,     0,   329,   330,
       0,     0,     0,     0,     0,   114,     0,     0,     0,     0,
       0,   147,   148,   439,     0,   150,     0,     0,   649,     0,
     121,     0,   151,   152,     0,     0,     0,   124,   125,     0,
     153,     0,     0,   127,   154,     0,   331,     0,     0,     0,
     328,    43,     0,   131,     0,     0,     0,   329,   330,     0,
     134,     0,     0,     0,     0,     0,     0,   139,   332,     0,
     141,   142,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   333,     0,     0,
       0,     0,   147,   148,   149,   331,   150,     0,     0,     0,
       0,     0,     0,   151,   152,     0,   334,   335,   336,     0,
       0,   153,     0,     0,     0,   154,     0,   332,     0,     0,
     337,   338,   339,   340,   341,   342,   343,   344,     0,     0,
     345,     0,   328,   524,     0,     0,   333,     0,     0,   329,
     330,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   334,   335,   336,   367,   328,
       0,     0,     0,     0,     0,     0,   329,   330,     0,   337,
     338,   339,   340,   341,   342,   343,   344,   331,     0,   345,
       0,     0,     0,     0,     0,     0,     0,     0,   424,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   332,
       0,   328,     0,     0,   331,     0,     0,     0,   329,   330,
       0,     0,     0,     0,     0,     0,     0,     0,   333,     0,
       0,   391,     0,     0,     0,     0,   332,   328,     0,     0,
       0,     0,     0,     0,   329,   330,     0,   334,   335,   336,
       0,     0,     0,     0,     0,   333,   331,     0,     0,     0,
       0,   337,   338,   339,   340,   341,   342,   343,   344,     0,
       0,   345,     0,     0,   334,   335,   336,     0,   332,     0,
       0,     0,   331,     0,     0,     0,     0,     0,   337,   338,
     339,   340,   341,   342,   343,   344,     0,   333,   345,     0,
       0,     0,     0,     0,   332,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   334,   335,   336,   549,
       0,     0,     0,   333,     0,     0,     0,     0,     0,     0,
     337,   338,   339,   340,   341,   342,   343,   344,     0,     0,
     345,     0,   334,   335,   336,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   337,   338,   339,   340,
     341,   342,   343,   344,     0,     0,   345
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-554)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      51,    24,   480,   497,    31,   517,    57,   432,    66,   434,
      52,    53,    31,    55,    36,    28,    28,    31,    36,    80,
      17,   118,   534,    74,    75,    17,    64,    31,    70,    19,
      31,    69,    31,   122,    24,    31,    87,   119,    89,   136,
     122,    92,   123,   119,    34,    31,    31,   600,    38,    76,
     139,   140,   605,    31,   532,    17,   137,    76,   100,   119,
       4,   112,    76,     7,     8,   116,    57,   118,    17,   120,
     118,   119,    76,   135,   135,    76,    73,    76,   121,   130,
      76,    73,   133,    74,   135,    31,    17,    31,   136,   140,
      76,    76,   119,   144,    64,   146,    87,   119,    76,    40,
     119,    31,   154,   121,   123,   119,   119,   119,   541,   136,
      31,    73,    56,    57,    58,   119,   135,   611,   119,   138,
     119,   112,   136,   119,    73,   116,   122,   118,    17,   120,
      76,    40,    76,   119,   119,   136,   122,   136,   123,   130,
     652,   119,    73,    17,   135,    32,    76,    17,   206,   140,
     208,    38,   136,   144,    95,    76,   140,     5,   136,   103,
     104,   105,   106,   107,    12,    13,   660,    11,    31,   211,
     603,   604,   650,   119,    61,   119,   135,    64,    22,   123,
     121,     9,    50,    11,    73,    48,    95,   128,   129,   119,
     136,   130,   131,   244,   138,   134,    50,   248,   119,    73,
     121,     4,    50,    73,     7,     8,   136,    45,   260,   261,
     262,   263,   121,    76,   265,   266,   267,   117,   118,   128,
     129,   108,   264,    67,    72,   277,   278,   135,   279,   130,
     131,   254,    43,   134,   286,   287,   137,    30,   289,   290,
      33,   283,   136,   295,   296,   136,   297,   299,   300,   140,
     301,   302,   136,    56,    57,    58,   119,   248,   121,   106,
     107,    50,   110,   111,   112,   316,   135,   128,   129,   135,
     139,   140,   118,   119,   265,   266,   267,   125,   126,   127,
     128,   129,   130,   131,     5,   131,   134,   117,   118,    36,
     136,    12,    13,   135,   135,   321,   322,   139,   140,   290,
     103,   104,   105,    65,   130,   131,   297,   349,   134,   351,
      64,   302,   128,   129,   130,   131,   136,   136,   134,   136,
     140,   140,   209,   140,   376,   119,   136,   119,   119,    50,
     140,   373,    80,   385,   135,   119,   136,   138,   389,   122,
     119,   119,    70,   120,   126,   387,   119,   135,   135,   135,
     401,    72,   404,   119,   135,   135,   119,    80,   119,   411,
     136,   136,   119,   119,   406,   252,   136,   120,   420,   136,
      91,   413,   136,    67,   120,   120,   135,   428,   119,   101,
     422,    65,   137,   135,   135,   135,   437,   438,   137,   110,
     111,   112,   135,     5,   123,   135,    21,   141,   389,    67,
      12,    13,   135,   124,   125,   126,   127,   128,   129,   130,
     131,   120,    37,   134,    26,   120,   137,   135,    43,   136,
     136,    46,    47,    50,   134,   108,   141,   478,    53,   119,
     119,   119,   484,   137,   136,   135,   135,   428,    50,    64,
      52,   492,   493,   119,    69,   137,   437,   438,   135,   135,
     135,   503,   114,    70,   123,   137,   137,   137,   123,   511,
      72,    86,   141,   141,   515,   516,   138,   137,    93,   521,
     141,   123,    97,   137,   526,   100,   137,   102,   141,    91,
     531,   523,   533,   138,   535,   141,   137,   137,   141,   138,
     137,   123,   123,   137,   119,    57,   138,   136,   110,   111,
     112,   492,   493,   136,   391,     5,   135,    27,   119,   135,
     138,   553,   124,   125,   126,   127,   128,   129,   130,   131,
     135,   563,   134,   137,   515,   516,    35,   569,   138,   137,
     121,   138,   574,   138,   138,   138,   138,   424,   138,   581,
     531,   103,   533,   137,   535,   119,   116,   107,    59,   137,
      50,   136,   594,   137,   596,   138,   123,   119,   137,   137,
     137,   123,   138,   138,   126,   127,   137,   101,   122,   138,
     135,   137,    72,   460,   137,    20,   137,   135,   629,   135,
     118,    51,   135,   145,   134,   137,   137,    56,   135,   151,
     152,   153,   138,   137,   118,   482,   647,   119,   137,    74,
      50,   130,    70,   654,   118,   138,   119,    25,   116,   137,
     110,   111,   112,   136,   501,    57,   137,   530,   116,   670,
     327,   499,   509,   128,   507,   125,   126,   127,   128,   129,
     130,   131,   519,   675,   134,    92,   601,    -1,   629,   681,
     682,   453,    -1,   660,     0,     1,    -1,     3,   699,    -1,
      -1,    -1,    -1,    -1,    -1,    11,   647,   708,    -1,    -1,
      -1,    -1,    -1,   654,    -1,    -1,    22,   554,   720,    -1,
     557,    -1,    28,    -1,    -1,   562,    -1,    -1,    -1,   670,
      -1,    -1,    -1,   570,    -1,    -1,   573,    -1,   575,    -1,
      46,   578,    -1,    49,    -1,   582,    -1,    -1,   585,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    75,
      -1,    -1,   284,    79,    -1,    81,    82,    -1,   615,    -1,
      -1,    -1,    -1,   620,    -1,    -1,    -1,    -1,    -1,   626,
      -1,    97,    98,    -1,    -1,    -1,    -1,   309,   635,    -1,
      -1,    -1,   639,    -1,    -1,    -1,    -1,    -1,   645,    -1,
      -1,    -1,    -1,   325,   326,   327,   328,    -1,    -1,   331,
     332,   333,   110,   111,   112,   337,    -1,    -1,    -1,   341,
     342,   343,   344,   345,   346,   347,    -1,   125,   126,   127,
     128,   129,   130,   131,    -1,    -1,   134,    -1,    -1,    -1,
      -1,     1,    -1,    -1,   366,   367,     6,    -1,    -1,    -1,
      10,    -1,    -1,    -1,    14,    15,    16,    17,    18,    -1,
      -1,    21,    -1,    23,    -1,    -1,    -1,    -1,   390,    29,
      -1,    31,    32,    -1,    -1,    35,    36,    37,    -1,    39,
      40,    41,    42,    43,    44,    -1,    -1,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    54,    55,    -1,    -1,    -1,    -1,
      60,    61,    62,    63,    64,    -1,    66,    -1,    68,    69,
      -1,    71,    -1,    73,    -1,    -1,    76,    77,    78,    -1,
      -1,    -1,    -1,    83,    84,    85,    86,    -1,    88,    89,
      90,    -1,    92,    93,    -1,    95,    96,    97,    -1,    99,
     100,    -1,   102,    -1,    -1,    -1,    -1,    -1,    -1,   109,
       6,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,   119,
      16,   121,    18,    -1,    -1,    -1,    -1,    -1,   128,   129,
      -1,    -1,    -1,    -1,    -1,    31,   136,   137,    -1,    -1,
     140,    37,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    61,    62,    63,    -1,    -1,
      66,    -1,    68,   535,    -1,    -1,    -1,    -1,    -1,    -1,
      76,     6,    78,    -1,    -1,    -1,    -1,   549,    -1,    85,
      -1,    16,    -1,    18,    -1,    -1,    92,    -1,    -1,    95,
      96,     5,    -1,    -1,   100,    -1,    31,    -1,    12,    13,
      -1,    -1,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,
      -1,   117,   118,   119,    -1,   121,    -1,    -1,   590,    -1,
      55,    -1,   128,   129,    -1,    -1,    -1,    62,    63,    -1,
     136,    -1,    -1,    68,   140,    -1,    50,    -1,    -1,    -1,
       5,    76,    -1,    78,    -1,    -1,    -1,    12,    13,    -1,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    92,    72,    -1,
      95,    96,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,
      -1,    -1,   117,   118,   119,    50,   121,    -1,    -1,    -1,
      -1,    -1,    -1,   128,   129,    -1,   110,   111,   112,    -1,
      -1,   136,    -1,    -1,    -1,   140,    -1,    72,    -1,    -1,
     124,   125,   126,   127,   128,   129,   130,   131,    -1,    -1,
     134,    -1,     5,   137,    -1,    -1,    91,    -1,    -1,    12,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   110,   111,   112,   113,     5,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    -1,   124,
     125,   126,   127,   128,   129,   130,   131,    50,    -1,   134,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    72,
      -1,     5,    -1,    -1,    50,    -1,    -1,    -1,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    91,    -1,
      -1,    94,    -1,    -1,    -1,    -1,    72,     5,    -1,    -1,
      -1,    -1,    -1,    -1,    12,    13,    -1,   110,   111,   112,
      -1,    -1,    -1,    -1,    -1,    91,    50,    -1,    -1,    -1,
      -1,   124,   125,   126,   127,   128,   129,   130,   131,    -1,
      -1,   134,    -1,    -1,   110,   111,   112,    -1,    72,    -1,
      -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,   124,   125,
     126,   127,   128,   129,   130,   131,    -1,    91,   134,    -1,
      -1,    -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   110,   111,   112,   113,
      -1,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,    -1,
     124,   125,   126,   127,   128,   129,   130,   131,    -1,    -1,
     134,    -1,   110,   111,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   124,   125,   126,   127,
     128,   129,   130,   131,    -1,    -1,   134
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   143,     0,     1,     3,    28,    46,    49,    75,    79,
      81,    82,    97,    98,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   160,   163,   169,   170,
     171,   172,   173,   174,   175,    64,    69,   119,   135,   119,
     122,   119,    31,    76,   119,   122,   256,    64,   122,   256,
     176,   121,   251,   180,   181,   180,   184,   185,    11,    22,
      67,   156,   157,   161,   164,   165,   184,   135,    50,    50,
     184,    45,   135,    43,   136,   136,   135,   135,   139,   140,
      50,   135,   135,    36,   119,   177,   257,   252,   253,   254,
     255,   256,   121,   257,   257,    65,   182,   183,   257,    64,
     178,     1,     6,    10,    14,    15,    16,    18,    21,    23,
      29,    32,    37,    39,    40,    41,    42,    43,    44,    47,
      54,    55,    60,    61,    62,    63,    66,    68,    69,    71,
      77,    78,    83,    84,    85,    86,    88,    89,    90,    92,
      93,    95,    96,    99,   100,   102,   109,   117,   118,   119,
     121,   128,   129,   136,   140,   175,   186,   187,   188,   189,
     190,   192,   193,   195,   196,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   217,   219,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   235,   238,   239,   240,   243,   246,   249,   250,   253,
     255,   263,   271,   119,   119,   119,   184,    80,   184,   136,
     166,   178,    28,   119,   119,   257,   122,   119,   253,   255,
     119,   255,   260,   261,   271,    28,   119,    21,    37,    43,
      46,    47,    53,    64,    69,    86,    93,    97,   100,   102,
     119,   126,   253,   120,   123,   255,   122,   259,   252,   135,
     135,   119,   135,   135,   179,   257,   135,   271,   119,   197,
     136,   140,   136,   140,   184,   136,    70,   136,   253,   119,
     210,   253,   255,   119,   253,   271,   253,   136,   140,   136,
     271,   271,   271,   251,   119,   253,   136,   140,   255,   136,
     136,   253,   119,   120,   258,   136,   140,   136,   253,   136,
     140,   136,   136,   253,   271,   255,   120,   264,   120,   136,
     271,   271,   271,   260,   135,   135,     4,     7,     8,    56,
      57,    58,   103,   104,   105,   106,   107,   138,     5,    12,
      13,    50,    72,    91,   110,   111,   112,   124,   125,   126,
     127,   128,   129,   130,   131,   134,   272,   273,    80,   178,
     119,   178,   184,   101,   167,   257,    67,   135,    65,   199,
     135,   135,   135,   137,   137,   141,   123,   113,    67,   135,
     120,   120,   255,   184,   180,   135,   136,   260,   260,   260,
     260,   257,   253,   253,   253,    17,    73,   241,   242,   136,
      50,    94,   260,   260,    48,   121,   255,   257,   271,   260,
     260,   108,   255,   253,    17,    73,   244,   245,   260,   260,
     253,    17,    73,   247,   248,   260,   260,   121,   255,   253,
      17,    73,   236,   237,    34,   271,   137,   141,   136,   255,
     119,   198,   119,   198,   198,   271,   271,    37,   100,   119,
     212,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   119,   257,    30,    33,   265,   257,   137,
     136,    80,   135,   135,   135,   119,   135,   271,   271,   135,
     135,   257,   260,   137,   141,   137,   141,   137,   114,   194,
     137,   260,   138,   257,    17,    73,   253,   271,   184,   137,
     141,   123,   123,   123,   137,   141,   255,   123,   137,   137,
     260,   138,   257,    17,    73,   137,   141,   137,   260,   138,
     257,    17,    73,   137,   141,   123,   123,   137,   260,   138,
     257,    17,    73,   184,   137,   253,   136,    70,   200,   200,
     200,   136,   253,   136,   253,   136,   265,   135,   118,   119,
     131,   136,   266,   267,    27,   158,   135,   184,   119,   113,
     135,   137,   255,   241,   138,   184,   260,   138,   137,    26,
      52,   220,    35,   234,   121,   253,   253,   118,   263,   244,
     138,   184,   260,   138,   247,   138,   184,   260,   138,   253,
     253,   236,   138,   184,   260,   138,   257,   137,   260,   119,
     116,   201,   201,   253,   241,   253,   236,   214,   253,   255,
     107,   162,   267,   130,   131,   134,    40,    95,   121,   128,
     129,   270,    59,   159,   137,   136,   168,   271,   257,   184,
     138,   184,    19,    24,    34,    38,   221,   184,   257,   123,
     137,   137,   137,   257,   184,   138,   184,   257,   184,   138,
     184,   137,   137,   257,   184,   138,   184,   101,   137,   271,
     137,   257,   137,   257,   135,   135,   138,    40,    95,   121,
     270,   259,   137,   267,   267,   268,   270,   118,   262,   263,
      20,   135,   184,   135,   184,   184,   253,   184,   184,   184,
     253,   241,   236,   215,   253,   118,    51,   262,   135,   118,
     136,   269,   253,   137,   257,   137,    56,   257,   257,   135,
     216,   138,   137,   118,   119,   255,   137,    74,    50,   218,
     130,    70,   138,   255,   118,   119,    25,   137,   116,   191,
     136,   260,   137
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
        break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}




/* The lookahead symbol.  */
int yychar;


#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 17:
/* Line 1792 of yacc.c  */
#line 449 "ascend/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in definition.");
	}
    break;

  case 18:
/* Line 1792 of yacc.c  */
#line 456 "ascend/compiler/ascParse.y"
    {
      /* the following steps apply to string buffers only, not files */
      struct gl_list_t *stats;
      int dispose;
      if ((yyvsp[(3) - (5)].slptr) != NULL) {
        stats = gl_create(1L);
        gl_append_ptr(stats,(void *)(yyvsp[(3) - (5)].slptr));
        if (g_untrapped_error) {
          ErrMsg_Generic("Because of a syntax error, the following statements are being ignored:");
          WriteStatementList(ASCERR,(yyvsp[(3) - (5)].slptr),4);
          DestroyStatementList((yyvsp[(3) - (5)].slptr));
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
                WriteStatementList(ASCERR,(yyvsp[(3) - (5)].slptr),4);
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
    break;

  case 19:
/* Line 1792 of yacc.c  */
#line 501 "ascend/compiler/ascParse.y"
    {
	  Asc_ScannerPushBuffer((yyvsp[(2) - (3)].dquote_ptr));
	}
    break;

  case 20:
/* Line 1792 of yacc.c  */
#line 505 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 21:
/* Line 1792 of yacc.c  */
#line 510 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 22:
/* Line 1792 of yacc.c  */
#line 518 "ascend/compiler/ascParse.y"
    {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[(2) - (3)].dquote_ptr));
	}
    break;

  case 23:
/* Line 1792 of yacc.c  */
#line 522 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 24:
/* Line 1792 of yacc.c  */
#line 527 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 25:
/* Line 1792 of yacc.c  */
#line 535 "ascend/compiler/ascParse.y"
    {
	  if(package_load((yyvsp[(4) - (5)].dquote_ptr),SCP((yyvsp[(2) - (5)].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[(4) - (5)].dquote_ptr)), SCP((yyvsp[(2) - (5)].id_ptr))
	    );
      }
	}
    break;

  case 26:
/* Line 1792 of yacc.c  */
#line 544 "ascend/compiler/ascParse.y"
    {
	  if(package_load(SCP((yyvsp[(2) - (3)].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[(2) - (3)].dquote_ptr))
	    );
	  }
	}
    break;

  case 27:
/* Line 1792 of yacc.c  */
#line 556 "ascend/compiler/ascParse.y"
    {
	  /*  see comments for notes statement.  */
	  if( (yyvsp[(3) - (4)].int_value) != NOTES_TOK ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, (yyvsp[(3) - (4)].int_value), NULL);
	  }
	  if ((yyvsp[(2) - (4)].notesptr) != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = (yyvsp[(2) - (4)].notesptr);
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
	    DestroyNoteTmpList((yyvsp[(2) - (4)].notesptr));
      }
      g_type_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
    break;

  case 28:
/* Line 1792 of yacc.c  */
#line 587 "ascend/compiler/ascParse.y"
    {
	  g_type_name = (yyvsp[(4) - (6)].id_ptr);
	  g_proc_name = (yyvsp[(5) - (6)].id_ptr);
	}
    break;

  case 29:
/* Line 1792 of yacc.c  */
#line 595 "ascend/compiler/ascParse.y"
    {
	  if ((yyvsp[(1) - (4)].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[(2) - (4)].listp));
	  } else {
	    if( (yyvsp[(3) - (4)].int_value) != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("ADD METHODS", NULL, (yyvsp[(3) - (4)].int_value), "METHODS");
	    }
	    if (AddMethods((yyvsp[(1) - (4)].tptr),(yyvsp[(2) - (4)].listp),g_untrapped_error) != 0) {
	      if ((yyvsp[(1) - (4)].tptr) != ILLEGAL_DEFINITION) {
	            error_reporter_current_line(ASC_USER_ERROR
	        ,"ADD METHODS failed for type %s"
	        ,SCP(GetName((yyvsp[(1) - (4)].tptr)))
	      );
	      DestroyProcedureList((yyvsp[(2) - (4)].listp));
	      } /* else adding in DEFINITION MODEL may have misgone */
	    }
	  }
	  g_untrapped_error = 0;
	}
    break;

  case 30:
/* Line 1792 of yacc.c  */
#line 618 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[(4) - (5)].id_ptr));
	  if(tmptype == NULL){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"ADD METHODS called with undefined type (%s)"
	      ,SCP((yyvsp[(4) - (5)].id_ptr))
	    );
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	  g_type_name = (yyvsp[(4) - (5)].id_ptr); /* scope for notes */
	}
    break;

  case 31:
/* Line 1792 of yacc.c  */
#line 631 "ascend/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 32:
/* Line 1792 of yacc.c  */
#line 639 "ascend/compiler/ascParse.y"
    {
	  if ((yyvsp[(1) - (4)].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[(2) - (4)].listp));
	  } else {
	    if( (yyvsp[(3) - (4)].int_value) != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("REPLACE METHODS", NULL, (yyvsp[(3) - (4)].int_value), "METHODS");
	    }
	    if (ReplaceMethods((yyvsp[(1) - (4)].tptr),(yyvsp[(2) - (4)].listp),g_untrapped_error) != 0) {
	      error_reporter_current_line(ASC_USER_ERROR
	        ,"REPLACE METHODS failed for type %s"
	        ,SCP(GetName((yyvsp[(1) - (4)].tptr)))
	      );
	      DestroyProcedureList((yyvsp[(2) - (4)].listp));
	    }
	  }
	  g_untrapped_error = 0;
	}
    break;

  case 33:
/* Line 1792 of yacc.c  */
#line 660 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[(4) - (5)].id_ptr));
	  if (tmptype == NULL) {
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"REPLACE METHODS called with undefined type (%s)"
	      ,SCP((yyvsp[(4) - (5)].id_ptr))
	    );
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	}
    break;

  case 34:
/* Line 1792 of yacc.c  */
#line 672 "ascend/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 35:
/* Line 1792 of yacc.c  */
#line 680 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (yyvsp[(5) - (6)].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("ATOM", SCP(g_type_name),
	                        (yyvsp[(5) - (6)].int_value), SCP(g_type_name));
	  }
	  g_atom_dim_ptr = CheckDimensionsMatch(g_default_dim_ptr,
	                                        g_atom_dim_ptr);
	  if (g_atom_dim_ptr != NULL) {
	    def_ptr = CreateAtomTypeDef(g_type_name,
	                                g_refines_name,
	                                real_type, /* ignored..really */
	                                Asc_CurrentModule(),
	                                (yyvsp[(1) - (6)].int_value),
	                                (yyvsp[(3) - (6)].slptr),
	                                (yyvsp[(4) - (6)].listp),
	                                g_defaulted,
	                                g_default_double,
	                                g_atom_dim_ptr,
	                                g_default_long,
	                                g_default_symbol,
					g_like_children,
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
	    DestroyStatementList((yyvsp[(3) - (6)].slptr));
	    DestroyProcedureList((yyvsp[(4) - (6)].listp));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
    break;

  case 36:
/* Line 1792 of yacc.c  */
#line 733 "ascend/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[(3) - (7)].id_ptr);
	  g_atom_dim_ptr = (yyvsp[(4) - (7)].dimp);
	  g_default_double = (yyvsp[(5) - (7)].real_value);
	  g_like_children = (yyvsp[(6) - (7)].lptr);
	  g_header_linenum = LineNum();
	}
    break;

  case 37:
/* Line 1792 of yacc.c  */
#line 745 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 38:
/* Line 1792 of yacc.c  */
#line 753 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 39:
/* Line 1792 of yacc.c  */
#line 759 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 40:
/* Line 1792 of yacc.c  */
#line 764 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
    break;

  case 41:
/* Line 1792 of yacc.c  */
#line 771 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
    break;

  case 42:
/* Line 1792 of yacc.c  */
#line 778 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 0;
	}
    break;

  case 43:
/* Line 1792 of yacc.c  */
#line 788 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = NULL;
	}
    break;

  case 44:
/* Line 1792 of yacc.c  */
#line 792 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(3) - (3)].lptr);
	}
    break;

  case 45:
/* Line 1792 of yacc.c  */
#line 798 "ascend/compiler/ascParse.y"
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
	                                    (yyvsp[(1) - (2)].int_value),
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
    break;

  case 46:
/* Line 1792 of yacc.c  */
#line 838 "ascend/compiler/ascParse.y"
    {
	  g_type_name = (yyvsp[(2) - (8)].id_ptr);
	  g_refines_name = (yyvsp[(4) - (8)].id_ptr);
	  g_atom_dim_ptr = (yyvsp[(5) - (8)].dimp);
	  switch (g_constant_type) {
	  case DOUBLECONSTANT:
	    g_default_double = (yyvsp[(6) - (8)].real_value);
	    break;
	  case LONGCONSTANT:
	    g_default_long = (yyvsp[(6) - (8)].real_value);
	    break;
	  case BOOLEANCONSTANT:
	    g_default_long = (yyvsp[(6) - (8)].int_value);
	    break;
	  case SYMBOLCONSTANT:
	    g_default_symbol = (yyvsp[(6) - (8)].sym_ptr);
	    break;
	  default:
	    ErrMsg_Generic("Wierd constant type assign encountered.");
	    break; /* better not be reached. */
	  }
	  g_header_linenum = LineNum();
	  if ((yyvsp[(7) - (8)].dquote_ptr) != NULL) {
	    CollectNote(CreateNote(g_type_name,InlineNote(),SelfNote(),NULL,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar((yyvsp[(7) - (8)].dquote_ptr),InlineNote()),
	                           g_header_linenum,NULL,nd_empty));
	  }
	}
    break;

  case 47:
/* Line 1792 of yacc.c  */
#line 871 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 48:
/* Line 1792 of yacc.c  */
#line 877 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 49:
/* Line 1792 of yacc.c  */
#line 882 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 50:
/* Line 1792 of yacc.c  */
#line 889 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 51:
/* Line 1792 of yacc.c  */
#line 896 "ascend/compiler/ascParse.y"
    {
	  (yyval.sym_ptr) = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
    break;

  case 52:
/* Line 1792 of yacc.c  */
#line 906 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;
	  if(( (yyvsp[(5) - (6)].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("MODEL", SCP(g_type_name),
	                        (yyvsp[(5) - (6)].int_value), SCP(g_type_name));
	  }
	  def_ptr = CreateModelTypeDef(g_type_name,
	                               g_refines_name,
	                               Asc_CurrentModule(),
	                               (yyvsp[(1) - (6)].int_value),
	                               (yyvsp[(3) - (6)].slptr),
	                               (yyvsp[(4) - (6)].listp),
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
    break;

  case 53:
/* Line 1792 of yacc.c  */
#line 948 "ascend/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[(2) - (4)].slptr);
	  g_parameter_wheres = (yyvsp[(3) - (4)].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
    break;

  case 54:
/* Line 1792 of yacc.c  */
#line 957 "ascend/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[(2) - (7)].slptr);
	  g_parameter_wheres = (yyvsp[(3) - (7)].slptr);
	  g_refines_name = (yyvsp[(5) - (7)].id_ptr);
	  g_parameter_reduction = (yyvsp[(6) - (7)].slptr);
	  g_header_linenum = LineNum();
	}
    break;

  case 55:
/* Line 1792 of yacc.c  */
#line 969 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 56:
/* Line 1792 of yacc.c  */
#line 977 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 57:
/* Line 1792 of yacc.c  */
#line 981 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 58:
/* Line 1792 of yacc.c  */
#line 988 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 59:
/* Line 1792 of yacc.c  */
#line 992 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(3) - (4)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 60:
/* Line 1792 of yacc.c  */
#line 999 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 61:
/* Line 1792 of yacc.c  */
#line 1003 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 62:
/* Line 1792 of yacc.c  */
#line 1010 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  if (((yyvsp[(4) - (5)].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("PATCH", SCP(g_type_name),
	                        (yyvsp[(4) - (5)].int_value), SCP(g_type_name));
	  }
	  def_ptr = CreatePatchTypeDef(g_type_name,
	                               g_refines_name,
	                               NULL,
	                               Asc_CurrentModule(),
	                               (yyvsp[(2) - (5)].slptr),
	                               (yyvsp[(3) - (5)].listp),
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
    break;

  case 63:
/* Line 1792 of yacc.c  */
#line 1042 "ascend/compiler/ascParse.y"
    {
	  /*
	   * A patch definition looks just like a model def.
	   * with the original name <=> refine name.
	   */
	  g_type_name = (yyvsp[(2) - (5)].id_ptr);
	  g_refines_name = (yyvsp[(4) - (5)].id_ptr);
	  g_header_linenum = LineNum();
	}
    break;

  case 64:
/* Line 1792 of yacc.c  */
#line 1055 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 65:
/* Line 1792 of yacc.c  */
#line 1059 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 66:
/* Line 1792 of yacc.c  */
#line 1066 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (yyvsp[(4) - (5)].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != (yyvsp[(1) - (5)].id_ptr) )) {
	    WarnMsg_MismatchEnd("DEFINITION", SCP((yyvsp[(1) - (5)].id_ptr)), (yyvsp[(4) - (5)].int_value), SCP((yyvsp[(1) - (5)].id_ptr)));
	  }
	  if( (yyvsp[(1) - (5)].id_ptr) == GetBaseTypeName(relation_type)) {
	    def_ptr = CreateRelationTypeDef(Asc_CurrentModule(),(yyvsp[(1) - (5)].id_ptr),(yyvsp[(2) - (5)].slptr),(yyvsp[(3) - (5)].listp));
	  }
	  else if( (yyvsp[(1) - (5)].id_ptr) == GetBaseTypeName(logrel_type) ) {
	    def_ptr = CreateLogRelTypeDef(Asc_CurrentModule(),(yyvsp[(1) - (5)].id_ptr),(yyvsp[(2) - (5)].slptr),(yyvsp[(3) - (5)].listp));
	  }
	  else {
	    ErrMsg_Generic("Bad type passed to DEFINITION statement.");
	    def_ptr = NULL;
	  }
	  if ( def_ptr != NULL ) {
	    keepnotes = AddType(def_ptr);
	  } else {
	    ErrMsg_NullDefPointer(SCP((yyvsp[(1) - (5)].id_ptr)));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = NULL;
	  g_untrapped_error = 0;
	}
    break;

  case 67:
/* Line 1792 of yacc.c  */
#line 1096 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 68:
/* Line 1792 of yacc.c  */
#line 1105 "ascend/compiler/ascParse.y"
    { /* nothing to do. just cruft to fix ; problem */ }
    break;

  case 69:
/* Line 1792 of yacc.c  */
#line 1110 "ascend/compiler/ascParse.y"
    {
	  struct UnitDefinition *ud;
	  unsigned long c,len;

	  if( (yyvsp[(3) - (3)].int_value) != UNITS_TOK ) {
	    WarnMsg_MismatchEnd("UNITS", NULL, (yyvsp[(3) - (3)].int_value), NULL);
	  }
	  len = gl_length((yyvsp[(2) - (3)].listp));
	  for (c=1; c <= len; c++) {
	    ud = (struct UnitDefinition *)gl_fetch((yyvsp[(2) - (3)].listp),c);
	    ProcessUnitDef(ud);
	    DestroyUnitDef(ud);
	  }
	  gl_destroy((yyvsp[(2) - (3)].listp));
	  (yyval.statptr) = NULL;
	}
    break;

  case 70:
/* Line 1792 of yacc.c  */
#line 1129 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(100L);
	}
    break;

  case 71:
/* Line 1792 of yacc.c  */
#line 1133 "ascend/compiler/ascParse.y"
    {
	  gl_append_ptr((yyvsp[(1) - (2)].listp),(char *)(yyvsp[(2) - (2)].udefptr));
	  (yyval.listp) = (yyvsp[(1) - (2)].listp);
	}
    break;

  case 72:
/* Line 1792 of yacc.c  */
#line 1141 "ascend/compiler/ascParse.y"
    {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[(1) - (4)].id_ptr),(yyvsp[(3) - (4)].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
    break;

  case 73:
/* Line 1792 of yacc.c  */
#line 1150 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = NULL;
	}
    break;

  case 74:
/* Line 1792 of yacc.c  */
#line 1154 "ascend/compiler/ascParse.y"
    { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
    break;

  case 75:
/* Line 1792 of yacc.c  */
#line 1161 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(3) - (3)].listp);
	}
    break;

  case 76:
/* Line 1792 of yacc.c  */
#line 1168 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(1) - (1)].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
    break;

  case 77:
/* Line 1792 of yacc.c  */
#line 1175 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 78:
/* Line 1792 of yacc.c  */
#line 1179 "ascend/compiler/ascParse.y"
    {
	  unsigned long c;
	  struct InitProcedure *oldproc;
	  c = gl_length((yyvsp[(1) - (2)].listp));
	  while (c > 0) {
	    oldproc = (struct InitProcedure *)gl_fetch((yyvsp[(1) - (2)].listp),c);
	    if (ProcName((yyvsp[(2) - (2)].procptr)) == ProcName(oldproc)) {
	      error_reporter_current_line(ASC_USER_WARNING
	        ,"Duplicate METHOD %s rejected", SCP(ProcName((yyvsp[(2) - (2)].procptr)))
	      );
	      break;
	    }
	    c--;
	  }
	  if (c) { /* broke early */
	    DestroyProcedure((yyvsp[(2) - (2)].procptr));
	  } else {
	    gl_append_ptr((yyvsp[(1) - (2)].listp),(char *)(yyvsp[(2) - (2)].procptr));
	  }
	  (yyval.listp) = (yyvsp[(1) - (2)].listp);
	}
    break;

  case 79:
/* Line 1792 of yacc.c  */
#line 1204 "ascend/compiler/ascParse.y"
    {
	  if (((yyvsp[(4) - (5)].int_value) != IDENTIFIER_TOK) || ((yyvsp[(1) - (5)].id_ptr) != g_end_identifier)) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("METHOD", SCP((yyvsp[(1) - (5)].id_ptr)), (yyvsp[(4) - (5)].int_value), SCP((yyvsp[(1) - (5)].id_ptr)));
	  }
	  (yyval.procptr) = CreateProcedure((yyvsp[(1) - (5)].id_ptr),(yyvsp[(3) - (5)].slptr));
	  g_proc_name = NULL;
	}
    break;

  case 80:
/* Line 1792 of yacc.c  */
#line 1218 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_proc_name = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 81:
/* Line 1792 of yacc.c  */
#line 1227 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = CreateStatementList((yyvsp[(1) - (1)].listp));
	}
    break;

  case 82:
/* Line 1792 of yacc.c  */
#line 1234 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 83:
/* Line 1792 of yacc.c  */
#line 1238 "ascend/compiler/ascParse.y"
    {
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ((yyvsp[(2) - (3)].statptr) != NULL) {
	    gl_append_ptr((yyvsp[(1) - (3)].listp),(char *)(yyvsp[(2) - (3)].statptr));
	  }
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 84:
/* Line 1792 of yacc.c  */
#line 1246 "ascend/compiler/ascParse.y"
    {
	  if ((yyvsp[(2) - (3)].listp) != NULL) {
	    gl_append_list((yyvsp[(1) - (3)].listp),(yyvsp[(2) - (3)].listp));
	  }
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 85:
/* Line 1792 of yacc.c  */
#line 1253 "ascend/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 127:
/* Line 1792 of yacc.c  */
#line 1309 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[(3) - (5)].id_ptr));
	  if ((yyvsp[(5) - (5)].eptr) != NULL) {
	    ErrMsg_Generic("WITH VALUE clause not allowed in IS_A.");
	    g_untrapped_error++;
	    DestroyVariableList((yyvsp[(1) - (5)].lptr));
	    DestroySetList(g_typeargs);
	    DestroyExprList((yyvsp[(5) - (5)].eptr));
	    (yyval.statptr) = NULL;
	  } else {
	    if (tmptype != NULL) {
	      if ((GetBaseType(tmptype) != model_type) &&
	          (g_typeargs != NULL)) {
	        error_reporter_current_line(ASC_USER_ERROR,
	                "IS_A has arguments to the nonmodel type %s.\n",
	                SCP((yyvsp[(3) - (5)].id_ptr)));
	        DestroyVariableList((yyvsp[(1) - (5)].lptr));
	        DestroySetList(g_typeargs);
	        DestroyExprList((yyvsp[(5) - (5)].eptr));
	        g_untrapped_error++;
	        (yyval.statptr) = NULL;
	      } else {
	        (yyval.statptr) = CreateISA((yyvsp[(1) - (5)].lptr),(yyvsp[(3) - (5)].id_ptr),g_typeargs,(yyvsp[(4) - (5)].id_ptr),0);
	      }
	    } else {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_A uses the undefined type %s.", SCP((yyvsp[(3) - (5)].id_ptr)));
	      DestroyVariableList((yyvsp[(1) - (5)].lptr));
	      DestroySetList(g_typeargs);
	      DestroyExprList((yyvsp[(5) - (5)].eptr));
	      g_untrapped_error++;
	      (yyval.statptr) = NULL;
	    }
	  }
	  g_typeargs = NULL;

	}
    break;

  case 128:
/* Line 1792 of yacc.c  */
#line 1350 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[(3) - (5)].id_ptr));
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) &&
	        (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"WILL_BE has arguments to the nonmodel type '%s'",SCP((yyvsp[(3) - (5)].id_ptr)));
	      DestroyVariableList((yyvsp[(1) - (5)].lptr));
	      DestroySetList(g_typeargs);
	      DestroyExprList((yyvsp[(5) - (5)].eptr));
	      g_untrapped_error++;
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateWILLBE((yyvsp[(1) - (5)].lptr),(yyvsp[(3) - (5)].id_ptr),g_typeargs,(yyvsp[(4) - (5)].id_ptr),(yyvsp[(5) - (5)].eptr));
	    }
	  } else {
	    DestroyVariableList((yyvsp[(1) - (5)].lptr));
	    DestroySetList(g_typeargs);
	    DestroyExprList((yyvsp[(5) - (5)].eptr));
	    g_untrapped_error++;
	    (yyval.statptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"WILL_BE uses the undefined type %s.",SCP((yyvsp[(3) - (5)].id_ptr)));
	  }
	  g_typeargs = NULL;
	}
    break;

  case 129:
/* Line 1792 of yacc.c  */
#line 1379 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateALIASES((yyvsp[(1) - (3)].lptr),(yyvsp[(3) - (3)].nptr));
	}
    break;

  case 130:
/* Line 1792 of yacc.c  */
#line 1384 "ascend/compiler/ascParse.y"
    {
	  int carray_err;
	  carray_err = 0;
	  if (VariableListLength((yyvsp[(1) - (12)].lptr)) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES allows only 1 LHS name. Found:");
	    WriteVariableList(ASCERR,(yyvsp[(1) - (12)].lptr));
	  }
	  if (VariableListLength((yyvsp[(7) - (12)].lptr)) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES/IS_A allows only one LHS name. Found:");
	    WriteVariableList(ASCERR,(yyvsp[(7) - (12)].lptr));
	  }
	  /* verify $9 == "set" */
	  if (!carray_err && (yyvsp[(9) - (12)].id_ptr) != GetBaseTypeName(set_type)) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,"Compound ALIASES statement requires IS_A %s. ",SCP(GetBaseTypeName(set_type)));
	    FPRINTF(ASCERR,"    Found %s.\n",SCP((yyvsp[(9) - (12)].id_ptr)));
	  }
	  /* verify set type */
	  if ((!carray_err) &&
	      ((yyvsp[(11) - (12)].id_ptr) != GetBaseTypeName(symbol_constant_type)) &&
	      ((yyvsp[(11) - (12)].id_ptr) != GetBaseTypeName(integer_constant_type))) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES IS_A statement requires %s or %s.\n",
	            SCP(GetBaseTypeName(integer_constant_type)),
	            SCP(GetBaseTypeName(symbol_constant_type)));
	    FPRINTF(ASCERR,"	Found %s.\n",SCP((yyvsp[(11) - (12)].id_ptr)));
	  }
	  if (carray_err) {
	    DestroyVariableList((yyvsp[(1) - (12)].lptr));
	    DestroyVariableList((yyvsp[(4) - (12)].lptr));
	    DestroyVariableList((yyvsp[(7) - (12)].lptr));
	    DestroySetList((yyvsp[(12) - (12)].sptr));
	    g_untrapped_error++;
	    (yyval.statptr) = NULL;
	  } else {
	    int intset;
	    intset = ((yyvsp[(11) - (12)].id_ptr) == GetBaseTypeName(integer_constant_type));
	    (yyval.statptr) = CreateARR((yyvsp[(1) - (12)].lptr),(yyvsp[(4) - (12)].lptr),(yyvsp[(7) - (12)].lptr),intset,(yyvsp[(12) - (12)].sptr));
	  }
	}
    break;

  case 131:
/* Line 1792 of yacc.c  */
#line 1433 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 132:
/* Line 1792 of yacc.c  */
#line 1437 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 133:
/* Line 1792 of yacc.c  */
#line 1444 "ascend/compiler/ascParse.y"
    {
	  CONST struct VariableList *vl;
	  CONST struct Name *nptr;
          struct VariableList *dervl;
	  struct StatementList *statlist;
          struct gl_list_t *stats;
	  struct Statement *isa;
	  struct Name *dername;
          vl = (yyvsp[(3) - (4)].lptr);
	  stats = gl_create(7L);
          while(vl) {
	    nptr = NamePointer(vl);
	    dervl = CreateVariableNode(CopyName(nptr));
	    if ((yyvsp[(4) - (4)].nptr)) LinkVariableNodes(dervl,CreateVariableNode((yyvsp[(4) - (4)].nptr)));
	    dername = CreateDerivName(CreateDeriv(CopyVariableList(dervl)));
	    isa = CreateISA(CreateVariableNode(dername),NULL,NULL,NULL,1);
	    gl_append_ptr(stats,(VOIDPTR)isa);
	    vl = NextVariableNode(vl);
	  }
	  statlist = CreateStatementList(stats);
	  (yyval.statptr) = CreateISDER(statlist,(yyvsp[(3) - (4)].lptr),(yyvsp[(4) - (4)].nptr));
	}
    break;

  case 134:
/* Line 1792 of yacc.c  */
#line 1470 "ascend/compiler/ascParse.y"
    {
	  CONST struct VariableList *vl;
	  CONST struct Name *nptr;
	  struct StatementList *statlist;
          struct gl_list_t *stats;
	  struct Statement *isa;
	  struct Name *prename;
          vl = (yyvsp[(2) - (2)].lptr);
	  stats = gl_create(7L);
          while(vl) {
	    nptr = NamePointer(vl);
	    prename = CreatePreName(CreatePre(CopyName(nptr)));
	    isa = CreateISA(CreateVariableNode(prename),NULL,NULL,NULL,2);
	    gl_append_ptr(stats,(VOIDPTR)isa);
	    vl = NextVariableNode(vl);
	  }
	  statlist = CreateStatementList(stats);
	  (yyval.statptr) = CreateISPRE(statlist,(yyvsp[(2) - (2)].lptr));
	}
    break;

  case 135:
/* Line 1792 of yacc.c  */
#line 1493 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 136:
/* Line 1792 of yacc.c  */
#line 1497 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (2)].nptr);
	}
    break;

  case 137:
/* Line 1792 of yacc.c  */
#line 1504 "ascend/compiler/ascParse.y"
    {
	  if (FindType((yyvsp[(3) - (4)].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[(3) - (4)].id_ptr)));
	  }
	}
    break;

  case 138:
/* Line 1792 of yacc.c  */
#line 1516 "ascend/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[(3) - (3)].id_ptr));
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) && 
	        (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_REFINED_TO has arguments to the nonmodel type %s.",SCP((yyvsp[(3) - (3)].id_ptr)));
	      DestroyVariableList((yyvsp[(1) - (3)].lptr));
	      DestroySetList(g_typeargs);
	      g_untrapped_error++;
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateIRT((yyvsp[(1) - (3)].lptr),(yyvsp[(3) - (3)].id_ptr),g_typeargs);
	    }
	  } else {
	    error_reporter_current_line(ASC_USER_ERROR,"The IS_REFINED_TO uses the undefined type %s.\n",SCP((yyvsp[(3) - (3)].id_ptr)));
	    DestroyVariableList((yyvsp[(1) - (3)].lptr));
	    DestroySetList(g_typeargs);
	    g_untrapped_error++;
	    (yyval.statptr) = NULL;
	  }
	  g_typeargs = NULL;
	}
    break;

  case 139:
/* Line 1792 of yacc.c  */
#line 1543 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_callargs = NULL;
	}
    break;

  case 140:
/* Line 1792 of yacc.c  */
#line 1548 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_callargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 141:
/* Line 1792 of yacc.c  */
#line 1556 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_typeargs = NULL;
	}
    break;

  case 142:
/* Line 1792 of yacc.c  */
#line 1561 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_typeargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 143:
/* Line 1792 of yacc.c  */
#line 1569 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 144:
/* Line 1792 of yacc.c  */
#line 1573 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 145:
/* Line 1792 of yacc.c  */
#line 1580 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 146:
/* Line 1792 of yacc.c  */
#line 1584 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 147:
/* Line 1792 of yacc.c  */
#line 1591 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = NULL;
	}
    break;

  case 148:
/* Line 1792 of yacc.c  */
#line 1595 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 149:
/* Line 1792 of yacc.c  */
#line 1602 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateAA((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 150:
/* Line 1792 of yacc.c  */
#line 1609 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = IgnoreLNK((yyvsp[(5) - (8)].sym_ptr),NULL,(yyvsp[(7) - (8)].lptr));
	}
    break;

  case 151:
/* Line 1792 of yacc.c  */
#line 1613 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateLNK((yyvsp[(3) - (6)].sym_ptr),NULL,(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 152:
/* Line 1792 of yacc.c  */
#line 1617 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 153:
/* Line 1792 of yacc.c  */
#line 1624 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateUNLNK((yyvsp[(3) - (6)].sym_ptr),NULL,(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 154:
/* Line 1792 of yacc.c  */
#line 1628 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 155:
/* Line 1792 of yacc.c  */
#line 1635 "ascend/compiler/ascParse.y"
    {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[(3) - (4)].lptr));
	}
    break;

  case 156:
/* Line 1792 of yacc.c  */
#line 1644 "ascend/compiler/ascParse.y"
    {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[(2) - (2)].lptr));
	}
    break;

  case 157:
/* Line 1792 of yacc.c  */
#line 1653 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateATS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 158:
/* Line 1792 of yacc.c  */
#line 1660 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 159:
/* Line 1792 of yacc.c  */
#line 1667 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWNBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 160:
/* Line 1792 of yacc.c  */
#line 1674 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 161:
/* Line 1792 of yacc.c  */
#line 1678 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 162:
/* Line 1792 of yacc.c  */
#line 1685 "ascend/compiler/ascParse.y"
    {
	  if (IsRelation((yyvsp[(1) - (1)].eptr))) {
	    if (g_parse_relns == 0) {
	      DestroyExprList((yyvsp[(1) - (1)].eptr));
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateREL(NULL,(yyvsp[(1) - (1)].eptr));
	    }
	  } else {
	    (yyval.statptr) = CreateLOGREL(NULL,(yyvsp[(1) - (1)].eptr));
	  }
	}
    break;

  case 163:
/* Line 1792 of yacc.c  */
#line 1698 "ascend/compiler/ascParse.y"
    {
	  if (IsRelation((yyvsp[(3) - (3)].eptr))) {
	    if (g_parse_relns == 0) {
	      DestroyExprList((yyvsp[(3) - (3)].eptr));
	      DestroyName((yyvsp[(1) - (3)].nptr));
	      (yyval.statptr) = NULL;
	    } else {
	      (yyval.statptr) = CreateREL((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	    }
	  } else {
	    (yyval.statptr) = CreateLOGREL((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	  }
	}
    break;

  case 164:
/* Line 1792 of yacc.c  */
#line 1715 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(1) - (1)].eptr);
	  if (NumberOfRelOps((yyvsp[(1) - (1)].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
    break;

  case 165:
/* Line 1792 of yacc.c  */
#line 1725 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 166:
/* Line 1792 of yacc.c  */
#line 1733 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 167:
/* Line 1792 of yacc.c  */
#line 1744 "ascend/compiler/ascParse.y"
    {
      /*
       * This is the blackbox declarative external relation.
       */
      	struct VariableList *vl;
	/*determine the number of variables declared in input_args and output_args*/
      	unsigned long n_inputs, n_outputs;
	n_inputs = VariableListLength((yyvsp[(5) - (9)].lptr));
	n_outputs = VariableListLength((yyvsp[(7) - (9)].lptr));
	/*continue with normal parsing process */
	vl = JoinVariableLists((yyvsp[(5) - (9)].lptr),(yyvsp[(7) - (9)].lptr)); 
	/* $$ = CreateEXTERN(2,$1,SCP($3),vl,$8,NULL); */

      /*$$ = CreateEXTERNBlackBox($1,SCP($3),vl,$8); //original */
      //statement now also knows how many of the variables in vl are inputs/outputs
      (yyval.statptr) = CreateEXTERNBlackBox((yyvsp[(1) - (9)].nptr),SCP((yyvsp[(3) - (9)].id_ptr)),vl,(yyvsp[(8) - (9)].nptr),n_inputs,n_outputs); 
    }
    break;

  case 168:
/* Line 1792 of yacc.c  */
#line 1765 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 169:
/* Line 1792 of yacc.c  */
#line 1772 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 170:
/* Line 1792 of yacc.c  */
#line 1779 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 171:
/* Line 1792 of yacc.c  */
#line 1783 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (4)].nptr);
	}
    break;

  case 172:
/* Line 1792 of yacc.c  */
#line 1790 "ascend/compiler/ascParse.y"
    {
	  /*
	   * This is the glassbox declarative external relation.
	   * This now allows a scope for placement of the relations
	   */
	  struct VariableList *vl = (yyvsp[(5) - (9)].lptr);
	  struct Name *nptr;
	  char tmp[32]; 
	  symchar *str;

	  sprintf(tmp,"%ld",(yyvsp[(7) - (9)].int_value));
	  str = AddSymbol(tmp);
	  nptr = CreateIdName(str);
	/* $$ = CreateEXTERN(1,$1,SCP($3),vl,nptr,$9); */
	  (yyval.statptr) = CreateEXTERNGlassBox((yyvsp[(1) - (9)].nptr),SCP((yyvsp[(3) - (9)].id_ptr)),vl,nptr,(yyvsp[(9) - (9)].nptr));
	}
    break;

  case 173:
/* Line 1792 of yacc.c  */
#line 1810 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 174:
/* Line 1792 of yacc.c  */
#line 1814 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (2)].nptr);
	}
    break;

  case 175:
/* Line 1792 of yacc.c  */
#line 1822 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(8) - (8)].int_value) != FOR_TOK ) {
	    WarnMsg_MismatchEnd("FOR", SCP((yyvsp[(2) - (8)].id_ptr)), (yyvsp[(8) - (8)].int_value), NULL);
	  }
	  if ((yyvsp[(6) - (8)].fkind) == fk_create && (yyvsp[(5) - (8)].order) != f_random) {
	    /* create cannot have an order in declarative FOR */
	    ErrMsg_Generic("FOR loops only accept DECREASING or INCREASING in the method section.");
	    g_untrapped_error++;
	  }
	  if ((yyvsp[(6) - (8)].fkind) == fk_do && (yyvsp[(5) - (8)].order) == f_random) {
	    /* all FOR/DO default to increasing */
	    (yyval.statptr) = CreateFOR((yyvsp[(2) - (8)].id_ptr),(yyvsp[(4) - (8)].eptr),(yyvsp[(7) - (8)].slptr),f_increasing,(yyvsp[(6) - (8)].fkind));
	  } else {
	    (yyval.statptr) = CreateFOR((yyvsp[(2) - (8)].id_ptr),(yyvsp[(4) - (8)].eptr),(yyvsp[(7) - (8)].slptr),(yyvsp[(5) - (8)].order),(yyvsp[(6) - (8)].fkind));
	  }
	}
    break;

  case 176:
/* Line 1792 of yacc.c  */
#line 1842 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_random;
	}
    break;

  case 177:
/* Line 1792 of yacc.c  */
#line 1846 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_increasing;
	}
    break;

  case 178:
/* Line 1792 of yacc.c  */
#line 1850 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_decreasing;
	}
    break;

  case 179:
/* Line 1792 of yacc.c  */
#line 1857 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
    break;

  case 180:
/* Line 1792 of yacc.c  */
#line 1861 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
    break;

  case 181:
/* Line 1792 of yacc.c  */
#line 1865 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
    break;

  case 182:
/* Line 1792 of yacc.c  */
#line 1869 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
    break;

  case 183:
/* Line 1792 of yacc.c  */
#line 1876 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(2) - (2)].nptr),NULL);
	}
    break;

  case 184:
/* Line 1792 of yacc.c  */
#line 1880 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(4) - (4)].nptr),(yyvsp[(2) - (4)].nptr));	  /* type :: name */
	}
    break;

  case 185:
/* Line 1792 of yacc.c  */
#line 1887 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 186:
/* Line 1792 of yacc.c  */
#line 1895 "ascend/compiler/ascParse.y"
    {
		struct Statement *assign = (yyvsp[(2) - (2)].statptr);
		struct Name *n = CopyName((yyvsp[(2) - (2)].statptr) -> v.asgn.nptr);
		struct VariableList *vars = CreateVariableNode(n);
		struct Statement *fix = CreateFIX(ReverseVariableList(vars));
		struct gl_list_t *fix_and_assign = gl_create(7L);
		gl_append_ptr(fix_and_assign,(char*)fix);
		gl_append_ptr(fix_and_assign,(char*)assign);
		(yyval.listp) = fix_and_assign;
	}
    break;

  case 187:
/* Line 1792 of yacc.c  */
#line 1909 "ascend/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateFREE((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 188:
/* Line 1792 of yacc.c  */
#line 1916 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[(2) - (2)].id_ptr)));
	}
    break;

  case 189:
/* Line 1792 of yacc.c  */
#line 1924 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[(2) - (3)].id_ptr)),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 190:
/* Line 1792 of yacc.c  */
#line 1932 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE();
	}
    break;

  case 191:
/* Line 1792 of yacc.c  */
#line 1940 "ascend/compiler/ascParse.y"
    {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[(2) - (5)].id_ptr)),(yyvsp[(4) - (5)].lptr));
	}
    break;

  case 192:
/* Line 1792 of yacc.c  */
#line 1951 "ascend/compiler/ascParse.y"
    {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[(2) - (2)].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
    break;

  case 193:
/* Line 1792 of yacc.c  */
#line 1962 "ascend/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateASSERT((yyvsp[(2) - (2)].eptr));
	}
    break;

  case 194:
/* Line 1792 of yacc.c  */
#line 1968 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[(2) - (6)].eptr),(yyvsp[(4) - (6)].slptr),(yyvsp[(5) - (6)].slptr));
	}
    break;

  case 195:
/* Line 1792 of yacc.c  */
#line 1978 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(5) - (5)].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[(5) - (5)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[(2) - (5)].eptr),(yyvsp[(4) - (5)].slptr));
	}
    break;

  case 196:
/* Line 1792 of yacc.c  */
#line 1987 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 197:
/* Line 1792 of yacc.c  */
#line 1991 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (2)].slptr);
	}
    break;

  case 198:
/* Line 1792 of yacc.c  */
#line 1998 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(4) - (4)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(4) - (4)].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[(3) - (4)].wptr));
	  DestroyVariableList((yyvsp[(2) - (4)].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
    break;

  case 199:
/* Line 1792 of yacc.c  */
#line 2009 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[(5) - (6)].wptr));
	  DestroyVariableList((yyvsp[(4) - (6)].lptr));
	  DestroyName((yyvsp[(1) - (6)].nptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
    break;

  case 200:
/* Line 1792 of yacc.c  */
#line 2021 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].wptr));
	}
    break;

  case 201:
/* Line 1792 of yacc.c  */
#line 2028 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(8) - (8)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(8) - (8)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[(1) - (8)].nptr),(yyvsp[(5) - (8)].lptr),(yyvsp[(7) - (8)].wptr));
	}
    break;

  case 202:
/* Line 1792 of yacc.c  */
#line 2038 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[(1) - (1)].wptr));
	}
    break;

  case 203:
/* Line 1792 of yacc.c  */
#line 2045 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 204:
/* Line 1792 of yacc.c  */
#line 2049 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 205:
/* Line 1792 of yacc.c  */
#line 2053 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].wptr));
	}
    break;

  case 206:
/* Line 1792 of yacc.c  */
#line 2057 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].wptr));
	}
    break;

  case 207:
/* Line 1792 of yacc.c  */
#line 2064 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
    break;

  case 208:
/* Line 1792 of yacc.c  */
#line 2068 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
    break;

  case 209:
/* Line 1792 of yacc.c  */
#line 2072 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
    break;

  case 210:
/* Line 1792 of yacc.c  */
#line 2076 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
    break;

  case 211:
/* Line 1792 of yacc.c  */
#line 2080 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[(2) - (2)].braced_ptr));
	}
    break;

  case 212:
/* Line 1792 of yacc.c  */
#line 2087 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFNAME((yyvsp[(2) - (2)].nptr));
	}
    break;

  case 213:
/* Line 1792 of yacc.c  */
#line 2094 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(4) - (4)].int_value) != EVENT_TOK ) {
	    WarnMsg_MismatchEnd("EVENT", NULL, (yyvsp[(4) - (4)].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in EVENT statement.");
	  DestroyEventList((yyvsp[(3) - (4)].evptr));
	  DestroyVariableList((yyvsp[(2) - (4)].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
    break;

  case 214:
/* Line 1792 of yacc.c  */
#line 2105 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != EVENT_TOK ) {
	    WarnMsg_MismatchEnd("EVENT", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in EVENT statement.");
	  DestroyEventList((yyvsp[(5) - (6)].evptr));
	  DestroyVariableList((yyvsp[(4) - (6)].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
    break;

  case 215:
/* Line 1792 of yacc.c  */
#line 2116 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != EVENT_TOK ) {
	    WarnMsg_MismatchEnd("EVENT", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateEVENT(NULL, (yyvsp[(3) - (6)].lptr), (yyvsp[(5) - (6)].evptr));
	}
    break;

  case 216:
/* Line 1792 of yacc.c  */
#line 2123 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(8) - (8)].int_value) != EVENT_TOK ) {
	    WarnMsg_MismatchEnd("EVENT", NULL, (yyvsp[(8) - (8)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateEVENT((yyvsp[(1) - (8)].nptr), (yyvsp[(5) - (8)].lptr), (yyvsp[(7) - (8)].evptr));
	}
    break;

  case 217:
/* Line 1792 of yacc.c  */
#line 2133 "ascend/compiler/ascParse.y"
    {
	  (yyval.evptr) = ReverseEventCases((yyvsp[(1) - (1)].evptr));
	}
    break;

  case 218:
/* Line 1792 of yacc.c  */
#line 2140 "ascend/compiler/ascParse.y"
    {
	  (yyval.evptr) = CreateEvent((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 219:
/* Line 1792 of yacc.c  */
#line 2144 "ascend/compiler/ascParse.y"
    {
	  (yyval.evptr) = CreateEvent(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 220:
/* Line 1792 of yacc.c  */
#line 2148 "ascend/compiler/ascParse.y"
    {
	  (yyval.evptr) = LinkEventCases(CreateEvent((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].evptr));
	}
    break;

  case 221:
/* Line 1792 of yacc.c  */
#line 2152 "ascend/compiler/ascParse.y"
    {
	  (yyval.evptr) = LinkEventCases(CreateEvent(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].evptr));
	}
    break;

  case 222:
/* Line 1792 of yacc.c  */
#line 2159 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(4) - (4)].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[(4) - (4)].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SELECT statement.");
	  DestroySelectList((yyvsp[(3) - (4)].septr));
	  DestroyVariableList((yyvsp[(2) - (4)].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
    break;

  case 223:
/* Line 1792 of yacc.c  */
#line 2170 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].septr));
	}
    break;

  case 224:
/* Line 1792 of yacc.c  */
#line 2180 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = ReverseSelectCases((yyvsp[(1) - (1)].septr));
	}
    break;

  case 225:
/* Line 1792 of yacc.c  */
#line 2187 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 226:
/* Line 1792 of yacc.c  */
#line 2191 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 227:
/* Line 1792 of yacc.c  */
#line 2195 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].septr));
	}
    break;

  case 228:
/* Line 1792 of yacc.c  */
#line 2199 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].septr));
	}
    break;

  case 229:
/* Line 1792 of yacc.c  */
#line 2206 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(4) - (4)].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[(4) - (4)].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SWITCH statement.");
	  DestroySwitchList((yyvsp[(3) - (4)].swptr));
	  DestroyVariableList((yyvsp[(2) - (4)].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
    break;

  case 230:
/* Line 1792 of yacc.c  */
#line 2217 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].swptr));
	}
    break;

  case 231:
/* Line 1792 of yacc.c  */
#line 2227 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[(1) - (1)].swptr));
	}
    break;

  case 232:
/* Line 1792 of yacc.c  */
#line 2234 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 233:
/* Line 1792 of yacc.c  */
#line 2238 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 234:
/* Line 1792 of yacc.c  */
#line 2242 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].swptr));
	}
    break;

  case 235:
/* Line 1792 of yacc.c  */
#line 2246 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].swptr));
	}
    break;

  case 236:
/* Line 1792 of yacc.c  */
#line 2253 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(3) - (3)].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[(3) - (3)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[(2) - (3)].slptr));
	}
    break;

  case 237:
/* Line 1792 of yacc.c  */
#line 2263 "ascend/compiler/ascParse.y"
    {
	  /*  All processing of notes takes place on the notes_body here.
	   *  Notes should NOT be added to the statement list.
	   *  Here we know the current type and method names.
	   */
	  if( (yyvsp[(3) - (3)].int_value) != NOTES_TOK ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, (yyvsp[(3) - (3)].int_value), NULL);
	  }
	  if ((yyvsp[(2) - (3)].notesptr) != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = (yyvsp[(2) - (3)].notesptr);
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
	    DestroyNoteTmpList((yyvsp[(2) - (3)].notesptr));
	  }
	  (yyval.statptr) = NULL;
	}
    break;

  case 238:
/* Line 1792 of yacc.c  */
#line 2300 "ascend/compiler/ascParse.y"
    {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[(2) - (2)].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[(1) - (2)].sym_ptr);
	}
    break;

  case 239:
/* Line 1792 of yacc.c  */
#line 2309 "ascend/compiler/ascParse.y"
    {
	  struct NoteTmp *nt;
	  (yyval.notesptr) = (yyvsp[(1) - (3)].notesptr);
	  assert((yyvsp[(3) - (3)].notesptr)->lang == NULL);
	  (yyvsp[(3) - (3)].notesptr)->lang = (yyvsp[(2) - (3)].sym_ptr);
	  nt = (yyval.notesptr);
	  while (nt->next != NULL) {
	    nt = nt->next;
	  }
	  LinkNoteTmp(nt,(yyvsp[(3) - (3)].notesptr));
	}
    break;

  case 240:
/* Line 1792 of yacc.c  */
#line 2324 "ascend/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(2) - (2)].braced_ptr),NULL),
	                     (void *)(yyvsp[(1) - (2)].lptr), LineNum());
	}
    break;

  case 241:
/* Line 1792 of yacc.c  */
#line 2329 "ascend/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(3) - (3)].braced_ptr),NULL),
	                     (void *)(yyvsp[(2) - (3)].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[(1) - (3)].notesptr));
	}
    break;

  case 242:
/* Line 1792 of yacc.c  */
#line 2338 "ascend/compiler/ascParse.y"
    {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[(1) - (1)].lptr));
	}
    break;

  case 243:
/* Line 1792 of yacc.c  */
#line 2349 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 244:
/* Line 1792 of yacc.c  */
#line 2353 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(3) - (3)].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[(1) - (3)].lptr));
	}
    break;

  case 245:
/* Line 1792 of yacc.c  */
#line 2358 "ascend/compiler/ascParse.y"
    {
	  ErrMsg_CommaName("name",(yyvsp[(2) - (2)].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[(2) - (2)].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[(1) - (2)].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
    break;

  case 246:
/* Line 1792 of yacc.c  */
#line 2371 "ascend/compiler/ascParse.y"
    {
	  symchar *simple;
	  void *data;
	  enum NoteData nd;
	  (yyval.nptr) = ReverseName((yyvsp[(1) - (2)].nptr));
	  if ((yyvsp[(2) - (2)].dquote_ptr) != NULL && (yyvsp[(1) - (2)].nptr) != NULL) {
	    simple = SimpleNameIdPtr((yyval.nptr));
	    data = (simple == NULL ? (void *)(yyval.nptr) : NULL);
	    nd = (data == NULL ? nd_empty : nd_name);
	    CollectNote(CreateNote(g_type_name, InlineNote(), simple,
	                           g_proc_name,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar((yyvsp[(2) - (2)].dquote_ptr),InlineNote()),
	                           LineNum(), data, nd));
	  }
	}
    break;

  case 247:
/* Line 1792 of yacc.c  */
#line 2391 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(1) - (1)].id_ptr));
	}
    break;

  case 248:
/* Line 1792 of yacc.c  */
#line 2395 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(3) - (3)].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[(1) - (3)].nptr));
	}
    break;

  case 249:
/* Line 1792 of yacc.c  */
#line 2400 "ascend/compiler/ascParse.y"
    {
          if (NameDeriv((yyvsp[(1) - (4)].nptr))) {
	    error_reporter_current_line(ASC_USER_ERROR,"syntax error: For a derivative of an array element use der(argument[set]); name: %s",GetIdFromVlist(DerVlist(NameDerPtr((yyvsp[(1) - (4)].nptr)))));
	    g_untrapped_error++;
          }else{
	    if ((yyvsp[(3) - (4)].sptr) == NULL) {
	      error_reporter_current_line(ASC_USER_ERROR,"syntax error: Empty set in name definition, name:");
	      WriteName(ASCERR,(yyvsp[(1) - (4)].nptr));
	      FPRINTF(ASCERR,"[]\n");
	      g_untrapped_error++;
	    } else {
	      (yyval.nptr) = CreateSetName((yyvsp[(3) - (4)].sptr));
	      LinkNames((yyval.nptr),(yyvsp[(1) - (4)].nptr));
	    }
          }
	}
    break;

  case 250:
/* Line 1792 of yacc.c  */
#line 2417 "ascend/compiler/ascParse.y"
    {	
	  (yyval.nptr) = CreateDerivName(CreateDeriv((yyvsp[(3) - (4)].lptr)));
	}
    break;

  case 251:
/* Line 1792 of yacc.c  */
#line 2421 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreatePreName(CreatePre((yyvsp[(3) - (4)].nptr)));
	}
    break;

  case 252:
/* Line 1792 of yacc.c  */
#line 2428 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
    break;

  case 253:
/* Line 1792 of yacc.c  */
#line 2433 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = EVENT_TOK;
	}
    break;

  case 254:
/* Line 1792 of yacc.c  */
#line 2438 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
    break;

  case 255:
/* Line 1792 of yacc.c  */
#line 2443 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
    break;

  case 256:
/* Line 1792 of yacc.c  */
#line 2448 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
    break;

  case 257:
/* Line 1792 of yacc.c  */
#line 2453 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
    break;

  case 258:
/* Line 1792 of yacc.c  */
#line 2458 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
    break;

  case 259:
/* Line 1792 of yacc.c  */
#line 2463 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
    break;

  case 260:
/* Line 1792 of yacc.c  */
#line 2468 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
    break;

  case 261:
/* Line 1792 of yacc.c  */
#line 2473 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
    break;

  case 262:
/* Line 1792 of yacc.c  */
#line 2478 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
    break;

  case 263:
/* Line 1792 of yacc.c  */
#line 2483 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
    break;

  case 264:
/* Line 1792 of yacc.c  */
#line 2488 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
    break;

  case 265:
/* Line 1792 of yacc.c  */
#line 2493 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = (yyvsp[(2) - (2)].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
    break;

  case 266:
/* Line 1792 of yacc.c  */
#line 2498 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
    break;

  case 267:
/* Line 1792 of yacc.c  */
#line 2506 "ascend/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = NULL;
	}
    break;

  case 268:
/* Line 1792 of yacc.c  */
#line 2510 "ascend/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = (yyvsp[(1) - (1)].braced_ptr);
	}
    break;

  case 269:
/* Line 1792 of yacc.c  */
#line 2517 "ascend/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = NULL;
	}
    break;

  case 270:
/* Line 1792 of yacc.c  */
#line 2521 "ascend/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = (yyvsp[(1) - (1)].dquote_ptr);
	}
    break;

  case 271:
/* Line 1792 of yacc.c  */
#line 2528 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = ReverseSetList((yyvsp[(1) - (1)].sptr));
	}
    break;

  case 272:
/* Line 1792 of yacc.c  */
#line 2532 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 273:
/* Line 1792 of yacc.c  */
#line 2539 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(1) - (1)].eptr));
	}
    break;

  case 274:
/* Line 1792 of yacc.c  */
#line 2543 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 275:
/* Line 1792 of yacc.c  */
#line 2547 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(3) - (3)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (3)].sptr));
	}
    break;

  case 276:
/* Line 1792 of yacc.c  */
#line 2552 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(3) - (5)].eptr),(yyvsp[(5) - (5)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (5)].sptr));
	}
    break;

  case 277:
/* Line 1792 of yacc.c  */
#line 2560 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
    break;

  case 278:
/* Line 1792 of yacc.c  */
#line 2566 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
    break;

  case 279:
/* Line 1792 of yacc.c  */
#line 2575 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (2)].real_value)*(yyvsp[(2) - (2)].real_value);
	}
    break;

  case 280:
/* Line 1792 of yacc.c  */
#line 2579 "ascend/compiler/ascParse.y"
    {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((yyvsp[(2) - (2)].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (yyval.real_value) = (double)(yyvsp[(1) - (2)].int_value)*UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
	    char **errv;
	    (yyval.real_value) = (double)(yyvsp[(1) - (2)].int_value);
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'", (yyvsp[(2) - (2)].braced_ptr));
	    errv = UnitsExplainError((yyvsp[(2) - (2)].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
    break;

  case 281:
/* Line 1792 of yacc.c  */
#line 2600 "ascend/compiler/ascParse.y"
    {
	  g_dim_ptr = Dimensionless();
	  (yyval.real_value) = 1.0;
	}
    break;

  case 282:
/* Line 1792 of yacc.c  */
#line 2605 "ascend/compiler/ascParse.y"
    {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((yyvsp[(1) - (1)].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (yyval.real_value) = UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
	    char **errv;
	    (yyval.real_value) = 1.0;
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'",(yyvsp[(1) - (1)].braced_ptr));
	    errv = UnitsExplainError((yyvsp[(1) - (1)].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
    break;

  case 283:
/* Line 1792 of yacc.c  */
#line 2626 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = (yyvsp[(2) - (2)].dimp);
	}
    break;

  case 284:
/* Line 1792 of yacc.c  */
#line 2630 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = Dimensionless();
	}
    break;

  case 285:
/* Line 1792 of yacc.c  */
#line 2634 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 286:
/* Line 1792 of yacc.c  */
#line 2641 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 287:
/* Line 1792 of yacc.c  */
#line 2645 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[(1) - (1)].dimen)));
	}
    break;

  case 288:
/* Line 1792 of yacc.c  */
#line 2652 "ascend/compiler/ascParse.y"
    {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[(1) - (1)].id_ptr)));
	}
    break;

  case 289:
/* Line 1792 of yacc.c  */
#line 2656 "ascend/compiler/ascParse.y"
    {
	  ClearDimensions(&((yyval.dimen)));
	}
    break;

  case 290:
/* Line 1792 of yacc.c  */
#line 2660 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = SubDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 291:
/* Line 1792 of yacc.c  */
#line 2664 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = AddDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 292:
/* Line 1792 of yacc.c  */
#line 2668 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[(1) - (3)].dimen)),(yyvsp[(3) - (3)].frac_value));
	}
    break;

  case 293:
/* Line 1792 of yacc.c  */
#line 2672 "ascend/compiler/ascParse.y"
    {
	  CopyDimensions(&((yyvsp[(2) - (3)].dimen)),&((yyval.dimen)));
	}
    break;

  case 294:
/* Line 1792 of yacc.c  */
#line 2679 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = (yyvsp[(1) - (2)].int_value) ? NegateF((yyvsp[(2) - (2)].frac_value)) : (yyvsp[(2) - (2)].frac_value);
	}
    break;

  case 295:
/* Line 1792 of yacc.c  */
#line 2686 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(1) - (1)].int_value),(short)1);
	}
    break;

  case 296:
/* Line 1792 of yacc.c  */
#line 2690 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(2) - (5)].int_value),(short)(yyvsp[(4) - (5)].int_value));
	}
    break;

  case 297:
/* Line 1792 of yacc.c  */
#line 2697 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 298:
/* Line 1792 of yacc.c  */
#line 2701 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 299:
/* Line 1792 of yacc.c  */
#line 2705 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 300:
/* Line 1792 of yacc.c  */
#line 2712 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr((yyvsp[(1) - (1)].int_value));
	}
    break;

  case 301:
/* Line 1792 of yacc.c  */
#line 2716 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
    break;

  case 302:
/* Line 1792 of yacc.c  */
#line 2720 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr((yyvsp[(1) - (1)].real_value),g_dim_ptr);
	}
    break;

  case 303:
/* Line 1792 of yacc.c  */
#line 2724 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
    break;

  case 304:
/* Line 1792 of yacc.c  */
#line 2728 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateTrueExpr();
	}
    break;

  case 305:
/* Line 1792 of yacc.c  */
#line 2732 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateFalseExpr();
	}
    break;

  case 306:
/* Line 1792 of yacc.c  */
#line 2736 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateAnyExpr();
	}
    break;

  case 307:
/* Line 1792 of yacc.c  */
#line 2740 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[(1) - (1)].sym_ptr));
	}
    break;

  case 308:
/* Line 1792 of yacc.c  */
#line 2744 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateVarExpr((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 309:
/* Line 1792 of yacc.c  */
#line 2748 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSetExpr((yyvsp[(2) - (3)].sptr));
	}
    break;

  case 310:
/* Line 1792 of yacc.c  */
#line 2752 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 311:
/* Line 1792 of yacc.c  */
#line 2757 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 312:
/* Line 1792 of yacc.c  */
#line 2762 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 313:
/* Line 1792 of yacc.c  */
#line 2767 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 314:
/* Line 1792 of yacc.c  */
#line 2772 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 315:
/* Line 1792 of yacc.c  */
#line 2777 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 316:
/* Line 1792 of yacc.c  */
#line 2782 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 317:
/* Line 1792 of yacc.c  */
#line 2787 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_not));
	}
    break;

  case 318:
/* Line 1792 of yacc.c  */
#line 2791 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 319:
/* Line 1792 of yacc.c  */
#line 2796 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 320:
/* Line 1792 of yacc.c  */
#line 2801 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 321:
/* Line 1792 of yacc.c  */
#line 2806 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 322:
/* Line 1792 of yacc.c  */
#line 2811 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 323:
/* Line 1792 of yacc.c  */
#line 2816 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 324:
/* Line 1792 of yacc.c  */
#line 2820 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_uminus));
	}
    break;

  case 325:
/* Line 1792 of yacc.c  */
#line 2824 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].real_value),g_dim_ptr);
	}
    break;

  case 326:
/* Line 1792 of yacc.c  */
#line 2828 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (4)].nptr),DBL_MAX,NULL);
	}
    break;

  case 327:
/* Line 1792 of yacc.c  */
#line 2832 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
    break;

  case 328:
/* Line 1792 of yacc.c  */
#line 2839 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 329:
/* Line 1792 of yacc.c  */
#line 2843 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
    break;

  case 330:
/* Line 1792 of yacc.c  */
#line 2850 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 331:
/* Line 1792 of yacc.c  */
#line 2854 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
    break;

  case 332:
/* Line 1792 of yacc.c  */
#line 2861 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 333:
/* Line 1792 of yacc.c  */
#line 2865 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
    break;

  case 334:
/* Line 1792 of yacc.c  */
#line 2872 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 335:
/* Line 1792 of yacc.c  */
#line 2876 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
    break;

  case 336:
/* Line 1792 of yacc.c  */
#line 2883 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 337:
/* Line 1792 of yacc.c  */
#line 2887 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
    break;

  case 338:
/* Line 1792 of yacc.c  */
#line 2894 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 339:
/* Line 1792 of yacc.c  */
#line 2898 "ascend/compiler/ascParse.y"
    {
	  CONST struct Func *fptr;
	  if ((fptr = LookupFunc(SCP((yyvsp[(1) - (4)].id_ptr))))!=NULL) {
	    (yyval.eptr) = JoinExprLists((yyvsp[(3) - (4)].eptr),CreateFuncExpr(fptr));
	  } else {
	    (yyval.eptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Function '%s' is not defined.",SCP((yyvsp[(1) - (4)].id_ptr)));
	    g_untrapped_error++;
	  }  
	}
    break;

  case 340:
/* Line 1792 of yacc.c  */
#line 2909 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (3)].eptr);
	}
    break;

  case 341:
/* Line 1792 of yacc.c  */
#line 2916 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
    break;

  case 342:
/* Line 1792 of yacc.c  */
#line 2920 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
    break;

  case 343:
/* Line 1792 of yacc.c  */
#line 2924 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
    break;

  case 344:
/* Line 1792 of yacc.c  */
#line 2928 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
    break;

  case 345:
/* Line 1792 of yacc.c  */
#line 2932 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
    break;

  case 346:
/* Line 1792 of yacc.c  */
#line 2936 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
    break;

  case 347:
/* Line 1792 of yacc.c  */
#line 2943 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
    break;

  case 348:
/* Line 1792 of yacc.c  */
#line 2947 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
    break;


/* Line 1792 of yacc.c  */
#line 5961 "ascend/compiler/ascParse.c"
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2055 of yacc.c  */
#line 2951 "ascend/compiler/ascParse.y"

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

