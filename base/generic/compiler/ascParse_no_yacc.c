/* A Bison parser, made by GNU Bison 2.0.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

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
     ADD_T = 258,
     ALIASES_T = 259,
     AND_T = 260,
     ANY_T = 261,
     AREALIKE_T = 262,
     ARETHESAME_T = 263,
     ARRAY_T = 264,
     ASSERT_T = 265,
     ATOM_T = 266,
     BEQ_T = 267,
     BNE_T = 268,
     BREAK_T = 269,
     CALL_T = 270,
     CARD_T = 271,
     CASE_T = 272,
     CHOICE_T = 273,
     CHECK_T = 274,
     CONDITIONAL_T = 275,
     CONSTANT_T = 276,
     CONTINUE_T = 277,
     CREATE_T = 278,
     DATA_T = 279,
     DECREASING_T = 280,
     DEFAULT_T = 281,
     DEFINITION_T = 282,
     DIMENSION_T = 283,
     DIMENSIONLESS_T = 284,
     DO_T = 285,
     ELSE_T = 286,
     END_T = 287,
     EXPECT_T = 288,
     EXTERNAL_T = 289,
     FALSE_T = 290,
     FALLTHRU_T = 291,
     FIX_T = 292,
     FOR_T = 293,
     FREE_T = 294,
     FROM_T = 295,
     GLOBAL_T = 296,
     IF_T = 297,
     IMPORT_T = 298,
     IN_T = 299,
     INPUT_T = 300,
     INCREASING_T = 301,
     INTERACTIVE_T = 302,
     INTERSECTION_T = 303,
     ISA_T = 304,
     _IS_T = 305,
     ISREFINEDTO_T = 306,
     MAXIMIZE_T = 307,
     MAXINTEGER_T = 308,
     MAXREAL_T = 309,
     METHODS_T = 310,
     METHOD_T = 311,
     MINIMIZE_T = 312,
     MODEL_T = 313,
     NOT_T = 314,
     NOTES_T = 315,
     OF_T = 316,
     OR_T = 317,
     OTHERWISE_T = 318,
     OUTPUT_T = 319,
     PATCH_T = 320,
     PROD_T = 321,
     PROVIDE_T = 322,
     REFINES_T = 323,
     REPLACE_T = 324,
     REQUIRE_T = 325,
     RETURN_T = 326,
     RUN_T = 327,
     SATISFIED_T = 328,
     SELECT_T = 329,
     SIZE_T = 330,
     STOP_T = 331,
     SUCHTHAT_T = 332,
     SUM_T = 333,
     SWITCH_T = 334,
     THEN_T = 335,
     TRUE_T = 336,
     UNION_T = 337,
     UNITS_T = 338,
     UNIVERSAL_T = 339,
     WHEN_T = 340,
     WHERE_T = 341,
     WHILE_T = 342,
     WILLBE_T = 343,
     WILLBETHESAME_T = 344,
     WILLNOTBETHESAME_T = 345,
     ASSIGN_T = 346,
     CASSIGN_T = 347,
     DBLCOLON_T = 348,
     USE_T = 349,
     LEQ_T = 350,
     GEQ_T = 351,
     NEQ_T = 352,
     DOTDOT_T = 353,
     WITH_T = 354,
     VALUE_T = 355,
     WITH_VALUE_T = 356,
     REAL_T = 357,
     INTEGER_T = 358,
     IDENTIFIER_T = 359,
     BRACEDTEXT_T = 360,
     SYMBOL_T = 361,
     DQUOTE_T = 362,
     UPLUS_T = 363,
     UMINUS_T = 364
   };
#endif
#define ADD_T 258
#define ALIASES_T 259
#define AND_T 260
#define ANY_T 261
#define AREALIKE_T 262
#define ARETHESAME_T 263
#define ARRAY_T 264
#define ASSERT_T 265
#define ATOM_T 266
#define BEQ_T 267
#define BNE_T 268
#define BREAK_T 269
#define CALL_T 270
#define CARD_T 271
#define CASE_T 272
#define CHOICE_T 273
#define CHECK_T 274
#define CONDITIONAL_T 275
#define CONSTANT_T 276
#define CONTINUE_T 277
#define CREATE_T 278
#define DATA_T 279
#define DECREASING_T 280
#define DEFAULT_T 281
#define DEFINITION_T 282
#define DIMENSION_T 283
#define DIMENSIONLESS_T 284
#define DO_T 285
#define ELSE_T 286
#define END_T 287
#define EXPECT_T 288
#define EXTERNAL_T 289
#define FALSE_T 290
#define FALLTHRU_T 291
#define FIX_T 292
#define FOR_T 293
#define FREE_T 294
#define FROM_T 295
#define GLOBAL_T 296
#define IF_T 297
#define IMPORT_T 298
#define IN_T 299
#define INPUT_T 300
#define INCREASING_T 301
#define INTERACTIVE_T 302
#define INTERSECTION_T 303
#define ISA_T 304
#define _IS_T 305
#define ISREFINEDTO_T 306
#define MAXIMIZE_T 307
#define MAXINTEGER_T 308
#define MAXREAL_T 309
#define METHODS_T 310
#define METHOD_T 311
#define MINIMIZE_T 312
#define MODEL_T 313
#define NOT_T 314
#define NOTES_T 315
#define OF_T 316
#define OR_T 317
#define OTHERWISE_T 318
#define OUTPUT_T 319
#define PATCH_T 320
#define PROD_T 321
#define PROVIDE_T 322
#define REFINES_T 323
#define REPLACE_T 324
#define REQUIRE_T 325
#define RETURN_T 326
#define RUN_T 327
#define SATISFIED_T 328
#define SELECT_T 329
#define SIZE_T 330
#define STOP_T 331
#define SUCHTHAT_T 332
#define SUM_T 333
#define SWITCH_T 334
#define THEN_T 335
#define TRUE_T 336
#define UNION_T 337
#define UNITS_T 338
#define UNIVERSAL_T 339
#define WHEN_T 340
#define WHERE_T 341
#define WHILE_T 342
#define WILLBE_T 343
#define WILLBETHESAME_T 344
#define WILLNOTBETHESAME_T 345
#define ASSIGN_T 346
#define CASSIGN_T 347
#define DBLCOLON_T 348
#define USE_T 349
#define LEQ_T 350
#define GEQ_T 351
#define NEQ_T 352
#define DOTDOT_T 353
#define WITH_T 354
#define VALUE_T 355
#define WITH_VALUE_T 356
#define REAL_T 357
#define INTEGER_T 358
#define IDENTIFIER_T 359
#define BRACEDTEXT_T 360
#define SYMBOL_T 361
#define DQUOTE_T 362
#define UPLUS_T 363
#define UMINUS_T 364




/* Copy the first part of user declarations.  */
#line 30 "base\\generic\\compiler\\ascParse.y"

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>		/* need DBL_MAX and LONG_MAX */
#include <float.h>		/* on a NeXT they are in here */

#include <compiler/parser.h>

#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>

#include <compiler/scanner.h>
#include <compiler/symtab.h>		/* the global string/symbol table */
#include <compiler/notate.h>		/* notes database wrapper */
#include <compiler/braced.h>


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

#ifndef lint
static CONST char ParserID[] = "$Id: ascParse.y,v 1.23 2000/01/25 02:25:59 ballan Exp $";
#endif

int g_compiler_warnings = 1;		/* level of whine to allow */

#include <compiler/redirectFile.h>
#ifndef ASCERR
#error "ASCERR not defined"
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
/*********************************************************************\
 * g_type_name is used by the scanner when closing a module to check if
 * the parser is in the middle of parsing a MODEL or ATOM type
 * definition.
 *      g_type_name == NULL implies that the parser is in between
 *                          definitions. This depends on proper
 *                          resets to NULL at END of type productions.
 *      g_type_name != NULL implies that the parser is in the middle
 *                          of a type definition and gives the name
 *                          of that type.
\*********************************************************************/

static symchar *g_end_identifier = NULL;
/*  This variable gets reset every time we see an ``END_T''.  If the
 *  token after END_T is missing (i.e., ``END;'') or if it is recognized
 *  (e.g., FOR_T), set this variable to NULL.  When we see an
 *  ``END_T IDENTIFIER_T'', set this variable to the pointer into the
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
 * contexts where args are not allowed, use IDENTIFIER_T instead of the
 * type_identifier production.
 */

static struct Set *g_callargs=NULL;
/* optional args to a user defined method.
 * it is set in the production call_identifier. in
 * contexts where args are not allowed, use IDENTIFIER_T instead of the
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
static void ErrMsg_ProcTypeMissing(CONST char *, CONST char *);
static void ErrMsg_ProcsRejected(CONST char *, CONST char *);
static void ErrMsg_DuplicateProc(struct InitProcedure *);
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
 *      NOTES ( SYMBOL_T ( fvarlist BRACEDTEXT_T )+ )+ END NOTES ';'
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
 *  a SELF_T token.  The latter is certainly easier to implement from
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
#define DEBUG_NOTES(s) 0

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

/* For 'inline' notes, note on DQUOTE_T from scanner.l:
 * Remember that DQUOTE_T is a string value which is local to the
 * production that finds it. It must be copied if you want to
 * keep it.
 */


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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 299 "base\\generic\\compiler\\ascParse.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 190 of yacc.c.  */
#line 599 "base\\generic\\compiler\\ascParse.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 611 "base\\generic\\compiler\\ascParse.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
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
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1180

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  127
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  116
/* YYNRULES -- Number of rules. */
#define YYNRULES  307
/* YYNRULES -- Number of states. */
#define YYNSTATES  615

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   364

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
static const unsigned short int yyprhs[] =
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

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     128,     0,    -1,    -1,   128,   129,    -1,   131,    -1,   132,
      -1,   133,    -1,   136,    -1,   138,    -1,   134,    -1,   144,
      -1,   140,    -1,   147,    -1,   156,    -1,   153,    -1,   158,
      -1,   130,    -1,     1,    -1,    41,   120,   168,   226,   120,
      -1,    70,   107,   120,    -1,    70,   225,   120,    -1,    70,
     225,    -1,    67,   107,   120,    -1,    67,   225,   120,    -1,
      67,   225,    -1,    43,   104,    40,   107,   120,    -1,    43,
     104,   120,    -1,   135,   220,   226,   120,    -1,     3,    60,
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
static const unsigned short int yyrline[] =
{
       0,   410,   410,   412,   416,   417,   418,   419,   420,   421,
     422,   423,   424,   425,   426,   427,   428,   429,   436,   481,
     485,   490,   498,   502,   507,   515,   519,   526,   557,   565,
     585,   595,   603,   621,   630,   638,   691,   702,   711,   716,
     721,   728,   735,   745,   784,   819,   824,   829,   836,   843,
     853,   894,   903,   916,   925,   928,   936,   939,   947,   950,
     957,   989,  1003,  1006,  1013,  1043,  1052,  1057,  1077,  1080,
    1088,  1098,  1102,  1101,  1115,  1123,  1126,  1149,  1163,  1172,
    1180,  1183,  1191,  1199,  1200,  1201,  1202,  1203,  1204,  1205,
    1206,  1207,  1208,  1209,  1210,  1211,  1212,  1213,  1214,  1215,
    1216,  1217,  1218,  1219,  1220,  1221,  1222,  1223,  1224,  1225,
    1226,  1227,  1228,  1232,  1273,  1302,  1306,  1357,  1360,  1367,
    1379,  1406,  1411,  1419,  1424,  1433,  1436,  1444,  1447,  1455,
    1458,  1465,  1472,  1479,  1486,  1493,  1497,  1504,  1517,  1534,
    1544,  1552,  1563,  1576,  1583,  1591,  1594,  1601,  1622,  1625,
    1632,  1654,  1657,  1661,  1668,  1672,  1676,  1680,  1687,  1691,
    1698,  1705,  1712,  1723,  1734,  1740,  1750,  1760,  1763,  1770,
    1781,  1793,  1800,  1810,  1817,  1821,  1825,  1829,  1836,  1840,
    1844,  1848,  1852,  1859,  1866,  1877,  1887,  1894,  1898,  1902,
    1906,  1913,  1924,  1934,  1941,  1945,  1949,  1953,  1960,  1970,
    2007,  2016,  2031,  2036,  2045,  2056,  2060,  2065,  2078,  2098,
    2102,  2107,  2122,  2127,  2132,  2137,  2142,  2147,  2152,  2157,
    2162,  2167,  2172,  2177,  2182,  2187,  2196,  2199,  2207,  2210,
    2217,  2222,  2228,  2232,  2236,  2241,  2249,  2255,  2264,  2268,
    2290,  2294,  2315,  2319,  2324,  2330,  2334,  2341,  2345,  2349,
    2353,  2357,  2361,  2368,  2375,  2379,  2387,  2390,  2394,  2401,
    2405,  2409,  2413,  2417,  2421,  2425,  2429,  2433,  2437,  2441,
    2446,  2451,  2456,  2461,  2466,  2471,  2476,  2480,  2485,  2490,
    2495,  2500,  2505,  2509,  2513,  2517,  2521,  2528,  2532,  2539,
    2543,  2550,  2554,  2561,  2565,  2572,  2576,  2583,  2587,  2598,
    2605,  2609,  2613,  2617,  2621,  2625,  2632,  2636
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ADD_T", "ALIASES_T", "AND_T", "ANY_T",
  "AREALIKE_T", "ARETHESAME_T", "ARRAY_T", "ASSERT_T", "ATOM_T", "BEQ_T",
  "BNE_T", "BREAK_T", "CALL_T", "CARD_T", "CASE_T", "CHOICE_T", "CHECK_T",
  "CONDITIONAL_T", "CONSTANT_T", "CONTINUE_T", "CREATE_T", "DATA_T",
  "DECREASING_T", "DEFAULT_T", "DEFINITION_T", "DIMENSION_T",
  "DIMENSIONLESS_T", "DO_T", "ELSE_T", "END_T", "EXPECT_T", "EXTERNAL_T",
  "FALSE_T", "FALLTHRU_T", "FIX_T", "FOR_T", "FREE_T", "FROM_T",
  "GLOBAL_T", "IF_T", "IMPORT_T", "IN_T", "INPUT_T", "INCREASING_T",
  "INTERACTIVE_T", "INTERSECTION_T", "ISA_T", "_IS_T", "ISREFINEDTO_T",
  "MAXIMIZE_T", "MAXINTEGER_T", "MAXREAL_T", "METHODS_T", "METHOD_T",
  "MINIMIZE_T", "MODEL_T", "NOT_T", "NOTES_T", "OF_T", "OR_T",
  "OTHERWISE_T", "OUTPUT_T", "PATCH_T", "PROD_T", "PROVIDE_T", "REFINES_T",
  "REPLACE_T", "REQUIRE_T", "RETURN_T", "RUN_T", "SATISFIED_T", "SELECT_T",
  "SIZE_T", "STOP_T", "SUCHTHAT_T", "SUM_T", "SWITCH_T", "THEN_T",
  "TRUE_T", "UNION_T", "UNITS_T", "UNIVERSAL_T", "WHEN_T", "WHERE_T",
  "WHILE_T", "WILLBE_T", "WILLBETHESAME_T", "WILLNOTBETHESAME_T",
  "ASSIGN_T", "CASSIGN_T", "DBLCOLON_T", "USE_T", "LEQ_T", "GEQ_T",
  "NEQ_T", "DOTDOT_T", "WITH_T", "VALUE_T", "WITH_VALUE_T", "REAL_T",
  "INTEGER_T", "IDENTIFIER_T", "BRACEDTEXT_T", "SYMBOL_T", "DQUOTE_T",
  "','", "'|'", "'<'", "'='", "'>'", "'+'", "'-'", "'/'", "'*'", "UPLUS_T",
  "UMINUS_T", "'^'", "';'", "'('", "')'", "':'", "'.'", "'['", "']'",
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
static const unsigned short int yytoknum[] =
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
static const unsigned char yyr1[] =
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
static const unsigned char yyr2[] =
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
static const unsigned short int yydefact[] =
{
       2,     0,     1,    17,     0,     0,     0,     0,     0,     0,
       0,     0,    68,    63,     3,    16,     4,     5,     6,     9,
       0,     7,    75,     8,    75,    11,    10,    12,    14,    80,
       0,    13,    80,    15,     0,     0,     0,    65,    80,     0,
       0,   209,     0,    24,     0,     0,    21,     0,     0,     0,
       0,    74,     0,    71,     0,     0,     0,     0,    80,     0,
      43,    80,    54,    71,    66,     0,     0,     0,     0,    26,
       0,    22,    23,     0,   231,     0,    19,    20,   225,     0,
      69,    67,   200,     0,   204,   205,   228,     0,     0,     0,
       0,    76,     0,     0,    72,     0,     0,   265,     0,   178,
       0,     0,     0,    80,   179,     0,   264,   180,     0,     0,
       0,     0,     0,     0,   260,   262,     0,     0,     0,     0,
     181,     0,     0,     0,   226,     0,     0,   263,     0,     0,
       0,     0,   240,   259,   209,   266,     0,     0,     0,   231,
     112,     0,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,   137,    95,    94,    98,    99,   100,   101,
      97,    96,   102,   103,   104,   105,   107,   106,   108,   109,
     110,   111,     0,   267,   261,   139,    37,     0,    53,    71,
       0,    71,    80,    56,     0,     0,     0,   125,     0,     0,
       0,   210,   267,     0,   230,   232,     0,     0,   212,   213,
     221,   214,   215,   216,   217,   218,   219,   220,   222,   223,
     224,     0,     0,   202,     0,   207,   229,   208,   201,    27,
      29,    78,    80,    32,    75,     0,    82,   164,   121,   163,
     231,   231,   231,   231,     0,     0,   160,     0,   161,     0,
     231,   231,   141,   140,   276,     0,   231,   231,   158,     0,
       0,     0,   227,   182,   231,   231,     0,     0,   231,   231,
       0,     0,     0,   183,   241,   238,   239,     0,   282,   283,
       0,     0,    81,     0,   131,   132,     0,     0,     0,     0,
     133,   134,     0,     0,     0,     0,   306,   307,     0,     0,
       0,   303,   304,   305,     0,   301,   300,   302,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   244,     0,     0,
       0,     0,     0,     0,    30,     0,     0,    18,    25,    61,
     211,     0,     0,     0,    33,     0,   203,   206,     0,    73,
      60,   231,     0,     0,     0,     0,   198,     0,     0,    80,
       0,     0,   199,     0,     0,     0,     0,     0,   231,     0,
       0,   186,     0,     0,     0,   231,     0,     0,   193,     0,
       0,     0,   231,     0,     0,   173,    80,     0,   299,   268,
       0,   115,   123,   127,   127,   120,   127,   135,   136,     0,
     209,   138,   274,   279,   275,   281,   280,   269,   270,   272,
     271,   273,   277,   278,   244,     0,     0,   243,    38,     0,
      55,    80,     0,    51,    64,    31,   126,    28,   234,   233,
      34,    70,     0,     0,   294,   295,   296,   297,     0,   151,
     167,   292,   293,   288,   289,   159,     0,   285,     0,     0,
      80,   184,   231,     0,   286,   287,     0,     0,    80,   191,
     231,     0,   290,   291,     0,     0,    80,   169,   231,     0,
       0,   298,     0,   231,     0,   129,   119,   129,     0,     0,
       0,    45,    35,   248,   247,   245,     0,   242,   246,   256,
       0,    50,     0,    58,     0,    77,   122,   162,   153,   152,
       0,    80,     0,     0,     0,     0,    80,   188,     0,    80,
       0,    80,   195,     0,    80,     0,    80,   175,     0,    80,
     166,     0,     0,   128,     0,   113,   114,     0,     0,     0,
       0,   267,   256,   228,     0,     0,     0,   256,    40,    41,
      42,   257,   258,     0,    36,    57,    80,     0,   235,   156,
     154,   157,   155,    80,   168,   165,   284,   185,   187,    80,
     190,   192,   194,    80,   197,   171,   174,    80,   177,     0,
     124,   130,     0,   170,     0,     0,     0,    48,    47,    49,
       0,     0,   252,   249,   250,   251,     0,   236,    39,   237,
       0,    52,     0,   189,   196,   176,     0,     0,   145,     0,
       0,   143,    46,    44,   254,     0,   253,    59,   150,     0,
     172,     0,     0,     0,   148,     0,     0,     0,   142,   144,
       0,   147,     0,     0,     0,   149,     0,   117,   146,   255,
       0,   116,   231,     0,   118
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    58,    59,   470,    26,    60,   513,
      27,    61,    62,   183,   311,   527,    28,    29,    30,    31,
      32,    33,    34,    47,    80,    95,   224,    50,    51,    91,
      92,    53,    54,   141,   142,   143,   144,   611,   145,   146,
     229,   373,   316,   455,   505,   147,   148,   149,   150,   151,
     152,   153,   154,   509,   578,   592,   155,   601,   156,   480,
     533,   157,   158,   159,   160,   161,   162,   163,   164,   482,
     165,   364,   365,   166,   167,   168,   350,   351,   169,   357,
     358,   170,   171,    49,    82,    83,    84,   192,    86,    81,
     253,   217,   193,   194,   568,   174,   265,   398,   467,   468,
     565,   586,   523,   195,   303,   304
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -437
static const short int yypact[] =
{
    -437,  1096,  -437,  -437,    81,   -59,     2,    25,    38,    57,
       1,    82,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
      71,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
      10,  -437,  -437,  -437,    77,   167,   179,  -437,  -437,   -30,
     202,  -437,   130,    43,   204,   133,    60,   -18,   150,   -19,
     223,   206,   223,   203,   593,   156,   159,   160,  -437,   197,
    -437,  -437,   145,   203,  -437,   -11,   166,   223,   168,  -437,
     169,  -437,  -437,   170,   165,   -10,  -437,  -437,   476,   161,
    -437,  -437,   150,   171,   -61,  -437,     0,   150,   157,   162,
     176,  -437,   163,   164,  -437,   223,   172,  -437,   165,  -437,
     181,    33,    44,  -437,  -437,   183,  -437,  -437,   150,   189,
     150,   165,    89,   165,  -437,  -437,   165,   165,    71,    96,
    -437,   150,   173,   -72,   186,   105,   -64,  -437,   108,   -62,
     165,   150,   190,   191,   177,  -437,   165,   165,   165,   165,
    -437,   180,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,    59,    84,  -437,   960,  -437,   231,  -437,   203,
     198,   203,  -437,   215,   223,   247,   193,   251,   194,   195,
     196,  -437,  -437,   185,   210,   801,   261,   207,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,   218,   219,  -437,   150,  -437,  -437,  -437,   150,  -437,
    -437,  -437,  -437,  -437,  -437,   212,  -437,   960,   205,  -437,
     165,   165,   165,   165,   223,   213,  -437,   289,  -437,   844,
     165,   165,   960,   960,  -437,   -19,   165,   165,   243,   150,
     150,    -5,  -437,  -437,   165,   165,   150,    -2,   165,   165,
     150,     7,   874,  -437,  -437,  -437,  -437,   165,   242,   242,
     244,   211,  -437,    -4,  -437,  -437,   246,   258,   246,   246,
    -437,  -437,   165,   165,   694,   165,  -437,  -437,   165,   165,
     165,  -437,  -437,  -437,   165,  -437,  -437,  -437,   165,   165,
     165,   165,   165,   165,   165,   260,   223,   187,   223,   245,
     248,   -49,   252,   253,  -437,   264,   254,  -437,  -437,  -437,
    -437,   165,   165,   255,  -437,   256,  -437,  -437,   223,  -437,
    -437,   165,   249,   239,   257,   259,  -437,   150,   165,  -437,
     262,   263,  -437,   268,   265,   150,     6,   270,   165,   271,
     223,    16,   274,   267,   276,   165,   279,   223,    36,   282,
     280,   283,   165,   285,   223,    48,  -437,   715,  -437,  -437,
     150,  -437,   288,   316,   316,  -437,   316,   960,   960,    11,
     290,  -437,   233,   233,   233,   375,   375,    63,    63,   242,
     242,   242,    80,   990,   187,   266,   -66,  -437,   352,   293,
    -437,  -437,   277,  -437,  -437,  -437,  -437,  -437,   917,   960,
    -437,  -437,   296,   295,  -437,  -437,  -437,  -437,   301,   758,
     387,  -437,  -437,  -437,  -437,  -437,   -26,  -437,    -5,   302,
    -437,  -437,   165,   303,  -437,  -437,    -2,   304,  -437,  -437,
     165,   305,  -437,  -437,     7,   306,  -437,  -437,   165,   308,
     223,  -437,   311,   165,   320,   333,  -437,   333,   150,     7,
     165,   291,  -437,  -437,  -437,  -437,   -52,  -437,    90,   -17,
     319,  -437,   314,   321,   165,  -437,  -437,  -437,  -437,  -437,
     127,  -437,   223,   191,   318,   223,  -437,  -437,   322,  -437,
     223,  -437,  -437,   323,  -437,   223,  -437,  -437,   324,  -437,
    -437,   355,   327,  -437,   165,  -437,  -437,   329,   223,   332,
     116,   -79,    39,   337,    40,   -52,   -52,   128,  -437,  -437,
    -437,  -437,  -437,   142,  -437,  -437,  -437,   334,   960,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,   150,
    -437,   960,     7,  -437,   150,   350,   411,  -437,  -437,  -437,
     142,   339,  -437,   338,   338,  -437,   -75,   191,  -437,  -437,
     340,  -437,   223,  -437,  -437,  -437,   415,   223,   346,   344,
     347,  -437,  -437,  -437,  -437,   370,  -437,  -437,  -437,   371,
    -437,   150,   354,   410,   433,   363,   418,   357,  -437,  -437,
     150,  -437,   378,   379,   458,  -437,   376,   391,  -437,  -437,
     372,  -437,   165,   377,  -437
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,   443,  -437,  -437,   -41,  -437,   -20,  -437,  -437,
    -437,   -31,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,   -27,  -437,  -253,    46,  -437,  -437,  -437,  -437,  -437,
    -437,   216,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -436,  -437,  -437,  -437,  -437,    76,  -437,  -437,    69,
    -437,  -437,  -437,   389,   422,   -28,  -437,   -48,   226,   -47,
    -437,    -3,  -128,  -437,   -45,  -417,  -437,   117,  -437,  -431,
    -437,  -437,  -373,   284,  -437,  -437
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -206
static const short int yytable[] =
{
      85,    63,    88,    89,    52,    93,   173,    67,   495,   484,
      68,   271,   348,    78,    78,   355,   185,   196,   518,   402,
     188,    55,   184,   508,   362,  -205,   172,   179,   584,  -205,
     181,    56,    41,   432,    85,   514,   215,   463,   464,    85,
      41,  -205,    41,    41,  -205,    37,   585,   214,   225,   250,
     465,   463,   464,   440,   212,   466,    44,   256,   349,   260,
      85,   356,    85,   273,   519,   448,   274,   275,    57,   466,
     363,   403,   234,   248,   557,    85,   132,   483,    85,   433,
     236,    85,   238,   263,   563,   564,    79,    87,  -205,   520,
      69,  -205,  -205,   186,   197,   251,   521,   522,   257,   441,
      41,   261,   332,   333,   334,   335,   569,   216,   276,   277,
     278,   449,   340,   341,   426,    41,   577,   370,   343,   344,
     558,   456,    38,   457,    73,    74,   352,   353,   427,    39,
     359,   360,   458,  -205,  -205,  -205,    35,   312,   306,   560,
     308,    36,    40,   569,   566,   559,   529,   279,   280,   281,
     530,   309,   521,   522,   230,   515,   516,   531,   231,   517,
     532,    41,   562,    72,    42,   232,   327,    73,    74,   233,
      85,    97,  -205,  -205,  -205,   282,   283,    48,   300,   301,
      77,   101,   302,   102,    73,    74,    41,   336,  -205,    45,
     212,   328,  -205,   298,   299,   300,   301,    64,   342,   302,
     106,   346,    85,   413,   329,   515,   516,   284,    85,   517,
     240,    65,    85,   112,   241,   396,   397,   246,   114,   115,
     429,   247,   347,    66,   117,   371,   254,   437,   354,   258,
     255,   119,   361,   259,   445,    43,   555,    46,   122,   556,
      70,   521,   522,   125,   132,   567,   127,   128,    75,   285,
      71,   375,   376,    76,    41,    78,   286,   287,    94,   395,
     176,   399,    90,   177,   178,   180,   182,   132,   133,   134,
     187,   135,   211,   190,   191,   189,   213,   219,   136,   137,
     221,   412,   220,   222,   223,   228,   138,   235,   288,    85,
     139,   252,   226,   237,   249,   264,   266,   425,   267,   305,
     272,   310,   307,   431,   488,   313,   289,   315,   420,   418,
     439,   320,   493,   314,   317,   318,   319,   447,   321,   323,
     498,   290,    85,   325,   326,   502,   331,   324,   291,   292,
     293,    85,   330,   338,   337,   450,   345,   369,   175,   291,
     292,   293,   452,   295,   296,   297,   298,   299,   300,   301,
     372,   459,   302,   294,   295,   296,   297,   298,   299,   300,
     301,   302,   374,   302,   394,   415,   368,   400,   406,   401,
     472,   414,   404,   405,   407,   410,   411,   454,   469,   416,
     285,   473,   227,   512,   421,   417,   462,   286,   287,   422,
     423,   424,   428,   435,   430,   239,   434,   242,   436,   487,
     243,   244,   438,   500,   442,   444,   443,   492,   446,   453,
      85,   460,   511,   471,   262,   497,   475,   476,   481,   288,
     268,   269,   270,   477,   503,   486,   489,   491,   494,   496,
     507,   499,   510,   501,   504,   535,   525,   289,   537,   524,
     536,   549,   526,   541,   216,   539,   543,   547,   545,   550,
     534,   552,   554,   580,   571,   538,   581,   517,   540,   583,
     542,   553,   587,   544,   589,   546,   591,   593,   548,   594,
     291,   292,   293,   595,   599,   596,   598,   600,   602,   603,
     604,   606,   608,   607,   613,   295,   296,   297,   298,   299,
     300,   301,   610,   612,   302,   570,   198,   140,   609,   614,
     381,    85,   572,   506,   485,   490,    85,   245,   573,   218,
     561,   461,   574,     0,   199,   582,   575,   200,   201,     0,
       0,   576,     0,   202,     0,   588,   579,     0,     0,     0,
     590,   203,     0,     0,     0,     0,   204,     0,     0,     0,
       0,     0,     0,   597,     0,     0,     0,     0,     0,     0,
     205,   367,   605,     0,     0,   206,     0,     0,     0,   207,
       0,   208,     0,   209,     0,     0,   377,   378,   175,   382,
       0,     0,   383,   384,   385,     0,     0,     0,   386,     0,
     210,     0,   387,   388,   389,   390,   391,   392,   393,     0,
       0,     0,     0,     0,    96,     0,     0,     0,     0,    97,
       0,     0,     0,    98,     0,   408,   409,    99,   100,   101,
     -79,   102,     0,   103,     0,   104,     0,     0,     0,     0,
       0,     0,   419,     0,   -79,   -79,     0,   105,   106,   107,
     108,   109,   110,     0,     0,   111,     0,     0,     0,     0,
       0,   112,     0,     0,     0,   113,   114,   115,   -79,     0,
     116,     0,   117,   118,     0,     0,   -79,     0,     0,   119,
       0,     0,     0,     0,   120,   121,   122,   123,     0,   124,
       0,   125,   126,     0,   127,   128,    12,     0,   129,     0,
     130,     0,     0,     0,     0,     0,     0,   131,     0,     0,
       0,     0,     0,     0,     0,   132,   133,   134,     0,   135,
      97,     0,     0,     0,     0,     0,   136,   137,     0,     0,
     101,     0,   102,     0,   138,   -79,     0,     0,   139,     0,
     285,     0,     0,     0,     0,     0,     0,   286,   287,   106,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   112,     0,   367,     0,   113,   114,   115,     0,
       0,   116,     0,   117,     0,     0,     0,     0,   528,   288,
     119,     0,     0,   285,     0,     0,     0,   122,     0,     0,
     286,   287,   125,     0,     0,   127,   128,   289,     0,   379,
       0,     0,     0,   478,     0,     0,     0,     0,   551,     0,
       0,     0,   290,     0,     0,     0,   132,   133,   380,     0,
     135,     0,   288,     0,   479,     0,   285,   136,   137,     0,
     291,   292,   293,   286,   287,   138,     0,     0,     0,   139,
     289,     0,     0,     0,   294,   295,   296,   297,   298,   299,
     300,   301,     0,     0,   302,   290,     0,   451,     0,     0,
       0,     0,     0,     0,     0,   288,     0,     0,     0,   285,
       0,     0,     0,   291,   292,   293,   286,   287,     0,     0,
       0,     0,     0,   289,     0,     0,     0,   294,   295,   296,
     297,   298,   299,   300,   301,     0,     0,   302,   290,   285,
       0,     0,     0,     0,     0,     0,   286,   287,   288,     0,
       0,     0,     0,     0,     0,     0,   291,   292,   293,   322,
       0,     0,     0,     0,   366,     0,   289,     0,     0,     0,
     294,   295,   296,   297,   298,   299,   300,   301,   288,     0,
     302,   290,   285,     0,   339,     0,     0,     0,     0,   286,
     287,     0,     0,     0,     0,     0,   289,     0,     0,   291,
     292,   293,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   290,     0,   294,   295,   296,   297,   298,   299,   300,
     301,   288,     0,   302,     0,   285,     0,     0,     0,   291,
     292,   293,   286,   287,     0,     0,     0,     0,     0,   289,
       0,     0,     0,   294,   295,   296,   297,   298,   299,   300,
     301,     0,     0,   302,   290,   285,     0,     0,     0,     0,
       0,     0,     0,     0,   288,     0,     0,     0,     0,     0,
       0,     0,   291,   292,   293,   474,     0,     0,     0,     0,
       0,     0,   289,     0,     0,     0,   294,   295,   296,   297,
     298,   299,   300,   301,   288,     0,   302,   290,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   289,     0,     0,   291,   292,   293,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   294,
     295,   296,   297,   298,   299,   300,   301,     0,     0,   302,
       0,     0,     0,     0,     0,   291,   292,   293,     0,     0,
       0,     0,     0,     0,     0,     0,     2,     3,     0,     4,
     295,   296,   297,   298,   299,   300,   301,   -62,     0,   302,
       0,     0,     0,     0,     0,     0,     0,   -62,     0,     0,
       0,     0,     0,     5,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     6,     0,     7,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   -62,     0,     0,     0,     0,     0,
       0,     8,     0,     9,     0,    10,    11,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    12,
      13
};

static const short int yycheck[] =
{
      48,    32,    49,    50,    24,    52,    54,    38,   444,   426,
      40,   139,    17,    32,    32,    17,    27,    27,    35,    68,
      67,    11,    63,   459,    17,   104,    54,    58,   103,   108,
      61,    21,   104,    17,    82,   466,    84,   103,   104,    87,
     104,   120,   104,   104,   123,   104,   121,   108,    95,   121,
     116,   103,   104,    17,    82,   121,    55,   121,    63,   121,
     108,    63,   110,     4,    81,    17,     7,     8,    58,   121,
      63,   120,   103,   121,    35,   123,   102,   103,   126,    63,
     108,   129,   110,   131,   515,   516,   104,   106,     4,   106,
     120,     7,     8,   104,   104,   123,   113,   114,   126,    63,
     104,   129,   230,   231,   232,   233,   523,   107,    49,    50,
      51,    63,   240,   241,   108,   104,   552,   121,   246,   247,
      81,   374,   120,   376,   124,   125,   254,   255,   122,   104,
     258,   259,   121,    49,    50,    51,    55,   184,   179,   512,
     181,    60,   104,   560,   517,   106,    19,    88,    89,    90,
      23,   182,   113,   114,   121,   115,   116,    30,   125,   119,
      33,   104,   122,   120,   107,   121,   214,   124,   125,   125,
     218,     6,    88,    89,    90,    91,    92,   106,   115,   116,
     120,    16,   119,    18,   124,   125,   104,   234,   104,   107,
     218,   222,   108,   113,   114,   115,   116,   120,   245,   119,
      35,   249,   250,   331,   224,   115,   116,   123,   256,   119,
     121,    44,   260,    48,   125,    28,    29,   121,    53,    54,
     348,   125,   250,    44,    59,   273,   121,   355,   256,   121,
     125,    66,   260,   125,   362,     9,   120,    11,    73,   123,
      38,   113,   114,    78,   102,   103,    81,    82,    44,     5,
     120,   278,   279,   120,   104,    32,    12,    13,    55,   306,
     104,   308,    56,   104,   104,    68,   121,   102,   103,   104,
     104,   106,   111,   104,   104,   107,   105,   120,   113,   114,
     104,   328,   120,   120,   120,   104,   121,   104,    44,   337,
     125,   105,   120,   104,   121,   105,   105,   345,   121,    68,
     120,    86,   104,   350,   432,    58,    62,    56,   339,   337,
     357,   126,   440,   120,   120,   120,   120,   364,   108,    58,
     448,    77,   370,   105,   105,   453,   121,   120,    95,    96,
      97,   379,   120,    44,   121,   366,    93,   126,    54,    95,
      96,    97,   370,   110,   111,   112,   113,   114,   115,   116,
     104,   379,   119,   109,   110,   111,   112,   113,   114,   115,
     116,   119,   104,   119,   104,   126,   122,   122,   104,   121,
     401,   122,   120,   120,   120,   120,   120,    61,    26,   122,
       5,   104,    98,    92,   122,   126,   120,    12,    13,   126,
     122,   126,   122,   126,   123,   111,   122,   113,   122,   430,
     116,   117,   123,   450,   122,   122,   126,   438,   123,   121,
     458,   121,   460,   120,   130,   446,   120,   122,    31,    44,
     136,   137,   138,   122,   104,   123,   123,   123,   123,   123,
     458,   123,   460,   122,   101,   482,   122,    62,   485,   120,
     122,    86,   121,   490,   107,   123,   123,   123,   495,   122,
     481,   122,   120,   103,   120,   486,    45,   119,   489,   120,
     491,   508,   122,   494,    49,   496,   120,   123,   499,   122,
      95,    96,    97,   103,    64,   104,   122,    44,   115,    61,
     123,   103,    24,   104,   612,   110,   111,   112,   113,   114,
     115,   116,   101,   121,   119,   526,    20,    54,   122,   122,
     284,   549,   533,   457,   428,   436,   554,   118,   539,    87,
     513,   394,   543,    -1,    38,   560,   547,    41,    42,    -1,
      -1,   549,    -1,    47,    -1,   572,   554,    -1,    -1,    -1,
     577,    55,    -1,    -1,    -1,    -1,    60,    -1,    -1,    -1,
      -1,    -1,    -1,   591,    -1,    -1,    -1,    -1,    -1,    -1,
      74,   267,   600,    -1,    -1,    79,    -1,    -1,    -1,    83,
      -1,    85,    -1,    87,    -1,    -1,   282,   283,   284,   285,
      -1,    -1,   288,   289,   290,    -1,    -1,    -1,   294,    -1,
     104,    -1,   298,   299,   300,   301,   302,   303,   304,    -1,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,     6,
      -1,    -1,    -1,    10,    -1,   321,   322,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,
      -1,    -1,   338,    -1,    31,    32,    -1,    34,    35,    36,
      37,    38,    39,    -1,    -1,    42,    -1,    -1,    -1,    -1,
      -1,    48,    -1,    -1,    -1,    52,    53,    54,    55,    -1,
      57,    -1,    59,    60,    -1,    -1,    63,    -1,    -1,    66,
      -1,    -1,    -1,    -1,    71,    72,    73,    74,    -1,    76,
      -1,    78,    79,    -1,    81,    82,    83,    -1,    85,    -1,
      87,    -1,    -1,    -1,    -1,    -1,    -1,    94,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   102,   103,   104,    -1,   106,
       6,    -1,    -1,    -1,    -1,    -1,   113,   114,    -1,    -1,
      16,    -1,    18,    -1,   121,   122,    -1,    -1,   125,    -1,
       5,    -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    35,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    48,    -1,   460,    -1,    52,    53,    54,    -1,
      -1,    57,    -1,    59,    -1,    -1,    -1,    -1,   474,    44,
      66,    -1,    -1,     5,    -1,    -1,    -1,    73,    -1,    -1,
      12,    13,    78,    -1,    -1,    81,    82,    62,    -1,    85,
      -1,    -1,    -1,    25,    -1,    -1,    -1,    -1,   504,    -1,
      -1,    -1,    77,    -1,    -1,    -1,   102,   103,   104,    -1,
     106,    -1,    44,    -1,    46,    -1,     5,   113,   114,    -1,
      95,    96,    97,    12,    13,   121,    -1,    -1,    -1,   125,
      62,    -1,    -1,    -1,   109,   110,   111,   112,   113,   114,
     115,   116,    -1,    -1,   119,    77,    -1,   122,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,     5,
      -1,    -1,    -1,    95,    96,    97,    12,    13,    -1,    -1,
      -1,    -1,    -1,    62,    -1,    -1,    -1,   109,   110,   111,
     112,   113,   114,   115,   116,    -1,    -1,   119,    77,     5,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    95,    96,    97,    98,
      -1,    -1,    -1,    -1,    30,    -1,    62,    -1,    -1,    -1,
     109,   110,   111,   112,   113,   114,   115,   116,    44,    -1,
     119,    77,     5,    -1,    80,    -1,    -1,    -1,    -1,    12,
      13,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,    95,
      96,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    -1,   109,   110,   111,   112,   113,   114,   115,
     116,    44,    -1,   119,    -1,     5,    -1,    -1,    -1,    95,
      96,    97,    12,    13,    -1,    -1,    -1,    -1,    -1,    62,
      -1,    -1,    -1,   109,   110,   111,   112,   113,   114,   115,
     116,    -1,    -1,   119,    77,     5,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    95,    96,    97,    98,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    -1,    -1,   109,   110,   111,   112,
     113,   114,   115,   116,    44,    -1,   119,    77,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    62,    -1,    -1,    95,    96,    97,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,
     110,   111,   112,   113,   114,   115,   116,    -1,    -1,   119,
      -1,    -1,    -1,    -1,    -1,    95,    96,    97,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     0,     1,    -1,     3,
     110,   111,   112,   113,   114,   115,   116,    11,    -1,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    -1,    -1,
      -1,    -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    -1,    43,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    65,    -1,    67,    -1,    69,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,
      84
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   128,     0,     1,     3,    27,    41,    43,    65,    67,
      69,    70,    83,    84,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   144,   147,   153,   154,
     155,   156,   157,   158,   159,    55,    60,   104,   120,   104,
     104,   104,   107,   225,    55,   107,   225,   160,   106,   220,
     164,   165,   164,   168,   169,    11,    21,    58,   141,   142,
     145,   148,   149,   168,   120,    44,    44,   168,    40,   120,
      38,   120,   120,   124,   125,    44,   120,   120,    32,   104,
     161,   226,   221,   222,   223,   224,   225,   106,   226,   226,
      56,   166,   167,   226,    55,   162,     1,     6,    10,    14,
      15,    16,    18,    20,    22,    34,    35,    36,    37,    38,
      39,    42,    48,    52,    53,    54,    57,    59,    60,    66,
      71,    72,    73,    74,    76,    78,    79,    81,    82,    85,
      87,    94,   102,   103,   104,   106,   113,   114,   121,   125,
     159,   170,   171,   172,   173,   175,   176,   182,   183,   184,
     185,   186,   187,   188,   189,   193,   195,   198,   199,   200,
     201,   202,   203,   204,   205,   207,   210,   211,   212,   215,
     218,   219,   222,   224,   232,   240,   104,   104,   104,   168,
      68,   168,   121,   150,   162,    27,   104,   104,   226,   107,
     104,   104,   224,   229,   230,   240,    27,   104,    20,    38,
      41,    42,    47,    55,    60,    74,    79,    83,    85,    87,
     104,   111,   222,   105,   108,   224,   107,   228,   221,   120,
     120,   104,   120,   120,   163,   226,   120,   240,   104,   177,
     121,   125,   121,   125,   168,   104,   222,   104,   222,   240,
     121,   125,   240,   240,   240,   220,   121,   125,   224,   121,
     121,   222,   105,   227,   121,   125,   121,   222,   121,   125,
     121,   222,   240,   224,   105,   233,   105,   121,   240,   240,
     240,   229,   120,     4,     7,     8,    49,    50,    51,    88,
      89,    90,    91,    92,   123,     5,    12,    13,    44,    62,
      77,    95,    96,    97,   109,   110,   111,   112,   113,   114,
     115,   116,   119,   241,   242,    68,   162,   104,   162,   168,
      86,   151,   226,    58,   120,    56,   179,   120,   120,   120,
     126,   108,    98,    58,   120,   105,   105,   224,   168,   164,
     120,   121,   229,   229,   229,   229,   226,   121,    44,    80,
     229,   229,   226,   229,   229,    93,   224,   222,    17,    63,
     213,   214,   229,   229,   222,    17,    63,   216,   217,   229,
     229,   222,    17,    63,   208,   209,    30,   240,   122,   126,
     121,   224,   104,   178,   104,   178,   178,   240,   240,    85,
     104,   188,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   104,   226,    28,    29,   234,   226,
     122,   121,    68,   120,   120,   120,   104,   120,   240,   240,
     120,   120,   226,   229,   122,   126,   122,   126,   222,   240,
     168,   122,   126,   122,   126,   224,   108,   122,   122,   229,
     123,   226,    17,    63,   122,   126,   122,   229,   123,   226,
      17,    63,   122,   126,   122,   229,   123,   226,    17,    63,
     168,   122,   222,   121,    61,   180,   180,   180,   121,   222,
     121,   234,   120,   103,   104,   116,   121,   235,   236,    26,
     143,   120,   168,   104,    98,   120,   122,   122,    25,    46,
     196,    31,   206,   103,   232,   213,   123,   168,   229,   123,
     216,   123,   168,   229,   123,   208,   123,   168,   229,   123,
     226,   122,   229,   104,   101,   181,   181,   222,   208,   190,
     222,   224,    92,   146,   236,   115,   116,   119,    35,    81,
     106,   113,   114,   239,   120,   122,   121,   152,   240,    19,
      23,    30,    33,   197,   168,   226,   122,   226,   168,   123,
     168,   226,   168,   123,   168,   226,   168,   123,   168,    86,
     122,   240,   122,   226,   120,   120,   123,    35,    81,   106,
     239,   228,   122,   236,   236,   237,   239,   103,   231,   232,
     168,   120,   168,   168,   168,   168,   222,   208,   191,   222,
     103,    45,   231,   120,   103,   121,   238,   122,   226,    49,
     226,   120,   192,   123,   122,   103,   104,   224,   122,    64,
      44,   194,   115,    61,   123,   224,   103,   104,    24,   122,
     101,   174,   121,   229,   122
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

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
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
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
    while (0)
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
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

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
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
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
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

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
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
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
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

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


  yyvsp[0] = yylval;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
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

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

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

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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
#line 430 "base\\generic\\compiler\\ascParse.y"
    {
	  ErrMsg_Generic("Error in definition.");
	;}
    break;

  case 18:
#line 437 "base\\generic\\compiler\\ascParse.y"
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
            } else {
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
	;}
    break;

  case 19:
#line 482 "base\\generic\\compiler\\ascParse.y"
    {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	;}
    break;

  case 20:
#line 486 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	;}
    break;

  case 21:
#line 491 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	;}
    break;

  case 22:
#line 499 "base\\generic\\compiler\\ascParse.y"
    {
          Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
        ;}
    break;

  case 23:
#line 503 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	;}
    break;

  case 24:
#line 508 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	;}
    break;

  case 25:
#line 516 "base\\generic\\compiler\\ascParse.y"
    {
	  (void)package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)));
	;}
    break;

  case 26:
#line 520 "base\\generic\\compiler\\ascParse.y"
    {
	  (void)package_load(SCP((yyvsp[-1].id_ptr)),NULL);
	;}
    break;

  case 27:
#line 527 "base\\generic\\compiler\\ascParse.y"
    {
	  /*  see comments for notes statement.  */
	  if( (yyvsp[-1].int_value) != NOTES_T ) {
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
	;}
    break;

  case 28:
#line 558 "base\\generic\\compiler\\ascParse.y"
    {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	;}
    break;

  case 29:
#line 566 "base\\generic\\compiler\\ascParse.y"
    {
	  if ((yyvsp[-3].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[-2].listp));
	  } else {
	    if( (yyvsp[-1].int_value) != METHODS_T ) {
	      WarnMsg_MismatchEnd("ADD METHODS", NULL, (yyvsp[-1].int_value), "METHODS");
	    }
	    if (AddMethods((yyvsp[-3].tptr),(yyvsp[-2].listp),g_untrapped_error) != 0) {
	      if ((yyvsp[-3].tptr) != ILLEGAL_DEFINITION) {
                ErrMsg_ProcsRejected("ADD",SCP(GetName((yyvsp[-3].tptr))));
	        DestroyProcedureList((yyvsp[-2].listp));
	      } /* else adding in DEFINITION MODEL may have misgone */
	    }
	  }
	  g_untrapped_error = 0;
	;}
    break;

  case 30:
#line 586 "base\\generic\\compiler\\ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-1].id_ptr));
	  if (tmptype == NULL) {
            ErrMsg_ProcTypeMissing("ADD", SCP((yyvsp[-1].id_ptr)));
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	  g_type_name = (yyvsp[-1].id_ptr); /* scope for notes */
	;}
    break;

  case 31:
#line 596 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	;}
    break;

  case 32:
#line 604 "base\\generic\\compiler\\ascParse.y"
    {
	  if ((yyvsp[-3].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[-2].listp));
	  } else {
	    if( (yyvsp[-1].int_value) != METHODS_T ) {
	      WarnMsg_MismatchEnd("REPLACE METHODS", NULL, (yyvsp[-1].int_value), "METHODS");
	    }
	    if (ReplaceMethods((yyvsp[-3].tptr),(yyvsp[-2].listp),g_untrapped_error) != 0) {
              ErrMsg_ProcsRejected("REPLACE",SCP(GetName((yyvsp[-3].tptr))));
	      DestroyProcedureList((yyvsp[-2].listp));
	    }
	  }
	  g_untrapped_error = 0;
	;}
    break;

  case 33:
#line 622 "base\\generic\\compiler\\ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-1].id_ptr));
	  if (tmptype == NULL) {
            ErrMsg_ProcTypeMissing("REPLACE", SCP((yyvsp[-1].id_ptr)));
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	;}
    break;

  case 34:
#line 631 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	;}
    break;

  case 35:
#line 639 "base\\generic\\compiler\\ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

          if(( (yyvsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != g_type_name )) {
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
	    error_reporter(ASC_USER_ERROR,Asc_ModuleBestName(Asc_CurrentModule()),g_header_linenum,NULL,
	            "Atom dimensions don't match in ATOM %s on line %s:%lu.\n",
	            SCP(g_type_name),
	            Asc_ModuleBestName(Asc_CurrentModule()),
	            g_header_linenum);
	    DestroyStatementList((yyvsp[-3].slptr));
	    DestroyProcedureList((yyvsp[-2].listp));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	;}
    break;

  case 36:
#line 692 "base\\generic\\compiler\\ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	;}
    break;

  case 37:
#line 703 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	;}
    break;

  case 38:
#line 711 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	;}
    break;

  case 39:
#line 717 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_defaulted = 1;
	;}
    break;

  case 40:
#line 722 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	;}
    break;

  case 41:
#line 729 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	;}
    break;

  case 42:
#line 736 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	;}
    break;

  case 43:
#line 746 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 44:
#line 786 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 45:
#line 819 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	;}
    break;

  case 46:
#line 825 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_defaulted = 1;
	;}
    break;

  case 47:
#line 830 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	;}
    break;

  case 48:
#line 837 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	;}
    break;

  case 49:
#line 844 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	;}
    break;

  case 50:
#line 854 "base\\generic\\compiler\\ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;
	  if(( (yyvsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != g_type_name )) {
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
	;}
    break;

  case 51:
#line 896 "base\\generic\\compiler\\ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	;}
    break;

  case 52:
#line 905 "base\\generic\\compiler\\ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	;}
    break;

  case 53:
#line 917 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	;}
    break;

  case 54:
#line 925 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = NULL;
	;}
    break;

  case 55:
#line 929 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	;}
    break;

  case 56:
#line 936 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = NULL;
	;}
    break;

  case 57:
#line 940 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	;}
    break;

  case 58:
#line 947 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = NULL;
	;}
    break;

  case 59:
#line 951 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	;}
    break;

  case 60:
#line 958 "base\\generic\\compiler\\ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  if (((yyvsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != g_type_name )) {
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
	;}
    break;

  case 61:
#line 990 "base\\generic\\compiler\\ascParse.y"
    {
	  /*
	   * A patch definition looks just like a model def.
	   * with the original name <=> refine name.
	   */
	  g_type_name = (yyvsp[-3].id_ptr);
	  g_refines_name = (yyvsp[-1].id_ptr);
	  g_header_linenum = LineNum();
	;}
    break;

  case 62:
#line 1003 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.int_value) = 0;
	;}
    break;

  case 63:
#line 1007 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.int_value) = 1;
	;}
    break;

  case 64:
#line 1014 "base\\generic\\compiler\\ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (yyvsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != (yyvsp[-4].id_ptr) )) {
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
	;}
    break;

  case 65:
#line 1044 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	;}
    break;

  case 66:
#line 1053 "base\\generic\\compiler\\ascParse.y"
    { /* nothing to do. just cruft to fix ; problem */ ;}
    break;

  case 67:
#line 1058 "base\\generic\\compiler\\ascParse.y"
    {
          struct UnitDefinition *ud;
          unsigned long c,len;

	  if( (yyvsp[0].int_value) != UNITS_T ) {
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
	;}
    break;

  case 68:
#line 1077 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.listp) = gl_create(100L);
	;}
    break;

  case 69:
#line 1081 "base\\generic\\compiler\\ascParse.y"
    {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	;}
    break;

  case 70:
#line 1089 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
                             LineNum());
	;}
    break;

  case 71:
#line 1098 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.listp) = NULL;
	;}
    break;

  case 72:
#line 1102 "base\\generic\\compiler\\ascParse.y"
    { /* To get rid of this, we will need a global proclist
           * that accumulates procs until a MODEL production is
           * completed. If any other sort of production is started,
           * and proclist is not NULL, it should be discarded.
           */
	;}
    break;

  case 73:
#line 1109 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.listp) = (yyvsp[0].listp);
	;}
    break;

  case 74:
#line 1116 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	;}
    break;

  case 75:
#line 1123 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	;}
    break;

  case 76:
#line 1127 "base\\generic\\compiler\\ascParse.y"
    {
	  unsigned long c;
	  struct InitProcedure *oldproc;
	  c = gl_length((yyvsp[-1].listp));
          while (c > 0) {
            oldproc = (struct InitProcedure *)gl_fetch((yyvsp[-1].listp),c);
            if (ProcName((yyvsp[0].procptr)) == ProcName(oldproc)) {
	      ErrMsg_DuplicateProc((yyvsp[0].procptr));
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
	;}
    break;

  case 77:
#line 1150 "base\\generic\\compiler\\ascParse.y"
    {
	  if (((yyvsp[-1].int_value) != IDENTIFIER_T) || ((yyvsp[-4].id_ptr) != g_end_identifier)) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("METHOD", SCP((yyvsp[-4].id_ptr)), (yyvsp[-1].int_value), SCP((yyvsp[-4].id_ptr)));
	  }
	  (yyval.procptr) = CreateProcedure((yyvsp[-4].id_ptr),(yyvsp[-2].slptr));
	  g_proc_name = NULL;
	;}
    break;

  case 78:
#line 1164 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	;}
    break;

  case 79:
#line 1173 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	;}
    break;

  case 80:
#line 1180 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.listp) = gl_create(7L);
	;}
    break;

  case 81:
#line 1184 "base\\generic\\compiler\\ascParse.y"
    {
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ((yyvsp[-1].statptr) != NULL) {
	    gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].statptr));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	;}
    break;

  case 82:
#line 1192 "base\\generic\\compiler\\ascParse.y"
    {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	;}
    break;

  case 113:
#line 1233 "base\\generic\\compiler\\ascParse.y"
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

	;}
    break;

  case 114:
#line 1274 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 115:
#line 1303 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	;}
    break;

  case 116:
#line 1308 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 117:
#line 1357 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = NULL;
	;}
    break;

  case 118:
#line 1361 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	;}
    break;

  case 119:
#line 1368 "base\\generic\\compiler\\ascParse.y"
    {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	;}
    break;

  case 120:
#line 1380 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 121:
#line 1407 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	;}
    break;

  case 122:
#line 1412 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	;}
    break;

  case 123:
#line 1420 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	;}
    break;

  case 124:
#line 1425 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	;}
    break;

  case 125:
#line 1433 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	;}
    break;

  case 126:
#line 1437 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	;}
    break;

  case 127:
#line 1444 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = NULL;
	;}
    break;

  case 128:
#line 1448 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	;}
    break;

  case 129:
#line 1455 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = NULL;
	;}
    break;

  case 130:
#line 1459 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[0].eptr);
	;}
    break;

  case 131:
#line 1466 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	;}
    break;

  case 132:
#line 1473 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	;}
    break;

  case 133:
#line 1480 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	;}
    break;

  case 134:
#line 1487 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	;}
    break;

  case 135:
#line 1494 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	;}
    break;

  case 136:
#line 1498 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	;}
    break;

  case 137:
#line 1505 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 138:
#line 1518 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 139:
#line 1535 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	;}
    break;

  case 140:
#line 1545 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	;}
    break;

  case 141:
#line 1553 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	;}
    break;

  case 142:
#line 1564 "base\\generic\\compiler\\ascParse.y"
    {
	  /*
	   * This is the blackbox declarative external relation.
	   */
	  struct VariableList *vl;
	  vl = JoinVariableLists((yyvsp[-4].lptr),(yyvsp[-2].lptr));
	  /* $$ = CreateEXTERN(2,$1,SCP($3),vl,$8,NULL); */
	  (yyval.statptr) = CreateEXTERNBlackBox((yyvsp[-8].nptr),SCP((yyvsp[-6].id_ptr)),vl,(yyvsp[-1].nptr));
	;}
    break;

  case 143:
#line 1577 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	;}
    break;

  case 144:
#line 1584 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	;}
    break;

  case 145:
#line 1591 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.nptr) = NULL;
	;}
    break;

  case 146:
#line 1595 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	;}
    break;

  case 147:
#line 1602 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 148:
#line 1622 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.nptr) = NULL;
	;}
    break;

  case 149:
#line 1626 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.nptr) = (yyvsp[0].nptr);
	;}
    break;

  case 150:
#line 1634 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != FOR_T ) {
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
	;}
    break;

  case 151:
#line 1654 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.order) = f_random;
	;}
    break;

  case 152:
#line 1658 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.order) = f_increasing;
	;}
    break;

  case 153:
#line 1662 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.order) = f_decreasing;
	;}
    break;

  case 154:
#line 1669 "base\\generic\\compiler\\ascParse.y"
    {
          (yyval.fkind) = fk_create; /* declarative FOR */
	;}
    break;

  case 155:
#line 1673 "base\\generic\\compiler\\ascParse.y"
    {
          (yyval.fkind) = fk_expect; /* parameter FOR */
	;}
    break;

  case 156:
#line 1677 "base\\generic\\compiler\\ascParse.y"
    {
          (yyval.fkind) = fk_check; /* WHERE FOR */
	;}
    break;

  case 157:
#line 1681 "base\\generic\\compiler\\ascParse.y"
    {
          (yyval.fkind) = fk_do; /* method FOR */
	;}
    break;

  case 158:
#line 1688 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	;}
    break;

  case 159:
#line 1692 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	;}
    break;

  case 160:
#line 1699 "base\\generic\\compiler\\ascParse.y"
    {
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	;}
    break;

  case 161:
#line 1706 "base\\generic\\compiler\\ascParse.y"
    {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	;}
    break;

  case 162:
#line 1713 "base\\generic\\compiler\\ascParse.y"
    {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	;}
    break;

  case 163:
#line 1724 "base\\generic\\compiler\\ascParse.y"
    {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
          g_callargs = NULL;
	;}
    break;

  case 164:
#line 1735 "base\\generic\\compiler\\ascParse.y"
    {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	;}
    break;

  case 165:
#line 1741 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != IF_T ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	;}
    break;

  case 166:
#line 1751 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != WHILE_T ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	;}
    break;

  case 167:
#line 1760 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = NULL;
	;}
    break;

  case 168:
#line 1764 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.slptr) = (yyvsp[0].slptr);
	;}
    break;

  case 169:
#line 1771 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[-1].wptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	;}
    break;

  case 170:
#line 1782 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[-1].wptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  DestroyName((yyvsp[-5].nptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	;}
    break;

  case 171:
#line 1794 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	;}
    break;

  case 172:
#line 1801 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	;}
    break;

  case 173:
#line 1811 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	;}
    break;

  case 174:
#line 1818 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.wptr) = CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr));
	;}
    break;

  case 175:
#line 1822 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	;}
    break;

  case 176:
#line 1826 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	;}
    break;

  case 177:
#line 1830 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	;}
    break;

  case 178:
#line 1837 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	;}
    break;

  case 179:
#line 1841 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	;}
    break;

  case 180:
#line 1845 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	;}
    break;

  case 181:
#line 1849 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	;}
    break;

  case 182:
#line 1853 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	;}
    break;

  case 183:
#line 1860 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	;}
    break;

  case 184:
#line 1867 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != SELECT_T ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SELECT statement.");
	  DestroySelectList((yyvsp[-1].septr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	;}
    break;

  case 185:
#line 1878 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != SELECT_T ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	;}
    break;

  case 186:
#line 1888 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	;}
    break;

  case 187:
#line 1895 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	;}
    break;

  case 188:
#line 1899 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	;}
    break;

  case 189:
#line 1903 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	;}
    break;

  case 190:
#line 1907 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	;}
    break;

  case 191:
#line 1914 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != SWITCH_T ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SWITCH statement.");
	  DestroySwitchList((yyvsp[-1].swptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	;}
    break;

  case 192:
#line 1925 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != SWITCH_T ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	;}
    break;

  case 193:
#line 1935 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	;}
    break;

  case 194:
#line 1942 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	;}
    break;

  case 195:
#line 1946 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	;}
    break;

  case 196:
#line 1950 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	;}
    break;

  case 197:
#line 1954 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	;}
    break;

  case 198:
#line 1961 "base\\generic\\compiler\\ascParse.y"
    {
	  if( (yyvsp[0].int_value) != CONDITIONAL_T ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	;}
    break;

  case 199:
#line 1971 "base\\generic\\compiler\\ascParse.y"
    {
	  /*  All processing of notes takes place on the notes_body here.
	   *  Notes should NOT be added to the statement list.
	   *  Here we know the current type and method names.
	   */
	  if( (yyvsp[0].int_value) != NOTES_T ) {
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
	;}
    break;

  case 200:
#line 2008 "base\\generic\\compiler\\ascParse.y"
    {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	;}
    break;

  case 201:
#line 2017 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 202:
#line 2032 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
                             (void *)(yyvsp[-1].lptr), LineNum());
	;}
    break;

  case 203:
#line 2037 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	;}
    break;

  case 204:
#line 2046 "base\\generic\\compiler\\ascParse.y"
    {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	;}
    break;

  case 205:
#line 2057 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	;}
    break;

  case 206:
#line 2061 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	;}
    break;

  case 207:
#line 2066 "base\\generic\\compiler\\ascParse.y"
    {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	;}
    break;

  case 208:
#line 2079 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 209:
#line 2099 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	;}
    break;

  case 210:
#line 2103 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	;}
    break;

  case 211:
#line 2108 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 212:
#line 2123 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = CONDITIONAL_T;
        ;}
    break;

  case 213:
#line 2128 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = FOR_T;
        ;}
    break;

  case 214:
#line 2133 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = IF_T;
        ;}
    break;

  case 215:
#line 2138 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = INTERACTIVE_T;
        ;}
    break;

  case 216:
#line 2143 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = METHODS_T;
        ;}
    break;

  case 217:
#line 2148 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = NOTES_T;
        ;}
    break;

  case 218:
#line 2153 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = SELECT_T;
        ;}
    break;

  case 219:
#line 2158 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = SWITCH_T;
        ;}
    break;

  case 220:
#line 2163 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = UNITS_T;
        ;}
    break;

  case 221:
#line 2168 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = GLOBAL_T;
        ;}
    break;

  case 222:
#line 2173 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = WHEN_T;
        ;}
    break;

  case 223:
#line 2178 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = WHILE_T;
        ;}
    break;

  case 224:
#line 2183 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = (yyvsp[0].id_ptr);
          (yyval.int_value) = IDENTIFIER_T;
        ;}
    break;

  case 225:
#line 2188 "base\\generic\\compiler\\ascParse.y"
    {
          g_end_identifier = NULL;
          (yyval.int_value) = END_T;
        ;}
    break;

  case 226:
#line 2196 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.braced_ptr) = NULL;
	;}
    break;

  case 227:
#line 2200 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	;}
    break;

  case 228:
#line 2207 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dquote_ptr) = NULL;
	;}
    break;

  case 229:
#line 2211 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	;}
    break;

  case 230:
#line 2218 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	;}
    break;

  case 231:
#line 2222 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = NULL;
	;}
    break;

  case 232:
#line 2229 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	;}
    break;

  case 233:
#line 2233 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 234:
#line 2237 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	;}
    break;

  case 235:
#line 2242 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	;}
    break;

  case 236:
#line 2250 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	;}
    break;

  case 237:
#line 2256 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	;}
    break;

  case 238:
#line 2265 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	;}
    break;

  case 239:
#line 2269 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 240:
#line 2290 "base\\generic\\compiler\\ascParse.y"
    {
	  g_dim_ptr = Dimensionless();
	  (yyval.real_value) = 1.0;
	;}
    break;

  case 241:
#line 2295 "base\\generic\\compiler\\ascParse.y"
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
	;}
    break;

  case 242:
#line 2316 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimp) = (yyvsp[0].dimp);
	;}
    break;

  case 243:
#line 2320 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimp) = Dimensionless();
	;}
    break;

  case 244:
#line 2324 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	;}
    break;

  case 245:
#line 2331 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimp) = WildDimension();
	;}
    break;

  case 246:
#line 2335 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	;}
    break;

  case 247:
#line 2342 "base\\generic\\compiler\\ascParse.y"
    {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	;}
    break;

  case 248:
#line 2346 "base\\generic\\compiler\\ascParse.y"
    {
	  ClearDimensions(&((yyval.dimen)));
	;}
    break;

  case 249:
#line 2350 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	;}
    break;

  case 250:
#line 2354 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	;}
    break;

  case 251:
#line 2358 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	;}
    break;

  case 252:
#line 2362 "base\\generic\\compiler\\ascParse.y"
    {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	;}
    break;

  case 253:
#line 2369 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	;}
    break;

  case 254:
#line 2376 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	;}
    break;

  case 255:
#line 2380 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	;}
    break;

  case 256:
#line 2387 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.int_value) = 0;
	;}
    break;

  case 257:
#line 2391 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.int_value) = 0;
	;}
    break;

  case 258:
#line 2395 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.int_value) = 1;
	;}
    break;

  case 259:
#line 2402 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	;}
    break;

  case 260:
#line 2406 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	;}
    break;

  case 261:
#line 2410 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	;}
    break;

  case 262:
#line 2414 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	;}
    break;

  case 263:
#line 2418 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateTrueExpr();
	;}
    break;

  case 264:
#line 2422 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateFalseExpr();
	;}
    break;

  case 265:
#line 2426 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateAnyExpr();
	;}
    break;

  case 266:
#line 2430 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	;}
    break;

  case 267:
#line 2434 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	;}
    break;

  case 268:
#line 2438 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	;}
    break;

  case 269:
#line 2442 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 270:
#line 2447 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 271:
#line 2452 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 272:
#line 2457 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 273:
#line 2462 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 274:
#line 2467 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 275:
#line 2472 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 276:
#line 2477 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	;}
    break;

  case 277:
#line 2481 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 278:
#line 2486 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 279:
#line 2491 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 280:
#line 2496 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 281:
#line 2501 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	;}
    break;

  case 282:
#line 2506 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[0].eptr);
	;}
    break;

  case 283:
#line 2510 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	;}
    break;

  case 284:
#line 2514 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	;}
    break;

  case 285:
#line 2518 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	;}
    break;

  case 286:
#line 2522 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	;}
    break;

  case 287:
#line 2529 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	;}
    break;

  case 288:
#line 2533 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	;}
    break;

  case 289:
#line 2540 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	;}
    break;

  case 290:
#line 2544 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	;}
    break;

  case 291:
#line 2551 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	;}
    break;

  case 292:
#line 2555 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	;}
    break;

  case 293:
#line 2562 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	;}
    break;

  case 294:
#line 2566 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	;}
    break;

  case 295:
#line 2573 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	;}
    break;

  case 296:
#line 2577 "base\\generic\\compiler\\ascParse.y"
    {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	;}
    break;

  case 297:
#line 2584 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	;}
    break;

  case 298:
#line 2588 "base\\generic\\compiler\\ascParse.y"
    {
	  CONST struct Func *fptr;
	  if ((fptr = LookupFunc(SCP((yyvsp[-3].id_ptr))))!=NULL) {
	    (yyval.eptr) = JoinExprLists((yyvsp[-1].eptr),CreateFuncExpr(fptr));
	  } else {
	    (yyval.eptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Function '%s' is not defined.",SCP((yyvsp[-3].id_ptr)));
	    g_untrapped_error++;
	  }
	;}
    break;

  case 299:
#line 2599 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	;}
    break;

  case 300:
#line 2606 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	;}
    break;

  case 301:
#line 2610 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_less);
	;}
    break;

  case 302:
#line 2614 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	;}
    break;

  case 303:
#line 2618 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	;}
    break;

  case 304:
#line 2622 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	;}
    break;

  case 305:
#line 2626 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	;}
    break;

  case 306:
#line 2633 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	;}
    break;

  case 307:
#line 2637 "base\\generic\\compiler\\ascParse.y"
    {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	;}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 4865 "base\\generic\\compiler\\ascParse.c"

  yyvsp -= yylen;
  yyssp -= yylen;


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
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {

		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 yydestruct ("Error: popping",
                             yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
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

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

yyvsp -= yylen;
  yyssp -= yylen;
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
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
  yydestruct ("Error: discarding lookahead",
              yytoken, &yylval);
  yychar = YYEMPTY;
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 2641 "base\\generic\\compiler\\ascParse.y"

/*
 * We really need to do something about freeing up the productions
 * that invoke this so we don't leak memory like a seive.
 * for example  z[i IN [1..2]][j IN [process[i]] IS_A mass; eats a ton.
 */
int
yyerror(char *s)
{
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


/*
 *  void ErrMsg_*(void)
 *
 *  The following print error and warning messages to the filehandles
 *  ASCERR and ASCWARN, respectively.
 *  The type of error/warning that will be printed is indicated by the
 *  functions name and the arguments to fprintf.
 */
static void
ErrMsg_Generic(CONST char *string)
{
  /* the module may have be already closed, Asc_CurrentModule will be null */
  error_reporter_current_line(ASC_USER_ERROR,"%s",string);

  if (g_type_name != NULL) {
    error_reporter_current_line(ASC_USER_ERROR,"    type %s\n",SCP(g_type_name));
  }
  if (g_proc_name != NULL) {
    error_reporter_current_line(ASC_USER_ERROR,"    METHOD %s\n",SCP(g_proc_name));
  }
}

static void ErrMsg_CommaName(CONST char *what, struct Name *name)
{
  struct module_t *mod;

  /* the module may have be already closed */
  mod = Asc_CurrentModule();

  error_reporter_current_line(ASC_USER_ERROR, "ASC-Error: Missing comma or operator before %s ",what);
  WriteName(ASCERR,name);
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
ErrMsg_ProcTypeMissing(CONST char *AorR, CONST char *type)
{
  error_reporter_current_line(ASC_USER_ERROR,
	  "%s METHODS called with undefined type (%s)", AorR, type);
}

static void
ErrMsg_ProcsRejected(CONST char *AorR, CONST char *type)
{
  error_reporter_current_line(ASC_USER_ERROR,
	  "%s METHODS failed for type %s", AorR, type);
}

static void
ErrMsg_DuplicateProc(struct InitProcedure *p)
{
  error_reporter_current_line(ASC_USER_WARNING,
	  "Duplicate METHOD %s rejected", SCP(ProcName(p)));
}

static void
ErrMsg_ParensBrackets(CONST char *operation)
{
  error_reporter_current_line(ASC_USER_ERROR,
          "  You should be using %s[] not %s()",
          operation,
          operation);
}


/*
 *  WarnMsg_MismatchEnd(statement, opt_name, end_token, expecting);
 *      const char *statement;
 *      const char *opt_name;
 *      unsigned long end_token;
 *      const char *expecting;
 *
 *  Print a warning message that the token after the END keyword did not
 *  match what we were expecting for the current statement.
 *  Arguments:
 *      statement --the current statement, e.g. ATOM, METHOD, FOR, IF, CASE
 *      opt_name  --the name of the thing we were defining for ATOMs, METHODs,
 *                  etc, or NULL anonymous statements (FOR, IF, CASE, etc)
 *      end_token --the TOKEN_T that we were received instead.  We use the
 *                  TokenAsString to produce a string given a TOKEN_T
 *      expecting --the keyword we were expecting to see after the END; if
 *                  NULL, we were expecting the string given in statement
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
 *  CONST char *TokenAsString(token);
 *      unsigned long token;
 *
 *  Takes a TOKEN_T (e.g., FOR_T, MODEL_T, END_T, IDENTIFIER_T) and returns
 *  a string representation of it:
 *      e.g.:  TokenAsString(FOR_T) ==> "FOR"
 *
 *  Since this function is only used inside WarnMsg_MismatchEnd, we do a
 *  couple of things specific to that function:  If token is END_T, we
 *  return an empty string, and if it is IDENTIFIER_T, we return the
 *  current value of g_end_identifier, or UNKNOWN if g_end_identifier is
 *  NULL.
 */
static CONST char *
TokenAsString(unsigned long token)
{
  switch( token ) {
  case ATOM_T:
    return "ATOM";
  case CONDITIONAL_T:
    return "CONDITIONAL";
  case FOR_T:
    return "FOR";
  case ASSERT_T:
	return "ASSERT";
  case IF_T:
    return "IF";
  case INTERACTIVE_T:
    return "INTERACTIVE";
  case METHOD_T:
    return "METHOD";
  case METHODS_T:
    return "METHODS";
  case MODEL_T:
    return "MODEL";
  case NOTES_T:
    return "NOTES";
  case PATCH_T:
    return "PATCH";
  case SELECT_T:
    return "SELECT";
  case SWITCH_T:
    return "SWITCH";
  case UNITS_T:
    return "UNITS";
  case WHEN_T:
    return "WHEN";
  case END_T:
    return "";
  case IDENTIFIER_T:
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

