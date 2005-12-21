#include <iostream>
#include <stdexcept>
using namespace std;

#include "method.h"

Method::Method(){
	throw runtime_error("Can't create new Methods, use Type.getMethods instead");
}

Method::Method(struct InitProcedure *initproc) : initproc(initproc){
	//cerr << "CREATED METHOD, name = " << SCP( initproc->name ) << "..."<< endl;
}

Method::~Method(){
	//cerr << "DESTROYED METHOD" << endl;
}

struct InitProcedure *
Method::getInternalType() const{
	return initproc;
}

const char*
Method::getName() const{
	return SCP( initproc->name );
}

SymChar
Method::getSym() const{
	/// @TODO this is not efficient. couldn't make it work the right way though...
	return SymChar( SCP( initproc->name ));
}
