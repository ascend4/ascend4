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

#ifndef ASC_NOTEQUERY_H
#define ASC_NOTEQUERY_H

/**	@addtogroup compiler Compiler
	@{
*/

#include <ascend/utilities/ascConfig.h>
#include <ascend/compiler/compiler.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/general/list.h>
#include <ascend/general/pairlist.h>

/**
	Get the most refined note of a certain 'language' lang for the specified
	variable (varname) within the TypeDescription t.
	@return pointer to the note text, or NULL if no note or no variable found.

	@param lang note type symchar, eg 'inline', 'description', 'block', etc.
*/
ASC_DLLSPEC const char *notes_get_for_variable(
	symchar *dbid
	, const struct TypeDescription *t
	, const symchar *varname
	, const symchar *lang
);

/**
	Look at a particular TypeDescription t its ancestors, and return
	the most refined note of a certain notetype for each variable.
	This is being implemented for detecting the inlets and outlets of a block 
	in our canvas-based modelling approach.
*/
ASC_DLLSPEC struct gl_list_t *notes_refined_for_type_with_lang(
	symchar *dbid
	,const struct TypeDescription *t
	,const symchar *lang
);
#endif

