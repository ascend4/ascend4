/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
	Copyright (C) 2006 Carnegie Mellon University

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
*//**
	@file
	This file defines and manages a little (we hope)
	database system for storing NOTES in-core in a variety
	of quickly accessible ways.

	If scale gets to be an issue, this could become a wrapper
	for a real database someday.

	This implementation is not optimized for anything -- the
	intent here is to learn if it can be done at all.
*//*
	By Ben Allan 4/98
	Last in CVS:$Revision: 1.3 $ $Date: 1998/06/16 16:38:42 $ $Author: mthomas $
*/

#ifndef ASC_NOTATE_H
#define ASC_NOTATE_H

#include <utilities/ascConfig.h>
#include <general/list.h>
#include "compiler.h"
#include "braced.h"

/*------------------------------------------------------------------------------
  MACROS AND DATA STRUCTURES
*/

/**
 * A wildcard pointer value for database queries.
 * It is in the zero page, odd (which is impossible for
 * a symchar) and outside the range of the NoteData enum.
 */
#define NOTESWILD ((symchar *)0x11)
#define NOTESWILDLIST ((struct gl_list_t *)0x11)
/**<
 * A wildcard list pointer value for database queries.
 * It is in the zero page, odd (which is impossible for
 * a symchar) and outside the range of the NoteData enum.
 */

/**
 * Type of the void pointer which may be stored with a note.
 * We don't want to crap up this header with half the
 * compiler's includes because interface clients shouldn't
 * know.
 */
enum NoteData {
  nd_wild,      /**< Matches any in notes queries. */
  nd_empty,     /**< Pointer meaningless, should be NULL. matches only empty. */
  nd_name,      /**< struct Name * assumed someone else's to destroy. */
  nd_vlist,     /**< struct VariableList * ours to destroy. */
  nd_module,    /**< struct module_t * not ours, obviously. */
  nd_anonymous  /**< Future expansion as needed. */
};

/* contents none of your business. our persistent record. */
struct Note;

/* wrapper for a regular expression matching engine. */
struct NoteEngine;

/** Temporary for parser use only. Some fields (lang) may not be reset by client.
 *  Do not store these anywhere permanently as they are not persistent.
 */
struct NoteTmp {
  symchar *lang;
  struct bracechar *bt;
  void *vardata;
  int line;
  struct NoteTmp *next;
};

/*------------------------------------------------------------------------------
   TEMPORARY NOTES
*/

/**
 * Create a notetmp for parsing NOTES statements. need this in order to
 * capture multiple fvarlist/bracedtext pairs for a single language key.
 */
extern struct NoteTmp *CreateNoteTmp(symchar *lang,
                                     struct bracechar *bt,
                                     void *varlist,
                                     int line);

/** Add a notetmp to a lifo (if you follow next) list. */
extern struct NoteTmp *LinkNoteTmp(struct NoteTmp *new_thing,
                                   struct NoteTmp *chain);

/**
 * Destroy a chain of struct NoteTmp following the next pointers.
 * The bt and vardata fields contained (if any) are not destroyed.
 */
extern void DestroyNoteTmpList(struct NoteTmp *head);

/*------------------------------------------------------------------------------
  REGULAR (PERSISTENT) NOTES
*/

/**
 * Initialize the notes database. Returns 0 if successful.
 * Input is the symchar by which the database will be addressed.
 * The very first call to this function should be made with
 * NULL and merely initializes internal structures without
 * creating something according to dbid. That call always
 * returns 1.
 */
ASC_DLLSPEC int InitNotesDatabase(symchar *dbid);

/**
 * Returns a gl_list containing symchar * of names (dbid)
 * of databases currently in existence.
 */
ASC_DLLSPEC struct gl_list_t *ListNotesDatabases(void);

/**
 * Destroy the database, and necessarily all tokens being held.
 * if dbid is 0x1, destroys all databases.
 */
ASC_DLLSPEC void DestroyNotesDatabase(symchar *dbid);

/**
 * Clear any notes associated with the type named out of
 * database. Useful if replacing a type.
 */
extern void DestroyNotesOnType(symchar *dbid, symchar *type_name);

/**
 * Returns a list of notes matching the keys specified.
 * NOTESWILD key values are wildcards.
 * This list returned is yours to destroy. The contents of the
 * list are struct Note * and  are Not yours to destroy.
 *
 * Examples:
 * <pre>
 * GetNotes(dbid,typename,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
 *   returns a list of all notes on a type or any part of the type.
 *   This may contain apparent duplicates depending the Commit
 *   optimizations that are performed.
 *
 * GetNotes(dbid,typename,language,NOTESWILD,NOTESWILD,nd_empty);
 *   returns a list of all notes on a type in the given language
 *   where the data slot is empty.
 *
 * GetNotes(dbid,typename,NOTESWILD,id,NULL,nd_empty);
 *   returns a list of all notes on a type about id (including SELF)
 *   in any language where method is NULL and no data is stored.
 *
 * GetNotes(dbid,typename,language,id,NULL,nd_empty);
 *   returns a list of all notes on a type not in a method about id in language.
 *
 * GetNotes(dbid,NOTESWILD,NOTESWILD,id,NOTESWILD,nd_wild);
 *   returns a list of all notes about id in any context, any language.
 * </pre>
 */
ASC_DLLSPEC struct gl_list_t *GetNotes(symchar *dbid,
                                  symchar *type,
                                  symchar *lang,
                                  symchar *id,
                                  symchar *method,
                                  enum NoteData nd);

/**
 * Given a pointer (alleged) to a note, checks that the
 * pointer is valid and returns a gl_list containing the
 * pointer. If the list is empty, the pointer is not valid.
 * This kind of boneheaded function is necessary for handling
 * scripted interfaces that can shove in garbage.
 * GetExactNote(dbid,n);
 */
ASC_DLLSPEC struct gl_list_t *GetExactNote(symchar *dbid, struct Note *n);

/**
 * Returns the list of notes matching any combination of the
 * keys stored in the gl_lists given. So, for example,
 * if you want the notes on id in typename or any ancestor,
 * cookup up the list of ancestor type names, stick id in idlist,
 * and pass NOTESWILD,nd_wild for the other two arguments.
 */
ASC_DLLSPEC struct gl_list_t *GetNotesList(symchar *dbid,
                                      struct gl_list_t *types,
                                      struct gl_list_t *langs,
                                      struct gl_list_t *ids,
                                      struct gl_list_t *methods,
                                      struct gl_list_t *nds);

/**
 * Return id of note. id is normally a child name or SELF.
 * Might be NULL.
 */
ASC_DLLSPEC symchar *GetNoteId(struct Note *n);

/**
 * Return method of note.  Method is normally NULL unless note
 * is in scope of a method.  A note with nonNULL method might
 * also have a nonnull id.
 * Might be NULL.
 */
ASC_DLLSPEC symchar *GetNoteMethod(struct Note *n);

/**
 * Return typename of note.
 * typename is normally something in the library.
 * Might be NULL.
 */
ASC_DLLSPEC symchar *GetNoteType(struct Note *n);

/** Return language of note.  Might be NULL. */
ASC_DLLSPEC symchar *GetNoteLanguage(struct Note *n);

/**
 * Return best module name of note.
 * Might be NULL.  Name is string from a symchar.
 */
ASC_DLLSPEC CONST char *GetNoteFilename(struct Note *n);

/** Return line number of note. possibly -1. */
ASC_DLLSPEC int GetNoteLineNum(struct Note *n);

/** Return the text string of a note.  It is ours. */
ASC_DLLSPEC struct bracechar *GetNoteText(struct Note *n);

/** Return the enum NoteData. */
ASC_DLLSPEC enum NoteData GetNoteEnum(struct Note *n);

/** Return the data of a note if it matches the enum given, or NULL. */
ASC_DLLSPEC void *GetNoteData(struct Note *n, enum NoteData nd);

/**
 * Puts the list of note pointers on an internal tracking list.
 * token returned may be passed to Note functions which take a
 * token. Returns NULL on memory failure or on list nl not appearing
 * to contain notes.
 * The notes list must be from the same database.
 */
ASC_DLLSPEC void *HoldNoteData(symchar *dbid, struct gl_list_t *nl);

/**
 * Returns NULL if token given is invalid in dbid.
 */
ASC_DLLSPEC struct gl_list_t *HeldNotes(symchar *dbid, void *token);

/**
 * Removes the list of note pointers associated with token
 * from internal tracking list.
 * Calling with a 0x1 token releases all held lists and all
 * existing tokens become invalid.
 * Calling with bogus token is ok.
 */
ASC_DLLSPEC void ReleaseNoteData(symchar *dbid, void *token);

/**
 * Return the languages found in any committed note as a list of symchars.
 */
ASC_DLLSPEC struct gl_list_t *GetNotesAllLanguages(symchar *dbid);

/*
 * The 4 following calls do not work until after at least 1 call of
 * InitNotesDatabase(NULL) has been done.
 */

/** Returns the 'Loaded Libraries' symchar, which is useful for the parser. */
ASC_DLLSPEC symchar*LibraryNote(void);

/** Returns the 'All Known Files' symchar, which is useful for the parser. */
extern symchar *GlobalNote(void);

/** Returns the 'inline' symchar, which is useful for the parser. */
extern symchar *InlineNote(void);

/** Returns the 'SELF' symchar, which is useful for the parser. */
extern symchar *SelfNote(void);

/**
 * Add a note to the database.
 * Notes with vlists may be internally replicated to optimize
 * future queries.
 */
extern int CommitNote(symchar *dbid, struct Note *note);

/**
 * Create a note. Destroy the note unless you commit it to a
 * database.
 */
extern struct Note *CreateNote(symchar *type,
                               symchar *lang,
                               symchar *id,
                               symchar *method,
                               CONST char *file,
                               struct bracechar *btext,
                               int line,
                               VOIDPTR data,
                               enum NoteData nd);

/**
 * Destroys the note given, subject to reference counting.
 * Do not externally destroy a note once it has been committed to
 * the database.
 */
extern void DestroyNote(struct Note *n);

/*------------------------------------------------------------------------------
  SEARCHING FUNCTIONS (LINK TO A REGULAR EXPRESSION ENGINE)
*/

/**
 * Returns a list of notes whose text (or name list if a vlist note in future)
 * matches the pattern given according to the engine given.
 * If token is NULL, entire database is given, OTHERWISE token
 * must be a held note list.
 * Engine is the non-NULL return from a call to NotesCreateEngine.
 * The engine NEdata, about which you know more than we,
 * should be consulted for errors if the return is NULL.
 */
ASC_DLLSPEC struct gl_list_t *GetMatchingNotes(symchar *dbid,
                                          char *pattern,
                                          void *token,
                                          struct NoteEngine *engine);

/**
 * An NEInitFunc takes whatever data was specified to the engine
 * and a string to be matched and prepares the engine for one or
 * more matches against that pattern.
 * NEInitFunc should return NULL if it fails to initialize,
 * else it should return a pointer to be used in processing strings.
 * Tcl_RegExpCompile is an example of this function class.
 */
typedef void *(*NEInitFunc)(void *, /* NEdata */
                            char *  /* pattern */);

/**
 * An NECompareFunc returns -1 for error, 0 for no match, 1 for match.
 * Tcl_RegExpExec is an example of this function class.
 */
typedef int (*NECompareFunc)(void *, /* NEdata */
                             void *, /* return from NEInitFunc */
                             char *, /* substring to test for match */
                             char *  /* beginning of string containing
                                      * substring (^ matches)
                                      */
                            );

/**
 * This is a wrapper to keep things independent of anyone in particular's
 * regular expression package.
 */
ASC_DLLSPEC struct NoteEngine *NotesCreateEngine(void *NEdata,
                                   NEInitFunc NEInit,
                                   NECompareFunc NECompare);

/**
 * Destroys a previously returned engine.
 */
ASC_DLLSPEC void NotesDestroyEngine(struct NoteEngine *engine);

#endif /* ASC_NOTATE_H */

