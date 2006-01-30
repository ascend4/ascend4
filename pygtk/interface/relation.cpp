#include <iostream>
#include <stdexcept>
using namespace std;

#include "relation.h"
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


Relation::Relation(){
	sim=NULL;
	rel=NULL;
}

Relation::Relation(const Relation &old) : sim(old.sim), rel(old.rel){
	// copy ctor
}

Relation::Relation(Simulation *sim, struct rel_relation *rel) : sim(sim), rel(rel){
	if(rel==NULL)throw runtime_error("Relation::Relation: rel is NULL");
}

const string
Relation::getName() const{
	char *n = WriteInstanceNameString((struct Instance *)rel_instance(rel),sim->getModel().getInternalType());
	string name = n;
	ascfree(n);

	return name;
}
