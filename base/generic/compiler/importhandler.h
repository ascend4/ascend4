/*	ASCEND modelling environment
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
	Handle the Import Handler library, which is a hash table of additional
	handlers for external scripts in the IMPORT statement.
*//*
	by John Pye
	Created Sept 7, 2006
*/

#ifndef IMPORTHANDLER_H
#define IMPORTHANDLER_H

#include <general/ospath.h>

/*------------------------------------------------------------------------------
  DATA STRUCTURES AND TYPES
*/

/**
	This is the 'create filename' function that is use to take a generic
	'partial filename' such as 'mystuff' and convert it into a correct
	filename for the ImportHandler in question. For example, a converted
	filename might be 'libmystuff.so' or 'mystuff.dll' or 'mystuff.py', etc.

	The function should return 0 on success.
*/
typedef char *ImportHandlerCreateFilenameFn(const char *partialname);

/**
	This is the 'import' function that will actually do the job of importing
	some external code.

	The function should return 0 on success.

	The 'user_data' parameter can be used to pass additional parameters to
	the import function, such as verbosity flags or the name of a 'registration'
	function (as may be the case for importing external DLLs/SOs)
*/
typedef int ImportHandlerImportFn(struct FilePath *fp,void *user_data);

struct ImportHandler{
	const char *name; /**< name of this import handler, eg 'extpy' */
	ImportHandlerCreateFilenameFn *filenamefn; /**< function which converts a partial filename into a correct filename, eg by adding a suffix */
	ImportHandlerImportFn *importfn; /**< function which loads an external script module once it has been located */
};

/**
	List of import handlers currently in effect. @TODO this shouldn't be a global,
	but unfortunately such globals are 'The ASCEND Way'.
*/
extern struct ImportHandler **ImportHandlerLibrary;

/** Function to add a new import handler to the list that will be tried during an IMPORT 
	@param handler Handler struct to be added to the list 
	@return 0 on success
*/
ASC_DLLSPEC(int) importhandler_add(struct ImportHandler *handler);

/** Function to attempt import of an external script
	@param partialname Name of the external script (without extension), relative to PATH.
	@param defaultpath Default value of file search PATH. Is trumped by value of pathenvvar if present in environment.
	@param pathenvvar Environment variable containing the user's preferred file search path value.
	@return 0 on success
*/
int importhandler_attemptimport(const char *partialname,const char *defaultpath, const char *pathenvvar);

/*------------------------------------------------------------------------------
  FUNCTIONS FOR IMPORT OF DLL/SO external libraries
*/
ImportHandlerCreateFilenameFn importhandler_extlib_filename;
ImportHandlerImportFn importhandler_extlib_import;

/*------------------------------------------------------------------------------
  LIST-BASED FUNCTIONS related to IMPORT handler 'library'
*/

int importhandler_remove(const char *name);
struct ImportHandler *importhandler_lookup(const char *name);
int importhandler_destroylibrary();
int importhandler_createlibrary();
int importhandler_printlibrary(FILE *fp);
int importhandler_printhandler(FILE *fp, struct ImportHandler *);

#endif
