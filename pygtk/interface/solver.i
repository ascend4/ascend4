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

class Simulation : public Instanc{
public:
	Simulation(Instance *&, const SymChar &name);
	Instanc &getModel();
	std::vector<Variable> getFixableVariables();
	void build();
	const bool check();
	void checkDoF() const;
	void run(const Method &);
	void setSolver(Solver&);
	void solve(Solver s);
	SolverParameters getSolverParameters() const;
	void setSolverParameters(SolverParameters&);
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

