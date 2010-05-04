#include "solverreporter.h"
#include <iostream>
using namespace std;

#define SOLVERREPORTER_DEBUG 0

SolverReporter::SolverReporter(){
#if SOLVERREPORTER_DEBUG
	CONSOLE_DEBUG("Creating SolverReporter at %p",this);
#endif
	// nothing
}

SolverReporter::~SolverReporter(){
#if SOLVERREPORTER_DEBUG
	CONSOLE_DEBUG("Destroying SolverReporter at %p",this);
#endif
	// nothing
}

int
SolverReporter::report(SolverStatus *status){
	//cerr << "Iteration: " << status->getIterationNum() << endl;
	return 0;
}

void
SolverReporter::finalise(SolverStatus *status){
	if(status->isConverged()){
		cerr << "Converged" << endl;
	}else{
		cerr << "Not converged" << endl;
	}
}
