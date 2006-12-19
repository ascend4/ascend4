/*
	SWIG interface for accessing Solver and choosing solver parameters
*/

%include <python/std_vector.i>
%include <python/std_except.i>

%import "ascpy.i"

%{
#include "integrator.h"
#include "integratorreporter.h"
#include "solver.h"
#include "incidencematrix.h"
#include "solverparameter.h"
#include "solverparameters.h"
#include "solverreporter.h"
#include "curve.h"
%}

%pythoncode{
	import types
}

%template(VariableVector) std::vector<Variable>;
%template(RelationVector) std::vector<Relation>;
%template(SolverVector) std::vector<Solver>;

%ignore registerSolver;
%ignore registerStandardSolvers;
%include "solver.h"


%include "simulation.h"
// SOLVE PARAMETERS

%pythoncode{
	class SolverParameterIter:
		def __init__(self, params):
			self.params = params;
			self.index = 0;

		def __iter__(self):
			return self

		def next(self):
			if self.index >= len(self.params):
				raise StopIteration
			p = self.params.getParameter(self.index)
			self.index = self.index +1
			return p
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
		def __getattr(self,index):
			for p in self:
				if p.getName()==index:
					return p
			raise KeyError
		def __getitem__(self,index):
			if type(index) != types.IntType:
				raise TypeError
			return self.getParameter(index)
		def __len__(self):
			return self.getLength()
		def getValue(self,codename):
			for p in self:
				if p.getName()==codename:
					return p.getValue()
			raise KeyError
		def set(self,codename,value):
			for p in self:
				if p.getName()==codename:
					p.setValue(value)
					return
			raise KeyError						
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

%extend SolverParameter{
	%pythoncode{
		def __str__(self):
			if self.isInt(): return "%s = %d" %(self.getName(),self.getIntValue())
			if self.isBool(): return "%s = %s" %(self.getName(),self.getBoolValue())
			if self.isStr(): return "%s = %s" %(self.getName(),self.getStrValue())
			if self.isReal(): return "%s = %f" %(self.getName(),self.getRealValue())
			raise TypeError
		def getValue(self):
			if self.isBool():return self.getBoolValue()
			if self.isReal():return self.getRealValue()
			if self.isInt(): return self.getIntValue()
			if self.isStr(): return self.getStrValue()
			raise TypeError
		def setValue(self,value):
			if self.isBool(): 
				self.setBoolValue(value)
				return
			if self.isReal():
				self.setRealValue(value)
				return
			if self.isInt():
				self.setIntValue(value)
				return
			if self.isStr():
				self.setStrValue(value)
				return
			raise TypeError
	}
}

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


%include "variable.h"

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
	Instanc getInstance() const;
	std::string getRelationAsString() const;
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

%extend Integrator{
	%pythoncode{
		def setParameter(self,name,value):
			""" set the value of a parameter for this integrator """
			P = self.getParameters()
			P.set(name,value)
			self.setParameters(P)
		def getParameterValue(self,name):
			""" retrieve the *value* of the specified parameter """
			P = self.getParameters()
			for p in P:
				if p.getName()==name:
					return p.getValue()
			raise KeyError
	}
}
		
%feature("director") IntegratorReporterCxx;

%ignore ascxx_integratorreporter_init;
%ignore ascxx_integratorreporter_write;
%ignore ascxx_integratorreporter_write_obs;
%ignore ascxx_integratorreporter_close;

%include "integratorreporter.h"
