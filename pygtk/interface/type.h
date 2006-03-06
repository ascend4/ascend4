#ifndef SWIG_TYPE_H
#define SWIG_TYPE_H

#include <vector>

extern "C"{
#include <utilities/ascConfig.h>
#include <fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/child.h>
#include <general/list.h>
#include <compiler/module.h>
#include <compiler/childinfo.h>
#include <compiler/slist.h>
#include <compiler/type_desc.h>
}

class Simulation;

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
	Simulation getSimulation(SymChar name);
	const Dimensions getDimensions() const;
	std::vector<Method> getMethods() const;
	const bool isRefinedSolverVar() const; ///< is this type a refinement of solver_var?
	const bool isRefinedAtom() const;
	const bool isRefinedReal() const;
	const bool isRefinedConstant() const;
	const bool hasParameters() const;
};

#endif
