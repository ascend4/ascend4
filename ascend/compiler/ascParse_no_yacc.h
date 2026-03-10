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

#ifndef YY_ZZ_ASCPARSE_H_INCLUDED
# define YY_ZZ_ASCPARSE_H_INCLUDED
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
    AREALIKE_TOK = 262,            /* AREALIKE_TOK  */
    ARETHESAME_TOK = 263,          /* ARETHESAME_TOK  */
    ARRAY_TOK = 264,               /* ARRAY_TOK  */
    ASSERT_TOK = 265,              /* ASSERT_TOK  */
    ATOM_TOK = 266,                /* ATOM_TOK  */
    BEQ_TOK = 267,                 /* BEQ_TOK  */
    BNE_TOK = 268,                 /* BNE_TOK  */
    BREAK_TOK = 269,               /* BREAK_TOK  */
    CALL_TOK = 270,                /* CALL_TOK  */
    CARD_TOK = 271,                /* CARD_TOK  */
    CASE_TOK = 272,                /* CASE_TOK  */
    CHOICE_TOK = 273,              /* CHOICE_TOK  */
    CHECK_TOK = 274,               /* CHECK_TOK  */
    CONDITIONAL_TOK = 275,         /* CONDITIONAL_TOK  */
    CONSTANT_TOK = 276,            /* CONSTANT_TOK  */
    CONTINUE_TOK = 277,            /* CONTINUE_TOK  */
    CREATE_TOK = 278,              /* CREATE_TOK  */
    DATA_TOK = 279,                /* DATA_TOK  */
    DECREASING_TOK = 280,          /* DECREASING_TOK  */
    DEFAULT_TOK = 281,             /* DEFAULT_TOK  */
    DEFINITION_TOK = 282,          /* DEFINITION_TOK  */
    DELETE_TOK = 283,              /* DELETE_TOK  */
    DER_TOK = 284,                 /* DER_TOK  */
    DIMENSION_TOK = 285,           /* DIMENSION_TOK  */
    DIMENSIONLESS_TOK = 286,       /* DIMENSIONLESS_TOK  */
    DO_TOK = 287,                  /* DO_TOK  */
    ELSE_TOK = 288,                /* ELSE_TOK  */
    END_TOK = 289,                 /* END_TOK  */
    EXPECT_TOK = 290,              /* EXPECT_TOK  */
    EXTERNAL_TOK = 291,            /* EXTERNAL_TOK  */
    FALSE_TOK = 292,               /* FALSE_TOK  */
    FALLTHRU_TOK = 293,            /* FALLTHRU_TOK  */
    FIX_TOK = 294,                 /* FIX_TOK  */
    FOR_TOK = 295,                 /* FOR_TOK  */
    FREE_TOK = 296,                /* FREE_TOK  */
    FROM_TOK = 297,                /* FROM_TOK  */
    FILE_TOK = 298,                /* FILE_TOK  */
    GLOBAL_TOK = 299,              /* GLOBAL_TOK  */
    IF_TOK = 300,                  /* IF_TOK  */
    IGNORE_TOK = 301,              /* IGNORE_TOK  */
    IMPORT_TOK = 302,              /* IMPORT_TOK  */
    IN_TOK = 303,                  /* IN_TOK  */
    INPUT_TOK = 304,               /* INPUT_TOK  */
    INCREASING_TOK = 305,          /* INCREASING_TOK  */
    INTERACTIVE_TOK = 306,         /* INTERACTIVE_TOK  */
    INDEPENDENT_TOK = 307,         /* INDEPENDENT_TOK  */
    INTERSECTION_TOK = 308,        /* INTERSECTION_TOK  */
    ISA_TOK = 309,                 /* ISA_TOK  */
    _IS_T = 310,                   /* _IS_T  */
    ISREFINEDTO_TOK = 311,         /* ISREFINEDTO_TOK  */
    LINEAR_TOK = 312,              /* LINEAR_TOK  */
    LOG_TOK = 313,                 /* LOG_TOK  */
    NOW_TOK = 314,                 /* NOW_TOK  */
    LINK_TOK = 315,                /* LINK_TOK  */
    MAXIMIZE_TOK = 316,            /* MAXIMIZE_TOK  */
    MAXINTEGER_TOK = 317,          /* MAXINTEGER_TOK  */
    MAXREAL_TOK = 318,             /* MAXREAL_TOK  */
    METHODS_TOK = 319,             /* METHODS_TOK  */
    METHOD_TOK = 320,              /* METHOD_TOK  */
    MINIMIZE_TOK = 321,            /* MINIMIZE_TOK  */
    MODEL_TOK = 322,               /* MODEL_TOK  */
    NOT_TOK = 323,                 /* NOT_TOK  */
    NOTES_TOK = 324,               /* NOTES_TOK  */
    OF_TOK = 325,                  /* OF_TOK  */
    OPTION_TOK = 326,              /* OPTION_TOK  */
    OR_TOK = 327,                  /* OR_TOK  */
    OTHERWISE_TOK = 328,           /* OTHERWISE_TOK  */
    OUTPUT_TOK = 329,              /* OUTPUT_TOK  */
    PROD_TOK = 330,                /* PROD_TOK  */
    PROVIDE_TOK = 331,             /* PROVIDE_TOK  */
    RATIO_TOK = 332,               /* RATIO_TOK  */
    REFINES_TOK = 333,             /* REFINES_TOK  */
    REPLACE_TOK = 334,             /* REPLACE_TOK  */
    REQUIRE_TOK = 335,             /* REQUIRE_TOK  */
    RETURN_TOK = 336,              /* RETURN_TOK  */
    RUN_TOK = 337,                 /* RUN_TOK  */
    SATISFIED_TOK = 338,           /* SATISFIED_TOK  */
    SELECT_TOK = 339,              /* SELECT_TOK  */
    SIZE_TOK = 340,                /* SIZE_TOK  */
    SOLVE_TOK = 341,               /* SOLVE_TOK  */
    SOLVER_TOK = 342,              /* SOLVER_TOK  */
    STOP_TOK = 343,                /* STOP_TOK  */
    SUCHTHAT_TOK = 344,            /* SUCHTHAT_TOK  */
    SUM_TOK = 345,                 /* SUM_TOK  */
    SWITCH_TOK = 346,              /* SWITCH_TOK  */
    SYSTEM_TOK = 347,              /* SYSTEM_TOK  */
    STEP_TOK = 348,                /* STEP_TOK  */
    STEPS_TOK = 349,               /* STEPS_TOK  */
    STUDY_TOK = 350,               /* STUDY_TOK  */
    TABLE_TOK = 351,               /* TABLE_TOK  */
    VALUES_TOK = 352,              /* VALUES_TOK  */
    DATASET_TOK = 353,             /* DATASET_TOK  */
    POSITIONAL_TOK = 354,          /* POSITIONAL_TOK  */
    INDEX_TOK = 355,               /* INDEX_TOK  */
    COLUMN_TOK = 356,              /* COLUMN_TOK  */
    EOL_TOK = 357,                 /* EOL_TOK  */
    THEN_TOK = 358,                /* THEN_TOK  */
    TO_TOK = 359,                  /* TO_TOK  */
    TRUE_TOK = 360,                /* TRUE_TOK  */
    UNION_TOK = 361,               /* UNION_TOK  */
    UNITS_TOK = 362,               /* UNITS_TOK  */
    LADDER_TOK = 363,              /* LADDER_TOK  */
    UNIVERSAL_TOK = 364,           /* UNIVERSAL_TOK  */
    UNLINK_TOK = 365,              /* UNLINK_TOK  */
    VARY_TOK = 366,                /* VARY_TOK  */
    WHEN_TOK = 367,                /* WHEN_TOK  */
    WHERE_TOK = 368,               /* WHERE_TOK  */
    WHILE_TOK = 369,               /* WHILE_TOK  */
    WILLBE_TOK = 370,              /* WILLBE_TOK  */
    WILLBETHESAME_TOK = 371,       /* WILLBETHESAME_TOK  */
    WILLNOTBETHESAME_TOK = 372,    /* WILLNOTBETHESAME_TOK  */
    ASSIGN_TOK = 373,              /* ASSIGN_TOK  */
    CASSIGN_TOK = 374,             /* CASSIGN_TOK  */
    DBLCOLON_TOK = 375,            /* DBLCOLON_TOK  */
    USE_TOK = 376,                 /* USE_TOK  */
    LEQ_TOK = 377,                 /* LEQ_TOK  */
    GEQ_TOK = 378,                 /* GEQ_TOK  */
    NEQ_TOK = 379,                 /* NEQ_TOK  */
    DOTDOT_TOK = 380,              /* DOTDOT_TOK  */
    WITH_TOK = 381,                /* WITH_TOK  */
    VALUE_TOK = 382,               /* VALUE_TOK  */
    WITH_VALUE_T = 383,            /* WITH_VALUE_T  */
    REAL_TOK = 384,                /* REAL_TOK  */
    INTEGER_TOK = 385,             /* INTEGER_TOK  */
    IDENTIFIER_TOK = 386,          /* IDENTIFIER_TOK  */
    BRACEDTEXT_TOK = 387,          /* BRACEDTEXT_TOK  */
    SYMBOL_TOK = 388,              /* SYMBOL_TOK  */
    DQUOTE_TOK = 389,              /* DQUOTE_TOK  */
    UMINUS_TOK = 390,              /* UMINUS_TOK  */
    UPLUS_TOK = 391                /* UPLUS_TOK  */
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
#define DELETE_TOK 283
#define DER_TOK 284
#define DIMENSION_TOK 285
#define DIMENSIONLESS_TOK 286
#define DO_TOK 287
#define ELSE_TOK 288
#define END_TOK 289
#define EXPECT_TOK 290
#define EXTERNAL_TOK 291
#define FALSE_TOK 292
#define FALLTHRU_TOK 293
#define FIX_TOK 294
#define FOR_TOK 295
#define FREE_TOK 296
#define FROM_TOK 297
#define FILE_TOK 298
#define GLOBAL_TOK 299
#define IF_TOK 300
#define IGNORE_TOK 301
#define IMPORT_TOK 302
#define IN_TOK 303
#define INPUT_TOK 304
#define INCREASING_TOK 305
#define INTERACTIVE_TOK 306
#define INDEPENDENT_TOK 307
#define INTERSECTION_TOK 308
#define ISA_TOK 309
#define _IS_T 310
#define ISREFINEDTO_TOK 311
#define LINEAR_TOK 312
#define LOG_TOK 313
#define NOW_TOK 314
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
#define PROD_TOK 330
#define PROVIDE_TOK 331
#define RATIO_TOK 332
#define REFINES_TOK 333
#define REPLACE_TOK 334
#define REQUIRE_TOK 335
#define RETURN_TOK 336
#define RUN_TOK 337
#define SATISFIED_TOK 338
#define SELECT_TOK 339
#define SIZE_TOK 340
#define SOLVE_TOK 341
#define SOLVER_TOK 342
#define STOP_TOK 343
#define SUCHTHAT_TOK 344
#define SUM_TOK 345
#define SWITCH_TOK 346
#define SYSTEM_TOK 347
#define STEP_TOK 348
#define STEPS_TOK 349
#define STUDY_TOK 350
#define TABLE_TOK 351
#define VALUES_TOK 352
#define DATASET_TOK 353
#define POSITIONAL_TOK 354
#define INDEX_TOK 355
#define COLUMN_TOK 356
#define EOL_TOK 357
#define THEN_TOK 358
#define TO_TOK 359
#define TRUE_TOK 360
#define UNION_TOK 361
#define UNITS_TOK 362
#define LADDER_TOK 363
#define UNIVERSAL_TOK 364
#define UNLINK_TOK 365
#define VARY_TOK 366
#define WHEN_TOK 367
#define WHERE_TOK 368
#define WHILE_TOK 369
#define WILLBE_TOK 370
#define WILLBETHESAME_TOK 371
#define WILLNOTBETHESAME_TOK 372
#define ASSIGN_TOK 373
#define CASSIGN_TOK 374
#define DBLCOLON_TOK 375
#define USE_TOK 376
#define LEQ_TOK 377
#define GEQ_TOK 378
#define NEQ_TOK 379
#define DOTDOT_TOK 380
#define WITH_TOK 381
#define VALUE_TOK 382
#define WITH_VALUE_T 383
#define REAL_TOK 384
#define INTEGER_TOK 385
#define IDENTIFIER_TOK 386
#define BRACEDTEXT_TOK 387
#define SYMBOL_TOK 388
#define DQUOTE_TOK 389
#define UMINUS_TOK 390
#define UPLUS_TOK 391

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 741 "ascParse.y"

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

#line 368 "ascParse.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE zz_lval;


int zz_parse (void);


#endif /* !YY_ZZ_ASCPARSE_H_INCLUDED  */
