/*
	SWIG interface for accessing Solver and choosing solver parameters
*/

%{
#include "integrator.h"
#include "integratorreporter.h"
%}

%template(VariableVector) std::vector<Variable>;
%template(RelationVector) std::vector<Relation>;


class Solver{
public:
	Solver(const std::string &name);
	Solver(const Solver &);

	const int &getIndex() const;
	const std::string getName() const;
};

class Simulation : public Instanc{
public:
	Simulation(Instance *&, const SymChar &name);
	Instanc &getModel();
	std::vector<Variable> getFixableVariables();
	std::vector<Variable> getVariablesNearBounds(const double &epsilon=1e-4);
	void build();
	const bool check();
	void checkDoF() const;
	void run(const Method &);
	void setSolver(Solver&);
	const Solver getSolver() const;
	void solve(Solver s,SolverReporter &);
	SolverParameters getSolverParameters() const;
	void setSolverParameters(SolverParameters&);

	IncidenceMatrix getIncidenceMatrix();
	const std::string getInstanceName(const Instanc &) const;

	void processVarStatus();
	const int getNumVars();
	const int getActiveBlock() const;
	void checkConsistency() const;
	void checkStructuralSingularity() const;
};

// SOLVE PARAMETERS

%pythoncode{
	class SolverParameterIter:
		def __init__(self, params):
			self.params = params;
			self.index = 0;

		def next(self):
			self.index = self.index + 1
			if self.index >= self.params.getLength():
				raise StopIteration
			return self.params.getParameter(self.index)
}

class SolverParameters{
public:
	const std::string toString();
	SolverParameters(const SolverParameters &);
	const int getLength() const;
	SolverParameter getParameter(const int &) const;
};

%extend SolverParameters{
	%pythoncode{
		def __iter__(self):
			return SolverParameterIter(self)
		def getitem(self,index):
			return
	}
}

class SolverParameter{
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
	void setIntValue(const int&);

	const bool getBoolValue() const;
	void setBoolValue(const bool&);

	const std::string getStrValue() const;
	const std::vector<std::string> getStrOptions() const;
	void setStrValue(const std::string &);
	void setStrOption(const int &opt);

	const double &getRealValue() const;
	const double &getRealLowerBound() const;
	const double &getRealUpperBound() const;
	void setRealValue(const double&);

	const bool isBounded() const;

	const std::string toString() const;
};

/* Incidence matrix stuff */
typedef enum{
	IM_NULL=0, IM_ACTIVE_FIXED, IM_ACTIVE_FREE, IM_DORMANT_FIXED, IM_DORMANT_FREE
} IncidencePointType;

class IncidencePoint{
public:
	IncidencePoint(const IncidencePoint &);

	int row;
	int col;
	IncidencePointType type;
};

%extend IncidencePoint{
	%pythoncode{
		def __repr__(self):
			return str([ self.row, self.col, int(self.type) ]);
	}
}

%template(IncidencePointVector) std::vector<IncidencePoint>;

class IncidenceMatrix{
public:
	explicit IncidenceMatrix(Simulation &);
	const std::vector<IncidencePoint> &getIncidenceData();
	const int &getNumRows() const;
	const int &getNumCols() const;
	const Variable getVariable(const int &col);
	const Relation getRelation(const int &col);
	const int getBlockRow(const int &row) const;
	const std::vector<Variable> getBlockVars(const int block);
	const std::vector<Relation> getBlockRels(const int block);
	const std::vector<int> getBlockLocation(const int &block) const;
	const int getNumBlocks();
};


/* Variables and relations belong to solvers, so they're here: */

class Variable{
public:
	explicit Variable(const Variable &old);
	const std::string getName() const;
	const double getValue() const;
	const double getNominal() const;
	const double getLowerBound() const;
	const double getUpperBound() const;
	const std::vector<Relation> getIncidentRelations() const;
	const int getNumIncidentRelations() const;
};

%extend Variable {
	%pythoncode{
		def __repr__(self):
			return self.getName()
	}
}

class Relation{
public:
	explicit Relation(const Relation &old);
	const std::string getName();
	const double &getResidual();
	const std::vector<Variable> getIncidentVariables() const;
	const int getNumIncidentVariables() const;
};

%extend Relation {
	%pythoncode{
		def __repr__(self):
			return self.getName()
	}
}


class SolverStatus{
public:
	SolverStatus();
	explicit SolverStatus(const SolverStatus &old);
	void getSimulationStatus(Simulation &);

	const bool isOK() const;
	const bool isOverDefined() const;
	const bool isUnderDefined() const;
	const bool isStructurallySingular() const;
	const bool isInconsistent() const;
	const bool isReadyToSolve() const;
	const bool isConverged() const;
	const bool isDiverged() const;
	const bool hasResidualCalculationErrors() const;
	const bool hasExceededIterationLimit() const;
	const bool hasExceededTimeLimit() const;
	const bool isInterrupted() const;
	const int getIterationNum() const;

	// block structure stuff...

	const int getNumBlocks() const;
	const int getCurrentBlockNum() const;
	const int getCurrentBlockSize() const;
	const int getCurrentBlockIteration() const;
	const int getNumConverged() const; /* previous total size */
	const int getNumJacobianEvals() const;
	const int getNumResidualEvals() const;
	const double getBlockResidualRMS() const;

};

%feature("director") SolverReporter;

class SolverReporter{
public:
	SolverReporter();
	virtual ~SolverReporter();
	virtual int report(SolverStatus *status);
	virtual void finalise(SolverStatus *status);
};

%apply SWIGTYPE *DISOWN { IntegratorReporterCxx *reporter };

%include "integrator.h"

%feature("director") IntegratorReporterCxx;

%ignore ascxx_integratorreporter_init;
%ignore ascxx_integratorreporter_write;
%ignore ascxx_integratorreporter_write_obs;
%ignore ascxx_integratorreporter_close;

%include "integratorreporter.h"
