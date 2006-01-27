#include <iostream>
#include <stdexcept>
using namespace std;

#include "units.h"
#include "dimensions.h"

#ifdef ASCXX_WORDY_BASE_DIMENSIONS
const string Dimensions::BASEUNITS[Dimensions::MAX_DIMS] = {
	UNIT_BASE_MASS,UNIT_BASE_QUANTITY,UNIT_BASE_LENGTH, UNIT_BASE_TIME, 
	UNIT_BASE_TEMPERATURE, UNIT_BASE_CURRENCY, 	UNIT_BASE_ELECTRIC_CURRENT, 
	UNIT_BASE_LUMINOUS_INTENSITY, UNIT_BASE_PLANE_ANGLE, UNIT_BASE_SOLID_ANGLE 
};
#else
// Preferred SI abbreviated dimension names
const string Dimensions::BASEUNITS[Dimensions::MAX_DIMS] = {
	"kg","mol","m","s","K","USD","A", 
	"cd", "rad", "sr"
};
#endif

const string
Dimensions::getBaseUnit(const unsigned &i){
	return BASEUNITS[i];
}

Dimensions::Dimensions(){
	// This ctor needs to exist for SWIG to be happy. *shrug*.
	throw runtime_error("Can't create Dimensions like this");
}

Dimensions::Dimensions(const Dimensions &old) : d(old.d){
	// nothing else
}

Dimensions::Dimensions(const dim_type *d) : d(d){
	// nothing else
}

const dim_type *
Dimensions::getInternalType() const{
	return d;
}

// Comparison operators

const bool 
Dimensions::operator<(const Dimensions &d1) const{
	return -1 == CmpDimen(d, d1.getInternalType());
}

const bool 
Dimensions::operator==(const Dimensions &d1) const{
	ERROR_REPORTER_START_HERE(ASC_USER_NOTE);
	FPRINTF(stderr,"Comparing dimensions; this=");
	PrintDimen(stderr,d);
	FPRINTF(stderr,", d1=");
	PrintDimen(stderr,d1.getInternalType());
	FPRINTF(stderr,": comparison result=%d",CmpDimen(d,d1.getInternalType()));
	error_reporter_end_flush();
	if(CmpDimen(d, d1.getInternalType()) == 0)return true;
	return false;
}

const bool
Dimensions::operator!=(const Dimensions &d1) const{
	return 0 != CmpDimen(d, d1.getInternalType());
}

const bool
Dimensions::isDimensionless() const{
	return d==Dimensionless();
}

const bool
Dimensions::isWild() const{
	return IsWild(d);
}


/**
	Get at the internal data structure: find the index of each fundamental dimension
*/
const FRACPART
Dimensions::getFractionNumerator(const unsigned &i) const{
	return Numerator( GetDimFraction(*d,i) );
}

const FRACPART
Dimensions::getFractionDenominator(const unsigned &i) const{
	return Denominator( GetDimFraction(*d,i) );
}
