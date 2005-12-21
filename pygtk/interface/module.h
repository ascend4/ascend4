#ifndef SWIG_MODULE_H
#define SWIG_MODULE_H

extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <general/list.h>
#include <compiler/module.h>
}

#include "type.h"

class Module{
private:
	const module_t *t;

public:
	Module();
	Module(const module_t *t);

	/// Name of the current module (as identified by ASCEND)
	const char *getName() const;

	/// Modification time (see <time.h>) for the current module
	const struct tm* getMtime() const;

	/// Filename for the current module
	const char *getFilename() const;

	/// Return the internal representation of the module
	const struct module_t *getInternalType() const;
};

#endif
