#include "config.h"
#include "reporter.h"
#include "solver.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
using namespace std;

extern "C"{
#include <solver/system.h>
#include <solver/slvDOF.h>
#include <solver/slv_stdcalls.h>
}

/**
	Create a solver by name (it must already be registered)
	Warning, this ctor throws an exception if the name is invalid!
*/
Solver::Solver(const string &name){
	//cerr << "CREATING SOLVER, name = " << name << endl;
	this->name = name;
}

Solver::Solver(const Solver &old){
	this->name = old.name;
	//cerr << "COPIED SOLVER, name = " << name << endl;
}

Solver::Solver(){
	//cerr << "RETREIVING SOLVER NAME" << name << endl;
	this->name = "";
}

const int
Solver::getIndex() const{
	int index = slv_lookup_client(name.c_str());
	if(index < 0){
		stringstream ss;
		ss << "Unknown or unregistered solver '" << name << "'";
		throw runtime_error(ss.str());
	}
	//cerr << "))))))))))))))SOLVER INDEX RETURNED IS " << index << endl;
	return index;
}

const string &
Solver::getName() const{
	return name;
}

//---------------------------------
// >>>> GLOBAL FUNCTIONS <<<<
// for registering solvers and querying the complete list

void
registerSolver(SlvRegistration regfuncptr){
	int newclient =-1;
	int res = slv_register_client(regfuncptr,NULL,NULL,&newclient);
	if(res!=0){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Unable to register solver");
		throw runtime_error("Solver::registerSolver: Unable to register solver");
	}else{
		string name = slv_solver_name(newclient);
		cerr << "Registered solver '" << name << "' (index " << newclient << ")" << endl;
	}
}

const vector<Solver>
getSolvers(){
	ASC_DLLSPEC int g_SlvNumberOfRegisteredClients;
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
	//cerr << "------------- REGISTERING SOLVERS -----------------" << endl;
	SlvRegisterStandardClients();
	/*
	registerSolver(slv3_register);
	registerSolver(slv9_register);
	*/
}

