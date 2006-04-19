#include "config.h"
#include "reporter.h"

#include <cstdio>
#include <iostream>
using namespace std;

#ifndef ASCXX_USE_PYTHON
# error "Where's ASCXX_USE_PYTHON?"
#endif


static const int REPORTER_MAX_ERROR_MSG = 1024;

#ifdef ASCXX_USE_PYTHON
// Python-invoking callback function
int reporter_error_python(ERROR_REPORTER_CALLBACK_ARGS){
	Reporter *reporter = Reporter::Instance();
	return reporter->reportErrorPython(ERROR_REPORTER_CALLBACK_VARS);
}
#endif

Reporter::Reporter(){
	error_reporter_set_callback(NULL);
}

Reporter *Reporter::_instance;

Reporter *
Reporter::Instance(){
	if(_instance==0){
		_instance = new Reporter();
	}
	return _instance;
}

Reporter *getReporter(){
	return Reporter::Instance();
}

Reporter::~Reporter(){
	error_reporter_set_callback(NULL);
}

void
Reporter::setErrorCallback(error_reporter_callback_t callback, void *client_data){
	this->client_data = client_data;
	error_reporter_set_callback(callback);
}

/*
int
Reporter::reportError(ERROR_REPORTER_CALLBACK_ARGS){
	char msg[REPORTER_MAX_ERROR_MSG];
	vsnprintf(msg,REPORTER_MAX_ERROR_MSG,fmt,args);
	cerr << char(27) << "[32;1m" << msg << char(27) << "[0m";
	return strlen(msg) + 11; // 11 chars worth of escape codes
}
*/

#ifdef ASCXX_USE_PYTHON
int
Reporter::reportErrorPython(ERROR_REPORTER_CALLBACK_ARGS){
	PyObject *pyfunc, *pyarglist, *pyresult;
	pyfunc = (PyObject *)client_data;

	char msg[REPORTER_MAX_ERROR_MSG];
	vsprintf(msg,fmt,args);

	cerr << "reportErrorPython: msg=" << msg ;
	cerr << "reportErrorPython: pyfunc=" << pyfunc << endl;

	pyarglist = Py_BuildValue("(H,z,i,z)",sev,filename,line,msg);             // Build argument list
	pyresult = PyEval_CallObject(pyfunc,pyarglist);     // Call Python
	Py_DECREF(pyarglist);                           // Trash arglist

   	int res = 0;
	if (pyresult) {                                 // If no errors, return int
    	long long_res = PyInt_AsLong(pyresult);
		res = int(long_res);
	}else{
		//cerr << "pyresult = 0"<< endl;
	}

	Py_XDECREF(pyresult);
	return res;
}

void
Reporter::setPythonErrorCallback(PyObject *pyfunc) {
	setErrorCallback(reporter_error_python, (void *) pyfunc);
	Py_INCREF(pyfunc);
	is_python = true;
}

void
Reporter::clearPythonErrorCallback(){
	if(is_python){
		PyObject *pyfunc = (PyObject *)client_data;
		Py_DECREF(pyfunc);
		is_python=false;
	}
	setErrorCallback(NULL);
}

#endif
