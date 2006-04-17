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




#if ! defined (ZZ_STYPE) && ! defined (ZZ_STYPE_IS_DECLARED)
#line 300 "/tmp/trunk/base/autotools/../generic/compiler/ascParse.y"
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
/* Line 1318 of yacc.c.  */
#line 283 "y.tab.h"
# define zz_stype ZZ_STYPE /* obsolescent; will be withdrawn */
# define ZZ_STYPE_IS_DECLARED 1
# define ZZ_STYPE_IS_TRIVIAL 1
#endif

extern ZZ_STYPE zz_lval;



