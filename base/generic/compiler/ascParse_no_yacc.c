/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse zz_parse
#define yylex   zz_lex
#define yyerror zz_error
#define yylval  zz_lval
#define yychar  zz_char
#define yydebug zz_debug
#define yynerrs zz_nerrs


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
     OR_TOK = 317,
     OTHERWISE_TOK = 318,
     OUTPUT_TOK = 319,
     PATCH_TOK = 320,
     PROD_TOK = 321,
     PROVIDE_TOK = 322,
     REFINES_TOK = 323,
     REPLACE_TOK = 324,
     REQUIRE_TOK = 325,
     RETURN_TOK = 326,
     RUN_TOK = 327,
     SATISFIED_TOK = 328,
     SELECT_TOK = 329,
     SIZE_TOK = 330,
     STOP_TOK = 331,
     SUCHTHAT_TOK = 332,
     SUM_TOK = 333,
     SWITCH_TOK = 334,
     THEN_TOK = 335,
     TRUE_TOK = 336,
     UNION_TOK = 337,
     UNITS_TOK = 338,
     UNIVERSAL_TOK = 339,
     WHEN_TOK = 340,
     WHERE_TOK = 341,
     WHILE_TOK = 342,
     WILLBE_TOK = 343,
     WILLBETHESAME_TOK = 344,
     WILLNOTBETHESAME_TOK = 345,
     ASSIGN_TOK = 346,
     CASSIGN_TOK = 347,
     DBLCOLON_TOK = 348,
     USE_TOK = 349,
     LEQ_TOK = 350,
     GEQ_TOK = 351,
     NEQ_TOK = 352,
     DOTDOT_TOK = 353,
     WITH_TOK = 354,
     VALUE_TOK = 355,
     WITH_VALUE_T = 356,
     REAL_TOK = 357,
     INTEGER_TOK = 358,
     IDENTIFIER_TOK = 359,
     BRACEDTEXT_TOK = 360,
     SYMBOL_TOK = 361,
     DQUOTE_TOK = 362,
     UPLUS_TOK = 363,
     UMINUS_TOK = 364
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
#define OR_TOK 317
#define OTHERWISE_TOK 318
#define OUTPUT_TOK 319
#define PATCH_TOK 320
#define PROD_TOK 321
#define PROVIDE_TOK 322
#define REFINES_TOK 323
#define REPLACE_TOK 324
#define REQUIRE_TOK 325
#define RETURN_TOK 326
#define RUN_TOK 327
#define SATISFIED_TOK 328
#define SELECT_TOK 329
#define SIZE_TOK 330
#define STOP_TOK 331
#define SUCHTHAT_TOK 332
#define SUM_TOK 333
#define SWITCH_TOK 334
#define THEN_TOK 335
#define TRUE_TOK 336
#define UNION_TOK 337
#define UNITS_TOK 338
#define UNIVERSAL_TOK 339
#define WHEN_TOK 340
#define WHERE_TOK 341
#define WHILE_TOK 342
#define WILLBE_TOK 343
#define WILLBETHESAME_TOK 344
#define WILLNOTBETHESAME_TOK 345
#define ASSIGN_TOK 346
#define CASSIGN_TOK 347
#define DBLCOLON_TOK 348
#define USE_TOK 349
#define LEQ_TOK 350
#define GEQ_TOK 351
#define NEQ_TOK 352
#define DOTDOT_TOK 353
#define WITH_TOK 354
#define VALUE_TOK 355
#define WITH_VALUE_T 356
#define REAL_TOK 357
#define INTEGER_TOK 358
#define IDENTIFIER_TOK 359
#define BRACEDTEXT_TOK 360
#define SYMBOL_TOK 361
#define DQUOTE_TOK 362
#define UPLUS_TOK 363
#define UMINUS_TOK 364




/* Copy the first part of user declarations.  */
#line 31 "base/generic/compiler/ascParse.y"

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>		/* need DBL_MAX and LONG_MAX */
#include <float.h>		/* on a NeXT they are in here */

#include <compiler/parser.h>

#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/scanner.h>
#include <compiler/symtab.h>		/* the global string/symbol table */
#include <compiler/notate.h>		/* notes database wrapper */
#include <compiler/braced.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/functype.h>
#include <compiler/func.h>
#include <compiler/expr_types.h>
#include <compiler/name.h>
#include <compiler/nameio.h>
#include <compiler/instance_enum.h>
#include <compiler/extfunc.h>
#include <compiler/packages.h>
#include <compiler/sets.h>
#include <compiler/exprs.h>
#include <compiler/exprio.h>
#include <compiler/vlist.h>
#include <compiler/vlistio.h>		/* for debugging only */
#include <compiler/stattypes.h>
#include <compiler/slist.h>
#include <compiler/statement.h>
#include <compiler/statio.h>
#include <compiler/units.h>
#include <compiler/when.h>
#include <compiler/select.h>
#include <compiler/switch.h>
#include <compiler/proc.h>
#include <compiler/watchpt.h>
#include <compiler/module.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/type_descio.h>
#include <compiler/typedef.h>
#include <compiler/library.h>
#include <compiler/syntax.h>
#include <compiler/lexer.h>

/* 1 ==> expr can find missing , w/o  shift/reduce conflicts */
#define COMMAEXPR_NOTBUGGY 0 
#if COMMAEXPR_NOTBUGGY
#include <compiler/exprio.h>
#endif /* for CommaExpr if working. */

int g_compiler_warnings = 1;		/* level of whine to allow */

#include <compiler/redirectFile.h>
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

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 304 "base/generic/compiler/ascParse.y"
{
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
}
/* Line 187 of yacc.c.  */
#line 619 "base/generic/compiler/ascParse.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 632 "base/generic/compiler/ascParse.c"

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
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
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
  yytype_int16 yyss;
  YYSTYPE yyvs;
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
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1102

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  127
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  116
/* YYNRULES -- Number of rules.  */
#define YYNRULES  307
/* YYNRULES -- Number of states.  */
#define YYNSTATES  616

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   364

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     121,   122,   116,   113,   108,   114,   124,   115,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   123,   120,
     110,   111,   112,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   125,     2,   126,   119,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   109,     2,     2,     2,     2,     2,
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
     105,   106,   107,   117,   118
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
     343,   345,   347,   349,   355,   361,   365,   378,   379,   384,
     389,   393,   395,   400,   402,   407,   408,   411,   412,   415,
     416,   419,   422,   425,   428,   431,   435,   439,   441,   445,
     447,   450,   453,   463,   467,   471,   472,   477,   487,   488,
     491,   500,   501,   503,   505,   507,   509,   511,   513,   516,
     521,   524,   527,   533,   536,   539,   546,   552,   553,   556,
     561,   568,   575,   584,   586,   591,   595,   601,   606,   608,
     610,   612,   614,   617,   620,   625,   632,   634,   639,   643,
     649,   654,   659,   666,   668,   673,   677,   683,   688,   692,
     696,   699,   703,   706,   710,   712,   714,   718,   721,   724,
     726,   730,   735,   738,   741,   744,   747,   750,   753,   756,
     759,   762,   765,   768,   771,   774,   776,   777,   779,   780,
     782,   784,   785,   787,   791,   795,   801,   803,   805,   808,
     811,   812,   814,   817,   819,   820,   822,   824,   826,   828,
     832,   836,   840,   844,   847,   849,   855,   856,   858,   860,
     862,   864,   866,   868,   870,   872,   874,   876,   878,   882,
     886,   890,   894,   898,   902,   906,   910,   913,   917,   921,
     925,   929,   933,   936,   939,   946,   951,   956,   961,   966,
     971,   976,   981,   986,   991,   996,  1001,  1006,  1011,  1016,
    1020,  1022,  1024,  1026,  1028,  1030,  1032,  1034
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     128,     0,    -1,    -1,   128,   129,    -1,   131,    -1,   132,
      -1,   133,    -1,   136,    -1,   138,    -1,   134,    -1,   144,
      -1,   140,    -1,   147,    -1,   156,    -1,   153,    -1,   158,
      -1,   130,    -1,     1,    -1,    41,   120,   168,   226,   120,
      -1,    70,   107,   120,    -1,    70,   225,   120,    -1,    70,
     225,    -1,    67,   107,   120,    -1,    67,   225,   120,    -1,
      67,   225,    -1,    43,   104,    40,   107,   120,    -1,    43,
     107,   120,    -1,   135,   220,   226,   120,    -1,     3,    60,
      44,   104,   179,   120,    -1,   137,   164,   226,   120,    -1,
       3,    55,    44,   104,   120,    -1,     3,    55,    44,    27,
      58,   120,    -1,   139,   164,   226,   120,    -1,    69,    55,
      44,   104,   120,    -1,    69,    55,    44,    27,    58,   120,
      -1,   155,   141,   168,   162,   226,   120,    -1,   142,    68,
     104,   234,   143,   120,    -1,    11,   104,    -1,    -1,    26,
     239,   231,    -1,    26,    35,    -1,    26,    81,    -1,    26,
     106,    -1,   155,   145,    -1,    21,   104,    68,   104,   234,
     146,   228,   120,    -1,    -1,    92,   239,   231,    -1,    92,
      81,    -1,    92,    35,    -1,    92,   106,    -1,   155,   148,
     168,   162,   226,   120,    -1,   149,   150,   151,   120,    -1,
     149,   150,   151,    68,   104,   152,   120,    -1,    58,   104,
      -1,    -1,   121,   168,   122,    -1,    -1,    86,   121,   168,
     122,    -1,    -1,   121,   168,   122,    -1,   154,   168,   162,
     226,   120,    -1,    65,   104,    38,   104,   120,    -1,    -1,
      84,    -1,   157,   168,   162,   226,   120,    -1,    27,   104,
      -1,   159,   120,    -1,    83,   160,   226,    -1,    -1,   160,
     161,    -1,   104,   111,   105,   120,    -1,    -1,    -1,    55,
     163,   164,    -1,   165,    -1,    -1,   165,   166,    -1,   167,
     120,   168,   226,   120,    -1,    56,   104,    -1,   169,    -1,
      -1,   169,   170,   120,    -1,   169,     1,   120,    -1,   171,
      -1,   172,    -1,   173,    -1,   175,    -1,   176,    -1,   182,
      -1,   183,    -1,   184,    -1,   185,    -1,   186,    -1,   187,
      -1,   193,    -1,   189,    -1,   202,    -1,   201,    -1,   195,
      -1,   198,    -1,   199,    -1,   200,    -1,   203,    -1,   204,
      -1,   205,    -1,   207,    -1,   211,    -1,   210,    -1,   212,
      -1,   215,    -1,   218,    -1,   219,    -1,   159,    -1,   222,
      49,   178,   180,   181,    -1,   222,    88,   178,   180,   181,
      -1,   222,     4,   224,    -1,   222,     4,   121,   222,   122,
      86,   222,    49,   104,    61,   104,   174,    -1,    -1,   101,
     121,   229,   122,    -1,   222,    50,   104,   180,    -1,   222,
      51,   178,    -1,   104,    -1,   104,   121,   229,   122,    -1,
     104,    -1,   104,   121,   229,   122,    -1,    -1,    56,   104,
      -1,    -1,    61,   104,    -1,    -1,   101,   240,    -1,   222,
       7,    -1,   222,     8,    -1,   222,    89,    -1,   222,    90,
      -1,   224,    91,   240,    -1,   224,    92,   240,    -1,   188,
      -1,   224,   123,   188,    -1,   240,    -1,    57,   240,    -1,
      52,   240,    -1,   224,   123,   104,   121,   190,   120,   191,
     192,   122,    -1,   222,   123,    45,    -1,   222,   123,    64,
      -1,    -1,   120,   224,   123,    24,    -1,   224,   123,   104,
     121,   222,   120,   103,   122,   194,    -1,    -1,    44,   224,
      -1,    38,   104,    44,   240,   196,   197,   168,   226,    -1,
      -1,    46,    -1,    25,    -1,    23,    -1,    33,    -1,    19,
      -1,    30,    -1,    72,   224,    -1,    72,   224,    93,   224,
      -1,    37,   222,    -1,    39,   222,    -1,    34,   104,   121,
     222,   122,    -1,    15,   177,    -1,    10,   240,    -1,    42,
     240,    80,   168,   206,   226,    -1,    87,   240,    30,   168,
     226,    -1,    -1,    31,   168,    -1,    85,   222,   208,   226,
      -1,   224,   123,    85,   222,   208,   226,    -1,    85,   121,
     222,   122,   208,   226,    -1,   224,   123,    85,   121,   222,
     122,   208,   226,    -1,   209,    -1,    17,   229,   123,   168,
      -1,    63,   123,   168,    -1,   209,    17,   229,   123,   168,
      -1,   209,    63,   123,   168,    -1,    14,    -1,    22,    -1,
      36,    -1,    71,    -1,    76,   227,    -1,    94,   224,    -1,
      74,   222,   213,   226,    -1,    74,   121,   222,   122,   213,
     226,    -1,   214,    -1,    17,   229,   123,   168,    -1,    63,
     123,   168,    -1,   214,    17,   229,   123,   168,    -1,   214,
      63,   123,   168,    -1,    79,   222,   216,   226,    -1,    79,
     121,   222,   122,   216,   226,    -1,   217,    -1,    17,   229,
     123,   168,    -1,    63,   123,   168,    -1,   217,    17,   229,
     123,   168,    -1,   217,    63,   123,   168,    -1,    20,   168,
     226,    -1,    60,   220,   226,    -1,   106,   221,    -1,   220,
     106,   221,    -1,   222,   105,    -1,   221,   222,   105,    -1,
     223,    -1,   224,    -1,   223,   108,   224,    -1,   223,   224,
      -1,   225,   228,    -1,   104,    -1,   225,   124,   104,    -1,
     225,   125,   229,   126,    -1,    32,    20,    -1,    32,    38,
      -1,    32,    42,    -1,    32,    47,    -1,    32,    55,    -1,
      32,    60,    -1,    32,    74,    -1,    32,    79,    -1,    32,
      83,    -1,    32,    41,    -1,    32,    85,    -1,    32,    87,
      -1,    32,   104,    -1,    32,    -1,    -1,   105,    -1,    -1,
     107,    -1,   230,    -1,    -1,   240,    -1,   240,    98,   240,
      -1,   230,   108,   240,    -1,   230,   108,   240,    98,   240,
      -1,   103,    -1,   232,    -1,   102,   233,    -1,   103,   105,
      -1,    -1,   105,    -1,    28,   235,    -1,    29,    -1,    -1,
     116,    -1,   236,    -1,   104,    -1,   103,    -1,   236,   115,
     236,    -1,   236,   116,   236,    -1,   236,   119,   237,    -1,
     121,   236,   122,    -1,   239,   238,    -1,   103,    -1,   121,
     103,   115,   103,   122,    -1,    -1,   113,    -1,   114,    -1,
     103,    -1,    53,    -1,   232,    -1,    54,    -1,    81,    -1,
      35,    -1,     6,    -1,   106,    -1,   224,    -1,   125,   229,
     126,    -1,   240,   113,   240,    -1,   240,   114,   240,    -1,
     240,   116,   240,    -1,   240,   115,   240,    -1,   240,   119,
     240,    -1,   240,     5,   240,    -1,   240,    62,   240,    -1,
      59,   240,    -1,   240,   241,   240,    -1,   240,   242,   240,
      -1,   240,    44,   240,    -1,   240,   109,   240,    -1,   240,
      77,   240,    -1,   113,   240,    -1,   114,   240,    -1,    73,
     121,   224,   108,   232,   122,    -1,    73,   121,   224,   122,
      -1,    78,   121,   229,   122,    -1,    78,   125,   229,   126,
      -1,    66,   121,   229,   122,    -1,    66,   125,   229,   126,
      -1,    82,   121,   229,   122,    -1,    82,   125,   229,   126,
      -1,    48,   121,   229,   122,    -1,    48,   125,   229,   126,
      -1,    16,   121,   229,   122,    -1,    16,   125,   229,   126,
      -1,    18,   121,   229,   122,    -1,    18,   125,   229,   126,
      -1,   104,   121,   240,   122,    -1,   121,   240,   122,    -1,
     111,    -1,   110,    -1,   112,    -1,    95,    -1,    96,    -1,
      97,    -1,    12,    -1,    13,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   423,   423,   425,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   449,   494,
     498,   503,   511,   515,   520,   528,   534,   543,   574,   582,
     605,   618,   626,   647,   659,   667,   719,   730,   739,   744,
     749,   756,   763,   773,   812,   847,   852,   857,   864,   871,
     881,   922,   931,   944,   953,   956,   964,   967,   975,   978,
     985,  1017,  1031,  1034,  1041,  1071,  1080,  1085,  1105,  1108,
    1116,  1126,  1130,  1129,  1143,  1151,  1154,  1179,  1193,  1202,
    1210,  1213,  1221,  1229,  1230,  1231,  1232,  1233,  1234,  1235,
    1236,  1237,  1238,  1239,  1240,  1241,  1242,  1243,  1244,  1245,
    1246,  1247,  1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,
    1256,  1257,  1258,  1262,  1303,  1332,  1336,  1387,  1390,  1397,
    1409,  1436,  1441,  1449,  1454,  1463,  1466,  1474,  1477,  1485,
    1488,  1495,  1502,  1509,  1516,  1523,  1527,  1534,  1547,  1564,
    1574,  1582,  1593,  1606,  1613,  1621,  1624,  1631,  1652,  1655,
    1662,  1684,  1687,  1691,  1698,  1702,  1706,  1710,  1717,  1721,
    1728,  1735,  1742,  1753,  1764,  1770,  1780,  1790,  1793,  1800,
    1811,  1823,  1830,  1840,  1847,  1851,  1855,  1859,  1866,  1870,
    1874,  1878,  1882,  1889,  1896,  1907,  1917,  1924,  1928,  1932,
    1936,  1943,  1954,  1964,  1971,  1975,  1979,  1983,  1990,  2000,
    2037,  2046,  2061,  2066,  2075,  2086,  2090,  2095,  2108,  2128,
    2132,  2137,  2152,  2157,  2162,  2167,  2172,  2177,  2182,  2187,
    2192,  2197,  2202,  2207,  2212,  2217,  2226,  2229,  2237,  2240,
    2247,  2252,  2258,  2262,  2266,  2271,  2279,  2285,  2294,  2298,
    2320,  2324,  2345,  2349,  2354,  2360,  2364,  2371,  2375,  2379,
    2383,  2387,  2391,  2398,  2405,  2409,  2417,  2420,  2424,  2431,
    2435,  2439,  2443,  2447,  2451,  2455,  2459,  2463,  2467,  2471,
    2476,  2481,  2486,  2491,  2496,  2501,  2506,  2510,  2515,  2520,
    2525,  2530,  2535,  2539,  2543,  2547,  2551,  2558,  2562,  2569,
    2573,  2580,  2584,  2591,  2595,  2602,  2606,  2613,  2617,  2628,
    2635,  2639,  2643,  2647,  2651,  2655,  2662,  2666
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
  "NOT_TOK", "NOTES_TOK", "OF_TOK", "OR_TOK", "OTHERWISE_TOK",
  "OUTPUT_TOK", "PATCH_TOK", "PROD_TOK", "PROVIDE_TOK", "REFINES_TOK",
  "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK", "SATISFIED_TOK",
  "SELECT_TOK", "SIZE_TOK", "STOP_TOK", "SUCHTHAT_TOK", "SUM_TOK",
  "SWITCH_TOK", "THEN_TOK", "TRUE_TOK", "UNION_TOK", "UNITS_TOK",
  "UNIVERSAL_TOK", "WHEN_TOK", "WHERE_TOK", "WHILE_TOK", "WILLBE_TOK",
  "WILLBETHESAME_TOK", "WILLNOTBETHESAME_TOK", "ASSIGN_TOK", "CASSIGN_TOK",
  "DBLCOLON_TOK", "USE_TOK", "LEQ_TOK", "GEQ_TOK", "NEQ_TOK", "DOTDOT_TOK",
  "WITH_TOK", "VALUE_TOK", "WITH_VALUE_T", "REAL_TOK", "INTEGER_TOK",
  "IDENTIFIER_TOK", "BRACEDTEXT_TOK", "SYMBOL_TOK", "DQUOTE_TOK", "','",
  "'|'", "'<'", "'='", "'>'", "'+'", "'-'", "'/'", "'*'", "UPLUS_TOK",
  "UMINUS_TOK", "'^'", "';'", "'('", "')'", "':'", "'.'", "'['", "']'",
  "$accept", "definitions", "definition", "global_def", "require_file",
  "provide_module", "import", "add_notes_def", "add_notes_head",
  "add_method_def", "add_method_head", "replace_method_def",
  "replace_method_head", "atom_def", "atom_head", "atom_id", "default_val",
  "constant_def", "constant_head", "constant_val", "model_def",
  "model_head", "model_id", "optional_model_parameters",
  "optional_parameter_wheres", "optional_parameter_reduction", "patch_def",
  "patch_head", "universal", "definition_def", "definition_id",
  "units_def", "units_statement", "unitdeflist", "unitdef", "methods",
  "@1", "proclist", "proclistf", "procedure", "procedure_id",
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
  "free_statement", "external_statement", "call_statement",
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
     355,   356,   357,   358,   359,   360,   361,   362,    44,   124,
      60,    61,    62,    43,    45,    47,    42,   363,   364,    94,
      59,    40,    41,    58,    46,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   127,   128,   128,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   130,   131,
     131,   131,   132,   132,   132,   133,   133,   134,   135,   136,
     137,   137,   138,   139,   139,   140,   141,   142,   143,   143,
     143,   143,   143,   144,   145,   146,   146,   146,   146,   146,
     147,   148,   148,   149,   150,   150,   151,   151,   152,   152,
     153,   154,   155,   155,   156,   157,   158,   159,   160,   160,
     161,   162,   163,   162,   164,   165,   165,   166,   167,   168,
     169,   169,   169,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   171,   172,   173,   173,   174,   174,   175,
     176,   177,   177,   178,   178,   179,   179,   180,   180,   181,
     181,   182,   183,   184,   185,   186,   186,   187,   187,   188,
     188,   188,   189,   190,   191,   192,   192,   193,   194,   194,
     195,   196,   196,   196,   197,   197,   197,   197,   198,   198,
     199,   200,   201,   202,   203,   204,   205,   206,   206,   207,
     207,   207,   207,   208,   209,   209,   209,   209,   210,   210,
     210,   210,   210,   211,   212,   212,   213,   214,   214,   214,
     214,   215,   215,   216,   217,   217,   217,   217,   218,   219,
     220,   220,   221,   221,   222,   223,   223,   223,   224,   225,
     225,   225,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   227,   227,   228,   228,
     229,   229,   230,   230,   230,   230,   231,   231,   232,   232,
     233,   233,   234,   234,   234,   235,   235,   236,   236,   236,
     236,   236,   236,   237,   238,   238,   239,   239,   239,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     241,   241,   241,   241,   241,   241,   242,   242
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
       1,     1,     1,     5,     5,     3,    12,     0,     4,     4,
       3,     1,     4,     1,     4,     0,     2,     0,     2,     0,
       2,     2,     2,     2,     2,     3,     3,     1,     3,     1,
       2,     2,     9,     3,     3,     0,     4,     9,     0,     2,
       8,     0,     1,     1,     1,     1,     1,     1,     2,     4,
       2,     2,     5,     2,     2,     6,     5,     0,     2,     4,
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

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       2,     0,     1,    17,     0,     0,     0,     0,     0,     0,
       0,     0,    68,    63,     3,    16,     4,     5,     6,     9,
       0,     7,    75,     8,    75,    11,    10,    12,    14,    80,
       0,    13,    80,    15,     0,     0,     0,    65,    80,     0,
       0,     0,   209,     0,    24,     0,     0,    21,     0,     0,
       0,     0,    74,     0,    71,     0,     0,     0,     0,    80,
       0,    43,    80,    54,    71,    66,     0,     0,     0,     0,
      26,     0,    22,    23,     0,   231,     0,    19,    20,   225,
       0,    69,    67,   200,     0,   204,   205,   228,     0,     0,
       0,     0,    76,     0,     0,    72,     0,     0,   265,     0,
     178,     0,     0,     0,    80,   179,     0,   264,   180,     0,
       0,     0,     0,     0,     0,   260,   262,     0,     0,     0,
       0,   181,     0,     0,     0,   226,     0,     0,   263,     0,
       0,     0,     0,   240,   259,   209,   266,     0,     0,     0,
     231,   112,     0,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,   137,    95,    94,    98,    99,   100,
     101,    97,    96,   102,   103,   104,   105,   107,   106,   108,
     109,   110,   111,     0,   267,   261,   139,    37,     0,    53,
      71,     0,    71,    80,    56,     0,     0,     0,   125,     0,
       0,     0,   210,   267,     0,   230,   232,     0,     0,   212,
     213,   221,   214,   215,   216,   217,   218,   219,   220,   222,
     223,   224,     0,     0,   202,     0,   207,   229,   208,   201,
      27,    29,    78,    80,    32,    75,     0,    82,   164,   121,
     163,   231,   231,   231,   231,     0,     0,   160,     0,   161,
       0,   231,   231,   141,   140,   276,     0,   231,   231,   158,
       0,     0,     0,   227,   182,   231,   231,     0,     0,   231,
     231,     0,     0,     0,   183,   241,   238,   239,     0,   282,
     283,     0,     0,    81,     0,   131,   132,     0,     0,     0,
       0,   133,   134,     0,     0,     0,     0,   306,   307,     0,
       0,     0,   303,   304,   305,     0,   301,   300,   302,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   244,     0,
       0,     0,     0,     0,     0,    30,     0,     0,    18,    25,
      61,   211,     0,     0,     0,    33,     0,   203,   206,     0,
      73,    60,   231,     0,     0,     0,     0,   198,     0,     0,
      80,     0,     0,   199,     0,     0,     0,     0,     0,   231,
       0,     0,   186,     0,     0,     0,   231,     0,     0,   193,
       0,     0,     0,   231,     0,     0,   173,    80,     0,   299,
     268,     0,   115,   123,   127,   127,   120,   127,   135,   136,
       0,   209,   138,   274,   279,   275,   281,   280,   269,   270,
     272,   271,   273,   277,   278,   244,     0,     0,   243,    38,
       0,    55,    80,     0,    51,    64,    31,   126,    28,   234,
     233,    34,    70,     0,     0,   294,   295,   296,   297,     0,
     151,   167,   292,   293,   288,   289,   159,     0,   285,     0,
       0,    80,   184,   231,     0,   286,   287,     0,     0,    80,
     191,   231,     0,   290,   291,     0,     0,    80,   169,   231,
       0,     0,   298,     0,   231,     0,   129,   119,   129,     0,
       0,     0,    45,    35,   248,   247,   245,     0,   242,   246,
     256,     0,    50,     0,    58,     0,    77,   122,   162,   153,
     152,     0,    80,     0,     0,     0,     0,    80,   188,     0,
      80,     0,    80,   195,     0,    80,     0,    80,   175,     0,
      80,   166,     0,     0,   128,     0,   113,   114,     0,     0,
       0,     0,   267,   256,   228,     0,     0,     0,   256,    40,
      41,    42,   257,   258,     0,    36,    57,    80,     0,   235,
     156,   154,   157,   155,    80,   168,   165,   284,   185,   187,
      80,   190,   192,   194,    80,   197,   171,   174,    80,   177,
       0,   124,   130,     0,   170,     0,     0,     0,    48,    47,
      49,     0,     0,   252,   249,   250,   251,     0,   236,    39,
     237,     0,    52,     0,   189,   196,   176,     0,     0,   145,
       0,     0,   143,    46,    44,   254,     0,   253,    59,   150,
       0,   172,     0,     0,     0,   148,     0,     0,     0,   142,
     144,     0,   147,     0,     0,     0,   149,     0,   117,   146,
     255,     0,   116,   231,     0,   118
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    59,    60,   471,    26,    61,   514,
      27,    62,    63,   184,   312,   528,    28,    29,    30,    31,
      32,    33,    34,    48,    81,    96,   225,    51,    52,    92,
      93,    54,    55,   142,   143,   144,   145,   612,   146,   147,
     230,   374,   317,   456,   506,   148,   149,   150,   151,   152,
     153,   154,   155,   510,   579,   593,   156,   602,   157,   481,
     534,   158,   159,   160,   161,   162,   163,   164,   165,   483,
     166,   365,   366,   167,   168,   169,   351,   352,   170,   358,
     359,   171,   172,    50,    83,    84,    85,   193,    87,    82,
     254,   218,   194,   195,   569,   175,   266,   399,   468,   469,
     566,   587,   524,   196,   304,   305
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -437
static const yytype_int16 yypact[] =
{
    -437,  1006,  -437,  -437,    88,   -83,   -54,   -52,    18,    17,
      81,    79,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
      43,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
       1,  -437,  -437,  -437,    64,   120,   167,  -437,  -437,   164,
     106,   195,  -437,   134,    31,   214,   140,    47,   -13,   166,
     -22,   248,   228,   248,   230,   531,   182,   183,   184,  -437,
     222,  -437,  -437,   170,   230,  -437,   -12,   188,   248,   186,
    -437,   190,  -437,  -437,   191,   681,   -11,  -437,  -437,   998,
     187,  -437,  -437,   166,   194,    77,  -437,     9,   166,   180,
     181,   198,  -437,   185,   189,  -437,   248,   193,  -437,   681,
    -437,   202,    92,   103,  -437,  -437,   203,  -437,  -437,   166,
     204,   166,   681,   125,   681,  -437,  -437,   681,   681,    43,
     130,  -437,   166,   197,   -81,   206,   131,   -78,  -437,   132,
     -53,   681,   166,   210,   216,   205,  -437,   681,   681,   681,
     681,  -437,   208,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,    21,    50,  -437,   889,  -437,   255,  -437,
     230,   220,   230,  -437,   241,   248,   272,   213,   283,   224,
     225,   229,  -437,  -437,   231,   240,   737,   300,   239,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,   256,   257,  -437,   166,  -437,  -437,  -437,   166,
    -437,  -437,  -437,  -437,  -437,  -437,   243,  -437,   889,   247,
    -437,   681,   681,   681,   681,   248,   249,  -437,   316,  -437,
     748,   681,   681,   889,   889,  -437,   -22,   681,   681,   271,
     166,   166,     0,  -437,  -437,   681,   681,   166,    13,   681,
     681,   166,    14,   777,  -437,  -437,  -437,  -437,   681,   250,
     250,   270,   245,  -437,    25,  -437,  -437,   268,   273,   268,
     268,  -437,  -437,   681,   681,   611,   681,  -437,  -437,   681,
     681,   681,  -437,  -437,  -437,   681,  -437,  -437,  -437,   681,
     681,   681,   681,   681,   681,   681,   287,   248,    86,   248,
     253,   266,   -41,   274,   277,  -437,   289,   278,  -437,  -437,
    -437,  -437,   681,   681,   279,  -437,   280,  -437,  -437,   248,
    -437,  -437,   681,   254,   275,   282,   276,  -437,   166,   681,
    -437,   284,   285,  -437,   286,   288,   166,   -14,   291,   681,
     293,   248,    27,   296,   294,   299,   681,   301,   248,    32,
     303,   302,   304,   681,   306,   248,    44,  -437,   383,  -437,
    -437,   166,  -437,   309,   344,   344,  -437,   344,   889,   889,
      58,   310,  -437,   126,   126,   126,   900,   900,    76,    76,
     250,   250,   250,   116,   152,    86,   312,   -71,  -437,   396,
     313,  -437,  -437,   305,  -437,  -437,  -437,  -437,  -437,   863,
     889,  -437,  -437,   314,   315,  -437,  -437,  -437,  -437,   317,
     661,   405,  -437,  -437,  -437,  -437,  -437,   141,  -437,     0,
     318,  -437,  -437,   681,   319,  -437,  -437,    13,   321,  -437,
    -437,   681,   323,  -437,  -437,    14,   324,  -437,  -437,   681,
     326,   248,  -437,   328,   681,   336,   350,  -437,   350,   166,
      14,   681,   360,  -437,  -437,  -437,  -437,    57,  -437,    91,
     -17,   333,  -437,   332,   334,   681,  -437,  -437,  -437,  -437,
    -437,    23,  -437,   248,   216,   335,   248,  -437,  -437,   339,
    -437,   248,  -437,  -437,   341,  -437,   248,  -437,  -437,   342,
    -437,  -437,   370,   337,  -437,   681,  -437,  -437,   345,   248,
     349,   149,    24,    39,   363,    78,    57,    57,   160,  -437,
    -437,  -437,  -437,  -437,   174,  -437,  -437,  -437,   352,   889,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
     166,  -437,   889,    14,  -437,   166,   371,   430,  -437,  -437,
    -437,   174,   357,  -437,   362,   362,  -437,   -56,   216,  -437,
    -437,   361,  -437,   248,  -437,  -437,  -437,   433,   248,   365,
     364,   366,  -437,  -437,  -437,  -437,   386,  -437,  -437,  -437,
     382,  -437,   166,   368,   427,   456,   389,   446,   388,  -437,
    -437,   166,  -437,   406,   410,   491,  -437,   395,   417,  -437,
    -437,   398,  -437,   681,   399,  -437
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,   467,  -437,  -437,   -57,  -437,   -20,  -437,  -437,
    -437,   -24,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,    -1,  -437,  -201,    65,  -437,  -437,  -437,  -437,  -437,
    -437,   242,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -436,  -437,  -437,  -437,  -437,    99,  -437,  -437,    94,
    -437,  -437,  -437,   414,   441,   -42,  -437,   -49,   207,   -48,
    -437,    20,  -129,  -437,   -26,  -426,  -437,   143,  -437,  -430,
    -437,  -437,  -338,    51,  -437,  -437
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -206
static const yytype_int16 yytable[] =
{
      86,   485,    89,    90,    53,    94,   174,   185,    64,   496,
      79,   272,    56,   173,    68,   186,   197,   349,   519,    79,
     189,    37,    57,    42,   509,   274,    42,   403,   275,   276,
     356,   363,   464,   465,    86,   180,   216,   515,   182,    86,
     251,   213,   530,   257,   433,   466,   531,   585,   226,   441,
     467,    42,    39,   532,  -205,    40,   533,  -205,  -205,    58,
      86,   449,    86,   350,   520,   586,    38,   237,   261,   239,
     277,   278,   279,   249,   558,    86,   357,   364,    86,   404,
     235,    86,   252,   264,    88,   258,   564,   565,   262,   521,
     434,    80,   187,   198,   427,   442,   522,   523,   570,  -205,
    -205,  -205,   333,   334,   335,   336,   176,   450,   428,   280,
     281,   282,   341,   342,   397,   398,   217,   578,   344,   345,
     559,    42,    41,   307,    43,   309,   353,   354,  -205,    42,
     360,   361,  -205,    74,    75,   570,    45,   313,  -205,  -205,
    -205,   283,   284,    35,  -205,   560,   371,  -205,    36,    49,
     228,    73,   522,   523,  -205,    74,    75,   286,  -205,   310,
     464,   465,    42,   240,    66,   243,   328,    78,   244,   245,
      86,    74,    75,   285,   457,   561,   458,   213,   467,   459,
     567,    42,   263,    42,    65,   215,    46,   337,   269,   270,
     271,   301,   302,   516,   517,   303,   289,   518,   343,   329,
     563,   347,    86,   414,    69,   330,   516,   517,    86,   348,
     518,    67,    86,   231,   290,   355,    44,   232,    47,   362,
     430,   292,   293,   294,   233,   372,    70,   438,   234,   299,
     300,   301,   302,    71,   446,   303,   296,   297,   298,   299,
     300,   301,   302,   133,   484,   303,   241,   292,   293,   294,
     242,   247,   255,   259,    72,   248,   256,   260,    76,   396,
      77,   400,   296,   297,   298,   299,   300,   301,   302,   556,
      42,   303,   557,   522,   523,   286,   133,   568,   376,   377,
      79,   413,   287,   288,    91,    95,   177,   178,   179,    86,
     181,   183,   188,   190,   191,   192,   419,   426,   212,   214,
     220,   221,   222,   432,   489,   223,   229,   236,   238,   224,
     440,   253,   494,   227,   289,   265,   421,   448,   250,   368,
     499,   267,    86,   306,   308,   503,   268,   311,   273,   453,
     314,    86,   290,   315,   378,   379,   176,   383,   460,   316,
     384,   385,   386,   451,   318,   319,   387,   291,   322,   320,
     388,   389,   390,   391,   392,   393,   394,   321,   324,   325,
     339,   326,   327,   331,   346,   292,   293,   294,   332,   303,
     338,   370,   373,   409,   410,   401,   415,   375,   473,   295,
     296,   297,   298,   299,   300,   301,   302,   402,   286,   303,
     420,   395,   369,   407,   405,   287,   288,   406,   408,   411,
     412,   416,   418,   501,   417,   455,   422,   488,   424,   474,
      86,   423,   512,   429,   425,   493,   431,   508,   435,   511,
     436,   437,   470,   498,   439,   443,   445,   289,   444,   447,
     454,   461,   463,   472,   476,   536,   482,   477,   538,   478,
     504,   487,   490,   542,   492,   290,   495,   497,   546,   500,
     502,   505,   513,   525,   526,   527,   550,   537,   535,   551,
     291,   554,   540,   539,   544,   548,   541,   553,   543,   555,
     217,   545,   572,   547,   581,   582,   549,   584,   292,   293,
     294,   518,   590,   588,   614,   592,   597,   594,   595,   596,
     599,   600,   295,   296,   297,   298,   299,   300,   301,   302,
     601,    86,   303,   571,   603,   452,    86,   604,   577,   607,
     573,   605,   368,   580,   608,   609,   574,   610,   611,   613,
     575,   615,   141,   507,   576,   589,   529,   382,   486,   219,
     591,   491,    97,   246,   562,   583,     0,    98,   462,     0,
       0,    99,     0,   598,     0,   100,   101,   102,   -79,   103,
       0,   104,   606,   105,     0,     0,   552,     0,     0,     0,
       0,     0,   -79,   -79,     0,   106,   107,   108,   109,   110,
     111,     0,     0,   112,     0,     0,     0,     0,     0,   113,
       0,     0,     0,   114,   115,   116,   -79,     0,   117,     0,
     118,   119,     0,     0,   -79,     0,     0,   120,     0,     0,
       0,     0,   121,   122,   123,   124,     0,   125,     0,   126,
     127,     0,   128,   129,    12,     0,   130,    98,   131,     0,
       0,     0,     0,     0,     0,   132,     0,   102,     0,   103,
       0,     0,     0,   133,   134,   135,     0,   136,     0,     0,
       0,     0,     0,     0,   137,   138,   107,     0,     0,     0,
       0,     0,   139,   -79,     0,     0,   140,     0,     0,   113,
       0,     0,     0,   114,   115,   116,   286,     0,   117,     0,
     118,     0,     0,   287,   288,     0,     0,   120,     0,     0,
       0,     0,     0,     0,   123,     0,   479,    98,     0,   126,
       0,     0,   128,   129,     0,     0,   380,   102,     0,   103,
       0,     0,     0,     0,     0,   289,     0,   480,     0,     0,
       0,     0,     0,   133,   134,   381,   107,   136,     0,     0,
       0,     0,     0,   290,   137,   138,     0,     0,     0,   113,
       0,     0,   139,     0,   115,   116,   140,     0,   291,     0,
     118,     0,   286,     0,     0,     0,     0,   120,     0,   287,
     288,     0,     0,   286,   123,     0,   292,   293,   294,   126,
     287,   288,   128,   129,     0,     0,     0,     0,     0,     0,
     295,   296,   297,   298,   299,   300,   301,   302,     0,     0,
     303,   289,   286,   133,   134,   135,     0,   136,     0,   287,
     288,     0,   289,     0,   137,   138,     0,     0,     0,   290,
       0,     0,   139,     0,     0,     0,   140,   367,     0,     0,
     290,     0,     0,     0,   291,     0,     0,     0,     0,     0,
       0,   289,     0,     0,     0,   291,     0,     0,   340,     0,
       0,     0,   292,   293,   294,   323,     0,     0,     0,   290,
       0,     0,     0,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   291,     0,   303,   295,   296,   297,
     298,   299,   300,   301,   302,     0,     0,   303,   286,     0,
       0,     0,   292,   293,   294,   287,   288,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   295,   296,   297,   298,
     299,   300,   301,   302,   286,     0,   303,     0,     0,     0,
       0,   287,   288,     0,     0,   286,     0,   289,     0,     0,
       0,     0,   287,   288,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   290,     0,     0,     0,     0,
       0,     0,     0,   289,     0,     0,     0,     0,     0,     0,
     291,     0,     0,     0,   289,     0,     0,     0,     0,     0,
       0,   290,     0,     0,     0,     0,     0,     0,   292,   293,
     294,   475,   290,     0,     0,     0,   291,     0,     0,     0,
       0,     0,   295,   296,   297,   298,   299,   300,   301,   302,
       0,     0,   303,     0,   292,   293,   294,     0,     0,     0,
       0,     0,     0,     0,     0,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,     2,     3,   303,     4,
     296,   297,   298,   299,   300,   301,   302,   -62,   199,   303,
       0,     0,     0,     0,     0,     0,     0,   -62,     0,     0,
       0,     0,     0,     5,     0,     0,   200,     0,     0,   201,
     202,     0,     0,     0,     0,   203,     0,     6,     0,     7,
       0,     0,     0,   204,     0,     0,     0,     0,   205,     0,
       0,     0,     0,     0,   -62,     0,     0,     0,     0,     0,
       0,     8,   206,     9,     0,    10,    11,   207,     0,     0,
       0,   208,     0,   209,     0,   210,     0,     0,     0,    12,
      13,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   211
};

static const yytype_int16 yycheck[] =
{
      49,   427,    50,    51,    24,    53,    55,    64,    32,   445,
      32,   140,    11,    55,    38,    27,    27,    17,    35,    32,
      68,   104,    21,   104,   460,     4,   104,    68,     7,     8,
      17,    17,   103,   104,    83,    59,    85,   467,    62,    88,
     121,    83,    19,   121,    17,   116,    23,   103,    96,    17,
     121,   104,   104,    30,     4,   107,    33,     7,     8,    58,
     109,    17,   111,    63,    81,   121,   120,   109,   121,   111,
      49,    50,    51,   122,    35,   124,    63,    63,   127,   120,
     104,   130,   124,   132,   106,   127,   516,   517,   130,   106,
      63,   104,   104,   104,   108,    63,   113,   114,   524,    49,
      50,    51,   231,   232,   233,   234,    55,    63,   122,    88,
      89,    90,   241,   242,    28,    29,   107,   553,   247,   248,
      81,   104,   104,   180,   107,   182,   255,   256,   104,   104,
     259,   260,   108,   124,   125,   561,    55,   185,    88,    89,
      90,    91,    92,    55,   120,   106,   121,   123,    60,   106,
      99,   120,   113,   114,   104,   124,   125,     5,   108,   183,
     103,   104,   104,   112,    44,   114,   215,   120,   117,   118,
     219,   124,   125,   123,   375,   513,   377,   219,   121,   121,
     518,   104,   131,   104,   120,   108,   107,   235,   137,   138,
     139,   115,   116,   115,   116,   119,    44,   119,   246,   223,
     122,   250,   251,   332,    40,   225,   115,   116,   257,   251,
     119,    44,   261,   121,    62,   257,     9,   125,    11,   261,
     349,    95,    96,    97,   121,   274,   120,   356,   125,   113,
     114,   115,   116,    38,   363,   119,   110,   111,   112,   113,
     114,   115,   116,   102,   103,   119,   121,    95,    96,    97,
     125,   121,   121,   121,   120,   125,   125,   125,    44,   307,
     120,   309,   110,   111,   112,   113,   114,   115,   116,   120,
     104,   119,   123,   113,   114,     5,   102,   103,   279,   280,
      32,   329,    12,    13,    56,    55,   104,   104,   104,   338,
      68,   121,   104,   107,   104,   104,   338,   346,   111,   105,
     120,   120,   104,   351,   433,   120,   104,   104,   104,   120,
     358,   105,   441,   120,    44,   105,   340,   365,   121,   268,
     449,   105,   371,    68,   104,   454,   121,    86,   120,   371,
      58,   380,    62,   120,   283,   284,   285,   286,   380,    56,
     289,   290,   291,   367,   120,   120,   295,    77,   108,   120,
     299,   300,   301,   302,   303,   304,   305,   126,    58,   120,
      44,   105,   105,   120,    93,    95,    96,    97,   121,   119,
     121,   126,   104,   322,   323,   122,   122,   104,   402,   109,
     110,   111,   112,   113,   114,   115,   116,   121,     5,   119,
     339,   104,   122,   104,   120,    12,    13,   120,   120,   120,
     120,   126,   126,   451,   122,    61,   122,   431,   122,   104,
     459,   126,   461,   122,   126,   439,   123,   459,   122,   461,
     126,   122,    26,   447,   123,   122,   122,    44,   126,   123,
     121,   121,   120,   120,   120,   483,    31,   122,   486,   122,
     104,   123,   123,   491,   123,    62,   123,   123,   496,   123,
     122,   101,    92,   120,   122,   121,    86,   122,   482,   122,
      77,   509,   123,   487,   123,   123,   490,   122,   492,   120,
     107,   495,   120,   497,   103,    45,   500,   120,    95,    96,
      97,   119,    49,   122,   613,   120,   104,   123,   122,   103,
     122,    64,   109,   110,   111,   112,   113,   114,   115,   116,
      44,   550,   119,   527,   115,   122,   555,    61,   550,   103,
     534,   123,   461,   555,   104,    24,   540,   122,   101,   121,
     544,   122,    55,   458,   548,   573,   475,   285,   429,    88,
     578,   437,     1,   119,   514,   561,    -1,     6,   395,    -1,
      -1,    10,    -1,   592,    -1,    14,    15,    16,    17,    18,
      -1,    20,   601,    22,    -1,    -1,   505,    -1,    -1,    -1,
      -1,    -1,    31,    32,    -1,    34,    35,    36,    37,    38,
      39,    -1,    -1,    42,    -1,    -1,    -1,    -1,    -1,    48,
      -1,    -1,    -1,    52,    53,    54,    55,    -1,    57,    -1,
      59,    60,    -1,    -1,    63,    -1,    -1,    66,    -1,    -1,
      -1,    -1,    71,    72,    73,    74,    -1,    76,    -1,    78,
      79,    -1,    81,    82,    83,    -1,    85,     6,    87,    -1,
      -1,    -1,    -1,    -1,    -1,    94,    -1,    16,    -1,    18,
      -1,    -1,    -1,   102,   103,   104,    -1,   106,    -1,    -1,
      -1,    -1,    -1,    -1,   113,   114,    35,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,    48,
      -1,    -1,    -1,    52,    53,    54,     5,    -1,    57,    -1,
      59,    -1,    -1,    12,    13,    -1,    -1,    66,    -1,    -1,
      -1,    -1,    -1,    -1,    73,    -1,    25,     6,    -1,    78,
      -1,    -1,    81,    82,    -1,    -1,    85,    16,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    44,    -1,    46,    -1,    -1,
      -1,    -1,    -1,   102,   103,   104,    35,   106,    -1,    -1,
      -1,    -1,    -1,    62,   113,   114,    -1,    -1,    -1,    48,
      -1,    -1,   121,    -1,    53,    54,   125,    -1,    77,    -1,
      59,    -1,     5,    -1,    -1,    -1,    -1,    66,    -1,    12,
      13,    -1,    -1,     5,    73,    -1,    95,    96,    97,    78,
      12,    13,    81,    82,    -1,    -1,    -1,    -1,    -1,    -1,
     109,   110,   111,   112,   113,   114,   115,   116,    -1,    -1,
     119,    44,     5,   102,   103,   104,    -1,   106,    -1,    12,
      13,    -1,    44,    -1,   113,   114,    -1,    -1,    -1,    62,
      -1,    -1,   121,    -1,    -1,    -1,   125,    30,    -1,    -1,
      62,    -1,    -1,    -1,    77,    -1,    -1,    -1,    -1,    -1,
      -1,    44,    -1,    -1,    -1,    77,    -1,    -1,    80,    -1,
      -1,    -1,    95,    96,    97,    98,    -1,    -1,    -1,    62,
      -1,    -1,    -1,    95,    96,    97,   109,   110,   111,   112,
     113,   114,   115,   116,    77,    -1,   119,   109,   110,   111,
     112,   113,   114,   115,   116,    -1,    -1,   119,     5,    -1,
      -1,    -1,    95,    96,    97,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,   112,
     113,   114,   115,   116,     5,    -1,   119,    -1,    -1,    -1,
      -1,    12,    13,    -1,    -1,     5,    -1,    44,    -1,    -1,
      -1,    -1,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      77,    -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    95,    96,
      97,    98,    62,    -1,    -1,    -1,    77,    -1,    -1,    -1,
      -1,    -1,   109,   110,   111,   112,   113,   114,   115,   116,
      -1,    -1,   119,    -1,    95,    96,    97,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    95,    96,    97,   109,   110,
     111,   112,   113,   114,   115,   116,     0,     1,   119,     3,
     110,   111,   112,   113,   114,   115,   116,    11,    20,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    -1,    -1,
      -1,    -1,    -1,    27,    -1,    -1,    38,    -1,    -1,    41,
      42,    -1,    -1,    -1,    -1,    47,    -1,    41,    -1,    43,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    60,    -1,
      -1,    -1,    -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    74,    67,    -1,    69,    70,    79,    -1,    -1,
      -1,    83,    -1,    85,    -1,    87,    -1,    -1,    -1,    83,
      84,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   104
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   128,     0,     1,     3,    27,    41,    43,    65,    67,
      69,    70,    83,    84,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   144,   147,   153,   154,
     155,   156,   157,   158,   159,    55,    60,   104,   120,   104,
     107,   104,   104,   107,   225,    55,   107,   225,   160,   106,
     220,   164,   165,   164,   168,   169,    11,    21,    58,   141,
     142,   145,   148,   149,   168,   120,    44,    44,   168,    40,
     120,    38,   120,   120,   124,   125,    44,   120,   120,    32,
     104,   161,   226,   221,   222,   223,   224,   225,   106,   226,
     226,    56,   166,   167,   226,    55,   162,     1,     6,    10,
      14,    15,    16,    18,    20,    22,    34,    35,    36,    37,
      38,    39,    42,    48,    52,    53,    54,    57,    59,    60,
      66,    71,    72,    73,    74,    76,    78,    79,    81,    82,
      85,    87,    94,   102,   103,   104,   106,   113,   114,   121,
     125,   159,   170,   171,   172,   173,   175,   176,   182,   183,
     184,   185,   186,   187,   188,   189,   193,   195,   198,   199,
     200,   201,   202,   203,   204,   205,   207,   210,   211,   212,
     215,   218,   219,   222,   224,   232,   240,   104,   104,   104,
     168,    68,   168,   121,   150,   162,    27,   104,   104,   226,
     107,   104,   104,   224,   229,   230,   240,    27,   104,    20,
      38,    41,    42,    47,    55,    60,    74,    79,    83,    85,
      87,   104,   111,   222,   105,   108,   224,   107,   228,   221,
     120,   120,   104,   120,   120,   163,   226,   120,   240,   104,
     177,   121,   125,   121,   125,   168,   104,   222,   104,   222,
     240,   121,   125,   240,   240,   240,   220,   121,   125,   224,
     121,   121,   222,   105,   227,   121,   125,   121,   222,   121,
     125,   121,   222,   240,   224,   105,   233,   105,   121,   240,
     240,   240,   229,   120,     4,     7,     8,    49,    50,    51,
      88,    89,    90,    91,    92,   123,     5,    12,    13,    44,
      62,    77,    95,    96,    97,   109,   110,   111,   112,   113,
     114,   115,   116,   119,   241,   242,    68,   162,   104,   162,
     168,    86,   151,   226,    58,   120,    56,   179,   120,   120,
     120,   126,   108,    98,    58,   120,   105,   105,   224,   168,
     164,   120,   121,   229,   229,   229,   229,   226,   121,    44,
      80,   229,   229,   226,   229,   229,    93,   224,   222,    17,
      63,   213,   214,   229,   229,   222,    17,    63,   216,   217,
     229,   229,   222,    17,    63,   208,   209,    30,   240,   122,
     126,   121,   224,   104,   178,   104,   178,   178,   240,   240,
      85,   104,   188,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   104,   226,    28,    29,   234,
     226,   122,   121,    68,   120,   120,   120,   104,   120,   240,
     240,   120,   120,   226,   229,   122,   126,   122,   126,   222,
     240,   168,   122,   126,   122,   126,   224,   108,   122,   122,
     229,   123,   226,    17,    63,   122,   126,   122,   229,   123,
     226,    17,    63,   122,   126,   122,   229,   123,   226,    17,
      63,   168,   122,   222,   121,    61,   180,   180,   180,   121,
     222,   121,   234,   120,   103,   104,   116,   121,   235,   236,
      26,   143,   120,   168,   104,    98,   120,   122,   122,    25,
      46,   196,    31,   206,   103,   232,   213,   123,   168,   229,
     123,   216,   123,   168,   229,   123,   208,   123,   168,   229,
     123,   226,   122,   229,   104,   101,   181,   181,   222,   208,
     190,   222,   224,    92,   146,   236,   115,   116,   119,    35,
      81,   106,   113,   114,   239,   120,   122,   121,   152,   240,
      19,    23,    30,    33,   197,   168,   226,   122,   226,   168,
     123,   168,   226,   168,   123,   168,   226,   168,   123,   168,
      86,   122,   240,   122,   226,   120,   120,   123,    35,    81,
     106,   239,   228,   122,   236,   236,   237,   239,   103,   231,
     232,   168,   120,   168,   168,   168,   168,   222,   208,   191,
     222,   103,    45,   231,   120,   103,   121,   238,   122,   226,
      49,   226,   120,   192,   123,   122,   103,   104,   224,   122,
      64,    44,   194,   115,    61,   123,   224,   103,   104,    24,
     122,   101,   174,   121,   229,   122
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
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
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
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
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



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

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
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

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
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
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
#line 443 "base/generic/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in definition.");
	}
    break;

  case 18:
#line 450 "base/generic/compiler/ascParse.y"
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
#line 495 "base/generic/compiler/ascParse.y"
    {
	  Asc_ScannerPushBuffer((yyvsp[(2) - (3)].dquote_ptr));
	}
    break;

  case 20:
#line 499 "base/generic/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 21:
#line 504 "base/generic/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 22:
#line 512 "base/generic/compiler/ascParse.y"
    {
          Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[(2) - (3)].dquote_ptr));
        }
    break;

  case 23:
#line 516 "base/generic/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (3)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 24:
#line 521 "base/generic/compiler/ascParse.y"
    {
	  DestroyName((yyvsp[(2) - (2)].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 25:
#line 529 "base/generic/compiler/ascParse.y"
    {
	  if(package_load((yyvsp[(4) - (5)].dquote_ptr),SCP((yyvsp[(2) - (5)].id_ptr)))){
		ErrMsg_Generic("IMPORT failed");
      }
	}
    break;

  case 26:
#line 535 "base/generic/compiler/ascParse.y"
    {
	  if(package_load(SCP((yyvsp[(2) - (3)].dquote_ptr)),NULL)){
        ErrMsg_Generic("IMPORT failed");
      }
	}
    break;

  case 27:
#line 544 "base/generic/compiler/ascParse.y"
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
#line 575 "base/generic/compiler/ascParse.y"
    {
	  g_type_name = (yyvsp[(4) - (6)].id_ptr);
	  g_proc_name = (yyvsp[(5) - (6)].id_ptr);
	}
    break;

  case 29:
#line 583 "base/generic/compiler/ascParse.y"
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
#line 606 "base/generic/compiler/ascParse.y"
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
#line 619 "base/generic/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 32:
#line 627 "base/generic/compiler/ascParse.y"
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
#line 648 "base/generic/compiler/ascParse.y"
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
#line 660 "base/generic/compiler/ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 35:
#line 668 "base/generic/compiler/ascParse.y"
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
#line 720 "base/generic/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[(3) - (6)].id_ptr);
	  g_atom_dim_ptr = (yyvsp[(4) - (6)].dimp);
	  g_default_double = (yyvsp[(5) - (6)].real_value);
	  g_header_linenum = LineNum();
	}
    break;

  case 37:
#line 731 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 38:
#line 739 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 39:
#line 745 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 40:
#line 750 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
    break;

  case 41:
#line 757 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
    break;

  case 42:
#line 764 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 0;
	}
    break;

  case 43:
#line 774 "base/generic/compiler/ascParse.y"
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
#line 814 "base/generic/compiler/ascParse.y"
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
#line 847 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 46:
#line 853 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(2) - (3)].int_value) ? -(yyvsp[(3) - (3)].real_value) : (yyvsp[(3) - (3)].real_value);
	  g_defaulted = 1;
	}
    break;

  case 47:
#line 858 "base/generic/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 48:
#line 865 "base/generic/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 49:
#line 872 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sym_ptr) = (yyvsp[(2) - (2)].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
    break;

  case 50:
#line 882 "base/generic/compiler/ascParse.y"
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
#line 924 "base/generic/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[(2) - (4)].slptr);
	  g_parameter_wheres = (yyvsp[(3) - (4)].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
    break;

  case 52:
#line 933 "base/generic/compiler/ascParse.y"
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
#line 945 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 54:
#line 953 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 55:
#line 957 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 56:
#line 964 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 57:
#line 968 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(3) - (4)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 58:
#line 975 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 59:
#line 979 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (3)].slptr); /* this could be much more sophisticated */
	}
    break;

  case 60:
#line 986 "base/generic/compiler/ascParse.y"
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
#line 1018 "base/generic/compiler/ascParse.y"
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
#line 1031 "base/generic/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 63:
#line 1035 "base/generic/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 64:
#line 1042 "base/generic/compiler/ascParse.y"
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
#line 1072 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_type_name = (yyvsp[(2) - (2)].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 66:
#line 1081 "base/generic/compiler/ascParse.y"
    { /* nothing to do. just cruft to fix ; problem */ }
    break;

  case 67:
#line 1086 "base/generic/compiler/ascParse.y"
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
#line 1105 "base/generic/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(100L);
	}
    break;

  case 69:
#line 1109 "base/generic/compiler/ascParse.y"
    {
	  gl_append_ptr((yyvsp[(1) - (2)].listp),(char *)(yyvsp[(2) - (2)].udefptr));
	  (yyval.listp) = (yyvsp[(1) - (2)].listp);
	}
    break;

  case 70:
#line 1117 "base/generic/compiler/ascParse.y"
    {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[(1) - (4)].id_ptr),(yyvsp[(3) - (4)].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
                             LineNum());
	}
    break;

  case 71:
#line 1126 "base/generic/compiler/ascParse.y"
    {
	  (yyval.listp) = NULL;
	}
    break;

  case 72:
#line 1130 "base/generic/compiler/ascParse.y"
    { /* To get rid of this, we will need a global proclist
           * that accumulates procs until a MODEL production is
           * completed. If any other sort of production is started,
           * and proclist is not NULL, it should be discarded.
           */
	}
    break;

  case 73:
#line 1137 "base/generic/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(3) - (3)].listp);
	}
    break;

  case 74:
#line 1144 "base/generic/compiler/ascParse.y"
    {
	  (yyval.listp) = (yyvsp[(1) - (1)].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
    break;

  case 75:
#line 1151 "base/generic/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 76:
#line 1155 "base/generic/compiler/ascParse.y"
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
#line 1180 "base/generic/compiler/ascParse.y"
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
#line 1194 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	  g_proc_name = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 79:
#line 1203 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = CreateStatementList((yyvsp[(1) - (1)].listp));
	}
    break;

  case 80:
#line 1210 "base/generic/compiler/ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	}
    break;

  case 81:
#line 1214 "base/generic/compiler/ascParse.y"
    {
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ((yyvsp[(2) - (3)].statptr) != NULL) {
	    gl_append_ptr((yyvsp[(1) - (3)].listp),(char *)(yyvsp[(2) - (3)].statptr));
	  }
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 82:
#line 1222 "base/generic/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[(1) - (3)].listp);
	}
    break;

  case 113:
#line 1263 "base/generic/compiler/ascParse.y"
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

  case 114:
#line 1304 "base/generic/compiler/ascParse.y"
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

  case 115:
#line 1333 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateALIASES((yyvsp[(1) - (3)].lptr),(yyvsp[(3) - (3)].nptr));
	}
    break;

  case 116:
#line 1338 "base/generic/compiler/ascParse.y"
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

  case 117:
#line 1387 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 118:
#line 1391 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 119:
#line 1398 "base/generic/compiler/ascParse.y"
    {
	  if (FindType((yyvsp[(3) - (4)].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[(1) - (4)].lptr),(yyvsp[(3) - (4)].id_ptr),(yyvsp[(4) - (4)].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[(3) - (4)].id_ptr)));
	  }
	}
    break;

  case 120:
#line 1410 "base/generic/compiler/ascParse.y"
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

  case 121:
#line 1437 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_callargs = NULL;
	}
    break;

  case 122:
#line 1442 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_callargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 123:
#line 1450 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (1)].id_ptr);
	  g_typeargs = NULL;
	}
    break;

  case 124:
#line 1455 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(1) - (4)].id_ptr);
	  g_typeargs = (yyvsp[(3) - (4)].sptr);
	}
    break;

  case 125:
#line 1463 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 126:
#line 1467 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 127:
#line 1474 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	}
    break;

  case 128:
#line 1478 "base/generic/compiler/ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[(2) - (2)].id_ptr);
	}
    break;

  case 129:
#line 1485 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = NULL;
	}
    break;

  case 130:
#line 1489 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 131:
#line 1496 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateAA((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 132:
#line 1503 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateATS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 133:
#line 1510 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 134:
#line 1517 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateWNBTS((yyvsp[(1) - (2)].lptr));
	}
    break;

  case 135:
#line 1524 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 136:
#line 1528 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[(1) - (3)].nptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 137:
#line 1535 "base/generic/compiler/ascParse.y"
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

  case 138:
#line 1548 "base/generic/compiler/ascParse.y"
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

  case 139:
#line 1565 "base/generic/compiler/ascParse.y"
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

  case 140:
#line 1575 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 141:
#line 1583 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[(2) - (2)].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 142:
#line 1594 "base/generic/compiler/ascParse.y"
    {
	  /*
	   * This is the blackbox declarative external relation.
	   */
	  struct VariableList *vl;
	  vl = JoinVariableLists((yyvsp[(5) - (9)].lptr),(yyvsp[(7) - (9)].lptr));
	  /* $$ = CreateEXTERN(2,$1,SCP($3),vl,$8,NULL); */
	  (yyval.statptr) = CreateEXTERNBlackBox((yyvsp[(1) - (9)].nptr),SCP((yyvsp[(3) - (9)].id_ptr)),vl,(yyvsp[(8) - (9)].nptr));
	}
    break;

  case 143:
#line 1607 "base/generic/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 144:
#line 1614 "base/generic/compiler/ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[(1) - (3)].lptr);
	}
    break;

  case 145:
#line 1621 "base/generic/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 146:
#line 1625 "base/generic/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (4)].nptr);
	}
    break;

  case 147:
#line 1632 "base/generic/compiler/ascParse.y"
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

  case 148:
#line 1652 "base/generic/compiler/ascParse.y"
    {
	  (yyval.nptr) = NULL;
	}
    break;

  case 149:
#line 1656 "base/generic/compiler/ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[(2) - (2)].nptr);
	}
    break;

  case 150:
#line 1664 "base/generic/compiler/ascParse.y"
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

  case 151:
#line 1684 "base/generic/compiler/ascParse.y"
    {
	  (yyval.order) = f_random;
	}
    break;

  case 152:
#line 1688 "base/generic/compiler/ascParse.y"
    {
	  (yyval.order) = f_increasing;
	}
    break;

  case 153:
#line 1692 "base/generic/compiler/ascParse.y"
    {
	  (yyval.order) = f_decreasing;
	}
    break;

  case 154:
#line 1699 "base/generic/compiler/ascParse.y"
    {
          (yyval.fkind) = fk_create; /* declarative FOR */
	}
    break;

  case 155:
#line 1703 "base/generic/compiler/ascParse.y"
    {
          (yyval.fkind) = fk_expect; /* parameter FOR */
	}
    break;

  case 156:
#line 1707 "base/generic/compiler/ascParse.y"
    {
          (yyval.fkind) = fk_check; /* WHERE FOR */
	}
    break;

  case 157:
#line 1711 "base/generic/compiler/ascParse.y"
    {
          (yyval.fkind) = fk_do; /* method FOR */
	}
    break;

  case 158:
#line 1718 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(2) - (2)].nptr),NULL);
	}
    break;

  case 159:
#line 1722 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[(4) - (4)].nptr),(yyvsp[(2) - (4)].nptr));	  /* type :: name */
	}
    break;

  case 160:
#line 1729 "base/generic/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateFIX((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 161:
#line 1736 "base/generic/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateFREE((yyvsp[(2) - (2)].lptr));
	}
    break;

  case 162:
#line 1743 "base/generic/compiler/ascParse.y"
    {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[(2) - (5)].id_ptr)),(yyvsp[(4) - (5)].lptr));
	}
    break;

  case 163:
#line 1754 "base/generic/compiler/ascParse.y"
    {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[(2) - (2)].id_ptr),g_callargs);
          g_callargs = NULL;
	}
    break;

  case 164:
#line 1765 "base/generic/compiler/ascParse.y"
    {
		(yyval.statptr) = CreateASSERT((yyvsp[(2) - (2)].eptr));
	}
    break;

  case 165:
#line 1771 "base/generic/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[(2) - (6)].eptr),(yyvsp[(4) - (6)].slptr),(yyvsp[(5) - (6)].slptr));
	}
    break;

  case 166:
#line 1781 "base/generic/compiler/ascParse.y"
    {
	  if( (yyvsp[(5) - (5)].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[(5) - (5)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[(2) - (5)].eptr),(yyvsp[(4) - (5)].slptr));
	}
    break;

  case 167:
#line 1790 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = NULL;
	}
    break;

  case 168:
#line 1794 "base/generic/compiler/ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[(2) - (2)].slptr);
	}
    break;

  case 169:
#line 1801 "base/generic/compiler/ascParse.y"
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

  case 170:
#line 1812 "base/generic/compiler/ascParse.y"
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

  case 171:
#line 1824 "base/generic/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].wptr));
	}
    break;

  case 172:
#line 1831 "base/generic/compiler/ascParse.y"
    {
	  if( (yyvsp[(8) - (8)].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[(8) - (8)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[(1) - (8)].nptr),(yyvsp[(5) - (8)].lptr),(yyvsp[(7) - (8)].wptr));
	}
    break;

  case 173:
#line 1841 "base/generic/compiler/ascParse.y"
    {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[(1) - (1)].wptr));
	}
    break;

  case 174:
#line 1848 "base/generic/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 175:
#line 1852 "base/generic/compiler/ascParse.y"
    {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 176:
#line 1856 "base/generic/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].wptr));
	}
    break;

  case 177:
#line 1860 "base/generic/compiler/ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].wptr));
	}
    break;

  case 178:
#line 1867 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
    break;

  case 179:
#line 1871 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
    break;

  case 180:
#line 1875 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
    break;

  case 181:
#line 1879 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
    break;

  case 182:
#line 1883 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[(2) - (2)].braced_ptr));
	}
    break;

  case 183:
#line 1890 "base/generic/compiler/ascParse.y"
    {
	  (yyval.statptr) = CreateFNAME((yyvsp[(2) - (2)].nptr));
	}
    break;

  case 184:
#line 1897 "base/generic/compiler/ascParse.y"
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

  case 185:
#line 1908 "base/generic/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].septr));
	}
    break;

  case 186:
#line 1918 "base/generic/compiler/ascParse.y"
    {
	  (yyval.septr) = ReverseSelectCases((yyvsp[(1) - (1)].septr));
	}
    break;

  case 187:
#line 1925 "base/generic/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 188:
#line 1929 "base/generic/compiler/ascParse.y"
    {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 189:
#line 1933 "base/generic/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].septr));
	}
    break;

  case 190:
#line 1937 "base/generic/compiler/ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].septr));
	}
    break;

  case 191:
#line 1944 "base/generic/compiler/ascParse.y"
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

  case 192:
#line 1955 "base/generic/compiler/ascParse.y"
    {
	  if( (yyvsp[(6) - (6)].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[(6) - (6)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[(3) - (6)].lptr),(yyvsp[(5) - (6)].swptr));
	}
    break;

  case 193:
#line 1965 "base/generic/compiler/ascParse.y"
    {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[(1) - (1)].swptr));
	}
    break;

  case 194:
#line 1972 "base/generic/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch((yyvsp[(2) - (4)].sptr),(yyvsp[(4) - (4)].slptr));
	}
    break;

  case 195:
#line 1976 "base/generic/compiler/ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[(3) - (3)].slptr));
	}
    break;

  case 196:
#line 1980 "base/generic/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[(3) - (5)].sptr),(yyvsp[(5) - (5)].slptr)),(yyvsp[(1) - (5)].swptr));
	}
    break;

  case 197:
#line 1984 "base/generic/compiler/ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[(4) - (4)].slptr)),(yyvsp[(1) - (4)].swptr));
	}
    break;

  case 198:
#line 1991 "base/generic/compiler/ascParse.y"
    {
	  if( (yyvsp[(3) - (3)].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[(3) - (3)].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[(2) - (3)].slptr));
	}
    break;

  case 199:
#line 2001 "base/generic/compiler/ascParse.y"
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

  case 200:
#line 2038 "base/generic/compiler/ascParse.y"
    {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[(2) - (2)].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[(1) - (2)].sym_ptr);
	}
    break;

  case 201:
#line 2047 "base/generic/compiler/ascParse.y"
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

  case 202:
#line 2062 "base/generic/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(2) - (2)].braced_ptr),NULL),
                             (void *)(yyvsp[(1) - (2)].lptr), LineNum());
	}
    break;

  case 203:
#line 2067 "base/generic/compiler/ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[(3) - (3)].braced_ptr),NULL),
	                     (void *)(yyvsp[(2) - (3)].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[(1) - (3)].notesptr));
	}
    break;

  case 204:
#line 2076 "base/generic/compiler/ascParse.y"
    {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[(1) - (1)].lptr));
	}
    break;

  case 205:
#line 2087 "base/generic/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 206:
#line 2091 "base/generic/compiler/ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[(3) - (3)].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[(1) - (3)].lptr));
	}
    break;

  case 207:
#line 2096 "base/generic/compiler/ascParse.y"
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

  case 208:
#line 2109 "base/generic/compiler/ascParse.y"
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

  case 209:
#line 2129 "base/generic/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(1) - (1)].id_ptr));
	}
    break;

  case 210:
#line 2133 "base/generic/compiler/ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[(3) - (3)].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[(1) - (3)].nptr));
	}
    break;

  case 211:
#line 2138 "base/generic/compiler/ascParse.y"
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

  case 212:
#line 2153 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = CONDITIONAL_TOK;
        }
    break;

  case 213:
#line 2158 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = FOR_TOK;
        }
    break;

  case 214:
#line 2163 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = IF_TOK;
        }
    break;

  case 215:
#line 2168 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = INTERACTIVE_TOK;
        }
    break;

  case 216:
#line 2173 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = METHODS_TOK;
        }
    break;

  case 217:
#line 2178 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = NOTES_TOK;
        }
    break;

  case 218:
#line 2183 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = SELECT_TOK;
        }
    break;

  case 219:
#line 2188 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = SWITCH_TOK;
        }
    break;

  case 220:
#line 2193 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = UNITS_TOK;
        }
    break;

  case 221:
#line 2198 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = GLOBAL_TOK;
        }
    break;

  case 222:
#line 2203 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = WHEN_TOK;
        }
    break;

  case 223:
#line 2208 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = WHILE_TOK;
        }
    break;

  case 224:
#line 2213 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = (yyvsp[(2) - (2)].id_ptr);
          (yyval.int_value) = IDENTIFIER_TOK;
        }
    break;

  case 225:
#line 2218 "base/generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = END_TOK;
        }
    break;

  case 226:
#line 2226 "base/generic/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = NULL;
	}
    break;

  case 227:
#line 2230 "base/generic/compiler/ascParse.y"
    {
	  (yyval.braced_ptr) = (yyvsp[(1) - (1)].braced_ptr);
	}
    break;

  case 228:
#line 2237 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = NULL;
	}
    break;

  case 229:
#line 2241 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dquote_ptr) = (yyvsp[(1) - (1)].dquote_ptr);
	}
    break;

  case 230:
#line 2248 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = ReverseSetList((yyvsp[(1) - (1)].sptr));
	}
    break;

  case 231:
#line 2252 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = NULL;
	}
    break;

  case 232:
#line 2259 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(1) - (1)].eptr));
	}
    break;

  case 233:
#line 2263 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 234:
#line 2267 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[(3) - (3)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (3)].sptr));
	}
    break;

  case 235:
#line 2272 "base/generic/compiler/ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[(3) - (5)].eptr),(yyvsp[(5) - (5)].eptr));
	  LinkSets((yyval.sptr),(yyvsp[(1) - (5)].sptr));
	}
    break;

  case 236:
#line 2280 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
    break;

  case 237:
#line 2286 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (1)].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
    break;

  case 238:
#line 2295 "base/generic/compiler/ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[(1) - (2)].real_value)*(yyvsp[(2) - (2)].real_value);
	}
    break;

  case 239:
#line 2299 "base/generic/compiler/ascParse.y"
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

  case 240:
#line 2320 "base/generic/compiler/ascParse.y"
    {
	  g_dim_ptr = Dimensionless();
	  (yyval.real_value) = 1.0;
	}
    break;

  case 241:
#line 2325 "base/generic/compiler/ascParse.y"
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

  case 242:
#line 2346 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimp) = (yyvsp[(2) - (2)].dimp);
	}
    break;

  case 243:
#line 2350 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimp) = Dimensionless();
	}
    break;

  case 244:
#line 2354 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 245:
#line 2361 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	}
    break;

  case 246:
#line 2365 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[(1) - (1)].dimen)));
	}
    break;

  case 247:
#line 2372 "base/generic/compiler/ascParse.y"
    {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[(1) - (1)].id_ptr)));
	}
    break;

  case 248:
#line 2376 "base/generic/compiler/ascParse.y"
    {
	  ClearDimensions(&((yyval.dimen)));
	}
    break;

  case 249:
#line 2380 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimen) = SubDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 250:
#line 2384 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimen) = AddDimensions(&((yyvsp[(1) - (3)].dimen)),&((yyvsp[(3) - (3)].dimen)));
	}
    break;

  case 251:
#line 2388 "base/generic/compiler/ascParse.y"
    {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[(1) - (3)].dimen)),(yyvsp[(3) - (3)].frac_value));
	}
    break;

  case 252:
#line 2392 "base/generic/compiler/ascParse.y"
    {
	  CopyDimensions(&((yyvsp[(2) - (3)].dimen)),&((yyval.dimen)));
	}
    break;

  case 253:
#line 2399 "base/generic/compiler/ascParse.y"
    {
	  (yyval.frac_value) = (yyvsp[(1) - (2)].int_value) ? NegateF((yyvsp[(2) - (2)].frac_value)) : (yyvsp[(2) - (2)].frac_value);
	}
    break;

  case 254:
#line 2406 "base/generic/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(1) - (1)].int_value),(short)1);
	}
    break;

  case 255:
#line 2410 "base/generic/compiler/ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[(2) - (5)].int_value),(short)(yyvsp[(4) - (5)].int_value));
	}
    break;

  case 256:
#line 2417 "base/generic/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 257:
#line 2421 "base/generic/compiler/ascParse.y"
    {
	  (yyval.int_value) = 0;
	}
    break;

  case 258:
#line 2425 "base/generic/compiler/ascParse.y"
    {
	  (yyval.int_value) = 1;
	}
    break;

  case 259:
#line 2432 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr((yyvsp[(1) - (1)].int_value));
	}
    break;

  case 260:
#line 2436 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
    break;

  case 261:
#line 2440 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr((yyvsp[(1) - (1)].real_value),g_dim_ptr);
	}
    break;

  case 262:
#line 2444 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
    break;

  case 263:
#line 2448 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateTrueExpr();
	}
    break;

  case 264:
#line 2452 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateFalseExpr();
	}
    break;

  case 265:
#line 2456 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateAnyExpr();
	}
    break;

  case 266:
#line 2460 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[(1) - (1)].sym_ptr));
	}
    break;

  case 267:
#line 2464 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateVarExpr((yyvsp[(1) - (1)].nptr));
	}
    break;

  case 268:
#line 2468 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSetExpr((yyvsp[(2) - (3)].sptr));
	}
    break;

  case 269:
#line 2472 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 270:
#line 2477 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 271:
#line 2482 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 272:
#line 2487 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 273:
#line 2492 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 274:
#line 2497 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 275:
#line 2502 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 276:
#line 2507 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_not));
	}
    break;

  case 277:
#line 2511 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 278:
#line 2516 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),(yyvsp[(2) - (3)].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 279:
#line 2521 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 280:
#line 2526 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 281:
#line 2531 "base/generic/compiler/ascParse.y"
    {
	  (yyvsp[(3) - (3)].eptr) = JoinExprLists((yyvsp[(3) - (3)].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[(1) - (3)].eptr),(yyvsp[(3) - (3)].eptr));
	}
    break;

  case 282:
#line 2536 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (2)].eptr);
	}
    break;

  case 283:
#line 2540 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[(2) - (2)].eptr),CreateOpExpr(e_uminus));
	}
    break;

  case 284:
#line 2544 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (6)].nptr),(yyvsp[(5) - (6)].real_value),g_dim_ptr);
	}
    break;

  case 285:
#line 2548 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[(3) - (4)].nptr),DBL_MAX,NULL);
	}
    break;

  case 286:
#line 2552 "base/generic/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
    break;

  case 287:
#line 2559 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 288:
#line 2563 "base/generic/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
    break;

  case 289:
#line 2570 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 290:
#line 2574 "base/generic/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
    break;

  case 291:
#line 2581 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 292:
#line 2585 "base/generic/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
    break;

  case 293:
#line 2592 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 294:
#line 2596 "base/generic/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
    break;

  case 295:
#line 2603 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 296:
#line 2607 "base/generic/compiler/ascParse.y"
    {
	  DestroySetList((yyvsp[(3) - (4)].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
    break;

  case 297:
#line 2614 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[(3) - (4)].sptr));
	}
    break;

  case 298:
#line 2618 "base/generic/compiler/ascParse.y"
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

  case 299:
#line 2629 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[(2) - (3)].eptr);
	}
    break;

  case 300:
#line 2636 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
    break;

  case 301:
#line 2640 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
    break;

  case 302:
#line 2644 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
    break;

  case 303:
#line 2648 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
    break;

  case 304:
#line 2652 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
    break;

  case 305:
#line 2656 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
    break;

  case 306:
#line 2663 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
    break;

  case 307:
#line 2667 "base/generic/compiler/ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
    break;


/* Line 1267 of yacc.c.  */
#line 5175 "base/generic/compiler/ascParse.c"
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
      /* If just tried and failed to reuse look-ahead token after an
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

  /* Else will try to reuse look-ahead token after shifting the error
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

  if (yyn == YYFINAL)
    YYACCEPT;

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

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
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


#line 2671 "base/generic/compiler/ascParse.y"

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
    error_reporter(ASC_USER_ERROR,NULL,0,"%s at end of input.\n",s);
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

