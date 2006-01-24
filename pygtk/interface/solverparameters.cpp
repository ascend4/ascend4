#include "solverparameters.h"

#include <iostream>
#include <sstream>
using namespace std;

SolverParameters::SolverParameters(const slv_parameters_t &p) : p(p){
	cerr << "CREATED SOLVERPARAMETERS" << endl;
}

SolverParameters::SolverParameters(){
	cerr << "CONSTRUCTED SOLVERPARAMETERS NULL" << endl;
}

SolverParameters::SolverParameters(const SolverParameters &old) : p(old.p){
	// copy constructor
}

const string
SolverParameters::toString() const{
	stringstream ss;
	ss << "SOLVERPARAMETERS:TOSTRING:" << endl;
	ss << "Number of parameters: " << p.num_parms << endl;
	return ss.str();
}

