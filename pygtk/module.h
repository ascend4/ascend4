#ifndef ASCXX_MODULE_H
#define ASCXX_MODULE_H

#include "config.h"
extern "C"{
#include <utilities/ascConfig.h>
#include <compiler/compiler.h>
#include <general/list.h>
#include <compiler/module.h>
}

#include "type.h"

/**
	A Module in ASCEND is an opened version of an A4L or A4C file. You
	can have multiple versions of a file active in memory at a time,
	theoretically, although no serious effort to support this in the
	PyGTK interface has yet been made.

	To view what types are present in a given Module, you currently
	need to query the Library object.
*/
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
