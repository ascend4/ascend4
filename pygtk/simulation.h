#ifndef ASCXX_SIMULATION_H
#define ASCXX_SIMULATION_H

#include <string>
#include <vector>
#include <map>

#include "symchar.h"
#include "type.h"
#include "instance.h"
#include "variable.h"
#include "relation.h"

#include "config.h"
extern "C"{
#include <compiler/createinst.h>
#include <solver/slv_types.h>
}

class Solver;
class SolverParameters;
class SolverStatus;
class IncidenceMatrix;
class SolverReporter;

/**
	A class to contain singularity information as returned by the DOF
	function slvDOF_structsing.
*/
class SingularityInfo{
public:
	bool isSingular() const;
	std::vector<Relation> rels; /**< relations involved in the singularity */
	std::vector<Variable> vars; /**< variables involved in the singularity */
	std::vector<Variable> freeablevars; /**< vars that should be freed */
};

enum StructuralStatus{
	ASCXX_DOF_UNDERSPECIFIED=1,
	ASCXX_DOF_SQUARE=2, /* = everything's ok */
	ASCXX_DOF_OVERSPECIFIED=4,
	ASCXX_DOF_STRUCT_SINGULAR=3
};

/**
	@TODO This class is for *Simulation* instances.

	Handle instantiating, running initialisation functions, solving
	and outputing results of solutions.

	In ASCEND C-code, a simulation is a special type of Instance. It
	has a 'simulation root' instance which often needs to be used for
	solving, inspecting, etc, rather than the simulation instance itself.
*/
class Simulation : public Instanc{
	friend class IncidenceMatrix;
	friend class SolverStatus;
	friend class Integrator;

private:
	Instanc simroot;
	slv_system_structure *sys;
	bool is_built;
	SingularityInfo *sing; /// will be used to store this iff singularity found

	// options to pass to BinTokenSetOptions
	/// @TODO these should probably be put somewhere else
	std::string *bin_srcname;
	std::string *bin_objname;
	std::string *bin_libname;
	std::string *bin_cmd;
	std::string *bin_rm;

	int activeblock;

protected:
	slv_system_structure *getSystem();
	Simulation();

public:
	explicit Simulation(Instance *i, const SymChar &name);
	Simulation(const Simulation &);
	~Simulation();

	Instanc &getModel();

	void runDefaultMethod();
	void run(const Method &method);
	void run(const Method &method, Instanc &model);
	enum StructuralStatus checkDoF() const;
	void checkInstance();
	void build();
	void solve(Solver s, SolverReporter &reporter);
	std::vector<Variable> getFixableVariables();
	std::vector<Variable> getVariablesNearBounds(const double &epsilon=1e-4);

	void write();

	void setSolver(Solver &s);
	const Solver getSolver() const;

	SolverParameters getSolverParameters() const;
	void setSolverParameters(SolverParameters &);

	IncidenceMatrix getIncidenceMatrix();

	const std::string getInstanceName(const Instanc &) const;

	void processVarStatus();
	const int getNumVars();

	const int getActiveBlock() const;

	std::vector<Variable> getFreeableVariables();
	bool checkStructuralSingularity();
	const SingularityInfo &getSingularityInfo() const;
};


#endif
