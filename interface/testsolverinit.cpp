#include "simulation.h"
#include "library.h"
#include "solver.h"

#include <iostream>
using namespace std;

int main(void){

	Library L;
	L.load("johnpye/testlog10.a4c");
	Type t = L.findType("testlog10");
	cerr << "Type = " << t.getName() << endl;
	Simulation S = t.getSimulation("S");
	
	S.build();
	
	cerr << "About to solve..." << endl;

	S.solve(Solver("QRSlv"));
}
