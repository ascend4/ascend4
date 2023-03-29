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
    DER_TOK = 283,                 /* DER_TOK  */
    DIMENSION_TOK = 284,           /* DIMENSION_TOK  */
    DIMENSIONLESS_TOK = 285,       /* DIMENSIONLESS_TOK  */
    DO_TOK = 286,                  /* DO_TOK  */
    ELSE_TOK = 287,                /* ELSE_TOK  */
    END_TOK = 288,                 /* END_TOK  */
    EXPECT_TOK = 289,              /* EXPECT_TOK  */
    EXTERNAL_TOK = 290,            /* EXTERNAL_TOK  */
    FALSE_TOK = 291,               /* FALSE_TOK  */
    FALLTHRU_TOK = 292,            /* FALLTHRU_TOK  */
    FIX_TOK = 293,                 /* FIX_TOK  */
    FOR_TOK = 294,                 /* FOR_TOK  */
    FREE_TOK = 295,                /* FREE_TOK  */
    FROM_TOK = 296,                /* FROM_TOK  */
    GLOBAL_TOK = 297,              /* GLOBAL_TOK  */
    IF_TOK = 298,                  /* IF_TOK  */
    IGNORE_TOK = 299,              /* IGNORE_TOK  */
    IMPORT_TOK = 300,              /* IMPORT_TOK  */
    IN_TOK = 301,                  /* IN_TOK  */
    INPUT_TOK = 302,               /* INPUT_TOK  */
    INCREASING_TOK = 303,          /* INCREASING_TOK  */
    INTERACTIVE_TOK = 304,         /* INTERACTIVE_TOK  */
    INDEPENDENT_TOK = 305,         /* INDEPENDENT_TOK  */
    INTERSECTION_TOK = 306,        /* INTERSECTION_TOK  */
    ISA_TOK = 307,                 /* ISA_TOK  */
    _IS_T = 308,                   /* _IS_T  */
    ISREFINEDTO_TOK = 309,         /* ISREFINEDTO_TOK  */
    LINK_TOK = 310,                /* LINK_TOK  */
    MAXIMIZE_TOK = 311,            /* MAXIMIZE_TOK  */
    MAXINTEGER_TOK = 312,          /* MAXINTEGER_TOK  */
    MAXREAL_TOK = 313,             /* MAXREAL_TOK  */
    METHODS_TOK = 314,             /* METHODS_TOK  */
    METHOD_TOK = 315,              /* METHOD_TOK  */
    MINIMIZE_TOK = 316,            /* MINIMIZE_TOK  */
    MODEL_TOK = 317,               /* MODEL_TOK  */
    NOT_TOK = 318,                 /* NOT_TOK  */
    NOTES_TOK = 319,               /* NOTES_TOK  */
    OF_TOK = 320,                  /* OF_TOK  */
    OPTION_TOK = 321,              /* OPTION_TOK  */
    OR_TOK = 322,                  /* OR_TOK  */
    OTHERWISE_TOK = 323,           /* OTHERWISE_TOK  */
    OUTPUT_TOK = 324,              /* OUTPUT_TOK  */
    PATCH_TOK = 325,               /* PATCH_TOK  */
    PROD_TOK = 326,                /* PROD_TOK  */
    PROVIDE_TOK = 327,             /* PROVIDE_TOK  */
    REFINES_TOK = 328,             /* REFINES_TOK  */
    REPLACE_TOK = 329,             /* REPLACE_TOK  */
    REQUIRE_TOK = 330,             /* REQUIRE_TOK  */
    RETURN_TOK = 331,              /* RETURN_TOK  */
    RUN_TOK = 332,                 /* RUN_TOK  */
    SATISFIED_TOK = 333,           /* SATISFIED_TOK  */
    SELECT_TOK = 334,              /* SELECT_TOK  */
    SIZE_TOK = 335,                /* SIZE_TOK  */
    SOLVE_TOK = 336,               /* SOLVE_TOK  */
    SOLVER_TOK = 337,              /* SOLVER_TOK  */
    STOP_TOK = 338,                /* STOP_TOK  */
    SUCHTHAT_TOK = 339,            /* SUCHTHAT_TOK  */
    SUM_TOK = 340,                 /* SUM_TOK  */
    SWITCH_TOK = 341,              /* SWITCH_TOK  */
    THEN_TOK = 342,                /* THEN_TOK  */
    TRUE_TOK = 343,                /* TRUE_TOK  */
    UNION_TOK = 344,               /* UNION_TOK  */
    UNITS_TOK = 345,               /* UNITS_TOK  */
    UNIVERSAL_TOK = 346,           /* UNIVERSAL_TOK  */
    UNLINK_TOK = 347,              /* UNLINK_TOK  */
    WHEN_TOK = 348,                /* WHEN_TOK  */
    WHERE_TOK = 349,               /* WHERE_TOK  */
    WHILE_TOK = 350,               /* WHILE_TOK  */
    WILLBE_TOK = 351,              /* WILLBE_TOK  */
    WILLBETHESAME_TOK = 352,       /* WILLBETHESAME_TOK  */
    WILLNOTBETHESAME_TOK = 353,    /* WILLNOTBETHESAME_TOK  */
    ASSIGN_TOK = 354,              /* ASSIGN_TOK  */
    CASSIGN_TOK = 355,             /* CASSIGN_TOK  */
    DBLCOLON_TOK = 356,            /* DBLCOLON_TOK  */
    USE_TOK = 357,                 /* USE_TOK  */
    LEQ_TOK = 358,                 /* LEQ_TOK  */
    GEQ_TOK = 359,                 /* GEQ_TOK  */
    NEQ_TOK = 360,                 /* NEQ_TOK  */
    DOTDOT_TOK = 361,              /* DOTDOT_TOK  */
    WITH_TOK = 362,                /* WITH_TOK  */
    VALUE_TOK = 363,               /* VALUE_TOK  */
    WITH_VALUE_T = 364,            /* WITH_VALUE_T  */
    REAL_TOK = 365,                /* REAL_TOK  */
    INTEGER_TOK = 366,             /* INTEGER_TOK  */
    IDENTIFIER_TOK = 367,          /* IDENTIFIER_TOK  */
    BRACEDTEXT_TOK = 368,          /* BRACEDTEXT_TOK  */
    SYMBOL_TOK = 369,              /* SYMBOL_TOK  */
    DQUOTE_TOK = 370,              /* DQUOTE_TOK  */
    UMINUS_TOK = 371,              /* UMINUS_TOK  */
    UPLUS_TOK = 372                /* UPLUS_TOK  */
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
#define THEN_TOK 342
#define TRUE_TOK 343
#define UNION_TOK 344
#define UNITS_TOK 345
#define UNIVERSAL_TOK 346
#define UNLINK_TOK 347
#define WHEN_TOK 348
#define WHERE_TOK 349
#define WHILE_TOK 350
#define WILLBE_TOK 351
#define WILLBETHESAME_TOK 352
#define WILLNOTBETHESAME_TOK 353
#define ASSIGN_TOK 354
#define CASSIGN_TOK 355
#define DBLCOLON_TOK 356
#define USE_TOK 357
#define LEQ_TOK 358
#define GEQ_TOK 359
#define NEQ_TOK 360
#define DOTDOT_TOK 361
#define WITH_TOK 362
#define VALUE_TOK 363
#define WITH_VALUE_T 364
#define REAL_TOK 365
#define INTEGER_TOK 366
#define IDENTIFIER_TOK 367
#define BRACEDTEXT_TOK 368
#define SYMBOL_TOK 369
#define DQUOTE_TOK 370
#define UMINUS_TOK 371
#define UPLUS_TOK 372

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 302 "ascend/compiler/ascParse.y"

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

#line 329 "ascend/compiler/ascParse.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE zz_lval;


int zz_parse (void);


#endif /* !YY_ZZ_ASCEND_COMPILER_ASCPARSE_H_INCLUDED  */
