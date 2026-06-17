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

static int g_classifier_predicate_depth = 0;
/* Inline SATISFIED(real_relation,tol) is only supported while parsing
 * classifier predicates attached to WHEN cases.
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


#line 794 "ascend/compiler/ascParse.c"

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
  YYSYMBOL_optional_when_otherwise_label = 298, /* optional_when_otherwise_label  */
  YYSYMBOL_optional_when_case_if = 299,    /* optional_when_case_if  */
  YYSYMBOL_300_4 = 300,                    /* $@4  */
  YYSYMBOL_optional_when_case_applies = 301, /* optional_when_case_applies  */
  YYSYMBOL_302_5 = 302,                    /* $@5  */
  YYSYMBOL_flow_statement = 303,           /* flow_statement  */
  YYSYMBOL_use_statement = 304,            /* use_statement  */
  YYSYMBOL_select_statement = 305,         /* select_statement  */
  YYSYMBOL_selectlist = 306,               /* selectlist  */
  YYSYMBOL_selectlistf = 307,              /* selectlistf  */
  YYSYMBOL_switch_statement = 308,         /* switch_statement  */
  YYSYMBOL_switchlist = 309,               /* switchlist  */
  YYSYMBOL_switchlistf = 310,              /* switchlistf  */
  YYSYMBOL_conditional_statement = 311,    /* conditional_statement  */
  YYSYMBOL_notes_statement = 312,          /* notes_statement  */
  YYSYMBOL_notes_body = 313,               /* notes_body  */
  YYSYMBOL_noteslist = 314,                /* noteslist  */
  YYSYMBOL_fvarlist = 315,                 /* fvarlist  */
  YYSYMBOL_varlist = 316,                  /* varlist  */
  YYSYMBOL_method_fvarlist = 317,          /* method_fvarlist  */
  YYSYMBOL_method_varlist = 318,           /* method_varlist  */
  YYSYMBOL_fvarref = 319,                  /* fvarref  */
  YYSYMBOL_fname = 320,                    /* fname  */
  YYSYMBOL_name = 321,                     /* name  */
  YYSYMBOL_end = 322,                      /* end  */
  YYSYMBOL_optional_bracedtext = 323,      /* optional_bracedtext  */
  YYSYMBOL_optional_notes = 324,           /* optional_notes  */
  YYSYMBOL_set = 325,                      /* set  */
  YYSYMBOL_setexprlist = 326,              /* setexprlist  */
  YYSYMBOL_number = 327,                   /* number  */
  YYSYMBOL_realnumber = 328,               /* realnumber  */
  YYSYMBOL_opunits = 329,                  /* opunits  */
  YYSYMBOL_dims = 330,                     /* dims  */
  YYSYMBOL_dimensions = 331,               /* dimensions  */
  YYSYMBOL_dimexpr = 332,                  /* dimexpr  */
  YYSYMBOL_fraction = 333,                 /* fraction  */
  YYSYMBOL_fractail = 334,                 /* fractail  */
  YYSYMBOL_optional_sign = 335,            /* optional_sign  */
  YYSYMBOL_expr = 336,                     /* expr  */
  YYSYMBOL_relop = 337,                    /* relop  */
  YYSYMBOL_logrelop = 338                  /* logrelop  */
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
#define YYLAST   1723

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  162
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  177
/* YYNRULES -- Number of rules.  */
#define YYNRULES  449
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  884

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
       0,   889,   889,   891,   895,   896,   897,   898,   899,   900,
     901,   902,   903,   904,   906,   907,   908,   919,   964,   968,
     973,   981,   985,   990,   998,  1007,  1019,  1050,  1058,  1081,
    1094,  1102,  1123,  1135,  1143,  1196,  1207,  1216,  1222,  1228,
    1236,  1244,  1255,  1295,  1329,  1334,  1340,  1344,  1366,  1372,
    1378,  1386,  1394,  1405,  1447,  1456,  1469,  1478,  1481,  1489,
    1492,  1500,  1503,  1554,  1557,  1564,  1594,  1603,  1608,  1625,
    1651,  1654,  1661,  1682,  1695,  1702,  1709,  1714,  1716,  1721,
    1733,  1746,  1750,  1757,  1766,  1768,  1772,  1773,  1777,  1781,
    1785,  1789,  1793,  1797,  1804,  1808,  1812,  1816,  1820,  1827,
    1834,  1836,  1843,  1844,  1848,  1855,  1856,  1860,  1861,  1862,
    1863,  1868,  1867,  1877,  1879,  1880,  1890,  1891,  1895,  1902,
    1906,  1913,  1917,  1924,  1928,  1935,  1940,  1949,  1952,  1960,
    1963,  1970,  1977,  1980,  1988,  1996,  1999,  2007,  2012,  2022,
    2026,  2025,  2040,  2043,  2051,  2059,  2062,  2087,  2101,  2110,
    2118,  2121,  2193,  2201,  2209,  2210,  2211,  2212,  2213,  2214,
    2215,  2216,  2217,  2218,  2219,  2220,  2221,  2222,  2223,  2224,
    2225,  2227,  2228,  2229,  2230,  2231,  2232,  2233,  2234,  2235,
    2236,  2237,  2238,  2239,  2240,  2241,  2242,  2243,  2244,  2245,
    2246,  2247,  2248,  2249,  2250,  2251,  2252,  2253,  2254,  2255,
    2259,  2264,  2297,  2327,  2331,  2382,  2385,  2392,  2404,  2431,
    2436,  2444,  2449,  2458,  2461,  2469,  2472,  2480,  2483,  2488,
    2496,  2503,  2507,  2511,  2518,  2522,  2529,  2538,  2547,  2554,
    2561,  2568,  2572,  2579,  2586,  2593,  2606,  2623,  2633,  2641,
    2652,  2673,  2680,  2688,  2691,  2728,  2750,  2753,  2757,  2764,
    2768,  2772,  2776,  2783,  2787,  2794,  2802,  2816,  2823,  2831,
    2838,  2846,  2851,  2858,  2865,  2873,  2876,  2884,  2883,  2900,
    2903,  2911,  2913,  2921,  2930,  2943,  2945,  2949,  2957,  2959,
    2967,  2969,  2977,  2979,  2986,  2993,  3004,  3015,  3021,  3031,
    3041,  3044,  3051,  3062,  3074,  3081,  3091,  3098,  3102,  3106,
    3110,  3117,  3120,  3127,  3130,  3130,  3138,  3141,  3141,  3149,
    3153,  3157,  3161,  3165,  3172,  3179,  3190,  3200,  3207,  3211,
    3215,  3219,  3226,  3237,  3247,  3254,  3258,  3262,  3266,  3273,
    3283,  3320,  3329,  3344,  3349,  3358,  3369,  3373,  3378,  3391,
    3398,  3402,  3407,  3417,  3421,  3428,  3448,  3452,  3457,  3462,
    3477,  3482,  3487,  3492,  3497,  3502,  3507,  3512,  3517,  3522,
    3527,  3532,  3537,  3542,  3551,  3554,  3562,  3565,  3572,  3577,
    3583,  3587,  3591,  3595,  3600,  3605,  3613,  3620,  3629,  3634,
    3659,  3664,  3687,  3691,  3696,  3702,  3706,  3713,  3717,  3721,
    3725,  3729,  3733,  3740,  3747,  3751,  3759,  3762,  3766,  3773,
    3777,  3781,  3785,  3789,  3793,  3797,  3801,  3805,  3809,  3813,
    3817,  3822,  3827,  3832,  3837,  3842,  3847,  3852,  3856,  3861,
    3866,  3871,  3876,  3881,  3885,  3889,  3902,  3906,  3910,  3917,
    3921,  3928,  3932,  3939,  3943,  3950,  3954,  3961,  3965,  3972,
    3976,  3998,  4005,  4009,  4013,  4017,  4021,  4025,  4032,  4036
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
  "optional_when_otherwise_label", "optional_when_case_if", "$@4",
  "optional_when_case_applies", "$@5", "flow_statement", "use_statement",
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

#define YYPACT_NINF (-816)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-345)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -816,  1269,  -816,  -816,   126,   -55,   -44,   -56,    48,    98,
     101,    73,  -816,  -816,  -816,  -816,  -816,  -816,  -816,    69,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,   159,  -816,  -816,
    -816,    49,   168,   176,  -816,  -816,   194,    95,  -816,   114,
      87,   195,   130,   125,  -816,   -14,   122,   -13,   245,   219,
     245,   161,   171,   178,  -816,   230,  -816,  -816,   163,   254,
     827,  -816,   -15,   190,   245,   203,  -816,  -816,  -816,   587,
       1,    -1,  -816,  -816,    14,  1584,   201,  -816,  -816,   122,
     209,    94,  -816,   -95,   122,   196,   211,   223,  -816,   224,
     231,  -816,   302,  -816,   254,   250,   339,  -816,   270,  -816,
     245,   237,  -816,   985,  -816,   255,    93,   139,  -816,  -816,
     296,   239,   241,   269,  -816,  -816,     9,   271,     9,   985,
     122,   362,   272,   140,   251,   985,  -816,  -816,   985,   985,
      69,   122,   281,   149,  -816,   122,   253,   273,   -85,   122,
     284,   285,   150,   -76,  -816,   122,   122,   287,  -816,   155,
     280,   -35,   985,   122,   288,   299,   291,  -816,   985,   985,
     985,   587,  -816,  -816,  -816,  -816,   286,   289,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,     8,    72,   207,  -816,  1497,   354,   293,   369,
     294,   297,   300,  -816,  -816,   283,   307,  1156,  -816,  -816,
     378,   298,   309,   303,   343,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,   320,   322,
    -816,   122,  -816,  -816,  -816,   122,  -816,  -816,  -816,  -816,
    -816,   324,   245,   295,  -816,   254,   308,   311,   -64,  -816,
     313,  -816,  1497,   314,  -816,   587,   587,   587,   587,   245,
    -816,   122,   122,   315,   316,  -816,  -816,    -2,    72,  -816,
     414,  -816,  -816,  1229,  -816,   985,  -816,   587,   587,   -24,
    1497,  1497,  -816,   -13,   407,   985,   587,   587,   345,     9,
     985,   122,    22,  -816,  -816,  -816,  -816,   587,   587,   985,
     122,    40,   122,  -816,   447,   434,   587,   587,   -98,   122,
      83,  1285,  -816,  -816,  -816,  -816,   985,   325,   325,   644,
     326,  -816,  -816,    36,  -816,  -816,   341,   342,   341,   341,
    -816,  -816,   985,   985,   363,   985,  -816,  -816,   985,   985,
     985,  -816,  -816,  -816,   985,  -816,  -816,  -816,   985,   985,
     985,   985,   985,   985,   985,   327,  -816,   346,   329,  -816,
    -816,   122,  -816,   931,   985,   332,  -816,   348,  -816,  -816,
    -816,   334,  -816,  -816,   245,    20,   335,    68,  -816,   466,
    -816,   245,  -816,  -816,   355,  -816,  -816,  -816,   587,   338,
     337,   349,   347,  -816,   351,   352,   122,   122,     9,  -816,
     985,  -816,  1349,   353,   358,   356,   370,   371,  -816,   359,
    -816,  1497,   364,   360,   122,   372,   -92,  1497,   366,   587,
     373,   245,    90,   367,   368,  1376,   375,   587,   377,   245,
      92,   386,  -816,   477,   985,   382,   398,   384,   385,   404,
     405,   392,   587,   409,   245,   147,  -816,  1005,  -816,  -816,
     122,  -816,   396,   476,   476,  -816,   476,  1497,  1497,    41,
     400,  -816,   268,   268,   268,   558,   558,   108,   108,   325,
     325,   325,   106,   962,  -816,  -816,  -816,   397,  -816,  1407,
    1497,  -816,  -816,  -816,   402,    68,  -816,   418,   432,  -816,
    -816,  -816,  -816,   -60,  -816,   137,   -19,   410,   415,   403,
     406,   412,  -816,  -816,  -816,  -816,   214,  -816,   420,   422,
    -816,  1199,   529,   985,  -816,  -816,   439,   122,   122,  -816,
    -816,  -816,  -816,   985,   236,  -816,   985,    22,   417,  -816,
    -816,   587,   423,  -816,  -816,   985,    40,   424,  -816,  -816,
     587,   425,   122,   496,   341,  -816,  1497,   222,   433,  -816,
    -816,   122,   122,    83,   540,  -816,   431,  -816,   587,   409,
     245,  -816,   435,   587,   451,    21,  -816,    21,   122,    83,
     985,  -816,   985,  -816,  -816,  -816,    10,   452,    63,   -60,
     -60,   227,  -816,  -816,  -816,  -816,  -816,   240,  -816,  -816,
    -816,  -816,   440,  -816,  -816,  -816,  -816,  -816,   188,  -816,
     245,  1439,   453,   441,   443,  1035,   299,   444,    80,   245,
    -816,  -816,   445,  -816,  1497,   245,  -816,  -816,   449,  -816,
     553,   463,   544,   476,    -9,  -816,  -816,  -816,  -816,    23,
     450,   180,  -816,  -816,   454,   455,   245,  -816,   607,  -816,
     540,   457,  -816,   497,   464,  -816,   985,   985,  -816,  -816,
     465,   245,   468,   462,   -82,  1497,  -816,  -816,  -816,   240,
     469,  -816,   471,   471,  -816,   -83,   299,  -816,  -816,   470,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,   490,   122,
    -816,  -816,  -816,  -816,   236,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,   985,  -816,  -816,   584,  -816,   985,  -816,
     491,  -816,  -816,   527,   475,  -816,   222,   985,  -816,  -816,
    -816,  -816,   985,   586,   478,  -816,   607,  -816,   122,  -816,
    1497,  1497,    83,  -816,   122,   582,  -816,  -816,  -816,   500,
    -816,  -816,   245,  -816,   483,   484,  -816,  -816,  1469,   501,
    -816,  1497,  -816,    45,  -816,  -816,  -816,  1497,     6,  1497,
    -816,  -816,   486,  -816,   583,   245,   492,   488,  -816,   498,
    -816,  -816,  -816,   985,  -816,   616,   525,   499,   554,   520,
     508,   515,  -816,  -816,   -33,   985,  -816,  -816,   533,  -816,
     122,   519,   596,   541,  1114,   577,   580,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,   642,   122,  -816,   -79,   557,   122,   649,
    1497,  -816,   622,   552,  -816,  -816,   556,   985,   985,   576,
    -816,  -816,   606,   559,    28,  -816,  -816,   578,  -816,  -816,
     560,   -79,   588,   696,  -816,  1497,  1497,   321,  -816,  -816,
      28,  -816,  -816,  -816,   670,  -816,   578,   594,  -816,  -816,
    -816,  -816,   673,   600,  -816,  -816,   589,  -816,   603,  -816,
     587,  -816,   590,  -816
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,    16,     0,     0,     0,     0,     0,     0,
       0,   132,    64,     3,    15,     4,     5,     6,     9,     0,
       7,   145,     8,   145,    11,    10,    12,     0,    13,   150,
      14,     0,     0,     0,    66,   150,     0,     0,   346,     0,
      23,     0,     0,    20,   135,     0,     0,     0,     0,   144,
       0,     0,     0,     0,   150,     0,    42,   150,    57,   139,
       0,    67,     0,     0,     0,     0,    25,    21,    22,   369,
       0,     0,    18,    19,     0,   363,     0,   133,    68,   331,
       0,   335,   336,   366,     0,     0,     0,     0,   146,     0,
       0,    36,     0,    56,   139,     0,   142,   150,    59,   140,
       0,     0,   405,     0,   309,     0,     0,     0,   150,   310,
       0,     0,     0,     0,   404,   311,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   400,   402,     0,     0,
       0,     0,     0,     0,   312,     0,     0,     0,     0,   261,
       0,   364,     0,     0,   267,     0,     0,     0,   403,     0,
       0,     0,     0,     0,   380,   399,   346,   406,     0,     0,
       0,   369,   196,   197,   198,   199,     0,     0,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   235,   171,   174,   175,   176,
     200,   177,   178,   179,   181,   180,   182,   183,   184,   185,
     173,   172,   186,   187,   188,   189,   191,   190,   192,   193,
     194,   195,     0,     0,   407,   401,   237,     0,     0,   213,
       0,     0,     0,   371,   407,     0,   368,   370,   348,   347,
       0,     0,   138,     0,    70,   350,   351,   359,   352,   353,
     354,   355,   356,   357,   358,   360,   361,   362,     0,     0,
     333,     0,   338,   367,   345,   332,    26,    28,   148,   150,
      31,     0,     0,   384,   150,   139,     0,     0,     0,   145,
       0,   153,   287,   209,   286,   369,   369,   369,   369,     0,
     284,     0,     0,     0,     0,   256,   255,   339,   340,   343,
       0,   257,   340,     0,   227,     0,   259,   369,   369,     0,
     239,   238,   417,     0,   265,     0,   369,   369,   253,     0,
       0,     0,     0,   262,   258,   365,   313,   369,   369,     0,
       0,     0,   269,    74,   100,     0,   369,   369,     0,     0,
       0,     0,   314,   381,   378,   379,     0,   423,   424,     0,
       0,   151,   152,     0,   220,   228,     0,     0,     0,     0,
     229,   230,     0,     0,     0,     0,   448,   449,     0,     0,
       0,   445,   446,   447,     0,   443,   442,   444,     0,     0,
       0,     0,     0,     0,     0,     0,    29,     0,     0,    17,
      24,     0,   349,     0,     0,     0,    32,     0,   136,    71,
      69,     0,   334,   337,     0,    46,     0,     0,   383,    37,
     143,     0,    58,   150,     0,    54,   141,    65,   369,     0,
       0,     0,     0,   329,     0,     0,     0,     0,     0,   342,
       0,   150,     0,     0,     0,     0,     0,     0,   330,     0,
     264,   260,     0,     0,     0,     0,   407,     0,     0,   369,
       0,     0,   317,     0,     0,     0,     0,   369,     0,     0,
     324,   271,   270,    79,     0,     0,     0,     0,     0,     0,
       0,     0,   369,   301,     0,   296,   150,     0,   441,   409,
       0,   203,   211,   215,   215,   208,   215,   231,   232,     0,
     346,   236,   415,   420,   416,   422,   421,   410,   411,   413,
     412,   414,   418,   419,    30,   214,    27,     0,   374,   373,
     372,    33,   137,   134,     0,     0,    45,     0,    48,    34,
     388,   387,   385,     0,   382,   386,   396,     0,     0,     0,
      61,     0,   436,   437,   438,   439,   408,   226,     0,     0,
     341,   246,   290,     0,   434,   435,     0,     0,     0,   266,
     430,   431,   254,     0,     0,   427,     0,     0,     0,   150,
     315,   369,     0,   428,   429,     0,     0,     0,   150,   322,
     369,     0,     0,   278,     0,    77,   101,     0,     0,   432,
     433,     0,     0,     0,   303,   302,     0,   292,   369,   301,
       0,   440,     0,   369,     0,   217,   207,   217,     0,     0,
       0,   408,     0,   147,    44,    47,   396,   366,     0,     0,
       0,   396,    39,    40,    41,   397,   398,     0,    35,    53,
      60,   150,     0,   210,   285,   344,   248,   247,     0,   150,
       0,     0,     0,     0,     0,     0,     0,     0,   418,     0,
     150,   319,     0,   150,   234,     0,   150,   326,     0,   150,
       0,     0,   282,   215,     0,   110,   109,   107,   108,     0,
       0,     0,   105,   111,     0,     0,     0,   304,   306,   150,
     303,     0,   289,     0,     0,   216,     0,     0,   201,   202,
       0,     0,     0,     0,   407,   375,    51,    50,    52,     0,
       0,   392,   389,   390,   391,     0,   376,    38,   377,     0,
      55,   251,   249,   252,   250,   150,   291,   288,     0,     0,
     222,   223,   233,   426,     0,   316,   318,   150,   321,   323,
     325,   150,   328,     0,   279,   283,   280,    80,     0,    81,
       0,    75,    78,     0,     0,   102,     0,     0,   113,   224,
     225,   294,     0,     0,     0,   298,   306,   150,     0,   212,
     219,   218,     0,   293,     0,     0,    49,    43,   394,     0,
     393,    62,     0,   263,     0,     0,   320,   327,     0,     0,
     268,    82,    83,     0,    99,   103,   106,   104,     0,   305,
     307,   150,     0,   300,     0,     0,   243,     0,   241,     0,
     245,   221,   425,     0,   281,     0,     0,     0,     0,     0,
       0,     0,   116,   117,     0,     0,   297,   150,     0,   295,
       0,     0,     0,     0,     0,     0,     0,    93,    97,    96,
      94,    98,    95,    89,    88,    90,    91,    92,    87,    85,
      86,   115,   112,     0,     0,   114,     0,     0,     0,     0,
     308,   299,     0,     0,   240,   242,     0,     0,     0,     0,
      76,    76,     0,     0,     0,   123,   124,   127,   122,   131,
       0,     0,     0,     0,   395,   274,   273,   275,    73,    72,
       0,   125,   121,   128,   129,   126,   127,   205,   244,   276,
     277,   272,     0,     0,   119,   120,     0,   204,     0,   130,
     369,   118,     0,   206
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,   684,  -816,  -816,  -816,  -816,  -108,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,    99,  -816,    24,  -816,
    -816,  -816,  -816,  -816,  -816,  -102,  -815,  -816,  -113,  -816,
    -816,  -816,  -816,  -816,  -816,   -91,  -816,  -816,   -22,  -816,
    -816,  -816,   -20,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -343,  -816,  -466,   165,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,   639,  -816,  -816,  -816,   408,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,  -816,
    -816,  -816,  -816,  -816,  -564,  -816,   177,    97,  -816,    25,
    -816,  -816,  -816,  -816,   212,  -816,  -816,   204,  -816,  -816,
    -816,   633,   680,    -4,  -816,   647,  -816,  -114,   -46,   267,
     102,  -816,   169,    -5,  -816,    89,  -525,  -816,  -816,   264,
    -481,  -816,  -816,  -506,   -16,   333,  -816
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    54,    55,   517,    25,    56,   508,
     597,    26,    57,    58,    98,   268,   612,    27,    28,    29,
      30,    31,   390,   163,   453,   763,   858,   644,   565,   722,
     786,   819,   820,   164,   455,   649,   650,   651,   652,   165,
     728,   768,   791,   792,   793,   847,   848,   794,   864,   874,
     829,    45,    77,    74,   233,   100,   269,   265,    48,    49,
      88,    89,    59,    60,   166,   167,   168,   169,   170,   877,
     171,   172,   274,   473,   378,   585,   668,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   672,   776,   801,   187,   618,   695,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   430,   198,   322,
     451,   563,   871,   642,   760,   716,   199,   200,   201,   202,
     203,   204,   620,   205,   464,   465,   576,   658,   732,   734,
     795,   206,   207,   208,   441,   442,   209,   449,   450,   210,
     211,    47,    79,    80,    81,   286,   287,   213,   224,    83,
      78,   316,   254,   225,   226,   687,   215,   334,   399,   514,
     515,   684,   750,   607,   227,   373,   374
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      82,    50,   288,   262,   292,   475,   476,   787,   586,   656,
     587,   826,   343,   217,   214,    64,   344,   345,   718,   627,
     602,   404,    75,    75,   425,   671,   827,   230,   284,   862,
     844,   228,   598,    82,    94,   252,   319,    96,    82,   284,
     439,    38,   788,   459,   216,   872,   785,   253,   666,   676,
      75,   544,   505,   506,    38,   748,   212,  -336,   447,   723,
     845,  -336,   846,    38,    69,   545,    70,   346,   347,   348,
     289,   311,   289,   749,    82,   249,  -336,   266,   510,   511,
     320,   -84,   688,    36,    34,    82,    37,   272,   279,   308,
     679,   405,    82,   313,   603,   685,   513,    82,   719,   323,
     324,   462,   440,   293,    38,    82,   720,   332,   551,   300,
     560,    35,   301,   302,   789,    38,   294,   426,   682,   683,
     448,   329,   604,   677,   218,    76,   828,   304,    84,   605,
     606,   349,   350,   351,   312,   507,   331,    38,   231,   321,
     229,   418,   337,   338,   339,   790,   721,   330,    38,    85,
      86,   678,    90,   232,   688,   -84,   340,   667,   605,   606,
     645,   646,   647,   463,   648,   578,   220,   845,    41,   846,
     552,    51,   561,   419,   401,    38,   234,   717,   775,   755,
      38,    52,   -84,   -84,   -84,   -84,   -84,    38,   -84,    44,
      39,   -84,   470,   -84,   -84,   435,    32,   588,   352,   353,
     -84,    33,   270,   -84,    61,   393,   510,   511,   691,    82,
      46,  -336,   692,   599,   600,  -336,  -336,   601,    62,   512,
     681,   643,   693,   704,   513,   694,    63,   579,   368,   369,
     370,   371,    53,    38,   372,   414,    82,   251,    65,   394,
      38,   289,    68,    42,   400,    71,    69,   406,    70,   275,
      66,   249,   276,   427,   368,   369,   370,   371,   370,   371,
     372,    38,   372,   289,   436,    82,  -336,  -336,  -336,    67,
     409,   410,   411,   412,    82,    40,    82,    43,   415,   422,
      73,    75,   460,    82,    69,    72,    70,   599,   600,   431,
      87,   601,   423,   424,   437,   277,   297,   471,   278,   298,
      91,   432,   433,   445,   530,   306,   317,   438,   307,   318,
      92,   326,   443,   444,   327,    95,   446,    93,   452,    97,
     467,   457,   458,   726,    99,   461,   727,   397,   398,   219,
    -336,  -336,  -336,  -343,  -343,   497,   477,   478,   216,   482,
    -344,  -344,   483,   484,   485,   221,  -336,   248,   486,   250,
    -336,   256,   487,   488,   489,   490,   491,   492,   493,   645,
     646,   647,   258,   648,   396,   354,   257,   499,   500,   102,
      82,   529,   289,   154,   626,   605,   606,   154,   686,   259,
     106,   413,   107,   519,   869,   870,   260,   261,   542,   263,
     264,   267,   271,   222,   273,   281,   280,   282,   361,   362,
     363,   532,   114,   521,   531,   428,   295,   299,   283,   309,
     290,   296,   528,   365,   366,   367,   368,   369,   370,   371,
     305,   123,   372,   314,    82,   315,   325,   375,   333,   310,
     125,   126,   127,    82,   548,   128,   328,   129,   566,   335,
     377,   341,   557,   382,   342,   133,   580,   336,   376,   379,
     383,   385,   380,   386,   137,   387,   381,   574,   388,   389,
     391,   142,   392,   395,   420,   402,   582,   403,   407,   429,
     408,   416,   417,   434,   454,   589,   148,   149,   456,   372,
     472,   474,   494,   479,   496,   495,   469,   501,   502,   503,
     509,    82,    82,   516,   520,   522,   504,   523,   539,   536,
     154,   155,   480,   518,   157,   562,   524,   525,   526,   527,
     534,   158,   159,   537,   538,   543,   640,   621,   535,   160,
     541,   540,   161,   547,   553,    82,    82,   625,   554,   631,
     628,   549,   556,   623,   624,   558,   564,   567,   637,   634,
     568,   569,    82,   550,   674,   570,   632,   571,   572,   573,
     575,   559,   583,   584,   591,   638,   590,   593,   595,   596,
     610,   806,   611,   355,   619,   608,   577,   654,   655,   613,
     609,   356,   357,   660,   467,   630,   675,   614,   664,   615,
     622,   633,   636,   639,   670,   641,   673,   657,   653,   659,
     665,   689,   663,   102,   253,   690,   699,   713,   700,   696,
     701,   703,   714,   707,   106,   725,   107,   711,   358,   715,
     706,   729,   730,   708,   733,   737,   710,   222,   738,   712,
     745,   739,   742,   744,   747,   601,   114,   751,   753,   759,
     765,   762,   764,   770,   778,   807,   771,   359,   779,   735,
     781,   782,   798,   784,   797,   123,   802,   800,   803,   355,
     740,   741,   805,    82,   821,   126,   127,   356,   357,   823,
     822,   129,   808,   809,   810,   811,   812,   824,   813,   133,
     825,   814,   832,   815,   816,   752,   834,   835,   137,   836,
     817,   840,   662,   818,   841,   142,   842,   756,   361,   362,
     363,   757,    82,   851,   358,   754,   849,   758,    82,   852,
     148,   149,   761,   365,   366,   367,   368,   369,   370,   371,
     853,   767,   372,   854,   857,   860,   769,   773,   863,   861,
     865,   868,   697,   359,   154,   155,   156,   867,   157,   873,
     876,   705,   878,   859,   774,   158,   159,   709,   223,   879,
     777,   360,   881,   160,   162,   880,   161,   883,   724,   866,
     766,   796,   669,   875,   833,   285,   661,   736,   731,   629,
     635,   772,   481,   303,   255,   291,   680,   804,   746,   594,
     546,     0,     0,   743,   361,   362,   363,   831,    82,   830,
       0,     0,    82,     0,     0,     0,     0,     0,   364,   365,
     366,   367,   368,   369,   370,   371,     0,     0,   372,     0,
       0,   468,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     843,   855,   856,     0,   850,     0,     0,     0,   101,     0,
       0,     0,     0,   102,     0,     0,     0,     0,   103,     0,
       0,     0,   104,   105,   106,  -149,   107,     0,   108,     0,
     109,     0,     0,     0,   780,     0,   110,   111,   112,     0,
       0,     0,  -149,  -149,     0,   113,   114,   115,   116,   117,
     118,     0,     0,     0,   119,   882,     0,   799,  -149,     0,
       0,     0,   120,   121,   122,   123,     0,     0,     0,     0,
       0,     0,     0,   124,   125,   126,   127,  -149,     0,   128,
       0,   129,   130,   131,     0,   132,     0,  -149,     0,   133,
       0,     0,     0,     0,     0,   134,   135,   136,   137,   138,
       0,   139,   140,   141,     0,   142,   143,     0,     0,     0,
     144,   145,   146,   147,     0,     0,     0,   102,     0,     0,
     148,   149,    11,     0,     0,   150,     0,   151,   106,   152,
     107,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,   222,     0,     0,   154,   155,   156,   355,   157,     0,
     114,     0,     0,     0,     0,   158,   159,     0,     0,     0,
       0,     0,     0,   160,  -149,     0,   161,     0,     0,   123,
       0,   102,     0,     0,     0,     0,     0,     0,     0,   126,
     127,     0,   106,     0,   107,   129,     0,     0,     0,     0,
     355,     0,   358,   133,     0,   222,     0,     0,   356,   357,
       0,     0,   137,     0,   114,     0,     0,     0,     0,   142,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     355,   359,     0,   123,   148,   149,     0,     0,   356,   357,
       0,     0,     0,   126,   127,   358,     0,     0,     0,   129,
       0,     0,     0,     0,     0,     0,     0,   133,   154,   155,
     156,     0,   157,     0,     0,     0,   137,     0,     0,   158,
     159,     0,   498,   142,   359,   358,     0,   160,     0,     0,
     161,     0,   361,   362,   363,     0,     0,     0,   148,   149,
       0,     0,   360,     0,     0,     0,     0,   365,   366,   367,
     368,   369,   370,   371,   359,     0,   372,     0,     0,   355,
       0,     0,   154,   155,   156,     0,   157,   356,   357,     0,
       0,     0,   360,   158,   159,   361,   362,   363,     0,     0,
       0,   160,     0,     0,   161,     0,     0,     0,     0,   364,
     365,   366,   367,   368,   369,   370,   371,     0,     0,   372,
       0,   355,   581,     0,   358,   361,   362,   363,     0,   356,
     357,     0,     0,     0,     0,     0,     0,     0,     0,   364,
     365,   366,   367,   368,   369,   370,   371,     0,     0,   372,
       0,     0,   702,   359,     0,     0,     0,     0,   837,     0,
       0,     0,     0,     0,   355,     0,   358,     0,     0,     0,
       0,   360,   356,   357,     0,   838,   839,     0,     0,     0,
       0,     0,     0,     0,     0,   616,     0,     0,     0,     0,
       0,     0,     0,     0,   355,   359,     0,     0,     0,     0,
       0,     0,   356,   357,   361,   362,   363,     0,     0,   358,
       0,     0,   617,   360,     0,     0,     0,     0,   364,   365,
     366,   367,   368,   369,   370,   371,     0,     0,   372,     2,
       3,     0,     4,     0,     0,     0,     0,     0,   359,   358,
       0,   -63,     0,     0,     0,     0,   361,   362,   363,   384,
     355,   -63,     0,     0,     0,     0,   360,     5,   356,   357,
     364,   365,   366,   367,   368,   369,   370,   371,   359,     0,
     372,     0,     0,     0,     0,     6,     0,     0,     7,   466,
       0,     0,     0,     0,     0,     0,   360,     0,     0,   361,
     362,   363,     0,     0,     0,   358,     0,     0,     0,     0,
     421,     0,   -63,   364,   365,   366,   367,   368,   369,   370,
     371,     0,     8,   372,   355,     9,    10,     0,     0,   361,
     362,   363,   356,   357,   359,     0,     0,     0,     0,     0,
       0,     0,     0,   364,   365,   366,   367,   368,   369,   370,
     371,   355,   360,   372,    11,     0,    12,     0,     0,   356,
     357,     0,     0,     0,     0,     0,     0,     0,     0,   358,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   355,     0,     0,   361,   362,   363,     0,     0,
     356,   357,     0,   555,     0,     0,   358,     0,   359,   364,
     365,   366,   367,   368,   369,   370,   371,     0,     0,   372,
       0,     0,     0,     0,   355,     0,   360,     0,     0,     0,
       0,     0,   356,   357,     0,   359,     0,   358,     0,     0,
       0,   533,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   360,   355,     0,     0,     0,     0,   361,
     362,   363,   356,   357,     0,     0,   359,     0,     0,   358,
       0,     0,     0,   364,   365,   366,   367,   368,   369,   370,
     371,     0,   355,   372,   360,     0,   361,   362,   363,     0,
     356,   357,     0,     0,     0,     0,     0,     0,   359,   358,
     364,   365,   366,   367,   368,   369,   370,   371,     0,     0,
     372,     0,     0,     0,     0,     0,   360,   361,   362,   363,
     592,   698,     0,     0,     0,     0,     0,   358,   359,     0,
       0,   364,   365,   366,   367,   368,   369,   370,   371,     0,
       0,   372,     0,     0,     0,     0,   360,     0,     0,   361,
     362,   363,     0,     0,     0,     0,   359,     0,     0,     0,
       0,   783,     0,   364,   365,   366,   367,   368,   369,   370,
     371,     0,     0,   372,   360,     0,     0,     0,     0,   361,
     362,   363,     0,     0,     0,   235,     0,     0,     0,     0,
       0,     0,     0,   364,   365,   366,   367,   368,   369,   370,
     371,     0,     0,   372,     0,     0,   236,   361,   362,   363,
     237,   238,     0,     0,     0,     0,     0,     0,   239,     0,
       0,   364,   365,   366,   367,   368,   369,   370,   371,     0,
       0,   372,     0,     0,   240,     0,     0,     0,     0,   241,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   242,     0,     0,     0,
       0,     0,     0,   243,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   244,
       0,     0,     0,     0,   245,     0,   246,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   247
};

static const yytype_int16 yycheck[] =
{
      46,    23,   116,    94,   118,   348,   349,     1,   474,   573,
     476,    44,     4,    28,    60,    35,     8,     9,    27,   544,
      39,    85,    36,    36,    48,   589,    59,    28,    30,   844,
     109,    30,   513,    79,    54,    81,   112,    57,    84,    30,
      18,   139,    36,   141,    60,   860,     1,   142,    27,    39,
      36,   143,    32,    33,   139,   138,    60,   139,    18,    36,
     139,   143,   141,   139,   159,   157,   161,    59,    60,    61,
     116,   156,   118,   156,   120,    79,   158,    97,   138,   139,
     156,    36,   607,   139,   139,   131,   142,   103,   108,   135,
     596,   155,   138,   139,   113,   601,   156,   143,   107,   145,
     146,    18,    80,   119,   139,   151,   115,   153,    18,   125,
      18,   155,   128,   129,   108,   139,   120,   141,   599,   600,
      80,   156,   141,   113,   139,   139,   159,   131,   141,   148,
     149,   123,   124,   125,   138,   115,   152,   139,   139,   143,
     139,   143,   158,   159,   160,   139,   155,   151,   139,    47,
      48,   141,    50,   139,   679,   110,   161,   136,   148,   149,
     137,   138,   139,    80,   141,    18,    64,   139,    70,   141,
      80,    12,    80,   287,   265,   139,    74,   643,   742,   704,
     139,    22,   137,   138,   139,   140,   141,   139,   143,   116,
     142,   146,   156,   148,   149,   309,    70,   156,   126,   127,
     155,    75,   100,   158,   155,   251,   138,   139,    20,   255,
     141,     4,    24,   150,   151,     8,     9,   154,    50,   151,
     157,   564,    34,   143,   156,    37,    50,    80,   148,   149,
     150,   151,    73,   139,   154,   281,   282,   143,    44,   259,
     139,   287,   155,   142,   264,    50,   159,   269,   161,   156,
     155,   255,   159,   299,   148,   149,   150,   151,   150,   151,
     154,   139,   154,   309,   310,   311,    59,    60,    61,   155,
     275,   276,   277,   278,   320,     8,   322,    10,   282,   295,
     155,    36,   328,   329,   159,   155,   161,   150,   151,   305,
      71,   154,   297,   298,   310,   156,   156,   343,   159,   159,
     139,   306,   307,   319,   418,   156,   156,   311,   159,   159,
     139,   156,   317,   318,   159,    85,   320,   139,   322,   156,
     336,   326,   327,   143,    70,   329,   146,    32,    33,   139,
     123,   124,   125,   126,   127,   381,   352,   353,   354,   355,
     126,   127,   358,   359,   360,   142,   139,   146,   364,   140,
     143,   155,   368,   369,   370,   371,   372,   373,   374,   137,
     138,   139,   139,   141,   262,   158,   155,   383,   384,     6,
     416,   417,   418,   137,   138,   148,   149,   137,   138,   155,
      17,   279,    19,   403,    63,    64,   155,    85,   434,   139,
      51,   121,   155,    30,   139,   156,   100,   156,   130,   131,
     132,   421,    39,   408,   420,   303,    44,   156,   139,   156,
     139,   139,   416,   145,   146,   147,   148,   149,   150,   151,
     139,    58,   154,   139,   470,   140,   139,    73,   140,   156,
      67,    68,    69,   479,   439,    72,   156,    74,   454,   140,
      71,   155,   447,   160,   155,    82,   466,   156,   155,   155,
     143,    73,   155,   155,    91,   146,   156,   462,   155,   116,
     140,    98,   140,   139,    50,   157,   470,   156,   155,    62,
     156,   156,   156,   128,    27,   479,   113,   114,    44,   154,
     139,   139,   155,   120,   155,   139,   160,   155,   140,   155,
     155,   537,   538,    27,   139,   157,   394,   160,   139,   143,
     137,   138,   139,   401,   141,   119,   157,   160,   157,   157,
     157,   148,   149,   143,   143,   143,   562,   533,   160,   156,
     160,   157,   159,   157,   157,   571,   572,   543,   160,   549,
     546,   158,   157,   537,   538,   158,    59,   155,   558,   555,
     142,   157,   588,   441,   590,   160,   551,   143,   143,   157,
     141,   449,   156,    77,   157,   560,   156,   155,   140,   127,
     157,    36,   156,     5,    35,   155,   464,   571,   572,   157,
     155,    13,    14,   578,   590,   158,   592,   157,   583,   157,
     141,   158,   158,   158,   588,    89,   590,    47,   155,   158,
     139,   611,   157,     6,   142,   155,   143,    44,   157,   619,
     157,   157,   139,   158,    17,   155,    19,   158,    50,    65,
     630,   157,   157,   633,     7,   158,   636,    30,   121,   639,
     158,   157,   157,   155,   155,   154,    39,   157,   138,    45,
     155,   140,   105,    47,    52,   110,   158,    79,   138,   659,
     157,   157,    59,   142,   158,    58,   158,   155,   150,     5,
     666,   667,    36,   699,   155,    68,    69,    13,    14,   139,
     106,    74,   137,   138,   139,   140,   141,   159,   143,    82,
     155,   146,   139,   148,   149,   695,   157,    81,    91,   138,
     155,   104,   580,   158,   104,    98,    44,   707,   130,   131,
     132,   711,   738,    44,    50,   699,   139,   713,   744,    77,
     113,   114,   718,   145,   146,   147,   148,   149,   150,   151,
     158,   727,   154,   157,   138,   109,   732,   737,   140,   160,
     160,    25,   620,    79,   137,   138,   139,   139,   141,    59,
     136,   629,    59,   841,   738,   148,   149,   635,   151,   139,
     744,    97,   139,   156,    60,   156,   159,   157,   649,   851,
     726,   771,   587,   866,   800,   116,   579,   660,   656,   547,
     556,   736,   354,   130,    84,   118,   597,   783,   679,   505,
     437,    -1,    -1,   671,   130,   131,   132,   797,   824,   795,
      -1,    -1,   828,    -1,    -1,    -1,    -1,    -1,   144,   145,
     146,   147,   148,   149,   150,   151,    -1,    -1,   154,    -1,
      -1,   157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     824,   837,   838,    -1,   828,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,     6,    -1,    -1,    -1,    -1,    11,    -1,
      -1,    -1,    15,    16,    17,    18,    19,    -1,    21,    -1,
      23,    -1,    -1,    -1,   752,    -1,    29,    30,    31,    -1,
      -1,    -1,    35,    36,    -1,    38,    39,    40,    41,    42,
      43,    -1,    -1,    -1,    47,   880,    -1,   775,    51,    -1,
      -1,    -1,    55,    56,    57,    58,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    -1,    72,
      -1,    74,    75,    76,    -1,    78,    -1,    80,    -1,    82,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      -1,    94,    95,    96,    -1,    98,    99,    -1,    -1,    -1,
     103,   104,   105,   106,    -1,    -1,    -1,     6,    -1,    -1,
     113,   114,   115,    -1,    -1,   118,    -1,   120,    17,   122,
      19,    -1,    -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,
      -1,    30,    -1,    -1,   137,   138,   139,     5,   141,    -1,
      39,    -1,    -1,    -1,    -1,   148,   149,    -1,    -1,    -1,
      -1,    -1,    -1,   156,   157,    -1,   159,    -1,    -1,    58,
      -1,     6,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    68,
      69,    -1,    17,    -1,    19,    74,    -1,    -1,    -1,    -1,
       5,    -1,    50,    82,    -1,    30,    -1,    -1,    13,    14,
      -1,    -1,    91,    -1,    39,    -1,    -1,    -1,    -1,    98,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       5,    79,    -1,    58,   113,   114,    -1,    -1,    13,    14,
      -1,    -1,    -1,    68,    69,    50,    -1,    -1,    -1,    74,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,   137,   138,
     139,    -1,   141,    -1,    -1,    -1,    91,    -1,    -1,   148,
     149,    -1,   151,    98,    79,    50,    -1,   156,    -1,    -1,
     159,    -1,   130,   131,   132,    -1,    -1,    -1,   113,   114,
      -1,    -1,    97,    -1,    -1,    -1,    -1,   145,   146,   147,
     148,   149,   150,   151,    79,    -1,   154,    -1,    -1,     5,
      -1,    -1,   137,   138,   139,    -1,   141,    13,    14,    -1,
      -1,    -1,    97,   148,   149,   130,   131,   132,    -1,    -1,
      -1,   156,    -1,    -1,   159,    -1,    -1,    -1,    -1,   144,
     145,   146,   147,   148,   149,   150,   151,    -1,    -1,   154,
      -1,     5,   157,    -1,    50,   130,   131,   132,    -1,    13,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,
     145,   146,   147,   148,   149,   150,   151,    -1,    -1,   154,
      -1,    -1,   157,    79,    -1,    -1,    -1,    -1,    84,    -1,
      -1,    -1,    -1,    -1,     5,    -1,    50,    -1,    -1,    -1,
      -1,    97,    13,    14,    -1,   101,   102,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    13,    14,   130,   131,   132,    -1,    -1,    50,
      -1,    -1,    53,    97,    -1,    -1,    -1,    -1,   144,   145,
     146,   147,   148,   149,   150,   151,    -1,    -1,   154,     0,
       1,    -1,     3,    -1,    -1,    -1,    -1,    -1,    79,    50,
      -1,    12,    -1,    -1,    -1,    -1,   130,   131,   132,   133,
       5,    22,    -1,    -1,    -1,    -1,    97,    28,    13,    14,
     144,   145,   146,   147,   148,   149,   150,   151,    79,    -1,
     154,    -1,    -1,    -1,    -1,    46,    -1,    -1,    49,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    97,    -1,    -1,   130,
     131,   132,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,
     111,    -1,    73,   144,   145,   146,   147,   148,   149,   150,
     151,    -1,    83,   154,     5,    86,    87,    -1,    -1,   130,
     131,   132,    13,    14,    79,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   144,   145,   146,   147,   148,   149,   150,
     151,     5,    97,   154,   115,    -1,   117,    -1,    -1,    13,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    50,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     5,    -1,    -1,   130,   131,   132,    -1,    -1,
      13,    14,    -1,    47,    -1,    -1,    50,    -1,    79,   144,
     145,   146,   147,   148,   149,   150,   151,    -1,    -1,   154,
      -1,    -1,    -1,    -1,     5,    -1,    97,    -1,    -1,    -1,
      -1,    -1,    13,    14,    -1,    79,    -1,    50,    -1,    -1,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    97,     5,    -1,    -1,    -1,    -1,   130,
     131,   132,    13,    14,    -1,    -1,    79,    -1,    -1,    50,
      -1,    -1,    -1,   144,   145,   146,   147,   148,   149,   150,
     151,    -1,     5,   154,    97,    -1,   130,   131,   132,    -1,
      13,    14,    -1,    -1,    -1,    -1,    -1,    -1,    79,    50,
     144,   145,   146,   147,   148,   149,   150,   151,    -1,    -1,
     154,    -1,    -1,    -1,    -1,    -1,    97,   130,   131,   132,
     133,   102,    -1,    -1,    -1,    -1,    -1,    50,    79,    -1,
      -1,   144,   145,   146,   147,   148,   149,   150,   151,    -1,
      -1,   154,    -1,    -1,    -1,    -1,    97,    -1,    -1,   130,
     131,   132,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,
      -1,   112,    -1,   144,   145,   146,   147,   148,   149,   150,
     151,    -1,    -1,   154,    97,    -1,    -1,    -1,    -1,   130,
     131,   132,    -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   144,   145,   146,   147,   148,   149,   150,
     151,    -1,    -1,   154,    -1,    -1,    42,   130,   131,   132,
      46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    54,    -1,
      -1,   144,   145,   146,   147,   148,   149,   150,   151,    -1,
      -1,   154,    -1,    -1,    70,    -1,    -1,    -1,    -1,    75,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    99,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,
      -1,    -1,    -1,    -1,   120,    -1,   122,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   163,     0,     1,     3,    28,    46,    49,    83,    86,
      87,   115,   117,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   179,   183,   189,   190,   191,
     192,   193,    70,    75,   139,   155,   139,   142,   139,   142,
     321,    70,   142,   321,   116,   223,   141,   313,   230,   231,
     230,    12,    22,    73,   176,   177,   180,   184,   185,   234,
     235,   155,    50,    50,   234,    44,   155,   155,   155,   159,
     161,    50,   155,   155,   225,    36,   139,   224,   322,   314,
     315,   316,   320,   321,   141,   322,   322,    71,   232,   233,
     322,   139,   139,   139,   234,    85,   234,   156,   186,    70,
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
     289,   290,   291,   292,   293,   295,   303,   304,   305,   308,
     311,   312,   315,   319,   320,   328,   336,    28,   139,   139,
     322,   142,    30,   151,   320,   325,   326,   336,    30,   139,
      28,   139,   139,   226,   322,    21,    42,    46,    47,    54,
      70,    75,    92,    99,   115,   120,   122,   139,   146,   315,
     140,   143,   320,   142,   324,   314,   155,   155,   139,   155,
     155,    85,   227,   139,    51,   229,   234,   121,   187,   228,
     322,   155,   336,   139,   244,   156,   159,   156,   159,   234,
     100,   156,   156,   139,    30,   257,   317,   318,   319,   320,
     139,   317,   319,   336,   315,    44,   139,   156,   159,   156,
     336,   336,   336,   313,   315,   139,   156,   159,   320,   156,
     156,   156,   315,   320,   139,   140,   323,   156,   159,   112,
     156,   315,   281,   320,   320,   139,   156,   159,   156,   156,
     315,   336,   320,   140,   329,   140,   156,   336,   336,   336,
     325,   155,   155,     4,     8,     9,    59,    60,    61,   123,
     124,   125,   126,   127,   158,     5,    13,    14,    50,    79,
      97,   130,   131,   132,   144,   145,   146,   147,   148,   149,
     150,   151,   154,   337,   338,    73,   155,    71,   246,   155,
     155,   156,   160,   143,   133,    73,   155,   146,   155,   116,
     194,   140,   140,   320,   234,   139,   322,    32,    33,   330,
     234,   227,   157,   156,    85,   155,   230,   155,   156,   325,
     325,   325,   325,   322,   320,   315,   156,   156,   143,   319,
      50,   111,   336,   325,   325,    48,   141,   320,   322,    62,
     279,   336,   325,   325,   128,   319,   320,   336,   315,    18,
      80,   306,   307,   325,   325,   336,   315,    18,    80,   309,
     310,   282,   315,   196,    27,   206,    44,   325,   325,   141,
     320,   315,    18,    80,   296,   297,    34,   336,   157,   160,
     156,   320,   139,   245,   139,   245,   245,   336,   336,   120,
     139,   261,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   155,   139,   155,   320,   151,   336,
     336,   155,   140,   155,   322,    32,    33,   115,   181,   155,
     138,   139,   151,   156,   331,   332,    27,   178,   322,   234,
     139,   325,   157,   160,   157,   160,   157,   157,   315,   320,
     319,   336,   234,   112,   157,   160,   143,   143,   143,   139,
     157,   160,   320,   143,   143,   157,   337,   157,   325,   158,
     322,    18,    80,   157,   160,    47,   157,   325,   158,   322,
      18,    80,   119,   283,    59,   200,   336,   155,   142,   157,
     160,   143,   143,   157,   325,   141,   298,   322,    18,    80,
     234,   157,   315,   156,    77,   247,   247,   247,   156,   315,
     156,   157,   133,   155,   331,   140,   127,   182,   332,   150,
     151,   154,    39,   113,   141,   148,   149,   335,   155,   155,
     157,   156,   188,   157,   157,   157,    26,    53,   267,    35,
     294,   336,   141,   315,   315,   336,   138,   328,   336,   306,
     158,   234,   325,   158,   336,   309,   158,   234,   325,   158,
     320,    89,   285,   245,   199,   137,   138,   139,   141,   207,
     208,   209,   210,   155,   315,   315,   296,    47,   299,   158,
     325,   298,   322,   157,   325,   139,    27,   136,   248,   248,
     315,   296,   263,   315,   320,   336,    39,   113,   141,   335,
     324,   157,   332,   332,   333,   335,   138,   327,   328,   234,
     155,    20,    24,    34,    37,   268,   234,   322,   102,   143,
     157,   157,   157,   157,   143,   322,   234,   158,   234,   322,
     234,   158,   234,    44,   139,    65,   287,   247,    27,   107,
     115,   155,   201,    36,   208,   155,   143,   146,   212,   157,
     157,   322,   300,     7,   301,   234,   299,   158,   121,   157,
     336,   336,   157,   322,   155,   158,   327,   155,   138,   156,
     334,   157,   234,   138,   315,   328,   234,   234,   336,    45,
     286,   336,   140,   197,   105,   155,   210,   336,   213,   336,
      47,   158,   301,   234,   315,   296,   264,   315,    52,   138,
     322,   157,   157,   112,   142,     1,   202,     1,    36,   108,
     139,   214,   215,   216,   219,   302,   234,   158,    59,   322,
     155,   265,   158,   150,   336,    36,    36,   110,   137,   138,
     139,   140,   141,   143,   146,   148,   149,   155,   158,   203,
     204,   155,   106,   139,   159,   155,    44,    59,   159,   222,
     336,   234,   139,   320,   157,    81,   138,    84,   101,   102,
     104,   104,    44,   315,   109,   139,   141,   217,   218,   139,
     315,    44,    77,   158,   157,   336,   336,   138,   198,   198,
     109,   160,   218,   140,   220,   160,   217,   139,    25,    63,
      64,   284,   218,    59,   221,   220,   136,   241,    59,   139,
     156,   139,   325,   157
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
     297,   298,   298,   299,   300,   299,   301,   302,   301,   303,
     303,   303,   303,   303,   304,   305,   305,   306,   307,   307,
     307,   307,   308,   308,   309,   310,   310,   310,   310,   311,
     312,   313,   313,   314,   314,   315,   316,   316,   316,   317,
     318,   318,   318,   319,   319,   320,   321,   321,   321,   321,
     322,   322,   322,   322,   322,   322,   322,   322,   322,   322,
     322,   322,   322,   322,   323,   323,   324,   324,   325,   325,
     326,   326,   326,   326,   326,   326,   327,   327,   328,   328,
     329,   329,   330,   330,   330,   331,   331,   332,   332,   332,
     332,   332,   332,   333,   334,   334,   335,   335,   335,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   337,   337,   337,   337,   337,   337,   338,   338
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
       0,     2,     4,     6,     6,     8,     1,     6,     4,     7,
       5,     0,     1,     0,     0,     3,     0,     0,     4,     1,
       1,     1,     1,     2,     2,     4,     6,     1,     4,     3,
       5,     4,     4,     6,     1,     4,     3,     5,     4,     3,
       3,     2,     3,     2,     3,     1,     1,     3,     2,     1,
       1,     3,     2,     1,     4,     2,     1,     3,     3,     4,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     1,     0,     1,     0,     1,     1,     0,
       1,     1,     3,     3,     3,     5,     1,     1,     2,     2,
       0,     1,     2,     1,     0,     1,     1,     1,     1,     3,
       3,     3,     3,     2,     1,     5,     0,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     3,     3,
       3,     3,     3,     2,     2,     8,     6,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     3,     1,     1,     1,     1,     1,     1,     1,     1
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
#line 909 "ascend/compiler/ascParse.y"
        {
	  /*
	   * The specific syntax diagnostic is already emitted by zz_error().
	   * Emitting a generic message here causes noisy duplicates during
	   * parser recovery.
	   */
	}
#line 2976 "ascend/compiler/ascParse.c"
    break;

  case 17: /* global_def: GLOBAL_TOK ';' fstatements end ';'  */
#line 920 "ascend/compiler/ascParse.y"
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
#line 3022 "ascend/compiler/ascParse.c"
    break;

  case 18: /* require_file: REQUIRE_TOK DQUOTE_TOK ';'  */
#line 965 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerPushBuffer((yyvsp[-1].dquote_ptr));
	}
#line 3030 "ascend/compiler/ascParse.c"
    break;

  case 19: /* require_file: REQUIRE_TOK name ';'  */
#line 969 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 3039 "ascend/compiler/ascParse.c"
    break;

  case 20: /* require_file: REQUIRE_TOK name  */
#line 974 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
#line 3048 "ascend/compiler/ascParse.c"
    break;

  case 21: /* provide_module: PROVIDE_TOK DQUOTE_TOK ';'  */
#line 982 "ascend/compiler/ascParse.y"
        {
	  Asc_ModuleCreateAlias(Asc_CurrentModule(),(yyvsp[-1].dquote_ptr));
	}
#line 3056 "ascend/compiler/ascParse.c"
    break;

  case 22: /* provide_module: PROVIDE_TOK name ';'  */
#line 986 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-1].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 3065 "ascend/compiler/ascParse.c"
    break;

  case 23: /* provide_module: PROVIDE_TOK name  */
#line 991 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[0].nptr));
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
#line 3074 "ascend/compiler/ascParse.c"
    break;

  case 24: /* import: IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'  */
#line 999 "ascend/compiler/ascParse.y"
        {
	  if(package_load((yyvsp[-1].dquote_ptr),SCP((yyvsp[-3].id_ptr)))){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' from '%s'."
	      ,SCP((yyvsp[-1].dquote_ptr)), SCP((yyvsp[-3].id_ptr))
            );
          }
	}
#line 3087 "ascend/compiler/ascParse.c"
    break;

  case 25: /* import: IMPORT_TOK DQUOTE_TOK ';'  */
#line 1008 "ascend/compiler/ascParse.y"
        {
	  if(package_load(SCP((yyvsp[-1].dquote_ptr)),NULL)){
	    error_reporter_current_line(ASC_USER_ERROR
	      ,"IMPORT of '%s' failed."
	      ,SCP((yyvsp[-1].dquote_ptr))
	    );
	  }
	}
#line 3100 "ascend/compiler/ascParse.c"
    break;

  case 26: /* add_notes_def: add_notes_head notes_body end ';'  */
#line 1020 "ascend/compiler/ascParse.y"
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
#line 3132 "ascend/compiler/ascParse.c"
    break;

  case 27: /* add_notes_head: ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'  */
#line 1051 "ascend/compiler/ascParse.y"
        {
	  g_type_name = (yyvsp[-2].id_ptr);
	  g_proc_name = (yyvsp[-1].id_ptr);
	}
#line 3141 "ascend/compiler/ascParse.c"
    break;

  case 28: /* add_method_def: add_method_head proclist end ';'  */
#line 1059 "ascend/compiler/ascParse.y"
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
#line 3165 "ascend/compiler/ascParse.c"
    break;

  case 29: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1082 "ascend/compiler/ascParse.y"
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
#line 3182 "ascend/compiler/ascParse.c"
    break;

  case 30: /* add_method_head: ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1095 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3191 "ascend/compiler/ascParse.c"
    break;

  case 31: /* replace_method_def: replace_method_head proclist end ';'  */
#line 1103 "ascend/compiler/ascParse.y"
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
#line 3213 "ascend/compiler/ascParse.c"
    break;

  case 32: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'  */
#line 1124 "ascend/compiler/ascParse.y"
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
#line 3229 "ascend/compiler/ascParse.c"
    break;

  case 33: /* replace_method_head: REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'  */
#line 1136 "ascend/compiler/ascParse.y"
        {
	  (yyval.tptr) = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
#line 3238 "ascend/compiler/ascParse.c"
    break;

  case 34: /* atom_def: universal atom_head fstatements methods end ';'  */
#line 1144 "ascend/compiler/ascParse.y"
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
#line 3292 "ascend/compiler/ascParse.c"
    break;

  case 35: /* atom_head: atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'  */
#line 1197 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_refines_name = (yyvsp[-3].id_ptr);
	  g_atom_dim_ptr = (yyvsp[-2].dimp);
	  g_default_double = (yyvsp[-1].real_value);
	  g_header_linenum = LineNum();
	}
#line 3304 "ascend/compiler/ascParse.c"
    break;

  case 36: /* atom_id: ATOM_TOK IDENTIFIER_TOK  */
#line 1208 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3313 "ascend/compiler/ascParse.c"
    break;

  case 37: /* default_val: %empty  */
#line 1216 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3324 "ascend/compiler/ascParse.c"
    break;

  case 38: /* default_val: DEFAULT_TOK optional_sign number  */
#line 1223 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3334 "ascend/compiler/ascParse.c"
    break;

  case 39: /* default_val: DEFAULT_TOK FALSE_TOK  */
#line 1229 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 0;
	  g_defaulted = 1;
	}
#line 3346 "ascend/compiler/ascParse.c"
    break;

  case 40: /* default_val: DEFAULT_TOK TRUE_TOK  */
#line 1237 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_long = 1;
	  g_defaulted = 1;
	}
#line 3358 "ascend/compiler/ascParse.c"
    break;

  case 41: /* default_val: DEFAULT_TOK SYMBOL_TOK  */
#line 1245 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_default_symbol = (yyvsp[0].sym_ptr);
	  g_defaulted = 0;
	}
#line 3370 "ascend/compiler/ascParse.c"
    break;

  case 42: /* constant_def: universal constant_head  */
#line 1256 "ascend/compiler/ascParse.y"
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
#line 3411 "ascend/compiler/ascParse.c"
    break;

  case 43: /* constant_head: CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK constant_dims constant_val optional_notes ';'  */
#line 1297 "ascend/compiler/ascParse.y"
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
#line 3445 "ascend/compiler/ascParse.c"
    break;

  case 44: /* constant_dims: DIMENSION_TOK dimensions  */
#line 1330 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	  g_constant_units = NULL;
	}
#line 3454 "ascend/compiler/ascParse.c"
    break;

  case 45: /* constant_dims: DIMENSIONLESS_TOK  */
#line 1335 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	  g_constant_units = NULL;
	}
#line 3463 "ascend/compiler/ascParse.c"
    break;

  case 46: /* constant_dims: %empty  */
#line 1340 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	  g_constant_units = NULL;
	}
#line 3472 "ascend/compiler/ascParse.c"
    break;

  case 47: /* constant_dims: UNITS_TOK BRACEDTEXT_TOK  */
#line 1345 "ascend/compiler/ascParse.y"
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
#line 3494 "ascend/compiler/ascParse.c"
    break;

  case 48: /* constant_val: %empty  */
#line 1366 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_default_units = NULL;
	  g_defaulted = 0;
	}
#line 3505 "ascend/compiler/ascParse.c"
    break;

  case 49: /* constant_val: CASSIGN_TOK optional_sign number  */
#line 1373 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].int_value) ? -(yyvsp[0].real_value) : (yyvsp[0].real_value);
	  g_default_units = g_number_units;
	  g_defaulted = 1;
	}
#line 3515 "ascend/compiler/ascParse.c"
    break;

  case 50: /* constant_val: CASSIGN_TOK TRUE_TOK  */
#line 1379 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3527 "ascend/compiler/ascParse.c"
    break;

  case 51: /* constant_val: CASSIGN_TOK FALSE_TOK  */
#line 1387 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = BOOLEANCONSTANT;
	}
#line 3539 "ascend/compiler/ascParse.c"
    break;

  case 52: /* constant_val: CASSIGN_TOK SYMBOL_TOK  */
#line 1395 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_default_units = NULL;
	  g_constant_type = SYMBOLCONSTANT;
	}
#line 3551 "ascend/compiler/ascParse.c"
    break;

  case 53: /* model_def: universal model_head fstatements initial methods end ';'  */
#line 1406 "ascend/compiler/ascParse.y"
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
#line 3594 "ascend/compiler/ascParse.c"
    break;

  case 54: /* model_head: model_id optional_model_parameters optional_parameter_wheres ';'  */
#line 1449 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-2].slptr);
	  g_parameter_wheres = (yyvsp[-1].slptr);
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
#line 3606 "ascend/compiler/ascParse.c"
    break;

  case 55: /* model_head: model_id optional_model_parameters optional_parameter_wheres REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'  */
#line 1458 "ascend/compiler/ascParse.y"
        {
	  /* g_type_name = $1; */
	  g_model_parameters = (yyvsp[-5].slptr);
	  g_parameter_wheres = (yyvsp[-4].slptr);
	  g_refines_name = (yyvsp[-2].id_ptr);
	  g_parameter_reduction = (yyvsp[-1].slptr);
	  g_header_linenum = LineNum();
	}
#line 3619 "ascend/compiler/ascParse.c"
    break;

  case 56: /* model_id: MODEL_TOK IDENTIFIER_TOK  */
#line 1470 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3628 "ascend/compiler/ascParse.c"
    break;

  case 57: /* optional_model_parameters: %empty  */
#line 1478 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3636 "ascend/compiler/ascParse.c"
    break;

  case 58: /* optional_model_parameters: '(' fstatements ')'  */
#line 1482 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3644 "ascend/compiler/ascParse.c"
    break;

  case 59: /* optional_parameter_wheres: %empty  */
#line 1489 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3652 "ascend/compiler/ascParse.c"
    break;

  case 60: /* optional_parameter_wheres: WHERE_TOK '(' fstatements ')'  */
#line 1493 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3660 "ascend/compiler/ascParse.c"
    break;

  case 61: /* optional_parameter_reduction: %empty  */
#line 1500 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 3668 "ascend/compiler/ascParse.c"
    break;

  case 62: /* optional_parameter_reduction: '(' fstatements ')'  */
#line 1504 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[-1].slptr); /* this could be much more sophisticated */
	}
#line 3676 "ascend/compiler/ascParse.c"
    break;

  case 63: /* universal: %empty  */
#line 1554 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3684 "ascend/compiler/ascParse.c"
    break;

  case 64: /* universal: UNIVERSAL_TOK  */
#line 1558 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3692 "ascend/compiler/ascParse.c"
    break;

  case 65: /* definition_def: definition_id fstatements methods end ';'  */
#line 1565 "ascend/compiler/ascParse.y"
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
#line 3723 "ascend/compiler/ascParse.c"
    break;

  case 66: /* definition_id: DEFINITION_TOK IDENTIFIER_TOK  */
#line 1595 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_type_name = (yyvsp[0].id_ptr); /* want this set early so parm lists see it */
	}
#line 3732 "ascend/compiler/ascParse.c"
    break;

  case 67: /* units_def: units_statement ';'  */
#line 1604 "ascend/compiler/ascParse.y"
        { /* nothing to do. just cruft to fix ; problem */ }
#line 3738 "ascend/compiler/ascParse.c"
    break;

  case 68: /* units_statement: UNITS_TOK unitdeflist end  */
#line 1609 "ascend/compiler/ascParse.y"
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
#line 3759 "ascend/compiler/ascParse.c"
    break;

  case 69: /* units_statement: UNITS_TOK LADDER_TOK unitladderitemlist end optional_ladder_end  */
#line 1626 "ascend/compiler/ascParse.y"
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
#line 3785 "ascend/compiler/ascParse.c"
    break;

  case 70: /* optional_ladder_end: %empty  */
#line 1651 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 3793 "ascend/compiler/ascParse.c"
    break;

  case 71: /* optional_ladder_end: LADDER_TOK  */
#line 1655 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 3801 "ascend/compiler/ascParse.c"
    break;

  case 72: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on table_body END_TOK TABLE_TOK table_mode_off  */
#line 1662 "ascend/compiler/ascParse.y"
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
#line 3826 "ascend/compiler/ascParse.c"
    break;

  case 73: /* table_statement: TABLE_TOK fname table_begin table_decl_opt table_options ';' table_mode_on error END_TOK TABLE_TOK table_mode_off  */
#line 1683 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-9].nptr));
	  TableParseAbort();
	  ErrMsg_Generic("Error in TABLE body.");
	  g_untrapped_error++;
	  yyerrok;
	  (yyval.statptr) = NULL;
	}
#line 3839 "ascend/compiler/ascParse.c"
    break;

  case 74: /* table_begin: %empty  */
#line 1695 "ascend/compiler/ascParse.y"
        {
	  TableParseBegin();
	}
#line 3847 "ascend/compiler/ascParse.c"
    break;

  case 75: /* table_mode_on: %empty  */
#line 1702 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(1);
	}
#line 3855 "ascend/compiler/ascParse.c"
    break;

  case 76: /* table_mode_off: %empty  */
#line 1709 "ascend/compiler/ascParse.y"
        {
	  Asc_ScannerSetTableMode(0);
	}
#line 3863 "ascend/compiler/ascParse.c"
    break;

  case 79: /* table_decl_opt: %empty  */
#line 1721 "ascend/compiler/ascParse.y"
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
#line 3880 "ascend/compiler/ascParse.c"
    break;

  case 80: /* table_decl_opt: ISA_TOK type_identifier optional_of  */
#line 1734 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.decl_typeargs != NULL) {
	    DestroySetList(g_table_parse.decl_typeargs);
	  }
	  g_table_parse.decl_type = (yyvsp[-1].id_ptr);
	  g_table_parse.decl_set_type = (yyvsp[0].id_ptr);
	  g_table_parse.decl_typeargs = g_typeargs;
	  g_typeargs = NULL;
	}
#line 3894 "ascend/compiler/ascParse.c"
    break;

  case 81: /* table_option: POSITIONAL_TOK  */
#line 1747 "ascend/compiler/ascParse.y"
        {
	  g_table_parse.positional = 1;
	}
#line 3902 "ascend/compiler/ascParse.c"
    break;

  case 82: /* table_option: DEFAULT_TOK expr  */
#line 1751 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.default_expr != NULL) {
	    DestroyExprList(g_table_parse.default_expr);
	  }
	  g_table_parse.default_expr = (yyvsp[0].eptr);
	}
#line 3913 "ascend/compiler/ascParse.c"
    break;

  case 83: /* table_option: UNITS_TOK BRACEDTEXT_TOK  */
#line 1758 "ascend/compiler/ascParse.y"
        {
	  if (g_table_parse.units != NULL) {
	    ascfree(g_table_parse.units);
	  }
	  g_table_parse.units = ASC_STRDUP((yyvsp[0].braced_ptr));
	}
#line 3924 "ascend/compiler/ascParse.c"
    break;

  case 87: /* table_body_item: ':'  */
#line 1774 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(":",0);
	}
#line 3932 "ascend/compiler/ascParse.c"
    break;

  case 88: /* table_body_item: '='  */
#line 1778 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("=",0);
	}
#line 3940 "ascend/compiler/ascParse.c"
    break;

  case 89: /* table_body_item: ','  */
#line 1782 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(",",0);
	}
#line 3948 "ascend/compiler/ascParse.c"
    break;

  case 90: /* table_body_item: '+'  */
#line 1786 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("+",0);
	}
#line 3956 "ascend/compiler/ascParse.c"
    break;

  case 91: /* table_body_item: '-'  */
#line 1790 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken("-",0);
	}
#line 3964 "ascend/compiler/ascParse.c"
    break;

  case 92: /* table_body_item: ';'  */
#line 1794 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3972 "ascend/compiler/ascParse.c"
    break;

  case 93: /* table_body_item: EOL_TOK  */
#line 1798 "ascend/compiler/ascParse.y"
        {
	  TableParseEndRow();
	}
#line 3980 "ascend/compiler/ascParse.c"
    break;

  case 94: /* table_scalar: IDENTIFIER_TOK  */
#line 1805 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendToken(SCP((yyvsp[0].id_ptr)),1);
	}
#line 3988 "ascend/compiler/ascParse.c"
    break;

  case 95: /* table_scalar: SYMBOL_TOK  */
#line 1809 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendSymbol((yyvsp[0].sym_ptr));
	}
#line 3996 "ascend/compiler/ascParse.c"
    break;

  case 96: /* table_scalar: INTEGER_TOK  */
#line 1813 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendInteger((yyvsp[0].int_value));
	}
#line 4004 "ascend/compiler/ascParse.c"
    break;

  case 97: /* table_scalar: REAL_TOK  */
#line 1817 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendReal((yyvsp[0].real_value));
	}
#line 4012 "ascend/compiler/ascParse.c"
    break;

  case 98: /* table_scalar: BRACEDTEXT_TOK  */
#line 1821 "ascend/compiler/ascParse.y"
        {
	  TableParseAppendBraced((yyvsp[0].braced_ptr));
	}
#line 4020 "ascend/compiler/ascParse.c"
    break;

  case 99: /* values_statement: VALUES_TOK fname values_default_opt ';' values_entries END_TOK VALUES_TOK  */
#line 1828 "ascend/compiler/ascParse.y"
        {
	  DestroyName((yyvsp[-5].nptr));
	  (yyval.statptr) = NULL;
	}
#line 4029 "ascend/compiler/ascParse.c"
    break;

  case 101: /* values_default_opt: DEFAULT_TOK expr  */
#line 1837 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 4037 "ascend/compiler/ascParse.c"
    break;

  case 104: /* values_entry: values_key_list '=' expr  */
#line 1849 "ascend/compiler/ascParse.y"
        {
	  DestroyExprList((yyvsp[0].eptr));
	}
#line 4045 "ascend/compiler/ascParse.c"
    break;

  case 111: /* $@1: %empty  */
#line 1868 "ascend/compiler/ascParse.y"
        {
	  DatasetParseBegin((yyvsp[-3].id_ptr),(yyvsp[-1].dquote_ptr));
	}
#line 4053 "ascend/compiler/ascParse.c"
    break;

  case 112: /* dataset_statement: DATASET_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';' $@1 dataset_items END_TOK DATASET_TOK  */
#line 1872 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = DatasetParseFinish();
	}
#line 4061 "ascend/compiler/ascParse.c"
    break;

  case 115: /* dataset_items: dataset_items error ';'  */
#line 1881 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in DATASET item.");
	  ErrMsg_Generic("Check DATASET statement syntax.");
	  g_untrapped_error++;
	  yyerrok;
	}
#line 4072 "ascend/compiler/ascParse.c"
    break;

  case 118: /* dataset_index_item: INDEX_TOK IDENTIFIER_TOK FROM_TOK COLUMN_TOK dataset_column_ref ISA_TOK IDENTIFIER_TOK  */
#line 1896 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddIndex((yyvsp[-5].id_ptr),(yyvsp[-2].id_ptr),(yyvsp[0].id_ptr));
	}
#line 4080 "ascend/compiler/ascParse.c"
    break;

  case 119: /* dataset_map_item: dataset_target FROM_TOK dataset_column_selector dataset_units_opt dataset_type_opt  */
#line 1903 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-2].id_ptr),(yyvsp[-1].braced_ptr),(yyvsp[0].id_ptr));
	}
#line 4088 "ascend/compiler/ascParse.c"
    break;

  case 120: /* dataset_map_item: dataset_target dataset_type_req FROM_TOK dataset_column_selector dataset_units_opt  */
#line 1907 "ascend/compiler/ascParse.y"
        {
	  DatasetParseAddMap((yyvsp[-4].nptr),(yyvsp[-1].id_ptr),(yyvsp[0].braced_ptr),(yyvsp[-3].id_ptr));
	}
#line 4096 "ascend/compiler/ascParse.c"
    break;

  case 121: /* dataset_column_selector: COLUMN_TOK dataset_column_ref  */
#line 1914 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4104 "ascend/compiler/ascParse.c"
    break;

  case 122: /* dataset_column_selector: dataset_column_ref  */
#line 1918 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4112 "ascend/compiler/ascParse.c"
    break;

  case 123: /* dataset_column_ref: IDENTIFIER_TOK  */
#line 1925 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4120 "ascend/compiler/ascParse.c"
    break;

  case 124: /* dataset_column_ref: SYMBOL_TOK  */
#line 1929 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].sym_ptr);
	}
#line 4128 "ascend/compiler/ascParse.c"
    break;

  case 125: /* dataset_target: IDENTIFIER_TOK '[' fvarlist ']'  */
#line 1936 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices(CreateIdName((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4137 "ascend/compiler/ascParse.c"
    break;

  case 126: /* dataset_target: dataset_target '[' fvarlist ']'  */
#line 1941 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = DatasetAppendIndices((yyvsp[-3].nptr),(yyvsp[-1].lptr));
	  DestroyVariableList((yyvsp[-1].lptr));
	}
#line 4146 "ascend/compiler/ascParse.c"
    break;

  case 127: /* dataset_units_opt: %empty  */
#line 1949 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 4154 "ascend/compiler/ascParse.c"
    break;

  case 128: /* dataset_units_opt: BRACEDTEXT_TOK  */
#line 1953 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 4162 "ascend/compiler/ascParse.c"
    break;

  case 129: /* dataset_type_opt: %empty  */
#line 1960 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4170 "ascend/compiler/ascParse.c"
    break;

  case 130: /* dataset_type_opt: ISA_TOK IDENTIFIER_TOK  */
#line 1964 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4178 "ascend/compiler/ascParse.c"
    break;

  case 131: /* dataset_type_req: ISA_TOK IDENTIFIER_TOK  */
#line 1971 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4186 "ascend/compiler/ascParse.c"
    break;

  case 132: /* unitdeflist: %empty  */
#line 1977 "ascend/compiler/ascParse.y"
                {
		  (yyval.listp) = gl_create(100L);
	}
#line 4194 "ascend/compiler/ascParse.c"
    break;

  case 133: /* unitdeflist: unitdeflist unitdef  */
#line 1981 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-1].listp),(char *)(yyvsp[0].udefptr));
	  (yyval.listp) = (yyvsp[-1].listp);
	}
#line 4203 "ascend/compiler/ascParse.c"
    break;

  case 134: /* unitdef: IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'  */
#line 1989 "ascend/compiler/ascParse.y"
        {
	  (yyval.udefptr) = CreateUnitDef((yyvsp[-3].id_ptr),(yyvsp[-1].braced_ptr),Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4212 "ascend/compiler/ascParse.c"
    break;

  case 135: /* unitladderitemlist: %empty  */
#line 1996 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(20L);
	}
#line 4220 "ascend/compiler/ascParse.c"
    break;

  case 136: /* unitladderitemlist: unitladderitemlist unitladderitem ';'  */
#line 2000 "ascend/compiler/ascParse.y"
        {
	  gl_append_ptr((yyvsp[-2].listp),(char *)(yyvsp[-1].ulitemptr));
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4229 "ascend/compiler/ascParse.c"
    break;

  case 137: /* unitladderitem: IDENTIFIER_TOK '=' BRACEDTEXT_TOK  */
#line 2008 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[-2].id_ptr),(yyvsp[0].braced_ptr),0,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4238 "ascend/compiler/ascParse.c"
    break;

  case 138: /* unitladderitem: IDENTIFIER_TOK  */
#line 2013 "ascend/compiler/ascParse.y"
        {
	  (yyval.ulitemptr) = CreateUnitLadderItem((yyvsp[0].id_ptr),NULL,1,Asc_ModuleBestName(Asc_CurrentModule()),
	                     LineNum());
	}
#line 4247 "ascend/compiler/ascParse.c"
    break;

  case 139: /* methods: %empty  */
#line 2022 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = NULL;
	}
#line 4255 "ascend/compiler/ascParse.c"
    break;

  case 140: /* $@2: %empty  */
#line 2026 "ascend/compiler/ascParse.y"
        { /* To get rid of this, we will need a global proclist
	   * that accumulates procs until a MODEL production is
	   * completed. If any other sort of production is started,
	   * and proclist is not NULL, it should be discarded.
	   */
	}
#line 4266 "ascend/compiler/ascParse.c"
    break;

  case 141: /* methods: METHODS_TOK $@2 proclist  */
#line 2033 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	}
#line 4274 "ascend/compiler/ascParse.c"
    break;

  case 142: /* initial: %empty  */
#line 2040 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = EmptyStatementList();
	}
#line 4282 "ascend/compiler/ascParse.c"
    break;

  case 143: /* initial: INITIAL_TOK fstatements  */
#line 2044 "ascend/compiler/ascParse.y"
        {
	  AddContext((yyvsp[0].slptr),context_INITIAL);
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 4291 "ascend/compiler/ascParse.c"
    break;

  case 144: /* proclist: proclistf  */
#line 2052 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = (yyvsp[0].listp);
	  gl_sort((yyval.listp),(CmpFunc)CmpProcs);
	}
#line 4300 "ascend/compiler/ascParse.c"
    break;

  case 145: /* proclistf: %empty  */
#line 2059 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4308 "ascend/compiler/ascParse.c"
    break;

  case 146: /* proclistf: proclistf procedure  */
#line 2063 "ascend/compiler/ascParse.y"
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
#line 4334 "ascend/compiler/ascParse.c"
    break;

  case 147: /* procedure: procedure_id ';' fstatements end ';'  */
#line 2088 "ascend/compiler/ascParse.y"
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
#line 4349 "ascend/compiler/ascParse.c"
    break;

  case 148: /* procedure_id: METHOD_TOK IDENTIFIER_TOK  */
#line 2102 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_proc_name = (yyvsp[0].id_ptr);
	}
#line 4358 "ascend/compiler/ascParse.c"
    break;

  case 149: /* fstatements: statements  */
#line 2111 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = CreateStatementList((yyvsp[0].listp));
	}
#line 4366 "ascend/compiler/ascParse.c"
    break;

  case 150: /* statements: %empty  */
#line 2118 "ascend/compiler/ascParse.y"
        {
	  (yyval.listp) = gl_create(7L);
	}
#line 4374 "ascend/compiler/ascParse.c"
    break;

  case 151: /* statements: statements statement ';'  */
#line 2122 "ascend/compiler/ascParse.y"
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
#line 4450 "ascend/compiler/ascParse.c"
    break;

  case 152: /* statements: statements complex_statement ';'  */
#line 2194 "ascend/compiler/ascParse.y"
        {
	  if ((yyvsp[-1].listp) != NULL) {
	    gl_append_list((yyvsp[-2].listp),(yyvsp[-1].listp));
            gl_destroy((yyvsp[-1].listp));
	  }
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4462 "ascend/compiler/ascParse.c"
    break;

  case 153: /* statements: statements error ';'  */
#line 2202 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_Generic("Error in statement input.");
	  (yyval.listp) = (yyvsp[-2].listp);
	}
#line 4471 "ascend/compiler/ascParse.c"
    break;

  case 201: /* isa_statement: fvarlist ISA_TOK type_identifier optional_of optional_with_value  */
#line 2265 "ascend/compiler/ascParse.y"
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
#line 4505 "ascend/compiler/ascParse.c"
    break;

  case 202: /* willbe_statement: fvarlist WILLBE_TOK type_identifier optional_of optional_with_value  */
#line 2298 "ascend/compiler/ascParse.y"
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
#line 4536 "ascend/compiler/ascParse.c"
    break;

  case 203: /* aliases_statement: fvarlist ALIASES_TOK fname  */
#line 2328 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateALIASES((yyvsp[-2].lptr),(yyvsp[0].nptr));
	}
#line 4544 "ascend/compiler/ascParse.c"
    break;

  case 204: /* aliases_statement: fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values  */
#line 2333 "ascend/compiler/ascParse.y"
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
#line 4594 "ascend/compiler/ascParse.c"
    break;

  case 205: /* optional_set_values: %empty  */
#line 2382 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 4602 "ascend/compiler/ascParse.c"
    break;

  case 206: /* optional_set_values: WITH_VALUE_T '(' set ')'  */
#line 2386 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = (yyvsp[-1].sptr);
	}
#line 4610 "ascend/compiler/ascParse.c"
    break;

  case 207: /* is_statement: fvarlist _IS_T IDENTIFIER_TOK optional_of  */
#line 2393 "ascend/compiler/ascParse.y"
        {
	  if (FindType((yyvsp[-1].id_ptr))) {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	  } else {
	    (yyval.statptr) = CreateREF((yyvsp[-3].lptr),(yyvsp[-1].id_ptr),(yyvsp[0].id_ptr),1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP((yyvsp[-1].id_ptr)));
	  }
	}
#line 4623 "ascend/compiler/ascParse.c"
    break;

  case 208: /* isrefinedto_statement: fvarlist ISREFINEDTO_TOK type_identifier  */
#line 2405 "ascend/compiler/ascParse.y"
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
#line 4651 "ascend/compiler/ascParse.c"
    break;

  case 209: /* call_identifier: IDENTIFIER_TOK  */
#line 2432 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_callargs = NULL;
	}
#line 4660 "ascend/compiler/ascParse.c"
    break;

  case 210: /* call_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2437 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_callargs = (yyvsp[-1].sptr);
	}
#line 4669 "ascend/compiler/ascParse.c"
    break;

  case 211: /* type_identifier: IDENTIFIER_TOK  */
#line 2445 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	  g_typeargs = NULL;
	}
#line 4678 "ascend/compiler/ascParse.c"
    break;

  case 212: /* type_identifier: IDENTIFIER_TOK '(' set ')'  */
#line 2450 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[-3].id_ptr);
	  g_typeargs = (yyvsp[-1].sptr);
	}
#line 4687 "ascend/compiler/ascParse.c"
    break;

  case 213: /* optional_method: %empty  */
#line 2458 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4695 "ascend/compiler/ascParse.c"
    break;

  case 214: /* optional_method: METHOD_TOK IDENTIFIER_TOK  */
#line 2462 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4703 "ascend/compiler/ascParse.c"
    break;

  case 215: /* optional_of: %empty  */
#line 2469 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = NULL;
	}
#line 4711 "ascend/compiler/ascParse.c"
    break;

  case 216: /* optional_of: OF_TOK IDENTIFIER_TOK  */
#line 2473 "ascend/compiler/ascParse.y"
        {
	  (yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 4719 "ascend/compiler/ascParse.c"
    break;

  case 217: /* optional_with_value: %empty  */
#line 2480 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 4727 "ascend/compiler/ascParse.c"
    break;

  case 218: /* optional_with_value: WITH_VALUE_T expr  */
#line 2484 "ascend/compiler/ascParse.y"
        {
	  g_decl_checkkind = ISCV_WITH_VALUE;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4736 "ascend/compiler/ascParse.c"
    break;

  case 219: /* optional_with_value: DEFAULT_TOK expr  */
#line 2489 "ascend/compiler/ascParse.y"
        {
	  g_decl_checkkind = ISCV_DEFAULT;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 4745 "ascend/compiler/ascParse.c"
    break;

  case 220: /* arealike_statement: fvarlist AREALIKE_TOK  */
#line 2497 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateAA((yyvsp[-1].lptr));
	}
#line 4753 "ascend/compiler/ascParse.c"
    break;

  case 221: /* link_statement: LINK_TOK '(' IGNORE_TOK ',' SYMBOL_TOK ',' fvarlist ')'  */
#line 2504 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = IgnoreLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4761 "ascend/compiler/ascParse.c"
    break;

  case 222: /* link_statement: LINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2508 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4769 "ascend/compiler/ascParse.c"
    break;

  case 223: /* link_statement: LINK_TOK '(' fname ',' fvarlist ')'  */
#line 2512 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4777 "ascend/compiler/ascParse.c"
    break;

  case 224: /* unlink_statement: UNLINK_TOK '(' SYMBOL_TOK ',' fvarlist ')'  */
#line 2519 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK((yyvsp[-3].sym_ptr),NULL,(yyvsp[-1].lptr));
	}
#line 4785 "ascend/compiler/ascParse.c"
    break;

  case 225: /* unlink_statement: UNLINK_TOK '(' fname ',' fvarlist ')'  */
#line 2523 "ascend/compiler/ascParse.y"
        {
	    (yyval.statptr) = CreateUNLNK(NULL,(yyvsp[-3].nptr),(yyvsp[-1].lptr));
	}
#line 4793 "ascend/compiler/ascParse.c"
    break;

  case 226: /* der_statement: DERLINK_TOK '(' fvarlist ')'  */
#line 2530 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("ode");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[-1].lptr));
	}
#line 4803 "ascend/compiler/ascParse.c"
    break;

  case 227: /* independent_statement: INDEPENDENT_TOK fvarlist  */
#line 2539 "ascend/compiler/ascParse.y"
        {
	    symchar *str;
	    str = AddSymbol("independent");
	    (yyval.statptr) = CreateLNK(str,NULL,(yyvsp[0].lptr));
	}
#line 4813 "ascend/compiler/ascParse.c"
    break;

  case 228: /* arethesame_statement: fvarlist ARETHESAME_TOK  */
#line 2548 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateATS((yyvsp[-1].lptr));
	}
#line 4821 "ascend/compiler/ascParse.c"
    break;

  case 229: /* willbethesame_statement: fvarlist WILLBETHESAME_TOK  */
#line 2555 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWBTS((yyvsp[-1].lptr));
	}
#line 4829 "ascend/compiler/ascParse.c"
    break;

  case 230: /* willnotbethesame_statement: fvarlist WILLNOTBETHESAME_TOK  */
#line 2562 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateWNBTS((yyvsp[-1].lptr));
	}
#line 4837 "ascend/compiler/ascParse.c"
    break;

  case 231: /* assignment_statement: fvarref ASSIGN_TOK expr  */
#line 2569 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4845 "ascend/compiler/ascParse.c"
    break;

  case 232: /* assignment_statement: fvarref CASSIGN_TOK expr  */
#line 2573 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateCASSIGN((yyvsp[-2].nptr),(yyvsp[0].eptr));
	}
#line 4853 "ascend/compiler/ascParse.c"
    break;

  case 233: /* reinit_statement: REINIT_TOK '(' fvarref ',' expr ')'  */
#line 2580 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateREINIT((yyvsp[-3].nptr),(yyvsp[-1].eptr));
	}
#line 4861 "ascend/compiler/ascParse.c"
    break;

  case 234: /* switchto_statement: SWITCH_TOK TO_TOK expr IF_TOK expr  */
#line 2587 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateSWITCHTO((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 4869 "ascend/compiler/ascParse.c"
    break;

  case 235: /* relation_statement: relation  */
#line 2594 "ascend/compiler/ascParse.y"
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
#line 4886 "ascend/compiler/ascParse.c"
    break;

  case 236: /* relation_statement: fname ':' relation  */
#line 2607 "ascend/compiler/ascParse.y"
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
#line 4904 "ascend/compiler/ascParse.c"
    break;

  case 237: /* relation: expr  */
#line 2624 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	  if (NumberOfRelOps((yyvsp[0].eptr)) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
#line 4918 "ascend/compiler/ascParse.c"
    break;

  case 238: /* relation: MINIMIZE_TOK expr  */
#line 2634 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minimize));
	  if (NumberOfRelOps((yyvsp[0].eptr)) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4930 "ascend/compiler/ascParse.c"
    break;

  case 239: /* relation: MAXIMIZE_TOK expr  */
#line 2642 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_maximize));
	  if (NumberOfRelOps((yyvsp[0].eptr))>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
#line 4942 "ascend/compiler/ascParse.c"
    break;

  case 240: /* blackbox_statement: fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'  */
#line 2653 "ascend/compiler/ascParse.y"
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
#line 4964 "ascend/compiler/ascParse.c"
    break;

  case 241: /* input_args: fvarlist ':' INPUT_TOK  */
#line 2674 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4972 "ascend/compiler/ascParse.c"
    break;

  case 242: /* output_args: fvarlist ':' OUTPUT_TOK  */
#line 2681 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = (yyvsp[-2].lptr);
	}
#line 4980 "ascend/compiler/ascParse.c"
    break;

  case 243: /* data_args: %empty  */
#line 2688 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = NULL;
	}
#line 4988 "ascend/compiler/ascParse.c"
    break;

  case 244: /* data_args: ';' fname ':' DATA_TOK  */
#line 2692 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[-2].nptr);
	}
#line 4996 "ascend/compiler/ascParse.c"
    break;

  case 245: /* for_statement: FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend fstatements end  */
#line 2730 "ascend/compiler/ascParse.y"
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
#line 5017 "ascend/compiler/ascParse.c"
    break;

  case 246: /* optional_direction: %empty  */
#line 2750 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_random;
	}
#line 5025 "ascend/compiler/ascParse.c"
    break;

  case 247: /* optional_direction: INCREASING_TOK  */
#line 2754 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_increasing;
	}
#line 5033 "ascend/compiler/ascParse.c"
    break;

  case 248: /* optional_direction: DECREASING_TOK  */
#line 2758 "ascend/compiler/ascParse.y"
        {
	  (yyval.order) = f_decreasing;
	}
#line 5041 "ascend/compiler/ascParse.c"
    break;

  case 249: /* forexprend: CREATE_TOK  */
#line 2765 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_create; /* declarative FOR */
	}
#line 5049 "ascend/compiler/ascParse.c"
    break;

  case 250: /* forexprend: EXPECT_TOK  */
#line 2769 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_expect; /* parameter FOR */
	}
#line 5057 "ascend/compiler/ascParse.c"
    break;

  case 251: /* forexprend: CHECK_TOK  */
#line 2773 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_check; /* WHERE FOR */
	}
#line 5065 "ascend/compiler/ascParse.c"
    break;

  case 252: /* forexprend: DO_TOK  */
#line 2777 "ascend/compiler/ascParse.y"
        {
	  (yyval.fkind) = fk_do; /* method FOR */
	}
#line 5073 "ascend/compiler/ascParse.c"
    break;

  case 253: /* run_statement: RUN_TOK fname  */
#line 2784 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),NULL);
	}
#line 5081 "ascend/compiler/ascParse.c"
    break;

  case 254: /* run_statement: RUN_TOK fname DBLCOLON_TOK fname  */
#line 2788 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateRUN((yyvsp[0].nptr),(yyvsp[-2].nptr));	  /* type :: name */
	}
#line 5089 "ascend/compiler/ascParse.c"
    break;

  case 255: /* fix_statement: FIX_TOK method_fvarlist  */
#line 2795 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'FIX' STATEMENT...");*/
		(yyval.statptr) = CreateFIX((yyvsp[0].lptr));
	}
#line 5098 "ascend/compiler/ascParse.c"
    break;

  case 256: /* fix_and_assign_statement: FIX_TOK assignment_statement  */
#line 2803 "ascend/compiler/ascParse.y"
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
#line 5113 "ascend/compiler/ascParse.c"
    break;

  case 257: /* free_statement: FREE_TOK method_fvarlist  */
#line 2817 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateFREE((yyvsp[0].lptr));
	}
#line 5121 "ascend/compiler/ascParse.c"
    break;

  case 258: /* solver_statement: SOLVER_TOK IDENTIFIER_TOK  */
#line 2824 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVER' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateSOLVER(SCP((yyvsp[0].id_ptr)));
	}
#line 5130 "ascend/compiler/ascParse.c"
    break;

  case 259: /* integrator_statement: INTEGRATOR_TOK IDENTIFIER_TOK  */
#line 2832 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateINTEGRATOR(SCP((yyvsp[0].id_ptr)));
	}
#line 5138 "ascend/compiler/ascParse.c"
    break;

  case 260: /* option_statement: OPTION_TOK IDENTIFIER_TOK expr  */
#line 2839 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'OPTION' STATEMENT WITH '%s'", SCP($2));*/
		(yyval.statptr) = CreateOPTION(SCP((yyvsp[-1].id_ptr)),(yyvsp[0].eptr));
	}
#line 5147 "ascend/compiler/ascParse.c"
    break;

  case 261: /* solve_statement: SOLVE_TOK  */
#line 2847 "ascend/compiler/ascParse.y"
        {
		/*CONSOLE_DEBUG("GOT 'SOLVE' STATEMENT");*/
		(yyval.statptr) = CreateSOLVE(NULL);
	}
#line 5156 "ascend/compiler/ascParse.c"
    break;

  case 262: /* solve_statement: SOLVE_TOK fname  */
#line 2852 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSOLVE((yyvsp[0].nptr));
	}
#line 5164 "ascend/compiler/ascParse.c"
    break;

  case 263: /* integrate_statement: INTEGRATE_TOK FROM_TOK expr TO_TOK expr STEPS_TOK INTEGER_TOK  */
#line 2859 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateINTEGRATE((yyvsp[-4].eptr), (yyvsp[-2].eptr), (yyvsp[0].int_value));
	}
#line 5172 "ascend/compiler/ascParse.c"
    break;

  case 264: /* observe_statement: OBSERVE_TOK fvarlist observe_as_opt  */
#line 2866 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateOBSERVE((yyvsp[-1].lptr), (yyvsp[0].id_ptr));
	}
#line 5180 "ascend/compiler/ascParse.c"
    break;

  case 265: /* observe_as_opt: %empty  */
#line 2873 "ascend/compiler/ascParse.y"
        {
		(yyval.id_ptr) = NULL;
	}
#line 5188 "ascend/compiler/ascParse.c"
    break;

  case 266: /* observe_as_opt: AS_TOK IDENTIFIER_TOK  */
#line 2877 "ascend/compiler/ascParse.y"
        {
		(yyval.id_ptr) = (yyvsp[0].id_ptr);
	}
#line 5196 "ascend/compiler/ascParse.c"
    break;

  case 267: /* $@3: %empty  */
#line 2884 "ascend/compiler/ascParse.y"
        {
		g_study_parse = StudyParseEmpty();
		g_study_run_method = NULL;
		g_study_now = 0;
		g_study_filename = NULL;
	}
#line 5207 "ascend/compiler/ascParse.c"
    break;

  case 268: /* study_statement: STUDY_TOK $@3 study_obs_opt study_vary_opt study_run_opt study_now_opt study_file_opt  */
#line 2891 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateSTUDY((yyvsp[-4].lptr), g_study_parse.vary, g_study_parse.lower, g_study_parse.upper,
			g_study_parse.steps, g_study_parse.value, g_study_parse.mode, g_study_parse.dist,
			g_study_run_method, g_study_now, g_study_filename);
	}
#line 5217 "ascend/compiler/ascParse.c"
    break;

  case 269: /* study_obs_opt: %empty  */
#line 2900 "ascend/compiler/ascParse.y"
        {
		(yyval.lptr) = NULL;
	}
#line 5225 "ascend/compiler/ascParse.c"
    break;

  case 270: /* study_obs_opt: fvarlist  */
#line 2904 "ascend/compiler/ascParse.y"
        {
		(yyval.lptr) = (yyvsp[0].lptr);
	}
#line 5233 "ascend/compiler/ascParse.c"
    break;

  case 271: /* study_vary_opt: %empty  */
#line 2911 "ascend/compiler/ascParse.y"
        {
	}
#line 5240 "ascend/compiler/ascParse.c"
    break;

  case 272: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEPS_TOK INTEGER_TOK study_distribution_opt  */
#line 2914 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-7].nptr);
		g_study_parse.lower = (yyvsp[-5].eptr);
		g_study_parse.upper = (yyvsp[-3].eptr);
		g_study_parse.steps = (yyvsp[-1].int_value);
		g_study_parse.mode = study_steps;
	}
#line 5252 "ascend/compiler/ascParse.c"
    break;

  case 273: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr STEP_TOK expr  */
#line 2922 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_step;
		g_study_parse.dist = study_dist_linear;
	}
#line 5265 "ascend/compiler/ascParse.c"
    break;

  case 274: /* study_vary_opt: VARY_TOK fname FROM_TOK expr TO_TOK expr RATIO_TOK expr  */
#line 2931 "ascend/compiler/ascParse.y"
        {
		g_study_parse.vary = (yyvsp[-6].nptr);
		g_study_parse.lower = (yyvsp[-4].eptr);
		g_study_parse.upper = (yyvsp[-2].eptr);
		g_study_parse.value = (yyvsp[0].eptr);
		g_study_parse.mode = study_ratio;
		g_study_parse.dist = study_dist_log;
	}
#line 5278 "ascend/compiler/ascParse.c"
    break;

  case 275: /* study_distribution_opt: %empty  */
#line 2943 "ascend/compiler/ascParse.y"
        {
	}
#line 5285 "ascend/compiler/ascParse.c"
    break;

  case 276: /* study_distribution_opt: LINEAR_TOK  */
#line 2946 "ascend/compiler/ascParse.y"
        {
		g_study_parse.dist = study_dist_linear;
	}
#line 5293 "ascend/compiler/ascParse.c"
    break;

  case 277: /* study_distribution_opt: LOG_TOK  */
#line 2950 "ascend/compiler/ascParse.y"
        {
		g_study_parse.dist = study_dist_log;
	}
#line 5301 "ascend/compiler/ascParse.c"
    break;

  case 278: /* study_run_opt: %empty  */
#line 2957 "ascend/compiler/ascParse.y"
        {
	}
#line 5308 "ascend/compiler/ascParse.c"
    break;

  case 279: /* study_run_opt: RUN_TOK IDENTIFIER_TOK  */
#line 2960 "ascend/compiler/ascParse.y"
        {
		g_study_run_method = (yyvsp[0].id_ptr);
	}
#line 5316 "ascend/compiler/ascParse.c"
    break;

  case 280: /* study_file_opt: %empty  */
#line 2967 "ascend/compiler/ascParse.y"
        {
	}
#line 5323 "ascend/compiler/ascParse.c"
    break;

  case 281: /* study_file_opt: FILE_TOK DQUOTE_TOK  */
#line 2970 "ascend/compiler/ascParse.y"
        {
		g_study_filename = (yyvsp[0].dquote_ptr);
	}
#line 5331 "ascend/compiler/ascParse.c"
    break;

  case 282: /* study_now_opt: %empty  */
#line 2977 "ascend/compiler/ascParse.y"
        {
	}
#line 5338 "ascend/compiler/ascParse.c"
    break;

  case 283: /* study_now_opt: NOW_TOK  */
#line 2980 "ascend/compiler/ascParse.y"
        {
		g_study_now = 1;
	}
#line 5346 "ascend/compiler/ascParse.c"
    break;

  case 284: /* delete_statement: DELETE_TOK SYSTEM_TOK  */
#line 2987 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateDELETESYSTEM();
	}
#line 5354 "ascend/compiler/ascParse.c"
    break;

  case 285: /* external_statement: EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'  */
#line 2994 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  (yyval.statptr) = CreateEXTERNMethod(SCP((yyvsp[-3].id_ptr)),(yyvsp[-1].lptr));
	}
#line 5366 "ascend/compiler/ascParse.c"
    break;

  case 286: /* call_statement: CALL_TOK call_identifier  */
#line 3005 "ascend/compiler/ascParse.y"
        {
	  /*
	   * This is proper procedural external method code.
	   */
	  (yyval.statptr) = CreateCALL((yyvsp[0].id_ptr),g_callargs);
	  g_callargs = NULL;
	}
#line 5378 "ascend/compiler/ascParse.c"
    break;

  case 287: /* assert_statement: ASSERT_TOK expr  */
#line 3016 "ascend/compiler/ascParse.y"
        {
		(yyval.statptr) = CreateASSERT((yyvsp[0].eptr));
	}
#line 5386 "ascend/compiler/ascParse.c"
    break;

  case 288: /* if_statement: IF_TOK expr THEN_TOK fstatements optional_else end  */
#line 3022 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateIF((yyvsp[-4].eptr),(yyvsp[-2].slptr),(yyvsp[-1].slptr));
	}
#line 5397 "ascend/compiler/ascParse.c"
    break;

  case 289: /* while_statement: WHILE_TOK expr DO_TOK fstatements end  */
#line 3032 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWhile((yyvsp[-3].eptr),(yyvsp[-1].slptr));
	}
#line 5408 "ascend/compiler/ascParse.c"
    break;

  case 290: /* optional_else: %empty  */
#line 3041 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = NULL;
	}
#line 5416 "ascend/compiler/ascParse.c"
    break;

  case 291: /* optional_else: ELSE_TOK fstatements  */
#line 3045 "ascend/compiler/ascParse.y"
        {
	  (yyval.slptr) = (yyvsp[0].slptr);
	}
#line 5424 "ascend/compiler/ascParse.c"
    break;

  case 292: /* when_statement: WHEN_TOK fvarlist whenlist end  */
#line 3052 "ascend/compiler/ascParse.y"
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
#line 5439 "ascend/compiler/ascParse.c"
    break;

  case 293: /* when_statement: fname ':' WHEN_TOK fvarlist whenlist end  */
#line 3063 "ascend/compiler/ascParse.y"
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
#line 5455 "ascend/compiler/ascParse.c"
    break;

  case 294: /* when_statement: WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 3075 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN(NULL,(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5466 "ascend/compiler/ascParse.c"
    break;

  case 295: /* when_statement: fname ':' WHEN_TOK '(' fvarlist ')' whenlist end  */
#line 3082 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateWHEN((yyvsp[-7].nptr),(yyvsp[-3].lptr),(yyvsp[-1].wptr));
	}
#line 5477 "ascend/compiler/ascParse.c"
    break;

  case 296: /* whenlist: whenlistf  */
#line 3092 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = ReverseWhenCases((yyvsp[0].wptr));
	}
#line 5485 "ascend/compiler/ascParse.c"
    break;

  case 297: /* whenlistf: CASE_TOK set optional_when_case_if optional_when_case_applies ':' fstatements  */
#line 3099 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhenIfApplies((yyvsp[-4].sptr),(yyvsp[-3].eptr),(yyvsp[-2].eptr),(yyvsp[0].slptr));
	}
#line 5493 "ascend/compiler/ascParse.c"
    break;

  case 298: /* whenlistf: OTHERWISE_TOK optional_when_otherwise_label ':' fstatements  */
#line 3103 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = CreateWhenOtherwise((yyvsp[-2].sym_ptr),(yyvsp[0].slptr));
	}
#line 5501 "ascend/compiler/ascParse.c"
    break;

  case 299: /* whenlistf: whenlistf CASE_TOK set optional_when_case_if optional_when_case_applies ':' fstatements  */
#line 3107 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhenIfApplies((yyvsp[-4].sptr),(yyvsp[-3].eptr),(yyvsp[-2].eptr),(yyvsp[0].slptr)),(yyvsp[-6].wptr));
	}
#line 5509 "ascend/compiler/ascParse.c"
    break;

  case 300: /* whenlistf: whenlistf OTHERWISE_TOK optional_when_otherwise_label ':' fstatements  */
#line 3111 "ascend/compiler/ascParse.y"
        {
	  (yyval.wptr) = LinkWhenCases(CreateWhenOtherwise((yyvsp[-2].sym_ptr),(yyvsp[0].slptr)),(yyvsp[-4].wptr));
	}
#line 5517 "ascend/compiler/ascParse.c"
    break;

  case 301: /* optional_when_otherwise_label: %empty  */
#line 3117 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = NULL;
	}
#line 5525 "ascend/compiler/ascParse.c"
    break;

  case 302: /* optional_when_otherwise_label: SYMBOL_TOK  */
#line 3121 "ascend/compiler/ascParse.y"
        {
	  (yyval.sym_ptr) = (yyvsp[0].sym_ptr);
	}
#line 5533 "ascend/compiler/ascParse.c"
    break;

  case 303: /* optional_when_case_if: %empty  */
#line 3127 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 5541 "ascend/compiler/ascParse.c"
    break;

  case 304: /* $@4: %empty  */
#line 3130 "ascend/compiler/ascParse.y"
             { ++g_classifier_predicate_depth; }
#line 5547 "ascend/compiler/ascParse.c"
    break;

  case 305: /* optional_when_case_if: IF_TOK $@4 expr  */
#line 3131 "ascend/compiler/ascParse.y"
        {
	  --g_classifier_predicate_depth;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 5556 "ascend/compiler/ascParse.c"
    break;

  case 306: /* optional_when_case_applies: %empty  */
#line 3138 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = NULL;
	}
#line 5564 "ascend/compiler/ascParse.c"
    break;

  case 307: /* $@5: %empty  */
#line 3141 "ascend/compiler/ascParse.y"
                         { ++g_classifier_predicate_depth; }
#line 5570 "ascend/compiler/ascParse.c"
    break;

  case 308: /* optional_when_case_applies: APPLIES_TOK IF_TOK $@5 expr  */
#line 3142 "ascend/compiler/ascParse.y"
        {
	  --g_classifier_predicate_depth;
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 5579 "ascend/compiler/ascParse.c"
    break;

  case 309: /* flow_statement: BREAK_TOK  */
#line 3150 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_break,NULL);
	}
#line 5587 "ascend/compiler/ascParse.c"
    break;

  case 310: /* flow_statement: CONTINUE_TOK  */
#line 3154 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_continue,NULL);
	}
#line 5595 "ascend/compiler/ascParse.c"
    break;

  case 311: /* flow_statement: FALLTHRU_TOK  */
#line 3158 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_fallthru,NULL);
	}
#line 5603 "ascend/compiler/ascParse.c"
    break;

  case 312: /* flow_statement: RETURN_TOK  */
#line 3162 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_return,NULL);
	}
#line 5611 "ascend/compiler/ascParse.c"
    break;

  case 313: /* flow_statement: STOP_TOK optional_bracedtext  */
#line 3166 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFlow(fc_stop,(yyvsp[0].braced_ptr));
	}
#line 5619 "ascend/compiler/ascParse.c"
    break;

  case 314: /* use_statement: USE_TOK fname  */
#line 3173 "ascend/compiler/ascParse.y"
        {
	  (yyval.statptr) = CreateFNAME((yyvsp[0].nptr));
	}
#line 5627 "ascend/compiler/ascParse.c"
    break;

  case 315: /* select_statement: SELECT_TOK fvarlist selectlist end  */
#line 3180 "ascend/compiler/ascParse.y"
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
#line 5642 "ascend/compiler/ascParse.c"
    break;

  case 316: /* select_statement: SELECT_TOK '(' fvarlist ')' selectlist end  */
#line 3191 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSELECT((yyvsp[-3].lptr),(yyvsp[-1].septr));
	}
#line 5653 "ascend/compiler/ascParse.c"
    break;

  case 317: /* selectlist: selectlistf  */
#line 3201 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = ReverseSelectCases((yyvsp[0].septr));
	}
#line 5661 "ascend/compiler/ascParse.c"
    break;

  case 318: /* selectlistf: CASE_TOK set ':' fstatements  */
#line 3208 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5669 "ascend/compiler/ascParse.c"
    break;

  case 319: /* selectlistf: OTHERWISE_TOK ':' fstatements  */
#line 3212 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = CreateSelect(NULL,(yyvsp[0].slptr));
	}
#line 5677 "ascend/compiler/ascParse.c"
    break;

  case 320: /* selectlistf: selectlistf CASE_TOK set ':' fstatements  */
#line 3216 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].septr));
	}
#line 5685 "ascend/compiler/ascParse.c"
    break;

  case 321: /* selectlistf: selectlistf OTHERWISE_TOK ':' fstatements  */
#line 3220 "ascend/compiler/ascParse.y"
        {
	  (yyval.septr) = LinkSelectCases(CreateSelect(NULL,(yyvsp[0].slptr)),(yyvsp[-3].septr));
	}
#line 5693 "ascend/compiler/ascParse.c"
    break;

  case 322: /* switch_statement: SWITCH_TOK fvarlist switchlist end  */
#line 3227 "ascend/compiler/ascParse.y"
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
#line 5708 "ascend/compiler/ascParse.c"
    break;

  case 323: /* switch_statement: SWITCH_TOK '(' fvarlist ')' switchlist end  */
#line 3238 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateSWITCH((yyvsp[-3].lptr),(yyvsp[-1].swptr));
	}
#line 5719 "ascend/compiler/ascParse.c"
    break;

  case 324: /* switchlist: switchlistf  */
#line 3248 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = ReverseSwitchCases((yyvsp[0].swptr));
	}
#line 5727 "ascend/compiler/ascParse.c"
    break;

  case 325: /* switchlistf: CASE_TOK set ':' fstatements  */
#line 3255 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr));
	}
#line 5735 "ascend/compiler/ascParse.c"
    break;

  case 326: /* switchlistf: OTHERWISE_TOK ':' fstatements  */
#line 3259 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = CreateSwitch(NULL,(yyvsp[0].slptr));
	}
#line 5743 "ascend/compiler/ascParse.c"
    break;

  case 327: /* switchlistf: switchlistf CASE_TOK set ':' fstatements  */
#line 3263 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch((yyvsp[-2].sptr),(yyvsp[0].slptr)),(yyvsp[-4].swptr));
	}
#line 5751 "ascend/compiler/ascParse.c"
    break;

  case 328: /* switchlistf: switchlistf OTHERWISE_TOK ':' fstatements  */
#line 3267 "ascend/compiler/ascParse.y"
        {
	  (yyval.swptr) = LinkSwitchCases(CreateSwitch(NULL,(yyvsp[0].slptr)),(yyvsp[-3].swptr));
	}
#line 5759 "ascend/compiler/ascParse.c"
    break;

  case 329: /* conditional_statement: CONDITIONAL_TOK fstatements end  */
#line 3274 "ascend/compiler/ascParse.y"
        {
	  if( (yyvsp[0].int_value) != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, (yyvsp[0].int_value), NULL);
	  }
	  (yyval.statptr) = CreateCOND((yyvsp[-1].slptr));
	}
#line 5770 "ascend/compiler/ascParse.c"
    break;

  case 330: /* notes_statement: NOTES_TOK notes_body end  */
#line 3284 "ascend/compiler/ascParse.y"
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
#line 5808 "ascend/compiler/ascParse.c"
    break;

  case 331: /* notes_body: SYMBOL_TOK noteslist  */
#line 3321 "ascend/compiler/ascParse.y"
        {
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  (yyval.notesptr) = (yyvsp[0].notesptr);
	  assert((yyval.notesptr)->lang == NULL);
	  (yyval.notesptr)->lang = (yyvsp[-1].sym_ptr);
	}
#line 5821 "ascend/compiler/ascParse.c"
    break;

  case 332: /* notes_body: notes_body SYMBOL_TOK noteslist  */
#line 3330 "ascend/compiler/ascParse.y"
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
#line 5837 "ascend/compiler/ascParse.c"
    break;

  case 333: /* noteslist: fvarlist BRACEDTEXT_TOK  */
#line 3345 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	}
#line 5846 "ascend/compiler/ascParse.c"
    break;

  case 334: /* noteslist: noteslist fvarlist BRACEDTEXT_TOK  */
#line 3350 "ascend/compiler/ascParse.y"
        {
	  (yyval.notesptr) = CreateNoteTmp(NULL, AddBraceChar((yyvsp[0].braced_ptr),NULL),
	                     (void *)(yyvsp[-1].lptr), LineNum());
	  LinkNoteTmp((yyval.notesptr),(yyvsp[-2].notesptr));
	}
#line 5856 "ascend/compiler/ascParse.c"
    break;

  case 335: /* fvarlist: varlist  */
#line 3359 "ascend/compiler/ascParse.y"
        {
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5868 "ascend/compiler/ascParse.c"
    break;

  case 336: /* varlist: fname  */
#line 3370 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5876 "ascend/compiler/ascParse.c"
    break;

  case 337: /* varlist: varlist ',' fname  */
#line 3374 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5885 "ascend/compiler/ascParse.c"
    break;

  case 338: /* varlist: varlist fname  */
#line 3379 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
#line 5899 "ascend/compiler/ascParse.c"
    break;

  case 339: /* method_fvarlist: method_varlist  */
#line 3392 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = ReverseVariableList((yyvsp[0].lptr));
	}
#line 5907 "ascend/compiler/ascParse.c"
    break;

  case 340: /* method_varlist: fvarref  */
#line 3399 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	}
#line 5915 "ascend/compiler/ascParse.c"
    break;

  case 341: /* method_varlist: method_varlist ',' fvarref  */
#line 3403 "ascend/compiler/ascParse.y"
        {
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-2].lptr));
	}
#line 5924 "ascend/compiler/ascParse.c"
    break;

  case 342: /* method_varlist: method_varlist fvarref  */
#line 3408 "ascend/compiler/ascParse.y"
        {
	  ErrMsg_CommaName("name",(yyvsp[0].nptr));
	  (yyval.lptr) = CreateVariableNode((yyvsp[0].nptr));
	  LinkVariableNodes((yyval.lptr),(yyvsp[-1].lptr));
	  g_untrapped_error++;
	}
#line 5935 "ascend/compiler/ascParse.c"
    break;

  case 343: /* fvarref: fname  */
#line 3418 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = (yyvsp[0].nptr);
	}
#line 5943 "ascend/compiler/ascParse.c"
    break;

  case 344: /* fvarref: DERIV_TOK '(' fname ')'  */
#line 3422 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateDerivativeRefName((yyvsp[-1].nptr));
	}
#line 5951 "ascend/compiler/ascParse.c"
    break;

  case 345: /* fname: name optional_notes  */
#line 3429 "ascend/compiler/ascParse.y"
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
#line 5972 "ascend/compiler/ascParse.c"
    break;

  case 346: /* name: IDENTIFIER_TOK  */
#line 3449 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	}
#line 5980 "ascend/compiler/ascParse.c"
    break;

  case 347: /* name: name '.' IDENTIFIER_TOK  */
#line 3453 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName((yyvsp[0].id_ptr));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5989 "ascend/compiler/ascParse.c"
    break;

  case 348: /* name: name '.' DERIV_TOK  */
#line 3458 "ascend/compiler/ascParse.y"
        {
	  (yyval.nptr) = CreateIdName(AddSymbol("der"));
	  LinkNames((yyval.nptr),(yyvsp[-2].nptr));
	}
#line 5998 "ascend/compiler/ascParse.c"
    break;

  case 349: /* name: name '[' set ']'  */
#line 3463 "ascend/compiler/ascParse.y"
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
#line 6014 "ascend/compiler/ascParse.c"
    break;

  case 350: /* end: END_TOK CONDITIONAL_TOK  */
#line 3478 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = CONDITIONAL_TOK;
	}
#line 6023 "ascend/compiler/ascParse.c"
    break;

  case 351: /* end: END_TOK FOR_TOK  */
#line 3483 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = FOR_TOK;
	}
#line 6032 "ascend/compiler/ascParse.c"
    break;

  case 352: /* end: END_TOK IF_TOK  */
#line 3488 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = IF_TOK;
	}
#line 6041 "ascend/compiler/ascParse.c"
    break;

  case 353: /* end: END_TOK INTERACTIVE_TOK  */
#line 3493 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = INTERACTIVE_TOK;
	}
#line 6050 "ascend/compiler/ascParse.c"
    break;

  case 354: /* end: END_TOK METHODS_TOK  */
#line 3498 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = METHODS_TOK;
	}
#line 6059 "ascend/compiler/ascParse.c"
    break;

  case 355: /* end: END_TOK NOTES_TOK  */
#line 3503 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = NOTES_TOK;
	}
#line 6068 "ascend/compiler/ascParse.c"
    break;

  case 356: /* end: END_TOK SELECT_TOK  */
#line 3508 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SELECT_TOK;
	}
#line 6077 "ascend/compiler/ascParse.c"
    break;

  case 357: /* end: END_TOK SWITCH_TOK  */
#line 3513 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = SWITCH_TOK;
	}
#line 6086 "ascend/compiler/ascParse.c"
    break;

  case 358: /* end: END_TOK UNITS_TOK  */
#line 3518 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = UNITS_TOK;
	}
#line 6095 "ascend/compiler/ascParse.c"
    break;

  case 359: /* end: END_TOK GLOBAL_TOK  */
#line 3523 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = GLOBAL_TOK;
	}
#line 6104 "ascend/compiler/ascParse.c"
    break;

  case 360: /* end: END_TOK WHEN_TOK  */
#line 3528 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHEN_TOK;
	}
#line 6113 "ascend/compiler/ascParse.c"
    break;

  case 361: /* end: END_TOK WHILE_TOK  */
#line 3533 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = WHILE_TOK;
	}
#line 6122 "ascend/compiler/ascParse.c"
    break;

  case 362: /* end: END_TOK IDENTIFIER_TOK  */
#line 3538 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = (yyvsp[0].id_ptr);
	  (yyval.int_value) = IDENTIFIER_TOK;
	}
#line 6131 "ascend/compiler/ascParse.c"
    break;

  case 363: /* end: END_TOK  */
#line 3543 "ascend/compiler/ascParse.y"
        {
	  g_end_identifier = NULL;
	  (yyval.int_value) = END_TOK;
	}
#line 6140 "ascend/compiler/ascParse.c"
    break;

  case 364: /* optional_bracedtext: %empty  */
#line 3551 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = NULL;
	}
#line 6148 "ascend/compiler/ascParse.c"
    break;

  case 365: /* optional_bracedtext: BRACEDTEXT_TOK  */
#line 3555 "ascend/compiler/ascParse.y"
        {
	  (yyval.braced_ptr) = (yyvsp[0].braced_ptr);
	}
#line 6156 "ascend/compiler/ascParse.c"
    break;

  case 366: /* optional_notes: %empty  */
#line 3562 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = NULL;
	}
#line 6164 "ascend/compiler/ascParse.c"
    break;

  case 367: /* optional_notes: DQUOTE_TOK  */
#line 3566 "ascend/compiler/ascParse.y"
        {
	  (yyval.dquote_ptr) = (yyvsp[0].dquote_ptr);
	}
#line 6172 "ascend/compiler/ascParse.c"
    break;

  case 368: /* set: setexprlist  */
#line 3573 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = ReverseSetList((yyvsp[0].sptr));
	}
#line 6180 "ascend/compiler/ascParse.c"
    break;

  case 369: /* set: %empty  */
#line 3577 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = NULL;
	}
#line 6188 "ascend/compiler/ascParse.c"
    break;

  case 370: /* setexprlist: expr  */
#line 3584 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	}
#line 6196 "ascend/compiler/ascParse.c"
    break;

  case 371: /* setexprlist: '*'  */
#line 3588 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet(CreateAnyExpr());
	}
#line 6204 "ascend/compiler/ascParse.c"
    break;

  case 372: /* setexprlist: expr DOTDOT_TOK expr  */
#line 3592 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6212 "ascend/compiler/ascParse.c"
    break;

  case 373: /* setexprlist: setexprlist ',' expr  */
#line 3596 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet((yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 6221 "ascend/compiler/ascParse.c"
    break;

  case 374: /* setexprlist: setexprlist ',' '*'  */
#line 3601 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateSingleSet(CreateAnyExpr());
	  LinkSets((yyval.sptr),(yyvsp[-2].sptr));
	}
#line 6230 "ascend/compiler/ascParse.c"
    break;

  case 375: /* setexprlist: setexprlist ',' expr DOTDOT_TOK expr  */
#line 3606 "ascend/compiler/ascParse.y"
        {
	  (yyval.sptr) = CreateRangeSet((yyvsp[-2].eptr),(yyvsp[0].eptr));
	  LinkSets((yyval.sptr),(yyvsp[-4].sptr));
	}
#line 6239 "ascend/compiler/ascParse.c"
    break;

  case 376: /* number: INTEGER_TOK  */
#line 3614 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].int_value);
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	  g_number_units = NULL;
	}
#line 6250 "ascend/compiler/ascParse.c"
    break;

  case 377: /* number: realnumber  */
#line 3621 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[0].real_value);
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
#line 6260 "ascend/compiler/ascParse.c"
    break;

  case 378: /* realnumber: REAL_TOK opunits  */
#line 3630 "ascend/compiler/ascParse.y"
        {
	  (yyval.real_value) = (yyvsp[-1].real_value)*(yyvsp[0].real_value);
	  g_number_units = g_parsed_units;
	}
#line 6269 "ascend/compiler/ascParse.c"
    break;

  case 379: /* realnumber: INTEGER_TOK BRACEDTEXT_TOK  */
#line 3635 "ascend/compiler/ascParse.y"
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
#line 6294 "ascend/compiler/ascParse.c"
    break;

  case 380: /* opunits: %empty  */
#line 3659 "ascend/compiler/ascParse.y"
        {
	  g_dim_ptr = Dimensionless();
	  g_parsed_units = NULL;
	  (yyval.real_value) = 1.0;
	}
#line 6304 "ascend/compiler/ascParse.c"
    break;

  case 381: /* opunits: BRACEDTEXT_TOK  */
#line 3665 "ascend/compiler/ascParse.y"
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
#line 6328 "ascend/compiler/ascParse.c"
    break;

  case 382: /* dims: DIMENSION_TOK dimensions  */
#line 3688 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = (yyvsp[0].dimp);
	}
#line 6336 "ascend/compiler/ascParse.c"
    break;

  case 383: /* dims: DIMENSIONLESS_TOK  */
#line 3692 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = Dimensionless();
	}
#line 6344 "ascend/compiler/ascParse.c"
    break;

  case 384: /* dims: %empty  */
#line 3696 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 6352 "ascend/compiler/ascParse.c"
    break;

  case 385: /* dimensions: '*'  */
#line 3703 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = WildDimension();
	}
#line 6360 "ascend/compiler/ascParse.c"
    break;

  case 386: /* dimensions: dimexpr  */
#line 3707 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimp) = FindOrAddDimen(&((yyvsp[0].dimen)));
	}
#line 6368 "ascend/compiler/ascParse.c"
    break;

  case 387: /* dimexpr: IDENTIFIER_TOK  */
#line 3714 "ascend/compiler/ascParse.y"
        {
	  ParseDim(&((yyval.dimen)),SCP((yyvsp[0].id_ptr)));
	}
#line 6376 "ascend/compiler/ascParse.c"
    break;

  case 388: /* dimexpr: INTEGER_TOK  */
#line 3718 "ascend/compiler/ascParse.y"
        {
	  ClearDimensions(&((yyval.dimen)));
	}
#line 6384 "ascend/compiler/ascParse.c"
    break;

  case 389: /* dimexpr: dimexpr '/' dimexpr  */
#line 3722 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = SubDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6392 "ascend/compiler/ascParse.c"
    break;

  case 390: /* dimexpr: dimexpr '*' dimexpr  */
#line 3726 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = AddDimensions(&((yyvsp[-2].dimen)),&((yyvsp[0].dimen)));
	}
#line 6400 "ascend/compiler/ascParse.c"
    break;

  case 391: /* dimexpr: dimexpr '^' fraction  */
#line 3730 "ascend/compiler/ascParse.y"
        {
	  (yyval.dimen) = ScaleDimensions(&((yyvsp[-2].dimen)),(yyvsp[0].frac_value));
	}
#line 6408 "ascend/compiler/ascParse.c"
    break;

  case 392: /* dimexpr: '(' dimexpr ')'  */
#line 3734 "ascend/compiler/ascParse.y"
        {
	  CopyDimensions(&((yyvsp[-1].dimen)),&((yyval.dimen)));
	}
#line 6416 "ascend/compiler/ascParse.c"
    break;

  case 393: /* fraction: optional_sign fractail  */
#line 3741 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = (yyvsp[-1].int_value) ? NegateF((yyvsp[0].frac_value)) : (yyvsp[0].frac_value);
	}
#line 6424 "ascend/compiler/ascParse.c"
    break;

  case 394: /* fractail: INTEGER_TOK  */
#line 3748 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[0].int_value),(short)1);
	}
#line 6432 "ascend/compiler/ascParse.c"
    break;

  case 395: /* fractail: '(' INTEGER_TOK '/' INTEGER_TOK ')'  */
#line 3752 "ascend/compiler/ascParse.y"
        {
	  (yyval.frac_value) = CreateFraction((short)(yyvsp[-3].int_value),(short)(yyvsp[-1].int_value));
	}
#line 6440 "ascend/compiler/ascParse.c"
    break;

  case 396: /* optional_sign: %empty  */
#line 3759 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6448 "ascend/compiler/ascParse.c"
    break;

  case 397: /* optional_sign: '+'  */
#line 3763 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 0;
	}
#line 6456 "ascend/compiler/ascParse.c"
    break;

  case 398: /* optional_sign: '-'  */
#line 3767 "ascend/compiler/ascParse.y"
        {
	  (yyval.int_value) = 1;
	}
#line 6464 "ascend/compiler/ascParse.c"
    break;

  case 399: /* expr: INTEGER_TOK  */
#line 3774 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr((yyvsp[0].int_value));
	}
#line 6472 "ascend/compiler/ascParse.c"
    break;

  case 400: /* expr: MAXINTEGER_TOK  */
#line 3778 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateIntExpr(LONG_MAX-1);
	}
#line 6480 "ascend/compiler/ascParse.c"
    break;

  case 401: /* expr: realnumber  */
#line 3782 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr((yyvsp[0].real_value),g_dim_ptr);
	}
#line 6488 "ascend/compiler/ascParse.c"
    break;

  case 402: /* expr: MAXREAL_TOK  */
#line 3786 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
#line 6496 "ascend/compiler/ascParse.c"
    break;

  case 403: /* expr: TRUE_TOK  */
#line 3790 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateTrueExpr();
	}
#line 6504 "ascend/compiler/ascParse.c"
    break;

  case 404: /* expr: FALSE_TOK  */
#line 3794 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateFalseExpr();
	}
#line 6512 "ascend/compiler/ascParse.c"
    break;

  case 405: /* expr: ANY_TOK  */
#line 3798 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateAnyExpr();
	}
#line 6520 "ascend/compiler/ascParse.c"
    break;

  case 406: /* expr: SYMBOL_TOK  */
#line 3802 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSymbolExpr((yyvsp[0].sym_ptr));
	}
#line 6528 "ascend/compiler/ascParse.c"
    break;

  case 407: /* expr: fname  */
#line 3806 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateVarExpr((yyvsp[0].nptr));
	}
#line 6536 "ascend/compiler/ascParse.c"
    break;

  case 408: /* expr: DERIV_TOK '(' fname ')'  */
#line 3810 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateDiffExpr((yyvsp[-1].nptr));
	}
#line 6544 "ascend/compiler/ascParse.c"
    break;

  case 409: /* expr: '[' set ']'  */
#line 3814 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSetExpr((yyvsp[-1].sptr));
	}
#line 6552 "ascend/compiler/ascParse.c"
    break;

  case 410: /* expr: expr '+' expr  */
#line 3818 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_plus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6561 "ascend/compiler/ascParse.c"
    break;

  case 411: /* expr: expr '-' expr  */
#line 3823 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_minus));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6570 "ascend/compiler/ascParse.c"
    break;

  case 412: /* expr: expr '*' expr  */
#line 3828 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_times));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6579 "ascend/compiler/ascParse.c"
    break;

  case 413: /* expr: expr '/' expr  */
#line 3833 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_divide));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6588 "ascend/compiler/ascParse.c"
    break;

  case 414: /* expr: expr '^' expr  */
#line 3838 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_power));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6597 "ascend/compiler/ascParse.c"
    break;

  case 415: /* expr: expr AND_TOK expr  */
#line 3843 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_and));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6606 "ascend/compiler/ascParse.c"
    break;

  case 416: /* expr: expr OR_TOK expr  */
#line 3848 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_or));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6615 "ascend/compiler/ascParse.c"
    break;

  case 417: /* expr: NOT_TOK expr  */
#line 3853 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_not));
	}
#line 6623 "ascend/compiler/ascParse.c"
    break;

  case 418: /* expr: expr relop expr  */
#line 3857 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6632 "ascend/compiler/ascParse.c"
    break;

  case 419: /* expr: expr logrelop expr  */
#line 3862 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),(yyvsp[-1].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6641 "ascend/compiler/ascParse.c"
    break;

  case 420: /* expr: expr IN_TOK expr  */
#line 3867 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_in));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6650 "ascend/compiler/ascParse.c"
    break;

  case 421: /* expr: expr '|' expr  */
#line 3872 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6659 "ascend/compiler/ascParse.c"
    break;

  case 422: /* expr: expr SUCHTHAT_TOK expr  */
#line 3877 "ascend/compiler/ascParse.y"
        {
	  (yyvsp[0].eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_st));
	  (yyval.eptr) = JoinExprLists((yyvsp[-2].eptr),(yyvsp[0].eptr));
	}
#line 6668 "ascend/compiler/ascParse.c"
    break;

  case 423: /* expr: '+' expr  */
#line 3882 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[0].eptr);
	}
#line 6676 "ascend/compiler/ascParse.c"
    break;

  case 424: /* expr: '-' expr  */
#line 3886 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = JoinExprLists((yyvsp[0].eptr),CreateOpExpr(e_uminus));
	}
#line 6684 "ascend/compiler/ascParse.c"
    break;

  case 425: /* expr: SATISFIED_TOK '(' expr relop expr ',' realnumber ')'  */
#line 3890 "ascend/compiler/ascParse.y"
        {
	  if(!g_classifier_predicate_depth) {
	    zz_error("SATISFIED(real relation, tolerance) is only supported in CASE IF and APPLIES IF predicates");
	    DestroyExprList((yyvsp[-5].eptr));
	    DestroyExprList((yyvsp[-4].eptr));
	    DestroyExprList((yyvsp[-3].eptr));
	    YYERROR;
	  }
	  (yyvsp[-3].eptr) = JoinExprLists((yyvsp[-3].eptr),(yyvsp[-4].eptr));
	  (yyval.eptr) = JoinExprLists((yyvsp[-5].eptr),(yyvsp[-3].eptr));
	  (yyval.eptr) = JoinExprLists((yyval.eptr),CreateSatisfiedExpr(NULL,(yyvsp[-1].real_value),g_dim_ptr));
	}
#line 6701 "ascend/compiler/ascParse.c"
    break;

  case 426: /* expr: SATISFIED_TOK '(' fname ',' realnumber ')'  */
#line 3903 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-3].nptr),(yyvsp[-1].real_value),g_dim_ptr);
	}
#line 6709 "ascend/compiler/ascParse.c"
    break;

  case 427: /* expr: SATISFIED_TOK '(' fname ')'  */
#line 3907 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateSatisfiedExpr((yyvsp[-1].nptr),DBL_MAX,NULL);
	}
#line 6717 "ascend/compiler/ascParse.c"
    break;

  case 428: /* expr: SUM_TOK '(' set ')'  */
#line 3911 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
#line 6728 "ascend/compiler/ascParse.c"
    break;

  case 429: /* expr: SUM_TOK '[' set ']'  */
#line 3918 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_sum,(yyvsp[-1].sptr));
	}
#line 6736 "ascend/compiler/ascParse.c"
    break;

  case 430: /* expr: PROD_TOK '(' set ')'  */
#line 3922 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
#line 6747 "ascend/compiler/ascParse.c"
    break;

  case 431: /* expr: PROD_TOK '[' set ']'  */
#line 3929 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_prod,(yyvsp[-1].sptr));
	}
#line 6755 "ascend/compiler/ascParse.c"
    break;

  case 432: /* expr: UNION_TOK '(' set ')'  */
#line 3933 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
#line 6766 "ascend/compiler/ascParse.c"
    break;

  case 433: /* expr: UNION_TOK '[' set ']'  */
#line 3940 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_union,(yyvsp[-1].sptr));
	}
#line 6774 "ascend/compiler/ascParse.c"
    break;

  case 434: /* expr: INTERSECTION_TOK '(' set ')'  */
#line 3944 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
#line 6785 "ascend/compiler/ascParse.c"
    break;

  case 435: /* expr: INTERSECTION_TOK '[' set ']'  */
#line 3951 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_inter,(yyvsp[-1].sptr));
	}
#line 6793 "ascend/compiler/ascParse.c"
    break;

  case 436: /* expr: CARD_TOK '(' set ')'  */
#line 3955 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
#line 6804 "ascend/compiler/ascParse.c"
    break;

  case 437: /* expr: CARD_TOK '[' set ']'  */
#line 3962 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_card,(yyvsp[-1].sptr));
	}
#line 6812 "ascend/compiler/ascParse.c"
    break;

  case 438: /* expr: CHOICE_TOK '(' set ')'  */
#line 3966 "ascend/compiler/ascParse.y"
        {
	  DestroySetList((yyvsp[-1].sptr));
	  (yyval.eptr) = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
#line 6823 "ascend/compiler/ascParse.c"
    break;

  case 439: /* expr: CHOICE_TOK '[' set ']'  */
#line 3973 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateBuiltin(e_choice,(yyvsp[-1].sptr));
	}
#line 6831 "ascend/compiler/ascParse.c"
    break;

  case 440: /* expr: IDENTIFIER_TOK '(' expr ')'  */
#line 3977 "ascend/compiler/ascParse.y"
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
#line 6857 "ascend/compiler/ascParse.c"
    break;

  case 441: /* expr: '(' expr ')'  */
#line 3999 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = (yyvsp[-1].eptr);
	}
#line 6865 "ascend/compiler/ascParse.c"
    break;

  case 442: /* relop: '='  */
#line 4006 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_equal);
	}
#line 6873 "ascend/compiler/ascParse.c"
    break;

  case 443: /* relop: '<'  */
#line 4010 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_less);
	}
#line 6881 "ascend/compiler/ascParse.c"
    break;

  case 444: /* relop: '>'  */
#line 4014 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greater);
	}
#line 6889 "ascend/compiler/ascParse.c"
    break;

  case 445: /* relop: LEQ_TOK  */
#line 4018 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_lesseq);
	}
#line 6897 "ascend/compiler/ascParse.c"
    break;

  case 446: /* relop: GEQ_TOK  */
#line 4022 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_greatereq);
	}
#line 6905 "ascend/compiler/ascParse.c"
    break;

  case 447: /* relop: NEQ_TOK  */
#line 4026 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_notequal);
	}
#line 6913 "ascend/compiler/ascParse.c"
    break;

  case 448: /* logrelop: BEQ_TOK  */
#line 4033 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_eq);
	}
#line 6921 "ascend/compiler/ascParse.c"
    break;

  case 449: /* logrelop: BNE_TOK  */
#line 4037 "ascend/compiler/ascParse.y"
        {
	  (yyval.eptr) = CreateOpExpr(e_boolean_neq);
	}
#line 6929 "ascend/compiler/ascParse.c"
    break;


#line 6933 "ascend/compiler/ascParse.c"

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

#line 4041 "ascend/compiler/ascParse.y"

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
