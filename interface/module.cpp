#include <iostream>
#include <stdexcept>
using namespace std;

#include "module.h"

Module::Module(const module_t *t) : t(t){
	//cerr << "CREATED MODULE" << endl;
}

Module::Module(){
	throw runtime_error("Can't create modules via C++ interface (use Library::load instead)");
}

const char *
Module::getName() const{
	return Asc_ModuleName(t);
}

const struct module_t *
Module::getInternalType() const{
	return t;
}

const struct tm *
Module::getMtime() const{
	return Asc_ModuleTimeModified(t);
}

const char *
Module::getFilename() const{
	return Asc_ModuleFileName(t);
}

