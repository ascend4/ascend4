#include "config.h"
#include "reporter.h"

#include <cstdio>
#include <iostream>
using namespace std;

#ifndef ASCXX_USE_PYTHON
# error "Where's ASCXX_USE_PYTHON?"
#endif


static const int REPORTER_MAX_ERROR_MSG = ERROR_REPORTER_MAX_MSG;

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
#ifndef PY_SSIZE_T_CLEAN
# error "PY_SSIZE_T_CLEAN needs to have been set in config.h"
#endif
#include <Python.h>

int 
Reporter::reportErrorPython(ERROR_REPORTER_CALLBACK_ARGS){
    /* Make sure we own the GIL even if this callback comes from a C++ thread */
    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject *pyfunc = static_cast<PyObject *>(client_data);
    if (!PyCallable_Check(pyfunc)) {
        std::cerr << "client_data is not callable\n";
        PyGILState_Release(gstate);
        return 0;
    }

    char msg[REPORTER_MAX_ERROR_MSG];
    vsnprintf(msg, sizeof msg, fmt, args);

    Py_ssize_t msglen = static_cast<Py_ssize_t>(strlen(msg));

    /* (H, z, i, s#)  →  (severity, filename|None, line, message, length) */
    PyObject *pyargs = Py_BuildValue("(Hzi s#)",
                                     sev,
                                     filename,
                                     line,
                                     msg, msglen);

    if (!pyargs) {            /* argument packing failed */
        PyErr_Print();
        PyGILState_Release(gstate);
        return 0;
    }

    PyObject *pyresult = PyObject_CallObject(pyfunc, pyargs);
    Py_DECREF(pyargs);

    int rc = 0;
    if (pyresult) {
        rc = static_cast<int>(PyLong_AsLong(pyresult));
        Py_DECREF(pyresult);
    } else {
        PyErr_Print();        /* show Python traceback */
    }

    PyGILState_Release(gstate);
    return rc;
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
