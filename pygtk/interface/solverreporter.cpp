#include "solverreporter.h"
#include <iostream>
using namespace std;

SolverReporter::SolverReporter(){
	// nothing
}

SolverReporter::~SolverReporter(){
	// nothing
}

SolverReporter::report(const SolverStatus &status) const;
	cerr << "Iteration: " << status.getIterationNum() << endl;
}

//-----

PythonSolverReporter::PythonSolverReporter(PyObject *pyfunc){
	this->pyfunc = pyfunc;
	Py_INCREF(pyfunc);
}

PythonSolverReporter::~PythonSolverReporter(){
	Py_DECREF(this->pyfunc);
}

int
PythonSolverReporter::report(const SolverStatus &status) const;
	pyarglist = Py_BuildValue("(o)",status); // THIS WON'T WORK :-D
	pyresult = PyEval_CallObject(pyfunc,pyarglist);
	Py_DECREF(pyarglist);
}


class SolverReporter{
public:
	Reporter();
	virtual ~Reporter();
	virtual int report(const SolverStatus &) const;
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
