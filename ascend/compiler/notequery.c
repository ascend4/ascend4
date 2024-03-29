/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Query function(s) for the NOTES database in notate.h
*/

#include "notequery.h"
#include "notate.h"
#include "symtab.h"
#include <ascend/utilities/error.h>

#ifdef NOTEQUERY_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

const char *notes_get_for_variable(symchar *dbid
	, const struct TypeDescription *t
	, const symchar *varname
	, const symchar *lang
){
	struct gl_list_t *noteslist = NULL;

	struct gl_list_t *types = GetAncestorNames(t);
	struct gl_list_t *langs = gl_create(1);
	struct gl_list_t *ids = gl_create(1);

	int i;
	for(i=1; i<=gl_length(types); ++i){
		MSG("ancestor %d: %s",i,SCP((symchar *)gl_fetch(types,i)));
	}

	symchar *inl = AddSymbol("inline");
	gl_append_ptr(langs,(VOIDPTR)inl);

	gl_append_ptr(ids,(VOIDPTR)varname);

	noteslist = GetNotesList(dbid,types,langs,ids,NOTESWILDLIST,NOTESWILDLIST);

	gl_destroy(types);
	gl_destroy(langs);
	gl_destroy(ids);

	MSG("noteslist = %ld items",gl_length(noteslist));

	if(gl_length(noteslist)==0){
		MSG("empty notes list returned");
		return NULL;
	}
	struct Note *n = (struct Note *)gl_fetch(noteslist,1);

	MSG("note ID = %s, lang = %s",SCP(GetNoteId(n)),SCP(GetNoteLanguage(n)));	

	MSG("note text = %s",BraceCharString(GetNoteText(n)));
	
	return BraceCharString(GetNoteText(n));
}

struct gl_list_t *notes_refined_for_type_with_lang(
	symchar *dbid
	, const struct TypeDescription *t
	, const symchar *lang
){
	int i, j;
	struct gl_list_t *noteslist = NULL;
	struct gl_list_t *types;
	struct pairlist_t *pl;
	unsigned long tlen, nlen;

	/* create list of ancestors' names; add this type's name at the end */
	types = GetAncestorNames(t);
	gl_append_ptr(types,(VOIDPTR)GetName(t));

	/* this pairlist will a list of (varname, NOTE) */
	pl = pairlist_create(1);	/* FIXME: should be gllist. stored noteid is never reused unless maybe in a debugger */
	
	tlen = gl_length(types);
	for(i=1; i<=tlen; ++i){
		symchar *typename = (symchar *)gl_fetch(types,i);
		noteslist = GetNotes(dbid, typename, lang, NOTESWILD, NOTESWILD, nd_wild);
		nlen = gl_length(noteslist);
		for(j=1; j<=nlen; ++j){
			struct Note * note_ptr;
			note_ptr = (struct Note *)gl_fetch(noteslist,j);
			symchar *noteid = GetNoteId(note_ptr);
			/* only care about NOTEs with non-NULL noteid. notes applied to
				the type itself don't have an associated child id.
			 */
			if(noteid)pairlist_set(pl, (void *)noteid, (void *)note_ptr);
		}
		gl_destroy(noteslist);		
	}
	gl_destroy(types);
	return pairlist_values_and_destroy(pl);
}

