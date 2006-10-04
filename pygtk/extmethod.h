#ifndef ASCXX_EXTMETHOD_H
#define ASCXX_EXTMETHOD_H

#include "config.h"

extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <compiler/extfunc.h>
}

#include "symchar.h"

#ifdef ASCXX_USE_PYTHON
# include <Python.h>
#endif

/**
	This is a wrapper for external methods as returned
	by the getExtMethods method of the Library object. At this stage it
	is purely for extracting meta-data about the ExtMethod.
*/
class ExtMethod {
private:
	const struct ExternalFunc *e;
public:
	ExtMethod();
	ExtMethod(const struct ExternalFunc *);
	ExtMethod(const ExtMethod &);
#ifdef ASCXX_USE_PYTHON
	ExtMethod(PyObject *);
#endif
	const char *getHelp() const;
	const char *getName() const;
	const unsigned long getNumInputs() const;
	const unsigned long getNumOutputs() const;
};

#endif // ASCXX_EXTMETHOD_H
