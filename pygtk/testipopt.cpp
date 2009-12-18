#include "library.h"
#include "simulation.h"
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
	S.run(t.getMethod("bound_self"));
	S.run(t.getMethod("default_self"));
	SolverReporter R;
	S.solve(Solver("IPOPT"),R);
	S.run(t.getMethod("self_test"));
}
