/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

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




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 304 "base/generic/compiler/ascParse.y"
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
/* Line 1447 of yacc.c.  */
#line 284 "base/generic/compiler/ascParse.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE zz_lval;



