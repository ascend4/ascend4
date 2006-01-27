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
#include "solverparameter.h"

#include <string>

// Iterator class for SolverParameters :-)

class SolverParameterIterator;
class SolverParameter;
	
/// Wrapper class for slv_parameters_t
class SolverParameters{

	typedef SolverParameterIterator iterator;

private:
	slv_parameters_t p;

protected:
	friend class Simulation;
	explicit SolverParameters(const slv_parameters_t &);

	friend class SolverParameterIterator;

public:
	SolverParameters();
	SolverParameters(const SolverParameters &);

	const std::string toString() const;

	const int getLength() const; ///< Number of parameters
	SolverParameter getParameter(const int &) const;

	const SolverParameterIterator begin() const;
	const SolverParameterIterator end() const;
};

#endif
