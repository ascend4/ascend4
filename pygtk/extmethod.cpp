#include "extmethod.h"

#include <stdexcept>
using namespace std;

#ifndef ASCXX_USE_PYTHON
# error "Where's ASCXX_USE_PYTHON?"
#endif

/*--- WARNING ---
	In the C++ interface I'm trying to make the nomenclature
	a bit more systematic. 'ExternalFunc' as listed end up
	being things that you can call like 'asc_free_all_vars' --
	they are effectively externally-defined methods on ASCEND
	models, so I'll call then 'external methods'.

	Meanwhile, external relations I will call 'ExtRelation'.

	External functions, if ever implemented, will be for
	functions like 'sin', 'exp', etc.
*/

ExtMethod::ExtMethod(const struct ExternalFunc *e) : e(e) {
	// nothing else
}

ExtMethod::ExtMethod(const ExtMethod &old) : e(old.e) {
	// nothing else
}

ExtMethod::ExtMethod(){
	throw runtime_error("Can't create empty ExtMethod");
}

#ifdef ASCXX_USE_PYTHON
/**
	Declare and register a new external script method from Python

	@TODO not sure if this is the right place for this... problems when creating
	the method might cause problems. Perhaps it should be in the 'library'
	object instead?
*/
ExtMethod::ExtMethod(PyObject *obj){
	CONSOLE_DEBUG("CREATING EXTERNAL PYTHON METHOD");

	ExtMethodRun *runfn;
	runfn = (ExtMethodRun *)importhandler_getsharedpointer("extpy.method");

	CreateUserFunctionMethod("myextpyfn",runfn,0,"external python function",(void *)obj);
	e = LookupExtFunc("myextpyfn");
}
#endif


const char *
ExtMethod::getName() const{
	return ExternalFuncName(e);
}

const char *
ExtMethod::getHelp() const{
	return e->help;
}

const unsigned long
ExtMethod::getNumInputs() const{
	return NumberInputArgs(e);
}

const unsigned long
ExtMethod::getNumOutputs() const{
	return NumberOutputArgs(e);
}
