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

	@param fp the file to be imported
	@param initfunc the 'name' of a registration 'function' to be run in the
	imported file. This comes from the ASCEND syntax "FROM XXXX IMPORT YYYY"
	(more or less, as I recall) but should normally be set to NULL so that
	the default registration function can be used, and simpler 'IMPORT "XXXX"'
	syntax can be used by the end user.
*/
typedef int ImportHandlerImportFn(const struct FilePath *fp,const char *initfunc, const char *partialpath);

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
ASC_DLLSPEC(int) importhandler_destroylibrary();
int importhandler_createlibrary();
int importhandler_printlibrary(FILE *fp);
int importhandler_printhandler(FILE *fp, struct ImportHandler *);

/*------------------------------------------------------------------------------
  PATH SEARCH ROUTINES
*/

/**
	Search through a path (a la unix $PATH variable) as specified by a specific
	environment variable (or fall back to a default hard-wired file path) and 
	find a file that matches the partial filename specfied. For each directory 
	in the path, the registered importhandlers will be tried, in the order
	they were registered. If no file matching any of the importhandler filename
	pattern (eg 'myext' becomes '/path/to/myext.py' for the case of 
	an import handler with a '.py' extension and a path component of '/path/to')
	then the next component of the search path is tried.

	@return NULL if no readable file is found, else return a FilePath structure
	pointing to the location of the file found.
*/	
struct FilePath *importhandler_findinpath(const char *partialname
		, char *defaultpath, char *envv, struct ImportHandler **handler
);

/*------------------------------------------------------------------------------
  SHARED POINTER TABLE
*/

/**
	Create a new registry. This should be called whenever an import handler
	library is being created, or when a GUI first wants to register pointers.

	@return 0 on success
*/
ASC_DLLSPEC(int) importhandler_createsharedpointertable();

/**
	Sets a pointer in the shared pointer table. This should only be called from
	the GUI code.

	This is also called from inside Type::getSimulation in the C++ interface.

	@return 0 on success
*/
ASC_DLLSPEC(int) importhandler_setsharedpointer(const char *key, void *ptr);

/**
	Retrieve a pointer from the shared pointer table. Returns NULL if the
	pointer is not found (or if it is found but has NULL value). This should
	only be used from import handler code.

	@return NULL on not found (or NULL value stored in registry)
*/
ASC_DLLSPEC(void *) importhandler_getsharedpointer(const char *key);

#endif
