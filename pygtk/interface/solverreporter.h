#ifndef ASCXX_SOLVERREPORTER_H
#define ASCXX_SOLVERREPORTER_H

#ifdef ASCXX_USE_PYTHON
# include <Python.h>
#endif

class SolverReporter{
public:
	Reporter();
	virtual ~Reporter();
	virtual int report(const SolverStatus &) const;
	void setClientData();
};

#ifdef ASCXX_USE_PYTHON

class PythonSolverReporter{
private:

public:
	PythonSolverReporter(PyObject *pyfunc);
	~PythonSolverReporter();

	virtual int report(const SolverStatus &) const;
};

#endif

#endif // ASCXX_SOLVERREPORTER_H
