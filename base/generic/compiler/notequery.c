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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Query function(s) for the NOTES database in notate.h
*/

#include "notequery.h"
#include "notate.h"
#include "symtab.h"
#include <utilities/error.h>

const char *notes_get_for_variable(symchar *dbid
	, const struct TypeDescription *t
	, const symchar *varname
	, const symchar *notetype
){
	struct gl_list_t *noteslist;

	struct gl_list_t *types = GetAncestorNames(t);
	struct gl_list_t *langs = gl_create(1);
	struct gl_list_t *methods = gl_create(1);
	struct gl_list_t *nds = gl_create(1);
	struct gl_list_t *ids = gl_create(1);

	int i;
	for(i=1; i<=gl_length(types); ++i){
		CONSOLE_DEBUG("ancestor %d: %s",i,SCP((symchar *)gl_fetch(types,i)));
	}

	symchar *inl = AddSymbol("inline");
	gl_append_ptr(langs,(VOIDPTR)inl);

	gl_append_ptr(ids,(VOIDPTR)varname);

	noteslist = GetNotesList(dbid,types,langs,ids,NOTESWILDLIST,NOTESWILDLIST);

	CONSOLE_DEBUG("noteslist = %ld items",gl_length(noteslist));

	if(gl_length(noteslist)==0){
		CONSOLE_DEBUG("empty notes list returned");
		return NULL;
	}
	struct Note *n = (struct Note *)gl_fetch(noteslist,1);

	CONSOLE_DEBUG("note ID = %s, lang = %s",SCP(GetNoteId(n)),SCP(GetNoteLanguage(n)));	

	CONSOLE_DEBUG("note text = %s",BraceCharString(GetNoteText(n)));
	
	return BraceCharString(GetNoteText(n));
}

