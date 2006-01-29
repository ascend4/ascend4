#ifndef ASCXX_SIMULATION_H
#define ASCXX_SIMULATION_H

#include <string>
#include <vector>


#include "symchar.h"
#include "type.h"
#include "instance.h"
#include "variable.h"

extern "C"{
#include <compiler/createinst.h>
#include <solver/slv_types.h>
}

class Solver;
class SolverParameters;
class IncidenceMatrix;

#ifndef ASCEND_INCDIR
#define ASCEND_INCDIR "/home/john/src/ascend/trunk/base/generic/lib"
#endif
#ifndef ASCEND_LIBDIR
#define ASCEND_LIBDIR "/home/john/src/ascend/trunk/base/jam/Release/linux"
#endif
#ifndef ASCEND_TMPDIR
#define ASCEND_TMPDIR "/tmp"
#endif
#ifndef ASCEND_MAKEFILEDIR
#define ASCEND_MAKEFILEDIR_1 "/home/john/src/ascend/trunk/pygtk/interface"
#define ASCEND_MAKEFILEDIR "/home/john/src/ascend/trunk/base/generic/lib"
#endif

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

private:
	Instanc simroot;
	slv_system_structure *sys;
	bool is_built;

	// options to pass to BinTokenSetOptions
	/// TODO these should probably be put somewhere else
	std::string *bin_srcname;
	std::string *bin_objname;
	std::string *bin_libname;
	std::string *bin_cmd;
	std::string *bin_rm;

protected:
	slv_system_structure *getSystem();
	
public:
	explicit Simulation(Instance *i, const SymChar &name);
	Simulation(const Simulation &);
	~Simulation();

	Instanc &getModel();
	void run(const Method &method);
	void checkDoF() const;
	const bool check();
	void build();
	void solve(Solver s);
	std::vector<Variable> getFixableVariables();
	void write();

	void setSolver(Solver &s);
	const Solver getSolver() const;

	SolverParameters getSolverParameters() const;
	void setSolverParameters(SolverParameters &);

	IncidenceMatrix getIncidenceMatrix();
};


#endif
