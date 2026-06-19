#include "config.h"
#include "reporter.h"
#include "solver.h"
#include "solverreporter.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
using namespace std;

extern "C"{
#include <ascend/system/system.h>
#include <ascend/solver/solver.h>
#include <ascend/solver/slvDOF.h>
}

extern "C" int ascxx_solver_progress_callback(
	const char *solver_name, const char *message, void *user_data
){
	SolverReporter *reporter = reinterpret_cast<SolverReporter *>(user_data);
	if(reporter == NULL)return 0;
	try{
		reporter->reportProgress(solver_name,message);
	}catch(...){
		/* progress callbacks should not abort solver execution */
	}
	return 0;
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
	//cerr << "))))))))))))))SOLVER INDEX RETURNED IS " << index << endl;s
	return index;
}

const string &
Solver::getName() const{
	return name;
}

const string
Solver::getVersion() const{
	char version[256];
	version[0] = '\0';
	if(solver_get_version(name.c_str(),version,sizeof(version))){
		return "";
	}
	return version;
}

//---------------------------------
// >>>> GLOBAL FUNCTIONS <<<<
// for registering solvers and querying the complete list

#if 0
void
registerSolver(SlvRegistration regfuncptr){
	int newclient =-1;
	int res = solver_register(slv_register_client(regfuncptr,NULL,NULL,&newclient);
	if(res!=0){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"Unable to register solver");
		throw runtime_error("Solver::registerSolver: Unable to register solver");
e	}else{
		string name = slv_solver_name(newclient);
		cerr << "Registered solver '" << name << "' (index " << newclient << ")" << endl;
	}
}
#endif

const vector<Solver>
getSolvers(){
	const struct gl_list_t *L = solver_get_engines();
	vector<Solver> v;
	for(unsigned long i=1; i <= gl_length(L); ++i){
		v.push_back(Solver( ( (SlvFunctionsT *)(gl_fetch(L,i)))->name) );
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

void
setSolverInterrupt(const bool &interrupt){
	slv_set_solver_interrupt(interrupt ? 1 : 0);
}

void
setSolverProgressReporter(SolverReporter *reporter){
	if(reporter == NULL){
		slv_clear_progress_callback();
		return;
	}
	slv_set_progress_callback(&ascxx_solver_progress_callback,reinterpret_cast<void *>(reporter));
}
