#ifndef ASCXX_SOLVER_PARAMETER_H
#define ASCXX_SOLVER_PARAMETER_H

#include <string>
#include <vector>

extern "C"{
#include <stdio.h>
#include "utilities/ascConfig.h"
#include "solver/slv_types.h"
#include "solver/rel.h"
#include "solver/logrel.h"
#include "solver/mtx.h"
#include "general/list.h"
#include "solver/slv_common.h"
}

class SolverParameter{
private:
	struct slv_parameter *p;

public:
	explicit SolverParameter(slv_parameter *);

	const std::string getName() const;
	const std::string getDescription() const;
	const std::string getLabel() const;
	const int &getNumber() const;
	const int &getPage() const;

	const bool isInt() const;
	const bool isBool() const;
	const bool isStr() const;
	const bool isReal() const;
	
	// The following throw execeptions unless the parameter type is correct
	const int &getIntValue() const;
	const int &getIntLowerBound() const;
	const int &getIntUpperBound() const;

	const bool getBoolValue() const;

	const std::string getStrValue() const;
	const std::vector<std::string> getStrOptions() const;

	const double &getRealValue() const;
	const double &getRealLowerBound() const;
	const double &getRealUpperBound() const;

	const std::string toString() const;
};

#endif
