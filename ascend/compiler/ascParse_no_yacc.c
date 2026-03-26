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


/* Substitute the variable and function names.  */
#define yyparse         zz_parse
#define yylex           zz_lex
#define yyerror         zz_error
#define yydebug         zz_debug
#define yynerrs         zz_nerrs
#define yylval          zz_lval
#define yychar          zz_char

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
static symchar *g_default_units;         /* units token of default value, if supplied */
static symchar *g_constant_units;        /* units token from CONSTANT ... UNITS ... */
static symchar *g_parsed_units;          /* units token from most recently parsed unit expression */
static symchar *g_number_units;          /* units token from most recently parsed number */

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
  char *units;
  struct Expr *default_expr;
  Asc_DString body;
  unsigned long rows;
  unsigned long scalars;
  unsigned long items;
};

static struct table_parse_state g_table_parse = {0,0,0,0,NULL,NULL,NULL,NULL,NULL,{0},0,0,0};

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
  if (g_table_parse.units != NULL) {
    ascfree(g_table_parse.units);
    g_table_parse.units = NULL;
  }
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
  if (g_table_parse.units != NULL) {
    ascfree(g_table_parse.units);
    g_table_parse.units = NULL;
  }
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

static void DatasetAppendImplicitSetDecls(struct gl_list_t *list, struct Statement *dataset_stat)
{
  struct DatasetMapItem *map;

  if (list == NULL || dataset_stat == NULL || StatementType(dataset_stat) != DATASETSTAT) {
    return;
  }
  if (dataset_stat->v.dataset.indices != NULL) {
    return;
  }

  for (map = dataset_stat->v.dataset.maps; map != NULL; map = map->next) {
    CONST struct Name *node;
    for (node = map->target; node != NULL; node = NextName(node)) {
      CONST struct Set *setnode;
      if (NameId(node)) {
        continue;
      }
      for (setnode = NameSetPtr(node); setnode != NULL; setnode = NextSet(setnode)) {
        CONST struct Expr *expr = GetSingleExpr(setnode);
        CONST struct Name *setexpr;
        symchar *setid;
        struct Name *setname;
        struct Statement *decl;
        struct VariableList *vl;

        if (expr == NULL || ExprType(expr) != e_var || ExprListLength(expr) != 1) {
          continue;
        }
        setexpr = ExprName(expr);
        setid = (setexpr != NULL) ? SimpleNameIdPtr(setexpr) : NULL;
        if (setid == NULL) {
          continue;
        }
        setname = CreateIdName(setid);
        if (StatementListHasTypeDeclForName(list,setname)) {
          DestroyName(setname);
          continue;
        }
        vl = CreateVariableNode(setname);
        decl = CreateISA(vl
          ,GetBaseTypeName(set_type)
          ,NULL
          ,GetBaseTypeName(integer_constant_type)
          ,NULL
          ,ISCV_NONE
        );
        decl->mod = dataset_stat->mod;
        decl->linenum = dataset_stat->linenum;
        decl->context = dataset_stat->context;
        gl_append_ptr(list,(char *)decl);
      }
    }
  }
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

struct StudyParse {
  struct Name *vary;
  struct Expr *lower;
  struct Expr *upper;
  struct Expr *value;
  long steps;
  enum StudyMode mode;
  enum StudyDistribution dist;
  symchar *run_method;
  unsigned int now;
  CONST char *filename;
};

static struct StudyParse StudyParseEmpty(void){
  struct StudyParse spec;
  spec.vary = NULL;
  spec.lower = NULL;
  spec.upper = NULL;
  spec.value = NULL;
  spec.steps = 0;
  spec.mode = study_none;
  spec.dist = study_dist_default;
  spec.run_method = NULL;
  spec.now = 0;
  spec.filename = NULL;
  return spec;
}

static struct StudyParse g_study_parse;
static symchar *g_study_run_method = NULL;
static unsigned int g_study_now = 0;
static CONST char *g_study_filename = NULL;
static unsigned char g_decl_checkkind = ISCV_NONE;

/* For 'inline' notes, note on DQUOTE_TOK from scanner.l:
 * Remember that DQUOTE_TOK is a string value which is local to the
 * production that finds it. It must be copied if you want to
 * keep it.
 */

/* MS VC++ won't compiler Bison output unless we switch this */
#ifdef _MSC_VER
# define __STDC__
#endif


#line 789 "ascend/compiler/ascParse.c"

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
  YYSYMBOL_DELETE_TOK = 28,                /* DELETE_TOK  */
  YYSYMBOL_DER_TOK = 29,                   /* DER_TOK  */
  YYSYMBOL_DIMENSION_TOK = 30,             /* DIMENSION_TOK  */
  YYSYMBOL_DIMENSIONLESS_TOK = 31,         /* DIMENSIONLESS_TOK  */
  YYSYMBOL_DO_TOK = 32,                    /* DO_TOK  */
  YYSYMBOL_ELSE_TOK = 33,                  /* ELSE_TOK  */
  YYSYMBOL_END_TOK = 34,                   /* END_TOK  */
  YYSYMBOL_EXPECT_TOK = 35,                /* EXPECT_TOK  */
  YYSYMBOL_EXTERNAL_TOK = 36,              /* EXTERNAL_TOK  */
  YYSYMBOL_FALSE_TOK = 37,                 /* FALSE_TOK  */
  YYSYMBOL_FALLTHRU_TOK = 38,              /* FALLTHRU_TOK  */
  YYSYMBOL_FIX_TOK = 39,                   /* FIX_TOK  */
  YYSYMBOL_FOR_TOK = 40,                   /* FOR_TOK  */
  YYSYMBOL_FREE_TOK = 41,                  /* FREE_TOK  */
  YYSYMBOL_FROM_TOK = 42,                  /* FROM_TOK  */
  YYSYMBOL_FILE_TOK = 43,                  /* FILE_TOK  */
  YYSYMBOL_GLOBAL_TOK = 44,                /* GLOBAL_TOK  */
  YYSYMBOL_IF_TOK = 45,                    /* IF_TOK  */
  YYSYMBOL_IGNORE_TOK = 46,                /* IGNORE_TOK  */
  YYSYMBOL_IMPORT_TOK = 47,                /* IMPORT_TOK  */
  YYSYMBOL_IN_TOK = 48,                    /* IN_TOK  */
  YYSYMBOL_INITIAL_TOK = 49,               /* INITIAL_TOK  */
  YYSYMBOL_INPUT_TOK = 50,                 /* INPUT_TOK  */
  YYSYMBOL_INCREASING_TOK = 51,            /* INCREASING_TOK  */
  YYSYMBOL_INTERACTIVE_TOK = 52,           /* INTERACTIVE_TOK  */
  YYSYMBOL_INDEPENDENT_TOK = 53,           /* INDEPENDENT_TOK  */
  YYSYMBOL_INTERSECTION_TOK = 54,          /* INTERSECTION_TOK  */
  YYSYMBOL_ISA_TOK = 55,                   /* ISA_TOK  */
  YYSYMBOL__IS_T = 56,                     /* _IS_T  */
  YYSYMBOL_ISREFINEDTO_TOK = 57,           /* ISREFINEDTO_TOK  */
  YYSYMBOL_LINEAR_TOK = 58,                /* LINEAR_TOK  */
  YYSYMBOL_LOG_TOK = 59,                   /* LOG_TOK  */
  YYSYMBOL_NOW_TOK = 60,                   /* NOW_TOK  */
  YYSYMBOL_LINK_TOK = 61,                  /* LINK_TOK  */
  YYSYMBOL_MAXIMIZE_TOK = 62,              /* MAXIMIZE_TOK  */
  YYSYMBOL_MAXINTEGER_TOK = 63,            /* MAXINTEGER_TOK  */
  YYSYMBOL_MAXREAL_TOK = 64,               /* MAXREAL_TOK  */
  YYSYMBOL_METHODS_TOK = 65,               /* METHODS_TOK  */
  YYSYMBOL_METHOD_TOK = 66,                /* METHOD_TOK  */
  YYSYMBOL_MINIMIZE_TOK = 67,              /* MINIMIZE_TOK  */
  YYSYMBOL_MODEL_TOK = 68,                 /* MODEL_TOK  */
  YYSYMBOL_NOT_TOK = 69,                   /* NOT_TOK  */
  YYSYMBOL_NOTES_TOK = 70,                 /* NOTES_TOK  */
  YYSYMBOL_OF_TOK = 71,                    /* OF_TOK  */
  YYSYMBOL_OPTION_TOK = 72,                /* OPTION_TOK  */
  YYSYMBOL_OR_TOK = 73,                    /* OR_TOK  */
  YYSYMBOL_OTHERWISE_TOK = 74,             /* OTHERWISE_TOK  */
  YYSYMBOL_OUTPUT_TOK = 75,                /* OUTPUT_TOK  */
  YYSYMBOL_PROD_TOK = 76,                  /* PROD_TOK  */
  YYSYMBOL_PROVIDE_TOK = 77,               /* PROVIDE_TOK  */
  YYSYMBOL_RATIO_TOK = 78,                 /* RATIO_TOK  */
  YYSYMBOL_REFINES_TOK = 79,               /* REFINES_TOK  */
  YYSYMBOL_REPLACE_TOK = 80,               /* REPLACE_TOK  */
  YYSYMBOL_REQUIRE_TOK = 81,               /* REQUIRE_TOK  */
  YYSYMBOL_RETURN_TOK = 82,                /* RETURN_TOK  */
  YYSYMBOL_RUN_TOK = 83,                   /* RUN_TOK  */
  YYSYMBOL_REINIT_TOK = 84,                /* REINIT_TOK  */
  YYSYMBOL_SATISFIED_TOK = 85,             /* SATISFIED_TOK  */
  YYSYMBOL_SELECT_TOK = 86,                /* SELECT_TOK  */
  YYSYMBOL_SIZE_TOK = 87,                  /* SIZE_TOK  */
  YYSYMBOL_SOLVE_TOK = 88,                 /* SOLVE_TOK  */
  YYSYMBOL_SOLVER_TOK = 89,                /* SOLVER_TOK  */
  YYSYMBOL_STOP_TOK = 90,                  /* STOP_TOK  */
  YYSYMBOL_SUCHTHAT_TOK = 91,              /* SUCHTHAT_TOK  */
  YYSYMBOL_SUM_TOK = 92,                   /* SUM_TOK  */
  YYSYMBOL_SWITCH_TOK = 93,                /* SWITCH_TOK  */
  YYSYMBOL_SYSTEM_TOK = 94,                /* SYSTEM_TOK  */
  YYSYMBOL_STEP_TOK = 95,                  /* STEP_TOK  */
  YYSYMBOL_STEPS_TOK = 96,                 /* STEPS_TOK  */
  YYSYMBOL_STUDY_TOK = 97,                 /* STUDY_TOK  */
  YYSYMBOL_TABLE_TOK = 98,                 /* TABLE_TOK  */
  YYSYMBOL_VALUES_TOK = 99,                /* VALUES_TOK  */
  YYSYMBOL_DATASET_TOK = 100,              /* DATASET_TOK  */
  YYSYMBOL_POSITIONAL_TOK = 101,           /* POSITIONAL_TOK  */
  YYSYMBOL_INDEX_TOK = 102,                /* INDEX_TOK  */
  YYSYMBOL_COLUMN_TOK = 103,               /* COLUMN_TOK  */
  YYSYMBOL_EOL_TOK = 104,                  /* EOL_TOK  */
  YYSYMBOL_THEN_TOK = 105,                 /* THEN_TOK  */
  YYSYMBOL_TO_TOK = 106,                   /* TO_TOK  */
  YYSYMBOL_TRUE_TOK = 107,                 /* TRUE_TOK  */
  YYSYMBOL_UNION_TOK = 108,                /* UNION_TOK  */
  YYSYMBOL_UNITS_TOK = 109,                /* UNITS_TOK  */
  YYSYMBOL_LADDER_TOK = 110,               /* LADDER_TOK  */
  YYSYMBOL_UNIVERSAL_TOK = 111,            /* UNIVERSAL_TOK  */
  YYSYMBOL_UNLINK_TOK = 112,               /* UNLINK_TOK  */
  YYSYMBOL_VARY_TOK = 113,                 /* VARY_TOK  */
  YYSYMBOL_WHEN_TOK = 114,                 /* WHEN_TOK  */
  YYSYMBOL_WHERE_TOK = 115,                /* WHERE_TOK  */
  YYSYMBOL_WHILE_TOK = 116,                /* WHILE_TOK  */
  YYSYMBOL_WILLBE_TOK = 117,               /* WILLBE_TOK  */
  YYSYMBOL_WILLBETHESAME_TOK = 118,        /* WILLBETHESAME_TOK  */
  YYSYMBOL_WILLNOTBETHESAME_TOK = 119,     /* WILLNOTBETHESAME_TOK  */
  YYSYMBOL_ASSIGN_TOK = 120,               /* ASSIGN_TOK  */
  YYSYMBOL_CASSIGN_TOK = 121,              /* CASSIGN_TOK  */
  YYSYMBOL_DBLCOLON_TOK = 122,             /* DBLCOLON_TOK  */
  YYSYMBOL_USE_TOK = 123,                  /* USE_TOK  */
  YYSYMBOL_LEQ_TOK = 124,                  /* LEQ_TOK  */
  YYSYMBOL_GEQ_TOK = 125,                  /* GEQ_TOK  */
  YYSYMBOL_NEQ_TOK = 126,                  /* NEQ_TOK  */
  YYSYMBOL_DOTDOT_TOK = 127,               /* DOTDOT_TOK  */
  YYSYMBOL_WITH_TOK = 128,                 /* WITH_TOK  */
  YYSYMBOL_VALUE_TOK = 129,                /* VALUE_TOK  */
  YYSYMBOL_WITH_VALUE_T = 130,             /* WITH_VALUE_T  */
  YYSYMBOL_REAL_TOK = 131,                 /* REAL_TOK  */
  YYSYMBOL_INTEGER_TOK = 132,              /* INTEGER_TOK  */
  YYSYMBOL_IDENTIFIER_TOK = 133,           /* IDENTIFIER_TOK  */
  YYSYMBOL_BRACEDTEXT_TOK = 134,           /* BRACEDTEXT_TOK  */
  YYSYMBOL_SYMBOL_TOK = 135,               /* SYMBOL_TOK  */
  YYSYMBOL_DQUOTE_TOK = 136,               /* DQUOTE_TOK  */
  YYSYMBOL_137_ = 137,                     /* ','  */
  YYSYMBOL_138_ = 138,                     /* '|'  */
  YYSYMBOL_139_ = 139,                     /* '<'  */
  YYSYMBOL_140_ = 140,                     /* '='  */
  YYSYMBOL_141_ = 141,                     /* '>'  */
  YYSYMBOL_142_ = 142,                     /* '+'  */
  YYSYMBOL_143_ = 143,                     /* '-'  */
  YYSYMBOL_144_ = 144,                     /* '/'  */
  YYSYMBOL_145_ = 145,                     /* '*'  */
  YYSYMBOL_UMINUS_TOK = 146,               /* UMINUS_TOK  */
  YYSYMBOL_UPLUS_TOK = 147,                /* UPLUS_TOK  */
  YYSYMBOL_148_ = 148,                     /* '^'  */
  YYSYMBOL_149_ = 149,                     /* ';'  */
  YYSYMBOL_150_ = 150,                     /* '('  */
  YYSYMBOL_151_ = 151,                     /* ')'  */
  YYSYMBOL_152_ = 152,                     /* ':'  */
  YYSYMBOL_153_ = 153,                     /* '['  */
  YYSYMBOL_154_ = 154,                     /* ']'  */
  YYSYMBOL_155_ = 155,                     /* '.'  */
  YYSYMBOL_YYACCEPT = 156,                 /* $accept  */
  YYSYMBOL_definitions = 157,              /* definitions  */
  YYSYMBOL_definition = 158,               /* definition  */
  YYSYMBOL_global_def = 159,               /* global_def  */
  YYSYMBOL_require_file = 160,             /* require_file  */
  YYSYMBOL_provide_module = 161,           /* provide_module  */
  YYSYMBOL_import = 162,                   /* import  */
  YYSYMBOL_add_notes_def = 163,            /* add_notes_def  */
  YYSYMBOL_add_notes_head = 164,           /* add_notes_head  */
  YYSYMBOL_add_method_def = 165,           /* add_method_def  */
  YYSYMBOL_add_method_head = 166,          /* add_method_head  */
  YYSYMBOL_replace_method_def = 167,       /* replace_method_def  */
  YYSYMBOL_replace_method_head = 168,      /* replace_method_head  */
  YYSYMBOL_atom_def = 169,                 /* atom_def  */
  YYSYMBOL_atom_head = 170,                /* atom_head  */
  YYSYMBOL_atom_id = 171,                  /* atom_id  */
  YYSYMBOL_default_val = 172,              /* default_val  */
  YYSYMBOL_constant_def = 173,             /* constant_def  */
  YYSYMBOL_constant_head = 174,            /* constant_head  */
  YYSYMBOL_constant_dims = 175,            /* constant_dims  */
  YYSYMBOL_constant_val = 176,             /* constant_val  */
  YYSYMBOL_model_def = 177,                /* model_def  */
  YYSYMBOL_model_head = 178,               /* model_head  */
  YYSYMBOL_model_id = 179,                 /* model_id  */
  YYSYMBOL_optional_model_parameters = 180, /* optional_model_parameters  */
  YYSYMBOL_optional_parameter_wheres = 181, /* optional_parameter_wheres  */
  YYSYMBOL_optional_parameter_reduction = 182, /* optional_parameter_reduction  */
  YYSYMBOL_universal = 183,                /* universal  */
  YYSYMBOL_definition_def = 184,           /* definition_def  */
  YYSYMBOL_definition_id = 185,            /* definition_id  */
  YYSYMBOL_units_def = 186,                /* units_def  */
  YYSYMBOL_units_statement = 187,          /* units_statement  */
  YYSYMBOL_optional_ladder_end = 188,      /* optional_ladder_end  */
  YYSYMBOL_table_statement = 189,          /* table_statement  */
  YYSYMBOL_table_begin = 190,              /* table_begin  */
  YYSYMBOL_table_mode_on = 191,            /* table_mode_on  */
  YYSYMBOL_table_mode_off = 192,           /* table_mode_off  */
  YYSYMBOL_table_options = 193,            /* table_options  */
  YYSYMBOL_table_decl_opt = 194,           /* table_decl_opt  */
  YYSYMBOL_table_option = 195,             /* table_option  */
  YYSYMBOL_table_body = 196,               /* table_body  */
  YYSYMBOL_table_body_item = 197,          /* table_body_item  */
  YYSYMBOL_table_scalar = 198,             /* table_scalar  */
  YYSYMBOL_values_statement = 199,         /* values_statement  */
  YYSYMBOL_values_default_opt = 200,       /* values_default_opt  */
  YYSYMBOL_values_entries = 201,           /* values_entries  */
  YYSYMBOL_values_entry = 202,             /* values_entry  */
  YYSYMBOL_values_key_list = 203,          /* values_key_list  */
  YYSYMBOL_values_key = 204,               /* values_key  */
  YYSYMBOL_dataset_statement = 205,        /* dataset_statement  */
  YYSYMBOL_206_1 = 206,                    /* $@1  */
  YYSYMBOL_dataset_items = 207,            /* dataset_items  */
  YYSYMBOL_dataset_item = 208,             /* dataset_item  */
  YYSYMBOL_dataset_index_item = 209,       /* dataset_index_item  */
  YYSYMBOL_dataset_map_item = 210,         /* dataset_map_item  */
  YYSYMBOL_dataset_column_selector = 211,  /* dataset_column_selector  */
  YYSYMBOL_dataset_column_ref = 212,       /* dataset_column_ref  */
  YYSYMBOL_dataset_target = 213,           /* dataset_target  */
  YYSYMBOL_dataset_units_opt = 214,        /* dataset_units_opt  */
  YYSYMBOL_dataset_type_opt = 215,         /* dataset_type_opt  */
  YYSYMBOL_dataset_type_req = 216,         /* dataset_type_req  */
  YYSYMBOL_unitdeflist = 217,              /* unitdeflist  */
  YYSYMBOL_unitdef = 218,                  /* unitdef  */
  YYSYMBOL_unitladderitemlist = 219,       /* unitladderitemlist  */
  YYSYMBOL_unitladderitem = 220,           /* unitladderitem  */
  YYSYMBOL_methods = 221,                  /* methods  */
  YYSYMBOL_222_2 = 222,                    /* $@2  */
  YYSYMBOL_initial = 223,                  /* initial  */
  YYSYMBOL_proclist = 224,                 /* proclist  */
  YYSYMBOL_proclistf = 225,                /* proclistf  */
  YYSYMBOL_procedure = 226,                /* procedure  */
  YYSYMBOL_procedure_id = 227,             /* procedure_id  */
  YYSYMBOL_fstatements = 228,              /* fstatements  */
  YYSYMBOL_statements = 229,               /* statements  */
  YYSYMBOL_statement = 230,                /* statement  */
  YYSYMBOL_complex_statement = 231,        /* complex_statement  */
  YYSYMBOL_isa_statement = 232,            /* isa_statement  */
  YYSYMBOL_willbe_statement = 233,         /* willbe_statement  */
  YYSYMBOL_aliases_statement = 234,        /* aliases_statement  */
  YYSYMBOL_optional_set_values = 235,      /* optional_set_values  */
  YYSYMBOL_is_statement = 236,             /* is_statement  */
  YYSYMBOL_isrefinedto_statement = 237,    /* isrefinedto_statement  */
  YYSYMBOL_call_identifier = 238,          /* call_identifier  */
  YYSYMBOL_type_identifier = 239,          /* type_identifier  */
  YYSYMBOL_optional_method = 240,          /* optional_method  */
  YYSYMBOL_optional_of = 241,              /* optional_of  */
  YYSYMBOL_optional_with_value = 242,      /* optional_with_value  */
  YYSYMBOL_arealike_statement = 243,       /* arealike_statement  */
  YYSYMBOL_link_statement = 244,           /* link_statement  */
  YYSYMBOL_unlink_statement = 245,         /* unlink_statement  */
  YYSYMBOL_der_statement = 246,            /* der_statement  */
  YYSYMBOL_independent_statement = 247,    /* independent_statement  */
  YYSYMBOL_arethesame_statement = 248,     /* arethesame_statement  */
  YYSYMBOL_willbethesame_statement = 249,  /* willbethesame_statement  */
  YYSYMBOL_willnotbethesame_statement = 250, /* willnotbethesame_statement  */
  YYSYMBOL_assignment_statement = 251,     /* assignment_statement  */
  YYSYMBOL_reinit_statement = 252,         /* reinit_statement  */
  YYSYMBOL_switchto_statement = 253,       /* switchto_statement  */
  YYSYMBOL_relation_statement = 254,       /* relation_statement  */
  YYSYMBOL_relation = 255,                 /* relation  */
  YYSYMBOL_blackbox_statement = 256,       /* blackbox_statement  */
  YYSYMBOL_input_args = 257,               /* input_args  */
  YYSYMBOL_output_args = 258,              /* output_args  */
  YYSYMBOL_data_args = 259,                /* data_args  */
  YYSYMBOL_for_statement = 260,            /* for_statement  */
  YYSYMBOL_optional_direction = 261,       /* optional_direction  */
  YYSYMBOL_forexprend = 262,               /* forexprend  */
  YYSYMBOL_run_statement = 263,            /* run_statement  */
  YYSYMBOL_fix_statement = 264,            /* fix_statement  */
  YYSYMBOL_fix_and_assign_statement = 265, /* fix_and_assign_statement  */
  YYSYMBOL_free_statement = 266,           /* free_statement  */
  YYSYMBOL_solver_statement = 267,         /* solver_statement  */
  YYSYMBOL_option_statement = 268,         /* option_statement  */
  YYSYMBOL_solve_statement = 269,          /* solve_statement  */
  YYSYMBOL_study_statement = 270,          /* study_statement  */
  YYSYMBOL_271_3 = 271,                    /* $@3  */
  YYSYMBOL_study_vary_opt = 272,           /* study_vary_opt  */
  YYSYMBOL_study_distribution_opt = 273,   /* study_distribution_opt  */
  YYSYMBOL_study_run_opt = 274,            /* study_run_opt  */
  YYSYMBOL_study_file_opt = 275,           /* study_file_opt  */
  YYSYMBOL_study_now_opt = 276,            /* study_now_opt  */
  YYSYMBOL_delete_statement = 277,         /* delete_statement  */
  YYSYMBOL_external_statement = 278,       /* external_statement  */
  YYSYMBOL_call_statement = 279,           /* call_statement  */
  YYSYMBOL_assert_statement = 280,         /* assert_statement  */
  YYSYMBOL_if_statement = 281,             /* if_statement  */
  YYSYMBOL_while_statement = 282,          /* while_statement  */
  YYSYMBOL_optional_else = 283,            /* optional_else  */
  YYSYMBOL_when_statement = 284,           /* when_statement  */
  YYSYMBOL_whenlist = 285,                 /* whenlist  */
  YYSYMBOL_whenlistf = 286,                /* whenlistf  */
  YYSYMBOL_flow_statement = 287,           /* flow_statement  */
  YYSYMBOL_use_statement = 288,            /* use_statement  */
  YYSYMBOL_select_statement = 289,         /* select_statement  */
  YYSYMBOL_selectlist = 290,               /* selectlist  */
  YYSYMBOL_selectlistf = 291,              /* selectlistf  */
  YYSYMBOL_switch_statement = 292,         /* switch_statement  */
  YYSYMBOL_switchlist = 293,               /* switchlist  */
  YYSYMBOL_switchlistf = 294,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 295,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 296,          /* notes_statement  */
  YYSYMBOL_notes_body = 297,               /* notes_body  */
  YYSYMBOL_noteslist = 298,                /* noteslist  */
  YYSYMBOL_fvarlist = 299,                 /* fvarlist  */
  YYSYMBOL_varlist = 300,                  /* varlist  */
  YYSYMBOL_method_fvarlist = 301,          /* method_fvarlist  */
  YYSYMBOL_method_varlist = 302,           /* method_varlist  */
  YYSYMBOL_fvarref = 303,                  /* fvarref  */
  YYSYMBOL_fname = 304,                    /* fname  */
  YYSYMBOL_name = 305,                     /* name  */
  YYSYMBOL_end = 306,                      /* end  */
  YYSYMBOL_optional_bracedtext = 307,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 308,           /* optional_notes  */
  YYSYMBOL_set = 309,                      /* set  */
  YYSYMBOL_setexprlist = 310,              /* setexprlist  */
  YYSYMBOL_number = 311,                   /* number  */
  YYSYMBOL_realnumber = 312,               /* realnumber  */
  YYSYMBOL_opunits = 313,                  /* opunits  */
  YYSYMBOL_dims = 314,                     /* dims  */
  YYSYMBOL_dimensions = 315,               /* dimensions  */
  YYSYMBOL_dimexpr = 316,                  /* dimexpr  */
  YYSYMBOL_fraction = 317,                 /* fraction  */
  YYSYMBOL_fractail = 318,                 /* fractail  */
  YYSYMBOL_optional_sign = 319,            /* optional_sign  */
  YYSYMBOL_expr = 320,                     /* expr  */
  YYSYMBOL_relop = 321,                    /* relop  */
  YYSYMBOL_logrelop = 322                  /* logrelop  */
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
#define YYLAST   1390

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  156
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  167
/* YYNRULES -- Number of rules.  */
#define YYNRULES  428
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  842

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   393


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
     150,   151,   145,   142,   137,   143,   155,   144,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   152,   149,
     139,   140,   141,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   153,     2,   154,   148,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   138,     2,     2,     2,     2,     2,
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
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   146,   147
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   879,   879,   881,   885,   886,   887,   888,   889,   890,
     891,   892,   893,   894,   896,   897,   898,   909,   954,   958,
     963,   971,   975,   980,   988,   997,  1009,  1040,  1048,  1071,
    1084,  1092,  1113,  1125,  1133,  1186,  1197,  1206,  1212,  1218,
    1226,  1234,  1245,  1285,  1319,  1324,  1330,  1334,  1356,  1362,
    1368,  1376,  1384,  1395,  1437,  1446,  1459,  1468,  1471,  1479,
    1482,  1490,  1493,  1544,  1547,  1554,  1584,  1593,  1598,  1615,
    1641,  1644,  1651,  1672,  1685,  1692,  1699,  1704,  1706,  1711,
    1723,  1736,  1740,  1747,  1756,  1758,  1762,  1763,  1767,  1771,
    1775,  1779,  1783,  1787,  1794,  1798,  1802,  1806,  1810,  1817,
    1824,  1826,  1833,  1834,  1838,  1845,  1846,  1850,  1851,  1852,
    1853,  1858,  1857,  1867,  1869,  1870,  1880,  1881,  1885,  1892,
    1896,  1903,  1907,  1914,  1918,  1925,  1930,  1939,  1942,  1950,
    1953,  1960,  1967,  1970,  1978,  1986,  1989,  1997,  2002,  2012,
    2016,  2015,  2030,  2033,  2041,  2049,  2052,  2077,  2091,  2100,
    2108,  2111,  2183,  2191,  2199,  2200,  2201,  2202,  2203,  2204,
    2205,  2206,  2207,  2208,  2209,  2210,  2211,  2212,  2213,  2214,
    2215,  2217,  2218,  2219,  2220,  2221,  2222,  2223,  2224,  2225,
    2226,  2227,  2228,  2229,  2230,  2231,  2232,  2233,  2234,  2235,
    2236,  2237,  2238,  2239,  2240,  2241,  2242,  2246,  2251,  2284,
    2314,  2318,  2369,  2372,  2379,  2391,  2418,  2423,  2431,  2436,
    2445,  2448,  2456,  2459,  2467,  2470,  2475,  2483,  2490,  2494,
    2498,  2505,  2509,  2516,  2525,  2534,  2541,  2548,  2555,  2559,
    2566,  2573,  2580,  2593,  2610,  2620,  2628,  2639,  2660,  2667,
    2675,  2678,  2715,  2737,  2740,  2744,  2751,  2755,  2759,  2763,
    2770,  2774,  2781,  2789,  2803,  2810,  2818,  2826,  2831,  2839,
    2838,  2855,  2857,  2865,  2874,  2887,  2889,  2893,  2901,  2903,
    2911,  2913,  2921,  2923,  2930,  2937,  2948,  2959,  2965,  2975,
    2985,  2988,  2995,  3006,  3018,  3025,  3035,  3042,  3046,  3050,
    3054,  3061,  3065,  3069,  3073,  3077,  3084,  3091,  3102,  3112,
    3119,  3123,  3127,  3131,  3138,  3149,  3159,  3166,  3170,  3174,
    3178,  3185,  3195,  3232,  3241,  3256,  3261,  3270,  3281,  3285,
    3290,  3303,  3310,  3314,  3319,  3329,  3333,  3340,  3360,  3364,
    3369,  3374,  3389,  3394,  3399,  3404,  3409,  3414,  3419,  3424,
    3429,  3434,  3439,  3444,  3449,  3454,  3463,  3466,  3474,  3477,
    3484,  3489,  3495,  3499,  3503,  3508,  3516,  3523,  3532,  3537,
    3562,  3567,  3590,  3594,  3599,  3605,  3609,  3616,  3620,  3624,
    3628,  3632,  3636,  3643,  3650,  3654,  3662,  3665,  3669,  3676,
    3680,  3684,  3688,  3692,  3696,  3700,  3704,  3708,  3712,  3716,
    3720,  3725,  3730,  3735,  3740,  3745,  3750,  3755,  3759,  3764,
    3769,  3774,  3779,  3784,  3788,  3792,  3796,  3800,  3807,  3811,
    3818,  3822,  3829,  3833,  3840,  3844,  3851,  3855,  3862,  3866,
    3888,  3895,  3899,  3903,  3907,  3911,  3915,  3922,  3926
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
  "DATA_TOK", "DECREASING_TOK", "DEFAULT_TOK", "DEFINITION_TOK",
  "DELETE_TOK", "DER_TOK", "DIMENSION_TOK", "DIMENSIONLESS_TOK", "DO_TOK",
  "ELSE_TOK", "END_TOK", "EXPECT_TOK", "EXTERNAL_TOK", "FALSE_TOK",
  "FALLTHRU_TOK", "FIX_TOK", "FOR_TOK", "FREE_TOK", "FROM_TOK", "FILE_TOK",
  "GLOBAL_TOK", "IF_TOK", "IGNORE_TOK", "IMPORT_TOK", "IN_TOK",
  "INITIAL_TOK", "INPUT_TOK", "INCREASING_TOK", "INTERACTIVE_TOK",
  "INDEPENDENT_TOK", "INTERSECTION_TOK", "ISA_TOK", "_IS_T",
  "ISREFINEDTO_TOK", "LINEAR_TOK", "LOG_TOK", "NOW_TOK", "LINK_TOK",
  "MAXIMIZE_TOK", "MAXINTEGER_TOK", "MAXREAL_TOK", "METHODS_TOK",
  "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK", "NOT_TOK", "NOTES_TOK",
  "OF_TOK", "OPTION_TOK", "OR_TOK", "OTHERWISE_TOK", "OUTPUT_TOK",
  "PROD_TOK", "PROVIDE_TOK", "RATIO_TOK", "REFINES_TOK", "REPLACE_TOK",
  "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK", "REINIT_TOK", "SATISFIED_TOK",
  "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK", "SOLVER_TOK", "STOP_TOK",
  "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK", "SYSTEM_TOK", "STEP_TOK",
  "STEPS_TOK", "STUDY_TOK", "TABLE_TOK", "VALUES_TOK", "DATASET_TOK",
  "POSITIONAL_TOK", "INDEX_TOK", "COLUMN_TOK", "EOL_TOK", "THEN_TOK",
  "TO_TOK", "TRUE_TOK", "UNION_TOK", "UNITS_TOK", "LADDER_TOK",
  "UNIVERSAL_TOK", "UNLINK_TOK", "VARY_TOK", "WHEN_TOK", "WHERE_TOK",
  "WHILE_TOK", "WILLBE_TOK", "WILLBETHESAME_TOK", "WILLNOTBETHESAME_TOK",
  "ASSIGN_TOK", "CASSIGN_TOK", "DBLCOLON_TOK", "USE_TOK", "LEQ_TOK",
  "GEQ_TOK", "NEQ_TOK", "DOTDOT_TOK", "WITH_TOK", "VALUE_TOK",
  "WITH_VALUE_T", "REAL_TOK", "INTEGER_TOK", "IDENTIFIER_TOK",
  "BRACEDTEXT_TOK", "SYMBOL_TOK", "DQUOTE_TOK", "','", "'|'", "'<'", "'='",
  "'>'", "'+'", "'-'", "'/'", "'*'", "UMINUS_TOK", "UPLUS_TOK", "'^'",
  "';'", "'('", "')'", "':'", "'['", "']'", "'.'", "$accept",
  "definitions", "definition", "global_def", "require_file",
  "provide_module", "import", "add_notes_def", "add_notes_head",
  "add_method_def", "add_method_head", "replace_method_def",
  "replace_method_head", "atom_def", "atom_head", "atom_id", "default_val",
  "constant_def", "constant_head", "constant_dims", "constant_val",
  "model_def", "model_head", "model_id", "optional_model_parameters",
  "optional_parameter_wheres", "optional_parameter_reduction", "universal",
  "definition_def", "definition_id", "units_def", "units_statement",
  "optional_ladder_end", "table_statement", "table_begin", "table_mode_on",
  "table_mode_off", "table_options", "table_decl_opt", "table_option",
  "table_body", "table_body_item", "table_scalar", "values_statement",
  "values_default_opt", "values_entries", "values_entry",
  "values_key_list", "values_key", "dataset_statement", "$@1",
  "dataset_items", "dataset_item", "dataset_index_item",
  "dataset_map_item", "dataset_column_selector", "dataset_column_ref",
  "dataset_target", "dataset_units_opt", "dataset_type_opt",
  "dataset_type_req", "unitdeflist", "unitdef", "unitladderitemlist",
  "unitladderitem", "methods", "$@2", "initial", "proclist", "proclistf",
  "procedure", "procedure_id", "fstatements", "statements", "statement",
  "complex_statement", "isa_statement", "willbe_statement",
  "aliases_statement", "optional_set_values", "is_statement",
  "isrefinedto_statement", "call_identifier", "type_identifier",
  "optional_method", "optional_of", "optional_with_value",
  "arealike_statement", "link_statement", "unlink_statement",
  "der_statement", "independent_statement", "arethesame_statement",
  "willbethesame_statement", "willnotbethesame_statement",
  "assignment_statement", "reinit_statement", "switchto_statement",
  "relation_statement", "relation", "blackbox_statement", "input_args",
  "output_args", "data_args", "for_statement", "optional_direction",
  "forexprend", "run_statement", "fix_statement",
  "fix_and_assign_statement", "free_statement", "solver_statement",
  "option_statement", "solve_statement", "study_statement", "$@3",
  "study_vary_opt", "study_distribution_opt", "study_run_opt",
  "study_file_opt", "study_now_opt", "delete_statement",
  "external_statement", "call_statement", "assert_statement",
  "if_statement", "while_statement", "optional_else", "when_statement",
  "whenlist", "whenlistf", "flow_statement", "use_statement",
  "select_statement", "selectlist", "selectlistf", "switch_statement",
  "switchlist", "switchlistf", "conditional_statement", "notes_statement",
  "notes_body", "noteslist", "fvarlist", "varlist", "method_fvarlist",
  "method_varlist", "fvarref", "fname", "name", "end",
  "optional_bracedtext", "optional_notes", "set", "setexprlist", "number",
  "realnumber", "opunits", "dims", "dimensions", "dimexpr", "fraction",
  "fractail", "optional_sign", "expr", "relop", "logrelop", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-609)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-327)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -609,   541,  -609,  -609,   202,   -69,  -106,   -92,    49,    21,
      80,   -12,  -609,  -609,  -609,  -609,  -609,  -609,  -609,   -28,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,    29,  -609,  -609,
    -609,    46,   106,   158,  -609,  -609,   173,    85,  -609,    89,
     152,   210,   117,   190,  -609,    -5,   145,    -6,   257,   240,
     257,   178,   191,   199,  -609,   263,  -609,  -609,   198,   291,
     785,  -609,    10,   249,   257,   254,  -609,  -609,  -609,   597,
       5,    15,  -609,  -609,    19,   399,   256,  -609,  -609,   145,
     260,   -81,  -609,   -76,   145,   265,   271,   270,  -609,   272,
     273,  -609,   333,  -609,   291,   292,   375,  -609,   311,  -609,
     257,   279,  -609,   597,  -609,   294,   181,   188,  -609,  -609,
     335,   280,   299,  -609,  -609,    17,   300,    17,   597,   145,
     196,   284,   597,  -609,  -609,   597,   597,   -28,   302,   219,
    -609,   145,   286,   287,  -103,   145,   305,   307,   224,    74,
    -609,   145,   145,   309,  -609,   228,   295,   -82,   597,   145,
     312,   313,   304,  -609,   597,   597,   597,   597,  -609,  -609,
    -609,  -609,   303,   308,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,    58,   -45,   233,  -609,  1208,
     388,   310,   392,   314,   316,   317,  -609,   306,   324,   980,
    -609,  -609,   398,   319,   330,   325,   365,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
     342,   343,  -609,   145,  -609,  -609,  -609,   145,  -609,  -609,
    -609,  -609,  -609,   348,   257,   153,  -609,   291,   331,   334,
      20,  -609,   337,  -609,  1208,   338,  -609,   597,   597,   597,
     597,   257,  -609,   145,   341,   346,  -609,  -609,    -2,   -45,
    -609,   435,  -609,  -609,  1036,  -609,   597,   597,    24,  1208,
    1208,  -609,    -6,   597,   597,   597,   376,    17,   145,   145,
      28,  -609,  -609,  -609,  -609,   597,   597,   597,   145,    67,
     145,  -609,   461,   455,   597,   597,   222,   145,    88,  1098,
    -609,  -609,  -609,  -609,   597,   351,   351,   636,   347,  -609,
    -609,    95,  -609,  -609,   369,   370,   369,   369,  -609,  -609,
     597,   597,   386,   597,  -609,  -609,   597,   597,   597,  -609,
    -609,  -609,   597,  -609,  -609,  -609,   597,   597,   597,   597,
     597,   597,   597,   357,  -609,   374,   360,  -609,  -609,   145,
    -609,   597,   597,   361,  -609,   377,  -609,  -609,  -609,   363,
    -609,  -609,   257,   -18,   378,   -78,  -609,   488,  -609,   257,
    -609,  -609,   390,  -609,  -609,  -609,   597,   379,   371,   380,
     381,  -609,   382,   383,   145,   145,    17,  -609,   597,  -609,
     389,   384,   400,   406,   410,  -609,  1208,   402,   395,   145,
     413,    35,   403,   597,   405,   257,   151,   404,   397,  1124,
     407,   597,   408,   257,   156,   446,   506,   597,   415,   429,
     416,   417,   433,   436,   421,   597,   422,   257,   162,  -609,
     831,  -609,  -609,   145,  -609,   425,   505,   505,  -609,   505,
    1208,  1208,   110,   430,  -609,  1008,  1008,  1008,  1242,  1242,
     223,   223,   351,   351,   351,   177,    78,  -609,  -609,  -609,
     428,  1153,  1208,  -609,  -609,  -609,   432,   -78,  -609,   448,
     462,  -609,  -609,  -609,  -609,   160,  -609,   231,   126,   437,
     438,   440,   434,   441,  -609,  -609,  -609,  -609,  -609,   166,
     443,   445,  -609,   938,   556,  -609,  -609,   464,   145,   145,
    -609,  -609,  -609,   597,   267,  -609,    28,   449,  -609,  -609,
     597,   450,  -609,  -609,   597,    67,   453,  -609,  -609,   597,
     456,   145,   517,   369,  -609,  1208,   230,   458,  -609,  -609,
     145,   145,    88,   459,  -609,  -609,   597,   460,   257,  -609,
     463,   597,   477,    23,  -609,    23,   145,    88,   597,  -609,
     597,  -609,  -609,  -609,   141,   480,   185,   160,   160,   258,
    -609,  -609,  -609,  -609,  -609,   278,  -609,  -609,  -609,  -609,
     468,  -609,  -609,  -609,  -609,  -609,   212,  -609,   257,   482,
     469,   472,   875,   313,   473,   257,  -609,  -609,   476,  -609,
    1208,   257,  -609,  -609,   478,  -609,   583,   496,   571,   505,
      -9,  -609,  -609,  -609,  -609,   124,   483,   247,  -609,  -609,
     484,   485,   257,  -609,  -609,   481,  -609,  -609,   522,   487,
    -609,   597,   597,  -609,  -609,   489,   257,   490,   491,   142,
    1208,  -609,  -609,  -609,   278,   495,  -609,   497,   497,  -609,
    -111,   313,  -609,  -609,   502,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,   145,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,   597,  -609,  -609,   603,
    -609,   597,  -609,   520,  -609,  -609,   558,   509,  -609,   230,
     597,  -609,  -609,  -609,  -609,  -609,  -609,  -609,   145,  -609,
    1208,  1208,    88,  -609,   145,   605,  -609,  -609,  -609,   527,
    -609,  -609,   257,   512,  -609,  -609,  1180,   528,  -609,  1208,
    -609,    56,  -609,  -609,  -609,  1208,    14,  -609,   610,   257,
     518,   519,  -609,   525,  -609,  -609,   597,  -609,   638,   -10,
     521,   574,   542,   523,   529,  -609,  -609,   -19,   544,  -609,
     145,   530,   604,   551,   949,   588,   589,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,   648,   145,  -609,   -72,   559,   145,   649,
     622,   543,  -609,  -609,   545,   597,   597,   566,  -609,  -609,
     596,   547,   253,  -609,  -609,   569,  -609,  -609,   552,   -72,
     575,   683,  -609,  1208,  1208,   359,  -609,  -609,   253,  -609,
    -609,  -609,   655,  -609,   569,   582,  -609,  -609,  -609,  -609,
     658,   584,  -609,  -609,   565,  -609,   585,  -609,   597,  -609,
     568,  -609
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    16,     0,     0,     0,     0,     0,     0,
       0,   132,    64,     3,    15,     4,     5,     6,     9,     0,
       7,   145,     8,   145,    11,    10,    12,     0,    13,   150,
      14,     0,     0,     0,    66,   150,     0,     0,   328,     0,
      23,     0,     0,    20,   135,     0,     0,     0,     0,   144,
       0,     0,     0,     0,   150,     0,    42,   150,    57,   139,
       0,    67,     0,     0,     0,     0,    25,    21,    22,   351,
       0,     0,    18,    19,     0,   345,     0,   133,    68,   313,
       0,   317,   318,   348,     0,     0,     0,     0,   146,     0,
       0,    36,     0,    56,   139,     0,   142,   150,    59,   140,
       0,     0,   385,     0,   291,     0,     0,     0,   150,   292,
       0,     0,     0,   384,   293,     0,     0,     0,     0,     0,
       0,     0,     0,   380,   382,     0,     0,     0,     0,     0,
     294,     0,     0,     0,     0,   257,     0,   346,     0,     0,
     259,     0,     0,     0,   383,     0,     0,     0,     0,     0,
     360,   379,   328,   386,     0,     0,     0,   351,   193,   194,
     195,   196,     0,     0,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   232,   171,   174,   175,   176,   197,   177,   178,   180,
     179,   181,   182,   173,   172,   183,   184,   185,   186,   188,
     187,   189,   190,   191,   192,     0,     0,   387,   381,   234,
       0,     0,   210,     0,     0,     0,   387,     0,   350,   352,
     330,   329,     0,     0,   138,     0,    70,   332,   333,   341,
     334,   335,   336,   337,   338,   339,   340,   342,   343,   344,
       0,     0,   315,     0,   320,   349,   327,   314,    26,    28,
     148,   150,    31,     0,     0,   364,   150,   139,     0,     0,
       0,   145,     0,   153,   277,   206,   276,   351,   351,   351,
     351,     0,   274,     0,     0,     0,   253,   252,   321,   322,
     325,     0,   254,   322,     0,   224,   351,   351,     0,   236,
     235,   397,     0,     0,   351,   351,   250,     0,     0,     0,
       0,   258,   255,   347,   295,   351,   351,     0,     0,     0,
       0,    74,   100,     0,   351,   351,     0,     0,     0,     0,
     296,   361,   358,   359,     0,   403,   404,     0,     0,   151,
     152,     0,   217,   225,     0,     0,     0,     0,   226,   227,
       0,     0,     0,     0,   427,   428,     0,     0,     0,   424,
     425,   426,     0,   422,   421,   423,     0,     0,     0,     0,
       0,     0,     0,     0,    29,     0,     0,    17,    24,     0,
     331,     0,     0,     0,    32,     0,   136,    71,    69,     0,
     316,   319,     0,    46,     0,     0,   363,    37,   143,     0,
      58,   150,     0,    54,   141,    65,   351,     0,     0,     0,
       0,   311,     0,   318,     0,     0,     0,   324,     0,   150,
       0,     0,     0,     0,     0,   312,   256,     0,     0,     0,
       0,     0,     0,   351,     0,     0,   299,     0,     0,     0,
       0,   351,     0,     0,   306,   261,    79,     0,     0,     0,
       0,     0,     0,     0,     0,   351,     0,     0,   286,   150,
       0,   420,   389,     0,   200,   208,   212,   212,   205,   212,
     228,   229,     0,   328,   233,   395,   400,   396,   402,   401,
     390,   391,   393,   392,   394,   398,   399,    30,   211,    27,
       0,   354,   353,    33,   137,   134,     0,     0,    45,     0,
      48,    34,   368,   367,   365,     0,   362,   366,   376,     0,
       0,     0,    61,     0,   415,   416,   417,   418,   223,   388,
       0,     0,   323,   243,   280,   413,   414,     0,     0,     0,
     409,   410,   251,     0,     0,   406,     0,     0,   150,   297,
     351,     0,   407,   408,     0,     0,     0,   150,   304,   351,
       0,     0,   268,     0,    77,   101,     0,     0,   411,   412,
       0,     0,     0,     0,   150,   282,   351,     0,     0,   419,
       0,   351,     0,   214,   204,   214,     0,     0,     0,   388,
       0,   147,    44,    47,   376,   348,     0,     0,     0,   376,
      39,    40,    41,   377,   378,     0,    35,    53,    60,   150,
       0,   207,   275,   326,   245,   244,     0,   150,     0,     0,
       0,     0,     0,     0,     0,     0,   150,   301,     0,   150,
     231,     0,   150,   308,     0,   150,     0,     0,   272,   212,
       0,   110,   109,   107,   108,     0,     0,     0,   105,   111,
       0,     0,     0,   150,   288,     0,   150,   279,     0,     0,
     213,     0,     0,   198,   199,     0,     0,     0,     0,   387,
     355,    51,    50,    52,     0,     0,   372,   369,   370,   371,
       0,   356,    38,   357,     0,    55,   248,   246,   249,   247,
     150,   281,   278,     0,   219,   220,   230,   405,   298,   300,
     150,   303,   305,   307,   150,   310,     0,   269,   273,   270,
      80,     0,    81,     0,    75,    78,     0,     0,   102,     0,
       0,   113,   221,   222,   284,   287,   150,   290,     0,   209,
     216,   215,     0,   283,     0,     0,    49,    43,   374,     0,
     373,    62,     0,     0,   302,   309,     0,     0,   260,    82,
      83,     0,    99,   103,   106,   104,     0,   289,     0,     0,
     240,     0,   238,     0,   242,   218,     0,   271,     0,     0,
       0,     0,     0,     0,     0,   116,   117,     0,     0,   285,
       0,     0,     0,     0,     0,     0,     0,    93,    97,    96,
      94,    98,    95,    89,    88,    90,    91,    92,    87,    85,
      86,   115,   112,     0,     0,   114,     0,     0,     0,     0,
       0,     0,   237,   239,     0,     0,     0,     0,    76,    76,
       0,     0,     0,   123,   124,   127,   122,   131,     0,     0,
       0,     0,   375,   264,   263,   265,    73,    72,     0,   125,
     121,   128,   129,   126,   127,   202,   241,   266,   267,   262,
       0,     0,   119,   120,     0,   201,     0,   130,   351,   118,
       0,   203
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,   656,  -609,  -609,  -609,  -609,   -79,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,    96,  -609,    25,  -609,
    -609,  -609,  -609,  -609,  -609,   -87,  -608,  -609,  -101,  -609,
    -609,  -609,  -609,  -609,  -609,   -90,  -609,  -609,   -22,  -609,
    -609,  -609,   157,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -334,  -609,  -448,   161,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,   616,  -609,  -609,  -609,   391,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,
    -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -609,  -542,
    -609,  -609,  -609,  -609,   208,  -609,  -609,   201,  -609,  -609,
    -609,   598,   653,    27,  -609,   626,  -609,   -97,   -46,   387,
     -42,  -609,   169,  -150,  -609,    91,  -505,  -609,  -609,   261,
    -469,  -609,  -609,  -297,   -44,  -609,  -609
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    54,    55,   499,    25,    56,   490,
     575,    26,    57,    58,    98,   260,   590,    27,    28,    29,
      30,    31,   378,   159,   436,   731,   816,   620,   544,   695,
     749,   779,   780,   160,   438,   625,   626,   627,   628,   161,
     701,   736,   754,   755,   756,   805,   806,   757,   822,   832,
     789,    45,    77,    74,   225,   100,   261,   257,    48,    49,
      88,    89,    59,    60,   162,   163,   164,   165,   166,   835,
     167,   168,   266,   456,   366,   563,   643,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   647,   740,   761,   183,   596,   670,   184,   185,   186,
     187,   188,   189,   190,   191,   310,   542,   829,   618,   728,
     689,   192,   193,   194,   195,   196,   197,   598,   198,   447,
     448,   199,   200,   201,   425,   426,   202,   433,   434,   203,
     204,    47,    79,    80,    81,   277,   278,   206,   216,    83,
      78,   304,   246,   217,   218,   662,   208,   322,   387,   496,
     497,   659,   720,   585,   219,   361,   362
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      82,    50,   458,   459,   254,    85,    86,   328,    90,   564,
     632,   565,   487,   488,   207,   750,   209,   691,   279,   604,
     283,   718,   213,   786,   766,   646,   576,   275,    75,    75,
      38,   802,   226,    82,   220,   244,   787,   210,    82,   719,
      51,    36,   222,    35,    37,   423,   275,   299,   751,   641,
      52,    38,    38,    75,   492,   493,   243,   748,   262,   264,
     245,   803,   331,   804,    34,   332,   333,   494,   317,   280,
     412,   280,   495,    82,   284,   340,   341,    69,   289,    70,
     663,   290,   291,   343,   431,   296,    41,   205,    82,   301,
     -84,   489,   692,    82,   767,   311,   312,    53,    44,   392,
     693,    82,   424,   320,   319,   445,   241,    46,   657,   658,
     325,   326,   327,   334,   335,   336,   752,   397,   398,   399,
     400,   768,   769,   770,   771,   772,   346,   773,    76,    84,
     774,    38,   775,   776,   788,   406,   410,   411,   221,   777,
     694,   432,   778,   211,   417,   418,   285,   753,   223,   663,
      38,   347,   224,   642,    62,   427,   428,    38,   696,   413,
     -84,   300,   446,   580,   440,   441,   309,   389,   530,   393,
     739,   690,   524,   539,   318,   337,   338,   339,   651,   556,
     307,   407,    38,   385,   386,    39,   525,   -84,   -84,   -84,
     -84,   -84,    64,   -84,   820,    61,   -84,   381,   -84,   -84,
     420,    82,   349,   350,   351,   -84,    63,    38,   -84,   619,
     830,    94,   384,    38,    96,    65,    42,   353,   354,   355,
     356,   357,   358,   359,   308,   531,   360,   403,    38,   401,
     540,   666,   280,   581,    66,   667,   557,  -318,    67,   394,
    -318,  -318,   414,    38,   668,   453,   503,   669,   652,   416,
     415,   280,   421,    82,   258,   621,   622,   623,    71,   624,
     566,   582,    82,   429,    82,   271,    72,    32,   583,   584,
     443,    82,    33,   527,   241,  -318,   653,   654,    38,  -318,
     450,   536,   660,   583,   584,   454,  -326,  -326,  -318,  -318,
    -318,    75,   492,   493,  -318,   553,   460,   461,   209,   465,
     402,    68,   466,   467,   468,    69,    87,    70,   469,   512,
     495,    91,   470,   471,   472,   473,   474,   475,   476,   356,
     357,   358,   359,   480,    92,   360,   422,   481,   482,   577,
     578,   267,    93,   579,   268,   430,   656,   435,   269,    73,
     486,   270,    95,    69,   444,    70,   286,   500,    97,   287,
    -318,  -318,  -318,  -325,  -325,    38,    99,   442,    82,   511,
     280,   621,   622,   623,   513,   624,  -318,   358,   359,   294,
    -318,   360,   295,   522,   305,   577,   578,   306,   314,   579,
     608,   315,   212,   529,   699,   342,   803,   700,   804,   614,
     214,   538,   102,   545,   242,    40,   240,    43,   150,   603,
     583,   584,   106,   250,   107,   555,   635,    82,   382,   150,
     661,   639,   253,   388,   248,   215,    82,   827,   828,   227,
     249,   251,   252,   113,   256,   255,   259,   265,   263,   272,
     273,   510,   274,   281,   288,   293,   297,   298,   302,   228,
     120,   303,   313,   229,   230,   316,   321,   323,   122,   123,
     124,   231,   329,   125,   324,   126,   363,   330,   365,   364,
     370,   371,   129,   367,   232,   368,   373,   369,   374,   233,
     375,   133,    82,    82,   376,   377,   379,   380,   138,   602,
     560,   383,   390,   408,   391,   234,   395,   437,   396,   567,
     610,   404,   235,   144,   145,   616,   405,   439,   419,   360,
     462,   452,   455,   457,    82,    82,   477,   478,   236,   479,
     483,   484,   485,   237,   498,   238,   637,   150,   151,   463,
      82,   153,   649,   502,   450,   505,   650,   491,   154,   155,
     504,   506,   239,   508,   509,   507,   156,   517,   516,   157,
     515,     2,     3,   518,     4,   600,   601,   519,   501,   521,
     523,   533,   -63,   520,   526,   532,   672,   528,   535,   541,
     537,   543,   -63,   678,   546,   547,   514,   548,     5,   682,
     550,   549,   552,   551,   554,   561,   562,   630,   631,   569,
     568,   571,   573,   574,   589,     6,   586,   587,     7,   597,
     704,   588,   591,   645,   592,   648,   593,   710,   711,   599,
     617,   606,   609,   102,   713,   612,   558,   629,   615,   -63,
     640,   633,   636,   106,   638,   107,   245,   665,     8,   673,
     674,     9,    10,   675,   677,   686,   215,    82,   680,   687,
     684,   688,   698,   706,   113,   702,   703,   708,   709,   714,
     712,   343,   726,   715,   717,   579,   727,   729,   344,   345,
      11,   120,    12,   721,   730,   742,   735,   732,   733,   743,
     123,   124,    82,   745,   747,   758,   126,   760,    82,   763,
     781,   762,   765,   129,   782,   783,   784,   790,   785,   793,
     744,   792,   133,   794,   346,   607,   798,   799,   840,   138,
     800,   809,   807,   810,   613,   811,   812,   759,   815,   818,
     723,   819,   764,   821,   144,   145,   823,   826,   825,   347,
     831,   634,   834,   836,   791,   838,   158,   837,   839,   841,
     817,   697,   824,   833,   734,   292,   644,   348,   150,   151,
     152,   276,   153,   464,   605,   738,   611,   247,    82,   154,
     155,   741,    82,   282,   655,   716,   664,   156,   572,     0,
     157,   813,   814,     0,   671,     0,     0,     0,     0,     0,
     349,   350,   351,   679,     0,     0,   681,     0,     0,   683,
       0,     0,   685,     0,   352,   353,   354,   355,   356,   357,
     358,   359,     0,     0,   360,     0,   101,   451,     0,     0,
     705,   102,     0,   707,     0,   103,     0,     0,     0,   104,
     105,   106,  -149,   107,     0,   108,     0,   109,     0,     0,
       0,   801,     0,   110,   111,   808,     0,     0,  -149,  -149,
       0,   112,   113,   114,   115,   116,   117,   722,     0,     0,
     118,     0,     0,     0,  -149,     0,   343,   724,   119,   120,
       0,   725,     0,   344,   345,     0,   121,   122,   123,   124,
    -149,     0,   125,     0,   126,   127,     0,   128,     0,  -149,
       0,   129,     0,   737,     0,     0,     0,   130,   131,   132,
     133,   134,     0,   135,   136,   137,     0,   138,   139,   346,
     343,     0,   140,   141,   142,   143,     0,   344,   345,     0,
       0,     0,   144,   145,    11,     0,     0,   146,     0,   147,
       0,   148,     0,     0,   347,     0,     0,     0,   149,     0,
       0,     0,     0,     0,     0,     0,   150,   151,   152,     0,
     153,     0,   348,   346,     0,     0,     0,   154,   155,     0,
       0,     0,     0,     0,     0,   156,  -149,     0,   157,     0,
       0,     0,     0,   343,     0,     0,     0,     0,   347,     0,
     344,   345,     0,     0,   343,   349,   350,   351,     0,     0,
       0,   344,   345,   594,     0,     0,   348,     0,     0,   352,
     353,   354,   355,   356,   357,   358,   359,     0,     0,   360,
       0,     0,   559,     0,     0,   343,   346,     0,     0,   595,
       0,     0,   344,   345,     0,     0,     0,   346,     0,   349,
     350,   351,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   347,     0,   352,   353,   354,   355,   356,   357,   358,
     359,     0,   347,   360,     0,     0,   676,   795,   346,   348,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     348,   343,     0,     0,   796,   797,     0,     0,   344,   345,
       0,     0,     0,   347,     0,     0,     0,     0,     0,     0,
       0,     0,   349,   350,   351,     0,     0,     0,     0,     0,
       0,   348,     0,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   346,     0,   360,   352,   353,   354,
     355,   356,   357,   358,   359,     0,     0,   360,     0,     0,
       0,     0,     0,   343,   349,   350,   351,   372,     0,   347,
     344,   345,     0,     0,     0,     0,     0,     0,   352,   353,
     354,   355,   356,   357,   358,   359,     0,   348,   360,   343,
     449,     0,   349,   350,   351,     0,   344,   345,     0,     0,
       0,   409,     0,     0,     0,     0,   346,   353,   354,   355,
     356,   357,   358,   359,     0,     0,   360,     0,   343,     0,
     349,   350,   351,     0,     0,   344,   345,     0,     0,   534,
       0,   347,   346,     0,   352,   353,   354,   355,   356,   357,
     358,   359,     0,     0,   360,   343,     0,     0,     0,   348,
       0,     0,   344,   345,     0,     0,     0,   347,     0,     0,
       0,   346,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   343,     0,   348,     0,     0,     0,     0,
     344,   345,   349,   350,   351,     0,   347,     0,   346,     0,
       0,     0,     0,     0,     0,     0,   352,   353,   354,   355,
     356,   357,   358,   359,   348,     0,   360,   343,   349,   350,
     351,     0,     0,   347,   344,   345,   346,     0,     0,     0,
       0,     0,   352,   353,   354,   355,   356,   357,   358,   359,
       0,   348,   360,     0,     0,     0,     0,   349,   350,   351,
     570,   347,     0,     0,     0,     0,   746,     0,     0,     0,
     346,   352,   353,   354,   355,   356,   357,   358,   359,   348,
       0,   360,     0,     0,   349,   350,   351,     0,     0,     0,
       0,     0,     0,     0,     0,   347,     0,     0,   352,   353,
     354,   355,   356,   357,   358,   359,     0,     0,   360,     0,
       0,     0,   349,   350,   351,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   352,   353,   354,   355,
     356,   357,   358,   359,     0,     0,   360,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   349,   350,   351,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   353,   354,   355,   356,   357,   358,   359,     0,     0,
     360
};

static const yytype_int16 yycheck[] =
{
      46,    23,   336,   337,    94,    47,    48,   157,    50,   457,
     552,   459,    30,    31,    60,     1,    60,    26,   115,   524,
     117,   132,    64,    42,    34,   567,   495,    29,    34,    34,
     133,   103,    74,    79,    29,    81,    55,    27,    84,   150,
      11,   133,    27,   149,   136,    17,    29,   150,    34,    26,
      21,   133,   133,    34,   132,   133,   137,     1,   100,   103,
     136,   133,     4,   135,   133,     7,     8,   145,   150,   115,
      46,   117,   150,   119,   118,   120,   121,   153,   122,   155,
     585,   125,   126,     5,    17,   131,    65,    60,   134,   135,
      34,   109,   101,   139,   104,   141,   142,    68,   110,    79,
     109,   147,    74,   149,   148,    17,    79,   135,   577,   578,
     154,   155,   156,    55,    56,    57,   102,   267,   268,   269,
     270,   131,   132,   133,   134,   135,    48,   137,   133,   135,
     140,   133,   142,   143,   153,   137,   286,   287,   133,   149,
     149,    74,   152,   133,   294,   295,   119,   133,   133,   654,
     133,    73,   133,   130,    48,   305,   306,   133,    34,   135,
     104,   134,    74,    37,   314,   315,   139,   257,    17,   149,
     712,   619,   137,    17,   147,   117,   118,   119,    37,    17,
     106,   278,   133,    30,    31,   136,   151,   131,   132,   133,
     134,   135,    35,   137,   802,   149,   140,   243,   142,   143,
     297,   247,   124,   125,   126,   149,    48,   133,   152,   543,
     818,    54,   254,   133,    57,    42,   136,   139,   140,   141,
     142,   143,   144,   145,   150,    74,   148,   273,   133,   271,
      74,    19,   278,   107,   149,    23,    74,     4,   149,   261,
       7,     8,   288,   133,    32,   150,   396,    35,   107,   293,
     292,   297,   298,   299,    97,   131,   132,   133,    48,   135,
     150,   135,   308,   307,   310,   108,   149,    65,   142,   143,
     316,   317,    70,   423,   247,   133,   135,   574,   133,   137,
     324,   431,   579,   142,   143,   331,   120,   121,    55,    56,
      57,    34,   132,   133,   152,   445,   340,   341,   342,   343,
     273,   149,   346,   347,   348,   153,    66,   155,   352,   406,
     150,   133,   356,   357,   358,   359,   360,   361,   362,   142,
     143,   144,   145,   369,   133,   148,   299,   371,   372,   144,
     145,   150,   133,   148,   153,   308,   151,   310,   150,   149,
     382,   153,    79,   153,   317,   155,   150,   389,   150,   153,
     117,   118,   119,   120,   121,   133,    65,   135,   404,   405,
     406,   131,   132,   133,   408,   135,   133,   144,   145,   150,
     137,   148,   153,   419,   150,   144,   145,   153,   150,   148,
     530,   153,   133,   425,   137,   152,   133,   140,   135,   539,
     136,   433,     6,   437,   134,     8,   140,    10,   131,   132,
     142,   143,    16,   133,    18,   447,   556,   453,   251,   131,
     132,   561,    79,   256,   149,    29,   462,    58,    59,    20,
     149,   149,   149,    37,    49,   133,   115,   133,   149,    94,
     150,   404,   133,   133,   150,   133,   150,   150,   133,    40,
      54,   134,   133,    44,    45,   150,   134,   134,    62,    63,
      64,    52,   149,    67,   150,    69,    68,   149,    66,   149,
     154,   137,    76,   149,    65,   149,    68,   150,   149,    70,
     140,    85,   518,   519,   149,   110,   134,   134,    92,   523,
     453,   133,   151,    48,   150,    86,   149,    26,   150,   462,
     534,   150,    93,   107,   108,   541,   150,    42,   122,   148,
     114,   154,   133,   133,   550,   551,   149,   133,   109,   149,
     149,   134,   149,   114,    26,   116,   558,   131,   132,   133,
     566,   135,   568,   133,   568,   154,   570,   149,   142,   143,
     151,   151,   133,   151,   151,   154,   150,   137,   154,   153,
     151,     0,     1,   137,     3,   518,   519,   137,   391,   154,
     137,   154,    11,   151,   151,   151,   598,   152,   151,   113,
     152,    55,    21,   605,   149,   136,   409,   151,    27,   611,
     137,   154,   151,   137,   152,   150,    71,   550,   551,   151,
     150,   149,   134,   121,   150,    44,   149,   149,    47,    33,
     632,   151,   151,   566,   151,   568,   151,   641,   642,   135,
      83,   152,   152,     6,   646,   152,   449,   149,   152,    68,
     133,   152,   152,    16,   151,    18,   136,   149,    77,   137,
     151,    80,    81,   151,   151,    42,    29,   673,   152,   133,
     152,    60,   149,   152,    37,   151,   151,   115,   151,   149,
     151,     5,   686,   152,   149,   148,    43,   691,    12,    13,
     109,    54,   111,   151,   134,    50,   700,    99,   149,   132,
      63,    64,   708,   151,   136,    55,    69,   149,   714,   144,
     149,   152,    34,    76,   100,   133,   153,   133,   149,    75,
     722,   151,    85,   132,    48,   528,    98,    98,   838,    92,
      42,    42,   133,    71,   537,   152,   151,   739,   132,   103,
     673,   154,   746,   134,   107,   108,   154,    24,   133,    73,
      55,   554,   130,    55,   760,   150,    60,   133,   133,   151,
     799,   625,   809,   824,   699,   127,   565,    91,   131,   132,
     133,   115,   135,   342,   526,   708,   535,    84,   784,   142,
     143,   714,   788,   117,   575,   654,   589,   150,   487,    -1,
     153,   795,   796,    -1,   597,    -1,    -1,    -1,    -1,    -1,
     124,   125,   126,   606,    -1,    -1,   609,    -1,    -1,   612,
      -1,    -1,   615,    -1,   138,   139,   140,   141,   142,   143,
     144,   145,    -1,    -1,   148,    -1,     1,   151,    -1,    -1,
     633,     6,    -1,   636,    -1,    10,    -1,    -1,    -1,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    -1,    -1,
      -1,   784,    -1,    28,    29,   788,    -1,    -1,    33,    34,
      -1,    36,    37,    38,    39,    40,    41,   670,    -1,    -1,
      45,    -1,    -1,    -1,    49,    -1,     5,   680,    53,    54,
      -1,   684,    -1,    12,    13,    -1,    61,    62,    63,    64,
      65,    -1,    67,    -1,    69,    70,    -1,    72,    -1,    74,
      -1,    76,    -1,   706,    -1,    -1,    -1,    82,    83,    84,
      85,    86,    -1,    88,    89,    90,    -1,    92,    93,    48,
       5,    -1,    97,    98,    99,   100,    -1,    12,    13,    -1,
      -1,    -1,   107,   108,   109,    -1,    -1,   112,    -1,   114,
      -1,   116,    -1,    -1,    73,    -1,    -1,    -1,   123,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   131,   132,   133,    -1,
     135,    -1,    91,    48,    -1,    -1,    -1,   142,   143,    -1,
      -1,    -1,    -1,    -1,    -1,   150,   151,    -1,   153,    -1,
      -1,    -1,    -1,     5,    -1,    -1,    -1,    -1,    73,    -1,
      12,    13,    -1,    -1,     5,   124,   125,   126,    -1,    -1,
      -1,    12,    13,    25,    -1,    -1,    91,    -1,    -1,   138,
     139,   140,   141,   142,   143,   144,   145,    -1,    -1,   148,
      -1,    -1,   151,    -1,    -1,     5,    48,    -1,    -1,    51,
      -1,    -1,    12,    13,    -1,    -1,    -1,    48,    -1,   124,
     125,   126,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    73,    -1,   138,   139,   140,   141,   142,   143,   144,
     145,    -1,    73,   148,    -1,    -1,   151,    78,    48,    91,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      91,     5,    -1,    -1,    95,    96,    -1,    -1,    12,    13,
      -1,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   124,   125,   126,    -1,    -1,    -1,    -1,    -1,
      -1,    91,    -1,   124,   125,   126,   138,   139,   140,   141,
     142,   143,   144,   145,    48,    -1,   148,   138,   139,   140,
     141,   142,   143,   144,   145,    -1,    -1,   148,    -1,    -1,
      -1,    -1,    -1,     5,   124,   125,   126,   127,    -1,    73,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,   138,   139,
     140,   141,   142,   143,   144,   145,    -1,    91,   148,     5,
      32,    -1,   124,   125,   126,    -1,    12,    13,    -1,    -1,
      -1,   105,    -1,    -1,    -1,    -1,    48,   139,   140,   141,
     142,   143,   144,   145,    -1,    -1,   148,    -1,     5,    -1,
     124,   125,   126,    -1,    -1,    12,    13,    -1,    -1,    45,
      -1,    73,    48,    -1,   138,   139,   140,   141,   142,   143,
     144,   145,    -1,    -1,   148,     5,    -1,    -1,    -1,    91,
      -1,    -1,    12,    13,    -1,    -1,    -1,    73,    -1,    -1,
      -1,    48,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     5,    -1,    91,    -1,    -1,    -1,    -1,
      12,    13,   124,   125,   126,    -1,    73,    -1,    48,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   138,   139,   140,   141,
     142,   143,   144,   145,    91,    -1,   148,     5,   124,   125,
     126,    -1,    -1,    73,    12,    13,    48,    -1,    -1,    -1,
      -1,    -1,   138,   139,   140,   141,   142,   143,   144,   145,
      -1,    91,   148,    -1,    -1,    -1,    -1,   124,   125,   126,
     127,    73,    -1,    -1,    -1,    -1,   106,    -1,    -1,    -1,
      48,   138,   139,   140,   141,   142,   143,   144,   145,    91,
      -1,   148,    -1,    -1,   124,   125,   126,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,   138,   139,
     140,   141,   142,   143,   144,   145,    -1,    -1,   148,    -1,
      -1,    -1,   124,   125,   126,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   138,   139,   140,   141,
     142,   143,   144,   145,    -1,    -1,   148,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   124,   125,   126,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   139,   140,   141,   142,   143,   144,   145,    -1,    -1,
     148
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   157,     0,     1,     3,    27,    44,    47,    77,    80,
      81,   109,   111,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   173,   177,   183,   184,   185,
     186,   187,    65,    70,   133,   149,   133,   136,   133,   136,
     305,    65,   136,   305,   110,   217,   135,   297,   224,   225,
     224,    11,    21,    68,   170,   171,   174,   178,   179,   228,
     229,   149,    48,    48,   228,    42,   149,   149,   149,   153,
     155,    48,   149,   149,   219,    34,   133,   218,   306,   298,
     299,   300,   304,   305,   135,   306,   306,    66,   226,   227,
     306,   133,   133,   133,   228,    79,   228,   150,   180,    65,
     221,     1,     6,    10,    14,    15,    16,    18,    20,    22,
      28,    29,    36,    37,    38,    39,    40,    41,    45,    53,
      54,    61,    62,    63,    64,    67,    69,    70,    72,    76,
      82,    83,    84,    85,    86,    88,    89,    90,    92,    93,
      97,    98,    99,   100,   107,   108,   112,   114,   116,   123,
     131,   132,   133,   135,   142,   143,   150,   153,   187,   189,
     199,   205,   230,   231,   232,   233,   234,   236,   237,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   260,   263,   264,   265,   266,   267,   268,
     269,   270,   277,   278,   279,   280,   281,   282,   284,   287,
     288,   289,   292,   295,   296,   299,   303,   304,   312,   320,
      27,   133,   133,   306,   136,    29,   304,   309,   310,   320,
      29,   133,    27,   133,   133,   220,   306,    20,    40,    44,
      45,    52,    65,    70,    86,    93,   109,   114,   116,   133,
     140,   299,   134,   137,   304,   136,   308,   298,   149,   149,
     133,   149,   149,    79,   221,   133,    49,   223,   228,   115,
     181,   222,   306,   149,   320,   133,   238,   150,   153,   150,
     153,   228,    94,   150,   133,    29,   251,   301,   302,   303,
     304,   133,   301,   303,   320,   299,   150,   153,   150,   320,
     320,   320,   297,   133,   150,   153,   304,   150,   150,   150,
     299,   304,   133,   134,   307,   150,   153,   106,   150,   299,
     271,   304,   304,   133,   150,   153,   150,   150,   299,   320,
     304,   134,   313,   134,   150,   320,   320,   320,   309,   149,
     149,     4,     7,     8,    55,    56,    57,   117,   118,   119,
     120,   121,   152,     5,    12,    13,    48,    73,    91,   124,
     125,   126,   138,   139,   140,   141,   142,   143,   144,   145,
     148,   321,   322,    68,   149,    66,   240,   149,   149,   150,
     154,   137,   127,    68,   149,   140,   149,   110,   188,   134,
     134,   304,   228,   133,   306,    30,    31,   314,   228,   221,
     151,   150,    79,   149,   224,   149,   150,   309,   309,   309,
     309,   306,   299,   304,   150,   150,   137,   303,    48,   105,
     309,   309,    46,   135,   304,   306,   320,   309,   309,   122,
     303,   304,   299,    17,    74,   290,   291,   309,   309,   320,
     299,    17,    74,   293,   294,   299,   190,    26,   200,    42,
     309,   309,   135,   304,   299,    17,    74,   285,   286,    32,
     320,   151,   154,   150,   304,   133,   239,   133,   239,   239,
     320,   320,   114,   133,   255,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   149,   133,   149,
     304,   320,   320,   149,   134,   149,   306,    30,    31,   109,
     175,   149,   132,   133,   145,   150,   315,   316,    26,   172,
     306,   228,   133,   309,   151,   154,   151,   154,   151,   151,
     299,   304,   303,   320,   228,   151,   154,   137,   137,   137,
     151,   154,   304,   137,   137,   151,   151,   309,   152,   306,
      17,    74,   151,   154,    45,   151,   309,   152,   306,    17,
      74,   113,   272,    55,   194,   320,   149,   136,   151,   154,
     137,   137,   151,   309,   152,   306,    17,    74,   228,   151,
     299,   150,    71,   241,   241,   241,   150,   299,   150,   151,
     127,   149,   315,   134,   121,   176,   316,   144,   145,   148,
      37,   107,   135,   142,   143,   319,   149,   149,   151,   150,
     182,   151,   151,   151,    25,    51,   261,    33,   283,   135,
     299,   299,   320,   132,   312,   290,   152,   228,   309,   152,
     320,   293,   152,   228,   309,   152,   304,    83,   274,   239,
     193,   131,   132,   133,   135,   201,   202,   203,   204,   149,
     299,   299,   285,   152,   228,   309,   152,   306,   151,   309,
     133,    26,   130,   242,   242,   299,   285,   257,   299,   304,
     320,    37,   107,   135,   319,   308,   151,   316,   316,   317,
     319,   132,   311,   312,   228,   149,    19,    23,    32,    35,
     262,   228,   306,   137,   151,   151,   151,   151,   306,   228,
     152,   228,   306,   228,   152,   228,    42,   133,    60,   276,
     241,    26,   101,   109,   149,   195,    34,   202,   149,   137,
     140,   206,   151,   151,   306,   228,   152,   228,   115,   151,
     320,   320,   151,   306,   149,   152,   311,   149,   132,   150,
     318,   151,   228,   299,   228,   228,   320,    43,   275,   320,
     134,   191,    99,   149,   204,   320,   207,   228,   299,   285,
     258,   299,    50,   132,   306,   151,   106,   136,     1,   196,
       1,    34,   102,   133,   208,   209,   210,   213,    55,   306,
     149,   259,   152,   144,   320,    34,    34,   104,   131,   132,
     133,   134,   135,   137,   140,   142,   143,   149,   152,   197,
     198,   149,   100,   133,   153,   149,    42,    55,   153,   216,
     133,   304,   151,    75,   132,    78,    95,    96,    98,    98,
      42,   299,   103,   133,   135,   211,   212,   133,   299,    42,
      71,   152,   151,   320,   320,   132,   192,   192,   103,   154,
     212,   134,   214,   154,   211,   133,    24,    58,    59,   273,
     212,    55,   215,   214,   130,   235,    55,   133,   150,   133,
     309,   151
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   156,   157,   157,   158,   158,   158,   158,   158,   158,
     158,   158,   158,   158,   158,   158,   158,   159,   160,   160,
     160,   161,   161,   161,   162,   162,   163,   164,   165,   166,
     166,   167,   168,   168,   169,   170,   171,   172,   172,   172,
     172,   172,   173,   174,   175,   175,   175,   175,   176,   176,
     176,   176,   176,   177,   178,   178,   179,   180,   180,   181,
     181,   182,   182,   183,   183,   184,   185,   186,   187,   187,
     188,   188,   189,   189,   190,   191,   192,   193,   193,   194,
     194,   195,   195,   195,   196,   196,   197,   197,   197,   197,
     197,   197,   197,   197,   198,   198,   198,   198,   198,   199,
     200,   200,   201,   201,   202,   203,   203,   204,   204,   204,
     204,   206,   205,   207,   207,   207,   208,   208,   209,   210,
     210,   211,   211,   212,   212,   213,   213,   214,   214,   215,
     215,   216,   217,   217,   218,   219,   219,   220,   220,   221,
     222,   221,   223,   223,   224,   225,   225,   226,   227,   228,
     229,   229,   229,   229,   230,   230,   230,   230,   230,   230,
     230,   230,   230,   230,   230,   230,   230,   230,   230,   230,
     230,   230,   230,   230,   230,   230,   230,   230,   230,   230,
     230,   230,   230,   230,   230,   230,   230,   230,   230,   230,
     230,   230,   230,   230,   230,   230,   230,   231,   232,   233,
     234,   234,   235,   235,   236,   237,   238,   238,   239,   239,
     240,   240,   241,   241,   242,   242,   242,   243,   244,   244,
     244,   245,   245,   246,   247,   248,   249,   250,   251,   251,
     252,   253,   254,   254,   255,   255,   255,   256,   257,   258,
     259,   259,   260,   261,   261,   261,   262,   262,   262,   262,
     263,   263,   264,   265,   266,   267,   268,   269,   269,   271,
     270,   272,   272,   272,   272,   273,   273,   273,   274,   274,
     275,   275,   276,   276,   277,   278,   279,   280,   281,   282,
     283,   283,   284,   284,   284,   284,   285,   286,   286,   286,
     286,   287,   287,   287,   287,   287,   288,   289,   289,   290,
     291,   291,   291,   291,   292,   292,   293,   294,   294,   294,
     294,   295,   296,   297,   297,   298,   298,   299,   300,   300,
     300,   301,   302,   302,   302,   303,   303,   304,   305,   305,
     305,   305,   306,   306,   306,   306,   306,   306,   306,   306,
     306,   306,   306,   306,   306,   306,   307,   307,   308,   308,
     309,   309,   310,   310,   310,   310,   311,   311,   312,   312,
     313,   313,   314,   314,   314,   315,   315,   316,   316,   316,
     316,   316,   316,   317,   318,   318,   319,   319,   319,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   321,   321,   321,   321,   321,   321,   322,   322
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     5,     3,     3,
       2,     3,     3,     2,     5,     3,     4,     6,     4,     5,
       6,     4,     5,     6,     6,     6,     2,     0,     3,     2,
       2,     2,     2,     8,     2,     1,     0,     2,     0,     3,
       2,     2,     2,     7,     4,     7,     2,     0,     3,     0,
       4,     0,     3,     0,     1,     5,     2,     2,     3,     5,
       0,     1,    11,    11,     0,     0,     0,     0,     2,     0,
       3,     1,     2,     2,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     7,
       0,     2,     2,     3,     3,     1,     3,     1,     1,     1,
       1,     0,     9,     0,     3,     3,     1,     1,     7,     5,
       5,     2,     1,     1,     1,     4,     4,     0,     1,     0,
       2,     2,     0,     2,     4,     0,     3,     3,     1,     0,
       0,     3,     0,     2,     1,     0,     2,     5,     2,     1,
       0,     3,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     5,     5,
       3,    12,     0,     4,     4,     3,     1,     4,     1,     4,
       0,     2,     0,     2,     0,     2,     2,     2,     8,     6,
       6,     6,     6,     4,     2,     2,     2,     2,     3,     3,
       6,     5,     1,     3,     1,     2,     2,     9,     3,     3,
       0,     4,     8,     0,     1,     1,     1,     1,     1,     1,
       2,     4,     2,     2,     2,     2,     3,     1,     2,     0,
       7,     0,     9,     8,     8,     0,     1,     1,     0,     2,
       0,     2,     0,     1,     2,     5,     2,     2,     6,     5,
       0,     2,     4,     6,     6,     8,     1,     4,     3,     5,
       4,     1,     1,     1,     1,     2,     2,     4,     6,     1,
       4,     3,     5,     4,     4,     6,     1,     4,     3,     5,
       4,     3,     3,     2,     3,     2,     3,     1,     1,     3,
       2,     1,     1,     3,     2,     1,     4,     2,     1,     3,
       3,     4,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     1,     0,     1,     0,     1,
       1,     0,     1,     3,     3,     5,     1,     1,     2,     2,
       0,     1,     2,     1,     0,     1,     1,     1,     1,     3,
       3,     3,     3,     2,     1,     5,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     3,     3,
       3,     3,     3,     2,     2,     6,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       3,     1,     1,     1,     1,     1,     1,     1,     1
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
#line 899 "ascend/compiler/ascParse.y"
        {
	  /*
	   * The specific syntax diagnostic is already emitted by zz_error().
	   * Emitting a generic message here causes noisy duplicates during
	   * parser recovery.
	   */
	}
#line 2865 "ascend/compiler/ascParse.c"
    break;

  case 17: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 910 "ascend/compiler/ascParse.y"
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
#line 2911 "ascend/compiler/ascParse.c"
    break;

  case 18: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 955 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 2919 "ascend/compiler/ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK name ';'  */
#line 959 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2928 "ascend/compiler/ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name  */
#line 964 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2937 "ascend/compiler/ascParse.c"
    break;

  case 21: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 972 "ascend/compiler/ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 2945 "ascend/compiler/ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK name ';'  */
#line 976 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2954 "ascend/compiler/ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name  */
#line 981 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2963 "ascend/compiler/ascParse.c"
    break;

  case 24: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 989 "ascend/compiler/ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
            );
          }
	}
#line 2976 "ascend/compiler/ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 998 "ascend/compiler/ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 2989 "ascend/compiler/ascParse.c"
    break;

  case 26: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 1010 "ascend/compiler/ascParse.y"
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
#line 3021 "ascend/compiler/ascParse.c"
    break;

  case 27: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 1041 "ascend/compiler/ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 3030 "ascend/compiler/ascParse.c"
    break;

  case 28: /* add_method_def: add_method_head proclist end ';'  */
#line 1049 "ascend/compiler/ascParse.y"
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
#line 3054 "ascend/compiler/ascParse.c"
    break;

  case 29: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1072 "ascend/compiler/ascParse.y"
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
#line 3071 "ascend/compiler/ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1085 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3080 "ascend/compiler/ascParse.c"
    break;

  case 31: /* replace_method_def: replace_method_head proclist end ';'  */
#line 1093 "ascend/compiler/ascParse.y"
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
#line 3102 "ascend/compiler/ascParse.c"
    break;

  case 32: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1114 "ascend/compiler/ascParse.y"
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
#line 3118 "ascend/compiler/ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1126 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3127 "ascend/compiler/ascParse.c"
    break;

  case 34: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 1134 "ascend/compiler/ascParse.y"
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
	                                g_default_units,
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
#line 3181 "ascend/compiler/ascParse.c"
    break;

  case 35: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 1187 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 3193 "ascend/compiler/ascParse.c"
    break;

  case 36: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 1198 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3202 "ascend/compiler/ascParse.c"
    break;

  case 37: /* default_val: %empty  */
#line 1206 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3213 "ascend/compiler/ascParse.c"
    break;

  case 38: /* default_val: DEFAULT_TOK optional_sign number  */
#line 1213 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3223 "ascend/compiler/ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 1219 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 3235 "ascend/compiler/ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 1227 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 3247 "ascend/compiler/ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 1235 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 3259 "ascend/compiler/ascParse.c"
    break;

  case 42: /* constant_def: universal constant_head  */
#line 1246 "ascend/compiler/ascParse.y"
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
	                                    (g_constant_units != NULL ? g_constant_units : g_default_units),
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
#line 3300 "ascend/compiler/ascParse.c"
    break;

  case 43: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK constant_dims constant_val optional_notes ';'  */
#line 1287 "ascend/compiler/ascParse.y"
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
#line 3334 "ascend/compiler/ascParse.c"
    break;

  case 44: /* constant_dims: DIMENSION_TOK dimensions  */
#line 1320 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	  g_constant_units = NULL;
	}
#line 3343 "ascend/compiler/ascParse.c"
    break;

  case 45: /* constant_dims: DIMENSIONLESS_TOK  */
#line 1325 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	  g_constant_units = NULL;
	}
#line 3352 "ascend/compiler/ascParse.c"
    break;

  case 46: /* constant_dims: %empty  */
#line 1330 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	  g_constant_units = NULL;
	}
#line 3361 "ascend/compiler/ascParse.c"
    break;

  case 47: /* constant_dims: UNITS_TOK BRACEDTEXT_TOK  */
#line 1335 "ascend/compiler/ascParse.y"
        {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((yyvsp[0].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (yyval.dimp) = UnitsDimensions(g_units_ptr);
	    g_constant_units = UnitsDescription(g_units_ptr);
	  } else {
	    char **errv;
	    (yyval.dimp) = WildDimension();
	    g_constant_units = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'",(yyvsp[0].braced_ptr));
	    errv = UnitsExplainError((yyvsp[0].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
#line 3383 "ascend/compiler/ascParse.c"
    break;

  case 48: /* constant_val: %empty  */
#line 1356 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3394 "ascend/compiler/ascParse.c"
    break;

  case 49: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 1363 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3404 "ascend/compiler/ascParse.c"
    break;

  case 50: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 1369 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3416 "ascend/compiler/ascParse.c"
    break;

  case 51: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 1377 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3428 "ascend/compiler/ascParse.c"
    break;

  case 52: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 1385 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 3440 "ascend/compiler/ascParse.c"
    break;

  case 53: /* model_def: universal model_head fstatements initial methods end ';'  */
#line 1396 "ascend/compiler/ascParse.y"
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
	                               (yyvsp[-6].int_value),
	                               (yyvsp[-4].slptr),
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
#line 3483 "ascend/compiler/ascParse.c"
    break;

  case 54: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 1439 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 3495 "ascend/compiler/ascParse.c"
    break;

  case 55: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 1448 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 3508 "ascend/compiler/ascParse.c"
    break;

  case 56: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 1460 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3517 "ascend/compiler/ascParse.c"
    break;

  case 57: /* optional_model_parameters: %empty  */
#line 1468 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3525 "ascend/compiler/ascParse.c"
    break;

  case 58: /* optional_model_parameters: '(' fstatements ')'  */
#line 1472 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3533 "ascend/compiler/ascParse.c"
    break;

  case 59: /* optional_parameter_wheres: %empty  */
#line 1479 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3541 "ascend/compiler/ascParse.c"
    break;

  case 60: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 1483 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3549 "ascend/compiler/ascParse.c"
    break;

  case 61: /* optional_parameter_reduction: %empty  */
#line 1490 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3557 "ascend/compiler/ascParse.c"
    break;

  case 62: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 1494 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3565 "ascend/compiler/ascParse.c"
    break;

  case 63: /* universal: %empty  */
#line 1544 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3573 "ascend/compiler/ascParse.c"
    break;

  case 64: /* universal: UNIVERSAL_TOK  */
#line 1548 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3581 "ascend/compiler/ascParse.c"
    break;

  case 65: /* definition_def: definition_id fstatements methods end ';'  */
#line 1555 "ascend/compiler/ascParse.y"
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
#line 3612 "ascend/compiler/ascParse.c"
    break;

  case 66: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1585 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3621 "ascend/compiler/ascParse.c"
    break;

  case 67: /* units_def: units_statement ';'  */
#line 1594 "ascend/compiler/ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 3627 "ascend/compiler/ascParse.c"
    break;

  case 68: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1599 "ascend/compiler/ascParse.y"
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
#line 3648 "ascend/compiler/ascParse.c"
    break;

  case 69: /* units_statement: UNITS_TOK LADDER_TOK unitladderitemlist end optional_ladder_end  */
#line 1616 "ascend/compiler/ascParse.y"
        {
	  struct UnitLadderItem *item;
	  unsigned long c,len;
	  int ladder_errors;

	  if( (yyvsp[-1].int_value) != UNITS_TOK ) {
	    WarnMsg_MismatchEnd("UNITS LADDER", NULL, (yyvsp[-1].int_value), NULL);
	  }
	  ladder_errors = ProcessUnitLadder((yyvsp[-2].listp));
	  if (ladder_errors) {
	    g_untrapped_error++;
	  }

	  len = gl_length((yyvsp[-2].listp));
	  for (c=1; c <= len; c++) {
	    item = (struct UnitLadderItem *)gl_fetch((yyvsp[-2].listp),c);
	    DestroyUnitLadderItem(item);
	  }
	  gl_destroy((yyvsp[-2].listp));
	  (yyval.statptr) = NULL;
	}
#line 3674 "ascend/compiler/ascParse.c"
    break;

  case 70: /* optional_ladder_end: %empty  */
#line 1641 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3682 "ascend/compiler/ascParse.c"
    break;

  case 71: /* optional_ladder_end: LADDER_TOK  */
#line 1645 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3690 "ascend/compiler/ascParse.c"
    break;

  case 72: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on table_body END_TOK TABLE_TOK table_mode_off  */
#line 1652 "ascend/compiler/ascParse.y"
        {
	  char *table_body;
	  table_body = TableParseFinish();
	  (yyval.statptr) = CreateTABLE((yyvsp[-9].nptr),
	                   g_table_parse.decl_type,
	                   g_table_parse.decl_typeargs,
	                   g_table_parse.decl_set_type,
	                   g_table_parse.units,
	                   g_table_parse.default_expr,
	                   g_table_parse.positional,
	                   g_table_parse.rows,
	                   g_table_parse.scalars,
	                   g_table_parse.items,
	                   table_body);
	  g_table_parse.decl_type = NULL;
	  g_table_parse.decl_typeargs = NULL;
	  g_table_parse.decl_set_type = NULL;
	  g_table_parse.units = NULL;
	  g_table_parse.default_expr = NULL;
	}
#line 3715 "ascend/compiler/ascParse.c"
    break;

  case 73: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on error END_TOK TABLE_TOK table_mode_off  */
#line 1673 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-9].nptr));
	  TableParseAbort();
	  ErrMsg_Generic("Error in TABLE body.");
	  g_untrapped_error++;
	  yyerrok;
	  (yyval.statptr) = NULL;
	}
#line 3728 "ascend/compiler/ascParse.c"
    break;

  case 74: /* table_begin: %empty  */
#line 1685 "ascend/compiler/ascParse.y"
        {
	  TableParseBegin();
	}
#line 3736 "ascend/compiler/ascParse.c"
    break;

  case 75: /* table_mode_on: %empty  */
#line 1692 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(1);
	}
#line 3744 "ascend/compiler/ascParse.c"
    break;

  case 76: /* table_mode_off: %empty  */
#line 1699 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(0);
	}
#line 3752 "ascend/compiler/ascParse.c"
    break;

  case 79: /* table_decl_opt: %empty  */
#line 1711 "ascend/compiler/ascParse.y"
        {
	  g_table_parse.decl_type = NULL;
	  g_table_parse.decl_set_type = NULL;
	  if (g_table_parse.units != NULL) {
	    ascfree(g_table_parse.units);
	  }
	  g_table_parse.units = NULL;
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_typeargs = NULL;
	}
#line 3769 "ascend/compiler/ascParse.c"
    break;

  case 80: /* table_decl_opt: ISA_TOK type_identifier optional_of  */
#line 1724 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_type = (yyvsp[-1].id_ptr);
	  g_table_parse.decl_set_type = (yyvsp[0].id_ptr);
	  g_table_parse.decl_typeargs = g_typeargs;
	  g_typeargs = NULL;
	}
#line 3783 "ascend/compiler/ascParse.c"
    break;

  case 81: /* table_option: POSITIONAL_TOK  */
#line 1737 "ascend/compiler/ascParse.y"
        {
	  g_table_parse.positional = 1;
	}
#line 3791 "ascend/compiler/ascParse.c"
    break;

  case 82: /* table_option: DEFAULT_TOK expr  */
#line 1741 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.default_expr != NULL) {
	    DestroyExprList(g_table_parse.default_expr);
	  }
	  g_table_parse.default_expr = (yyvsp[0].eptr);
	}
#line 3802 "ascend/compiler/ascParse.c"
    break;

  case 83: /* table_option: UNITS_TOK BRACEDTEXT_TOK  */
#line 1748 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.units != NULL) {
	    ascfree(g_table_parse.units);
	  }
	  g_table_parse.units = ASC_STRDUP((yyvsp[0].braced_ptr));
	}
#line 3813 "ascend/compiler/ascParse.c"
    break;

  case 87: /* table_body_item: ':'  */
#line 1764 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(":",0);
	}
#line 3821 "ascend/compiler/ascParse.c"
    break;

  case 88: /* table_body_item: '='  */
#line 1768 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("=",0);
	}
#line 3829 "ascend/compiler/ascParse.c"
    break;

  case 89: /* table_body_item: ','  */
#line 1772 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(",",0);
	}
#line 3837 "ascend/compiler/ascParse.c"
    break;

  case 90: /* table_body_item: '+'  */
#line 1776 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("+",0);
	}
#line 3845 "ascend/compiler/ascParse.c"
    break;

  case 91: /* table_body_item: '-'  */
#line 1780 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("-",0);
	}
#line 3853 "ascend/compiler/ascParse.c"
    break;

  case 92: /* table_body_item: ';'  */
#line 1784 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3861 "ascend/compiler/ascParse.c"
    break;

  case 93: /* table_body_item: EOL_TOK  */
#line 1788 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3869 "ascend/compiler/ascParse.c"
    break;

  case 94: /* table_scalar: IDENTIFIER_TOK  */
#line 1795 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(SCP((yyvsp[0].id_ptr)),1);
	}
#line 3877 "ascend/compiler/ascParse.c"
    break;

  case 95: /* table_scalar: SYMBOL_TOK  */
#line 1799 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendSymbol((yyvsp[0].sym_ptr));
	}
#line 3885 "ascend/compiler/ascParse.c"
    break;

  case 96: /* table_scalar: INTEGER_TOK  */
#line 1803 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendInteger((yyvsp[0].int_value));
	}
#line 3893 "ascend/compiler/ascParse.c"
    break;

  case 97: /* table_scalar: REAL_TOK  */
#line 1807 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendReal((yyvsp[0].real_value));
	}
#line 3901 "ascend/compiler/ascParse.c"
    break;

  case 98: /* table_scalar: BRACEDTEXT_TOK  */
#line 1811 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendBraced((yyvsp[0].braced_ptr));
	}
#line 3909 "ascend/compiler/ascParse.c"
    break;

  case 99: /* values_statement: VALUES_TOK fname values_default_opt ';' values_entries END_TOK VALUES_TOK  */
#line 1818 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-5].nptr));
	  (yyval.statptr) = NULL;
	}
#line 3918 "ascend/compiler/ascParse.c"
    break;

  case 101: /* values_default_opt: DEFAULT_TOK expr  */
#line 1827 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3926 "ascend/compiler/ascParse.c"
    break;

  case 104: /* values_entry: values_key_list '=' expr  */
#line 1839 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3934 "ascend/compiler/ascParse.c"
    break;

  case 111: /* $@1: %empty  */
#line 1858 "ascend/compiler/ascParse.y"
        {
	  DatasetParseBegin((yyvsp[-3].id_ptr),(yyvsp[-1].dquote_ptr));
	}
#line 3942 "ascend/compiler/ascParse.c"
    break;

  case 112: /* dataset_statement: DATASET_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';' $@1 dataset_items END_TOK DATASET_TOK  */
#line 1862 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = DatasetParseFinish();
	}
#line 3950 "ascend/compiler/ascParse.c"
    break;

  case 115: /* dataset_items: dataset_items error ';'  */
#line 1871 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in DATASET item.");
	  ErrMsg_Generic("Check DATASET statement syntax.");
	  g_untrapped_error++;
	  yyerrok;
	}
#line 3961 "ascend/compiler/ascParse.c"
    break;

  case 118: /* dataset_index_item: INDEX_TOK IDENTIFIER_TOK FROM_TOK COLUMN_TOK dataset_column_ref ISA_TOK IDENTIFIER_TOK  */
#line 1886 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddIndex((yyvsp[-5].id_ptr),(yyvsp[-2].id_ptr),(yyvsp[0].id_ptr));
	}
#line 3969 "ascend/compiler/ascParse.c"
    break;

  case 119: /* dataset_map_item: dataset_target FROM_TOK dataset_column_selector dataset_units_opt dataset_type_opt  */
#line 1893 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-2].id_ptr),(yyvsp[-1].braced_ptr),(yyvsp[0].id_ptr));
	}
#line 3977 "ascend/compiler/ascParse.c"
    break;

  case 120: /* dataset_map_item: dataset_target dataset_type_req FROM_TOK dataset_column_selector dataset_units_opt  */
#line 1897 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-1].id_ptr),(yyvsp[0].braced_ptr),(yyvsp[-3].id_ptr));
	}
#line 3985 "ascend/compiler/ascParse.c"
    break;

  case 121: /* dataset_column_selector: COLUMN_TOK dataset_column_ref  */
#line 1904 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3993 "ascend/compiler/ascParse.c"
    break;

  case 122: /* dataset_column_selector: dataset_column_ref  */
#line 1908 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4001 "ascend/compiler/ascParse.c"
    break;

  case 123: /* dataset_column_ref: IDENTIFIER_TOK  */
#line 1915 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4009 "ascend/compiler/ascParse.c"
    break;

  case 124: /* dataset_column_ref: SYMBOL_TOK  */
#line 1919 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].sym_ptr);
	}
#line 4017 "ascend/compiler/ascParse.c"
    break;

  case 125: /* dataset_target: IDENTIFIER_TOK '[' fvarlist ']'  */
#line 1926 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices(CreateIdName((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4026 "ascend/compiler/ascParse.c"
    break;

  case 126: /* dataset_target: dataset_target '[' fvarlist ']'  */
#line 1931 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices((yyvsp[-3].nptr),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4035 "ascend/compiler/ascParse.c"
    break;

  case 127: /* dataset_units_opt: %empty  */
#line 1939 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 4043 "ascend/compiler/ascParse.c"
    break;

  case 128: /* dataset_units_opt: BRACEDTEXT_TOK  */
#line 1943 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 4051 "ascend/compiler/ascParse.c"
    break;

  case 129: /* dataset_type_opt: %empty  */
#line 1950 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4059 "ascend/compiler/ascParse.c"
    break;

  case 130: /* dataset_type_opt: ISA_TOK IDENTIFIER_TOK  */
#line 1954 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4067 "ascend/compiler/ascParse.c"
    break;

  case 131: /* dataset_type_req: ISA_TOK IDENTIFIER_TOK  */
#line 1961 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4075 "ascend/compiler/ascParse.c"
    break;

  case 132: /* unitdeflist: %empty  */
#line 1967 "ascend/compiler/ascParse.y"
                {
		  (yyval.listp) = gl_create(100L);
	}
#line 4083 "ascend/compiler/ascParse.c"
    break;

  case 133: /* unitdeflist: unitdeflist unitdef  */
#line 1971 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 4092 "ascend/compiler/ascParse.c"
    break;

  case 134: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1979 "ascend/compiler/ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4101 "ascend/compiler/ascParse.c"
    break;

  case 135: /* unitladderitemlist: %empty  */
#line 1986 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(20L);
	}
#line 4109 "ascend/compiler/ascParse.c"
    break;

  case 136: /* unitladderitemlist: unitladderitemlist unitladderitem ';'  */
#line 1990 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].ulitemptr));
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4118 "ascend/compiler/ascParse.c"
    break;

  case 137: /* unitladderitem: IDENTIFIER_TOK '=' BRACEDTEXT_TOK  */
#line 1998 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[-2].id_ptr),(yyvsp[0].braced_ptr),0,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4127 "ascend/compiler/ascParse.c"
    break;

  case 138: /* unitladderitem: IDENTIFIER_TOK  */
#line 2003 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[0].id_ptr),NULL,1,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4136 "ascend/compiler/ascParse.c"
    break;

  case 139: /* methods: %empty  */
#line 2012 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 4144 "ascend/compiler/ascParse.c"
    break;

  case 140: /* $@2: %empty  */
#line 2016 "ascend/compiler/ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 4155 "ascend/compiler/ascParse.c"
    break;

  case 141: /* methods: METHODS_TOK $@2 proclist  */
#line 2023 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 4163 "ascend/compiler/ascParse.c"
    break;

  case 142: /* initial: %empty  */
#line 2030 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = EmptyStatementList();
	}
#line 4171 "ascend/compiler/ascParse.c"
    break;

  case 143: /* initial: INITIAL_TOK fstatements  */
#line 2034 "ascend/compiler/ascParse.y"
        {
	  AddContext((yyvsp[0].slptr),context_INITIAL);
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 4180 "ascend/compiler/ascParse.c"
    break;

  case 144: /* proclist: proclistf  */
#line 2042 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 4189 "ascend/compiler/ascParse.c"
    break;

  case 145: /* proclistf: %empty  */
#line 2049 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4197 "ascend/compiler/ascParse.c"
    break;

  case 146: /* proclistf: proclistf procedure  */
#line 2053 "ascend/compiler/ascParse.y"
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
#line 4223 "ascend/compiler/ascParse.c"
    break;

  case 147: /* procedure: procedure_id ';' fstatements end ';'  */
#line 2078 "ascend/compiler/ascParse.y"
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
#line 4238 "ascend/compiler/ascParse.c"
    break;

  case 148: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 2092 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 4247 "ascend/compiler/ascParse.c"
    break;

  case 149: /* fstatements: statements  */
#line 2101 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 4255 "ascend/compiler/ascParse.c"
    break;

  case 150: /* statements: %empty  */
#line 2108 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4263 "ascend/compiler/ascParse.c"
    break;

  case 151: /* statements: statements statement ';'  */
#line 2112 "ascend/compiler/ascParse.y"
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
	        ,NULL
	        ,ISCV_NONE
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
	              ,NULL
	              ,ISCV_NONE
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
	      DatasetAppendImplicitSetDecls((yyvsp[-2].listp),(yyvsp[-1].statptr));
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
	              ,NULL
	              ,ISCV_NONE
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
#line 4339 "ascend/compiler/ascParse.c"
    break;

  case 152: /* statements: statements complex_statement ';'  */
#line 2184 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
            gl_destroy((yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4351 "ascend/compiler/ascParse.c"
    break;

  case 153: /* statements: statements error ';'  */
#line 2192 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4360 "ascend/compiler/ascParse.c"
    break;

  case 198: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 2252 "ascend/compiler/ascParse.y"
        {
	  struct TypeDescription *tmptype;
	  tmptype = FindType((yyvsp[-2].id_ptr));
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
	      (yyval.statptr) = CreateISA((yyvsp[-4].lptr),(yyvsp[-2].id_ptr),g_typeargs,(yyvsp[-1].id_ptr),(yyvsp[0].eptr),g_decl_checkkind);
	    }
	  } else {
	    error_reporter_current_line(ASC_USER_ERROR,"IS_A uses the undefined type %s.", SCP((yyvsp[-2].id_ptr)));
	    DestroyVariableList((yyvsp[-4].lptr));
	    DestroySetList(g_typeargs);
	    DestroyExprList((yyvsp[0].eptr));
	    g_untrapped_error++;
	    (yyval.statptr) = NULL;
	  }
	  g_typeargs = NULL;
	  g_decl_checkkind = ISCV_NONE;

	}
#line 4394 "ascend/compiler/ascParse.c"
    break;

  case 199: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 2285 "ascend/compiler/ascParse.y"
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
	      (yyval.statptr) = CreateWILLBE((yyvsp[-4].lptr),(yyvsp[-2].id_ptr),g_typeargs,(yyvsp[-1].id_ptr),(yyvsp[0].eptr),g_decl_checkkind);
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
	  g_decl_checkkind = ISCV_NONE;
	}
#line 4425 "ascend/compiler/ascParse.c"
    break;

  case 200: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 2315 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 4433 "ascend/compiler/ascParse.c"
    break;

  case 201: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 2320 "ascend/compiler/ascParse.y"
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
#line 4483 "ascend/compiler/ascParse.c"
    break;

  case 202: /* optional_set_values: %empty  */
#line 2369 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4491 "ascend/compiler/ascParse.c"
    break;

  case 203: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 2373 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 4499 "ascend/compiler/ascParse.c"
    break;

  case 204: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 2380 "ascend/compiler/ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 4512 "ascend/compiler/ascParse.c"
    break;

  case 205: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 2392 "ascend/compiler/ascParse.y"
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
#line 4540 "ascend/compiler/ascParse.c"
    break;

  case 206: /* call_identifier: IDENTIFIER_TOK  */
#line 2419 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 4549 "ascend/compiler/ascParse.c"
    break;

  case 207: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2424 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 4558 "ascend/compiler/ascParse.c"
    break;

  case 208: /* type_identifier: IDENTIFIER_TOK  */
#line 2432 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 4567 "ascend/compiler/ascParse.c"
    break;

  case 209: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2437 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 4576 "ascend/compiler/ascParse.c"
    break;

  case 210: /* optional_method: %empty  */
#line 2445 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4584 "ascend/compiler/ascParse.c"
    break;

  case 211: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 2449 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4592 "ascend/compiler/ascParse.c"
    break;

  case 212: /* optional_of: %empty  */
#line 2456 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4600 "ascend/compiler/ascParse.c"
    break;

  case 213: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 2460 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4608 "ascend/compiler/ascParse.c"
    break;

  case 214: /* optional_with_value: %empty  */
#line 2467 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 4616 "ascend/compiler/ascParse.c"
    break;

  case 215: /* optional_with_value: WITH_VALUE_T expr  */
#line 2471 "ascend/compiler/ascParse.y"
        {
	  g_decl_checkkind = ISCV_WITH_VALUE;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4625 "ascend/compiler/ascParse.c"
    break;

  case 216: /* optional_with_value: DEFAULT_TOK expr  */
#line 2476 "ascend/compiler/ascParse.y"
        {
	  g_decl_checkkind = ISCV_DEFAULT;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4634 "ascend/compiler/ascParse.c"
    break;

  case 217: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 2484 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 4642 "ascend/compiler/ascParse.c"
    break;

  case 218: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 2491 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4650 "ascend/compiler/ascParse.c"
    break;

  case 219: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2495 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4658 "ascend/compiler/ascParse.c"
    break;

  case 220: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 2499 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4666 "ascend/compiler/ascParse.c"
    break;

  case 221: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2506 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4674 "ascend/compiler/ascParse.c"
    break;

  case 222: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 2510 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4682 "ascend/compiler/ascParse.c"
    break;

  case 223: /* der_statement: DER_TOK '(' fvarlist ')'  */
#line 2517 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 4692 "ascend/compiler/ascParse.c"
    break;

  case 224: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 2526 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 4702 "ascend/compiler/ascParse.c"
    break;

  case 225: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 2535 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 4710 "ascend/compiler/ascParse.c"
    break;

  case 226: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 2542 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 4718 "ascend/compiler/ascParse.c"
    break;

  case 227: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 2549 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 4726 "ascend/compiler/ascParse.c"
    break;

  case 228: /* assignment_statement: fvarref ASSIGN_TOK expr  */
#line 2556 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4734 "ascend/compiler/ascParse.c"
    break;

  case 229: /* assignment_statement: fvarref CASSIGN_TOK expr  */
#line 2560 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4742 "ascend/compiler/ascParse.c"
    break;

  case 230: /* reinit_statement: REINIT_TOK '(' fvarref ',' expr ')'  */
#line 2567 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateREINIT((yyvsp[-3].nptr),(yyvsp[-1].eptr));
	}
#line 4750 "ascend/compiler/ascParse.c"
    break;

  case 231: /* switchto_statement: SWITCH_TOK TO_TOK expr IF_TOK expr  */
#line 2574 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateSWITCHTO((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4758 "ascend/compiler/ascParse.c"
    break;

  case 232: /* relation_statement: relation  */
#line 2581 "ascend/compiler/ascParse.y"
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
#line 4775 "ascend/compiler/ascParse.c"
    break;

  case 233: /* relation_statement: fname ':' relation  */
#line 2594 "ascend/compiler/ascParse.y"
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
#line 4793 "ascend/compiler/ascParse.c"
    break;

  case 234: /* relation: expr  */
#line 2611 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 4807 "ascend/compiler/ascParse.c"
    break;

  case 235: /* relation: MINIMIZE_TOK expr  */
#line 2621 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4819 "ascend/compiler/ascParse.c"
    break;

  case 236: /* relation: MAXIMIZE_TOK expr  */
#line 2629 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4831 "ascend/compiler/ascParse.c"
    break;

  case 237: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 2640 "ascend/compiler/ascParse.y"
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
#line 4853 "ascend/compiler/ascParse.c"
    break;

  case 238: /* input_args: fvarlist ':' INPUT_TOK  */
#line 2661 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4861 "ascend/compiler/ascParse.c"
    break;

  case 239: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 2668 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4869 "ascend/compiler/ascParse.c"
    break;

  case 240: /* data_args: %empty  */
#line 2675 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 4877 "ascend/compiler/ascParse.c"
    break;

  case 241: /* data_args: ';' fname ':' DATA_TOK  */
#line 2679 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 4885 "ascend/compiler/ascParse.c"
    break;

  case 242: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 2717 "ascend/compiler/ascParse.y"
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
#line 4906 "ascend/compiler/ascParse.c"
    break;

  case 243: /* optional_direction: %empty  */
#line 2737 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 4914 "ascend/compiler/ascParse.c"
    break;

  case 244: /* optional_direction: INCREASING_TOK  */
#line 2741 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 4922 "ascend/compiler/ascParse.c"
    break;

  case 245: /* optional_direction: DECREASING_TOK  */
#line 2745 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 4930 "ascend/compiler/ascParse.c"
    break;

  case 246: /* forexprend: CREATE_TOK  */
#line 2752 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 4938 "ascend/compiler/ascParse.c"
    break;

  case 247: /* forexprend: EXPECT_TOK  */
#line 2756 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 4946 "ascend/compiler/ascParse.c"
    break;

  case 248: /* forexprend: CHECK_TOK  */
#line 2760 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 4954 "ascend/compiler/ascParse.c"
    break;

  case 249: /* forexprend: DO_TOK  */
#line 2764 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 4962 "ascend/compiler/ascParse.c"
    break;

  case 250: /* run_statement: RUN_TOK fname  */
#line 2771 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 4970 "ascend/compiler/ascParse.c"
    break;

  case 251: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 2775 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 4978 "ascend/compiler/ascParse.c"
    break;

  case 252: /* fix_statement: FIX_TOK method_fvarlist  */
#line 2782 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 4987 "ascend/compiler/ascParse.c"
    break;

  case 253: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 2790 "ascend/compiler/ascParse.y"
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
#line 5002 "ascend/compiler/ascParse.c"
    break;

  case 254: /* free_statement: FREE_TOK method_fvarlist  */
#line 2804 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 5010 "ascend/compiler/ascParse.c"
    break;

  case 255: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 2811 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 5019 "ascend/compiler/ascParse.c"
    break;

  case 256: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 2819 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 5028 "ascend/compiler/ascParse.c"
    break;

  case 257: /* solve_statement: SOLVE_TOK  */
#line 2827 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE(NULL);
	}
#line 5037 "ascend/compiler/ascParse.c"
    break;

  case 258: /* solve_statement: SOLVE_TOK fname  */
#line 2832 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSOLVE((yyvsp[0].nptr));
	}
#line 5045 "ascend/compiler/ascParse.c"
    break;

  case 259: /* $@3: %empty  */
#line 2839 "ascend/compiler/ascParse.y"
        {
		g_study_parse = StudyParseEmpty();
		g_study_run_method = NULL;
		g_study_now = 0;
		g_study_filename = NULL;
	}
#line 5056 "ascend/compiler/ascParse.c"
    break;

  case 260: /* study_statement: STUDY_TOK $@3 fvarlist study_vary_opt study_run_opt study_now_opt study_file_opt  */
#line 2846 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSTUDY((yyvsp[-4].lptr), g_study_parse.vary, g_study_parse.lower, g_study_parse.upper,
			g_study_parse.steps, g_study_parse.value, g_study_parse.mode, g_study_parse.dist,
			g_study_run_method, g_study_now, g_study_filename);
	}
#line 5066 "ascend/compiler/ascParse.c"
    break;

  case 261: /* study_vary_opt: %empty  */
#line 2855 "ascend/compiler/ascParse.y"
        {
	}
#line 5073 "ascend/compiler/ascParse.c"
    break;

  case 262: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEPS_TOK INTEGER_TOK study_distribution_opt  */
#line 2858 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-7].nptr);
		g_study_parse.lower = (yyvsp[-5].eptr);
		g_study_parse.upper = (yyvsp[-3].eptr);
		g_study_parse.steps = (yyvsp[-1].int_value);
		g_study_parse.mode = study_steps;
	}
#line 5085 "ascend/compiler/ascParse.c"
    break;

  case 263: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEP_TOK expr  */
#line 2866 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_step;
		g_study_parse.dist = study_dist_linear;
	}
#line 5098 "ascend/compiler/ascParse.c"
    break;

  case 264: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr RATIO_TOK expr  */
#line 2875 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_ratio;
		g_study_parse.dist = study_dist_log;
	}
#line 5111 "ascend/compiler/ascParse.c"
    break;

  case 265: /* study_distribution_opt: %empty  */
#line 2887 "ascend/compiler/ascParse.y"
        {
	}
#line 5118 "ascend/compiler/ascParse.c"
    break;

  case 266: /* study_distribution_opt: LINEAR_TOK  */
#line 2890 "ascend/compiler/ascParse.y"
        {
		g_study_parse.dist = study_dist_linear;
	}
#line 5126 "ascend/compiler/ascParse.c"
    break;

  case 267: /* study_distribution_opt: LOG_TOK  */
#line 2894 "ascend/compiler/ascParse.y"
        {
		g_study_parse.dist = study_dist_log;
	}
#line 5134 "ascend/compiler/ascParse.c"
    break;

  case 268: /* study_run_opt: %empty  */
#line 2901 "ascend/compiler/ascParse.y"
        {
	}
#line 5141 "ascend/compiler/ascParse.c"
    break;

  case 269: /* study_run_opt: RUN_TOK IDENTIFIER_TOK  */
#line 2904 "ascend/compiler/ascParse.y"
        {
		g_study_run_method = (yyvsp[0].id_ptr);
	}
#line 5149 "ascend/compiler/ascParse.c"
    break;

  case 270: /* study_file_opt: %empty  */
#line 2911 "ascend/compiler/ascParse.y"
        {
	}
#line 5156 "ascend/compiler/ascParse.c"
    break;

  case 271: /* study_file_opt: FILE_TOK DQUOTE_TOK  */
#line 2914 "ascend/compiler/ascParse.y"
        {
		g_study_filename = (yyvsp[0].dquote_ptr);
	}
#line 5164 "ascend/compiler/ascParse.c"
    break;

  case 272: /* study_now_opt: %empty  */
#line 2921 "ascend/compiler/ascParse.y"
        {
	}
#line 5171 "ascend/compiler/ascParse.c"
    break;

  case 273: /* study_now_opt: NOW_TOK  */
#line 2924 "ascend/compiler/ascParse.y"
        {
		g_study_now = 1;
	}
#line 5179 "ascend/compiler/ascParse.c"
    break;

  case 274: /* delete_statement: DELETE_TOK SYSTEM_TOK  */
#line 2931 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateDELETESYSTEM();
	}
#line 5187 "ascend/compiler/ascParse.c"
    break;

  case 275: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 2938 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 5199 "ascend/compiler/ascParse.c"
    break;

  case 276: /* call_statement: CALL_TOK call_identifier  */
#line 2949 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 5211 "ascend/compiler/ascParse.c"
    break;

  case 277: /* assert_statement: ASSERT_TOK expr  */
#line 2960 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 5219 "ascend/compiler/ascParse.c"
    break;

  case 278: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 2966 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 5230 "ascend/compiler/ascParse.c"
    break;

  case 279: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 2976 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 5241 "ascend/compiler/ascParse.c"
    break;

  case 280: /* optional_else: %empty  */
#line 2985 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 5249 "ascend/compiler/ascParse.c"
    break;

  case 281: /* optional_else: ELSE_TOK fstatements  */
#line 2989 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 5257 "ascend/compiler/ascParse.c"
    break;

  case 282: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 2996 "ascend/compiler/ascParse.y"
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
#line 5272 "ascend/compiler/ascParse.c"
    break;

  case 283: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 3007 "ascend/compiler/ascParse.y"
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
#line 5288 "ascend/compiler/ascParse.c"
    break;

  case 284: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 3019 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5299 "ascend/compiler/ascParse.c"
    break;

  case 285: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 3026 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5310 "ascend/compiler/ascParse.c"
    break;

  case 286: /* whenlist: whenlistf  */
#line 3036 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 5318 "ascend/compiler/ascParse.c"
    break;

  case 287: /* whenlistf: CASE_TOK set ':' fstatements  */
#line 3043 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5326 "ascend/compiler/ascParse.c"
    break;

  case 288: /* whenlistf: OTHERWISE_TOK ':' fstatements  */
#line 3047 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	}
#line 5334 "ascend/compiler/ascParse.c"
    break;

  case 289: /* whenlistf: whenlistf CASE_TOK set ':' fstatements  */
#line 3051 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	}
#line 5342 "ascend/compiler/ascParse.c"
    break;

  case 290: /* whenlistf: whenlistf OTHERWISE_TOK ':' fstatements  */
#line 3055 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	}
#line 5350 "ascend/compiler/ascParse.c"
    break;

  case 291: /* flow_statement: BREAK_TOK  */
#line 3062 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 5358 "ascend/compiler/ascParse.c"
    break;

  case 292: /* flow_statement: CONTINUE_TOK  */
#line 3066 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 5366 "ascend/compiler/ascParse.c"
    break;

  case 293: /* flow_statement: FALLTHRU_TOK  */
#line 3070 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 5374 "ascend/compiler/ascParse.c"
    break;

  case 294: /* flow_statement: RETURN_TOK  */
#line 3074 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 5382 "ascend/compiler/ascParse.c"
    break;

  case 295: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 3078 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 5390 "ascend/compiler/ascParse.c"
    break;

  case 296: /* use_statement: USE_TOK fname  */
#line 3085 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 5398 "ascend/compiler/ascParse.c"
    break;

  case 297: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 3092 "ascend/compiler/ascParse.y"
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
#line 5413 "ascend/compiler/ascParse.c"
    break;

  case 298: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 3103 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 5424 "ascend/compiler/ascParse.c"
    break;

  case 299: /* selectlist: selectlistf  */
#line 3113 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 5432 "ascend/compiler/ascParse.c"
    break;

  case 300: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 3120 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5440 "ascend/compiler/ascParse.c"
    break;

  case 301: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 3124 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 5448 "ascend/compiler/ascParse.c"
    break;

  case 302: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 3128 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 5456 "ascend/compiler/ascParse.c"
    break;

  case 303: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 3132 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 5464 "ascend/compiler/ascParse.c"
    break;

  case 304: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 3139 "ascend/compiler/ascParse.y"
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
#line 5479 "ascend/compiler/ascParse.c"
    break;

  case 305: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 3150 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 5490 "ascend/compiler/ascParse.c"
    break;

  case 306: /* switchlist: switchlistf  */
#line 3160 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 5498 "ascend/compiler/ascParse.c"
    break;

  case 307: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 3167 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5506 "ascend/compiler/ascParse.c"
    break;

  case 308: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 3171 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 5514 "ascend/compiler/ascParse.c"
    break;

  case 309: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 3175 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 5522 "ascend/compiler/ascParse.c"
    break;

  case 310: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 3179 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 5530 "ascend/compiler/ascParse.c"
    break;

  case 311: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 3186 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 5541 "ascend/compiler/ascParse.c"
    break;

  case 312: /* notes_statement: NOTES_TOK notes_body end  */
#line 3196 "ascend/compiler/ascParse.y"
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
#line 5579 "ascend/compiler/ascParse.c"
    break;

  case 313: /* notes_body: SYMBOL_TOK noteslist  */
#line 3233 "ascend/compiler/ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 5592 "ascend/compiler/ascParse.c"
    break;

  case 314: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 3242 "ascend/compiler/ascParse.y"
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
#line 5608 "ascend/compiler/ascParse.c"
    break;

  case 315: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 3257 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 5617 "ascend/compiler/ascParse.c"
    break;

  case 316: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 3262 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 5627 "ascend/compiler/ascParse.c"
    break;

  case 317: /* fvarlist: varlist  */
#line 3271 "ascend/compiler/ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5639 "ascend/compiler/ascParse.c"
    break;

  case 318: /* varlist: fname  */
#line 3282 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5647 "ascend/compiler/ascParse.c"
    break;

  case 319: /* varlist: varlist ',' fname  */
#line 3286 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5656 "ascend/compiler/ascParse.c"
    break;

  case 320: /* varlist: varlist fname  */
#line 3291 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 5670 "ascend/compiler/ascParse.c"
    break;

  case 321: /* method_fvarlist: method_varlist  */
#line 3304 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5678 "ascend/compiler/ascParse.c"
    break;

  case 322: /* method_varlist: fvarref  */
#line 3311 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5686 "ascend/compiler/ascParse.c"
    break;

  case 323: /* method_varlist: method_varlist ',' fvarref  */
#line 3315 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5695 "ascend/compiler/ascParse.c"
    break;

  case 324: /* method_varlist: method_varlist fvarref  */
#line 3320 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  g_untrapped_error++;
	}
#line 5706 "ascend/compiler/ascParse.c"
    break;

  case 325: /* fvarref: fname  */
#line 3330 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[0].nptr);
	}
#line 5714 "ascend/compiler/ascParse.c"
    break;

  case 326: /* fvarref: DER_TOK '(' fname ')'  */
#line 3334 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateDerivativeRefName((yyvsp[-1].nptr));
	}
#line 5722 "ascend/compiler/ascParse.c"
    break;

  case 327: /* fname: name optional_notes  */
#line 3341 "ascend/compiler/ascParse.y"
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
#line 5743 "ascend/compiler/ascParse.c"
    break;

  case 328: /* name: IDENTIFIER_TOK  */
#line 3361 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 5751 "ascend/compiler/ascParse.c"
    break;

  case 329: /* name: name '.' IDENTIFIER_TOK  */
#line 3365 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5760 "ascend/compiler/ascParse.c"
    break;

  case 330: /* name: name '.' DER_TOK  */
#line 3370 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName(AddSymbol("der"));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5769 "ascend/compiler/ascParse.c"
    break;

  case 331: /* name: name '[' set ']'  */
#line 3375 "ascend/compiler/ascParse.y"
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
#line 5785 "ascend/compiler/ascParse.c"
    break;

  case 332: /* end: END_TOK CONDITIONAL_TOK  */
#line 3390 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 5794 "ascend/compiler/ascParse.c"
    break;

  case 333: /* end: END_TOK FOR_TOK  */
#line 3395 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 5803 "ascend/compiler/ascParse.c"
    break;

  case 334: /* end: END_TOK IF_TOK  */
#line 3400 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 5812 "ascend/compiler/ascParse.c"
    break;

  case 335: /* end: END_TOK INTERACTIVE_TOK  */
#line 3405 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 5821 "ascend/compiler/ascParse.c"
    break;

  case 336: /* end: END_TOK METHODS_TOK  */
#line 3410 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 5830 "ascend/compiler/ascParse.c"
    break;

  case 337: /* end: END_TOK NOTES_TOK  */
#line 3415 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 5839 "ascend/compiler/ascParse.c"
    break;

  case 338: /* end: END_TOK SELECT_TOK  */
#line 3420 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 5848 "ascend/compiler/ascParse.c"
    break;

  case 339: /* end: END_TOK SWITCH_TOK  */
#line 3425 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 5857 "ascend/compiler/ascParse.c"
    break;

  case 340: /* end: END_TOK UNITS_TOK  */
#line 3430 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 5866 "ascend/compiler/ascParse.c"
    break;

  case 341: /* end: END_TOK GLOBAL_TOK  */
#line 3435 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 5875 "ascend/compiler/ascParse.c"
    break;

  case 342: /* end: END_TOK WHEN_TOK  */
#line 3440 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 5884 "ascend/compiler/ascParse.c"
    break;

  case 343: /* end: END_TOK WHILE_TOK  */
#line 3445 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 5893 "ascend/compiler/ascParse.c"
    break;

  case 344: /* end: END_TOK IDENTIFIER_TOK  */
#line 3450 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 5902 "ascend/compiler/ascParse.c"
    break;

  case 345: /* end: END_TOK  */
#line 3455 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 5911 "ascend/compiler/ascParse.c"
    break;

  case 346: /* optional_bracedtext: %empty  */
#line 3463 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 5919 "ascend/compiler/ascParse.c"
    break;

  case 347: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 3467 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 5927 "ascend/compiler/ascParse.c"
    break;

  case 348: /* optional_notes: %empty  */
#line 3474 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 5935 "ascend/compiler/ascParse.c"
    break;

  case 349: /* optional_notes: DQUOTE_TOK  */
#line 3478 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 5943 "ascend/compiler/ascParse.c"
    break;

  case 350: /* set: setexprlist  */
#line 3485 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 5951 "ascend/compiler/ascParse.c"
    break;

  case 351: /* set: %empty  */
#line 3489 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 5959 "ascend/compiler/ascParse.c"
    break;

  case 352: /* setexprlist: expr  */
#line 3496 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 5967 "ascend/compiler/ascParse.c"
    break;

  case 353: /* setexprlist: expr DOTDOT_TOK expr  */
#line 3500 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5975 "ascend/compiler/ascParse.c"
    break;

  case 354: /* setexprlist: setexprlist ',' expr  */
#line 3504 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 5984 "ascend/compiler/ascParse.c"
    break;

  case 355: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 3509 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 5993 "ascend/compiler/ascParse.c"
    break;

  case 356: /* number: INTEGER_TOK  */
#line 3517 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	  g_number_units = NULL;
	}
#line 6004 "ascend/compiler/ascParse.c"
    break;

  case 357: /* number: realnumber  */
#line 3524 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 6014 "ascend/compiler/ascParse.c"
    break;

  case 358: /* realnumber: REAL_TOK opunits  */
#line 3533 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	  g_number_units = g_parsed_units;
	}
#line 6023 "ascend/compiler/ascParse.c"
    break;

  case 359: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 3538 "ascend/compiler/ascParse.y"
        {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((yyvsp[0].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (yyval.real_value) = (double)(yyvsp[-1].int_value)*UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	    g_parsed_units = UnitsDescription(g_units_ptr);
	  } else {
	    char **errv;
	    (yyval.real_value) = (double)(yyvsp[-1].int_value);
	    g_dim_ptr = WildDimension();
	    g_parsed_units = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'", (yyvsp[0].braced_ptr));
	    errv = UnitsExplainError((yyvsp[0].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	  g_number_units = g_parsed_units;
	}
#line 6048 "ascend/compiler/ascParse.c"
    break;

  case 360: /* opunits: %empty  */
#line 3562 "ascend/compiler/ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  g_parsed_units = NULL;
	  (yyval.real_value) = 1.0;
	}
#line 6058 "ascend/compiler/ascParse.c"
    break;

  case 361: /* opunits: BRACEDTEXT_TOK  */
#line 3568 "ascend/compiler/ascParse.y"
        {
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits((yyvsp[0].braced_ptr),&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    (yyval.real_value) = UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	    g_parsed_units = UnitsDescription(g_units_ptr);
	  } else {
	    char **errv;
	    (yyval.real_value) = 1.0;
	    g_dim_ptr = WildDimension();
	    g_parsed_units = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'",(yyvsp[0].braced_ptr));
	    errv = UnitsExplainError((yyvsp[0].braced_ptr),error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
#line 6082 "ascend/compiler/ascParse.c"
    break;

  case 362: /* dims: DIMENSION_TOK dimensions  */
#line 3591 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 6090 "ascend/compiler/ascParse.c"
    break;

  case 363: /* dims: DIMENSIONLESS_TOK  */
#line 3595 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 6098 "ascend/compiler/ascParse.c"
    break;

  case 364: /* dims: %empty  */
#line 3599 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 6106 "ascend/compiler/ascParse.c"
    break;

  case 365: /* dimensions: '*'  */
#line 3606 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 6114 "ascend/compiler/ascParse.c"
    break;

  case 366: /* dimensions: dimexpr  */
#line 3610 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 6122 "ascend/compiler/ascParse.c"
    break;

  case 367: /* dimexpr: IDENTIFIER_TOK  */
#line 3617 "ascend/compiler/ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 6130 "ascend/compiler/ascParse.c"
    break;

  case 368: /* dimexpr: INTEGER_TOK  */
#line 3621 "ascend/compiler/ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 6138 "ascend/compiler/ascParse.c"
    break;

  case 369: /* dimexpr: dimexpr '/' dimexpr  */
#line 3625 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6146 "ascend/compiler/ascParse.c"
    break;

  case 370: /* dimexpr: dimexpr '*' dimexpr  */
#line 3629 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6154 "ascend/compiler/ascParse.c"
    break;

  case 371: /* dimexpr: dimexpr '^' fraction  */
#line 3633 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 6162 "ascend/compiler/ascParse.c"
    break;

  case 372: /* dimexpr: '(' dimexpr ')'  */
#line 3637 "ascend/compiler/ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 6170 "ascend/compiler/ascParse.c"
    break;

  case 373: /* fraction: optional_sign fractail  */
#line 3644 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 6178 "ascend/compiler/ascParse.c"
    break;

  case 374: /* fractail: INTEGER_TOK  */
#line 3651 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 6186 "ascend/compiler/ascParse.c"
    break;

  case 375: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 3655 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 6194 "ascend/compiler/ascParse.c"
    break;

  case 376: /* optional_sign: %empty  */
#line 3662 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6202 "ascend/compiler/ascParse.c"
    break;

  case 377: /* optional_sign: '+'  */
#line 3666 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6210 "ascend/compiler/ascParse.c"
    break;

  case 378: /* optional_sign: '-'  */
#line 3670 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 6218 "ascend/compiler/ascParse.c"
    break;

  case 379: /* expr: INTEGER_TOK  */
#line 3677 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 6226 "ascend/compiler/ascParse.c"
    break;

  case 380: /* expr: MAXINTEGER_TOK  */
#line 3681 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 6234 "ascend/compiler/ascParse.c"
    break;

  case 381: /* expr: realnumber  */
#line 3685 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 6242 "ascend/compiler/ascParse.c"
    break;

  case 382: /* expr: MAXREAL_TOK  */
#line 3689 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 6250 "ascend/compiler/ascParse.c"
    break;

  case 383: /* expr: TRUE_TOK  */
#line 3693 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 6258 "ascend/compiler/ascParse.c"
    break;

  case 384: /* expr: FALSE_TOK  */
#line 3697 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 6266 "ascend/compiler/ascParse.c"
    break;

  case 385: /* expr: ANY_TOK  */
#line 3701 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 6274 "ascend/compiler/ascParse.c"
    break;

  case 386: /* expr: SYMBOL_TOK  */
#line 3705 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 6282 "ascend/compiler/ascParse.c"
    break;

  case 387: /* expr: fname  */
#line 3709 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 6290 "ascend/compiler/ascParse.c"
    break;

  case 388: /* expr: DER_TOK '(' fname ')'  */
#line 3713 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateDiffExpr((yyvsp[-1].nptr));
	}
#line 6298 "ascend/compiler/ascParse.c"
    break;

  case 389: /* expr: '[' set ']'  */
#line 3717 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 6306 "ascend/compiler/ascParse.c"
    break;

  case 390: /* expr: expr '+' expr  */
#line 3721 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6315 "ascend/compiler/ascParse.c"
    break;

  case 391: /* expr: expr '-' expr  */
#line 3726 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6324 "ascend/compiler/ascParse.c"
    break;

  case 392: /* expr: expr '*' expr  */
#line 3731 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6333 "ascend/compiler/ascParse.c"
    break;

  case 393: /* expr: expr '/' expr  */
#line 3736 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6342 "ascend/compiler/ascParse.c"
    break;

  case 394: /* expr: expr '^' expr  */
#line 3741 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6351 "ascend/compiler/ascParse.c"
    break;

  case 395: /* expr: expr AND_TOK expr  */
#line 3746 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6360 "ascend/compiler/ascParse.c"
    break;

  case 396: /* expr: expr OR_TOK expr  */
#line 3751 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6369 "ascend/compiler/ascParse.c"
    break;

  case 397: /* expr: NOT_TOK expr  */
#line 3756 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 6377 "ascend/compiler/ascParse.c"
    break;

  case 398: /* expr: expr relop expr  */
#line 3760 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6386 "ascend/compiler/ascParse.c"
    break;

  case 399: /* expr: expr logrelop expr  */
#line 3765 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6395 "ascend/compiler/ascParse.c"
    break;

  case 400: /* expr: expr IN_TOK expr  */
#line 3770 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6404 "ascend/compiler/ascParse.c"
    break;

  case 401: /* expr: expr '|' expr  */
#line 3775 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6413 "ascend/compiler/ascParse.c"
    break;

  case 402: /* expr: expr SUCHTHAT_TOK expr  */
#line 3780 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6422 "ascend/compiler/ascParse.c"
    break;

  case 403: /* expr: '+' expr  */
#line 3785 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 6430 "ascend/compiler/ascParse.c"
    break;

  case 404: /* expr: '-' expr  */
#line 3789 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 6438 "ascend/compiler/ascParse.c"
    break;

  case 405: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 3793 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 6446 "ascend/compiler/ascParse.c"
    break;

  case 406: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 3797 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 6454 "ascend/compiler/ascParse.c"
    break;

  case 407: /* expr: SUM_TOK '(' set ')'  */
#line 3801 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 6465 "ascend/compiler/ascParse.c"
    break;

  case 408: /* expr: SUM_TOK '[' set ']'  */
#line 3808 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 6473 "ascend/compiler/ascParse.c"
    break;

  case 409: /* expr: PROD_TOK '(' set ')'  */
#line 3812 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 6484 "ascend/compiler/ascParse.c"
    break;

  case 410: /* expr: PROD_TOK '[' set ']'  */
#line 3819 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 6492 "ascend/compiler/ascParse.c"
    break;

  case 411: /* expr: UNION_TOK '(' set ')'  */
#line 3823 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 6503 "ascend/compiler/ascParse.c"
    break;

  case 412: /* expr: UNION_TOK '[' set ']'  */
#line 3830 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 6511 "ascend/compiler/ascParse.c"
    break;

  case 413: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 3834 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 6522 "ascend/compiler/ascParse.c"
    break;

  case 414: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 3841 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 6530 "ascend/compiler/ascParse.c"
    break;

  case 415: /* expr: CARD_TOK '(' set ')'  */
#line 3845 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 6541 "ascend/compiler/ascParse.c"
    break;

  case 416: /* expr: CARD_TOK '[' set ']'  */
#line 3852 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 6549 "ascend/compiler/ascParse.c"
    break;

  case 417: /* expr: CHOICE_TOK '(' set ')'  */
#line 3856 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 6560 "ascend/compiler/ascParse.c"
    break;

  case 418: /* expr: CHOICE_TOK '[' set ']'  */
#line 3863 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 6568 "ascend/compiler/ascParse.c"
    break;

  case 419: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 3867 "ascend/compiler/ascParse.y"
        {
	  CONST struct Func *fptr;
	  if (strcmp(SCP((yyvsp[-3].id_ptr)),"pre")==0) {
	    if ((yyvsp[-1].eptr) != NULL && NextExpr((yyvsp[-1].eptr)) == NULL && ExprType((yyvsp[-1].eptr)) == e_var) {
	      (yyval.eptr) = CreatePreExpr(CopyName(ExprName((yyvsp[-1].eptr))));
	      DestroyExprList((yyvsp[-1].eptr));
	    } else {
	      (yyval.eptr) = NULL;
	      if((yyvsp[-1].eptr) != NULL) DestroyExprList((yyvsp[-1].eptr));
	      error_reporter_current_line(ASC_USER_ERROR,
	        "pre(...) currently requires a single variable reference argument.");
	      g_untrapped_error++;
	    }
	  } else if ((fptr = LookupFunc(SCP((yyvsp[-3].id_ptr))))!=NULL) {
	    (yyval.eptr) = JoinExprLists((yyvsp[-1].eptr),CreateFuncExpr(fptr));
	  } else {
	    (yyval.eptr) = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Function '%s' is not defined.",SCP((yyvsp[-3].id_ptr)));
	    g_untrapped_error++;
	  }
	}
#line 6594 "ascend/compiler/ascParse.c"
    break;

  case 420: /* expr: '(' expr ')'  */
#line 3889 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 6602 "ascend/compiler/ascParse.c"
    break;

  case 421: /* relop: '='  */
#line 3896 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 6610 "ascend/compiler/ascParse.c"
    break;

  case 422: /* relop: '<'  */
#line 3900 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 6618 "ascend/compiler/ascParse.c"
    break;

  case 423: /* relop: '>'  */
#line 3904 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 6626 "ascend/compiler/ascParse.c"
    break;

  case 424: /* relop: LEQ_TOK  */
#line 3908 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 6634 "ascend/compiler/ascParse.c"
    break;

  case 425: /* relop: GEQ_TOK  */
#line 3912 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 6642 "ascend/compiler/ascParse.c"
    break;

  case 426: /* relop: NEQ_TOK  */
#line 3916 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 6650 "ascend/compiler/ascParse.c"
    break;

  case 427: /* logrelop: BEQ_TOK  */
#line 3923 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 6658 "ascend/compiler/ascParse.c"
    break;

  case 428: /* logrelop: BNE_TOK  */
#line 3927 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 6666 "ascend/compiler/ascParse.c"
    break;


#line 6670 "ascend/compiler/ascParse.c"

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

#line 3931 "ascend/compiler/ascParse.y"

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
  case LADDER_TOK:
    return "LADDER";
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
