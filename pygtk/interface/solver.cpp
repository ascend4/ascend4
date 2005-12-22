#include "reporter.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
using namespace std;

#include "solver.h"

extern "C"{
#include <solver/slv0.h>
#include <solver/slv1.h>
#include <solver/slv2.h>
#include <solver/slv3.h>
#include <solver/slv9.h>
}

/**
	Create a solver by name (it must already be registered)
	Warning, this ctor throws an exception if the name is invalid!
*/
Solver::Solver(const string &name){
	cerr << "CREATING SOLVER, name = " << name << endl;
	this->name = name;
}

Solver::Solver(const Solver &old){
	this->name = old.name;
	cerr << "COPIED SOLVER, name = " << name << endl;
}

const int
Solver::getIndex() const{
	int index = slv_lookup_client(name.c_str());
	if(index < 0){
		stringstream ss;
		ss << "Unknown or unregistered solver '" << name << "'";
		throw runtime_error(ss.str());
	}
	cerr << "))))))))))))))SOLVER INDEX RETURNED IS " << index << endl;
	return index;
}

const string &
Solver::getName() const{
	return name;
}

//---------------------------------
// global functions for registering solvers and querying the complete list

void
registerSolver(SlvRegistration regfuncptr){
	int newclient =-1;
	int res = slv_register_client(regfuncptr,NULL,NULL,&newclient);
	if(res!=0){
		error_reporter(ASC_PROG_ERROR,NULL,0,"Unable to register solver");
		throw runtime_error("Solver::registerSolver: Unable to register solver");
	}else{
		string name = slv_solver_name(newclient);
		error_reporter(ASC_PROG_NOTE,NULL,0,"Registered solver '%s' (index %d)\n", name.c_str(), newclient );
	} 
}

const vector<Solver>
getSolvers(){
	extern int g_SlvNumberOfRegisteredClients;
	vector<Solver> v;
	for(int i=0; i < g_SlvNumberOfRegisteredClients; ++i){
		v.push_back(Solver(slv_solver_name(i)));
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
	cerr << "------------- REGISTERING SOLVERS -----------------" << endl;
	registerSolver(slv3_register);
	registerSolver(slv9_register);
}

