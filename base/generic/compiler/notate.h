/** 
 *  notate.h
 *  By Ben Allan
 *  4/98
 *  Part of ASCEND
 *  Version: $Revision: 1.3 $
 *  Version control file: $RCSfile: notate.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:42 $
 *  Last modified by: $Author: mthomas $
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
/** This file defines and manages a little (we hope)
 * database system for storing NOTES in-core in a variety
 * of quickly accessible ways.
 * If scale gets to be an issue, this could become a wrapper
 * for a real database someday.
 * This implementation is not optimized for anything -- the
 * intent here is to learn if it can be done at all.
 */

/** requires:
 * #include "general/list.h"
 * #include "compiler/compiler.h"
 * #include "compiler/braced.h"
 */

#ifndef __NOTATE_H_SEEN__
#define __NOTATE_H_SEEN__

/** 
 * For database queries, we need a wildcard pointer value.
 * This is that wildcard. It is in the zero page,
 * odd (which is impossible for a symchar) and outside
 * the range of the NoteData enum.
 */
#define NOTESWILD ((symchar *)0x11)
#define NOTESWILDLIST ((struct gl_list_t *)0x11)

/** 
 * a void pointer may be stored with a note.
 * This tells what the void pointer is.
 * We don't want to crap up this header with half the
 * compiler's includes because interface clients shouldn't
 * know.
 */
enum NoteData {
  nd_wild,	/** matches any in notes queries */
  nd_empty,	/** pointer meaningless, should be NULL. matches only empty */
  nd_name,	/** struct Name * assumed someone else's to destroy */
  nd_vlist,	/** struct VariableList * ours to destroy */
  nd_module,	/** struct module_t * not ours, obviously */
  nd_anonymous  /** future expansion as needed */
};

/** contents none of your business. our persistent record. */
struct Note;

/** wrapper for a regular expression matching engine. */
struct NoteEngine;

struct NoteTmp {
  /** for parser use only. some fields (lang) may not be reset by client */
  /** do not store these anywhere permanently as they are not persistent */
  symchar *lang;
  struct bracechar *bt;
  void *vardata;
  int line;
  struct NoteTmp *next;
};

/*** tmp notes functions ***/

/** create a notetmp for parsing NOTES statements. need this in order to
 * capture multiple fvarlist/bracedtext pairs for a single language key.
 */
extern struct NoteTmp *CreateNoteTmp(symchar *, struct bracechar *,
                                     void *, int);

/** add a notetmp to a lifo (if you follow next) list. */
extern struct NoteTmp *LinkNoteTmp(struct NoteTmp *,struct NoteTmp *);

/** destroy a chain of struct NoteTmp following the next pointers.
 * The bt and vardata fields contained (if any) are not destroyed.
 */
extern void DestroyNoteTmpList(struct NoteTmp *);

/*** regular notes functions ***/

/** init the database. return 0 if successful.
 * input is the symchar by which the database
 * will be addressed.
 * The very first call to this function should be made with
 * NULL and merely initializes internal structures without
 * creating something according to dbid. That call always
 * returns 1.
 * err = InitNotesDatabase(dbid);
 */
extern int InitNotesDatabase(symchar *);

/** 
 * list  = ListNotesDatabases();
 * returns a gl_list containing symchar * of names (dbid)
 * of databases currently in existence.
 */
extern struct gl_list_t *ListNotesDatabases(void);

/** destroy the database, and necessarily all tokens being held.
 * if dbid is 0x1, destroys all databases.
 * DestroyNotesDatabase(dbid);
 */
extern void DestroyNotesDatabase(symchar *);

/** clear any notes associated with the type named out of
 * database. Useful if replacing a type.
 * DestroyNotesOnType(dbid,typename);
 */
extern void DestroyNotesOnType(symchar *, symchar *);

/** 
 * list = GetNotes(dbid,typename,language,id,method,nd);
 *   returns a list of notes matching the keys specified.
 *   NOTESWILD key values are wildcards.
 * This list returned is yours to destroy. The contents of the
 * list are struct Note * and  are Not yours to destroy.
 *
 * E.G.:
 * GetNotes(dbid,typename,NOTESWILD,NOTESWILD,NOTESWILD,nd_wild);
 *   returns a list of all notes on a type or any part of the type.
 *   This may contain apparent duplicates depending the Commit
 *   optimizations that are performed.
 * GetNotes(dbid,typename,language,NOTESWILD,NOTESWILD,nd_empty);
 *   returns a list of all notes on a type in the given language
 *   where the data slot is empty.
 * GetNotes(dbid,typename,NOTESWILD,id,NULL,nd_empty);
 *   returns a list of all notes on a type about id (including SELF)
 *   in any language where method is NULL and no data is stored.
 * GetNotes(dbid,typename,language,id,NULL,nd_empty);
 *   returns a list of all notes on a type not in a method about id in language.
 * GetNotes(dbid,NOTESWILD,NOTESWILD,id,NOTESWILD,nd_wild);
 *   returns a list of all notes about id in any context, any language.
 */
extern struct gl_list_t *GetNotes(symchar *, symchar *, symchar *,
                                  symchar *, symchar *, enum NoteData);

/** 
 * Given a pointer (alleged) to a note, checks that the
 * pointer is valid and returns a gl_list containing the
 * pointer. If the list is empty, the pointer is not valid.
 * This kind of boneheaded function is necessary for handling
 * scripted interfaces that can shove in garbage.
 * GetExactNote(dbid,n);
 */
extern struct gl_list_t *GetExactNote(symchar *,struct Note *);

/** 
 * GetNotesList(dbid,typelist,langlist,idlist,methodlist,ndlist);
 * Returns the list of notes matching any combination of the
 * keys stored in the gl_lists given. So, for example,
 * if you want the notes on id in typename or any ancestor,
 * cookup up the list of ancestor type names, stick id in idlist,
 * and pass NOTESWILD,nd_wild for the other two arguments.
 */
extern struct gl_list_t *GetNotesList(symchar *,
                                      struct gl_list_t *, struct gl_list_t *,
                                      struct gl_list_t *, struct gl_list_t *,
                                      struct gl_list_t *);

/** Return id of note. id is normally a child name or SELF.
 * might be NULL.
 */
extern symchar *GetNoteId(struct Note *);

/** Return method of note. method is normally NULL unless note
 * is in scope of a method. a note with nonnull method might
 * also have a nonnull id.
 * might be NULL.
 */
extern symchar *GetNoteMethod(struct Note *);

/** Return typename of note. typename is normally something in the library.
 * might be NULL.
 */
extern symchar *GetNoteType(struct Note *);

/** Return language of note. Might be NULL.
 */
extern symchar *GetNoteLanguage(struct Note *);

/** Return best module name of note. possibly NULL.
 * name is string from a symchar.
 */
extern CONST char *GetNoteFilename(struct Note *);

/** Return line number of note. possibly -1.
 */
extern int GetNoteLineNum(struct Note *);

/** return the text string of a note. it is ours.
 */
extern struct bracechar *GetNoteText(struct Note *);

/** return the enum NoteData.
 */
extern enum NoteData GetNoteEnum(struct Note *);

/** return the data of a note if it matches the enum given, or NULL.
 */
extern void *GetNoteData(struct Note *,enum NoteData);

/** 
 * token = HoldNoteData(dbid,nl);
 * Puts the list of note pointers on an internal tracking list.
 * token returned may be passed to Note functions which take a
 * token. Returns NULL on memory failure or on list nl not appearing
 * to contain notes.
 * The notes list must be from the same database.
 */
extern void *HoldNoteData(symchar *,struct gl_list_t *);

/** 
 * list = HeldNotes(dbid,token);
 * Returns NULL if token given is invalid in dbid.
 */
extern struct gl_list_t *HeldNotes(symchar *,void *);

/** 
 * ReleaseNoteData(dbid,token);
 * Removes the list of note pointers associated with token
 * from internal tracking list.
 * Calling with a 0x1 token releases all held lists and all
 * existing tokens become invalid.
 * Calling with bogus token is ok.
 */
extern void ReleaseNoteData(symchar *,void *);

/** 
 * return the languages found in any committed note as a list of symchars.
 * GetNotesAllLanguages(dbid);
 */
extern struct gl_list_t *GetNotesAllLanguages(symchar *);

/** 
 * The 4 following calls do not work until after at least 1 call of
 * InitNotesDatabase(NULL) has been done.
 */

/** returns the 'Loaded Libraries' symchar, which is useful for the parser. */
extern symchar *LibraryNote(void);

/** returns the 'All Known Files' symchar, which is useful for the parser. */
extern symchar *GlobalNote(void);

/** returns the 'inline' symchar, which is useful for the parser. */
extern symchar *InlineNote(void);

/** returns the 'SELF' symchar, which is useful for the parser. */
extern symchar *SelfNote(void);

/** Add a note to the database.
 * CommitNote(dbid,note);
 * Notes with vlists may be internally replicated to optimize
 * future queries.
 */
extern int CommitNote(symchar *, struct Note *);

/** 
 * Create a note. Destroy the note unless you commit it to a
 * database.
 * n = CreateNote(type,lang,id,method,file,btext,line,data,nd);
 */
extern struct Note *CreateNote(symchar *, symchar *, symchar *, symchar *,
                               CONST char *, struct bracechar *, int,
                               VOIDPTR, enum NoteData);

/** Destroys the note given, subject to reference counting.
 * Do not externally destroy a note once it has been committed to
 * the database.
 */
extern void DestroyNote(struct Note *);

/******** regular expression matching madness for the database ******/
/******** regular expression matching madness for the database ******/

/** 
 * nl = GetMatchingNotes(dbid, pattern, token, engine);
 * Returns a list of notes whose text (or name list if a vlist note in future)
 * matches the pattern given according to the engine given.
 * If token is NULL, entire database is given, OTHERWISE token
 * must be a held note list.
 * Engine is the non-NULL return from a call to NotesCreateEngine.
 * The engine NEdata, about which you know more than we,
 * should be consulted for errors if the return is NULL.
 */
extern struct gl_list_t *GetMatchingNotes(symchar *, char *,
                                          void *, struct NoteEngine *);

/** 
 * An NEInitFunc takes whatever data was specified to the engine
 * and a string to be matched and prepares the engine for one or
 * more matches against that pattern.
 * NEInitFunc should return NULL if it fails to initialize,
 * else it should return a pointer to be used in processing strings.
 * Tcl_RegExpCompile is an example of this function class.
 */
typedef void *(*NEInitFunc)(void *, /*NEdata*/
                            char * /*pattern*/);

/** 
 * match = NECompareFunc(nedata,neinitfuncreturn,string,string);
 * An NECompareFunc returns -1 for error, 0 for no match, 1 for match.
 * Tcl_RegExpExec is an example of this function class.
 */
typedef int (*NECompareFunc)(void *, /*NEdata*/
                             void *, /** return from NEInitFunc */
                             char *, /** substring to test for match */
                             char *  /** beginning of string containing
                                      * substring (^ matches)
                                      */
                            );

/** 
 * This is a wrapper to keep things independent of anyone in particular's
 * regular expression package.
 * engine = NotesCreateEngine(NEdata,NEInit,NECompare);
 */
struct NoteEngine *NotesCreateEngine(void *,NEInitFunc,NECompareFunc);

/** 
 * NotesDestroyEngine(engine);
 * Destroys a previously returned engine.
 */
extern void NotesDestroyEngine(struct NoteEngine *);

#endif /** __NOTATE_H_SEEN__ */
