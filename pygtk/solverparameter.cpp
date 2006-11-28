#include "solverparameter.h"

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
using namespace std;

SolverParameter::SolverParameter(slv_parameter *p) : p(p){
	// created new SolverParameter wrapper
}

const std::string
SolverParameter::getName() const{
	if(!p->name){
		return "";
	}
	return string(p->name);
}

const std::string
SolverParameter::getDescription() const{
	return string(p->description);
}

const std::string
SolverParameter::getLabel() const{
	return string(p->interface_label);
}

const int &
SolverParameter::getNumber() const{
	return p->number;
}

const int &
SolverParameter::getPage() const{
	return p->display;
}

// Parameter type tests

const bool
SolverParameter::isBool() const{
	return p->type == bool_parm;
}

const bool
SolverParameter::isInt() const{
	return p->type == int_parm;
}

const bool 
SolverParameter::isReal() const{
	return p->type == real_parm;
}

const bool
SolverParameter::isStr() const{
	return p->type == char_parm;
}

// Integer parameters

const int &
SolverParameter::getIntValue() const{
	if(!isInt()){
		throw runtime_error("Not an integer parameter");
	}
	return p->info.i.value;
}

const int &
SolverParameter::getIntLowerBound() const{
	if(!isInt()){
		throw runtime_error("Not an integer parameter");
	}
	return p->info.i.low;
}

const int &
SolverParameter::getIntUpperBound() const{
	if(!isInt()){
		throw runtime_error("Not an integer parameter");
	}
	return p->info.i.high;
}

void
SolverParameter::setIntValue(const int &val){
	if(!isInt()){
		throw runtime_error("Not an integer parameter");
	}
	if(isBounded() && (val > getIntUpperBound() || val < getIntLowerBound())){
		stringstream ss;
		ss << "Out of bounds (range is [" << getIntLowerBound() << "," << getIntUpperBound() << "])" << endl;
		throw runtime_error(ss.str());
	}
	p->info.i.value = val;
}

// Boolean parameters

const bool
SolverParameter::getBoolValue() const{
	if(!isBool()){
		throw runtime_error("Not an boolean parameter");
	}
	return p->info.b.value !=0;
}

void
SolverParameter::setBoolValue(const bool &val){
	if(!isBool()){
		throw runtime_error("Not a boolean parameter");
	}
	p->info.b.value = val;
}

// Real parameters

const double &
SolverParameter::getRealValue() const{
	if(!isReal()){
		throw runtime_error("Not an real parameter");
	}
	return p->info.r.value;
}

const double &
SolverParameter::getRealLowerBound() const{
	if(!isReal()){
		throw runtime_error("Not an real parameter");
	}
	return p->info.r.low;
}

const double &
SolverParameter::getRealUpperBound() const{
	if(!isReal()){
		throw runtime_error("Not an real parameter");
	}
	return p->info.r.high;
}

void
SolverParameter::setRealValue(const double &val){
	if(!isReal()){
		throw runtime_error("Not a real parameter");
	}
	if(isBounded() && (val > getRealUpperBound() || val < getRealLowerBound())){
		stringstream ss;
		ss << "Out of bounds (range is [" << getRealLowerBound() << "," << getRealUpperBound() << "])" << endl;
		throw runtime_error(ss.str());
	}
	p->info.r.value = val;
}

// String parameters

const string
SolverParameter::getStrValue() const{
	if(!isStr()){
		throw runtime_error("Not a string parameter");
	}
	return string(p->info.c.value);
}

const vector<string>
SolverParameter::getStrOptions() const{
	if(!isStr()){
		throw runtime_error("Not a string parameter");
	}
	vector<string> v;
	for(int i=0; i< p->info.c.high; ++i){
		v.push_back(p->info.c.argv[i]);
	}
	return v;
};

void
SolverParameter::setStrOption(const int &opt){
	if(!isStr()){
		throw runtime_error("Not a string parameter");
	}
	if(opt < 0 || opt > p->info.c.high){
		throw runtime_error("Invalid option index");
	}
	slv_set_char_parameter(&(p->info.c.value),p->info.c.argv[opt]);
}

void
SolverParameter::setStrValue(const std::string &str){
	if(!isStr()){
		throw runtime_error("Not a string parameter");
	}
	slv_set_char_parameter( &(p->info.c.value),str.c_str() );
}

// To String

const string
SolverParameter::toString() const{
	return "STRING";
}

// Bounded?

const bool
SolverParameter::isBounded() const{
	if(isInt() && getIntLowerBound() < getIntUpperBound()){
		return true;
	}else if(isReal() && getRealLowerBound() < getRealUpperBound()){
		return true;
	}
	return false;
}



