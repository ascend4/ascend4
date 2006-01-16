#ifndef ASCXX_REPORTER_H
#define ASCXX_REPORTER_H

#ifdef ASCXX_USE_PYTHON
# include <Python.h>
#endif

#include <cstdarg>


/**
	Comment out this line if you won't be wanted to link to python.
	It shouldn't matter if you *don't* comment this though.
*/

extern "C"{
	#include "error.h"
}

/**
	This class provides C++ abstraction of the error.h error callback
	interface.

	Initially, it's trying to just handling the conveying of error
	messages back to python, but it could be used to pass back
	all sorts of other 'messages' eventually.

	Maybe raising alerts, notifying of the progress of big tasks, etc.

	The client_data pointer allows callback context to be set. This will
	be used to specify which Python function should be used for error
	reporting, in the case of the Python extension to this class.
*/
class Reporter{
private:
	void *client_data;
	Reporter(); // This class will be a singleton
	~Reporter();
	static Reporter *_instance;
#ifdef ASCXX_USE_PYTHON
	bool is_python;
#endif

public:
	static Reporter *Instance();
	void setErrorCallback(error_reporter_callback_t, void *client_data=NULL);

#ifdef ASCXX_USE_PYTHON
	void setPythonErrorCallback(PyObject *pyfunc);
	void clearPythonErrorCallback();
	int reportErrorPython(ERROR_REPORTER_CALLBACK_ARGS);
#endif

};

Reporter *getReporter();

#ifdef ASCXX_USE_PYTHON
// Python-invoking callback function
int reporter_error_python(ERROR_REPORTER_CALLBACK_ARGS);
#endif

#endif // ASCXX_REPORTER_H
