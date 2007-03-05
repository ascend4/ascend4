#ifndef ASCXX_SOLVER_H
#define ASCXX_SOLVER_H

#include <string>
#include <vector>

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/instance_enum.h>
#include <system/slv_client.h>
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
};

/**
	Some global functions
*/
void registerStandardSolvers();
void registerSolver(SlvRegistration regfuncptr);
const std::vector<Solver> getSolvers();

#endif
