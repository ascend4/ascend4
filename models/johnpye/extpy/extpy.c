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
	Import handler to provide external python script functionality for ASCEND.
*//*
	by John Pye, Oct 2006
*/

#include <Python.h>

#include <stdio.h>
#include <string.h>

#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/general/ospath.h>

#include <ascend/compiler/importhandler.h>
#include <ascend/compiler/extfunc.h>

#define EXTPY_DEBUG
#ifdef EXTPY_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif


ImportHandlerCreateFilenameFn extpy_filename;
ImportHandlerImportFn extpy_import;
ImportHandlerDestroyFn extpy_handler_destroy;

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
extern ASC_EXPORT int extpy_register(){
	int result = 0;

	struct ImportHandler *handler;
	handler = ASC_NEW(struct ImportHandler);

	handler->name = "extpy";
	handler->filenamefn = &extpy_filename;
	handler->importfn =   &extpy_import;
	handler->destroyfn = &extpy_handler_destroy;

	result = importhandler_add(handler);

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to register import handler (error = %d)",result);
	}

	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Loaded EXPERIMENTAL 'extpy' import handler.");

	return result;
}

int extpy_handler_destroy(struct ImportHandler *handler){
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Not implemented");
	return 0;
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
		MSG("Unable to retrieve __main__ module");
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	dict = PyModule_GetDict(mainmodule);
	if(dict==NULL){
		MSG("Unable to retrieve __main__ dict");
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	MSG("Running python method '%s'",extpydata->name);

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
		MSG("Failed retrieving instance");
		ret = 1;
		goto cleanup_extpy_invokemethod;
	}

	arglist = Py_BuildValue("(O)", pyinstance);


	PyErr_Clear();
	result = PyEval_CallObject(extpydata->fn, arglist);
	(void)result; // we don't use the result.
	
	if(PyErr_Occurred()){
		MSG("Error occured in PyEval_CallObject");

		/* get the content of the error message */
		PyErr_Fetch(&perrtype, &perrvalue, &perrtrace);

		errtypestring = NULL;
		if(perrtype != NULL
			&& (errtypestring = PyObject_Str(perrtype)) != NULL
		    && PyUnicode_Check(errtypestring)
		){
			// nothing
		}else{
			errtypestring = Py_BuildValue("");
		}

		errstring = NULL;
		if(perrvalue != NULL
			&& (errstring = PyObject_Str(perrvalue)) != NULL
		    && PyUnicode_Check(errstring)
		){
			error_reporter(ASC_PROG_ERR
				,extpydata->name,0
				,PyUnicode_AsUTF8(errtypestring)
				,"%s",PyUnicode_AsUTF8(errstring)
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

	/* MSG("FOUND FN=%p",fn); */

	name = PyObject_GetAttr(fn,PyUnicode_FromString("__name__"));
	if(name==NULL){
		MSG("No __name__ attribute");
		PyErr_SetString(PyExc_TypeError,"No __name__ attribute");
		return NULL;
	}
	cname = PyUnicode_AsUTF8(name);

	/* MSG("REGISTERED METHOD '%s' HAS %d ARGS",cname,nargs); */

	docstring = PyObject_GetAttr(fn,PyUnicode_FromString("func_doc"));
	cdocstring = "(no help)";
	if(name!=NULL){
		cdocstring = PyUnicode_AsUTF8(docstring);
		//MSG("DOCSTRING: %s",cdocstring);
	}

	extpydata = ASC_NEW(struct ExtPyData);
	extpydata->name = ASC_NEW_ARRAY(char,strlen(cname)+1);
	extpydata->fn = fn;
	strcpy(extpydata->name, cname);

	res = CreateUserFunctionMethod(cname,&extpy_invokemethod,nargs,cdocstring,(void *)extpydata,&extpy_destroy);
	Py_INCREF(fn);

	/* MSG("EXTPY 'fn' IS AT %p",fn); */

	/* MSG("EXTPY INVOKER IS AT %p",extpy_invokemethod); */

	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem registering external script method (%d)",res);
		PyErr_SetString(PyExc_Exception,"unable to register script method");
		return NULL;
	}

	MSG("Registered python method '%s'\n",cname);

	/* nothing gets returned (but possibly an exception) */
	return Py_BuildValue("");
}

static PyMethodDef extpymethods[] = {
	{"getbrowser", extpy_getbrowser, METH_NOARGS,"Retrieve browser pointer"}
	,{"registermethod", extpy_registermethod, METH_VARARGS,"Register a python method as an ASCEND script method"}
	,{NULL,NULL,0,NULL}
};

static PyModuleDef extpymodule = {
	PyModuleDef_HEAD_INIT
	,"extpy"
	,"Module for accessing shared ASCEND pointers from python"
	,-1
	,extpymethods
};

PyMODINIT_FUNC
PyInit_extpy(void){
	MSG("Actually creating the module...");
	PyObject *mod = PyModule_Create(&extpymodule);
	if(!mod) MSG("Some error creating the module...");
	else MSG("Module created OK");
	return mod;
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
	//MSG("New filename is '%s'",name);
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
	int iserr;

	MSG("Importing Python script %s",name);

	if(Py_IsInitialized()){
		MSG("Python was already initialised");
	}else{
		MSG("INITIALISING PYTHON");
		Py_InitializeEx(0); // no new signal handlers... for now
		MSG("COMPLETED ATTEMPT TO INITIALISE PYTHON");
	}

	if(!Py_IsInitialized()){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to initialise Python");
		MSG("UNABLE TO INITIALIZE PYTHON");
		ASC_FREE(name);
		return 1;
	}


	PyObject *mod = PyInit_extpy();
	if(0 != PyState_AddModule(mod, &extpymodule)){
		MSG("Unable to add module");
	}else{
		MSG("Module added");
	}

	if(-1 == PyImport_AppendInittab("extpy",&PyInit_extpy)){
		MSG("Unable to extend table of built-in modules");
	}else{
		MSG("Added to table of built-in modules");
	}

	PyObject *mod1 = PyState_FindModule(&extpymodule);
	if(mod1 == NULL){
		MSG("Unable to FindModule");
	}else{
		MSG("Module found");
	}
	
	//PyImport_Import(PyUnicode_FromString("extpy"));
#if 0
	MSG("About to create module extpy...");
	PyObject *mod = PyModule_Create(&extpymodule);
	if(mod == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to create 'extpy' module");
		ASC_FREE(name);
		return 1;
	}
	MSG("Created module '%s'",PyModule_GetName(mod));
#endif

	MSG("Importing 'extpy'");
	PyObject *pimp = PyImport_ImportModule("extpy");
	if(!pimp){
		PyErr_Print();
		MSG("Failed importing 'extpy'");
	}else{
		MSG("Imported extpy OK!");
	}

	MSG("Importing 'ascpy'");
	if(PyRun_SimpleString("import ascpy")){
		PyErr_Print();
		MSG("Failed importing 'ascpy'");
		ASC_FREE(name);
		return 1;
	}else{
		MSG("Imported 'ascpy' OK!");
	}

	MSG("Reading script '%s'",name);
	f = fopen(name,"r");
	if(f==NULL){
		MSG("Failed opening script");
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to open '%s' (%s)",partialpath,name);
		ASC_FREE(name);
		return 1;
	}
	PyErr_Clear();

	MSG("Running script '%s'",name);
	iserr = PyRun_AnyFileEx(f,name,1);

	if(iserr){
		MSG("Failed running script");
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);
		PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

		// Convert the error value to a string, if it's not already
		PyObject* str_exc_value = PyObject_Str(pvalue);
		if (str_exc_value != NULL) {
			// Convert Python string to C string
			const char* error_msg = PyUnicode_AsUTF8(str_exc_value);
			if (error_msg != NULL) {
				// Output or log the error message using your preferred method
				ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Unable to run '%s' (%s):\n%s", partialpath, name, error_msg);
			}
			Py_DECREF(str_exc_value);
		}else{
			ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Unable to run '%s' (%s).", partialpath, name);
		}

		// Remember to decref the objects you have fetched
		Py_XDECREF(ptype);
		Py_XDECREF(pvalue);
		Py_XDECREF(ptraceback);	
		ASC_FREE(name);
		return 1;
	}

	MSG("Imported python script '%s'\n",partialpath);

	ASC_FREE(name);
	return 0;
}


