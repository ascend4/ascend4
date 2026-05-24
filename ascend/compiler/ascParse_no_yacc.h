/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_ZZ_ASCEND_COMPILER_ASCPARSE_H_INCLUDED
# define YY_ZZ_ASCEND_COMPILER_ASCPARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int zz_debug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ADD_TOK = 258,                 /* ADD_TOK  */
    ALIASES_TOK = 259,             /* ALIASES_TOK  */
    AND_TOK = 260,                 /* AND_TOK  */
    ANY_TOK = 261,                 /* ANY_TOK  */
    APPLIES_TOK = 262,             /* APPLIES_TOK  */
    AREALIKE_TOK = 263,            /* AREALIKE_TOK  */
    ARETHESAME_TOK = 264,          /* ARETHESAME_TOK  */
    ARRAY_TOK = 265,               /* ARRAY_TOK  */
    ASSERT_TOK = 266,              /* ASSERT_TOK  */
    ATOM_TOK = 267,                /* ATOM_TOK  */
    BEQ_TOK = 268,                 /* BEQ_TOK  */
    BNE_TOK = 269,                 /* BNE_TOK  */
    BREAK_TOK = 270,               /* BREAK_TOK  */
    CALL_TOK = 271,                /* CALL_TOK  */
    CARD_TOK = 272,                /* CARD_TOK  */
    CASE_TOK = 273,                /* CASE_TOK  */
    CHOICE_TOK = 274,              /* CHOICE_TOK  */
    CHECK_TOK = 275,               /* CHECK_TOK  */
    CONDITIONAL_TOK = 276,         /* CONDITIONAL_TOK  */
    CONSTANT_TOK = 277,            /* CONSTANT_TOK  */
    CONTINUE_TOK = 278,            /* CONTINUE_TOK  */
    CREATE_TOK = 279,              /* CREATE_TOK  */
    DATA_TOK = 280,                /* DATA_TOK  */
    DECREASING_TOK = 281,          /* DECREASING_TOK  */
    DEFAULT_TOK = 282,             /* DEFAULT_TOK  */
    DEFINITION_TOK = 283,          /* DEFINITION_TOK  */
    DELETE_TOK = 284,              /* DELETE_TOK  */
    DERIV_TOK = 285,               /* DERIV_TOK  */
    DERLINK_TOK = 286,             /* DERLINK_TOK  */
    DIMENSION_TOK = 287,           /* DIMENSION_TOK  */
    DIMENSIONLESS_TOK = 288,       /* DIMENSIONLESS_TOK  */
    DO_TOK = 289,                  /* DO_TOK  */
    ELSE_TOK = 290,                /* ELSE_TOK  */
    END_TOK = 291,                 /* END_TOK  */
    EXPECT_TOK = 292,              /* EXPECT_TOK  */
    EXTERNAL_TOK = 293,            /* EXTERNAL_TOK  */
    FALSE_TOK = 294,               /* FALSE_TOK  */
    FALLTHRU_TOK = 295,            /* FALLTHRU_TOK  */
    FIX_TOK = 296,                 /* FIX_TOK  */
    FOR_TOK = 297,                 /* FOR_TOK  */
    FREE_TOK = 298,                /* FREE_TOK  */
    FROM_TOK = 299,                /* FROM_TOK  */
    FILE_TOK = 300,                /* FILE_TOK  */
    GLOBAL_TOK = 301,              /* GLOBAL_TOK  */
    IF_TOK = 302,                  /* IF_TOK  */
    IGNORE_TOK = 303,              /* IGNORE_TOK  */
    IMPORT_TOK = 304,              /* IMPORT_TOK  */
    IN_TOK = 305,                  /* IN_TOK  */
    INITIAL_TOK = 306,             /* INITIAL_TOK  */
    INPUT_TOK = 307,               /* INPUT_TOK  */
    INCREASING_TOK = 308,          /* INCREASING_TOK  */
    INTERACTIVE_TOK = 309,         /* INTERACTIVE_TOK  */
    INDEPENDENT_TOK = 310,         /* INDEPENDENT_TOK  */
    INTEGRATE_TOK = 311,           /* INTEGRATE_TOK  */
    INTEGRATOR_TOK = 312,          /* INTEGRATOR_TOK  */
    INTERSECTION_TOK = 313,        /* INTERSECTION_TOK  */
    ISA_TOK = 314,                 /* ISA_TOK  */
    _IS_T = 315,                   /* _IS_T  */
    ISREFINEDTO_TOK = 316,         /* ISREFINEDTO_TOK  */
    AS_TOK = 317,                  /* AS_TOK  */
    LINEAR_TOK = 318,              /* LINEAR_TOK  */
    LOG_TOK = 319,                 /* LOG_TOK  */
    NOW_TOK = 320,                 /* NOW_TOK  */
    LINK_TOK = 321,                /* LINK_TOK  */
    MAXIMIZE_TOK = 322,            /* MAXIMIZE_TOK  */
    MAXINTEGER_TOK = 323,          /* MAXINTEGER_TOK  */
    MAXREAL_TOK = 324,             /* MAXREAL_TOK  */
    METHODS_TOK = 325,             /* METHODS_TOK  */
    METHOD_TOK = 326,              /* METHOD_TOK  */
    MINIMIZE_TOK = 327,            /* MINIMIZE_TOK  */
    MODEL_TOK = 328,               /* MODEL_TOK  */
    NOT_TOK = 329,                 /* NOT_TOK  */
    NOTES_TOK = 330,               /* NOTES_TOK  */
    OBSERVE_TOK = 331,             /* OBSERVE_TOK  */
    OF_TOK = 332,                  /* OF_TOK  */
    OPTION_TOK = 333,              /* OPTION_TOK  */
    OR_TOK = 334,                  /* OR_TOK  */
    OTHERWISE_TOK = 335,           /* OTHERWISE_TOK  */
    OUTPUT_TOK = 336,              /* OUTPUT_TOK  */
    PROD_TOK = 337,                /* PROD_TOK  */
    PROVIDE_TOK = 338,             /* PROVIDE_TOK  */
    RATIO_TOK = 339,               /* RATIO_TOK  */
    REFINES_TOK = 340,             /* REFINES_TOK  */
    REPLACE_TOK = 341,             /* REPLACE_TOK  */
    REQUIRE_TOK = 342,             /* REQUIRE_TOK  */
    RETURN_TOK = 343,              /* RETURN_TOK  */
    RUN_TOK = 344,                 /* RUN_TOK  */
    REINIT_TOK = 345,              /* REINIT_TOK  */
    SATISFIED_TOK = 346,           /* SATISFIED_TOK  */
    SELECT_TOK = 347,              /* SELECT_TOK  */
    SIZE_TOK = 348,                /* SIZE_TOK  */
    SOLVE_TOK = 349,               /* SOLVE_TOK  */
    SOLVER_TOK = 350,              /* SOLVER_TOK  */
    STOP_TOK = 351,                /* STOP_TOK  */
    SUCHTHAT_TOK = 352,            /* SUCHTHAT_TOK  */
    SUM_TOK = 353,                 /* SUM_TOK  */
    SWITCH_TOK = 354,              /* SWITCH_TOK  */
    SYSTEM_TOK = 355,              /* SYSTEM_TOK  */
    STEP_TOK = 356,                /* STEP_TOK  */
    STEPS_TOK = 357,               /* STEPS_TOK  */
    STUDY_TOK = 358,               /* STUDY_TOK  */
    TABLE_TOK = 359,               /* TABLE_TOK  */
    VALUES_TOK = 360,              /* VALUES_TOK  */
    DATASET_TOK = 361,             /* DATASET_TOK  */
    POSITIONAL_TOK = 362,          /* POSITIONAL_TOK  */
    INDEX_TOK = 363,               /* INDEX_TOK  */
    COLUMN_TOK = 364,              /* COLUMN_TOK  */
    EOL_TOK = 365,                 /* EOL_TOK  */
    THEN_TOK = 366,                /* THEN_TOK  */
    TO_TOK = 367,                  /* TO_TOK  */
    TRUE_TOK = 368,                /* TRUE_TOK  */
    UNION_TOK = 369,               /* UNION_TOK  */
    UNITS_TOK = 370,               /* UNITS_TOK  */
    LADDER_TOK = 371,              /* LADDER_TOK  */
    UNIVERSAL_TOK = 372,           /* UNIVERSAL_TOK  */
    UNLINK_TOK = 373,              /* UNLINK_TOK  */
    VARY_TOK = 374,                /* VARY_TOK  */
    WHEN_TOK = 375,                /* WHEN_TOK  */
    WHERE_TOK = 376,               /* WHERE_TOK  */
    WHILE_TOK = 377,               /* WHILE_TOK  */
    WILLBE_TOK = 378,              /* WILLBE_TOK  */
    WILLBETHESAME_TOK = 379,       /* WILLBETHESAME_TOK  */
    WILLNOTBETHESAME_TOK = 380,    /* WILLNOTBETHESAME_TOK  */
    ASSIGN_TOK = 381,              /* ASSIGN_TOK  */
    CASSIGN_TOK = 382,             /* CASSIGN_TOK  */
    DBLCOLON_TOK = 383,            /* DBLCOLON_TOK  */
    USE_TOK = 384,                 /* USE_TOK  */
    LEQ_TOK = 385,                 /* LEQ_TOK  */
    GEQ_TOK = 386,                 /* GEQ_TOK  */
    NEQ_TOK = 387,                 /* NEQ_TOK  */
    DOTDOT_TOK = 388,              /* DOTDOT_TOK  */
    WITH_TOK = 389,                /* WITH_TOK  */
    VALUE_TOK = 390,               /* VALUE_TOK  */
    WITH_VALUE_T = 391,            /* WITH_VALUE_T  */
    REAL_TOK = 392,                /* REAL_TOK  */
    INTEGER_TOK = 393,             /* INTEGER_TOK  */
    IDENTIFIER_TOK = 394,          /* IDENTIFIER_TOK  */
    BRACEDTEXT_TOK = 395,          /* BRACEDTEXT_TOK  */
    SYMBOL_TOK = 396,              /* SYMBOL_TOK  */
    DQUOTE_TOK = 397,              /* DQUOTE_TOK  */
    UMINUS_TOK = 398,              /* UMINUS_TOK  */
    UPLUS_TOK = 399                /* UPLUS_TOK  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define ADD_TOK 258
#define ALIASES_TOK 259
#define AND_TOK 260
#define ANY_TOK 261
#define APPLIES_TOK 262
#define AREALIKE_TOK 263
#define ARETHESAME_TOK 264
#define ARRAY_TOK 265
#define ASSERT_TOK 266
#define ATOM_TOK 267
#define BEQ_TOK 268
#define BNE_TOK 269
#define BREAK_TOK 270
#define CALL_TOK 271
#define CARD_TOK 272
#define CASE_TOK 273
#define CHOICE_TOK 274
#define CHECK_TOK 275
#define CONDITIONAL_TOK 276
#define CONSTANT_TOK 277
#define CONTINUE_TOK 278
#define CREATE_TOK 279
#define DATA_TOK 280
#define DECREASING_TOK 281
#define DEFAULT_TOK 282
#define DEFINITION_TOK 283
#define DELETE_TOK 284
#define DERIV_TOK 285
#define DERLINK_TOK 286
#define DIMENSION_TOK 287
#define DIMENSIONLESS_TOK 288
#define DO_TOK 289
#define ELSE_TOK 290
#define END_TOK 291
#define EXPECT_TOK 292
#define EXTERNAL_TOK 293
#define FALSE_TOK 294
#define FALLTHRU_TOK 295
#define FIX_TOK 296
#define FOR_TOK 297
#define FREE_TOK 298
#define FROM_TOK 299
#define FILE_TOK 300
#define GLOBAL_TOK 301
#define IF_TOK 302
#define IGNORE_TOK 303
#define IMPORT_TOK 304
#define IN_TOK 305
#define INITIAL_TOK 306
#define INPUT_TOK 307
#define INCREASING_TOK 308
#define INTERACTIVE_TOK 309
#define INDEPENDENT_TOK 310
#define INTEGRATE_TOK 311
#define INTEGRATOR_TOK 312
#define INTERSECTION_TOK 313
#define ISA_TOK 314
#define _IS_T 315
#define ISREFINEDTO_TOK 316
#define AS_TOK 317
#define LINEAR_TOK 318
#define LOG_TOK 319
#define NOW_TOK 320
#define LINK_TOK 321
#define MAXIMIZE_TOK 322
#define MAXINTEGER_TOK 323
#define MAXREAL_TOK 324
#define METHODS_TOK 325
#define METHOD_TOK 326
#define MINIMIZE_TOK 327
#define MODEL_TOK 328
#define NOT_TOK 329
#define NOTES_TOK 330
#define OBSERVE_TOK 331
#define OF_TOK 332
#define OPTION_TOK 333
#define OR_TOK 334
#define OTHERWISE_TOK 335
#define OUTPUT_TOK 336
#define PROD_TOK 337
#define PROVIDE_TOK 338
#define RATIO_TOK 339
#define REFINES_TOK 340
#define REPLACE_TOK 341
#define REQUIRE_TOK 342
#define RETURN_TOK 343
#define RUN_TOK 344
#define REINIT_TOK 345
#define SATISFIED_TOK 346
#define SELECT_TOK 347
#define SIZE_TOK 348
#define SOLVE_TOK 349
#define SOLVER_TOK 350
#define STOP_TOK 351
#define SUCHTHAT_TOK 352
#define SUM_TOK 353
#define SWITCH_TOK 354
#define SYSTEM_TOK 355
#define STEP_TOK 356
#define STEPS_TOK 357
#define STUDY_TOK 358
#define TABLE_TOK 359
#define VALUES_TOK 360
#define DATASET_TOK 361
#define POSITIONAL_TOK 362
#define INDEX_TOK 363
#define COLUMN_TOK 364
#define EOL_TOK 365
#define THEN_TOK 366
#define TO_TOK 367
#define TRUE_TOK 368
#define UNION_TOK 369
#define UNITS_TOK 370
#define LADDER_TOK 371
#define UNIVERSAL_TOK 372
#define UNLINK_TOK 373
#define VARY_TOK 374
#define WHEN_TOK 375
#define WHERE_TOK 376
#define WHILE_TOK 377
#define WILLBE_TOK 378
#define WILLBETHESAME_TOK 379
#define WILLNOTBETHESAME_TOK 380
#define ASSIGN_TOK 381
#define CASSIGN_TOK 382
#define DBLCOLON_TOK 383
#define USE_TOK 384
#define LEQ_TOK 385
#define GEQ_TOK 386
#define NEQ_TOK 387
#define DOTDOT_TOK 388
#define WITH_TOK 389
#define VALUE_TOK 390
#define WITH_VALUE_T 391
#define REAL_TOK 392
#define INTEGER_TOK 393
#define IDENTIFIER_TOK 394
#define BRACEDTEXT_TOK 395
#define SYMBOL_TOK 396
#define DQUOTE_TOK 397
#define UMINUS_TOK 398
#define UPLUS_TOK 399

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 744 "ascend/compiler/ascParse.y"

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
  struct UnitLadderItem *ulitemptr;
  dim_type dimen;
  enum ForOrder order;
  enum ForKind fkind;

#line 384 "ascend/compiler/ascParse.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE zz_lval;


int zz_parse (void);


#endif /* !YY_ZZ_ASCEND_COMPILER_ASCPARSE_H_INCLUDED  */
