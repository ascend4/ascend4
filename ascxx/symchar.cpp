#include <stdexcept>
using namespace std;

extern "C"{
#include <ascend/compiler/cmpfunc.h>
}

#include "symchar.h"

/*
SymChar::SymChar(){
	throw runtime_error("Can't create empty SymChar");
}
*/
SymChar::SymChar(const SymChar &old) : sc(old.sc){
	// nothing else
}

SymChar::SymChar(const string &name){
	sc = AddSymbol(name.c_str());
}


SymChar::SymChar(const char *name){
	sc = AddSymbol(name);
}

#if 0
SymChar::SymChar(const symchar *hash) : sc(){
	cerr << "CREATING SymChar from symchar*, '" << toString() << "'" << endl;
	// nothing else
}
#endif

SymChar::SymChar(const struct InstanceName &in) {

	//cerr << "Symchar(InstanceName) called" << endl;

	if(InstanceNameType(in) == IntArrayIndex){
		char n[50];
		snprintf(n,50,"%ld",InstanceIntIndex(in));
		sc = AddSymbol(n);
	}else{
		sc = InstanceNameStr(in);
	}

	if(sc==NULL){
		throw runtime_error("Invalid name in SymChar::SymChar(InstanceName)");
	}
	//cerr << "CREATING SymChar from InstanceName, '" << getName() << "'" << endl;
}

const char *
SymChar::toString() const{
	return SCP(sc);
}

bool
SymChar::operator==(const SymChar &other) const{
	if(sc == other.sc)return true;
	return false;
}

bool
SymChar::operator<(const SymChar &other) const{
	return CmpSymchar(sc,other.sc) == -1;
}

const symchar *
SymChar::getInternalType() const{
	return sc;
}

ostream& operator << (ostream& os, const SymChar& s){
	return os << s.toString();
}
