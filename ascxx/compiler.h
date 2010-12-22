#ifndef ASCXX_COMPILER_H
#define ASCXX_COMPILER_H

/**
	This class holds compiler configuration. At the moment there is only one
	possible compiler, although one day there might be different compilers
	for different model file types.

	During instantiation of a model (Type::getSimulation) we will need to
	access the Compiler object (which is a singleton for the moment, although
	it should not have to be) and query its values.

	The GUI must also access the Compiler object when it wants to set
	configuration options.
*/

#include <string>

class Compiler{
private:
	Compiler();
	~Compiler();

	/* options for bintoken compilation */
	bool use_bintoken;
	bool bintoken_options_sent;
	std::string bt_targetstem;
	std::string bt_srcname;
	std::string bt_objname;
	std::string bt_libname;
	std::string bt_cmd;
	std::string bt_rm;

protected:
	friend class Type;
	void sendBinaryCompilationOptions(); /* pass them down to libascend */
public:
	static Compiler *instance(); /* (as in singleton pattern) */

	const bool getUseRelationSharing() const;
	void setUseRelationSharing(const bool&);

	void setBinaryCompilation(const bool&);
};

/** Compiler access function for use with Python */
Compiler *getCompiler();

#endif
