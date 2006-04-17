#ifndef SWIG_METHOD_H
#define SWIG_METHOD_H

#include <string>

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <general/list.h>
#include <compiler/instance_enum.h>
#include <compiler/watchpt.h>

#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/types.h>
#include <compiler/proc.h>
#include <compiler/initialize.h>

}

#include "symchar.h"

/**
	Initialisation functions are specified
	in models using "METHOD" statements.

	This class acts as a holder for these funtions, so that they
	can be passed from a type to an instance via instanc.run(method).

	(@TODO rename this class to Method or similar)
*/
class Method{
private:
	struct InitProcedure *initproc;
public:
	Method();
	Method(struct InitProcedure *initproc);
	~Method();
	struct InitProcedure *getInternalType() const;
	const char *getName() const;
	SymChar getSym() const;
};

#endif
