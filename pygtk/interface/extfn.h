#ifndef ASCXX_EXTFN_H
#define ASCXX_EXTFN_H

extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <compiler/extfunc.h>
}

#include "symchar.h"

class ExtFn {
private:
	const struct ExternalFunc *e;
public:
	ExtFn();
	ExtFn(const struct ExternalFunc *);
	ExtFn(const ExtFn &);
	const char *getHelp() const;
	const char *getName() const;
	const unsigned long getNumInputs() const;
	const unsigned long getNumOutputs() const;
};

#endif // ASCXX_EXTFN_H
