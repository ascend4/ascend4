
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
     DIMENSION_TOK = 283,
     DIMENSIONLESS_TOK = 284,
     DO_TOK = 285,
     ELSE_TOK = 286,
     END_TOK = 287,
     EXPECT_TOK = 288,
     EXTERNAL_TOK = 289,
     FALSE_TOK = 290,
     FALLTHRU_TOK = 291,
     FIX_TOK = 292,
     FOR_TOK = 293,
     FREE_TOK = 294,
     FROM_TOK = 295,
     GLOBAL_TOK = 296,
     IF_TOK = 297,
     IMPORT_TOK = 298,
     IN_TOK = 299,
     INPUT_TOK = 300,
     INCREASING_TOK = 301,
     INTERACTIVE_TOK = 302,
     INTERSECTION_TOK = 303,
     ISA_TOK = 304,
     _IS_T = 305,
     ISREFINEDTO_TOK = 306,
     MAXIMIZE_TOK = 307,
     MAXINTEGER_TOK = 308,
     MAXREAL_TOK = 309,
     METHODS_TOK = 310,
     METHOD_TOK = 311,
     MINIMIZE_TOK = 312,
     MODEL_TOK = 313,
     NOT_TOK = 314,
     NOTES_TOK = 315,
     OF_TOK = 316,
     OPTION_TOK = 317,
     OR_TOK = 318,
     OTHERWISE_TOK = 319,
     OUTPUT_TOK = 320,
     PATCH_TOK = 321,
     PROD_TOK = 322,
     PROVIDE_TOK = 323,
     REFINES_TOK = 324,
     REPLACE_TOK = 325,
     REQUIRE_TOK = 326,
     RETURN_TOK = 327,
     RUN_TOK = 328,
     SATISFIED_TOK = 329,
     SELECT_TOK = 330,
     SIZE_TOK = 331,
     SOLVE_TOK = 332,
     SOLVER_TOK = 333,
     STOP_TOK = 334,
     SUCHTHAT_TOK = 335,
     SUM_TOK = 336,
     SWITCH_TOK = 337,
     THEN_TOK = 338,
     TRUE_TOK = 339,
     UNION_TOK = 340,
     UNITS_TOK = 341,
     UNIVERSAL_TOK = 342,
     WHEN_TOK = 343,
     WHERE_TOK = 344,
     WHILE_TOK = 345,
     WILLBE_TOK = 346,
     WILLBETHESAME_TOK = 347,
     WILLNOTBETHESAME_TOK = 348,
     ASSIGN_TOK = 349,
     CASSIGN_TOK = 350,
     DBLCOLON_TOK = 351,
     USE_TOK = 352,
     LEQ_TOK = 353,
     GEQ_TOK = 354,
     NEQ_TOK = 355,
     DOTDOT_TOK = 356,
     WITH_TOK = 357,
     VALUE_TOK = 358,
     WITH_VALUE_T = 359,
     REAL_TOK = 360,
     INTEGER_TOK = 361,
     IDENTIFIER_TOK = 362,
     BRACEDTEXT_TOK = 363,
     SYMBOL_TOK = 364,
     DQUOTE_TOK = 365,
     UPLUS_TOK = 366,
     UMINUS_TOK = 367
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
#define DIMENSION_TOK 283
#define DIMENSIONLESS_TOK 284
#define DO_TOK 285
#define ELSE_TOK 286
#define END_TOK 287
#define EXPECT_TOK 288
#define EXTERNAL_TOK 289
#define FALSE_TOK 290
#define FALLTHRU_TOK 291
#define FIX_TOK 292
#define FOR_TOK 293
#define FREE_TOK 294
#define FROM_TOK 295
#define GLOBAL_TOK 296
#define IF_TOK 297
#define IMPORT_TOK 298
#define IN_TOK 299
#define INPUT_TOK 300
#define INCREASING_TOK 301
#define INTERACTIVE_TOK 302
#define INTERSECTION_TOK 303
#define ISA_TOK 304
#define _IS_T 305
#define ISREFINEDTO_TOK 306
#define MAXIMIZE_TOK 307
#define MAXINTEGER_TOK 308
#define MAXREAL_TOK 309
#define METHODS_TOK 310
#define METHOD_TOK 311
#define MINIMIZE_TOK 312
#define MODEL_TOK 313
#define NOT_TOK 314
#define NOTES_TOK 315
#define OF_TOK 316
#define OPTION_TOK 317
#define OR_TOK 318
#define OTHERWISE_TOK 319
#define OUTPUT_TOK 320
#define PATCH_TOK 321
#define PROD_TOK 322
#define PROVIDE_TOK 323
#define REFINES_TOK 324
#define REPLACE_TOK 325
#define REQUIRE_TOK 326
#define RETURN_TOK 327
#define RUN_TOK 328
#define SATISFIED_TOK 329
#define SELECT_TOK 330
#define SIZE_TOK 331
#define SOLVE_TOK 332
#define SOLVER_TOK 333
#define STOP_TOK 334
#define SUCHTHAT_TOK 335
#define SUM_TOK 336
#define SWITCH_TOK 337
#define THEN_TOK 338
#define TRUE_TOK 339
#define UNION_TOK 340
#define UNITS_TOK 341
#define UNIVERSAL_TOK 342
#define WHEN_TOK 343
#define WHERE_TOK 344
#define WHILE_TOK 345
#define WILLBE_TOK 346
#define WILLBETHESAME_TOK 347
#define WILLNOTBETHESAME_TOK 348
#define ASSIGN_TOK 349
#define CASSIGN_TOK 350
#define DBLCOLON_TOK 351
#define USE_TOK 352
#define LEQ_TOK 353
#define GEQ_TOK 354
#define NEQ_TOK 355
#define DOTDOT_TOK 356
#define WITH_TOK 357
#define VALUE_TOK 358
#define WITH_VALUE_T 359
#define REAL_TOK 360
#define INTEGER_TOK 361
#define IDENTIFIER_TOK 362
#define BRACEDTEXT_TOK 363
#define SYMBOL_TOK 364
#define DQUOTE_TOK 365
#define UPLUS_TOK 366
#define UMINUS_TOK 367




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
#line 641 "ascend/compiler/ascParse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 653 "ascend/compiler/ascParse.c"

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
#define YYLAST   1177

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  130
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  119
/* YYNRULES -- Number of rules.  */
#define YYNRULES  313
/* YYNRULES -- Number of states.  */
#define YYNSTATES  625

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   367

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     124,   125,   119,   116,   111,   117,   127,   118,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   126,   123,
     113,   114,   115,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   128,     2,   129,   122,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   112,     2,     2,     2,     2,     2,
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
     105,   106,   107,   108,   109,   110,   120,   121
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
     280,   281,   285,   289,   291,   293,   295,   297,   299,   301,
     303,   305,   307,   309,   311,   313,   315,   317,   319,   321,
     323,   325,   327,   329,   331,   333,   335,   337,   339,   341,
     343,   345,   347,   349,   351,   353,   355,   361,   367,   371,
     384,   385,   390,   395,   399,   401,   406,   408,   413,   414,
     417,   418,   421,   422,   425,   428,   431,   434,   437,   441,
     445,   447,   451,   453,   456,   459,   469,   473,   477,   478,
     483,   493,   494,   497,   506,   507,   509,   511,   513,   515,
     517,   519,   522,   527,   530,   533,   536,   540,   542,   548,
     551,   554,   561,   567,   568,   571,   576,   583,   590,   599,
     601,   606,   610,   616,   621,   623,   625,   627,   629,   632,
     635,   640,   647,   649,   654,   658,   664,   669,   674,   681,
     683,   688,   692,   698,   703,   707,   711,   714,   718,   721,
     725,   727,   729,   733,   736,   739,   741,   745,   750,   753,
     756,   759,   762,   765,   768,   771,   774,   777,   780,   783,
     786,   789,   791,   792,   794,   795,   797,   799,   800,   802,
     806,   810,   816,   818,   820,   823,   826,   827,   829,   832,
     834,   835,   837,   839,   841,   843,   847,   851,   855,   859,
     862,   864,   870,   871,   873,   875,   877,   879,   881,   883,
     885,   887,   889,   891,   893,   897,   901,   905,   909,   913,
     917,   921,   925,   928,   932,   936,   940,   944,   948,   951,
     954,   961,   966,   971,   976,   981,   986,   991,   996,  1001,
    1006,  1011,  1016,  1021,  1026,  1031,  1035,  1037,  1039,  1041,
    1043,  1045,  1047,  1049
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     131,     0,    -1,    -1,   131,   132,    -1,   134,    -1,   135,
      -1,   136,    -1,   139,    -1,   141,    -1,   137,    -1,   147,
      -1,   143,    -1,   150,    -1,   159,    -1,   156,    -1,   161,
      -1,   133,    -1,     1,    -1,    41,   123,   171,   232,   123,
      -1,    71,   110,   123,    -1,    71,   231,   123,    -1,    71,
     231,    -1,    68,   110,   123,    -1,    68,   231,   123,    -1,
      68,   231,    -1,    43,   107,    40,   110,   123,    -1,    43,
     110,   123,    -1,   138,   226,   232,   123,    -1,     3,    60,
      44,   107,   182,   123,    -1,   140,   167,   232,   123,    -1,
       3,    55,    44,   107,   123,    -1,     3,    55,    44,    27,
      58,   123,    -1,   142,   167,   232,   123,    -1,    70,    55,
      44,   107,   123,    -1,    70,    55,    44,    27,    58,   123,
      -1,   158,   144,   171,   165,   232,   123,    -1,   145,    69,
     107,   240,   146,   123,    -1,    11,   107,    -1,    -1,    26,
     245,   237,    -1,    26,    35,    -1,    26,    84,    -1,    26,
     109,    -1,   158,   148,    -1,    21,   107,    69,   107,   240,
     149,   234,   123,    -1,    -1,    95,   245,   237,    -1,    95,
      84,    -1,    95,    35,    -1,    95,   109,    -1,   158,   151,
     171,   165,   232,   123,    -1,   152,   153,   154,   123,    -1,
     152,   153,   154,    69,   107,   155,   123,    -1,    58,   107,
      -1,    -1,   124,   171,   125,    -1,    -1,    89,   124,   171,
     125,    -1,    -1,   124,   171,   125,    -1,   157,   171,   165,
     232,   123,    -1,    66,   107,    38,   107,   123,    -1,    -1,
      87,    -1,   160,   171,   165,   232,   123,    -1,    27,   107,
      -1,   162,   123,    -1,    86,   163,   232,    -1,    -1,   163,
     164,    -1,   107,   114,   108,   123,    -1,    -1,    -1,    55,
     166,   167,    -1,   168,    -1,    -1,   168,   169,    -1,   170,
     123,   171,   232,   123,    -1,    56,   107,    -1,   172,    -1,
      -1,   172,   173,   123,    -1,   172,     1,   123,    -1,   174,
      -1,   175,    -1,   176,    -1,   178,    -1,   179,    -1,   185,
      -1,   186,    -1,   187,    -1,   188,    -1,   189,    -1,   190,
      -1,   196,    -1,   192,    -1,   208,    -1,   207,    -1,   198,
      -1,   201,    -1,   202,    -1,   203,    -1,   204,    -1,   206,
      -1,   205,    -1,   209,    -1,   210,    -1,   211,    -1,   213,
      -1,   217,    -1,   216,    -1,   218,    -1,   221,    -1,   224,
      -1,   225,    -1,   162,    -1,   228,    49,   181,   183,   184,
      -1,   228,    91,   181,   183,   184,    -1,   228,     4,   230,
      -1,   228,     4,   124,   228,   125,    89,   228,    49,   107,
      61,   107,   177,    -1,    -1,   104,   124,   235,   125,    -1,
     228,    50,   107,   183,    -1,   228,    51,   181,    -1,   107,
      -1,   107,   124,   235,   125,    -1,   107,    -1,   107,   124,
     235,   125,    -1,    -1,    56,   107,    -1,    -1,    61,   107,
      -1,    -1,   104,   246,    -1,   228,     7,    -1,   228,     8,
      -1,   228,    92,    -1,   228,    93,    -1,   230,    94,   246,
      -1,   230,    95,   246,    -1,   191,    -1,   230,   126,   191,
      -1,   246,    -1,    57,   246,    -1,    52,   246,    -1,   230,
     126,   107,   124,   193,   123,   194,   195,   125,    -1,   228,
     126,    45,    -1,   228,   126,    65,    -1,    -1,   123,   230,
     126,    24,    -1,   230,   126,   107,   124,   228,   123,   106,
     125,   197,    -1,    -1,    44,   230,    -1,    38,   107,    44,
     246,   199,   200,   171,   232,    -1,    -1,    46,    -1,    25,
      -1,    23,    -1,    33,    -1,    19,    -1,    30,    -1,    73,
     230,    -1,    73,   230,    96,   230,    -1,    37,   228,    -1,
      39,   228,    -1,    78,   107,    -1,    62,   107,   246,    -1,
      77,    -1,    34,   107,   124,   228,   125,    -1,    15,   180,
      -1,    10,   246,    -1,    42,   246,    83,   171,   212,   232,
      -1,    90,   246,    30,   171,   232,    -1,    -1,    31,   171,
      -1,    88,   228,   214,   232,    -1,   230,   126,    88,   228,
     214,   232,    -1,    88,   124,   228,   125,   214,   232,    -1,
     230,   126,    88,   124,   228,   125,   214,   232,    -1,   215,
      -1,    17,   235,   126,   171,    -1,    64,   126,   171,    -1,
     215,    17,   235,   126,   171,    -1,   215,    64,   126,   171,
      -1,    14,    -1,    22,    -1,    36,    -1,    72,    -1,    79,
     233,    -1,    97,   230,    -1,    75,   228,   219,   232,    -1,
      75,   124,   228,   125,   219,   232,    -1,   220,    -1,    17,
     235,   126,   171,    -1,    64,   126,   171,    -1,   220,    17,
     235,   126,   171,    -1,   220,    64,   126,   171,    -1,    82,
     228,   222,   232,    -1,    82,   124,   228,   125,   222,   232,
      -1,   223,    -1,    17,   235,   126,   171,    -1,    64,   126,
     171,    -1,   223,    17,   235,   126,   171,    -1,   223,    64,
     126,   171,    -1,    20,   171,   232,    -1,    60,   226,   232,
      -1,   109,   227,    -1,   226,   109,   227,    -1,   228,   108,
      -1,   227,   228,   108,    -1,   229,    -1,   230,    -1,   229,
     111,   230,    -1,   229,   230,    -1,   231,   234,    -1,   107,
      -1,   231,   127,   107,    -1,   231,   128,   235,   129,    -1,
      32,    20,    -1,    32,    38,    -1,    32,    42,    -1,    32,
      47,    -1,    32,    55,    -1,    32,    60,    -1,    32,    75,
      -1,    32,    82,    -1,    32,    86,    -1,    32,    41,    -1,
      32,    88,    -1,    32,    90,    -1,    32,   107,    -1,    32,
      -1,    -1,   108,    -1,    -1,   110,    -1,   236,    -1,    -1,
     246,    -1,   246,   101,   246,    -1,   236,   111,   246,    -1,
     236,   111,   246,   101,   246,    -1,   106,    -1,   238,    -1,
     105,   239,    -1,   106,   108,    -1,    -1,   108,    -1,    28,
     241,    -1,    29,    -1,    -1,   119,    -1,   242,    -1,   107,
      -1,   106,    -1,   242,   118,   242,    -1,   242,   119,   242,
      -1,   242,   122,   243,    -1,   124,   242,   125,    -1,   245,
     244,    -1,   106,    -1,   124,   106,   118,   106,   125,    -1,
      -1,   116,    -1,   117,    -1,   106,    -1,    53,    -1,   238,
      -1,    54,    -1,    84,    -1,    35,    -1,     6,    -1,   109,
      -1,   230,    -1,   128,   235,   129,    -1,   246,   116,   246,
      -1,   246,   117,   246,    -1,   246,   119,   246,    -1,   246,
     118,   246,    -1,   246,   122,   246,    -1,   246,     5,   246,
      -1,   246,    63,   246,    -1,    59,   246,    -1,   246,   247,
     246,    -1,   246,   248,   246,    -1,   246,    44,   246,    -1,
     246,   112,   246,    -1,   246,    80,   246,    -1,   116,   246,
      -1,   117,   246,    -1,    74,   124,   230,   111,   238,   125,
      -1,    74,   124,   230,   125,    -1,    81,   124,   235,   125,
      -1,    81,   128,   235,   129,    -1,    67,   124,   235,   125,
      -1,    67,   128,   235,   129,    -1,    85,   124,   235,   125,
      -1,    85,   128,   235,   129,    -1,    48,   124,   235,   125,
      -1,    48,   128,   235,   129,    -1,    16,   124,   235,   125,
      -1,    16,   128,   235,   129,    -1,    18,   124,   235,   125,
      -1,    18,   128,   235,   129,    -1,   107,   124,   246,   125,
      -1,   124,   246,   125,    -1,   114,    -1,   113,    -1,   115,
      -1,    98,    -1,    99,    -1,   100,    -1,    12,    -1,    13,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   425,   425,   427,   431,   432,   433,   434,   435,   436,
     437,   438,   439,   440,   441,   442,   443,   444,   451,   496,
     500,   505,   513,   517,   522,   530,   536,   545,   576,   584,
     607,   620,   628,   649,   661,   669,   721,   732,   741,   746,
     751,   758,   765,   775,   814,   849,   854,   859,   866,   873,
     883,   924,   933,   946,   955,   958,   966,   969,   977,   980,
     987,  1019,  1033,  1036,  1043,  1073,  1082,  1087,  1107,  1110,
    1118,  1128,  1132,  1131,  1145,  1153,  1156,  1181,  1195,  1204,
    1212,  1215,  1223,  1231,  1232,  1233,  1234,  1235,  1236,  1237,
    1238,  1239,  1240,  1241,  1242,  1243,  1244,  1245,  1246,  1247,
    1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,
    1258,  1259,  1260,  1261,  1262,  1263,  1267,  1308,  1337,  1341,
    1392,  1395,  1402,  1414,  1441,  1446,  1454,  1459,  1468,  1471,
    1479,  1482,  1490,  1493,  1500,  1507,  1514,  1521,  1528,  1532,
    1539,  1552,  1569,  1579,  1587,  1598,  1619,  1626,  1634,  1637,
    1644,  1665,  1668,  1675,  1697,  1700,  1704,  1711,  1715,  1719,
    1723,  1730,  1734,  1741,  1749,  1756,  1764,  1772,  1780,  1791,
    1802,  1808,  1818,  1828,  1831,  1838,  1849,  1861,  1868,  1878,
    1885,  1889,  1893,  1897,  1904,  1908,  1912,  1916,  1920,  1927,
    1934,  1945,  1955,  1962,  1966,  1970,  1974,  1981,  1992,  2002,
    2009,  2013,  2017,  2021,  2028,  2038,  2075,  2084,  2099,  2104,
    2113,  2124,  2128,  2133,  2146,  2166,  2170,  2175,  2190,  2195,
    2200,  2205,  2210,  2215,  2220,  2225,  2230,  2235,  2240,  2245,
    2250,  2255,  2264,  2267,  2275,  2278,  2285,  2290,  2296,  2300,
    2304,  2309,  2317,  2323,  2332,  2336,  2358,  2362,  2383,  2387,
    2392,  2398,  2402,  2409,  2413,  2417,  2421,  2425,  2429,  2436,
    2443,  2447,  2455,  2458,  2462,  2469,  2473,  2477,  2481,  2485,
    2489,  2493,  2497,  2501,  2505,  2509,  2514,  2519,  2524,  2529,
    2534,  2539,  2544,  2548,  2553,  2558,  2563,  2568,  2573,  2577,
    2581,  2585,  2589,  2596,  2600,  2607,  2611,  2618,  2622,  2629,
    2633,  2640,  2644,  2651,  2655,  2666,  2673,  2677,  2681,  2685,
    2689,  2693,  2700,  2704
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
  "DEFAULT_TOK", "DEFINITION_TOK", "DIMENSION_TOK", "DIMENSIONLESS_TOK",
  "DO_TOK", "ELSE_TOK", "END_TOK", "EXPECT_TOK", "EXTERNAL_TOK",
  "FALSE_TOK", "FALLTHRU_TOK", "FIX_TOK", "FOR_TOK", "FREE_TOK",
  "FROM_TOK", "GLOBAL_TOK", "IF_TOK", "IMPORT_TOK", "IN_TOK", "INPUT_TOK",
  "INCREASING_TOK", "INTERACTIVE_TOK", "INTERSECTION_TOK", "ISA_TOK",
  "_IS_T", "ISREFINEDTO_TOK", "MAXIMIZE_TOK", "MAXINTEGER_TOK",
  "MAXREAL_TOK", "METHODS_TOK", "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK",
  "NOT_TOK", "NOTES_TOK", "OF_TOK", "OPTION_TOK", "OR_TOK",
  "OTHERWISE_TOK", "OUTPUT_TOK", "PATCH_TOK", "PROD_TOK", "PROVIDE_TOK",
  "REFINES_TOK", "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK",
  "SATISFIED_TOK", "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK", "SOLVER_TOK",
  "STOP_TOK", "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK", "THEN_TOK",
  "TRUE_TOK", "UNION_TOK", "UNITS_TOK", "UNIVERSAL_TOK", "WHEN_TOK",
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
  "constant_def", "constant_head", "constant_val", "model_def",
  "model_head", "model_id", "optional_model_parameters",
  "optional_parameter_wheres", "optional_parameter_reduction", "patch_def",
  "patch_head", "universal", "definition_def", "definition_id",
  "units_def", "units_statement", "unitdeflist", "unitdef", "methods",
  "$@1", "proclist", "proclistf", "procedure", "procedure_id",
  "fstatements", "statements", "statement", "isa_statement",
  "willbe_statement", "aliases_statement", "optional_set_values",
  "is_statement", "isrefinedto_statement", "call_identifier",
  "type_identifier", "optional_method", "optional_of",
  "optional_with_value", "arealike_statement", "arethesame_statement",
  "willbethesame_statement", "willnotbethesame_statement",
  "assignment_statement", "relation_statement", "relation",
  "blackbox_statement", "input_args", "output_args", "data_args",
  "glassbox_statement", "optional_scope", "for_statement",
  "optional_direction", "forexprend", "run_statement", "fix_statement",
  "free_statement", "solver_statement", "option_statement",
  "solve_statement", "external_statement", "call_statement",
  "assert_statement", "if_statement", "while_statement", "optional_else",
  "when_statement", "whenlist", "whenlistf", "flow_statement",
  "use_statement", "select_statement", "selectlist", "selectlistf",
  "switch_statement", "switchlist", "switchlistf", "conditional_statement",
  "notes_statement", "notes_body", "noteslist", "fvarlist", "varlist",
  "fname", "name", "end", "optional_bracedtext", "optional_notes", "set",
  "setexprlist", "number", "realnumber", "opunits", "dims", "dimensions",
  "dimexpr", "fraction", "fractail", "optional_sign", "expr", "relop",
  "logrelop", 0
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
     365,    44,   124,    60,    61,    62,    43,    45,    47,    42,
     366,   367,    94,    59,    40,    41,    58,    46,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   130,   131,   131,   132,   132,   132,   132,   132,   132,
     132,   132,   132,   132,   132,   132,   132,   132,   133,   134,
     134,   134,   135,   135,   135,   136,   136,   137,   138,   139,
     140,   140,   141,   142,   142,   143,   144,   145,   146,   146,
     146,   146,   146,   147,   148,   149,   149,   149,   149,   149,
     150,   151,   151,   152,   153,   153,   154,   154,   155,   155,
     156,   157,   158,   158,   159,   160,   161,   162,   163,   163,
     164,   165,   166,   165,   167,   168,   168,   169,   170,   171,
     172,   172,   172,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   174,   175,   176,   176,
     177,   177,   178,   179,   180,   180,   181,   181,   182,   182,
     183,   183,   184,   184,   185,   186,   187,   188,   189,   189,
     190,   190,   191,   191,   191,   192,   193,   194,   195,   195,
     196,   197,   197,   198,   199,   199,   199,   200,   200,   200,
     200,   201,   201,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   212,   213,   213,   213,   213,   214,
     215,   215,   215,   215,   216,   216,   216,   216,   216,   217,
     218,   218,   219,   220,   220,   220,   220,   221,   221,   222,
     223,   223,   223,   223,   224,   225,   226,   226,   227,   227,
     228,   229,   229,   229,   230,   231,   231,   231,   232,   232,
     232,   232,   232,   232,   232,   232,   232,   232,   232,   232,
     232,   232,   233,   233,   234,   234,   235,   235,   236,   236,
     236,   236,   237,   237,   238,   238,   239,   239,   240,   240,
     240,   241,   241,   242,   242,   242,   242,   242,   242,   243,
     244,   244,   245,   245,   245,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   247,   247,   247,   247,
     247,   247,   248,   248
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
       0,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     5,     5,     3,    12,
       0,     4,     4,     3,     1,     4,     1,     4,     0,     2,
       0,     2,     0,     2,     2,     2,     2,     2,     3,     3,
       1,     3,     1,     2,     2,     9,     3,     3,     0,     4,
       9,     0,     2,     8,     0,     1,     1,     1,     1,     1,
       1,     2,     4,     2,     2,     2,     3,     1,     5,     2,
       2,     6,     5,     0,     2,     4,     6,     6,     8,     1,
       4,     3,     5,     4,     1,     1,     1,     1,     2,     2,
       4,     6,     1,     4,     3,     5,     4,     4,     6,     1,
       4,     3,     5,     4,     3,     3,     2,     3,     2,     3,
       1,     1,     3,     2,     2,     1,     3,     4,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     1,     0,     1,     0,     1,     1,     0,     1,     3,
       3,     5,     1,     1,     2,     2,     0,     1,     2,     1,
       0,     1,     1,     1,     1,     3,     3,     3,     3,     2,
       1,     5,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     3,     3,     3,     3,     3,     2,     2,
       6,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     3,     1,     1,     1,     1,
       1,     1,     1,     1
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
       0,     0,   215,     0,    24,     0,     0,    21,     0,     0,
       0,     0,    74,     0,    71,     0,     0,     0,     0,    80,
       0,    43,    80,    54,    71,    66,     0,     0,     0,     0,
      26,     0,    22,    23,     0,   237,     0,    19,    20,   231,
       0,    69,    67,   206,     0,   210,   211,   234,     0,     0,
       0,     0,    76,     0,     0,    72,     0,     0,   271,     0,
     184,     0,     0,     0,    80,   185,     0,   270,   186,     0,
       0,     0,     0,     0,     0,   266,   268,     0,     0,     0,
       0,     0,   187,     0,     0,     0,   167,     0,   232,     0,
       0,   269,     0,     0,     0,     0,   246,   265,   215,   272,
       0,     0,     0,   237,   115,     0,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,   140,    95,    94,
      98,    99,   100,   101,   102,   104,   103,    97,    96,   105,
     106,   107,   108,   110,   109,   111,   112,   113,   114,     0,
     273,   267,   142,    37,     0,    53,    71,     0,    71,    80,
      56,     0,     0,     0,   128,     0,     0,     0,   216,   273,
       0,   236,   238,     0,     0,   218,   219,   227,   220,   221,
     222,   223,   224,   225,   226,   228,   229,   230,     0,     0,
     208,     0,   213,   235,   214,   207,    27,    29,    78,    80,
      32,    75,     0,    82,   170,   124,   169,   237,   237,   237,
     237,     0,     0,   163,     0,   164,     0,   237,   237,   144,
     143,   282,     0,     0,   237,   237,   161,     0,     0,     0,
     165,   233,   188,   237,   237,     0,     0,   237,   237,     0,
       0,     0,   189,   247,   244,   245,     0,   288,   289,     0,
       0,    81,     0,   134,   135,     0,     0,     0,     0,   136,
     137,     0,     0,     0,     0,   312,   313,     0,     0,     0,
     309,   310,   311,     0,   307,   306,   308,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   250,     0,     0,     0,
       0,     0,     0,    30,     0,     0,    18,    25,    61,   217,
       0,     0,     0,    33,     0,   209,   212,     0,    73,    60,
     237,     0,     0,     0,     0,   204,     0,     0,    80,     0,
       0,   205,   166,     0,     0,     0,     0,     0,   237,     0,
       0,   192,     0,     0,     0,   237,     0,     0,   199,     0,
       0,     0,   237,     0,     0,   179,    80,     0,   305,   274,
       0,   118,   126,   130,   130,   123,   130,   138,   139,     0,
     215,   141,   280,   285,   281,   287,   286,   275,   276,   278,
     277,   279,   283,   284,   250,     0,     0,   249,    38,     0,
      55,    80,     0,    51,    64,    31,   129,    28,   240,   239,
      34,    70,     0,     0,   300,   301,   302,   303,     0,   154,
     173,   298,   299,   294,   295,   162,     0,   291,     0,     0,
      80,   190,   237,     0,   292,   293,     0,     0,    80,   197,
     237,     0,   296,   297,     0,     0,    80,   175,   237,     0,
       0,   304,     0,   237,     0,   132,   122,   132,     0,     0,
       0,    45,    35,   254,   253,   251,     0,   248,   252,   262,
       0,    50,     0,    58,     0,    77,   125,   168,   156,   155,
       0,    80,     0,     0,     0,     0,    80,   194,     0,    80,
       0,    80,   201,     0,    80,     0,    80,   181,     0,    80,
     172,     0,     0,   131,     0,   116,   117,     0,     0,     0,
       0,   273,   262,   234,     0,     0,     0,   262,    40,    41,
      42,   263,   264,     0,    36,    57,    80,     0,   241,   159,
     157,   160,   158,    80,   174,   171,   290,   191,   193,    80,
     196,   198,   200,    80,   203,   177,   180,    80,   183,     0,
     127,   133,     0,   176,     0,     0,     0,    48,    47,    49,
       0,     0,   258,   255,   256,   257,     0,   242,    39,   243,
       0,    52,     0,   195,   202,   182,     0,     0,   148,     0,
       0,   146,    46,    44,   260,     0,   259,    59,   153,     0,
     178,     0,     0,     0,   151,     0,     0,     0,   145,   147,
       0,   150,     0,     0,     0,   152,     0,   120,   149,   261,
       0,   119,   237,     0,   121
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    59,    60,   480,    26,    61,   523,
      27,    62,    63,   190,   320,   537,    28,    29,    30,    31,
      32,    33,    34,    48,    81,    96,   231,    51,    52,    92,
      93,    54,    55,   145,   146,   147,   148,   621,   149,   150,
     236,   383,   325,   465,   515,   151,   152,   153,   154,   155,
     156,   157,   158,   519,   588,   602,   159,   611,   160,   490,
     543,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   492,   172,   374,   375,   173,   174,   175,   360,
     361,   176,   367,   368,   177,   178,    50,    83,    84,    85,
     199,    87,    82,   262,   224,   200,   201,   578,   181,   274,
     408,   477,   478,   575,   596,   533,   202,   312,   313
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -465
static const yytype_int16 yypact[] =
{
    -465,  1090,  -465,  -465,   -37,   -52,   -38,    60,   -32,    76,
      57,    82,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
      18,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
      11,  -465,  -465,  -465,    32,     0,    92,  -465,  -465,   120,
      61,    94,  -465,    89,   -71,   186,   114,    41,   -16,   156,
     -11,   229,   209,   229,   213,   574,   167,   175,   177,  -465,
     216,  -465,  -465,   162,   213,  -465,   -14,   180,   229,   178,
    -465,   183,  -465,  -465,   185,    35,   -10,  -465,  -465,   176,
     181,  -465,  -465,   156,   188,    17,  -465,    19,   156,   171,
     179,   191,  -465,   182,   184,  -465,   229,   190,  -465,    35,
    -465,   192,    49,    73,  -465,  -465,   193,  -465,  -465,   156,
     194,   156,    35,   100,    35,  -465,  -465,    35,    35,    18,
     197,   126,  -465,   156,   196,   -79,  -465,   201,   206,   129,
     -65,  -465,   131,    26,    35,   156,   207,   208,   198,  -465,
      35,    35,    35,    35,  -465,   195,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,    88,
     149,  -465,   956,  -465,   252,  -465,   213,   226,   213,  -465,
     246,   229,   278,   215,   283,   220,   221,   222,  -465,  -465,
     212,   236,   796,   290,   227,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,   243,   245,
    -465,   156,  -465,  -465,  -465,   156,  -465,  -465,  -465,  -465,
    -465,  -465,   231,  -465,   956,   233,  -465,    35,    35,    35,
      35,   229,   234,  -465,   317,  -465,   836,    35,    35,   956,
     956,  -465,   -11,    35,    35,    35,   266,   156,   156,     8,
    -465,  -465,  -465,    35,    35,   156,     9,    35,    35,   156,
      13,   876,  -465,  -465,  -465,  -465,    35,   242,   242,   630,
     237,  -465,    47,  -465,  -465,   258,   260,   258,   258,  -465,
    -465,    35,    35,   275,    35,  -465,  -465,    35,    35,    35,
    -465,  -465,  -465,    35,  -465,  -465,  -465,    35,    35,    35,
      35,    35,    35,    35,   262,   229,    71,   229,   247,   251,
     -62,   248,   253,  -465,   270,   255,  -465,  -465,  -465,  -465,
      35,    35,   263,  -465,   265,  -465,  -465,   229,  -465,  -465,
      35,   254,   264,   269,   271,  -465,   156,    35,  -465,   276,
     273,  -465,   956,   279,   277,   156,   -78,   280,    35,   257,
     229,    37,   282,   281,   284,    35,   259,   229,    54,   286,
     285,   288,    35,   289,   229,    62,  -465,   673,  -465,  -465,
     156,  -465,   293,   328,   328,  -465,   328,   956,   956,    51,
     294,  -465,  1006,  1006,  1006,   706,   706,    84,    84,   242,
     242,   242,   154,   981,    71,   272,    42,  -465,   382,   297,
    -465,  -465,   315,  -465,  -465,  -465,  -465,  -465,   916,   956,
    -465,  -465,   300,   301,  -465,  -465,  -465,  -465,   303,   753,
     398,  -465,  -465,  -465,  -465,  -465,    85,  -465,     8,   304,
    -465,  -465,    35,   305,  -465,  -465,     9,   307,  -465,  -465,
      35,   308,  -465,  -465,    13,   309,  -465,  -465,    35,   310,
     229,  -465,   312,    35,   331,   335,  -465,   335,   156,    13,
      35,   345,  -465,  -465,  -465,  -465,   -57,  -465,   116,    -6,
     318,  -465,   320,   319,    35,  -465,  -465,  -465,  -465,  -465,
     155,  -465,   229,   208,   321,   229,  -465,  -465,   316,  -465,
     229,  -465,  -465,   322,  -465,   229,  -465,  -465,   323,  -465,
    -465,   361,   326,  -465,    35,  -465,  -465,   329,   229,   330,
      87,   -80,    78,   346,   127,   -57,   -57,   161,  -465,  -465,
    -465,  -465,  -465,   142,  -465,  -465,  -465,   332,   956,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,   156,
    -465,   956,    13,  -465,   156,   352,   414,  -465,  -465,  -465,
     142,   337,  -465,   339,   339,  -465,   -87,   208,  -465,  -465,
     338,  -465,   229,  -465,  -465,  -465,   413,   229,   341,   340,
     343,  -465,  -465,  -465,  -465,   359,  -465,  -465,  -465,   362,
    -465,   156,   348,   406,   430,   358,   417,   353,  -465,  -465,
     156,  -465,   375,   376,   460,  -465,   363,   383,  -465,  -465,
     366,  -465,    35,   367,  -465
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,   431,  -465,  -465,   -63,  -465,   -20,  -465,  -465,
    -465,   -24,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,    -8,  -465,  -165,    24,  -465,  -465,  -465,  -465,  -465,
    -465,   200,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,  -465,
    -465,  -465,  -465,  -465,  -445,  -465,  -465,  -465,  -465,    56,
    -465,  -465,    50,  -465,  -465,  -465,   378,   407,   -43,  -465,
     -49,   218,   -48,  -465,   -25,  -133,  -465,   -70,  -425,  -465,
      95,  -465,  -461,  -465,  -465,  -464,   256,  -465,  -465
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -212
static const yytype_int16 yytable[] =
{
      86,   191,    89,    90,    53,    94,   180,   412,    64,   505,
     280,   494,   179,   192,    68,   524,    79,   203,    35,   594,
     195,    79,    56,    36,   518,   358,   365,  -211,    42,   528,
     372,  -211,    57,   436,    86,   186,   222,   595,   188,    86,
     219,    98,    42,  -211,    66,   258,  -211,   437,   232,   473,
     474,   102,    73,   103,   442,    37,    74,    75,   570,   265,
      86,   413,    86,   576,   573,   574,   243,   476,   245,    58,
     107,   450,   359,   366,   256,    41,    86,   373,   529,   458,
     241,    86,   259,   113,    86,    38,   272,   266,   115,   116,
     270,    80,   282,   193,   118,   283,   284,   204,    88,   406,
     407,   443,   121,   530,   341,   342,   343,   344,   579,   124,
     531,   532,    45,   567,   349,   350,   129,   587,   451,   131,
     132,   353,   354,   315,    42,   317,   459,    49,   221,   223,
     362,   363,    71,    42,   369,   370,    67,   285,   286,   287,
     136,   137,   138,   321,   139,   579,    74,    75,   473,   474,
     269,   140,   141,  -211,    42,    65,  -211,  -211,    42,   142,
      69,   475,   568,   143,    78,   318,   476,    39,    74,    75,
      40,   380,   336,   237,   539,   468,    86,   238,   540,   288,
     289,   290,   219,    42,    70,   541,    43,   569,   542,    42,
     136,   493,    46,   345,   531,   532,   205,   239,  -211,  -211,
    -211,   240,   309,   310,   351,   337,   311,   423,   356,    86,
     565,   338,    72,   566,   206,   357,    86,   207,   208,   466,
      86,   467,   364,   209,   247,   439,   371,    44,   248,    47,
      76,   210,   447,   381,   525,   526,   211,    77,   527,   455,
    -211,  -211,  -211,   291,   292,   525,   526,   136,   577,   527,
     254,   212,   572,   263,   255,   267,  -211,   264,   213,   268,
    -211,    79,   214,    42,   215,    91,   216,   405,    95,   409,
     307,   308,   309,   310,   183,   293,   311,   531,   532,   385,
     386,    98,   184,   217,   185,   187,   189,   194,   196,   422,
     197,   102,   198,   103,   226,   218,   220,    86,   228,   235,
     242,   244,   227,   428,   253,   229,   435,   230,   260,   498,
     107,   182,   441,   233,   261,   273,   275,   503,   281,   449,
     257,   314,   276,   113,   430,   508,   457,   114,   115,   116,
     512,    86,   117,   316,   118,   319,   322,   462,   323,   324,
      86,   329,   121,   326,   327,   328,   469,   330,   332,   124,
     333,   334,   460,   335,   339,   234,   129,   340,   346,   131,
     132,   347,   355,   389,   311,   382,   379,   384,   246,   404,
     249,   414,   410,   250,   251,   411,   415,   416,   417,   424,
     136,   137,   390,   440,   139,   448,   420,   482,   421,   464,
     271,   140,   141,   425,   426,   472,   277,   278,   279,   142,
     427,   431,   432,   143,   433,   438,   434,   444,   479,   446,
     445,   452,   510,   454,   453,   456,   497,   463,   470,    86,
     481,   521,   483,   485,   502,   517,   486,   520,   487,   491,
     496,   499,   507,   501,   504,   506,   509,   511,   513,   514,
     522,   534,   549,   536,   545,   535,   546,   547,   553,   557,
     559,   560,   551,   564,   562,   581,   223,   555,   590,   591,
     593,   527,   599,   597,   601,   605,   603,   544,   604,   606,
     563,   609,   548,   608,   610,   550,   612,   552,   613,   614,
     554,   616,   556,   617,   618,   558,   144,   620,   619,   623,
     622,   516,   624,   391,   495,   225,   500,   252,   571,   471,
     592,     0,     0,     0,     0,     0,     0,     0,     0,   352,
      86,     0,   580,     0,     0,    86,   586,     0,     0,   582,
       0,   589,     0,     0,     0,   583,     0,     0,     0,   584,
       0,     0,   377,   585,   598,     0,     0,     0,     0,   600,
       0,     0,     0,     0,     0,     0,     0,   387,   388,   182,
     392,     0,   607,   393,   394,   395,     0,     0,     0,   396,
       0,   615,     0,   397,   398,   399,   400,   401,   402,   403,
       0,     0,     0,     0,     0,    97,     0,     0,     0,     0,
      98,     0,     0,     0,    99,     0,   418,   419,   100,   101,
     102,   -79,   103,     0,   104,     0,   105,     0,     0,     0,
       0,     0,     0,   429,     0,   -79,   -79,     0,   106,   107,
     108,   109,   110,   111,     0,     0,   112,     0,     0,     0,
       0,     0,   113,     0,     0,     0,   114,   115,   116,   -79,
       0,   117,     0,   118,   119,   294,   120,     0,   -79,     0,
       0,   121,   295,   296,     0,     0,   122,   123,   124,   125,
       0,   126,   127,   128,     0,   129,   130,     0,   131,   132,
      12,     0,   133,     0,   134,     0,     0,     0,     0,     0,
       0,   135,     0,     0,   297,     0,     0,     0,   294,   136,
     137,   138,     0,   139,     0,   295,   296,     0,     0,     0,
     140,   141,     0,   298,     0,     0,     0,     0,   142,   -79,
       0,     0,   143,     0,     0,     0,     0,     0,     0,     0,
     299,   294,     0,     0,     0,     0,     0,   297,   295,   296,
       0,     0,     0,     0,     0,     0,   377,     0,   300,   301,
     302,     0,     0,     0,     0,     0,   298,     0,     0,     0,
     538,     0,   303,   304,   305,   306,   307,   308,   309,   310,
     297,     0,   311,   299,     0,   378,     0,     0,   294,     0,
       0,     0,     0,     0,     0,   295,   296,     0,     0,   298,
     561,   300,   301,   302,     0,     0,     0,     0,   488,     0,
       0,     0,     0,     0,     0,   303,   304,   305,   306,   307,
     308,   309,   310,     0,     0,   311,     0,   297,   461,   489,
       0,   294,     0,     0,   300,   301,   302,     0,   295,   296,
       0,     0,     0,     0,     0,     0,   298,     0,     0,   304,
     305,   306,   307,   308,   309,   310,     0,     0,   311,     0,
       0,     0,     0,   299,     0,     0,     0,     0,     0,     0,
     297,   294,     0,     0,     0,     0,     0,     0,   295,   296,
       0,   300,   301,   302,     0,     0,     0,     0,     0,   298,
       0,     0,     0,     0,     0,   303,   304,   305,   306,   307,
     308,   309,   310,     0,     0,   311,   299,     0,     0,     0,
     297,   294,     0,     0,     0,     0,     0,     0,   295,   296,
       0,     0,     0,     0,   300,   301,   302,   331,     0,   298,
       0,     0,     0,     0,     0,     0,   376,     0,   303,   304,
     305,   306,   307,   308,   309,   310,   299,     0,   311,   348,
     297,   294,     0,     0,     0,     0,     0,     0,   295,   296,
       0,     0,     0,     0,   300,   301,   302,     0,     0,   298,
       0,     0,     0,     0,     0,     0,     0,     0,   303,   304,
     305,   306,   307,   308,   309,   310,   299,     0,   311,     0,
     297,   294,     0,     0,     0,     0,     0,     0,   295,   296,
       0,     0,     0,     0,   300,   301,   302,     0,     0,   298,
       0,     0,     0,     0,     0,     0,   294,     0,   303,   304,
     305,   306,   307,   308,   309,   310,   299,     0,   311,     0,
     297,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   300,   301,   302,   484,     0,   298,
       0,     0,     0,     0,     0,   297,     0,     0,   303,   304,
     305,   306,   307,   308,   309,   310,   299,     0,   311,     0,
       0,     0,     0,     0,   298,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   300,   301,   302,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   303,   304,
     305,   306,   307,   308,   309,   310,     0,     0,   311,   300,
     301,   302,     0,     0,     0,     0,     0,     0,     0,     0,
       2,     3,     0,     4,   304,   305,   306,   307,   308,   309,
     310,   -62,     0,   311,   300,   301,   302,     0,     0,     0,
       0,   -62,     0,     0,     0,     0,     0,     5,     0,   304,
     305,   306,   307,   308,   309,   310,     0,     0,   311,     0,
       0,     6,     0,     7,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   -62,     0,
       0,     0,     0,     0,     0,     0,     8,     0,     9,     0,
      10,    11,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    12,    13
};

static const yytype_int16 yycheck[] =
{
      49,    64,    50,    51,    24,    53,    55,    69,    32,   454,
     143,   436,    55,    27,    38,   476,    32,    27,    55,   106,
      68,    32,    11,    60,   469,    17,    17,   107,   107,    35,
      17,   111,    21,   111,    83,    59,    85,   124,    62,    88,
      83,     6,   107,   123,    44,   124,   126,   125,    96,   106,
     107,    16,   123,    18,    17,   107,   127,   128,   522,   124,
     109,   123,   111,   527,   525,   526,   109,   124,   111,    58,
      35,    17,    64,    64,   123,   107,   125,    64,    84,    17,
     104,   130,   125,    48,   133,   123,   135,   130,    53,    54,
     133,   107,     4,   107,    59,     7,     8,   107,   109,    28,
      29,    64,    67,   109,   237,   238,   239,   240,   533,    74,
     116,   117,    55,    35,   247,   248,    81,   562,    64,    84,
      85,   254,   255,   186,   107,   188,    64,   109,   111,   110,
     263,   264,    38,   107,   267,   268,    44,    49,    50,    51,
     105,   106,   107,   191,   109,   570,   127,   128,   106,   107,
     124,   116,   117,     4,   107,   123,     7,     8,   107,   124,
      40,   119,    84,   128,   123,   189,   124,   107,   127,   128,
     110,   124,   221,   124,    19,   124,   225,   128,    23,    91,
      92,    93,   225,   107,   123,    30,   110,   109,    33,   107,
     105,   106,   110,   241,   116,   117,    20,   124,    49,    50,
      51,   128,   118,   119,   252,   229,   122,   340,   257,   258,
     123,   231,   123,   126,    38,   258,   265,    41,    42,   384,
     269,   386,   265,    47,   124,   358,   269,     9,   128,    11,
      44,    55,   365,   282,   118,   119,    60,   123,   122,   372,
      91,    92,    93,    94,    95,   118,   119,   105,   106,   122,
     124,    75,   125,   124,   128,   124,   107,   128,    82,   128,
     111,    32,    86,   107,    88,    56,    90,   315,    55,   317,
     116,   117,   118,   119,   107,   126,   122,   116,   117,   287,
     288,     6,   107,   107,   107,    69,   124,   107,   110,   337,
     107,    16,   107,    18,   123,   114,   108,   346,   107,   107,
     107,   107,   123,   346,   107,   123,   355,   123,   107,   442,
      35,    55,   360,   123,   108,   108,   108,   450,   123,   367,
     124,    69,   124,    48,   348,   458,   374,    52,    53,    54,
     463,   380,    57,   107,    59,    89,    58,   380,   123,    56,
     389,   129,    67,   123,   123,   123,   389,   111,    58,    74,
     123,   108,   376,   108,   123,    99,    81,   124,   124,    84,
      85,    44,    96,    88,   122,   107,   129,   107,   112,   107,
     114,   123,   125,   117,   118,   124,   123,   107,   123,   125,
     105,   106,   107,   126,   109,   126,   123,   411,   123,    61,
     134,   116,   117,   129,   125,   123,   140,   141,   142,   124,
     129,   125,   129,   128,   125,   125,   129,   125,    26,   125,
     129,   125,   460,   125,   129,   126,   440,   124,   124,   468,
     123,   470,   107,   123,   448,   468,   125,   470,   125,    31,
     126,   126,   456,   126,   126,   126,   126,   125,   107,   104,
      95,   123,   126,   124,   492,   125,   125,   495,   126,   126,
      89,   125,   500,   123,   125,   123,   110,   505,   106,    45,
     123,   122,    49,   125,   123,   106,   126,   491,   125,   107,
     518,    65,   496,   125,    44,   499,   118,   501,    61,   126,
     504,   106,   506,   107,    24,   509,    55,   104,   125,   622,
     124,   467,   125,   293,   438,    88,   446,   119,   523,   404,
     570,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   253,
     559,    -1,   536,    -1,    -1,   564,   559,    -1,    -1,   543,
      -1,   564,    -1,    -1,    -1,   549,    -1,    -1,    -1,   553,
      -1,    -1,   276,   557,   582,    -1,    -1,    -1,    -1,   587,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   291,   292,   293,
     294,    -1,   601,   297,   298,   299,    -1,    -1,    -1,   303,
      -1,   610,    -1,   307,   308,   309,   310,   311,   312,   313,
      -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,
       6,    -1,    -1,    -1,    10,    -1,   330,   331,    14,    15,
      16,    17,    18,    -1,    20,    -1,    22,    -1,    -1,    -1,
      -1,    -1,    -1,   347,    -1,    31,    32,    -1,    34,    35,
      36,    37,    38,    39,    -1,    -1,    42,    -1,    -1,    -1,
      -1,    -1,    48,    -1,    -1,    -1,    52,    53,    54,    55,
      -1,    57,    -1,    59,    60,     5,    62,    -1,    64,    -1,
      -1,    67,    12,    13,    -1,    -1,    72,    73,    74,    75,
      -1,    77,    78,    79,    -1,    81,    82,    -1,    84,    85,
      86,    -1,    88,    -1,    90,    -1,    -1,    -1,    -1,    -1,
      -1,    97,    -1,    -1,    44,    -1,    -1,    -1,     5,   105,
     106,   107,    -1,   109,    -1,    12,    13,    -1,    -1,    -1,
     116,   117,    -1,    63,    -1,    -1,    -1,    -1,   124,   125,
      -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,     5,    -1,    -1,    -1,    -1,    -1,    44,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,   470,    -1,    98,    99,
     100,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,    -1,
     484,    -1,   112,   113,   114,   115,   116,   117,   118,   119,
      44,    -1,   122,    80,    -1,   125,    -1,    -1,     5,    -1,
      -1,    -1,    -1,    -1,    -1,    12,    13,    -1,    -1,    63,
     514,    98,    99,   100,    -1,    -1,    -1,    -1,    25,    -1,
      -1,    -1,    -1,    -1,    -1,   112,   113,   114,   115,   116,
     117,   118,   119,    -1,    -1,   122,    -1,    44,   125,    46,
      -1,     5,    -1,    -1,    98,    99,   100,    -1,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,   113,
     114,   115,   116,   117,   118,   119,    -1,    -1,   122,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      44,     5,    -1,    -1,    -1,    -1,    -1,    -1,    12,    13,
      -1,    98,    99,   100,    -1,    -1,    -1,    -1,    -1,    63,
      -1,    -1,    -1,    -1,    -1,   112,   113,   114,   115,   116,
     117,   118,   119,    -1,    -1,   122,    80,    -1,    -1,    -1,
      44,     5,    -1,    -1,    -1,    -1,    -1,    -1,    12,    13,
      -1,    -1,    -1,    -1,    98,    99,   100,   101,    -1,    63,
      -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,   112,   113,
     114,   115,   116,   117,   118,   119,    80,    -1,   122,    83,
      44,     5,    -1,    -1,    -1,    -1,    -1,    -1,    12,    13,
      -1,    -1,    -1,    -1,    98,    99,   100,    -1,    -1,    63,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,   113,
     114,   115,   116,   117,   118,   119,    80,    -1,   122,    -1,
      44,     5,    -1,    -1,    -1,    -1,    -1,    -1,    12,    13,
      -1,    -1,    -1,    -1,    98,    99,   100,    -1,    -1,    63,
      -1,    -1,    -1,    -1,    -1,    -1,     5,    -1,   112,   113,
     114,   115,   116,   117,   118,   119,    80,    -1,   122,    -1,
      44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    99,   100,   101,    -1,    63,
      -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,   112,   113,
     114,   115,   116,   117,   118,   119,    80,    -1,   122,    -1,
      -1,    -1,    -1,    -1,    63,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    98,    99,   100,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,   113,
     114,   115,   116,   117,   118,   119,    -1,    -1,   122,    98,
      99,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       0,     1,    -1,     3,   113,   114,   115,   116,   117,   118,
     119,    11,    -1,   122,    98,    99,   100,    -1,    -1,    -1,
      -1,    21,    -1,    -1,    -1,    -1,    -1,    27,    -1,   113,
     114,   115,   116,   117,   118,   119,    -1,    -1,   122,    -1,
      -1,    41,    -1,    43,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    66,    -1,    68,    -1,
      70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    86,    87
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   131,     0,     1,     3,    27,    41,    43,    66,    68,
      70,    71,    86,    87,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   147,   150,   156,   157,
     158,   159,   160,   161,   162,    55,    60,   107,   123,   107,
     110,   107,   107,   110,   231,    55,   110,   231,   163,   109,
     226,   167,   168,   167,   171,   172,    11,    21,    58,   144,
     145,   148,   151,   152,   171,   123,    44,    44,   171,    40,
     123,    38,   123,   123,   127,   128,    44,   123,   123,    32,
     107,   164,   232,   227,   228,   229,   230,   231,   109,   232,
     232,    56,   169,   170,   232,    55,   165,     1,     6,    10,
      14,    15,    16,    18,    20,    22,    34,    35,    36,    37,
      38,    39,    42,    48,    52,    53,    54,    57,    59,    60,
      62,    67,    72,    73,    74,    75,    77,    78,    79,    81,
      82,    84,    85,    88,    90,    97,   105,   106,   107,   109,
     116,   117,   124,   128,   162,   173,   174,   175,   176,   178,
     179,   185,   186,   187,   188,   189,   190,   191,   192,   196,
     198,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   213,   216,   217,   218,   221,   224,   225,   228,
     230,   238,   246,   107,   107,   107,   171,    69,   171,   124,
     153,   165,    27,   107,   107,   232,   110,   107,   107,   230,
     235,   236,   246,    27,   107,    20,    38,    41,    42,    47,
      55,    60,    75,    82,    86,    88,    90,   107,   114,   228,
     108,   111,   230,   110,   234,   227,   123,   123,   107,   123,
     123,   166,   232,   123,   246,   107,   180,   124,   128,   124,
     128,   171,   107,   228,   107,   228,   246,   124,   128,   246,
     246,   246,   226,   107,   124,   128,   230,   124,   124,   228,
     107,   108,   233,   124,   128,   124,   228,   124,   128,   124,
     228,   246,   230,   108,   239,   108,   124,   246,   246,   246,
     235,   123,     4,     7,     8,    49,    50,    51,    91,    92,
      93,    94,    95,   126,     5,    12,    13,    44,    63,    80,
      98,    99,   100,   112,   113,   114,   115,   116,   117,   118,
     119,   122,   247,   248,    69,   165,   107,   165,   171,    89,
     154,   232,    58,   123,    56,   182,   123,   123,   123,   129,
     111,   101,    58,   123,   108,   108,   230,   171,   167,   123,
     124,   235,   235,   235,   235,   232,   124,    44,    83,   235,
     235,   232,   246,   235,   235,    96,   230,   228,    17,    64,
     219,   220,   235,   235,   228,    17,    64,   222,   223,   235,
     235,   228,    17,    64,   214,   215,    30,   246,   125,   129,
     124,   230,   107,   181,   107,   181,   181,   246,   246,    88,
     107,   191,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   107,   232,    28,    29,   240,   232,
     125,   124,    69,   123,   123,   123,   107,   123,   246,   246,
     123,   123,   232,   235,   125,   129,   125,   129,   228,   246,
     171,   125,   129,   125,   129,   230,   111,   125,   125,   235,
     126,   232,    17,    64,   125,   129,   125,   235,   126,   232,
      17,    64,   125,   129,   125,   235,   126,   232,    17,    64,
     171,   125,   228,   124,    61,   183,   183,   183,   124,   228,
     124,   240,   123,   106,   107,   119,   124,   241,   242,    26,
     146,   123,   171,   107,   101,   123,   125,   125,    25,    46,
     199,    31,   212,   106,   238,   219,   126,   171,   235,   126,
     222,   126,   171,   235,   126,   214,   126,   171,   235,   126,
     232,   125,   235,   107,   104,   184,   184,   228,   214,   193,
     228,   230,    95,   149,   242,   118,   119,   122,    35,    84,
     109,   116,   117,   245,   123,   125,   124,   155,   246,    19,
      23,    30,    33,   200,   171,   232,   125,   232,   171,   126,
     171,   232,   171,   126,   171,   232,   171,   126,   171,    89,
     125,   246,   125,   232,   123,   123,   126,    35,    84,   109,
     245,   234,   125,   242,   242,   243,   245,   106,   237,   238,
     171,   123,   171,   171,   171,   171,   228,   214,   194,   228,
     106,    45,   237,   123,   106,   124,   244,   125,   232,    49,
     232,   123,   195,   126,   125,   106,   107,   230,   125,    65,
      44,   197,   118,    61,   126,   230,   106,   107,    24,   125,
     104,   177,   124,   235,   125
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
#line 445 "ascend/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in definition.");
	}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 452 "ascend/compiler/ascParse.y"
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
#line 497 "ascend/compiler/ascParse.y"
    {
	  Asc_ScannerPushBuffer((yyvsp[(2) - (3)].dquote_ptr));
	}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 501 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 506 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 514 "ascend/compiler/ascParse.y"
    {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[(2) - (3)].dquote_ptr));
	}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 518 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 523 "ascend/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 531 "ascend/compiler/ascParse.y"
    {
	  if(package_load((yyvsp[(4) - (5)].dquote_ptr),SCP((yyvsp[(2) - (5)].id_ptr)))){
		ErrMsg_Generic("IMPORT failed");
      }
	}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 537 "ascend/compiler/ascParse.y"
    {
	  if(package_load(SCP((yyvsp[(2) - (3)].dquote_ptr)),NULL)){
	    ErrMsg_Generic("IMPORT failed");
	  }
	}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 546 "ascend/compiler/ascParse.y"
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
#line 577 "ascend/compiler/ascParse.y"
    {
	  g_type_name = (yyvsp[(4) - (6)].id_ptr);
	  g_proc_name = (yyvsp[(5) - (6)].id_ptr);
	}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 585 "ascend/compiler/ascParse.y"
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
#line 608 "ascend/compiler/ascParse.y"
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
#line 621 "ascend/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 629 "ascend/compiler/ascParse.y"
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
#line 650 "ascend/compiler/ascParse.y"
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
#line 662 "ascend/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 670 "ascend/compiler/ascParse.y"
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
#line 722 "ascend/compiler/ascParse.y"
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
#line 733 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 741 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 747 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 752 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 759 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 766 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 0;
	}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 776 "ascend/compiler/ascParse.y"
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
#line 816 "ascend/compiler/ascParse.y"
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
#line 849 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 855 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 860 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 867 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 874 "ascend/compiler/ascParse.y"
    {
	  (yyval.sym_ptr) = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 884 "ascend/compiler/ascParse.y"
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
#line 926 "ascend/compiler/ascParse.y"
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
#line 935 "ascend/compiler/ascParse.y"
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
#line 947 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 955 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 959 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 966 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 970 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(3) - (4)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 977 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 981 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 988 "ascend/compiler/ascParse.y"
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
#line 1020 "ascend/compiler/ascParse.y"
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
#line 1033 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 1037 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 1044 "ascend/compiler/ascParse.y"
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
#line 1074 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 1083 "ascend/compiler/ascParse.y"
    { /* nothing to do. just cruft to fix ; problem */ }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 1088 "ascend/compiler/ascParse.y"
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
#line 1107 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(100L);
	}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 1111 "ascend/compiler/ascParse.y"
    {
	  gl_append_ptr((yyvsp[(1) - (2)].listp),(char *)(yyvsp[(2) - (2)].udefptr));
	  (yyval.listp) = (yyvsp[(1) - (2)].listp);
	}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 1119 "ascend/compiler/ascParse.y"
    {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[(1) - (4)].id_ptr),(yyvsp[(3) - (4)].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 1128 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = NULL;
	}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 1132 "ascend/compiler/ascParse.y"
    { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 1139 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(3) - (3)].listp);
	}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 1146 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(1) - (1)].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 1153 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 1157 "ascend/compiler/ascParse.y"
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
#line 1182 "ascend/compiler/ascParse.y"
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
#line 1196 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_proc_name = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 1205 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = CreateStatementList((yyvsp[(1) - (1)].listp));
	}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 1212 "ascend/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 1216 "ascend/compiler/ascParse.y"
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
#line 1224 "ascend/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1268 "ascend/compiler/ascParse.y"
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

  case 117:

/* Line 1455 of yacc.c  */
#line 1309 "ascend/compiler/ascParse.y"
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

  case 118:

/* Line 1455 of yacc.c  */
#line 1338 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateALIASES((yyvsp[(1) - (3)].lptr),(yyvsp[(3) - (3)].nptr));
	}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1343 "ascend/compiler/ascParse.y"
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

  case 120:

/* Line 1455 of yacc.c  */
#line 1392 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1396 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1403 "ascend/compiler/ascParse.y"
    {
	  if (FindType((yyvsp[(3) - (4)].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[(3) - (4)].id_ptr)));
	  }
	}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1415 "ascend/compiler/ascParse.y"
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

  case 124:

/* Line 1455 of yacc.c  */
#line 1442 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_callargs = NULL;
	}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1447 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_callargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1455 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_typeargs = NULL;
	}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1460 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_typeargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1468 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1472 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1479 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1483 "ascend/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1490 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = NULL;
	}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1494 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1501 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateAA((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1508 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateATS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1515 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1522 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWNBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1529 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1533 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1540 "ascend/compiler/ascParse.y"
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

  case 141:

/* Line 1455 of yacc.c  */
#line 1553 "ascend/compiler/ascParse.y"
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

  case 142:

/* Line 1455 of yacc.c  */
#line 1570 "ascend/compiler/ascParse.y"
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

  case 143:

/* Line 1455 of yacc.c  */
#line 1580 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1588 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1599 "ascend/compiler/ascParse.y"
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

  case 146:

/* Line 1455 of yacc.c  */
#line 1620 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1627 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1634 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1638 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (4)].nptr);
	}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1645 "ascend/compiler/ascParse.y"
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

  case 151:

/* Line 1455 of yacc.c  */
#line 1665 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1669 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (2)].nptr);
	}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1677 "ascend/compiler/ascParse.y"
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

  case 154:

/* Line 1455 of yacc.c  */
#line 1697 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_random;
	}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1701 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_increasing;
	}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1705 "ascend/compiler/ascParse.y"
    {
	  (yyval.order) = f_decreasing;
	}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1712 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1716 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1720 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1724 "ascend/compiler/ascParse.y"
    {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1731 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(2) - (2)].nptr),NULL);
	}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1735 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(4) - (4)].nptr),(yyvsp[(2) - (4)].nptr));	  /* type :: name */
	}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1742 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1750 "ascend/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateFREE((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1757 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[(2) - (2)].id_ptr)));
	}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1765 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[(2) - (3)].id_ptr)),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1773 "ascend/compiler/ascParse.y"
    {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE();
	}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1781 "ascend/compiler/ascParse.y"
    {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[(2) - (5)].id_ptr)),(yyvsp[(4) - (5)].lptr));
	}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1792 "ascend/compiler/ascParse.y"
    {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[(2) - (2)].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1803 "ascend/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateASSERT((yyvsp[(2) - (2)].eptr));
	}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1809 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[(2) - (6)].eptr),(yyvsp[(4) - (6)].slptr),(yyvsp[(5) - (6)].slptr));
	}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1819 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(5) - (5)].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[(5) - (5)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[(2) - (5)].eptr),(yyvsp[(4) - (5)].slptr));
	}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1828 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1832 "ascend/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (2)].slptr);
	}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1839 "ascend/compiler/ascParse.y"
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

  case 176:

/* Line 1455 of yacc.c  */
#line 1850 "ascend/compiler/ascParse.y"
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

  case 177:

/* Line 1455 of yacc.c  */
#line 1862 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].wptr));
	}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1869 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(8) - (8)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(8) - (8)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[(1) - (8)].nptr),(yyvsp[(5) - (8)].lptr),(yyvsp[(7) - (8)].wptr));
	}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1879 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[(1) - (1)].wptr));
	}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1886 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1890 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1894 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].wptr));
	}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1898 "ascend/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].wptr));
	}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1905 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1909 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1913 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1917 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1921 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[(2) - (2)].braced_ptr));
	}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1928 "ascend/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFNAME((yyvsp[(2) - (2)].nptr));
	}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1935 "ascend/compiler/ascParse.y"
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

  case 191:

/* Line 1455 of yacc.c  */
#line 1946 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].septr));
	}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1956 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = ReverseSelectCases((yyvsp[(1) - (1)].septr));
	}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1963 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1967 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1971 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].septr));
	}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1975 "ascend/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].septr));
	}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1982 "ascend/compiler/ascParse.y"
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

  case 198:

/* Line 1455 of yacc.c  */
#line 1993 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].swptr));
	}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 2003 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[(1) - (1)].swptr));
	}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 2010 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 2014 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 2018 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].swptr));
	}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 2022 "ascend/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].swptr));
	}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 2029 "ascend/compiler/ascParse.y"
    {
	  if( (yyvsp[(3) - (3)].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[(3) - (3)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[(2) - (3)].slptr));
	}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 2039 "ascend/compiler/ascParse.y"
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

  case 206:

/* Line 1455 of yacc.c  */
#line 2076 "ascend/compiler/ascParse.y"
    {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[(2) - (2)].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[(1) - (2)].sym_ptr);
	}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 2085 "ascend/compiler/ascParse.y"
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

  case 208:

/* Line 1455 of yacc.c  */
#line 2100 "ascend/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(2) - (2)].braced_ptr),NULL),
	                     (void *)(yyvsp[(1) - (2)].lptr), LineNum());
	}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 2105 "ascend/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(3) - (3)].braced_ptr),NULL),
	                     (void *)(yyvsp[(2) - (3)].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[(1) - (3)].notesptr));
	}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 2114 "ascend/compiler/ascParse.y"
    {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[(1) - (1)].lptr));
	}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 2125 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 2129 "ascend/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(3) - (3)].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[(1) - (3)].lptr));
	}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 2134 "ascend/compiler/ascParse.y"
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

  case 214:

/* Line 1455 of yacc.c  */
#line 2147 "ascend/compiler/ascParse.y"
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

  case 215:

/* Line 1455 of yacc.c  */
#line 2167 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(1) - (1)].id_ptr));
	}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 2171 "ascend/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(3) - (3)].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[(1) - (3)].nptr));
	}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 2176 "ascend/compiler/ascParse.y"
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

  case 218:

/* Line 1455 of yacc.c  */
#line 2191 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 2196 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 2201 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 2206 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 2211 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 2216 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 2221 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 2226 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 2231 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 2236 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 2241 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 2246 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 2251 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = (yyvsp[(2) - (2)].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 2256 "ascend/compiler/ascParse.y"
    {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 2264 "ascend/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = NULL;
	}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 2268 "ascend/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = (yyvsp[(1) - (1)].braced_ptr);
	}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 2275 "ascend/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = NULL;
	}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 2279 "ascend/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = (yyvsp[(1) - (1)].dquote_ptr);
	}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 2286 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = ReverseSetList((yyvsp[(1) - (1)].sptr));
	}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 2290 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 2297 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(1) - (1)].eptr));
	}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 2301 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 2305 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(3) - (3)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (3)].sptr));
	}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 2310 "ascend/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(3) - (5)].eptr),(yyvsp[(5) - (5)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (5)].sptr));
	}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 2318 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 2324 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 2333 "ascend/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (2)].real_value)*(yyvsp[(2) - (2)].real_value);
	}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 2337 "ascend/compiler/ascParse.y"
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

  case 246:

/* Line 1455 of yacc.c  */
#line 2358 "ascend/compiler/ascParse.y"
    {
	  g_dim_ptr = Dimensionless();
	  (yyval.real_value) = 1.0;
	}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 2363 "ascend/compiler/ascParse.y"
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

  case 248:

/* Line 1455 of yacc.c  */
#line 2384 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = (yyvsp[(2) - (2)].dimp);
	}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 2388 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = Dimensionless();
	}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 2392 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 2399 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 2403 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[(1) - (1)].dimen)));
	}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 2410 "ascend/compiler/ascParse.y"
    {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[(1) - (1)].id_ptr)));
	}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 2414 "ascend/compiler/ascParse.y"
    {
	  ClearDimensions(&((yyval.dimen)));
	}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 2418 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = SubDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 2422 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = AddDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 2426 "ascend/compiler/ascParse.y"
    {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[(1) - (3)].dimen)),(yyvsp[(3) - (3)].frac_value));
	}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 2430 "ascend/compiler/ascParse.y"
    {
	  CopyDimensions(&((yyvsp[(2) - (3)].dimen)),&((yyval.dimen)));
	}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 2437 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = (yyvsp[(1) - (2)].int_value) ? NegateF((yyvsp[(2) - (2)].frac_value)) : (yyvsp[(2) - (2)].frac_value);
	}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 2444 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(1) - (1)].int_value),(short)1);
	}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 2448 "ascend/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(2) - (5)].int_value),(short)(yyvsp[(4) - (5)].int_value));
	}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 2455 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 2459 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 2463 "ascend/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 2470 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr((yyvsp[(1) - (1)].int_value));
	}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 2474 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 2478 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr((yyvsp[(1) - (1)].real_value),g_dim_ptr);
	}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 2482 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 2486 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateTrueExpr();
	}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2490 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateFalseExpr();
	}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 2494 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateAnyExpr();
	}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 2498 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[(1) - (1)].sym_ptr));
	}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 2502 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateVarExpr((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 2506 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSetExpr((yyvsp[(2) - (3)].sptr));
	}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 2510 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 2515 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 2520 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 2525 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 2530 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 2535 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 2540 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 2545 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_not));
	}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 2549 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 2554 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 2559 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 2564 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 2569 "ascend/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 2574 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 2578 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_uminus));
	}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 2582 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].real_value),g_dim_ptr);
	}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 2586 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (4)].nptr),DBL_MAX,NULL);
	}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 2590 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 2597 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 2601 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 2608 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 2612 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 2619 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 2623 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 2630 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 2634 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 2641 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 2645 "ascend/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 2652 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 2656 "ascend/compiler/ascParse.y"
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

  case 305:

/* Line 1455 of yacc.c  */
#line 2667 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (3)].eptr);
	}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 2674 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 2678 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 2682 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 2686 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 2690 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 2694 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 2701 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 2705 "ascend/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
    break;



/* Line 1455 of yacc.c  */
#line 5780 "ascend/compiler/ascParse.c"
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
#line 2709 "ascend/compiler/ascParse.y"

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

