#ifndef ASCXX_TYPE_H
#define ASCXX_TYPE_H

#include "config.h"

#include <vector>

extern "C"{
#include <ascend/compiler/type_desc.h>
}

class Simulation;
class Module;

#include "symchar.h"
#include "method.h"
#include "dimensions.h"

/**
	A model type as loaded from an ASCEND a4c file. For example, a type
	might be a 'test_controller' from the simple_fs.a4l example file.

	Once you have a type, you can create an instance of it (getInstance)
	which will then allow you to start on solving it.
*/
class Type{
private:
	const TypeDescription *t;
public:
	Type();
	Type(const TypeDescription *t);
	const SymChar getName() const;
	const int getParameterCount() const;
	const TypeDescription *getInternalType() const;
	// workaround for bug #494:
	Simulation getSimulation(const SymChar &name, const bool rundefaultmethod);
	//Simulation getSimulation(const SymChar &name="sim", const bool &rundefaultmethod=true);
	const Dimensions getDimensions() const;
	std::vector<Method> getMethods() const;
	Method getMethod(const SymChar &name) const; ///< exception if not found
	const bool isRefinedSolverVar() const; ///< is this type a refinement of solver_var?
	const bool isRefinedAtom() const;
	const bool isRefinedReal() const;
	const bool isRefinedConstant() const;
	const bool isFundamental() const;
	const bool isModel() const;
	const bool hasParameters() const;
	const Type &findMember(const SymChar &name);

	Module getModule() const;

	bool operator<(const Type &other) const;
};

#endif
