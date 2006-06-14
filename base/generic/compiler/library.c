/* ex: set ts=8: */
/*
 *  Library Implementation
 *  by Tom Epperly
 *  Created: 1/15/89
 *  Version: $Revision: 1.28 $
 *  Version control file: $RCSfile: library.c,v $
 *  Date last modified: $Date: 1998/06/23 22:02:08 $
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
 * 
 */

#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include "compiler.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include <general/list.h>
#include "compiler.h"
#include "symtab.h"
#include "notate.h"
#include "fractions.h"
#include "dimen.h"
#include "functype.h"
#include "types.h"
#include "child.h"
#include "type_desc.h"
#include "type_descio.h"
#include "prototype.h"
#include "dump.h"
#include "typedef.h"
#include "module.h"
#include "library.h"

#ifndef lint
static CONST char LibraryRCSid[]="$Id: library.c,v 1.28 1998/06/23 22:02:08 ballan Exp $";
#endif

/*
 * hashing on heap symbol pointer. SIZE must be 2^n (n even)
 * and mask in LIBHASHINDEX must be 2^n - 1.
 */
#define LIBRARYHASHSIZE (unsigned long)1024
/*
 * hash function multiply, shift by 30 - n, and mask to SIZE.
 */
#define LIBHASHINDEX(p) (((((long) (p))*1103515245) >> 20) & 1023)

/* these make an important optimization possible.
 * The relation and when types must still be in the library,
 * but heavy use clients can get the same pointers by
 * FindRelationType and  FindWhenType.
 * AddType is responsible for maintaining these.
 * These pointers should never change values except
 * if the library is destroyed.
 */
static struct TypeDescription *g_relation_type = NULL;
static struct TypeDescription *g_logrel_type = NULL;
static struct TypeDescription *g_dummy_type = NULL;
static struct TypeDescription *g_when_type = NULL;
static struct TypeDescription *g_set_type = NULL;
static struct TypeDescription *g_externalmodel_type = NULL;

/*
 * array of symbol table entries we need.
 */
static symchar *g_symbols[14];
#define G__SYMBOL_NAME 	g_symbols[0]
#define G__REAL_NAME	g_symbols[1]
#define G__INTEGER_NAME	g_symbols[2]
#define G__BOOLEAN_NAME 	g_symbols[3]
#define G__CON_SYMBOL_NAME 	g_symbols[4]
#define G__CON_REAL_NAME 	g_symbols[5]
#define G__CON_INTEGER_NAME g_symbols[6]
#define G__CON_BOOLEAN_NAME g_symbols[7]
#define G__SET_NAME 	g_symbols[8]
#define G__WHEN_NAME 	g_symbols[9]
#define G__REL_NAME 	g_symbols[10]
#define G__LOGREL_NAME 	g_symbols[11]
#define G__UNSELECTED 	g_symbols[12]
#define G__EXT_NAME 	g_symbols[13]

struct LibraryStructure {
  struct LibraryStructure *next;
  struct TypeDescription *type;
  unsigned long open_count;
};

struct LibraryStructure *LibraryHashTable[LIBRARYHASHSIZE];

void InitializeLibrary(void)
{
  unsigned c;
  /* init hash */
  for(c=0;c<LIBRARYHASHSIZE;LibraryHashTable[c++]=NULL); /* no body */
  /* init reused symbols */
  G__SYMBOL_NAME 	= GetBaseTypeName(symbol_type);
  G__REAL_NAME	= GetBaseTypeName(real_type); 
  G__INTEGER_NAME	= GetBaseTypeName(integer_type);
  G__BOOLEAN_NAME 	= GetBaseTypeName(boolean_type);
  G__CON_SYMBOL_NAME = GetBaseTypeName(symbol_constant_type);
  G__CON_REAL_NAME 	= GetBaseTypeName(real_constant_type);
  G__CON_INTEGER_NAME = GetBaseTypeName(integer_constant_type);
  G__CON_BOOLEAN_NAME = GetBaseTypeName(boolean_constant_type);
  G__SET_NAME 	= GetBaseTypeName(set_type);
  G__WHEN_NAME 	= GetBaseTypeName(when_type);
  G__REL_NAME 	= GetBaseTypeName(relation_type);
  G__LOGREL_NAME 	= GetBaseTypeName(logrel_type);
  G__UNSELECTED	= GetBaseTypeName(dummy_type);
  /* odd cases */
  G__EXT_NAME 	= GetBaseTypeName(model_type & patch_type);
}

struct TypeDescription *FindRelationType(void)
{
  if (g_relation_type==NULL) {
    FPRINTF(ASCERR,
            "FindRelationType called before RELATION_DEFINITION set.\n");
    FPRINTF(ASCERR,"You need a system.a4l or equivalent loaded.\n");
  }
  return g_relation_type;
}

struct TypeDescription *FindLogRelType(void)
{
  /* probably should be an assert instead of this if */
  if (g_logrel_type==NULL) {
    FPRINTF(ASCERR,"FindLogRelType called before logrel defined.\n");
    FPRINTF(ASCERR,"You need a system.a4l or equivalent loaded.\n");
  }
  return g_logrel_type;
}

struct TypeDescription *FindSetType(void)
{
  /* probably should be an assert instead of this if */
  if (g_set_type==NULL) {
    FPRINTF(ASCERR,"FindSetType called before set defined.\n");
    FPRINTF(ASCERR,"This is extremely odd!.\n");
  }
  return g_set_type;
}

struct TypeDescription *FindWhenType(void)
{
  /* probably should be an assert instead of this if */
  if (g_when_type==NULL) {
    FPRINTF(ASCERR,"FindWhenType called before when defined.\n");
    FPRINTF(ASCERR,"This is extremely odd!.\n");
  }
  return g_when_type;
}

struct TypeDescription *FindDummyType(void)
{
  /* probably should be an assert instead of this if */
  if (g_dummy_type==NULL) {
    FPRINTF(ASCERR,"FinddummyType called before when defined.\n");
    FPRINTF(ASCERR,"This is extremely odd!.\n");
  }
  return g_dummy_type;
}

struct TypeDescription *FindExternalType(void)
{
  /* probably should be an assert instead of this if */
  if (g_externalmodel_type==NULL) {
    FPRINTF(ASCERR,"FindExternalType called before external defined.\n");
    FPRINTF(ASCERR,"This is extremely odd!.\n");
  }
  return g_externalmodel_type;
}

struct TypeDescription *FindType(symchar *name)
{
  struct LibraryStructure *ptr;

  if (name==NULL) return NULL;
  assert(AscFindSymbol(name) != NULL);
  ptr = LibraryHashTable[LIBHASHINDEX(SCP(name))];
  /*if(ptr==NULL){
    CONSOLE_DEBUG("Found no values in the LibraryHashTable");
  }*/
  while (ptr!=NULL){
    /* CONSOLE_DEBUG("Found a type '%s'...",SCP(GetName(ptr->type)) ); */
    if (name == GetName(ptr->type)) { /* pointers == on table symbols */
      assert((ptr->type->t & ERROR_KIND)==0);
      return ptr->type;
    }
    ptr = ptr->next;
  }
  /* CONSOLE_DEBUG("Failed to locate '%s'", name); */
  return NULL;
}

void DestroyLibrary(void)
{
  register unsigned c;
  register struct LibraryStructure *ptr,*next;
  for(c=0;c<LIBRARYHASHSIZE;c++) {
    if(LibraryHashTable[c]!=NULL){
      ptr = LibraryHashTable[c];
      while(ptr!=NULL){
	DeleteTypeDesc(ptr->type);
	next = ptr->next;
	ascfree((char *)ptr);
	ptr = next;
      }
      LibraryHashTable[c]=NULL;
    }
  }
  DestroyTypedefRecycle();
  g_externalmodel_type = NULL;
  g_relation_type = NULL;
  g_logrel_type = NULL;
  g_dummy_type = NULL;
  g_when_type = NULL;
  g_set_type = NULL;
}


struct gl_list_t *FindFundamentalTypes(void)
{
  register unsigned c;
  register struct LibraryStructure *ptr,*next;
  struct TypeDescription *d;
  struct gl_list_t *fundies;

  fundies = gl_create(40L);

  for(c=0;c<LIBRARYHASHSIZE;c++) {
    if(LibraryHashTable[c]!=NULL){
      ptr = LibraryHashTable[c];
      while(ptr!=NULL){
	d = ptr->type;
        if (GetModule(d)==NULL) {
          gl_append_ptr(fundies,(VOIDPTR)d);
        }
	next = ptr->next;
	ptr = next;
      }
    }
  }
  return fundies;
}

static int CmpDescNames(struct TypeDescription *desc1,
                        struct TypeDescription *desc2)
{
  assert(desc1&&desc2);
  return CmpSymchar(GetName(desc1),GetName(desc2));
}

static int CmpDescModNames(struct TypeDescription *desc1,
                           struct TypeDescription *desc2)
{
  assert(desc1&&desc2);
  return strcmp(Asc_ModuleName(GetModule(desc1)),
                Asc_ModuleName(GetModule(desc2)));
}

static int CmpDescModPtrs(struct TypeDescription *desc1,
                          struct TypeDescription *desc2)
{
  assert(desc1&&desc2);
  return Asc_ModulesEqual(GetModule(desc1),GetModule(desc2));
}

static void ReplaceType(struct TypeDescription *desc,
			struct LibraryStructure *ptr)
{
  DeletePrototype(GetName(desc));
  TrashType(GetName(desc));
  DestroyNotesOnType(LibraryNote(),GetName(desc));
  DeleteTypeDesc(ptr->type);
  ptr->type = desc;
}

static
struct TypeDescription *EquivalentExists(struct TypeDescription *desc)
{
  struct TypeDescription *old;
  if (desc == NULL) {
    return NULL;
  }
  old=FindType(GetName(desc));
  if (old == NULL) {
    return NULL;
  }
  if (TypesAreEquivalent(old,desc)) {
    return old;
  }
  /* FPRINTF(ASCERR,"TYPE FOUND FOR %s BUT NOT EQUIV\n",GetName(desc)); */
  return NULL;
}

int AddType(struct TypeDescription *desc)
{
  unsigned long bucket;
  struct TypeDescription *equiv;
  struct LibraryStructure *ptr;

  /* FPRINTF(ASCERR,"ADD TYPE '%s'...\n",GetName(desc)); */
  assert(desc!=NULL);

  equiv = EquivalentExists(desc);
  if (equiv != NULL) {
    CONSOLE_DEBUG("Keeping equivalent %s loaded from %s."
		,SCP(GetName(desc)),Asc_ModuleName(GetModule(equiv))
	);
    DeleteNewTypeDesc(desc);
    return 0;
  }

  if (GetName(desc) == G__REL_NAME) {
    g_relation_type = desc;
    /* and we will assume this pointer will replace any current relationdef */
    /* in the one case where it does not replace, we will have to reset it */
  }
  if (GetName(desc) == G__LOGREL_NAME) {
    g_logrel_type = desc;
    /* and we will assume this pointer will replace any current relationdef */
    /* in the one case where it does not replace, we will have to reset it */
  }
  /* the following system types don't have a module associated.
   * If there's a module, it can't be one of these that is being defined.
   */
  if (GetModule(desc) == NULL) {
    if (GetName(desc) == G__WHEN_NAME) {
      g_when_type = desc;
      /* and we will assume this pointer will replace any current whendef */
      /* in the one case where it does not replace, we will have to reset it */
    }
    if (GetName(desc) == G__UNSELECTED) {
      g_dummy_type = desc;
      /* and we will assume this pointer will replace any current whendef */
      /* in the one case where it does not replace, we will have to reset it */
    }
    if (GetName(desc) == G__SET_NAME) {
      g_set_type = desc;
      /* and we will assume this pointer will replace any current set def */
      /* in the one case where it does not replace, we will have to reset it */
    }
    if (GetName(desc) == G__EXT_NAME) {
      g_externalmodel_type = desc;
      /* and we will assume this pointer will replace any current extdef */
      /* in the one case where it does not replace, we will have to reset it */
    }
  }
  bucket = LIBHASHINDEX(SCP(GetName(desc)));
  ptr = LibraryHashTable[bucket];
  /* search for name collisions */
  while (ptr) {

    if (desc == ptr->type) {
      /* FPRINTF(ASCERR,"...KEEPING OLD TYPE\n"); */
      return 0;
    }

    if (GetName(desc) == GetName(ptr->type)) {
      if (CmpDescModPtrs(desc,ptr->type)) {
	if (Asc_ModuleTimesOpened(GetModule(desc)) <= ptr->open_count) {
	  FPRINTF(ASCERR,"Multiple definitions of type %s in module %s.\n",
		  SCP(GetName(desc)),Asc_ModuleName(GetModule(desc)));
	  FPRINTF(ASCERR,"  Overwriting previous definition.\n");
	} else {
	  if (GetRefinement(desc) == GetRefinement(ptr->type)) {
            /* here's the one cases */
            if (GetName(desc) == G__REL_NAME) {
              g_relation_type = ptr->type;
            }
            if (GetName(desc) == G__LOGREL_NAME) {
              g_logrel_type = ptr->type;
            }
            if (GetModule(desc)==NULL) {
              if (GetName(desc) == G__WHEN_NAME) {
                g_when_type = ptr->type;
              }
              if (GetName(desc) == G__UNSELECTED) {
                g_dummy_type = ptr->type;
              }
              if (GetName(desc) == G__SET_NAME) {
                g_set_type = ptr->type;
              }
              if (GetName(desc) == G__EXT_NAME) {
                g_externalmodel_type = ptr->type;
              }
            }
	    /* keep the old copy. */
	    DeleteNewTypeDesc(desc);
	    FPRINTF(ASCERR,"KEEPING OLD TYPE (DELETING)\n");
            return 0;
	  } else {
            ReplaceType(desc,ptr);
	    /* the thing this type refines has been modified so the */
	    /* library needs this new type definition */
	  }
	  FPRINTF(ASCERR,"ADDED (REPLACETYPE)\n");
	  return 1;
	}
      } else {
        if ( CmpDescModNames(desc,ptr->type) != 0 ) {
          FPRINTF(ASCERR,"Type definition of %s in module %s conflicts\n",
                  SCP(GetName(desc)), Asc_ModuleName(GetModule(desc)) );
          FPRINTF(ASCERR,"with definition in module %s.\n",
                  Asc_ModuleName(GetModule(ptr->type)) );
          FPRINTF(ASCERR,"  Overwriting previous definition.\n");
        } else {
          FPRINTF(ASCERR,"Updating %s.\n",SCP(GetName(ptr->type)));
        }
      }
      ReplaceType(desc,ptr);
      FPRINTF(ASCERR,"ADDING TYPE (REPLACETYPE 2)\n");
      return 1;
    }

    ptr = ptr->next;
  }

  /* add new type to the head of the list */
  ptr =	(struct LibraryStructure *)ascmalloc(sizeof(struct LibraryStructure));
  ptr->next = LibraryHashTable[bucket];
  ptr->type = desc;
  ptr->open_count = Asc_ModuleTimesOpened(GetModule(desc));
  LibraryHashTable[bucket] = ptr;
  /* FPRINTF(ASCERR,"ADDED TYPE '%s'\n",GetName(desc)); */
  if(FindType(GetName(desc))==NULL){
    FPRINTF(ASCERR,"UNABLE TO FIND TYPE '%s'\n",GetName(desc));
  }else{
	/* FPRINTF(ASCERR,"TYPE '%s' FOUND OK\n",GetName(desc)); */
  }
	
  return 1;
}

struct  gl_list_t *DefinitionList(void)
{
  struct gl_list_t *result;
  register unsigned c;
  register struct LibraryStructure *ptr;
  result = gl_create(200L);
  for(c=0;c<LIBRARYHASHSIZE;c++){
    ptr = LibraryHashTable[c];
    while(ptr){
      if (GetBaseType(ptr->type)!=array_type)
	gl_append_ptr(result,(VOIDPTR)ptr->type);
      ptr = ptr->next;
    }
  }
  gl_sort(result,(CmpFunc)CmpDescNames);
  return result;
}

/*
 * The following is so as not to export the library
 * hashfunctions internals.
 */
unsigned int CheckFundamental(symchar *f)
{
  if (
      (f==G__INTEGER_NAME) ||
      (f==G__REAL_NAME) ||
      (f==G__SYMBOL_NAME) ||
      (f==G__BOOLEAN_NAME) ||
      (f==G__SET_NAME) ||
      (f==G__CON_BOOLEAN_NAME) ||
      (f==G__CON_INTEGER_NAME) ||
      (f==G__CON_REAL_NAME) ||
      (f==G__CON_SYMBOL_NAME)
     ) {
    return 1;
  } else {
    return 0;
  }
}


struct gl_list_t *Asc_TypeByModule(CONST struct module_t *m)
{
  struct gl_list_t *result;
  register unsigned c;
  register struct LibraryStructure *ptr;
  CONST struct module_t *modptr;

  assert(m != NULL);
  result = gl_create(20L);
  for( c = 0; c < LIBRARYHASHSIZE; c++ ) {
    ptr = LibraryHashTable[c];
    while( ptr ) {
      modptr = GetModule(ptr->type);
      if (modptr != NULL) {
	if((GetBaseType(ptr->type) != array_type) && (modptr == m)) {
	  gl_append_ptr(result,(VOIDPTR)ptr->type->name);
        }
      }
      ptr = ptr->next;
    }
  }
  return result;
}

/* sometimes there is too much confusion about parents and children !! */

struct gl_list_t *TypesThatRefineMe(symchar *name)
{
  struct gl_list_t *result;
  register unsigned c;
  register struct LibraryStructure *ptr;
  CONST struct TypeDescription *refdesc;
  symchar *refname;

  /*
   * Actually, this is fairly efficient for atoms and base types,
   * but for models it is inefficient.
   */
  assert(name!=NULL && AscFindSymbol(name) != NULL);
  result = gl_create(40L);
  for(c=0;c<LIBRARYHASHSIZE;c++){
    ptr = LibraryHashTable[c];
    while(ptr){
      refdesc = GetRefinement(ptr->type);
      if (refdesc != NULL) {
	refname = GetName(refdesc);
	if ( (GetBaseType(ptr->type) != array_type) &&
	     (refname == name)) {
	  gl_append_ptr(result,(VOIDPTR)ptr->type->name);
        }
      }
      ptr = ptr->next;
    }
  }
  return result;
}

/*
 * returns a flat list of typenames that refine the type given
 */
struct gl_list_t *AllTypesThatRefineMe_Flat(symchar *name)
{
  struct gl_list_t *result;
  register unsigned c;
  register struct LibraryStructure *ptr;
  struct TypeDescription *refdesc,*desc;
  symchar *refname;

  assert(name!=NULL && AscFindSymbol(name) != NULL);
  desc=FindType(name);
  if (!desc) { /* nobody home by that name */
    result=gl_create(1L);
    return result;
  }
  if (GetBaseType(desc)==model_type) {
    result=gl_create(10L); /* probably only needs extension on components */
  } else {
    result=gl_create(200L); /* lots of atoms, usually */
  }
  for(c=0;c<LIBRARYHASHSIZE;c++){
    ptr = LibraryHashTable[c];
    while(ptr){
      refdesc=ptr->type;
      if (refdesc) {
        if (MoreRefined(desc,refdesc)==refdesc && desc!=refdesc) {
          refname = GetName(refdesc);
          if ( GetBaseType(refdesc)!=array_type ) {
            gl_append_ptr(result,(VOIDPTR)ptr->type->name);
          }
        }
      }
      ptr = ptr->next;
    }
  }
  return result;
}

static
struct gl_list_t *AllTypesThatRefineMe_FlatType(struct TypeDescription *desc)
{
  struct gl_list_t *result;
  register unsigned c;
  register struct LibraryStructure *ptr;
  struct TypeDescription *refdesc;

  if (!desc) { /* nobody home by that name */
    result=gl_create(1L);
    return result;
  }
  if (GetBaseType(desc)==model_type)
    result=gl_create(10L); /* probably only needs extension on components */
  else
    result=gl_create(200L); /* lots of atoms, usually */
  for(c=0;c<LIBRARYHASHSIZE;c++){
    ptr = LibraryHashTable[c];
    while(ptr){
      refdesc=ptr->type;
      if (refdesc) {
        if (MoreRefined(desc,refdesc)==refdesc &&
            desc!=refdesc) {
	  if ( GetBaseType(refdesc)!=array_type )
	    gl_append_ptr(result,(VOIDPTR)refdesc);
        }
      }
      ptr = ptr->next;
    }
  }
  return result;
}

/*
 * fl is the list of children of not yet established heredity.
 * hd is a HierarchyNode with desc and descendents list set, though
 * the descendents list may be incomplete. fl will shrink if the head
 * given has any direct descendents.
 * This is horrendously inefficient for doing long (>20) lists.
 * Breadth first searching.
 */
static void EstablishPaternity(struct HierarchyNode *hd, struct gl_list_t *fl)
{
  struct TypeDescription *desc=NULL;
  unsigned long c,size,end;
  struct HierarchyNode head, *child;
  head=(*hd);
  for (c=1; c <=gl_length(fl);) {
    desc=(struct TypeDescription *)gl_fetch(fl,c);
    if (GetRefinement(desc)==head.desc) {
      /* squirrel away direct descendents and delete from fl. */
      child=(struct HierarchyNode *)ascmalloc(sizeof(struct HierarchyNode));
      child->desc=desc;
      gl_append_ptr(head.descendents,(VOIDPTR)child);
      gl_delete(fl,c,0); /* here's the horrendous bit */
    } else {
      /* indirect descendents pass over */
      c++;
    }
  }
  /* make lists for the found children */
  size=head.descendents->capacity;
  end=gl_length(head.descendents);
  for ( c=1; c<=end; c++) {
    child=(struct HierarchyNode *)gl_fetch(head.descendents,c);
    child->descendents=gl_create(size);
    if (gl_length(fl) >0) {
      EstablishPaternity(child,fl);
    } else {
      size=2; /* everyone gets a list, even if fl empty. */
    }
  }
}

/*
 * returns a HierarchyNode tree of types that refine the type given
 */
struct HierarchyNode *AllTypesThatRefineMe_Tree(symchar *name)
{
  struct gl_list_t *flatlist;
  register unsigned c,end;
  struct TypeDescription *refdesc,*desc;
  struct HierarchyNode *head;

  assert(name!=NULL && AscFindSymbol(name) != NULL);
  desc = FindType(name);
  if (!desc) return NULL; /* nobody home by that name */
  head = (struct HierarchyNode *)ascmalloc(sizeof(struct HierarchyNode));
  head->desc=desc;
  if (GetBaseType(desc)==model_type) {
    head->descendents=gl_create(10L);
    /* probably only needs extension on components */
  } else {
    head->descendents=gl_create(200L);
    /* lots of atoms, usually */
  }
  flatlist=AllTypesThatRefineMe_FlatType(desc);
  EstablishPaternity(head,flatlist);
  if ((end=gl_length(flatlist))>0) {
    FPRINTF(ASCERR,"The following types in the current library\n");
    FPRINTF(ASCERR,"refine old types refining %s\n",SCP(name));
    FPRINTF(ASCERR,"that are no longer current. Use of these types.\n");
    FPRINTF(ASCERR,"Current type (refines):       replaced type:\n");
    for (c=1;c<=end; c++) {
      desc=(struct TypeDescription *)gl_fetch(flatlist,c);
      if (desc!=NULL) {
        FPRINTF(ASCERR,"%-30s",SCP(GetName(desc)));
        if ((refdesc=GetRefinement(desc))!=NULL) {
          FPRINTF(ASCERR,"%s\n",SCP(GetName(refdesc)));
        } else {
          FPRINTF(ASCERR,"%s\n","null_type_description!!");
        }
      }
    }
  }
  gl_destroy(flatlist);
  return head;
}

void DestroyHierarchyNode(struct HierarchyNode *head)
{
  if (head) {
    head->desc=NULL;
    if (head->descendents) {
      gl_iterate(head->descendents,(void (*)(VOIDPTR))DestroyHierarchyNode);
      gl_destroy(head->descendents);
    }
    ascfree(head);
  }
}

int IsTypeRefined(CONST struct TypeDescription *desc)
{
  register unsigned c;
  register struct LibraryStructure *ptr;
  CONST struct TypeDescription *refdesc;

  for(c=0;c<LIBRARYHASHSIZE;c++){
    ptr = LibraryHashTable[c];
    while(ptr){
      refdesc= GetRefinement(ptr->type);
      if (refdesc != NULL) {
	if ((GetBaseType(ptr->type)!=array_type) &&
	    (desc==refdesc))
	  return 1;
      }
      ptr = ptr->next;
    }
  }
  return 0; /* if here the type is not refined */
}

