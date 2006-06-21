/*
 *  Type description structure Implementation
 *  by Tom Epperly
 *  Created: 1/12/90
 *  Version: $Revision: 1.41 $
 *  Version control file: $RCSfile: type_desc.c,v $
 *  Date last modified: $Date: 1998/05/18 16:36:48 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include <utilities/ascPanic.h>
#include <general/list.h>
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "expr_types.h"
#include "sets.h"
#include "proc.h"
#include "symtab.h"
#include "vlist.h"
#include "stattypes.h"
#include "statement.h"
#include "statio.h"
#include "slist.h"
#include "select.h"
#include "child.h"
#include "childinfo.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "module.h"
#include "library.h"
#include "watchpt.h"
#include "initialize.h"
#include "type_desc.h"
#include "type_descio.h"

#ifndef lint
static CONST char TypeDescRCSid[] = "$Id: type_desc.c,v 1.41 1998/05/18 16:36:48 ballan Exp $";
#endif

#if 0 /* a debugging version of tmalloc. change 0 to 1 to use it. */
#define TMALLOC(x) (x) = \
  (struct TypeDescription *)ascmalloc(sizeof(struct TypeDescription)); \
  ascbfill((void *)(x),sizeof(struct TypeDescription))
#else
#define TMALLOC(x) (x) = \
  (struct TypeDescription *)ascmalloc(sizeof(struct TypeDescription))
#endif

#define TYPELINKDEBUG 0
/*
 * When this is 1, generates lots of spew about type linking
 * and unlinking using d->refiners.
 */

static
long g_parse_count = 1;
/*
 * This is the total number of times a create type of any sort
 * has been called. It can be used to disambiguate type
 * queries by assigning the parseid from parse_count.
 * by initialization, 0 is never a valid parseid.
 * negative parseid indicate something destroyed.
 */

struct ArrayDescList {
  struct ArrayDescList *next;
  struct TypeDescription *desc;
};

static unsigned long g_array_desc_count = 0L;
/*
 * Count of the number of names ever made for array nodes, which normally
 * are invisible but having names is awfully convenient in some places.
 * should be reset 0 when list goes empty.
 */
static struct ArrayDescList *g_array_desc_list=NULL;
/*
 * Singly linked list of array type descriptions.
 * Descriptions are added/deleted/searched linearly all the time
 * which may be a very bad idea.
 */


static struct IndexType g_it_dummy_enum = {NULL,NULL,0};
static struct IndexType g_it_dummy_int = {NULL,NULL,1};

struct IndexType *CreateDummyIndexType(int intindex)
{
  if (intindex) {
    return &g_it_dummy_int;
  } else {
    return &g_it_dummy_enum;
  }
}

struct IndexType *CreateIndexType(struct Set *set, int int_index)
{
  char *set_str;				/* KAA_DEBUG */
  struct IndexType *result;
  result = (struct IndexType *)ascmalloc(sizeof(struct IndexType));
  result->int_index = int_index ? 1 : 0;
  set_str = CreateStrFromSet(set);
  result->set = set;			/* this will NOT go away. baa */
  result->sptr = AddSymbol(set_str);
  ascfree(set_str);
  return result;
}

struct IndexType *CreateIndexTypeFromStr(char *str, int int_index)
{
  struct IndexType *result;			/* KAA_DEBUG */
  result = (struct IndexType *)ascmalloc(sizeof(struct IndexType));
  result->int_index = int_index ? 1 : 0;
  result->sptr = AddSymbol(str);	/*  in symbol table */
  result->set = NULL;
  return result;
}


void DestroyIndexType(struct IndexType *ind)
{
  if (ind == &g_it_dummy_int || ind == &g_it_dummy_enum) return;
  AssertAllocatedMemory(ind,sizeof(struct IndexType));
  ind->sptr = NULL;		/* we own the string in symtab */
  if (ind->set) {
    DestroySetList(ind->set);
    ind->set = NULL;
  }
  ascfree(ind);
}

/*
 * This function compares two types by name and if identical complains
 * unless ptrs are also identical.
 * Does not tolerate null input very well.
 * We need a function to compare symchar properly here.
 * Breaks ties with parseid.
 */
static
int CmpLinkedTypes(struct TypeDescription *t1, struct TypeDescription *t2)
{
  int result;
  assert(t1!=NULL);
  assert(t2!=NULL);
  if (t1==t2) {
    return 0;
  }
  result = CmpSymchar(t1->name,t2->name);
  if (!result) {
    /* same name */
#if TYPELINKDEBUG
    FPRINTF(ASCERR,"Distinct refinements with same name (%s) found %ld %ld\n",
      SCP(t1->name),GetParseId(t1),GetParseId(t2));
#endif
    assert(GetParseId(t1)>0 && GetParseId(t2)>0);
    /* assumes parseid are unique! */
    return ((GetParseId(t1) > GetParseId(t2))? 1 : -1);
  }
  return result;
}
/*
 * This functions establishes the forward pointer to the new type
 * from the old type. The new type is the more refined one.
 * Whines if another refinement of the same name is found.
 */
static
void LinkTypeDesc(struct TypeDescription *old,
                  struct TypeDescription *new)
{
  if (old==NULL || new == NULL) {
    Asc_Panic(2, NULL,
              "Attempt to link bad types- old (%p) new (%p)\n"
              "Extreme error. Please notify \n\t%s\n",
              old, new, ASC_BIG_BUGMAIL);
  }
  if (old->refiners == NULL) {
    old->refiners = gl_create(2);
  }
  assert(old->refiners != NULL);
#if TYPELINKDEBUG
  if (1) {
    unsigned long int c,len;
    struct TypeDescription *desc;

    FPRINTF(ASCERR,"%s refiners:\n",GetName(old));
    len = gl_length(old->refiners);
    for (c=1; c <= len; c++) {
      desc = (struct TypeDescription *)gl_fetch(old->refiners,c);
      FPRINTF(ASCERR,"%s (%ld)\n",GetName(desc),GetParseId(desc));
    }
  }
#endif
  gl_insert_sorted(old->refiners,new,(CmpFunc)CmpLinkedTypes);
#if TYPELINKDEBUG
  if (1) {
    unsigned long int c,len;
    struct TypeDescription *desc;

    FPRINTF(ASCERR,"%s refiners:\n",GetName(old));
    len = gl_length(old->refiners);
    for (c=1; c <= len; c++) {
      desc = (struct TypeDescription *)gl_fetch(old->refiners,c);
      FPRINTF(ASCERR,"%s (%ld)\n",GetName(desc),GetParseId(desc));
    }
  }
#endif
}
/*
 * deletes the reference to new in old->refiners
 * so that new can be tossed.
 */
static
void UnLinkTypeDesc(struct TypeDescription *old,
                    struct TypeDescription *new)
{
  unsigned long int pos;
  assert(old!=NULL);
  assert(new!=NULL);
  assert(old->refiners!=NULL);
  pos = gl_search(old->refiners,new,(CmpFunc)CmpLinkedTypes);
  if (!pos) {
    FPRINTF(ASCERR,"Attempt to unlink corrupted type descriptions");
    return;
  }
#if TYPELINKDEBUG
  if (1) {
    struct TypeDescription *desc;
    desc = (struct TypeDescription *)gl_fetch(old->refiners,pos);
    FPRINTF(ASCERR,"Unlinking attempt %s (%ld)\n",new->name,GetParseId(new));
    FPRINTF(ASCERR,"Unlinking type %s (%ld)\n",desc->name,GetParseId(desc));
  }
#endif
  gl_delete(old->refiners,pos,0);
}

/* a widget to check for := in a statement list. returns the bit
 * TYPECONTAINSDEFAULTS  appropriate for defaults if so and 0 otherwise.
 * handle null input gracefully.
 */
static
unsigned short int StatListHasDefaults(struct StatementList *sl) {
  register unsigned long len,c;
  register CONST struct gl_list_t *l;

  len = StatementListLength(sl);
  if (len==0L) return 0;
  l = GetList(sl);
  for(c=1;c<=len;c++) {
    if ( StatementType((struct Statement *)gl_fetch(l,c)) == ASGN ) {
      return TYPECONTAINSDEFAULTS;
    }
  }
  return 0;
}

/* forward declaration */
static
unsigned short int ParametersInType(struct StatementList *,
                                    struct StatementList *);

/*
 * To check for () in the statements lists of a SELECT
 * statement. returns 1 for parameters if so and 0 otherwise.
 */
static
int ParametersInTypeInsideSelect(struct Statement *stat)
{
  struct SelectList *cases;
  struct StatementList *sl;

  cases = SelectStatCases(stat);

  while ( cases!=NULL ) {
    sl = SelectStatementList(cases);
    if (ParametersInType(sl,NULL) != 0) {
      return 1;
    }
    cases = NextSelectCase(cases);
  }
  return 0;
}


/* a widget to check for () in a statement list. returns the bit
 * TYPECONTAINSPARINSTS appropriate for parameters if so and 0 otherwise.
 * handle null input gracefully.
 */
static
unsigned short int ParametersInType(struct StatementList *sl,
                                    struct StatementList *psl) {
  register unsigned long len,c;
  register CONST struct gl_list_t *l;
  struct TypeDescription *d;
  struct Statement *stat;
  unsigned int forflags = (contains_ISA | contains_WILLBE | contains_IRT );

  if (StatementListLength(psl)!=0L) {
    return TYPECONTAINSPARINSTS;
  }
  len = StatementListLength(sl);
  if (len==0L) return 0;
  l = GetList(sl);
  for(c=1;c<=len;c++) {
    stat = (struct Statement *)gl_fetch(l,c);
    switch ( StatementType(stat) ) {
    case WILLBE:
    case ISA:
    case IRT:
      d = GetStatTypeDesc(stat);
      if ( d != NULL && TypeHasParameterizedInsts(d)!=0 ) {
        return TYPECONTAINSPARINSTS;
      }
      break;
    case FOR:
      if ((ForContains(stat) & forflags) != 0 ) {
        if (ParametersInType(ForStatStmts(stat),NULL) != 0) {
          return TYPECONTAINSPARINSTS;
        }
      }
      break;
    case SELECT:
        if (ParametersInTypeInsideSelect(stat) != 0) {
          return TYPECONTAINSPARINSTS;
        }
      break;
    default:
      break;
    }
  }
  return 0;
}

/* counts total lhs names in a list of IS_A's
 * and WILL_BE's. list given may be null.
 */
static
unsigned int CountParameters(CONST struct StatementList *sl)
{
  unsigned cnt=0;
  unsigned long c,len;
  CONST struct Statement *s;
  /* count the IS_A's and WILL_BE's, then add the number
   * of extra identifiers the WILL_BE's have.
   */
  len = StatementListLength(sl);
  cnt = len;
  if (cnt > 0) {
    for (c=1;c<=len;c++) {
      s = GetStatement(sl,c);
      switch(StatementType(s)) {
      case WILLBE:
        cnt += (VariableListLength(GetStatVarList(s)) - 1);
        break;
      case ISA:
      default:
        break;
      }
    }
  }
  return cnt;
}

/* returns old, the number of procedures in pl already
 * claimed by another type.
 */
static
int ClaimNewMethodsTypeDesc(long parseid, struct gl_list_t *pl)
{
  unsigned long c;
  struct InitProcedure *p;
  int old=0;
  assert(parseid != 0);
  if (pl==NULL) {
    return 0;
  }
  for (c = gl_length(pl); c > 0; c--) {
    p = (struct InitProcedure *)gl_fetch(pl,c);
    if (p != NULL && GetProcParseId(p) == 0) {
      SetProcParseId(p,parseid);
    } else {
      old++;
    }
  }
  return old;
}

struct TypeDescription
*CreateModelTypeDesc(symchar *name, 	   /* name of the type*/
		     struct TypeDescription *rdesc,/* type that it */
		                                   /* refines or NULL */
		     struct module_t *mod, /* module it is defined in */
		     ChildListPtr cl, /* list of the type's */
		                           /* child names */
		     struct gl_list_t *pl, /* list of initialization */
					   /* procedures */
		     struct StatementList *sl, /* list of declarative */
					       /* statements */
		     int univ,		/* UNIVERSAL flag */
		     struct StatementList *psl, /* list of parameter */
					       /* statements */
		     struct StatementList *rsl,  /* list of reduction */
					       /* statements */
		     struct StatementList *tsl, /* list of reduced */
					       /* statements */
		     struct StatementList *wsl  /* list of wbts statements */
                    )

{
  register struct TypeDescription *result;
  TMALLOC(result);
  result->ref_count = 1;
  result->t = model_type;
  result->name = name;
  result->refines = rdesc;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  ClaimNewMethodsTypeDesc(result->parseid,pl);
  if (rdesc!=NULL) CopyTypeDesc(rdesc);
  if (rdesc!=NULL) LinkTypeDesc(rdesc,result);
  result->mod = mod;
  result->children = cl;
  result->init = pl;
  result->stats = sl;
  result->universal = univ;
  result->flags = 0;
  result->flags |=  StatListHasDefaults(sl);
  result->flags |=  ParametersInType(sl,psl);
  result->flags |=  TYPESHOW;
  result->u.modarg.declarations = psl;
  result->u.modarg.absorbed = tsl;
  result->u.modarg.reductions = rsl;
  result->u.modarg.wheres = wsl;
/*
  result->u.modarg.argdata = NULL;
*/
  result->u.modarg.argcnt = CountParameters(psl);
  return result;
}

struct TypeDescription
*CreateDummyTypeDesc(symchar *name)
{
  register struct TypeDescription *result;
  TMALLOC(result);
  result->ref_count = 1;
  result->t = dummy_type;
  result->name = name;
  result->refines = NULL;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  result->mod = NULL;
  result->children = NULL;
  result->init = NULL;
  result->stats = EmptyStatementList();
  result->universal = 1;
  result->flags = 0;
  result->flags |=  TYPESHOW;
  return result;
}

struct TypeDescription
  *CreateConstantTypeDesc(symchar *name,	/* name of type */
		      enum type_kind t,	/* base type of atom */
		      struct TypeDescription *rdesc, /* type */
						     /* description */
						     /* what it refines */
		      struct module_t *mod, /* module where the type */
					    /* is defined */
			unsigned long bytesize, /* instance size */
		      int defaulted, /* valid for constants */
				     /* indicates default value was */
				     /* assigned */
		      double rval, /* default value for real const */
		      CONST dim_type *dim, /* dimensions of default real */
			long ival, /* default integer const */
			symchar *sval,  /* default symbol */
		      int univ)
{
  register struct TypeDescription *result;
  TMALLOC(result);
  result->t = t;
  result->ref_count = 1;
  result->name = name;
  result->refines = rdesc;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  if (rdesc!=NULL) CopyTypeDesc(rdesc);
  if (rdesc!=NULL) LinkTypeDesc(rdesc,result);
  result->mod = mod;
  result->children = NULL;
  result->init = NULL;
  result->stats = EmptyStatementList();
  result->universal = univ;
  result->flags = 0;
  result->flags |=  TYPESHOW;
  result->u.constant.byte_length = bytesize;
  result->u.constant.defaulted = (defaulted) ? 1 : 0;
  switch (t) {
  case real_constant_type:
    result->u.constant.u.defreal = rval;
    result->u.constant.dimp = dim;
    break;
  case integer_constant_type:
    result->u.constant.u.definteger = ival;
    break;
  case boolean_constant_type:
    result->u.constant.u.defboolean = (ival) ? 1 : 0;
    break;
  case symbol_constant_type:
    result->u.constant.u.defsymbol = sval;
    break;
  default: /* not reached we hope */
    Asc_Panic(2, NULL, "ERROR 666\n");
    break;
  }
  return result;
}

struct TypeDescription
  *CreateAtomTypeDesc(symchar *name,	/* name of type */
		      enum type_kind t,	/* base type of atom */
		      struct TypeDescription *rdesc, /* type */
						     /* description */
						     /* what it refines */
		      struct module_t *mod, /* module where the type */
					    /* is defined */
		      ChildListPtr childl,	/* list of children names */
		      struct gl_list_t *procl, /* list of */
					       /* initialization procedures */
		      struct StatementList *statl, /* list of */
						   /* declarative statements */
		      unsigned long int bytesize, /* size of an */
						  /* instance in bytes. */
		      struct ChildDesc *childd,	/* description of the */
						/* atom's children */
		      int defaulted, /* TRUE indicates default value was */
				     /* assigned */
		      double dval, /* default value for real atoms */
		      CONST dim_type *ddim, /* dimensions of default value */
		      int univ,
		      long ival,
		      symchar *sval
)
{
  register struct TypeDescription *result;
  TMALLOC(result);
#if TYPELINKDEBUG
  FPRINTF(ASCERR,"\n");
#endif
  result->t = t;
  result->ref_count = 1;
  result->name = name;
  result->refines = rdesc;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  ClaimNewMethodsTypeDesc(result->parseid,procl);
  if (rdesc!=NULL) CopyTypeDesc(rdesc);
  if (rdesc!=NULL) LinkTypeDesc(rdesc,result);
  result->mod = mod;
  result->children = childl;
  result->init = procl;
  result->stats = statl;
  result->universal = univ;
  result->flags = 0;
  result->flags |=  StatListHasDefaults(statl);
  result->flags |=  TYPESHOW;
  result->u.atom.byte_length = bytesize;
  result->u.atom.childinfo = childd;
  result->u.atom.defaulted = (defaulted) ? 1 : 0;
  switch(t) {
  case real_type:
    result->u.atom.u.defval = dval;
    break;
  case integer_type:
    result->u.atom.u.defint = ival;
    break;
  case boolean_type:
    result->u.atom.u.defbool = ival ? 1 : 0;
    break;
  case symbol_type:
    result->u.atom.u.defsym = sval;
    break;
  case set_type: /* don't have defaults, but are atoms... eww! */
    break;
  default:
    FPRINTF(ASCERR,"CreateAtomTypeDesc called with unexpected type for %s\n",
      SCP(name));
    break;
  }
  result->u.atom.dimp = ddim;
  return result;
}

static
int IndicesEqual(struct gl_list_t *i1, struct gl_list_t *i2)
{
  unsigned long c,len;
  struct IndexType *ind1,*ind2;
  if (gl_length(i1)!=gl_length(i2)) return 0;
  len = gl_length(i1);
  for(c=1;c<=len;c++){
    ind1 = (struct IndexType *)gl_fetch(i1,c);
    ind2 = (struct IndexType *)gl_fetch(i2,c);
    if (ind1==ind2) continue; /* matching dummies */
    if (ind1->int_index!=ind2->int_index) return 0;
/*  FOR loop ALIASES-IS_A has forced the dummies into the typedesc. */
    if (ind1->sptr == NULL || ind2->sptr==NULL) {
      /* unequal dummies or dummy vs real index are only possible NULL */
      return 0;
    }
    assert(AscFindSymbol(ind1->sptr)!=NULL);
    assert(AscFindSymbol(ind2->sptr)!=NULL);
    if (ind1->sptr != ind2->sptr) {
      return 0;
    }
  }
  return 1;
}

static
int ArrayDescsEqual(struct TypeDescription *src,
		    struct module_t *mod,
		    struct TypeDescription *desc,
		    int isintset,
		    int isrel,
                    int islogrel,
                    int iswhen,
		    struct gl_list_t *indices)
{
  if (src->mod != mod) return 0;
  if (src->u.array.desc != desc) return 0;
  if ((src->u.array.isrelation&&!isrel)||
     (isrel&&!src->u.array.isrelation)) return 0;
  if ((src->u.array.islogrel&&!islogrel)||
     (islogrel&&!src->u.array.islogrel)) return 0;
  if ((src->u.array.iswhen&&!iswhen)||
     (iswhen&&!src->u.array.iswhen)) return 0;
  if ((src->u.array.isintset&&!isintset)||(isintset&&!src->u.array.isintset))
    return 0;
  return IndicesEqual(src->u.array.indices,indices);
}

static
struct TypeDescription *FindArray(struct module_t *mod,
				  struct TypeDescription *desc,
				  int isintset,
				  int isrel,
                                  int islogrel,
                                  int iswhen,
				  struct gl_list_t *indices)
{
  register struct ArrayDescList *ptr;
  int ade;
  ptr = g_array_desc_list;
  while(ptr!=NULL){
   ade =
     ArrayDescsEqual(ptr->desc,mod,desc,isintset,
                     isrel,islogrel,iswhen,indices);
   if (ade) {
      CopyTypeDesc(ptr->desc);
      return ptr->desc;
    }
    ptr = ptr->next;
  }
  return NULL;
}

static
void AddArray(struct TypeDescription *d)
{
  register struct ArrayDescList *ptr;
  ptr = g_array_desc_list;
  g_array_desc_list =
    (struct ArrayDescList *)ascmalloc(sizeof(struct ArrayDescList));
  g_array_desc_list->next = ptr;
  g_array_desc_list->desc = d;
}

static
void RemoveArrayTypeDesc(struct TypeDescription *d)
{
  register struct ArrayDescList *ptr , *next;
  ptr = g_array_desc_list;
  if (ptr!=NULL){
    if (ptr->desc == d){
      g_array_desc_list = ptr->next;
      ascfree(ptr);
    } else {
      while(NULL != (next = ptr->next)){
	if (next->desc == d){
	  ptr->next = next->next;
	  ascfree(next);
	  return;
	} else {
          ptr = next;
        }
      }
    }
  }
  if (g_array_desc_list==NULL) {
    g_array_desc_count = 0L; /* reset the count if all gone */
  }
}

struct TypeDescription *CreateArrayTypeDesc(struct module_t *mod,
					    struct TypeDescription *desc,
					    int isint,
					    int isrel,
                                            int islogrel,
                                            int iswhen,
					    struct gl_list_t *indices)
{
  register struct TypeDescription *result;
#if MAKEARRAYNAMES
  char name[64];
#endif
  if ((result =FindArray(mod,desc,isint,isrel,islogrel,iswhen,indices))==NULL){
    TMALLOC(result);
    result->t = array_type;
#if MAKEARRAYNAMES
    sprintf(name,"array_%lu",g_array_desc_count++);
    result->name = AddSymbol(name);
#else
    result->name = NULL;
#endif
    result->refines = NULL;
    result->refiners = NULL;
    result->parseid = g_parse_count++;
    result->mod = mod;
    result->children = NULL;
    result->init = NULL;
    result->stats = NULL;
    result->universal = 0;
    result->flags = 0;
    result->flags |=  TYPESHOW;
    result->ref_count = 1;
    result->u.array.indices = indices;
    result->u.array.isintset = isint;
    result->u.array.isrelation = isrel;
    result->u.array.islogrel = islogrel;
    result->u.array.iswhen = iswhen;
    if (desc) CopyTypeDesc(desc);
    result->u.array.desc = desc;
    AddArray(result);
  }
  else{				/* array type already exists */
    if (indices){
      gl_iterate(indices,(void (*)(VOIDPTR))DestroyIndexType);
      gl_destroy(indices);
    }
  }
  return result;
}

struct TypeDescription *CreateRelationTypeDesc(struct module_t *mod,
					       ChildListPtr clist,
					       struct gl_list_t *plist,
					       struct StatementList *statl,
					       unsigned long int bytesize,
					       struct ChildDesc *childd)
{
  struct TypeDescription *result;
  TMALLOC(result);
  result->t = relation_type;
  result->ref_count = 1;
  result->name = GetBaseTypeName(relation_type);
  result->refines = NULL;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  result->mod = mod;
  result->children = clist;
  result->init = plist;
  result->stats = statl;
  result->universal = 0;
  result->flags = 0;
  result->flags |=  StatListHasDefaults(statl);
  result->flags |=  TYPESHOW;
  result->u.atom.byte_length = bytesize;
  result->u.atom.childinfo = childd;
  result->u.atom.defaulted = 0;
  result->u.atom.defval = 0.0;
  result->u.atom.dimp = NULL;
  return result;
}


struct TypeDescription *CreateLogRelTypeDesc(struct module_t *mod,
					       ChildListPtr clist,
					       struct gl_list_t *plist,
					       struct StatementList *statl,
					       unsigned long int bytesize,
					       struct ChildDesc *childd)
{
  struct TypeDescription *result;
  TMALLOC(result);
  result->t = logrel_type;
  result->ref_count = 1;
  result->name = GetBaseTypeName(logrel_type);
  result->refines = NULL;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  result->mod = mod;
  result->children = clist;
  result->init = plist;
  result->stats = statl;
  result->universal = 0;
  result->flags = 0;
  result->flags |=  StatListHasDefaults(statl);
  result->flags |=  TYPESHOW;
  result->u.atom.byte_length = bytesize;
  result->u.atom.childinfo = childd;
  result->u.atom.defaulted = 0;
  result->u.atom.defval = 0.0;
  result->u.atom.dimp = NULL;
  result->u.atom.u.defbool = 0;
  return result;
}


struct TypeDescription
*CreateWhenTypeDesc(struct module_t *mod,    /* module it is defined in */
		    struct gl_list_t *plist,
		    struct StatementList *statl)
{
  struct TypeDescription *result;
  TMALLOC(result);
  result->ref_count = 1;
  result->t = when_type;
  result->name = GetBaseTypeName(when_type);
  result->refines = NULL;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  result->mod = mod;
  result->children = NULL;
  result->init = plist;
  result->stats = statl;
  result->universal = 0;
  result->flags = 0;
  result->flags |=  TYPESHOW;
  return result;
}


struct TypeDescription
*CreatePatchTypeDesc(symchar *name, 	   	   /* name of the type*/
		     struct TypeDescription *rdesc,/* type that it patches*/
		     struct module_t *mod, 	/* module patch was defined */
		     struct gl_list_t *pl,	/* procedures */
		     struct StatementList *sl) 	/* declarative */
{
  register struct TypeDescription *result;
  assert(rdesc!=NULL);			/* mandatory */
  TMALLOC(result);
  result->ref_count = 1;
  result->t = patch_type;
  result->name = name;
  result->refines = rdesc;
  result->refiners = NULL;
  result->parseid = g_parse_count++;
  if (rdesc!=NULL) CopyTypeDesc(rdesc);
  result->mod = mod;
  result->children = NULL;
  result->init = pl;
  result->stats = sl;
  result->flags = 0;
  result->flags |=  StatListHasDefaults(sl);
  result->flags |=  TYPESHOW;
  result->universal = 0;
  return result;
}


struct TypeDescription *MoreRefined(CONST struct TypeDescription *desc1,
				    CONST struct TypeDescription *desc2)
{
  register CONST struct TypeDescription *ptr1,*ptr2;
  AssertAllocatedMemory(desc1,sizeof(struct TypeDescription));
  AssertAllocatedMemory(desc2,sizeof(struct TypeDescription));
  if (desc1->t!=desc2->t) return NULL; /* base types unequal */
  ptr1 = desc1;
  ptr2 = desc2;
  while (((ptr1!=NULL)||(ptr2!=NULL))&&(ptr1!=desc2)&&(ptr2!=desc1)){
    if (ptr1!=NULL) ptr1 = ptr1->refines;
    if (ptr2!=NULL) ptr2 = ptr2->refines;
  }
  if (ptr1==desc2) {
    /* desc1 is more refined */
    return (struct TypeDescription *)desc1;
  }
  if (ptr2==desc1) {
    /* desc2 is more refined */
    return (struct TypeDescription *)desc2;
  }
  return NULL;			/* unconformable */
}

struct ancestor {
  CONST struct TypeDescription *d;
  struct ancestor *next;
};
#define CreateAncestor(p) \
  (p) = (struct ancestor *)ascmalloc(sizeof(struct ancestor))
#define DestroyAncestor(p) ascfree(p)
/*
 * produce a list of ancestors starting at a base type and
 * continuing through d;
 */
static struct ancestor *CreateAncestorList(CONST struct TypeDescription *d) {
  struct ancestor *list=NULL, *new;
  while (d!=NULL) {
    CreateAncestor(new);
    new->d = d;
    new->next = list;
    list = new;
    d = d->refines;
  }
  return list;
}

static void DestroyAncestorList(struct ancestor *list) {
  struct ancestor *old;
  while (list != NULL) {
    old = list;
    list = list->next;
    DestroyAncestor(old);
  }
}

CONST struct TypeDescription *
GreatestCommonAncestor(CONST struct TypeDescription *d1,
                       CONST struct TypeDescription *d2)
{
  CONST struct TypeDescription *result;
  struct ancestor *head1, *head2, *a1, *a2;
  if (d1==d2) {
    return d1;
  }
  if (d1 == NULL || d2 == NULL || d1->t != d2->t) {
    return NULL; /* base types unequal */
  }
  a1 = head1 = CreateAncestorList(d1);
  a2 = head2 = CreateAncestorList(d2);
  assert(a1!=NULL);
  assert(a2!=NULL);
  /* now we know the head1->d == head2->d by construction, unless the
   * types in question are of MODEL, which has no base.
   * So walk, looking one step ahead for type incompatibility
   * or the end of either list.
   */
  while (a1->next != NULL &&		/* stop if a1 is at its last node */
         a2->next != NULL &&		/* stop if a2 is at its last node */
         a1->next->d == a2->next->d	/* stop if next is incompatible */) {
    a1 = a1->next;
    a2 = a2->next;
  }
  /* we are now at the last place a1 and a2 are compatible
   * or we are stuck at the head of each list so may
   * return a1 or a2 ->d if both are the same, else NULL.
   */
  result = (a1->d==a2->d) ? a1->d : NULL;
  DestroyAncestorList(head1);
  DestroyAncestorList(head2);
  return result;
}

struct gl_list_t *GetAncestorNames(CONST struct TypeDescription *d)
{
  struct gl_list_t *result;
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  result = gl_create(4L);
  while (GetRefinement(d) != NULL) {
    d = GetRefinement(d);
    gl_append_ptr(result,(VOIDPTR)GetName(d));
  }
  return result;
}

ChildListPtr GetChildListF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return (CONST ChildListPtr)(d->children);
}

enum type_kind GetBaseTypeF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return d->t;
}

CONST struct StatementList *GetStatementListF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return d->stats;
}

struct gl_list_t *GetInitializationListF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return d->init;
}

/* This recursively adds a method to d and its refiners.
 * When following a refiner trail, if we reach a type that
 * has a method with the same name as new, that type remains
 * undisturbed and that trail stops. This is because that
 * other definition of the method in the new type would supercede the
 * proc we are inserting in the ancestor type if the method
 * had been written in the ancestor when first the ancestor
 * was parsed.
 *
 * The method pointer this is called with does not become
 * part of anything. if it is kept, it is by copying.
 *
 * Due to the implementation of the base MODEL methods in
 * a global list, this function will allow you to Add 
 * a method with the same name as a global one. This is
 * inconsistent but desirable. If you wish to disallow that,
 * then the filter should be in AddMethods, not in this function.
 */
static
void RealAddMethod(struct TypeDescription *d, struct InitProcedure *new)
{
  unsigned long c;
  struct gl_list_t *opl;
  struct InitProcedure *old, *copy;
  struct gl_list_t *refiners;

  assert(d != NULL);
  assert(new != NULL);
  
  opl = GetInitializationList(d);
  old = SearchProcList(opl,ProcName(new));
  if (old != NULL) {
    return;
  }
  copy = CopyProcedure(new);
  gl_insert_sorted(opl,copy,(CmpFunc)CmpProcs);
  refiners = GetRefiners(d);
  if (refiners != NULL) {
    for (c=gl_length(refiners); c > 0; c--) {
      d = (struct TypeDescription *)gl_fetch(refiners,c);
      RealAddMethod(d,new);
    }
  }
}

static
void RealReplaceMethod(struct TypeDescription *d, struct InitProcedure *new)
{
  unsigned long c,pos;
  struct gl_list_t *opl;
  struct InitProcedure *old = NULL, *copy;
  struct gl_list_t *refiners;

  assert(d != NULL);
  assert(new != NULL);
  
  opl = GetInitializationList(d);
  pos = gl_search(opl,new,(CmpFunc)CmpProcs);

  if ((pos == 0) || (0 != (old = (struct InitProcedure *)gl_fetch(opl,pos))),
      GetProcParseId(old) > GetProcParseId(new)) {
    return; /* type never had it or type redefined it */
  }
  copy = CopyProcedure(new);
  gl_store(opl,pos,copy);
  DestroyProcedure(old);
  refiners = GetRefiners(d);
  if (refiners != NULL) {
    c = gl_length(refiners);
    for (; c > 0; c--) {
      d = (struct TypeDescription *)gl_fetch(refiners,c);
      RealReplaceMethod(d,new);
    }
  }
}

/* what's legal in the next two functions is different and needs
 * different checking.
 */
int AddMethods(struct TypeDescription *d, struct gl_list_t *pl, int err)
{
  unsigned long c,len;
  int old;
  struct gl_list_t *opl;
  struct InitProcedure *newproc, *oldproc;
  if (d==NULL) {
    return 1;
  }
  if (err!= 0) {
    FPRINTF(ASCERR, "%sADD METHODS abandoned due to previous syntax errors.\n",
      StatioLabel(3));
    return 1;
  }
  if (d == ILLEGAL_DEFINITION) {
    /* stick in UNIVERSAL list */
    opl = GetUniversalProcedureList();
    if (opl == NULL) {
      SetUniversalProcedureList(pl);
      return 0;
    }
    len = gl_length(pl);
    for (c = 1; c <= len; c++) {
      newproc = (struct InitProcedure *)gl_fetch(pl,c);
      oldproc = SearchProcList(opl,ProcName(newproc));
      if (oldproc != NULL) {
        err++;
        FPRINTF(ASCERR,
          "%s: ADD METHODS cannot replace MODEL DEFINITION METHOD %s.\n",
          StatioLabel(3),SCP(ProcName(newproc)));
        DestroyProcedure(newproc);
      } else {
        gl_append_ptr(opl,newproc);
      }
    }
    SetUniversalProcedureList(opl);
    gl_destroy(pl);
    return (int)err;
  } else {
    if (pl==NULL) {
      return 0;
    }
    old = ClaimNewMethodsTypeDesc(GetParseId(d),pl);
    if (old) {
      return 1; /* we want virgin pl only */
    }
    len = gl_length(pl);
    opl = GetInitializationList(d);
    for (c = 1; c <= len; c++) {
      newproc = (struct InitProcedure *)gl_fetch(pl,c);
      oldproc = SearchProcList(opl,ProcName(newproc));
      if (oldproc != NULL) {
        err++;
        /* should whine something here */
        FPRINTF(ASCERR,
          "%s: ADD METHODS cannot replace METHOD %s in type %s.\n",
          StatioLabel(3),SCP(ProcName(newproc)),SCP(GetName(d)));
      } else {
        RealAddMethod(d,newproc); 
        /* find and copy to destinations recursively */
      }
    }
    if (!err) {
      DestroyProcedureList(pl);
      return 0;
    }
    return 1;
  }
}

int ReplaceMethods(struct TypeDescription *d,struct gl_list_t *pl, int err)
{
  unsigned long c,len,pos;
  int old;
  struct gl_list_t *opl;
  struct InitProcedure *newproc, *oldproc;

  if (d==NULL) {
    return 1;
  }
  if (err!= 0) {
    FPRINTF(ASCERR, 
      "%sREPLACE METHODS abandoned due to previous syntax errors.\n",
      StatioLabel(3));
    return 1;
  }
  if (d == ILLEGAL_DEFINITION) {
    /* replace in UNIVERSAL list */
    opl = GetUniversalProcedureList();
    if (opl == NULL) {
      SetUniversalProcedureList(pl);
      return 0;
    }
    len = gl_length(pl);
    for (c = 1; c <= len; c++) {
      newproc = (struct InitProcedure *)gl_fetch(pl,c);
      pos = gl_search(opl,newproc,(CmpFunc)CmpProcs);
      if (pos == 0) {
        err++;
        FPRINTF(ASCERR,
          "%s: REPLACE METHODS cannot ADD method %s in MODEL DEFINITION.\n",
          StatioLabel(3),SCP(ProcName(newproc)));
        DestroyProcedure(newproc);
      } else {
        oldproc = (struct InitProcedure *)gl_fetch(opl,pos);
        gl_store(opl,pos,newproc);
        DestroyProcedure(oldproc);
      }
    }
    SetUniversalProcedureList(opl);
    gl_destroy(pl);
    return (int)err;
  } else {
    if (pl==NULL) {
      return 0;
    }
    old = ClaimNewMethodsTypeDesc(GetParseId(d),pl);
    if (old) {
      return 1; /* we want virgin pl only */
    }
    len = gl_length(pl);
    opl = GetInitializationList(d);
    for (c = 1; c <= len; c++) {
      newproc = (struct InitProcedure *)gl_fetch(pl,c);
      oldproc = SearchProcList(opl,ProcName(newproc));
      if (oldproc == NULL) {
        err++;
        FPRINTF(ASCERR,
          "%s: REPLACE METHODS cannot add METHOD %s in type %s.\n",
          StatioLabel(3),SCP(ProcName(newproc)),SCP(GetName(d)));
      } else {
        RealReplaceMethod(d,newproc); 
        /* find and copy to destinations (existing) recursively */
      }
    }
    if (!err) {
      DestroyProcedureList(pl);
      return 0;
    }
    return 1;
  }
}

void CopyTypeDescF(struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  d->ref_count++;
}

void DeleteNewTypeDesc(struct TypeDescription *d)
{
  if (d->ref_count!=1) {
    FPRINTF(ASCERR,"New type definition %s with unexpectedly high ref_count\n",
      SCP(GetName(d)));
  }
  DeleteTypeDesc(d);
}

void DeleteTypeDesc(struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  --d->ref_count;
  if (d->ref_count == 0){
#if (TYPELINKDEBUG)
    FPRINTF(ASCERR,"Deleteing type: %s, parseid %ld\n",
      d->name,GetParseId(d));
#endif
    switch(d->t){
    case relation_type:
    case logrel_type:
    case real_type:
    case boolean_type:
    case integer_type:
    case set_type:
    case symbol_type:
      if (d->u.atom.childinfo && d->children)
	DestroyChildDescArray(d->u.atom.childinfo,
			      ChildListLen(d->children));
      break;
    case array_type:
      if (d->u.array.desc) DeleteTypeDesc(d->u.array.desc);
      if (d->u.array.indices){
	gl_iterate(d->u.array.indices,(void (*)(VOIDPTR))DestroyIndexType);
	gl_destroy(d->u.array.indices);
      }
      RemoveArrayTypeDesc(d);
      break;
    case model_type:
      DestroyStatementList(d->u.modarg.declarations);
      DestroyStatementList(d->u.modarg.absorbed);
      DestroyStatementList(d->u.modarg.reductions);
      DestroyStatementList(d->u.modarg.wheres);
/* not in use. probably needs to be smarter if it was.
      if (d->u.modarg.argdata!=NULL) {
        gl_destroy(d->u.modarg.argdata);
      }
*/
      break;
    case patch_type:
      break;
    case when_type:
      break;
    case dummy_type:
      break;
    default:
      break;
    }
    if (d->refines != NULL) {
      UnLinkTypeDesc(d->refines,d); /* tell ancestor to forget d */
      DeleteTypeDesc(d->refines);  /* remove d's reference to ancestor */
      /* d->refines->refiners isn't a reference. */
    }
    if (d->refiners!=NULL) {
      assert(gl_length(d->refiners)==0); /* non 0 => bad refcount */
      gl_destroy(d->refiners);
    }
    if (d->children!=NULL) {
      DestroyChildList(d->children);
    }
    DestroyStatementList(d->stats);
    DestroyProcedureList(d->init);
    d->t = ERROR_KIND; /* should be error_type. patch will do */
    d->parseid = -(d->parseid); /* flip the sign */
    ascfree(d);
  }
}

unsigned GetByteSizeF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return (unsigned)d->u.atom.byte_length;
}

CONST struct ChildDesc *GetChildDescF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return d->u.atom.childinfo;
}

int GetUniversalFlagF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return d->universal;
}

unsigned short int GetTypeFlagsF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return d->flags;
}

unsigned int TypeHasDefaultStatementsF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return (d->flags & TYPECONTAINSDEFAULTS);
}

unsigned int TypeHasParameterizedInstsF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return (d->flags & TYPECONTAINSPARINSTS);
}


double GetRealDefaultF(CONST struct TypeDescription *d,
                       CONST char *file, CONST int line)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  if (GetBaseType(d)==real_type) {
    return d->u.atom.u.defval;
  } else {
    error_reporter(ASC_PROG_ERROR,file,line,NULL,"GetRealDefault called without real_type");
    return 0.0;
  }
}

unsigned GetBoolDefaultF(CONST struct TypeDescription *d,
                       CONST char *file, CONST int line)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  if (GetBaseType(d)==boolean_type) {
    return d->u.atom.u.defbool;
  } else {
    error_reporter(ASC_PROG_ERROR,file,line,NULL,"GetBoolDefault called without boolean_type");
    return 0;
  }
}

CONST dim_type *GetRealDimensF(CONST struct TypeDescription *d,
				CONST char *file, CONST int line)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  if (BaseTypeIsAtomic(d)) {
    return d->u.atom.dimp;
  } else {
    error_reporter(ASC_PROG_ERROR,file,line,NULL,"GetRealDimens called non-atom type");
    return WildDimension();
  }
}

CONST dim_type *GetConstantDimensF(CONST struct TypeDescription *d,
				CONST char *file, CONST int line)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  if (GetBaseType(d)==real_constant_type ||
	GetBaseType(d)==integer_constant_type ||
	GetBaseType(d)==boolean_constant_type ||
	GetBaseType(d)==symbol_constant_type) {
    return d->u.constant.dimp;
  } else {
    error_reporter(ASC_PROG_ERROR,file,line,NULL,"GetConstDimens called without constant type");
    return WildDimension();
  }
}

symchar *GetNameF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  assert(d->ref_count > 0);
  return d->name;
}

int TypesAreEquivalent(CONST struct TypeDescription *d1,
                       CONST struct TypeDescription *d2)
{
  unsigned long n;
  
  if (d1 == d2) {
    return 1;
  }
  if (d1->t != d2->t ||
      d1->name != d2->name ||
      d1->universal != d2->universal ||
      d1->refines != d2->refines) {
    return 0; /* basetype, univ, symtab name, ancestor must be == */
  }
  /* 
   * check special things, then for all types, check stats, init.
   */
  switch (d1->t) {
  case real_type:
    if (d1->u.atom.defaulted != d2->u.atom.defaulted ||
        (d1->u.atom.defaulted  && 
           d1->u.atom.u.defval != d2->u.atom.u.defval) ||
        d1->u.atom.dimp != d2->u.atom.dimp
       ) {
      return 0;
    }
    break;
  case boolean_type:
    if (d1->u.atom.defaulted != d2->u.atom.defaulted ||
        (d1->u.atom.defaulted  && 
            d1->u.atom.u.defbool != d2->u.atom.u.defbool)
       ) {
      return 0;
    }
    break;
  case integer_type:
    if (d1->u.atom.defaulted != d2->u.atom.defaulted ||
        (d1->u.atom.defaulted  && 
            d1->u.atom.u.defint != d2->u.atom.u.defint)
       ) {
      return 0;
    }
    break;
  case symbol_type:
    if (d1->u.atom.defaulted != d2->u.atom.defaulted ||
        (d1->u.atom.defaulted  && 
            d1->u.atom.u.defsym != d2->u.atom.u.defsym)
       ) {
      return 0;
    }
    break;
  case real_constant_type:
    if (d1->u.constant.defaulted != d2->u.constant.defaulted ||
        (d1->u.constant.defaulted  && 
           d1->u.constant.u.defreal != d2->u.constant.u.defreal) ||
        d1->u.constant.dimp != d2->u.constant.dimp
       ) {
      return 0;
    }
    break;
  case boolean_constant_type:
    if (d1->u.constant.defaulted != d2->u.constant.defaulted ||
        (d1->u.constant.defaulted  && 
            d1->u.constant.u.defboolean != d2->u.constant.u.defboolean)
       ) {
      return 0;
    }
    break;
  case integer_constant_type:
    if (d1->u.constant.defaulted != d2->u.constant.defaulted ||
        (d1->u.constant.defaulted  && 
            d1->u.constant.u.definteger != d2->u.constant.u.definteger)
       ) {
      return 0;
    }
    break;
  case symbol_constant_type:
    if (d1->u.constant.defaulted != d2->u.constant.defaulted ||
        (d1->u.constant.defaulted  && 
            d1->u.constant.u.defsymbol != d2->u.constant.u.defsymbol)
       ) {
      return 0;
    }
    break;
  case set_type: /* odd! */
  case relation_type:
    /* fall through */
  case logrel_type:
    break;
  case array_type:
    return 0; /* array types are wierd */
  case model_type:
  if (
      CompareStatementLists(
          d1->u.modarg.declarations,
          d2->u.modarg.declarations,&n) != 0 ||
      CompareStatementLists(
          d1->u.modarg.absorbed,
          d2->u.modarg.absorbed,&n) != 0 ||
      CompareStatementLists(
          d1->u.modarg.reductions,
          d2->u.modarg.reductions,&n) != 0 ||
      CompareStatementLists( 
          d1->u.modarg.wheres, 
          d2->u.modarg.wheres,&n) != 0  ||
      CompareChildLists( d1->children, d2->children,&n) != 0 
     ) {
    return 0;
  }
/* not in use. probably needs to be smarter if it was.
 *    if (d->u.modarg.argdata!=NULL) {
 *    }
 */
    break;
  case patch_type:
    return 0; /* patches are not to be checked in detail */
  case when_type:
  case dummy_type:
    return 0; /* not to be checked. and should never be sent. */
  default:
    return 0;  /* say what? */
  }
/*
 *if (d->refiners!=NULL) {
 *  we don't care who REFINES us, we're only comparing these two.
 *  presumably one is new and won't have refiners yet.
 *}
 */
  if (CompareStatementLists(d1->stats,d2->stats,&n) != 0 ||
      CompareProcedureLists(d1->init,d2->init,&n) != 0) {
    return 0;
  }
  return 1; /* if everything checks, return 1; */
}

void DifferentVersionCheck(CONST struct TypeDescription *desc1,
			   CONST struct TypeDescription *desc2)
{
  register CONST struct TypeDescription *ptr;
  int erred;
  erred = 0;
  AssertAllocatedMemory(desc1,sizeof(struct TypeDescription));
  AssertAllocatedMemory(desc2,sizeof(struct TypeDescription));

  if (desc1->t != desc2->t) return;
  ptr = desc1;
  while (ptr){
    if (!CmpSymchar(ptr->name,desc2->name)){
      erred = 1;
      FPRINTF(ASCERR,"%s and %s are conformable by name;",
	      SCP(desc1->name), SCP(desc2->name));
      FPRINTF(ASCERR,"but they are unconformable because of different");
      FPRINTF(ASCERR,"different type versions.\n");
   }
    ptr = ptr->refines;
  }
  ptr = desc2;
  while (ptr){
    if (!CmpSymchar(ptr->name,desc1->name)) {
      erred = 1;
      FPRINTF(ASCERR,"%s and %s are conformable by name;",
	      SCP(desc1->name), SCP(desc2->name));
      FPRINTF(ASCERR,"but they are unconformable because of different");
      FPRINTF(ASCERR,"type versions.\n");
    }
    ptr = ptr->refines;
  }
  if (erred) {
    FPRINTF(ASCERR,"Different versions caused by previous redefinition\n");
    FPRINTF(ASCERR,
            "Try writing values, deleting all types, and recompiling\n");
  }
}

struct TypeDescription *GetStatTypeDesc(CONST struct Statement *s) {
  symchar *tn;
  if (s==NULL) return NULL;
  switch(StatementType(s)) {
  case ISA:
  case WILLBE:
  case IRT:
    tn = GetStatType(s);
    if (tn!=NULL && tn != GetBaseTypeName(set_type)) {
      return FindType(tn);
    }
    break;
  case ALIASES:
  case ARR:
  case ATS:
  case WBTS:
  case WNBTS:
  case AA:
  case FOR:
  case REL:
  case LOGREL:
  case ASGN:
  case CASGN:
  case WHEN:
  case FNAME:
  case SELECT:
  case EXT:
  case REF:
  case COND:
  case RUN:
  case CALL:
  case IF:
    break;
  default:
    FPRINTF(ASCERR,"GetStatTypeDesc called with unknown stat type\n");
  }
  return NULL;
}

void WriteArrayTypeList(FILE *f) {
  struct ArrayDescList *ptr;
  unsigned long count = 0L;

  FPRINTF(f,"Number of descriptions made: %lu\n",g_array_desc_count);
  ptr = g_array_desc_list;
  if (ptr!=NULL) {
    FPRINTF(f,"Descriptions:\n");
  }
  while (ptr!=NULL) {
    count++;
    if (ptr->desc!=NULL) {
      WriteDefinition(f,ptr->desc);
    } else {
      FPRINTF(f," *** NULL Description! ***\n");
    }
    ptr = ptr->next;
  }
  FPRINTF(f,"Number of descriptions current: %lu\n",count);
}


unsigned TypeShowF(CONST struct TypeDescription *d)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  return (d->flags & TYPESHOW);
}


void SetTypeShowBit(struct TypeDescription *d, int value)
{
  AssertAllocatedMemory(d,sizeof(struct TypeDescription));
  assert((d->t&ERROR_KIND)==0);
  if (value) {
    d->flags |= TYPESHOW;
  } else {
    d->flags &= ~TYPESHOW;
  }
}

