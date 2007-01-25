#include "simulation.h"
#include "library.h"
#include "solver.h"
#include "solverreporter.h"
#include "integrator.h"
#include "solverparameters.h"
#include "integratorreporter.h"

#include <iostream>
using namespace std;

int main(void){

	Library L;
	L.load("johnpye/idadenx.a4c");

	Type T = L.findType("idadenx");
	Simulation M = T.getSimulation("sim");

//	M.setSolver(Solver("QRSlv"));
	
	M.build();

	Integrator I(M);

	I.setEngine("IDA");
	SolverParameters P = I.getParameters();
	P.getParameter("safeeval").setBoolValue(false);
	P.getParameter("linsolver").setStrValue("DENSE");
	I.setParameters(P);

	IntegratorReporterConsole R(&I);
	I.setReporter(&R);

	I.setLogTimesteps(UnitsM("s"), 0.4, 4e10, 11);

	I.analyse();
	I.solve();
}
