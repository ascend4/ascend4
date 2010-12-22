#ifndef ASCXX_METHOD_H
#define ASCXX_METHOD_H

#include <string>

#include "config.h"
extern "C"{
#include <ascend/general/platform.h>
#include <ascend/general/list.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/watchpt.h>

#include <ascend/compiler/fractions.h>
#include <ascend/compiler/compiler.h>
#include <ascend/compiler/dimen.h>
#include <ascend/compiler/expr_types.h>
#include <ascend/compiler/proc.h>
#include <ascend/compiler/initialize.h>

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
	Method(struct InitProcedure *initproc);
	Method();
	Method(const Method &);
	~Method();
	struct InitProcedure *getInternalType() const;
	const char *getName() const;
	SymChar getSym() const;
};

#endif
