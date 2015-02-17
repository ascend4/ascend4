
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         zz_parse
#define yylex           zz_lex
#define yyerror         zz_error
#define yylval          zz_lval
#define yychar          zz_char
#define yydebug         zz_debug
#define yynerrs         zz_nerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 31 "ascend/compiler/ascParse.y"

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



/* Line 189 of yacc.c  */
#line 351 "ascend/compiler/ascParse.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
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
     CONDITIONAL_TOK = 275,
     CONSTANT_TOK = 276,
     CONTINUE_TOK = 277,
     CREATE_TOK = 278,
     DATA_TOK = 279,
     DECREASING_TOK = 280,
     DEFAULT_TOK = 281,
     DEFINITION_TOK = 282,
     DER_TOK = 283,
     DIMENSION_TOK = 284,
     DIMENSIONLESS_TOK = 285,
     DO_TOK = 286,
     ELSE_TOK = 287,
     END_TOK = 288,
     EXPECT_TOK = 289,
     EXTERNAL_TOK = 290,
     FALSE_TOK = 291,
     FALLTHRU_TOK = 292,
     FIX_TOK = 293,
     FOR_TOK = 294,
     FREE_TOK = 295,
     FROM_TOK = 296,
     GLOBAL_TOK = 297,
     IF_TOK = 298,
     IGNORE_TOK = 299,
     IMPORT_TOK = 300,
     IN_TOK = 301,
     INPUT_TOK = 302,
     INCREASING_TOK = 303,
     INTERACTIVE_TOK = 304,
     INDEPENDENT_TOK = 305,
     INTERSECTION_TOK = 306,
     ISA_TOK = 307,
     _IS_T = 308,
     ISREFINEDTO_TOK = 309,
     LINK_TOK = 310,
     MAXIMIZE_TOK = 311,
     MAXINTEGER_TOK = 312,
     MAXREAL_TOK = 313,
     METHODS_TOK = 314,
     METHOD_TOK = 315,
     MINIMIZE_TOK = 316,
     MODEL_TOK = 317,
     NOT_TOK = 318,
     NOTES_TOK = 319,
     OF_TOK = 320,
     OPTION_TOK = 321,
     OR_TOK = 322,
     OTHERWISE_TOK = 323,
     OUTPUT_TOK = 324,
     PATCH_TOK = 325,
     PROD_TOK = 326,
     PROVIDE_TOK = 327,
     REFINES_TOK = 328,
     REPLACE_TOK = 329,
     REQUIRE_TOK = 330,
     RETURN_TOK = 331,
     RUN_TOK = 332,
     SATISFIED_TOK = 333,
     SELECT_TOK = 334,
     SIZE_TOK = 335,
     SOLVE_TOK = 336,
     SOLVER_TOK = 337,
     STOP_TOK = 338,
     SUCHTHAT_TOK = 339,
     SUM_TOK = 340,
     SWITCH_TOK = 341,
     INTEGRATE_TOK = 342,
     TO_TOK = 343,
     STEPS_TOK = 344,
     SUBSOLVER_TOK = 345,
     ISINTEGRATOR_TOK = 346,
     THEN_TOK = 347,
     TRUE_TOK = 348,
     UNION_TOK = 349,
     UNITS_TOK = 350,
     UNIVERSAL_TOK = 351,
     UNLINK_TOK = 352,
     WHEN_TOK = 353,
     WHERE_TOK = 354,
     WHILE_TOK = 355,
     WILLBE_TOK = 356,
     WILLBETHESAME_TOK = 357,
     WILLNOTBETHESAME_TOK = 358,
     ASSIGN_TOK = 359,
     CASSIGN_TOK = 360,
     DBLCOLON_TOK = 361,
     USE_TOK = 362,
     LEQ_TOK = 363,
     GEQ_TOK = 364,
     NEQ_TOK = 365,
     DOTDOT_TOK = 366,
     WITH_TOK = 367,
     VALUE_TOK = 368,
     WITH_VALUE_T = 369,
     REAL_TOK = 370,
     INTEGER_TOK = 371,
     IDENTIFIER_TOK = 372,
     BRACEDTEXT_TOK = 373,
     SYMBOL_TOK = 374,
     DQUOTE_TOK = 375,
     UPLUS_TOK = 376,
     UMINUS_TOK = 377
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
#define CONDITIONAL_TOK 275
#define CONSTANT_TOK 276
#define CONTINUE_TOK 277
#define CREATE_TOK 278
#define DATA_TOK 279
#define DECREASING_TOK 280
#define DEFAULT_TOK 281
#define DEFINITION_TOK 282
#define DER_TOK 283
#define DIMENSION_TOK 284
#define DIMENSIONLESS_TOK 285
#define DO_TOK 286
#define ELSE_TOK 287
#define END_TOK 288
#define EXPECT_TOK 289
#define EXTERNAL_TOK 290
#define FALSE_TOK 291
#define FALLTHRU_TOK 292
#define FIX_TOK 293
#define FOR_TOK 294
#define FREE_TOK 295
#define FROM_TOK 296
#define GLOBAL_TOK 297
#define IF_TOK 298
#define IGNORE_TOK 299
#define IMPORT_TOK 300
#define IN_TOK 301
#define INPUT_TOK 302
#define INCREASING_TOK 303
#define INTERACTIVE_TOK 304
#define INDEPENDENT_TOK 305
#define INTERSECTION_TOK 306
#define ISA_TOK 307
#define _IS_T 308
#define ISREFINEDTO_TOK 309
#define LINK_TOK 310
#define MAXIMIZE_TOK 311
#define MAXINTEGER_TOK 312
#define MAXREAL_TOK 313
#define METHODS_TOK 314
#define METHOD_TOK 315
#define MINIMIZE_TOK 316
#define MODEL_TOK 317
#define NOT_TOK 318
#define NOTES_TOK 319
#define OF_TOK 320
#define OPTION_TOK 321
#define OR_TOK 322
#define OTHERWISE_TOK 323
#define OUTPUT_TOK 324
#define PATCH_TOK 325
#define PROD_TOK 326
#define PROVIDE_TOK 327
#define REFINES_TOK 328
#define REPLACE_TOK 329
#define REQUIRE_TOK 330
#define RETURN_TOK 331
#define RUN_TOK 332
#define SATISFIED_TOK 333
#define SELECT_TOK 334
#define SIZE_TOK 335
#define SOLVE_TOK 336
#define SOLVER_TOK 337
#define STOP_TOK 338
#define SUCHTHAT_TOK 339
#define SUM_TOK 340
#define SWITCH_TOK 341
#define INTEGRATE_TOK 342
#define TO_TOK 343
#define STEPS_TOK 344
#define SUBSOLVER_TOK 345
#define ISINTEGRATOR_TOK 346
#define THEN_TOK 347
#define TRUE_TOK 348
#define UNION_TOK 349
#define UNITS_TOK 350
#define UNIVERSAL_TOK 351
#define UNLINK_TOK 352
#define WHEN_TOK 353
#define WHERE_TOK 354
#define WHILE_TOK 355
#define WILLBE_TOK 356
#define WILLBETHESAME_TOK 357
#define WILLNOTBETHESAME_TOK 358
#define ASSIGN_TOK 359
#define CASSIGN_TOK 360
#define DBLCOLON_TOK 361
#define USE_TOK 362
#define LEQ_TOK 363
#define GEQ_TOK 364
#define NEQ_TOK 365
#define DOTDOT_TOK 366
#define WITH_TOK 367
#define VALUE_TOK 368
#define WITH_VALUE_T 369
#define REAL_TOK 370
#define INTEGER_TOK 371
#define IDENTIFIER_TOK 372
#define BRACEDTEXT_TOK 373
#define SYMBOL_TOK 374
#define DQUOTE_TOK 375
#define UPLUS_TOK 376
#define UMINUS_TOK 377




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 304 "ascend/compiler/ascParse.y"

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
  struct NoteTmp *notesptr;	/* change this once struct Notes is defined */
  struct gl_list_t *listp;
  struct InitProcedure *procptr;
  CONST dim_type *dimp;
  struct TypeDescription *tptr;
  struct UnitDefinition *udefptr;
  dim_type dimen;
  enum ForOrder order;
  enum ForKind fkind;



/* Line 214 of yacc.c  */
#line 661 "ascend/compiler/ascParse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 673 "ascend/compiler/ascParse.c"

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
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
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

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

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

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1292

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  140
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  127
/* YYNRULES -- Number of rules.  */
#define YYNRULES  332
/* YYNRULES -- Number of states.  */
#define YYNSTATES  678

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   377

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     134,   135,   129,   126,   121,   127,   137,   128,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   136,   133,
     123,   124,   125,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   138,     2,   139,   132,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   122,     2,     2,     2,     2,     2,
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
     115,   116,   117,   118,   119,   120,   130,   131
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    41,
      45,    49,    52,    56,    60,    63,    69,    73,    78,    85,
      90,    96,   103,   108,   114,   121,   128,   135,   138,   139,
     143,   146,   149,   152,   155,   164,   165,   169,   172,   175,
     178,   185,   190,   198,   201,   202,   206,   207,   212,   213,
     217,   223,   229,   230,   232,   238,   241,   244,   248,   249,
     252,   257,   258,   259,   263,   265,   266,   269,   275,   278,
     280,   281,   285,   289,   293,   295,   297,   299,   301,   303,
     305,   307,   309,   311,   313,   315,   317,   319,   321,   323,
     325,   327,   329,   331,   333,   335,   337,   339,   341,   343,
     345,   347,   349,   351,   353,   355,   357,   359,   361,   363,
     365,   367,   369,   371,   373,   379,   385,   389,   402,   403,
     408,   413,   417,   419,   424,   426,   431,   432,   435,   436,
     439,   440,   443,   446,   455,   462,   469,   476,   483,   488,
     491,   494,   497,   500,   504,   508,   510,   514,   516,   519,
     522,   532,   536,   540,   541,   546,   556,   557,   560,   569,
     570,   572,   574,   576,   578,   580,   582,   585,   590,   593,
     596,   599,   607,   610,   613,   617,   621,   623,   629,   632,
     635,   642,   648,   649,   652,   657,   664,   671,   680,   682,
     687,   691,   697,   702,   704,   706,   708,   710,   713,   716,
     721,   728,   730,   735,   739,   745,   750,   755,   762,   764,
     769,   773,   779,   784,   788,   792,   795,   799,   802,   806,
     808,   810,   814,   817,   820,   822,   826,   831,   834,   837,
     840,   843,   846,   849,   852,   855,   858,   861,   864,   867,
     870,   872,   873,   875,   876,   878,   880,   881,   883,   887,
     891,   897,   899,   901,   904,   907,   908,   910,   913,   915,
     916,   918,   920,   922,   924,   928,   932,   936,   940,   943,
     945,   951,   952,   954,   956,   958,   960,   962,   964,   966,
     968,   970,   972,   974,   978,   982,   986,   990,   994,   998,
    1002,  1006,  1009,  1013,  1017,  1021,  1025,  1029,  1032,  1035,
    1042,  1047,  1052,  1057,  1062,  1067,  1072,  1077,  1082,  1087,
    1092,  1097,  1102,  1107,  1112,  1116,  1118,  1120,  1122,  1124,
    1126,  1128,  1130
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     141,     0,    -1,    -1,   141,   142,    -1,   144,    -1,   145,
      -1,   146,    -1,   149,    -1,   151,    -1,   147,    -1,   157,
      -1,   153,    -1,   160,    -1,   169,    -1,   166,    -1,   171,
      -1,   143,    -1,     1,    -1,    42,   133,   181,   250,   133,
      -1,    75,   120,   133,    -1,    75,   249,   133,    -1,    75,
     249,    -1,    72,   120,   133,    -1,    72,   249,   133,    -1,
      72,   249,    -1,    45,   117,    41,   120,   133,    -1,    45,
     120,   133,    -1,   148,   244,   250,   133,    -1,     3,    64,
      46,   117,   193,   133,    -1,   150,   177,   250,   133,    -1,
       3,    59,    46,   117,   133,    -1,     3,    59,    46,    27,
      62,   133,    -1,   152,   177,   250,   133,    -1,    74,    59,
      46,   117,   133,    -1,    74,    59,    46,    27,    62,   133,
      -1,   168,   154,   181,   175,   250,   133,    -1,   155,    73,
     117,   258,   156,   133,    -1,    11,   117,    -1,    -1,    26,
     263,   255,    -1,    26,    36,    -1,    26,    93,    -1,    26,
     119,    -1,   168,   158,    -1,    21,   117,    73,   117,   258,
     159,   252,   133,    -1,    -1,   105,   263,   255,    -1,   105,
      93,    -1,   105,    36,    -1,   105,   119,    -1,   168,   161,
     181,   175,   250,   133,    -1,   162,   163,   164,   133,    -1,
     162,   163,   164,    73,   117,   165,   133,    -1,    62,   117,
      -1,    -1,   134,   181,   135,    -1,    -1,    99,   134,   181,
     135,    -1,    -1,   134,   181,   135,    -1,   167,   181,   175,
     250,   133,    -1,    70,   117,    39,   117,   133,    -1,    -1,
      96,    -1,   170,   181,   175,   250,   133,    -1,    27,   117,
      -1,   172,   133,    -1,    95,   173,   250,    -1,    -1,   173,
     174,    -1,   117,   124,   118,   133,    -1,    -1,    -1,    59,
     176,   177,    -1,   178,    -1,    -1,   178,   179,    -1,   180,
     133,   181,   250,   133,    -1,    60,   117,    -1,   182,    -1,
      -1,   182,   183,   133,    -1,   182,   184,   133,    -1,   182,
       1,   133,    -1,   185,    -1,   186,    -1,   187,    -1,   189,
      -1,   190,    -1,   196,    -1,   197,    -1,   198,    -1,   199,
      -1,   200,    -1,   201,    -1,   202,    -1,   203,    -1,   204,
      -1,   205,    -1,   211,    -1,   207,    -1,   226,    -1,   225,
      -1,   213,    -1,   216,    -1,   217,    -1,   219,    -1,   222,
      -1,   224,    -1,   223,    -1,   220,    -1,   221,    -1,   227,
      -1,   228,    -1,   229,    -1,   231,    -1,   235,    -1,   234,
      -1,   236,    -1,   239,    -1,   242,    -1,   243,    -1,   172,
      -1,   218,    -1,   246,    52,   192,   194,   195,    -1,   246,
     101,   192,   194,   195,    -1,   246,     4,   248,    -1,   246,
       4,   134,   246,   135,    99,   246,    52,   117,    65,   117,
     188,    -1,    -1,   114,   134,   253,   135,    -1,   246,    53,
     117,   194,    -1,   246,    54,   192,    -1,   117,    -1,   117,
     134,   253,   135,    -1,   117,    -1,   117,   134,   253,   135,
      -1,    -1,    60,   117,    -1,    -1,    65,   117,    -1,    -1,
     114,   264,    -1,   246,     7,    -1,    55,   134,    44,   121,
     119,   121,   246,   135,    -1,    55,   134,   119,   121,   246,
     135,    -1,    55,   134,   248,   121,   246,   135,    -1,    97,
     134,   119,   121,   246,   135,    -1,    97,   134,   248,   121,
     246,   135,    -1,    28,   134,   246,   135,    -1,    50,   246,
      -1,   246,     8,    -1,   246,   102,    -1,   246,   103,    -1,
     248,   104,   264,    -1,   248,   105,   264,    -1,   206,    -1,
     248,   136,   206,    -1,   264,    -1,    61,   264,    -1,    56,
     264,    -1,   248,   136,   117,   134,   208,   133,   209,   210,
     135,    -1,   246,   136,    47,    -1,   246,   136,    69,    -1,
      -1,   133,   248,   136,    24,    -1,   248,   136,   117,   134,
     246,   133,   116,   135,   212,    -1,    -1,    46,   248,    -1,
      39,   117,    46,   264,   214,   215,   181,   250,    -1,    -1,
      48,    -1,    25,    -1,    23,    -1,    34,    -1,    19,    -1,
      31,    -1,    77,   248,    -1,    77,   248,   106,   248,    -1,
      38,   246,    -1,    38,   204,    -1,    40,   246,    -1,    87,
      41,   264,    88,   264,    89,   264,    -1,    90,   117,    -1,
      82,   117,    -1,    82,   117,    91,    -1,    66,   117,   264,
      -1,    81,    -1,    35,   117,   134,   246,   135,    -1,    15,
     191,    -1,    10,   264,    -1,    43,   264,    92,   181,   230,
     250,    -1,   100,   264,    31,   181,   250,    -1,    -1,    32,
     181,    -1,    98,   246,   232,   250,    -1,   248,   136,    98,
     246,   232,   250,    -1,    98,   134,   246,   135,   232,   250,
      -1,   248,   136,    98,   134,   246,   135,   232,   250,    -1,
     233,    -1,    17,   253,   136,   181,    -1,    68,   136,   181,
      -1,   233,    17,   253,   136,   181,    -1,   233,    68,   136,
     181,    -1,    14,    -1,    22,    -1,    37,    -1,    76,    -1,
      83,   251,    -1,   107,   248,    -1,    79,   246,   237,   250,
      -1,    79,   134,   246,   135,   237,   250,    -1,   238,    -1,
      17,   253,   136,   181,    -1,    68,   136,   181,    -1,   238,
      17,   253,   136,   181,    -1,   238,    68,   136,   181,    -1,
      86,   246,   240,   250,    -1,    86,   134,   246,   135,   240,
     250,    -1,   241,    -1,    17,   253,   136,   181,    -1,    68,
     136,   181,    -1,   241,    17,   253,   136,   181,    -1,   241,
      68,   136,   181,    -1,    20,   181,   250,    -1,    64,   244,
     250,    -1,   119,   245,    -1,   244,   119,   245,    -1,   246,
     118,    -1,   245,   246,   118,    -1,   247,    -1,   248,    -1,
     247,   121,   248,    -1,   247,   248,    -1,   249,   252,    -1,
     117,    -1,   249,   137,   117,    -1,   249,   138,   253,   139,
      -1,    33,    20,    -1,    33,    39,    -1,    33,    43,    -1,
      33,    49,    -1,    33,    59,    -1,    33,    64,    -1,    33,
      79,    -1,    33,    86,    -1,    33,    95,    -1,    33,    42,
      -1,    33,    98,    -1,    33,   100,    -1,    33,   117,    -1,
      33,    -1,    -1,   118,    -1,    -1,   120,    -1,   254,    -1,
      -1,   264,    -1,   264,   111,   264,    -1,   254,   121,   264,
      -1,   254,   121,   264,   111,   264,    -1,   116,    -1,   256,
      -1,   115,   257,    -1,   116,   118,    -1,    -1,   118,    -1,
      29,   259,    -1,    30,    -1,    -1,   129,    -1,   260,    -1,
     117,    -1,   116,    -1,   260,   128,   260,    -1,   260,   129,
     260,    -1,   260,   132,   261,    -1,   134,   260,   135,    -1,
     263,   262,    -1,   116,    -1,   134,   116,   128,   116,   135,
      -1,    -1,   126,    -1,   127,    -1,   116,    -1,    57,    -1,
     256,    -1,    58,    -1,    93,    -1,    36,    -1,     6,    -1,
     119,    -1,   248,    -1,   138,   253,   139,    -1,   264,   126,
     264,    -1,   264,   127,   264,    -1,   264,   129,   264,    -1,
     264,   128,   264,    -1,   264,   132,   264,    -1,   264,     5,
     264,    -1,   264,    67,   264,    -1,    63,   264,    -1,   264,
     265,   264,    -1,   264,   266,   264,    -1,   264,    46,   264,
      -1,   264,   122,   264,    -1,   264,    84,   264,    -1,   126,
     264,    -1,   127,   264,    -1,    78,   134,   248,   121,   256,
     135,    -1,    78,   134,   248,   135,    -1,    85,   134,   253,
     135,    -1,    85,   138,   253,   139,    -1,    71,   134,   253,
     135,    -1,    71,   138,   253,   139,    -1,    94,   134,   253,
     135,    -1,    94,   138,   253,   139,    -1,    51,   134,   253,
     135,    -1,    51,   138,   253,   139,    -1,    16,   134,   253,
     135,    -1,    16,   138,   253,   139,    -1,    18,   134,   253,
     135,    -1,    18,   138,   253,   139,    -1,   117,   134,   264,
     135,    -1,   134,   264,   135,    -1,   124,    -1,   123,    -1,
     125,    -1,   108,    -1,   109,    -1,   110,    -1,    12,    -1,
      13,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   426,   426,   428,   432,   433,   434,   435,   436,   437,
     438,   439,   440,   441,   442,   443,   444,   445,   452,   497,
     501,   506,   514,   518,   523,   531,   540,   552,   583,   591,
     614,   627,   635,   656,   668,   676,   728,   739,   748,   753,
     758,   765,   772,   782,   821,   856,   861,   866,   873,   880,
     890,   931,   940,   953,   962,   965,   973,   976,   984,   987,
     994,  1026,  1040,  1043,  1050,  1080,  1089,  1094,  1114,  1117,
    1125,  1135,  1139,  1138,  1152,  1160,  1163,  1188,  1202,  1211,
    1219,  1222,  1230,  1237,  1245,  1246,  1247,  1248,  1249,  1250,
    1251,  1252,  1253,  1254,  1255,  1256,  1257,  1258,  1259,  1260,
    1261,  1262,  1263,  1264,  1265,  1266,  1267,  1268,  1269,  1270,
    1271,  1272,  1273,  1274,  1275,  1276,  1277,  1278,  1279,  1280,
    1281,  1282,  1283,  1287,  1292,  1333,  1362,  1366,  1417,  1420,
    1427,  1439,  1466,  1471,  1479,  1484,  1493,  1496,  1504,  1507,
    1515,  1518,  1525,  1532,  1536,  1540,  1547,  1551,  1558,  1567,
    1576,  1583,  1590,  1597,  1601,  1608,  1621,  1638,  1648,  1656,
    1667,  1688,  1695,  1703,  1706,  1713,  1734,  1737,  1744,  1766,
    1769,  1773,  1780,  1784,  1788,  1792,  1799,  1803,  1810,  1818,
    1832,  1838,  1844,  1849,  1854,  1862,  1870,  1878,  1889,  1900,
    1906,  1916,  1926,  1929,  1936,  1947,  1959,  1966,  1976,  1983,
    1987,  1991,  1995,  2002,  2006,  2010,  2014,  2018,  2025,  2032,
    2043,  2053,  2060,  2064,  2068,  2072,  2079,  2090,  2100,  2107,
    2111,  2115,  2119,  2126,  2136,  2173,  2182,  2197,  2202,  2211,
    2222,  2226,  2231,  2244,  2264,  2268,  2273,  2288,  2293,  2298,
    2303,  2308,  2313,  2318,  2323,  2328,  2333,  2338,  2343,  2348,
    2353,  2362,  2365,  2373,  2376,  2383,  2388,  2394,  2398,  2402,
    2407,  2415,  2421,  2430,  2434,  2456,  2460,  2481,  2485,  2490,
    2496,  2500,  2507,  2511,  2515,  2519,  2523,  2527,  2534,  2541,
    2545,  2553,  2556,  2560,  2567,  2571,  2575,  2579,  2583,  2587,
    2591,  2595,  2599,  2603,  2607,  2612,  2617,  2622,  2627,  2632,
    2637,  2642,  2646,  2651,  2656,  2661,  2666,  2671,  2675,  2679,
    2683,  2687,  2694,  2698,  2705,  2709,  2716,  2720,  2727,  2731,
    2738,  2742,  2749,  2753,  2764,  2771,  2775,  2779,  2783,  2787,
    2791,  2798,  2802
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ADD_TOK", "ALIASES_TOK", "AND_TOK",
  "ANY_TOK", "AREALIKE_TOK", "ARETHESAME_TOK", "ARRAY_TOK", "ASSERT_TOK",
  "ATOM_TOK", "BEQ_TOK", "BNE_TOK", "BREAK_TOK", "CALL_TOK", "CARD_TOK",
  "CASE_TOK", "CHOICE_TOK", "CHECK_TOK", "CONDITIONAL_TOK", "CONSTANT_TOK",
  "CONTINUE_TOK", "CREATE_TOK", "DATA_TOK", "DECREASING_TOK",
  "DEFAULT_TOK", "DEFINITION_TOK", "DER_TOK", "DIMENSION_TOK",
  "DIMENSIONLESS_TOK", "DO_TOK", "ELSE_TOK", "END_TOK", "EXPECT_TOK",
  "EXTERNAL_TOK", "FALSE_TOK", "FALLTHRU_TOK", "FIX_TOK", "FOR_TOK",
  "FREE_TOK", "FROM_TOK", "GLOBAL_TOK", "IF_TOK", "IGNORE_TOK",
  "IMPORT_TOK", "IN_TOK", "INPUT_TOK", "INCREASING_TOK", "INTERACTIVE_TOK",
  "INDEPENDENT_TOK", "INTERSECTION_TOK", "ISA_TOK", "_IS_T",
  "ISREFINEDTO_TOK", "LINK_TOK", "MAXIMIZE_TOK", "MAXINTEGER_TOK",
  "MAXREAL_TOK", "METHODS_TOK", "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK",
  "NOT_TOK", "NOTES_TOK", "OF_TOK", "OPTION_TOK", "OR_TOK",
  "OTHERWISE_TOK", "OUTPUT_TOK", "PATCH_TOK", "PROD_TOK", "PROVIDE_TOK",
  "REFINES_TOK", "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK",
  "SATISFIED_TOK", "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK", "SOLVER_TOK",
  "STOP_TOK", "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK", "INTEGRATE_TOK",
  "TO_TOK", "STEPS_TOK", "SUBSOLVER_TOK", "ISINTEGRATOR_TOK", "THEN_TOK",
  "TRUE_TOK", "UNION_TOK", "UNITS_TOK", "UNIVERSAL_TOK", "UNLINK_TOK",
  "WHEN_TOK", "WHERE_TOK", "WHILE_TOK", "WILLBE_TOK", "WILLBETHESAME_TOK",
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
  "integrate_statement", "subsolver_statement", "solver_statement",
  "option_statement", "solve_statement", "external_statement",
  "call_statement", "assert_statement", "if_statement", "while_statement",
  "optional_else", "when_statement", "whenlist", "whenlistf",
  "flow_statement", "use_statement", "select_statement", "selectlist",
  "selectlistf", "switch_statement", "switchlist", "switchlistf",
  "conditional_statement", "notes_statement", "notes_body", "noteslist",
  "fvarlist", "varlist", "fname", "name", "end", "optional_bracedtext",
  "optional_notes", "set", "setexprlist", "number", "realnumber",
  "opunits", "dims", "dimensions", "dimexpr", "fraction", "fractail",
  "optional_sign", "expr", "relop", "logrelop", 0
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
     375,    44,   124,    60,    61,    62,    43,    45,    47,    42,
     376,   377,    94,    59,    40,    41,    58,    46,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   140,   141,   141,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   143,   144,
     144,   144,   145,   145,   145,   146,   146,   147,   148,   149,
     150,   150,   151,   152,   152,   153,   154,   155,   156,   156,
     156,   156,   156,   157,   158,   159,   159,   159,   159,   159,
     160,   161,   161,   162,   163,   163,   164,   164,   165,   165,
     166,   167,   168,   168,   169,   170,   171,   172,   173,   173,
     174,   175,   176,   175,   177,   178,   178,   179,   180,   181,
     182,   182,   182,   182,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   184,   185,   186,   187,   187,   188,   188,
     189,   190,   191,   191,   192,   192,   193,   193,   194,   194,
     195,   195,   196,   197,   197,   197,   198,   198,   199,   200,
     201,   202,   203,   204,   204,   205,   205,   206,   206,   206,
     207,   208,   209,   210,   210,   211,   212,   212,   213,   214,
     214,   214,   215,   215,   215,   215,   216,   216,   217,   218,
     219,   220,   221,   222,   222,   223,   224,   225,   226,   227,
     228,   229,   230,   230,   231,   231,   231,   231,   232,   233,
     233,   233,   233,   234,   234,   234,   234,   234,   235,   236,
     236,   237,   238,   238,   238,   238,   239,   239,   240,   241,
     241,   241,   241,   242,   243,   244,   244,   245,   245,   246,
     247,   247,   247,   248,   249,   249,   249,   250,   250,   250,
     250,   250,   250,   250,   250,   250,   250,   250,   250,   250,
     250,   251,   251,   252,   252,   253,   253,   254,   254,   254,
     254,   255,   255,   256,   256,   257,   257,   258,   258,   258,
     259,   259,   260,   260,   260,   260,   260,   260,   261,   262,
     262,   263,   263,   263,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   265,   265,   265,   265,   265,
     265,   266,   266
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
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
       1,     1,     1,     1,     5,     5,     3,    12,     0,     4,
       4,     3,     1,     4,     1,     4,     0,     2,     0,     2,
       0,     2,     2,     8,     6,     6,     6,     6,     4,     2,
       2,     2,     2,     3,     3,     1,     3,     1,     2,     2,
       9,     3,     3,     0,     4,     9,     0,     2,     8,     0,
       1,     1,     1,     1,     1,     1,     2,     4,     2,     2,
       2,     7,     2,     2,     3,     3,     1,     5,     2,     2,
       6,     5,     0,     2,     4,     6,     6,     8,     1,     4,
       3,     5,     4,     1,     1,     1,     1,     2,     2,     4,
       6,     1,     4,     3,     5,     4,     4,     6,     1,     4,
       3,     5,     4,     3,     3,     2,     3,     2,     3,     1,
       1,     3,     2,     2,     1,     3,     4,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       1,     0,     1,     0,     1,     1,     0,     1,     3,     3,
       5,     1,     1,     2,     2,     0,     1,     2,     1,     0,
       1,     1,     1,     1,     3,     3,     3,     3,     2,     1,
       5,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     3,     3,     3,     3,     3,     2,     2,     6,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     3,     1,     1,     1,     1,     1,
       1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     1,    17,     0,     0,     0,     0,     0,     0,
       0,     0,    68,    63,     3,    16,     4,     5,     6,     9,
       0,     7,    75,     8,    75,    11,    10,    12,    14,    80,
       0,    13,    80,    15,     0,     0,     0,    65,    80,     0,
       0,     0,   234,     0,    24,     0,     0,    21,     0,     0,
       0,     0,    74,     0,    71,     0,     0,     0,     0,    80,
       0,    43,    80,    54,    71,    66,     0,     0,     0,     0,
      26,     0,    22,    23,     0,   256,     0,    19,    20,   250,
       0,    69,    67,   225,     0,   229,   230,   253,     0,     0,
       0,     0,    76,     0,     0,    72,     0,     0,   290,     0,
     203,     0,     0,     0,    80,   204,     0,     0,   289,   205,
       0,     0,     0,     0,     0,     0,     0,     0,   285,   287,
       0,     0,     0,     0,     0,   206,     0,     0,     0,   186,
       0,   251,     0,     0,     0,     0,   288,     0,     0,     0,
       0,     0,   265,   284,   234,   291,     0,     0,     0,   256,
     122,     0,     0,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,   155,   100,
      99,   103,   104,   105,   123,   106,   110,   111,   107,   109,
     108,   102,   101,   112,   113,   114,   115,   117,   116,   118,
     119,   120,   121,     0,   292,   286,   157,    37,     0,    53,
      71,     0,    71,    80,    56,     0,     0,     0,   136,     0,
       0,     0,   235,   292,     0,   255,   257,     0,     0,   237,
     238,   246,   239,   240,   241,   242,   243,   244,   245,   247,
     248,   249,     0,     0,   227,     0,   232,   254,   233,   226,
      27,    29,    78,    80,    32,    75,     0,    83,   189,   132,
     188,   256,   256,   256,   256,     0,     0,     0,   179,   178,
     230,     0,   180,     0,   149,   256,   256,     0,   159,   158,
     301,     0,     0,   256,   256,   176,     0,     0,     0,   183,
     252,   207,   256,   256,     0,     0,     0,   182,   256,   256,
       0,     0,     0,     0,   208,   266,   263,   264,     0,   307,
     308,     0,     0,    81,    82,     0,   142,   150,     0,     0,
       0,     0,   151,   152,     0,     0,     0,     0,   331,   332,
       0,     0,     0,   328,   329,   330,     0,   326,   325,   327,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   269,
       0,     0,     0,     0,     0,     0,    30,     0,     0,    18,
      25,    61,   236,     0,     0,     0,    33,     0,   228,   231,
       0,    73,    60,   256,     0,     0,     0,     0,   223,     0,
       0,     0,    80,     0,     0,     0,     0,     0,   224,   185,
       0,     0,     0,     0,     0,   256,     0,     0,   211,   184,
       0,     0,     0,   256,     0,     0,   218,     0,     0,     0,
       0,     0,     0,   256,     0,     0,   198,    80,     0,   324,
     293,     0,   126,   134,   138,   138,   131,   138,   153,   154,
       0,   234,   156,   299,   304,   300,   306,   305,   294,   295,
     297,   296,   298,   302,   303,   269,     0,     0,   268,    38,
       0,    55,    80,     0,    51,    64,    31,   137,    28,   259,
     258,    34,    70,     0,     0,   319,   320,   321,   322,   148,
       0,   169,   192,   317,   318,     0,     0,     0,   313,   314,
     177,     0,   310,     0,     0,    80,   209,   256,     0,   311,
     312,     0,     0,    80,   216,   256,     0,     0,   315,   316,
       0,     0,     0,     0,    80,   194,   256,     0,     0,   323,
       0,   256,     0,   140,   130,   140,     0,     0,     0,    45,
      35,   273,   272,   270,     0,   267,   271,   281,     0,    50,
       0,    58,     0,    77,   133,   187,   171,   170,     0,    80,
       0,     0,     0,     0,     0,     0,     0,    80,   213,     0,
      80,     0,    80,   220,     0,    80,     0,     0,     0,     0,
      80,   200,     0,    80,   191,     0,     0,   139,     0,   124,
     125,     0,     0,     0,     0,   292,   281,   253,     0,     0,
       0,   281,    40,    41,    42,   282,   283,     0,    36,    57,
      80,     0,   260,   174,   172,   175,   173,    80,   193,   190,
       0,   144,   145,   309,   210,   212,    80,   215,   217,   219,
      80,   222,     0,   146,   147,   196,   199,    80,   202,     0,
     135,   141,     0,   195,     0,     0,     0,    48,    47,    49,
       0,     0,   277,   274,   275,   276,     0,   261,    39,   262,
       0,    52,     0,     0,   214,   221,   181,   201,     0,     0,
     163,     0,     0,   161,    46,    44,   279,     0,   278,    59,
     168,   143,     0,   197,     0,     0,     0,   166,     0,     0,
       0,   160,   162,     0,   165,     0,     0,     0,   167,     0,
     128,   164,   280,     0,   127,   256,     0,   129
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    59,    60,   518,    26,    61,   567,
      27,    62,    63,   204,   343,   581,    28,    29,    30,    31,
      32,    33,    34,    48,    81,    96,   245,    51,    52,    92,
      93,    54,    55,   151,   152,   153,   154,   155,   674,   156,
     157,   250,   414,   348,   503,   559,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   563,   640,
     655,   170,   664,   171,   528,   587,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     530,   186,   405,   406,   187,   188,   189,   387,   388,   190,
     395,   396,   191,   192,    50,    83,    84,    85,   213,    87,
      82,   281,   238,   214,   215,   628,   195,   296,   439,   515,
     516,   625,   648,   577,   216,   335,   336
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -498
static const yytype_int16 yypact[] =
{
    -498,   537,  -498,  -498,    -2,   -98,   -87,    29,   -57,    49,
      65,    94,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
     -43,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
      19,  -498,  -498,  -498,   -34,    58,    69,  -498,  -498,   103,
      24,   148,  -498,    79,   -82,   158,    87,   -50,   -12,   109,
     -19,   200,   178,   200,   191,   649,   140,   144,   149,  -498,
     185,  -498,  -498,   131,   191,  -498,   -15,   151,   200,   152,
    -498,   154,  -498,  -498,   159,   734,   -14,  -498,  -498,   220,
     153,  -498,  -498,   109,   146,   -68,  -498,   -10,   109,   142,
     145,   163,  -498,   150,   155,  -498,   200,   156,  -498,   734,
    -498,   165,    40,    45,  -498,  -498,   162,   168,  -498,  -498,
     109,   169,   109,   734,   109,    72,   164,   734,  -498,  -498,
     734,   734,   -43,   170,    83,  -498,   109,   166,    -5,  -498,
     173,   175,    85,     9,   256,   184,  -498,    98,   174,    17,
     734,   109,   186,   192,   182,  -498,   734,   734,   734,   734,
    -498,   176,   189,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,   128,    16,  -498,  1129,  -498,   229,  -498,
     191,   202,   191,  -498,   204,   200,   255,   190,   264,   201,
     205,   206,  -498,  -498,   197,   222,   870,   282,   215,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,   233,   236,  -498,   109,  -498,  -498,  -498,   109,
    -498,  -498,  -498,  -498,  -498,  -498,   223,  -498,  1129,   221,
    -498,   734,   734,   734,   734,   200,   109,   227,  -498,  -498,
      63,   311,  -498,   954,  -498,   734,   734,   -28,  1129,  1129,
    -498,   -19,   734,   734,   734,   258,   109,   109,    10,   275,
    -498,  -498,   734,   734,   109,    14,   734,  -498,   734,   734,
     -91,   109,    18,  1001,  -498,  -498,  -498,  -498,   734,   237,
     237,   768,   231,  -498,  -498,    21,  -498,  -498,   251,   259,
     251,   251,  -498,  -498,   734,   734,   289,   734,  -498,  -498,
     734,   734,   734,  -498,  -498,  -498,   734,  -498,  -498,  -498,
     734,   734,   734,   734,   734,   734,   734,   261,   200,   214,
     200,   240,   238,   -62,   246,   247,  -498,   268,   248,  -498,
    -498,  -498,  -498,   734,   734,   253,  -498,   257,  -498,  -498,
     200,  -498,  -498,   734,   260,   249,   262,   252,  -498,   263,
     109,   734,  -498,   265,   254,   271,   280,   281,  -498,  1129,
     274,   272,   109,   -77,   278,   734,   283,   200,    26,  -498,
     279,   285,   287,   734,   290,   200,    28,  1028,   295,   292,
     312,   313,   297,   734,   299,   200,    30,  -498,   796,  -498,
    -498,   109,  -498,   276,   331,   331,  -498,   331,  1129,  1129,
      31,   302,  -498,   927,   927,   927,   203,   203,    71,    71,
     237,   237,   237,    66,   353,   214,   304,   -75,  -498,   412,
     306,  -498,  -498,   323,  -498,  -498,  -498,  -498,  -498,  1094,
    1129,  -498,  -498,   310,   309,  -498,  -498,  -498,  -498,  -498,
     316,   902,   414,  -498,  -498,   328,   109,   109,  -498,  -498,
    -498,   130,  -498,    10,   322,  -498,  -498,   734,   324,  -498,
    -498,    14,   329,  -498,  -498,   734,   330,   734,  -498,  -498,
     109,   109,    18,   332,  -498,  -498,   734,   333,   200,  -498,
     335,   734,   350,   357,  -498,   357,   109,    18,   734,   367,
    -498,  -498,  -498,  -498,    -3,  -498,    73,   -18,   342,  -498,
     348,   352,   734,  -498,  -498,  -498,  -498,  -498,   141,  -498,
     200,   366,   358,   359,   192,   361,   200,  -498,  -498,   356,
    -498,   200,  -498,  -498,   362,  -498,  1160,   365,   369,   200,
    -498,  -498,   370,  -498,  -498,   389,   373,  -498,   734,  -498,
    -498,   374,   200,   364,   101,   -69,    70,   381,    56,    -3,
      -3,   121,  -498,  -498,  -498,  -498,  -498,   137,  -498,  -498,
    -498,   372,  1129,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
     109,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,   734,  -498,  -498,  -498,  -498,  -498,  -498,   109,
    -498,  1129,    18,  -498,   109,   386,   464,  -498,  -498,  -498,
     137,   380,  -498,   382,   382,  -498,  -101,   192,  -498,  -498,
     383,  -498,   200,   384,  -498,  -498,  1129,  -498,   465,   200,
     388,   390,   387,  -498,  -498,  -498,  -498,   408,  -498,  -498,
    -498,  -498,   410,  -498,   109,   393,   456,   483,   402,   466,
     396,  -498,  -498,   109,  -498,   417,   418,   510,  -498,   401,
     425,  -498,  -498,   409,  -498,   734,   407,  -498
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,   489,  -498,  -498,   -55,  -498,   -21,  -498,  -498,
    -498,   -30,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,   -56,  -498,  -292,    41,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,   435,  -498,   235,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -482,  -498,  -498,  -498,  -498,    74,  -498,  -498,
      68,  -498,  -498,  -498,   430,   467,   -17,  -498,   -49,   147,
     -46,  -498,   -13,  -112,  -498,   -67,  -470,  -498,   124,  -498,
    -497,  -498,  -498,  -486,   308,  -498,  -498
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -231
static const yytype_int16 yytable[] =
{
      86,   535,    64,    53,    89,    90,   194,    94,    68,   205,
     549,   443,   206,   217,    79,   646,   375,   568,   572,    37,
    -230,    79,   209,  -230,  -230,   562,    42,   385,   400,   200,
      56,   393,   202,   647,    86,   403,   236,   302,   193,    86,
      57,   511,   512,   477,   471,   485,    38,   496,  -230,    42,
     246,    73,  -230,   235,   513,    74,    75,    35,   472,   514,
      41,   260,    36,    86,  -230,    86,   233,  -230,  -230,  -230,
    -230,   444,   623,   624,   255,   573,    49,   275,   386,    86,
     620,    58,   394,    78,    86,   626,   404,    74,    75,    42,
      86,   376,   294,   259,   478,   262,   486,   264,   497,    65,
      88,   574,   207,   218,    66,    80,   617,   629,   575,   576,
     237,   278,    42,   511,   512,    67,   285,  -230,  -230,  -230,
     314,   315,   292,   504,    45,   505,    42,    74,    75,   277,
     639,   514,   305,  -230,    42,   306,   307,  -230,    42,   364,
     365,   366,   367,   284,    69,   338,    39,   340,    42,    40,
     629,   291,   316,   373,   374,   411,    44,    70,    47,   344,
     583,   380,   381,   618,   584,   506,    42,   314,   315,    43,
     390,   391,   585,   341,   251,   586,   398,   399,   252,   253,
     308,   309,   310,   254,   569,   570,   359,    71,   571,   619,
      86,   622,   330,   331,   332,   333,   575,   576,   334,   332,
     333,   569,   570,   334,    76,   571,   265,    86,   317,   368,
     266,    42,    72,   360,    46,   318,   319,   273,   377,   282,
      77,   274,   233,   283,   361,   378,    42,   383,    86,   311,
     312,   313,   288,    79,   615,    86,   289,   616,    91,   369,
     219,   401,    86,   437,   438,   142,   534,   575,   576,   320,
      95,   454,   142,   627,   416,   417,   412,   197,   201,   220,
     384,   198,   221,   222,   234,   203,   199,   392,   208,   223,
     321,   211,   210,   474,   402,   240,   212,   232,   241,   224,
     242,   482,   249,   243,   225,   257,   261,   272,   244,   247,
     279,   493,   436,   280,   440,    98,   256,   286,   267,   226,
     276,   287,   337,   342,   295,   102,   227,   103,   290,   303,
     297,   323,   324,   325,   453,   228,   298,   345,   229,   339,
     230,    86,   304,   346,   347,   108,   327,   328,   329,   330,
     331,   332,   333,   470,   349,   334,   352,   231,   350,   351,
     115,   476,   462,   353,   355,   117,   118,   119,   356,   484,
     120,   357,   121,   460,   358,   363,   362,   371,   317,   495,
     124,   370,    86,   196,   382,   539,   389,   127,   413,   334,
     410,    86,   442,   544,   132,   441,   415,   498,   435,   445,
     446,   448,   136,   137,   552,   447,   451,   420,   456,   556,
     452,   458,   465,   464,   500,   455,   502,   457,   459,   320,
     463,   466,   467,   507,   142,   143,   421,   248,   145,   468,
     501,   469,   520,   473,   479,   146,   147,    86,    86,   475,
     321,   263,   481,   148,   480,   268,   483,   149,   269,   270,
     488,   489,   492,   490,   491,   494,   508,   510,   517,   519,
     521,    86,    86,   523,   524,   538,   529,   531,   293,   532,
     533,   525,   554,   543,   299,   300,   301,    86,   537,   565,
     540,   323,   324,   325,   551,   542,   545,   557,   550,   553,
     555,   558,   566,   547,   548,   578,   327,   328,   329,   330,
     331,   332,   333,   579,   589,   334,   580,   590,   609,   561,
     594,   564,   596,   591,   592,   598,   593,   614,   600,   588,
     603,   237,   642,   605,   604,   631,   607,   595,   610,   612,
     597,   643,   599,   645,   571,   601,   613,   652,   649,   651,
     606,   654,   657,   608,   658,   662,   656,   659,   661,   663,
     665,   666,   667,   669,   671,   670,   672,     2,     3,   673,
       4,    86,   677,   675,   150,   258,   560,   536,   -62,   541,
     630,   422,   271,   644,   621,   239,     0,   632,   -62,   509,
      86,     0,     0,   676,     5,    86,   634,     0,     0,     0,
     635,     0,     0,   633,     0,     0,     0,   637,     0,     6,
     379,     0,     7,     0,     0,     0,   650,     0,     0,     0,
       0,     0,   638,   653,   397,     0,     0,   641,     0,   -62,
       0,     0,     0,     0,     0,   660,   408,     8,     0,     9,
       0,    10,    11,     0,   668,     0,     0,     0,     0,     0,
       0,     0,   418,   419,   196,   423,     0,     0,   424,   425,
     426,     0,    12,    13,   427,     0,     0,     0,   428,   429,
     430,   431,   432,   433,   434,     0,     0,     0,     0,     0,
      97,     0,     0,     0,     0,    98,     0,     0,     0,    99,
       0,   449,   450,   100,   101,   102,   -79,   103,     0,   104,
       0,   105,     0,     0,     0,     0,     0,   106,     0,   461,
       0,   -79,   -79,     0,   107,   108,   109,   110,   111,   112,
       0,     0,   113,     0,     0,     0,     0,     0,     0,   114,
     115,     0,     0,     0,   116,   117,   118,   119,   -79,     0,
     120,     0,   121,   122,     0,   123,     0,   -79,     0,     0,
     124,     0,     0,     0,     0,   125,   126,   127,   128,     0,
     129,   130,   131,     0,   132,   133,   134,     0,     0,   135,
      98,     0,   136,   137,    12,     0,   138,   139,     0,   140,
     102,     0,   103,     0,     0,     0,   141,     0,     0,     0,
       0,     0,     0,     0,   142,   143,   144,     0,   145,     0,
     108,     0,     0,   317,     0,   146,   147,     0,     0,     0,
     318,   319,     0,   148,   -79,   115,     0,   149,     0,     0,
       0,   118,   119,     0,     0,   546,     0,   121,     0,     0,
       0,   317,     0,     0,     0,   124,     0,     0,   318,   319,
       0,     0,   127,     0,   320,     0,   408,     0,     0,   132,
       0,     0,     0,     0,     0,     0,     0,   136,   137,     0,
     582,     0,     0,     0,     0,   321,     0,     0,     0,     0,
       0,     0,   320,     0,     0,     0,     0,     0,     0,   142,
     143,   144,   322,   145,     0,     0,     0,     0,     0,     0,
     146,   147,     0,   321,     0,     0,   611,     0,   148,     0,
       0,     0,   149,     0,     0,   317,   323,   324,   325,     0,
     322,     0,   318,   319,     0,     0,     0,     0,     0,     0,
     326,   327,   328,   329,   330,   331,   332,   333,     0,     0,
     334,     0,     0,   409,   323,   324,   325,   317,     0,     0,
     636,     0,     0,     0,   318,   319,   320,     0,   326,   327,
     328,   329,   330,   331,   332,   333,     0,   526,   334,     0,
       0,   499,     0,     0,     0,     0,     0,   321,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   320,     0,
     527,     0,     0,     0,   322,     0,     0,     0,     0,   317,
       0,     0,     0,     0,     0,     0,   318,   319,     0,   321,
       0,     0,     0,     0,     0,     0,     0,     0,   323,   324,
     325,   354,     0,     0,     0,     0,   322,     0,     0,     0,
       0,     0,   326,   327,   328,   329,   330,   331,   332,   333,
     320,     0,   334,     0,     0,     0,   317,     0,     0,     0,
     323,   324,   325,   318,   319,     0,     0,     0,     0,     0,
       0,   321,     0,     0,   326,   327,   328,   329,   330,   331,
     332,   333,   407,   317,   334,   323,   324,   325,   322,     0,
     318,   319,     0,     0,     0,     0,   372,   320,     0,     0,
     327,   328,   329,   330,   331,   332,   333,     0,     0,   334,
       0,     0,   323,   324,   325,     0,     0,     0,   321,     0,
       0,     0,     0,     0,   320,     0,   326,   327,   328,   329,
     330,   331,   332,   333,     0,   322,   334,     0,     0,     0,
       0,     0,     0,     0,     0,   321,     0,     0,     0,   317,
       0,     0,     0,     0,     0,     0,   318,   319,     0,   323,
     324,   325,   322,     0,     0,     0,   487,     0,     0,     0,
       0,     0,     0,   326,   327,   328,   329,   330,   331,   332,
     333,     0,     0,   334,   317,     0,   323,   324,   325,     0,
     320,   318,   319,     0,     0,     0,     0,     0,     0,     0,
     326,   327,   328,   329,   330,   331,   332,   333,     0,     0,
     334,   321,     0,     0,     0,   317,     0,     0,     0,     0,
       0,     0,   318,   319,     0,   320,     0,     0,   322,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   321,     0,     0,     0,
       0,     0,   323,   324,   325,   522,   320,     0,     0,     0,
       0,     0,     0,   322,     0,     0,   326,   327,   328,   329,
     330,   331,   332,   333,     0,     0,   334,   321,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   323,   324,   325,
       0,     0,     0,     0,   322,     0,     0,     0,     0,   602,
       0,   326,   327,   328,   329,   330,   331,   332,   333,     0,
       0,   334,     0,     0,     0,     0,     0,     0,   323,   324,
     325,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   326,   327,   328,   329,   330,   331,   332,   333,
       0,     0,   334
};

static const yytype_int16 yycheck[] =
{
      49,   471,    32,    24,    50,    51,    55,    53,    38,    64,
     492,    73,    27,    27,    33,   116,    44,   514,    36,   117,
       4,    33,    68,     7,     8,   507,   117,    17,   119,    59,
      11,    17,    62,   134,    83,    17,    85,   149,    55,    88,
      21,   116,   117,    17,   121,    17,   133,    17,   117,   117,
      96,   133,   121,   121,   129,   137,   138,    59,   135,   134,
     117,   110,    64,   112,   133,   114,    83,   136,    52,    53,
      54,   133,   569,   570,   104,    93,   119,   126,    68,   128,
     566,    62,    68,   133,   133,   571,    68,   137,   138,   117,
     139,   119,   141,   110,    68,   112,    68,   114,    68,   133,
     119,   119,   117,   117,    46,   117,    36,   577,   126,   127,
     120,   128,   117,   116,   117,    46,   133,   101,   102,   103,
     104,   105,   139,   415,    59,   417,   117,   137,   138,   134,
     612,   134,     4,   117,   117,     7,     8,   121,   117,   251,
     252,   253,   254,   134,    41,   200,   117,   202,   117,   120,
     620,   134,   136,   265,   266,   134,     9,   133,    11,   205,
      19,   273,   274,    93,    23,   134,   117,   104,   105,   120,
     282,   283,    31,   203,   134,    34,   288,   289,   138,   134,
      52,    53,    54,   138,   128,   129,   235,    39,   132,   119,
     239,   135,   126,   127,   128,   129,   126,   127,   132,   128,
     129,   128,   129,   132,    46,   132,   134,   256,     5,   255,
     138,   117,   133,   243,   120,    12,    13,   134,   267,   134,
     133,   138,   239,   138,   245,   271,   117,   276,   277,   101,
     102,   103,   134,    33,   133,   284,   138,   136,    60,   256,
      20,   290,   291,    29,    30,   115,   116,   126,   127,    46,
      59,   363,   115,   116,   310,   311,   305,   117,    73,    39,
     277,   117,    42,    43,   118,   134,   117,   284,   117,    49,
      67,   117,   120,   385,   291,   133,   117,   124,   133,    59,
     117,   393,   117,   133,    64,   117,   117,   117,   133,   133,
     117,   403,   338,   118,   340,     6,   134,    41,   134,    79,
     134,   117,    73,    99,   118,    16,    86,    18,   134,   133,
     118,   108,   109,   110,   360,    95,   134,    62,    98,   117,
     100,   370,   133,   133,    60,    36,   123,   124,   125,   126,
     127,   128,   129,   382,   133,   132,   139,   117,   133,   133,
      51,   387,   372,   121,    62,    56,    57,    58,   133,   395,
      61,   118,    63,   370,   118,   134,   133,    46,     5,   405,
      71,   134,   411,    55,   106,   477,    91,    78,   117,   132,
     139,   420,   134,   485,    85,   135,   117,   407,   117,   133,
     133,   133,    93,    94,   496,   117,   133,    98,   139,   501,
     133,   139,   121,   139,   411,   135,    65,   135,   135,    46,
     135,   121,   121,   420,   115,   116,   117,    99,   119,   135,
     134,   139,   442,   135,   135,   126,   127,   466,   467,   136,
      67,   113,   135,   134,   139,   117,   136,   138,   120,   121,
     135,   139,   135,   121,   121,   136,   134,   133,    26,   133,
     117,   490,   491,   133,   135,   475,    32,   119,   140,   466,
     467,   135,   498,   483,   146,   147,   148,   506,   136,   508,
     136,   108,   109,   110,   494,   136,   136,   117,   136,   136,
     135,   114,   105,   490,   491,   133,   123,   124,   125,   126,
     127,   128,   129,   135,   530,   132,   134,   121,    99,   506,
     536,   508,   136,   135,   135,   541,   135,   133,   136,   529,
     135,   120,   116,   549,   135,   133,   136,   537,   135,   135,
     540,    47,   542,   133,   132,   545,   562,    52,   135,   135,
     550,   133,   135,   553,   116,    69,   136,   117,   135,    46,
     128,    65,   136,   116,    24,   117,   135,     0,     1,   114,
       3,   590,   135,   134,    55,   110,   505,   473,    11,   481,
     580,   316,   122,   620,   567,    88,    -1,   587,    21,   435,
     609,    -1,    -1,   675,    27,   614,   596,    -1,    -1,    -1,
     600,    -1,    -1,   590,    -1,    -1,    -1,   607,    -1,    42,
     272,    -1,    45,    -1,    -1,    -1,   632,    -1,    -1,    -1,
      -1,    -1,   609,   639,   286,    -1,    -1,   614,    -1,    62,
      -1,    -1,    -1,    -1,    -1,   654,   298,    70,    -1,    72,
      -1,    74,    75,    -1,   663,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   314,   315,   316,   317,    -1,    -1,   320,   321,
     322,    -1,    95,    96,   326,    -1,    -1,    -1,   330,   331,
     332,   333,   334,   335,   336,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,     6,    -1,    -1,    -1,    10,
      -1,   353,   354,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    -1,    -1,    -1,    -1,    -1,    28,    -1,   371,
      -1,    32,    33,    -1,    35,    36,    37,    38,    39,    40,
      -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,    50,
      51,    -1,    -1,    -1,    55,    56,    57,    58,    59,    -1,
      61,    -1,    63,    64,    -1,    66,    -1,    68,    -1,    -1,
      71,    -1,    -1,    -1,    -1,    76,    77,    78,    79,    -1,
      81,    82,    83,    -1,    85,    86,    87,    -1,    -1,    90,
       6,    -1,    93,    94,    95,    -1,    97,    98,    -1,   100,
      16,    -1,    18,    -1,    -1,    -1,   107,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   115,   116,   117,    -1,   119,    -1,
      36,    -1,    -1,     5,    -1,   126,   127,    -1,    -1,    -1,
      12,    13,    -1,   134,   135,    51,    -1,   138,    -1,    -1,
      -1,    57,    58,    -1,    -1,   487,    -1,    63,    -1,    -1,
      -1,     5,    -1,    -1,    -1,    71,    -1,    -1,    12,    13,
      -1,    -1,    78,    -1,    46,    -1,   508,    -1,    -1,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    -1,
     522,    -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,    -1,
      -1,    -1,    46,    -1,    -1,    -1,    -1,    -1,    -1,   115,
     116,   117,    84,   119,    -1,    -1,    -1,    -1,    -1,    -1,
     126,   127,    -1,    67,    -1,    -1,   558,    -1,   134,    -1,
      -1,    -1,   138,    -1,    -1,     5,   108,   109,   110,    -1,
      84,    -1,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
     122,   123,   124,   125,   126,   127,   128,   129,    -1,    -1,
     132,    -1,    -1,   135,   108,   109,   110,     5,    -1,    -1,
     602,    -1,    -1,    -1,    12,    13,    46,    -1,   122,   123,
     124,   125,   126,   127,   128,   129,    -1,    25,   132,    -1,
      -1,   135,    -1,    -1,    -1,    -1,    -1,    67,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,    -1,
      48,    -1,    -1,    -1,    84,    -1,    -1,    -1,    -1,     5,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    -1,    67,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
     110,   111,    -1,    -1,    -1,    -1,    84,    -1,    -1,    -1,
      -1,    -1,   122,   123,   124,   125,   126,   127,   128,   129,
      46,    -1,   132,    -1,    -1,    -1,     5,    -1,    -1,    -1,
     108,   109,   110,    12,    13,    -1,    -1,    -1,    -1,    -1,
      -1,    67,    -1,    -1,   122,   123,   124,   125,   126,   127,
     128,   129,    31,     5,   132,   108,   109,   110,    84,    -1,
      12,    13,    -1,    -1,    -1,    -1,    92,    46,    -1,    -1,
     123,   124,   125,   126,   127,   128,   129,    -1,    -1,   132,
      -1,    -1,   108,   109,   110,    -1,    -1,    -1,    67,    -1,
      -1,    -1,    -1,    -1,    46,    -1,   122,   123,   124,   125,
     126,   127,   128,   129,    -1,    84,   132,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,     5,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    -1,   108,
     109,   110,    84,    -1,    -1,    -1,    88,    -1,    -1,    -1,
      -1,    -1,    -1,   122,   123,   124,   125,   126,   127,   128,
     129,    -1,    -1,   132,     5,    -1,   108,   109,   110,    -1,
      46,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     122,   123,   124,   125,   126,   127,   128,   129,    -1,    -1,
     132,    67,    -1,    -1,    -1,     5,    -1,    -1,    -1,    -1,
      -1,    -1,    12,    13,    -1,    46,    -1,    -1,    84,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,
      -1,    -1,   108,   109,   110,   111,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    -1,    -1,   122,   123,   124,   125,
     126,   127,   128,   129,    -1,    -1,   132,    67,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,   110,
      -1,    -1,    -1,    -1,    84,    -1,    -1,    -1,    -1,    89,
      -1,   122,   123,   124,   125,   126,   127,   128,   129,    -1,
      -1,   132,    -1,    -1,    -1,    -1,    -1,    -1,   108,   109,
     110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   122,   123,   124,   125,   126,   127,   128,   129,
      -1,    -1,   132
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   141,     0,     1,     3,    27,    42,    45,    70,    72,
      74,    75,    95,    96,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   157,   160,   166,   167,
     168,   169,   170,   171,   172,    59,    64,   117,   133,   117,
     120,   117,   117,   120,   249,    59,   120,   249,   173,   119,
     244,   177,   178,   177,   181,   182,    11,    21,    62,   154,
     155,   158,   161,   162,   181,   133,    46,    46,   181,    41,
     133,    39,   133,   133,   137,   138,    46,   133,   133,    33,
     117,   174,   250,   245,   246,   247,   248,   249,   119,   250,
     250,    60,   179,   180,   250,    59,   175,     1,     6,    10,
      14,    15,    16,    18,    20,    22,    28,    35,    36,    37,
      38,    39,    40,    43,    50,    51,    55,    56,    57,    58,
      61,    63,    64,    66,    71,    76,    77,    78,    79,    81,
      82,    83,    85,    86,    87,    90,    93,    94,    97,    98,
     100,   107,   115,   116,   117,   119,   126,   127,   134,   138,
     172,   183,   184,   185,   186,   187,   189,   190,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
     211,   213,   216,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   231,   234,   235,   236,
     239,   242,   243,   246,   248,   256,   264,   117,   117,   117,
     181,    73,   181,   134,   163,   175,    27,   117,   117,   250,
     120,   117,   117,   248,   253,   254,   264,    27,   117,    20,
      39,    42,    43,    49,    59,    64,    79,    86,    95,    98,
     100,   117,   124,   246,   118,   121,   248,   120,   252,   245,
     133,   133,   117,   133,   133,   176,   250,   133,   264,   117,
     191,   134,   138,   134,   138,   181,   134,   117,   204,   246,
     248,   117,   246,   264,   246,   134,   138,   134,   264,   264,
     264,   244,   117,   134,   138,   248,   134,   134,   246,   117,
     118,   251,   134,   138,   134,   246,    41,   117,   134,   138,
     134,   134,   246,   264,   248,   118,   257,   118,   134,   264,
     264,   264,   253,   133,   133,     4,     7,     8,    52,    53,
      54,   101,   102,   103,   104,   105,   136,     5,    12,    13,
      46,    67,    84,   108,   109,   110,   122,   123,   124,   125,
     126,   127,   128,   129,   132,   265,   266,    73,   175,   117,
     175,   181,    99,   164,   250,    62,   133,    60,   193,   133,
     133,   133,   139,   121,   111,    62,   133,   118,   118,   248,
     181,   177,   133,   134,   253,   253,   253,   253,   250,   246,
     134,    46,    92,   253,   253,    44,   119,   248,   250,   264,
     253,   253,   106,   248,   246,    17,    68,   237,   238,    91,
     253,   253,   246,    17,    68,   240,   241,   264,   253,   253,
     119,   248,   246,    17,    68,   232,   233,    31,   264,   135,
     139,   134,   248,   117,   192,   117,   192,   192,   264,   264,
      98,   117,   206,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   117,   250,    29,    30,   258,
     250,   135,   134,    73,   133,   133,   133,   117,   133,   264,
     264,   133,   133,   250,   253,   135,   139,   135,   139,   135,
     246,   264,   181,   135,   139,   121,   121,   121,   135,   139,
     248,   121,   135,   135,   253,   136,   250,    17,    68,   135,
     139,   135,   253,   136,   250,    17,    68,    88,   135,   139,
     121,   121,   135,   253,   136,   250,    17,    68,   181,   135,
     246,   134,    65,   194,   194,   194,   134,   246,   134,   258,
     133,   116,   117,   129,   134,   259,   260,    26,   156,   133,
     181,   117,   111,   133,   135,   135,    25,    48,   214,    32,
     230,   119,   246,   246,   116,   256,   237,   136,   181,   253,
     136,   240,   136,   181,   253,   136,   264,   246,   246,   232,
     136,   181,   253,   136,   250,   135,   253,   117,   114,   195,
     195,   246,   232,   208,   246,   248,   105,   159,   260,   128,
     129,   132,    36,    93,   119,   126,   127,   263,   133,   135,
     134,   165,   264,    19,    23,    31,    34,   215,   181,   250,
     121,   135,   135,   135,   250,   181,   136,   181,   250,   181,
     136,   181,    89,   135,   135,   250,   181,   136,   181,    99,
     135,   264,   135,   250,   133,   133,   136,    36,    93,   119,
     263,   252,   135,   260,   260,   261,   263,   116,   255,   256,
     181,   133,   181,   246,   181,   181,   264,   181,   246,   232,
     209,   246,   116,    47,   255,   133,   116,   134,   262,   135,
     250,   135,    52,   250,   133,   210,   136,   135,   116,   117,
     248,   135,    69,    46,   212,   128,    65,   136,   248,   116,
     117,    24,   135,   114,   188,   134,   253,   135
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
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
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

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

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

       Refer to the stacks thru separate pointers, to allow yyoverflow
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
  int yytoken;
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

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

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
  if (yyn == YYPACT_NINF)
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
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
  *++yyvsp = yylval;

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

/* Line 1455 of yacc.c  */
#line 446 "ascend/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in definition.");
	}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 453 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 498 "ascend/compiler/ascParse.y"
    {
	  Asc_ScannerPushBuffer((yyvsp[(2) - (3)].dquote_ptr));
	}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 502 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 507 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 515 "ascend/compiler/ascParse.y"
    {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[(2) - (3)].dquote_ptr));
	}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 519 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 524 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 532 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 541 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 553 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 584 "ascend/compiler/ascParse.y"
    {
	  g_type_name = (yyvsp[(4) - (6)].id_ptr);
	  g_proc_name = (yyvsp[(5) - (6)].id_ptr);
	}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 592 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 615 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 628 "ascend/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 636 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 657 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 669 "ascend/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 677 "ascend/compiler/ascParse.y"
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

/* Line 1455 of yacc.c  */
#line 729 "ascend/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[(3) - (6)].id_ptr);
	  g_atom_dim_ptr = (yyvsp[(4) - (6)].dimp);
	  g_default_double = (yyvsp[(5) - (6)].real_value);
	  g_header_linenum = LineNum();
	}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 740 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 748 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 754 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 759 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 766 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 773 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 0;
	}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 783 "ascend/compiler/ascParse.y"
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

  case 44:

/* Line 1455 of yacc.c  */
#line 823 "ascend/compiler/ascParse.y"
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

  case 45:

/* Line 1455 of yacc.c  */
#line 856 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 862 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 867 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 874 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 881 "ascend/compiler/ascParse.y"
    {
	  (yyval.sym_ptr) = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 891 "ascend/compiler/ascParse.y"
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

  case 51:

/* Line 1455 of yacc.c  */
#line 933 "ascend/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[(2) - (4)].slptr);
	  g_parameter_wheres = (yyvsp[(3) - (4)].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 942 "ascend/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[(2) - (7)].slptr);
	  g_parameter_wheres = (yyvsp[(3) - (7)].slptr);
	  g_refines_name = (yyvsp[(5) - (7)].id_ptr);
	  g_parameter_reduction = (yyvsp[(6) - (7)].slptr);
	  g_header_linenum = LineNum();
	}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 954 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 962 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 966 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 973 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 977 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(3) - (4)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 984 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 988 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 995 "ascend/compiler/ascParse.y"
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

  case 61:

/* Line 1455 of yacc.c  */
#line 1027 "ascend/compiler/ascParse.y"
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

  case 62:

/* Line 1455 of yacc.c  */
#line 1040 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1044 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1051 "ascend/compiler/ascParse.y"
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

  case 65:

/* Line 1455 of yacc.c  */
#line 1081 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1090 "ascend/compiler/ascParse.y"
    { /* nothing to do. just cruft to fix ; problem */ }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1095 "ascend/compiler/ascParse.y"
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

  case 68:

/* Line 1455 of yacc.c  */
#line 1114 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(100L);
	}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1118 "ascend/compiler/ascParse.y"
    {
	  gl_append_ptr((yyvsp[(1) - (2)].listp),(char *)(yyvsp[(2) - (2)].udefptr));
	  (yyval.listp) = (yyvsp[(1) - (2)].listp);
	}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1126 "ascend/compiler/ascParse.y"
    {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[(1) - (4)].id_ptr),(yyvsp[(3) - (4)].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1135 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = NULL;
	}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1139 "ascend/compiler/ascParse.y"
    { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1146 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(3) - (3)].listp);
	}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1153 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(1) - (1)].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1160 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1164 "ascend/compiler/ascParse.y"
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

  case 77:

/* Line 1455 of yacc.c  */
#line 1189 "ascend/compiler/ascParse.y"
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

  case 78:

/* Line 1455 of yacc.c  */
#line 1203 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_proc_name = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1212 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = CreateStatementList((yyvsp[(1) - (1)].listp));
	}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1219 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1223 "ascend/compiler/ascParse.y"
    {
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ((yyvsp[(2) - (3)].statptr) != NULL) {
	    gl_append_ptr((yyvsp[(1) - (3)].listp),(char *)(yyvsp[(2) - (3)].statptr));
	  }
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 1231 "ascend/compiler/ascParse.y"
    {
	  if ((yyvsp[(2) - (3)].listp) != NULL) {
	    gl_append_list((yyvsp[(1) - (3)].listp),(yyvsp[(2) - (3)].listp));
	  }
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 1238 "ascend/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1293 "ascend/compiler/ascParse.y"
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
	        (yyval.statptr) = CreateISA((yyvsp[(1) - (5)].lptr),(yyvsp[(3) - (5)].id_ptr),g_typeargs,(yyvsp[(4) - (5)].id_ptr));
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

  case 125:

/* Line 1455 of yacc.c  */
#line 1334 "ascend/compiler/ascParse.y"
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

  case 126:

/* Line 1455 of yacc.c  */
#line 1363 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateALIASES((yyvsp[(1) - (3)].lptr),(yyvsp[(3) - (3)].nptr));
	}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1368 "ascend/compiler/ascParse.y"
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

  case 128:

/* Line 1455 of yacc.c  */
#line 1417 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1421 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1428 "ascend/compiler/ascParse.y"
    {
	  if (FindType((yyvsp[(3) - (4)].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[(3) - (4)].id_ptr)));
	  }
	}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1440 "ascend/compiler/ascParse.y"
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

  case 132:

/* Line 1455 of yacc.c  */
#line 1467 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_callargs = NULL;
	}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1472 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_callargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1480 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_typeargs = NULL;
	}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1485 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_typeargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1493 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1497 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1504 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1508 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1515 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = NULL;
	}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1519 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1526 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateAA((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1533 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = IgnoreLNK((yyvsp[(5) - (8)].sym_ptr),NULL,(yyvsp[(7) - (8)].lptr));
	}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1537 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateLNK((yyvsp[(3) - (6)].sym_ptr),NULL,(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1541 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1548 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateUNLNK((yyvsp[(3) - (6)].sym_ptr),NULL,(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1552 "ascend/compiler/ascParse.y"
    {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].lptr));
	}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1559 "ascend/compiler/ascParse.y"
    {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[(3) - (4)].lptr));
	}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1568 "ascend/compiler/ascParse.y"
    {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[(2) - (2)].lptr));
	}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1577 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateATS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1584 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1591 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWNBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1598 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1602 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1609 "ascend/compiler/ascParse.y"
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

  case 156:

/* Line 1455 of yacc.c  */
#line 1622 "ascend/compiler/ascParse.y"
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

  case 157:

/* Line 1455 of yacc.c  */
#line 1639 "ascend/compiler/ascParse.y"
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

  case 158:

/* Line 1455 of yacc.c  */
#line 1649 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1657 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1668 "ascend/compiler/ascParse.y"
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

  case 161:

/* Line 1455 of yacc.c  */
#line 1689 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1696 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1703 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1707 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (4)].nptr);
	}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1714 "ascend/compiler/ascParse.y"
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

  case 166:

/* Line 1455 of yacc.c  */
#line 1734 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1738 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (2)].nptr);
	}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1746 "ascend/compiler/ascParse.y"
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

  case 169:

/* Line 1455 of yacc.c  */
#line 1766 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_random;
	}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1770 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_increasing;
	}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1774 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_decreasing;
	}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1781 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1785 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1789 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1793 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1800 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(2) - (2)].nptr),NULL);
	}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1804 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(4) - (4)].nptr),(yyvsp[(2) - (4)].nptr));	  /* type :: name */
	}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1811 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1819 "ascend/compiler/ascParse.y"
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

  case 180:

/* Line 1455 of yacc.c  */
#line 1833 "ascend/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateFREE((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1839 "ascend/compiler/ascParse.y"
    {
		CONSOLE_DEBUG("GOT 'INTEGRATE' STATEMENT from %d to %d steps %d", (yyvsp[(3) - (7)].eptr),(yyvsp[(5) - (7)].eptr),(yyvsp[(7) - (7)].eptr));
	}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1845 "ascend/compiler/ascParse.y"
    {
		CONSOLE_DEBUG("GOT 'SUBSOLVER' STATEMENT WITH '%s'", SCP((yyvsp[(2) - (2)].id_ptr)));
	}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1850 "ascend/compiler/ascParse.y"
    {
		CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP((yyvsp[(2) - (2)].id_ptr)));
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[(2) - (2)].id_ptr)));
	}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1855 "ascend/compiler/ascParse.y"
    {
		CONSOLE_DEBUG("GOT 'SOLVER is isintegrator' STATEMENT WITH '%s'", SCP((yyvsp[(2) - (3)].id_ptr)));
		printf("is integrator \n");
	}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1863 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[(2) - (3)].id_ptr)),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1871 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE();
	}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1879 "ascend/compiler/ascParse.y"
    {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[(2) - (5)].id_ptr)),(yyvsp[(4) - (5)].lptr));
	}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1890 "ascend/compiler/ascParse.y"
    {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[(2) - (2)].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1901 "ascend/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateASSERT((yyvsp[(2) - (2)].eptr));
	}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1907 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[(2) - (6)].eptr),(yyvsp[(4) - (6)].slptr),(yyvsp[(5) - (6)].slptr));
	}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1917 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(5) - (5)].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[(5) - (5)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[(2) - (5)].eptr),(yyvsp[(4) - (5)].slptr));
	}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1926 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1930 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (2)].slptr);
	}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1937 "ascend/compiler/ascParse.y"
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

  case 195:

/* Line 1455 of yacc.c  */
#line 1948 "ascend/compiler/ascParse.y"
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

  case 196:

/* Line 1455 of yacc.c  */
#line 1960 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].wptr));
	}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1967 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(8) - (8)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(8) - (8)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[(1) - (8)].nptr),(yyvsp[(5) - (8)].lptr),(yyvsp[(7) - (8)].wptr));
	}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1977 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[(1) - (1)].wptr));
	}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1984 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1988 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1992 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].wptr));
	}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1996 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].wptr));
	}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 2003 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 2007 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 2011 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 2015 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 2019 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[(2) - (2)].braced_ptr));
	}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 2026 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFNAME((yyvsp[(2) - (2)].nptr));
	}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 2033 "ascend/compiler/ascParse.y"
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

  case 210:

/* Line 1455 of yacc.c  */
#line 2044 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].septr));
	}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 2054 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = ReverseSelectCases((yyvsp[(1) - (1)].septr));
	}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 2061 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 2065 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 2069 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].septr));
	}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 2073 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].septr));
	}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 2080 "ascend/compiler/ascParse.y"
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

  case 217:

/* Line 1455 of yacc.c  */
#line 2091 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].swptr));
	}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 2101 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[(1) - (1)].swptr));
	}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 2108 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 2112 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 2116 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].swptr));
	}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 2120 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].swptr));
	}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 2127 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(3) - (3)].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[(3) - (3)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[(2) - (3)].slptr));
	}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 2137 "ascend/compiler/ascParse.y"
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

  case 225:

/* Line 1455 of yacc.c  */
#line 2174 "ascend/compiler/ascParse.y"
    {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[(2) - (2)].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[(1) - (2)].sym_ptr);
	}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 2183 "ascend/compiler/ascParse.y"
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

  case 227:

/* Line 1455 of yacc.c  */
#line 2198 "ascend/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(2) - (2)].braced_ptr),NULL),
	                     (void *)(yyvsp[(1) - (2)].lptr), LineNum());
	}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 2203 "ascend/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(3) - (3)].braced_ptr),NULL),
	                     (void *)(yyvsp[(2) - (3)].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[(1) - (3)].notesptr));
	}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 2212 "ascend/compiler/ascParse.y"
    {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[(1) - (1)].lptr));
	}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 2223 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 2227 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(3) - (3)].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[(1) - (3)].lptr));
	}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2232 "ascend/compiler/ascParse.y"
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

  case 233:

/* Line 1455 of yacc.c  */
#line 2245 "ascend/compiler/ascParse.y"
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

  case 234:

/* Line 1455 of yacc.c  */
#line 2265 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(1) - (1)].id_ptr));
	}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 2269 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(3) - (3)].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[(1) - (3)].nptr));
	}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 2274 "ascend/compiler/ascParse.y"
    {
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
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 2289 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2294 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2299 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 2304 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 2309 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 2314 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 2319 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 2324 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 2329 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 2334 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 2339 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 2344 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2349 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = (yyvsp[(2) - (2)].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2354 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 2362 "ascend/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = NULL;
	}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 2366 "ascend/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = (yyvsp[(1) - (1)].braced_ptr);
	}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2373 "ascend/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = NULL;
	}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2377 "ascend/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = (yyvsp[(1) - (1)].dquote_ptr);
	}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2384 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = ReverseSetList((yyvsp[(1) - (1)].sptr));
	}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 2388 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 2395 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(1) - (1)].eptr));
	}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 2399 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 2403 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(3) - (3)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (3)].sptr));
	}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2408 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(3) - (5)].eptr),(yyvsp[(5) - (5)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (5)].sptr));
	}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2416 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 2422 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 2431 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (2)].real_value)*(yyvsp[(2) - (2)].real_value);
	}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2435 "ascend/compiler/ascParse.y"
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

  case 265:

/* Line 1455 of yacc.c  */
#line 2456 "ascend/compiler/ascParse.y"
    {
	  g_dim_ptr = Dimensionless();
	  (yyval.real_value) = 1.0;
	}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2461 "ascend/compiler/ascParse.y"
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

  case 267:

/* Line 1455 of yacc.c  */
#line 2482 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = (yyvsp[(2) - (2)].dimp);
	}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2486 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = Dimensionless();
	}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2490 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2497 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2501 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[(1) - (1)].dimen)));
	}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2508 "ascend/compiler/ascParse.y"
    {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[(1) - (1)].id_ptr)));
	}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2512 "ascend/compiler/ascParse.y"
    {
	  ClearDimensions(&((yyval.dimen)));
	}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2516 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = SubDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2520 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = AddDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2524 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[(1) - (3)].dimen)),(yyvsp[(3) - (3)].frac_value));
	}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2528 "ascend/compiler/ascParse.y"
    {
	  CopyDimensions(&((yyvsp[(2) - (3)].dimen)),&((yyval.dimen)));
	}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2535 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = (yyvsp[(1) - (2)].int_value) ? NegateF((yyvsp[(2) - (2)].frac_value)) : (yyvsp[(2) - (2)].frac_value);
	}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2542 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(1) - (1)].int_value),(short)1);
	}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2546 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(2) - (5)].int_value),(short)(yyvsp[(4) - (5)].int_value));
	}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2553 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2557 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2561 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2568 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr((yyvsp[(1) - (1)].int_value));
	}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2572 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2576 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr((yyvsp[(1) - (1)].real_value),g_dim_ptr);
	}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2580 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2584 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateTrueExpr();
	}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2588 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateFalseExpr();
	}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2592 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateAnyExpr();
	}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2596 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[(1) - (1)].sym_ptr));
	}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2600 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateVarExpr((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2604 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSetExpr((yyvsp[(2) - (3)].sptr));
	}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2608 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2613 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2618 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2623 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2628 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2633 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2638 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2643 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_not));
	}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2647 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2652 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2657 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 2662 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2667 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2672 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2676 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_uminus));
	}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2680 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].real_value),g_dim_ptr);
	}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2684 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (4)].nptr),DBL_MAX,NULL);
	}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2688 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2695 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2699 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 2706 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 2710 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 2717 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 2721 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 2728 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 2732 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 2739 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 2743 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 2750 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 2754 "ascend/compiler/ascParse.y"
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

  case 324:

/* Line 1455 of yacc.c  */
#line 2765 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (3)].eptr);
	}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 2772 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 2776 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 2780 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 2784 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 2788 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 2792 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 2799 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 2803 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
    break;



/* Line 1455 of yacc.c  */
#line 5992 "ascend/compiler/ascParse.c"
      default: break;
    }
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
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
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
      if (yyn != YYPACT_NINF)
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

  *++yyvsp = yylval;


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

#if !defined(yyoverflow) || YYERROR_VERBOSE
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
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
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



/* Line 1675 of yacc.c  */
#line 2807 "ascend/compiler/ascParse.y"

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


