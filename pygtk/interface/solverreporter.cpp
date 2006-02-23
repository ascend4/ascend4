#include "solverreporter.h"
#include <iostream>
using namespace std;

SolverReporter::SolverReporter(){
	// nothing
}

SolverReporter::~SolverReporter(){
	// nothing
}

int
SolverReporter::report(SolverStatus *status){
	cerr << "Iteration: " << status->getIterationNum() << endl;
	return 0;
}
