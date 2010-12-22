#ifndef ASCXX_NAME_H
#define ASCXX_NAME_H

#include <string>

#include "config.h"
extern "C"{
#include <ascend/general/platform.h>
#include <ascend/compiler/fractions.h>
#include <ascend/compiler/compiler.h>
#include <ascend/compiler/dimen.h>
#include <ascend/compiler/expr_types.h>
#include <ascend/compiler/name.h>
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

