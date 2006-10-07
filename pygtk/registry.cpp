#include "registry.h"

extern "C"{
#include <compiler/importhandler.h>
}

void
Registry::setInteger(const char *key, int val){
	importhandler_setsharedpointer(key,(void *)val);
}

void
Registry::setPointer(const char *key, void *val){
	importhandler_setsharedpointer(key,val);
}

Instanc *
Registry::getInstance(const char *key){
	return new Instanc((struct Instance*)importhandler_getsharedpointer(key));
}

#ifdef ASCXX_USE_PYTHON

void
Registry::setPyObject(const char *key, PyObject *val){
	importhandler_setsharedpointer(key,(void *)val);
}

#endif
