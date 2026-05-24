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
  YYSYMBOL_APPLIES_TOK = 7,                /* APPLIES_TOK  */
  YYSYMBOL_AREALIKE_TOK = 8,               /* AREALIKE_TOK  */
  YYSYMBOL_ARETHESAME_TOK = 9,             /* ARETHESAME_TOK  */
  YYSYMBOL_ARRAY_TOK = 10,                 /* ARRAY_TOK  */
  YYSYMBOL_ASSERT_TOK = 11,                /* ASSERT_TOK  */
  YYSYMBOL_ATOM_TOK = 12,                  /* ATOM_TOK  */
  YYSYMBOL_BEQ_TOK = 13,                   /* BEQ_TOK  */
  YYSYMBOL_BNE_TOK = 14,                   /* BNE_TOK  */
  YYSYMBOL_BREAK_TOK = 15,                 /* BREAK_TOK  */
  YYSYMBOL_CALL_TOK = 16,                  /* CALL_TOK  */
  YYSYMBOL_CARD_TOK = 17,                  /* CARD_TOK  */
  YYSYMBOL_CASE_TOK = 18,                  /* CASE_TOK  */
  YYSYMBOL_CHOICE_TOK = 19,                /* CHOICE_TOK  */
  YYSYMBOL_CHECK_TOK = 20,                 /* CHECK_TOK  */
  YYSYMBOL_CONDITIONAL_TOK = 21,           /* CONDITIONAL_TOK  */
  YYSYMBOL_CONSTANT_TOK = 22,              /* CONSTANT_TOK  */
  YYSYMBOL_CONTINUE_TOK = 23,              /* CONTINUE_TOK  */
  YYSYMBOL_CREATE_TOK = 24,                /* CREATE_TOK  */
  YYSYMBOL_DATA_TOK = 25,                  /* DATA_TOK  */
  YYSYMBOL_DECREASING_TOK = 26,            /* DECREASING_TOK  */
  YYSYMBOL_DEFAULT_TOK = 27,               /* DEFAULT_TOK  */
  YYSYMBOL_DEFINITION_TOK = 28,            /* DEFINITION_TOK  */
  YYSYMBOL_DELETE_TOK = 29,                /* DELETE_TOK  */
  YYSYMBOL_DERIV_TOK = 30,                 /* DERIV_TOK  */
  YYSYMBOL_DERLINK_TOK = 31,               /* DERLINK_TOK  */
  YYSYMBOL_DIMENSION_TOK = 32,             /* DIMENSION_TOK  */
  YYSYMBOL_DIMENSIONLESS_TOK = 33,         /* DIMENSIONLESS_TOK  */
  YYSYMBOL_DO_TOK = 34,                    /* DO_TOK  */
  YYSYMBOL_ELSE_TOK = 35,                  /* ELSE_TOK  */
  YYSYMBOL_END_TOK = 36,                   /* END_TOK  */
  YYSYMBOL_EXPECT_TOK = 37,                /* EXPECT_TOK  */
  YYSYMBOL_EXTERNAL_TOK = 38,              /* EXTERNAL_TOK  */
  YYSYMBOL_FALSE_TOK = 39,                 /* FALSE_TOK  */
  YYSYMBOL_FALLTHRU_TOK = 40,              /* FALLTHRU_TOK  */
  YYSYMBOL_FIX_TOK = 41,                   /* FIX_TOK  */
  YYSYMBOL_FOR_TOK = 42,                   /* FOR_TOK  */
  YYSYMBOL_FREE_TOK = 43,                  /* FREE_TOK  */
  YYSYMBOL_FROM_TOK = 44,                  /* FROM_TOK  */
  YYSYMBOL_FILE_TOK = 45,                  /* FILE_TOK  */
  YYSYMBOL_GLOBAL_TOK = 46,                /* GLOBAL_TOK  */
  YYSYMBOL_IF_TOK = 47,                    /* IF_TOK  */
  YYSYMBOL_IGNORE_TOK = 48,                /* IGNORE_TOK  */
  YYSYMBOL_IMPORT_TOK = 49,                /* IMPORT_TOK  */
  YYSYMBOL_IN_TOK = 50,                    /* IN_TOK  */
  YYSYMBOL_INITIAL_TOK = 51,               /* INITIAL_TOK  */
  YYSYMBOL_INPUT_TOK = 52,                 /* INPUT_TOK  */
  YYSYMBOL_INCREASING_TOK = 53,            /* INCREASING_TOK  */
  YYSYMBOL_INTERACTIVE_TOK = 54,           /* INTERACTIVE_TOK  */
  YYSYMBOL_INDEPENDENT_TOK = 55,           /* INDEPENDENT_TOK  */
  YYSYMBOL_INTEGRATE_TOK = 56,             /* INTEGRATE_TOK  */
  YYSYMBOL_INTEGRATOR_TOK = 57,            /* INTEGRATOR_TOK  */
  YYSYMBOL_INTERSECTION_TOK = 58,          /* INTERSECTION_TOK  */
  YYSYMBOL_ISA_TOK = 59,                   /* ISA_TOK  */
  YYSYMBOL__IS_T = 60,                     /* _IS_T  */
  YYSYMBOL_ISREFINEDTO_TOK = 61,           /* ISREFINEDTO_TOK  */
  YYSYMBOL_AS_TOK = 62,                    /* AS_TOK  */
  YYSYMBOL_LINEAR_TOK = 63,                /* LINEAR_TOK  */
  YYSYMBOL_LOG_TOK = 64,                   /* LOG_TOK  */
  YYSYMBOL_NOW_TOK = 65,                   /* NOW_TOK  */
  YYSYMBOL_LINK_TOK = 66,                  /* LINK_TOK  */
  YYSYMBOL_MAXIMIZE_TOK = 67,              /* MAXIMIZE_TOK  */
  YYSYMBOL_MAXINTEGER_TOK = 68,            /* MAXINTEGER_TOK  */
  YYSYMBOL_MAXREAL_TOK = 69,               /* MAXREAL_TOK  */
  YYSYMBOL_METHODS_TOK = 70,               /* METHODS_TOK  */
  YYSYMBOL_METHOD_TOK = 71,                /* METHOD_TOK  */
  YYSYMBOL_MINIMIZE_TOK = 72,              /* MINIMIZE_TOK  */
  YYSYMBOL_MODEL_TOK = 73,                 /* MODEL_TOK  */
  YYSYMBOL_NOT_TOK = 74,                   /* NOT_TOK  */
  YYSYMBOL_NOTES_TOK = 75,                 /* NOTES_TOK  */
  YYSYMBOL_OBSERVE_TOK = 76,               /* OBSERVE_TOK  */
  YYSYMBOL_OF_TOK = 77,                    /* OF_TOK  */
  YYSYMBOL_OPTION_TOK = 78,                /* OPTION_TOK  */
  YYSYMBOL_OR_TOK = 79,                    /* OR_TOK  */
  YYSYMBOL_OTHERWISE_TOK = 80,             /* OTHERWISE_TOK  */
  YYSYMBOL_OUTPUT_TOK = 81,                /* OUTPUT_TOK  */
  YYSYMBOL_PROD_TOK = 82,                  /* PROD_TOK  */
  YYSYMBOL_PROVIDE_TOK = 83,               /* PROVIDE_TOK  */
  YYSYMBOL_RATIO_TOK = 84,                 /* RATIO_TOK  */
  YYSYMBOL_REFINES_TOK = 85,               /* REFINES_TOK  */
  YYSYMBOL_REPLACE_TOK = 86,               /* REPLACE_TOK  */
  YYSYMBOL_REQUIRE_TOK = 87,               /* REQUIRE_TOK  */
  YYSYMBOL_RETURN_TOK = 88,                /* RETURN_TOK  */
  YYSYMBOL_RUN_TOK = 89,                   /* RUN_TOK  */
  YYSYMBOL_REINIT_TOK = 90,                /* REINIT_TOK  */
  YYSYMBOL_SATISFIED_TOK = 91,             /* SATISFIED_TOK  */
  YYSYMBOL_SELECT_TOK = 92,                /* SELECT_TOK  */
  YYSYMBOL_SIZE_TOK = 93,                  /* SIZE_TOK  */
  YYSYMBOL_SOLVE_TOK = 94,                 /* SOLVE_TOK  */
  YYSYMBOL_SOLVER_TOK = 95,                /* SOLVER_TOK  */
  YYSYMBOL_STOP_TOK = 96,                  /* STOP_TOK  */
  YYSYMBOL_SUCHTHAT_TOK = 97,              /* SUCHTHAT_TOK  */
  YYSYMBOL_SUM_TOK = 98,                   /* SUM_TOK  */
  YYSYMBOL_SWITCH_TOK = 99,                /* SWITCH_TOK  */
  YYSYMBOL_SYSTEM_TOK = 100,               /* SYSTEM_TOK  */
  YYSYMBOL_STEP_TOK = 101,                 /* STEP_TOK  */
  YYSYMBOL_STEPS_TOK = 102,                /* STEPS_TOK  */
  YYSYMBOL_STUDY_TOK = 103,                /* STUDY_TOK  */
  YYSYMBOL_TABLE_TOK = 104,                /* TABLE_TOK  */
  YYSYMBOL_VALUES_TOK = 105,               /* VALUES_TOK  */
  YYSYMBOL_DATASET_TOK = 106,              /* DATASET_TOK  */
  YYSYMBOL_POSITIONAL_TOK = 107,           /* POSITIONAL_TOK  */
  YYSYMBOL_INDEX_TOK = 108,                /* INDEX_TOK  */
  YYSYMBOL_COLUMN_TOK = 109,               /* COLUMN_TOK  */
  YYSYMBOL_EOL_TOK = 110,                  /* EOL_TOK  */
  YYSYMBOL_THEN_TOK = 111,                 /* THEN_TOK  */
  YYSYMBOL_TO_TOK = 112,                   /* TO_TOK  */
  YYSYMBOL_TRUE_TOK = 113,                 /* TRUE_TOK  */
  YYSYMBOL_UNION_TOK = 114,                /* UNION_TOK  */
  YYSYMBOL_UNITS_TOK = 115,                /* UNITS_TOK  */
  YYSYMBOL_LADDER_TOK = 116,               /* LADDER_TOK  */
  YYSYMBOL_UNIVERSAL_TOK = 117,            /* UNIVERSAL_TOK  */
  YYSYMBOL_UNLINK_TOK = 118,               /* UNLINK_TOK  */
  YYSYMBOL_VARY_TOK = 119,                 /* VARY_TOK  */
  YYSYMBOL_WHEN_TOK = 120,                 /* WHEN_TOK  */
  YYSYMBOL_WHERE_TOK = 121,                /* WHERE_TOK  */
  YYSYMBOL_WHILE_TOK = 122,                /* WHILE_TOK  */
  YYSYMBOL_WILLBE_TOK = 123,               /* WILLBE_TOK  */
  YYSYMBOL_WILLBETHESAME_TOK = 124,        /* WILLBETHESAME_TOK  */
  YYSYMBOL_WILLNOTBETHESAME_TOK = 125,     /* WILLNOTBETHESAME_TOK  */
  YYSYMBOL_ASSIGN_TOK = 126,               /* ASSIGN_TOK  */
  YYSYMBOL_CASSIGN_TOK = 127,              /* CASSIGN_TOK  */
  YYSYMBOL_DBLCOLON_TOK = 128,             /* DBLCOLON_TOK  */
  YYSYMBOL_USE_TOK = 129,                  /* USE_TOK  */
  YYSYMBOL_LEQ_TOK = 130,                  /* LEQ_TOK  */
  YYSYMBOL_GEQ_TOK = 131,                  /* GEQ_TOK  */
  YYSYMBOL_NEQ_TOK = 132,                  /* NEQ_TOK  */
  YYSYMBOL_DOTDOT_TOK = 133,               /* DOTDOT_TOK  */
  YYSYMBOL_WITH_TOK = 134,                 /* WITH_TOK  */
  YYSYMBOL_VALUE_TOK = 135,                /* VALUE_TOK  */
  YYSYMBOL_WITH_VALUE_T = 136,             /* WITH_VALUE_T  */
  YYSYMBOL_REAL_TOK = 137,                 /* REAL_TOK  */
  YYSYMBOL_INTEGER_TOK = 138,              /* INTEGER_TOK  */
  YYSYMBOL_IDENTIFIER_TOK = 139,           /* IDENTIFIER_TOK  */
  YYSYMBOL_BRACEDTEXT_TOK = 140,           /* BRACEDTEXT_TOK  */
  YYSYMBOL_SYMBOL_TOK = 141,               /* SYMBOL_TOK  */
  YYSYMBOL_DQUOTE_TOK = 142,               /* DQUOTE_TOK  */
  YYSYMBOL_143_ = 143,                     /* ','  */
  YYSYMBOL_144_ = 144,                     /* '|'  */
  YYSYMBOL_145_ = 145,                     /* '<'  */
  YYSYMBOL_146_ = 146,                     /* '='  */
  YYSYMBOL_147_ = 147,                     /* '>'  */
  YYSYMBOL_148_ = 148,                     /* '+'  */
  YYSYMBOL_149_ = 149,                     /* '-'  */
  YYSYMBOL_150_ = 150,                     /* '/'  */
  YYSYMBOL_151_ = 151,                     /* '*'  */
  YYSYMBOL_UMINUS_TOK = 152,               /* UMINUS_TOK  */
  YYSYMBOL_UPLUS_TOK = 153,                /* UPLUS_TOK  */
  YYSYMBOL_154_ = 154,                     /* '^'  */
  YYSYMBOL_155_ = 155,                     /* ';'  */
  YYSYMBOL_156_ = 156,                     /* '('  */
  YYSYMBOL_157_ = 157,                     /* ')'  */
  YYSYMBOL_158_ = 158,                     /* ':'  */
  YYSYMBOL_159_ = 159,                     /* '['  */
  YYSYMBOL_160_ = 160,                     /* ']'  */
  YYSYMBOL_161_ = 161,                     /* '.'  */
  YYSYMBOL_YYACCEPT = 162,                 /* $accept  */
  YYSYMBOL_definitions = 163,              /* definitions  */
  YYSYMBOL_definition = 164,               /* definition  */
  YYSYMBOL_global_def = 165,               /* global_def  */
  YYSYMBOL_require_file = 166,             /* require_file  */
  YYSYMBOL_provide_module = 167,           /* provide_module  */
  YYSYMBOL_import = 168,                   /* import  */
  YYSYMBOL_add_notes_def = 169,            /* add_notes_def  */
  YYSYMBOL_add_notes_head = 170,           /* add_notes_head  */
  YYSYMBOL_add_method_def = 171,           /* add_method_def  */
  YYSYMBOL_add_method_head = 172,          /* add_method_head  */
  YYSYMBOL_replace_method_def = 173,       /* replace_method_def  */
  YYSYMBOL_replace_method_head = 174,      /* replace_method_head  */
  YYSYMBOL_atom_def = 175,                 /* atom_def  */
  YYSYMBOL_atom_head = 176,                /* atom_head  */
  YYSYMBOL_atom_id = 177,                  /* atom_id  */
  YYSYMBOL_default_val = 178,              /* default_val  */
  YYSYMBOL_constant_def = 179,             /* constant_def  */
  YYSYMBOL_constant_head = 180,            /* constant_head  */
  YYSYMBOL_constant_dims = 181,            /* constant_dims  */
  YYSYMBOL_constant_val = 182,             /* constant_val  */
  YYSYMBOL_model_def = 183,                /* model_def  */
  YYSYMBOL_model_head = 184,               /* model_head  */
  YYSYMBOL_model_id = 185,                 /* model_id  */
  YYSYMBOL_optional_model_parameters = 186, /* optional_model_parameters  */
  YYSYMBOL_optional_parameter_wheres = 187, /* optional_parameter_wheres  */
  YYSYMBOL_optional_parameter_reduction = 188, /* optional_parameter_reduction  */
  YYSYMBOL_universal = 189,                /* universal  */
  YYSYMBOL_definition_def = 190,           /* definition_def  */
  YYSYMBOL_definition_id = 191,            /* definition_id  */
  YYSYMBOL_units_def = 192,                /* units_def  */
  YYSYMBOL_units_statement = 193,          /* units_statement  */
  YYSYMBOL_optional_ladder_end = 194,      /* optional_ladder_end  */
  YYSYMBOL_table_statement = 195,          /* table_statement  */
  YYSYMBOL_table_begin = 196,              /* table_begin  */
  YYSYMBOL_table_mode_on = 197,            /* table_mode_on  */
  YYSYMBOL_table_mode_off = 198,           /* table_mode_off  */
  YYSYMBOL_table_options = 199,            /* table_options  */
  YYSYMBOL_table_decl_opt = 200,           /* table_decl_opt  */
  YYSYMBOL_table_option = 201,             /* table_option  */
  YYSYMBOL_table_body = 202,               /* table_body  */
  YYSYMBOL_table_body_item = 203,          /* table_body_item  */
  YYSYMBOL_table_scalar = 204,             /* table_scalar  */
  YYSYMBOL_values_statement = 205,         /* values_statement  */
  YYSYMBOL_values_default_opt = 206,       /* values_default_opt  */
  YYSYMBOL_values_entries = 207,           /* values_entries  */
  YYSYMBOL_values_entry = 208,             /* values_entry  */
  YYSYMBOL_values_key_list = 209,          /* values_key_list  */
  YYSYMBOL_values_key = 210,               /* values_key  */
  YYSYMBOL_dataset_statement = 211,        /* dataset_statement  */
  YYSYMBOL_212_1 = 212,                    /* $@1  */
  YYSYMBOL_dataset_items = 213,            /* dataset_items  */
  YYSYMBOL_dataset_item = 214,             /* dataset_item  */
  YYSYMBOL_dataset_index_item = 215,       /* dataset_index_item  */
  YYSYMBOL_dataset_map_item = 216,         /* dataset_map_item  */
  YYSYMBOL_dataset_column_selector = 217,  /* dataset_column_selector  */
  YYSYMBOL_dataset_column_ref = 218,       /* dataset_column_ref  */
  YYSYMBOL_dataset_target = 219,           /* dataset_target  */
  YYSYMBOL_dataset_units_opt = 220,        /* dataset_units_opt  */
  YYSYMBOL_dataset_type_opt = 221,         /* dataset_type_opt  */
  YYSYMBOL_dataset_type_req = 222,         /* dataset_type_req  */
  YYSYMBOL_unitdeflist = 223,              /* unitdeflist  */
  YYSYMBOL_unitdef = 224,                  /* unitdef  */
  YYSYMBOL_unitladderitemlist = 225,       /* unitladderitemlist  */
  YYSYMBOL_unitladderitem = 226,           /* unitladderitem  */
  YYSYMBOL_methods = 227,                  /* methods  */
  YYSYMBOL_228_2 = 228,                    /* $@2  */
  YYSYMBOL_initial = 229,                  /* initial  */
  YYSYMBOL_proclist = 230,                 /* proclist  */
  YYSYMBOL_proclistf = 231,                /* proclistf  */
  YYSYMBOL_procedure = 232,                /* procedure  */
  YYSYMBOL_procedure_id = 233,             /* procedure_id  */
  YYSYMBOL_fstatements = 234,              /* fstatements  */
  YYSYMBOL_statements = 235,               /* statements  */
  YYSYMBOL_statement = 236,                /* statement  */
  YYSYMBOL_complex_statement = 237,        /* complex_statement  */
  YYSYMBOL_isa_statement = 238,            /* isa_statement  */
  YYSYMBOL_willbe_statement = 239,         /* willbe_statement  */
  YYSYMBOL_aliases_statement = 240,        /* aliases_statement  */
  YYSYMBOL_optional_set_values = 241,      /* optional_set_values  */
  YYSYMBOL_is_statement = 242,             /* is_statement  */
  YYSYMBOL_isrefinedto_statement = 243,    /* isrefinedto_statement  */
  YYSYMBOL_call_identifier = 244,          /* call_identifier  */
  YYSYMBOL_type_identifier = 245,          /* type_identifier  */
  YYSYMBOL_optional_method = 246,          /* optional_method  */
  YYSYMBOL_optional_of = 247,              /* optional_of  */
  YYSYMBOL_optional_with_value = 248,      /* optional_with_value  */
  YYSYMBOL_arealike_statement = 249,       /* arealike_statement  */
  YYSYMBOL_link_statement = 250,           /* link_statement  */
  YYSYMBOL_unlink_statement = 251,         /* unlink_statement  */
  YYSYMBOL_der_statement = 252,            /* der_statement  */
  YYSYMBOL_independent_statement = 253,    /* independent_statement  */
  YYSYMBOL_arethesame_statement = 254,     /* arethesame_statement  */
  YYSYMBOL_willbethesame_statement = 255,  /* willbethesame_statement  */
  YYSYMBOL_willnotbethesame_statement = 256, /* willnotbethesame_statement  */
  YYSYMBOL_assignment_statement = 257,     /* assignment_statement  */
  YYSYMBOL_reinit_statement = 258,         /* reinit_statement  */
  YYSYMBOL_switchto_statement = 259,       /* switchto_statement  */
  YYSYMBOL_relation_statement = 260,       /* relation_statement  */
  YYSYMBOL_relation = 261,                 /* relation  */
  YYSYMBOL_blackbox_statement = 262,       /* blackbox_statement  */
  YYSYMBOL_input_args = 263,               /* input_args  */
  YYSYMBOL_output_args = 264,              /* output_args  */
  YYSYMBOL_data_args = 265,                /* data_args  */
  YYSYMBOL_for_statement = 266,            /* for_statement  */
  YYSYMBOL_optional_direction = 267,       /* optional_direction  */
  YYSYMBOL_forexprend = 268,               /* forexprend  */
  YYSYMBOL_run_statement = 269,            /* run_statement  */
  YYSYMBOL_fix_statement = 270,            /* fix_statement  */
  YYSYMBOL_fix_and_assign_statement = 271, /* fix_and_assign_statement  */
  YYSYMBOL_free_statement = 272,           /* free_statement  */
  YYSYMBOL_solver_statement = 273,         /* solver_statement  */
  YYSYMBOL_integrator_statement = 274,     /* integrator_statement  */
  YYSYMBOL_option_statement = 275,         /* option_statement  */
  YYSYMBOL_solve_statement = 276,          /* solve_statement  */
  YYSYMBOL_integrate_statement = 277,      /* integrate_statement  */
  YYSYMBOL_observe_statement = 278,        /* observe_statement  */
  YYSYMBOL_observe_as_opt = 279,           /* observe_as_opt  */
  YYSYMBOL_study_statement = 280,          /* study_statement  */
  YYSYMBOL_281_3 = 281,                    /* $@3  */
  YYSYMBOL_study_obs_opt = 282,            /* study_obs_opt  */
  YYSYMBOL_study_vary_opt = 283,           /* study_vary_opt  */
  YYSYMBOL_study_distribution_opt = 284,   /* study_distribution_opt  */
  YYSYMBOL_study_run_opt = 285,            /* study_run_opt  */
  YYSYMBOL_study_file_opt = 286,           /* study_file_opt  */
  YYSYMBOL_study_now_opt = 287,            /* study_now_opt  */
  YYSYMBOL_delete_statement = 288,         /* delete_statement  */
  YYSYMBOL_external_statement = 289,       /* external_statement  */
  YYSYMBOL_call_statement = 290,           /* call_statement  */
  YYSYMBOL_assert_statement = 291,         /* assert_statement  */
  YYSYMBOL_if_statement = 292,             /* if_statement  */
  YYSYMBOL_while_statement = 293,          /* while_statement  */
  YYSYMBOL_optional_else = 294,            /* optional_else  */
  YYSYMBOL_when_statement = 295,           /* when_statement  */
  YYSYMBOL_whenlist = 296,                 /* whenlist  */
  YYSYMBOL_whenlistf = 297,                /* whenlistf  */
  YYSYMBOL_optional_when_case_if = 298,    /* optional_when_case_if  */
  YYSYMBOL_optional_when_case_applies = 299, /* optional_when_case_applies  */
  YYSYMBOL_flow_statement = 300,           /* flow_statement  */
  YYSYMBOL_use_statement = 301,            /* use_statement  */
  YYSYMBOL_select_statement = 302,         /* select_statement  */
  YYSYMBOL_selectlist = 303,               /* selectlist  */
  YYSYMBOL_selectlistf = 304,              /* selectlistf  */
  YYSYMBOL_switch_statement = 305,         /* switch_statement  */
  YYSYMBOL_switchlist = 306,               /* switchlist  */
  YYSYMBOL_switchlistf = 307,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 308,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 309,          /* notes_statement  */
  YYSYMBOL_notes_body = 310,               /* notes_body  */
  YYSYMBOL_noteslist = 311,                /* noteslist  */
  YYSYMBOL_fvarlist = 312,                 /* fvarlist  */
  YYSYMBOL_varlist = 313,                  /* varlist  */
  YYSYMBOL_method_fvarlist = 314,          /* method_fvarlist  */
  YYSYMBOL_method_varlist = 315,           /* method_varlist  */
  YYSYMBOL_fvarref = 316,                  /* fvarref  */
  YYSYMBOL_fname = 317,                    /* fname  */
  YYSYMBOL_name = 318,                     /* name  */
  YYSYMBOL_end = 319,                      /* end  */
  YYSYMBOL_optional_bracedtext = 320,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 321,           /* optional_notes  */
  YYSYMBOL_set = 322,                      /* set  */
  YYSYMBOL_setexprlist = 323,              /* setexprlist  */
  YYSYMBOL_number = 324,                   /* number  */
  YYSYMBOL_realnumber = 325,               /* realnumber  */
  YYSYMBOL_opunits = 326,                  /* opunits  */
  YYSYMBOL_dims = 327,                     /* dims  */
  YYSYMBOL_dimensions = 328,               /* dimensions  */
  YYSYMBOL_dimexpr = 329,                  /* dimexpr  */
  YYSYMBOL_fraction = 330,                 /* fraction  */
  YYSYMBOL_fractail = 331,                 /* fractail  */
  YYSYMBOL_optional_sign = 332,            /* optional_sign  */
  YYSYMBOL_expr = 333,                     /* expr  */
  YYSYMBOL_relop = 334,                    /* relop  */
  YYSYMBOL_logrelop = 335                  /* logrelop  */
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
#define YYLAST   1727

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  162
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  174
/* YYNRULES -- Number of rules.  */
#define YYNRULES  444
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  873

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   399


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
     156,   157,   151,   148,   143,   149,   161,   150,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   158,   155,
     145,   146,   147,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   159,     2,   160,   154,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   144,     2,     2,     2,     2,     2,
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
     135,   136,   137,   138,   139,   140,   141,   142,   152,   153
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   883,   883,   885,   889,   890,   891,   892,   893,   894,
     895,   896,   897,   898,   900,   901,   902,   913,   958,   962,
     967,   975,   979,   984,   992,  1001,  1013,  1044,  1052,  1075,
    1088,  1096,  1117,  1129,  1137,  1190,  1201,  1210,  1216,  1222,
    1230,  1238,  1249,  1289,  1323,  1328,  1334,  1338,  1360,  1366,
    1372,  1380,  1388,  1399,  1441,  1450,  1463,  1472,  1475,  1483,
    1486,  1494,  1497,  1548,  1551,  1558,  1588,  1597,  1602,  1619,
    1645,  1648,  1655,  1676,  1689,  1696,  1703,  1708,  1710,  1715,
    1727,  1740,  1744,  1751,  1760,  1762,  1766,  1767,  1771,  1775,
    1779,  1783,  1787,  1791,  1798,  1802,  1806,  1810,  1814,  1821,
    1828,  1830,  1837,  1838,  1842,  1849,  1850,  1854,  1855,  1856,
    1857,  1862,  1861,  1871,  1873,  1874,  1884,  1885,  1889,  1896,
    1900,  1907,  1911,  1918,  1922,  1929,  1934,  1943,  1946,  1954,
    1957,  1964,  1971,  1974,  1982,  1990,  1993,  2001,  2006,  2016,
    2020,  2019,  2034,  2037,  2045,  2053,  2056,  2081,  2095,  2104,
    2112,  2115,  2187,  2195,  2203,  2204,  2205,  2206,  2207,  2208,
    2209,  2210,  2211,  2212,  2213,  2214,  2215,  2216,  2217,  2218,
    2219,  2221,  2222,  2223,  2224,  2225,  2226,  2227,  2228,  2229,
    2230,  2231,  2232,  2233,  2234,  2235,  2236,  2237,  2238,  2239,
    2240,  2241,  2242,  2243,  2244,  2245,  2246,  2247,  2248,  2249,
    2253,  2258,  2291,  2321,  2325,  2376,  2379,  2386,  2398,  2425,
    2430,  2438,  2443,  2452,  2455,  2463,  2466,  2474,  2477,  2482,
    2490,  2497,  2501,  2505,  2512,  2516,  2523,  2532,  2541,  2548,
    2555,  2562,  2566,  2573,  2580,  2587,  2600,  2617,  2627,  2635,
    2646,  2667,  2674,  2682,  2685,  2722,  2744,  2747,  2751,  2758,
    2762,  2766,  2770,  2777,  2781,  2788,  2796,  2810,  2817,  2825,
    2832,  2840,  2845,  2852,  2859,  2867,  2870,  2878,  2877,  2894,
    2897,  2905,  2907,  2915,  2924,  2937,  2939,  2943,  2951,  2953,
    2961,  2963,  2971,  2973,  2980,  2987,  2998,  3009,  3015,  3025,
    3035,  3038,  3045,  3056,  3068,  3075,  3085,  3092,  3096,  3100,
    3104,  3111,  3114,  3121,  3124,  3131,  3135,  3139,  3143,  3147,
    3154,  3161,  3172,  3182,  3189,  3193,  3197,  3201,  3208,  3219,
    3229,  3236,  3240,  3244,  3248,  3255,  3265,  3302,  3311,  3326,
    3331,  3340,  3351,  3355,  3360,  3373,  3380,  3384,  3389,  3399,
    3403,  3410,  3430,  3434,  3439,  3444,  3459,  3464,  3469,  3474,
    3479,  3484,  3489,  3494,  3499,  3504,  3509,  3514,  3519,  3524,
    3533,  3536,  3544,  3547,  3554,  3559,  3565,  3569,  3573,  3577,
    3582,  3587,  3595,  3602,  3611,  3616,  3641,  3646,  3669,  3673,
    3678,  3684,  3688,  3695,  3699,  3703,  3707,  3711,  3715,  3722,
    3729,  3733,  3741,  3744,  3748,  3755,  3759,  3763,  3767,  3771,
    3775,  3779,  3783,  3787,  3791,  3795,  3799,  3804,  3809,  3814,
    3819,  3824,  3829,  3834,  3838,  3843,  3848,  3853,  3858,  3863,
    3867,  3871,  3875,  3879,  3886,  3890,  3897,  3901,  3908,  3912,
    3919,  3923,  3930,  3934,  3941,  3945,  3967,  3974,  3978,  3982,
    3986,  3990,  3994,  4001,  4005
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
  "ALIASES_TOK", "AND_TOK", "ANY_TOK", "APPLIES_TOK", "AREALIKE_TOK",
  "ARETHESAME_TOK", "ARRAY_TOK", "ASSERT_TOK", "ATOM_TOK", "BEQ_TOK",
  "BNE_TOK", "BREAK_TOK", "CALL_TOK", "CARD_TOK", "CASE_TOK", "CHOICE_TOK",
  "CHECK_TOK", "CONDITIONAL_TOK", "CONSTANT_TOK", "CONTINUE_TOK",
  "CREATE_TOK", "DATA_TOK", "DECREASING_TOK", "DEFAULT_TOK",
  "DEFINITION_TOK", "DELETE_TOK", "DERIV_TOK", "DERLINK_TOK",
  "DIMENSION_TOK", "DIMENSIONLESS_TOK", "DO_TOK", "ELSE_TOK", "END_TOK",
  "EXPECT_TOK", "EXTERNAL_TOK", "FALSE_TOK", "FALLTHRU_TOK", "FIX_TOK",
  "FOR_TOK", "FREE_TOK", "FROM_TOK", "FILE_TOK", "GLOBAL_TOK", "IF_TOK",
  "IGNORE_TOK", "IMPORT_TOK", "IN_TOK", "INITIAL_TOK", "INPUT_TOK",
  "INCREASING_TOK", "INTERACTIVE_TOK", "INDEPENDENT_TOK", "INTEGRATE_TOK",
  "INTEGRATOR_TOK", "INTERSECTION_TOK", "ISA_TOK", "_IS_T",
  "ISREFINEDTO_TOK", "AS_TOK", "LINEAR_TOK", "LOG_TOK", "NOW_TOK",
  "LINK_TOK", "MAXIMIZE_TOK", "MAXINTEGER_TOK", "MAXREAL_TOK",
  "METHODS_TOK", "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK", "NOT_TOK",
  "NOTES_TOK", "OBSERVE_TOK", "OF_TOK", "OPTION_TOK", "OR_TOK",
  "OTHERWISE_TOK", "OUTPUT_TOK", "PROD_TOK", "PROVIDE_TOK", "RATIO_TOK",
  "REFINES_TOK", "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK",
  "REINIT_TOK", "SATISFIED_TOK", "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK",
  "SOLVER_TOK", "STOP_TOK", "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK",
  "SYSTEM_TOK", "STEP_TOK", "STEPS_TOK", "STUDY_TOK", "TABLE_TOK",
  "VALUES_TOK", "DATASET_TOK", "POSITIONAL_TOK", "INDEX_TOK", "COLUMN_TOK",
  "EOL_TOK", "THEN_TOK", "TO_TOK", "TRUE_TOK", "UNION_TOK", "UNITS_TOK",
  "LADDER_TOK", "UNIVERSAL_TOK", "UNLINK_TOK", "VARY_TOK", "WHEN_TOK",
  "WHERE_TOK", "WHILE_TOK", "WILLBE_TOK", "WILLBETHESAME_TOK",
  "WILLNOTBETHESAME_TOK", "ASSIGN_TOK", "CASSIGN_TOK", "DBLCOLON_TOK",
  "USE_TOK", "LEQ_TOK", "GEQ_TOK", "NEQ_TOK", "DOTDOT_TOK", "WITH_TOK",
  "VALUE_TOK", "WITH_VALUE_T", "REAL_TOK", "INTEGER_TOK", "IDENTIFIER_TOK",
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
  "integrator_statement", "option_statement", "solve_statement",
  "integrate_statement", "observe_statement", "observe_as_opt",
  "study_statement", "$@3", "study_obs_opt", "study_vary_opt",
  "study_distribution_opt", "study_run_opt", "study_file_opt",
  "study_now_opt", "delete_statement", "external_statement",
  "call_statement", "assert_statement", "if_statement", "while_statement",
  "optional_else", "when_statement", "whenlist", "whenlistf",
  "optional_when_case_if", "optional_when_case_applies", "flow_statement",
  "use_statement", "select_statement", "selectlist", "selectlistf",
  "switch_statement", "switchlist", "switchlistf", "conditional_statement",
  "notes_statement", "notes_body", "noteslist", "fvarlist", "varlist",
  "method_fvarlist", "method_varlist", "fvarref", "fname", "name", "end",
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

#define YYPACT_NINF (-802)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-341)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -802,   212,  -802,  -802,    86,   -31,  -112,   -85,   -66,    48,
      75,    20,  -802,  -802,  -802,  -802,  -802,  -802,  -802,     5,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,     7,  -802,  -802,
    -802,    43,    74,   112,  -802,  -802,   131,    82,  -802,    84,
     101,   157,    97,   111,  -802,     1,   140,   -16,   250,   223,
     250,   167,   174,   176,  -802,   239,  -802,  -802,   146,   256,
     786,  -802,   -11,   189,   250,   192,  -802,  -802,  -802,   412,
      -8,   -10,  -802,  -802,     3,   945,   190,  -802,  -802,   140,
     201,   116,  -802,  -100,   140,   197,   208,   209,  -802,   218,
     219,  -802,   279,  -802,   256,   228,   324,  -802,   255,  -802,
     250,   230,  -802,  1047,  -802,   247,    91,    98,  -802,  -802,
     291,   236,   237,   258,  -802,  -802,    -7,   259,    -7,  1047,
     140,   350,   261,   125,   245,  1047,  -802,  -802,  1047,  1047,
       5,   140,   263,   133,  -802,   140,   248,   252,   -18,   140,
     264,   266,   152,   -87,  -802,   140,   140,   270,  -802,   158,
     254,     2,  1047,   140,   271,   273,   260,  -802,  1047,  1047,
    1047,   412,  -802,  -802,  -802,  -802,   262,   265,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,    42,   -63,   301,  -802,  1524,   341,   267,   344,
     275,   278,   280,  -802,  -802,   274,   276,  1168,  -802,  -802,
     348,   282,   289,   286,   327,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,   305,   306,
    -802,   140,  -802,  -802,  -802,   140,  -802,  -802,  -802,  -802,
    -802,   308,   250,   268,  -802,   256,   292,   294,   -64,  -802,
     293,  -802,  1524,   296,  -802,   412,   412,   412,   412,   250,
    -802,   140,   140,   298,   299,  -802,  -802,   -17,   -63,  -802,
     406,  -802,  -802,  1195,  -802,  1047,  -802,   412,   412,    14,
    1524,  1524,  -802,   -16,   395,  1047,   412,   412,   330,    -7,
     140,   140,    16,  -802,  -802,  -802,  -802,   412,   412,  1047,
     140,    18,   140,  -802,   433,   417,   412,   412,    45,   140,
      26,  1273,  -802,  -802,  -802,  -802,  1047,   309,   309,   565,
     304,  -802,  -802,    50,  -802,  -802,   328,   329,   328,   328,
    -802,  -802,  1047,  1047,   890,  1047,  -802,  -802,  1047,  1047,
    1047,  -802,  -802,  -802,  1047,  -802,  -802,  -802,  1047,  1047,
    1047,  1047,  1047,  1047,  1047,   311,  -802,   332,   314,  -802,
    -802,   140,  -802,   944,  1047,   317,  -802,   333,  -802,  -802,
    -802,   320,  -802,  -802,   250,     8,   321,    52,  -802,   450,
    -802,   250,  -802,  -802,   339,  -802,  -802,  -802,   412,   325,
     323,   331,   335,  -802,   336,   340,   140,   140,    -7,  -802,
    1047,  -802,  1316,   342,   346,   349,   353,   357,  -802,   362,
    -802,  1524,   345,   347,   140,   361,     6,   351,   412,   354,
     250,    35,   352,   355,  1346,   356,   412,   358,   250,    37,
     366,  -802,   428,  1047,   334,   369,   363,   359,   375,   379,
     372,   412,   373,   250,    40,  -802,  1020,  -802,  -802,   140,
    -802,   378,   458,   458,  -802,   458,  1524,  1524,    60,   384,
    -802,   632,   632,   632,  1551,  1551,    78,    78,   309,   309,
     309,    95,  1463,  -802,  -802,  -802,   385,  -802,  1376,  1524,
    -802,  -802,  -802,   388,    52,  -802,   405,   419,  -802,  -802,
    -802,  -802,   -72,  -802,   137,    44,   393,   399,   398,   400,
     401,  -802,  -802,  -802,  -802,   193,  -802,   402,   407,  -802,
    1122,   522,  1047,  -802,  -802,   421,   140,   140,  -802,  -802,
    -802,  -802,  1047,   185,  -802,    16,   411,  -802,  -802,   412,
     418,  -802,  -802,  1047,    18,   422,  -802,  -802,   412,   426,
     140,   486,   328,  -802,  1524,   130,   430,  -802,  -802,   140,
     140,    26,   530,  -802,  -802,   412,   429,   250,  -802,   431,
     412,   447,    -3,  -802,    -3,   140,    26,  1047,  -802,  1047,
    -802,  -802,  -802,    47,   449,    69,   -72,   -72,   182,  -802,
    -802,  -802,  -802,  -802,   195,  -802,  -802,  -802,  -802,   434,
    -802,  -802,  -802,  -802,  -802,   163,  -802,   250,  1401,   451,
     435,   436,  1083,   273,   438,   250,  -802,  -802,   439,  -802,
    1524,   250,  -802,  -802,   440,  -802,   552,   460,   536,   458,
     -20,  -802,  -802,  -802,  -802,    32,   453,   150,  -802,  -802,
     448,   454,   250,  1047,   597,  -802,   530,  -802,  -802,   489,
     455,  -802,  1047,  1047,  -802,  -802,   456,   250,   459,   461,
     -49,  1524,  -802,  -802,  -802,   195,   462,  -802,   464,   464,
    -802,   -78,   273,  -802,  -802,   465,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,   478,   140,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  1047,  -802,
    -802,   576,  -802,  1047,  -802,   483,  -802,  -802,   521,   472,
    -802,   130,  1047,  -802,  -802,  -802,  -802,  1524,   581,   471,
     597,  -802,   140,  -802,  1524,  1524,    26,  -802,   140,   578,
    -802,  -802,  -802,   494,  -802,  -802,   250,  -802,   476,  -802,
    -802,  1431,   492,  -802,  1524,  -802,   241,  -802,  -802,  -802,
    1524,    11,  1047,  -802,   477,   577,   250,   482,   481,  -802,
     490,  -802,  -802,  1047,  -802,   605,  1569,   487,   537,   506,
     491,   496,  -802,  -802,   -29,  1524,  -802,  -802,   507,  -802,
     140,   495,   575,   519,   523,   554,   555,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,   616,   140,  -802,   -60,   524,   140,   617,
    -802,   587,   508,  -802,  -802,   518,  1047,  1047,   540,  -802,
    -802,   570,   525,   134,  -802,  -802,   542,  -802,  -802,   527,
     -60,   544,   659,  -802,  1524,  1524,   283,  -802,  -802,   134,
    -802,  -802,  -802,   629,  -802,   542,   553,  -802,  -802,  -802,
    -802,   631,   560,  -802,  -802,   538,  -802,   561,  -802,   412,
    -802,   545,  -802
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    16,     0,     0,     0,     0,     0,     0,
       0,   132,    64,     3,    15,     4,     5,     6,     9,     0,
       7,   145,     8,   145,    11,    10,    12,     0,    13,   150,
      14,     0,     0,     0,    66,   150,     0,     0,   342,     0,
      23,     0,     0,    20,   135,     0,     0,     0,     0,   144,
       0,     0,     0,     0,   150,     0,    42,   150,    57,   139,
       0,    67,     0,     0,     0,     0,    25,    21,    22,   365,
       0,     0,    18,    19,     0,   359,     0,   133,    68,   327,
       0,   331,   332,   362,     0,     0,     0,     0,   146,     0,
       0,    36,     0,    56,   139,     0,   142,   150,    59,   140,
       0,     0,   401,     0,   305,     0,     0,     0,   150,   306,
       0,     0,     0,     0,   400,   307,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   396,   398,     0,     0,
       0,     0,     0,     0,   308,     0,     0,     0,     0,   261,
       0,   360,     0,     0,   267,     0,     0,     0,   399,     0,
       0,     0,     0,     0,   376,   395,   342,   402,     0,     0,
       0,   365,   196,   197,   198,   199,     0,     0,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   235,   171,   174,   175,   176,
     200,   177,   178,   179,   181,   180,   182,   183,   184,   185,
     173,   172,   186,   187,   188,   189,   191,   190,   192,   193,
     194,   195,     0,     0,   403,   397,   237,     0,     0,   213,
       0,     0,     0,   367,   403,     0,   364,   366,   344,   343,
       0,     0,   138,     0,    70,   346,   347,   355,   348,   349,
     350,   351,   352,   353,   354,   356,   357,   358,     0,     0,
     329,     0,   334,   363,   341,   328,    26,    28,   148,   150,
      31,     0,     0,   380,   150,   139,     0,     0,     0,   145,
       0,   153,   287,   209,   286,   365,   365,   365,   365,     0,
     284,     0,     0,     0,     0,   256,   255,   335,   336,   339,
       0,   257,   336,     0,   227,     0,   259,   365,   365,     0,
     239,   238,   413,     0,   265,     0,   365,   365,   253,     0,
       0,     0,     0,   262,   258,   361,   309,   365,   365,     0,
       0,     0,   269,    74,   100,     0,   365,   365,     0,     0,
       0,     0,   310,   377,   374,   375,     0,   419,   420,     0,
       0,   151,   152,     0,   220,   228,     0,     0,     0,     0,
     229,   230,     0,     0,     0,     0,   443,   444,     0,     0,
       0,   440,   441,   442,     0,   438,   437,   439,     0,     0,
       0,     0,     0,     0,     0,     0,    29,     0,     0,    17,
      24,     0,   345,     0,     0,     0,    32,     0,   136,    71,
      69,     0,   330,   333,     0,    46,     0,     0,   379,    37,
     143,     0,    58,   150,     0,    54,   141,    65,   365,     0,
       0,     0,     0,   325,     0,     0,     0,     0,     0,   338,
       0,   150,     0,     0,     0,     0,     0,     0,   326,     0,
     264,   260,     0,     0,     0,     0,     0,     0,   365,     0,
       0,   313,     0,     0,     0,     0,   365,     0,     0,   320,
     271,   270,    79,     0,     0,     0,     0,     0,     0,     0,
       0,   365,     0,     0,   296,   150,     0,   436,   405,     0,
     203,   211,   215,   215,   208,   215,   231,   232,     0,   342,
     236,   411,   416,   412,   418,   417,   406,   407,   409,   408,
     410,   414,   415,    30,   214,    27,     0,   370,   369,   368,
      33,   137,   134,     0,     0,    45,     0,    48,    34,   384,
     383,   381,     0,   378,   382,   392,     0,     0,     0,    61,
       0,   431,   432,   433,   434,   404,   226,     0,     0,   337,
     246,   290,     0,   429,   430,     0,     0,     0,   266,   425,
     426,   254,     0,     0,   422,     0,     0,   150,   311,   365,
       0,   423,   424,     0,     0,     0,   150,   318,   365,     0,
       0,   278,     0,    77,   101,     0,     0,   427,   428,     0,
       0,     0,   301,   150,   292,   365,     0,     0,   435,     0,
     365,     0,   217,   207,   217,     0,     0,     0,   404,     0,
     147,    44,    47,   392,   362,     0,     0,     0,   392,    39,
      40,    41,   393,   394,     0,    35,    53,    60,   150,     0,
     210,   285,   340,   248,   247,     0,   150,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   150,   315,     0,   150,
     234,     0,   150,   322,     0,   150,     0,     0,   282,   215,
       0,   110,   109,   107,   108,     0,     0,     0,   105,   111,
       0,     0,     0,     0,   303,   298,   301,   150,   289,     0,
       0,   216,     0,     0,   201,   202,     0,     0,     0,     0,
     403,   371,    51,    50,    52,     0,     0,   388,   385,   386,
     387,     0,   372,    38,   373,     0,    55,   251,   249,   252,
     250,   150,   291,   288,     0,     0,   222,   223,   233,   421,
     312,   314,   150,   317,   319,   321,   150,   324,     0,   279,
     283,   280,    80,     0,    81,     0,    75,    78,     0,     0,
     102,     0,     0,   113,   224,   225,   294,   302,     0,     0,
     303,   300,     0,   212,   219,   218,     0,   293,     0,     0,
      49,    43,   390,     0,   389,    62,     0,   263,     0,   316,
     323,     0,     0,   268,    82,    83,     0,    99,   103,   106,
     104,     0,     0,   150,     0,     0,     0,   243,     0,   241,
       0,   245,   221,     0,   281,     0,     0,     0,     0,     0,
       0,     0,   116,   117,     0,   304,   297,   150,     0,   295,
       0,     0,     0,     0,     0,     0,     0,    93,    97,    96,
      94,    98,    95,    89,    88,    90,    91,    92,    87,    85,
      86,   115,   112,     0,     0,   114,     0,     0,     0,     0,
     299,     0,     0,   240,   242,     0,     0,     0,     0,    76,
      76,     0,     0,     0,   123,   124,   127,   122,   131,     0,
       0,     0,     0,   391,   274,   273,   275,    73,    72,     0,
     125,   121,   128,   129,   126,   127,   205,   244,   276,   277,
     272,     0,     0,   119,   120,     0,   204,     0,   130,   365,
     118,     0,   206
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,   641,  -802,  -802,  -802,  -802,  -127,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,    59,  -802,   -13,  -802,
    -802,  -802,  -802,  -802,  -802,  -135,  -801,  -802,  -149,  -802,
    -802,  -802,  -802,  -802,  -802,   -88,  -802,  -802,   -21,  -802,
    -802,  -802,   -26,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -344,  -802,  -465,   136,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,   601,  -802,  -802,  -802,   364,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,  -802,
    -802,  -802,  -802,  -802,  -560,  -802,    67,    -6,  -802,  -802,
    -802,   180,  -802,  -802,   172,  -802,  -802,  -802,   600,   643,
      -4,  -802,   613,  -802,  -115,   -46,   217,   104,  -802,   138,
     -96,  -802,    58,  -527,  -802,  -802,   232,  -485,  -802,  -802,
    -434,   -15,  -802,  -802
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    54,    55,   516,    25,    56,   507,
     594,    26,    57,    58,    98,   268,   609,    27,    28,    29,
      30,    31,   390,   163,   452,   756,   847,   640,   563,   717,
     776,   809,   810,   164,   454,   645,   646,   647,   648,   165,
     723,   761,   781,   782,   783,   836,   837,   784,   853,   863,
     819,    45,    77,    74,   233,   100,   269,   265,    48,    49,
      88,    89,    59,    60,   166,   167,   168,   169,   170,   866,
     171,   172,   274,   472,   378,   582,   664,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   668,   767,   791,   187,   615,   691,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   430,   198,   322,
     450,   561,   860,   638,   753,   711,   199,   200,   201,   202,
     203,   204,   617,   205,   463,   464,   654,   729,   206,   207,
     208,   440,   441,   209,   448,   449,   210,   211,    47,    79,
      80,    81,   286,   287,   213,   224,    83,    78,   316,   254,
     225,   226,   683,   215,   334,   399,   513,   514,   680,   744,
     604,   227,   373,   374
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      82,   288,    50,   292,   474,   475,   262,   713,   583,    64,
     584,   652,   777,   284,   214,   816,   624,   217,   230,    51,
      75,   404,   228,   284,   662,   319,   667,   595,    94,    52,
     817,    96,   851,    82,   438,   252,   446,    75,    82,    75,
     504,   505,   253,    35,   461,   216,   343,   778,   861,   833,
     344,   345,    38,   549,    36,   558,   212,    37,   575,    69,
     742,    70,   425,   352,   353,   340,   509,   510,   718,   320,
     289,   266,   289,    38,    82,   249,    39,   684,   743,   834,
      53,   835,   279,   599,   512,    82,   672,   714,   272,   308,
    -332,   405,    82,   313,  -332,   715,   439,    82,   447,   323,
     324,   346,   347,   348,   293,    82,   462,   332,    34,  -332,
     300,   678,   679,   301,   302,   550,   294,   559,    41,   779,
     576,    38,    38,   506,    62,    84,   418,   304,   218,   231,
     818,   229,    38,   663,   312,   716,    44,   331,   311,   321,
      76,    38,   232,   337,   338,   339,    46,   330,   684,   543,
     780,    85,    86,    38,    90,   426,    32,   600,   329,   675,
     673,    33,    63,   544,   681,   349,   350,   351,   220,   641,
     642,   643,   419,   644,   712,    65,   766,   401,   234,   409,
     410,   411,   412,   687,    38,   601,   458,   688,   674,    38,
     509,   510,   602,   603,   435,   602,   603,   689,    61,    38,
     690,   423,   424,   511,   270,   393,   469,    71,   512,    82,
     432,   433,     2,     3,    38,     4,   585,    42,   639,   596,
     597,   442,   443,   598,   -63,    40,   677,    43,   370,   371,
     456,   457,   372,   394,   -63,   414,    82,    66,   400,    67,
       5,   289,   775,   368,   369,   370,   371,   275,   406,   372,
     276,   249,    72,   427,   277,    38,    68,   278,     6,   251,
      69,     7,    70,   289,   436,    82,    73,   641,   642,   643,
      69,   644,    70,   834,    82,   835,    82,   -84,   415,    38,
     422,   297,   459,    82,   298,   -63,    75,   596,   597,   306,
     431,   598,   307,   721,    87,     8,   722,   470,     9,    10,
     397,   398,    97,   529,   444,  -332,    91,   437,   317,  -332,
    -332,   318,   520,    92,   326,    93,   445,   327,   451,  -340,
    -340,   466,   154,   623,    95,   460,    99,    11,   219,    12,
     602,   603,   154,   682,   221,   496,   248,   476,   477,   216,
     481,   250,   546,   482,   483,   484,   858,   859,   258,   485,
     555,   -84,   256,   486,   487,   488,   489,   490,   491,   492,
    -332,  -332,  -332,   257,   261,   572,   396,   263,   498,   499,
      82,   528,   289,   259,   260,   264,   267,   518,   -84,   -84,
     -84,   -84,   -84,   413,   -84,   271,   273,   -84,   541,   -84,
     -84,   280,   281,   282,   295,   531,   -84,   283,   290,   -84,
     296,   299,   305,   314,   309,   530,   315,   428,   310,   325,
     328,   333,   527,   335,   375,   377,   336,   341,   102,   383,
     342,   385,   376,    82,  -332,  -332,  -332,  -339,  -339,   106,
     379,   107,    82,   380,   382,   387,   381,   386,   564,   577,
    -332,   388,   222,   389,  -332,   391,   392,   395,   407,   402,
     403,   114,   408,   628,   416,   417,   420,   429,   434,   354,
     453,   455,   634,   372,   468,   579,   493,   471,   473,   495,
     123,   494,   500,   501,   586,   502,   508,   515,   519,   656,
     126,   127,   521,   522,   660,   560,   129,   562,   523,   565,
      82,    82,   535,   525,   133,   524,   536,   526,   503,   533,
     537,   538,   539,   137,   542,   517,   534,   540,   545,   551,
     142,   566,   547,   554,   636,   552,   556,   618,   569,   568,
     567,   627,   570,    82,    82,   148,   149,   622,   355,   571,
     633,   573,   620,   621,   580,   581,   356,   357,   630,    82,
     587,   670,   588,   590,   548,   592,   593,   655,   605,   154,
     155,   156,   557,   157,   606,   607,   608,   616,   610,   611,
     158,   159,   619,   223,   612,   650,   651,   574,   160,   626,
     355,   161,   466,   358,   671,   637,   629,   653,   356,   357,
     632,   666,   685,   669,   635,   649,   661,   657,   659,   686,
     692,   253,   696,   697,   695,   699,   708,   702,   706,   709,
     701,   710,   359,   703,   728,   724,   705,   826,   720,   707,
     732,   725,   733,   736,   738,   358,   747,   741,   598,   739,
     360,   752,   745,   755,   827,   828,   757,   758,   762,   763,
     769,   731,   770,   772,   774,   787,   788,   790,   727,   792,
     793,   795,   811,   812,   359,   813,   821,   734,   735,    82,
     814,   815,   823,   361,   362,   363,   824,   825,   829,   830,
     831,   840,   360,   838,   841,   746,   842,   364,   365,   366,
     367,   368,   369,   370,   371,   843,   749,   372,   846,   849,
     750,   658,   852,   856,   857,   850,    82,   854,   862,   865,
     867,   748,    82,   751,   869,   361,   362,   363,   754,   868,
     870,   162,   872,   848,   719,   855,   864,   760,   759,   364,
     365,   366,   367,   368,   369,   370,   371,   285,   480,   372,
     665,   693,   467,   730,   764,   625,   631,   255,   765,   700,
     303,   291,   676,   740,   768,   704,   591,   786,     0,     0,
       0,     0,     0,     0,   822,     0,     0,   785,     0,     0,
       0,     0,     0,     0,     0,     0,   726,     0,   794,     0,
       0,   820,   361,   362,   363,     0,     0,     0,    82,     0,
       0,   737,    82,   871,     0,     0,     0,   365,   366,   367,
     368,   369,   370,   371,     0,     0,   372,   101,     0,     0,
       0,     0,   102,     0,     0,     0,     0,   103,     0,     0,
       0,   104,   105,   106,  -149,   107,     0,   108,     0,   109,
     832,   844,   845,     0,   839,   110,   111,   112,     0,     0,
       0,  -149,  -149,     0,   113,   114,   115,   116,   117,   118,
       0,     0,     0,   119,     0,     0,     0,  -149,     0,     0,
       0,   120,   121,   122,   123,     0,     0,     0,     0,     0,
     771,     0,   124,   125,   126,   127,  -149,     0,   128,     0,
     129,   130,   131,     0,   132,     0,  -149,     0,   133,     0,
     789,     0,     0,     0,   134,   135,   136,   137,   138,     0,
     139,   140,   141,     0,   142,   143,     0,     0,     0,   144,
     145,   146,   147,     0,     0,     0,   102,     0,     0,   148,
     149,    11,     0,     0,   150,     0,   151,   106,   152,   107,
       0,     0,     0,     0,     0,   153,     0,     0,     0,     0,
     222,     0,     0,   154,   155,   156,     0,   157,     0,   114,
       0,     0,     0,     0,   158,   159,     0,     0,     0,     0,
       0,     0,   160,  -149,     0,   161,     0,     0,   123,     0,
     102,     0,     0,     0,     0,     0,     0,   125,   126,   127,
       0,   106,   128,   107,   129,     0,   235,     0,     0,     0,
       0,     0,   133,     0,   222,     0,     0,     0,     0,     0,
       0,   137,     0,   114,     0,     0,     0,   236,   142,     0,
       0,   237,   238,     0,     0,     0,     0,     0,     0,   239,
       0,     0,   123,   148,   149,     0,     0,     0,     0,     0,
     478,     0,   126,   127,     0,   240,     0,     0,   129,     0,
     241,     0,     0,     0,     0,   355,   133,   154,   155,   479,
       0,   157,     0,   356,   357,   137,     0,   242,   158,   159,
       0,     0,   142,     0,   243,     0,   160,     0,     0,   161,
       0,     0,     0,   102,     0,     0,     0,   148,   149,     0,
     244,     0,     0,     0,   106,   245,   107,   246,     0,     0,
     358,     0,     0,     0,     0,     0,     0,   222,     0,     0,
       0,   154,   155,   156,   247,   157,   114,     0,   355,     0,
       0,     0,   158,   159,     0,   497,   356,   357,     0,   359,
     160,     0,     0,   161,     0,   123,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   126,   127,   360,     0,     0,
       0,   129,     0,     0,     0,     0,     0,   355,     0,   133,
       0,     0,     0,   358,     0,   356,   357,     0,   137,     0,
       0,     0,     0,     0,     0,   142,     0,     0,   613,     0,
     361,   362,   363,     0,     0,     0,     0,     0,     0,     0,
     148,   149,   359,     0,   364,   365,   366,   367,   368,   369,
     370,   371,   358,   355,   372,   614,     0,   578,     0,     0,
     360,   356,   357,     0,   154,   155,   156,     0,   157,     0,
       0,     0,     0,     0,     0,   158,   159,     0,     0,     0,
     355,   359,     0,   160,     0,     0,   161,     0,   356,   357,
       0,     0,     0,   361,   362,   363,     0,     0,   358,   360,
       0,     0,     0,     0,     0,     0,     0,   364,   365,   366,
     367,   368,   369,   370,   371,     0,     0,   372,     0,     0,
     698,     0,     0,     0,     0,   358,     0,   359,     0,     0,
       0,     0,   361,   362,   363,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   360,   364,   365,   366,   367,
     368,   369,   370,   371,   359,     0,   372,     0,   355,     0,
       0,     0,     0,     0,     0,     0,   356,   357,     0,     0,
       0,     0,   360,     0,     0,     0,     0,     0,   361,   362,
     363,   384,     0,     0,     0,     0,   421,   465,     0,     0,
       0,     0,   364,   365,   366,   367,   368,   369,   370,   371,
       0,   355,   372,   358,     0,   361,   362,   363,     0,   356,
     357,     0,     0,     0,     0,     0,     0,     0,     0,   364,
     365,   366,   367,   368,   369,   370,   371,     0,     0,   372,
       0,   355,   359,     0,     0,     0,     0,     0,     0,   356,
     357,     0,     0,     0,     0,     0,   358,     0,     0,     0,
     360,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   355,     0,     0,     0,     0,     0,     0,     0,   356,
     357,     0,     0,   553,     0,   359,   358,     0,     0,     0,
       0,     0,     0,   361,   362,   363,   355,     0,     0,     0,
       0,     0,     0,   360,   356,   357,     0,   364,   365,   366,
     367,   368,   369,   370,   371,   359,   358,   372,   532,     0,
       0,     0,     0,     0,     0,     0,   355,     0,     0,     0,
       0,     0,     0,   360,   356,   357,   361,   362,   363,     0,
       0,   358,     0,     0,     0,   359,     0,     0,     0,     0,
     364,   365,   366,   367,   368,   369,   370,   371,   355,     0,
     372,     0,     0,   360,     0,     0,   361,   362,   363,     0,
     359,   358,     0,     0,     0,     0,     0,     0,     0,     0,
     364,   365,   366,   367,   368,   369,   370,   371,   360,     0,
     372,     0,     0,   694,     0,     0,   361,   362,   363,   589,
     359,     0,     0,   358,     0,     0,     0,     0,     0,     0,
     364,   365,   366,   367,   368,   369,   370,   371,   360,   355,
     372,   361,   362,   363,     0,     0,     0,   356,   357,     0,
       0,     0,   359,   773,     0,   364,   365,   366,   367,   368,
     369,   370,   371,     0,     0,   372,   355,     0,     0,     0,
       0,   361,   362,   363,   356,   357,     0,     0,     0,     0,
       0,     0,     0,     0,   358,   364,   365,   366,   367,   368,
     369,   370,   371,     0,     0,   372,     0,     0,     0,     0,
       0,     0,     0,   361,   362,   363,     0,     0,     0,     0,
       0,   358,     0,   359,     0,   796,     0,     0,   365,   366,
     367,   368,   369,   370,   371,     0,     0,   372,     0,     0,
       0,   360,     0,     0,     0,     0,     0,     0,     0,     0,
     359,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   361,   362,   363,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   364,   365,
     366,   367,   368,   369,   370,   371,     0,     0,   372,   797,
       0,   361,   362,   363,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   365,   366,   367,   368,
     369,   370,   371,     0,     0,   372,   798,   799,   800,   801,
     802,     0,   803,     0,     0,   804,     0,   805,   806,     0,
       0,     0,     0,     0,   807,     0,     0,   808
};

static const yytype_int16 yycheck[] =
{
      46,   116,    23,   118,   348,   349,    94,    27,   473,    35,
     475,   571,     1,    30,    60,    44,   543,    28,    28,    12,
      36,    85,    30,    30,    27,   112,   586,   512,    54,    22,
      59,    57,   833,    79,    18,    81,    18,    36,    84,    36,
      32,    33,   142,   155,    18,    60,     4,    36,   849,   109,
       8,     9,   139,    18,   139,    18,    60,   142,    18,   159,
     138,   161,    48,   126,   127,   161,   138,   139,    36,   156,
     116,    97,   118,   139,   120,    79,   142,   604,   156,   139,
      73,   141,   108,    39,   156,   131,    39,   107,   103,   135,
     139,   155,   138,   139,   143,   115,    80,   143,    80,   145,
     146,    59,    60,    61,   119,   151,    80,   153,   139,   158,
     125,   596,   597,   128,   129,    80,   120,    80,    70,   108,
      80,   139,   139,   115,    50,   141,   143,   131,   139,   139,
     159,   139,   139,   136,   138,   155,   116,   152,   156,   143,
     139,   139,   139,   158,   159,   160,   141,   151,   675,   143,
     139,    47,    48,   139,    50,   141,    70,   113,   156,   593,
     113,    75,    50,   157,   598,   123,   124,   125,    64,   137,
     138,   139,   287,   141,   639,    44,   736,   265,    74,   275,
     276,   277,   278,    20,   139,   141,   141,    24,   141,   139,
     138,   139,   148,   149,   309,   148,   149,    34,   155,   139,
      37,   297,   298,   151,   100,   251,   156,    50,   156,   255,
     306,   307,     0,     1,   139,     3,   156,   142,   562,   150,
     151,   317,   318,   154,    12,     8,   157,    10,   150,   151,
     326,   327,   154,   259,    22,   281,   282,   155,   264,   155,
      28,   287,     1,   148,   149,   150,   151,   156,   269,   154,
     159,   255,   155,   299,   156,   139,   155,   159,    46,   143,
     159,    49,   161,   309,   310,   311,   155,   137,   138,   139,
     159,   141,   161,   139,   320,   141,   322,    36,   282,   139,
     295,   156,   328,   329,   159,    73,    36,   150,   151,   156,
     305,   154,   159,   143,    71,    83,   146,   343,    86,    87,
      32,    33,   156,   418,   319,     4,   139,   311,   156,     8,
       9,   159,   408,   139,   156,   139,   320,   159,   322,   126,
     127,   336,   137,   138,    85,   329,    70,   115,   139,   117,
     148,   149,   137,   138,   142,   381,   146,   352,   353,   354,
     355,   140,   438,   358,   359,   360,    63,    64,   139,   364,
     446,   110,   155,   368,   369,   370,   371,   372,   373,   374,
      59,    60,    61,   155,    85,   461,   262,   139,   383,   384,
     416,   417,   418,   155,   155,    51,   121,   403,   137,   138,
     139,   140,   141,   279,   143,   155,   139,   146,   434,   148,
     149,   100,   156,   156,    44,   421,   155,   139,   139,   158,
     139,   156,   139,   139,   156,   420,   140,   303,   156,   139,
     156,   140,   416,   140,    73,    71,   156,   155,     6,   143,
     155,    73,   155,   469,   123,   124,   125,   126,   127,    17,
     155,    19,   478,   155,   160,   146,   156,   155,   453,   465,
     139,   155,    30,   116,   143,   140,   140,   139,   155,   157,
     156,    39,   156,   549,   156,   156,    50,    62,   128,   158,
      27,    44,   558,   154,   160,   469,   155,   139,   139,   155,
      58,   139,   155,   140,   478,   155,   155,    27,   139,   575,
      68,    69,   157,   160,   580,   119,    74,    59,   157,   155,
     536,   537,   143,   157,    82,   160,   143,   157,   394,   157,
     143,   139,   157,    91,   143,   401,   160,   160,   157,   157,
      98,   142,   158,   157,   560,   160,   158,   532,   143,   160,
     157,   547,   143,   569,   570,   113,   114,   542,     5,   157,
     556,   158,   536,   537,   156,    77,    13,    14,   553,   585,
     156,   587,   157,   155,   440,   140,   127,   573,   155,   137,
     138,   139,   448,   141,   155,   157,   156,    35,   157,   157,
     148,   149,   141,   151,   157,   569,   570,   463,   156,   158,
       5,   159,   587,    50,   589,    89,   158,    47,    13,    14,
     158,   585,   608,   587,   158,   155,   139,   158,   157,   155,
     616,   142,   157,   157,   143,   157,    44,   158,   158,   139,
     626,    65,    79,   629,     7,   157,   632,    84,   155,   635,
     121,   157,   157,   157,   155,    50,   138,   155,   154,   158,
      97,    45,   157,   140,   101,   102,   105,   155,    47,   158,
      52,   657,   138,   157,   142,   158,    59,   155,   653,   158,
     150,    36,   155,   106,    79,   139,   139,   662,   663,   695,
     159,   155,   157,   130,   131,   132,    81,   138,   104,   104,
      44,    44,    97,   139,    77,   691,   158,   144,   145,   146,
     147,   148,   149,   150,   151,   157,   702,   154,   138,   109,
     706,   577,   140,   139,    25,   160,   732,   160,    59,   136,
      59,   695,   738,   708,   156,   130,   131,   132,   713,   139,
     139,    60,   157,   830,   645,   840,   855,   722,   721,   144,
     145,   146,   147,   148,   149,   150,   151,   116,   354,   154,
     584,   617,   157,   656,   730,   545,   554,    84,   732,   625,
     130,   118,   594,   675,   738,   631,   504,   763,    -1,    -1,
      -1,    -1,    -1,    -1,   790,    -1,    -1,   762,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   652,    -1,   773,    -1,
      -1,   787,   130,   131,   132,    -1,    -1,    -1,   814,    -1,
      -1,   667,   818,   869,    -1,    -1,    -1,   145,   146,   147,
     148,   149,   150,   151,    -1,    -1,   154,     1,    -1,    -1,
      -1,    -1,     6,    -1,    -1,    -1,    -1,    11,    -1,    -1,
      -1,    15,    16,    17,    18,    19,    -1,    21,    -1,    23,
     814,   826,   827,    -1,   818,    29,    30,    31,    -1,    -1,
      -1,    35,    36,    -1,    38,    39,    40,    41,    42,    43,
      -1,    -1,    -1,    47,    -1,    -1,    -1,    51,    -1,    -1,
      -1,    55,    56,    57,    58,    -1,    -1,    -1,    -1,    -1,
     746,    -1,    66,    67,    68,    69,    70,    -1,    72,    -1,
      74,    75,    76,    -1,    78,    -1,    80,    -1,    82,    -1,
     766,    -1,    -1,    -1,    88,    89,    90,    91,    92,    -1,
      94,    95,    96,    -1,    98,    99,    -1,    -1,    -1,   103,
     104,   105,   106,    -1,    -1,    -1,     6,    -1,    -1,   113,
     114,   115,    -1,    -1,   118,    -1,   120,    17,   122,    19,
      -1,    -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,    -1,
      30,    -1,    -1,   137,   138,   139,    -1,   141,    -1,    39,
      -1,    -1,    -1,    -1,   148,   149,    -1,    -1,    -1,    -1,
      -1,    -1,   156,   157,    -1,   159,    -1,    -1,    58,    -1,
       6,    -1,    -1,    -1,    -1,    -1,    -1,    67,    68,    69,
      -1,    17,    72,    19,    74,    -1,    21,    -1,    -1,    -1,
      -1,    -1,    82,    -1,    30,    -1,    -1,    -1,    -1,    -1,
      -1,    91,    -1,    39,    -1,    -1,    -1,    42,    98,    -1,
      -1,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    54,
      -1,    -1,    58,   113,   114,    -1,    -1,    -1,    -1,    -1,
     120,    -1,    68,    69,    -1,    70,    -1,    -1,    74,    -1,
      75,    -1,    -1,    -1,    -1,     5,    82,   137,   138,   139,
      -1,   141,    -1,    13,    14,    91,    -1,    92,   148,   149,
      -1,    -1,    98,    -1,    99,    -1,   156,    -1,    -1,   159,
      -1,    -1,    -1,     6,    -1,    -1,    -1,   113,   114,    -1,
     115,    -1,    -1,    -1,    17,   120,    19,   122,    -1,    -1,
      50,    -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,    -1,
      -1,   137,   138,   139,   139,   141,    39,    -1,     5,    -1,
      -1,    -1,   148,   149,    -1,   151,    13,    14,    -1,    79,
     156,    -1,    -1,   159,    -1,    58,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    68,    69,    97,    -1,    -1,
      -1,    74,    -1,    -1,    -1,    -1,    -1,     5,    -1,    82,
      -1,    -1,    -1,    50,    -1,    13,    14,    -1,    91,    -1,
      -1,    -1,    -1,    -1,    -1,    98,    -1,    -1,    26,    -1,
     130,   131,   132,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,   114,    79,    -1,   144,   145,   146,   147,   148,   149,
     150,   151,    50,     5,   154,    53,    -1,   157,    -1,    -1,
      97,    13,    14,    -1,   137,   138,   139,    -1,   141,    -1,
      -1,    -1,    -1,    -1,    -1,   148,   149,    -1,    -1,    -1,
       5,    79,    -1,   156,    -1,    -1,   159,    -1,    13,    14,
      -1,    -1,    -1,   130,   131,   132,    -1,    -1,    50,    97,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,   145,   146,
     147,   148,   149,   150,   151,    -1,    -1,   154,    -1,    -1,
     157,    -1,    -1,    -1,    -1,    50,    -1,    79,    -1,    -1,
      -1,    -1,   130,   131,   132,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    97,   144,   145,   146,   147,
     148,   149,   150,   151,    79,    -1,   154,    -1,     5,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    13,    14,    -1,    -1,
      -1,    -1,    97,    -1,    -1,    -1,    -1,    -1,   130,   131,
     132,   133,    -1,    -1,    -1,    -1,   111,    34,    -1,    -1,
      -1,    -1,   144,   145,   146,   147,   148,   149,   150,   151,
      -1,     5,   154,    50,    -1,   130,   131,   132,    -1,    13,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,
     145,   146,   147,   148,   149,   150,   151,    -1,    -1,   154,
      -1,     5,    79,    -1,    -1,    -1,    -1,    -1,    -1,    13,
      14,    -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,    -1,
      97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,
      14,    -1,    -1,    47,    -1,    79,    50,    -1,    -1,    -1,
      -1,    -1,    -1,   130,   131,   132,     5,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    13,    14,    -1,   144,   145,   146,
     147,   148,   149,   150,   151,    79,    50,   154,   112,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,    -1,    -1,    -1,
      -1,    -1,    -1,    97,    13,    14,   130,   131,   132,    -1,
      -1,    50,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
     144,   145,   146,   147,   148,   149,   150,   151,     5,    -1,
     154,    -1,    -1,    97,    -1,    -1,   130,   131,   132,    -1,
      79,    50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     144,   145,   146,   147,   148,   149,   150,   151,    97,    -1,
     154,    -1,    -1,   102,    -1,    -1,   130,   131,   132,   133,
      79,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,    -1,
     144,   145,   146,   147,   148,   149,   150,   151,    97,     5,
     154,   130,   131,   132,    -1,    -1,    -1,    13,    14,    -1,
      -1,    -1,    79,   112,    -1,   144,   145,   146,   147,   148,
     149,   150,   151,    -1,    -1,   154,     5,    -1,    -1,    -1,
      -1,   130,   131,   132,    13,    14,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    50,   144,   145,   146,   147,   148,
     149,   150,   151,    -1,    -1,   154,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   130,   131,   132,    -1,    -1,    -1,    -1,
      -1,    50,    -1,    79,    -1,    36,    -1,    -1,   145,   146,
     147,   148,   149,   150,   151,    -1,    -1,   154,    -1,    -1,
      -1,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   130,   131,   132,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,   145,
     146,   147,   148,   149,   150,   151,    -1,    -1,   154,   110,
      -1,   130,   131,   132,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   145,   146,   147,   148,
     149,   150,   151,    -1,    -1,   154,   137,   138,   139,   140,
     141,    -1,   143,    -1,    -1,   146,    -1,   148,   149,    -1,
      -1,    -1,    -1,    -1,   155,    -1,    -1,   158
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   163,     0,     1,     3,    28,    46,    49,    83,    86,
      87,   115,   117,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   179,   183,   189,   190,   191,
     192,   193,    70,    75,   139,   155,   139,   142,   139,   142,
     318,    70,   142,   318,   116,   223,   141,   310,   230,   231,
     230,    12,    22,    73,   176,   177,   180,   184,   185,   234,
     235,   155,    50,    50,   234,    44,   155,   155,   155,   159,
     161,    50,   155,   155,   225,    36,   139,   224,   319,   311,
     312,   313,   317,   318,   141,   319,   319,    71,   232,   233,
     319,   139,   139,   139,   234,    85,   234,   156,   186,    70,
     227,     1,     6,    11,    15,    16,    17,    19,    21,    23,
      29,    30,    31,    38,    39,    40,    41,    42,    43,    47,
      55,    56,    57,    58,    66,    67,    68,    69,    72,    74,
      75,    76,    78,    82,    88,    89,    90,    91,    92,    94,
      95,    96,    98,    99,   103,   104,   105,   106,   113,   114,
     118,   120,   122,   129,   137,   138,   139,   141,   148,   149,
     156,   159,   193,   195,   205,   211,   236,   237,   238,   239,
     240,   242,   243,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   266,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   280,   288,
     289,   290,   291,   292,   293,   295,   300,   301,   302,   305,
     308,   309,   312,   316,   317,   325,   333,    28,   139,   139,
     319,   142,    30,   151,   317,   322,   323,   333,    30,   139,
      28,   139,   139,   226,   319,    21,    42,    46,    47,    54,
      70,    75,    92,    99,   115,   120,   122,   139,   146,   312,
     140,   143,   317,   142,   321,   311,   155,   155,   139,   155,
     155,    85,   227,   139,    51,   229,   234,   121,   187,   228,
     319,   155,   333,   139,   244,   156,   159,   156,   159,   234,
     100,   156,   156,   139,    30,   257,   314,   315,   316,   317,
     139,   314,   316,   333,   312,    44,   139,   156,   159,   156,
     333,   333,   333,   310,   312,   139,   156,   159,   317,   156,
     156,   156,   312,   317,   139,   140,   320,   156,   159,   112,
     156,   312,   281,   317,   317,   139,   156,   159,   156,   156,
     312,   333,   317,   140,   326,   140,   156,   333,   333,   333,
     322,   155,   155,     4,     8,     9,    59,    60,    61,   123,
     124,   125,   126,   127,   158,     5,    13,    14,    50,    79,
      97,   130,   131,   132,   144,   145,   146,   147,   148,   149,
     150,   151,   154,   334,   335,    73,   155,    71,   246,   155,
     155,   156,   160,   143,   133,    73,   155,   146,   155,   116,
     194,   140,   140,   317,   234,   139,   319,    32,    33,   327,
     234,   227,   157,   156,    85,   155,   230,   155,   156,   322,
     322,   322,   322,   319,   317,   312,   156,   156,   143,   316,
      50,   111,   333,   322,   322,    48,   141,   317,   319,    62,
     279,   333,   322,   322,   128,   316,   317,   312,    18,    80,
     303,   304,   322,   322,   333,   312,    18,    80,   306,   307,
     282,   312,   196,    27,   206,    44,   322,   322,   141,   317,
     312,    18,    80,   296,   297,    34,   333,   157,   160,   156,
     317,   139,   245,   139,   245,   245,   333,   333,   120,   139,
     261,   333,   333,   333,   333,   333,   333,   333,   333,   333,
     333,   333,   333,   155,   139,   155,   317,   151,   333,   333,
     155,   140,   155,   319,    32,    33,   115,   181,   155,   138,
     139,   151,   156,   328,   329,    27,   178,   319,   234,   139,
     322,   157,   160,   157,   160,   157,   157,   312,   317,   316,
     333,   234,   112,   157,   160,   143,   143,   143,   139,   157,
     160,   317,   143,   143,   157,   157,   322,   158,   319,    18,
      80,   157,   160,    47,   157,   322,   158,   319,    18,    80,
     119,   283,    59,   200,   333,   155,   142,   157,   160,   143,
     143,   157,   322,   158,   319,    18,    80,   234,   157,   312,
     156,    77,   247,   247,   247,   156,   312,   156,   157,   133,
     155,   328,   140,   127,   182,   329,   150,   151,   154,    39,
     113,   141,   148,   149,   332,   155,   155,   157,   156,   188,
     157,   157,   157,    26,    53,   267,    35,   294,   333,   141,
     312,   312,   333,   138,   325,   303,   158,   234,   322,   158,
     333,   306,   158,   234,   322,   158,   317,    89,   285,   245,
     199,   137,   138,   139,   141,   207,   208,   209,   210,   155,
     312,   312,   296,    47,   298,   234,   322,   158,   319,   157,
     322,   139,    27,   136,   248,   248,   312,   296,   263,   312,
     317,   333,    39,   113,   141,   332,   321,   157,   329,   329,
     330,   332,   138,   324,   325,   234,   155,    20,    24,    34,
      37,   268,   234,   319,   102,   143,   157,   157,   157,   157,
     319,   234,   158,   234,   319,   234,   158,   234,    44,   139,
      65,   287,   247,    27,   107,   115,   155,   201,    36,   208,
     155,   143,   146,   212,   157,   157,   319,   333,     7,   299,
     298,   234,   121,   157,   333,   333,   157,   319,   155,   158,
     324,   155,   138,   156,   331,   157,   234,   138,   312,   234,
     234,   333,    45,   286,   333,   140,   197,   105,   155,   210,
     333,   213,    47,   158,   299,   312,   296,   264,   312,    52,
     138,   319,   157,   112,   142,     1,   202,     1,    36,   108,
     139,   214,   215,   216,   219,   333,   234,   158,    59,   319,
     155,   265,   158,   150,   333,    36,    36,   110,   137,   138,
     139,   140,   141,   143,   146,   148,   149,   155,   158,   203,
     204,   155,   106,   139,   159,   155,    44,    59,   159,   222,
     234,   139,   317,   157,    81,   138,    84,   101,   102,   104,
     104,    44,   312,   109,   139,   141,   217,   218,   139,   312,
      44,    77,   158,   157,   333,   333,   138,   198,   198,   109,
     160,   218,   140,   220,   160,   217,   139,    25,    63,    64,
     284,   218,    59,   221,   220,   136,   241,    59,   139,   156,
     139,   322,   157
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   162,   163,   163,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   165,   166,   166,
     166,   167,   167,   167,   168,   168,   169,   170,   171,   172,
     172,   173,   174,   174,   175,   176,   177,   178,   178,   178,
     178,   178,   179,   180,   181,   181,   181,   181,   182,   182,
     182,   182,   182,   183,   184,   184,   185,   186,   186,   187,
     187,   188,   188,   189,   189,   190,   191,   192,   193,   193,
     194,   194,   195,   195,   196,   197,   198,   199,   199,   200,
     200,   201,   201,   201,   202,   202,   203,   203,   203,   203,
     203,   203,   203,   203,   204,   204,   204,   204,   204,   205,
     206,   206,   207,   207,   208,   209,   209,   210,   210,   210,
     210,   212,   211,   213,   213,   213,   214,   214,   215,   216,
     216,   217,   217,   218,   218,   219,   219,   220,   220,   221,
     221,   222,   223,   223,   224,   225,   225,   226,   226,   227,
     228,   227,   229,   229,   230,   231,   231,   232,   233,   234,
     235,   235,   235,   235,   236,   236,   236,   236,   236,   236,
     236,   236,   236,   236,   236,   236,   236,   236,   236,   236,
     236,   236,   236,   236,   236,   236,   236,   236,   236,   236,
     236,   236,   236,   236,   236,   236,   236,   236,   236,   236,
     236,   236,   236,   236,   236,   236,   236,   236,   236,   236,
     237,   238,   239,   240,   240,   241,   241,   242,   243,   244,
     244,   245,   245,   246,   246,   247,   247,   248,   248,   248,
     249,   250,   250,   250,   251,   251,   252,   253,   254,   255,
     256,   257,   257,   258,   259,   260,   260,   261,   261,   261,
     262,   263,   264,   265,   265,   266,   267,   267,   267,   268,
     268,   268,   268,   269,   269,   270,   271,   272,   273,   274,
     275,   276,   276,   277,   278,   279,   279,   281,   280,   282,
     282,   283,   283,   283,   283,   284,   284,   284,   285,   285,
     286,   286,   287,   287,   288,   289,   290,   291,   292,   293,
     294,   294,   295,   295,   295,   295,   296,   297,   297,   297,
     297,   298,   298,   299,   299,   300,   300,   300,   300,   300,
     301,   302,   302,   303,   304,   304,   304,   304,   305,   305,
     306,   307,   307,   307,   307,   308,   309,   310,   310,   311,
     311,   312,   313,   313,   313,   314,   315,   315,   315,   316,
     316,   317,   318,   318,   318,   318,   319,   319,   319,   319,
     319,   319,   319,   319,   319,   319,   319,   319,   319,   319,
     320,   320,   321,   321,   322,   322,   323,   323,   323,   323,
     323,   323,   324,   324,   325,   325,   326,   326,   327,   327,
     327,   328,   328,   329,   329,   329,   329,   329,   329,   330,
     331,   331,   332,   332,   332,   333,   333,   333,   333,   333,
     333,   333,   333,   333,   333,   333,   333,   333,   333,   333,
     333,   333,   333,   333,   333,   333,   333,   333,   333,   333,
     333,   333,   333,   333,   333,   333,   333,   333,   333,   333,
     333,   333,   333,   333,   333,   333,   333,   334,   334,   334,
     334,   334,   334,   335,   335
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
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     5,     5,     3,    12,     0,     4,     4,     3,     1,
       4,     1,     4,     0,     2,     0,     2,     0,     2,     2,
       2,     8,     6,     6,     6,     6,     4,     2,     2,     2,
       2,     3,     3,     6,     5,     1,     3,     1,     2,     2,
       9,     3,     3,     0,     4,     8,     0,     1,     1,     1,
       1,     1,     1,     2,     4,     2,     2,     2,     2,     2,
       3,     1,     2,     7,     3,     0,     2,     0,     7,     0,
       1,     0,     9,     8,     8,     0,     1,     1,     0,     2,
       0,     2,     0,     1,     2,     5,     2,     2,     6,     5,
       0,     2,     4,     6,     6,     8,     1,     6,     3,     7,
       4,     0,     2,     0,     3,     1,     1,     1,     1,     2,
       2,     4,     6,     1,     4,     3,     5,     4,     4,     6,
       1,     4,     3,     5,     4,     3,     3,     2,     3,     2,
       3,     1,     1,     3,     2,     1,     1,     3,     2,     1,
       4,     2,     1,     3,     3,     4,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     1,
       0,     1,     0,     1,     1,     0,     1,     1,     3,     3,
       3,     5,     1,     1,     2,     2,     0,     1,     2,     1,
       0,     1,     1,     1,     1,     3,     3,     3,     3,     2,
       1,     5,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     3,     3,     3,     3,     3,     2,
       2,     6,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     3,     1,     1,     1,
       1,     1,     1,     1,     1
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
#line 903 "ascend/compiler/ascParse.y"
        {
	  /*
	   * The specific syntax diagnostic is already emitted by zz_error().
	   * Emitting a generic message here causes noisy duplicates during
	   * parser recovery.
	   */
	}
#line 2964 "ascend/compiler/ascParse.c"
    break;

  case 17: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 914 "ascend/compiler/ascParse.y"
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
#line 3010 "ascend/compiler/ascParse.c"
    break;

  case 18: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 959 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 3018 "ascend/compiler/ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK name ';'  */
#line 963 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 3027 "ascend/compiler/ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name  */
#line 968 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 3036 "ascend/compiler/ascParse.c"
    break;

  case 21: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 976 "ascend/compiler/ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 3044 "ascend/compiler/ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK name ';'  */
#line 980 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 3053 "ascend/compiler/ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name  */
#line 985 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 3062 "ascend/compiler/ascParse.c"
    break;

  case 24: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 993 "ascend/compiler/ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
            );
          }
	}
#line 3075 "ascend/compiler/ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 1002 "ascend/compiler/ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 3088 "ascend/compiler/ascParse.c"
    break;

  case 26: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 1014 "ascend/compiler/ascParse.y"
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
#line 3120 "ascend/compiler/ascParse.c"
    break;

  case 27: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 1045 "ascend/compiler/ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 3129 "ascend/compiler/ascParse.c"
    break;

  case 28: /* add_method_def: add_method_head proclist end ';'  */
#line 1053 "ascend/compiler/ascParse.y"
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
#line 3153 "ascend/compiler/ascParse.c"
    break;

  case 29: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1076 "ascend/compiler/ascParse.y"
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
#line 3170 "ascend/compiler/ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1089 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3179 "ascend/compiler/ascParse.c"
    break;

  case 31: /* replace_method_def: replace_method_head proclist end ';'  */
#line 1097 "ascend/compiler/ascParse.y"
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
#line 3201 "ascend/compiler/ascParse.c"
    break;

  case 32: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1118 "ascend/compiler/ascParse.y"
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
#line 3217 "ascend/compiler/ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1130 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3226 "ascend/compiler/ascParse.c"
    break;

  case 34: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 1138 "ascend/compiler/ascParse.y"
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
#line 3280 "ascend/compiler/ascParse.c"
    break;

  case 35: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 1191 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 3292 "ascend/compiler/ascParse.c"
    break;

  case 36: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 1202 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3301 "ascend/compiler/ascParse.c"
    break;

  case 37: /* default_val: %empty  */
#line 1210 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3312 "ascend/compiler/ascParse.c"
    break;

  case 38: /* default_val: DEFAULT_TOK optional_sign number  */
#line 1217 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3322 "ascend/compiler/ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 1223 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 3334 "ascend/compiler/ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 1231 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 3346 "ascend/compiler/ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 1239 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 3358 "ascend/compiler/ascParse.c"
    break;

  case 42: /* constant_def: universal constant_head  */
#line 1250 "ascend/compiler/ascParse.y"
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
#line 3399 "ascend/compiler/ascParse.c"
    break;

  case 43: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK constant_dims constant_val optional_notes ';'  */
#line 1291 "ascend/compiler/ascParse.y"
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
#line 3433 "ascend/compiler/ascParse.c"
    break;

  case 44: /* constant_dims: DIMENSION_TOK dimensions  */
#line 1324 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	  g_constant_units = NULL;
	}
#line 3442 "ascend/compiler/ascParse.c"
    break;

  case 45: /* constant_dims: DIMENSIONLESS_TOK  */
#line 1329 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	  g_constant_units = NULL;
	}
#line 3451 "ascend/compiler/ascParse.c"
    break;

  case 46: /* constant_dims: %empty  */
#line 1334 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	  g_constant_units = NULL;
	}
#line 3460 "ascend/compiler/ascParse.c"
    break;

  case 47: /* constant_dims: UNITS_TOK BRACEDTEXT_TOK  */
#line 1339 "ascend/compiler/ascParse.y"
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
#line 3482 "ascend/compiler/ascParse.c"
    break;

  case 48: /* constant_val: %empty  */
#line 1360 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3493 "ascend/compiler/ascParse.c"
    break;

  case 49: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 1367 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3503 "ascend/compiler/ascParse.c"
    break;

  case 50: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 1373 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3515 "ascend/compiler/ascParse.c"
    break;

  case 51: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 1381 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3527 "ascend/compiler/ascParse.c"
    break;

  case 52: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 1389 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 3539 "ascend/compiler/ascParse.c"
    break;

  case 53: /* model_def: universal model_head fstatements initial methods end ';'  */
#line 1400 "ascend/compiler/ascParse.y"
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
#line 3582 "ascend/compiler/ascParse.c"
    break;

  case 54: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 1443 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 3594 "ascend/compiler/ascParse.c"
    break;

  case 55: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 1452 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 3607 "ascend/compiler/ascParse.c"
    break;

  case 56: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 1464 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3616 "ascend/compiler/ascParse.c"
    break;

  case 57: /* optional_model_parameters: %empty  */
#line 1472 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3624 "ascend/compiler/ascParse.c"
    break;

  case 58: /* optional_model_parameters: '(' fstatements ')'  */
#line 1476 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3632 "ascend/compiler/ascParse.c"
    break;

  case 59: /* optional_parameter_wheres: %empty  */
#line 1483 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3640 "ascend/compiler/ascParse.c"
    break;

  case 60: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 1487 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3648 "ascend/compiler/ascParse.c"
    break;

  case 61: /* optional_parameter_reduction: %empty  */
#line 1494 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3656 "ascend/compiler/ascParse.c"
    break;

  case 62: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 1498 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3664 "ascend/compiler/ascParse.c"
    break;

  case 63: /* universal: %empty  */
#line 1548 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3672 "ascend/compiler/ascParse.c"
    break;

  case 64: /* universal: UNIVERSAL_TOK  */
#line 1552 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3680 "ascend/compiler/ascParse.c"
    break;

  case 65: /* definition_def: definition_id fstatements methods end ';'  */
#line 1559 "ascend/compiler/ascParse.y"
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
#line 3711 "ascend/compiler/ascParse.c"
    break;

  case 66: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1589 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3720 "ascend/compiler/ascParse.c"
    break;

  case 67: /* units_def: units_statement ';'  */
#line 1598 "ascend/compiler/ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 3726 "ascend/compiler/ascParse.c"
    break;

  case 68: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1603 "ascend/compiler/ascParse.y"
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
#line 3747 "ascend/compiler/ascParse.c"
    break;

  case 69: /* units_statement: UNITS_TOK LADDER_TOK unitladderitemlist end optional_ladder_end  */
#line 1620 "ascend/compiler/ascParse.y"
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
#line 3773 "ascend/compiler/ascParse.c"
    break;

  case 70: /* optional_ladder_end: %empty  */
#line 1645 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3781 "ascend/compiler/ascParse.c"
    break;

  case 71: /* optional_ladder_end: LADDER_TOK  */
#line 1649 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3789 "ascend/compiler/ascParse.c"
    break;

  case 72: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on table_body END_TOK TABLE_TOK table_mode_off  */
#line 1656 "ascend/compiler/ascParse.y"
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
#line 3814 "ascend/compiler/ascParse.c"
    break;

  case 73: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on error END_TOK TABLE_TOK table_mode_off  */
#line 1677 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-9].nptr));
	  TableParseAbort();
	  ErrMsg_Generic("Error in TABLE body.");
	  g_untrapped_error++;
	  yyerrok;
	  (yyval.statptr) = NULL;
	}
#line 3827 "ascend/compiler/ascParse.c"
    break;

  case 74: /* table_begin: %empty  */
#line 1689 "ascend/compiler/ascParse.y"
        {
	  TableParseBegin();
	}
#line 3835 "ascend/compiler/ascParse.c"
    break;

  case 75: /* table_mode_on: %empty  */
#line 1696 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(1);
	}
#line 3843 "ascend/compiler/ascParse.c"
    break;

  case 76: /* table_mode_off: %empty  */
#line 1703 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(0);
	}
#line 3851 "ascend/compiler/ascParse.c"
    break;

  case 79: /* table_decl_opt: %empty  */
#line 1715 "ascend/compiler/ascParse.y"
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
#line 3868 "ascend/compiler/ascParse.c"
    break;

  case 80: /* table_decl_opt: ISA_TOK type_identifier optional_of  */
#line 1728 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_type = (yyvsp[-1].id_ptr);
	  g_table_parse.decl_set_type = (yyvsp[0].id_ptr);
	  g_table_parse.decl_typeargs = g_typeargs;
	  g_typeargs = NULL;
	}
#line 3882 "ascend/compiler/ascParse.c"
    break;

  case 81: /* table_option: POSITIONAL_TOK  */
#line 1741 "ascend/compiler/ascParse.y"
        {
	  g_table_parse.positional = 1;
	}
#line 3890 "ascend/compiler/ascParse.c"
    break;

  case 82: /* table_option: DEFAULT_TOK expr  */
#line 1745 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.default_expr != NULL) {
	    DestroyExprList(g_table_parse.default_expr);
	  }
	  g_table_parse.default_expr = (yyvsp[0].eptr);
	}
#line 3901 "ascend/compiler/ascParse.c"
    break;

  case 83: /* table_option: UNITS_TOK BRACEDTEXT_TOK  */
#line 1752 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.units != NULL) {
	    ascfree(g_table_parse.units);
	  }
	  g_table_parse.units = ASC_STRDUP((yyvsp[0].braced_ptr));
	}
#line 3912 "ascend/compiler/ascParse.c"
    break;

  case 87: /* table_body_item: ':'  */
#line 1768 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(":",0);
	}
#line 3920 "ascend/compiler/ascParse.c"
    break;

  case 88: /* table_body_item: '='  */
#line 1772 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("=",0);
	}
#line 3928 "ascend/compiler/ascParse.c"
    break;

  case 89: /* table_body_item: ','  */
#line 1776 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(",",0);
	}
#line 3936 "ascend/compiler/ascParse.c"
    break;

  case 90: /* table_body_item: '+'  */
#line 1780 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("+",0);
	}
#line 3944 "ascend/compiler/ascParse.c"
    break;

  case 91: /* table_body_item: '-'  */
#line 1784 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("-",0);
	}
#line 3952 "ascend/compiler/ascParse.c"
    break;

  case 92: /* table_body_item: ';'  */
#line 1788 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3960 "ascend/compiler/ascParse.c"
    break;

  case 93: /* table_body_item: EOL_TOK  */
#line 1792 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3968 "ascend/compiler/ascParse.c"
    break;

  case 94: /* table_scalar: IDENTIFIER_TOK  */
#line 1799 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(SCP((yyvsp[0].id_ptr)),1);
	}
#line 3976 "ascend/compiler/ascParse.c"
    break;

  case 95: /* table_scalar: SYMBOL_TOK  */
#line 1803 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendSymbol((yyvsp[0].sym_ptr));
	}
#line 3984 "ascend/compiler/ascParse.c"
    break;

  case 96: /* table_scalar: INTEGER_TOK  */
#line 1807 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendInteger((yyvsp[0].int_value));
	}
#line 3992 "ascend/compiler/ascParse.c"
    break;

  case 97: /* table_scalar: REAL_TOK  */
#line 1811 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendReal((yyvsp[0].real_value));
	}
#line 4000 "ascend/compiler/ascParse.c"
    break;

  case 98: /* table_scalar: BRACEDTEXT_TOK  */
#line 1815 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendBraced((yyvsp[0].braced_ptr));
	}
#line 4008 "ascend/compiler/ascParse.c"
    break;

  case 99: /* values_statement: VALUES_TOK fname values_default_opt ';' values_entries END_TOK VALUES_TOK  */
#line 1822 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-5].nptr));
	  (yyval.statptr) = NULL;
	}
#line 4017 "ascend/compiler/ascParse.c"
    break;

  case 101: /* values_default_opt: DEFAULT_TOK expr  */
#line 1831 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 4025 "ascend/compiler/ascParse.c"
    break;

  case 104: /* values_entry: values_key_list '=' expr  */
#line 1843 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 4033 "ascend/compiler/ascParse.c"
    break;

  case 111: /* $@1: %empty  */
#line 1862 "ascend/compiler/ascParse.y"
        {
	  DatasetParseBegin((yyvsp[-3].id_ptr),(yyvsp[-1].dquote_ptr));
	}
#line 4041 "ascend/compiler/ascParse.c"
    break;

  case 112: /* dataset_statement: DATASET_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';' $@1 dataset_items END_TOK DATASET_TOK  */
#line 1866 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = DatasetParseFinish();
	}
#line 4049 "ascend/compiler/ascParse.c"
    break;

  case 115: /* dataset_items: dataset_items error ';'  */
#line 1875 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in DATASET item.");
	  ErrMsg_Generic("Check DATASET statement syntax.");
	  g_untrapped_error++;
	  yyerrok;
	}
#line 4060 "ascend/compiler/ascParse.c"
    break;

  case 118: /* dataset_index_item: INDEX_TOK IDENTIFIER_TOK FROM_TOK COLUMN_TOK dataset_column_ref ISA_TOK IDENTIFIER_TOK  */
#line 1890 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddIndex((yyvsp[-5].id_ptr),(yyvsp[-2].id_ptr),(yyvsp[0].id_ptr));
	}
#line 4068 "ascend/compiler/ascParse.c"
    break;

  case 119: /* dataset_map_item: dataset_target FROM_TOK dataset_column_selector dataset_units_opt dataset_type_opt  */
#line 1897 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-2].id_ptr),(yyvsp[-1].braced_ptr),(yyvsp[0].id_ptr));
	}
#line 4076 "ascend/compiler/ascParse.c"
    break;

  case 120: /* dataset_map_item: dataset_target dataset_type_req FROM_TOK dataset_column_selector dataset_units_opt  */
#line 1901 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-1].id_ptr),(yyvsp[0].braced_ptr),(yyvsp[-3].id_ptr));
	}
#line 4084 "ascend/compiler/ascParse.c"
    break;

  case 121: /* dataset_column_selector: COLUMN_TOK dataset_column_ref  */
#line 1908 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4092 "ascend/compiler/ascParse.c"
    break;

  case 122: /* dataset_column_selector: dataset_column_ref  */
#line 1912 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4100 "ascend/compiler/ascParse.c"
    break;

  case 123: /* dataset_column_ref: IDENTIFIER_TOK  */
#line 1919 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4108 "ascend/compiler/ascParse.c"
    break;

  case 124: /* dataset_column_ref: SYMBOL_TOK  */
#line 1923 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].sym_ptr);
	}
#line 4116 "ascend/compiler/ascParse.c"
    break;

  case 125: /* dataset_target: IDENTIFIER_TOK '[' fvarlist ']'  */
#line 1930 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices(CreateIdName((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4125 "ascend/compiler/ascParse.c"
    break;

  case 126: /* dataset_target: dataset_target '[' fvarlist ']'  */
#line 1935 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices((yyvsp[-3].nptr),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4134 "ascend/compiler/ascParse.c"
    break;

  case 127: /* dataset_units_opt: %empty  */
#line 1943 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 4142 "ascend/compiler/ascParse.c"
    break;

  case 128: /* dataset_units_opt: BRACEDTEXT_TOK  */
#line 1947 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 4150 "ascend/compiler/ascParse.c"
    break;

  case 129: /* dataset_type_opt: %empty  */
#line 1954 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4158 "ascend/compiler/ascParse.c"
    break;

  case 130: /* dataset_type_opt: ISA_TOK IDENTIFIER_TOK  */
#line 1958 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4166 "ascend/compiler/ascParse.c"
    break;

  case 131: /* dataset_type_req: ISA_TOK IDENTIFIER_TOK  */
#line 1965 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4174 "ascend/compiler/ascParse.c"
    break;

  case 132: /* unitdeflist: %empty  */
#line 1971 "ascend/compiler/ascParse.y"
                {
		  (yyval.listp) = gl_create(100L);
	}
#line 4182 "ascend/compiler/ascParse.c"
    break;

  case 133: /* unitdeflist: unitdeflist unitdef  */
#line 1975 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 4191 "ascend/compiler/ascParse.c"
    break;

  case 134: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1983 "ascend/compiler/ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4200 "ascend/compiler/ascParse.c"
    break;

  case 135: /* unitladderitemlist: %empty  */
#line 1990 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(20L);
	}
#line 4208 "ascend/compiler/ascParse.c"
    break;

  case 136: /* unitladderitemlist: unitladderitemlist unitladderitem ';'  */
#line 1994 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].ulitemptr));
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4217 "ascend/compiler/ascParse.c"
    break;

  case 137: /* unitladderitem: IDENTIFIER_TOK '=' BRACEDTEXT_TOK  */
#line 2002 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[-2].id_ptr),(yyvsp[0].braced_ptr),0,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4226 "ascend/compiler/ascParse.c"
    break;

  case 138: /* unitladderitem: IDENTIFIER_TOK  */
#line 2007 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[0].id_ptr),NULL,1,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4235 "ascend/compiler/ascParse.c"
    break;

  case 139: /* methods: %empty  */
#line 2016 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 4243 "ascend/compiler/ascParse.c"
    break;

  case 140: /* $@2: %empty  */
#line 2020 "ascend/compiler/ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 4254 "ascend/compiler/ascParse.c"
    break;

  case 141: /* methods: METHODS_TOK $@2 proclist  */
#line 2027 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 4262 "ascend/compiler/ascParse.c"
    break;

  case 142: /* initial: %empty  */
#line 2034 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = EmptyStatementList();
	}
#line 4270 "ascend/compiler/ascParse.c"
    break;

  case 143: /* initial: INITIAL_TOK fstatements  */
#line 2038 "ascend/compiler/ascParse.y"
        {
	  AddContext((yyvsp[0].slptr),context_INITIAL);
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 4279 "ascend/compiler/ascParse.c"
    break;

  case 144: /* proclist: proclistf  */
#line 2046 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 4288 "ascend/compiler/ascParse.c"
    break;

  case 145: /* proclistf: %empty  */
#line 2053 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4296 "ascend/compiler/ascParse.c"
    break;

  case 146: /* proclistf: proclistf procedure  */
#line 2057 "ascend/compiler/ascParse.y"
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
#line 4322 "ascend/compiler/ascParse.c"
    break;

  case 147: /* procedure: procedure_id ';' fstatements end ';'  */
#line 2082 "ascend/compiler/ascParse.y"
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
#line 4337 "ascend/compiler/ascParse.c"
    break;

  case 148: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 2096 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 4346 "ascend/compiler/ascParse.c"
    break;

  case 149: /* fstatements: statements  */
#line 2105 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 4354 "ascend/compiler/ascParse.c"
    break;

  case 150: /* statements: %empty  */
#line 2112 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4362 "ascend/compiler/ascParse.c"
    break;

  case 151: /* statements: statements statement ';'  */
#line 2116 "ascend/compiler/ascParse.y"
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
#line 4438 "ascend/compiler/ascParse.c"
    break;

  case 152: /* statements: statements complex_statement ';'  */
#line 2188 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
            gl_destroy((yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4450 "ascend/compiler/ascParse.c"
    break;

  case 153: /* statements: statements error ';'  */
#line 2196 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4459 "ascend/compiler/ascParse.c"
    break;

  case 201: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 2259 "ascend/compiler/ascParse.y"
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
#line 4493 "ascend/compiler/ascParse.c"
    break;

  case 202: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 2292 "ascend/compiler/ascParse.y"
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
#line 4524 "ascend/compiler/ascParse.c"
    break;

  case 203: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 2322 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 4532 "ascend/compiler/ascParse.c"
    break;

  case 204: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 2327 "ascend/compiler/ascParse.y"
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
#line 4582 "ascend/compiler/ascParse.c"
    break;

  case 205: /* optional_set_values: %empty  */
#line 2376 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4590 "ascend/compiler/ascParse.c"
    break;

  case 206: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 2380 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 4598 "ascend/compiler/ascParse.c"
    break;

  case 207: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 2387 "ascend/compiler/ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 4611 "ascend/compiler/ascParse.c"
    break;

  case 208: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 2399 "ascend/compiler/ascParse.y"
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
#line 4639 "ascend/compiler/ascParse.c"
    break;

  case 209: /* call_identifier: IDENTIFIER_TOK  */
#line 2426 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 4648 "ascend/compiler/ascParse.c"
    break;

  case 210: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2431 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 4657 "ascend/compiler/ascParse.c"
    break;

  case 211: /* type_identifier: IDENTIFIER_TOK  */
#line 2439 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 4666 "ascend/compiler/ascParse.c"
    break;

  case 212: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2444 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 4675 "ascend/compiler/ascParse.c"
    break;

  case 213: /* optional_method: %empty  */
#line 2452 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4683 "ascend/compiler/ascParse.c"
    break;

  case 214: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 2456 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4691 "ascend/compiler/ascParse.c"
    break;

  case 215: /* optional_of: %empty  */
#line 2463 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4699 "ascend/compiler/ascParse.c"
    break;

  case 216: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 2467 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4707 "ascend/compiler/ascParse.c"
    break;

  case 217: /* optional_with_value: %empty  */
#line 2474 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 4715 "ascend/compiler/ascParse.c"
    break;

  case 218: /* optional_with_value: WITH_VALUE_T expr  */
#line 2478 "ascend/compiler/ascParse.y"
        {
	  g_decl_checkkind = ISCV_WITH_VALUE;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4724 "ascend/compiler/ascParse.c"
    break;

  case 219: /* optional_with_value: DEFAULT_TOK expr  */
#line 2483 "ascend/compiler/ascParse.y"
        {
	  g_decl_checkkind = ISCV_DEFAULT;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4733 "ascend/compiler/ascParse.c"
    break;

  case 220: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 2491 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 4741 "ascend/compiler/ascParse.c"
    break;

  case 221: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 2498 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4749 "ascend/compiler/ascParse.c"
    break;

  case 222: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2502 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4757 "ascend/compiler/ascParse.c"
    break;

  case 223: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 2506 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4765 "ascend/compiler/ascParse.c"
    break;

  case 224: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2513 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4773 "ascend/compiler/ascParse.c"
    break;

  case 225: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 2517 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4781 "ascend/compiler/ascParse.c"
    break;

  case 226: /* der_statement: DERLINK_TOK '(' fvarlist ')'  */
#line 2524 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 4791 "ascend/compiler/ascParse.c"
    break;

  case 227: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 2533 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 4801 "ascend/compiler/ascParse.c"
    break;

  case 228: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 2542 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 4809 "ascend/compiler/ascParse.c"
    break;

  case 229: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 2549 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 4817 "ascend/compiler/ascParse.c"
    break;

  case 230: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 2556 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 4825 "ascend/compiler/ascParse.c"
    break;

  case 231: /* assignment_statement: fvarref ASSIGN_TOK expr  */
#line 2563 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4833 "ascend/compiler/ascParse.c"
    break;

  case 232: /* assignment_statement: fvarref CASSIGN_TOK expr  */
#line 2567 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4841 "ascend/compiler/ascParse.c"
    break;

  case 233: /* reinit_statement: REINIT_TOK '(' fvarref ',' expr ')'  */
#line 2574 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateREINIT((yyvsp[-3].nptr),(yyvsp[-1].eptr));
	}
#line 4849 "ascend/compiler/ascParse.c"
    break;

  case 234: /* switchto_statement: SWITCH_TOK TO_TOK expr IF_TOK expr  */
#line 2581 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateSWITCHTO((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4857 "ascend/compiler/ascParse.c"
    break;

  case 235: /* relation_statement: relation  */
#line 2588 "ascend/compiler/ascParse.y"
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
#line 4874 "ascend/compiler/ascParse.c"
    break;

  case 236: /* relation_statement: fname ':' relation  */
#line 2601 "ascend/compiler/ascParse.y"
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
#line 4892 "ascend/compiler/ascParse.c"
    break;

  case 237: /* relation: expr  */
#line 2618 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 4906 "ascend/compiler/ascParse.c"
    break;

  case 238: /* relation: MINIMIZE_TOK expr  */
#line 2628 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4918 "ascend/compiler/ascParse.c"
    break;

  case 239: /* relation: MAXIMIZE_TOK expr  */
#line 2636 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4930 "ascend/compiler/ascParse.c"
    break;

  case 240: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 2647 "ascend/compiler/ascParse.y"
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
#line 4952 "ascend/compiler/ascParse.c"
    break;

  case 241: /* input_args: fvarlist ':' INPUT_TOK  */
#line 2668 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4960 "ascend/compiler/ascParse.c"
    break;

  case 242: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 2675 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4968 "ascend/compiler/ascParse.c"
    break;

  case 243: /* data_args: %empty  */
#line 2682 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 4976 "ascend/compiler/ascParse.c"
    break;

  case 244: /* data_args: ';' fname ':' DATA_TOK  */
#line 2686 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 4984 "ascend/compiler/ascParse.c"
    break;

  case 245: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 2724 "ascend/compiler/ascParse.y"
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
#line 5005 "ascend/compiler/ascParse.c"
    break;

  case 246: /* optional_direction: %empty  */
#line 2744 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 5013 "ascend/compiler/ascParse.c"
    break;

  case 247: /* optional_direction: INCREASING_TOK  */
#line 2748 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 5021 "ascend/compiler/ascParse.c"
    break;

  case 248: /* optional_direction: DECREASING_TOK  */
#line 2752 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 5029 "ascend/compiler/ascParse.c"
    break;

  case 249: /* forexprend: CREATE_TOK  */
#line 2759 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 5037 "ascend/compiler/ascParse.c"
    break;

  case 250: /* forexprend: EXPECT_TOK  */
#line 2763 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 5045 "ascend/compiler/ascParse.c"
    break;

  case 251: /* forexprend: CHECK_TOK  */
#line 2767 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 5053 "ascend/compiler/ascParse.c"
    break;

  case 252: /* forexprend: DO_TOK  */
#line 2771 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 5061 "ascend/compiler/ascParse.c"
    break;

  case 253: /* run_statement: RUN_TOK fname  */
#line 2778 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 5069 "ascend/compiler/ascParse.c"
    break;

  case 254: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 2782 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 5077 "ascend/compiler/ascParse.c"
    break;

  case 255: /* fix_statement: FIX_TOK method_fvarlist  */
#line 2789 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 5086 "ascend/compiler/ascParse.c"
    break;

  case 256: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 2797 "ascend/compiler/ascParse.y"
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
#line 5101 "ascend/compiler/ascParse.c"
    break;

  case 257: /* free_statement: FREE_TOK method_fvarlist  */
#line 2811 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 5109 "ascend/compiler/ascParse.c"
    break;

  case 258: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 2818 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 5118 "ascend/compiler/ascParse.c"
    break;

  case 259: /* integrator_statement: INTEGRATOR_TOK IDENTIFIER_TOK  */
#line 2826 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateINTEGRATOR(SCP((yyvsp[0].id_ptr)));
	}
#line 5126 "ascend/compiler/ascParse.c"
    break;

  case 260: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 2833 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 5135 "ascend/compiler/ascParse.c"
    break;

  case 261: /* solve_statement: SOLVE_TOK  */
#line 2841 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE(NULL);
	}
#line 5144 "ascend/compiler/ascParse.c"
    break;

  case 262: /* solve_statement: SOLVE_TOK fname  */
#line 2846 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSOLVE((yyvsp[0].nptr));
	}
#line 5152 "ascend/compiler/ascParse.c"
    break;

  case 263: /* integrate_statement: INTEGRATE_TOK FROM_TOK expr TO_TOK expr STEPS_TOK INTEGER_TOK  */
#line 2853 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateINTEGRATE((yyvsp[-4].eptr), (yyvsp[-2].eptr), (yyvsp[0].int_value));
	}
#line 5160 "ascend/compiler/ascParse.c"
    break;

  case 264: /* observe_statement: OBSERVE_TOK fvarlist observe_as_opt  */
#line 2860 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateOBSERVE((yyvsp[-1].lptr), (yyvsp[0].id_ptr));
	}
#line 5168 "ascend/compiler/ascParse.c"
    break;

  case 265: /* observe_as_opt: %empty  */
#line 2867 "ascend/compiler/ascParse.y"
        {
		(yyval.id_ptr) = NULL;
	}
#line 5176 "ascend/compiler/ascParse.c"
    break;

  case 266: /* observe_as_opt: AS_TOK IDENTIFIER_TOK  */
#line 2871 "ascend/compiler/ascParse.y"
        {
		(yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 5184 "ascend/compiler/ascParse.c"
    break;

  case 267: /* $@3: %empty  */
#line 2878 "ascend/compiler/ascParse.y"
        {
		g_study_parse = StudyParseEmpty();
		g_study_run_method = NULL;
		g_study_now = 0;
		g_study_filename = NULL;
	}
#line 5195 "ascend/compiler/ascParse.c"
    break;

  case 268: /* study_statement: STUDY_TOK $@3 study_obs_opt study_vary_opt study_run_opt study_now_opt study_file_opt  */
#line 2885 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSTUDY((yyvsp[-4].lptr), g_study_parse.vary, g_study_parse.lower, g_study_parse.upper,
			g_study_parse.steps, g_study_parse.value, g_study_parse.mode, g_study_parse.dist,
			g_study_run_method, g_study_now, g_study_filename);
	}
#line 5205 "ascend/compiler/ascParse.c"
    break;

  case 269: /* study_obs_opt: %empty  */
#line 2894 "ascend/compiler/ascParse.y"
        {
		(yyval.lptr) = NULL;
	}
#line 5213 "ascend/compiler/ascParse.c"
    break;

  case 270: /* study_obs_opt: fvarlist  */
#line 2898 "ascend/compiler/ascParse.y"
        {
		(yyval.lptr) = (yyvsp[0].lptr);
	}
#line 5221 "ascend/compiler/ascParse.c"
    break;

  case 271: /* study_vary_opt: %empty  */
#line 2905 "ascend/compiler/ascParse.y"
        {
	}
#line 5228 "ascend/compiler/ascParse.c"
    break;

  case 272: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEPS_TOK INTEGER_TOK study_distribution_opt  */
#line 2908 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-7].nptr);
		g_study_parse.lower = (yyvsp[-5].eptr);
		g_study_parse.upper = (yyvsp[-3].eptr);
		g_study_parse.steps = (yyvsp[-1].int_value);
		g_study_parse.mode = study_steps;
	}
#line 5240 "ascend/compiler/ascParse.c"
    break;

  case 273: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEP_TOK expr  */
#line 2916 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_step;
		g_study_parse.dist = study_dist_linear;
	}
#line 5253 "ascend/compiler/ascParse.c"
    break;

  case 274: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr RATIO_TOK expr  */
#line 2925 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_ratio;
		g_study_parse.dist = study_dist_log;
	}
#line 5266 "ascend/compiler/ascParse.c"
    break;

  case 275: /* study_distribution_opt: %empty  */
#line 2937 "ascend/compiler/ascParse.y"
        {
	}
#line 5273 "ascend/compiler/ascParse.c"
    break;

  case 276: /* study_distribution_opt: LINEAR_TOK  */
#line 2940 "ascend/compiler/ascParse.y"
        {
		g_study_parse.dist = study_dist_linear;
	}
#line 5281 "ascend/compiler/ascParse.c"
    break;

  case 277: /* study_distribution_opt: LOG_TOK  */
#line 2944 "ascend/compiler/ascParse.y"
        {
		g_study_parse.dist = study_dist_log;
	}
#line 5289 "ascend/compiler/ascParse.c"
    break;

  case 278: /* study_run_opt: %empty  */
#line 2951 "ascend/compiler/ascParse.y"
        {
	}
#line 5296 "ascend/compiler/ascParse.c"
    break;

  case 279: /* study_run_opt: RUN_TOK IDENTIFIER_TOK  */
#line 2954 "ascend/compiler/ascParse.y"
        {
		g_study_run_method = (yyvsp[0].id_ptr);
	}
#line 5304 "ascend/compiler/ascParse.c"
    break;

  case 280: /* study_file_opt: %empty  */
#line 2961 "ascend/compiler/ascParse.y"
        {
	}
#line 5311 "ascend/compiler/ascParse.c"
    break;

  case 281: /* study_file_opt: FILE_TOK DQUOTE_TOK  */
#line 2964 "ascend/compiler/ascParse.y"
        {
		g_study_filename = (yyvsp[0].dquote_ptr);
	}
#line 5319 "ascend/compiler/ascParse.c"
    break;

  case 282: /* study_now_opt: %empty  */
#line 2971 "ascend/compiler/ascParse.y"
        {
	}
#line 5326 "ascend/compiler/ascParse.c"
    break;

  case 283: /* study_now_opt: NOW_TOK  */
#line 2974 "ascend/compiler/ascParse.y"
        {
		g_study_now = 1;
	}
#line 5334 "ascend/compiler/ascParse.c"
    break;

  case 284: /* delete_statement: DELETE_TOK SYSTEM_TOK  */
#line 2981 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateDELETESYSTEM();
	}
#line 5342 "ascend/compiler/ascParse.c"
    break;

  case 285: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 2988 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 5354 "ascend/compiler/ascParse.c"
    break;

  case 286: /* call_statement: CALL_TOK call_identifier  */
#line 2999 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 5366 "ascend/compiler/ascParse.c"
    break;

  case 287: /* assert_statement: ASSERT_TOK expr  */
#line 3010 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 5374 "ascend/compiler/ascParse.c"
    break;

  case 288: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 3016 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 5385 "ascend/compiler/ascParse.c"
    break;

  case 289: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 3026 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 5396 "ascend/compiler/ascParse.c"
    break;

  case 290: /* optional_else: %empty  */
#line 3035 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 5404 "ascend/compiler/ascParse.c"
    break;

  case 291: /* optional_else: ELSE_TOK fstatements  */
#line 3039 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 5412 "ascend/compiler/ascParse.c"
    break;

  case 292: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 3046 "ascend/compiler/ascParse.y"
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
#line 5427 "ascend/compiler/ascParse.c"
    break;

  case 293: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 3057 "ascend/compiler/ascParse.y"
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
#line 5443 "ascend/compiler/ascParse.c"
    break;

  case 294: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 3069 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5454 "ascend/compiler/ascParse.c"
    break;

  case 295: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 3076 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5465 "ascend/compiler/ascParse.c"
    break;

  case 296: /* whenlist: whenlistf  */
#line 3086 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 5473 "ascend/compiler/ascParse.c"
    break;

  case 297: /* whenlistf: CASE_TOK set optional_when_case_if optional_when_case_applies ':' fstatements  */
#line 3093 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhenIfApplies((yyvsp[-4].sptr),(yyvsp[-3].eptr),(yyvsp[-2].eptr),(yyvsp[0].slptr));
	}
#line 5481 "ascend/compiler/ascParse.c"
    break;

  case 298: /* whenlistf: OTHERWISE_TOK ':' fstatements  */
#line 3097 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	}
#line 5489 "ascend/compiler/ascParse.c"
    break;

  case 299: /* whenlistf: whenlistf CASE_TOK set optional_when_case_if optional_when_case_applies ':' fstatements  */
#line 3101 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhenIfApplies((yyvsp[-4].sptr),(yyvsp[-3].eptr),(yyvsp[-2].eptr),(yyvsp[0].slptr)),(yyvsp[-6].wptr));
	}
#line 5497 "ascend/compiler/ascParse.c"
    break;

  case 300: /* whenlistf: whenlistf OTHERWISE_TOK ':' fstatements  */
#line 3105 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	}
#line 5505 "ascend/compiler/ascParse.c"
    break;

  case 301: /* optional_when_case_if: %empty  */
#line 3111 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 5513 "ascend/compiler/ascParse.c"
    break;

  case 302: /* optional_when_case_if: IF_TOK expr  */
#line 3115 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 5521 "ascend/compiler/ascParse.c"
    break;

  case 303: /* optional_when_case_applies: %empty  */
#line 3121 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 5529 "ascend/compiler/ascParse.c"
    break;

  case 304: /* optional_when_case_applies: APPLIES_TOK IF_TOK expr  */
#line 3125 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 5537 "ascend/compiler/ascParse.c"
    break;

  case 305: /* flow_statement: BREAK_TOK  */
#line 3132 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 5545 "ascend/compiler/ascParse.c"
    break;

  case 306: /* flow_statement: CONTINUE_TOK  */
#line 3136 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 5553 "ascend/compiler/ascParse.c"
    break;

  case 307: /* flow_statement: FALLTHRU_TOK  */
#line 3140 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 5561 "ascend/compiler/ascParse.c"
    break;

  case 308: /* flow_statement: RETURN_TOK  */
#line 3144 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 5569 "ascend/compiler/ascParse.c"
    break;

  case 309: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 3148 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 5577 "ascend/compiler/ascParse.c"
    break;

  case 310: /* use_statement: USE_TOK fname  */
#line 3155 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 5585 "ascend/compiler/ascParse.c"
    break;

  case 311: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 3162 "ascend/compiler/ascParse.y"
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
#line 5600 "ascend/compiler/ascParse.c"
    break;

  case 312: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 3173 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 5611 "ascend/compiler/ascParse.c"
    break;

  case 313: /* selectlist: selectlistf  */
#line 3183 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 5619 "ascend/compiler/ascParse.c"
    break;

  case 314: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 3190 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5627 "ascend/compiler/ascParse.c"
    break;

  case 315: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 3194 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 5635 "ascend/compiler/ascParse.c"
    break;

  case 316: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 3198 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 5643 "ascend/compiler/ascParse.c"
    break;

  case 317: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 3202 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 5651 "ascend/compiler/ascParse.c"
    break;

  case 318: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 3209 "ascend/compiler/ascParse.y"
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
#line 5666 "ascend/compiler/ascParse.c"
    break;

  case 319: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 3220 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 5677 "ascend/compiler/ascParse.c"
    break;

  case 320: /* switchlist: switchlistf  */
#line 3230 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 5685 "ascend/compiler/ascParse.c"
    break;

  case 321: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 3237 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5693 "ascend/compiler/ascParse.c"
    break;

  case 322: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 3241 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 5701 "ascend/compiler/ascParse.c"
    break;

  case 323: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 3245 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 5709 "ascend/compiler/ascParse.c"
    break;

  case 324: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 3249 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 5717 "ascend/compiler/ascParse.c"
    break;

  case 325: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 3256 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 5728 "ascend/compiler/ascParse.c"
    break;

  case 326: /* notes_statement: NOTES_TOK notes_body end  */
#line 3266 "ascend/compiler/ascParse.y"
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
#line 5766 "ascend/compiler/ascParse.c"
    break;

  case 327: /* notes_body: SYMBOL_TOK noteslist  */
#line 3303 "ascend/compiler/ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 5779 "ascend/compiler/ascParse.c"
    break;

  case 328: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 3312 "ascend/compiler/ascParse.y"
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
#line 5795 "ascend/compiler/ascParse.c"
    break;

  case 329: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 3327 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 5804 "ascend/compiler/ascParse.c"
    break;

  case 330: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 3332 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 5814 "ascend/compiler/ascParse.c"
    break;

  case 331: /* fvarlist: varlist  */
#line 3341 "ascend/compiler/ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5826 "ascend/compiler/ascParse.c"
    break;

  case 332: /* varlist: fname  */
#line 3352 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5834 "ascend/compiler/ascParse.c"
    break;

  case 333: /* varlist: varlist ',' fname  */
#line 3356 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5843 "ascend/compiler/ascParse.c"
    break;

  case 334: /* varlist: varlist fname  */
#line 3361 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 5857 "ascend/compiler/ascParse.c"
    break;

  case 335: /* method_fvarlist: method_varlist  */
#line 3374 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5865 "ascend/compiler/ascParse.c"
    break;

  case 336: /* method_varlist: fvarref  */
#line 3381 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5873 "ascend/compiler/ascParse.c"
    break;

  case 337: /* method_varlist: method_varlist ',' fvarref  */
#line 3385 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5882 "ascend/compiler/ascParse.c"
    break;

  case 338: /* method_varlist: method_varlist fvarref  */
#line 3390 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  g_untrapped_error++;
	}
#line 5893 "ascend/compiler/ascParse.c"
    break;

  case 339: /* fvarref: fname  */
#line 3400 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[0].nptr);
	}
#line 5901 "ascend/compiler/ascParse.c"
    break;

  case 340: /* fvarref: DERIV_TOK '(' fname ')'  */
#line 3404 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateDerivativeRefName((yyvsp[-1].nptr));
	}
#line 5909 "ascend/compiler/ascParse.c"
    break;

  case 341: /* fname: name optional_notes  */
#line 3411 "ascend/compiler/ascParse.y"
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
#line 5930 "ascend/compiler/ascParse.c"
    break;

  case 342: /* name: IDENTIFIER_TOK  */
#line 3431 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 5938 "ascend/compiler/ascParse.c"
    break;

  case 343: /* name: name '.' IDENTIFIER_TOK  */
#line 3435 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5947 "ascend/compiler/ascParse.c"
    break;

  case 344: /* name: name '.' DERIV_TOK  */
#line 3440 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName(AddSymbol("der"));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5956 "ascend/compiler/ascParse.c"
    break;

  case 345: /* name: name '[' set ']'  */
#line 3445 "ascend/compiler/ascParse.y"
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
#line 5972 "ascend/compiler/ascParse.c"
    break;

  case 346: /* end: END_TOK CONDITIONAL_TOK  */
#line 3460 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 5981 "ascend/compiler/ascParse.c"
    break;

  case 347: /* end: END_TOK FOR_TOK  */
#line 3465 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 5990 "ascend/compiler/ascParse.c"
    break;

  case 348: /* end: END_TOK IF_TOK  */
#line 3470 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 5999 "ascend/compiler/ascParse.c"
    break;

  case 349: /* end: END_TOK INTERACTIVE_TOK  */
#line 3475 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 6008 "ascend/compiler/ascParse.c"
    break;

  case 350: /* end: END_TOK METHODS_TOK  */
#line 3480 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 6017 "ascend/compiler/ascParse.c"
    break;

  case 351: /* end: END_TOK NOTES_TOK  */
#line 3485 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 6026 "ascend/compiler/ascParse.c"
    break;

  case 352: /* end: END_TOK SELECT_TOK  */
#line 3490 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 6035 "ascend/compiler/ascParse.c"
    break;

  case 353: /* end: END_TOK SWITCH_TOK  */
#line 3495 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 6044 "ascend/compiler/ascParse.c"
    break;

  case 354: /* end: END_TOK UNITS_TOK  */
#line 3500 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 6053 "ascend/compiler/ascParse.c"
    break;

  case 355: /* end: END_TOK GLOBAL_TOK  */
#line 3505 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 6062 "ascend/compiler/ascParse.c"
    break;

  case 356: /* end: END_TOK WHEN_TOK  */
#line 3510 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 6071 "ascend/compiler/ascParse.c"
    break;

  case 357: /* end: END_TOK WHILE_TOK  */
#line 3515 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 6080 "ascend/compiler/ascParse.c"
    break;

  case 358: /* end: END_TOK IDENTIFIER_TOK  */
#line 3520 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 6089 "ascend/compiler/ascParse.c"
    break;

  case 359: /* end: END_TOK  */
#line 3525 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 6098 "ascend/compiler/ascParse.c"
    break;

  case 360: /* optional_bracedtext: %empty  */
#line 3533 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 6106 "ascend/compiler/ascParse.c"
    break;

  case 361: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 3537 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 6114 "ascend/compiler/ascParse.c"
    break;

  case 362: /* optional_notes: %empty  */
#line 3544 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 6122 "ascend/compiler/ascParse.c"
    break;

  case 363: /* optional_notes: DQUOTE_TOK  */
#line 3548 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 6130 "ascend/compiler/ascParse.c"
    break;

  case 364: /* set: setexprlist  */
#line 3555 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 6138 "ascend/compiler/ascParse.c"
    break;

  case 365: /* set: %empty  */
#line 3559 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 6146 "ascend/compiler/ascParse.c"
    break;

  case 366: /* setexprlist: expr  */
#line 3566 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 6154 "ascend/compiler/ascParse.c"
    break;

  case 367: /* setexprlist: '*'  */
#line 3570 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet(CreateAnyExpr());
	}
#line 6162 "ascend/compiler/ascParse.c"
    break;

  case 368: /* setexprlist: expr DOTDOT_TOK expr  */
#line 3574 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6170 "ascend/compiler/ascParse.c"
    break;

  case 369: /* setexprlist: setexprlist ',' expr  */
#line 3578 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 6179 "ascend/compiler/ascParse.c"
    break;

  case 370: /* setexprlist: setexprlist ',' '*'  */
#line 3583 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet(CreateAnyExpr());
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 6188 "ascend/compiler/ascParse.c"
    break;

  case 371: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 3588 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 6197 "ascend/compiler/ascParse.c"
    break;

  case 372: /* number: INTEGER_TOK  */
#line 3596 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	  g_number_units = NULL;
	}
#line 6208 "ascend/compiler/ascParse.c"
    break;

  case 373: /* number: realnumber  */
#line 3603 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 6218 "ascend/compiler/ascParse.c"
    break;

  case 374: /* realnumber: REAL_TOK opunits  */
#line 3612 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	  g_number_units = g_parsed_units;
	}
#line 6227 "ascend/compiler/ascParse.c"
    break;

  case 375: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 3617 "ascend/compiler/ascParse.y"
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
#line 6252 "ascend/compiler/ascParse.c"
    break;

  case 376: /* opunits: %empty  */
#line 3641 "ascend/compiler/ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  g_parsed_units = NULL;
	  (yyval.real_value) = 1.0;
	}
#line 6262 "ascend/compiler/ascParse.c"
    break;

  case 377: /* opunits: BRACEDTEXT_TOK  */
#line 3647 "ascend/compiler/ascParse.y"
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
#line 6286 "ascend/compiler/ascParse.c"
    break;

  case 378: /* dims: DIMENSION_TOK dimensions  */
#line 3670 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 6294 "ascend/compiler/ascParse.c"
    break;

  case 379: /* dims: DIMENSIONLESS_TOK  */
#line 3674 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 6302 "ascend/compiler/ascParse.c"
    break;

  case 380: /* dims: %empty  */
#line 3678 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 6310 "ascend/compiler/ascParse.c"
    break;

  case 381: /* dimensions: '*'  */
#line 3685 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 6318 "ascend/compiler/ascParse.c"
    break;

  case 382: /* dimensions: dimexpr  */
#line 3689 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 6326 "ascend/compiler/ascParse.c"
    break;

  case 383: /* dimexpr: IDENTIFIER_TOK  */
#line 3696 "ascend/compiler/ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 6334 "ascend/compiler/ascParse.c"
    break;

  case 384: /* dimexpr: INTEGER_TOK  */
#line 3700 "ascend/compiler/ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 6342 "ascend/compiler/ascParse.c"
    break;

  case 385: /* dimexpr: dimexpr '/' dimexpr  */
#line 3704 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6350 "ascend/compiler/ascParse.c"
    break;

  case 386: /* dimexpr: dimexpr '*' dimexpr  */
#line 3708 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6358 "ascend/compiler/ascParse.c"
    break;

  case 387: /* dimexpr: dimexpr '^' fraction  */
#line 3712 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 6366 "ascend/compiler/ascParse.c"
    break;

  case 388: /* dimexpr: '(' dimexpr ')'  */
#line 3716 "ascend/compiler/ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 6374 "ascend/compiler/ascParse.c"
    break;

  case 389: /* fraction: optional_sign fractail  */
#line 3723 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 6382 "ascend/compiler/ascParse.c"
    break;

  case 390: /* fractail: INTEGER_TOK  */
#line 3730 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 6390 "ascend/compiler/ascParse.c"
    break;

  case 391: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 3734 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 6398 "ascend/compiler/ascParse.c"
    break;

  case 392: /* optional_sign: %empty  */
#line 3741 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6406 "ascend/compiler/ascParse.c"
    break;

  case 393: /* optional_sign: '+'  */
#line 3745 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6414 "ascend/compiler/ascParse.c"
    break;

  case 394: /* optional_sign: '-'  */
#line 3749 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 6422 "ascend/compiler/ascParse.c"
    break;

  case 395: /* expr: INTEGER_TOK  */
#line 3756 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 6430 "ascend/compiler/ascParse.c"
    break;

  case 396: /* expr: MAXINTEGER_TOK  */
#line 3760 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 6438 "ascend/compiler/ascParse.c"
    break;

  case 397: /* expr: realnumber  */
#line 3764 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 6446 "ascend/compiler/ascParse.c"
    break;

  case 398: /* expr: MAXREAL_TOK  */
#line 3768 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 6454 "ascend/compiler/ascParse.c"
    break;

  case 399: /* expr: TRUE_TOK  */
#line 3772 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 6462 "ascend/compiler/ascParse.c"
    break;

  case 400: /* expr: FALSE_TOK  */
#line 3776 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 6470 "ascend/compiler/ascParse.c"
    break;

  case 401: /* expr: ANY_TOK  */
#line 3780 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 6478 "ascend/compiler/ascParse.c"
    break;

  case 402: /* expr: SYMBOL_TOK  */
#line 3784 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 6486 "ascend/compiler/ascParse.c"
    break;

  case 403: /* expr: fname  */
#line 3788 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 6494 "ascend/compiler/ascParse.c"
    break;

  case 404: /* expr: DERIV_TOK '(' fname ')'  */
#line 3792 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateDiffExpr((yyvsp[-1].nptr));
	}
#line 6502 "ascend/compiler/ascParse.c"
    break;

  case 405: /* expr: '[' set ']'  */
#line 3796 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 6510 "ascend/compiler/ascParse.c"
    break;

  case 406: /* expr: expr '+' expr  */
#line 3800 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6519 "ascend/compiler/ascParse.c"
    break;

  case 407: /* expr: expr '-' expr  */
#line 3805 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6528 "ascend/compiler/ascParse.c"
    break;

  case 408: /* expr: expr '*' expr  */
#line 3810 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6537 "ascend/compiler/ascParse.c"
    break;

  case 409: /* expr: expr '/' expr  */
#line 3815 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6546 "ascend/compiler/ascParse.c"
    break;

  case 410: /* expr: expr '^' expr  */
#line 3820 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6555 "ascend/compiler/ascParse.c"
    break;

  case 411: /* expr: expr AND_TOK expr  */
#line 3825 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6564 "ascend/compiler/ascParse.c"
    break;

  case 412: /* expr: expr OR_TOK expr  */
#line 3830 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6573 "ascend/compiler/ascParse.c"
    break;

  case 413: /* expr: NOT_TOK expr  */
#line 3835 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 6581 "ascend/compiler/ascParse.c"
    break;

  case 414: /* expr: expr relop expr  */
#line 3839 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6590 "ascend/compiler/ascParse.c"
    break;

  case 415: /* expr: expr logrelop expr  */
#line 3844 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6599 "ascend/compiler/ascParse.c"
    break;

  case 416: /* expr: expr IN_TOK expr  */
#line 3849 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6608 "ascend/compiler/ascParse.c"
    break;

  case 417: /* expr: expr '|' expr  */
#line 3854 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6617 "ascend/compiler/ascParse.c"
    break;

  case 418: /* expr: expr SUCHTHAT_TOK expr  */
#line 3859 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6626 "ascend/compiler/ascParse.c"
    break;

  case 419: /* expr: '+' expr  */
#line 3864 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 6634 "ascend/compiler/ascParse.c"
    break;

  case 420: /* expr: '-' expr  */
#line 3868 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 6642 "ascend/compiler/ascParse.c"
    break;

  case 421: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 3872 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 6650 "ascend/compiler/ascParse.c"
    break;

  case 422: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 3876 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 6658 "ascend/compiler/ascParse.c"
    break;

  case 423: /* expr: SUM_TOK '(' set ')'  */
#line 3880 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 6669 "ascend/compiler/ascParse.c"
    break;

  case 424: /* expr: SUM_TOK '[' set ']'  */
#line 3887 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 6677 "ascend/compiler/ascParse.c"
    break;

  case 425: /* expr: PROD_TOK '(' set ')'  */
#line 3891 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 6688 "ascend/compiler/ascParse.c"
    break;

  case 426: /* expr: PROD_TOK '[' set ']'  */
#line 3898 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 6696 "ascend/compiler/ascParse.c"
    break;

  case 427: /* expr: UNION_TOK '(' set ')'  */
#line 3902 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 6707 "ascend/compiler/ascParse.c"
    break;

  case 428: /* expr: UNION_TOK '[' set ']'  */
#line 3909 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 6715 "ascend/compiler/ascParse.c"
    break;

  case 429: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 3913 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 6726 "ascend/compiler/ascParse.c"
    break;

  case 430: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 3920 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 6734 "ascend/compiler/ascParse.c"
    break;

  case 431: /* expr: CARD_TOK '(' set ')'  */
#line 3924 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 6745 "ascend/compiler/ascParse.c"
    break;

  case 432: /* expr: CARD_TOK '[' set ']'  */
#line 3931 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 6753 "ascend/compiler/ascParse.c"
    break;

  case 433: /* expr: CHOICE_TOK '(' set ')'  */
#line 3935 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 6764 "ascend/compiler/ascParse.c"
    break;

  case 434: /* expr: CHOICE_TOK '[' set ']'  */
#line 3942 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 6772 "ascend/compiler/ascParse.c"
    break;

  case 435: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 3946 "ascend/compiler/ascParse.y"
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
#line 6798 "ascend/compiler/ascParse.c"
    break;

  case 436: /* expr: '(' expr ')'  */
#line 3968 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 6806 "ascend/compiler/ascParse.c"
    break;

  case 437: /* relop: '='  */
#line 3975 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 6814 "ascend/compiler/ascParse.c"
    break;

  case 438: /* relop: '<'  */
#line 3979 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 6822 "ascend/compiler/ascParse.c"
    break;

  case 439: /* relop: '>'  */
#line 3983 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 6830 "ascend/compiler/ascParse.c"
    break;

  case 440: /* relop: LEQ_TOK  */
#line 3987 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 6838 "ascend/compiler/ascParse.c"
    break;

  case 441: /* relop: GEQ_TOK  */
#line 3991 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 6846 "ascend/compiler/ascParse.c"
    break;

  case 442: /* relop: NEQ_TOK  */
#line 3995 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 6854 "ascend/compiler/ascParse.c"
    break;

  case 443: /* logrelop: BEQ_TOK  */
#line 4002 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 6862 "ascend/compiler/ascParse.c"
    break;

  case 444: /* logrelop: BNE_TOK  */
#line 4006 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 6870 "ascend/compiler/ascParse.c"
    break;


#line 6874 "ascend/compiler/ascParse.c"

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

#line 4010 "ascend/compiler/ascParse.y"

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
  case APPLIES_TOK:
    return "APPLIES";
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
