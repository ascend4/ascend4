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

/* For 'inline' notes, note on DQUOTE_TOK from scanner.l:
 * Remember that DQUOTE_TOK is a string value which is local to the
 * production that finds it. It must be copied if you want to
 * keep it.
 */

/* MS VC++ won't compiler Bison output unless we switch this */
#ifdef _MSC_VER
# define __STDC__
#endif


#line 786 "ascParse.c"

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
  YYSYMBOL_NOW_TOK = 59,                   /* NOW_TOK  */
  YYSYMBOL_LINK_TOK = 60,                  /* LINK_TOK  */
  YYSYMBOL_MAXIMIZE_TOK = 61,              /* MAXIMIZE_TOK  */
  YYSYMBOL_MAXINTEGER_TOK = 62,            /* MAXINTEGER_TOK  */
  YYSYMBOL_MAXREAL_TOK = 63,               /* MAXREAL_TOK  */
  YYSYMBOL_METHODS_TOK = 64,               /* METHODS_TOK  */
  YYSYMBOL_METHOD_TOK = 65,                /* METHOD_TOK  */
  YYSYMBOL_MINIMIZE_TOK = 66,              /* MINIMIZE_TOK  */
  YYSYMBOL_MODEL_TOK = 67,                 /* MODEL_TOK  */
  YYSYMBOL_NOT_TOK = 68,                   /* NOT_TOK  */
  YYSYMBOL_NOTES_TOK = 69,                 /* NOTES_TOK  */
  YYSYMBOL_OF_TOK = 70,                    /* OF_TOK  */
  YYSYMBOL_OPTION_TOK = 71,                /* OPTION_TOK  */
  YYSYMBOL_OR_TOK = 72,                    /* OR_TOK  */
  YYSYMBOL_OTHERWISE_TOK = 73,             /* OTHERWISE_TOK  */
  YYSYMBOL_OUTPUT_TOK = 74,                /* OUTPUT_TOK  */
  YYSYMBOL_PROD_TOK = 75,                  /* PROD_TOK  */
  YYSYMBOL_PROVIDE_TOK = 76,               /* PROVIDE_TOK  */
  YYSYMBOL_RATIO_TOK = 77,                 /* RATIO_TOK  */
  YYSYMBOL_REFINES_TOK = 78,               /* REFINES_TOK  */
  YYSYMBOL_REPLACE_TOK = 79,               /* REPLACE_TOK  */
  YYSYMBOL_REQUIRE_TOK = 80,               /* REQUIRE_TOK  */
  YYSYMBOL_RETURN_TOK = 81,                /* RETURN_TOK  */
  YYSYMBOL_RUN_TOK = 82,                   /* RUN_TOK  */
  YYSYMBOL_SATISFIED_TOK = 83,             /* SATISFIED_TOK  */
  YYSYMBOL_SELECT_TOK = 84,                /* SELECT_TOK  */
  YYSYMBOL_SIZE_TOK = 85,                  /* SIZE_TOK  */
  YYSYMBOL_SOLVE_TOK = 86,                 /* SOLVE_TOK  */
  YYSYMBOL_SOLVER_TOK = 87,                /* SOLVER_TOK  */
  YYSYMBOL_STOP_TOK = 88,                  /* STOP_TOK  */
  YYSYMBOL_SUCHTHAT_TOK = 89,              /* SUCHTHAT_TOK  */
  YYSYMBOL_SUM_TOK = 90,                   /* SUM_TOK  */
  YYSYMBOL_SWITCH_TOK = 91,                /* SWITCH_TOK  */
  YYSYMBOL_SYSTEM_TOK = 92,                /* SYSTEM_TOK  */
  YYSYMBOL_STEP_TOK = 93,                  /* STEP_TOK  */
  YYSYMBOL_STEPS_TOK = 94,                 /* STEPS_TOK  */
  YYSYMBOL_STUDY_TOK = 95,                 /* STUDY_TOK  */
  YYSYMBOL_TABLE_TOK = 96,                 /* TABLE_TOK  */
  YYSYMBOL_VALUES_TOK = 97,                /* VALUES_TOK  */
  YYSYMBOL_DATASET_TOK = 98,               /* DATASET_TOK  */
  YYSYMBOL_POSITIONAL_TOK = 99,            /* POSITIONAL_TOK  */
  YYSYMBOL_INDEX_TOK = 100,                /* INDEX_TOK  */
  YYSYMBOL_COLUMN_TOK = 101,               /* COLUMN_TOK  */
  YYSYMBOL_EOL_TOK = 102,                  /* EOL_TOK  */
  YYSYMBOL_THEN_TOK = 103,                 /* THEN_TOK  */
  YYSYMBOL_TO_TOK = 104,                   /* TO_TOK  */
  YYSYMBOL_TRUE_TOK = 105,                 /* TRUE_TOK  */
  YYSYMBOL_UNION_TOK = 106,                /* UNION_TOK  */
  YYSYMBOL_UNITS_TOK = 107,                /* UNITS_TOK  */
  YYSYMBOL_LADDER_TOK = 108,               /* LADDER_TOK  */
  YYSYMBOL_UNIVERSAL_TOK = 109,            /* UNIVERSAL_TOK  */
  YYSYMBOL_UNLINK_TOK = 110,               /* UNLINK_TOK  */
  YYSYMBOL_VARY_TOK = 111,                 /* VARY_TOK  */
  YYSYMBOL_WHEN_TOK = 112,                 /* WHEN_TOK  */
  YYSYMBOL_WHERE_TOK = 113,                /* WHERE_TOK  */
  YYSYMBOL_WHILE_TOK = 114,                /* WHILE_TOK  */
  YYSYMBOL_WILLBE_TOK = 115,               /* WILLBE_TOK  */
  YYSYMBOL_WILLBETHESAME_TOK = 116,        /* WILLBETHESAME_TOK  */
  YYSYMBOL_WILLNOTBETHESAME_TOK = 117,     /* WILLNOTBETHESAME_TOK  */
  YYSYMBOL_ASSIGN_TOK = 118,               /* ASSIGN_TOK  */
  YYSYMBOL_CASSIGN_TOK = 119,              /* CASSIGN_TOK  */
  YYSYMBOL_DBLCOLON_TOK = 120,             /* DBLCOLON_TOK  */
  YYSYMBOL_USE_TOK = 121,                  /* USE_TOK  */
  YYSYMBOL_LEQ_TOK = 122,                  /* LEQ_TOK  */
  YYSYMBOL_GEQ_TOK = 123,                  /* GEQ_TOK  */
  YYSYMBOL_NEQ_TOK = 124,                  /* NEQ_TOK  */
  YYSYMBOL_DOTDOT_TOK = 125,               /* DOTDOT_TOK  */
  YYSYMBOL_WITH_TOK = 126,                 /* WITH_TOK  */
  YYSYMBOL_VALUE_TOK = 127,                /* VALUE_TOK  */
  YYSYMBOL_WITH_VALUE_T = 128,             /* WITH_VALUE_T  */
  YYSYMBOL_REAL_TOK = 129,                 /* REAL_TOK  */
  YYSYMBOL_INTEGER_TOK = 130,              /* INTEGER_TOK  */
  YYSYMBOL_IDENTIFIER_TOK = 131,           /* IDENTIFIER_TOK  */
  YYSYMBOL_BRACEDTEXT_TOK = 132,           /* BRACEDTEXT_TOK  */
  YYSYMBOL_SYMBOL_TOK = 133,               /* SYMBOL_TOK  */
  YYSYMBOL_DQUOTE_TOK = 134,               /* DQUOTE_TOK  */
  YYSYMBOL_135_ = 135,                     /* ','  */
  YYSYMBOL_136_ = 136,                     /* '|'  */
  YYSYMBOL_137_ = 137,                     /* '<'  */
  YYSYMBOL_138_ = 138,                     /* '='  */
  YYSYMBOL_139_ = 139,                     /* '>'  */
  YYSYMBOL_140_ = 140,                     /* '+'  */
  YYSYMBOL_141_ = 141,                     /* '-'  */
  YYSYMBOL_142_ = 142,                     /* '/'  */
  YYSYMBOL_143_ = 143,                     /* '*'  */
  YYSYMBOL_UMINUS_TOK = 144,               /* UMINUS_TOK  */
  YYSYMBOL_UPLUS_TOK = 145,                /* UPLUS_TOK  */
  YYSYMBOL_146_ = 146,                     /* '^'  */
  YYSYMBOL_147_ = 147,                     /* ';'  */
  YYSYMBOL_148_ = 148,                     /* '('  */
  YYSYMBOL_149_ = 149,                     /* ')'  */
  YYSYMBOL_150_ = 150,                     /* ':'  */
  YYSYMBOL_151_ = 151,                     /* '['  */
  YYSYMBOL_152_ = 152,                     /* ']'  */
  YYSYMBOL_153_ = 153,                     /* '.'  */
  YYSYMBOL_YYACCEPT = 154,                 /* $accept  */
  YYSYMBOL_definitions = 155,              /* definitions  */
  YYSYMBOL_definition = 156,               /* definition  */
  YYSYMBOL_global_def = 157,               /* global_def  */
  YYSYMBOL_require_file = 158,             /* require_file  */
  YYSYMBOL_provide_module = 159,           /* provide_module  */
  YYSYMBOL_import = 160,                   /* import  */
  YYSYMBOL_add_notes_def = 161,            /* add_notes_def  */
  YYSYMBOL_add_notes_head = 162,           /* add_notes_head  */
  YYSYMBOL_add_method_def = 163,           /* add_method_def  */
  YYSYMBOL_add_method_head = 164,          /* add_method_head  */
  YYSYMBOL_replace_method_def = 165,       /* replace_method_def  */
  YYSYMBOL_replace_method_head = 166,      /* replace_method_head  */
  YYSYMBOL_atom_def = 167,                 /* atom_def  */
  YYSYMBOL_atom_head = 168,                /* atom_head  */
  YYSYMBOL_atom_id = 169,                  /* atom_id  */
  YYSYMBOL_default_val = 170,              /* default_val  */
  YYSYMBOL_constant_def = 171,             /* constant_def  */
  YYSYMBOL_constant_head = 172,            /* constant_head  */
  YYSYMBOL_constant_dims = 173,            /* constant_dims  */
  YYSYMBOL_constant_val = 174,             /* constant_val  */
  YYSYMBOL_model_def = 175,                /* model_def  */
  YYSYMBOL_model_head = 176,               /* model_head  */
  YYSYMBOL_model_id = 177,                 /* model_id  */
  YYSYMBOL_optional_model_parameters = 178, /* optional_model_parameters  */
  YYSYMBOL_optional_parameter_wheres = 179, /* optional_parameter_wheres  */
  YYSYMBOL_optional_parameter_reduction = 180, /* optional_parameter_reduction  */
  YYSYMBOL_universal = 181,                /* universal  */
  YYSYMBOL_definition_def = 182,           /* definition_def  */
  YYSYMBOL_definition_id = 183,            /* definition_id  */
  YYSYMBOL_units_def = 184,                /* units_def  */
  YYSYMBOL_units_statement = 185,          /* units_statement  */
  YYSYMBOL_optional_ladder_end = 186,      /* optional_ladder_end  */
  YYSYMBOL_table_statement = 187,          /* table_statement  */
  YYSYMBOL_table_begin = 188,              /* table_begin  */
  YYSYMBOL_table_mode_on = 189,            /* table_mode_on  */
  YYSYMBOL_table_mode_off = 190,           /* table_mode_off  */
  YYSYMBOL_table_options = 191,            /* table_options  */
  YYSYMBOL_table_decl_opt = 192,           /* table_decl_opt  */
  YYSYMBOL_table_option = 193,             /* table_option  */
  YYSYMBOL_table_body = 194,               /* table_body  */
  YYSYMBOL_table_body_item = 195,          /* table_body_item  */
  YYSYMBOL_table_scalar = 196,             /* table_scalar  */
  YYSYMBOL_values_statement = 197,         /* values_statement  */
  YYSYMBOL_values_default_opt = 198,       /* values_default_opt  */
  YYSYMBOL_values_entries = 199,           /* values_entries  */
  YYSYMBOL_values_entry = 200,             /* values_entry  */
  YYSYMBOL_values_key_list = 201,          /* values_key_list  */
  YYSYMBOL_values_key = 202,               /* values_key  */
  YYSYMBOL_dataset_statement = 203,        /* dataset_statement  */
  YYSYMBOL_204_1 = 204,                    /* $@1  */
  YYSYMBOL_dataset_items = 205,            /* dataset_items  */
  YYSYMBOL_dataset_item = 206,             /* dataset_item  */
  YYSYMBOL_dataset_index_item = 207,       /* dataset_index_item  */
  YYSYMBOL_dataset_map_item = 208,         /* dataset_map_item  */
  YYSYMBOL_dataset_column_selector = 209,  /* dataset_column_selector  */
  YYSYMBOL_dataset_column_ref = 210,       /* dataset_column_ref  */
  YYSYMBOL_dataset_target = 211,           /* dataset_target  */
  YYSYMBOL_dataset_units_opt = 212,        /* dataset_units_opt  */
  YYSYMBOL_dataset_type_opt = 213,         /* dataset_type_opt  */
  YYSYMBOL_dataset_type_req = 214,         /* dataset_type_req  */
  YYSYMBOL_unitdeflist = 215,              /* unitdeflist  */
  YYSYMBOL_unitdef = 216,                  /* unitdef  */
  YYSYMBOL_unitladderitemlist = 217,       /* unitladderitemlist  */
  YYSYMBOL_unitladderitem = 218,           /* unitladderitem  */
  YYSYMBOL_methods = 219,                  /* methods  */
  YYSYMBOL_220_2 = 220,                    /* $@2  */
  YYSYMBOL_proclist = 221,                 /* proclist  */
  YYSYMBOL_proclistf = 222,                /* proclistf  */
  YYSYMBOL_procedure = 223,                /* procedure  */
  YYSYMBOL_procedure_id = 224,             /* procedure_id  */
  YYSYMBOL_fstatements = 225,              /* fstatements  */
  YYSYMBOL_statements = 226,               /* statements  */
  YYSYMBOL_statement = 227,                /* statement  */
  YYSYMBOL_complex_statement = 228,        /* complex_statement  */
  YYSYMBOL_isa_statement = 229,            /* isa_statement  */
  YYSYMBOL_willbe_statement = 230,         /* willbe_statement  */
  YYSYMBOL_aliases_statement = 231,        /* aliases_statement  */
  YYSYMBOL_optional_set_values = 232,      /* optional_set_values  */
  YYSYMBOL_is_statement = 233,             /* is_statement  */
  YYSYMBOL_isrefinedto_statement = 234,    /* isrefinedto_statement  */
  YYSYMBOL_call_identifier = 235,          /* call_identifier  */
  YYSYMBOL_type_identifier = 236,          /* type_identifier  */
  YYSYMBOL_optional_method = 237,          /* optional_method  */
  YYSYMBOL_optional_of = 238,              /* optional_of  */
  YYSYMBOL_optional_with_value = 239,      /* optional_with_value  */
  YYSYMBOL_arealike_statement = 240,       /* arealike_statement  */
  YYSYMBOL_link_statement = 241,           /* link_statement  */
  YYSYMBOL_unlink_statement = 242,         /* unlink_statement  */
  YYSYMBOL_der_statement = 243,            /* der_statement  */
  YYSYMBOL_independent_statement = 244,    /* independent_statement  */
  YYSYMBOL_arethesame_statement = 245,     /* arethesame_statement  */
  YYSYMBOL_willbethesame_statement = 246,  /* willbethesame_statement  */
  YYSYMBOL_willnotbethesame_statement = 247, /* willnotbethesame_statement  */
  YYSYMBOL_assignment_statement = 248,     /* assignment_statement  */
  YYSYMBOL_relation_statement = 249,       /* relation_statement  */
  YYSYMBOL_relation = 250,                 /* relation  */
  YYSYMBOL_blackbox_statement = 251,       /* blackbox_statement  */
  YYSYMBOL_input_args = 252,               /* input_args  */
  YYSYMBOL_output_args = 253,              /* output_args  */
  YYSYMBOL_data_args = 254,                /* data_args  */
  YYSYMBOL_for_statement = 255,            /* for_statement  */
  YYSYMBOL_optional_direction = 256,       /* optional_direction  */
  YYSYMBOL_forexprend = 257,               /* forexprend  */
  YYSYMBOL_run_statement = 258,            /* run_statement  */
  YYSYMBOL_fix_statement = 259,            /* fix_statement  */
  YYSYMBOL_fix_and_assign_statement = 260, /* fix_and_assign_statement  */
  YYSYMBOL_free_statement = 261,           /* free_statement  */
  YYSYMBOL_solver_statement = 262,         /* solver_statement  */
  YYSYMBOL_option_statement = 263,         /* option_statement  */
  YYSYMBOL_solve_statement = 264,          /* solve_statement  */
  YYSYMBOL_study_statement = 265,          /* study_statement  */
  YYSYMBOL_266_3 = 266,                    /* $@3  */
  YYSYMBOL_study_vary_opt = 267,           /* study_vary_opt  */
  YYSYMBOL_study_distribution_opt = 268,   /* study_distribution_opt  */
  YYSYMBOL_study_run_opt = 269,            /* study_run_opt  */
  YYSYMBOL_study_file_opt = 270,           /* study_file_opt  */
  YYSYMBOL_study_now_opt = 271,            /* study_now_opt  */
  YYSYMBOL_delete_statement = 272,         /* delete_statement  */
  YYSYMBOL_external_statement = 273,       /* external_statement  */
  YYSYMBOL_call_statement = 274,           /* call_statement  */
  YYSYMBOL_assert_statement = 275,         /* assert_statement  */
  YYSYMBOL_if_statement = 276,             /* if_statement  */
  YYSYMBOL_while_statement = 277,          /* while_statement  */
  YYSYMBOL_optional_else = 278,            /* optional_else  */
  YYSYMBOL_when_statement = 279,           /* when_statement  */
  YYSYMBOL_whenlist = 280,                 /* whenlist  */
  YYSYMBOL_whenlistf = 281,                /* whenlistf  */
  YYSYMBOL_flow_statement = 282,           /* flow_statement  */
  YYSYMBOL_use_statement = 283,            /* use_statement  */
  YYSYMBOL_select_statement = 284,         /* select_statement  */
  YYSYMBOL_selectlist = 285,               /* selectlist  */
  YYSYMBOL_selectlistf = 286,              /* selectlistf  */
  YYSYMBOL_switch_statement = 287,         /* switch_statement  */
  YYSYMBOL_switchlist = 288,               /* switchlist  */
  YYSYMBOL_switchlistf = 289,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 290,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 291,          /* notes_statement  */
  YYSYMBOL_notes_body = 292,               /* notes_body  */
  YYSYMBOL_noteslist = 293,                /* noteslist  */
  YYSYMBOL_fvarlist = 294,                 /* fvarlist  */
  YYSYMBOL_varlist = 295,                  /* varlist  */
  YYSYMBOL_fname = 296,                    /* fname  */
  YYSYMBOL_name = 297,                     /* name  */
  YYSYMBOL_end = 298,                      /* end  */
  YYSYMBOL_optional_bracedtext = 299,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 300,           /* optional_notes  */
  YYSYMBOL_set = 301,                      /* set  */
  YYSYMBOL_setexprlist = 302,              /* setexprlist  */
  YYSYMBOL_number = 303,                   /* number  */
  YYSYMBOL_realnumber = 304,               /* realnumber  */
  YYSYMBOL_opunits = 305,                  /* opunits  */
  YYSYMBOL_dims = 306,                     /* dims  */
  YYSYMBOL_dimensions = 307,               /* dimensions  */
  YYSYMBOL_dimexpr = 308,                  /* dimexpr  */
  YYSYMBOL_fraction = 309,                 /* fraction  */
  YYSYMBOL_fractail = 310,                 /* fractail  */
  YYSYMBOL_optional_sign = 311,            /* optional_sign  */
  YYSYMBOL_expr = 312,                     /* expr  */
  YYSYMBOL_relop = 313,                    /* relop  */
  YYSYMBOL_logrelop = 314                  /* logrelop  */
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
#define YYLAST   1496

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  154
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  161
/* YYNRULES -- Number of rules.  */
#define YYNRULES  413
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  807

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   391


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
     148,   149,   143,   140,   135,   141,   153,   142,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   150,   147,
     137,   138,   139,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   151,     2,   152,   146,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   136,     2,     2,     2,     2,     2,
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
     144,   145
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   875,   875,   877,   881,   882,   883,   884,   885,   886,
     887,   888,   889,   890,   892,   893,   894,   905,   950,   954,
     959,   967,   971,   976,   984,   993,  1005,  1036,  1044,  1067,
    1080,  1088,  1109,  1121,  1129,  1182,  1193,  1202,  1208,  1214,
    1222,  1230,  1241,  1281,  1315,  1320,  1326,  1330,  1352,  1358,
    1364,  1372,  1380,  1391,  1432,  1441,  1454,  1463,  1466,  1474,
    1477,  1485,  1488,  1539,  1542,  1549,  1579,  1588,  1593,  1610,
    1636,  1639,  1646,  1667,  1680,  1687,  1694,  1699,  1701,  1706,
    1718,  1731,  1735,  1742,  1751,  1753,  1757,  1758,  1762,  1766,
    1770,  1774,  1778,  1782,  1789,  1793,  1797,  1801,  1805,  1812,
    1819,  1821,  1828,  1829,  1833,  1840,  1841,  1845,  1846,  1847,
    1848,  1853,  1852,  1862,  1864,  1865,  1875,  1876,  1880,  1887,
    1891,  1898,  1902,  1909,  1913,  1920,  1925,  1934,  1937,  1945,
    1948,  1955,  1962,  1965,  1973,  1981,  1984,  1992,  1997,  2007,
    2011,  2010,  2024,  2032,  2035,  2060,  2074,  2083,  2091,  2094,
    2160,  2168,  2176,  2177,  2178,  2179,  2180,  2181,  2182,  2183,
    2184,  2185,  2186,  2187,  2188,  2189,  2190,  2192,  2193,  2194,
    2195,  2196,  2197,  2198,  2199,  2200,  2201,  2202,  2203,  2204,
    2205,  2206,  2207,  2208,  2209,  2210,  2211,  2212,  2213,  2214,
    2215,  2216,  2217,  2221,  2226,  2267,  2296,  2300,  2351,  2354,
    2361,  2373,  2400,  2405,  2413,  2418,  2427,  2430,  2438,  2441,
    2449,  2452,  2459,  2466,  2470,  2474,  2481,  2485,  2492,  2501,
    2510,  2517,  2524,  2531,  2535,  2542,  2555,  2572,  2582,  2590,
    2601,  2622,  2629,  2637,  2640,  2677,  2699,  2702,  2706,  2713,
    2717,  2721,  2725,  2732,  2736,  2743,  2751,  2765,  2772,  2780,
    2788,  2793,  2801,  2800,  2817,  2819,  2827,  2836,  2849,  2851,
    2855,  2863,  2865,  2873,  2875,  2883,  2885,  2892,  2899,  2910,
    2921,  2927,  2937,  2947,  2950,  2957,  2968,  2980,  2987,  2997,
    3004,  3008,  3012,  3016,  3023,  3027,  3031,  3035,  3039,  3046,
    3053,  3064,  3074,  3081,  3085,  3089,  3093,  3100,  3111,  3121,
    3128,  3132,  3136,  3140,  3147,  3157,  3194,  3203,  3218,  3223,
    3232,  3243,  3247,  3252,  3265,  3285,  3289,  3294,  3309,  3314,
    3319,  3324,  3329,  3334,  3339,  3344,  3349,  3354,  3359,  3364,
    3369,  3374,  3383,  3386,  3394,  3397,  3404,  3409,  3415,  3419,
    3423,  3428,  3436,  3443,  3452,  3457,  3482,  3487,  3510,  3514,
    3519,  3525,  3529,  3536,  3540,  3544,  3548,  3552,  3556,  3563,
    3570,  3574,  3582,  3585,  3589,  3596,  3600,  3604,  3608,  3612,
    3616,  3620,  3624,  3628,  3632,  3636,  3641,  3646,  3651,  3656,
    3661,  3666,  3671,  3675,  3680,  3685,  3690,  3695,  3700,  3704,
    3708,  3712,  3716,  3723,  3727,  3734,  3738,  3745,  3749,  3756,
    3760,  3767,  3771,  3778,  3782,  3793,  3800,  3804,  3808,  3812,
    3816,  3820,  3827,  3831
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
  "LOG_TOK", "NOW_TOK", "LINK_TOK", "MAXIMIZE_TOK", "MAXINTEGER_TOK",
  "MAXREAL_TOK", "METHODS_TOK", "METHOD_TOK", "MINIMIZE_TOK", "MODEL_TOK",
  "NOT_TOK", "NOTES_TOK", "OF_TOK", "OPTION_TOK", "OR_TOK",
  "OTHERWISE_TOK", "OUTPUT_TOK", "PROD_TOK", "PROVIDE_TOK", "RATIO_TOK",
  "REFINES_TOK", "REPLACE_TOK", "REQUIRE_TOK", "RETURN_TOK", "RUN_TOK",
  "SATISFIED_TOK", "SELECT_TOK", "SIZE_TOK", "SOLVE_TOK", "SOLVER_TOK",
  "STOP_TOK", "SUCHTHAT_TOK", "SUM_TOK", "SWITCH_TOK", "SYSTEM_TOK",
  "STEP_TOK", "STEPS_TOK", "STUDY_TOK", "TABLE_TOK", "VALUES_TOK",
  "DATASET_TOK", "POSITIONAL_TOK", "INDEX_TOK", "COLUMN_TOK", "EOL_TOK",
  "THEN_TOK", "TO_TOK", "TRUE_TOK", "UNION_TOK", "UNITS_TOK", "LADDER_TOK",
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
  "study_statement", "$@3", "study_vary_opt", "study_distribution_opt",
  "study_run_opt", "study_file_opt", "study_now_opt", "delete_statement",
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

#define YYPACT_NINF (-714)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-312)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -714,  1387,  -714,  -714,    82,  -105,   -98,   114,   128,   -23,
     133,   -44,  -714,  -714,  -714,  -714,  -714,  -714,  -714,    15,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,    16,  -714,  -714,
    -714,    45,   150,   154,  -714,  -714,   213,   113,  -714,   159,
      86,   166,   165,    91,  -714,    -2,   134,   -10,   252,   250,
     252,   190,   193,   194,  -714,   251,  -714,  -714,   183,   268,
     740,  -714,    -5,   202,   252,   201,  -714,  -714,  -714,   837,
     205,    -4,  -714,  -714,     2,   421,   199,  -714,  -714,   134,
     208,    99,  -714,  -106,   134,   196,   198,   210,  -714,   200,
     206,  -714,   271,  -714,   268,   215,   268,  -714,   243,  -714,
     252,   211,  -714,   837,  -714,   224,    64,   118,  -714,  -714,
     265,   214,   228,  -714,  -714,   134,   230,   134,   837,   134,
     120,   216,   837,  -714,  -714,   837,   837,    15,   232,   122,
    -714,   134,   217,   -97,   134,   237,   238,   139,   -92,  -714,
     134,   134,   245,  -714,   162,   221,   -83,   837,   134,   246,
     247,   236,  -714,   837,   837,   837,   837,  -714,  -714,  -714,
    -714,   240,   241,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,    54,   296,  -714,  1243,   318,   244,   325,   249,
     253,  -714,   254,   258,  1046,  -714,   327,   257,   259,   260,
     290,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,   267,   273,  -714,   134,  -714,  -714,
    -714,   134,  -714,  -714,  -714,  -714,  -714,   277,   252,   152,
     252,   274,   261,   -63,  -714,   282,  -714,  1243,   276,  -714,
     837,   837,   837,   837,   252,  -714,   134,   278,  -714,  -714,
     110,   382,  -714,  1101,  -714,   837,   837,    11,  1243,  1243,
    -714,   -10,   837,   837,   837,   312,   134,   134,     8,  -714,
    -714,  -714,  -714,   837,   837,   134,    23,   134,  -714,   407,
     392,   837,   837,   185,   134,    26,  1143,  -714,  -714,  -714,
    -714,   837,   289,   289,   873,   285,  -714,  -714,   -57,  -714,
    -714,   307,   308,   307,   307,  -714,  -714,   837,   837,   551,
     837,  -714,  -714,   837,   837,   837,  -714,  -714,  -714,   837,
    -714,  -714,  -714,   837,   837,   837,   837,   837,   837,   837,
     293,  -714,   311,   297,  -714,  -714,  -714,   837,   837,   298,
    -714,   317,  -714,  -714,  -714,   303,  -714,  -714,   252,     0,
     304,    63,  -714,   417,   305,  -714,  -714,   324,  -714,  -714,
    -714,   837,   310,   315,   313,   319,  -714,   321,   134,   837,
    -714,   326,   322,   329,   338,   341,  -714,  1243,   330,   328,
     134,    30,   332,   837,   306,   252,    62,   333,   335,   339,
     837,   334,   252,    94,   346,   406,   837,   342,   360,   347,
     345,   363,   364,   351,   837,   352,   252,   115,  -714,   901,
    -714,  -714,   134,  -714,   353,   433,   433,  -714,   433,  1243,
    1243,   -42,   356,  -714,   279,   279,   279,  1287,  1287,    32,
      32,   289,   289,   289,    81,   508,  -714,  -714,  -714,  1188,
    1243,  -714,  -714,  -714,   359,    63,  -714,   375,   389,  -714,
    -714,  -714,  -714,    28,  -714,   104,    40,   362,  -714,   361,
     366,   368,  -714,  -714,  -714,  -714,  -714,   369,  1021,   478,
    -714,  -714,   386,   134,   134,  -714,  -714,  -714,   145,  -714,
       8,   370,  -714,  -714,   837,   372,  -714,  -714,    23,   374,
    -714,  -714,   837,   376,   134,   443,   307,  -714,  1243,   123,
     380,  -714,  -714,   134,   134,    26,   381,  -714,  -714,   837,
     387,   252,  -714,   383,   837,   399,   408,  -714,   408,   134,
      26,   837,   837,  -714,  -714,  -714,    56,   404,    67,    28,
      28,   141,  -714,  -714,  -714,  -714,  -714,   172,  -714,  -714,
    -714,   393,  -714,  -714,  -714,  -714,   117,  -714,   252,   409,
     390,   394,   247,   398,   252,  -714,  -714,   391,  -714,   252,
    -714,  -714,   400,  -714,   500,   418,   489,   433,    -9,  -714,
    -714,  -714,  -714,    33,   411,   176,  -714,  -714,   402,   405,
     252,  -714,  -714,   403,  -714,  -714,   447,   413,  -714,   837,
    -714,  -714,   415,   252,   419,   420,   -72,  1243,  -714,  -714,
    -714,   172,   424,  -714,   426,   426,  -714,   -88,   247,  -714,
    -714,   416,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
     134,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,   837,  -714,  -714,   530,  -714,   837,  -714,   442,
    -714,  -714,   479,   428,  -714,   123,   837,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,   134,  -714,  1243,    26,  -714,   134,
     528,  -714,  -714,  -714,   448,  -714,  -714,   252,   430,  -714,
    -714,  1215,   449,  -714,  1243,  -714,   242,  -714,  -714,  -714,
    1243,    12,  -714,   527,   252,   435,   434,  -714,   444,  -714,
    -714,   837,  -714,   553,   -16,   438,   491,   459,   440,   445,
    -714,  -714,   -33,   462,  -714,   134,   450,   522,   467,   959,
     502,   504,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,   559,   134,
    -714,   -81,   474,   134,   564,   538,   460,  -714,  -714,   466,
     837,   837,   481,  -714,  -714,   517,   457,   189,  -714,  -714,
     490,  -714,  -714,   469,   -81,   492,   600,  -714,  1243,  1243,
     234,  -714,  -714,   189,  -714,  -714,  -714,   571,  -714,   490,
     499,  -714,  -714,  -714,  -714,   575,   505,  -714,  -714,   487,
    -714,   506,  -714,   837,  -714,   494,  -714
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    16,     0,     0,     0,     0,     0,     0,
       0,   132,    64,     3,    15,     4,     5,     6,     9,     0,
       7,   143,     8,   143,    11,    10,    12,     0,    13,   148,
      14,     0,     0,     0,    66,   148,     0,     0,   315,     0,
      23,     0,     0,    20,   135,     0,     0,     0,     0,   142,
       0,     0,     0,     0,   148,     0,    42,   148,    57,   139,
       0,    67,     0,     0,     0,     0,    25,    21,    22,   337,
       0,     0,    18,    19,     0,   331,     0,   133,    68,   306,
       0,   310,   311,   334,     0,     0,     0,     0,   144,     0,
       0,    36,     0,    56,   139,     0,   139,   148,    59,   140,
       0,     0,   371,     0,   284,     0,     0,     0,   148,   285,
       0,     0,     0,   370,   286,     0,     0,     0,     0,     0,
       0,     0,     0,   366,   368,     0,     0,     0,     0,     0,
     287,     0,     0,     0,   250,     0,   332,     0,     0,   252,
       0,     0,     0,   369,     0,     0,     0,     0,     0,   346,
     365,   315,   372,     0,     0,     0,   337,   189,   190,   191,
     192,     0,     0,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   225,   167,
     170,   171,   172,   193,   173,   174,   176,   175,   177,   178,
     169,   168,   179,   180,   181,   182,   184,   183,   185,   186,
     187,   188,     0,   373,   367,   227,     0,     0,   206,     0,
       0,   373,     0,   336,   338,   316,     0,     0,   138,     0,
      70,   318,   319,   327,   320,   321,   322,   323,   324,   325,
     326,   328,   329,   330,     0,     0,   308,     0,   313,   335,
     314,   307,    26,    28,   146,   148,    31,     0,     0,   350,
       0,     0,     0,     0,   143,     0,   151,   270,   202,   269,
     337,   337,   337,   337,     0,   267,     0,     0,   246,   245,
     311,     0,   247,     0,   219,   337,   337,     0,   229,   228,
     382,     0,     0,   337,   337,   243,     0,     0,     0,   251,
     248,   333,   288,   337,   337,     0,     0,     0,    74,   100,
       0,   337,   337,     0,     0,     0,     0,   289,   347,   344,
     345,     0,   388,   389,     0,     0,   149,   150,     0,   212,
     220,     0,     0,     0,     0,   221,   222,     0,     0,     0,
       0,   412,   413,     0,     0,     0,   409,   410,   411,     0,
     407,   406,   408,     0,     0,     0,     0,     0,     0,     0,
       0,    29,     0,     0,    17,    24,   317,     0,     0,     0,
      32,     0,   136,    71,    69,     0,   309,   312,     0,    46,
       0,     0,   349,    37,     0,    58,   148,     0,    54,   141,
      65,   337,     0,     0,     0,     0,   304,     0,     0,     0,
     148,     0,     0,     0,     0,     0,   305,   249,     0,     0,
       0,     0,     0,   337,     0,     0,   292,     0,     0,     0,
     337,     0,     0,   299,   254,    79,     0,     0,     0,     0,
       0,     0,     0,     0,   337,     0,     0,   279,   148,     0,
     405,   374,     0,   196,   204,   208,   208,   201,   208,   223,
     224,     0,   315,   226,   380,   385,   381,   387,   386,   375,
     376,   378,   377,   379,   383,   384,    30,   207,    27,   340,
     339,    33,   137,   134,     0,     0,    45,     0,    48,    34,
     354,   353,   351,     0,   348,   352,   362,     0,    53,     0,
      61,     0,   400,   401,   402,   403,   218,     0,   236,   273,
     398,   399,     0,     0,     0,   394,   395,   244,     0,   391,
       0,     0,   148,   290,   337,     0,   392,   393,     0,     0,
     148,   297,   337,     0,     0,   261,     0,    77,   101,     0,
       0,   396,   397,     0,     0,     0,     0,   148,   275,   337,
       0,     0,   404,     0,   337,     0,   210,   200,   210,     0,
       0,     0,     0,   145,    44,    47,   362,   334,     0,     0,
       0,   362,    39,    40,    41,   363,   364,     0,    35,    60,
     148,     0,   203,   268,   238,   237,     0,   148,     0,     0,
       0,     0,     0,     0,     0,   148,   294,     0,   148,     0,
     148,   301,     0,   148,     0,     0,   265,   208,     0,   110,
     109,   107,   108,     0,     0,     0,   105,   111,     0,     0,
       0,   148,   281,     0,   148,   272,     0,     0,   209,     0,
     194,   195,     0,     0,     0,     0,   373,   341,    51,    50,
      52,     0,     0,   358,   355,   356,   357,     0,   342,    38,
     343,     0,    55,   241,   239,   242,   240,   148,   274,   271,
       0,   214,   215,   390,   291,   293,   148,   296,   298,   300,
     148,   303,     0,   262,   266,   263,    80,     0,    81,     0,
      75,    78,     0,     0,   102,     0,     0,   113,   216,   217,
     277,   280,   148,   283,     0,   205,   211,     0,   276,     0,
       0,    49,    43,   360,     0,   359,    62,     0,     0,   295,
     302,     0,     0,   253,    82,    83,     0,    99,   103,   106,
     104,     0,   282,     0,     0,   233,     0,   231,     0,   235,
     213,     0,   264,     0,     0,     0,     0,     0,     0,     0,
     116,   117,     0,     0,   278,     0,     0,     0,     0,     0,
       0,     0,    93,    97,    96,    94,    98,    95,    89,    88,
      90,    91,    92,    87,    85,    86,   115,   112,     0,     0,
     114,     0,     0,     0,     0,     0,     0,   230,   232,     0,
       0,     0,     0,    76,    76,     0,     0,     0,   123,   124,
     127,   122,   131,     0,     0,     0,     0,   361,   257,   256,
     258,    73,    72,     0,   125,   121,   128,   129,   126,   127,
     198,   234,   259,   260,   255,     0,     0,   119,   120,     0,
     197,     0,   130,   337,   118,     0,   199
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,   578,  -714,  -714,  -714,  -714,  -125,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,    47,  -714,   -21,  -714,
    -714,  -714,  -714,  -714,  -714,  -122,  -713,  -714,  -134,  -714,
    -714,  -714,  -714,  -714,  -714,    60,  -714,   -19,  -714,  -714,
    -714,    93,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -312,  -714,  -430,   126,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,   543,  -714,   336,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,  -714,
    -714,  -714,  -714,  -714,  -714,  -714,  -524,  -714,  -714,  -714,
    -714,   167,  -714,  -714,   158,  -714,  -714,  -714,   542,   586,
      22,  -714,   -46,   320,   -45,  -714,   125,   -76,  -714,    53,
    -491,  -714,  -714,   212,  -429,  -714,  -714,  -374,   -50,  -714,
    -714
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    54,    55,   477,    25,    56,   468,
     547,    26,    57,    58,    98,   253,   561,    27,    28,    29,
      30,    31,   364,   158,   415,   696,   781,   588,   517,   661,
     714,   744,   745,   159,   417,   593,   594,   595,   596,   160,
     667,   701,   719,   720,   721,   770,   771,   722,   787,   797,
     754,    45,    77,    74,   219,   100,   254,    48,    49,    88,
      89,    59,    60,   161,   162,   163,   164,   165,   800,   166,
     167,   259,   435,   353,   536,   610,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   614,   705,
     726,   180,   566,   637,   181,   182,   183,   184,   185,   186,
     187,   188,   297,   515,   794,   586,   693,   655,   189,   190,
     191,   192,   193,   194,   568,   195,   426,   427,   196,   197,
     198,   405,   406,   199,   412,   413,   200,   201,    47,    79,
      80,    81,   211,    83,    78,   292,   240,   212,   213,   629,
     204,   309,   373,   474,   475,   626,   685,   557,   214,   348,
     349
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      82,   600,    85,    86,    50,    90,   537,   573,   538,   751,
     205,   437,   438,   715,   203,   377,   613,   657,   731,   209,
     767,   752,   206,   216,    75,   403,    34,    51,   239,   220,
     465,   466,    75,    82,    38,   238,    75,    52,    82,    38,
     410,    41,   683,   424,   548,    69,   716,    70,    38,    35,
     768,   287,   769,   257,   785,   255,   295,   393,   318,  -311,
     684,   319,   320,  -311,    44,   304,   630,   662,   273,   270,
     795,    82,   278,    82,    38,   279,   280,   552,  -311,   504,
     315,   404,   202,    53,   378,   285,   732,    82,   289,    38,
     658,   432,    82,   618,   298,   299,   411,   306,   659,   425,
      82,   235,   307,   312,   313,   314,   539,   467,   321,   322,
     323,   512,   717,   733,   734,   735,   736,   737,   753,   738,
     624,   625,   739,    84,   740,   741,   207,   217,    64,    76,
     630,   742,   529,   218,   743,   505,   633,   269,   660,   272,
     634,   274,    38,   718,   394,   553,    32,    94,    46,   635,
      96,    33,   636,   704,   248,   288,   250,   656,   470,   471,
     296,   619,   589,   590,   591,   498,   592,   513,   305,   324,
     325,   326,   621,   554,   345,   346,   473,   627,   347,   499,
     555,   556,   371,   372,   382,   383,   384,   385,   530,   620,
     251,   367,    61,   470,   471,    82,   555,   556,    62,   391,
     392,   264,    63,   370,   587,   374,   472,   398,   399,   549,
     550,   473,   260,   551,    71,   261,   623,   407,   408,   386,
      82,   343,   344,   345,   346,   419,   420,   347,   327,   328,
      38,   395,   397,    68,   237,   379,   396,    69,    73,    70,
     401,    82,    69,   713,    70,    36,   549,   550,    37,    82,
     551,    82,   589,   590,   591,    65,   592,   422,    82,    38,
      66,   429,    39,   235,    38,    38,   262,    42,   275,   263,
     283,   276,   433,   284,   149,   572,   -84,   439,   440,   205,
     444,   555,   556,   445,   446,   447,    75,   293,   387,   448,
     294,   792,   793,   449,   450,   451,   452,   453,   454,   455,
    -311,   149,   628,  -311,  -311,   481,    67,   459,   460,   402,
     301,   665,    72,   302,   666,    87,    38,   409,   421,   414,
     768,    91,   769,   464,    92,    93,   423,   501,    40,    95,
      43,    97,    99,   208,   509,   210,   215,   234,   368,   488,
     236,   244,    82,   242,   -84,   243,   249,   245,   526,   247,
    -311,  -311,  -311,   246,   497,   258,   252,   265,   256,   267,
     503,   271,   266,   282,   277,   286,   518,   511,   290,   303,
     291,   -84,   -84,   -84,   -84,   -84,   300,   -84,   308,   310,
     -84,   528,   -84,   -84,   311,   350,    82,   316,   317,   -84,
     352,   351,   -84,   357,   359,    82,   354,   361,   363,   365,
     355,   336,   337,   338,   360,   366,   356,   362,   369,   376,
     487,  -311,  -311,  -311,   327,   328,   340,   341,   342,   343,
     344,   345,   346,   375,   381,   347,   388,  -311,   577,   380,
     389,  -311,   400,   416,   418,   347,   582,   431,   434,   436,
     456,   221,   457,   476,   458,   461,   329,    82,    82,   462,
     463,   469,   478,   603,   533,   480,   502,   514,   607,   482,
     516,   222,   484,   540,   492,   223,   224,   483,   584,   479,
     486,   485,   225,   493,   491,   490,   494,    82,    82,   495,
     496,   500,   506,   489,   510,   226,   605,   507,   508,   519,
     227,   429,   617,    82,   520,   616,   521,   522,   523,   524,
     525,   534,   527,   535,   541,   228,   543,   545,   546,   558,
     559,   567,   229,   330,   560,   570,   571,   562,   563,   569,
     575,   531,   578,   639,   580,   585,   583,   597,   230,   644,
     608,   601,   606,   231,   648,   232,   609,   604,   239,   641,
     632,   646,   652,   642,   640,   598,   599,   643,   654,   653,
     650,   668,   233,   672,   669,   670,   333,   102,   664,   676,
     674,   612,   675,   615,   677,   686,   679,   106,   678,   107,
     680,   682,   551,   692,   695,   698,   697,   707,   708,   710,
     334,   723,   725,   712,   727,   746,   728,   730,   113,   747,
     748,   749,   750,   755,    82,   576,   758,   759,   763,   757,
     764,   765,   691,   581,   120,   772,   774,   694,   775,   784,
     776,   780,   122,   123,   124,   777,   700,   125,   783,   126,
     602,   788,   786,   790,   791,   796,   129,   799,    82,   801,
     336,   337,   338,    82,   132,   803,   802,   804,   157,   782,
     663,   137,   709,   806,   699,   340,   341,   342,   343,   344,
     345,   346,   789,   631,   347,   798,   143,   144,   268,   724,
     638,   729,   688,   441,   611,   443,   579,   574,   645,   281,
     241,   647,   622,   649,   681,     0,   651,   544,     0,   756,
     149,   150,   442,     0,   152,     0,     0,     0,     0,     0,
       0,   153,   154,     0,   671,     0,   703,   673,     0,   155,
       0,   706,   156,    82,     0,     0,     0,    82,     0,     0,
     778,   779,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   805,     0,     0,
     687,     0,     0,     0,     0,     0,     0,     0,     0,   689,
       0,   101,     0,   690,     0,     0,   102,     0,     0,     0,
     103,     0,     0,     0,   104,   105,   106,  -147,   107,     0,
     108,     0,   109,     0,     0,   702,     0,     0,   110,   111,
       0,   766,     0,  -147,  -147,   773,   112,   113,   114,   115,
     116,   117,     0,     0,     0,   118,     0,     0,     0,     0,
       0,     0,   119,   120,     0,     0,     0,     0,     0,     0,
     121,   122,   123,   124,  -147,     0,   125,     0,   126,   127,
       0,   128,     0,  -147,     0,   129,     0,     0,     0,     0,
       0,   130,   131,   132,   133,     0,   134,   135,   136,     0,
     137,   138,     0,     0,     0,   139,   140,   141,   142,     0,
       0,     0,     0,   102,     0,   143,   144,    11,     0,     0,
     145,     0,   146,   106,   147,   107,     0,     0,     0,     0,
       0,   148,     0,     0,     0,     0,     0,     0,     0,   149,
     150,   151,     0,   152,   113,     0,     0,     0,   330,     0,
     153,   154,     0,     0,     0,   331,   332,     0,   155,  -147,
     120,   156,     0,     0,     0,     0,     0,     0,     0,   123,
     124,     0,     0,     0,     0,   126,   330,     0,     0,     0,
       0,     0,   129,   331,   332,     0,     0,     0,     0,     0,
     132,   333,     0,     0,     0,     0,     0,   137,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   143,   144,     0,   334,     0,     0,     0,   333,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   335,     0,   330,     0,   149,   150,   151,     0,
     152,   331,   332,   334,     0,     0,     0,   153,   154,     0,
       0,     0,     0,     0,     0,   155,     0,     0,   156,     0,
     335,     0,     0,     0,     0,   336,   337,   338,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   333,     0,   339,
     340,   341,   342,   343,   344,   345,   346,     0,     0,   347,
       0,     0,   430,   336,   337,   338,   330,     0,     0,     0,
       0,   334,     0,   331,   332,     0,   760,   339,   340,   341,
     342,   343,   344,   345,   346,     0,   564,   347,   335,     0,
     532,   330,   761,   762,     0,     0,     0,     0,   331,   332,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   333,
       0,   565,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   336,   337,   338,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   334,   333,   339,   340,   341,   342,   343,
     344,   345,   346,     0,     0,   347,   330,     0,     0,     0,
     335,     0,     0,   331,   332,     0,     0,     0,   334,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   335,     0,     0,     0,     0,
       0,     0,     0,   336,   337,   338,     0,     0,   330,   333,
       0,     0,     0,     0,     0,   331,   332,   339,   340,   341,
     342,   343,   344,   345,   346,     0,     0,   347,   336,   337,
     338,   358,     0,   334,     0,   428,     0,     0,     0,     0,
       0,     0,   339,   340,   341,   342,   343,   344,   345,   346,
     335,   333,   347,   330,     0,     0,     0,     0,     0,     0,
     331,   332,     0,     0,   390,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   334,     0,     0,     0,     0,
     330,     0,     0,   336,   337,   338,     0,   331,   332,     0,
       0,     0,   335,     0,     0,     0,   333,   339,   340,   341,
     342,   343,   344,   345,   346,     0,     0,   347,   330,     0,
       0,     0,     0,     0,     0,   331,   332,     0,     0,     0,
     334,     0,     0,   333,     0,   336,   337,   338,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   335,     0,   339,
     340,   341,   342,   343,   344,   345,   346,   334,     0,   347,
       0,   333,   330,     0,     0,     0,     0,     0,     0,   331,
     332,     0,     0,     0,   335,     0,     0,     0,     0,     0,
     336,   337,   338,   542,     0,   334,     0,     0,     0,   711,
       0,     0,     0,     0,   339,   340,   341,   342,   343,   344,
     345,   346,   335,     0,   347,   333,     0,   336,   337,   338,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   339,   340,   341,   342,   343,   344,   345,   346,   334,
       0,   347,     0,     0,     0,   336,   337,   338,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   339,
     340,   341,   342,   343,   344,   345,   346,     2,     3,   347,
       4,     0,     0,     0,     0,     0,     0,     0,   -63,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   -63,   336,
     337,   338,     0,     0,     5,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   340,   341,   342,   343,   344,   345,
     346,     6,     0,   347,     7,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   -63,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     0,     0,     9,    10,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    11,     0,    12
};

static const yytype_int16 yycheck[] =
{
      46,   525,    47,    48,    23,    50,   436,   498,   438,    42,
      60,   323,   324,     1,    60,    78,   540,    26,    34,    64,
     101,    54,    27,    27,    34,    17,   131,    11,   134,    74,
      30,    31,    34,    79,   131,    81,    34,    21,    84,   131,
      17,    64,   130,    17,   473,   151,    34,   153,   131,   147,
     131,   148,   133,   103,   767,   100,   148,    46,     4,   131,
     148,     7,     8,   135,   108,   148,   557,    34,   118,   115,
     783,   117,   122,   119,   131,   125,   126,    37,   150,    17,
     156,    73,    60,    67,   147,   131,   102,   133,   134,   131,
      99,   148,   138,    37,   140,   141,    73,   147,   107,    73,
     146,    79,   148,   153,   154,   155,   148,   107,    54,    55,
      56,    17,   100,   129,   130,   131,   132,   133,   151,   135,
     549,   550,   138,   133,   140,   141,   131,   131,    35,   131,
     621,   147,    17,   131,   150,    73,    19,   115,   147,   117,
      23,   119,   131,   131,   133,   105,    64,    54,   133,    32,
      57,    69,    35,   677,    94,   133,    96,   587,   130,   131,
     138,   105,   129,   130,   131,   135,   133,    73,   146,   115,
     116,   117,   546,   133,   142,   143,   148,   551,   146,   149,
     140,   141,    30,    31,   260,   261,   262,   263,    73,   133,
      97,   237,   147,   130,   131,   241,   140,   141,    48,   275,
     276,   108,    48,   248,   516,   250,   143,   283,   284,   142,
     143,   148,   148,   146,    48,   151,   149,   293,   294,   264,
     266,   140,   141,   142,   143,   301,   302,   146,   118,   119,
     131,   277,   282,   147,   135,   254,   281,   151,   147,   153,
     286,   287,   151,     1,   153,   131,   142,   143,   134,   295,
     146,   297,   129,   130,   131,    42,   133,   303,   304,   131,
     147,   311,   134,   241,   131,   131,   148,   134,   148,   151,
     148,   151,   318,   151,   129,   130,    34,   327,   328,   329,
     330,   140,   141,   333,   334,   335,    34,   148,   266,   339,
     151,    57,    58,   343,   344,   345,   346,   347,   348,   349,
       4,   129,   130,     7,     8,   381,   147,   357,   358,   287,
     148,   135,   147,   151,   138,    65,   131,   295,   133,   297,
     131,   131,   133,   368,   131,   131,   304,   403,     8,    78,
      10,   148,    64,   131,   410,   134,   131,   138,   245,   389,
     132,   131,   388,   147,   102,   147,   131,   147,   424,    78,
      54,    55,    56,   147,   400,   131,   113,    92,   147,   131,
     405,   131,   148,   131,   148,   148,   416,   412,   131,   148,
     132,   129,   130,   131,   132,   133,   131,   135,   132,   132,
     138,   426,   140,   141,   148,    67,   432,   147,   147,   147,
      65,   147,   150,   135,    67,   441,   147,   138,   108,   132,
     147,   122,   123,   124,   147,   132,   152,   147,   131,   148,
     388,   115,   116,   117,   118,   119,   137,   138,   139,   140,
     141,   142,   143,   149,   148,   146,   148,   131,   504,   147,
      48,   135,   120,    26,    42,   146,   512,   152,   131,   131,
     147,    20,   131,    26,   147,   147,   150,   493,   494,   132,
     147,   147,   147,   529,   432,   131,   150,   111,   534,   149,
      54,    40,   149,   441,   135,    44,    45,   152,   514,   376,
     149,   152,    51,   135,   152,   149,   135,   523,   524,   149,
     152,   149,   149,   390,   150,    64,   531,   152,   149,   147,
      69,   541,   542,   539,   134,   541,   149,   152,   135,   135,
     149,   148,   150,    70,   148,    84,   147,   132,   119,   147,
     149,    33,    91,     5,   148,   493,   494,   149,   149,   133,
     150,   428,   150,   568,   150,    82,   150,   147,   107,   574,
     131,   150,   149,   112,   579,   114,   128,   150,   134,   149,
     147,   150,    42,   149,   135,   523,   524,   149,    59,   131,
     150,   149,   131,   150,   149,   600,    48,     6,   147,   609,
     113,   539,   149,   541,   149,   149,   147,    16,   613,    18,
     150,   147,   146,    43,   132,   147,    97,    49,   130,   149,
      72,    54,   147,   134,   150,   147,   142,    34,    37,    98,
     131,   151,   147,   131,   640,   502,    74,   130,    96,   149,
      96,    42,   652,   510,    53,   131,    42,   657,    70,   152,
     150,   130,    61,    62,    63,   149,   666,    66,   101,    68,
     527,   152,   132,   131,    24,    54,    75,   128,   674,    54,
     122,   123,   124,   679,    83,   148,   131,   131,    60,   764,
     593,    90,   687,   149,   665,   137,   138,   139,   140,   141,
     142,   143,   774,   560,   146,   789,   105,   106,   115,   704,
     567,   711,   640,   112,   538,   329,   508,   500,   575,   127,
      84,   578,   547,   580,   621,    -1,   583,   465,    -1,   725,
     129,   130,   131,    -1,   133,    -1,    -1,    -1,    -1,    -1,
      -1,   140,   141,    -1,   601,    -1,   674,   604,    -1,   148,
      -1,   679,   151,   749,    -1,    -1,    -1,   753,    -1,    -1,
     760,   761,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   803,    -1,    -1,
     637,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   646,
      -1,     1,    -1,   650,    -1,    -1,     6,    -1,    -1,    -1,
      10,    -1,    -1,    -1,    14,    15,    16,    17,    18,    -1,
      20,    -1,    22,    -1,    -1,   672,    -1,    -1,    28,    29,
      -1,   749,    -1,    33,    34,   753,    36,    37,    38,    39,
      40,    41,    -1,    -1,    -1,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,
      60,    61,    62,    63,    64,    -1,    66,    -1,    68,    69,
      -1,    71,    -1,    73,    -1,    75,    -1,    -1,    -1,    -1,
      -1,    81,    82,    83,    84,    -1,    86,    87,    88,    -1,
      90,    91,    -1,    -1,    -1,    95,    96,    97,    98,    -1,
      -1,    -1,    -1,     6,    -1,   105,   106,   107,    -1,    -1,
     110,    -1,   112,    16,   114,    18,    -1,    -1,    -1,    -1,
      -1,   121,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,
     130,   131,    -1,   133,    37,    -1,    -1,    -1,     5,    -1,
     140,   141,    -1,    -1,    -1,    12,    13,    -1,   148,   149,
      53,   151,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    62,
      63,    -1,    -1,    -1,    -1,    68,     5,    -1,    -1,    -1,
      -1,    -1,    75,    12,    13,    -1,    -1,    -1,    -1,    -1,
      83,    48,    -1,    -1,    -1,    -1,    -1,    90,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   105,   106,    -1,    72,    -1,    -1,    -1,    48,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    89,    -1,     5,    -1,   129,   130,   131,    -1,
     133,    12,    13,    72,    -1,    -1,    -1,   140,   141,    -1,
      -1,    -1,    -1,    -1,    -1,   148,    -1,    -1,   151,    -1,
      89,    -1,    -1,    -1,    -1,   122,   123,   124,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    -1,   136,
     137,   138,   139,   140,   141,   142,   143,    -1,    -1,   146,
      -1,    -1,   149,   122,   123,   124,     5,    -1,    -1,    -1,
      -1,    72,    -1,    12,    13,    -1,    77,   136,   137,   138,
     139,   140,   141,   142,   143,    -1,    25,   146,    89,    -1,
     149,     5,    93,    94,    -1,    -1,    -1,    -1,    12,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,
      -1,    50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    72,    48,   136,   137,   138,   139,   140,
     141,   142,   143,    -1,    -1,   146,     5,    -1,    -1,    -1,
      89,    -1,    -1,    12,    13,    -1,    -1,    -1,    72,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   122,   123,   124,    -1,    -1,     5,    48,
      -1,    -1,    -1,    -1,    -1,    12,    13,   136,   137,   138,
     139,   140,   141,   142,   143,    -1,    -1,   146,   122,   123,
     124,   125,    -1,    72,    -1,    32,    -1,    -1,    -1,    -1,
      -1,    -1,   136,   137,   138,   139,   140,   141,   142,   143,
      89,    48,   146,     5,    -1,    -1,    -1,    -1,    -1,    -1,
      12,    13,    -1,    -1,   103,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    72,    -1,    -1,    -1,    -1,
       5,    -1,    -1,   122,   123,   124,    -1,    12,    13,    -1,
      -1,    -1,    89,    -1,    -1,    -1,    48,   136,   137,   138,
     139,   140,   141,   142,   143,    -1,    -1,   146,     5,    -1,
      -1,    -1,    -1,    -1,    -1,    12,    13,    -1,    -1,    -1,
      72,    -1,    -1,    48,    -1,   122,   123,   124,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,   136,
     137,   138,   139,   140,   141,   142,   143,    72,    -1,   146,
      -1,    48,     5,    -1,    -1,    -1,    -1,    -1,    -1,    12,
      13,    -1,    -1,    -1,    89,    -1,    -1,    -1,    -1,    -1,
     122,   123,   124,   125,    -1,    72,    -1,    -1,    -1,   104,
      -1,    -1,    -1,    -1,   136,   137,   138,   139,   140,   141,
     142,   143,    89,    -1,   146,    48,    -1,   122,   123,   124,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   136,   137,   138,   139,   140,   141,   142,   143,    72,
      -1,   146,    -1,    -1,    -1,   122,   123,   124,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,
     137,   138,   139,   140,   141,   142,   143,     0,     1,   146,
       3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    11,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,   122,
     123,   124,    -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   137,   138,   139,   140,   141,   142,
     143,    44,    -1,   146,    47,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    76,    -1,    -1,    79,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   107,    -1,   109
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   155,     0,     1,     3,    27,    44,    47,    76,    79,
      80,   107,   109,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   171,   175,   181,   182,   183,
     184,   185,    64,    69,   131,   147,   131,   134,   131,   134,
     297,    64,   134,   297,   108,   215,   133,   292,   221,   222,
     221,    11,    21,    67,   168,   169,   172,   176,   177,   225,
     226,   147,    48,    48,   225,    42,   147,   147,   147,   151,
     153,    48,   147,   147,   217,    34,   131,   216,   298,   293,
     294,   295,   296,   297,   133,   298,   298,    65,   223,   224,
     298,   131,   131,   131,   225,    78,   225,   148,   178,    64,
     219,     1,     6,    10,    14,    15,    16,    18,    20,    22,
      28,    29,    36,    37,    38,    39,    40,    41,    45,    52,
      53,    60,    61,    62,    63,    66,    68,    69,    71,    75,
      81,    82,    83,    84,    86,    87,    88,    90,    91,    95,
      96,    97,    98,   105,   106,   110,   112,   114,   121,   129,
     130,   131,   133,   140,   141,   148,   151,   185,   187,   197,
     203,   227,   228,   229,   230,   231,   233,   234,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     255,   258,   259,   260,   261,   262,   263,   264,   265,   272,
     273,   274,   275,   276,   277,   279,   282,   283,   284,   287,
     290,   291,   294,   296,   304,   312,    27,   131,   131,   298,
     134,   296,   301,   302,   312,   131,    27,   131,   131,   218,
     298,    20,    40,    44,    45,    51,    64,    69,    84,    91,
     107,   112,   114,   131,   138,   294,   132,   135,   296,   134,
     300,   293,   147,   147,   131,   147,   147,    78,   219,   131,
     219,   225,   113,   179,   220,   298,   147,   312,   131,   235,
     148,   151,   148,   151,   225,    92,   148,   131,   248,   294,
     296,   131,   294,   312,   294,   148,   151,   148,   312,   312,
     312,   292,   131,   148,   151,   296,   148,   148,   294,   296,
     131,   132,   299,   148,   151,   148,   294,   266,   296,   296,
     131,   148,   151,   148,   148,   294,   312,   296,   132,   305,
     132,   148,   312,   312,   312,   301,   147,   147,     4,     7,
       8,    54,    55,    56,   115,   116,   117,   118,   119,   150,
       5,    12,    13,    48,    72,    89,   122,   123,   124,   136,
     137,   138,   139,   140,   141,   142,   143,   146,   313,   314,
      67,   147,    65,   237,   147,   147,   152,   135,   125,    67,
     147,   138,   147,   108,   186,   132,   132,   296,   225,   131,
     298,    30,    31,   306,   298,   149,   148,    78,   147,   221,
     147,   148,   301,   301,   301,   301,   298,   294,   148,    48,
     103,   301,   301,    46,   133,   296,   298,   312,   301,   301,
     120,   296,   294,    17,    73,   285,   286,   301,   301,   294,
      17,    73,   288,   289,   294,   188,    26,   198,    42,   301,
     301,   133,   296,   294,    17,    73,   280,   281,    32,   312,
     149,   152,   148,   296,   131,   236,   131,   236,   236,   312,
     312,   112,   131,   250,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   147,   131,   147,   312,
     312,   147,   132,   147,   298,    30,    31,   107,   173,   147,
     130,   131,   143,   148,   307,   308,    26,   170,   147,   225,
     131,   301,   149,   152,   149,   152,   149,   294,   312,   225,
     149,   152,   135,   135,   135,   149,   152,   296,   135,   149,
     149,   301,   150,   298,    17,    73,   149,   152,   149,   301,
     150,   298,    17,    73,   111,   267,    54,   192,   312,   147,
     134,   149,   152,   135,   135,   149,   301,   150,   298,    17,
      73,   225,   149,   294,   148,    70,   238,   238,   238,   148,
     294,   148,   125,   147,   307,   132,   119,   174,   308,   142,
     143,   146,    37,   105,   133,   140,   141,   311,   147,   149,
     148,   180,   149,   149,    25,    50,   256,    33,   278,   133,
     294,   294,   130,   304,   285,   150,   225,   301,   150,   288,
     150,   225,   301,   150,   296,    82,   269,   236,   191,   129,
     130,   131,   133,   199,   200,   201,   202,   147,   294,   294,
     280,   150,   225,   301,   150,   298,   149,   301,   131,   128,
     239,   239,   294,   280,   252,   294,   296,   312,    37,   105,
     133,   311,   300,   149,   308,   308,   309,   311,   130,   303,
     304,   225,   147,    19,    23,    32,    35,   257,   225,   298,
     135,   149,   149,   149,   298,   225,   150,   225,   298,   225,
     150,   225,    42,   131,    59,   271,   238,    26,    99,   107,
     147,   193,    34,   200,   147,   135,   138,   204,   149,   149,
     298,   225,   150,   225,   113,   149,   312,   149,   298,   147,
     150,   303,   147,   130,   148,   310,   149,   225,   294,   225,
     225,   312,    43,   270,   312,   132,   189,    97,   147,   202,
     312,   205,   225,   294,   280,   253,   294,    49,   130,   298,
     149,   104,   134,     1,   194,     1,    34,   100,   131,   206,
     207,   208,   211,    54,   298,   147,   254,   150,   142,   312,
      34,    34,   102,   129,   130,   131,   132,   133,   135,   138,
     140,   141,   147,   150,   195,   196,   147,    98,   131,   151,
     147,    42,    54,   151,   214,   131,   296,   149,    74,   130,
      77,    93,    94,    96,    96,    42,   294,   101,   131,   133,
     209,   210,   131,   294,    42,    70,   150,   149,   312,   312,
     130,   190,   190,   101,   152,   210,   132,   212,   152,   209,
     131,    24,    57,    58,   268,   210,    54,   213,   212,   128,
     232,    54,   131,   148,   131,   301,   149
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   154,   155,   155,   156,   156,   156,   156,   156,   156,
     156,   156,   156,   156,   156,   156,   156,   157,   158,   158,
     158,   159,   159,   159,   160,   160,   161,   162,   163,   164,
     164,   165,   166,   166,   167,   168,   169,   170,   170,   170,
     170,   170,   171,   172,   173,   173,   173,   173,   174,   174,
     174,   174,   174,   175,   176,   176,   177,   178,   178,   179,
     179,   180,   180,   181,   181,   182,   183,   184,   185,   185,
     186,   186,   187,   187,   188,   189,   190,   191,   191,   192,
     192,   193,   193,   193,   194,   194,   195,   195,   195,   195,
     195,   195,   195,   195,   196,   196,   196,   196,   196,   197,
     198,   198,   199,   199,   200,   201,   201,   202,   202,   202,
     202,   204,   203,   205,   205,   205,   206,   206,   207,   208,
     208,   209,   209,   210,   210,   211,   211,   212,   212,   213,
     213,   214,   215,   215,   216,   217,   217,   218,   218,   219,
     220,   219,   221,   222,   222,   223,   224,   225,   226,   226,
     226,   226,   227,   227,   227,   227,   227,   227,   227,   227,
     227,   227,   227,   227,   227,   227,   227,   227,   227,   227,
     227,   227,   227,   227,   227,   227,   227,   227,   227,   227,
     227,   227,   227,   227,   227,   227,   227,   227,   227,   227,
     227,   227,   227,   228,   229,   230,   231,   231,   232,   232,
     233,   234,   235,   235,   236,   236,   237,   237,   238,   238,
     239,   239,   240,   241,   241,   241,   242,   242,   243,   244,
     245,   246,   247,   248,   248,   249,   249,   250,   250,   250,
     251,   252,   253,   254,   254,   255,   256,   256,   256,   257,
     257,   257,   257,   258,   258,   259,   260,   261,   262,   263,
     264,   264,   266,   265,   267,   267,   267,   267,   268,   268,
     268,   269,   269,   270,   270,   271,   271,   272,   273,   274,
     275,   276,   277,   278,   278,   279,   279,   279,   279,   280,
     281,   281,   281,   281,   282,   282,   282,   282,   282,   283,
     284,   284,   285,   286,   286,   286,   286,   287,   287,   288,
     289,   289,   289,   289,   290,   291,   292,   292,   293,   293,
     294,   295,   295,   295,   296,   297,   297,   297,   298,   298,
     298,   298,   298,   298,   298,   298,   298,   298,   298,   298,
     298,   298,   299,   299,   300,   300,   301,   301,   302,   302,
     302,   302,   303,   303,   304,   304,   305,   305,   306,   306,
     306,   307,   307,   308,   308,   308,   308,   308,   308,   309,
     310,   310,   311,   311,   311,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   312,   312,   312,   312,
     312,   312,   312,   312,   312,   312,   313,   313,   313,   313,
     313,   313,   314,   314
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
       1,     2,     0,     7,     0,     9,     8,     8,     0,     1,
       1,     0,     2,     0,     2,     0,     1,     2,     5,     2,
       2,     6,     5,     0,     2,     4,     6,     6,     8,     1,
       4,     3,     5,     4,     1,     1,     1,     1,     2,     2,
       4,     6,     1,     4,     3,     5,     4,     4,     6,     1,
       4,     3,     5,     4,     3,     3,     2,     3,     2,     3,
       1,     1,     3,     2,     2,     1,     3,     4,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     1,     0,     1,     0,     1,     1,     0,     1,     3,
       3,     5,     1,     1,     2,     2,     0,     1,     2,     1,
       0,     1,     1,     1,     1,     3,     3,     3,     3,     2,
       1,     5,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     3,     3,     3,     3,     3,     2,     2,
       6,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     3,     1,     1,     1,     1,
       1,     1,     1,     1
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
#line 895 "ascParse.y"
        {
	  /*
	   * The specific syntax diagnostic is already emitted by zz_error().
	   * Emitting a generic message here causes noisy duplicates during
	   * parser recovery.
	   */
	}
#line 2856 "ascParse.c"
    break;

  case 17: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 906 "ascParse.y"
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
#line 2902 "ascParse.c"
    break;

  case 18: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 951 "ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 2910 "ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK name ';'  */
#line 955 "ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2919 "ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name  */
#line 960 "ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 2928 "ascParse.c"
    break;

  case 21: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 968 "ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 2936 "ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK name ';'  */
#line 972 "ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2945 "ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name  */
#line 977 "ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 2954 "ascParse.c"
    break;

  case 24: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 985 "ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
            );
          }
	}
#line 2967 "ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 994 "ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 2980 "ascParse.c"
    break;

  case 26: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 1006 "ascParse.y"
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
#line 3012 "ascParse.c"
    break;

  case 27: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 1037 "ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 3021 "ascParse.c"
    break;

  case 28: /* add_method_def: add_method_head proclist end ';'  */
#line 1045 "ascParse.y"
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
#line 3045 "ascParse.c"
    break;

  case 29: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1068 "ascParse.y"
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
#line 3062 "ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1081 "ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3071 "ascParse.c"
    break;

  case 31: /* replace_method_def: replace_method_head proclist end ';'  */
#line 1089 "ascParse.y"
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
#line 3093 "ascParse.c"
    break;

  case 32: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1110 "ascParse.y"
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
#line 3109 "ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1122 "ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3118 "ascParse.c"
    break;

  case 34: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 1130 "ascParse.y"
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
#line 3172 "ascParse.c"
    break;

  case 35: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 1183 "ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 3184 "ascParse.c"
    break;

  case 36: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 1194 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3193 "ascParse.c"
    break;

  case 37: /* default_val: %empty  */
#line 1202 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3204 "ascParse.c"
    break;

  case 38: /* default_val: DEFAULT_TOK optional_sign number  */
#line 1209 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3214 "ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 1215 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 3226 "ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 1223 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 3238 "ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 1231 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 3250 "ascParse.c"
    break;

  case 42: /* constant_def: universal constant_head  */
#line 1242 "ascParse.y"
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
#line 3291 "ascParse.c"
    break;

  case 43: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK constant_dims constant_val optional_notes ';'  */
#line 1283 "ascParse.y"
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
#line 3325 "ascParse.c"
    break;

  case 44: /* constant_dims: DIMENSION_TOK dimensions  */
#line 1316 "ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	  g_constant_units = NULL;
	}
#line 3334 "ascParse.c"
    break;

  case 45: /* constant_dims: DIMENSIONLESS_TOK  */
#line 1321 "ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	  g_constant_units = NULL;
	}
#line 3343 "ascParse.c"
    break;

  case 46: /* constant_dims: %empty  */
#line 1326 "ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	  g_constant_units = NULL;
	}
#line 3352 "ascParse.c"
    break;

  case 47: /* constant_dims: UNITS_TOK BRACEDTEXT_TOK  */
#line 1331 "ascParse.y"
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
#line 3374 "ascParse.c"
    break;

  case 48: /* constant_val: %empty  */
#line 1352 "ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3385 "ascParse.c"
    break;

  case 49: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 1359 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3395 "ascParse.c"
    break;

  case 50: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 1365 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3407 "ascParse.c"
    break;

  case 51: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 1373 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3419 "ascParse.c"
    break;

  case 52: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 1381 "ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 3431 "ascParse.c"
    break;

  case 53: /* model_def: universal model_head fstatements methods end ';'  */
#line 1392 "ascParse.y"
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
#line 3473 "ascParse.c"
    break;

  case 54: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 1434 "ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 3485 "ascParse.c"
    break;

  case 55: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 1443 "ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 3498 "ascParse.c"
    break;

  case 56: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 1455 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3507 "ascParse.c"
    break;

  case 57: /* optional_model_parameters: %empty  */
#line 1463 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3515 "ascParse.c"
    break;

  case 58: /* optional_model_parameters: '(' fstatements ')'  */
#line 1467 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3523 "ascParse.c"
    break;

  case 59: /* optional_parameter_wheres: %empty  */
#line 1474 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3531 "ascParse.c"
    break;

  case 60: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 1478 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3539 "ascParse.c"
    break;

  case 61: /* optional_parameter_reduction: %empty  */
#line 1485 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3547 "ascParse.c"
    break;

  case 62: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 1489 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3555 "ascParse.c"
    break;

  case 63: /* universal: %empty  */
#line 1539 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3563 "ascParse.c"
    break;

  case 64: /* universal: UNIVERSAL_TOK  */
#line 1543 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3571 "ascParse.c"
    break;

  case 65: /* definition_def: definition_id fstatements methods end ';'  */
#line 1550 "ascParse.y"
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
#line 3602 "ascParse.c"
    break;

  case 66: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1580 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3611 "ascParse.c"
    break;

  case 67: /* units_def: units_statement ';'  */
#line 1589 "ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 3617 "ascParse.c"
    break;

  case 68: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1594 "ascParse.y"
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
#line 3638 "ascParse.c"
    break;

  case 69: /* units_statement: UNITS_TOK LADDER_TOK unitladderitemlist end optional_ladder_end  */
#line 1611 "ascParse.y"
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
#line 3664 "ascParse.c"
    break;

  case 70: /* optional_ladder_end: %empty  */
#line 1636 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3672 "ascParse.c"
    break;

  case 71: /* optional_ladder_end: LADDER_TOK  */
#line 1640 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3680 "ascParse.c"
    break;

  case 72: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on table_body END_TOK TABLE_TOK table_mode_off  */
#line 1647 "ascParse.y"
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
#line 3705 "ascParse.c"
    break;

  case 73: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on error END_TOK TABLE_TOK table_mode_off  */
#line 1668 "ascParse.y"
        {
	  DestroyName((yyvsp[-9].nptr));
	  TableParseAbort();
	  ErrMsg_Generic("Error in TABLE body.");
	  g_untrapped_error++;
	  yyerrok;
	  (yyval.statptr) = NULL;
	}
#line 3718 "ascParse.c"
    break;

  case 74: /* table_begin: %empty  */
#line 1680 "ascParse.y"
        {
	  TableParseBegin();
	}
#line 3726 "ascParse.c"
    break;

  case 75: /* table_mode_on: %empty  */
#line 1687 "ascParse.y"
        {
	  Asc_ScannerSetTableMode(1);
	}
#line 3734 "ascParse.c"
    break;

  case 76: /* table_mode_off: %empty  */
#line 1694 "ascParse.y"
        {
	  Asc_ScannerSetTableMode(0);
	}
#line 3742 "ascParse.c"
    break;

  case 79: /* table_decl_opt: %empty  */
#line 1706 "ascParse.y"
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
#line 3759 "ascParse.c"
    break;

  case 80: /* table_decl_opt: ISA_TOK type_identifier optional_of  */
#line 1719 "ascParse.y"
        {
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_type = (yyvsp[-1].id_ptr);
	  g_table_parse.decl_set_type = (yyvsp[0].id_ptr);
	  g_table_parse.decl_typeargs = g_typeargs;
	  g_typeargs = NULL;
	}
#line 3773 "ascParse.c"
    break;

  case 81: /* table_option: POSITIONAL_TOK  */
#line 1732 "ascParse.y"
        {
	  g_table_parse.positional = 1;
	}
#line 3781 "ascParse.c"
    break;

  case 82: /* table_option: DEFAULT_TOK expr  */
#line 1736 "ascParse.y"
        {
	  if (g_table_parse.default_expr != NULL) {
	    DestroyExprList(g_table_parse.default_expr);
	  }
	  g_table_parse.default_expr = (yyvsp[0].eptr);
	}
#line 3792 "ascParse.c"
    break;

  case 83: /* table_option: UNITS_TOK BRACEDTEXT_TOK  */
#line 1743 "ascParse.y"
        {
	  if (g_table_parse.units != NULL) {
	    ascfree(g_table_parse.units);
	  }
	  g_table_parse.units = ASC_STRDUP((yyvsp[0].braced_ptr));
	}
#line 3803 "ascParse.c"
    break;

  case 87: /* table_body_item: ':'  */
#line 1759 "ascParse.y"
        {
	  TableParseAppendToken(":",0);
	}
#line 3811 "ascParse.c"
    break;

  case 88: /* table_body_item: '='  */
#line 1763 "ascParse.y"
        {
	  TableParseAppendToken("=",0);
	}
#line 3819 "ascParse.c"
    break;

  case 89: /* table_body_item: ','  */
#line 1767 "ascParse.y"
        {
	  TableParseAppendToken(",",0);
	}
#line 3827 "ascParse.c"
    break;

  case 90: /* table_body_item: '+'  */
#line 1771 "ascParse.y"
        {
	  TableParseAppendToken("+",0);
	}
#line 3835 "ascParse.c"
    break;

  case 91: /* table_body_item: '-'  */
#line 1775 "ascParse.y"
        {
	  TableParseAppendToken("-",0);
	}
#line 3843 "ascParse.c"
    break;

  case 92: /* table_body_item: ';'  */
#line 1779 "ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3851 "ascParse.c"
    break;

  case 93: /* table_body_item: EOL_TOK  */
#line 1783 "ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3859 "ascParse.c"
    break;

  case 94: /* table_scalar: IDENTIFIER_TOK  */
#line 1790 "ascParse.y"
        {
	  TableParseAppendToken(SCP((yyvsp[0].id_ptr)),1);
	}
#line 3867 "ascParse.c"
    break;

  case 95: /* table_scalar: SYMBOL_TOK  */
#line 1794 "ascParse.y"
        {
	  TableParseAppendSymbol((yyvsp[0].sym_ptr));
	}
#line 3875 "ascParse.c"
    break;

  case 96: /* table_scalar: INTEGER_TOK  */
#line 1798 "ascParse.y"
        {
	  TableParseAppendInteger((yyvsp[0].int_value));
	}
#line 3883 "ascParse.c"
    break;

  case 97: /* table_scalar: REAL_TOK  */
#line 1802 "ascParse.y"
        {
	  TableParseAppendReal((yyvsp[0].real_value));
	}
#line 3891 "ascParse.c"
    break;

  case 98: /* table_scalar: BRACEDTEXT_TOK  */
#line 1806 "ascParse.y"
        {
	  TableParseAppendBraced((yyvsp[0].braced_ptr));
	}
#line 3899 "ascParse.c"
    break;

  case 99: /* values_statement: VALUES_TOK fname values_default_opt ';' values_entries END_TOK VALUES_TOK  */
#line 1813 "ascParse.y"
        {
	  DestroyName((yyvsp[-5].nptr));
	  (yyval.statptr) = NULL;
	}
#line 3908 "ascParse.c"
    break;

  case 101: /* values_default_opt: DEFAULT_TOK expr  */
#line 1822 "ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3916 "ascParse.c"
    break;

  case 104: /* values_entry: values_key_list '=' expr  */
#line 1834 "ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 3924 "ascParse.c"
    break;

  case 111: /* $@1: %empty  */
#line 1853 "ascParse.y"
        {
	  DatasetParseBegin((yyvsp[-3].id_ptr),(yyvsp[-1].dquote_ptr));
	}
#line 3932 "ascParse.c"
    break;

  case 112: /* dataset_statement: DATASET_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';' $@1 dataset_items END_TOK DATASET_TOK  */
#line 1857 "ascParse.y"
        {
	  (yyval.statptr) = DatasetParseFinish();
	}
#line 3940 "ascParse.c"
    break;

  case 115: /* dataset_items: dataset_items error ';'  */
#line 1866 "ascParse.y"
        {
	  ErrMsg_Generic("Error in DATASET item.");
	  ErrMsg_Generic("Check DATASET statement syntax.");
	  g_untrapped_error++;
	  yyerrok;
	}
#line 3951 "ascParse.c"
    break;

  case 118: /* dataset_index_item: INDEX_TOK IDENTIFIER_TOK FROM_TOK COLUMN_TOK dataset_column_ref ISA_TOK IDENTIFIER_TOK  */
#line 1881 "ascParse.y"
        {
	  DatasetParseAddIndex((yyvsp[-5].id_ptr),(yyvsp[-2].id_ptr),(yyvsp[0].id_ptr));
	}
#line 3959 "ascParse.c"
    break;

  case 119: /* dataset_map_item: dataset_target FROM_TOK dataset_column_selector dataset_units_opt dataset_type_opt  */
#line 1888 "ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-2].id_ptr),(yyvsp[-1].braced_ptr),(yyvsp[0].id_ptr));
	}
#line 3967 "ascParse.c"
    break;

  case 120: /* dataset_map_item: dataset_target dataset_type_req FROM_TOK dataset_column_selector dataset_units_opt  */
#line 1892 "ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-1].id_ptr),(yyvsp[0].braced_ptr),(yyvsp[-3].id_ptr));
	}
#line 3975 "ascParse.c"
    break;

  case 121: /* dataset_column_selector: COLUMN_TOK dataset_column_ref  */
#line 1899 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3983 "ascParse.c"
    break;

  case 122: /* dataset_column_selector: dataset_column_ref  */
#line 1903 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3991 "ascParse.c"
    break;

  case 123: /* dataset_column_ref: IDENTIFIER_TOK  */
#line 1910 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 3999 "ascParse.c"
    break;

  case 124: /* dataset_column_ref: SYMBOL_TOK  */
#line 1914 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].sym_ptr);
	}
#line 4007 "ascParse.c"
    break;

  case 125: /* dataset_target: IDENTIFIER_TOK '[' fvarlist ']'  */
#line 1921 "ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices(CreateIdName((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4016 "ascParse.c"
    break;

  case 126: /* dataset_target: dataset_target '[' fvarlist ']'  */
#line 1926 "ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices((yyvsp[-3].nptr),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4025 "ascParse.c"
    break;

  case 127: /* dataset_units_opt: %empty  */
#line 1934 "ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 4033 "ascParse.c"
    break;

  case 128: /* dataset_units_opt: BRACEDTEXT_TOK  */
#line 1938 "ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 4041 "ascParse.c"
    break;

  case 129: /* dataset_type_opt: %empty  */
#line 1945 "ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4049 "ascParse.c"
    break;

  case 130: /* dataset_type_opt: ISA_TOK IDENTIFIER_TOK  */
#line 1949 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4057 "ascParse.c"
    break;

  case 131: /* dataset_type_req: ISA_TOK IDENTIFIER_TOK  */
#line 1956 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4065 "ascParse.c"
    break;

  case 132: /* unitdeflist: %empty  */
#line 1962 "ascParse.y"
                {
		  (yyval.listp) = gl_create(100L);
	}
#line 4073 "ascParse.c"
    break;

  case 133: /* unitdeflist: unitdeflist unitdef  */
#line 1966 "ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 4082 "ascParse.c"
    break;

  case 134: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1974 "ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4091 "ascParse.c"
    break;

  case 135: /* unitladderitemlist: %empty  */
#line 1981 "ascParse.y"
        {
	  (yyval.listp) = gl_create(20L);
	}
#line 4099 "ascParse.c"
    break;

  case 136: /* unitladderitemlist: unitladderitemlist unitladderitem ';'  */
#line 1985 "ascParse.y"
        {
	  gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].ulitemptr));
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4108 "ascParse.c"
    break;

  case 137: /* unitladderitem: IDENTIFIER_TOK '=' BRACEDTEXT_TOK  */
#line 1993 "ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[-2].id_ptr),(yyvsp[0].braced_ptr),0,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4117 "ascParse.c"
    break;

  case 138: /* unitladderitem: IDENTIFIER_TOK  */
#line 1998 "ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[0].id_ptr),NULL,1,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4126 "ascParse.c"
    break;

  case 139: /* methods: %empty  */
#line 2007 "ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 4134 "ascParse.c"
    break;

  case 140: /* $@2: %empty  */
#line 2011 "ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 4145 "ascParse.c"
    break;

  case 141: /* methods: METHODS_TOK $@2 proclist  */
#line 2018 "ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 4153 "ascParse.c"
    break;

  case 142: /* proclist: proclistf  */
#line 2025 "ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 4162 "ascParse.c"
    break;

  case 143: /* proclistf: %empty  */
#line 2032 "ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4170 "ascParse.c"
    break;

  case 144: /* proclistf: proclistf procedure  */
#line 2036 "ascParse.y"
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
#line 4196 "ascParse.c"
    break;

  case 145: /* procedure: procedure_id ';' fstatements end ';'  */
#line 2061 "ascParse.y"
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
#line 4211 "ascParse.c"
    break;

  case 146: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 2075 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 4220 "ascParse.c"
    break;

  case 147: /* fstatements: statements  */
#line 2084 "ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 4228 "ascParse.c"
    break;

  case 148: /* statements: %empty  */
#line 2091 "ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4236 "ascParse.c"
    break;

  case 149: /* statements: statements statement ';'  */
#line 2095 "ascParse.y"
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
#line 4306 "ascParse.c"
    break;

  case 150: /* statements: statements complex_statement ';'  */
#line 2161 "ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
            gl_destroy((yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4318 "ascParse.c"
    break;

  case 151: /* statements: statements error ';'  */
#line 2169 "ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4327 "ascParse.c"
    break;

  case 194: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 2227 "ascParse.y"
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
#line 4369 "ascParse.c"
    break;

  case 195: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 2268 "ascParse.y"
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
#line 4399 "ascParse.c"
    break;

  case 196: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 2297 "ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 4407 "ascParse.c"
    break;

  case 197: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 2302 "ascParse.y"
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
#line 4457 "ascParse.c"
    break;

  case 198: /* optional_set_values: %empty  */
#line 2351 "ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4465 "ascParse.c"
    break;

  case 199: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 2355 "ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 4473 "ascParse.c"
    break;

  case 200: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 2362 "ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 4486 "ascParse.c"
    break;

  case 201: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 2374 "ascParse.y"
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
#line 4514 "ascParse.c"
    break;

  case 202: /* call_identifier: IDENTIFIER_TOK  */
#line 2401 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 4523 "ascParse.c"
    break;

  case 203: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2406 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 4532 "ascParse.c"
    break;

  case 204: /* type_identifier: IDENTIFIER_TOK  */
#line 2414 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 4541 "ascParse.c"
    break;

  case 205: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2419 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 4550 "ascParse.c"
    break;

  case 206: /* optional_method: %empty  */
#line 2427 "ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4558 "ascParse.c"
    break;

  case 207: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 2431 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4566 "ascParse.c"
    break;

  case 208: /* optional_of: %empty  */
#line 2438 "ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4574 "ascParse.c"
    break;

  case 209: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 2442 "ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4582 "ascParse.c"
    break;

  case 210: /* optional_with_value: %empty  */
#line 2449 "ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 4590 "ascParse.c"
    break;

  case 211: /* optional_with_value: WITH_VALUE_T expr  */
#line 2453 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4598 "ascParse.c"
    break;

  case 212: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 2460 "ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 4606 "ascParse.c"
    break;

  case 213: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 2467 "ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4614 "ascParse.c"
    break;

  case 214: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2471 "ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4622 "ascParse.c"
    break;

  case 215: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 2475 "ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4630 "ascParse.c"
    break;

  case 216: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2482 "ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4638 "ascParse.c"
    break;

  case 217: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 2486 "ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4646 "ascParse.c"
    break;

  case 218: /* der_statement: DER_TOK '(' fvarlist ')'  */
#line 2493 "ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 4656 "ascParse.c"
    break;

  case 219: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 2502 "ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 4666 "ascParse.c"
    break;

  case 220: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 2511 "ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 4674 "ascParse.c"
    break;

  case 221: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 2518 "ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 4682 "ascParse.c"
    break;

  case 222: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 2525 "ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 4690 "ascParse.c"
    break;

  case 223: /* assignment_statement: fname ASSIGN_TOK expr  */
#line 2532 "ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4698 "ascParse.c"
    break;

  case 224: /* assignment_statement: fname CASSIGN_TOK expr  */
#line 2536 "ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4706 "ascParse.c"
    break;

  case 225: /* relation_statement: relation  */
#line 2543 "ascParse.y"
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
#line 4723 "ascParse.c"
    break;

  case 226: /* relation_statement: fname ':' relation  */
#line 2556 "ascParse.y"
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
#line 4741 "ascParse.c"
    break;

  case 227: /* relation: expr  */
#line 2573 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 4755 "ascParse.c"
    break;

  case 228: /* relation: MINIMIZE_TOK expr  */
#line 2583 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4767 "ascParse.c"
    break;

  case 229: /* relation: MAXIMIZE_TOK expr  */
#line 2591 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4779 "ascParse.c"
    break;

  case 230: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 2602 "ascParse.y"
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
#line 4801 "ascParse.c"
    break;

  case 231: /* input_args: fvarlist ':' INPUT_TOK  */
#line 2623 "ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4809 "ascParse.c"
    break;

  case 232: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 2630 "ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4817 "ascParse.c"
    break;

  case 233: /* data_args: %empty  */
#line 2637 "ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 4825 "ascParse.c"
    break;

  case 234: /* data_args: ';' fname ':' DATA_TOK  */
#line 2641 "ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 4833 "ascParse.c"
    break;

  case 235: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 2679 "ascParse.y"
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
#line 4854 "ascParse.c"
    break;

  case 236: /* optional_direction: %empty  */
#line 2699 "ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 4862 "ascParse.c"
    break;

  case 237: /* optional_direction: INCREASING_TOK  */
#line 2703 "ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 4870 "ascParse.c"
    break;

  case 238: /* optional_direction: DECREASING_TOK  */
#line 2707 "ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 4878 "ascParse.c"
    break;

  case 239: /* forexprend: CREATE_TOK  */
#line 2714 "ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 4886 "ascParse.c"
    break;

  case 240: /* forexprend: EXPECT_TOK  */
#line 2718 "ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 4894 "ascParse.c"
    break;

  case 241: /* forexprend: CHECK_TOK  */
#line 2722 "ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 4902 "ascParse.c"
    break;

  case 242: /* forexprend: DO_TOK  */
#line 2726 "ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 4910 "ascParse.c"
    break;

  case 243: /* run_statement: RUN_TOK fname  */
#line 2733 "ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 4918 "ascParse.c"
    break;

  case 244: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 2737 "ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 4926 "ascParse.c"
    break;

  case 245: /* fix_statement: FIX_TOK fvarlist  */
#line 2744 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 4935 "ascParse.c"
    break;

  case 246: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 2752 "ascParse.y"
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
#line 4950 "ascParse.c"
    break;

  case 247: /* free_statement: FREE_TOK fvarlist  */
#line 2766 "ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 4958 "ascParse.c"
    break;

  case 248: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 2773 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 4967 "ascParse.c"
    break;

  case 249: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 2781 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 4976 "ascParse.c"
    break;

  case 250: /* solve_statement: SOLVE_TOK  */
#line 2789 "ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE(NULL);
	}
#line 4985 "ascParse.c"
    break;

  case 251: /* solve_statement: SOLVE_TOK fname  */
#line 2794 "ascParse.y"
        {
		(yyval.statptr) = CreateSOLVE((yyvsp[0].nptr));
	}
#line 4993 "ascParse.c"
    break;

  case 252: /* $@3: %empty  */
#line 2801 "ascParse.y"
        {
		g_study_parse = StudyParseEmpty();
		g_study_run_method = NULL;
		g_study_now = 0;
		g_study_filename = NULL;
	}
#line 5004 "ascParse.c"
    break;

  case 253: /* study_statement: STUDY_TOK $@3 fvarlist study_vary_opt study_run_opt study_now_opt study_file_opt  */
#line 2808 "ascParse.y"
        {
		(yyval.statptr) = CreateSTUDY((yyvsp[-4].lptr), g_study_parse.vary, g_study_parse.lower, g_study_parse.upper,
			g_study_parse.steps, g_study_parse.value, g_study_parse.mode, g_study_parse.dist,
			g_study_run_method, g_study_now, g_study_filename);
	}
#line 5014 "ascParse.c"
    break;

  case 254: /* study_vary_opt: %empty  */
#line 2817 "ascParse.y"
        {
	}
#line 5021 "ascParse.c"
    break;

  case 255: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEPS_TOK INTEGER_TOK study_distribution_opt  */
#line 2820 "ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-7].nptr);
		g_study_parse.lower = (yyvsp[-5].eptr);
		g_study_parse.upper = (yyvsp[-3].eptr);
		g_study_parse.steps = (yyvsp[-1].int_value);
		g_study_parse.mode = study_steps;
	}
#line 5033 "ascParse.c"
    break;

  case 256: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEP_TOK expr  */
#line 2828 "ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_step;
		g_study_parse.dist = study_dist_linear;
	}
#line 5046 "ascParse.c"
    break;

  case 257: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr RATIO_TOK expr  */
#line 2837 "ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_ratio;
		g_study_parse.dist = study_dist_log;
	}
#line 5059 "ascParse.c"
    break;

  case 258: /* study_distribution_opt: %empty  */
#line 2849 "ascParse.y"
        {
	}
#line 5066 "ascParse.c"
    break;

  case 259: /* study_distribution_opt: LINEAR_TOK  */
#line 2852 "ascParse.y"
        {
		g_study_parse.dist = study_dist_linear;
	}
#line 5074 "ascParse.c"
    break;

  case 260: /* study_distribution_opt: LOG_TOK  */
#line 2856 "ascParse.y"
        {
		g_study_parse.dist = study_dist_log;
	}
#line 5082 "ascParse.c"
    break;

  case 261: /* study_run_opt: %empty  */
#line 2863 "ascParse.y"
        {
	}
#line 5089 "ascParse.c"
    break;

  case 262: /* study_run_opt: RUN_TOK IDENTIFIER_TOK  */
#line 2866 "ascParse.y"
        {
		g_study_run_method = (yyvsp[0].id_ptr);
	}
#line 5097 "ascParse.c"
    break;

  case 263: /* study_file_opt: %empty  */
#line 2873 "ascParse.y"
        {
	}
#line 5104 "ascParse.c"
    break;

  case 264: /* study_file_opt: FILE_TOK DQUOTE_TOK  */
#line 2876 "ascParse.y"
        {
		g_study_filename = (yyvsp[0].dquote_ptr);
	}
#line 5112 "ascParse.c"
    break;

  case 265: /* study_now_opt: %empty  */
#line 2883 "ascParse.y"
        {
	}
#line 5119 "ascParse.c"
    break;

  case 266: /* study_now_opt: NOW_TOK  */
#line 2886 "ascParse.y"
        {
		g_study_now = 1;
	}
#line 5127 "ascParse.c"
    break;

  case 267: /* delete_statement: DELETE_TOK SYSTEM_TOK  */
#line 2893 "ascParse.y"
        {
		(yyval.statptr) = CreateDELETESYSTEM();
	}
#line 5135 "ascParse.c"
    break;

  case 268: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 2900 "ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 5147 "ascParse.c"
    break;

  case 269: /* call_statement: CALL_TOK call_identifier  */
#line 2911 "ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 5159 "ascParse.c"
    break;

  case 270: /* assert_statement: ASSERT_TOK expr  */
#line 2922 "ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 5167 "ascParse.c"
    break;

  case 271: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 2928 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 5178 "ascParse.c"
    break;

  case 272: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 2938 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 5189 "ascParse.c"
    break;

  case 273: /* optional_else: %empty  */
#line 2947 "ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 5197 "ascParse.c"
    break;

  case 274: /* optional_else: ELSE_TOK fstatements  */
#line 2951 "ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 5205 "ascParse.c"
    break;

  case 275: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 2958 "ascParse.y"
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
#line 5220 "ascParse.c"
    break;

  case 276: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 2969 "ascParse.y"
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
#line 5236 "ascParse.c"
    break;

  case 277: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2981 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5247 "ascParse.c"
    break;

  case 278: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 2988 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5258 "ascParse.c"
    break;

  case 279: /* whenlist: whenlistf  */
#line 2998 "ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 5266 "ascParse.c"
    break;

  case 280: /* whenlistf: CASE_TOK set ':' fstatements  */
#line 3005 "ascParse.y"
        {
	  (yyval.wptr) = CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5274 "ascParse.c"
    break;

  case 281: /* whenlistf: OTHERWISE_TOK ':' fstatements  */
#line 3009 "ascParse.y"
        {
	  (yyval.wptr) = CreateWhen(NULL,(yyvsp[0].slptr));
	}
#line 5282 "ascParse.c"
    break;

  case 282: /* whenlistf: whenlistf CASE_TOK set ':' fstatements  */
#line 3013 "ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	}
#line 5290 "ascParse.c"
    break;

  case 283: /* whenlistf: whenlistf OTHERWISE_TOK ':' fstatements  */
#line 3017 "ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhen(NULL,(yyvsp[0].slptr)),(yyvsp[-3].wptr));
	}
#line 5298 "ascParse.c"
    break;

  case 284: /* flow_statement: BREAK_TOK  */
#line 3024 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 5306 "ascParse.c"
    break;

  case 285: /* flow_statement: CONTINUE_TOK  */
#line 3028 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 5314 "ascParse.c"
    break;

  case 286: /* flow_statement: FALLTHRU_TOK  */
#line 3032 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 5322 "ascParse.c"
    break;

  case 287: /* flow_statement: RETURN_TOK  */
#line 3036 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 5330 "ascParse.c"
    break;

  case 288: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 3040 "ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 5338 "ascParse.c"
    break;

  case 289: /* use_statement: USE_TOK fname  */
#line 3047 "ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 5346 "ascParse.c"
    break;

  case 290: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 3054 "ascParse.y"
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
#line 5361 "ascParse.c"
    break;

  case 291: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 3065 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 5372 "ascParse.c"
    break;

  case 292: /* selectlist: selectlistf  */
#line 3075 "ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 5380 "ascParse.c"
    break;

  case 293: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 3082 "ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5388 "ascParse.c"
    break;

  case 294: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 3086 "ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 5396 "ascParse.c"
    break;

  case 295: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 3090 "ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 5404 "ascParse.c"
    break;

  case 296: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 3094 "ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 5412 "ascParse.c"
    break;

  case 297: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 3101 "ascParse.y"
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
#line 5427 "ascParse.c"
    break;

  case 298: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 3112 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 5438 "ascParse.c"
    break;

  case 299: /* switchlist: switchlistf  */
#line 3122 "ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 5446 "ascParse.c"
    break;

  case 300: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 3129 "ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5454 "ascParse.c"
    break;

  case 301: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 3133 "ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 5462 "ascParse.c"
    break;

  case 302: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 3137 "ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 5470 "ascParse.c"
    break;

  case 303: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 3141 "ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 5478 "ascParse.c"
    break;

  case 304: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 3148 "ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 5489 "ascParse.c"
    break;

  case 305: /* notes_statement: NOTES_TOK notes_body end  */
#line 3158 "ascParse.y"
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
#line 5527 "ascParse.c"
    break;

  case 306: /* notes_body: SYMBOL_TOK noteslist  */
#line 3195 "ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 5540 "ascParse.c"
    break;

  case 307: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 3204 "ascParse.y"
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
#line 5556 "ascParse.c"
    break;

  case 308: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 3219 "ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 5565 "ascParse.c"
    break;

  case 309: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 3224 "ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 5575 "ascParse.c"
    break;

  case 310: /* fvarlist: varlist  */
#line 3233 "ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5587 "ascParse.c"
    break;

  case 311: /* varlist: fname  */
#line 3244 "ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5595 "ascParse.c"
    break;

  case 312: /* varlist: varlist ',' fname  */
#line 3248 "ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5604 "ascParse.c"
    break;

  case 313: /* varlist: varlist fname  */
#line 3253 "ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 5618 "ascParse.c"
    break;

  case 314: /* fname: name optional_notes  */
#line 3266 "ascParse.y"
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
#line 5639 "ascParse.c"
    break;

  case 315: /* name: IDENTIFIER_TOK  */
#line 3286 "ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 5647 "ascParse.c"
    break;

  case 316: /* name: name '.' IDENTIFIER_TOK  */
#line 3290 "ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5656 "ascParse.c"
    break;

  case 317: /* name: name '[' set ']'  */
#line 3295 "ascParse.y"
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
#line 5672 "ascParse.c"
    break;

  case 318: /* end: END_TOK CONDITIONAL_TOK  */
#line 3310 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 5681 "ascParse.c"
    break;

  case 319: /* end: END_TOK FOR_TOK  */
#line 3315 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 5690 "ascParse.c"
    break;

  case 320: /* end: END_TOK IF_TOK  */
#line 3320 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 5699 "ascParse.c"
    break;

  case 321: /* end: END_TOK INTERACTIVE_TOK  */
#line 3325 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 5708 "ascParse.c"
    break;

  case 322: /* end: END_TOK METHODS_TOK  */
#line 3330 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 5717 "ascParse.c"
    break;

  case 323: /* end: END_TOK NOTES_TOK  */
#line 3335 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 5726 "ascParse.c"
    break;

  case 324: /* end: END_TOK SELECT_TOK  */
#line 3340 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 5735 "ascParse.c"
    break;

  case 325: /* end: END_TOK SWITCH_TOK  */
#line 3345 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 5744 "ascParse.c"
    break;

  case 326: /* end: END_TOK UNITS_TOK  */
#line 3350 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 5753 "ascParse.c"
    break;

  case 327: /* end: END_TOK GLOBAL_TOK  */
#line 3355 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 5762 "ascParse.c"
    break;

  case 328: /* end: END_TOK WHEN_TOK  */
#line 3360 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 5771 "ascParse.c"
    break;

  case 329: /* end: END_TOK WHILE_TOK  */
#line 3365 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 5780 "ascParse.c"
    break;

  case 330: /* end: END_TOK IDENTIFIER_TOK  */
#line 3370 "ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 5789 "ascParse.c"
    break;

  case 331: /* end: END_TOK  */
#line 3375 "ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 5798 "ascParse.c"
    break;

  case 332: /* optional_bracedtext: %empty  */
#line 3383 "ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 5806 "ascParse.c"
    break;

  case 333: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 3387 "ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 5814 "ascParse.c"
    break;

  case 334: /* optional_notes: %empty  */
#line 3394 "ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 5822 "ascParse.c"
    break;

  case 335: /* optional_notes: DQUOTE_TOK  */
#line 3398 "ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 5830 "ascParse.c"
    break;

  case 336: /* set: setexprlist  */
#line 3405 "ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 5838 "ascParse.c"
    break;

  case 337: /* set: %empty  */
#line 3409 "ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 5846 "ascParse.c"
    break;

  case 338: /* setexprlist: expr  */
#line 3416 "ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 5854 "ascParse.c"
    break;

  case 339: /* setexprlist: expr DOTDOT_TOK expr  */
#line 3420 "ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 5862 "ascParse.c"
    break;

  case 340: /* setexprlist: setexprlist ',' expr  */
#line 3424 "ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 5871 "ascParse.c"
    break;

  case 341: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 3429 "ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 5880 "ascParse.c"
    break;

  case 342: /* number: INTEGER_TOK  */
#line 3437 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	  g_number_units = NULL;
	}
#line 5891 "ascParse.c"
    break;

  case 343: /* number: realnumber  */
#line 3444 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 5901 "ascParse.c"
    break;

  case 344: /* realnumber: REAL_TOK opunits  */
#line 3453 "ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	  g_number_units = g_parsed_units;
	}
#line 5910 "ascParse.c"
    break;

  case 345: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 3458 "ascParse.y"
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
#line 5935 "ascParse.c"
    break;

  case 346: /* opunits: %empty  */
#line 3482 "ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  g_parsed_units = NULL;
	  (yyval.real_value) = 1.0;
	}
#line 5945 "ascParse.c"
    break;

  case 347: /* opunits: BRACEDTEXT_TOK  */
#line 3488 "ascParse.y"
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
#line 5969 "ascParse.c"
    break;

  case 348: /* dims: DIMENSION_TOK dimensions  */
#line 3511 "ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 5977 "ascParse.c"
    break;

  case 349: /* dims: DIMENSIONLESS_TOK  */
#line 3515 "ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 5985 "ascParse.c"
    break;

  case 350: /* dims: %empty  */
#line 3519 "ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 5993 "ascParse.c"
    break;

  case 351: /* dimensions: '*'  */
#line 3526 "ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 6001 "ascParse.c"
    break;

  case 352: /* dimensions: dimexpr  */
#line 3530 "ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 6009 "ascParse.c"
    break;

  case 353: /* dimexpr: IDENTIFIER_TOK  */
#line 3537 "ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 6017 "ascParse.c"
    break;

  case 354: /* dimexpr: INTEGER_TOK  */
#line 3541 "ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 6025 "ascParse.c"
    break;

  case 355: /* dimexpr: dimexpr '/' dimexpr  */
#line 3545 "ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6033 "ascParse.c"
    break;

  case 356: /* dimexpr: dimexpr '*' dimexpr  */
#line 3549 "ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6041 "ascParse.c"
    break;

  case 357: /* dimexpr: dimexpr '^' fraction  */
#line 3553 "ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 6049 "ascParse.c"
    break;

  case 358: /* dimexpr: '(' dimexpr ')'  */
#line 3557 "ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 6057 "ascParse.c"
    break;

  case 359: /* fraction: optional_sign fractail  */
#line 3564 "ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 6065 "ascParse.c"
    break;

  case 360: /* fractail: INTEGER_TOK  */
#line 3571 "ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 6073 "ascParse.c"
    break;

  case 361: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 3575 "ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 6081 "ascParse.c"
    break;

  case 362: /* optional_sign: %empty  */
#line 3582 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6089 "ascParse.c"
    break;

  case 363: /* optional_sign: '+'  */
#line 3586 "ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6097 "ascParse.c"
    break;

  case 364: /* optional_sign: '-'  */
#line 3590 "ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 6105 "ascParse.c"
    break;

  case 365: /* expr: INTEGER_TOK  */
#line 3597 "ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 6113 "ascParse.c"
    break;

  case 366: /* expr: MAXINTEGER_TOK  */
#line 3601 "ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 6121 "ascParse.c"
    break;

  case 367: /* expr: realnumber  */
#line 3605 "ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 6129 "ascParse.c"
    break;

  case 368: /* expr: MAXREAL_TOK  */
#line 3609 "ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 6137 "ascParse.c"
    break;

  case 369: /* expr: TRUE_TOK  */
#line 3613 "ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 6145 "ascParse.c"
    break;

  case 370: /* expr: FALSE_TOK  */
#line 3617 "ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 6153 "ascParse.c"
    break;

  case 371: /* expr: ANY_TOK  */
#line 3621 "ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 6161 "ascParse.c"
    break;

  case 372: /* expr: SYMBOL_TOK  */
#line 3625 "ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 6169 "ascParse.c"
    break;

  case 373: /* expr: fname  */
#line 3629 "ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 6177 "ascParse.c"
    break;

  case 374: /* expr: '[' set ']'  */
#line 3633 "ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 6185 "ascParse.c"
    break;

  case 375: /* expr: expr '+' expr  */
#line 3637 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6194 "ascParse.c"
    break;

  case 376: /* expr: expr '-' expr  */
#line 3642 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6203 "ascParse.c"
    break;

  case 377: /* expr: expr '*' expr  */
#line 3647 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6212 "ascParse.c"
    break;

  case 378: /* expr: expr '/' expr  */
#line 3652 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6221 "ascParse.c"
    break;

  case 379: /* expr: expr '^' expr  */
#line 3657 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6230 "ascParse.c"
    break;

  case 380: /* expr: expr AND_TOK expr  */
#line 3662 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6239 "ascParse.c"
    break;

  case 381: /* expr: expr OR_TOK expr  */
#line 3667 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6248 "ascParse.c"
    break;

  case 382: /* expr: NOT_TOK expr  */
#line 3672 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 6256 "ascParse.c"
    break;

  case 383: /* expr: expr relop expr  */
#line 3676 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6265 "ascParse.c"
    break;

  case 384: /* expr: expr logrelop expr  */
#line 3681 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6274 "ascParse.c"
    break;

  case 385: /* expr: expr IN_TOK expr  */
#line 3686 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6283 "ascParse.c"
    break;

  case 386: /* expr: expr '|' expr  */
#line 3691 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6292 "ascParse.c"
    break;

  case 387: /* expr: expr SUCHTHAT_TOK expr  */
#line 3696 "ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6301 "ascParse.c"
    break;

  case 388: /* expr: '+' expr  */
#line 3701 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 6309 "ascParse.c"
    break;

  case 389: /* expr: '-' expr  */
#line 3705 "ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 6317 "ascParse.c"
    break;

  case 390: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 3709 "ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 6325 "ascParse.c"
    break;

  case 391: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 3713 "ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 6333 "ascParse.c"
    break;

  case 392: /* expr: SUM_TOK '(' set ')'  */
#line 3717 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 6344 "ascParse.c"
    break;

  case 393: /* expr: SUM_TOK '[' set ']'  */
#line 3724 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 6352 "ascParse.c"
    break;

  case 394: /* expr: PROD_TOK '(' set ')'  */
#line 3728 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 6363 "ascParse.c"
    break;

  case 395: /* expr: PROD_TOK '[' set ']'  */
#line 3735 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 6371 "ascParse.c"
    break;

  case 396: /* expr: UNION_TOK '(' set ')'  */
#line 3739 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 6382 "ascParse.c"
    break;

  case 397: /* expr: UNION_TOK '[' set ']'  */
#line 3746 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 6390 "ascParse.c"
    break;

  case 398: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 3750 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 6401 "ascParse.c"
    break;

  case 399: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 3757 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 6409 "ascParse.c"
    break;

  case 400: /* expr: CARD_TOK '(' set ')'  */
#line 3761 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 6420 "ascParse.c"
    break;

  case 401: /* expr: CARD_TOK '[' set ']'  */
#line 3768 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 6428 "ascParse.c"
    break;

  case 402: /* expr: CHOICE_TOK '(' set ')'  */
#line 3772 "ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 6439 "ascParse.c"
    break;

  case 403: /* expr: CHOICE_TOK '[' set ']'  */
#line 3779 "ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 6447 "ascParse.c"
    break;

  case 404: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 3783 "ascParse.y"
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
#line 6462 "ascParse.c"
    break;

  case 405: /* expr: '(' expr ')'  */
#line 3794 "ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 6470 "ascParse.c"
    break;

  case 406: /* relop: '='  */
#line 3801 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 6478 "ascParse.c"
    break;

  case 407: /* relop: '<'  */
#line 3805 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 6486 "ascParse.c"
    break;

  case 408: /* relop: '>'  */
#line 3809 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 6494 "ascParse.c"
    break;

  case 409: /* relop: LEQ_TOK  */
#line 3813 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 6502 "ascParse.c"
    break;

  case 410: /* relop: GEQ_TOK  */
#line 3817 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 6510 "ascParse.c"
    break;

  case 411: /* relop: NEQ_TOK  */
#line 3821 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 6518 "ascParse.c"
    break;

  case 412: /* logrelop: BEQ_TOK  */
#line 3828 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 6526 "ascParse.c"
    break;

  case 413: /* logrelop: BNE_TOK  */
#line 3832 "ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 6534 "ascParse.c"
    break;


#line 6538 "ascParse.c"

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

#line 3836 "ascParse.y"

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
