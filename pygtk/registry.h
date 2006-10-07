/**
	Provide methods to registry GUI objects with libascend
	so that they can be accessed from script methods there.
*/

#ifndef ASCXX_REGISTRY_H
#define ASCXX_REGISTRY_H

#include "config.h"

#ifdef ASCXX_USE_PYTHON
# include <Python.h>
#endif

#include "instance.h"

class Registry{
public:
	void setInteger(const char *key, int value);
	void setPointer(const char *key, void *value);
	Instanc *getInstance(const char *key);
#ifdef ASCXX_USE_PYTHON
	void setPyObject(const char *key, PyObject *obj);
#endif
};

#endif /* ASCXX_REGISTRY_H */
