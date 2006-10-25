#include "registry.h"

extern "C"{
#include <compiler/importhandler.h>
}

#include <stdexcept>

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

Simulation &
Registry::getSimulation(const char *key){
	// how dangerous is this!
	Simulation *s = (Simulation *)importhandler_getsharedpointer(key);
	if(s==NULL){
		throw std::runtime_error("Simulation pointer was NULL");
	}
	CONSOLE_DEBUG("Pointer value is %p",s);
	return (Simulation &)(*s);
}

#ifdef ASCXX_USE_PYTHON

void
Registry::setPyObject(const char *key, PyObject *val){
	importhandler_setsharedpointer(key,(void *)val);
}

#endif
