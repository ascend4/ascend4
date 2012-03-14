#include "library.h"
#include "simulation.h"
#include "solver.h"
#include "solverreporter.h"
#include "solverhooks.h"

#include <iostream>
using namespace std;

int main(void){
	SolverReporter R; // contains methods to report solver progress;
	SolverHooks H(&R); // hooks slvreq into Simulation
	SolverHooksManager::Instance()->setHooks(&H); // register our SolverHooks

	Library L;
	L.load("test/slvreq/test2.a4c");
	Type t = L.findType("test2");

	// in here, there is a call to slvreq_assign_hooks.
	Simulation S = t.getSimulation("S",1);

	Method M = t.getMethod("on_load");
	S.run(M);

	CONSOLE_DEBUG("Completed OK");
	// the test1 model contains SOLVE command, so the output will now show that
	// the requested model has been solved.
}
