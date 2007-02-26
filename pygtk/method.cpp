#include <iostream>
#include <stdexcept>
using namespace std;

#include "method.h"

Method::Method() : initproc(NULL){
	cerr << "EMPTY METHOD CREATED" << endl;
}

Method::Method(const Method &old) : initproc(old.initproc){
	// nothing else
}

Method::Method(struct InitProcedure *initproc) : initproc(initproc){
	//cerr << "CREATED METHOD, name = " << SCP( initproc->name ) << "..."<< endl;
}

Method::~Method(){
	//cerr << "DESTROYED METHOD" << endl;
}

struct InitProcedure *
Method::getInternalType() const{
	if(initproc==NULL)throw runtime_error("NULL initproc value");
	return initproc;
}

const char*
Method::getName() const{
	if(initproc==NULL)throw runtime_error("NULL initproc value");
	return SCP( ProcName(initproc) );
}

SymChar
Method::getSym() const{
	/// @TODO this is not efficient. couldn't make it work the right way though...
	return SymChar(getName());
}
