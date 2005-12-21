#ifndef SWIG_VARIABLE_H
#define SWIG_VARIABLE_H

#include <string>

extern "C"{
#include <utilities/ascConfig.h>
#include <solver/slv_types.h>
#include <solver/var.h>
}

/**
	This is a wrapper for the var_variable type in ASCEND. This
	type is used in reporting the variables in an instance, including
	when looking for 'eligible' variables which can be fixed.
*/
class Variable{
private:
	slv_system_t s;
	struct var_variable *var;
	std::string name;

public:
	Variable();
	Variable(slv_system_t s, var_variable *var);
	~Variable();

	const std::string &getName();
};

#endif
