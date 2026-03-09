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
  spec.filename = NULL;
  return spec;
}

/* For 'inline' notes, note on DQUOTE_TOK from scanner.l:
 * Remember that DQUOTE_TOK is a string value which is local to the
 * production that finds it. It must be copied if you want to
 * keep it.
 */

/* MS VC++ won't compiler Bison output unless we switch this */
#ifdef _MSC_VER
# define __STDC__
#endif


#line 779 "ascend/compiler/ascParse.c"

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
  YYSYMBOL_INPUT_TOK = 49,                 /* INPUT_TOK  */
  YYSYMBOL_INCREASING_TOK = 50,            /* INCREASING_TOK  */
  YYSYMBOL_INTERACTIVE_TOK = 51,           /* INTERACTIVE_TOK  */
  YYSYMBOL_INDEPENDENT_TOK = 52,           /* INDEPENDENT_TOK  */
  YYSYMBOL_INTERSECTION_TOK = 53,          /* INTERSECTION_TOK  */
  YYSYMBOL_ISA_TOK = 54,                   /* ISA_TOK  */
  YYSYMBOL__IS_T = 55,                     /* _IS_T  */
  YYSYMBOL_ISREFINEDTO_TOK = 56,           /* ISREFINEDTO_TOK  */
  YYSYMBOL_LINEAR_TOK = 57,                /* LINEAR_TOK  */
  YYSYMBOL_LOG_TOK = 58,                   /* LOG_TOK  */
  YYSYMBOL_LINK_TOK = 59,                  /* LINK_TOK  */
  YYSYMBOL_MAXIMIZE_TOK = 60,              /* MAXIMIZE_TOK  */
  YYSYMBOL_MAXINTEGER_TOK = 61,            /* MAXINTEGER_TOK  */
  YYSYMBOL_MAXREAL_TOK = 62,               /* MAXREAL_TOK  */
  YYSYMBOL_METHODS_TOK = 63,               /* METHODS_TOK  */
  YYSYMBOL_METHOD_TOK = 64,                /* METHOD_TOK  */
  YYSYMBOL_MINIMIZE_TOK = 65,              /* MINIMIZE_TOK  */
  YYSYMBOL_MODEL_TOK = 66,                 /* MODEL_TOK  */
  YYSYMBOL_NOT_TOK = 67,                   /* NOT_TOK  */
  YYSYMBOL_NOTES_TOK = 68,                 /* NOTES_TOK  */
  YYSYMBOL_OF_TOK = 69,                    /* OF_TOK  */
  YYSYMBOL_OPTION_TOK = 70,                /* OPTION_TOK  */
  YYSYMBOL_OR_TOK = 71,                    /* OR_TOK  */
  YYSYMBOL_OTHERWISE_TOK = 72,             /* OTHERWISE_TOK  */
  YYSYMBOL_OUTPUT_TOK = 73,                /* OUTPUT_TOK  */
  YYSYMBOL_PROD_TOK = 74,                  /* PROD_TOK  */
  YYSYMBOL_PROVIDE_TOK = 75,               /* PROVIDE_TOK  */
  YYSYMBOL_RATIO_TOK = 76,                 /* RATIO_TOK  */
  YYSYMBOL_REFINES_TOK = 77,               /* REFINES_TOK  */
  YYSYMBOL_REPLACE_TOK = 78,               /* REPLACE_TOK  */
  YYSYMBOL_REQUIRE_TOK = 79,               /* REQUIRE_TOK  */
  YYSYMBOL_RETURN_TOK = 80,                /* RETURN_TOK  */
  YYSYMBOL_RUN_TOK = 81,                   /* RUN_TOK  */
  YYSYMBOL_SATISFIED_TOK = 82,             /* SATISFIED_TOK  */
  YYSYMBOL_SELECT_TOK = 83,                /* SELECT_TOK  */
  YYSYMBOL_SIZE_TOK = 84,                  /* SIZE_TOK  */
  YYSYMBOL_SOLVE_TOK = 85,                 /* SOLVE_TOK  */
  YYSYMBOL_SOLVER_TOK = 86,                /* SOLVER_TOK  */
  YYSYMBOL_STOP_TOK = 87,                  /* STOP_TOK  */
  YYSYMBOL_SUCHTHAT_TOK = 88,              /* SUCHTHAT_TOK  */
  YYSYMBOL_SUM_TOK = 89,                   /* SUM_TOK  */
  YYSYMBOL_SWITCH_TOK = 90,                /* SWITCH_TOK  */
  YYSYMBOL_SYSTEM_TOK = 91,                /* SYSTEM_TOK  */
  YYSYMBOL_STEP_TOK = 92,                  /* STEP_TOK  */
  YYSYMBOL_STEPS_TOK = 93,                 /* STEPS_TOK  */
  YYSYMBOL_STUDY_TOK = 94,                 /* STUDY_TOK  */
  YYSYMBOL_TABLE_TOK = 95,                 /* TABLE_TOK  */
  YYSYMBOL_VALUES_TOK = 96,                /* VALUES_TOK  */
  YYSYMBOL_DATASET_TOK = 97,               /* DATASET_TOK  */
  YYSYMBOL_POSITIONAL_TOK = 98,            /* POSITIONAL_TOK  */
  YYSYMBOL_INDEX_TOK = 99,                 /* INDEX_TOK  */
  YYSYMBOL_COLUMN_TOK = 100,               /* COLUMN_TOK  */
  YYSYMBOL_EOL_TOK = 101,                  /* EOL_TOK  */
  YYSYMBOL_THEN_TOK = 102,                 /* THEN_TOK  */
  YYSYMBOL_TO_TOK = 103,                   /* TO_TOK  */
  YYSYMBOL_TRUE_TOK = 104,                 /* TRUE_TOK  */
  YYSYMBOL_UNION_TOK = 105,                /* UNION_TOK  */
  YYSYMBOL_UNITS_TOK = 106,                /* UNITS_TOK  */
  YYSYMBOL_LADDER_TOK = 107,               /* LADDER_TOK  */
  YYSYMBOL_UNIVERSAL_TOK = 108,            /* UNIVERSAL_TOK  */
  YYSYMBOL_UNLINK_TOK = 109,               /* UNLINK_TOK  */
  YYSYMBOL_VARY_TOK = 110,                 /* VARY_TOK  */
  YYSYMBOL_WHEN_TOK = 111,                 /* WHEN_TOK  */
  YYSYMBOL_WHERE_TOK = 112,                /* WHERE_TOK  */
  YYSYMBOL_WHILE_TOK = 113,                /* WHILE_TOK  */
  YYSYMBOL_WILLBE_TOK = 114,               /* WILLBE_TOK  */
  YYSYMBOL_WILLBETHESAME_TOK = 115,        /* WILLBETHESAME_TOK  */
  YYSYMBOL_WILLNOTBETHESAME_TOK = 116,     /* WILLNOTBETHESAME_TOK  */
  YYSYMBOL_ASSIGN_TOK = 117,               /* ASSIGN_TOK  */
  YYSYMBOL_CASSIGN_TOK = 118,              /* CASSIGN_TOK  */
  YYSYMBOL_DBLCOLON_TOK = 119,             /* DBLCOLON_TOK  */
  YYSYMBOL_USE_TOK = 120,                  /* USE_TOK  */
  YYSYMBOL_LEQ_TOK = 121,                  /* LEQ_TOK  */
  YYSYMBOL_GEQ_TOK = 122,                  /* GEQ_TOK  */
  YYSYMBOL_NEQ_TOK = 123,                  /* NEQ_TOK  */
  YYSYMBOL_DOTDOT_TOK = 124,               /* DOTDOT_TOK  */
  YYSYMBOL_WITH_TOK = 125,                 /* WITH_TOK  */
  YYSYMBOL_VALUE_TOK = 126,                /* VALUE_TOK  */
  YYSYMBOL_WITH_VALUE_T = 127,             /* WITH_VALUE_T  */
  YYSYMBOL_REAL_TOK = 128,                 /* REAL_TOK  */
  YYSYMBOL_INTEGER_TOK = 129,              /* INTEGER_TOK  */
  YYSYMBOL_IDENTIFIER_TOK = 130,           /* IDENTIFIER_TOK  */
  YYSYMBOL_BRACEDTEXT_TOK = 131,           /* BRACEDTEXT_TOK  */
  YYSYMBOL_SYMBOL_TOK = 132,               /* SYMBOL_TOK  */
  YYSYMBOL_DQUOTE_TOK = 133,               /* DQUOTE_TOK  */
  YYSYMBOL_134_ = 134,                     /* ','  */
  YYSYMBOL_135_ = 135,                     /* '|'  */
  YYSYMBOL_136_ = 136,                     /* '<'  */
  YYSYMBOL_137_ = 137,                     /* '='  */
  YYSYMBOL_138_ = 138,                     /* '>'  */
  YYSYMBOL_139_ = 139,                     /* '+'  */
  YYSYMBOL_140_ = 140,                     /* '-'  */
  YYSYMBOL_141_ = 141,                     /* '/'  */
  YYSYMBOL_142_ = 142,                     /* '*'  */
  YYSYMBOL_UMINUS_TOK = 143,               /* UMINUS_TOK  */
  YYSYMBOL_UPLUS_TOK = 144,                /* UPLUS_TOK  */
  YYSYMBOL_145_ = 145,                     /* '^'  */
  YYSYMBOL_146_ = 146,                     /* ';'  */
  YYSYMBOL_147_ = 147,                     /* '('  */
  YYSYMBOL_148_ = 148,                     /* ')'  */
  YYSYMBOL_149_ = 149,                     /* ':'  */
  YYSYMBOL_150_ = 150,                     /* '['  */
  YYSYMBOL_151_ = 151,                     /* ']'  */
  YYSYMBOL_152_ = 152,                     /* '.'  */
  YYSYMBOL_YYACCEPT = 153,                 /* $accept  */
  YYSYMBOL_definitions = 154,              /* definitions  */
  YYSYMBOL_definition = 155,               /* definition  */
  YYSYMBOL_global_def = 156,               /* global_def  */
  YYSYMBOL_require_file = 157,             /* require_file  */
  YYSYMBOL_provide_module = 158,           /* provide_module  */
  YYSYMBOL_import = 159,                   /* import  */
  YYSYMBOL_add_notes_def = 160,            /* add_notes_def  */
  YYSYMBOL_add_notes_head = 161,           /* add_notes_head  */
  YYSYMBOL_add_method_def = 162,           /* add_method_def  */
  YYSYMBOL_add_method_head = 163,          /* add_method_head  */
  YYSYMBOL_replace_method_def = 164,       /* replace_method_def  */
  YYSYMBOL_replace_method_head = 165,      /* replace_method_head  */
  YYSYMBOL_atom_def = 166,                 /* atom_def  */
  YYSYMBOL_atom_head = 167,                /* atom_head  */
  YYSYMBOL_atom_id = 168,                  /* atom_id  */
  YYSYMBOL_default_val = 169,              /* default_val  */
  YYSYMBOL_constant_def = 170,             /* constant_def  */
  YYSYMBOL_constant_head = 171,            /* constant_head  */
  YYSYMBOL_constant_dims = 172,            /* constant_dims  */
  YYSYMBOL_constant_val = 173,             /* constant_val  */
  YYSYMBOL_model_def = 174,                /* model_def  */
  YYSYMBOL_model_head = 175,               /* model_head  */
  YYSYMBOL_model_id = 176,                 /* model_id  */
  YYSYMBOL_optional_model_parameters = 177, /* optional_model_parameters  */
  YYSYMBOL_optional_parameter_wheres = 178, /* optional_parameter_wheres  */
  YYSYMBOL_optional_parameter_reduction = 179, /* optional_parameter_reduction  */
  YYSYMBOL_universal = 180,                /* universal  */
  YYSYMBOL_definition_def = 181,           /* definition_def  */
  YYSYMBOL_definition_id = 182,            /* definition_id  */
  YYSYMBOL_units_def = 183,                /* units_def  */
  YYSYMBOL_units_statement = 184,          /* units_statement  */
  YYSYMBOL_optional_ladder_end = 185,      /* optional_ladder_end  */
  YYSYMBOL_table_statement = 186,          /* table_statement  */
  YYSYMBOL_table_begin = 187,              /* table_begin  */
  YYSYMBOL_table_mode_on = 188,            /* table_mode_on  */
  YYSYMBOL_table_mode_off = 189,           /* table_mode_off  */
  YYSYMBOL_table_options = 190,            /* table_options  */
  YYSYMBOL_table_decl_opt = 191,           /* table_decl_opt  */
  YYSYMBOL_table_option = 192,             /* table_option  */
  YYSYMBOL_table_body = 193,               /* table_body  */
  YYSYMBOL_table_body_item = 194,          /* table_body_item  */
  YYSYMBOL_table_scalar = 195,             /* table_scalar  */
  YYSYMBOL_values_statement = 196,         /* values_statement  */
  YYSYMBOL_values_default_opt = 197,       /* values_default_opt  */
  YYSYMBOL_values_entries = 198,           /* values_entries  */
  YYSYMBOL_values_entry = 199,             /* values_entry  */
  YYSYMBOL_values_key_list = 200,          /* values_key_list  */
  YYSYMBOL_values_key = 201,               /* values_key  */
  YYSYMBOL_dataset_statement = 202,        /* dataset_statement  */
  YYSYMBOL_203_1 = 203,                    /* $@1  */
  YYSYMBOL_dataset_items = 204,            /* dataset_items  */
  YYSYMBOL_dataset_item = 205,             /* dataset_item  */
  YYSYMBOL_dataset_index_item = 206,       /* dataset_index_item  */
  YYSYMBOL_dataset_map_item = 207,         /* dataset_map_item  */
  YYSYMBOL_dataset_column_selector = 208,  /* dataset_column_selector  */
  YYSYMBOL_dataset_column_ref = 209,       /* dataset_column_ref  */
  YYSYMBOL_dataset_target = 210,           /* dataset_target  */
  YYSYMBOL_dataset_units_opt = 211,        /* dataset_units_opt  */
  YYSYMBOL_dataset_type_opt = 212,         /* dataset_type_opt  */
  YYSYMBOL_dataset_type_req = 213,         /* dataset_type_req  */
  YYSYMBOL_unitdeflist = 214,              /* unitdeflist  */
  YYSYMBOL_unitdef = 215,                  /* unitdef  */
  YYSYMBOL_unitladderitemlist = 216,       /* unitladderitemlist  */
  YYSYMBOL_unitladderitem = 217,           /* unitladderitem  */
  YYSYMBOL_methods = 218,                  /* methods  */
  YYSYMBOL_219_2 = 219,                    /* $@2  */
  YYSYMBOL_proclist = 220,                 /* proclist  */
  YYSYMBOL_proclistf = 221,                /* proclistf  */
  YYSYMBOL_procedure = 222,                /* procedure  */
  YYSYMBOL_procedure_id = 223,             /* procedure_id  */
  YYSYMBOL_fstatements = 224,              /* fstatements  */
  YYSYMBOL_statements = 225,               /* statements  */
  YYSYMBOL_statement = 226,                /* statement  */
  YYSYMBOL_complex_statement = 227,        /* complex_statement  */
  YYSYMBOL_isa_statement = 228,            /* isa_statement  */
  YYSYMBOL_willbe_statement = 229,         /* willbe_statement  */
  YYSYMBOL_aliases_statement = 230,        /* aliases_statement  */
  YYSYMBOL_optional_set_values = 231,      /* optional_set_values  */
  YYSYMBOL_is_statement = 232,             /* is_statement  */
  YYSYMBOL_isrefinedto_statement = 233,    /* isrefinedto_statement  */
  YYSYMBOL_call_identifier = 234,          /* call_identifier  */
  YYSYMBOL_type_identifier = 235,          /* type_identifier  */
  YYSYMBOL_optional_method = 236,          /* optional_method  */
  YYSYMBOL_optional_of = 237,              /* optional_of  */
  YYSYMBOL_optional_with_value = 238,      /* optional_with_value  */
  YYSYMBOL_arealike_statement = 239,       /* arealike_statement  */
  YYSYMBOL_link_statement = 240,           /* link_statement  */
  YYSYMBOL_unlink_statement = 241,         /* unlink_statement  */
  YYSYMBOL_der_statement = 242,            /* der_statement  */
  YYSYMBOL_independent_statement = 243,    /* independent_statement  */
  YYSYMBOL_arethesame_statement = 244,     /* arethesame_statement  */
  YYSYMBOL_willbethesame_statement = 245,  /* willbethesame_statement  */
  YYSYMBOL_willnotbethesame_statement = 246, /* willnotbethesame_statement  */
  YYSYMBOL_assignment_statement = 247,     /* assignment_statement  */
  YYSYMBOL_relation_statement = 248,       /* relation_statement  */
  YYSYMBOL_relation = 249,                 /* relation  */
  YYSYMBOL_blackbox_statement = 250,       /* blackbox_statement  */
  YYSYMBOL_input_args = 251,               /* input_args  */
  YYSYMBOL_output_args = 252,              /* output_args  */
  YYSYMBOL_data_args = 253,                /* data_args  */
  YYSYMBOL_for_statement = 254,            /* for_statement  */
  YYSYMBOL_optional_direction = 255,       /* optional_direction  */
  YYSYMBOL_forexprend = 256,               /* forexprend  */
  YYSYMBOL_run_statement = 257,            /* run_statement  */
  YYSYMBOL_fix_statement = 258,            /* fix_statement  */
  YYSYMBOL_fix_and_assign_statement = 259, /* fix_and_assign_statement  */
  YYSYMBOL_free_statement = 260,           /* free_statement  */
  YYSYMBOL_solver_statement = 261,         /* solver_statement  */
  YYSYMBOL_option_statement = 262,         /* option_statement  */
  YYSYMBOL_solve_statement = 263,          /* solve_statement  */
  YYSYMBOL_study_statement = 264,          /* study_statement  */
  YYSYMBOL_study_vary_opt = 265,           /* study_vary_opt  */
  YYSYMBOL_study_distribution_opt = 266,   /* study_distribution_opt  */
  YYSYMBOL_study_run_opt = 267,            /* study_run_opt  */
  YYSYMBOL_study_file_opt = 268,           /* study_file_opt  */
  YYSYMBOL_delete_statement = 269,         /* delete_statement  */
  YYSYMBOL_external_statement = 270,       /* external_statement  */
  YYSYMBOL_call_statement = 271,           /* call_statement  */
  YYSYMBOL_assert_statement = 272,         /* assert_statement  */
  YYSYMBOL_if_statement = 273,             /* if_statement  */
  YYSYMBOL_while_statement = 274,          /* while_statement  */
  YYSYMBOL_optional_else = 275,            /* optional_else  */
  YYSYMBOL_when_statement = 276,           /* when_statement  */
  YYSYMBOL_whenlist = 277,                 /* whenlist  */
  YYSYMBOL_whenlistf = 278,                /* whenlistf  */
  YYSYMBOL_flow_statement = 279,           /* flow_statement  */
  YYSYMBOL_use_statement = 280,            /* use_statement  */
  YYSYMBOL_select_statement = 281,         /* select_statement  */
  YYSYMBOL_selectlist = 282,               /* selectlist  */
  YYSYMBOL_selectlistf = 283,              /* selectlistf  */
  YYSYMBOL_switch_statement = 284,         /* switch_statement  */
  YYSYMBOL_switchlist = 285,               /* switchlist  */
  YYSYMBOL_switchlistf = 286,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 287,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 288,          /* notes_statement  */
  YYSYMBOL_notes_body = 289,               /* notes_body  */
  YYSYMBOL_noteslist = 290,                /* noteslist  */
  YYSYMBOL_fvarlist = 291,                 /* fvarlist  */
  YYSYMBOL_varlist = 292,                  /* varlist  */
  YYSYMBOL_fname = 293,                    /* fname  */
  YYSYMBOL_name = 294,                     /* name  */
  YYSYMBOL_end = 295,                      /* end  */
  YYSYMBOL_optional_bracedtext = 296,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 297,           /* optional_notes  */
  YYSYMBOL_set = 298,                      /* set  */
  YYSYMBOL_setexprlist = 299,              /* setexprlist  */
  YYSYMBOL_number = 300,                   /* number  */
  YYSYMBOL_realnumber = 301,               /* realnumber  */
  YYSYMBOL_opunits = 302,                  /* opunits  */
  YYSYMBOL_dims = 303,                     /* dims  */
  YYSYMBOL_dimensions = 304,               /* dimensions  */
  YYSYMBOL_dimexpr = 305,                  /* dimexpr  */
  YYSYMBOL_fraction = 306,                 /* fraction  */
  YYSYMBOL_fractail = 307,                 /* fractail  */
  YYSYMBOL_optional_sign = 308,            /* optional_sign  */
  YYSYMBOL_expr = 309,                     /* expr  */
  YYSYMBOL_relop = 310,                    /* relop  */
  YYSYMBOL_logrelop = 311                  /* logrelop  */
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
#define YYLAST   1407

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  153
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  159
/* YYNRULES -- Number of rules.  */
#define YYNRULES  410
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  804

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   390


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
     147,   148,   142,   139,   134,   140,   152,   141,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   149,   146,
     136,   137,   138,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   150,     2,   151,   145,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   135,     2,     2,     2,     2,     2,
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
     125,   126,   127,   128,   129,   130,   131,   132,   133,   143,
     144
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   869,   869,   871,   875,   876,   877,   878,   879,   880,
     881,   882,   883,   884,   886,   887,   888,   899,   944,   948,
     953,   961,   965,   970,   978,   987,   999,  1030,  1038,  1061,
    1074,  1082,  1103,  1115,  1123,  1176,  1187,  1196,  1202,  1208,
    1216,  1224,  1235,  1275,  1309,  1314,  1320,  1324,  1346,  1352,
    1358,  1366,  1374,  1385,  1426,  1435,  1448,  1457,  1460,  1468,
    1471,  1479,  1482,  1533,  1536,  1543,  1573,  1582,  1587,  1604,
    1630,  1633,  1640,  1661,  1674,  1681,  1688,  1693,  1695,  1700,
    1712,  1725,  1729,  1736,  1745,  1747,  1751,  1752,  1756,  1760,
    1764,  1768,  1772,  1776,  1783,  1787,  1791,  1795,  1799,  1806,
    1813,  1815,  1822,  1823,  1827,  1834,  1835,  1839,  1840,  1841,
    1842,  1847,  1846,  1856,  1858,  1859,  1869,  1870,  1874,  1881,
    1885,  1892,  1896,  1903,  1907,  1914,  1919,  1928,  1931,  1939,
    1942,  1949,  1956,  1959,  1967,  1975,  1978,  1986,  1991,  2001,
    2005,  2004,  2018,  2026,  2029,  2054,  2068,  2077,  2085,  2088,
    2154,  2162,  2170,  2171,  2172,  2173,  2174,  2175,  2176,  2177,
    2178,  2179,  2180,  2181,  2182,  2183,  2184,  2186,  2187,  2188,
    2189,  2190,  2191,  2192,  2193,  2194,  2195,  2196,  2197,  2198,
    2199,  2200,  2201,  2202,  2203,  2204,  2205,  2206,  2207,  2208,
    2209,  2210,  2211,  2215,  2220,  2261,  2290,  2294,  2345,  2348,
    2355,  2367,  2394,  2399,  2407,  2412,  2421,  2424,  2432,  2435,
    2443,  2446,  2453,  2460,  2464,  2468,  2475,  2479,  2486,  2495,
    2504,  2511,  2518,  2525,  2529,  2536,  2549,  2566,  2576,  2584,
    2595,  2616,  2623,  2631,  2634,  2671,  2693,  2696,  2700,  2707,
    2711,  2715,  2719,  2726,  2730,  2737,  2745,  2759,  2766,  2774,
    2782,  2787,  2794,  2803,  2806,  2816,  2826,  2840,  2843,  2848,
    2857,  2860,  2869,  2872,  2880,  2887,  2898,  2909,  2915,  2925,
    2935,  2938,  2945,  2956,  2968,  2975,  2985,  2992,  2996,  3000,
    3004,  3011,  3015,  3019,  3023,  3027,  3034,  3041,  3052,  3062,
    3069,  3073,  3077,  3081,  3088,  3099,  3109,  3116,  3120,  3124,
    3128,  3135,  3145,  3182,  3191,  3206,  3211,  3220,  3231,  3235,
    3240,  3253,  3273,  3277,  3282,  3297,  3302,  3307,  3312,  3317,
    3322,  3327,  3332,  3337,  3342,  3347,  3352,  3357,  3362,  3371,
    3374,  3382,  3385,  3392,  3397,  3403,  3407,  3411,  3416,  3424,
    3431,  3440,  3445,  3470,  3475,  3498,  3502,  3507,  3513,  3517,
    3524,  3528,  3532,  3536,  3540,  3544,  3551,  3558,  3562,  3570,
    3573,  3577,  3584,  3588,  3592,  3596,  3600,  3604,  3608,  3612,
    3616,  3620,  3624,  3629,  3634,  3639,  3644,  3649,  3654,  3659,
    3663,  3668,  3673,  3678,  3683,  3688,  3692,  3696,  3700,  3704,
    3711,  3715,  3722,  3726,  3733,  3737,  3744,  3748,  3755,  3759,
    3766,  3770,  3781,  3788,  3792,  3796,  3800,  3804,  3808,  3815,
    3819
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
  "INPUT_TOK", "INCREASING_TOK", "INTERACTIVE_TOK", "INDEPENDENT_TOK",
  "INTERSECTION_TOK", "ISA_TOK", "_IS_T", "ISREFINEDTO_TOK", "LINEAR_TOK",
  "LOG_TOK", "LINK_TOK", "MAXIMIZE_TOK", "MAXINTEGER_TOK", "MAXREAL_TOK",
  "METHODS_TOK", "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK", "NOT_TOK",
  "NOTES_TOK", "OF_TOK", "OPTION_TOK", "OR_TOK", "OTHERWISE_TOK",
  "OUTPUT_TOK", "PROD_TOK", "PROVIDE_TOK", "RATIO_TOK", "REFINES_TOK",
  "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK", "SATISFIED_TOK",
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
  "study_statement", "study_vary_opt", "study_distribution_opt",
  "study_run_opt", "study_file_opt", "delete_statement",
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

#define YYPACT_NINF (-734)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-309)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -734,  1299,  -734,  -734,   -32,   -93,  -101,    85,   113,    27,
     138,    30,  -734,  -734,  -734,  -734,  -734,  -734,  -734,    11,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,    42,  -734,  -734,
    -734,     3,   108,   137,  -734,  -734,   155,    59,  -734,    88,
     104,   169,    98,   123,  -734,   -11,   140,   -16,   243,   227,
     243,   166,   193,   211,  -734,   240,  -734,  -734,   189,   286,
     747,  -734,   -17,   225,   243,   224,  -734,  -734,  -734,   589,
     233,   -12,  -734,  -734,   -10,   420,   228,  -734,  -734,   140,
     235,    95,  -734,  -103,   140,   221,   226,   234,  -734,   229,
     236,  -734,   294,  -734,   286,   249,   286,  -734,   261,  -734,
     243,   238,  -734,   589,  -734,   250,   156,   157,  -734,  -734,
     290,   239,   258,  -734,  -734,   140,   259,   140,   589,   140,
     171,   245,   589,  -734,  -734,   589,   589,    11,   264,   172,
    -734,   140,   252,    31,   140,   267,   260,   185,    32,   140,
     140,   140,   271,  -734,   187,   255,    40,   589,   140,   272,
     273,   262,  -734,   589,   589,   589,   589,  -734,  -734,  -734,
    -734,   265,   266,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,   146,   244,  -734,  1193,   344,   269,   349,   270,
     274,  -734,   277,   283,   878,  -734,   352,   287,   282,   288,
     322,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,   300,   301,  -734,   140,  -734,  -734,
    -734,   140,  -734,  -734,  -734,  -734,  -734,   305,   243,    26,
     243,   291,   289,   -45,  -734,   308,  -734,  1193,   304,  -734,
     589,   589,   589,   589,   243,  -734,   140,   310,  -734,  -734,
     -53,   407,  -734,  1017,  -734,   589,   589,   -34,  1193,  1193,
    -734,   -16,   589,   589,   589,   342,   140,   140,     0,  -734,
    -734,  -734,  -734,   589,   589,   140,    37,   353,  -734,   441,
     424,   589,   589,    66,   140,    51,  1054,  -734,  -734,  -734,
    -734,   589,   325,   325,   777,   321,  -734,  -734,    46,  -734,
    -734,   343,   345,   343,   343,  -734,  -734,   589,   589,   440,
     589,  -734,  -734,   589,   589,   589,  -734,  -734,  -734,   589,
    -734,  -734,  -734,   589,   589,   589,   589,   589,   589,   589,
     328,  -734,   346,   332,  -734,  -734,  -734,   589,   589,   335,
    -734,   354,  -734,  -734,  -734,   341,  -734,  -734,   243,     9,
     348,    39,  -734,   463,   358,  -734,  -734,   360,  -734,  -734,
    -734,   589,   350,   340,   351,   355,  -734,   361,   140,   589,
    -734,   364,   357,   379,   381,   382,  -734,  1193,   369,   367,
     140,    56,   371,   589,   372,   243,    57,   375,   376,   380,
     589,   383,   243,    58,   140,   411,   466,   589,   384,   401,
     388,   386,   404,   405,   392,   589,   393,   243,   116,  -734,
     833,  -734,  -734,   140,  -734,   394,   474,   474,  -734,   474,
    1193,  1193,    64,   399,  -734,   143,   143,   143,  1116,  1116,
      94,    94,   325,   325,   325,    82,   285,  -734,  -734,  -734,
    1079,  1193,  -734,  -734,  -734,   403,    39,  -734,   416,   434,
    -734,  -734,  -734,  -734,   -50,  -734,   160,   -18,   409,  -734,
     408,   406,   410,  -734,  -734,  -734,  -734,  -734,   412,   940,
     524,  -734,  -734,   429,   140,   140,  -734,  -734,  -734,   158,
    -734,     0,   413,  -734,  -734,   589,   414,  -734,  -734,    37,
     415,  -734,  -734,   589,   417,   523,   443,   528,   343,  -734,
    1193,   165,   428,  -734,  -734,   140,   140,    51,   426,  -734,
    -734,   589,   427,   243,  -734,   430,   589,   447,   455,  -734,
     455,   140,    51,   589,   589,  -734,  -734,  -734,     6,   450,
      71,   -50,   -50,   173,  -734,  -734,  -734,  -734,  -734,   216,
    -734,  -734,  -734,   438,  -734,  -734,  -734,  -734,   132,  -734,
     243,   451,   444,   448,   273,   452,   243,  -734,  -734,   439,
    -734,   243,  -734,  -734,   442,  -734,   589,  -734,   456,  -734,
     474,   -15,  -734,  -734,  -734,  -734,    78,   453,   206,  -734,
    -734,   454,   458,   243,  -734,  -734,   449,  -734,  -734,   489,
     460,  -734,   589,  -734,  -734,   462,   243,   457,   464,   -79,
    1193,  -734,  -734,  -734,   216,   465,  -734,   459,   459,  -734,
     -81,   273,  -734,  -734,   467,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,   140,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  1156,  -734,  -734,   589,  -734,
     481,  -734,  -734,   518,   470,  -734,   165,   589,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,   140,  -734,  1193,    51,  -734,
     140,   570,  -734,  -734,  -734,   491,  -734,  -734,   243,   473,
    -734,  -734,   589,  1193,  -734,    43,  -734,  -734,  -734,  1193,
      12,  -734,   568,   243,   478,   476,  -734,   486,  -734,  -734,
     915,   594,   313,   484,   534,   502,   483,   490,  -734,  -734,
     -33,   507,  -734,   140,   492,   565,   512,   589,   589,   514,
     549,   550,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,   604,   140,
    -734,   -71,   517,   140,   607,   583,   504,  -734,  -734,   506,
    1193,  1193,   295,  -734,  -734,   555,   508,   218,  -734,  -734,
     526,  -734,  -734,   509,   -71,   531,   634,  -734,  -734,  -734,
    -734,  -734,  -734,   218,  -734,  -734,  -734,   608,  -734,   526,
     538,  -734,   613,   539,  -734,  -734,   525,  -734,   540,  -734,
     589,  -734,   532,  -734
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    16,     0,     0,     0,     0,     0,     0,
       0,   132,    64,     3,    15,     4,     5,     6,     9,     0,
       7,   143,     8,   143,    11,    10,    12,     0,    13,   148,
      14,     0,     0,     0,    66,   148,     0,     0,   312,     0,
      23,     0,     0,    20,   135,     0,     0,     0,     0,   142,
       0,     0,     0,     0,   148,     0,    42,   148,    57,   139,
       0,    67,     0,     0,     0,     0,    25,    21,    22,   334,
       0,     0,    18,    19,     0,   328,     0,   133,    68,   303,
       0,   307,   308,   331,     0,     0,     0,     0,   144,     0,
       0,    36,     0,    56,   139,     0,   139,   148,    59,   140,
       0,     0,   368,     0,   281,     0,     0,     0,   148,   282,
       0,     0,     0,   367,   283,     0,     0,     0,     0,     0,
       0,     0,     0,   363,   365,     0,     0,     0,     0,     0,
     284,     0,     0,     0,   250,     0,   329,     0,     0,     0,
       0,     0,     0,   366,     0,     0,     0,     0,     0,   343,
     362,   312,   369,     0,     0,     0,   334,   189,   190,   191,
     192,     0,     0,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   225,   167,
     170,   171,   172,   193,   173,   174,   176,   175,   177,   178,
     169,   168,   179,   180,   181,   182,   184,   183,   185,   186,
     187,   188,     0,   370,   364,   227,     0,     0,   206,     0,
       0,   370,     0,   333,   335,   313,     0,     0,   138,     0,
      70,   315,   316,   324,   317,   318,   319,   320,   321,   322,
     323,   325,   326,   327,     0,     0,   305,     0,   310,   332,
     311,   304,    26,    28,   146,   148,    31,     0,     0,   347,
       0,     0,     0,     0,   143,     0,   151,   267,   202,   266,
     334,   334,   334,   334,     0,   264,     0,     0,   246,   245,
     308,     0,   247,     0,   219,   334,   334,     0,   229,   228,
     379,     0,     0,   334,   334,   243,     0,     0,     0,   251,
     248,   330,   285,   334,   334,     0,     0,   253,    74,   100,
       0,   334,   334,     0,     0,     0,     0,   286,   344,   341,
     342,     0,   385,   386,     0,     0,   149,   150,     0,   212,
     220,     0,     0,     0,     0,   221,   222,     0,     0,     0,
       0,   409,   410,     0,     0,     0,   406,   407,   408,     0,
     404,   403,   405,     0,     0,     0,     0,     0,     0,     0,
       0,    29,     0,     0,    17,    24,   314,     0,     0,     0,
      32,     0,   136,    71,    69,     0,   306,   309,     0,    46,
       0,     0,   346,    37,     0,    58,   148,     0,    54,   141,
      65,   334,     0,     0,     0,     0,   301,     0,     0,     0,
     148,     0,     0,     0,     0,     0,   302,   249,     0,     0,
       0,     0,     0,   334,     0,     0,   289,     0,     0,     0,
     334,     0,     0,   296,     0,   260,    79,     0,     0,     0,
       0,     0,     0,     0,     0,   334,     0,     0,   276,   148,
       0,   402,   371,     0,   196,   204,   208,   208,   201,   208,
     223,   224,     0,   312,   226,   377,   382,   378,   384,   383,
     372,   373,   375,   374,   376,   380,   381,    30,   207,    27,
     337,   336,    33,   137,   134,     0,     0,    45,     0,    48,
      34,   351,   350,   348,     0,   345,   349,   359,     0,    53,
       0,    61,     0,   397,   398,   399,   400,   218,     0,   236,
     270,   395,   396,     0,     0,     0,   391,   392,   244,     0,
     388,     0,     0,   148,   287,   334,     0,   389,   390,     0,
       0,   148,   294,   334,     0,     0,     0,   262,     0,    77,
     101,     0,     0,   393,   394,     0,     0,     0,     0,   148,
     272,   334,     0,     0,   401,     0,   334,     0,   210,   200,
     210,     0,     0,     0,     0,   145,    44,    47,   359,   331,
       0,     0,     0,   359,    39,    40,    41,   360,   361,     0,
      35,    60,   148,     0,   203,   265,   238,   237,     0,   148,
       0,     0,     0,     0,     0,     0,     0,   148,   291,     0,
     148,     0,   148,   298,     0,   148,     0,   261,     0,   252,
     208,     0,   110,   109,   107,   108,     0,     0,     0,   105,
     111,     0,     0,     0,   148,   278,     0,   148,   269,     0,
       0,   209,     0,   194,   195,     0,     0,     0,     0,   370,
     338,    51,    50,    52,     0,     0,   355,   352,   353,   354,
       0,   339,    38,   340,     0,    55,   241,   239,   242,   240,
     148,   271,   268,     0,   214,   215,   387,   288,   290,   148,
     293,   295,   297,   148,   300,     0,   263,    80,     0,    81,
       0,    75,    78,     0,     0,   102,     0,     0,   113,   216,
     217,   274,   277,   148,   280,     0,   205,   211,     0,   273,
       0,     0,    49,    43,   357,     0,   356,    62,     0,     0,
     292,   299,     0,    82,    83,     0,    99,   103,   106,   104,
       0,   279,     0,     0,   233,     0,   231,     0,   235,   213,
       0,     0,     0,     0,     0,     0,     0,     0,   116,   117,
       0,     0,   275,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    93,    97,    96,    94,    98,    95,    89,    88,
      90,    91,    92,    87,    85,    86,   115,   112,     0,     0,
     114,     0,     0,     0,     0,     0,     0,   230,   232,     0,
     256,   255,   257,    76,    76,     0,     0,     0,   123,   124,
     127,   122,   131,     0,     0,     0,     0,   358,   258,   259,
     254,    73,    72,     0,   125,   121,   128,   129,   126,   127,
     198,   234,     0,     0,   119,   120,     0,   197,     0,   130,
     334,   118,     0,   199
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,   614,  -734,  -734,  -734,  -734,   -88,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,    83,  -734,    16,  -734,
    -734,  -734,  -734,  -734,  -734,   -90,  -733,  -734,  -104,  -734,
    -734,  -734,  -734,  -734,  -734,   159,  -734,   -21,  -734,  -734,
    -734,   106,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -319,  -734,  -431,   147,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,   574,  -734,   366,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,  -734,
    -734,  -734,  -734,  -734,  -526,  -734,  -734,  -734,  -734,   196,
    -734,  -734,   181,  -734,  -734,  -734,   571,   615,   -57,  -734,
     -46,   268,   -22,  -734,   151,  -136,  -734,    77,  -492,  -734,
    -734,   246,  -447,  -734,  -734,  -316,   -19,  -734,  -734
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    54,    55,   478,    25,    56,   469,
     549,    26,    57,    58,    98,   253,   563,    27,    28,    29,
      30,    31,   364,   158,   416,   695,   781,   591,   519,   662,
     712,   744,   745,   159,   418,   596,   597,   598,   599,   160,
     668,   700,   717,   718,   719,   770,   771,   720,   787,   794,
     754,    45,    77,    74,   219,   100,   254,    48,    49,    88,
      89,    59,    60,   161,   162,   163,   164,   165,   797,   166,
     167,   259,   436,   353,   538,   613,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   617,   704,
     724,   180,   568,   640,   181,   182,   183,   184,   185,   186,
     187,   188,   415,   780,   517,   589,   189,   190,   191,   192,
     193,   194,   570,   195,   427,   428,   196,   197,   198,   405,
     406,   199,   412,   413,   200,   201,    47,    79,    80,    81,
     211,    83,    78,   292,   240,   212,   213,   632,   204,   309,
     373,   475,   476,   629,   686,   559,   214,   348,   349
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      82,   603,    50,   202,   438,   439,   539,   575,   540,   751,
     206,   658,   393,   713,   203,   216,   616,   403,    75,   554,
     315,   752,   235,    75,    75,    85,    86,   550,    90,   767,
     239,    32,   377,    82,   785,   238,    33,    34,    82,   466,
     467,   205,   209,   621,   711,    35,   714,    69,   684,    70,
     792,  -308,   220,    51,   410,  -308,   371,   372,   269,   768,
     272,   769,   274,    52,   327,   328,   685,   633,   425,   270,
    -308,    82,   404,    82,   505,   513,   288,   -84,   255,   471,
     472,   296,   297,   659,   257,   285,   555,    82,   289,   305,
      41,   660,    82,    82,   298,   299,    38,   474,   394,   273,
      82,   378,   307,   278,   627,   628,   279,   280,    53,   411,
     622,   715,   663,   207,   556,   468,    84,   753,   217,    76,
     218,   557,   558,   426,   382,   383,   384,   385,   306,   506,
     514,   661,   633,   531,   312,   313,   314,    44,   623,   391,
     392,    64,   716,    46,   -84,   557,   558,   398,   399,    61,
     318,   636,   703,   319,   320,   637,    62,   407,   408,   657,
      94,    38,    38,    96,   638,   420,   421,   639,   471,   472,
      38,   -84,   -84,   -84,   -84,   -84,    38,   -84,   287,   295,
     -84,   473,   -84,   -84,   235,    63,   474,   304,   532,   -84,
     499,   367,   -84,   433,    38,    82,    38,    65,   422,   590,
     321,   322,   323,   251,   500,    66,   592,   593,   594,   387,
     595,   541,   551,   552,   264,    36,   553,    71,    37,   626,
      82,   343,   344,   345,   346,    38,   370,   347,   374,   237,
     402,   395,   624,   379,    67,   345,   346,   630,   409,   347,
     401,    82,   386,    38,    72,   482,    39,   424,  -308,    82,
      68,  -308,  -308,   248,    69,   250,    70,   423,    82,   396,
     324,   325,   326,   397,   336,   337,   338,   502,    38,    73,
      38,    42,   434,    69,   510,    70,    40,    75,    43,   340,
     341,   342,   343,   344,   345,   346,   149,   574,   347,   528,
     330,    87,   430,   592,   593,   594,    91,   595,  -308,  -308,
    -308,   551,   552,   260,   262,   553,   261,   263,   440,   441,
     205,   445,   557,   558,   446,   447,   448,    95,   275,   283,
     449,   276,   284,    92,   450,   451,   452,   453,   454,   455,
     456,   488,   293,   333,   301,   294,    97,   302,   460,   461,
     666,    93,    82,   667,   149,   631,   465,   731,   768,    99,
     769,   368,   778,   779,   498,   208,   334,   210,  -308,  -308,
    -308,   327,   328,   215,   244,   234,   236,   242,   515,   579,
     489,   247,   243,   252,  -308,   245,   535,   584,  -308,   249,
     258,   265,   246,   504,   256,   542,   266,    82,   267,   271,
     512,   291,   277,   329,   282,   606,    82,   290,   520,   286,
     610,   300,   303,   308,   310,   530,   336,   337,   338,   311,
     350,   316,   317,   352,   732,   351,   354,   357,   359,   361,
     355,   340,   341,   342,   343,   344,   345,   346,   356,   363,
     347,   365,   366,   360,   362,   369,   376,   572,   573,   375,
     221,   733,   734,   735,   736,   737,   102,   738,    82,    82,
     739,   381,   740,   741,   380,   389,   106,   388,   107,   742,
     222,   400,   743,   414,   223,   224,   419,   417,   601,   602,
     347,   225,   432,   435,   457,   437,   458,   113,   459,    82,
      82,   462,   480,   226,   615,   463,   618,   464,   227,   477,
     481,   484,   516,   120,   470,    82,   490,   619,   483,   485,
     122,   123,   124,   228,   479,   125,   486,   126,   492,   487,
     229,   608,   491,   493,   129,   494,   495,   496,   497,   501,
     518,   503,   132,   507,   430,   620,   230,   508,   509,   137,
     521,   231,   511,   232,   522,   533,   523,   524,   525,   526,
     527,   536,   529,   537,   143,   144,   543,   547,   642,   545,
     233,   442,   548,   562,   647,   560,   561,   569,   564,   651,
     565,   571,   577,   580,   582,   586,   585,   655,   149,   150,
     443,   588,   152,   587,   600,   604,   607,   611,   609,   153,
     154,   671,   612,   239,   635,   643,   689,   155,   649,   656,
     156,   653,   644,   677,   679,   102,   645,    82,   673,   665,
     646,   675,   669,   680,   553,   106,   670,   107,   676,   578,
     678,   683,   694,   681,   696,   687,   697,   583,   702,   706,
     707,   709,   721,   705,   723,   725,   113,   726,   730,    82,
     746,   747,   748,   749,    82,   605,   750,   755,   758,   693,
     757,   759,   120,   762,   763,   764,   765,   772,   699,   774,
     123,   124,   775,   776,   777,   783,   126,   786,   791,   784,
     788,   790,   793,   129,   802,   796,   708,   798,   634,   799,
     801,   132,   800,   710,   157,   641,   782,   756,   137,   664,
     803,   722,   698,   648,   789,   795,   650,   614,   652,   268,
     581,   654,   766,   143,   144,   444,   773,   576,   281,   241,
     625,   682,     0,    82,     0,     0,     0,    82,   760,   761,
     672,     0,   546,   674,     0,     0,     0,   149,   150,   151,
       0,   152,     0,     0,     0,     0,     0,     0,   153,   154,
       0,     0,     0,     0,     0,     0,   155,     0,     0,   156,
       0,     0,     0,     0,     0,     0,   688,     0,   101,     0,
       0,     0,     0,   102,     0,   690,     0,   103,     0,   691,
       0,   104,   105,   106,  -147,   107,     0,   108,     0,   109,
       0,     0,     0,     0,     0,   110,   111,     0,     0,   701,
    -147,  -147,   330,   112,   113,   114,   115,   116,   117,   331,
     332,     0,   118,     0,     0,     0,     0,     0,     0,   119,
     120,     0,     0,     0,     0,     0,   121,   122,   123,   124,
    -147,     0,   125,     0,   126,   127,     0,   128,     0,  -147,
       0,   129,     0,     0,     0,   333,     0,   130,   131,   132,
     133,     0,   134,   135,   136,     0,   137,   138,   330,     0,
       0,   139,   140,   141,   142,   331,   332,     0,   334,     0,
       0,   143,   144,    11,     0,     0,   145,     0,   146,     0,
     147,     0,     0,     0,     0,   335,     0,   148,     0,     0,
       0,     0,     0,     0,     0,   149,   150,   151,     0,   152,
       0,   333,     0,   330,     0,     0,   153,   154,     0,     0,
     331,   332,     0,     0,   155,  -147,     0,   156,   336,   337,
     338,     0,     0,     0,   334,     0,     0,     0,     0,     0,
       0,     0,   339,   340,   341,   342,   343,   344,   345,   346,
     330,   335,   347,     0,     0,   431,   333,   331,   332,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   330,     0,     0,     0,   334,
       0,     0,   331,   332,   336,   337,   338,     0,     0,     0,
       0,     0,     0,   333,     0,   566,   335,     0,   339,   340,
     341,   342,   343,   344,   345,   346,     0,     0,   347,     0,
       0,   534,     0,     0,     0,     0,   334,     0,   333,     0,
     567,   727,     0,     0,     0,     0,     0,     0,     0,   336,
     337,   338,   358,   335,     0,     0,     0,   728,   729,     0,
       0,   334,     0,   339,   340,   341,   342,   343,   344,   345,
     346,     0,   330,   347,     0,     0,     0,     0,   335,   331,
     332,     0,     0,     0,     0,     0,   336,   337,   338,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     339,   340,   341,   342,   343,   344,   345,   346,     0,   330,
     347,   336,   337,   338,     0,   333,   331,   332,     0,     0,
       0,     0,     0,     0,     0,   339,   340,   341,   342,   343,
     344,   345,   346,     0,   330,   347,   429,     0,   334,     0,
       0,   331,   332,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   333,     0,     0,   335,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   390,
       0,   330,     0,     0,     0,   334,     0,   333,   331,   332,
       0,     0,     0,     0,     0,     0,     0,     0,   336,   337,
     338,     0,   335,     0,     0,     0,     0,     0,     0,     0,
     334,     0,   339,   340,   341,   342,   343,   344,   345,   346,
       0,   330,   347,     0,   333,     0,     0,   335,   331,   332,
       0,     0,     0,     0,     0,   336,   337,   338,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   334,     0,   339,
     340,   341,   342,   343,   344,   345,   346,     0,   330,   347,
     336,   337,   338,   544,   333,   331,   332,     0,     0,     0,
       0,     0,     0,     0,   339,   340,   341,   342,   343,   344,
     345,   346,     0,     0,   347,     0,     0,   334,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   336,   337,   338,
       0,   333,     0,     0,   335,     0,     0,     0,     0,     0,
       0,     0,   340,   341,   342,   343,   344,   345,   346,   692,
       0,   347,     0,     0,   334,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   336,   337,   338,
       0,   335,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   339,   340,   341,   342,   343,   344,   345,   346,     2,
       3,   347,     4,     0,     0,     0,     0,     0,     0,     0,
     -63,     0,     0,     0,   336,   337,   338,     0,     0,     0,
     -63,     0,     0,     0,     0,     0,     5,     0,   339,   340,
     341,   342,   343,   344,   345,   346,     0,     0,   347,     0,
       0,     0,     0,     6,     0,     0,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   -63,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     0,     0,     9,    10,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    11,     0,    12
};

static const yytype_int16 yycheck[] =
{
      46,   527,    23,    60,   323,   324,   437,   499,   439,    42,
      27,    26,    46,     1,    60,    27,   542,    17,    34,    37,
     156,    54,    79,    34,    34,    47,    48,   474,    50,   100,
     133,    63,    77,    79,   767,    81,    68,   130,    84,    30,
      31,    60,    64,    37,     1,   146,    34,   150,   129,   152,
     783,   130,    74,    11,    17,   134,    30,    31,   115,   130,
     117,   132,   119,    21,   117,   118,   147,   559,    17,   115,
     149,   117,    72,   119,    17,    17,   133,    34,   100,   129,
     130,   138,   139,    98,   103,   131,   104,   133,   134,   146,
      63,   106,   138,   139,   140,   141,   130,   147,   132,   118,
     146,   146,   148,   122,   551,   552,   125,   126,    66,    72,
     104,    99,    34,   130,   132,   106,   132,   150,   130,   130,
     130,   139,   140,    72,   260,   261,   262,   263,   147,    72,
      72,   146,   624,    17,   153,   154,   155,   107,   132,   275,
     276,    35,   130,   132,   101,   139,   140,   283,   284,   146,
       4,    19,   678,     7,     8,    23,    48,   293,   294,   590,
      54,   130,   130,    57,    32,   301,   302,    35,   129,   130,
     130,   128,   129,   130,   131,   132,   130,   134,   147,   147,
     137,   142,   139,   140,   241,    48,   147,   147,    72,   146,
     134,   237,   149,   147,   130,   241,   130,    42,   132,   518,
      54,    55,    56,    97,   148,   146,   128,   129,   130,   266,
     132,   147,   141,   142,   108,   130,   145,    48,   133,   148,
     266,   139,   140,   141,   142,   130,   248,   145,   250,   134,
     287,   277,   548,   254,   146,   141,   142,   553,   295,   145,
     286,   287,   264,   130,   146,   381,   133,   304,     4,   295,
     146,     7,     8,    94,   150,    96,   152,   303,   304,   281,
     114,   115,   116,   282,   121,   122,   123,   403,   130,   146,
     130,   133,   318,   150,   410,   152,     8,    34,    10,   136,
     137,   138,   139,   140,   141,   142,   128,   129,   145,   425,
       5,    64,   311,   128,   129,   130,   130,   132,    54,    55,
      56,   141,   142,   147,   147,   145,   150,   150,   327,   328,
     329,   330,   139,   140,   333,   334,   335,    77,   147,   147,
     339,   150,   150,   130,   343,   344,   345,   346,   347,   348,
     349,   388,   147,    48,   147,   150,   147,   150,   357,   358,
     134,   130,   388,   137,   128,   129,   368,    34,   130,    63,
     132,   245,    57,    58,   400,   130,    71,   133,   114,   115,
     116,   117,   118,   130,   130,   137,   131,   146,   414,   505,
     389,    77,   146,   112,   130,   146,   433,   513,   134,   130,
     130,    91,   146,   405,   146,   442,   147,   433,   130,   130,
     412,   131,   147,   149,   130,   531,   442,   130,   417,   147,
     536,   130,   147,   131,   131,   427,   121,   122,   123,   147,
      66,   146,   146,    64,   101,   146,   146,   134,    66,   137,
     146,   136,   137,   138,   139,   140,   141,   142,   151,   107,
     145,   131,   131,   146,   146,   130,   147,   494,   495,   148,
      20,   128,   129,   130,   131,   132,     6,   134,   494,   495,
     137,   147,   139,   140,   146,    48,    16,   147,    18,   146,
      40,   119,   149,   110,    44,    45,    42,    26,   525,   526,
     145,    51,   151,   130,   146,   130,   130,    37,   146,   525,
     526,   146,   376,    63,   541,   131,   543,   146,    68,    26,
     130,   151,    81,    53,   146,   541,   390,   543,   148,   148,
      60,    61,    62,    83,   146,    65,   151,    67,   151,   148,
      90,   533,   148,   134,    74,   134,   134,   148,   151,   148,
      54,   149,    82,   148,   543,   544,   106,   151,   148,    89,
     146,   111,   149,   113,   133,   429,   148,   151,   134,   134,
     148,   147,   149,    69,   104,   105,   147,   131,   570,   146,
     130,   111,   118,   147,   576,   146,   148,    33,   148,   581,
     148,   132,   149,   149,   149,    42,   149,   586,   128,   129,
     130,    43,   132,   130,   146,   149,   149,   130,   148,   139,
     140,   603,   127,   133,   146,   134,   643,   147,   149,   133,
     150,   149,   148,   612,   616,     6,   148,   643,   149,   146,
     148,   112,   148,   146,   145,    16,   148,    18,   148,   503,
     148,   146,   131,   149,    96,   148,   146,   511,   675,    49,
     129,   148,    54,   680,   146,   149,    37,   141,    34,   675,
     146,    97,   130,   150,   680,   529,   146,   130,    73,   658,
     148,   129,    53,   129,    95,    95,    42,   130,   667,    42,
      61,    62,    69,   149,   148,   100,    67,   131,    24,   151,
     151,   130,    54,    74,   800,   127,   688,    54,   562,   130,
     130,    82,   147,   692,    60,   569,   764,   723,    89,   596,
     148,   703,   666,   577,   774,   789,   580,   540,   582,   115,
     509,   585,   749,   104,   105,   329,   753,   501,   127,    84,
     549,   624,    -1,   749,    -1,    -1,    -1,   753,   727,   728,
     604,    -1,   466,   607,    -1,    -1,    -1,   128,   129,   130,
      -1,   132,    -1,    -1,    -1,    -1,    -1,    -1,   139,   140,
      -1,    -1,    -1,    -1,    -1,    -1,   147,    -1,    -1,   150,
      -1,    -1,    -1,    -1,    -1,    -1,   640,    -1,     1,    -1,
      -1,    -1,    -1,     6,    -1,   649,    -1,    10,    -1,   653,
      -1,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    28,    29,    -1,    -1,   673,
      33,    34,     5,    36,    37,    38,    39,    40,    41,    12,
      13,    -1,    45,    -1,    -1,    -1,    -1,    -1,    -1,    52,
      53,    -1,    -1,    -1,    -1,    -1,    59,    60,    61,    62,
      63,    -1,    65,    -1,    67,    68,    -1,    70,    -1,    72,
      -1,    74,    -1,    -1,    -1,    48,    -1,    80,    81,    82,
      83,    -1,    85,    86,    87,    -1,    89,    90,     5,    -1,
      -1,    94,    95,    96,    97,    12,    13,    -1,    71,    -1,
      -1,   104,   105,   106,    -1,    -1,   109,    -1,   111,    -1,
     113,    -1,    -1,    -1,    -1,    88,    -1,   120,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   128,   129,   130,    -1,   132,
      -1,    48,    -1,     5,    -1,    -1,   139,   140,    -1,    -1,
      12,    13,    -1,    -1,   147,   148,    -1,   150,   121,   122,
     123,    -1,    -1,    -1,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   135,   136,   137,   138,   139,   140,   141,   142,
       5,    88,   145,    -1,    -1,   148,    48,    12,    13,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     5,    -1,    -1,    -1,    71,
      -1,    -1,    12,    13,   121,   122,   123,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    -1,    25,    88,    -1,   135,   136,
     137,   138,   139,   140,   141,   142,    -1,    -1,   145,    -1,
      -1,   148,    -1,    -1,    -1,    -1,    71,    -1,    48,    -1,
      50,    76,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
     122,   123,   124,    88,    -1,    -1,    -1,    92,    93,    -1,
      -1,    71,    -1,   135,   136,   137,   138,   139,   140,   141,
     142,    -1,     5,   145,    -1,    -1,    -1,    -1,    88,    12,
      13,    -1,    -1,    -1,    -1,    -1,   121,   122,   123,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     135,   136,   137,   138,   139,   140,   141,   142,    -1,     5,
     145,   121,   122,   123,    -1,    48,    12,    13,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   135,   136,   137,   138,   139,
     140,   141,   142,    -1,     5,   145,    32,    -1,    71,    -1,
      -1,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    48,    -1,    -1,    88,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   102,
      -1,     5,    -1,    -1,    -1,    71,    -1,    48,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
     123,    -1,    88,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      71,    -1,   135,   136,   137,   138,   139,   140,   141,   142,
      -1,     5,   145,    -1,    48,    -1,    -1,    88,    12,    13,
      -1,    -1,    -1,    -1,    -1,   121,   122,   123,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,   135,
     136,   137,   138,   139,   140,   141,   142,    -1,     5,   145,
     121,   122,   123,   124,    48,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   135,   136,   137,   138,   139,   140,
     141,   142,    -1,    -1,   145,    -1,    -1,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,   123,
      -1,    48,    -1,    -1,    88,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   136,   137,   138,   139,   140,   141,   142,   103,
      -1,   145,    -1,    -1,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,   123,
      -1,    88,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   135,   136,   137,   138,   139,   140,   141,   142,     0,
       1,   145,     3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      11,    -1,    -1,    -1,   121,   122,   123,    -1,    -1,    -1,
      21,    -1,    -1,    -1,    -1,    -1,    27,    -1,   135,   136,
     137,   138,   139,   140,   141,   142,    -1,    -1,   145,    -1,
      -1,    -1,    -1,    44,    -1,    -1,    47,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    75,    -1,    -1,    78,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   106,    -1,   108
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   154,     0,     1,     3,    27,    44,    47,    75,    78,
      79,   106,   108,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   170,   174,   180,   181,   182,
     183,   184,    63,    68,   130,   146,   130,   133,   130,   133,
     294,    63,   133,   294,   107,   214,   132,   289,   220,   221,
     220,    11,    21,    66,   167,   168,   171,   175,   176,   224,
     225,   146,    48,    48,   224,    42,   146,   146,   146,   150,
     152,    48,   146,   146,   216,    34,   130,   215,   295,   290,
     291,   292,   293,   294,   132,   295,   295,    64,   222,   223,
     295,   130,   130,   130,   224,    77,   224,   147,   177,    63,
     218,     1,     6,    10,    14,    15,    16,    18,    20,    22,
      28,    29,    36,    37,    38,    39,    40,    41,    45,    52,
      53,    59,    60,    61,    62,    65,    67,    68,    70,    74,
      80,    81,    82,    83,    85,    86,    87,    89,    90,    94,
      95,    96,    97,   104,   105,   109,   111,   113,   120,   128,
     129,   130,   132,   139,   140,   147,   150,   184,   186,   196,
     202,   226,   227,   228,   229,   230,   232,   233,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     254,   257,   258,   259,   260,   261,   262,   263,   264,   269,
     270,   271,   272,   273,   274,   276,   279,   280,   281,   284,
     287,   288,   291,   293,   301,   309,    27,   130,   130,   295,
     133,   293,   298,   299,   309,   130,    27,   130,   130,   217,
     295,    20,    40,    44,    45,    51,    63,    68,    83,    90,
     106,   111,   113,   130,   137,   291,   131,   134,   293,   133,
     297,   290,   146,   146,   130,   146,   146,    77,   218,   130,
     218,   224,   112,   178,   219,   295,   146,   309,   130,   234,
     147,   150,   147,   150,   224,    91,   147,   130,   247,   291,
     293,   130,   291,   309,   291,   147,   150,   147,   309,   309,
     309,   289,   130,   147,   150,   293,   147,   147,   291,   293,
     130,   131,   296,   147,   150,   147,   291,   291,   293,   293,
     130,   147,   150,   147,   147,   291,   309,   293,   131,   302,
     131,   147,   309,   309,   309,   298,   146,   146,     4,     7,
       8,    54,    55,    56,   114,   115,   116,   117,   118,   149,
       5,    12,    13,    48,    71,    88,   121,   122,   123,   135,
     136,   137,   138,   139,   140,   141,   142,   145,   310,   311,
      66,   146,    64,   236,   146,   146,   151,   134,   124,    66,
     146,   137,   146,   107,   185,   131,   131,   293,   224,   130,
     295,    30,    31,   303,   295,   148,   147,    77,   146,   220,
     146,   147,   298,   298,   298,   298,   295,   291,   147,    48,
     102,   298,   298,    46,   132,   293,   295,   309,   298,   298,
     119,   293,   291,    17,    72,   282,   283,   298,   298,   291,
      17,    72,   285,   286,   110,   265,   187,    26,   197,    42,
     298,   298,   132,   293,   291,    17,    72,   277,   278,    32,
     309,   148,   151,   147,   293,   130,   235,   130,   235,   235,
     309,   309,   111,   130,   249,   309,   309,   309,   309,   309,
     309,   309,   309,   309,   309,   309,   309,   146,   130,   146,
     309,   309,   146,   131,   146,   295,    30,    31,   106,   172,
     146,   129,   130,   142,   147,   304,   305,    26,   169,   146,
     224,   130,   298,   148,   151,   148,   151,   148,   291,   309,
     224,   148,   151,   134,   134,   134,   148,   151,   293,   134,
     148,   148,   298,   149,   295,    17,    72,   148,   151,   148,
     298,   149,   295,    17,    72,   293,    81,   267,    54,   191,
     309,   146,   133,   148,   151,   134,   134,   148,   298,   149,
     295,    17,    72,   224,   148,   291,   147,    69,   237,   237,
     237,   147,   291,   147,   124,   146,   304,   131,   118,   173,
     305,   141,   142,   145,    37,   104,   132,   139,   140,   308,
     146,   148,   147,   179,   148,   148,    25,    50,   255,    33,
     275,   132,   291,   291,   129,   301,   282,   149,   224,   298,
     149,   285,   149,   224,   298,   149,    42,   130,    43,   268,
     235,   190,   128,   129,   130,   132,   198,   199,   200,   201,
     146,   291,   291,   277,   149,   224,   298,   149,   295,   148,
     298,   130,   127,   238,   238,   291,   277,   251,   291,   293,
     309,    37,   104,   132,   308,   297,   148,   305,   305,   306,
     308,   129,   300,   301,   224,   146,    19,    23,    32,    35,
     256,   224,   295,   134,   148,   148,   148,   295,   224,   149,
     224,   295,   224,   149,   224,   309,   133,   237,    26,    98,
     106,   146,   192,    34,   199,   146,   134,   137,   203,   148,
     148,   295,   224,   149,   224,   112,   148,   309,   148,   295,
     146,   149,   300,   146,   129,   147,   307,   148,   224,   291,
     224,   224,   103,   309,   131,   188,    96,   146,   201,   309,
     204,   224,   291,   277,   252,   291,    49,   129,   295,   148,
     309,     1,   193,     1,    34,    99,   130,   205,   206,   207,
     210,    54,   295,   146,   253,   149,   141,    76,    92,    93,
      34,    34,   101,   128,   129,   130,   131,   132,   134,   137,
     139,   140,   146,   149,   194,   195,   146,    97,   130,   150,
     146,    42,    54,   150,   213,   130,   293,   148,    73,   129,
     309,   309,   129,    95,    95,    42,   291,   100,   130,   132,
     208,   209,   130,   291,    42,    69,   149,   148,    57,    58,
     266,   189,   189,   100,   151,   209,   131,   211,   151,   208,
     130,    24,   209,    54,   212,   211,   127,   231,    54,   130,
     147,   130,   298,   148
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   153,   154,   154,   155,   155,   155,   155,   155,   155,
     155,   155,   155,   155,   155,   155,   155,   156,   157,   157,
     157,   158,   158,   158,   159,   159,   160,   161,   162,   163,
     163,   164,   165,   165,   166,   167,   168,   169,   169,   169,
     169,   169,   170,   171,   172,   172,   172,   172,   173,   173,
     173,   173,   173,   174,   175,   175,   176,   177,   177,   178,
     178,   179,   179,   180,   180,   181,   182,   183,   184,   184,
     185,   185,   186,   186,   187,   188,   189,   190,   190,   191,
     191,   192,   192,   192,   193,   193,   194,   194,   194,   194,
     194,   194,   194,   194,   195,   195,   195,   195,   195,   196,
     197,   197,   198,   198,   199,   200,   200,   201,   201,   201,
     201,   203,   202,   204,   204,   204,   205,   205,   206,   207,
     207,   208,   208,   209,   209,   210,   210,   211,   211,   212,
     212,   213,   214,   214,   215,   216,   216,   217,   217,   218,
     219,   218,   220,   221,   221,   222,   223,   224,   225,   225,
     225,   225,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   227,   228,   229,   230,   230,   231,   231,
     232,   233,   234,   234,   235,   235,   236,   236,   237,   237,
     238,   238,   239,   240,   240,   240,   241,   241,   242,   243,
     244,   245,   246,   247,   247,   248,   248,   249,   249,   249,
     250,   251,   252,   253,   253,   254,   255,   255,   255,   256,
     256,   256,   256,   257,   257,   258,   259,   260,   261,   262,
     263,   263,   264,   265,   265,   265,   265,   266,   266,   266,
     267,   267,   268,   268,   269,   270,   271,   272,   273,   274,
     275,   275,   276,   276,   276,   276,   277,   278,   278,   278,
     278,   279,   279,   279,   279,   279,   280,   281,   281,   282,
     283,   283,   283,   283,   284,   284,   285,   286,   286,   286,
     286,   287,   288,   289,   289,   290,   290,   291,   292,   292,
     292,   293,   294,   294,   294,   295,   295,   295,   295,   295,
     295,   295,   295,   295,   295,   295,   295,   295,   295,   296,
     296,   297,   297,   298,   298,   299,   299,   299,   299,   300,
     300,   301,   301,   302,   302,   303,   303,   303,   304,   304,
     305,   305,   305,   305,   305,   305,   306,   307,   307,   308,
     308,   308,   309,   309,   309,   309,   309,   309,   309,   309,
     309,   309,   309,   309,   309,   309,   309,   309,   309,   309,
     309,   309,   309,   309,   309,   309,   309,   309,   309,   309,
     309,   309,   309,   309,   309,   309,   309,   309,   309,   309,
     309,   309,   309,   310,   310,   310,   310,   310,   310,   311,
     311
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
       1,     1,     1,     1,     5,     5,     3,    12,     0,     4,
       4,     3,     1,     4,     1,     4,     0,     2,     0,     2,
       0,     2,     2,     8,     6,     6,     6,     6,     4,     2,
       2,     2,     2,     3,     3,     1,     3,     1,     2,     2,
       9,     3,     3,     0,     4,     8,     0,     1,     1,     1,
       1,     1,     1,     2,     4,     2,     2,     2,     2,     3,
       1,     2,     5,     0,     9,     8,     8,     0,     1,     1,
       0,     2,     0,     2,     2,     5,     2,     2,     6,     5,
       0,     2,     4,     6,     6,     8,     1,     4,     3,     5,
       4,     1,     1,     1,     1,     2,     2,     4,     6,     1,
       4,     3,     5,     4,     4,     6,     1,     4,     3,     5,
       4,     3,     3,     2,     3,     2,     3,     1,     1,     3,
       2,     2,     1,     3,     4,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     1,     0,
       1,     0,     1,     1,     0,     1,     3,     3,     5,     1,
       1,     2,     2,     0,     1,     2,     1,     0,     1,     1,
       1,     1,     3,     3,     3,     3,     2,     1,     5,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       3,     3,     3,     3,     3,     2,     2,     6,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     3,     1,     1,     1,     1,     1,     1,     1,
       1
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
#line 889 "ascend/compiler/ascParse.y"
        {
	  /*
	   * The specific syntax diagnostic is already emitted by zz_error().
	   * Emitting a generic message here causes noisy duplicates during
	   * parser recovery.
	   */
	}
#line 2826 "ascend/compiler/ascParse.c"
    break;

  case 17: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 900 "ascend/compiler/ascParse.y"
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
#line 2872 "ascend/compiler/ascParse.c"
    break;

  case 18: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 945 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 2880 "ascend/compiler/ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK name ';'  */
#line 949 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2889 "ascend/compiler/ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name  */
#line 954 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2898 "ascend/compiler/ascParse.c"
    break;

  case 21: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 962 "ascend/compiler/ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 2906 "ascend/compiler/ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK name ';'  */
#line 966 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2915 "ascend/compiler/ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name  */
#line 971 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2924 "ascend/compiler/ascParse.c"
    break;

  case 24: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 979 "ascend/compiler/ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
            );
          }
	}
#line 2937 "ascend/compiler/ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 988 "ascend/compiler/ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 2950 "ascend/compiler/ascParse.c"
    break;

  case 26: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 1000 "ascend/compiler/ascParse.y"
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
#line 2982 "ascend/compiler/ascParse.c"
    break;

  case 27: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 1031 "ascend/compiler/ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 2991 "ascend/compiler/ascParse.c"
    break;

  case 28: /* add_method_def: add_method_head proclist end ';'  */
#line 1039 "ascend/compiler/ascParse.y"
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
#line 3015 "ascend/compiler/ascParse.c"
    break;

  case 29: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1062 "ascend/compiler/ascParse.y"
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
#line 3032 "ascend/compiler/ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1075 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3041 "ascend/compiler/ascParse.c"
    break;

  case 31: /* replace_method_def: replace_method_head proclist end ';'  */
#line 1083 "ascend/compiler/ascParse.y"
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
#line 3063 "ascend/compiler/ascParse.c"
    break;

  case 32: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1104 "ascend/compiler/ascParse.y"
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
#line 3079 "ascend/compiler/ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1116 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3088 "ascend/compiler/ascParse.c"
    break;

  case 34: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 1124 "ascend/compiler/ascParse.y"
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
#line 3142 "ascend/compiler/ascParse.c"
    break;

  case 35: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 1177 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 3154 "ascend/compiler/ascParse.c"
    break;

  case 36: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 1188 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3163 "ascend/compiler/ascParse.c"
    break;

  case 37: /* default_val: %empty  */
#line 1196 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3174 "ascend/compiler/ascParse.c"
    break;

  case 38: /* default_val: DEFAULT_TOK optional_sign number  */
#line 1203 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3184 "ascend/compiler/ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 1209 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 3196 "ascend/compiler/ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 1217 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 3208 "ascend/compiler/ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 1225 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 3220 "ascend/compiler/ascParse.c"
    break;

  case 42: /* constant_def: universal constant_head  */
#line 1236 "ascend/compiler/ascParse.y"
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
#line 3261 "ascend/compiler/ascParse.c"
    break;

  case 43: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK constant_dims constant_val optional_notes ';'  */
#line 1277 "ascend/compiler/ascParse.y"
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
#line 3295 "ascend/compiler/ascParse.c"
    break;

  case 44: /* constant_dims: DIMENSION_TOK dimensions  */
#line 1310 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	  g_constant_units = NULL;
	}
#line 3304 "ascend/compiler/ascParse.c"
    break;

  case 45: /* constant_dims: DIMENSIONLESS_TOK  */
#line 1315 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	  g_constant_units = NULL;
	}
#line 3313 "ascend/compiler/ascParse.c"
    break;

  case 46: /* constant_dims: %empty  */
#line 1320 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	  g_constant_units = NULL;
	}
#line 3322 "ascend/compiler/ascParse.c"
    break;

  case 47: /* constant_dims: UNITS_TOK BRACEDTEXT_TOK  */
#line 1325 "ascend/compiler/ascParse.y"
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
#line 3344 "ascend/compiler/ascParse.c"
    break;

  case 48: /* constant_val: %empty  */
#line 1346 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3355 "ascend/compiler/ascParse.c"
    break;

  case 49: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 1353 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3365 "ascend/compiler/ascParse.c"
    break;

  case 50: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 1359 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3377 "ascend/compiler/ascParse.c"
    break;

  case 51: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 1367 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3389 "ascend/compiler/ascParse.c"
    break;

  case 52: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 1375 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 3401 "ascend/compiler/ascParse.c"
    break;

  case 53: /* model_def: universal model_head fstatements methods end ';'  */
#line 1386 "ascend/compiler/ascParse.y"
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
#line 3443 "ascend/compiler/ascParse.c"
    break;

  case 54: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 1428 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 3455 "ascend/compiler/ascParse.c"
    break;

  case 55: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 1437 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 3468 "ascend/compiler/ascParse.c"
    break;

  case 56: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 1449 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3477 "ascend/compiler/ascParse.c"
    break;

  case 57: /* optional_model_parameters: %empty  */
#line 1457 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3485 "ascend/compiler/ascParse.c"
    break;

  case 58: /* optional_model_parameters: '(' fstatements ')'  */
#line 1461 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3493 "ascend/compiler/ascParse.c"
    break;

  case 59: /* optional_parameter_wheres: %empty  */
#line 1468 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3501 "ascend/compiler/ascParse.c"
    break;

  case 60: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 1472 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3509 "ascend/compiler/ascParse.c"
    break;

  case 61: /* optional_parameter_reduction: %empty  */
#line 1479 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3517 "ascend/compiler/ascParse.c"
    break;

  case 62: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 1483 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3525 "ascend/compiler/ascParse.c"
    break;

  case 63: /* universal: %empty  */
#line 1533 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3533 "ascend/compiler/ascParse.c"
    break;

  case 64: /* universal: UNIVERSAL_TOK  */
#line 1537 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3541 "ascend/compiler/ascParse.c"
    break;

  case 65: /* definition_def: definition_id fstatements methods end ';'  */
#line 1544 "ascend/compiler/ascParse.y"
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
#line 3572 "ascend/compiler/ascParse.c"
    break;

  case 66: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1574 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3581 "ascend/compiler/ascParse.c"
    break;

  case 67: /* units_def: units_statement ';'  */
#line 1583 "ascend/compiler/ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 3587 "ascend/compiler/ascParse.c"
    break;

  case 68: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1588 "ascend/compiler/ascParse.y"
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
#line 3608 "ascend/compiler/ascParse.c"
    break;

  case 69: /* units_statement: UNITS_TOK LADDER_TOK unitladderitemlist end optional_ladder_end  */
#line 1605 "ascend/compiler/ascParse.y"
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
#line 3634 "ascend/compiler/ascParse.c"
    break;

  case 70: /* optional_ladder_end: %empty  */
#line 1630 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3642 "ascend/compiler/ascParse.c"
    break;

  case 71: /* optional_ladder_end: LADDER_TOK  */
#line 1634 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3650 "ascend/compiler/ascParse.c"
    break;

  case 72: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on table_body END_TOK TABLE_TOK table_mode_off  */
#line 1641 "ascend/compiler/ascParse.y"
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
#line 3675 "ascend/compiler/ascParse.c"
    break;

  case 73: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on error END_TOK TABLE_TOK table_mode_off  */
#line 1662 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-9].nptr));
	  TableParseAbort();
	  ErrMsg_Generic("Error in TABLE body.");
	  g_untrapped_error++;
	  yyerrok;
	  (yyval.statptr) = NULL;
	}
#line 3688 "ascend/compiler/ascParse.c"
    break;

  case 74: /* table_begin: %empty  */
#line 1674 "ascend/compiler/ascParse.y"
        {
	  TableParseBegin();
	}
#line 3696 "ascend/compiler/ascParse.c"
    break;

  case 75: /* table_mode_on: %empty  */
#line 1681 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(1);
	}
#line 3704 "ascend/compiler/ascParse.c"
    break;

  case 76: /* table_mode_off: %empty  */
#line 1688 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(0);
	}
#line 3712 "ascend/compiler/ascParse.c"
    break;

  case 79: /* table_decl_opt: %empty  */
#line 1700 "ascend/compiler/ascParse.y"
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
#line 3729 "ascend/compiler/ascParse.c"
    break;

  case 80: /* table_decl_opt: ISA_TOK type_identifier optional_of  */
#line 1713 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_type = (yyvsp[-1].id_ptr);
	  g_table_parse.decl_set_type = (yyvsp[0].id_ptr);
	  g_table_parse.decl_typeargs = g_typeargs;
	  g_typeargs = NULL;
	}
#line 3743 "ascend/compiler/ascParse.c"
    break;

  case 81: /* table_option: POSITIONAL_TOK  */
#line 1726 "ascend/compiler/ascParse.y"
        {
	  g_table_parse.positional = 1;
	}
#line 3751 "ascend/compiler/ascParse.c"
    break;

  case 82: /* table_option: DEFAULT_TOK expr  */
#line 1730 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.default_expr != NULL) {
	    DestroyExprList(g_table_parse.default_expr);
	  }
	  g_table_parse.default_expr = (yyvsp[0].eptr);
	}
#line 3762 "ascend/compiler/ascParse.c"
    break;

  case 83: /* table_option: UNITS_TOK BRACEDTEXT_TOK  */
#line 1737 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.units != NULL) {
	    ascfree(g_table_parse.units);
	  }
	  g_table_parse.units = ASC_STRDUP((yyvsp[0].braced_ptr));
	}
#line 3773 "ascend/compiler/ascParse.c"
    break;

  case 87: /* table_body_item: ':'  */
#line 1753 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(":",0);
	}
#line 3781 "ascend/compiler/ascParse.c"
    break;

  case 88: /* table_body_item: '='  */
#line 1757 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("=",0);
	}
#line 3789 "ascend/compiler/ascParse.c"
    break;

  case 89: /* table_body_item: ','  */
#line 1761 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(",",0);
	}
#line 3797 "ascend/compiler/ascParse.c"
    break;

  case 90: /* table_body_item: '+'  */
#line 1765 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("+",0);
	}
#line 3805 "ascend/compiler/ascParse.c"
    break;

  case 91: /* table_body_item: '-'  */
#line 1769 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("-",0);
	}
#line 3813 "ascend/compiler/ascParse.c"
    break;

  case 92: /* table_body_item: ';'  */
#line 1773 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3821 "ascend/compiler/ascParse.c"
    break;

  case 93: /* table_body_item: EOL_TOK  */
#line 1777 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3829 "ascend/compiler/ascParse.c"
    break;

  case 94: /* table_scalar: IDENTIFIER_TOK  */
#line 1784 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(SCP((yyvsp[0].id_ptr)),1);
	}
#line 3837 "ascend/compiler/ascParse.c"
    break;

  case 95: /* table_scalar: SYMBOL_TOK  */
#line 1788 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendSymbol((yyvsp[0].sym_ptr));
	}
#line 3845 "ascend/compiler/ascParse.c"
    break;

  case 96: /* table_scalar: INTEGER_TOK  */
#line 1792 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendInteger((yyvsp[0].int_value));
	}
#line 3853 "ascend/compiler/ascParse.c"
    break;

  case 97: /* table_scalar: REAL_TOK  */
#line 1796 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendReal((yyvsp[0].real_value));
	}
#line 3861 "ascend/compiler/ascParse.c"
    break;

  case 98: /* table_scalar: BRACEDTEXT_TOK  */
#line 1800 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendBraced((yyvsp[0].braced_ptr));
	}
#line 3869 "ascend/compiler/ascParse.c"
    break;

  case 99: /* values_statement: VALUES_TOK fname values_default_opt ';' values_entries END_TOK VALUES_TOK  */
#line 1807 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-5].nptr));
	  (yyval.statptr) = NULL;
	}
#line 3878 "ascend/compiler/ascParse.c"
    break;

  case 101: /* values_default_opt: DEFAULT_TOK expr  */
#line 1816 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3886 "ascend/compiler/ascParse.c"
    break;

  case 104: /* values_entry: values_key_list '=' expr  */
#line 1828 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3894 "ascend/compiler/ascParse.c"
    break;

  case 111: /* $@1: %empty  */
#line 1847 "ascend/compiler/ascParse.y"
        {
	  DatasetParseBegin((yyvsp[-3].id_ptr),(yyvsp[-1].dquote_ptr));
	}
#line 3902 "ascend/compiler/ascParse.c"
    break;

  case 112: /* dataset_statement: DATASET_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';' $@1 dataset_items END_TOK DATASET_TOK  */
#line 1851 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = DatasetParseFinish();
	}
#line 3910 "ascend/compiler/ascParse.c"
    break;

  case 115: /* dataset_items: dataset_items error ';'  */
#line 1860 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in DATASET item.");
	  ErrMsg_Generic("Check DATASET statement syntax.");
	  g_untrapped_error++;
	  yyerrok;
	}
#line 3921 "ascend/compiler/ascParse.c"
    break;

  case 118: /* dataset_index_item: INDEX_TOK IDENTIFIER_TOK FROM_TOK COLUMN_TOK dataset_column_ref ISA_TOK IDENTIFIER_TOK  */
#line 1875 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddIndex((yyvsp[-5].id_ptr),(yyvsp[-2].id_ptr),(yyvsp[0].id_ptr));
	}
#line 3929 "ascend/compiler/ascParse.c"
    break;

  case 119: /* dataset_map_item: dataset_target FROM_TOK dataset_column_selector dataset_units_opt dataset_type_opt  */
#line 1882 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-2].id_ptr),(yyvsp[-1].braced_ptr),(yyvsp[0].id_ptr));
	}
#line 3937 "ascend/compiler/ascParse.c"
    break;

  case 120: /* dataset_map_item: dataset_target dataset_type_req FROM_TOK dataset_column_selector dataset_units_opt  */
#line 1886 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-1].id_ptr),(yyvsp[0].braced_ptr),(yyvsp[-3].id_ptr));
	}
#line 3945 "ascend/compiler/ascParse.c"
    break;

  case 121: /* dataset_column_selector: COLUMN_TOK dataset_column_ref  */
#line 1893 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3953 "ascend/compiler/ascParse.c"
    break;

  case 122: /* dataset_column_selector: dataset_column_ref  */
#line 1897 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3961 "ascend/compiler/ascParse.c"
    break;

  case 123: /* dataset_column_ref: IDENTIFIER_TOK  */
#line 1904 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3969 "ascend/compiler/ascParse.c"
    break;

  case 124: /* dataset_column_ref: SYMBOL_TOK  */
#line 1908 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].sym_ptr);
	}
#line 3977 "ascend/compiler/ascParse.c"
    break;

  case 125: /* dataset_target: IDENTIFIER_TOK '[' fvarlist ']'  */
#line 1915 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices(CreateIdName((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 3986 "ascend/compiler/ascParse.c"
    break;

  case 126: /* dataset_target: dataset_target '[' fvarlist ']'  */
#line 1920 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices((yyvsp[-3].nptr),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 3995 "ascend/compiler/ascParse.c"
    break;

  case 127: /* dataset_units_opt: %empty  */
#line 1928 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 4003 "ascend/compiler/ascParse.c"
    break;

  case 128: /* dataset_units_opt: BRACEDTEXT_TOK  */
#line 1932 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 4011 "ascend/compiler/ascParse.c"
    break;

  case 129: /* dataset_type_opt: %empty  */
#line 1939 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4019 "ascend/compiler/ascParse.c"
    break;

  case 130: /* dataset_type_opt: ISA_TOK IDENTIFIER_TOK  */
#line 1943 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4027 "ascend/compiler/ascParse.c"
    break;

  case 131: /* dataset_type_req: ISA_TOK IDENTIFIER_TOK  */
#line 1950 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4035 "ascend/compiler/ascParse.c"
    break;

  case 132: /* unitdeflist: %empty  */
#line 1956 "ascend/compiler/ascParse.y"
                {
		  (yyval.listp) = gl_create(100L);
	}
#line 4043 "ascend/compiler/ascParse.c"
    break;

  case 133: /* unitdeflist: unitdeflist unitdef  */
#line 1960 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 4052 "ascend/compiler/ascParse.c"
    break;

  case 134: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1968 "ascend/compiler/ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4061 "ascend/compiler/ascParse.c"
    break;

  case 135: /* unitladderitemlist: %empty  */
#line 1975 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(20L);
	}
#line 4069 "ascend/compiler/ascParse.c"
    break;

  case 136: /* unitladderitemlist: unitladderitemlist unitladderitem ';'  */
#line 1979 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].ulitemptr));
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4078 "ascend/compiler/ascParse.c"
    break;

  case 137: /* unitladderitem: IDENTIFIER_TOK '=' BRACEDTEXT_TOK  */
#line 1987 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[-2].id_ptr),(yyvsp[0].braced_ptr),0,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4087 "ascend/compiler/ascParse.c"
    break;

  case 138: /* unitladderitem: IDENTIFIER_TOK  */
#line 1992 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[0].id_ptr),NULL,1,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4096 "ascend/compiler/ascParse.c"
    break;

  case 139: /* methods: %empty  */
#line 2001 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 4104 "ascend/compiler/ascParse.c"
    break;

  case 140: /* $@2: %empty  */
#line 2005 "ascend/compiler/ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 4115 "ascend/compiler/ascParse.c"
    break;

  case 141: /* methods: METHODS_TOK $@2 proclist  */
#line 2012 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 4123 "ascend/compiler/ascParse.c"
    break;

  case 142: /* proclist: proclistf  */
#line 2019 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 4132 "ascend/compiler/ascParse.c"
    break;

  case 143: /* proclistf: %empty  */
#line 2026 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4140 "ascend/compiler/ascParse.c"
    break;

  case 144: /* proclistf: proclistf procedure  */
#line 2030 "ascend/compiler/ascParse.y"
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
#line 4166 "ascend/compiler/ascParse.c"
    break;

  case 145: /* procedure: procedure_id ';' fstatements end ';'  */
#line 2055 "ascend/compiler/ascParse.y"
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
#line 4181 "ascend/compiler/ascParse.c"
    break;

  case 146: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 2069 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 4190 "ascend/compiler/ascParse.c"
    break;

  case 147: /* fstatements: statements  */
#line 2078 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 4198 "ascend/compiler/ascParse.c"
    break;

  case 148: /* statements: %empty  */
#line 2085 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4206 "ascend/compiler/ascParse.c"
    break;

  case 149: /* statements: statements statement ';'  */
#line 2089 "ascend/compiler/ascParse.y"
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
#line 4276 "ascend/compiler/ascParse.c"
    break;

  case 150: /* statements: statements complex_statement ';'  */
#line 2155 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
            gl_destroy((yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4288 "ascend/compiler/ascParse.c"
    break;

  case 151: /* statements: statements error ';'  */
#line 2163 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4297 "ascend/compiler/ascParse.c"
    break;

  case 194: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 2221 "ascend/compiler/ascParse.y"
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
#line 4339 "ascend/compiler/ascParse.c"
    break;

  case 195: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 2262 "ascend/compiler/ascParse.y"
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
#line 4369 "ascend/compiler/ascParse.c"
    break;

  case 196: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 2291 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 4377 "ascend/compiler/ascParse.c"
    break;

  case 197: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 2296 "ascend/compiler/ascParse.y"
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
#line 4427 "ascend/compiler/ascParse.c"
    break;

  case 198: /* optional_set_values: %empty  */
#line 2345 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4435 "ascend/compiler/ascParse.c"
    break;

  case 199: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 2349 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 4443 "ascend/compiler/ascParse.c"
    break;

  case 200: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 2356 "ascend/compiler/ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 4456 "ascend/compiler/ascParse.c"
    break;

  case 201: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 2368 "ascend/compiler/ascParse.y"
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
#line 4484 "ascend/compiler/ascParse.c"
    break;

  case 202: /* call_identifier: IDENTIFIER_TOK  */
#line 2395 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 4493 "ascend/compiler/ascParse.c"
    break;

  case 203: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2400 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 4502 "ascend/compiler/ascParse.c"
    break;

  case 204: /* type_identifier: IDENTIFIER_TOK  */
#line 2408 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 4511 "ascend/compiler/ascParse.c"
    break;

  case 205: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2413 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 4520 "ascend/compiler/ascParse.c"
    break;

  case 206: /* optional_method: %empty  */
#line 2421 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4528 "ascend/compiler/ascParse.c"
    break;

  case 207: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 2425 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4536 "ascend/compiler/ascParse.c"
    break;

  case 208: /* optional_of: %empty  */
#line 2432 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4544 "ascend/compiler/ascParse.c"
    break;

  case 209: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 2436 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4552 "ascend/compiler/ascParse.c"
    break;

  case 210: /* optional_with_value: %empty  */
#line 2443 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 4560 "ascend/compiler/ascParse.c"
    break;

  case 211: /* optional_with_value: WITH_VALUE_T expr  */
#line 2447 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4568 "ascend/compiler/ascParse.c"
    break;

  case 212: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 2454 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 4576 "ascend/compiler/ascParse.c"
    break;

  case 213: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 2461 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4584 "ascend/compiler/ascParse.c"
    break;

  case 214: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2465 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4592 "ascend/compiler/ascParse.c"
    break;

  case 215: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 2469 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4600 "ascend/compiler/ascParse.c"
    break;

  case 216: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2476 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4608 "ascend/compiler/ascParse.c"
    break;

  case 217: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 2480 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4616 "ascend/compiler/ascParse.c"
    break;

  case 218: /* der_statement: DER_TOK '(' fvarlist ')'  */
#line 2487 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 4626 "ascend/compiler/ascParse.c"
    break;

  case 219: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 2496 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 4636 "ascend/compiler/ascParse.c"
    break;

  case 220: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 2505 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 4644 "ascend/compiler/ascParse.c"
    break;

  case 221: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 2512 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 4652 "ascend/compiler/ascParse.c"
    break;

  case 222: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 2519 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 4660 "ascend/compiler/ascParse.c"
    break;

  case 223: /* assignment_statement: fname ASSIGN_TOK expr  */
#line 2526 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4668 "ascend/compiler/ascParse.c"
    break;

  case 224: /* assignment_statement: fname CASSIGN_TOK expr  */
#line 2530 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4676 "ascend/compiler/ascParse.c"
    break;

  case 225: /* relation_statement: relation  */
#line 2537 "ascend/compiler/ascParse.y"
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
#line 4693 "ascend/compiler/ascParse.c"
    break;

  case 226: /* relation_statement: fname ':' relation  */
#line 2550 "ascend/compiler/ascParse.y"
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
#line 4711 "ascend/compiler/ascParse.c"
    break;

  case 227: /* relation: expr  */
#line 2567 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 4725 "ascend/compiler/ascParse.c"
    break;

  case 228: /* relation: MINIMIZE_TOK expr  */
#line 2577 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4737 "ascend/compiler/ascParse.c"
    break;

  case 229: /* relation: MAXIMIZE_TOK expr  */
#line 2585 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4749 "ascend/compiler/ascParse.c"
    break;

  case 230: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 2596 "ascend/compiler/ascParse.y"
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
#line 4771 "ascend/compiler/ascParse.c"
    break;

  case 231: /* input_args: fvarlist ':' INPUT_TOK  */
#line 2617 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4779 "ascend/compiler/ascParse.c"
    break;

  case 232: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 2624 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4787 "ascend/compiler/ascParse.c"
    break;

  case 233: /* data_args: %empty  */
#line 2631 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 4795 "ascend/compiler/ascParse.c"
    break;

  case 234: /* data_args: ';' fname ':' DATA_TOK  */
#line 2635 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 4803 "ascend/compiler/ascParse.c"
    break;

  case 235: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 2673 "ascend/compiler/ascParse.y"
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
#line 4824 "ascend/compiler/ascParse.c"
    break;

  case 236: /* optional_direction: %empty  */
#line 2693 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 4832 "ascend/compiler/ascParse.c"
    break;

  case 237: /* optional_direction: INCREASING_TOK  */
#line 2697 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 4840 "ascend/compiler/ascParse.c"
    break;

  case 238: /* optional_direction: DECREASING_TOK  */
#line 2701 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 4848 "ascend/compiler/ascParse.c"
    break;

  case 239: /* forexprend: CREATE_TOK  */
#line 2708 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 4856 "ascend/compiler/ascParse.c"
    break;

  case 240: /* forexprend: EXPECT_TOK  */
#line 2712 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 4864 "ascend/compiler/ascParse.c"
    break;

  case 241: /* forexprend: CHECK_TOK  */
#line 2716 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 4872 "ascend/compiler/ascParse.c"
    break;

  case 242: /* forexprend: DO_TOK  */
#line 2720 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 4880 "ascend/compiler/ascParse.c"
    break;

  case 243: /* run_statement: RUN_TOK fname  */
#line 2727 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 4888 "ascend/compiler/ascParse.c"
    break;

  case 244: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 2731 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 4896 "ascend/compiler/ascParse.c"
    break;

  case 245: /* fix_statement: FIX_TOK fvarlist  */
#line 2738 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 4905 "ascend/compiler/ascParse.c"
    break;

  case 246: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 2746 "ascend/compiler/ascParse.y"
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
#line 4920 "ascend/compiler/ascParse.c"
    break;

  case 247: /* free_statement: FREE_TOK fvarlist  */
#line 2760 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 4928 "ascend/compiler/ascParse.c"
    break;

  case 248: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 2767 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 4937 "ascend/compiler/ascParse.c"
    break;

  case 249: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 2775 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 4946 "ascend/compiler/ascParse.c"
    break;

  case 250: /* solve_statement: SOLVE_TOK  */
#line 2783 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE(NULL);
	}
#line 4955 "ascend/compiler/ascParse.c"
    break;

  case 251: /* solve_statement: SOLVE_TOK fname  */
#line 2788 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSOLVE((yyvsp[0].nptr));
	}
#line 4963 "ascend/compiler/ascParse.c"
    break;

  case 252: /* study_statement: STUDY_TOK fvarlist study_vary_opt study_run_opt study_file_opt  */
#line 2795 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSTUDY((yyvsp[-3].lptr), (yyvsp[-2].studyparse).vary, (yyvsp[-2].studyparse).lower, (yyvsp[-2].studyparse).upper, (yyvsp[-2].studyparse).steps, (yyvsp[-2].studyparse).value,
			(yyvsp[-2].studyparse).mode, (yyvsp[-2].studyparse).dist, (yyvsp[-1].studyparse).run_method, (yyvsp[0].studyparse).filename);
	}
#line 4972 "ascend/compiler/ascParse.c"
    break;

  case 253: /* study_vary_opt: %empty  */
#line 2803 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
	}
#line 4980 "ascend/compiler/ascParse.c"
    break;

  case 254: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEPS_TOK INTEGER_TOK study_distribution_opt  */
#line 2807 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
		(yyval.studyparse).vary = (yyvsp[-7].nptr);
		(yyval.studyparse).lower = (yyvsp[-5].eptr);
		(yyval.studyparse).upper = (yyvsp[-3].eptr);
		(yyval.studyparse).steps = (yyvsp[-1].int_value);
		(yyval.studyparse).mode = study_steps;
		(yyval.studyparse).dist = (yyvsp[0].studyparse).dist;
	}
#line 4994 "ascend/compiler/ascParse.c"
    break;

  case 255: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEP_TOK expr  */
#line 2817 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
		(yyval.studyparse).vary = (yyvsp[-6].nptr);
		(yyval.studyparse).lower = (yyvsp[-4].eptr);
		(yyval.studyparse).upper = (yyvsp[-2].eptr);
		(yyval.studyparse).value = (yyvsp[0].eptr);
		(yyval.studyparse).mode = study_step;
		(yyval.studyparse).dist = study_dist_linear;
	}
#line 5008 "ascend/compiler/ascParse.c"
    break;

  case 256: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr RATIO_TOK expr  */
#line 2827 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
		(yyval.studyparse).vary = (yyvsp[-6].nptr);
		(yyval.studyparse).lower = (yyvsp[-4].eptr);
		(yyval.studyparse).upper = (yyvsp[-2].eptr);
		(yyval.studyparse).value = (yyvsp[0].eptr);
		(yyval.studyparse).mode = study_ratio;
		(yyval.studyparse).dist = study_dist_log;
	}
#line 5022 "ascend/compiler/ascParse.c"
    break;

  case 257: /* study_distribution_opt: %empty  */
#line 2840 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
	}
#line 5030 "ascend/compiler/ascParse.c"
    break;

  case 258: /* study_distribution_opt: LINEAR_TOK  */
#line 2844 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
		(yyval.studyparse).dist = study_dist_linear;
	}
#line 5039 "ascend/compiler/ascParse.c"
    break;

  case 259: /* study_distribution_opt: LOG_TOK  */
#line 2849 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
		(yyval.studyparse).dist = study_dist_log;
	}
#line 5048 "ascend/compiler/ascParse.c"
    break;

  case 260: /* study_run_opt: %empty  */
#line 2857 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
	}
#line 5056 "ascend/compiler/ascParse.c"
    break;

  case 261: /* study_run_opt: RUN_TOK IDENTIFIER_TOK  */
#line 2861 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
		(yyval.studyparse).run_method = (yyvsp[0].id_ptr);
	}
#line 5065 "ascend/compiler/ascParse.c"
    break;

  case 262: /* study_file_opt: %empty  */
#line 2869 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
	}
#line 5073 "ascend/compiler/ascParse.c"
    break;

  case 263: /* study_file_opt: FILE_TOK DQUOTE_TOK  */
#line 2873 "ascend/compiler/ascParse.y"
        {
		(yyval.studyparse) = StudyParseEmpty();
		(yyval.studyparse).filename = (yyvsp[0].dquote_ptr);
	}
#line 5082 "ascend/compiler/ascParse.c"
    break;

  case 264: /* delete_statement: DELETE_TOK SYSTEM_TOK  */
#line 2881 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateDELETESYSTEM();
	}
#line 5090 "ascend/compiler/ascParse.c"
    break;

  case 265: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 2888 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 5102 "ascend/compiler/ascParse.c"
    break;

  case 266: /* call_statement: CALL_TOK call_identifier  */
#line 2899 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 5114 "ascend/compiler/ascParse.c"
    break;

  case 267: /* assert_statement: ASSERT_TOK expr  */
#line 2910 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 5122 "ascend/compiler/ascParse.c"
    break;

  case 268: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 2916 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 5133 "ascend/compiler/ascParse.c"
    break;

  case 269: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 2926 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 5144 "ascend/compiler/ascParse.c"
    break;

  case 270: /* optional_else: %empty  */
#line 2935 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 5152 "ascend/compiler/ascParse.c"
    break;

  case 271: /* optional_else: ELSE_TOK fstatements  */
#line 2939 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 5160 "ascend/compiler/ascParse.c"
    break;

  case 272: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 2946 "ascend/compiler/ascParse.y"
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
#line 5175 "ascend/compiler/ascParse.c"
    break;

  case 273: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 2957 "ascend/compiler/ascParse.y"
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
#line 5191 "ascend/compiler/ascParse.c"
    break;

  case 274: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2969 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5202 "ascend/compiler/ascParse.c"
    break;

  case 275: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2976 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5213 "ascend/compiler/ascParse.c"
    break;

  case 276: /* whenlist: whenlistf  */
#line 2986 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 5221 "ascend/compiler/ascParse.c"
    break;

  case 277: /* whenlistf: CASE_TOK set ':' fstatements  */
#line 2993 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5229 "ascend/compiler/ascParse.c"
    break;

  case 278: /* whenlistf: OTHERWISE_TOK ':' fstatements  */
#line 2997 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	}
#line 5237 "ascend/compiler/ascParse.c"
    break;

  case 279: /* whenlistf: whenlistf CASE_TOK set ':' fstatements  */
#line 3001 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	}
#line 5245 "ascend/compiler/ascParse.c"
    break;

  case 280: /* whenlistf: whenlistf OTHERWISE_TOK ':' fstatements  */
#line 3005 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	}
#line 5253 "ascend/compiler/ascParse.c"
    break;

  case 281: /* flow_statement: BREAK_TOK  */
#line 3012 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 5261 "ascend/compiler/ascParse.c"
    break;

  case 282: /* flow_statement: CONTINUE_TOK  */
#line 3016 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 5269 "ascend/compiler/ascParse.c"
    break;

  case 283: /* flow_statement: FALLTHRU_TOK  */
#line 3020 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 5277 "ascend/compiler/ascParse.c"
    break;

  case 284: /* flow_statement: RETURN_TOK  */
#line 3024 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 5285 "ascend/compiler/ascParse.c"
    break;

  case 285: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 3028 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 5293 "ascend/compiler/ascParse.c"
    break;

  case 286: /* use_statement: USE_TOK fname  */
#line 3035 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 5301 "ascend/compiler/ascParse.c"
    break;

  case 287: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 3042 "ascend/compiler/ascParse.y"
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
#line 5316 "ascend/compiler/ascParse.c"
    break;

  case 288: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 3053 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 5327 "ascend/compiler/ascParse.c"
    break;

  case 289: /* selectlist: selectlistf  */
#line 3063 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 5335 "ascend/compiler/ascParse.c"
    break;

  case 290: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 3070 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5343 "ascend/compiler/ascParse.c"
    break;

  case 291: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 3074 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 5351 "ascend/compiler/ascParse.c"
    break;

  case 292: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 3078 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 5359 "ascend/compiler/ascParse.c"
    break;

  case 293: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 3082 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 5367 "ascend/compiler/ascParse.c"
    break;

  case 294: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 3089 "ascend/compiler/ascParse.y"
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
#line 5382 "ascend/compiler/ascParse.c"
    break;

  case 295: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 3100 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 5393 "ascend/compiler/ascParse.c"
    break;

  case 296: /* switchlist: switchlistf  */
#line 3110 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 5401 "ascend/compiler/ascParse.c"
    break;

  case 297: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 3117 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5409 "ascend/compiler/ascParse.c"
    break;

  case 298: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 3121 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 5417 "ascend/compiler/ascParse.c"
    break;

  case 299: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 3125 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 5425 "ascend/compiler/ascParse.c"
    break;

  case 300: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 3129 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 5433 "ascend/compiler/ascParse.c"
    break;

  case 301: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 3136 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 5444 "ascend/compiler/ascParse.c"
    break;

  case 302: /* notes_statement: NOTES_TOK notes_body end  */
#line 3146 "ascend/compiler/ascParse.y"
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
#line 5482 "ascend/compiler/ascParse.c"
    break;

  case 303: /* notes_body: SYMBOL_TOK noteslist  */
#line 3183 "ascend/compiler/ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 5495 "ascend/compiler/ascParse.c"
    break;

  case 304: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 3192 "ascend/compiler/ascParse.y"
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
#line 5511 "ascend/compiler/ascParse.c"
    break;

  case 305: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 3207 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 5520 "ascend/compiler/ascParse.c"
    break;

  case 306: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 3212 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 5530 "ascend/compiler/ascParse.c"
    break;

  case 307: /* fvarlist: varlist  */
#line 3221 "ascend/compiler/ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5542 "ascend/compiler/ascParse.c"
    break;

  case 308: /* varlist: fname  */
#line 3232 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5550 "ascend/compiler/ascParse.c"
    break;

  case 309: /* varlist: varlist ',' fname  */
#line 3236 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5559 "ascend/compiler/ascParse.c"
    break;

  case 310: /* varlist: varlist fname  */
#line 3241 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 5573 "ascend/compiler/ascParse.c"
    break;

  case 311: /* fname: name optional_notes  */
#line 3254 "ascend/compiler/ascParse.y"
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
#line 5594 "ascend/compiler/ascParse.c"
    break;

  case 312: /* name: IDENTIFIER_TOK  */
#line 3274 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 5602 "ascend/compiler/ascParse.c"
    break;

  case 313: /* name: name '.' IDENTIFIER_TOK  */
#line 3278 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5611 "ascend/compiler/ascParse.c"
    break;

  case 314: /* name: name '[' set ']'  */
#line 3283 "ascend/compiler/ascParse.y"
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
#line 5627 "ascend/compiler/ascParse.c"
    break;

  case 315: /* end: END_TOK CONDITIONAL_TOK  */
#line 3298 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 5636 "ascend/compiler/ascParse.c"
    break;

  case 316: /* end: END_TOK FOR_TOK  */
#line 3303 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 5645 "ascend/compiler/ascParse.c"
    break;

  case 317: /* end: END_TOK IF_TOK  */
#line 3308 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 5654 "ascend/compiler/ascParse.c"
    break;

  case 318: /* end: END_TOK INTERACTIVE_TOK  */
#line 3313 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 5663 "ascend/compiler/ascParse.c"
    break;

  case 319: /* end: END_TOK METHODS_TOK  */
#line 3318 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 5672 "ascend/compiler/ascParse.c"
    break;

  case 320: /* end: END_TOK NOTES_TOK  */
#line 3323 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 5681 "ascend/compiler/ascParse.c"
    break;

  case 321: /* end: END_TOK SELECT_TOK  */
#line 3328 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 5690 "ascend/compiler/ascParse.c"
    break;

  case 322: /* end: END_TOK SWITCH_TOK  */
#line 3333 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 5699 "ascend/compiler/ascParse.c"
    break;

  case 323: /* end: END_TOK UNITS_TOK  */
#line 3338 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 5708 "ascend/compiler/ascParse.c"
    break;

  case 324: /* end: END_TOK GLOBAL_TOK  */
#line 3343 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 5717 "ascend/compiler/ascParse.c"
    break;

  case 325: /* end: END_TOK WHEN_TOK  */
#line 3348 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 5726 "ascend/compiler/ascParse.c"
    break;

  case 326: /* end: END_TOK WHILE_TOK  */
#line 3353 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 5735 "ascend/compiler/ascParse.c"
    break;

  case 327: /* end: END_TOK IDENTIFIER_TOK  */
#line 3358 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 5744 "ascend/compiler/ascParse.c"
    break;

  case 328: /* end: END_TOK  */
#line 3363 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 5753 "ascend/compiler/ascParse.c"
    break;

  case 329: /* optional_bracedtext: %empty  */
#line 3371 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 5761 "ascend/compiler/ascParse.c"
    break;

  case 330: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 3375 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 5769 "ascend/compiler/ascParse.c"
    break;

  case 331: /* optional_notes: %empty  */
#line 3382 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 5777 "ascend/compiler/ascParse.c"
    break;

  case 332: /* optional_notes: DQUOTE_TOK  */
#line 3386 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 5785 "ascend/compiler/ascParse.c"
    break;

  case 333: /* set: setexprlist  */
#line 3393 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 5793 "ascend/compiler/ascParse.c"
    break;

  case 334: /* set: %empty  */
#line 3397 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 5801 "ascend/compiler/ascParse.c"
    break;

  case 335: /* setexprlist: expr  */
#line 3404 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 5809 "ascend/compiler/ascParse.c"
    break;

  case 336: /* setexprlist: expr DOTDOT_TOK expr  */
#line 3408 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5817 "ascend/compiler/ascParse.c"
    break;

  case 337: /* setexprlist: setexprlist ',' expr  */
#line 3412 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 5826 "ascend/compiler/ascParse.c"
    break;

  case 338: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 3417 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 5835 "ascend/compiler/ascParse.c"
    break;

  case 339: /* number: INTEGER_TOK  */
#line 3425 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	  g_number_units = NULL;
	}
#line 5846 "ascend/compiler/ascParse.c"
    break;

  case 340: /* number: realnumber  */
#line 3432 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 5856 "ascend/compiler/ascParse.c"
    break;

  case 341: /* realnumber: REAL_TOK opunits  */
#line 3441 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	  g_number_units = g_parsed_units;
	}
#line 5865 "ascend/compiler/ascParse.c"
    break;

  case 342: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 3446 "ascend/compiler/ascParse.y"
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
#line 5890 "ascend/compiler/ascParse.c"
    break;

  case 343: /* opunits: %empty  */
#line 3470 "ascend/compiler/ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  g_parsed_units = NULL;
	  (yyval.real_value) = 1.0;
	}
#line 5900 "ascend/compiler/ascParse.c"
    break;

  case 344: /* opunits: BRACEDTEXT_TOK  */
#line 3476 "ascend/compiler/ascParse.y"
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
#line 5924 "ascend/compiler/ascParse.c"
    break;

  case 345: /* dims: DIMENSION_TOK dimensions  */
#line 3499 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 5932 "ascend/compiler/ascParse.c"
    break;

  case 346: /* dims: DIMENSIONLESS_TOK  */
#line 3503 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 5940 "ascend/compiler/ascParse.c"
    break;

  case 347: /* dims: %empty  */
#line 3507 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 5948 "ascend/compiler/ascParse.c"
    break;

  case 348: /* dimensions: '*'  */
#line 3514 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 5956 "ascend/compiler/ascParse.c"
    break;

  case 349: /* dimensions: dimexpr  */
#line 3518 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 5964 "ascend/compiler/ascParse.c"
    break;

  case 350: /* dimexpr: IDENTIFIER_TOK  */
#line 3525 "ascend/compiler/ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 5972 "ascend/compiler/ascParse.c"
    break;

  case 351: /* dimexpr: INTEGER_TOK  */
#line 3529 "ascend/compiler/ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 5980 "ascend/compiler/ascParse.c"
    break;

  case 352: /* dimexpr: dimexpr '/' dimexpr  */
#line 3533 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 5988 "ascend/compiler/ascParse.c"
    break;

  case 353: /* dimexpr: dimexpr '*' dimexpr  */
#line 3537 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 5996 "ascend/compiler/ascParse.c"
    break;

  case 354: /* dimexpr: dimexpr '^' fraction  */
#line 3541 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 6004 "ascend/compiler/ascParse.c"
    break;

  case 355: /* dimexpr: '(' dimexpr ')'  */
#line 3545 "ascend/compiler/ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 6012 "ascend/compiler/ascParse.c"
    break;

  case 356: /* fraction: optional_sign fractail  */
#line 3552 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 6020 "ascend/compiler/ascParse.c"
    break;

  case 357: /* fractail: INTEGER_TOK  */
#line 3559 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 6028 "ascend/compiler/ascParse.c"
    break;

  case 358: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 3563 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 6036 "ascend/compiler/ascParse.c"
    break;

  case 359: /* optional_sign: %empty  */
#line 3570 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6044 "ascend/compiler/ascParse.c"
    break;

  case 360: /* optional_sign: '+'  */
#line 3574 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6052 "ascend/compiler/ascParse.c"
    break;

  case 361: /* optional_sign: '-'  */
#line 3578 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 6060 "ascend/compiler/ascParse.c"
    break;

  case 362: /* expr: INTEGER_TOK  */
#line 3585 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 6068 "ascend/compiler/ascParse.c"
    break;

  case 363: /* expr: MAXINTEGER_TOK  */
#line 3589 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 6076 "ascend/compiler/ascParse.c"
    break;

  case 364: /* expr: realnumber  */
#line 3593 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 6084 "ascend/compiler/ascParse.c"
    break;

  case 365: /* expr: MAXREAL_TOK  */
#line 3597 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 6092 "ascend/compiler/ascParse.c"
    break;

  case 366: /* expr: TRUE_TOK  */
#line 3601 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 6100 "ascend/compiler/ascParse.c"
    break;

  case 367: /* expr: FALSE_TOK  */
#line 3605 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 6108 "ascend/compiler/ascParse.c"
    break;

  case 368: /* expr: ANY_TOK  */
#line 3609 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 6116 "ascend/compiler/ascParse.c"
    break;

  case 369: /* expr: SYMBOL_TOK  */
#line 3613 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 6124 "ascend/compiler/ascParse.c"
    break;

  case 370: /* expr: fname  */
#line 3617 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 6132 "ascend/compiler/ascParse.c"
    break;

  case 371: /* expr: '[' set ']'  */
#line 3621 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 6140 "ascend/compiler/ascParse.c"
    break;

  case 372: /* expr: expr '+' expr  */
#line 3625 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6149 "ascend/compiler/ascParse.c"
    break;

  case 373: /* expr: expr '-' expr  */
#line 3630 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6158 "ascend/compiler/ascParse.c"
    break;

  case 374: /* expr: expr '*' expr  */
#line 3635 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6167 "ascend/compiler/ascParse.c"
    break;

  case 375: /* expr: expr '/' expr  */
#line 3640 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6176 "ascend/compiler/ascParse.c"
    break;

  case 376: /* expr: expr '^' expr  */
#line 3645 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6185 "ascend/compiler/ascParse.c"
    break;

  case 377: /* expr: expr AND_TOK expr  */
#line 3650 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6194 "ascend/compiler/ascParse.c"
    break;

  case 378: /* expr: expr OR_TOK expr  */
#line 3655 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6203 "ascend/compiler/ascParse.c"
    break;

  case 379: /* expr: NOT_TOK expr  */
#line 3660 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 6211 "ascend/compiler/ascParse.c"
    break;

  case 380: /* expr: expr relop expr  */
#line 3664 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6220 "ascend/compiler/ascParse.c"
    break;

  case 381: /* expr: expr logrelop expr  */
#line 3669 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6229 "ascend/compiler/ascParse.c"
    break;

  case 382: /* expr: expr IN_TOK expr  */
#line 3674 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6238 "ascend/compiler/ascParse.c"
    break;

  case 383: /* expr: expr '|' expr  */
#line 3679 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6247 "ascend/compiler/ascParse.c"
    break;

  case 384: /* expr: expr SUCHTHAT_TOK expr  */
#line 3684 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6256 "ascend/compiler/ascParse.c"
    break;

  case 385: /* expr: '+' expr  */
#line 3689 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 6264 "ascend/compiler/ascParse.c"
    break;

  case 386: /* expr: '-' expr  */
#line 3693 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 6272 "ascend/compiler/ascParse.c"
    break;

  case 387: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 3697 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 6280 "ascend/compiler/ascParse.c"
    break;

  case 388: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 3701 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 6288 "ascend/compiler/ascParse.c"
    break;

  case 389: /* expr: SUM_TOK '(' set ')'  */
#line 3705 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 6299 "ascend/compiler/ascParse.c"
    break;

  case 390: /* expr: SUM_TOK '[' set ']'  */
#line 3712 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 6307 "ascend/compiler/ascParse.c"
    break;

  case 391: /* expr: PROD_TOK '(' set ')'  */
#line 3716 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 6318 "ascend/compiler/ascParse.c"
    break;

  case 392: /* expr: PROD_TOK '[' set ']'  */
#line 3723 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 6326 "ascend/compiler/ascParse.c"
    break;

  case 393: /* expr: UNION_TOK '(' set ')'  */
#line 3727 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 6337 "ascend/compiler/ascParse.c"
    break;

  case 394: /* expr: UNION_TOK '[' set ']'  */
#line 3734 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 6345 "ascend/compiler/ascParse.c"
    break;

  case 395: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 3738 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 6356 "ascend/compiler/ascParse.c"
    break;

  case 396: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 3745 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 6364 "ascend/compiler/ascParse.c"
    break;

  case 397: /* expr: CARD_TOK '(' set ')'  */
#line 3749 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 6375 "ascend/compiler/ascParse.c"
    break;

  case 398: /* expr: CARD_TOK '[' set ']'  */
#line 3756 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 6383 "ascend/compiler/ascParse.c"
    break;

  case 399: /* expr: CHOICE_TOK '(' set ')'  */
#line 3760 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 6394 "ascend/compiler/ascParse.c"
    break;

  case 400: /* expr: CHOICE_TOK '[' set ']'  */
#line 3767 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 6402 "ascend/compiler/ascParse.c"
    break;

  case 401: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 3771 "ascend/compiler/ascParse.y"
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
#line 6417 "ascend/compiler/ascParse.c"
    break;

  case 402: /* expr: '(' expr ')'  */
#line 3782 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 6425 "ascend/compiler/ascParse.c"
    break;

  case 403: /* relop: '='  */
#line 3789 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 6433 "ascend/compiler/ascParse.c"
    break;

  case 404: /* relop: '<'  */
#line 3793 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 6441 "ascend/compiler/ascParse.c"
    break;

  case 405: /* relop: '>'  */
#line 3797 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 6449 "ascend/compiler/ascParse.c"
    break;

  case 406: /* relop: LEQ_TOK  */
#line 3801 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 6457 "ascend/compiler/ascParse.c"
    break;

  case 407: /* relop: GEQ_TOK  */
#line 3805 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 6465 "ascend/compiler/ascParse.c"
    break;

  case 408: /* relop: NEQ_TOK  */
#line 3809 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 6473 "ascend/compiler/ascParse.c"
    break;

  case 409: /* logrelop: BEQ_TOK  */
#line 3816 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 6481 "ascend/compiler/ascParse.c"
    break;

  case 410: /* logrelop: BNE_TOK  */
#line 3820 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 6489 "ascend/compiler/ascParse.c"
    break;


#line 6493 "ascend/compiler/ascParse.c"

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

#line 3824 "ascend/compiler/ascParse.y"

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
