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
	@TOD This class is for *Simulation* instances. Should be renamed.

	Handle instantiating, running initialisation functions, solving
	and outputing results of solutions.

	This class has to be called 'Instanc' in C++ to avoid a name clash
	with C. Maybe coulda done it with namespaces but didn't know how.

	This class is renamed back to 'Instance' by SWIG, so use 'Instance'
	when you're in Python.

	In ASCEND C-code, a simulation is a special type of Instance. It
	has a 'simulation root' instance which often needs to be used for
	solving, inspecting, etc, rather than the simulation instance itself.
*/
class Simulation : public Instanc{
private:
	Instanc simroot;
	slv_system_t sys;

	// options to pass to BinTokenSetOptions
	std::string *bin_srcname;
	std::string *bin_objname;
	std::string *bin_libname;
	std::string *bin_cmd;
	std::string *bin_rm;

public:
	Simulation(Instance *i, const SymChar &name);
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
};


#endif
