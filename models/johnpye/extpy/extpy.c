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
#include <compiler/extfunc.h>

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

	CONSOLE_DEBUG("Hello...");

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
  PYTHON METHOD INVOKER
*/

ExtMethodRun extpy_invokemethod;

/** Method invoker. extpy will supply a pointer to this function whenever it
	registers a python function as an external script method. This function will
	then dereference the user_data field into a python function, and execute that
	python function.

	One difficult aspect is the question of how to usefully pass the 'context'
	argument to Python?
*/
int extpy_invokemethod(struct Instance *context, struct gl_list_t *args, void *user_data){
	PyObject *fn;
	/* cast user data to PyObject pointer */
	fn = (PyObject *) user_data;

	ERROR_REPORTER_HERE(ASC_USER_NOTE,"RUNNING PYTHON METHOD");
	CONSOLE_DEBUG("RUNNING PYTHON METHOD...");
	return 1;
}

/*------------------------------------------------------------------------------
  'EXTPY' PYTHON STATIC MODULE
*/

static PyObject *extpy_getbrowser(PyObject *self, PyObject *args){
	PyObject *browser;
	if(args!=NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"args is not NULL?!");
	}
	browser = (PyObject *)importhandler_getsharedpointer("browser");
	return Py_BuildValue("O",browser);
}

static PyObject *extpy_registermethod(PyObject *self, PyObject *args){
	PyObject *fn, *name, *docstring;
	const char *cname, *cdocstring;
	int res;
	int nargs = 1;

	PyArg_ParseTuple(args,"O:registermethod", &fn);
	if(!PyCallable_Check(fn)){
		PyErr_SetString(PyExc_TypeError,"parameter must be callable");
		return NULL;
	}
	Py_INCREF(fn);

	CONSOLE_DEBUG("FOUND FN=%p",fn);

	name = PyObject_GetAttr(fn,PyString_FromString("__name__"));
	if(name==NULL){
		CONSOLE_DEBUG("No __name__ attribute");
		PyErr_SetString(PyExc_TypeError,"No __name__ attribute");
		return NULL;
	}
	cname = PyString_AsString(name);

	CONSOLE_DEBUG("REGISTERED METHOD '%s' HAS %d ARGS",cname,nargs);

	docstring = PyObject_GetAttr(fn,PyString_FromString("func_doc"));
	cdocstring = "(no help)";
	if(name!=NULL){
		cdocstring = PyString_AsString(docstring);
		CONSOLE_DEBUG("DOCSTRING: %s",cdocstring);
	}

	res = CreateUserFunctionMethod(cname,extpy_invokemethod,nargs,cdocstring,fn);

	CONSOLE_DEBUG("EXTPY INVOKER IS AT %p",extpy_invokemethod);

	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem registering external script method (%d)",res);
		PyErr_SetString(PyExc_Exception,"unable to register script method");
		return NULL;
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Registered python method '%s'",cname);

	/* nothing gets returned (but possibly an exception) */
	Py_INCREF(Py_None);
	return Py_None;	
}

static PyMethodDef extpymethods[] = {
	{"getbrowser", extpy_getbrowser, METH_NOARGS,"Retrieve browser pointer"}
	,{"registermethod", extpy_registermethod, METH_VARARGS,"Register a python method as an ASCEND script method"}
	,{NULL,NULL,0,NULL}
};

PyMODINIT_FUNC initextpy(void){
    PyObject *obj;
	obj = Py_InitModule3("extpy", extpymethods,"Module for accessing shared ASCEND pointers from python");
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
	PyObject *pyfile;

	CONSOLE_DEBUG("Importing Python script %s",name);
	if(Py_IsInitialized()){
		CONSOLE_DEBUG("Python was already initialised");
	}else{
		CONSOLE_DEBUG("INITIALISING PYTHON");
		Py_Initialize();
		CONSOLE_DEBUG("COMPLETED ATTEMPT TO INITIALISE PYTHON");
	}

	if(!Py_IsInitialized()){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to initialise Python");
		CONSOLE_DEBUG("UNABLE TO INITIALIZE PYTHON");
		ASC_FREE(name);
		return 1;
	}

	initextpy();

	pyfile = PyFile_FromString(name,"r");
	if(pyfile==NULL){
		CONSOLE_DEBUG("Failed opening script");
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to open '%s' (%s)",partialpath,name);
		ASC_FREE(name);
		return 1;
	}
	
	f = PyFile_AsFile(pyfile);		
	if(f==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to cast PyObject to FILE*");
		ASC_FREE(name);
		return 1;
	}

	PyRun_AnyFileEx(f,name,1);
	/*if(PyErr_Occurred()){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"An error occurred in the python script '%s'. Check the console for details");
		PyErr_Print();
		PyErr_Clear();
		return 1;
	}*/

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Imported python script '%s' (check console for errors)",partialpath);

	ASC_FREE(name);
	return 0;
}

