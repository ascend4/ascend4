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
	Import handler to provide external python script functionality for ASCEND.
*/

#include <stdio.h>
#include <string.h>

#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <general/ospath.h>

#include <compiler/importhandler.h>

#include <Python.h>

ImportHandlerCreateFilenameFn extpy_filename;
ImportHandlerImportFn extpy_import;

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

/**
	This is the function called from "IMPORT extpy"

	It sets up the functions in this external function library
*/
extern ASC_EXPORT(int) extpy_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Hello from EXTPY...");

	struct ImportHandler *handler;
	handler = ASC_NEW(struct ImportHandler);

	handler->name = "extpy";
	handler->filenamefn = extpy_filename;
	handler->importfn = extpy_import;

	result = importhandler_add(handler);

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to register import handler (error = %d)",result);
	}
	return result;
}

/*------------------------------------------------------------------------------
  METHODS TO EXPOSE DATA TO THE EXTERNAL SCRIPT
*/

/* Return the number of arguments of the application command line */
static PyObject *extpy_getbrowser(PyObject *self, PyObject *args){
	PyObject *browser;
	if(args!=NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"args is not NULL?!");
	}
	browser = (PyObject *)importhandler_getsharedpointer("browser");
	return Py_BuildValue("O",browser);
}

static PyMethodDef extpymethods[] = {
	{"getbrowser", extpy_getbrowser, METH_NOARGS,"Retrieve browser pointer"}
	,{NULL,NULL,0,NULL}
};

PyMODINIT_FUNC initextpy(void){
    PyObject *obj;
	CONSOLE_DEBUG("registering 'extpy' module...");
	obj = Py_InitModule3("extpy", extpymethods,"Module for accessing shared ASCEND pointers from python");
	CONSOLE_DEBUG("returned %p",obj);
	CONSOLE_DEBUG("name %s",PyModule_GetName(obj));
}

/*------------------------------------------------------------------------------
  STANDARD IMPORT HANDLER ROUTINES
*/

/**
	Create a filename base on a partial filename. In that case of python, this
	just means adding '.py' to the end.

	@param partialname the filename without suffix, as specified in the user's "IMPORT" command
	@return new filename, or NULL on failure
*/
char *extpy_filename(const char *partialname){
	char *name;
	int len;
	if(partialname==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Partial name is NULL, can't work out filename");
		return NULL;
	}

	len = strlen(partialname);
	name = ASC_NEW_ARRAY_CLEAR(char,len+4);
	strcpy(name,partialname);
	strcat(name,".py");
	CONSOLE_DEBUG("New filename is '%s'",name);
	return name;
}

/**
	Import a python script located at the path indicated.

	@return 0 on success, else error codes (TBD)
*/
int extpy_import(const struct FilePath *fp, const char *initfunc, const char *partialpath){
	char *name;
	name = ospath_str(fp);
	FILE *f;

	CONSOLE_DEBUG("IMPORTING PYTHON SCRIPT %s",name);
	if(Py_IsInitialized()){
		CONSOLE_DEBUG("PYTHON IS ALREADY INITIALISED");
	}else{
		CONSOLE_DEBUG("INITIALISING PYTHON");
		Py_Initialize();
		CONSOLE_DEBUG("COMPLETED ATTEMPT TO INITIALISE PYTHON");
	}

	if(!Py_IsInitialized()){
		CONSOLE_DEBUG("UNABLE TO INITIALIZE PYTHON");
		return 1;
	}
	PyRun_SimpleString("print \"HELLO FROM PYTHON IN C\"");
	CONSOLE_DEBUG("IMPORTING ASCPY...");
	PyRun_SimpleString("import ascpy");
	CONSOLE_DEBUG("CREATING LIBRARY OBJECT...");
	PyRun_SimpleString("L = ascpy.Library()");
	CONSOLE_DEBUG("PRINTING MESSAGE...");
	PyRun_SimpleString("print \"IMPORTED ASCPY\"");
	PyRun_SimpleString("print L");

	initextpy();

	CONSOLE_DEBUG("OPENING THE SCRIPT \"%s\"",name);
	f = fopen(name,"r");
	if(f==NULL){
		CONSOLE_DEBUG("Failed opening script");
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"UNABLE TO OPEN SCRIPT");
		return 1;
	}
	CONSOLE_DEBUG("RUNNING THE SCRIPT");
	PyRun_AnyFile(f,name);
	CONSOLE_DEBUG("FINISHED RUNNING THE SCRIPT");

	fclose(f);

	ASC_FREE(name);
	return 1;
}

