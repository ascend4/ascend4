#ifndef ASCXX_SOLVER_H
#define ASCXX_SOLVER_H

#include <string>
#include <vector>

extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/instance_enum.h>
#include <solver/slv_types.h>
#include <solver/mtx.h>
#include <solver/var.h>
#include <solver/rel.h>
#include <solver/discrete.h>
#include <solver/conditional.h>
#include <solver/logrel.h>
#include <solver/bnd.h>
#include <solver/linsol.h>
#include <solver/linsolqr.h>
#include <solver/slv_common.h>
#include <solver/slv_client.h>
}

class Solver;

/**
	Some global functions
*/
void registerStandardSolvers();
void registerSolver(SlvRegistration regfuncptr);
const std::vector<Solver> getSolvers();

/**
	This is a rather problematic wrapper for slv_client.h. It's hard because registerd solvers
	are referenced primarily by index, not by pointer etc, so given the index, we always have
	to look up the solver in some way or other.	
*/
class Solver{
private:
	std::string name;
public:
	Solver(const Solver &);
	Solver(const std::string &name);

	const int getIndex() const;
	const std::string& getName() const;
};

#endif
