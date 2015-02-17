#include <iostream>
#include <stdexcept>

#include "disvar.h"
#include "simulation.h"

extern "C"{
#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>

#include <ascend/general/dstring.h>

#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/instance_io.h>
}

using namespace std;

Disvar::Disvar(){
	sim=NULL;
	disvar=NULL;

	// default ctor
}

Disvar::Disvar(const Disvar &old) : sim(old.sim), disvar(old.disvar){
	// copy ctor
}

Disvar::Disvar(Simulation *sim, struct dis_discrete *disvar) : sim(sim), disvar(disvar){
	if(disvar==NULL)throw runtime_error("Disvar::Disvar: disvar is NULL");
}

const string
Disvar::getName() const{
	if(disvar==NULL)throw runtime_error("Disvar::getName: disvar is NULL");
	char *n = WriteInstanceNameString((struct Instance *)dis_instance(disvar),sim->getModel().getInternalType());
	if(n==NULL)throw runtime_error("Disvar::getName: n is NULL");
	string name = n;
	ascfree(n);

	return name;
}

const double
Disvar::getValue() const{
	return dis_value(disvar);
}

Instanc
Disvar::getInstance(){
	return Instanc((struct Instance *)dis_instance(disvar),"disvarfromsolver");
}
