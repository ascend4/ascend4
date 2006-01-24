#ifndef ASCXX_SOLVERPARAMETERS_H
#define ASCXX_SOLVERPARAMETERS_H

#include <stdio.h>
#include <utilities/ascConfig.h>
#include <solver/slv_types.h>
#include <solver/rel.h>
#include <solver/logrel.h>
#include <solver/mtx.h>
#include <general/list.h>
#include <solver/slv_common.h>

#include "solver.h"

#include <string>

class SolverParameters{
private:
	slv_parameters_t p;

protected:
	friend class Simulation;
	explicit SolverParameters(const slv_parameters_t &);

public:
	SolverParameters();
	SolverParameters(const SolverParameters &);
	const std::string toString() const;
};

#endif
