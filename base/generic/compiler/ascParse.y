/*
	ASCEND modelling environment

	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 1997 Benjamin Andrew Allan & Vicente Rico-Ramirez
	Copyright (C) 1998, 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//*
	Ascend Grammar file
	by Tom Epperly
	Version: $Revision: 1.23 $
	Version control file: $RCSfile: ascParse.y,v $
	Date last modified: $Date: 2000/01/25 02:25:59 $
	Last modified by: $Author: ballan $
*/
%{
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>		/* need DBL_MAX and LONG_MAX */
#include <float.h>		/* on a NeXT they are in here */

#include <compiler/parser.h>

#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/scanner.h>
#include <compiler/symtab.h>		/* the global string/symbol table */
#include <compiler/notate.h>		/* notes database wrapper */
#include <compiler/braced.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/functype.h>
#include <compiler/func.h>
#include <compiler/expr_types.h>
#include <compiler/name.h>
#include <compiler/nameio.h>
#include <compiler/instance_enum.h>
#include <compiler/extfunc.h>
#include <compiler/packages.h>
#include <compiler/sets.h>
#include <compiler/exprs.h>
#include <compiler/exprio.h>
#include <compiler/vlist.h>
#include <compiler/vlistio.h>		/* for debugging only */
#include <compiler/stattypes.h>
#include <compiler/slist.h>
#include <compiler/statement.h>
#include <compiler/statio.h>
#include <compiler/units.h>
#include <compiler/when.h>
#include <compiler/select.h>
#include <compiler/switch.h>
#include <compiler/proc.h>
#include <compiler/watchpt.h>
#include <compiler/module.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/type_descio.h>
#include <compiler/typedef.h>
#include <compiler/library.h>
#include <compiler/syntax.h>
#include <compiler/lexer.h>

/* 1 ==> expr can find missing , w/o  shift/reduce conflicts */
#define COMMAEXPR_NOTBUGGY 0 
#if COMMAEXPR_NOTBUGGY
#include <compiler/exprio.h>
#endif /* for CommaExpr if working. */

#ifndef lint
static CONST char ParserID[] = "$Id: ascParse.y,v 1.23 2000/01/25 02:25:59 ballan Exp $";
#endif

int g_compiler_warnings = 1;		/* level of whine to allow */

#include <compiler/redirectFile.h>
#ifndef ASCERR
# error "ASCERR not defined"
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
/*********************************************************************\
 * g_type_name is used by the scanner when closing a module to check if
 * the parser is in the middle of parsing a MODEL or ATOM type
 * definition.
 *      g_type_name == NULL implies that the parser is in between
 *                          definitions. This depends on proper
 *                          resets to NULL at END of type productions.
 *      g_type_name != NULL implies that the parser is in the middle
 *                          of a type definition and gives the name
 *                          of that type.
\*********************************************************************/

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

/*  Forward declaration of error message reporting
 *  functions provided at the end of this file.
 */
static void ErrMsg_Generic(CONST char *);
static void ErrMsg_CommaName(CONST char *, struct Name *);
#if COMMAEXPR_NOTBUGGY
static void ErrMsg_CommaExpr(CONST char *, struct Expr *);
#endif /* COMMAEXPR_NOTBUGGY */
static void ErrMsg_NullDefPointer(CONST char *);
static void ErrMsg_ProcTypeMissing(CONST char *, CONST char *);
static void ErrMsg_ProcsRejected(CONST char *, CONST char *);
static void ErrMsg_DuplicateProc(struct InitProcedure *);
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
#define DEBUG_NOTES(s) 0

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
%}

%union {
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
}

%token ADD_TOK ALIASES_TOK AND_TOK ANY_TOK AREALIKE_TOK ARETHESAME_TOK ARRAY_TOK ASSERT_TOK ATOM_TOK
%token BEQ_TOK BNE_TOK BREAK_TOK
%token CALL_TOK CARD_TOK CASE_TOK CHOICE_TOK CHECK_TOK CONDITIONAL_TOK CONSTANT_TOK
%token CONTINUE_TOK CREATE_TOK
%token DATA_TOK DECREASING_TOK DEFAULT_TOK DEFINITION_TOK DIMENSION_TOK
%token DIMENSIONLESS_TOK DO_TOK
%token ELSE_TOK END_TOK EXPECT_TOK EXTERNAL_TOK
%token FALSE_TOK FALLTHRU_TOK FIX_TOK FOR_TOK FREE_TOK FROM_TOK
%token GLOBAL_TOK
%token IF_TOK IMPORT_TOK IN_TOK INPUT_TOK INCREASING_TOK INTERACTIVE_TOK
%token INTERSECTION_TOK ISA_TOK _IS_T ISREFINEDTO_TOK
%token MAXIMIZE_TOK MAXINTEGER_TOK MAXREAL_TOK METHODS_TOK METHOD_TOK MINIMIZE_TOK MODEL_TOK
%token NOT_TOK NOTES_TOK
%token OF_TOK OR_TOK OTHERWISE_TOK OUTPUT_TOK
%token PATCH_TOK PROD_TOK PROVIDE_TOK
%token REFINES_TOK REPLACE_TOK REQUIRE_TOK RETURN_TOK RUN_TOK
%token SATISFIED_TOK SELECT_TOK SIZE_TOK STOP_TOK SUCHTHAT_TOK SUM_TOK SWITCH_TOK
%token THEN_TOK TRUE_TOK
%token UNION_TOK UNITS_TOK UNIVERSAL_TOK
%token WHEN_TOK WHERE_TOK WHILE_TOK WILLBE_TOK WILLBETHESAME_TOK WILLNOTBETHESAME_TOK
%token ASSIGN_TOK CASSIGN_TOK DBLCOLON_TOK USE_TOK LEQ_TOK GEQ_TOK NEQ_TOK
%token DOTDOT_TOK WITH_TOK VALUE_TOK WITH_VALUE_T
%token <real_value> REAL_TOK
%token <int_value> INTEGER_TOK
%token <id_ptr> IDENTIFIER_TOK
%token <braced_ptr> BRACEDTEXT_TOK
%token <sym_ptr> SYMBOL_TOK
%token <dquote_ptr> DQUOTE_TOK

/* Set associativities */
%left ',' '|' SUCHTHAT_TOK
%left BEQ_TOK BNE_TOK
%left AND_TOK OR_TOK IN_TOK
%left '<' '=' '>' LEQ_TOK GEQ_TOK NEQ_TOK
%left '+' '-'
%left '/' '*'
%left UMINUS_TOK UPLUS_TOK
%right '^'
%left NOT_TOK
%start definitions

%type <real_value> default_val number realnumber opunits
%type <int_value> end optional_sign universal 
%type <fkind> forexprend
%type <frac_value> fraction fractail
%type <id_ptr> optional_of optional_method type_identifier call_identifier
%type <dquote_ptr> optional_notes
%type <braced_ptr> optional_bracedtext
%type <nptr> data_args fname name optional_scope
%type <eptr> relation expr relop logrelop optional_with_value
%type <sptr> set setexprlist optional_set_values
%type <lptr> fvarlist input_args output_args varlist
%type <statptr> statement isa_statement willbe_statement aliases_statement
%type <statptr> is_statement isrefinedto_statement arealike_statement
%type <statptr> arethesame_statement willbethesame_statement
%type <statptr> willnotbethesame_statement assignment_statement
%type <statptr> relation_statement glassbox_statement blackbox_statement
%type <statptr> call_statement units_statement
%type <statptr> external_statement for_statement run_statement if_statement assert_statement fix_statement free_statement
%type <statptr> when_statement use_statement select_statement
%type <statptr> conditional_statement notes_statement
%type <statptr> flow_statement while_statement
%type <statptr> switch_statement
%type <slptr> fstatements global_def optional_else
%type <slptr> optional_model_parameters optional_parameter_reduction
%type <slptr> optional_parameter_wheres
%type <septr> selectlist selectlistf
%type <swptr> switchlist switchlistf
%type <wptr> whenlist whenlistf
%type <notesptr> notes_body noteslist
%type <listp> methods proclist proclistf statements unitdeflist
%type <procptr> procedure
%type <dimp> dims dimensions
%type <dimen> dimexpr
%type <order> optional_direction
%type <tptr> add_method_head replace_method_head
%type <udefptr> unitdef
%type <id_ptr> model_id atom_id procedure_id definition_id

/* stuff without a particular need for a type */

%%

definitions:
    /* empty */
    | definitions definition
    ;

definition:
    require_file
    | provide_module
    | import
    | add_method_def
    | replace_method_def
    | add_notes_def
    | constant_def
    | atom_def
    | model_def
    | definition_def
    | patch_def
    | units_def
    | global_def
    | error
	{
	  ErrMsg_Generic("Error in definition.");
	}
    ;

global_def:
    GLOBAL_TOK ';' fstatements end ';'
	{
          /* the following steps apply to string buffers only, not files */
	  struct gl_list_t *stats;
          int dispose;
	  if ($3 != NULL) {
	    stats = gl_create(1L);
	    gl_append_ptr(stats,(void *)$3);
	    if (g_untrapped_error) {
	      ErrMsg_Generic("Because of a syntax error, the following statements are being ignored:");
		WriteStatementList(ASCERR,$3,4);
	      DestroyStatementList($3);
            } else {
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
		  WriteStatementList(ASCERR,$3,4);
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
    ;

require_file:
    REQUIRE_TOK DQUOTE_TOK ';'
	{
	  Asc_ScannerPushBuffer($2);
	}
    | REQUIRE_TOK name ';'
	{
	  DestroyName($2);
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    | REQUIRE_TOK name
	{
	  DestroyName($2);
	  ErrMsg_Generic("REQUIRE statement syntax is 'REQUIRE \"filename\";'.");
	}
    ;

provide_module:
    PROVIDE_TOK DQUOTE_TOK ';'
	{
          Asc_ModuleCreateAlias(Asc_CurrentModule(),$2);
        }
    | PROVIDE_TOK name ';'
	{
	  DestroyName($2);
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    | PROVIDE_TOK name
	{
	  DestroyName($2);
	  ErrMsg_Generic("PROVIDE statement syntax is 'PROVIDE \"filename\";'.");
	}
    ;

import:
    IMPORT_TOK IDENTIFIER_TOK FROM_TOK DQUOTE_TOK ';'
	{
	  (void)LoadArchiveLibrary($4,SCP($2));
	}
	| IMPORT_TOK DQUOTE_TOK ';'
	{
	  (void)LoadArchiveLibrary(SCP($2),NULL);
	}
    ;

add_notes_def:
    add_notes_head notes_body end ';'
	{
	  /*  see comments for notes statement.  */
	  if( $3 != NOTES_TOK ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, $3, NULL);
	  }
	  if ($2 != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = $2;
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
	    DestroyNoteTmpList($2);
          }
          g_type_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
    ;

add_notes_head:
    ADD_TOK NOTES_TOK IN_TOK IDENTIFIER_TOK optional_method ';'
	{
	  g_type_name = $4;
	  g_proc_name = $5;
	}
    ;

add_method_def:
    add_method_head proclist end ';'
	{
	  if ($1 == NULL) {
	    DestroyProcedureList($2);
	  } else {
	    if( $3 != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("ADD METHODS", NULL, $3, "METHODS");
	    }
	    if (AddMethods($1,$2,g_untrapped_error) != 0) {
	      if ($1 != ILLEGAL_DEFINITION) {
                ErrMsg_ProcsRejected("ADD",SCP(GetName($1)));
	        DestroyProcedureList($2);
	      } /* else adding in DEFINITION MODEL may have misgone */
	    }
	  }
	  g_untrapped_error = 0;
	}
    ;

add_method_head:
    ADD_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'
	{
	  struct TypeDescription *tmptype;
	  tmptype = FindType($4);
	  if (tmptype == NULL) {
            ErrMsg_ProcTypeMissing("ADD", SCP($4));
	  }
	  $$ = tmptype; /* parent should check for NULL */
	  g_type_name = $4; /* scope for notes */
	}
    | ADD_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'
	{
	  $$ = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    ;

replace_method_def:
    replace_method_head proclist end ';'
	{
	  if ($1 == NULL) {
	    DestroyProcedureList($2);
	  } else {
	    if( $3 != METHODS_TOK ) {
	      WarnMsg_MismatchEnd("REPLACE METHODS", NULL, $3, "METHODS");
	    }
	    if (ReplaceMethods($1,$2,g_untrapped_error) != 0) {
              ErrMsg_ProcsRejected("REPLACE",SCP(GetName($1)));
	      DestroyProcedureList($2);
	    }
	  }
	  g_untrapped_error = 0;
	}
    ;

replace_method_head:
    REPLACE_TOK METHODS_TOK IN_TOK IDENTIFIER_TOK ';'
	{
	  struct TypeDescription *tmptype;
	  tmptype = FindType($4);
	  if (tmptype == NULL) {
            ErrMsg_ProcTypeMissing("REPLACE", SCP($4));
	  }
	  $$ = tmptype; /* parent should check for NULL */
	}
    | REPLACE_TOK METHODS_TOK IN_TOK DEFINITION_TOK MODEL_TOK ';'
	{
	  $$ = ILLEGAL_DEFINITION;
	  /* need a bit of global state here to tag base methods */
	}
    ;

atom_def:
    universal atom_head fstatements methods end ';'
	{
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

          if(( $5 != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("ATOM", SCP(g_type_name),
	                        $5, SCP(g_type_name));
	  }
	  g_atom_dim_ptr = CheckDimensionsMatch(g_default_dim_ptr,
	                                        g_atom_dim_ptr);
	  if (g_atom_dim_ptr != NULL) {
	    def_ptr = CreateAtomTypeDef(g_type_name,
	                                g_refines_name,
	                                real_type, /* ignored..really */
	                                Asc_CurrentModule(),
	                                $1,
	                                $3,
	                                $4,
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
	    error_reporter(ASC_USER_ERROR,Asc_ModuleBestName(Asc_CurrentModule()),g_header_linenum,NULL,
	            "Atom dimensions don't match in ATOM %s on line %s:%lu.\n",
	            SCP(g_type_name),
	            Asc_ModuleBestName(Asc_CurrentModule()),
	            g_header_linenum);
	    DestroyStatementList($3);
	    DestroyProcedureList($4);
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = g_refines_name = g_proc_name = NULL;
	  g_untrapped_error = 0;
	}
    ;

atom_head:
    atom_id REFINES_TOK IDENTIFIER_TOK dims default_val ';'
	{
	  /* g_type_name = $1; */
	  g_refines_name = $3;
	  g_atom_dim_ptr = $4;
	  g_default_double = $5;
	  g_header_linenum = LineNum();
	}
    ;

atom_id:
    ATOM_TOK IDENTIFIER_TOK 
	{
	  $$ = $2;
	  g_type_name = $2; /* want this set early so parm lists see it */
	}
    ;

default_val:
    /* empty */
	{
	  $$ = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    | DEFAULT_TOK optional_sign number
	{
	  $$ = $2 ? -$3 : $3;
	  g_defaulted = 1;
	}
    | DEFAULT_TOK FALSE_TOK
	{
	  $$ = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 0;
	  g_defaulted = 1;
	}
    | DEFAULT_TOK TRUE_TOK
	{
	  $$ = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_long = 1;
	  g_defaulted = 1;
	}
    | DEFAULT_TOK SYMBOL_TOK
	{
	  $$ = 0.0;
	  g_default_dim_ptr = Dimensionless();
	  g_default_symbol = $2;
	  g_defaulted = 0;
	}
    ;

constant_def:
    universal constant_head
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
	                                    $1,
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
    ;

constant_head:
    CONSTANT_TOK IDENTIFIER_TOK REFINES_TOK IDENTIFIER_TOK dims constant_val
    optional_notes ';'
	{
	  g_type_name = $2;
	  g_refines_name = $4;
	  g_atom_dim_ptr = $5;
	  switch (g_constant_type) {
	  case DOUBLECONSTANT:
	    g_default_double = $<real_value>6;
	    break;
	  case LONGCONSTANT:
	    g_default_long = $<real_value>6;
	    break;
	  case BOOLEANCONSTANT:
	    g_default_long = $<int_value>6;
	    break;
	  case SYMBOLCONSTANT:
	    g_default_symbol = $<sym_ptr>6;
	    break;
	  default:
	    ErrMsg_Generic("Wierd constant type assign encountered.");
	    break; /* better not be reached. */
	  }
	  g_header_linenum = LineNum();
	  if ($7 != NULL) {
	    CollectNote(CreateNote(g_type_name,InlineNote(),SelfNote(),NULL,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar($7,InlineNote()),
	                           g_header_linenum,NULL,nd_empty));
	  }
	}
    ;

constant_val:
    /* empty */
	{
	  $<real_value>$ = 0.0;
	  g_default_dim_ptr = WildDimension();
	  g_defaulted = 0;
	}
    | CASSIGN_TOK optional_sign number
	{
	  $<real_value>$ = $2 ? -$3 : $3;
	  g_defaulted = 1;
	}
    | CASSIGN_TOK TRUE_TOK
	{
	  $<int_value>$ = 1;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    | CASSIGN_TOK FALSE_TOK
	{
	  $<int_value>$ = 0;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = BOOLEANCONSTANT;
	}
    | CASSIGN_TOK SYMBOL_TOK
	{
	  $<sym_ptr>$ = $2;
	  g_defaulted = 1;
	  g_default_dim_ptr = Dimensionless();
	  g_constant_type = SYMBOLCONSTANT;
	}
    ;

model_def:
    universal model_head fstatements methods end ';'
	{
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;
	  if(( $5 != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("MODEL", SCP(g_type_name),
	                        $5, SCP(g_type_name));
	  }
	  def_ptr = CreateModelTypeDef(g_type_name,
	                               g_refines_name,
	                               Asc_CurrentModule(),
	                               $1,
	                               $3,
	                               $4,
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
    ;

model_head:
    model_id optional_model_parameters
    optional_parameter_wheres ';'
	{
	  /* g_type_name = $1; */
	  g_model_parameters = $2;
	  g_parameter_wheres = $3;
	  g_refines_name = NULL;
	  g_header_linenum = LineNum();
	}
    | model_id optional_model_parameters optional_parameter_wheres
    REFINES_TOK IDENTIFIER_TOK optional_parameter_reduction ';'
	{
	  /* g_type_name = $1; */
	  g_model_parameters = $2;
	  g_parameter_wheres = $3;
	  g_refines_name = $5;
	  g_parameter_reduction = $6;
	  g_header_linenum = LineNum();
	}
    ;

model_id:
    MODEL_TOK IDENTIFIER_TOK 
	{
	  $$ = $2;
	  g_type_name = $2; /* want this set early so parm lists see it */
	}
    ;

optional_model_parameters:
    /* empty */
	{
	  $$ = NULL;
	}
    |  '(' fstatements ')'
	{
	  $$ = $2; /* this could be much more sophisticated */
	}
    ;

optional_parameter_wheres:
    /* empty */
	{
	  $$ = NULL;
	}
    | WHERE_TOK '(' fstatements ')'
	{
	  $$ = $3; /* this could be much more sophisticated */
	}
    ;

optional_parameter_reduction:
    /* empty */
	{
	  $$ = NULL;
	}
    | '(' fstatements ')'
	{
	  $$ = $2; /* this could be much more sophisticated */
	}
    ;

patch_def:
    patch_head fstatements methods end ';'
	{
	  struct TypeDescription *def_ptr;
	  if (($4 != IDENTIFIER_TOK ) || ( g_end_identifier != g_type_name )) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("PATCH", SCP(g_type_name),
	                        $4, SCP(g_type_name));
	  }
	  def_ptr = CreatePatchTypeDef(g_type_name,
	                               g_refines_name,
	                               NULL,
	                               Asc_CurrentModule(),
	                               $2,
	                               $3,
	                               g_untrapped_error);
	  g_untrapped_error = 0;
	  if (def_ptr != NULL) {
	    AddType(def_ptr);
	  } else {
	    /* CreatePatchTypeDef is responsible for freeing (if needed)
	     * all args sent to it so we don't have to here.
	     * in particular $2 $3
	     */
	    ErrMsg_NullDefPointer(SCP(g_type_name));
	  }
	  g_type_name = g_refines_name = g_proc_name = NULL;
	}
    ;

patch_head:
    PATCH_TOK IDENTIFIER_TOK FOR_TOK IDENTIFIER_TOK ';'
	{
	  /*
	   * A patch definition looks just like a model def.
	   * with the original name <=> refine name.
	   */
	  g_type_name = $2;
	  g_refines_name = $4;
	  g_header_linenum = LineNum();
	}
    ;

universal:
    /* empty */
	{
	  $$ = 0;
	}
    | UNIVERSAL_TOK
	{
	  $$ = 1;
	}
    ;

definition_def:
    definition_id fstatements methods end ';'
	{
	  struct TypeDescription *def_ptr;
	  int keepnotes = 0;

	  if(( $4 != IDENTIFIER_TOK ) || ( g_end_identifier != $1 )) {
	    WarnMsg_MismatchEnd("DEFINITION", SCP($1), $4, SCP($1));
	  }
	  if( $1 == GetBaseTypeName(relation_type)) {
	    def_ptr = CreateRelationTypeDef(Asc_CurrentModule(),$1,$2,$3);
	  }
	  else if( $1 == GetBaseTypeName(logrel_type) ) {
	    def_ptr = CreateLogRelTypeDef(Asc_CurrentModule(),$1,$2,$3);
	  }
	  else {
	    ErrMsg_Generic("Bad type passed to DEFINITION statement.");
	    def_ptr = NULL;
	  }
	  if ( def_ptr != NULL ) {
	    keepnotes = AddType(def_ptr);
	  } else {
	    ErrMsg_NullDefPointer(SCP($1));
	  }
	  ProcessNotes(keepnotes);
	  g_type_name = NULL;
	  g_untrapped_error = 0;
	}
    ;

definition_id:
    DEFINITION_TOK IDENTIFIER_TOK
	{
	  $$ = $2;
	  g_type_name = $2; /* want this set early so parm lists see it */
	}
    ;


units_def:
    units_statement ';'
        { /* nothing to do. just cruft to fix ; problem */ }
    ;

units_statement:
    UNITS_TOK unitdeflist end
	{
          struct UnitDefinition *ud;
          unsigned long c,len;

	  if( $3 != UNITS_TOK ) {
	    WarnMsg_MismatchEnd("UNITS", NULL, $3, NULL);
	  }
          len = gl_length($2);
          for (c=1; c <= len; c++) {
            ud = (struct UnitDefinition *)gl_fetch($2,c);
            ProcessUnitDef(ud);
            DestroyUnitDef(ud);
          }
          gl_destroy($2);
          $$ = NULL;
	}
    ;

unitdeflist:
	{
	  $$ = gl_create(100L);
	}
    | unitdeflist unitdef
	{
	  gl_append_ptr($1,(char *)$2);
	  $$ = $1;
	}
    ;

unitdef:
    IDENTIFIER_TOK '=' BRACEDTEXT_TOK ';'
	{
	  $$ = CreateUnitDef($1,$3,Asc_ModuleBestName(Asc_CurrentModule()),
                             LineNum());
	}
    ;


methods:
    /* empty */
	{
	  $$ = NULL;
	}
    | METHODS_TOK
	{ /* To get rid of this, we will need a global proclist
           * that accumulates procs until a MODEL production is
           * completed. If any other sort of production is started,
           * and proclist is not NULL, it should be discarded.
           */
	}
    proclist
	{
	  $$ = $3;
	}
    ;

proclist:
    proclistf
	{
	  $$ = $1;
	  gl_sort($$,(CmpFunc)CmpProcs);
	}
    ;

proclistf:
	{
	  $$ = gl_create(7L);
	}
    | proclistf procedure
	{
	  unsigned long c;
	  struct InitProcedure *oldproc;
	  c = gl_length($1);
          while (c > 0) {
            oldproc = (struct InitProcedure *)gl_fetch($1,c);
            if (ProcName($2) == ProcName(oldproc)) {
	      ErrMsg_DuplicateProc($2);
              break;
            }
            c--;
          }
	  if (c) { /* broke early */
	    DestroyProcedure($2);
	  } else {
	    gl_append_ptr($1,(char *)$2);
	  }
	  $$ = $1;
	}
    ;

procedure:
    procedure_id ';' fstatements end ';'
	{
	  if (($4 != IDENTIFIER_TOK) || ($1 != g_end_identifier)) {
	    /* all identifier_t are from symbol table, so ptr match
	     * is sufficient for equality.
	     */
	    WarnMsg_MismatchEnd("METHOD", SCP($1), $4, SCP($1));
	  }
	  $$ = CreateProcedure($1,$3);
	  g_proc_name = NULL;
	}
    ;

procedure_id:
    METHOD_TOK IDENTIFIER_TOK
	{
	  $$ = $2;
	  g_proc_name = $2;
	}
    ;


fstatements:
    statements
	{
	  $$ = CreateStatementList($1);
	}
    ;

statements:
    /* empty */
	{
	  $$ = gl_create(7L);
	}
    | statements statement ';'
	{
	  /* this is appending to a gllist of statements, not yet slist. */
	  if ($2 != NULL) {
	    gl_append_ptr($1,(char *)$2);
	  }
	  $$ = $1;
	}
    | statements error ';'
	{
	  ErrMsg_Generic("Error in statement input.");
	  $$ = $1;
	}
    ;

statement:
    isa_statement
    | willbe_statement
    | aliases_statement
    | is_statement
    | isrefinedto_statement
    | arealike_statement
    | arethesame_statement
    | willbethesame_statement
    | willnotbethesame_statement
    | assignment_statement
    | relation_statement
    | glassbox_statement
    | blackbox_statement
    | call_statement
    | external_statement
    | for_statement
    | run_statement
    | fix_statement
    | free_statement
    | assert_statement
    | if_statement
    | while_statement
    | when_statement
    | use_statement
    | flow_statement
    | select_statement
    | switch_statement
    | conditional_statement
    | notes_statement
    | units_statement
    ;

isa_statement:
    fvarlist ISA_TOK type_identifier optional_of optional_with_value
	{
	  struct TypeDescription *tmptype;
	  tmptype = FindType($3);
	  if ($5 != NULL) {
	    ErrMsg_Generic("WITH VALUE clause not allowed in IS_A.");
	    g_untrapped_error++;
	    DestroyVariableList($1);
	    DestroySetList(g_typeargs);
	    DestroyExprList($5);
	    $$ = NULL;
	  } else {
	    if (tmptype != NULL) {
	      if ((GetBaseType(tmptype) != model_type) &&
	          (g_typeargs != NULL)) {
	        error_reporter_current_line(ASC_USER_ERROR,
	                "IS_A has arguments to the nonmodel type %s.\n",
	                SCP($3));
	        DestroyVariableList($1);
	        DestroySetList(g_typeargs);
	        DestroyExprList($5);
	        g_untrapped_error++;
	        $$ = NULL;
	      } else {
	        $$ = CreateISA($1,$3,g_typeargs,$4);
	      }
	    } else {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_A uses the undefined type %s.", SCP($3));
	      DestroyVariableList($1);
	      DestroySetList(g_typeargs);
	      DestroyExprList($5);
	      g_untrapped_error++;
	      $$ = NULL;
	    }
	  }
	  g_typeargs = NULL;

	}
    ;

willbe_statement:
    fvarlist WILLBE_TOK type_identifier optional_of optional_with_value
	{
	  struct TypeDescription *tmptype;
	  tmptype = FindType($3);
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) &&
	        (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"WILL_BE has arguments to the nonmodel type '%s'",SCP($3));
	      DestroyVariableList($1);
	      DestroySetList(g_typeargs);
	      DestroyExprList($5);
	      g_untrapped_error++;
	      $$ = NULL;
	    } else {
	      $$ = CreateWILLBE($1,$3,g_typeargs,$4,$5);
	    }
	  } else {
	    DestroyVariableList($1);
	    DestroySetList(g_typeargs);
	    DestroyExprList($5);
	    g_untrapped_error++;
	    $$ = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"WILL_BE uses the undefined type %s.",SCP($3));
	  }
	  g_typeargs = NULL;
	}
    ;

aliases_statement:
    fvarlist ALIASES_TOK fname
	{
	  $$ = CreateALIASES($1,$3);
	}
    | fvarlist ALIASES_TOK '(' fvarlist ')' WHERE_TOK fvarlist ISA_TOK
    IDENTIFIER_TOK OF_TOK IDENTIFIER_TOK optional_set_values
	{
	  int carray_err;
	  carray_err = 0;
	  if (VariableListLength($1) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES allows only 1 LHS name. Found:");
	    WriteVariableList(ASCERR,$1);
	  }
	  if (VariableListLength($7) != 1L) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES/IS_A allows only one LHS name. Found:");
	    WriteVariableList(ASCERR,$7);
	  }
	  /* verify $9 == "set" */
	  if (!carray_err && $9 != GetBaseTypeName(set_type)) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,"Compound ALIASES statement requires IS_A %s. ",SCP(GetBaseTypeName(set_type)));
	    FPRINTF(ASCERR,"    Found %s.\n",SCP($9));
	  }
	  /* verify set type */
	  if ((!carray_err) &&
	      ($11 != GetBaseTypeName(symbol_constant_type)) &&
	      ($11 != GetBaseTypeName(integer_constant_type))) {
	    carray_err = 1;
	    error_reporter_current_line(ASC_USER_ERROR,
	            "Compound ALIASES IS_A statement requires %s or %s.\n",
	            SCP(GetBaseTypeName(integer_constant_type)),
	            SCP(GetBaseTypeName(symbol_constant_type)));
	    FPRINTF(ASCERR,"	Found %s.\n",SCP($11));
	  }
	  if (carray_err) {
	    DestroyVariableList($1);
	    DestroyVariableList($4);
	    DestroyVariableList($7);
	    DestroySetList($12);
	    g_untrapped_error++;
	    $$ = NULL;
	  } else {
	    int intset;
	    intset = ($11 == GetBaseTypeName(integer_constant_type));
	    $$ = CreateARR($1,$4,$7,intset,$12);
	  }
	}
    ;

optional_set_values:
    /* empty */
	{
	  $$ = NULL;
	}
    | WITH_VALUE_T '(' set ')'
	{
	  $$ = $3;
	}
    ;

is_statement:
    fvarlist _IS_T IDENTIFIER_TOK optional_of
	{
	  if (FindType($3)) {
	    $$ = CreateREF($1,$3,$4,1);
	  } else {
	    $$ = CreateREF($1,$3,$4,1);
	    error_reporter_current_line(ASC_USER_WARNING,"_IS_ uses the unbuilt prototype %s.\n",SCP($3));
	  }
	}
    ;

isrefinedto_statement:
    fvarlist ISREFINEDTO_TOK type_identifier
	{
	  struct TypeDescription *tmptype;
	  tmptype = FindType($3);
	  if (tmptype != NULL) {
	    if ((GetBaseType(tmptype) != model_type) && 
                (g_typeargs != NULL)) {
	      error_reporter_current_line(ASC_USER_ERROR,"IS_REFINED_TO has arguments to the nonmodel type %s.",SCP($3));
	      DestroyVariableList($1);
	      DestroySetList(g_typeargs);
	      g_untrapped_error++;
	      $$ = NULL;
	    } else {
	      $$ = CreateIRT($1,$3,g_typeargs);
	    }
	  } else {
	    error_reporter_current_line(ASC_USER_ERROR,"The IS_REFINED_TO uses the undefined type %s.\n",SCP($3));
	    DestroyVariableList($1);
	    DestroySetList(g_typeargs);
	    g_untrapped_error++;
	    $$ = NULL;
	  }
	  g_typeargs = NULL;
	}
    ;

call_identifier:
    IDENTIFIER_TOK
	{
	  $$ = $1;
	  g_callargs = NULL;
	}
    | IDENTIFIER_TOK '(' set ')'
	{
	  $$ = $1;
	  g_callargs = $3;
	}
    ;

type_identifier:
    IDENTIFIER_TOK
	{
	  $$ = $1;
	  g_typeargs = NULL;
	}
    | IDENTIFIER_TOK '(' set ')'
	{
	  $$ = $1;
	  g_typeargs = $3;
	}
    ;

optional_method:
    /* empty */
	{
	  $$ = NULL;
	}
    | METHOD_TOK IDENTIFIER_TOK
	{
	  $$ = $2;
	}
    ;

optional_of:
    /* empty */
	{
	  $$ = NULL;
	}
    | OF_TOK IDENTIFIER_TOK
	{
	  $$ = $2;
	}
    ;

optional_with_value:
    /* empty */
	{
	  $$ = NULL;
	}
    | WITH_VALUE_T expr
	{
	  $$ = $2;
	}
    ;

arealike_statement:
    fvarlist AREALIKE_TOK
	{
	  $$ = CreateAA($1);
	}
    ;

arethesame_statement:
    fvarlist ARETHESAME_TOK
	{
	  $$ = CreateATS($1);
	}
    ;

willbethesame_statement:
    fvarlist WILLBETHESAME_TOK
	{
	  $$ = CreateWBTS($1);
	}
    ;

willnotbethesame_statement:
    fvarlist WILLNOTBETHESAME_TOK
	{
	  $$ = CreateWNBTS($1);
	}
    ;

assignment_statement:
    fname ASSIGN_TOK expr
	{
	  $$ = CreateASSIGN($1,$3);
	}
    | fname CASSIGN_TOK expr
	{
	  $$ = CreateCASSIGN($1,$3);
	}
    ;

relation_statement:
    relation
	{
	  if (IsRelation($1)) {
	    if (g_parse_relns == 0) {
	      DestroyExprList($1);
	      $$ = NULL;
	    } else {
	      $$ = CreateREL(NULL,$1);
	    }
	  } else {
	    $$ = CreateLOGREL(NULL,$1);
	  }
	}
    | fname ':' relation
	{
	  if (IsRelation($3)) {
	    if (g_parse_relns == 0) {
	      DestroyExprList($3);
	      DestroyName($1);
	      $$ = NULL;
	    } else {
	      $$ = CreateREL($1,$3);
	    }
	  } else {
	    $$ = CreateLOGREL($1,$3);
	  }
	}
    ;

relation:
    expr
	{
	  $$ = $1;
	  if (NumberOfRelOps($1) < 1) {
	    /* want at least 1. restriction to exactly 1 is in typelint */
	    ErrMsg_Generic("Missing punctuation (,;:) or else expression contains the \
wrong number of relation operators (=, ==, <, >, <=, >=, !=) preceeding or.");
	    g_untrapped_error++;
	  }
	}
    | MINIMIZE_TOK expr
	{
	  $$ = JoinExprLists($2,CreateOpExpr(e_minimize));
	  if (NumberOfRelOps($2) > 0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    | MAXIMIZE_TOK expr
	{
	  $$ = JoinExprLists($2,CreateOpExpr(e_maximize));
	  if (NumberOfRelOps($2)>0) {
	    ErrMsg_Generic("Objective function contains relation operators (=, ==, <, >, <=, >=, !=).");
	    g_untrapped_error++;
	  }
	}
    ;

blackbox_statement:
    fname ':' IDENTIFIER_TOK '(' input_args ';' output_args data_args ')'
	{
	  /*
	   * This is the blackbox declarative external relation.
	   */
	  struct VariableList *vl;
	  vl = JoinVariableLists($5,$7);
	  /* $$ = CreateEXTERN(2,$1,SCP($3),vl,$8,NULL); */
	  $$ = CreateEXTERNBlackBox($1,SCP($3),vl,$8);
	}
    ;

input_args:
    fvarlist ':' INPUT_TOK
	{
	  $$ = $1;
	}
    ;

output_args:
    fvarlist ':' OUTPUT_TOK
	{
	  $$ = $1;
	}
    ;

data_args:
    /* empty */
	{
	  $$ = NULL;
	}
    | ';' fname ':' DATA_TOK
	{
	  $$ = $2;
	}
    ;

glassbox_statement:
    fname ':' IDENTIFIER_TOK '(' fvarlist ';' INTEGER_TOK ')' optional_scope
	{
	  /*
	   * This is the glassbox declarative external relation.
	   * This now allows a scope for placement of the relations
	   */
	  struct VariableList *vl = $5;
	  struct Name *nptr;
	  char tmp[32]; 
	  symchar *str;

	  sprintf(tmp,"%ld",$7);
	  str = AddSymbol(tmp);
	  nptr = CreateIdName(str);
	/* $$ = CreateEXTERN(1,$1,SCP($3),vl,nptr,$9); */
	  $$ = CreateEXTERNGlassBox($1,SCP($3),vl,nptr,$9);
	}
    ;

optional_scope:
    /* empty */
	{
	  $$ = NULL;
	}
    | IN_TOK fname
	{
	  $$ = $2;
	}
    ;

for_statement:
    FOR_TOK IDENTIFIER_TOK IN_TOK expr optional_direction forexprend 
    fstatements end
	{
	  if( $8 != FOR_TOK ) {
	    WarnMsg_MismatchEnd("FOR", SCP($2), $8, NULL);
	  }
          if ($6 == fk_create && $5 != f_random) {
            /* create cannot have an order in declarative FOR */
	    ErrMsg_Generic("FOR loops only accept DECREASING or INCREASING in the method section.");
	    g_untrapped_error++;
          }
          if ($6 == fk_do && $5 == f_random) {
            /* all FOR/DO default to increasing */
	    $$ = CreateFOR($2,$4,$7,f_increasing,$6);
          } else {
	    $$ = CreateFOR($2,$4,$7,$5,$6);
          }
	}
    ;

optional_direction:
    /* empty */
	{
	  $$ = f_random;
	}
    | INCREASING_TOK
	{
	  $$ = f_increasing;
	}
    | DECREASING_TOK
	{
	  $$ = f_decreasing;
	}
    ;

forexprend:
    CREATE_TOK
	{
          $$ = fk_create; /* declarative FOR */
	}
    | EXPECT_TOK
	{
          $$ = fk_expect; /* parameter FOR */
	}
    | CHECK_TOK
	{
          $$ = fk_check; /* WHERE FOR */
	}
    | DO_TOK
	{
          $$ = fk_do; /* method FOR */
	}
    ;

run_statement:
    RUN_TOK fname
	{
	  $$ = CreateRUN($2,NULL);
	}
    | RUN_TOK fname DBLCOLON_TOK fname
	{
	  $$ = CreateRUN($4,$2);	  /* type :: name */
	}
    ;

fix_statement:
	FIX_TOK fvarlist
	{
		$$ = CreateFIX($2);
	}
	;

free_statement:
	FREE_TOK fvarlist
	{
		$$ = CreateFREE($2);
	}
	;

external_statement:
    EXTERNAL_TOK IDENTIFIER_TOK '(' fvarlist ')'
	{
	  /*
	   * This is procedural external code. Was:
	  $$ = CreateEXTERN(0,NULL,SCP($2),$4,NULL,NULL);
	   */
	  $$ = CreateEXTERNMethod(SCP($2),$4);
	}
    ;

call_statement:
    CALL_TOK call_identifier
	{
	  /*
	   * This is proper procedural external method code.
	   */
	  $$ = CreateCALL($2,g_callargs);
          g_callargs = NULL;
	}
    ;

assert_statement:
	ASSERT_TOK expr
	{
		$$ = CreateASSERT($2);
	}	

if_statement:
    IF_TOK expr THEN_TOK fstatements optional_else end
	{
	  if( $6 != IF_TOK ) {
	    WarnMsg_MismatchEnd("IF", NULL, $6, NULL);
	  }
	  $$ = CreateIF($2,$4,$5);
	}
    ;

while_statement:
    WHILE_TOK expr DO_TOK fstatements end
	{
	  if( $5 != WHILE_TOK ) {
	    WarnMsg_MismatchEnd("WHILE", NULL, $5, NULL);
	  }
	  $$ = CreateWhile($2,$4);
	}
    ;

optional_else:
	{
	  $$ = NULL;
	}
    | ELSE_TOK fstatements
	{
	  $$ = $2;
	}
    ;

when_statement:
    WHEN_TOK fvarlist whenlist end
	{
	  if( $4 != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, $4, NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList($3);
	  DestroyVariableList($2);
	  g_untrapped_error++;
	  $$ = NULL;
	}
    | fname ':' WHEN_TOK fvarlist whenlist end
	{
	  if( $6 != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, $6, NULL);
	  }
	  ErrMsg_Generic("() missing in WHEN statement.");
	  DestroyWhenList($5);
	  DestroyVariableList($4);
	  DestroyName($1);
	  g_untrapped_error++;
	  $$ = NULL;
	}
    | WHEN_TOK '(' fvarlist ')' whenlist end
	{
	  if( $6 != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, $6, NULL);
	  }
	  $$ = CreateWHEN(NULL,$3,$5);
	}
    | fname ':' WHEN_TOK '(' fvarlist ')' whenlist end
	{
	  if( $8 != WHEN_TOK ) {
	    WarnMsg_MismatchEnd("WHEN", NULL, $8, NULL);
	  }
	  $$ = CreateWHEN($1,$5,$7);
	}
    ;

whenlist:
    whenlistf
	{
	  $$ = ReverseWhenCases($1);
	}
    ;

whenlistf:
    CASE_TOK set ':' fstatements
	{
	  $$ = CreateWhen($2,$4);
	}
    | OTHERWISE_TOK ':' fstatements
	{
	  $$ = CreateWhen(NULL,$3);
	}
    | whenlistf CASE_TOK set ':' fstatements
	{
	  $$ = LinkWhenCases(CreateWhen($3,$5),$1);
	}
    | whenlistf OTHERWISE_TOK ':' fstatements
	{
	  $$ = LinkWhenCases(CreateWhen(NULL,$4),$1);
	}
    ;

flow_statement:
    BREAK_TOK 
	{
	  $$ = CreateFlow(fc_break,NULL);
	}
    | CONTINUE_TOK
	{
	  $$ = CreateFlow(fc_continue,NULL);
	}
    | FALLTHRU_TOK
	{
	  $$ = CreateFlow(fc_fallthru,NULL);
	}
    | RETURN_TOK
	{
	  $$ = CreateFlow(fc_return,NULL);
	}
    | STOP_TOK optional_bracedtext
	{
	  $$ = CreateFlow(fc_stop,$2);
	}
    ;

use_statement:
    USE_TOK fname
	{
	  $$ = CreateFNAME($2);
	}
    ;

select_statement:
    SELECT_TOK fvarlist selectlist end
	{
	  if( $4 != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, $4, NULL);
	  }
	  ErrMsg_Generic("() missing in SELECT statement.");
	  DestroySelectList($3);
	  DestroyVariableList($2);
	  g_untrapped_error++;
	  $$ = NULL;
	}
    | SELECT_TOK '(' fvarlist ')' selectlist end
	{
	  if( $6 != SELECT_TOK ) {
	    WarnMsg_MismatchEnd("SELECT", NULL, $6, NULL);
	  }
	  $$ = CreateSELECT($3,$5);
	}
    ;

selectlist:
    selectlistf
	{
	  $$ = ReverseSelectCases($1);
	}
    ;

selectlistf:
    CASE_TOK set ':' fstatements
	{
	  $$ = CreateSelect($2,$4);
	}
    | OTHERWISE_TOK ':' fstatements
	{
	  $$ = CreateSelect(NULL,$3);
	}
    | selectlistf CASE_TOK set ':' fstatements
	{
	  $$ = LinkSelectCases(CreateSelect($3,$5),$1);
	}
    | selectlistf OTHERWISE_TOK ':' fstatements
	{
	  $$ = LinkSelectCases(CreateSelect(NULL,$4),$1);
	}
    ;

switch_statement:
    SWITCH_TOK fvarlist switchlist end
	{
	  if( $4 != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, $4, NULL);
	  }
	  ErrMsg_Generic("() missing in SWITCH statement.");
	  DestroySwitchList($3);
	  DestroyVariableList($2);
	  g_untrapped_error++;
	  $$ = NULL;
	}
    | SWITCH_TOK '(' fvarlist ')' switchlist end
	{
	  if( $6 != SWITCH_TOK ) {
	    WarnMsg_MismatchEnd("SWITCH", NULL, $6, NULL);
	  }
	  $$ = CreateSWITCH($3,$5);
	}
    ;

switchlist:
    switchlistf
	{
	  $$ = ReverseSwitchCases($1);
	}
    ;

switchlistf:
    CASE_TOK set ':' fstatements
	{
	  $$ = CreateSwitch($2,$4);
	}
    | OTHERWISE_TOK ':' fstatements
	{
	  $$ = CreateSwitch(NULL,$3);
	}
    | switchlistf CASE_TOK set ':' fstatements
	{
	  $$ = LinkSwitchCases(CreateSwitch($3,$5),$1);
	}
    | switchlistf OTHERWISE_TOK ':' fstatements
	{
	  $$ = LinkSwitchCases(CreateSwitch(NULL,$4),$1);
	}
    ;

conditional_statement:
    CONDITIONAL_TOK fstatements end
	{
	  if( $3 != CONDITIONAL_TOK ) {
	    WarnMsg_MismatchEnd("CONDITIONAL", NULL, $3, NULL);
	  }
	  $$ = CreateCOND($2);
	}
    ;

notes_statement:
    NOTES_TOK notes_body end
	{
	  /*  All processing of notes takes place on the notes_body here.
	   *  Notes should NOT be added to the statement list.
	   *  Here we know the current type and method names.
	   */
	  if( $3 != NOTES_TOK ) {
	    WarnMsg_MismatchEnd("NOTES", NULL, $3, NULL);
	  }
	  if ($2 != NULL) {
	    struct NoteTmp *nt;
	    symchar *lang=NULL; /* dummy */
	    nt = $2;
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
	    DestroyNoteTmpList($2);
          }
	  $$ = NULL;
	}
    ;

notes_body:
    SYMBOL_TOK noteslist
	{
	  /*  At this point we have the "language", the names of the
	   *  objects we are explaining, and the explanation/notes itself.
	   */
	  $$ = $2;
	  assert($$->lang == NULL);
	  $$->lang = $1;
	}
    | notes_body SYMBOL_TOK noteslist
	{
	  struct NoteTmp *nt;
	  $$ = $1;
	  assert($3->lang == NULL);
	  $3->lang = $2;
	  nt = $$;
	  while (nt->next != NULL) {
	    nt = nt->next;
	  }
	  LinkNoteTmp(nt,$3);
	}
    ;

noteslist:
    fvarlist BRACEDTEXT_TOK
	{
	  $$ = CreateNoteTmp(NULL, AddBraceChar($2,NULL),
                             (void *)$1, LineNum());
	}
    | noteslist fvarlist BRACEDTEXT_TOK
	{
	  $$ = CreateNoteTmp(NULL, AddBraceChar($3,NULL),
	                     (void *)$2, LineNum());
	  LinkNoteTmp($$,$1);
	}
    ;

fvarlist:
    varlist
	{
	  /*
	   * Reversing the variable list is now essential to deal with
	   * external procedures and other things where order is important.
	   */
	  $$ = ReverseVariableList($1);
	}
    ;

varlist:
    fname
	{
	  $$ = CreateVariableNode($1);
	}
    | varlist ',' fname
	{
	  $$ = CreateVariableNode($3);
	  LinkVariableNodes($$,$1);
	}
    | varlist fname
	{
	  ErrMsg_CommaName("name",$2);
	  $$ = CreateVariableNode($2);
	  LinkVariableNodes($$,$1);
	  /* trash the definition. keep the loose fname around because
	   * destroying here is inconvenient
	   */
	  g_untrapped_error++;
	}
    ;

fname:
    name optional_notes
	{
	  symchar *simple;
	  void *data;
	  enum NoteData nd;
	  $$ = ReverseName($1);
	  if ($2 != NULL && $1 != NULL) {
            simple = SimpleNameIdPtr($$);
	    data = (simple == NULL ? (void *)$$ : NULL);
	    nd = (data == NULL ? nd_empty : nd_name);
	    CollectNote(CreateNote(g_type_name, InlineNote(), simple,
	                           g_proc_name,
	                           Asc_ModuleBestName(Asc_CurrentModule()),
	                           AddBraceChar($2,InlineNote()),
	                           LineNum(), data, nd));
	  }
	}
    ;

name:
    IDENTIFIER_TOK
	{
	  $$ = CreateIdName($1);
	}
    | name '.' IDENTIFIER_TOK
	{
	  $$ = CreateIdName($3);
	  LinkNames($$,$1);
	}
    | name '[' set ']'
	{
	  if ($3 == NULL) {
	    error_reporter_current_line(ASC_USER_ERROR,"syntax error: Empty set in name definition, name:");
	    WriteName(ASCERR,$1);
	    FPRINTF(ASCERR,"[]\n");
	    g_untrapped_error++;
	  } else {
	    $$ = CreateSetName($3);
	    LinkNames($$,$1);
	  }
	}
    ;

end:
    END_TOK CONDITIONAL_TOK
	{
          g_end_identifier = NULL;
          $$ = CONDITIONAL_TOK;
        }
    | END_TOK FOR_TOK
	{
          g_end_identifier = NULL;
          $$ = FOR_TOK;
        }
    | END_TOK IF_TOK
	{
          g_end_identifier = NULL;
          $$ = IF_TOK;
        }
    | END_TOK INTERACTIVE_TOK
	{
          g_end_identifier = NULL;
          $$ = INTERACTIVE_TOK;
        }
    | END_TOK METHODS_TOK
	{
          g_end_identifier = NULL;
          $$ = METHODS_TOK;
        }
    | END_TOK NOTES_TOK
	{
          g_end_identifier = NULL;
          $$ = NOTES_TOK;
        }
    | END_TOK SELECT_TOK
	{
          g_end_identifier = NULL;
          $$ = SELECT_TOK;
        }
    | END_TOK SWITCH_TOK
	{
          g_end_identifier = NULL;
          $$ = SWITCH_TOK;
        }
    | END_TOK UNITS_TOK
	{
          g_end_identifier = NULL;
          $$ = UNITS_TOK;
        }
    | END_TOK GLOBAL_TOK
	{
          g_end_identifier = NULL;
          $$ = GLOBAL_TOK;
        }
    | END_TOK WHEN_TOK
	{
          g_end_identifier = NULL;
          $$ = WHEN_TOK;
        }
    | END_TOK WHILE_TOK
	{
          g_end_identifier = NULL;
          $$ = WHILE_TOK;
        }
    | END_TOK IDENTIFIER_TOK
	{
          g_end_identifier = $2;
          $$ = IDENTIFIER_TOK;
        }
    | END_TOK /* empty */
	{
          g_end_identifier = NULL;
          $$ = END_TOK;
        }
    ;

optional_bracedtext:
    /* empty */
	{
	  $$ = NULL;
	}
    | BRACEDTEXT_TOK
	{
	  $$ = $1;
	}
    ;

optional_notes:
    /* empty */
	{
	  $$ = NULL;
	}
    | DQUOTE_TOK
	{
	  $$ = $1;
	}
    ;

set:
    setexprlist
	{
	  $$ = ReverseSetList($1);
	}
    | /* empty */
	{
	  $$ = NULL;
	}
    ;

setexprlist:
    expr
	{
	  $$ = CreateSingleSet($1);
	}
    | expr DOTDOT_TOK expr
	{
	  $$ = CreateRangeSet($1,$3);
	}
    | setexprlist ',' expr
	{
	  $$ = CreateSingleSet($3);
	  LinkSets($$,$1);
	}
    | setexprlist ',' expr DOTDOT_TOK expr
	{
	  $$ = CreateRangeSet($3,$5);
	  LinkSets($$,$1);
	}
    ;

number:
    INTEGER_TOK
	{
	  $$ = $1;
	  g_constant_type = LONGCONSTANT;
	  g_default_dim_ptr = Dimensionless();
	}
    | realnumber
	{
	  $$ = $1;
	  g_constant_type = DOUBLECONSTANT;
	  g_default_dim_ptr = g_dim_ptr;
	}
    ;

realnumber:
    REAL_TOK opunits
	{
	  $$ = $1*$2;
	}
    | INTEGER_TOK BRACEDTEXT_TOK
	{
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits($2,&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    $$ = (double)$1*UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
            char **errv;
	    $$ = (double)$1;
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'", $2);
            errv = UnitsExplainError($2,error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
    ;

opunits:
    /* empty */
	{
	  g_dim_ptr = Dimensionless();
	  $$ = 1.0;
	}
    | BRACEDTEXT_TOK
	{
	  unsigned long pos;
	  int error_code;
	  g_units_ptr = FindOrDefineUnits($1,&pos,&error_code);
	  if (g_units_ptr != NULL) {
	    $$ = UnitsConvFactor(g_units_ptr);
	    g_dim_ptr = UnitsDimensions(g_units_ptr);
	  } else {
            char **errv;
	    $$ = 1.0;
	    g_dim_ptr = WildDimension();
	    error_reporter_current_line(ASC_USER_ERROR,"Undefined units '%s'",$1);
            errv = UnitsExplainError($1,error_code,pos);
	    error_reporter_current_line(ASC_USER_ERROR,"  %s\n  %s\n  %s\n",errv[0],errv[1],errv[2]);
	    g_untrapped_error++;
	  }
	}
    ;

dims:
    DIMENSION_TOK dimensions
	{
	  $$ = $2;
	}
    | DIMENSIONLESS_TOK
	{
	  $$ = Dimensionless();
	}
    | /* empty */
	{
	  $$ = WildDimension();
	}
    ;

dimensions:
    '*'
	{
	  $$ = WildDimension();
	}
    | dimexpr
	{
	  $$ = FindOrAddDimen(&($1));
	}
    ;

dimexpr:
    IDENTIFIER_TOK
	{
	  ParseDim(&($$),SCP($1));
	}
    | INTEGER_TOK
	{
	  ClearDimensions(&($$));
	}
    | dimexpr '/' dimexpr
	{
	  $$ = SubDimensions(&($1),&($3));
	}
    | dimexpr '*' dimexpr
	{
	  $$ = AddDimensions(&($1),&($3));
	}
    | dimexpr '^' fraction
	{
	  $$ = ScaleDimensions(&($1),$3);
	}
    | '(' dimexpr ')'
	{
	  CopyDimensions(&($2),&($$));
	}
    ;

fraction:
    optional_sign fractail
	{
	  $$ = $1 ? NegateF($2) : $2;
	}
    ;

fractail:
    INTEGER_TOK
	{
	  $$ = CreateFraction((short)$1,(short)1);
	}
    | '(' INTEGER_TOK '/' INTEGER_TOK ')'
	{
	  $$ = CreateFraction((short)$2,(short)$4);
	}
    ;

optional_sign:
    /* empty */
	{
	  $$ = 0;
	}
    | '+'
	{
	  $$ = 0;
	}
    | '-'
	{
	  $$ = 1;
	}
    ;

expr:
    INTEGER_TOK
	{
	  $$ = CreateIntExpr($1);
	}
    | MAXINTEGER_TOK
	{
	  $$ = CreateIntExpr(LONG_MAX-1);
	}
    | realnumber
	{
	  $$ = CreateRealExpr($1,g_dim_ptr);
	}
    | MAXREAL_TOK
	{
	  $$ = CreateRealExpr(DBL_MAX/(1+1e-15),Dimensionless());
	}
    | TRUE_TOK
	{
	  $$ = CreateTrueExpr();
	}
    | FALSE_TOK
	{
	  $$ = CreateFalseExpr();
	}
    | ANY_TOK
	{
	  $$ = CreateAnyExpr();
	}
    | SYMBOL_TOK
	{
	  $$ = CreateSymbolExpr($1);
	}
    | fname
	{
	  $$ = CreateVarExpr($1);
	}
    | '[' set ']'
	{
	  $$ = CreateSetExpr($2);
	}
    | expr '+' expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_plus));
	  $$ = JoinExprLists($1,$3);
	}
    | expr '-' expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_minus));
	  $$ = JoinExprLists($1,$3);
	}
    | expr '*' expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_times));
	  $$ = JoinExprLists($1,$3);
	}
    | expr '/' expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_divide));
	  $$ = JoinExprLists($1,$3);
	}
    | expr '^' expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_power));
	  $$ = JoinExprLists($1,$3);
	}
    | expr AND_TOK expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_and));
	  $$ = JoinExprLists($1,$3);
	}
    | expr OR_TOK expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_or));
	  $$ = JoinExprLists($1,$3);
	}
    | NOT_TOK expr
	{
	  $$ = JoinExprLists($2,CreateOpExpr(e_not));
	}
    | expr relop expr %prec NEQ_TOK
	{
	  $3 = JoinExprLists($3,$2);
	  $$ = JoinExprLists($1,$3);
	}
    | expr logrelop expr %prec BEQ_TOK
	{
	  $3 = JoinExprLists($3,$2);
	  $$ = JoinExprLists($1,$3);
	}
    | expr IN_TOK expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_in));
	  $$ = JoinExprLists($1,$3);
	}
    | expr '|' expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_st));
	  $$ = JoinExprLists($1,$3);
	}
    | expr SUCHTHAT_TOK expr
	{
	  $3 = JoinExprLists($3,CreateOpExpr(e_st));
	  $$ = JoinExprLists($1,$3);
	}
    | '+' expr %prec UPLUS_TOK
	{
	  $$ = $2;
	}
    | '-' expr %prec UMINUS_TOK
	{
	  $$ = JoinExprLists($2,CreateOpExpr(e_uminus));
	}
    | SATISFIED_TOK '(' fname ',' realnumber ')'
	{
	  $$ = CreateSatisfiedExpr($3,$5,g_dim_ptr);
	}
    | SATISFIED_TOK '(' fname ')'
	{
	  $$ = CreateSatisfiedExpr($3,DBL_MAX,NULL);
	}
    | SUM_TOK '(' set ')'
	{
	  DestroySetList($3);
	  $$ = NULL;
	  ErrMsg_ParensBrackets("SUM");
	  g_untrapped_error++;
	}
    | SUM_TOK '[' set ']'
	{
	  $$ = CreateBuiltin(e_sum,$3);
	}
    | PROD_TOK '(' set ')'
	{
	  DestroySetList($3);
	  $$ = NULL;
	  ErrMsg_ParensBrackets("PROD");
	  g_untrapped_error++;
	}
    | PROD_TOK '[' set ']'
	{
	  $$ = CreateBuiltin(e_prod,$3);
	}
    | UNION_TOK '(' set ')'
	{
	  DestroySetList($3);
	  $$ = NULL;
	  ErrMsg_ParensBrackets("UNION");
	  g_untrapped_error++;
	}
    | UNION_TOK '[' set ']'
	{
	  $$ = CreateBuiltin(e_union,$3);
	}
    | INTERSECTION_TOK '(' set ')'
	{
	  DestroySetList($3);
	  $$ = NULL;
	  ErrMsg_ParensBrackets("INTERSECTION");
	  g_untrapped_error++;
	}
    | INTERSECTION_TOK '[' set ']'
	{
	  $$ = CreateBuiltin(e_inter,$3);
	}
    | CARD_TOK '(' set ')'
	{
	  DestroySetList($3);
	  $$ = NULL;
	  ErrMsg_ParensBrackets("CARD");
	  g_untrapped_error++;
	}
    | CARD_TOK '[' set ']'
	{
	  $$ = CreateBuiltin(e_card,$3);
	}
    | CHOICE_TOK '(' set ')'
	{
	  DestroySetList($3);
	  $$ = NULL;
	  ErrMsg_ParensBrackets("CHOICE");
	  g_untrapped_error++;
	}
    | CHOICE_TOK '[' set ']'
	{
	  $$ = CreateBuiltin(e_choice,$3);
	}
    | IDENTIFIER_TOK '(' expr ')'
	{
	  CONST struct Func *fptr;
	  if ((fptr = LookupFunc(SCP($1)))!=NULL) {
	    $$ = JoinExprLists($3,CreateFuncExpr(fptr));
	  } else {
	    $$ = NULL;
	    error_reporter_current_line(ASC_USER_ERROR,"Function '%s' is not defined.",SCP($1));
	    g_untrapped_error++;
	  }
	}
    | '(' expr ')'
	{
	  $$ = $2;
	}
    ;

relop:
    '='
	{
	  $$ = CreateOpExpr(e_equal);
	}
    | '<'
	{
	  $$ = CreateOpExpr(e_less);
	}
    | '>'
	{
	  $$ = CreateOpExpr(e_greater);
	}
    | LEQ_TOK /* less than or equal written "<=" */
	{
	  $$ = CreateOpExpr(e_lesseq);
	}
    | GEQ_TOK /* greater than or equal written ">=" */
	{
	  $$ = CreateOpExpr(e_greatereq);
	}
    | NEQ_TOK /* not equal written "<>" */
	{
	  $$ = CreateOpExpr(e_notequal);
	}
    ;

logrelop:
    BEQ_TOK /* equality in boolean relations */
	{
	  $$ = CreateOpExpr(e_boolean_eq);
	}
    | BNE_TOK /* non equality in boolean relations */
	{
	  $$ = CreateOpExpr(e_boolean_neq);
	}
    ;
%%
/*
 * We really need to do something about freeing up the productions
 * that invoke this so we don't leak memory like a seive.
 * for example  z[i IN [1..2]][j IN [process[i]] IS_A mass; eats a ton.
 */
int
yyerror(char *s)
{
  g_untrapped_error++;
  if (Asc_CurrentModule() != NULL) {
    error_reporter_current_line(ASC_USER_ERROR,"%s",s);
  } else {
    error_reporter(ASC_USER_ERROR,NULL,0,"%s at end of input.\n",s);
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
	static errcount=0;
	if(errcount<30){ 
		/* the module may have be already closed, Asc_CurrentModule will be null */
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
  struct module_t *mod;

  /* the module may have be already closed */
  mod = Asc_CurrentModule();

  error_reporter_current_line(ASC_USER_ERROR, "ASC-Error: Missing comma or operator before %s ",what);
  WriteName(ASCERR,name);
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
ErrMsg_NullDefPointer(CONST char *object)
{
  error_reporter_current_line(ASC_USER_ERROR,"Rejected '%s'", object);
}

static void
ErrMsg_ProcTypeMissing(CONST char *AorR, CONST char *type)
{
  error_reporter_current_line(ASC_USER_ERROR,
	  "%s METHODS called with undefined type (%s)", AorR, type);
}

static void
ErrMsg_ProcsRejected(CONST char *AorR, CONST char *type)
{
  error_reporter_current_line(ASC_USER_ERROR,
	  "%s METHODS failed for type %s", AorR, type);
}

static void
ErrMsg_DuplicateProc(struct InitProcedure *p)
{
  error_reporter_current_line(ASC_USER_WARNING,
	  "Duplicate METHOD %s rejected", SCP(ProcName(p)));
}

static void
ErrMsg_ParensBrackets(CONST char *operation)
{
  error_reporter_current_line(ASC_USER_ERROR,
          "  You should be using %s[] not %s()",
          operation,
          operation);
}


/*
 *  WarnMsg_MismatchEnd(statement, opt_name, end_token, expecting);
 *      const char *statement;
 *      const char *opt_name;
 *      unsigned long end_token;
 *      const char *expecting;
 *
 *  Print a warning message that the token after the END keyword did not
 *  match what we were expecting for the current statement.
 *  Arguments:
 *      statement --the current statement, e.g. ATOM, METHOD, FOR, IF, CASE
 *      opt_name  --the name of the thing we were defining for ATOMs, METHODs,
 *                  etc, or NULL anonymous statements (FOR, IF, CASE, etc)
 *      end_token --the TOKEN_TOK that we were received instead.  We use the
 *                  TokenAsString to produce a string given a TOKEN_TOK
 *      expecting --the keyword we were expecting to see after the END; if
 *                  NULL, we were expecting the string given in statement
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
 *  CONST char *TokenAsString(token);
 *      unsigned long token;
 *
 *  Takes a TOKEN_TOK (e.g., FOR_TOK, MODEL_TOK, END_TOK, IDENTIFIER_TOK) and returns
 *  a string representation of it:
 *      e.g.:  TokenAsString(FOR_TOK) ==> "FOR"
 *
 *  Since this function is only used inside WarnMsg_MismatchEnd, we do a
 *  couple of things specific to that function:  If token is END_TOK, we
 *  return an empty string, and if it is IDENTIFIER_TOK, we return the
 *  current value of g_end_identifier, or UNKNOWN if g_end_identifier is
 *  NULL.
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
  case PATCH_TOK:
    return "PATCH";
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
	va_list args;
	va_start(args,fmt);
	va_error_reporter(sev,Asc_ModuleBestName(Asc_CurrentModule()),(int)LineNum(),NULL,fmt,args);
	va_end(args);
}
