#include <stdexcept>
using namespace std;

#include "extfn.h"

ExtFn::ExtFn(const struct ExternalFunc *e) : e(e) {
	// nothing else
}

ExtFn::ExtFn(const ExtFn &old) : e(old.e) {
	// nothing else
}

ExtFn::ExtFn(){
	throw runtime_error("Can't create empty ExtFn");
}

const char *
ExtFn::getName() const{
	return ExternalFuncName(e);
}

const char *
ExtFn::getHelp() const{
	return e->help;
}

const unsigned long
ExtFn::getNumInputs() const{
	return NumberInputArgs(e);
}

const unsigned long
ExtFn::getNumOutputs() const{
	return NumberOutputArgs(e);
}
