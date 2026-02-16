#ifndef ASCXX_SOLVERREPORTER_H
#define ASCXX_SOLVERREPORTER_H

#include "solverstatus.h"

class SolverReporter{
public:
	SolverReporter();
	virtual ~SolverReporter();

	virtual int report(SolverStatus *status);
	virtual void finalise(SolverStatus *status);
	virtual void reportProgress(const char *solver_name, const char *message);
};

#endif // ASCXX_SOLVERREPORTER_H
