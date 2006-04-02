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

/* All symbols defined below should begin with zz_ or ZZ_, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define ZZ_BISON 1

/* Skeleton name.  */
#define ZZ_SKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define ZZ_PURE 0

/* Using locations.  */
#define ZZ_LSP_NEEDED 0



/* Tokens.  */
#ifndef ZZ_TOKENTYPE
# define ZZ_TOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum zz_tokentype {
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
#line 1 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"

/*
 *  Ascend Grammar file
 *  by Tom Epperly
 *  Version: $Revision: 1.23 $
 *  Version control file: $RCSfile: ascParse.y,v $
 *  Date last modified: $Date: 2000/01/25 02:25:59 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1997 Benjamin Andrew Allan & Vicente Rico-Ramirez
 *  Copyright (C) 1998 Carnegie Mellon University
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
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>		/* need DBL_MAX and LONG_MAX */
#include <float.h>		/* on a NeXT they are in here */

#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "general/list.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/scanner.h"
#include "compiler/symtab.h"		/* the global string/symbol table */
#include "compiler/notate.h"		/* notes database wrapper */
#include "compiler/braced.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/func.h"
#include "compiler/types.h"
#include "compiler/name.h"
#include "compiler/nameio.h"
#include "compiler/instance_enum.h"
#include "compiler/extfunc.h"
#include "compiler/packages.h"
#include "compiler/sets.h"
#include "compiler/exprs.h"
#include "compiler/exprio.h"
#include "compiler/vlist.h"
#include "compiler/vlistio.h"		/* for debugging only */
#include "compiler/stattypes.h"
#include "compiler/slist.h"
#include "compiler/statement.h"
#include "compiler/statio.h"
#include "compiler/units.h"
#include "compiler/when.h"
#include "compiler/select.h"
#include "compiler/switch.h"
#include "compiler/proc.h"
#include "compiler/watchpt.h"
#include "compiler/module.h"
#include "compiler/child.h"
#include "compiler/type_desc.h"
#include "compiler/type_descio.h"
#include "compiler/typedef.h"
#include "compiler/library.h"
#include "compiler/syntax.h"
#include "compiler/lexer.h"
/* 1 ==> expr can find missing , w/o  shift/reduce conflicts */
#define COMMAEXPR_NOTBUGGY 0 
#if COMMAEXPR_NOTBUGGY
#include "compiler/exprio.h"
#endif /* for CommaExpr if working. */

#ifndef lint
static CONST char ParserID[] = "$Id: ascParse.y,v 1.23 2000/01/25 02:25:59 ballan Exp $";
#endif

int g_compiler_warnings = 1;		/* level of whine to allow */

#include "compiler/redirectFile.h"
#ifndef ASCERR
#define ASCERR g_ascend_errors
#define ASCWAR g_ascend_warnings
#define ASCINF g_ascend_information
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
#ifndef ZZ_DEBUG
# define ZZ_DEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef ZZ_ERROR_VERBOSE
# undef ZZ_ERROR_VERBOSE
# define ZZ_ERROR_VERBOSE 1
#else
# define ZZ_ERROR_VERBOSE 0
#endif

#if ! defined (ZZ_STYPE) && ! defined (ZZ_STYPE_IS_DECLARED)
#line 300 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
typedef union ZZ_STYPE {
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
} ZZ_STYPE;
/* Line 190 of yacc.c.  */
#line 621 "y.tab.c"
# define zz_stype ZZ_STYPE /* obsolescent; will be withdrawn */
# define ZZ_STYPE_IS_DECLARED 1
# define ZZ_STYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 633 "y.tab.c"

#if ! defined (zz_overflow) || ZZ_ERROR_VERBOSE

# ifndef ZZ_FREE
#  define ZZ_FREE free
# endif
# ifndef ZZ_MALLOC
#  define ZZ_MALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef ZZ_STACK_USE_ALLOCA
#  if ZZ_STACK_USE_ALLOCA
#   ifdef __GNUC__
#    define ZZ_STACK_ALLOC __builtin_alloca
#   else
#    define ZZ_STACK_ALLOC alloca
#   endif
#  endif
# endif

# ifdef ZZ_STACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define ZZ_STACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define ZZ_SIZE_T size_t
#  endif
#  define ZZ_STACK_ALLOC ZZ_MALLOC
#  define ZZ_STACK_FREE ZZ_FREE
# endif
#endif /* ! defined (zz_overflow) || ZZ_ERROR_VERBOSE */


#if (! defined (zz_overflow) \
     && (! defined (__cplusplus) \
	 || (defined (ZZ_STYPE_IS_TRIVIAL) && ZZ_STYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union zz_alloc
{
  short int zz_ss;
  ZZ_STYPE zz_vs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define ZZ_STACK_GAP_MAXIMUM (sizeof (union zz_alloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define ZZ_STACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (ZZ_STYPE))			\
      + ZZ_STACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef ZZ_COPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define ZZ_COPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define ZZ_COPY(To, From, Count)		\
      do					\
	{					\
	  register ZZ_SIZE_T zz_i;		\
	  for (zz_i = 0; zz_i < (Count); zz_i++)	\
	    (To)[zz_i] = (From)[zz_i];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables ZZ_SIZE and ZZ_STACKSIZE give the old and new number of
   elements in the stack, and ZZ_PTR gives the new location of the
   stack.  Advance ZZ_PTR to a properly aligned location for the next
   stack.  */
# define ZZ_STACK_RELOCATE(Stack)					\
    do									\
      {									\
	ZZ_SIZE_T zz_newbytes;						\
	ZZ_COPY (&zz_ptr->Stack, Stack, zz_size);				\
	Stack = &zz_ptr->Stack;						\
	zz_newbytes = zz_stacksize * sizeof (*Stack) + ZZ_STACK_GAP_MAXIMUM; \
	zz_ptr += zz_newbytes / sizeof (*zz_ptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char zz_signed_char;
#else
   typedef short int zz_signed_char;
#endif

/* ZZ_FINAL -- State number of the termination state. */
#define ZZ_FINAL  2
/* ZZ_LAST -- Last index in ZZ_TABLE.  */
#define ZZ_LAST   1180

/* ZZ_NTOKENS -- Number of terminals. */
#define ZZ_NTOKENS  127
/* ZZ_NNTS -- Number of nonterminals. */
#define ZZ_NNTS  116
/* ZZ_NRULES -- Number of rules. */
#define ZZ_NRULES  307
/* ZZ_NRULES -- Number of states. */
#define ZZ_NSTATES  615

/* ZZ_TRANSLATE(ZZ_LEX) -- Bison symbol number corresponding to ZZ_LEX.  */
#define ZZ_UNDEFTOK  2
#define ZZ_MAXUTOK   364

#define ZZ_TRANSLATE(ZZ_X) 						\
  ((unsigned int) (ZZ_X) <= ZZ_MAXUTOK ? zz_translate[ZZ_X] : ZZ_UNDEFTOK)

/* ZZ_TRANSLATE[ZZ_LEX] -- Bison symbol number corresponding to ZZ_LEX.  */
static const unsigned char zz_translate[] =
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

#if ZZ_DEBUG
/* ZZ_PRHS[ZZ_N] -- Index of the first RHS symbol of rule number ZZ_N in
   ZZ_RHS.  */
static const unsigned short int zz_prhs[] =
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

/* ZZ_RHS -- A `-1'-separated list of the rules' RHS. */
static const short int zz_rhs[] =
{
     128,     0,    -1,    -1,   128,   129,    -1,   131,    -1,   132,
      -1,   133,    -1,   136,    -1,   138,    -1,   134,    -1,   144,
      -1,   140,    -1,   147,    -1,   156,    -1,   153,    -1,   158,
      -1,   130,    -1,     1,    -1,    41,   120,   168,   226,   120,
      -1,    70,   107,   120,    -1,    70,   225,   120,    -1,    70,
     225,    -1,    67,   107,   120,    -1,    67,   225,   120,    -1,
      67,   225,    -1,    43,   104,    40,   104,   120,    -1,    43,
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

/* ZZ_RLINE[ZZ_N] -- source line where rule number ZZ_N was defined.  */
static const unsigned short int zz_rline[] =
{
       0,   409,   409,   411,   415,   416,   417,   418,   419,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   435,   480,
     484,   489,   497,   501,   506,   514,   518,   525,   556,   564,
     584,   594,   602,   620,   629,   637,   690,   701,   710,   715,
     720,   727,   734,   744,   783,   818,   823,   828,   835,   842,
     852,   893,   902,   915,   924,   927,   935,   938,   946,   949,
     956,   988,  1002,  1005,  1012,  1042,  1051,  1056,  1076,  1079,
    1087,  1097,  1101,  1100,  1114,  1122,  1125,  1148,  1162,  1171,
    1179,  1182,  1190,  1198,  1199,  1200,  1201,  1202,  1203,  1204,
    1205,  1206,  1207,  1208,  1209,  1210,  1211,  1212,  1213,  1214,
    1215,  1216,  1217,  1218,  1219,  1220,  1221,  1222,  1223,  1224,
    1225,  1226,  1227,  1231,  1272,  1301,  1305,  1356,  1359,  1366,
    1378,  1405,  1410,  1418,  1423,  1432,  1435,  1443,  1446,  1454,
    1457,  1464,  1471,  1478,  1485,  1492,  1496,  1503,  1516,  1533,
    1543,  1551,  1562,  1574,  1581,  1589,  1592,  1599,  1619,  1622,
    1629,  1651,  1654,  1658,  1665,  1669,  1673,  1677,  1684,  1688,
    1695,  1702,  1709,  1719,  1730,  1736,  1746,  1756,  1759,  1766,
    1777,  1789,  1796,  1806,  1813,  1817,  1821,  1825,  1832,  1836,
    1840,  1844,  1848,  1855,  1862,  1873,  1883,  1890,  1894,  1898,
    1902,  1909,  1920,  1930,  1937,  1941,  1945,  1949,  1956,  1966,
    2003,  2012,  2027,  2032,  2041,  2052,  2056,  2061,  2074,  2094,
    2098,  2103,  2118,  2123,  2128,  2133,  2138,  2143,  2148,  2153,
    2158,  2163,  2168,  2173,  2178,  2183,  2192,  2195,  2203,  2206,
    2213,  2218,  2224,  2228,  2232,  2237,  2245,  2251,  2260,  2264,
    2286,  2290,  2311,  2315,  2320,  2326,  2330,  2337,  2341,  2345,
    2349,  2353,  2357,  2364,  2371,  2375,  2383,  2386,  2390,  2397,
    2401,  2405,  2409,  2413,  2417,  2421,  2425,  2429,  2433,  2437,
    2442,  2447,  2452,  2457,  2462,  2467,  2472,  2476,  2481,  2486,
    2491,  2496,  2501,  2505,  2509,  2513,  2517,  2524,  2528,  2535,
    2539,  2546,  2550,  2557,  2561,  2568,  2572,  2579,  2583,  2594,
    2601,  2605,  2609,  2613,  2617,  2621,  2628,  2632
};
#endif

#if ZZ_DEBUG || ZZ_ERROR_VERBOSE
/* ZZ_TNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at ZZ_NTOKENS, nonterminals. */
static const char *const zz_tname[] =
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

# ifdef ZZ_PRINT
/* ZZ_TOKNUM[ZZ_LEX-NUM] -- Internal token number corresponding to
   token ZZ_LEX-NUM.  */
static const unsigned short int zz_toknum[] =
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

/* ZZ_R1[ZZ_N] -- Symbol number of symbol that rule ZZ_N derives.  */
static const unsigned char zz_r1[] =
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

/* ZZ_R2[ZZ_N] -- Number of symbols composing right hand side of rule ZZ_N.  */
static const unsigned char zz_r2[] =
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

/* ZZ_DEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when ZZ_TABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int zz_defact[] =
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

/* ZZ_DEFGOTO[NTERM-NUM]. */
static const short int zz_defgoto[] =
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

/* ZZ_PACT[STATE-NUM] -- Index in ZZ_TABLE of the portion describing
   STATE-NUM.  */
#define ZZ_PACT_NINF -437
static const short int zz_pact[] =
{
    -437,  1096,  -437,  -437,    81,   -59,     2,    25,    38,    57,
       1,    82,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
      71,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
      10,  -437,  -437,  -437,    77,   167,   179,  -437,  -437,   -30,
     202,  -437,   130,    43,   204,   133,    60,   -18,   150,   -19,
     223,   206,   223,   203,   593,   156,   159,   160,  -437,   197,
    -437,  -437,   145,   203,  -437,   -11,   166,   223,   168,  -437,
     169,  -437,  -437,   170,   165,   -10,  -437,  -437,   476,   164,
    -437,  -437,   150,   171,   -61,  -437,     0,   150,   157,   162,
     176,  -437,   163,   172,  -437,   223,   173,  -437,   165,  -437,
     180,    33,    44,  -437,  -437,   181,  -437,  -437,   150,   183,
     150,   165,    89,   165,  -437,  -437,   165,   165,    71,    96,
    -437,   150,   174,   -72,   186,   105,   -64,  -437,   108,   -62,
     165,   150,   189,   191,   177,  -437,   165,   165,   165,   165,
    -437,   182,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,  -437,
    -437,  -437,    59,    84,  -437,   960,  -437,   231,  -437,   203,
     196,   203,  -437,   215,   223,   247,   193,   251,   194,   195,
     198,  -437,  -437,   185,   208,   801,   261,   207,  -437,  -437,
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

/* ZZ_PGOTO[NTERM-NUM].  */
static const short int zz_pgoto[] =
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

/* ZZ_TABLE[ZZ_PACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what ZZ_DEFACT says.
   If ZZ_TABLE_NINF, syntax error.  */
#define ZZ_TABLE_NINF -206
static const short int zz_table[] =
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
     187,   135,   189,   190,   191,   211,   213,   219,   136,   137,
     221,   412,   220,   222,   228,   235,   138,   237,   288,    85,
     139,   252,   223,   226,   264,   249,   266,   425,   267,   305,
     307,   310,   272,   431,   488,   313,   289,   315,   420,   418,
     439,   320,   493,   314,   317,   318,   321,   447,   319,   323,
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

static const short int zz_check[] =
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
     104,   106,   104,   104,   104,   111,   105,   120,   113,   114,
     104,   328,   120,   120,   104,   104,   121,   104,    44,   337,
     125,   105,   120,   120,   105,   121,   105,   345,   121,    68,
     104,    86,   120,   350,   432,    58,    62,    56,   339,   337,
     357,   126,   440,   120,   120,   120,   108,   364,   120,    58,
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

/* ZZ_STOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char zz_stos[] =
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
      68,   168,   121,   150,   162,    27,   104,   104,   226,   104,
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

#if ! defined (ZZ_SIZE_T) && defined (__SIZE_TYPE__)
# define ZZ_SIZE_T __SIZE_TYPE__
#endif
#if ! defined (ZZ_SIZE_T) && defined (size_t)
# define ZZ_SIZE_T size_t
#endif
#if ! defined (ZZ_SIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define ZZ_SIZE_T size_t
# endif
#endif
#if ! defined (ZZ_SIZE_T)
# define ZZ_SIZE_T unsigned int
#endif

#define zz_errok		(zz_errstatus = 0)
#define zz_clearin	(zz_char = ZZ_EMPTY)
#define ZZ_EMPTY		(-2)
#define ZZ_EOF		0

#define ZZ_ACCEPT	goto zz_acceptlab
#define ZZ_ABORT		goto zz_abortlab
#define ZZ_ERROR		goto zz_errorlab


/* Like ZZ_ERROR except do call zz_error.  This remains here temporarily
   to ease the transition to the new meaning of ZZ_ERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define ZZ_FAIL		goto zz_errlab

#define ZZ_RECOVERING()  (!!zz_errstatus)

#define ZZ_BACKUP(Token, Value)					\
do								\
  if (zz_char == ZZ_EMPTY && zz_len == 1)				\
    {								\
      zz_char = (Token);						\
      zz_lval = (Value);						\
      zz_token = ZZ_TRANSLATE (zz_char);				\
      ZZ_POPSTACK;						\
      goto zz_backup;						\
    }								\
  else								\
    { 								\
      zz_error ("syntax error: cannot back up");\
      ZZ_ERROR;							\
    }								\
while (0)


#define ZZ_TERROR	1
#define ZZ_ERRCODE	256


/* ZZ_LLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define ZZ_RHSLOC(Rhs, K) ((Rhs)[K])
#ifndef ZZ_LLOC_DEFAULT
# define ZZ_LLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = ZZ_RHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = ZZ_RHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = ZZ_RHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = ZZ_RHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    ZZ_RHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    ZZ_RHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* ZZ__LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef ZZ__LOCATION_PRINT
# if ZZ_LTYPE_IS_TRIVIAL
#  define ZZ__LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define ZZ__LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* ZZ_LEX -- calling `zz_lex' with the right arguments.  */

#ifdef ZZ_LEX_PARAM
# define ZZ_LEX zz_lex (ZZ_LEX_PARAM)
#else
# define ZZ_LEX zz_lex ()
#endif

/* Enable debugging if requested.  */
#if ZZ_DEBUG

# ifndef ZZ_FPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define ZZ_FPRINTF fprintf
# endif

# define ZZ_DPRINTF(Args)			\
do {						\
  if (zz_debug)					\
    ZZ_FPRINTF Args;				\
} while (0)

# define ZZ__SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (zz_debug)							\
    {								\
      ZZ_FPRINTF (stderr, "%s ", Title);				\
      zz_symprint (stderr, 					\
                  Type, Value);	\
      ZZ_FPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| zz__stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
zz__stack_print (short int *bottom, short int *top)
#else
static void
zz__stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  ZZ_FPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    ZZ_FPRINTF (stderr, " %d", *bottom);
  ZZ_FPRINTF (stderr, "\n");
}

# define ZZ__STACK_PRINT(Bottom, Top)				\
do {								\
  if (zz_debug)							\
    zz__stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the ZZ_RULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
zz__reduce_print (int zz_rule)
#else
static void
zz__reduce_print (zz_rule)
    int zz_rule;
#endif
{
  int zz_i;
  unsigned int zz_lno = zz_rline[zz_rule];
  ZZ_FPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             zz_rule - 1, zz_lno);
  /* Print the symbols being reduced, and their result.  */
  for (zz_i = zz_prhs[zz_rule]; 0 <= zz_rhs[zz_i]; zz_i++)
    ZZ_FPRINTF (stderr, "%s ", zz_tname [zz_rhs[zz_i]]);
  ZZ_FPRINTF (stderr, "-> %s\n", zz_tname [zz_r1[zz_rule]]);
}

# define ZZ__REDUCE_PRINT(Rule)		\
do {					\
  if (zz_debug)				\
    zz__reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int zz_debug;
#else /* !ZZ_DEBUG */
# define ZZ_DPRINTF(Args)
# define ZZ__SYMBOL_PRINT(Title, Type, Value, Location)
# define ZZ__STACK_PRINT(Bottom, Top)
# define ZZ__REDUCE_PRINT(Rule)
#endif /* !ZZ_DEBUG */


/* ZZ_INITDEPTH -- initial size of the parser's stacks.  */
#ifndef	ZZ_INITDEPTH
# define ZZ_INITDEPTH 200
#endif

/* ZZ_MAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < ZZ_STACK_BYTES (ZZ_MAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef ZZ_MAXDEPTH
# define ZZ_MAXDEPTH 10000
#endif



#if ZZ_ERROR_VERBOSE

# ifndef zz_strlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define zz_strlen strlen
#  else
/* Return the length of ZZ_STR.  */
static ZZ_SIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
zz_strlen (const char *zz_str)
#   else
zz_strlen (zz_str)
     const char *zz_str;
#   endif
{
  register const char *zz_s = zz_str;

  while (*zz_s++ != '\0')
    continue;

  return zz_s - zz_str - 1;
}
#  endif
# endif

# ifndef zz_stpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define zz_stpcpy stpcpy
#  else
/* Copy ZZ_SRC to ZZ_DEST, returning the address of the terminating '\0' in
   ZZ_DEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
zz_stpcpy (char *zz_dest, const char *zz_src)
#   else
zz_stpcpy (zz_dest, zz_src)
     char *zz_dest;
     const char *zz_src;
#   endif
{
  register char *zz_d = zz_dest;
  register const char *zz_s = zz_src;

  while ((*zz_d++ = *zz_s++) != '\0')
    continue;

  return zz_d - 1;
}
#  endif
# endif

#endif /* !ZZ_ERROR_VERBOSE */



#if ZZ_DEBUG
/*--------------------------------.
| Print this symbol on ZZ_OUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
zz_symprint (FILE *zz_output, int zz_type, ZZ_STYPE *zz_valuep)
#else
static void
zz_symprint (zz_output, zz_type, zz_valuep)
    FILE *zz_output;
    int zz_type;
    ZZ_STYPE *zz_valuep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) zz_valuep;

  if (zz_type < ZZ_NTOKENS)
    ZZ_FPRINTF (zz_output, "token %s (", zz_tname[zz_type]);
  else
    ZZ_FPRINTF (zz_output, "nterm %s (", zz_tname[zz_type]);


# ifdef ZZ_PRINT
  if (zz_type < ZZ_NTOKENS)
    ZZ_PRINT (zz_output, zz_toknum[zz_type], *zz_valuep);
# endif
  switch (zz_type)
    {
      default:
        break;
    }
  ZZ_FPRINTF (zz_output, ")");
}

#endif /* ! ZZ_DEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
zz_destruct (const char *zz_msg, int zz_type, ZZ_STYPE *zz_valuep)
#else
static void
zz_destruct (zz_msg, zz_type, zz_valuep)
    const char *zz_msg;
    int zz_type;
    ZZ_STYPE *zz_valuep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) zz_valuep;

  if (!zz_msg)
    zz_msg = "Deleting";
  ZZ__SYMBOL_PRINT (zz_msg, zz_type, zz_valuep, zz_locationp);

  switch (zz_type)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef ZZ_PARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int zz_parse (void *ZZ_PARSE_PARAM);
# else
int zz_parse ();
# endif
#else /* ! ZZ_PARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int zz_parse (void);
#else
int zz_parse ();
#endif
#endif /* ! ZZ_PARSE_PARAM */



/* The look-ahead symbol.  */
int zz_char;

/* The semantic value of the look-ahead symbol.  */
ZZ_STYPE zz_lval;

/* Number of syntax errors so far.  */
int zz_nerrs;



/*----------.
| zz_parse.  |
`----------*/

#ifdef ZZ_PARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int zz_parse (void *ZZ_PARSE_PARAM)
# else
int zz_parse (ZZ_PARSE_PARAM)
  void *ZZ_PARSE_PARAM;
# endif
#else /* ! ZZ_PARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
zz_parse (void)
#else
int
zz_parse ()

#endif
#endif
{
  
  register int zz_state;
  register int zz_n;
  int zz_result;
  /* Number of tokens to shift before error messages enabled.  */
  int zz_errstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int zz_token = 0;

  /* Three stacks and their tools:
     `zz_ss': related to states,
     `zz_vs': related to semantic values,
     `zz_ls': related to locations.

     Refer to the stacks thru separate pointers, to allow zz_overflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int zz_ssa[ZZ_INITDEPTH];
  short int *zz_ss = zz_ssa;
  register short int *zz_ssp;

  /* The semantic value stack.  */
  ZZ_STYPE zz_vsa[ZZ_INITDEPTH];
  ZZ_STYPE *zz_vs = zz_vsa;
  register ZZ_STYPE *zz_vsp;



#define ZZ_POPSTACK   (zz_vsp--, zz_ssp--)

  ZZ_SIZE_T zz_stacksize = ZZ_INITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  ZZ_STYPE zz_val;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int zz_len;

  ZZ_DPRINTF ((stderr, "Starting parse\n"));

  zz_state = 0;
  zz_errstatus = 0;
  zz_nerrs = 0;
  zz_char = ZZ_EMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  zz_ssp = zz_ss;
  zz_vsp = zz_vs;


  zz_vsp[0] = zz_lval;

  goto zz_setstate;

/*------------------------------------------------------------.
| zz_newstate -- Push a new state, which is found in zz_state.  |
`------------------------------------------------------------*/
 zz_newstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  zz_ssp++;

 zz_setstate:
  *zz_ssp = zz_state;

  if (zz_ss + zz_stacksize - 1 <= zz_ssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      ZZ_SIZE_T zz_size = zz_ssp - zz_ss + 1;

#ifdef zz_overflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	ZZ_STYPE *zz_vs1 = zz_vs;
	short int *zz_ss1 = zz_ss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if zz_overflow is a macro.  */
	zz_overflow ("parser stack overflow",
		    &zz_ss1, zz_size * sizeof (*zz_ssp),
		    &zz_vs1, zz_size * sizeof (*zz_vsp),

		    &zz_stacksize);

	zz_ss = zz_ss1;
	zz_vs = zz_vs1;
      }
#else /* no zz_overflow */
# ifndef ZZ_STACK_RELOCATE
      goto zz_overflowlab;
# else
      /* Extend the stack our own way.  */
      if (ZZ_MAXDEPTH <= zz_stacksize)
	goto zz_overflowlab;
      zz_stacksize *= 2;
      if (ZZ_MAXDEPTH < zz_stacksize)
	zz_stacksize = ZZ_MAXDEPTH;

      {
	short int *zz_ss1 = zz_ss;
	union zz_alloc *zz_ptr =
	  (union zz_alloc *) ZZ_STACK_ALLOC (ZZ_STACK_BYTES (zz_stacksize));
	if (! zz_ptr)
	  goto zz_overflowlab;
	ZZ_STACK_RELOCATE (zz_ss);
	ZZ_STACK_RELOCATE (zz_vs);

#  undef ZZ_STACK_RELOCATE
	if (zz_ss1 != zz_ssa)
	  ZZ_STACK_FREE (zz_ss1);
      }
# endif
#endif /* no zz_overflow */

      zz_ssp = zz_ss + zz_size - 1;
      zz_vsp = zz_vs + zz_size - 1;


      ZZ_DPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) zz_stacksize));

      if (zz_ss + zz_stacksize - 1 <= zz_ssp)
	ZZ_ABORT;
    }

  ZZ_DPRINTF ((stderr, "Entering state %d\n", zz_state));

  goto zz_backup;

/*-----------.
| zz_backup.  |
`-----------*/
zz_backup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* zz_resume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  zz_n = zz_pact[zz_state];
  if (zz_n == ZZ_PACT_NINF)
    goto zz_default;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* ZZ_CHAR is either ZZ_EMPTY or ZZ_EOF or a valid look-ahead symbol.  */
  if (zz_char == ZZ_EMPTY)
    {
      ZZ_DPRINTF ((stderr, "Reading a token: "));
      zz_char = ZZ_LEX;
    }

  if (zz_char <= ZZ_EOF)
    {
      zz_char = zz_token = ZZ_EOF;
      ZZ_DPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      zz_token = ZZ_TRANSLATE (zz_char);
      ZZ__SYMBOL_PRINT ("Next token is", zz_token, &zz_lval, &zz_lloc);
    }

  /* If the proper action on seeing token ZZ_TOKEN is to reduce or to
     detect an error, take that action.  */
  zz_n += zz_token;
  if (zz_n < 0 || ZZ_LAST < zz_n || zz_check[zz_n] != zz_token)
    goto zz_default;
  zz_n = zz_table[zz_n];
  if (zz_n <= 0)
    {
      if (zz_n == 0 || zz_n == ZZ_TABLE_NINF)
	goto zz_errlab;
      zz_n = -zz_n;
      goto zz_reduce;
    }

  if (zz_n == ZZ_FINAL)
    ZZ_ACCEPT;

  /* Shift the look-ahead token.  */
  ZZ__SYMBOL_PRINT ("Shifting", zz_token, &zz_lval, &zz_lloc);

  /* Discard the token being shifted unless it is eof.  */
  if (zz_char != ZZ_EOF)
    zz_char = ZZ_EMPTY;

  *++zz_vsp = zz_lval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (zz_errstatus)
    zz_errstatus--;

  zz_state = zz_n;
  goto zz_newstate;


/*-----------------------------------------------------------.
| zz_default -- do the default action for the current state.  |
`-----------------------------------------------------------*/
zz_default:
  zz_n = zz_defact[zz_state];
  if (zz_n == 0)
    goto zz_errlab;
  goto zz_reduce;


/*-----------------------------.
| zz_reduce -- Do a reduction.  |
`-----------------------------*/
zz_reduce:
  /* zz_n is the number of a rule to reduce with.  */
  zz_len = zz_r2[zz_n];

  /* If ZZ_LEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets ZZ_VAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to ZZ_VAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that ZZ_VAL may be used uninitialized.  */
  zz_val = zz_vsp[1-zz_len];


  ZZ__REDUCE_PRINT (zz_n);
  switch (zz_n)
    {
        case 17:
#line 429 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in definition.");
	}
    break;

  case 18:
#line 436 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          /* the following steps apply to string buffers only, not files */
	  struct gl_list_t *stats;
          int dispose;
	  if ((zz_vsp[-2].slptr) != NULL) {
	    stats = gl_create(1L);
	    gl_append_ptr(stats,(void *)(zz_vsp[-2].slptr));
	    if (g_untrapped_error) {
	      ErrMsg_Generic("Because of a syntax error, the following statements are being ignored:");
		WriteStatementList(ASCERR,(zz_vsp[-2].slptr),4);
	      DestroyStatementList((zz_vsp[-2].slptr));
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
		  WriteStatementList(ASCERR,(zz_vsp[-2].slptr),4);
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
#line 481 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  Asc_ScannerPushBuffer((zz_vsp[-1].dquote_ptr));
	}
    break;

  case 20:
#line 485 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroyName((zz_vsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 21:
#line 490 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroyName((zz_vsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    break;

  case 22:
#line 498 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          Asc_ModuleCreateAlias(Asc_CurrentModule(),(zz_vsp[-1].dquote_ptr));
        }
    break;

  case 23:
#line 502 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroyName((zz_vsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 24:
#line 507 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroyName((zz_vsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    break;

  case 25:
#line 515 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (void)LoadArchiveLibrary(SCP((zz_vsp[-1].id_ptr)),SCP((zz_vsp[-3].id_ptr)));
	}
    break;

  case 26:
#line 519 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (void)LoadArchiveLibrary(SCP((zz_vsp[-1].id_ptr)),NULL);
	}
    break;

  case 27:
#line 526 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*  see comments for notes statement.  */
	  if( (zz_vsp[-1].int_value) != NOTES_T ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, (zz_vsp[-1].int_value), NULL);
	  }
	  if ((zz_vsp[-2].notesptr) != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = (zz_vsp[-2].notesptr);
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
	    DestroyNoteTmpList((zz_vsp[-2].notesptr));
          }
          g_type_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
    break;

  case 28:
#line 557 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  g_type_name = (zz_vsp[-2].id_ptr);
	  g_proc_name = (zz_vsp[-1].id_ptr);
	}
    break;

  case 29:
#line 565 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if ((zz_vsp[-3].tptr) == NULL) {
	    DestroyProcedureList((zz_vsp[-2].listp));
	  } else {
	    if( (zz_vsp[-1].int_value) != METHODS_T ) {
	      WarnMsg_MismatchEnd("ADD METHODS", NULL, (zz_vsp[-1].int_value), "METHODS");
	    }
	    if (AddMethods((zz_vsp[-3].tptr),(zz_vsp[-2].listp),g_untrapped_error) != 0) {
	      if ((zz_vsp[-3].tptr) != ILLEGAL_DEFINITION) {
                ErrMsg_ProcsRejected("ADD",SCP(GetName((zz_vsp[-3].tptr))));
	        DestroyProcedureList((zz_vsp[-2].listp));
	      } /* else adding in DEFINITION MODEL may have misgone */
	    }
	  }
	  g_untrapped_error = 0;
	}
    break;

  case 30:
#line 585 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((zz_vsp[-1].id_ptr));
	  if (tmptype == NULL) {
            ErrMsg_ProcTypeMissing("ADD", SCP((zz_vsp[-1].id_ptr)));
	  }
	  (zz_val.tptr) = tmptype; /* parent should check for NULL */
	  g_type_name = (zz_vsp[-1].id_ptr); /* scope for notes */
	}
    break;

  case 31:
#line 595 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 32:
#line 603 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if ((zz_vsp[-3].tptr) == NULL) {
	    DestroyProcedureList((zz_vsp[-2].listp));
	  } else {
	    if( (zz_vsp[-1].int_value) != METHODS_T ) {
	      WarnMsg_MismatchEnd("REPLACE METHODS", NULL, (zz_vsp[-1].int_value), "METHODS");
	    }
	    if (ReplaceMethods((zz_vsp[-3].tptr),(zz_vsp[-2].listp),g_untrapped_error) != 0) {
              ErrMsg_ProcsRejected("REPLACE",SCP(GetName((zz_vsp[-3].tptr))));
	      DestroyProcedureList((zz_vsp[-2].listp));
	    }
	  }
	  g_untrapped_error = 0;
	}
    break;

  case 33:
#line 621 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((zz_vsp[-1].id_ptr));
	  if (tmptype == NULL) {
            ErrMsg_ProcTypeMissing("REPLACE", SCP((zz_vsp[-1].id_ptr)));
	  }
	  (zz_val.tptr) = tmptype; /* parent should check for NULL */
	}
    break;

  case 34:
#line 630 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    break;

  case 35:
#line 638 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

          if(( (zz_vsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("ATOM", SCP(g_type_name),
	                        (zz_vsp[-1].int_value), SCP(g_type_name));
	  }
	  g_atom_dim_ptr = CheckDimensionsMatch(g_default_dim_ptr,
	                                        g_atom_dim_ptr);
	  if (g_atom_dim_ptr != NULL) {
	    def_ptr = CreateAtomTypeDef(g_type_name,
	                                g_refines_name,
	                                real_type, /* ignored..really */
	                                Asc_CurrentModule(),
	                                (zz_vsp[-5].int_value),
	                                (zz_vsp[-3].slptr),
	                                (zz_vsp[-2].listp),
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
	    DestroyStatementList((zz_vsp[-3].slptr));
	    DestroyProcedureList((zz_vsp[-2].listp));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
    break;

  case 36:
#line 691 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_refines_name = (zz_vsp[-3].id_ptr);
	  g_atom_dim_ptr = (zz_vsp[-2].dimp);
	  g_default_double = (zz_vsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
    break;

  case 37:
#line 702 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	  g_type_name = (zz_vsp[0].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 38:
#line 710 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 39:
#line 716 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = (zz_vsp[-1].int_value) ? -(zz_vsp[0].real_value) : (zz_vsp[0].real_value);
	  g_defaulted = 1;
	}
    break;

  case 40:
#line 721 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
    break;

  case 41:
#line 728 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
    break;

  case 42:
#line 735 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (zz_vsp[0].sym_ptr);
	  g_defaulted = 0;
	}
    break;

  case 43:
#line 745 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
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
	                                    (zz_vsp[-1].int_value),
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
#line 785 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  g_type_name = (zz_vsp[-6].id_ptr);
	  g_refines_name = (zz_vsp[-4].id_ptr);
	  g_atom_dim_ptr = (zz_vsp[-3].dimp);
	  switch (g_constant_type) {
	  case DOUBLECONSTANT:
	    g_default_double = (zz_vsp[-2].real_value);
	    break;
	  case LONGCONSTANT:
	    g_default_long = (zz_vsp[-2].real_value);
	    break;
	  case BOOLEANCONSTANT:
	    g_default_long = (zz_vsp[-2].int_value);
	    break;
	  case SYMBOLCONSTANT:
	    g_default_symbol = (zz_vsp[-2].sym_ptr);
	    break;
	  default:
	    ErrMsg_Generic("Wierd constant type assign encountered.");
	    break; /* better not be reached. */
	  }
	  g_header_linenum = LineNum();
	  if ((zz_vsp[-1].dquote_ptr) != NULL) {
	    CollectNote(CreateNote(g_type_name,InlineNote(),SelfNote(),NULL,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar((zz_vsp[-1].dquote_ptr),InlineNote()),
	                           g_header_linenum,NULL,nd_empty));
	  }
	}
    break;

  case 45:
#line 818 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    break;

  case 46:
#line 824 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = (zz_vsp[-1].int_value) ? -(zz_vsp[0].real_value) : (zz_vsp[0].real_value);
	  g_defaulted = 1;
	}
    break;

  case 47:
#line 829 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 48:
#line 836 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    break;

  case 49:
#line 843 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sym_ptr) = (zz_vsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
    break;

  case 50:
#line 853 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;
	  if(( (zz_vsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("MODEL", SCP(g_type_name),
	                        (zz_vsp[-1].int_value), SCP(g_type_name));
	  }
	  def_ptr = CreateModelTypeDef(g_type_name,
	                               g_refines_name,
	                               Asc_CurrentModule(),
	                               (zz_vsp[-5].int_value),
	                               (zz_vsp[-3].slptr),
	                               (zz_vsp[-2].listp),
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
#line 895 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (zz_vsp[-2].slptr);
	  g_parameter_wheres = (zz_vsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
    break;

  case 52:
#line 904 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /* g_type_name = $1; */
	  g_model_parameters = (zz_vsp[-5].slptr);
	  g_parameter_wheres = (zz_vsp[-4].slptr);
	  g_refines_name = (zz_vsp[-2].id_ptr);
	  g_parameter_reduction = (zz_vsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
    break;

  case 53:
#line 916 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	  g_type_name = (zz_vsp[0].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 54:
#line 924 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = NULL;
	}
    break;

  case 55:
#line 928 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = (zz_vsp[-1].slptr); /* this could be much more sophisticated */
	}
    break;

  case 56:
#line 935 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = NULL;
	}
    break;

  case 57:
#line 939 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = (zz_vsp[-1].slptr); /* this could be much more sophisticated */
	}
    break;

  case 58:
#line 946 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = NULL;
	}
    break;

  case 59:
#line 950 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = (zz_vsp[-1].slptr); /* this could be much more sophisticated */
	}
    break;

  case 60:
#line 957 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  if (((zz_vsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("PATCH", SCP(g_type_name),
	                        (zz_vsp[-1].int_value), SCP(g_type_name));
	  }
	  def_ptr = CreatePatchTypeDef(g_type_name,
	                               g_refines_name,
	                               NULL,
	                               Asc_CurrentModule(),
	                               (zz_vsp[-3].slptr),
	                               (zz_vsp[-2].listp),
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
#line 989 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*
	   * A patch definition looks just like a model def.
	   * with the original name <=> refine name.
	   */
	  g_type_name = (zz_vsp[-3].id_ptr);
	  g_refines_name = (zz_vsp[-1].id_ptr);
	  g_header_linenum = LineNum();
	}
    break;

  case 62:
#line 1002 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.int_value) = 0;
	}
    break;

  case 63:
#line 1006 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.int_value) = 1;
	}
    break;

  case 64:
#line 1013 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (zz_vsp[-1].int_value) != IDENTIFIER_T ) || ( g_end_identifier != (zz_vsp[-4].id_ptr) )) {
	    WarnMsg_MismatchEnd("DEFINITION", SCP((zz_vsp[-4].id_ptr)), (zz_vsp[-1].int_value), SCP((zz_vsp[-4].id_ptr)));
	  }
	  if( (zz_vsp[-4].id_ptr) == GetBaseTypeName(relation_type)) {
	    def_ptr = CreateRelationTypeDef(Asc_CurrentModule(),(zz_vsp[-4].id_ptr),(zz_vsp[-3].slptr),(zz_vsp[-2].listp));
	  }
	  else if( (zz_vsp[-4].id_ptr) == GetBaseTypeName(logrel_type) ) {
	    def_ptr = CreateLogRelTypeDef(Asc_CurrentModule(),(zz_vsp[-4].id_ptr),(zz_vsp[-3].slptr),(zz_vsp[-2].listp));
	  }
	  else {
	    ErrMsg_Generic("Bad type passed to DEFINITION statement.");
	    def_ptr = NULL;
	  }
	  if ( def_ptr != NULL ) {
	    keepnotes = AddType(def_ptr);
	  } else {
	    ErrMsg_NullDefPointer(SCP((zz_vsp[-4].id_ptr)));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = NULL;
	  g_untrapped_error = 0;
	}
    break;

  case 65:
#line 1043 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	  g_type_name = (zz_vsp[0].id_ptr); /* want this set early so parm lists see it */
	}
    break;

  case 66:
#line 1052 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    { /* nothing to do. just cruft to fix ; problem */ }
    break;

  case 67:
#line 1057 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          struct UnitDefinition *ud;
          unsigned long c,len;

	  if( (zz_vsp[0].int_value) != UNITS_T ) {
	    WarnMsg_MismatchEnd("UNITS", NULL, (zz_vsp[0].int_value), NULL);
	  }
          len = gl_length((zz_vsp[-1].listp));
          for (c=1; c <= len; c++) {
            ud = (struct UnitDefinition *)gl_fetch((zz_vsp[-1].listp),c);
            ProcessUnitDef(ud);
            DestroyUnitDef(ud);
          }
          gl_destroy((zz_vsp[-1].listp));
          (zz_val.statptr) = NULL;
	}
    break;

  case 68:
#line 1076 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.listp) = gl_create(100L);
	}
    break;

  case 69:
#line 1080 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  gl_append_ptr((zz_vsp[-1].listp),(char *)(zz_vsp[0].udefptr));
	  (zz_val.listp) = (zz_vsp[-1].listp);
	}
    break;

  case 70:
#line 1088 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.udefptr) = CreateUnitDef((zz_vsp[-3].id_ptr),(zz_vsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
                             LineNum());
	}
    break;

  case 71:
#line 1097 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.listp) = NULL;
	}
    break;

  case 72:
#line 1101 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    { /* To get rid of this, we will need a global proclist
           * that accumulates procs until a MODEL production is
           * completed. If any other sort of production is started,
           * and proclist is not NULL, it should be discarded.
           */
	}
    break;

  case 73:
#line 1108 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.listp) = (zz_vsp[0].listp);
	}
    break;

  case 74:
#line 1115 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.listp) = (zz_vsp[0].listp);
	  gl_sort((zz_val.listp),(CmpFunc)CmpProcs);
	}
    break;

  case 75:
#line 1122 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.listp) = gl_create(7L);
	}
    break;

  case 76:
#line 1126 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  unsigned long c;
	  struct InitProcedure *oldproc;
	  c = gl_length((zz_vsp[-1].listp));
          while (c > 0) {
            oldproc = (struct InitProcedure *)gl_fetch((zz_vsp[-1].listp),c);
            if (ProcName((zz_vsp[0].procptr)) == ProcName(oldproc)) {
	      ErrMsg_DuplicateProc((zz_vsp[0].procptr));
              break;
            }
            c--;
          }
	  if (c) { /* broke early */
	    DestroyProcedure((zz_vsp[0].procptr));
	  } else {
	    gl_append_ptr((zz_vsp[-1].listp),(char *)(zz_vsp[0].procptr));
	  }
	  (zz_val.listp) = (zz_vsp[-1].listp);
	}
    break;

  case 77:
#line 1149 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if (((zz_vsp[-1].int_value) != IDENTIFIER_T) || ((zz_vsp[-4].id_ptr) != g_end_identifier)) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("METHOD", SCP((zz_vsp[-4].id_ptr)), (zz_vsp[-1].int_value), SCP((zz_vsp[-4].id_ptr)));
	  }
	  (zz_val.procptr) = CreateProcedure((zz_vsp[-4].id_ptr),(zz_vsp[-2].slptr));
	  g_proc_name = NULL;
	}
    break;

  case 78:
#line 1163 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	  g_proc_name = (zz_vsp[0].id_ptr);
	}
    break;

  case 79:
#line 1172 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = CreateStatementList((zz_vsp[0].listp));
	}
    break;

  case 80:
#line 1179 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.listp) = gl_create(7L);
	}
    break;

  case 81:
#line 1183 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ((zz_vsp[-1].statptr) != NULL) {
	    gl_append_ptr((zz_vsp[-2].listp),(char *)(zz_vsp[-1].statptr));
	  }
	  (zz_val.listp) = (zz_vsp[-2].listp);
	}
    break;

  case 82:
#line 1191 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  ErrMsg_Generic("Error in statement input.");
	  (zz_val.listp) = (zz_vsp[-2].listp);
	}
    break;

  case 113:
#line 1232 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((zz_vsp[-2].id_ptr));
	  if ((zz_vsp[0].eptr) != NULL) {
	    ErrMsg_Generic("WITH VALUE clause not allowed in IS_A.");
	    g_untrapped_error++;
	    DestroyVariableList((zz_vsp[-4].lptr));
	    DestroySetList(g_typeargs);
	    DestroyExprList((zz_vsp[0].eptr));
	    (zz_val.statptr) = NULL;
	  } else {
	    if (tmptype != NULL) {
	      if ((GetBaseType(tmptype) != model_type) &&
	          (g_typeargs != NULL)) {
	        error_reporter_current_line(ASC_USER_ERROR,
	                "IS_A has arguments to the nonmodel type %s.\n",
	                SCP((zz_vsp[-2].id_ptr)));
	        DestroyVariableList((zz_vsp[-4].lptr));
	        DestroySetList(g_typeargs);
	        DestroyExprList((zz_vsp[0].eptr));
	        g_untrapped_error++;
	        (zz_val.statptr) = NULL;
	      } else {
	        (zz_val.statptr) = CreateISA((zz_vsp[-4].lptr),(zz_vsp[-2].id_ptr),g_typeargs,(zz_vsp[-1].id_ptr));
	      }
	    } else {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_A uses the undefined type %s.", SCP((zz_vsp[-2].id_ptr)));
	      DestroyVariableList((zz_vsp[-4].lptr));
	      DestroySetList(g_typeargs);
	      DestroyExprList((zz_vsp[0].eptr));
	      g_untrapped_error++;
	      (zz_val.statptr) = NULL;
	    }
	  }
	  g_typeargs = NULL;

	}
    break;

  case 114:
#line 1273 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((zz_vsp[-2].id_ptr));
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) &&
	        (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"WILL_BE has arguments to the nonmodel type '%s'",SCP((zz_vsp[-2].id_ptr)));
	      DestroyVariableList((zz_vsp[-4].lptr));
	      DestroySetList(g_typeargs);
	      DestroyExprList((zz_vsp[0].eptr));
	      g_untrapped_error++;
	      (zz_val.statptr) = NULL;
	    } else {
	      (zz_val.statptr) = CreateWILLBE((zz_vsp[-4].lptr),(zz_vsp[-2].id_ptr),g_typeargs,(zz_vsp[-1].id_ptr),(zz_vsp[0].eptr));
	    }
	  } else {
	    DestroyVariableList((zz_vsp[-4].lptr));
	    DestroySetList(g_typeargs);
	    DestroyExprList((zz_vsp[0].eptr));
	    g_untrapped_error++;
	    (zz_val.statptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"WILL_BE uses the undefined type %s.",SCP((zz_vsp[-2].id_ptr)));
	  }
	  g_typeargs = NULL;
	}
    break;

  case 115:
#line 1302 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateALIASES((zz_vsp[-2].lptr),(zz_vsp[0].nptr));
	}
    break;

  case 116:
#line 1307 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  int carray_err;
	  carray_err = 0;
	  if (VariableListLength((zz_vsp[-11].lptr)) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES allows only 1 LHS name. Found:");
	    WriteVariableList(ASCERR,(zz_vsp[-11].lptr));
	  }
	  if (VariableListLength((zz_vsp[-5].lptr)) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES/IS_A allows only one LHS name. Found:");
	    WriteVariableList(ASCERR,(zz_vsp[-5].lptr));
	  }
	  /* verify $9 == "set" */
	  if (!carray_err && (zz_vsp[-3].id_ptr) != GetBaseTypeName(set_type)) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,"Compound ALIASES statement requires IS_A %s. ",SCP(GetBaseTypeName(set_type)));
	    FPRINTF(ASCERR,"    Found %s.\n",SCP((zz_vsp[-3].id_ptr)));
	  }
	  /* verify set type */
	  if ((!carray_err) &&
	      ((zz_vsp[-1].id_ptr) != GetBaseTypeName(symbol_constant_type)) &&
	      ((zz_vsp[-1].id_ptr) != GetBaseTypeName(integer_constant_type))) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES IS_A statement requires %s or %s.\n",
	            SCP(GetBaseTypeName(integer_constant_type)),
	            SCP(GetBaseTypeName(symbol_constant_type)));
	    FPRINTF(ASCERR,"	Found %s.\n",SCP((zz_vsp[-1].id_ptr)));
	  }
	  if (carray_err) {
	    DestroyVariableList((zz_vsp[-11].lptr));
	    DestroyVariableList((zz_vsp[-8].lptr));
	    DestroyVariableList((zz_vsp[-5].lptr));
	    DestroySetList((zz_vsp[0].sptr));
	    g_untrapped_error++;
	    (zz_val.statptr) = NULL;
	  } else {
	    int intset;
	    intset = ((zz_vsp[-1].id_ptr) == GetBaseTypeName(integer_constant_type));
	    (zz_val.statptr) = CreateARR((zz_vsp[-11].lptr),(zz_vsp[-8].lptr),(zz_vsp[-5].lptr),intset,(zz_vsp[0].sptr));
	  }
	}
    break;

  case 117:
#line 1356 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = NULL;
	}
    break;

  case 118:
#line 1360 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = (zz_vsp[-1].sptr);
	}
    break;

  case 119:
#line 1367 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if (FindType((zz_vsp[-1].id_ptr))) {
	    (zz_val.statptr) = CreateREF((zz_vsp[-3].lptr),(zz_vsp[-1].id_ptr),(zz_vsp[0].id_ptr),1);
	  } else {
	    (zz_val.statptr) = CreateREF((zz_vsp[-3].lptr),(zz_vsp[-1].id_ptr),(zz_vsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((zz_vsp[-1].id_ptr)));
	  }
	}
    break;

  case 120:
#line 1379 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((zz_vsp[0].id_ptr));
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) && 
                (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_REFINED_TO has arguments to the nonmodel type %s.",SCP((zz_vsp[0].id_ptr)));
	      DestroyVariableList((zz_vsp[-2].lptr));
	      DestroySetList(g_typeargs);
	      g_untrapped_error++;
	      (zz_val.statptr) = NULL;
	    } else {
	      (zz_val.statptr) = CreateIRT((zz_vsp[-2].lptr),(zz_vsp[0].id_ptr),g_typeargs);
	    }
	  } else {
	    error_reporter_current_line(ASC_USER_ERROR,"The IS_REFINED_TO uses the undefined type %s.\n",SCP((zz_vsp[0].id_ptr)));
	    DestroyVariableList((zz_vsp[-2].lptr));
	    DestroySetList(g_typeargs);
	    g_untrapped_error++;
	    (zz_val.statptr) = NULL;
	  }
	  g_typeargs = NULL;
	}
    break;

  case 121:
#line 1406 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	  g_callargs = NULL;
	}
    break;

  case 122:
#line 1411 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[-3].id_ptr);
	  g_callargs = (zz_vsp[-1].sptr);
	}
    break;

  case 123:
#line 1419 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	  g_typeargs = NULL;
	}
    break;

  case 124:
#line 1424 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[-3].id_ptr);
	  g_typeargs = (zz_vsp[-1].sptr);
	}
    break;

  case 125:
#line 1432 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = NULL;
	}
    break;

  case 126:
#line 1436 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	}
    break;

  case 127:
#line 1443 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = NULL;
	}
    break;

  case 128:
#line 1447 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.id_ptr) = (zz_vsp[0].id_ptr);
	}
    break;

  case 129:
#line 1454 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = NULL;
	}
    break;

  case 130:
#line 1458 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = (zz_vsp[0].eptr);
	}
    break;

  case 131:
#line 1465 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateAA((zz_vsp[-1].lptr));
	}
    break;

  case 132:
#line 1472 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateATS((zz_vsp[-1].lptr));
	}
    break;

  case 133:
#line 1479 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateWBTS((zz_vsp[-1].lptr));
	}
    break;

  case 134:
#line 1486 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateWNBTS((zz_vsp[-1].lptr));
	}
    break;

  case 135:
#line 1493 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateASSIGN((zz_vsp[-2].nptr),(zz_vsp[0].eptr));
	}
    break;

  case 136:
#line 1497 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateCASSIGN((zz_vsp[-2].nptr),(zz_vsp[0].eptr));
	}
    break;

  case 137:
#line 1504 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if (IsRelation((zz_vsp[0].eptr))) {
	    if (g_parse_relns == 0) {
	      DestroyExprList((zz_vsp[0].eptr));
	      (zz_val.statptr) = NULL;
	    } else {
	      (zz_val.statptr) = CreateREL(NULL,(zz_vsp[0].eptr));
	    }
	  } else {
	    (zz_val.statptr) = CreateLOGREL(NULL,(zz_vsp[0].eptr));
	  }
	}
    break;

  case 138:
#line 1517 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if (IsRelation((zz_vsp[0].eptr))) {
	    if (g_parse_relns == 0) {
	      DestroyExprList((zz_vsp[0].eptr));
	      DestroyName((zz_vsp[-2].nptr));
	      (zz_val.statptr) = NULL;
	    } else {
	      (zz_val.statptr) = CreateREL((zz_vsp[-2].nptr),(zz_vsp[0].eptr));
	    }
	  } else {
	    (zz_val.statptr) = CreateLOGREL((zz_vsp[-2].nptr),(zz_vsp[0].eptr));
	  }
	}
    break;

  case 139:
#line 1534 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = (zz_vsp[0].eptr);
	  if (NumberOfRelOps((zz_vsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
    break;

  case 140:
#line 1544 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((zz_vsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 141:
#line 1552 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((zz_vsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    break;

  case 142:
#line 1563 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*
	   * This is the blackbox declarative external relation.
	   */
	  struct VariableList *vl;
	  vl = JoinVariableLists((zz_vsp[-4].lptr),(zz_vsp[-2].lptr));
	  (zz_val.statptr) = CreateEXTERN(2,(zz_vsp[-8].nptr),SCP((zz_vsp[-6].id_ptr)),vl,(zz_vsp[-1].nptr),NULL);
	}
    break;

  case 143:
#line 1575 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.lptr) = (zz_vsp[-2].lptr);
	}
    break;

  case 144:
#line 1582 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.lptr) = (zz_vsp[-2].lptr);
	}
    break;

  case 145:
#line 1589 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.nptr) = NULL;
	}
    break;

  case 146:
#line 1593 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.nptr) = (zz_vsp[-2].nptr);
	}
    break;

  case 147:
#line 1600 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*
	   * This is the glassbox declarative external relation.
	   * This now allows a scope for placement of the relations
	   */
	  struct VariableList *vl = (zz_vsp[-4].lptr);
	  struct Name *nptr;
	  char tmp[32]; 
	  symchar *str;

	  sprintf(tmp,"%ld",(zz_vsp[-2].int_value));
	  str = AddSymbol(tmp);
	  nptr = CreateIdName(str);
	  (zz_val.statptr) = CreateEXTERN(1,(zz_vsp[-8].nptr),SCP((zz_vsp[-6].id_ptr)),vl,nptr,(zz_vsp[0].nptr));
	}
    break;

  case 148:
#line 1619 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.nptr) = NULL;
	}
    break;

  case 149:
#line 1623 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.nptr) = (zz_vsp[0].nptr);
	}
    break;

  case 150:
#line 1631 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != FOR_T ) {
	    WarnMsg_MismatchEnd("FOR", SCP((zz_vsp[-6].id_ptr)), (zz_vsp[0].int_value), NULL);
	  }
          if ((zz_vsp[-2].fkind) == fk_create && (zz_vsp[-3].order) != f_random) {
            /* create cannot have an order in declarative FOR */
	    ErrMsg_Generic("FOR loops only accept DECREASING or INCREASING in the method section.");
	    g_untrapped_error++;
          }
          if ((zz_vsp[-2].fkind) == fk_do && (zz_vsp[-3].order) == f_random) {
            /* all FOR/DO default to increasing */
	    (zz_val.statptr) = CreateFOR((zz_vsp[-6].id_ptr),(zz_vsp[-4].eptr),(zz_vsp[-1].slptr),f_increasing,(zz_vsp[-2].fkind));
          } else {
	    (zz_val.statptr) = CreateFOR((zz_vsp[-6].id_ptr),(zz_vsp[-4].eptr),(zz_vsp[-1].slptr),(zz_vsp[-3].order),(zz_vsp[-2].fkind));
          }
	}
    break;

  case 151:
#line 1651 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.order) = f_random;
	}
    break;

  case 152:
#line 1655 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.order) = f_increasing;
	}
    break;

  case 153:
#line 1659 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.order) = f_decreasing;
	}
    break;

  case 154:
#line 1666 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          (zz_val.fkind) = fk_create; /* declarative FOR */
	}
    break;

  case 155:
#line 1670 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          (zz_val.fkind) = fk_expect; /* parameter FOR */
	}
    break;

  case 156:
#line 1674 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          (zz_val.fkind) = fk_check; /* WHERE FOR */
	}
    break;

  case 157:
#line 1678 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          (zz_val.fkind) = fk_do; /* method FOR */
	}
    break;

  case 158:
#line 1685 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateRUN((zz_vsp[0].nptr),NULL);
	}
    break;

  case 159:
#line 1689 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateRUN((zz_vsp[0].nptr),(zz_vsp[-2].nptr));	  /* type :: name */
	}
    break;

  case 160:
#line 1696 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
		(zz_val.statptr) = CreateFIX((zz_vsp[0].lptr));
	}
    break;

  case 161:
#line 1703 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
		(zz_val.statptr) = CreateFREE((zz_vsp[0].lptr));
	}
    break;

  case 162:
#line 1710 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*
	   * This is procedural external code.
	   */
	  (zz_val.statptr) = CreateEXTERN(0,NULL,SCP((zz_vsp[-3].id_ptr)),(zz_vsp[-1].lptr),NULL,NULL);
	}
    break;

  case 163:
#line 1720 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*
	   * This is proper procedural external method code.
	   */
	  (zz_val.statptr) = CreateCALL((zz_vsp[0].id_ptr),g_callargs);
          g_callargs = NULL;
	}
    break;

  case 164:
#line 1731 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
		(zz_val.statptr) = CreateASSERT((zz_vsp[0].eptr));
	}
    break;

  case 165:
#line 1737 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != IF_T ) {
	    WarnMsg_MismatchEnd("IF", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  (zz_val.statptr) = CreateIF((zz_vsp[-4].eptr),(zz_vsp[-2].slptr),(zz_vsp[-1].slptr));
	}
    break;

  case 166:
#line 1747 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != WHILE_T ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  (zz_val.statptr) = CreateWhile((zz_vsp[-3].eptr),(zz_vsp[-1].slptr));
	}
    break;

  case 167:
#line 1756 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = NULL;
	}
    break;

  case 168:
#line 1760 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.slptr) = (zz_vsp[0].slptr);
	}
    break;

  case 169:
#line 1767 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((zz_vsp[-1].wptr));
	  DestroyVariableList((zz_vsp[-2].lptr));
	  g_untrapped_error++;
	  (zz_val.statptr) = NULL;
	}
    break;

  case 170:
#line 1778 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((zz_vsp[-1].wptr));
	  DestroyVariableList((zz_vsp[-2].lptr));
	  DestroyName((zz_vsp[-5].nptr));
	  g_untrapped_error++;
	  (zz_val.statptr) = NULL;
	}
    break;

  case 171:
#line 1790 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  (zz_val.statptr) = CreateWHEN(NULL,(zz_vsp[-3].lptr),(zz_vsp[-1].wptr));
	}
    break;

  case 172:
#line 1797 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != WHEN_T ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  (zz_val.statptr) = CreateWHEN((zz_vsp[-7].nptr),(zz_vsp[-3].lptr),(zz_vsp[-1].wptr));
	}
    break;

  case 173:
#line 1807 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.wptr) = ReverseWhenCases((zz_vsp[0].wptr));
	}
    break;

  case 174:
#line 1814 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.wptr) = CreateWhen((zz_vsp[-2].sptr),(zz_vsp[0].slptr));
	}
    break;

  case 175:
#line 1818 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.wptr) = CreateWhen(NULL,(zz_vsp[0].slptr));
	}
    break;

  case 176:
#line 1822 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.wptr) = LinkWhenCases(CreateWhen((zz_vsp[-2].sptr),(zz_vsp[0].slptr)),(zz_vsp[-4].wptr));
	}
    break;

  case 177:
#line 1826 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.wptr) = LinkWhenCases(CreateWhen(NULL,(zz_vsp[0].slptr)),(zz_vsp[-3].wptr));
	}
    break;

  case 178:
#line 1833 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateFlow(fc_break,NULL);
	}
    break;

  case 179:
#line 1837 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateFlow(fc_continue,NULL);
	}
    break;

  case 180:
#line 1841 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateFlow(fc_fallthru,NULL);
	}
    break;

  case 181:
#line 1845 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateFlow(fc_return,NULL);
	}
    break;

  case 182:
#line 1849 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateFlow(fc_stop,(zz_vsp[0].braced_ptr));
	}
    break;

  case 183:
#line 1856 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.statptr) = CreateFNAME((zz_vsp[0].nptr));
	}
    break;

  case 184:
#line 1863 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != SELECT_T ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SELECT statement.");
	  DestroySelectList((zz_vsp[-1].septr));
	  DestroyVariableList((zz_vsp[-2].lptr));
	  g_untrapped_error++;
	  (zz_val.statptr) = NULL;
	}
    break;

  case 185:
#line 1874 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != SELECT_T ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  (zz_val.statptr) = CreateSELECT((zz_vsp[-3].lptr),(zz_vsp[-1].septr));
	}
    break;

  case 186:
#line 1884 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.septr) = ReverseSelectCases((zz_vsp[0].septr));
	}
    break;

  case 187:
#line 1891 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.septr) = CreateSelect((zz_vsp[-2].sptr),(zz_vsp[0].slptr));
	}
    break;

  case 188:
#line 1895 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.septr) = CreateSelect(NULL,(zz_vsp[0].slptr));
	}
    break;

  case 189:
#line 1899 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.septr) = LinkSelectCases(CreateSelect((zz_vsp[-2].sptr),(zz_vsp[0].slptr)),(zz_vsp[-4].septr));
	}
    break;

  case 190:
#line 1903 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.septr) = LinkSelectCases(CreateSelect(NULL,(zz_vsp[0].slptr)),(zz_vsp[-3].septr));
	}
    break;

  case 191:
#line 1910 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != SWITCH_T ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SWITCH statement.");
	  DestroySwitchList((zz_vsp[-1].swptr));
	  DestroyVariableList((zz_vsp[-2].lptr));
	  g_untrapped_error++;
	  (zz_val.statptr) = NULL;
	}
    break;

  case 192:
#line 1921 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != SWITCH_T ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  (zz_val.statptr) = CreateSWITCH((zz_vsp[-3].lptr),(zz_vsp[-1].swptr));
	}
    break;

  case 193:
#line 1931 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.swptr) = ReverseSwitchCases((zz_vsp[0].swptr));
	}
    break;

  case 194:
#line 1938 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.swptr) = CreateSwitch((zz_vsp[-2].sptr),(zz_vsp[0].slptr));
	}
    break;

  case 195:
#line 1942 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.swptr) = CreateSwitch(NULL,(zz_vsp[0].slptr));
	}
    break;

  case 196:
#line 1946 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.swptr) = LinkSwitchCases(CreateSwitch((zz_vsp[-2].sptr),(zz_vsp[0].slptr)),(zz_vsp[-4].swptr));
	}
    break;

  case 197:
#line 1950 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.swptr) = LinkSwitchCases(CreateSwitch(NULL,(zz_vsp[0].slptr)),(zz_vsp[-3].swptr));
	}
    break;

  case 198:
#line 1957 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if( (zz_vsp[0].int_value) != CONDITIONAL_T ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  (zz_val.statptr) = CreateCOND((zz_vsp[-1].slptr));
	}
    break;

  case 199:
#line 1967 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*  All processing of notes takes place on the notes_body here.
	   *  Notes should NOT be added to the statement list.
	   *  Here we know the current type and method names.
	   */
	  if( (zz_vsp[0].int_value) != NOTES_T ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, (zz_vsp[0].int_value), NULL);
	  }
	  if ((zz_vsp[-1].notesptr) != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = (zz_vsp[-1].notesptr);
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
	    DestroyNoteTmpList((zz_vsp[-1].notesptr));
          }
	  (zz_val.statptr) = NULL;
	}
    break;

  case 200:
#line 2004 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (zz_val.notesptr) = (zz_vsp[0].notesptr);
	  assert((zz_val.notesptr)->lang == NULL);
	  (zz_val.notesptr)->lang = (zz_vsp[-1].sym_ptr);
	}
    break;

  case 201:
#line 2013 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  struct NoteTmp *nt;
	  (zz_val.notesptr) = (zz_vsp[-2].notesptr);
	  assert((zz_vsp[0].notesptr)->lang == NULL);
	  (zz_vsp[0].notesptr)->lang = (zz_vsp[-1].sym_ptr);
	  nt = (zz_val.notesptr);
	  while (nt->next != NULL) {
	    nt = nt->next;
	  }
	  LinkNoteTmp(nt,(zz_vsp[0].notesptr));
	}
    break;

  case 202:
#line 2028 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.notesptr) = CreateNoteTmp(NULL, AddBraceChar((zz_vsp[0].braced_ptr),NULL),
                             (void *)(zz_vsp[-1].lptr), LineNum());
	}
    break;

  case 203:
#line 2033 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.notesptr) = CreateNoteTmp(NULL, AddBraceChar((zz_vsp[0].braced_ptr),NULL),
	                     (void *)(zz_vsp[-1].lptr), LineNum());
	  LinkNoteTmp((zz_val.notesptr),(zz_vsp[-2].notesptr));
	}
    break;

  case 204:
#line 2042 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (zz_val.lptr) = ReverseVariableList((zz_vsp[0].lptr));
	}
    break;

  case 205:
#line 2053 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.lptr) = CreateVariableNode((zz_vsp[0].nptr));
	}
    break;

  case 206:
#line 2057 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.lptr) = CreateVariableNode((zz_vsp[0].nptr));
	  LinkVariableNodes((zz_val.lptr),(zz_vsp[-2].lptr));
	}
    break;

  case 207:
#line 2062 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  ErrMsg_CommaName("name",(zz_vsp[0].nptr));
	  (zz_val.lptr) = CreateVariableNode((zz_vsp[0].nptr));
	  LinkVariableNodes((zz_val.lptr),(zz_vsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
    break;

  case 208:
#line 2075 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  symchar *simple;
	  void *data;
	  enum NoteData nd;
	  (zz_val.nptr) = ReverseName((zz_vsp[-1].nptr));
	  if ((zz_vsp[0].dquote_ptr) != NULL && (zz_vsp[-1].nptr) != NULL) {
            simple = SimpleNameIdPtr((zz_val.nptr));
	    data = (simple == NULL ? (void *)(zz_val.nptr) : NULL);
	    nd = (data == NULL ? nd_empty : nd_name);
	    CollectNote(CreateNote(g_type_name, InlineNote(), simple,
	                           g_proc_name,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar((zz_vsp[0].dquote_ptr),InlineNote()),
	                           LineNum(), data, nd));
	  }
	}
    break;

  case 209:
#line 2095 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.nptr) = CreateIdName((zz_vsp[0].id_ptr));
	}
    break;

  case 210:
#line 2099 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.nptr) = CreateIdName((zz_vsp[0].id_ptr));
	  LinkNames((zz_val.nptr),(zz_vsp[-2].nptr));
	}
    break;

  case 211:
#line 2104 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  if ((zz_vsp[-1].sptr) == NULL) {
	    error_reporter_current_line(ASC_USER_ERROR,"syntax error: Empty set in name definition, name:");
	    WriteName(ASCERR,(zz_vsp[-3].nptr));
	    FPRINTF(ASCERR,"[]\n");
	    g_untrapped_error++;
	  } else {
	    (zz_val.nptr) = CreateSetName((zz_vsp[-1].sptr));
	    LinkNames((zz_val.nptr),(zz_vsp[-3].nptr));
	  }
	}
    break;

  case 212:
#line 2119 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = CONDITIONAL_T;
        }
    break;

  case 213:
#line 2124 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = FOR_T;
        }
    break;

  case 214:
#line 2129 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = IF_T;
        }
    break;

  case 215:
#line 2134 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = INTERACTIVE_T;
        }
    break;

  case 216:
#line 2139 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = METHODS_T;
        }
    break;

  case 217:
#line 2144 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = NOTES_T;
        }
    break;

  case 218:
#line 2149 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = SELECT_T;
        }
    break;

  case 219:
#line 2154 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = SWITCH_T;
        }
    break;

  case 220:
#line 2159 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = UNITS_T;
        }
    break;

  case 221:
#line 2164 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = GLOBAL_T;
        }
    break;

  case 222:
#line 2169 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = WHEN_T;
        }
    break;

  case 223:
#line 2174 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = WHILE_T;
        }
    break;

  case 224:
#line 2179 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = (zz_vsp[0].id_ptr);
          (zz_val.int_value) = IDENTIFIER_T;
        }
    break;

  case 225:
#line 2184 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
          g_end_identifier = NULL;
          (zz_val.int_value) = END_T;
        }
    break;

  case 226:
#line 2192 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.braced_ptr) = NULL;
	}
    break;

  case 227:
#line 2196 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.braced_ptr) = (zz_vsp[0].braced_ptr);
	}
    break;

  case 228:
#line 2203 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dquote_ptr) = NULL;
	}
    break;

  case 229:
#line 2207 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dquote_ptr) = (zz_vsp[0].dquote_ptr);
	}
    break;

  case 230:
#line 2214 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = ReverseSetList((zz_vsp[0].sptr));
	}
    break;

  case 231:
#line 2218 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = NULL;
	}
    break;

  case 232:
#line 2225 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = CreateSingleSet((zz_vsp[0].eptr));
	}
    break;

  case 233:
#line 2229 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = CreateRangeSet((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 234:
#line 2233 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = CreateSingleSet((zz_vsp[0].eptr));
	  LinkSets((zz_val.sptr),(zz_vsp[-2].sptr));
	}
    break;

  case 235:
#line 2238 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.sptr) = CreateRangeSet((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	  LinkSets((zz_val.sptr),(zz_vsp[-4].sptr));
	}
    break;

  case 236:
#line 2246 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = (zz_vsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
    break;

  case 237:
#line 2252 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = (zz_vsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
    break;

  case 238:
#line 2261 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.real_value) = (zz_vsp[-1].real_value)*(zz_vsp[0].real_value);
	}
    break;

  case 239:
#line 2265 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((zz_vsp[0].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (zz_val.real_value) = (double)(zz_vsp[-1].int_value)*UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
            char **errv;
	    (zz_val.real_value) = (double)(zz_vsp[-1].int_value);
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'", (zz_vsp[0].braced_ptr));
            errv = UnitsExplainError((zz_vsp[0].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
    break;

  case 240:
#line 2286 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  g_dim_ptr = Dimensionless();
	  (zz_val.real_value) = 1.0;
	}
    break;

  case 241:
#line 2291 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((zz_vsp[0].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (zz_val.real_value) = UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
            char **errv;
	    (zz_val.real_value) = 1.0;
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'",(zz_vsp[0].braced_ptr));
            errv = UnitsExplainError((zz_vsp[0].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
    break;

  case 242:
#line 2312 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimp) = (zz_vsp[0].dimp);
	}
    break;

  case 243:
#line 2316 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimp) = Dimensionless();
	}
    break;

  case 244:
#line 2320 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimp) = WildDimension();
	}
    break;

  case 245:
#line 2327 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimp) = WildDimension();
	}
    break;

  case 246:
#line 2331 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimp) = FindOrAddDimen(&((zz_vsp[0].dimen)));
	}
    break;

  case 247:
#line 2338 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  ParseDim(&((zz_val.dimen)),SCP((zz_vsp[0].id_ptr)));
	}
    break;

  case 248:
#line 2342 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  ClearDimensions(&((zz_val.dimen)));
	}
    break;

  case 249:
#line 2346 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimen) = SubDimensions(&((zz_vsp[-2].dimen)),&((zz_vsp[0].dimen)));
	}
    break;

  case 250:
#line 2350 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimen) = AddDimensions(&((zz_vsp[-2].dimen)),&((zz_vsp[0].dimen)));
	}
    break;

  case 251:
#line 2354 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.dimen) = ScaleDimensions(&((zz_vsp[-2].dimen)),(zz_vsp[0].frac_value));
	}
    break;

  case 252:
#line 2358 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  CopyDimensions(&((zz_vsp[-1].dimen)),&((zz_val.dimen)));
	}
    break;

  case 253:
#line 2365 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.frac_value) = (zz_vsp[-1].int_value) ? NegateF((zz_vsp[0].frac_value)) : (zz_vsp[0].frac_value);
	}
    break;

  case 254:
#line 2372 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.frac_value) = CreateFraction((short)(zz_vsp[0].int_value),(short)1);
	}
    break;

  case 255:
#line 2376 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.frac_value) = CreateFraction((short)(zz_vsp[-3].int_value),(short)(zz_vsp[-1].int_value));
	}
    break;

  case 256:
#line 2383 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.int_value) = 0;
	}
    break;

  case 257:
#line 2387 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.int_value) = 0;
	}
    break;

  case 258:
#line 2391 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.int_value) = 1;
	}
    break;

  case 259:
#line 2398 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateIntExpr((zz_vsp[0].int_value));
	}
    break;

  case 260:
#line 2402 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateIntExpr(LONG_MAX-1);
	}
    break;

  case 261:
#line 2406 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateRealExpr((zz_vsp[0].real_value),g_dim_ptr);
	}
    break;

  case 262:
#line 2410 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
    break;

  case 263:
#line 2414 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateTrueExpr();
	}
    break;

  case 264:
#line 2418 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateFalseExpr();
	}
    break;

  case 265:
#line 2422 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateAnyExpr();
	}
    break;

  case 266:
#line 2426 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateSymbolExpr((zz_vsp[0].sym_ptr));
	}
    break;

  case 267:
#line 2430 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateVarExpr((zz_vsp[0].nptr));
	}
    break;

  case 268:
#line 2434 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateSetExpr((zz_vsp[-1].sptr));
	}
    break;

  case 269:
#line 2438 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_plus));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 270:
#line 2443 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_minus));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 271:
#line 2448 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_times));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 272:
#line 2453 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_divide));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 273:
#line 2458 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_power));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 274:
#line 2463 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_and));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 275:
#line 2468 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_or));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 276:
#line 2473 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_not));
	}
    break;

  case 277:
#line 2477 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),(zz_vsp[-1].eptr));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 278:
#line 2482 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),(zz_vsp[-1].eptr));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 279:
#line 2487 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_in));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 280:
#line 2492 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_st));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 281:
#line 2497 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_vsp[0].eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_st));
	  (zz_val.eptr) = JoinExprLists((zz_vsp[-2].eptr),(zz_vsp[0].eptr));
	}
    break;

  case 282:
#line 2502 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = (zz_vsp[0].eptr);
	}
    break;

  case 283:
#line 2506 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = JoinExprLists((zz_vsp[0].eptr),CreateOpExpr(e_uminus));
	}
    break;

  case 284:
#line 2510 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateSatisfiedExpr((zz_vsp[-3].nptr),(zz_vsp[-1].real_value),g_dim_ptr);
	}
    break;

  case 285:
#line 2514 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateSatisfiedExpr((zz_vsp[-1].nptr),DBL_MAX,NULL);
	}
    break;

  case 286:
#line 2518 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroySetList((zz_vsp[-1].sptr));
	  (zz_val.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
    break;

  case 287:
#line 2525 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateBuiltin(e_sum,(zz_vsp[-1].sptr));
	}
    break;

  case 288:
#line 2529 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroySetList((zz_vsp[-1].sptr));
	  (zz_val.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
    break;

  case 289:
#line 2536 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateBuiltin(e_prod,(zz_vsp[-1].sptr));
	}
    break;

  case 290:
#line 2540 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroySetList((zz_vsp[-1].sptr));
	  (zz_val.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
    break;

  case 291:
#line 2547 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateBuiltin(e_union,(zz_vsp[-1].sptr));
	}
    break;

  case 292:
#line 2551 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroySetList((zz_vsp[-1].sptr));
	  (zz_val.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
    break;

  case 293:
#line 2558 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateBuiltin(e_inter,(zz_vsp[-1].sptr));
	}
    break;

  case 294:
#line 2562 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroySetList((zz_vsp[-1].sptr));
	  (zz_val.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
    break;

  case 295:
#line 2569 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateBuiltin(e_card,(zz_vsp[-1].sptr));
	}
    break;

  case 296:
#line 2573 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  DestroySetList((zz_vsp[-1].sptr));
	  (zz_val.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
    break;

  case 297:
#line 2580 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateBuiltin(e_choice,(zz_vsp[-1].sptr));
	}
    break;

  case 298:
#line 2584 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  CONST struct Func *fptr;
	  if ((fptr = LookupFunc(SCP((zz_vsp[-3].id_ptr))))!=NULL) {
	    (zz_val.eptr) = JoinExprLists((zz_vsp[-1].eptr),CreateFuncExpr(fptr));
	  } else {
	    (zz_val.eptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Function '%s' is not defined.",SCP((zz_vsp[-3].id_ptr)));
	    g_untrapped_error++;
	  }
	}
    break;

  case 299:
#line 2595 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = (zz_vsp[-1].eptr);
	}
    break;

  case 300:
#line 2602 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_equal);
	}
    break;

  case 301:
#line 2606 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_less);
	}
    break;

  case 302:
#line 2610 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_greater);
	}
    break;

  case 303:
#line 2614 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_lesseq);
	}
    break;

  case 304:
#line 2618 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_greatereq);
	}
    break;

  case 305:
#line 2622 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_notequal);
	}
    break;

  case 306:
#line 2629 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_boolean_eq);
	}
    break;

  case 307:
#line 2633 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"
    {
	  (zz_val.eptr) = CreateOpExpr(e_boolean_neq);
	}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 4884 "y.tab.c"

  zz_vsp -= zz_len;
  zz_ssp -= zz_len;


  ZZ__STACK_PRINT (zz_ss, zz_ssp);

  *++zz_vsp = zz_val;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  zz_n = zz_r1[zz_n];

  zz_state = zz_pgoto[zz_n - ZZ_NTOKENS] + *zz_ssp;
  if (0 <= zz_state && zz_state <= ZZ_LAST && zz_check[zz_state] == *zz_ssp)
    zz_state = zz_table[zz_state];
  else
    zz_state = zz_defgoto[zz_n - ZZ_NTOKENS];

  goto zz_newstate;


/*------------------------------------.
| zz_errlab -- here on detecting error |
`------------------------------------*/
zz_errlab:
  /* If not already recovering from an error, report this error.  */
  if (!zz_errstatus)
    {
      ++zz_nerrs;
#if ZZ_ERROR_VERBOSE
      zz_n = zz_pact[zz_state];

      if (ZZ_PACT_NINF < zz_n && zz_n < ZZ_LAST)
	{
	  ZZ_SIZE_T zz_size = 0;
	  int zz_type = ZZ_TRANSLATE (zz_char);
	  const char* zz_prefix;
	  char *zz_msg;
	  int zz_x;

	  /* Start ZZ_X at -ZZ_N if negative to avoid negative indexes in
	     ZZ_CHECK.  */
	  int zz_xbegin = zz_n < 0 ? -zz_n : 0;

	  /* Stay within bounds of both zz_check and zz_tname.  */
	  int zz_checklim = ZZ_LAST - zz_n;
	  int zz_xend = zz_checklim < ZZ_NTOKENS ? zz_checklim : ZZ_NTOKENS;
	  int zz_count = 0;

	  zz_prefix = ", expecting ";
	  for (zz_x = zz_xbegin; zz_x < zz_xend; ++zz_x)
	    if (zz_check[zz_x + zz_n] == zz_x && zz_x != ZZ_TERROR)
	      {
		zz_size += zz_strlen (zz_prefix) + zz_strlen (zz_tname [zz_x]);
		zz_count += 1;
		if (zz_count == 5)
		  {
		    zz_size = 0;
		    break;
		  }
	      }
	  zz_size += (sizeof ("syntax error, unexpected ")
		     + zz_strlen (zz_tname[zz_type]));
	  zz_msg = (char *) ZZ_STACK_ALLOC (zz_size);
	  if (zz_msg != 0)
	    {
	      char *zz_p = zz_stpcpy (zz_msg, "syntax error, unexpected ");
	      zz_p = zz_stpcpy (zz_p, zz_tname[zz_type]);

	      if (zz_count < 5)
		{
		  zz_prefix = ", expecting ";
		  for (zz_x = zz_xbegin; zz_x < zz_xend; ++zz_x)
		    if (zz_check[zz_x + zz_n] == zz_x && zz_x != ZZ_TERROR)
		      {
			zz_p = zz_stpcpy (zz_p, zz_prefix);
			zz_p = zz_stpcpy (zz_p, zz_tname[zz_x]);
			zz_prefix = " or ";
		      }
		}
	      zz_error (zz_msg);
	      ZZ_STACK_FREE (zz_msg);
	    }
	  else
	    zz_error ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* ZZ_ERROR_VERBOSE */
	zz_error ("syntax error");
    }



  if (zz_errstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (zz_char <= ZZ_EOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (zz_char == ZZ_EOF)
	     for (;;)
	       {

		 ZZ_POPSTACK;
		 if (zz_ssp == zz_ss)
		   ZZ_ABORT;
		 zz_destruct ("Error: popping",
                             zz_stos[*zz_ssp], zz_vsp);
	       }
        }
      else
	{
	  zz_destruct ("Error: discarding", zz_token, &zz_lval);
	  zz_char = ZZ_EMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto zz_errlab1;


/*---------------------------------------------------.
| zz_errorlab -- error raised explicitly by ZZ_ERROR.  |
`---------------------------------------------------*/
zz_errorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes ZZ_ERROR and the label
     zz_errorlab therefore never appears in user code.  */
  if (0)
     goto zz_errorlab;
#endif

zz_vsp -= zz_len;
  zz_ssp -= zz_len;
  zz_state = *zz_ssp;
  goto zz_errlab1;


/*-------------------------------------------------------------.
| zz_errlab1 -- common code for both syntax error and ZZ_ERROR.  |
`-------------------------------------------------------------*/
zz_errlab1:
  zz_errstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      zz_n = zz_pact[zz_state];
      if (zz_n != ZZ_PACT_NINF)
	{
	  zz_n += ZZ_TERROR;
	  if (0 <= zz_n && zz_n <= ZZ_LAST && zz_check[zz_n] == ZZ_TERROR)
	    {
	      zz_n = zz_table[zz_n];
	      if (0 < zz_n)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (zz_ssp == zz_ss)
	ZZ_ABORT;


      zz_destruct ("Error: popping", zz_stos[zz_state], zz_vsp);
      ZZ_POPSTACK;
      zz_state = *zz_ssp;
      ZZ__STACK_PRINT (zz_ss, zz_ssp);
    }

  if (zz_n == ZZ_FINAL)
    ZZ_ACCEPT;

  *++zz_vsp = zz_lval;


  /* Shift the error token. */
  ZZ__SYMBOL_PRINT ("Shifting", zz_stos[zz_n], zz_vsp, zz_lsp);

  zz_state = zz_n;
  goto zz_newstate;


/*-------------------------------------.
| zz_acceptlab -- ZZ_ACCEPT comes here.  |
`-------------------------------------*/
zz_acceptlab:
  zz_result = 0;
  goto zz_return;

/*-----------------------------------.
| zz_abortlab -- ZZ_ABORT comes here.  |
`-----------------------------------*/
zz_abortlab:
  zz_destruct ("Error: discarding lookahead",
              zz_token, &zz_lval);
  zz_char = ZZ_EMPTY;
  zz_result = 1;
  goto zz_return;

#ifndef zz_overflow
/*----------------------------------------------.
| zz_overflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
zz_overflowlab:
  zz_error ("parser stack overflow");
  zz_result = 2;
  /* Fall through.  */
#endif

zz_return:
#ifndef zz_overflow
  if (zz_ss != zz_ssa)
    ZZ_STACK_FREE (zz_ss);
#endif
  return zz_result;
}


#line 2637 "/home/ballan/cmu/trunk/base/autotools/../generic/compiler/ascParse.y"

/*
 * We really need to do something about freeing up the productions
 * that invoke this so we don't leak memory like a seive.
 * for example  z[i IN [1..2]][j IN [process[i]] IS_A mass; eats a ton.
 */
int
zz_error(char *s)
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

