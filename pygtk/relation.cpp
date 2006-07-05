#include <iostream>
#include <stdexcept>
using namespace std;

#include "relation.h"
#include "simulation.h"
#include "variable.h"

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

const double
Relation::getResidual() const{
	return rel_residual(rel);
}

const std::vector<Variable> 
Relation::getIncidentVariables() const{
	struct var_variable **incid = rel_incidence_list_to_modify(rel);
	int n = rel_n_incidences(rel);
	vector<Variable> v;
	for(int i=0; i<n; ++i){
		v.push_back(Variable(sim,incid[i]));
	}
	return v;
}

const int
Relation::getNumIncidentVariables() const{
	return rel_n_incidences(rel);
}

Instanc
Relation::getInstance() const{
	return Instanc((struct Instance *)rel_instance(rel));
}

string
Relation::getRelationAsString() const{
	if(sim==NULL){
		throw runtime_error("Simulation not set");
	}
	return getInstance().getRelationAsString(sim->getModel());
}

