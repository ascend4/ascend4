#include <iostream>
#include <stdexcept>
#include <sstream>
using namespace std;

#include "name.h"

extern "C"{
#include <ascend/general/dstring.h>
#include <ascend/general/platform.h>

#include <ascend/compiler/expr_types.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/nameio.h>
}

Nam::Nam(){
	throw runtime_error("Can't create new Nam objects");
}

Nam::Nam(struct Name *name) : name(name){
	// nothing else
}

Nam::Nam(const SymChar &sym){
	//cerr << "CREATING NAME from SymChar '" << sym << "'" << endl;
	name = CreateIdName(sym.getInternalType());
}

Nam::~Nam(){
	// cerr << "DESTROY NAME" << endl;
	DestroyName(name);
}

const string
Nam::getName() const{
	/// @TODO Make this more efficient...
	stringstream ss;
	char *longname = WriteNameString(name);
	ss << longname;
	return ss.str();
}

struct Name *
Nam::getInternalType() const{
	return name;
}
	
