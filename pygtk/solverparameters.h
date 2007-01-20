#ifndef ASCXX_SOLVERPARAMETERS_H
#define ASCXX_SOLVERPARAMETERS_H

extern "C"{
#include <solver/slv_common.h>
}

#include "solver.h"
#include "solverparameter.h"

#include <string>

// Iterator class for SolverParameters :-)

class SolverParameterIterator;
class SolverParameter;
class Integrator;
	
/// Wrapper class for slv_parameters_t
class SolverParameters{

	typedef SolverParameterIterator iterator;

private:
	slv_parameters_t p;

protected:
	friend class Simulation;
	friend class Integrator;

	explicit SolverParameters(const slv_parameters_t &);
	slv_parameters_t &getInternalType();
	inline const slv_parameters_t &getInternalTypeConst() const{return p;}

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
