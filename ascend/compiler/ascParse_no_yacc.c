/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 29 "ascend/compiler/ascParse.y"

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

//#define ASCPARSE_DEBUG
#ifdef ASCPARSE_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif


int g_compiler_warnings = 1;		/* level of whine to allow */

#include <ascend/compiler/redirectFile.h>
#ifndef ASCERR
# error "ASCERR not defined"
#endif

//#define ASCPARSE_DEBUG
#ifdef ASCPARSE_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
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

struct table_parse_state {
  int active;
  int body_init;
  int positional;
  int row_has_items;
  symchar *decl_type;
  struct Set *decl_typeargs;
  symchar *decl_set_type;
  struct Expr *default_expr;
  Asc_DString body;
  unsigned long rows;
  unsigned long scalars;
  unsigned long items;
};

static struct table_parse_state g_table_parse = {0,0,0,0,NULL,NULL,NULL,NULL,{0},0,0,0};

struct dataset_parse_state {
  int active;
  symchar *name;
  char *filename;
  struct DatasetIndexItem *indices;
  struct DatasetMapItem *maps;
};

static struct dataset_parse_state g_dataset_parse = {0,NULL,NULL,NULL,NULL};

static void TableParseEnsureBody(void){
  if (!g_table_parse.body_init) {
    Asc_DStringInit(&g_table_parse.body);
    g_table_parse.body_init = 1;
  }
}

static void TableParseBegin(void){
  if (g_table_parse.decl_typeargs != NULL) {
    DestroySetList(g_table_parse.decl_typeargs);
    g_table_parse.decl_typeargs = NULL;
  }
  g_table_parse.decl_type = NULL;
  g_table_parse.decl_set_type = NULL;
  if (g_table_parse.default_expr != NULL) {
    DestroyExprList(g_table_parse.default_expr);
  }
  g_table_parse.default_expr = NULL;
  TableParseEnsureBody();
  Asc_DStringTrunc(&g_table_parse.body,0);
  g_table_parse.active = 1;
  g_table_parse.positional = 0;
  g_table_parse.row_has_items = 0;
  g_table_parse.rows = 0;
  g_table_parse.scalars = 0;
  g_table_parse.items = 0;
}

static void TableParseAbort(void){
  if (g_table_parse.decl_typeargs != NULL) {
    DestroySetList(g_table_parse.decl_typeargs);
    g_table_parse.decl_typeargs = NULL;
  }
  g_table_parse.decl_type = NULL;
  g_table_parse.decl_set_type = NULL;
  if (g_table_parse.default_expr != NULL) {
    DestroyExprList(g_table_parse.default_expr);
    g_table_parse.default_expr = NULL;
  }
  if (g_table_parse.body_init) {
    Asc_DStringTrunc(&g_table_parse.body,0);
  }
  g_table_parse.active = 0;
  g_table_parse.row_has_items = 0;
  g_table_parse.rows = 0;
  g_table_parse.scalars = 0;
  g_table_parse.items = 0;
  g_table_parse.positional = 0;
}

static void TableParseAppendToken(CONST char *tok, int scalar){
  if (!g_table_parse.active || tok == NULL) {
    return;
  }
  TableParseEnsureBody();
  if (g_table_parse.row_has_items) {
    Asc_DStringAppend(&g_table_parse.body," ",1);
  }
  Asc_DStringAppend(&g_table_parse.body,tok,-1);
  g_table_parse.items++;
  if (scalar) {
    g_table_parse.scalars++;
  }
  g_table_parse.row_has_items = 1;
}

static void TableParseAppendInteger(long v){
  char buf[64];
  snprintf(buf,sizeof(buf),"%ld",v);
  TableParseAppendToken(buf,1);
}

static void TableParseAppendReal(double v){
  char buf[64];
  snprintf(buf,sizeof(buf),"%.17g",v);
  TableParseAppendToken(buf,1);
}

static void TableParseAppendSymbol(symchar *sym){
  if (sym == NULL) {
    return;
  }
  TableParseEnsureBody();
  if (g_table_parse.row_has_items) {
    Asc_DStringAppend(&g_table_parse.body," ",1);
  }
  Asc_DStringAppend(&g_table_parse.body,"'",1);
  Asc_DStringAppend(&g_table_parse.body,SCP(sym),-1);
  Asc_DStringAppend(&g_table_parse.body,"'",1);
  g_table_parse.items++;
  g_table_parse.scalars++;
  g_table_parse.row_has_items = 1;
}

static void TableParseAppendBraced(CONST char *txt){
  if (txt == NULL) {
    return;
  }
  TableParseEnsureBody();
  if (g_table_parse.row_has_items) {
    Asc_DStringAppend(&g_table_parse.body," ",1);
  }
  Asc_DStringAppend(&g_table_parse.body,"{",1);
  Asc_DStringAppend(&g_table_parse.body,txt,-1);
  Asc_DStringAppend(&g_table_parse.body,"}",1);
  g_table_parse.items++;
  g_table_parse.scalars++;
  g_table_parse.row_has_items = 1;
}

static void TableParseEndRow(void){
  if (!g_table_parse.active) {
    return;
  }
  if (g_table_parse.row_has_items) {
    TableParseEnsureBody();
    Asc_DStringAppend(&g_table_parse.body,"\n",1);
    g_table_parse.rows++;
    g_table_parse.row_has_items = 0;
  }
}

static char *TableParseFinish(void){
  char *result;
  TableParseEndRow();
  TableParseEnsureBody();
  result = Asc_DStringResult(&g_table_parse.body);
  g_table_parse.active = 0;
  return result;
}

static void DatasetParseClear(void){
  struct DatasetIndexItem *idx = g_dataset_parse.indices;
  struct DatasetMapItem *map = g_dataset_parse.maps;
  while (idx != NULL) {
    struct DatasetIndexItem *next = idx->next;
    ASC_FREE(idx);
    idx = next;
  }
  while (map != NULL) {
    struct DatasetMapItem *next = map->next;
    if (map->target != NULL) {
      DestroyName(map->target);
    }
    if (map->units != NULL) {
      ascfree(map->units);
    }
    ASC_FREE(map);
    map = next;
  }
  if (g_dataset_parse.filename != NULL) {
    ascfree(g_dataset_parse.filename);
  }
  g_dataset_parse.active = 0;
  g_dataset_parse.name = NULL;
  g_dataset_parse.filename = NULL;
  g_dataset_parse.indices = NULL;
  g_dataset_parse.maps = NULL;
}

static void DatasetParseBegin(symchar *name, CONST char *filename){
  DatasetParseClear();
  g_dataset_parse.active = 1;
  g_dataset_parse.name = name;
  g_dataset_parse.filename = (filename != NULL) ? ASC_STRDUP(filename) : NULL;
}

static void DatasetParseAddIndex(symchar *set_name,
                                 symchar *column_name,
                                 symchar *type_name)
{
  struct DatasetIndexItem *item;
  if (!g_dataset_parse.active) {
    return;
  }
  item = ASC_NEW(struct DatasetIndexItem);
  item->set_name = set_name;
  item->column_name = column_name;
  item->type_name = type_name;
  item->next = NULL;
  if (g_dataset_parse.indices == NULL) {
    g_dataset_parse.indices = item;
  } else {
    struct DatasetIndexItem *tail = g_dataset_parse.indices;
    while (tail->next != NULL) {
      tail = tail->next;
    }
    tail->next = item;
  }
}

static void DatasetParseAddMap(struct Name *target,
                               symchar *column_name,
                               CONST char *units,
                               symchar *type_name)
{
  struct DatasetMapItem *item;
  if (!g_dataset_parse.active) {
    if (target != NULL) {
      DestroyName(target);
    }
    return;
  }
  item = ASC_NEW(struct DatasetMapItem);
  item->target = target;
  item->column_name = column_name;
  item->units = (units != NULL) ? ASC_STRDUP(units) : NULL;
  item->type_name = type_name;
  item->next = NULL;
  if (g_dataset_parse.maps == NULL) {
    g_dataset_parse.maps = item;
  } else {
    struct DatasetMapItem *tail = g_dataset_parse.maps;
    while (tail->next != NULL) {
      tail = tail->next;
    }
    tail->next = item;
  }
}

static struct Statement *DatasetParseFinish(void){
  struct Statement *result;
  if (!g_dataset_parse.active) {
    return NULL;
  }
  result = CreateDATASET(g_dataset_parse.name,
                         g_dataset_parse.filename,
                         g_dataset_parse.indices,
                         g_dataset_parse.maps);
  g_dataset_parse.active = 0;
  g_dataset_parse.name = NULL;
  g_dataset_parse.filename = NULL;
  g_dataset_parse.indices = NULL;
  g_dataset_parse.maps = NULL;
  return result;
}

static struct Name *DatasetAppendIndices(struct Name *base, struct VariableList *vl)
{
  CONST struct VariableList *node;
  struct Name *result = base;
  for (node = vl; node != NULL; node = NextVariableNode(node)) {
    CONST struct Name *nptr = NamePointer(node);
    struct Name *idxname = CopyName((struct Name *)nptr);
    struct Set *setnode = CreateSingleSet(CreateVarExpr(idxname));
    struct Name *setname = CreateSetName(setnode);
    result = JoinNames(result,setname);
  }
  return result;
}

static struct Name *TableDeclNameFromTarget(CONST struct Name *target){
  CONST struct Name *node;
  struct Name *result = NULL;

  for (node = target; node != NULL; node = NextName(node)) {
    if (NameId(node)) {
      result = CopyAppendNameNode(result,node);
    } else {
      CONST struct Set *setnode;
      for (setnode = NameSetPtr(node); setnode != NULL; setnode = NextSet(setnode)) {
        struct Name *idx = CreateSetName(CopySetNode(setnode));
        result = JoinNames(result,idx);
      }
    }
  }
  return result;
}

static int StatementListHasTypeDeclForName(CONST struct gl_list_t *list,
                                           CONST struct Name *name)
{
  unsigned long len;
  unsigned long c;
  if (list == NULL || name == NULL) {
    return 0;
  }
  len = gl_length(list);
  for (c = 1; c <= len; ++c) {
    struct Statement *s = (struct Statement *)gl_fetch(list,c);
    CONST struct VariableList *vl;
    if (s == NULL) {
      continue;
    }
    if (StatementType(s) != ISA && StatementType(s) != IRT && StatementType(s) != WILLBE) {
      continue;
    }
    for (vl = s->v.i.vl; vl != NULL; vl = NextVariableNode(vl)) {
      CONST struct Name *nptr = NamePointer(vl);
      if (nptr != NULL && CompareNames(nptr,name) == 0) {
        return 1;
      }
    }
  }
  return 0;
}

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


#line 679 "ascend/compiler/ascParse.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "ascParse.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ADD_TOK = 3,                    /* ADD_TOK  */
  YYSYMBOL_ALIASES_TOK = 4,                /* ALIASES_TOK  */
  YYSYMBOL_AND_TOK = 5,                    /* AND_TOK  */
  YYSYMBOL_ANY_TOK = 6,                    /* ANY_TOK  */
  YYSYMBOL_AREALIKE_TOK = 7,               /* AREALIKE_TOK  */
  YYSYMBOL_ARETHESAME_TOK = 8,             /* ARETHESAME_TOK  */
  YYSYMBOL_ARRAY_TOK = 9,                  /* ARRAY_TOK  */
  YYSYMBOL_ASSERT_TOK = 10,                /* ASSERT_TOK  */
  YYSYMBOL_ATOM_TOK = 11,                  /* ATOM_TOK  */
  YYSYMBOL_BEQ_TOK = 12,                   /* BEQ_TOK  */
  YYSYMBOL_BNE_TOK = 13,                   /* BNE_TOK  */
  YYSYMBOL_BREAK_TOK = 14,                 /* BREAK_TOK  */
  YYSYMBOL_CALL_TOK = 15,                  /* CALL_TOK  */
  YYSYMBOL_CARD_TOK = 16,                  /* CARD_TOK  */
  YYSYMBOL_CASE_TOK = 17,                  /* CASE_TOK  */
  YYSYMBOL_CHOICE_TOK = 18,                /* CHOICE_TOK  */
  YYSYMBOL_CHECK_TOK = 19,                 /* CHECK_TOK  */
  YYSYMBOL_CONDITIONAL_TOK = 20,           /* CONDITIONAL_TOK  */
  YYSYMBOL_CONSTANT_TOK = 21,              /* CONSTANT_TOK  */
  YYSYMBOL_CONTINUE_TOK = 22,              /* CONTINUE_TOK  */
  YYSYMBOL_CREATE_TOK = 23,                /* CREATE_TOK  */
  YYSYMBOL_DATA_TOK = 24,                  /* DATA_TOK  */
  YYSYMBOL_DECREASING_TOK = 25,            /* DECREASING_TOK  */
  YYSYMBOL_DEFAULT_TOK = 26,               /* DEFAULT_TOK  */
  YYSYMBOL_DEFINITION_TOK = 27,            /* DEFINITION_TOK  */
  YYSYMBOL_DER_TOK = 28,                   /* DER_TOK  */
  YYSYMBOL_DIMENSION_TOK = 29,             /* DIMENSION_TOK  */
  YYSYMBOL_DIMENSIONLESS_TOK = 30,         /* DIMENSIONLESS_TOK  */
  YYSYMBOL_DO_TOK = 31,                    /* DO_TOK  */
  YYSYMBOL_ELSE_TOK = 32,                  /* ELSE_TOK  */
  YYSYMBOL_END_TOK = 33,                   /* END_TOK  */
  YYSYMBOL_EXPECT_TOK = 34,                /* EXPECT_TOK  */
  YYSYMBOL_EXTERNAL_TOK = 35,              /* EXTERNAL_TOK  */
  YYSYMBOL_FALSE_TOK = 36,                 /* FALSE_TOK  */
  YYSYMBOL_FALLTHRU_TOK = 37,              /* FALLTHRU_TOK  */
  YYSYMBOL_FIX_TOK = 38,                   /* FIX_TOK  */
  YYSYMBOL_FOR_TOK = 39,                   /* FOR_TOK  */
  YYSYMBOL_FREE_TOK = 40,                  /* FREE_TOK  */
  YYSYMBOL_FROM_TOK = 41,                  /* FROM_TOK  */
  YYSYMBOL_GLOBAL_TOK = 42,                /* GLOBAL_TOK  */
  YYSYMBOL_IF_TOK = 43,                    /* IF_TOK  */
  YYSYMBOL_IGNORE_TOK = 44,                /* IGNORE_TOK  */
  YYSYMBOL_IMPORT_TOK = 45,                /* IMPORT_TOK  */
  YYSYMBOL_IN_TOK = 46,                    /* IN_TOK  */
  YYSYMBOL_INPUT_TOK = 47,                 /* INPUT_TOK  */
  YYSYMBOL_INCREASING_TOK = 48,            /* INCREASING_TOK  */
  YYSYMBOL_INTERACTIVE_TOK = 49,           /* INTERACTIVE_TOK  */
  YYSYMBOL_INDEPENDENT_TOK = 50,           /* INDEPENDENT_TOK  */
  YYSYMBOL_INTERSECTION_TOK = 51,          /* INTERSECTION_TOK  */
  YYSYMBOL_ISA_TOK = 52,                   /* ISA_TOK  */
  YYSYMBOL__IS_T = 53,                     /* _IS_T  */
  YYSYMBOL_ISREFINEDTO_TOK = 54,           /* ISREFINEDTO_TOK  */
  YYSYMBOL_LINK_TOK = 55,                  /* LINK_TOK  */
  YYSYMBOL_MAXIMIZE_TOK = 56,              /* MAXIMIZE_TOK  */
  YYSYMBOL_MAXINTEGER_TOK = 57,            /* MAXINTEGER_TOK  */
  YYSYMBOL_MAXREAL_TOK = 58,               /* MAXREAL_TOK  */
  YYSYMBOL_METHODS_TOK = 59,               /* METHODS_TOK  */
  YYSYMBOL_METHOD_TOK = 60,                /* METHOD_TOK  */
  YYSYMBOL_MINIMIZE_TOK = 61,              /* MINIMIZE_TOK  */
  YYSYMBOL_MODEL_TOK = 62,                 /* MODEL_TOK  */
  YYSYMBOL_NOT_TOK = 63,                   /* NOT_TOK  */
  YYSYMBOL_NOTES_TOK = 64,                 /* NOTES_TOK  */
  YYSYMBOL_OF_TOK = 65,                    /* OF_TOK  */
  YYSYMBOL_OPTION_TOK = 66,                /* OPTION_TOK  */
  YYSYMBOL_OR_TOK = 67,                    /* OR_TOK  */
  YYSYMBOL_OTHERWISE_TOK = 68,             /* OTHERWISE_TOK  */
  YYSYMBOL_OUTPUT_TOK = 69,                /* OUTPUT_TOK  */
  YYSYMBOL_PROD_TOK = 70,                  /* PROD_TOK  */
  YYSYMBOL_PROVIDE_TOK = 71,               /* PROVIDE_TOK  */
  YYSYMBOL_REFINES_TOK = 72,               /* REFINES_TOK  */
  YYSYMBOL_REPLACE_TOK = 73,               /* REPLACE_TOK  */
  YYSYMBOL_REQUIRE_TOK = 74,               /* REQUIRE_TOK  */
  YYSYMBOL_RETURN_TOK = 75,                /* RETURN_TOK  */
  YYSYMBOL_RUN_TOK = 76,                   /* RUN_TOK  */
  YYSYMBOL_SATISFIED_TOK = 77,             /* SATISFIED_TOK  */
  YYSYMBOL_SELECT_TOK = 78,                /* SELECT_TOK  */
  YYSYMBOL_SIZE_TOK = 79,                  /* SIZE_TOK  */
  YYSYMBOL_SOLVE_TOK = 80,                 /* SOLVE_TOK  */
  YYSYMBOL_SOLVER_TOK = 81,                /* SOLVER_TOK  */
  YYSYMBOL_STOP_TOK = 82,                  /* STOP_TOK  */
  YYSYMBOL_SUCHTHAT_TOK = 83,              /* SUCHTHAT_TOK  */
  YYSYMBOL_SUM_TOK = 84,                   /* SUM_TOK  */
  YYSYMBOL_SWITCH_TOK = 85,                /* SWITCH_TOK  */
  YYSYMBOL_TABLE_TOK = 86,                 /* TABLE_TOK  */
  YYSYMBOL_VALUES_TOK = 87,                /* VALUES_TOK  */
  YYSYMBOL_DATASET_TOK = 88,               /* DATASET_TOK  */
  YYSYMBOL_POSITIONAL_TOK = 89,            /* POSITIONAL_TOK  */
  YYSYMBOL_INDEX_TOK = 90,                 /* INDEX_TOK  */
  YYSYMBOL_COLUMN_TOK = 91,                /* COLUMN_TOK  */
  YYSYMBOL_EOL_TOK = 92,                   /* EOL_TOK  */
  YYSYMBOL_THEN_TOK = 93,                  /* THEN_TOK  */
  YYSYMBOL_TRUE_TOK = 94,                  /* TRUE_TOK  */
  YYSYMBOL_UNION_TOK = 95,                 /* UNION_TOK  */
  YYSYMBOL_UNITS_TOK = 96,                 /* UNITS_TOK  */
  YYSYMBOL_UNIVERSAL_TOK = 97,             /* UNIVERSAL_TOK  */
  YYSYMBOL_UNLINK_TOK = 98,                /* UNLINK_TOK  */
  YYSYMBOL_WHEN_TOK = 99,                  /* WHEN_TOK  */
  YYSYMBOL_WHERE_TOK = 100,                /* WHERE_TOK  */
  YYSYMBOL_WHILE_TOK = 101,                /* WHILE_TOK  */
  YYSYMBOL_WILLBE_TOK = 102,               /* WILLBE_TOK  */
  YYSYMBOL_WILLBETHESAME_TOK = 103,        /* WILLBETHESAME_TOK  */
  YYSYMBOL_WILLNOTBETHESAME_TOK = 104,     /* WILLNOTBETHESAME_TOK  */
  YYSYMBOL_ASSIGN_TOK = 105,               /* ASSIGN_TOK  */
  YYSYMBOL_CASSIGN_TOK = 106,              /* CASSIGN_TOK  */
  YYSYMBOL_DBLCOLON_TOK = 107,             /* DBLCOLON_TOK  */
  YYSYMBOL_USE_TOK = 108,                  /* USE_TOK  */
  YYSYMBOL_LEQ_TOK = 109,                  /* LEQ_TOK  */
  YYSYMBOL_GEQ_TOK = 110,                  /* GEQ_TOK  */
  YYSYMBOL_NEQ_TOK = 111,                  /* NEQ_TOK  */
  YYSYMBOL_DOTDOT_TOK = 112,               /* DOTDOT_TOK  */
  YYSYMBOL_WITH_TOK = 113,                 /* WITH_TOK  */
  YYSYMBOL_VALUE_TOK = 114,                /* VALUE_TOK  */
  YYSYMBOL_WITH_VALUE_T = 115,             /* WITH_VALUE_T  */
  YYSYMBOL_REAL_TOK = 116,                 /* REAL_TOK  */
  YYSYMBOL_INTEGER_TOK = 117,              /* INTEGER_TOK  */
  YYSYMBOL_IDENTIFIER_TOK = 118,           /* IDENTIFIER_TOK  */
  YYSYMBOL_BRACEDTEXT_TOK = 119,           /* BRACEDTEXT_TOK  */
  YYSYMBOL_SYMBOL_TOK = 120,               /* SYMBOL_TOK  */
  YYSYMBOL_DQUOTE_TOK = 121,               /* DQUOTE_TOK  */
  YYSYMBOL_122_ = 122,                     /* ','  */
  YYSYMBOL_123_ = 123,                     /* '|'  */
  YYSYMBOL_124_ = 124,                     /* '<'  */
  YYSYMBOL_125_ = 125,                     /* '='  */
  YYSYMBOL_126_ = 126,                     /* '>'  */
  YYSYMBOL_127_ = 127,                     /* '+'  */
  YYSYMBOL_128_ = 128,                     /* '-'  */
  YYSYMBOL_129_ = 129,                     /* '/'  */
  YYSYMBOL_130_ = 130,                     /* '*'  */
  YYSYMBOL_UMINUS_TOK = 131,               /* UMINUS_TOK  */
  YYSYMBOL_UPLUS_TOK = 132,                /* UPLUS_TOK  */
  YYSYMBOL_133_ = 133,                     /* '^'  */
  YYSYMBOL_134_ = 134,                     /* ';'  */
  YYSYMBOL_135_ = 135,                     /* '('  */
  YYSYMBOL_136_ = 136,                     /* ')'  */
  YYSYMBOL_137_ = 137,                     /* ':'  */
  YYSYMBOL_138_ = 138,                     /* '['  */
  YYSYMBOL_139_ = 139,                     /* ']'  */
  YYSYMBOL_140_ = 140,                     /* '.'  */
  YYSYMBOL_YYACCEPT = 141,                 /* $accept  */
  YYSYMBOL_definitions = 142,              /* definitions  */
  YYSYMBOL_definition = 143,               /* definition  */
  YYSYMBOL_global_def = 144,               /* global_def  */
  YYSYMBOL_require_file = 145,             /* require_file  */
  YYSYMBOL_provide_module = 146,           /* provide_module  */
  YYSYMBOL_import = 147,                   /* import  */
  YYSYMBOL_add_notes_def = 148,            /* add_notes_def  */
  YYSYMBOL_add_notes_head = 149,           /* add_notes_head  */
  YYSYMBOL_add_method_def = 150,           /* add_method_def  */
  YYSYMBOL_add_method_head = 151,          /* add_method_head  */
  YYSYMBOL_replace_method_def = 152,       /* replace_method_def  */
  YYSYMBOL_replace_method_head = 153,      /* replace_method_head  */
  YYSYMBOL_atom_def = 154,                 /* atom_def  */
  YYSYMBOL_atom_head = 155,                /* atom_head  */
  YYSYMBOL_atom_id = 156,                  /* atom_id  */
  YYSYMBOL_default_val = 157,              /* default_val  */
  YYSYMBOL_constant_def = 158,             /* constant_def  */
  YYSYMBOL_constant_head = 159,            /* constant_head  */
  YYSYMBOL_constant_val = 160,             /* constant_val  */
  YYSYMBOL_model_def = 161,                /* model_def  */
  YYSYMBOL_model_head = 162,               /* model_head  */
  YYSYMBOL_model_id = 163,                 /* model_id  */
  YYSYMBOL_optional_model_parameters = 164, /* optional_model_parameters  */
  YYSYMBOL_optional_parameter_wheres = 165, /* optional_parameter_wheres  */
  YYSYMBOL_optional_parameter_reduction = 166, /* optional_parameter_reduction  */
  YYSYMBOL_universal = 167,                /* universal  */
  YYSYMBOL_definition_def = 168,           /* definition_def  */
  YYSYMBOL_definition_id = 169,            /* definition_id  */
  YYSYMBOL_units_def = 170,                /* units_def  */
  YYSYMBOL_units_statement = 171,          /* units_statement  */
  YYSYMBOL_table_statement = 172,          /* table_statement  */
  YYSYMBOL_table_begin = 173,              /* table_begin  */
  YYSYMBOL_table_mode_on = 174,            /* table_mode_on  */
  YYSYMBOL_table_mode_off = 175,           /* table_mode_off  */
  YYSYMBOL_table_options = 176,            /* table_options  */
  YYSYMBOL_table_decl_opt = 177,           /* table_decl_opt  */
  YYSYMBOL_table_option = 178,             /* table_option  */
  YYSYMBOL_table_body = 179,               /* table_body  */
  YYSYMBOL_table_body_item = 180,          /* table_body_item  */
  YYSYMBOL_table_scalar = 181,             /* table_scalar  */
  YYSYMBOL_values_statement = 182,         /* values_statement  */
  YYSYMBOL_values_default_opt = 183,       /* values_default_opt  */
  YYSYMBOL_values_entries = 184,           /* values_entries  */
  YYSYMBOL_values_entry = 185,             /* values_entry  */
  YYSYMBOL_values_key_list = 186,          /* values_key_list  */
  YYSYMBOL_values_key = 187,               /* values_key  */
  YYSYMBOL_dataset_statement = 188,        /* dataset_statement  */
  YYSYMBOL_189_1 = 189,                    /* $@1  */
  YYSYMBOL_dataset_items = 190,            /* dataset_items  */
  YYSYMBOL_dataset_item = 191,             /* dataset_item  */
  YYSYMBOL_dataset_index_item = 192,       /* dataset_index_item  */
  YYSYMBOL_dataset_map_item = 193,         /* dataset_map_item  */
  YYSYMBOL_dataset_target = 194,           /* dataset_target  */
  YYSYMBOL_dataset_units_opt = 195,        /* dataset_units_opt  */
  YYSYMBOL_dataset_type_opt = 196,         /* dataset_type_opt  */
  YYSYMBOL_unitdeflist = 197,              /* unitdeflist  */
  YYSYMBOL_unitdef = 198,                  /* unitdef  */
  YYSYMBOL_methods = 199,                  /* methods  */
  YYSYMBOL_200_2 = 200,                    /* $@2  */
  YYSYMBOL_proclist = 201,                 /* proclist  */
  YYSYMBOL_proclistf = 202,                /* proclistf  */
  YYSYMBOL_procedure = 203,                /* procedure  */
  YYSYMBOL_procedure_id = 204,             /* procedure_id  */
  YYSYMBOL_fstatements = 205,              /* fstatements  */
  YYSYMBOL_statements = 206,               /* statements  */
  YYSYMBOL_statement = 207,                /* statement  */
  YYSYMBOL_complex_statement = 208,        /* complex_statement  */
  YYSYMBOL_isa_statement = 209,            /* isa_statement  */
  YYSYMBOL_willbe_statement = 210,         /* willbe_statement  */
  YYSYMBOL_aliases_statement = 211,        /* aliases_statement  */
  YYSYMBOL_optional_set_values = 212,      /* optional_set_values  */
  YYSYMBOL_is_statement = 213,             /* is_statement  */
  YYSYMBOL_isrefinedto_statement = 214,    /* isrefinedto_statement  */
  YYSYMBOL_call_identifier = 215,          /* call_identifier  */
  YYSYMBOL_type_identifier = 216,          /* type_identifier  */
  YYSYMBOL_optional_method = 217,          /* optional_method  */
  YYSYMBOL_optional_of = 218,              /* optional_of  */
  YYSYMBOL_optional_with_value = 219,      /* optional_with_value  */
  YYSYMBOL_arealike_statement = 220,       /* arealike_statement  */
  YYSYMBOL_link_statement = 221,           /* link_statement  */
  YYSYMBOL_unlink_statement = 222,         /* unlink_statement  */
  YYSYMBOL_der_statement = 223,            /* der_statement  */
  YYSYMBOL_independent_statement = 224,    /* independent_statement  */
  YYSYMBOL_arethesame_statement = 225,     /* arethesame_statement  */
  YYSYMBOL_willbethesame_statement = 226,  /* willbethesame_statement  */
  YYSYMBOL_willnotbethesame_statement = 227, /* willnotbethesame_statement  */
  YYSYMBOL_assignment_statement = 228,     /* assignment_statement  */
  YYSYMBOL_relation_statement = 229,       /* relation_statement  */
  YYSYMBOL_relation = 230,                 /* relation  */
  YYSYMBOL_blackbox_statement = 231,       /* blackbox_statement  */
  YYSYMBOL_input_args = 232,               /* input_args  */
  YYSYMBOL_output_args = 233,              /* output_args  */
  YYSYMBOL_data_args = 234,                /* data_args  */
  YYSYMBOL_for_statement = 235,            /* for_statement  */
  YYSYMBOL_optional_direction = 236,       /* optional_direction  */
  YYSYMBOL_forexprend = 237,               /* forexprend  */
  YYSYMBOL_run_statement = 238,            /* run_statement  */
  YYSYMBOL_fix_statement = 239,            /* fix_statement  */
  YYSYMBOL_fix_and_assign_statement = 240, /* fix_and_assign_statement  */
  YYSYMBOL_free_statement = 241,           /* free_statement  */
  YYSYMBOL_solver_statement = 242,         /* solver_statement  */
  YYSYMBOL_option_statement = 243,         /* option_statement  */
  YYSYMBOL_solve_statement = 244,          /* solve_statement  */
  YYSYMBOL_external_statement = 245,       /* external_statement  */
  YYSYMBOL_call_statement = 246,           /* call_statement  */
  YYSYMBOL_assert_statement = 247,         /* assert_statement  */
  YYSYMBOL_if_statement = 248,             /* if_statement  */
  YYSYMBOL_while_statement = 249,          /* while_statement  */
  YYSYMBOL_optional_else = 250,            /* optional_else  */
  YYSYMBOL_when_statement = 251,           /* when_statement  */
  YYSYMBOL_whenlist = 252,                 /* whenlist  */
  YYSYMBOL_whenlistf = 253,                /* whenlistf  */
  YYSYMBOL_flow_statement = 254,           /* flow_statement  */
  YYSYMBOL_use_statement = 255,            /* use_statement  */
  YYSYMBOL_select_statement = 256,         /* select_statement  */
  YYSYMBOL_selectlist = 257,               /* selectlist  */
  YYSYMBOL_selectlistf = 258,              /* selectlistf  */
  YYSYMBOL_switch_statement = 259,         /* switch_statement  */
  YYSYMBOL_switchlist = 260,               /* switchlist  */
  YYSYMBOL_switchlistf = 261,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 262,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 263,          /* notes_statement  */
  YYSYMBOL_notes_body = 264,               /* notes_body  */
  YYSYMBOL_noteslist = 265,                /* noteslist  */
  YYSYMBOL_fvarlist = 266,                 /* fvarlist  */
  YYSYMBOL_varlist = 267,                  /* varlist  */
  YYSYMBOL_fname = 268,                    /* fname  */
  YYSYMBOL_name = 269,                     /* name  */
  YYSYMBOL_end = 270,                      /* end  */
  YYSYMBOL_optional_bracedtext = 271,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 272,           /* optional_notes  */
  YYSYMBOL_set = 273,                      /* set  */
  YYSYMBOL_setexprlist = 274,              /* setexprlist  */
  YYSYMBOL_number = 275,                   /* number  */
  YYSYMBOL_realnumber = 276,               /* realnumber  */
  YYSYMBOL_opunits = 277,                  /* opunits  */
  YYSYMBOL_dims = 278,                     /* dims  */
  YYSYMBOL_dimensions = 279,               /* dimensions  */
  YYSYMBOL_dimexpr = 280,                  /* dimexpr  */
  YYSYMBOL_fraction = 281,                 /* fraction  */
  YYSYMBOL_fractail = 282,                 /* fractail  */
  YYSYMBOL_optional_sign = 283,            /* optional_sign  */
  YYSYMBOL_expr = 284,                     /* expr  */
  YYSYMBOL_relop = 285,                    /* relop  */
  YYSYMBOL_logrelop = 286                  /* logrelop  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1252

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  141
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  146
/* YYNRULES -- Number of rules.  */
#define YYNRULES  376
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  748

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   378


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     135,   136,   130,   127,   122,   128,   140,   129,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   137,   134,
     124,   125,   126,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   138,     2,   139,   133,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   123,     2,     2,     2,     2,     2,
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
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   131,   132
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   767,   767,   769,   773,   774,   775,   776,   777,   778,
     779,   780,   781,   782,   784,   785,   786,   793,   838,   842,
     847,   855,   859,   864,   872,   881,   893,   924,   932,   955,
     968,   976,   997,  1009,  1017,  1069,  1080,  1089,  1094,  1099,
    1106,  1113,  1123,  1162,  1197,  1202,  1207,  1214,  1221,  1231,
    1272,  1281,  1294,  1303,  1306,  1314,  1317,  1325,  1328,  1379,
    1382,  1389,  1419,  1428,  1433,  1453,  1472,  1485,  1492,  1499,
    1504,  1506,  1511,  1519,  1532,  1536,  1545,  1547,  1551,  1552,
    1556,  1560,  1564,  1568,  1572,  1576,  1583,  1587,  1591,  1595,
    1599,  1606,  1613,  1615,  1622,  1623,  1627,  1634,  1635,  1639,
    1640,  1641,  1642,  1647,  1646,  1656,  1658,  1659,  1669,  1670,
    1674,  1681,  1688,  1693,  1702,  1705,  1713,  1716,  1723,  1726,
    1734,  1744,  1748,  1747,  1761,  1769,  1772,  1797,  1811,  1820,
    1828,  1831,  1896,  1904,  1912,  1913,  1914,  1915,  1916,  1917,
    1918,  1919,  1920,  1921,  1922,  1923,  1924,  1925,  1926,  1928,
    1929,  1930,  1931,  1932,  1933,  1934,  1935,  1936,  1937,  1938,
    1939,  1940,  1941,  1942,  1943,  1944,  1945,  1946,  1947,  1948,
    1949,  1950,  1951,  1955,  1960,  2001,  2030,  2034,  2085,  2088,
    2095,  2107,  2134,  2139,  2147,  2152,  2161,  2164,  2172,  2175,
    2183,  2186,  2193,  2200,  2204,  2208,  2215,  2219,  2226,  2235,
    2244,  2251,  2258,  2265,  2269,  2276,  2289,  2306,  2316,  2324,
    2335,  2356,  2363,  2371,  2374,  2411,  2433,  2436,  2440,  2447,
    2451,  2455,  2459,  2466,  2470,  2477,  2485,  2499,  2506,  2514,
    2522,  2530,  2541,  2552,  2558,  2568,  2578,  2581,  2588,  2599,
    2611,  2618,  2628,  2635,  2639,  2643,  2647,  2654,  2658,  2662,
    2666,  2670,  2677,  2684,  2695,  2705,  2712,  2716,  2720,  2724,
    2731,  2742,  2752,  2759,  2763,  2767,  2771,  2778,  2788,  2825,
    2834,  2849,  2854,  2863,  2874,  2878,  2883,  2896,  2916,  2920,
    2925,  2940,  2945,  2950,  2955,  2960,  2965,  2970,  2975,  2980,
    2985,  2990,  2995,  3000,  3005,  3014,  3017,  3025,  3028,  3035,
    3040,  3046,  3050,  3054,  3059,  3067,  3073,  3082,  3086,  3108,
    3112,  3133,  3137,  3142,  3148,  3152,  3159,  3163,  3167,  3171,
    3175,  3179,  3186,  3193,  3197,  3205,  3208,  3212,  3219,  3223,
    3227,  3231,  3235,  3239,  3243,  3247,  3251,  3255,  3259,  3264,
    3269,  3274,  3279,  3284,  3289,  3294,  3298,  3303,  3308,  3313,
    3318,  3323,  3327,  3331,  3335,  3339,  3346,  3350,  3357,  3361,
    3368,  3372,  3379,  3383,  3390,  3394,  3401,  3405,  3416,  3423,
    3427,  3431,  3435,  3439,  3443,  3450,  3454
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ADD_TOK",
  "ALIASES_TOK", "AND_TOK", "ANY_TOK", "AREALIKE_TOK", "ARETHESAME_TOK",
  "ARRAY_TOK", "ASSERT_TOK", "ATOM_TOK", "BEQ_TOK", "BNE_TOK", "BREAK_TOK",
  "CALL_TOK", "CARD_TOK", "CASE_TOK", "CHOICE_TOK", "CHECK_TOK",
  "CONDITIONAL_TOK", "CONSTANT_TOK", "CONTINUE_TOK", "CREATE_TOK",
  "DATA_TOK", "DECREASING_TOK", "DEFAULT_TOK", "DEFINITION_TOK", "DER_TOK",
  "DIMENSION_TOK", "DIMENSIONLESS_TOK", "DO_TOK", "ELSE_TOK", "END_TOK",
  "EXPECT_TOK", "EXTERNAL_TOK", "FALSE_TOK", "FALLTHRU_TOK", "FIX_TOK",
  "FOR_TOK", "FREE_TOK", "FROM_TOK", "GLOBAL_TOK", "IF_TOK", "IGNORE_TOK",
  "IMPORT_TOK", "IN_TOK", "INPUT_TOK", "INCREASING_TOK", "INTERACTIVE_TOK",
  "INDEPENDENT_TOK", "INTERSECTION_TOK", "ISA_TOK", "_IS_T",
  "ISREFINEDTO_TOK", "LINK_TOK", "MAXIMIZE_TOK", "MAXINTEGER_TOK",
  "MAXREAL_TOK", "METHODS_TOK", "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK",
  "NOT_TOK", "NOTES_TOK", "OF_TOK", "OPTION_TOK", "OR_TOK",
  "OTHERWISE_TOK", "OUTPUT_TOK", "PROD_TOK", "PROVIDE_TOK", "REFINES_TOK",
  "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK", "SATISFIED_TOK",
  "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK", "SOLVER_TOK", "STOP_TOK",
  "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK", "TABLE_TOK", "VALUES_TOK",
  "DATASET_TOK", "POSITIONAL_TOK", "INDEX_TOK", "COLUMN_TOK", "EOL_TOK",
  "THEN_TOK", "TRUE_TOK", "UNION_TOK", "UNITS_TOK", "UNIVERSAL_TOK",
  "UNLINK_TOK", "WHEN_TOK", "WHERE_TOK", "WHILE_TOK", "WILLBE_TOK",
  "WILLBETHESAME_TOK", "WILLNOTBETHESAME_TOK", "ASSIGN_TOK", "CASSIGN_TOK",
  "DBLCOLON_TOK", "USE_TOK", "LEQ_TOK", "GEQ_TOK", "NEQ_TOK", "DOTDOT_TOK",
  "WITH_TOK", "VALUE_TOK", "WITH_VALUE_T", "REAL_TOK", "INTEGER_TOK",
  "IDENTIFIER_TOK", "BRACEDTEXT_TOK", "SYMBOL_TOK", "DQUOTE_TOK", "','",
  "'|'", "'<'", "'='", "'>'", "'+'", "'-'", "'/'", "'*'", "UMINUS_TOK",
  "UPLUS_TOK", "'^'", "';'", "'('", "')'", "':'", "'['", "']'", "'.'",
  "$accept", "definitions", "definition", "global_def", "require_file",
  "provide_module", "import", "add_notes_def", "add_notes_head",
  "add_method_def", "add_method_head", "replace_method_def",
  "replace_method_head", "atom_def", "atom_head", "atom_id", "default_val",
  "constant_def", "constant_head", "constant_val", "model_def",
  "model_head", "model_id", "optional_model_parameters",
  "optional_parameter_wheres", "optional_parameter_reduction", "universal",
  "definition_def", "definition_id", "units_def", "units_statement",
  "table_statement", "table_begin", "table_mode_on", "table_mode_off",
  "table_options", "table_decl_opt", "table_option", "table_body",
  "table_body_item", "table_scalar", "values_statement",
  "values_default_opt", "values_entries", "values_entry",
  "values_key_list", "values_key", "dataset_statement", "$@1",
  "dataset_items", "dataset_item", "dataset_index_item",
  "dataset_map_item", "dataset_target", "dataset_units_opt",
  "dataset_type_opt", "unitdeflist", "unitdef", "methods", "$@2",
  "proclist", "proclistf", "procedure", "procedure_id", "fstatements",
  "statements", "statement", "complex_statement", "isa_statement",
  "willbe_statement", "aliases_statement", "optional_set_values",
  "is_statement", "isrefinedto_statement", "call_identifier",
  "type_identifier", "optional_method", "optional_of",
  "optional_with_value", "arealike_statement", "link_statement",
  "unlink_statement", "der_statement", "independent_statement",
  "arethesame_statement", "willbethesame_statement",
  "willnotbethesame_statement", "assignment_statement",
  "relation_statement", "relation", "blackbox_statement", "input_args",
  "output_args", "data_args", "for_statement", "optional_direction",
  "forexprend", "run_statement", "fix_statement",
  "fix_and_assign_statement", "free_statement", "solver_statement",
  "option_statement", "solve_statement", "external_statement",
  "call_statement", "assert_statement", "if_statement", "while_statement",
  "optional_else", "when_statement", "whenlist", "whenlistf",
  "flow_statement", "use_statement", "select_statement", "selectlist",
  "selectlistf", "switch_statement", "switchlist", "switchlistf",
  "conditional_statement", "notes_statement", "notes_body", "noteslist",
  "fvarlist", "varlist", "fname", "name", "end", "optional_bracedtext",
  "optional_notes", "set", "setexprlist", "number", "realnumber",
  "opunits", "dims", "dimensions", "dimexpr", "fraction", "fractail",
  "optional_sign", "expr", "relop", "logrelop", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-498)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-275)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -498,  1155,  -498,  -498,   -31,   -91,   -86,   -59,   107,    23,
     109,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,   -11,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,    24,  -498,  -498,
    -498,    -7,    88,   122,  -498,  -498,   116,    41,  -498,    56,
      42,   155,    72,    70,   -22,   131,   -15,   212,   191,   212,
     137,   163,   171,  -498,   221,  -498,  -498,   164,   246,   656,
    -498,    -6,   189,   212,   190,  -498,  -498,  -498,   428,   195,
      -4,  -498,  -498,  1105,   196,  -498,  -498,   131,   204,    59,
    -498,   -85,   131,   181,   202,   225,  -498,   213,   214,  -498,
     277,  -498,   246,   234,   246,  -498,   254,  -498,   212,   222,
    -498,   428,  -498,   241,   105,   106,  -498,  -498,   229,   242,
    -498,  -498,   131,   247,   131,   428,   131,   125,   232,   428,
    -498,  -498,   428,   428,   -11,   253,   127,  -498,   131,   237,
      38,  -498,   259,   255,   156,    43,   131,   131,   260,  -498,
     160,   245,    44,   428,   131,   268,   269,   258,  -498,   428,
     428,   428,   428,  -498,  -498,  -498,  -498,   261,   263,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,    35,   111,  -498,  1003,
     336,   265,   340,   270,   276,  -498,   264,   286,   834,  -498,
     349,   279,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,   297,   298,  -498,   131,  -498,
    -498,  -498,   131,  -498,  -498,  -498,  -498,  -498,   300,   212,
     112,   212,   283,   295,   -41,  -498,   301,  -498,  1003,   302,
    -498,   428,   428,   428,   428,   212,   131,   303,  -498,  -498,
     223,   385,  -498,   872,  -498,   428,   428,   -14,  1003,  1003,
    -498,   -15,   428,   428,   428,   325,   131,   131,    27,  -498,
    -498,  -498,   428,   428,   131,    48,  -498,   410,   398,   428,
     428,   -71,   131,    64,   901,  -498,  -498,  -498,  -498,   428,
     307,   307,   482,   306,  -498,  -498,    51,  -498,  -498,   323,
     329,   323,   323,  -498,  -498,   428,   428,   274,   428,  -498,
    -498,   428,   428,   428,  -498,  -498,  -498,   428,  -498,  -498,
    -498,   428,   428,   428,   428,   428,   428,   428,   314,  -498,
     331,   316,  -498,  -498,  -498,   428,   428,   317,  -498,   318,
    -498,  -498,   212,   112,   319,   -67,  -498,   431,   320,  -498,
    -498,   341,  -498,  -498,  -498,   428,   326,   322,   327,   328,
    -498,   330,   131,   428,  -498,   332,   333,   343,   353,   354,
    -498,  1003,   342,   338,   131,   -96,   344,   428,   345,   212,
      86,   347,   350,   348,   428,   351,   212,    90,   429,   428,
     356,   371,   357,   358,   374,   377,   364,   428,   365,   212,
     103,  -498,   734,  -498,  -498,   131,  -498,   366,   439,   439,
    -498,   439,  1003,  1003,    54,   372,  -498,   694,   694,   694,
     296,   296,   146,   146,   307,   307,   307,   256,  1013,  -498,
    -498,  -498,   973,  1003,  -498,  -498,   376,   400,  -498,  -498,
    -498,  -498,    31,  -498,   187,   206,   379,  -498,   378,   373,
     380,  -498,  -498,  -498,  -498,  -498,   381,   765,   486,  -498,
    -498,   399,   131,   131,  -498,  -498,  -498,   -56,  -498,    27,
     384,  -498,  -498,   428,   387,  -498,  -498,    48,   388,  -498,
    -498,   428,   390,   323,  -498,  1003,   186,   395,  -498,  -498,
     131,   131,    64,   393,  -498,  -498,   428,   394,   212,  -498,
     397,   428,   416,   420,  -498,   420,   131,    64,   428,   428,
    -498,   218,   415,   123,    31,    31,   143,  -498,  -498,  -498,
    -498,  -498,   224,  -498,  -498,  -498,   403,  -498,  -498,  -498,
    -498,   165,  -498,   212,   417,   404,   405,   269,   406,   212,
    -498,  -498,   413,  -498,   212,  -498,  -498,   421,  -498,   439,
      -9,  -498,  -498,  -498,  -498,    13,   409,   197,  -498,  -498,
     411,   423,   212,  -498,  -498,   424,  -498,  -498,   452,   426,
    -498,   428,  -498,  -498,   432,   212,   419,   433,    33,  1003,
    -498,  -498,  -498,   224,   435,  -498,   427,   427,  -498,  -110,
     269,  -498,  -498,   437,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,   131,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,   428,  -498,  -498,  -498,   484,
     440,  -498,   186,   428,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,   131,  -498,  1003,    64,  -498,   131,   531,  -498,  -498,
    -498,   462,  -498,  -498,   212,   445,  -498,  -498,  1003,    75,
    -498,  -498,  -498,  1003,     8,  -498,   530,   212,   451,   449,
    -498,   458,  -498,  -498,   556,   944,   456,   506,   479,   461,
     466,  -498,  -498,   -25,   485,  -498,   131,   468,   533,   497,
     535,   536,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,   575,   131,
    -498,   526,   131,   554,   487,  -498,  -498,   489,  -498,  -498,
     532,   488,   508,   490,   510,   606,  -498,  -498,  -498,   514,
    -498,   515,  -498,   520,  -498,   584,  -498,   585,   503,  -498,
     521,   524,  -498,   428,  -498,  -498,   507,  -498
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    16,     0,     0,     0,     0,     0,     0,
       0,   118,    60,     3,    15,     4,     5,     6,     9,     0,
       7,   125,     8,   125,    11,    10,    12,     0,    13,   130,
      14,     0,     0,     0,    62,   130,     0,     0,   278,     0,
      23,     0,     0,    20,     0,     0,     0,     0,   124,     0,
       0,     0,     0,   130,     0,    42,   130,    53,   121,     0,
      63,     0,     0,     0,     0,    25,    21,    22,   300,     0,
       0,    18,    19,   294,     0,   119,    64,   269,     0,   273,
     274,   297,     0,     0,     0,     0,   126,     0,     0,    36,
       0,    52,   121,     0,   121,   130,    55,   122,     0,     0,
     334,     0,   247,     0,     0,     0,   130,   248,     0,     0,
     333,   249,     0,     0,     0,     0,     0,     0,     0,     0,
     329,   331,     0,     0,     0,     0,     0,   250,     0,     0,
       0,   230,     0,   295,     0,     0,     0,     0,     0,   332,
       0,     0,     0,     0,     0,   309,   328,   278,   335,     0,
       0,     0,   300,   169,   170,   171,   172,     0,     0,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   205,   149,   152,   153,   154,   173,
     155,   156,   158,   157,   151,   150,   159,   160,   161,   162,
     164,   163,   165,   166,   167,   168,     0,   336,   330,   207,
       0,     0,   186,     0,     0,   336,     0,   299,   301,   279,
       0,     0,   281,   282,   290,   283,   284,   285,   286,   287,
     288,   289,   291,   292,   293,     0,     0,   271,     0,   276,
     298,   277,   270,    26,    28,   128,   130,    31,     0,     0,
     313,     0,     0,     0,     0,   125,     0,   133,   233,   182,
     232,   300,   300,   300,   300,     0,     0,     0,   226,   225,
     274,     0,   227,     0,   199,   300,   300,     0,   209,   208,
     345,     0,     0,   300,   300,   223,     0,     0,     0,   228,
     296,   251,   300,   300,     0,     0,    67,    92,     0,   300,
     300,     0,     0,     0,     0,   252,   310,   307,   308,     0,
     351,   352,     0,     0,   131,   132,     0,   192,   200,     0,
       0,     0,     0,   201,   202,     0,     0,     0,     0,   375,
     376,     0,     0,     0,   372,   373,   374,     0,   370,   369,
     371,     0,     0,     0,     0,     0,     0,     0,     0,    29,
       0,     0,    17,    24,   280,     0,     0,     0,    32,     0,
     272,   275,     0,   313,     0,     0,   312,    37,     0,    54,
     130,     0,    50,   123,    61,   300,     0,     0,     0,     0,
     267,     0,     0,     0,   130,     0,     0,     0,     0,     0,
     268,   229,     0,     0,     0,     0,     0,   300,     0,     0,
     255,     0,     0,     0,   300,     0,     0,   262,    72,     0,
       0,     0,     0,     0,     0,     0,     0,   300,     0,     0,
     242,   130,     0,   368,   337,     0,   176,   184,   188,   188,
     181,   188,   203,   204,     0,   278,   206,   343,   348,   344,
     350,   349,   338,   339,   341,   340,   342,   346,   347,    30,
     187,    27,   303,   302,    33,   120,     0,    44,    34,   317,
     316,   314,     0,   311,   315,   325,     0,    49,     0,    57,
       0,   363,   364,   365,   366,   198,     0,   216,   236,   361,
     362,     0,     0,     0,   357,   358,   224,     0,   354,     0,
       0,   130,   253,   300,     0,   355,   356,     0,     0,   130,
     260,   300,     0,     0,    70,    93,     0,     0,   359,   360,
       0,     0,     0,     0,   130,   238,   300,     0,     0,   367,
       0,   300,     0,   190,   180,   190,     0,     0,     0,     0,
     127,   325,   297,     0,     0,     0,   325,    39,    40,    41,
     326,   327,     0,    35,    56,   130,     0,   183,   231,   218,
     217,     0,   130,     0,     0,     0,     0,     0,     0,     0,
     130,   257,     0,   130,     0,   130,   264,     0,   130,   188,
       0,   102,   101,    99,   100,     0,     0,     0,    97,   103,
       0,     0,     0,   130,   244,     0,   130,   235,     0,     0,
     189,     0,   174,   175,     0,     0,     0,     0,   336,   304,
      47,    46,    48,     0,     0,   321,   318,   319,   320,     0,
     305,    38,   306,     0,    51,   221,   219,   222,   220,   130,
     237,   234,     0,   194,   195,   353,   254,   256,   130,   259,
     261,   263,   130,   266,    73,     0,    74,    68,    71,     0,
       0,    94,     0,     0,   105,   196,   197,   240,   243,   130,
     246,     0,   185,   191,     0,   239,     0,     0,    45,    43,
     323,     0,   322,    58,     0,     0,   258,   265,    75,     0,
      91,    95,    98,    96,     0,   245,     0,     0,   213,     0,
     211,     0,   215,   193,     0,     0,     0,     0,     0,     0,
       0,   108,   109,     0,     0,   241,     0,     0,     0,     0,
       0,     0,    85,    89,    88,    86,    90,    87,    81,    80,
      82,    83,    84,    79,    77,    78,   107,   104,     0,     0,
     106,     0,     0,     0,     0,   210,   212,     0,    69,    69,
       0,     0,     0,     0,     0,     0,   324,    66,    65,     0,
     112,   114,   113,   178,   214,     0,   115,   116,     0,   177,
       0,     0,   111,   300,   110,   117,     0,   179
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
     586,  -498,  -498,  -498,   -73,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,    82,  -498,    16,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,   126,  -498,
     -21,  -498,  -498,  -498,    22,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -308,  -498,  -413,   134,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,   538,  -498,   335,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,  -498,
    -498,  -497,  -498,  -498,  -498,  -498,   174,  -498,  -498,   168,
    -498,  -498,  -498,   534,   574,   -58,  -498,   -45,   228,   -34,
    -498,   138,  -130,  -498,    66,  -453,  -498,   310,  -498,  -414,
    -498,  -498,  -376,   -49,  -498,  -498
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    53,    54,   456,    25,    55,   522,
      26,    56,    57,    96,   244,   536,    27,    28,    29,    30,
      31,   154,   398,   659,   727,   560,   494,   628,   675,   704,
     705,   155,   400,   565,   566,   567,   568,   156,   634,   664,
     680,   681,   682,   683,   737,   742,    44,    75,    98,   245,
      47,    48,    86,    87,    58,    59,   157,   158,   159,   160,
     161,   739,   162,   163,   250,   418,   341,   513,   582,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   586,   668,   687,   176,   541,   609,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   543,
     189,   409,   410,   190,   191,   192,   389,   390,   193,   396,
     397,   194,   195,    46,    77,    78,    79,   205,    81,    76,
     281,   231,   206,   207,   601,   198,   297,   357,   453,   454,
     598,   652,   532,   208,   336,   337
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      80,   196,    49,   420,   421,   572,   514,   650,   515,   676,
     199,    73,    83,    84,   197,    88,   711,   625,    73,   226,
     585,   200,   303,   210,   548,   651,   477,    34,    32,   203,
     377,   361,    80,    33,   229,    50,   230,    80,   523,   306,
     478,   677,   307,   308,   387,    51,   629,    38,    35,   404,
     449,   450,   248,    68,   259,    69,   262,    63,   264,    36,
     145,   547,    37,   451,   246,   394,   263,   260,   452,    80,
     268,    80,   278,   269,   270,    92,   674,   285,    94,   602,
     626,   407,    41,   275,   293,    80,    52,   309,   310,   311,
      80,   286,   287,   362,   294,   388,    74,    80,   678,   295,
     300,   301,   302,   483,    38,    82,   378,   491,   -76,    45,
     596,   597,   201,   712,   211,  -274,   395,   242,  -274,  -274,
     506,   366,   367,   368,   369,   627,   679,    60,   255,   561,
     562,   563,   408,   564,    61,   375,   376,   312,   313,   314,
     602,   355,   356,   382,   383,   593,   624,   667,   449,   450,
     599,  -274,   391,   392,   484,  -274,    38,    64,   492,   402,
     403,    38,    38,  -274,  -274,  -274,   452,   -76,    62,    38,
    -274,   507,    38,   277,   226,    65,    67,    38,   284,   292,
      68,   228,    69,   351,   605,   559,   415,    80,   606,   516,
      66,   -76,   -76,   -76,   -76,   -76,   607,   -76,   371,   608,
     -76,    70,   -76,   -76,    72,   354,    71,   358,    68,   -76,
      69,    80,   -76,  -274,  -274,  -274,   315,   316,   239,   386,
     241,   370,   379,   381,   363,    38,   393,    38,    39,  -274,
      42,   385,    80,  -274,   406,   460,    40,   380,    43,    80,
     251,   253,   527,   252,   254,    73,   405,    80,   317,    38,
     412,    85,   524,   525,   590,    89,   526,   480,   352,   595,
     265,   416,   273,   266,   488,   274,   422,   423,   199,   427,
     530,   531,   428,   429,   430,   333,   334,   503,   431,   335,
     100,    90,   432,   433,   434,   435,   436,   437,   438,    91,
     104,   282,   105,    93,   283,   289,   442,   443,   290,    95,
     528,   318,   561,   562,   563,    97,   564,   202,   319,   320,
     110,   204,   591,   209,   466,   233,   524,   525,   446,   632,
     526,   225,   633,   227,   467,   117,   529,    80,   315,   316,
     119,   120,   121,   530,   531,   122,   234,   123,   592,   476,
     145,   600,   321,   235,   126,   530,   531,   236,   237,   238,
     495,   129,   240,   552,   243,   482,   247,   510,   134,   249,
     257,   557,   490,   322,   256,   261,   517,   267,   139,   140,
      80,   272,   276,   424,   280,   505,   575,   279,   288,    80,
     291,   579,   458,   331,   332,   333,   334,   296,   298,   335,
     145,   146,   425,   299,   148,   304,   468,   305,   338,   339,
     340,   149,   150,   344,   342,   324,   325,   326,   345,   151,
     343,   347,   152,   348,   545,   546,   349,   350,   353,   359,
     328,   329,   330,   331,   332,   333,   334,    80,    80,   335,
     360,   373,   384,   508,   100,   364,   399,   365,   372,   401,
     335,   417,   570,   571,   104,   414,   105,   419,   439,   440,
     441,   444,   445,   448,   457,    80,    80,   455,   584,   459,
     587,   462,   461,   463,   110,   471,   465,   464,   469,   412,
     589,    80,   470,   588,   577,   472,   473,   475,   474,   117,
     479,   493,   481,   485,   487,   120,   121,   318,   489,   486,
     496,   123,   497,   498,   319,   320,   500,   499,   126,   501,
     502,   511,   504,   551,   512,   129,   521,   518,   535,   611,
     520,   556,   134,   533,   534,   616,   537,   538,   542,   544,
     620,   550,   139,   140,   553,   555,   574,   558,   321,   569,
     573,   576,   643,   578,   580,   581,   230,   604,   637,   612,
     613,   614,   615,   631,   145,   146,   147,   635,   148,   322,
     618,   645,   641,   646,   655,   149,   150,   603,   622,   636,
     526,   639,   642,   151,   610,   323,   152,    80,   644,   649,
     647,   660,   617,   653,   661,   619,   658,   621,   670,   671,
     623,   673,   684,   666,   663,   686,   688,   689,   669,   690,
     706,   324,   325,   326,   707,   638,    80,   708,   640,   709,
     710,    80,   716,   713,   715,   327,   328,   329,   330,   331,
     332,   333,   334,   746,   717,   335,   720,   722,   413,   724,
     672,   718,   719,   729,   725,   726,   731,   730,   733,   732,
     734,   654,   735,   685,   736,   738,   740,   741,   743,   744,
     656,   714,   745,   747,   657,   153,   728,   630,   662,   583,
     258,   721,   426,   549,   723,   554,   232,    99,   271,   648,
     594,   665,   100,   447,    80,     0,   101,    80,     0,     0,
     102,   103,   104,  -129,   105,     0,   106,     0,   107,     0,
       0,     0,     0,     0,   108,     0,     0,     0,  -129,  -129,
       0,   109,   110,   111,   112,   113,   114,     0,     0,   115,
       0,     0,     0,     0,     0,     0,   116,   117,     0,     0,
       0,   118,   119,   120,   121,  -129,     0,   122,     0,   123,
     124,     0,   125,     0,  -129,     0,   126,     0,     0,     0,
       0,   127,   128,   129,   130,     0,   131,   132,   133,   318,
     134,   135,   136,   137,   138,     0,   319,   320,     0,     0,
     139,   140,    11,     0,   141,   142,     0,   143,     0,     0,
       0,     0,     0,     0,   144,     0,     0,     0,     0,     0,
     318,     0,   145,   146,   147,     0,   148,   319,   320,     0,
     321,     0,     0,   149,   150,     0,     0,     0,     0,     0,
     539,   151,  -129,     0,   152,     0,     0,     0,     0,     0,
       0,   322,     0,   324,   325,   326,     0,     0,     0,     0,
       0,   321,     0,   540,     0,     0,     0,   323,   328,   329,
     330,   331,   332,   333,   334,     0,     0,   335,     0,     0,
       0,     0,   322,     0,     0,     0,     0,     0,     0,   318,
       0,     0,     0,   324,   325,   326,   319,   320,   323,     0,
       0,     0,     0,     0,     0,     0,     0,   327,   328,   329,
     330,   331,   332,   333,   334,     0,     0,   335,     0,     0,
     509,     0,     0,     0,   324,   325,   326,   318,     0,     0,
     321,     0,     0,     0,   319,   320,     0,     0,   327,   328,
     329,   330,   331,   332,   333,   334,     0,     0,   335,     0,
       0,   322,     0,     0,     0,     0,   318,     0,     0,     0,
       0,     0,     0,   319,   320,     0,     0,   323,   321,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   411,     0,     0,     0,     0,     0,     0,   322,
       0,     0,     0,   324,   325,   326,   346,   321,     0,     0,
       0,     0,     0,     0,     0,   323,     0,   327,   328,   329,
     330,   331,   332,   333,   334,   374,     0,   335,   322,     0,
       0,     0,     0,     0,     0,     0,     0,   691,   318,     0,
       0,   324,   325,   326,   323,   319,   320,     0,     0,     0,
       0,     0,     0,     0,     0,   327,   328,   329,   330,   331,
     332,   333,   334,     0,     0,   335,     0,     0,   318,     0,
     324,   325,   326,     0,     0,   319,   320,     0,   318,   321,
       0,     0,     0,     0,   327,   328,   329,   330,   331,   332,
     333,   334,     0,     0,   335,     0,   692,     0,     0,     0,
     322,     0,     0,     0,     0,     0,     0,     0,     0,   321,
       0,     0,     0,     0,     0,     0,   323,     0,     0,   321,
     693,   694,   695,   696,   697,     0,   698,     0,     0,   699,
     322,   700,   701,     0,     0,     0,     0,     0,   702,     0,
     322,   703,   324,   325,   326,   519,   323,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   327,   328,   329,   330,
     331,   332,   333,   334,     0,     0,   335,     0,     0,     0,
       0,     0,   324,   325,   326,     0,     0,     0,     0,     0,
       0,     0,   324,   325,   326,   212,   327,   328,   329,   330,
     331,   332,   333,   334,     0,     0,   335,   328,   329,   330,
     331,   332,   333,   334,   213,     0,   335,   214,   215,     0,
       0,     0,     0,     0,   216,     2,     3,     0,     4,     0,
       0,     0,     0,     0,   217,     0,   -59,     0,     0,   218,
       0,     0,     0,     0,     0,     0,   -59,     0,     0,     0,
       0,     0,     5,   219,     0,     0,     0,     0,     0,     0,
     220,     0,     0,     0,     0,     0,     0,     6,     0,     0,
       7,   221,     0,     0,   222,     0,   223,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   -59,     0,     0,
       0,     0,     0,   224,     0,     0,     8,     0,     9,    10,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    11,    12
};

static const yytype_int16 yycheck[] =
{
      45,    59,    23,   311,   312,   502,   419,   117,   421,     1,
      59,    33,    46,    47,    59,    49,    41,    26,    33,    77,
     517,    27,   152,    27,   477,   135,   122,   118,    59,    63,
      44,    72,    77,    64,    79,    11,   121,    82,   452,     4,
     136,    33,     7,     8,    17,    21,    33,   118,   134,   120,
     117,   118,   101,   138,   112,   140,   114,    35,   116,   118,
     116,   117,   121,   130,    98,    17,   115,   112,   135,   114,
     119,   116,   130,   122,   123,    53,     1,   135,    56,   532,
      89,    17,    59,   128,   142,   130,    62,    52,    53,    54,
     135,   136,   137,   134,   143,    68,   118,   142,    90,   144,
     149,   150,   151,    17,   118,   120,   120,    17,    33,   120,
     524,   525,   118,   138,   118,     4,    68,    95,     7,     8,
      17,   251,   252,   253,   254,   134,   118,   134,   106,   116,
     117,   118,    68,   120,    46,   265,   266,   102,   103,   104,
     593,    29,    30,   273,   274,   521,   559,   644,   117,   118,
     526,   118,   282,   283,    68,   122,   118,    41,    68,   289,
     290,   118,   118,    52,    53,    54,   135,    92,    46,   118,
     137,    68,   118,   135,   232,   134,   134,   118,   135,   135,
     138,   122,   140,   228,    19,   493,   135,   232,    23,   135,
     134,   116,   117,   118,   119,   120,    31,   122,   256,    34,
     125,    46,   127,   128,   134,   239,   134,   241,   138,   134,
     140,   256,   137,   102,   103,   104,   105,   106,    92,   277,
      94,   255,   267,   272,   245,   118,   284,   118,   121,   118,
     121,   276,   277,   122,   292,   365,     8,   271,    10,   284,
     135,   135,    36,   138,   138,    33,   291,   292,   137,   118,
     299,    60,   129,   130,    36,   118,   133,   387,   236,   136,
     135,   306,   135,   138,   394,   138,   315,   316,   317,   318,
     127,   128,   321,   322,   323,   129,   130,   407,   327,   133,
       6,   118,   331,   332,   333,   334,   335,   336,   337,   118,
      16,   135,    18,    72,   138,   135,   345,   346,   138,   135,
      94,     5,   116,   117,   118,    59,   120,   118,    12,    13,
      36,   121,    94,   118,   372,   134,   129,   130,   352,   122,
     133,   125,   125,   119,   373,    51,   120,   372,   105,   106,
      56,    57,    58,   127,   128,    61,   134,    63,   120,   384,
     116,   117,    46,   118,    70,   127,   128,   134,   134,    72,
     399,    77,   118,   483,   100,   389,   134,   415,    84,   118,
     118,   491,   396,    67,   135,   118,   424,   135,    94,    95,
     415,   118,   135,    99,   119,   409,   506,   118,   118,   424,
     135,   511,   360,   127,   128,   129,   130,   119,   119,   133,
     116,   117,   118,   135,   120,   134,   374,   134,    62,   134,
      60,   127,   128,   139,   134,   109,   110,   111,   122,   135,
     134,    62,   138,   134,   472,   473,   119,   119,   118,   136,
     124,   125,   126,   127,   128,   129,   130,   472,   473,   133,
     135,    46,   107,   411,     6,   134,    26,   135,   135,    41,
     133,   118,   500,   501,    16,   139,    18,   118,   134,   118,
     134,   134,   134,   134,   134,   500,   501,    26,   516,   118,
     518,   139,   136,   136,    36,   122,   136,   139,   136,   518,
     519,   516,   139,   518,   508,   122,   122,   139,   136,    51,
     136,    52,   137,   136,   136,    57,    58,     5,   137,   139,
     134,    63,   121,   136,    12,    13,   122,   139,    70,   122,
     136,   135,   137,   481,    65,    77,   106,   135,   135,   543,
     134,   489,    84,   134,   136,   549,   136,   136,    32,   120,
     554,   137,    94,    95,   137,   137,   504,   137,    46,   134,
     137,   137,   581,   136,   118,   115,   121,   134,   572,   122,
     136,   136,   136,   134,   116,   117,   118,   136,   120,    67,
     137,   585,   100,   134,   612,   127,   128,   535,   137,   136,
     133,   137,   136,   135,   542,    83,   138,   612,   136,   134,
     137,    87,   550,   136,   134,   553,   625,   555,    47,   117,
     558,   136,    52,   641,   633,   134,   137,   129,   646,    33,
     134,   109,   110,   111,    88,   573,   641,   118,   576,   138,
     134,   646,    69,   118,   136,   123,   124,   125,   126,   127,
     128,   129,   130,   743,   117,   133,    41,    91,   136,    65,
     654,    86,    86,    91,   137,   136,   118,   139,   118,   139,
      24,   609,   118,   667,   119,   115,    52,    52,   135,   118,
     618,   686,   118,   136,   622,    59,   719,   565,   632,   515,
     112,   709,   317,   479,   712,   487,    82,     1,   124,   593,
     522,   639,     6,   353,   709,    -1,    10,   712,    -1,    -1,
      14,    15,    16,    17,    18,    -1,    20,    -1,    22,    -1,
      -1,    -1,    -1,    -1,    28,    -1,    -1,    -1,    32,    33,
      -1,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    -1,    -1,    -1,    -1,    -1,    50,    51,    -1,    -1,
      -1,    55,    56,    57,    58,    59,    -1,    61,    -1,    63,
      64,    -1,    66,    -1,    68,    -1,    70,    -1,    -1,    -1,
      -1,    75,    76,    77,    78,    -1,    80,    81,    82,     5,
      84,    85,    86,    87,    88,    -1,    12,    13,    -1,    -1,
      94,    95,    96,    -1,    98,    99,    -1,   101,    -1,    -1,
      -1,    -1,    -1,    -1,   108,    -1,    -1,    -1,    -1,    -1,
       5,    -1,   116,   117,   118,    -1,   120,    12,    13,    -1,
      46,    -1,    -1,   127,   128,    -1,    -1,    -1,    -1,    -1,
      25,   135,   136,    -1,   138,    -1,    -1,    -1,    -1,    -1,
      -1,    67,    -1,   109,   110,   111,    -1,    -1,    -1,    -1,
      -1,    46,    -1,    48,    -1,    -1,    -1,    83,   124,   125,
     126,   127,   128,   129,   130,    -1,    -1,   133,    -1,    -1,
      -1,    -1,    67,    -1,    -1,    -1,    -1,    -1,    -1,     5,
      -1,    -1,    -1,   109,   110,   111,    12,    13,    83,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   123,   124,   125,
     126,   127,   128,   129,   130,    -1,    -1,   133,    -1,    -1,
     136,    -1,    -1,    -1,   109,   110,   111,     5,    -1,    -1,
      46,    -1,    -1,    -1,    12,    13,    -1,    -1,   123,   124,
     125,   126,   127,   128,   129,   130,    -1,    -1,   133,    -1,
      -1,    67,    -1,    -1,    -1,    -1,     5,    -1,    -1,    -1,
      -1,    -1,    -1,    12,    13,    -1,    -1,    83,    46,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    67,
      -1,    -1,    -1,   109,   110,   111,   112,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    83,    -1,   123,   124,   125,
     126,   127,   128,   129,   130,    93,    -1,   133,    67,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    33,     5,    -1,
      -1,   109,   110,   111,    83,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   123,   124,   125,   126,   127,
     128,   129,   130,    -1,    -1,   133,    -1,    -1,     5,    -1,
     109,   110,   111,    -1,    -1,    12,    13,    -1,     5,    46,
      -1,    -1,    -1,    -1,   123,   124,   125,   126,   127,   128,
     129,   130,    -1,    -1,   133,    -1,    92,    -1,    -1,    -1,
      67,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      -1,    -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,    46,
     116,   117,   118,   119,   120,    -1,   122,    -1,    -1,   125,
      67,   127,   128,    -1,    -1,    -1,    -1,    -1,   134,    -1,
      67,   137,   109,   110,   111,   112,    83,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   123,   124,   125,   126,
     127,   128,   129,   130,    -1,    -1,   133,    -1,    -1,    -1,
      -1,    -1,   109,   110,   111,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   109,   110,   111,    20,   123,   124,   125,   126,
     127,   128,   129,   130,    -1,    -1,   133,   124,   125,   126,
     127,   128,   129,   130,    39,    -1,   133,    42,    43,    -1,
      -1,    -1,    -1,    -1,    49,     0,     1,    -1,     3,    -1,
      -1,    -1,    -1,    -1,    59,    -1,    11,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    -1,    -1,    -1,
      -1,    -1,    27,    78,    -1,    -1,    -1,    -1,    -1,    -1,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    42,    -1,    -1,
      45,    96,    -1,    -1,    99,    -1,   101,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,
      -1,    -1,    -1,   118,    -1,    -1,    71,    -1,    73,    74,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    96,    97
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   142,     0,     1,     3,    27,    42,    45,    71,    73,
      74,    96,    97,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   158,   161,   167,   168,   169,
     170,   171,    59,    64,   118,   134,   118,   121,   118,   121,
     269,    59,   121,   269,   197,   120,   264,   201,   202,   201,
      11,    21,    62,   155,   156,   159,   162,   163,   205,   206,
     134,    46,    46,   205,    41,   134,   134,   134,   138,   140,
      46,   134,   134,    33,   118,   198,   270,   265,   266,   267,
     268,   269,   120,   270,   270,    60,   203,   204,   270,   118,
     118,   118,   205,    72,   205,   135,   164,    59,   199,     1,
       6,    10,    14,    15,    16,    18,    20,    22,    28,    35,
      36,    37,    38,    39,    40,    43,    50,    51,    55,    56,
      57,    58,    61,    63,    64,    66,    70,    75,    76,    77,
      78,    80,    81,    82,    84,    85,    86,    87,    88,    94,
      95,    98,    99,   101,   108,   116,   117,   118,   120,   127,
     128,   135,   138,   171,   172,   182,   188,   207,   208,   209,
     210,   211,   213,   214,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   235,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   251,
     254,   255,   256,   259,   262,   263,   266,   268,   276,   284,
      27,   118,   118,   270,   121,   268,   273,   274,   284,   118,
      27,   118,    20,    39,    42,    43,    49,    59,    64,    78,
      85,    96,    99,   101,   118,   125,   266,   119,   122,   268,
     121,   272,   265,   134,   134,   118,   134,   134,    72,   199,
     118,   199,   205,   100,   165,   200,   270,   134,   284,   118,
     215,   135,   138,   135,   138,   205,   135,   118,   228,   266,
     268,   118,   266,   284,   266,   135,   138,   135,   284,   284,
     284,   264,   118,   135,   138,   268,   135,   135,   266,   118,
     119,   271,   135,   138,   135,   266,   268,   268,   118,   135,
     138,   135,   135,   266,   284,   268,   119,   277,   119,   135,
     284,   284,   284,   273,   134,   134,     4,     7,     8,    52,
      53,    54,   102,   103,   104,   105,   106,   137,     5,    12,
      13,    46,    67,    83,   109,   110,   111,   123,   124,   125,
     126,   127,   128,   129,   130,   133,   285,   286,    62,   134,
      60,   217,   134,   134,   139,   122,   112,    62,   134,   119,
     119,   268,   205,   118,   270,    29,    30,   278,   270,   136,
     135,    72,   134,   201,   134,   135,   273,   273,   273,   273,
     270,   266,   135,    46,    93,   273,   273,    44,   120,   268,
     270,   284,   273,   273,   107,   268,   266,    17,    68,   257,
     258,   273,   273,   266,    17,    68,   260,   261,   173,    26,
     183,    41,   273,   273,   120,   268,   266,    17,    68,   252,
     253,    31,   284,   136,   139,   135,   268,   118,   216,   118,
     216,   216,   284,   284,    99,   118,   230,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   134,
     118,   134,   284,   284,   134,   134,   270,   278,   134,   117,
     118,   130,   135,   279,   280,    26,   157,   134,   205,   118,
     273,   136,   139,   136,   139,   136,   266,   284,   205,   136,
     139,   122,   122,   122,   136,   139,   268,   122,   136,   136,
     273,   137,   270,    17,    68,   136,   139,   136,   273,   137,
     270,    17,    68,    52,   177,   284,   134,   121,   136,   139,
     122,   122,   136,   273,   137,   270,    17,    68,   205,   136,
     266,   135,    65,   218,   218,   218,   135,   266,   135,   112,
     134,   106,   160,   280,   129,   130,   133,    36,    94,   120,
     127,   128,   283,   134,   136,   135,   166,   136,   136,    25,
      48,   236,    32,   250,   120,   266,   266,   117,   276,   257,
     137,   205,   273,   137,   260,   137,   205,   273,   137,   216,
     176,   116,   117,   118,   120,   184,   185,   186,   187,   134,
     266,   266,   252,   137,   205,   273,   137,   270,   136,   273,
     118,   115,   219,   219,   266,   252,   232,   266,   268,   284,
      36,    94,   120,   283,   272,   136,   280,   280,   281,   283,
     117,   275,   276,   205,   134,    19,    23,    31,    34,   237,
     205,   270,   122,   136,   136,   136,   270,   205,   137,   205,
     270,   205,   137,   205,   218,    26,    89,   134,   178,    33,
     185,   134,   122,   125,   189,   136,   136,   270,   205,   137,
     205,   100,   136,   284,   136,   270,   134,   137,   275,   134,
     117,   135,   282,   136,   205,   266,   205,   205,   284,   174,
      87,   134,   187,   284,   190,   205,   266,   252,   233,   266,
      47,   117,   270,   136,     1,   179,     1,    33,    90,   118,
     191,   192,   193,   194,    52,   270,   134,   234,   137,   129,
      33,    33,    92,   116,   117,   118,   119,   120,   122,   125,
     127,   128,   134,   137,   180,   181,   134,    88,   118,   138,
     134,    41,   138,   118,   268,   136,    69,   117,    86,    86,
      41,   266,    91,   266,    65,   137,   136,   175,   175,    91,
     139,   118,   139,   118,    24,   118,   119,   195,   115,   212,
      52,    52,   196,   135,   118,   118,   273,   136
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   141,   142,   142,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   144,   145,   145,
     145,   146,   146,   146,   147,   147,   148,   149,   150,   151,
     151,   152,   153,   153,   154,   155,   156,   157,   157,   157,
     157,   157,   158,   159,   160,   160,   160,   160,   160,   161,
     162,   162,   163,   164,   164,   165,   165,   166,   166,   167,
     167,   168,   169,   170,   171,   172,   172,   173,   174,   175,
     176,   176,   177,   177,   178,   178,   179,   179,   180,   180,
     180,   180,   180,   180,   180,   180,   181,   181,   181,   181,
     181,   182,   183,   183,   184,   184,   185,   186,   186,   187,
     187,   187,   187,   189,   188,   190,   190,   190,   191,   191,
     192,   193,   194,   194,   195,   195,   196,   196,   197,   197,
     198,   199,   200,   199,   201,   202,   202,   203,   204,   205,
     206,   206,   206,   206,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   208,   209,   210,   211,   211,   212,   212,
     213,   214,   215,   215,   216,   216,   217,   217,   218,   218,
     219,   219,   220,   221,   221,   221,   222,   222,   223,   224,
     225,   226,   227,   228,   228,   229,   229,   230,   230,   230,
     231,   232,   233,   234,   234,   235,   236,   236,   236,   237,
     237,   237,   237,   238,   238,   239,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   250,   251,   251,
     251,   251,   252,   253,   253,   253,   253,   254,   254,   254,
     254,   254,   255,   256,   256,   257,   258,   258,   258,   258,
     259,   259,   260,   261,   261,   261,   261,   262,   263,   264,
     264,   265,   265,   266,   267,   267,   267,   268,   269,   269,
     269,   270,   270,   270,   270,   270,   270,   270,   270,   270,
     270,   270,   270,   270,   270,   271,   271,   272,   272,   273,
     273,   274,   274,   274,   274,   275,   275,   276,   276,   277,
     277,   278,   278,   278,   279,   279,   280,   280,   280,   280,
     280,   280,   281,   282,   282,   283,   283,   283,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   285,
     285,   285,   285,   285,   285,   286,   286
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     5,     3,     3,
       2,     3,     3,     2,     5,     3,     4,     6,     4,     5,
       6,     4,     5,     6,     6,     6,     2,     0,     3,     2,
       2,     2,     2,     8,     0,     3,     2,     2,     2,     6,
       4,     7,     2,     0,     3,     0,     4,     0,     3,     0,
       1,     5,     2,     2,     3,    11,    11,     0,     0,     0,
       0,     2,     0,     3,     1,     2,     0,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     7,     0,     2,     2,     3,     3,     1,     3,     1,
       1,     1,     1,     0,     9,     0,     3,     3,     1,     1,
       7,     6,     4,     4,     0,     1,     0,     2,     0,     2,
       4,     0,     0,     3,     1,     0,     2,     5,     2,     1,
       0,     3,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     5,     5,     3,    12,     0,     4,
       4,     3,     1,     4,     1,     4,     0,     2,     0,     2,
       0,     2,     2,     8,     6,     6,     6,     6,     4,     2,
       2,     2,     2,     3,     3,     1,     3,     1,     2,     2,
       9,     3,     3,     0,     4,     8,     0,     1,     1,     1,
       1,     1,     1,     2,     4,     2,     2,     2,     2,     3,
       1,     5,     2,     2,     6,     5,     0,     2,     4,     6,
       6,     8,     1,     4,     3,     5,     4,     1,     1,     1,
       1,     2,     2,     4,     6,     1,     4,     3,     5,     4,
       4,     6,     1,     4,     3,     5,     4,     3,     3,     2,
       3,     2,     3,     1,     1,     3,     2,     2,     1,     3,
       4,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     0,     1,     0,     1,     1,
       0,     1,     3,     3,     5,     1,     1,     2,     2,     0,
       1,     2,     1,     0,     1,     1,     1,     1,     3,     3,
       3,     3,     2,     1,     5,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     3,     3,     3,     3,
       3,     2,     2,     6,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     3,     1,
       1,     1,     1,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


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
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
      if (yytable_value_is_error (yyn))
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 16: /* definition: error  */
#line 787 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in definition.");
	}
#line 2627 "ascend/compiler/ascParse.c"
    break;

  case 17: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 794 "ascend/compiler/ascParse.y"
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
    }
#line 2673 "ascend/compiler/ascParse.c"
    break;

  case 18: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 839 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 2681 "ascend/compiler/ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK name ';'  */
#line 843 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2690 "ascend/compiler/ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name  */
#line 848 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2699 "ascend/compiler/ascParse.c"
    break;

  case 21: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 856 "ascend/compiler/ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 2707 "ascend/compiler/ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK name ';'  */
#line 860 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2716 "ascend/compiler/ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name  */
#line 865 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2725 "ascend/compiler/ascParse.c"
    break;

  case 24: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 873 "ascend/compiler/ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
            );
          }
	}
#line 2738 "ascend/compiler/ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 882 "ascend/compiler/ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 2751 "ascend/compiler/ascParse.c"
    break;

  case 26: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 894 "ascend/compiler/ascParse.y"
        {
	  /*  see comments for notes statement.  */
	  if( (yyvsp[-1].int_value) != NOTES_TOK ) {
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
	}
#line 2783 "ascend/compiler/ascParse.c"
    break;

  case 27: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 925 "ascend/compiler/ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 2792 "ascend/compiler/ascParse.c"
    break;

  case 28: /* add_method_def: add_method_head proclist end ';'  */
#line 933 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-3].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[-2].listp));
	  } else {
	    if( (yyvsp[-1].int_value) != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("ADD METHODS", NULL, (yyvsp[-1].int_value), "METHODS");
	    }
	    if (AddMethods((yyvsp[-3].tptr),(yyvsp[-2].listp),g_untrapped_error) != 0) {
	      if ((yyvsp[-3].tptr) != ILLEGAL_DEFINITION) {
	            error_reporter_current_line(ASC_USER_ERROR
	        ,"ADD METHODS failed for type %s"
	        ,SCP(GetName((yyvsp[-3].tptr)))
	      );
	      DestroyProcedureList((yyvsp[-2].listp));
	      } /* else adding in DEFINITION MODEL may have misgone */
	    }
	  }
	  g_untrapped_error = 0;
	}
#line 2816 "ascend/compiler/ascParse.c"
    break;

  case 29: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 956 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-1].id_ptr));
	  if(tmptype == NULL){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"ADD METHODS called with undefined type (%s)"
	      ,SCP((yyvsp[-1].id_ptr))
	    );
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	  g_type_name = (yyvsp[-1].id_ptr); /* scope for notes */
	}
#line 2833 "ascend/compiler/ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 969 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 2842 "ascend/compiler/ascParse.c"
    break;

  case 31: /* replace_method_def: replace_method_head proclist end ';'  */
#line 977 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-3].tptr) == NULL) {
	    DestroyProcedureList((yyvsp[-2].listp));
	  } else {
	    if( (yyvsp[-1].int_value) != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("REPLACE METHODS", NULL, (yyvsp[-1].int_value), "METHODS");
	    }
	    if (ReplaceMethods((yyvsp[-3].tptr),(yyvsp[-2].listp),g_untrapped_error) != 0) {
	      error_reporter_current_line(ASC_USER_ERROR
	        ,"REPLACE METHODS failed for type %s"
	        ,SCP(GetName((yyvsp[-3].tptr)))
	      );
	      DestroyProcedureList((yyvsp[-2].listp));
	    }
	  }
	  g_untrapped_error = 0;
	}
#line 2864 "ascend/compiler/ascParse.c"
    break;

  case 32: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 998 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-1].id_ptr));
	  if (tmptype == NULL) {
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"REPLACE METHODS called with undefined type (%s)"
	      ,SCP((yyvsp[-1].id_ptr))
	    );
	  }
	  (yyval.tptr) = tmptype; /* parent should check for NULL */
	}
#line 2880 "ascend/compiler/ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1010 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 2889 "ascend/compiler/ascParse.c"
    break;

  case 34: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 1018 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (yyvsp[-1].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
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
	    error_reporter(ASC_USER_ERROR,Asc_ModuleBestName(Asc_CurrentModule()),g_header_linenum,NULL
	      ,"Atom dimensions don't match in ATOM %s"
	      ,SCP(g_type_name)
	    );
	    DestroyStatementList((yyvsp[-3].slptr));
	    DestroyProcedureList((yyvsp[-2].listp));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
#line 2942 "ascend/compiler/ascParse.c"
    break;

  case 35: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 1070 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 2954 "ascend/compiler/ascParse.c"
    break;

  case 36: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 1081 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 2963 "ascend/compiler/ascParse.c"
    break;

  case 37: /* default_val: %empty  */
#line 1089 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
#line 2973 "ascend/compiler/ascParse.c"
    break;

  case 38: /* default_val: DEFAULT_TOK optional_sign number  */
#line 1095 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_defaulted = 1;
	}
#line 2982 "ascend/compiler/ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 1100 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 2993 "ascend/compiler/ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 1107 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 3004 "ascend/compiler/ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 1114 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 3015 "ascend/compiler/ascParse.c"
    break;

  case 42: /* constant_def: universal constant_head  */
#line 1124 "ascend/compiler/ascParse.y"
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
	}
#line 3055 "ascend/compiler/ascParse.c"
    break;

  case 43: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK dims constant_val optional_notes ';'  */
#line 1164 "ascend/compiler/ascParse.y"
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
	}
#line 3089 "ascend/compiler/ascParse.c"
    break;

  case 44: /* constant_val: %empty  */
#line 1197 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
#line 3099 "ascend/compiler/ascParse.c"
    break;

  case 45: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 1203 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_defaulted = 1;
	}
#line 3108 "ascend/compiler/ascParse.c"
    break;

  case 46: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 1208 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3119 "ascend/compiler/ascParse.c"
    break;

  case 47: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 1215 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3130 "ascend/compiler/ascParse.c"
    break;

  case 48: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 1222 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 3141 "ascend/compiler/ascParse.c"
    break;

  case 49: /* model_def: universal model_head fstatements methods end ';'  */
#line 1232 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;
	  if(( (yyvsp[-1].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
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
	}
#line 3183 "ascend/compiler/ascParse.c"
    break;

  case 50: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 1274 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 3195 "ascend/compiler/ascParse.c"
    break;

  case 51: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 1283 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 3208 "ascend/compiler/ascParse.c"
    break;

  case 52: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 1295 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3217 "ascend/compiler/ascParse.c"
    break;

  case 53: /* optional_model_parameters: %empty  */
#line 1303 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3225 "ascend/compiler/ascParse.c"
    break;

  case 54: /* optional_model_parameters: '(' fstatements ')'  */
#line 1307 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3233 "ascend/compiler/ascParse.c"
    break;

  case 55: /* optional_parameter_wheres: %empty  */
#line 1314 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3241 "ascend/compiler/ascParse.c"
    break;

  case 56: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 1318 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3249 "ascend/compiler/ascParse.c"
    break;

  case 57: /* optional_parameter_reduction: %empty  */
#line 1325 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3257 "ascend/compiler/ascParse.c"
    break;

  case 58: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 1329 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3265 "ascend/compiler/ascParse.c"
    break;

  case 59: /* universal: %empty  */
#line 1379 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3273 "ascend/compiler/ascParse.c"
    break;

  case 60: /* universal: UNIVERSAL_TOK  */
#line 1383 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3281 "ascend/compiler/ascParse.c"
    break;

  case 61: /* definition_def: definition_id fstatements methods end ';'  */
#line 1390 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( (yyvsp[-1].int_value) != IDENTIFIER_TOK ) || ( g_end_identifier != (yyvsp[-4].id_ptr) )) {
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
	}
#line 3312 "ascend/compiler/ascParse.c"
    break;

  case 62: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1420 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3321 "ascend/compiler/ascParse.c"
    break;

  case 63: /* units_def: units_statement ';'  */
#line 1429 "ascend/compiler/ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 3327 "ascend/compiler/ascParse.c"
    break;

  case 64: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1434 "ascend/compiler/ascParse.y"
        {
	  struct UnitDefinition *ud;
	  unsigned long c,len;

	  if( (yyvsp[0].int_value) != UNITS_TOK ) {
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
	}
#line 3348 "ascend/compiler/ascParse.c"
    break;

  case 65: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on table_body END_TOK TABLE_TOK table_mode_off  */
#line 1454 "ascend/compiler/ascParse.y"
        {
	  char *table_body;
	  table_body = TableParseFinish();
	  (yyval.statptr) = CreateTABLE((yyvsp[-9].nptr),
	                   g_table_parse.decl_type,
	                   g_table_parse.decl_typeargs,
	                   g_table_parse.decl_set_type,
	                   g_table_parse.default_expr,
	                   g_table_parse.positional,
	                   g_table_parse.rows,
	                   g_table_parse.scalars,
	                   g_table_parse.items,
	                   table_body);
	  g_table_parse.decl_type = NULL;
	  g_table_parse.decl_typeargs = NULL;
	  g_table_parse.decl_set_type = NULL;
	  g_table_parse.default_expr = NULL;
	}
#line 3371 "ascend/compiler/ascParse.c"
    break;

  case 66: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on error END_TOK TABLE_TOK table_mode_off  */
#line 1473 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-9].nptr));
	  TableParseAbort();
	  ErrMsg_Generic("Error in TABLE body.");
	  g_untrapped_error++;
	  yyerrok;
	  (yyval.statptr) = NULL;
	}
#line 3384 "ascend/compiler/ascParse.c"
    break;

  case 67: /* table_begin: %empty  */
#line 1485 "ascend/compiler/ascParse.y"
        {
	  TableParseBegin();
	}
#line 3392 "ascend/compiler/ascParse.c"
    break;

  case 68: /* table_mode_on: %empty  */
#line 1492 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(1);
	}
#line 3400 "ascend/compiler/ascParse.c"
    break;

  case 69: /* table_mode_off: %empty  */
#line 1499 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(0);
	}
#line 3408 "ascend/compiler/ascParse.c"
    break;

  case 72: /* table_decl_opt: %empty  */
#line 1511 "ascend/compiler/ascParse.y"
        {
	  g_table_parse.decl_type = NULL;
	  g_table_parse.decl_set_type = NULL;
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_typeargs = NULL;
	}
#line 3421 "ascend/compiler/ascParse.c"
    break;

  case 73: /* table_decl_opt: ISA_TOK type_identifier optional_of  */
#line 1520 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_type = (yyvsp[-1].id_ptr);
	  g_table_parse.decl_set_type = (yyvsp[0].id_ptr);
	  g_table_parse.decl_typeargs = g_typeargs;
	  g_typeargs = NULL;
	}
#line 3435 "ascend/compiler/ascParse.c"
    break;

  case 74: /* table_option: POSITIONAL_TOK  */
#line 1533 "ascend/compiler/ascParse.y"
        {
	  g_table_parse.positional = 1;
	}
#line 3443 "ascend/compiler/ascParse.c"
    break;

  case 75: /* table_option: DEFAULT_TOK expr  */
#line 1537 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.default_expr != NULL) {
	    DestroyExprList(g_table_parse.default_expr);
	  }
	  g_table_parse.default_expr = (yyvsp[0].eptr);
	}
#line 3454 "ascend/compiler/ascParse.c"
    break;

  case 79: /* table_body_item: ':'  */
#line 1553 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(":",0);
	}
#line 3462 "ascend/compiler/ascParse.c"
    break;

  case 80: /* table_body_item: '='  */
#line 1557 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("=",0);
	}
#line 3470 "ascend/compiler/ascParse.c"
    break;

  case 81: /* table_body_item: ','  */
#line 1561 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(",",0);
	}
#line 3478 "ascend/compiler/ascParse.c"
    break;

  case 82: /* table_body_item: '+'  */
#line 1565 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("+",0);
	}
#line 3486 "ascend/compiler/ascParse.c"
    break;

  case 83: /* table_body_item: '-'  */
#line 1569 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("-",0);
	}
#line 3494 "ascend/compiler/ascParse.c"
    break;

  case 84: /* table_body_item: ';'  */
#line 1573 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3502 "ascend/compiler/ascParse.c"
    break;

  case 85: /* table_body_item: EOL_TOK  */
#line 1577 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3510 "ascend/compiler/ascParse.c"
    break;

  case 86: /* table_scalar: IDENTIFIER_TOK  */
#line 1584 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(SCP((yyvsp[0].id_ptr)),1);
	}
#line 3518 "ascend/compiler/ascParse.c"
    break;

  case 87: /* table_scalar: SYMBOL_TOK  */
#line 1588 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendSymbol((yyvsp[0].sym_ptr));
	}
#line 3526 "ascend/compiler/ascParse.c"
    break;

  case 88: /* table_scalar: INTEGER_TOK  */
#line 1592 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendInteger((yyvsp[0].int_value));
	}
#line 3534 "ascend/compiler/ascParse.c"
    break;

  case 89: /* table_scalar: REAL_TOK  */
#line 1596 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendReal((yyvsp[0].real_value));
	}
#line 3542 "ascend/compiler/ascParse.c"
    break;

  case 90: /* table_scalar: BRACEDTEXT_TOK  */
#line 1600 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendBraced((yyvsp[0].braced_ptr));
	}
#line 3550 "ascend/compiler/ascParse.c"
    break;

  case 91: /* values_statement: VALUES_TOK fname values_default_opt ';' values_entries END_TOK VALUES_TOK  */
#line 1607 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-5].nptr));
	  (yyval.statptr) = NULL;
	}
#line 3559 "ascend/compiler/ascParse.c"
    break;

  case 93: /* values_default_opt: DEFAULT_TOK expr  */
#line 1616 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3567 "ascend/compiler/ascParse.c"
    break;

  case 96: /* values_entry: values_key_list '=' expr  */
#line 1628 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3575 "ascend/compiler/ascParse.c"
    break;

  case 103: /* $@1: %empty  */
#line 1647 "ascend/compiler/ascParse.y"
        {
	  DatasetParseBegin((yyvsp[-3].id_ptr),(yyvsp[-1].dquote_ptr));
	}
#line 3583 "ascend/compiler/ascParse.c"
    break;

  case 104: /* dataset_statement: DATASET_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';' $@1 dataset_items END_TOK DATASET_TOK  */
#line 1651 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = DatasetParseFinish();
	}
#line 3591 "ascend/compiler/ascParse.c"
    break;

  case 107: /* dataset_items: dataset_items error ';'  */
#line 1660 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in DATASET item.");
	  ErrMsg_Generic("Check DATASET statement syntax.");
	  g_untrapped_error++;
	  yyerrok;
	}
#line 3602 "ascend/compiler/ascParse.c"
    break;

  case 110: /* dataset_index_item: INDEX_TOK IDENTIFIER_TOK FROM_TOK COLUMN_TOK IDENTIFIER_TOK ISA_TOK IDENTIFIER_TOK  */
#line 1675 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddIndex((yyvsp[-5].id_ptr),(yyvsp[-2].id_ptr),(yyvsp[0].id_ptr));
	}
#line 3610 "ascend/compiler/ascParse.c"
    break;

  case 111: /* dataset_map_item: dataset_target FROM_TOK COLUMN_TOK IDENTIFIER_TOK dataset_units_opt dataset_type_opt  */
#line 1682 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-5].nptr),(yyvsp[-2].id_ptr),(yyvsp[-1].braced_ptr),(yyvsp[0].id_ptr));
	}
#line 3618 "ascend/compiler/ascParse.c"
    break;

  case 112: /* dataset_target: IDENTIFIER_TOK '[' fvarlist ']'  */
#line 1689 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices(CreateIdName((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 3627 "ascend/compiler/ascParse.c"
    break;

  case 113: /* dataset_target: dataset_target '[' fvarlist ']'  */
#line 1694 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices((yyvsp[-3].nptr),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 3636 "ascend/compiler/ascParse.c"
    break;

  case 114: /* dataset_units_opt: %empty  */
#line 1702 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 3644 "ascend/compiler/ascParse.c"
    break;

  case 115: /* dataset_units_opt: BRACEDTEXT_TOK  */
#line 1706 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 3652 "ascend/compiler/ascParse.c"
    break;

  case 116: /* dataset_type_opt: %empty  */
#line 1713 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 3660 "ascend/compiler/ascParse.c"
    break;

  case 117: /* dataset_type_opt: ISA_TOK IDENTIFIER_TOK  */
#line 1717 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3668 "ascend/compiler/ascParse.c"
    break;

  case 118: /* unitdeflist: %empty  */
#line 1723 "ascend/compiler/ascParse.y"
                {
		  (yyval.listp) = gl_create(100L);
	}
#line 3676 "ascend/compiler/ascParse.c"
    break;

  case 119: /* unitdeflist: unitdeflist unitdef  */
#line 1727 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 3685 "ascend/compiler/ascParse.c"
    break;

  case 120: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1735 "ascend/compiler/ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 3694 "ascend/compiler/ascParse.c"
    break;

  case 121: /* methods: %empty  */
#line 1744 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 3702 "ascend/compiler/ascParse.c"
    break;

  case 122: /* $@2: %empty  */
#line 1748 "ascend/compiler/ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 3713 "ascend/compiler/ascParse.c"
    break;

  case 123: /* methods: METHODS_TOK $@2 proclist  */
#line 1755 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 3721 "ascend/compiler/ascParse.c"
    break;

  case 124: /* proclist: proclistf  */
#line 1762 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 3730 "ascend/compiler/ascParse.c"
    break;

  case 125: /* proclistf: %empty  */
#line 1769 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 3738 "ascend/compiler/ascParse.c"
    break;

  case 126: /* proclistf: proclistf procedure  */
#line 1773 "ascend/compiler/ascParse.y"
        {
	  unsigned long c;
	  struct InitProcedure *oldproc;
	  c = gl_length((yyvsp[-1].listp));
	  while (c > 0) {
	    oldproc = (struct InitProcedure *)gl_fetch((yyvsp[-1].listp),c);
	    if (ProcName((yyvsp[0].procptr)) == ProcName(oldproc)) {
	      error_reporter_current_line(ASC_USER_WARNING
	        ,"Duplicate METHOD %s rejected", SCP(ProcName((yyvsp[0].procptr)))
	      );
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
	}
#line 3764 "ascend/compiler/ascParse.c"
    break;

  case 127: /* procedure: procedure_id ';' fstatements end ';'  */
#line 1798 "ascend/compiler/ascParse.y"
        {
	  if (((yyvsp[-1].int_value) != IDENTIFIER_TOK) || ((yyvsp[-4].id_ptr) != g_end_identifier)) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("METHOD", SCP((yyvsp[-4].id_ptr)), (yyvsp[-1].int_value), SCP((yyvsp[-4].id_ptr)));
	  }
	  (yyval.procptr) = CreateProcedure((yyvsp[-4].id_ptr),(yyvsp[-2].slptr));
	  g_proc_name = NULL;
	}
#line 3779 "ascend/compiler/ascParse.c"
    break;

  case 128: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 1812 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 3788 "ascend/compiler/ascParse.c"
    break;

  case 129: /* fstatements: statements  */
#line 1821 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 3796 "ascend/compiler/ascParse.c"
    break;

  case 130: /* statements: %empty  */
#line 1828 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 3804 "ascend/compiler/ascParse.c"
    break;

  case 131: /* statements: statements statement ';'  */
#line 1832 "ascend/compiler/ascParse.y"
        {
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ((yyvsp[-1].statptr) != NULL) {
	    if (StatementType((yyvsp[-1].statptr)) == TABLESTAT && (yyvsp[-1].statptr)->v.table.decl_type != NULL) {
	      struct Statement *decl;
	      struct VariableList *vl;
	      vl = CreateVariableNode(TableDeclNameFromTarget((yyvsp[-1].statptr)->v.table.name));
	      decl = CreateISA(vl
	        ,(yyvsp[-1].statptr)->v.table.decl_type
	        ,CopySetList((yyvsp[-1].statptr)->v.table.decl_typeargs)
	        ,(yyvsp[-1].statptr)->v.table.decl_set_type
	      );
	      decl->mod = (yyvsp[-1].statptr)->mod;
	      decl->linenum = (yyvsp[-1].statptr)->linenum;
	      decl->context = (yyvsp[-1].statptr)->context;
	      gl_append_ptr((yyvsp[-2].listp),(char *)decl);
	    }
	    if (StatementType((yyvsp[-1].statptr)) == DATASETSTAT) {
	      struct DatasetIndexItem *idx;
	      struct DatasetMapItem *map;
	      for (idx = (yyvsp[-1].statptr)->v.dataset.indices; idx != NULL; idx = idx->next) {
	        if (idx->type_name != NULL) {
	          struct Statement *decl;
	          struct VariableList *vl;
	          struct Name *setname = CreateIdName(idx->set_name);
	          if (!StatementListHasTypeDeclForName((yyvsp[-2].listp),setname)) {
	            vl = CreateVariableNode(setname);
	            decl = CreateISA(vl
	              ,GetBaseTypeName(set_type)
	              ,NULL
	              ,idx->type_name
	            );
	            decl->mod = (yyvsp[-1].statptr)->mod;
	            decl->linenum = (yyvsp[-1].statptr)->linenum;
	            decl->context = (yyvsp[-1].statptr)->context;
	            gl_append_ptr((yyvsp[-2].listp),(char *)decl);
	          } else {
	            DestroyName(setname);
	          }
	        }
	      }
	      for (map = (yyvsp[-1].statptr)->v.dataset.maps; map != NULL; map = map->next) {
	        if (map->type_name != NULL) {
	          struct Statement *decl;
	          struct VariableList *vl;
	          if (!StatementListHasTypeDeclForName((yyvsp[-2].listp),map->target)) {
	            vl = CreateVariableNode(CopyName(map->target));
	            decl = CreateISA(vl
	              ,map->type_name
	              ,NULL
	              ,NULL
	            );
	            decl->mod = (yyvsp[-1].statptr)->mod;
	            decl->linenum = (yyvsp[-1].statptr)->linenum;
	            decl->context = (yyvsp[-1].statptr)->context;
	            gl_append_ptr((yyvsp[-2].listp),(char *)decl);
	          }
	        }
	      }
	    }
	    gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].statptr));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 3873 "ascend/compiler/ascParse.c"
    break;

  case 132: /* statements: statements complex_statement ';'  */
#line 1897 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
            gl_destroy((yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 3885 "ascend/compiler/ascParse.c"
    break;

  case 133: /* statements: statements error ';'  */
#line 1905 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 3894 "ascend/compiler/ascParse.c"
    break;

  case 174: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 1961 "ascend/compiler/ascParse.y"
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

	}
#line 3936 "ascend/compiler/ascParse.c"
    break;

  case 175: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 2002 "ascend/compiler/ascParse.y"
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
	}
#line 3966 "ascend/compiler/ascParse.c"
    break;

  case 176: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 2031 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 3974 "ascend/compiler/ascParse.c"
    break;

  case 177: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 2036 "ascend/compiler/ascParse.y"
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
	}
#line 4024 "ascend/compiler/ascParse.c"
    break;

  case 178: /* optional_set_values: %empty  */
#line 2085 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4032 "ascend/compiler/ascParse.c"
    break;

  case 179: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 2089 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 4040 "ascend/compiler/ascParse.c"
    break;

  case 180: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 2096 "ascend/compiler/ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 4053 "ascend/compiler/ascParse.c"
    break;

  case 181: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 2108 "ascend/compiler/ascParse.y"
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
	}
#line 4081 "ascend/compiler/ascParse.c"
    break;

  case 182: /* call_identifier: IDENTIFIER_TOK  */
#line 2135 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 4090 "ascend/compiler/ascParse.c"
    break;

  case 183: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2140 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 4099 "ascend/compiler/ascParse.c"
    break;

  case 184: /* type_identifier: IDENTIFIER_TOK  */
#line 2148 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 4108 "ascend/compiler/ascParse.c"
    break;

  case 185: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2153 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 4117 "ascend/compiler/ascParse.c"
    break;

  case 186: /* optional_method: %empty  */
#line 2161 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4125 "ascend/compiler/ascParse.c"
    break;

  case 187: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 2165 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4133 "ascend/compiler/ascParse.c"
    break;

  case 188: /* optional_of: %empty  */
#line 2172 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4141 "ascend/compiler/ascParse.c"
    break;

  case 189: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 2176 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4149 "ascend/compiler/ascParse.c"
    break;

  case 190: /* optional_with_value: %empty  */
#line 2183 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 4157 "ascend/compiler/ascParse.c"
    break;

  case 191: /* optional_with_value: WITH_VALUE_T expr  */
#line 2187 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4165 "ascend/compiler/ascParse.c"
    break;

  case 192: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 2194 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 4173 "ascend/compiler/ascParse.c"
    break;

  case 193: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 2201 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4181 "ascend/compiler/ascParse.c"
    break;

  case 194: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2205 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4189 "ascend/compiler/ascParse.c"
    break;

  case 195: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 2209 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4197 "ascend/compiler/ascParse.c"
    break;

  case 196: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2216 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4205 "ascend/compiler/ascParse.c"
    break;

  case 197: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 2220 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4213 "ascend/compiler/ascParse.c"
    break;

  case 198: /* der_statement: DER_TOK '(' fvarlist ')'  */
#line 2227 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 4223 "ascend/compiler/ascParse.c"
    break;

  case 199: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 2236 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 4233 "ascend/compiler/ascParse.c"
    break;

  case 200: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 2245 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 4241 "ascend/compiler/ascParse.c"
    break;

  case 201: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 2252 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 4249 "ascend/compiler/ascParse.c"
    break;

  case 202: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 2259 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 4257 "ascend/compiler/ascParse.c"
    break;

  case 203: /* assignment_statement: fname ASSIGN_TOK expr  */
#line 2266 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4265 "ascend/compiler/ascParse.c"
    break;

  case 204: /* assignment_statement: fname CASSIGN_TOK expr  */
#line 2270 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4273 "ascend/compiler/ascParse.c"
    break;

  case 205: /* relation_statement: relation  */
#line 2277 "ascend/compiler/ascParse.y"
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
	}
#line 4290 "ascend/compiler/ascParse.c"
    break;

  case 206: /* relation_statement: fname ':' relation  */
#line 2290 "ascend/compiler/ascParse.y"
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
	}
#line 4308 "ascend/compiler/ascParse.c"
    break;

  case 207: /* relation: expr  */
#line 2307 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 4322 "ascend/compiler/ascParse.c"
    break;

  case 208: /* relation: MINIMIZE_TOK expr  */
#line 2317 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4334 "ascend/compiler/ascParse.c"
    break;

  case 209: /* relation: MAXIMIZE_TOK expr  */
#line 2325 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4346 "ascend/compiler/ascParse.c"
    break;

  case 210: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 2336 "ascend/compiler/ascParse.y"
    {
      /*
       * This is the blackbox declarative external relation.
       */
      	struct VariableList *vl;
	/*determine the number of variables declared in input_args and output_args*/
      	unsigned long n_inputs, n_outputs;
	n_inputs = VariableListLength((yyvsp[-4].lptr));
	n_outputs = VariableListLength((yyvsp[-2].lptr));
	/*continue with normal parsing process */
	vl = JoinVariableLists((yyvsp[-4].lptr),(yyvsp[-2].lptr)); 
	/* $$ = CreateEXTERN(2,$1,SCP($3),vl,$8,NULL); */

      /*$$ = CreateEXTERNBlackBox($1,SCP($3),vl,$8); //original */
      //statement now also knows how many of the variables in vl are inputs/outputs
      (yyval.statptr) = CreateEXTERNBlackBox((yyvsp[-8].nptr),SCP((yyvsp[-6].id_ptr)),vl,(yyvsp[-1].nptr),n_inputs,n_outputs); 
    }
#line 4368 "ascend/compiler/ascParse.c"
    break;

  case 211: /* input_args: fvarlist ':' INPUT_TOK  */
#line 2357 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4376 "ascend/compiler/ascParse.c"
    break;

  case 212: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 2364 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4384 "ascend/compiler/ascParse.c"
    break;

  case 213: /* data_args: %empty  */
#line 2371 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 4392 "ascend/compiler/ascParse.c"
    break;

  case 214: /* data_args: ';' fname ':' DATA_TOK  */
#line 2375 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 4400 "ascend/compiler/ascParse.c"
    break;

  case 215: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 2413 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != FOR_TOK ) {
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
	}
#line 4421 "ascend/compiler/ascParse.c"
    break;

  case 216: /* optional_direction: %empty  */
#line 2433 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 4429 "ascend/compiler/ascParse.c"
    break;

  case 217: /* optional_direction: INCREASING_TOK  */
#line 2437 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 4437 "ascend/compiler/ascParse.c"
    break;

  case 218: /* optional_direction: DECREASING_TOK  */
#line 2441 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 4445 "ascend/compiler/ascParse.c"
    break;

  case 219: /* forexprend: CREATE_TOK  */
#line 2448 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 4453 "ascend/compiler/ascParse.c"
    break;

  case 220: /* forexprend: EXPECT_TOK  */
#line 2452 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 4461 "ascend/compiler/ascParse.c"
    break;

  case 221: /* forexprend: CHECK_TOK  */
#line 2456 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 4469 "ascend/compiler/ascParse.c"
    break;

  case 222: /* forexprend: DO_TOK  */
#line 2460 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 4477 "ascend/compiler/ascParse.c"
    break;

  case 223: /* run_statement: RUN_TOK fname  */
#line 2467 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 4485 "ascend/compiler/ascParse.c"
    break;

  case 224: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 2471 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 4493 "ascend/compiler/ascParse.c"
    break;

  case 225: /* fix_statement: FIX_TOK fvarlist  */
#line 2478 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 4502 "ascend/compiler/ascParse.c"
    break;

  case 226: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 2486 "ascend/compiler/ascParse.y"
        {
		struct Statement *assign = (yyvsp[0].statptr);
		struct Name *n = CopyName((yyvsp[0].statptr) -> v.asgn.nptr);
		struct VariableList *vars = CreateVariableNode(n);
		struct Statement *fix = CreateFIX(ReverseVariableList(vars));
		struct gl_list_t *fix_and_assign = gl_create(7L);
		gl_append_ptr(fix_and_assign,(char*)fix);
		gl_append_ptr(fix_and_assign,(char*)assign);
		(yyval.listp) = fix_and_assign;
	}
#line 4517 "ascend/compiler/ascParse.c"
    break;

  case 227: /* free_statement: FREE_TOK fvarlist  */
#line 2500 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 4525 "ascend/compiler/ascParse.c"
    break;

  case 228: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 2507 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 4534 "ascend/compiler/ascParse.c"
    break;

  case 229: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 2515 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 4543 "ascend/compiler/ascParse.c"
    break;

  case 230: /* solve_statement: SOLVE_TOK  */
#line 2523 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE();
	}
#line 4552 "ascend/compiler/ascParse.c"
    break;

  case 231: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 2531 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 4564 "ascend/compiler/ascParse.c"
    break;

  case 232: /* call_statement: CALL_TOK call_identifier  */
#line 2542 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 4576 "ascend/compiler/ascParse.c"
    break;

  case 233: /* assert_statement: ASSERT_TOK expr  */
#line 2553 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 4584 "ascend/compiler/ascParse.c"
    break;

  case 234: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 2559 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 4595 "ascend/compiler/ascParse.c"
    break;

  case 235: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 2569 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 4606 "ascend/compiler/ascParse.c"
    break;

  case 236: /* optional_else: %empty  */
#line 2578 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 4614 "ascend/compiler/ascParse.c"
    break;

  case 237: /* optional_else: ELSE_TOK fstatements  */
#line 2582 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 4622 "ascend/compiler/ascParse.c"
    break;

  case 238: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 2589 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[-1].wptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 4637 "ascend/compiler/ascParse.c"
    break;

  case 239: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 2600 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList((yyvsp[-1].wptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  DestroyName((yyvsp[-5].nptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 4653 "ascend/compiler/ascParse.c"
    break;

  case 240: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2612 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 4664 "ascend/compiler/ascParse.c"
    break;

  case 241: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2619 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 4675 "ascend/compiler/ascParse.c"
    break;

  case 242: /* whenlist: whenlistf  */
#line 2629 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 4683 "ascend/compiler/ascParse.c"
    break;

  case 243: /* whenlistf: CASE_TOK set ':' fstatements  */
#line 2636 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 4691 "ascend/compiler/ascParse.c"
    break;

  case 244: /* whenlistf: OTHERWISE_TOK ':' fstatements  */
#line 2640 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	}
#line 4699 "ascend/compiler/ascParse.c"
    break;

  case 245: /* whenlistf: whenlistf CASE_TOK set ':' fstatements  */
#line 2644 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	}
#line 4707 "ascend/compiler/ascParse.c"
    break;

  case 246: /* whenlistf: whenlistf OTHERWISE_TOK ':' fstatements  */
#line 2648 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	}
#line 4715 "ascend/compiler/ascParse.c"
    break;

  case 247: /* flow_statement: BREAK_TOK  */
#line 2655 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 4723 "ascend/compiler/ascParse.c"
    break;

  case 248: /* flow_statement: CONTINUE_TOK  */
#line 2659 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 4731 "ascend/compiler/ascParse.c"
    break;

  case 249: /* flow_statement: FALLTHRU_TOK  */
#line 2663 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 4739 "ascend/compiler/ascParse.c"
    break;

  case 250: /* flow_statement: RETURN_TOK  */
#line 2667 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 4747 "ascend/compiler/ascParse.c"
    break;

  case 251: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 2671 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 4755 "ascend/compiler/ascParse.c"
    break;

  case 252: /* use_statement: USE_TOK fname  */
#line 2678 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 4763 "ascend/compiler/ascParse.c"
    break;

  case 253: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 2685 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SELECT statement.");
	  DestroySelectList((yyvsp[-1].septr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 4778 "ascend/compiler/ascParse.c"
    break;

  case 254: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 2696 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 4789 "ascend/compiler/ascParse.c"
    break;

  case 255: /* selectlist: selectlistf  */
#line 2706 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 4797 "ascend/compiler/ascParse.c"
    break;

  case 256: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 2713 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 4805 "ascend/compiler/ascParse.c"
    break;

  case 257: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 2717 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 4813 "ascend/compiler/ascParse.c"
    break;

  case 258: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 2721 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 4821 "ascend/compiler/ascParse.c"
    break;

  case 259: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 2725 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 4829 "ascend/compiler/ascParse.c"
    break;

  case 260: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 2732 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  ErrMsg_Generic("() missing in SWITCH statement.");
	  DestroySwitchList((yyvsp[-1].swptr));
	  DestroyVariableList((yyvsp[-2].lptr));
	  g_untrapped_error++;
	  (yyval.statptr) = NULL;
	}
#line 4844 "ascend/compiler/ascParse.c"
    break;

  case 261: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 2743 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 4855 "ascend/compiler/ascParse.c"
    break;

  case 262: /* switchlist: switchlistf  */
#line 2753 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 4863 "ascend/compiler/ascParse.c"
    break;

  case 263: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 2760 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 4871 "ascend/compiler/ascParse.c"
    break;

  case 264: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 2764 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 4879 "ascend/compiler/ascParse.c"
    break;

  case 265: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 2768 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 4887 "ascend/compiler/ascParse.c"
    break;

  case 266: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 2772 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 4895 "ascend/compiler/ascParse.c"
    break;

  case 267: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 2779 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 4906 "ascend/compiler/ascParse.c"
    break;

  case 268: /* notes_statement: NOTES_TOK notes_body end  */
#line 2789 "ascend/compiler/ascParse.y"
        {
	  /*  All processing of notes takes place on the notes_body here.
	   *  Notes should NOT be added to the statement list.
	   *  Here we know the current type and method names.
	   */
	  if( (yyvsp[0].int_value) != NOTES_TOK ) {
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
	}
#line 4944 "ascend/compiler/ascParse.c"
    break;

  case 269: /* notes_body: SYMBOL_TOK noteslist  */
#line 2826 "ascend/compiler/ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 4957 "ascend/compiler/ascParse.c"
    break;

  case 270: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 2835 "ascend/compiler/ascParse.y"
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
	}
#line 4973 "ascend/compiler/ascParse.c"
    break;

  case 271: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 2850 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 4982 "ascend/compiler/ascParse.c"
    break;

  case 272: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 2855 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 4992 "ascend/compiler/ascParse.c"
    break;

  case 273: /* fvarlist: varlist  */
#line 2864 "ascend/compiler/ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5004 "ascend/compiler/ascParse.c"
    break;

  case 274: /* varlist: fname  */
#line 2875 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5012 "ascend/compiler/ascParse.c"
    break;

  case 275: /* varlist: varlist ',' fname  */
#line 2879 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5021 "ascend/compiler/ascParse.c"
    break;

  case 276: /* varlist: varlist fname  */
#line 2884 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 5035 "ascend/compiler/ascParse.c"
    break;

  case 277: /* fname: name optional_notes  */
#line 2897 "ascend/compiler/ascParse.y"
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
	}
#line 5056 "ascend/compiler/ascParse.c"
    break;

  case 278: /* name: IDENTIFIER_TOK  */
#line 2917 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 5064 "ascend/compiler/ascParse.c"
    break;

  case 279: /* name: name '.' IDENTIFIER_TOK  */
#line 2921 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5073 "ascend/compiler/ascParse.c"
    break;

  case 280: /* name: name '[' set ']'  */
#line 2926 "ascend/compiler/ascParse.y"
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
	}
#line 5089 "ascend/compiler/ascParse.c"
    break;

  case 281: /* end: END_TOK CONDITIONAL_TOK  */
#line 2941 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 5098 "ascend/compiler/ascParse.c"
    break;

  case 282: /* end: END_TOK FOR_TOK  */
#line 2946 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 5107 "ascend/compiler/ascParse.c"
    break;

  case 283: /* end: END_TOK IF_TOK  */
#line 2951 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 5116 "ascend/compiler/ascParse.c"
    break;

  case 284: /* end: END_TOK INTERACTIVE_TOK  */
#line 2956 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 5125 "ascend/compiler/ascParse.c"
    break;

  case 285: /* end: END_TOK METHODS_TOK  */
#line 2961 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 5134 "ascend/compiler/ascParse.c"
    break;

  case 286: /* end: END_TOK NOTES_TOK  */
#line 2966 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 5143 "ascend/compiler/ascParse.c"
    break;

  case 287: /* end: END_TOK SELECT_TOK  */
#line 2971 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 5152 "ascend/compiler/ascParse.c"
    break;

  case 288: /* end: END_TOK SWITCH_TOK  */
#line 2976 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 5161 "ascend/compiler/ascParse.c"
    break;

  case 289: /* end: END_TOK UNITS_TOK  */
#line 2981 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 5170 "ascend/compiler/ascParse.c"
    break;

  case 290: /* end: END_TOK GLOBAL_TOK  */
#line 2986 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 5179 "ascend/compiler/ascParse.c"
    break;

  case 291: /* end: END_TOK WHEN_TOK  */
#line 2991 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 5188 "ascend/compiler/ascParse.c"
    break;

  case 292: /* end: END_TOK WHILE_TOK  */
#line 2996 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 5197 "ascend/compiler/ascParse.c"
    break;

  case 293: /* end: END_TOK IDENTIFIER_TOK  */
#line 3001 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 5206 "ascend/compiler/ascParse.c"
    break;

  case 294: /* end: END_TOK  */
#line 3006 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 5215 "ascend/compiler/ascParse.c"
    break;

  case 295: /* optional_bracedtext: %empty  */
#line 3014 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 5223 "ascend/compiler/ascParse.c"
    break;

  case 296: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 3018 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 5231 "ascend/compiler/ascParse.c"
    break;

  case 297: /* optional_notes: %empty  */
#line 3025 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 5239 "ascend/compiler/ascParse.c"
    break;

  case 298: /* optional_notes: DQUOTE_TOK  */
#line 3029 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 5247 "ascend/compiler/ascParse.c"
    break;

  case 299: /* set: setexprlist  */
#line 3036 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 5255 "ascend/compiler/ascParse.c"
    break;

  case 300: /* set: %empty  */
#line 3040 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 5263 "ascend/compiler/ascParse.c"
    break;

  case 301: /* setexprlist: expr  */
#line 3047 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 5271 "ascend/compiler/ascParse.c"
    break;

  case 302: /* setexprlist: expr DOTDOT_TOK expr  */
#line 3051 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5279 "ascend/compiler/ascParse.c"
    break;

  case 303: /* setexprlist: setexprlist ',' expr  */
#line 3055 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 5288 "ascend/compiler/ascParse.c"
    break;

  case 304: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 3060 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 5297 "ascend/compiler/ascParse.c"
    break;

  case 305: /* number: INTEGER_TOK  */
#line 3068 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
#line 5307 "ascend/compiler/ascParse.c"
    break;

  case 306: /* number: realnumber  */
#line 3074 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 5317 "ascend/compiler/ascParse.c"
    break;

  case 307: /* realnumber: REAL_TOK opunits  */
#line 3083 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	}
#line 5325 "ascend/compiler/ascParse.c"
    break;

  case 308: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 3087 "ascend/compiler/ascParse.y"
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
	}
#line 5347 "ascend/compiler/ascParse.c"
    break;

  case 309: /* opunits: %empty  */
#line 3108 "ascend/compiler/ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  (yyval.real_value) = 1.0;
	}
#line 5356 "ascend/compiler/ascParse.c"
    break;

  case 310: /* opunits: BRACEDTEXT_TOK  */
#line 3113 "ascend/compiler/ascParse.y"
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
	}
#line 5378 "ascend/compiler/ascParse.c"
    break;

  case 311: /* dims: DIMENSION_TOK dimensions  */
#line 3134 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 5386 "ascend/compiler/ascParse.c"
    break;

  case 312: /* dims: DIMENSIONLESS_TOK  */
#line 3138 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 5394 "ascend/compiler/ascParse.c"
    break;

  case 313: /* dims: %empty  */
#line 3142 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 5402 "ascend/compiler/ascParse.c"
    break;

  case 314: /* dimensions: '*'  */
#line 3149 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 5410 "ascend/compiler/ascParse.c"
    break;

  case 315: /* dimensions: dimexpr  */
#line 3153 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 5418 "ascend/compiler/ascParse.c"
    break;

  case 316: /* dimexpr: IDENTIFIER_TOK  */
#line 3160 "ascend/compiler/ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 5426 "ascend/compiler/ascParse.c"
    break;

  case 317: /* dimexpr: INTEGER_TOK  */
#line 3164 "ascend/compiler/ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 5434 "ascend/compiler/ascParse.c"
    break;

  case 318: /* dimexpr: dimexpr '/' dimexpr  */
#line 3168 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 5442 "ascend/compiler/ascParse.c"
    break;

  case 319: /* dimexpr: dimexpr '*' dimexpr  */
#line 3172 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 5450 "ascend/compiler/ascParse.c"
    break;

  case 320: /* dimexpr: dimexpr '^' fraction  */
#line 3176 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 5458 "ascend/compiler/ascParse.c"
    break;

  case 321: /* dimexpr: '(' dimexpr ')'  */
#line 3180 "ascend/compiler/ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 5466 "ascend/compiler/ascParse.c"
    break;

  case 322: /* fraction: optional_sign fractail  */
#line 3187 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 5474 "ascend/compiler/ascParse.c"
    break;

  case 323: /* fractail: INTEGER_TOK  */
#line 3194 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 5482 "ascend/compiler/ascParse.c"
    break;

  case 324: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 3198 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 5490 "ascend/compiler/ascParse.c"
    break;

  case 325: /* optional_sign: %empty  */
#line 3205 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 5498 "ascend/compiler/ascParse.c"
    break;

  case 326: /* optional_sign: '+'  */
#line 3209 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 5506 "ascend/compiler/ascParse.c"
    break;

  case 327: /* optional_sign: '-'  */
#line 3213 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 5514 "ascend/compiler/ascParse.c"
    break;

  case 328: /* expr: INTEGER_TOK  */
#line 3220 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 5522 "ascend/compiler/ascParse.c"
    break;

  case 329: /* expr: MAXINTEGER_TOK  */
#line 3224 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 5530 "ascend/compiler/ascParse.c"
    break;

  case 330: /* expr: realnumber  */
#line 3228 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 5538 "ascend/compiler/ascParse.c"
    break;

  case 331: /* expr: MAXREAL_TOK  */
#line 3232 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 5546 "ascend/compiler/ascParse.c"
    break;

  case 332: /* expr: TRUE_TOK  */
#line 3236 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 5554 "ascend/compiler/ascParse.c"
    break;

  case 333: /* expr: FALSE_TOK  */
#line 3240 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 5562 "ascend/compiler/ascParse.c"
    break;

  case 334: /* expr: ANY_TOK  */
#line 3244 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 5570 "ascend/compiler/ascParse.c"
    break;

  case 335: /* expr: SYMBOL_TOK  */
#line 3248 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 5578 "ascend/compiler/ascParse.c"
    break;

  case 336: /* expr: fname  */
#line 3252 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 5586 "ascend/compiler/ascParse.c"
    break;

  case 337: /* expr: '[' set ']'  */
#line 3256 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 5594 "ascend/compiler/ascParse.c"
    break;

  case 338: /* expr: expr '+' expr  */
#line 3260 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5603 "ascend/compiler/ascParse.c"
    break;

  case 339: /* expr: expr '-' expr  */
#line 3265 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5612 "ascend/compiler/ascParse.c"
    break;

  case 340: /* expr: expr '*' expr  */
#line 3270 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5621 "ascend/compiler/ascParse.c"
    break;

  case 341: /* expr: expr '/' expr  */
#line 3275 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5630 "ascend/compiler/ascParse.c"
    break;

  case 342: /* expr: expr '^' expr  */
#line 3280 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5639 "ascend/compiler/ascParse.c"
    break;

  case 343: /* expr: expr AND_TOK expr  */
#line 3285 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5648 "ascend/compiler/ascParse.c"
    break;

  case 344: /* expr: expr OR_TOK expr  */
#line 3290 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5657 "ascend/compiler/ascParse.c"
    break;

  case 345: /* expr: NOT_TOK expr  */
#line 3295 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 5665 "ascend/compiler/ascParse.c"
    break;

  case 346: /* expr: expr relop expr  */
#line 3299 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5674 "ascend/compiler/ascParse.c"
    break;

  case 347: /* expr: expr logrelop expr  */
#line 3304 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5683 "ascend/compiler/ascParse.c"
    break;

  case 348: /* expr: expr IN_TOK expr  */
#line 3309 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5692 "ascend/compiler/ascParse.c"
    break;

  case 349: /* expr: expr '|' expr  */
#line 3314 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5701 "ascend/compiler/ascParse.c"
    break;

  case 350: /* expr: expr SUCHTHAT_TOK expr  */
#line 3319 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5710 "ascend/compiler/ascParse.c"
    break;

  case 351: /* expr: '+' expr  */
#line 3324 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 5718 "ascend/compiler/ascParse.c"
    break;

  case 352: /* expr: '-' expr  */
#line 3328 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 5726 "ascend/compiler/ascParse.c"
    break;

  case 353: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 3332 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 5734 "ascend/compiler/ascParse.c"
    break;

  case 354: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 3336 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 5742 "ascend/compiler/ascParse.c"
    break;

  case 355: /* expr: SUM_TOK '(' set ')'  */
#line 3340 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 5753 "ascend/compiler/ascParse.c"
    break;

  case 356: /* expr: SUM_TOK '[' set ']'  */
#line 3347 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 5761 "ascend/compiler/ascParse.c"
    break;

  case 357: /* expr: PROD_TOK '(' set ')'  */
#line 3351 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 5772 "ascend/compiler/ascParse.c"
    break;

  case 358: /* expr: PROD_TOK '[' set ']'  */
#line 3358 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 5780 "ascend/compiler/ascParse.c"
    break;

  case 359: /* expr: UNION_TOK '(' set ')'  */
#line 3362 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 5791 "ascend/compiler/ascParse.c"
    break;

  case 360: /* expr: UNION_TOK '[' set ']'  */
#line 3369 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 5799 "ascend/compiler/ascParse.c"
    break;

  case 361: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 3373 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 5810 "ascend/compiler/ascParse.c"
    break;

  case 362: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 3380 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 5818 "ascend/compiler/ascParse.c"
    break;

  case 363: /* expr: CARD_TOK '(' set ')'  */
#line 3384 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 5829 "ascend/compiler/ascParse.c"
    break;

  case 364: /* expr: CARD_TOK '[' set ']'  */
#line 3391 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 5837 "ascend/compiler/ascParse.c"
    break;

  case 365: /* expr: CHOICE_TOK '(' set ')'  */
#line 3395 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 5848 "ascend/compiler/ascParse.c"
    break;

  case 366: /* expr: CHOICE_TOK '[' set ']'  */
#line 3402 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 5856 "ascend/compiler/ascParse.c"
    break;

  case 367: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 3406 "ascend/compiler/ascParse.y"
        {
	  CONST struct Func *fptr;
	  if ((fptr = LookupFunc(SCP((yyvsp[-3].id_ptr))))!=NULL) {
	    (yyval.eptr) = JoinExprLists((yyvsp[-1].eptr),CreateFuncExpr(fptr));
	  } else {
	    (yyval.eptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Function '%s' is not defined.",SCP((yyvsp[-3].id_ptr)));
	    g_untrapped_error++;
	  }
	}
#line 5871 "ascend/compiler/ascParse.c"
    break;

  case 368: /* expr: '(' expr ')'  */
#line 3417 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 5879 "ascend/compiler/ascParse.c"
    break;

  case 369: /* relop: '='  */
#line 3424 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 5887 "ascend/compiler/ascParse.c"
    break;

  case 370: /* relop: '<'  */
#line 3428 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 5895 "ascend/compiler/ascParse.c"
    break;

  case 371: /* relop: '>'  */
#line 3432 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 5903 "ascend/compiler/ascParse.c"
    break;

  case 372: /* relop: LEQ_TOK  */
#line 3436 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 5911 "ascend/compiler/ascParse.c"
    break;

  case 373: /* relop: GEQ_TOK  */
#line 3440 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 5919 "ascend/compiler/ascParse.c"
    break;

  case 374: /* relop: NEQ_TOK  */
#line 3444 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 5927 "ascend/compiler/ascParse.c"
    break;

  case 375: /* logrelop: BEQ_TOK  */
#line 3451 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 5935 "ascend/compiler/ascParse.c"
    break;

  case 376: /* logrelop: BNE_TOK  */
#line 3455 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 5943 "ascend/compiler/ascParse.c"
    break;


#line 5947 "ascend/compiler/ascParse.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 3459 "ascend/compiler/ascParse.y"

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
  const char *tok = Asc_ScannerTokenText();
  unsigned long col = Asc_ScannerTokenColumn();
  char tokbuf[64];
  size_t i;
  if (tok == NULL) {
    tok = "";
  }

  for (i = 0; i < sizeof(tokbuf) - 1 && tok[i] != '\0'; ++i) {
    char c = tok[i];
    tokbuf[i] = (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
  }
  tokbuf[i] = '\0';

  g_untrapped_error++;
  if (Asc_CurrentModule() != NULL) {
    MSG("message string '%s'",s);
    if (tokbuf[0] != '\0') {
      error_reporter_current_line(ASC_USER_ERROR,"%s near token '%s' at column %lu",s,tokbuf,col);
    } else {
      error_reporter_current_line(ASC_USER_ERROR,"%s at column %lu",s,col);
    }
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
		MSG("generic message, '%s'",string);
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
  //struct module_t *mod;

  /* the module may have be already closed */
  /* mod = */ Asc_CurrentModule();

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
ErrMsg_NullDefPointer(CONST char *object){
  MSG("Rejecting '%s'",object);
  MSG("About to reject '%s'",object);
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
#if 0
  case PATCH_TOK:
    return "PATCH";
#endif
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
	MSG("format = %s",fmt);
	va_list args, args2;
	va_start(args,fmt);
	va_copy(args2,args);
	va_error_reporter(sev,Asc_ModuleBestName(Asc_CurrentModule()),(int)LineNum(),NULL,fmt,args2);
	va_end(args);
}

/* vim: set ts=8: */
