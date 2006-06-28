/*
 *  notate.c
 *  By Ben Allan
 *  4/98
 *  Part of ASCEND
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: notate.c,v $
 *  Date last modified: $Date: 1998/06/18 18:44:58 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */
/* This file defines and manages a little (we hope)
 * database system for storing NOTES in-core in a variety
 * of quickly accessible ways.
 * If scale gets to be an issue, this could become a wrapper
 * for a real database someday.
 * This implementation is not optimized for anything -- the
 * intent here is to learn if it can be done at all.
 */
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/pool.h>
#include "compiler.h"
#include "symtab.h"
#include "braced.h"
#include "vlist.h"
#include "fractions.h"
#include "dimen.h"
#include "expr_types.h"
#include "sets.h"
#include "name.h"
#include "instance_enum.h"
#include "cmpfunc.h"
#include "notate.h"

struct Note {
  symchar *typename;	/* ascend type library name, if any, for context. */
  symchar *id;		/* child name in type, if only for 1 child, or SELF */
  symchar *method;	/* method name type, if in method context */
  symchar *lang;	/* language keyword in '' */
  CONST char *filename;	/* where note is from */
  struct bracechar *t;	/* text of the note */
  VOIDPTR data;		/* note data */
  int line;		/* where note occured, to our best guess */
  enum NoteData kind;	/* void pointer type */
  int refcount;		/* number of database references to the note */
  int found;		/* 0 normal. 1 if found. 2 if collected. search flag */
  struct data_base *db;	/* database */
  struct Note *next;	/* database master list chain */
};

struct note_bucket {
  symchar *key;		/* hash key. type, id, or other heap pointer. */
  struct note_bucket *next;	/* next bucket. duh */
  void *obj;		/* data for bucket. usually a note or list. */
};

/* replace this with a pool */
#define NPALLOC ASC_NEW(struct Note)
#define NPFREE(n) ascfree(n)

/* replace this with a pool */
#define NBALLOC ASC_NEW(struct note_bucket)
#define NBFREE(b) ascfree(b)

/* increment note reference count */
#define ReferenceNote(n) ((n)->refcount++)

/* list of held tokens for clients */
#define GNT db->note_tokens

#define NTAB 1024
#define PTRHASH(p) (((((long) (p))*1103515245) >> 20) & 1023)

/*
 * This module manages possibly several databases of this sort.
 */
struct data_base {
  struct Note *all;			/* all notes committed */
  struct gl_list_t *note_tokens;
  symchar *dbid;		/* name of this database */
  struct data_base *next;	/* linked list of databases */
  struct note_bucket *typetab[NTAB];		/* hash keyed on type name */
  struct note_bucket *idtab[NTAB];		/* hash keyed on id name */
  int dead;
};

/*
 * All the databases have this in common and initing one
 * will init all the fields if it has not been inited.
 * Destroying the last dbid will also uninit all these fields.
 */
#define NDB g_notes_data_base
static struct fixed_data {
  symchar *inlinenote;		/* symtab entry 'inline' */
  symchar *selfnote;		/* symtab entry 'SELF' */
  symchar *librarynote;		/* symtab entry 'Loaded Libraries' */
  symchar *globalnote;		/* symtab entry 'All Known Files' */
  /* pool_t */
  struct data_base *dblist;	/* list of created databases */
} g_notes_data_base = {NULL,NULL,NULL,NULL,NULL};

/* fwd declaration */
static struct Note *CopyNote(struct Note *);

struct NoteTmp *CreateNoteTmp(symchar *lang, struct bracechar *bt,
                              void *varlist, int line)
{
  struct NoteTmp *nt;
  nt = ASC_NEW(struct NoteTmp);
  nt->lang = lang;
  nt->bt = bt;
  nt->vardata = varlist;
  nt->line = line;
  nt->next = NULL;
  return nt;
}

/* var data and bt assumed not our problem */
void DestroyNoteTmpList(struct NoteTmp *head)
{
  struct NoteTmp *old;
  while (head != NULL) {
    old = head;
    head = old->next;
    ascfree(old);
  }
}

struct NoteTmp *LinkNoteTmp(struct NoteTmp *new, struct NoteTmp *chain)
{
  if (chain == NULL) {
    return new;
  }
  if (new == NULL) {
    return chain;
  }
  new->next = chain;
  return new;
}

/* return pointer to database if it is in global list and not dead. */
static
struct data_base *FindDatabase(symchar *dbid)
{
  struct data_base *db;
  db = NDB.dblist;
  while (db != NULL) {
    if (db->dbid == dbid && !(db->dead)) {
      return db;
    }
    db = db->next;
  }
  return NULL;
}

/* delete dead entries from the dblist and frees them.
 * Stupid implementation.
 */
static
void ClearDeadDB(void)
{
  struct data_base *db, *odb;
  struct gl_list_t *keep;
  unsigned long len;
  if (NDB.dblist == NULL) {
    return;
  }
  keep = gl_create(3);
  if (keep == NULL) {return;}
  db = NDB.dblist;
  while (db != NULL) {
    if (!db->dead) {
      gl_append_ptr(keep,db);
      db = db->next;
    } else {
      odb = db;
      db = db->next;
      ascfree(odb);
    }
  }
  NDB.dblist = NULL;
  len = gl_length(keep);
  while (len > 0) {
    db = (struct data_base *)gl_fetch(keep,len);
    db->next = NDB.dblist;
    NDB.dblist = db;
    len--;
  }
  gl_destroy(keep);
}

/* this should be between the declarations and the execution in any call
 * that takes a dbid as a data source.
 * It defines a variable db, checks that dbid given is correct, and
 * returns failval if not. failval should be appropriate to context.
 * It's miniscully wasteful so that it can END in ; in use as
 * CHECKDB(1); Use CHECKDBV for void functions.
 */
#define CHECKDB(failval) struct data_base *db; \
  if (FindDatabase(dbid) == NULL) { return (failval); } db = FindDatabase(dbid)
#define CHECKDBV struct data_base *db; \
  if (FindDatabase(dbid) == NULL) { return; } db = FindDatabase(dbid)

/* init the database. return 0 if successful. NULL is not considered dbid. */
int InitNotesDatabase(symchar *dbid)
{
  int c;
  struct data_base *db;
  NDB.librarynote = AddSymbolL("Loaded Libraries",16);
  NDB.globalnote = AddSymbolL("All Known Files",15);
  NDB.inlinenote = AddSymbolL("inline",6);
  NDB.selfnote = AddSymbolL("SELF",4);
  if (dbid==NULL) {
    return 1;
  }
  db = ASC_NEW(struct data_base);
  if (db == NULL) {
    return 1;
  }
  /* init pool here if exists and NDB.dblist == NULL. */
  db->all = NULL;
  db->dbid = dbid;
  db->note_tokens = NULL;
  db->dead = 0;
  db->next = NDB.dblist;
  NDB.dblist = db;
  for (c=0;c < NTAB;c++) {
    db->typetab[c] = NULL;
    db->idtab[c] = NULL;
  }
  return 0;
}

/* bucket is assumed to contain a gl_list of notes matching key */
static
void NoteDestroyHash(struct note_bucket **tab, int size)
{
  struct note_bucket *old,*next;
  int c;
  for (c = 0; c < size; c++) {
    next = tab[c];
    tab[c] = NULL;
    while (next != NULL) {
      old = next;
      next = old->next;
      if (old->obj != NULL) {
        gl_iterate((struct gl_list_t *)old->obj,(DestroyFunc)DestroyNote);
        gl_destroy((struct gl_list_t *)old->obj);
      }
      NBFREE(old);
    }
  }
}

struct gl_list_t *ListNotesDatabases(void)
{
  struct data_base *db;
  struct gl_list_t *names;
  db = NDB.dblist;
  names = gl_create(2);
  if (names == NULL) {
    return NULL;
  }
  while (db != NULL) {
    if (!(db->dead)) {
      gl_append_ptr(names,(VOIDPTR)db->dbid);
    }
    db = db->next;
  }
  return names;
}

static
void RealDestroyDatabase(struct data_base *db, symchar *dbid)
{
  struct Note *old, *next;
  if (GNT != NULL) {
    ReleaseNoteData(dbid,(void *)0x1);
  }
  NoteDestroyHash(db->typetab,NTAB);
  NoteDestroyHash(db->idtab,NTAB);
  next = db->all;
  db->all = NULL;
  while (next != NULL) {
    old = next;
    next = old->next;
    DestroyNote(old);
  }
  db->dead = 1;
}

void DestroyNotesDatabase(symchar *dbid)
{
  struct data_base *db;

  if (dbid == (symchar *)0x1) {
    db = NDB.dblist;
    while (db != NULL) {
      RealDestroyDatabase(db,db->dbid);
      db = db->next;
    }
  } else {
    db = FindDatabase(dbid);
    if (db == NULL) {
      return;
    }
    RealDestroyDatabase(db,dbid);
  }
  ClearDeadDB();
}

/* clear any notes associated with the type named out of
 * database. Useful if replacing a type.
 */
void DestroyNotesOnType(symchar *dbid, symchar *typename)
{
  struct note_bucket *b, *prev = NULL;
  unsigned long len;
  struct gl_list_t *nl;
  struct Note *n,*p;
  long index;
  CHECKDBV;

  /* find type table data and dump it */
  index = PTRHASH(typename);
  b = db->typetab[index];
  if (b==NULL) {
    return;
  }
  if (b->key == typename) {
    db->typetab[index] = b->next;
  } else {
    while (b != NULL && b->key != typename) {
      prev = b;
      b = b->next;
    }
    if (b==NULL) {
      return; /* very odd */
    }
    prev->next = b->next;
  }
  gl_iterate((struct gl_list_t *)b->obj,(DestroyFunc)DestroyNote);
  gl_destroy((struct gl_list_t *)b->obj);
  NBFREE(b);

  /* run through other tables looking for type and kill those entries. */
  /* we eliminate the notes from the gl_lists, not table buckets, since
   * the only reason to delete a type instead of the whole database
   * is to replace it.
   */
  /* id table */
  for (index = 0; index < NTAB; index++) {
    b = db->idtab[index];
    while (b != NULL) {
      nl = (struct gl_list_t *)b->obj;
      len = gl_length(nl);
      for (; len > 0; len--) {
        n = (struct Note *)gl_fetch(nl,len);
        if (n->typename == typename) {
          gl_delete(nl,len,0);
          DestroyNote(n);
        }
      }
      b = b->next;
    }
  }

  /* run through ndb.all looking for type and kill those. */
  /* eat occurences of typename at head of list */
  while (db->all != NULL && db->all->typename == typename) {
    n = db->all;
    db->all = n->next;
    DestroyNote(n);
  }
  /* eat interior occurences. this loop will not change db->all. */
  if (db->all != NULL) {
    p = db->all;
    n = p->next;
    while (n != NULL) {
      if (n->typename == typename) {
        p->next = n->next;
        DestroyNote(n);
      } else {
        p = n;
      }
      n = p->next;
    }
  }
}

/* returns e if it is in the main list NDB.all. returns NULL if not. */
static
struct Note *FindNote(symchar *dbid, struct Note *e)
{
  struct Note *n;
  CHECKDB(NULL);
  n = db->all;
  while (n != NULL) {
    if (n == e) {
      return e;
    }
    n = n->next;
  }
  return NULL;
}

/* check list for worthiness and keep it, converting pointer to
 * void for user.
 */
void *HoldNoteData(symchar *dbid, struct gl_list_t *nl)
{
  CHECKDB(NULL);
  if (nl == NULL) {
    return NULL;
  }
  if (GNT == NULL) {
    GNT = gl_create(2);
    if (GNT == NULL) {
      return NULL;
    }
  }
  /* reject list if first note is bogus. mostly safe. */
  if (gl_length(nl) > 0L &&
      FindNote(dbid,(struct Note *)gl_fetch(nl,1)) == NULL) {
    return NULL;
  }
  gl_append_ptr(GNT,nl);
  return (void *)nl;
}

struct gl_list_t *HeldNotes(symchar *dbid, void *token)
{
  unsigned long pos;
  struct gl_list_t *nl;
  CHECKDB(NULL);
  if (token == NULL || GNT == NULL) {
    return NULL;
  }
  pos = gl_ptr_search(GNT,token,0);
  if (pos != 0) {
    nl = (struct gl_list_t *)gl_fetch(GNT,pos);
    return nl;
  }
  return NULL;
}

void ReleaseNoteData(symchar *dbid, void *token)
{
  unsigned long pos;
  struct gl_list_t *nl;
  CHECKDBV;
  if (token == NULL || GNT == NULL) {
    return;
  }
  if (token == (void *)0x1) {
    gl_iterate(GNT,(DestroyFunc)gl_destroy);
    gl_destroy(GNT);
    GNT = NULL;
    return;
  }
  pos = gl_ptr_search(GNT,token,0);
  if (pos != 0) {
    nl = (struct gl_list_t *)gl_fetch(GNT,pos);
    assert(nl==token);
    gl_destroy(nl);
    gl_delete(GNT,pos,0);
  }
}

/*
 */
struct gl_list_t *GetNotes(symchar *dbid,
                           symchar *type, symchar *lang,
                           symchar *id, symchar *method, enum NoteData nd)
{
  struct gl_list_t *result;
  struct Note *n;
  CHECKDB(NULL);
  /* unimplemented optimized searches */
  /* if type but not id or method given, grab its bucket and list and
   * match lang, nd as needed.
   */
  /* do something here */
  /*
   * If id or method given, grab its bucket and filter using the
   * other fields. filter sequence type, method, nd.
   */
  /* do something here */

  /* do the damned expensive thing here until the above get done */
  n= db->all;
  result = gl_create(40);
  if (result == NULL) {
    return NULL;
  }
  while (n != NULL) {
    /* nd_wild, NOTESWILD is the wildcard */
    if ((type == NOTESWILD || type == n->typename) &&
        (id == NOTESWILD || id == n->id) &&
        (method == NOTESWILD || method == n->method) &&
        (lang == NOTESWILD || lang == n->lang) &&
        (nd == nd_wild || nd == n->kind)) {
      gl_append_ptr(result,n);
    }
    n = n->next;
  }
  return result;
}

/* manual search of a list, since user may hand us a sorted but
 * not by ptr list.
 * returns 1 if list or data is NULL or if data is found in list.
 * Else returns 0;
 */
static
int inDataListOrWild(void *data, struct gl_list_t *list)
{
  unsigned long len;
  void *elt;
  if (list == NULL) {
    return 0;
  }
  if (list == NOTESWILDLIST) {
    return 1;
  }
  len = gl_length(list);
  for (; len > 0; len--) {
    elt = gl_fetch(list,len);
    if (elt == data || (enum NoteData)elt == nd_wild) {
      return 1;
    }
  }
  return 0;
}

/* manual search of a list, since user may hand us a sorted but
 * not by ptr list.
 * returns 1 if list or data is NULL or if data is found in list.
 * Else returns 0;
 */
static
int inDataListOrNull(void *data, struct gl_list_t *list)
{
  unsigned long len;
  void *elt;
  if (list == NULL) {
    return 0;
  }
  if (list == NOTESWILDLIST) {
    return 1;
  }
  len = gl_length(list);
  for (; len > 0; len--) {
    elt = gl_fetch(list,len);
    if (elt == data || elt == NOTESWILD) {
      return 1;
    }
  }
  return 0;
}
/*
 * GetNotesList(typelist,langlist,idlist,ndlist);
 * Returns the list of notes matching any combination of the
 * keys stored in the gl_lists given. So, for example,
 * if you want the notes on id in typename or any ancestor,
 * cookup up the list of ancestor type names, stick id in idlist,
 * and pass NULL,nd_empty for the other two arguments.
 */
struct gl_list_t *GetNotesList(symchar *dbid,
                               struct gl_list_t *types,
                               struct gl_list_t *langs,
                               struct gl_list_t *ids,
                               struct gl_list_t *methods,
                               struct gl_list_t *nds)
{
  struct gl_list_t *result;
  struct Note *n;
  CHECKDB(NULL);

  /* unimplemented optimized searches */
  /* if type but not id or method given, grab its bucket and list and
   * match lang, nd as needed.
   */
  /* do something here */
  /*
   * If id or method given, grab its bucket and filter using the
   * other fields. filter sequence type, method, nd.
   */
  /* do something here */

  /* do the damned expensive thing here until the above get done */
  n= db->all;
  result = gl_create(40);
  if (result == NULL) {
    return NULL;
  }
  while (n != NULL) {
    if (inDataListOrNull((void *)(n->typename),types) &&
        inDataListOrNull((void *)(n->id),ids) &&
        inDataListOrNull((void *)(n->lang),langs) &&
        inDataListOrNull((void *)(n->method),methods) &&
        inDataListOrWild((void *)(n->kind),nds)) {
      gl_append_ptr(result,n);
    }
    n = n->next;
  }
  return result;
}

struct gl_list_t *GetExactNote(symchar *dbid, struct Note *e)
{
  struct gl_list_t *result;
  struct Note *n;

  result = gl_create(1);
  if (result == NULL) {
    return NULL;
  }
  n = FindNote(dbid,e);
  if (n != NULL) {
    gl_append_ptr(result,e);
  }
  return result;
}

symchar *GetNoteId(struct Note *n)
{
  if (n != NULL)  {
    return n->id;
  }
  return NULL;
}

symchar *GetNoteMethod(struct Note *n)
{
  if (n != NULL)  {
    return n->method;
  }
  return NULL;
}

symchar *GetNoteType(struct Note *n)
{
  if (n != NULL)  {
    return n->typename;
  }
  return NULL;
}

symchar *GetNoteLanguage(struct Note *n)
{
  if (n != NULL)  {
    return n->lang;
  }
  return NULL;
}

CONST char *GetNoteFilename(struct Note *n)
{
  if (n != NULL)  {
    return n->filename;
  }
  return NULL;
}

int GetNoteLineNum(struct Note *n)
{
  if (n != NULL)  {
    return n->line;
  }
  return 0;
}

struct bracechar *GetNoteText(struct Note *n)
{
  if (n != NULL)  {
    return n->t;
  }
  return NULL;
}

enum NoteData GetNoteEnum(struct Note *n)
{
  if (n != NULL)  {
    return n->kind;
  }
  return nd_empty;
}

void *GetNoteData(struct Note *n, enum NoteData nd)
{
  if (n != NULL && nd == n->kind)  {
    return n->data;
  }
  return NULL;
}

struct gl_list_t *GetNotesAllLanguages(symchar *dbid)
{
  struct Note *n;
  struct gl_list_t *result;
  CHECKDB(NULL);
  result = gl_create(40);
  if (result == NULL) {
    return NULL;
  }
  n= db->all;
  while (n != NULL) {
    /* null, empty is the wildcard */
    if (n->lang != NULL && gl_ptr_search(result,n->lang,1)==0) {
      /* if you see lang repeats, change 1 to 0 above */
      gl_insert_sorted(result,(void *)n->lang,(CmpFunc)CmpPtrs);
    }
    n = n->next;
  }
  return result;

}

symchar *GlobalNote(void)
{
  return NDB.globalnote;
}

symchar *LibraryNote(void)
{
  return NDB.librarynote;
}

symchar *InlineNote(void)
{
  return NDB.inlinenote;
}

symchar *SelfNote(void)
{
  return NDB.selfnote;
}

/* creates and inserts bucket, returning pointer.
 * return NULL if malloc fail.
 */
static
struct note_bucket *AddEntry(struct note_bucket **tab, symchar *key, void *obj)
{
  long index;
  struct note_bucket *new;

  index = PTRHASH(key);
  new = NBALLOC;
  if (new==NULL) {
    return NULL;
  }
  new->key = key;
  new->obj = obj;
  new->next = tab[index];
  tab[index] = new;
  return new;
}

static
struct note_bucket *FindEntry(struct note_bucket **tab, symchar *key)
{
  struct note_bucket *b;
  long index;
  index = PTRHASH(key);
  b = tab[index];
  while (b != NULL && b->key != key) {
    b = b->next;
  }
  return b;
}


/* add finished note to tables.
 * idtable buckets match both methods and ids.
 * typetable matches typename.
 */
static
int InsertNote(struct data_base *db, struct Note *n)
{
  struct note_bucket *b;
  struct gl_list_t *matches;

  if (n == NULL) {
    return 0;
  }

  /* insert in master list */
  n->next = db->all;
  db->all = n;

  /* insert in id in id table. buckets in id typically short lists */
  if (n->id != NULL) {
    b = FindEntry(db->idtab,n->id);
    if (b==NULL) {
      matches = gl_create(2);
      if (matches == NULL) {
        return 1;
      }
      b = AddEntry(db->idtab,n->id,matches);
    }
    if (b == NULL) {
      return 1;
    }
    matches = (struct gl_list_t *)(b->obj);
    ReferenceNote(n);
    gl_append_ptr(matches,(VOIDPTR)n);
  }

  /* insert in method in id table. buckets in id typically short lists */
  if (n->method != NULL) {
    b = FindEntry(db->idtab,n->method);
    if (b==NULL) {
      matches = gl_create(2);
      if (matches == NULL) {
        return 1;
      }
      b = AddEntry(db->idtab,n->method,matches);
    }
    if (b == NULL) {
      return 1;
    }
    matches = (struct gl_list_t *)(b->obj);
    ReferenceNote(n);
    gl_append_ptr(matches,(VOIDPTR)n);
  }

  /* insert in type table. buckets in type contain fairly big lists. */
  if (n->typename != NULL) {
    b = FindEntry(db->typetab,n->typename);
    if (b==NULL) {
      matches = gl_create(5);
      if (matches == NULL) {
        return 1;
      }
      b = AddEntry(db->typetab,n->typename,matches);
    }
    if (b == NULL) {
      return 1;
    }
    matches = (struct gl_list_t *)(b->obj);
    ReferenceNote(n);
    gl_append_ptr(matches,(VOIDPTR)n);
  }

  return 0;
}

/* Add a note to the database.
 * CommitNote(note);
 */
int CommitNote(symchar *dbid, struct Note *n)
{
  CONST struct VariableList *vl;
  CONST struct Name *name;
  struct Note *new;
  int errcnt=0;
  symchar *id;
  CHECKDB(1);

  assert(n->db == NULL);
  n->db = db;
  if (n->kind == nd_vlist) {
    vl = (struct VariableList *)(n->data);
    while (vl != NULL) {
      name = NamePointer(vl);
      id = SimpleNameIdPtr(name);
      new = CopyNote(n);
      if (id == NULL) {
        new->kind = nd_name;
        new->data = (VOIDPTR)name;
      } else {
        new->id = id;
      }
      errcnt += InsertNote(db,new);
      vl = NextVariableNode(vl);
    }
  }
  errcnt += InsertNote(db,n);
  return errcnt;
}

struct Note *CreateNote(symchar *type, symchar *lang,
                        symchar *id, symchar *method, CONST char *file,
                        struct bracechar *t, int line,
                        VOIDPTR data, enum NoteData nd)
{
  struct Note *n;
  if (t==NULL) {
    return NULL;
  }
  n = NPALLOC;
  n->typename = type;
  n->lang = lang;
  n->db = NULL;
  n->id = id;
  n->method = method;
  n->filename = file;
  n->t = t;
  n->line = line;
  n->refcount = 0;
  switch (nd) {
  case nd_name:
  case nd_vlist:
  case nd_module:
  case nd_anonymous:
    n->data = data;
    n->kind = nd;
    break;
  case nd_empty:
  case nd_wild:
  default:
    n->data = NULL;
    n->kind = nd_empty;
  }
  return n;
}

/* does not copy vlist data and nd, if found.
 * OTHERWISE copies everything about the note.
 * The text t is copied by reference.
 * this cannot be used to move a note between databases.
 */
static
struct Note *CopyNote(struct Note *old)
{
  struct Note *n;
  if (old==NULL) {
    return NULL;
  }
  n = NPALLOC;
  n->typename = old->typename;
  n->lang = old->lang;
  n->id = old->id;
  n->db = old->db;
  n->method = old->method;
  n->filename = old->filename;
  n->t = CopyBraceChar(old->t);
  n->line = old->line;
  n->refcount = 0;
  switch (old->kind) {
  case nd_name:
  case nd_module:
  case nd_anonymous:
    n->data = old->data;
    n->kind = old->kind;
    break;
  case nd_vlist: /* original holder owns this */
  case nd_empty:
  default:
    n->data = NULL;
    n->kind = nd_empty;
  }
  return n;
}

void DestroyNote(struct Note *n)
{
  if (n==NULL) {
    return;
  }
  if (!(n->refcount)) {
    DestroyBraceChar(n->t); /* bracechar plays reference games itself */
    switch (n->kind) {
    case nd_vlist:
      DestroyVariableList((struct VariableList *)n->data);
      break;
    case nd_wild:
    case nd_empty:
    case nd_name:
    case nd_module:
    case nd_anonymous:
      break;
    }
    NPFREE(n);
  } else {
    (n->refcount)--;
  }
}

struct NoteEngine {
  int enginekey;
  void *reinterp;
  NEInitFunc recompile;
  NECompareFunc reexec;
};
#define ENGINEMAGIC 345987607

struct gl_list_t *GetMatchingNotes(symchar *dbid, char *pattern,void *token,
                                   struct NoteEngine *engine)
{
  struct gl_list_t *result;
  unsigned long len;
  struct gl_list_t *oldlist;
  struct Note *n;
  int trash = 0;
  void *regexp;
  char *string;
  CHECKDB(NULL);

  if (pattern == (char *)NULL || strlen(pattern) == 0 ||
      engine == (struct NoteEngine *)NULL ||
      engine->recompile == (NEInitFunc)NULL ||
      engine->reexec == (NECompareFunc)NULL) {
    return NULL;
  }
  oldlist = HeldNotes(dbid,token);
  if (token != NULL && oldlist == NULL) {
    return NULL;
  }
  if (oldlist == NULL) {
    oldlist = GetNotes(dbid,NOTESWILD,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
    trash = 1;
  }
  if (oldlist == NULL) {
    return NULL;
  }
  if (gl_length(oldlist)==0) {
    if (trash) {
      gl_destroy(oldlist);
    }
    return NULL;
  }
  result = gl_create(10);
  regexp = engine->recompile(engine->reinterp,pattern);
  if (regexp == NULL) {
    if (trash) {
      gl_destroy(oldlist);
    }
    return result;
  }
  len = gl_length(oldlist);
  for (; len > 0; len--) {
    n = (struct Note *)gl_fetch(oldlist,len);
    if (n == NULL || GetNoteEnum(n) == nd_vlist) {
      continue;
    }
    string = (char *)BCS(GetNoteText(n));
    if (string == NULL) {
      continue;
    }
    switch (engine->reexec(engine->reinterp,regexp,string,string)) {
    case 1:
      gl_append_ptr(result,n);
      break;
    case 0:
    case -1:
    default:
      break;
    }
  }
  if (trash) {
    gl_destroy(oldlist);
  }
  return result;
}

struct NoteEngine *NotesCreateEngine(void *ned,
                                     NEInitFunc neif,
                                     NECompareFunc necf)
{
  struct NoteEngine *ne;
  ne = ASC_NEW(struct NoteEngine);
  if (neif == (NEInitFunc)NULL || necf == (NECompareFunc)NULL) {
    return (struct NoteEngine *)NULL;
  }
  if (ne == (struct NoteEngine *)NULL) {
    return (struct NoteEngine *)NULL;
  }
  ne->enginekey = ENGINEMAGIC;
  ne->reinterp = ned;
  ne->recompile = neif;
  ne->reexec = necf;
  return ne;
}

void NotesDestroyEngine(struct NoteEngine *e)
{
  if (e != NULL && e->enginekey == ENGINEMAGIC ) {
    e->enginekey = -1;
    ascfree(e);
  }
}
