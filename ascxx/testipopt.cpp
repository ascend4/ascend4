#include "library.h"
#include "simulation.h"
#include "solver.h"
#include "solverreporter.h"
#include <cstdlib>

#include <iostream>
using namespace std;

int main(void){
	//cerr << "env var ASCENDSOLVERS = " << getenv("ASCENDSOLVERS") << endl;
	
	Library L;

#if 0
	cerr << "available solvers: ";
	vector<Solver> vec = getSolvers();
	for (const auto& item : vec) {
		cerr << item.getName() << " ";
	}
	cerr << endl;
#endif 
	
#define TESTNAME "test6"
	L.load("test/ipopt/" TESTNAME ".a4c");
	Type t = L.findType(TESTNAME);
#undef TESTNAME
	Simulation S = t.getSimulation("S",1);
	SolverReporter R;
	try{
		S.solve(Solver("IPOPT"),R);
	}catch(runtime_error &e){
		cerr << "ERROR in solving:" << e.what() << endl;
		exit(1);
	}
	S.run(t.getMethod("self_test"));

	L.clear();
}
