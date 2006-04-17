#ifndef SWIG_VARIABLE_H
#define SWIG_VARIABLE_H

#include <string>

#include "simulation.h"

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <solver/slv_types.h>
#include <solver/var.h>
}

/**
	This is a wrapper for the var_variable type in ASCEND. This
	type is used in reporting the variables in an instance, including
	when looking for 'eligible' variables which can be fixed.
*/
class Variable{
private:
	Simulation *sim;
	struct var_variable *var;

public:
	Variable();
	Variable(const Variable &old);
	Variable(Simulation *sim, var_variable *var);

	const std::string getName() const;
	const double getValue() const;
	const double getNominal() const;
	const double getUpperBound() const;
	const double getLowerBound() const;
};

#endif
