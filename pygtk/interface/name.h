#ifndef SWIG_NAME_H
#define SWIG_NAME_H

#include <string>

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/types.h>
#include <compiler/name.h>
}

#include "symchar.h"

/*
	Handles 'temporary names', which are used for example
	when calling methods on instances.

	This class will be renamed to 'Name' when used
	from Python.
*/
class Nam{
private:
	struct Name *name;
public:
	Nam();
	Nam(const SymChar &);
	Nam(struct Name*);
	~Nam();
	struct Name *getInternalType() const;
	const std::string getName() const;
};

#endif

