#include <stdexcept>
using namespace std;

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

SymChar::SymChar(const symchar *hash) : sc(sc){
	cerr << "CREATING SymChar from symchar*, '" << toString() << "'" << endl;
	// nothing else
}

SymChar::SymChar(const struct InstanceName &in) {

	//cerr << "Symchar(InstanceName) called" << endl;

	if(InstanceNameType(in) == IntArrayIndex){
		char n[50];
		snprintf(n,50,"%d",InstanceIntIndex(in));
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

const symchar *
SymChar::getInternalType() const{
	return sc;
}

ostream& operator << (ostream& os, const SymChar& s){
	return os << s.toString();
}
