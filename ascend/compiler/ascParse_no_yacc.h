/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
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
/* Line 2058 of yacc.c  */
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


/* Line 2058 of yacc.c  */
#line 335 "ascend/compiler/ascParse.h"
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
