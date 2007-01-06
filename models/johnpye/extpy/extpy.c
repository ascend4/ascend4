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
*//*
	by John Pye, Oct 2006
*/

#ifndef WITH_PYTHON
# error "Can't build 'extpy' without WITH_PYTHON set"
#else

#include <Python.h>

#include <stdio.h>
#include <string.h>

#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <general/ospath.h>

#include <compiler/importhandler.h>
#include <compiler/extfunc.h>


ImportHandlerCreateFilenameFn extpy_filename;
ImportHandlerImportFn extpy_import;

ExtMethodDestroyFn extpy_destroy;


#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

struct ExtPyData{
	PyObject *fn;
	char *name;
};

/**
	This is the function called from "IMPORT extpy"

	It sets up the functions in this external function library
*/
extern ASC_EXPORT(int) extpy_register(){
	int result = 0;

	struct ImportHandler *handler;
	handler = ASC_NEW(struct ImportHandler);

	handler->name = "extpy";
	handler->filenamefn = &extpy_filename;
	handler->importfn =   &extpy_import;

	result = importhandler_add(handler);

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to register import handler (error = %d)",result);
	}

	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Loaded EXPERIMENTAL 'extpy' import handler.");
	
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
	PyObject *arglist=NULL, *result=NULL, *pyinstance=NULL, *dict=NULL
		, *mainmodule=NULL, *errstring=NULL, *errtypestring=NULL;
	PyObject *perrtype=NULL, *perrvalue=NULL, *perrtrace=NULL;

	int ret;
	struct ExtPyData *extpydata;

	/* cast user data to PyObject pointer */
	extpydata = (struct ExtPyData *)user_data;

	mainmodule = PyImport_AddModule("__main__");
	if(mainmodule==NULL){
		CONSOLE_DEBUG("Unable to retrieve __main__ module");
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	dict = PyModule_GetDict(mainmodule);
	if(dict==NULL){
		CONSOLE_DEBUG("Unable to retrieve __main__ dict");
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	CONSOLE_DEBUG("Running python method '%s'",extpydata->name);

	if(!PyCallable_Check(extpydata->fn)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"user_data is not a PyCallable");
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	/*
		We need to be able to convert C 'struct Instance' pointers to Python 'Instance' objects.
		This functionality is implemented in 'ascpy' but we're not going to link to that here,
		so we will use the importhandler 'setsharedpointer' trick to pass the object to the
		'registry' then write a routine in ascpy that will cast it into the appropriate
		Python object.
	*/
	importhandler_setsharedpointer("context",(void *)context);

	PyErr_Clear();
	pyinstance = PyRun_String("ascpy.Registry().getInstance('context')",Py_eval_input,dict,dict);
	if(PyErr_Occurred()){
		CONSOLE_DEBUG("Failed retrieving instance");
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	arglist = Py_BuildValue("(O)", pyinstance);
	

	PyErr_Clear();
	result = PyEval_CallObject(extpydata->fn, arglist);

	if(PyErr_Occurred()){
		CONSOLE_DEBUG("Error occured in PyEval_CallObject");

		/* get the content of the error message */
		PyErr_Fetch(&perrtype, &perrvalue, &perrtrace);		

		errtypestring = NULL;
		if(perrtype != NULL
			&& (errtypestring = PyObject_Str(perrtype)) != NULL
		    && PyString_Check(errtypestring)
		){
			// nothing
		}else{
			errtypestring = Py_BuildValue("");
		}
	
		errstring = NULL;
		if(perrvalue != NULL
			&& (errstring = PyObject_Str(perrvalue)) != NULL
		    && PyString_Check(errstring)
		){
			error_reporter(ASC_PROG_ERR
				,extpydata->name,0
				,PyString_AsString(errtypestring)
				,"%s",PyString_AsString(errstring)
			);
		}else{
			error_reporter(ASC_PROG_ERR,extpydata->name,0,extpydata->name,"(unknown python error)");
		}
		PyErr_Print();
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	ret=0;

cleanup_extpy_invokemethod:
	Py_XDECREF(dict);
	Py_XDECREF(arglist);
	Py_XDECREF(pyinstance);
	Py_XDECREF(errstring);
	Py_XDECREF(errtypestring);
	Py_XDECREF(perrtype);
	Py_XDECREF(perrvalue);
	Py_XDECREF(perrtrace);
	return ret;
}

/**
	Free memory associated with a registered script method.
	@return 0 on success
*/
int extpy_destroy(void *user_data){
	struct ExtPyData *extpydata;
	extpydata = (struct ExtPyData *)user_data;
	Py_DECREF(extpydata->fn);
	ASC_FREE(extpydata->name);
	ASC_FREE(extpydata);
	return 0;
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
	if(browser==NULL){
		return Py_BuildValue("");
	}
	return browser;
	/* return Py_BuildValue("O",browser);*/
}

static PyObject *extpy_registermethod(PyObject *self, PyObject *args){
	PyObject *fn, *name, *docstring;
	const char *cname, *cdocstring;
	int res;
	int nargs = 1;
	struct ExtPyData *extpydata;

	PyArg_ParseTuple(args,"O:registermethod", &fn);
	if(!PyCallable_Check(fn)){
		PyErr_SetString(PyExc_TypeError,"parameter must be callable");
		return NULL;
	}

	/* CONSOLE_DEBUG("FOUND FN=%p",fn); */

	name = PyObject_GetAttr(fn,PyString_FromString("__name__"));
	if(name==NULL){
		CONSOLE_DEBUG("No __name__ attribute");
		PyErr_SetString(PyExc_TypeError,"No __name__ attribute");
		return NULL;
	}
	cname = PyString_AsString(name);

	/* CONSOLE_DEBUG("REGISTERED METHOD '%s' HAS %d ARGS",cname,nargs); */

	docstring = PyObject_GetAttr(fn,PyString_FromString("func_doc"));
	cdocstring = "(no help)";
	if(name!=NULL){
		cdocstring = PyString_AsString(docstring);
		CONSOLE_DEBUG("DOCSTRING: %s",cdocstring);
	}

	extpydata = ASC_NEW(struct ExtPyData);
	extpydata->name = ASC_NEW_ARRAY(char,strlen(cname)+1);
	extpydata->fn = fn;
	strcpy(extpydata->name, cname);

	res = CreateUserFunctionMethod(cname,&extpy_invokemethod,nargs,cdocstring,(void *)extpydata,&extpy_destroy);
	Py_INCREF(fn);

	/* CONSOLE_DEBUG("EXTPY 'fn' IS AT %p",fn); */

	/* CONSOLE_DEBUG("EXTPY INVOKER IS AT %p",extpy_invokemethod); */

	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem registering external script method (%d)",res);
		PyErr_SetString(PyExc_Exception,"unable to register script method");
		return NULL;
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Registered python method '%s'\n",cname);

	/* nothing gets returned (but possibly an exception) */
	return Py_BuildValue("");
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
	int iserr;

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

	if(PyRun_SimpleString("import ascpy")){
		CONSOLE_DEBUG("Failed importing 'ascpy'");
		return 1;
	}

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

	PyErr_Clear();

	iserr = PyRun_AnyFileEx(f,name,1);
	
	if(iserr){
		/* according to the manual, there is no way of determining anything more about the error. */
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"An error occurring in importing the script '%s'",name);
		return 1;
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Imported python script '%s'\n",partialpath);

	ASC_FREE(name);
	return 0;
}

#endif /* WITH_PYTHON */

