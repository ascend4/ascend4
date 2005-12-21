#include "reporter.h"

#include <stdexcept>
#include <iostream>
using namespace std;

#include "solver.h"

extern "C"{
#include <solver/slv3.h>
}

/**
	We're only implementing the 'first' form of solver registration
	here, where the function pointer is already known (not dynamic
	loading).
*/

Solver::Solver(const int &index){
	cerr << "CREATING SOLVER, index = " << index << endl;
	this->index=index;
}

Solver::Solver(const Solver &old){
	this->index = old.index;
	cerr << "COPIED SOLVER, index = " << index << endl;
}

const int &
Solver::getIndex() const{
	cerr << "SOLVER INDEX RETURNED IS " << index << endl;
	return index;
}

const string
Solver::getName() const{
	const char *name = slv_solver_name(index);
	if(name==NULL){
		error_reporter(ASC_PROG_ERROR,NULL,0,"Invalid solver index '%d'",index);
		throw runtime_error("Solver::getSolverName: Invalid solver index");
	}
	return string(name);
}

//---------------------------------
// global functions for registering solvers and querying the complete list

void
registerSolver(SlvRegistration regfuncptr){
	int res = slv_register_client(regfuncptr,NULL,NULL);
	if(res!=0){
		error_reporter(ASC_PROG_ERROR,NULL,0,"Unable to register solver");
		throw runtime_error("Solver::registerSolver: Unable to register solver");
	}else{
		Solver s(res);
		error_reporter(ASC_PROG_NOTE,NULL,0,"Registered solver '%s' (index %d)", s.getName().c_str(), s.getIndex() );
	} 
}

const vector<Solver>
getSolvers(){
	extern int g_SlvNumberOfRegisteredClients;
	vector<Solver> v;
	for(int i=0; i < g_SlvNumberOfRegisteredClients; ++i){
		v.push_back(Solver(i));
	}
	return v;
}

/**
	Register the solvers which will be accessible via index number. The order you 
	register them determines the resulting index ids, so don't mess around with the
	order of stuff in this function.

	Add to this list as you feel necessary...
*/
void
registerStandardSolvers(){
	registerSolver(slv3_register);
}

