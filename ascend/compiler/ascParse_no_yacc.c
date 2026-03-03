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
#line 29 "ascParse.y"

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

/* For 'inline' notes, note on DQUOTE_TOK from scanner.l:
 * Remember that DQUOTE_TOK is a string value which is local to the
 * production that finds it. It must be copied if you want to
 * keep it.
 */

/* MS VC++ won't compiler Bison output unless we switch this */
#ifdef _MSC_VER
# define __STDC__
#endif


#line 753 "ascParse.c"

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
  YYSYMBOL_LADDER_TOK = 97,                /* LADDER_TOK  */
  YYSYMBOL_UNIVERSAL_TOK = 98,             /* UNIVERSAL_TOK  */
  YYSYMBOL_UNLINK_TOK = 99,                /* UNLINK_TOK  */
  YYSYMBOL_WHEN_TOK = 100,                 /* WHEN_TOK  */
  YYSYMBOL_WHERE_TOK = 101,                /* WHERE_TOK  */
  YYSYMBOL_WHILE_TOK = 102,                /* WHILE_TOK  */
  YYSYMBOL_WILLBE_TOK = 103,               /* WILLBE_TOK  */
  YYSYMBOL_WILLBETHESAME_TOK = 104,        /* WILLBETHESAME_TOK  */
  YYSYMBOL_WILLNOTBETHESAME_TOK = 105,     /* WILLNOTBETHESAME_TOK  */
  YYSYMBOL_ASSIGN_TOK = 106,               /* ASSIGN_TOK  */
  YYSYMBOL_CASSIGN_TOK = 107,              /* CASSIGN_TOK  */
  YYSYMBOL_DBLCOLON_TOK = 108,             /* DBLCOLON_TOK  */
  YYSYMBOL_USE_TOK = 109,                  /* USE_TOK  */
  YYSYMBOL_LEQ_TOK = 110,                  /* LEQ_TOK  */
  YYSYMBOL_GEQ_TOK = 111,                  /* GEQ_TOK  */
  YYSYMBOL_NEQ_TOK = 112,                  /* NEQ_TOK  */
  YYSYMBOL_DOTDOT_TOK = 113,               /* DOTDOT_TOK  */
  YYSYMBOL_WITH_TOK = 114,                 /* WITH_TOK  */
  YYSYMBOL_VALUE_TOK = 115,                /* VALUE_TOK  */
  YYSYMBOL_WITH_VALUE_T = 116,             /* WITH_VALUE_T  */
  YYSYMBOL_REAL_TOK = 117,                 /* REAL_TOK  */
  YYSYMBOL_INTEGER_TOK = 118,              /* INTEGER_TOK  */
  YYSYMBOL_IDENTIFIER_TOK = 119,           /* IDENTIFIER_TOK  */
  YYSYMBOL_BRACEDTEXT_TOK = 120,           /* BRACEDTEXT_TOK  */
  YYSYMBOL_SYMBOL_TOK = 121,               /* SYMBOL_TOK  */
  YYSYMBOL_DQUOTE_TOK = 122,               /* DQUOTE_TOK  */
  YYSYMBOL_123_ = 123,                     /* ','  */
  YYSYMBOL_124_ = 124,                     /* '|'  */
  YYSYMBOL_125_ = 125,                     /* '<'  */
  YYSYMBOL_126_ = 126,                     /* '='  */
  YYSYMBOL_127_ = 127,                     /* '>'  */
  YYSYMBOL_128_ = 128,                     /* '+'  */
  YYSYMBOL_129_ = 129,                     /* '-'  */
  YYSYMBOL_130_ = 130,                     /* '/'  */
  YYSYMBOL_131_ = 131,                     /* '*'  */
  YYSYMBOL_UMINUS_TOK = 132,               /* UMINUS_TOK  */
  YYSYMBOL_UPLUS_TOK = 133,                /* UPLUS_TOK  */
  YYSYMBOL_134_ = 134,                     /* '^'  */
  YYSYMBOL_135_ = 135,                     /* ';'  */
  YYSYMBOL_136_ = 136,                     /* '('  */
  YYSYMBOL_137_ = 137,                     /* ')'  */
  YYSYMBOL_138_ = 138,                     /* ':'  */
  YYSYMBOL_139_ = 139,                     /* '['  */
  YYSYMBOL_140_ = 140,                     /* ']'  */
  YYSYMBOL_141_ = 141,                     /* '.'  */
  YYSYMBOL_YYACCEPT = 142,                 /* $accept  */
  YYSYMBOL_definitions = 143,              /* definitions  */
  YYSYMBOL_definition = 144,               /* definition  */
  YYSYMBOL_global_def = 145,               /* global_def  */
  YYSYMBOL_require_file = 146,             /* require_file  */
  YYSYMBOL_provide_module = 147,           /* provide_module  */
  YYSYMBOL_import = 148,                   /* import  */
  YYSYMBOL_add_notes_def = 149,            /* add_notes_def  */
  YYSYMBOL_add_notes_head = 150,           /* add_notes_head  */
  YYSYMBOL_add_method_def = 151,           /* add_method_def  */
  YYSYMBOL_add_method_head = 152,          /* add_method_head  */
  YYSYMBOL_replace_method_def = 153,       /* replace_method_def  */
  YYSYMBOL_replace_method_head = 154,      /* replace_method_head  */
  YYSYMBOL_atom_def = 155,                 /* atom_def  */
  YYSYMBOL_atom_head = 156,                /* atom_head  */
  YYSYMBOL_atom_id = 157,                  /* atom_id  */
  YYSYMBOL_default_val = 158,              /* default_val  */
  YYSYMBOL_constant_def = 159,             /* constant_def  */
  YYSYMBOL_constant_head = 160,            /* constant_head  */
  YYSYMBOL_constant_dims = 161,            /* constant_dims  */
  YYSYMBOL_constant_val = 162,             /* constant_val  */
  YYSYMBOL_model_def = 163,                /* model_def  */
  YYSYMBOL_model_head = 164,               /* model_head  */
  YYSYMBOL_model_id = 165,                 /* model_id  */
  YYSYMBOL_optional_model_parameters = 166, /* optional_model_parameters  */
  YYSYMBOL_optional_parameter_wheres = 167, /* optional_parameter_wheres  */
  YYSYMBOL_optional_parameter_reduction = 168, /* optional_parameter_reduction  */
  YYSYMBOL_universal = 169,                /* universal  */
  YYSYMBOL_definition_def = 170,           /* definition_def  */
  YYSYMBOL_definition_id = 171,            /* definition_id  */
  YYSYMBOL_units_def = 172,                /* units_def  */
  YYSYMBOL_units_statement = 173,          /* units_statement  */
  YYSYMBOL_optional_ladder_end = 174,      /* optional_ladder_end  */
  YYSYMBOL_table_statement = 175,          /* table_statement  */
  YYSYMBOL_table_begin = 176,              /* table_begin  */
  YYSYMBOL_table_mode_on = 177,            /* table_mode_on  */
  YYSYMBOL_table_mode_off = 178,           /* table_mode_off  */
  YYSYMBOL_table_options = 179,            /* table_options  */
  YYSYMBOL_table_decl_opt = 180,           /* table_decl_opt  */
  YYSYMBOL_table_option = 181,             /* table_option  */
  YYSYMBOL_table_body = 182,               /* table_body  */
  YYSYMBOL_table_body_item = 183,          /* table_body_item  */
  YYSYMBOL_table_scalar = 184,             /* table_scalar  */
  YYSYMBOL_values_statement = 185,         /* values_statement  */
  YYSYMBOL_values_default_opt = 186,       /* values_default_opt  */
  YYSYMBOL_values_entries = 187,           /* values_entries  */
  YYSYMBOL_values_entry = 188,             /* values_entry  */
  YYSYMBOL_values_key_list = 189,          /* values_key_list  */
  YYSYMBOL_values_key = 190,               /* values_key  */
  YYSYMBOL_dataset_statement = 191,        /* dataset_statement  */
  YYSYMBOL_192_1 = 192,                    /* $@1  */
  YYSYMBOL_dataset_items = 193,            /* dataset_items  */
  YYSYMBOL_dataset_item = 194,             /* dataset_item  */
  YYSYMBOL_dataset_index_item = 195,       /* dataset_index_item  */
  YYSYMBOL_dataset_map_item = 196,         /* dataset_map_item  */
  YYSYMBOL_dataset_column_selector = 197,  /* dataset_column_selector  */
  YYSYMBOL_dataset_column_ref = 198,       /* dataset_column_ref  */
  YYSYMBOL_dataset_target = 199,           /* dataset_target  */
  YYSYMBOL_dataset_units_opt = 200,        /* dataset_units_opt  */
  YYSYMBOL_dataset_type_opt = 201,         /* dataset_type_opt  */
  YYSYMBOL_dataset_type_req = 202,         /* dataset_type_req  */
  YYSYMBOL_unitdeflist = 203,              /* unitdeflist  */
  YYSYMBOL_unitdef = 204,                  /* unitdef  */
  YYSYMBOL_unitladderitemlist = 205,       /* unitladderitemlist  */
  YYSYMBOL_unitladderitem = 206,           /* unitladderitem  */
  YYSYMBOL_methods = 207,                  /* methods  */
  YYSYMBOL_208_2 = 208,                    /* $@2  */
  YYSYMBOL_proclist = 209,                 /* proclist  */
  YYSYMBOL_proclistf = 210,                /* proclistf  */
  YYSYMBOL_procedure = 211,                /* procedure  */
  YYSYMBOL_procedure_id = 212,             /* procedure_id  */
  YYSYMBOL_fstatements = 213,              /* fstatements  */
  YYSYMBOL_statements = 214,               /* statements  */
  YYSYMBOL_statement = 215,                /* statement  */
  YYSYMBOL_complex_statement = 216,        /* complex_statement  */
  YYSYMBOL_isa_statement = 217,            /* isa_statement  */
  YYSYMBOL_willbe_statement = 218,         /* willbe_statement  */
  YYSYMBOL_aliases_statement = 219,        /* aliases_statement  */
  YYSYMBOL_optional_set_values = 220,      /* optional_set_values  */
  YYSYMBOL_is_statement = 221,             /* is_statement  */
  YYSYMBOL_isrefinedto_statement = 222,    /* isrefinedto_statement  */
  YYSYMBOL_call_identifier = 223,          /* call_identifier  */
  YYSYMBOL_type_identifier = 224,          /* type_identifier  */
  YYSYMBOL_optional_method = 225,          /* optional_method  */
  YYSYMBOL_optional_of = 226,              /* optional_of  */
  YYSYMBOL_optional_with_value = 227,      /* optional_with_value  */
  YYSYMBOL_arealike_statement = 228,       /* arealike_statement  */
  YYSYMBOL_link_statement = 229,           /* link_statement  */
  YYSYMBOL_unlink_statement = 230,         /* unlink_statement  */
  YYSYMBOL_der_statement = 231,            /* der_statement  */
  YYSYMBOL_independent_statement = 232,    /* independent_statement  */
  YYSYMBOL_arethesame_statement = 233,     /* arethesame_statement  */
  YYSYMBOL_willbethesame_statement = 234,  /* willbethesame_statement  */
  YYSYMBOL_willnotbethesame_statement = 235, /* willnotbethesame_statement  */
  YYSYMBOL_assignment_statement = 236,     /* assignment_statement  */
  YYSYMBOL_relation_statement = 237,       /* relation_statement  */
  YYSYMBOL_relation = 238,                 /* relation  */
  YYSYMBOL_blackbox_statement = 239,       /* blackbox_statement  */
  YYSYMBOL_input_args = 240,               /* input_args  */
  YYSYMBOL_output_args = 241,              /* output_args  */
  YYSYMBOL_data_args = 242,                /* data_args  */
  YYSYMBOL_for_statement = 243,            /* for_statement  */
  YYSYMBOL_optional_direction = 244,       /* optional_direction  */
  YYSYMBOL_forexprend = 245,               /* forexprend  */
  YYSYMBOL_run_statement = 246,            /* run_statement  */
  YYSYMBOL_fix_statement = 247,            /* fix_statement  */
  YYSYMBOL_fix_and_assign_statement = 248, /* fix_and_assign_statement  */
  YYSYMBOL_free_statement = 249,           /* free_statement  */
  YYSYMBOL_solver_statement = 250,         /* solver_statement  */
  YYSYMBOL_option_statement = 251,         /* option_statement  */
  YYSYMBOL_solve_statement = 252,          /* solve_statement  */
  YYSYMBOL_external_statement = 253,       /* external_statement  */
  YYSYMBOL_call_statement = 254,           /* call_statement  */
  YYSYMBOL_assert_statement = 255,         /* assert_statement  */
  YYSYMBOL_if_statement = 256,             /* if_statement  */
  YYSYMBOL_while_statement = 257,          /* while_statement  */
  YYSYMBOL_optional_else = 258,            /* optional_else  */
  YYSYMBOL_when_statement = 259,           /* when_statement  */
  YYSYMBOL_whenlist = 260,                 /* whenlist  */
  YYSYMBOL_whenlistf = 261,                /* whenlistf  */
  YYSYMBOL_flow_statement = 262,           /* flow_statement  */
  YYSYMBOL_use_statement = 263,            /* use_statement  */
  YYSYMBOL_select_statement = 264,         /* select_statement  */
  YYSYMBOL_selectlist = 265,               /* selectlist  */
  YYSYMBOL_selectlistf = 266,              /* selectlistf  */
  YYSYMBOL_switch_statement = 267,         /* switch_statement  */
  YYSYMBOL_switchlist = 268,               /* switchlist  */
  YYSYMBOL_switchlistf = 269,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 270,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 271,          /* notes_statement  */
  YYSYMBOL_notes_body = 272,               /* notes_body  */
  YYSYMBOL_noteslist = 273,                /* noteslist  */
  YYSYMBOL_fvarlist = 274,                 /* fvarlist  */
  YYSYMBOL_varlist = 275,                  /* varlist  */
  YYSYMBOL_fname = 276,                    /* fname  */
  YYSYMBOL_name = 277,                     /* name  */
  YYSYMBOL_end = 278,                      /* end  */
  YYSYMBOL_optional_bracedtext = 279,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 280,           /* optional_notes  */
  YYSYMBOL_set = 281,                      /* set  */
  YYSYMBOL_setexprlist = 282,              /* setexprlist  */
  YYSYMBOL_number = 283,                   /* number  */
  YYSYMBOL_realnumber = 284,               /* realnumber  */
  YYSYMBOL_opunits = 285,                  /* opunits  */
  YYSYMBOL_dims = 286,                     /* dims  */
  YYSYMBOL_dimensions = 287,               /* dimensions  */
  YYSYMBOL_dimexpr = 288,                  /* dimexpr  */
  YYSYMBOL_fraction = 289,                 /* fraction  */
  YYSYMBOL_fractail = 290,                 /* fractail  */
  YYSYMBOL_optional_sign = 291,            /* optional_sign  */
  YYSYMBOL_expr = 292,                     /* expr  */
  YYSYMBOL_relop = 293,                    /* relop  */
  YYSYMBOL_logrelop = 294                  /* logrelop  */
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
#define YYLAST   1205

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  142
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  153
/* YYNRULES -- Number of rules.  */
#define YYNRULES  394
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  775

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   379


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
     136,   137,   131,   128,   123,   129,   141,   130,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   138,   135,
     125,   126,   127,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   139,     2,   140,   134,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   124,     2,     2,     2,     2,     2,
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
     115,   116,   117,   118,   119,   120,   121,   122,   132,   133
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   836,   836,   838,   842,   843,   844,   845,   846,   847,
     848,   849,   850,   851,   853,   854,   855,   866,   911,   915,
     920,   928,   932,   937,   945,   954,   966,   997,  1005,  1028,
    1041,  1049,  1070,  1082,  1090,  1143,  1154,  1163,  1169,  1175,
    1183,  1191,  1202,  1242,  1276,  1281,  1287,  1291,  1313,  1319,
    1325,  1333,  1341,  1352,  1393,  1402,  1415,  1424,  1427,  1435,
    1438,  1446,  1449,  1500,  1503,  1510,  1540,  1549,  1554,  1571,
    1597,  1600,  1607,  1628,  1641,  1648,  1655,  1660,  1662,  1667,
    1679,  1692,  1696,  1703,  1712,  1714,  1718,  1719,  1723,  1727,
    1731,  1735,  1739,  1743,  1750,  1754,  1758,  1762,  1766,  1773,
    1780,  1782,  1789,  1790,  1794,  1801,  1802,  1806,  1807,  1808,
    1809,  1814,  1813,  1823,  1825,  1826,  1836,  1837,  1841,  1848,
    1852,  1859,  1863,  1870,  1874,  1881,  1886,  1895,  1898,  1906,
    1909,  1916,  1923,  1926,  1934,  1942,  1945,  1953,  1958,  1968,
    1972,  1971,  1985,  1993,  1996,  2021,  2035,  2044,  2052,  2055,
    2121,  2129,  2137,  2138,  2139,  2140,  2141,  2142,  2143,  2144,
    2145,  2146,  2147,  2148,  2149,  2150,  2151,  2153,  2154,  2155,
    2156,  2157,  2158,  2159,  2160,  2161,  2162,  2163,  2164,  2165,
    2166,  2167,  2168,  2169,  2170,  2171,  2172,  2173,  2174,  2175,
    2176,  2180,  2185,  2226,  2255,  2259,  2310,  2313,  2320,  2332,
    2359,  2364,  2372,  2377,  2386,  2389,  2397,  2400,  2408,  2411,
    2418,  2425,  2429,  2433,  2440,  2444,  2451,  2460,  2469,  2476,
    2483,  2490,  2494,  2501,  2514,  2531,  2541,  2549,  2560,  2581,
    2588,  2596,  2599,  2636,  2658,  2661,  2665,  2672,  2676,  2680,
    2684,  2691,  2695,  2702,  2710,  2724,  2731,  2739,  2747,  2755,
    2766,  2777,  2783,  2793,  2803,  2806,  2813,  2824,  2836,  2843,
    2853,  2860,  2864,  2868,  2872,  2879,  2883,  2887,  2891,  2895,
    2902,  2909,  2920,  2930,  2937,  2941,  2945,  2949,  2956,  2967,
    2977,  2984,  2988,  2992,  2996,  3003,  3013,  3050,  3059,  3074,
    3079,  3088,  3099,  3103,  3108,  3121,  3141,  3145,  3150,  3165,
    3170,  3175,  3180,  3185,  3190,  3195,  3200,  3205,  3210,  3215,
    3220,  3225,  3230,  3239,  3242,  3250,  3253,  3260,  3265,  3271,
    3275,  3279,  3284,  3292,  3299,  3308,  3313,  3338,  3343,  3366,
    3370,  3375,  3381,  3385,  3392,  3396,  3400,  3404,  3408,  3412,
    3419,  3426,  3430,  3438,  3441,  3445,  3452,  3456,  3460,  3464,
    3468,  3472,  3476,  3480,  3484,  3488,  3492,  3497,  3502,  3507,
    3512,  3517,  3522,  3527,  3531,  3536,  3541,  3546,  3551,  3556,
    3560,  3564,  3568,  3572,  3579,  3583,  3590,  3594,  3601,  3605,
    3612,  3616,  3623,  3627,  3634,  3638,  3649,  3656,  3660,  3664,
    3668,  3672,  3676,  3683,  3687
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
  "THEN_TOK", "TRUE_TOK", "UNION_TOK", "UNITS_TOK", "LADDER_TOK",
  "UNIVERSAL_TOK", "UNLINK_TOK", "WHEN_TOK", "WHERE_TOK", "WHILE_TOK",
  "WILLBE_TOK", "WILLBETHESAME_TOK", "WILLNOTBETHESAME_TOK", "ASSIGN_TOK",
  "CASSIGN_TOK", "DBLCOLON_TOK", "USE_TOK", "LEQ_TOK", "GEQ_TOK",
  "NEQ_TOK", "DOTDOT_TOK", "WITH_TOK", "VALUE_TOK", "WITH_VALUE_T",
  "REAL_TOK", "INTEGER_TOK", "IDENTIFIER_TOK", "BRACEDTEXT_TOK",
  "SYMBOL_TOK", "DQUOTE_TOK", "','", "'|'", "'<'", "'='", "'>'", "'+'",
  "'-'", "'/'", "'*'", "UMINUS_TOK", "UPLUS_TOK", "'^'", "';'", "'('",
  "')'", "':'", "'['", "']'", "'.'", "$accept", "definitions",
  "definition", "global_def", "require_file", "provide_module", "import",
  "add_notes_def", "add_notes_head", "add_method_def", "add_method_head",
  "replace_method_def", "replace_method_head", "atom_def", "atom_head",
  "atom_id", "default_val", "constant_def", "constant_head",
  "constant_dims", "constant_val", "model_def", "model_head", "model_id",
  "optional_model_parameters", "optional_parameter_wheres",
  "optional_parameter_reduction", "universal", "definition_def",
  "definition_id", "units_def", "units_statement", "optional_ladder_end",
  "table_statement", "table_begin", "table_mode_on", "table_mode_off",
  "table_options", "table_decl_opt", "table_option", "table_body",
  "table_body_item", "table_scalar", "values_statement",
  "values_default_opt", "values_entries", "values_entry",
  "values_key_list", "values_key", "dataset_statement", "$@1",
  "dataset_items", "dataset_item", "dataset_index_item",
  "dataset_map_item", "dataset_column_selector", "dataset_column_ref",
  "dataset_target", "dataset_units_opt", "dataset_type_opt",
  "dataset_type_req", "unitdeflist", "unitdef", "unitladderitemlist",
  "unitladderitem", "methods", "$@2", "proclist", "proclistf", "procedure",
  "procedure_id", "fstatements", "statements", "statement",
  "complex_statement", "isa_statement", "willbe_statement",
  "aliases_statement", "optional_set_values", "is_statement",
  "isrefinedto_statement", "call_identifier", "type_identifier",
  "optional_method", "optional_of", "optional_with_value",
  "arealike_statement", "link_statement", "unlink_statement",
  "der_statement", "independent_statement", "arethesame_statement",
  "willbethesame_statement", "willnotbethesame_statement",
  "assignment_statement", "relation_statement", "relation",
  "blackbox_statement", "input_args", "output_args", "data_args",
  "for_statement", "optional_direction", "forexprend", "run_statement",
  "fix_statement", "fix_and_assign_statement", "free_statement",
  "solver_statement", "option_statement", "solve_statement",
  "external_statement", "call_statement", "assert_statement",
  "if_statement", "while_statement", "optional_else", "when_statement",
  "whenlist", "whenlistf", "flow_statement", "use_statement",
  "select_statement", "selectlist", "selectlistf", "switch_statement",
  "switchlist", "switchlistf", "conditional_statement", "notes_statement",
  "notes_body", "noteslist", "fvarlist", "varlist", "fname", "name", "end",
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

#define YYPACT_NINF (-689)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-293)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -689,   869,  -689,  -689,    -3,   -97,   -81,    76,    91,    18,
     166,   -37,  -689,  -689,  -689,  -689,  -689,  -689,  -689,   -26,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,    20,  -689,  -689,
    -689,   -47,   111,   137,  -689,  -689,   160,    73,  -689,   102,
      93,   174,   107,   143,  -689,   -15,   147,    -9,   223,   212,
     223,   162,   167,   177,  -689,   190,  -689,  -689,   170,   251,
     721,  -689,   -22,   200,   223,   205,  -689,  -689,  -689,   808,
     230,    -1,  -689,  -689,     1,   346,   232,  -689,  -689,   147,
     236,   140,  -689,   -70,   147,   227,   229,   256,  -689,   247,
     249,  -689,   305,  -689,   251,   267,   251,  -689,   292,  -689,
     223,   261,  -689,   808,  -689,   278,   172,   184,  -689,  -689,
     268,   284,  -689,  -689,   147,   289,   147,   808,   147,   189,
     275,   808,  -689,  -689,   808,   808,   -26,   301,   201,  -689,
     147,   285,   -87,  -689,   304,   287,   202,    54,   147,   147,
     306,  -689,   203,   293,    78,   808,   147,   308,   310,   296,
    -689,   808,   808,   808,   808,  -689,  -689,  -689,  -689,   299,
     300,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,   241,   164,
    -689,  1071,   374,   302,   378,   309,   312,  -689,   303,   318,
     534,  -689,   383,   314,   324,   316,   355,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
     333,   336,  -689,   147,  -689,  -689,  -689,   147,  -689,  -689,
    -689,  -689,  -689,   338,   223,   271,   223,   321,   323,   -45,
    -689,   325,  -689,  1071,   326,  -689,   808,   808,   808,   808,
     223,   147,   327,  -689,  -689,   226,   415,  -689,   980,  -689,
     808,   808,     0,  1071,  1071,  -689,    -9,   808,   808,   808,
     356,   147,   147,    28,  -689,  -689,  -689,   808,   808,   147,
      33,  -689,   443,   430,   808,   808,    44,   147,    34,   461,
    -689,  -689,  -689,  -689,   808,   341,   341,   849,   337,  -689,
    -689,    85,  -689,  -689,   359,   360,   359,   359,  -689,  -689,
     808,   808,   273,   808,  -689,  -689,   808,   808,   808,  -689,
    -689,  -689,   808,  -689,  -689,  -689,   808,   808,   808,   808,
     808,   808,   808,   347,  -689,   362,   349,  -689,  -689,  -689,
     808,   808,   352,  -689,   369,  -689,  -689,  -689,   358,  -689,
    -689,   223,     7,   361,    88,  -689,   465,   364,  -689,  -689,
     376,  -689,  -689,  -689,   808,   363,   365,   367,   366,  -689,
     371,   147,   808,  -689,   372,   370,   379,   388,   389,  -689,
    1071,   377,   375,   147,   -61,   380,   808,   381,   223,    58,
     384,   382,   390,   808,   392,   223,    82,   449,   808,   385,
     391,   394,   393,   409,   412,   400,   808,   402,   223,   112,
    -689,   905,  -689,  -689,   147,  -689,   405,   451,   451,  -689,
     451,  1071,  1071,    86,   406,  -689,   288,   288,   288,   187,
     187,   -67,   -67,   341,   341,   341,   146,    12,  -689,  -689,
    -689,  1015,  1071,  -689,  -689,  -689,   403,    88,  -689,   425,
     441,  -689,  -689,  -689,  -689,    13,  -689,    55,    53,   414,
    -689,   413,   417,   418,  -689,  -689,  -689,  -689,  -689,   420,
     941,   526,  -689,  -689,   439,   147,   147,  -689,  -689,  -689,
     242,  -689,    28,   423,  -689,  -689,   808,   424,  -689,  -689,
      33,   426,  -689,  -689,   808,   428,   359,  -689,  1071,   186,
     432,  -689,  -689,   147,   147,    34,   431,  -689,  -689,   808,
     437,   223,  -689,   433,   808,   444,   460,  -689,   460,   147,
      34,   808,   808,  -689,  -689,  -689,   118,   456,   127,    13,
      13,   243,  -689,  -689,  -689,  -689,  -689,   262,  -689,  -689,
    -689,   447,  -689,  -689,  -689,  -689,   221,  -689,   223,   470,
     442,   446,   310,   457,   223,  -689,  -689,   458,  -689,   223,
    -689,  -689,   459,  -689,   451,    21,  -689,  -689,  -689,  -689,
     -10,   464,   225,  -689,  -689,   463,   467,   223,  -689,  -689,
     468,  -689,  -689,   483,   471,  -689,   808,  -689,  -689,   472,
     223,   477,   469,    65,  1071,  -689,  -689,  -689,   262,   478,
    -689,   480,   480,  -689,   -90,   310,  -689,  -689,   481,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,   147,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
     808,  -689,   482,  -689,  -689,   518,   485,  -689,   186,   808,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,   147,  -689,  1071,
      34,  -689,   147,   568,  -689,  -689,  -689,   503,  -689,  -689,
     223,   487,  -689,  -689,  1071,  -689,    41,  -689,  -689,  -689,
    1071,    15,  -689,   570,   223,   490,   488,  -689,   497,  -689,
    -689,   596,  1041,   495,   543,   513,   494,   499,  -689,  -689,
     -33,   517,  -689,   147,   500,   569,   521,   554,   556,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,   602,   147,  -689,    75,   529,
     147,   610,   587,   515,  -689,  -689,   519,  -689,  -689,   563,
     530,    56,  -689,  -689,   535,  -689,  -689,   532,    75,   547,
     645,  -689,  -689,  -689,    56,  -689,  -689,  -689,   621,  -689,
     535,   558,  -689,   623,   557,  -689,  -689,   542,  -689,   560,
    -689,   808,  -689,   544,  -689
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    16,     0,     0,     0,     0,     0,     0,
       0,   132,    64,     3,    15,     4,     5,     6,     9,     0,
       7,   143,     8,   143,    11,    10,    12,     0,    13,   148,
      14,     0,     0,     0,    66,   148,     0,     0,   296,     0,
      23,     0,     0,    20,   135,     0,     0,     0,     0,   142,
       0,     0,     0,     0,   148,     0,    42,   148,    57,   139,
       0,    67,     0,     0,     0,     0,    25,    21,    22,   318,
       0,     0,    18,    19,     0,   312,     0,   133,    68,   287,
       0,   291,   292,   315,     0,     0,     0,     0,   144,     0,
       0,    36,     0,    56,   139,     0,   139,   148,    59,   140,
       0,     0,   352,     0,   265,     0,     0,     0,   148,   266,
       0,     0,   351,   267,     0,     0,     0,     0,     0,     0,
       0,     0,   347,   349,     0,     0,     0,     0,     0,   268,
       0,     0,     0,   248,     0,   313,     0,     0,     0,     0,
       0,   350,     0,     0,     0,     0,     0,   327,   346,   296,
     353,     0,     0,     0,   318,   187,   188,   189,   190,     0,
       0,   152,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   223,   167,   170,   171,
     172,   191,   173,   174,   176,   175,   169,   168,   177,   178,
     179,   180,   182,   181,   183,   184,   185,   186,     0,   354,
     348,   225,     0,     0,   204,     0,     0,   354,     0,   317,
     319,   297,     0,     0,   138,     0,    70,   299,   300,   308,
     301,   302,   303,   304,   305,   306,   307,   309,   310,   311,
       0,     0,   289,     0,   294,   316,   295,   288,    26,    28,
     146,   148,    31,     0,     0,   331,     0,     0,     0,     0,
     143,     0,   151,   251,   200,   250,   318,   318,   318,   318,
       0,     0,     0,   244,   243,   292,     0,   245,     0,   217,
     318,   318,     0,   227,   226,   363,     0,     0,   318,   318,
     241,     0,     0,     0,   246,   314,   269,   318,   318,     0,
       0,    74,   100,     0,   318,   318,     0,     0,     0,     0,
     270,   328,   325,   326,     0,   369,   370,     0,     0,   149,
     150,     0,   210,   218,     0,     0,     0,     0,   219,   220,
       0,     0,     0,     0,   393,   394,     0,     0,     0,   390,
     391,   392,     0,   388,   387,   389,     0,     0,     0,     0,
       0,     0,     0,     0,    29,     0,     0,    17,    24,   298,
       0,     0,     0,    32,     0,   136,    71,    69,     0,   290,
     293,     0,    46,     0,     0,   330,    37,     0,    58,   148,
       0,    54,   141,    65,   318,     0,     0,     0,     0,   285,
       0,     0,     0,   148,     0,     0,     0,     0,     0,   286,
     247,     0,     0,     0,     0,     0,   318,     0,     0,   273,
       0,     0,     0,   318,     0,     0,   280,    79,     0,     0,
       0,     0,     0,     0,     0,     0,   318,     0,     0,   260,
     148,     0,   386,   355,     0,   194,   202,   206,   206,   199,
     206,   221,   222,     0,   296,   224,   361,   366,   362,   368,
     367,   356,   357,   359,   358,   360,   364,   365,    30,   205,
      27,   321,   320,    33,   137,   134,     0,     0,    45,     0,
      48,    34,   335,   334,   332,     0,   329,   333,   343,     0,
      53,     0,    61,     0,   381,   382,   383,   384,   216,     0,
     234,   254,   379,   380,     0,     0,     0,   375,   376,   242,
       0,   372,     0,     0,   148,   271,   318,     0,   373,   374,
       0,     0,   148,   278,   318,     0,     0,    77,   101,     0,
       0,   377,   378,     0,     0,     0,     0,   148,   256,   318,
       0,     0,   385,     0,   318,     0,   208,   198,   208,     0,
       0,     0,     0,   145,    44,    47,   343,   315,     0,     0,
       0,   343,    39,    40,    41,   344,   345,     0,    35,    60,
     148,     0,   201,   249,   236,   235,     0,   148,     0,     0,
       0,     0,     0,     0,     0,   148,   275,     0,   148,     0,
     148,   282,     0,   148,   206,     0,   110,   109,   107,   108,
       0,     0,     0,   105,   111,     0,     0,     0,   148,   262,
       0,   148,   253,     0,     0,   207,     0,   192,   193,     0,
       0,     0,     0,   354,   322,    51,    50,    52,     0,     0,
     339,   336,   337,   338,     0,   323,    38,   324,     0,    55,
     239,   237,   240,   238,   148,   255,   252,     0,   212,   213,
     371,   272,   274,   148,   277,   279,   281,   148,   284,    80,
       0,    81,     0,    75,    78,     0,     0,   102,     0,     0,
     113,   214,   215,   258,   261,   148,   264,     0,   203,   209,
       0,   257,     0,     0,    49,    43,   341,     0,   340,    62,
       0,     0,   276,   283,    82,    83,     0,    99,   103,   106,
     104,     0,   263,     0,     0,   231,     0,   229,     0,   233,
     211,     0,     0,     0,     0,     0,     0,     0,   116,   117,
       0,     0,   259,     0,     0,     0,     0,     0,     0,    93,
      97,    96,    94,    98,    95,    89,    88,    90,    91,    92,
      87,    85,    86,   115,   112,     0,     0,   114,     0,     0,
       0,     0,     0,     0,   228,   230,     0,    76,    76,     0,
       0,     0,   123,   124,   127,   122,   131,     0,     0,     0,
       0,   342,    73,    72,     0,   125,   121,   128,   129,   126,
     127,   196,   232,     0,     0,   119,   120,     0,   195,     0,
     130,   318,   118,     0,   197
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,   622,  -689,  -689,  -689,  -689,   -55,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,   105,  -689,    38,  -689,
    -689,  -689,  -689,  -689,  -689,   -60,  -688,  -689,   -73,  -689,
    -689,  -689,  -689,  -689,  -689,   196,  -689,   -21,  -689,  -689,
    -689,   -14,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -313,  -689,  -421,   161,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,   576,  -689,   386,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,
    -689,  -689,  -689,  -689,  -689,  -689,  -689,  -689,  -505,  -689,
    -689,  -689,  -689,   199,  -689,  -689,   192,  -689,  -689,  -689,
     571,   614,   -59,  -689,   -46,   344,   -35,  -689,   165,  -143,
    -689,    95,  -460,  -689,  -689,   250,  -459,  -689,  -689,  -411,
     373,  -689,  -689
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    54,    55,   469,    25,    56,   460,
     537,    26,    57,    58,    98,   249,   551,    27,    28,    29,
      30,    31,   357,   156,   407,   676,   752,   575,   507,   644,
     692,   721,   722,   157,   409,   580,   581,   582,   583,   158,
     650,   681,   697,   698,   699,   744,   745,   700,   758,   765,
     731,    45,    77,    74,   215,   100,   250,    48,    49,    88,
      89,    59,    60,   159,   160,   161,   162,   163,   768,   164,
     165,   255,   427,   346,   526,   597,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   601,   685,
     704,   178,   556,   624,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   558,   191,   418,   419,
     192,   193,   194,   398,   399,   195,   405,   406,   196,   197,
      47,    79,    80,    81,   207,    83,    78,   286,   236,   208,
     209,   616,   200,   302,   366,   466,   467,   613,   668,   547,
     210,   341,   342
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      82,   198,    50,   429,   430,   202,   538,   527,   728,   528,
     587,   308,    85,    86,   199,    90,   693,   323,    75,   729,
     231,    64,    34,   645,    75,   600,   212,   370,   666,   205,
     563,    51,    38,    82,    75,   234,   457,   458,    82,   216,
      94,    52,   691,    96,   386,   396,   667,   640,   694,   282,
     403,   416,   235,   756,    35,   264,    32,   267,   326,   269,
      44,    33,   490,   338,   339,   251,   763,   340,   265,    69,
      82,    70,    82,   283,   -84,   496,   491,    41,   290,   327,
     611,   612,    53,   247,   280,   298,    82,   617,    61,   542,
     371,    82,   291,   292,   260,    46,   397,   203,    82,   504,
     300,   404,   417,   459,    76,   695,   730,   576,   577,   578,
     641,   579,    84,   375,   376,   377,   378,   642,   213,    38,
     214,   387,   329,   330,   331,   608,   497,   384,   385,   519,
     614,   462,   463,   -84,   696,   391,   392,   333,   334,   335,
     336,   337,   338,   339,   400,   401,   340,   543,   617,   465,
     505,   411,   412,   639,   605,   684,   643,    62,   -84,   -84,
     -84,   -84,   -84,    38,   -84,   413,   741,   -84,  -292,   -84,
     -84,  -292,  -292,    38,   544,   742,   -84,   743,   231,   -84,
     520,   545,   546,    63,  -292,   539,   540,   360,  -292,   541,
     289,    82,   323,   574,   742,    36,   743,    38,    37,   324,
     325,    65,   380,  -292,    38,    38,   462,   463,    66,   363,
      38,   367,   606,    39,   297,    82,  -292,  -292,  -292,   464,
      71,   424,   529,   395,   465,   379,   388,   361,    68,   372,
     402,   473,    69,   326,    70,   394,    82,    67,   415,   607,
     620,   389,    72,    82,   621,   311,   545,   546,   312,   313,
     414,    82,   622,   493,   327,   623,    75,   539,   540,    38,
     501,   541,    95,   233,   610,   425,    38,  -292,  -292,  -292,
     320,   321,    87,   516,   336,   337,   338,   339,    73,   102,
     340,    91,    69,  -292,    70,    38,    92,  -292,    42,   106,
     244,   107,   246,   314,   315,   316,    93,   329,   330,   331,
     364,   365,   322,   576,   577,   578,    97,   579,   256,   112,
      99,   257,   333,   334,   335,   336,   337,   338,   339,   204,
     258,   340,   479,   259,   119,   270,   456,   206,   271,   121,
     122,   123,   320,   321,   124,    82,   125,   278,   287,   294,
     279,   288,   295,   128,   317,   318,   319,   489,   648,   211,
     131,   649,    40,   567,    43,   471,   232,   136,   230,   147,
     562,   572,   238,   495,   239,   523,   217,   141,   142,   481,
     503,   545,   546,   433,   530,   240,   590,   243,    82,   147,
     615,   594,   241,   518,   242,   218,   245,    82,   219,   220,
     147,   148,   434,   248,   150,   221,   252,   254,   329,   330,
     331,   151,   152,   262,   261,   222,   521,   285,   266,   153,
     223,   272,   154,   333,   334,   335,   336,   337,   338,   339,
     277,   281,   340,   284,   224,   293,   560,   561,   301,   296,
     303,   225,   304,   201,   309,   310,   343,   344,   345,    82,
      82,   350,   226,   349,   347,   352,   227,   348,   228,   353,
     354,   355,   356,   358,   585,   586,   359,   362,   368,   369,
     373,   382,   374,   381,   393,   229,   323,    82,    82,   408,
     599,   410,   602,   324,   325,   340,   253,   423,   426,   428,
     566,   449,   448,    82,   450,   603,   592,   453,   571,   454,
     268,   468,   420,   455,   273,   472,   461,   274,   275,   470,
     474,   506,   484,   589,   476,   475,   477,   326,   478,   482,
     483,   485,   486,   510,   487,   488,   525,   492,   299,   494,
     509,   498,   499,   626,   305,   306,   307,   500,   327,   631,
     502,   511,   513,   512,   635,   514,   618,   515,   533,   323,
     517,   524,   531,   625,   328,   535,   324,   325,   536,   548,
     549,   632,   653,   550,   634,   552,   636,   553,   557,   638,
     559,   565,   568,   595,   570,   661,   573,   584,   671,   588,
     593,   329,   330,   331,   654,   591,   596,   656,   235,   628,
     326,    82,   619,   629,   657,   332,   333,   334,   335,   336,
     337,   338,   339,   627,   630,   340,   633,   637,   683,   647,
     651,   327,   675,   686,   652,   677,   655,   663,   658,   660,
     670,    82,   662,   665,   541,   687,    82,   328,   669,   672,
     678,   688,   701,   673,   690,   703,   705,   706,   773,   707,
     723,   724,   725,   726,   727,   689,   732,   734,   735,   736,
     737,   682,   738,   739,   329,   330,   331,   351,   746,   702,
     390,   748,   749,   750,   754,   757,   751,   733,   332,   333,
     334,   335,   336,   337,   338,   339,   761,   740,   340,   762,
     755,   747,   759,   764,   767,   769,   770,   421,   771,   772,
      82,   774,   155,   753,    82,   646,   679,   766,   760,   598,
     263,   564,   569,   431,   432,   201,   436,   276,   237,   437,
     438,   439,   609,   664,     0,   440,     0,   534,   435,   441,
     442,   443,   444,   445,   446,   447,     0,     0,     0,     0,
       0,     0,   101,   451,   452,     0,     0,   102,     0,     0,
       0,   103,     0,     0,     0,   104,   105,   106,  -147,   107,
       0,   108,     0,   109,     0,     0,     0,     0,     0,   110,
       0,     0,     0,  -147,  -147,   480,   111,   112,   113,   114,
     115,   116,     0,     0,   117,     0,     0,     0,     0,     0,
       0,   118,   119,     0,     0,     0,   120,   121,   122,   123,
    -147,   508,   124,     0,   125,   126,     0,   127,     0,  -147,
       0,   128,     0,     0,     0,     0,   129,   130,   131,   132,
       0,   133,   134,   135,     0,   136,   137,   138,   139,   140,
       0,     0,     0,     0,   102,   141,   142,    11,     0,     0,
     143,   144,     0,   145,   106,     0,   107,     0,     0,     0,
     146,     0,     0,     0,     0,     0,     0,     0,   147,   148,
     149,     0,   150,     0,   112,     0,     0,     0,     0,   151,
     152,     0,     0,     0,   323,     0,     0,   153,  -147,   119,
     154,   324,   325,     0,     0,   122,   123,     0,     0,     2,
       3,   125,     4,     0,     0,     0,     0,     0,   128,     0,
     -63,     0,     0,     0,     0,   131,     0,     0,     0,     0,
     -63,     0,   136,     0,     0,   326,     5,     0,     0,     0,
       0,     0,   141,   142,   421,   604,     0,     0,     0,     0,
     323,     6,     0,     0,     7,     0,   327,   324,   325,     0,
       0,     0,     0,     0,     0,   147,   148,   149,     0,   150,
       0,   -63,   328,     0,     0,     0,   151,   152,     0,     0,
       8,     0,     9,    10,   153,     0,   323,   154,     0,     0,
       0,   326,     0,   324,   325,     0,     0,     0,     0,   329,
     330,   331,     0,     0,     0,    11,   554,    12,     0,   659,
       0,     0,   327,   332,   333,   334,   335,   336,   337,   338,
     339,     0,     0,   340,     0,   323,   422,   326,   328,   555,
       0,     0,   324,   325,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   327,     0,
       0,     0,     0,   674,     0,   329,   330,   331,     0,     0,
     323,     0,   680,     0,   328,     0,   326,   324,   325,   332,
     333,   334,   335,   336,   337,   338,   339,     0,     0,   340,
       0,     0,   522,     0,     0,     0,     0,   327,     0,     0,
       0,   329,   330,   331,     0,     0,     0,     0,     0,     0,
       0,   326,     0,   328,     0,   332,   333,   334,   335,   336,
     337,   338,   339,   383,   708,   340,   323,     0,     0,     0,
       0,     0,   327,   324,   325,     0,     0,     0,     0,     0,
     329,   330,   331,     0,     0,     0,     0,     0,   328,     0,
       0,     0,     0,     0,   332,   333,   334,   335,   336,   337,
     338,   339,     0,     0,   340,     0,     0,   326,     0,     0,
       0,     0,     0,     0,     0,   329,   330,   331,   532,     0,
       0,     0,     0,   709,     0,     0,     0,     0,   327,   332,
     333,   334,   335,   336,   337,   338,   339,     0,     0,   340,
       0,     0,     0,     0,   328,     0,     0,     0,   710,   711,
     712,   713,   714,     0,   715,     0,     0,   716,     0,   717,
     718,     0,     0,     0,     0,     0,   719,     0,     0,   720,
       0,   329,   330,   331,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   332,   333,   334,   335,   336,
     337,   338,   339,     0,     0,   340
};

static const yytype_int16 yycheck[] =
{
      46,    60,    23,   316,   317,    27,   465,   428,    41,   430,
     515,   154,    47,    48,    60,    50,     1,     5,    33,    52,
      79,    35,   119,    33,    33,   530,    27,    72,   118,    64,
     490,    11,   119,    79,    33,    81,    29,    30,    84,    74,
      54,    21,     1,    57,    44,    17,   136,    26,    33,   136,
      17,    17,   122,   741,   135,   114,    59,   116,    46,   118,
      97,    64,   123,   130,   131,   100,   754,   134,   114,   139,
     116,   141,   118,   132,    33,    17,   137,    59,   137,    67,
     539,   540,    62,    97,   130,   144,   132,   547,   135,    36,
     135,   137,   138,   139,   108,   121,    68,   119,   144,    17,
     146,    68,    68,    96,   119,    90,   139,   117,   118,   119,
      89,   121,   121,   256,   257,   258,   259,    96,   119,   119,
     119,   121,   110,   111,   112,   536,    68,   270,   271,    17,
     541,   118,   119,    92,   119,   278,   279,   125,   126,   127,
     128,   129,   130,   131,   287,   288,   134,    94,   608,   136,
      68,   294,   295,   574,    36,   660,   135,    46,   117,   118,
     119,   120,   121,   119,   123,   121,    91,   126,     4,   128,
     129,     7,     8,   119,   121,   119,   135,   121,   237,   138,
      68,   128,   129,    46,   119,   130,   131,   233,   123,   134,
     136,   237,     5,   506,   119,   119,   121,   119,   122,    12,
      13,    41,   261,   138,   119,   119,   118,   119,   135,   244,
     119,   246,    94,   122,   136,   261,    52,    53,    54,   131,
      46,   136,   136,   282,   136,   260,   272,   241,   135,   250,
     289,   374,   139,    46,   141,   281,   282,   135,   297,   121,
      19,   276,   135,   289,    23,     4,   128,   129,     7,     8,
     296,   297,    31,   396,    67,    34,    33,   130,   131,   119,
     403,   134,    72,   123,   137,   311,   119,   103,   104,   105,
     106,   107,    60,   416,   128,   129,   130,   131,   135,     6,
     134,   119,   139,   119,   141,   119,   119,   123,   122,    16,
      94,    18,    96,    52,    53,    54,   119,   110,   111,   112,
      29,    30,   138,   117,   118,   119,   136,   121,   136,    36,
      59,   139,   125,   126,   127,   128,   129,   130,   131,   119,
     136,   134,   381,   139,    51,   136,   361,   122,   139,    56,
      57,    58,   106,   107,    61,   381,    63,   136,   136,   136,
     139,   139,   139,    70,   103,   104,   105,   393,   123,   119,
      77,   126,     8,   496,    10,   369,   120,    84,   126,   117,
     118,   504,   135,   398,   135,   424,    20,    94,    95,   383,
     405,   128,   129,   100,   433,   119,   519,    72,   424,   117,
     118,   524,   135,   418,   135,    39,   119,   433,    42,    43,
     117,   118,   119,   101,   121,    49,   135,   119,   110,   111,
     112,   128,   129,   119,   136,    59,   420,   120,   119,   136,
      64,   136,   139,   125,   126,   127,   128,   129,   130,   131,
     119,   136,   134,   119,    78,   119,   485,   486,   120,   136,
     120,    85,   136,    60,   135,   135,    62,   135,    60,   485,
     486,   123,    96,   140,   135,    62,   100,   135,   102,   135,
     126,   135,    97,   120,   513,   514,   120,   119,   137,   136,
     135,    46,   136,   136,   108,   119,     5,   513,   514,    26,
     529,    41,   531,    12,    13,   134,   103,   140,   119,   119,
     494,   119,   135,   529,   135,   531,   521,   135,   502,   120,
     117,    26,    31,   135,   121,   119,   135,   124,   125,   135,
     137,    52,   123,   517,   137,   140,   140,    46,   137,   137,
     140,   123,   123,   122,   137,   140,    65,   137,   145,   138,
     135,   137,   140,   558,   151,   152,   153,   137,    67,   564,
     138,   137,   123,   140,   569,   123,   550,   137,   135,     5,
     138,   136,   136,   557,    83,   120,    12,    13,   107,   135,
     137,   565,   587,   136,   568,   137,   570,   137,    32,   573,
     121,   138,   138,   119,   138,   600,   138,   135,   627,   138,
     137,   110,   111,   112,   588,   138,   116,   591,   122,   137,
      46,   627,   135,   137,   101,   124,   125,   126,   127,   128,
     129,   130,   131,   123,   137,   134,   138,   138,   657,   135,
     137,    67,   120,   662,   137,    87,   138,   138,   137,   137,
     624,   657,   135,   135,   134,    47,   662,    83,   137,   633,
     135,   118,    52,   637,   137,   135,   138,   130,   771,    33,
     135,    88,   119,   139,   135,   670,   119,   137,    69,   118,
      86,   655,    86,    41,   110,   111,   112,   113,   119,   684,
     277,    41,    65,   138,    91,   120,   137,   703,   124,   125,
     126,   127,   128,   129,   130,   131,   119,   726,   134,    24,
     140,   730,   140,    52,   116,    52,   119,   304,   136,   119,
     726,   137,    60,   738,   730,   580,   648,   760,   748,   528,
     114,   492,   500,   320,   321,   322,   323,   126,    84,   326,
     327,   328,   537,   608,    -1,   332,    -1,   457,   322,   336,
     337,   338,   339,   340,   341,   342,    -1,    -1,    -1,    -1,
      -1,    -1,     1,   350,   351,    -1,    -1,     6,    -1,    -1,
      -1,    10,    -1,    -1,    -1,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,    28,
      -1,    -1,    -1,    32,    33,   382,    35,    36,    37,    38,
      39,    40,    -1,    -1,    43,    -1,    -1,    -1,    -1,    -1,
      -1,    50,    51,    -1,    -1,    -1,    55,    56,    57,    58,
      59,   408,    61,    -1,    63,    64,    -1,    66,    -1,    68,
      -1,    70,    -1,    -1,    -1,    -1,    75,    76,    77,    78,
      -1,    80,    81,    82,    -1,    84,    85,    86,    87,    88,
      -1,    -1,    -1,    -1,     6,    94,    95,    96,    -1,    -1,
      99,   100,    -1,   102,    16,    -1,    18,    -1,    -1,    -1,
     109,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,   118,
     119,    -1,   121,    -1,    36,    -1,    -1,    -1,    -1,   128,
     129,    -1,    -1,    -1,     5,    -1,    -1,   136,   137,    51,
     139,    12,    13,    -1,    -1,    57,    58,    -1,    -1,     0,
       1,    63,     3,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      11,    -1,    -1,    -1,    -1,    77,    -1,    -1,    -1,    -1,
      21,    -1,    84,    -1,    -1,    46,    27,    -1,    -1,    -1,
      -1,    -1,    94,    95,   531,   532,    -1,    -1,    -1,    -1,
       5,    42,    -1,    -1,    45,    -1,    67,    12,    13,    -1,
      -1,    -1,    -1,    -1,    -1,   117,   118,   119,    -1,   121,
      -1,    62,    83,    -1,    -1,    -1,   128,   129,    -1,    -1,
      71,    -1,    73,    74,   136,    -1,     5,   139,    -1,    -1,
      -1,    46,    -1,    12,    13,    -1,    -1,    -1,    -1,   110,
     111,   112,    -1,    -1,    -1,    96,    25,    98,    -1,   596,
      -1,    -1,    67,   124,   125,   126,   127,   128,   129,   130,
     131,    -1,    -1,   134,    -1,     5,   137,    46,    83,    48,
      -1,    -1,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    67,    -1,
      -1,    -1,    -1,   640,    -1,   110,   111,   112,    -1,    -1,
       5,    -1,   649,    -1,    83,    -1,    46,    12,    13,   124,
     125,   126,   127,   128,   129,   130,   131,    -1,    -1,   134,
      -1,    -1,   137,    -1,    -1,    -1,    -1,    67,    -1,    -1,
      -1,   110,   111,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    46,    -1,    83,    -1,   124,   125,   126,   127,   128,
     129,   130,   131,    93,    33,   134,     5,    -1,    -1,    -1,
      -1,    -1,    67,    12,    13,    -1,    -1,    -1,    -1,    -1,
     110,   111,   112,    -1,    -1,    -1,    -1,    -1,    83,    -1,
      -1,    -1,    -1,    -1,   124,   125,   126,   127,   128,   129,
     130,   131,    -1,    -1,   134,    -1,    -1,    46,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   110,   111,   112,   113,    -1,
      -1,    -1,    -1,    92,    -1,    -1,    -1,    -1,    67,   124,
     125,   126,   127,   128,   129,   130,   131,    -1,    -1,   134,
      -1,    -1,    -1,    -1,    83,    -1,    -1,    -1,   117,   118,
     119,   120,   121,    -1,   123,    -1,    -1,   126,    -1,   128,
     129,    -1,    -1,    -1,    -1,    -1,   135,    -1,    -1,   138,
      -1,   110,   111,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   124,   125,   126,   127,   128,
     129,   130,   131,    -1,    -1,   134
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   143,     0,     1,     3,    27,    42,    45,    71,    73,
      74,    96,    98,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   159,   163,   169,   170,   171,
     172,   173,    59,    64,   119,   135,   119,   122,   119,   122,
     277,    59,   122,   277,    97,   203,   121,   272,   209,   210,
     209,    11,    21,    62,   156,   157,   160,   164,   165,   213,
     214,   135,    46,    46,   213,    41,   135,   135,   135,   139,
     141,    46,   135,   135,   205,    33,   119,   204,   278,   273,
     274,   275,   276,   277,   121,   278,   278,    60,   211,   212,
     278,   119,   119,   119,   213,    72,   213,   136,   166,    59,
     207,     1,     6,    10,    14,    15,    16,    18,    20,    22,
      28,    35,    36,    37,    38,    39,    40,    43,    50,    51,
      55,    56,    57,    58,    61,    63,    64,    66,    70,    75,
      76,    77,    78,    80,    81,    82,    84,    85,    86,    87,
      88,    94,    95,    99,   100,   102,   109,   117,   118,   119,
     121,   128,   129,   136,   139,   173,   175,   185,   191,   215,
     216,   217,   218,   219,   221,   222,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   243,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   259,   262,   263,   264,   267,   270,   271,   274,   276,
     284,   292,    27,   119,   119,   278,   122,   276,   281,   282,
     292,   119,    27,   119,   119,   206,   278,    20,    39,    42,
      43,    49,    59,    64,    78,    85,    96,   100,   102,   119,
     126,   274,   120,   123,   276,   122,   280,   273,   135,   135,
     119,   135,   135,    72,   207,   119,   207,   213,   101,   167,
     208,   278,   135,   292,   119,   223,   136,   139,   136,   139,
     213,   136,   119,   236,   274,   276,   119,   274,   292,   274,
     136,   139,   136,   292,   292,   292,   272,   119,   136,   139,
     276,   136,   136,   274,   119,   120,   279,   136,   139,   136,
     274,   276,   276,   119,   136,   139,   136,   136,   274,   292,
     276,   120,   285,   120,   136,   292,   292,   292,   281,   135,
     135,     4,     7,     8,    52,    53,    54,   103,   104,   105,
     106,   107,   138,     5,    12,    13,    46,    67,    83,   110,
     111,   112,   124,   125,   126,   127,   128,   129,   130,   131,
     134,   293,   294,    62,   135,    60,   225,   135,   135,   140,
     123,   113,    62,   135,   126,   135,    97,   174,   120,   120,
     276,   213,   119,   278,    29,    30,   286,   278,   137,   136,
      72,   135,   209,   135,   136,   281,   281,   281,   281,   278,
     274,   136,    46,    93,   281,   281,    44,   121,   276,   278,
     292,   281,   281,   108,   276,   274,    17,    68,   265,   266,
     281,   281,   274,    17,    68,   268,   269,   176,    26,   186,
      41,   281,   281,   121,   276,   274,    17,    68,   260,   261,
      31,   292,   137,   140,   136,   276,   119,   224,   119,   224,
     224,   292,   292,   100,   119,   238,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   135,   119,
     135,   292,   292,   135,   120,   135,   278,    29,    30,    96,
     161,   135,   118,   119,   131,   136,   287,   288,    26,   158,
     135,   213,   119,   281,   137,   140,   137,   140,   137,   274,
     292,   213,   137,   140,   123,   123,   123,   137,   140,   276,
     123,   137,   137,   281,   138,   278,    17,    68,   137,   140,
     137,   281,   138,   278,    17,    68,    52,   180,   292,   135,
     122,   137,   140,   123,   123,   137,   281,   138,   278,    17,
      68,   213,   137,   274,   136,    65,   226,   226,   226,   136,
     274,   136,   113,   135,   287,   120,   107,   162,   288,   130,
     131,   134,    36,    94,   121,   128,   129,   291,   135,   137,
     136,   168,   137,   137,    25,    48,   244,    32,   258,   121,
     274,   274,   118,   284,   265,   138,   213,   281,   138,   268,
     138,   213,   281,   138,   224,   179,   117,   118,   119,   121,
     187,   188,   189,   190,   135,   274,   274,   260,   138,   213,
     281,   138,   278,   137,   281,   119,   116,   227,   227,   274,
     260,   240,   274,   276,   292,    36,    94,   121,   291,   280,
     137,   288,   288,   289,   291,   118,   283,   284,   213,   135,
      19,    23,    31,    34,   245,   213,   278,   123,   137,   137,
     137,   278,   213,   138,   213,   278,   213,   138,   213,   226,
      26,    89,    96,   135,   181,    33,   188,   135,   123,   126,
     192,   137,   137,   278,   213,   138,   213,   101,   137,   292,
     137,   278,   135,   138,   283,   135,   118,   136,   290,   137,
     213,   274,   213,   213,   292,   120,   177,    87,   135,   190,
     292,   193,   213,   274,   260,   241,   274,    47,   118,   278,
     137,     1,   182,     1,    33,    90,   119,   194,   195,   196,
     199,    52,   278,   135,   242,   138,   130,    33,    33,    92,
     117,   118,   119,   120,   121,   123,   126,   128,   129,   135,
     138,   183,   184,   135,    88,   119,   139,   135,    41,    52,
     139,   202,   119,   276,   137,    69,   118,    86,    86,    41,
     274,    91,   119,   121,   197,   198,   119,   274,    41,    65,
     138,   137,   178,   178,    91,   140,   198,   120,   200,   140,
     197,   119,    24,   198,    52,   201,   200,   116,   220,    52,
     119,   136,   119,   281,   137
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   142,   143,   143,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   145,   146,   146,
     146,   147,   147,   147,   148,   148,   149,   150,   151,   152,
     152,   153,   154,   154,   155,   156,   157,   158,   158,   158,
     158,   158,   159,   160,   161,   161,   161,   161,   162,   162,
     162,   162,   162,   163,   164,   164,   165,   166,   166,   167,
     167,   168,   168,   169,   169,   170,   171,   172,   173,   173,
     174,   174,   175,   175,   176,   177,   178,   179,   179,   180,
     180,   181,   181,   181,   182,   182,   183,   183,   183,   183,
     183,   183,   183,   183,   184,   184,   184,   184,   184,   185,
     186,   186,   187,   187,   188,   189,   189,   190,   190,   190,
     190,   192,   191,   193,   193,   193,   194,   194,   195,   196,
     196,   197,   197,   198,   198,   199,   199,   200,   200,   201,
     201,   202,   203,   203,   204,   205,   205,   206,   206,   207,
     208,   207,   209,   210,   210,   211,   212,   213,   214,   214,
     214,   214,   215,   215,   215,   215,   215,   215,   215,   215,
     215,   215,   215,   215,   215,   215,   215,   215,   215,   215,
     215,   215,   215,   215,   215,   215,   215,   215,   215,   215,
     215,   215,   215,   215,   215,   215,   215,   215,   215,   215,
     215,   216,   217,   218,   219,   219,   220,   220,   221,   222,
     223,   223,   224,   224,   225,   225,   226,   226,   227,   227,
     228,   229,   229,   229,   230,   230,   231,   232,   233,   234,
     235,   236,   236,   237,   237,   238,   238,   238,   239,   240,
     241,   242,   242,   243,   244,   244,   244,   245,   245,   245,
     245,   246,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   258,   259,   259,   259,   259,
     260,   261,   261,   261,   261,   262,   262,   262,   262,   262,
     263,   264,   264,   265,   266,   266,   266,   266,   267,   267,
     268,   269,   269,   269,   269,   270,   271,   272,   272,   273,
     273,   274,   275,   275,   275,   276,   277,   277,   277,   278,
     278,   278,   278,   278,   278,   278,   278,   278,   278,   278,
     278,   278,   278,   279,   279,   280,   280,   281,   281,   282,
     282,   282,   282,   283,   283,   284,   284,   285,   285,   286,
     286,   286,   287,   287,   288,   288,   288,   288,   288,   288,
     289,   290,   290,   291,   291,   291,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   293,   293,   293,
     293,   293,   293,   294,   294
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     5,     3,     3,
       2,     3,     3,     2,     5,     3,     4,     6,     4,     5,
       6,     4,     5,     6,     6,     6,     2,     0,     3,     2,
       2,     2,     2,     8,     2,     1,     0,     2,     0,     3,
       2,     2,     2,     6,     4,     7,     2,     0,     3,     0,
       4,     0,     3,     0,     1,     5,     2,     2,     3,     5,
       0,     1,    11,    11,     0,     0,     0,     0,     2,     0,
       3,     1,     2,     2,     0,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     7,
       0,     2,     2,     3,     3,     1,     3,     1,     1,     1,
       1,     0,     9,     0,     3,     3,     1,     1,     7,     5,
       5,     2,     1,     1,     1,     4,     4,     0,     1,     0,
       2,     2,     0,     2,     4,     0,     3,     3,     1,     0,
       0,     3,     1,     0,     2,     5,     2,     1,     0,     3,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     5,     5,     3,    12,     0,     4,     4,     3,
       1,     4,     1,     4,     0,     2,     0,     2,     0,     2,
       2,     8,     6,     6,     6,     6,     4,     2,     2,     2,
       2,     3,     3,     1,     3,     1,     2,     2,     9,     3,
       3,     0,     4,     8,     0,     1,     1,     1,     1,     1,
       1,     2,     4,     2,     2,     2,     2,     3,     1,     5,
       2,     2,     6,     5,     0,     2,     4,     6,     6,     8,
       1,     4,     3,     5,     4,     1,     1,     1,     1,     2,
       2,     4,     6,     1,     4,     3,     5,     4,     4,     6,
       1,     4,     3,     5,     4,     3,     3,     2,     3,     2,
       3,     1,     1,     3,     2,     2,     1,     3,     4,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     1,     0,     1,     0,     1,     1,     0,     1,
       3,     3,     5,     1,     1,     2,     2,     0,     1,     2,
       1,     0,     1,     1,     1,     1,     3,     3,     3,     3,
       2,     1,     5,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     3,
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
#line 856 "ascParse.y"
        {
	  /*
	   * The specific syntax diagnostic is already emitted by zz_error().
	   * Emitting a generic message here causes noisy duplicates during
	   * parser recovery.
	   */
	}
#line 2722 "ascParse.c"
    break;

  case 17: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 867 "ascParse.y"
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
#line 2768 "ascParse.c"
    break;

  case 18: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 912 "ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 2776 "ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK name ';'  */
#line 916 "ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2785 "ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name  */
#line 921 "ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2794 "ascParse.c"
    break;

  case 21: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 929 "ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 2802 "ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK name ';'  */
#line 933 "ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2811 "ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name  */
#line 938 "ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2820 "ascParse.c"
    break;

  case 24: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 946 "ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
            );
          }
	}
#line 2833 "ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 955 "ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 2846 "ascParse.c"
    break;

  case 26: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 967 "ascParse.y"
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
#line 2878 "ascParse.c"
    break;

  case 27: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 998 "ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 2887 "ascParse.c"
    break;

  case 28: /* add_method_def: add_method_head proclist end ';'  */
#line 1006 "ascParse.y"
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
#line 2911 "ascParse.c"
    break;

  case 29: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1029 "ascParse.y"
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
#line 2928 "ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1042 "ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 2937 "ascParse.c"
    break;

  case 31: /* replace_method_def: replace_method_head proclist end ';'  */
#line 1050 "ascParse.y"
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
#line 2959 "ascParse.c"
    break;

  case 32: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1071 "ascParse.y"
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
#line 2975 "ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1083 "ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 2984 "ascParse.c"
    break;

  case 34: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 1091 "ascParse.y"
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
#line 3038 "ascParse.c"
    break;

  case 35: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 1144 "ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 3050 "ascParse.c"
    break;

  case 36: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 1155 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3059 "ascParse.c"
    break;

  case 37: /* default_val: %empty  */
#line 1163 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3070 "ascParse.c"
    break;

  case 38: /* default_val: DEFAULT_TOK optional_sign number  */
#line 1170 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3080 "ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 1176 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 3092 "ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 1184 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 3104 "ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 1192 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 3116 "ascParse.c"
    break;

  case 42: /* constant_def: universal constant_head  */
#line 1203 "ascParse.y"
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
#line 3157 "ascParse.c"
    break;

  case 43: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK constant_dims constant_val optional_notes ';'  */
#line 1244 "ascParse.y"
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
#line 3191 "ascParse.c"
    break;

  case 44: /* constant_dims: DIMENSION_TOK dimensions  */
#line 1277 "ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	  g_constant_units = NULL;
	}
#line 3200 "ascParse.c"
    break;

  case 45: /* constant_dims: DIMENSIONLESS_TOK  */
#line 1282 "ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	  g_constant_units = NULL;
	}
#line 3209 "ascParse.c"
    break;

  case 46: /* constant_dims: %empty  */
#line 1287 "ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	  g_constant_units = NULL;
	}
#line 3218 "ascParse.c"
    break;

  case 47: /* constant_dims: UNITS_TOK BRACEDTEXT_TOK  */
#line 1292 "ascParse.y"
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
#line 3240 "ascParse.c"
    break;

  case 48: /* constant_val: %empty  */
#line 1313 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3251 "ascParse.c"
    break;

  case 49: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 1320 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3261 "ascParse.c"
    break;

  case 50: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 1326 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3273 "ascParse.c"
    break;

  case 51: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 1334 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3285 "ascParse.c"
    break;

  case 52: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 1342 "ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 3297 "ascParse.c"
    break;

  case 53: /* model_def: universal model_head fstatements methods end ';'  */
#line 1353 "ascParse.y"
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
#line 3339 "ascParse.c"
    break;

  case 54: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 1395 "ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 3351 "ascParse.c"
    break;

  case 55: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 1404 "ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 3364 "ascParse.c"
    break;

  case 56: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 1416 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3373 "ascParse.c"
    break;

  case 57: /* optional_model_parameters: %empty  */
#line 1424 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3381 "ascParse.c"
    break;

  case 58: /* optional_model_parameters: '(' fstatements ')'  */
#line 1428 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3389 "ascParse.c"
    break;

  case 59: /* optional_parameter_wheres: %empty  */
#line 1435 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3397 "ascParse.c"
    break;

  case 60: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 1439 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3405 "ascParse.c"
    break;

  case 61: /* optional_parameter_reduction: %empty  */
#line 1446 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3413 "ascParse.c"
    break;

  case 62: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 1450 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3421 "ascParse.c"
    break;

  case 63: /* universal: %empty  */
#line 1500 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3429 "ascParse.c"
    break;

  case 64: /* universal: UNIVERSAL_TOK  */
#line 1504 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3437 "ascParse.c"
    break;

  case 65: /* definition_def: definition_id fstatements methods end ';'  */
#line 1511 "ascParse.y"
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
#line 3468 "ascParse.c"
    break;

  case 66: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1541 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3477 "ascParse.c"
    break;

  case 67: /* units_def: units_statement ';'  */
#line 1550 "ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 3483 "ascParse.c"
    break;

  case 68: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1555 "ascParse.y"
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
#line 3504 "ascParse.c"
    break;

  case 69: /* units_statement: UNITS_TOK LADDER_TOK unitladderitemlist end optional_ladder_end  */
#line 1572 "ascParse.y"
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
#line 3530 "ascParse.c"
    break;

  case 70: /* optional_ladder_end: %empty  */
#line 1597 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3538 "ascParse.c"
    break;

  case 71: /* optional_ladder_end: LADDER_TOK  */
#line 1601 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3546 "ascParse.c"
    break;

  case 72: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on table_body END_TOK TABLE_TOK table_mode_off  */
#line 1608 "ascParse.y"
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
#line 3571 "ascParse.c"
    break;

  case 73: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on error END_TOK TABLE_TOK table_mode_off  */
#line 1629 "ascParse.y"
        {
	  DestroyName((yyvsp[-9].nptr));
	  TableParseAbort();
	  ErrMsg_Generic("Error in TABLE body.");
	  g_untrapped_error++;
	  yyerrok;
	  (yyval.statptr) = NULL;
	}
#line 3584 "ascParse.c"
    break;

  case 74: /* table_begin: %empty  */
#line 1641 "ascParse.y"
        {
	  TableParseBegin();
	}
#line 3592 "ascParse.c"
    break;

  case 75: /* table_mode_on: %empty  */
#line 1648 "ascParse.y"
        {
	  Asc_ScannerSetTableMode(1);
	}
#line 3600 "ascParse.c"
    break;

  case 76: /* table_mode_off: %empty  */
#line 1655 "ascParse.y"
        {
	  Asc_ScannerSetTableMode(0);
	}
#line 3608 "ascParse.c"
    break;

  case 79: /* table_decl_opt: %empty  */
#line 1667 "ascParse.y"
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
#line 3625 "ascParse.c"
    break;

  case 80: /* table_decl_opt: ISA_TOK type_identifier optional_of  */
#line 1680 "ascParse.y"
        {
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_type = (yyvsp[-1].id_ptr);
	  g_table_parse.decl_set_type = (yyvsp[0].id_ptr);
	  g_table_parse.decl_typeargs = g_typeargs;
	  g_typeargs = NULL;
	}
#line 3639 "ascParse.c"
    break;

  case 81: /* table_option: POSITIONAL_TOK  */
#line 1693 "ascParse.y"
        {
	  g_table_parse.positional = 1;
	}
#line 3647 "ascParse.c"
    break;

  case 82: /* table_option: DEFAULT_TOK expr  */
#line 1697 "ascParse.y"
        {
	  if (g_table_parse.default_expr != NULL) {
	    DestroyExprList(g_table_parse.default_expr);
	  }
	  g_table_parse.default_expr = (yyvsp[0].eptr);
	}
#line 3658 "ascParse.c"
    break;

  case 83: /* table_option: UNITS_TOK BRACEDTEXT_TOK  */
#line 1704 "ascParse.y"
        {
	  if (g_table_parse.units != NULL) {
	    ascfree(g_table_parse.units);
	  }
	  g_table_parse.units = ASC_STRDUP((yyvsp[0].braced_ptr));
	}
#line 3669 "ascParse.c"
    break;

  case 87: /* table_body_item: ':'  */
#line 1720 "ascParse.y"
        {
	  TableParseAppendToken(":",0);
	}
#line 3677 "ascParse.c"
    break;

  case 88: /* table_body_item: '='  */
#line 1724 "ascParse.y"
        {
	  TableParseAppendToken("=",0);
	}
#line 3685 "ascParse.c"
    break;

  case 89: /* table_body_item: ','  */
#line 1728 "ascParse.y"
        {
	  TableParseAppendToken(",",0);
	}
#line 3693 "ascParse.c"
    break;

  case 90: /* table_body_item: '+'  */
#line 1732 "ascParse.y"
        {
	  TableParseAppendToken("+",0);
	}
#line 3701 "ascParse.c"
    break;

  case 91: /* table_body_item: '-'  */
#line 1736 "ascParse.y"
        {
	  TableParseAppendToken("-",0);
	}
#line 3709 "ascParse.c"
    break;

  case 92: /* table_body_item: ';'  */
#line 1740 "ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3717 "ascParse.c"
    break;

  case 93: /* table_body_item: EOL_TOK  */
#line 1744 "ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3725 "ascParse.c"
    break;

  case 94: /* table_scalar: IDENTIFIER_TOK  */
#line 1751 "ascParse.y"
        {
	  TableParseAppendToken(SCP((yyvsp[0].id_ptr)),1);
	}
#line 3733 "ascParse.c"
    break;

  case 95: /* table_scalar: SYMBOL_TOK  */
#line 1755 "ascParse.y"
        {
	  TableParseAppendSymbol((yyvsp[0].sym_ptr));
	}
#line 3741 "ascParse.c"
    break;

  case 96: /* table_scalar: INTEGER_TOK  */
#line 1759 "ascParse.y"
        {
	  TableParseAppendInteger((yyvsp[0].int_value));
	}
#line 3749 "ascParse.c"
    break;

  case 97: /* table_scalar: REAL_TOK  */
#line 1763 "ascParse.y"
        {
	  TableParseAppendReal((yyvsp[0].real_value));
	}
#line 3757 "ascParse.c"
    break;

  case 98: /* table_scalar: BRACEDTEXT_TOK  */
#line 1767 "ascParse.y"
        {
	  TableParseAppendBraced((yyvsp[0].braced_ptr));
	}
#line 3765 "ascParse.c"
    break;

  case 99: /* values_statement: VALUES_TOK fname values_default_opt ';' values_entries END_TOK VALUES_TOK  */
#line 1774 "ascParse.y"
        {
	  DestroyName((yyvsp[-5].nptr));
	  (yyval.statptr) = NULL;
	}
#line 3774 "ascParse.c"
    break;

  case 101: /* values_default_opt: DEFAULT_TOK expr  */
#line 1783 "ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3782 "ascParse.c"
    break;

  case 104: /* values_entry: values_key_list '=' expr  */
#line 1795 "ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3790 "ascParse.c"
    break;

  case 111: /* $@1: %empty  */
#line 1814 "ascParse.y"
        {
	  DatasetParseBegin((yyvsp[-3].id_ptr),(yyvsp[-1].dquote_ptr));
	}
#line 3798 "ascParse.c"
    break;

  case 112: /* dataset_statement: DATASET_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';' $@1 dataset_items END_TOK DATASET_TOK  */
#line 1818 "ascParse.y"
        {
	  (yyval.statptr) = DatasetParseFinish();
	}
#line 3806 "ascParse.c"
    break;

  case 115: /* dataset_items: dataset_items error ';'  */
#line 1827 "ascParse.y"
        {
	  ErrMsg_Generic("Error in DATASET item.");
	  ErrMsg_Generic("Check DATASET statement syntax.");
	  g_untrapped_error++;
	  yyerrok;
	}
#line 3817 "ascParse.c"
    break;

  case 118: /* dataset_index_item: INDEX_TOK IDENTIFIER_TOK FROM_TOK COLUMN_TOK dataset_column_ref ISA_TOK IDENTIFIER_TOK  */
#line 1842 "ascParse.y"
        {
	  DatasetParseAddIndex((yyvsp[-5].id_ptr),(yyvsp[-2].id_ptr),(yyvsp[0].id_ptr));
	}
#line 3825 "ascParse.c"
    break;

  case 119: /* dataset_map_item: dataset_target FROM_TOK dataset_column_selector dataset_units_opt dataset_type_opt  */
#line 1849 "ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-2].id_ptr),(yyvsp[-1].braced_ptr),(yyvsp[0].id_ptr));
	}
#line 3833 "ascParse.c"
    break;

  case 120: /* dataset_map_item: dataset_target dataset_type_req FROM_TOK dataset_column_selector dataset_units_opt  */
#line 1853 "ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-1].id_ptr),(yyvsp[0].braced_ptr),(yyvsp[-3].id_ptr));
	}
#line 3841 "ascParse.c"
    break;

  case 121: /* dataset_column_selector: COLUMN_TOK dataset_column_ref  */
#line 1860 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3849 "ascParse.c"
    break;

  case 122: /* dataset_column_selector: dataset_column_ref  */
#line 1864 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3857 "ascParse.c"
    break;

  case 123: /* dataset_column_ref: IDENTIFIER_TOK  */
#line 1871 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3865 "ascParse.c"
    break;

  case 124: /* dataset_column_ref: SYMBOL_TOK  */
#line 1875 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].sym_ptr);
	}
#line 3873 "ascParse.c"
    break;

  case 125: /* dataset_target: IDENTIFIER_TOK '[' fvarlist ']'  */
#line 1882 "ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices(CreateIdName((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 3882 "ascParse.c"
    break;

  case 126: /* dataset_target: dataset_target '[' fvarlist ']'  */
#line 1887 "ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices((yyvsp[-3].nptr),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 3891 "ascParse.c"
    break;

  case 127: /* dataset_units_opt: %empty  */
#line 1895 "ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 3899 "ascParse.c"
    break;

  case 128: /* dataset_units_opt: BRACEDTEXT_TOK  */
#line 1899 "ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 3907 "ascParse.c"
    break;

  case 129: /* dataset_type_opt: %empty  */
#line 1906 "ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 3915 "ascParse.c"
    break;

  case 130: /* dataset_type_opt: ISA_TOK IDENTIFIER_TOK  */
#line 1910 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3923 "ascParse.c"
    break;

  case 131: /* dataset_type_req: ISA_TOK IDENTIFIER_TOK  */
#line 1917 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3931 "ascParse.c"
    break;

  case 132: /* unitdeflist: %empty  */
#line 1923 "ascParse.y"
                {
		  (yyval.listp) = gl_create(100L);
	}
#line 3939 "ascParse.c"
    break;

  case 133: /* unitdeflist: unitdeflist unitdef  */
#line 1927 "ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 3948 "ascParse.c"
    break;

  case 134: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1935 "ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 3957 "ascParse.c"
    break;

  case 135: /* unitladderitemlist: %empty  */
#line 1942 "ascParse.y"
        {
	  (yyval.listp) = gl_create(20L);
	}
#line 3965 "ascParse.c"
    break;

  case 136: /* unitladderitemlist: unitladderitemlist unitladderitem ';'  */
#line 1946 "ascParse.y"
        {
	  gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].ulitemptr));
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 3974 "ascParse.c"
    break;

  case 137: /* unitladderitem: IDENTIFIER_TOK '=' BRACEDTEXT_TOK  */
#line 1954 "ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[-2].id_ptr),(yyvsp[0].braced_ptr),0,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 3983 "ascParse.c"
    break;

  case 138: /* unitladderitem: IDENTIFIER_TOK  */
#line 1959 "ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[0].id_ptr),NULL,1,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 3992 "ascParse.c"
    break;

  case 139: /* methods: %empty  */
#line 1968 "ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 4000 "ascParse.c"
    break;

  case 140: /* $@2: %empty  */
#line 1972 "ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 4011 "ascParse.c"
    break;

  case 141: /* methods: METHODS_TOK $@2 proclist  */
#line 1979 "ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 4019 "ascParse.c"
    break;

  case 142: /* proclist: proclistf  */
#line 1986 "ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 4028 "ascParse.c"
    break;

  case 143: /* proclistf: %empty  */
#line 1993 "ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4036 "ascParse.c"
    break;

  case 144: /* proclistf: proclistf procedure  */
#line 1997 "ascParse.y"
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
#line 4062 "ascParse.c"
    break;

  case 145: /* procedure: procedure_id ';' fstatements end ';'  */
#line 2022 "ascParse.y"
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
#line 4077 "ascParse.c"
    break;

  case 146: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 2036 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 4086 "ascParse.c"
    break;

  case 147: /* fstatements: statements  */
#line 2045 "ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 4094 "ascParse.c"
    break;

  case 148: /* statements: %empty  */
#line 2052 "ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4102 "ascParse.c"
    break;

  case 149: /* statements: statements statement ';'  */
#line 2056 "ascParse.y"
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
#line 4172 "ascParse.c"
    break;

  case 150: /* statements: statements complex_statement ';'  */
#line 2122 "ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
            gl_destroy((yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4184 "ascParse.c"
    break;

  case 151: /* statements: statements error ';'  */
#line 2130 "ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4193 "ascParse.c"
    break;

  case 192: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 2186 "ascParse.y"
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
#line 4235 "ascParse.c"
    break;

  case 193: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 2227 "ascParse.y"
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
#line 4265 "ascParse.c"
    break;

  case 194: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 2256 "ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 4273 "ascParse.c"
    break;

  case 195: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 2261 "ascParse.y"
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
#line 4323 "ascParse.c"
    break;

  case 196: /* optional_set_values: %empty  */
#line 2310 "ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4331 "ascParse.c"
    break;

  case 197: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 2314 "ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 4339 "ascParse.c"
    break;

  case 198: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 2321 "ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 4352 "ascParse.c"
    break;

  case 199: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 2333 "ascParse.y"
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
#line 4380 "ascParse.c"
    break;

  case 200: /* call_identifier: IDENTIFIER_TOK  */
#line 2360 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 4389 "ascParse.c"
    break;

  case 201: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2365 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 4398 "ascParse.c"
    break;

  case 202: /* type_identifier: IDENTIFIER_TOK  */
#line 2373 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 4407 "ascParse.c"
    break;

  case 203: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2378 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 4416 "ascParse.c"
    break;

  case 204: /* optional_method: %empty  */
#line 2386 "ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4424 "ascParse.c"
    break;

  case 205: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 2390 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4432 "ascParse.c"
    break;

  case 206: /* optional_of: %empty  */
#line 2397 "ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4440 "ascParse.c"
    break;

  case 207: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 2401 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4448 "ascParse.c"
    break;

  case 208: /* optional_with_value: %empty  */
#line 2408 "ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 4456 "ascParse.c"
    break;

  case 209: /* optional_with_value: WITH_VALUE_T expr  */
#line 2412 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4464 "ascParse.c"
    break;

  case 210: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 2419 "ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 4472 "ascParse.c"
    break;

  case 211: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 2426 "ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4480 "ascParse.c"
    break;

  case 212: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2430 "ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4488 "ascParse.c"
    break;

  case 213: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 2434 "ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4496 "ascParse.c"
    break;

  case 214: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2441 "ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4504 "ascParse.c"
    break;

  case 215: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 2445 "ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4512 "ascParse.c"
    break;

  case 216: /* der_statement: DER_TOK '(' fvarlist ')'  */
#line 2452 "ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 4522 "ascParse.c"
    break;

  case 217: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 2461 "ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 4532 "ascParse.c"
    break;

  case 218: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 2470 "ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 4540 "ascParse.c"
    break;

  case 219: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 2477 "ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 4548 "ascParse.c"
    break;

  case 220: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 2484 "ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 4556 "ascParse.c"
    break;

  case 221: /* assignment_statement: fname ASSIGN_TOK expr  */
#line 2491 "ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4564 "ascParse.c"
    break;

  case 222: /* assignment_statement: fname CASSIGN_TOK expr  */
#line 2495 "ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4572 "ascParse.c"
    break;

  case 223: /* relation_statement: relation  */
#line 2502 "ascParse.y"
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
#line 4589 "ascParse.c"
    break;

  case 224: /* relation_statement: fname ':' relation  */
#line 2515 "ascParse.y"
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
#line 4607 "ascParse.c"
    break;

  case 225: /* relation: expr  */
#line 2532 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 4621 "ascParse.c"
    break;

  case 226: /* relation: MINIMIZE_TOK expr  */
#line 2542 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4633 "ascParse.c"
    break;

  case 227: /* relation: MAXIMIZE_TOK expr  */
#line 2550 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4645 "ascParse.c"
    break;

  case 228: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 2561 "ascParse.y"
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
#line 4667 "ascParse.c"
    break;

  case 229: /* input_args: fvarlist ':' INPUT_TOK  */
#line 2582 "ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4675 "ascParse.c"
    break;

  case 230: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 2589 "ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4683 "ascParse.c"
    break;

  case 231: /* data_args: %empty  */
#line 2596 "ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 4691 "ascParse.c"
    break;

  case 232: /* data_args: ';' fname ':' DATA_TOK  */
#line 2600 "ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 4699 "ascParse.c"
    break;

  case 233: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 2638 "ascParse.y"
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
#line 4720 "ascParse.c"
    break;

  case 234: /* optional_direction: %empty  */
#line 2658 "ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 4728 "ascParse.c"
    break;

  case 235: /* optional_direction: INCREASING_TOK  */
#line 2662 "ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 4736 "ascParse.c"
    break;

  case 236: /* optional_direction: DECREASING_TOK  */
#line 2666 "ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 4744 "ascParse.c"
    break;

  case 237: /* forexprend: CREATE_TOK  */
#line 2673 "ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 4752 "ascParse.c"
    break;

  case 238: /* forexprend: EXPECT_TOK  */
#line 2677 "ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 4760 "ascParse.c"
    break;

  case 239: /* forexprend: CHECK_TOK  */
#line 2681 "ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 4768 "ascParse.c"
    break;

  case 240: /* forexprend: DO_TOK  */
#line 2685 "ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 4776 "ascParse.c"
    break;

  case 241: /* run_statement: RUN_TOK fname  */
#line 2692 "ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 4784 "ascParse.c"
    break;

  case 242: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 2696 "ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 4792 "ascParse.c"
    break;

  case 243: /* fix_statement: FIX_TOK fvarlist  */
#line 2703 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 4801 "ascParse.c"
    break;

  case 244: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 2711 "ascParse.y"
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
#line 4816 "ascParse.c"
    break;

  case 245: /* free_statement: FREE_TOK fvarlist  */
#line 2725 "ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 4824 "ascParse.c"
    break;

  case 246: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 2732 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 4833 "ascParse.c"
    break;

  case 247: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 2740 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 4842 "ascParse.c"
    break;

  case 248: /* solve_statement: SOLVE_TOK  */
#line 2748 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE();
	}
#line 4851 "ascParse.c"
    break;

  case 249: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 2756 "ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 4863 "ascParse.c"
    break;

  case 250: /* call_statement: CALL_TOK call_identifier  */
#line 2767 "ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 4875 "ascParse.c"
    break;

  case 251: /* assert_statement: ASSERT_TOK expr  */
#line 2778 "ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 4883 "ascParse.c"
    break;

  case 252: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 2784 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 4894 "ascParse.c"
    break;

  case 253: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 2794 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 4905 "ascParse.c"
    break;

  case 254: /* optional_else: %empty  */
#line 2803 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 4913 "ascParse.c"
    break;

  case 255: /* optional_else: ELSE_TOK fstatements  */
#line 2807 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 4921 "ascParse.c"
    break;

  case 256: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 2814 "ascParse.y"
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
#line 4936 "ascParse.c"
    break;

  case 257: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 2825 "ascParse.y"
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
#line 4952 "ascParse.c"
    break;

  case 258: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2837 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 4963 "ascParse.c"
    break;

  case 259: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2844 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 4974 "ascParse.c"
    break;

  case 260: /* whenlist: whenlistf  */
#line 2854 "ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 4982 "ascParse.c"
    break;

  case 261: /* whenlistf: CASE_TOK set ':' fstatements  */
#line 2861 "ascParse.y"
        {
	  (yyval.wptr) = CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 4990 "ascParse.c"
    break;

  case 262: /* whenlistf: OTHERWISE_TOK ':' fstatements  */
#line 2865 "ascParse.y"
        {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	}
#line 4998 "ascParse.c"
    break;

  case 263: /* whenlistf: whenlistf CASE_TOK set ':' fstatements  */
#line 2869 "ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	}
#line 5006 "ascParse.c"
    break;

  case 264: /* whenlistf: whenlistf OTHERWISE_TOK ':' fstatements  */
#line 2873 "ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	}
#line 5014 "ascParse.c"
    break;

  case 265: /* flow_statement: BREAK_TOK  */
#line 2880 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 5022 "ascParse.c"
    break;

  case 266: /* flow_statement: CONTINUE_TOK  */
#line 2884 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 5030 "ascParse.c"
    break;

  case 267: /* flow_statement: FALLTHRU_TOK  */
#line 2888 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 5038 "ascParse.c"
    break;

  case 268: /* flow_statement: RETURN_TOK  */
#line 2892 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 5046 "ascParse.c"
    break;

  case 269: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 2896 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 5054 "ascParse.c"
    break;

  case 270: /* use_statement: USE_TOK fname  */
#line 2903 "ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 5062 "ascParse.c"
    break;

  case 271: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 2910 "ascParse.y"
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
#line 5077 "ascParse.c"
    break;

  case 272: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 2921 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 5088 "ascParse.c"
    break;

  case 273: /* selectlist: selectlistf  */
#line 2931 "ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 5096 "ascParse.c"
    break;

  case 274: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 2938 "ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5104 "ascParse.c"
    break;

  case 275: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 2942 "ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 5112 "ascParse.c"
    break;

  case 276: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 2946 "ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 5120 "ascParse.c"
    break;

  case 277: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 2950 "ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 5128 "ascParse.c"
    break;

  case 278: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 2957 "ascParse.y"
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
#line 5143 "ascParse.c"
    break;

  case 279: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 2968 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 5154 "ascParse.c"
    break;

  case 280: /* switchlist: switchlistf  */
#line 2978 "ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 5162 "ascParse.c"
    break;

  case 281: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 2985 "ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5170 "ascParse.c"
    break;

  case 282: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 2989 "ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 5178 "ascParse.c"
    break;

  case 283: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 2993 "ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 5186 "ascParse.c"
    break;

  case 284: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 2997 "ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 5194 "ascParse.c"
    break;

  case 285: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 3004 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 5205 "ascParse.c"
    break;

  case 286: /* notes_statement: NOTES_TOK notes_body end  */
#line 3014 "ascParse.y"
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
#line 5243 "ascParse.c"
    break;

  case 287: /* notes_body: SYMBOL_TOK noteslist  */
#line 3051 "ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 5256 "ascParse.c"
    break;

  case 288: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 3060 "ascParse.y"
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
#line 5272 "ascParse.c"
    break;

  case 289: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 3075 "ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 5281 "ascParse.c"
    break;

  case 290: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 3080 "ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 5291 "ascParse.c"
    break;

  case 291: /* fvarlist: varlist  */
#line 3089 "ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5303 "ascParse.c"
    break;

  case 292: /* varlist: fname  */
#line 3100 "ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5311 "ascParse.c"
    break;

  case 293: /* varlist: varlist ',' fname  */
#line 3104 "ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5320 "ascParse.c"
    break;

  case 294: /* varlist: varlist fname  */
#line 3109 "ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 5334 "ascParse.c"
    break;

  case 295: /* fname: name optional_notes  */
#line 3122 "ascParse.y"
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
#line 5355 "ascParse.c"
    break;

  case 296: /* name: IDENTIFIER_TOK  */
#line 3142 "ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 5363 "ascParse.c"
    break;

  case 297: /* name: name '.' IDENTIFIER_TOK  */
#line 3146 "ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5372 "ascParse.c"
    break;

  case 298: /* name: name '[' set ']'  */
#line 3151 "ascParse.y"
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
#line 5388 "ascParse.c"
    break;

  case 299: /* end: END_TOK CONDITIONAL_TOK  */
#line 3166 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 5397 "ascParse.c"
    break;

  case 300: /* end: END_TOK FOR_TOK  */
#line 3171 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 5406 "ascParse.c"
    break;

  case 301: /* end: END_TOK IF_TOK  */
#line 3176 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 5415 "ascParse.c"
    break;

  case 302: /* end: END_TOK INTERACTIVE_TOK  */
#line 3181 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 5424 "ascParse.c"
    break;

  case 303: /* end: END_TOK METHODS_TOK  */
#line 3186 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 5433 "ascParse.c"
    break;

  case 304: /* end: END_TOK NOTES_TOK  */
#line 3191 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 5442 "ascParse.c"
    break;

  case 305: /* end: END_TOK SELECT_TOK  */
#line 3196 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 5451 "ascParse.c"
    break;

  case 306: /* end: END_TOK SWITCH_TOK  */
#line 3201 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 5460 "ascParse.c"
    break;

  case 307: /* end: END_TOK UNITS_TOK  */
#line 3206 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 5469 "ascParse.c"
    break;

  case 308: /* end: END_TOK GLOBAL_TOK  */
#line 3211 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 5478 "ascParse.c"
    break;

  case 309: /* end: END_TOK WHEN_TOK  */
#line 3216 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 5487 "ascParse.c"
    break;

  case 310: /* end: END_TOK WHILE_TOK  */
#line 3221 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 5496 "ascParse.c"
    break;

  case 311: /* end: END_TOK IDENTIFIER_TOK  */
#line 3226 "ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 5505 "ascParse.c"
    break;

  case 312: /* end: END_TOK  */
#line 3231 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 5514 "ascParse.c"
    break;

  case 313: /* optional_bracedtext: %empty  */
#line 3239 "ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 5522 "ascParse.c"
    break;

  case 314: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 3243 "ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 5530 "ascParse.c"
    break;

  case 315: /* optional_notes: %empty  */
#line 3250 "ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 5538 "ascParse.c"
    break;

  case 316: /* optional_notes: DQUOTE_TOK  */
#line 3254 "ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 5546 "ascParse.c"
    break;

  case 317: /* set: setexprlist  */
#line 3261 "ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 5554 "ascParse.c"
    break;

  case 318: /* set: %empty  */
#line 3265 "ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 5562 "ascParse.c"
    break;

  case 319: /* setexprlist: expr  */
#line 3272 "ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 5570 "ascParse.c"
    break;

  case 320: /* setexprlist: expr DOTDOT_TOK expr  */
#line 3276 "ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5578 "ascParse.c"
    break;

  case 321: /* setexprlist: setexprlist ',' expr  */
#line 3280 "ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 5587 "ascParse.c"
    break;

  case 322: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 3285 "ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 5596 "ascParse.c"
    break;

  case 323: /* number: INTEGER_TOK  */
#line 3293 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	  g_number_units = NULL;
	}
#line 5607 "ascParse.c"
    break;

  case 324: /* number: realnumber  */
#line 3300 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 5617 "ascParse.c"
    break;

  case 325: /* realnumber: REAL_TOK opunits  */
#line 3309 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	  g_number_units = g_parsed_units;
	}
#line 5626 "ascParse.c"
    break;

  case 326: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 3314 "ascParse.y"
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
#line 5651 "ascParse.c"
    break;

  case 327: /* opunits: %empty  */
#line 3338 "ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  g_parsed_units = NULL;
	  (yyval.real_value) = 1.0;
	}
#line 5661 "ascParse.c"
    break;

  case 328: /* opunits: BRACEDTEXT_TOK  */
#line 3344 "ascParse.y"
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
#line 5685 "ascParse.c"
    break;

  case 329: /* dims: DIMENSION_TOK dimensions  */
#line 3367 "ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 5693 "ascParse.c"
    break;

  case 330: /* dims: DIMENSIONLESS_TOK  */
#line 3371 "ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 5701 "ascParse.c"
    break;

  case 331: /* dims: %empty  */
#line 3375 "ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 5709 "ascParse.c"
    break;

  case 332: /* dimensions: '*'  */
#line 3382 "ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 5717 "ascParse.c"
    break;

  case 333: /* dimensions: dimexpr  */
#line 3386 "ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 5725 "ascParse.c"
    break;

  case 334: /* dimexpr: IDENTIFIER_TOK  */
#line 3393 "ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 5733 "ascParse.c"
    break;

  case 335: /* dimexpr: INTEGER_TOK  */
#line 3397 "ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 5741 "ascParse.c"
    break;

  case 336: /* dimexpr: dimexpr '/' dimexpr  */
#line 3401 "ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 5749 "ascParse.c"
    break;

  case 337: /* dimexpr: dimexpr '*' dimexpr  */
#line 3405 "ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 5757 "ascParse.c"
    break;

  case 338: /* dimexpr: dimexpr '^' fraction  */
#line 3409 "ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 5765 "ascParse.c"
    break;

  case 339: /* dimexpr: '(' dimexpr ')'  */
#line 3413 "ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 5773 "ascParse.c"
    break;

  case 340: /* fraction: optional_sign fractail  */
#line 3420 "ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 5781 "ascParse.c"
    break;

  case 341: /* fractail: INTEGER_TOK  */
#line 3427 "ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 5789 "ascParse.c"
    break;

  case 342: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 3431 "ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 5797 "ascParse.c"
    break;

  case 343: /* optional_sign: %empty  */
#line 3438 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 5805 "ascParse.c"
    break;

  case 344: /* optional_sign: '+'  */
#line 3442 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 5813 "ascParse.c"
    break;

  case 345: /* optional_sign: '-'  */
#line 3446 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 5821 "ascParse.c"
    break;

  case 346: /* expr: INTEGER_TOK  */
#line 3453 "ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 5829 "ascParse.c"
    break;

  case 347: /* expr: MAXINTEGER_TOK  */
#line 3457 "ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 5837 "ascParse.c"
    break;

  case 348: /* expr: realnumber  */
#line 3461 "ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 5845 "ascParse.c"
    break;

  case 349: /* expr: MAXREAL_TOK  */
#line 3465 "ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 5853 "ascParse.c"
    break;

  case 350: /* expr: TRUE_TOK  */
#line 3469 "ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 5861 "ascParse.c"
    break;

  case 351: /* expr: FALSE_TOK  */
#line 3473 "ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 5869 "ascParse.c"
    break;

  case 352: /* expr: ANY_TOK  */
#line 3477 "ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 5877 "ascParse.c"
    break;

  case 353: /* expr: SYMBOL_TOK  */
#line 3481 "ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 5885 "ascParse.c"
    break;

  case 354: /* expr: fname  */
#line 3485 "ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 5893 "ascParse.c"
    break;

  case 355: /* expr: '[' set ']'  */
#line 3489 "ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 5901 "ascParse.c"
    break;

  case 356: /* expr: expr '+' expr  */
#line 3493 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5910 "ascParse.c"
    break;

  case 357: /* expr: expr '-' expr  */
#line 3498 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5919 "ascParse.c"
    break;

  case 358: /* expr: expr '*' expr  */
#line 3503 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5928 "ascParse.c"
    break;

  case 359: /* expr: expr '/' expr  */
#line 3508 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5937 "ascParse.c"
    break;

  case 360: /* expr: expr '^' expr  */
#line 3513 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5946 "ascParse.c"
    break;

  case 361: /* expr: expr AND_TOK expr  */
#line 3518 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5955 "ascParse.c"
    break;

  case 362: /* expr: expr OR_TOK expr  */
#line 3523 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5964 "ascParse.c"
    break;

  case 363: /* expr: NOT_TOK expr  */
#line 3528 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 5972 "ascParse.c"
    break;

  case 364: /* expr: expr relop expr  */
#line 3532 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5981 "ascParse.c"
    break;

  case 365: /* expr: expr logrelop expr  */
#line 3537 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5990 "ascParse.c"
    break;

  case 366: /* expr: expr IN_TOK expr  */
#line 3542 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5999 "ascParse.c"
    break;

  case 367: /* expr: expr '|' expr  */
#line 3547 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6008 "ascParse.c"
    break;

  case 368: /* expr: expr SUCHTHAT_TOK expr  */
#line 3552 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6017 "ascParse.c"
    break;

  case 369: /* expr: '+' expr  */
#line 3557 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 6025 "ascParse.c"
    break;

  case 370: /* expr: '-' expr  */
#line 3561 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 6033 "ascParse.c"
    break;

  case 371: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 3565 "ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 6041 "ascParse.c"
    break;

  case 372: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 3569 "ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 6049 "ascParse.c"
    break;

  case 373: /* expr: SUM_TOK '(' set ')'  */
#line 3573 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 6060 "ascParse.c"
    break;

  case 374: /* expr: SUM_TOK '[' set ']'  */
#line 3580 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 6068 "ascParse.c"
    break;

  case 375: /* expr: PROD_TOK '(' set ')'  */
#line 3584 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 6079 "ascParse.c"
    break;

  case 376: /* expr: PROD_TOK '[' set ']'  */
#line 3591 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 6087 "ascParse.c"
    break;

  case 377: /* expr: UNION_TOK '(' set ')'  */
#line 3595 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 6098 "ascParse.c"
    break;

  case 378: /* expr: UNION_TOK '[' set ']'  */
#line 3602 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 6106 "ascParse.c"
    break;

  case 379: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 3606 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 6117 "ascParse.c"
    break;

  case 380: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 3613 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 6125 "ascParse.c"
    break;

  case 381: /* expr: CARD_TOK '(' set ')'  */
#line 3617 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 6136 "ascParse.c"
    break;

  case 382: /* expr: CARD_TOK '[' set ']'  */
#line 3624 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 6144 "ascParse.c"
    break;

  case 383: /* expr: CHOICE_TOK '(' set ')'  */
#line 3628 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 6155 "ascParse.c"
    break;

  case 384: /* expr: CHOICE_TOK '[' set ']'  */
#line 3635 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 6163 "ascParse.c"
    break;

  case 385: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 3639 "ascParse.y"
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
#line 6178 "ascParse.c"
    break;

  case 386: /* expr: '(' expr ')'  */
#line 3650 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 6186 "ascParse.c"
    break;

  case 387: /* relop: '='  */
#line 3657 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 6194 "ascParse.c"
    break;

  case 388: /* relop: '<'  */
#line 3661 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 6202 "ascParse.c"
    break;

  case 389: /* relop: '>'  */
#line 3665 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 6210 "ascParse.c"
    break;

  case 390: /* relop: LEQ_TOK  */
#line 3669 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 6218 "ascParse.c"
    break;

  case 391: /* relop: GEQ_TOK  */
#line 3673 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 6226 "ascParse.c"
    break;

  case 392: /* relop: NEQ_TOK  */
#line 3677 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 6234 "ascParse.c"
    break;

  case 393: /* logrelop: BEQ_TOK  */
#line 3684 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 6242 "ascParse.c"
    break;

  case 394: /* logrelop: BNE_TOK  */
#line 3688 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 6250 "ascParse.c"
    break;


#line 6254 "ascParse.c"

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

#line 3692 "ascParse.y"

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
