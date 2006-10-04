#include "registry.h"

extern "C"{
#include <compiler/importhandler.h>
}

void
Registry::setInteger(const char *key, int val){
	importhandler_setsharedpointer(key,(void *)val);
}

#ifdef ASCXX_USE_PYTHON

void
Registry::setPyObject(const char *key, PyObject *val){
	importhandler_setsharedpointer(key,(void *)val);
}

#endif
