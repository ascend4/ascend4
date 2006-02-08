#include <iostream>
#include <stdexcept>
using namespace std;

#include "variable.h"
#include "simulation.h"

extern "C"{
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>

#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/symtab.h>
#include <compiler/instance_enum.h>
#include <compiler/instance_io.h>
}

Variable::Variable(){
	sim=NULL;
	var=NULL;

	// default ctor
}

Variable::Variable(const Variable &old) : sim(old.sim), var(old.var){
	// copy ctor
}

Variable::Variable(Simulation *sim, struct var_variable *var) : sim(sim), var(var){
	if(var==NULL)throw runtime_error("Variable::Variable: var is NULL");
}

const string
Variable::getName() const{
	char *n = WriteInstanceNameString((struct Instance *)var_instance(var),sim->getModel().getInternalType());
	string name = n;
	ascfree(n);

	return name;
}

const double
Variable::getValue() const{
	return var_value(var);
}

const double
Variable::getNominal() const{
	return var_nominal(var);
}

