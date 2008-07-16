#include "simulation.h"
#include "library.h"
#include "solver.h"
#include "solverreporter.h"

#include <iostream>
using namespace std;

int main(void){

	Library L;
	L.load("test/ipopt/test1.a4c");
	Type t = L.findType("test1");
	cerr << "Type = " << t.getName() << endl;
	Simulation S = t.getSimulation("S");
	
	S.build();
	
	cerr << "About to solve..." << endl;

	SolverReporter R;
	S.solve(Solver("IPOPT"),R);
}
