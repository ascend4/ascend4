/*
	SWIG interface for accessing Solver and choosing solver parameters
*/

class Solver{
public:
	Solver(const std::string &name);
	Solver(const Solver &);

	const int &getIndex() const;
	const std::string getName() const;
};

// SOLVE PARAMETERS

%pythoncode{
	class SolverParameterIter:
		def __init__(self, params):
			self.params = params;
			self.index = 0;

		def next(self):
			if self.index >= self.params.getLength():
				raise StopIteration
			self.index = self.index + 1
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

	const bool getBoolValue() const;

	const std::string getStrValue() const;
	const std::vector<std::string> getStrOptions() const;

	const double &getRealValue() const;
	const double &getRealLowerBound() const;
	const double &getRealUpperBound() const;
};

