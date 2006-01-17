#include "simulation.h"
#include "library.h"
#include "solver.h"

#include <iostream>
using namespace std;

int main(void){

	Library L;
	L.load("johnpye/extfn/jdpipe.a4c");
	Type t = L.findType("test_jdpipe");
	cerr << "Type = " << t.getName() << endl;
	Simulation S = t.getSimulation("S");
	
	S.build();
	
	cerr << "About to solve..." << endl;

	S.solve(Solver("QRSlv"));
}
