#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cfloat>
#include <cmath>
using namespace std;

#include "units.h"
#include "dimensions.h"

extern "C"{
#include <ascend/compiler/cmpfunc.h>
}

UnitsM::UnitsM(){
	int errcode;
	long unsigned pos;
	u = FindOrDefineUnits("?",&pos,&errcode);
	if(errcode){
		// naughty to throw exceptions during ctor.
		throw runtime_error("Can't create wildcard (?)");
	}
}

UnitsM::UnitsM(const struct Units *u) : u(u){
	// nothing else
}

/*
exception
UnitsM::getException(const int &errorcode, const char *ustr, const int &pos){
	const char *s;
	switch(errorcode){
		case 0: return runtime_error("This error should not have been thrown!");
		case 1: s = "Undefined unit used in string"; break;
		case 2: s = "Unbalanced parenthesis"; break;
		case 3: s = "Illegal character"; break;
		case 4: s = "Illegal real value"; break;
		case 5: s = "Oversized identifier or real"; break;
		case 6: s = "Missing operator in real followed by identifier"; break;
		case 7: s = "Term missing after * or / or ("; break;
		case 8: s = "Term missing before * or /"; break;
		case 9: s = "Too many closing parentheses"; break;
		case 10: s = "Bad fraction exponent"; break;
		case 11: raise runtime_error("Invalid UnitsM::getException errorcode = 11");
	}
	exception e = runtime_error(s);
	CONSOLE_DEBUG("Units error: %s",s);
	CONSOLE_DEBUG("%s",ustr);
	char indic[strlen(ustr)+1];
	for(int i=0; i<pos; ++i){
		indic[i] = ' ';
	}
	indic[pos] = '^'; indic[pos+1] = '\0';
	CONSOLE_DEBUG(indic);
	return e;
}
*/

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

const bool
UnitsM::hasLadder() const{
	return UnitsLadderId(u) >= 0;
}

const long
UnitsM::getLadderId() const{
	return UnitsLadderId(u);
}

const long
UnitsM::getLadderRank() const{
	return UnitsLadderRank(u);
}

const UnitsM
UnitsM::getAutoScaledUnits(const double &value_si, const double &lower, const double &upper) const{
	const struct Units *cand = NULL;
	const struct Units *best = u;
	double best_score = DBL_MAX;
	bool have_inrange = false;
	double abs_si;
	long rank;
	long ladder_id;
	double target;

	if(!hasLadder()){
		return *this;
	}
	if(!(lower > 0.0) || !(upper > lower)){
		throw runtime_error("UnitsM::getAutoScaledUnits: require 0 < lower < upper");
	}
	abs_si = fabs(value_si);
	if(abs_si == 0.0){
		return *this;
	}
	ladder_id = getLadderId();
	target = 0.5 * (log10(lower) + log10(upper));

	for(rank = 0; ; ++rank){
		double value;
		double score;
		bool inrange;

		cand = LookupUnitsByLadder(ladder_id,rank);
		if(cand == NULL){
			break;
		}
		value = abs_si / UnitsConvFactor(cand);
		if(value <= 0.0){
			continue;
		}
		inrange = (value >= lower && value < upper);
		if(inrange){
			score = fabs(log10(value) - target);
			if(!have_inrange || score < best_score){
				best = cand;
				best_score = score;
				have_inrange = true;
			}
		}else if(!have_inrange){
			/* fallback: pick the rung giving value closest to 1 */
			score = fabs(log10(value));
			if(score < best_score){
				best = cand;
				best_score = score;
			}
		}
	}

	return UnitsM(best);
}

const bool
UnitsM::operator==(const UnitsM &other) const{
	// because of the FindOrDefineUnits thing, equivalent units will always have the same pointer
	return 0==CmpRealPtrs(getInternalType(),other.getInternalType());
}
