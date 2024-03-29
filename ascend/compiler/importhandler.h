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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include <ascend/general/platform.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>

/**	@addtogroup compiler_file Compiler File Handling
	@{
*/

/*------------------------------------------------------------------------------
  DATA STRUCTURES AND TYPES
*/

struct ImportHandler;

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

/**
	This is the 'unload' function that will unload external code
	when no longer required.

	The function should return 0 on success.
*/
typedef int ImportHandlerUnloadFn(const struct FilePath *fp,const char *cleanupfunc);

/**
	This function can be used to unload an import handler, and should
	deallocate any memory that was used or unload any shared libraries that
	were loaded in order to provide this import handler's functionality.
	@param handler pointer to the un-destroyed handler. Pointer will be invalid on return.
	@return 0 on success.

	@note destroying (unloading) an import handler is not the same as unloading
	whatever the thing was that was imported (ie a specific 'package'). This
	latter case is handled by `unloadfn`.
*/
typedef int ImportHandlerDestroyFn(struct ImportHandler *handler);

struct ImportHandler{
	const char *name; /**< name of this import handler, eg 'extpy' */
	ImportHandlerCreateFilenameFn *filenamefn; /**< function which converts a partial filename into a correct filename, eg by adding a suffix */
	ImportHandlerImportFn *importfn; /**< function which loads an external script module once it has been located */
	ImportHandlerUnloadFn *unloadfn; /**< (optional) function with unloads an external script/module as part of memory clean-up (returns zero on success) */
	ImportHandlerDestroyFn *destroyfn; /**< function that can unload and destroy this import handler */
};

struct ImportPackage{
	struct FilePath *fp;
	char *partialpath;
	const char *cleanupfunc;
	struct ImportHandler *handler;
};

/**
	List of import handlers currently in effect. @TODO this shouldn't be a global,
	but unfortunately such globals are 'The ASCEND Way' for now.
*/
struct ImportHandlerLibrary{
	struct ImportHandler **handlers;
	struct gl_list_t *packages;
};

/*------------------------------------------------------------------------------
  FUNCTION TO PERFORM AN IMPORT
*/

/** Import an 'import package' using a specified handler, and record it in the import library if successful.
	@param partialname Name of the external script (without extension), relative to PATH.
	@param defaultpath Default value of file search PATH. Is trumped by value of pathenvvar if present in environment.
	@param pathenvvar Environment variable containing the user's preferred file search path value.
	@return 0 on success
*/
int importhandler_import(struct ImportHandler *handler, struct FilePath *fp
	, const char *initfunc, const char *cleanupfunc, const char *partialpath
);

/*------------------------------------------------------------------------------
  FUNCTIONS TO AND AND REMOVE import handlers
*/

/** Function to add a new import handler to the list that will be tried during an IMPORT 
	@param handler Handler struct to be added to the list 
	@return 0 on success
*/
ASC_DLLSPEC int importhandler_add(struct ImportHandler *handler);

/**
	Destroy/unload an import handler, and deallocate the memory pointed to
	by handler.
	@param handler the handler to be destroyed.
	@return 0 on success. Otherwise, the handler will not have been freed.
*/
int importhandler_destroy(struct ImportHandler *handler);

/*------------------------------------------------------------------------------
  LIST-BASED FUNCTIONS related to IMPORT handler 'library'
*/

struct ImportHandler *importhandler_lookup(const char *name);
ASC_DLLSPEC int importhandler_destroylibrary();
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
		, const char *defaultpath, char *envv, struct ImportHandler **handler
);

/*------------------------------------------------------------------------------
  BUILT-IN IMPORT HANDLER FOR DEALING WITH DLL/SO external libraries
*/

ImportHandlerCreateFilenameFn importhandler_extlib_filename;
ImportHandlerImportFn importhandler_extlib_import;

/*------------------------------------------------------------------------------
  SHARED POINTER TABLE
*/

/**
	Create a new registry. This should be called whenever an import handler
	library is being created, or when a GUI first wants to register pointers.

	@return 0 on success
*/
ASC_DLLSPEC int importhandler_createsharedpointertable();

/**
	Sets a pointer in the shared pointer table. This should only be called from
	the GUI code.

	This is also called from inside Type::getSimulation in the C++ interface.

	@return 0 on success
*/
ASC_DLLSPEC int importhandler_setsharedpointer(const char *key, void *ptr);

/**
	Retrieve a pointer from the shared pointer table. Returns NULL if the
	pointer is not found (or if it is found but has NULL value). This should
	only be used from import handler code.

	@return NULL on not found (or NULL value stored in registry)
*/
ASC_DLLSPEC void *importhandler_getsharedpointer(const char *key);

/* }@ */

#endif
