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
	Created Sept 26, 2006
*/

#include <utilities/ascConfig.h>
#include <utilities/config.h>
#include <utilities/error.h>
#include "importhandler.h"

/*
	Maximum number of importhandlers possible in one session. Hard to imagine
	that you would want more than this.
*/
#define IMPORTHANDLER_MAX 10

/**
	List of import handlers currently in effect. @TODO this shouldn't be a global,
	but unfortunately such globals are 'The ASCEND Way'.
*/
struct ImportHandler **importhandler_library=NULL;

ASC_DLLSPEC(int) importhandler_add(struct ImportHandler *handler){
	int i;
	if(handler==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Handler is NULL");
		return 2;
	}
	if(importhandler_library == NULL){
		importhandler_createlibrary();
	}
	for(i=0; i< IMPORTHANDLER_MAX; ++i){
		if(importhandler_library[i] == NULL)break;
		if(importhandler_library[i]->name == handler->name){
			ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Handler already loaded");
			return 0;
		}
	}
	if(i==IMPORTHANDLER_MAX){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Too many import handlers register (IMPORTHANDLER_MAX=%d)",IMPORTHANDLER_MAX);
		return 1;
	}
	importhandler_library[i] = handler;
	return 0;
}

/* Function to attempt import of an external script
	@param partialname Name of the external script (without extension), relative to PATH.
	@param defaultpath Default value of file search PATH. Is trumped by value of pathenvvar if present in environment.
	@param pathenvvar Environment variable containing the user's preferred file search path value.
	@return 0 on success
*/
int importhandler_attemptimport(const char *partialname,const char *defaultpath, const char *pathenvvar){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s not implemented",__FUNCTION__);
	return 1;
}

/*------------------------------------------------------------------------------
  DEFAULT IMPORT HANDLER FOR DLL/SO FILES
*/

/**
	Create a filename for an external library (DLL/SO) based on a
	partial filename.

	@param partialname The partial filename (eg 'mylib')
	@return Complete filename (eg 'libmylib.so' or 'mylib.dlll', etc)
*/
char *importhandler_extlib_filename(const char *partialname){
	char *buffer;
	buffer = ASC_NEW_ARRAY(char,PATH_MAX);

#if defined(ASC_SHLIBSUFFIX) && defined(ASC_SHLIBPREFIX)
	/*
		this is the preferred operation: SCons reports what the local system
		uses as its shared library file extension.
	*/
	snprintf(buffer,PATH_MAX,"%s%s%s",ASC_SHLIBPREFIX,s1,ASC_SHLIBSUFFIX);
#else
	/**
		@DEPRECATED

		If we don't have ASC_SHLIB-SUFFIX and -PREFIX then we can do some
		system-specific stuff here, but it's not as general.
	*/
# ifdef __WIN32__
	snprintf(buffer,PATH_MAX,"%s.dll",partialname1);
# elif defined(linux)
	snprintf(buffer,PATH_MAX,"lib%s.so",partialname); /* changed from .o to .so -- JP */
# elif defined(sun) || defined(solaris)
	snprintf(buffer,PATH_MAX,"%s.so.1.0",partialname);
# elif defined(__hpux)
	snprintf(buffer,PATH_MAX,"%s.sl",partialname);
# elif defined(_SGI_SOURCE)
	snprintf(buffer,PATH_MAX,"%s.so",partialname);
# else
#  error "Unknown system type (please define ASC_SHLIBSUFFIX and ASC_SHLIBPREFIX)"
# endif
#endif
	
	return buffer;
}

/**
	Perform the actual importing of an external DLL/SO in to ASCEND. Can assume
	that the file exists and is readable.

	@param fp Location of DLL/SO file
	@return 0 on success
*/
int importhandler_extlib_import(struct FilePath *fp,void *user_data){
	return 1;
}

/*------------------------------------------------------------------------------
  LIST-BASED FUNCTIONS related to IMPORT handler 'library'
*/

int importhandler_createlibrary(){
	int i;
	if(importhandler_library!=NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Already created");
		return 1;
	};
	importhandler_library=ASC_NEW_ARRAY(struct ImportHandler *,IMPORTHANDLER_MAX);
	for(i=0; i < IMPORTHANDLER_MAX; ++i){
		importhandler_library[i] = NULL;
	}
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"ImportHandler library created");
	return 0;
}

int importhandler_remove(const char *name){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s not implemented",__FUNCTION__);
	return 1;
}

struct ImportHandler *importhandler_lookup(const char *name){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s not implemented",__FUNCTION__);
	return NULL;
}

int importhandler_destroylibrary(){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s not implemented",__FUNCTION__);
	CONSOLE_DEBUG("NOT IMPLEMENTED");
	return 1;
}


int importhandler_printlibrary(FILE *fp){
	CONSOLE_DEBUG("NOT IMPLEMENTED");
	return 1;
}

int importhandler_printhandler(FILE *fp, struct ImportHandler *handler){
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"%s not implemented",__FUNCTION__);
	CONSOLE_DEBUG("NOT IMPLEMENTED");
	return 1;
}


