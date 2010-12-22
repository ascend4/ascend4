#include "simulation.h"
#include "library.h"
#include "solver.h"
#include "solverreporter.h"

#include <iostream>
using namespace std;

int main(void){

	Library L;
	L.load("johnpye/extfn/extfntest.a4c");
	Type t = L.findType("test_extfntest");
	cerr << "Type = " << t.getName() << endl;
	Simulation S = t.getSimulation("S");
	
	S.build();
	
	cerr << "About to solve..." << endl;

	SolverReporter r = SolverReporter();
	S.solve(Solver("QRSlv"),r);
}
