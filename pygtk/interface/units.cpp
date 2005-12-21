#include <iostream>
#include <stdexcept>
#include <sstream>
using namespace std;

#include "units.h"
#include "dimensions.h"

UnitsM::UnitsM(){
	/// ctor required to keep SWIG happy. don't use it though.
	throw runtime_error("Can't create new Units like this");
}

UnitsM::UnitsM(const struct Units *u) : u(u){
	// nothing else
}

/**
	Parse a units string, create a new UnitsM object from the string.

	UnitsM doesn't allocate any storage so it's OK to throw
	an exception in a ctor
*/
UnitsM::UnitsM(const char *units){
	const struct Units *u = LookupUnits(units);
	if(u==NULL){
		//cerr << "About to create new units '" << units << "'" << endl;
		long unsigned pos;
		int err;
		u = FindOrDefineUnits(units, &pos, &err);
		if(u==NULL){
			char **errv = UnitsExplainError(units, err, pos);
			stringstream ss;
			ss << "Error parsing units: " << errv[0] << endl << errv[1] << endl << errv[2];
			throw runtime_error(ss.str());
		}
	}/*else{
		cerr << "Units '" << units << "' were found in lookup" << endl;
	}*/
	this->u = u;
}

const struct Units *
UnitsM::getInternalType() const{
	return u;
}

const SymChar
UnitsM::getName() const{
	return SymChar(SCP( UnitsDescription(u) ));
}

const Dimensions
UnitsM::getDimensions() const{
	const dim_type *d = UnitsDimensions(u);
	return Dimensions(d);
}

const double
UnitsM::getConversion() const{
	return UnitsConvFactor(u);
}
