/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994, 1995 Thomas Guthrie Epperly

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
	@file
	Type definition module
*//*
	by Tom Epperly
	Created: 1/12/90
	Last in CVS: $Revision: 1.60 $ $Date: 1998/04/21 23:50:02 $ $Author: ballan $
*/

#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/dstring.h>



#include "functype.h"
#include "expr_types.h"
#include "stattypes.h"
#include "statement.h"
#include "slist.h"
#include "statio.h"
#include "symtab.h"
#include "module.h"
#include "library.h"
#include "child.h"
#include "vlist.h"
#include "name.h"
#include "nameio.h"
#include "when.h"
#include "select.h"
#include "sets.h"
#include "setio.h"
#include "exprs.h"
#include "exprio.h"
#include "forvars.h"
#include "bit.h"
#include "setinstval.h"
#include "childinfo.h"
#include "instance_enum.h"
#include "type_desc.h"
#include "type_descio.h"
#include "atomsize.h"
#include "value_type.h"
#include "evaluate.h"
#include "proc.h"
#include "typelint.h"
#include "childdef.h"
#include "cmpfunc.h"
#include "typedef.h"
#include <general/mathmacros.h>

#ifndef lint
static CONST char TypeDefinitionRCSid[] ="$Id: typedef.c,v 1.60 1998/04/21 23:50:02 ballan Exp $";
#endif


/*
 *  To generate a name for a relation, logrelation or when using
 *  the number of the relation, logrelation or when in the model,
 *  rather than the line number
 */

/*
 *  number of a relation,logrelation or when
 */
static unsigned long g_number= 0;

/*
 *  unused at present
 *static unsigned long g_typedef_linenum = 0;
 */


/*
 *  function to find if name is proper FOR variable. Returns 1 if so. 0 not.
 *  should be in another file.
 */
static
int NameInForTable(CONST struct for_table_t *ft, CONST struct Name *n)
{
  symchar *name;
  struct for_var_t *ptr;
  if (ft != NULL && n != NULL) {
    AssertMemory(ft);
    name = SimpleNameIdPtr(n);
    if (name != NULL) {
      ptr = FindForVar(ft,name);
      if (ptr != NULL) {
        switch(GetForKind(ptr)){
        case f_integer:
        case f_symbol:
        case f_set:
        case f_untyped: /* we aren't interpretting, just name spacing */
          return 1;
        default:
          FPRINTF(ASCERR,"Untyped FOR variable (%s).\n",SCP(name));
        }
      }
    }
  }
  return 0;
}

/*----------------------------------------------------------------------------
 *  Data structures to help track reducing a prior argument list by the
 *  reduction assignments.
 *----------------------------------------------------------------------------
 */

/* Redlist auxillaries. */
/* a struct for Reduce use */
struct RedListEntry {
  struct Statement *olddeclstat;
  /* these are temporary references, so the statement copy fcn not used */
  CONST struct Name *name;
  /* assumes each IS_A has one lhs name. if NULL, olddeclstat is a WILL_BE. */
  CONST struct Name *wbname;
  /* one WILL_BE may have several RLEs. This entry is not NULL if
   * rle is from a WILL_BE.
   */
  int assigned; /* relevant only for name != NULL */
  /*
   * -2 name is of WILL_BE MODEL/variable, array or not;
   * -1 name is of constant array; multiple assignments allowed.
   * 0 = name is constant/set not yet assigned;
   * 1 name has assigned value, possibly from constant typedesc;
   */
};

/* forward declarations */
static
enum typelinterr
DoIS_A(CONST struct StatementList *stats);

static
enum typelinterr
DoWhens(symchar *, CONST struct StatementList *, struct gl_list_t *);

static
enum typelinterr
DoRelations(symchar *, CONST struct StatementList *, struct gl_list_t *);

/* stuff used to build child lists from statements, etc */

/**********************************************************\
 * During the production of a child list we desire to ultimately
 * produce a sorted gllist of pointers to struct ChildListEntry *
 * that contain the names, array status, and most refined
 * basetypes determinable based on already existing type
 * definitions and the new statements of the type for which
 * we are making the child list.
 * We desire to be able to do this for some
 * number of children between 0 and 100000
 *
 * The proposed and hence implemented solution is in the
 * section of code that follows: a doubly linked list containing
 * struct ChildListEntry and the operators to manage it.
 * Once these size and naming of all the children are determined
 * and sorted into this DL structure, we can map them into
 * a gl_list for further processing.
 * The child.h list interface specifies that we the user are
 * responsible for the ChildListEntries in the input gllist,
 * so we here use a lifo recycle list to avoid constant calls
 * to malloc/free.
\**********************************************************/
struct LinkChildListEntry {
  struct ChildListEntry e; /* must be first! */
  /* the next 3 are not yet in use, really. */
  /* struct LinkChildListEntry *alike; -- removed, JP */
  /* pointer to aliked child list entries, which will all have the same type.
   * This will be NULL unless an ARE_ALIKE has been seen or the CLE is a
   * scalar. Array names in particular unless a statement aliking the
   * array elements over its set of definition will have null alike ptr.
   * ARE_ALIKE of individual array elements will not show up here.
   * Circularly linked list.
   */
  /* struct LinkChildListEntry *arrthesame; --removed, JP */
  /* struct LinkChildListEntry *eltsthesame; -- removed, JP */
  /* pointer to merged child list entries, which will all have the same type.
   * This will be NULL unless an ARE_THE_SAME has been seen or the CLE is a
   * scalar. Array names in particular unless a statement merging the
   * array elements over its set of definition will have NULL eltsthesame ptr.
   * ARE_THE_SAME of individual array elements will not show up here.
   * Merging arrays, as is the apparent case with an alias of an array,
   * will show up in the arrthesame ptr.
   * Circularly linked list.
   */
  /* pointers of the doubly-linked LCL structure */
  struct LinkChildListEntry *prev;
  struct LinkChildListEntry *next;
};

static
struct LinkChildListEntry *g_lcl_head = NULL, *g_lcl_tail = NULL;
static
struct LinkChildListEntry *g_lcl_recycle = NULL;
/* above the head and tail anchors of the list and the anchor
 * for a lifo recycle list of these structures.
 */

static
struct LinkChildListEntry *g_lcl_pivot = NULL;
/* a pointer to somewhere in the working list. used heuristically
 * to speed name-based search.
 */

#define LCLNAME g_lcl_pivot->e.strptr
/* returns the name of the current pivot. assumes the pivot is valid */

static
unsigned long g_lcl_length = 0;

#ifndef NDEBUG
static
unsigned long g_lclrecycle_length = 0;
#endif

void DestroyTypedefRecycle(void)
{
  struct LinkChildListEntry *old;
  while (g_lcl_recycle != NULL) {
    old = g_lcl_recycle;
    g_lcl_recycle = old->next;
    ascfree(old);
  }
}

/*
 * returns a recycled or a new lcl element
 * whichever is first available. Does nothing else
 * except possibly update the length of the recycle list
 * during debugging.
 */
static
struct LinkChildListEntry *GetLCL(void)
{
  struct LinkChildListEntry *new;
  if (g_lcl_recycle!=NULL) {
    new = g_lcl_recycle;
    g_lcl_recycle = new->next;
#ifndef NDEBUG
    g_lclrecycle_length--;
#endif
  } else {
    new = ASC_NEW(struct LinkChildListEntry);
  }
  return new;
}

static
void ClearLCL(void)
{
#ifndef NDEBUG
  struct LinkChildListEntry *old;
  /* do some book keeping and reinitializing, working from the tail. */
  while (g_lcl_tail!=NULL) {
    assert(g_lcl_length!=0L);
    /* init */
    old = g_lcl_tail;
    old->e.strptr = NULL;
    old->e.typeptr = NULL;
    old->e.isarray = 0;
    old->e.origin = origin_ERR;
    /* cut off tail */
    g_lcl_tail = old->prev;
    g_lcl_length--;
    /* push old into singly linked recycle list */
    old->prev = NULL;
    old->next = g_lcl_recycle;
    g_lcl_recycle = old;
    g_lclrecycle_length++;
  }
  assert(g_lcl_length==0L);
#else
  /* insert current list at head of recycle */
  if (g_lcl_tail!=NULL) {
    g_lcl_tail->next = g_lcl_recycle;
  }
  /* if anything was added, get new head */
  if (g_lcl_head != NULL) {
    g_lcl_recycle = g_lcl_head;
  }
#endif
  g_lcl_tail = g_lcl_head = g_lcl_pivot = NULL;
  g_lcl_length=0;
}

/*
 * copies the pointers from the LCL to a gllistt.
 * the lcl still exists. We do not clear it until
 * after the gllist containing the copies is finished with.
 * This should never return null.
 */
static
struct gl_list_t *CopyLCLToGL(void)
{
  struct gl_list_t *list;
  struct LinkChildListEntry *e;
  list = gl_create(g_lcl_length);
  assert(list!=NULL);
  e = g_lcl_head;
  while (e!=NULL) {
    /* since lcl is sorted, insert should always be at tail.
     * since we created it big enough, this should never have to expand.
     */
    gl_insert_sorted(list,e,(CmpFunc)CmpChildListEntries);
    e = e->next;
  }
  assert(gl_length(list)==g_lcl_length);
  return list;
}

/*
 * On entry assumes g_lcl_pivot==NULL means the LCL is empty.
 * s must not be NULL.
 *
 * Moves g_lcl_pivot to the element e such that e->next should be
 * changed to point to the new element according to s.
 * If an element with the name s already is found,
 * returns 0 instead of 1 and pivot will be pointing to the
 * entry with that symchar.
 * On exit g_lcl_pivot might be null if head and tail are
 * or if the s given is ahead in alphabetical order of all the
 * list elements.c
 *
 */
static
int LocateLCLPivot(symchar *s)
{
  int cmp;
  assert(s!=NULL);
  if (g_lcl_pivot == NULL) {
    /* list is empty. null is the pivot. */
    return 1;
  }
  if (s == g_lcl_pivot->e.strptr) return 0;
  /* repeated name check by ptr */
  cmp = CmpSymchar(LCLNAME,s);
  if (cmp==0) return 0;
  if (cmp<0) {
    /* search forward */
    while (g_lcl_pivot->next != NULL) {
      g_lcl_pivot = g_lcl_pivot->next;
      cmp = CmpSymchar(LCLNAME,s);
      if (cmp >= 0) {
        if (cmp==0) return 0;
        g_lcl_pivot = g_lcl_pivot->prev;
        return 1;
      }
    }
    assert(g_lcl_pivot==g_lcl_tail);
    return 1;
  } else {
    /* search backward */
    while (g_lcl_pivot->prev != NULL) {
      g_lcl_pivot = g_lcl_pivot->prev;
      cmp = CmpSymchar(LCLNAME,s);
      if (cmp <= 0) {
        if (cmp==0) return 0;
        return 1;
      }
    }
    assert(g_lcl_pivot==g_lcl_head);
    g_lcl_pivot = NULL;
    return 1;
  }
}

#define STATBODY 0
#define STATPARAMETRIC 1

/** Store a new 'child record' in the linked list of statements.

	@param s name of the child (symchar that's already AddSymbol-ed)
	@param d type pointer for the childname, which may be null,
	@param nsubs number of known subscripts (if an array),
		0 if not an array,
		<0 if it's an alias (subscripts to be determined at the end).
		See the 'note' below.
	@param parametric 1 if stat is a parameter decl, 0 if body stat (...?)

	@return 1 on success, 0 on failure by duplicate name, -1 on MALLOC or bad input.

	This function inserts a named named statement/type into the named-ordered
	list of statements.

	On entry assumes g_lcl_pivot != NULL unless there are no children yet.

	For aliases, the correct value of nsubs must be set before
	calling MakeChildList.

	@note For 'nsubs', a value of '-1' should be given for <tt>b ALIASES a;</tt>
		and a value of '-2' should be given for <tt>b[1..n] ALIASES a;</tt> since
		we don't know for certain the subscriptedness of 'a' untill children
		are all parsed.
*/
static
int AddLCL(symchar *s,CONST struct TypeDescription *d, int nsubs,
           CONST struct Statement *stat, int parametric)
{
  struct LinkChildListEntry *new;

  /* search for insertion location, which means move pivot to the
   * name just before this one so we can insert after pivot.
   */
  if (!LocateLCLPivot(s)) {
    return 0; /* if found the exact name, exit early */
  }

  /* get a LCLEntry to fill with data */
  new = GetLCL();
  if (new==NULL) return -1;

  new->e.strptr = s;
  new->e.typeptr = d;
  new->e.statement = stat;
  new->e.isarray = nsubs;
  new->e.bflags = CBF_VISIBLE;
  switch (StatementType(stat)) {
  case ALIASES:
    new->e.origin = origin_ALI;
    break;
  case ARR:
    new->e.origin = origin_ARR;
    break;
  case ISA:
    new->e.origin = origin_ISA;
    break;
  case WILLBE:
    new->e.origin = origin_WB;
    break;
  case REL:
  case LOGREL:
  case WHEN:
  case EXT:
    /* not strictly kosher TRUE, but truer than saying an error */
    /* all rel/logrel/when are implicitly IS_A'd/typed */
    new->e.origin = origin_ISA;
    break;
  default:
    new->e.origin = origin_ERR;
    break;
  }
  if (new->e.origin != origin_ERR && parametric==STATPARAMETRIC) {
    new->e.origin += origin_PARAMETER_OFFSET;
  }
  g_lcl_length++;

  /* insert after pivot and make new element new pivot */
  new->prev = g_lcl_pivot;  /* might be null */
  if (g_lcl_pivot!=NULL) {
    /* the list has a head we can add in after. we need to point back at
     * it, it at us, and possibly we are the tail or the tail points back.
     */
    /* point new element at tail of list */
    new->next = g_lcl_pivot->next; /* might be null */
    /* update tail back pointer */
    if (new->next == NULL) {
      /* added to end of list */
      g_lcl_tail = new;
    } else {
      /* added before some tail */
      new->next->prev = new;
    }
    /* point it at us */
    g_lcl_pivot->next = new;
  } else {
    /* first element being inserted, or new element leads list. */
    new->next = g_lcl_head;
    g_lcl_head = new;
    if (g_lcl_tail == NULL) { /* new is the only element in list */
      g_lcl_tail = new;
    } else {
      new->next->prev = new;
    }
  }
  g_lcl_pivot = new; /* cannot be NULL */
  assert(new->e.origin != origin_ERR);
  return 1;
}

/*
 * Searches the LCL for an entry with a symchar with same
 * value as string given. string given is expected to
 * be from the symbol table.
 * Returns the pointer to the LCLentry if exact match found
 * else returns NULL.
 * Side effects: relocates lcl_pivot to entry matching s, if
 *   such an entry exists, else no side effects.
 * If symchar gets redefined, this will most probably need
 * reimplementing.
 */
static
struct LinkChildListEntry *FindLCL(symchar *s)
{
  struct LinkChildListEntry *hold;
  hold = g_lcl_pivot;
  if (LocateLCLPivot(s)) {
    /* locate found an insertion point, so name wasn't in list. */
    g_lcl_pivot = hold;
    return NULL;
  } else {
    /* assumption: we will always call FindLCL correctly, so that
     * a zero return does not mean an error.
     */
    return g_lcl_pivot;
  }
}


/**********************************************************\
end linkchildlistentry memory manipulation functions.
could we put all the above in the childdef file?
\**********************************************************/

#define DoName(n,c,s) DoNameF((n),(c),(s),1)
#define DoNameQuietly(n,c) DoNameF((n),(c),(s),0)

/**
	Checks the first element of a name for being in the child list.

	@return DEF_OKAY if the child was added to the list
		DEF_NAME_DUPLICATE if the child was already in the list (can't be added)

	@param nptr name being added. We are only looking at the first 'link' in the name, eg 'a' in 'a[5]'.
	@param noisy output stuff when child can't be added

	Name must be an id (ie a variable name like 'my_var')

	Also checks if name is name of an array.
	This function should NOT be use on parameter declarations.
*/
static enum typelinterr
DoNameF(CONST struct Name *nptr,
        CONST struct TypeDescription *type,
        CONST struct Statement *stat,
        int noisy)
{
  register symchar *name;
  int ok;
  int nsubs=0;

  /*char *nstr;
  nstr = WriteNameString(nptr);
  CONSOLE_DEBUG(nstr);
  ascfree(nstr);*/

  if (NameId(nptr) !=0){
    name = NameIdPtr(nptr);
    switch (StatementType(stat)) {
    case EXT:
	  /* CONSOLE_DEBUG("PROCESSING EXTERNAL RELATION"); */
      nsubs = NameLength(nptr) - 1;
	  /* CONSOLE_DEBUG("NSUBS = %d",nsubs); */
	  break;
    case ISA:
    case REF: /* IS_A of prototype */
    case WILLBE:
    case REL:
    case LOGREL:
    case WHEN:
      nsubs = NameLength(nptr) - 1;
      break;
    case ALIASES:
      nsubs -= NameLength(nptr); /* because init to 0 */
      break;
    case ARR:
      /* god this case is ugly */
      if (nptr==NamePointer(ArrayStatAvlNames(stat))) {
        /* first field is an alias array */
        nsubs -= NameLength(nptr); /* because init to 0 */
        type = NULL;
      } else {
        /* second field is an IS_A of a set */
        type = FindSetType();
        nsubs = NameLength(nptr) - 1;
      }
      break;
    default:
      /* should never happen */
      return DEF_STAT_MISLOCATED;
    }
    ok = AddLCL( name,type, nsubs,
                 stat, /* statement of initial IS_A/ALIASES,relation */
                 STATBODY
               );
    if (ok < 1) {
      if (ok < 0) {
        ERROR_REPORTER_NOLINE(ASC_PROG_FATAL,"Insufficient memory during parse.");
        return DEF_ILLEGAL; /* well, having insufficient memory is illegal */
      }
      if (noisy && ok == 0) {
        ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Same instance name \"%s\" used twice.",SCP(name));
        assert(g_lcl_pivot!=NULL);
        if (g_lcl_pivot->e.statement != stat ) {
          STATEMENT_ERROR(g_lcl_pivot->e.statement,"  First seen:");
        } else {
          FPRINTF(ASCERR,"\n");
        }
      }
      return DEF_NAME_DUPLICATE;
    }
  } else {
    /* should never happen due to new upstream filters. */
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Bad name structure found in variable list.");
    return DEF_NAME_INCORRECT;
  }
  return DEF_OKAY;
}

static
enum typelinterr DoVarList(CONST struct VariableList *vlist,
                           CONST struct TypeDescription *type,
                           CONST struct Statement *stat)
{
  register CONST struct Name *nptr;
  enum typelinterr error_code;
  while(vlist!=NULL){
    nptr = NamePointer(vlist);
    error_code = DoName(nptr,type,stat);
    if (error_code != DEF_OKAY) return error_code;
    vlist = NextVariableNode(vlist);
  }
  return DEF_OKAY;
}

/*
 *  This function is supposed to handle the IS_A's inside a
 *  SELECT statement. However, now all of the statements inside
 *  SELECT are contained in the main statement list, which is
 *  flat. So, it is not required anymore; thus, the #if
 */
#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
enum typelinterr DoSelectList(struct SelectList *cases)
{
  enum typelinterr error_code;
  while(cases != NULL){
    error_code = DoIS_A(SelectStatementList(cases));
    if (error_code != DEF_OKAY) {
      return error_code;
    }
    cases = NextSelectCase(cases);
  }
  return DEF_OKAY;
}
#endif /*  THIS_IS_AN_UNUSED_FUNCTION  */


/*
 * Calls functions to check child name list against rhs of ALIASES statements.
 * Recurses in for loops. The check is only partial on qualified names, but
 * that catches some typos.
 * this function should be merged with Lint Once Lint is working.
 * It can only work after the complete child list is constructed.
 * It returns DEF_OKAY unless a rhs is missing or a relation alias
 * is attempted.
 * Basically, prevents aliasing of relations.
 */
static
enum typelinterr VerifyALIASES(CONST struct StatementList *stats,
                           struct gl_list_t *childlist)
{
  register struct gl_list_t *statements;
  register unsigned long c,len,pos;
  enum typelinterr error_code;
  struct Statement *stat;
  struct ChildListEntry test;
  struct ChildListEntry *clep;
  statements = GetList(stats);
  len = gl_length(statements);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case ALIASES:
      /* for aliases, checking the rhs can only be done partially,
       * and only after the whole child list is available.
       */
      test.strptr = NameIdPtr(AliasStatName(stat));
      pos = gl_search(childlist,&test,(CmpFunc)CmpChildListEntries);
      /* the preceding gl_search relies on the fact that the comparison
       * element in the ChildListEntry is the symchar.
       * It will break if things are different.
       */
      if (pos != 0) {
        clep = (struct ChildListEntry *)gl_fetch(childlist,pos);
        /* check relation aliases */
        if (clep->typeptr != NULL) {
          if (GetBaseType(clep->typeptr) == relation_type ||
              GetBaseType(clep->typeptr) == logrel_type ) {
            /* mark stat wrong */
            MarkStatContext(stat,context_WRONG);
            error_code = DEF_ILLEGAL_RHS;
            TypeLintError(ASCERR,stat, error_code);
            WSS(ASCERR,stat);
            return error_code;
          }
        }
        error_code = DEF_OKAY;
      } else {
        error_code = DEF_NAME_MISSING;
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case FOR:
      error_code = VerifyALIASES(ForStatStmts(stat),childlist);
      if (error_code != DEF_OKAY){
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case COND:
      error_code = VerifyALIASES(CondStatList(stat),childlist);
      if (error_code != DEF_OKAY){
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case SELECT:
      /* statements inside SELECT are analyzed as part of the flat
       * statement list
       * fall through
       */
    case REF:
    case ISA:
    case WHEN:
    default: /* IRT, ATS, AA, REL, ASGN, RUN, IF, EXT, CASGN, too. */
      break;
    }
  }
  return DEF_OKAY;
}

/* calls functions to check child name list against lhs of statements
 * Recurses in for loops. builds the child name list as it goes through
 * subsidiary functions.
 */
static
enum typelinterr DoIS_A(CONST struct StatementList *stats)
{
  register struct gl_list_t *statements;
  register unsigned long c,len;
  enum typelinterr error_code;
  struct Statement *stat;
  statements = GetList(stats);
  len = gl_length(statements);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case WILLBE:
    case ISA:
      /* the type part to the statement was checked during parse,
       * but not type arguments.
       */
      error_code = DoVarList(GetStatVarList(stat),
                             FindType(GetStatType(stat)),stat);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case ALIASES:
      /* for aliases, checking the rhs can only be done partially,
       * and only after the whole child list is available.
       */
      error_code = DoVarList(GetStatVarList(stat),NULL,stat);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case ARR:
      /* for aliases, checking the rhs can only be done partially,
       * and only after the whole child list is available.
       */
      error_code = DoVarList(ArrayStatAvlNames(stat),NULL,stat);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      error_code = DoVarList(ArrayStatSetName(stat),NULL,stat);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case REF:
      error_code = DoVarList(ReferenceStatVlist(stat),NULL,stat);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat,error_code);
        return error_code;
      }
      break;
    case SELECT:
      /* All the statements in the select are now in the main
       * statement list which is a flat list. the following
       * code is not required anymore;

       * error_code = DoSelectList(SelectStatCases(stat));
       * if (error_code != DEF_OKAY) {
       * TypeLintError(ASCERR,stat, error_code);
       * return error_code;
       * }

       */
      break;
    case FOR:
      error_code = DoIS_A(ForStatStmts(stat));
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case COND:
      error_code = DoIS_A(CondStatList(stat));
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    default: /* IRT, ATS, AA, REL, ASGN, RUN, WHEN, IF, EXT, CASGN  */
      break;
    }
  }
  return DEF_OKAY;
}

/*
 * make a name string unique in the child list of the
 * format <typename>_<relnum><sufficient a-z letters to be
 * unique>.
 * Side effects: leaves lclpivot at or near place name ought
 * to be added in the list.
 */
static
symchar *GenerateId(symchar *type,
                    CONST char *module,
                    unsigned long int number)
{
  unsigned length;
  symchar *result;
  char statname[MAXTOKENLENGTH+12],c;
  sprintf(statname,"%s_%lu",SCP(type),number);
  result = AddSymbol(statname);
  if (FindLCL(result)==NULL) {
    return result;
  }
  length = SCLEN(result);
  while( (length+1) < (MAXTOKENLENGTH+12) ) {
    statname[length+1]='\0';
    for(c='a';c<='z';c++){
      statname[length]=c;
      result = AddSymbol(statname);
      if (FindLCL(result)==NULL) {
        return result;
      }
    }
    length++;
  }
  Asc_Panic(2, NULL,
            "%s Unable to generate unique name.\n"
            "  The statement is in module %s.\n"
            "  Insufficiently uniqe name is \n%s.  Burp!\n",
            StatioLabel(4), module, statname);

}

static int IndexUsed(symchar *name, CONST struct Expr *expr);

static int UsedInSet(symchar *name, CONST struct Set *sptr)
{
  while (sptr != NULL){
    if (SetType(sptr)) {	/* range */
      if (IndexUsed(name,GetLowerExpr(sptr))) return 1;
      if (IndexUsed(name,GetUpperExpr(sptr))) return 1;
    } else {			/* single */
      if (IndexUsed(name,GetSingleExpr(sptr))) return 1;
    }
    sptr = NextSet(sptr);
  }
  return 0;
}


static
int UsedInVar(symchar *name, CONST struct Name *nptr)
{
  /* check if it is a exact match */
  if ((nptr !=NULL)&&NameId(nptr)&&(NextName(nptr)==NULL)&&
      (NameIdPtr(nptr) == name)) {
    return 1;
  }
  while (nptr!=NULL){
    if (!NameId(nptr))
      if (UsedInSet(name,NameSetPtr(nptr))) return 1;
    nptr = NextName(nptr);
  }
  return 0;
}

static
int IndexUsed(symchar *name, CONST struct Expr *expr)
{
  while (expr!=NULL){
    switch(ExprType(expr)){
    case e_var:
      if (UsedInVar(name,ExprName(expr))) return 1;
      break;
    case e_set:
      if (UsedInSet(name,ExprSValue(expr))) return 1;
      break;
    case e_card:
    case e_choice:
    case e_sum:
    case e_prod:
    case e_union:
    case e_inter:
      if (UsedInSet(name,ExprBuiltinSet(expr))) return 1;
      break;
    default:
        /* e_func e_int e_real e_boolean e_symbol e_plus e_minus e_times
       	* e_divide e_power e_subexpr e_const e_par e_glassbox
       	* e_blackbox e_opcode e_token e_undefined e_nop e_or e_and
        * e_in e_st e_equal e_notequal e_less e_greater e_lesseq
        * e_greatereq e_not e_uminus e_qstring e_maximize e_minimize
        * e_zero
        */
      break;
    }
    expr = NextExpr(expr);
  }
  return 0;
}

static
struct Name *CreateIndexName(symchar *name)
{
  return CreateSetName(CreateSingleSet(CreateVarExpr(CreateIdName(name))));
}

static
struct Name *GenerateRelationName(symchar *type,
        			  CONST char *module,
        			  struct Expr *expr,
        			  unsigned long int relnum,
        			  struct gl_list_t *ft)
{
  struct Name *result;
  unsigned long activefors;
  symchar *idname;
  struct for_var_t *fv;
  idname = GenerateId(type,module,relnum);
  result = CreateSystemIdName(idname);
  activefors = ActiveForLoops(ft);
  while (activefors>0){
    fv = LoopIndex(ft,activefors);
    if (IndexUsed(GetForName(fv),expr)){
      result = JoinNames(result,CreateIndexName(GetForName(fv)));
    }
    activefors--;
  }
  return result;
}

static
struct Name *GenerateWhenName(symchar *type,
        		      CONST char *module,
        		      unsigned long int linenum,
        		      struct gl_list_t *ft)
{
  struct Name *result;
  unsigned long activefors;
  symchar *idname;
  struct for_var_t *fv;
  idname = GenerateId(type,module,linenum);
  result = CreateSystemIdName(idname);
  activefors = ActiveForLoops(ft);
  while (activefors>0){
    fv = LoopIndex(ft,activefors);
    result = JoinNames(result,CreateIndexName(GetForName(fv)));
    activefors--;
  }
  return result;
}


/* this function makes sure the relation has a name, generating
 * one if required.
 */
static
int DoRelation(symchar *type,
               struct Statement *stat,
               struct gl_list_t *ft)
{
  struct Name *nptr;
  /* CONSOLE_DEBUG("..."); */
  assert(stat && (StatementType(stat) == REL));
  g_number++;
  nptr = RelationStatName(stat);
  if (nptr == NULL){
    nptr = GenerateRelationName(type,Asc_ModuleName(StatementModule(stat)),
        			RelationStatExpr(stat),
        			g_number,ft);
    SetRelationName(stat,nptr);
  } else {
    if (ActiveForLoops(ft)+1 != (unsigned long)NameLength(nptr) ||
	NextIdName(nptr) != NULL) {
      return DEF_RELARRAY_SUBS;
    }
  }
  return DoName(nptr,FindRelationType(),stat);
}

static
int DoWhen(symchar *type,
           struct Statement *stat,
           struct gl_list_t *ft)
{
  struct Name *nptr;
  assert(stat && (StatementType(stat) == WHEN));
  g_number++;
  if ((nptr = WhenStatName(stat))==NULL){
    nptr = GenerateWhenName(type,Asc_ModuleName(StatementModule(stat)),
        		    g_number,ft);
    SetWhenName(stat,nptr);
  }
  return DoName(nptr,FindWhenType(),stat);
}

static
int DoLogRel(symchar *type,
               struct Statement *stat,
               struct gl_list_t *ft)
{
  struct Name *nptr;
  assert(stat && (StatementType(stat) == LOGREL));
  g_number++;
  nptr = LogicalRelStatName(stat);
  if (nptr ==NULL) {
    nptr = GenerateRelationName(type,Asc_ModuleName(StatementModule(stat)),
        			LogicalRelStatExpr(stat),
        			g_number,ft);
    SetLogicalRelName(stat,nptr);
  } else {
    if (ActiveForLoops(ft)+1 != (unsigned long)NameLength(nptr) ||
	NextIdName(nptr) != NULL) {
      return DEF_RELARRAY_SUBS;
    }
  }
  return DoName(nptr,FindLogRelType(),stat);
}


/** Process an external statement (i.e. add it to the child list, simply) */
static
int DoExternal(symchar *type,
               struct Statement *stat,
               struct gl_list_t *ft)
{
  struct Name *nptr;
  int doname_status;

  (void) type; (void) ft;

  assert(stat && (StatementType(stat) == EXT));
  /*
   * The grammar specifies that External function calls
   * must be named.
   */
  switch (ExternalStatMode(stat) ) {
  case ek_black:
    nptr = ExternalStatNameRelation(stat);
    doname_status = DoName(nptr,FindRelationType(),stat);
    return doname_status;
  case ek_glass:
    nptr = ExternalStatNameRelation(stat);
    doname_status = DoName(nptr,FindExternalType(),stat);
    return doname_status;
  default:
    nptr = NULL;
    break;
  }

  /* if a ext method shows up, we'll get DEF_NAME_INCORRECT as nptr is null */
  doname_status = DoName(nptr,FindExternalType(),stat);
  /* CONSOLE_DEBUG("DONAME STATUS = %d",doname_status); */
  return doname_status;
}


/*
 * Since we implemented the WHEN statement as an instance, we
 * generate an automatic name for each WHEN. The following
 * function deals with the names of nested WHENs
 */
static
enum typelinterr ProcessWhenCases(symchar *type,
                                  struct WhenList *whenlist,
                                  struct gl_list_t *ft)
{
  enum typelinterr error_code;
  while (whenlist!=NULL){
    error_code = DoWhens(type,WhenStatementList(whenlist),ft);
    if (error_code != DEF_OKAY) {
      return error_code;
    }
    whenlist = NextWhenCase(whenlist);
  }
  return DEF_OKAY;
}



/*
 * This function is supposed to handle the relations inside a
 * SELECT statement. However, now all of the statements inside
 * SELECT are contained in the main statement list, which is
 * flat. So, it is not required anymore, thus, the #ifdef
 *
 */
#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
enum typelinterr ProcessSelectCases(CONST char *type,
        	                    struct SelectList *selectlist,
        	                    struct gl_list_t *ft)
{
  enum typelinterr error_code;
  while (selectlist!=NULL){
    error_code = DoRelations(type,SelectStatementList(selectlist),ft);
    if (error_code !=DEF_OKAY) return error_code;
    selectlist = NextSelectCase(selectlist);
  }
  return DEF_OKAY;
}
#endif /*  THIS_IS_AN_UNUSED_FUNCTION  */


static
enum typelinterr DoRelations(symchar *type,
        	             CONST struct StatementList *stats,
        	             struct gl_list_t *ft)
{
  register struct gl_list_t *statements;
  register unsigned long c,len;
  register struct Statement *stat;
  enum typelinterr error_code;
  statements = GetList(stats);
  len = gl_length(statements);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case REL:
      error_code = DoRelation(type,stat,ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case LOGREL:
      error_code = DoLogRel(type,stat,ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case EXT:
      /* CONSOLE_DEBUG("PROCESSING EXTERNAL REL"); */
      error_code = DoExternal(type,stat,ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case SELECT:
      /*
       * Now all of the statements inside a SELECT (including
       * relations )are contained in the main statement list, which is
       * which is flat. So, this case  is not required anymore.
       *

       * error_code = ProcessSelectCases(type,SelectStatCases(stat),ft);
       * if (error_code != DEF_OKAY) {
       * TypeLintError(ASCERR,stat, error_code);
       * return error_code;
       * }

       */
      break;
    case FOR:
      AddLoopVariable(ft,CreateForVar(ForStatIndex(stat)));
      error_code = DoRelations(type,ForStatStmts(stat),ft);
      RemoveForVariable(ft);
      if (error_code != DEF_OKAY) {
        return error_code;
      }
      break;
    case COND:
      error_code=DoRelations(type,CondStatList(stat),ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    default: /* ISA, IRT, ATS, AA, ASGN, WHEN, RUN, IF, REF, CASGN, CALL*/
      break;
    }
  }
  return DEF_OKAY;
}

/*
 * Since we implemented the WHEN statement as an instance, we
 * generate an automatic name for each WHEN. The following
 * function deals with the names of a WHEN statement. For
 * nested WHEN, the function ProcessWhenCases is called.
 */


static
enum typelinterr DoWhens(symchar *type,
                         CONST struct StatementList *stats,
                         struct gl_list_t *ft)
{
  register struct gl_list_t *statements;
  register unsigned long c,len;
  register struct Statement *stat;
  enum typelinterr error_code;
  statements = GetList(stats);
  len = gl_length(statements);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case WHEN:
      error_code = DoWhen(type,stat,ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      error_code = ProcessWhenCases(type,WhenStatCases(stat),ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case FOR:
      AddLoopVariable(ft,CreateForVar(ForStatIndex(stat)));
      error_code = DoWhens(type,ForStatStmts(stat),ft);
      RemoveForVariable(ft);
      if (error_code != DEF_OKAY)  {
        return error_code;
      }
      break;
    default:
      break;
    }
  }
  return DEF_OKAY;
}


/*****************************************************************\
  Functions to help determine the types of children.
\*****************************************************************/

/*
 * this is a little structure we require for a temporary singly linked
 * list of statements.
 */
enum albits {
  AL_WAIT = 0, /* wait means we haven't been able to determine anything yet */
  AL_WARR,     /* nothing yet on ARR statement common rhslist type. */
  AL_DONE,     /* done means we've processed this alias statement */
  AL_NORHS     /* norhs means the rhs cannot be found */
};

struct AliasList {
  struct Statement *stat;
  struct AliasList *next;
  enum albits bits;
};

/*
 * create an alias list entry using statement s.
 */
static
struct AliasList *ALCreate(struct Statement *s)
{
  struct AliasList *ret;
  ret = (struct AliasList *)ascmalloc(sizeof(struct AliasList));
  assert(ret!=NULL);
  ret->stat = s;
  ret->next = NULL;
  if (StatementType(s) == ARR) {
    ret->bits = AL_WARR;
  } else {
    ret->bits = AL_WAIT;
  }
  return ret;
}

/*
 * Destroy an aliases list entry
 */
static
void ALDestroy(struct AliasList *a)
{
  ascfree(a);
}

/*
 * Destroy an aliases list. input may be null.
 */
static
void DestroyAliasesList(struct AliasList *a)
{
  struct AliasList *old;
  while (a != NULL) {
    old = a;
    a = old->next;
    ALDestroy(old);
  }
}
/*
 * this function creates prepends an alias list entry
 * and returns the new head of the list.
 * ele or list may be null.
 */
static
struct AliasList *ALPrepend(struct AliasList *list, struct AliasList *ele)
{
  if (ele==NULL) return list;
  if (list != NULL) {
    if (ele->next == NULL) {
      /* usual cheap case */
      ele->next = list;
    } else {
      /* case where ele is a list */
      struct AliasList *tail;
      tail = ele;
      while (tail->next != NULL) {
        tail = tail->next;
      }
      tail->next = list;
    }
  }
  return ele;
}

/*
 * Returns a list of aliases statements found in the
 * given list.
 * Recursive in some compound statements
 */
static
struct AliasList *CreateAliasesList(struct StatementList *stats)
{
  register struct Statement *stat;
  struct gl_list_t *statements;
  struct AliasList *result=NULL;
  unsigned long c,len;

  statements = GetList(stats);
  len = gl_length(statements);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case ALIASES:
    case ARR:
      result = ALPrepend(result,ALCreate(stat));
      break;
    case FOR:
      result = ALPrepend(result,CreateAliasesList(ForStatStmts(stat)));
      break;
    case SELECT:
      /*
       * Now all of the statements inside a SELECT statement
       * are contained in the main statement list, which is
       * flat. So, this case is not required anymore.
       *
       */
      break;
    default:
      break;
    }
  }
  return result;
}

/*
 */
enum e_findrhs {
  FRC_ok = 0,   /* got type. name is simple */
  FRC_array,    /* got type, which is base of array */
  FRC_badname,  /* part named is impossible */
  FRC_attrname, /* part named is subatomic and can't be aliased */
  FRC_unable,   /* unable to determine type of rhs, but might later */
  FRC_fail      /* unable to determine type of rhs ever */
};

/*
 * This function tries to determine the first chain item in a name
 * which was constructed by being passed into the type of
 * which the item is a part.
 * Works from the childlist of the typedescription given.
 * This function will recurse as required.
 * The name given may or may not be compound.
 * Possible returns:
 *  FRC_ok: no parameter origin parts were found. *nptrerr <-- NULL.
 *  FRC_badname: part name starting at *nptrerr is impossible, array, etc.
 *  FRC_attrname: part named starting at *nptrerr is subatomic.
 *  FRC_array:  part named starting at *nptrerr has too many/not enough []
 *  FRC_fail: parametric origin part was found. *nptrerr <-- param part.
 *  FRC_unable: never returned.
 * On the first call from the user, nptrerr should be a name to
 * evaluate in the context of the type given.
 */
static
enum e_findrhs AANameIdHasParameterizedPart(CONST struct Name **nptrerr,
                                            CONST struct TypeDescription *type)
{
  CONST struct Name *nptr;
  CONST struct Name *pnptr,*tnptr;
  CONST struct TypeDescription *rtype;
  ChildListPtr cl;
  unsigned long pos;
  int alen,subseen,subsleft;

  assert(type!=NULL);
  assert(NameId(*nptrerr)!=0);
  nptr = *nptrerr;
  assert(nptr!=NULL);

  if ( GetBaseType(type)== patch_type) {
    type = GetPatchOriginal(type);
    if (type==NULL) {
      return FRC_badname;
    }
  }
  if ( GetBaseType(type) != model_type) {
    /* cannot alias subatomic parts, and arrays don't have independent
     * typedescs yet.
     */
    return FRC_attrname;
  }
  cl = GetChildList(type);
  if (cl==NULL) {
    /* very wierd case, but then we have very wierd users. */
    return FRC_badname;
  }
  pos = ChildPos(cl,NameIdPtr(nptr));
  if (pos == 0) { /* name not found */
    return FRC_badname;
  }
  rtype = ChildBaseTypePtr(cl,pos);
  if (rtype == NULL) {
    return FRC_badname;
  }
  if (ChildParametric(cl,pos)!=0) {
    return FRC_fail;
  }
  alen = ChildIsArray(cl,pos);
  pnptr = NextIdName(nptr);
  if (pnptr==NULL) {
    /* end of the dot qualified line */
    tnptr = NextName(nptr);
    if (tnptr==NULL) {
      /* a simple name possibly root of array */
      if (alen) {
        return FRC_array; /* we don't like array roots */
      } else {
        *nptrerr=NULL;
        return FRC_ok;
      }
    } else {
      /* sub array or array element. */
      subseen = 0;
      while (tnptr!=pnptr)  {
        subseen++;
        tnptr=NextName(tnptr);
      }
      subsleft = alen - subseen;
      if (subsleft < 0) { /* name not found. too many subscripts. */
        return FRC_array;
      }
      if (subsleft) {
        return FRC_array; /* we don't like array roots */
      } else {
        *nptrerr=NULL;
        return FRC_ok;
      }
    }
  }
  /* there's more to the name. keep going, after checking that
   * all subscripts required are filled.
   */
  subseen = 0;
  tnptr = NextName(nptr);
  while (tnptr!=pnptr) {
    subseen++;
    tnptr=NextName(tnptr);
  }
  subsleft = alen - subseen;
  if (subsleft != 0) {
    /* name not found. too many/not enough subscripts. */
    return FRC_array;
  }
  *nptrerr = pnptr;
  return AANameIdHasParameterizedPart(nptrerr,rtype);
}

/*
 * This function tries to determine the type of the name given
 * based on the childlist in the typedescription given.
 * Return value is in accordance with the header for
 * FIndRHSType.
 * This function will recurse as required.
 * The name given may or may not be compound.
 * Type must be of a MODEL or patch with a child list.
 */
static
CONST struct TypeDescription
*FindChildTypeFromName(CONST struct Name *nptr,
                       CONST struct TypeDescription *type,
                       enum e_findrhs *rval,
                       int *rlen)
{
  CONST struct Name *pnptr,*tnptr;
  CONST struct TypeDescription *rtype;
  ChildListPtr cl;
  unsigned long pos;
  int alen,subseen,subsleft;

  assert(type!=NULL);
  assert(NameId(nptr)!=0);
  assert(rval!=NULL);
  if ( GetBaseType(type)== patch_type) {
    type = GetPatchOriginal(type);
    if (type==NULL) {
      *rval = FRC_fail;
      return NULL;
    }
  }
  if ( GetBaseType(type) != model_type) {
    /* cannot alias subatomic parts, and arrays don't have independent
     * typedescs yet.
     */
    *rval = FRC_attrname;
    return NULL;
  }
  cl = GetChildList(type);
  if (cl==NULL) {
    /* very wierd case, but then we have very wierd users. */
    *rval = FRC_badname;
    return NULL;
  }
  pos = ChildPos(cl,NameIdPtr(nptr));
  if (pos == 0) { /* name not found */
    *rval = FRC_badname;
    return NULL;
  }
  rtype = ChildBaseTypePtr(cl,pos);
  if (rtype == NULL) {
    /* rhs type not established. will not be later. */
    *rval = FRC_fail;
    return NULL;
  }
  alen = ChildIsArray(cl,pos);
  pnptr = NextIdName(nptr);
  if (pnptr==NULL) {
    /* end of the dot qualified line */
    tnptr = NextName(nptr);
    if (tnptr==NULL) {
      /* aliasing a simple name */
      *rlen = alen;
      if (alen) {
        *rval = FRC_array;
      } else {
        *rval = FRC_ok;
      }
    } else {
      /* aliasing sub array or array element. */
      subseen = 0;
      while (tnptr!=pnptr)  {
        subseen++;
        tnptr=NextName(tnptr);
      }
      subsleft = alen - subseen;
      if (subsleft < 0) { /* name not found. too many subscripts. */
        *rval = FRC_badname;
        *rlen = 0;
        return NULL;
      }
      *rlen = subsleft;
      if (subsleft) {
        *rval = FRC_array;
      } else {
        *rval = FRC_ok;
      }
    }
    return rtype;
  }
  /* there's more to the name. keep going, after checking that
   * all subscripts required are filled.
   */
  subseen = 0;
  tnptr = NextName(nptr);
  while (tnptr!=pnptr) {
    subseen++;
    tnptr=NextName(tnptr);
  }
  subsleft = alen - subseen;
  if (subsleft != 0) {
    /* name not found. too many/not enough subscripts. */
    *rlen = 0;
    *rval = FRC_badname;
    return NULL;
  }
  return FindChildTypeFromName(pnptr,rtype,rval,rlen);
}

/*
 * This function tries to determine from the RHS name of an
 * aliases statement what the type of that name
 * is, to a first approximation, and whether it is definitely array.
 * Returns NULL if type not found. If not null, *rval will contain
 * FRC_ok or FRC_array to communicate arrayness.
 *
 * There is some ambiguity about arrays because (without finding
 * the defining statement which is tricky) we can't know how many
 * subscripts there are and hence can't know whether an array name
 * points to a final element or to a sub-array.
 *
 * This function does some preliminary work to digest the rhs part
 * name based on the clist given, and then (if required) hands off
 * to a routine which determines (if possible) from the existing type tree
 * what the type is that goes with the name.
 *
 * 12/96 Revisions:
 * Finds the EXACT ARRAYNESS, not an approximation. FRC_ok only when
 * the name resolves to a single array element, OTHERWISE FRC_array.
 * If FRC_array, then *rlen will be the number of subscripts left
 * unspecified in the name, OTHERWISE rlen should be ignored.
 * It does NOT check in a for table. You can't alias dummy vars.
 * *origin will be the origin_ flag of the first name element
 * (local scope name) if return value is FRC_ok or FRC_array.
 * OTHERWISE *origin will be returned as an ERR.
 *
 * Due to its multiple usages, this function is not well named,
 * nor is its behavior particularly simple. Since the CHOICE is
 * between overdue and do-over, this is a do-over. The price of
 * handling errors in a language which specializes in managing
 * anarchy is really quite high.
 */
static
CONST struct TypeDescription *FindRHSType(CONST struct Name *nptr,
                                          CONST struct gl_list_t *clist,
                                          enum e_findrhs *rval,
                                          int *rlen,
                                          unsigned int *origin)
{
  CONST struct Name *pnptr, *tnptr;
  struct ChildListEntry *found;
  struct ChildListEntry test;
  unsigned long pos;

  *origin = origin_ERR;
  /* digest the first part of the name in the local scope */
  if (!NameId(nptr)) {
    /* names  like [1] are obviouslly goop */
    *rval = FRC_badname;
    return NULL;
  }
  test.strptr = NameIdPtr(nptr); /* fetch the symchar */
  pos = gl_search(clist,&test,(CmpFunc)CmpChildListEntries);
  if (pos == 0) {
    /* name not found */
    *rval = FRC_badname;
    return NULL;
  }
  /* name found. */
  found = (struct ChildListEntry *) gl_fetch(clist,pos);
  if (found->typeptr == NULL || found->isarray < 0) {
    /* rhs type not yet established. try later. */
    *rval = FRC_unable;
    return NULL;
  }
  *origin = found->origin;
  *rlen = found->isarray;
  if (NameLength(nptr) == 1) {
    /* local scalar name */
    if (found->isarray) {
      *rval = FRC_array;
    } else {
      *rval = FRC_ok;
    }
    return found->typeptr;
  }
  /* compound name. could be local or part of part. */
  pnptr = NextIdName(nptr);
  tnptr = NextName(nptr);
  while (tnptr!=pnptr) {
    (*rlen)--;
    tnptr = NextName(tnptr);
  }
  if (*rlen < 0) {
    *rval = FRC_badname;
    return NULL;
  }
  if (pnptr==NULL) {
    if (*rlen > 0) {
      *rval = FRC_array;
    } else {
      *rval = FRC_ok;
    }
    return found->typeptr;
  } else {
    if (*rlen > 0) {
      /* name is of form a.b where it should be a[k].b; missing subscripts */
      *rval = FRC_badname;
      return NULL;
    }
    return FindChildTypeFromName(pnptr,found->typeptr,rval,rlen);
  }
}

/*
 * Need to watch out for a.b type names and not mark them?
 */
static
void MarkIfPassedArgs(CONST struct Name *nptr, CONST struct gl_list_t *clist)
{
  CONST struct Name *pnptr;
  struct ChildListEntry *found;
  struct ChildListEntry test;
  int rlen;
  unsigned long pos;

  /* digest the first part of the name in the local scope */
  if (!NameId(nptr)) {
    /* names  like [1] are obviouslly goop */
    return;
  }
  test.strptr = NameIdPtr(nptr); /* fetch the symchar */
  pos = gl_search(clist,&test,(CmpFunc)CmpChildListEntries);
  if (pos == 0) {
    /* name not found */
    return;
  }
  /* name found. */
  found = (struct ChildListEntry *) gl_fetch(clist,pos);
  rlen = found->isarray;
  if (NameLength(nptr) == 1) {
    /* local scalar name */
    found->bflags |= CBF_PASSED;
    return;
  }
  /* compound name. could be local or part of part. */
  pnptr = NextIdName(nptr); /* if a.b, pnptr will be not NULL */
  if (pnptr == NULL) {
    /* local array name */
    found->bflags |= CBF_PASSED;
  }
}

/*
 * This function tries to determine from the RHS name list of an
 * ALIASES-IS_A statement what the basetype of the array should be.
 * to a first approximation, and whether it is a well formed array.
 * Doesn't try to ferret out array subscript type (int/sym) mismatches.
 * Returns NULL if type not derivable. If not null, *rval will contain
 * FRC_ok or FRC_array to communicate arrayness.
 *
 * Finds the exact arrayness, not an approximation. FRC_ok only when
 * the name resolves to a single array element, OTHERWISE FRC_array.
 * If FRC_array, then *rlen will be the number of subscripts left
 * unspecified in the name, OTHERWISE rlen should be ignored.
 * It does NOT check in a for table--  You can't alias dummy vars.
 * When return value is FRC_ok or FRC_array
 * *origin will be the origin_ARR or origin_PARR
 * OTHERWISE *origin should be ignored on return.
 * If any of the list is parametric, *origin is origin_PARR.
 * This may lead to some incorrect restrictions on the elements of
 * the array created from the variablelist given.
 */
static
CONST struct TypeDescription *FindCommonType(CONST struct VariableList *vl,
                                             CONST struct gl_list_t *clist,
                                             enum e_findrhs *val,
                                             int *len,
                                             unsigned int *origin)
{
  /* return value holders */
  CONST struct TypeDescription *rtype=NULL;
  enum e_findrhs rval = FRC_fail;
  int rlen = -1;
  int parametric = 0;

  /* temporaries */
  CONST struct Name *nptr;
  CONST struct TypeDescription *type;

  while (vl != NULL) {
    nptr = NamePointer(vl);
    type = FindRHSType(nptr,clist,val,len,origin);
    if (type == NULL) {
      switch (*val) {
      case FRC_ok:
      case FRC_array:
        /* good FRC codes not seen if type is NULL */
        ASC_PANIC("good FRC codes not seen if type is NULL");
        break;
      case FRC_badname:  /* part named is impossible */
      case FRC_attrname: /* part named is subatomic and can't be aliased */
        TLNM(ASCERR,nptr,"Impossible/subatomic name: ",3);
        *val = FRC_fail;
        break;
      case FRC_unable:   /* unable to determine type of rhs, but might later */
        break;
      case FRC_fail:     /* unable to determine type of rhs ever */
        TLNM(ASCERR,nptr,"Type indeterminate name: ",3);
        *val = FRC_fail;
        break;
      }
      return NULL;
    }
    /* else we have some type, be it base of array or OTHERWISE */
    if (rtype != NULL) {
      /* check base type compatibility */
      rtype = GreatestCommonAncestor(rtype,type);
      if (rtype==NULL) {
        TLNM(ASCERR,nptr,"Type incompatible name: ",3);
        *val = FRC_fail;
        return NULL;
      }
      /* check arrayness equivalent */
      if (*val != rval /* mismatched FRC_ok and FRC_array */ ||
          *len != rlen /* mismatched number of subscripts */) {
        TLNM(ASCERR,nptr,"Array dimensionally incompatible name: ",3);
        *val = FRC_fail;
        return NULL;
      }
      /* modify parametric as needed */
      parametric = (ParametricOrigin(*origin) || parametric);
    } else {
      /* first case */
      rtype = type; /* this value may become less refined */
      rlen = *len; /* this value will persist to end if successful */
      rval = *val; /* this value will persist to end if successful */
      parametric = ParametricOrigin(*origin);
    }
    vl = NextVariableNode(vl);
  }
  /* go here, so list was compatible in some way. */
  if (parametric!=0) {
    *origin = origin_PARR;
  } else {
    *origin = origin_ARR;
  }
  return rtype;
}

/*
 * This function takes the type given and its array status (in rval)
 * and marks all the names from the VariableList found in clist
 * as being of that type. Clist is a list of ChildListEntries.
 * Marking the type of the same child twice is fatal.
 * should only be called with vlists from aliases statements.
 */
static
void SetLHSTypes(CONST struct VariableList *vlist,struct gl_list_t *clist,
                 CONST struct TypeDescription *rtype, enum e_findrhs rval,
                 int subsopen, unsigned int origin)
{
  struct ChildListEntry test;
  struct ChildListEntry *clep;
  CONST struct Name *nptr;
  symchar *name;
  unsigned long place;

  (void) rval;
  while (vlist!=NULL) {
    nptr = NamePointer(vlist);
    name = NameIdPtr(nptr);
    assert(name!=NULL);
    test.strptr = name;
    place = gl_search(clist,&test,(CmpFunc)CmpChildListEntries);
    assert(place!=0);
    clep = (struct ChildListEntry *) gl_fetch(clist,place);
    assert(clep->typeptr==NULL);
    assert(subsopen >= 0);
    assert(origin!=origin_ERR);
    assert(clep->origin==origin_ALI || clep->origin==origin_ARR);
    assert(clep->isarray < 0);
    if (ParametricOrigin(origin)) {
      if (clep->origin == origin_ALI) {
        clep->origin = origin_PALI;
      } else {
        clep->origin = origin_PARR;
      }
    }
    clep->typeptr = rtype;
    clep->isarray = ABS(clep->isarray + 1) + subsopen;
    /*              ^^^^^^^^^^ works because of how we init it in DoNameF */
    vlist = NextVariableNode(vlist);
  }
}

/*
 * This function takes a completed list of child names with
 * type information from IS_A and relations and tries to
 * derive type information for names defined with aliases
 * and in other obscure ways in the list of stats.
 *
 * This function could be a bit cleverer, but we're not
 * going to optimize it until there is some justification.
 * Iterates over the list of alii until no more information
 * is derivable.
 * Before returning whines about unresolvable names, which
 * are probably local alias loops.
 * Returns the number of whines. required, normally 0.
 *
 * Needs to track IS_REFINED_TO ARE_THE_SAME and ARE_ALIKE
 * where possible, which basically means over complete sets.
 */
static
int DeriveChildTypes(struct StatementList *stats, struct gl_list_t *clist)
{
  struct AliasList *head, *tmp;
  CONST struct TypeDescription *rtype; /* rhs name type */
  int changed, whines=0;
  enum e_findrhs rval;
  int subsopen;
  unsigned int origin;

  head = CreateAliasesList(stats);
  changed = 1;
  while (changed) {
    tmp = head;
    changed = 0;
    while (tmp!=NULL) {
      switch(tmp->bits) {
      case AL_WAIT:
        rtype = FindRHSType(AliasStatName(tmp->stat),clist,
                            &rval,&subsopen,&origin);
        if (rtype != NULL) {
          changed = 1;
          SetLHSTypes(GetStatVarList(tmp->stat),clist,rtype,
                      rval,subsopen,origin);
          tmp->bits = AL_DONE;
        } else {
          switch (rval) {
          case FRC_badname: /* definitely garbage rhs */
            tmp->bits = AL_NORHS;
            MarkStatContext(tmp->stat,context_WRONG);
            STATEMENT_ERROR(tmp->stat,"Impossible RHS of ALIASES");
            WSS(ASCERR,tmp->stat);
            whines++;
            break;
          case FRC_attrname: /* ATOM child rhs */
            tmp->bits = AL_DONE;
            MarkStatContext(tmp->stat,context_WRONG);
            STATEMENT_ERROR(tmp->stat,"Illegal subatomic RHS of ALIASES");
            WSS(ASCERR,tmp->stat);
            whines++;
            break;
          case FRC_fail:    /* permanently ambiguous rhs name of part */
            WSEM(ASCWAR,tmp->stat,"Unable to determine child basetype");
            whines++;
            /* shouldn't happen, but symptom of certain screwups */
            changed = 1;
            tmp->bits = AL_DONE;
            break;
          case FRC_unable:  /* try later */
            break;
          default:
            ASC_PANIC("NOT REACHED should never see other values");
            break;
          }
        }
        break;
      case AL_WARR:
        rtype = FindCommonType(GetStatVarList(tmp->stat),clist,
                               &rval,&subsopen,&origin);
        if (rtype != NULL) {
          changed = 1;
          SetLHSTypes(ArrayStatAvlNames(tmp->stat),clist,rtype,
                      rval,subsopen,origin);
          tmp->bits = AL_DONE;
        } else {
          switch (rval) {
          case FRC_badname: /* definitely garbage rhs (masked) */
          case FRC_attrname: /* ATOM child rhs (masked) */
          case FRC_fail:    /* permanently confused ALIASES-IS_A */
            MarkStatContext(tmp->stat,context_WRONG);
            WSEM(ASCWAR,tmp->stat,
              "Unable to determine common ancestor type for array elements");
            WSS(ASCERR,tmp->stat);
            whines++;
            /* shouldn't happen, but symptom of certain screwups */
            /* such as the user trying to put incompatible stuff in an array */
            changed = 1;
            tmp->bits = AL_DONE;
            break;
          case FRC_unable:  /* try later */
            break;
          default:
            ASC_PANIC("NOT REACHED should never see other values");
            break;
          }
        }
        break;
      case AL_DONE:
      case AL_NORHS:
        break;
      }
      tmp = tmp->next;
    }
  }
  tmp = head;
  while (tmp!=NULL) {
    switch (tmp->bits) {
    case AL_WAIT:
    case AL_WARR:
      WSSM(ASCERR,tmp->stat,"Probably involved in recursive ALIASES",3);
      whines++;
      break;
    default:
      break;
    }
    tmp = tmp->next;
  }
  DestroyAliasesList(head);
  return whines;
}

/*****************************************************************\
  End of functions to help determine the types of children
  necessitated by aliases.
\*****************************************************************/

/*****************************************************************\
 begin stuff to help refine the types of children
 using ARE_ALIKE ARE_THE_SAME IS_REFINED_TO info.
\*****************************************************************/

/*
 * Little function to get the loops surrounding a sparse IS_A, srch.
 * Expensive task.
 * Recursive. Returns 1 if srch is found in sl or its descendants, 0 if not.
 * List returned to topmost caller will be a list of the for loops surround-
 * ing srch in reverse (INSIDE-OUT) order.
 * srch, sl and loops must all be nonnull on entry.
 * In the recursion, nothing gets appended to loops until the
 * srch statement is found.
 */
static
int GetISALoops(CONST struct Statement *srch,
                CONST struct StatementList *sl,
                struct gl_list_t *loops)
{
  struct Statement *s;
  unsigned long c, len;
  int ret;
  assert(srch!=NULL && sl!=NULL && loops != NULL && StatementType(srch)==ISA);

  len = StatementListLength(sl);
  for (c=1;c <= len;c++) {
    s = GetStatement(sl,c);
    if (s!=NULL) {
      if (StatementType(s)==FOR && ForContainsIsa(s)) {
        ret = GetISALoops(srch,ForStatStmts(s),loops);
        if (ret == 1) {
          gl_append_ptr(loops,(VOIDPTR)s);
          return 1;
        }
      }
      if (s == srch) return 1;
    }
  }
  return 0;
}

/* a little alternative forvar that carries the loop definition. */
struct forinfo_t {
  symchar *strname;
  struct Expr *ex;
};

/* delete anything sitting in forinfo and return */
static
void ClearForInfo(struct gl_list_t *fl)
{
  unsigned long c, len;
  if (fl!=NULL) {
    len = gl_length(fl);
    for (c=len;c>=1;c--) {
      gl_delete(fl,c,1);
    }
  }
}
/* compares forinfo by strnames. NULL  > all */
static
int CmpForInfo(struct forinfo_t *f1, struct forinfo_t *f2)
{
  if (f1==NULL) return 1;
  if (f2==NULL) return -1;
  if (f1->strname==NULL) return 1;
  if (f2->strname==NULL) return -1;
  if (f1->strname==f2->strname) return 0;
  return CmpSymchar(f1->strname,f2->strname);
}
static
struct forinfo_t *FindForInfo(struct gl_list_t *forinfo, symchar *name)
{
  struct forinfo_t test;
  unsigned long pos;
  if (name==NULL || forinfo == NULL) {
    return NULL;
  }
  test.strname = name;
  pos = gl_search(forinfo,&test,(CmpFunc)CmpForInfo);
  if (pos==0L) {
    return NULL;
  }
  return gl_fetch(forinfo,pos);
}
/* add name and ex to info list */
static
void AddForInfo( struct gl_list_t *forinfo,
                 symchar *name,
                 struct Expr *ex)
{
  struct forinfo_t *i=NULL;
  assert(name!=NULL);
  assert(ex!=NULL);
  i = (struct forinfo_t *)ascmalloc(sizeof(struct forinfo_t));
  assert(i!=NULL);
  i->strname = name;
  i->ex = ex;
  gl_append_ptr(forinfo,(VOIDPTR)i);
}
/* delete named entry from list, after finding it */
static
void RemoveForInfo( struct gl_list_t *forinfo, symchar *name)
{
  struct forinfo_t test;
  unsigned long pos;
  test.strname = name;
  pos = gl_search(forinfo,&test,(CmpFunc)CmpForInfo);
  if (pos==0L) {
    FPRINTF(ASCERR,"Nonexistent forinfo %s removed\n",SCP(name));
    return;
  }
  gl_delete(forinfo,pos,1);
}
/*
 * takes a local array name n and tries to match the subscripts named
 * in it against the declaration of the array via IS_A.
 * clep given should correspond to n given.
 * Stuff that is aliased will most likely return FALSE negative results.
 * Returns 1 if all the elements of the array declared are named in
 * the name given. In the case of names containing for loop indices,
 * the range of the for is checked in forinfo to see if that matches
 * the IS_A.
 * Returns 0 if mismatch or too hard to tell.
 * Basically, sets must compare exactly in their unevaluated form
 * for this work. Some of the twisty sparse array addressings allowed
 * in the language may be indecipherable and yield a FALSE negative.
 */
static
int AllElementsNamed(struct ChildListEntry *clep,
                     CONST struct Name *n,
                     struct gl_list_t *clist,
                     struct gl_list_t *forinfo,
                     struct StatementList *pstats)
{
  CONST struct Name *decln=NULL; /* name IS_A'd/WILL_BE'd under */
  CONST struct VariableList *vl;
  struct Set *fset;
  struct Set *fsetorig;
  CONST struct Expr *sex;
  struct forinfo_t *fi;
  struct forinfo_t *origfi;
  struct gl_list_t *looplist;
  struct gl_list_t *loopinfo;
  struct Statement *s;
  unsigned long c,len;
  int killfset=0;
  int killfsetorig=0;
  int setcomp;

  if (clep == NULL || clep->statement == NULL ||
      n==NULL || clist == NULL || forinfo == NULL ||
      StatementType(clep->statement) == ALIASES /* alii too hard */ ) {
    return 0;
  }
  /* hunt out the name declared in original IS_A */
  vl = GetStatVarList(clep->statement);
  while (vl != NULL) {
    /* name elements are out of symbol table, so compare by ptr to syms. */
    if (NameIdPtr(NamePointer(vl)) == NameIdPtr(n)) {
      decln = NamePointer(vl);
      break;
    }
    vl = NextVariableNode(vl);
  }
  if (decln == NULL || NameLength(decln)!=NameLength(n)) {
    /* damned odd! */
    return 0;
  }
  /* ok, so decln is the name we want to match and n is the
   * name used in the refinement statement.
   * To match sparse IS_REFINED_TO to sparse IS_A properly is
   * a second, fairly major case.
   */
  /* eat array heads */
  decln = NextName(decln);
  n = NextName(n);
  if (StatInFOR(clep->statement) == 0 ) {
    /*
     * This only works for dense IS_A's.
     */
    /* do for each subscript */
    while (n != NULL) {
      /* compare either the for loop expression or the name set of n
       * to the set defined in dense decln.
       */
      if (SetType(NameSetPtr(n))==0 &&
          (sex = GetSingleExpr(NameSetPtr(n))) != NULL &&
          ExprListLength(sex) == 1 &&
          ExprType(sex) == e_var &&
          (fi = FindForInfo(forinfo,SimpleNameIdPtr(ExprName(sex)))) != NULL
         ) {
        /* must be a for index */
        if (ExprListLength(fi->ex)!=1 || ExprType(fi->ex) != e_set) {
          fset = CreateSingleSet(fi->ex);
          killfset = 1;
        } else {
          fset = ExprSValue(fi->ex);
        }
        setcomp = CompareSetStructures(fset,NameSetPtr(decln));
        if (killfset) {
          DestroySetHead(fset);
        }
        if (setcomp != 0) {
          return 0;
        }
      } else {
        if (CompareSetStructures(NameSetPtr(n),NameSetPtr(decln))!=0) {
          return 0;
        }
      }
      decln = NextName(decln);
      n = NextName(n);
    }
  } else {
    /* sparse IS_A/sparse IS_REFINED_TO */
    looplist = gl_create(2L);
    if (looplist == NULL) {
      return 0;
    }
    (void)GetISALoops(clep->statement,pstats,looplist);
    if (gl_length(looplist)==0L) {
      gl_destroy(looplist);
      return 0;
    } else {
      /* convert looplist to forvar info */
      loopinfo = gl_create(gl_length(looplist));
      if (loopinfo == NULL) {
        gl_destroy(looplist);
        return 0;
      }
      len = gl_length(looplist);
      for (c=1;c <= len; c++) {
        s = (struct Statement *)gl_fetch(looplist,c);
        AddForInfo(loopinfo,ForStatIndex(s),ForStatExpr(s));
      }
      gl_destroy(looplist);
      looplist = NULL;
    }
    /* things to clean up past this point: loopinfo */
    /* foreach subscript:
     *   find index from n in forinfo passed in and get its defining expr.
     *   find index from decln in looplist and get its defining expr.
     *   if sets !=, return 0, else cleanup and return 1.
     */
    while (n != NULL) {
      /* compare either the for loop expressions
       * to the sets defined in sparse decln.
       */
      /* must be a simple for index in IS_REFINED_TO/etc. get set
       * definitions corresponding to indices.
       */
      if (SetType(NameSetPtr(n))==0 &&
          (sex = GetSingleExpr(NameSetPtr(n))) != NULL &&
          ExprListLength(sex) == 1 &&
          ExprType(sex) == e_var &&
          (fi = FindForInfo(forinfo,SimpleNameIdPtr(ExprName(sex)))) != NULL &&
          /* found this statement's set expression */
          SetType(NameSetPtr(decln))==0 &&
          (sex = GetSingleExpr(NameSetPtr(decln))) != NULL &&
          ExprListLength(sex) == 1 &&
          ExprType(sex) == e_var &&
          (origfi = FindForInfo(loopinfo,SimpleNameIdPtr(ExprName(sex))))!=NULL
          /* found original statement's set expression */
         ) { /* end of if conditions */
        if (ExprListLength(fi->ex)!=1 || ExprType(fi->ex) != e_set) {
          fset = CreateSingleSet(fi->ex);
          killfset = 1;
        } else {
          fset = ExprSValue(fi->ex);
        }
        if (ExprListLength(origfi->ex)!=1 || ExprType(origfi->ex) != e_set) {
          fsetorig = CreateSingleSet(origfi->ex);
          killfsetorig = 1;
        } else {
          fsetorig = ExprSValue(origfi->ex);
        }
        setcomp = CompareSetStructures(fset,fsetorig);
        if (killfset) {
          DestroySetHead(fset);
        }
        if (killfsetorig) {
          DestroySetHead(fsetorig);
        }
        if (setcomp != 0) {
          ClearForInfo(loopinfo);
          gl_destroy(loopinfo);
          return 0;
        }
      } else {
        /* clean up. we gave up due to some complexity */
        ClearForInfo(loopinfo);
        gl_destroy(loopinfo);
        return 0;
      }
      decln = NextName(decln);
      n = NextName(n);
    }
    ClearForInfo(loopinfo);
    gl_destroy(loopinfo);
  }
  return 1;
}

/*
 * not that forinfo is NOT a forvar table; it should be a supplied,
 * empty gllist from the initial caller.
 * it will be returned empty and of no consequence to the initial
 * caller.
 * It should handle ARE_ALIKE/ARE_THE_SAME but so far only IS_REFINED_TO.
 * Recursive function.
 * When ascend code is well written, the current implementation
 * is sufficient to shut up all undefined name whines. code that
 * still whines is POORLY modeled.
 */
static int g_drt_depth=0; /* depth in this function, for bookkeeping. */
static
enum typelinterr DeriveRefinedTypes(struct StatementList *stats,
                                    struct gl_list_t *clist,
                                    struct gl_list_t *forinfo,
                                    struct StatementList *pstats
                                   )
{
  struct Statement *s;
  /* rhs type of IS_REFINED_TO statement */
  symchar *rname;
  CONST struct TypeDescription *rdesc;
  /* lhs member type from IS_REFINED_TO statement */
  CONST struct TypeDescription *d;
  CONST struct VariableList *vl;
  CONST struct Name *n;
  unsigned long c,len,pos;
  unsigned int origin;
  int subsopen;
  enum e_findrhs rval;
  enum typelinterr error_code;
  struct ChildListEntry test;
  struct ChildListEntry *clep;

  assert(clist !=NULL);
  assert(forinfo !=NULL);

  len = StatementListLength(stats);
  for (c = 1; c <= len; c++) {
    s = GetStatement(stats,c);
    switch (StatementType(s)) {
    case IRT:
      if (StatInSELECT(s)) { /* Ignore refinements inside SELECT */
        break;
      }
      rname = GetStatType(s); /* sets do not get refined, so don't check */
      rdesc = FindType(rname);
      assert(rdesc!=NULL);
      vl = GetStatVarList(s);
      while (vl!=NULL) {
        n = NamePointer(vl);
        if (NameCompound(n)==0) { /* only care if local, nonsubatomic */
          d = FindRHSType(n,clist,&rval,&subsopen,&origin);
          if (d==NULL ||
              MoreRefined(d,rdesc)==NULL ||
              subsopen > 0) {
            if (d!=NULL && subsopen>0) {
              FPRINTF(ASCERR,
                "%sRefinement can only be done on array elements.\n",
                StatioLabel(3));
            }
            ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
			FPRINTF(ASCERR,"Incompatible type (%s) of LHS name: ",
              (d!=NULL)?SCP(GetName(d)):"UNDEFINED");
            WriteName(ASCERR,n);
            FPRINTF(ASCERR,"\n");
            error_code = DEF_ILLEGAL_REFINE;
            TypeLintError(ASCERR,s, error_code);
			error_reporter_end_flush();

            if (!g_drt_depth) {
              ClearForInfo(forinfo);
            }
            return error_code;
          }
          /* so we know d compatible. The question is can we
           * upgrade the CLE of local n or not?
           * yes if scalar or if all elements of array are
           * upgraded together.
           */
          if (d!=rdesc) {
            test.strptr = NameIdPtr(n);
            pos = gl_search(clist,&test,(CmpFunc)CmpChildListEntries);
            assert(pos!=0L);
            clep = (struct ChildListEntry *)gl_fetch(clist,pos);
            if (SimpleNameIdPtr(n) != NULL ||
                AllElementsNamed(clep,n,clist,forinfo,pstats)==1) {
              clep->typeptr = MoreRefined(d,rdesc);
            }
          }
        }
        vl = NextVariableNode(vl);
      }
      break;
    case FOR:
      g_drt_depth++;
      AddForInfo(forinfo,ForStatIndex(s),ForStatExpr(s));
      error_code = DeriveRefinedTypes(ForStatStmts(s),clist,forinfo,pstats);
      RemoveForInfo(forinfo,ForStatIndex(s));
      g_drt_depth--;
      if (error_code != DEF_OKAY) {
        if (!g_drt_depth) {
          ClearForInfo(forinfo);
        }
        return error_code;
      }
      break;
    case AA:
      /* if we were clever, do something here using LCLE info */
      break;
    case ATS:
      /* if we were clever, do something here using LCLE info */
      break;
    case SELECT:
    case COND:
      /* if we were clever, do something here using LCLE info maybe */
      break;
    default:
      break;
    }
  }

  if (!g_drt_depth) {
    ClearForInfo(forinfo);
  }
  return DEF_OKAY;
}
/*****************************************************************\
  End of functions to help refine the types of children
  necessitated by ARE_ALIKE IS_REFINED_TO ARE_THE_SAME.
\*****************************************************************/

/*** stuff for defining parameterized models and models in general ***/

/* if any name in the set given is not defined in lcl,
 * returns 0, OTHERWISE 1.
 * Checks base type of name, which must symbol/integer/set constants
 */
static
int SetNamesInLCL(CONST struct Set *sn)
{
  struct gl_list_t *nl;
  struct gl_list_t *lclgl;
  CONST struct TypeDescription *rtype;
  CONST struct Name *n;
  enum e_findrhs rval;
  unsigned long c,len;
  int subsopen; /* must never come back anything but zero */
  unsigned int origin; /* ignored */

  assert(sn!=NULL);

  nl = SetNameList(sn);
  lclgl = CopyLCLToGL(); /* we want a peek at the lcl in progress */
  len = gl_length(nl);
  for (c = 1; c <= len; c++) {
    n = (CONST struct Name *)gl_fetch(nl,c);
    /* check forvars here first in future. tempvars would be tricky,
     * except SetNameList doesn't return tempvars because
     * EvaluateNamesNeeded doesn't report those (we hope).
     */
    /* not in forvars, so check declarations */
    rtype = FindRHSType(n,lclgl,&rval,&subsopen,&origin);
    if (rtype==NULL || rval != FRC_ok  /* can't compute on arrays */||
        (GetBaseType(rtype) != integer_constant_type &&
         GetBaseType(rtype) != symbol_constant_type &&
         GetBaseType(rtype) != set_type)
       ) {
      gl_destroy(lclgl);
      gl_destroy(nl);
      return 0;
    }
  }
  gl_destroy(lclgl);
  gl_destroy(nl);
  return 1;
}

/*
 * checks that lhs of :=  (declarative) are variables.
 * checks that lhs of :== are constants and not of parametric origin.
 * checks that rhs of :== are constants.
 * checks that come up totally missing are morphed to defokay because
 * of the chance that refinement is biting us.
 */
static
enum typelinterr VerifyDefsAsgns(symchar *name,
                                 CONST struct StatementList *stats,
                                 struct gl_list_t *lclgl,
                                 struct gl_list_t *ft)
{
  register struct gl_list_t *statements;
  register unsigned long c,len;
  register unsigned long nc,nlen;
  CONST struct TypeDescription *rtype;
  struct Statement *stat;
  CONST struct Name *nptr;
  struct gl_list_t *nl=NULL;
  enum e_findrhs rval;
  int subsopen;
  unsigned int origin;
  enum typelinterr error_code=DEF_OKAY;

  statements = GetList(stats);
  len = gl_length(statements);
  for (c = 1; c <= len; c++) {
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case ASGN:
      nptr = DefaultStatVar(stat);
      rtype = FindRHSType(nptr,lclgl,&rval,&subsopen,&origin);
      if ( rtype == NULL ) {
        if (rval != FRC_attrname) {
          char *iostring;
          error_code = DEF_ASGN_INCORRECT;
          iostring = (char *)ascmalloc(6+SCLEN(name));
          sprintf(iostring,"In %s:\n",SCP(name));
          TypeLintErrorAuxillary(ASCERR,iostring,DEF_MISC_WARNING,TRUE);
          ascfree(iostring);
          TypeLintError(ASCERR,stat, error_code);
          error_code = DEF_OKAY;
        } /* else assignment to subatomic part. style bitch. */
        break;
      }
      if (rval != FRC_ok /* must be scalar */ ||
          BaseTypeIsAtomic(rtype) == 0 /* must be variable */ ||
          BaseTypeIsSet(rtype) != 0
         ) {
        error_code = DEF_ASGN_INCORRECT;
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      /* check rhs expr */
      nl = EvaluateNamesNeeded(DefaultStatRHS(stat),NULL,nl);
      nlen = gl_length(nl);
      for (nc=1;nc<=nlen;nc++) {
        nptr = (struct Name *)gl_fetch(nl,nc);
        if (NameInForTable(ft,nptr)) {
          continue;
        }
        rtype = FindRHSType(nptr,lclgl,&rval,&subsopen,&origin);
        if (rtype==NULL) {
          if (rval != FRC_attrname) {
            char *iostring;
            TLNM(ASCERR,nptr,"Unverifiable name in RHS: ",2);
            error_code = DEF_NAME_MISSING;
            iostring = (char *)ascmalloc(6+SCLEN(name));
            sprintf(iostring,"In %s:\n",SCP(name));
            TypeLintErrorAuxillary(ASCERR,iostring,DEF_MISC_WARNING,TRUE);
            TypeLintError(ASCERR,stat, error_code);
            ascfree(iostring);
            error_code = DEF_OKAY;
            /* here it would be nice if we could punt, but refinement
             * rules that out since the name might be valid and we not know.
             */
          }
          continue;
        }
        if ( rval != FRC_ok /* arrays not evaluatable */ ||
             (BaseTypeIsAtomic(rtype) == 0 && BaseTypeIsConstant(rtype)==0)
           ) {
          TLNM(ASCERR,nptr,"Improper non-scalar in RHS: ",3);
          gl_destroy(nl);
          error_code = DEF_ILLEGAL_ASGN;
          TypeLintError(ASCERR,stat, error_code);
          return error_code;
        }
      }
      gl_destroy(nl);
      nl = NULL;
      break;
    case CASGN:
      nptr = AssignStatVar(stat);
      rtype = FindRHSType(nptr,lclgl,&rval,&subsopen,&origin);
      if (rtype == NULL) {
        char *iostring;
        error_code = DEF_CASGN_INCORRECT;
        iostring = (char *)ascmalloc(6+SCLEN(name));
        sprintf(iostring,"In %s:\n",SCP(name));
        TypeLintErrorAuxillary(ASCERR,iostring,DEF_MISC_WARNING,TRUE);
        TypeLintError(ASCERR,stat, error_code);
        ascfree(iostring);
        error_code = DEF_OKAY;
        origin = origin_ISA;
        /* this will never be reached for a parameter list object. this safe */
      } else {
        if (rval != FRC_ok /* must be scalar */ ||
          (BaseTypeIsConstant(rtype) ==0 && BaseTypeIsSet(rtype)==0)
         ) {
          error_code = DEF_CASGN_INCORRECT;
          TypeLintError(ASCERR,stat, error_code);
          return error_code;
        }
      }
      if (ParametricOrigin(origin)) {
        error_code = DEF_PARAM_MODIFIED;
        TLNNE(ASCERR,nptr,"Parameter modified ");
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      nl = EvaluateNamesNeeded(AssignStatRHS(stat),NULL,nl);
      nlen = gl_length(nl);
      for (nc=1;nc<=nlen;nc++) {
        nptr = (struct Name *)gl_fetch(nl,nc);
        if (NameInForTable(ft,nptr)) {
          continue;
        }
        rtype = FindRHSType(nptr,lclgl,&rval,&subsopen,&origin);
        if (rtype==NULL) {
          char *iostring;
          TLNM(ASCERR,nptr,"Unverifiable name in :== RHS: ",2);
          error_code = DEF_NAME_MISSING;
          iostring = (char *)ascmalloc(6+SCLEN(name));
          sprintf(iostring,"In %s:\n",SCP(name));
          TypeLintErrorAuxillary(ASCERR,iostring,DEF_MISC_WARNING,TRUE);
          ascfree(iostring);
          TypeLintError(ASCERR,stat, error_code);
          error_code = DEF_OKAY;
          /* here it would be nice if we could punt, but refinement
           * rules that out since the name might be valid and we not know.
           */
        } else {
          if ( rval != FRC_ok /* arrays not evaluatable */ ||
               (BaseTypeIsSet(rtype) == 0 && BaseTypeIsConstant(rtype)==0)
             ) {
            TLNM(ASCERR,nptr,"Improper non-constant in RHS: ",3);
            gl_destroy(nl);
            error_code = DEF_ILLEGAL_CASGN;
            TypeLintError(ASCERR,stat, error_code);
            return error_code;
          }
        }
      }
      gl_destroy(nl);
      nl = NULL;
      break;
    case FOR:
      AddLoopVariable(ft,CreateForVar(ForStatIndex(stat)));
      error_code = VerifyDefsAsgns(name,ForStatStmts(stat),lclgl,ft);
      RemoveForVariable(ft);
      if (error_code != DEF_OKAY){
        return error_code;
      }
      break;
    case COND:
      error_code = VerifyDefsAsgns(name,CondStatList(stat),lclgl,ft);
      if (error_code != DEF_OKAY){
        return error_code;
      }
      break;
    case SELECT:
      /* statements inside SELECT are analyzed as part of the flat
         list of statements */
      break;
    default: /* LREL REL, ASGN, RUN, IF, EXT, REF ISA WHEN too. */
      break;
    }
  }
  return DEF_OKAY;
}

/*
 * Insures that all IS_REFINED_TO/ARE_ALIKE/ARE_THE_SAME do
 * not change anything passed in. Fascistically.
 * Complains about errors found, so caller needn't.
 * Also ought to in general check that all lhs/varlist entries exist,
 * but doesn't -- only checks shallowly.
 *
 * Should as a side effect upgrade base types of children where
 * this is determinable (ie in the case of arrays, must be over all children).
 *
 * Also checks that ATS statements do not merge illegal types.
 */
static
enum typelinterr VerifyRefinementLegal(CONST struct StatementList *stats,
                                       struct gl_list_t *lclgl)
{
  register unsigned long c,len,pos;
  register struct gl_list_t *statements;
  CONST struct VariableList *vl;
  struct Statement *stat;
  CONST struct Name *nptr;
  struct ChildListEntry *clep;
  CONST struct TypeDescription *aatype, *atstype;
  enum typelinterr error_code=DEF_OKAY;
  enum e_findrhs rval;
  unsigned int origin;
  int subsopen;
  struct ChildListEntry test;

  statements = GetList(stats);
  len = gl_length(statements);
  for (c = 1; c <= len; c++) {
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case ATS:
    case AA:
    case IRT:
      vl = GetStatVarList(stat);
      while (vl != NULL) {
        /* shallow check that parameters are not being modified */
        nptr = NamePointer(vl);
        test.strptr = NameIdPtr(nptr);
        /* find local root of name */
        pos = gl_search(lclgl,&test,(CmpFunc)CmpChildListEntries);
        /* the preceding gl_search relies on the fact that the comparison
         * element in the ChildListEntry is the symchar.
         * It will break if things are different.
         */
        if (pos != 0) {
          clep = (struct ChildListEntry *)gl_fetch(lclgl,pos);
          if (ParametricOrigin(clep->origin)) {
            error_code = DEF_PARAM_MODIFIED;
            TLNNE(ASCERR,nptr,"Parameter modified");
            TypeLintError(ASCERR,stat, error_code);
            return error_code;
          }
          if (StatementType(stat)==ATS) {
            /* here we find explicit relation merges
             * and disallow them since we do insist that
             * relations have 1 parent. Merging rels when merging models
             * is obviously allowed.
             * We also trap merging arrays of relations since
             * arrays are supposed to be only a naming convention
             * but allows multiple parents and messes up tree properties.
             */
            atstype = FindRHSType(nptr,lclgl,&rval,&subsopen,&origin);
            if (atstype == NULL) {
              TLNM(ASCERR,nptr,"Suspicious or bad name: ",2);
              if (TLINT_WARNING) {
                WriteStatementErrorMessage(ASCERR,stat,
                    "Unknown or unmergable part",0,2);
              }
              /* do not OTHERWISE return error because part may actually exist
               * once some refinement we've missed occurs.
               */
            } else {
              /* Also disallow ARE_THE_SAME of relations/whens. */
              assert(GetBaseType(atstype)!=array_type);
              switch(GetBaseType(atstype)) {
              case relation_type:
              case logrel_type:
              case when_type: /* odd case indeed */
                TLNM(ASCERR,nptr,"Part with unmergable type: ",3);
                TypeLintError(ASCERR,stat,DEF_ILLEGAL_ATS);
                return DEF_ILLEGAL_ATS;
              default:
                break;
              }
            }
          }
          if (StatementType(stat)==AA) {
            /* here we find parametric arguments to AA
             * and disallow them since parametric ARE_ALIKE makes
             * no sense.
             * Also trap illegal ARE_ALIKE on fundamental types
             * and warn about them.
             * Maybe should check relations too.
             */
            aatype = FindRHSType(nptr,lclgl,&rval,&subsopen,&origin);
            if (aatype == NULL) {
              TLNM(ASCERR,nptr,"Suspicious or bad name: ",2);
              TypeLintError(ASCERR,stat, DEF_ILLEGAL_AA);
              if (rval == FRC_attrname) {
                return DEF_ILLEGAL_AA;
              }
              /* do not OTHERWISE return error because part may actually exist
               * once some refinement we've missed occurs.
               */
            } else {
              /* disallow parameterized models declared however deep */
              if (GetBaseType(aatype) == model_type &&
                  StatementListLength(GetModelParameterList(aatype))!=0L) {
                TLNM(ASCERR,nptr,"Parameterized part: ",3);
                TypeLintError(ASCERR,stat, DEF_ILLPARAM_AA);
                return DEF_ILLPARAM_AA;
              }
              /* Also disallow ARE_ALIKE of arrays. */
              if (subsopen != 0) {
                TLNM(ASCERR,nptr,"Array elements:",3);
                TypeLintError(ASCERR,stat, DEF_ILLARRAY_AA);
                return DEF_ILLARRAY_AA;
              }
              /* Also disallow ARE_ALIKE of relations/whens. */
              switch(GetBaseType(aatype)) {
              case relation_type:
              case logrel_type:
              case when_type: /* odd case indeed */
                TLNM(ASCERR,nptr,"Part with illegal type: ",3);
                TypeLintError(ASCERR,stat,DEF_ILLEGAL_AA);
                return DEF_ILLEGAL_AA;
              default:
                break;
              }
              /* Disallow refinements by ARE_ALIKE of down-in parts
               * that got down-in by being passed there.
               */
              nptr = NextIdName(nptr);
              /* Found the next . in the name. Parts declared
               * at this scope that are not parametric we can
               * ARE_ALIKE if we so chose since anarchy is required.
               * pos is the local child root of the name and
               * we don't want to check it again, but we need
               * its type to verify things.
               */
              if (nptr != NULL) {
                rval = AANameIdHasParameterizedPart(&nptr,clep->typeptr);
                switch (rval) {
                case FRC_attrname:
                  TLNM(ASCERR, NamePointer(vl),
                    "Incorrect subatomic name in ",3);
                  if (nptr!=NULL) {
                    TypeLintNameNode(ASCERR , nptr ,"starting at ");
                  }
                  TypeLintError(ASCERR,stat,DEF_ILLEGAL_AA);
                  return DEF_ILLEGAL_AA;
                case FRC_array:
                  TLNM(ASCERR, NamePointer(vl),
                    "Incorrect array name in ",3);
                  if (nptr!=NULL) {
                    TypeLintNameNode(ASCERR , nptr ,"starting at ");
                  }
                  TypeLintError(ASCERR,stat,DEF_ILLARRAY_AA);
                  return DEF_ILLARRAY_AA;
                case FRC_badname:
                  if (nptr!=NULL) {
                    TLNNE(ASCERR, nptr,"Impossible part");
                    TypeLintName(ASCERR, NamePointer(vl),
                      "means you can't ARE_ALIKE ");
                    TypeLintError(ASCERR,stat,DEF_ILLEGAL_AA);
                    return DEF_ILLEGAL_AA;
                  } else {
                    TLNM(ASCERR , NamePointer(vl) ,"Bad name ",3);
                    TypeLintError(ASCERR,stat,DEF_ILLEGAL_AA);
                    return DEF_ILLEGAL_AA;
                  }
                case FRC_fail:
                  if (nptr!=NULL) {
                    TLNNE(ASCERR, nptr, "Parametric part");
                    TypeLintName(ASCERR, NamePointer(vl),
                      "means you can't ARE_ALIKE ");
                    TypeLintError(ASCERR,stat,DEF_ILLPARAM_AA);
                    return DEF_ILLPARAM_AA;
                  } else {
                    TLNM(ASCERR , NamePointer(vl) ,"Bad name ",3);
                    TypeLintError(ASCERR,stat,DEF_ILLPARAM_AA);
                    return DEF_ILLPARAM_AA;
                  }
                case FRC_ok:
                  break;
                default:
                  TLNM(ASCERR,NamePointer(vl),"AA Trouble digesting: ",3);
                  return DEF_ILLEGAL_AA;
                }
              }
            }
          }
        } else {
          error_code = DEF_NAME_MISSING;
          TLNNE(ASCERR,nptr,"Undefined name");
          TypeLintError(ASCERR,stat, error_code);
          return error_code;
        }
        vl = NextVariableNode(vl);
      }
      break;
    case FOR:
      if (ForContainsAlike(stat) ||
          ForContainsAts(stat) ||
          ForContainsIrt(stat) ) {
        error_code = VerifyRefinementLegal(ForStatStmts(stat),lclgl);
        if (error_code != DEF_OKAY){
          return error_code;
        }
      }
      break;
    case SELECT:
      /*
       * statements inside SELECT are analyzed as part of the flat
       * list of statements
       */
      break;
    case COND:
      break;
    default: /* LREL REL, ASGN, RUN, IF, EXT, REF ISA WHEN too. */
      break;
    }
  }
  return DEF_OKAY;
}

/*
 * checks that all names used in an expression are traceable
 * to appropriate set/var/constant/array origins.
 * In particular, since FIndRHSType disallows FRC_attr,
 * this function won't let through ATOM children.
 */
static
enum typelinterr VerifyValueNames(CONST struct Expr *ex,
                                  struct gl_list_t *lclgl,
                                  struct gl_list_t *ft)
{
  struct gl_list_t *nl=NULL;
  CONST struct TypeDescription *rtype;
  CONST struct Name *n;
  unsigned long c,len;
  enum e_findrhs rval;
  int errcnt=0;
  int subsopen=0; /* we don't care */
  unsigned int origin; /* ignored */

  assert(ex!=NULL);

  nl = EvaluateNamesNeeded(ex,NULL,nl);
  /* create, possibly empty, list of all variables including
   * variables in set expressions needed to interpret an expression.
   */
  assert(nl!=NULL);
  len = gl_length(nl);
  for (c = 1; c <= len; c++) {
    n = (CONST struct Name *)gl_fetch(nl,c);
    /* check forvars here first. tempvars would be tricky,
     * except EvaluateNamesNeeded doesn't report those (we hope).
     */
    if (NameInForTable(ft,n)) {
      continue; /* skip to next name */
    }
    /* not in forvars, so check declarations */
    rtype = FindRHSType(n,lclgl,&rval,&subsopen,&origin);
    if (rtype==NULL) {
      TLNM(ASCERR,n,"Undefined argument name ",3);
      errcnt++;
    } else {
      if ( (rval != FRC_ok && rval != FRC_array) ||
           (BaseTypeIsConstant(rtype) == 0 && BaseTypeIsSet(rtype) == 0)
         ) {
        TLNM(ASCERR,n,"Incorrect non-constant/non-set type: ",3);
        errcnt++;
      }
    }
    MarkIfPassedArgs(n,lclgl);
    /* name was ok */
  }
  gl_destroy(nl);
  if (errcnt > 0) {
    return DEF_ILLEGAL; /* pretty darn silly user */
  }
  return DEF_OKAY;
}

/* match WILL_BE to names, isas to exprs , verifying base types. */
/*
 * error_code = VerifyTypeArgs(alist,d,lclgl,ft);
 * Checks that the arg list given matches the expected args of the
 * type d given, as much as can be correctly checked.
 * Arg count is assumed correct, as that was checked at
 * TypeDefIllegal time.
 * This function could be tighter, but we need better control of
 * sets at parse time to do it, which is not practical.
 */
static
enum typelinterr VerifyTypeArgs(CONST struct Set *alist,
                                CONST struct TypeDescription *d,
                                struct gl_list_t *lclgl,
                                struct gl_list_t *ft)
{
  CONST struct StatementList *psl;
  CONST struct Statement *stat;
  CONST struct TypeDescription *atype; /* type argument expected */
  CONST struct TypeDescription *ptype; /* type parameter received */
  CONST struct VariableList *vl;
  CONST struct Set *sn;
  CONST struct Name *n;
  CONST struct Expr *ex;
  symchar *atn;
  unsigned long c,len;
  enum e_findrhs rval;
  unsigned int origin;
  int subsopen,argc,subsneed;
  int arrayerr;

  psl = GetModelParameterList(d);
  len = StatementListLength(psl);
  sn = alist;
  argc = 1;
  for (c=1;c <= len; c++) {
    stat = GetStatement(psl,c);
    switch(StatementType(stat)) {
    case WILLBE:
      vl = GetStatVarList(stat);
      atn = GetStatType(stat);
      atype = FindType(atn);
      assert(atype!=NULL);
      /* will need set type special case handling, or else assume
       * integer/symbolness correct and check at instantiate time.
       */
      while (vl != NULL) {
        /* dig up type of parameter user wants to pass */
        if (SetType(sn)!=0) {
          FPRINTF(ASCERR,
            "%sRange found where instance argument expected\n  Argument %d: ",
            StatioLabel(3),argc);
          WriteSetNode(ASCERR,sn);
          FPRINTF(ASCERR,"\n");
          return DEF_ARGS_INCORRECT;
        }
        ex = GetSingleExpr(sn);
        if (ExprType(ex)!=e_var) {
          FPRINTF(ASCERR,
            "%sIncorrect expression where instance expected\n  Argument %d: ",
            StatioLabel(3),argc);
          WriteSetNode(ASCERR,sn);
          FPRINTF(ASCERR,"\n");
          return DEF_ARGS_INCORRECT;
        }
        n = ExprName(ex);
        if (NameInForTable(ft,n)) {
          FPRINTF(ASCERR,
            "%s Loop index used where instance expected\n  Argument %d: ",
            StatioLabel(3),argc);
          WriteSetNode(ASCERR,sn);
          FPRINTF(ASCERR,"\n");
          return DEF_ARGS_INCORRECT;
        }
        ptype = FindRHSType(n,lclgl,&rval,&subsopen,&origin);
        if (ptype == NULL || (rval != FRC_ok && rval != FRC_array) ) {
          ERROR_REPORTER_START_HERE(ASC_USER_ERROR);
          FPRINTF(ASCERR,"Undefined name where instance expected. Argument %d: ",argc);
          WriteSetNode(ASCERR,sn);
          error_reporter_end_flush();
          return DEF_ARGS_INCORRECT;
        }
        /* dig up set type details to match from MODEL arg list WILL_BE,
         * or (as here) be an optimist until instantiate about sets.
         * Only insist on compatibility and not type >= WILL_BE because
         * the type might be <= and refined elsewhere to the correct sort.
         */
        if (ptype!=atype && MoreRefined(ptype,atype)==NULL) {
          FPRINTF(ASCERR,
            "%sType incompatible %s instance passed where %s expected\n",
            StatioLabel(3),SCP(GetName(ptype)),SCP(GetName(atype)));
          FPRINTF(ASCERR,"  Argument %d: ",argc);
          WriteSetNode(ASCERR,sn);
          FPRINTF(ASCERR,"\n");
          return DEF_ARGS_INCORRECT;
        }
        /* check the set madness */
        subsneed = NameLength(NamePointer(vl)) -1; /* subs expected */
        arrayerr=0;
        if (rval != FRC_ok) {
          /* passing an array */
          if (subsneed != subsopen) {
            arrayerr=1;
          }
        } else {
          /* passing a scalar */
          if (subsneed != 0) {
            arrayerr=1;
          }
        }
        if(arrayerr){
		  ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
          FPRINTF(ASCERR,
            "Instance of incorrect arrayness passed. Argument %d: "
            ,argc);
          WriteSetNode(ASCERR,sn);
          FPRINTF(ASCERR,"\n");
          FPRINTF(ASCERR,
                  "Expected name of a %d-dimensional object.",subsneed);
		  error_reporter_end_flush();
          return DEF_ARGS_INCORRECT;
        }
        MarkIfPassedArgs(n,lclgl);
        /* checked out ok as much as possible. */
        argc++;
        sn = NextSet(sn);
        vl = NextVariableNode(vl);
      }
      break;
    case ISA:
      if (SetType(sn)!=0) {
        FPRINTF(ASCERR,
           "%sIncorrect range value passed for\n  Argument %d: ",
           StatioLabel(3),argc);
        WriteExpr(ASCERR,GetLowerExpr(sn));
        FPRINTF(ASCERR,"..");
        WriteExpr(ASCERR,GetUpperExpr(sn));
        FPRINTF(ASCERR,"\n");
        FPRINTF(ASCERR,"  Set values should be enclosed in []s.\n");
        return DEF_ARGS_INCORRECT;
      } else {
        if (VerifyValueNames(GetSingleExpr(sn),lclgl,ft)!=DEF_OKAY) {
          FPRINTF(ASCERR,
            "%sIncorrect value expression passed for\n  Argument %d: ",
            StatioLabel(3),argc);
          WriteExpr(ASCERR,GetSingleExpr(sn));
          FPRINTF(ASCERR,"\n");
          return DEF_ARGS_INCORRECT;
        }
      }
      sn = NextSet(sn);
      argc++;
      break;
    case FOR:
      /* future messiness */
      break;
    default:
      ASC_PANIC("NOTREACHED. filtered types in typedefillegal fcns");
      break;
    }
  }
  return DEF_OKAY;
}

/* Needs to check that the expression passed in each
 * position is either object of correct type (WILL_BE)
 * or expression/set expression of appropriate type.
 * Checks IS_A IS_REFINED_TO statements in the list given.
 * Checks are somewhat loose, because not all correct statements
 * can be proven correct at parse time. (damn arrays and sets to hell)
 */
static
enum typelinterr VerifyISAARGS(CONST struct StatementList *stats,
                               struct gl_list_t *lclgl,
                               struct gl_list_t *ft)
{
  register unsigned long c,len;
  struct Statement *stat;
  CONST struct Set *alist;
  CONST struct TypeDescription *d;
  enum typelinterr error_code=DEF_OKAY;

  len = StatementListLength(stats);
  for (c = 1; c <= len; c++) {
    stat = GetStatement(stats,c);
    switch(StatementType(stat)){
    case IRT:
    case ISA:
      alist = GetStatTypeArgs(stat);
      if (alist != NULL) {
        d = FindType(GetStatType(stat));
        assert(d!=NULL);
        error_code = VerifyTypeArgs(alist,d,lclgl,ft);
        if (error_code != DEF_OKAY) {
          TypeLintError(ASCERR,stat,error_code);
          return error_code;
        }
      }
      break;
    case FOR:
      AddLoopVariable(ft,CreateForVar(ForStatIndex(stat)));
      error_code = VerifyISAARGS(ForStatStmts(stat),lclgl,ft);
      RemoveForVariable(ft);
      if (error_code != DEF_OKAY){
        return error_code;
      }
      break;
    case COND:
      error_code = VerifyISAARGS(CondStatList(stat),lclgl,ft);
      if (error_code != DEF_OKAY){
        return error_code;
      }
      break;
    case SELECT:
      /* Analyze tha flat list of statements instead */
      break;
    default: /* LREL REL, ASGN, RUN, IF, EXT, REF ISA WHEN too. */
      break;
    }
  }
  return DEF_OKAY;
}

/*
 * checks that all for loop indices are legal (not shadowing)
 * and that all index sets smell ok.
 */
static
enum typelinterr VerifyForVars(CONST struct StatementList *stats,
                               struct gl_list_t *lclgl,
                               struct gl_list_t *ft,
                               symchar *name)
{
  register unsigned long c,len,nc,nlen;
  struct Statement *stat;
  symchar *fvname;
  unsigned long pos;
  enum typelinterr error_code=DEF_OKAY;
  struct ChildListEntry test;
  /* */
  CONST struct TypeDescription *rtype;
  struct Name *nptr;
  struct gl_list_t *nl=NULL;
  enum e_findrhs rval;
  int subsopen=0; /* we don't care */
  unsigned int origin; /* ignored */
  char *msg=NULL;

  len = StatementListLength(stats);
  for (c = 1; c <= len; c++) {
    stat = GetStatement(stats,c);
    switch(StatementType(stat)){
    case FOR:
      fvname = ForStatIndex(stat);
      if (FindForVar(ft,fvname)!=NULL) {
        msg = (char *)ascmalloc(SCLEN(fvname)+1+80);
        sprintf(msg,"%sIndex %s shadows outer index",
                StatioLabel(3),SCP(fvname));
        STATEMENT_ERROR(stat,msg);
        ascfree(msg);
        return DEF_FOR_SHADOW;
      }
      test.strptr = fvname;
      pos = gl_search(lclgl,&test,(CmpFunc)CmpChildListEntries);
      if (pos!=0) {
        msg = (char *)ascmalloc(SCLEN(fvname)+1+80);
        sprintf(msg,"%sIndex %s shadows instance.",StatioLabel(3),
                SCP(fvname));
        STATEMENT_ERROR(stat,msg);
        ascfree(msg);
        return DEF_FOR_SHADOW;
      }

      /* check set expr */
      nl = EvaluateNamesNeeded(ForStatExpr(stat),NULL,nl);
      nlen = gl_length(nl);
      for (nc=1;nc<=nlen;nc++) {
        nptr = (struct Name *)gl_fetch(nl,nc);
        if (NameInForTable(ft,nptr)) {
          continue;
        }
        rtype = FindRHSType(nptr,lclgl,&rval,&subsopen,&origin);
        if (rtype==NULL) {
          if (rval != FRC_attrname) {
            char *iostring;
            TLNM(ASCERR,nptr,"Unverifiable name in FOR index set: ",2);
            error_code = DEF_NAME_MISSING;
            iostring = (char *)ascmalloc(6+SCLEN(name));
            sprintf(iostring,"In %s:\n",SCP(name));
            TypeLintErrorAuxillary(ASCERR,iostring,DEF_MISC_STYLE,TRUE);
            ascfree(iostring);
            TypeLintError(ASCERR,stat, error_code);
            error_code = DEF_OKAY;
            /* here it would be nice if we could punt, but refinement
             * rules that out since the name might be valid and we not know.
             */
          }
          continue;
        }
        if ( rval != FRC_ok /* arrays not evaluatable */ ||
             (BaseTypeIsAtomic(rtype) == 0 && BaseTypeIsConstant(rtype)==0)
           ) {
          TLNM(ASCERR,nptr,"Improper non-scalar in FOR index set: ",3);
          gl_destroy(nl);
          error_code = DEF_ILLEGAL_FORSET;
          TypeLintError(ASCERR,stat, error_code);
          return error_code;
        }
      }
      gl_destroy(nl);
      nl = NULL;
      /* end of checking expression */

      AddLoopVariable(ft,CreateForVar(fvname));
      error_code = VerifyForVars(ForStatStmts(stat),lclgl,ft,name);
      RemoveForVariable(ft);
      if (error_code != DEF_OKAY) {
        return error_code;
      }
      break;
    case COND:
      error_code = VerifyForVars(CondStatList(stat),lclgl,ft,name);
      if (error_code != DEF_OKAY) {
        return error_code;
      }
      break;
    case SELECT:
      /* analyze the flat list of statements instead */
      break;
    default:
      break;
    }
  }
  return error_code;
}
/*
 * checks that all names used in an expression are traceable
 * to appropriate set/var/constant origins.
 * In particular, since FIndRHSType disallows FRC_attr,
 * this function won't let through ATOM children.
 */
static
enum typelinterr VerifyScalarNames(CONST struct Expr *ex,
                                   struct gl_list_t *lclgl,
                                   struct gl_list_t *ft)
{
  struct gl_list_t *nl=NULL;
  CONST struct TypeDescription *rtype;
  CONST struct Name *n;
  unsigned long c,len;
  enum e_findrhs rval;
  int errcnt=0;
  int subsopen=0; /* we don't care */
  unsigned int origin; /* ignored */

  assert(ex!=NULL);

  nl = EvaluateNamesNeeded(ex,NULL,nl);
  /* create, possibly empty, list of all variables including
   * variables in set expressions needed to interpret an expression.
   */
  assert(nl!=NULL);
  len = gl_length(nl);
  for (c = 1; c <= len; c++) {
    n = (CONST struct Name *)gl_fetch(nl,c);
    /* check forvars here first. tempvars would be tricky,
     * except EvaluateNamesNeeded doesn't report those (we hope).
     */
    if (NameInForTable(ft,n)) {
      continue; /* skip to next name */
    }
    /* not in forvars, so check declarations */
    rtype = FindRHSType(n,lclgl,&rval,&subsopen,&origin);
    if (rtype==NULL) {
      TLNM(ASCERR,n,"Undefined WITH_VALUE variable name ",3);
      errcnt++;
    } else {
      if (rval != FRC_ok  /* can't compute on arrays */ ||
           (BaseTypeIsConstant(rtype) == 0 &&
            BaseTypeIsAtomic(rtype) == 0
           )
         ) {
        TLNM(ASCERR,n,"Incorrect non-scalar/non-set type: ",3);
        errcnt++;
      }
    }
    /* name was ok */
  }
  gl_destroy(nl);
  if (errcnt > 0) {
    return DEF_ILLEGAL; /* pretty darn silly user */
  }
  return DEF_OKAY;
}

/*
 * checks that all names used in an expression are traceable
 * to set/scalar ATOM/constant origins.
 * In particular, since FIndRHSType disallows FRC_attr,
 * this function won't let through ATOM children.
 * This function could be more specialized (picky)
 * when it comes to logical vs real relations.
 * disallows integer-based variables, symbol vars.
 */
static
enum typelinterr VerifyArithmeticNames(symchar *name,
                                       CONST struct Expr *ex,
                                       struct gl_list_t *lclgl,
                                       struct gl_list_t *ft)
{
  struct gl_list_t *nl=NULL;
  CONST struct TypeDescription *rtype;
  CONST struct Name *n;
  unsigned long c,len;
  enum e_findrhs rval;
  int errcnt=0;
  unsigned int origin; /* ignored */
  int subsopen=0; /* we don't care */
  char *iostring;

  assert(ex!=NULL);

  nl = EvaluateNamesNeeded(ex,NULL,nl);
  /* create, possibly empty, list of all variables including
   * variables in set expressions needed to interpret an expression.
   */
  assert(nl!=NULL);
  len = gl_length(nl);
  for (c = 1; c <= len; c++) {
    n = (const struct Name *)gl_fetch(nl,c);
    /* check forvars here first. tempvars would be tricky,
     * except EvaluateNamesNeeded doesn't report those (we hope).
     */
    if (NameInForTable(ft,n)) {
      continue;
    }
    /* not in forvars, so check declarations */
    rtype = FindRHSType(n,lclgl,&rval,&subsopen,&origin);
    if (rtype==NULL) {
      iostring = ASC_NEW_ARRAY(char,200);
      sprintf(iostring,"In model '%s', undefined variable named ",SCP(name));
      /* TypeLintErrorAuxillary(ASCERR,iostring,DEF_MISC_WARNING,TRUE); */
      TLNM(ASCERR,n,iostring,2);
      ascfree(iostring);
      errcnt++;
    } else {
      if (rval != FRC_ok  /* can't compute on arrays */ ||
           (BaseTypeIsConstant(rtype) == 0 &&
            BaseTypeIsSet(rtype) == 0 &&
            GetBaseType(rtype) != real_type &&
            GetBaseType(rtype) != boolean_type
           )
         ) {
        TLNM(ASCERR,n,"Incorrect non-algebraic type:",3);
        errcnt++;
      }
    }
    /* name was ok */
  }
  gl_destroy(nl);
  if (errcnt > 0) {
    return DEF_ILLEGAL_INREL;
  }
  return DEF_OKAY;
}

static
enum typelinterr VerifyLogRel(symchar *name,
                              CONST struct Statement *stat,
                              struct gl_list_t *lclgl,
                              struct gl_list_t *ft)
{
  enum typelinterr rval;
  if (stat==NULL ||
      StatementType(stat) != LOGREL ||
      LogicalRelStatExpr(stat) == NULL) {
    return DEF_ILLEGAL_INREL;
  }
  rval = VerifyArithmeticNames(name,LogicalRelStatExpr(stat),lclgl,ft);
  return rval;
}

static
enum typelinterr VerifyRelation(symchar *name,
                                CONST struct Statement *stat,
                                struct gl_list_t *lclgl,
                                struct gl_list_t *ft)
{
  enum typelinterr rval;
  if (stat==NULL ||
      StatementType(stat) != REL ||
      RelationStatExpr(stat) == NULL) {
    return DEF_ILLEGAL_INREL;
  }
  rval = VerifyArithmeticNames(name,RelationStatExpr(stat),lclgl,ft);
  return rval;
}

/* checks all the names in stats list rels/logrels, etc
 * for whether the expression arguments exist.
 * Returns something other than DEF_OKAY if insanity detected.
 * ignore select, ext statements, which should not be the case.
 * We should also be verifying when vars.
 * Actually, always returns defokay because the bizarre ways of
 * refinement (ARE_ALIKE,ARE_THE_SAME) in ascend make provability
 * of error difficult.
 */
static
enum typelinterr VerifyRelationNames(symchar * name,
                                     CONST struct StatementList *stats,
                                     struct gl_list_t *lclgl,
                                     struct gl_list_t *ft)
{
  register struct gl_list_t *statements;
  register unsigned long c,len;
  register struct Statement *stat;
  enum typelinterr error_code;
  statements = GetList(stats);
  len = gl_length(statements);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case REL:
      error_code = VerifyRelation(name,stat,lclgl,ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        /* return error_code; no, whine but continue */
      }
      break;
    case LOGREL:
      error_code = VerifyLogRel(name,stat,lclgl,ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        /* return error_code; whine but continue */
      }
      break;
    case EXT:
      /* this needs a much more sophisticated check */
      break;
    case SELECT:
      /*
       * Now all of the statements inside
       * SELECT are contained in the main statement list, which is
       * flat. So, this code is not required anymore.
       *
       * error_code = ProcessSelectCases(NULL,SelectStatCases(stat),ft);
       * if (error_code != DEF_OKAY) {
       * TypeLintError(ASCERR,stat, error_code);
       * return error_code;
       * }
       */
      break;
    case FOR:
      AddLoopVariable(ft,CreateForVar(ForStatIndex(stat)));
      error_code = VerifyRelationNames(name,ForStatStmts(stat),lclgl,ft);
      RemoveForVariable(ft);
      if (error_code != DEF_OKAY) {
        return error_code;
      }
      break;
    case COND:
      error_code=VerifyRelationNames(name,CondStatList(stat),lclgl,ft);
      if (error_code != DEF_OKAY) {
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    default: /* ISA, IRT, ATS, AA, ASGN, WHEN, RUN, IF, REF, CASGN*/
      break;
    }
  }
  return DEF_OKAY;
}
/*
 * MACROS and functions for messing with rles defined at top.
 */
#define SRLE(e) ((struct RedListEntry *)(e))
#define CREATERLE SRLE(ascmalloc(sizeof(struct RedListEntry)))
#define DESTROYRLE(r) ascfree(r)
/* we should probably run a little lifo recycle of rles */

/* compares the leading ids of n1,n2. assumes they
 * are ids and may not return OTHERWISE.
 */
static
int ShallowCompareNames(CONST struct Name *n1, CONST struct Name *n2)
{
  assert(n1!=NULL && n2!=NULL);
  assert(NameId(n1) && NameId(n2));
  return CmpSymchar(NameIdPtr(n1),NameIdPtr(n2));
}

/* not adequate for glsort. uses only CompareName to exact match.
 * remember that WILL_BE statements don't record a name in rle,
 * so this will never match a WILL_BE (hence its name.)
 * Needs to match foo['a'] to foo[set] if set contains 'a' once
 * set is assigned. maybe.
 * compares names from the rles shallowly, by id only, not
 * subscripts.
 */
static
int CmpISA_RLE(struct RedListEntry *rle1, struct RedListEntry *rle2)
{
  if (rle1==NULL || rle1->name == NULL || NameId(rle1->name)==0) return 1;
  if (rle2==NULL || rle2->name == NULL || NameId(rle2->name)==0) return -1;
  return ShallowCompareNames(rle1->name,rle2->name);
}

/* not adequate for glsort, but ok for linear search.
 * compares statements by pointer.
 */
static
int CmpRLEStat(struct RedListEntry *rle1, struct RedListEntry *rle2)
{
  if (rle1==NULL) return 1;
  if (rle2==NULL) return -1;
  if (rle1->olddeclstat!=rle2->olddeclstat) return 1;
  return 0;
}

/* return the matching rle from nspace if n is found there in an IS_A
 * WILL_BEs not checked.
 * The name is matched only shallowly, i.e. by leading id, ignoring
 * subscripts.
 */
static
struct RedListEntry *FindRLEName(struct gl_list_t *nspace,CONST struct Name *n)
{
  unsigned long pos;
  struct RedListEntry test;

  test.name = n;
  pos = gl_search(nspace,&test,(CmpFunc)CmpISA_RLE);
  if (pos == 0L) return NULL;
  return SRLE(gl_fetch(nspace,pos));
}

/* not adequate for glsort, but ok for linear search. return 0
 * if the name in rle matches name or WILL_BEs in other rle.
 * at least one rle must have a name.
 * Only the leading parts of names are compared from the rles.
 * subscripts are ignored.
 * There are potentially multiple RLEs per WILL_BE statement
 * if the WILL_BE statement has multiple vars.
 */
static
int CmpRLEVar(struct RedListEntry *rle1, struct RedListEntry *rle2)
{
  CONST struct Name *n, *wbn;

  if (rle1==NULL || rle2 == NULL) return 1;
  if (rle2->name==NULL && rle1->name == NULL) return 1;
  if (rle2->name==rle1->name) return 0; /* very weird case */
  if (rle2->name!=NULL && rle1->name != NULL) {
    /* IS_A rle */
    return ShallowCompareNames(rle1->name,rle2->name);
  }
  /* WILL_BE rle */
  if (rle1->name == NULL) {
    wbn = rle1->wbname;
    n = rle2->name;
  } else {
    wbn = rle2->wbname;
    n = rle1->name;
  }
  return ShallowCompareNames(n,wbn);
}
/* here we have a FindRLEVar that would search rle names or
 * if WILL_BE the varlist of the WILL_BE statement.
 * It only matches the leading id, not the array subscripts.
 */
static
struct RedListEntry *FindRLEVar(struct gl_list_t *nspace,CONST struct Name *n)
{
  unsigned long pos;
  struct RedListEntry test;

  test.name = n;
  pos = gl_search(nspace,&test,(CmpFunc)CmpRLEVar);
  if (pos == 0L) return NULL;
  return SRLE(gl_fetch(nspace,pos));
}

/* add an rle to the nspace given deriving the rle from statement s.
 * assumes IS_A statements have 1 rhs, while WILL_BE may have several.
 * As currently used on old, checked statements, should always return defokay
 */
static
enum typelinterr AddRLE(struct gl_list_t *nspace, struct Statement *s)
{
  struct RedListEntry *rle;
  CONST struct VariableList *vl;
  CONST struct Expr *checkval;
  enum typelinterr rval = DEF_OKAY;

  /* remember to append pointer before leaving function, but after
   * searching for the name so that we don't leak memory.
   */
  if (StatementType(s) == ISA) {
    vl = GetStatVarList(s);
    /* in typelint we ensured that vl will be length 1 */
    rle = CREATERLE;
    if (rle==NULL) {
      ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Out of memory error");
      return DEF_ILLEGAL;
    }
    rle->assigned = 0;
    rle->olddeclstat = s;
    rle->name = NamePointer(vl);
    rle->wbname = NULL;
    if (FindRLEVar(nspace,rle->name) != NULL) {
      rval = DEF_NAME_DUPLICATE;
    }
    gl_append_ptr(nspace,(VOIDPTR)rle);
  } else {
    assert(StatementType(s)==WILLBE);
    vl = GetStatVarList(s);
    checkval = GetStatCheckValue(s);
    /* WILL_BE creates as many rle as there are lhs names. */
    while (vl != NULL) {
      rle = CREATERLE;
      if (rle==NULL) {
        ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Out of memory error");
        return DEF_ILLEGAL;
      }
      rle->assigned = -2; /* or more to the point, we don't care. */
      rle->olddeclstat = s;
      rle->name = NULL;
      rle->wbname = NamePointer(vl);
      if (FindRLEVar(nspace,NamePointer(vl)) != NULL) {
        /* append only after searching. shouldn't be
         * possible to get here, I don't think.
         */
        gl_append_ptr(nspace,(VOIDPTR)rle);
        rval = DEF_NAME_DUPLICATE;
        break;
      } else {
        gl_append_ptr(nspace,(VOIDPTR)rle);
      }
      vl = NextVariableNode(vl);
    }
  }
  return rval;
}

/* destroys gllist and its rle contents. */
static
void DestroyNameSpace(struct gl_list_t *ns) {
  gl_free_and_destroy(ns);
}

/*
 * END RLE basic functions
 */

/*
 * this function verifies that all subscripts are apparently
 * computable from the items currently found in LCL and the
 * type library.
 * normally returns DEF_OKAY.
 * DOES NOT try the computation of subscripts, just
 * checks that the namespace is ok.
 * param names are always scalar or array, not . qualified.
 * This is enforced by typelint and assumed here.
 * Could check for existence of qualified name parts, but doesn't.
 * nptr should be a complete name starting with a nonsubscript.
 */
static
enum typelinterr CheckParameterSubscripts( CONST struct Name *nptr)
{
  CONST struct Name *n;
  CONST struct Set *s;
  n = NextName(nptr); /* move to 1st subscript */
  while (n != NULL) {
    /* n will be a set expression sort of name */
    s = NameSetPtr(n);
    if (SetNamesInLCL(s)==0) {
      ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
      FPRINTF(ASCERR,"Undefined subscript: [");
      WriteSet(ASCERR,s);
      FPRINTF(ASCERR,"]\n");
	  error_reporter_end_flush();
      return DEF_NAME_MISSING;
    }
    n = NextName(n);
  }
  return DEF_OKAY;
}

/*
 * this function verifies that WITH_VALUE expression of the
 * statement given is
 * computable from the items currently found in LCL and the
 * type library.
 * normally returns DEF_OKAY.
 * param names are always scalar or array, not . qualified.
 * does not return unless statement is a WILL_BE.
 */
static
enum typelinterr CheckWITHVALUE( CONST struct Statement *stat)
{
  enum typelinterr rval= DEF_OKAY;
  struct gl_list_t *lclgl;

  if (StatementType(stat)!=WILLBE) {
    ASC_PANIC("StatementType(stat)!=WILLBE");
  }
  if (GetStatCheckValue(stat)!=NULL) {
    lclgl = CopyLCLToGL();
    rval = VerifyScalarNames(GetStatCheckValue(stat),lclgl,NULL);
    gl_destroy(lclgl);
  }
  return rval;
}

/*
 * Checks the first element of a name for being in the child list.
 * If not in child list, checks/adds it. returns DEF_OKAY.
 * If in child list, returns DEF_NAME_DUPLICATE.
 * Name must be an id.
 * Checks includes if name is name of an array.
 * If an array, checks that the subscripts are computable
 * as expressions in names already added to the list
 * unless checksubs is 0.
 * The natural result of the way this function is coded
 * is that array objects can only be headered AFTER the
 * variables that define the subscripts.
 * Calculates the number of subscripts in the name.
 */
#define NOCHECKSUBS 0
#define CHECKSUBS 1
static
enum typelinterr DoParamName(CONST struct Name *nptr,
                             CONST struct TypeDescription *type,
        		     CONST struct Statement *stat,
                             int checksubs)
{
  register symchar *name;
  int isarray =0;
  int ok;
  if (NameId(nptr)){
    name = NameIdPtr(nptr);
    isarray = NameLength(nptr) - 1;
    ok = AddLCL( name,type,isarray, stat, STATPARAMETRIC );
    if (ok < 1) {
      if (ok==0) {
        ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
        FPRINTF(ASCERR,"Parameter \"%s\" redeclared.",SCP(name));
        assert(g_lcl_pivot!=NULL);
        if (g_lcl_pivot->e.statement != stat ) {
          STATEMENT_ERROR(g_lcl_pivot->e.statement,"  First seen:");
        }
		error_reporter_end_flush();

        return DEF_NAME_DUPLICATE;
      } else {
        ERROR_REPORTER_NOLINE(ASC_PROG_FATAL,"Insufficient memory during parameter parse.");
        return DEF_ILLEGAL; /* well, having insufficient memory is illegal */
      }
    }
    if (isarray && checksubs == CHECKSUBS) {
      if (CheckParameterSubscripts(nptr) != DEF_OKAY) {
        ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Array parameter '%s' uses undefined "
			"subscript. Subscript must be defined by constant leading parameters."
			,SCP(name)
		);
        return DEF_NAME_INCORRECT;
      }
    }
  } else{
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Bad name structure found in parameter list.");
    return DEF_NAME_INCORRECT;
  }
  return DEF_OKAY;
}

/* digest a parameter list in order given. See DoParamNAme
 * for details.
 */
static
enum typelinterr DoParamVarList(CONST struct VariableList *vlist,
                           CONST struct TypeDescription *type,
                           CONST struct Statement *stat,
                           int checksubs)
{
  register CONST struct Name *nptr;
  enum typelinterr error_code;

  while(vlist!=NULL){
    nptr = NamePointer(vlist);
    error_code = DoParamName(nptr,type,stat,checksubs);
    if (error_code != DEF_OKAY) return error_code;
    vlist = NextVariableNode(vlist);
  }
  return DEF_OKAY;
}

/* ParametricChildList(name,sl,checksubs) digests a 'correct'
 * parameter list and adds the children defined therein to LCL.
 * If children repeat or refer outside previously added children
 * (as in array subscripts) we know they are bogus children.
 * We check for bogus subscripts if you call this with CHECKSUBS,
 * which you should always do unless the statements have been
 * already checked, as in the case of absorbed IS_A that passed
 * when checked as part of psl.
 * Tolerates NULL input. checks that array dimensions are const
 * expressions if CHECKSUBS.
 * In the event of error return, empties LCL before returning.
 */
static
enum typelinterr ParametricChildList(symchar *name,
                                     struct StatementList *slist,
                                     int checksubs)
{
  register struct gl_list_t *statements;
  register unsigned long c,len;
  enum typelinterr error_code;
  struct Statement *stat;

  (void) name; /* may need it later */
  len = StatementListLength(slist);
  if (len==0L) return DEF_OKAY;
  statements = GetList(slist);
  for (c=1;c<=len;c++) {
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case WILLBE:
      /* here we should have a check of withvalue clauses
       * for any names being defined in leading parameters.
       */
      error_code = CheckWITHVALUE(stat);
      if (error_code != DEF_OKAY) {
        ClearLCL();
        FPRINTF(ASCERR,
          "%sWITH_VALUE must be computable from leading parameters.\n",
          StatioLabel(3));
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      /* fall through */
    case ISA:
      error_code = DoParamVarList(GetStatVarList(stat),
                     FindType(GetStatType(stat)),stat,checksubs);
      if (error_code != DEF_OKAY) {
        ClearLCL();
        TypeLintError(ASCERR,stat, error_code);
        return error_code;
      }
      break;
    case CASGN:
      /* these are checked by ReduceModelParameters etc */
      break;
    default:
      return DEF_STAT_MISLOCATED;
    }
  }
  return DEF_OKAY;
}

/* this needs to verify the lhs of WILL_BE_THE_SAME and WILL_BE's
 * and to verify the rhs of WILL_BE's for existence if such is
 * not already enforced by the parser.
 * htis function whines.
 */
static
enum typelinterr CheckParameterWheres(symchar *name,
                                      struct StatementList *slist,
                                      struct gl_list_t *lclgl)
{
  register struct gl_list_t *statements;
  register unsigned long c,len;
  struct Statement *stat;
  CONST struct VariableList *vl;
  CONST struct TypeDescription *rtype;
  CONST struct Name *n;
  enum e_findrhs rval;
  int errcnt;
  enum typelinterr error_code;
  unsigned int origin; /* ignored */
  int subsopen=0; /* we don't care */

  len = StatementListLength(slist);
  if (len==0L) return DEF_OKAY;
  statements = GetList(slist);
  for (c=1;c<=len;c++) {
    stat = (struct Statement *)gl_fetch(statements,c);
    switch(StatementType(stat)){
    case FOR:
      /* check for args */
      /* check for body */
      error_code = CheckParameterWheres(name,ForStatStmts(stat),lclgl);
      if (error_code != DEF_OKAY){
        return error_code;
      }
      break;
    case WBTS:
    case WNBTS:
      /* check for existence of all lhs parts and warn if
       * they do not exist. All lhs names must start in
       * the WILL_BE parameters.
       */
      vl = GetStatVarList(stat);
      errcnt = 0;
      while (vl!=NULL) {
        n = NamePointer(vl);
        rtype = FindRHSType(n,lclgl,&rval,&subsopen,&origin);
        if (rtype==NULL) {
          char *iostring;
          iostring = (char *)ascmalloc(6+SCLEN(name));
          sprintf(iostring,"In %s:\n",SCP(name));
          TypeLintErrorAuxillary(ASCERR,iostring,DEF_MISC_ERROR,TRUE);
          ascfree(iostring);
          TLNM(ASCERR,n,"Undefined or unmergeable part name ",3);
          errcnt++;
        }
        vl = NextVariableNode(vl);
      }
      if (errcnt !=0) {
        TypeLintError(ASCERR,stat,DEF_ILLEGAL_WBTS);
        return DEF_ILLEGAL_WBTS;
      }
      break;
    case WILLBE:
      /* there are only 2 cases.
       * e.g. MODEL test(a WILL_BE foo) WHERE (a.c WILL_BE bar);
       * Case 1)
       * type foo does not have a part c, but the
       * expected use of test is that it will have a refinement
       * of foo passed in for a. In this case it is obvious that
       * we cannot check anything except the existence of a (the
       * first part of the lhs a.c.)
       * IMHO, this makes MODEL test very difficult to use, on the
       * other hand it provides a check on part a.c which is otherwise
       * not restricted in anyway.
       * Case 2)
       * type foo has a part c, which may or may not have been refined
       * up to bar in the definition of foo. The best we can do is
       * check that a.c exists and its type is ancestral to the type
       * given in the WILL_BE.
       */
      TypeLintError(ASCERR,stat,DEF_UNIMPLEMENTED_WB);
      return DEF_UNIMPLEMENTED_WB;
    case LOGREL:
    case REL:
      /* bug fix me baa. need to check arguments exist in relations. */
      break;
    default:
      TypeLintError(ASCERR,stat,DEF_ILLEGAL);
      return DEF_ILLEGAL_PARAM;
    }
  }
  return DEF_OKAY;
}

/*
 * Takes the namespace and rle being and anything we
 * can extract from the type library to fill in the assigned
 * slot of the rle.
 * It should have been already established that this
 * rle either isn't yet in nspace, or correctly
 * belongs there without duplicating names.
 * Fills in rle assigned as best possible.
 * Do not call on same rle twice.
 * Assumes the type in the IS_A is set or constant type.
 * As currently used on old, checked statements, should always return defokay
 */
static
enum typelinterr DeriveAssignedFromISA(struct gl_list_t *nspace,
                                       struct RedListEntry *rle)
{
  struct TypeDescription *d;
  enum typelinterr rval=DEF_OKAY;

  (void)nspace;
  assert(rle!=NULL);

  if (SimpleNameIdPtr(rle->name)==NULL) {
    /* array */
    rle->assigned = -1;
    return rval;
  }
  d = FindType(GetStatType(rle->olddeclstat));
  /* test for cases where we are not assigned from IS_A */
  if ( GetStatSetType(rle->olddeclstat) != NULL /* set */ ||
       ConstantDefaulted(d) == 0 /* constant not defaulted */
     ) {
    rle->assigned = 0;
  } else {
    rle->assigned = 1;
  }
  return rval;
}

/*
 * Pull the isas/wbs out in a conveniently searchable form.
 * Assumes tmpl is already checked and won't contain
 * duplicates, since it came from an older, approved type.
 * *errloc filled with the location of error in tmpl
 * if not defokay.
 * If absorbed != 0, the list input is supposed to
 * be a list of absorbed statements. if == 0,
 * list is a parameter list.
 */
static
enum typelinterr ExtractRLEs(CONST struct StatementList *tmpl,
                             struct gl_list_t *nspace,
                             int absorbed, /* is this an absorbed list? */
                             unsigned long *errloc)
{
  unsigned long int len,c;
  struct Statement *s;
  struct RedListEntry *rle;
  enum typelinterr ret;

  *errloc = 0L;
  len = StatementListLength(tmpl);
  for (c=1;c <= len; c++) {
    s = GetStatement(tmpl,c);
    switch(StatementType(s)) {
    case ISA:
      ret = AddRLE(nspace,s);
      if (ret != DEF_OKAY) {
        *errloc = c;
        return ret;
      }
      /* not to mess with the abstraction boundary, we do this : */
      rle = FindRLEName(nspace,NamePointer(GetStatVarList((s))));
      /* when we know damn well nspace(gl_length(nspace)) is it, sigh. */
      assert(rle!=NULL);
      ret = DeriveAssignedFromISA(nspace,rle);
      if (ret != DEF_OKAY) {
        *errloc = c;
        return ret;
      }
      break;
    case WILLBE:
      if (absorbed != 0) {
        ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Incorrect WILL_BE found in absorbed list.");
        *errloc = c;
        return DEF_STAT_MISLOCATED;
      }
      ret = AddRLE(nspace,s);
      if (ret != DEF_OKAY) {
        *errloc = c;
        return ret;
      }
      break;
    case CASGN:
      if (absorbed == 0) {
        ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Incorrect assignment found in parameter list.");
        *errloc = c;
        return DEF_STAT_MISLOCATED;
      }
      rle = FindRLEName(nspace,AssignStatVar(s));
      if (rle == NULL) {
        *errloc = c;
        return DEF_NAME_MISSING;
        /* should never happen if tmpl is from Absorbed. */
      }
      /* this next line is incorrect around arrays/sets. */
      rle->assigned = 1;
      break;
    default: /* should be impossible due to TypedefIllegal functions */
      ERROR_REPORTER_START_NOLINE(ASC_PROG_FATAL);
      FPRINTF(ASCERR,"Incorrect statement found in %s list.\n",
              (absorbed != 0) ? "absorbed" : "parameter");
      TypeLintError(ASCERR,s,DEF_STAT_MISLOCATED);
	  error_reporter_end_flush();

      ASC_PANIC("%sIncorrect statement found in %s list.\n",
                StatioLabel(4),
                (absorbed != 0) ? "absorbed" : "parameter");
      break;
    }
  }
  return DEF_OKAY;
}

/*
 * Returns the assigned flag of the statement, if there is an
 * associated rle in nspace.
 * If the IS_A statement is not located in nspace, does not return.
 * Statements other than isa may cause a crash, in particular
 * because WILL_BE statements do not map uniquely into nspace.
 */
static
int FindAssignedStatement(struct gl_list_t *nspace, struct Statement *s)
{
  unsigned long pos;
  struct RedListEntry test;
  assert(StatementType(s)==ISA);
  test.olddeclstat = s;
  pos = gl_search(nspace,&test,(CmpFunc)CmpRLEStat);
  if (pos==0) {
	ERROR_REPORTER_START_NOLINE(ASC_PROG_FATAL);
	FPRINTF(ASCERR,"FindAssignedStatement called with unknown stat: ");
    WriteStatement(ASCERR,s,2);
	error_reporter_end_flush();

    ASC_PANIC("%sFindAssignedStatement called with unknown stat:\n",
              StatioLabel(4));
  }
  return SRLE(gl_fetch(nspace,pos))->assigned;
}

/*
 * All names on rhs expression should be either in nspace and
 * assigned or in parametric WILL_BE's of the Childlist of rdesc.
 * Basetypes of names should also be constant of appropriate type.
 * Anything inappropriate causes a return other than defokay.
 * statement should be a casgn.
 */
static
enum typelinterr CheckReductionRHS(CONST struct Statement *s,
                                   struct gl_list_t *nspace,
                                   CONST struct TypeDescription *rdesc)
{
  struct gl_list_t *nl=NULL;
  CONST struct Name *n,*nextn,*tnptr;
  CONST struct TypeDescription *d;
  enum e_findrhs rval;
  unsigned long c, len;
  struct RedListEntry *rle;
  int rlen=0;

  (void)rdesc;
  assert(s!=NULL);
  assert(nspace!=NULL);

  nl = EvaluateNamesNeeded(AssignStatRHS(s),NULL,nl);
  len =  gl_length(nl);
  for (c=1;c<=len;c++) {
    n = (struct Name *)gl_fetch(nl,c);
    /* could check FOR vars here in future. */
    rle = FindRLEVar(nspace,n);
    if (rle==NULL) {
      TLNM(ASCERR,n,"Improper undefined RHS name: ",3);
      gl_destroy(nl);
      return DEF_NAME_MISSING;
    }
    nextn = NextIdName(n);
    tnptr = NextName(n);
    rlen = NameLength(NamePointer(GetStatVarList(rle->olddeclstat))) -1;
    while (tnptr!=nextn) {
      rlen--;
      tnptr = NextName(tnptr);
    }
    if (rlen != 0) {
      TLNM(ASCERR,n,"Improper length RHS name: ",3);
      gl_destroy(nl);
      return DEF_ILLEGAL_CASGN;
    }
    if (nextn!=NULL) {
      d = FindChildTypeFromName(nextn,
            FindType(GetStatType(rle->olddeclstat)),
            &rval,
            &rlen);
      if ( d==NULL || rval != FRC_ok /* arrays not evaluatable */ ||
           (BaseTypeIsSet(d) == 0 && BaseTypeIsConstant(d)==0)
         ) {
        TLNM(ASCERR,n,"Improper RHS name: ",3);
        gl_destroy(nl);
        return DEF_ILLEGAL_CASGN;
      }
    }
  }
  gl_destroy(nl);
  return DEF_OKAY;
}
/*
 * ReduceModelParameters
 * Takes the info from rdesc parameters/args to
 * construct pslbase, tsl after accounting for rsl.
 * pslbase and tsl MUST be null on entry, might still be on exit.
 * The type rdesc must have a nonnull declarations list.
 * rsl must not be NULL/empty.
 * Unlike general MODEL bodies, the lists involved here should never
 * be very long, so we'll just use gl_lists for intermediate
 * structure.
 * If a problem is found, will return something other than
 * DEF_OKAY. pslbase and tsl should then be cleaned up by
 * the caller.
 */
static
enum typelinterr ReduceModelParameters(symchar * name,
                                       CONST struct TypeDescription *rdesc,
                                       CONST struct StatementList *rsl,
                                       struct StatementList **pslbase,
                                       struct StatementList **tsl)
{
  CONST struct StatementList *oldpsl, *tmpl;
  struct Statement *s;
  unsigned long bug, c, len;
  struct gl_list_t *nspace;
  struct RedListEntry *rle;
  enum typelinterr res;

  (void) name; /* may need it later */

  assert(*pslbase == NULL);
  assert(*tsl == NULL);
  nspace = gl_create(10L); /* DO NOT SORT THIS LIST. order matters in params */
  assert(nspace != NULL);
  oldpsl = GetModelParameterList(rdesc);
  assert(StatementListLength(oldpsl) > 0L);
  assert(StatementListLength(rsl) > 0L);

  /* get names of already absorbed parameters so we can't
   * absorb them twice.
   */
  tmpl = GetModelAbsorbedParameters(rdesc);
  if (StatementListLength(tmpl) > 0L) {
    *tsl = AppendStatementLists(tmpl,EmptyStatementList());
    res = ExtractRLEs(tmpl,nspace,1,&bug);
    if (res != DEF_OKAY) { /* should never happen */
      TypeLintError(ASCERR,GetStatement(tmpl,bug),res);
      DestroyNameSpace(nspace);
      return res;
    }
  } else {
    *tsl = EmptyStatementList();
  }
  /* get names of parameters we'll have to match or absorb.
   */
  res = ExtractRLEs(oldpsl,nspace,0,&bug);
  if (res != DEF_OKAY) { /* should never happen except for idiot users. */
    TypeLintError(ASCERR,GetStatement(oldpsl,bug),res);
    DestroyNameSpace(nspace);
    return res;
  }
  /* add new reductions and matching isas to tsl
   */
  len = StatementListLength(rsl);
  for (c=1; c <= len; c++) {
    s = GetStatement(rsl,c);

    res = CheckReductionRHS(s,nspace,rdesc);
    /* this should verify that EvaluateNamesNeeded
     * returns a happy list wrt RLE.
     */
    if (res != DEF_OKAY) {
      TypeLintError(ASCERR,s,res);
      DestroyNameSpace(nspace);
      return res;
    }
    /* at this point, only oldpsl rles should exist, so
     * we cannot reduce a parameter declared in the same
     * type since the new psl isn't known. This is intentional,
     * otherwise you could MODEL foo(n IS_A bar;) REFINES baz(n:==3;);
     * which violates type WYSIWYG and looks silly since baz
     * may not even have a part n.
     */
    rle = FindRLEName(nspace,AssignStatVar(s));
    /* not bright enough around arrays yet */
    if (rle == NULL) {
      res = DEF_VALUEPARAM_BAD;
      TypeLintError(ASCERR,s,res);
      DestroyNameSpace(nspace);
      return res;
    }
    if (rle->assigned>0) {
    /* not bright enough around arrays yet */
      res = DEF_REASSIGNED_PARAM;
      TypeLintError(ASCERR,s,res);
      DestroyNameSpace(nspace);
      return res;
    }
    if (rle->assigned==0) {
      /* this is correct, because array IS_A rles have assigned < 0. */
      rle->assigned = 1;
    }

    /* move IS_A into absorbed list, if not already there, as
     * may be the case with arrays.
     */
    if (StatementInList(*tsl,rle->olddeclstat)==0) {
      AppendStatement(*tsl,rle->olddeclstat);
    }
    AppendStatement(*tsl,s);
  }
  /* now fill up pslbase with everything in the old list
   * that wasn't just absorbed.
   */
  len = StatementListLength(oldpsl);
  *pslbase = EmptyStatementList();
  for (c=1; c <= len; c++) {
    s = GetStatement(oldpsl,c);
    switch(StatementType(s)) {
    case WILLBE:
      AppendStatement(*pslbase,s);
      break;
    case ISA:
      if (FindAssignedStatement(nspace,s)==0) {
        AppendStatement(*pslbase,s);
      }
      /* don't copy if it's been added to tsl */
      break;
    default:
      TypeLintErrorAuxillary(ASCERR,"Bizarre error in ReduceModelParameters\n",
        DEF_MISC_ERROR,TRUE);
      /* this shouldn't be possible. lint again. */
      res = DEF_ILLEGAL;
      TypeLintError(ASCERR,s,res);
      DestroyNameSpace(nspace);
      return res;
    }
  }
  DestroyNameSpace(nspace);
  return DEF_OKAY;
}

/*
 *  MMP makes sure that psl is identical to pslbase up to
 * len pslbase.
 * Allow morerefined types.
 */
static
enum typelinterr MatchModelParameters(symchar *name, /* goes w/psl */
                                      symchar *refname,
                                      struct StatementList *pslbase,
                                      struct StatementList *psl)
{
  unsigned long int oldlen, newlen,diff;
  int c;
  CONST struct Statement *stat, *firsterr= NULL;

  /* check list lengths first */
  newlen = StatementListLength(psl);
  oldlen = StatementListLength(pslbase);
  if (newlen < oldlen) {
	ERROR_REPORTER_START_NOLINE(ASC_PROG_FATAL);
    FPRINTF(ASCERR,"Parameter list mismatch with '%s': missing declarations in\n  ",
      SCP(refname));
    for (diff = 1; diff <=oldlen; diff++) {
      if (psl==NULL || gl_search(GetList(psl),
                    GetStatement(pslbase,diff),
                    (CmpFunc)CompareISStatements) == 0) {
        if (firsterr==NULL) {
          firsterr = GetStatement(pslbase,diff);
        }
        WriteStatement(ASCERR,GetStatement(pslbase,diff),2);
      }
    }
    if (firsterr!=NULL && StatementType(firsterr)==ISA) {
      FPRINTF(ASCERR,"The first missing declaration is an IS_A;"
		"it may be that you forgot some :== in the REFINES() list.");
    }
	error_reporter_end_flush();

    ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"HINT:\n"
      "Parameter definitions in a refining MODEL must match in order\n"
      "the parameters of the MODEL being refined. The type of a parameter\n"
      "in the new MODEL may be more refined than the corresponding\n"
      "parameter in the old MODEL.\n"
      "An IS_A statement in the old MODEL whose variable is assigned\n"
      "in the REFINES() clause of the new MODEL must not be repeated\n"
      "in the parameter list of the new MODEL.");
    return DEF_ILLEGAL_PARAM;
  }
  if (newlen==0) return DEF_OKAY; /* no parameters for either */
  c = CompareISLists(pslbase,psl,&diff);
  if (c != 0 && diff <= oldlen) {
	ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
    FPRINTF(ASCERR,"Parameter %lu mismatch between %s and %s.\n",diff,SCP(name),SCP(refname));
    stat = (CONST struct Statement *)gl_fetch(GetList(psl),diff);
    WriteStatement(ASCERR,stat,2);
    FPRINTF(ASCERR,"  Must match:\n");
    stat = (CONST struct Statement *)gl_fetch(GetList(pslbase),diff);
    WriteStatement(ASCERR,stat,2);
	error_reporter_end_flush();
    return DEF_ILLEGAL_PARAM;
  }
  return DEF_OKAY;
}

/*
 * MPW makes sure that wsl is identical to wslbase up to
 * len wslbase.
 * Allow morerefined types.
 */
static
enum typelinterr MatchParameterWheres(symchar *name, /* goes w/wsl */
                                      symchar *refname,
                                      struct StatementList *wslbase,
                                      struct StatementList *wsl)
{
  unsigned long int oldlen, newlen,diff;
  int c;
  CONST struct Statement *stat;

  /* check list lengths first */
  newlen = StatementListLength(wsl);
  oldlen = StatementListLength(wslbase);
  if (newlen < oldlen) {
    ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
	FPRINTF(ASCERR,"WHERE list mismatch with %s.\n",SCP(refname));
    FPRINTF(ASCERR,"  %s is missing declarations:\n",SCP(name));
    for (diff = 1; diff <=oldlen; diff++) {
      if (wsl==NULL || gl_search(GetList(wsl),
                    GetStatement(wslbase,diff),
                    (CmpFunc)CompareISStatements) == 0) {
        WriteStatement(ASCERR,GetStatement(wslbase,diff),2);
      }
    }
	error_reporter_end_flush();
    return DEF_ILLEGAL_PARAM;
  }
  if (newlen==0) return DEF_OKAY; /* no parameters for either */
  c = CompareISLists(wslbase,wsl,&diff);
  if (c != 0 && diff <= oldlen) {
    ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
    FPRINTF(ASCERR,"WHERE statement %lu mismatch between %s and %s.\n",
		diff,SCP(name),SCP(refname));
    stat = (CONST struct Statement *)gl_fetch(GetList(wsl),diff);
    WriteStatement(ASCERR,stat,2);
    FPRINTF(ASCERR,"  Must match:\n");
    stat = (CONST struct Statement *)gl_fetch(GetList(wslbase),diff);
    WriteStatement(ASCERR,stat,2);
	error_reporter_end_flush();
    return DEF_ILLEGAL_PARAM;
  }
  return DEF_OKAY;
}

/*
 * This function digests the body statements, adding them to any
 * childlist info that already exists, or creating that list.
 * It also calls all error checking routines that can only work
 * after the full namespace of the type is available, such as
 * alias and relation sanity.
 */
static
ChildListPtr FinishChildList(symchar *type, struct StatementList *stats)
{
  ChildListPtr result;
  register struct gl_list_t *childlist;
  struct gl_list_t *ilist;
  struct for_table_t *ftable;
  enum typelinterr error_code;

  if (DoIS_A(stats) !=DEF_OKAY){
    ClearLCL();
    return NULL;
  }
  ftable = CreateForTable();
  error_code = DoRelations(type,stats,ftable);
  DestroyForTable(ftable);
  if (error_code !=DEF_OKAY){
    ClearLCL();
    return NULL;
  }
  ftable = CreateForTable();
  error_code = DoWhens(type,stats,ftable);
  DestroyForTable(ftable);
  if (error_code !=DEF_OKAY){
    ClearLCL();
    return NULL;
  }
  /* move all the data to a gllist */
  childlist = CopyLCLToGL();
  /* get the IS_A type of all children */
  if (DeriveChildTypes(stats,childlist) > 0) {
    /* accept no whining! */
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* check for loop index shadows */
  ftable = CreateForTable();
  error_code = VerifyForVars(stats,childlist,ftable,type);
  DestroyForTable(ftable);
  if (error_code != DEF_OKAY) {
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* check for illegal alii */
  if (VerifyALIASES(stats,childlist) != DEF_OKAY){
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* check for parametric IS_REFINED_TO ARE_ALIKE ARE_THE_SAME args ,
   * which are not legal. And check that all lhs names of these
   * will at least Start in the childlist.
   */
  error_code = VerifyRefinementLegal(stats,childlist);
  if (error_code != DEF_OKAY) {
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* try to derive additional type information from IS_REFINED_TO ARE_ALIKE
   * and ARE_THE_SAME about objects. if a provable type incompatibility
   * is spotted, may return an error. highly unlikely.
   * In particular, should refine basetype of an array up it all its
   * elements are upgraded.
   */
  ilist = gl_create(4L);
  error_code = DeriveRefinedTypes(stats,childlist,ilist,stats);
  gl_destroy(ilist);
  if (error_code != DEF_OKAY) {
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* check for absurd IS_A args */
  ftable = CreateForTable();
  error_code = VerifyISAARGS(stats,childlist,ftable);
  DestroyForTable(ftable);
  if (error_code != DEF_OKAY) {
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* check for correct :=, :== */
  ftable = CreateForTable();
  error_code = VerifyDefsAsgns(type,stats,childlist,ftable);
  DestroyForTable(ftable);
  if (error_code != DEF_OKAY) {
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* check for sane names in relations, now that we almost know the type of
   * everything. This will still let errors in set arithmetic
   * and subscript range errors through since we can't check those
   * in the general case. This should cure a lot of typos, though.
   * This lets suspicious array names through, because refinement may
   * make them legal.
   */
  ftable = CreateForTable();
  error_code = VerifyRelationNames(type,stats,childlist,ftable);
  DestroyForTable(ftable);
  if (error_code !=DEF_OKAY){
    gl_destroy(childlist);
    ClearLCL();
    return NULL;
  }

  /* error_code = TypeLintIronUnplugged(stats,childlist);
   * if (error_code != DEF_OKAY) {
   *   assert(HouseBurningDown(childlist));
   * }
   */

  result = CreateChildList(childlist);
  gl_destroy(childlist);
  ClearLCL();
  if (result == NULL) {
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR
		,"Unable to determine a child type in '%s'."
		,SCP(type)
	);
  }
  return result;
}

/* destroys everything fed it. tolerates null input */
static
void DestroyTypeDefArgs(struct StatementList *sl,
                        struct gl_list_t *pl,
                        struct StatementList *psl,
                        struct StatementList *rsl,
                        struct StatementList *tsl,
                        struct StatementList *wsl)
{
  DestroyStatementList(sl);
  DestroyProcedureList(pl);
  DestroyStatementList(psl);
  DestroyStatementList(rsl);
  DestroyStatementList(tsl);
  DestroyStatementList(wsl);
}


/*
 * To handle the SELECT statement, we need to create a flat
 * statement list containing all the lists of statements in
 * all of the CASEs of the SELECT. Also this function has
 * to work recursively for all of the nested SELECTs
 */
static
void FlatListSelectStmts(struct StatementList *newstatl,
        		 struct Statement *stat)
{
  struct SelectList *cases;
  struct StatementList *sl;
  unsigned long c,length;
  struct Statement *s;

  cases = SelectStatCases(stat);
  while ( cases!=NULL) {
    sl = SelectStatementList(cases);
    length = StatementListLength(sl);
    for(c=1;c<=length;c++){
      s = GetStatement(sl,c);
      assert(s!=NULL);
      switch(StatementType(s)) {
        case SELECT:
          AppendStatement(newstatl,s);
          FlatListSelectStmts(newstatl,s);
          break;
        default:
          AppendStatement(newstatl,s);
          break;
      }
    }
    cases = NextSelectCase(cases);
  }
}

/* This function should have at most 2 callers: the yacc, and
 * possibly another function in this file creating a base type.
 * Here we sanity check type guts, derive additional guts, and
 * create/return a type if all checks are passed.
 * Spews to ascerr about failures in checks.
 * The basic philosophy here is check early and often, deriving
 * as much as possible from the statements because the typedef is
 * very small compared to an instance.
 */
struct TypeDescription *CreateModelTypeDef(symchar *name,
        				   symchar *refines,
        				   struct module_t *mod,
        				   int univ,
        				   struct StatementList *sl,
        				   struct gl_list_t *pl,
        				   struct StatementList *psl,
        				   struct StatementList *rsl,
        				   struct StatementList *wsl,
                                           unsigned int err)
{
  struct TypeDescription *rdesc;
  ChildListPtr clist;
  struct gl_list_t *childlist;	    /* child list gl after parameters found */
  struct StatementList *tsl = NULL; /* reduced parameters derived */
  struct StatementList *pslbase = NULL; /* parameter list psl must match */
  unsigned long len;                /* length of sl */
  unsigned long c;
  struct StatementList *newstatl;   /* flat list of statements */
  struct Statement *stat;

  /* To generate (log)relation names */
  g_number = 0;

  if(err!=0){
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR
      ,"Model definition '%s' abandoned due to syntax errors."
      ,SCP(name)
    );
    DestroyTypeDefArgs(sl,pl,psl,rsl,NULL,wsl);
    return NULL;
  }
  assert(g_lcl_head == NULL);
  assert(g_lcl_tail == NULL);
  assert(g_lcl_pivot == NULL);
  if (TypeLintIllegalParamStats(ASCERR,name,psl) != DEF_OKAY ||
      /* single constant IS_A, nonrelation WILL_BE only */
      TypeLintIllegalWhereStats(ASCERR,name,wsl) != DEF_OKAY ||
      /* structural assertions (WILL_BE_THE_SAME, WILL_BE) only */
      TypeLintIllegalReductionStats(ASCERR,name,rsl) != DEF_OKAY ||
      /* structural assignments only */
      TypeLintIllegalBodyStats(ASCERR,name,sl,context_MODEL) != DEF_OKAY ||
      /* no WILL_BE,IF,RUN statements in body */
      TypeLintIllegalMethodStats(ASCERR,name,pl,context_METH) != DEF_OKAY
      /* no structural stuff in methods -- yet */) {
    DestroyTypeDefArgs(sl,pl,psl,rsl,NULL,wsl);
    return NULL;
  } else {
    AddContext(psl,context_MODPARAM);
    /* AddContext(rsl,context_MODPARAM); reductions are declarative */
    AddContext(wsl,context_MODWHERE);
  }

  /* Making a flat list containing all of the statements in SELECTs */
  if (SlistHasWhat(sl) & contains_SELECT) {
    len = StatementListLength(sl);
    newstatl = EmptyStatementList();
    for (c=1; c<=len; c++) {
      stat = GetStatement(sl,c);
      AppendStatement(newstatl,stat);
      if (StatementType(stat) == SELECT) {
        FlatListSelectStmts(newstatl,stat);
      }
    }
    DestroyStatementList(sl);
    sl = newstatl;
  }

  if (refines==NULL) {
    rdesc = NULL;
  } else {
    rdesc=FindType(refines);
    if (rdesc==NULL) {
      ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
      FPRINTF(ASCERR,"Model '%s' attempts to refine '%s', which is not a known type.",
              SCP(name), SCP(refines));
      DestroyTypeDefArgs(sl,pl,psl,rsl,NULL,wsl);
	  error_reporter_end_flush();
      return NULL;
    }
    if (GetBaseType(rdesc) != model_type){
      ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
	  FPRINTF(ASCERR,"Model '%s' attempts to refine non-MODEL type '%s'.\n",
              SCP(name),SCP(refines));
      DestroyTypeDefArgs(sl,pl,psl,rsl,NULL,wsl);
	  error_reporter_end_flush();
      return NULL;
    }

    /* universality is inherited */
    if (GetUniversalFlag(rdesc)) univ = 1;
    /* add the new statements/procedures and those from the refined type */
    sl = AppendStatementLists(GetStatementList(rdesc),sl);
    pl = MergeProcedureLists(GetInitializationList(rdesc),pl);
    /* new procedures will have a parseid of 0 that yet needs setting. */
  }
  /* ok. eat the arg lists */
  if (rdesc == NULL) {
    /* We don't refine anything. There can be no already reduced,
     * and we disallow declaring and absorbing a var in the same
     * type.
     */
    if (ParametricChildList(name,psl,CHECKSUBS) != DEF_OKAY) {
      DestroyTypeDefArgs(sl,pl,psl,rsl,NULL,wsl);
      /* ParametricChildList will whine, so we don't whine here.
       */
      return NULL;
    }
  } else {
    /* we refine something */
    if (StatementListLength(GetModelParameterList(rdesc)) == 0L) {
      /* no parameters in rdesc. */
      if (StatementListLength(rsl) != 0L) {
        /* error. can't reduce what ain't there */
	    ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
        FPRINTF(ASCERR,"Model %s attempts to reduce non-parameters in %s.\n",
          SCP(name),SCP(refines));
        WriteStatementList(ASCERR,rsl,4);
		error_reporter_end_flush();
        DestroyTypeDefArgs(sl,pl,psl,rsl,NULL,wsl);
        return NULL;
      }
      /* rdesc could have completed IS_A/:== pairs. */
      if (StatementListLength(GetModelAbsorbedParameters(rdesc)) != 0L) {
        /* make ourselves a copy to add to without messing up the
         * existing one.
         */
        tsl = AppendStatementLists(GetModelAbsorbedParameters(rdesc),
                                   EmptyStatementList());
        if (ParametricChildList(name,tsl,NOCHECKSUBS) != DEF_OKAY) {
          /* contents of tsl have been checked already */
		  ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Model %s victim of bizarre error 1.",
				SCP(name));
          DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
        }
      }
      if (ParametricChildList(name,psl,CHECKSUBS) != DEF_OKAY ) {
        DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
        /* ParametricChildList will whine, so we don't here */
        return NULL;
      }
      /* we now have full parameter name space in LCL in no-param ancestor
       * case.
       */
    } else {
      /* the old type had a parameter list too. */
      if (StatementListLength(rsl) != 0L) {
        /* absorb isas/:== if required */
        if (ReduceModelParameters(name,rdesc,rsl,&pslbase,&tsl)!=DEF_OKAY) {
          DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
          DestroyStatementList(pslbase);
          /* ReduceParameters will whine, so we don't here */
          return NULL;
        }
      } else {
        /* just copy the old parameters and tsl */
        pslbase = AppendStatementLists(GetModelParameterList(rdesc),
                                       EmptyStatementList());
        if (GetModelAbsorbedParameters(rdesc)!=NULL) {
          tsl = AppendStatementLists(GetModelAbsorbedParameters(rdesc),
                                     EmptyStatementList());
        }
      }
      /* pslbase must now be matched by psl of the new type.
       * But first add the absorbed stuff to the child list
       * so we can't redeclare it.
       */
      if (ParametricChildList(name,tsl,NOCHECKSUBS) != DEF_OKAY) {
        /* contents of tsl, if any, have been checked already */
		ERROR_REPORTER_START_NOLINE(ASC_PROG_ERROR);
        FPRINTF(ASCERR,"Model %s victim of bizarre error 2.\n",SCP(name));
        WriteStatementList(ASCERR,tsl,2);
		error_reporter_end_flush();

        DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
        DestroyStatementList(pslbase);
        return NULL;
      }
      /* verify parameter order/matching up to length of pslbase.
       */
      if (MatchModelParameters(name,refines,pslbase,psl) != DEF_OKAY) {
        DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
        DestroyStatementList(pslbase);
        ClearLCL();
        /* CMP will have whined, so we don't here. */
        return NULL;
      }
      DestroyStatementList(pslbase); /* the new type keeps its own list */
      /* if speed becomes an issue, think about replacing the like
       * elements of the new psl with their pslbase counterparts.
       * No justification at present, however, and it's rather messy
       * to add.
       */
      pslbase = NULL;
      /* add parameter list children */
      if (ParametricChildList(name,psl,CHECKSUBS) != DEF_OKAY ||
          MatchParameterWheres(name,refines,GetModelParameterWheres(rdesc),wsl)
          != DEF_OKAY) {
        DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
        ClearLCL();
        /* tests will whine, so we don't here */
        return NULL;
      }
    }
  }

  if (univ==1 && StatementListLength(psl)!=0L) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"UNIVERSAL type %s cannot have parameters because only the first instance of the type could set them.",SCP(name));
    DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
    ClearLCL();
    return NULL;
  }
  /* check WHERE statements and warn about unnamed parts */
  childlist = CopyLCLToGL();
  if (CheckParameterWheres(name,wsl,childlist)!= DEF_OKAY) {
    ClearLCL();
    DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
    gl_destroy(childlist);
    childlist = NULL;
    return NULL;
  }
  gl_destroy(childlist);
  childlist = NULL;
  /* ok, eat regular body statements of new type */
  clist = FinishChildList(name,sl);
  if (clist != NULL) {
    return CreateModelTypeDesc(name,rdesc,mod,
                               clist,pl,sl,univ,psl,rsl,tsl,wsl);
  } else {
    /* FinishChildList will whine, so we don't here */
    DestroyTypeDefArgs(sl,pl,psl,rsl,tsl,wsl);
    return NULL;
  }
}

/*** stuff for atoms largely moved to childdef.c since that needs its
 * own flavor of evaluation and so forth.
 * Only this function here  to make use of LCL.
 */
static
ChildListPtr MakeAtomChildList(struct StatementList *sl)
{
  register struct Statement *stat;
  ChildListPtr result;
  register struct gl_list_t *list,*childlist;
  register unsigned long c,len;
  enum typelinterr error_code;
  list = GetList(sl);
  len = gl_length(list);
  for(c=1;c<=len;c++){
    stat = (struct Statement *)gl_fetch(list,c);
    switch(StatementType(stat)){
    case ASGN:
      break;
    case ISA:
      if ( BaseType(GetStatType(stat)) < NUM_FUNDTYPES) {
        /* could be more efficient by not sorting, but oh well */
        error_code =
          DoVarList(GetStatVarList(stat),FindType(GetStatType(stat)),stat);
        if (error_code != DEF_OKAY) {
          TypeLintError(ASCERR,stat, error_code);
          ClearLCL();
          return NULL;
        }
        break;
      }
      /* fall through when first if fails */
    default:
      STATEMENT_ERROR(stat, "Illegal statement in atom definition. Skipping it.");
      break;
    }
  }
  childlist = CopyLCLToGL();
  result = CreateChildList(childlist);
  gl_destroy(childlist);
  ClearLCL();
  if (result == NULL) {
    FPRINTF(ASCERR,"Unable to determine a child type in %s.\n","ATOM");
  }
  return result;
}

/* a function to check sanity and setup for call to type_desc for const */
struct TypeDescription *CreateConstantTypeDef(symchar *name,
        				      symchar *refines,
        				      struct module_t *mod,
        				      int univ,
        				      int defaulted,
        				      double rval,
        				      long ival,
        				      symchar *sval,
        				      CONST dim_type *dim,
                                              unsigned int err)
{
  struct TypeDescription *rdesc;
  enum type_kind t;

  if (err) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Constant definition '%s' abandoned due to syntax errors.",SCP(name));
    return NULL;
  }
  if (refines==NULL) {
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Constants must refine constant basetypes.");
    return NULL;
  }
  rdesc = FindType(refines);
  if (rdesc==NULL){
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to locate type '%s' for definition of type '%s'.",SCP(refines),SCP(name));
    return NULL;
  }
  t = GetBaseType(rdesc);
  if ( t != real_constant_type &&
       t != integer_constant_type &&
       t != symbol_constant_type &&
       t != boolean_constant_type) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Constant '%s' attempts to refine a non-constant type '%s'.",SCP(name),SCP(refines));
    return NULL;
  }
  if (GetUniversalFlag(rdesc)) univ=1;
  /* if new and old defaulted, error */
  if ( (defaulted) && (ConstantDefaulted(rdesc)) ) {
    /* can't default twice */
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Constant refinement %s reassigns value of %s.",SCP(name),SCP(refines));
    return NULL;
  }
  if (!defaulted && univ && g_compiler_warnings) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"(%s): Universal Constant should be assigned.",SCP(name));
  }

  switch (t) {
  case real_constant_type:
    dim = CheckDimensionsMatch(dim,GetConstantDimens(rdesc));
    if (dim==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Dimensions of constant refinement %s don't match those of %s.",SCP(name),SCP(refines));
        return NULL;
    }
    if ( ConstantDefaulted(rdesc) ) {
      defaulted = 1;
      rval = GetConstantDefReal(rdesc);
    }
    break; /* end real const */
  case integer_constant_type: /* fall through */
  case boolean_constant_type:
    if ( ConstantDefaulted(rdesc) ) {
      defaulted = 1;
      ival = GetConstantDefInteger(rdesc);
    }
    break; /* end integer,boolean const */
  case symbol_constant_type:
    if ( ConstantDefaulted(rdesc) ) {
      defaulted = 1;
      sval = GetConstantDefSymbol(rdesc);
    }
    break; /* end symbol const */
  default:
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"CreateConstantTypeDef miscalled");
    return NULL;
  }
  if (defaulted && !univ && g_compiler_warnings) {
    FPRINTF(ASCWAR,"%s(%s): Assigned Constant better if Universal.\n",
      StatioLabel(1),SCP(name));
  }
  return CreateConstantTypeDesc(name,t,rdesc,mod,CalcByteSize(t,NULL,NULL),
        			defaulted,rval,dim,ival,sval,univ);
}

struct TypeDescription *CreateAtomTypeDef(symchar *name,
        				  symchar *refines,
        				  enum type_kind t,
        				  struct module_t *mod,
        				  int univ,
        				  struct StatementList *sl,
        				  struct gl_list_t *pl,
        				  int defaulted,
        				  double val,
        				  CONST dim_type *dim,
        				  long ival,
        				  symchar *sval,
                                          unsigned int err)
{
  struct TypeDescription *rdesc;
  ChildListPtr clist;
  struct ChildDesc *childd;
  register unsigned long bytesize;

  if (err) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Atom definition \"%s\" abandoned due to syntax errors.",SCP(name));
    DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
    return NULL;
  }
  if (refines!=NULL){
    rdesc = FindType(refines);
    if (rdesc==NULL){
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to locate type %s for %s's type definition.",SCP(refines),SCP(name));
      DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
      return NULL;
    }
    if ((GetBaseType(rdesc)==model_type) ||
        (GetBaseType(rdesc)==real_constant_type) ||
        (GetBaseType(rdesc)==integer_constant_type) ||
        (GetBaseType(rdesc)==boolean_constant_type) ||
        (GetBaseType(rdesc)==symbol_constant_type) ||
        (GetBaseType(rdesc)==array_type) ||
        (GetBaseType(rdesc)==relation_type) ||
        (GetBaseType(rdesc)==logrel_type) ){
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Atom %s attempts to refine a non-ATOM type %s.",SCP(name),SCP(refines));
      DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
      return NULL;
    }
    dim = CheckDimensionsMatch(dim,GetRealDimens(rdesc));
    /* remarkably, dim check won't bother other atom types */
    if (dim==NULL){
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Dimensions of atom refinement %s don't match those of %s.",SCP(name),SCP(refines));
      DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
      return NULL;
    }
    t = GetBaseType(rdesc);
    if (GetUniversalFlag(rdesc)) univ=1;
    sl = AppendStatementLists(GetStatementList(rdesc),sl);
    pl = MergeProcedureLists(GetInitializationList(rdesc),pl);
    if ((!defaulted)&&(AtomDefaulted(rdesc))){
      defaulted = 1;
      if (t==real_type) val = GetRealDefault(rdesc);
      if (t==integer_type) ival = GetIntDefault(rdesc);
      if (t==boolean_type) ival = GetBoolDefault(rdesc);
      if (t==symbol_type) sval = GetSymDefault(rdesc);
    }
  }
  else rdesc = NULL;
  /* make clist */
  clist = MakeAtomChildList(sl); /* this thing should filter goop */
  if (clist!=NULL){
    /* make childd */
    childd = MakeChildDesc(name,sl,clist);
    if (childd!=NULL){
      /* calculate bytesize */
      bytesize = CalcByteSize(t,clist,childd);
      return CreateAtomTypeDesc(name,t,rdesc,mod,clist,pl,sl,bytesize,
        			childd,defaulted,val,dim,univ,ival,sval);
    } else {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"CreateAtomTypeDef: unable to MakeChildDesc");
      DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
      return NULL;
    }
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"CreateAtomTypeDef: unable to MakeAtomChildList");
    DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
    return NULL;
  }
}

/* if this fails we're in real trouble */
struct TypeDescription *CreateRelationTypeDef(struct module_t *mod,
                                              symchar *name,
        				      struct StatementList *sl,
        				      struct gl_list_t *pl)
{
  ChildListPtr clist;
  struct ChildDesc *childd;
  unsigned long bytesize;
  clist = MakeAtomChildList(sl);
  if (clist!=NULL){
    childd = MakeChildDesc(name,sl,clist);
    if (childd!=NULL){
      bytesize = CalcByteSize(relation_type,clist,childd);
      return CreateRelationTypeDesc(mod,clist,pl,sl,bytesize,childd);
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}


/* if this fails we're in real trouble */
struct TypeDescription *CreateLogRelTypeDef(struct module_t *mod,
                                            symchar *name,
                                            struct StatementList *sl,
                                            struct gl_list_t *pl)
{
  ChildListPtr clist;
  struct ChildDesc *childd;
  unsigned long bytesize;
  clist = MakeAtomChildList(sl);
  if (clist!=NULL){
    childd = MakeChildDesc(name,sl,clist);
    if (childd!=NULL){
      bytesize = CalcByteSize(logrel_type,clist,childd);
      return CreateLogRelTypeDesc(mod,clist,pl,sl,bytesize,childd);
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}



struct TypeDescription *CreatePatchTypeDef(symchar *patch,
        				   symchar *original,
        				   symchar *orig_mod,
        				   struct module_t *mod,
        				   struct StatementList *sl,
        				   struct gl_list_t *pl,
                                           unsigned int err)
{
  struct TypeDescription *rdesc, *result;
  (void) orig_mod;
  if (original==NULL) {
    FPRINTF(ASCERR,"Unable to locate the original type for the patch %s\n",
            SCP(patch));
    DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
    return NULL;
  }
  if (err) {
    FPRINTF(ASCERR,"Patch \"%s\" abandoned due to syntax errors\n",
            SCP(patch));
    DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
    return NULL;
  }
  rdesc = FindType(original);
  if (rdesc==NULL) {
    FPRINTF(ASCERR,
            "Unable to locate the original type, %s, for the patch %s\n",
            SCP(original),SCP(patch));
    DestroyTypeDefArgs(sl,pl,NULL,NULL,NULL,NULL);
    return NULL;
  }
  result = CreatePatchTypeDesc(patch,rdesc,mod,pl,sl);
  return result;
}

/*********************************************************************\
DefineEMType(fname,basetype);
Create a external MODEL root type.
\*********************************************************************/
static void DefineEMType(symchar *sym, enum type_kind t)
{
  struct TypeDescription *def;
  (void) t;
  def = CreateModelTypeDesc(sym,NULL,NULL,NULL,NULL,
          EmptyStatementList(),0,EmptyStatementList(),
          EmptyStatementList(), EmptyStatementList(),EmptyStatementList());
  if (def) {
    AddType(def);
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to define %s.",SCP(sym));
  }
}

/*********************************************************************\
DefineDType(fname,basetype);
Create a dummy  type.
\*********************************************************************/
static void DefineDType(symchar *sym)
{
  struct TypeDescription *def;
  def = CreateDummyTypeDesc(sym);
  if (def) {
    AddType(def);
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to define dummy type %s.",SCP(sym));
  }
}

/*********************************************************************\
DefineWhenType();
Creates the when typedef.
\*********************************************************************/
static void DefineWhenType(void)
{
  struct TypeDescription *def;
  def = CreateWhenTypeDesc(NULL,NULL,EmptyStatementList());
  if (def) {
    AddType(def);
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to define WHEN type.");
  }
}

/*********************************************************************\
DefineCType(fname,basetype);
Create a fundamental Constant type of name cname for type basetype.
\*********************************************************************/
static void DefineCType(symchar *sym, enum type_kind t)
{
  struct TypeDescription *def;
  def = CreateConstantTypeDesc(sym,t,NULL,NULL,CalcByteSize(t,NULL,NULL),
                               0,0.0,WildDimension(),0,NULL,0);
  if (def) {
    AddType(def);
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Unable to define fundamental constant %s.",
      SCP(sym));
  }
}

/*********************************************************************\
DefineFType(fname,basetype);
Create a fundamental atom type of name fname for type basetype.
\*********************************************************************/
static void DefineFType(symchar *sym, enum type_kind t)
{
  struct TypeDescription *def;
  def = CreateAtomTypeDef(sym,NULL,t,NULL,0,EmptyStatementList(),NULL,
        		  0,0.0,WildDimension(),0,NULL,0);
  if (def) {
    AddType(def);
  } else {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Unable to define fundamental atom %s.",
      SCP(sym));
  }
}

void DefineFundamentalTypes(void)
{
  /* define atomic types */
  DefineFType(GetBaseTypeName(real_type),real_type);
  DefineFType(GetBaseTypeName(integer_type),integer_type);
  DefineFType(GetBaseTypeName(symbol_type),symbol_type);
  DefineFType(GetBaseTypeName(boolean_type),boolean_type);
  DefineFType(GetBaseTypeName(set_type),set_type);
  /* define constant types */
  DefineCType(GetBaseTypeName(real_constant_type),real_constant_type);
  DefineCType(GetBaseTypeName(integer_constant_type),integer_constant_type);
  DefineCType(GetBaseTypeName(symbol_constant_type),symbol_constant_type);
  DefineCType(GetBaseTypeName(boolean_constant_type),boolean_constant_type);
  /* define when class */
  DefineWhenType();
  /* define external MODEL class */
  DefineEMType(GetBaseTypeName(model_type&patch_type),model_type);
  /* define uncompiled class */
  DefineDType(GetBaseTypeName(dummy_type));
}

