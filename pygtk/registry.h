/**
	@file
	This head files provides SWIGed access to the importhandler 'shared pointer'
	mechanism used to pass data from 'extpy' to 'ascpy' in particular (and
	hopefully between other scripting languages/GUI combinations in future)
*/

#ifndef ASCXX_REGISTRY_H
#define ASCXX_REGISTRY_H

#include "config.h"

#ifdef ASCXX_USE_PYTHON
# include <Python.h>
#endif

/*
extern "C"{
#ifdef ASCXX_USE_PYTHON
ASC_IMPORT(void *) importhandler_getsharedpointer(const char *);
#endif
}
*/

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
