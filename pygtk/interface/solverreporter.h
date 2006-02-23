#ifndef ASCXX_SOLVERREPORTER_H
#define ASCXX_SOLVERREPORTER_H

#include "solverstatus.h"

class SolverReporter{
public:
	SolverReporter();
	virtual ~SolverReporter();

	virtual int report(SolverStatus *status);
	virtual void finalise(SolverStatus *status);
};

#endif // ASCXX_SOLVERREPORTER_H
