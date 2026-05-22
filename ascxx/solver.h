#ifndef ASCXX_SOLVER_H
#define ASCXX_SOLVER_H

#include <string>
#include <vector>

#include "config.h"
extern "C"{
#include <ascend/general/platform.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/system/slv_client.h>
}

/**
	This is a rather problematic wrapper for slv_client.h. It's hard because registerd solvers
	are referenced primarily by index, not by pointer etc, so given the index, we always have
	to look up the solver in some way or other.

	Because the index is changeable from session to session, depending on the order of
	registration, we'll use the *name* as the primary key and look up the index as needed.
*/
class Solver{
private:
	std::string name;
public:
	Solver();
	Solver(const Solver &);
	Solver(const std::string &name);

	const int getIndex() const;
	const std::string& getName() const;
	const std::string getVersion() const;
};

class SolverReporter;

/**
	Some global functions
*/
void registerStandardSolvers();
void setAutoRegisterStandardSolvers(bool enabled);
bool getAutoRegisterStandardSolvers();
const std::vector<std::string> getStandardSolvers();
const std::vector<std::string> getStandardSolverImports();
bool isSolverRegistered(const std::string &name);
int loadSolver(const std::string &name);
//void registerSolver(SlvRegistration regfuncptr);
const std::vector<Solver> getSolvers();
void setSolverInterrupt(const bool &interrupt);
void setSolverProgressReporter(SolverReporter *reporter);

#endif
